/**************************************************************************/
/*  nav_map_2d.cpp                                                        */
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

#include "nav_map_2d.h"

#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "core/os/os.h"
#include "core/templates/rb_map.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "thirdparty/rvo2/rvo2_2d/Obstacle2d.h"

#ifdef CLIPPER2_ENABLED
#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include <thirdparty/misc/polypartition.h>
#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"

Mutex NavMeshGenerator2D::baking_navmesh_mutex;
Mutex NavMeshGenerator2D::generator_task_mutex;
RWLock NavMeshGenerator2D::generator_parsers_rwlock;
bool NavMeshGenerator2D::use_threads = true;
bool NavMeshGenerator2D::baking_use_multiple_threads = true;
bool NavMeshGenerator2D::baking_use_high_priority_threads = true;
HashSet<Ref<NavigationPolygon>> NavMeshGenerator2D::baking_navmeshes;
LocalVector<NavMeshGeometryParser2D*> NavMeshGenerator2D::generator_parsers;

void NavMeshGenerator2D::initialize()
{
	_initialized = true;
}

void NavMeshGenerator2D::finalize()
{
	if (!_initialized) {
		return;
	}
	cleanup();
	_initialized = false;
}

void NavMeshGenerator2D::finish() { cleanup(); }

bool NavMeshGenerator2D::is_baking(Ref<NavigationPolygon> p_navigation_polygon)
{
	MutexLock baking_navmesh_lock(baking_navmesh_mutex);
	return baking_navmeshes.has(p_navigation_polygon);
}

void NavMeshGenerator2D::generator_thread_bake(void* p_arg)
{
	NavMeshGeneratorTask2D* generator_task = static_cast<NavMeshGeneratorTask2D*>(p_arg);

	generator_bake_from_source_geometry_data(
		generator_task->navigation_mesh, generator_task->source_geometry_data);

	generator_task->status = NavMeshGeneratorTask2D::TaskStatus::BAKING_FINISHED;
}

void NavMeshGenerator2D::set_generator_parsers(
	const LocalVector<NavMeshGeometryParser2D*>& p_parsers)
{
	RWLockWrite write_lock(generator_parsers_rwlock);
	generator_parsers = p_parsers;
}

static void generator_recursive_process_polytree_items(
	List<TPPLPoly>& p_tppl_in_polygon, const Clipper2Lib::PolyPathD* p_polypath_item)
{
	using namespace Clipper2Lib;

	TPPLPoly tp;
	int size = p_polypath_item->Polygon().size();
	tp.Init(size);

	int j = 0;
	for (const PointD& polypath_point : p_polypath_item->Polygon()) {
		tp[j] =
			Vector2(static_cast<real_t>(polypath_point.x), static_cast<real_t>(polypath_point.y));
		++j;
	}

	if (p_polypath_item->IsHole()) {
		tp.SetOrientation(TPPL_ORIENTATION_CW);
		tp.SetHole(true);
	}
	else {
		tp.SetOrientation(TPPL_ORIENTATION_CCW);
	}
	p_tppl_in_polygon.push_back(tp);

	for (size_t i = 0; i < p_polypath_item->Count(); i++) {
		const PolyPathD* polypath_item = p_polypath_item->Child(i);
		generator_recursive_process_polytree_items(p_tppl_in_polygon, polypath_item);
	}
}

void NavMeshGenerator2D::generator_bake_from_source_geometry_data(
	Ref<NavigationPolygon> p_navigation_mesh,
	Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data)
{
	if (p_navigation_mesh.is_null() || p_source_geometry_data.is_null()) {
		return;
	}

	using namespace Clipper2Lib;
	PathsD traversable_polygon_paths;
	PathsD obstruction_polygon_paths;
	bool empty_projected_obstructions = true;
	{
		RWLockRead read_lock(p_source_geometry_data->geometry_rwlock);

		const Vector<Vector<Vector2>>& traversable_outlines =
			p_source_geometry_data->traversable_outlines;
		int outline_count = p_navigation_mesh->get_outline_count();

		if (outline_count == 0 &&
			(!p_source_geometry_data->has_data() || (traversable_outlines.is_empty()))) {
			return;
		}

		const Vector<Vector<Vector2>>& obstruction_outlines =
			p_source_geometry_data->obstruction_outlines;
		const Vector<NavigationMeshSourceGeometryData2D::ProjectedObstruction>&
			projected_obstructions = p_source_geometry_data->_projected_obstructions;

		traversable_polygon_paths.reserve(outline_count + traversable_outlines.size());
		obstruction_polygon_paths.reserve(obstruction_outlines.size());

		for (int i = 0; i < outline_count; i++) {
			const Vector<Vector2>& traversable_outline = p_navigation_mesh->get_outline(i);
			PathD subject_path;
			subject_path.reserve(traversable_outline.size());
			for (const Vector2& traversable_point : traversable_outline) {
				subject_path.emplace_back(traversable_point.x, traversable_point.y);
			}
			traversable_polygon_paths.push_back(std::move(subject_path));
		}

		for (const Vector<Vector2>& traversable_outline : traversable_outlines) {
			PathD subject_path;
			subject_path.reserve(traversable_outline.size());
			for (const Vector2& traversable_point : traversable_outline) {
				subject_path.emplace_back(traversable_point.x, traversable_point.y);
			}
			traversable_polygon_paths.push_back(std::move(subject_path));
		}

		empty_projected_obstructions = projected_obstructions.is_empty();
		if (!empty_projected_obstructions) {
			for (const NavigationMeshSourceGeometryData2D::ProjectedObstruction&
					 projected_obstruction : projected_obstructions) {
				if (projected_obstruction.carve) {
					continue;
				}
				if (projected_obstruction.vertices.is_empty() ||
					projected_obstruction.vertices.size() % 2 != 0) {
					continue;
				}

				PathD clip_path;
				clip_path.reserve(projected_obstruction.vertices.size() / 2);
				for (int i = 0; i < projected_obstruction.vertices.size() / 2; i++) {
					clip_path.emplace_back(projected_obstruction.vertices[i * 2],
						projected_obstruction.vertices[i * 2 + 1]);
				}
				if (!IsPositive(clip_path)) {
					std::reverse(clip_path.begin(), clip_path.end());
				}
				obstruction_polygon_paths.push_back(std::move(clip_path));
			}
		}

		for (const Vector<Vector2>& obstruction_outline : obstruction_outlines) {
			PathD clip_path;
			clip_path.reserve(obstruction_outline.size());
			for (const Vector2& obstruction_point : obstruction_outline) {
				clip_path.emplace_back(obstruction_point.x, obstruction_point.y);
			}
			obstruction_polygon_paths.push_back(std::move(clip_path));
		}
	}

	Rect2 baking_rect = p_navigation_mesh->get_baking_rect();
	if (baking_rect.has_area()) {
		Vector2 baking_rect_offset = p_navigation_mesh->get_baking_rect_offset();

		const int rect_begin_x = baking_rect.position[0] + baking_rect_offset.x;
		const int rect_begin_y = baking_rect.position[1] + baking_rect_offset.y;
		const int rect_end_x = baking_rect.position[0] + baking_rect.size[0] + baking_rect_offset.x;
		const int rect_end_y = baking_rect.position[1] + baking_rect.size[1] + baking_rect_offset.y;

		RectD clipper_rect = RectD(rect_begin_x, rect_begin_y, rect_end_x, rect_end_y);

		traversable_polygon_paths = RectClip(clipper_rect, traversable_polygon_paths);
		obstruction_polygon_paths = RectClip(clipper_rect, obstruction_polygon_paths);
	}

	// first merge all traversable polygons according to user specified fill rule
	PathsD dummy_clip_path;
	traversable_polygon_paths =
		Union(traversable_polygon_paths, dummy_clip_path, FillRule::NonZero);
	// merge all obstruction polygons, don't allow holes for what is considered "solid" 2D geometry
	obstruction_polygon_paths =
		Union(obstruction_polygon_paths, dummy_clip_path, FillRule::NonZero);

	PathsD path_solution =
		Difference(traversable_polygon_paths, obstruction_polygon_paths, FillRule::NonZero);

	real_t agent_radius_offset = p_navigation_mesh->get_agent_radius();
	if (agent_radius_offset > 0.0) {
		path_solution =
			InflatePaths(path_solution, -agent_radius_offset, JoinType::Miter, EndType::Polygon);
	}

	// Apply obstructions that are not affected by agent radius, the ones with carve enabled.
	if (!empty_projected_obstructions) {
		RWLockRead read_lock(p_source_geometry_data->geometry_rwlock);
		const Vector<NavigationMeshSourceGeometryData2D::ProjectedObstruction>&
			projected_obstructions = p_source_geometry_data->_projected_obstructions;
		obstruction_polygon_paths.resize(0);
		for (const NavigationMeshSourceGeometryData2D::ProjectedObstruction& projected_obstruction :
			projected_obstructions) {
			if (!projected_obstruction.carve) {
				continue;
			}
			if (projected_obstruction.vertices.is_empty() ||
				projected_obstruction.vertices.size() % 2 != 0) {
				continue;
			}

			PathD clip_path;
			clip_path.reserve(projected_obstruction.vertices.size() / 2);
			for (int i = 0; i < projected_obstruction.vertices.size() / 2; i++) {
				clip_path.emplace_back(projected_obstruction.vertices[i * 2],
					projected_obstruction.vertices[i * 2 + 1]);
			}
			if (!IsPositive(clip_path)) {
				std::reverse(clip_path.begin(), clip_path.end());
			}
			obstruction_polygon_paths.push_back(std::move(clip_path));
		}
		if (obstruction_polygon_paths.size() > 0) {
			path_solution = Difference(path_solution, obstruction_polygon_paths, FillRule::NonZero);
		}
	}

	// path_solution = RamerDouglasPeucker(path_solution, 0.025); //

	real_t border_size = p_navigation_mesh->get_border_size();
	if (baking_rect.has_area() && border_size > 0.0) {
		Vector2 baking_rect_offset = p_navigation_mesh->get_baking_rect_offset();

		const double rect_begin_x = baking_rect.position[0] + baking_rect_offset.x + border_size;
		const double rect_begin_y = baking_rect.position[1] + baking_rect_offset.y + border_size;
		const double rect_end_x =
			baking_rect.position[0] + baking_rect.size[0] + baking_rect_offset.x - border_size;
		const double rect_end_y =
			baking_rect.position[1] + baking_rect.size[1] + baking_rect_offset.y - border_size;

		RectD clipper_rect = RectD(rect_begin_x, rect_begin_y, rect_end_x, rect_end_y);

		path_solution = RectClip(clipper_rect, path_solution);
	}

	if (path_solution.size() == 0) {
		p_navigation_mesh->clear();
		return;
	}

	ClipType clipper_cliptype = ClipType::Union;

	List<TPPLPoly> tppl_in_polygon, tppl_out_polygon;

	PolyTreeD polytree;
	ClipperD clipper_D;

	clipper_D.AddSubject(path_solution);
	clipper_D.Execute(clipper_cliptype, FillRule::NonZero, polytree);

	for (size_t i = 0; i < polytree.Count(); i++) {
		const PolyPathD* polypath_item = polytree[i];
		generator_recursive_process_polytree_items(tppl_in_polygon, polypath_item);
	}

	TPPLPartition tpart;

	NavigationPolygon::SamplePartitionType sample_partition_type =
		p_navigation_mesh->get_sample_partition_type();

	switch (sample_partition_type) {
	case NavigationPolygon::SamplePartitionType::SAMPLE_PARTITION_CONVEX_PARTITION:
		if (tpart.ConvexPartition_HM(&tppl_in_polygon, &tppl_out_polygon) == 0) {
			ERR_PRINT("NavigationPolygon polygon convex partition failed. Unable to create a valid "
					  "navigation mesh polygon layout from provided source geometry.");
			p_navigation_mesh->set_vertices(Vector<Vector2>());
			p_navigation_mesh->clear_polygons();
			return;
		}
		break;
	case NavigationPolygon::SamplePartitionType::SAMPLE_PARTITION_TRIANGULATE:
		if (tpart.Triangulate_EC(&tppl_in_polygon, &tppl_out_polygon) == 0) {
			ERR_PRINT("NavigationPolygon polygon triangulation failed. Unable to create a valid "
					  "navigation mesh polygon layout from provided source geometry.");
			p_navigation_mesh->set_vertices(Vector<Vector2>());
			p_navigation_mesh->clear_polygons();
			return;
		}
		break;
	default: {
		ERR_PRINT("NavigationPolygon polygon partitioning failed. Unrecognized partition type.");
		p_navigation_mesh->set_vertices(Vector<Vector2>());
		p_navigation_mesh->clear_polygons();
		return;
	}
	}

	Vector<Vector2> new_vertices;
	Vector<Vector<int>> new_polygons;

	HashMap<Vector2, int> points;
	for (const TPPLPoly& tp : tppl_out_polygon) {
		Vector<int> new_polygon;

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			HashMap<Vector2, int>::Iterator E = points.find(tp[i]);
			if (!E) {
				E = points.insert(tp[i], new_vertices.size());
				new_vertices.push_back(tp[i]);
			}
			new_polygon.push_back(E->value);
		}

		new_polygons.push_back(new_polygon);
	}

	p_navigation_mesh->set_data(new_vertices, new_polygons);
}

void NavMeshGenerator2D::sync() {}

void NavMeshGenerator2D::cleanup() {}

#endif // CLIPPER2_ENABLED

using namespace Nav2D;
using namespace NavigationDefaults2D;
using namespace NavigationEnums2D;

Vector2 Triangle2::get_random_point_inside() const {
	real_t a = Math::random(0.0, 1.0);
	real_t b = Math::random(0.0, 1.0);
	if (a > b) {
		SWAP(a, b);
	}

	return vertex[0] * a + vertex[1] * (b - a) + vertex[2] * (1.0f - b);
}

