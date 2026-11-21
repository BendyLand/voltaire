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

// --- Folded from modules/openxr/extensions/openxr_interaction_profile_extension.h ---
#include <openxr/openxr.h>
#include "../action_map/openxr_action_map.h"
#include "core/templates/hash_map.h"
#include "core/types.h"

class OpenXRInteractionProfileExtension final
{
public:
	enum ProfileKind
	{
		PROFILE_HTC_CONTROLLER = 0,
		PROFILE_HTC_VIVE_TRACKER,
		PROFILE_HUAWEI_CONTROLLER,
		PROFILE_KHR_GENERIC_CONTROLLER,
		PROFILE_META_CONTROLLER,
		PROFILE_ML2_CONTROLLER,
		PROFILE_MXINK,
		PROFILE_PALM_POSE,
		PROFILE_PICO_CONTROLLER,
		PROFILE_VALVE_CONTROLLER,
		PROFILE_WMR_CONTROLLER,
		PROFILE_HAND_INTERACTION,
		PROFILE_LOCAL_FLOOR,
		PROFILE_DPAD_BINDING,
		PROFILE_VALVE_ANALOG_THRESHOLD,
		PROFILE_COMPOSITION_LAYER_DEPTH,
		PROFILE_MAX
	};

	enum HTCControllers
	{
		HTC_VIVE_COSMOS = 0,
		HTC_VIVE_FOCUS3 = 1,
		HTC_HAND_INTERACTION = 2,
		HTC_MAX_CONTROLLERS = 3
	};

	enum MetaControllers
	{
		META_TOUCH_PROXIMITY = 0,
		META_TOUCH_PRO = 1,
		META_TOUCH_PLUS = 2,
		META_MAX_CONTROLLERS = 3
	};

	enum WMRControllers
	{
		WMR_HPMR = 0,
		WMR_SAMSUNG_ODESSY = 1,
		WMR_HAND_INTERACTION = 2,
		WMR_MAX_CONTROLLERS = 3
	};

private:
	ProfileKind kind = PROFILE_KHR_GENERIC_CONTROLLER;
	bool flags[3] = {false, false, false};

public:
	explicit OpenXRInteractionProfileExtension(
		ProfileKind p_kind = PROFILE_KHR_GENERIC_CONTROLLER);

	ProfileKind get_kind() const { return kind; }

	HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	PackedStringArray get_suggested_tracker_names() const;
	bool on_event_polled(const XrEventDataBuffer& p_event);
	void on_register_metadata();

	bool is_available(int p_subindex = 0) const;

	static inline OpenXRInteractionProfileExtension* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static OpenXRInteractionProfileExtension* get_profile(ProfileKind p_kind);
	static bool is_profile_available(ProfileKind p_kind, int p_subindex = 0);
	static HashMap<String, bool*> get_all_requested_extensions(XrVersion p_version);
};

class OpenXRDpadBindingModifier final : public OpenXRBindingModifier
{
private:
	PackedByteArray dpad_bindings_data;
	XrInteractionProfileDpadBindingEXT* dpad_bindings = nullptr;
	String input_path;
	Ref<OpenXRActionSet> action_set;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRDpadBindingModifier();

	void set_action_set(const Ref<OpenXRActionSet>& p_action_set);
	Ref<OpenXRActionSet> get_action_set() const;

	void set_input_path(const String& p_input_path);
	String get_input_path() const;

	void set_threshold(float p_threshold);
	float get_threshold() const;

	void set_threshold_released(float p_threshold);
	float get_threshold_released() const;

	void set_center_region(float p_center_region);
	float get_center_region() const;

	void set_wedge_angle(float p_wedge_angle);
	float get_wedge_angle() const;

	void set_wedge_angle_deg(float p_wedge_angle);
	float get_wedge_angle_deg() const;

	void set_is_sticky(bool p_sticky);
	bool get_is_sticky() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "DPad modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};

class OpenXRAnalogThresholdModifier final : public OpenXRBindingModifier
{
private:
	XrInteractionProfileAnalogThresholdVALVE analog_threshold;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRAnalogThresholdModifier();

	void set_on_threshold(float p_threshold);
	float get_on_threshold() const;

