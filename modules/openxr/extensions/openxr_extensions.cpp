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
// Merged from modules/openxr/extensions/openxr_interaction_profile_extension.cpp
// ============================================================================

#include "../openxr_api.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include "modules/openxr/extensions/openxr_extensions.h"

#ifndef XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME
#define XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME "XR_LOGITECH_mx_ink_stylus_interaction"
#endif

#ifndef XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME
#define XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_VALVE_frame_controller_interaction"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileExtension

OpenXRInteractionProfileExtension::OpenXRInteractionProfileExtension(ProfileKind p_kind)
	: kind(p_kind)
{
}

void OpenXRInteractionProfileExtension::initialize()
{
	if (data) {
		return;
	}
	data = memnew_arr(OpenXRInteractionProfileExtension, PROFILE_MAX);
	for (int i = 0; i < PROFILE_MAX; i++) {
		data[i] = OpenXRInteractionProfileExtension(static_cast<ProfileKind>(i));
	}
}

void OpenXRInteractionProfileExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete_arr(data);
	data = nullptr;
}

OpenXRInteractionProfileExtension* OpenXRInteractionProfileExtension::get_profile(
	ProfileKind p_kind)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, nullptr);
	return data ? &data[p_kind] : nullptr;
}

bool OpenXRInteractionProfileExtension::is_profile_available(ProfileKind p_kind, int p_subindex)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, false);
	return data != nullptr && data[p_kind].is_available(p_subindex);
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_all_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> result;
	if (!data) {
		return result;
	}
	for (int i = 0; i < PROFILE_MAX; i++) {
		HashMap<String, bool*> ext = data[i].get_requested_extensions(p_version);
		for (const KeyValue<String, bool*>& kv : ext) {
			result[kv.key] = kv.value;
		}
	}
	return result;
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_COSMOS];
			request_extensions[XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_FOCUS3];
		}
		request_extensions[XR_HTC_HAND_INTERACTION_EXTENSION_NAME] = &flags[HTC_HAND_INTERACTION];
	} break;
	case PROFILE_HTC_VIVE_TRACKER: {
		request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_HUAWEI_CONTROLLER: {
		request_extensions[XR_HUAWEI_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_KHR_GENERIC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_KHR_GENERIC_CONTROLLER_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_META_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME] = &flags[META_TOUCH_PRO];
			request_extensions[XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME] =
				&flags[META_TOUCH_PLUS];
		}
		request_extensions[XR_FB_TOUCH_CONTROLLER_PROXIMITY_EXTENSION_NAME] =
			&flags[META_TOUCH_PROXIMITY];
	} break;
	case PROFILE_ML2_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_ML_ML2_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_MXINK: {
		request_extensions[XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_PALM_POSE: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_PALM_POSE_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_PICO_CONTROLLER: {
		request_extensions[XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_VALVE_CONTROLLER: {
		request_extensions[XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_WMR_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_HPMR];
			request_extensions[XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_SAMSUNG_ODESSY];
		}
		request_extensions[XR_MSFT_HAND_INTERACTION_EXTENSION_NAME] = &flags[WMR_HAND_INTERACTION];
	} break;
	case PROFILE_HAND_INTERACTION: {
		request_extensions[XR_EXT_HAND_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_LOCAL_FLOOR: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_LOCAL_FLOOR_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_DPAD_BINDING: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_EXT_DPAD_BINDING_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_VALVE_ANALOG_THRESHOLD: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_VALVE_ANALOG_THRESHOLD_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_COMPOSITION_LAYER_DEPTH: {
		request_extensions[XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_MAX:
		break;
	}

	return request_extensions;
}

PackedStringArray OpenXRInteractionProfileExtension::get_suggested_tracker_names() const
{
	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		return PackedStringArray{
			"/user/hand_htc/left",
			"/user/hand_htc/right",
		};
	}
	case PROFILE_HTC_VIVE_TRACKER: {
		return PackedStringArray{
			"/user/vive_tracker_htcx/role/handheld_object",
			"/user/vive_tracker_htcx/role/left_foot",
			"/user/vive_tracker_htcx/role/right_foot",
			"/user/vive_tracker_htcx/role/left_shoulder",
			"/user/vive_tracker_htcx/role/right_shoulder",
			"/user/vive_tracker_htcx/role/left_elbow",
			"/user/vive_tracker_htcx/role/right_elbow",
			"/user/vive_tracker_htcx/role/left_knee",
			"/user/vive_tracker_htcx/role/right_knee",
			"/user/vive_tracker_htcx/role/waist",
			"/user/vive_tracker_htcx/role/chest",
			"/user/vive_tracker_htcx/role/camera",
			"/user/vive_tracker_htcx/role/keyboard",
			"/user/vive_tracker_htcx/role/left_wrist",
			"/user/vive_tracker_htcx/role/right_wrist",
			"/user/vive_tracker_htcx/role/left_ankle",
			"/user/vive_tracker_htcx/role/right_ankle",
		};
	}
	default:
		return PackedStringArray();
	}
}

bool OpenXRInteractionProfileExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	if (kind == PROFILE_HTC_VIVE_TRACKER &&
		p_event.type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX) {
		print_verbose("OpenXR EVENT: VIVE tracker connected");
		return true;
	}
	return false;
}

void OpenXRInteractionProfileExtension::on_register_metadata()
{
	if (kind != PROFILE_HAND_INTERACTION) {
		return;
	}

	const String profile_path = "/interaction_profiles/ext/hand_interaction_ext";
	OpenXRInteractionProfileMetadata::register_interaction_profile(
		"Hand interaction", profile_path, XR_EXT_HAND_INTERACTION_EXTENSION_NAME);
	for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip pose", user_path,
			user_path + "/input/grip/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim pose", user_path,
			user_path + "/input/aim/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch pose", user_path,
			user_path + "/input/pinch_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Poke pose", user_path,
			user_path + "/input/poke_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip surface pose",
			user_path, user_path + "/input/grip_surface/pose",
			XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
											"," XR_OPENXR_1_1_NAME,
			OpenXRAction::OPENXR_ACTION_POSE);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch", user_path,
			user_path + "/input/pinch_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch ready", user_path,
			user_path + "/input/pinch_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate", user_path,
			user_path + "/input/aim_activate_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate ready",
			user_path, user_path + "/input/aim_activate_ext/ready_ext", "",
			OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp", user_path,
			user_path + "/input/grasp_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp ready", user_path,
			user_path + "/input/grasp_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe left", user_path,
			user_path + "/input/swipe_left_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe right", user_path,
			user_path + "/input/swipe_right_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe forward", user_path,
			user_path + "/input/swipe_forward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe backward", user_path,
			user_path + "/input/swipe_backward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Tap thumb", user_path,
			user_path + "/input/tap_thumb_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
	}
}

bool OpenXRInteractionProfileExtension::is_available(int p_subindex) const
{
	if (kind == PROFILE_DPAD_BINDING || kind == PROFILE_VALVE_ANALOG_THRESHOLD) {
		return flags[0] && flags[1];
	}
	ERR_FAIL_INDEX_V(p_subindex, 3, false);
	return flags[p_subindex];
}

////////////////////////////////////////////////////////////////////////////
// OpenXRDpadBindingModifier

OpenXRDpadBindingModifier::OpenXRDpadBindingModifier()
{
	ERR_FAIL_COND(
		dpad_bindings_data.resize_initialized(sizeof(XrInteractionProfileDpadBindingEXT)) != OK);
	dpad_bindings = (XrInteractionProfileDpadBindingEXT*)dpad_bindings_data.ptrw();

	dpad_bindings->type = XR_TYPE_INTERACTION_PROFILE_DPAD_BINDING_EXT;
	dpad_bindings->next = nullptr;

	dpad_bindings->forceThreshold = 0.6;
	dpad_bindings->forceThresholdReleased = 0.4;
	dpad_bindings->centerRegion = 0.1;
	dpad_bindings->wedgeAngle = Math::deg_to_rad(90.0);
	dpad_bindings->isSticky = false;
}

