/**************************************************************************/
/*  openxr_extensions.cpp                                                 */
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

#include "../action_map/openxr_action_map.h"
#include "../openxr_api.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "openxr_extensions.h"
#include "platform/android/api/java_class_wrapper.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/texture_storage.h"
#include "servers/xr/xr_server.h"

#ifdef XR_USE_PLATFORM_ANDROID
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXREyeGazeInteractionExtension

void OpenXREyeGazeInteractionExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXREyeGazeInteractionExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXREyeGazeInteractionExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/eye_gaze_interaction") &&
		(!OS::has_feature("mobile") ||
			OS::has_feature(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME))) {
		request_extensions[XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME] = &data->available;
	}

	return request_extensions;
}

void* OpenXREyeGazeInteractionExtension::set_system_properties_and_get_next_pointer(
	void* p_next_pointer)
{
	if (!data->available) {
		return p_next_pointer;
	}

	data->properties.type = XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT;
	data->properties.next = p_next_pointer;
	data->properties.supportsEyeGazeInteraction = false;

	return &data->properties;
}

PackedStringArray OpenXREyeGazeInteractionExtension::get_suggested_tracker_names()
{
	PackedStringArray arr = {"/user/eyes_ext"};
	return arr;
}

bool OpenXREyeGazeInteractionExtension::is_available() { return data != nullptr && data->available; }

bool OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction()
{
	if (data->available && data->properties.supportsEyeGazeInteraction) {
		return !OS::has_feature("mobile") ||
			   OS::has_feature("PERMISSION_XR_EXT_eye_gaze_interaction");
	}

	return false;
}

bool OpenXREyeGazeInteractionExtension::get_eye_gaze_pose(double p_dist, Vector3& r_eye_pose)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	if (!data->init_eye_gaze_pose) {
		data->init_eye_gaze_pose = true;

		data->eye_tracker = OpenXRAPI::find_tracker("/user/eyes_ext");
		if (data->eye_tracker.is_null()) {
			WARN_PRINT("Couldn't obtain eye tracker");
		}

		data->eye_action = OpenXRAPI::find_action("eye_gaze_pose");
		if (data->eye_action.is_null()) {
			WARN_PRINT("Couldn't obtain pose action for `eye_gaze_pose`, make sure to add this to "
					   "your action map.");
		}
	}

	if (data->eye_tracker.is_null() || data->eye_action.is_null()) {
		return false;
	}

	Transform3D eye_transform;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	XRPose::TrackingConfidence confidence = OpenXRAPI::get_action_pose(
		data->eye_action, data->eye_tracker, eye_transform, linear_velocity, angular_velocity);
	if (confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
		return false;
	}

	r_eye_pose = eye_transform.origin + eye_transform.basis[2] * p_dist;

	return true;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRUserPresenceExtension

void OpenXRUserPresenceExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRUserPresenceExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void* OpenXRUserPresenceExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
	if (!data->available) {
		return p_next_pointer;
	}

	data->properties.type = XR_TYPE_SYSTEM_USER_PRESENCE_PROPERTIES_EXT;
	data->properties.next = p_next_pointer;
	data->properties.supportsUserPresence = false;

	return &data->properties;
}

bool OpenXRUserPresenceExtension::is_active()
{
	return data->available && data->properties.supportsUserPresence;
}

void OpenXRUserPresenceExtension::on_state_ready() { data->user_present = true; }

void OpenXRUserPresenceExtension::on_state_stopping() { data->user_present = false; }

bool OpenXRUserPresenceExtension::is_user_present()
{
	return data != nullptr && data->user_present;
}

HashMap<String, bool*> OpenXRUserPresenceExtension::get_requested_extensions(XrVersion)
{
	return HashMap<String, bool*>();
}

bool OpenXRUserPresenceExtension::on_event_polled(const XrEventDataBuffer&) { return false; }

////////////////////////////////////////////////////////////////////////////
// OpenXRAndroidThreadSettingsExtension

void OpenXRAndroidThreadSettingsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRAndroidThreadSettingsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRAndroidThreadSettingsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

#ifdef XR_USE_PLATFORM_ANDROID
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = &data->available;
#endif

	return request_extensions;
}

void OpenXRAndroidThreadSettingsExtension::on_instance_created(XrInstance)
{
	if (!data->available) {
		return;
	}

	if (!_initialize_openxr_android_thread_settings_extension()) {
		print_error("OpenXR: Failed to initialize android thread settings extension");
		data->available = false;
	}
}

bool OpenXRAndroidThreadSettingsExtension::set_application_thread_type(
	ThreadType p_thread_type, uint32_t p_thread_id)
{
	if (!data->available) {
		return false;
	}

#ifdef XR_USE_PLATFORM_ANDROID
	XrAndroidThreadTypeKHR thread_type{};
	switch (p_thread_type) {
	case THREAD_TYPE_APPLICATION_MAIN:
		thread_type = XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR;
		break;
	case THREAD_TYPE_APPLICATION_WORKER:
		thread_type = XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR;
		break;
	case THREAD_TYPE_RENDERER_MAIN:
		thread_type = XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR;
		break;
	case THREAD_TYPE_RENDERER_WORKER:
		thread_type = XR_ANDROID_THREAD_TYPE_RENDERER_WORKER_KHR;
		break;
	default:
		print_error(
			vformat("OpenXR: Failed to set android application thread; invalid thread type %d",
				p_thread_type));
		return false;
	}

	XrResult result = xrSetAndroidApplicationThreadKHR(
		OpenXRAPI::get_session(), thread_type, p_thread_id == 0 ? gettid() : p_thread_id);
	if (result != XR_SUCCESS) {
		print_error(vformat("OpenXR: Failed to set android application thread; %s",
			OpenXRAPI::get_error_string(result)));
		return false;
	}
#endif

	return true;
}

bool OpenXRAndroidThreadSettingsExtension::_initialize_openxr_android_thread_settings_extension()
{
#ifdef XR_USE_PLATFORM_ANDROID
	EXT_INIT_XR_FUNC_V(xrSetAndroidApplicationThreadKHR);
#endif
	return true;
}

void OpenXRAndroidThreadSettingsExtension::_set_render_thread_type()
{
	if (Thread::is_main_thread()) {
		return;
	}

	set_application_thread_type(THREAD_TYPE_RENDERER_MAIN);
}

void OpenXRAndroidThreadSettingsExtension::on_session_created(XrSession) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRDisplayRefreshRateExtension

void OpenXRDisplayRefreshRateExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRDisplayRefreshRateExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRDisplayRefreshRateExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME] = &data->display_refresh_rate_ext;

	return request_extensions;
}

void OpenXRDisplayRefreshRateExtension::on_instance_created(const XrInstance)
{
	if (data->display_refresh_rate_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateDisplayRefreshRatesFB);
		EXT_INIT_XR_FUNC(xrGetDisplayRefreshRateFB);
		EXT_INIT_XR_FUNC(xrRequestDisplayRefreshRateFB);
	}
}

void OpenXRDisplayRefreshRateExtension::on_instance_destroyed()
{
	data->display_refresh_rate_ext = false;
}

bool OpenXRDisplayRefreshRateExtension::on_event_polled(const XrEventDataBuffer& event)
{
	switch (event.type) {
	case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB: {
		const XrEventDataDisplayRefreshRateChangedFB* event_fb =
			(const XrEventDataDisplayRefreshRateChangedFB*)&event;

		OpenXRInterface* xr_interface = OpenXRAPI::get_xr_interface();
		if (xr_interface) {
			xr_interface->on_refresh_rate_changes(event_fb->toDisplayRefreshRate);
		}

		return true;
	} break;
	default:
		return false;
	}
}

void OpenXRDisplayRefreshRateExtension::set_refresh_rate(float) {}

float OpenXRDisplayRefreshRateExtension::get_refresh_rate() { return 0.0f; }

////////////////////////////////////////////////////////////////////////////
// OpenXRPerformanceSettingsExtension

void OpenXRPerformanceSettingsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRPerformanceSettingsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRPerformanceSettingsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME] = &data->available;

	return request_extensions;
}

void OpenXRPerformanceSettingsExtension::on_instance_created(const XrInstance)
{
	if (data->available) {
		EXT_INIT_XR_FUNC(xrPerfSettingsSetPerformanceLevelEXT);
	}
}

bool OpenXRPerformanceSettingsExtension::on_event_polled(const XrEventDataBuffer& event)
{
	switch (event.type) {
	case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
		const XrEventDataPerfSettingsEXT* perf_settings_event =
			(const XrEventDataPerfSettingsEXT*)&event;

		OpenXRInterface::PerfSettingsSubDomain sub_domain =
			openxr_to_sub_domain(perf_settings_event->subDomain);
		OpenXRInterface::PerfSettingsNotificationLevel from_level =
			openxr_to_notification_level(perf_settings_event->fromLevel);
		OpenXRInterface::PerfSettingsNotificationLevel to_level =
			openxr_to_notification_level(perf_settings_event->toLevel);

		OpenXRInterface* openxr_interface = OpenXRAPI::get_xr_interface();

		if (perf_settings_event->domain == XR_PERF_SETTINGS_DOMAIN_CPU_EXT) {
			openxr_interface->on_cpu_level_changed(sub_domain, from_level, to_level);
		}
		else if (perf_settings_event->domain == XR_PERF_SETTINGS_DOMAIN_GPU_EXT) {
			openxr_interface->on_gpu_level_changed(sub_domain, from_level, to_level);
		}
		else {
			print_error(vformat(
				"OpenXR: no matching performance settings domain for %s", perf_settings_event->domain));
		}
		return true;
	} break;
	default:
		return false;
	}
}

bool OpenXRPerformanceSettingsExtension::is_available() { return data != nullptr && data->available; }

void OpenXRPerformanceSettingsExtension::set_cpu_level(OpenXRInterface::PerfSettingsLevel p_level)
{
	XrSession session = OpenXRAPI::get_session();
	XrPerfSettingsLevelEXT xr_level = level_to_openxr(p_level);

	XrResult result =
		xrPerfSettingsSetPerformanceLevelEXT(session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, xr_level);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: failed to set CPU performance level [%s]",
			OpenXRAPI::get_error_string(result)));
	}
}

void OpenXRPerformanceSettingsExtension::set_gpu_level(OpenXRInterface::PerfSettingsLevel p_level)
{
	XrSession session = OpenXRAPI::get_session();
	XrPerfSettingsLevelEXT xr_level = level_to_openxr(p_level);

	XrResult result =
		xrPerfSettingsSetPerformanceLevelEXT(session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, xr_level);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: failed to set GPU performance level [%s]",
			OpenXRAPI::get_error_string(result)));
	}
}

