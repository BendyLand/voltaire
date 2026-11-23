/**************************************************************************/
/*  node_3d.h                                                             */
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

#include "scene/main/node.h"
// --- Inlined/Folded from scene/resources/3d/world_3d.h ---
#include "core/io/resource.h"
#include "scene/resources/environment.h"

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

class CameraAttributes;
class Camera3D;
class Compositor;
class VisibleOnScreenNotifier3D;
struct SpatialIndexer;

class World3D : public Resource
{
private:
	RID scenario;
	mutable RID space;
#ifndef NAVIGATION_3D_DISABLED
	mutable RID navigation_map;
#endif // NAVIGATION_3D_DISABLED

	Ref<Environment> environment;
	Ref<Environment> fallback_environment;
	Ref<CameraAttributes> camera_attributes;
	Ref<Compositor> compositor;

	HashSet<Camera3D*> cameras;

protected:

	friend class Camera3D;

	void _register_camera(Camera3D* p_camera);
	void _remove_camera(Camera3D* p_camera);

public:
	RID get_space() const;
#ifndef NAVIGATION_3D_DISABLED
	RID get_navigation_map() const;
#endif // NAVIGATION_3D_DISABLED
	RID get_scenario() const;

	void set_environment(const Ref<Environment>& p_environment);
	Ref<Environment> get_environment() const;

	void set_fallback_environment(const Ref<Environment>& p_environment);
	Ref<Environment> get_fallback_environment() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	void set_compositor(const Ref<Compositor>& p_compositor);
	Ref<Compositor> get_compositor() const;

	_FORCE_INLINE_ const HashSet<Camera3D*>& get_cameras() const { return cameras; }

#ifndef PHYSICS_3D_DISABLED
	PhysicsDirectSpaceState3D* get_direct_space_state();
#endif // PHYSICS_3D_DISABLED

	World3D();
	~World3D();
};
#include "scene/resources/environment.h"
#include "servers/rendering/rendering_server_enums.h"

struct CollisionState3D;

class Node3DGizmo : public RefCounted
{
public:
	virtual void create() = 0;
	virtual void transform() = 0;
	virtual void clear() = 0;
	virtual void redraw() = 0;
	virtual void free() = 0;

	Node3DGizmo();

	virtual ~Node3DGizmo() {}
};

class Node3D : public Node
{
	friend class SceneTreeFTI;
	friend class SceneTreeFTITests;
	friend struct RenderInstance3D;

public:
	// Edit mode for the rotation.
	// THIS MODE ONLY AFFECTS HOW DATA IS EDITED AND SAVED
	// IT DOES _NOT_ AFFECT THE TRANSFORM LOGIC (see comment in TransformDirty).
	enum RotationEditMode
	{
		ROTATION_EDIT_MODE_EULER,
		ROTATION_EDIT_MODE_QUATERNION,
		ROTATION_EDIT_MODE_BASIS,
	};

private:
	// For the sake of ease of use, Node3D can operate with Transforms (Basis+Origin),
	// Quaternion/Scale and Euler Rotation/Scale. Transform and Quaternion are stored in
	// data.local_transform Basis (so quaternion is not really stored, but converted back/forth from
	// 3x3 matrix on demand). Euler needs to be kept separate because converting to Basis and back
	// may result in a different vector (which is troublesome for users editing in the inspector,
	// not only because of the numerical precision loss but because they expect these rotations to
	// be consistent, or support "redundant" rotations for animation interpolation, like going from
	// 0 to 720 degrees).
	//
	// As such, the system works in a way where if the local transform is set (via
	// transform/basis/quaternion), the EULER rotation and scale becomes dirty. It will remain dirty
	// until reading back is attempted (for performance reasons). Likewise, if the Euler rotation
	// scale are set, the local transform will become dirty (and again, will not become valid again
	// until read).
	//
	// All this is transparent from outside the Node3D API, which allows everything to works by
	// calling these functions in exchange.
	//
	// Additionally, setting either transform, quaternion, Euler rotation or scale makes the global
	// transform dirty, which will be updated when read again.
	//
	// NOTE: Again, RotationEditMode is _independent_ of this mechanism, it is only meant to expose
	// the right set of properties for editing (editor) and saving (to scene, in order to keep the
	// same values and avoid data loss on conversions). It has zero influence in the logic described
	// above.
	enum TransformDirty
	{
		DIRTY_NONE = 0,
		DIRTY_EULER_ROTATION_AND_SCALE = 1,
		DIRTY_LOCAL_TRANSFORM = 2,
		DIRTY_GLOBAL_TRANSFORM = 4,
		DIRTY_GLOBAL_INTERPOLATED_TRANSFORM = 8,
	};

