/**************************************************************************/
/*  navigation_server_3d.h                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/math/aabb.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/os/thread_safe.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_path_query_parameters_3d.h"
#include "servers/navigation_3d/navigation_path_query_result_3d.h"

class NavigationServer3D final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	enum ProcessInfo
	{
		INFO_ACTIVE_MAPS,
		INFO_REGION_COUNT,
		INFO_AGENT_COUNT,
		INFO_LINK_COUNT,
		INFO_POLYGON_COUNT,
		INFO_EDGE_COUNT,
		INFO_EDGE_MERGE_COUNT,
		INFO_EDGE_CONNECTION_COUNT,
		INFO_EDGE_FREE_COUNT,
		INFO_OBSTACLE_COUNT,
	};

private:
	struct NavMap;
	struct NavRegion;
	struct NavLink;
	struct NavAgent;
	struct NavObstacle;

	struct Data
	{
		bool active = true;
		bool debug_enabled = false;

		LocalVector<NavMap*> active_maps;
		LocalVector<NavMap*> iteration_maps;

		RID_Owner<NavMap> map_owner;
		RID_Owner<NavRegion> region_owner;
		RID_Owner<NavLink> link_owner;
		RID_Owner<NavAgent> agent_owner;
		RID_Owner<NavObstacle> obstacle_owner;

#ifdef DEBUG_ENABLED
		bool debug_dirty = true;
		bool debug_navigation_enabled = false;
		bool navigation_debug_dirty = true;
		bool debug_avoidance_enabled = false;
		bool avoidance_debug_dirty = true;

		Color debug_navigation_edge_connection_color = Color(1.0, 0.0, 1.0, 1.0);
		Color debug_navigation_geometry_edge_color = Color(0.5, 1.0, 1.0, 1.0);
		Color debug_navigation_geometry_face_color = Color(0.5, 1.0, 1.0, 0.4);
		Color debug_navigation_geometry_edge_disabled_color = Color(0.5, 0.5, 0.5, 1.0);
		Color debug_navigation_geometry_face_disabled_color = Color(0.5, 0.5, 0.5, 0.4);
		Color debug_navigation_link_connection_color = Color(1.0, 0.5, 1.0, 1.0);
		Color debug_navigation_link_connection_disabled_color = Color(0.5, 0.5, 0.5, 1.0);
		Color debug_navigation_agent_path_color = Color(1.0, 0.0, 0.0, 1.0);

		real_t debug_navigation_agent_path_point_size = 4.0;

		Color debug_navigation_avoidance_agents_radius_color = Color(1.0, 1.0, 0.0, 0.25);
		Color debug_navigation_avoidance_obstacles_radius_color = Color(1.0, 0.5, 0.0, 0.25);

		Color debug_navigation_avoidance_static_obstacle_pushin_face_color =
			Color(1.0, 0.0, 0.0, 0.0);
		Color debug_navigation_avoidance_static_obstacle_pushout_face_color =
			Color(1.0, 1.0, 0.0, 0.5);
		Color debug_navigation_avoidance_static_obstacle_pushin_edge_color =
			Color(1.0, 0.0, 0.0, 1.0);
		Color debug_navigation_avoidance_static_obstacle_pushout_edge_color =
			Color(1.0, 1.0, 0.0, 1.0);

		bool debug_navigation_enable_edge_connections = true;
		bool debug_navigation_enable_edge_connections_xray = true;
		bool debug_navigation_enable_edge_lines = true;
		bool debug_navigation_enable_edge_lines_xray = true;
		bool debug_navigation_enable_geometry_face_random_color = true;
		bool debug_navigation_enable_link_connections = true;
		bool debug_navigation_enable_link_connections_xray = true;
		bool debug_navigation_enable_agent_paths = true;
		bool debug_navigation_enable_agent_paths_xray = true;

		bool debug_navigation_avoidance_enable_agents_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_static = true;

		Ref<StandardMaterial3D> debug_navigation_geometry_edge_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_face_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_edge_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_face_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_edge_connections_material;
		Ref<StandardMaterial3D> debug_navigation_link_connections_material;
		Ref<StandardMaterial3D> debug_navigation_link_connections_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_agents_radius_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_obstacles_radius_material;

		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushin_face_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushout_face_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushin_edge_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushout_edge_material;

		Ref<StandardMaterial3D> debug_navigation_agent_path_line_material;
		Ref<StandardMaterial3D> debug_navigation_agent_path_point_material;
#endif // DEBUG_ENABLED
	};

public:
	static inline Data* data = nullptr;

	/* SERVER LIFECYCLE */

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	static void set_active(bool p_active);
	static void sync();
	static void process(double p_delta_time);
	static void physics_process(double p_delta_time);
	static void free_rid(RID p_rid);

	static int get_process_info(ProcessInfo p_info);

	/* MAP API */

	static RID map_create();
	static void map_set_active(RID p_map, bool p_active);
	static bool map_is_active(RID p_map);

	static void map_set_up(RID p_map, Vector3 p_up);
	static Vector3 map_get_up(RID p_map);

	static void map_set_cell_size(RID p_map, real_t p_cell_size);
	static real_t map_get_cell_size(RID p_map);

	static void map_set_cell_height(RID p_map, real_t p_height);
	static real_t map_get_cell_height(RID p_map);

	static void map_set_merge_rasterizer_cell_scale(RID p_map, float p_value);
	static float map_get_merge_rasterizer_cell_scale(RID p_map);

	static void map_set_use_edge_connections(RID p_map, bool p_enabled);
	static bool map_get_use_edge_connections(RID p_map);

	static void map_set_edge_connection_margin(RID p_map, real_t p_connection_margin);
	static real_t map_get_edge_connection_margin(RID p_map);

	static void map_set_link_connection_radius(RID p_map, real_t p_connection_radius);
	static real_t map_get_link_connection_radius(RID p_map);

	static Vector<Vector3> map_get_path(RID p_map, Vector3 p_origin, Vector3 p_destination,
		bool p_optimize, uint32_t p_navigation_layers = 1);

	static Vector3 map_get_closest_point_to_segment(
		RID p_map, const Vector3& p_from, const Vector3& p_to, const bool p_use_collision = false);
	static Vector3 map_get_closest_point(RID p_map, const Vector3& p_point);
	static Vector3 map_get_closest_point_normal(RID p_map, const Vector3& p_point);
	static RID map_get_closest_point_owner(RID p_map, const Vector3& p_point);

	static void map_force_update(RID p_map);
	static uint32_t map_get_iteration_id(RID p_map);

	static void map_set_use_async_iterations(RID p_map, bool p_enabled);
	static bool map_get_use_async_iterations(RID p_map);

	static Vector3 map_get_random_point(RID p_map, uint32_t p_navigation_layers, bool p_uniformly);

	/* REGION API */

	static RID region_create();
	static uint32_t region_get_iteration_id(RID p_region);

	static void region_set_use_async_iterations(RID p_region, bool p_enabled);
	static bool region_get_use_async_iterations(RID p_region);

	static void region_set_enabled(RID p_region, bool p_enabled);
	static bool region_get_enabled(RID p_region);

	static void region_set_use_edge_connections(RID p_region, bool p_enabled);
	static bool region_get_use_edge_connections(RID p_region);

	static void region_set_enter_cost(RID p_region, real_t p_enter_cost);
	static real_t region_get_enter_cost(RID p_region);

	static void region_set_travel_cost(RID p_region, real_t p_travel_cost);
	static real_t region_get_travel_cost(RID p_region);

	static bool region_owns_point(RID p_region, const Vector3& p_point);

	static void region_set_map(RID p_region, RID p_map);
	static RID region_get_map(RID p_region);

	static void region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers);
	static uint32_t region_get_navigation_layers(RID p_region);

	static void region_set_transform(RID p_region, Transform3D p_transform);
	static Transform3D region_get_transform(RID p_region);

	static void region_set_navigation_mesh(RID p_region, Ref<NavigationMesh> p_navigation_mesh);

	static int region_get_connections_count(RID p_region);
	static Vector3 region_get_connection_pathway_start(RID p_region, int p_connection_id);
	static Vector3 region_get_connection_pathway_end(RID p_region, int p_connection_id);

	static Vector3 region_get_closest_point_to_segment(
		RID p_region, const Vector3& p_from, const Vector3& p_to, bool p_use_collision = false);
	static Vector3 region_get_closest_point(RID p_region, const Vector3& p_point);
	static Vector3 region_get_closest_point_normal(RID p_region, const Vector3& p_point);
	static Vector3 region_get_random_point(
		RID p_region, uint32_t p_navigation_layers, bool p_uniformly);

	static AABB region_get_bounds(RID p_region);

	/* LINK API */

	static RID link_create();
	static uint32_t link_get_iteration_id(RID p_link);

	static void link_set_map(RID p_link, RID p_map);
	static RID link_get_map(RID p_link);

	static void link_set_enabled(RID p_link, bool p_enabled);
	static bool link_get_enabled(RID p_link);

	static void link_set_bidirectional(RID p_link, bool p_bidirectional);
	static bool link_is_bidirectional(RID p_link);

	static void link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers);
	static uint32_t link_get_navigation_layers(RID p_link);

	static void link_set_start_position(RID p_link, Vector3 p_position);
	static Vector3 link_get_start_position(RID p_link);

	static void link_set_end_position(RID p_link, Vector3 p_position);
	static Vector3 link_get_end_position(RID p_link);

	static void link_set_enter_cost(RID p_link, real_t p_enter_cost);
	static real_t link_get_enter_cost(RID p_link);

	static void link_set_travel_cost(RID p_link, real_t p_travel_cost);
	static real_t link_get_travel_cost(RID p_link);

	/* AGENT API */

	static RID agent_create();

	static void agent_set_map(RID p_agent, RID p_map);
	static RID agent_get_map(RID p_agent);

	static void agent_set_paused(RID p_agent, bool p_paused);
	static bool agent_get_paused(RID p_agent);

	static void agent_set_avoidance_enabled(RID p_agent, bool p_enabled);
	static bool agent_get_avoidance_enabled(RID p_agent);

	static void agent_set_use_3d_avoidance(RID p_agent, bool p_enabled);
	static bool agent_get_use_3d_avoidance(RID p_agent);

	static void agent_set_neighbor_distance(RID p_agent, real_t p_distance);
	static real_t agent_get_neighbor_distance(RID p_agent);

	static void agent_set_max_neighbors(RID p_agent, int p_count);
	static int agent_get_max_neighbors(RID p_agent);

	static void agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon);
	static real_t agent_get_time_horizon_agents(RID p_agent);

	static void agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon);
	static real_t agent_get_time_horizon_obstacles(RID p_agent);

	static void agent_set_radius(RID p_agent, real_t p_radius);
	static real_t agent_get_radius(RID p_agent);

	static void agent_set_height(RID p_agent, real_t p_height);
	static real_t agent_get_height(RID p_agent);

	static void agent_set_max_speed(RID p_agent, real_t p_max_speed);
	static real_t agent_get_max_speed(RID p_agent);

	static void agent_set_velocity_forced(RID p_agent, Vector3 p_velocity);
	static void agent_set_velocity(RID p_agent, Vector3 p_velocity);
	static Vector3 agent_get_velocity(RID p_agent);

	static void agent_set_position(RID p_agent, Vector3 p_position);
	static Vector3 agent_get_position(RID p_agent);

	static bool agent_is_map_changed(RID p_agent);
	static bool agent_has_avoidance_callback(RID p_agent);

	static void agent_set_avoidance_layers(RID p_agent, uint32_t p_layers);
	static uint32_t agent_get_avoidance_layers(RID p_agent);

	static void agent_set_avoidance_mask(RID p_agent, uint32_t p_mask);
	static uint32_t agent_get_avoidance_mask(RID p_agent);

	static void agent_set_avoidance_priority(RID p_agent, real_t p_priority);
	static real_t agent_get_avoidance_priority(RID p_agent);

	/* OBSTACLE API */

	static RID obstacle_create();

	static void obstacle_set_map(RID p_obstacle, RID p_map);
	static RID obstacle_get_map(RID p_obstacle);

	static void obstacle_set_paused(RID p_obstacle, bool p_paused);
	static bool obstacle_get_paused(RID p_obstacle);

	static void obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_avoidance_enabled(RID p_obstacle);

	static void obstacle_set_use_3d_avoidance(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_use_3d_avoidance(RID p_obstacle);

	static void obstacle_set_radius(RID p_obstacle, real_t p_radius);
	static real_t obstacle_get_radius(RID p_obstacle);

	static void obstacle_set_height(RID p_obstacle, real_t p_height);
	static real_t obstacle_get_height(RID p_obstacle);

	static void obstacle_set_velocity(RID p_obstacle, Vector3 p_velocity);
	static Vector3 obstacle_get_velocity(RID p_obstacle);

	static void obstacle_set_position(RID p_obstacle, Vector3 p_position);
	static Vector3 obstacle_get_position(RID p_obstacle);

	static void obstacle_set_vertices(RID p_obstacle, const Vector<Vector3>& p_vertices);
	static Vector<Vector3> obstacle_get_vertices(RID p_obstacle);

	static void obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers);
	static uint32_t obstacle_get_avoidance_layers(RID p_obstacle);

	/* PATH QUERY & UTILITIES */

	static void query_path(const Ref<NavigationPathQueryParameters3D>& p_query_parameters,
		Ref<NavigationPathQueryResult3D> p_query_result);
	static Vector<Vector3> simplify_path(const Vector<Vector3>& p_path, real_t p_epsilon);

	/* DEBUG API */

	static void set_debug_enabled(bool p_enabled);
	static bool get_debug_enabled();

