/**************************************************************************/
/*  openxr_api.h                                                          */
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
#include "action_map/openxr_action.h"
#include "core/error/error_macros.h"
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/string/ustring.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "servers/xr/xr_pose.h"
#include "util.h"

// Name we add to our extensions if OpenXR 1.1 context is available.
#define XR_OPENXR_1_1_NAME "OPENXR_1_1"

#define XR_API_VERSION_1_1_0 XR_MAKE_VERSION(1, 1, 0)

// forward declarations, we don't want to include these fully
class OpenXRInterface;

class OpenXRAPI final
{
public:
	class OpenXRSwapChainInfo
	{
	private:
		XrSwapchain swapchain = XR_NULL_HANDLE;
		void* swapchain_graphics_data = nullptr;
		uint32_t image_index = 0;
		bool image_acquired = false;
		bool skip_acquire_swapchain = false;

		static Vector<OpenXRSwapChainInfo> free_queue;

	public:
		_FORCE_INLINE_ XrSwapchain get_swapchain() const { return swapchain; }

		_FORCE_INLINE_ bool is_image_acquired() const { return image_acquired; }

		bool create(XrSwapchainCreateFlags p_create_flags, XrSwapchainUsageFlags p_usage_flags,
			int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height,
			uint32_t p_sample_count, uint32_t p_array_size, bool p_next_extensions = false);
		void queue_free();
		static void free_queued();
		void free();

		bool acquire(bool& p_should_render);
		bool release();
		RID get_image();
		RID get_density_map();
	};

private:
	enum OpenXRSwapChainTypes
	{
		OPENXR_SWAPCHAIN_COLOR,
		OPENXR_SWAPCHAIN_DEPTH,
		OPENXR_SWAPCHAIN_MAX
	};

	// When LOCAL_FLOOR isn't supported, we use an approach based on the example code in the
	// OpenXR spec in order to emulate it.
	// See: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_local_floor
	struct LocalFloorEmulation
	{
		bool enabled = false;
		XrSpace local_space = XR_NULL_HANDLE;
		XrSpace stage_space = XR_NULL_HANDLE;
		bool should_reset_floor_height = false;
	};

	static bool reset_emulated_floor_height();

	static bool load_layer_properties();
	static bool load_supported_extensions();
	static bool is_extension_supported(const String& p_extension);
	static bool is_any_extension_enabled(const String& p_extensions);

	struct RequestExtension
	{
		String name;
		bool* enabled;
	};

	static XrResult attempt_create_instance(XrVersion p_version);

	static bool openxr_loader_init();
	static bool resolve_instance_openxr_symbols();