void OpenXRDpadBindingModifier::set_action_set(const Ref<OpenXRActionSet>& p_action_set)
{
	action_set = p_action_set;
}

Ref<OpenXRActionSet> OpenXRDpadBindingModifier::get_action_set() const { return action_set; }

void OpenXRDpadBindingModifier::set_input_path(const String& p_input_path)
{
	input_path = p_input_path;
	emit_changed();
}

String OpenXRDpadBindingModifier::get_input_path() const { return input_path; }

void OpenXRDpadBindingModifier::set_threshold(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThreshold = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThreshold;
}

void OpenXRDpadBindingModifier::set_threshold_released(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThresholdReleased = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold_released() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThresholdReleased;
}

void OpenXRDpadBindingModifier::set_center_region(float p_center_region)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_center_region < 0.0 || p_center_region > 1.0);

	dpad_bindings->centerRegion = p_center_region;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_center_region() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->centerRegion;
}

void OpenXRDpadBindingModifier::set_wedge_angle(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = p_wedge_angle;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->wedgeAngle;
}

void OpenXRDpadBindingModifier::set_wedge_angle_deg(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = Math::deg_to_rad(p_wedge_angle);
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle_deg() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return Math::rad_to_deg(dpad_bindings->wedgeAngle);
}

void OpenXRDpadBindingModifier::set_is_sticky(bool p_sticky)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->isSticky = p_sticky;
	emit_changed();
}

bool OpenXRDpadBindingModifier::get_is_sticky() const
{
	ERR_FAIL_NULL_V(dpad_bindings, false);
	return dpad_bindings->isSticky;
}

void OpenXRDpadBindingModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_on_haptic() const { return on_haptic; }

void OpenXRDpadBindingModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRDpadBindingModifier::get_ip_modification()
{
	ERR_FAIL_NULL_V(dpad_bindings, PackedByteArray());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedByteArray());

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_DPAD_BINDING)) {
		WARN_PRINT("DPad binding extension is not enabled or available.");
		return PackedByteArray();
	}

	dpad_bindings->binding = OpenXRAPI::get_xr_path(input_path);
	ERR_FAIL_COND_V(dpad_bindings->binding == XR_NULL_PATH, PackedByteArray());

	ERR_FAIL_COND_V(action_set.is_null(), PackedByteArray());
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), PackedByteArray());
	dpad_bindings->actionSet = OpenXRAPI::action_set_get_handle(action_set_rid);

	dpad_bindings->onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	dpad_bindings->offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	return dpad_bindings_data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRAnalogThresholdModifier

OpenXRAnalogThresholdModifier::OpenXRAnalogThresholdModifier()
{
	analog_threshold.type = XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE;
	analog_threshold.next = nullptr;

	analog_threshold.onThreshold = 0.6;
	analog_threshold.offThreshold = 0.4;
}

void OpenXRAnalogThresholdModifier::set_on_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.onThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_on_threshold() const
{
	return analog_threshold.onThreshold;
}

void OpenXRAnalogThresholdModifier::set_off_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.offThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_off_threshold() const
{
	return analog_threshold.offThreshold;
}

void OpenXRAnalogThresholdModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_on_haptic() const { return on_haptic; }

void OpenXRAnalogThresholdModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRAnalogThresholdModifier::get_ip_modification()
{
	PackedByteArray ret;

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_VALVE_ANALOG_THRESHOLD)) {
		WARN_PRINT("Analog threshold extension is not enabled or available.");
		return ret;
	}

	ERR_FAIL_NULL_V(ip_binding, ret);

	Ref<OpenXRAction> action = ip_binding->get_action();
	ERR_FAIL_COND_V(action.is_null(), ret);

	Ref<OpenXRActionSet> action_set = action->get_action_set();
	ERR_FAIL_COND_V(action_set.is_null(), ret);
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), ret);

	RID action_rid = OpenXRAPI::find_action(action->get_name(), action_set_rid);
	ERR_FAIL_COND_V(!action_rid.is_valid(), ret);

	analog_threshold.action = OpenXRAPI::action_get_handle(action_rid);

	analog_threshold.binding = OpenXRAPI::get_xr_path(ip_binding->get_binding_path());
	ERR_FAIL_COND_V(analog_threshold.binding == XR_NULL_PATH, ret);

	analog_threshold.onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	analog_threshold.offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	ERR_FAIL_COND_V(ret.resize(sizeof(XrInteractionProfileAnalogThresholdVALVE)) != OK, ret);
	memcpy(ret.ptrw(), &analog_threshold, sizeof(XrInteractionProfileAnalogThresholdVALVE));

	return ret;
}

// ============================================================================
// Merged from modules/openxr/extensions/spatial_entities/openxr_spatial_entities.cpp
// ============================================================================

#include <cstring>
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
#include "core/config/project_settings.h"
#include "modules/openxr/extensions/openxr_extensions.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/xr/xr_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationBaseHeader

OpenXRSpatialCapabilityConfigurationBaseHeader::OpenXRSpatialCapabilityConfigurationBaseHeader(
	CapabilityType p_capability_type)
{
	set_capability_type(p_capability_type);
}

OpenXRSpatialCapabilityConfigurationBaseHeader::~OpenXRSpatialCapabilityConfigurationBaseHeader() {}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_capability_type(
	CapabilityType p_capability_type)
{
	capability_type = p_capability_type;
	enabled_components.clear();

	if (capability_type == CAPABILITY_ARUCO) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/aruco_dict");
		aruco_dict = (XrSpatialMarkerArucoDictEXT)(XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT + dict);
	}
	else if (capability_type == CAPABILITY_APRIL_TAG) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/april_tag_dict");
		april_dict =
			(XrSpatialMarkerAprilTagDictEXT)(XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT + dict);
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::has_valid_configuration() const
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);
		return OpenXRSpatialAnchorCapability::is_spatial_anchor_supported();
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), false);
		return OpenXRSpatialPlaneTrackingCapability::is_supported();
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported();
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported();
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_aruco_supported();
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported();
	}
	case CAPABILITY_NONE:
	default:
		return false;
	}
}

