/**************************************************************************/
/*  openxr_api.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "action_map/openxr_action_map.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/memory.h"
#include "core/types.h"
#include "core/version.h"
#include "openxr_api.h"
#include "openxr_interface.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_tracker.h"

#ifdef ANDROID_ENABLED
#include "core/os/os.h"
#endif

#include "extensions/openxr_extensions.h"
#include "modules/openxr/extensions/openxr_extensions.h"

#include "openxr_platform_inc.h" // IWYU pragma: keep.

#ifdef VULKAN_ENABLED
#include "extensions/platform/openxr_vulkan_extension.h"
#endif

#ifdef METAL_ENABLED
#include "extensions/platform/openxr_metal_extension.h"
#endif

#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
#include "extensions/platform/openxr_opengl_extension.h"
#endif

#ifdef D3D12_ENABLED
#include "extensions/platform/openxr_d3d12_extension.h"
#endif

#ifdef ANDROID_ENABLED
#define OPENXR_LOADER_NAME "libopenxr_loader.so"
#endif

Vector<OpenXRAPI::OpenXRSwapChainInfo> OpenXRAPI::OpenXRSwapChainInfo::free_queue;

bool OpenXRAPI::OpenXRSwapChainInfo::create(XrSwapchainCreateFlags p_create_flags,
	XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, bool p_use_next_extensions)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSession xr_session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(xr_session == XR_NULL_HANDLE, false);

	// We already have a swapchain?
	ERR_FAIL_COND_V(swapchain != XR_NULL_HANDLE, false);

	XrResult result;

	void* next_pointer = nullptr;

	XrSwapchainCreateInfo swapchain_create_info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO, // type
		next_pointer,				   // next
		p_create_flags,				   // createFlags
		p_usage_flags,				   // usageFlags
		p_swapchain_format,			   // format
		p_sample_count,				   // sampleCount
		p_width,					   // width
		p_height,					   // height
		1,							   // faceCount
		p_array_size,				   // arraySize
		1							   // mipCount
	};

	XrSwapchain new_swapchain;
	result = OpenXRAPI::xrCreateSwapchain(xr_session, &swapchain_create_info, &new_swapchain);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain [", OpenXRAPI::get_error_string(result),
		// "]");
		return false;
	}

	swapchain = new_swapchain;

	return true;
}

void OpenXRAPI::OpenXRSwapChainInfo::queue_free()
{
	if (image_acquired) {
		release();
	}

	if (swapchain != XR_NULL_HANDLE) {
		free_queue.push_back(*this);

		swapchain_graphics_data = nullptr;
		swapchain = XR_NULL_HANDLE;
	}
}

void OpenXRAPI::OpenXRSwapChainInfo::free_queued()
{
	for (OpenXRAPI::OpenXRSwapChainInfo& swapchain_info : free_queue) {
		swapchain_info.free();
	}
	free_queue.clear();
}

void OpenXRAPI::OpenXRSwapChainInfo::free()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (image_acquired) {
		release();
	}

	if (swapchain != XR_NULL_HANDLE) {
		OpenXRAPI::xrDestroySwapchain(swapchain);
		swapchain = XR_NULL_HANDLE;
	}
}

bool OpenXRAPI::OpenXRSwapChainInfo::release()
{
	if (!image_acquired) {
		// Already released or never acquired.
		return true;
	}

	image_acquired = false; // Regardless if we succeed or not, consider this released.

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSwapchainImageReleaseInfo swapchain_image_release_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, // type
		nullptr								  // next
	};
	XrResult result = OpenXRAPI::xrReleaseSwapchainImage(swapchain, &swapchain_image_release_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to release swapchain image! [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	return true;
}

bool OpenXRAPI::openxr_is_enabled(bool p_check_run_in_editor)
{
	if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
		if (Engine::is_editor_hint() && p_check_run_in_editor) {
			// For now, don't start OpenXR when the editor starts up. In the future, this may change
			// if we want to integrate more XR features into the editor experience.
			return false;
		}
	}
}

String OpenXRAPI::get_error_string(XrResult result) { return String(); }

void OpenXRAPI::set_object_name(
	XrObjectType p_object_type, uint64_t p_object_handle, const String& p_object_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::set_object_name(p_object_type, p_object_handle, p_object_name.utf8().get_data());
}

void OpenXRAPI::begin_debug_label_region(const String& p_label_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::begin_debug_label_region(p_label_name.utf8().get_data());
}

void OpenXRAPI::end_debug_label_region()
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::end_debug_label_region();
}

void OpenXRAPI::insert_debug_label(const String& p_label_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::insert_debug_label(p_label_name.utf8().get_data());
}

bool OpenXRAPI::load_layer_properties()
{
	// This queries additional layers that are available and can be initialized when we create our
	// OpenXR instance
	if (!data->layer_properties.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_layer_properties = 0;
	XrResult result = xrEnumerateApiLayerProperties(0, &num_layer_properties, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of api layer properties");

	data->layer_properties.resize(num_layer_properties);
	for (XrApiLayerProperties& layer : data->layer_properties) {
		layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		layer.next = nullptr;
	}

	result = xrEnumerateApiLayerProperties(
		num_layer_properties, &num_layer_properties, data->layer_properties.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate api layer properties");

	for (const XrApiLayerProperties& layer : data->layer_properties) {
		print_verbose(vformat("OpenXR: Found OpenXR layer %s.", layer.layerName));
	}

	return true;
}

bool OpenXRAPI::load_supported_extensions()
{
	// This queries supported extensions that are available and can be initialized when we create
	// our OpenXR instance

	if (!data->supported_extensions.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_supported_extensions = 0;
	XrResult result =
		xrEnumerateInstanceExtensionProperties(nullptr, 0, &num_supported_extensions, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of extension properties");

	data->supported_extensions.resize(num_supported_extensions);

	// set our types
	for (XrExtensionProperties& extension : data->supported_extensions) {
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		extension.next = nullptr;
	}
	result = xrEnumerateInstanceExtensionProperties(
		nullptr, num_supported_extensions, &num_supported_extensions, data->supported_extensions.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate extension properties");

	for (const XrExtensionProperties& extension : data->supported_extensions) {
		print_verbose(vformat("OpenXR: Found OpenXR extension %s.", extension.extensionName));
	}

	return true;
}

bool OpenXRAPI::is_extension_supported(const String& p_extension)
{
	for (const XrExtensionProperties& extension : data->supported_extensions) {
		if (extension.extensionName == p_extension) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_any_extension_enabled(const String& p_extensions)
{
	// We allow a comma separated list of extensions here, only one needs to be supported.
	// This allows us to check for extensions that were renamed or that were embedded in core
	// at a specific OpenXR version.
	for (const String& name : p_extensions.split(",", false)) {
		CharString extension = name.utf8();

		if (data->enabled_extensions.has(extension)) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_top_level_path_supported(const String& p_toplevel_path)
{
	String required_extensions =
		OpenXRInteractionProfileMetadata::get_top_level_extensions(
			p_toplevel_path);

	// If unsupported is returned we likely have a misspelled interaction profile path in our action
	// map. Always output that as an error.
	ERR_FAIL_COND_V_MSG(required_extensions == XR_PATH_UNSUPPORTED_NAME, false,
		"OpenXR: Unsupported toplevel path " + p_toplevel_path);

	if (required_extensions == "") {
		// no extension needed, core top level are always "supported", they just won't be used if
		// not really supported
		return true;
	}

	if (!is_any_extension_enabled(required_extensions)) {
		// It is very likely we have top level paths for which the extension is not available so
		// don't flood the logs with unnecessary spam.
		print_verbose("OpenXR: Top level path " + p_toplevel_path + " requires extension " +
					  required_extensions.replace(",", " or "));
		return false;
	}

	return true;
}

bool OpenXRAPI::is_interaction_profile_supported(const String& p_ip_path)
{
	String required_extensions =
		OpenXRInteractionProfileMetadata::get_interaction_profile_extensions(
			p_ip_path);

	// If unsupported is returned we likely have a misspelled interaction profile path in our action
	// map. Always output that as an error.
	ERR_FAIL_COND_V_MSG(required_extensions == XR_PATH_UNSUPPORTED_NAME, false,
		"OpenXR: Unsupported interaction profile " + p_ip_path);

	if (required_extensions == "") {
		// no extension needed, core interaction profiles are always "supported", they just won't be
		// used if not really supported
		return true;
	}

	if (!is_any_extension_enabled(required_extensions)) {
		// It is very likely we have interaction profiles for which the extension is not available
		// so don't flood the logs with unnecessary spam.
		print_verbose("OpenXR: Interaction profile " + p_ip_path + " requires extension " +
					  required_extensions.replace(",", " or "));
		return false;
	}

	return true;
}

bool OpenXRAPI::interaction_profile_supports_io_path(
	const String& p_ip_path, const String& p_io_path)
{
	if (!is_interaction_profile_supported(p_ip_path)) {
		return false;
	}

	const OpenXRInteractionProfileMetadata::IOPath* io_path =
		OpenXRInteractionProfileMetadata::get_io_path(p_ip_path, p_io_path);

	// If the io_path is not part of our metadata we've likely got a misspelled name or a bad action
	// map, report
	ERR_FAIL_NULL_V_MSG(
		io_path, false, "OpenXR: Unsupported io path " + String(p_ip_path) + String(p_io_path));

	if (io_path->openxr_extension_names == "") {
		// no extension needed, core io paths are always "supported", they just won't be used if not
		// really supported
		return true;
	}

	if (!is_any_extension_enabled(io_path->openxr_extension_names)) {
		// It is very likely we have io paths for which the extension is not available so don't
		// flood the logs with unnecessary spam.
		print_verbose("OpenXR: IO path " + String(p_ip_path) + String(p_io_path) +
					  " requires extension " +
					  io_path->openxr_extension_names.replace(",", " or "));
		return false;
	}

	return true;
}

void OpenXRAPI::copy_string_to_char_buffer(const String& p_string, char* p_buffer, int p_buffer_len)
{
	CharString char_string = p_string.utf8();
	int len = char_string.length();
	if (len < p_buffer_len - 1) {
		// was having weird CI issues with strcpy so....
		memcpy(p_buffer, char_string.get_data(), len);
		p_buffer[len] = '\0';
	}
	else {
		memcpy(p_buffer, char_string.get_data(), p_buffer_len - 1);
		p_buffer[p_buffer_len - 1] = '\0';
	}
}

bool OpenXRAPI::load_supported_view_configuration_types()
{
	// This queries the supported configuration types, likely there will only be one choosing
	// between Mono (phone AR) and Stereo (HMDs)

	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	data->supported_view_configuration_types.clear();

	uint32_t num_view_configuration_types = 0;
	XrResult result = xrEnumerateViewConfigurations(
		data->instance, data->system_id, 0, &num_view_configuration_types, nullptr);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to get view configuration count [", get_error_string(result), "]");
		return false;
	}

	data->supported_view_configuration_types.resize(num_view_configuration_types);

	result = xrEnumerateViewConfigurations(data->instance, data->system_id, num_view_configuration_types,
		&num_view_configuration_types, data->supported_view_configuration_types.ptr());
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerateview configurations");
	ERR_FAIL_COND_V_MSG(num_view_configuration_types == 0, false,
		"OpenXR: Failed to enumerateview configurations"); // JIC there should be at least 1!

	for (const XrViewConfigurationType& view_configuration_type :
		data->supported_view_configuration_types) {
		print_verbose(vformat("OpenXR: Found supported view configuration %s.",
			OpenXRUtil::get_view_configuration_name(view_configuration_type)));
	}

	// Check value we loaded at startup...
	if (!is_view_configuration_supported(data->view_configuration)) {
		print_verbose(vformat("OpenXR: %s isn't supported, defaulting to %s.",
			OpenXRUtil::get_view_configuration_name(data->view_configuration),
			OpenXRUtil::get_view_configuration_name(data->supported_view_configuration_types[0])));

		data->view_configuration = data->supported_view_configuration_types[0];
	}

	return true;
}

bool OpenXRAPI::load_supported_environmental_blend_modes()
{
	// This queries the supported environmental blend modes.

	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	data->supported_environment_blend_modes.clear();

	uint32_t num_supported_environment_blend_modes = 0;
	XrResult result = xrEnumerateEnvironmentBlendModes(data->instance, data->system_id, data->view_configuration, 0,
		&num_supported_environment_blend_modes, nullptr);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get supported environmental blend mode count [",
		// 	get_error_string(result), "]");
		return false;
	}

	data->supported_environment_blend_modes.resize(num_supported_environment_blend_modes);

	result = xrEnumerateEnvironmentBlendModes(data->instance, data->system_id, data->view_configuration,
		num_supported_environment_blend_modes, &num_supported_environment_blend_modes,
		data->supported_environment_blend_modes.ptrw());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate environmental blend modes");
	ERR_FAIL_COND_V_MSG(num_supported_environment_blend_modes == 0, false,
		"OpenXR: Failed to enumerate environmental blend modes"); // JIC there should be at least 1!

	for (const XrEnvironmentBlendMode& supported_environment_blend_mode :
		data->supported_environment_blend_modes) {
		print_verbose(vformat("OpenXR: Found environmental blend mode %s.",
			OpenXRUtil::get_environment_blend_mode_name(supported_environment_blend_mode)));
	}

	return true;
}

bool OpenXRAPI::is_view_configuration_supported(XrViewConfigurationType p_configuration_type)
{
	return data->supported_view_configuration_types.has(p_configuration_type);
}

bool OpenXRAPI::is_reference_space_supported(XrReferenceSpaceType p_reference_space)
{
	return data->supported_reference_spaces.has(p_reference_space);
}

bool OpenXRAPI::is_swapchain_format_supported(int64_t p_swapchain_format)
{
	return data->supported_swapchain_formats.has(p_swapchain_format);
}

void OpenXRAPI::set_form_factor(XrFormFactor p_form_factor)
{
	ERR_FAIL_COND(is_initialized());

	data->form_factor = p_form_factor;
}

uint32_t OpenXRAPI::get_view_count() { return data->view_configuration_views.size(); }

void OpenXRAPI::set_view_configuration(XrViewConfigurationType p_view_configuration)
{
	ERR_FAIL_COND(is_initialized());

	data->view_configuration = p_view_configuration;
}

bool OpenXRAPI::set_requested_reference_space(XrReferenceSpaceType p_requested_reference_space)
{
	if (data->custom_play_space != XR_NULL_HANDLE) {
		return false;
	}

	data->requested_reference_space = p_requested_reference_space;
	data->play_space_is_dirty = true;

	return true;
}

void OpenXRAPI::set_custom_play_space(XrSpace p_custom_space)
{
	data->custom_play_space = p_custom_space;
	data->play_space_is_dirty = true;
}

void OpenXRAPI::set_submit_depth_buffer(bool p_submit_depth_buffer)
{
	ERR_FAIL_COND(is_initialized());

	data->submit_depth_buffer = p_submit_depth_buffer;
}

bool OpenXRAPI::is_initialized() { return data != nullptr && (data->instance != XR_NULL_HANDLE); }

bool OpenXRAPI::is_running()
{
	if (data->instance == XR_NULL_HANDLE) {
		return false;
	}
	if (data->session == XR_NULL_HANDLE) {
		return false;
	}

	return data->running;
}

bool OpenXRAPI::openxr_loader_init()
{
#ifdef ANDROID_ENABLED
	ERR_FAIL_COND_V_MSG(
		data->openxr_loader_library_handle != nullptr, false, "OpenXR Loader library is already loaded.");

	{
		Error error_code = OS::open_dynamic_library(
			OPENXR_LOADER_NAME, data->openxr_loader_library_handle);
		ERR_FAIL_COND_V_MSG(error_code != OK, false, "OpenXR loader not found.");
	}

	{
		Error error_code = OS::get_dynamic_library_symbol_handle(
			data->openxr_loader_library_handle, "xrGetInstanceProcAddr", (void*&)data->xrGetInstanceProcAddr);
		ERR_FAIL_COND_V_MSG(error_code != OK, false,
			"Symbol xrGetInstanceProcAddr not found in OpenXR Loader library.");
	}
#endif

	// Resolve the symbols that don't require an instance
	OPENXR_API_INIT_XR_FUNC_V(xrCreateInstance);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateApiLayerProperties);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateInstanceExtensionProperties);

	return true;
}

bool OpenXRAPI::resolve_instance_openxr_symbols()
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	OPENXR_API_INIT_XR_FUNC_V(xrAcquireSwapchainImage);
	OPENXR_API_INIT_XR_FUNC_V(xrApplyHapticFeedback);
	OPENXR_API_INIT_XR_FUNC_V(xrAttachSessionActionSets);
	OPENXR_API_INIT_XR_FUNC_V(xrBeginFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrBeginSession);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateAction);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateActionSet);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateActionSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateReferenceSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateSession);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateSwapchain);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyAction);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyActionSet);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyInstance);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySession);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySpace);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySwapchain);
	OPENXR_API_INIT_XR_FUNC_V(xrEndFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrEndSession);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateEnvironmentBlendModes);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateReferenceSpaces);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateSwapchainFormats);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateViewConfigurations);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateViewConfigurationViews);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateBoolean);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateFloat);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateVector2f);
	OPENXR_API_INIT_XR_FUNC_V(xrGetCurrentInteractionProfile);
	OPENXR_API_INIT_XR_FUNC_V(xrGetReferenceSpaceBoundsRect);
	OPENXR_API_INIT_XR_FUNC_V(xrGetSystem);
	OPENXR_API_INIT_XR_FUNC_V(xrGetSystemProperties);
	OPENXR_API_INIT_XR_FUNC_V(xrLocateViews);
	OPENXR_API_INIT_XR_FUNC_V(xrLocateSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrPathToString);
	OPENXR_API_INIT_XR_FUNC_V(xrPollEvent);
	OPENXR_API_INIT_XR_FUNC_V(xrReleaseSwapchainImage);
	OPENXR_API_INIT_XR_FUNC_V(xrResultToString);
	OPENXR_API_INIT_XR_FUNC_V(xrStringToPath);
	OPENXR_API_INIT_XR_FUNC_V(xrSuggestInteractionProfileBindings);
	OPENXR_API_INIT_XR_FUNC_V(xrSyncActions);
	OPENXR_API_INIT_XR_FUNC_V(xrWaitFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrWaitSwapchainImage);

	return true;
}

XrResult OpenXRAPI::try_get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr)
{
	#ifdef ANDROID_ENABLED
	return data->xrGetInstanceProcAddr(data->instance, p_name, p_addr);
#else
	return ::xrGetInstanceProcAddr(data->instance, p_name, p_addr);
#endif
}

XrResult OpenXRAPI::get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr)
{
	XrResult result = try_get_instance_proc_addr(p_name, p_addr);

	if (result != XR_SUCCESS) {
		String error_message = String("Symbol ") + p_name + " not found in OpenXR instance.";
		ERR_FAIL_V_MSG(result, error_message.utf8().get_data());
	}

	return result;
}

bool OpenXRAPI::initialize_session()
{
	if (!create_session()) {
		destroy_session();
		return false;
	}

	if (!load_supported_reference_spaces()) {
		destroy_session();
		return false;
	}

	if (!setup_view_space()) {
		destroy_session();
		return false;
	}

	if (!load_supported_swapchain_formats()) {
		destroy_session();
		return false;
	}

	if (!obtain_swapchain_formats()) {
		destroy_session();
		return false;
	}

	allocate_view_buffers(data->view_configuration_views.size(), data->submit_depth_buffer);

	return true;
}

void OpenXRAPI::finish()
{
	destroy_session();

	destroy_instance();
}

void OpenXRAPI::set_xr_interface(OpenXRInterface* p_xr_interface) { data->xr_interface = p_xr_interface; }

Size2 OpenXRAPI::get_recommended_target_size()
{
	ERR_FAIL_COND_V(data->view_configuration_views.is_empty(), Size2());

	Size2 target_size;

	if (Renderer::data && Renderer::is_on_render_thread()) {
		target_size.width = data->view_configuration_views[0].recommendedImageRectWidth *
							data->render_state.render_target_size_multiplier;
		target_size.height = data->view_configuration_views[0].recommendedImageRectHeight *
							 data->render_state.render_target_size_multiplier;
	}
	else {
		target_size.width =
			data->view_configuration_views[0].recommendedImageRectWidth * data->render_target_size_multiplier;
		target_size.height =
			data->view_configuration_views[0].recommendedImageRectHeight * data->render_target_size_multiplier;
	}

	return target_size;
}

XRPose::TrackingConfidence OpenXRAPI::get_head_center(
	Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	XrResult result;

	if (!data->running) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	// Get display time
	XrTime display_time = get_predicted_display_time();
	if (display_time == 0) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr,				// next
		0,						// velocityFlags
		{0.0, 0.0, 0.0},		// linearVelocity
		{0.0, 0.0, 0.0}			// angularVelocity
	};

	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity,				// next
		0,						// locationFlags
		{
			{0.0, 0.0, 0.0, 0.0}, // orientation
			{0.0, 0.0, 0.0}		  // position
		}						  // pose
	};

	result = xrLocateSpace(data->view_space, data->play_space, display_time, &location);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to locate view space in play space [", get_error_string(result), "]");
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XRPose::TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	if (data->head_pose_confidence != confidence) {
		// prevent error spam
		data->head_pose_confidence = confidence;
		if (data->head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
			// print_line("OpenXR head space location not valid (check tracking?)");
		}
		else if (data->head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_LOW) {
			print_verbose("OpenVR Head pose now tracking with low confidence");
		}
		else {
			print_verbose("OpenVR Head pose now tracking with high confidence");
		}
	}

	return confidence;
}

bool OpenXRAPI::get_view_transform(uint32_t p_view, Transform3D& r_transform)
{
	ERR_NOT_ON_RENDER_THREAD_V(false);

	if (!data->render_state.running) {
		return false;
	}

	// we don't have valid view info
	if (data->render_state.views.is_empty() || !data->render_state.view_pose_valid) {
		return false;
	}

	// Note, the timing of this is set right before rendering, which is what we need here.
	r_transform = transform_from_pose(data->render_state.views[p_view].pose);

	return true;
}

void OpenXRAPI::_allocate_view_buffers_rt(uint32_t p_view_count, bool p_submit_depth_buffer)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	data->render_state.submit_depth_buffer = p_submit_depth_buffer;

	// Allocate buffers we'll be populating with view information.
	data->render_state.views.resize(p_view_count);
	data->render_state.projection_views.resize(p_view_count);

	for (uint32_t i = 0; i < p_view_count; i++) {
		data->render_state.views[i] = {
			XR_TYPE_VIEW, // type
			nullptr,	  // next
			{},			  // pose
			{},			  // fov
		};
		data->render_state.projection_views[i] = {
			XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, // type
			nullptr,								   // next
			{},										   // pose
			{},										   // fov
			{},										   // subImage
		};
	}

	if (p_submit_depth_buffer &&
		OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_COMPOSITION_LAYER_DEPTH)) {
		data->render_state.depth_views.resize(p_view_count);

		for (uint32_t i = 0; i < p_view_count; i++) {
			data->render_state.depth_views[i] = {
				XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR, // type
				nullptr,								  // next
				{},										  // subImage
				0.0,									  // minDepth
				0.0,									  // maxDepth
				0.0,									  // nearZ
				0.0,									  // farZ
			};
		}
	}
}

void OpenXRAPI::_set_render_session_running_rt(bool p_is_running)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.running = p_is_running;
}

void OpenXRAPI::_set_render_display_info_rt(XrTime p_predicted_display_time, bool p_should_render)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.predicted_display_time = p_predicted_display_time;
	data->render_state.should_render = p_should_render;
}

void OpenXRAPI::_set_render_environment_blend_mode_rt(int32_t p_environment_blend_mode)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.environment_blend_mode =
		XrEnvironmentBlendMode(p_environment_blend_mode);
}

void OpenXRAPI::_set_render_play_space_rt(uint64_t p_play_space)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.play_space = XrSpace(p_play_space);
}

void OpenXRAPI::_set_render_state_multiplier_rt(double p_render_target_size_multiplier)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.render_target_size_multiplier = p_render_target_size_multiplier;
}

void OpenXRAPI::_set_render_state_render_region_rt(const Rect2i& p_render_region)
{
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.render_region = p_render_region;
}

void OpenXRAPI::_update_main_swapchain_size_rt()
{
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	uint32_t view_count_output = 0;
	XrResult result = OpenXRAPI::xrEnumerateViewConfigurationViews(data->instance,
		data->system_id, data->view_configuration, OpenXRAPI::get_view_count(),
		&view_count_output, data->view_configuration_views.ptr());
	if (XR_FAILED(result)) {
		return;
	}

#ifdef DEBUG_ENABLED
	for (uint32_t i = 0; i < view_count_output; i++) {
		print_verbose("OpenXR: Recommended resolution changed");
		print_verbose(String(" - recommended render width: ") +
					  itos(data->view_configuration_views[i].recommendedImageRectWidth));
		print_verbose(String(" - recommended render height: ") +
					  itos(data->view_configuration_views[i].recommendedImageRectHeight));
	}
#endif
}

void OpenXRAPI::free_main_swapchains()
{
	for (int i = 0; i < OPENXR_SWAPCHAIN_MAX; i++) {
		data->render_state.main_swapchains[i].queue_free();
	}
}

XrSwapchain OpenXRAPI::get_color_swapchain()
{
	ERR_NOT_ON_RENDER_THREAD_V(XR_NULL_HANDLE);

	return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_swapchain();
}

RID OpenXRAPI::get_color_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_image();
}

RID OpenXRAPI::get_depth_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	// Note, image will not be acquired if we didn't have a suitable swap chain format.
	if (data->render_state.submit_depth_buffer &&
		data->render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].is_image_acquired()) {
		return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].get_image();
	}
	else {
		return RID();
	}
}

RID OpenXRAPI::get_density_map_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_density_map();
	}

	return RID();
}

void OpenXRAPI::set_velocity_texture(RID p_render_target) { data->velocity_texture = p_render_target; }

RID OpenXRAPI::get_velocity_texture() { return data->velocity_texture; }

void OpenXRAPI::set_velocity_depth_texture(RID p_render_target)
{
	data->velocity_depth_texture = p_render_target;
}

RID OpenXRAPI::get_velocity_depth_texture() { return data->velocity_depth_texture; }

void OpenXRAPI::set_velocity_target_size(const Size2i& p_target_size)
{
	data->velocity_target_size = p_target_size;
}

Size2i OpenXRAPI::get_velocity_target_size() { return data->velocity_target_size; }

const XrCompositionLayerProjection* OpenXRAPI::get_projection_layer()
{
	ERR_NOT_ON_RENDER_THREAD_V(nullptr);

	return &data->render_state.projection_layer;
}

float OpenXRAPI::get_display_refresh_rate()
{
	if (OpenXRDisplayRefreshRateExtension::is_initialized()) {
		return OpenXRDisplayRefreshRateExtension::get_refresh_rate();
	}

	return 0.0;
}

void OpenXRAPI::set_display_refresh_rate(float p_refresh_rate)
{
	if (OpenXRDisplayRefreshRateExtension::is_initialized()) {
		OpenXRDisplayRefreshRateExtension::set_refresh_rate(p_refresh_rate);
	}
}

double OpenXRAPI::get_render_target_size_multiplier()
{
	return data->render_target_size_multiplier;
}

void OpenXRAPI::set_render_target_size_multiplier(double multiplier)
{
	data->render_target_size_multiplier = multiplier;
	set_render_state_multiplier(multiplier);
}

Rect2i OpenXRAPI::get_render_region() { return data->render_region; }

void OpenXRAPI::set_render_region(const Rect2i& p_render_region)
{
	data->render_region = p_render_region;
	set_render_state_render_region(p_render_region);
}

bool OpenXRAPI::is_foveation_supported()
{
	return OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled();
}

int OpenXRAPI::get_foveation_level()
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		switch (OpenXRFBFoveationExtension::get_foveation_level()) {
		case XR_FOVEATION_LEVEL_NONE_FB:
			return 0;
		case XR_FOVEATION_LEVEL_LOW_FB:
			return 1;
		case XR_FOVEATION_LEVEL_MEDIUM_FB:
			return 2;
		case XR_FOVEATION_LEVEL_HIGH_FB:
			return 3;
		default:
			return 0;
		}
	}

	return 0;
}

void OpenXRAPI::set_foveation_level(int p_foveation_level)
{
	ERR_FAIL_UNSIGNED_INDEX(p_foveation_level, 4);
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		XrFoveationLevelFB levels[] = {XR_FOVEATION_LEVEL_NONE_FB, XR_FOVEATION_LEVEL_LOW_FB,
			XR_FOVEATION_LEVEL_MEDIUM_FB, XR_FOVEATION_LEVEL_HIGH_FB};
		OpenXRFBFoveationExtension::set_foveation_level(levels[p_foveation_level]);
	}
}

bool OpenXRAPI::get_foveation_dynamic()
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		return OpenXRFBFoveationExtension::get_foveation_dynamic() == XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB;
	}
	return false;
}

void OpenXRAPI::set_foveation_dynamic(bool p_foveation_dynamic)
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		OpenXRFBFoveationExtension::set_foveation_dynamic(p_foveation_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB
														   : XR_FOVEATION_DYNAMIC_DISABLED_FB);
	}
}

bool OpenXRAPI::get_foveation_with_subsampled_images()
{
	if (OpenXRFBFoveationExtension::is_initialized()) {
		return OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled();
	}
	return false;
}

void OpenXRAPI::set_foveation_with_subsampled_images(bool p_enabled)
{
	if (OpenXRFBFoveationExtension::is_initialized()) {
		OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(p_enabled);
	}
}

PackedInt64Array OpenXRAPI::get_supported_swapchain_formats()
{
	return data->supported_swapchain_formats;
}

Transform3D OpenXRAPI::transform_from_pose(const XrPosef& p_pose)
{
	Quaternion q(
		p_pose.orientation.x, p_pose.orientation.y, p_pose.orientation.z, p_pose.orientation.w);
	Basis basis(q);
	Vector3 origin(p_pose.position.x, p_pose.position.y, p_pose.position.z);

	return Transform3D(basis, origin);
}

XrPosef OpenXRAPI::pose_from_transform(const Transform3D& p_transform)
{
	XrPosef pose;

	Quaternion q(p_transform.basis);
	pose.orientation.x = q.x;
	pose.orientation.y = q.y;
	pose.orientation.z = q.z;
	pose.orientation.w = q.w;

	pose.position.x = p_transform.origin.x;
	pose.position.y = p_transform.origin.y;
	pose.position.z = p_transform.origin.z;

	return pose;
}

template <typename T>
XRPose::TrackingConfidence _transform_from_location(const T& p_location, Transform3D& r_transform)
{
	XRPose::TrackingConfidence confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;
	const XrPosef& pose = p_location.pose;

	// Check orientation
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quaternion q(
			pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		r_transform.basis = Basis(q);

		if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) {
			// Fully valid orientation, so either 3DOF or 6DOF tracking with high confidence so
			// default to HIGH_TRACKING
			confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
		}
		else {
			// Orientation is being tracked but we're using old/predicted data, so low tracking
			// confidence
			confidence = XRPose::XR_TRACKING_CONFIDENCE_LOW;
		}
	}
	else {
		r_transform.basis = Basis();
	}

	// Check location
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		r_transform.origin = Vector3(pose.position.x, pose.position.y, pose.position.z);

		if (!(p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) {
			// Location is being tracked but we're using old/predicted data, so low tracking
			// confidence
			confidence = XRPose::XR_TRACKING_CONFIDENCE_LOW;
		}
		else if (confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
			// Position tracking without orientation tracking?
			confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
		}
	}
	else {
		// No tracking or 3DOF I guess..
		r_transform.origin = Vector3();
	}

	return confidence;
}

XRPose::TrackingConfidence OpenXRAPI::transform_from_location(
	const XrSpaceLocation& p_location, Transform3D& r_transform)
{
	return _transform_from_location(p_location, r_transform);
}

XRPose::TrackingConfidence OpenXRAPI::transform_from_location(
	const XrHandJointLocationEXT& p_location, Transform3D& r_transform)
{
	return _transform_from_location(p_location, r_transform);
}

void OpenXRAPI::parse_velocities(
	const XrSpaceVelocity& p_velocity, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
		XrVector3f linear_velocity = p_velocity.linearVelocity;
		r_linear_velocity = Vector3(linear_velocity.x, linear_velocity.y, linear_velocity.z);
	}
	else {
		r_linear_velocity = Vector3();
	}
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) {
		XrVector3f angular_velocity = p_velocity.angularVelocity;
		r_angular_velocity = Vector3(angular_velocity.x, angular_velocity.y, angular_velocity.z);
	}
	else {
		r_angular_velocity = Vector3();
	}
}

XrPath OpenXRAPI::get_xr_path(const String& p_path) { return XrPath(); }

String OpenXRAPI::get_xr_path_name(const XrPath& p_path)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, String());

	uint32_t size = 0;
	char path_name[XR_MAX_PATH_LENGTH];

	XrResult result = xrPathToString(data->instance, p_path, XR_MAX_PATH_LENGTH, &size, path_name);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			String(), "OpenXR: failed to get name for a path! [" + get_error_string(result) + "]");
	}

	return String(path_name);
}

RID OpenXRAPI::get_tracker_rid(XrPath p_path)
{
	for (const RID& tracker_rid : data->tracker_owner.get_owned_list()) {
		Tracker* tracker = data->tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->toplevel_path == p_path) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_tracker(const String& p_name)
{
	for (const RID& tracker_rid : data->tracker_owner.get_owned_list()) {
		Tracker* tracker = data->tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->name == p_name) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::tracker_create(const String& p_name)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());

	Tracker new_tracker;
	new_tracker.name = p_name;
	new_tracker.toplevel_path = XR_NULL_PATH;
	new_tracker.active_profile_rid = RID();

	new_tracker.toplevel_path = get_xr_path(p_name);
	ERR_FAIL_COND_V(new_tracker.toplevel_path == XR_NULL_PATH, RID());

	return data->tracker_owner.make_rid(new_tracker);
}

String OpenXRAPI::tracker_get_name(RID p_tracker)
{
	if (p_tracker.is_null()) {
		return String("None");
	}

	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, String());

	return tracker->name;
}

void OpenXRAPI::tracker_check_profile(RID p_tracker, XrSession p_session)
{
	if (p_session == XR_NULL_HANDLE) {
		p_session = data->session;
	}

	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL(tracker);

	if (tracker->toplevel_path == XR_NULL_PATH) {
		// no path, how was this even created?
		return;
	}

	XrInteractionProfileState profile_state = {
		XR_TYPE_INTERACTION_PROFILE_STATE, // type
		nullptr,						   // next
		XR_NULL_PATH					   // interactionProfile
	};

	XrResult result =
		xrGetCurrentInteractionProfile(p_session, tracker->toplevel_path, &profile_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get interaction profile for", itos(tracker->toplevel_path),
		// 	"[", get_error_string(result), "]");
		return;
	}

	XrPath new_profile = profile_state.interactionProfile;
	XrPath was_profile = get_interaction_profile_path(tracker->active_profile_rid);
	if (was_profile != new_profile) {
		tracker->active_profile_rid = get_interaction_profile_rid(new_profile);

		if (data->xr_interface) {
			data->xr_interface->tracker_profile_changed(p_tracker, tracker->active_profile_rid);
		}
	}
}

void OpenXRAPI::tracker_free(RID p_tracker)
{
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL(tracker);

	// there is nothing to free here

	data->tracker_owner.free(p_tracker);
}

RID OpenXRAPI::action_set_create(
	const String& p_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());
	ActionSet action_set;

	action_set.name = p_name;
	action_set.is_attached = false;

	// create our action set...
	XrActionSetCreateInfo action_set_info = {
		XR_TYPE_ACTION_SET_CREATE_INFO, // type
		nullptr,						// next
		"",								// actionSetName
		"",								// localizedActionSetName
		uint32_t(p_priority)			// priority
	};

	copy_string_to_char_buffer(p_name, action_set_info.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE);
	copy_string_to_char_buffer(p_localized_name, action_set_info.localizedActionSetName,
		XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);

	XrResult result = xrCreateActionSet(data->instance, &action_set_info, &action_set.handle);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: failed to create action set ", p_name, "! [", get_error_string(result), "]");
		return RID();
	}

	set_object_name(XR_OBJECT_TYPE_ACTION_SET, uint64_t(action_set.handle), p_name);

	return data->action_set_owner.make_rid(action_set);
}

RID OpenXRAPI::find_action_set(const String& p_name)
{
	for (const RID& action_set_rid : data->action_set_owner.get_owned_list()) {
		ActionSet* action_set = data->action_set_owner.get_or_null(action_set_rid);
		if (action_set && action_set->name == p_name) {
			return action_set_rid;
		}
	}

	return RID();
}

String OpenXRAPI::action_set_get_name(RID p_action_set)
{
	if (p_action_set.is_null()) {
		return String("None");
	}

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, String());

	return action_set->name;
}

XrActionSet OpenXRAPI::action_set_get_handle(RID p_action_set)
{
	if (p_action_set.is_null()) {
		return XR_NULL_HANDLE;
	}

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, XR_NULL_HANDLE);

	return action_set->handle;
}

bool OpenXRAPI::attach_action_sets(const Vector<RID>& p_action_sets)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);

	Vector<XrActionSet> action_handles;
	action_handles.resize(p_action_sets.size());
	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = data->action_set_owner.get_or_null(p_action_sets[i]);
		ERR_FAIL_NULL_V(action_set, false);

		if (action_set->is_attached) {
			return false;
		}

		action_handles.set(i, action_set->handle);
	}

	// So according to the docs, once we attach our action set to our session it becomes read only..
	// https://www.khronos.org/registry/OpenXR/specs/1.0/man/html/xrAttachSessionActionSets.html
	XrSessionActionSetsAttachInfo attach_info = {
		XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, // type
		nullptr,								 // next
		(uint32_t)p_action_sets.size(),			 // countActionSets,
		action_handles.ptr()					 // actionSets
	};

	XrResult result = xrAttachSessionActionSets(data->session, &attach_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to attach action sets! [", get_error_string(result), "]");
		return false;
	}

	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = data->action_set_owner.get_or_null(p_action_sets[i]);
		ERR_FAIL_NULL_V(action_set, false);
		action_set->is_attached = true;
	}

	/* For debugging:
	print_verbose("Attached set " + action_set->name);
	List<RID> action_rids;
	action_owner.get_owned_list(&action_rids);
	for (int i = 0; i < action_rids.size(); i++) {
		Action * action = action_owner.get_or_null(action_rids[i]);
		if (action && action->action_set_rid == p_action_set) {
			print_verbose(" - Action " + action->name + ": " +
	OpenXRUtil::get_action_type_name(action->action_type)); for (int j = 0; j <
	action->trackers.size(); j++) { Tracker * tracker =
	tracker_owner.get_or_null(action->trackers[j].tracker_rid); if (tracker) { print_verbose("    -
	" + tracker->name);
				}
			}
		}
	}
	*/

	return true;
}

