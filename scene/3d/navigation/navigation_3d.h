/**************************************************************************/
/*  navigation_3d.h                                                       */
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

#include "scene/3d/node_3d.h"
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