XrPerfSettingsLevelEXT OpenXRPerformanceSettingsExtension::level_to_openxr(
	OpenXRInterface::PerfSettingsLevel p_level)
{
	switch (p_level) {
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_POWER_SAVINGS:
		return XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_SUSTAINED_LOW:
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_SUSTAINED_HIGH:
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_BOOST:
		return XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
	default:
		print_error("Invalid performance settings level.");
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	}
}

OpenXRInterface::PerfSettingsSubDomain OpenXRPerformanceSettingsExtension::openxr_to_sub_domain(
	XrPerfSettingsSubDomainEXT p_sub_domain)
{
	switch (p_sub_domain) {
	case XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_COMPOSITING;
	case XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_RENDERING;
	case XR_PERF_SETTINGS_SUB_DOMAIN_THERMAL_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_THERMAL;
	default:
		print_error("Invalid performance settings sub domain.");
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_COMPOSITING;
	}
}

OpenXRInterface::PerfSettingsNotificationLevel
OpenXRPerformanceSettingsExtension::openxr_to_notification_level(
	XrPerfSettingsNotificationLevelEXT p_notification_level)
{
	switch (p_notification_level) {
	case XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_NORMAL;
	case XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_WARNING;
	case XR_PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED;
	default:
		print_error("Invalid performance settings notification level.");
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_NORMAL;
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRDebugUtilsExtension

void OpenXRDebugUtilsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRDebugUtilsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRDebugUtilsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_DEBUG_UTILS_EXTENSION_NAME] = &data->debug_utils_ext;

	return request_extensions;
}

void OpenXRDebugUtilsExtension::on_instance_destroyed()
{
	if (data->default_messenger != XR_NULL_HANDLE) {
		XrResult result = xrDestroyDebugUtilsMessengerEXT(data->default_messenger);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to destroy debug callback [" +
					  OpenXRAPI::get_error_string(result) + "]");
		}

		data->default_messenger = XR_NULL_HANDLE;
	}

	xrCreateDebugUtilsMessengerEXT_ptr = nullptr;
	xrDestroyDebugUtilsMessengerEXT_ptr = nullptr;
	xrSetDebugUtilsObjectNameEXT_ptr = nullptr;
	xrSessionBeginDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionEndDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionInsertDebugUtilsLabelEXT_ptr = nullptr;
	data->debug_utils_ext = false;
}

bool OpenXRDebugUtilsExtension::get_active() { return data != nullptr && data->debug_utils_ext; }

void OpenXRDebugUtilsExtension::set_object_name(
	XrObjectType p_object_type, uint64_t p_object_handle, const char* p_object_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSetDebugUtilsObjectNameEXT_ptr);

	const XrDebugUtilsObjectNameInfoEXT space_name_info = {
		XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		p_object_type,
		p_object_handle,
		p_object_name,
	};

	XrResult result = xrSetDebugUtilsObjectNameEXT_ptr(OpenXRAPI::get_instance(), &space_name_info);
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to set object name [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::begin_debug_label_region(const char* p_label_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionBeginDebugUtilsLabelRegionEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		p_label_name,
	};

	XrResult result = xrSessionBeginDebugUtilsLabelRegionEXT_ptr(
		OpenXRAPI::get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to begin label region [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::end_debug_label_region()
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionEndDebugUtilsLabelRegionEXT_ptr);

	XrResult result = xrSessionEndDebugUtilsLabelRegionEXT_ptr(OpenXRAPI::get_session());
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to end label region [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::insert_debug_label(const char* p_label_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionInsertDebugUtilsLabelEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		p_label_name,
	};

	XrResult result = xrSessionInsertDebugUtilsLabelEXT_ptr(
		OpenXRAPI::get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to insert label [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

XrBool32 XRAPI_PTR OpenXRDebugUtilsExtension::_debug_callback(
	XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
	XrDebugUtilsMessageTypeFlagsEXT p_message_types,
	const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data)
{
	if (OpenXRDebugUtilsExtension::is_initialized()) {
		return OpenXRDebugUtilsExtension::debug_callback(
			p_message_severity, p_message_types, p_callback_data, p_user_data);
	}

	return XR_FALSE;
}

XrBool32 OpenXRDebugUtilsExtension::debug_callback(
	XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
	XrDebugUtilsMessageTypeFlagsEXT p_message_types,
	const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void*)
{
	String msg;

	ERR_FAIL_NULL_V(p_callback_data, XR_FALSE);

	if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		msg = ", type: General";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		msg = ", type: Validation";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		msg = ", type: Performance";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT) {
		msg = ", type: Conformance";
	}
	else {
		msg = ", type: Unknown (" + String::num_uint64(p_message_types) + ")";
	}

	if (p_callback_data->functionName) {
		msg += ", function Name: " + String(p_callback_data->functionName);
	}
	if (p_callback_data->messageId) {
		msg += "\nMessage ID: " + String(p_callback_data->messageId);
	}
	if (p_callback_data->message) {
		msg += "\nMessage: " + String(p_callback_data->message);
	}

	if (p_callback_data->objectCount > 0) {
		String objects;

		for (uint32_t i = 0; i < p_callback_data->objectCount; i++) {
			if (!objects.is_empty()) {
				objects += ", ";
			}
			objects += p_callback_data->objects[i].objectName;
		}

		msg += "\nObjects: " + objects;
	}

	if (p_callback_data->sessionLabelCount > 0) {
		String labels;

		for (uint32_t i = 0; i < p_callback_data->sessionLabelCount; i++) {
			if (!labels.is_empty()) {
				labels += ", ";
			}
			labels += p_callback_data->sessionLabels[i].labelName;
		}

		msg += "\nLabels: " + labels;
	}

	if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		ERR_PRINT("OpenXR: Severity: Error" + msg);
	}
	else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		WARN_PRINT("OpenXR: Severity: Warning" + msg);
	}
	else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		print_verbose("OpenXR: Severity: Verbose" + msg);
	}

	return XR_FALSE;
}

void OpenXRDebugUtilsExtension::on_instance_created(const XrInstance) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMaskExtension

static const char* VISIBILITY_MASK_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode unshaded, shadows_disabled, cull_disabled;\n"
	"void vertex() {\n"
	"\tif (int(VERTEX.z) == VIEW_INDEX) {\n"
	"\t\tVERTEX.z = -1.0;\n"
	"\t\tVERTEX += EYE_OFFSET;\n"
	"\t\tPOSITION = PROJECTION_MATRIX * vec4(VERTEX, 1.0);\n"
	"\t\tPOSITION.xy /= POSITION.w;\n"
	"\t\tPOSITION.z = 1.0;\n"
	"\t\tPOSITION.w = 1.0;\n"
	"\t} else {\n"
	"\t\tPOSITION = vec4(2.0, 2.0, 2.0, 1.0);\n"
	"\t}\n"
	"}\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

void OpenXRVisibilityMaskExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRVisibilityMaskExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRVisibilityMaskExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_VISIBILITY_MASK_EXTENSION_NAME] = &data->available;

	return request_extensions;
}

void OpenXRVisibilityMaskExtension::on_instance_created(const XrInstance)
{
	if (data->available) {
		EXT_INIT_XR_FUNC(xrGetVisibilityMaskKHR);
	}
}

void OpenXRVisibilityMaskExtension::on_session_created(const XrSession)
{
	if (data->available) {
		ERR_FAIL_NULL(Renderer::data);
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		data->shader = Renderer::shader_create();
		Renderer::shader_set_code(data->shader, VISIBILITY_MASK_SHADER_CODE);

		data->material = Renderer::material_create();
		Renderer::material_set_shader(data->material, data->shader);
		Renderer::material_set_render_priority(data->material, 99);

		data->mesh_count = OpenXRAPI::get_view_count();
		for (uint32_t i = 0; i < data->mesh_count; i++) {
			_update_mesh_data(i);
		}

		_update_mesh();
	}
}

void OpenXRVisibilityMaskExtension::on_session_destroyed()
{
	ERR_FAIL_NULL(Renderer::data);

	if (data->mesh.is_valid()) {
		Renderer::free_rid(data->mesh);
		data->mesh = RID();
	}

	if (data->material.is_valid()) {
		Renderer::free_rid(data->material);
		data->material = RID();
	}

	if (data->shader.is_valid()) {
		Renderer::free_rid(data->shader);
		data->shader = RID();
	}

	data->mesh_count = 0;
}

void OpenXRVisibilityMaskExtension::on_pre_render() { _update_mesh(); }

bool OpenXRVisibilityMaskExtension::on_event_polled(const XrEventDataBuffer& event)
{
	if (event.type == XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
		const XrEventDataVisibilityMaskChangedKHR* vismask_event =
			(const XrEventDataVisibilityMaskChangedKHR*)&event;

		print_verbose("OpenXR EVENT: Visibility mask changed for view " +
					  String::num_uint64(vismask_event->viewIndex));

		if (data->available) {
			_update_mesh_data(vismask_event->viewIndex);
		}

		return true;
	}

	return false;
}

bool OpenXRVisibilityMaskExtension::is_available() { return data != nullptr && data->available; }

RID OpenXRVisibilityMaskExtension::get_mesh() { return data ? data->mesh : RID(); }

void OpenXRVisibilityMaskExtension::_update_mesh_data(uint32_t p_view)
{
	if (data->available) {
		ERR_FAIL_UNSIGNED_INDEX(p_view, 4);
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrSession session = OpenXRAPI::get_session();
		XrViewConfigurationType view_configuration_type = OpenXRAPI::get_view_configuration();

		XrVisibilityMaskKHR visibility_mask_data = {
			XR_TYPE_VISIBILITY_MASK_KHR,
			nullptr,
			0,
			0,
			nullptr,
			0,
			0,
			nullptr,
		};

		XrResult result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			return;
		}

		data->mesh_data[p_view].vertices.resize(visibility_mask_data.vertexCountOutput);
		data->mesh_data[p_view].indices.resize(visibility_mask_data.indexCountOutput);

		visibility_mask_data.vertexCapacityInput = visibility_mask_data.vertexCountOutput;
		visibility_mask_data.vertices = data->mesh_data[p_view].vertices.ptrw();
		visibility_mask_data.indexCapacityInput = visibility_mask_data.indexCountOutput;
		visibility_mask_data.indices = data->mesh_data[p_view].indices.ptrw();

		result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			return;
		}

		data->is_dirty = true;
	}
}