	void set_off_threshold(float p_threshold);
	float get_off_threshold() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "Analog threshold modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};

// --- End modules/openxr/extensions/openxr_interaction_profile_extension.h ---

// --- Folded from modules/openxr/extensions/spatial_entities/openxr_spatial_entities.h ---
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"

#include "core/templates/rid_owner.h"
#include "scene/resources/mesh.h"
#include "servers/xr/xr_tracker.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

#define XR_NULL_ENTITY 0x7FFFFFFF

// Persistence configuration (folded into OpenXRStructureBase)
using OpenXRSpatialContextPersistenceConfig = OpenXRStructureBase;

// Consolidated capability configuration wrapper for XrSpatialCapabilityConfigurationBaseHeaderEXT
class OpenXRSpatialCapabilityConfigurationBaseHeader final : public RefCounted
{
public:
	enum CapabilityType
	{
		CAPABILITY_NONE,
		CAPABILITY_ANCHOR,
		CAPABILITY_PLANE_TRACKING,
		CAPABILITY_QR_CODE,
		CAPABILITY_MICRO_QR_CODE,
		CAPABILITY_ARUCO,
		CAPABILITY_APRIL_TAG,
	};

	enum ArucoDict
	{
		ARUCO_DICT_4X4_50 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT,
		ARUCO_DICT_4X4_100 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_100_EXT,
		ARUCO_DICT_4X4_250 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_250_EXT,
		ARUCO_DICT_4X4_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_1000_EXT,
		ARUCO_DICT_5X5_50 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_50_EXT,
		ARUCO_DICT_5X5_100 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_100_EXT,
		ARUCO_DICT_5X5_250 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_250_EXT,
		ARUCO_DICT_5X5_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_1000_EXT,
		ARUCO_DICT_6X6_50 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_50_EXT,
		ARUCO_DICT_6X6_100 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_100_EXT,
		ARUCO_DICT_6X6_250 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_250_EXT,
		ARUCO_DICT_6X6_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_1000_EXT,
		ARUCO_DICT_7X7_50 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_50_EXT,
		ARUCO_DICT_7X7_100 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_100_EXT,
		ARUCO_DICT_7X7_250 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_250_EXT,
		ARUCO_DICT_7X7_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT,
	};

	enum AprilTagDict
	{
		APRIL_TAG_DICT_16H5 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT,
		APRIL_TAG_DICT_25H9 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_25H9_EXT,
		APRIL_TAG_DICT_36H10 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H10_EXT,
		APRIL_TAG_DICT_36H11 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT,
	};

	OpenXRSpatialCapabilityConfigurationBaseHeader(CapabilityType p_capability_type = CAPABILITY_NONE);
	~OpenXRSpatialCapabilityConfigurationBaseHeader();

	void set_capability_type(CapabilityType p_capability_type);
	CapabilityType get_capability_type() const { return capability_type; }

	bool has_valid_configuration() const;
	uint64_t _get_configurationgd();
	XrSpatialCapabilityConfigurationBaseHeaderEXT* get_configuration();

	Vector<XrSpatialComponentTypeEXT> get_enabled_components() const { return enabled_components; }

	// Plane tracking configuration queries
	bool get_supports_mesh_2d();
	bool get_supports_polygons();
	bool get_supports_labels();

	// ArUco dictionary configuration
	void set_aruco_dict(XrSpatialMarkerArucoDictEXT p_dict);
	XrSpatialMarkerArucoDictEXT get_aruco_dict() const;

	// AprilTag dictionary configuration
	void set_april_dict(XrSpatialMarkerAprilTagDictEXT p_dict);
	XrSpatialMarkerAprilTagDictEXT get_april_dict() const;

private:
	CapabilityType capability_type = CAPABILITY_NONE;
	int supports_mesh_2d = -1;
	int supports_polygons = -1;
	int supports_labels = -1;
	XrSpatialMarkerArucoDictEXT aruco_dict = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT;
	XrSpatialMarkerAprilTagDictEXT april_dict = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT;

	Vector<XrSpatialComponentTypeEXT> enabled_components;

	union ConfigPayload
	{
		XrSpatialCapabilityConfigurationBaseHeaderEXT base;
		XrSpatialCapabilityConfigurationAnchorEXT anchor;
		XrSpatialCapabilityConfigurationPlaneTrackingEXT plane;
		XrSpatialCapabilityConfigurationQrCodeEXT qrcode;
		XrSpatialCapabilityConfigurationMicroQrCodeEXT micro_qrcode;
		XrSpatialCapabilityConfigurationArucoMarkerEXT aruco;
		XrSpatialCapabilityConfigurationAprilTagEXT april_tag;

