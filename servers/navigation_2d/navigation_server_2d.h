/**************************************************************************/
/*  navigation_server_2d.h                                                */
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

#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/math/vector2.h"
#include "core/os/thread_safe.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/types.h"
// --- Inlined/Folded from scene/resources/2d/navigation_mesh_source_geometry_data_2d.h ---
#include "core/io/resource.h"
#include "core/os/mutex.h"

class NavigationMeshSourceGeometryData2D : public Resource
{
	friend class NavMeshGenerator2D;

	RWLock geometry_rwlock;

	Vector<Vector<Vector2>> traversable_outlines;
	Vector<Vector<Vector2>> obstruction_outlines;

	Rect2 bounds;
	bool bounds_dirty = true;

public:
	struct ProjectedObstruction;

private:
	Vector<ProjectedObstruction> _projected_obstructions;

protected:

public:
	struct ProjectedObstruction
	{
		static inline uint32_t VERSION = 1; // Increase when format changes so we can detect
											// outdated formats and provide compatibility.

		Vector<float> vertices;
		bool carve = false;
	};

	void _set_traversable_outlines(const Vector<Vector<Vector2>>& p_traversable_outlines);
	const Vector<Vector<Vector2>>& _get_traversable_outlines() const;

	void _set_obstruction_outlines(const Vector<Vector<Vector2>>& p_obstruction_outlines);
	const Vector<Vector<Vector2>>& _get_obstruction_outlines() const;

	void _add_traversable_outline(const Vector<Vector2>& p_shape_outline);
	void _add_obstruction_outline(const Vector<Vector2>& p_shape_outline);

	// kept root node transform here on the geometry data
	// if we add this transform to all exposed functions we need to break comp on all functions
	// later when navmesh changes from global transform to relative to navregion but if it stays
	// here we can just remove it and change the internal functions only
	Transform2D root_node_transform;

	void add_traversable_outline(const PackedVector2Array& p_shape_outline);
	void add_obstruction_outline(const PackedVector2Array& p_shape_outline);

	bool has_data();
	void clear();
	void clear_projected_obstructions();

	void add_projected_obstruction(const Vector<Vector2>& p_vertices, bool p_carve);
	Vector<ProjectedObstruction> _get_projected_obstructions() const;

	void merge(const Ref<NavigationMeshSourceGeometryData2D>& p_other_geometry);

	void set_data(const Vector<Vector<Vector2>>& p_traversable_outlines,
		const Vector<Vector<Vector2>>& p_obstruction_outlines,
		Vector<ProjectedObstruction>& p_projected_obstructions);
	void get_data(Vector<Vector<Vector2>>& r_traversable_outlines,
		Vector<Vector<Vector2>>& r_obstruction_outlines,
		Vector<ProjectedObstruction>& r_projected_obstructions);

	Rect2 get_bounds();

	~NavigationMeshSourceGeometryData2D() { clear(); }
};
// --- Inlined/Folded from scene/resources/2d/navigation_polygon.h ---
#include "scene/resources/navigation_mesh.h"

class NavigationPolygon : public Resource
{
	RWLock rwlock;

	Vector<Vector2> vertices;
	Vector<Vector<int>> polygons;
	Vector<Vector<Vector2>> outlines;

	mutable Rect2 item_rect;
	mutable bool rect_cache_dirty = true;

	Mutex navigation_mesh_generation;
	// Navigation mesh
	Ref<NavigationMesh> navigation_mesh;

	real_t cell_size = 1.0f;
	real_t border_size = 0.0f;

	Rect2 baking_rect;
	Vector2 baking_rect_offset;

protected:

public:
	NavigationPolygon();

#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const;
	bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;
#endif // DEBUG_ENABLED

	enum SamplePartitionType
	{
		SAMPLE_PARTITION_CONVEX_PARTITION = 0,
		SAMPLE_PARTITION_TRIANGULATE,
		SAMPLE_PARTITION_MAX
	};

