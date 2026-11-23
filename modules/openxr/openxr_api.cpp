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

#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/memory.h"
#include "core/types.h"
#include "core/version.h"
#include "openxr_api.h"
// --- Inlined/Folded from modules/openxr/extensions/openxr_extensions.h ---
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
// --- Folded from modules/openxr/openxr_interface.h ---
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

#include "modules/openxr/openxr_api.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"

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
#include "servers/xr/xr_server.h"

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

// --- End modules/openxr/openxr_interface.h ---
#include "core/math/transform_3d.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "modules/modules_enabled.gen.h"
#include "servers/xr/xr_server.h"

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
#include "modules/openxr/openxr_api.h"
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
#include "servers/xr/xr_server.h"

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
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

#ifdef ANDROID_ENABLED
#include "core/os/os.h"
#endif




#include "openxr_platform_inc.h" // IWYU pragma: keep.

#ifdef VULKAN_ENABLED
// --- Inlined/Folded from modules/openxr/extensions/platform/openxr_vulkan_extension.h ---
#include "modules/openxr/openxr_api.h"
#include "core/templates/vector.h"
#include "core/types.h"

// Always include this as late as possible.
#include "modules/openxr/openxr_platform_inc.h" // IWYU pragma: keep.

class OpenXRVulkanExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void* set_session_create_and_get_next_pointer(void* p_next_pointer);

	static bool create_vulkan_instance(
		const VkInstanceCreateInfo* p_vulkan_create_info, VkInstance* r_instance);
	static bool get_physical_device(VkPhysicalDevice* r_device);
	static bool create_vulkan_device(
		const VkDeviceCreateInfo* p_device_create_info, VkDevice* r_device);
	static void set_direct_queue_family_and_index(
		uint32_t p_queue_family_index, uint32_t p_queue_index);
	static bool use_subsampled_images();
	static bool use_fragment_density_offsets();
	static void get_fragment_density_offsets(
		LocalVector<VkOffset2D>& r_offets, const Vector2i& p_granularity);

	static void get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains);
	static void get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format,
		uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size,
		void** r_swapchain_graphics_data);
	static void cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data);
	static bool create_projection_fov(
		const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix);
	static RID get_texture(void* p_swapchain_graphics_data, int p_image_index);
	static RID get_density_map(void* p_swapchain_graphics_data, int p_image_index);

private:
	static XrGraphicsBindingVulkanKHR
		graphics_binding_vulkan; // declaring this as static so we don't need to know its size and
								 // we only need it once when creating our session

	struct SwapchainGraphicsData
	{
		bool is_multiview;
		Vector<RID> texture_rids;
		Vector<RID> density_map_rids;
	};

	static bool check_graphics_api_support(XrVersion p_desired_version);

	EXT_PROTO_XRRESULT_FUNC3(xrGetVulkanGraphicsRequirements2KHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsVulkanKHR*), p_graphics_requirements)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateVulkanInstanceKHR, (XrInstance), p_instance,
		(const XrVulkanInstanceCreateInfoKHR*), p_create_info, (VkInstance*), r_vulkan_instance,
		(VkResult*), r_vulkan_result)
	EXT_PROTO_XRRESULT_FUNC3(xrGetVulkanGraphicsDevice2KHR, (XrInstance), p_instance,
		(const XrVulkanGraphicsDeviceGetInfoKHR*), p_get_info, (VkPhysicalDevice*),
		r_vulkan_physical_device)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateVulkanDeviceKHR, (XrInstance), p_instance,
		(const XrVulkanDeviceCreateInfoKHR*), p_create_info, (VkDevice*), r_device, (VkResult*),
		r_result)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t),
		p_image_capacity_input, (uint32_t*), p_image_count_output, (XrSwapchainImageBaseHeader*),
		p_images)

public:
	struct Data
	{
		VkInstance vulkan_instance = nullptr;
		VkPhysicalDevice vulkan_physical_device = nullptr;
		VkDevice vulkan_device = nullptr;
		uint32_t vulkan_queue_family_index = UINT32_MAX;
		uint32_t vulkan_queue_index = UINT32_MAX;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRVulkanExtension() = delete;
	OpenXRVulkanExtension(const OpenXRVulkanExtension&) = delete;
	OpenXRVulkanExtension& operator=(const OpenXRVulkanExtension&) = delete;
	~OpenXRVulkanExtension() = delete;
};
#endif