Vector2 Triangle2::get_closest_point_to(const Vector2 &p_point) const {
	Vector2 edge0 = vertex[1] - vertex[0];
	Vector2 edge1 = vertex[2] - vertex[0];
	Vector2 v0 = vertex[0] - p_point;

	real_t a = edge0.dot(edge0);
	real_t b = edge0.dot(edge1);
	real_t c = edge1.dot(edge1);
	real_t d = edge0.dot(v0);
	real_t e = edge1.dot(v0);

	real_t det = a * c - b * b;
	real_t s = b * e - c * d;
	real_t t = b * d - a * e;

	if (s + t < det) {
		if (s < 0.f) {
			if (t < 0.f) {
				if (d < 0.f) {
					s = CLAMP(-d / a, 0.f, 1.f);
					t = 0.f;
				} else {
					s = 0.f;
					t = CLAMP(-e / c, 0.f, 1.f);
				}
			} else {
				s = 0.f;
				t = CLAMP(-e / c, 0.f, 1.f);
			}
		} else if (t < 0.f) {
			s = CLAMP(-d / a, 0.f, 1.f);
			t = 0.f;
		} else {
			real_t inv_det = 1.f / det;
			s *= inv_det;
			t *= inv_det;
		}
	} else {
		if (s < 0.f) {
			real_t tmp0 = b + d;
			real_t tmp1 = c + e;
			if (tmp1 > tmp0) {
				real_t numer = tmp1 - tmp0;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				t = CLAMP(-e / c, 0.f, 1.f);
				s = 0.f;
			}
		} else if (t < 0.f) {
			if (a + d > b + e) {
				real_t numer = c + e - b - d;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				s = CLAMP(-d / a, 0.f, 1.f);
				t = 0.f;
			}
		} else {
			real_t numer = c + e - b - d;
			real_t denom = a - 2 * b + c;
			s = CLAMP(numer / denom, 0.f, 1.f);
			t = 1.f - s;
		}
	}

	return vertex[0] + s * edge0 + t * edge1;
}


void NavAgent2D::set_avoidance_enabled(bool p_enabled)
{
	avoidance_enabled = p_enabled;
	_update_rvo_agent_properties();
}

