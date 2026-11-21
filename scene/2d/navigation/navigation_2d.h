/**************************************************************************/
/*  navigation_2d.h                                                       */
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

#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"

class NavigationMeshSourceGeometryData2D;

class NavigationAgent2D final : public Node
{
	Node2D* agent_parent = nullptr;

	RID agent;
	RID map_override;

	bool avoidance_enabled = false;
	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;
	uint32_t navigation_layers = 1;
	NavigationPathQueryParameters2D::PathfindingAlgorithm pathfinding_algorithm =
		NavigationPathQueryParameters2D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
	NavigationPathQueryParameters2D::PathPostProcessing path_postprocessing =
		NavigationPathQueryParameters2D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
	uint32_t path_metadata_flags =
		NavigationPathQueryParameters2D::PathMetadataFlags::PATH_METADATA_INCLUDE_ALL;

	real_t path_desired_distance = 20.0;
	real_t target_desired_distance = 10.0;
	real_t radius = NavigationDefaults2D::AVOIDANCE_AGENT_RADIUS;
	real_t neighbor_distance = NavigationDefaults2D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	int max_neighbors = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t time_horizon_agents = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	real_t max_speed = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t path_max_distance = 100.0;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;
	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

	Vector2 target_position;

	Ref<NavigationPathQueryParameters2D> navigation_query;
	Ref<NavigationPathQueryResult2D> navigation_result;
	int navigation_path_index = 0;

	Vector2 safe_velocity;

	Vector2 velocity;
	bool velocity_submitted = false;

	Vector2 velocity_forced;
	bool velocity_forced_submitted = false;

	bool target_position_submitted = false;

	bool target_reached = false;
	bool navigation_finished = true;
	bool last_waypoint_reached = false;

	bool debug_enabled = false;
	float debug_path_custom_point_size = 4.0;
	float debug_path_custom_line_width = -1.0;
	bool debug_use_custom = false;
	Color debug_path_custom_color = Color(1.0, 1.0, 1.0, 1.0);

#ifdef DEBUG_ENABLED
	bool debug_path_dirty = true;
	RID debug_path_instance;
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationAgent2D();
	~NavigationAgent2D();

	RID get_rid() const { return agent; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_agent_parent(Node* p_agent_parent);

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_pathfinding_algorithm(
		const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm);

	NavigationPathQueryParameters2D::PathfindingAlgorithm get_pathfinding_algorithm() const
	{
		return pathfinding_algorithm;
	}

	void set_path_postprocessing(
		const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing);

	NavigationPathQueryParameters2D::PathPostProcessing get_path_postprocessing() const
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

	void set_target_position(Vector2 p_position);
	Vector2 get_target_position() const;

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

	Vector2 get_next_path_position();

	Ref<NavigationPathQueryResult2D> get_current_navigation_result() const
	{
		return navigation_result;
	}

	const Vector<Vector2>& get_current_navigation_path() const
	{
		return navigation_result->get_path();
	}

	int get_current_navigation_path_index() const { return navigation_path_index; }

	real_t distance_to_target() const;
	bool is_target_reached() const;
	bool is_target_reachable();
	bool is_navigation_finished();
	Vector2 get_final_position();

	void set_velocity(const Vector2 p_velocity);

	Vector2 get_velocity() { return velocity; }

	void set_velocity_forced(const Vector2 p_velocity);

	void _avoidance_done(Vector2 p_new_velocity);

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

	void set_debug_path_custom_line_width(float p_line_width);
	float get_debug_path_custom_line_width() const;

private:
	bool _is_target_reachable() const;
	Vector2 _get_final_position() const;

	void _update_navigation();
	void _advance_waypoints(const Vector2& p_origin);
	void _request_repath();

	bool _is_last_waypoint() const;
	void _move_to_next_waypoint();
	bool _is_within_waypoint_distance(const Vector2& p_origin) const;
	bool _is_within_target_distance(const Vector2& p_origin) const;

	void _trigger_waypoint_reached();
	void _transition_to_navigation_finished();
	void _transition_to_target_reached();

#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_debug_path();
#endif // DEBUG_ENABLED
};

class NavigationLink2D final : public Node2D
{
	bool enabled = true;
	RID link;
	RID map_override;
	bool bidirectional = true;
	uint32_t navigation_layers = 1;
	Vector2 end_position;
	Vector2 start_position;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

#ifdef DEBUG_ENABLED
	void _update_debug_mesh();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const override;
	bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED
	RID get_rid() const;

