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
#include "scene/resources/mesh.h"
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
#include "servers/xr/xr_server.h"

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

// --- Folded from scene/3d/visual_instance_3d.h ---

#include "scene/resources/environment.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_server_enums.h"

class TriangleMesh;

struct RenderInstance3D
{
	enum GetFacesFlags
	{
		FACES_SOLID = 1,
		FACES_ENCLOSING = 2,
		FACES_DYNAMIC = 4,
	};

	enum ShadowCastingSetting
	{
		SHADOW_CASTING_SETTING_OFF = RSE::SHADOW_CASTING_SETTING_OFF,
		SHADOW_CASTING_SETTING_ON = RSE::SHADOW_CASTING_SETTING_ON,
		SHADOW_CASTING_SETTING_DOUBLE_SIDED = RSE::SHADOW_CASTING_SETTING_DOUBLE_SIDED,
		SHADOW_CASTING_SETTING_SHADOWS_ONLY = RSE::SHADOW_CASTING_SETTING_SHADOWS_ONLY,
	};

	enum GIMode
	{
		GI_MODE_DISABLED,
		GI_MODE_STATIC,
		GI_MODE_DYNAMIC,
	};

	enum LightmapScale
	{
		LIGHTMAP_SCALE_1X,
		LIGHTMAP_SCALE_2X,
		LIGHTMAP_SCALE_4X,
		LIGHTMAP_SCALE_8X,
		LIGHTMAP_SCALE_MAX,
	};

	enum VisibilityRangeFadeMode
	{
		VISIBILITY_RANGE_FADE_DISABLED = RSE::VISIBILITY_RANGE_FADE_DISABLED,
		VISIBILITY_RANGE_FADE_SELF = RSE::VISIBILITY_RANGE_FADE_SELF,
		VISIBILITY_RANGE_FADE_DEPENDENCIES = RSE::VISIBILITY_RANGE_FADE_DEPENDENCIES,
	};

	RID base;
	RID instance;
	uint32_t layers = 1;
	float sorting_offset = 0.0f;
	bool sorting_use_aabb_center = true;

	ShadowCastingSetting shadow_casting_setting = SHADOW_CASTING_SETTING_ON;
	Ref<Material> material_override;
	Ref<Material> material_overlay;

	float visibility_range_begin = 0.0f;
	float visibility_range_end = 0.0f;
	float visibility_range_begin_margin = 0.0f;
	float visibility_range_end_margin = 0.0f;
	VisibilityRangeFadeMode visibility_range_fade_mode = VISIBILITY_RANGE_FADE_DISABLED;

	float transparency = 0.0f;
	float lod_bias = 1.0f;
	mutable HashMap<String, String> instance_shader_parameter_property_remap;

	float extra_cull_margin = 0.0f;
	AABB custom_aabb;
	float lightmap_texel_scale = 1.0f;
	GIMode gi_mode = GI_MODE_STATIC;
	bool ignore_occlusion_culling = false;

	void update_visibility(Node3D* p_owner);
	void set_instance_use_identity_transform(Node3D* p_owner, bool p_enable);
	void fti_update_servers_xform(Node3D* p_owner);
	void notification(Node3D* p_owner, int p_what);

	RID get_instance() const;

	void set_base(const RID& p_base);
	RID get_base() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

	void set_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_layer_mask_value(int p_layer_number) const;

	void set_sorting_offset(float p_offset);
	float get_sorting_offset() const;

	void set_sorting_use_aabb_center(bool p_enabled);
	bool is_sorting_use_aabb_center() const;

	void set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting);
	ShadowCastingSetting get_cast_shadows_setting() const;

	float get_transparency() const;
	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;
	VisibilityRangeFadeMode get_visibility_range_fade_mode() const;

	void set_material_override(const Ref<Material>& p_material);
	Ref<Material> get_material_override() const;

	void set_material_overlay(const Ref<Material>& p_material);
	Ref<Material> get_material_overlay() const;

	void set_extra_cull_margin(float p_margin);
	float get_extra_cull_margin() const;

	void set_lod_bias(float p_bias);
	float get_lod_bias() const;

	void set_gi_mode(GIMode p_mode);
	GIMode get_gi_mode() const;

	void set_lightmap_texel_scale(float p_scale);
	float get_lightmap_texel_scale() const;

	AABB get_custom_aabb() const;

	void set_ignore_occlusion_culling(bool p_enabled);
	bool is_ignoring_occlusion_culling() const;

	const String* _instance_uniform_get_remap(const String& p_name) const;
	void append_geometry_warnings(PackedStringArray& r_warnings) const;

	RenderInstance3D() = default;
	~RenderInstance3D();
};

// --- Folded from scene/3d/decal.h ---


class Decal : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DecalTexture
	{
		TEXTURE_ALBEDO,
		TEXTURE_NORMAL,
		TEXTURE_ORM,
		TEXTURE_EMISSION,
		TEXTURE_MAX
	};

private:
	RID decal;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture2D> textures[TEXTURE_MAX];
	real_t emission_energy = 1.0;
	real_t albedo_mix = 1.0;
	Color modulate = Color(1, 1, 1, 1);
	uint32_t cull_mask = (1 << 20) - 1;
	real_t normal_fade = 0.0;
	real_t upper_fade = 0.3;
	real_t lower_fade = 0.3;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_length = 10.0;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	Vector3 get_size() const;

	void set_texture(DecalTexture p_type, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture(DecalTexture p_type) const;

	void set_emission_energy(real_t p_energy);
	real_t get_emission_energy() const;

	void set_albedo_mix(real_t p_mix);
	real_t get_albedo_mix() const;

	void set_modulate(Color p_modulate);
	Color get_modulate() const;

	void set_upper_fade(real_t p_energy);
	real_t get_upper_fade() const;

	void set_lower_fade(real_t p_fade);
	real_t get_lower_fade() const;

	void set_normal_fade(real_t p_fade);
	real_t get_normal_fade() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	uint32_t get_cull_mask() const;

	AABB get_aabb() const;

	Decal();
	~Decal();
};



// --- End scene/3d/decal.h ---

// --- Folded from scene/3d/label_3d.h ---

#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "servers/text/text_server.h"

class Label3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawFlags
	{
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

private:
	real_t pixel_size = 0.005;
	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;

	AABB aabb;

	mutable Ref<TriangleMesh> triangle_mesh;
	RID mesh;

	struct SurfaceData
	{
		PackedVector3Array mesh_vertices;
		PackedVector3Array mesh_normals;
		PackedFloat32Array mesh_tangents;
		PackedColorArray mesh_colors;
		PackedVector2Array mesh_uvs;
		PackedInt32Array indices;
		int offset = 0;
		float z_shift = 0.0;
		RID material;
	};

	struct SurfaceKey
	{
		uint64_t texture_id;
		int32_t priority;
		int32_t outline_size;

		bool operator==(const SurfaceKey& p_b) const
		{
			return (texture_id == p_b.texture_id) && (priority == p_b.priority) &&
				   (outline_size == p_b.outline_size);
		}

		SurfaceKey(uint64_t p_texture_id, int p_priority, int p_outline_size)
		{
			texture_id = p_texture_id;
			priority = p_priority;
			outline_size = p_outline_size;
		}
	};

	struct SurfaceKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const SurfaceKey& p_a)
		{
			return hash_murmur3_buffer(&p_a, sizeof(SurfaceKey));
		}
	};

	HashMap<SurfaceKey, SurfaceData, SurfaceKeyHasher> surfaces;

	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
	String text;
	String xl_text;
	bool uppercase = false;

	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	uint32_t autowrap_flags_trim =
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
						 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
						 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	float width = 500.0;

	int font_size = 32;
	Ref<Font> font_override;
	mutable Ref<Font> theme_font;
	Color modulate = Color(1, 1, 1, 1);
	Point2 lbl_offset;
	int outline_render_priority = -1;
	int render_priority = 0;

	int outline_size = 12;
	Color outline_modulate = Color(0, 0, 0, 1);

	float line_spacing = 0.f;

	String language;
	TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

	RID text_rid;
	Vector<RID> lines_rid;

	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;

	bool pending_update = false;

	bool dirty_lines = true;
	bool dirty_font = true;
	bool dirty_text = true;

	void _generate_glyph_surfaces(const Glyph& p_glyph, Vector2& r_offset, const Color& p_modulate,
		int p_priority = 0, int p_outline_size = 0);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	void _font_changed();
	void _queue_update();

	void _shape();

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_outline_render_priority(int p_priority);
	int get_outline_render_priority() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_line_spacing(float p_size);
	float get_line_spacing() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_outline_modulate(const Color& p_color);
	Color get_outline_modulate() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_width(float p_width);
	float get_width() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	Label3D();
	~Label3D();
};



// --- End scene/3d/label_3d.h ---

// --- Folded from scene/3d/occluder_instance_3d.h ---


class ArrayMesh;

class Occluder3D : public Resource
{
public:
	enum OccluderType
	{
		OCCLUDER_ARRAY,
		OCCLUDER_QUAD,
		OCCLUDER_BOX,
		OCCLUDER_SPHERE,
		OCCLUDER_POLYGON,
	};

private:
	static constexpr int RINGS = 7;
	static constexpr int RADIAL_SEGMENTS = 7;

	OccluderType occluder_type = OCCLUDER_ARRAY;

	RID occluder;
	PackedVector3Array vertices;
	PackedInt32Array indices;
	AABB aabb;

	PackedVector3Array array_vertices;
	PackedInt32Array array_indices;
	Size2 quad_size = Vector2(1.0f, 1.0f);
	Vector3 box_size = Vector3(1.0f, 1.0f, 1.0f);
	float radius = 1.0f;
	Vector<Vector2> polygon;

	mutable Ref<ArrayMesh> debug_mesh;
	mutable Vector<Vector3> debug_lines;

	bool _has_editable_3d_polygon_no_depth() const;

protected:
	void _update();
	void _update_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

public:
	void set_occluder_type(OccluderType p_type);
	OccluderType get_occluder_type() const;

	void set_arrays(PackedVector3Array p_vertices, PackedInt32Array p_indices);
	void set_vertices(PackedVector3Array p_vertices);
	void set_indices(PackedInt32Array p_indices);

	PackedVector3Array get_vertices() const;
	PackedInt32Array get_indices() const;

	void set_quad_size(const Size2& p_size);
	Size2 get_quad_size() const;

	void set_box_size(const Vector3& p_size);
	Vector3 get_box_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	Vector<Vector3> get_debug_lines() const;
	Ref<ArrayMesh> get_debug_mesh() const;
	AABB get_aabb() const;

	virtual RID get_rid() const override;
	Occluder3D(OccluderType p_type = OCCLUDER_ARRAY);
	virtual ~Occluder3D();
};

class OccluderInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<Occluder3D> occluder;
	uint32_t bake_mask = 0xFFFFFFFF;
	float bake_simplification_dist = 0.1f;

	void _bake_node(Node* p_node, PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	bool _is_editable_3d_polygon() const;
	Ref<Resource> _get_editable_3d_polygon_resource() const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	enum BakeError
	{
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_CANT_SAVE,
	};

	void set_occluder(const Ref<Occluder3D>& p_occluder);
	Ref<Occluder3D> get_occluder() const;

	AABB get_aabb() const;

	uint32_t get_bake_mask() const;

	void set_bake_simplification_distance(float p_dist);
	float get_bake_simplification_distance() const;

	bool get_bake_mask_value(int p_layer_number) const;

	BakeError bake_scene(Node* p_from_node, String p_occluder_path = "");
	static void bake_single_node(const Node3D* p_node, float p_simplification_distance,
		PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	OccluderInstance3D();
	~OccluderInstance3D();
};



// --- End scene/3d/occluder_instance_3d.h ---

// --- Folded from scene/3d/sprite_3d.h ---

#include "scene/resources/material.h"
// --- Folded from scene/resources/sprite_frames.h ---
#include "scene/resources/texture.h"

static const float SPRITE_FRAME_MINIMUM_DURATION = 0.01;

class SpriteFrames : public Resource
{
public:
	enum LoopMode
	{
		LOOP_NONE,
		LOOP_LINEAR,
		LOOP_PINGPONG,
	};

private:
	struct Frame
	{
		Ref<Texture2D> texture;
		float duration = 1.0;
	};

	struct Anim
	{
		double speed = 5.0;
		LoopMode loop = LoopMode::LOOP_LINEAR;
		Vector<Frame> frames;
	};

	HashMap<String, Anim> animations;

protected:

public:
	void add_animation(const String& p_anim);
	bool has_animation(const String& p_anim) const;
	void duplicate_animation(const String& p_from, const String& p_to);
	void remove_animation(const String& p_anim);
	void rename_animation(const String& p_prev, const String& p_next);

	void get_animation_list(List<String>* r_animations) const;
	Vector<String> get_animation_names() const;

	void set_animation_speed(const String& p_anim, double p_fps);
	double get_animation_speed(const String& p_anim) const;

#ifndef DISABLE_DEPRECATED
	void set_animation_loop(const String& p_anim, bool p_loop);
	bool get_animation_loop(const String& p_anim) const;
#endif

	void set_animation_loop_mode(const String& p_anim, LoopMode p_loop_mode);
	LoopMode get_animation_loop_mode(const String& p_anim) const;

	void add_frame(const String& p_anim, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0, int p_at_pos = -1);
	void set_frame(const String& p_anim, int p_idx, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0);
	void remove_frame(const String& p_anim, int p_idx);

	int get_frame_count(const String& p_anim) const;

	_FORCE_INLINE_ Ref<Texture2D> get_frame_texture(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(
			!E, Ref<Texture2D>(), "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, Ref<Texture2D>());
		if (p_idx >= E->value.frames.size()) {
			return Ref<Texture2D>();
		}

		return E->value.frames[p_idx].texture;
	}

	_FORCE_INLINE_ float get_frame_duration(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(!E, 1.0, "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, 1.0);
		if (p_idx >= E->value.frames.size()) {
			return 1.0;
		}

		return E->value.frames[p_idx].duration;
	}

	void clear(const String& p_anim);
	void clear_all();

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	SpriteFrames();
};



// --- End scene/resources/sprite_frames.h ---

class Sprite3D : public Node3D
{
public:
	enum DrawFlags
	{
		FLAG_TRANSPARENT,
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

	RenderInstance3D render_instance;

private:
	mutable Ref<TriangleMesh> triangle_mesh;

	bool color_dirty = true;
	Color color_accum;

	Sprite3D* parent_sprite = nullptr;
	List<Sprite3D*> children;
	List<Sprite3D*>::Element* pI = nullptr;

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;

	Color modulate = Color(1, 1, 1, 1);
	int render_priority = 0;

	Vector3::Axis axis = Vector3::AXIS_Z;
	real_t pixel_size = 0.01;
	AABB aabb;

	RID mesh;
	RID material;

	RID last_shader;
	RID last_texture;

	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;
	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
	bool redraw_needed = false;
	bool pending_update = false;

	uint32_t mesh_surface_offsets[RSE::ARRAY_MAX];
	PackedByteArray vertex_buffer;
	PackedByteArray attribute_buffer;
	uint32_t vertex_stride = 0;
	uint32_t normal_tangent_stride = 0;
	uint32_t attrib_stride = 0;
	uint32_t skin_stride = 0;
	uint32_t mesh_surface_format = 0;

	// Single-texture sprite state
	Ref<Texture2D> texture;
	bool region = false;
	Rect2 region_rect;
	int frame = 0;
	int vframes = 1;
	int hframes = 1;

	// Optional SpriteFrames animation state (used when frames.is_valid())
	Ref<SpriteFrames> frames;
	String autoplay;
	bool playing = false;
	String animation = SceneString(default_);
	float speed_scale = 1.0;
	float custom_speed_scale = 1.0;
	real_t frame_speed_scale = 1.0;
	real_t frame_progress = 0.0;

	void _im_update();
	void _propagate_color_changed();
	Color _get_color_accum();
	void _draw();
	void draw_texture_rect(Ref<Texture2D> p_texture, Rect2 p_dst_rect, Rect2 p_src_rect);
	void _queue_redraw();

	double _get_frame_duration();
	void _calc_frame_speed_scale();
	void _stop_internal(bool p_reset);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_axis(Vector3::Axis p_axis);
	Vector3::Axis get_axis() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	// Static/sheet texture API
	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_region_enabled(bool p_region);
	bool is_region_enabled() const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2i& p_coord);
	Vector2i get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	// SpriteFrames animation API
	void set_sprite_frames(const Ref<SpriteFrames>& p_frames);
	Ref<SpriteFrames> get_sprite_frames() const;

	void play(const String& p_name = String(), float p_custom_scale = 1.0f, bool p_from_end = false);
	void play_backwards(const String& p_name = String());
	void pause();
	void stop();

	bool is_playing() const;

	void set_animation(const String& p_name);
	String get_animation() const;

	void set_autoplay(const String& p_name);
	String get_autoplay() const;

	void set_frame_progress(real_t p_progress);
	real_t get_frame_progress() const;

	void set_speed_scale(float p_speed_scale);
	float get_speed_scale() const;
	float get_playing_speed() const;

	Rect2 get_item_rect() const;
	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	virtual PackedStringArray get_configuration_warnings() const override;

#ifdef TOOLS_ENABLED
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	Sprite3D();
	~Sprite3D();
};

// --- End scene/3d/sprite_3d.h ---

// --- Folded from scene/3d/light_3d.h ---


class Light3D : public Node3D
{
public:
	enum Param
	{
		PARAM_ENERGY = RSE::LIGHT_PARAM_ENERGY,
		PARAM_INDIRECT_ENERGY = RSE::LIGHT_PARAM_INDIRECT_ENERGY,
		PARAM_VOLUMETRIC_FOG_ENERGY = RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY,
		PARAM_SPECULAR = RSE::LIGHT_PARAM_SPECULAR,
		PARAM_RANGE = RSE::LIGHT_PARAM_RANGE,
		PARAM_SIZE = RSE::LIGHT_PARAM_SIZE,
		PARAM_ATTENUATION = RSE::LIGHT_PARAM_ATTENUATION,
		PARAM_SPOT_ANGLE = RSE::LIGHT_PARAM_SPOT_ANGLE,
		PARAM_SPOT_ATTENUATION = RSE::LIGHT_PARAM_SPOT_ATTENUATION,
		PARAM_SHADOW_MAX_DISTANCE = RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE,
		PARAM_SHADOW_SPLIT_1_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
		PARAM_SHADOW_SPLIT_2_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
		PARAM_SHADOW_SPLIT_3_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
		PARAM_SHADOW_FADE_START = RSE::LIGHT_PARAM_SHADOW_FADE_START,
		PARAM_SHADOW_NORMAL_BIAS = RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS,
		PARAM_SHADOW_BIAS = RSE::LIGHT_PARAM_SHADOW_BIAS,
		PARAM_SHADOW_PANCAKE_SIZE = RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE,
		PARAM_SHADOW_OPACITY = RSE::LIGHT_PARAM_SHADOW_OPACITY,
		PARAM_SHADOW_BLUR = RSE::LIGHT_PARAM_SHADOW_BLUR,
		PARAM_TRANSMITTANCE_BIAS = RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS,
		PARAM_INTENSITY = RSE::LIGHT_PARAM_INTENSITY,
		PARAM_MAX = RSE::LIGHT_PARAM_MAX
	};

	enum BakeMode
	{
		BAKE_DISABLED,
		BAKE_STATIC,
		BAKE_DYNAMIC,
	};

	enum DirectionalShadowMode
	{
		DIRECTIONAL_SHADOW_ORTHOGONAL,
		DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
		DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS,
	};

	enum SkyMode
	{
		SKY_MODE_LIGHT_AND_SKY,
		SKY_MODE_LIGHT_ONLY,
		SKY_MODE_SKY_ONLY,
	};

	enum OmniShadowMode
	{
		OMNI_SHADOW_DUAL_PARABOLOID,
		OMNI_SHADOW_CUBE,
	};

	RenderInstance3D render_instance;

private:
	RSE::LightType type = RSE::LIGHT_DIRECTIONAL;
	RID light;

	Color color = Color(1, 1, 1);
	real_t param[PARAM_MAX] = {};
	bool shadow = false;
	bool negative = false;
	bool reverse_cull = false;
	uint32_t cull_mask = 0xFFFFFFFF;
	uint32_t shadow_caster_mask = 0xFFFFFFFF;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_shadow = 50.0;
	real_t distance_fade_length = 10.0;
	bool editor_only = false;
	BakeMode bake_mode = BAKE_DYNAMIC;
	Ref<Texture2D> projector;
	Color correlated_color = Color(1.0, 1.0, 1.0);
	float temperature = 6500.0;

	// Directional-specific state
	bool blend_splits = false;
	DirectionalShadowMode directional_shadow_mode = DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS;
	SkyMode sky_mode = SKY_MODE_LIGHT_AND_SKY;

	// Omni-specific state
	OmniShadowMode omni_shadow_mode = OMNI_SHADOW_CUBE;

	// Area-specific state
	Vector2 area_size = Vector2(1.0, 1.0);
	Ref<Texture2D> area_texture;
	bool area_normalize_energy = true;

	void _create_light_rid();
	void _free_light_rid();
	void _update_visibility();

	virtual void owner_changed_notify() override;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_light_type(RSE::LightType p_type);
	RSE::LightType get_light_type() const { return type; }

	void set_editor_only(bool p_editor_only);
	bool is_editor_only() const;

	void set_param(Param p_param, real_t p_value);
	real_t get_param(Param p_param) const;

	void set_shadow(bool p_enable);
	bool has_shadow() const;

	void set_negative(bool p_enable);
	bool is_negative() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_shadow(real_t p_distance);
	real_t get_distance_fade_shadow() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_shadow_reverse_cull_face(bool p_enable);
	bool get_shadow_reverse_cull_face() const;

	void set_shadow_caster_mask(uint32_t p_caster_mask);
	uint32_t get_shadow_caster_mask() const;

	void set_bake_mode(BakeMode p_mode);
	BakeMode get_bake_mode() const;

	void set_projector(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_projector() const;

	float get_temperature() const;
	Color get_correlated_color() const;

	// Directional light parameters
	void set_directional_shadow_mode(DirectionalShadowMode p_mode);
	DirectionalShadowMode get_directional_shadow_mode() const { return directional_shadow_mode; }

	void set_blend_splits(bool p_enable);
	bool is_blend_splits_enabled() const { return blend_splits; }

	void set_sky_mode(SkyMode p_mode);
	SkyMode get_sky_mode() const { return sky_mode; }

	// Omni light parameters
	void set_omni_shadow_mode(OmniShadowMode p_mode);
	OmniShadowMode get_omni_shadow_mode() const { return omni_shadow_mode; }

	// Area light parameters
	void set_area_size(const Vector2& p_size);
	Vector2 get_area_size() const { return area_size; }

	void set_area_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_area_texture() const { return area_texture; }

	void set_area_normalize_energy(bool p_enable);
	bool is_area_normalizing_energy() const { return area_normalize_energy; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	Light3D(RSE::LightType p_type = RSE::LIGHT_DIRECTIONAL);
	~Light3D();
};

// --- End scene/3d/light_3d.h ---

// --- Folded from scene/3d/lightmapper.h ---
#include "core/io/image.h"
#include "core/templates/hash_set.h"
#include "core/types.h"

class Image;

class LightmapDenoiser : public RefCounted
{
protected:
	static Ref<LightmapDenoiser> (*create_function)();

public:
	virtual Ref<Image> denoise_image(const Ref<Image>& p_image) = 0;
	static Ref<LightmapDenoiser> create();
};

class LightmapRaycaster : public RefCounted
{
public:
	struct EmbreeState;

private:
	EmbreeState* embree_state = nullptr;

public:
	// Compatible with embree4 rays.
	struct alignas(16) Ray
	{
		const static unsigned int INVALID_GEOMETRY_ID = ((unsigned int)-1); // from rtcore_common.h

		/*! Default construction does nothing. */
		_FORCE_INLINE_ Ray() : geomID(INVALID_GEOMETRY_ID) {}

		/*! Constructs a ray from origin, direction, and ray segment. Near
		 *  has to be smaller than far. */
		_FORCE_INLINE_ Ray(const Vector3& p_org, const Vector3& p_dir, float p_tnear = 0.0f,
			float p_tfar = Math::INF)
			: org(p_org), tnear(p_tnear), dir(p_dir), time(0.0f), tfar(p_tfar), mask(-1), u(0.0),
			  v(0.0), primID(INVALID_GEOMETRY_ID), geomID(INVALID_GEOMETRY_ID),
			  instID(INVALID_GEOMETRY_ID)
		{
		}

		/*! Tests if we hit something. */
		_FORCE_INLINE_ explicit operator bool() const { return geomID != INVALID_GEOMETRY_ID; }

	public:
		Vector3 org;		//!< Ray origin + tnear
		float tnear;		//!< Start of ray segment
		Vector3 dir;		//!< Ray direction + tfar
		float time;			//!< Time of this ray for motion blur.
		float tfar;			//!< End of ray segment
		unsigned int mask;	//!< used to mask out objects during traversal
		unsigned int id;	//!< ray ID
		unsigned int flags; //!< ray flags

		Vector3 normal;		 //!< Not normalized geometry normal
		float u;			 //!< Barycentric u coordinate of hit
		float v;			 //!< Barycentric v coordinate of hit
		unsigned int primID; //!< primitive ID
		unsigned int geomID; //!< geometry ID
		unsigned int instID; //!< instance ID
	};

	bool intersect(Ray& p_ray);
	void intersect(Vector<Ray>& r_rays);

	void add_mesh(const Vector<Vector3>& p_vertices, const Vector<Vector3>& p_normals,
		const Vector<Vector2>& p_uv2s, unsigned int p_id);
	void set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id);
	void commit();

	void set_mesh_filter(const HashSet<int>& p_mesh_ids);
	void clear_mesh_filter();

	static Ref<LightmapRaycaster> create();

	LightmapRaycaster();
	~LightmapRaycaster();
};

class Lightmapper : public RefCounted
{
public:
	struct RDState;

private:
	RDState* rd_state = nullptr;

public:
	enum GenerateProbes
	{
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum LightType
	{
		LIGHT_TYPE_DIRECTIONAL,
		LIGHT_TYPE_OMNI,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_AREA,
	};

	enum BakeError
	{
		BAKE_OK,
		BAKE_ERROR_TEXTURE_EXCEEDS_MAX_SIZE,
		BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES,
		BAKE_ERROR_ATLAS_TOO_SMALL,
		BAKE_ERROR_USER_ABORTED,
	};

	enum BakeQuality
	{
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	typedef bool (*BakeStepFunc)(
		float, const String&, void*, bool); // step index, step total, step description, userdata

	struct MeshData
	{
		// triangle data
		Vector<Vector3> points;
		Vector<Vector2> uv2;
		Vector<Vector3> normal;
		Vector<RID> material;
		Ref<Image> albedo_on_uv2;
		Ref<Image> emission_on_uv2;
	};

	void add_mesh(const MeshData& p_mesh);
	void add_directional_light(const String& p_name, bool p_static, const Vector3& p_direction,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_angular_distance,
		float p_shadow_blur);
	void add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
		float p_attenuation, float p_size, float p_shadow_blur);
	void add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation,
		float p_size, float p_shadow_blur);
	void add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, const Vector3& p_area_width,
		const Vector3& p_area_height, float p_size, float p_shadow_blur,
		const Rect2& p_texture_rect, float p_max_mipmap);
	void add_area_light_atlas(
		const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data);
	void add_probe(const Vector3& p_position);
	BakeError bake(BakeQuality p_quality, bool p_use_denoiser, float p_denoiser_strength,
		int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy, float p_bias,
		int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask, bool p_texture_for_bounces,
		GenerateProbes p_generate_probes, const Ref<Image>& p_environment_panorama,
		const Basis& p_environment_transform, BakeStepFunc p_step_function = nullptr,
		void* p_step_userdata = nullptr, float p_exposure_normalization = 1.0,
		float p_supersampling_factor = 1.0);

	int get_bake_texture_count() const;
	Ref<Image> get_bake_texture(int p_index) const;
	int get_shadowmask_texture_count() const;
	Ref<Image> get_shadowmask_texture(int p_index) const;
	int get_bake_mesh_count() const;
	Rect2 get_bake_mesh_uv_scale(int p_index) const;
	int get_bake_mesh_texture_slice(int p_index) const;

	int get_bake_probe_count() const;
	Vector3 get_bake_probe_point(int p_probe) const;
	Vector<Color> get_bake_probe_sh(int p_probe) const;

	static Ref<Lightmapper> create();

	Lightmapper();
	~Lightmapper();
};

// --- End scene/3d/lightmapper.h ---

// --- Folded from scene/3d/multimesh_instance_3d.h ---

// --- Inlined/Folded from scene/resources/multimesh.h ---
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server_enums.h"

class MultiMesh : public Resource
{
public:
	enum TransformFormat
	{
		TRANSFORM_2D = RSE::MULTIMESH_TRANSFORM_2D,
		TRANSFORM_3D = RSE::MULTIMESH_TRANSFORM_3D
	};

	enum PhysicsInterpolationQuality
	{
		INTERP_QUALITY_FAST,
		INTERP_QUALITY_HIGH,
	};

private:
	Ref<Mesh> mesh;
	RID multimesh;
	TransformFormat transform_format = TRANSFORM_2D;
	AABB custom_aabb;
	bool use_colors = false;
	bool use_custom_data = false;
	int instance_count = 0;
	int visible_instance_count = -1;
	PhysicsInterpolationQuality _physics_interpolation_quality = INTERP_QUALITY_FAST;

protected:

#ifndef DISABLE_DEPRECATED
	// Kept for compatibility from 3.x to 4.0.
	void _set_transform_array(const Vector<Vector3>& p_array);
	Vector<Vector3> _get_transform_array() const;

	void _set_transform_2d_array(const Vector<Vector2>& p_array);
	Vector<Vector2> _get_transform_2d_array() const;

	void _set_color_array(const Vector<Color>& p_array);
	Vector<Color> _get_color_array() const;

	void _set_custom_data_array(const Vector<Color>& p_array);
	Vector<Color> _get_custom_data_array() const;
#endif
	void set_buffer(const Vector<float>& p_buffer);
	Vector<float> get_buffer() const;

	void set_buffer_interpolated(
		const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);

public:
	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_use_colors(bool p_enable);
	bool is_using_colors() const;

	void set_use_custom_data(bool p_enable);
	bool is_using_custom_data() const;

	void set_transform_format(TransformFormat p_transform_format);
	TransformFormat get_transform_format() const;

	void set_instance_count(int p_count);
	int get_instance_count() const;

	void set_visible_instance_count(int p_count);
	int get_visible_instance_count() const;

	void set_physics_interpolation_quality(PhysicsInterpolationQuality p_quality);

	PhysicsInterpolationQuality get_physics_interpolation_quality() const
	{
		return _physics_interpolation_quality;
	}

	void set_instance_transform(int p_instance, const Transform3D& p_transform);
	void set_instance_transform_2d(int p_instance, const Transform2D& p_transform);
	Transform3D get_instance_transform(int p_instance) const;
	Transform2D get_instance_transform_2d(int p_instance) const;

	void set_instance_color(int p_instance, const Color& p_color);
	Color get_instance_color(int p_instance) const;

	void set_instance_custom_data(int p_instance, const Color& p_custom_data);
	Color get_instance_custom_data(int p_instance) const;

	void reset_instance_physics_interpolation(int p_instance);
	void reset_instances_physics_interpolation();

	void set_physics_interpolated(bool p_interpolated);

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	virtual AABB get_aabb() const;

	virtual RID get_rid() const override;

	MultiMesh();
	~MultiMesh();
};

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED

class MultiMeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<MultiMesh> multimesh;

	void _refresh_interpolated();

protected:
	virtual void _physics_interpolated_changed() override;
	virtual void fti_update_servers_xform() override;
	void _notification(int p_what);

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_multimesh(const Ref<MultiMesh>& p_multimesh);
	Ref<MultiMesh> get_multimesh() const;

	AABB get_aabb() const;

private:
#ifndef NAVIGATION_3D_DISABLED
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

public:
#ifndef NAVIGATION_3D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	MultiMeshInstance3D() = default;
	~MultiMeshInstance3D() = default;
};

// --- End scene/3d/multimesh_instance_3d.h ---

// --- Folded from scene/3d/camera_3d.h ---
// --- Folded from core/templates/interpolated_property.h ---
struct Vector2;

namespace InterpolatedPropertyFuncs {
float lerp(float p_a, float p_b, float p_fraction);
double lerp(double p_a, double p_b, float p_fraction);
Vector2 lerp(const Vector2 &p_a, const Vector2 &p_b, float p_fraction);
} //namespace InterpolatedPropertyFuncs

// This class is intended to reduce the boiler plate involved to
// support custom properties to be physics interpolated.

template <class T>
class InterpolatedProperty {
	// Only needs interpolating / updating the servers when
	// curr and prev are different.
	bool _needs_interpolating = false;

	// Each time we hit a physics tick, curr = prev,
	// BUT the server may be out of date, and needs an update
	// on the next frame, OR taking the node off the property update list.
	bool _server_stale = false;

	T _interpolated;
	T curr;
	T prev;

public:
	// FTI depends on the constant flow between current values
	// (on the current tick) and stored previous values (on the previous tick).
	// These should be updated both on each tick, and also on resets.
	void pump() {
		prev = curr;
		_needs_interpolating = false;
		_server_stale = true;
	}
	void reset() { pump(); }

	void set_interpolated_value(const T &p_val) {
		_interpolated = p_val;
	}
	const T &interpolated() const { return _interpolated; }

	// Returns true if an update to the server is needed.
	bool interpolate(float p_interpolation_fraction) {
		if (_needs_interpolating) {
			_interpolated = InterpolatedPropertyFuncs::lerp(prev, curr, p_interpolation_fraction);
			return true;
		}
		if (_server_stale) {
			_interpolated = curr;
			_server_stale = false;
			return true;
		}
		return false;
	}

	operator T() const {
		return curr;
	}

	bool operator==(const T &p_o) const {
		return p_o == curr;
	}

	bool operator!=(const T &p_o) const {
		return p_o != curr;
	}

	InterpolatedProperty &operator=(T p_val) {
		curr = p_val;
		_interpolated = p_val;
		_needs_interpolating = true;
		return *this;
	}
	InterpolatedProperty(T p_val) {
		curr = p_val;
		_interpolated = p_val;
		pump();
	}
	InterpolatedProperty() {
		// Ensure either the constructor is run,
		// or the memory is zeroed if using a fundamental type.
		_interpolated = T{};
		curr = T{};
		prev = T{};
	}
};

// --- End core/templates/interpolated_property.h ---

// --- Folded from scene/3d/velocity_tracker_3d.h ---
#include "core/types.h"

class VelocityTracker3D : public RefCounted
{
	struct PositionHistory
	{
		uint64_t frame = 0;
		Vector3 position;
	};

	bool physics_step = false;
	Vector<PositionHistory> position_history;
	int position_history_len = 0;

public:
	void reset(const Vector3& p_new_pos);
	void set_track_physics_step(bool p_track_physics_step);
	bool is_tracking_physics_step() const;
	void update_position(const Vector3& p_position);
	Vector3 get_tracked_linear_velocity() const;

	VelocityTracker3D();
};



// --- End scene/3d/velocity_tracker_3d.h ---
#include "scene/resources/material.h"
#include "scene/resources/environment.h"
#include "servers/xr/xr_server.h"

class Camera3D : public Node3D
{
public:
	enum ProjectionType
	{
		PROJECTION_PERSPECTIVE,
		PROJECTION_ORTHOGONAL,
		PROJECTION_FRUSTUM
	};

	enum KeepAspect
	{
		KEEP_WIDTH,
		KEEP_HEIGHT
	};

	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP
	};

private:
	bool force_change = false;
	bool current = false;
	Viewport* viewport = nullptr;

	ProjectionType mode = PROJECTION_PERSPECTIVE;

	InterpolatedProperty<real_t> fov = 75.0;
	InterpolatedProperty<real_t> size = 1.0;
	InterpolatedProperty<Vector2> frustum_offset;
	// _ prefix to avoid conflict with Windows defines.
	InterpolatedProperty<real_t> _near = 0.05;
	InterpolatedProperty<real_t> _far = 4000.0;
	real_t v_offset = 0.0;
	real_t h_offset = 0.0;
	KeepAspect keep_aspect = KEEP_HEIGHT;

	RID camera;
	RID scenario_id;

	uint32_t layers = 0xfffff;

	Ref<Environment> environment;
	Ref<CameraAttributes> attributes;
	Ref<Compositor> compositor;
	void _attributes_changed();

	friend class Viewport;
	void _update_audio_listener_state();

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
	Ref<VelocityTracker3D> velocity_tracker;

#ifndef PHYSICS_3D_DISABLED
	RID pyramid_shape;
	Vector<Vector3> pyramid_shape_points;
#endif // PHYSICS_3D_DISABLED

	bool _desired_process_internal = false;
	bool _desired_physics_process_internal = false;

	bool xr_enabled = false;
	String tracker_name = "head";
	String pose_name = "default";
	Ref<XRTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _pose_changed(const Ref<XRPose>& p_pose);

	void _update_process_mode();

protected:
	void set_desired_process_modes(bool p_process_internal, bool p_physics_process_internal);

	virtual void _physics_interpolated_changed() override;
	Transform3D _get_adjusted_camera_transform(const Transform3D& p_xform) const;

	void _update_camera();
	void _request_camera_update();

	virtual void fti_pump_property() override;
	virtual void fti_update_servers_property() override;
	virtual void fti_update_servers_xform() override;

	void _notification(int p_what);

	Projection _get_camera_projection(real_t p_near) const;

public:
	enum
	{
		NOTIFICATION_BECAME_CURRENT = 50,
		NOTIFICATION_LOST_CURRENT = 51
	};

	void set_xr_enabled(bool p_enabled);
	bool is_xr_enabled() const { return xr_enabled; }

	PackedStringArray get_configuration_warnings() const override;

	void set_projection(Camera3D::ProjectionType p_mode);

	void make_current();
	void clear_current(bool p_enable_next = true);
	void set_current(bool p_enabled);
	bool is_current() const;

	RID get_camera() const;

	real_t get_fov() const;
	real_t get_size() const;
	real_t get_far() const;
	real_t get_near() const;
	Vector2 get_frustum_offset() const;

	ProjectionType get_projection() const;

	Transform3D get_camera_transform() const;
	Projection get_camera_projection() const;

	Vector3 project_ray_normal(const Point2& p_pos) const;
	Vector3 project_ray_origin(const Point2& p_pos) const;
	Vector3 project_local_ray_normal(const Point2& p_pos) const;
	Point2 unproject_position(const Vector3& p_pos) const;
	bool is_position_behind(const Vector3& p_pos) const;
	Vector3 project_position(const Point2& p_point, real_t p_z_depth) const;

	Vector<Vector3> get_near_plane_points() const;

	uint32_t get_cull_mask() const;

	bool get_cull_mask_value(int p_layer_number) const;

	Vector<Plane> get_frustum() const;
	bool is_position_in_frustum(const Vector3& p_position) const;

	Ref<Environment> get_environment() const;

	void set_attributes(const Ref<CameraAttributes>& p_effects);
	Ref<CameraAttributes> get_attributes() const;

	Ref<Compositor> get_compositor() const;

	void set_keep_aspect_mode(KeepAspect p_aspect);
	KeepAspect get_keep_aspect_mode() const;

	void set_v_offset(real_t p_offset);
	real_t get_v_offset() const;

	void set_h_offset(real_t p_offset);
	real_t get_h_offset() const;

	DopplerTracking get_doppler_tracking() const;

	Vector3 get_doppler_tracked_velocity() const;

#ifndef PHYSICS_3D_DISABLED
	RID get_pyramid_shape_rid();
#endif // PHYSICS_3D_DISABLED

	Camera3D(bool p_xr = false);
	~Camera3D();
};

// --- Folded from scene/3d/audio_listener_3d.h ---



class AudioListener3D : public Node3D
{
public:
	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP,
	};

	void make_current();
	void clear_current();
	bool is_current() const;

	virtual Transform3D get_listener_transform() const;

	void set_doppler_tracking(DopplerTracking p_tracking);
	DopplerTracking get_doppler_tracking() const;

	Vector3 get_doppler_tracked_velocity() const;

	AudioListener3D();
	~AudioListener3D();

private:
	bool force_change = false;
	bool current = false;

	RID scenario_id;

	friend class Viewport;
	void _update_audio_listener_state();

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
	Ref<VelocityTracker3D> velocity_tracker;

protected:
	void _update_listener();
	virtual void _request_listener_update();

	void _notification(int p_what);

};



