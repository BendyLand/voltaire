/**************************************************************************/
/*  navigation_server_2d.cpp                                              */
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

#include "navigation_server_2d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryParameters2D

void NavigationPathQueryParameters2D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	pathfinding_algorithm = p_pathfinding_algorithm;
}

NavigationPathQueryParameters2D::PathfindingAlgorithm
NavigationPathQueryParameters2D::get_pathfinding_algorithm() const
{
	return pathfinding_algorithm;
}

void NavigationPathQueryParameters2D::set_path_postprocessing(
	const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing)
{
	path_postprocessing = p_path_postprocessing;
}

NavigationPathQueryParameters2D::PathPostProcessing
NavigationPathQueryParameters2D::get_path_postprocessing() const
{
	return path_postprocessing;
}

void NavigationPathQueryParameters2D::set_map(RID p_map) { map = p_map; }

RID NavigationPathQueryParameters2D::get_map() const { return map; }

void NavigationPathQueryParameters2D::set_start_position(Vector2 p_start_position)
{
	start_position = p_start_position;
}

Vector2 NavigationPathQueryParameters2D::get_start_position() const { return start_position; }

void NavigationPathQueryParameters2D::set_target_position(Vector2 p_target_position)
{
	target_position = p_target_position;
}

Vector2 NavigationPathQueryParameters2D::get_target_position() const { return target_position; }

void NavigationPathQueryParameters2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	navigation_layers = p_navigation_layers;
}

uint32_t NavigationPathQueryParameters2D::get_navigation_layers() const
{
	return navigation_layers;
}

void NavigationPathQueryParameters2D::set_metadata_flags(uint32_t p_flags)
{
	metadata_flags = (int64_t)p_flags;
}

uint32_t NavigationPathQueryParameters2D::get_metadata_flags() const
{
	return (int64_t)metadata_flags;
}

void NavigationPathQueryParameters2D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
}

bool NavigationPathQueryParameters2D::get_simplify_path() const { return simplify_path; }

void NavigationPathQueryParameters2D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
}

real_t NavigationPathQueryParameters2D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationPathQueryParameters2D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
}

float NavigationPathQueryParameters2D::get_path_return_max_length() const
{
	return path_return_max_length;
}

void NavigationPathQueryParameters2D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
}

float NavigationPathQueryParameters2D::get_path_return_max_radius() const
{
	return path_return_max_radius;
}

void NavigationPathQueryParameters2D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
}

int NavigationPathQueryParameters2D::get_path_search_max_polygons() const
{
	return path_search_max_polygons;
}

void NavigationPathQueryParameters2D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
}

float NavigationPathQueryParameters2D::get_path_search_max_distance() const
{
	return path_search_max_distance;
}

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryResult2D

void NavigationPathQueryResult2D::set_path(const Vector<Vector2>& p_path) { path = p_path; }

const Vector<Vector2>& NavigationPathQueryResult2D::get_path() const { return path; }

void NavigationPathQueryResult2D::set_path_types(const Vector<int32_t>& p_path_types)
{
	path_types = p_path_types;
}

const Vector<int32_t>& NavigationPathQueryResult2D::get_path_types() const { return path_types; }

void NavigationPathQueryResult2D::set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids)
{
	path_owner_ids = p_path_owner_ids;
}

const Vector<int64_t>& NavigationPathQueryResult2D::get_path_owner_ids() const
{
	return path_owner_ids;
}

void NavigationPathQueryResult2D::set_path_length(float p_length) { path_length = p_length; }

float NavigationPathQueryResult2D::get_path_length() const { return path_length; }

void NavigationPathQueryResult2D::reset() {}

////////////////////////////////////////////////////////////////////////////
// NavigationServer2D

bool NavigationServer2D::get_debug_enabled() { return data->debug_enabled; }

#ifdef DEBUG_ENABLED

bool NavigationServer2D::get_debug_navigation_enabled() { return data->debug_navigation_enabled; }