void OpenXRAPI::action_set_free(RID p_action_set)
{
	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL(action_set);

	if (action_set->handle != XR_NULL_HANDLE) {
		xrDestroyActionSet(action_set->handle);
	}

	data->action_set_owner.free(p_action_set);
}

RID OpenXRAPI::get_action_rid(XrAction p_action)
{
	for (const RID& action_rid : data->action_owner.get_owned_list()) {
		Action* action = data->action_owner.get_or_null(action_rid);
		if (action && action->handle == p_action) {
			return action_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_action(const String& p_name, const RID& p_action_set)
{
	for (const RID& action_rid : data->action_owner.get_owned_list()) {
		Action* action = data->action_owner.get_or_null(action_rid);
		if (action && action->name == p_name &&
			(p_action_set.is_null() || action->action_set_rid == p_action_set)) {
			return action_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::action_create(RID p_action_set, const String& p_name, const String& p_localized_name,
	OpenXRAction::ActionType p_action_type, const Vector<RID>& p_trackers)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());

	Action action;
	action.name = p_name;

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, RID());
	ERR_FAIL_COND_V(action_set->handle == XR_NULL_HANDLE, RID());
	action.action_set_rid = p_action_set;

	switch (p_action_type) {
	case OpenXRAction::OPENXR_ACTION_BOOL:
		action.action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_FLOAT:
		action.action_type = XR_ACTION_TYPE_FLOAT_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_VECTOR2:
		action.action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_POSE:
		action.action_type = XR_ACTION_TYPE_POSE_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_HAPTIC:
		action.action_type = XR_ACTION_TYPE_VIBRATION_OUTPUT;
		break;
	default:
		ERR_FAIL_V(RID());
		break;
	}

	Vector<XrPath> toplevel_paths;
	for (int i = 0; i < p_trackers.size(); i++) {
		Tracker* tracker = data->tracker_owner.get_or_null(p_trackers[i]);
		if (tracker != nullptr && tracker->toplevel_path != XR_NULL_PATH) {
			ActionTracker action_tracker = {
				p_trackers[i],	// tracker
				XR_NULL_HANDLE, // space
				false			// was_location_valid
			};
			action.trackers.push_back(action_tracker);

			toplevel_paths.push_back(tracker->toplevel_path);
		}
	}

	XrActionCreateInfo action_info = {
		XR_TYPE_ACTION_CREATE_INFO,		 // type
		nullptr,						 // next
		"",								 // actionName
		action.action_type,				 // actionType
		uint32_t(toplevel_paths.size()), // countSubactionPaths
		toplevel_paths.ptr(),			 // subactionPaths
		""								 // localizedActionName
	};

	copy_string_to_char_buffer(p_name, action_info.actionName, XR_MAX_ACTION_NAME_SIZE);
	copy_string_to_char_buffer(
		p_localized_name, action_info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

	XrResult result = xrCreateAction(action_set->handle, &action_info, &action.handle);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: failed to create action ", p_name, "! [", get_error_string(result), "]");
		return RID();
	}

	set_object_name(XR_OBJECT_TYPE_ACTION, uint64_t(action.handle), p_name);

	return data->action_owner.make_rid(action);
}

String OpenXRAPI::action_get_name(RID p_action)
{
	if (p_action.is_null()) {
		return String("None");
	}

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, String());

	return action->name;
}

XrAction OpenXRAPI::action_get_handle(RID p_action)
{
	if (p_action.is_null()) {
		return XR_NULL_HANDLE;
	}

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XR_NULL_HANDLE);

	return action->handle;
}

void OpenXRAPI::action_free(RID p_action)
{
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL(action);

	if (action->handle != XR_NULL_HANDLE) {
		xrDestroyAction(action->handle);
	}

	data->action_owner.free(p_action);
}

RID OpenXRAPI::get_interaction_profile_rid(XrPath p_path)
{
	for (const RID& ip_rid : data->interaction_profile_owner.get_owned_list()) {
		InteractionProfile* ip = data->interaction_profile_owner.get_or_null(ip_rid);
		if (ip && ip->path == p_path) {
			return ip_rid;
		}
	}

	return RID();
}

XrPath OpenXRAPI::get_interaction_profile_path(RID p_interaction_profile)
{
	if (p_interaction_profile.is_null()) {
		return XR_NULL_PATH;
	}

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, XR_NULL_PATH);

	return ip->path;
}