		ConfigPayload()
			: base{XR_TYPE_UNKNOWN, nullptr, XR_SPATIAL_CAPABILITY_MAX_ENUM_EXT, 0, nullptr}
		{
		}
	} config;

	PackedInt64Array _get_enabled_components() const;
	void _set_aruco_dict(ArucoDict p_dict);
	ArucoDict _get_aruco_dict() const;
	void _set_april_dict(AprilTagDict p_dict);
	AprilTagDict _get_april_dict() const;
};

// Consolidated wrapper class for spatial component data returned by discovery and update queries
class OpenXRSpatialComponentData final : public RefCounted
{
public:
	enum ComponentKind
	{
		COMPONENT_NONE,
		COMPONENT_QUERY_RESULT,
		COMPONENT_BOUNDED_2D,
		COMPONENT_BOUNDED_3D,
		COMPONENT_PARENT,
		COMPONENT_MESH_2D,
		COMPONENT_MESH_3D,
		COMPONENT_ANCHOR,
		COMPONENT_PERSISTENCE,
		COMPONENT_PLANE_ALIGNMENT,
		COMPONENT_POLYGON_2D,
		COMPONENT_PLANE_SEMANTIC_LABEL,
		COMPONENT_MARKER,
	};

	enum PlaneAlignment
	{
		PLANE_ALIGNMENT_HORIZONTAL_UPWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_UPWARD_EXT,
		PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD_EXT,
		PLANE_ALIGNMENT_VERTICAL = XR_SPATIAL_PLANE_ALIGNMENT_VERTICAL_EXT,
		PLANE_ALIGNMENT_ARBITRARY = XR_SPATIAL_PLANE_ALIGNMENT_ARBITRARY_EXT,
	};

	enum PlaneSemanticLabel
	{
		PLANE_SEMANTIC_LABEL_UNCATEGORIZED = XR_SPATIAL_PLANE_SEMANTIC_LABEL_UNCATEGORIZED_EXT,
		PLANE_SEMANTIC_LABEL_FLOOR = XR_SPATIAL_PLANE_SEMANTIC_LABEL_FLOOR_EXT,
		PLANE_SEMANTIC_LABEL_WALL = XR_SPATIAL_PLANE_SEMANTIC_LABEL_WALL_EXT,
		PLANE_SEMANTIC_LABEL_CEILING = XR_SPATIAL_PLANE_SEMANTIC_LABEL_CEILING_EXT,
		PLANE_SEMANTIC_LABEL_TABLE = XR_SPATIAL_PLANE_SEMANTIC_LABEL_TABLE_EXT,
	};

	enum MarkerType
	{
		MARKER_TYPE_UNKNOWN,
		MARKER_TYPE_QRCODE,
		MARKER_TYPE_MICRO_QRCODE,
		MARKER_TYPE_ARUCO,
		MARKER_TYPE_APRIL_TAG,
		MARKER_TYPE_MAX
	};

	OpenXRSpatialComponentData(ComponentKind p_kind = COMPONENT_NONE);
	~OpenXRSpatialComponentData();

	OpenXRSpatialComponentData(const OpenXRSpatialComponentData&) = delete;
	OpenXRSpatialComponentData& operator=(const OpenXRSpatialComponentData&) = delete;

	void set_component_kind(ComponentKind p_kind);
	ComponentKind get_component_kind() const { return component_kind; }

	void set_capacity(uint32_t p_capacity);
	int64_t get_capacity() const { return capacity; }

	int64_t _get_component_typegd() const;
	XrSpatialComponentTypeEXT get_component_type() const;
	void* get_structure_data(void* p_next);

	// COMPONENT_QUERY_RESULT accessors
	XrSpatialEntityIdEXT get_entity_id(int64_t p_index) const;
	XrSpatialEntityTrackingStateEXT get_entity_state(int64_t p_index) const;
	static String get_entity_tracking_state_name(XrSpatialEntityTrackingStateEXT p_tracking_state);

	// COMPONENT_BOUNDED_2D / COMPONENT_BOUNDED_3D accessors
	Transform3D get_center_pose(int64_t p_index) const;
	Transform3D get_center_pose_2d(int64_t p_index) const;
	Transform3D get_center_pose_3d(int64_t p_index) const;
	Vector2 get_size_2d(int64_t p_index) const;
	Vector3 get_size_3d(int64_t p_index) const;