uint64_t OpenXRSpatialCapabilityConfigurationBaseHeader::_get_configurationgd()
{
	return (uint64_t)get_configuration();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationBaseHeader::get_configuration()
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT);
		if (OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT);
		}

		config.anchor = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ANCHOR_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.anchor;
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialPlaneTrackingCapability::is_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT);

		if (get_supports_mesh_2d()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT);
		}
		else if (get_supports_polygons()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT);
		}
		if (get_supports_labels()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT);
		}

		config.plane = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_PLANE_TRACKING_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.plane;
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.qrcode;
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.micro_qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_MICRO_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.micro_qrcode;
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.aruco = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ARUCO_MARKER_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			aruco_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.aruco;
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.april_tag = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_APRIL_TAG_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			april_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.april_tag;
	}
	case CAPABILITY_NONE:
	default:
		return nullptr;
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_mesh_2d()
{
	if (supports_mesh_2d == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_mesh_2d =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT)
				? 1
				: 0;
	}

	return supports_mesh_2d == 1;
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_polygons()
{
	if (supports_polygons == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_polygons =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT)
				? 1
				: 0;
	}

	return supports_polygons == 1;
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_labels()
{
	if (supports_labels == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_labels =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
				XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT)
				? 1
				: 0;
	}

	return supports_labels == 1;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_aruco_dict(
	XrSpatialMarkerArucoDictEXT p_dict)
{
	aruco_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_aruco_dict(ArucoDict p_dict)
{
	set_aruco_dict((XrSpatialMarkerArucoDictEXT)p_dict);
}

XrSpatialMarkerArucoDictEXT OpenXRSpatialCapabilityConfigurationBaseHeader::get_aruco_dict() const
{
	return aruco_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::ArucoDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_aruco_dict() const
{
	return (ArucoDict)get_aruco_dict();
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_april_dict(
	XrSpatialMarkerAprilTagDictEXT p_dict)
{
	april_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_april_dict(AprilTagDict p_dict)
{
	set_april_dict((XrSpatialMarkerAprilTagDictEXT)p_dict);
}

XrSpatialMarkerAprilTagDictEXT
OpenXRSpatialCapabilityConfigurationBaseHeader::get_april_dict() const
{
	return april_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::AprilTagDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_april_dict() const
{
	return (AprilTagDict)get_april_dict();
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationBaseHeader::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentData

OpenXRSpatialComponentData::OpenXRSpatialComponentData(ComponentKind p_kind)
	: component_kind(p_kind)
{
	_init_structure_header();
}

OpenXRSpatialComponentData::~OpenXRSpatialComponentData() { _free_buffers(); }

void OpenXRSpatialComponentData::_free_buffers()
{
	if (data_ptr != nullptr) {
		memfree(data_ptr);
		data_ptr = nullptr;
	}
	if (entity_states_ptr != nullptr) {
		memfree(entity_states_ptr);
		entity_states_ptr = nullptr;
	}
	capacity = 0;
}

void OpenXRSpatialComponentData::_init_structure_header()
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result = {
			XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT,
			nullptr,
			capacity,
			0,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
			capacity,
			0,
			entity_states_ptr,
		};
		break;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialBounded2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrBoxf*>(data_ptr),
		};
		break;
	case COMPONENT_PARENT:
		structure.parent_list = {
			XR_TYPE_SPATIAL_COMPONENT_PARENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_ANCHOR:
		structure.anchor_list = {
			XR_TYPE_SPATIAL_COMPONENT_ANCHOR_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrPosef*>(data_ptr),
		};
		break;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list = {
			XR_TYPE_SPATIAL_COMPONENT_PERSISTENCE_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPersistenceDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_ALIGNMENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneAlignmentEXT*>(data_ptr),
		};
		break;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_POLYGON_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPolygon2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_SEMANTIC_LABEL_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneSemanticLabelEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MARKER:
		structure.marker_list = {
			XR_TYPE_SPATIAL_COMPONENT_MARKER_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMarkerDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_NONE:
	default:
		break;
	}
}

void OpenXRSpatialComponentData::set_component_kind(ComponentKind p_kind)
{
	if (component_kind == p_kind) {
		return;
	}
	_free_buffers();
	component_kind = p_kind;
	_init_structure_header();
}

void OpenXRSpatialComponentData::set_capacity(uint32_t p_capacity)
{
	if (component_kind == COMPONENT_NONE) {
		_free_buffers();
		return;
	}

	if (p_capacity == 0) {
		_free_buffers();
		_init_structure_header();
		return;
	}

	size_t elem_size = 0;
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_BOUNDED_2D:
		elem_size = sizeof(XrSpatialBounded2DDataEXT);
		break;
	case COMPONENT_BOUNDED_3D:
		elem_size = sizeof(XrBoxf);
		break;
	case COMPONENT_PARENT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		elem_size = sizeof(XrSpatialMeshDataEXT);
		break;
	case COMPONENT_ANCHOR:
		elem_size = sizeof(XrPosef);
		break;
	case COMPONENT_PERSISTENCE:
		elem_size = sizeof(XrSpatialPersistenceDataEXT);
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		elem_size = sizeof(XrSpatialPlaneAlignmentEXT);
		break;
	case COMPONENT_POLYGON_2D:
		elem_size = sizeof(XrSpatialPolygon2DDataEXT);
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		elem_size = sizeof(XrSpatialPlaneSemanticLabelEXT);
		break;
	case COMPONENT_MARKER:
		elem_size = sizeof(XrSpatialMarkerDataEXT);
		break;
	case COMPONENT_NONE:
	default:
		return;
	}

	uint32_t old_capacity = capacity;
	data_ptr = memrealloc(data_ptr, elem_size * p_capacity);
	ERR_FAIL_NULL(data_ptr);
	if (p_capacity > old_capacity) {
		memset(static_cast<uint8_t*>(data_ptr) + elem_size * old_capacity, 0,
			elem_size * (p_capacity - old_capacity));
	}

	if (component_kind == COMPONENT_QUERY_RESULT) {
		entity_states_ptr = static_cast<XrSpatialEntityTrackingStateEXT*>(memrealloc(
			entity_states_ptr, sizeof(XrSpatialEntityTrackingStateEXT) * p_capacity));
		ERR_FAIL_NULL(entity_states_ptr);
		if (p_capacity > old_capacity) {
			memset(entity_states_ptr + old_capacity, 0,
				sizeof(XrSpatialEntityTrackingStateEXT) * (p_capacity - old_capacity));
		}
	}

	capacity = p_capacity;
	_init_structure_header();
}

int64_t OpenXRSpatialComponentData::_get_component_typegd() const
{
	return (int64_t)get_component_type();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentData::get_component_type() const
{
	switch (component_kind) {
	case COMPONENT_BOUNDED_2D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT;
	case COMPONENT_BOUNDED_3D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT;
	case COMPONENT_PARENT:
		return XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT;
	case COMPONENT_MESH_2D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT;
	case COMPONENT_MESH_3D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT;
	case COMPONENT_ANCHOR:
		return XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT;
	case COMPONENT_PERSISTENCE:
		return XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT;
	case COMPONENT_PLANE_ALIGNMENT:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT;
	case COMPONENT_POLYGON_2D:
		return XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT;
	case COMPONENT_MARKER:
		return XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT;
	case COMPONENT_QUERY_RESULT:
	case COMPONENT_NONE:
	default:
		return XR_SPATIAL_COMPONENT_TYPE_MAX_ENUM_EXT;
	}
}

void* OpenXRSpatialComponentData::get_structure_data(void* p_next)
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result.next = p_next;
		structure.query_result.entityIdCountOutput = 0;
		structure.query_result.entityStateCountOutput = 0;
		return &structure.query_result;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list.next = p_next;
		return &structure.bounded2d_list;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list.next = p_next;
		return &structure.bounded3d_list;
	case COMPONENT_PARENT:
		structure.parent_list.next = p_next;
		return &structure.parent_list;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list.next = p_next;
		return &structure.mesh2d_list;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list.next = p_next;
		return &structure.mesh3d_list;
	case COMPONENT_ANCHOR:
		structure.anchor_list.next = p_next;
		return &structure.anchor_list;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list.next = p_next;
		return &structure.persistence_list;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list.next = p_next;
		return &structure.plane_alignment_list;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list.next = p_next;
		return &structure.polygon2d_list;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list.next = p_next;
		return &structure.plane_semantic_label_list;
	case COMPONENT_MARKER:
		structure.marker_list.next = p_next;
		return &structure.marker_list;
	case COMPONENT_NONE:
	default:
		return nullptr;
	}
}

XrSpatialEntityIdEXT OpenXRSpatialComponentData::get_entity_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_QUERY_RESULT, XR_NULL_ENTITY);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_NULL_ENTITY);

	return static_cast<const XrSpatialEntityIdEXT*>(data_ptr)[p_index];
}

uint64_t OpenXRSpatialComponentData::_get_entity_id(int64_t p_index) const
{
	return (uint64_t)get_entity_id(p_index);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialComponentData::get_entity_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_QUERY_RESULT, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);

	return entity_states_ptr[p_index];
}

String OpenXRSpatialComponentData::get_entity_tracking_state_name(
	XrSpatialEntityTrackingStateEXT p_tracking_state)
{
	XR_ENUM_SWITCH(XrSpatialEntityTrackingStateEXT, p_tracking_state)
}

Transform3D OpenXRSpatialComponentData::get_center_pose(int64_t p_index) const
{
	if (component_kind == COMPONENT_BOUNDED_2D) {
		return get_center_pose_2d(p_index);
	}
	if (component_kind == COMPONENT_BOUNDED_3D) {
		return get_center_pose_3d(p_index);
	}
	ERR_FAIL_V(Transform3D());
}

