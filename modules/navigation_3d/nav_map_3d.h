/**************************************************************************/
/*  nav_map_3d.h                                                          */
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

#include <cfloat>

#include "core/math/aabb.h"
#include "core/math/math_defs.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/os/rw_lock.h"
#include "core/os/semaphore.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
#include "core/types.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/nav_heap.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "thirdparty/rvo2/rvo2_2d/Agent2d.h"
#include "thirdparty/rvo2/rvo2_2d/KdTree2d.h"
#include "thirdparty/rvo2/rvo2_2d/RVOSimulator2d.h"
#include "thirdparty/rvo2/rvo2_3d/Agent3d.h"
#include "thirdparty/rvo2/rvo2_3d/KdTree3d.h"
#include "thirdparty/rvo2/rvo2_3d/RVOSimulator3d.h"

class NavIteration3D;
class NavMap3D;
class NavRegion3D;
class NavLink3D;
class NavAgent3D;
class NavObstacle3D;
struct NavMapIteration3D;
struct NavMapIterationBuild3D;
struct NavRegionIterationBuild3D;

namespace Nav3D {

struct Polygon;

union PointKey {
	struct {
		int64_t x : 21;
		int64_t y : 22;
		int64_t z : 21;
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
	Vector3 pathway_start;
	Vector3 pathway_end;
};

struct Connection {
	Polygon* polygon = nullptr;
	int edge = -1;
	Vector3 pathway_start;
	Vector3 pathway_end;
};

struct Polygon {
	uint32_t id = UINT32_MAX;
	const NavIteration3D* owner = nullptr;
	LocalVector<Vector3> vertices;
	real_t surface_area = 0.0;
};

struct NavigationPoly {
	const Polygon* poly = nullptr;
	uint32_t traversable_poly_index = UINT32_MAX;

	int back_navigation_poly_id = -1;
	int back_navigation_edge = -1;
	Vector3 back_navigation_edge_pathway_start;
	Vector3 back_navigation_edge_pathway_end;

	Vector3 entry;
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
	Vector3 point;
	Vector3 normal;
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

} // namespace Nav3D

class NavIteration3D final : public RefCounted
{
public:
	bool enabled = true;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	NavigationEnums3D::PathSegmentType owner_type = NavigationEnums3D::PATH_SEGMENT_TYPE_REGION;
	RID owner_rid;
	bool owner_use_edge_connections = false;
	LocalVector<Nav3D::Polygon> navmesh_polygons;
	LocalVector<LocalVector<Nav3D::Connection>> internal_connections;

	// Region iteration data.
	Transform3D transform;
	real_t surface_area = 0.0;
	AABB bounds;
	LocalVector<Nav3D::ConnectableEdge> external_edges;

	// Link iteration data.
	bool bidirectional = true;
	Vector3 start_position;
	Vector3 end_position;

	bool get_enabled() const { return enabled; }
	NavigationEnums3D::PathSegmentType get_type() const { return owner_type; }
	RID get_self() const { return owner_rid; }
	uint32_t get_navigation_layers() const { return navigation_layers; }
	real_t get_enter_cost() const { return enter_cost; }
	real_t get_travel_cost() const { return travel_cost; }
	bool get_use_edge_connections() const { return owner_use_edge_connections; }

	const LocalVector<Nav3D::Polygon>& get_navmesh_polygons() const { return navmesh_polygons; }
	const LocalVector<LocalVector<Nav3D::Connection>>& get_internal_connections() const
	{
		return internal_connections;
	}

	const Transform3D& get_transform() const { return transform; }
	real_t get_surface_area() const { return surface_area; }
	AABB get_bounds() const { return bounds; }
	const LocalVector<Nav3D::ConnectableEdge>& get_external_edges() const { return external_edges; }

	Vector3 get_start_position() const { return start_position; }
	Vector3 get_end_position() const { return end_position; }
	bool is_bidirectional() const { return bidirectional; }

