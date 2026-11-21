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
#include "servers/rendering/renderer.h"

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