	struct ClientPhysicsInterpolationData
	{
		Transform3D global_xform_curr;
		Transform3D global_xform_prev;
		uint64_t current_physics_tick = 0;
		uint64_t timeout_physics_tick = 0;
	};

	mutable SelfList<Node> xform_change;
	SelfList<Node3D> _client_physics_interpolation_node_3d_list;

	// This Data struct is to avoid namespace pollution in derived classes.

	struct Data
	{
		// Interpolated global transform - correct on the frame only.
		// Only used with FTI.
		Transform3D global_transform_interpolated;

		// Current xforms are either
		// * Used for everything (when not using FTI)
		// * Correct on the physics tick (when using FTI)
		mutable Transform3D global_transform;
		mutable Transform3D local_transform;

		// Only used with FTI.
		Transform3D local_transform_prev;

		mutable EulerOrder euler_rotation_order = EulerOrder::YXZ;
		mutable Vector3 euler_rotation;
		mutable Vector3 scale = Vector3(1, 1, 1);
		mutable RotationEditMode rotation_edit_mode = ROTATION_EDIT_MODE_EULER;

		mutable MTNumeric<uint32_t> dirty;

		Viewport* viewport = nullptr;

		bool top_level : 1;
		bool inside_world : 1;

		// This is cached, and only currently kept up to date in visual instances.
		// This is set if a visual instance is (a) in the tree AND (b) visible via
		// is_visible_in_tree() call.
		bool vi_visible : 1;

		bool ignore_notification : 1;
		bool notify_local_transform : 1;
		bool notify_transform : 1;

		bool visible : 1;
		bool disable_scale : 1;

		// Scene tree interpolation.
		bool fti_on_frame_xform_list : 1;
		bool fti_on_frame_property_list : 1;
		bool fti_on_tick_xform_list : 1;
		bool fti_on_tick_property_list : 1;
		bool fti_global_xform_interp_set : 1;
		bool fti_frame_xform_force_update : 1;
		bool fti_is_identity_xform : 1;
		bool fti_processed : 1;

		RID visibility_parent;

		Node3D* parent = nullptr;

		// An unordered vector of `Spatial` children only.
		// This is a subset of the `Node::children`, purely
		// an optimization for faster traversal.
		LocalVector<Node3D*> node3d_children;
		uint32_t index_in_parent = UINT32_MAX;

		ClientPhysicsInterpolationData* client_physics_interpolation_data = nullptr;

#ifdef TOOLS_ENABLED
		Vector<Ref<Node3DGizmo>> gizmos;
		bool gizmos_requested : 1;
		bool gizmos_disabled : 1;
		bool gizmos_dirty : 1;
		bool transform_gizmo_visible : 1;
#endif

	} data;

	NodePath visibility_parent_path;

	_FORCE_INLINE_ uint32_t _read_dirty_mask() const
	{
		return is_group_processing() ? data.dirty.mt.get() : data.dirty.st;
	}

	_FORCE_INLINE_ bool _test_dirty_bits(uint32_t p_bits) const
	{
		return (is_group_processing() ? data.dirty.mt.get() : data.dirty.st) & p_bits;
	}

	void _replace_dirty_mask(uint32_t p_mask) const;
	void _set_dirty_bits(uint32_t p_bits) const;
	void _clear_dirty_bits(uint32_t p_bits) const;