	~NavIteration3D() override
	{
		external_edges.clear();
		navmesh_polygons.clear();
		internal_connections.clear();
	}
};

struct NavRegionIterationBuild3D
{
	Nav3D::PerformanceData performance_data;

	NavRegion3D* region = nullptr;

	Vector3 map_cell_size;
	Transform3D region_transform;

	struct NavMeshData
	{
		Vector<Vector3> vertices;
		Vector<Vector<int>> polygons;

		void clear()
		{
			vertices.clear();
			polygons.clear();
		}
	} navmesh_data;

	Ref<NavIteration3D> region_iteration;

	HashMap<Nav3D::EdgeKey, Nav3D::EdgeConnectionPair, Nav3D::EdgeKey> iter_connection_pairs_map;

	void reset()
	{
		performance_data.reset();

		navmesh_data.clear();
		region_iteration = Ref<NavIteration3D>();
		iter_connection_pairs_map.clear();
	}
};

class NavMeshQueries3D final
{
public:
	struct PathQuerySlot
	{
		LocalVector<Nav3D::NavigationPoly> path_corridor;
		Heap<Nav3D::NavigationPoly*, Nav3D::NavPolyTravelCostGreaterThan, Nav3D::NavPolyHeapIndexer>
			traversable_polys;
		bool in_use = false;
		uint32_t slot_index = 0;
		AHashMap<const Nav3D::Polygon*, uint32_t> poly_to_id;
	};

	struct NavMeshPathQueryTask3D
	{
		enum TaskStatus
		{
			QUERY_STARTED,
			QUERY_FINISHED,
			QUERY_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Vector3 start_position;
		Vector3 target_position;
		uint32_t navigation_layers;
		uint32_t metadata_flags = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_ALL;
		NavigationEnums3D::PathfindingAlgorithm pathfinding_algorithm =
			NavigationEnums3D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
		NavigationEnums3D::PathPostProcessing path_postprocessing =
			NavigationEnums3D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
		bool simplify_path = false;
		real_t simplify_epsilon = 0.0;

		bool exclude_regions = false;
		bool include_regions = false;
		LocalVector<RID> excluded_regions;
		LocalVector<RID> included_regions;

		float path_return_max_length = 0.0;
		float path_return_max_radius = 0.0;
		int path_search_max_polygons = NavigationDefaults3D::path_search_max_polygons;
		float path_search_max_distance = 0.0;

		Vector3 begin_position;
		Vector3 end_position;
		const Nav3D::Polygon* begin_polygon = nullptr;
		const Nav3D::Polygon* end_polygon = nullptr;
		uint32_t least_cost_id = 0;

		Vector3 map_up;
		NavMap3D* map = nullptr;
		PathQuerySlot* path_query_slot = nullptr;

		LocalVector<Vector3> path_points;
		LocalVector<int32_t> path_meta_point_types;
		LocalVector<RID> path_meta_point_rids;
		LocalVector<int64_t> path_meta_point_owners;
		float path_length = 0.0;

		Ref<NavigationPathQueryParameters3D> query_parameters;
		Ref<NavigationPathQueryResult3D> query_result;
		NavMeshPathQueryTask3D::TaskStatus status =
			NavMeshPathQueryTask3D::TaskStatus::QUERY_STARTED;

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

	static Vector3 polygons_get_random_point(const LocalVector<Nav3D::Polygon>& p_polygons,
		uint32_t p_navigation_layers, bool p_uniformly);

	static Vector3 polygons_get_closest_point_to_segment(
		const LocalVector<Nav3D::Polygon>& p_polygons, const Vector3& p_from, const Vector3& p_to,
		const bool p_use_collision);
	static Vector3 polygons_get_closest_point(
		const LocalVector<Nav3D::Polygon>& p_polygons, const Vector3& p_point);
	static Vector3 polygons_get_closest_point_normal(
		const LocalVector<Nav3D::Polygon>& p_polygons, const Vector3& p_point);
	static Nav3D::ClosestPointQueryResult polygons_get_closest_point_info(
		const LocalVector<Nav3D::Polygon>& p_polygons, const Vector3& p_point);
	static RID polygons_get_closest_point_owner(
		const LocalVector<Nav3D::Polygon>& p_polygons, const Vector3& p_point);

