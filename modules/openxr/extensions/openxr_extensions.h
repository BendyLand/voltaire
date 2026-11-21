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
#include "../openxr_api.h"
#include "../openxr_interface.h"
#include "core/math/quaternion.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "modules/modules_enabled.gen.h"
#include "servers/xr/xr_tracker.h"

#ifdef XR_USE_PLATFORM_ANDROID
#include <jni.h>
#include <openxr/openxr_platform.h>
#endif

#ifdef ANDROID_ENABLED
#include <jni.h>

typedef XrResult(XRAPI_PTR* PFN_xrCreateSwapchainAndroidSurfaceKHR)(
	XrSession session, const XrSwapchainCreateInfo* info, XrSwapchain* swapchain, jobject* surface);
#endif

#ifdef MODULE_GLTF_ENABLED
class GLTFDocument;
class GLTFState;
class Node3D;
template <typename T> class TypedArray;
#endif

class JavaObject;

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

///////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayerExtension

class OpenXRCompositionLayerExtension final
{
public:
	// Must be identical to Filter enum definition in OpenXRCompositionLayer.
	enum Filter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_CUBIC,
	};

	// Must be identical to MipmapMode enum definition in OpenXRCompositionLayer.
	enum MipmapMode
	{
		MIPMAP_MODE_DISABLED,
		MIPMAP_MODE_NEAREST,
		MIPMAP_MODE_LINEAR,
	};

	// Must be identical to Wrap enum definition in OpenXRCompositionLayer.
	enum Wrap
	{
		WRAP_CLAMP_TO_BORDER,
		WRAP_CLAMP_TO_EDGE,
		WRAP_REPEAT,
		WRAP_MIRRORED_REPEAT,
		WRAP_MIRROR_CLAMP_TO_EDGE,
	};

	// Must be identical to Swizzle enum definition in OpenXRCompositionLayer.
	enum Swizzle
	{
		SWIZZLE_RED,
		SWIZZLE_GREEN,
		SWIZZLE_BLUE,
		SWIZZLE_ALPHA,
		SWIZZLE_ZERO,
		SWIZZLE_ONE,
	};

	// Must be identical to EyeVisibility enum definition in OpenXRCompositionLayer.
	enum EyeVisibility
	{
		EYE_VISIBILITY_BOTH,
		EYE_VISIBILITY_LEFT,
		EYE_VISIBILITY_RIGHT,
	};

	enum PoseSpace
	{
		POSE_WORLD_LOCKED,
		POSE_HEAD_LOCKED,
	};

	struct SwapchainState
	{
		Filter min_filter = Filter::FILTER_LINEAR;
		Filter mag_filter = Filter::FILTER_LINEAR;
		MipmapMode mipmap_mode = MipmapMode::MIPMAP_MODE_LINEAR;
		Wrap horizontal_wrap = Wrap::WRAP_CLAMP_TO_BORDER;
		Wrap vertical_wrap = Wrap::WRAP_CLAMP_TO_BORDER;
		Swizzle red_swizzle = Swizzle::SWIZZLE_RED;
		Swizzle green_swizzle = Swizzle::SWIZZLE_GREEN;
		Swizzle blue_swizzle = Swizzle::SWIZZLE_BLUE;
		Swizzle alpha_swizzle = Swizzle::SWIZZLE_ALPHA;
		float max_anisotropy = 1.0;
		Color border_color = {0.0, 0.0, 0.0, 0.0};
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(const XrInstance p_instance);
	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();
	static void on_pre_render();

	static int get_composition_layer_count();
	static XrCompositionLayerBaseHeader* get_composition_layer(int p_index);
	static int get_composition_layer_order(int p_index);

	static RID composition_layer_create(XrCompositionLayerBaseHeader* p_openxr_layer);
	static void composition_layer_free(RID p_layer);

	static void composition_layer_register(RID p_layer);
	static void composition_layer_unregister(RID p_layer);

#define OPENXR_LAYER_FUNC1_DECL(m_name, m_arg1)                                                    \
	static void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1);                          \
	static void composition_layer_##m_name(RID p_layer, m_arg1 p1);