bool NavigationServer2D::get_debug_avoidance_enabled() { return data->debug_avoidance_enabled; }

void NavigationServer2D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	data->debug_navigation_link_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_color()
{
	return data->debug_navigation_link_connection_color;
}

void NavigationServer2D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	data->debug_navigation_link_connection_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_disabled_color()
{
	return data->debug_navigation_link_connection_disabled_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_lines(const bool p_value)
{
	data->debug_navigation_enable_edge_lines = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_lines()
{
	return data->debug_navigation_enable_edge_lines;
}

void NavigationServer2D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	data->debug_navigation_agent_path_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_agent_path_color()
{
	return data->debug_navigation_agent_path_color;
}

void NavigationServer2D::set_debug_navigation_enable_agent_paths(const bool p_value)
{
	data->debug_navigation_enable_agent_paths = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_agent_paths()
{
	return data->debug_navigation_enable_agent_paths;
}

void NavigationServer2D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	data->debug_navigation_agent_path_point_size = p_point_size;
}

real_t NavigationServer2D::get_debug_navigation_agent_path_point_size()
{
	return data->debug_navigation_agent_path_point_size;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_agents_radius(const bool p_value)
{
	data->debug_navigation_avoidance_enable_agents_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_agents_radius()
{
	return data->debug_navigation_avoidance_enable_agents_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_radius(const bool p_value)
{
	data->debug_navigation_avoidance_enable_obstacles_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_radius()
{
	return data->debug_navigation_avoidance_enable_obstacles_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_agents_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_agents_radius_color()
{
	return data->debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_obstacles_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_obstacles_radius_color()
{
	return data->debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_static(const bool p_value)
{
	data->debug_navigation_avoidance_enable_obstacles_static = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_static()
{
	return data->debug_navigation_avoidance_enable_obstacles_static;
}

void NavigationServer2D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_color()
{
	if (!data) {
		return Color(0.0, 1.0, 1.0, 0.4);
	}
	return data->debug_navigation_geometry_face_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_color()
{
	if (!data) {
		return Color(0.0, 1.0, 1.0, 0.8);
	}
	return data->debug_navigation_geometry_edge_color;
}

void NavigationServer2D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_face_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_disabled_color()
{
	if (!data) {
		return Color(1.0, 0.0, 0.0, 0.4);
	}
	return data->debug_navigation_geometry_face_disabled_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_edge_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_disabled_color()
{
	if (!data) {
		return Color(1.0, 0.0, 0.0, 0.8);
	}
	return data->debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer2D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_edge_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_edge_connection_color()
{
	if (!data) {
		return Color(1.0, 0.0, 1.0, 0.4);
	}
	return data->debug_navigation_edge_connection_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_connections(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_edge_connections = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_connections()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_edge_connections;
}

void NavigationServer2D::set_debug_navigation_enable_geometry_face_random_color(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_geometry_face_random_color = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_geometry_face_random_color()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_geometry_face_random_color;
}

void NavigationServer2D::set_debug_navigation_enable_agent_radius(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_agent_radius = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_agent_radius()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_agent_radius;
}

void NavigationServer2D::set_debug_navigation_enable_obstacles(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_obstacles = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_obstacles()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_obstacles;
}

void NavigationServer2D::set_debug_navigation_enable_link_connections(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_link_connections = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_link_connections()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_link_connections;
}
#endif // DEBUG_ENABLED

struct NavigationServer2D::NavMap
{
};
struct NavigationServer2D::NavRegion
{
};
struct NavigationServer2D::NavLink
{
};
struct NavigationServer2D::NavAgent
{
};
struct NavigationServer2D::NavObstacle
{
};

void NavigationServer2D::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void NavigationServer2D::finalize()
{
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void NavigationServer2D::process(double p_delta_time) {}

void NavigationServer2D::free_rid(RID p_rid) {}

int NavigationServer2D::get_process_info(ProcessInfo p_info) { return 0; }

RID NavigationServer2D::map_create() { return RID(); }

void NavigationServer2D::map_set_active(RID p_map, bool p_active) {}

void NavigationServer2D::map_set_cell_size(RID p_map, real_t p_cell_size) {}

real_t NavigationServer2D::map_get_link_connection_radius(RID p_map) { return 0.0; }

bool NavigationServer2D::map_get_use_edge_connections(RID p_map) { return false; }

real_t NavigationServer2D::map_get_edge_connection_margin(RID p_map) { return 0.0; }

void NavigationServer2D::region_set_map(RID p_region, RID p_map) {}

void NavigationServer2D::region_set_transform(RID p_region, Transform2D p_transform) {}

void NavigationServer2D::region_set_use_edge_connections(RID p_region, bool p_enabled) {}

void NavigationServer2D::region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers) {}

void NavigationServer2D::region_set_enter_cost(RID p_region, real_t p_enter_cost) {}

void NavigationServer2D::region_set_travel_cost(RID p_region, real_t p_travel_cost) {}

int NavigationServer2D::region_get_connections_count(RID p_region) { return 0; }

Vector2 NavigationServer2D::region_get_connection_pathway_start(RID p_region, int p_connection_id)
{
	return Vector2();
}

Vector2 NavigationServer2D::region_get_connection_pathway_end(RID p_region, int p_connection_id)
{
	return Vector2();
}

RID NavigationServer2D::link_create() { return RID(); }

void NavigationServer2D::link_set_map(RID p_link, RID p_map) {}

void NavigationServer2D::link_set_enabled(RID p_link, bool p_enabled) {}

void NavigationServer2D::link_set_bidirectional(RID p_link, bool p_bidirectional) {}

void NavigationServer2D::link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers) {}

void NavigationServer2D::link_set_enter_cost(RID p_link, real_t p_enter_cost) {}

void NavigationServer2D::link_set_travel_cost(RID p_link, real_t p_travel_cost) {}

RID NavigationServer2D::obstacle_create() { return RID(); }

void NavigationServer2D::obstacle_set_map(RID p_obstacle, RID p_map) {}

void NavigationServer2D::obstacle_set_radius(RID p_obstacle, real_t p_radius) {}

void NavigationServer2D::obstacle_set_velocity(RID p_obstacle, Vector2 p_velocity) {}

void NavigationServer2D::obstacle_set_vertices(RID p_obstacle, const Vector<Vector2>& p_vertices) {}

void NavigationServer2D::obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers) {}

void NavigationServer2D::obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled) {}

void NavigationServer2D::obstacle_set_paused(RID p_obstacle, bool p_paused) {}

void NavigationServer2D::agent_set_map(RID p_agent, RID p_map) {}

void NavigationServer2D::agent_set_position(RID p_agent, Vector2 p_position) {}

void NavigationServer2D::agent_set_velocity(RID p_agent, Vector2 p_velocity) {}

void NavigationServer2D::agent_set_velocity_forced(RID p_agent, Vector2 p_velocity) {}

void NavigationServer2D::agent_set_radius(RID p_agent, real_t p_radius) {}

void NavigationServer2D::agent_set_neighbor_distance(RID p_agent, real_t p_distance) {}

void NavigationServer2D::agent_set_max_neighbors(RID p_agent, int p_count) {}

void NavigationServer2D::agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon) {}

void NavigationServer2D::agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon) {}

void NavigationServer2D::agent_set_max_speed(RID p_agent, real_t p_max_speed) {}

void NavigationServer2D::agent_set_avoidance_layers(RID p_agent, uint32_t p_layers) {}

void NavigationServer2D::agent_set_avoidance_mask(RID p_agent, uint32_t p_mask) {}

void NavigationServer2D::agent_set_avoidance_priority(RID p_agent, real_t p_priority) {}

void NavigationServer2D::agent_set_paused(RID p_agent, bool p_paused) {}