	static Vector3 map_iteration_get_closest_point_to_segment(
		const NavMapIteration3D& p_map_iteration, const Vector3& p_from, const Vector3& p_to,
		const bool p_use_collision);
	static Vector3 map_iteration_get_closest_point(
		const NavMapIteration3D& p_map_iteration, const Vector3& p_point);
	static Vector3 map_iteration_get_closest_point_normal(
		const NavMapIteration3D& p_map_iteration, const Vector3& p_point);
	static RID map_iteration_get_closest_point_owner(
		const NavMapIteration3D& p_map_iteration, const Vector3& p_point);
	static Nav3D::ClosestPointQueryResult map_iteration_get_closest_point_info(
		const NavMapIteration3D& p_map_iteration, const Vector3& p_point);
	static Vector3 map_iteration_get_random_point(
		const NavMapIteration3D& p_map_iteration, uint32_t p_navigation_layers, bool p_uniformly);

	static void query_task_map_iteration_get_path(
		NavMeshPathQueryTask3D& p_query_task, const NavMapIteration3D& p_map_iteration);
	static void _query_task_push_back_point_with_metadata(NavMeshPathQueryTask3D& p_query_task,
		const Vector3& p_point, const Nav3D::Polygon* p_point_polygon);
	static void _query_task_find_start_end_positions(
		NavMeshPathQueryTask3D& p_query_task, const NavMapIteration3D& p_map_iteration);
	static void _query_task_build_path_corridor(
		NavMeshPathQueryTask3D& p_query_task, const NavMapIteration3D& p_map_iteration);
	static void _query_task_post_process_corridorfunnel(NavMeshPathQueryTask3D& p_query_task);
	static void _query_task_post_process_edgecentered(NavMeshPathQueryTask3D& p_query_task);
	static void _query_task_post_process_nopostprocessing(NavMeshPathQueryTask3D& p_query_task);
	static void _query_task_clip_path(NavMeshPathQueryTask3D& p_query_task,
		const Nav3D::NavigationPoly* from_poly, const Vector3& p_to_point,
		const Nav3D::NavigationPoly* p_to_poly);
	static void _query_task_simplified_path_points(NavMeshPathQueryTask3D& p_query_task);
	static bool _query_task_is_connection_owner_usable(
		const NavMeshPathQueryTask3D& p_query_task, const NavIteration3D* p_owner);
	static void _query_task_process_path_result_limits(NavMeshPathQueryTask3D& p_query_task);

	static void _query_task_search_polygon_connections(NavMeshPathQueryTask3D& p_query_task,
		const Nav3D::Connection& p_connection, uint32_t p_least_cost_id,
		const Nav3D::NavigationPoly& p_least_cost_poly, real_t p_poly_enter_cost,
		const Vector3& p_end_point);

	static void simplify_path_segment(int p_start_inx, int p_end_inx,
		const LocalVector<Vector3>& p_points, real_t p_epsilon,
		LocalVector<uint32_t>& r_simplified_path_indices);
	static LocalVector<uint32_t> get_simplified_path_indices(
		const LocalVector<Vector3>& p_path, real_t p_epsilon);

	static float _calculate_path_length(
		const LocalVector<Vector3>& p_path, uint32_t p_start_index, uint32_t p_end_index);
};

struct NavMapIterationBuild3D
{
	Vector3 merge_rasterizer_cell_size;
	bool use_edge_connections = true;
	real_t edge_connection_margin;
	real_t link_connection_radius;
	Nav3D::PerformanceData performance_data;
	int polygon_count = 0;
	int free_edge_count = 0;