RID OpenXRAPI::interaction_profile_create(const String& p_name)
{
	if (!is_interaction_profile_supported(p_name)) {
		// The extension enabling this path must not be active, we will silently skip this
		// interaction profile
		return RID();
	}

	InteractionProfile new_interaction_profile;

	new_interaction_profile.internal_name = get_interaction_profile_internal_name(p_name);

	XrResult result = xrStringToPath(
		data->instance, new_interaction_profile.internal_name.get_data(), &new_interaction_profile.path);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to get path for ", p_name, "! [", get_error_string(result),
		// "]");
		return RID();
	}

	RID existing_ip = get_interaction_profile_rid(new_interaction_profile.path);
	if (existing_ip.is_valid()) {
		return existing_ip;
	}

	new_interaction_profile.name = p_name;
	return data->interaction_profile_owner.make_rid(new_interaction_profile);
}

String OpenXRAPI::interaction_profile_get_name(RID p_interaction_profile)
{
	if (p_interaction_profile.is_null()) {
		return String("None");
	}

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, String());

	return ip->name;
}

void OpenXRAPI::interaction_profile_clear_bindings(RID p_interaction_profile)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
}

CharString OpenXRAPI::get_interaction_profile_internal_name(
	const String& p_interaction_profile_name)
{
	CharString internal_name = p_interaction_profile_name.utf8();

	if (data->openxr_version < XR_API_VERSION_1_1_0) {
		// These interaction profiles were renamed in OpenXR 1.1,
		// if we don't support OpenXR 1.1, rename them back.
		if (internal_name == "/interaction_profiles/meta/touch_pro_controller") {
			return "/interaction_profiles/facebook/touch_controller_pro";
		}
		else if (internal_name == "/interaction_profiles/meta/touch_plus_controller") {
			return "/interaction_profiles/meta/touch_controller_plus";
		}
	}

	return internal_name;
}

const char* OpenXRAPI::check_profile_path(
	const CharString& p_interaction_profile_name, const char* p_path)
{
	// We store the new names of these paths in our action map, so if we're on older versions of
	// OpenXR, we need to use the old names.

	struct RenameMap
	{
		const XrVersion
			before_version;	 // If we're on an older version of OpenXR than this (if none zero).
		const char* from;	 // Rename from this
		const char* to;		 // to this
		const char* profile; // limiting to this profile (unless nullptr)
	};

	// The order of entries is important as we early exit when we encounter a before_version value
	// before our current value (excluding 0).
	const RenameMap renames[] = {
		// The touch_controller is an exception where it is still using the vendor names because
		// they were introduced after OpenXR 1.1 was defined.
		{0, "/user/hand/left/input/trigger/proximity", "/user/hand/left/input/trigger/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_fb/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_fb/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},

		// Once applicable, add XR_API_VERSION_1_2 here.

		// Before OpenXR 1.1 we're using palm_ext/pose (we're checking for enabled palm pose
		// extension elsewhere).
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/grip_surface/pose",
			"/user/hand/left/input/palm_ext/pose", nullptr},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/grip_surface/pose",
			"/user/hand/right/input/palm_ext/pose", nullptr},

		// Specific renames for touch_controller_pro, note that we would have already renamed it to
		// the old name.
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/grip_surface/pose",
			"/user/hand/left/input/palm_ext/pose",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/grip_surface/pose",
			"/user/hand/right/input/palm_ext/pose",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/stylus/force",
			"/user/hand/left/input/stylus_fb/force",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/stylus/force",
			"/user/hand/right/input/stylus_fb/force",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger/proximity",
			"/user/hand/left/input/trigger/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/output/haptic_trigger",
			"/user/hand/left/output/haptic_trigger_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/output/haptic_trigger",
			"/user/hand/right/output/haptic_trigger_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/output/haptic_thumb",
			"/user/hand/left/output/haptic_thumb_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/output/haptic_thumb",
			"/user/hand/right/output/haptic_thumb_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_fb/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_fb/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_curl/value",
			"/user/hand/left/input/trigger/curl_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_curl/value",
			"/user/hand/right/input/trigger/curl_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_slide/value",
			"/user/hand/left/input/trigger/slide_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_slide/value",
			"/user/hand/right/input/trigger/slide_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},

		// Specific renames for touch_controller_plus, note that we would have already renamed it to
		// the old name.
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger/proximity",
			"/user/hand/left/input/trigger/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_meta/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_meta/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_curl/value",
			"/user/hand/left/input/trigger/curl_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_curl/value",
			"/user/hand/right/input/trigger/curl_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_slide/value",
			"/user/hand/left/input/trigger/slide_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_slide/value",
			"/user/hand/right/input/trigger/slide_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
	};
	constexpr size_t length = sizeof(renames) / sizeof(renames[0]);

	for (size_t i = 0; i < length; i++) {
		const RenameMap& rename = renames[i];

		if (rename.before_version != 0 && data->openxr_version >= rename.before_version) {
			// We're done, we are on a new version than this, no need to check further.
			return p_path;
		}

		if ((rename.profile == nullptr || p_interaction_profile_name == rename.profile) &&
			strcmp(p_path, rename.from) == 0) {
			return rename.to;
		}
	}

	return p_path;
}

int OpenXRAPI::interaction_profile_add_binding(
	RID p_interaction_profile, RID p_action, const String& p_path)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, -1);

	if (!interaction_profile_supports_io_path(ip->name, p_path)) {
		return -1;
	}

	XrActionSuggestedBinding binding;

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_COND_V(action == nullptr || action->handle == XR_NULL_HANDLE, -1);

	binding.action = action->handle;

	XrResult result = xrStringToPath(data->instance,
		check_profile_path(ip->internal_name, p_path.utf8().get_data()), &binding.binding);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to get path for ", p_path, "! [", get_error_string(result),
		// "]");
		return -1;
	}

	ip->bindings.push_back(binding);

	return ip->bindings.size() - 1;
}

bool OpenXRAPI::interaction_profile_add_modifier(
	RID p_interaction_profile, const PackedByteArray& p_modifier)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, false);

	if (!p_modifier.is_empty()) {
		// Add it to our stack.
		ip->modifiers.push_back(p_modifier);
	}

	return true;
}

bool OpenXRAPI::interaction_profile_suggest_bindings(RID p_interaction_profile)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, false);

	void* next = nullptr;

	// Note, extensions should only add binding modifiers if they are supported, else this may fail.
	XrBindingModificationsKHR binding_modifiers;
	Vector<const XrBindingModificationBaseHeaderKHR*> modifiers;
	if (!ip->modifiers.is_empty()) {
		for (const PackedByteArray& modifier : ip->modifiers) {
			const XrBindingModificationBaseHeaderKHR* ptr =
				(const XrBindingModificationBaseHeaderKHR*)modifier.ptr();
			modifiers.push_back(ptr);
		}

		binding_modifiers.type = XR_TYPE_BINDING_MODIFICATIONS_KHR;
		binding_modifiers.next = next;
		binding_modifiers.bindingModificationCount = modifiers.size();
		binding_modifiers.bindingModifications = modifiers.ptr();
		next = &binding_modifiers;
	}

	const XrInteractionProfileSuggestedBinding suggested_bindings = {
		XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, // type
		next,										   // next
		ip->path,									   // interactionProfile
		uint32_t(ip->bindings.size()),				   // countSuggestedBindings
		ip->bindings.ptr()							   // suggestedBindings
	};

	XrResult result = xrSuggestInteractionProfileBindings(data->instance, &suggested_bindings);
	if (result == XR_ERROR_PATH_UNSUPPORTED) {
		// this is fine, not all runtimes support all devices.
		print_verbose(
			"OpenXR Interaction profile " + ip->name + " is not supported on this runtime");
	}
	else if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to suggest bindings for ", ip->name, "! [",
		// get_error_string(result), "]");
		// reporting is enough...
	}

	/* For debugging:
	print_verbose("Suggested bindings for " + ip->name);
	for (int i = 0; i < ip->bindings.size(); i++) {
		uint32_t strlen;
		char path[XR_MAX_PATH_LENGTH];

		String action_name = action_get_name(get_action_rid(ip->bindings[i].action));

		XrResult result = xrPathToString(instance, ip->bindings[i].binding, XR_MAX_PATH_LENGTH,
	&strlen, path); if (XR_FAILED(result)) { print_line("OpenXR: failed to retrieve bindings for ",
	action_name, "! [", get_error_string(result), "]");
		}
		print_verbose(" - " + action_name + " => " + String(path));
	}
	*/

	return true;
}

void OpenXRAPI::interaction_profile_free(RID p_interaction_profile)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
	ip->modifiers.clear();

	data->interaction_profile_owner.free(p_interaction_profile);
}

bool OpenXRAPI::get_action_bool(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!data->running) {
		return false;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_BOOLEAN_INPUT, false);

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateBoolean result_state;
	result_state.type = XR_TYPE_ACTION_STATE_BOOLEAN, result_state.next = nullptr;
	XrResult result = xrGetActionStateBoolean(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action boolean! [", get_error_string(result), "]");
		return false;
	}

	return result_state.isActive && result_state.currentState;
}

float OpenXRAPI::get_action_float(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, 0.0);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, 0.0);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, 0.0);

	if (!data->running) {

		return 0.0;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_FLOAT_INPUT, 0.0);

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateFloat result_state;
	result_state.type = XR_TYPE_ACTION_STATE_FLOAT, result_state.next = nullptr;
	XrResult result = xrGetActionStateFloat(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action float! [", get_error_string(result), "]");
		return 0.0;
	}

	return result_state.isActive ? result_state.currentState : 0.0;
}

Vector2 OpenXRAPI::get_action_vector2(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, Vector2());
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, Vector2());
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, Vector2());

	if (!data->running) {
		return Vector2();
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_VECTOR2F_INPUT, Vector2());

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateVector2f result_state;
	result_state.type = XR_TYPE_ACTION_STATE_VECTOR2F, result_state.next = nullptr;
	XrResult result = xrGetActionStateVector2f(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action vector2! [", get_error_string(result), "]");
		return Vector2();
	}

	return result_state.isActive ? Vector2(result_state.currentState.x, result_state.currentState.y)
								 : Vector2();
}