#define OPENXR_LAYER_FUNC2_DECL(m_name, m_arg1, m_arg2)                                            \
	static void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1, m_arg2 p2);               \
	static void composition_layer_##m_name(RID p_layer, m_arg1 p1, m_arg2 p2);

	OPENXR_LAYER_FUNC2_DECL(set_viewport, RID, const Size2i&);
	OPENXR_LAYER_FUNC2_DECL(set_use_android_surface, bool, const Size2i&);
	OPENXR_LAYER_FUNC1_DECL(set_sort_order, int);
	OPENXR_LAYER_FUNC1_DECL(set_alpha_blend, bool);
	OPENXR_LAYER_FUNC1_DECL(set_transform, const Transform3D&);
	OPENXR_LAYER_FUNC1_DECL(set_protected_content, bool);

	OPENXR_LAYER_FUNC1_DECL(set_min_filter, Filter);
	OPENXR_LAYER_FUNC1_DECL(set_mag_filter, Filter);
	OPENXR_LAYER_FUNC1_DECL(set_mipmap_mode, MipmapMode);
	OPENXR_LAYER_FUNC1_DECL(set_horizontal_wrap, Wrap);
	OPENXR_LAYER_FUNC1_DECL(set_vertical_wrap, Wrap);
	OPENXR_LAYER_FUNC1_DECL(set_red_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_blue_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_green_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_alpha_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_max_anisotropy, float);
	OPENXR_LAYER_FUNC1_DECL(set_border_color, const Color&);
	OPENXR_LAYER_FUNC1_DECL(set_pose_space, PoseSpace);
	OPENXR_LAYER_FUNC1_DECL(set_eye_visibility, EyeVisibility);

	OPENXR_LAYER_FUNC1_DECL(set_quad_size, const Size2&);

	OPENXR_LAYER_FUNC1_DECL(set_cylinder_radius, float);
	OPENXR_LAYER_FUNC1_DECL(set_cylinder_aspect_ratio, float);
	OPENXR_LAYER_FUNC1_DECL(set_cylinder_central_angle, float);

	OPENXR_LAYER_FUNC1_DECL(set_equirect_radius, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_central_horizontal_angle, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_upper_vertical_angle, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_lower_vertical_angle, float);

#undef OPENXR_LAYER_FUNC1_DECL
#undef OPENXR_LAYER_FUNC2_DECL

	static Ref<JavaObject> composition_layer_get_android_surface(RID p_layer);

	static bool is_available(XrStructureType p_which);

	static bool is_android_surface_swapchain_available()
	{
		return data != nullptr && data->android_surface_ext_available;
	}

private:
	static void _composition_layer_free_rt(RID p_layer);
	static void _composition_layer_register_rt(RID p_layer);
	static void _composition_layer_unregister_rt(RID p_layer);

#ifdef ANDROID_ENABLED
	static bool create_android_surface_swapchain(
		XrSwapchainCreateInfo* p_info, XrSwapchain* r_swapchain, jobject* r_surface);

	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateSwapchainAndroidSurfaceKHR, (XrSession), session,
		(const XrSwapchainCreateInfo*), info, (XrSwapchain*), swapchain, (jobject*), surface)
