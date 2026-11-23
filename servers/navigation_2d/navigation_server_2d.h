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
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
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
#include "scene/resources/mesh.h"

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

// --- Folded from servers/nav_heap.h ---
#include "core/templates/local_vector.h"

// This file contains Heap which is used by both 2D and 3D navigation.

template <typename T>
struct NoopIndexer {
	void operator()(const T &p_value, uint32_t p_index) {}
};

/**
 * A max-heap implementation that notifies of element index changes.
 */
template <typename T, typename LessThan = Comparator<T>, typename Indexer = NoopIndexer<T>>
class Heap {
	LocalVector<T> _buffer;

	LessThan _less_than;
	Indexer _indexer;

public:
	static constexpr uint32_t INVALID_INDEX = UINT32_MAX;
	void reserve(uint32_t p_size) {
		_buffer.reserve(p_size);
	}

	uint32_t size() const {
		return _buffer.size();
	}

	bool is_empty() const {
		return _buffer.is_empty();
	}

	void push(const T &p_element) {
		_buffer.push_back(p_element);
		_indexer(p_element, _buffer.size() - 1);
		_shift_up(_buffer.size() - 1);
	}

	T pop() {
		ERR_FAIL_COND_V_MSG(_buffer.is_empty(), T(), "Can't pop an empty heap.");
		T value = _buffer[0];
		_indexer(value, INVALID_INDEX);
		if (_buffer.size() > 1) {
			_buffer[0] = _buffer[_buffer.size() - 1];
			_indexer(_buffer[0], 0);
			_buffer.remove_at(_buffer.size() - 1);
			_shift_down(0);
		} else {
			_buffer.remove_at(_buffer.size() - 1);
		}
		return value;
	}

	/**
	 * Update the position of the element in the heap if necessary.
	 */
	void shift(uint32_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX_MSG(p_index, _buffer.size(), "Heap element index is out of range.");
		if (!_shift_up(p_index)) {
			_shift_down(p_index);
		}
	}

	void clear() {
		for (const T &value : _buffer) {
			_indexer(value, INVALID_INDEX);
		}
		_buffer.clear();
	}

	Heap() {}

	Heap(const LessThan &p_less_than) :
			_less_than(p_less_than) {}

	Heap(const Indexer &p_indexer) :
			_indexer(p_indexer) {}

	Heap(const LessThan &p_less_than, const Indexer &p_indexer) :
			_less_than(p_less_than), _indexer(p_indexer) {}

private:
	bool _shift_up(uint32_t p_index) {
		T value = _buffer[p_index];
		uint32_t current_index = p_index;
		uint32_t parent_index = (current_index - 1) / 2;
		while (current_index > 0 && _less_than(_buffer[parent_index], value)) {
			_buffer[current_index] = _buffer[parent_index];
			_indexer(_buffer[current_index], current_index);
			current_index = parent_index;
			parent_index = (current_index - 1) / 2;
		}
		if (current_index != p_index) {
			_buffer[current_index] = value;
			_indexer(value, current_index);
			return true;
		} else {
			return false;
		}
	}

	bool _shift_down(uint32_t p_index) {
		T value = _buffer[p_index];
		uint32_t current_index = p_index;
		uint32_t child_index = 2 * current_index + 1;
		while (child_index < _buffer.size()) {
			if (child_index + 1 < _buffer.size() &&
					_less_than(_buffer[child_index], _buffer[child_index + 1])) {
				child_index++;
			}
			if (_less_than(_buffer[child_index], value)) {
				break;
			}
			_buffer[current_index] = _buffer[child_index];
			_indexer(_buffer[current_index], current_index);
			current_index = child_index;
			child_index = 2 * current_index + 1;
		}
		if (current_index != p_index) {
			_buffer[current_index] = value;
			_indexer(value, current_index);
			return true;
		} else {
			return false;
		}
	}
};
// --- End servers/nav_heap.h ---

// --- Folded from modules/navigation_2d/nav_map_2d.h ---
#include <cfloat>

#include "core/math/math_defs.h"
#include "core/math/rect2.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/os/mutex.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/self_list.h"
#include "core/templates/vector.h"
#include "core/types.h"