#ifdef METAL_ENABLED
#include "extensions/platform/openxr_metal_extension.h"
#endif

#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
// --- Inlined/Folded from modules/openxr/extensions/platform/openxr_opengl_extension.h ---
#ifdef GLES3_ENABLED

#include "modules/openxr/openxr_api.h"
#include "core/templates/vector.h"
#include "core/types.h"

// Always include this as late as possible.
#include "modules/openxr/openxr_platform_inc.h" // IWYU pragma: keep.

class OpenXROpenGLExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void* set_session_create_and_get_next_pointer(void* p_next_pointer);

	static void get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains);
	static void get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format,
		uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size,
		void** r_swapchain_graphics_data);
	static void cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data);
	static bool create_projection_fov(
		const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix);
	static RID get_texture(void* p_swapchain_graphics_data, int p_image_index);

	static RID get_density_map(void* p_swapchain_graphics_data, int p_image_index)
	{
		return RID();
	}

private:

#ifdef WIN32
	static XrGraphicsBindingOpenGLWin32KHR graphics_binding_gl;
#elif defined(ANDROID_ENABLED)
	static XrGraphicsBindingOpenGLESAndroidKHR graphics_binding_gl;
#elif defined(LINUXBSD_ENABLED)
#ifdef X11_ENABLED
	static XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
#endif
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	static XrGraphicsBindingEGLMNDX graphics_binding_egl;

#endif
#else
#error "OpenXR with OpenGL isn't supported on this platform"
#endif

	struct SwapchainGraphicsData
	{
		bool is_multiview;
		Vector<RID> texture_rids;
	};

	static bool check_graphics_api_support(XrVersion p_desired_version);

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	EXT_PROTO_XRRESULT_FUNC3(xrGetOpenGLESGraphicsRequirementsKHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsOpenGLESKHR*), p_graphics_requirements)
#else
	EXT_PROTO_XRRESULT_FUNC3(xrGetOpenGLGraphicsRequirementsKHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsOpenGLKHR*), p_graphics_requirements)
#endif
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t),
		p_image_capacity_input, (uint32_t*), p_image_count_output, (XrSwapchainImageBaseHeader*),
		p_images)

public:
	struct Data
	{
#if defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
		bool egl_extension_enabled = false;
#endif
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXROpenGLExtension() = delete;
	OpenXROpenGLExtension(const OpenXROpenGLExtension&) = delete;
	OpenXROpenGLExtension& operator=(const OpenXROpenGLExtension&) = delete;
	~OpenXROpenGLExtension() = delete;
};

#endif // GLES3_ENABLED
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

#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"


#include "modules/modules_enabled.gen.h"
#include "openxr_api.h"

#include "modules/register_module_types.h"


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
#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"


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


#include "modules/openxr/openxr_api.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"

#include "platform/android/api/java_class_wrapper.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/node_3d.h"
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

// ============================================================================
// Merged from modules/openxr/extensions/openxr_extensions.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/string/print_string.h"

#include "platform/android/api/java_class_wrapper.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer.h"
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
#include "modules/openxr/openxr_platform_inc.h"
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

#include "modules/openxr/openxr_api.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"


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


// --- Merged from modules/openxr/extensions/platform/openxr_vulkan_extension.cpp ---
#if defined(VULKAN_ENABLED)
#include "modules/openxr/openxr_api.h"

#include "core/string/print_string.h"

#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

void OpenXRVulkanExtension::initialize()
{
	if (data == nullptr) {
		data = memnew(Data);
		VulkanHooks::bind<OpenXRVulkanExtension>();
	}
}

void OpenXRVulkanExtension::finalize()
{
	if (data != nullptr) {
		VulkanHooks::reset();
		memdelete(data);
		data = nullptr;
	}
}

HashMap<String, bool*> OpenXRVulkanExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME] = nullptr; // must be available

	return request_extensions;
}