#endif

	struct CompositionLayer
	{
		union
		{
			XrCompositionLayerBaseHeader composition_layer;
			XrCompositionLayerQuad composition_layer_quad;
			XrCompositionLayerCylinderKHR composition_layer_cylinder;
			XrCompositionLayerEquirect2KHR composition_layer_equirect;
		};

		int sort_order = 1;
		bool alpha_blend = false;
		bool extension_property_values_changed = true;

		struct
		{
			RID viewport;
			Size2i viewport_size;
			OpenXRAPI::OpenXRSwapChainInfo swapchain_info;
			bool static_image = false;
			bool swapchain_protected_content = false;
		} subviewport;

#ifdef ANDROID_ENABLED
		struct
		{
			XrSwapchain swapchain = XR_NULL_HANDLE;
			Ref<JavaObject> surface;
		} android_surface;
#endif

		PoseSpace pose_space = POSE_WORLD_LOCKED;
		XrSpace layer_reference_space = XR_NULL_HANDLE;

		bool use_android_surface = false;
		bool protected_content = false;
		Size2i swapchain_size;

		SwapchainState swapchain_state;
		bool swapchain_state_is_dirty = false;

		void set_viewport(RID p_viewport, const Size2i& p_size);
		void set_use_android_surface(bool p_use_android_surface, const Size2i& p_size);

		void set_sort_order(int p_sort_order) { sort_order = p_sort_order; }

		void set_alpha_blend(bool p_alpha_blend);

		void set_protected_content(bool p_protected_content)
		{
			protected_content = p_protected_content;
		}

		void set_transform(const Transform3D& p_transform);

		void set_min_filter(Filter p_mode);
		void set_mag_filter(Filter p_mode);
		void set_mipmap_mode(MipmapMode p_mode);
		void set_horizontal_wrap(Wrap p_mode);
		void set_vertical_wrap(Wrap p_mode);
		void set_red_swizzle(Swizzle p_mode);
		void set_green_swizzle(Swizzle p_mode);
		void set_blue_swizzle(Swizzle p_mode);
		void set_alpha_swizzle(Swizzle p_mode);
		void set_max_anisotropy(float p_value);
		void set_border_color(const Color& p_color);
		void set_pose_space(PoseSpace p_pose_space);
		void set_eye_visibility(EyeVisibility p_eye_visibility);

		void set_quad_size(const Size2& p_size);

		void set_cylinder_radius(float p_radius);
		void set_cylinder_aspect_ratio(float p_aspect_ratio);
		void set_cylinder_central_angle(float p_central_angle);

		void set_equirect_radius(float p_radius);
		void set_equirect_central_horizontal_angle(float p_angle);
		void set_equirect_upper_vertical_angle(float p_angle);
		void set_equirect_lower_vertical_angle(float p_angle);

		Ref<JavaObject> get_android_surface();
		void on_pre_render();
		XrCompositionLayerBaseHeader* get_composition_layer();
		void free();

	private:
		void update_swapchain_state();
		void update_swapchain_sub_image(XrSwapchainSubImage& r_subimage);
		bool update_and_acquire_swapchain(bool p_static_image);
		RID get_current_swapchain_texture();
		void free_swapchain();

#ifdef ANDROID_ENABLED
		void create_android_surface();
#endif
	};