#include "thirdparty/rvo2/rvo2_2d/Agent2d.h"
#include "thirdparty/rvo2/rvo2_2d/KdTree2d.h"
#include "thirdparty/rvo2/rvo2_2d/RVOSimulator2d.h"

class NavIteration2D;
class NavMap2D;
class NavRegion2D;
class NavLink2D;
class NavAgent2D;
class NavObstacle2D;
struct NavMapIteration2D;
struct NavMapIterationBuild2D;
struct NavRegionIterationBuild2D;

struct Triangle2
{
	Vector2 vertex[3];

	real_t get_area() const
	{
		return Math::abs((vertex[0] - vertex[1]).cross(vertex[0] - vertex[2])) * 0.5f;
	}

	Vector2 get_random_point_inside() const;

	Vector2 get_closest_point_to(const Vector2& p_point) const;

	Triangle2() {}
	Triangle2(const Vector2& p_v1, const Vector2& p_v2, const Vector2& p_v3)
	{
		vertex[0] = p_v1;
		vertex[1] = p_v2;
		vertex[2] = p_v3;
	}
};

namespace Nav2D {

struct Polygon;

union PointKey {
	struct {
		int64_t x : 32;
		int64_t y : 32;
	};

	uint64_t key = 0;
};

struct EdgeKey {
	PointKey a;
	PointKey b;

	static uint32_t hash(const EdgeKey& p_val)
	{
		return hash_one_uint64(p_val.a.key) ^ hash_one_uint64(p_val.b.key);
	}

	bool operator==(const EdgeKey& p_key) const
	{
		return (a.key == p_key.a.key) && (b.key == p_key.b.key);
	}

	EdgeKey(const PointKey& p_a = PointKey(), const PointKey& p_b = PointKey()) : a(p_a), b(p_b)
	{
		if (a.key > b.key) {
			SWAP(a, b);
		}
	}
};

struct ConnectableEdge {
	EdgeKey ek;
	uint32_t polygon_index;
	int edge = -1;
	Vector2 pathway_start;
	Vector2 pathway_end;
};

struct Connection {
	Polygon* polygon = nullptr;
	int edge = -1;
	Vector2 pathway_start;
	Vector2 pathway_end;
};

struct Polygon {
	uint32_t id = UINT32_MAX;
	const NavIteration2D* owner = nullptr;
	LocalVector<Vector2> vertices;
	real_t surface_area = 0.0;
};

struct NavigationPoly {
	const Polygon* poly = nullptr;
	uint32_t traversable_poly_index = UINT32_MAX;

	int back_navigation_poly_id = -1;
	int back_navigation_edge = -1;
	Vector2 back_navigation_edge_pathway_start;
	Vector2 back_navigation_edge_pathway_end;

	Vector2 entry;
	real_t traveled_distance = 0.0;
	real_t distance_to_destination = 0.0;

	real_t total_travel_cost() const { return traveled_distance + distance_to_destination; }

	bool operator==(const NavigationPoly& p_other) const { return poly == p_other.poly; }

	bool operator!=(const NavigationPoly& p_other) const { return !(*this == p_other); }

	void reset()
	{
		poly = nullptr;
		traversable_poly_index = UINT32_MAX;
		back_navigation_poly_id = -1;
		back_navigation_edge = -1;
		traveled_distance = FLT_MAX;
		distance_to_destination = 0.0;
	}
};

struct NavPolyTravelCostGreaterThan {
	bool operator()(const NavigationPoly* p_poly_a, const NavigationPoly* p_poly_b) const
	{
		real_t f_cost_a = p_poly_a->total_travel_cost();
		real_t h_cost_a = p_poly_a->distance_to_destination;
		real_t f_cost_b = p_poly_b->total_travel_cost();
		real_t h_cost_b = p_poly_b->distance_to_destination;

		if (f_cost_a != f_cost_b) {
			return f_cost_a > f_cost_b;
		}
		else {
			return h_cost_a > h_cost_b;
		}
	}
};

struct NavPolyHeapIndexer {
	void operator()(NavigationPoly* p_poly, uint32_t p_heap_index) const
	{
		p_poly->traversable_poly_index = p_heap_index;
	}
};

struct ClosestPointQueryResult {
	Vector2 point;
	RID owner;
};

struct EdgeConnectionPair {
	Connection connections[2];
	int size = 0;
};

struct PerformanceData {
	int pm_region_count = 0;
	int pm_agent_count = 0;
	int pm_link_count = 0;
	int pm_polygon_count = 0;
	int pm_edge_count = 0;
	int pm_edge_merge_count = 0;
	int pm_edge_connection_count = 0;
	int pm_edge_free_count = 0;
	int pm_obstacle_count = 0;

