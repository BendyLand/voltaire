/**************************************************************************/
/*  openxr_interface.h                                                    */
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

// A note on multithreading and thread safety in OpenXR.
//
// Most entry points will be called from the main thread in Godot
// however a number of entry points will be called from the
// rendering thread, potentially while we're already processing
// the next frame on the main thread.
//
// OpenXR itself has been designed with threading in mind including
// a high likelihood that the XR runtime runs in separate threads
// as well.
// Hence all the frame timing information, use of swapchains and
// sync functions.
// Do note that repeated calls to tracking APIs will provide
// increasingly more accurate data for the same timestamp as
// tracking data is continuously updated.
//
// For our code we mostly implement this in our OpenXRAPI class.
// We store data accessed from the rendering thread in a separate
// struct, setting values through our renderer command queue.
//
// As some data is setup before we start rendering, and cleaned up
// after we've stopped, that is accessed directly from both threads.

#include "openxr_api.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_tracker.h"

// declare some default strings
#define INTERACTION_PROFILE_NONE "/interaction_profiles/none"

class OpenXRInterface final : public XRInterface
{
private:
	bool initialized = false;
	bool reference_stage_changing = false;
	XRInterface::TrackingStatus tracking_state;

	// At a minimum we need a tracker for our head
	Ref<XRTracker> head;
	Transform3D head_transform;
	Vector3 head_linear_velocity;
	Vector3 head_angular_velocity;
	XRPose::TrackingConfidence head_confidence;
	Transform3D transform_for_view[2]; // We currently assume 2, but could be 4 for VARJO which we
									   // do not support yet

	XRVRS xr_vrs;

	void _load_action_map();

	struct Action
	{						// An action we've registered with OpenXR
		String action_name; // Name of our action as presented to Godot (can be altered from the
							// action map)
		OpenXRAction::ActionType action_type; // The action type of this action
		RID action_rid;						  // RID of the action registered with our OpenXR API
	};

	struct ActionSet
	{							 // An action set we've registered with OpenXR
		String action_set_name;	 // Name of our action set
		bool is_active;			 // If true this action set is active and we will sync it
		Vector<Action*> actions; // List of actions in this action set
		RID action_set_rid;		 // RID of the action registered with our OpenXR API
	};

	struct Tracker
	{							 // A tracker we've registered with OpenXR
		String tracker_name;	 // Name of our tracker (can be altered from the action map)
		Vector<Action*> actions; // Actions related to this tracker
		Ref<XRTracker>
			controller_tracker;	 // Our positional tracker object that holds our tracker state
		RID tracker_rid;		 // RID of the tracker registered with our OpenXR API
		RID interaction_profile; // RID of the interaction profile bound to this tracker (can be
								 // null)
	};

	Vector<ActionSet*> action_sets;
	Vector<RID> interaction_profiles;
	Vector<Tracker*> trackers;

	ActionSet* create_action_set(
		const String& p_action_set_name, const String& p_localized_name, const int p_priority);
	void free_action_sets();

	Action* create_action(ActionSet* p_action_set, const String& p_action_name,
		const String& p_localized_name, OpenXRAction::ActionType p_action_type,
		const Vector<Tracker*> p_trackers);
	Action* find_action(const String& p_action_name);
	void free_actions(ActionSet* p_action_set);

	Tracker* find_tracker(const String& p_tracker_name, bool p_create = false);
	void handle_tracker(Tracker* p_tracker);
	void free_trackers();

	void free_interaction_profiles();

	void _set_default_pos(Transform3D& r_transform, double p_world_scale, uint64_t p_eye);

protected:

public:
	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual PackedStringArray get_suggested_tracker_names() const override;
	virtual TrackingStatus get_tracking_status() const override;

	bool is_hand_tracking_supported();
	bool is_hand_interaction_supported() const;
	bool is_eye_gaze_interaction_supported();

	bool initialize_on_startup() const;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