void NavAgent2D::_update_rvo_agent_properties()
{
	rvo_agent.neighborDist_ = neighbor_distance;
	rvo_agent.maxNeighbors_ = max_neighbors;
	rvo_agent.timeHorizon_ = time_horizon_agents;
	rvo_agent.timeHorizonObst_ = time_horizon_obstacles;
	rvo_agent.radius_ = radius;
	rvo_agent.maxSpeed_ = max_speed;
	rvo_agent.position_ = RVO2D::Vector2(position.x, position.y);
	// Replacing the internal velocity directly causes major jitter / bugs due to unpredictable
	// velocity jumps, left line here for testing.
	// rvo_agent.velocity_ = RVO2D::Vector2(velocity.x, velocity.y);
	rvo_agent.prefVelocity_ = RVO2D::Vector2(velocity.x, velocity.y);
	rvo_agent.avoidance_layers_ = avoidance_layers;
	rvo_agent.avoidance_mask_ = avoidance_mask;
	rvo_agent.avoidance_priority_ = avoidance_priority;

	if (map != nullptr) {
		if (avoidance_enabled) {
			map->set_agent_as_controlled(this);
		}
		else {
			map->remove_agent_as_controlled(this);
		}
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_map(NavMap2D* p_map)
{
	if (map == p_map) {
		return;
	}

	cancel_sync_request();

	if (map) {
		map->remove_agent(this);
	}

	map = p_map;
	agent_dirty = true;

	if (map) {
		map->add_agent(this);
		if (avoidance_enabled) {
			map->set_agent_as_controlled(this);
		}

		request_sync();
	}
}

bool NavAgent2D::is_map_changed()
{
	if (map) {
		bool is_changed = map->get_iteration_id() != last_map_iteration_id;
		last_map_iteration_id = map->get_iteration_id();
		return is_changed;
	}
	else {
		return false;
	}
}

void NavAgent2D::set_neighbor_distance(real_t p_neighbor_distance)
{
	neighbor_distance = p_neighbor_distance;
	rvo_agent.neighborDist_ = neighbor_distance;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_max_neighbors(int p_max_neighbors)
{
	max_neighbors = p_max_neighbors;
	rvo_agent.maxNeighbors_ = max_neighbors;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_time_horizon_agents(real_t p_time_horizon)
{
	time_horizon_agents = p_time_horizon;
	rvo_agent.timeHorizon_ = time_horizon_agents;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	time_horizon_obstacles = p_time_horizon;
	rvo_agent.timeHorizonObst_ = time_horizon_obstacles;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_radius(real_t p_radius)
{
	radius = p_radius;
	rvo_agent.radius_ = radius;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_max_speed(real_t p_max_speed)
{
	max_speed = p_max_speed;
	if (avoidance_enabled) {
		rvo_agent.maxSpeed_ = max_speed;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_position(const Vector2& p_position)
{
	position = p_position;
	if (avoidance_enabled) {
		rvo_agent.position_ = RVO2D::Vector2(p_position.x, p_position.y);
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_target_position(const Vector2& p_target_position)
{
	target_position = p_target_position;
}

void NavAgent2D::set_velocity(const Vector2& p_velocity)
{
	// Sets the "wanted" velocity for an agent as a suggestion
	// This velocity is not guaranteed, RVO simulation will only try to fulfill it
	velocity = p_velocity;
	if (avoidance_enabled) {
		rvo_agent.prefVelocity_ = RVO2D::Vector2(velocity.x, velocity.y);
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_velocity_forced(const Vector2& p_velocity)
{
	// This function replaces the internal rvo simulation velocity
	// should only be used after the agent was teleported
	// as it destroys consistency in movement in cramped situations
	// use velocity instead to update with a safer "suggestion"
	velocity_forced = p_velocity;
	if (avoidance_enabled) {
		rvo_agent.velocity_ = RVO2D::Vector2(p_velocity.x, p_velocity.y);
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::update()
{
	// Updates this agent with the calculated results from the rvo simulation
	if (avoidance_enabled) {
		velocity = Vector2(rvo_agent.velocity_.x(), rvo_agent.velocity_.y());
	}
}

void NavAgent2D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	rvo_agent.avoidance_mask_ = avoidance_mask;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	rvo_agent.avoidance_layers_ = avoidance_layers;
	agent_dirty = true;

	request_sync();
}

void NavAgent2D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	rvo_agent.avoidance_priority_ = avoidance_priority;
	agent_dirty = true;

	request_sync();
}

bool NavAgent2D::is_dirty() const { return agent_dirty; }

void NavAgent2D::sync() { agent_dirty = false; }

void NavAgent2D::set_paused(bool p_paused)
{
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;

	if (map) {
		if (paused) {
			map->remove_agent_as_controlled(this);
		}
		else if (avoidance_enabled) {
			map->set_agent_as_controlled(this);
		}
	}
}

bool NavAgent2D::get_paused() const { return paused; }

NavAgent2D::NavAgent2D() : sync_dirty_request_list_element(this) {}

NavAgent2D::~NavAgent2D() { cancel_sync_request(); }

void NavAgent2D::request_sync() {}


void NavAgent2D::cancel_sync_request() {}

bool NavAgent2D::has_avoidance_callback() const { return false; }

void NavAgent2D::dispatch_avoidance_callback() {}


void NavObstacle2D::set_agent(NavAgent2D* p_agent)
{
	if (agent == p_agent) {
		return;
	}

	agent = p_agent;

	internal_update_agent();

	request_sync();
}

void NavObstacle2D::set_avoidance_enabled(bool p_enabled)
{
	if (avoidance_enabled == p_enabled) {
		return;
	}

	avoidance_enabled = p_enabled;
	obstacle_dirty = true;

	internal_update_agent();

	request_sync();
}

void NavObstacle2D::set_map(NavMap2D* p_map)
{
	if (map == p_map) {
		return;
	}

	cancel_sync_request();

	if (map) {
		map->remove_obstacle(this);
		if (agent) {
			agent->set_map(nullptr);
		}
	}

	map = p_map;
	obstacle_dirty = true;

	if (map) {
		map->add_obstacle(this);
		internal_update_agent();

		request_sync();
	}
}

void NavObstacle2D::set_position(const Vector2& p_position)
{
	if (position == p_position) {
		return;
	}

	position = p_position;
	obstacle_dirty = true;

	if (agent) {
		agent->set_position(position);
	}

	request_sync();
}

void NavObstacle2D::set_radius(real_t p_radius)
{
	if (radius == p_radius) {
		return;
	}

	radius = p_radius;

	if (agent) {
		agent->set_radius(radius);
	}
}

void NavObstacle2D::set_velocity(const Vector2& p_velocity)
{
	velocity = p_velocity;

	if (agent) {
		agent->set_velocity(velocity);
	}
}

void NavObstacle2D::set_vertices(const Vector<Vector2>& p_vertices)
{
	if (vertices == p_vertices) {
		return;
	}

	vertices = p_vertices;
	obstacle_dirty = true;

	request_sync();
}

bool NavObstacle2D::is_map_changed()
{
	if (map) {
		bool is_changed = map->get_iteration_id() != last_map_iteration_id;
		last_map_iteration_id = map->get_iteration_id();
		return is_changed;
	}
	else {
		return false;
	}
}

void NavObstacle2D::set_avoidance_layers(uint32_t p_layers)
{
	if (avoidance_layers == p_layers) {
		return;
	}

	avoidance_layers = p_layers;
	obstacle_dirty = true;

	if (agent) {
		agent->set_avoidance_layers(avoidance_layers);
	}

	request_sync();
}

bool NavObstacle2D::is_dirty() const { return obstacle_dirty; }

void NavObstacle2D::sync() { obstacle_dirty = false; }

void NavObstacle2D::internal_update_agent()
{
	if (agent) {
		agent->set_neighbor_distance(0.0);
		agent->set_max_neighbors(0.0);
		agent->set_time_horizon_agents(0.0);
		agent->set_time_horizon_obstacles(0.0);
		agent->set_avoidance_mask(0.0);
		agent->set_neighbor_distance(0.0);
		agent->set_avoidance_priority(1.0);
		agent->set_map(map);
		agent->set_paused(paused);
		agent->set_radius(radius);
		agent->set_position(position);
		agent->set_avoidance_layers(avoidance_layers);
		agent->set_avoidance_enabled(avoidance_enabled);
	}
}

void NavObstacle2D::set_paused(bool p_paused)
{
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;

	if (map) {
		if (paused) {
			map->remove_obstacle(this);
		}
		else {
			map->add_obstacle(this);
		}
	}
	internal_update_agent();
}

bool NavObstacle2D::get_paused() const { return paused; }

NavObstacle2D::NavObstacle2D() : sync_dirty_request_list_element(this) {}

NavObstacle2D::~NavObstacle2D() { cancel_sync_request(); }

void NavObstacle2D::request_sync() {}



void NavObstacle2D::cancel_sync_request() {}


void NavLink2D::set_map(NavMap2D* p_map)
{
	if (map == p_map) {
		return;
	}

	cancel_sync_request();

	if (map) {
		map->remove_link(this);
	}

	map = p_map;
	iteration_dirty = true;

	if (map) {
		map->add_link(this);
		request_sync();
	}
}

void NavLink2D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_bidirectional(bool p_bidirectional)
{
	if (bidirectional == p_bidirectional) {
		return;
	}
	bidirectional = p_bidirectional;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_start_position(const Vector2 p_position)
{
	if (start_position == p_position) {
		return;
	}
	start_position = p_position;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_end_position(const Vector2 p_position)
{
	if (end_position == p_position) {
		return;
	}
	end_position = p_position;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}
	navigation_layers = p_navigation_layers;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_enter_cost(real_t p_enter_cost)
{
	real_t new_enter_cost = MAX(p_enter_cost, 0.0);
	if (enter_cost == new_enter_cost) {
		return;
	}
	enter_cost = new_enter_cost;
	iteration_dirty = true;

	request_sync();
}

void NavLink2D::set_travel_cost(real_t p_travel_cost)
{
	real_t new_travel_cost = MAX(p_travel_cost, 0.0);
	if (travel_cost == new_travel_cost) {
		return;
	}
	travel_cost = new_travel_cost;
	iteration_dirty = true;

	request_sync();
}

bool NavLink2D::sync()
{
	bool requires_map_update = false;
	if (!map) {
		return requires_map_update;
	}

	if (iteration_dirty && !iteration_building && !iteration_ready) {
		_build_iteration();
		iteration_ready = false;
		requires_map_update = true;
	}

	return requires_map_update;
}

void NavLink2D::_build_iteration()
{
	if (!iteration_dirty || iteration_building || iteration_ready) {
		return;
	}

	iteration_dirty = false;
	iteration_building = true;
	iteration_ready = false;

	Ref<NavIteration2D> new_iteration;
	new_iteration.instantiate();

	new_iteration->navigation_layers = get_navigation_layers();
	new_iteration->enter_cost = get_enter_cost();
	new_iteration->travel_cost = get_travel_cost();
	new_iteration->owner_type = get_type();
	new_iteration->owner_rid = get_self();

	new_iteration->enabled = get_enabled();
	new_iteration->start_position = get_start_position();
	new_iteration->end_position = get_end_position();
	new_iteration->bidirectional = is_bidirectional();

	RWLockWrite write_lock(iteration_rwlock);
	ERR_FAIL_COND(iteration.is_null());
	iteration = Ref<NavIteration2D>();
	DEV_ASSERT(iteration.is_null());
	iteration = new_iteration;
	iteration_id = iteration_id % UINT32_MAX + 1;

	iteration_building = false;
	iteration_ready = true;
}

Ref<NavIteration2D> NavLink2D::get_iteration()
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration;
}

NavLink2D::NavLink2D() : sync_dirty_request_list_element(this)
{
	iteration.instantiate();
}

NavLink2D::~NavLink2D()
{
	cancel_sync_request();

	iteration = Ref<NavIteration2D>();
}

void NavLink2D::request_sync() {}


void NavLink2D::cancel_sync_request() {}


void NavRegion2D::set_map(NavMap2D* p_map)
{
	if (map == p_map) {
		return;
	}

	cancel_async_thread_join();
	cancel_sync_request();

	if (map) {
		map->remove_region(this);
	}

	map = p_map;
	iteration_dirty = true;

	if (map) {
		map->add_region(this);
		request_sync();
	}
}

void NavRegion2D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections != p_enabled) {
		use_edge_connections = p_enabled;
		iteration_dirty = true;
	}

	request_sync();
}

void NavRegion2D::set_transform(Transform2D p_transform)
{
	if (transform == p_transform) {
		return;
	}
	transform = p_transform;
	iteration_dirty = true;

	request_sync();
}

ClosestPointQueryResult NavRegion2D::get_closest_point_info(const Vector2& p_point) const
{
	RWLockRead read_lock(region_rwlock);

	return NavMeshQueries2D::polygons_get_closest_point_info(get_polygons(), p_point);
}

Vector2 NavRegion2D::get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const
{
	RWLockRead read_lock(region_rwlock);

	if (!get_enabled()) {
		return Vector2();
	}

	return NavMeshQueries2D::polygons_get_random_point(
		get_polygons(), p_navigation_layers, p_uniformly);
}

void NavRegion2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}
	navigation_layers = p_navigation_layers;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_enter_cost(real_t p_enter_cost)
{
	real_t new_enter_cost = MAX(p_enter_cost,
 0.0);
	if (enter_cost == new_enter_cost) {
		return;
	}
	enter_cost = new_enter_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::set_travel_cost(real_t p_travel_cost)
{
	real_t new_travel_cost = MAX(p_travel_cost, 0.0);
	if (travel_cost == new_travel_cost) {
		return;
	}
	travel_cost = new_travel_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion2D::scratch_polygons()
{
	iteration_dirty = true;

	request_sync();
}

real_t NavRegion2D::get_surface_area() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_surface_area();
}

Rect2 NavRegion2D::get_bounds() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_bounds();
}

const LocalVector<Nav2D::Polygon>& NavRegion2D::get_polygons() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_navmesh_polygons();
}

bool NavRegion2D::sync()
{
	bool requires_map_update = false;
	if (!map) {
		return requires_map_update;
	}

	if (iteration_dirty && !iteration_building && !iteration_ready) {
		_build_iteration();
	}

	if (iteration_ready) {
		_sync_iteration();
		requires_map_update = true;
	}

	return requires_map_update;
}

void NavRegion2D::_build_iteration_threaded(void* p_arg)
{
	NavRegionIterationBuild2D* _iteration_build = static_cast<NavRegionIterationBuild2D*>(p_arg);

	NavRegionBuilder2D::build_iteration(*_iteration_build);
}

void NavRegion2D::_sync_iteration()
{
	if (iteration_building || !iteration_ready) {
		return;
	}

	performance_data.pm_polygon_count = iteration_build.performance_data.pm_polygon_count;
	performance_data.pm_edge_count = iteration_build.performance_data.pm_edge_count;
	performance_data.pm_edge_merge_count = iteration_build.performance_data.pm_edge_merge_count;

	RWLockWrite write_lock(iteration_rwlock);
	ERR_FAIL_COND(iteration.is_null());
	iteration = Ref<NavIteration2D>();
	DEV_ASSERT(iteration.is_null());
	iteration = iteration_build.region_iteration;
	iteration_build.region_iteration = Ref<NavIteration2D>();
	DEV_ASSERT(iteration_build.region_iteration.is_null());
	iteration_id = iteration_id % UINT32_MAX + 1;

	iteration_ready = false;

	cancel_async_thread_join();
}

Ref<NavIteration2D> NavRegion2D::get_iteration()
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration;
}

void NavRegion2D::set_use_async_iterations(bool p_enabled)
{
	if (use_async_iterations == p_enabled) {
		return;
	}
#ifdef THREADS_ENABLED
	use_async_iterations = p_enabled;
#endif
}

bool NavRegion2D::get_use_async_iterations() const { return use_async_iterations; }

NavRegion2D::NavRegion2D() : sync_dirty_request_list_element(this), async_list_element(this)
{
	iteration_build.region = this;
	iteration.instantiate();

#ifndef THREADS_ENABLED
	use_async_iterations = false;
#endif
}

NavRegion2D::~NavRegion2D()
{
	cancel_async_thread_join();
	cancel_sync_request();

	iteration_build.region = nullptr;
	iteration_build.region_iteration = Ref<NavIteration2D>();
	iteration = Ref<NavIteration2D>();
}

void NavRegion2D::request_sync() {}



void NavRegion2D::cancel_async_thread_join() {}

void NavRegion2D::cancel_sync_request() {}

void NavRegion2D::set_navigation_mesh(Ref<NavigationPolygon>) {}

void NavRegion2D::_build_iteration() {}


#ifdef DEBUG_ENABLED
#define NAVMAP_ITERATION_ZERO_ERROR_MSG()                                                          \
	ERR_PRINT_ONCE(                                                                                \
		"NavigationServer navigation map query failed because it was made before first map synchronization.\n\
	NavigationServer 'map_changed' signal can be used to receive update notifications.\n\
	NavigationServer 'map_get_iteration_id()' can be used to check if a map has finished its newest iteration.");
#else
#define NAVMAP_ITERATION_ZERO_ERROR_MSG()
#endif // DEBUG_ENABLED

#define GET_MAP_ITERATION()                                                                        \
	iteration_slot_rwlock.read_lock();                                                             \
	NavMapIteration2D& map_iteration = iteration_slots[iteration_slot_index];                      \
	NavMapIterationRead2D iteration_read_lock(map_iteration);                                      \
	iteration_slot_rwlock.read_unlock();

#define GET_MAP_ITERATION_CONST()                                                                  \
	iteration_slot_rwlock.read_lock();                                                             \
	const NavMapIteration2D& map_iteration = iteration_slots[iteration_slot_index];                \
	NavMapIterationRead2D iteration_read_lock(map_iteration);                                      \
	iteration_slot_rwlock.read_unlock();

void NavMap2D::set_cell_size(real_t p_cell_size)
{
	if (cell_size == p_cell_size) {
		return;
	}
	cell_size = MAX(p_cell_size, NavigationDefaults2D::NAV_MESH_CELL_SIZE_MIN);
	_update_merge_rasterizer_cell_dimensions();
	map_settings_dirty = true;
}

void NavMap2D::set_merge_rasterizer_cell_scale(float p_value)
{
	if (merge_rasterizer_cell_scale == p_value) {
		return;
	}
	merge_rasterizer_cell_scale =
		MAX(MIN(p_value, 0.1), NavigationDefaults2D::NAV_MESH_CELL_SIZE_MIN);
	_update_merge_rasterizer_cell_dimensions();
	map_settings_dirty = true;
}

void NavMap2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}
	use_edge_connections = p_enabled;
	iteration_dirty = true;
}

void NavMap2D::set_edge_connection_margin(real_t p_edge_connection_margin)
{
	if (edge_connection_margin == p_edge_connection_margin) {
		return;
	}
	edge_connection_margin = p_edge_connection_margin;
	iteration_dirty = true;
}

void NavMap2D::set_link_connection_radius(real_t p_link_connection_radius)
{
	if (link_connection_radius == p_link_connection_radius) {
		return;
	}
	link_connection_radius = p_link_connection_radius;
	iteration_dirty = true;
}

const Vector2& NavMap2D::get_merge_rasterizer_cell_size() const
{
	return merge_rasterizer_cell_size;
}

PointKey NavMap2D::get_point_key(const Vector2& p_pos) const
{
	const int x = static_cast<int>(Math::floor(p_pos.x / merge_rasterizer_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / merge_rasterizer_cell_size.y));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	return p;
}

void NavMap2D::query_path(NavMeshQueries2D::NavMeshPathQueryTask2D& p_query_task)
{
	if (iteration_id == 0) {
		return;
	}

	GET_MAP_ITERATION();

	map_iteration.path_query_slots_semaphore.wait();

	map_iteration.path_query_slots_mutex.lock();
	for (NavMeshQueries2D::PathQuerySlot& p_path_query_slot : map_iteration.path_query_slots) {
		if (!p_path_query_slot.in_use) {
			p_path_query_slot.in_use = true;
			p_query_task.path_query_slot = &p_path_query_slot;
			break;
		}
	}
	map_iteration.path_query_slots_mutex.unlock();

	if (p_query_task.path_query_slot == nullptr) {
		map_iteration.path_query_slots_semaphore.post();
		ERR_FAIL_NULL_MSG(p_query_task.path_query_slot,
			"No unused NavMap2D path query slot found! This should never happen :(.");
	}

	NavMeshQueries2D::query_task_map_iteration_get_path(p_query_task, map_iteration);

	map_iteration.path_query_slots_mutex.lock();
	uint32_t used_slot_index = p_query_task.path_query_slot->slot_index;
	map_iteration.path_query_slots[used_slot_index].in_use = false;
	p_query_task.path_query_slot = nullptr;
	map_iteration.path_query_slots_mutex.unlock();

	map_iteration.path_query_slots_semaphore.post();
}

Vector2 NavMap2D::get_closest_point(const Vector2& p_point) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return Vector2();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries2D::map_iteration_get_closest_point(map_iteration, p_point);
}

RID NavMap2D::get_closest_point_owner(const Vector2& p_point) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return RID();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries2D::map_iteration_get_closest_point_owner(map_iteration, p_point);
}

ClosestPointQueryResult NavMap2D::get_closest_point_info(const Vector2& p_point) const
{
	GET_MAP_ITERATION_CONST();

	return NavMeshQueries2D::map_iteration_get_closest_point_info(map_iteration, p_point);
}

void NavMap2D::add_region(NavRegion2D* p_region)
{
	DEV_ASSERT(!regions.has(p_region));

	regions.push_back(p_region);
	iteration_dirty = true;
}

void NavMap2D::remove_region(NavRegion2D* p_region)
{
	if (regions.erase_unordered(p_region)) {
		iteration_dirty = true;
	}
}

void NavMap2D::add_link(NavLink2D* p_link)
{
	DEV_ASSERT(!links.has(p_link));

	links.push_back(p_link);
	iteration_dirty = true;
}

void NavMap2D::remove_link(NavLink2D* p_link)
{
	if (links.erase_unordered(p_link)) {
		iteration_dirty = true;
	}
}

bool NavMap2D::has_agent(NavAgent2D* p_agent) const { return agents.has(p_agent); }

void NavMap2D::add_agent(NavAgent2D* p_agent)
{
	if (!has_agent(p_agent)) {
		agents.push_back(p_agent);
		agents_dirty = true;
	}
}

void NavMap2D::remove_agent(NavAgent2D* p_agent)
{
	remove_agent_as_controlled(p_agent);
	if (agents.erase_unordered(p_agent)) {
		agents_dirty = true;
	}
}

bool NavMap2D::has_obstacle(NavObstacle2D* p_obstacle) const { return obstacles.has(p_obstacle); }

void NavMap2D::add_obstacle(NavObstacle2D* p_obstacle)
{
	if (p_obstacle->get_paused()) {
		// No point in adding a paused obstacle, it will add itself when unpaused again.
		return;
	}

	if (!has_obstacle(p_obstacle)) {
		obstacles.push_back(p_obstacle);
		obstacles_dirty = true;
	}
}

void NavMap2D::remove_obstacle(NavObstacle2D* p_obstacle)
{
	if (obstacles.erase_unordered(p_obstacle)) {
		obstacles_dirty = true;
	}
}

void NavMap2D::set_agent_as_controlled(NavAgent2D* p_agent)
{
	remove_agent_as_controlled(p_agent);

	if (p_agent->get_paused()) {
		// No point in adding a paused agent, it will add itself when unpaused again.
		return;
	}

	int64_t agent_index = active_avoidance_agents.find(p_agent);
	if (agent_index < 0) {
		active_avoidance_agents.push_back(p_agent);
		agents_dirty = true;
	}
}

void NavMap2D::remove_agent_as_controlled(NavAgent2D* p_agent)
{
	if (active_avoidance_agents.erase_unordered(p_agent)) {
		agents_dirty = true;
	}
}

Vector2 NavMap2D::get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const
{
	GET_MAP_ITERATION_CONST();

	return NavMeshQueries2D::map_iteration_get_random_point(
		map_iteration, p_navigation_layers, p_uniformly);
}

void NavMap2D::_build_iteration()
{
	if (!iteration_dirty || iteration_building || iteration_ready) {
		return;
	}

	// Get the next free iteration slot that should be potentially unused.
	iteration_slot_rwlock.read_lock();
	NavMapIteration2D& next_map_iteration = iteration_slots[(iteration_slot_index + 1) % 2];
	// Check if the iteration slot is truly free or still used by an external thread.
	bool iteration_is_free = next_map_iteration.users.get() == 0;
	iteration_slot_rwlock.read_unlock();

	if (!iteration_is_free) {
		// A long running pathfinding thread or something is still reading
		// from this older iteration and needs to finish first.
		// Return and wait for the next sync cycle to check again.
		return;
	}

	// Iteration slot is free and no longer used by anything, let's build.

	iteration_dirty = false;
	iteration_building = true;
	iteration_ready = false;

	// We don't need to hold any lock because at this point nothing else can touch it.
	// All new queries are already forwarded to the other iteration slot.

	iteration_build.reset();

	iteration_build.merge_rasterizer_cell_size = get_merge_rasterizer_cell_size();
	iteration_build.use_edge_connections = get_use_edge_connections();
	iteration_build.edge_connection_margin = get_edge_connection_margin();
	iteration_build.link_connection_radius = get_link_connection_radius();

	next_map_iteration.clear();

	next_map_iteration.region_iterations.resize(regions.size());
	next_map_iteration.link_iterations.resize(links.size());

	uint32_t region_id_count = 0;
	uint32_t link_id_count = 0;

	for (NavRegion2D* region : regions) {
		const Ref<NavIteration2D> region_iteration = region->get_iteration();
		next_map_iteration.region_iterations[region_id_count++] = region_iteration;
		next_map_iteration.region_ptr_to_region_iteration[region] = region_iteration;
	}
	for (NavLink2D* link : links) {
		const Ref<NavIteration2D> link_iteration = link->get_iteration();
		next_map_iteration.link_iterations[link_id_count++] = link_iteration;
	}

	iteration_build.map_iteration = &next_map_iteration;

	NavMapBuilder2D::build_navmap_iteration(iteration_build);

	iteration_building = false;
	iteration_ready = true;
}

void NavMap2D::_build_iteration_threaded(void* p_arg)
{
	NavMapIterationBuild2D* _iteration_build = static_cast<NavMapIterationBuild2D*>(p_arg);

	NavMapBuilder2D::build_navmap_iteration(*_iteration_build);
}

void NavMap2D::_sync_iteration()
{
	if (iteration_building || !iteration_ready) {
		return;
	}

	performance_data.pm_edge_connection_count =
		iteration_build.performance_data.pm_edge_connection_count;
	performance_data.pm_edge_free_count = iteration_build.performance_data.pm_edge_free_count;

	iteration_id = iteration_id % UINT32_MAX + 1;

	// Finally ping-pong switch the iteration slot.
	iteration_slot_rwlock.write_lock();
	uint32_t next_iteration_slot_index = (iteration_slot_index + 1) % 2;
	iteration_slot_index = next_iteration_slot_index;
	iteration_slot_rwlock.write_unlock();

	iteration_ready = false;
}

void NavMap2D::_sync_avoidance()
{
	_sync_dirty_avoidance_update_requests();

	if (obstacles_dirty || agents_dirty) {
		_update_rvo_simulation();
	}

	obstacles_dirty = false;
	agents_dirty = false;
}

void NavMap2D::_update_rvo_obstacles_tree()
{
	int obstacle_vertex_count = 0;
	for (NavObstacle2D* obstacle : obstacles) {
		obstacle_vertex_count += obstacle->get_vertices().size();
	}

	// Cleaning old obstacles.
	for (size_t i = 0; i < rvo_simulation.obstacles_.size(); ++i) {
		delete rvo_simulation.obstacles_[i];
	}
	rvo_simulation.obstacles_.clear();

	// Cannot use LocalVector here as RVO library expects std::vector to build KdTree
	std::vector<RVO2D::Obstacle2D*>& raw_obstacles = rvo_simulation.obstacles_;
	raw_obstacles.reserve(obstacle_vertex_count);

	// The following block is modified copy from RVO2D::AddObstacle()
	// Obstacles are linked and depend on all other obstacles.
	for (NavObstacle2D* obstacle : obstacles) {
		if (!obstacle->is_avoidance_enabled()) {
			continue;
		}
		const Vector2& _obstacle_position = obstacle->get_position();
		const Vector<Vector2>& _obstacle_vertices = obstacle->get_vertices();

		if (_obstacle_vertices.size() < 2) {
			continue;
		}

		std::vector<RVO2D::Vector2> rvo_vertices;
		rvo_vertices.reserve(_obstacle_vertices.size());

		uint32_t _obstacle_avoidance_layers = obstacle->get_avoidance_layers();

		for (const Vector2& _obstacle_vertex : _obstacle_vertices) {
			rvo_vertices.push_back(RVO2D::Vector2(_obstacle_vertex.x + _obstacle_position.x,
				_obstacle_vertex.y + _obstacle_position.y));
		}

		const size_t obstacleNo = raw_obstacles.size();

		for (size_t i = 0; i < rvo_vertices.size(); i++) {
			RVO2D::Obstacle2D* rvo_obstacle = new RVO2D::Obstacle2D();
			rvo_obstacle->point_ = rvo_vertices[i];

			rvo_obstacle->avoidance_layers_ = _obstacle_avoidance_layers;

			if (i != 0) {
				rvo_obstacle->prevObstacle_ = raw_obstacles.back();
				rvo_obstacle->prevObstacle_->nextObstacle_ = rvo_obstacle;
			}

			if (i == rvo_vertices.size() - 1) {
				rvo_obstacle->nextObstacle_ = raw_obstacles[obstacleNo];
				rvo_obstacle->nextObstacle_->prevObstacle_ = rvo_obstacle;
			}

			rvo_obstacle->unitDir_ = normalize(
				rvo_vertices[(i == rvo_vertices.size() - 1 ? 0 : i + 1)] - rvo_vertices[i]);

			if (rvo_vertices.size() == 2) {
				rvo_obstacle->isConvex_ = true;
			}
			else {
				rvo_obstacle->isConvex_ =
					(leftOf(rvo_vertices[(i == 0 ? rvo_vertices.size() - 1 : i - 1)],
						 rvo_vertices[i],
						 rvo_vertices[(i == rvo_vertices.size() - 1 ? 0 : i + 1)]) >= 0.0f);
			}

			rvo_obstacle->id_ = raw_obstacles.size();

			raw_obstacles.push_back(rvo_obstacle);
		}
	}

	rvo_simulation.kdTree_->buildObstacleTree(raw_obstacles);
}

void NavMap2D::_update_rvo_agents_tree()
{
	// Cannot use LocalVector here as RVO library expects std::vector to build KdTree.
	std::vector<RVO2D::Agent2D*> raw_agents;
	raw_agents.reserve(active_avoidance_agents.size());
	for (NavAgent2D* agent : active_avoidance_agents) {
		raw_agents.push_back(agent->get_rvo_agent());
	}
	rvo_simulation.kdTree_->buildAgentTree(raw_agents);
}

void NavMap2D::_update_rvo_simulation()
{
	if (obstacles_dirty) {
		_update_rvo_obstacles_tree();
	}
	if (agents_dirty) {
		_update_rvo_agents_tree();
	}
}

void NavMap2D::compute_single_avoidance_step(uint32_t p_index, NavAgent2D** p_agent)
{
	(*(p_agent + p_index))->get_rvo_agent()->computeNeighbors(&rvo_simulation);
	(*(p_agent + p_index))->get_rvo_agent()->computeNewVelocity(&rvo_simulation);
	(*(p_agent + p_index))->get_rvo_agent()->update(&rvo_simulation);
	(*(p_agent + p_index))->update();
}

void NavMap2D::dispatch_callbacks()
{
	for (NavAgent2D* agent : active_avoidance_agents) {
		agent->dispatch_avoidance_callback();
	}
}

void NavMap2D::_update_merge_rasterizer_cell_dimensions()
{
	merge_rasterizer_cell_size.x = cell_size * merge_rasterizer_cell_scale;
	merge_rasterizer_cell_size.y = cell_size * merge_rasterizer_cell_scale;
}

int NavMap2D::get_region_connections_count(NavRegion2D* p_region) const
{
	ERR_FAIL_NULL_V(p_region, 0);

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion2D*, Ref<NavIteration2D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration2D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			return found_connections->value.size();
		}
	}

	return 0;
}

Vector2 NavMap2D::get_region_connection_pathway_start(
	NavRegion2D* p_region, int p_connection_id) const
{
	ERR_FAIL_NULL_V(p_region, Vector2());

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion2D*, Ref<NavIteration2D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration2D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			ERR_FAIL_INDEX_V(p_connection_id, int(found_connections->value.size()), Vector2());
			return found_connections->value[p_connection_id].pathway_start;
		}
	}

	return Vector2();
}