XRPose::TrackingConfidence OpenXRAPI::get_action_pose(RID p_action, RID p_tracker,
	Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, XRPose::XR_TRACKING_CONFIDENCE_NONE);

	if (!data->running) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	ERR_FAIL_COND_V(
		action->action_type != XR_ACTION_TYPE_POSE_INPUT, XRPose::XR_TRACKING_CONFIDENCE_NONE);

	// print_verbose("Checking " + action->name + " => " + tracker->name + " (" +
	// itos(tracker->toplevel_path) + ")");

	uint64_t index = 0xFFFFFFFF;
	uint64_t size = uint64_t(action->trackers.size());
	for (uint64_t i = 0; i < size && index == 0xFFFFFFFF; i++) {
		if (action->trackers[i].tracker_rid == p_tracker) {
			index = i;
		}
	}

	if (index == 0xFFFFFFFF) {
		// couldn't find it?
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XrTime display_time = get_predicted_display_time();
	if (display_time == 0) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	if (action->trackers[index].space == XR_NULL_HANDLE) {
		// if this is a pose we need to define spaces

		XrActionSpaceCreateInfo action_space_info = {
			XR_TYPE_ACTION_SPACE_CREATE_INFO, // type
			nullptr,						  // next
			action->handle,					  // action
			tracker->toplevel_path,			  // subactionPath
			{
				{0.0, 0.0, 0.0, 1.0}, // orientation
				{0.0, 0.0, 0.0}		  // position
			}						  // poseInActionSpace
		};

		XrSpace space;
		XrResult result = xrCreateActionSpace(data->session, &action_space_info, &space);
		if (XR_FAILED(result)) {
			// print_line("OpenXR: couldn't create action space! [", get_error_string(result), "]");
			return XRPose::XR_TRACKING_CONFIDENCE_NONE;
		}

		action->trackers.ptrw()[index].space = space;
	}

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr,				// next
		0,						// velocityFlags
		{0.0, 0.0, 0.0},		// linearVelocity
		{0.0, 0.0, 0.0}			// angularVelocity
	};

	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity,				// next
		0,						// locationFlags
		{
			{0.0, 0.0, 0.0, 0.0}, // orientation
			{0.0, 0.0, 0.0}		  // position
		}						  // pose
	};

	XrResult result =
		xrLocateSpace(action->trackers[index].space, data->play_space, display_time, &location);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to locate space! [", get_error_string(result), "]");
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XRPose::TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	return confidence;
}

bool OpenXRAPI::trigger_haptic_pulse(
	RID p_action, RID p_tracker, float p_frequency, float p_amplitude, XrDuration p_duration_ns)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!data->running) {
		return false;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_VIBRATION_OUTPUT, false);

	XrHapticActionInfo action_info = {
		XR_TYPE_HAPTIC_ACTION_INFO, // type
		nullptr,					// next
		action->handle,				// action
		tracker->toplevel_path		// subactionPath
	};

	XrHapticVibration vibration = {
		XR_TYPE_HAPTIC_VIBRATION, // type
		nullptr,				  // next
		p_duration_ns,			  // duration
		p_frequency,			  // frequency
		p_amplitude,			  // amplitude
	};

	XrResult result =
		xrApplyHapticFeedback(data->session, &action_info, (const XrHapticBaseHeader*)&vibration);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to apply haptic feedback! [", get_error_string(result), "]");
		return false;
	}

	return true;
}

const Vector<XrEnvironmentBlendMode> OpenXRAPI::get_supported_environment_blend_modes()
{
	return data->supported_environment_blend_modes;
}

bool OpenXRAPI::is_environment_blend_mode_supported(XrEnvironmentBlendMode p_blend_mode)
{
	return data->supported_environment_blend_modes.has(p_blend_mode);
}

bool OpenXRAPI::set_environment_blend_mode(XrEnvironmentBlendMode p_blend_mode)
{
	if (data->emulate_environment_blend_mode_alpha_blend &&
		p_blend_mode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) {
		data->requested_environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
		data->environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		set_render_environment_blend_mode(data->environment_blend_mode);
		return true;
	}
	// We allow setting this when not initialized and will check if it is supported when
	// initializing. After OpenXR is initialized we verify we're setting a supported blend mode.
	else if (!is_initialized() || is_environment_blend_mode_supported(p_blend_mode)) {
		data->requested_environment_blend_mode = p_blend_mode;
		data->environment_blend_mode = p_blend_mode;
		set_render_environment_blend_mode(data->environment_blend_mode);
		return true;
	}
	return false;
}

void OpenXRAPI::set_emulate_environment_blend_mode_alpha_blend(bool p_enabled)
{
	data->emulate_environment_blend_mode_alpha_blend = p_enabled;
}

OpenXRAPI::OpenXRAlphaBlendModeSupport OpenXRAPI::is_environment_blend_mode_alpha_blend_supported()
{
	if (data->emulate_environment_blend_mode_alpha_blend) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING;
	}
	else if (is_environment_blend_mode_supported(XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND)) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL;
	}
	return OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE;
}

RID OpenXRAPI::OpenXRSwapChainInfo::get_image() { return RID(); }

bool OpenXRAPI::OpenXRSwapChainInfo::acquire(bool& p_should_render) { return true; }

void OpenXRAPI::set_render_environment_blend_mode(XrEnvironmentBlendMode p_mode) {}

void OpenXRAPI::destroy_session() {}

void OpenXRAPI::update_main_swapchain_size() {}

bool OpenXRAPI::setup_view_space() { return true; }

void OpenXRAPI::set_render_state_render_region(const Rect2i& p_render_region) {}

void OpenXRAPI::set_render_state_multiplier(double p_render_target_size_multiplier) {}

void OpenXRAPI::pre_render() {}

bool OpenXRAPI::pre_draw_viewport(RID p_render_target) { return true; }

void OpenXRAPI::post_draw_viewport(RID p_render_target) {}

RID OpenXRAPI::OpenXRSwapChainInfo::get_density_map() { return RID(); }

void OpenXRAPI::setup()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRAPI::finalize()
{
	if (!data) {
		return;
	}
	finish();
	memdelete(data);
	data = nullptr;
}

bool OpenXRAPI::obtain_swapchain_formats() { return true; }

bool OpenXRAPI::load_supported_swapchain_formats() { return true; }

bool OpenXRAPI::load_supported_reference_spaces() { return true; }

bool OpenXRAPI::get_view_projection(
	uint32_t p_view, double p_z_near, double p_z_far, Projection& p_camera_matrix)
{
	return true;
}

Size2 OpenXRAPI::get_play_space_bounds() { return Size2(); }

XrHandTrackerEXT OpenXRAPI::get_hand_tracker(int p_hand_index) { return XrHandTrackerEXT(); }

Vector2 OpenXRAPI::get_eye_focus(uint32_t p_view, float p_aspect) { return Vector2(); }

PackedStringArray OpenXRAPI::get_all_requested_extensions(XrVersion p_xr_version)
{
	return PackedStringArray();
}

void OpenXRAPI::end_frame() {}

void OpenXRAPI::destroy_instance() {}

bool OpenXRAPI::create_session() { return true; }

void OpenXRAPI::cleanup_extension_wrappers() {}

void OpenXRAPI::allocate_view_buffers(uint32_t p_view_count, bool p_submit_depth_buffer) {}

String OpenXRAPI::get_swapchain_format_name(long p_swapchain_format) { return String(); }



////////////////////////////////////////////////////////////////////////////
// OpenXRUtil

String OpenXRUtil::get_result_string(XrResult p_result)
{
	XR_ENUM_SWITCH(XrResult, p_result)
}

String OpenXRUtil::get_view_configuration_name(XrViewConfigurationType p_view_configuration)
{
	XR_ENUM_SWITCH(XrViewConfigurationType, p_view_configuration)
}

String OpenXRUtil::get_reference_space_name(XrReferenceSpaceType p_reference_space)
{
	XR_ENUM_SWITCH(XrReferenceSpaceType, p_reference_space)
}

String OpenXRUtil::get_structure_type_name(XrStructureType p_structure_type)
{
	XR_ENUM_SWITCH(XrStructureType, p_structure_type)
}

String OpenXRUtil::get_session_state_name(XrSessionState p_session_state)
{
	XR_ENUM_SWITCH(XrSessionState, p_session_state)
}

String OpenXRUtil::get_action_type_name(XrActionType p_action_type)
{
	XR_ENUM_SWITCH(XrActionType, p_action_type)
}

String OpenXRUtil::get_environment_blend_mode_name(XrEnvironmentBlendMode p_blend_mode)
{
	XR_ENUM_SWITCH(XrEnvironmentBlendMode, p_blend_mode);
}

String OpenXRUtil::make_xr_version_string(XrVersion p_version)
{
	String version;

	version += String::num_int64(XR_VERSION_MAJOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_MINOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_PATCH(p_version));

	return version;
}

String OpenXRUtil::get_handle_as_hex_string(void* p_handle)
{
	String hex;

	if (p_handle == XR_NULL_HANDLE) {
		return "null";
	}

	uint64_t handle = (uint64_t)p_handle;

	while (handle != 0) {
		uint8_t a = handle & 0x0F;
		uint8_t b = (handle & 0xF0) >> 4;
		handle = handle >> 8;

		if (a < 10) {
			hex = (a + '0') + hex;
		}
		else {
			hex = (a + 'a' - 10) + hex;
		}

		if (b < 10) {
			hex = (b + '0') + hex;
		}
		else {
			hex = (b + 'a' - 10) + hex;
		}
	}

	return "0x" + hex;
}

String OpenXRUtil::string_from_xruuid(const XrUuid& xr_uuid)
{
	String ret;
	bool non_zero = false;

	for (int i = 0; i < XR_UUID_SIZE; i++) {
		non_zero |= xr_uuid.data[i] != 0;

		char a = xr_uuid.data[i] & 0xF0 >> 4;
		char b = xr_uuid.data[i] & 0x0F;

		if (a < 10) {
			ret += '0' + a;
		}
		else {
			ret += 'a' + a - 10;
		}

		if (b < 10) {
			ret += '0' + b;
		}
		else {
			ret += 'a' + b - 10;
		}
	}

	if (non_zero) {
		return ret;
	}
	else {
		return "";
	}
}

XrUuid OpenXRUtil::xruuid_from_string(const String& p_uuid)
{
	XrUuid new_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	int len = p_uuid.length();
	if (len == 0) {
		return new_uuid;
	}
	else if (len != (2 * XR_UUID_SIZE)) {
		WARN_PRINT("OpenXR: Unexpected UUID length: " + String::num_int64(len) +
				   " != " + String::num_int64(2 * XR_UUID_SIZE));
	}

	int j = 0;
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		uint8_t val = 0;

		// 2 chars per byte.
		for (int k = 0; k < 2; k++) {
			if (j < len) {
				val <<= 4;

				char32_t c = p_uuid[j++];
				if (c >= '0' && c <= '9') {
					val += uint8_t(c - '0');
				}
				else if (c >= 'a' && c <= 'f') {
					val += uint8_t(10 + c - 'a');
				}
				else if (c >= 'A' && c <= 'F') {
					val += uint8_t(10 + c - 'A');
				}
				else {
					WARN_PRINT("OpenXR: Unexpected character in UUID: " + String::num_int64(c));
				}
			}
		}

		new_uuid.data[i] = val;
	}

	return new_uuid;
}

void OpenXRUtil::XrMatrix4x4f_CreateProjection(XrMatrix4x4f* result, const float tanAngleLeft,
	const float tanAngleRight, const float tanAngleUp, const float tanAngleDown,
	const float nearZ, const float farZ)
{
	const float tanAngleWidth = tanAngleRight - tanAngleLeft;

	// Set to tanAngleUp - tanAngleDown for a clip space with positive Y up.
	const float tanAngleHeight = (tanAngleUp - tanAngleDown);

	// Set to nearZ for a [-1,1] Z clip space.
	const float offsetZ = nearZ;

	if (farZ <= nearZ) {
		// place the far plane at infinity
		result->m[0] = 2.0f / tanAngleWidth;
		result->m[4] = 0.0f;
		result->m[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
		result->m[12] = 0.0f;

		result->m[1] = 0.0f;
		result->m[5] = 2.0f / tanAngleHeight;
		result->m[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
		result->m[13] = 0.0f;

		result->m[2] = 0.0f;
		result->m[6] = 0.0f;
		result->m[10] = -1.0f;
		result->m[14] = -(nearZ + offsetZ);

		result->m[3] = 0.0f;
		result->m[7] = 0.0f;
		result->m[11] = -1.0f;
		result->m[15] = 0.0f;
	}
	else {
		// normal projection
		result->m[0] = 2.0f / tanAngleWidth;
		result->m[4] = 0.0f;
		result->m[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
		result->m[12] = 0.0f;

		result->m[1] = 0.0f;
		result->m[5] = 2.0f / tanAngleHeight;
		result->m[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
		result->m[13] = 0.0f;

		result->m[2] = 0.0f;
		result->m[6] = 0.0f;
		result->m[10] = -(farZ + offsetZ) / (farZ - nearZ);
		result->m[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

		result->m[3] = 0.0f;
		result->m[7] = 0.0f;
		result->m[11] = -1.0f;
		result->m[15] = 0.0f;
	}
}

void OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(
	XrMatrix4x4f* result, const XrFovf fov, const float nearZ, const float farZ)
{
	const float tanLeft = Math::tan(fov.angleLeft);
	const float tanRight = Math::tan(fov.angleRight);

	const float tanDown = Math::tan(fov.angleDown);
	const float tanUp = Math::tan(fov.angleUp);

	XrMatrix4x4f_CreateProjection(result, tanLeft, tanRight, tanUp, tanDown, nearZ, farZ);
}

////////////////////////////////////////////////////////////////////////////
// OpenXRStructureBase

void OpenXRStructureBase::set_next(const Ref<OpenXRStructureBase> p_next) { next = p_next; }

Ref<OpenXRStructureBase> OpenXRStructureBase::get_next() const { return next; }

bool OpenXRStructureBase::has_valid_configuration() const
{
	if (persistence_contexts.is_empty()) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);

	if (!OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
		return false;
	}

	return true;
}

void* OpenXRStructureBase::get_header(void* p_next)
{
	void* n = p_next;
	if (get_next().is_valid()) {
		n = get_next()->get_header(n);
	}

	if (has_valid_configuration()) {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);

		context_handles.resize(persistence_contexts.size());

		XrSpatialPersistenceContextEXT* ptr = context_handles.ptrw();
		int i = 0;
		for (const RID& rid : persistence_contexts) {
			ptr[i++] = OpenXRSpatialAnchorCapability::get_persistence_context_handle(rid);
		}

		persistence_config.next = n;
		persistence_config.persistenceContextCount = (uint32_t)context_handles.size();
		persistence_config.persistenceContexts = context_handles.ptr();

		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&persistence_config;
	}

	return n;
}

XrStructureType OpenXRStructureBase::get_structure_type()
{
	if (!persistence_contexts.is_empty()) {
		return XR_TYPE_SPATIAL_CONTEXT_PERSISTENCE_CONFIG_EXT;
	}

	XrStructureType* header = (XrStructureType*)get_header();
	if (header == nullptr) {
		return XR_TYPE_UNKNOWN;
	}
	return *header;
}

void OpenXRStructureBase::add_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(persistence_contexts.has(p_persistence_context));

	persistence_contexts.push_back(p_persistence_context);
}

void OpenXRStructureBase::remove_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(!persistence_contexts.has(p_persistence_context));

	persistence_contexts.erase(p_persistence_context);
}

uint64_t OpenXRStructureBase::_get_structure_type() { return (uint64_t)get_structure_type(); }

////////////////////////////////////////////////////////////////////////////
// OpenXRAPIExtension

uint64_t OpenXRAPIExtension::get_openxr_version()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_openxr_version();
}

uint64_t OpenXRAPIExtension::get_instance()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_instance();
}

uint64_t OpenXRAPIExtension::get_system_id()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_system_id();
}

uint64_t OpenXRAPIExtension::get_session()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_session();
}

bool OpenXRAPIExtension::openxr_is_enabled(bool p_check_run_in_editor)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::openxr_is_enabled(p_check_run_in_editor);
}

uint64_t OpenXRAPIExtension::get_instance_proc_addr(const String& p_name)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	CharString str = p_name.utf8();
	PFN_xrVoidFunction addr = nullptr;
	XrResult result = OpenXRAPI::get_instance_proc_addr(str.get_data(), &addr);
	if (result != XR_SUCCESS) {
		return 0;
	}
	return reinterpret_cast<uint64_t>(addr);
}

String OpenXRAPIExtension::get_error_string(uint64_t result)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), String());
	return OpenXRAPI::get_error_string((XrResult)result);
}

String OpenXRAPIExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), String());
	return OpenXRAPI::get_swapchain_format_name(p_swapchain_format);
}

void OpenXRAPIExtension::set_object_name(
	int64_t p_object_type, uint64_t p_object_handle, const String& p_object_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::set_object_name(XrObjectType(p_object_type), p_object_handle, p_object_name);
}

void OpenXRAPIExtension::begin_debug_label_region(const String& p_label_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::begin_debug_label_region(p_label_name);
}

void OpenXRAPIExtension::end_debug_label_region()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::end_debug_label_region();
}

void OpenXRAPIExtension::insert_debug_label(const String& p_label_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::insert_debug_label(p_label_name);
}

uint32_t OpenXRAPIExtension::get_view_count() const
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint32_t)OpenXRAPI::get_view_count();
}

uint64_t OpenXRAPIExtension::get_view_configuration() const
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_view_configuration();
}

bool OpenXRAPIExtension::is_initialized()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::is_initialized();
}

bool OpenXRAPIExtension::is_running()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::is_running();
}

uint64_t OpenXRAPIExtension::get_play_space()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_play_space();
}

int64_t OpenXRAPIExtension::get_predicted_display_time()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (XrTime)OpenXRAPI::get_predicted_display_time();
}

int64_t OpenXRAPIExtension::get_next_frame_time()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	WARN_PRINT_ONCE("OpenXR: Next frame timing called, verify this is intended.");

	return (XrTime)OpenXRAPI::get_next_frame_time();
}

bool OpenXRAPIExtension::can_render()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::can_render();
}

RID OpenXRAPIExtension::find_action(const String& p_name, const RID& p_action_set)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());
	return OpenXRAPI::find_action(p_name, p_action_set);
}

uint64_t OpenXRAPIExtension::action_get_handle(RID p_action)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	XrAction action_handle = OpenXRAPI::action_get_handle(p_action);
	return (uint64_t)action_handle;
}

uint64_t OpenXRAPIExtension::get_hand_tracker(int p_hand_index)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_hand_tracker(p_hand_index);
}