	virtual void trigger_haptic_pulse(const String& p_action_name, const String& p_tracker_name,
		double p_frequency, double p_amplitude, double p_duration_sec,
		double p_delay_sec = 0) override;

	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual XRInterface::PlayAreaMode get_play_area_mode() const override;
	virtual bool set_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual PackedVector3Array get_play_area() const override;

	float get_display_refresh_rate() const;
	void set_display_refresh_rate(float p_refresh_rate);

	bool is_action_set_active(const String& p_action_set) const;
	void set_action_set_active(const String& p_action_set, bool p_active);

	double get_render_target_size_multiplier() const;
	void set_render_target_size_multiplier(double multiplier);

	bool is_foveation_supported() const;

	int get_foveation_level() const;
	void set_foveation_level(int p_foveation_level);

	bool get_foveation_dynamic() const;
	void set_foveation_dynamic(bool p_foveation_dynamic);

	bool get_foveation_with_subsampled_images() const;
	void set_foveation_with_subsampled_images(bool p_enabled);

	float get_vrs_min_radius() const;
	void set_vrs_min_radius(float p_vrs_min_radius);

	float get_vrs_strength() const;
	void set_vrs_strength(float p_vrs_strength);

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(
		uint32_t p_view, const Transform3D& p_cam_transform) override;
	virtual Projection get_projection_for_view(
		uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;

	virtual Rect2i get_render_region() override;

	virtual RID get_color_texture() override;
	virtual RID get_depth_texture() override;
	virtual RID get_velocity_texture() override;
	virtual RID get_velocity_depth_texture() override;
	virtual Size2i get_velocity_target_size() override;

	virtual void process() override;
	virtual void pre_render() override;
	bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(
		RID p_render_target, const Rect2& p_screen_rect) override;
	virtual void end_frame() override;

	virtual bool is_passthrough_supported() override;
	virtual bool is_passthrough_enabled() override;
	virtual bool start_passthrough() override;
	virtual void stop_passthrough() override;

	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(XRInterface::EnvironmentBlendMode mode) override;

	void on_state_ready();
	void on_state_visible();
	void on_state_focused();
	void on_state_synchronized();
	void on_state_stopping();
	void on_state_loss_pending();
	void on_state_exiting();
	void on_reference_space_change_pending(XrReferenceSpaceType p_type);
	void on_refresh_rate_changes(float p_new_rate);
	void tracker_profile_changed(RID p_tracker, RID p_interaction_profile);

	/** Session */
	enum SessionState
	{ // Should mirror XrSessionState
		SESSION_STATE_UNKNOWN = 0,
		SESSION_STATE_IDLE = 1,
		SESSION_STATE_READY = 2,
		SESSION_STATE_SYNCHRONIZED = 3,
		SESSION_STATE_VISIBLE = 4,
		SESSION_STATE_FOCUSED = 5,
		SESSION_STATE_STOPPING = 6,
		SESSION_STATE_LOSS_PENDING = 7,
		SESSION_STATE_EXITING = 8,
	};

	SessionState get_session_state();

	/** User presence. */
	bool is_user_presence_supported() const;
	bool is_user_present() const;

	/** Hand tracking. */
	enum Hand
	{
		HAND_LEFT,
		HAND_RIGHT,
		HAND_MAX,
	};

	enum HandMotionRange
	{
		HAND_MOTION_RANGE_UNOBSTRUCTED,
		HAND_MOTION_RANGE_CONFORM_TO_CONTROLLER,
		HAND_MOTION_RANGE_MAX
	};

	void set_motion_range(const Hand p_hand, const HandMotionRange p_motion_range);
	HandMotionRange get_motion_range(const Hand p_hand) const;

	enum HandTrackedSource
	{
		HAND_TRACKED_SOURCE_UNKNOWN,
		HAND_TRACKED_SOURCE_UNOBSTRUCTED,
		HAND_TRACKED_SOURCE_CONTROLLER,
		HAND_TRACKED_SOURCE_MAX
	};

	HandTrackedSource get_hand_tracking_source(const Hand p_hand) const;

	enum HandJoints
	{
		HAND_JOINT_PALM = 0,
		HAND_JOINT_WRIST = 1,
		HAND_JOINT_THUMB_METACARPAL = 2,
		HAND_JOINT_THUMB_PROXIMAL = 3,
		HAND_JOINT_THUMB_DISTAL = 4,
		HAND_JOINT_THUMB_TIP = 5,
		HAND_JOINT_INDEX_METACARPAL = 6,
		HAND_JOINT_INDEX_PROXIMAL = 7,
		HAND_JOINT_INDEX_INTERMEDIATE = 8,
		HAND_JOINT_INDEX_DISTAL = 9,
		HAND_JOINT_INDEX_TIP = 10,
		HAND_JOINT_MIDDLE_METACARPAL = 11,
		HAND_JOINT_MIDDLE_PROXIMAL = 12,
		HAND_JOINT_MIDDLE_INTERMEDIATE = 13,
		HAND_JOINT_MIDDLE_DISTAL = 14,
		HAND_JOINT_MIDDLE_TIP = 15,
		HAND_JOINT_RING_METACARPAL = 16,
		HAND_JOINT_RING_PROXIMAL = 17,
		HAND_JOINT_RING_INTERMEDIATE = 18,
		HAND_JOINT_RING_DISTAL = 19,
		HAND_JOINT_RING_TIP = 20,
		HAND_JOINT_LITTLE_METACARPAL = 21,
		HAND_JOINT_LITTLE_PROXIMAL = 22,
		HAND_JOINT_LITTLE_INTERMEDIATE = 23,
		HAND_JOINT_LITTLE_DISTAL = 24,
		HAND_JOINT_LITTLE_TIP = 25,
		HAND_JOINT_MAX = 26,
	};

	enum HandJointFlags
	{
		HAND_JOINT_NONE = 0,
		HAND_JOINT_ORIENTATION_VALID = 1,
		HAND_JOINT_ORIENTATION_TRACKED = 2,
		HAND_JOINT_POSITION_VALID = 4,
		HAND_JOINT_POSITION_TRACKED = 8,
		HAND_JOINT_LINEAR_VELOCITY_VALID = 16,
		HAND_JOINT_ANGULAR_VELOCITY_VALID = 32,
	};

	uint32_t get_hand_joint_flags(Hand p_hand, HandJoints p_joint) const;
	Quaternion get_hand_joint_rotation(Hand p_hand, HandJoints p_joint) const;
	Vector3 get_hand_joint_position(Hand p_hand, HandJoints p_joint) const;
	float get_hand_joint_radius(Hand p_hand, HandJoints p_joint) const;

	Vector3 get_hand_joint_linear_velocity(Hand p_hand, HandJoints p_joint) const;
	Vector3 get_hand_joint_angular_velocity(Hand p_hand, HandJoints p_joint) const;

	virtual RID get_vrs_texture() override;
	virtual VRSTextureFormat get_vrs_texture_format() override;

	// Performance settings.
	enum PerfSettingsLevel
	{
		PERF_SETTINGS_LEVEL_POWER_SAVINGS,
		PERF_SETTINGS_LEVEL_SUSTAINED_LOW,
		PERF_SETTINGS_LEVEL_SUSTAINED_HIGH,
		PERF_SETTINGS_LEVEL_BOOST,
	};

	enum PerfSettingsSubDomain
	{
		PERF_SETTINGS_SUB_DOMAIN_COMPOSITING,
		PERF_SETTINGS_SUB_DOMAIN_RENDERING,
		PERF_SETTINGS_SUB_DOMAIN_THERMAL,
	};

	enum PerfSettingsNotificationLevel
	{
		PERF_SETTINGS_NOTIF_LEVEL_NORMAL,
		PERF_SETTINGS_NOTIF_LEVEL_WARNING,
		PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED,
	};

	void set_cpu_level(PerfSettingsLevel p_level);
	void set_gpu_level(PerfSettingsLevel p_level);
	void on_cpu_level_changed(PerfSettingsSubDomain p_sub_domain,
		PerfSettingsNotificationLevel p_from_level, PerfSettingsNotificationLevel p_to_level);
	void on_gpu_level_changed(PerfSettingsSubDomain p_sub_domain,
		PerfSettingsNotificationLevel p_from_level, PerfSettingsNotificationLevel p_to_level);

	OpenXRInterface();
	~OpenXRInterface();
};

// --- Folded from modules/openxr/scene/openxr_scene.h ---
#include <openxr/openxr.h>
#include "modules/modules_enabled.gen.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "servers/xr/xr_tracker.h"

class JavaObject;
class MeshInstance3D;
class Mesh;
class OpenXRAPI;
class OpenXRCompositionLayerExtension;
class OpenXRHandTrackingExtension;
class SubViewport;
class Camera3D;

///////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayer

class OpenXRCompositionLayer final : public Node3D
{
public:
	enum LayerType
	{
		LAYER_QUAD,
		LAYER_CYLINDER,
		LAYER_EQUIRECT,
	};