Vector2 NavMap2D::get_region_connection_pathway_end(
	NavRegion2D* p_region, int p_connection_id) const
{
	ERR_FAIL_NULL_V(p_region, Vector2());

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion2D*, Ref<NavIteration2D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration2D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			ERR_FAIL_INDEX_V(p_connection_id, int(found_connections->value.size()), Vector2());
			return found_connections->value[p_connection_id].pathway_end;
		}
	}

	return Vector2();
}

void NavMap2D::set_use_async_iterations(bool p_enabled)
{
	if (use_async_iterations == p_enabled) {
		return;
	}
#ifdef THREADS_ENABLED
	use_async_iterations = p_enabled;
#endif
}

bool NavMap2D::get_use_async_iterations() const { return use_async_iterations; }



NavMap2D::~NavMap2D() {}

void NavMap2D::sync() {}

void NavMap2D::_sync_dirty_avoidance_update_requests() {}

void NavMap2D::step(double) {}

NavMap2D::NavMap2D() {}

#undef NAVMAP_ITERATION_ZERO_ERROR_MSG
#undef GET_MAP_ITERATION
#undef GET_MAP_ITERATION_CONST


PointKey NavMapBuilder2D::get_point_key(const Vector2& p_pos, const Vector2& p_cell_size)
{
	const int x = static_cast<int>(Math::floor(p_pos.x / p_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / p_cell_size.y));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	return p;
}

void NavMapBuilder2D::build_navmap_iteration(NavMapIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	performance_data.pm_polygon_count = 0;
	performance_data.pm_edge_count = 0;
	performance_data.pm_edge_merge_count = 0;
	performance_data.pm_edge_connection_count = 0;
	performance_data.pm_edge_free_count = 0;

	_build_step_gather_region_polygons(r_build);

	_build_step_find_edge_connection_pairs(r_build);

	_build_step_merge_edge_connection_pairs(r_build);

	_build_step_edge_connection_margin_connections(r_build);

	_build_step_navlink_connections(r_build);

	_build_update_map_iteration(r_build);
}

void NavMapBuilder2D::_build_step_gather_region_polygons(NavMapIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;
	NavMapIteration2D* map_iteration = r_build.map_iteration;

	const LocalVector<Ref<NavIteration2D>>& regions = map_iteration->region_iterations;
	HashMap<const NavIteration2D*, LocalVector<Connection>>& region_external_connections =
		map_iteration->external_region_connections;

	map_iteration->navbases_polygons_external_connections.clear();

	// Remove regions connections.
	region_external_connections.clear();

	// Copy all region polygons in the map.
	int polygon_count = 0;
	for (const Ref<NavIteration2D>& region : regions) {
		const uint32_t polygons_size = region->navmesh_polygons.size();
		polygon_count += polygons_size;

		region_external_connections[region.ptr()] = LocalVector<Connection>();
		map_iteration->navbases_polygons_external_connections[region.ptr()] =
			LocalVector<LocalVector<Connection>>();
		map_iteration->navbases_polygons_external_connections[region.ptr()].resize(polygons_size);
	}

	performance_data.pm_polygon_count = polygon_count;
	r_build.polygon_count = polygon_count;
}

void NavMapBuilder2D::_build_step_find_edge_connection_pairs(NavMapIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;
	NavMapIteration2D* map_iteration = r_build.map_iteration;
	int polygon_count = r_build.polygon_count;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>& connection_pairs_map =
		r_build.iter_connection_pairs_map;

	// Group all edges per key.
	connection_pairs_map.clear();
	connection_pairs_map.reserve(polygon_count);
	int free_edges_count = 0; // How many ConnectionPairs have only one Connection.
	int edge_merge_error_count = 0;

	for (const Ref<NavIteration2D>& region : map_iteration->region_iterations) {
		for (const ConnectableEdge& connectable_edge : region->get_external_edges()) {
			const EdgeKey& ek = connectable_edge.ek;

			HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>::Iterator pair_it =
				connection_pairs_map.find(ek);
			if (!pair_it) {
				pair_it = connection_pairs_map.insert(ek, EdgeConnectionPair());
				performance_data.pm_edge_count += 1;
				++free_edges_count;
			}
			EdgeConnectionPair& pair = pair_it->value;
			if (pair.size < 2) {
				// Add the polygon/edge tuple to this key.
				Connection new_connection;
				new_connection.polygon = &region->navmesh_polygons[connectable_edge.polygon_index];
				new_connection.edge = connectable_edge.edge;
				new_connection.pathway_start = connectable_edge.pathway_start;
				new_connection.pathway_end = connectable_edge.pathway_end;

				pair.connections[pair.size] = new_connection;
				++pair.size;
				if (pair.size == 2) {
					--free_edges_count;
				}

			}
			else {
				// The edge is already connected with another edge, skip.
				edge_merge_error_count++;
			}
		}
	}

	if (edge_merge_error_count > 0) {
		WARN_PRINT(
			"Navigation map synchronization had " + itos(edge_merge_error_count) +
			" edge error(s).\nMore than 2 edges tried to occupy the same map rasterization "
			"space.\nThis causes a logical error in the navigation mesh geometry and is commonly "
			"caused by overlap or too densely placed edges.\nConsider baking with a higher "
			"'cell_size', greater geometry margin, and less detailed bake objects to cause fewer "
			"edges.\nConsider lowering the 'navigation/2d/merge_rasterizer_cell_scale' in the "
			"project settings.\nThis warning can be toggled under "
			"'navigation/2d/warnings/navmesh_edge_merge_errors' in the project settings.");
	}

	r_build.free_edge_count = free_edges_count;
}

void NavMapBuilder2D::_build_step_merge_edge_connection_pairs(NavMapIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>& connection_pairs_map =
		r_build.iter_connection_pairs_map;
	LocalVector<Connection>& free_edges = r_build.iter_free_edges;
	int free_edges_count = r_build.free_edge_count;
	bool use_edge_connections = r_build.use_edge_connections;

	free_edges.clear();
	free_edges.reserve(free_edges_count);

	NavMapIteration2D* map_iteration = r_build.map_iteration;

	HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>&
		navbases_polygons_external_connections =
			map_iteration->navbases_polygons_external_connections;

	for (const KeyValue<EdgeKey, EdgeConnectionPair>& pair_it : connection_pairs_map) {
		const EdgeConnectionPair& pair = pair_it.value;
		if (pair.size == 2) {
			// Connect edge that are shared in different polygons.
			const Connection& c1 = pair.connections[0];
			const Connection& c2 = pair.connections[1];

			navbases_polygons_external_connections[c1.polygon->owner][c1.polygon->id].push_back(c2);
			navbases_polygons_external_connections[c2.polygon->owner][c2.polygon->id].push_back(c1);
			performance_data.pm_edge_connection_count += 1;

		}
		else {
			CRASH_COND_MSG(
				pair.size != 1, vformat("Number of connection != 1. Found: %d", pair.size));
			if (use_edge_connections &&
				pair.connections[0].polygon->owner->get_use_edge_connections()) {
				free_edges.push_back(pair.connections[0]);
			}
		}
	}
}