double OpenXRAPIExtension::get_render_state_z_near()
{
	ERR_NOT_ON_RENDER_THREAD_V(0.0);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0.0);
	return OpenXRAPI::get_render_state_z_near();
}

double OpenXRAPIExtension::get_render_state_z_far()
{
	ERR_NOT_ON_RENDER_THREAD_V(0.0);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0.0);
	return OpenXRAPI::get_render_state_z_far();
}

void OpenXRAPIExtension::set_velocity_texture(RID p_render_target)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_texture(p_render_target);
}

void OpenXRAPIExtension::set_velocity_depth_texture(RID p_render_target)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_depth_texture(p_render_target);
}

void OpenXRAPIExtension::set_velocity_target_size(const Size2i& p_target_size)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_target_size(p_target_size);
}

PackedInt64Array OpenXRAPIExtension::get_supported_swapchain_formats()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedInt64Array());
	return OpenXRAPI::get_supported_swapchain_formats();
}

uint64_t OpenXRAPIExtension::openxr_swapchain_create(XrSwapchainCreateFlags p_create_flags,
	XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	OpenXRAPI::OpenXRSwapChainInfo* new_swapchain_info = memnew(OpenXRAPI::OpenXRSwapChainInfo);
	new_swapchain_info->create(p_create_flags, p_usage_flags, p_swapchain_format, p_width, p_height,
		p_sample_count, p_array_size);
	return reinterpret_cast<uint64_t>(new_swapchain_info);
}

void OpenXRAPIExtension::openxr_swapchain_free(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	swapchain_info->free();
	memfree(swapchain_info);
}

uint64_t OpenXRAPIExtension::openxr_swapchain_get_swapchain(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	XrSwapchain swapchain = swapchain_info->get_swapchain();
	return (uint64_t)swapchain;
}

void OpenXRAPIExtension::openxr_swapchain_acquire(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	bool should_render = true;
	swapchain_info->acquire(should_render);
}

RID OpenXRAPIExtension::openxr_swapchain_get_image(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	return swapchain_info->get_image();
}

void OpenXRAPIExtension::openxr_swapchain_release(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	swapchain_info->release();
}

uint64_t OpenXRAPIExtension::get_projection_layer()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_projection_layer();
}

void OpenXRAPIExtension::set_render_region(const Rect2i& p_render_region)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_render_region(p_render_region);
}

void OpenXRAPIExtension::update_main_swapchain_size()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::update_main_swapchain_size();
}

void OpenXRAPIExtension::set_emulate_environment_blend_mode_alpha_blend(bool p_enabled)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_emulate_environment_blend_mode_alpha_blend(p_enabled);
}

OpenXRAPIExtension::OpenXRAlphaBlendModeSupport
OpenXRAPIExtension::is_environment_blend_mode_alpha_blend_supported()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE);
	return (OpenXRAPIExtension::OpenXRAlphaBlendModeSupport)
		OpenXRAPI::is_environment_blend_mode_alpha_blend_supported();
}

OpenXRAPIExtension::OpenXRAPIExtension() {}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from modules/openxr/register_types.cpp
// ============================================================================

#include "action_map/openxr_action_map.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "extensions/openxr_extensions.h"
#include "modules/openxr/extensions/openxr_extensions.h"
#include "modules/modules_enabled.gen.h"
#include "openxr_api.h"
#include "openxr_interface.h"
#include "register_types.h"
#include "modules/openxr/openxr_interface.h"

#ifdef ANDROID_ENABLED
#include "extensions/platform/openxr_android_extension.h"
#endif

static Ref<OpenXRInterface> openxr_interface;

void uninitialize_openxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (openxr_interface.is_valid()) {
		// uninitialize just in case
		if (openxr_interface->is_initialized()) {
			openxr_interface->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			if (XRServer::get_primary_interface() == openxr_interface) {
				XRServer::set_primary_interface(Ref<XRInterface>());
			}
			XRServer::remove_interface(openxr_interface);
		}

		// and release
		openxr_interface.unref();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::finalize();
	}

	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		OpenXRInteractionProfileMetadata::finalize();
	}

	if (OpenXRInteractionProfileExtension::is_initialized()) {
		OpenXRInteractionProfileExtension::finalize();
	}

	// cleanup our extension wrappers
	OpenXRAPI::cleanup_extension_wrappers();
}

void initialize_openxr_module(ModuleInitializationLevel p_level) {}

// ============================================================================
// Merged from modules/openxr/openxr_interface.cpp
// ============================================================================

#include <openxr/openxr.h>
#include "action_map/openxr_action_map.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "extensions/openxr_extensions.h"
#include "modules/openxr/extensions/openxr_extensions.h"
#include "openxr_interface.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server_types.h"

String OpenXRInterface::get_name() const { return String("OpenXR"); }

uint32_t OpenXRInterface::get_capabilities() const
{
	return XRInterface::XR_VR + XRInterface::XR_STEREO;
}

XRInterface::TrackingStatus OpenXRInterface::get_tracking_status() const { return tracking_state; }

OpenXRInterface::ActionSet* OpenXRInterface::create_action_set(
	const String& p_action_set_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	// find if it already exists
	for (int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i]->action_set_name == p_action_set_name) {
			// already exists in this set
			return nullptr;
		}
	}

	ActionSet* action_set = memnew(ActionSet);
	action_set->action_set_name = p_action_set_name;
	action_set->is_active = true;
	action_set->action_set_rid =
		OpenXRAPI::action_set_create(p_action_set_name, p_localized_name, p_priority);
	action_sets.push_back(action_set);

	return action_set;
}

void OpenXRInterface::free_action_sets()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < action_sets.size(); i++) {
		ActionSet* action_set = action_sets[i];

		free_actions(action_set);

		OpenXRAPI::action_set_free(action_set->action_set_rid);

		memfree(action_set);
	}
	action_sets.clear();
}

OpenXRInterface::Action* OpenXRInterface::create_action(ActionSet* p_action_set,
	const String& p_action_name, const String& p_localized_name,
	OpenXRAction::ActionType p_action_type, const Vector<Tracker*> p_trackers)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		if (p_action_set->actions[i]->action_name == p_action_name) {
			// already exists in this set
			return nullptr;
		}
	}

	Vector<RID> tracker_rids;
	for (int i = 0; i < p_trackers.size(); i++) {
		tracker_rids.push_back(p_trackers[i]->tracker_rid);
	}

	Action* action = memnew(Action);
	if (p_action_type == OpenXRAction::OPENXR_ACTION_POSE) {
		// We can't have dual action names in OpenXR hence we added _pose,
		// but default, aim and grip and default pose action names in Godot so rename them on the
		// tracker. NOTE need to decide on whether we should keep the naming convention or rename it
		// on Godots side
		if (p_action_name == "default_pose") {
			action->action_name = "default";
		}
		else if (p_action_name == "aim_pose") {
			action->action_name = "aim";
		}
		else if (p_action_name == "grip_pose") {
			action->action_name = "grip";
		}
		else {
			action->action_name = p_action_name;
		}
	}
	else {
		action->action_name = p_action_name;
	}

	action->action_type = p_action_type;
	action->action_rid = OpenXRAPI::action_create(
		p_action_set->action_set_rid, p_action_name, p_localized_name, p_action_type, tracker_rids);
	p_action_set->actions.push_back(action);

	// we link our actions back to our trackers so we know which actions to check when we're
	// processing our trackers
	for (int i = 0; i < p_trackers.size(); i++) {
		if (!p_trackers[i]->actions.has(action)) {
			p_trackers[i]->actions.push_back(action);
		}
	}

	return action;
}

OpenXRInterface::Action* OpenXRInterface::find_action(const String& p_action_name)
{
	// We just find the first action by this name

	for (int i = 0; i < action_sets.size(); i++) {
		for (int j = 0; j < action_sets[i]->actions.size(); j++) {
			if (action_sets[i]->actions[j]->action_name == p_action_name) {
				return action_sets[i]->actions[j];
			}
		}
	}

	// not found
	return nullptr;
}

void OpenXRInterface::free_actions(ActionSet* p_action_set)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		Action* action = p_action_set->actions[i];

		OpenXRAPI::action_free(action->action_rid);

		memdelete(action);
	}
	p_action_set->actions.clear();
}

OpenXRInterface::Tracker* OpenXRInterface::find_tracker(const String& p_tracker_name, bool p_create)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), nullptr);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size(); i++) {
		tracker = trackers[i];
		if (tracker->tracker_name == p_tracker_name) {
			return tracker;
		}
	}

	if (!p_create) {
		return nullptr;
	}

	ERR_FAIL_COND_V(!OpenXRAPI::is_top_level_path_supported(p_tracker_name), nullptr);

	// Create our RID
	RID tracker_rid = OpenXRAPI::tracker_create(p_tracker_name);
	ERR_FAIL_COND_V(tracker_rid.is_null(), nullptr);

	// Create our controller tracker.
	Ref<XRTracker> controller_tracker;
	controller_tracker.instantiate();
	controller_tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);

	// We have standardized some names to make things nicer to the user so lets recognize the
	// toplevel paths related to these.
	if (p_tracker_name == "/user/hand/left") {
		controller_tracker->set_tracker_name("left_hand");
		controller_tracker->set_tracker_desc("Left hand controller");
		controller_tracker->set_tracker_hand(XRTracker::TRACKER_HAND_LEFT);
	}
	else if (p_tracker_name == "/user/hand/right") {
		controller_tracker->set_tracker_name("right_hand");
		controller_tracker->set_tracker_desc("Right hand controller");
		controller_tracker->set_tracker_hand(XRTracker::TRACKER_HAND_RIGHT);
	}
	else {
		controller_tracker->set_tracker_name(p_tracker_name);
		controller_tracker->set_tracker_desc(p_tracker_name);
	}
	controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	XRServer::add_tracker(controller_tracker);

	// create a new entry
	tracker = memnew(Tracker);
	tracker->tracker_name = p_tracker_name;
	tracker->tracker_rid = tracker_rid;
	tracker->controller_tracker = controller_tracker;
	tracker->interaction_profile = RID();
	trackers.push_back(tracker);

	return tracker;
}

void OpenXRInterface::tracker_profile_changed(RID p_tracker, RID p_interaction_profile)
{
	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size() && tracker == nullptr; i++) {
		if (trackers[i]->tracker_rid == p_tracker) {
			tracker = trackers[i];
		}
	}
	ERR_FAIL_NULL(tracker);

	tracker->interaction_profile = p_interaction_profile;

	if (p_interaction_profile.is_null()) {
		print_verbose("OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " +
					  INTERACTION_PROFILE_NONE);
		tracker->controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	}
	else {
		String name = OpenXRAPI::interaction_profile_get_name(p_interaction_profile);
		print_verbose(
			"OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " + name);
		tracker->controller_tracker->set_tracker_profile(name);
	}
}

void OpenXRInterface::trigger_haptic_pulse(const String& p_action_name,
	const String& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	Action* action = find_action(p_action_name);
	ERR_FAIL_NULL(action);

	// We need to map our tracker name to our OpenXR name for our inbuild names.
	String tracker_name = p_tracker_name;
	if (tracker_name == "left_hand") {
		tracker_name = "/user/hand/left";
	}
	else if (tracker_name == "right_hand") {
		tracker_name = "/user/hand/right";
	}
	Tracker* tracker = find_tracker(tracker_name);
	ERR_FAIL_NULL(tracker);

	// TODO OpenXR does not support delay, so we may need to add support for that somehow...

	XrDuration duration = XrDuration(p_duration_sec * 1000000000.0); // seconds -> nanoseconds

	OpenXRAPI::trigger_haptic_pulse(
		action->action_rid, tracker->tracker_rid, p_frequency, p_amplitude, duration);
}

void OpenXRInterface::free_trackers()
{
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < trackers.size(); i++) {
		Tracker* tracker = trackers[i];

		OpenXRAPI::tracker_free(tracker->tracker_rid);
		XRServer::remove_tracker(tracker->controller_tracker);
		tracker->controller_tracker.unref();

		memdelete(tracker);
	}
	trackers.clear();
}

void OpenXRInterface::free_interaction_profiles()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (const RID& interaction_profile : interaction_profiles) {
		OpenXRAPI::interaction_profile_free(interaction_profile);
	}
	interaction_profiles.clear();
}

bool OpenXRInterface::initialize_on_startup() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return true;
	}
}

bool OpenXRInterface::is_initialized() const { return initialized; }

bool OpenXRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	if (p_mode == XRInterface::XR_PLAY_AREA_3DOF) {
		return false;
	}
	return true;
}

XRInterface::PlayAreaMode OpenXRInterface::get_play_area_mode() const
{
	if (!OpenXRAPI::is_allocated() || !initialized) {
		return XRInterface::XR_PLAY_AREA_UNKNOWN;
	}

	XrReferenceSpaceType reference_space = OpenXRAPI::get_reference_space();

	if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL) {
		return XRInterface::XR_PLAY_AREA_SITTING;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT) {
		return XRInterface::XR_PLAY_AREA_ROOMSCALE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_STAGE) {
		return XRInterface::XR_PLAY_AREA_STAGE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_MAX_ENUM) {
		return XRInterface::XR_PLAY_AREA_CUSTOM;
	}

	return XRInterface::XR_PLAY_AREA_UNKNOWN;
}

bool OpenXRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrReferenceSpaceType reference_space;

	if (p_mode == XRInterface::XR_PLAY_AREA_SITTING) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_ROOMSCALE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_STAGE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_STAGE;
	}
	else {
		return false;
	}

	if (OpenXRAPI::set_requested_reference_space(reference_space)) {
		if (XRServer::is_initialized()) {
			XRServer::clear_reference_frame();
		}
		return true;
	}

	return false;
}

PackedVector3Array OpenXRInterface::get_play_area() const
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), PackedVector3Array());
	PackedVector3Array arr;

	Vector3 sides[4] = {
		Vector3(-0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, 0.5f),
		Vector3(-0.5f, 0.0f, 0.5f),
	};

	if (OpenXRAPI::is_allocated() && OpenXRAPI::is_initialized()) {
		Size2 extents = OpenXRAPI::get_play_space_bounds();
		if (extents.width != 0.0 && extents.height != 0.0) {
			Transform3D reference_frame = XRServer::get_reference_frame();

			for (int i = 0; i < 4; i++) {
				Vector3 coord = sides[i];

				// Scale it up.
				coord.x *= extents.width;
				coord.z *= extents.height;

				// Now apply our reference.
				Vector3 out = reference_frame.xform(coord);
				arr.push_back(out);
			}
		}
		else {
			WARN_PRINT_ONCE("OpenXR: No extents available.");
		}
	}

	return arr;
}

float OpenXRInterface::get_display_refresh_rate() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0.0;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return 0.0;
	}
	else {
		return OpenXRAPI::get_display_refresh_rate();
	}
}

void OpenXRInterface::set_display_refresh_rate(float p_refresh_rate)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return;
	}
	else {
		OpenXRAPI::set_display_refresh_rate(p_refresh_rate);
	}
}

bool OpenXRInterface::is_hand_tracking_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRHandTrackingExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRHandTrackingExtension::get_active();
		}
	}
}

bool OpenXRInterface::is_hand_interaction_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRInteractionProfileExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRInteractionProfileExtension::is_profile_available(
				OpenXRInteractionProfileExtension::PROFILE_HAND_INTERACTION);
		}
	}
}

bool OpenXRInterface::is_eye_gaze_interaction_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXREyeGazeInteractionExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction();
		}
	}
}

bool OpenXRInterface::is_action_set_active(const String& p_action_set) const
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			return action_set->is_active;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
	return false;
}

void OpenXRInterface::set_action_set_active(const String& p_action_set, bool p_active)
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			action_set->is_active = p_active;
			return;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
}

float OpenXRInterface::get_vrs_min_radius() const { return xr_vrs.get_vrs_min_radius(); }

void OpenXRInterface::set_vrs_min_radius(float p_vrs_min_radius)
{
	xr_vrs.set_vrs_min_radius(p_vrs_min_radius);
}

float OpenXRInterface::get_vrs_strength() const { return xr_vrs.get_vrs_strength(); }

void OpenXRInterface::set_vrs_strength(float p_vrs_strength)
{
	xr_vrs.set_vrs_strength(p_vrs_strength);
}

double OpenXRInterface::get_render_target_size_multiplier() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 1.0;
	}
	else {
		return OpenXRAPI::get_render_target_size_multiplier();
	}
}

void OpenXRInterface::set_render_target_size_multiplier(double multiplier)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_render_target_size_multiplier(multiplier);
	}
}

bool OpenXRInterface::is_foveation_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::is_foveation_supported();
	}
}

int OpenXRInterface::get_foveation_level() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0;
	}
	else {
		return OpenXRAPI::get_foveation_level();
	}
}

void OpenXRInterface::set_foveation_level(int p_foveation_level)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_level(p_foveation_level);
	}
}

bool OpenXRInterface::get_foveation_dynamic() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_dynamic();
	}
}

void OpenXRInterface::set_foveation_dynamic(bool p_foveation_dynamic)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_dynamic(p_foveation_dynamic);
	}
}

bool OpenXRInterface::get_foveation_with_subsampled_images() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_with_subsampled_images();
	}
}

void OpenXRInterface::set_foveation_with_subsampled_images(bool p_enabled)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_with_subsampled_images(p_enabled);
	}
}

Size2 OpenXRInterface::get_render_target_size()
{
	if (!OpenXRAPI::is_allocated()) {
		return Size2();
	}
	else {
		return OpenXRAPI::get_recommended_target_size();
	}
}

uint32_t OpenXRInterface::get_view_count()
{
	// TODO set this based on our configuration
	return 2;
}

void OpenXRInterface::_set_default_pos(
	Transform3D& r_transform, double p_world_scale, uint64_t p_eye)
{
	r_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	r_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		r_transform.origin.x = 0.03 * p_world_scale;
	}
	else if (p_eye == 2) {
		r_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D OpenXRInterface::get_camera_transform()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());

	Transform3D hmd_transform;
	double world_scale = XRServer::get_world_scale();

	// head_transform should be updated in process

	hmd_transform.basis = head_transform.basis;
	hmd_transform.origin = head_transform.origin * world_scale;

	return hmd_transform;
}