Transform3D OpenXRSpatialComponentData::get_center_pose_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrSpatialBounded2DDataEXT* arr =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Transform3D OpenXRSpatialComponentData::get_center_pose_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrBoxf* arr = static_cast<const XrBoxf*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Vector2 OpenXRSpatialComponentData::get_size_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Vector2());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector2());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector2());

	const XrExtent2Df& extents =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr)[p_index].extents;
	return Vector2(extents.width, extents.height);
}

Vector3 OpenXRSpatialComponentData::get_size_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Vector3());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector3());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector3());

	const XrExtent3Df& extents = static_cast<const XrBoxf*>(data_ptr)[p_index].extents;
	return Vector3(extents.width, extents.height, extents.depth);
}

RID OpenXRSpatialComponentData::get_parent(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PARENT, RID());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, RID());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), RID());

	const XrSpatialEntityIdEXT* arr = static_cast<const XrSpatialEntityIdEXT*>(data_ptr);
	return OpenXRSpatialEntityExtension::find_spatial_entity(arr[p_index]);
}

Transform3D OpenXRSpatialComponentData::get_transform(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	switch (component_kind) {
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].origin);
	case COMPONENT_POLYGON_2D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].origin);
	default:
		ERR_FAIL_V(Transform3D());
	}
}

PackedVector2Array OpenXRSpatialComponentData::get_vertices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedVector2Array());

	const XrSpatialBufferEXT* buffer = nullptr;
	if (component_kind == COMPONENT_MESH_2D) {
		buffer = &static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else if (component_kind == COMPONENT_POLYGON_2D) {
		buffer = &static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else {
		ERR_FAIL_V(PackedVector2Array());
	}

	if (buffer->bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedVector2Array();
	}

	ERR_FAIL_COND_V(
		buffer->bufferType != XR_SPATIAL_BUFFER_TYPE_VECTOR2F_EXT, PackedVector2Array());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedVector2Array());

	return OpenXRSpatialEntityExtension::get_vector2_buffer(p_snapshot, buffer->bufferId);
}

PackedInt32Array OpenXRSpatialComponentData::get_indices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_2D, PackedInt32Array());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedInt32Array());

	const XrSpatialBufferEXT& buffer =
		static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].indexBuffer;
	if (buffer.bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedInt32Array();
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedInt32Array());

	PackedInt32Array ret;

	switch (buffer.bufferType) {
	case XR_SPATIAL_BUFFER_TYPE_UINT8_EXT: {
		PackedByteArray data =
			OpenXRSpatialEntityExtension::get_uint8_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT16_EXT: {
		Vector<uint16_t> data =
			OpenXRSpatialEntityExtension::get_uint16_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT32_EXT: {
		Vector<uint32_t> data =
			OpenXRSpatialEntityExtension::get_uint32_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(PackedInt32Array(), "OpenXR: Unsupported buffer type for indices.");
	} break;
	}

	return ret;
}

Ref<Mesh> OpenXRSpatialComponentData::get_mesh(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_3D, nullptr);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, nullptr);

	// TODO implement, need to convert mesh data to Godot mesh resource
	return nullptr;
}

Transform3D OpenXRSpatialComponentData::get_entity_pose(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_ANCHOR, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	return OpenXRAPI::transform_from_pose(static_cast<const XrPosef*>(data_ptr)[p_index]);
}

XrUuid OpenXRSpatialComponentData::get_persistent_uuid(int64_t p_index) const
{
	XrUuid null_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	ERR_FAIL_COND_V(component_kind != COMPONENT_PERSISTENCE, null_uuid);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_uuid);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistUuid;
}

String OpenXRSpatialComponentData::_get_persistent_uuid(int64_t p_index) const
{
	return OpenXRUtil::string_from_xruuid(get_persistent_uuid(p_index));
}

XrSpatialPersistenceStateEXT OpenXRSpatialComponentData::get_persistent_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PERSISTENCE, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistState;
}

uint64_t OpenXRSpatialComponentData::_get_persistent_state(int64_t p_index) const
{
	return (uint64_t)get_persistent_state(p_index);
}

String OpenXRSpatialComponentData::get_persistence_state_name(XrSpatialPersistenceStateEXT p_state)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceStateEXT, p_state)
}

XrSpatialPlaneAlignmentEXT OpenXRSpatialComponentData::get_plane_alignment(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PLANE_ALIGNMENT, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneAlignmentEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialComponentData::_get_plane_alignment(
	int64_t p_index) const
{
	return (PlaneAlignment)get_plane_alignment(p_index);
}

XrSpatialPlaneSemanticLabelEXT OpenXRSpatialComponentData::get_plane_semantic_label(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PLANE_SEMANTIC_LABEL,
		XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneSemanticLabelEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneSemanticLabel
OpenXRSpatialComponentData::_get_plane_semantic_label(int64_t p_index) const
{
	return (PlaneSemanticLabel)get_plane_semantic_label(p_index);
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialComponentData::get_marker_type(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, MARKER_TYPE_UNKNOWN);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, MARKER_TYPE_UNKNOWN);

	switch (static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].capability) {
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT:
		return MARKER_TYPE_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT:
		return MARKER_TYPE_MICRO_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT:
		return MARKER_TYPE_ARUCO;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT:
		return MARKER_TYPE_APRIL_TAG;
	default:
		return MARKER_TYPE_UNKNOWN;
	}
}

uint32_t OpenXRSpatialComponentData::get_marker_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, 0);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, 0);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].markerId;
}

XrSpatialBufferEXT OpenXRSpatialComponentData::get_marker_data(int64_t p_index) const
{
	XrSpatialBufferEXT null_buffer = {
		XR_NULL_SPATIAL_BUFFER_ID_EXT, XR_SPATIAL_BUFFER_TYPE_UNKNOWN_EXT};

	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, null_buffer);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_buffer);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityTracker

OpenXRSpatialEntityTracker::OpenXRSpatialEntityTracker(EntityType p_entity_type)
	: entity_type(p_entity_type)
{
	xr_tracker.instantiate(XRServer::TrackerType::TRACKER_ANCHOR);
}

OpenXRSpatialEntityTracker::~OpenXRSpatialEntityTracker()
{
	if (spatial_entity.is_valid()) {
		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}
}

void OpenXRSpatialEntityTracker::set_spatial_context(const RID& p_spatial_context)
{
	// Trackers shouldn't be switching spatial contexts; always create a new tracker for new
	// contexts
	ERR_FAIL_COND(spatial_context.is_valid() && spatial_context != p_spatial_context);
	spatial_context = p_spatial_context;
}

RID OpenXRSpatialEntityTracker::get_spatial_context() const { return spatial_context; }

void OpenXRSpatialEntityTracker::set_entity(const RID& p_entity)
{
	if (spatial_entity.is_valid()) {
		if (spatial_entity == p_entity) {
			return;
		}

		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}

	spatial_entity = p_entity;

	if (p_entity.is_valid()) {
		ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

		XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(p_entity);

		String tracker_name = String("openxr/spatial_entity/") + String::num_int64(entity_id);
		set_tracker_name(tracker_name);
	}
	else {
		set_tracker_name("openxr/spatial_entity/null");
	}
}

RID OpenXRSpatialEntityTracker::get_entity() const { return spatial_entity; }

void OpenXRSpatialEntityTracker::set_spatial_tracking_state(
	const XrSpatialEntityTrackingStateEXT p_state)
{
	if (spatial_tracking_state != p_state) {
		spatial_tracking_state = p_state;
	}
}