	enum ParsedGeometryType
	{
		PARSED_GEOMETRY_MESH_INSTANCES = 0,
		PARSED_GEOMETRY_STATIC_COLLIDERS,
		PARSED_GEOMETRY_BOTH,
		PARSED_GEOMETRY_MAX
	};

	enum SourceGeometryMode
	{
		SOURCE_GEOMETRY_ROOT_NODE_CHILDREN = 0,
		SOURCE_GEOMETRY_GROUPS_WITH_CHILDREN,
		SOURCE_GEOMETRY_GROUPS_EXPLICIT,
		SOURCE_GEOMETRY_MAX
	};

	real_t agent_radius = 10.0f;

	SamplePartitionType partition_type = SAMPLE_PARTITION_CONVEX_PARTITION;
	ParsedGeometryType parsed_geometry_type = PARSED_GEOMETRY_BOTH;
	uint32_t parsed_collision_mask = 0xFFFFFFFF;

	SourceGeometryMode source_geometry_mode = SOURCE_GEOMETRY_ROOT_NODE_CHILDREN;
	String source_geometry_group_name = "navigation_polygon_source_geometry_group";

	void set_vertices(const Vector<Vector2>& p_vertices);
	const Vector<Vector2>& get_vertices() const _LIFETIME_BOUND_;

	void add_polygon(const Vector<int>& p_polygon);
	int get_polygon_count() const;

	void add_outline(const Vector<Vector2>& p_outline);
	void add_outline_at_index(const Vector<Vector2>& p_outline, int p_index);
	void set_outline(int p_idx, const Vector<Vector2>& p_outline);
	Vector<Vector2> get_outline(int p_idx) const;
	void remove_outline(int p_idx);
	int get_outline_count() const;
	void set_outlines(const Vector<Vector<Vector2>>& p_outlines);
	Vector<Vector<Vector2>> get_outlines() const;

	void clear_outlines();
#ifndef DISABLE_DEPRECATED
	void make_polygons_from_outlines();
#endif // DISABLE_DEPRECATED

	void set_polygons(const Vector<Vector<int>>& p_polygons);
	Vector<Vector<int>> get_polygons() const;
	Vector<int> get_polygon(int p_idx);
	void clear_polygons();

	void set_sample_partition_type(SamplePartitionType p_value);
	SamplePartitionType get_sample_partition_type() const;

	void set_parsed_geometry_type(ParsedGeometryType p_geometry_type);
	ParsedGeometryType get_parsed_geometry_type() const;

	void set_parsed_collision_mask(uint32_t p_mask);
	uint32_t get_parsed_collision_mask() const;

	void set_parsed_collision_mask_value(int p_layer_number, bool p_value);
	bool get_parsed_collision_mask_value(int p_layer_number) const;

	void set_source_geometry_mode(SourceGeometryMode p_geometry_mode);
	SourceGeometryMode get_source_geometry_mode() const;

	void set_source_geometry_group_name(const String& p_group_name);
	String get_source_geometry_group_name() const;

	void set_agent_radius(real_t p_value);
	real_t get_agent_radius() const;

	Ref<NavigationMesh> get_navigation_mesh();

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const;

	void set_border_size(real_t p_value);
	real_t get_border_size() const;

	void set_baking_rect(const Rect2& p_rect);
	Rect2 get_baking_rect() const;

	void set_baking_rect_offset(const Vector2& p_rect_offset);
	Vector2 get_baking_rect_offset() const;

	void clear();

	void set_data(const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons);
	void set_data(const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons,
		const Vector<Vector<Vector2>>& p_outlines);
	void get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons);
	void get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons,
		Vector<Vector<Vector2>>& r_outlines);
};