// --- End scene/3d/audio_listener_3d.h ---

// --- Folded from scene/3d/audio_stream_player_3d.h ---
#include "core/templates/span.h"

#include "servers/audio/audio_server.h"

#ifndef PHYSICS_3D_DISABLED
class Area3D;
#endif // PHYSICS_3D_DISABLED
struct AudioFrame;
class AudioStream;
class AudioStreamPlayback;
class AudioStreamPlayerInternal;
class VelocityTracker3D;

class AudioStreamPlayer3D : public Node3D
{
public:
	enum AttenuationModel
	{
		ATTENUATION_INVERSE_DISTANCE,
		ATTENUATION_INVERSE_SQUARE_DISTANCE,
		ATTENUATION_LOGARITHMIC,
		ATTENUATION_DISABLED,
	};

	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP
	};

private:
	enum
	{
		MAX_OUTPUTS = 8,
		MAX_INTERSECT_AREAS = 32

	};

	static constexpr int64_t VOLUME_VECTOR_SIZE = AudioServer::MAX_CHANNELS_PER_BUS;

	AudioStreamPlayerInternal* internal = nullptr;

	SafeNumeric<float> setplay{-1.0};
	Ref<AudioStreamPlayback> setplayback;

	AttenuationModel attenuation_model = ATTENUATION_INVERSE_DISTANCE;
	float unit_size = 10.0;
	float max_db = 3.0;
	// Internally used to take doppler tracking into account.
	float actual_pitch_scale = 1.0;

	uint64_t last_mix_count = -1;
	bool force_update_panning = false;

	static void _calc_output_vol(const Vector3& source_dir, real_t tightness,
		FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& output);
	static AudioFrame _calc_output_vol_stereo(const Vector3& source_dir, real_t panning_strength);