void OpenXRVisibilityMaskExtension::_update_mesh() {}


////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayerExtension

void OpenXRCompositionLayerExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRCompositionLayerExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRCompositionLayerExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME] =
		&data->cylinder_ext_available;
	request_extensions[XR_KHR_COMPOSITION_LAYER_EQUIRECT2_EXTENSION_NAME] =
		&data->equirect_ext_available;

#ifdef ANDROID_ENABLED
	request_extensions[XR_KHR_ANDROID_SURFACE_SWAPCHAIN_EXTENSION_NAME] =
		&data->android_surface_ext_available;
#endif

	return request_extensions;
}

void OpenXRCompositionLayerExtension::on_instance_created(const XrInstance p_instance)
{
#ifdef ANDROID_ENABLED
	EXT_INIT_XR_FUNC(xrDestroySwapchain);
	if (data->android_surface_ext_available) {
		EXT_INIT_XR_FUNC(xrCreateSwapchainAndroidSurfaceKHR);
	}
#endif
}

void OpenXRCompositionLayerExtension::on_pre_render()
{
	for (CompositionLayer* composition_layer : data->registered_composition_layers) {
		composition_layer->on_pre_render();
	}
}

int OpenXRCompositionLayerExtension::get_composition_layer_count()
{
	return data->registered_composition_layers.size();
}

XrCompositionLayerBaseHeader* OpenXRCompositionLayerExtension::get_composition_layer(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(
		(unsigned int)p_index, data->registered_composition_layers.size(), nullptr);
	return data->registered_composition_layers[p_index]->get_composition_layer();
}

int OpenXRCompositionLayerExtension::get_composition_layer_order(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V((unsigned int)p_index, data->registered_composition_layers.size(), 1);
	return data->registered_composition_layers[p_index]->sort_order;
}

RID OpenXRCompositionLayerExtension::composition_layer_create(
	XrCompositionLayerBaseHeader* p_openxr_layer)
{
	RID rid = data->composition_layer_owner.make_rid();
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(rid);

	switch (p_openxr_layer->type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		layer->composition_layer_quad = *(XrCompositionLayerQuad*)p_openxr_layer;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		layer->composition_layer_cylinder = *(XrCompositionLayerCylinderKHR*)p_openxr_layer;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		layer->composition_layer_equirect = *(XrCompositionLayerEquirect2KHR*)p_openxr_layer;
	} break;
	default: {
		ERR_PRINT(vformat("Invalid OpenXR composition layer type: %s", p_openxr_layer->type));
		data->composition_layer_owner.free(rid);
		return RID();
	}
	}

	return rid;
}

Ref<JavaObject> OpenXRCompositionLayerExtension::composition_layer_get_android_surface(RID p_layer)
{
	MutexLock lock(data->composition_layer_mutex);
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL_V(layer, Ref<JavaObject>());
	return layer->get_android_surface();
}

void OpenXRCompositionLayerExtension::_composition_layer_register_rt(RID p_layer)
{
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL(layer);
	data->registered_composition_layers.push_back(layer);
}

void OpenXRCompositionLayerExtension::_composition_layer_unregister_rt(RID p_layer)
{
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL(layer);
	data->registered_composition_layers.erase(layer);
}

bool OpenXRCompositionLayerExtension::is_available(XrStructureType p_which)
{
	switch (p_which) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		return true;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		return data != nullptr && data->cylinder_ext_available;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		return data != nullptr && data->equirect_ext_available;
	} break;
	default: {
		ERR_PRINT(vformat("Unsupported composition layer type: %s", p_which));
		return false;
	}
	}
}

#ifdef ANDROID_ENABLED
bool OpenXRCompositionLayerExtension::create_android_surface_swapchain(
	XrSwapchainCreateInfo* p_info, XrSwapchain* r_swapchain, jobject* r_surface)
{
	if (data->android_surface_ext_available) {
		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		XrResult result = xrCreateSwapchainAndroidSurfaceKHR(
			OpenXRAPI::get_session(), p_info, r_swapchain, r_surface);
		if (XR_FAILED(result)) {
			print_line("OpenXR: Failed to create Android surface swapchain [",
				OpenXRAPI::get_error_string(result), "]");
			return false;
		}

		return true;
	}

	return false;
}
#endif