	void _update_gizmos();
	void _notify_dirty();
	void _propagate_transform_changed(Node3D* p_origin);

	void _propagate_visibility_changed();

	void _propagate_visibility_parent();
	void _update_visibility_parent(bool p_update_root);
	void _propagate_transform_changed_deferred();

protected:
	_FORCE_INLINE_ void set_ignore_transform_notification(bool p_ignore)
	{
		data.ignore_notification = p_ignore;
	}

	_FORCE_INLINE_ void _update_local_transform() const;
	_FORCE_INLINE_ void _update_rotation_and_scale() const;

	void _set_vi_visible(bool p_visible) { data.vi_visible = p_visible; }

	bool _is_vi_visible() const { return data.vi_visible; }

	Transform3D _get_global_transform_interpolated(real_t p_interpolation_fraction);

	const Transform3D& _get_cached_global_transform_interpolated() const
	{
		return data.global_transform_interpolated;
	}

	void _disable_client_physics_interpolation();

	// Calling this announces to the FTI system that a node has been moved,
	// or requires an update in terms of interpolation
	// (e.g. changing Camera zoom even if position hasn't changed).
	void fti_notify_node_changed(bool p_transform_changed = true);

	// Opportunity after FTI to update the servers
	// with global_transform_interpolated,
	// and any custom interpolated data in derived classes.
	// Make sure to call the parent class fti_update_servers(),
	// so all data is updated to the servers.
	virtual void fti_update_servers_xform() {}

	virtual void fti_update_servers_property() {}

	// Pump the FTI data, also gives a chance for inherited classes
	// to pump custom data, but they *must* call the base class here too.
	// This is the opportunity for classes to move current values for
	// transforms and properties to stored previous values,
	// and this should take place both on ticks, and during resets.
	virtual void fti_pump_xform();

	virtual void fti_pump_property() {}

	void _notification(int p_what);
	static void _bind_methods();

	bool _property_can_revert(const String& p_name) const;

public:
	enum
	{
		NOTIFICATION_TRANSFORM_CHANGED = 2000, // Keep in sync with SceneTree.
		NOTIFICATION_ENTER_WORLD = 41,
		NOTIFICATION_EXIT_WORLD = 42,
		NOTIFICATION_VISIBILITY_CHANGED = 43,
		NOTIFICATION_LOCAL_TRANSFORM_CHANGED = 44,
	};

	Node3D* get_parent_node_3d() const;

	Ref<World3D> get_world_3d() const;

	void set_position(const Vector3& p_position);

	void set_rotation_edit_mode(RotationEditMode p_mode);
	RotationEditMode get_rotation_edit_mode() const;

	void set_rotation_order(EulerOrder p_order);
	void set_rotation(const Vector3& p_euler_rad);
	void set_rotation_degrees(const Vector3& p_euler_degrees);
	void set_scale(const Vector3& p_scale);

	void set_global_position(const Vector3& p_position);
	void set_global_basis(const Basis& p_basis);
	void set_global_rotation(const Vector3& p_euler_rad);
	void set_global_rotation_degrees(const Vector3& p_euler_degrees);

	Vector3 get_position() const;

	EulerOrder get_rotation_order() const;
	Vector3 get_rotation() const;
	Vector3 get_rotation_degrees() const;
	Vector3 get_scale() const;

	Vector3 get_global_position() const;
	Basis get_global_basis() const;
	Vector3 get_global_rotation() const;
	Vector3 get_global_rotation_degrees() const;

	void set_transform(const Transform3D& p_transform);
	void set_basis(const Basis& p_basis);
	void set_quaternion(const Quaternion& p_quaternion);
	void set_global_transform(const Transform3D& p_transform);

	Transform3D get_transform() const;
	Basis get_basis() const;
	Quaternion get_quaternion() const;
	Transform3D get_global_transform() const;

	Transform3D get_global_transform_interpolated();
	bool update_client_physics_interpolation_data();

#ifdef TOOLS_ENABLED
	virtual Transform3D get_global_gizmo_transform() const;
	virtual Transform3D get_local_gizmo_transform() const;