void OpenXRVulkanExtension::on_instance_created(const XrInstance p_instance)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	// Obtain pointers to functions we're accessing here, they are (not yet) part of core.

	EXT_INIT_XR_FUNC(xrGetVulkanGraphicsRequirements2KHR);
	EXT_INIT_XR_FUNC(xrCreateVulkanInstanceKHR);
	EXT_INIT_XR_FUNC(xrGetVulkanGraphicsDevice2KHR);
	EXT_INIT_XR_FUNC(xrCreateVulkanDeviceKHR);
	EXT_INIT_XR_FUNC(xrEnumerateSwapchainImages);
}

void OpenXRVulkanExtension::set_direct_queue_family_and_index(
	uint32_t p_queue_family_index, uint32_t p_queue_index)
{
	ERR_FAIL_NULL(data);
	data->vulkan_queue_family_index = p_queue_family_index;
	data->vulkan_queue_index = p_queue_index;
}

bool OpenXRVulkanExtension::use_fragment_density_offsets()
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return false;
	}

	return OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled();
}

void OpenXRVulkanExtension::get_fragment_density_offsets(
	LocalVector<VkOffset2D>& r_vk_offsets, const Vector2i& p_granularity)
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return;
	}

	LocalVector<Vector2i> offsets;
	OpenXRFBFoveationExtension::get_fragment_density_offsets(offsets);

	r_vk_offsets.reserve(offsets.size());
	for (Vector2i offset : offsets) {
		offset = ((offset + p_granularity / 2) / p_granularity) * p_granularity;

		r_vk_offsets.push_back(VkOffset2D{offset.x, offset.y});
	}
}

bool OpenXRVulkanExtension::use_subsampled_images()
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return false;
	}

	return OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled();
}

XrGraphicsBindingVulkanKHR OpenXRVulkanExtension::graphics_binding_vulkan;

void* OpenXRVulkanExtension::set_session_create_and_get_next_pointer(void* p_next_pointer)
{
	DEV_ASSERT(data->vulkan_queue_family_index < UINT32_MAX &&
			   "Direct queue family index was not specified yet.");
	DEV_ASSERT(data->vulkan_queue_index < UINT32_MAX && "Direct queue index was not specified yet.");

	graphics_binding_vulkan.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
	graphics_binding_vulkan.next = p_next_pointer;
	graphics_binding_vulkan.instance = data->vulkan_instance;
	graphics_binding_vulkan.physicalDevice = data->vulkan_physical_device;
	graphics_binding_vulkan.device = data->vulkan_device;
	graphics_binding_vulkan.queueFamilyIndex = data->vulkan_queue_family_index;
	graphics_binding_vulkan.queueIndex = data->vulkan_queue_index;

	return &graphics_binding_vulkan;
}

void OpenXRVulkanExtension::get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains)
{
	// We might want to do more here especially if we keep things in linear color space
	// Possibly add in R10G10B10A2 as an option if we're using the mobile renderer.
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_UINT);
}

void OpenXRVulkanExtension::get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains)
{
	// Note, it is very likely we do NOT support any of depth formats where we can combine our
	// stencil support (e.g. _S8_UINT). Right now this isn't a problem but once stencil support
	// becomes an issue, we need to check for this in the rendering engine and create a separate
	// buffer for the stencil.

	p_usable_swap_chains.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_D32_SFLOAT);
}

bool OpenXRVulkanExtension::create_projection_fov(
	const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix)
{
	OpenXRUtil::XrMatrix4x4f matrix;
	OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(&matrix, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXRVulkanExtension::get_texture(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->texture_rids.size(), RID());
	return data->texture_rids[p_image_index];
}

RID OpenXRVulkanExtension::get_density_map(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->density_map_rids.size(), RID());
	return data->density_map_rids[p_image_index];
}

void OpenXRVulkanExtension::cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data)
{
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData* data = (SwapchainGraphicsData*)*p_swapchain_graphics_data;

	ERR_FAIL_NULL(Renderer::data);
	ERR_FAIL_NULL(RD::data);

	for (const RID& texture_rid : data->texture_rids) {
		// This should clean up our RIDs and associated texture objects but shouldn't destroy the
		// images, they are owned by our XrSwapchain.
		RD::free_rid(texture_rid);
	}
	data->texture_rids.clear();

	for (int i = 0; i < data->density_map_rids.size(); i++) {
		if (data->density_map_rids[i].is_valid()) {
			RD::free_rid(data->density_map_rids[i]);
		}
	}
	data->density_map_rids.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

#define ENUM_TO_STRING_CASE(e)                                                                     \
	case e: {                                                                                      \
		return String(#e);                                                                         \
	} break;

String OpenXRVulkanExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	// This really should be in vulkan_context...
	VkFormat format = VkFormat(p_swapchain_format);
	switch (format) {
		ENUM_TO_STRING_CASE(VK_FORMAT_UNDEFINED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4_UNORM_PACK8)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G6B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G6R5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_X8_D24_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D24_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_UFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_SFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8B8G8R8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8G8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16B16G16R16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B16G16R16G16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_MAX_ENUM)
	default: {
		return String("Swapchain format ") + String::num_int64(int64_t(p_swapchain_format));
	} break;
	}
}