	// COMPONENT_PARENT accessors
	RID get_parent(int64_t p_index) const;

	// COMPONENT_MESH_2D / COMPONENT_MESH_3D / COMPONENT_POLYGON_2D accessors
	Transform3D get_transform(int64_t p_index) const;
	PackedVector2Array get_vertices(RID p_snapshot, int64_t p_index) const;
	PackedInt32Array get_indices(RID p_snapshot, int64_t p_index) const;
	Ref<Mesh> get_mesh(int64_t p_index) const;

	// COMPONENT_ANCHOR accessors
	Transform3D get_entity_pose(int64_t p_index) const;

	// COMPONENT_PERSISTENCE accessors
	XrUuid get_persistent_uuid(int64_t p_index) const;
	XrSpatialPersistenceStateEXT get_persistent_state(int64_t p_index) const;
	static String get_persistence_state_name(XrSpatialPersistenceStateEXT p_state);

	// COMPONENT_PLANE_ALIGNMENT accessors
	XrSpatialPlaneAlignmentEXT get_plane_alignment(int64_t p_index) const;

	// COMPONENT_PLANE_SEMANTIC_LABEL accessors
	XrSpatialPlaneSemanticLabelEXT get_plane_semantic_label(int64_t p_index) const;

	// COMPONENT_MARKER accessors
	MarkerType get_marker_type(int64_t p_index) const;
	uint32_t get_marker_id(int64_t p_index) const;
	XrSpatialBufferEXT get_marker_data(int64_t p_index) const;

private:
	ComponentKind component_kind = COMPONENT_NONE;
	uint32_t capacity = 0;
	void* data_ptr = nullptr;
	XrSpatialEntityTrackingStateEXT* entity_states_ptr = nullptr;

	union StructurePayload
	{
		XrSpatialComponentDataQueryResultEXT query_result;
		XrSpatialComponentBounded2DListEXT bounded2d_list;
		XrSpatialComponentBounded3DListEXT bounded3d_list;
		XrSpatialComponentParentListEXT parent_list;
		XrSpatialComponentMesh2DListEXT mesh2d_list;
		XrSpatialComponentMesh3DListEXT mesh3d_list;
		XrSpatialComponentAnchorListEXT anchor_list;
		XrSpatialComponentPersistenceListEXT persistence_list;
		XrSpatialComponentPlaneAlignmentListEXT plane_alignment_list;
		XrSpatialComponentPolygon2DListEXT polygon2d_list;
		XrSpatialComponentPlaneSemanticLabelListEXT plane_semantic_label_list;
		XrSpatialComponentMarkerListEXT marker_list;

		StructurePayload()
			: query_result{XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT, nullptr, 0, 0, nullptr,
				  0, 0, nullptr}
		{
		}
	} structure;

	void _free_buffers();
	void _init_structure_header();

	uint64_t _get_entity_id(int64_t p_index) const;
	String _get_persistent_uuid(int64_t p_index) const;
	uint64_t _get_persistent_state(int64_t p_index) const;
	PlaneAlignment _get_plane_alignment(int64_t p_index) const;
	PlaneSemanticLabel _get_plane_semantic_label(int64_t p_index) const;
};

// Unified tracker for spatial entities (generic, anchor, plane, marker), composing XRTracker
class OpenXRSpatialEntityTracker final : public RefCounted
{
public:
	enum EntityType
	{
		ENTITY_GENERIC,
		ENTITY_ANCHOR,
		ENTITY_PLANE,
		ENTITY_MARKER,
	};

	enum EntityTrackingState
	{
		ENTITY_TRACKING_STATE_STOPPED = XR_SPATIAL_ENTITY_TRACKING_STATE_STOPPED_EXT,
		ENTITY_TRACKING_STATE_PAUSED = XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT,
		ENTITY_TRACKING_STATE_TRACKING = XR_SPATIAL_ENTITY_TRACKING_STATE_TRACKING_EXT,
	};

	OpenXRSpatialEntityTracker(EntityType p_entity_type = ENTITY_GENERIC);
	~OpenXRSpatialEntityTracker() override;

	const Ref<XRTracker>& get_xr_tracker() const { return xr_tracker; }