void OpenXRCompositionLayerExtension::CompositionLayer::set_viewport(
	RID p_viewport, const Size2i& p_size)
{
	ERR_FAIL_COND(use_android_surface);

	if (subviewport.viewport != p_viewport) {
		if (subviewport.viewport.is_valid()) {
			RID rt = Renderer::viewport_get_render_target(subviewport.viewport);
			RendererTextureStorage::render_target_set_override(rt, RID(), RID(), RID(), RID());
		}

		subviewport.viewport = p_viewport;

		if (subviewport.viewport.is_valid()) {
			subviewport.viewport_size = p_size;
		}
		else {
			free_swapchain();
			subviewport.viewport_size = Size2i();
		}
	}
	else if (subviewport.viewport_size != p_size) {
		subviewport.viewport_size = p_size;
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_use_android_surface(
	bool p_use_android_surface, const Size2i& p_size)
{
#ifdef ANDROID_ENABLED
	if (p_use_android_surface == use_android_surface) {
		if (use_android_surface && swapchain_size != p_size) {
			if (OpenXRFBUpdateSwapchainExtension::is_initialized() &&
				OpenXRFBUpdateSwapchainExtension::is_android_ext_enabled()) {
				swapchain_size = p_size;
				OpenXRFBUpdateSwapchainExtension::update_swapchain_surface_size(
					android_surface.swapchain, swapchain_size);
			}
		}
		return;
	}

	use_android_surface = p_use_android_surface;

	if (use_android_surface) {
		if (!OpenXRCompositionLayerExtension::is_android_surface_swapchain_available()) {
			ERR_PRINT_ONCE("OpenXR: Cannot use Android surface for composition layer because the "
						   "extension isn't available");
		}

		if (subviewport.viewport.is_valid()) {
			set_viewport(RID(), Size2i());
		}

		swapchain_size = p_size;
	}
	else {
		free_swapchain();
	}
#endif
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_alpha_blend(bool p_alpha_blend)
{
	if (alpha_blend != p_alpha_blend) {
		alpha_blend = p_alpha_blend;
		if (alpha_blend) {
			composition_layer.layerFlags |= XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		}
		else {
			composition_layer.layerFlags &= ~XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_transform(
	const Transform3D& p_transform)
{
	Transform3D xf;

	if (pose_space == POSE_HEAD_LOCKED) {
		xf = p_transform;
	}
	else {
		Transform3D reference_frame = XRServer::get_reference_frame();
		xf = reference_frame.inverse() * p_transform;
	}

	Quaternion quat(xf.basis.orthonormalized());

	if (Math::is_zero_approx(quat.length())) {
		quat = Quaternion();
	}

	XrPosef pose = {{(float)quat.x, (float)quat.y, (float)quat.z, (float)quat.w},
		{(float)xf.origin.x, (float)xf.origin.y, (float)xf.origin.z}};

	switch (composition_layer.type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		composition_layer_quad.pose = pose;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		composition_layer_cylinder.pose = pose;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		composition_layer_equirect.pose = pose;
	} break;
	default: {
		ERR_PRINT(vformat("Cannot set transform on unsupported composition layer type: %s",
			composition_layer.type));
	}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_min_filter(Filter p_mode)
{
	swapchain_state.min_filter = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_mag_filter(Filter p_mode)
{
	swapchain_state.mag_filter = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_mipmap_mode(MipmapMode p_mode)
{
	swapchain_state.mipmap_mode = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_horizontal_wrap(Wrap p_mode)
{
	swapchain_state.horizontal_wrap = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_vertical_wrap(Wrap p_mode)
{
	swapchain_state.vertical_wrap = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_red_swizzle(Swizzle p_mode)
{
	swapchain_state.red_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_green_swizzle(Swizzle p_mode)
{
	swapchain_state.green_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_blue_swizzle(Swizzle p_mode)
{
	swapchain_state.blue_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_alpha_swizzle(Swizzle p_mode)
{
	swapchain_state.alpha_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_max_anisotropy(float p_value)
{
	swapchain_state.max_anisotropy = p_value;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_border_color(const Color& p_color)
{
	swapchain_state.border_color = p_color;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_pose_space(PoseSpace p_pose_space)
{
	pose_space = p_pose_space;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_eye_visibility(
	EyeVisibility p_eye_visibility)
{
	XrEyeVisibility eye_visibility;

	switch (p_eye_visibility) {
	case EYE_VISIBILITY_BOTH: {
		eye_visibility = XR_EYE_VISIBILITY_BOTH;
	} break;
	case EYE_VISIBILITY_LEFT: {
		eye_visibility = XR_EYE_VISIBILITY_LEFT;
	} break;
	case EYE_VISIBILITY_RIGHT: {
		eye_visibility = XR_EYE_VISIBILITY_RIGHT;
	} break;
	default: {
		eye_visibility = XR_EYE_VISIBILITY_BOTH;
	}
	}

	switch (composition_layer.type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		composition_layer_quad.eyeVisibility = eye_visibility;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		composition_layer_cylinder.eyeVisibility = eye_visibility;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		composition_layer_equirect.eyeVisibility = eye_visibility;
	} break;
	default: {
		ERR_PRINT(vformat("%s does not support setting eye visibility.", composition_layer.type));
	}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_quad_size(const Size2& p_size)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_QUAD);
	composition_layer_quad.size = {(float)p_size.x, (float)p_size.y};
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_radius(float p_radius)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.radius = p_radius;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_aspect_ratio(
	float p_aspect_ratio)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.aspectRatio = p_aspect_ratio;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_central_angle(
	float p_central_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.centralAngle = p_central_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_radius(float p_radius)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.radius = p_radius;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_central_horizontal_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.centralHorizontalAngle = p_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_upper_vertical_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.upperVerticalAngle = p_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_lower_vertical_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.lowerVerticalAngle = p_angle;
}

Ref<JavaObject> OpenXRCompositionLayerExtension::CompositionLayer::get_android_surface()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		MutexLock lock(OpenXRCompositionLayerExtension::data->composition_layer_mutex);
		if (android_surface.surface.is_null()) {
			create_android_surface();
		}
		return android_surface.surface;
	}
#endif
	return Ref<JavaObject>();
}

void OpenXRCompositionLayerExtension::CompositionLayer::on_pre_render()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		MutexLock lock(OpenXRCompositionLayerExtension::data->composition_layer_mutex);
		if (android_surface.surface.is_null()) {
			create_android_surface();
		}
		return;
	}
#endif

	if (subviewport.viewport.is_valid() && OpenXRAPI::is_allocated() && OpenXRAPI::is_running()) {
		RSE::ViewportUpdateMode update_mode =
			Renderer::viewport_get_update_mode(subviewport.viewport);
		if (update_mode == RSE::VIEWPORT_UPDATE_ONCE ||
			update_mode == RSE::VIEWPORT_UPDATE_ALWAYS) {
			if (update_and_acquire_swapchain(update_mode == RSE::VIEWPORT_UPDATE_ONCE)) {
				RID rt = Renderer::viewport_get_render_target(subviewport.viewport);
				RendererTextureStorage::render_target_set_override(
					rt, get_current_swapchain_texture(), RID(), RID(), RID());
			}
		}
	}

	if (swapchain_state_is_dirty) {
		update_swapchain_state();
		swapchain_state_is_dirty = false;
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::free()
{
	if (use_android_surface) {
		free_swapchain();
	}
	else {
		set_viewport(RID(), Size2i());
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::update_swapchain_state()
{
	if (!OpenXRFBUpdateSwapchainExtension::is_initialized()) {
		return;
	}

#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		if (android_surface.swapchain == XR_NULL_HANDLE) {
			return;
		}

		OpenXRFBUpdateSwapchainExtension::update_swapchain_state(
			android_surface.swapchain, &swapchain_state);
	}
	else
#endif
	{
		if (subviewport.swapchain_info.get_swapchain() == XR_NULL_HANDLE) {
			return;
		}

		OpenXRFBUpdateSwapchainExtension::update_swapchain_state(
			subviewport.swapchain_info.get_swapchain(), &swapchain_state);
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::update_swapchain_sub_image(
	XrSwapchainSubImage& r_subimage)
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		r_subimage.swapchain = android_surface.swapchain;
	}
	else
#endif
	{
		XrSwapchain swapchain = subviewport.swapchain_info.get_swapchain();

		if (swapchain && subviewport.swapchain_info.is_image_acquired()) {
			subviewport.swapchain_info.release();
		}

		r_subimage.swapchain = swapchain;
	}

	r_subimage.imageRect.extent.width = swapchain_size.width;
	r_subimage.imageRect.extent.height = swapchain_size.height;
}

bool OpenXRCompositionLayerExtension::CompositionLayer::update_and_acquire_swapchain(
	bool p_static_image)
{
	ERR_FAIL_COND_V(use_android_surface, false);

	if (!OpenXRAPI::is_allocated() || !OpenXRCompositionLayerExtension::is_initialized()) {
		return false;
	}
	if (!OpenXRCompositionLayerExtension::is_available(composition_layer.type)) {
		return false;
	}

	if (subviewport.swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
		if (swapchain_size == subviewport.viewport_size && !p_static_image &&
			!subviewport.static_image &&
			protected_content == subviewport.swapchain_protected_content) {
			bool should_render = true;
			return subviewport.swapchain_info.acquire(should_render);
		}

		subviewport.swapchain_info.queue_free();
	}

	int64_t swapchain_format = OpenXRAPI::get_color_swapchain_format();
	const uint32_t sample_count = 1;
	const uint32_t array_size = 1;
	XrSwapchainCreateFlags create_flags = 0;
	if (p_static_image) {
		create_flags |= XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
	}
	if (protected_content) {
		create_flags |= XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;
	}
	if (!subviewport.swapchain_info.create(create_flags,
			XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
				XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT,
			swapchain_format, subviewport.viewport_size.width, subviewport.viewport_size.height,
			sample_count, array_size)) {
		swapchain_size = Size2i();
		return false;
	}

	swapchain_state_is_dirty = true;

	bool should_render = true;
	bool ret = subviewport.swapchain_info.acquire(should_render);

	swapchain_size = subviewport.viewport_size;
	subviewport.static_image = p_static_image;
	subviewport.swapchain_protected_content = protected_content;
	return ret;
}

RID OpenXRCompositionLayerExtension::CompositionLayer::get_current_swapchain_texture()
{
	ERR_FAIL_COND_V(use_android_surface, RID());

	if (!OpenXRAPI::is_allocated()) {
		return RID();
	}

	return subviewport.swapchain_info.get_image();
}

void OpenXRCompositionLayerExtension::CompositionLayer::free_swapchain()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		if (android_surface.swapchain != XR_NULL_HANDLE) {
			OpenXRCompositionLayerExtension::xrDestroySwapchain(android_surface.swapchain);
			android_surface.swapchain = XR_NULL_HANDLE;
			android_surface.surface.unref();
		}
	}
	else
#endif
	{
		if (subviewport.swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
			subviewport.swapchain_info.queue_free();
		}
		subviewport.static_image = false;
	}

	swapchain_size = Size2i();
}

#ifdef ANDROID_ENABLED
void OpenXRCompositionLayerExtension::CompositionLayer::create_android_surface()
{
	ERR_FAIL_COND(
		android_surface.swapchain != XR_NULL_HANDLE || android_surface.surface.is_valid());

	XrSwapchainCreateFlags create_flags = 0;
	if (protected_content) {
		create_flags = XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;
	}

	XrSwapchainCreateInfo info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO,
		nullptr,
		create_flags,
		XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
			XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT,
		0,
		0,
		(uint32_t)swapchain_size.x,
		(uint32_t)swapchain_size.y,
		0,
		0,
		0,
	};

	jobject surface = nullptr;
	OpenXRCompositionLayerExtension::create_android_surface_swapchain(
		&info, &android_surface.swapchain, &surface);

	swapchain_state_is_dirty = true;

	if (surface) {
		android_surface.surface.instantiate(
			JavaClassWrapper::wrap("android.view.Surface"), surface);
	}
}
#endif

void OpenXRCompositionLayerExtension::composition_layer_set_viewport(
	RID p_layer, RID p_viewport, const Vector2i& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
	RID p_layer, bool p_enable, const Vector2i& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_unregister(RID p_layer) {}

void OpenXRCompositionLayerExtension::composition_layer_set_vertical_wrap(
	RID p_layer, OpenXRCompositionLayerExtension::Wrap p_wrap)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_red_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_quad_size(
	RID p_layer, const Vector2& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_protected_content(
	RID p_layer, bool p_protected)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_mipmap_mode(
	RID p_layer, OpenXRCompositionLayerExtension::MipmapMode p_mode)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_min_filter(
	RID p_layer, OpenXRCompositionLayerExtension::Filter p_filter)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_max_anisotropy(
	RID p_layer, float p_anisotropy)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_mag_filter(
	RID p_layer, OpenXRCompositionLayerExtension::Filter p_filter)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_horizontal_wrap(
	RID p_layer, OpenXRCompositionLayerExtension::Wrap p_wrap)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_green_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_eye_visibility(
	RID p_layer, OpenXRCompositionLayerExtension::EyeVisibility p_visibility)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_upper_vertical_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_radius(
	RID p_layer, float p_radius)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_lower_vertical_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_central_horizontal_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_radius(
	RID p_layer, float p_radius)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_central_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_aspect_ratio(
	RID p_layer, float p_ratio)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_border_color(
	RID p_layer, const Color& p_color)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_blue_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_alpha_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_alpha_blend(RID p_layer, bool p_blend)
{
}

void OpenXRCompositionLayerExtension::composition_layer_register(RID p_layer) {}

XrCompositionLayerBaseHeader*
OpenXRCompositionLayerExtension::CompositionLayer::get_composition_layer()
{
	return &composition_layer;
}

void OpenXRCompositionLayerExtension::on_session_created(XrSession_T* p_session) {}

void OpenXRCompositionLayerExtension::on_session_destroyed() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRFrameSynthesisExtension

void OpenXRFrameSynthesisExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRFrameSynthesisExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRFrameSynthesisExtension::prepare_view_configuration(uint32_t p_view_count)
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	data->render_state.config_views.resize(p_view_count);

	for (XrFrameSynthesisConfigViewEXT& config_view : data->render_state.config_views) {
		config_view.type = XR_TYPE_FRAME_SYNTHESIS_CONFIG_VIEW_EXT;
		config_view.next = nullptr;
		config_view.recommendedMotionVectorImageRectWidth = 0;
		config_view.recommendedMotionVectorImageRectHeight = 0;
	}
}

void* OpenXRFrameSynthesisExtension::set_view_configuration_and_get_next_pointer(
	uint32_t p_view, void* p_next_pointer)
{
	if (!data->frame_synthesis_ext) {
		return nullptr;
	}

	ERR_FAIL_UNSIGNED_INDEX_V(p_view, data->render_state.config_views.size(), nullptr);

	XrFrameSynthesisConfigViewEXT& config_view = data->render_state.config_views[p_view];
	config_view.next = p_next_pointer;

	return &config_view;
}

void OpenXRFrameSynthesisExtension::on_session_destroyed()
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	free_swapchains();
}

void OpenXRFrameSynthesisExtension::on_pre_draw_viewport(RID p_render_target)
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (!data->enabled || data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2 || data->render_state.skip_next_frame) {
		OpenXRAPI::set_velocity_texture(RID());
		OpenXRAPI::set_velocity_depth_texture(RID());
		data->render_state.previous_transform = XRServer::get_world_origin();
		return;
	}

	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		bool should_render = true;
		data->render_state.swapchains[i].acquire(should_render);
	}

	OpenXRAPI::set_velocity_texture(
		data->render_state.swapchains[SWAPCHAIN_MOTION_VECTOR].get_image());
	OpenXRAPI::set_velocity_depth_texture(
		data->render_state.swapchains[SWAPCHAIN_DEPTH].get_image());

	uint32_t width = data->render_state.config_views[0].recommendedMotionVectorImageRectWidth;
	uint32_t height = data->render_state.config_views[0].recommendedMotionVectorImageRectHeight;
	OpenXRAPI::set_velocity_target_size(Size2i(width, height));

	Transform3D world_transform = XRServer::get_world_origin();
	Transform3D delta_transform =
		data->render_state.previous_transform.affine_inverse() * world_transform;
	Quaternion delta_quat = delta_transform.basis.get_quaternion();
	Vector3 delta_origin = delta_transform.origin;

	float z_far = OpenXRAPI::get_render_state_z_far();
	float z_near = OpenXRAPI::get_render_state_z_near();

	for (XrFrameSynthesisInfoEXT& frame_synthesis_info : data->render_state.frame_synthesis_info) {
		frame_synthesis_info.layerFlags =
			data->render_state.relax_frame_interval
				? XR_FRAME_SYNTHESIS_INFO_REQUEST_RELAXED_FRAME_INTERVAL_BIT_EXT
				: 0;

		frame_synthesis_info.appSpaceDeltaPose = {
			{(float)delta_quat.x, (float)delta_quat.y, (float)delta_quat.z, (float)delta_quat.w},
			{(float)delta_origin.x, (float)delta_origin.y, (float)delta_origin.z}};

		if (z_far != z_near) {
			frame_synthesis_info.nearZ = z_far;
			frame_synthesis_info.farZ = z_near;
		}
	}

	data->render_state.previous_transform = world_transform;
}

void OpenXRFrameSynthesisExtension::on_post_draw_viewport(RID p_render_target)
{
	if (!data->frame_synthesis_ext || !data->enabled ||
		data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2 || data->render_state.skip_next_frame) {
		return;
	}

	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		data->render_state.swapchains[i].release();
	}
}

void* OpenXRFrameSynthesisExtension::set_projection_views_and_get_next_pointer(
	int p_view_index, void* p_next_pointer)
{
	if (!data->frame_synthesis_ext || !data->enabled ||
		data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2) {
		return nullptr;
	}

	if (data->render_state.skip_next_frame) {
		if (p_view_index == 1) {
			data->render_state.skip_next_frame = false;
		}
		return nullptr;
	}

	data->render_state.frame_synthesis_info[p_view_index].next = p_next_pointer;
	return &data->render_state.frame_synthesis_info[p_view_index];
}

bool OpenXRFrameSynthesisExtension::is_available()
{
	return data != nullptr && data->frame_synthesis_ext;
}

bool OpenXRFrameSynthesisExtension::is_enabled()
{
	return data != nullptr && data->frame_synthesis_ext && data->enabled;
}

bool OpenXRFrameSynthesisExtension::get_relax_frame_interval()
{
	return data != nullptr && data->relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::_set_render_state_enabled_rt(bool p_enabled)
{
	data->render_state.enabled = p_enabled;
}

void OpenXRFrameSynthesisExtension::_set_relax_frame_interval_rt(bool p_relax_frame_interval)
{
	data->render_state.relax_frame_interval = p_relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::free_swapchains()
{
	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		data->render_state.swapchains[i].queue_free();
	}
}

void OpenXRFrameSynthesisExtension::_set_skip_next_frame_rt()
{
	data->render_state.skip_next_frame = true;
}

HashMap<String, bool*> OpenXRFrameSynthesisExtension::get_requested_extensions(
	XrVersion p_version)
{
	return HashMap<String, bool*>();
}

void OpenXRFrameSynthesisExtension::on_instance_created(XrInstance_T* p_instance) {}

void OpenXRFrameSynthesisExtension::on_instance_destroyed() {}

void OpenXRFrameSynthesisExtension::print_view_configuration_info(unsigned int p_view_config) {}

void OpenXRFrameSynthesisExtension::on_main_swapchains_created() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRFutureResult & OpenXRFutureExtension

void OpenXRFutureExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRFutureExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

OpenXRFutureResult::ResultStatus OpenXRFutureResult::get_status() const { return status; }

XrFutureEXT OpenXRFutureResult::get_future() const { return future; }

uint64_t OpenXRFutureResult::_get_future() const { return (uint64_t)future; }

void OpenXRFutureResult::cancel_future()
{
	ERR_FAIL_COND(status != RESULT_RUNNING);
	ERR_FAIL_COND(!OpenXRFutureExtension::is_initialized());

	OpenXRFutureExtension::cancel_future(future);
}

void OpenXRFutureResult::_mark_as_cancelled() {}

void OpenXRFutureResult::_mark_as_finished() {}

HashMap<String, bool*> OpenXRFutureExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_FUTURE_EXTENSION_NAME] = &data->future_ext;

	return request_extensions;
}

void OpenXRFutureExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->future_ext) {
		EXT_INIT_XR_FUNC(xrPollFutureEXT);
		EXT_INIT_XR_FUNC(xrCancelFutureEXT);

		data->future_ext = xrPollFutureEXT_ptr && xrCancelFutureEXT_ptr;
	}
}

void OpenXRFutureExtension::on_instance_destroyed()
{
	xrPollFutureEXT_ptr = nullptr;
	xrCancelFutureEXT_ptr = nullptr;
}

void OpenXRFutureExtension::on_session_destroyed()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFutureCancelInfoEXT cancel_info = {
			XR_TYPE_FUTURE_CANCEL_INFO_EXT,
			nullptr,
			element.key,
		};
		XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
		if (XR_FAILED(result)) {
			WARN_PRINT(
				"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
		}

		element.value->_mark_as_cancelled();
	}
	data->futures.clear();
}

bool OpenXRFutureExtension::is_active() { return data != nullptr && data->future_ext; }

void OpenXRFutureExtension::cancel_future(XrFutureEXT p_future)
{
	ERR_FAIL_COND(!data->futures.has(p_future));
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrFutureCancelInfoEXT cancel_info = {
		XR_TYPE_FUTURE_CANCEL_INFO_EXT,
		nullptr,
		p_future,
	};
	XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
	if (XR_FAILED(result)) {
		WARN_PRINT(
			"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
	}

	data->futures[p_future]->_mark_as_cancelled();
	data->futures.erase(p_future);
}

void OpenXRFutureExtension::_cancel_future(uint64_t p_future)
{
	cancel_future((XrFutureEXT)p_future);
}

void OpenXRFutureExtension::on_process()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	Vector<XrFutureEXT> completed;
	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFuturePollInfoEXT poll_info = {
			XR_TYPE_FUTURE_POLL_INFO_EXT,
			nullptr,
			element.key,
		};
		XrFuturePollResultEXT poll_result = {
			XR_TYPE_FUTURE_POLL_RESULT_EXT,
			nullptr,
			XR_FUTURE_STATE_MAX_ENUM_EXT,
		};
		XrResult result = xrPollFutureEXT_ptr(OpenXRAPI::get_instance(), &poll_info, &poll_result);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to obtain future status [" +
					  OpenXRAPI::get_error_string(result) + "]");
			continue;
		}
		if (poll_result.state == XR_FUTURE_STATE_READY_EXT) {
			element.value->_mark_as_finished();
			completed.push_back(element.key);
		}
	}

	for (const XrFutureEXT& future : completed) {
		data->futures.erase(future);
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRHandTrackingExtension

void OpenXRHandTrackingExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRHandTrackingExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRHandTrackingExtension::on_session_destroyed() { cleanup_hand_tracking(); }

void OpenXRHandTrackingExtension::on_state_stopping() { cleanup_hand_tracking(); }

const OpenXRHandTrackingExtension::HandTracker* OpenXRHandTrackingExtension::get_hand_tracker(
	HandTrackedHands p_hand)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_hand, OPENXR_MAX_TRACKED_HANDS, nullptr);

	return &data->hand_trackers[p_hand];
}

bool OpenXRHandTrackingExtension::get_active() { return true; }

void OpenXRHandTrackingExtension::cleanup_hand_tracking() {}

void OpenXRHandTrackingExtension::on_instance_destroyed() {}

void* OpenXRHandTrackingExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
	return p_next_pointer;
}

void OpenXRHandTrackingExtension::on_state_ready() {}

void OpenXRHandTrackingExtension::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRRenderModelExtension

#ifdef MODULE_GLTF_ENABLED
#include "modules/gltf/gltf_document.h"
#include "scene/3d/node_3d.h"

OpenXRRenderModelData::OpenXRRenderModelData() = default;
OpenXRRenderModelData::~OpenXRRenderModelData() = default;

Ref<GLTFState> OpenXRRenderModelData::get_gltf_state() { return gltf_state; }

void OpenXRRenderModelExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRRenderModelExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRRenderModelExtension::on_instance_created(const XrInstance p_instance)
{
	EXT_INIT_XR_FUNC(xrLocateSpace);
	EXT_INIT_XR_FUNC(xrDestroySpace);
	EXT_INIT_XR_FUNC(xrPathToString);

	if (data->render_model_ext) {
		EXT_INIT_XR_FUNC(xrCreateRenderModelEXT);
		EXT_INIT_XR_FUNC(xrDestroyRenderModelEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelPropertiesEXT);
		EXT_INIT_XR_FUNC(xrCreateRenderModelSpaceEXT);
		EXT_INIT_XR_FUNC(xrCreateRenderModelAssetEXT);
		EXT_INIT_XR_FUNC(xrDestroyRenderModelAssetEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelAssetDataEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelAssetPropertiesEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelStateEXT);
	}

	if (data->interaction_render_model_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateInteractionRenderModelIdsEXT);
		EXT_INIT_XR_FUNC(xrEnumerateRenderModelSubactionPathsEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelPoseTopLevelUserPathEXT);
	}
}

void OpenXRRenderModelExtension::on_session_created(const XrSession p_session)
{
	data->_interaction_data_dirty = true;
}

void OpenXRRenderModelExtension::on_instance_destroyed()
{
	xrCreateRenderModelEXT_ptr = nullptr;
	xrDestroyRenderModelEXT_ptr = nullptr;
	xrGetRenderModelPropertiesEXT_ptr = nullptr;
	xrCreateRenderModelSpaceEXT_ptr = nullptr;
	xrCreateRenderModelAssetEXT_ptr = nullptr;
	xrDestroyRenderModelAssetEXT_ptr = nullptr;
	xrGetRenderModelAssetDataEXT_ptr = nullptr;
	xrGetRenderModelAssetPropertiesEXT_ptr = nullptr;
	xrGetRenderModelStateEXT_ptr = nullptr;
	xrEnumerateInteractionRenderModelIdsEXT_ptr = nullptr;
	xrEnumerateRenderModelSubactionPathsEXT_ptr = nullptr;
	xrGetRenderModelPoseTopLevelUserPathEXT_ptr = nullptr;

	data->uuid_ext = false;
	data->render_model_ext = false;
	data->interaction_render_model_ext = false;
}

void OpenXRRenderModelExtension::on_session_destroyed()
{
	_clear_interaction_data();
	_clear_render_model_data();
	data->xr_sync_has_run = false;
}

bool OpenXRRenderModelExtension::on_event_polled(const XrEventDataBuffer& event)
{
	if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_RENDER_MODELS_CHANGED_EXT) {
		data->_interaction_data_dirty = true;
		return true;
	}
	else if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
		data->_interaction_data_dirty = true;
		return false;
	}

	return false;
}

void OpenXRRenderModelExtension::on_sync_actions()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	data->xr_sync_has_run = true;

	if (data->_interaction_data_dirty) {
		_update_interaction_data();
	}

	LocalVector<RID> owned = data->render_model_owner.get_owned_list();

	for (const RID& rid : owned) {
		RenderModel* render_model = data->render_model_owner.get_or_null(rid);
		if (render_model && render_model->xr_space != XR_NULL_HANDLE) {
			XrSpaceLocation render_model_location = {
				XR_TYPE_SPACE_LOCATION,
				nullptr,
				0,
				{{0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}},
			};

			XrResult result = xrLocateSpace(render_model->xr_space, OpenXRAPI::get_play_space(),
				OpenXRAPI::get_predicted_display_time(), &render_model_location);
			ERR_CONTINUE_MSG(XR_FAILED(result), "OpenXR: Failed to locate render model space [" +
													OpenXRAPI::get_error_string(result) + "]");

			render_model->confidence = OpenXRAPI::transform_from_location(
				render_model_location, render_model->root_transform);

			if (!render_model->node_states.is_empty()) {
				XrRenderModelStateGetInfoEXT get_state_info = {
					XR_TYPE_RENDER_MODEL_STATE_GET_INFO_EXT,
					nullptr,
					OpenXRAPI::get_predicted_display_time(),
				};

				XrRenderModelStateEXT state = {
					XR_TYPE_RENDER_MODEL_STATE_EXT,
					nullptr,
					render_model->animatable_node_count,
					render_model->node_states.ptr(),
				};

				result = xrGetRenderModelStateEXT(
					render_model->xr_render_model, &get_state_info, &state);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to update node states [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
			}

			XrPath new_path = XR_NULL_PATH;

			if (data->toplevel_paths.is_empty()) {
				data->toplevel_paths.push_back(OpenXRAPI::get_xr_path("/user/hand/left"));
				data->toplevel_paths.push_back(OpenXRAPI::get_xr_path("/user/hand/right"));
			}

			XrInteractionRenderModelTopLevelUserPathGetInfoEXT info = {
				XR_TYPE_INTERACTION_RENDER_MODEL_TOP_LEVEL_USER_PATH_GET_INFO_EXT,
				nullptr,
				(uint32_t)data->toplevel_paths.size(),
				data->toplevel_paths.ptr(),
			};
			result = xrGetRenderModelPoseTopLevelUserPathEXT(
				render_model->xr_render_model, &info, &new_path);
			if (XR_FAILED(result)) {
				ERR_PRINT("OpenXR: Failed to update the top level path for render models [" +
						  OpenXRAPI::get_error_string(result) + "]");
			}
			else if (new_path != render_model->top_level_path) {
				print_verbose("OpenXR: Render model top level path changed to " +
							  OpenXRAPI::get_xr_path_name(new_path));
				render_model->top_level_path = new_path;
			}
		}
	}
}

bool OpenXRRenderModelExtension::is_active()
{
	return data != nullptr && data->render_model_ext && data->interaction_render_model_ext;
}

void OpenXRRenderModelExtension::_clear_interaction_data()
{
	for (const KeyValue<XrRenderModelIdEXT, RID>& e : data->interaction_render_models) {
		render_model_destroy(e.value);
	}
	data->interaction_render_models.clear();
}

bool OpenXRRenderModelExtension::_update_interaction_data()
{
	ERR_FAIL_COND_V_MSG(!data->interaction_render_model_ext, false,
		"Interaction render model extension hasn't been enabled.");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	if (!data->xr_sync_has_run) {
		return true;
	}

	data->_interaction_data_dirty = false;

	XrInteractionRenderModelIdsEnumerateInfoEXT interaction_info = {
		XR_TYPE_INTERACTION_RENDER_MODEL_IDS_ENUMERATE_INFO_EXT,
		nullptr,
	};

	uint32_t interaction_count = 0;
	XrResult result = xrEnumerateInteractionRenderModelIdsEXT(
		session, &interaction_info, 0, &interaction_count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(false, "OpenXR: Failed to obtain render model interaction id count [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	LocalVector<XrRenderModelIdEXT> render_model_interaction_ids;
	render_model_interaction_ids.resize(interaction_count);

	if (!render_model_interaction_ids.is_empty()) {
		result = xrEnumerateInteractionRenderModelIdsEXT(session, &interaction_info,
			render_model_interaction_ids.size(), &interaction_count,
			render_model_interaction_ids.ptr());
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to obtain render model interaction ids [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}
	}

	LocalVector<XrRenderModelIdEXT> erase_ids;
	for (const KeyValue<XrRenderModelIdEXT, RID>& e : data->interaction_render_models) {
		if (!render_model_interaction_ids.has(e.key)) {
			if (e.value.is_valid()) {
				render_model_destroy(e.value);
			}
			erase_ids.push_back(e.key);
		}
	}

	for (const XrRenderModelIdEXT& id : erase_ids) {
		data->interaction_render_models.erase(id);
	}

	for (const XrRenderModelIdEXT& id : render_model_interaction_ids) {
		if (!data->interaction_render_models.has(id)) {
			data->interaction_render_models[id] = render_model_create(id);
		}
	}

	return true;
}

bool OpenXRRenderModelExtension::has_render_model(RID p_render_model)
{
	return data->render_model_owner.owns(p_render_model);
}

RID OpenXRRenderModelExtension::render_model_create(XrRenderModelIdEXT p_render_model_id)
{
	ERR_FAIL_COND_V_MSG(
		!data->render_model_ext, RID(), "Render model extension hasn't been enabled.");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, RID());

	RenderModel render_model;
	render_model.xr_render_model_id = p_render_model_id;

	const HashSet<String> supported_gltf_extensions_hash_set =
		GLTFDocument::get_supported_gltf_extensions_hashset();
	Vector<CharString> supported_gltf_extensions_char_string;
	supported_gltf_extensions_char_string.resize(supported_gltf_extensions_hash_set.size());
	int64_t supported_gltf_extension_index = 0;
	for (const String& ext : supported_gltf_extensions_hash_set) {
		supported_gltf_extensions_char_string.set(supported_gltf_extension_index, ext.utf8());
		supported_gltf_extension_index++;
	}
	Vector<const char*> supported_gltf_extensions;
	supported_gltf_extensions.resize(supported_gltf_extensions_char_string.size());
	for (int64_t i = 0; i < supported_gltf_extensions_char_string.size(); i++) {
		supported_gltf_extensions.write[i] = supported_gltf_extensions_char_string[i].get_data();
	}

	XrRenderModelCreateInfoEXT create_info = {
		XR_TYPE_RENDER_MODEL_CREATE_INFO_EXT,
		nullptr,
		p_render_model_id,
		uint32_t(supported_gltf_extensions.size()),
		supported_gltf_extensions.ptr(),
	};

	XrResult result = xrCreateRenderModelEXT(session, &create_info, &render_model.xr_render_model);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create render model [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	XrRenderModelPropertiesGetInfoEXT properties_info = {
		XR_TYPE_RENDER_MODEL_PROPERTIES_GET_INFO_EXT,
		nullptr,
	};

	XrRenderModelPropertiesEXT properties = {
		XR_TYPE_RENDER_MODEL_PROPERTIES_EXT,
		nullptr,
		{},
		0,
	};

	result =
		xrGetRenderModelPropertiesEXT(render_model.xr_render_model, &properties_info, &properties);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to get render model properties [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}
	else {
		render_model.animatable_node_count = properties.animatableNodeCount;
		render_model.render_model_data =
			_get_render_model_data(properties.cacheId, properties.animatableNodeCount);
	}

	XrRenderModelSpaceCreateInfoEXT space_create_info = {
		XR_TYPE_RENDER_MODEL_SPACE_CREATE_INFO_EXT,
		nullptr,
		render_model.xr_render_model,
	};

	result = xrCreateRenderModelSpaceEXT(session, &space_create_info, &render_model.xr_space);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to create render model space [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	if (render_model.animatable_node_count > 0) {
		render_model.node_states.resize(render_model.animatable_node_count);
	}

	return data->render_model_owner.make_rid(render_model);
}

RID OpenXRRenderModelExtension::_render_model_create(uint64_t p_render_model_id)
{
	RID ret;

	ERR_FAIL_COND_V(p_render_model_id == XR_NULL_RENDER_MODEL_ID_EXT, ret);

	if (is_active()) {
		ret = render_model_create(XrRenderModelIdEXT(p_render_model_id));
	}

	return ret;
}

void OpenXRRenderModelExtension::render_model_destroy(RID p_render_model)
{
	ERR_FAIL_COND_MSG(!data->render_model_ext, "Render model extension hasn't been enabled.");

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL(render_model);

	if (render_model->xr_space != XR_NULL_HANDLE) {
		xrDestroySpace(render_model->xr_space);
	}

	render_model->node_states.clear();

	XrResult result = xrDestroyRenderModelEXT(render_model->xr_render_model);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to destroy render model [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	data->render_model_owner.free(p_render_model);
}

Node3D* OpenXRRenderModelExtension::render_model_new_scene_instance(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, nullptr);

	if (render_model->render_model_data.is_null()) {
		return nullptr;
	}

	return render_model->render_model_data->new_scene_instance();
}

PackedStringArray OpenXRRenderModelExtension::render_model_get_subaction_paths(RID p_render_model)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedStringArray());

	XrInstance instance = OpenXRAPI::get_instance();
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, PackedStringArray());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, PackedStringArray());

	PackedStringArray subaction_paths;

	XrInteractionRenderModelSubactionPathInfoEXT subaction_info = {
		XR_TYPE_INTERACTION_RENDER_MODEL_SUBACTION_PATH_INFO_EXT,
		nullptr,
	};

	uint32_t capacity;

	XrResult result = xrEnumerateRenderModelSubactionPathsEXT(
		render_model->xr_render_model, &subaction_info, 0, &capacity, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			PackedStringArray(), "OpenXR: Failed to obtain render model subaction path count [" +
									 OpenXRAPI::get_error_string(result) + "]");
	}

	if (capacity > 0) {
		LocalVector<XrPath> paths;
		paths.resize(capacity);

		result = xrEnumerateRenderModelSubactionPathsEXT(
			render_model->xr_render_model, &subaction_info, capacity, &capacity, paths.ptr());
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(PackedStringArray(),
				"OpenXR: Failed to obtain render model subaction paths [" +
					OpenXRAPI::get_error_string(result) + "]");
		}

		for (uint32_t i = 0; i < capacity; i++) {
			char buffer[1024];
			uint32_t size = 0;
			xrPathToString(instance, paths[i], 1024, &size, buffer);
			if (size > 0) {
				subaction_paths.push_back(String(buffer));
			}
		}
	}

	return subaction_paths;
}