#ifndef PHYSICS_3D_DISABLED
	void _calc_reverb_vol(Area3D* area, Vector3 listener_area_pos,
		const FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& direct_path_vol,
		FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& reverb_vol);
#endif // PHYSICS_3D_DISABLED

	static void _listener_changed_cb(void* self)
	{
		reinterpret_cast<AudioStreamPlayer3D*>(self)->force_update_panning = true;
	}

	void _set_playing(bool p_enable);
	bool _is_active() const;
	String _get_actual_bus();
#ifndef PHYSICS_3D_DISABLED
	Area3D* _get_overriding_area();
#endif // PHYSICS_3D_DISABLED
	Vector<AudioFrame> _update_panning();

	uint32_t area_mask = 0;

	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	bool emission_angle_enabled = false;
	float emission_angle = 45.0;
	float emission_angle_filter_attenuation_db = -12.0;
	float attenuation_filter_cutoff_hz = 5000.0;
	float attenuation_filter_db = -24.0;

	float linear_attenuation = 0;

	float max_distance = 0.0;
	bool was_further_than_max_distance_last_frame = false;

	Ref<VelocityTracker3D> velocity_tracker;

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;

	float _get_attenuation_db(float p_distance) const;

	float panning_strength = 1.0f;
	float cached_global_panning_strength = 0.5f;

	/// Hash map storing the bus volumes during the panning update.
	/// This hash map is stored as member for efficiency reasons.
	HashMap<String, Vector<AudioFrame>> bus_volumes;

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	bool _is_autoplay_enabled_bind_compat_86907();
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	void set_stream(Ref<AudioStream> p_stream);
	Ref<AudioStream> get_stream() const;

	void set_volume_db(float p_volume);
	float get_volume_db() const;

	void set_volume_linear(float p_volume);
	float get_volume_linear() const;

	float get_unit_size() const;

	void set_max_db(float p_boost);
	float get_max_db() const;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void play(float p_from_pos = 0.0);
	void seek(float p_seconds);
	void stop();
	bool is_playing() const;
	float get_playback_position();

	void set_bus(const String& p_bus);
	String get_bus() const;

	void set_max_polyphony(int p_max_polyphony);
	int get_max_polyphony() const;

	void set_autoplay(bool p_enable);
	bool is_autoplay_enabled() const;

	float get_max_distance() const;

	void set_area_mask(uint32_t p_mask);
	uint32_t get_area_mask() const;

	bool is_emission_angle_enabled() const;

	float get_emission_angle() const;

	void set_emission_angle_filter_attenuation_db(float p_angle_attenuation_db);
	float get_emission_angle_filter_attenuation_db() const;

	void set_attenuation_filter_cutoff_hz(float p_hz);
	float get_attenuation_filter_cutoff_hz() const;

	void set_attenuation_filter_db(float p_db);
	float get_attenuation_filter_db() const;

	AttenuationModel get_attenuation_model() const;

	void set_doppler_tracking(DopplerTracking p_tracking);
	DopplerTracking get_doppler_tracking() const;

	void set_stream_paused(bool p_pause);
	bool get_stream_paused() const;

	void set_panning_strength(float p_panning_strength);
	float get_panning_strength() const;

	bool has_stream_playback();
	Ref<AudioStreamPlayback> get_stream_playback();

	AudioServer::PlaybackType get_playback_type() const;
	void set_playback_type(AudioServer::PlaybackType p_playback_type);

	AudioStreamPlayer3D();
	~AudioStreamPlayer3D();
};



// --- End scene/3d/audio_stream_player_3d.h ---

// --- Folded from scene/3d/world_environment.h ---
#include "scene/main/node.h"
#include "scene/resources/material.h"
#include "scene/resources/environment.h"

class WorldEnvironment : public Node
{
	Ref<Environment> environment;
	Ref<CameraAttributes> camera_attributes;
	Ref<Compositor> compositor;

	void _update_current_environment();
	void _update_current_camera_attributes();
	void _update_current_compositor();

protected:
	void _notification(int p_what);

public:
	Ref<Environment> get_environment() const;

	Ref<CameraAttributes> get_camera_attributes() const;

	Ref<Compositor> get_compositor() const;

	PackedStringArray get_configuration_warnings() const override;

	WorldEnvironment();
};



// --- End scene/3d/world_environment.h ---
// --- End scene/3d/camera_3d.h ---
// --- End scene/3d/visual_instance_3d.h ---

// --- Folded from scene/3d/skeleton_3d.h ---
#include "core/templates/a_hash_map.h"

// --- Inlined/Folded from scene/resources/3d/skin.h ---
#include "core/io/resource.h"

class Skin : public Resource
{
	struct Bind
	{
		int bone = -1;
		String name;
		Transform3D pose;
	};

	Vector<Bind> binds;

	Bind* binds_ptr = nullptr;
	int bind_count = 0;

protected:
	virtual void reset_state() override;

public:
	void set_bind_count(int p_size);

	inline int get_bind_count() const { return bind_count; }

	void add_bind(int p_bone, const Transform3D& p_pose);
	void add_named_bind(const String& p_name, const Transform3D& p_pose);

	void set_bind_bone(int p_index, int p_bone);
	void set_bind_pose(int p_index, const Transform3D& p_pose);
	void set_bind_name(int p_index, const String& p_name);

	inline int get_bind_bone(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, -1);
		return binds_ptr[p_index].bone;
	}

	inline String get_bind_name(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, String());
		return binds_ptr[p_index].name;
	}

	inline Transform3D get_bind_pose(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, Transform3D());
		return binds_ptr[p_index].pose;
	}

	void clear_binds();

	Skin() = default;
};

typedef int BoneId;

class Skeleton3D;

class SkinReference : public RefCounted
{
	friend class Skeleton3D;

	Skeleton3D* skeleton_node = nullptr;
	RID skeleton;
	Ref<Skin> skin;
	uint32_t bind_count = 0;
	uint64_t skeleton_version = 0;
	Vector<uint32_t> skin_bone_indices;
	uint32_t* skin_bone_indices_ptrs = nullptr;

protected:

public:
	// Public for use as signal callback.
	void _skin_changed();

	RID get_skeleton() const;
	Ref<Skin> get_skin() const;
	~SkinReference();
};

class Skeleton3D : public Node3D
{
#ifdef TOOLS_ENABLED
	bool saving = false;
#endif // TOOLS_ENABLED

#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	bool animate_physical_bones = true;
	Node* simulator = nullptr;
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

public:
	enum ModifierCallbackModeProcess
	{
		MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS,
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE,
		MODIFIER_CALLBACK_MODE_PROCESS_MANUAL,
	};

private:
	friend class SkinReference;

	enum UpdateFlag
	{
		UPDATE_FLAG_NONE = 1,
		UPDATE_FLAG_MODIFIER = 2,
		UPDATE_FLAG_POSE = 4,
	};

	void _update_deferred(UpdateFlag p_update_flag = UPDATE_FLAG_POSE);
	uint8_t update_flags = UPDATE_FLAG_NONE;
	bool updating = false; // Is updating now?
	double update_delta = 0.0;

	struct Bone
	{
		String name;

		int parent = -1;
		Vector<int> child_bones;

		Transform3D rest;
		Transform3D global_rest;

		bool enabled = true;
		bool pose_cache_dirty = true;
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;
		int nested_set_offset = 0; // Offset in nested set of bone hierarchy.
		int nested_set_span = 0;   // Subtree span in nested set of bone hierarchy.

		bool modifier_applied = false;
		Transform3D modifier_pose_cache;

		void make_bone_modified()
		{
			modifier_applied = true;
			modifier_pose_cache = pose_cache;
		}

		void update_pose_cache()
		{
			if (pose_cache_dirty) {
				pose_cache.basis.set_quaternion_scale(pose_rotation, pose_scale);
				pose_cache.origin = pose_position;
				pose_cache_dirty = false;
			}
		}

#ifndef DISABLE_DEPRECATED
		Transform3D pose_global_no_override;
		real_t global_pose_override_amount = 0.0;
		bool global_pose_override_reset = false;
		Transform3D global_pose_override;
#endif // _DISABLE_DEPRECATED
	};

	struct BonePoseBackup
	{
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;

		void save(const Bone& p_bone)
		{
			pose_cache = p_bone.pose_cache;
			pose_position = p_bone.pose_position;
			pose_rotation = p_bone.pose_rotation;
			pose_scale = p_bone.pose_scale;
			global_pose = p_bone.global_pose;
		}

		void restore(Bone& r_bone)
		{
			r_bone.pose_cache = pose_cache;
			r_bone.pose_position = pose_position;
			r_bone.pose_rotation = pose_rotation;
			r_bone.pose_scale = pose_scale;
			r_bone.global_pose = global_pose;
		}
	};

	HashSet<SkinReference*> skin_bindings;
	void _skin_changed();

	mutable LocalVector<Bone> bones;
	mutable bool process_order_dirty = false;

	mutable Vector<int> parentless_bones;
	AHashMap<String, int> name_to_bone_index;

	mutable String concatenated_bone_names;
	void _update_bone_names() const;

	void _make_dirty();
	mutable bool dirty = false;
	mutable bool rest_dirty = false;

	bool show_rest_only = false;
	float motion_scale = 1.0;

	uint64_t version = 1;

	void _update_process_order() const;

	// To process modifiers.
	ModifierCallbackModeProcess modifier_callback_mode_process =
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE;
	bool modifiers_dirty = false;
	void _find_modifiers();
	void _process_modifiers();
	void _process_changed();
	void _make_modifiers_dirty();
	bool modifier_updating = false; // Is modifier updating now?

	// Global bone pose calculation.
	mutable LocalVector<int>
		nested_set_offset_to_bone_index; // Map from Bone::nested_set_offset to bone index.
	mutable LocalVector<bool> bone_global_pose_dirty; // Indexable with Bone::nested_set_offset.
	void _update_bones_nested_set() const;
	int _update_bone_nested_set(int p_bone, int p_offset) const;
	void _make_bone_global_poses_dirty() const;
	void _make_bone_global_pose_subtree_dirty(int p_bone) const;
	void _update_bone_global_pose(int p_bone) const;

#ifndef DISABLE_DEPRECATED
	void _add_bone_bind_compat_88791(const String& p_name);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

protected:
	void _notification(int p_what);
	TypedArray<String> _get_bone_meta_list_bind(int p_bone) const;

	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	enum
	{
		NOTIFICATION_UPDATE_SKELETON = 50
	};

	// Skeleton creation API
	uint64_t get_version() const;
	int add_bone(const String& p_name);
	int find_bone(const String& p_name) const;
	String get_bone_name(int p_bone) const;
	void set_bone_name(int p_bone, const String& p_name);
	String get_concatenated_bone_names() const;

	bool is_bone_parent_of(int p_bone_id, int p_parent_bone_id) const;

	void set_bone_parent(int p_bone, int p_parent);
	int get_bone_parent(int p_bone) const;

	void unparent_bone_and_rest(int p_bone);

	Vector<int> get_bone_children(int p_bone) const;
	Vector<int> get_parentless_bones() const;

	int get_bone_count() const;

	void set_bone_rest(int p_bone, const Transform3D& p_rest);
	Transform3D get_bone_rest(int p_bone) const;
	Transform3D get_bone_global_rest(int p_bone) const;

	void set_bone_enabled(int p_bone, bool p_enabled);
	bool is_bone_enabled(int p_bone) const;

	void set_show_rest_only(bool p_enabled);
	bool is_show_rest_only() const;
	void clear_bones();

	void set_motion_scale(float p_motion_scale);
	float get_motion_scale() const;

	// bone metadata
	void get_bone_meta_list(int p_bone, List<String>* p_list) const;
	bool has_bone_meta(int p_bone, const String& p_key) const;

	// Posing API
	Transform3D get_bone_pose(int p_bone) const;
	Vector3 get_bone_pose_position(int p_bone) const;
	Quaternion get_bone_pose_rotation(int p_bone) const;
	Vector3 get_bone_pose_scale(int p_bone) const;
	void set_bone_pose(int p_bone, const Transform3D& p_pose);
	void set_bone_pose_position(int p_bone, const Vector3& p_position);
	void set_bone_pose_rotation(int p_bone, const Quaternion& p_rotation);
	void set_bone_pose_scale(int p_bone, const Vector3& p_scale);

	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D& p_pose);

	void reset_bone_pose(int p_bone);
	void reset_bone_poses();

	void localize_rests(); // Used for loaders and tools.

	Ref<Skin> create_skin_from_rest_transforms();

	Ref<SkinReference> register_skin(const Ref<Skin>& p_skin);

	void force_update_all_dirty_bones();
	void _force_update_all_dirty_bones() const;
	void force_update_all_bone_transforms();
	void _force_update_all_bone_transforms() const;
	void force_update_bone_children_transforms(int bone_idx);
	void _force_update_bone_children_transforms(int bone_idx) const;
	void force_update_deferred();

	void set_modifier_callback_mode_process(ModifierCallbackModeProcess p_mode);
	ModifierCallbackModeProcess get_modifier_callback_mode_process() const;

	void advance(double p_delta);

#ifndef DISABLE_DEPRECATED
	Transform3D get_bone_global_pose_no_override(int p_bone) const;
	void clear_bones_global_pose_override();
	Transform3D get_bone_global_pose_override(int p_bone) const;
	void set_bone_global_pose_override(
		int p_bone, const Transform3D& p_pose, real_t p_amount, bool p_persistent = false);

	Node* get_simulator();
#ifndef PHYSICS_3D_DISABLED
	void set_animate_physical_bones(bool p_enabled);
	bool get_animate_physical_bones() const;
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String>& p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