	HashMap<Nav3D::EdgeKey, Nav3D::EdgeConnectionPair, Nav3D::EdgeKey> iter_connection_pairs_map;
	LocalVector<Nav3D::Connection> iter_free_edges;

	NavMapIteration3D* map_iteration = nullptr;

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

struct NavMapIteration3D
{
	mutable SafeNumeric<uint32_t> users;
	RWLock rwlock;

	Vector3 map_up;

	LocalVector<Ref<NavIteration3D>> region_iterations;
	LocalVector<Ref<NavIteration3D>> link_iterations;

	int navmesh_polygon_count = 0;

	HashMap<const NavIteration3D*, LocalVector<Nav3D::Connection>> external_region_connections;
	HashMap<const NavIteration3D*, LocalVector<LocalVector<Nav3D::Connection>>>
		navbases_polygons_external_connections;

	LocalVector<Nav3D::Polygon> navlink_polygons;

	HashMap<NavRegion3D*, Ref<NavIteration3D>> region_ptr_to_region_iteration;

	LocalVector<NavMeshQueries3D::PathQuerySlot> path_query_slots;
	Mutex path_query_slots_mutex;
	Semaphore path_query_slots_semaphore;

	void clear()
	{
		map_up = Vector3();
		navmesh_polygon_count = 0;

		region_iterations.clear();
		link_iterations.clear();
		external_region_connections.clear();
		navbases_polygons_external_connections.clear();
		navlink_polygons.clear();
		region_ptr_to_region_iteration.clear();
	}
};

class NavMapIterationRead3D final
{
	const NavMapIteration3D& map_iteration;

public:
	_ALWAYS_INLINE_ NavMapIterationRead3D(const NavMapIteration3D& p_iteration)
		: map_iteration(p_iteration)
	{
		map_iteration.rwlock.read_lock();
		map_iteration.users.increment();
	}

	_ALWAYS_INLINE_ ~NavMapIterationRead3D()
	{
		map_iteration.users.decrement();
		map_iteration.rwlock.read_unlock();
	}
};

class NavMapBuilder3D final
{
	static void _build_step_gather_region_polygons(NavMapIterationBuild3D& r_build);
	static void _build_step_find_edge_connection_pairs(NavMapIterationBuild3D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavMapIterationBuild3D& r_build);
	static void _build_step_edge_connection_margin_connections(NavMapIterationBuild3D& r_build);
	static void _build_step_navlink_connections(NavMapIterationBuild3D& r_build);
	static void _build_update_map_iteration(NavMapIterationBuild3D& r_build);

public:
	static Nav3D::PointKey get_point_key(const Vector3& p_pos, const Vector3& p_cell_size);

	static void build_navmap_iteration(NavMapIterationBuild3D& r_build);
};

class NavRegionBuilder3D final
{
	static void _build_step_process_navmesh_data(NavRegionIterationBuild3D& r_build);
	static void _build_step_find_edge_connection_pairs(NavRegionIterationBuild3D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavRegionIterationBuild3D& r_build);
	static void _build_update_iteration(NavRegionIterationBuild3D& r_build);

public:
	static Nav3D::PointKey get_point_key(const Vector3& p_pos, const Vector3& p_cell_size);
	static Nav3D::EdgeKey get_edge_key(
		const Vector3& p_vertex1, const Vector3& p_vertex2, const Vector3& p_cell_size);

	static void build_iteration(NavRegionIterationBuild3D& r_build);
};

class NavAgent3D final
{
	RID self;

	Vector3 position;
	Vector3 target_position;
	Vector3 velocity;
	Vector3 velocity_forced;
	real_t height = NavigationDefaults3D::AVOIDANCE_AGENT_HEIGHT;
	real_t radius = NavigationDefaults3D::AVOIDANCE_AGENT_RADIUS;
	real_t max_speed = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t time_horizon_agents = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	int max_neighbors = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t neighbor_distance = NavigationDefaults3D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	Vector3 safe_velocity;
	bool clamp_speed = true;