	// Composed XRTracker forwarding helpers
	void set_tracker_type(XRServer::TrackerType p_type) { xr_tracker->set_tracker_type(p_type); }
	XRServer::TrackerType get_tracker_type() const { return xr_tracker->get_tracker_type(); }
	void set_tracker_name(const String& p_name) { xr_tracker->set_tracker_name(p_name); }
	String get_tracker_name() const { return xr_tracker->get_tracker_name(); }
	void set_tracker_desc(const String& p_desc) { xr_tracker->set_tracker_desc(p_desc); }
	String get_tracker_desc() const { return xr_tracker->get_tracker_desc(); }
	void set_has_tracking_data(bool p_has_tracking_data)
	{
		xr_tracker->set_has_tracking_data(p_has_tracking_data);
	}
	bool get_has_tracking_data() const { return xr_tracker->get_has_tracking_data(); }
	bool has_pose(const String& p_action_name) const { return xr_tracker->has_pose(p_action_name); }
	Ref<XRPose> get_pose(const String& p_action_name) const
	{
		return xr_tracker->get_pose(p_action_name);
	}
	void invalidate_pose(const String& p_action_name)
	{
		xr_tracker->invalidate_pose(p_action_name);
	}
	void set_pose(const String& p_action_name, const Transform3D& p_transform,
		const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
		const XRPose::TrackingConfidence p_tracking_confidence =
			XRPose::XR_TRACKING_CONFIDENCE_HIGH)
	{
		xr_tracker->set_pose(p_action_name, p_transform, p_linear_velocity, p_angular_velocity,
			p_tracking_confidence);
	}

	void set_entity_type(EntityType p_entity_type) { entity_type = p_entity_type; }
	EntityType get_entity_type() const { return entity_type; }

	void set_spatial_context(const RID& p_spatial_context);
	RID get_spatial_context() const;

	void set_entity(const RID& p_entity);
	RID get_entity() const;

	void set_spatial_tracking_state(const XrSpatialEntityTrackingStateEXT p_state);
	XrSpatialEntityTrackingStateEXT get_spatial_tracking_state() const;

	void add_next(Ref<OpenXRStructureBase> p_next);
	void remove_next(Ref<OpenXRStructureBase> p_next);
	Ref<OpenXRStructureBase> get_next() const;

	// Anchor fields & methods
	bool has_uuid() const;
	XrUuid get_uuid() const;
	void set_uuid(const XrUuid& p_uuid);

	// Shared Plane / Marker bounds
	void set_bounds_size(const Vector2& p_bounds_size);
	Vector2 get_bounds_size() const;

	// Plane fields & methods
	void set_plane_alignment(OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment);
	OpenXRSpatialComponentData::PlaneAlignment get_plane_alignment() const;

	void set_plane_label(const String& p_plane_label);
	String get_plane_label() const;

	void set_mesh_data(const Transform3D& p_origin, const PackedVector2Array& p_vertices,
		const PackedInt32Array& p_indices = PackedInt32Array());
	void clear_mesh_data();

	Transform3D get_mesh_offset() const;
	Ref<Mesh> get_mesh() { return mesh.mesh; }
#ifndef PHYSICS_3D_DISABLED
	Ref<Shape3D> get_shape(real_t p_thickness = 0.01);
#endif

	// Marker fields & methods
	void set_marker_type(OpenXRSpatialComponentData::MarkerType p_marker_type);
	OpenXRSpatialComponentData::MarkerType get_marker_type() const;

	void set_marker_id(uint32_t p_id);
	uint32_t get_marker_id() const;

private:
	Ref<XRTracker> xr_tracker;
	EntityType entity_type = ENTITY_GENERIC;
	RID spatial_context;
	RID spatial_entity;
	XrSpatialEntityTrackingStateEXT spatial_tracking_state =
		XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT;
	Ref<OpenXRStructureBase> next;

	// Anchor data
	XrUuid uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Plane & Marker data
	Vector2 bounds_size;
	OpenXRSpatialComponentData::PlaneAlignment plane_alignment =
		OpenXRSpatialComponentData::PLANE_ALIGNMENT_HORIZONTAL_UPWARD;
	String plane_label;

	struct MeshData
	{
		bool has_mesh_data = false;
		Transform3D origin;
		PackedVector2Array vertices;
		PackedInt32Array indices;

		Ref<Mesh> mesh;
#ifndef PHYSICS_3D_DISABLED
		Ref<Shape3D> shape3d;
#endif
	} mesh;

	struct Edge
	{
		int32_t a;
		int32_t b;