namespace NavigationEnums2D {

enum PathfindingAlgorithm {
	PATHFINDING_ALGORITHM_ASTAR = 0,
};

enum PathPostProcessing {
	PATH_POSTPROCESSING_CORRIDORFUNNEL = 0,
	PATH_POSTPROCESSING_EDGECENTERED,
	PATH_POSTPROCESSING_NONE,
};

enum PathSegmentType {
	PATH_SEGMENT_TYPE_REGION = 0,
	PATH_SEGMENT_TYPE_LINK
};

enum PathMetadataFlags {
	PATH_INCLUDE_NONE = 0,
	PATH_INCLUDE_TYPES = 1,
	PATH_INCLUDE_RIDS = 2,
	PATH_INCLUDE_OWNERS = 4,
	PATH_INCLUDE_ALL = PATH_INCLUDE_TYPES | PATH_INCLUDE_RIDS | PATH_INCLUDE_OWNERS
};

} // namespace NavigationEnums2D

namespace NavigationDefaults2D {

// Rasterization.

// Same as in 3D but larger since 1px is treated as 1m.
constexpr float NAV_MESH_CELL_SIZE = 1.0f; // Must match ProjectSettings default 2D cell_size.
constexpr float NAV_MESH_CELL_SIZE_MIN = 0.01f;
constexpr const char *const NAV_MESH_CELL_SIZE_HINT = "0.001,100,0.001,or_greater";

// Map.

constexpr float EDGE_CONNECTION_MARGIN = 1.0f;
constexpr float LINK_CONNECTION_RADIUS = 4.0f;
constexpr int path_search_max_polygons = 4096;

// Agent.

constexpr float AVOIDANCE_AGENT_RADIUS = 10.0;
constexpr float AVOIDANCE_AGENT_MAX_SPEED = 100.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_AGENTS = 1.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES = 0.0;
constexpr int AVOIDANCE_AGENT_MAX_NEIGHBORS = 10;
constexpr float AVOIDANCE_AGENT_NEIGHBOR_DISTANCE = 500.0;

} // namespace NavigationDefaults2D

class NavigationPathQueryParameters2D final : public RefCounted
{
public:
	enum PathfindingAlgorithm
	{
		PATHFINDING_ALGORITHM_ASTAR = NavigationEnums2D::PATHFINDING_ALGORITHM_ASTAR,
	};

	enum PathPostProcessing
	{
		PATH_POSTPROCESSING_CORRIDORFUNNEL = NavigationEnums2D::PATH_POSTPROCESSING_CORRIDORFUNNEL,
		PATH_POSTPROCESSING_EDGECENTERED = NavigationEnums2D::PATH_POSTPROCESSING_EDGECENTERED,
		PATH_POSTPROCESSING_NONE = NavigationEnums2D::PATH_POSTPROCESSING_NONE,
	};

	enum PathMetadataFlags
	{
		PATH_METADATA_INCLUDE_NONE = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_NONE,
		PATH_METADATA_INCLUDE_TYPES = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_TYPES,
		PATH_METADATA_INCLUDE_RIDS = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_RIDS,
		PATH_METADATA_INCLUDE_OWNERS = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_OWNERS,
		PATH_METADATA_INCLUDE_ALL = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_ALL
	};

private:
	PathfindingAlgorithm pathfinding_algorithm = PATHFINDING_ALGORITHM_ASTAR;
	PathPostProcessing path_postprocessing = PATH_POSTPROCESSING_CORRIDORFUNNEL;
	RID map;
	Vector2 start_position;
	Vector2 target_position;
	uint32_t navigation_layers = 1;
	uint32_t metadata_flags = PATH_METADATA_INCLUDE_ALL;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;

	LocalVector<RID> _excluded_regions;
	LocalVector<RID> _included_regions;

	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

public:
	void set_pathfinding_algorithm(const PathfindingAlgorithm p_pathfinding_algorithm);
	PathfindingAlgorithm get_pathfinding_algorithm() const;

	void set_path_postprocessing(const PathPostProcessing p_path_postprocessing);
	PathPostProcessing get_path_postprocessing() const;

	void set_map(RID p_map);
	RID get_map() const;

	void set_start_position(const Vector2 p_start_position);
	Vector2 get_start_position() const;

	void set_target_position(const Vector2 p_target_position);
	Vector2 get_target_position() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_metadata_flags(uint32_t p_flags);
	uint32_t get_metadata_flags() const;

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
};