void OpenXRSpatialEntityTracker::_set_spatial_tracking_state(const EntityTrackingState p_state)
{
	set_spatial_tracking_state((XrSpatialEntityTrackingStateEXT)p_state);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialEntityTracker::get_spatial_tracking_state() const
{
	return spatial_tracking_state;
}

OpenXRSpatialEntityTracker::EntityTrackingState
OpenXRSpatialEntityTracker::_get_spatial_tracking_state() const
{
	return (EntityTrackingState)get_spatial_tracking_state();
}

void OpenXRSpatialEntityTracker::add_next(Ref<OpenXRStructureBase> p_next)
{
	// Prepend p_next to next
	if (p_next.is_valid()) {
		if (next.is_valid()) {
			p_next->set_next(next);
		}

		next = p_next;
	}
}

void OpenXRSpatialEntityTracker::remove_next(Ref<OpenXRStructureBase> p_next)
{
	if (p_next.is_null()) {
		return;
	}

	Ref<OpenXRStructureBase> prev;
	for (Ref<OpenXRStructureBase> n = next; n.is_valid(); n = n->get_next()) {
		if (n == p_next) {
			if (prev.is_null()) {
				next = p_next->get_next();
			}
			else {
				prev->set_next(p_next->get_next());
			}

			p_next->set_next(nullptr);
			break;
		}

		prev = n;
	}
}

Ref<OpenXRStructureBase> OpenXRSpatialEntityTracker::get_next() const { return next; }

bool OpenXRSpatialEntityTracker::has_uuid() const
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (uuid.data[i] != 0) {
			return true;
		}
	}

	return false;
}

XrUuid OpenXRSpatialEntityTracker::get_uuid() const { return uuid; }

void OpenXRSpatialEntityTracker::set_uuid(const XrUuid& p_uuid)
{
	if (uuid_is_equal(uuid, p_uuid)) {
		return;
	}

	uuid = p_uuid;
}

String OpenXRSpatialEntityTracker::_get_uuid() const { return OpenXRUtil::string_from_xruuid(uuid); }

void OpenXRSpatialEntityTracker::_set_uuid(const String& p_uuid)
{
	set_uuid(OpenXRUtil::xruuid_from_string(p_uuid));
}

bool OpenXRSpatialEntityTracker::uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b)
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (p_a.data[i] != p_b.data[i]) {
			return false;
		}
	}

	return true;
}

void OpenXRSpatialEntityTracker::set_bounds_size(const Vector2& p_bounds_size)
{
	if (entity_type == ENTITY_PLANE) {
		if (Math::abs(bounds_size.x - p_bounds_size.x) > 0.001 ||
			Math::abs(bounds_size.y - p_bounds_size.y) > 0.001) {
			bounds_size = p_bounds_size;

			if (!mesh.has_mesh_data) {
				// Bounds changing only effects mesh data if we don't have polygon data.
				clear_mesh_data();
			}
		}
	}
	else {
		bounds_size = p_bounds_size;
	}
}

Vector2 OpenXRSpatialEntityTracker::get_bounds_size() const { return bounds_size; }

void OpenXRSpatialEntityTracker::set_plane_alignment(
	OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment)
{
	if (plane_alignment != p_plane_alignment) {
		plane_alignment = p_plane_alignment;
	}
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialEntityTracker::get_plane_alignment() const
{
	return plane_alignment;
}

void OpenXRSpatialEntityTracker::set_plane_label(const String& p_plane_label)
{
	if (plane_label != p_plane_label) {
		plane_label = p_plane_label;

		// Also copy to description, should do something nicer here.
		set_tracker_desc(plane_label);
	}
}

String OpenXRSpatialEntityTracker::get_plane_label() const { return plane_label; }

void OpenXRSpatialEntityTracker::set_mesh_data(const Transform3D& p_origin,
	const PackedVector2Array& p_vertices, const PackedInt32Array& p_indices)
{
	if (p_vertices.size() < 3) {
		if (mesh.has_mesh_data) {
			clear_mesh_data();
		}
	}
	else {
		bool has_changed = !mesh.has_mesh_data;

		mesh.has_mesh_data = true;
		mesh.origin = p_origin;

		if (mesh.vertices.size() != p_vertices.size()) {
			has_changed = true;
		}
		else {
			// Compare the vertices with a bit of margin, we ignore small jittering on vertices.
			for (uint32_t i = 0; i < p_vertices.size() && !has_changed; i++) {
				const Vector2& a = p_vertices[i];
				const Vector2& b = mesh.vertices[i];
				has_changed = (Math::abs(a.x - b.x) > 0.001) || (Math::abs(a.y - b.y) > 0.001);
			}
		}
		if (has_changed) {
			mesh.vertices = p_vertices;
		}

		if (p_indices.is_empty()) {
			// Assume polygon, turn into triangle strip...
			int count = (p_vertices.size() - 2) * 3;

			if (has_changed || mesh.indices.size() != count) {
				has_changed = true;

				int offset = 1;
				mesh.indices.resize(count);
				int32_t* idx = mesh.indices.ptrw();
				for (int i = 0; i < count; i += 3) {
					idx[i + 0] = 0;
					idx[i + 2] = offset++;
					idx[i + 1] = offset;
				}
			}
		}
		else {
			if (mesh.indices.size() != p_indices.size()) {
				has_changed = true;
			}
			else {
				for (uint32_t i = 0; i < p_indices.size() && !has_changed; i++) {
					has_changed = mesh.indices[i] != p_indices[i];
				}
			}
			if (has_changed) {
				mesh.indices = p_indices;
			}
		}

		if (has_changed) {
			mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
			mesh.shape3d.unref();
#endif
		}
	}
}

void OpenXRSpatialEntityTracker::clear_mesh_data()
{
	mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
	mesh.shape3d.unref();
#endif

	if (mesh.has_mesh_data) {
		mesh.has_mesh_data = false;
		mesh.origin = Transform3D();
		mesh.vertices.clear();
		mesh.indices.clear();
	}
}

Transform3D OpenXRSpatialEntityTracker::get_mesh_offset() const
{
	Transform3D offset;

	if (mesh.has_mesh_data) {
		offset = mesh.origin;

		Ref<XRPose> pose = get_pose(SNAME("default"));
		if (pose.is_valid()) {
			offset = pose->get_transform().inverse() * offset;
		}

		if (XRServer::is_initialized()) {
			offset.origin *= XRServer::get_world_scale();
		}
	}

	return offset;
}

#ifndef PHYSICS_3D_DISABLED
Ref<Shape3D> OpenXRSpatialEntityTracker::get_shape(real_t p_thickness)
{
	if (mesh.shape3d.is_valid()) {
		return mesh.shape3d;
	}

	if (mesh.has_mesh_data) {
		Ref<Shape3D> shape;
		Vector<Vector3> faces;

		int isize = mesh.indices.size();
		const Vector2* vr = mesh.vertices.ptr();
		const int32_t* ir = mesh.indices.ptr();

		HashMap<Edge, int, Edge> edge_counts;
		for (int i = 0; i < isize; i += 3) {
			for (int j = 0; j < 3; j++) {
				Edge e(ir[i + j], ir[i + ((j + 1) % 3)]);
				edge_counts[e]++;
			}
		}

		thread_local LocalVector<Edge> outer_edges;
		outer_edges.clear();
		for (const KeyValue<Edge, int>& e : edge_counts) {
			if (e.value > 1) {
				outer_edges.push_back(e.key);
			}
		}

		faces.resize(2 * isize + 6 * outer_edges.size());
		Vector3* write = faces.ptrw();

		for (int i = 0; i < isize; i += 3) {
			Vector3 a = Vector3(vr[ir[i]].x, vr[ir[i]].y, 0.0);
			Vector3 b = Vector3(vr[ir[i + 1]].x, vr[ir[i + 1]].y, 0.0);
			Vector3 c = Vector3(vr[ir[i + 2]].x, vr[ir[i + 2]].y, 0.0);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			a.z = -p_thickness;
			b.z = -p_thickness;
			c.z = -p_thickness;

			*write++ = a;
			*write++ = c;
			*write++ = b;
		}

		for (const Edge& edge : outer_edges) {
			Vector3 a = Vector3(vr[edge.a].x, vr[edge.a].y, 0.0);
			Vector3 b = Vector3(vr[edge.b].x, vr[edge.b].y, 0.0);
			Vector3 c = b + Vector3(0.0, 0.0, -p_thickness);
			Vector3 d = a + Vector3(0.0, 0.0, -p_thickness);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			*write++ = a;
			*write++ = c;
			*write++ = d;
		}

		shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
		shape->set_faces(faces);

		mesh.shape3d = shape;
	}
	else if (bounds_size.x > 0.0 && bounds_size.y > 0.0) {
		Ref<Shape3D> box_shape = memnew(Shape3D(Shape3D::SHAPE_BOX));
		box_shape->set_size(Vector3(bounds_size.x, bounds_size.y, p_thickness));

		mesh.shape3d = box_shape;
	}

	return mesh.shape3d;
}
#endif // PHYSICS_3D_DISABLED

void OpenXRSpatialEntityTracker::set_marker_type(
	OpenXRSpatialComponentData::MarkerType p_marker_type)
{
	marker_type = p_marker_type;
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialEntityTracker::get_marker_type() const
{
	return marker_type;
}

void OpenXRSpatialEntityTracker::set_marker_id(uint32_t p_id) { marker_id = p_id; }

uint32_t OpenXRSpatialEntityTracker::get_marker_id() const { return marker_id; }


////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityExtension

void OpenXRSpatialEntityExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialEntityExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialEntityExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled")) {
		request_extensions[XR_EXT_SPATIAL_ENTITY_EXTENSION_NAME] = &data->spatial_entity_ext;
	}

	return request_extensions;
}

void OpenXRSpatialEntityExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->spatial_entity_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilitiesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityComponentTypesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityFeaturesEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialContextEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotCompleteEXT);
		EXT_INIT_XR_FUNC(xrQuerySpatialComponentDataEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialSnapshotEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialEntityFromIdEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialEntityEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialUpdateSnapshotEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferStringEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint8EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint16EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint32EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferFloatEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector2fEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector3fEXT);
	}
}

void OpenXRSpatialEntityExtension::on_instance_destroyed()
{
	data->supported_capabilities.clear();
	data->capabilities_load_state = 0;

	xrEnumerateSpatialCapabilitiesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityComponentTypesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityFeaturesEXT_ptr = nullptr;
	xrCreateSpatialContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialContextEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotAsyncEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotCompleteEXT_ptr = nullptr;
	xrQuerySpatialComponentDataEXT_ptr = nullptr;
	xrDestroySpatialSnapshotEXT_ptr = nullptr;
	xrCreateSpatialEntityFromIdEXT_ptr = nullptr;
	xrDestroySpatialEntityEXT_ptr = nullptr;
	xrCreateSpatialUpdateSnapshotEXT_ptr = nullptr;
	xrGetSpatialBufferStringEXT_ptr = nullptr;
	xrGetSpatialBufferUint8EXT_ptr = nullptr;
	xrGetSpatialBufferUint16EXT_ptr = nullptr;
	xrGetSpatialBufferUint32EXT_ptr = nullptr;
	xrGetSpatialBufferFloatEXT_ptr = nullptr;
	xrGetSpatialBufferVector2fEXT_ptr = nullptr;
	xrGetSpatialBufferVector3fEXT_ptr = nullptr;
}

bool OpenXRSpatialEntityExtension::get_active()
{
	return data != nullptr && data->spatial_entity_ext;
}

bool OpenXRSpatialEntityExtension::_load_capabilities()
{
	if (data->capabilities_load_state == 0) {
		if (!data->spatial_entity_ext) {
			return false;
		}

		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		XrInstance instance = OpenXRAPI::get_instance();
		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);
		XrSystemId system_id = OpenXRAPI::get_system_id();
		ERR_FAIL_COND_V(system_id == 0, false);

		data->capabilities_load_state = 2;

		Vector<XrSpatialCapabilityEXT> capabilities;
		uint32_t capability_size = 0;
		XrResult result =
			xrEnumerateSpatialCapabilitiesEXT(instance, system_id, 0, &capability_size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capability count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (capability_size > 0) {
			capabilities.resize(capability_size);
			result = xrEnumerateSpatialCapabilitiesEXT(
				instance, system_id, capabilities.size(), &capability_size, capabilities.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capabilities [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}

			for (const XrSpatialCapabilityEXT& capability : capabilities) {
				print_verbose("OpenXR: Found spatial entity capability " +
							  get_spatial_capability_name(capability) + ".");

				SpatialEntityCapabality& spatial_entity_capability =
					data->supported_capabilities[capability];

				XrSpatialCapabilityComponentTypesEXT component_types = {
					XR_TYPE_SPATIAL_CAPABILITY_COMPONENT_TYPES_EXT,
					nullptr,
					0,
					0,
					nullptr,
				};
				result = xrEnumerateSpatialCapabilityComponentTypesEXT(
					instance, system_id, capability, &component_types);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity component type count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (component_types.componentTypeCountOutput > 0) {
					spatial_entity_capability.component_types.resize(
						component_types.componentTypeCountOutput);
					component_types.componentTypeCapacityInput =
						spatial_entity_capability.component_types.size();
					component_types.componentTypeCountOutput = 0;
					component_types.componentTypes =
						spatial_entity_capability.component_types.ptrw();
					result = xrEnumerateSpatialCapabilityComponentTypesEXT(
						instance, system_id, capability, &component_types);
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity component types [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialComponentTypeEXT& component_type :
							spatial_entity_capability.component_types) {
							print_verbose("- component type " +
										  get_spatial_component_type_name(component_type));
						}
					}
				}

				result = xrEnumerateSpatialCapabilityFeaturesEXT(
					instance, system_id, capability, 0, &capability_size, nullptr);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity feature count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (capability_size > 0) {
					spatial_entity_capability.features.resize(capability_size);
					result = xrEnumerateSpatialCapabilityFeaturesEXT(instance, system_id,
						capability, spatial_entity_capability.features.size(), &capability_size,
						spatial_entity_capability.features.ptrw());
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity features [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialCapabilityFeatureEXT& feature :
							spatial_entity_capability.features) {
							print_verbose("- feature " + get_spatial_feature_name(feature));
						}
					}
				}
			}
		}

		data->capabilities_load_state = 1;
	}

	return data->capabilities_load_state == 1;
}

bool OpenXRSpatialEntityExtension::supports_capability(XrSpatialCapabilityEXT p_capability)
{
	if (!_load_capabilities()) {
		return false;
	}

	return data->supported_capabilities.has(p_capability);
}

bool OpenXRSpatialEntityExtension::_supports_capability(Capability p_capability)
{
	return supports_capability((XrSpatialCapabilityEXT)p_capability);
}

bool OpenXRSpatialEntityExtension::supports_component_type(
	XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type)
{
	if (!_load_capabilities()) {
		return false;
	}

	if (data->supported_capabilities.has(p_capability)) {
		return data->supported_capabilities[p_capability].component_types.has(p_component_type);
	}
	return false;
}

bool OpenXRSpatialEntityExtension::_supports_component_type(
	Capability p_capability, ComponentType p_component_type)
{
	return supports_component_type(
		(XrSpatialCapabilityEXT)p_capability, (XrSpatialComponentTypeEXT)p_component_type);
}

bool OpenXRSpatialEntityExtension::get_spatial_context_ready(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, false);

	return context_data->spatial_context != XR_NULL_HANDLE;
}

