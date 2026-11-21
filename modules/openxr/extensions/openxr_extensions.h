/**************************************************************************/
/*  openxr_extensions.h                                                   */
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

#pragma once

#include <openxr/openxr.h>
#include "../openxr_interface.h"
#include "../util.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/types.h"

#ifdef XR_USE_PLATFORM_ANDROID
#include <jni.h>
#include <openxr/openxr_platform.h>
#endif

///////////////////////////////////////////////////////////////////////////
// OpenXREyeGazeInteractionExtension

class OpenXREyeGazeInteractionExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);

	static PackedStringArray get_suggested_tracker_names();

	static bool is_available();
	static bool supports_eye_gaze_interaction();

	static bool get_eye_gaze_pose(double p_dist, Vector3& r_eye_pose);

	struct Data
	{
		bool available = false;
		XrSystemEyeGazeInteractionPropertiesEXT properties;
		bool init_eye_gaze_pose = false;
		RID eye_tracker;
		RID eye_action;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXREyeGazeInteractionExtension() = delete;
	OpenXREyeGazeInteractionExtension(const OpenXREyeGazeInteractionExtension&) = delete;
	OpenXREyeGazeInteractionExtension& operator=(const OpenXREyeGazeInteractionExtension&) = delete;
	~OpenXREyeGazeInteractionExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRUserPresenceExtension

class OpenXRUserPresenceExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);

	static void on_state_ready();
	static void on_state_stopping();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_user_present();
	static bool is_active();

	struct Data
	{
		bool available = false;
		XrSystemUserPresencePropertiesEXT properties;
		bool user_present = true;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRUserPresenceExtension() = delete;
	OpenXRUserPresenceExtension(const OpenXRUserPresenceExtension&) = delete;
	OpenXRUserPresenceExtension& operator=(const OpenXRUserPresenceExtension&) = delete;
	~OpenXRUserPresenceExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRAndroidThreadSettingsExtension

class OpenXRAndroidThreadSettingsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(XrInstance p_instance);
	static void on_session_created(XrSession p_session);

	enum ThreadType
	{
		THREAD_TYPE_APPLICATION_MAIN,
		THREAD_TYPE_APPLICATION_WORKER,
		THREAD_TYPE_RENDERER_MAIN,
		THREAD_TYPE_RENDERER_WORKER,
	};

	static bool set_application_thread_type(ThreadType p_thread_type, uint32_t p_thread_id = 0);

private:
	static bool _initialize_openxr_android_thread_settings_extension();
	static void _set_render_thread_type();

#ifdef XR_USE_PLATFORM_ANDROID
	EXT_PROTO_XRRESULT_FUNC3(xrSetAndroidApplicationThreadKHR, (XrSession), session,
		(XrAndroidThreadTypeKHR), threadType, (uint32_t), threadId);
#endif

public:
	struct Data
	{
		bool available = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRAndroidThreadSettingsExtension() = delete;
	OpenXRAndroidThreadSettingsExtension(const OpenXRAndroidThreadSettingsExtension&) = delete;
	OpenXRAndroidThreadSettingsExtension& operator=(
		const OpenXRAndroidThreadSettingsExtension&) = delete;
	~OpenXRAndroidThreadSettingsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRDisplayRefreshRateExtension

class OpenXRDisplayRefreshRateExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static bool on_event_polled(const XrEventDataBuffer& event);

	static float get_refresh_rate();
	static void set_refresh_rate(float p_refresh_rate);

private:
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateDisplayRefreshRatesFB, (XrSession), session, (uint32_t),
		displayRefreshRateCapacityInput, (uint32_t*), displayRefreshRateCountOutput, (float*),
		displayRefreshRates);
	EXT_PROTO_XRRESULT_FUNC2(
		xrGetDisplayRefreshRateFB, (XrSession), session, (float*), display_refresh_rate);
	EXT_PROTO_XRRESULT_FUNC2(
		xrRequestDisplayRefreshRateFB, (XrSession), session, (float), display_refresh_rate);

public:
	struct Data
	{
		bool display_refresh_rate_ext = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRDisplayRefreshRateExtension() = delete;
	OpenXRDisplayRefreshRateExtension(const OpenXRDisplayRefreshRateExtension&) = delete;
	OpenXRDisplayRefreshRateExtension& operator=(const OpenXRDisplayRefreshRateExtension&) = delete;
	~OpenXRDisplayRefreshRateExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRPerformanceSettingsExtension

class OpenXRPerformanceSettingsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_available();

	static void set_cpu_level(OpenXRInterface::PerfSettingsLevel p_level);
	static void set_gpu_level(OpenXRInterface::PerfSettingsLevel p_level);

private:
	static XrPerfSettingsLevelEXT level_to_openxr(OpenXRInterface::PerfSettingsLevel p_level);
	static OpenXRInterface::PerfSettingsSubDomain openxr_to_sub_domain(
		XrPerfSettingsSubDomainEXT p_sub_domain);
	static OpenXRInterface::PerfSettingsNotificationLevel openxr_to_notification_level(
		XrPerfSettingsNotificationLevelEXT p_notification_level);

	EXT_PROTO_XRRESULT_FUNC3(xrPerfSettingsSetPerformanceLevelEXT, (XrSession), session,
		(XrPerfSettingsDomainEXT), domain, (XrPerfSettingsLevelEXT), level)

public:
	struct Data
	{
		bool available = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRPerformanceSettingsExtension() = delete;
	OpenXRPerformanceSettingsExtension(const OpenXRPerformanceSettingsExtension&) = delete;
	OpenXRPerformanceSettingsExtension& operator=(
		const OpenXRPerformanceSettingsExtension&) = delete;
	~OpenXRPerformanceSettingsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRDebugUtilsExtension

class OpenXRDebugUtilsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static bool get_active();

	static void set_object_name(
		XrObjectType p_object_type, uint64_t p_object_handle, const char* p_object_name);
	static void begin_debug_label_region(const char* p_label_name);
	static void end_debug_label_region();
	static void insert_debug_label(const char* p_label_name);

private:
	static XrBool32 XRAPI_PTR _debug_callback(
		XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);
	static XrBool32 debug_callback(XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);

	EXT_PROTO_XRRESULT_FUNC3(xrCreateDebugUtilsMessengerEXT, (XrInstance), p_instance,
		(const XrDebugUtilsMessengerCreateInfoEXT*), p_create_info, (XrDebugUtilsMessengerEXT*),
		p_messenger)
	EXT_PROTO_XRRESULT_FUNC1(
		xrDestroyDebugUtilsMessengerEXT, (XrDebugUtilsMessengerEXT), p_messenger)
	EXT_PROTO_XRRESULT_FUNC2(xrSetDebugUtilsObjectNameEXT, (XrInstance), p_instance,
		(const XrDebugUtilsObjectNameInfoEXT*), p_name_info)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionBeginDebugUtilsLabelRegionEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)
	EXT_PROTO_XRRESULT_FUNC1(xrSessionEndDebugUtilsLabelRegionEXT, (XrSession), p_session)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionInsertDebugUtilsLabelEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)

public:
	struct Data
	{
		bool debug_utils_ext = false;
		XrDebugUtilsMessengerEXT default_messenger = XR_NULL_HANDLE;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRDebugUtilsExtension() = delete;
	OpenXRDebugUtilsExtension(const OpenXRDebugUtilsExtension&) = delete;
	OpenXRDebugUtilsExtension& operator=(const OpenXRDebugUtilsExtension&) = delete;
	~OpenXRDebugUtilsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMaskExtension

class OpenXRVisibilityMaskExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);

	static void on_session_created(const XrSession p_instance);
	static void on_session_destroyed();

	static void on_pre_render();
	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_available();
	static RID get_mesh();

private:
	struct MeshData
	{
		Vector<XrVector2f> vertices;
		Vector<uint32_t> indices;
	};

	static void _update_mesh_data(uint32_t p_view);
	static void _update_mesh();

	EXT_PROTO_XRRESULT_FUNC5(xrGetVisibilityMaskKHR, (XrSession), session,
		(XrViewConfigurationType), viewConfigurationType, (uint32_t), viewIndex,
		(XrVisibilityMaskTypeKHR), visibilityMaskType, (XrVisibilityMaskKHR*), visibilityMask);

public:
	struct Data
	{
		bool available = false;
		bool is_dirty = false;
		RID shader;
		RID material;
		RID mesh;
		uint32_t mesh_count = 0;
		MeshData mesh_data[4];
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRVisibilityMaskExtension() = delete;
	OpenXRVisibilityMaskExtension(const OpenXRVisibilityMaskExtension&) = delete;
	OpenXRVisibilityMaskExtension& operator=(const OpenXRVisibilityMaskExtension&) = delete;
	~OpenXRVisibilityMaskExtension() = delete;
};