class NavigationPathQueryResult2D final : public RefCounted
{
	Vector<Vector2> path;
	Vector<int32_t> path_types;
	Vector<int64_t> path_owner_ids;
	float path_length = 0.0;

public:
	enum PathSegmentType
	{
		PATH_SEGMENT_TYPE_REGION = NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_REGION,
		PATH_SEGMENT_TYPE_LINK = NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_LINK,
	};

	void set_path(const Vector<Vector2>& p_path);
	const Vector<Vector2>& get_path() const;

	void set_path_types(const Vector<int32_t>& p_path_types);
	const Vector<int32_t>& get_path_types() const;

	void set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids);
	const Vector<int64_t>& get_path_owner_ids() const;

	void set_path_length(float p_length);
	float get_path_length() const;

	void reset();

	void set_data(const LocalVector<Vector2>& p_path, const LocalVector<int32_t>& p_path_types,
		const LocalVector<RID>& p_path_rids, const LocalVector<int64_t>& p_path_owner_ids);
};

class NavigationServer2D final
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
		bool debug_navigation_enable_agent_radius;
		bool debug_navigation_enable_obstacles;
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
		bool debug_navigation_enable_edge_lines = true;
		bool debug_navigation_enable_geometry_face_random_color = true;
		bool debug_navigation_enable_link_connections = true;
		bool debug_navigation_enable_agent_paths = true;

		bool debug_navigation_avoidance_enable_agents_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_static = true;
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

	static void map_set_cell_size(RID p_map, real_t p_cell_size);
	static real_t map_get_cell_size(RID p_map);

	static void map_set_merge_rasterizer_cell_scale(RID p_map, float p_value);
	static float map_get_merge_rasterizer_cell_scale(RID p_map);

	static void map_set_use_edge_connections(RID p_map, bool p_enabled);
	static bool map_get_use_edge_connections(RID p_map);

	static void map_set_edge_connection_margin(RID p_map, real_t p_connection_margin);
	static real_t map_get_edge_connection_margin(RID p_map);

	static void map_set_link_connection_radius(RID p_map, real_t p_connection_radius);
	static real_t map_get_link_connection_radius(RID p_map);

	static Vector<Vector2> map_get_path(RID p_map, Vector2 p_origin, Vector2 p_destination,
		bool p_optimize, uint32_t p_navigation_layers = 1);
	static Vector2 map_get_closest_point(RID p_map, const Vector2& p_point);
	static RID map_get_closest_point_owner(RID p_map, const Vector2& p_point);

	static void map_force_update(RID p_map);
	static uint32_t map_get_iteration_id(RID p_map);

	static void map_set_use_async_iterations(RID p_map, bool p_enabled);
	static bool map_get_use_async_iterations(RID p_map);

	static Vector2 map_get_random_point(RID p_map, uint32_t p_navigation_layers, bool p_uniformly);

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

	static bool region_owns_point(RID p_region, const Vector2& p_point);

	static void region_set_map(RID p_region, RID p_map);
	static RID region_get_map(RID p_region);

	static void region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers);
	static uint32_t region_get_navigation_layers(RID p_region);

	static void region_set_transform(RID p_region, Transform2D p_transform);
	static Transform2D region_get_transform(RID p_region);

	static void region_set_navigation_polygon(
		RID p_region, Ref<NavigationPolygon> p_navigation_polygon);

	static int region_get_connections_count(RID p_region);
	static Vector2 region_get_connection_pathway_start(RID p_region, int p_connection_id);
	static Vector2 region_get_connection_pathway_end(RID p_region, int p_connection_id);

	static Vector2 region_get_closest_point(RID p_region, const Vector2& p_point);
	static Vector2 region_get_random_point(
		RID p_region, uint32_t p_navigation_layers, bool p_uniformly);
	static Rect2 region_get_bounds(RID p_region);

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

	static void link_set_start_position(RID p_link, Vector2 p_position);
	static Vector2 link_get_start_position(RID p_link);

	static void link_set_end_position(RID p_link, Vector2 p_position);
	static Vector2 link_get_end_position(RID p_link);

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

	static void agent_set_max_speed(RID p_agent, real_t p_max_speed);
	static real_t agent_get_max_speed(RID p_agent);

	static void agent_set_velocity_forced(RID p_agent, Vector2 p_velocity);
	static void agent_set_velocity(RID p_agent, Vector2 p_velocity);
	static Vector2 agent_get_velocity(RID p_agent);

	static void agent_set_position(RID p_agent, Vector2 p_position);
	static Vector2 agent_get_position(RID p_agent);

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

	static void obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_avoidance_enabled(RID p_obstacle);

	static void obstacle_set_map(RID p_obstacle, RID p_map);
	static RID obstacle_get_map(RID p_obstacle);

	static void obstacle_set_paused(RID p_obstacle, bool p_paused);
	static bool obstacle_get_paused(RID p_obstacle);

	static void obstacle_set_radius(RID p_obstacle, real_t p_radius);
	static real_t obstacle_get_radius(RID p_obstacle);

	static void obstacle_set_velocity(RID p_obstacle, Vector2 p_velocity);
	static Vector2 obstacle_get_velocity(RID p_obstacle);

	static void obstacle_set_position(RID p_obstacle, Vector2 p_position);
	static Vector2 obstacle_get_position(RID p_obstacle);

	static void obstacle_set_vertices(RID p_obstacle, const Vector<Vector2>& p_vertices);
	static Vector<Vector2> obstacle_get_vertices(RID p_obstacle);

	static void obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers);
	static uint32_t obstacle_get_avoidance_layers(RID p_obstacle);

	/* PATH QUERY & UTILITIES */

	static void query_path(const Ref<NavigationPathQueryParameters2D>& p_query_parameters,
		Ref<NavigationPathQueryResult2D> p_query_result);
	static Vector<Vector2> simplify_path(const Vector<Vector2>& p_path, real_t p_epsilon);

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

	static void set_debug_navigation_geometry_face_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_color();

	static void set_debug_navigation_geometry_face_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_disabled_color();

	static void set_debug_navigation_geometry_edge_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_color();

	static void set_debug_navigation_enable_obstacles(bool p_enable);
	static bool get_debug_navigation_enable_obstacles();

	static void set_debug_navigation_geometry_edge_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_disabled_color();

	static void set_debug_navigation_link_connection_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_color();

	static void set_debug_navigation_link_connection_disabled_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_disabled_color();

	static void set_debug_navigation_enable_edge_connections(bool p_value);
	static bool get_debug_navigation_enable_edge_connections();

	static void set_debug_navigation_enable_geometry_face_random_color(bool p_value);
	static bool get_debug_navigation_enable_geometry_face_random_color();

	static void set_debug_navigation_enable_link_connections(bool p_enable);
	static bool get_debug_navigation_enable_link_connections();

	static void set_debug_navigation_enable_edge_lines(bool p_value);
	static bool get_debug_navigation_enable_edge_lines();

	static void set_debug_navigation_agent_path_color(const Color& p_color);
	static Color get_debug_navigation_agent_path_color();

	static void set_debug_navigation_enable_agent_radius(bool p_enable);
	static bool get_debug_navigation_enable_agent_radius();

	static void set_debug_navigation_enable_agent_paths(bool p_value);
	static bool get_debug_navigation_enable_agent_paths();

	static void set_debug_navigation_agent_path_point_size(real_t p_point_size);
	static real_t get_debug_navigation_agent_path_point_size();

	static void set_debug_navigation_avoidance_enable_agents_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_agents_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_radius();

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

	static void set_debug_navigation_avoidance_enable_obstacles_static(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_static();
#endif // DEBUG_ENABLED

	/* LIFECYCLE DISALLOWANCE */

	NavigationServer2D() = delete;
	NavigationServer2D(const NavigationServer2D&) = delete;
	NavigationServer2D& operator=(const NavigationServer2D&) = delete;
	~NavigationServer2D() = delete;
};
