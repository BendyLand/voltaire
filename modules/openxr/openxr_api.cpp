/**************************************************************************/
/*  openxr_api.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "action_map/openxr_interaction_profile_metadata.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/memory.h"
#include "core/types.h"
#include "core/version.h"
#include "openxr_api.h"
#include "openxr_interface.h"
#include "openxr_util.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_globals.h"

#ifdef ANDROID_ENABLED
#include "core/os/os.h"
#endif

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

#include "extensions/openxr_composition_layer_depth_extension.h"
#include "extensions/openxr_debug_utils_extension.h"
#include "extensions/openxr_eye_gaze_interaction.h"
#include "extensions/openxr_fb_display_refresh_rate_extension.h"
#include "extensions/openxr_fb_foveation_extension.h"
#include "extensions/openxr_fb_update_swapchain_extension.h"
#include "extensions/openxr_hand_tracking_extension.h"

#ifdef ANDROID_ENABLED
#define OPENXR_LOADER_NAME "libopenxr_loader.so"
#endif

////////////////////////////////////
// OpenXRAPI::OpenXRSwapChainInfo

Vector<OpenXRAPI::OpenXRSwapChainInfo> OpenXRAPI::OpenXRSwapChainInfo::free_queue;

bool OpenXRAPI::OpenXRSwapChainInfo::create(XrSwapchainCreateFlags p_create_flags,
	XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, bool p_use_next_extensions)
{
	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, false);

	XrSession xr_session = openxr_api->get_session();
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
	result = openxr_api->xrCreateSwapchain(xr_session, &swapchain_create_info, &new_swapchain);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain [", openxr_api->get_error_string(result),
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
	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	if (image_acquired) {
		release();
	}

	if (swapchain != XR_NULL_HANDLE) {
		openxr_api->xrDestroySwapchain(swapchain);
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

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, false);

	XrSwapchainImageReleaseInfo swapchain_image_release_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, // type
		nullptr								  // next
	};
	XrResult result = openxr_api->xrReleaseSwapchainImage(swapchain, &swapchain_image_release_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to release swapchain image! [",
		// openxr_api->get_error_string(result), "]");
		return false;
	}

	return true;
}

////////////////////////////////////
// OpenXRAPI

OpenXRAPI* OpenXRAPI::singleton = nullptr;

bool OpenXRAPI::openxr_is_enabled(bool p_check_run_in_editor)
{
	if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
		if (Engine::get_singleton()->is_editor_hint() && p_check_run_in_editor) {
			// For now, don't start OpenXR when the editor starts up. In the future, this may change
			// if we want to integrate more XR features into the editor experience.
			return false;
		}
	}
}

void OpenXRAPI::set_object_name(
	XrObjectType p_object_type, uint64_t p_object_handle, const String& p_object_name)
{
	OpenXRDebugUtilsExtension* debug_utils = OpenXRDebugUtilsExtension::get_singleton();
	if (!debug_utils || !debug_utils->get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	debug_utils->set_object_name(p_object_type, p_object_handle, p_object_name.utf8().get_data());
}

void OpenXRAPI::begin_debug_label_region(const String& p_label_name)
{
	OpenXRDebugUtilsExtension* debug_utils = OpenXRDebugUtilsExtension::get_singleton();
	if (!debug_utils || !debug_utils->get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	debug_utils->begin_debug_label_region(p_label_name.utf8().get_data());
}

void OpenXRAPI::end_debug_label_region()
{
	OpenXRDebugUtilsExtension* debug_utils = OpenXRDebugUtilsExtension::get_singleton();
	if (!debug_utils || !debug_utils->get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	debug_utils->end_debug_label_region();
}

void OpenXRAPI::insert_debug_label(const String& p_label_name)
{
	OpenXRDebugUtilsExtension* debug_utils = OpenXRDebugUtilsExtension::get_singleton();
	if (!debug_utils || !debug_utils->get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	debug_utils->insert_debug_label(p_label_name.utf8().get_data());
}

bool OpenXRAPI::load_layer_properties()
{
	// This queries additional layers that are available and can be initialized when we create our
	// OpenXR instance
	if (!layer_properties.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_layer_properties = 0;
	XrResult result = xrEnumerateApiLayerProperties(0, &num_layer_properties, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of api layer properties");

	layer_properties.resize(num_layer_properties);
	for (XrApiLayerProperties& layer : layer_properties) {
		layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		layer.next = nullptr;
	}

	result = xrEnumerateApiLayerProperties(
		num_layer_properties, &num_layer_properties, layer_properties.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate api layer properties");

	for (const XrApiLayerProperties& layer : layer_properties) {
		print_verbose(vformat("OpenXR: Found OpenXR layer %s.", layer.layerName));
	}

	return true;
}

bool OpenXRAPI::load_supported_extensions()
{
	// This queries supported extensions that are available and can be initialized when we create
	// our OpenXR instance

	if (!supported_extensions.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_supported_extensions = 0;
	XrResult result =
		xrEnumerateInstanceExtensionProperties(nullptr, 0, &num_supported_extensions, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of extension properties");

	supported_extensions.resize(num_supported_extensions);

	// set our types
	for (XrExtensionProperties& extension : supported_extensions) {
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		extension.next = nullptr;
	}
	result = xrEnumerateInstanceExtensionProperties(
		nullptr, num_supported_extensions, &num_supported_extensions, supported_extensions.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate extension properties");

	for (const XrExtensionProperties& extension : supported_extensions) {
		print_verbose(vformat("OpenXR: Found OpenXR extension %s.", extension.extensionName));
	}

	return true;
}

bool OpenXRAPI::is_extension_supported(const String& p_extension) const
{
	for (const XrExtensionProperties& extension : supported_extensions) {
		if (extension.extensionName == p_extension) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_any_extension_enabled(const String& p_extensions) const
{
	// We allow a comma separated list of extensions here, only one needs to be supported.
	// This allows us to check for extensions that were renamed or that were embedded in core
	// at a specific OpenXR version.
	for (const String& name : p_extensions.split(",", false)) {
		CharString extension = name.utf8();

		if (enabled_extensions.has(extension)) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_top_level_path_supported(const String& p_toplevel_path)
{
	String required_extensions =
		OpenXRInteractionProfileMetadata::get_singleton()->get_top_level_extensions(
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
		OpenXRInteractionProfileMetadata::get_singleton()->get_interaction_profile_extensions(
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
		OpenXRInteractionProfileMetadata::get_singleton()->get_io_path(p_ip_path, p_io_path);

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

	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	supported_view_configuration_types.clear();

	uint32_t num_view_configuration_types = 0;
	XrResult result = xrEnumerateViewConfigurations(
		instance, system_id, 0, &num_view_configuration_types, nullptr);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to get view configuration count [", get_error_string(result), "]");
		return false;
	}

	supported_view_configuration_types.resize(num_view_configuration_types);

	result = xrEnumerateViewConfigurations(instance, system_id, num_view_configuration_types,
		&num_view_configuration_types, supported_view_configuration_types.ptr());
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerateview configurations");
	ERR_FAIL_COND_V_MSG(num_view_configuration_types == 0, false,
		"OpenXR: Failed to enumerateview configurations"); // JIC there should be at least 1!

	for (const XrViewConfigurationType& view_configuration_type :
		supported_view_configuration_types) {
		print_verbose(vformat("OpenXR: Found supported view configuration %s.",
			OpenXRUtil::get_view_configuration_name(view_configuration_type)));
	}

	// Check value we loaded at startup...
	if (!is_view_configuration_supported(view_configuration)) {
		print_verbose(vformat("OpenXR: %s isn't supported, defaulting to %s.",
			OpenXRUtil::get_view_configuration_name(view_configuration),
			OpenXRUtil::get_view_configuration_name(supported_view_configuration_types[0])));

		view_configuration = supported_view_configuration_types[0];
	}

	return true;
}

bool OpenXRAPI::load_supported_environmental_blend_modes()
{
	// This queries the supported environmental blend modes.

	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	supported_environment_blend_modes.clear();

	uint32_t num_supported_environment_blend_modes = 0;
	XrResult result = xrEnumerateEnvironmentBlendModes(instance, system_id, view_configuration, 0,
		&num_supported_environment_blend_modes, nullptr);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get supported environmental blend mode count [",
		// 	get_error_string(result), "]");
		return false;
	}

	supported_environment_blend_modes.resize(num_supported_environment_blend_modes);

	result = xrEnumerateEnvironmentBlendModes(instance, system_id, view_configuration,
		num_supported_environment_blend_modes, &num_supported_environment_blend_modes,
		supported_environment_blend_modes.ptrw());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate environmental blend modes");
	ERR_FAIL_COND_V_MSG(num_supported_environment_blend_modes == 0, false,
		"OpenXR: Failed to enumerate environmental blend modes"); // JIC there should be at least 1!

	for (const XrEnvironmentBlendMode& supported_environment_blend_mode :
		supported_environment_blend_modes) {
		print_verbose(vformat("OpenXR: Found environmental blend mode %s.",
			OpenXRUtil::get_environment_blend_mode_name(supported_environment_blend_mode)));
	}

	return true;
}

bool OpenXRAPI::is_view_configuration_supported(XrViewConfigurationType p_configuration_type) const
{
	return supported_view_configuration_types.has(p_configuration_type);
}

bool OpenXRAPI::is_reference_space_supported(XrReferenceSpaceType p_reference_space)
{
	return supported_reference_spaces.has(p_reference_space);
}

bool OpenXRAPI::is_swapchain_format_supported(int64_t p_swapchain_format)
{
	return supported_swapchain_formats.has(p_swapchain_format);
}

void OpenXRAPI::set_form_factor(XrFormFactor p_form_factor)
{
	ERR_FAIL_COND(is_initialized());

	form_factor = p_form_factor;
}

uint32_t OpenXRAPI::get_view_count() const { return view_configuration_views.size(); }

void OpenXRAPI::set_view_configuration(XrViewConfigurationType p_view_configuration)
{
	ERR_FAIL_COND(is_initialized());

	view_configuration = p_view_configuration;
}

bool OpenXRAPI::set_requested_reference_space(XrReferenceSpaceType p_requested_reference_space)
{
	if (custom_play_space != XR_NULL_HANDLE) {
		return false;
	}

	requested_reference_space = p_requested_reference_space;
	play_space_is_dirty = true;

	return true;
}

void OpenXRAPI::set_custom_play_space(XrSpace p_custom_space)
{
	custom_play_space = p_custom_space;
	play_space_is_dirty = true;
}

void OpenXRAPI::set_submit_depth_buffer(bool p_submit_depth_buffer)
{
	ERR_FAIL_COND(is_initialized());

	submit_depth_buffer = p_submit_depth_buffer;
}

bool OpenXRAPI::is_initialized() { return (instance != XR_NULL_HANDLE); }

bool OpenXRAPI::is_running()
{
	if (instance == XR_NULL_HANDLE) {
		return false;
	}
	if (session == XR_NULL_HANDLE) {
		return false;
	}

	return running;
}

bool OpenXRAPI::openxr_loader_init()
{
#ifdef ANDROID_ENABLED
	ERR_FAIL_COND_V_MSG(
		openxr_loader_library_handle != nullptr, false, "OpenXR Loader library is already loaded.");

	{
		Error error_code = OS::get_singleton()->open_dynamic_library(
			OPENXR_LOADER_NAME, openxr_loader_library_handle);
		ERR_FAIL_COND_V_MSG(error_code != OK, false, "OpenXR loader not found.");
	}

	{
		Error error_code = OS::get_singleton()->get_dynamic_library_symbol_handle(
			openxr_loader_library_handle, "xrGetInstanceProcAddr", (void*&)xrGetInstanceProcAddr);
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
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

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
	return xrGetInstanceProcAddr(instance, p_name, p_addr);
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

	allocate_view_buffers(view_configuration_views.size(), submit_depth_buffer);

	return true;
}

void OpenXRAPI::finish()
{
	destroy_session();

	destroy_instance();
}

void OpenXRAPI::set_xr_interface(OpenXRInterface* p_xr_interface) { xr_interface = p_xr_interface; }

Size2 OpenXRAPI::get_recommended_target_size()
{
	RenderingServer* rendering_server = RenderingServer::get_singleton();
	ERR_FAIL_COND_V(view_configuration_views.is_empty(), Size2());

	Size2 target_size;

	if (rendering_server && rendering_server->is_on_render_thread()) {
		target_size.width = view_configuration_views[0].recommendedImageRectWidth *
							render_state.render_target_size_multiplier;
		target_size.height = view_configuration_views[0].recommendedImageRectHeight *
							 render_state.render_target_size_multiplier;
	}
	else {
		target_size.width =
			view_configuration_views[0].recommendedImageRectWidth * render_target_size_multiplier;
		target_size.height =
			view_configuration_views[0].recommendedImageRectHeight * render_target_size_multiplier;
	}

	return target_size;
}

XRPose::TrackingConfidence OpenXRAPI::get_head_center(
	Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	XrResult result;

	if (!running) {
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

	result = xrLocateSpace(view_space, play_space, display_time, &location);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to locate view space in play space [", get_error_string(result), "]");
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XRPose::TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	if (head_pose_confidence != confidence) {
		// prevent error spam
		head_pose_confidence = confidence;
		if (head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
			// print_line("OpenXR head space location not valid (check tracking?)");
		}
		else if (head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_LOW) {
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

	if (!render_state.running) {
		return false;
	}

	// we don't have valid view info
	if (render_state.views.is_empty() || !render_state.view_pose_valid) {
		return false;
	}

	// Note, the timing of this is set right before rendering, which is what we need here.
	r_transform = transform_from_pose(render_state.views[p_view].pose);

	return true;
}

void OpenXRAPI::_allocate_view_buffers_rt(uint32_t p_view_count, bool p_submit_depth_buffer)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	openxr_api->render_state.submit_depth_buffer = p_submit_depth_buffer;

	// Allocate buffers we'll be populating with view information.
	openxr_api->render_state.views.resize(p_view_count);
	openxr_api->render_state.projection_views.resize(p_view_count);

	for (uint32_t i = 0; i < p_view_count; i++) {
		openxr_api->render_state.views[i] = {
			XR_TYPE_VIEW, // type
			nullptr,	  // next
			{},			  // pose
			{},			  // fov
		};
		openxr_api->render_state.projection_views[i] = {
			XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, // type
			nullptr,								   // next
			{},										   // pose
			{},										   // fov
			{},										   // subImage
		};
	}

	if (p_submit_depth_buffer &&
		OpenXRCompositionLayerDepthExtension::get_singleton()->is_available()) {
		openxr_api->render_state.depth_views.resize(p_view_count);

		for (uint32_t i = 0; i < p_view_count; i++) {
			openxr_api->render_state.depth_views[i] = {
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

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.running = p_is_running;
}

void OpenXRAPI::_set_render_display_info_rt(XrTime p_predicted_display_time, bool p_should_render)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.predicted_display_time = p_predicted_display_time;
	openxr_api->render_state.should_render = p_should_render;
}

void OpenXRAPI::_set_render_environment_blend_mode_rt(int32_t p_environment_blend_mode)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.environment_blend_mode =
		XrEnvironmentBlendMode(p_environment_blend_mode);
}

void OpenXRAPI::_set_render_play_space_rt(uint64_t p_play_space)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.play_space = XrSpace(p_play_space);
}

void OpenXRAPI::_set_render_state_multiplier_rt(double p_render_target_size_multiplier)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.render_target_size_multiplier = p_render_target_size_multiplier;
}

void OpenXRAPI::_set_render_state_render_region_rt(const Rect2i& p_render_region)
{
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);
	openxr_api->render_state.render_region = p_render_region;
}

void OpenXRAPI::_update_main_swapchain_size_rt()
{
	ERR_NOT_ON_RENDER_THREAD;

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	uint32_t view_count_output = 0;
	XrResult result = openxr_api->xrEnumerateViewConfigurationViews(openxr_api->instance,
		openxr_api->system_id, openxr_api->view_configuration, openxr_api->get_view_count(),
		&view_count_output, openxr_api->view_configuration_views.ptr());
	if (XR_FAILED(result)) {
		return;
	}

#ifdef DEBUG_ENABLED
	for (uint32_t i = 0; i < view_count_output; i++) {
		print_verbose("OpenXR: Recommended resolution changed");
		print_verbose(String(" - recommended render width: ") +
					  itos(openxr_api->view_configuration_views[i].recommendedImageRectWidth));
		print_verbose(String(" - recommended render height: ") +
					  itos(openxr_api->view_configuration_views[i].recommendedImageRectHeight));
	}
#endif
}

void OpenXRAPI::free_main_swapchains()
{
	for (int i = 0; i < OPENXR_SWAPCHAIN_MAX; i++) {
		render_state.main_swapchains[i].queue_free();
	}
}

XrSwapchain OpenXRAPI::get_color_swapchain()
{
	ERR_NOT_ON_RENDER_THREAD_V(XR_NULL_HANDLE);

	return render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_swapchain();
}

RID OpenXRAPI::get_color_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	return render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_image();
}

RID OpenXRAPI::get_depth_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	// Note, image will not be acquired if we didn't have a suitable swap chain format.
	if (render_state.submit_depth_buffer &&
		render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].is_image_acquired()) {
		return render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].get_image();
	}
	else {
		return RID();
	}
}

RID OpenXRAPI::get_density_map_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext && fov_ext->is_enabled()) {
		return render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_density_map();
	}

	return RID();
}

void OpenXRAPI::set_velocity_texture(RID p_render_target) { velocity_texture = p_render_target; }

RID OpenXRAPI::get_velocity_texture() { return velocity_texture; }

void OpenXRAPI::set_velocity_depth_texture(RID p_render_target)
{
	velocity_depth_texture = p_render_target;
}

RID OpenXRAPI::get_velocity_depth_texture() { return velocity_depth_texture; }

void OpenXRAPI::set_velocity_target_size(const Size2i& p_target_size)
{
	velocity_target_size = p_target_size;
}

Size2i OpenXRAPI::get_velocity_target_size() { return velocity_target_size; }

const XrCompositionLayerProjection* OpenXRAPI::get_projection_layer() const
{
	ERR_NOT_ON_RENDER_THREAD_V(nullptr);

	return &render_state.projection_layer;
}

float OpenXRAPI::get_display_refresh_rate() const
{
	OpenXRDisplayRefreshRateExtension* drrext = OpenXRDisplayRefreshRateExtension::get_singleton();
	if (drrext) {
		return drrext->get_refresh_rate();
	}

	return 0.0;
}

void OpenXRAPI::set_display_refresh_rate(float p_refresh_rate)
{
	OpenXRDisplayRefreshRateExtension* drrext = OpenXRDisplayRefreshRateExtension::get_singleton();
	if (drrext != nullptr) {
		drrext->set_refresh_rate(p_refresh_rate);
	}
}

double OpenXRAPI::get_render_target_size_multiplier() const
{
	return render_target_size_multiplier;
}

void OpenXRAPI::set_render_target_size_multiplier(double multiplier)
{
	render_target_size_multiplier = multiplier;
	set_render_state_multiplier(multiplier);
}

Rect2i OpenXRAPI::get_render_region() const { return render_region; }

void OpenXRAPI::set_render_region(const Rect2i& p_render_region)
{
	render_region = p_render_region;
	set_render_state_render_region(p_render_region);
}

bool OpenXRAPI::is_foveation_supported() const
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	return fov_ext != nullptr && fov_ext->is_enabled();
}

int OpenXRAPI::get_foveation_level() const
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr && fov_ext->is_enabled()) {
		switch (fov_ext->get_foveation_level()) {
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
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr && fov_ext->is_enabled()) {
		XrFoveationLevelFB levels[] = {XR_FOVEATION_LEVEL_NONE_FB, XR_FOVEATION_LEVEL_LOW_FB,
			XR_FOVEATION_LEVEL_MEDIUM_FB, XR_FOVEATION_LEVEL_HIGH_FB};
		fov_ext->set_foveation_level(levels[p_foveation_level]);
	}
}

bool OpenXRAPI::get_foveation_dynamic() const
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr && fov_ext->is_enabled()) {
		return fov_ext->get_foveation_dynamic() == XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB;
	}
	return false;
}

void OpenXRAPI::set_foveation_dynamic(bool p_foveation_dynamic)
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr && fov_ext->is_enabled()) {
		fov_ext->set_foveation_dynamic(p_foveation_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB
														   : XR_FOVEATION_DYNAMIC_DISABLED_FB);
	}
}

bool OpenXRAPI::get_foveation_with_subsampled_images() const
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr) {
		return fov_ext->is_foveation_with_subsampled_images_enabled();
	}
	return false;
}

void OpenXRAPI::set_foveation_with_subsampled_images(bool p_enabled)
{
	OpenXRFBFoveationExtension* fov_ext = OpenXRFBFoveationExtension::get_singleton();
	if (fov_ext != nullptr) {
		fov_ext->set_foveation_with_subsampled_images_enabled(p_enabled);
	}
}

PackedInt64Array OpenXRAPI::get_supported_swapchain_formats()
{
	return supported_swapchain_formats;
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

String OpenXRAPI::get_xr_path_name(const XrPath& p_path)
{
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, String());

	uint32_t size = 0;
	char path_name[XR_MAX_PATH_LENGTH];

	XrResult result = xrPathToString(instance, p_path, XR_MAX_PATH_LENGTH, &size, path_name);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			String(), "OpenXR: failed to get name for a path! [" + get_error_string(result) + "]");
	}

	return String(path_name);
}

RID OpenXRAPI::get_tracker_rid(XrPath p_path)
{
	for (const RID& tracker_rid : tracker_owner.get_owned_list()) {
		Tracker* tracker = tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->toplevel_path == p_path) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_tracker(const String& p_name)
{
	for (const RID& tracker_rid : tracker_owner.get_owned_list()) {
		Tracker* tracker = tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->name == p_name) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::tracker_create(const String& p_name)
{
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, RID());

	Tracker new_tracker;
	new_tracker.name = p_name;
	new_tracker.toplevel_path = XR_NULL_PATH;
	new_tracker.active_profile_rid = RID();

	new_tracker.toplevel_path = get_xr_path(p_name);
	ERR_FAIL_COND_V(new_tracker.toplevel_path == XR_NULL_PATH, RID());

	return tracker_owner.make_rid(new_tracker);
}

String OpenXRAPI::tracker_get_name(RID p_tracker)
{
	if (p_tracker.is_null()) {
		return String("None");
	}

	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, String());

	return tracker->name;
}

void OpenXRAPI::tracker_check_profile(RID p_tracker, XrSession p_session)
{
	if (p_session == XR_NULL_HANDLE) {
		p_session = session;
	}

	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
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

		if (xr_interface) {
			xr_interface->tracker_profile_changed(p_tracker, tracker->active_profile_rid);
		}
	}
}

void OpenXRAPI::tracker_free(RID p_tracker)
{
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL(tracker);

	// there is nothing to free here

	tracker_owner.free(p_tracker);
}

RID OpenXRAPI::action_set_create(
	const String& p_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, RID());
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

	XrResult result = xrCreateActionSet(instance, &action_set_info, &action_set.handle);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: failed to create action set ", p_name, "! [", get_error_string(result), "]");
		return RID();
	}

	set_object_name(XR_OBJECT_TYPE_ACTION_SET, uint64_t(action_set.handle), p_name);

	return action_set_owner.make_rid(action_set);
}

RID OpenXRAPI::find_action_set(const String& p_name)
{
	for (const RID& action_set_rid : action_set_owner.get_owned_list()) {
		ActionSet* action_set = action_set_owner.get_or_null(action_set_rid);
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

	ActionSet* action_set = action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, String());

	return action_set->name;
}

XrActionSet OpenXRAPI::action_set_get_handle(RID p_action_set)
{
	if (p_action_set.is_null()) {
		return XR_NULL_HANDLE;
	}

	ActionSet* action_set = action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, XR_NULL_HANDLE);

	return action_set->handle;
}

bool OpenXRAPI::attach_action_sets(const Vector<RID>& p_action_sets)
{
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	Vector<XrActionSet> action_handles;
	action_handles.resize(p_action_sets.size());
	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = action_set_owner.get_or_null(p_action_sets[i]);
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

	XrResult result = xrAttachSessionActionSets(session, &attach_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to attach action sets! [", get_error_string(result), "]");
		return false;
	}

	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = action_set_owner.get_or_null(p_action_sets[i]);
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
	ActionSet* action_set = action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL(action_set);

	if (action_set->handle != XR_NULL_HANDLE) {
		xrDestroyActionSet(action_set->handle);
	}

	action_set_owner.free(p_action_set);
}

RID OpenXRAPI::get_action_rid(XrAction p_action)
{
	for (const RID& action_rid : action_owner.get_owned_list()) {
		Action* action = action_owner.get_or_null(action_rid);
		if (action && action->handle == p_action) {
			return action_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_action(const String& p_name, const RID& p_action_set)
{
	for (const RID& action_rid : action_owner.get_owned_list()) {
		Action* action = action_owner.get_or_null(action_rid);
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
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, RID());

	Action action;
	action.name = p_name;

	ActionSet* action_set = action_set_owner.get_or_null(p_action_set);
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
		Tracker* tracker = tracker_owner.get_or_null(p_trackers[i]);
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

	return action_owner.make_rid(action);
}

String OpenXRAPI::action_get_name(RID p_action)
{
	if (p_action.is_null()) {
		return String("None");
	}

	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, String());

	return action->name;
}

XrAction OpenXRAPI::action_get_handle(RID p_action)
{
	if (p_action.is_null()) {
		return XR_NULL_HANDLE;
	}

	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XR_NULL_HANDLE);

	return action->handle;
}

void OpenXRAPI::action_free(RID p_action)
{
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL(action);

	if (action->handle != XR_NULL_HANDLE) {
		xrDestroyAction(action->handle);
	}

	action_owner.free(p_action);
}

RID OpenXRAPI::get_interaction_profile_rid(XrPath p_path)
{
	for (const RID& ip_rid : interaction_profile_owner.get_owned_list()) {
		InteractionProfile* ip = interaction_profile_owner.get_or_null(ip_rid);
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

	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
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
		instance, new_interaction_profile.internal_name.get_data(), &new_interaction_profile.path);
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
	return interaction_profile_owner.make_rid(new_interaction_profile);
}

String OpenXRAPI::interaction_profile_get_name(RID p_interaction_profile)
{
	if (p_interaction_profile.is_null()) {
		return String("None");
	}

	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, String());

	return ip->name;
}

void OpenXRAPI::interaction_profile_clear_bindings(RID p_interaction_profile)
{
	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
}

CharString OpenXRAPI::get_interaction_profile_internal_name(
	const String& p_interaction_profile_name) const
{
	CharString internal_name = p_interaction_profile_name.utf8();

	if (openxr_version < XR_API_VERSION_1_1_0) {
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
	const CharString& p_interaction_profile_name, const char* p_path) const
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

		if (rename.before_version != 0 && openxr_version >= rename.before_version) {
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
	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, -1);

	if (!interaction_profile_supports_io_path(ip->name, p_path)) {
		return -1;
	}

	XrActionSuggestedBinding binding;

	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_COND_V(action == nullptr || action->handle == XR_NULL_HANDLE, -1);

	binding.action = action->handle;

	XrResult result = xrStringToPath(instance,
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
	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, false);

	if (!p_modifier.is_empty()) {
		// Add it to our stack.
		ip->modifiers.push_back(p_modifier);
	}

	return true;
}

bool OpenXRAPI::interaction_profile_suggest_bindings(RID p_interaction_profile)
{
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
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

	XrResult result = xrSuggestInteractionProfileBindings(instance, &suggested_bindings);
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
	InteractionProfile* ip = interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
	ip->modifiers.clear();

	interaction_profile_owner.free(p_interaction_profile);
}

bool OpenXRAPI::get_action_bool(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!running) {
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
	XrResult result = xrGetActionStateBoolean(session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action boolean! [", get_error_string(result), "]");
		return false;
	}

	return result_state.isActive && result_state.currentState;
}

float OpenXRAPI::get_action_float(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, 0.0);
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, 0.0);
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, 0.0);

	if (!running) {
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
	XrResult result = xrGetActionStateFloat(session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action float! [", get_error_string(result), "]");
		return 0.0;
	}

	return result_state.isActive ? result_state.currentState : 0.0;
}

Vector2 OpenXRAPI::get_action_vector2(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, Vector2());
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, Vector2());
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, Vector2());

	if (!running) {
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
	XrResult result = xrGetActionStateVector2f(session, &get_info, &result_state);
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
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, XRPose::XR_TRACKING_CONFIDENCE_NONE);

	if (!running) {
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
		XrResult result = xrCreateActionSpace(session, &action_space_info, &space);
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
		xrLocateSpace(action->trackers[index].space, play_space, display_time, &location);
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
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);
	Action* action = action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!running) {
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
		xrApplyHapticFeedback(session, &action_info, (const XrHapticBaseHeader*)&vibration);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to apply haptic feedback! [", get_error_string(result), "]");
		return false;
	}

	return true;
}

const Vector<XrEnvironmentBlendMode> OpenXRAPI::get_supported_environment_blend_modes()
{
	return supported_environment_blend_modes;
}

bool OpenXRAPI::is_environment_blend_mode_supported(XrEnvironmentBlendMode p_blend_mode) const
{
	return supported_environment_blend_modes.has(p_blend_mode);
}

bool OpenXRAPI::set_environment_blend_mode(XrEnvironmentBlendMode p_blend_mode)
{
	if (emulate_environment_blend_mode_alpha_blend &&
		p_blend_mode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) {
		requested_environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
		environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		set_render_environment_blend_mode(environment_blend_mode);
		return true;
	}
	// We allow setting this when not initialized and will check if it is supported when
	// initializing. After OpenXR is initialized we verify we're setting a supported blend mode.
	else if (!is_initialized() || is_environment_blend_mode_supported(p_blend_mode)) {
		requested_environment_blend_mode = p_blend_mode;
		environment_blend_mode = p_blend_mode;
		set_render_environment_blend_mode(environment_blend_mode);
		return true;
	}
	return false;
}

void OpenXRAPI::set_emulate_environment_blend_mode_alpha_blend(bool p_enabled)
{
	emulate_environment_blend_mode_alpha_blend = p_enabled;
}

OpenXRAPI::OpenXRAlphaBlendModeSupport OpenXRAPI::is_environment_blend_mode_alpha_blend_supported()
{
	if (emulate_environment_blend_mode_alpha_blend) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING;
	}
	else if (is_environment_blend_mode_supported(XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND)) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL;
	}
	return OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE;
}