	void reset()
	{
		pm_region_count = 0;
		pm_agent_count = 0;
		pm_link_count = 0;
		pm_polygon_count = 0;
		pm_edge_count = 0;
		pm_edge_merge_count = 0;
		pm_edge_connection_count = 0;
		pm_edge_free_count = 0;
		pm_obstacle_count = 0;
	}
};

} // namespace Nav2D

class NavIteration2D final : public RefCounted
{
public:
	bool enabled = true;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	NavigationEnums2D::PathSegmentType owner_type = NavigationEnums2D::PATH_SEGMENT_TYPE_REGION;
	RID owner_rid;
	bool owner_use_edge_connections = false;
	LocalVector<Nav2D::Polygon> navmesh_polygons;
	LocalVector<LocalVector<Nav2D::Connection>> internal_connections;

	// Region iteration data.
	Transform2D transform;
	real_t surface_area = 0.0;
	Rect2 bounds;
	LocalVector<Nav2D::ConnectableEdge> external_edges;

	// Link iteration data.
	bool bidirectional = true;
	Vector2 start_position;
	Vector2 end_position;

	bool get_enabled() const { return enabled; }
	NavigationEnums2D::PathSegmentType get_type() const { return owner_type; }
	RID get_self() const { return owner_rid; }
	uint32_t get_navigation_layers() const { return navigation_layers; }
	real_t get_enter_cost() const { return enter_cost; }
	real_t get_travel_cost() const { return travel_cost; }
	bool get_use_edge_connections() const { return owner_use_edge_connections; }

	const LocalVector<Nav2D::Polygon>& get_navmesh_polygons() const { return navmesh_polygons; }
	const LocalVector<LocalVector<Nav2D::Connection>>& get_internal_connections() const
	{
		return internal_connections;
	}

	const Transform2D& get_transform() const { return transform; }
	real_t get_surface_area() const { return surface_area; }
	Rect2 get_bounds() const { return bounds; }
	const LocalVector<Nav2D::ConnectableEdge>& get_external_edges() const { return external_edges; }

	Vector2 get_start_position() const { return start_position; }
	Vector2 get_end_position() const { return end_position; }
	bool is_bidirectional() const { return bidirectional; }

	~NavIteration2D() override
	{
		external_edges.clear();
		navmesh_polygons.clear();
		internal_connections.clear();
	}
};

struct NavRegionIterationBuild2D
{
	Nav2D::PerformanceData performance_data;

	NavRegion2D* region = nullptr;

	Vector2 map_cell_size;
	Transform2D region_transform;

	struct NavMeshData
	{
		Vector<Vector2> vertices;
		Vector<Vector<int>> polygons;

		void clear()
		{
			vertices.clear();
			polygons.clear();
		}
	} navmesh_data;

	Ref<NavIteration2D> region_iteration;

	HashMap<Nav2D::EdgeKey, Nav2D::EdgeConnectionPair, Nav2D::EdgeKey> iter_connection_pairs_map;

	void reset()
	{
		performance_data.reset();

		navmesh_data.clear();
		region_iteration = Ref<NavIteration2D>();
		iter_connection_pairs_map.clear();
	}
};

class NavMeshQueries2D final
{
public:
	struct PathQuerySlot
	{
		LocalVector<Nav2D::NavigationPoly> path_corridor;
		Heap<Nav2D::NavigationPoly*, Nav2D::NavPolyTravelCostGreaterThan, Nav2D::NavPolyHeapIndexer>
			traversable_polys;
		bool in_use = false;
		uint32_t slot_index = 0;
		AHashMap<const Nav2D::Polygon*, uint32_t> poly_to_id;
	};