bool OpenXRVulkanExtension::create_vulkan_instance(
	const VkInstanceCreateInfo* p_create_info, VkInstance* r_instance)
{
	return true;
}

bool OpenXRVulkanExtension::get_physical_device(VkPhysicalDevice* r_physical_device)
{
	return true;
}

bool OpenXRVulkanExtension::create_vulkan_device(
	const VkDeviceCreateInfo* p_create_info, VkDevice* r_device)
{
	return true;
}

bool OpenXRVulkanExtension::get_swapchain_image_data(XrSwapchain_T* p_swapchain, long p_index,
	unsigned int p_width, unsigned int p_height, unsigned int p_layers, unsigned int p_format,
	void** r_image_data)
{
	return true;
}
#ifdef ENUM_TO_STRING_CASE
#undef ENUM_TO_STRING_CASE
#endif
#endif // defined(VULKAN_ENABLED)


// --- Merged from modules/openxr/extensions/platform/openxr_opengl_extension.cpp ---
#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)


#ifdef GLES3_ENABLED

#include "modules/openxr/openxr_api.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "servers/display/display_server.h"

void OpenXROpenGLExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXROpenGLExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


// OpenXR requires us to submit sRGB textures so that it recognizes the content
// as being in sRGB color space. We do fall back on "normal" textures but this
// will likely result in incorrect colors as OpenXR will double the sRGB conversion.
// All major XR runtimes support sRGB textures.

// In OpenGL output of the fragment shader is assumed to be in the color space of
// the developers choice, however a linear to sRGB HW conversion can be enabled
// through enabling GL_FRAMEBUFFER_SRGB if an sRGB color attachment is used.
// This is a global setting.
// See: https://www.khronos.org/opengl/wiki/Framebuffer

// In OpenGLES output of the fragment shader is assumed to be in linear color space
// and will be converted by default to sRGB if an sRGB color attachment is used.
// The extension GL_EXT_sRGB_write_control was introduced to enable turning this
// feature off.
// See: https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt

HashMap<String, bool*> OpenXROpenGLExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif
#if defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	request_extensions[XR_MNDX_EGL_ENABLE_EXTENSION_NAME] = &data->egl_extension_enabled;
#endif

	return request_extensions;
}

void OpenXROpenGLExtension::on_instance_created(const XrInstance p_instance)
{
	// Obtain pointers to functions we're accessing here.
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	EXT_INIT_XR_FUNC(xrGetOpenGLESGraphicsRequirementsKHR);
#else
	EXT_INIT_XR_FUNC(xrGetOpenGLGraphicsRequirementsKHR);
#endif
	EXT_INIT_XR_FUNC(xrEnumerateSwapchainImages);
}

#ifdef WIN32
XrGraphicsBindingOpenGLWin32KHR OpenXROpenGLExtension::graphics_binding_gl;
#elif defined(ANDROID_ENABLED)
XrGraphicsBindingOpenGLESAndroidKHR OpenXROpenGLExtension::graphics_binding_gl;
#elif defined(LINUXBSD_ENABLED)
#ifdef X11_ENABLED
XrGraphicsBindingOpenGLXlibKHR OpenXROpenGLExtension::graphics_binding_gl;
#endif
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
XrGraphicsBindingEGLMNDX OpenXROpenGLExtension::graphics_binding_egl;
#endif
#endif

void* OpenXROpenGLExtension::set_session_create_and_get_next_pointer(void* p_next_pointer)
{
	GLint gl_version_major = 0;
	GLint gl_version_minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &gl_version_major);
	glGetIntegerv(GL_MINOR_VERSION, &gl_version_minor);

	XrVersion desired_version = XR_MAKE_VERSION(gl_version_major, gl_version_minor, 0);

	if (!check_graphics_api_support(desired_version)) {
		__print_line("OpenXR: Trying to initialize with OpenGL anyway...");
		// return p_next_pointer;
	}