public:
	Skeleton3D();
	~Skeleton3D();
};

// --- Folded from scene/3d/skeleton_modifier_3d.h ---


class SkeletonModifier3D : public Node3D
{
	void rebind();

public:
	// For the case to indicate bone axis on basis without custom vector.
	enum BoneAxis
	{
		BONE_AXIS_PLUS_X,
		BONE_AXIS_MINUS_X,
		BONE_AXIS_PLUS_Y,
		BONE_AXIS_MINUS_Y,
		BONE_AXIS_PLUS_Z,
		BONE_AXIS_MINUS_Z,
	};

	static String get_hint_bone_axis() { return "+X,-X,+Y,-Y,+Z,-Z"; }

	// For the case to indicate Head-Tail of the bone.
	enum BoneDirection
	{
		BONE_DIRECTION_PLUS_X,
		BONE_DIRECTION_MINUS_X,
		BONE_DIRECTION_PLUS_Y,
		BONE_DIRECTION_MINUS_Y,
		BONE_DIRECTION_PLUS_Z,
		BONE_DIRECTION_MINUS_Z,
		BONE_DIRECTION_FROM_PARENT,
	};

	static String get_hint_bone_direction() { return "+X,-X,+Y,-Y,+Z,-Z,FromParent"; }

	// For the case to define secondary axis of the bone local space.
	enum SecondaryDirection
	{
		SECONDARY_DIRECTION_NONE,
		SECONDARY_DIRECTION_PLUS_X,
		SECONDARY_DIRECTION_MINUS_X,
		SECONDARY_DIRECTION_PLUS_Y,
		SECONDARY_DIRECTION_MINUS_Y,
		SECONDARY_DIRECTION_PLUS_Z,
		SECONDARY_DIRECTION_MINUS_Z,
		SECONDARY_DIRECTION_CUSTOM,
	};

	static String get_hint_secondary_direction() { return "None,+X,-X,+Y,-Y,+Z,-Z,Custom"; }

	// For the case to define rotation direction without identification plus/minus.
	enum RotationAxis
	{
		ROTATION_AXIS_X,
		ROTATION_AXIS_Y,
		ROTATION_AXIS_Z,
		ROTATION_AXIS_ALL,
		ROTATION_AXIS_CUSTOM,
	};

	static String get_hint_rotation_axis() { return "X,Y,Z,All,Custom"; }

protected:
	bool active = true;
	real_t influence = 1.0;

	void _update_skeleton_path();

	virtual void _skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new);
	virtual void _validate_bone_names();

	virtual void _set_active(bool p_active);
	virtual void _process_modification(double p_delta);
	bool should_check_node_path();

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton() const;

	virtual bool has_process() const
	{
		return false;
	} // Return true if modifier needs to modify bone pose without external animation such as
	  // physics, jiggle and etc.

	bool is_active() const;
	void set_influence(real_t p_influence);
	real_t get_influence() const;

	void process_modification(double p_delta);

	// Utility APIs.
	static Vector3 get_vector_from_bone_axis(BoneAxis p_axis);
	static Vector3 get_vector_from_axis(Vector3::Axis p_axis);
	static Vector3::Axis get_axis_from_bone_axis(BoneAxis p_axis);

	// 3D math.
	static Vector3 limit_length(
		const Vector3& p_origin, const Vector3& p_destination, float p_length);
	static Quaternion get_local_pose_rotation(
		Skeleton3D* p_skeleton, int p_bone, const Quaternion& p_global_pose_rotation);
	static Quaternion get_from_to_rotation(
		const Vector3& p_from, const Vector3& p_to, const Quaternion& p_prev_rot);
	static Quaternion get_from_to_rotation_by_axis(
		const Vector3& p_from, const Vector3& p_to, const Vector3& p_axis);
	static Quaternion get_swing(const Quaternion& p_rotation, const Vector3& p_axis);
	static Vector3 snap_vector_to_plane(const Vector3& p_plane_normal, const Vector3& p_vector);
	static double symmetrize_angle(double p_angle);
	static double get_roll_angle(const Quaternion& p_rotation, const Vector3& p_roll_axis);
	static Vector3 get_projected_normal(
		const Vector3& p_a, const Vector3& p_b, const Vector3& p_point);

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const { return false; }
#endif

	SkeletonModifier3D();
};

// --- Folded from scene/3d/bone_constraint_3d.h ---

#include "scene/resources/material.h"

class BoneConstraint3D : public SkeletonModifier3D {
public:
	enum ConstraintType {
		CONSTRAINT_AIM,
		CONSTRAINT_CONVERT_TRANSFORM,
		CONSTRAINT_COPY_TRANSFORM,
		CONSTRAINT_BONE_TARGET,
		CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		CONSTRAINT_TWIST_DISPERSER,

		TYPE_AIM = CONSTRAINT_AIM,
		TYPE_CONVERT_TRANSFORM = CONSTRAINT_CONVERT_TRANSFORM,
		TYPE_COPY_TRANSFORM = CONSTRAINT_COPY_TRANSFORM,
		TYPE_BONE_TARGET = CONSTRAINT_BONE_TARGET,
		TYPE_LIMIT_ANGULAR_VELOCITY = CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		TYPE_TWIST_DISPERSER = CONSTRAINT_TWIST_DISPERSER,
	};

	enum ReferenceType {
		REFERENCE_TYPE_BONE,
		REFERENCE_TYPE_NODE,
	};

	enum TransformMode {
		TRANSFORM_MODE_POSITION,
		TRANSFORM_MODE_ROTATION,
		TRANSFORM_MODE_SCALE,
	};

	enum TransformFlag {
		TRANSFORM_FLAG_POSITION = 1,
		TRANSFORM_FLAG_ROTATION = 2,
		TRANSFORM_FLAG_SCALE = 4,
		TRANSFORM_FLAG_ALL = TRANSFORM_FLAG_POSITION | TRANSFORM_FLAG_ROTATION | TRANSFORM_FLAG_SCALE,
	};

	enum AxisFlag {
		AXIS_FLAG_X = 1,
		AXIS_FLAG_Y = 2,
		AXIS_FLAG_Z = 4,
		AXIS_FLAG_ALL = AXIS_FLAG_X | AXIS_FLAG_Y | AXIS_FLAG_Z,
	};

	enum DisperseMode {
		DISPERSE_MODE_EVEN,
		DISPERSE_MODE_WEIGHTED,
		DISPERSE_MODE_CUSTOM,
	};

	struct BoneJoint {
		String name;
		int bone = -1;
	};

	struct DisperseJointSetting {
		BoneJoint joint;
		double custom_amount = 1.0;
		double amount = 1.0;
		Vector3 axis;
	};

	typedef Pair<int, Quaternion> BoneRot;

	struct ConstraintSetting {
		float amount = 1.0f;

		String apply_bone_name;
		int apply_bone = -1;

		ReferenceType reference_type = REFERENCE_TYPE_BONE;
		String reference_bone_name;
		int reference_bone = -1;
		NodePath reference_node;

		bool relative = true;
		bool additive = false;

		// Aim settings.
		BoneAxis forward_axis = BONE_AXIS_PLUS_Y;
		bool use_euler = false;
		Vector3::Axis primary_rotation_axis = Vector3::AXIS_X;
		bool use_secondary_rotation = true;

		// Convert Transform settings.
		TransformMode apply_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis apply_axis = Vector3::AXIS_X;
		float apply_range_min = 0.0f;
		float apply_range_max = 0.0f;
		TransformMode reference_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis reference_axis = Vector3::AXIS_X;
		float reference_range_min = 0.0f;
		float reference_range_max = 0.0f;

		// Copy Transform settings.
		uint32_t copy_flags = TRANSFORM_FLAG_ALL;
		uint32_t axis_flags = AXIS_FLAG_ALL;
		uint32_t invert_flags = 0;

		// Chain & Twist Disperser settings (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
		BoneJoint root_bone;
		BoneJoint end_bone;
		bool joints_dirty = false;
		DisperseMode disperse_mode = DISPERSE_MODE_EVEN;
		bool twist_from_rest = true;
		Quaternion twist_from;
		LocalVector<DisperseJointSetting> joints;
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float weight_position = 0.5f;
		Ref<Curve> damping_curve;

		bool is_relative_effective(ConstraintType p_type) const {
			if (p_type != CONSTRAINT_AIM && reference_type == REFERENCE_TYPE_NODE) {
				return false;
			}
			return relative;
		}
	};

protected:
	ConstraintType constraint_type = CONSTRAINT_COPY_TRANSFORM;
	LocalVector<ConstraintSetting> settings;

	// BoneTarget state.
	String target_bone_name;
	int target_bone = -1;

	// LimitAngularVelocity state.
	bool exclude = false;
	double max_angular_velocity = Math::TAU;
	LocalVector<BoneJoint> velocity_joints;
	LocalVector<BoneRot> velocity_bones;
	bool velocity_joints_dirty = false;
	bool velocity_init_needed = true;

	// TwistDisperser state.
	bool mutable_bone_axes = true;

	void _notification(int p_what);
	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void _set_active(bool p_active) override;
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount);
	void _process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_bone_target(Skeleton3D *p_skeleton);
	void _process_limit_angular_velocity(Skeleton3D *p_skeleton, double p_delta);
	void _process_twist_disperser(Skeleton3D *p_skeleton);

	void _make_velocity_joints_dirty();
	void _update_velocity_joints();
	bool _is_velocity_joint_contained(int p_bone) const;

	void _make_all_twist_joints_dirty();
	void _make_twist_joints_dirty(int p_index);
	void _update_twist_joints(int p_index);
	void _update_twist_curve(int p_index);

public:
	BoneConstraint3D(ConstraintType p_type = CONSTRAINT_COPY_TRANSFORM);
	~BoneConstraint3D();

	void set_constraint_type(ConstraintType p_type);
	ConstraintType get_constraint_type() const { return constraint_type; }

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const override { return constraint_type == CONSTRAINT_BONE_TARGET; }
#endif // TOOLS_ENABLED

	virtual bool has_process() const override { return constraint_type == CONSTRAINT_LIMIT_ANGULAR_VELOCITY; }

	void set_setting_count(int p_count);
	int get_setting_count() const;
	void clear_settings();

	// Common setting properties.
	void set_amount(int p_index, float p_amount);
	float get_amount(int p_index) const;

	void set_apply_bone_name(int p_index, const String &p_name);
	String get_apply_bone_name(int p_index) const;
	void set_apply_bone(int p_index, int p_bone);
	int get_apply_bone(int p_index) const;

	void set_reference_type(int p_index, ReferenceType p_type);
	ReferenceType get_reference_type(int p_index) const;

	void set_reference_bone_name(int p_index, const String &p_name);
	String get_reference_bone_name(int p_index) const;
	void set_reference_bone(int p_index, int p_bone);
	int get_reference_bone(int p_index) const;

	void set_reference_node(int p_index, const NodePath &p_node);
	NodePath get_reference_node(int p_index) const;

	void set_relative(int p_index, bool p_enabled);
	bool is_relative(int p_index) const;
	void set_additive(int p_index, bool p_enabled);
	bool is_additive(int p_index) const;

	// Aim properties.
	void set_forward_axis(int p_index, BoneAxis p_axis);
	BoneAxis get_forward_axis(int p_index) const;
	void set_use_euler(int p_index, bool p_enabled);
	bool is_using_euler(int p_index) const;
	void set_primary_rotation_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_primary_rotation_axis(int p_index) const;
	void set_use_secondary_rotation(int p_index, bool p_enabled);
	bool is_using_secondary_rotation(int p_index) const;

	// Convert Transform properties.
	void set_apply_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_apply_transform_mode(int p_index) const;
	void set_apply_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_apply_axis(int p_index) const;
	void set_apply_range_min(int p_index, float p_range_min);
	float get_apply_range_min(int p_index) const;
	void set_apply_range_max(int p_index, float p_range_max);
	float get_apply_range_max(int p_index) const;

	void set_reference_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_reference_transform_mode(int p_index) const;
	void set_reference_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_reference_axis(int p_index) const;
	void set_reference_range_min(int p_index, float p_range_min);
	float get_reference_range_min(int p_index) const;
	void set_reference_range_max(int p_index, float p_range_max);
	float get_reference_range_max(int p_index) const;

	// Copy Transform properties.
	void set_copy_flags(int p_index, uint32_t p_copy_flags);
	uint32_t get_copy_flags(int p_index) const;
	void set_copy_position(int p_index, bool p_enabled);
	bool is_position_copying(int p_index) const;
	void set_copy_rotation(int p_index, bool p_enabled);
	bool is_rotation_copying(int p_index) const;
	void set_copy_scale(int p_index, bool p_enabled);
	bool is_scale_copying(int p_index) const;

	void set_axis_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_axis_flags(int p_index) const;
	void set_axis_x_enabled(int p_index, bool p_enabled);
	bool is_axis_x_enabled(int p_index) const;
	void set_axis_y_enabled(int p_index, bool p_enabled);
	bool is_axis_y_enabled(int p_index) const;
	void set_axis_z_enabled(int p_index, bool p_enabled);
	bool is_axis_z_enabled(int p_index) const;

	void set_invert_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_invert_flags(int p_index) const;
	void set_axis_x_inverted(int p_index, bool p_enabled);
	bool is_axis_x_inverted(int p_index) const;
	void set_axis_y_inverted(int p_index, bool p_enabled);
	bool is_axis_y_inverted(int p_index) const;
	void set_axis_z_inverted(int p_index, bool p_enabled);
	bool is_axis_z_inverted(int p_index) const;

	// BoneTarget properties (CONSTRAINT_BONE_TARGET / TYPE_BONE_TARGET).
	void set_bone_name(const String &p_bone_name);
	String get_bone_name() const;
	void set_bone(int p_bone);
	int get_bone() const;

	// Chain root/end properties (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
	void set_root_bone_name(int p_index, const String &p_bone_name);
	String get_root_bone_name(int p_index) const;
	void set_root_bone(int p_index, int p_bone);
	int get_root_bone(int p_index) const;

	void set_end_bone_name(int p_index, const String &p_bone_name);
	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	// LimitAngularVelocity properties (CONSTRAINT_LIMIT_ANGULAR_VELOCITY / TYPE_LIMIT_ANGULAR_VELOCITY).
	void set_chain_count(int p_count);
	int get_chain_count() const;
	void clear_chains();
	String get_joint_bone_name(int p_index) const;
	int get_joint_bone(int p_index) const;
	int get_velocity_joint_count() const;
	void set_max_angular_velocity(double p_angular_velocity);
	double get_max_angular_velocity() const;
	void set_exclude(bool p_exclude);
	bool is_exclude() const;
	void reset();

	// TwistDisperser properties (CONSTRAINT_TWIST_DISPERSER / TYPE_TWIST_DISPERSER).
	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;
	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_bone_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_twist_from_rest(int p_index, bool p_enabled);
	bool is_twist_from_rest(int p_index) const;
	void set_twist_from(int p_index, const Quaternion &p_from);
	Quaternion get_twist_from(int p_index) const;
	void set_disperse_mode(int p_index, DisperseMode p_disperse_mode);
	DisperseMode get_disperse_mode(int p_index) const;
	void set_weight_position(int p_index, float p_position);
	float get_weight_position(int p_index) const;
	void set_damping_curve(int p_index, const Ref<Curve> &p_damping_curve);
	Ref<Curve> get_damping_curve(int p_index) const;
	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;
	void set_joint_twist_amount(int p_index, int p_joint, float p_amount);
	float get_joint_twist_amount(int p_index, int p_joint) const;
	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;
};