	void set_enabled(bool p_enabled);

	bool is_enabled() const { return enabled; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_bidirectional(bool p_bidirectional);

	bool is_bidirectional() const { return bidirectional; }

	void set_navigation_layers(uint32_t p_navigation_layers);

	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_start_position(Vector2 p_position);

	Vector2 get_start_position() const { return start_position; }

	void set_end_position(Vector2 p_position);

	Vector2 get_end_position() const { return end_position; }

	void set_global_start_position(Vector2 p_position);
	Vector2 get_global_start_position() const;

	void set_global_end_position(Vector2 p_position);
	Vector2 get_global_end_position() const;

	void set_enter_cost(real_t p_enter_cost);

	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);

	real_t get_travel_cost() const { return travel_cost; }

	PackedStringArray get_configuration_warnings() const override;

	NavigationLink2D();
	~NavigationLink2D();

private:
	void _link_enter_navigation_map();
	void _link_exit_navigation_map();
	void _link_update_transform();
};

class NavigationObstacle2D final : public Node2D
{
	RID obstacle;
	RID map_before_pause;
	RID map_override;
	RID map_current;

	real_t radius = 0.0;

	Vector<Vector2> vertices;
	bool vertices_are_clockwise = true;
	bool vertices_are_valid = true;

	bool avoidance_enabled = true;
	uint32_t avoidance_layers = 1;

	Transform2D previous_transform;

	Vector2 velocity;
	Vector2 previous_velocity;
	bool velocity_submitted = false;

	bool affect_navigation_mesh = false;
	bool carve_navigation_mesh = false;

#ifdef DEBUG_ENABLED
private:
	RID debug_canvas_item;
	RID debug_mesh_rid;

	void _update_fake_agent_radius_debug();
	void _update_static_obstacle_debug();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationObstacle2D();
	~NavigationObstacle2D();

	RID get_rid() const { return obstacle; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_vertices(const Vector<Vector2>& p_vertices);

	const Vector<Vector2>& get_vertices() const { return vertices; }

	bool are_vertices_clockwise() const { return vertices_are_clockwise; }

	bool are_vertices_valid() const { return vertices_are_valid; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_velocity(const Vector2 p_velocity);

	Vector2 get_velocity() const { return velocity; }

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
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);

private:
	void _update_map(RID p_map);
	void _update_position(const Vector2 p_position);
	void _update_transform();
};

class NavigationRegion2D final : public Node2D
{
	bool enabled = true;
	bool use_edge_connections = true;

	RID region;
	RID map_override;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	Ref<NavigationPolygon> navigation_polygon;

	void _navigation_polygon_changed();

	Rect2 bounds;

#ifdef DEBUG_ENABLED
private:
	RID debug_mesh_rid;
	RID debug_instance_rid;

	bool debug_mesh_dirty = true;

	void _set_debug_visible(bool p_visible);
	void _update_debug_mesh();
	void _update_debug_edge_connections_mesh();
	void _update_debug_baking_rect();
	void _navigation_map_changed(RID p_map);
	void _navigation_debug_changed();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const override;
	bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED
	RID get_rid() const;

	void set_enabled(bool p_enabled);
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

	void set_navigation_polygon(const Ref<NavigationPolygon>& p_navigation_polygon);
	Ref<NavigationPolygon> get_navigation_polygon() const;

	PackedStringArray get_configuration_warnings() const override;

	void bake_navigation_polygon(bool p_on_thread);
	void _bake_finished();
	bool is_baking() const;

	Rect2 get_bounds() const { return bounds; }

	NavigationRegion2D();
	~NavigationRegion2D();

private:
	void _update_bounds();
	void _region_enter_navigation_map();
	void _region_exit_navigation_map();
	void _region_update_transform();
};