		static _FORCE_INLINE_ uint32_t hash(const Edge& p_edge)
		{
			uint32_t h = hash_murmur3_one_32(p_edge.a);
			return hash_murmur3_one_32(p_edge.b, h);
		}

		bool operator==(const Edge& p_edge) const { return (a == p_edge.a && b == p_edge.b); }

		Edge(int32_t p_a = 0, int32_t p_b = 0)
		{
			a = p_a;
			b = p_b;
			if (a < b) {
				SWAP(a, b);
			}
		}
	};

	OpenXRSpatialComponentData::MarkerType marker_type =
		OpenXRSpatialComponentData::MARKER_TYPE_UNKNOWN;
	uint32_t marker_id = 0;

	void _set_spatial_tracking_state(const EntityTrackingState p_state);
	EntityTrackingState _get_spatial_tracking_state() const;

	String _get_uuid() const;
	void _set_uuid(const String& p_uuid);
	static bool uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b);
};

using OpenXRAnchorTracker = OpenXRSpatialEntityTracker;
using OpenXRMarkerTracker = OpenXRSpatialEntityTracker;
using OpenXRPlaneTracker = OpenXRSpatialEntityTracker;

// Spatial entity extension
class OpenXRSpatialEntityExtension final
{
public:
	enum Capability
	{
		CAPABILITY_PLANE_TRACKING = XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
		CAPABILITY_MARKER_TRACKING_QR_CODE = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_ARUCO_MARKER =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
		CAPABILITY_MARKER_TRACKING_APRIL_TAG = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
		CAPABILITY_ANCHOR = XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
	};

	enum ComponentType
	{
		COMPONENT_TYPE_BOUNDED_2D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT,
		COMPONENT_TYPE_BOUNDED_3D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT,
		COMPONENT_TYPE_PARENT = XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT,
		COMPONENT_TYPE_MESH_3D = XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT,
		COMPONENT_TYPE_PLANE_ALIGNMENT = XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT,
		COMPONENT_TYPE_MESH_2D = XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT,
		COMPONENT_TYPE_POLYGON_2D = XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT,
		COMPONENT_TYPE_PLANE_SEMANTIC_LABEL = XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT,
		COMPONENT_TYPE_MARKER = XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT,
		COMPONENT_TYPE_ANCHOR = XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT,
		COMPONENT_TYPE_PERSISTENCE = XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT,
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool get_active();
	static bool supports_capability(XrSpatialCapabilityEXT p_capability);
	static bool supports_component_type(
		XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type);

	// Spatial contexts
	static bool get_spatial_context_ready(RID p_spatial_context);
	static void free_spatial_context(RID p_spatial_context);
	static XrSpatialContextEXT get_spatial_context_handle(RID p_spatial_context);

	// Update query
	static RID update_spatial_entities(RID p_spatial_context, const LocalVector<RID>& p_entities,
		const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
		Ref<OpenXRStructureBase> p_next);