// --- End scene/3d/bone_constraint_3d.h ---

// --- Folded from scene/resources/3d/joint_limitation_3d.h ---
#include "core/io/resource.h"

#ifdef TOOLS_ENABLED
#include "scene/resources/mesh.h"
#endif // TOOLS_ENABLED

class JointLimitation3D : public Resource
{
	real_t angle = Math::TAU * 0.25;

	Vector3 _solve(const Vector3& p_direction) const;

public:
	void set_angle(real_t p_angle);
	real_t get_angle() const;

	Quaternion make_space(const Vector3& p_local_forward_vector,
		const Vector3& p_local_right_vector, const Quaternion& p_rotation_offset) const;

	Vector3 solve(const Vector3& p_local_forward_vector, const Vector3& p_local_right_vector,
		const Quaternion& p_rotation_offset, const Vector3& p_local_current_vector) const;

#ifdef TOOLS_ENABLED
	void draw_shape(Ref<SurfaceTool>& p_surface_tool, const Transform3D& p_transform,
		float p_bone_length, const Color& p_color) const;
#endif // TOOLS_ENABLED
};

// --- End scene/resources/3d/joint_limitation_3d.h ---

// --- Folded from scene/3d/physics/collision_object_3d.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"

#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

// Plain non-virtual composed component for 3D collision state (shared by PhysicsBody3D and Area3D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape3D/CollisionPolygon3D owners.
struct CollisionState3D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform3D local_xform;
		Ref<Shape3D> shape;
		bool disabled = false;
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform3D xform;
		Node *owner_node = nullptr;

		struct ShapeBase {
			RID debug_shape;
			Ref<Shape3D> shape;
			int index = 0;
		};

		Vector<ShapeBase> shapes;
		bool disabled = false;
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS3DE::BodyMode body_mode = PS3DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;
	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	int debug_shapes_count = 0;
	Transform3D debug_shape_old_transform;

	void init(bool p_area, PS3DE::BodyMode p_mode = PS3DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node3D *p_owner, int p_what);
	void on_transform_changed(const Node3D *p_owner);
	void clear_debug_shapes();

	void apply_disabled(Node3D *p_owner);
	void apply_enabled(Node3D *p_owner);
	void update_pickable(const Node3D *p_owner);
	void set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node3D *p_owner, DisableMode p_mode);
	void set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable);

	// Inline shape slots (no child CollisionShape3D node required).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform);
	Transform3D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape3D / CollisionPolygon3D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform);
	Transform3D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape3D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};

// --- Folded from scene/3d/physics/kinematic_collision_3d.h ---
#include "servers/physics_3d/physics_server_3d.h"

class KinematicCollision3D : public RefCounted
{
	friend class PhysicsBody3D;
	PS3DT::MotionResult result;

protected:

public:
	Vector3 get_travel() const;
	Vector3 get_remainder() const;
	int get_collision_count() const;
	real_t get_depth() const;
	Vector3 get_position(int p_collision_index = 0) const;
	Vector3 get_normal(int p_collision_index = 0) const;
	real_t get_angle(
		int p_collision_index = 0, const Vector3& p_up_direction = Vector3(0.0, 1.0, 0.0)) const;
	RID get_collider_rid(int p_collision_index = 0) const;
	int get_collider_shape_index(int p_collision_index = 0) const;
	Vector3 get_collider_velocity(int p_collision_index = 0) const;
};



// --- End scene/3d/physics/kinematic_collision_3d.h ---

// --- Folded from scene/3d/physics/physics_body_3d.h ---


#include "scene/resources/material.h"
#include "servers/physics_3d/physics_server_3d.h"

class PhysicsBody3D;
class PhysicalBoneSimulator3D;

class VehicleWheel3D : public Node3D {
	friend class PhysicsBody3D;

	struct WheelXform {
		Vector3 up;
		Vector3 right;
		Vector3 origin;
		real_t steering = 0.0f;
	};

	struct FTIData {
		real_t curr_rotation = 0.0f;
		real_t curr_rotation_delta = 0.0f;
		WheelXform curr;
		WheelXform prev;
		bool unset = true;
		bool reset_queued = false;

		void rotate(real_t p_delta) {
			curr_rotation += p_delta;
			curr_rotation_delta = p_delta;
			double wrapped = Math::fmod(curr_rotation + Math::PI, Math::TAU);
			if (wrapped < 0) {
				wrapped += Math::TAU;
			}
			curr_rotation = wrapped - Math::PI;
		}

		void pump() {
			prev = curr;
			curr_rotation_delta = 0.0f;
		}

		void update_world_xform(Transform3D &r_xform, real_t p_interpolation_fraction);
	} fti_data;

	Transform3D m_worldTransform;
	Transform3D local_xform;
	bool engine_traction = false;
	bool steers = false;

	Vector3 m_chassisConnectionPointCS;
	Vector3 m_wheelDirectionCS;
	Vector3 m_wheelAxleCS;

	real_t m_suspensionRestLength = 0.15f;
	real_t m_maxSuspensionTravel = 0.2f;
	real_t m_wheelRadius = 0.5f;

	real_t m_suspensionStiffness = 5.88f;
	real_t m_wheelsDampingCompression = 0.83f;
	real_t m_wheelsDampingRelaxation = 0.88f;
	real_t m_frictionSlip = 10.5f;
	real_t m_maxSuspensionForce = 6000.0f;
	bool m_bIsFrontWheel = false;

	PhysicsBody3D *body = nullptr;

	real_t m_steering = 0.0f;
	real_t m_rotation = 0.0f;
	real_t m_deltaRotation = 0.0f;
	real_t m_rpm = 0.0f;
	real_t m_rollInfluence = 0.1f;
	real_t m_engineForce = 0.0f;
	real_t m_brake = 0.0f;

	real_t m_clippedInvContactDotSuspension = 1.0f;
	real_t m_suspensionRelativeVelocity = 0.0f;
	real_t m_wheelsSuspensionForce = 0.0f;
	real_t m_skidInfo = 0.0f;

	struct RaycastInfo {
		Vector3 m_contactNormalWS;
		Vector3 m_contactPointWS;
		real_t m_suspensionLength = 0.0f;
		Vector3 m_hardPointWS;
		Vector3 m_wheelDirectionWS;
		Vector3 m_wheelAxleWS;
		bool m_isInContact = false;
		PhysicsBody3D *m_groundObject = nullptr;
	} m_raycastInfo;

	void _update(PhysicsDirectBodyState3D *s);

protected:
	void _notification(int p_what);

public:
	void set_radius(real_t p_radius) { m_wheelRadius = p_radius; }
	real_t get_radius() const { return m_wheelRadius; }

	void set_suspension_rest_length(real_t p_length) { m_suspensionRestLength = p_length; }
	real_t get_suspension_rest_length() const { return m_suspensionRestLength; }

	void set_suspension_travel(real_t p_length) { m_maxSuspensionTravel = p_length; }
	real_t get_suspension_travel() const { return m_maxSuspensionTravel; }

	void set_suspension_stiffness(real_t p_value) { m_suspensionStiffness = p_value; }
	real_t get_suspension_stiffness() const { return m_suspensionStiffness; }

	void set_suspension_max_force(real_t p_value) { m_maxSuspensionForce = p_value; }
	real_t get_suspension_max_force() const { return m_maxSuspensionForce; }

	void set_damping_compression(real_t p_value) { m_wheelsDampingCompression = p_value; }
	real_t get_damping_compression() const { return m_wheelsDampingCompression; }

	void set_damping_relaxation(real_t p_value) { m_wheelsDampingRelaxation = p_value; }
	real_t get_damping_relaxation() const { return m_wheelsDampingRelaxation; }

	void set_friction_slip(real_t p_value) { m_frictionSlip = p_value; }
	real_t get_friction_slip() const { return m_frictionSlip; }

	void set_use_as_traction(bool p_enable) { engine_traction = p_enable; }
	bool is_used_as_traction() const { return engine_traction; }

	void set_use_as_steering(bool p_enabled) { steers = p_enabled; }
	bool is_used_as_steering() const { return steers; }

	bool is_in_contact() const { return m_raycastInfo.m_isInContact; }
	Vector3 get_contact_point() const { return m_raycastInfo.m_contactPointWS; }
	Vector3 get_contact_normal() const { return m_raycastInfo.m_contactNormalWS; }
	Node3D *get_contact_body() const;

	void set_roll_influence(real_t p_value) { m_rollInfluence = p_value; }
	real_t get_roll_influence() const { return m_rollInfluence; }

	real_t get_skidinfo() const { return m_skidInfo; }
	real_t get_rpm() const { return m_rpm; }

	void set_engine_force(real_t p_engine_force) { m_engineForce = p_engine_force; }
	real_t get_engine_force() const { return m_engineForce; }

	void set_brake(real_t p_brake) { m_brake = p_brake; }
	real_t get_brake() const { return m_brake; }

	void set_steering(real_t p_steering) { m_steering = p_steering; }
	real_t get_steering() const { return m_steering; }

	virtual PackedStringArray get_configuration_warnings() const override;

	VehicleWheel3D() = default;
};

class PhysicsBody3D : public Node3D {
	friend class VehicleWheel3D;

public:
	enum BodyMode {
		MODE_STATIC,
		MODE_ANIMATABLE,
		MODE_CHARACTER,
		MODE_RIGID,
		MODE_VEHICLE,
		MODE_PHYSICAL_BONE,
	};

	enum FreezeMode {
		FREEZE_MODE_STATIC,
		FREEZE_MODE_KINEMATIC,
	};

	enum CenterOfMassMode {
		CENTER_OF_MASS_MODE_AUTO,
		CENTER_OF_MASS_MODE_CUSTOM,
	};

	enum DampMode {
		DAMP_MODE_COMBINE,
		DAMP_MODE_REPLACE,
	};

	enum MotionMode {
		MOTION_MODE_GROUNDED,
		MOTION_MODE_FLOATING,
	};

	enum PlatformOnLeave {
		PLATFORM_ON_LEAVE_ADD_VELOCITY,
		PLATFORM_ON_LEAVE_ADD_UPWARD_VELOCITY,
		PLATFORM_ON_LEAVE_DO_NOTHING,
	};

	enum BoneJointType {
		BONE_JOINT_NONE,
		BONE_JOINT_PIN,
		BONE_JOINT_CONE,
		BONE_JOINT_HINGE,
		BONE_JOINT_SLIDER,
		BONE_JOINT_6DOF,
	};

	using DisableMode = CollisionState3D::DisableMode;

	// Plain composed character mover state (for MODE_CHARACTER).
	struct CharacterMover3D {
		struct FloorFlags {
			bool floor = false;
			bool wall = false;
			bool ceiling = false;

			FloorFlags() = default;
			FloorFlags(bool p_floor, bool p_wall, bool p_ceiling) :
					floor(p_floor), wall(p_wall), ceiling(p_ceiling) {}
		};

		real_t margin = 0.001f;
		MotionMode motion_mode = MOTION_MODE_GROUNDED;
		PlatformOnLeave platform_on_leave = PLATFORM_ON_LEAVE_ADD_VELOCITY;

		FloorFlags flags;
		bool floor_constant_speed = false;
		bool floor_stop_on_slope = true;
		bool floor_block_on_wall = true;
		bool slide_on_ceiling = true;
		int max_slides = 6;
		int platform_layer = 0;
		RID platform_rid;
		uint32_t platform_floor_layers = UINT32_MAX;
		uint32_t platform_wall_layers = 0;
		real_t floor_snap_length = 0.1f;
		real_t floor_max_angle = Math::deg_to_rad((real_t)45.0);
		real_t wall_min_slide_angle = Math::deg_to_rad((real_t)15.0);
		Vector3 up_direction = Vector3(0.0f, 1.0f, 0.0f);
		Vector3 velocity;
		Vector3 floor_normal;
		Vector3 wall_normal;
		Vector3 ceiling_normal;
		Vector3 last_motion;
		Vector3 platform_velocity;
		Vector3 platform_angular_velocity;
		Vector3 platform_ceiling_velocity;
		Vector3 previous_position;
		Vector3 real_velocity;

		Vector<PS3DT::MotionResult> motion_results;
		Vector<Ref<KinematicCollision3D>> slide_colliders;
	};

	// Plain composed vehicle rig state (for MODE_VEHICLE).
	struct VehicleRig3D {
		real_t engine_force = 0.0f;
		real_t brake = 0.0f;
		real_t pitch_control = 0.0f;
		real_t steering = 0.0f;
		real_t speed_km_hour = 0.0f;
		HashSet<RID> exclude;
		Vector<Vector3> forward_ws;
		Vector<Vector3> axle;
		Vector<real_t> forward_impulse;
		Vector<real_t> side_impulse;
		Vector<VehicleWheel3D *> wheels;
	};