	struct NavMeshPathQueryTask2D
	{
		enum TaskStatus
		{
			QUERY_STARTED,
			QUERY_FINISHED,
			QUERY_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Vector2 start_position;
		Vector2 target_position;
		uint32_t navigation_layers;
		uint32_t metadata_flags = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_ALL;
		NavigationEnums2D::PathfindingAlgorithm pathfinding_algorithm =
			NavigationEnums2D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
		NavigationEnums2D::PathPostProcessing path_postprocessing =
			NavigationEnums2D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
		bool simplify_path = false;
		real_t simplify_epsilon = 0.0;

		bool exclude_regions = false;
		bool include_regions = false;
		LocalVector<RID> excluded_regions;
		LocalVector<RID> included_regions;

		float path_return_max_length = 0.0;
		float path_return_max_radius = 0.0;
		int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
		float path_search_max_distance = 0.0;

		Vector2 begin_position;
		Vector2 end_position;
		const Nav2D::Polygon* begin_polygon = nullptr;
		const Nav2D::Polygon* end_polygon = nullptr;
		uint32_t least_cost_id = 0;

		NavMap2D* map = nullptr;
		PathQuerySlot* path_query_slot = nullptr;

		LocalVector<Vector2> path_points;
		LocalVector<int32_t> path_meta_point_types;
		LocalVector<RID> path_meta_point_rids;
		LocalVector<int64_t> path_meta_point_owners;
		float path_length = 0.0;

		Ref<NavigationPathQueryParameters2D> query_parameters;
		Ref<NavigationPathQueryResult2D> query_result;
		NavMeshPathQueryTask2D::TaskStatus status =
			NavMeshPathQueryTask2D::TaskStatus::QUERY_STARTED;

		void path_clear()
		{
			path_points.clear();
			path_meta_point_types.clear();
			path_meta_point_rids.clear();
			path_meta_point_owners.clear();
		}

		void path_reverse()
		{
			path_points.reverse();
			path_meta_point_types.reverse();
			path_meta_point_rids.reverse();
			path_meta_point_owners.reverse();
		}
	};

	static Vector2 polygons_get_random_point(const LocalVector<Nav2D::Polygon>& p_polygons,
		uint32_t p_navigation_layers, bool p_uniformly);

	static Vector2 polygons_get_closest_point(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);
	static Nav2D::ClosestPointQueryResult polygons_get_closest_point_info(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);
	static RID polygons_get_closest_point_owner(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);