void NavMapBuilder2D::_build_step_edge_connection_margin_connections(
	NavMapIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;
	NavMapIteration2D* map_iteration = r_build.map_iteration;

	real_t edge_connection_margin = r_build.edge_connection_margin;

	LocalVector<Connection>& free_edges = r_build.iter_free_edges;
	HashMap<const NavIteration2D*, LocalVector<Connection>>& region_external_connections =
		map_iteration->external_region_connections;

	HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>&
		navbases_polygons_external_connections =
			map_iteration->navbases_polygons_external_connections;

	// Find the compatible near edges.
	//
	// Note:
	// Considering that the edges must be compatible (for obvious reasons)
	// to be connected, create new polygons to remove that small gap is
	// not really useful and would result in wasteful computation during
	// connection, integration and path finding.
	performance_data.pm_edge_free_count = free_edges.size();

	const real_t edge_connection_margin_squared = edge_connection_margin * edge_connection_margin;

	for (uint32_t i = 0; i < free_edges.size(); i++) {
		const Connection& free_edge = free_edges[i];
		const Vector2& edge_p1 = free_edge.pathway_start;
		const Vector2& edge_p2 = free_edge.pathway_end;

		for (uint32_t j = 0; j < free_edges.size(); j++) {
			const Connection& other_edge = free_edges[j];
			if (i == j || free_edge.polygon->owner == other_edge.polygon->owner) {
				continue;
			}

			const Vector2& other_edge_p1 = other_edge.pathway_start;
			const Vector2& other_edge_p2 = other_edge.pathway_end;

			// Compute the projection of the opposite edge on the current one
			Vector2 edge_vector = edge_p2 - edge_p1;
			real_t projected_p1_ratio =
				edge_vector.dot(other_edge_p1 - edge_p1) / (edge_vector.length_squared());
			real_t projected_p2_ratio =
				edge_vector.dot(other_edge_p2 - edge_p1) / (edge_vector.length_squared());
			if ((projected_p1_ratio < 0.0 && projected_p2_ratio < 0.0) ||
				(projected_p1_ratio > 1.0 && projected_p2_ratio > 1.0)) {
				continue;
			}

			// Check if the two edges are close to each other enough and compute a pathway between
			// the two regions.
			Vector2 self1 = edge_vector * CLAMP(projected_p1_ratio, 0.0, 1.0) + edge_p1;
			Vector2 other1;
			if (projected_p1_ratio >= 0.0 && projected_p1_ratio <= 1.0) {
				other1 = other_edge_p1;
			}
			else {
				other1 = other_edge_p1.lerp(other_edge_p2,
					(1.0 - projected_p1_ratio) / (projected_p2_ratio - projected_p1_ratio));
			}
			if (other1.distance_squared_to(self1) > edge_connection_margin_squared) {
				continue;
			}

			Vector2 self2 = edge_vector * CLAMP(projected_p2_ratio, 0.0, 1.0) + edge_p1;
			Vector2 other2;
			if (projected_p2_ratio >= 0.0 && projected_p2_ratio <= 1.0) {
				other2 = other_edge_p2;
			}
			else {
				other2 = other_edge_p1.lerp(other_edge_p2,
					(0.0 - projected_p1_ratio) / (projected_p2_ratio - projected_p1_ratio));
			}
			if (other2.distance_squared_to(self2) > edge_connection_margin_squared) {
				continue;
			}

			// The edges can now be connected.
			Connection new_connection = other_edge;
			new_connection.pathway_start = (self1 + other1) / 2.0;
			new_connection.pathway_end = (self2 + other2) / 2.0;
			// free_edge.polygon->connections.push_back(new_connection);

			// Add the connection to the region_connection map.
			region_external_connections[free_edge.polygon->owner].push_back(new_connection);
			navbases_polygons_external_connections[free_edge.polygon->owner][free_edge.polygon->id]
				.push_back(new_connection);
			performance_data.pm_edge_connection_count += 1;
		}
	}
}

void NavMapBuilder2D::_build_step_navlink_connections(NavMapIterationBuild2D& r_build)
{
	NavMapIteration2D* map_iteration = r_build.map_iteration;

	real_t link_connection_radius = r_build.link_connection_radius;

	const LocalVector<Ref<NavIteration2D>>& links = map_iteration->link_iterations;

	int polygon_count = r_build.polygon_count;

	real_t link_connection_radius_sqr = link_connection_radius * link_connection_radius;

	HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>&
		navbases_polygons_external_connections =
			map_iteration->navbases_polygons_external_connections;
	LocalVector<Nav2D::Polygon>& navlink_polygons = map_iteration->navlink_polygons;
	navlink_polygons.clear();
	navlink_polygons.resize(links.size());
	uint32_t navlink_index = 0;

	// Search for polygons within range of a nav link.
	for (const Ref<NavIteration2D>& link : links) {
		polygon_count++;
		Polygon& new_polygon = navlink_polygons[navlink_index++];

		new_polygon.id = 0;
		new_polygon.owner = link.ptr();

		const Vector2 link_start_pos = link->get_start_position();
		const Vector2 link_end_pos = link->get_end_position();

		Polygon* closest_start_polygon = nullptr;
		real_t closest_start_sqr_dist = link_connection_radius_sqr;
		Vector2 closest_start_point;

		Polygon* closest_end_polygon = nullptr;
		real_t closest_end_sqr_dist = link_connection_radius_sqr;
		Vector2 closest_end_point;

		for (const Ref<NavIteration2D>& region : map_iteration->region_iterations) {
			Rect2 region_bounds = region->get_bounds().grow(link_connection_radius);
			if (!region_bounds.has_point(link_start_pos) &&
				!region_bounds.has_point(link_end_pos)) {
				continue;
			}

			for (Polygon& polyon : region->navmesh_polygons) {
				for (uint32_t point_id = 2; point_id < polyon.vertices.size(); point_id += 1) {
					const Triangle2 triangle(polyon.vertices[0], polyon.vertices[point_id - 1],
						polyon.vertices[point_id]);

					{
						const Vector2 start_point = triangle.get_closest_point_to(link_start_pos);
						const real_t sqr_dist = start_point.distance_squared_to(link_start_pos);

						// Pick the polygon that is within our radius and is closer than anything
						// we've seen yet.
						if (sqr_dist < closest_start_sqr_dist) {
							closest_start_sqr_dist = sqr_dist;
							closest_start_point = start_point;
							closest_start_polygon = &polyon;
						}
					}

					{
						const Vector2 end_point = triangle.get_closest_point_to(link_end_pos);
						const real_t sqr_dist = end_point.distance_squared_to(link_end_pos);

						// Pick the polygon that is within our radius and is closer than anything
						// we've seen yet.
						if (sqr_dist < closest_end_sqr_dist) {
							closest_end_sqr_dist = sqr_dist;
							closest_end_point = end_point;
							closest_end_polygon = &polyon;
						}
					}
				}
			}
		}

		// If we have both a start and end point, then create a synthetic polygon to route through.
		if (closest_start_polygon && closest_end_polygon) {
			new_polygon.vertices.resize(4);

			// Build a set of vertices that create a thin polygon going from the start to the end
			// point.
			new_polygon.vertices[0] = closest_start_point;
			new_polygon.vertices[1] = closest_start_point;
			new_polygon.vertices[2] = closest_end_point;
			new_polygon.vertices[3] = closest_end_point;

			// Setup connections to go forward in the link.
			{
				Connection entry_connection;
				entry_connection.polygon = &new_polygon;
				entry_connection.edge = -1;
				entry_connection.pathway_start = new_polygon.vertices[0];
				entry_connection.pathway_end = new_polygon.vertices[1];
				navbases_polygons_external_connections[closest_start_polygon->owner]
													  [closest_start_polygon->id]
														  .push_back(entry_connection);

				Connection exit_connection;
				exit_connection.polygon = closest_end_polygon;
				exit_connection.edge = -1;
				exit_connection.pathway_start = new_polygon.vertices[2];
				exit_connection.pathway_end = new_polygon.vertices[3];
				navbases_polygons_external_connections[link.ptr()].push_back(
					LocalVector<Nav2D::Connection>());
				navbases_polygons_external_connections[link.ptr()][new_polygon.id].push_back(
					exit_connection);
			}

			// If the link is bi-directional, create connections from the end to the start.
			if (link->is_bidirectional()) {
				Connection entry_connection;
				entry_connection.polygon = &new_polygon;
				entry_connection.edge = -1;
				entry_connection.pathway_start = new_polygon.vertices[2];
				entry_connection.pathway_end = new_polygon.vertices[3];
				navbases_polygons_external_connections[closest_end_polygon->owner]
													  [closest_end_polygon->id]
														  .push_back(entry_connection);

				Connection exit_connection;
				exit_connection.polygon = closest_start_polygon;
				exit_connection.edge = -1;
				exit_connection.pathway_start = new_polygon.vertices[0];
				exit_connection.pathway_end = new_polygon.vertices[1];
				navbases_polygons_external_connections[link.ptr()].push_back(
					LocalVector<Nav2D::Connection>());
				navbases_polygons_external_connections[link.ptr()][new_polygon.id].push_back(
					exit_connection);
			}
		}
	}

	r_build.polygon_count = polygon_count;
}

void NavMapBuilder2D::_build_update_map_iteration(NavMapIterationBuild2D& r_build)
{
	NavMapIteration2D* map_iteration = r_build.map_iteration;

	map_iteration->navmesh_polygon_count = r_build.polygon_count;

	uint32_t navmesh_polygon_count = r_build.polygon_count;
	uint32_t total_polygon_count = navmesh_polygon_count;

	map_iteration->path_query_slots_mutex.lock();
	for (NavMeshQueries2D::PathQuerySlot& p_path_query_slot : map_iteration->path_query_slots) {
		p_path_query_slot.traversable_polys.clear();
		p_path_query_slot.traversable_polys.reserve(navmesh_polygon_count * 0.25);
		p_path_query_slot.path_corridor.clear();

		p_path_query_slot.path_corridor.resize(total_polygon_count);

		p_path_query_slot.poly_to_id.clear();
		p_path_query_slot.poly_to_id.reserve(total_polygon_count);

		int polygon_id = 0;
		for (Ref<NavIteration2D>& region : map_iteration->region_iterations) {
			for (const Polygon& polygon : region->navmesh_polygons) {
				p_path_query_slot.poly_to_id[&polygon] = polygon_id;
				polygon_id++;
			}
		}

		for (const Polygon& polygon : map_iteration->navlink_polygons) {
			p_path_query_slot.poly_to_id[&polygon] = polygon_id;
			polygon_id++;
		}

		DEV_ASSERT(p_path_query_slot.path_corridor.size() == p_path_query_slot.poly_to_id.size());
	}

	map_iteration->path_query_slots_mutex.unlock();
}


void NavRegionBuilder2D::build_iteration(NavRegionIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	performance_data.pm_polygon_count = 0;
	performance_data.pm_edge_count = 0;
	performance_data.pm_edge_merge_count = 0;
	performance_data.pm_edge_connection_count = 0;
	performance_data.pm_edge_free_count = 0;

	_build_step_process_navmesh_data(r_build);

	_build_step_find_edge_connection_pairs(r_build);

	_build_step_merge_edge_connection_pairs(r_build);

	_build_update_iteration(r_build);
}

void NavRegionBuilder2D::_build_step_process_navmesh_data(NavRegionIterationBuild2D& r_build)
{
	Vector<Vector2> _navmesh_vertices = r_build.navmesh_data.vertices;
	Vector<Vector<int>> _navmesh_polygons = r_build.navmesh_data.polygons;

	if (_navmesh_vertices.is_empty() || _navmesh_polygons.is_empty()) {
		return;
	}

	PerformanceData& performance_data = r_build.performance_data;
	Ref<NavIteration2D> region_iteration = r_build.region_iteration;

	const Transform2D& region_transform = region_iteration->transform;
	LocalVector<Nav2D::Polygon>& navmesh_polygons = region_iteration->navmesh_polygons;

	const int vertex_count = _navmesh_vertices.size();

	const Vector2* vertices_ptr = _navmesh_vertices.ptr();
	const Vector<int>* polygons_ptr = _navmesh_polygons.ptr();

	navmesh_polygons.resize(_navmesh_polygons.size());

	real_t _new_region_surface_area = 0.0;
	Rect2 _new_region_bounds;

	bool first_vertex = true;

	for (uint32_t i = 0; i < navmesh_polygons.size(); i++) {
		Polygon& polygon = navmesh_polygons[i];
		polygon.id = i;
		polygon.owner = region_iteration.ptr();
		polygon.surface_area = 0.0;

		Vector<int> polygon_indices = polygons_ptr[i];

		int polygon_size = polygon_indices.size();
		if (polygon_size < 3) {
			continue;
		}

		const int* indices_ptr = polygon_indices.ptr();

		bool polygon_valid = true;

		polygon.vertices.resize(polygon_size);

		{
			real_t _new_polygon_surface_area = 0.0;

			for (int j(2); j < polygon_size; j++) {
				const Triangle2 triangle =
					Triangle2(region_transform.xform(vertices_ptr[indices_ptr[0]]),
						region_transform.xform(vertices_ptr[indices_ptr[j - 1]]),
						region_transform.xform(vertices_ptr[indices_ptr[j]]));

				_new_polygon_surface_area += triangle.get_area();
			}

			polygon.surface_area = _new_polygon_surface_area;
			_new_region_surface_area += _new_polygon_surface_area;
		}

		for (int j(0); j < polygon_size; j++) {
			int vertex_index = indices_ptr[j];
			if (vertex_index < 0 || vertex_index >= vertex_count) {
				polygon_valid = false;
				break;
			}

			const Vector2 point_position = region_transform.xform(vertices_ptr[vertex_index]);
			polygon.vertices[j] = point_position;

			if (first_vertex) {
				first_vertex = false;
				_new_region_bounds.position = point_position;
			}
			else {
				_new_region_bounds.expand_to(point_position);
			}
		}

		if (!polygon_valid) {
			polygon.surface_area = 0.0;
			polygon.vertices.clear();
			ERR_FAIL_COND_MSG(!polygon_valid, "Corrupted navigation mesh set on region. The "
											  "indices of a polygon are out of range.");
		}
	}

	region_iteration->surface_area = _new_region_surface_area;
	region_iteration->bounds = _new_region_bounds;

	performance_data.pm_polygon_count = navmesh_polygons.size();
}