XrPath OpenXRRenderModelExtension::render_model_get_top_level_path(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE);

	return render_model->top_level_path;
}

String OpenXRRenderModelExtension::render_model_get_top_level_path_as_string(RID p_render_model)
{
	String ret;

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	if (is_active() && has_render_model(p_render_model)) {
		XrPath path = render_model_get_top_level_path(p_render_model);
		if (path == XR_NULL_PATH) {
			return "None";
		}
		else {
			return OpenXRAPI::get_xr_path_name(path);
		}
	}

	return ret;
}

XRPose::TrackingConfidence OpenXRRenderModelExtension::render_model_get_confidence(
	RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE);

	return render_model->confidence;
}

Transform3D OpenXRRenderModelExtension::render_model_get_root_transform(RID p_render_model)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, Transform3D());

	real_t world_scale = XRServer::get_world_scale();
	Transform3D root_transform =
		render_model->root_transform.scaled(Vector3(world_scale, world_scale, world_scale));

	return XRServer::get_reference_frame() * root_transform;
}

uint32_t OpenXRRenderModelExtension::render_model_get_animatable_node_count(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, 0);

	return render_model->animatable_node_count;
}

String OpenXRRenderModelExtension::render_model_get_animatable_node_name(
	RID p_render_model, uint32_t p_index)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, String());

	if (render_model->render_model_data.is_null()) {
		return String();
	}

	return render_model->render_model_data->get_node_name(p_index);
}