void OpenXRSpatialEntityExtension::free_spatial_context(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL(context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (context_data->spatial_context != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialContextEXT(context_data->spatial_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		context_data->spatial_context = XR_NULL_HANDLE;

		data->spatial_context_owner.free(p_spatial_context);
	}
}

XrSpatialContextEXT OpenXRSpatialEntityExtension::get_spatial_context_handle(
	RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, XR_NULL_HANDLE);

	return context_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_context_handle(RID p_spatial_context)
{
	return (uint64_t)get_spatial_context_handle(p_spatial_context);
}

RID OpenXRSpatialEntityExtension::update_spatial_entities(RID p_spatial_context,
	const LocalVector<RID>& p_entities,
	const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
	Ref<OpenXRStructureBase> p_next)
{
	if (!get_active()) {
		return RID();
	}

	ERR_FAIL_COND_V(p_entities.is_empty(), RID());

	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, RID());

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	thread_local LocalVector<XrSpatialEntityEXT> entities;

	entities.resize(p_entities.size());
	XrSpatialEntityEXT* ptr = entities.ptr();
	for (const RID& rid : p_entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(rid);
		*ptr = entity_data ? entity_data->entity : XR_NULL_HANDLE;
		ptr++;
	}

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	SpatialSnapshotData spatial_snapshot_data;
	spatial_snapshot_data.spatial_context = p_spatial_context;

	XrSpatialUpdateSnapshotCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_UPDATE_SNAPSHOT_CREATE_INFO_EXT,
		next,
		(uint32_t)entities.size(),
		entities.ptr(),
		(uint32_t)p_component_types.size(),
		p_component_types.is_empty() ? nullptr : p_component_types.ptr(),
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
	};
	XrResult result = xrCreateSpatialUpdateSnapshotEXT(
		context_data->spatial_context, &create_info, &spatial_snapshot_data.spatial_snapshot);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create update snapshot [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_snapshot_owner.make_rid(spatial_snapshot_data);
}

void OpenXRSpatialEntityExtension::free_spatial_snapshot(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL(snapshot_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (snapshot_data->spatial_snapshot != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialSnapshotEXT(snapshot_data->spatial_snapshot);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial snapshot [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		snapshot_data->spatial_snapshot = XR_NULL_HANDLE;
	}

	data->spatial_snapshot_owner.free(p_spatial_snapshot);
}

XrSpatialSnapshotEXT OpenXRSpatialEntityExtension::get_spatial_snapshot_handle(
	RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, XR_NULL_HANDLE);

	return snapshot_data->spatial_snapshot;
}

RID OpenXRSpatialEntityExtension::get_spatial_snapshot_context(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, RID());

	return snapshot_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_snapshot_handle(RID p_spatial_snapshot)
{
	return (uint64_t)get_spatial_snapshot_handle(p_spatial_snapshot);
}

String OpenXRSpatialEntityExtension::get_string(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	String ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferStringEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	LocalVector<char> buffer;
	buffer.resize(count + 1);
	buffer[count] = '\0';

	result = xrGetSpatialBufferStringEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret = String::utf8(buffer.ptr());
	return ret;
}

PackedByteArray OpenXRSpatialEntityExtension::get_uint8_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedByteArray ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint8EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint8EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (uint8_t*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedByteArray(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint16_t> OpenXRSpatialEntityExtension::get_uint16_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint16_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint16EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint16EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint16_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint32_t> OpenXRSpatialEntityExtension::get_uint32_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint32_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint32EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint32EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint32_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::get_float_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedFloat32Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferFloatEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferFloatEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedFloat32Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedVector2Array OpenXRSpatialEntityExtension::get_vector2_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector2Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector2fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector2f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector2* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector2f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector2Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

PackedVector3Array OpenXRSpatialEntityExtension::get_vector3_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector3Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector3fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector3f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector3* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
		ptr[i].z = buffer[i].z;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector3f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector3Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

String OpenXRSpatialEntityExtension::_get_string(RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_string(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedByteArray OpenXRSpatialEntityExtension::_get_uint8_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_uint8_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint16_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint16_t> buffer =
		get_uint16_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint32_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint32_t> buffer =
		get_uint32_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::_get_float_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_float_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector2Array OpenXRSpatialEntityExtension::_get_vector2_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector2_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector3Array OpenXRSpatialEntityExtension::_get_vector3_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector3_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

RID OpenXRSpatialEntityExtension::find_spatial_entity(XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	LocalVector<RID> entities = data->spatial_entity_owner.get_owned_list();
	for (const RID& entity : entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(entity);
		ERR_FAIL_NULL_V(entity_data, RID());

		if (entity_data->entity_id == p_entity_id) {
			return entity;
		}
	}

	return RID();
}

RID OpenXRSpatialEntityExtension::_find_entity(uint64_t p_entity_id)
{
	return find_spatial_entity((XrSpatialEntityIdEXT)p_entity_id);
}

RID OpenXRSpatialEntityExtension::add_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	spatial_entity_data.entity = p_entity;

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_add_entity(
	RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity)
{
	return add_spatial_entity(
		p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id, (XrSpatialEntityEXT)p_entity);
}

RID OpenXRSpatialEntityExtension::make_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	XrSpatialEntityFromIdCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ENTITY_FROM_ID_CREATE_INFO_EXT,
		nullptr,
		p_entity_id,
	};
	XrResult result = xrCreateSpatialEntityFromIdEXT(
		get_spatial_context_handle(p_spatial_context), &create_info, &spatial_entity_data.entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create spatial entity [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_make_entity(RID p_spatial_context, uint64_t p_entity_id)
{
	return make_spatial_entity(p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id);
}

XrSpatialEntityIdEXT OpenXRSpatialEntityExtension::get_spatial_entity_id(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, XR_NULL_ENTITY);

	return entity_data->entity_id;
}

uint64_t OpenXRSpatialEntityExtension::_get_entity_id(RID p_entity)
{
	return (uint64_t)get_spatial_entity_id(p_entity);
}

RID OpenXRSpatialEntityExtension::get_spatial_entity_context(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, RID());

	return entity_data->spatial_context;
}

void OpenXRSpatialEntityExtension::free_spatial_entity(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL(entity_data);
	ERR_FAIL_COND(entity_data->entity == XR_NULL_HANDLE);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrResult result = xrDestroySpatialEntityEXT_ptr(entity_data->entity);
	if (XR_FAILED(result)) {
		WARN_PRINT("OpenXR: Failed to destroy spatial entity [" +
				   OpenXRAPI::get_error_string(result) + "]");
	}

	data->spatial_entity_owner.free(p_entity);
}

String OpenXRSpatialEntityExtension::get_spatial_capability_name(
	XrSpatialCapabilityEXT p_capability)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityEXT, p_capability)
}

String OpenXRSpatialEntityExtension::get_spatial_component_type_name(
	XrSpatialComponentTypeEXT p_component_type)
{
	XR_ENUM_SWITCH(XrSpatialComponentTypeEXT, p_component_type)
}

String OpenXRSpatialEntityExtension::get_spatial_feature_name(
	XrSpatialCapabilityFeatureEXT p_feature)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityFeatureEXT, p_feature)
}

void OpenXRSpatialEntityExtension::on_session_destroyed() {}

bool OpenXRSpatialEntityExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialAnchorCapability

void OpenXRSpatialAnchorCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialAnchorCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialAnchorCapability::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_spatial_anchors")) {
		request_extensions[XR_EXT_SPATIAL_ANCHOR_EXTENSION_NAME] = &data->spatial_anchor_ext;
		if (GLOBAL_GET_CACHED(
				bool, "xr/openxr/extensions/spatial_entity/enable_persistent_anchors")) {
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_EXTENSION_NAME] =
				&data->spatial_persistence_ext;
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_OPERATIONS_EXTENSION_NAME] =
				&data->spatial_persistence_operations_ext;
		}
	}

	return request_extensions;
}