#ifdef WIN32
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
	graphics_binding_gl.next = p_next_pointer;

	graphics_binding_gl.hDC =
		(HDC)DisplayServer::window_get_native_handle(DisplayServerEnums::WINDOW_VIEW);
	graphics_binding_gl.hGLRC =
		(HGLRC)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
#elif defined(ANDROID_ENABLED)
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
	graphics_binding_gl.next = p_next_pointer;

	graphics_binding_gl.display =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::DISPLAY_HANDLE);
	graphics_binding_gl.config =
		(EGLConfig)0; // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/graphicsplugin_opengles.cpp#L122
	graphics_binding_gl.context =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
#else
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	if (DisplayServer::get_name() == "Wayland") {
		ERR_FAIL_COND_V_MSG(!data->egl_extension_enabled, p_next_pointer,
			"OpenXR cannot initialize on Wayland without the XR_MNDX_egl_enable extension.");

		graphics_binding_egl.type = XR_TYPE_GRAPHICS_BINDING_EGL_MNDX;
		graphics_binding_egl.next = p_next_pointer;

		graphics_binding_egl.getProcAddress = eglGetProcAddress;
		graphics_binding_egl.display =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::EGL_DISPLAY);
		graphics_binding_egl.config =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::EGL_CONFIG);
		graphics_binding_egl.context =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);

		return &graphics_binding_egl;
	}
#endif
#if defined(X11_ENABLED)
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
	graphics_binding_gl.next = p_next_pointer;

	void* display_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::DISPLAY_HANDLE);
	void* glxcontext_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
	void* glxdrawable_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::WINDOW_HANDLE);
	void* glx_fbconfig_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::GLX_FBCONFIG);
	VisualID glx_visualid =
		(VisualID)DisplayServer::window_get_native_handle(DisplayServerEnums::GLX_VISUALID);

	graphics_binding_gl.xDisplay = (Display*)display_handle;
	graphics_binding_gl.glxContext = (GLXContext)glxcontext_handle;
	graphics_binding_gl.glxDrawable = (GLXDrawable)glxdrawable_handle;
	graphics_binding_gl.glxFBConfig = (GLXFBConfig)glx_fbconfig_handle;
	graphics_binding_gl.visualid = glx_visualid;
#endif
#endif

#if defined(WIN32) || defined(ANDROID_ENABLED) || defined(X11_ENABLED)
	return &graphics_binding_gl;
#else
	return p_next_pointer;
#endif
}

void OpenXROpenGLExtension::get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains)
{
	p_usable_swap_chains.push_back(GL_SRGB8_ALPHA8);
	p_usable_swap_chains.push_back(GL_RGBA8);
}

void OpenXROpenGLExtension::get_usable_depth_formats(Vector<int64_t>& p_usable_depth_formats)
{
	p_usable_depth_formats.push_back(GL_DEPTH_COMPONENT32F);
	p_usable_depth_formats.push_back(GL_DEPTH24_STENCIL8);
	p_usable_depth_formats.push_back(GL_DEPTH32F_STENCIL8);
	p_usable_depth_formats.push_back(GL_DEPTH_COMPONENT24);
}