Nav2D::PointKey NavRegionBuilder2D::get_point_key(const Vector2& p_pos, const Vector2& p_cell_size)
{
	const int x = static_cast<int>(Math::floor(p_pos.x / p_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / p_cell_size.y));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	return p;
}

Nav2D::EdgeKey NavRegionBuilder2D::get_edge_key(
	const Vector2& p_vertex1, const Vector2& p_vertex2, const Vector2& p_cell_size)
{
	EdgeKey ek(get_point_key(p_vertex1, p_cell_size), get_point_key(p_vertex2, p_cell_size));
	return ek;
}

void NavRegionBuilder2D::_build_step_find_edge_connection_pairs(NavRegionIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	const Vector2& map_cell_size = r_build.map_cell_size;
	Ref<NavIteration2D> region_iteration = r_build.region_iteration;
	LocalVector<Nav2D::Polygon>& navmesh_polygons = region_iteration->navmesh_polygons;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>& connection_pairs_map =
		r_build.iter_connection_pairs_map;
	connection_pairs_map.clear();

	region_iteration->internal_connections.clear();
	region_iteration->internal_connections.resize(navmesh_polygons.size());

	region_iteration->external_edges.clear();

	int free_edges_count = 0;
	int edge_merge_error_count = 0;

	for (Polygon& poly : region_iteration->navmesh_polygons) {
		for (uint32_t p = 0; p < poly.vertices.size(); p++) {
			const int next_point = (p + 1) % poly.vertices.size();
			const EdgeKey ek =
				get_edge_key(poly.vertices[p], poly.vertices[next_point], map_cell_size);

			HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>::Iterator pair_it =
				connection_pairs_map.find(ek);
			if (!pair_it) {
				pair_it = connection_pairs_map.insert(ek, EdgeConnectionPair());
				performance_data.pm_edge_count += 1;
				++free_edges_count;
			}
			EdgeConnectionPair& pair = pair_it->value;
			if (pair.size < 2) {
				// Add the polygon/edge tuple to this key.
				Connection new_connection;
				new_connection.polygon = &poly;
				new_connection.edge = p;
				new_connection.pathway_start = poly.vertices[p];
				new_connection.pathway_end = poly.vertices[next_point];

				pair.connections[pair.size] = new_connection;
				++pair.size;
				if (pair.size == 2) {
					--free_edges_count;
				}

			}
			else {
				// The edge is already connected with another edge, skip.
				edge_merge_error_count++;
			}
		}
	}

	if (edge_merge_error_count > 0 &&
		GLOBAL_GET_CACHED(bool, "navigation/2d/warnings/navmesh_edge_merge_errors")) {
		WARN_PRINT(
			"Navigation region synchronization had " + itos(edge_merge_error_count) +
			" edge error(s).\nMore than 2 edges tried to occupy the same map rasterization "
			"space.\nThis causes a logical error in the navigation mesh geometry and is commonly "
			"caused by overlap or too densely placed edges.\nConsider baking with a higher "
			"'cell_size', greater geometry margin, and less detailed bake objects to cause fewer "
			"edges.\nConsider lowering the 'navigation/2d/merge_rasterizer_cell_scale' in the "
			"project settings.\nThis warning can be toggled under "
			"'navigation/2d/warnings/navmesh_edge_merge_errors' in the project settings.");
	}

	performance_data.pm_edge_free_count = free_edges_count;
}

void NavRegionBuilder2D::_build_step_merge_edge_connection_pairs(NavRegionIterationBuild2D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	Ref<NavIteration2D> region_iteration = r_build.region_iteration;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>& connection_pairs_map =
		r_build.iter_connection_pairs_map;

	for (const KeyValue<EdgeKey, EdgeConnectionPair>& pair_it : connection_pairs_map) {
		const EdgeConnectionPair& pair = pair_it.value;
		if (pair.size == 2) {
			// Connect edge that are shared in different polygons.
			const Connection& c1 = pair.connections[0];
			const Connection& c2 = pair.connections[1];
			region_iteration->internal_connections[c1.polygon->id].push_back(c2);
			region_iteration->internal_connections[c2.polygon->id].push_back(c1);
			performance_data.pm_edge_merge_count += 1;

		}
		else {
			ERR_FAIL_COND_MSG(
				pair.size != 1, vformat("Number of connection != 1. Found: %d", pair.size));

			const Connection& connection = pair.connections[0];

			ConnectableEdge ce;
			ce.ek = pair_it.key;
			ce.polygon_index = connection.polygon->id;
			ce.edge = connection.edge;
			ce.pathway_start = connection.pathway_start;
			ce.pathway_end = connection.pathway_end;

			region_iteration->external_edges.push_back(ce);
		}
	}
}

void NavRegionBuilder2D::_build_update_iteration(NavRegionIterationBuild2D& r_build)
{
	ERR_FAIL_NULL(r_build.region);
	// Stub. End of the build.
}


#define THREE_POINTS_CROSS_PRODUCT(m_a, m_b, m_c) (-((m_c) - (m_a)).cross((m_b) - (m_a)))

Vector2 NavMeshQueries2D::polygons_get_random_point(
	const LocalVector<Polygon>& p_polygons, uint32_t p_navigation_layers, bool p_uniformly)
{
	const LocalVector<Polygon>& region_polygons = p_polygons;

	if (region_polygons.is_empty()) {
		return Vector2();
	}

	if (p_uniformly) {
		real_t accumulated_area = 0;
		RBMap<real_t, uint32_t> region_area_map;

		for (uint32_t rp_index = 0; rp_index < region_polygons.size(); rp_index++) {
			const Polygon& region_polygon = region_polygons[rp_index];
			real_t polyon_area = region_polygon.surface_area;

			if (polyon_area == 0.0) {
				continue;
			}
			region_area_map[accumulated_area] = rp_index;
			accumulated_area += polyon_area;
		}
		if (region_area_map.is_empty() || accumulated_area == 0) {
			// All polygons have no real surface / no area.
			return Vector2();
		}

		real_t region_area_map_pos = Math::random(real_t(0), accumulated_area);

		RBMap<real_t, uint32_t>::Iterator region_E =
			region_area_map.find_closest(region_area_map_pos);
		ERR_FAIL_COND_V(!region_E, Vector2());
		uint32_t rrp_polygon_index = region_E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(rrp_polygon_index, region_polygons.size(), Vector2());

		const Polygon& rr_polygon = region_polygons[rrp_polygon_index];

		real_t accumulated_polygon_area = 0;
		RBMap<real_t, uint32_t> polygon_area_map;

		for (uint32_t rpp_index = 2; rpp_index < rr_polygon.vertices.size(); rpp_index++) {
			real_t triangle_area = Triangle2(rr_polygon.vertices[0],
				rr_polygon.vertices[rpp_index - 1], rr_polygon.vertices[rpp_index])
									   .get_area();

			if (triangle_area == 0.0) {
				continue;
			}
			polygon_area_map[accumulated_polygon_area] = rpp_index;
			accumulated_polygon_area += triangle_area;
		}
		if (polygon_area_map.is_empty() || accumulated_polygon_area == 0) {
			// All faces have no real surface / no area.
			return Vector2();
		}

		real_t polygon_area_map_pos = Math::random(real_t(0), accumulated_polygon_area);

		RBMap<real_t, uint32_t>::Iterator polygon_E =
			polygon_area_map.find_closest(polygon_area_map_pos);
		ERR_FAIL_COND_V(!polygon_E, Vector2());
		uint32_t rrp_face_index = polygon_E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(rrp_face_index, rr_polygon.vertices.size(), Vector2());

		const Triangle2 triangle(rr_polygon.vertices[0], rr_polygon.vertices[rrp_face_index - 1],
			rr_polygon.vertices[rrp_face_index]);

		Vector2 triangle_random_position = triangle.get_random_point_inside();
		return triangle_random_position;

	}
	else {
		uint32_t rrp_polygon_index = Math::random(int(0), region_polygons.size() - 1);

		const Polygon& rr_polygon = region_polygons[rrp_polygon_index];

		uint32_t rrp_face_index = Math::random(int(2), rr_polygon.vertices.size() - 1);

		const Triangle2 triangle(rr_polygon.vertices[0], rr_polygon.vertices[rrp_face_index - 1],
			rr_polygon.vertices[rrp_face_index]);

		Vector2 triangle_random_position = triangle.get_random_point_inside();
		return triangle_random_position;
	}
}

void NavMeshQueries2D::_query_task_find_start_end_positions(
	NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration)
{
	real_t begin_d = FLT_MAX;
	real_t end_d = FLT_MAX;

	const LocalVector<Ref<NavIteration2D>>& regions = p_map_iteration.region_iterations;

	for (const Ref<NavIteration2D>& region : regions) {
		if (!_query_task_is_connection_owner_usable(p_query_task, region.ptr())) {
			continue;
		}

		// Find the initial poly and the end poly on this map.
		for (const Polygon& p : region->get_navmesh_polygons()) {
			// Only consider the polygon if it in a region with compatible layers.
			if ((p_query_task.navigation_layers & p.owner->get_navigation_layers()) == 0) {
				continue;
			}

			// For each triangle check the distance between the origin/destination.
			for (uint32_t point_id = 2; point_id < p.vertices.size(); point_id++) {
				const Triangle2 triangle(
					p.vertices[0], p.vertices[point_id - 1], p.vertices[point_id]);

				Vector2 point = triangle.get_closest_point_to(p_query_task.start_position);
				real_t distance_to_point = point.distance_to(p_query_task.start_position);
				if (distance_to_point < begin_d) {
					begin_d = distance_to_point;
					p_query_task.begin_polygon = &p;
					p_query_task.begin_position = point;
				}

				point = triangle.get_closest_point_to(p_query_task.target_position);
				distance_to_point = point.distance_to(p_query_task.target_position);
				if (distance_to_point < end_d) {
					end_d = distance_to_point;
					p_query_task.end_polygon = &p;
					p_query_task.end_position = point;
				}
			}
		}
	}
}

void NavMeshQueries2D::_query_task_search_polygon_connections(NavMeshPathQueryTask2D& p_query_task,
	const Connection& p_connection, uint32_t p_least_cost_id,
	const NavigationPoly& p_least_cost_poly, real_t p_poly_enter_cost, const Vector2& p_end_point)
{
	const NavIteration2D* connection_owner = p_connection.polygon->owner;
	ERR_FAIL_NULL(connection_owner);
	const bool owner_is_usable =
		_query_task_is_connection_owner_usable(p_query_task, connection_owner);
	if (!owner_is_usable) {
		return;
	}

	Heap<NavigationPoly*, NavPolyTravelCostGreaterThan, NavPolyHeapIndexer>& traversable_polys =
		p_query_task.path_query_slot->traversable_polys;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	real_t poly_travel_cost = p_least_cost_poly.poly->owner->get_travel_cost();

	Vector2 new_entry = Geometry2D::get_closest_point_to_segment(
		p_least_cost_poly.entry, p_connection.pathway_start, p_connection.pathway_end);
	real_t new_traveled_distance =
		p_least_cost_poly.entry.distance_to(new_entry) * poly_travel_cost + p_poly_enter_cost +
		p_least_cost_poly.traveled_distance;

	// Check if the neighbor polygon has already been processed.
	NavigationPoly& neighbor_poly =
		navigation_polys[p_query_task.path_query_slot->poly_to_id[p_connection.polygon]];
	if (new_traveled_distance < neighbor_poly.traveled_distance) {
		// Add the polygon to the heap of polygons to traverse next.
		neighbor_poly.back_navigation_poly_id = p_least_cost_id;
		neighbor_poly.back_navigation_edge = p_connection.edge;
		neighbor_poly.back_navigation_edge_pathway_start = p_connection.pathway_start;
		neighbor_poly.back_navigation_edge_pathway_end = p_connection.pathway_end;
		neighbor_poly.traveled_distance = new_traveled_distance;
		neighbor_poly.distance_to_destination =
			new_entry.distance_to(p_end_point) * connection_owner->get_travel_cost();
		neighbor_poly.entry = new_entry;

		if (neighbor_poly.traversable_poly_index != traversable_polys.INVALID_INDEX) {
			traversable_polys.shift(neighbor_poly.traversable_poly_index);
		}
		else {
			neighbor_poly.poly = p_connection.polygon;
			traversable_polys.push(&neighbor_poly);
		}
	}
}