bool OpenXRRenderModelExtension::render_model_is_animatable_node_visible(
	RID p_render_model, uint32_t p_index)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, render_model->animatable_node_count, false);

	if (render_model->node_states.is_empty()) {
		return false;
	}

	return render_model->node_states[p_index].isVisible;
}

Transform3D OpenXRRenderModelExtension::render_model_get_animatable_node_transform(
	RID p_render_model, uint32_t p_index)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, Transform3D());
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, render_model->animatable_node_count, Transform3D());

	if (render_model->node_states.is_empty()) {
		return Transform3D();
	}

	return OpenXRAPI::transform_from_pose(render_model->node_states[p_index].nodePose);
}

Ref<OpenXRRenderModelData> OpenXRRenderModelExtension::_get_render_model_data(
	XrUuidEXT p_cache_id, uint32_t p_animatable_node_count)
{
	if (data->render_model_data_cache.has(p_cache_id)) {
		return data->render_model_data_cache[p_cache_id];
	}

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, nullptr);

	XrRenderModelAssetEXT asset;
	XrRenderModelAssetCreateInfoEXT create_info = {
		XR_TYPE_RENDER_MODEL_ASSET_CREATE_INFO_EXT,
		nullptr,
		p_cache_id,
	};

	XrResult result = xrCreateRenderModelAssetEXT(session, &create_info, &asset);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to create render model asset [" +
									OpenXRAPI::get_error_string(result) + "]");
	}

	Ref<OpenXRRenderModelData> render_model_data = _load_asset(asset, p_animatable_node_count);

	result = xrDestroyRenderModelAssetEXT(asset);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to destroy render model asset [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	data->render_model_data_cache[p_cache_id] = render_model_data;

	return render_model_data;
}