	// Plain value-type joint configuration & bone attachment state (for MODE_PHYSICAL_BONE).
	struct PhysicalBoneState3D {
		struct SixDOFAxisParams {
			bool linear_limit_enabled = true;
			real_t linear_limit_upper = 0.0f;
			real_t linear_limit_lower = 0.0f;
			real_t linear_limit_softness = 0.7f;
			real_t linear_restitution = 0.5f;
			real_t linear_damping = 1.0f;
			bool linear_spring_enabled = false;
			real_t linear_spring_stiffness = 0.0f;
			real_t linear_spring_damping = 0.0f;
			real_t linear_equilibrium_point = 0.0f;
			bool angular_limit_enabled = true;
			real_t angular_limit_upper = 0.0f;
			real_t angular_limit_lower = 0.0f;
			real_t angular_limit_softness = 0.5f;
			real_t angular_restitution = 0.0f;
			real_t angular_damping = 1.0f;
			real_t erp = 0.5f;
			bool angular_spring_enabled = false;
			real_t angular_spring_stiffness = 0.0f;
			real_t angular_spring_damping = 0.0f;
			real_t angular_equilibrium_point = 0.0f;
		};

		struct JointParams {
			// Pin / Cone / Hinge shared parameters.
			real_t bias = 0.3f;
			real_t damping = 1.0f;
			real_t impulse_clamp = 0.0f;
			real_t swing_span = Math::PI * 0.25f;
			real_t twist_span = Math::PI;
			real_t softness = 0.8f;
			real_t relaxation = 1.0f;

			// Hinge / Slider limits.
			bool angular_limit_enabled = false;
			real_t angular_limit_upper = Math::PI * 0.5f;
			real_t angular_limit_lower = -Math::PI * 0.5f;
			real_t angular_limit_bias = 0.3f;
			real_t angular_limit_softness = 0.9f;
			real_t angular_limit_relaxation = 1.0f;
			real_t angular_limit_restitution = 0.7f;
			real_t angular_limit_damping = 1.0f;

			real_t linear_limit_upper = 1.0f;
			real_t linear_limit_lower = -1.0f;
			real_t linear_limit_softness = 1.0f;
			real_t linear_limit_restitution = 0.7f;
			real_t linear_limit_damping = 1.0f;

			SixDOFAxisParams six_dof_axes[3];
		};

		BoneJointType joint_type = BONE_JOINT_NONE;
		JointParams joint_params;
		Transform3D joint_offset;
		RID joint;
		Transform3D body_offset;
		Transform3D body_offset_inverse;
		bool simulate_physics = false;
		bool internal_simulate_physics = false;
		int bone_id = -1;
		String bone_name;
		real_t bounce = 0.0f;
		real_t friction = 1.0f;
#ifdef TOOLS_ENABLED
		bool gizmo_move_joint = false;
#endif
	};

private:
	BodyMode mode = MODE_STATIC;
	CollisionState3D collision_state;
	Ref<KinematicCollision3D> motion_cache;
	uint16_t locked_axis = 0;

	// Static / Animatable properties.
	Vector3 constant_linear_velocity;
	Vector3 constant_angular_velocity;
	bool sync_to_physics = true;
	Transform3D last_valid_transform;

	// Rigid / Vehicle / PhysicalBone properties.
	Ref<PhysicsMaterial> physics_material_override;
	bool can_sleep = true;
	bool lock_rotation = false;
	bool freeze = false;
	FreezeMode freeze_mode = FREEZE_MODE_STATIC;

	real_t mass = 1.0f;
	Vector3 inertia;
	CenterOfMassMode center_of_mass_mode = CENTER_OF_MASS_MODE_AUTO;
	Vector3 center_of_mass;
	Basis inverse_inertia_tensor;
	real_t gravity_scale = 1.0f;

	DampMode linear_damp_mode = DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = DAMP_MODE_COMBINE;
	real_t linear_damp = 0.0f;
	real_t angular_damp = 0.0f;

	Vector3 linear_velocity;
	Vector3 angular_velocity;
	bool sleeping = false;
	bool ccd = false;
	int max_contacts_reported = 0;
	int contact_count = 0;
	bool custom_integrator = false;
	bool contact_monitor = false;

	CharacterMover3D character;
	VehicleRig3D vehicle;
	PhysicalBoneState3D bone_state;

	void _apply_server_body_mode();
	void _update_kinematic_motion();
	void _body_state_changed(PhysicsDirectBodyState3D *p_state);

	// Character helpers.
	void _set_collision_direction(const PS3DT::MotionResult &p_result, CharacterMover3D::FloorFlags &r_state, CharacterMover3D::FloorFlags p_apply_state = CharacterMover3D::FloorFlags(true, true, true));
	bool _on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up);
	void _snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up);

	// Vehicle helpers.
	struct VehicleWheelContactPoint {
		PhysicsDirectBodyState3D *m_s = nullptr;
		PhysicsBody3D *m_body1 = nullptr;
		Vector3 m_frictionPositionWorld;
		Vector3 m_frictionDirectionWorld;
		real_t m_jacDiagABInv = 0.0f;
		real_t m_maxImpulse = 0.0f;

		VehicleWheelContactPoint(PhysicsDirectBodyState3D *s, PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld, real_t maxImpulse);
	};

	void _resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1, PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, real_t p_rollInfluence);
	real_t _calc_rolling_friction(VehicleWheelContactPoint &contactPoint);
	void _update_vehicle_friction(PhysicsDirectBodyState3D *s);
	void _update_vehicle_suspension(PhysicsDirectBodyState3D *s);
	real_t _vehicle_ray_cast(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s);
	void _update_wheel(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_vehicle_process_mode();
	void _step_vehicle(PhysicsDirectBodyState3D *p_state);

	// Physical bone helpers.
	void _start_bone_physics_simulation();
	void _stop_bone_physics_simulation();

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;
	virtual void fti_pump_xform() override;

public:
	PhysicsBody3D(BodyMode p_mode = MODE_STATIC);
	~PhysicsBody3D();

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const { return mode; }

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	// Collision state forwarding.
	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_collision_priority(real_t p_priority) { collision_state.set_collision_priority(p_priority); }
	real_t get_collision_priority() const { return collision_state.collision_priority; }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape slots (direct collision shapes without child CollisionShape3D).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform) { collision_state.set_inline_shape_transform(p_index, p_xform); }
	Transform3D get_inline_shape_transform(int p_index) const { return collision_state.get_inline_shape_transform(p_index); }
	void set_inline_shape_disabled(int p_index, bool p_disabled) { collision_state.set_inline_shape_disabled(p_index, p_disabled); }
	bool is_inline_shape_disabled(int p_index) const { return collision_state.is_inline_shape_disabled(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	// Common PhysicsBody3D motion, locks & exceptions.
	bool move_and_collide(const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult &r_result, bool p_test_only = false, bool p_cancel_sliding = true);
	bool test_move(const Transform3D &p_from, const Vector3 &p_motion, const Ref<KinematicCollision3D> &r_collision = Ref<KinematicCollision3D>(), real_t p_margin = 0.001f, bool p_recovery_as_collision = false, int p_max_collisions = 1);
	Vector3 get_gravity() const;

	void set_axis_lock(PS3DE::BodyAxis p_axis, bool p_lock);
	bool get_axis_lock(PS3DE::BodyAxis p_axis) const;

	void add_collision_exception_with(Node *rp_node);
	void remove_collision_exception_with(Node *rp_node);

	// Static / Animatable API.
	void set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override);
	Ref<PhysicsMaterial> get_physics_material_override() const { return physics_material_override; }
	void set_constant_linear_velocity(const Vector3 &p_vel) { constant_linear_velocity = p_vel; }
	Vector3 get_constant_linear_velocity() const { return constant_linear_velocity; }
	void set_constant_angular_velocity(const Vector3 &p_vel) { constant_angular_velocity = p_vel; }
	Vector3 get_constant_angular_velocity() const { return constant_angular_velocity; }
	void set_sync_to_physics(bool p_enable);
	bool is_sync_to_physics_enabled() const { return sync_to_physics; }

	// Rigid / Dynamic API.
	void set_lock_rotation_enabled(bool p_lock_rotation);
	bool is_lock_rotation_enabled() const { return lock_rotation; }
	void set_freeze_enabled(bool p_freeze);
	bool is_freeze_enabled() const { return freeze; }
	void set_freeze_mode(FreezeMode p_freeze_mode);
	FreezeMode get_freeze_mode() const { return freeze_mode; }

	void set_mass(real_t p_mass);
	real_t get_mass() const { return mass; }
	real_t get_inverse_mass() const;

	void set_inertia(const Vector3 &p_inertia) { inertia = p_inertia; }
	const Vector3 &get_inertia() const { return inertia; }
	Basis get_inverse_inertia_tensor() const { return inverse_inertia_tensor; }

	void set_center_of_mass_mode(CenterOfMassMode p_mode) { center_of_mass_mode = p_mode; }
	CenterOfMassMode get_center_of_mass_mode() const { return center_of_mass_mode; }
	void set_center_of_mass(const Vector3 &p_center_of_mass) { center_of_mass = p_center_of_mass; }
	const Vector3 &get_center_of_mass() const { return center_of_mass; }

	void set_linear_velocity(const Vector3 &p_velocity) { linear_velocity = p_velocity; }
	Vector3 get_linear_velocity() const;
	void set_axis_velocity(const Vector3 &p_axis);
	void set_angular_velocity(const Vector3 &p_velocity) { angular_velocity = p_velocity; }
	Vector3 get_angular_velocity() const;

	void set_gravity_scale(real_t p_gravity_scale) { gravity_scale = p_gravity_scale; }
	real_t get_gravity_scale() const { return gravity_scale; }

	void set_linear_damp_mode(DampMode p_mode) { linear_damp_mode = p_mode; }
	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode) { angular_damp_mode = p_mode; }
	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_linear_damp(real_t p_linear_damp) { linear_damp = p_linear_damp; }
	real_t get_linear_damp() const { return linear_damp; }
	void set_angular_damp(real_t p_angular_damp) { angular_damp = p_angular_damp; }
	real_t get_angular_damp() const { return angular_damp; }

	void set_use_custom_integrator(bool p_enable);
	bool is_using_custom_integrator() const { return custom_integrator; }
	void set_sleeping(bool p_sleeping) { sleeping = p_sleeping; }
	bool is_sleeping() const { return sleeping; }
	void set_can_sleep(bool p_active) { can_sleep = p_active; }
	bool is_able_to_sleep() const { return can_sleep; }
	void set_contact_monitor(bool p_enabled) { contact_monitor = p_enabled; }
	bool is_contact_monitor_enabled() const { return contact_monitor; }
	void set_max_contacts_reported(int p_amount);
	int get_max_contacts_reported() const { return max_contacts_reported; }
	int get_contact_count() const { return contact_count; }
	void set_use_continuous_collision_detection(bool p_enable);
	bool is_using_continuous_collision_detection() const { return ccd; }

	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3());
	void apply_torque_impulse(const Vector3 &p_impulse);
	void apply_central_force(const Vector3 &p_force);
	void apply_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void apply_torque(const Vector3 &p_torque);
	void add_constant_central_force(const Vector3 &p_force);
	void add_constant_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void add_constant_torque(const Vector3 &p_torque);
	void set_constant_force(const Vector3 &p_force);
	Vector3 get_constant_force() const;
	void set_constant_torque(const Vector3 &p_torque);
	Vector3 get_constant_torque() const;

	// CharacterMover3D API.
	CharacterMover3D &get_character_mover() { return character; }
	const CharacterMover3D &get_character_mover() const { return character; }
	bool move_and_slide();
	void apply_floor_snap();
	const Vector3 &get_velocity() const { return character.velocity; }
	void set_velocity(const Vector3 &p_velocity) { character.velocity = p_velocity; }
	const Vector3 &get_up_direction() const { return character.up_direction; }
	void set_up_direction(const Vector3 &p_up_direction);
	bool is_on_floor() const { return character.flags.floor; }
	bool is_on_floor_only() const { return character.flags.floor && !character.flags.wall && !character.flags.ceiling; }
	bool is_on_wall() const { return character.flags.wall; }
	bool is_on_wall_only() const { return character.flags.wall && !character.flags.floor && !character.flags.ceiling; }
	bool is_on_ceiling() const { return character.flags.ceiling; }
	bool is_on_ceiling_only() const { return character.flags.ceiling && !character.flags.floor && !character.flags.wall; }
	const Vector3 &get_last_motion() const { return character.last_motion; }
	Vector3 get_position_delta() const { return get_global_transform().origin - character.previous_position; }
	const Vector3 &get_floor_normal() const { return character.floor_normal; }
	const Vector3 &get_wall_normal() const { return character.wall_normal; }
	const Vector3 &get_real_velocity() const { return character.real_velocity; }
	real_t get_floor_angle(const Vector3 &p_up_direction = Vector3(0.0f, 1.0f, 0.0f)) const;
	const Vector3 &get_platform_velocity() const { return character.platform_velocity; }
	const Vector3 &get_platform_angular_velocity() const { return character.platform_angular_velocity; }
	int get_slide_collision_count() const { return character.motion_results.size(); }
	PS3DT::MotionResult get_slide_collision(int p_bounce) const;
	void set_safe_margin(real_t p_margin) { character.margin = p_margin; }
	real_t get_safe_margin() const { return character.margin; }
	bool is_floor_stop_on_slope_enabled() const { return character.floor_stop_on_slope; }
	void set_floor_stop_on_slope_enabled(bool p_enabled) { character.floor_stop_on_slope = p_enabled; }
	bool is_floor_constant_speed_enabled() const { return character.floor_constant_speed; }
	void set_floor_constant_speed_enabled(bool p_enabled) { character.floor_constant_speed = p_enabled; }
	bool is_floor_block_on_wall_enabled() const { return character.floor_block_on_wall; }
	void set_floor_block_on_wall_enabled(bool p_enabled) { character.floor_block_on_wall = p_enabled; }
	bool is_slide_on_ceiling_enabled() const { return character.slide_on_ceiling; }
	void set_slide_on_ceiling_enabled(bool p_enabled) { character.slide_on_ceiling = p_enabled; }
	int get_max_slides() const { return character.max_slides; }
	void set_max_slides(int p_max_slides);
	real_t get_floor_max_angle() const { return character.floor_max_angle; }
	void set_floor_max_angle(real_t p_radians) { character.floor_max_angle = p_radians; }
	real_t get_floor_snap_length() const { return character.floor_snap_length; }
	void set_floor_snap_length(real_t p_floor_snap_length);
	real_t get_wall_min_slide_angle() const { return character.wall_min_slide_angle; }
	void set_wall_min_slide_angle(real_t p_radians) { character.wall_min_slide_angle = p_radians; }
	uint32_t get_platform_floor_layers() const { return character.platform_floor_layers; }
	void set_platform_floor_layers(uint32_t p_exclude_layer) { character.platform_floor_layers = p_exclude_layer; }
	uint32_t get_platform_wall_layers() const { return character.platform_wall_layers; }
	void set_platform_wall_layers(uint32_t p_exclude_layer) { character.platform_wall_layers = p_exclude_layer; }
	void set_motion_mode(MotionMode p_mode) { character.motion_mode = p_mode; }
	MotionMode get_motion_mode() const { return character.motion_mode; }
	void set_platform_on_leave(PlatformOnLeave p_on_leave) { character.platform_on_leave = p_on_leave; }
	PlatformOnLeave get_platform_on_leave() const { return character.platform_on_leave; }

	// VehicleRig3D API.
	VehicleRig3D &get_vehicle_rig() { return vehicle; }
	const VehicleRig3D &get_vehicle_rig() const { return vehicle; }
	void set_engine_force(real_t p_engine_force);
	real_t get_engine_force() const { return vehicle.engine_force; }
	void set_brake(real_t p_brake);
	real_t get_brake() const { return vehicle.brake; }
	void set_steering(real_t p_steering);
	real_t get_steering() const { return vehicle.steering; }

	// PhysicalBoneState3D API.
	PhysicalBoneState3D &get_physical_bone_state() { return bone_state; }
	const PhysicalBoneState3D &get_physical_bone_state() const { return bone_state; }
	PhysicalBoneSimulator3D *get_simulator() const;
	int get_bone_id() const { return bone_state.bone_id; }
	RID get_joint_rid() const { return bone_state.joint; }
	void set_bone_joint_type(BoneJointType p_joint_type) { bone_state.joint_type = p_joint_type; }
	BoneJointType get_bone_joint_type() const { return bone_state.joint_type; }
	const PhysicalBoneState3D::JointParams &get_bone_joint_params() const { return bone_state.joint_params; }
	PhysicalBoneState3D::JointParams &get_bone_joint_params() { return bone_state.joint_params; }
	void set_joint_offset(const Transform3D &p_offset) { bone_state.joint_offset = p_offset; }
	const Transform3D &get_joint_offset() const { return bone_state.joint_offset; }
	Vector3 get_joint_rotation() const { return bone_state.joint_offset.basis.get_euler_normalized(); }
	void set_body_offset(const Transform3D &p_offset);
	const Transform3D &get_body_offset() const { return bone_state.body_offset; }
	void set_simulate_physics(bool p_simulate);
	bool get_simulate_physics() const { return bone_state.simulate_physics; }
	bool is_simulating_physics() const { return bone_state.internal_simulate_physics; }
	void set_bone_name(const String &p_name) { bone_state.bone_name = p_name; }
	const String &get_bone_name() const { return bone_state.bone_name; }
	void set_friction(real_t p_friction) { bone_state.friction = p_friction; }
	real_t get_friction() const { return bone_state.friction; }
	void set_bounce(real_t p_bounce) { bone_state.bounce = p_bounce; }
	real_t get_bounce() const { return bone_state.bounce; }
	void reset_physics_simulation_state();