	// Must be identical to Filter enum definition in OpenXRCompositionLayerExtension.
	enum Filter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_CUBIC,
	};

	// Must be identical to MipmapMode enum definition in OpenXRCompositionLayerExtension.
	enum MipmapMode
	{
		MIPMAP_MODE_DISABLED,
		MIPMAP_MODE_NEAREST,
		MIPMAP_MODE_LINEAR,
	};

	// Must be identical to Wrap enum definition in OpenXRCompositionLayerExtension.
	enum Wrap
	{
		WRAP_CLAMP_TO_BORDER,
		WRAP_CLAMP_TO_EDGE,
		WRAP_REPEAT,
		WRAP_MIRRORED_REPEAT,
		WRAP_MIRROR_CLAMP_TO_EDGE,
	};

	// Must be identical to Swizzle enum definition in OpenXRCompositionLayerExtension.
	enum Swizzle
	{
		SWIZZLE_RED,
		SWIZZLE_GREEN,
		SWIZZLE_BLUE,
		SWIZZLE_ALPHA,
		SWIZZLE_ZERO,
		SWIZZLE_ONE,
	};

	// Must be identical to EyeVisibility enum definition in OpenXRCompositionLayerExtension.
	enum EyeVisibility
	{
		EYE_VISIBILITY_BOTH,
		EYE_VISIBILITY_LEFT,
		EYE_VISIBILITY_RIGHT,
	};

private:
	RID composition_layer;
	LayerType layer_type = LAYER_QUAD;