Transform3D OpenXRInterface::get_transform_for_view(
	uint32_t p_view, const Transform3D& p_cam_transform)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(
		p_view, get_view_count(), Transform3D(), "View index outside bounds.");

	Transform3D t;
	if (OpenXRAPI::is_allocated() && OpenXRAPI::get_view_transform(p_view, t)) {
		// update our cached value if we have a valid transform
		transform_for_view[p_view] = t;
	}
	else {
		// reuse cached value
		t = transform_for_view[p_view];
	}

	// Apply our world scale
	double world_scale = XRServer::get_world_scale();
	t.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * t;
}

Projection OpenXRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	Projection cm;
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(p_view, get_view_count(), cm, "View index outside bounds.");

	if (OpenXRAPI::is_allocated()) {
		if (OpenXRAPI::get_view_projection(p_view, p_z_near, p_z_far, cm)) {
			return cm;
		}
	}

	// Failed to get from our OpenXR device? Default to some sort of sensible camera matrix..
	cm.set_for_hmd(p_view + 1, 1.0, 6.0, 14.5, 4.0, 1.5, p_z_near, p_z_far);

	return cm;
}

Rect2i OpenXRInterface::get_render_region()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_render_region();
	}
	else {
		return Rect2i();
	}
}

RID OpenXRInterface::get_color_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_color_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_depth_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_depth_texture();
	}
	else {
		return RID();
	}
}

Size2i OpenXRInterface::get_velocity_target_size()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_target_size();
	}
	else {
		return Size2i();
	}
}

void OpenXRInterface::pre_render()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::pre_render();
	}
}

bool OpenXRInterface::pre_draw_viewport(RID p_render_target)
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::pre_draw_viewport(p_render_target);
	}
	else {
		// don't render
		return false;
	}
}

Vector<RenderingServerTypes::BlitToScreen> OpenXRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

#ifndef ANDROID_ENABLED
	// If separate HMD we should output one eye to screen
	if (p_screen_rect != Rect2()) {
		RenderingServerTypes::BlitToScreen blit;

		blit.render_target = p_render_target;
		blit.multi_view.use_layer = true;
		blit.multi_view.layer = 0;
		blit.lens_distortion.apply = false;

		Size2 render_size = get_render_target_size();
		Rect2 dst_rect = p_screen_rect;
		float new_height = dst_rect.size.x * (render_size.y / render_size.x);
		if (new_height > dst_rect.size.y) {
			dst_rect.position.y = (0.5 * dst_rect.size.y) - (0.5 * new_height);
			dst_rect.size.y = new_height;
		}
		else {
			float new_width = dst_rect.size.y * (render_size.x / render_size.y);

			dst_rect.position.x = (0.5 * dst_rect.size.x) - (0.5 * new_width);
			dst_rect.size.x = new_width;
		}

		blit.dst_rect = dst_rect;
		blit_to_screen.push_back(blit);
	}
#endif

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::post_draw_viewport(p_render_target);
	}

	return blit_to_screen;
}

void OpenXRInterface::end_frame()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::end_frame();
	}
}

bool OpenXRInterface::is_passthrough_enabled()
{
	return get_environment_blend_mode() == XR_ENV_BLEND_MODE_ALPHA_BLEND;
}

bool OpenXRInterface::start_passthrough()
{
	return set_environment_blend_mode(XR_ENV_BLEND_MODE_ALPHA_BLEND);
}

void OpenXRInterface::stop_passthrough() { set_environment_blend_mode(XR_ENV_BLEND_MODE_OPAQUE); }

XRInterface::EnvironmentBlendMode OpenXRInterface::get_environment_blend_mode() const
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode = OpenXRAPI::get_environment_blend_mode();
		switch (oxr_blend_mode) {
		case XR_ENVIRONMENT_BLEND_MODE_OPAQUE: {
			return XR_ENV_BLEND_MODE_OPAQUE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE: {
			return XR_ENV_BLEND_MODE_ADDITIVE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND: {
			return XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} break;
		default:
			break;
		}
	}

	return XR_ENV_BLEND_MODE_OPAQUE;
}

bool OpenXRInterface::set_environment_blend_mode(XRInterface::EnvironmentBlendMode mode)
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode;
		switch (mode) {
		case XR_ENV_BLEND_MODE_OPAQUE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
			break;
		case XR_ENV_BLEND_MODE_ADDITIVE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
			break;
		case XR_ENV_BLEND_MODE_ALPHA_BLEND:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
			break;
		default:
			WARN_PRINT("Unknown blend mode requested: " + String::num_int64(int64_t(mode)));
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		}
		return OpenXRAPI::set_environment_blend_mode(oxr_blend_mode);
	}
	return false;
}

void OpenXRInterface::on_reference_space_change_pending(XrReferenceSpaceType p_type)
{
	reference_stage_changing = true;

	// Emit play area bounds changed signal when the reference space changes.
	PlayAreaMode mode = XR_PLAY_AREA_UNKNOWN;

	switch (p_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW:
		mode = XR_PLAY_AREA_3DOF;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL:
		mode = XR_PLAY_AREA_SITTING;
		break;
	case XR_REFERENCE_SPACE_TYPE_STAGE:
		mode = XR_PLAY_AREA_STAGE;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR:
		mode = XR_PLAY_AREA_ROOMSCALE;
		break;
	default:
		mode = XR_PLAY_AREA_UNKNOWN;
		break;
	}

	print_verbose("OpenXR Interface: Play area changed, emitting signal.");
}

OpenXRInterface::SessionState OpenXRInterface::get_session_state()
{
	if (OpenXRAPI::is_allocated()) {
		return (SessionState)OpenXRAPI::get_session_state();
	}

	return SESSION_STATE_UNKNOWN;
}

/** User Presence. */
bool OpenXRInterface::is_user_presence_supported() const
{
	if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return OpenXRUserPresenceExtension::is_initialized() && OpenXRUserPresenceExtension::is_active();
	}
}

bool OpenXRInterface::is_user_present() const
{
	// If extension is unavailable or unsupported, we default to user is present.
	if (!is_user_presence_supported()) {
		return true;
	}
	else {
		return OpenXRUserPresenceExtension::is_user_present();
	}
}

RID OpenXRInterface::get_vrs_texture()
{
	if (!OpenXRAPI::is_allocated()) {
		return RID();
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return density_map;
	}

	PackedVector2Array eye_foci;

	Size2 target_size = get_render_target_size();
	real_t aspect_ratio = target_size.x / target_size.y;
	uint32_t view_count = get_view_count();

	for (uint32_t v = 0; v < view_count; v++) {
		eye_foci.push_back(OpenXRAPI::get_eye_focus(v, aspect_ratio));
	}

	xr_vrs.set_vrs_render_region(get_render_region());

	return xr_vrs.make_vrs_texture(target_size, eye_foci);
}

XRInterface::VRSTextureFormat OpenXRInterface::get_vrs_texture_format()
{
	if (!OpenXRAPI::is_allocated()) {
		return XR_VRS_TEXTURE_FORMAT_UNIFIED;
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return XR_VRS_TEXTURE_FORMAT_FRAGMENT_DENSITY_MAP;
	}

	return XR_VRS_TEXTURE_FORMAT_UNIFIED;
}

void OpenXRInterface::set_cpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_cpu_level(p_level);
	}
}

void OpenXRInterface::set_gpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_gpu_level(p_level);
	}
}

OpenXRInterface::OpenXRInterface()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(this);
	}

	// while we don't have head tracking, don't put the headset on the floor...
	_set_default_pos(head_transform, 1.0, 0);
	_set_default_pos(transform_for_view[0], 1.0, 1);
	_set_default_pos(transform_for_view[1], 1.0, 2);
}

OpenXRInterface::~OpenXRInterface()
{
	if (is_initialized()) {
		uninitialize();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(nullptr);
	}
}

void OpenXRInterface::uninitialize() {}

void OpenXRInterface::on_refresh_rate_changes(float) {}

void OpenXRInterface::on_gpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

void OpenXRInterface::on_cpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

PackedStringArray OpenXRInterface::get_suggested_tracker_names() const
{
	return PackedStringArray();
}

bool OpenXRInterface::initialize() { return false; }

void OpenXRInterface::process() {}

bool OpenXRInterface::is_passthrough_supported() { return false; }



// ============================================================================
// Merged from modules/openxr/action_map/openxr_action_map.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"
#include "modules/openxr/action_map/openxr_action_map.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRAction

Ref<OpenXRAction> OpenXRAction::new_action(const char* p_name, const char* p_localized_name,
	const ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> action;
	action.instantiate();
	action->set_name(String(p_name));
	action->set_localized_name(String(p_localized_name));
	action->set_action_type(p_action_type);
	action->parse_toplevel_paths(String(p_toplevel_paths));

	return action;
}

String OpenXRAction::get_name_with_set() const
{
	String action_name = get_name();

	if (action_set != nullptr) {
		action_name = action_set->get_name() + "/" + action_name;
	}

	return action_name;
}

void OpenXRAction::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRAction::get_localized_name() const { return localized_name; }

void OpenXRAction::set_action_type(const OpenXRAction::ActionType p_action_type)
{
	action_type = p_action_type;
	emit_changed();
}

OpenXRAction::ActionType OpenXRAction::get_action_type() const { return action_type; }

void OpenXRAction::set_toplevel_paths(const PackedStringArray p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths;
	emit_changed();
}

PackedStringArray OpenXRAction::get_toplevel_paths() const { return toplevel_paths; }

void OpenXRAction::add_toplevel_path(const String& p_toplevel_path)
{
	if (!toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.push_back(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::rem_toplevel_path(const String& p_toplevel_path)
{
	if (toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.erase(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::parse_toplevel_paths(const String& p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths.split(",", false);
	emit_changed();
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionSet

Ref<OpenXRActionSet> OpenXRActionSet::new_action_set(
	const char* p_name, const char* p_localized_name, const int p_priority)
{
	Ref<OpenXRActionSet> action_set;
	action_set.instantiate();
	action_set->set_name(String(p_name));
	action_set->set_localized_name(p_localized_name);
	action_set->set_priority(p_priority);

	return action_set;
}

void OpenXRActionSet::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRActionSet::get_localized_name() const { return localized_name; }

void OpenXRActionSet::set_priority(const int p_priority)
{
	priority = p_priority;
	emit_changed();
}

int OpenXRActionSet::get_priority() const { return priority; }

Ref<OpenXRAction> OpenXRActionSet::add_new_action(const char* p_name, const char* p_localized_name,
	const OpenXRAction::ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> new_action =
		OpenXRAction::new_action(p_name, p_localized_name, p_action_type, p_toplevel_paths);
	add_action(new_action);
	return new_action;
}

OpenXRActionSet::~OpenXRActionSet() { clear_actions(); }

void OpenXRActionSet::clear_actions() {}

void OpenXRActionSet::add_action(Ref<OpenXRAction> const&) {}

void OpenXRActionSet::remove_action(Ref<OpenXRAction> const&) {}

Ref<OpenXRAction> OpenXRActionSet::get_action(String const&) const { return Ref<OpenXRAction>(); }

////////////////////////////////////////////////////////////////////////////
// OpenXRHapticBase

void OpenXRHapticBase::set_duration(int64_t p_duration)
{
	haptic_vibration.duration = p_duration;
	emit_changed();
}

int64_t OpenXRHapticBase::get_duration() const { return haptic_vibration.duration; }

void OpenXRHapticBase::set_frequency(float p_frequency)
{
	haptic_vibration.frequency = p_frequency;
	emit_changed();
}

float OpenXRHapticBase::get_frequency() const { return haptic_vibration.frequency; }

void OpenXRHapticBase::set_amplitude(float p_amplitude)
{
	haptic_vibration.amplitude = p_amplitude;
	emit_changed();
}

float OpenXRHapticBase::get_amplitude() const { return haptic_vibration.amplitude; }

const XrHapticBaseHeader* OpenXRHapticBase::get_xr_structure()
{
	return (XrHapticBaseHeader*)&haptic_vibration;
}

OpenXRHapticBase::OpenXRHapticBase()
{
	haptic_vibration.type = XR_TYPE_HAPTIC_VIBRATION;
	haptic_vibration.next = nullptr;
	haptic_vibration.duration = -1;
	haptic_vibration.frequency = 0.0;
	haptic_vibration.amplitude = 1.0;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRBindingModifier

PackedByteArray OpenXRBindingModifier::get_ip_modification() { return PackedByteArray(); }

String OpenXRBindingModifier::get_description() const { return ""; }

OpenXRBindingModifier::OpenXRBindingModifier() {}

OpenXRBindingModifier::~OpenXRBindingModifier() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRIPBinding & OpenXRInteractionProfile

Ref<OpenXRIPBinding> OpenXRIPBinding::new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_binding_path)
{
	Ref<OpenXRIPBinding> binding;
	binding.instantiate();
	binding->set_action(p_action);
	binding->set_binding_path(p_binding_path);

	return binding;
}

void OpenXRIPBinding::set_action(const Ref<OpenXRAction>& p_action)
{
	action = p_action;
	emit_changed();
}

Ref<OpenXRAction> OpenXRIPBinding::get_action() const { return action; }

void OpenXRIPBinding::set_binding_path(const String& p_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		binding_path = OpenXRInteractionProfileMetadata::check_path_name(p_path);
	}
	else {
		binding_path = p_path;
	}

	emit_changed();
}

String OpenXRIPBinding::get_binding_path() const { return binding_path; }

int OpenXRIPBinding::get_binding_modifier_count() const { return binding_modifiers.size(); }

Ref<OpenXRBindingModifier> OpenXRIPBinding::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRIPBinding::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->ip_binding = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRIPBinding::add_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->ip_binding && p_binding_modifier->ip_binding != this) {
			p_binding_modifier->ip_binding->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->ip_binding = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRIPBinding::remove_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->ip_binding != this,
			"Removing binding modifier that belongs to this binding but had incorrect binding "
			"pointer.");
		p_binding_modifier->ip_binding = nullptr;

		emit_changed();
	}
}

#ifndef DISABLE_DEPRECATED

void OpenXRIPBinding::set_paths(const PackedStringArray& p_paths)
{
	binding_path = "";
	for (const String& path : p_paths) {
		if (!binding_path.is_empty()) {
			binding_path += ",";
		}
		binding_path += path;
	}
}

PackedStringArray OpenXRIPBinding::get_paths() const
{
	PackedStringArray paths = binding_path.split(",", false);
	return paths;
}

int OpenXRIPBinding::get_path_count() const
{
	return binding_path.is_empty() ? 0 : 1;
}

bool OpenXRIPBinding::has_path(const String& p_path) const
{
	return binding_path == p_path;
}

void OpenXRIPBinding::add_path(const String& p_path)
{
	if (binding_path != p_path) {
		ERR_FAIL_COND_MSG(!binding_path.is_empty(),
			"Method add_path has been deprecated. A binding path was already set, create separate "
			"binding resources for each path and use set_binding_path instead.");

		binding_path = p_path;
		emit_changed();
	}
}

void OpenXRIPBinding::remove_path(const String& p_path)
{
	ERR_FAIL_COND_MSG(binding_path != p_path,
		"Method remove_path has been deprecated. Attempt at removing a different binding path, "
		"remove the correct binding record from the interaction profile instead.");

	binding_path = p_path;
	emit_changed();
}

#endif // DISABLE_DEPRECATED

OpenXRIPBinding::~OpenXRIPBinding() { action.unref(); }

Ref<OpenXRInteractionProfile> OpenXRInteractionProfile::new_profile(
	const char* p_input_profile_path)
{
	Ref<OpenXRInteractionProfile> profile;
	profile.instantiate();
	profile->set_interaction_profile_path(String(p_input_profile_path));

	return profile;
}

void OpenXRInteractionProfile::set_interaction_profile_path(const String& p_input_profile_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		interaction_profile_path =
			OpenXRInteractionProfileMetadata::check_profile_name(p_input_profile_path);
	}
	else {
		interaction_profile_path = p_input_profile_path;
	}
	emit_changed();
}

String OpenXRInteractionProfile::get_interaction_profile_path() const
{
	return interaction_profile_path;
}

void OpenXRInteractionProfile::add_new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_paths)
{
	PackedStringArray paths = p_paths.split(",", false);

	for (const String& path : paths) {
		Ref<OpenXRIPBinding> binding = OpenXRIPBinding::new_binding(p_action, path);
		add_binding(binding);
	}
}

int OpenXRInteractionProfile::get_binding_modifier_count() const
{
	return binding_modifiers.size();
}

Ref<OpenXRBindingModifier> OpenXRInteractionProfile::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRInteractionProfile::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->interaction_profile = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRInteractionProfile::add_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->interaction_profile &&
			p_binding_modifier->interaction_profile != this) {
			p_binding_modifier->interaction_profile->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->interaction_profile = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRInteractionProfile::remove_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->interaction_profile != this,
			"Removing binding modifier that belongs to this interaction profile but had incorrect "
			"interaction profile pointer.");
		p_binding_modifier->interaction_profile = nullptr;

		emit_changed();
	}
}

void OpenXRInteractionProfile::add_binding(const Ref<OpenXRIPBinding>& p_binding) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileMetadata

void OpenXRInteractionProfileMetadata::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
	_register_core_metadata();
}