public:
	struct Data
	{
		bool cylinder_ext_available = false;
		bool equirect_ext_available = false;
		bool android_surface_ext_available = false;
		Mutex composition_layer_mutex;
		RID_Owner<CompositionLayer, true> composition_layer_owner;
		LocalVector<CompositionLayer*> registered_composition_layers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRCompositionLayerExtension() = delete;
	OpenXRCompositionLayerExtension(const OpenXRCompositionLayerExtension&) = delete;
	OpenXRCompositionLayerExtension& operator=(const OpenXRCompositionLayerExtension&) = delete;
	~OpenXRCompositionLayerExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFBUpdateSwapchainExtension

class OpenXRFBUpdateSwapchainExtension final
{
protected:
	friend class OpenXRFBFoveationExtension;

public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static bool is_enabled();
	static bool is_android_ext_enabled();

	static void update_swapchain_state(XrSwapchain p_swapchain,
		const OpenXRCompositionLayerExtension::SwapchainState* p_swapchain_state);

	static void update_swapchain_surface_size(XrSwapchain p_swapchain, const Size2i& p_size);

private:
	static uint32_t filter_to_gl(OpenXRCompositionLayerExtension::Filter p_filter,
		OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode =
			OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED);
	static uint32_t wrap_to_gl(OpenXRCompositionLayerExtension::Wrap p_wrap);
	static uint32_t swizzle_to_gl(OpenXRCompositionLayerExtension::Swizzle p_swizzle);

	static uint32_t filter_to_vk(OpenXRCompositionLayerExtension::Filter p_filter);
	static uint32_t mipmap_mode_to_vk(OpenXRCompositionLayerExtension::MipmapMode p_mipmap);
	static uint32_t wrap_to_vk(OpenXRCompositionLayerExtension::Wrap p_wrap);
	static uint32_t swizzle_to_vk(OpenXRCompositionLayerExtension::Swizzle p_swizzle);

	EXT_PROTO_XRRESULT_FUNC2(xrUpdateSwapchainFB, (XrSwapchain), swapchain,
		(const XrSwapchainStateBaseHeaderFB*), state);
	EXT_PROTO_XRRESULT_FUNC2(
		xrGetSwapchainStateFB, (XrSwapchain), swapchain, (XrSwapchainStateBaseHeaderFB*), state);

public:
	struct Data
	{
		String rendering_driver;
		bool fb_swapchain_update_state_ext = false;
		bool fb_swapchain_update_state_vulkan_ext = false;
		bool fb_swapchain_update_state_opengles_ext = false;
		bool fb_swapchain_update_state_android_ext = false;
	};

	static inline Data* data = nullptr;

	static void initialize(const String& p_rendering_driver = String());
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFBUpdateSwapchainExtension() = delete;
	OpenXRFBUpdateSwapchainExtension(const OpenXRFBUpdateSwapchainExtension&) = delete;
	OpenXRFBUpdateSwapchainExtension& operator=(const OpenXRFBUpdateSwapchainExtension&) = delete;
	~OpenXRFBUpdateSwapchainExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFBFoveationExtension

class OpenXRFBFoveationExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	static void* set_swapchain_create_info_and_get_next_pointer(void* p_next_pointer);

	static void on_main_swapchains_created();

	static bool is_enabled();

	static XrFoveationLevelFB get_foveation_level();
	static void set_foveation_level(XrFoveationLevelFB p_foveation_level);

	static XrFoveationDynamicFB get_foveation_dynamic();
	static void set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic);

	static bool is_foveation_eye_tracked_enabled();
	static void get_fragment_density_offsets(LocalVector<Vector2i>& r_offsets);

	static void set_foveation_with_subsampled_images_enabled(bool p_enabled);
	static bool is_foveation_with_subsampled_images_enabled();

	static void set_foveation_with_subsampled_images_active(bool p_active);

private:
	static void _update_profile_rt();
	static void update_profile();

	EXT_PROTO_XRRESULT_FUNC3(xrCreateFoveationProfileFB, (XrSession), session,
		(const XrFoveationProfileCreateInfoFB*), create_info, (XrFoveationProfileFB*), profile);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyFoveationProfileFB, (XrFoveationProfileFB), profile);
	EXT_PROTO_XRRESULT_FUNC2(xrGetFoveationEyeTrackedStateMETA, (XrSession), session,
		(XrFoveationEyeTrackedStateMETA*), foveationState);

public:
	struct Data;

	static inline Data* data = nullptr;

	static void initialize(const String& p_rendering_driver = String());
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFBFoveationExtension() = delete;
	OpenXRFBFoveationExtension(const OpenXRFBFoveationExtension&) = delete;
	OpenXRFBFoveationExtension& operator=(const OpenXRFBFoveationExtension&) = delete;
	~OpenXRFBFoveationExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFrameSynthesisExtension

class OpenXRFrameSynthesisExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void prepare_view_configuration(uint32_t p_view_count);
	static void* set_view_configuration_and_get_next_pointer(
		uint32_t p_view, void* p_next_pointer);
	static void print_view_configuration_info(uint32_t p_view);

	static void on_session_destroyed();

	static void on_main_swapchains_created();
	static void on_pre_draw_viewport(RID p_render_target);
	static void on_post_draw_viewport(RID p_render_target);
	static void* set_projection_views_and_get_next_pointer(int p_view_index, void* p_next_pointer);

	static bool is_available();

	static bool is_enabled();
	static void set_enabled(bool p_enabled);

	static bool get_relax_frame_interval();
	static void set_relax_frame_interval(bool p_relax_frame_interval);

	static void skip_next_frame();

protected:
	static void _set_render_state_enabled_rt(bool p_enabled);
	static void _set_relax_frame_interval_rt(bool p_relax_frame_interval);
	static void _set_skip_next_frame_rt();

private:
	enum SwapchainTypes
	{
		SWAPCHAIN_MOTION_VECTOR,
		SWAPCHAIN_DEPTH,
		SWAPCHAIN_MAX
	};

	static void free_swapchains();

	struct RenderState
	{
		bool enabled = true;
		bool relax_frame_interval = false;
		bool skip_next_frame = false;

		LocalVector<XrFrameSynthesisConfigViewEXT> config_views;
		OpenXRAPI::OpenXRSwapChainInfo swapchains[SWAPCHAIN_MAX];
		LocalVector<XrFrameSynthesisInfoEXT> frame_synthesis_info;
		Transform3D previous_transform;
	};