	// Quad parameters.
	Size2 quad_size = Size2(1.0, 1.0);

	// Cylinder / Equirect shared parameters.
	float radius = 1.0;
	uint32_t fallback_segments = 10;

	// Cylinder parameters.
	float aspect_ratio = 1.0;
	float central_angle = Math::PI / 2.0;

	// Equirect parameters.
	float central_horizontal_angle = Math::PI / 2.0;
	float upper_vertical_angle = Math::PI / 4.0;
	float lower_vertical_angle = Math::PI / 4.0;

	SubViewport* layer_viewport = nullptr;
	bool use_android_surface = false;
	Size2i android_surface_size = Size2i(1024, 1024);
	bool enable_hole_punch = false;
	bool alpha_blend = false;
	int sort_order = 1;
	bool protected_content = false;
	MeshInstance3D* fallback = nullptr;
	bool should_update_fallback_mesh = false;
	bool openxr_session_running = false;
	bool registered = false;

	Filter min_filter = FILTER_LINEAR;
	Filter mag_filter = FILTER_LINEAR;
	MipmapMode mipmap_mode = MIPMAP_MODE_LINEAR;
	Wrap horizontal_wrap = WRAP_CLAMP_TO_BORDER;
	Wrap vertical_wrap = WRAP_CLAMP_TO_BORDER;
	Swizzle red_swizzle = SWIZZLE_RED;
	Swizzle green_swizzle = SWIZZLE_GREEN;
	Swizzle blue_swizzle = SWIZZLE_BLUE;
	Swizzle alpha_swizzle = SWIZZLE_ALPHA;
	float max_anisotropy = 1.0;
	Color border_color = {0.0, 0.0, 0.0, 0.0};
	EyeVisibility eye_visibility = EYE_VISIBILITY_BOTH;