bool OpenXROpenGLExtension::get_swapchain_image_data(XrSwapchain p_swapchain,
	int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count,
	uint32_t p_array_size, void** r_swapchain_graphics_data)
{
	uint32_t swapchain_length;
	XrResult result = xrEnumerateSwapchainImages(p_swapchain, 0, &swapchain_length, nullptr);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain image count [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	LocalVector<XrSwapchainImageOpenGLESKHR> images;
#else
	LocalVector<XrSwapchainImageOpenGLKHR> images;
#endif
	images.resize(swapchain_length);

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	for (XrSwapchainImageOpenGLESKHR& image : images) {
		image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#else
	for (XrSwapchainImageOpenGLKHR& image : images) {
		image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
		image.next = nullptr;
		image.image = 0;
	}

	result = xrEnumerateSwapchainImages(p_swapchain, swapchain_length, &swapchain_length,
		(XrSwapchainImageBaseHeader*)images.ptr());
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain images [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	SwapchainGraphicsData* data = memnew(SwapchainGraphicsData);
	if (data == nullptr) {
		// print_line("OpenXR: Failed to allocate memory for swapchain data");
		return false;
	}
	*r_swapchain_graphics_data = data;
	data->is_multiview = (p_array_size > 1);

	Image::Format format = Image::FORMAT_RGBA8;

	Vector<RID> texture_rids;

	for (uint64_t i = 0; i < swapchain_length; i++) {
		RID texture_rid = GLES3::TextureStorage::texture_create_from_native_handle(
			p_array_size == 1 ? RSE::TEXTURE_TYPE_2D : RSE::TEXTURE_TYPE_LAYERED, format,
			images[i].image, p_width, p_height, 1, p_array_size);

		texture_rids.push_back(texture_rid);
	}

	data->texture_rids = texture_rids;

	return true;
}

bool OpenXROpenGLExtension::create_projection_fov(
	const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix)
{
	OpenXRUtil::XrMatrix4x4f matrix;
	OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(&matrix, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXROpenGLExtension::get_texture(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->texture_rids.size(), RID());
	return data->texture_rids[p_image_index];
}

void OpenXROpenGLExtension::cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data)
{
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData* data = (SwapchainGraphicsData*)*p_swapchain_graphics_data;

	for (const RID& texture_rid : data->texture_rids) {
		GLES3::TextureStorage::texture_free(texture_rid);
	}
	data->texture_rids.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

#define ENUM_TO_STRING_CASE(e)                                                                     \
	case e: {                                                                                      \
		return String(#e);                                                                         \
	} break;

String OpenXROpenGLExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	// These are somewhat different per platform, will need to weed some stuff out...
	switch (p_swapchain_format) {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		// using definitions from GLES3/gl3.h

		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGB565)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_10F_11F_11F_REV)
		ENUM_TO_STRING_CASE(GL_RGB9_E5)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_5_9_9_9_REV)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RG)
		ENUM_TO_STRING_CASE(GL_RG_INTEGER)
		ENUM_TO_STRING_CASE(GL_R8)
		ENUM_TO_STRING_CASE(GL_RG8)
		ENUM_TO_STRING_CASE(GL_R16F)
		ENUM_TO_STRING_CASE(GL_R32F)
		ENUM_TO_STRING_CASE(GL_RG16F)
		ENUM_TO_STRING_CASE(GL_RG32F)
		ENUM_TO_STRING_CASE(GL_R8I)
		ENUM_TO_STRING_CASE(GL_R8UI)
		ENUM_TO_STRING_CASE(GL_R16I)
		ENUM_TO_STRING_CASE(GL_R16UI)
		ENUM_TO_STRING_CASE(GL_R32I)
		ENUM_TO_STRING_CASE(GL_R32UI)
		ENUM_TO_STRING_CASE(GL_RG8I)
		ENUM_TO_STRING_CASE(GL_RG8UI)
		ENUM_TO_STRING_CASE(GL_RG16I)
		ENUM_TO_STRING_CASE(GL_RG16UI)
		ENUM_TO_STRING_CASE(GL_RG32I)
		ENUM_TO_STRING_CASE(GL_RG32UI)
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGBA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)

#else
		// using definitions from GLAD
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_R16_SNORM)
		ENUM_TO_STRING_CASE(GL_RG16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB4)
		ENUM_TO_STRING_CASE(GL_RGB5)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGB10)
		ENUM_TO_STRING_CASE(GL_RGB12)
		ENUM_TO_STRING_CASE(GL_RGB16)
		ENUM_TO_STRING_CASE(GL_RGBA2)
		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA12)
		ENUM_TO_STRING_CASE(GL_RGBA16)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB_ALPHA)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32F)
		ENUM_TO_STRING_CASE(GL_DEPTH32F_STENCIL8)
#endif
	default: {
		return String("Swapchain format 0x") + String::num_int64(p_swapchain_format, 16);
	} break;
	}
}

bool OpenXROpenGLExtension::check_graphics_api_support(unsigned long p_api) { return false; }

#endif // GLES3_ENABLED
#ifdef ENUM_TO_STRING_CASE
#undef ENUM_TO_STRING_CASE
#endif
#endif // defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