#ifdef DEBUG_ENABLED
	static void set_debug_navigation_enabled(bool p_enabled);
	static bool get_debug_navigation_enabled();

	static void set_debug_avoidance_enabled(bool p_enabled);
	static bool get_debug_avoidance_enabled();

	static void set_debug_navigation_edge_connection_color(const Color& p_color);
	static Color get_debug_navigation_edge_connection_color();

	static void set_debug_navigation_geometry_edge_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_color();

	static void set_debug_navigation_geometry_face_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_color();

	static void set_debug_navigation_geometry_edge_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_disabled_color();

	static void set_debug_navigation_geometry_face_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_disabled_color();

	static void set_debug_navigation_link_connection_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_color();

	static void set_debug_navigation_link_connection_disabled_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_disabled_color();

	static void set_debug_navigation_agent_path_color(const Color& p_color);
	static Color get_debug_navigation_agent_path_color();

	static void set_debug_navigation_avoidance_agents_radius_color(const Color& p_color);
	static Color get_debug_navigation_avoidance_agents_radius_color();

	static void set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color);
	static Color get_debug_navigation_avoidance_obstacles_radius_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushin_face_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushout_face_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushin_edge_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushout_edge_color();

	static void set_debug_navigation_enable_edge_connections(bool p_value);
	static bool get_debug_navigation_enable_edge_connections();

	static void set_debug_navigation_enable_edge_connections_xray(bool p_value);
	static bool get_debug_navigation_enable_edge_connections_xray();

	static void set_debug_navigation_enable_edge_lines(bool p_value);
	static bool get_debug_navigation_enable_edge_lines();

	static void set_debug_navigation_enable_edge_lines_xray(bool p_value);
	static bool get_debug_navigation_enable_edge_lines_xray();

	static void set_debug_navigation_enable_geometry_face_random_color(bool p_value);
	static bool get_debug_navigation_enable_geometry_face_random_color();

	static void set_debug_navigation_enable_link_connections(bool p_value);
	static bool get_debug_navigation_enable_link_connections();

	static void set_debug_navigation_enable_link_connections_xray(bool p_value);
	static bool get_debug_navigation_enable_link_connections_xray();

	static void set_debug_navigation_enable_agent_paths(bool p_value);
	static bool get_debug_navigation_enable_agent_paths();

	static void set_debug_navigation_enable_agent_paths_xray(bool p_value);
	static bool get_debug_navigation_enable_agent_paths_xray();

	static void set_debug_navigation_agent_path_point_size(real_t p_point_size);
	static real_t get_debug_navigation_agent_path_point_size();

	static void set_debug_navigation_avoidance_enable_agents_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_agents_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_static(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_static();

	static Ref<StandardMaterial3D> get_debug_navigation_geometry_face_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_edge_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_face_disabled_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_edge_disabled_material();
	static Ref<StandardMaterial3D> get_debug_navigation_edge_connections_material();
	static Ref<StandardMaterial3D> get_debug_navigation_link_connections_material();
	static Ref<StandardMaterial3D> get_debug_navigation_link_connections_disabled_material();

	static Ref<StandardMaterial3D> get_debug_navigation_agent_path_line_material();
	static Ref<StandardMaterial3D> get_debug_navigation_agent_path_point_material();

	static Ref<StandardMaterial3D> get_debug_navigation_avoidance_agents_radius_material();
	static Ref<StandardMaterial3D> get_debug_navigation_avoidance_obstacles_radius_material();

	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushin_face_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushout_face_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushin_edge_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushout_edge_material();
#endif // DEBUG_ENABLED

	/* LIFECYCLE DISALLOWANCE */

	NavigationServer3D() = delete;
	NavigationServer3D(const NavigationServer3D&) = delete;
	NavigationServer3D& operator=(const NavigationServer3D&) = delete;
	~NavigationServer3D() = delete;
};