	NavMap3D* map = nullptr;

	RVO2D::Agent2D rvo_agent_2d;
	RVO3D::Agent3D rvo_agent_3d;
	bool use_3d_avoidance = false;
	bool avoidance_enabled = false;

	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;

	bool agent_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavAgent3D> sync_dirty_request_list_element;

public:
	NavAgent3D();
	~NavAgent3D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_use_3d_avoidance(bool p_enabled);
	bool get_use_3d_avoidance() { return use_3d_avoidance; }

	void set_map(NavMap3D* p_map);
	NavMap3D* get_map() { return map; }

	bool is_map_changed();

	RVO2D::Agent2D* get_rvo_agent_2d() { return &rvo_agent_2d; }
	RVO3D::Agent3D* get_rvo_agent_3d() { return &rvo_agent_3d; }

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

	void set_height(real_t p_height);
	real_t get_height() const { return height; }

	void set_max_speed(real_t p_max_speed);
	real_t get_max_speed() const { return max_speed; }

	void set_position(const Vector3 p_position);
	const Vector3& get_position() const { return position; }

	void set_target_position(const Vector3 p_target_position);
	const Vector3& get_target_position() const { return target_position; }

	void set_velocity(const Vector3 p_velocity);
	const Vector3& get_velocity() const { return velocity; }

	void set_velocity_forced(const Vector3 p_velocity);
	const Vector3& get_velocity_forced() const { return velocity_forced; }

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

class NavObstacle3D final
{
	RID self;

	NavAgent3D* agent = nullptr;
	NavMap3D* map = nullptr;
	Vector3 velocity;
	Vector3 position;
	Vector<Vector3> vertices;

	real_t radius = 0.0;
	real_t height = 0.0;

	bool avoidance_enabled = false;
	bool use_3d_avoidance = false;
	uint32_t avoidance_layers = 1;

	bool obstacle_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavObstacle3D> sync_dirty_request_list_element;

public:
	NavObstacle3D();
	~NavObstacle3D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_use_3d_avoidance(bool p_enabled);
	bool get_use_3d_avoidance() { return use_3d_avoidance; }

	void set_map(NavMap3D* p_map);
	NavMap3D* get_map() { return map; }

	void set_agent(NavAgent3D* p_agent);
	NavAgent3D* get_agent() { return agent; }