void OpenXRInteractionProfileMetadata::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRInteractionProfileMetadata::register_profile_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->profile_renames.has(p_old_name));

	data->profile_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_profile_name(const String& p_name)
{
	if (data->profile_renames.has(p_name)) {
		return data->profile_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_path_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->path_renames.has(p_old_name));

	data->path_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_path_name(const String& p_name)
{
	if (data->path_renames.has(p_name)) {
		return data->path_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_top_level_path(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_top_level_path(p_openxr_path), p_openxr_path + " had already been registered");

	TopLevelPath new_toplevel_path = {p_display_name, p_openxr_path, p_openxr_extension_names};

	data->top_level_paths.push_back(new_toplevel_path);
}

void OpenXRInteractionProfileMetadata::register_interaction_profile(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_interaction_profile(p_openxr_path), p_openxr_path + " has already been registered");

	InteractionProfile new_profile;
	new_profile.display_name = p_display_name;
	new_profile.openxr_path = p_openxr_path;
	new_profile.openxr_extension_names = p_openxr_extension_names;

	data->interaction_profiles.push_back(new_profile);
}

void OpenXRInteractionProfileMetadata::register_io_path(const String& p_interaction_profile,
	const String& p_display_name, const String& p_toplevel_path, const String& p_openxr_path,
	const String& p_openxr_extension_names, OpenXRAction::ActionType p_action_type)
{
	ERR_FAIL_COND_MSG(!has_interaction_profile(p_interaction_profile),
		"Unknown interaction profile " + p_interaction_profile);
	ERR_FAIL_COND_MSG(
		!has_top_level_path(p_toplevel_path), "Unknown top level path " + p_toplevel_path);

	for (InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_interaction_profile) {
			ERR_FAIL_COND_MSG(interaction_profile.has_io_path(p_openxr_path),
				p_interaction_profile + " already has io path " + p_openxr_path + " registered!");

			IOPath new_io_path = {p_display_name, p_toplevel_path, p_openxr_path,
				p_openxr_extension_names, p_action_type};

			interaction_profile.io_paths.push_back(new_io_path);
		}
	}
}

bool OpenXRInteractionProfileMetadata::has_top_level_path(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_top_level_name(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].display_name;
		}
	}

	return String();
}

String OpenXRInteractionProfileMetadata::get_top_level_extensions(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

bool OpenXRInteractionProfileMetadata::has_interaction_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_interaction_profile_extensions(
	const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return interaction_profile.openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

const OpenXRInteractionProfileMetadata::InteractionProfile*
OpenXRInteractionProfileMetadata::get_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return &interaction_profile;
		}
	}

	return nullptr;
}

bool OpenXRInteractionProfileMetadata::InteractionProfile::has_io_path(
	const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return true;
		}
	}

	return false;
}

const OpenXRInteractionProfileMetadata::IOPath*
OpenXRInteractionProfileMetadata::InteractionProfile::get_io_path(const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return &io_paths[i];
		}
	}

	return nullptr;
}

const OpenXRInteractionProfileMetadata::IOPath* OpenXRInteractionProfileMetadata::get_io_path(
	const String& p_interaction_profile, const String& p_io_path)
{
	const OpenXRInteractionProfileMetadata::InteractionProfile* profile =
		get_profile(p_interaction_profile);
	if (profile != nullptr) {
		return profile->get_io_path(p_io_path);
	}

	return nullptr;
}

PackedStringArray OpenXRInteractionProfileMetadata::get_interaction_profile_paths()
{
	PackedStringArray arr;

	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		arr.push_back(interaction_profile.openxr_path);
	}

	return arr;
}