Ref<OpenXRRenderModelData> OpenXRRenderModelExtension::_load_asset(
	XrRenderModelAssetEXT p_asset, uint32_t p_animatable_node_count)
{
	XrRenderModelAssetDataGetInfoEXT get_info = {
		XR_TYPE_RENDER_MODEL_ASSET_DATA_GET_INFO_EXT,
		nullptr,
	};

	XrRenderModelAssetDataEXT asset_data = {
		XR_TYPE_RENDER_MODEL_ASSET_DATA_EXT,
		nullptr,
		0,
		0,
		nullptr,
	};

	XrResult result = xrGetRenderModelAssetDataEXT(p_asset, &get_info, &asset_data);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model buffer size [" +
									OpenXRAPI::get_error_string(result) + "]");
	}
	ERR_FAIL_COND_V(asset_data.bufferCountOutput == 0, nullptr);

	PackedByteArray buffer;
	buffer.resize(asset_data.bufferCountOutput);
	asset_data.buffer = buffer.ptrw();
	asset_data.bufferCapacityInput = asset_data.bufferCountOutput;

	result = xrGetRenderModelAssetDataEXT(p_asset, &get_info, &asset_data);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model buffer [" +
									OpenXRAPI::get_error_string(result) + "]");
	}

	PackedStringArray node_names;

	if (p_animatable_node_count > 0) {
		Vector<XrRenderModelAssetNodePropertiesEXT> node_properties;
		node_properties.resize(p_animatable_node_count);

		XrRenderModelAssetPropertiesGetInfoEXT properties_info = {
			XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_GET_INFO_EXT,
			nullptr,
		};

		XrRenderModelAssetPropertiesEXT asset_properties = {
			XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_EXT,
			nullptr,
			uint32_t(node_properties.size()),
			node_properties.ptrw(),
		};

		result = xrGetRenderModelAssetPropertiesEXT(p_asset, &properties_info, &asset_properties);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model property info [" +
										OpenXRAPI::get_error_string(result) + "]");
		}

		node_names.resize(p_animatable_node_count);
		String* node_names_ptrw = node_names.ptrw();
		for (uint32_t i = 0; i < p_animatable_node_count; i++) {
			node_names_ptrw[i] = String(node_properties[i].uniqueName);
		}
	}

	Ref<OpenXRRenderModelData> render_model_data;
	render_model_data.instantiate();

	render_model_data->parse_gltf_document(buffer);
	render_model_data->set_node_names(node_names);

	return render_model_data;
}

void OpenXRRenderModelExtension::_clear_render_model_data()
{
	data->toplevel_paths.clear();
	data->render_model_data_cache.clear();

	LocalVector<RID> owned = data->render_model_owner.get_owned_list();
	for (const RID& rid : owned) {
		render_model_destroy(rid);
	}
}

bool OpenXRRenderModelData::parse_gltf_document(const PackedByteArray& p_bytes)
{
	Ref<GLTFState> new_state;
	new_state.instantiate();
	Ref<GLTFDocument> new_gltf_document;
	new_gltf_document.instantiate();

	Error err = new_gltf_document->append_from_buffer(p_bytes, "", new_state);
	if (err != OK) {
		ERR_FAIL_V_MSG(false, "OpenXR: Failed to parse GLTF data.");
	}

	gltf_document = new_gltf_document;
	gltf_state = new_state;
	return true;
}

void OpenXRRenderModelData::set_node_names(const PackedStringArray& p_node_names)
{
	node_names = p_node_names;
}

PackedStringArray OpenXRRenderModelData::get_node_names() const { return node_names; }

const String OpenXRRenderModelData::get_node_name(uint32_t p_node_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_node_index, node_names.size(), String());

	return node_names[p_node_index];
}

Node3D* OpenXRRenderModelData::new_scene_instance()
{
	return nullptr;
}

HashMap<String, bool*> OpenXRRenderModelExtension::get_requested_extensions(XrVersion p_version)
{
	return HashMap<String, bool*>();
}
#endif // MODULE_GLTF_ENABLED

////////////////////////////////////////////////////////////////////////////
// Platform-header dependent extensions (included last to isolate X11/driver macros)

#define Window OpenXRX11Window
#include "../openxr_platform_inc.h"
#undef Window

////////////////////////////////////////////////////////////////////////////
// OpenXRFBUpdateSwapchainExtension

#ifndef GL_CUBIC_IMG
#define GL_CUBIC_IMG 0x9139
#endif
#ifndef GL_CUBIC_MIPMAP_LINEAR_IMG
#define GL_CUBIC_MIPMAP_LINEAR_IMG 0x913B
#endif
#ifndef GL_CUBIC_MIPMAP_NEAREST_IMG
#define GL_CUBIC_MIPMAP_NEAREST_IMG 0x913A
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

void OpenXRFBUpdateSwapchainExtension::initialize(const String& p_rendering_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->rendering_driver = p_rendering_driver;
}

void OpenXRFBUpdateSwapchainExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRFBUpdateSwapchainExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME] =
		&data->fb_swapchain_update_state_ext;

	if (data->rendering_driver == "vulkan") {
#ifdef XR_USE_GRAPHICS_API_VULKAN
		request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_VULKAN_EXTENSION_NAME] =
			&data->fb_swapchain_update_state_vulkan_ext;
#endif
	}
	else if (data->rendering_driver == "opengl3" || data->rendering_driver == "opengl3_es") {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME] =
			&data->fb_swapchain_update_state_opengles_ext;
#endif
	}

#ifdef ANDROID_ENABLED
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_ANDROID_SURFACE_EXTENSION_NAME] =
		&data->fb_swapchain_update_state_android_ext;
#endif

	return request_extensions;
}

void OpenXRFBUpdateSwapchainExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->fb_swapchain_update_state_ext) {
		EXT_INIT_XR_FUNC(xrUpdateSwapchainFB);
		EXT_INIT_XR_FUNC(xrGetSwapchainStateFB);
	}
}

void OpenXRFBUpdateSwapchainExtension::on_instance_destroyed()
{
	data->fb_swapchain_update_state_ext = false;
	data->fb_swapchain_update_state_vulkan_ext = false;
	data->fb_swapchain_update_state_opengles_ext = false;
}

bool OpenXRFBUpdateSwapchainExtension::is_enabled()
{
	return data != nullptr && data->fb_swapchain_update_state_ext;
}

bool OpenXRFBUpdateSwapchainExtension::is_android_ext_enabled()
{
	return data != nullptr && data->fb_swapchain_update_state_android_ext;
}