	virtual void set_transform_gizmo_visible(bool p_enabled)
	{
		data.transform_gizmo_visible = p_enabled;
	}

	virtual bool is_transform_gizmo_visible() const { return data.transform_gizmo_visible; }
#endif
	void reparent(Node* p_parent, bool p_keep_global_transform);
	void set_disable_gizmos(bool p_enabled);
	void update_gizmos();
	void set_subgizmo_selection(
		Ref<Node3DGizmo> p_gizmo, int p_id, Transform3D p_transform = Transform3D());
	void clear_subgizmo_selection();
	Vector<Ref<Node3DGizmo>> get_gizmos() const;
	void add_gizmo(Ref<Node3DGizmo> p_gizmo);
	void remove_gizmo(Ref<Node3DGizmo> p_gizmo);
	void clear_gizmos();

	void set_as_top_level(bool p_enabled);
	void set_as_top_level_keep_local(bool p_enabled);
	bool is_set_as_top_level() const;

	void set_disable_scale(bool p_enabled);
	bool is_scale_disabled() const;

	_FORCE_INLINE_ bool is_inside_world() const { return data.inside_world; }

	Transform3D get_relative_transform(const Node* p_parent) const;

	void rotate(const Vector3& p_axis, real_t p_angle);
	void rotate_x(real_t p_angle);
	void rotate_y(real_t p_angle);
	void rotate_z(real_t p_angle);
	void translate(const Vector3& p_offset);
	void scale(const Vector3& p_ratio);

	void rotate_object_local(const Vector3& p_axis, real_t p_angle);
	void scale_object_local(const Vector3& p_scale);
	void translate_object_local(const Vector3& p_offset);

	void global_rotate(const Vector3& p_axis, real_t p_angle);
	void global_scale(const Vector3& p_scale);
	void global_translate(const Vector3& p_offset);

	void look_at(
		const Vector3& p_target, const Vector3& p_up = Vector3::UP, bool p_use_model_front = false);
	void look_at_from_position(const Vector3& p_pos, const Vector3& p_target,
		const Vector3& p_up = Vector3::UP, bool p_use_model_front = false);

	Vector3 to_local(Vector3 p_global) const;
	Vector3 to_global(Vector3 p_local) const;

	void set_notify_transform(bool p_enabled);
	bool is_transform_notification_enabled() const;

	void set_notify_local_transform(bool p_enabled);
	bool is_local_transform_notification_enabled() const;

	void orthonormalize();
	void set_identity();

	void set_visible(bool p_visible);
	void show();
	void hide();
	bool is_visible() const;
	bool is_visible_in_tree() const;

	void force_update_transform();

	void set_visibility_parent(const NodePath& p_path);
	NodePath get_visibility_parent() const;

	virtual bool has_render_instance() const { return false; }
	virtual CollisionState3D* get_collision_state_3d() { return nullptr; }
	virtual const CollisionState3D* get_collision_state_3d() const { return nullptr; }

	Node3D() = default;
	~Node3D();
};

// --- Folded from scene/3d/navigation/navigation_3d.h ---
#include "scene/main/node.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"

class ArrayMesh;
class NavigationMeshSourceGeometryData3D;
class StandardMaterial3D;

class NavigationAgent3D final : public Node
{
	Node3D* agent_parent = nullptr;

	RID agent;
	RID map_override;

	bool avoidance_enabled = false;
	bool use_3d_avoidance = false;
	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;
	uint32_t navigation_layers = 1;
	NavigationPathQueryParameters3D::PathfindingAlgorithm pathfinding_algorithm =
		NavigationPathQueryParameters3D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
	NavigationPathQueryParameters3D::PathPostProcessing path_postprocessing =
		NavigationPathQueryParameters3D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
	uint32_t path_metadata_flags =
		NavigationPathQueryParameters3D::PathMetadataFlags::PATH_METADATA_INCLUDE_ALL;