void OpenXRInteractionProfileMetadata::_register_core_metadata()
{
	register_path_rename(
		"/user/hand/left/input/palm_ext/pose", "/user/hand/left/input/grip_surface/pose");
	register_path_rename(
		"/user/hand/right/input/palm_ext/pose", "/user/hand/right/input/grip_surface/pose");

	// Our core toplevel paths
	register_top_level_path("Left hand controller", "/user/hand/left", "");
	register_top_level_path("Right hand controller", "/user/hand/right", "");
	register_top_level_path("Head", "/user/head", "");
	register_top_level_path("Gamepad", "/user/gamepad", "");
	register_top_level_path("Treadmill", "/user/treadmill", "");

	{ // Fallback Khronos simple controller
		const String profile_path = "/interaction_profiles/khr/simple_controller";
		register_interaction_profile("Simple controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Select click", user_path,
				user_path + "/input/select/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Original HTC Vive wands
		const String profile_path = "/interaction_profiles/htc/vive_controller";
		register_interaction_profile("HTC Vive wand", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Microsoft motion controller (original WMR controllers)
		const String profile_path = "/interaction_profiles/microsoft/motion_controller";
		register_interaction_profile("MS Motion controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Meta touch controller (original touch controllers, Quest 1 and Quest 2 controllers)
		const String profile_path = "/interaction_profiles/oculus/touch_controller";
		register_interaction_profile("Touch controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbrest touch", user_path,
				user_path + "/input/thumbrest/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}

		register_io_path(profile_path, "Menu click", "/user/hand/left",
			"/user/hand/left/input/menu/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "System click", "/user/hand/right",
			"/user/hand/right/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

		register_io_path(profile_path, "X click", "/user/hand/left",
			"/user/hand/left/input/x/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "X touch", "/user/hand/left",
			"/user/hand/left/input/x/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y click", "/user/hand/left",
			"/user/hand/left/input/y/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y touch", "/user/hand/left",
			"/user/hand/left/input/y/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A click", "/user/hand/right",
			"/user/hand/right/input/a/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A touch", "/user/hand/right",
			"/user/hand/right/input/a/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B click", "/user/hand/right",
			"/user/hand/right/input/b/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B touch", "/user/hand/right",
			"/user/hand/right/input/b/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
	}

	{ // Valve Index controller
		const String profile_path = "/interaction_profiles/valve/index_controller";
		register_interaction_profile("Index controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "A click", user_path, user_path + "/input/a/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "A touch", user_path, user_path + "/input/a/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B click", user_path, user_path + "/input/b/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B touch", user_path, user_path + "/input/b/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Squeeze force", user_path,
				user_path + "/input/squeeze/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad force", user_path,
				user_path + "/input/trackpad/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionMap

void OpenXRActionMap::create_default_action_sets()
{
	Ref<OpenXRActionSet> action_set = OpenXRActionSet::new_action_set("godot", "Godot action set");
	add_action_set(action_set);

	Ref<OpenXRAction> trigger = action_set->add_new_action("trigger", "Trigger",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_click = action_set->add_new_action("trigger_click", "Trigger click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_touch = action_set->add_new_action("trigger_touch",
		"Trigger touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip = action_set->add_new_action(
		"grip", "Grip", OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_click = action_set->add_new_action("grip_click", "Grip click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_force = action_set->add_new_action("grip_force", "Grip force",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary =
		action_set->add_new_action("primary", "Primary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_click =
		action_set->add_new_action("primary_click", "Primary joystick/thumbstick/trackpad click",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_touch =
		action_set->add_new_action("primary_touch", "Primary joystick/thumbstick/trackpad touching",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary =
		action_set->add_new_action("secondary", "Secondary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_click = action_set->add_new_action("secondary_click",
		"Secondary joystick/thumbstick/trackpad click", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_touch = action_set->add_new_action("secondary_touch",
		"Secondary joystick/thumbstick/trackpad touching", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> menu_button = action_set->add_new_action("menu_button", "Menu button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> select_button = action_set->add_new_action("select_button", "Select button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_button = action_set->add_new_action("ax_button", "A/X button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_touch = action_set->add_new_action("ax_touch", "A/X touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_button = action_set->add_new_action("by_button", "B/Y button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_touch = action_set->add_new_action("by_touch", "B/Y touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> default_pose =
		action_set->add_new_action("default_pose", "Default pose", OpenXRAction::OPENXR_ACTION_POSE,
			"/user/hand/left,"
			"/user/hand/right,"
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle,"
			"/user/eyes_ext");
	Ref<OpenXRAction> aim_pose = action_set->add_new_action("aim_pose", "Aim pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_pose = action_set->add_new_action("grip_pose", "Grip pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> palm_pose = action_set->add_new_action("palm_pose", "Palm pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> haptic =
		action_set->add_new_action("haptic", "Haptic", OpenXRAction::OPENXR_ACTION_HAPTIC,
			"/user/hand/left,"
			"/user/hand/right,"
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle");

	// Create our interaction profiles.
	Ref<OpenXRInteractionProfile> profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/khr/generic_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/primary/click,/user/hand/right/input/primary/click");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/secondary/click,/user/hand/right/input/secondary/click");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Meta touch controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/oculus/touch_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/system/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Pico 4 controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/bytedance/pico4_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		select_button, "/user/hand/left/input/system/click,/user/hand/right/input/system/click");
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our hand interaction profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/ext/hand_interaction_ext");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	add_interaction_profile(profile);
}

void OpenXRActionMap::create_editor_action_sets() {}

Ref<OpenXRAction> OpenXRActionMap::get_action(const String& p_path) const
{
	PackedStringArray paths = p_path.split("/", false);
	ERR_FAIL_COND_V(paths.size() != 2, Ref<OpenXRAction>());

	Ref<OpenXRActionSet> action_set = find_action_set(paths[0]);
	if (action_set.is_valid()) {
		return action_set->get_action(paths[1]);
	}

	return Ref<OpenXRAction>();
}

void OpenXRActionMap::add_interaction_profile(const Ref<OpenXRInteractionProfile>&) {}

int OpenXRActionMap::get_interaction_profile_count() const { return 0; }

void OpenXRActionMap::add_action_set(Ref<OpenXRActionSet> const&) {}

void OpenXRActionMap::remove_interaction_profile(Ref<OpenXRInteractionProfile> const&) {}

void OpenXRActionMap::remove_action_set(Ref<OpenXRActionSet> const&) {}

Ref<OpenXRInteractionProfile> OpenXRActionMap::get_interaction_profile(int) const
{
	return Ref<OpenXRInteractionProfile>();
}

Ref<OpenXRActionSet> OpenXRActionMap::find_action_set(String const&) const
{
	return Ref<OpenXRActionSet>();
}

// ============================================================================
// Merged from modules/openxr/scene/openxr_scene.cpp
// ============================================================================

#include "modules/openxr/extensions/openxr_extensions.h"
#include "modules/openxr/openxr_api.h"
#include "modules/openxr/openxr_interface.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "modules/openxr/openxr_interface.h"
#include "platform/android/api/java_class_wrapper.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/xr/xr_nodes.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayer

Vector<OpenXRCompositionLayer*> OpenXRCompositionLayer::composition_layer_nodes;

static const char* HOLE_PUNCH_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode blend_mix, depth_draw_opaque, cull_back, shadow_to_opacity, shadows_disabled;\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

OpenXRCompositionLayer::OpenXRCompositionLayer(LayerType p_type) : layer_type(p_type)
{
	if (!OpenXRCompositionLayerExtension::is_initialized()) {
		return;
	}

	switch (layer_type) {
	case LAYER_QUAD: {
		XrCompositionLayerQuad openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_QUAD,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			{(float)quad_size.x, (float)quad_size.y},
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_CYLINDER: {
		XrCompositionLayerCylinderKHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_angle,
			aspect_ratio,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_EQUIRECT: {
		XrCompositionLayerEquirect2KHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_horizontal_angle,
			upper_vertical_angle,
			-lower_vertical_angle,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	}
}

XrStructureType OpenXRCompositionLayer::_get_openxr_type() const
{
	switch (layer_type) {
	case LAYER_QUAD:
		return XR_TYPE_COMPOSITION_LAYER_QUAD;
	case LAYER_CYLINDER:
		return XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR;
	case LAYER_EQUIRECT:
		return XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR;
	}
	return XR_TYPE_COMPOSITION_LAYER_QUAD;
}

Ref<Mesh> OpenXRCompositionLayer::_create_fallback_mesh()
{
	if (layer_type == LAYER_QUAD) {
		Ref<PrimitiveMesh> mesh;
		mesh.instantiate(PrimitiveMesh::MESH_QUAD);
		mesh->set_plane_size(quad_size);
		return mesh;
	}
	Ref<Mesh> m = memnew(Mesh);
	return m;
}

bool OpenXRCompositionLayer::_should_use_fallback_node()
{
	if (Engine::is_editor_hint() || !OpenXRAPI::is_allocated()) {
		return true;
	}
	else if (openxr_session_running) {
		return enable_hole_punch || !is_natively_supported();
	}
	return false;
}

void OpenXRCompositionLayer::_create_fallback_node()
{
	ERR_FAIL_COND(fallback);
	fallback = memnew(MeshInstance3D);
	fallback->set_cast_shadows_setting(RenderInstance3D::SHADOW_CASTING_SETTING_OFF);
	add_child(fallback, false, INTERNAL_MODE_FRONT);
	should_update_fallback_mesh = true;
}

void OpenXRCompositionLayer::_remove_fallback_node()
{
	ERR_FAIL_COND(fallback == nullptr);
	remove_child(fallback);
	fallback->queue_free();
	fallback = nullptr;
}

void OpenXRCompositionLayer::_setup_composition_layer()
{
	if (use_android_surface || layer_viewport) {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_register(composition_layer);
			registered = true;

			if (layer_viewport) {
				OpenXRCompositionLayerExtension::composition_layer_set_viewport(composition_layer,
					layer_viewport->get_viewport_rid(), layer_viewport->get_size());
			}
		}
	}
}

void OpenXRCompositionLayer::_clear_composition_layer()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_unregister(composition_layer);
		registered = false;

		if (!use_android_surface) {
			OpenXRCompositionLayerExtension::composition_layer_set_viewport(
				composition_layer, RID(), Size2i());
		}
	}
}

void OpenXRCompositionLayer::_viewport_size_changed()
{
	if (layer_viewport && openxr_session_running &&
		OpenXRCompositionLayerExtension::is_initialized() && is_natively_supported() &&
		is_visible_in_tree() && is_inside_tree()) {
		OpenXRCompositionLayerExtension::composition_layer_set_viewport(
			composition_layer, layer_viewport->get_viewport_rid(), layer_viewport->get_size());
	}
}

void OpenXRCompositionLayer::_on_openxr_session_begun()
{
	openxr_session_running = true;
	if (_should_register()) {
		_setup_composition_layer();
	}
	if (!fallback && _should_use_fallback_node()) {
		_create_fallback_node();
	}
}

void OpenXRCompositionLayer::_on_openxr_session_stopping()
{
	openxr_session_running = false;
	if (fallback && !_should_use_fallback_node()) {
		_remove_fallback_node();
	}
	_clear_composition_layer();
}

void OpenXRCompositionLayer::update_fallback_mesh() { should_update_fallback_mesh = true; }

bool OpenXRCompositionLayer::_should_register()
{
	return !registered && openxr_session_running && is_inside_tree() && is_visible_in_tree() &&
		   is_natively_supported();
}

bool OpenXRCompositionLayer::is_viewport_in_use(SubViewport* p_viewport)
{
	ERR_FAIL_NULL_V(p_viewport, false);
	for (const OpenXRCompositionLayer* other_composition_layer : composition_layer_nodes) {
		if (other_composition_layer != this && other_composition_layer->is_inside_tree() &&
			other_composition_layer->get_layer_viewport() == p_viewport) {
			return true;
		}
	}
	return false;
}

void OpenXRCompositionLayer::set_use_android_surface(bool p_use_android_surface)
{
	if (use_android_surface == p_use_android_surface) {
		return;
	}

	use_android_surface = p_use_android_surface;
	if (use_android_surface) {
		set_layer_viewport(nullptr);
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, true, android_surface_size);
		}
		if (_should_register()) {
			_setup_composition_layer();
		}
	}
	else {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, false, Size2i());
		}
	}
}

bool OpenXRCompositionLayer::get_use_android_surface() const { return use_android_surface; }

void OpenXRCompositionLayer::set_android_surface_size(Size2i p_size)
{
	if (android_surface_size == p_size) {
		return;
	}

	android_surface_size = p_size;
	if (use_android_surface && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
			composition_layer, true, android_surface_size);
	}
}

Size2i OpenXRCompositionLayer::get_android_surface_size() const { return android_surface_size; }

SubViewport* OpenXRCompositionLayer::get_layer_viewport() const { return layer_viewport; }

bool OpenXRCompositionLayer::get_enable_hole_punch() const { return enable_hole_punch; }

int OpenXRCompositionLayer::get_sort_order() const { return sort_order; }

void OpenXRCompositionLayer::set_alpha_blend(bool p_alpha_blend)
{
	alpha_blend = p_alpha_blend;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_blend(
			composition_layer, p_alpha_blend);
	}
	if (fallback) {
		_reset_fallback_material();
	}
}

bool OpenXRCompositionLayer::get_alpha_blend() const { return alpha_blend; }

bool OpenXRCompositionLayer::is_natively_supported() const
{
	if (OpenXRCompositionLayerExtension::is_initialized() && OpenXRAPI::is_allocated()) {
		return OpenXRCompositionLayerExtension::is_available(_get_openxr_type());
	}
	return false;
}

void OpenXRCompositionLayer::set_protected_content(bool p_protected_content)
{
	if (protected_content == p_protected_content) {
		return;
	}
	protected_content = p_protected_content;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_protected_content(
			composition_layer, p_protected_content);
	}
}

bool OpenXRCompositionLayer::is_protected_content() const { return protected_content; }

void OpenXRCompositionLayer::set_min_filter(Filter p_mode)
{
	if (min_filter == p_mode) {
		return;
	}
	min_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_min_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_min_filter() const { return min_filter; }

void OpenXRCompositionLayer::set_mag_filter(Filter p_mode)
{
	if (mag_filter == p_mode) {
		return;
	}
	mag_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mag_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_mag_filter() const { return mag_filter; }

void OpenXRCompositionLayer::set_mipmap_mode(MipmapMode p_mode)
{
	if (mipmap_mode == p_mode) {
		return;
	}
	mipmap_mode = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mipmap_mode(
			composition_layer, (OpenXRCompositionLayerExtension::MipmapMode)p_mode);
	}
}

OpenXRCompositionLayer::MipmapMode OpenXRCompositionLayer::get_mipmap_mode() const
{
	return mipmap_mode;
}

void OpenXRCompositionLayer::set_horizontal_wrap(Wrap p_mode)
{
	if (horizontal_wrap == p_mode) {
		return;
	}
	horizontal_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_horizontal_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_horizontal_wrap() const
{
	return horizontal_wrap;
}

void OpenXRCompositionLayer::set_vertical_wrap(Wrap p_mode)
{
	if (vertical_wrap == p_mode) {
		return;
	}
	vertical_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_vertical_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_vertical_wrap() const
{
	return vertical_wrap;
}

void OpenXRCompositionLayer::set_red_swizzle(Swizzle p_mode)
{
	if (red_swizzle == p_mode) {
		return;
	}
	red_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_red_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_red_swizzle() const
{
	return red_swizzle;
}

void OpenXRCompositionLayer::set_green_swizzle(Swizzle p_mode)
{
	if (green_swizzle == p_mode) {
		return;
	}
	green_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_green_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_green_swizzle() const
{
	return green_swizzle;
}

void OpenXRCompositionLayer::set_blue_swizzle(Swizzle p_mode)
{
	if (blue_swizzle == p_mode) {
		return;
	}
	blue_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_blue_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_blue_swizzle() const
{
	return blue_swizzle;
}

void OpenXRCompositionLayer::set_alpha_swizzle(Swizzle p_mode)
{
	if (alpha_swizzle == p_mode) {
		return;
	}
	alpha_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_alpha_swizzle() const
{
	return alpha_swizzle;
}

void OpenXRCompositionLayer::set_max_anisotropy(float p_value)
{
	if (max_anisotropy == p_value) {
		return;
	}
	max_anisotropy = p_value;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_max_anisotropy(
			composition_layer, p_value);
	}
}

float OpenXRCompositionLayer::get_max_anisotropy() const { return max_anisotropy; }

void OpenXRCompositionLayer::set_border_color(const Color& p_color)
{
	if (border_color == p_color) {
		return;
	}
	border_color = p_color;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_border_color(
			composition_layer, p_color);
	}
}

Color OpenXRCompositionLayer::get_border_color() const { return border_color; }

void OpenXRCompositionLayer::set_eye_visibility(EyeVisibility p_eye_visibility)
{
	if (eye_visibility == p_eye_visibility) {
		return;
	}
	eye_visibility = p_eye_visibility;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_eye_visibility(
			composition_layer, (OpenXRCompositionLayerExtension::EyeVisibility)p_eye_visibility);
	}
}

OpenXRCompositionLayer::EyeVisibility OpenXRCompositionLayer::get_eye_visibility() const
{
	return eye_visibility;
}

void OpenXRCompositionLayer::set_quad_size(const Size2& p_size)
{
	quad_size = p_size;
	if (layer_type == LAYER_QUAD && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_quad_size(composition_layer, p_size);
	}
	update_fallback_mesh();
}

Size2 OpenXRCompositionLayer::get_quad_size() const { return quad_size; }

void OpenXRCompositionLayer::set_radius(float p_radius)
{
	ERR_FAIL_COND(p_radius <= 0);
	radius = p_radius;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		if (layer_type == LAYER_CYLINDER) {
			OpenXRCompositionLayerExtension::composition_layer_set_cylinder_radius(
				composition_layer, p_radius);
		}
		else if (layer_type == LAYER_EQUIRECT) {
			OpenXRCompositionLayerExtension::composition_layer_set_equirect_radius(
				composition_layer, p_radius);
		}
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_radius() const { return radius; }

void OpenXRCompositionLayer::set_fallback_segments(uint32_t p_fallback_segments)
{
	ERR_FAIL_COND(p_fallback_segments == 0);
	fallback_segments = p_fallback_segments;
	update_fallback_mesh();
}

uint32_t OpenXRCompositionLayer::get_fallback_segments() const { return fallback_segments; }

void OpenXRCompositionLayer::set_aspect_ratio(float p_aspect_ratio)
{
	ERR_FAIL_COND(p_aspect_ratio <= 0);
	aspect_ratio = p_aspect_ratio;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_aspect_ratio(
			composition_layer, p_aspect_ratio);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_aspect_ratio() const { return aspect_ratio; }

void OpenXRCompositionLayer::set_central_angle(float p_central_angle)
{
	ERR_FAIL_COND(p_central_angle <= 0);
	central_angle = p_central_angle;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_central_angle(
			composition_layer, p_central_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_angle() const { return central_angle; }

void OpenXRCompositionLayer::set_central_horizontal_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0);
	central_horizontal_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_central_horizontal_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_horizontal_angle() const
{
	return central_horizontal_angle;
}

void OpenXRCompositionLayer::set_upper_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	upper_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_upper_vertical_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_upper_vertical_angle() const
{
	return upper_vertical_angle;
}

void OpenXRCompositionLayer::set_lower_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	lower_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_lower_vertical_angle(
			composition_layer, -p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_lower_vertical_angle() const
{
	return lower_vertical_angle;
}

Ref<JavaObject> OpenXRCompositionLayer::get_android_surface()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		return OpenXRCompositionLayerExtension::composition_layer_get_android_surface(
			composition_layer);
	}
	return Ref<JavaObject>();
}

Vector2 OpenXRCompositionLayer::intersects_ray(
	const Vector3& p_origin, const Vector3& p_direction) const
{
	switch (layer_type) {
	case LAYER_QUAD: {
		Transform3D quad_transform = get_global_transform();
		Vector3 quad_normal = quad_transform.basis.get_column(2);

		float denom = quad_normal.dot(p_direction);
		if (Math::abs(denom) > 0.0001) {
			Vector3 vector = quad_transform.origin - p_origin;
			float t = vector.dot(quad_normal) / denom;
			if (t < 0.0) {
				return Vector2(-1.0, -1.0);
			}
			Vector3 intersection = p_origin + p_direction * t;

			Vector3 relative_point = intersection - quad_transform.origin;
			Vector2 projected_point = Vector2(relative_point.dot(quad_transform.basis.get_column(0)),
				relative_point.dot(quad_transform.basis.get_column(1)));
			if (Math::abs(projected_point.x) > quad_size.x / 2.0) {
				return Vector2(-1.0, -1.0);
			}
			if (Math::abs(projected_point.y) > quad_size.y / 2.0) {
				return Vector2(-1.0, -1.0);
			}

			float u = 0.5 + (projected_point.x / quad_size.x);
			float v = 1.0 - (0.5 + (projected_point.y / quad_size.y));

			return Vector2(u, v);
		}
		return Vector2(-1.0, -1.0);
	}
	case LAYER_CYLINDER: {
		Transform3D cylinder_transform = get_global_transform();
		Vector3 cylinder_axis = cylinder_transform.basis.get_column(1);

		Vector3 offset = p_origin - cylinder_transform.origin;
		float a = p_direction.dot(p_direction - cylinder_axis * p_direction.dot(cylinder_axis));
		float b = 2.0 * (p_direction.dot(offset - cylinder_axis * offset.dot(cylinder_axis)));
		float c =
			offset.dot(offset - cylinder_axis * offset.dot(cylinder_axis)) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = cylinder_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - cylinder_transform.origin);

		Vector2 projected_point = Vector2(relative_point.x, relative_point.z);
		float intersection_angle = Math::atan2(projected_point.y, projected_point.x);
		if (Math::abs(intersection_angle) > central_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float arc_length = radius * central_angle;
		float height = aspect_ratio * arc_length;
		if (Math::abs(relative_point.y) > height / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float u = 0.5 + (intersection_angle / central_angle);
		float v = 1.0 - (0.5 + (relative_point.y / height));

		return Vector2(u, v);
	}
	case LAYER_EQUIRECT: {
		Transform3D equirect_transform = get_global_transform();

		Vector3 offset = p_origin - equirect_transform.origin;
		float a = p_direction.dot(p_direction);
		float b = 2.0 * offset.dot(p_direction);
		float c = offset.dot(offset) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = equirect_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - equirect_transform.origin);

		float horizontal_intersection_angle = Math::atan2(relative_point.z, relative_point.x);
		if (Math::abs(horizontal_intersection_angle) > central_horizontal_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float vertical_intersection_angle =
			Math::acos(relative_point.y / radius) - (Math::PI / 2.0);
		if (vertical_intersection_angle < 0) {
			if (Math::abs(vertical_intersection_angle) > upper_vertical_angle) {
				return Vector2(-1.0, -1.0);
			}
		}
		else if (vertical_intersection_angle > lower_vertical_angle) {
			return Vector2(-1.0, -1.0);
		}

		if (upper_vertical_angle != lower_vertical_angle) {
			vertical_intersection_angle -= (-upper_vertical_angle + lower_vertical_angle) / 2.0;
		}

		float u = 0.5 + (horizontal_intersection_angle / central_horizontal_angle);
		float v = 0.5 + (vertical_intersection_angle / (upper_vertical_angle + lower_vertical_angle));

		return Vector2(u, v);
	}
	}
	return Vector2(-1.0, -1.0);
}

void OpenXRCompositionLayer::_reset_fallback_material()
{
	ERR_FAIL_NULL(fallback);

	if (fallback->get_mesh().is_null()) {
		return;
	}

	if (enable_hole_punch && !Engine::is_editor_hint() && is_natively_supported()) {
		Ref<ShaderMaterial> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			Ref<Shader> shader;
			shader.instantiate();
			shader->set_code(HOLE_PUNCH_SHADER_CODE);

			material.instantiate();
			material->set_shader(shader);
			fallback->set_surface_override_material(0, material);
		}
	}
	else if (layer_viewport) {
		Ref<StandardMaterial3D> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			material.instantiate();
			material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
			material->set_local_to_scene(true);
			fallback->set_surface_override_material(0, material);
		}
		material->set_transparency(get_alpha_blend() ? StandardMaterial3D::TRANSPARENCY_ALPHA
													 : StandardMaterial3D::TRANSPARENCY_DISABLED);
		material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, layer_viewport->get_texture());
	}
	else {
		fallback->set_surface_override_material(0, Ref<Material>());
	}
}

PackedStringArray OpenXRCompositionLayer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_transform().basis.is_orthonormal()) {
		warnings.push_back(RTR("OpenXR composition layers must have orthonormalized transforms "
							   "(ie. no scale or shearing)."));
	}

	if (enable_hole_punch && get_sort_order() >= 0) {
		warnings.push_back(
			RTR("Hole punching won't work as expected unless the sort order is less than zero."));
	}

	return warnings;
}

Camera3D* OpenXRCompositionLayer::_get_xrcamera3d_ancestor() const
{
	Node* parent = get_parent();
	while (parent != nullptr) {
		Camera3D* camera = dynamic_cast<Camera3D*>(parent);
		if (camera != nullptr && camera->is_xr_enabled()) {
			return camera;
		}
		parent = parent->get_parent();
	}
	return nullptr;
}

void OpenXRCompositionLayer::set_layer_viewport(SubViewport* p_viewport) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRHand

OpenXRHand::OpenXRHand() {}

void OpenXRHand::set_hand(Hands p_hand)
{
	ERR_FAIL_INDEX(p_hand, HAND_MAX);

	hand = p_hand;
}

OpenXRHand::Hands OpenXRHand::get_hand() const { return hand; }

void OpenXRHand::set_hand_skeleton(const NodePath& p_hand_skeleton)
{
	hand_skeleton = p_hand_skeleton;
}

void OpenXRHand::set_motion_range(MotionRange p_motion_range)
{
	ERR_FAIL_INDEX(p_motion_range, MOTION_RANGE_MAX);
	motion_range = p_motion_range;

	_set_motion_range();
}

OpenXRHand::MotionRange OpenXRHand::get_motion_range() const { return motion_range; }

NodePath OpenXRHand::get_hand_skeleton() const { return hand_skeleton; }

void OpenXRHand::set_skeleton_rig(SkeletonRig p_skeleton_rig)
{
	ERR_FAIL_INDEX(p_skeleton_rig, SKELETON_RIG_MAX);

	skeleton_rig = p_skeleton_rig;
}

OpenXRHand::SkeletonRig OpenXRHand::get_skeleton_rig() const { return skeleton_rig; }

void OpenXRHand::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);

	bone_update = p_bone_update;
}

OpenXRHand::BoneUpdate OpenXRHand::get_bone_update() const { return bone_update; }

void OpenXRHand::_get_joint_data()
{
	static const String bone_names[SKELETON_RIG_MAX][XR_HAND_JOINT_COUNT_EXT] = {
		{"Palm", "Wrist", "Thumb_Metacarpal", "Thumb_Proximal", "Thumb_Distal", "Thumb_Tip",
			"Index_Metacarpal", "Index_Proximal", "Index_Intermediate", "Index_Distal", "Index_Tip",
			"Middle_Metacarpal", "Middle_Proximal", "Middle_Intermediate", "Middle_Distal",
			"Middle_Tip", "Ring_Metacarpal", "Ring_Proximal", "Ring_Intermediate", "Ring_Distal",
			"Ring_Tip", "Little_Metacarpal", "Little_Proximal", "Little_Intermediate",
			"Little_Distal", "Little_Tip"},
		{"Palm", "Hand", "ThumbMetacarpal", "ThumbProximal", "ThumbDistal", "ThumbTip",
			"IndexMetacarpal", "IndexProximal", "IndexIntermediate", "IndexDistal", "IndexTip",
			"MiddleMetacarpal", "MiddleProximal", "MiddleIntermediate", "MiddleDistal", "MiddleTip",
			"RingMetacarpal", "RingProximal", "RingIntermediate", "RingDistal", "RingTip",
			"LittleMetacarpal", "LittleProximal", "LittleIntermediate", "LittleDistal",
			"LittleTip"}};

	static const String bone_name_formats[SKELETON_RIG_MAX][2] = {
		{"<bone>_L", "<bone>_R"},
		{"Left<bone>", "Right<bone>"}};

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i].bone = -1;
		joints[i].parent_joint = -1;
	}

	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	int bones[XR_HAND_JOINT_COUNT_EXT];
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		String bone_name =
			bone_name_formats[skeleton_rig][hand].replace("<bone>", bone_names[skeleton_rig][i]);
		bones[i] = skeleton->find_bone(bone_name);
	}

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		const int bone = bones[i];
		if (bone == -1) {
			continue;
		}

		const int parent_bone = skeleton->get_bone_parent(bone);
		if (parent_bone == -1) {
			joints[i].bone = bone;
			joints[i].parent_joint = XR_HAND_JOINT_PALM_EXT;
			continue;
		}

		for (int j = 0; j < XR_HAND_JOINT_COUNT_EXT; ++j) {
			if (bones[j] == parent_bone) {
				joints[i].bone = bone;
				joints[i].parent_joint = j;
				break;
			}
		}
	}
}

void OpenXRHand::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_joint_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			joints[i].bone = -1;
			joints[i].parent_joint = -1;
		}
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_skeleton();
	} break;
	default: {
	} break;
	}
}

void OpenXRHand::_update_skeleton() {}

void OpenXRHand::_set_motion_range() {}

Skeleton3D* OpenXRHand::get_skeleton() { return nullptr; }

////////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMask

void OpenXRVisibilityMask::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void OpenXRVisibilityMask::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_base(RID());
	} break;
	}
}

void OpenXRVisibilityMask::_on_openxr_session_begun()
{
	if (is_inside_tree()) {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	}
}

void OpenXRVisibilityMask::_on_openxr_session_stopping() { set_base(RID()); }

AABB OpenXRVisibilityMask::get_aabb() const
{
	AABB ret;
	ret.position = Vector3(-1000.0, -1000.0, -1000.0);
	ret.size = Vector3(2000.0, 2000.0, 2000.0);
	return ret;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRRenderModel & OpenXRRenderModelManager

#ifdef MODULE_GLTF_ENABLED

void OpenXRRenderModel::_load_render_model_scene() {}

String OpenXRRenderModel::get_top_level_path() const
{
	String ret;

	if (render_model.is_valid() && OpenXRRenderModelExtension::is_initialized()) {
		ret = OpenXRRenderModelExtension::render_model_get_top_level_path_as_string(render_model);
	}

	return ret;
}

RID OpenXRRenderModel::get_render_model() const { return render_model; }

void OpenXRRenderModel::set_render_model(RID p_render_model)
{
	render_model = p_render_model;
	if (is_inside_tree() && render_model.is_valid()) {
		_load_render_model_scene();
	}
}

bool OpenXRRenderModelManager::_has_filters() { return tracker != 0; }

void OpenXRRenderModelManager::_on_render_model_added(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_removed(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_top_level_path_changed(RID p_path)
{
	if (_has_filters()) {
		is_dirty = true;
	}
}

void OpenXRRenderModelManager::set_tracker(RenderModelTracker p_tracker)
{
	if (tracker != p_tracker) {
		tracker = p_tracker;
		is_dirty = true;

		if (tracker == RENDER_MODEL_TRACKER_ANY || tracker == RENDER_MODEL_TRACKER_NONE_SET) {
			xr_path = XR_NULL_PATH;
		}
		else if (!Engine::is_editor_hint()) {
			if (OpenXRAPI::is_allocated() && XRServer::is_initialized()) {
				String toplevel_path;
				String tracker_name;
				if (tracker == RENDER_MODEL_TRACKER_LEFT_HAND) {
					tracker_name = "left_hand";
					toplevel_path = "/user/hand/left";
				}
				else if (tracker == RENDER_MODEL_TRACKER_RIGHT_HAND) {
					tracker_name = "right_hand";
					toplevel_path = "/user/hand/right";
				}
				else {
					ERR_FAIL_MSG("Unsupported tracker value set.");
				}

				positional_tracker = XRServer::get_tracker(tracker_name);
				if (positional_tracker.is_null()) {
					WARN_PRINT("OpenXR: Can't find tracker " + tracker_name);
				}

				xr_path = OpenXRAPI::get_xr_path(toplevel_path);
				if (xr_path == XR_NULL_PATH) {
					WARN_PRINT("OpenXR: Can't find path for " + toplevel_path);
				}
			}
		}

		set_process_internal(true);
	}
}

OpenXRRenderModelManager::RenderModelTracker OpenXRRenderModelManager::get_tracker() const
{
	return tracker;
}

void OpenXRRenderModelManager::set_make_local_to_pose(const String& p_action)
{
	if (make_local_to_pose != p_action) {
		make_local_to_pose = p_action;

		if (container) {
			container->set_transform(Transform3D());
		}
	}
}

String OpenXRRenderModelManager::get_make_local_to_pose() const { return make_local_to_pose; }

void OpenXRRenderModelManager::_update_models() {}

#endif // MODULE_GLTF_ENABLED