#ifdef TOOLS_ENABLED
	void _set_gizmo_move_joint(bool p_move_joint) { bone_state.gizmo_move_joint = p_move_joint; }
	virtual Transform3D get_global_gizmo_transform() const override;
	virtual Transform3D get_local_gizmo_transform() const override;
#endif

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/physics_body_3d.h ---

// --- Folded from scene/3d/physics/physical_bone_simulator_3d.h ---

class PhysicsBody3D;

class PhysicalBoneSimulator3D : public SkeletonModifier3D {
	bool simulating = false;

	struct SimulatedBone {
		int parent = -1;
		Vector<int> child_bones;
		Transform3D global_pose;
		PhysicsBody3D *physical_bone = nullptr;
		PhysicsBody3D *cache_parent_physical_bone = nullptr;
	};

	LocalVector<SimulatedBone> bones;

	PhysicsBody3D *_get_physical_bone_parent(int p_bone);
	void _reset_physical_bones_state();

protected:
	virtual void _set_active(bool p_active) override;
	void _bone_pose_updated(Skeleton3D *skeleton, int p_bone_id);
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;

public:
#ifndef DISABLE_DEPRECATED
	bool is_compat = false;
#endif // _DISABLE_DEPRECATED
	bool is_simulating_physics() const;
	int get_bone_count() const;
	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D &p_pose);
	PhysicsBody3D *get_physical_bone(int p_bone);
	PhysicsBody3D *get_physical_bone_parent(int p_bone);
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String> &p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
	PhysicalBoneSimulator3D() = default;
};

// --- End scene/3d/physics/physical_bone_simulator_3d.h ---

// --- Folded from scene/3d/physics/area_3d.h ---
#include "core/templates/vector.h"


class Area3D : public Node3D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState3D::DisableMode;

private:
	CollisionState3D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector3 gravity_vec = Vector3(0, -1, 0);
	real_t gravity = 9.8f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t angular_damp = 0.1f;
	real_t linear_damp = 0.1f;

	int priority = 0;

	real_t wind_force_magnitude = 0.0f;
	real_t wind_attenuation_factor = 0.0f;
	NodePath wind_source_path;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	bool use_reverb_bus = false;
	String reverb_bus;
	float reverb_amount = 0.0f;
	float reverb_uniformity = 0.0f;

	void _clear_monitoring();
	void _initialize_wind();

protected:
	void _notification(int p_what);

public:
	Area3D();
	~Area3D();

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector3 &p_center);
	const Vector3 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector3 &p_direction);
	const Vector3 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_wind_force_magnitude(real_t p_wind_force_magnitude);
	real_t get_wind_force_magnitude() const;

	void set_wind_attenuation_factor(real_t p_wind_attenuation_factor);
	real_t get_wind_attenuation_factor() const;

	void set_wind_source_path(const NodePath &p_wind_source_path);
	const NodePath &get_wind_source_path() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	bool has_overlapping_bodies() const;
	bool has_overlapping_areas() const;

	bool overlaps_area(Node *rp_area) const;
	bool overlaps_body(Node *rp_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const String &p_audio_bus);
	String get_audio_bus_name() const;

	void set_use_reverb_bus(bool p_enable);
	bool is_using_reverb_bus() const;

	void set_reverb_bus_name(const String &p_audio_bus);
	String get_reverb_bus_name() const;

	void set_reverb_amount(float p_amount);
	float get_reverb_amount() const;

	void set_reverb_uniformity(float p_uniformity);
	float get_reverb_uniformity() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/area_3d.h ---

// --- Folded from scene/3d/physics/collision_shape_3d.h ---

#include "scene/resources/mesh.h"

class CollisionShape3D : public Node3D {
	Ref<Shape3D> shape;

	uint32_t owner_id = 0;
	CollisionState3D *collision_state = nullptr;

	Color debug_color;
	bool debug_fill = true;
	bool disabled = false;

	Color _get_default_debug_color() const;

#ifdef DEBUG_ENABLED
	void _shape_changed();
#endif // DEBUG_ENABLED

#ifndef DISABLE_DEPRECATED
	void resource_changed(Ref<Resource> res);
#endif

protected:
	void _update_in_shape_owner(bool p_xform_only = false);
	void _notification(int p_what);

public:
	void make_convex_from_siblings();

	void set_shape(const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	void set_debug_fill_enabled(bool p_enable);
	bool get_debug_fill_enabled() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionShape3D();
	~CollisionShape3D();
};

// --- End scene/3d/physics/collision_shape_3d.h ---

// --- Folded from scene/3d/bone_attachment_3d.h ---
class BoneAttachment3D : public Node3D
{
	bool bound = false;
	String bone_name;
	int bone_idx = -1;

	bool override_pose = false;
	bool _override_dirty = false;
	bool overriding = false;

	bool use_external_skeleton = false;
	NodePath external_skeleton_node;

	void _check_bind();
	void _check_unbind();

	bool updating = false;
	void _transform_changed();
	void _update_external_skeleton_cache();

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	virtual void _on_bone_pose_update_bind_compat_90575(int p_bone_index);
	static void _bind_compatibility_methods();
#endif

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton();

	void set_bone_name(const String& p_name);
	String get_bone_name() const;

	void set_bone_idx(const int& p_idx);
	int get_bone_idx() const;

	void set_override_pose(bool p_override_pose);
	bool get_override_pose() const;

	void set_use_external_skeleton(bool p_use_external_skeleton);
	bool get_use_external_skeleton() const;
	void set_external_skeleton(NodePath p_external_skeleton);
	NodePath get_external_skeleton() const;

	virtual void on_skeleton_update();

#ifdef TOOLS_ENABLED
	virtual void notify_rebind_required();
#endif

	BoneAttachment3D() = default;
};

// --- Folded from scene/3d/importer_mesh_instance_3d.h ---



class ImporterMesh;

class ImporterMeshInstance3D : public Node3D
{
	Ref<ImporterMesh> mesh;
	Ref<Skin> skin;
	NodePath skeleton_path;
	Vector<Ref<Material>> surface_materials;
	uint32_t layer_mask = 1;
	RenderInstance3D::ShadowCastingSetting shadow_casting_setting =
		RenderInstance3D::SHADOW_CASTING_SETTING_ON;
	float visibility_range_begin = 0.0;
	float visibility_range_end = 0.0;
	float visibility_range_begin_margin = 0.0;
	float visibility_range_end_margin = 0.0;
	RenderInstance3D::VisibilityRangeFadeMode visibility_range_fade_mode =
		RenderInstance3D::VISIBILITY_RANGE_FADE_DISABLED;

public:
	void set_mesh(const Ref<ImporterMesh>& p_mesh);
	Ref<ImporterMesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_surface_material(int p_idx, const Ref<Material>& p_material);
	Ref<Material> get_surface_material(int p_idx) const;

	void set_skeleton_path(const NodePath& p_path);
	NodePath get_skeleton_path() const;

	void set_layer_mask(const uint32_t p_layer_mask);
	uint32_t get_layer_mask() const;

	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_shadow_casting_setting);
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const;

	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;

	RenderInstance3D::VisibilityRangeFadeMode get_visibility_range_fade_mode() const;
};

// --- Folded from scene/3d/mesh_instance_3d.h ---
#include "core/templates/local_vector.h"


#include "scene/resources/environment.h"
#include "scene/resources/mesh.h"

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED
class Skin;
class SkinReference;

class MeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

protected:
	Ref<Mesh> mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path;

	LocalVector<float> blend_shape_tracks;
	HashMap<String, int> blend_shape_properties;
	Vector<Ref<Material>> surface_override_materials;

	void _resolve_skeleton_path();

	bool surface_index_0 = false;

	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	bool _property_can_revert(const String& p_name) const;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_layer_mask(uint32_t p_mask) { render_instance.set_layer_mask(p_mask); }
	uint32_t get_layer_mask() const { return render_instance.get_layer_mask(); }
	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_setting)
	{
		render_instance.set_cast_shadows_setting(p_setting);
	}
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const
	{
		return render_instance.get_cast_shadows_setting();
	}
	void set_material_override(const Ref<Material>& p_material)
	{
		render_instance.set_material_override(p_material);
	}
	Ref<Material> get_material_override() const { return render_instance.get_material_override(); }
	void set_material_overlay(const Ref<Material>& p_material)
	{
		render_instance.set_material_overlay(p_material);
	}
	Ref<Material> get_material_overlay() const { return render_instance.get_material_overlay(); }
	void set_gi_mode(RenderInstance3D::GIMode p_mode) { render_instance.set_gi_mode(p_mode); }
	RenderInstance3D::GIMode get_gi_mode() const { return render_instance.get_gi_mode(); }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_skeleton_path(const NodePath& p_skeleton);
	NodePath get_skeleton_path();

	Ref<SkinReference> get_skin_reference() const;

	int get_blend_shape_count() const;
	int find_blend_shape_by_name(const String& p_name);
	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	int get_surface_override_material_count() const;
	void set_surface_override_material(int p_surface, const Ref<Material>& p_material);
	Ref<Material> get_surface_override_material(int p_surface) const;
	Ref<Material> get_active_material(int p_surface) const;

#ifndef PHYSICS_3D_DISABLED
	Node* create_trimesh_collision_node();
	void create_trimesh_collision();

	Node* create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
	void create_convex_collision(bool p_clean = true, bool p_simplify = false);

	Node* create_multiple_convex_collisions_node(
		const Ref<MeshConvexDecompositionSettings>& p_settings =
			Ref<MeshConvexDecompositionSettings>());
	void create_multiple_convex_collisions(const Ref<MeshConvexDecompositionSettings>& p_settings =
											   Ref<MeshConvexDecompositionSettings>());
#endif // PHYSICS_3D_DISABLED

	MeshInstance3D* create_debug_tangents_node();
	void create_debug_tangents();

	AABB get_aabb() const;

	Ref<ArrayMesh> bake_mesh_from_current_blend_shape_mix(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());
	Ref<ArrayMesh> bake_mesh_from_current_skeleton_pose(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());

	Ref<TriangleMesh> generate_triangle_mesh() const;

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	virtual PackedStringArray get_configuration_warnings() const override;

	MeshInstance3D();
};
// --- End scene/3d/mesh_instance_3d.h ---
// --- End scene/3d/skeleton_3d.h ---