void NavMeshQueries2D::_query_task_build_path_corridor(
	NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration)
{
	const Vector2 p_target_position = p_query_task.target_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector2 begin_point = p_query_task.begin_position;
	Vector2 end_point = p_query_task.end_position;

	// Heap of polygons to travel next.
	Heap<NavigationPoly*, NavPolyTravelCostGreaterThan, NavPolyHeapIndexer>& traversable_polys =
		p_query_task.path_query_slot->traversable_polys;
	traversable_polys.clear();

	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;
	for (NavigationPoly& polygon : navigation_polys) {
		polygon.reset();
	}

	// Initialize the matching navigation polygon.
	NavigationPoly& begin_navigation_poly =
		navigation_polys[p_query_task.path_query_slot->poly_to_id[begin_poly]];
	begin_navigation_poly.poly = begin_poly;
	begin_navigation_poly.entry = begin_point;
	begin_navigation_poly.back_navigation_edge_pathway_start = begin_point;
	begin_navigation_poly.back_navigation_edge_pathway_end = begin_point;
	begin_navigation_poly.traveled_distance = 0.f;

	// This is an implementation of the A* algorithm.
	uint32_t least_cost_id = p_query_task.path_query_slot->poly_to_id[begin_poly];
	bool found_route = false;

	const Polygon* reachable_end = nullptr;
	real_t distance_to_reachable_end = FLT_MAX;
	bool is_reachable = true;
	real_t poly_enter_cost = 0.0;

	const HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>&
		navbases_polygons_external_connections =
			p_map_iteration.navbases_polygons_external_connections;

	// True if we reached the max polygon search count or distance from the begin position.
	bool path_search_max_reached = false;

	const float path_search_max_distance_sqr =
		p_query_task.path_search_max_distance * p_query_task.path_search_max_distance;
	bool has_path_search_max_distance = path_search_max_distance_sqr > 0.0;

	int processed_polygon_count = 0;
	bool has_path_search_max_polygons = p_query_task.path_search_max_polygons > 0;

	bool has_path_search_max =
		p_query_task.path_search_max_polygons > 0 || path_search_max_distance_sqr > 0.0;

	while (true) {
		const NavigationPoly& least_cost_poly = navigation_polys[least_cost_id];

		const NavIteration2D* least_cost_navbase = least_cost_poly.poly->owner;

		processed_polygon_count += 1;

		const uint32_t navbase_local_polygon_id = least_cost_poly.poly->id;
		const LocalVector<LocalVector<Connection>>& navbase_polygons_to_connections =
			least_cost_poly.poly->owner->get_internal_connections();

		if (navbase_polygons_to_connections.size() > 0) {
			const LocalVector<Connection>& polygon_connections =
				navbase_polygons_to_connections[navbase_local_polygon_id];

			for (const Connection& connection : polygon_connections) {
				_query_task_search_polygon_connections(p_query_task, connection, least_cost_id,
					least_cost_poly, poly_enter_cost, end_point);
			}
		}

		// Search region external navmesh polygon connections, aka connections to other regions
		// created by outline edge merge or links.
		for (const Connection& connection :
			navbases_polygons_external_connections[least_cost_navbase][navbase_local_polygon_id]) {
			_query_task_search_polygon_connections(p_query_task, connection, least_cost_id,
				least_cost_poly, poly_enter_cost, end_point);
		}

		if (has_path_search_max && !path_search_max_reached) {
			if (has_path_search_max_polygons &&
				processed_polygon_count >= p_query_task.path_search_max_polygons) {
				path_search_max_reached = true;
				traversable_polys.clear();
			}
			else if (has_path_search_max_distance &&
					   begin_point.distance_squared_to(least_cost_poly.entry) >
						   path_search_max_distance_sqr) {
				path_search_max_reached = true;
				traversable_polys.clear();
			}
		}

		poly_enter_cost = 0;
		// When the heap of traversable polygons is empty at this point it means the end polygon is
		// unreachable.
		if (traversable_polys.is_empty()) {
			// Thus use the further reachable polygon
			ERR_BREAK_MSG(is_reachable == false,
				"Invalid navigation index or connection pointers. Check "
				"preceding navmesh geometry or placement errors.");
			is_reachable = false;
			if (reachable_end == nullptr) {
				// The path is not found and there is not a way out.
				break;
			}

			// Set as end point the furthest reachable point.
			end_poly = reachable_end;
			real_t end_d = FLT_MAX;

			for (uint32_t point_id = 2; point_id < end_poly->vertices.size(); point_id++) {
				Triangle2 t(end_poly->vertices[0], end_poly->vertices[point_id - 1],
					end_poly->vertices[point_id]);
				Vector2 spoint = t.get_closest_point_to(p_target_position);
				real_t dpoint = spoint.distance_squared_to(p_target_position);
				if (dpoint < end_d) {
					end_point = spoint;
					end_d = dpoint;
				}
			}

			// Search all faces of start polygon as well.
			bool closest_point_on_start_poly = false;

			for (uint32_t point_id = 2; point_id < begin_poly->vertices.size(); point_id++) {
				Triangle2 t(begin_poly->vertices[0], begin_poly->vertices[point_id - 1],
					begin_poly->vertices[point_id]);
				Vector2 spoint = t.get_closest_point_to(p_target_position);
				real_t dpoint = spoint.distance_squared_to(p_target_position);
				if (dpoint < end_d) {
					end_point = spoint;
					end_d = dpoint;
					closest_point_on_start_poly = true;
				}
			}

			if (closest_point_on_start_poly) {
				// No point to run PostProcessing when start and end convex polygon is the same.
				p_query_task.path_clear();

				_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
				_query_task_push_back_point_with_metadata(p_query_task, end_point, begin_poly);
				p_query_task.status = NavMeshPathQueryTask2D::TaskStatus::QUERY_FINISHED;
				return;
			}

			for (NavigationPoly& nav_poly : navigation_polys) {
				nav_poly.poly = nullptr;
				nav_poly.traveled_distance = FLT_MAX;
			}
			uint32_t _bp_id = p_query_task.path_query_slot->poly_to_id[begin_poly];
			navigation_polys[_bp_id].poly = begin_poly;
			navigation_polys[_bp_id].traveled_distance = 0;
			least_cost_id = _bp_id;
			reachable_end = nullptr;
		}
		else {
			// Pop the polygon with the lowest travel cost from the heap of traversable polygons.
			least_cost_id = p_query_task.path_query_slot->poly_to_id[traversable_polys.pop()->poly];

			// Store the farthest reachable end polygon in case our goal is not reachable.
			if (is_reachable) {
				real_t distance =
					navigation_polys[least_cost_id].entry.distance_squared_to(p_target_position);
				if (distance_to_reachable_end > distance) {
					distance_to_reachable_end = distance;
					reachable_end = navigation_polys[least_cost_id].poly;
				}
			}

			// Check if we reached the end
			if (navigation_polys[least_cost_id].poly == end_poly) {
				found_route = true;
				break;
			}

			if (navigation_polys[least_cost_id].poly->owner->get_self() !=
				least_cost_poly.poly->owner->get_self()) {
				ERR_FAIL_NULL(least_cost_poly.poly->owner);
				poly_enter_cost = least_cost_poly.poly->owner->get_enter_cost();
			}
		}
	}

	// We did not find a route but we have both a start polygon and an end polygon at this point.
	// Usually this happens because there was not a single external or internal connected edge, e.g.
	// our start polygon is an isolated, single convex polygon.
	if (!found_route) {
		real_t end_d = FLT_MAX;
		// Search all faces of the start polygon for the closest point to our target position.

		for (uint32_t point_id = 2; point_id < begin_poly->vertices.size(); point_id++) {
			Triangle2 t(begin_poly->vertices[0], begin_poly->vertices[point_id - 1],
				begin_poly->vertices[point_id]);
			Vector2 spoint = t.get_closest_point_to(p_target_position);
			real_t dpoint = spoint.distance_squared_to(p_target_position);
			if (dpoint < end_d) {
				end_point = spoint;
				end_d = dpoint;
			}
		}

		p_query_task.path_clear();

		_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
		_query_task_push_back_point_with_metadata(p_query_task, end_point, begin_poly);
		p_query_task.status = NavMeshPathQueryTask2D::TaskStatus::QUERY_FINISHED;
	}
	else {
		p_query_task.end_position = end_point;
		p_query_task.end_polygon = end_poly;
		p_query_task.begin_position = begin_point;
		p_query_task.begin_polygon = begin_poly;
		p_query_task.least_cost_id = least_cost_id;
	}
}

float NavMeshQueries2D::_calculate_path_length(
	const LocalVector<Vector2>& p_path, uint32_t p_start_index, uint32_t p_end_index)
{
	const uint32_t path_size = p_path.size();
	if (path_size < 2) {
		return 0.0;
	}

	ERR_FAIL_COND_V(p_start_index >= p_end_index, 0.0);
	ERR_FAIL_COND_V(p_start_index >= path_size - 1, 0.0);
	ERR_FAIL_COND_V(p_end_index >= path_size, 0.0);

	const Vector2* path_ptr = p_path.ptr();

	float path_length = 0.0;

	for (uint32_t i = p_start_index; i < p_end_index; i++) {
		const Vector2& vertex1 = path_ptr[i];
		const Vector2& vertex2 = path_ptr[i + 1];
		float edge_length = vertex1.distance_to(vertex2);
		path_length += edge_length;
	}

	return path_length;
}

void NavMeshQueries2D::_query_task_post_process_corridorfunnel(NavMeshPathQueryTask2D& p_query_task)
{
	Vector2 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector2 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	// Set the apex poly/point to the end point.
	NavigationPoly* apex_poly = &navigation_polys[least_cost_id];

	const Vector2 back_edge_closest_point = Geometry2D::get_closest_point_to_segment(end_point,
		apex_poly->back_navigation_edge_pathway_start, apex_poly->back_navigation_edge_pathway_end);
	if (end_point.is_equal_approx(back_edge_closest_point)) {
		// The end point is basically on top of the last crossed edge, funneling around the corners
		// would at best do nothing. At worst it would add an unwanted path point before the last
		// point due to precision issues so skip to the next polygon.
		if (apex_poly->back_navigation_poly_id != -1) {
			apex_poly = &navigation_polys[apex_poly->back_navigation_poly_id];
		}
	}

	Vector2 apex_point = end_point;

	NavigationPoly* left_poly = apex_poly;
	Vector2 left_portal = apex_point;
	NavigationPoly* right_poly = apex_poly;
	Vector2 right_portal = apex_point;

	NavigationPoly* p = apex_poly;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	while (p) {
		// Set left and right points of the pathway between polygons.
		Vector2 left = p->back_navigation_edge_pathway_start;
		Vector2 right = p->back_navigation_edge_pathway_end;
		if (THREE_POINTS_CROSS_PRODUCT(apex_point, left, right) < 0) {
			SWAP(left, right);
		}

		bool skip = false;
		if (THREE_POINTS_CROSS_PRODUCT(apex_point, left_portal, left) >= 0) {
			// Process.
			if (left_portal == apex_point ||
				THREE_POINTS_CROSS_PRODUCT(apex_point, left, right_portal) > 0) {
				left_poly = p;
				left_portal = left;
			}
			else {
				_query_task_clip_path(p_query_task, apex_poly, right_portal, right_poly);

				apex_point = right_portal;
				p = right_poly;
				left_poly = p;
				apex_poly = p;
				left_portal = apex_point;
				right_portal = apex_point;

				_query_task_push_back_point_with_metadata(
					p_query_task, apex_point, apex_poly->poly);
				skip = true;
			}
		}

		if (!skip && THREE_POINTS_CROSS_PRODUCT(apex_point, right_portal, right) <= 0) {
			// Process.
			if (right_portal == apex_point ||
				THREE_POINTS_CROSS_PRODUCT(apex_point, right, left_portal) < 0) {
				right_poly = p;
				right_portal = right;
			}
			else {
				_query_task_clip_path(p_query_task, apex_poly, left_portal, left_poly);

				apex_point = left_portal;
				p = left_poly;
				right_poly = p;
				apex_poly = p;
				right_portal = apex_point;
				left_portal = apex_point;

				_query_task_push_back_point_with_metadata(
					p_query_task, apex_point, apex_poly->poly);
			}
		}

		// Go to the previous polygon.
		if (p->back_navigation_poly_id != -1) {
			p = &navigation_polys[p->back_navigation_poly_id];
		}
		else {
			// The end
			p = nullptr;
		}
	}

	// If the last point is not the begin point, add it to the list.
	if (p_query_task.path_points[p_query_task.path_points.size() - 1] != begin_point) {
		_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
	}
}

void NavMeshQueries2D::_query_task_post_process_edgecentered(NavMeshPathQueryTask2D& p_query_task)
{
	Vector2 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector2 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	// Add mid points.
	int np_id = least_cost_id;
	while (np_id != -1 && navigation_polys[np_id].back_navigation_poly_id != -1) {
		if (navigation_polys[np_id].back_navigation_edge != -1) {
			int prev = navigation_polys[np_id].back_navigation_edge;
			int prev_n = (navigation_polys[np_id].back_navigation_edge + 1) %
						 navigation_polys[np_id].poly->vertices.size();
			Vector2 point = (navigation_polys[np_id].poly->vertices[prev] +
								navigation_polys[np_id].poly->vertices[prev_n]) *
							0.5;

			_query_task_push_back_point_with_metadata(
				p_query_task, point, navigation_polys[np_id].poly);
		}
		else {
			_query_task_push_back_point_with_metadata(
				p_query_task, navigation_polys[np_id].entry, navigation_polys[np_id].poly);
		}

		np_id = navigation_polys[np_id].back_navigation_poly_id;
	}

	_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
}

void NavMeshQueries2D::_query_task_post_process_nopostprocessing(
	NavMeshPathQueryTask2D& p_query_task)
{
	Vector2 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector2 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	// Add mid points.
	int np_id = least_cost_id;
	while (np_id != -1 && navigation_polys[np_id].back_navigation_poly_id != -1) {
		_query_task_push_back_point_with_metadata(
			p_query_task, navigation_polys[np_id].entry, navigation_polys[np_id].poly);

		np_id = navigation_polys[np_id].back_navigation_poly_id;
	}

	_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
}

Vector2 NavMeshQueries2D::map_iteration_get_closest_point(
	const NavMapIteration2D& p_map_iteration, const Vector2& p_point)
{
	ClosestPointQueryResult cp = map_iteration_get_closest_point_info(p_map_iteration, p_point);
	return cp.point;
}

RID NavMeshQueries2D::map_iteration_get_closest_point_owner(
	const NavMapIteration2D& p_map_iteration, const Vector2& p_point)
{
	ClosestPointQueryResult cp = map_iteration_get_closest_point_info(p_map_iteration, p_point);
	return cp.owner;
}