	static Vector2 map_iteration_get_closest_point(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static RID map_iteration_get_closest_point_owner(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static Nav2D::ClosestPointQueryResult map_iteration_get_closest_point_info(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static Vector2 map_iteration_get_random_point(
		const NavMapIteration2D& p_map_iteration, uint32_t p_navigation_layers, bool p_uniformly);

	static void query_task_map_iteration_get_path(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_push_back_point_with_metadata(NavMeshPathQueryTask2D& p_query_task,
		const Vector2& p_point, const Nav2D::Polygon* p_point_polygon);
	static void _query_task_find_start_end_positions(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_build_path_corridor(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_post_process_corridorfunnel(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_post_process_edgecentered(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_post_process_nopostprocessing(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_clip_path(NavMeshPathQueryTask2D& p_query_task,
		const Nav2D::NavigationPoly* p_from_poly, const Vector2& p_to_point,
		const Nav2D::NavigationPoly* p_to_poly);
	static void _query_task_simplified_path_points(NavMeshPathQueryTask2D& p_query_task);
	static bool _query_task_is_connection_owner_usable(
		const NavMeshPathQueryTask2D& p_query_task, const NavIteration2D* p_owner);
	static void _query_task_process_path_result_limits(NavMeshPathQueryTask2D& p_query_task);

	static void _query_task_search_polygon_connections(NavMeshPathQueryTask2D& p_query_task,
		const Nav2D::Connection& p_connection, uint32_t p_least_cost_id,
		const Nav2D::NavigationPoly& p_least_cost_poly, real_t p_poly_enter_cost,
		const Vector2& p_end_point);

	static void simplify_path_segment(int p_start_inx, int p_end_inx,
		const LocalVector<Vector2>& p_points, real_t p_epsilon,
		LocalVector<uint32_t>& r_simplified_path_indices);
	static LocalVector<uint32_t> get_simplified_path_indices(
		const LocalVector<Vector2>& p_path, real_t p_epsilon);

	static float _calculate_path_length(
		const LocalVector<Vector2>& p_path, uint32_t p_start_index, uint32_t p_end_index);
};

struct NavMapIterationBuild2D
{
	Vector2 merge_rasterizer_cell_size;
	bool use_edge_connections = true;
	real_t edge_connection_margin;
	real_t link_connection_radius;
	Nav2D::PerformanceData performance_data;
	int polygon_count = 0;
	int free_edge_count = 0;

	HashMap<Nav2D::EdgeKey, Nav2D::EdgeConnectionPair, Nav2D::EdgeKey> iter_connection_pairs_map;
	LocalVector<Nav2D::Connection> iter_free_edges;

	NavMapIteration2D* map_iteration = nullptr;

	int navmesh_polygon_count = 0;

	void reset()
	{
		performance_data.reset();

		iter_connection_pairs_map.clear();
		iter_free_edges.clear();
		polygon_count = 0;
		free_edge_count = 0;

		navmesh_polygon_count = 0;
	}
};

struct NavMapIteration2D
{
	mutable SafeNumeric<uint32_t> users;
	RWLock rwlock;

	LocalVector<Ref<NavIteration2D>> region_iterations;
	LocalVector<Ref<NavIteration2D>> link_iterations;

	int navmesh_polygon_count = 0;

	HashMap<const NavIteration2D*, LocalVector<Nav2D::Connection>> external_region_connections;
	HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>
		navbases_polygons_external_connections;

	LocalVector<Nav2D::Polygon> navlink_polygons;

	HashMap<NavRegion2D*, Ref<NavIteration2D>> region_ptr_to_region_iteration;

	LocalVector<NavMeshQueries2D::PathQuerySlot> path_query_slots;
	Mutex path_query_slots_mutex;
	Semaphore path_query_slots_semaphore;

	void clear()
	{
		navmesh_polygon_count = 0;

		region_iterations.clear();
		link_iterations.clear();
		external_region_connections.clear();
		navbases_polygons_external_connections.clear();
		navlink_polygons.clear();
		region_ptr_to_region_iteration.clear();
	}
};

class NavMapIterationRead2D final
{
	const NavMapIteration2D& map_iteration;

public:
	_ALWAYS_INLINE_ NavMapIterationRead2D(const NavMapIteration2D& p_iteration)
		: map_iteration(p_iteration)
	{
		map_iteration.rwlock.read_lock();
		map_iteration.users.increment();
	}

	_ALWAYS_INLINE_ ~NavMapIterationRead2D()
	{
		map_iteration.users.decrement();
		map_iteration.rwlock.read_unlock();
	}
};

class NavMapBuilder2D final
{
	static void _build_step_gather_region_polygons(NavMapIterationBuild2D& r_build);
	static void _build_step_find_edge_connection_pairs(NavMapIterationBuild2D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavMapIterationBuild2D& r_build);
	static void _build_step_edge_connection_margin_connections(NavMapIterationBuild2D& r_build);
	static void _build_step_navlink_connections(NavMapIterationBuild2D& r_build);
	static void _build_update_map_iteration(NavMapIterationBuild2D& r_build);

public:
	static Nav2D::PointKey get_point_key(const Vector2& p_pos, const Vector2& p_cell_size);

	static void build_navmap_iteration(NavMapIterationBuild2D& r_build);
};

class NavRegionBuilder2D final
{
	static void _build_step_process_navmesh_data(NavRegionIterationBuild2D& r_build);
	static void _build_step_find_edge_connection_pairs(NavRegionIterationBuild2D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavRegionIterationBuild2D& r_build);
	static void _build_update_iteration(NavRegionIterationBuild2D& r_build);

public:
	static Nav2D::PointKey get_point_key(const Vector2& p_pos, const Vector2& p_cell_size);
	static Nav2D::EdgeKey get_edge_key(
		const Vector2& p_vertex1, const Vector2& p_vertex2, const Vector2& p_cell_size);

	static void build_iteration(NavRegionIterationBuild2D& r_build);
};

class NavAgent2D final
{
	RID self;

	Vector2 position;
	Vector2 target_position;
	Vector2 velocity;
	Vector2 velocity_forced;
	real_t radius = NavigationDefaults2D::AVOIDANCE_AGENT_RADIUS;
	real_t max_speed = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t time_horizon_agents = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	int max_neighbors = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t neighbor_distance = NavigationDefaults2D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	Vector2 safe_velocity;
	bool clamp_speed = true;

	NavMap2D* map = nullptr;

	RVO2D::Agent2D rvo_agent;
	bool avoidance_enabled = false;

	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;

	bool agent_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavAgent2D> sync_dirty_request_list_element;

public:
	NavAgent2D();
	~NavAgent2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() { return map; }

	bool is_map_changed();

	RVO2D::Agent2D* get_rvo_agent() { return &rvo_agent; }

	bool has_avoidance_callback() const;
	void dispatch_avoidance_callback();

	void set_neighbor_distance(real_t p_neighbor_distance);
	real_t get_neighbor_distance() const { return neighbor_distance; }

	void set_max_neighbors(int p_max_neighbors);
	int get_max_neighbors() const { return max_neighbors; }

	void set_time_horizon_agents(real_t p_time_horizon);
	real_t get_time_horizon_agents() const { return time_horizon_agents; }

	void set_time_horizon_obstacles(real_t p_time_horizon);
	real_t get_time_horizon_obstacles() const { return time_horizon_obstacles; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_max_speed(real_t p_max_speed);
	real_t get_max_speed() const { return max_speed; }

	void set_position(const Vector2& p_position);
	Vector2 get_position() const { return position; }

	void set_target_position(const Vector2& p_target_position);
	Vector2 get_target_position() const { return target_position; }

	void set_velocity(const Vector2& p_velocity);
	Vector2 get_velocity() const { return velocity; }

	void set_velocity_forced(const Vector2& p_velocity);
	Vector2 get_velocity_forced() const { return velocity_forced; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const { return avoidance_layers; }

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const { return avoidance_mask; }

	void set_avoidance_priority(real_t p_priority);
	real_t get_avoidance_priority() const { return avoidance_priority; }

	void set_paused(bool p_paused);
	bool get_paused() const;

	bool is_dirty() const;
	void sync();
	void request_sync();
	void cancel_sync_request();

	void update();

private:
	void _update_rvo_agent_properties();
};

class NavObstacle2D final
{
	RID self;

	NavAgent2D* agent = nullptr;
	NavMap2D* map = nullptr;
	Vector2 velocity;
	Vector2 position;
	Vector<Vector2> vertices;

	real_t radius = 0.0;

	bool avoidance_enabled = false;
	uint32_t avoidance_layers = 1;

	bool obstacle_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavObstacle2D> sync_dirty_request_list_element;

public:
	NavObstacle2D();
	~NavObstacle2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() { return map; }

	void set_agent(NavAgent2D* p_agent);
	NavAgent2D* get_agent() { return agent; }

	void set_position(const Vector2& p_position);
	Vector2 get_position() const { return position; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_velocity(const Vector2& p_velocity);
	Vector2 get_velocity() const { return velocity; }

	void set_vertices(const Vector<Vector2>& p_vertices);
	const Vector<Vector2>& get_vertices() const { return vertices; }

	bool is_map_changed();

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const { return avoidance_layers; }

	void set_paused(bool p_paused);
	bool get_paused() const;

	bool is_dirty() const;
	void sync();
	void request_sync();
	void cancel_sync_request();

private:
	void internal_update_agent();
};

class NavLink2D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	NavMap2D* map = nullptr;
	bool bidirectional = true;
	Vector2 start_position;
	Vector2 end_position;
	bool enabled = true;

	SelfList<NavLink2D> sync_dirty_request_list_element;

	uint32_t iteration_id = 0;

	mutable RWLock iteration_rwlock;
	Ref<NavIteration2D> iteration;

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavLink2D();
	~NavLink2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums2D::PathSegmentType get_type() const
	{
		return NavigationEnums2D::PATH_SEGMENT_TYPE_LINK;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() const { return map; }

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_bidirectional(bool p_bidirectional);
	bool is_bidirectional() const { return bidirectional; }

	void set_start_position(Vector2 p_position);
	Vector2 get_start_position() const { return start_position; }

	void set_end_position(Vector2 p_position);
	Vector2 get_end_position() const { return end_position; }

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const { return travel_cost; }

	bool sync();
	void request_sync();
	void cancel_sync_request();

	Ref<NavIteration2D> get_iteration();
};

class NavRegion2D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	RWLock region_rwlock;

	NavMap2D* map = nullptr;
	Transform2D transform;
	bool enabled = true;

	bool use_edge_connections = true;

	Rect2 bounds;

	Ref<NavigationPolygon> navmesh;

	Nav2D::PerformanceData performance_data;

	uint32_t iteration_id = 0;

	SelfList<NavRegion2D> sync_dirty_request_list_element;
	mutable RWLock iteration_rwlock;
	Ref<NavIteration2D> iteration;

	NavRegionIterationBuild2D iteration_build;
	bool use_async_iterations = true;
	SelfList<NavRegion2D> async_list_element;
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavRegion2D();
	~NavRegion2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums2D::PathSegmentType get_type() const
	{
		return NavigationEnums2D::PATH_SEGMENT_TYPE_REGION;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void scratch_polygons();

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() const { return map; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_transform(Transform2D transform);
	const Transform2D& get_transform() const { return transform; }

	void set_navigation_mesh(Ref<NavigationPolygon> p_navigation_mesh);
	Ref<NavigationPolygon> get_navigation_mesh() const { return navmesh; }

	const LocalVector<Nav2D::Polygon>& get_polygons() const;

	Nav2D::ClosestPointQueryResult get_closest_point_info(const Vector2& p_point) const;
	Vector2 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	real_t get_surface_area() const;
	Rect2 get_bounds() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const { return travel_cost; }

	bool sync();
	void request_sync();
	void cancel_sync_request();

	void sync_async_tasks();
	void request_async_thread_join();
	void cancel_async_thread_join();

	Ref<NavIteration2D> get_iteration();

	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;
};

class NavMap2D final
{
	RID self;

	real_t cell_size = NavigationDefaults2D::NAV_MESH_CELL_SIZE;
	Vector2 merge_rasterizer_cell_size = Vector2(cell_size, cell_size);
	float merge_rasterizer_cell_scale = 0.1;

	bool use_edge_connections = true;
	real_t edge_connection_margin = NavigationDefaults2D::EDGE_CONNECTION_MARGIN;
	real_t link_connection_radius = NavigationDefaults2D::LINK_CONNECTION_RADIUS;

	bool map_settings_dirty = true;

	LocalVector<NavRegion2D*> regions;
	LocalVector<NavLink2D*> links;

	RVO2D::RVOSimulator2D rvo_simulation;

	LocalVector<NavAgent2D*> active_avoidance_agents;
	bool agents_dirty = true;
	LocalVector<NavAgent2D*> agents;

	LocalVector<NavObstacle2D*> obstacles;
	bool obstacles_dirty = true;

	uint32_t iteration_id = 0;

	bool use_threads = true;
	bool avoidance_use_multiple_threads = true;
	bool avoidance_use_high_priority_threads = true;

	Nav2D::PerformanceData performance_data;

	int path_query_slots_max = 4;

	bool use_async_iterations = true;

	uint32_t iteration_slot_index = 0;
	LocalVector<NavMapIteration2D> iteration_slots;
	mutable RWLock iteration_slot_rwlock;

	NavMapIterationBuild2D iteration_build;
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavMap2D();
	~NavMap2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const { return cell_size; }

	void set_merge_rasterizer_cell_scale(float p_value);
	float get_merge_rasterizer_cell_scale() const { return merge_rasterizer_cell_scale; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_edge_connection_margin(real_t p_edge_connection_margin);
	real_t get_edge_connection_margin() const { return edge_connection_margin; }

	void set_link_connection_radius(real_t p_link_connection_radius);
	real_t get_link_connection_radius() const { return link_connection_radius; }

	Nav2D::PointKey get_point_key(const Vector2& p_pos) const;
	const Vector2& get_merge_rasterizer_cell_size() const;

	void query_path(NavMeshQueries2D::NavMeshPathQueryTask2D& p_query_task);

	Vector2 get_closest_point(const Vector2& p_point) const;
	Nav2D::ClosestPointQueryResult get_closest_point_info(const Vector2& p_point) const;
	RID get_closest_point_owner(const Vector2& p_point) const;

	void add_region(NavRegion2D* p_region);
	void remove_region(NavRegion2D* p_region);
	const LocalVector<NavRegion2D*>& get_regions() const { return regions; }

	void add_link(NavLink2D* p_link);
	void remove_link(NavLink2D* p_link);
	const LocalVector<NavLink2D*>& get_links() const { return links; }

	bool has_agent(NavAgent2D* p_agent) const;
	void add_agent(NavAgent2D* p_agent);
	void remove_agent(NavAgent2D* p_agent);
	const LocalVector<NavAgent2D*>& get_agents() const { return agents; }

	void set_agent_as_controlled(NavAgent2D* p_agent);
	void remove_agent_as_controlled(NavAgent2D* p_agent);

	bool has_obstacle(NavObstacle2D* p_obstacle) const;
	void add_obstacle(NavObstacle2D* p_obstacle);
	void remove_obstacle(NavObstacle2D* p_obstacle);
	const LocalVector<NavObstacle2D*>& get_obstacles() const { return obstacles; }

	Vector2 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	void sync();
	void step(double p_delta_time);
	void dispatch_callbacks();

	int get_pm_region_count() const { return performance_data.pm_region_count; }
	int get_pm_agent_count() const { return performance_data.pm_agent_count; }
	int get_pm_link_count() const { return performance_data.pm_link_count; }
	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }
	int get_pm_edge_connection_count() const { return performance_data.pm_edge_connection_count; }
	int get_pm_edge_free_count() const { return performance_data.pm_edge_free_count; }
	int get_pm_obstacle_count() const { return performance_data.pm_obstacle_count; }

	int get_region_connections_count(NavRegion2D* p_region) const;
	Vector2 get_region_connection_pathway_start(NavRegion2D* p_region, int p_connection_id) const;
	Vector2 get_region_connection_pathway_end(NavRegion2D* p_region, int p_connection_id) const;

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;

private:
	void _sync_dirty_map_update_requests();
	void _sync_dirty_avoidance_update_requests();
	void _sync_async_tasks();

	void compute_single_step(uint32_t p_index, NavAgent2D** p_agent);
	void compute_single_avoidance_step(uint32_t p_index, NavAgent2D** p_agent);

	void _sync_avoidance();
	void _update_rvo_simulation();
	void _update_rvo_obstacles_tree();
	void _update_rvo_agents_tree();

	void _update_merge_rasterizer_cell_dimensions();
};

#ifdef CLIPPER2_ENABLED

class Node;

struct NavMeshGeometryParser2D
{
	RID self;
};

class NavMeshGenerator2D final
{
	static inline bool _initialized = false;

	static Mutex baking_navmesh_mutex;
	static Mutex generator_task_mutex;

	static RWLock generator_parsers_rwlock;
	static LocalVector<NavMeshGeometryParser2D*> generator_parsers;

	static bool use_threads;
	static bool baking_use_multiple_threads;
	static bool baking_use_high_priority_threads;

	struct NavMeshGeneratorTask2D
	{
		enum TaskStatus
		{
			BAKING_STARTED,
			BAKING_FINISHED,
			BAKING_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Ref<NavigationPolygon> navigation_mesh;
		Ref<NavigationMeshSourceGeometryData2D> source_geometry_data;
		NavMeshGeneratorTask2D::TaskStatus status =
			NavMeshGeneratorTask2D::TaskStatus::BAKING_STARTED;
	};

	static void generator_thread_bake(void* p_arg);

	static HashSet<Ref<NavigationPolygon>> baking_navmeshes;

	static void generator_parse_geometry_node(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node,
		bool p_recurse_children);
	static void generator_parse_source_geometry_data(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_root_node);
	static void generator_bake_from_source_geometry_data(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return _initialized; }

	static void sync();
	static void cleanup();
	static void finish();

	static void set_generator_parsers(const LocalVector<NavMeshGeometryParser2D*>& p_parsers);

	static bool is_baking(Ref<NavigationPolygon> p_navigation_polygon);

	NavMeshGenerator2D() = delete;
	NavMeshGenerator2D(const NavMeshGenerator2D&) = delete;
	NavMeshGenerator2D& operator=(const NavMeshGenerator2D&) = delete;
	~NavMeshGenerator2D() = delete;
};

#endif // CLIPPER2_ENABLED
// --- End modules/navigation_2d/nav_map_2d.h ---