	// function pointers
	EXT_PROTO_XRRESULT_FUNC3(xrAcquireSwapchainImage, (XrSwapchain), swapchain,
		(const XrSwapchainImageAcquireInfo*), acquireInfo, (uint32_t*), index)
	EXT_PROTO_XRRESULT_FUNC3(xrApplyHapticFeedback, (XrSession), session,
		(const XrHapticActionInfo*), hapticActionInfo, (const XrHapticBaseHeader*), hapticFeedback)
	EXT_PROTO_XRRESULT_FUNC2(xrAttachSessionActionSets, (XrSession), session,
		(const XrSessionActionSetsAttachInfo*), attachInfo)
	EXT_PROTO_XRRESULT_FUNC2(
		xrBeginFrame, (XrSession), session, (const XrFrameBeginInfo*), frameBeginInfo)
	EXT_PROTO_XRRESULT_FUNC2(
		xrBeginSession, (XrSession), session, (const XrSessionBeginInfo*), beginInfo)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateAction, (XrActionSet), actionSet, (const XrActionCreateInfo*),
		createInfo, (XrAction*), action)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateActionSet, (XrInstance), instance,
		(const XrActionSetCreateInfo*), createInfo, (XrActionSet*), actionSet)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateActionSpace, (XrSession), session,
		(const XrActionSpaceCreateInfo*), createInfo, (XrSpace*), space)
	EXT_PROTO_XRRESULT_FUNC2(
		xrCreateInstance, (const XrInstanceCreateInfo*), createInfo, (XrInstance*), instance)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateReferenceSpace, (XrSession), session,
		(const XrReferenceSpaceCreateInfo*), createInfo, (XrSpace*), space)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSession, (XrInstance), instance, (const XrSessionCreateInfo*),
		createInfo, (XrSession*), session)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSwapchain, (XrSession), session,
		(const XrSwapchainCreateInfo*), createInfo, (XrSwapchain*), swapchain)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyAction, (XrAction), action)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyActionSet, (XrActionSet), actionSet)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyInstance, (XrInstance), instance)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySession, (XrSession), session)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpace, (XrSpace), space)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC2(
		xrEndFrame, (XrSession), session, (const XrFrameEndInfo*), frameEndInfo)
	EXT_PROTO_XRRESULT_FUNC1(xrEndSession, (XrSession), session)
	EXT_PROTO_XRRESULT_FUNC3(xrEnumerateApiLayerProperties, (uint32_t), propertyCapacityInput,
		(uint32_t*), propertyCountOutput, (XrApiLayerProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateEnvironmentBlendModes, (XrInstance), instance, (XrSystemId),
		systemId, (XrViewConfigurationType), viewConfigurationType, (uint32_t),
		environmentBlendModeCapacityInput, (uint32_t*), environmentBlendModeCountOutput,
		(XrEnvironmentBlendMode*), environmentBlendModes)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateInstanceExtensionProperties, (const char*), layerName,
		(uint32_t), propertyCapacityInput, (uint32_t*), propertyCountOutput,
		(XrExtensionProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateReferenceSpaces, (XrSession), session, (uint32_t),
		spaceCapacityInput, (uint32_t*), spaceCountOutput, (XrReferenceSpaceType*), spaces)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainFormats, (XrSession), session, (uint32_t),
		formatCapacityInput, (uint32_t*), formatCountOutput, (int64_t*), formats)
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateViewConfigurations, (XrInstance), instance, (XrSystemId),
		systemId, (uint32_t), viewConfigurationTypeCapacityInput, (uint32_t*),
		viewConfigurationTypeCountOutput, (XrViewConfigurationType*), viewConfigurationTypes)
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateViewConfigurationViews, (XrInstance), instance,
		(XrSystemId), systemId, (XrViewConfigurationType), viewConfigurationType, (uint32_t),
		viewCapacityInput, (uint32_t*), viewCountOutput, (XrViewConfigurationView*), views)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateBoolean, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateBoolean*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateFloat, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateFloat*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateVector2f, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateVector2f*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetCurrentInteractionProfile, (XrSession), session, (XrPath),
		topLevelUserPath, (XrInteractionProfileState*), interactionProfile)
	EXT_PROTO_XRRESULT_FUNC2(xrGetInstanceProperties, (XrInstance), instance,
		(XrInstanceProperties*), instanceProperties)
	EXT_PROTO_XRRESULT_FUNC3(xrGetReferenceSpaceBoundsRect, (XrSession), session,
		(XrReferenceSpaceType), referenceSpaceType, (XrExtent2Df*), bounds)
	EXT_PROTO_XRRESULT_FUNC3(xrGetSystem, (XrInstance), instance, (const XrSystemGetInfo*), getInfo,
		(XrSystemId*), systemId)
	EXT_PROTO_XRRESULT_FUNC3(xrGetSystemProperties, (XrInstance), instance, (XrSystemId), systemId,
		(XrSystemProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC4(xrLocateSpace, (XrSpace), space, (XrSpace), baseSpace, (XrTime), time,
		(XrSpaceLocation*), location)
	EXT_PROTO_XRRESULT_FUNC6(xrLocateViews, (XrSession), session, (const XrViewLocateInfo*),
		viewLocateInfo, (XrViewState*), viewState, (uint32_t), viewCapacityInput, (uint32_t*),
		viewCountOutput, (XrView*), views)
	EXT_PROTO_XRRESULT_FUNC5(xrPathToString, (XrInstance), instance, (XrPath), path, (uint32_t),
		bufferCapacityInput, (uint32_t*), bufferCountOutput, (char*), buffer)
	EXT_PROTO_XRRESULT_FUNC2(xrPollEvent, (XrInstance), instance, (XrEventDataBuffer*), eventData)
	EXT_PROTO_XRRESULT_FUNC2(xrReleaseSwapchainImage, (XrSwapchain), swapchain,
		(const XrSwapchainImageReleaseInfo*), releaseInfo)
	EXT_PROTO_XRRESULT_FUNC3(
		xrResultToString, (XrInstance), instance, (XrResult), value, (char*), buffer)
	EXT_PROTO_XRRESULT_FUNC3(
		xrStringToPath, (XrInstance), instance, (const char*), pathString, (XrPath*), path)
	EXT_PROTO_XRRESULT_FUNC2(xrSuggestInteractionProfileBindings, (XrInstance), instance,
		(const XrInteractionProfileSuggestedBinding*), suggestedBindings)
	EXT_PROTO_XRRESULT_FUNC2(
		xrSyncActions, (XrSession), session, (const XrActionsSyncInfo*), syncInfo)
	EXT_PROTO_XRRESULT_FUNC3(xrWaitFrame, (XrSession), session, (const XrFrameWaitInfo*),
		frameWaitInfo, (XrFrameState*), frameState)
	EXT_PROTO_XRRESULT_FUNC2(
		xrWaitSwapchainImage, (XrSwapchain), swapchain, (const XrSwapchainImageWaitInfo*), waitInfo)

	// instance
	static bool create_instance();
	static bool get_system_info();
	static bool load_supported_view_configuration_types();
	static bool load_supported_environmental_blend_modes();
	static bool is_view_configuration_supported(XrViewConfigurationType p_configuration_type);
	static bool load_supported_view_configuration_views(XrViewConfigurationType p_configuration_type);
	static void destroy_instance();

	// session
	static bool create_session();
	static bool load_supported_reference_spaces();
	static bool is_reference_space_supported(XrReferenceSpaceType p_reference_space);
	static bool setup_play_space();
	static bool setup_view_space();
	static bool load_supported_swapchain_formats();
	static bool is_swapchain_format_supported(int64_t p_swapchain_format);
	static bool obtain_swapchain_formats();
	static bool create_main_swapchains(const Size2i& p_size);
	static void free_main_swapchains();
	static void destroy_session();

	// action map
	struct Tracker
	{				 // Trackers represent tracked physical objects such as controllers, pucks, etc.
		String name; // Name for this tracker (i.e. "/user/hand/left")
		XrPath toplevel_path;	// OpenXR XrPath for this tracker
		RID active_profile_rid; // RID of the active profile for this tracker
	};
	static RID get_tracker_rid(XrPath p_path); // If true we need to check for updates to our active_profile_rid.

	struct ActionSet
	{						// Action sets define a set of actions that can be enabled together
		String name;		// Name for this action set (i.e. "godot_action_set")
		bool is_attached;	// If true our action set has been attached to the session and can no
							// longer be modified
		XrActionSet handle; // OpenXR handle for this action set
	};

	struct ActionTracker
	{							 // Links and action to a tracker
		RID tracker_rid;		 // RID of the tracker
		XrSpace space;			 // Optional space for pose actions
		bool was_location_valid; // If true the last position we obtained was valid
	};

	struct Action
	{									// Actions define the inputs and outputs in OpenXR
		RID action_set_rid;				// RID of the action set this action belongs to
		String name;					// Name for this action (i.e. "aim_pose")
		XrActionType action_type;		// Type of action (bool, float, etc.)
		Vector<ActionTracker> trackers; // The trackers this action can be used with
		XrAction handle;				// OpenXR handle for this action
	};
	static RID get_action_rid(XrAction p_action);

	struct InteractionProfile
	{ // Interaction profiles define suggested bindings between the physical inputs on controller
	  // types and our actions
		String name; // Name of the interaction profile (i.e.
					 // "/interaction_profiles/valve/index_controller")
		CharString
			internal_name; // Internal name of the interaction profile (translated if required)
		XrPath path;	   // OpenXR path for this profile
		Vector<XrActionSuggestedBinding> bindings; // OpenXR action bindings
		Vector<PackedByteArray>
			modifiers; // Array of modifiers we'll add into XrBindingModificationsKHR
	};
	static RID get_interaction_profile_rid(XrPath p_path);
	static XrPath get_interaction_profile_path(RID p_interaction_profile);

	static CharString get_interaction_profile_internal_name(
		const String& p_interaction_profile_name);
	static const char* check_profile_path(
		const CharString& p_interaction_profile_name, const char* p_path);

	struct OrderedCompositionLayer
	{
		const XrCompositionLayerBaseHeader* composition_layer;
		int sort_order;

		_FORCE_INLINE_ bool operator()(
			const OrderedCompositionLayer& a, const OrderedCompositionLayer& b) const
		{
			return a.sort_order < b.sort_order ||
				   (a.sort_order == b.sort_order &&
					   uint64_t(a.composition_layer) < uint64_t(b.composition_layer));
		}
	};

	// state changes
	static bool poll_events();
	static bool on_state_idle();
	static bool on_state_ready();
	static bool on_state_synchronized();
	static bool on_state_visible();
	static bool on_state_focused();
	static bool on_state_stopping();
	static bool on_state_loss_pending();
	static bool on_state_exiting();

	// convenience
	static void copy_string_to_char_buffer(const String& p_string, char* p_buffer, int p_buffer_len);

	// Render state, Only accessible in rendering thread
	struct RenderState
	{
		bool running = false;
		bool should_render = false;
		bool has_xr_viewport = false;
		XrTime predicted_display_time = 0;
		XrSpace play_space = XR_NULL_HANDLE;
		XrEnvironmentBlendMode environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		double render_target_size_multiplier = 1.0;
		uint64_t frame = 0;
		Rect2i render_region;

		LocalVector<XrView> views;
		LocalVector<XrCompositionLayerProjectionView> projection_views;
		LocalVector<XrCompositionLayerDepthInfoKHR>
			depth_views; // Only used by Composition Layer Depth Extension if available
		bool submit_depth_buffer = false;  // if set to true we submit depth buffers to OpenXR if a
										   // suitable extension is enabled.
		bool use_subsampled_images = true; // We need to default to true for the warning to be shown
										   // if we fallback immediately at startup.
		bool view_pose_valid = false;

		double z_near = 0.0;
		double z_far = 0.0;

		XrCompositionLayerProjection projection_layer = {
			XR_TYPE_COMPOSITION_LAYER_PROJECTION, // type
			nullptr,							  // next
			0,									  // layerFlags
			XR_NULL_HANDLE,						  // space
			0,									  // viewCount
			nullptr								  // views
		};

		Size2i main_swapchain_size;
		OpenXRSwapChainInfo main_swapchains[OPENXR_SWAPCHAIN_MAX];
	};

	static void _allocate_view_buffers_rt(uint32_t p_view_count, bool p_submit_depth_buffer);
	static void _set_render_session_running_rt(bool p_is_running);
	static void _set_render_display_info_rt(XrTime p_predicted_display_time, bool p_should_render);
	static void _set_render_play_space_rt(uint64_t p_play_space);
	static void _set_render_environment_blend_mode_rt(int32_t p_environment_blend_mode);
	static void _set_render_state_multiplier_rt(double p_render_target_size_multiplier);
	static void _set_render_state_render_region_rt(const Rect2i& p_render_region);
	static void _update_main_swapchain_size_rt();

	static void allocate_view_buffers(uint32_t p_view_count, bool p_submit_depth_buffer);
	static void set_render_session_running(bool p_is_running);
	static void set_render_display_info(XrTime p_predicted_display_time, bool p_should_render);
	static void set_render_play_space(XrSpace p_play_space);
	static void set_render_environment_blend_mode(XrEnvironmentBlendMode p_mode);
	static void set_render_state_multiplier(double p_render_target_size_multiplier);
	static void set_render_state_render_region(const Rect2i& p_render_region);

public:
	static void update_main_swapchain_size();

	static XrVersion get_openxr_version() { return data->openxr_version; }

	static XrInstance get_instance() { return data->instance; }

	static XrSystemId get_system_id() { return data->system_id; }

	static XrSession get_session() { return data->session; }

	static XrSessionState get_session_state() { return data->session_state; }

	static String get_runtime_name() { return data->runtime_name; }

	static String get_runtime_version() { return data->runtime_version; }

	static String get_system_name() { return data->system_name; }

	static uint32_t get_vendor_id() { return data->vendor_id; }

	// helper method to convert an XrPosef to a Transform3D
	static Transform3D transform_from_pose(const XrPosef& p_pose);
	static XrPosef pose_from_transform(const Transform3D& p_transform);

	// helper method to get a valid Transform3D from an openxr space location
	static XRPose::TrackingConfidence transform_from_location(
		const XrSpaceLocation& p_location, Transform3D& r_transform);
	static XRPose::TrackingConfidence transform_from_location(
		const XrHandJointLocationEXT& p_location, Transform3D& r_transform);
	static void parse_velocities(
		const XrSpaceVelocity& p_velocity, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static XrPath get_xr_path(const String& p_path);
	static String get_xr_path_name(const XrPath& p_path);
	static bool is_top_level_path_supported(const String& p_toplevel_path);
	static bool is_interaction_profile_supported(const String& p_ip_path);
	static bool interaction_profile_supports_io_path(const String& p_ip_path, const String& p_io_path);

	static bool openxr_is_enabled(bool p_check_run_in_editor = true);

	static XrResult try_get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr);
	static XrResult get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr);
	static String get_error_string(XrResult result);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static void set_object_name(
		XrObjectType p_object_type, uint64_t p_object_handle, const String& p_object_name);
	static void begin_debug_label_region(const String& p_label_name);
	static void end_debug_label_region();
	static void insert_debug_label(const String& p_label_name);

	static OpenXRInterface* get_xr_interface() { return data->xr_interface; }

	static void set_xr_interface(OpenXRInterface* p_xr_interface);
	static void cleanup_extension_wrappers();
	static PackedStringArray get_all_requested_extensions(XrVersion p_xr_version);

	static void set_form_factor(XrFormFactor p_form_factor);

	static XrFormFactor get_form_factor() { return data->form_factor; }

	static uint32_t get_view_count();
	static void set_view_configuration(XrViewConfigurationType p_view_configuration);

	static XrViewConfigurationType get_view_configuration() { return data->view_configuration; }

	static bool set_requested_reference_space(XrReferenceSpaceType p_requested_reference_space);

	static XrReferenceSpaceType get_requested_reference_space() { return data->requested_reference_space; }

	static XrReferenceSpaceType get_reference_space() { return data->reference_space; }

	static void set_custom_play_space(XrSpace p_custom_space);

	static void set_submit_depth_buffer(bool p_submit_depth_buffer);

	static bool get_submit_depth_buffer() { return data->submit_depth_buffer; }

	static bool is_initialized();
	static bool is_running();
	static bool initialize(const String& p_rendering_driver);
	static bool initialize_session();
	static void finish();

	static _FORCE_INLINE_ XrSpace get_play_space() { return data->play_space; }

	static _FORCE_INLINE_ XrSpace get_view_space() { return data->view_space; }

	static _FORCE_INLINE_ XrTime get_predicted_display_time() { return data->frame_state.predictedDisplayTime; }

	static _FORCE_INLINE_ XrTime get_next_frame_time()
	{
		return data->frame_state.predictedDisplayTime + data->frame_state.predictedDisplayPeriod;
	}

	static _FORCE_INLINE_ bool can_render()
	{
		return data->instance != XR_NULL_HANDLE && data->session != XR_NULL_HANDLE && data->running &&
			   data->frame_state.shouldRender;
	}

	static XrHandTrackerEXT get_hand_tracker(int p_hand_index);

	static Size2 get_recommended_target_size();
	static XRPose::TrackingConfidence get_head_center(
		Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static bool get_view_transform(uint32_t p_view, Transform3D& r_transform);
	static bool get_view_projection(
		uint32_t p_view, double p_z_near, double p_z_far, Projection& p_camera_matrix);
	static Vector2 get_eye_focus(uint32_t p_view, float p_aspect);
	static bool process();

	static void pre_render();
	static bool pre_draw_viewport(RID p_render_target);
	static XrSwapchain get_color_swapchain();
	static RID get_color_texture();
	static RID get_depth_texture();
	static RID get_density_map_texture();
	static void set_velocity_texture(RID p_render_target);
	static RID get_velocity_texture();
	static void set_velocity_depth_texture(RID p_render_target);
	static RID get_velocity_depth_texture();
	static void set_velocity_target_size(const Size2i& p_target_size);
	static Size2i get_velocity_target_size();
	static const XrCompositionLayerProjection* get_projection_layer();
	static void post_draw_viewport(RID p_render_target);
	static void end_frame();

	// Display refresh rate
	static float get_display_refresh_rate();
	static void set_display_refresh_rate(float p_refresh_rate);

	// Render Target size multiplier
	static double get_render_target_size_multiplier();
	static void set_render_target_size_multiplier(double multiplier);

	static Rect2i get_render_region();
	static void set_render_region(const Rect2i& p_render_region);

	// Foveation settings
	static bool is_foveation_supported();

	static int get_foveation_level();
	static void set_foveation_level(int p_foveation_level);

	static bool get_foveation_dynamic();
	static void set_foveation_dynamic(bool p_foveation_dynamic);

	static bool get_foveation_with_subsampled_images();
	static void set_foveation_with_subsampled_images(bool p_enabled);

	// Play space.
	static Size2 get_play_space_bounds();

	// swapchains
	static PackedInt64Array get_supported_swapchain_formats();

	static int64_t get_color_swapchain_format() { return data->color_swapchain_format; }

	static int64_t get_depth_swapchain_format() { return data->depth_swapchain_format; }

	static double get_render_state_z_near() { return data->render_state.z_near; }

	static double get_render_state_z_far() { return data->render_state.z_far; }

	// action map
	static String get_default_action_map_resource_name();

	static RID tracker_create(const String& p_name);
	static String tracker_get_name(RID p_tracker);
	static void tracker_check_profile(RID p_tracker, XrSession p_session = XR_NULL_HANDLE);
	static void tracker_free(RID p_tracker);

	static RID action_set_create(
		const String& p_name, const String& p_localized_name, const int p_priority);
	static String action_set_get_name(RID p_action_set);
	static XrActionSet action_set_get_handle(RID p_action_set);
	static bool attach_action_sets(const Vector<RID>& p_action_sets);
	static void action_set_free(RID p_action_set);

	static RID action_create(RID p_action_set, const String& p_name, const String& p_localized_name,
		OpenXRAction::ActionType p_action_type, const Vector<RID>& p_trackers);
	static String action_get_name(RID p_action);
	static XrAction action_get_handle(RID p_action);
	static void action_free(RID p_action);

	static RID interaction_profile_create(const String& p_name);
	static String interaction_profile_get_name(RID p_interaction_profile);
	static void interaction_profile_clear_bindings(RID p_interaction_profile);
	static int interaction_profile_add_binding(
		RID p_interaction_profile, RID p_action, const String& p_path);
	static bool interaction_profile_add_modifier(
		RID p_interaction_profile, const PackedByteArray& p_modifier);
	static bool interaction_profile_suggest_bindings(RID p_interaction_profile);
	static void interaction_profile_free(RID p_interaction_profile);

	static RID find_tracker(const String& p_name);
	static RID find_action_set(const String& p_name);
	static RID find_action(const String& p_name, const RID& p_action_set = RID());

	static bool sync_action_sets(const Vector<RID>& p_active_sets);
	static bool get_action_bool(RID p_action, RID p_tracker);
	static float get_action_float(RID p_action, RID p_tracker);
	static Vector2 get_action_vector2(RID p_action, RID p_tracker);
	static XRPose::TrackingConfidence get_action_pose(RID p_action, RID p_tracker,
		Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static bool trigger_haptic_pulse(RID p_action, RID p_tracker, float p_frequency, float p_amplitude,
		XrDuration p_duration_ns);

	static const Vector<XrEnvironmentBlendMode> get_supported_environment_blend_modes();

	static bool is_environment_blend_mode_supported(XrEnvironmentBlendMode p_blend_mode);
	static bool set_environment_blend_mode(XrEnvironmentBlendMode p_blend_mode);

	static XrEnvironmentBlendMode get_environment_blend_mode()
	{
		return data->requested_environment_blend_mode;
	}

	enum OpenXRAlphaBlendModeSupport
	{
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE = 0,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL = 1,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING = 2,
	};

	static void set_emulate_environment_blend_mode_alpha_blend(bool p_enabled);
	static OpenXRAlphaBlendModeSupport is_environment_blend_mode_alpha_blend_supported();

public:
	struct Data
	{
#ifdef ANDROID_ENABLED
		void* openxr_loader_library_handle = nullptr;
		PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr = nullptr;
#endif
		OpenXRInterface* xr_interface = nullptr;
		LocalVector<XrApiLayerProperties> layer_properties;
		LocalVector<XrExtensionProperties> supported_extensions;
		LocalVector<CharString> enabled_extensions;
		LocalVector<XrViewConfigurationType> supported_view_configuration_types;
		LocalVector<XrReferenceSpaceType> supported_reference_spaces;
		PackedInt64Array supported_swapchain_formats;
		XrVersion openxr_version;
		String runtime_name;
		String runtime_version;
		XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		XrViewConfigurationType view_configuration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		XrReferenceSpaceType requested_reference_space = XR_REFERENCE_SPACE_TYPE_STAGE;
		XrReferenceSpaceType reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
		bool submit_depth_buffer = false;
		XrEnvironmentBlendMode environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		XrEnvironmentBlendMode requested_environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		Vector<XrEnvironmentBlendMode> supported_environment_blend_modes;
		bool emulate_environment_blend_mode_alpha_blend = false;
		XrInstance instance = XR_NULL_HANDLE;
		XrSystemId system_id = 0;
		String system_name;
		uint32_t vendor_id = 0;
		XrSystemTrackingProperties tracking_properties;
		XrSession session = XR_NULL_HANDLE;
		XrSessionState session_state = XR_SESSION_STATE_UNKNOWN;
		bool running = false;
		XrFrameState frame_state = {XR_TYPE_FRAME_STATE, nullptr, 0, 0, false};
		double render_target_size_multiplier = 1.0;
		Rect2i render_region;
		XrSystemGraphicsProperties graphics_properties;
		LocalVector<XrViewConfigurationView> view_configuration_views;
		int64_t color_swapchain_format = 0;
		int64_t depth_swapchain_format = 0;
		bool play_space_is_dirty = true;
		XrSpace play_space = XR_NULL_HANDLE;
		XrSpace custom_play_space = XR_NULL_HANDLE;
		XrSpace view_space = XR_NULL_HANDLE;
		XRPose::TrackingConfidence head_pose_confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;
		RID velocity_texture;
		RID velocity_depth_texture;
		Size2i velocity_target_size;
		LocalFloorEmulation local_floor_emulation;
		RID_Owner<Tracker, true> tracker_owner;
		bool interaction_profile_changed =
		true;
		RID_Owner<ActionSet, true> action_set_owner;
		RID_Owner<Action, true> action_owner;
		RID_Owner<InteractionProfile, true> interaction_profile_owner;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void setup();
	static void finalize();
	_FORCE_INLINE_ static bool is_allocated() { return data != nullptr; }

	OpenXRAPI() = delete;
	OpenXRAPI(const OpenXRAPI&) = delete;
	OpenXRAPI& operator=(const OpenXRAPI&) = delete;
	~OpenXRAPI() = delete;
};