	void set_position(const Vector3 p_position);
	const Vector3& get_position() const { return position; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_height(const real_t p_height);
	real_t get_height() const { return height; }

	void set_velocity(const Vector3 p_velocity);
	const Vector3& get_velocity() const { return velocity; }

	void set_vertices(const Vector<Vector3>& p_vertices);
	const Vector<Vector3>& get_vertices() const { return vertices; }

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

class NavLink3D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	NavMap3D* map = nullptr;
	bool bidirectional = true;
	Vector3 start_position;
	Vector3 end_position;
	bool enabled = true;

	SelfList<NavLink3D> sync_dirty_request_list_element;

	uint32_t iteration_id = 0;

	mutable RWLock iteration_rwlock;
	Ref<NavIteration3D> iteration;

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavLink3D();
	~NavLink3D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums3D::PathSegmentType get_type() const
	{
		return NavigationEnums3D::PATH_SEGMENT_TYPE_LINK;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_map(NavMap3D* p_map);
	NavMap3D* get_map() const { return map; }

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_bidirectional(bool p_bidirectional);
	bool is_bidirectional() const { return bidirectional; }

	void set_start_position(Vector3 p_position);
	Vector3 get_start_position() const { return start_position; }

	void set_end_position(Vector3 p_position);
	Vector3 get_end_position() const { return end_position; }

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const { return travel_cost; }

	bool sync();
	void request_sync();
	void cancel_sync_request();

	Ref<NavIteration3D> get_iteration();
};

class NavRegion3D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	RWLock region_rwlock;

	NavMap3D* map = nullptr;
	Transform3D transform;
	bool enabled = true;

	bool use_edge_connections = true;

	AABB bounds;

	Ref<NavigationMesh> navmesh;

	Nav3D::PerformanceData performance_data;

	uint32_t iteration_id = 0;

	SelfList<NavRegion3D> sync_dirty_request_list_element{this};
	mutable RWLock iteration_rwlock;
	Ref<NavIteration3D> iteration;

	NavRegionIterationBuild3D iteration_build;
	bool use_async_iterations = true;
	SelfList<NavRegion3D> async_list_element{this};
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavRegion3D();
	~NavRegion3D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums3D::PathSegmentType get_type() const
	{
		return NavigationEnums3D::PATH_SEGMENT_TYPE_REGION;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void scratch_polygons();

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_map(NavMap3D* p_map);
	NavMap3D* get_map() const { return map; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_transform(Transform3D transform);
	const Transform3D& get_transform() const { return transform; }

	void set_navigation_mesh(Ref<NavigationMesh> p_navigation_mesh);
	Ref<NavigationMesh> get_navigation_mesh() const { return navmesh; }

	const LocalVector<Nav3D::Polygon>& get_polygons() const;

	Vector3 get_closest_point_to_segment(
		const Vector3& p_from, const Vector3& p_to, bool p_use_collision) const;
	Nav3D::ClosestPointQueryResult get_closest_point_info(const Vector3& p_point) const;
	Vector3 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	real_t get_surface_area() const;
	AABB get_bounds() const;

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

	Ref<NavIteration3D> get_iteration();

	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;
};

class NavMap3D final
{
	RID self;

	Vector3 up = Vector3(0, 1, 0);

	real_t cell_size = NavigationDefaults3D::NAV_MESH_CELL_SIZE;
	real_t cell_height = NavigationDefaults3D::NAV_MESH_CELL_HEIGHT;

	Vector3 merge_rasterizer_cell_size = Vector3(cell_size, cell_height, cell_size);
	float merge_rasterizer_cell_scale = 0.1;

	bool use_edge_connections = true;
	real_t edge_connection_margin = NavigationDefaults3D::EDGE_CONNECTION_MARGIN;
	real_t link_connection_radius = NavigationDefaults3D::LINK_CONNECTION_RADIUS;

	bool map_settings_dirty = true;

	LocalVector<NavRegion3D*> regions;
	LocalVector<NavLink3D*> links;

	RVO2D::RVOSimulator2D rvo_simulation_2d;
	RVO3D::RVOSimulator3D rvo_simulation_3d;

	LocalVector<NavAgent3D*> active_2d_avoidance_agents;
	LocalVector<NavAgent3D*> active_3d_avoidance_agents;

	bool agents_dirty = true;
	LocalVector<NavAgent3D*> agents;

	LocalVector<NavObstacle3D*> obstacles;
	bool obstacles_dirty = true;

	uint32_t iteration_id = 0;

	bool use_threads = true;
	bool avoidance_use_multiple_threads = true;
	bool avoidance_use_high_priority_threads = true;

	Nav3D::PerformanceData performance_data;

	struct
	{
		struct
		{
			RWLock rwlock;
			SelfList<NavRegion3D>::List list;
		} regions;

		struct
		{
			RWLock rwlock;
			SelfList<NavLink3D>::List list;
		} links;

		struct
		{
			RWLock rwlock;
			SelfList<NavAgent3D>::List list;
		} agents;

		struct
		{
			RWLock rwlock;
			SelfList<NavObstacle3D>::List list;
		} obstacles;
	} sync_dirty_requests;

	struct
	{
		struct
		{
			RWLock rwlock;
			SelfList<NavRegion3D>::List list;
		} regions;
	} async_dirty_requests;

	int path_query_slots_max = 4;

	bool use_async_iterations = true;

	uint32_t iteration_slot_index = 0;
	LocalVector<NavMapIteration3D> iteration_slots;
	mutable RWLock iteration_slot_rwlock;

	NavMapIterationBuild3D iteration_build;
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavMap3D();
	~NavMap3D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_up(Vector3 p_up);
	Vector3 get_up() const { return up; }

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const { return cell_size; }

	void set_cell_height(real_t p_cell_height);
	real_t get_cell_height() const { return cell_height; }

	void set_merge_rasterizer_cell_scale(float p_value);
	float get_merge_rasterizer_cell_scale() const { return merge_rasterizer_cell_scale; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_edge_connection_margin(real_t p_edge_connection_margin);
	real_t get_edge_connection_margin() const { return edge_connection_margin; }

	void set_link_connection_radius(real_t p_link_connection_radius);
	real_t get_link_connection_radius() const { return link_connection_radius; }

	Nav3D::PointKey get_point_key(const Vector3& p_pos) const;
	const Vector3& get_merge_rasterizer_cell_size() const;

	void query_path(NavMeshQueries3D::NavMeshPathQueryTask3D& p_query_task);

	Vector3 get_closest_point_to_segment(
		const Vector3& p_from, const Vector3& p_to, const bool p_use_collision) const;
	Vector3 get_closest_point(const Vector3& p_point) const;
	Vector3 get_closest_point_normal(const Vector3& p_point) const;
	Nav3D::ClosestPointQueryResult get_closest_point_info(const Vector3& p_point) const;
	RID get_closest_point_owner(const Vector3& p_point) const;

	void add_region(NavRegion3D* p_region);
	void remove_region(NavRegion3D* p_region);
	const LocalVector<NavRegion3D*>& get_regions() const { return regions; }

	void add_link(NavLink3D* p_link);
	void remove_link(NavLink3D* p_link);
	const LocalVector<NavLink3D*>& get_links() const { return links; }

	bool has_agent(NavAgent3D* agent) const;
	void add_agent(NavAgent3D* agent);
	void remove_agent(NavAgent3D* agent);
	const LocalVector<NavAgent3D*>& get_agents() const { return agents; }

	void set_agent_as_controlled(NavAgent3D* agent);
	void remove_agent_as_controlled(NavAgent3D* agent);

	bool has_obstacle(NavObstacle3D* obstacle) const;
	void add_obstacle(NavObstacle3D* obstacle);
	void remove_obstacle(NavObstacle3D* obstacle);
	const LocalVector<NavObstacle3D*>& get_obstacles() const { return obstacles; }

	Vector3 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

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

	int get_region_connections_count(NavRegion3D* p_region) const;
	Vector3 get_region_connection_pathway_start(NavRegion3D* p_region, int p_connection_id) const;
	Vector3 get_region_connection_pathway_end(NavRegion3D* p_region, int p_connection_id) const;

	void add_region_async_thread_join_request(SelfList<NavRegion3D>* p_async_request);
	void remove_region_async_thread_join_request(SelfList<NavRegion3D>* p_async_request);

	void add_region_sync_dirty_request(SelfList<NavRegion3D>* p_sync_request);
	void add_link_sync_dirty_request(SelfList<NavLink3D>* p_sync_request);
	void add_agent_sync_dirty_request(SelfList<NavAgent3D>* p_sync_request);
	void add_obstacle_sync_dirty_request(SelfList<NavObstacle3D>* p_sync_request);

	void remove_region_sync_dirty_request(SelfList<NavRegion3D>* p_sync_request);
	void remove_link_sync_dirty_request(SelfList<NavLink3D>* p_sync_request);
	void remove_agent_sync_dirty_request(SelfList<NavAgent3D>* p_sync_request);
	void remove_obstacle_sync_dirty_request(SelfList<NavObstacle3D>* p_sync_request);

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;

private:
	void _sync_dirty_map_update_requests();
	void _sync_dirty_avoidance_update_requests();
	void _sync_async_tasks();

	void compute_single_step(uint32_t index, NavAgent3D** agent);

	void compute_single_avoidance_step_2d(uint32_t index, NavAgent3D** agent);
	void compute_single_avoidance_step_3d(uint32_t index, NavAgent3D** agent);

	void _sync_avoidance();
	void _update_rvo_simulation();
	void _update_rvo_obstacles_tree_2d();
	void _update_rvo_agents_tree_2d();
	void _update_rvo_agents_tree_3d();

	void _update_merge_rasterizer_cell_dimensions();
};

class Node;
class NavigationMeshSourceGeometryData3D;

class NavMeshGenerator3D final
{
	static inline bool _initialized = false;

	static Mutex baking_navmesh_mutex;
	static Mutex generator_task_mutex;

	static RWLock generator_parsers_rwlock;

	static bool use_threads;
	static bool baking_use_multiple_threads;
	static bool baking_use_high_priority_threads;

public:
	enum NavMeshBakeState
	{
		BAKE_STATE_NONE,
		BAKE_STATE_CONFIGURATION,
		BAKE_STATE_CALC_GRID_SIZE,
		BAKE_STATE_CREATE_HEIGHTFIELD,
		BAKE_STATE_MARK_WALKABLE_TRIANGLES,
		BAKE_STATE_CONSTRUCT_COMPACT_HEIGHTFIELD,
		BAKE_STATE_ERODE_WALKABLE_AREA,
		BAKE_STATE_SAMPLE_PARTITIONING,
		BAKE_STATE_CREATING_CONTOURS,
		BAKE_STATE_CREATING_POLYMESH,
		BAKE_STATE_CONVERTING_NATIVE_NAVMESH,
		BAKE_STATE_BAKE_CLEANUP,
		BAKE_STATE_BAKE_FINISHED,
		BAKE_STATE_MAX,
	};

private:
	struct NavMeshGeneratorTask3D
	{
		enum TaskStatus
		{
			BAKING_STARTED,
			BAKING_FINISHED,
			BAKING_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Ref<NavigationMesh> navigation_mesh;
		Ref<NavigationMeshSourceGeometryData3D> source_geometry_data;
		NavMeshGeneratorTask3D::TaskStatus status =
			NavMeshGeneratorTask3D::TaskStatus::BAKING_STARTED;

		NavMeshBakeState bake_state = NavMeshBakeState::BAKE_STATE_NONE;
	};

	static void generator_thread_bake(void* p_arg);

	static HashMap<Ref<NavigationMesh>, NavMeshGeneratorTask3D*> baking_navmeshes;

	static void generator_parse_geometry_node(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node,
		bool p_recurse_children);
	static void generator_parse_source_geometry_data(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_root_node);
	static void generator_bake_from_source_geometry_data(NavMeshGeneratorTask3D* p_generator_task);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return _initialized; }

	static void sync();
	static void cleanup();
	static void finish();

	static bool is_baking(Ref<NavigationMesh> p_navigation_mesh);
	static String get_baking_state_msg(Ref<NavigationMesh> p_navigation_mesh);

	NavMeshGenerator3D() = delete;
	NavMeshGenerator3D(const NavMeshGenerator3D&) = delete;
	NavMeshGenerator3D& operator=(const NavMeshGenerator3D&) = delete;
	~NavMeshGenerator3D() = delete;
};

#ifndef DISABLE_DEPRECATED
class NavigationMeshGenerator final
{
	static inline bool _initialized = false;

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return _initialized; }

	static void bake(const Ref<NavigationMesh>& p_navigation_mesh, Node* p_root_node);
	static void clear(Ref<NavigationMesh> p_navigation_mesh);

	NavigationMeshGenerator() = delete;
	NavigationMeshGenerator(const NavigationMeshGenerator&) = delete;
	NavigationMeshGenerator& operator=(const NavigationMeshGenerator&) = delete;
	~NavigationMeshGenerator() = delete;
};
#endif // DISABLE_DEPRECATED