	real_t path_desired_distance = 1.0;
	real_t target_desired_distance = 1.0;
	real_t height = NavigationDefaults3D::AVOIDANCE_AGENT_HEIGHT;
	real_t radius = NavigationDefaults3D::AVOIDANCE_AGENT_RADIUS;
	real_t path_height_offset = 0.0;
	real_t neighbor_distance = NavigationDefaults3D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	int max_neighbors = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t time_horizon_agents = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	real_t max_speed = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t path_max_distance = 5.0;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;
	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults3D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

	Vector3 target_position;

	Ref<NavigationPathQueryParameters3D> navigation_query;
	Ref<NavigationPathQueryResult3D> navigation_result;
	int navigation_path_index = 0;

	Vector3 safe_velocity;

	Vector3 velocity;
	bool velocity_submitted = false;

	Vector3 velocity_forced;
	bool velocity_forced_submitted = false;

	bool keep_y_velocity = true;
	float stored_y_velocity = 0.0;

	bool target_position_submitted = false;
	bool target_reached = false;
	bool navigation_finished = true;
	bool last_waypoint_reached = false;

	bool debug_enabled = false;
	float debug_path_custom_point_size = 4.0;
	bool debug_use_custom = false;
	Color debug_path_custom_color = Color(1.0, 1.0, 1.0, 1.0);
#ifdef DEBUG_ENABLED
	bool debug_path_dirty = true;
	RID debug_path_instance;
	Ref<ArrayMesh> debug_path_mesh;
	Ref<StandardMaterial3D> debug_agent_path_line_custom_material;
	Ref<StandardMaterial3D> debug_agent_path_point_custom_material;
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationAgent3D();
	~NavigationAgent3D();