	// Snapshot data
	static void free_spatial_snapshot(RID p_spatial_snapshot);
	static XrSpatialSnapshotEXT get_spatial_snapshot_handle(RID p_spatial_snapshot);
	static RID get_spatial_snapshot_context(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String get_string(RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedByteArray get_uint8_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint16_t> get_uint16_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint32_t> get_uint32_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedFloat32Array get_float_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector2Array get_vector2_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector3Array get_vector3_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);

	// Entities
	static RID find_spatial_entity(XrSpatialEntityIdEXT p_entity_id);
	static RID add_spatial_entity(
		RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity);
	static RID make_spatial_entity(RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id);
	static XrSpatialEntityIdEXT get_spatial_entity_id(RID p_entity);
	static RID get_spatial_entity_context(RID p_entity);
	static void free_spatial_entity(RID p_entity);

	static String get_spatial_capability_name(XrSpatialCapabilityEXT p_capability);
	static String get_spatial_component_type_name(XrSpatialComponentTypeEXT p_component_type);
	static String get_spatial_feature_name(XrSpatialCapabilityFeatureEXT p_feature);

private:
	// Capabilities
	struct SpatialEntityCapabality
	{
		Vector<XrSpatialComponentTypeEXT> component_types;
		Vector<XrSpatialCapabilityFeatureEXT> features;
	};
	static bool _load_capabilities();

	static bool _supports_capability(Capability p_capability);
	static bool _supports_component_type(Capability p_capability, ComponentType p_component_type);

	// Spatial context
	struct SpatialContextData
	{
		XrSpatialContextEXT spatial_context = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_context_handle(RID p_spatial_context);

	// Snapshot data
	struct SpatialSnapshotData
	{
		RID spatial_context;
		XrSpatialSnapshotEXT spatial_snapshot = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_snapshot_handle(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String _get_string(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedByteArray _get_uint8_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint16_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint32_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedFloat32Array _get_float_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector2Array _get_vector2_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector3Array _get_vector3_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);

	// Entities
	struct SpatialEntityData
	{
		RID spatial_context;
		XrSpatialEntityIdEXT entity_id = XR_NULL_ENTITY;
		XrSpatialEntityEXT entity = XR_NULL_HANDLE;
	};

	static RID _find_entity(uint64_t p_entity_id);
	static RID _add_entity(RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity);
	static RID _make_entity(RID p_spatial_context, uint64_t p_entity_id);
	static uint64_t _get_entity_id(RID p_entity);

	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialCapabilitiesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), capability_capacity_input, (uint32_t*),
		capability_count_output, (XrSpatialCapabilityEXT*), capabilities);
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSpatialCapabilityComponentTypesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability,
		(XrSpatialCapabilityComponentTypesEXT*), capability_components);
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateSpatialCapabilityFeaturesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability, (uint32_t),
		capability_feature_capacity_input, (uint32_t*), capability_feature_count_output,
		(XrSpatialCapabilityFeatureEXT*), capability_features);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextAsyncEXT, (XrSession), session,
		(const XrSpatialContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextCompleteEXT, (XrSession), session, (XrFutureEXT),
		future, (XrCreateSpatialContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialContextEXT, (XrSpatialContextEXT), spatial_context);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotAsyncEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialDiscoverySnapshotCreateInfoEXT*), create_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotCompleteEXT, (XrSpatialContextEXT),
		spatial_context, (const XrCreateSpatialDiscoverySnapshotCompletionInfoEXT*),
		create_snapshot_completion_info, (XrCreateSpatialDiscoverySnapshotCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrQuerySpatialComponentDataEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialComponentDataQueryConditionEXT*), query_condition,
		(XrSpatialComponentDataQueryResultEXT*), query_result);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialSnapshotEXT, (XrSpatialSnapshotEXT), snapshot);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialEntityFromIdEXT, (XrSpatialContextEXT), spatial_context,
		(const XrSpatialEntityFromIdCreateInfoEXT*), create_info, (XrSpatialEntityEXT*),
		spatial_entity);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialEntityEXT, (XrSpatialEntityEXT), spatial_entity);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialUpdateSnapshotEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialUpdateSnapshotCreateInfoEXT*), createInfo,
		(XrSpatialSnapshotEXT*), snapshot);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferStringEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (char*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint8EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint8_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint16EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint16_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint32EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint32_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferFloatEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (float*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector2fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector2f*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector3fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector3f*), buffer);

public:
	struct Data
	{
		bool spatial_entity_ext = false;
		HashMap<XrSpatialCapabilityEXT, SpatialEntityCapabality> supported_capabilities;
		int capabilities_load_state = 0;
		RID_Owner<SpatialContextData> spatial_context_owner;
		RID_Owner<SpatialSnapshotData> spatial_snapshot_owner;
		RID_Owner<SpatialEntityData> spatial_entity_owner;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialEntityExtension() = delete;
	OpenXRSpatialEntityExtension(const OpenXRSpatialEntityExtension&) = delete;
	OpenXRSpatialEntityExtension& operator=(const OpenXRSpatialEntityExtension&) = delete;
	~OpenXRSpatialEntityExtension() = delete;
};

// (Persistent) anchor logic
class OpenXRSpatialAnchorCapability final
{
public:
	enum PersistenceScope
	{
		PERSISTENCE_SCOPE_SYSTEM_MANAGED = XR_SPATIAL_PERSISTENCE_SCOPE_SYSTEM_MANAGED_EXT,
		PERSISTENCE_SCOPE_LOCAL_ANCHORS = XR_SPATIAL_PERSISTENCE_SCOPE_LOCAL_ANCHORS_EXT,
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_spatial_anchor_supported();
	static bool is_spatial_persistence_supported();

	// Persistence scopes
	static bool is_persistence_scope_supported(XrSpatialPersistenceScopeEXT p_scope);