public:
	struct Data
	{
		bool frame_synthesis_ext = false;
		bool enabled = true;
		bool relax_frame_interval = false;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFrameSynthesisExtension() = delete;
	OpenXRFrameSynthesisExtension(const OpenXRFrameSynthesisExtension&) = delete;
	OpenXRFrameSynthesisExtension& operator=(const OpenXRFrameSynthesisExtension&) = delete;
	~OpenXRFrameSynthesisExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFutureResult & OpenXRFutureExtension

class OpenXRFutureExtension;

class OpenXRFutureResult final : public RefCounted
{
	friend class OpenXRFutureExtension;

protected:
	void _mark_as_finished();
	void _mark_as_cancelled();

public:
	enum ResultStatus
	{
		RESULT_RUNNING,
		RESULT_FINISHED,
		RESULT_CANCELLED,
	};

	ResultStatus get_status() const;
	XrFutureEXT get_future() const;

	void cancel_future();

private:
	ResultStatus status = RESULT_RUNNING;
	XrFutureEXT future;

	uint64_t _get_future() const;
};

class OpenXRFutureExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static void on_process();

	static bool is_active();

	static void cancel_future(XrFutureEXT p_future);

private:
	static void _cancel_future(uint64_t p_future);

	EXT_PROTO_XRRESULT_FUNC3(xrPollFutureEXT, (XrInstance), instance, (const XrFuturePollInfoEXT*),
		poll_info, (XrFuturePollResultEXT*), poll_result);
	EXT_PROTO_XRRESULT_FUNC2(
		xrCancelFutureEXT, (XrInstance), instance, (const XrFutureCancelInfoEXT*), cancel_info);

public:
	struct Data
	{
		bool future_ext = false;
		HashMap<XrFutureEXT, Ref<OpenXRFutureResult>> futures;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFutureExtension() = delete;
	OpenXRFutureExtension(const OpenXRFutureExtension&) = delete;
	OpenXRFutureExtension& operator=(const OpenXRFutureExtension&) = delete;
	~OpenXRFutureExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRHandTrackingExtension

class OpenXRHandTrackingExtension final
{
public:
	enum HandTrackedHands
	{
		OPENXR_TRACKED_LEFT_HAND,
		OPENXR_TRACKED_RIGHT_HAND,
		OPENXR_MAX_TRACKED_HANDS
	};

	enum HandTrackedSource
	{
		OPENXR_SOURCE_UNKNOWN,
		OPENXR_SOURCE_UNOBSTRUCTED,
		OPENXR_SOURCE_CONTROLLER,
		OPENXR_SOURCE_NOT_TRACKED,
		OPENXR_SOURCE_MAX
	};

	struct HandTracker
	{
		bool is_initialized = false;
		Ref<XRTracker> godot_tracker;
		HandTrackedSource source = OPENXR_SOURCE_UNKNOWN;
	};

	static void on_instance_destroyed();
	static void on_session_destroyed();

	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	static void on_state_ready();
	static void on_process();
	static void on_state_stopping();

	static bool get_active();
	static const HandTracker* get_hand_tracker(HandTrackedHands p_hand);

private:
	static void cleanup_hand_tracking();

public:
	struct Data
	{
		HandTracker hand_trackers[OPENXR_MAX_TRACKED_HANDS];
		bool hand_tracking_ext = false;
		bool hand_motion_range_ext = false;
		bool hand_tracking_source_ext = false;
		bool unobstructed_data_source = false;
		bool controller_data_source = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRHandTrackingExtension() = delete;
	OpenXRHandTrackingExtension(const OpenXRHandTrackingExtension&) = delete;
	OpenXRHandTrackingExtension& operator=(const OpenXRHandTrackingExtension&) = delete;
	~OpenXRHandTrackingExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRRenderModelData & OpenXRRenderModelExtension

#ifdef MODULE_GLTF_ENABLED
class OpenXRRenderModelData final : public RefCounted
{
private:
	Ref<GLTFDocument> gltf_document;
	Ref<GLTFState> gltf_state;
	PackedStringArray node_names;

public:
	Ref<GLTFState> get_gltf_state();

	bool parse_gltf_document(const PackedByteArray& p_bytes);
	Node3D* new_scene_instance();

	void set_node_names(const PackedStringArray& p_node_names);
	PackedStringArray get_node_names() const;
	const String get_node_name(uint32_t p_node_index) const;

	OpenXRRenderModelData();
	~OpenXRRenderModelData();
};

class OpenXRRenderModelExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_session_created(const XrSession p_session);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static bool on_event_polled(const XrEventDataBuffer& event);
	static void on_sync_actions();

	static bool is_active();

	// Render model.
	static bool has_render_model(RID p_render_model);
	static RID render_model_create(XrRenderModelIdEXT p_render_model_id);
	static void render_model_destroy(RID p_render_model);

	static TypedArray<RID> render_model_get_all();
	static Node3D* render_model_new_scene_instance(RID p_render_model);
	static PackedStringArray render_model_get_subaction_paths(RID p_render_model);
	static XrPath render_model_get_top_level_path(RID p_render_model);
	static String render_model_get_top_level_path_as_string(RID p_render_model);
	static XRPose::TrackingConfidence render_model_get_confidence(RID p_render_model);
	static Transform3D render_model_get_root_transform(RID p_render_model);
	static uint32_t render_model_get_animatable_node_count(RID p_render_model);
	static String render_model_get_animatable_node_name(RID p_render_model, uint32_t p_index);
	static bool render_model_is_animatable_node_visible(RID p_render_model, uint32_t p_index);
	static Transform3D render_model_get_animatable_node_transform(
		RID p_render_model, uint32_t p_index);

private:
	static void _clear_interaction_data();
	static bool _update_interaction_data();

	struct RenderModel
	{
		XrRenderModelIdEXT xr_render_model_id = XR_NULL_RENDER_MODEL_ID_EXT;
		XrRenderModelEXT xr_render_model = XR_NULL_HANDLE;
		uint32_t animatable_node_count = 0;
		Ref<OpenXRRenderModelData> render_model_data;
		XrSpace xr_space = XR_NULL_HANDLE;
		XRPose::TrackingConfidence confidence =
			XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE;
		Transform3D root_transform;
		LocalVector<XrRenderModelNodeStateEXT> node_states;
		XrPath top_level_path = XR_NULL_PATH;
	};

	static Ref<OpenXRRenderModelData> _get_render_model_data(
		XrUuidEXT p_cache_id, uint32_t p_animatable_node_count);
	static Ref<OpenXRRenderModelData> _load_asset(
		XrRenderModelAssetEXT p_asset, uint32_t p_animatable_node_count);
	static void _clear_render_model_data();

	static RID _render_model_create(uint64_t p_render_model_id);

	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelEXT, (XrSession), session,
		(const XrRenderModelCreateInfoEXT*), createInfo, (XrRenderModelEXT*), renderModel);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyRenderModelEXT, (XrRenderModelEXT), renderModel);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelPropertiesEXT, (XrRenderModelEXT), renderModel,
		(const XrRenderModelPropertiesGetInfoEXT*), getInfo, (XrRenderModelPropertiesEXT*),
		properties);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelSpaceEXT, (XrSession), session,
		(const XrRenderModelSpaceCreateInfoEXT*), createInfo, (XrSpace*), space);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelAssetEXT, (XrSession), session,
		(const XrRenderModelAssetCreateInfoEXT*), createInfo, (XrRenderModelAssetEXT*), asset);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyRenderModelAssetEXT, (XrRenderModelAssetEXT), asset);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelAssetDataEXT, (XrRenderModelAssetEXT), asset,
		(const XrRenderModelAssetDataGetInfoEXT*), getInfo, (XrRenderModelAssetDataEXT*), buffer);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelAssetPropertiesEXT, (XrRenderModelAssetEXT), asset,
		(const XrRenderModelAssetPropertiesGetInfoEXT*), getInfo,
		(XrRenderModelAssetPropertiesEXT*), properties);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelStateEXT, (XrRenderModelEXT), renderModel,
		(const XrRenderModelStateGetInfoEXT*), getInfo, (XrRenderModelStateEXT*), state);
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateInteractionRenderModelIdsEXT, (XrSession), session,
		(const XrInteractionRenderModelIdsEnumerateInfoEXT*), getInfo, (uint32_t),
		renderModelIdCapacityInput, (uint32_t*), renderModelIdCountOutput, (XrRenderModelIdEXT*),
		renderModelIds);
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateRenderModelSubactionPathsEXT, (XrRenderModelEXT),
		renderModel, (const XrInteractionRenderModelSubactionPathInfoEXT*), info, (uint32_t),
		pathCapacityInput, (uint32_t*), pathCountOutput, (XrPath*), paths);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelPoseTopLevelUserPathEXT, (XrRenderModelEXT),
		renderModel, (const XrInteractionRenderModelTopLevelUserPathGetInfoEXT*), info, (XrPath*),
		topLevelUserPath);

	EXT_PROTO_XRRESULT_FUNC4(xrLocateSpace, (XrSpace), space, (XrSpace), baseSpace, (XrTime), time,
		(XrSpaceLocation*), location);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpace, (XrSpace), space);
	EXT_PROTO_XRRESULT_FUNC5(xrPathToString, (XrInstance), instance, (XrPath), path, (uint32_t),
		bufferCapacityInput, (uint32_t*), bufferCountOutput, (char*), buffer);

public:
	struct Data
	{
		bool uuid_ext = false;
		bool render_model_ext = false;
		bool interaction_render_model_ext = false;
		bool xr_sync_has_run = false;
		bool _interaction_data_dirty = true;
		HashMap<XrRenderModelIdEXT, RID> interaction_render_models;
		Vector<XrPath> toplevel_paths;
		RID_Owner<RenderModel, true> render_model_owner;
		HashMap<XrUuidEXT, Ref<OpenXRRenderModelData>, HashMapHasherXrUuidEXT>
			render_model_data_cache;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRRenderModelExtension() = delete;
	OpenXRRenderModelExtension(const OpenXRRenderModelExtension&) = delete;
	OpenXRRenderModelExtension& operator=(const OpenXRRenderModelExtension&) = delete;
	~OpenXRRenderModelExtension() = delete;
};
#endif // MODULE_GLTF_ENABLED