	RID get_rid() const { return agent; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_agent_parent(Node* p_agent_parent);

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_pathfinding_algorithm(
		const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm);

	NavigationPathQueryParameters3D::PathfindingAlgorithm get_pathfinding_algorithm() const
	{
		return pathfinding_algorithm;
	}

	void set_path_postprocessing(
		const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing);

	NavigationPathQueryParameters3D::PathPostProcessing get_path_postprocessing() const
	{
		return path_postprocessing;
	}

	void set_path_metadata_flags(uint32_t p_flags);

	uint32_t get_path_metadata_flags() const { return path_metadata_flags; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_path_desired_distance(real_t p_dd);

	real_t get_path_desired_distance() const { return path_desired_distance; }

	void set_target_desired_distance(real_t p_dd);

	real_t get_target_desired_distance() const { return target_desired_distance; }

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_height(real_t p_height);

	real_t get_height() const { return height; }

	void set_path_height_offset(real_t p_path_height_offset);

	real_t get_path_height_offset() const { return path_height_offset; }

	void set_use_3d_avoidance(bool p_use_3d_avoidance);

	bool get_use_3d_avoidance() const { return use_3d_avoidance; }

	void set_keep_y_velocity(bool p_enabled);
	bool get_keep_y_velocity() const;

	void set_neighbor_distance(real_t p_distance);

	real_t get_neighbor_distance() const { return neighbor_distance; }

	void set_max_neighbors(int p_count);

	int get_max_neighbors() const { return max_neighbors; }

	void set_time_horizon_agents(real_t p_time_horizon);

	real_t get_time_horizon_agents() const { return time_horizon_agents; }

	void set_time_horizon_obstacles(real_t p_time_horizon);

	real_t get_time_horizon_obstacles() const { return time_horizon_obstacles; }

	void set_max_speed(real_t p_max_speed);

	real_t get_max_speed() const { return max_speed; }

	void set_path_max_distance(real_t p_pmd);
	real_t get_path_max_distance();

	void set_target_position(Vector3 p_position);
	Vector3 get_target_position() const;

	void set_simplify_path(bool p_enabled);
	bool get_simplify_path() const;

	void set_simplify_epsilon(real_t p_epsilon);
	real_t get_simplify_epsilon() const;

	void set_path_return_max_length(float p_length);
	float get_path_return_max_length() const;

	void set_path_return_max_radius(float p_radius);
	float get_path_return_max_radius() const;

	void set_path_search_max_polygons(int p_max_polygons);
	int get_path_search_max_polygons() const;

	void set_path_search_max_distance(float p_distance);
	float get_path_search_max_distance() const;

	float get_path_length() const;

	Vector3 get_next_path_position();

	Ref<NavigationPathQueryResult3D> get_current_navigation_result() const
	{
		return navigation_result;
	}

	const Vector<Vector3>& get_current_navigation_path() const
	{
		return navigation_result->get_path();
	}

	int get_current_navigation_path_index() const { return navigation_path_index; }

	real_t distance_to_target() const;
	bool is_target_reached() const;
	bool is_target_reachable();
	bool is_navigation_finished();
	Vector3 get_final_position();

	void set_velocity(const Vector3 p_velocity);

	Vector3 get_velocity() { return velocity; }

	void set_velocity_forced(const Vector3 p_velocity);

	void _avoidance_done(Vector3 p_new_velocity);

	PackedStringArray get_configuration_warnings() const override;

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_avoidance_mask_value(int p_mask_number, bool p_value);
	bool get_avoidance_mask_value(int p_mask_number) const;

	void set_avoidance_priority(real_t p_priority);
	real_t get_avoidance_priority() const;

	void set_debug_enabled(bool p_enabled);
	bool get_debug_enabled() const;

	void set_debug_use_custom(bool p_enabled);
	bool get_debug_use_custom() const;

	void set_debug_path_custom_color(Color p_color);
	Color get_debug_path_custom_color() const;

	void set_debug_path_custom_point_size(float p_point_size);
	float get_debug_path_custom_point_size() const;

private:
	bool _is_target_reachable() const;
	Vector3 _get_final_position() const;

	void _update_navigation();
	void _advance_waypoints(const Vector3& p_origin);
	void _request_repath();

	bool _is_last_waypoint() const;
	void _move_to_next_waypoint();
	bool _is_within_waypoint_distance(const Vector3& p_origin) const;
	bool _is_within_target_distance(const Vector3& p_origin) const;

	void _trigger_waypoint_reached();
	void _transition_to_navigation_finished();
	void _transition_to_target_reached();

#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_debug_path();
#endif // DEBUG_ENABLED
};

class NavigationLink3D final : public Node3D
{
	bool enabled = true;
	RID link;
	RID map_override;
	bool bidirectional = true;
	uint32_t navigation_layers = 1;
	Vector3 end_position;
	Vector3 start_position;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

#ifdef DEBUG_ENABLED
	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

	void _update_debug_mesh();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationLink3D();
	~NavigationLink3D();

	RID get_rid() const;

	bool is_enabled() const { return enabled; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	bool is_bidirectional() const { return bidirectional; }

	void set_navigation_layers(uint32_t p_navigation_layers);

	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	Vector3 get_start_position() const { return start_position; }

	Vector3 get_end_position() const { return end_position; }

	Vector3 get_global_start_position() const;

	Vector3 get_global_end_position() const;

	void set_enter_cost(real_t p_enter_cost);

	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);

	real_t get_travel_cost() const { return travel_cost; }

	PackedStringArray get_configuration_warnings() const override;

private:
	void _link_enter_navigation_map();
	void _link_exit_navigation_map();
	void _link_update_transform();
};

class NavigationObstacle3D final : public Node3D
{
	RID obstacle;
	RID map_before_pause;
	RID map_override;
	RID map_current;

	real_t height = 1.0;
	real_t radius = 0.0;

	Vector<Vector3> vertices;
	bool vertices_are_clockwise = true;
	bool vertices_are_valid = true;

	bool avoidance_enabled = true;
	uint32_t avoidance_layers = 1;

	bool use_3d_avoidance = false;

	Vector3 velocity;
	Vector3 previous_velocity;
	bool velocity_submitted = false;

	bool affect_navigation_mesh = false;
	bool carve_navigation_mesh = false;

#ifdef DEBUG_ENABLED
	RID fake_agent_radius_debug_instance_rid;
	RID fake_agent_radius_debug_mesh_rid;

	RID static_obstacle_debug_instance_rid;
	RID static_obstacle_debug_mesh_rid;

private:
	void _update_debug();
	void _update_fake_agent_radius_debug();
	void _update_static_obstacle_debug();
	void _clear_debug();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationObstacle3D();
	~NavigationObstacle3D();

	RID get_rid() const { return obstacle; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_height(real_t p_height);

	real_t get_height() const { return height; }

	void set_vertices(const Vector<Vector3>& p_vertices);

	const Vector<Vector3>& get_vertices() const { return vertices; }

	bool are_vertices_clockwise() const { return vertices_are_clockwise; }

	bool are_vertices_valid() const { return vertices_are_valid; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_use_3d_avoidance(bool p_use_3d_avoidance);

	bool get_use_3d_avoidance() const { return use_3d_avoidance; }

	void set_velocity(const Vector3 p_velocity);

	Vector3 get_velocity() const { return velocity; }

	void _avoidance_done(Vector3 p_new_velocity); // Dummy

	void set_affect_navigation_mesh(bool p_enabled);
	bool get_affect_navigation_mesh() const;

	void set_carve_navigation_mesh(bool p_enabled);
	bool get_carve_navigation_mesh() const;

	PackedStringArray get_configuration_warnings() const override;

private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);

private:
	void _update_map(RID p_map);
	void _update_position(const Vector3 p_position);
	void _update_transform();
	void _update_use_3d_avoidance(bool p_use_3d_avoidance);
};

class NavigationRegion3D final : public Node3D
{
	bool enabled = true;
	bool use_edge_connections = true;

	RID region;
	RID map_override;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	Ref<NavigationMesh> navigation_mesh;

	void _navigation_mesh_changed();

	AABB bounds;

#ifdef DEBUG_ENABLED
	RID debug_instance;
	RID debug_edge_connections_instance;
	Ref<ArrayMesh> debug_mesh;
	Ref<ArrayMesh> debug_edge_connections_mesh;

private:
	void _update_debug_mesh();
	void _update_debug_edge_connections_mesh();
	void _navigation_map_changed(RID p_map);
	void _navigation_debug_changed();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	RID get_rid() const;

	bool is_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	RID get_region_rid() const;

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const;

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const;

	void set_navigation_mesh(const Ref<NavigationMesh>& p_navigation_mesh);
	Ref<NavigationMesh> get_navigation_mesh() const;

	void bake_navigation_mesh(bool p_on_thread);
	void _bake_finished();
	bool is_baking() const;

	PackedStringArray get_configuration_warnings() const override;

	AABB get_bounds() const { return bounds; }

	NavigationRegion3D();
	~NavigationRegion3D();

private:
	void _update_bounds();
	void _region_enter_navigation_map();
	void _region_exit_navigation_map();
	void _region_update_transform();
};

// --- Folded from scene/3d/path_3d.h ---
#include "scene/resources/curve.h"
#include "scene/resources/material.h"

class ArrayMesh;

class Path3D : public Node3D
{
private:
	Ref<Curve3D> curve;
	RID debug_instance;
	Color debug_custom_color;
	Ref<ArrayMesh> debug_mesh;
	Ref<Material> debug_material;

	void _update_debug_mesh();
	void _update_debug_path_material();
	void _curve_changed();

protected:
	void _notification(int p_what);

public:
	void set_curve(const Ref<Curve3D>& p_curve);
	Ref<Curve3D> get_curve() const;

	const Color& get_debug_custom_color() const;
	void set_debug_custom_color(const Color& p_color);

	Ref<StandardMaterial3D> get_debug_material();

	Path3D();
	~Path3D();
};

class PathFollow3D : public Node3D
{
public:
	enum RotationMode
	{
		ROTATION_NONE,
		ROTATION_Y,
		ROTATION_XY,
		ROTATION_XYZ,
		ROTATION_ORIENTED
	};

private:
	Path3D* path = nullptr;
	real_t progress = 0.0;
	real_t h_offset = 0.0;
	real_t v_offset = 0.0;
	bool cubic = true;
	bool loop = true;
	bool tilt_enabled = true;
	bool use_model_front = false;
	RotationMode rotation_mode = ROTATION_XYZ;

protected:
	void _notification(int p_what);


public:
	void set_progress(real_t p_progress);
	real_t get_progress() const;

	void set_h_offset(real_t p_h_offset);
	real_t get_h_offset() const;

	void set_v_offset(real_t p_v_offset);
	real_t get_v_offset() const;

	void set_progress_ratio(real_t p_ratio);
	real_t get_progress_ratio() const;

	void set_loop(bool p_loop);
	bool has_loop() const;

	void set_tilt_enabled(bool p_enabled);
	bool is_tilt_enabled() const;

	RotationMode get_rotation_mode() const;

	void set_use_model_front(bool p_use_model_front);
	bool is_using_model_front() const;

	void set_cubic_interpolation_enabled(bool p_enabled);
	bool is_cubic_interpolation_enabled() const;

	PackedStringArray get_configuration_warnings() const override;

	void update_transform();

	static Transform3D correct_posture(
		Transform3D p_transform, PathFollow3D::RotationMode p_rotation_mode);
};

// --- Folded from scene/3d/xr/xr_nodes.h ---
#include "servers/xr/xr_tracker.h"

/*
	XRNode3D is a consolidated helper node that binds to an XRTracker
	(supporting generic tracker, controller, and anchor modes).

	It must be a child node of an XROrigin3D node.
*/

class XRNode3D final : public Node3D
{
public:
	enum Mode
	{
		MODE_TRACKER,
		MODE_CONTROLLER,
		MODE_ANCHOR,
	};

private:
	Mode mode = MODE_TRACKER;
	String tracker_name;
	String pose_name = SceneString(default_);
	bool has_tracking_data = false;
	bool show_when_tracked = false;
	Vector3 size;

	Ref<XRTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);

	void _pose_changed(const Ref<XRPose>& p_pose);
	void _pose_lost_tracking(const Ref<XRPose>& p_pose);
	void _set_has_tracking_data(bool p_has_tracking_data);

	void _update_visibility();

	void _button_pressed(const String& p_name);
	void _button_released(const String& p_name);
	void _input_float_changed(const String& p_name, float p_value);
	void _input_vector2_changed(const String& p_name, Vector2 p_value);
	void _profile_changed(const String& p_role);

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }

	void set_tracker(const String& p_tracker_name);
	String get_tracker() const;

	void set_pose_name(const String& p_pose);
	String get_pose_name() const;

	bool get_is_active() const;
	bool get_has_tracking_data() const;

	void set_show_when_tracked(bool p_show);
	bool get_show_when_tracked() const;

	void trigger_haptic_pulse(const String& p_action_name, double p_frequency, double p_amplitude,
		double p_duration_sec, double p_delay_sec = 0);

	Ref<XRPose> get_pose();

	// Controller methods (MODE_CONTROLLER)
	bool is_button_pressed(const String& p_name) const;
	XRTracker::InputValue get_input(const String& p_name) const;
	float get_float(const String& p_name) const;
	Vector2 get_vector2(const String& p_name) const;
	XRTracker::TrackerHand get_tracker_hand() const;

	// Anchor methods (MODE_ANCHOR)
	Vector3 get_size() const;
	Plane get_plane() const;

	PackedStringArray get_configuration_warnings() const override;

	XRNode3D(Mode p_mode = MODE_TRACKER);
	~XRNode3D();
};

/*
	XROrigin3D is a special spatial node that acts as our origin point mapping our real world center
	of our tracking volume into our virtual world.
*/

class XROrigin3D final : public Node3D
{
private:
	bool current = false;
	static Vector<XROrigin3D*> origin_nodes; // all origin nodes in tree

	void _set_current(bool p_enabled, bool p_update_others);

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;

public:
	PackedStringArray get_configuration_warnings() const override;

	real_t get_world_scale() const;
	void set_world_scale(real_t p_world_scale);

	void set_current(bool p_enabled);
	bool is_current() const;
};