void OpenXRSpatialAnchorCapability::on_instance_created(const XrInstance p_instance)
{
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	if (data->spatial_anchor_ext) {
		EXT_INIT_XR_FUNC(xrCreateSpatialAnchorEXT);
	}

	if (data->spatial_persistence_ext) {
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrResult xr_result =
			OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceScopesEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		if (xr_result != XR_SUCCESS) {
			print_verbose("OpenXR: xrEnumerateSpatialPersistenceScopesEXT is not supported, "
						  "falling back to xrEnumerateSpatialPersistenceStoresEXT!");
			xr_result = OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceStoresEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		}
		ERR_FAIL_COND(XR_FAILED(xr_result));

		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialPersistenceContextEXT);
	}

	if (data->spatial_persistence_operations_ext) {
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityCompleteEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityCompleteEXT);
	}
}

void OpenXRSpatialAnchorCapability::on_instance_destroyed()
{
	xrCreateSpatialAnchorEXT_ptr = nullptr;

	xrEnumerateSpatialPersistenceScopesEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialPersistenceContextEXT_ptr = nullptr;
}

bool OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()
{
	return data != nullptr && data->spatial_anchor_supported;
}

bool OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()
{
	if (!is_spatial_anchor_supported()) {
		return false;
	}

	return data->spatial_persistence_ext;
}

bool OpenXRSpatialAnchorCapability::_load_supported_persistence_scopes()
{
	ERR_FAIL_COND_V(!is_spatial_persistence_supported(), false);

	if (data->supported_persistence_scopes.is_empty()) {
		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		uint32_t size;
		XrInstance instance = OpenXRAPI::get_instance();
		XrSystemId system_id = OpenXRAPI::get_system_id();

		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

		XrResult result =
			xrEnumerateSpatialPersistenceScopesEXT(instance, system_id, 0, &size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scope count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (size > 0) {
			data->supported_persistence_scopes.resize(size);
			result = xrEnumerateSpatialPersistenceScopesEXT(instance, system_id,
				data->supported_persistence_scopes.size(), &size,
				data->supported_persistence_scopes.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scopes [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}
		}

		if (is_print_verbose_enabled()) {
			if (!data->supported_persistence_scopes.is_empty()) {
				print_verbose("OpenXR: Supported spatial persistence scopes:");
				for (const XrSpatialPersistenceScopeEXT& scope :
					data->supported_persistence_scopes) {
					print_verbose(" - " + get_spatial_persistence_scope_name(scope));
				}
			}
			else {
				WARN_PRINT("OpenXR: No persistence scopes found!");
			}
		}
	}

	return true;
}

bool OpenXRSpatialAnchorCapability::is_persistence_scope_supported(
	XrSpatialPersistenceScopeEXT p_scope)
{
	if (!is_spatial_persistence_supported()) {
		return false;
	}

	if (!_load_supported_persistence_scopes()) {
		return false;
	}

	return data->supported_persistence_scopes.has(p_scope);
}

bool OpenXRSpatialAnchorCapability::_is_persistence_scope_supported(PersistenceScope p_scope)
{
	return is_persistence_scope_supported((XrSpatialPersistenceScopeEXT)p_scope);
}

XrSpatialPersistenceContextEXT OpenXRSpatialAnchorCapability::get_persistence_context_handle(
	RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL_V(persistence_context_data, XR_NULL_HANDLE);

	return persistence_context_data->persistence_context;
}

uint64_t OpenXRSpatialAnchorCapability::_get_persistence_context_handle(RID p_persistence_context)
{
	return (uint64_t)get_persistence_context_handle(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::free_persistence_context(RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL(persistence_context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (persistence_context_data->persistence_context != XR_NULL_HANDLE) {
		XrResult result =
			xrDestroySpatialPersistenceContextEXT(persistence_context_data->persistence_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the persistence context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		persistence_context_data->persistence_context = XR_NULL_HANDLE;
	}

	data->persistence_context_owner.free(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::_on_persistence_context_completed(RID p_persistence_context)
{
	data->persistence_context = p_persistence_context;

	_create_spatial_context();
}

void OpenXRSpatialAnchorCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialAnchorCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

Ref<OpenXRAnchorTracker> OpenXRSpatialAnchorCapability::create_new_anchor(
	const Transform3D& p_transform, RID p_spatial_context, Ref<OpenXRStructureBase> p_next)
{
	Ref<OpenXRAnchorTracker> tracker;

	ERR_FAIL_COND_V_MSG(!is_spatial_anchor_supported(), tracker,
		"OpenXR: Spatial entity anchor capability is not supported on this hardware!");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), tracker);
	ERR_FAIL_COND_V(!XRServer::is_initialized(), tracker);
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), tracker);

	XrPosef pose = OpenXRAPI::pose_from_transform(p_transform);

	RID sc = p_spatial_context.is_valid() ? p_spatial_context : data->spatial_context;
	ERR_FAIL_COND_V(sc.is_null(), tracker);

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	XrSpatialAnchorCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_EXT,
		next,
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
		pose,
	};
	XrSpatialEntityIdEXT entity_id;
	XrSpatialEntityEXT entity;
	XrResult result = xrCreateSpatialAnchorEXT(
		OpenXRSpatialEntityExtension::get_spatial_context_handle(sc), &create_info, &entity_id,
		&entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(tracker,
			"OpenXR: Failed to create anchor [" + OpenXRAPI::get_error_string(result) + "]");
	}

	tracker.instantiate(OpenXRSpatialEntityTracker::ENTITY_ANCHOR);
	tracker->set_spatial_context(sc);
	tracker->set_entity(OpenXRSpatialEntityExtension::add_spatial_entity(sc, entity_id, entity));
	tracker->set_tracker_desc("Anchor");
	tracker->set_pose(SNAME("default"), p_transform, Vector3(), Vector3());
	tracker->add_next(p_next);

	data->anchor_trackers[sc][entity_id] = tracker;
	XRServer::add_tracker(tracker->get_xr_tracker());

	return tracker;
}

void OpenXRSpatialAnchorCapability::remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker)
{
	ERR_FAIL_COND(p_anchor_tracker.is_null());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	ERR_FAIL_COND_MSG(p_anchor_tracker->has_uuid(),
		"OpenXR: This anchor is persistent. It must first be made unpersistent.");

	XRServer::remove_tracker(p_anchor_tracker->get_xr_tracker());

	RID entity = p_anchor_tracker->get_entity();
	ERR_FAIL_COND(entity.is_null());

	XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(entity);
	ERR_FAIL_COND(entity_id == XR_NULL_ENTITY);

	for (KeyValue<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>>& anchors :
		data->anchor_trackers) {
		if (anchors.value.has(entity_id)) {
			anchors.value.erase(entity_id);
			break;
		}
	}

	p_anchor_tracker->set_entity(RID());
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_scope_name(
	XrSpatialPersistenceScopeEXT p_scope)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceScopeEXT, p_scope)
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_context_result_name(
	XrSpatialPersistenceContextResultEXT p_result)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceContextResultEXT, p_result)
}

Ref<OpenXRFutureResult> OpenXRSpatialAnchorCapability::_create_spatial_context()
{
	Ref<OpenXRFutureResult> oxfr = memnew(OpenXRFutureResult);
	return oxfr;
}

void OpenXRSpatialAnchorCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialAnchorCapability::on_session_destroyed() {}

void OpenXRSpatialAnchorCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialMarkerTrackingCapability

void OpenXRSpatialMarkerTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialMarkerTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialMarkerTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_marker_tracking")) {
		request_extensions[XR_EXT_SPATIAL_MARKER_TRACKING_EXTENSION_NAME] =
			&data->spatial_marker_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()
{
	if (!data->spatial_marker_tracking_ext) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT);
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialMarkerTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialMarkerTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialMarkerTrackingCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialPlaneTrackingCapability

void OpenXRSpatialPlaneTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialPlaneTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialPlaneTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_plane_tracking")) {
		request_extensions[XR_EXT_SPATIAL_PLANE_TRACKING_EXTENSION_NAME] =
			&data->spatial_plane_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialPlaneTrackingCapability::is_supported()
{
	return data != nullptr && data->spatial_plane_tracking_supported;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialPlaneTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialPlaneTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialPlaneTrackingCapability::on_process() {}