	bool _should_use_fallback_node();
	void _create_fallback_node();
	void _reset_fallback_material();
	void _remove_fallback_node();

	void _setup_composition_layer();
	void _clear_composition_layer();

	void _viewport_size_changed();

	Camera3D* _get_xrcamera3d_ancestor() const;

	void _notification(int p_what);

	void _on_openxr_session_begun();
	void _on_openxr_session_stopping();

	bool _should_register();

	Ref<Mesh> _create_fallback_mesh();
	XrStructureType _get_openxr_type() const;

	void update_transform();
	void update_fallback_mesh();

	static Vector<OpenXRCompositionLayer*> composition_layer_nodes;
	bool is_viewport_in_use(SubViewport* p_viewport);

public:
	LayerType get_layer_type() const { return layer_type; }

	void set_layer_viewport(SubViewport* p_viewport);
	SubViewport* get_layer_viewport() const;

	void set_use_android_surface(bool p_use_android_surface);
	bool get_use_android_surface() const;

	void set_android_surface_size(Size2i p_size);
	Size2i get_android_surface_size() const;

	bool get_enable_hole_punch() const;

	int get_sort_order() const;

	void set_alpha_blend(bool p_alpha_blend);
	bool get_alpha_blend() const;

	Ref<JavaObject> get_android_surface();
	bool is_natively_supported() const;

	void set_protected_content(bool p_protected_content);
	bool is_protected_content() const;

	void set_min_filter(Filter p_mode);
	Filter get_min_filter() const;

	void set_mag_filter(Filter p_mode);
	Filter get_mag_filter() const;

	void set_mipmap_mode(MipmapMode p_mode);
	MipmapMode get_mipmap_mode() const;

	void set_horizontal_wrap(Wrap p_mode);
	Wrap get_horizontal_wrap() const;

	void set_vertical_wrap(Wrap p_mode);
	Wrap get_vertical_wrap() const;

	void set_red_swizzle(Swizzle p_mode);
	Swizzle get_red_swizzle() const;

	void set_green_swizzle(Swizzle p_mode);
	Swizzle get_green_swizzle() const;

	void set_blue_swizzle(Swizzle p_mode);
	Swizzle get_blue_swizzle() const;

	void set_alpha_swizzle(Swizzle p_mode);
	Swizzle get_alpha_swizzle() const;

	void set_max_anisotropy(float p_value);
	float get_max_anisotropy() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_eye_visibility(EyeVisibility p_eye_visibility);
	EyeVisibility get_eye_visibility() const;

	// Quad properties.
	void set_quad_size(const Size2& p_size);
	Size2 get_quad_size() const;

	// Cylinder & Equirect shared properties.
	void set_radius(float p_radius);
	float get_radius() const;

	void set_fallback_segments(uint32_t p_fallback_segments);
	uint32_t get_fallback_segments() const;

	// Cylinder properties.
	void set_aspect_ratio(float p_aspect_ratio);
	float get_aspect_ratio() const;

	void set_central_angle(float p_angle);
	float get_central_angle() const;

	// Equirect properties.
	void set_central_horizontal_angle(float p_angle);
	float get_central_horizontal_angle() const;

	void set_upper_vertical_angle(float p_angle);
	float get_upper_vertical_angle() const;

	void set_lower_vertical_angle(float p_angle);
	float get_lower_vertical_angle() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	Vector2 intersects_ray(const Vector3& p_origin, const Vector3& p_direction) const;