ClosestPointQueryResult NavMeshQueries2D::map_iteration_get_closest_point_info(
	const NavMapIteration2D& p_map_iteration, const Vector2& p_point)
{
	ClosestPointQueryResult result;
	real_t closest_point_distance_squared = FLT_MAX;

	// TODO: Check for further 2D improvements.

	const LocalVector<Ref<NavIteration2D>>& regions = p_map_iteration.region_iterations;
	for (const Ref<NavIteration2D>& region : regions) {
		for (const Polygon& polygon : region->get_navmesh_polygons()) {
			real_t cross = -(polygon.vertices[1] - polygon.vertices[0])
								.cross(polygon.vertices[2] - polygon.vertices[0]);
			Vector2 closest_on_polygon;
			real_t closest = FLT_MAX;
			bool inside = true;
			Vector2 previous = polygon.vertices[polygon.vertices.size() - 1];
			for (uint32_t point_id = 0; point_id < polygon.vertices.size(); ++point_id) {
				Vector2 edge = polygon.vertices[point_id] - previous;
				Vector2 to_point = p_point - previous;
				real_t edge_to_point_cross = -edge.cross(to_point);
				bool clockwise = (edge_to_point_cross * cross) > 0;
				// If we are not clockwise, the point will never be inside the polygon and so the
				// closest point will be on an edge.
				if (!clockwise) {
					inside = false;
					real_t point_projected_on_edge = edge.dot(to_point);
					real_t edge_square = edge.length_squared();

					if (point_projected_on_edge > edge_square) {
						real_t distance = polygon.vertices[point_id].distance_squared_to(p_point);
						if (distance < closest) {
							closest_on_polygon = polygon.vertices[point_id];
							closest = distance;
						}
					}
					else if (point_projected_on_edge < 0.0) {
						real_t distance = previous.distance_squared_to(p_point);
						if (distance < closest) {
							closest_on_polygon = previous;
							closest = distance;
						}
					}
					else {
						// If we project on this edge, this will be the closest point.
						real_t percent = point_projected_on_edge / edge_square;
						closest_on_polygon = previous + percent * edge;
						break;
					}
				}
				previous = polygon.vertices[point_id];
			}

			if (inside) {
				closest_point_distance_squared = 0.0;
				result.point = p_point;
				result.owner = polygon.owner->get_self();

				break;
			}
			else {
				real_t distance = closest_on_polygon.distance_squared_to(p_point);
				if (distance < closest_point_distance_squared) {
					closest_point_distance_squared = distance;
					result.point = closest_on_polygon;
					result.owner = polygon.owner->get_self();
				}
			}
		}
	}

	return result;
}

Vector2 NavMeshQueries2D::map_iteration_get_random_point(
	const NavMapIteration2D& p_map_iteration, uint32_t p_navigation_layers, bool p_uniformly)
{
	if (p_map_iteration.region_iterations.is_empty()) {
		return Vector2();
	}

	LocalVector<uint32_t> accessible_regions;
	accessible_regions.reserve(p_map_iteration.region_iterations.size());

	for (uint32_t i = 0; i < p_map_iteration.region_iterations.size(); i++) {
		const Ref<NavIteration2D>& region = p_map_iteration.region_iterations[i];
		if (!region->get_enabled() ||
			(p_navigation_layers & region->get_navigation_layers()) == 0) {
			continue;
		}
		accessible_regions.push_back(i);
	}

	if (accessible_regions.is_empty()) {
		// All existing region polygons are disabled.
		return Vector2();
	}

	if (p_uniformly) {
		real_t accumulated_region_surface_area = 0;
		RBMap<real_t, uint32_t> accessible_regions_area_map;

		for (uint32_t accessible_region_index = 0;
			 accessible_region_index < accessible_regions.size(); accessible_region_index++) {
			const Ref<NavIteration2D>& region =
				p_map_iteration.region_iterations[accessible_regions[accessible_region_index]];

			real_t region_surface_area = region->surface_area;

			if (region_surface_area == 0.0f) {
				continue;
			}

			accessible_regions_area_map[accumulated_region_surface_area] = accessible_region_index;
			accumulated_region_surface_area += region_surface_area;
		}
		if (accessible_regions_area_map.is_empty() || accumulated_region_surface_area == 0) {
			// All faces have no real surface / no area.
			return Vector2();
		}

		real_t random_accessible_regions_area_map =
			Math::random(real_t(0), accumulated_region_surface_area);

		RBMap<real_t, uint32_t>::Iterator E =
			accessible_regions_area_map.find_closest(random_accessible_regions_area_map);
		ERR_FAIL_COND_V(!E, Vector2());
		uint32_t random_region_index = E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(random_region_index, accessible_regions.size(), Vector2());

		const Ref<NavIteration2D>& random_region =
			p_map_iteration.region_iterations[accessible_regions[random_region_index]];

		return NavMeshQueries2D::polygons_get_random_point(
			random_region->navmesh_polygons, p_navigation_layers, p_uniformly);

	}
	else {
		uint32_t random_region_index = Math::random(int(0), accessible_regions.size() - 1);

		const Ref<NavIteration2D>& random_region =
			p_map_iteration.region_iterations[accessible_regions[random_region_index]];

		return NavMeshQueries2D::polygons_get_random_point(
			random_region->navmesh_polygons, p_navigation_layers, p_uniformly);
	}
}

Vector2 NavMeshQueries2D::polygons_get_closest_point(
	const LocalVector<Polygon>& p_polygons, const Vector2& p_point)
{
	ClosestPointQueryResult cp = polygons_get_closest_point_info(p_polygons, p_point);
	return cp.point;
}

ClosestPointQueryResult NavMeshQueries2D::polygons_get_closest_point_info(
	const LocalVector<Polygon>& p_polygons, const Vector2& p_point)
{
	ClosestPointQueryResult result;
	real_t closest_point_distance_squared = FLT_MAX;

	// TODO: Check for further 2D improvements.

	for (const Polygon& polygon : p_polygons) {
		real_t cross = -(polygon.vertices[1] - polygon.vertices[0])
							.cross(polygon.vertices[2] - polygon.vertices[0]);
		Vector2 closest_on_polygon;
		real_t closest = FLT_MAX;
		bool inside = true;
		Vector2 previous = polygon.vertices[polygon.vertices.size() - 1];
		for (uint32_t point_id = 0; point_id < polygon.vertices.size(); ++point_id) {
			Vector2 edge = polygon.vertices[point_id] - previous;
			Vector2 to_point = p_point - previous;
			real_t edge_to_point_cross = -edge.cross(to_point);
			bool clockwise = (edge_to_point_cross * cross) > 0;
			// If we are not clockwise, the point will never be inside the polygon and so the
			// closest point will be on an edge.
			if (!clockwise) {
				inside = false;
				real_t point_projected_on_edge = edge.dot(to_point);
				real_t edge_square = edge.length_squared();

				if (point_projected_on_edge > edge_square) {
					real_t distance = polygon.vertices[point_id].distance_squared_to(p_point);
					if (distance < closest) {
						closest_on_polygon = polygon.vertices[point_id];
						closest = distance;
					}
				}
				else if (point_projected_on_edge < 0.0) {
					real_t distance = previous.distance_squared_to(p_point);
					if (distance < closest) {
						closest_on_polygon = previous;
						closest = distance;
					}
				}
				else {
					// If we project on this edge, this will be the closest point.
					real_t percent = point_projected_on_edge / edge_square;
					closest_on_polygon = previous + percent * edge;
					break;
				}
			}
			previous = polygon.vertices[point_id];
		}

		if (inside) {
			closest_point_distance_squared = 0.0;
			result.point = p_point;
			result.owner = polygon.owner->get_self();
			break;
		}
		else {
			real_t distance = closest_on_polygon.distance_squared_to(p_point);
			if (distance < closest_point_distance_squared) {
				closest_point_distance_squared = distance;
				result.point = closest_on_polygon;
				result.owner = polygon.owner->get_self();
			}
		}
	}

	return result;
}

RID NavMeshQueries2D::polygons_get_closest_point_owner(
	const LocalVector<Polygon>& p_polygons, const Vector2& p_point)
{
	ClosestPointQueryResult cp = polygons_get_closest_point_info(p_polygons, p_point);
	return cp.owner;
}

static bool _line_intersects_segment(const Vector2& p_line_normal, real_t p_line_d,
	const Vector2& p_segment_begin, const Vector2& p_segment_end, Vector2& r_intersection)
{
	Vector2 segment = p_segment_begin - p_segment_end;
	real_t den = p_line_normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (p_line_normal.dot(p_segment_begin) - p_line_d) / den;

	if (dist < (real_t)-CMP_EPSILON || dist > (1.0 + (real_t)CMP_EPSILON)) {
		return false;
	}

	r_intersection = p_segment_begin - segment * dist;
	return true;
}

void NavMeshQueries2D::_query_task_clip_path(NavMeshPathQueryTask2D& p_query_task,
	const NavigationPoly* p_from_poly, const Vector2& p_to_point, const NavigationPoly* p_to_poly)
{
	Vector2 from = p_query_task.path_points[p_query_task.path_points.size() - 1];
	const LocalVector<NavigationPoly>& p_navigation_polys =
		p_query_task.path_query_slot->path_corridor;

	if (from.is_equal_approx(p_to_point)) {
		return;
	}

	// Compute line parameters (equivalent to the Plane case in 3D).
	const Vector2 normal = -(from - p_to_point).orthogonal().normalized();
	const real_t d = normal.dot(from);

	while (p_from_poly != p_to_poly) {
		Vector2 pathway_start = p_from_poly->back_navigation_edge_pathway_start;
		Vector2 pathway_end = p_from_poly->back_navigation_edge_pathway_end;

		ERR_FAIL_COND(p_from_poly->back_navigation_poly_id == -1);
		p_from_poly = &p_navigation_polys[p_from_poly->back_navigation_poly_id];

		if (!pathway_start.is_equal_approx(pathway_end)) {
			Vector2 inters;
			if (_line_intersects_segment(normal, d, pathway_start, pathway_end, inters)) {
				if (!inters.is_equal_approx(p_to_point) &&
					!inters.is_equal_approx(
						p_query_task.path_points[p_query_task.path_points.size() - 1])) {
					_query_task_push_back_point_with_metadata(
						p_query_task, inters, p_from_poly->poly);
				}
			}
		}
	}
}

bool NavMeshQueries2D::_query_task_is_connection_owner_usable(
	const NavMeshPathQueryTask2D& p_query_task, const NavIteration2D* p_owner)
{
	ERR_FAIL_NULL_V(p_owner, false);

	bool owner_usable = true;

	if (!p_owner->get_enabled()) {
		owner_usable = false;
		return owner_usable;
	}

	if ((p_query_task.navigation_layers & p_owner->get_navigation_layers()) == 0) {
		// Not usable. No matching bit between task filter bitmask and owner bitmask.
		owner_usable = false;
		return owner_usable;
	}

	if (p_query_task.exclude_regions || p_query_task.include_regions) {
		switch (p_owner->get_type()) {
		case NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_REGION: {
			if (p_query_task.exclude_regions &&
				p_query_task.excluded_regions.has(p_owner->get_self())) {
				// Not usable. Exclude region filter is active and this region is excluded.
				owner_usable = false;
			}
			else if (p_query_task.include_regions &&
					   !p_query_task.included_regions.has(p_owner->get_self())) {
				// Not usable. Include region filter is active and this region is not included.
				owner_usable = false;
			}
		} break;
		case NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_LINK: {
			const LocalVector<Polygon>& link_polygons = p_owner->get_navmesh_polygons();
			if (link_polygons.size() != 2) {
				// Not usable. Whatever this is, it is not a valid connected link.
				owner_usable = false;
			}
			else {
				const RID link_start_region = link_polygons[0].owner->get_self();
				const RID link_end_region = link_polygons[1].owner->get_self();
				if (p_query_task.exclude_regions &&
					(p_query_task.excluded_regions.has(link_start_region) ||
						p_query_task.excluded_regions.has(link_end_region))) {
					// Not usable. Exclude region filter is active and at least one region of the
					// link is excluded.
					owner_usable = false;
				}
				if (p_query_task.include_regions &&
					(!p_query_task.included_regions.has(link_start_region) ||
						!p_query_task.excluded_regions.has(link_end_region))) {
					// Not usable. Include region filter is active and not both regions of the links
					// are included.
					owner_usable = false;
				}
			}
		} break;
		}
	}

	return owner_usable;
}

LocalVector<uint32_t> NavMeshQueries2D::get_simplified_path_indices(
	const LocalVector<Vector2>& p_path, real_t p_epsilon)
{
	p_epsilon = MAX(0.0, p_epsilon);
	real_t squared_epsilon = p_epsilon * p_epsilon;

	LocalVector<uint32_t> simplified_path_indices;
	simplified_path_indices.reserve(p_path.size());
	simplified_path_indices.push_back(0);
	simplify_path_segment(0, p_path.size() - 1, p_path, squared_epsilon, simplified_path_indices);
	simplified_path_indices.push_back(p_path.size() - 1);

	return simplified_path_indices;
}

void NavMeshQueries2D::simplify_path_segment(int p_start_inx, int p_end_inx,
	const LocalVector<Vector2>& p_points, real_t p_epsilon,
	LocalVector<uint32_t>& r_simplified_path_indices)
{
	real_t point_max_distance = 0.0;
	int point_max_index = 0;

	for (int i = p_start_inx; i < p_end_inx; i++) {
		const Vector2& checked_point = p_points[i];

		const Vector2 closest_point = Geometry2D::get_closest_point_to_segment(
			checked_point, p_points[p_start_inx], p_points[p_end_inx]);
		real_t distance_squared = closest_point.distance_squared_to(checked_point);

		if (distance_squared > point_max_distance) {
			point_max_index = i;
			point_max_distance = distance_squared;
		}
	}

	if (point_max_distance > p_epsilon) {
		simplify_path_segment(
			p_start_inx, point_max_index, p_points, p_epsilon, r_simplified_path_indices);
		r_simplified_path_indices.push_back(point_max_index);
		simplify_path_segment(
			point_max_index, p_end_inx, p_points, p_epsilon, r_simplified_path_indices);
	}
}

void NavMeshQueries2D::_query_task_push_back_point_with_metadata(
	NavMeshPathQueryTask2D& p_query_task, const Vector2& p_point,
	const Nav2D::Polygon* p_point_polygon)
{
}

void NavMeshQueries2D::query_task_map_iteration_get_path(
	NavMeshQueries2D::NavMeshPathQueryTask2D& p_task, const NavMapIteration2D& p_map_iteration)
{
}

#undef THREE_POINTS_CROSS_PRODUCT