void OpenXRFBUpdateSwapchainExtension::update_swapchain_state(XrSwapchain p_swapchain,
	const OpenXRCompositionLayerExtension::SwapchainState* p_swapchain_state)
{
	if (!p_swapchain_state) {
		return;
	}

	if (data->rendering_driver == "vulkan") {
#ifdef XR_USE_GRAPHICS_API_VULKAN
		if (!data->fb_swapchain_update_state_ext || !data->fb_swapchain_update_state_vulkan_ext) {
			return;
		}

		Color border_color = p_swapchain_state->border_color;
		XrSwapchainStateSamplerVulkanFB swapchain_state = {
			XR_TYPE_SWAPCHAIN_STATE_SAMPLER_VULKAN_FB,
			nullptr,
			(VkFilter)filter_to_vk(p_swapchain_state->min_filter),
			(VkFilter)filter_to_vk(p_swapchain_state->mag_filter),
			(VkSamplerMipmapMode)mipmap_mode_to_vk(p_swapchain_state->mipmap_mode),
			(VkSamplerAddressMode)wrap_to_vk(p_swapchain_state->horizontal_wrap),
			(VkSamplerAddressMode)wrap_to_vk(p_swapchain_state->vertical_wrap),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->red_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->green_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->blue_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->alpha_swizzle),
			p_swapchain_state->max_anisotropy,
			{border_color.r, border_color.g, border_color.b, border_color.a},
		};

		XrResult result =
			xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
		if (XR_FAILED(result)) {
			print_error(vformat(
				"OpenXR: Failed to update swapchain [%s]", OpenXRAPI::get_error_string(result)));
			return;
		}
#endif
	}
	else if (data->rendering_driver == "opengl3" || data->rendering_driver == "opengl3_es") {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		if (!data->fb_swapchain_update_state_ext ||
			!data->fb_swapchain_update_state_opengles_ext) {
			return;
		}

		Color border_color = p_swapchain_state->border_color;
		XrSwapchainStateSamplerOpenGLESFB swapchain_state = {
			XR_TYPE_SWAPCHAIN_STATE_SAMPLER_OPENGL_ES_FB,
			nullptr,
			filter_to_gl(p_swapchain_state->min_filter, p_swapchain_state->mipmap_mode),
			filter_to_gl(p_swapchain_state->mag_filter),
			wrap_to_gl(p_swapchain_state->horizontal_wrap),
			wrap_to_gl(p_swapchain_state->vertical_wrap),
			swizzle_to_gl(p_swapchain_state->red_swizzle),
			swizzle_to_gl(p_swapchain_state->green_swizzle),
			swizzle_to_gl(p_swapchain_state->blue_swizzle),
			swizzle_to_gl(p_swapchain_state->alpha_swizzle),
			p_swapchain_state->max_anisotropy,
			{border_color.r, border_color.g, border_color.b, border_color.a},
		};

		XrResult result =
			xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
		if (XR_FAILED(result)) {
			print_error(vformat(
				"OpenXR: Failed to update swapchain [%s]", OpenXRAPI::get_error_string(result)));
			return;
		}
#endif
	}
}

void OpenXRFBUpdateSwapchainExtension::update_swapchain_surface_size(
	XrSwapchain p_swapchain, const Size2i& p_size)
{
#ifdef ANDROID_ENABLED
	if (!data->fb_swapchain_update_state_ext || !data->fb_swapchain_update_state_android_ext) {
		return;
	}

	XrSwapchainStateAndroidSurfaceDimensionsFB swapchain_state = {
		XR_TYPE_SWAPCHAIN_STATE_ANDROID_SURFACE_DIMENSIONS_FB,
		nullptr,
		(uint32_t)p_size.width,
		(uint32_t)p_size.height,
	};

	XrResult result =
		xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: Failed to update swapchain surface size [%s]",
			OpenXRAPI::get_error_string(result)));
	}
#endif
}

uint32_t OpenXRFBUpdateSwapchainExtension::filter_to_gl(
	OpenXRCompositionLayerExtension::Filter p_filter,
	OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_mipmap_mode) {
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_IMG;
		}
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_NEAREST:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR_MIPMAP_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_MIPMAP_NEAREST_IMG;
		}
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_LINEAR:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST_MIPMAP_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR_MIPMAP_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_MIPMAP_LINEAR_IMG;
		}
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::wrap_to_gl(OpenXRCompositionLayerExtension::Wrap p_wrap)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_wrap) {
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_BORDER:
		return GL_CLAMP_TO_BORDER;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_REPEAT:
		return GL_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRRORED_REPEAT:
		return GL_MIRRORED_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRROR_CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::swizzle_to_gl(
	OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_swizzle) {
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_RED:
		return GL_RED;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_GREEN:
		return GL_GREEN;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_BLUE:
		return GL_BLUE;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ALPHA:
		return GL_ALPHA;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ZERO:
		return GL_ZERO;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ONE:
		return GL_ONE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::filter_to_vk(
	OpenXRCompositionLayerExtension::Filter p_filter)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_filter) {
	case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
		return VK_FILTER_NEAREST;
	case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
		return VK_FILTER_LINEAR;
	case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
		return VK_FILTER_CUBIC_EXT;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::mipmap_mode_to_vk(
	OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_mipmap_mode) {
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_NEAREST:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_LINEAR:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::wrap_to_vk(OpenXRCompositionLayerExtension::Wrap p_wrap)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_wrap) {
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRROR_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::swizzle_to_vk(
	OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_swizzle) {
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_RED:
		return VK_COMPONENT_SWIZZLE_R;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_GREEN:
		return VK_COMPONENT_SWIZZLE_G;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_BLUE:
		return VK_COMPONENT_SWIZZLE_B;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ALPHA:
		return VK_COMPONENT_SWIZZLE_A;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ZERO:
		return VK_COMPONENT_SWIZZLE_ZERO;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ONE:
		return VK_COMPONENT_SWIZZLE_ONE;
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRFBFoveationExtension

struct OpenXRFBFoveationExtension::Data
{
#ifdef VULKAN_ENABLED
	XrVulkanSwapchainCreateInfoMETA meta_vulkan_swapchain_create_info;
#endif
	String rendering_driver;
	bool fb_foveation_ext = false;
	bool fb_foveation_configuration_ext = false;
	bool fb_foveation_vulkan_ext = false;
	bool meta_foveation_eye_tracked_ext = false;
	bool meta_vulkan_swapchain_create_info_ext = false;
	XrFoveationLevelFB foveation_level = XR_FOVEATION_LEVEL_NONE_FB;
	XrFoveationDynamicFB foveation_dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
	bool foveation_with_subsampled_images_enabled = false;
	bool foveation_with_subsampled_images_active = false;
	XrSwapchainCreateInfoFoveationFB swapchain_create_info_foveation_fb;
	XrSystemFoveationEyeTrackedPropertiesMETA meta_foveation_eye_tracked_properties;
	XrFoveationEyeTrackedProfileCreateInfoMETA meta_foveation_eye_tracked_create_info;
};

void OpenXRFBFoveationExtension::initialize(const String& p_rendering_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->rendering_driver = p_rendering_driver;
}

void OpenXRFBFoveationExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRFBFoveationExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->fb_foveation_ext) {
		EXT_INIT_XR_FUNC(xrCreateFoveationProfileFB);
		EXT_INIT_XR_FUNC(xrDestroyFoveationProfileFB);
	}

	if (data->meta_foveation_eye_tracked_ext) {
		EXT_INIT_XR_FUNC(xrGetFoveationEyeTrackedStateMETA);
	}
}

void OpenXRFBFoveationExtension::on_instance_destroyed()
{
	data->fb_foveation_ext = false;
	data->fb_foveation_configuration_ext = false;
	data->meta_foveation_eye_tracked_ext = false;
}

bool OpenXRFBFoveationExtension::is_enabled()
{
	bool enabled = OpenXRFBUpdateSwapchainExtension::is_initialized() &&
				   OpenXRFBUpdateSwapchainExtension::is_enabled() && data->fb_foveation_ext &&
				   data->fb_foveation_configuration_ext;
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		enabled = enabled && data->fb_foveation_vulkan_ext;
	}
#endif
	return enabled;
}

void* OpenXRFBFoveationExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		data->meta_foveation_eye_tracked_properties.next = p_next_pointer;
		return &data->meta_foveation_eye_tracked_properties;
	}
#endif
	return p_next_pointer;
}

void* OpenXRFBFoveationExtension::set_swapchain_create_info_and_get_next_pointer(
	void* p_next_pointer)
{
	void* next = p_next_pointer;
	if (is_enabled() && data->foveation_level > 0) {
		data->swapchain_create_info_foveation_fb.next = next;
		next = &data->swapchain_create_info_foveation_fb;

#ifdef VULKAN_ENABLED
		if (data->meta_vulkan_swapchain_create_info_ext) {
			data->meta_vulkan_swapchain_create_info.additionalCreateFlags = 0;
			if (data->meta_foveation_eye_tracked_ext &&
				data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
			}
			if (data->foveation_with_subsampled_images_enabled &&
				data->foveation_with_subsampled_images_active) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
			}

			data->meta_vulkan_swapchain_create_info.next = next;
			next = &data->meta_vulkan_swapchain_create_info;
		}
#endif
	}

	return next;
}

void OpenXRFBFoveationExtension::on_main_swapchains_created() { update_profile(); }

XrFoveationLevelFB OpenXRFBFoveationExtension::get_foveation_level()
{
	return data ? data->foveation_level : XR_FOVEATION_LEVEL_NONE_FB;
}

void OpenXRFBFoveationExtension::set_foveation_level(XrFoveationLevelFB p_foveation_level)
{
	data->foveation_level = p_foveation_level;
	update_profile();
}

XrFoveationDynamicFB OpenXRFBFoveationExtension::get_foveation_dynamic()
{
	return data ? data->foveation_dynamic : XR_FOVEATION_DYNAMIC_DISABLED_FB;
}

void OpenXRFBFoveationExtension::set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic)
{
	data->foveation_dynamic = p_foveation_dynamic;
	update_profile();
}

bool OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled()
{
	return is_enabled() && data->meta_foveation_eye_tracked_ext &&
		   data->meta_vulkan_swapchain_create_info_ext &&
		   data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(bool p_enabled)
{
	data->foveation_with_subsampled_images_enabled = p_enabled;
}

bool OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled()
{
	return is_enabled() && data->foveation_level > 0 &&
		   data->meta_vulkan_swapchain_create_info_ext &&
		   data->foveation_with_subsampled_images_enabled;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_active(bool p_active)
{
	data->foveation_with_subsampled_images_active = p_active;
}

void OpenXRFBFoveationExtension::update_profile() {}

void OpenXRFBFoveationExtension::get_fragment_density_offsets(LocalVector<Vector2i>& r_offsets) {}

HashMap<String, bool*> OpenXRFBFoveationExtension::get_requested_extensions(XrVersion p_version)
{
	return HashMap<String, bool*>();
}