	static XrSpatialPersistenceContextEXT get_persistence_context_handle(RID p_persistence_context);
	static void free_persistence_context(RID p_persistence_context);

	static Ref<OpenXRAnchorTracker> create_new_anchor(const Transform3D& p_transform,
		RID p_spatial_context = RID(),
		Ref<OpenXRStructureBase> p_next = Ref<OpenXRStructureBase>());
	static void remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker);

	static String get_spatial_persistence_scope_name(XrSpatialPersistenceScopeEXT p_scope);
	static String get_spatial_persistence_context_result_name(
		XrSpatialPersistenceContextResultEXT p_result);

private:
	static bool _load_supported_persistence_scopes();

	struct PersistenceContextData
	{
		XrSpatialPersistenceScopeEXT scope;
		XrSpatialPersistenceContextEXT persistence_context = XR_NULL_HANDLE;
	};

	static bool _is_persistence_scope_supported(PersistenceScope p_scope);
	static uint64_t _get_persistence_context_handle(RID p_persistence_context);

	static void _on_persistence_context_completed(RID p_persistence_context);
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

	EXT_PROTO_XRRESULT_FUNC4(xrCreateSpatialAnchorEXT, (XrSpatialContextEXT), spatialContext,
		(const XrSpatialAnchorCreateInfoEXT*), create_info, (XrSpatialEntityIdEXT*),
		anchor_entity_id, (XrSpatialEntityEXT*), anchor_entity);

	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialPersistenceScopesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), persistence_scope_capacity_input, (uint32_t*),
		persistence_scope_count_output, (XrSpatialPersistenceScopeEXT*), persistence_scopes);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextAsyncEXT, (XrSession), session,
		(const XrSpatialPersistenceContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextCompleteEXT, (XrSession), session,
		(XrFutureEXT), future, (XrCreateSpatialPersistenceContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialPersistenceContextEXT,
		(XrSpatialPersistenceContextEXT), persistence_context);

	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityPersistInfoEXT*), persist_info, (XrFutureEXT*),
		future);
	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrPersistSpatialEntityCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityUnpersistInfoEXT*), unpersist_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrUnpersistSpatialEntityCompletionEXT*),
		completion);

public:
	struct Data
	{
		bool spatial_anchor_ext = false;
		bool spatial_persistence_ext = false;
		bool spatial_persistence_operations_ext = false;
		bool spatial_anchor_supported = false;
		RID spatial_context;
		RID persistence_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> anchor_configuration;
		Ref<OpenXRSpatialContextPersistenceConfig> persistence_configuration;
		Vector<XrSpatialPersistenceScopeEXT> supported_persistence_scopes;
		RID_Owner<PersistenceContextData> persistence_context_owner;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>> anchor_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialAnchorCapability() = delete;
	OpenXRSpatialAnchorCapability(const OpenXRSpatialAnchorCapability&) = delete;
	OpenXRSpatialAnchorCapability& operator=(const OpenXRSpatialAnchorCapability&) = delete;
	~OpenXRSpatialAnchorCapability() = delete;
};

// Marker tracking logic
class OpenXRSpatialMarkerTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_qrcode_supported();
	static bool is_micro_qrcode_supported();
	static bool is_aruco_supported();
	static bool is_april_tag_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_marker_tracking_ext = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> micro_qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> aruco_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> april_tag_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRMarkerTracker>>> marker_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialMarkerTrackingCapability() = delete;
	OpenXRSpatialMarkerTrackingCapability(const OpenXRSpatialMarkerTrackingCapability&) = delete;
	OpenXRSpatialMarkerTrackingCapability& operator=(
		const OpenXRSpatialMarkerTrackingCapability&) = delete;
	~OpenXRSpatialMarkerTrackingCapability() = delete;
};

// Plane tracking logic
class OpenXRSpatialPlaneTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_plane_tracking_ext = false;
		bool spatial_plane_tracking_supported = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> plane_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRPlaneTracker>>> plane_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialPlaneTrackingCapability() = delete;
	OpenXRSpatialPlaneTrackingCapability(const OpenXRSpatialPlaneTrackingCapability&) = delete;
	OpenXRSpatialPlaneTrackingCapability& operator=(
		const OpenXRSpatialPlaneTrackingCapability&) = delete;
	~OpenXRSpatialPlaneTrackingCapability() = delete;
};

// --- End modules/openxr/extensions/spatial_entities/openxr_spatial_entities.h ---