	OpenXRCompositionLayer(LayerType p_type = LAYER_QUAD);
	~OpenXRCompositionLayer() = default;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRHand

class OpenXRHand final : public Node3D
{
public:
	enum Hands
	{
		HAND_LEFT,
		HAND_RIGHT,
		HAND_MAX
	};

	enum MotionRange
	{
		MOTION_RANGE_UNOBSTRUCTED,
		MOTION_RANGE_CONFORM_TO_CONTROLLER,
		MOTION_RANGE_MAX
	};

	enum SkeletonRig
	{
		SKELETON_RIG_OPENXR,
		SKELETON_RIG_HUMANOID,
		SKELETON_RIG_MAX
	};

	enum BoneUpdate
	{
		BONE_UPDATE_FULL,
		BONE_UPDATE_ROTATION_ONLY,
		BONE_UPDATE_MAX
	};

private:
	struct JointData
	{
		int bone = -1;
		int parent_joint = -1;
	};

	Hands hand = HAND_LEFT;
	MotionRange motion_range = MOTION_RANGE_UNOBSTRUCTED;
	NodePath hand_skeleton;
	SkeletonRig skeleton_rig = SKELETON_RIG_OPENXR;
	BoneUpdate bone_update = BONE_UPDATE_FULL;

	JointData joints[XR_HAND_JOINT_COUNT_EXT];

	void _set_motion_range();

	Skeleton3D* get_skeleton();
	void _get_joint_data();
	void _update_skeleton();

public:
	OpenXRHand();

	void set_hand(Hands p_hand);
	Hands get_hand() const;

	void set_motion_range(MotionRange p_motion_range);
	MotionRange get_motion_range() const;

	void set_hand_skeleton(const NodePath& p_hand_skeleton);
	NodePath get_hand_skeleton() const;

	void set_skeleton_rig(SkeletonRig p_skeleton_rig);
	SkeletonRig get_skeleton_rig() const;

	void set_bone_update(BoneUpdate p_bone_update);
	BoneUpdate get_bone_update() const;

	void _notification(int p_what);
};

///////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMask

class OpenXRVisibilityMask final : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	void _on_openxr_session_begun();
	void _on_openxr_session_stopping();

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	OpenXRVisibilityMask() = default;
	~OpenXRVisibilityMask() = default;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRRenderModel & OpenXRRenderModelManager

#ifdef MODULE_GLTF_ENABLED
class OpenXRRenderModel final : public Node3D
{
private:
	RID render_model;
	Node3D* scene = nullptr;
	HashMap<String, Node3D*> animatable_nodes;

	void _load_render_model_scene();
	void _on_render_model_top_level_path_changed(RID p_render_model);
	void _notification(int p_what);

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	RID get_render_model() const;
	void set_render_model(RID p_render_model);

	String get_top_level_path() const;
};

class OpenXRRenderModelManager final : public Node3D
{
public:
	enum RenderModelTracker
	{
		RENDER_MODEL_TRACKER_ANY,
		RENDER_MODEL_TRACKER_NONE_SET,
		RENDER_MODEL_TRACKER_LEFT_HAND,
		RENDER_MODEL_TRACKER_RIGHT_HAND,
	};

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_tracker(RenderModelTracker p_tracker);
	RenderModelTracker get_tracker() const;

	void set_make_local_to_pose(const String& p_action);
	String get_make_local_to_pose() const;

private:
	HashMap<RID, Node3D*> render_models;
	Node3D* container = nullptr;

	bool is_dirty = false;
	RenderModelTracker tracker = RENDER_MODEL_TRACKER_ANY;
	String make_local_to_pose;

	Ref<XRTracker> positional_tracker;
	XrPath xr_path = XR_NULL_PATH;

	bool _has_filters();
	void _on_render_model_added(RID p_render_model);
	void _on_render_model_removed(RID p_render_model);
	void _on_render_model_top_level_path_changed(RID p_path);
	void _update_models();
	void _notification(int p_what);
};
#endif // MODULE_GLTF_ENABLED

// --- End modules/openxr/scene/openxr_scene.h ---
