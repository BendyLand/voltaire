/**************************************************************************/
/*  nav_map_3d.cpp                                                        */
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

#include "nav_map_3d.h"
#include "core/config/engine.h"
#include "core/os/thread.h"
#include "register_types.h"
#include "scene/3d/node_3d.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "thirdparty/recastnavigation/Recast/Include/Recast.h"

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "core/templates/rb_map.h"
#include "core/types.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "thirdparty/rvo2/rvo2_2d/Obstacle2d.h"

using namespace Nav3D;
using namespace NavigationDefaults3D;
using namespace NavigationEnums3D;

void NavAgent3D::set_avoidance_enabled(bool p_enabled)
{
	avoidance_enabled = p_enabled;
	_update_rvo_agent_properties();
}

void NavAgent3D::set_use_3d_avoidance(bool p_enabled)
{
	use_3d_avoidance = p_enabled;
	_update_rvo_agent_properties();
}

void NavAgent3D::_update_rvo_agent_properties()
{
	if (use_3d_avoidance) {
		rvo_agent_3d.neighborDist_ = neighbor_distance;
		rvo_agent_3d.maxNeighbors_ = max_neighbors;
		rvo_agent_3d.timeHorizon_ = time_horizon_agents;
		rvo_agent_3d.timeHorizonObst_ = time_horizon_obstacles;
		rvo_agent_3d.radius_ = radius;
		rvo_agent_3d.maxSpeed_ = max_speed;
		rvo_agent_3d.position_ = RVO3D::Vector3(position.x, position.y, position.z);
		// Replacing the internal velocity directly causes major jitter / bugs due to unpredictable
		// velocity jumps, left line here for testing.
		// rvo_agent_3d.velocity_ = RVO3D::Vector3(velocity.x, velocity.y ,velocity.z);
		rvo_agent_3d.prefVelocity_ = RVO3D::Vector3(velocity.x, velocity.y, velocity.z);
		rvo_agent_3d.height_ = height;
		rvo_agent_3d.avoidance_layers_ = avoidance_layers;
		rvo_agent_3d.avoidance_mask_ = avoidance_mask;
		rvo_agent_3d.avoidance_priority_ = avoidance_priority;
	}
	else {
		rvo_agent_2d.neighborDist_ = neighbor_distance;
		rvo_agent_2d.maxNeighbors_ = max_neighbors;
		rvo_agent_2d.timeHorizon_ = time_horizon_agents;
		rvo_agent_2d.timeHorizonObst_ = time_horizon_obstacles;
		rvo_agent_2d.radius_ = radius;
		rvo_agent_2d.maxSpeed_ = max_speed;
		rvo_agent_2d.position_ = RVO2D::Vector2(position.x, position.z);
		rvo_agent_2d.elevation_ = position.y;
		// Replacing the internal velocity directly causes major jitter / bugs due to unpredictable
		// velocity jumps, left line here for testing.
		// rvo_agent_2d.velocity_ = RVO2D::Vector2(velocity.x, velocity.z);
		rvo_agent_2d.prefVelocity_ = RVO2D::Vector2(velocity.x, velocity.z);
		rvo_agent_2d.height_ = height;
		rvo_agent_2d.avoidance_layers_ = avoidance_layers;
		rvo_agent_2d.avoidance_mask_ = avoidance_mask;
		rvo_agent_2d.avoidance_priority_ = avoidance_priority;
	}

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

void NavAgent3D::set_map(NavMap3D* p_map)
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

bool NavAgent3D::is_map_changed()
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

void NavAgent3D::set_neighbor_distance(real_t p_neighbor_distance)
{
	neighbor_distance = p_neighbor_distance;
	if (use_3d_avoidance) {
		rvo_agent_3d.neighborDist_ = neighbor_distance;
	}
	else {
		rvo_agent_2d.neighborDist_ = neighbor_distance;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_max_neighbors(int p_max_neighbors)
{
	max_neighbors = p_max_neighbors;
	if (use_3d_avoidance) {
		rvo_agent_3d.maxNeighbors_ = max_neighbors;
	}
	else {
		rvo_agent_2d.maxNeighbors_ = max_neighbors;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_time_horizon_agents(real_t p_time_horizon)
{
	time_horizon_agents = p_time_horizon;
	if (use_3d_avoidance) {
		rvo_agent_3d.timeHorizon_ = time_horizon_agents;
	}
	else {
		rvo_agent_2d.timeHorizon_ = time_horizon_agents;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	time_horizon_obstacles = p_time_horizon;
	if (use_3d_avoidance) {
		rvo_agent_3d.timeHorizonObst_ = time_horizon_obstacles;
	}
	else {
		rvo_agent_2d.timeHorizonObst_ = time_horizon_obstacles;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	if (use_3d_avoidance) {
		rvo_agent_3d.radius_ = radius;
	}
	else {
		rvo_agent_2d.radius_ = radius;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_height(real_t p_height)
{
	height = p_height;
	if (use_3d_avoidance) {
		rvo_agent_3d.height_ = height;
	}
	else {
		rvo_agent_2d.height_ = height;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_max_speed(real_t p_max_speed)
{
	max_speed = p_max_speed;
	if (avoidance_enabled) {
		if (use_3d_avoidance) {
			rvo_agent_3d.maxSpeed_ = max_speed;
		}
		else {
			rvo_agent_2d.maxSpeed_ = max_speed;
		}
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_position(const Vector3 p_position)
{
	position = p_position;
	if (avoidance_enabled) {
		if (use_3d_avoidance) {
			rvo_agent_3d.position_ = RVO3D::Vector3(p_position.x, p_position.y, p_position.z);
		}
		else {
			rvo_agent_2d.elevation_ = p_position.y;
			rvo_agent_2d.position_ = RVO2D::Vector2(p_position.x, p_position.z);
		}
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_target_position(const Vector3 p_target_position)
{
	target_position = p_target_position;
}

void NavAgent3D::set_velocity(const Vector3 p_velocity)
{
	// Sets the "wanted" velocity for an agent as a suggestion
	// This velocity is not guaranteed, RVO simulation will only try to fulfill it
	velocity = p_velocity;
	if (avoidance_enabled) {
		if (use_3d_avoidance) {
			rvo_agent_3d.prefVelocity_ = RVO3D::Vector3(velocity.x, velocity.y, velocity.z);
		}
		else {
			rvo_agent_2d.prefVelocity_ = RVO2D::Vector2(velocity.x, velocity.z);
		}
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_velocity_forced(const Vector3 p_velocity)
{
	// This function replaces the internal rvo simulation velocity
	// should only be used after the agent was teleported
	// as it destroys consistency in movement in cramped situations
	// use velocity instead to update with a safer "suggestion"
	velocity_forced = p_velocity;
	if (avoidance_enabled) {
		if (use_3d_avoidance) {
			rvo_agent_3d.velocity_ = RVO3D::Vector3(p_velocity.x, p_velocity.y, p_velocity.z);
		}
		else {
			rvo_agent_2d.velocity_ = RVO2D::Vector2(p_velocity.x, p_velocity.z);
		}
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::update()
{
	// Updates this agent with the calculated results from the rvo simulation
	if (avoidance_enabled) {
		if (use_3d_avoidance) {
			velocity = Vector3(
				rvo_agent_3d.velocity_.x(), rvo_agent_3d.velocity_.y(), rvo_agent_3d.velocity_.z());
		}
		else {
			velocity = Vector3(rvo_agent_2d.velocity_.x(), 0.0, rvo_agent_2d.velocity_.y());
		}
	}
}

void NavAgent3D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	if (use_3d_avoidance) {
		rvo_agent_3d.avoidance_mask_ = avoidance_mask;
	}
	else {
		rvo_agent_2d.avoidance_mask_ = avoidance_mask;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	if (use_3d_avoidance) {
		rvo_agent_3d.avoidance_layers_ = avoidance_layers;
	}
	else {
		rvo_agent_2d.avoidance_layers_ = avoidance_layers;
	}
	agent_dirty = true;

	request_sync();
}

void NavAgent3D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	if (use_3d_avoidance) {
		rvo_agent_3d.avoidance_priority_ = avoidance_priority;
	}
	else {
		rvo_agent_2d.avoidance_priority_ = avoidance_priority;
	}
	agent_dirty = true;

	request_sync();
}

bool NavAgent3D::is_dirty() const { return agent_dirty; }

void NavAgent3D::sync() { agent_dirty = false; }

void NavAgent3D::set_paused(bool p_paused)
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

bool NavAgent3D::get_paused() const { return paused; }

void NavAgent3D::request_sync()
{
	if (map && !sync_dirty_request_list_element.in_list()) {
		map->add_agent_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

void NavAgent3D::cancel_sync_request()
{
	if (map && sync_dirty_request_list_element.in_list()) {
		map->remove_agent_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

NavAgent3D::NavAgent3D() : sync_dirty_request_list_element(this) {}

NavAgent3D::~NavAgent3D() { cancel_sync_request(); }



void NavAgent3D::dispatch_avoidance_callback() {}

bool NavAgent3D::has_avoidance_callback() const { return false; }


void NavObstacle3D::set_agent(NavAgent3D *p_agent) {
	if (agent == p_agent) {
		return;
	}

	agent = p_agent;

	internal_update_agent();

	request_sync();
}

void NavObstacle3D::set_avoidance_enabled(bool p_enabled) {
	if (avoidance_enabled == p_enabled) {
		return;
	}

	avoidance_enabled = p_enabled;
	obstacle_dirty = true;

	internal_update_agent();

	request_sync();
}

void NavObstacle3D::set_use_3d_avoidance(bool p_enabled) {
	if (use_3d_avoidance == p_enabled) {
		return;
	}

	use_3d_avoidance = p_enabled;
	obstacle_dirty = true;

	if (agent) {
		agent->set_use_3d_avoidance(use_3d_avoidance);
	}

	request_sync();
}

void NavObstacle3D::set_map(NavMap3D *p_map) {
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

void NavObstacle3D::set_position(const Vector3 p_position) {
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

void NavObstacle3D::set_radius(real_t p_radius) {
	if (radius == p_radius) {
		return;
	}

	radius = p_radius;

	if (agent) {
		agent->set_radius(radius);
	}
}

void NavObstacle3D::set_height(const real_t p_height) {
	if (height == p_height) {
		return;
	}

	height = p_height;
	obstacle_dirty = true;

	if (agent) {
		agent->set_height(height);
	}

	request_sync();
}

void NavObstacle3D::set_velocity(const Vector3 p_velocity) {
	velocity = p_velocity;

	if (agent) {
		agent->set_velocity(velocity);
	}
}

void NavObstacle3D::set_vertices(const Vector<Vector3> &p_vertices) {
	if (vertices == p_vertices) {
		return;
	}

	vertices = p_vertices;
	obstacle_dirty = true;

	request_sync();
}

bool NavObstacle3D::is_map_changed() {
	if (map) {
		bool is_changed = map->get_iteration_id() != last_map_iteration_id;
		last_map_iteration_id = map->get_iteration_id();
		return is_changed;
	} else {
		return false;
	}
}

void NavObstacle3D::set_avoidance_layers(uint32_t p_layers) {
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

bool NavObstacle3D::is_dirty() const {
	return obstacle_dirty;
}

void NavObstacle3D::sync() {
	obstacle_dirty = false;
}

void NavObstacle3D::internal_update_agent() {
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
		agent->set_height(height);
		agent->set_position(position);
		agent->set_avoidance_layers(avoidance_layers);
		agent->set_avoidance_enabled(avoidance_enabled);
		agent->set_use_3d_avoidance(use_3d_avoidance);
	}
}

void NavObstacle3D::set_paused(bool p_paused) {
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;

	if (map) {
		if (paused) {
			map->remove_obstacle(this);
		} else {
			map->add_obstacle(this);
		}
	}
	internal_update_agent();
}

bool NavObstacle3D::get_paused() const {
	return paused;
}

void NavObstacle3D::request_sync() {
	if (map && !sync_dirty_request_list_element.in_list()) {
		map->add_obstacle_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

void NavObstacle3D::cancel_sync_request() {
	if (map && sync_dirty_request_list_element.in_list()) {
		map->remove_obstacle_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

NavObstacle3D::NavObstacle3D() :
		sync_dirty_request_list_element(this) {
}

NavObstacle3D::~NavObstacle3D() {
	cancel_sync_request();
}


void NavLink3D::set_map(NavMap3D* p_map)
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

void NavLink3D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_bidirectional(bool p_bidirectional)
{
	if (bidirectional == p_bidirectional) {
		return;
	}
	bidirectional = p_bidirectional;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_start_position(const Vector3 p_position)
{
	if (start_position == p_position) {
		return;
	}
	start_position = p_position;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_end_position(const Vector3 p_position)
{
	if (end_position == p_position) {
		return;
	}
	end_position = p_position;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}
	navigation_layers = p_navigation_layers;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_enter_cost(real_t p_enter_cost)
{
	real_t new_enter_cost = MAX(p_enter_cost, 0.0);
	if (enter_cost == new_enter_cost) {
		return;
	}
	enter_cost = new_enter_cost;
	iteration_dirty = true;

	request_sync();
}

void NavLink3D::set_travel_cost(real_t p_travel_cost)
{
	real_t new_travel_cost = MAX(p_travel_cost, 0.0);
	if (travel_cost == new_travel_cost) {
		return;
	}
	travel_cost = new_travel_cost;
	iteration_dirty = true;

	request_sync();
}

bool NavLink3D::sync()
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

void NavLink3D::_build_iteration()
{
	if (!iteration_dirty || iteration_building || iteration_ready) {
		return;
	}

	iteration_dirty = false;
	iteration_building = true;
	iteration_ready = false;

	Ref<NavIteration3D> new_iteration;
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
	iteration = Ref<NavIteration3D>();
	DEV_ASSERT(iteration.is_null());
	iteration = new_iteration;
	iteration_id = iteration_id % UINT32_MAX + 1;

	iteration_building = false;
	iteration_ready = true;
}

void NavLink3D::request_sync()
{
	if (map && !sync_dirty_request_list_element.in_list()) {
		map->add_link_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

void NavLink3D::cancel_sync_request()
{
	if (map && sync_dirty_request_list_element.in_list()) {
		map->remove_link_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

Ref<NavIteration3D> NavLink3D::get_iteration()
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration;
}

NavLink3D::NavLink3D() : sync_dirty_request_list_element(this)
{
	iteration.instantiate();
}

NavLink3D::~NavLink3D()
{
	cancel_sync_request();

	iteration = Ref<NavIteration3D>();
}


void NavRegion3D::set_map(NavMap3D* p_map)
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

void NavRegion3D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;
	iteration_dirty = true;

	request_sync();
}

void NavRegion3D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections != p_enabled) {
		use_edge_connections = p_enabled;
		iteration_dirty = true;
	}

	request_sync();
}

void NavRegion3D::set_transform(Transform3D p_transform)
{
	if (transform == p_transform) {
		return;
	}
	transform = p_transform;
	iteration_dirty = true;

	request_sync();

#ifdef DEBUG_ENABLED
	if (map && Math::rad_to_deg(map->get_up().angle_to(transform.basis.get_column(1))) >= 90.0f) {
		ERR_PRINT_ONCE("Attempted to update a navigation region transform rotated 90 degrees or "
					   "more away from the current navigation map UP orientation.");
	}
#endif // DEBUG_ENABLED
}

void NavRegion3D::set_navigation_mesh(Ref<NavigationMesh> p_navigation_mesh)
{
#ifdef DEBUG_ENABLED
	if (map && p_navigation_mesh.is_valid() &&
		GLOBAL_GET_CACHED(bool, "navigation/3d/warnings/navmesh_cell_size_mismatch")) {
		const double map_cell_size = double(map->get_cell_size());
		const double map_cell_height = double(map->get_cell_height());
		const double navmesh_cell_size = double(p_navigation_mesh->get_cell_size());
		const double navmesh_cell_height = double(p_navigation_mesh->get_cell_height());

		if (map_cell_size > navmesh_cell_size) {
			WARN_PRINT(vformat(
				"A navigation mesh that uses a `cell_size` of %s was assigned to a navigation map "
				"set to a larger `cell_size` of %s.\nThis mismatch in cell size can cause "
				"rasterization errors with navigation mesh edges on the navigation map.\nThe cell "
				"size for navigation maps can be changed by using the NavigationServer "
				"map_set_cell_size() function.\nThe cell size for default navigation maps can also "
				"be changed in the project settings.\nThis warning can be toggled under "
				"'navigation/3d/warnings/navmesh_cell_size_mismatch' in the project settings.",
				navmesh_cell_size, map_cell_size));
		}
		if (map_cell_height > navmesh_cell_height) {
			WARN_PRINT(vformat(
				"A navigation mesh that uses a `cell_height` of %s was assigned to a navigation "
				"map set to a larger `cell_height` of %s.\nThis mismatch in cell height can cause "
				"rasterization errors with navigation mesh edges on the navigation map.\nThe cell "
				"height for navigation maps can be changed by using the NavigationServer "
				"map_set_cell_height() function.\nThe cell height for default navigation maps can "
				"also be changed in the project settings.\nThis warning can be toggled under "
				"'navigation/3d/warnings/navmesh_cell_size_mismatch' in the project settings.",
				navmesh_cell_height, map_cell_height));
		}
	}
#endif // DEBUG_ENABLED

	navmesh = p_navigation_mesh;

	iteration_dirty = true;

	request_sync();
}

Vector3 NavRegion3D::get_closest_point_to_segment(
	const Vector3& p_from, const Vector3& p_to, bool p_use_collision) const
{
	RWLockRead read_lock(region_rwlock);

	return NavMeshQueries3D::polygons_get_closest_point_to_segment(
		get_polygons(), p_from, p_to, p_use_collision);
}

ClosestPointQueryResult NavRegion3D::get_closest_point_info(const Vector3& p_point) const
{
	RWLockRead read_lock(region_rwlock);

	return NavMeshQueries3D::polygons_get_closest_point_info(get_polygons(), p_point);
}

Vector3 NavRegion3D::get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const
{
	RWLockRead read_lock(region_rwlock);

	if (!get_enabled()) {
		return Vector3();
	}

	return NavMeshQueries3D::polygons_get_random_point(
		get_polygons(), p_navigation_layers, p_uniformly);
}

void NavRegion3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}
	navigation_layers = p_navigation_layers;
	iteration_dirty = true;

	request_sync();
}

void NavRegion3D::set_enter_cost(real_t p_enter_cost)
{
	real_t new_enter_cost = MAX(p_enter_cost, 0.0);
	if (enter_cost == new_enter_cost) {
		return;
	}
	enter_cost = new_enter_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion3D::set_travel_cost(real_t p_travel_cost)
{
	real_t new_travel_cost = MAX(p_travel_cost, 0.0);
	if (travel_cost == new_travel_cost) {
		return;
	}
	travel_cost = new_travel_cost;
	iteration_dirty = true;

	request_sync();
}

void NavRegion3D::scratch_polygons()
{
	iteration_dirty = true;

	request_sync();
}

real_t NavRegion3D::get_surface_area() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_surface_area();
}

AABB NavRegion3D::get_bounds() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_bounds();
}

const LocalVector<Nav3D::Polygon>& NavRegion3D::get_polygons() const
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration->get_navmesh_polygons();
}

bool NavRegion3D::sync()
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

void NavRegion3D::_build_iteration_threaded(void* p_arg)
{
	NavRegionIterationBuild3D* _iteration_build = static_cast<NavRegionIterationBuild3D*>(p_arg);

	NavRegionBuilder3D::build_iteration(*_iteration_build);
}

void NavRegion3D::_sync_iteration()
{
	if (iteration_building || !iteration_ready) {
		return;
	}

	performance_data.pm_polygon_count = iteration_build.performance_data.pm_polygon_count;
	performance_data.pm_edge_count = iteration_build.performance_data.pm_edge_count;
	performance_data.pm_edge_merge_count = iteration_build.performance_data.pm_edge_merge_count;

	RWLockWrite write_lock(iteration_rwlock);
	ERR_FAIL_COND(iteration.is_null());
	iteration = Ref<NavIteration3D>();
	DEV_ASSERT(iteration.is_null());
	iteration = iteration_build.region_iteration;
	iteration_build.region_iteration = Ref<NavIteration3D>();
	DEV_ASSERT(iteration_build.region_iteration.is_null());
	iteration_id = iteration_id % UINT32_MAX + 1;

	iteration_ready = false;

	cancel_async_thread_join();
}

Ref<NavIteration3D> NavRegion3D::get_iteration()
{
	RWLockRead read_lock(iteration_rwlock);
	return iteration;
}

void NavRegion3D::request_async_thread_join()
{
	DEV_ASSERT(map);
	if (map && !async_list_element.in_list()) {
		map->add_region_async_thread_join_request(&async_list_element);
	}
}

void NavRegion3D::cancel_async_thread_join()
{
	if (map && async_list_element.in_list()) {
		map->remove_region_async_thread_join_request(&async_list_element);
	}
}

void NavRegion3D::request_sync()
{
	if (map && !sync_dirty_request_list_element.in_list()) {
		map->add_region_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

void NavRegion3D::cancel_sync_request()
{
	if (map && sync_dirty_request_list_element.in_list()) {
		map->remove_region_sync_dirty_request(&sync_dirty_request_list_element);
	}
}

void NavRegion3D::set_use_async_iterations(bool p_enabled)
{
	if (use_async_iterations == p_enabled) {
		return;
	}
#ifdef THREADS_ENABLED
	use_async_iterations = p_enabled;
#endif
}

bool NavRegion3D::get_use_async_iterations() const { return use_async_iterations; }



NavRegion3D::~NavRegion3D() {}

void NavRegion3D::sync_async_tasks() {}

NavRegion3D::NavRegion3D() {}

void NavRegion3D::_build_iteration() {}


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
	NavMapIteration3D& map_iteration = iteration_slots[iteration_slot_index];                      \
	NavMapIterationRead3D iteration_read_lock(map_iteration);                                      \
	iteration_slot_rwlock.read_unlock();

#define GET_MAP_ITERATION_CONST()                                                                  \
	iteration_slot_rwlock.read_lock();                                                             \
	const NavMapIteration3D& map_iteration = iteration_slots[iteration_slot_index];                \
	NavMapIterationRead3D iteration_read_lock(map_iteration);                                      \
	iteration_slot_rwlock.read_unlock();

void NavMap3D::set_up(Vector3 p_up)
{
	if (up == p_up) {
		return;
	}
	up = p_up;
	map_settings_dirty = true;
}

void NavMap3D::set_cell_size(real_t p_cell_size)
{
	if (cell_size == p_cell_size) {
		return;
	}
	cell_size = MAX(p_cell_size, NavigationDefaults3D::NAV_MESH_CELL_SIZE_MIN);
	_update_merge_rasterizer_cell_dimensions();
	map_settings_dirty = true;
}

void NavMap3D::set_cell_height(real_t p_cell_height)
{
	if (cell_height == p_cell_height) {
		return;
	}
	cell_height = MAX(p_cell_height, NavigationDefaults3D::NAV_MESH_CELL_SIZE_MIN);
	_update_merge_rasterizer_cell_dimensions();
	map_settings_dirty = true;
}

void NavMap3D::set_merge_rasterizer_cell_scale(float p_value)
{
	if (merge_rasterizer_cell_scale == p_value) {
		return;
	}
	merge_rasterizer_cell_scale =
		MAX(MIN(p_value, 0.1), NavigationDefaults3D::NAV_MESH_CELL_SIZE_MIN);
	_update_merge_rasterizer_cell_dimensions();
	map_settings_dirty = true;
}

void NavMap3D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}
	use_edge_connections = p_enabled;
	iteration_dirty = true;
}

void NavMap3D::set_edge_connection_margin(real_t p_edge_connection_margin)
{
	if (edge_connection_margin == p_edge_connection_margin) {
		return;
	}
	edge_connection_margin = p_edge_connection_margin;
	iteration_dirty = true;
}

void NavMap3D::set_link_connection_radius(real_t p_link_connection_radius)
{
	if (link_connection_radius == p_link_connection_radius) {
		return;
	}
	link_connection_radius = p_link_connection_radius;
	iteration_dirty = true;
}

const Vector3& NavMap3D::get_merge_rasterizer_cell_size() const
{
	return merge_rasterizer_cell_size;
}

PointKey NavMap3D::get_point_key(const Vector3& p_pos) const
{
	const int x = static_cast<int>(Math::floor(p_pos.x / merge_rasterizer_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / merge_rasterizer_cell_size.y));
	const int z = static_cast<int>(Math::floor(p_pos.z / merge_rasterizer_cell_size.z));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	p.z = z;
	return p;
}

void NavMap3D::query_path(NavMeshQueries3D::NavMeshPathQueryTask3D& p_query_task)
{
	if (iteration_id == 0) {
		return;
	}

	GET_MAP_ITERATION();

	map_iteration.path_query_slots_semaphore.wait();

	map_iteration.path_query_slots_mutex.lock();
	for (NavMeshQueries3D::PathQuerySlot& p_path_query_slot : map_iteration.path_query_slots) {
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
			"No unused NavMap3D path query slot found! This should never happen :(.");
	}

	p_query_task.map_up = map_iteration.map_up;

	NavMeshQueries3D::query_task_map_iteration_get_path(p_query_task, map_iteration);

	map_iteration.path_query_slots_mutex.lock();
	uint32_t used_slot_index = p_query_task.path_query_slot->slot_index;
	map_iteration.path_query_slots[used_slot_index].in_use = false;
	p_query_task.path_query_slot = nullptr;
	map_iteration.path_query_slots_mutex.unlock();

	map_iteration.path_query_slots_semaphore.post();
}

Vector3 NavMap3D::get_closest_point_to_segment(
	const Vector3& p_from, const Vector3& p_to, const bool p_use_collision) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return Vector3();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_closest_point_to_segment(
		map_iteration, p_from, p_to, p_use_collision);
}

Vector3 NavMap3D::get_closest_point(const Vector3& p_point) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return Vector3();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_closest_point(map_iteration, p_point);
}

Vector3 NavMap3D::get_closest_point_normal(const Vector3& p_point) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return Vector3();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_closest_point_normal(map_iteration, p_point);
}

RID NavMap3D::get_closest_point_owner(const Vector3& p_point) const
{
	if (iteration_id == 0) {
		NAVMAP_ITERATION_ZERO_ERROR_MSG();
		return RID();
	}

	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_closest_point_owner(map_iteration, p_point);
}

ClosestPointQueryResult NavMap3D::get_closest_point_info(const Vector3& p_point) const
{
	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_closest_point_info(map_iteration, p_point);
}

void NavMap3D::add_region(NavRegion3D* p_region)
{
	DEV_ASSERT(!regions.has(p_region));

	regions.push_back(p_region);
	iteration_dirty = true;
}

void NavMap3D::remove_region(NavRegion3D* p_region)
{
	if (regions.erase_unordered(p_region)) {
		iteration_dirty = true;
	}
}

void NavMap3D::add_link(NavLink3D* p_link)
{
	DEV_ASSERT(!links.has(p_link));

	links.push_back(p_link);
	iteration_dirty = true;
}

void NavMap3D::remove_link(NavLink3D* p_link)
{
	if (links.erase_unordered(p_link)) {
		iteration_dirty = true;
	}
}

bool NavMap3D::has_agent(NavAgent3D* agent) const { return agents.has(agent); }

void NavMap3D::add_agent(NavAgent3D* agent)
{
	if (!has_agent(agent)) {
		agents.push_back(agent);
		agents_dirty = true;
	}
}

void NavMap3D::remove_agent(NavAgent3D* agent)
{
	remove_agent_as_controlled(agent);
	if (agents.erase_unordered(agent)) {
		agents_dirty = true;
	}
}

bool NavMap3D::has_obstacle(NavObstacle3D* obstacle) const { return obstacles.has(obstacle); }

void NavMap3D::add_obstacle(NavObstacle3D* obstacle)
{
	if (obstacle->get_paused()) {
		// No point in adding a paused obstacle, it will add itself when unpaused again.
		return;
	}

	if (!has_obstacle(obstacle)) {
		obstacles.push_back(obstacle);
		obstacles_dirty = true;
	}
}

void NavMap3D::remove_obstacle(NavObstacle3D* obstacle)
{
	if (obstacles.erase_unordered(obstacle)) {
		obstacles_dirty = true;
	}
}

void NavMap3D::set_agent_as_controlled(NavAgent3D* agent)
{
	remove_agent_as_controlled(agent);

	if (agent->get_paused()) {
		// No point in adding a paused agent, it will add itself when unpaused again.
		return;
	}

	if (agent->get_use_3d_avoidance()) {
		int64_t agent_3d_index = active_3d_avoidance_agents.find(agent);
		if (agent_3d_index < 0) {
			active_3d_avoidance_agents.push_back(agent);
			agents_dirty = true;
		}
	}
	else {
		int64_t agent_2d_index = active_2d_avoidance_agents.find(agent);
		if (agent_2d_index < 0) {
			active_2d_avoidance_agents.push_back(agent);
			agents_dirty = true;
		}
	}
}

void NavMap3D::remove_agent_as_controlled(NavAgent3D* agent)
{
	if (active_3d_avoidance_agents.erase_unordered(agent)) {
		agents_dirty = true;
	}
	if (active_2d_avoidance_agents.erase_unordered(agent)) {
		agents_dirty = true;
	}
}

Vector3 NavMap3D::get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const
{
	GET_MAP_ITERATION_CONST();

	return NavMeshQueries3D::map_iteration_get_random_point(
		map_iteration, p_navigation_layers, p_uniformly);
}

void NavMap3D::_build_iteration_threaded(void* p_arg)
{
	NavMapIterationBuild3D* _iteration_build = static_cast<NavMapIterationBuild3D*>(p_arg);

	NavMapBuilder3D::build_navmap_iteration(*_iteration_build);
}

void NavMap3D::_sync_iteration()
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

void NavMap3D::_sync_avoidance()
{
	_sync_dirty_avoidance_update_requests();

	if (obstacles_dirty || agents_dirty) {
		_update_rvo_simulation();
	}

	obstacles_dirty = false;
	agents_dirty = false;
}

void NavMap3D::_update_rvo_obstacles_tree_2d()
{
	int obstacle_vertex_count = 0;
	for (NavObstacle3D* obstacle : obstacles) {
		obstacle_vertex_count += obstacle->get_vertices().size();
	}

	// Cleaning old obstacles.
	for (size_t i = 0; i < rvo_simulation_2d.obstacles_.size(); ++i) {
		delete rvo_simulation_2d.obstacles_[i];
	}
	rvo_simulation_2d.obstacles_.clear();

	// Cannot use LocalVector here as RVO library expects std::vector to build KdTree
	std::vector<RVO2D::Obstacle2D*>& raw_obstacles = rvo_simulation_2d.obstacles_;
	raw_obstacles.reserve(obstacle_vertex_count);

	// The following block is modified copy from RVO2D::AddObstacle()
	// Obstacles are linked and depend on all other obstacles.
	for (NavObstacle3D* obstacle : obstacles) {
		if (!obstacle->is_avoidance_enabled()) {
			continue;
		}
		const Vector3& _obstacle_position = obstacle->get_position();
		const Vector<Vector3>& _obstacle_vertices = obstacle->get_vertices();

		if (_obstacle_vertices.size() < 2) {
			continue;
		}

		std::vector<RVO2D::Vector2> rvo_2d_vertices;
		rvo_2d_vertices.reserve(_obstacle_vertices.size());

		uint32_t _obstacle_avoidance_layers = obstacle->get_avoidance_layers();
		real_t _obstacle_height = obstacle->get_height();

		for (const Vector3& _obstacle_vertex : _obstacle_vertices) {
#ifdef TOOLS_ENABLED
			if (_obstacle_vertex.y != 0) {
				WARN_PRINT_ONCE("Y coordinates of static obstacle vertices are ignored. Please use "
								"obstacle position Y to change elevation of obstacle.");
			}
#endif
			rvo_2d_vertices.push_back(RVO2D::Vector2(_obstacle_vertex.x + _obstacle_position.x,
				_obstacle_vertex.z + _obstacle_position.z));
		}

		const size_t obstacleNo = raw_obstacles.size();

		for (size_t i = 0; i < rvo_2d_vertices.size(); i++) {
			RVO2D::Obstacle2D* rvo_2d_obstacle = new RVO2D::Obstacle2D();
			rvo_2d_obstacle->point_ = rvo_2d_vertices[i];
			rvo_2d_obstacle->height_ = _obstacle_height;
			rvo_2d_obstacle->elevation_ = _obstacle_position.y;

			rvo_2d_obstacle->avoidance_layers_ = _obstacle_avoidance_layers;

			if (i != 0) {
				rvo_2d_obstacle->prevObstacle_ = raw_obstacles.back();
				rvo_2d_obstacle->prevObstacle_->nextObstacle_ = rvo_2d_obstacle;
			}

			if (i == rvo_2d_vertices.size() - 1) {
				rvo_2d_obstacle->nextObstacle_ = raw_obstacles[obstacleNo];
				rvo_2d_obstacle->nextObstacle_->prevObstacle_ = rvo_2d_obstacle;
			}

			rvo_2d_obstacle->unitDir_ =
				normalize(rvo_2d_vertices[(i == rvo_2d_vertices.size() - 1 ? 0 : i + 1)] -
						  rvo_2d_vertices[i]);

			if (rvo_2d_vertices.size() == 2) {
				rvo_2d_obstacle->isConvex_ = true;
			}
			else {
				rvo_2d_obstacle->isConvex_ =
					(leftOf(rvo_2d_vertices[(i == 0 ? rvo_2d_vertices.size() - 1 : i - 1)],
						 rvo_2d_vertices[i],
						 rvo_2d_vertices[(i == rvo_2d_vertices.size() - 1 ? 0 : i + 1)]) >= 0.0f);
			}

			rvo_2d_obstacle->id_ = raw_obstacles.size();

			raw_obstacles.push_back(rvo_2d_obstacle);
		}
	}

	rvo_simulation_2d.kdTree_->buildObstacleTree(raw_obstacles);
}

void NavMap3D::_update_rvo_agents_tree_2d()
{
	// Cannot use LocalVector here as RVO library expects std::vector to build KdTree.
	std::vector<RVO2D::Agent2D*> raw_agents;
	raw_agents.reserve(active_2d_avoidance_agents.size());
	for (NavAgent3D* agent : active_2d_avoidance_agents) {
		raw_agents.push_back(agent->get_rvo_agent_2d());
	}
	rvo_simulation_2d.kdTree_->buildAgentTree(raw_agents);
}

void NavMap3D::_update_rvo_agents_tree_3d()
{
	// Cannot use LocalVector here as RVO library expects std::vector to build KdTree.
	std::vector<RVO3D::Agent3D*> raw_agents;
	raw_agents.reserve(active_3d_avoidance_agents.size());
	for (NavAgent3D* agent : active_3d_avoidance_agents) {
		raw_agents.push_back(agent->get_rvo_agent_3d());
	}
	rvo_simulation_3d.kdTree_->buildAgentTree(raw_agents);
}

void NavMap3D::_update_rvo_simulation()
{
	if (obstacles_dirty) {
		_update_rvo_obstacles_tree_2d();
	}
	if (agents_dirty) {
		_update_rvo_agents_tree_2d();
		_update_rvo_agents_tree_3d();
	}
}

void NavMap3D::compute_single_avoidance_step_2d(uint32_t index, NavAgent3D** agent)
{
	(*(agent + index))->get_rvo_agent_2d()->computeNeighbors(&rvo_simulation_2d);
	(*(agent + index))->get_rvo_agent_2d()->computeNewVelocity(&rvo_simulation_2d);
	(*(agent + index))->get_rvo_agent_2d()->update(&rvo_simulation_2d);
	(*(agent + index))->update();
}

void NavMap3D::compute_single_avoidance_step_3d(uint32_t index, NavAgent3D** agent)
{
	(*(agent + index))->get_rvo_agent_3d()->computeNeighbors(&rvo_simulation_3d);
	(*(agent + index))->get_rvo_agent_3d()->computeNewVelocity(&rvo_simulation_3d);
	(*(agent + index))->get_rvo_agent_3d()->update(&rvo_simulation_3d);
	(*(agent + index))->update();
}

void NavMap3D::dispatch_callbacks()
{
	for (NavAgent3D* agent : active_2d_avoidance_agents) {
		agent->dispatch_avoidance_callback();
	}

	for (NavAgent3D* agent : active_3d_avoidance_agents) {
		agent->dispatch_avoidance_callback();
	}
}

void NavMap3D::_update_merge_rasterizer_cell_dimensions()
{
	merge_rasterizer_cell_size.x = cell_size * merge_rasterizer_cell_scale;
	merge_rasterizer_cell_size.y = cell_height * merge_rasterizer_cell_scale;
	merge_rasterizer_cell_size.z = cell_size * merge_rasterizer_cell_scale;
}

int NavMap3D::get_region_connections_count(NavRegion3D* p_region) const
{
	ERR_FAIL_NULL_V(p_region, 0);

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion3D*, Ref<NavIteration3D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration3D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			return found_connections->value.size();
		}
	}

	return 0;
}

Vector3 NavMap3D::get_region_connection_pathway_start(
	NavRegion3D* p_region, int p_connection_id) const
{
	ERR_FAIL_NULL_V(p_region, Vector3());

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion3D*, Ref<NavIteration3D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration3D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			ERR_FAIL_INDEX_V(p_connection_id, int(found_connections->value.size()), Vector3());
			return found_connections->value[p_connection_id].pathway_start;
		}
	}

	return Vector3();
}

Vector3 NavMap3D::get_region_connection_pathway_end(
	NavRegion3D* p_region, int p_connection_id) const
{
	ERR_FAIL_NULL_V(p_region, Vector3());

	GET_MAP_ITERATION_CONST();

	HashMap<NavRegion3D*, Ref<NavIteration3D>>::ConstIterator found_id =
		map_iteration.region_ptr_to_region_iteration.find(p_region);
	if (found_id) {
		HashMap<const NavIteration3D*, LocalVector<Connection>>::ConstIterator
			found_connections =
				map_iteration.external_region_connections.find(found_id->value.ptr());
		if (found_connections) {
			ERR_FAIL_INDEX_V(p_connection_id, int(found_connections->value.size()), Vector3());
			return found_connections->value[p_connection_id].pathway_end;
		}
	}

	return Vector3();
}

void NavMap3D::add_region_sync_dirty_request(SelfList<NavRegion3D>* p_sync_request)
{
	if (p_sync_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(sync_dirty_requests.regions.rwlock);
	sync_dirty_requests.regions.list.add(p_sync_request);
}

void NavMap3D::add_link_sync_dirty_request(SelfList<NavLink3D>* p_sync_request)
{
	if (p_sync_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(sync_dirty_requests.links.rwlock);
	sync_dirty_requests.links.list.add(p_sync_request);
}

void NavMap3D::add_agent_sync_dirty_request(SelfList<NavAgent3D>* p_sync_request)
{
	if (p_sync_request->in_list()) {
		return;
	}
	sync_dirty_requests.agents.list.add(p_sync_request);
}

void NavMap3D::add_obstacle_sync_dirty_request(SelfList<NavObstacle3D>* p_sync_request)
{
	if (p_sync_request->in_list()) {
		return;
	}
	sync_dirty_requests.obstacles.list.add(p_sync_request);
}

void NavMap3D::remove_region_sync_dirty_request(SelfList<NavRegion3D>* p_sync_request)
{
	if
 (!p_sync_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(sync_dirty_requests.regions.rwlock);
	sync_dirty_requests.regions.list.remove(p_sync_request);
}

void NavMap3D::remove_link_sync_dirty_request(SelfList<NavLink3D>* p_sync_request)
{
	if (!p_sync_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(sync_dirty_requests.links.rwlock);
	sync_dirty_requests.links.list.remove(p_sync_request);
}

void NavMap3D::remove_agent_sync_dirty_request(SelfList<NavAgent3D>* p_sync_request)
{
	if (!p_sync_request->in_list()) {
		return;
	}
	sync_dirty_requests.agents.list.remove(p_sync_request);
}

void NavMap3D::remove_obstacle_sync_dirty_request(SelfList<NavObstacle3D>* p_sync_request)
{
	if (!p_sync_request->in_list()) {
		return;
	}
	sync_dirty_requests.obstacles.list.remove(p_sync_request);
}

void NavMap3D::_sync_dirty_map_update_requests()
{
	// If entire map settings changed make all regions dirty.
	if (map_settings_dirty) {
		for (NavRegion3D* region : regions) {
			region->scratch_polygons();
		}
		iteration_dirty = true;
	}

	// Sync NavRegions.
	RWLockWrite write_lock_regions(sync_dirty_requests.regions.rwlock);
	for (SelfList<NavRegion3D>* element = sync_dirty_requests.regions.list.first(); element;
		 element = element->next()) {
		bool requires_map_update = element->self()->sync();
		if (requires_map_update) {
			iteration_dirty = true;
		}
	}
	sync_dirty_requests.regions.list.clear();

	// Sync NavLinks.
	RWLockWrite write_lock_links(sync_dirty_requests.links.rwlock);
	for (SelfList<NavLink3D>* element = sync_dirty_requests.links.list.first(); element;
		 element = element->next()) {
		bool requires_map_update = element->self()->sync();
		if (requires_map_update) {
			iteration_dirty = true;
		}
	}
	sync_dirty_requests.links.list.clear();
}

void NavMap3D::_sync_dirty_avoidance_update_requests()
{
	// Sync NavAgents.
	if (!agents_dirty) {
		agents_dirty = sync_dirty_requests.agents.list.first();
	}
	for (SelfList<NavAgent3D>* element = sync_dirty_requests.agents.list.first(); element;
		 element = element->next()) {
		element->self()->sync();
	}
	sync_dirty_requests.agents.list.clear();

	// Sync NavObstacles.
	if (!obstacles_dirty) {
		obstacles_dirty = sync_dirty_requests.obstacles.list.first();
	}
	for (SelfList<NavObstacle3D>* element = sync_dirty_requests.obstacles.list.first(); element;
		 element = element->next()) {
		element->self()->sync();
	}
	sync_dirty_requests.obstacles.list.clear();
}

void NavMap3D::add_region_async_thread_join_request(SelfList<NavRegion3D>* p_async_request)
{
	if (p_async_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(async_dirty_requests.regions.rwlock);
	async_dirty_requests.regions.list.add(p_async_request);
}

void NavMap3D::remove_region_async_thread_join_request(SelfList<NavRegion3D>* p_async_request)
{
	if (!p_async_request->in_list()) {
		return;
	}
	RWLockWrite write_lock(async_dirty_requests.regions.rwlock);
	async_dirty_requests.regions.list.remove(p_async_request);
}

void NavMap3D::_sync_async_tasks()
{
	// Sync NavRegions that run async thread tasks.
	RWLockWrite write_lock_regions(async_dirty_requests.regions.rwlock);
	for (SelfList<NavRegion3D>* element = async_dirty_requests.regions.list.first(); element;
		 element = element->next()) {
		element->self()->sync_async_tasks();
	}
}

void NavMap3D::set_use_async_iterations(bool p_enabled)
{
	if (use_async_iterations == p_enabled) {
		return;
	}
#ifdef THREADS_ENABLED
	use_async_iterations = p_enabled;
#endif
}

bool NavMap3D::get_use_async_iterations() const { return use_async_iterations; }



NavMap3D::~NavMap3D() {}

void NavMap3D::sync() {}

void NavMap3D::step(double) {}

NavMap3D::NavMap3D() {}

#undef NAVMAP_ITERATION_ZERO_ERROR_MSG
#undef GET_MAP_ITERATION
#undef GET_MAP_ITERATION_CONST


PointKey NavMapBuilder3D::get_point_key(const Vector3 &p_pos, const Vector3 &p_cell_size) {
	const int x = static_cast<int>(Math::floor(p_pos.x / p_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / p_cell_size.y));
	const int z = static_cast<int>(Math::floor(p_pos.z / p_cell_size.z));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	p.z = z;
	return p;
}

void NavMapBuilder3D::build_navmap_iteration(NavMapIterationBuild3D &r_build) {
	PerformanceData &performance_data = r_build.performance_data;

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

void NavMapBuilder3D::_build_step_gather_region_polygons(NavMapIterationBuild3D &r_build) {
	PerformanceData &performance_data = r_build.performance_data;
	NavMapIteration3D *map_iteration = r_build.map_iteration;

	const LocalVector<Ref<NavIteration3D>> &regions = map_iteration->region_iterations;
	HashMap<const NavIteration3D *, LocalVector<Connection>> &region_external_connections = map_iteration->external_region_connections;

	map_iteration->navbases_polygons_external_connections.clear();

	// Remove regions connections.
	region_external_connections.clear();

	// Copy all region polygons in the map.
	int polygon_count = 0;
	for (const Ref<NavIteration3D> &region : regions) {
		const uint32_t polygons_size = region->navmesh_polygons.size();
		polygon_count += polygons_size;

		region_external_connections[region.ptr()] = LocalVector<Connection>();
		map_iteration->navbases_polygons_external_connections[region.ptr()] = LocalVector<LocalVector<Connection>>();
		map_iteration->navbases_polygons_external_connections[region.ptr()].resize(polygons_size);
	}

	performance_data.pm_polygon_count = polygon_count;
	r_build.polygon_count = polygon_count;
}

void NavMapBuilder3D::_build_step_find_edge_connection_pairs(NavMapIterationBuild3D &r_build) {
	PerformanceData &performance_data = r_build.performance_data;
	NavMapIteration3D *map_iteration = r_build.map_iteration;
	int polygon_count = r_build.polygon_count;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey> &connection_pairs_map = r_build.iter_connection_pairs_map;

	// Group all edges per key.
	connection_pairs_map.clear();
	connection_pairs_map.reserve(polygon_count);
	int free_edges_count = 0; // How many ConnectionPairs have only one Connection.
	int edge_merge_error_count = 0;

	for (const Ref<NavIteration3D> &region : map_iteration->region_iterations) {
		for (const ConnectableEdge &connectable_edge : region->get_external_edges()) {
			const EdgeKey &ek = connectable_edge.ek;

			HashMap<EdgeKey, EdgeConnectionPair, EdgeKey>::Iterator pair_it = connection_pairs_map.find(ek);
			if (!pair_it) {
				pair_it = connection_pairs_map.insert(ek, EdgeConnectionPair());
				performance_data.pm_edge_count += 1;
				++free_edges_count;
			}
			EdgeConnectionPair &pair = pair_it->value;
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

			} else {
				// The edge is already connected with another edge, skip.
				edge_merge_error_count++;
			}
		}
	}

	if (edge_merge_error_count > 0 && GLOBAL_GET_CACHED(bool, "navigation/3d/warnings/navmesh_edge_merge_errors")) {
		WARN_PRINT("Navigation map synchronization had " + itos(edge_merge_error_count) + " edge error(s).\nMore than 2 edges tried to occupy the same map rasterization space.\nThis causes a logical error in the navigation mesh geometry and is commonly caused by overlap or too densely placed edges.\nConsider baking with a higher 'cell_size', greater geometry margin, and less detailed bake objects to cause fewer edges.\nConsider lowering the 'navigation/3d/merge_rasterizer_cell_scale' in the project settings.\nThis warning can be toggled under 'navigation/3d/warnings/navmesh_edge_merge_errors' in the project settings.");
	}

	r_build.free_edge_count = free_edges_count;
}

void NavMapBuilder3D::_build_step_merge_edge_connection_pairs(NavMapIterationBuild3D &r_build) {
	PerformanceData &performance_data = r_build.performance_data;

	HashMap<EdgeKey, EdgeConnectionPair, EdgeKey> &connection_pairs_map = r_build.iter_connection_pairs_map;
	LocalVector<Connection> &free_edges = r_build.iter_free_edges;
	int free_edges_count = r_build.free_edge_count;
	bool use_edge_connections = r_build.use_edge_connections;

	free_edges.clear();
	free_edges.reserve(free_edges_count);

	NavMapIteration3D *map_iteration = r_build.map_iteration;

	HashMap<const NavIteration3D *, LocalVector<LocalVector<Nav3D::Connection>>> &navbases_polygons_external_connections = map_iteration->navbases_polygons_external_connections;

	for (const KeyValue<EdgeKey, EdgeConnectionPair> &pair_it : connection_pairs_map) {
		const EdgeConnectionPair &pair = pair_it.value;
		if (pair.size == 2) {
			// Connect edge that are shared in different polygons.
			const Connection &c1 = pair.connections[0];
			const Connection &c2 = pair.connections[1];

			navbases_polygons_external_connections[c1.polygon->owner][c1.polygon->id].push_back(c2);
			navbases_polygons_external_connections[c2.polygon->owner][c2.polygon->id].push_back(c1);
			performance_data.pm_edge_connection_count += 1;

		} else {
			CRASH_COND_MSG(pair.size != 1, vformat("Number of connection != 1. Found: %d", pair.size));
			if (use_edge_connections && pair.connections[0].polygon->owner->get_use_edge_connections()) {
				free_edges.push_back(pair.connections[0]);
			}
		}
	}
}

void NavMapBuilder3D::_build_step_edge_connection_margin_connections(NavMapIterationBuild3D &r_build) {
	PerformanceData &performance_data = r_build.performance_data;
	NavMapIteration3D *map_iteration = r_build.map_iteration;

	real_t edge_connection_margin = r_build.edge_connection_margin;

	LocalVector<Connection> &free_edges = r_build.iter_free_edges;
	HashMap<const NavIteration3D *, LocalVector<Connection>> &region_external_connections = map_iteration->external_region_connections;

	HashMap<const NavIteration3D *, LocalVector<LocalVector<Nav3D::Connection>>> &navbases_polygons_external_connections = map_iteration->navbases_polygons_external_connections;

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
		const Connection &free_edge = free_edges[i];
		const Vector3 &edge_p1 = free_edge.pathway_start;
		const Vector3 &edge_p2 = free_edge.pathway_end;

		for (uint32_t j = 0; j < free_edges.size(); j++) {
			const Connection &other_edge = free_edges[j];
			if (i == j || free_edge.polygon->owner == other_edge.polygon->owner) {
				continue;
			}

			const Vector3 &other_edge_p1 = other_edge.pathway_start;
			const Vector3 &other_edge_p2 = other_edge.pathway_end;

			// Compute the projection of the opposite edge on the current one
			Vector3 edge_vector = edge_p2 - edge_p1;
			real_t projected_p1_ratio = edge_vector.dot(other_edge_p1 - edge_p1) / (edge_vector.length_squared());
			real_t projected_p2_ratio = edge_vector.dot(other_edge_p2 - edge_p1) / (edge_vector.length_squared());
			if ((projected_p1_ratio < 0.0 && projected_p2_ratio < 0.0) || (projected_p1_ratio > 1.0 && projected_p2_ratio > 1.0)) {
				continue;
			}

			// Check if the two edges are close to each other enough and compute a pathway between the two regions.
			Vector3 self1 = edge_vector * CLAMP(projected_p1_ratio, 0.0, 1.0) + edge_p1;
			Vector3 other1;
			if (projected_p1_ratio >= 0.0 && projected_p1_ratio <= 1.0) {
				other1 = other_edge_p1;
			} else {
				other1 = other_edge_p1.lerp(other_edge_p2, (1.0 - projected_p1_ratio) / (projected_p2_ratio - projected_p1_ratio));
			}
			if (other1.distance_squared_to(self1) > edge_connection_margin_squared) {
				continue;
			}

			Vector3 self2 = edge_vector * CLAMP(projected_p2_ratio, 0.0, 1.0) + edge_p1;
			Vector3 other2;
			if (projected_p2_ratio >= 0.0 && projected_p2_ratio <= 1.0) {
				other2 = other_edge_p2;
			} else {
				other2 = other_edge_p1.lerp(other_edge_p2, (0.0 - projected_p1_ratio) / (projected_p2_ratio - projected_p1_ratio));
			}
			if (other2.distance_squared_to(self2) > edge_connection_margin_squared) {
				continue;
			}

			// The edges can now be connected.
			Connection new_connection = other_edge;
			new_connection.pathway_start = (self1 + other1) / 2.0;
			new_connection.pathway_end = (self2 + other2) / 2.0;
			//free_edge.polygon->connections.push_back(new_connection);

			// Add the connection to the region_connection map.
			region_external_connections[free_edge.polygon->owner].push_back(new_connection);
			navbases_polygons_external_connections[free_edge.polygon->owner][free_edge.polygon->id].push_back(new_connection);
			performance_data.pm_edge_connection_count += 1;
		}
	}
}

void NavMapBuilder3D::_build_step_navlink_connections(NavMapIterationBuild3D &r_build) {
	NavMapIteration3D *map_iteration = r_build.map_iteration;

	real_t link_connection_radius = r_build.link_connection_radius;

	const LocalVector<Ref<NavIteration3D>> &links = map_iteration->link_iterations;

	int polygon_count = r_build.polygon_count;

	real_t link_connection_radius_sqr = link_connection_radius * link_connection_radius;

	HashMap<const NavIteration3D *, LocalVector<LocalVector<Nav3D::Connection>>> &navbases_polygons_external_connections = map_iteration->navbases_polygons_external_connections;
	LocalVector<Nav3D::Polygon> &navlink_polygons = map_iteration->navlink_polygons;
	navlink_polygons.clear();
	navlink_polygons.resize(links.size());
	uint32_t navlink_index = 0;

	// Search for polygons within range of a nav link.
	for (const Ref<NavIteration3D> &link : links) {
		polygon_count++;
		Polygon &new_polygon = navlink_polygons[navlink_index++];

		new_polygon.id = 0;
		new_polygon.owner = link.ptr();

		const Vector3 link_start_pos = link->get_start_position();
		const Vector3 link_end_pos = link->get_end_position();

		Polygon *closest_start_polygon = nullptr;
		real_t closest_start_sqr_dist = link_connection_radius_sqr;
		Vector3 closest_start_point;

		Polygon *closest_end_polygon = nullptr;
		real_t closest_end_sqr_dist = link_connection_radius_sqr;
		Vector3 closest_end_point;

		for (const Ref<NavIteration3D> &region : map_iteration->region_iterations) {
			AABB region_bounds = region->get_bounds().grow(link_connection_radius);
			if (!region_bounds.has_point(link_start_pos) && !region_bounds.has_point(link_end_pos)) {
				continue;
			}

			for (Polygon &polyon : region->navmesh_polygons) {
				for (uint32_t point_id = 2; point_id < polyon.vertices.size(); point_id += 1) {
					const Face3 face(polyon.vertices[0], polyon.vertices[point_id - 1], polyon.vertices[point_id]);

					{
						const Vector3 start_point = face.get_closest_point_to(link_start_pos);
						const real_t sqr_dist = start_point.distance_squared_to(link_start_pos);

						// Pick the polygon that is within our radius and is closer than anything we've seen yet.
						if (sqr_dist < closest_start_sqr_dist) {
							closest_start_sqr_dist = sqr_dist;
							closest_start_point = start_point;
							closest_start_polygon = &polyon;
						}
					}

					{
						const Vector3 end_point = face.get_closest_point_to(link_end_pos);
						const real_t sqr_dist = end_point.distance_squared_to(link_end_pos);

						// Pick the polygon that is within our radius and is closer than anything we've seen yet.
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

			// Build a set of vertices that create a thin polygon going from the start to the end point.
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
				navbases_polygons_external_connections[closest_start_polygon->owner][closest_start_polygon->id].push_back(entry_connection);

				Connection exit_connection;
				exit_connection.polygon = closest_end_polygon;
				exit_connection.edge = -1;
				exit_connection.pathway_start = new_polygon.vertices[2];
				exit_connection.pathway_end = new_polygon.vertices[3];
				navbases_polygons_external_connections[link.ptr()].push_back(LocalVector<Nav3D::Connection>());
				navbases_polygons_external_connections[link.ptr()][new_polygon.id].push_back(exit_connection);
			}

			// If the link is bi-directional, create connections from the end to the start.
			if (link->is_bidirectional()) {
				Connection entry_connection;
				entry_connection.polygon = &new_polygon;
				entry_connection.edge = -1;
				entry_connection.pathway_start = new_polygon.vertices[2];
				entry_connection.pathway_end = new_polygon.vertices[3];
				navbases_polygons_external_connections[closest_end_polygon->owner][closest_end_polygon->id].push_back(entry_connection);

				Connection exit_connection;
				exit_connection.polygon = closest_start_polygon;
				exit_connection.edge = -1;
				exit_connection.pathway_start = new_polygon.vertices[0];
				exit_connection.pathway_end = new_polygon.vertices[1];
				navbases_polygons_external_connections[link.ptr()].push_back(LocalVector<Nav3D::Connection>());
				navbases_polygons_external_connections[link.ptr()][new_polygon.id].push_back(exit_connection);
			}
		}
	}

	r_build.polygon_count = polygon_count;
}

void NavMapBuilder3D::_build_update_map_iteration(NavMapIterationBuild3D &r_build) {
	NavMapIteration3D *map_iteration = r_build.map_iteration;

	map_iteration->navmesh_polygon_count = r_build.polygon_count;

	uint32_t navmesh_polygon_count = r_build.polygon_count;
	uint32_t total_polygon_count = navmesh_polygon_count;

	map_iteration->path_query_slots_mutex.lock();
	for (NavMeshQueries3D::PathQuerySlot &p_path_query_slot : map_iteration->path_query_slots) {
		p_path_query_slot.traversable_polys.clear();
		p_path_query_slot.traversable_polys.reserve(navmesh_polygon_count * 0.25);
		p_path_query_slot.path_corridor.clear();

		p_path_query_slot.path_corridor.resize(total_polygon_count);

		p_path_query_slot.poly_to_id.clear();
		p_path_query_slot.poly_to_id.reserve(total_polygon_count);

		int polygon_id = 0;
		for (Ref<NavIteration3D> &region : map_iteration->region_iterations) {
			for (const Polygon &polygon : region->navmesh_polygons) {
				p_path_query_slot.poly_to_id[&polygon] = polygon_id;
				polygon_id++;
			}
		}

		for (const Polygon &polygon : map_iteration->navlink_polygons) {
			p_path_query_slot.poly_to_id[&polygon] = polygon_id;
			polygon_id++;
		}

		DEV_ASSERT(p_path_query_slot.path_corridor.size() == p_path_query_slot.poly_to_id.size());
	}

	map_iteration->path_query_slots_mutex.unlock();
}


void NavRegionBuilder3D::build_iteration(NavRegionIterationBuild3D& r_build)
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

void NavRegionBuilder3D::_build_step_process_navmesh_data(NavRegionIterationBuild3D& r_build)
{
	Vector<Vector3> _navmesh_vertices = r_build.navmesh_data.vertices;
	Vector<Vector<int>> _navmesh_polygons = r_build.navmesh_data.polygons;

	if (_navmesh_vertices.is_empty() || _navmesh_polygons.is_empty()) {
		return;
	}

	PerformanceData& performance_data = r_build.performance_data;
	Ref<NavIteration3D> region_iteration = r_build.region_iteration;

	const Transform3D& region_transform = region_iteration->transform;
	LocalVector<Nav3D::Polygon>& navmesh_polygons = region_iteration->navmesh_polygons;

	const int vertex_count = _navmesh_vertices.size();

	const Vector3* vertices_ptr = _navmesh_vertices.ptr();
	const Vector<int>* polygons_ptr = _navmesh_polygons.ptr();

	navmesh_polygons.resize(_navmesh_polygons.size());

	real_t _new_region_surface_area = 0.0;
	AABB _new_region_bounds;

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
				const Face3 face = Face3(region_transform.xform(vertices_ptr[indices_ptr[0]]),
					region_transform.xform(vertices_ptr[indices_ptr[j - 1]]),
					region_transform.xform(vertices_ptr[indices_ptr[j]]));

				_new_polygon_surface_area += face.get_area();
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

			const Vector3 point_position = region_transform.xform(vertices_ptr[vertex_index]);
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

Nav3D::PointKey NavRegionBuilder3D::get_point_key(const Vector3& p_pos, const Vector3& p_cell_size)
{
	const int x = static_cast<int>(Math::floor(p_pos.x / p_cell_size.x));
	const int y = static_cast<int>(Math::floor(p_pos.y / p_cell_size.y));
	const int z = static_cast<int>(Math::floor(p_pos.z / p_cell_size.z));

	PointKey p;
	p.key = 0;
	p.x = x;
	p.y = y;
	p.z = z;
	return p;
}

Nav3D::EdgeKey NavRegionBuilder3D::get_edge_key(
	const Vector3& p_vertex1, const Vector3& p_vertex2, const Vector3& p_cell_size)
{
	EdgeKey ek(get_point_key(p_vertex1, p_cell_size), get_point_key(p_vertex2, p_cell_size));
	return ek;
}

void NavRegionBuilder3D::_build_step_find_edge_connection_pairs(NavRegionIterationBuild3D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	const Vector3& map_cell_size = r_build.map_cell_size;
	Ref<NavIteration3D> region_iteration = r_build.region_iteration;
	LocalVector<Nav3D::Polygon>& navmesh_polygons = region_iteration->navmesh_polygons;

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
		GLOBAL_GET_CACHED(bool, "navigation/3d/warnings/navmesh_edge_merge_errors")) {
		WARN_PRINT(
			"Navigation region synchronization had " + itos(edge_merge_error_count) +
			" edge error(s).\nMore than 2 edges tried to occupy the same map rasterization "
			"space.\nThis causes a logical error in the navigation mesh geometry and is commonly "
			"caused by overlap or too densely placed edges.\nConsider baking with a higher "
			"'cell_size', greater geometry margin, and less detailed bake objects to cause fewer "
			"edges.\nConsider lowering the 'navigation/3d/merge_rasterizer_cell_scale' in the "
			"project settings.\nThis warning can be toggled under "
			"'navigation/3d/warnings/navmesh_edge_merge_errors' in the project settings.");
	}

	performance_data.pm_edge_free_count = free_edges_count;
}

void NavRegionBuilder3D::_build_step_merge_edge_connection_pairs(NavRegionIterationBuild3D& r_build)
{
	PerformanceData& performance_data = r_build.performance_data;

	Ref<NavIteration3D> region_iteration = r_build.region_iteration;

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

void NavRegionBuilder3D::_build_update_iteration(NavRegionIterationBuild3D& r_build)
{
	ERR_FAIL_NULL(r_build.region);
	// Stub. End of the build.
}


#define THREE_POINTS_CROSS_PRODUCT(m_a, m_b, m_c) (((m_c) - (m_a)).cross((m_b) - (m_a)))

Vector3 NavMeshQueries3D::polygons_get_random_point(
	const LocalVector<Polygon>& p_polygons, uint32_t p_navigation_layers, bool p_uniformly)
{
	const LocalVector<Polygon>& region_polygons = p_polygons;

	if (region_polygons.is_empty()) {
		return Vector3();
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
			return Vector3();
		}

		real_t region_area_map_pos = Math::random(real_t(0), accumulated_area);

		RBMap<real_t, uint32_t>::Iterator region_E =
			region_area_map.find_closest(region_area_map_pos);
		ERR_FAIL_COND_V(!region_E, Vector3());
		uint32_t rrp_polygon_index = region_E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(rrp_polygon_index, region_polygons.size(), Vector3());

		const Polygon& rr_polygon = region_polygons[rrp_polygon_index];

		real_t accumulated_polygon_area = 0;
		RBMap<real_t, uint32_t> polygon_area_map;

		for (uint32_t rpp_index = 2; rpp_index < rr_polygon.vertices.size(); rpp_index++) {
			real_t face_area = Face3(rr_polygon.vertices[0], rr_polygon.vertices[rpp_index - 1],
				rr_polygon.vertices[rpp_index])
								   .get_area();

			if (face_area == 0.0) {
				continue;
			}
			polygon_area_map[accumulated_polygon_area] = rpp_index;
			accumulated_polygon_area += face_area;
		}
		if (polygon_area_map.is_empty() || accumulated_polygon_area == 0) {
			// All faces have no real surface / no area.
			return Vector3();
		}

		real_t polygon_area_map_pos = Math::random(real_t(0), accumulated_polygon_area);

		RBMap<real_t, uint32_t>::Iterator polygon_E =
			polygon_area_map.find_closest(polygon_area_map_pos);
		ERR_FAIL_COND_V(!polygon_E, Vector3());
		uint32_t rrp_face_index = polygon_E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(rrp_face_index, rr_polygon.vertices.size(), Vector3());

		const Face3 face(rr_polygon.vertices[0], rr_polygon.vertices[rrp_face_index - 1],
			rr_polygon.vertices[rrp_face_index]);

		Vector3 face_random_position = face.get_random_point_inside();
		return face_random_position;

	}
	else {
		uint32_t rrp_polygon_index = Math::random(int(0), region_polygons.size() - 1);

		const Polygon& rr_polygon = region_polygons[rrp_polygon_index];

		uint32_t rrp_face_index = Math::random(int(2), rr_polygon.vertices.size() - 1);

		const Face3 face(rr_polygon.vertices[0], rr_polygon.vertices[rrp_face_index - 1],
			rr_polygon.vertices[rrp_face_index]);

		Vector3 face_random_position = face.get_random_point_inside();
		return face_random_position;
	}
}

void NavMeshQueries3D::_query_task_find_start_end_positions(
	NavMeshPathQueryTask3D& p_query_task, const NavMapIteration3D& p_map_iteration)
{
	real_t begin_d = FLT_MAX;
	real_t end_d = FLT_MAX;

	const LocalVector<Ref<NavIteration3D>>& regions = p_map_iteration.region_iterations;

	for (const Ref<NavIteration3D>& region : regions) {
		if (!_query_task_is_connection_owner_usable(p_query_task, region.ptr())) {
			continue;
		}

		// Find the initial poly and the end poly on this map.
		for (const Polygon& p : region->get_navmesh_polygons()) {
			// Only consider the polygon if it in a region with compatible layers.
			if ((p_query_task.navigation_layers & p.owner->get_navigation_layers()) == 0) {
				continue;
			}

			// For each face check the distance between the origin/destination.
			for (uint32_t point_id = 2; point_id < p.vertices.size(); point_id++) {
				const Face3 face(p.vertices[0], p.vertices[point_id - 1], p.vertices[point_id]);

				Vector3 point = face.get_closest_point_to(p_query_task.start_position);
				real_t distance_to_point = point.distance_to(p_query_task.start_position);
				if (distance_to_point < begin_d) {
					begin_d = distance_to_point;
					p_query_task.begin_polygon = &p;
					p_query_task.begin_position = point;
				}

				point = face.get_closest_point_to(p_query_task.target_position);
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

void NavMeshQueries3D::_query_task_search_polygon_connections(NavMeshPathQueryTask3D& p_query_task,
	const Connection& p_connection, uint32_t p_least_cost_id,
	const NavigationPoly& p_least_cost_poly, real_t p_poly_enter_cost, const Vector3& p_end_point)
{
	const NavIteration3D* connection_owner = p_connection.polygon->owner;
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

	Vector3 new_entry = Geometry3D::get_closest_point_to_segment(
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

void NavMeshQueries3D::_query_task_build_path_corridor(
	NavMeshPathQueryTask3D& p_query_task, const NavMapIteration3D& p_map_iteration)
{
	const Vector3 p_target_position = p_query_task.target_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector3 begin_point = p_query_task.begin_position;
	Vector3 end_point = p_query_task.end_position;

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

	const HashMap<const NavIteration3D*, LocalVector<LocalVector<Nav3D::Connection>>>&
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

		const NavIteration3D* least_cost_navbase = least_cost_poly.poly->owner;

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
				Face3 f(end_poly->vertices[0], end_poly->vertices[point_id - 1],
					end_poly->vertices[point_id]);
				Vector3 spoint = f.get_closest_point_to(p_target_position);
				real_t dpoint = spoint.distance_squared_to(p_target_position);
				if (dpoint < end_d) {
					end_point = spoint;
					end_d = dpoint;
				}
			}

			// Search all faces of start polygon as well.
			bool closest_point_on_start_poly = false;

			for (uint32_t point_id = 2; point_id < begin_poly->vertices.size(); point_id++) {
				Face3 f(begin_poly->vertices[0], begin_poly->vertices[point_id - 1],
					begin_poly->vertices[point_id]);
				Vector3 spoint = f.get_closest_point_to(p_target_position);
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
				p_query_task.status = NavMeshPathQueryTask3D::TaskStatus::QUERY_FINISHED;
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
			Face3 f(begin_poly->vertices[0], begin_poly->vertices[point_id - 1],
				begin_poly->vertices[point_id]);
			Vector3 spoint = f.get_closest_point_to(p_target_position);
			real_t dpoint = spoint.distance_squared_to(p_target_position);
			if (dpoint < end_d) {
				end_point = spoint;
				end_d = dpoint;
			}
		}

		p_query_task.path_clear();

		_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
		_query_task_push_back_point_with_metadata(p_query_task, end_point, begin_poly);
		p_query_task.status = NavMeshPathQueryTask3D::TaskStatus::QUERY_FINISHED;
	}
	else {
		p_query_task.end_position = end_point;
		p_query_task.end_polygon = end_poly;
		p_query_task.begin_position = begin_point;
		p_query_task.begin_polygon = begin_poly;
		p_query_task.least_cost_id = least_cost_id;
	}
}

float NavMeshQueries3D::_calculate_path_length(
	const LocalVector<Vector3>& p_path, uint32_t p_start_index, uint32_t p_end_index)
{
	const uint32_t path_size = p_path.size();
	if (path_size < 2) {
		return 0.0;
	}

	ERR_FAIL_COND_V(p_start_index >= p_end_index, 0.0);
	ERR_FAIL_COND_V(p_start_index >= path_size - 1, 0.0);
	ERR_FAIL_COND_V(p_end_index >= path_size, 0.0);

	const Vector3* path_ptr = p_path.ptr();

	float path_length = 0.0;

	for (uint32_t i = p_start_index; i < p_end_index; i++) {
		const Vector3& vertex1 = path_ptr[i];
		const Vector3& vertex2 = path_ptr[i + 1];
		float edge_length = vertex1.distance_to(vertex2);
		path_length += edge_length;
	}

	return path_length;
}

void NavMeshQueries3D::_query_task_post_process_corridorfunnel(NavMeshPathQueryTask3D& p_query_task)
{
	Vector3 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector3 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;
	Vector3 p_map_up = p_query_task.map_up;

	// Set the apex poly/point to the end point
	NavigationPoly* apex_poly = &navigation_polys[least_cost_id];

	const Vector3 back_edge_closest_point = Geometry3D::get_closest_point_to_segment(end_point,
		apex_poly->back_navigation_edge_pathway_start, apex_poly->back_navigation_edge_pathway_end);
	if (end_point.is_equal_approx(back_edge_closest_point)) {
		// The end point is basically on top of the last crossed edge, funneling around the corners
		// would at best do nothing. At worst it would add an unwanted path point before the last
		// point due to precision issues so skip to the next polygon.
		if (apex_poly->back_navigation_poly_id != -1) {
			apex_poly = &navigation_polys[apex_poly->back_navigation_poly_id];
		}
	}

	Vector3 apex_point = end_point;

	NavigationPoly* left_poly = apex_poly;
	Vector3 left_portal = apex_point;
	NavigationPoly* right_poly = apex_poly;
	Vector3 right_portal = apex_point;

	NavigationPoly* p = apex_poly;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	while (p) {
		// Set left and right points of the pathway between polygons.
		Vector3 left = p->back_navigation_edge_pathway_start;
		Vector3 right = p->back_navigation_edge_pathway_end;
		if (THREE_POINTS_CROSS_PRODUCT(apex_point, left, right).dot(p_map_up) < 0) {
			SWAP(left, right);
		}

		bool skip = false;
		if (THREE_POINTS_CROSS_PRODUCT(apex_point, left_portal, left).dot(p_map_up) >= 0) {
			// process
			if (left_portal == apex_point ||
				THREE_POINTS_CROSS_PRODUCT(apex_point, left, right_portal).dot(p_map_up) > 0) {
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

		if (!skip &&
			THREE_POINTS_CROSS_PRODUCT(apex_point, right_portal, right).dot(p_map_up) <= 0) {
			// process
			if (right_portal == apex_point ||
				THREE_POINTS_CROSS_PRODUCT(apex_point, right, left_portal).dot(p_map_up) < 0) {
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

void NavMeshQueries3D::_query_task_post_process_edgecentered(NavMeshPathQueryTask3D& p_query_task)
{
	Vector3 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector3 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	// Add mid points
	int np_id = least_cost_id;
	while (np_id != -1 && navigation_polys[np_id].back_navigation_poly_id != -1) {
		if (navigation_polys[np_id].back_navigation_edge != -1) {
			int prev = navigation_polys[np_id].back_navigation_edge;
			int prev_n = (navigation_polys[np_id].back_navigation_edge + 1) %
						 navigation_polys[np_id].poly->vertices.size();
			Vector3 point = (navigation_polys[np_id].poly->vertices[prev] +
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

void NavMeshQueries3D::_query_task_post_process_nopostprocessing(
	NavMeshPathQueryTask3D& p_query_task)
{
	Vector3 end_point = p_query_task.end_position;
	const Polygon* end_poly = p_query_task.end_polygon;
	Vector3 begin_point = p_query_task.begin_position;
	const Polygon* begin_poly = p_query_task.begin_polygon;
	uint32_t least_cost_id = p_query_task.least_cost_id;
	LocalVector<NavigationPoly>& navigation_polys = p_query_task.path_query_slot->path_corridor;

	_query_task_push_back_point_with_metadata(p_query_task, end_point, end_poly);

	// Add mid points
	int np_id = least_cost_id;
	while (np_id != -1 && navigation_polys[np_id].back_navigation_poly_id != -1) {
		_query_task_push_back_point_with_metadata(
			p_query_task, navigation_polys[np_id].entry, navigation_polys[np_id].poly);

		np_id = navigation_polys[np_id].back_navigation_poly_id;
	}

	_query_task_push_back_point_with_metadata(p_query_task, begin_point, begin_poly);
}

Vector3 NavMeshQueries3D::map_iteration_get_closest_point_to_segment(
	const NavMapIteration3D& p_map_iteration, const Vector3& p_from, const Vector3& p_to,
	const bool p_use_collision)
{
	bool use_collision = p_use_collision;
	Vector3 closest_point;
	real_t closest_point_distance = FLT_MAX;

	const LocalVector<Ref<NavIteration3D>>& regions = p_map_iteration.region_iterations;
	for (const Ref<NavIteration3D>& region : regions) {
		for (const Polygon& polygon : region->get_navmesh_polygons()) {
			// For each face check the distance to the segment.
			for (uint32_t point_id = 2; point_id < polygon.vertices.size(); point_id += 1) {
				const Face3 face(polygon.vertices[0], polygon.vertices[point_id - 1],
					polygon.vertices[point_id]);
				Vector3 intersection_point;
				if (face.intersects_segment(p_from, p_to, &intersection_point)) {
					const real_t d = p_from.distance_to(intersection_point);
					if (!use_collision) {
						closest_point = intersection_point;
						use_collision = true;
						closest_point_distance = d;
					}
					else if (closest_point_distance > d) {
						closest_point = intersection_point;
						closest_point_distance = d;
					}
				}
				// If segment does not itersect face, check the distance from segment's endpoints.
				else if (!use_collision) {
					const Vector3 p_from_closest = face.get_closest_point_to(p_from);
					const real_t d_p_from = p_from.distance_to(p_from_closest);
					if (closest_point_distance > d_p_from) {
						closest_point = p_from_closest;
						closest_point_distance = d_p_from;
					}

					const Vector3 p_to_closest = face.get_closest_point_to(p_to);
					const real_t d_p_to = p_to.distance_to(p_to_closest);
					if (closest_point_distance > d_p_to) {
						closest_point = p_to_closest;
						closest_point_distance = d_p_to;
					}
				}
			}
			// Finally, check for a case when shortest distance is between some point located on a
			// face's edge and some point located on a line segment.
			if (!use_collision) {
				for (uint32_t point_id = 0; point_id < polygon.vertices.size(); point_id += 1) {
					Vector3 a, b;

					Geometry3D::get_closest_points_between_segments(p_from, p_to,
						polygon.vertices[point_id],
						polygon.vertices[(point_id + 1) % polygon.vertices.size()], a, b);

					const real_t d = a.distance_to(b);
					if (d < closest_point_distance) {
						closest_point_distance = d;
						closest_point = b;
					}
				}
			}
		}
	}

	return closest_point;
}

Vector3 NavMeshQueries3D::map_iteration_get_closest_point(
	const NavMapIteration3D& p_map_iteration, const Vector3& p_point)
{
	ClosestPointQueryResult cp = map_iteration_get_closest_point_info(p_map_iteration, p_point);
	return cp.point;
}

Vector3 NavMeshQueries3D::map_iteration_get_closest_point_normal(
	const NavMapIteration3D& p_map_iteration, const Vector3& p_point)
{
	ClosestPointQueryResult cp = map_iteration_get_closest_point_info(p_map_iteration, p_point);
	return cp.normal;
}

RID NavMeshQueries3D::map_iteration_get_closest_point_owner(
	const NavMapIteration3D& p_map_iteration, const Vector3& p_point)
{
	ClosestPointQueryResult cp = map_iteration_get_closest_point_info(p_map_iteration, p_point);
	return cp.owner;
}

ClosestPointQueryResult NavMeshQueries3D::map_iteration_get_closest_point_info(
	const NavMapIteration3D& p_map_iteration, const Vector3& p_point)
{
	ClosestPointQueryResult result;
	real_t closest_point_distance_squared = FLT_MAX;

	const LocalVector<Ref<NavIteration3D>>& regions = p_map_iteration.region_iterations;
	for (const Ref<NavIteration3D>& region : regions) {
		for (const Polygon& polygon : region->get_navmesh_polygons()) {
			Vector3 plane_normal = (polygon.vertices[1] - polygon.vertices[0])
									   .cross(polygon.vertices[2] - polygon.vertices[0]);
			Vector3 closest_on_polygon;
			real_t closest = FLT_MAX;
			bool inside = true;
			Vector3 previous = polygon.vertices[polygon.vertices.size() - 1];
			for (uint32_t point_id = 0; point_id < polygon.vertices.size(); ++point_id) {
				Vector3 edge = polygon.vertices[point_id] - previous;
				Vector3 to_point = p_point - previous;
				Vector3 edge_to_point_pormal = edge.cross(to_point);
				bool clockwise = edge_to_point_pormal.dot(plane_normal) > 0;
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
					else if (point_projected_on_edge < 0.f) {
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
				Vector3 plane_normalized = plane_normal.normalized();
				real_t distance = plane_normalized.dot(p_point - polygon.vertices[0]);
				real_t distance_squared = distance * distance;
				if (distance_squared < closest_point_distance_squared) {
					closest_point_distance_squared = distance_squared;
					result.point = p_point - plane_normalized * distance;
					result.normal = plane_normalized;
					result.owner = polygon.owner->get_self();

					if (Math::is_zero_approx(distance)) {
						break;
					}
				}
			}
			else {
				real_t distance = closest_on_polygon.distance_squared_to(p_point);
				if (distance < closest_point_distance_squared) {
					closest_point_distance_squared = distance;
					result.point = closest_on_polygon;
					result.normal = plane_normal.normalized();
					result.owner = polygon.owner->get_self();
				}
			}
		}
	}

	return result;
}

Vector3 NavMeshQueries3D::map_iteration_get_random_point(
	const NavMapIteration3D& p_map_iteration, uint32_t p_navigation_layers, bool p_uniformly)
{
	if (p_map_iteration.region_iterations.is_empty()) {
		return Vector3();
	}

	LocalVector<uint32_t> accessible_regions;
	accessible_regions.reserve(p_map_iteration.region_iterations.size());

	for (uint32_t i = 0; i < p_map_iteration.region_iterations.size(); i++) {
		const Ref<NavIteration3D>& region = p_map_iteration.region_iterations[i];
		if (!region->get_enabled() ||
			(p_navigation_layers & region->get_navigation_layers()) == 0) {
			continue;
		}
		accessible_regions.push_back(i);
	}

	if (accessible_regions.is_empty()) {
		// All existing region polygons are disabled.
		return Vector3();
	}

	if (p_uniformly) {
		real_t accumulated_region_surface_area = 0;
		RBMap<real_t, uint32_t> accessible_regions_area_map;

		for (uint32_t accessible_region_index = 0;
			 accessible_region_index < accessible_regions.size(); accessible_region_index++) {
			const Ref<NavIteration3D>& region =
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
			return Vector3();
		}

		real_t random_accessible_regions_area_map =
			Math::random(real_t(0), accumulated_region_surface_area);

		RBMap<real_t, uint32_t>::Iterator E =
			accessible_regions_area_map.find_closest(random_accessible_regions_area_map);
		ERR_FAIL_COND_V(!E, Vector3());
		uint32_t random_region_index = E->value;
		ERR_FAIL_UNSIGNED_INDEX_V(random_region_index, accessible_regions.size(), Vector3());

		const Ref<NavIteration3D>& random_region =
			p_map_iteration.region_iterations[accessible_regions[random_region_index]];

		return NavMeshQueries3D::polygons_get_random_point(
			random_region->navmesh_polygons, p_navigation_layers, p_uniformly);

	}
	else {
		uint32_t random_region_index = Math::random(int(0), accessible_regions.size() - 1);

		const Ref<NavIteration3D>& random_region =
			p_map_iteration.region_iterations[accessible_regions[random_region_index]];

		return NavMeshQueries3D::polygons_get_random_point(
			random_region->navmesh_polygons, p_navigation_layers, p_uniformly);
	}
}

Vector3 NavMeshQueries3D::polygons_get_closest_point_to_segment(
	const LocalVector<Polygon>& p_polygons, const Vector3& p_from, const Vector3& p_to,
	const bool p_use_collision)
{
	bool use_collision = p_use_collision;
	Vector3 closest_point;
	real_t closest_point_distance = FLT_MAX;

	for (const Polygon& polygon : p_polygons) {
		// For each face check the distance to the segment.
		for (uint32_t point_id = 2; point_id < polygon.vertices.size(); point_id += 1) {
			const Face3 face(
				polygon.vertices[0], polygon.vertices[point_id - 1], polygon.vertices[point_id]);
			Vector3 intersection_point;
			if (face.intersects_segment(p_from, p_to, &intersection_point)) {
				const real_t d = p_from.distance_to(intersection_point);
				if (!use_collision) {
					closest_point = intersection_point;
					use_collision = true;
					closest_point_distance = d;
				}
				else if (closest_point_distance > d) {
					closest_point = intersection_point;
					closest_point_distance = d;
				}
			}
			// If segment does not itersect face, check the distance from segment's endpoints.
			else if (!use_collision) {
				const Vector3 p_from_closest = face.get_closest_point_to(p_from);
				const real_t d_p_from = p_from.distance_to(p_from_closest);
				if (closest_point_distance > d_p_from) {
					closest_point = p_from_closest;
					closest_point_distance = d_p_from;
				}

				const Vector3 p_to_closest = face.get_closest_point_to(p_to);
				const real_t d_p_to = p_to.distance_to(p_to_closest);
				if (closest_point_distance > d_p_to) {
					closest_point = p_to_closest;
					closest_point_distance = d_p_to;
				}
			}
		}
		// Finally, check for a case when shortest distance is between some point located on a
		// face's edge and some point located on a line segment.
		if (!use_collision) {
			for (uint32_t point_id = 0; point_id < polygon.vertices.size(); point_id += 1) {
				Vector3 a, b;

				Geometry3D::get_closest_points_between_segments(p_from, p_to,
					polygon.vertices[point_id],
					polygon.vertices[(point_id + 1) % polygon.vertices.size()], a, b);

				const real_t d = a.distance_to(b);
				if (d < closest_point_distance) {
					closest_point_distance = d;
					closest_point = b;
				}
			}
		}
	}

	return closest_point;
}

Vector3 NavMeshQueries3D::polygons_get_closest_point(
	const LocalVector<Polygon>& p_polygons, const Vector3& p_point)
{
	ClosestPointQueryResult cp = polygons_get_closest_point_info(p_polygons, p_point);
	return cp.point;
}

Vector3 NavMeshQueries3D::polygons_get_closest_point_normal(
	const LocalVector<Polygon>& p_polygons, const Vector3& p_point)
{
	ClosestPointQueryResult cp = polygons_get_closest_point_info(p_polygons, p_point);
	return cp.normal;
}

ClosestPointQueryResult NavMeshQueries3D::polygons_get_closest_point_info(
	const LocalVector<Polygon>& p_polygons, const Vector3& p_point)
{
	ClosestPointQueryResult result;
	real_t closest_point_distance_squared = FLT_MAX;

	for (const Polygon& polygon : p_polygons) {
		Vector3 plane_normal = (polygon.vertices[1] - polygon.vertices[0])
								   .cross(polygon.vertices[2] - polygon.vertices[0]);
		Vector3 closest_on_polygon;
		real_t closest = FLT_MAX;
		bool inside = true;
		Vector3 previous = polygon.vertices[polygon.vertices.size() - 1];
		for (uint32_t point_id = 0; point_id < polygon.vertices.size(); ++point_id) {
			Vector3 edge = polygon.vertices[point_id] - previous;
			Vector3 to_point = p_point - previous;
			Vector3 edge_to_point_pormal = edge.cross(to_point);
			bool clockwise = edge_to_point_pormal.dot(plane_normal) > 0;
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
				else if (point_projected_on_edge < 0.f) {
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
			Vector3 plane_normalized = plane_normal.normalized();
			real_t distance = plane_normalized.dot(p_point - polygon.vertices[0]);
			real_t distance_squared = distance * distance;
			if (distance_squared < closest_point_distance_squared) {
				closest_point_distance_squared = distance_squared;
				result.point = p_point - plane_normalized * distance;
				result.normal = plane_normal;
				result.owner = polygon.owner->get_self();

				if (Math::is_zero_approx(distance)) {
					break;
				}
			}
		}
		else {
			real_t distance = closest_on_polygon.distance_squared_to(p_point);
			if (distance < closest_point_distance_squared) {
				closest_point_distance_squared = distance;
				result.point = closest_on_polygon;
				result.normal = plane_normal;
				result.owner = polygon.owner->get_self();
			}
		}
	}

	return result;
}

RID NavMeshQueries3D::polygons_get_closest_point_owner(
	const LocalVector<Polygon>& p_polygons, const Vector3& p_point)
{
	ClosestPointQueryResult cp = polygons_get_closest_point_info(p_polygons, p_point);
	return cp.owner;
}

void NavMeshQueries3D::_query_task_clip_path(NavMeshPathQueryTask3D& p_query_task,
	const NavigationPoly* from_poly, const Vector3& p_to_point, const NavigationPoly* p_to_poly)
{
	Vector3 from = p_query_task.path_points[p_query_task.path_points.size() - 1];
	const LocalVector<NavigationPoly>& p_navigation_polys =
		p_query_task.path_query_slot->path_corridor;
	const Vector3 p_map_up = p_query_task.map_up;

	if (from.is_equal_approx(p_to_point)) {
		return;
	}

	Plane cut_plane;
	cut_plane.normal = (from - p_to_point).cross(p_map_up);
	if (cut_plane.normal == Vector3()) {
		return;
	}
	cut_plane.normal.normalize();
	cut_plane.d = cut_plane.normal.dot(from);

	while (from_poly != p_to_poly) {
		Vector3 pathway_start = from_poly->back_navigation_edge_pathway_start;
		Vector3 pathway_end = from_poly->back_navigation_edge_pathway_end;

		ERR_FAIL_COND(from_poly->back_navigation_poly_id == -1);
		from_poly = &p_navigation_polys[from_poly->back_navigation_poly_id];

		if (!pathway_start.is_equal_approx(pathway_end)) {
			Vector3 inters;
			if (cut_plane.intersects_segment(pathway_start, pathway_end, &inters)) {
				if (!inters.is_equal_approx(p_to_point) &&
					!inters.is_equal_approx(
						p_query_task.path_points[p_query_task.path_points.size() - 1])) {
					_query_task_push_back_point_with_metadata(
						p_query_task, inters, from_poly->poly);
				}
			}
		}
	}
}

bool NavMeshQueries3D::_query_task_is_connection_owner_usable(
	const NavMeshPathQueryTask3D& p_query_task, const NavIteration3D* p_owner)
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
		case NavigationEnums3D::PathSegmentType::PATH_SEGMENT_TYPE_REGION: {
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
		case NavigationEnums3D::PathSegmentType::PATH_SEGMENT_TYPE_LINK: {
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

LocalVector<uint32_t> NavMeshQueries3D::get_simplified_path_indices(
	const LocalVector<Vector3>& p_path, real_t p_epsilon)
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

void NavMeshQueries3D::simplify_path_segment(int p_start_inx, int p_end_inx,
	const LocalVector<Vector3>& p_points, real_t p_epsilon,
	LocalVector<uint32_t>& r_simplified_path_indices)
{
	const Vector3 path_segment_a = p_points[p_start_inx];
	const Vector3 path_segment_b = p_points[p_end_inx];

	real_t point_max_distance = 0.0;
	int point_max_index = 0;

	for (int i = p_start_inx; i < p_end_inx; i++) {
		const Vector3& checked_point = p_points[i];

		const Vector3 closest_point =
			Geometry3D::get_closest_point_to_segment(checked_point, path_segment_a, path_segment_b);
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

void NavMeshQueries3D::_query_task_push_back_point_with_metadata(
	NavMeshPathQueryTask3D& p_query_task, const Vector3& p_point,
	const Nav3D::Polygon* p_point_polygon)
{
}

void NavMeshQueries3D::query_task_map_iteration_get_path(
	NavMeshQueries3D::NavMeshPathQueryTask3D& p_task, const NavMapIteration3D& p_map_iteration)
{
}

#undef THREE_POINTS_CROSS_PRODUCT

Mutex NavMeshGenerator3D::baking_navmesh_mutex;
Mutex NavMeshGenerator3D::generator_task_mutex;
RWLock NavMeshGenerator3D::generator_parsers_rwlock;
bool NavMeshGenerator3D::use_threads = true;
bool NavMeshGenerator3D::baking_use_multiple_threads = true;
bool NavMeshGenerator3D::baking_use_high_priority_threads = true;
HashMap<Ref<NavigationMesh>, NavMeshGenerator3D::NavMeshGeneratorTask3D*>
	NavMeshGenerator3D::baking_navmeshes;

static const char*
	_navmesh_bake_state_msgs[(size_t)NavMeshGenerator3D::NavMeshBakeState::BAKE_STATE_MAX] = {
		"",
		"Setting up configuration...",
		"Calculating grid size...",
		"Creating heightfield...",
		"Marking walkable triangles...",
		"Constructing compact heightfield...",
		"Eroding walkable area...",
		"Sample partitioning...",
		"Creating contours...",
		"Creating polymesh...",
		"Converting to native navigation mesh...",
		"Baking cleanup...",
		"Baking finished.",
};

void NavMeshGenerator3D::initialize()
{
	_initialized = true;
}

void NavMeshGenerator3D::finalize()
{
	if (!_initialized) {
		return;
	}
	cleanup();
	_initialized = false;
}

void NavMeshGenerator3D::finish() { cleanup(); }

bool NavMeshGenerator3D::is_baking(Ref<NavigationMesh> p_navigation_mesh)
{
	MutexLock baking_navmesh_lock(baking_navmesh_mutex);
	return baking_navmeshes.has(p_navigation_mesh);
}

String NavMeshGenerator3D::get_baking_state_msg(Ref<NavigationMesh> p_navigation_mesh)
{
	String bake_state_msg;
	MutexLock baking_navmesh_lock(baking_navmesh_mutex);
	if (baking_navmeshes.has(p_navigation_mesh)) {
		bake_state_msg = _navmesh_bake_state_msgs[baking_navmeshes[p_navigation_mesh]->bake_state];
	}
	else {
		bake_state_msg = _navmesh_bake_state_msgs[NavMeshBakeState::BAKE_STATE_NONE];
	}
	return bake_state_msg;
}

void NavMeshGenerator3D::generator_thread_bake(void* p_arg)
{
	NavMeshGeneratorTask3D* generator_task = static_cast<NavMeshGeneratorTask3D*>(p_arg);

	generator_bake_from_source_geometry_data(generator_task);

	generator_task->status = NavMeshGeneratorTask3D::TaskStatus::BAKING_FINISHED;
}

void NavMeshGenerator3D::generator_bake_from_source_geometry_data(
	NavMeshGeneratorTask3D* p_generator_task)
{
	Ref<NavigationMesh> p_navigation_mesh = p_generator_task->navigation_mesh;
	const Ref<NavigationMeshSourceGeometryData3D>& p_source_geometry_data =
		p_generator_task->source_geometry_data;

	if (p_navigation_mesh.is_null() || p_source_geometry_data.is_null()) {
		return;
	}

	Vector<float> source_geometry_vertices;
	Vector<int> source_geometry_indices;
	Vector<NavigationMeshSourceGeometryData3D::ProjectedObstruction> projected_obstructions;

	p_source_geometry_data->get_data(
		source_geometry_vertices, source_geometry_indices, projected_obstructions);

	if (source_geometry_vertices.size() < 3 || source_geometry_indices.size() < 3) {
		return;
	}

	rcHeightfield* hf = nullptr;
	rcCompactHeightfield* chf = nullptr;
	rcContourSet* cset = nullptr;
	rcPolyMesh* poly_mesh = nullptr;
	rcPolyMeshDetail* detail_mesh = nullptr;
	rcContext ctx;

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_CONFIGURATION;

	const float* verts = source_geometry_vertices.ptr();
	const int nverts = source_geometry_vertices.size() / 3;
	const int* tris = source_geometry_indices.ptr();
	const int ntris = source_geometry_indices.size() / 3;

	float bmin[3], bmax[3];
	rcCalcBounds(verts, nverts, bmin, bmax);

	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));

	cfg.cs = p_navigation_mesh->get_cell_size();
	cfg.ch = p_navigation_mesh->get_cell_height();
	if (p_navigation_mesh->get_border_size() > 0.0) {
		cfg.borderSize = (int)Math::ceil(p_navigation_mesh->get_border_size() / cfg.cs);
	}
	cfg.walkableSlopeAngle = p_navigation_mesh->get_agent_max_slope();
	cfg.walkableHeight = (int)Math::ceil(p_navigation_mesh->get_agent_height() / cfg.ch);
	cfg.walkableClimb = (int)Math::floor(p_navigation_mesh->get_agent_max_climb() / cfg.ch);
	cfg.walkableRadius = (int)Math::ceil(p_navigation_mesh->get_agent_radius() / cfg.cs);
	cfg.maxEdgeLen =
		(int)(p_navigation_mesh->get_edge_max_length() / p_navigation_mesh->get_cell_size());
	cfg.maxSimplificationError = p_navigation_mesh->get_edge_max_error();
	cfg.minRegionArea =
		(int)(p_navigation_mesh->get_region_min_size() * p_navigation_mesh->get_region_min_size());
	cfg.mergeRegionArea = (int)(p_navigation_mesh->get_region_merge_size() *
								p_navigation_mesh->get_region_merge_size());
	cfg.maxVertsPerPoly = (int)p_navigation_mesh->get_vertices_per_polygon();
	cfg.detailSampleDist = MAX(
		p_navigation_mesh->get_cell_size() * p_navigation_mesh->get_detail_sample_distance(), 0.1f);
	cfg.detailSampleMaxError =
		p_navigation_mesh->get_cell_height() * p_navigation_mesh->get_detail_sample_max_error();

	if (p_navigation_mesh->get_border_size() > 0.0 &&
		!Math::is_zero_approx(
			Math::fmod(p_navigation_mesh->get_border_size(), p_navigation_mesh->get_cell_size()))) {
		WARN_PRINT("Property border_size is ceiled to cell_size voxel units and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.walkableHeight * cfg.ch, p_navigation_mesh->get_agent_height())) {
		WARN_PRINT(
			"Property agent_height is ceiled to cell_height voxel units and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.walkableClimb * cfg.ch, p_navigation_mesh->get_agent_max_climb())) {
		WARN_PRINT(
			"Property agent_max_climb is floored to cell_height voxel units and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.walkableRadius * cfg.cs, p_navigation_mesh->get_agent_radius())) {
		WARN_PRINT("Property agent_radius is ceiled to cell_size voxel units and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.maxEdgeLen * cfg.cs, p_navigation_mesh->get_edge_max_length())) {
		WARN_PRINT(
			"Property edge_max_length is rounded to cell_size voxel units and loses precision.");
	}
	if (!Math::is_equal_approx((float)cfg.minRegionArea,
			p_navigation_mesh->get_region_min_size() * p_navigation_mesh->get_region_min_size())) {
		WARN_PRINT("Property region_min_size is converted to int and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.mergeRegionArea, p_navigation_mesh->get_region_merge_size() *
											p_navigation_mesh->get_region_merge_size())) {
		WARN_PRINT("Property region_merge_size is converted to int and loses precision.");
	}
	if (!Math::is_equal_approx(
			(float)cfg.maxVertsPerPoly, p_navigation_mesh->get_vertices_per_polygon())) {
		WARN_PRINT("Property vertices_per_polygon is converted to int and loses precision.");
	}
	if (p_navigation_mesh->get_cell_size() * p_navigation_mesh->get_detail_sample_distance() <
		0.1f) {
		WARN_PRINT("Property detail_sample_distance is clamped to 0.1 world units as the resulting "
				   "value from multiplying with cell_size is too low.");
	}

	cfg.bmin[0] = bmin[0];
	cfg.bmin[1] = bmin[1];
	cfg.bmin[2] = bmin[2];
	cfg.bmax[0] = bmax[0];
	cfg.bmax[1] = bmax[1];
	cfg.bmax[2] = bmax[2];

	AABB baking_aabb = p_navigation_mesh->get_filter_baking_aabb();
	if (baking_aabb.has_volume()) {
		Vector3 baking_aabb_offset = p_navigation_mesh->get_filter_baking_aabb_offset();
		cfg.bmin[0] = baking_aabb.position[0] + baking_aabb_offset.x;
		cfg.bmin[1] = baking_aabb.position[1] + baking_aabb_offset.y;
		cfg.bmin[2] = baking_aabb.position[2] + baking_aabb_offset.z;
		cfg.bmax[0] = cfg.bmin[0] + baking_aabb.size[0];
		cfg.bmax[1] = cfg.bmin[1] + baking_aabb.size[1];
		cfg.bmax[2] = cfg.bmin[2] + baking_aabb.size[2];
	}

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_CALC_GRID_SIZE;
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	if ((cfg.width * cfg.height) > 30000000) {
		ERR_FAIL_MSG("Baking interrupted."
					 "\nNavigationMesh baking process would likely crash the engine."
					 "\nSource geometry is suspiciously big for the current Cell Size and Cell "
					 "Height in the NavMesh Resource bake settings."
					 "\nIf baking does not crash the engine or fail, the resulting NavigationMesh "
					 "will create serious pathfinding performance issues."
					 "\nIt is advised to increase Cell Size and/or Cell Height in the NavMesh "
					 "Resource bake settings or reduce the size / scale of the source geometry."
					 "\nIf you would like to try baking anyway, disable the "
					 "'navigation/baking/use_crash_prevention_checks' project setting.");
		return;
	}

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_CREATE_HEIGHTFIELD;
	hf = rcAllocHeightfield();

	ERR_FAIL_NULL(hf);
	ERR_FAIL_COND(
		!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch));

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_MARK_WALKABLE_TRIANGLES;
	{
		Vector<unsigned char> tri_areas;
		tri_areas.resize(ntris);

		ERR_FAIL_COND(tri_areas.is_empty());

		memset(tri_areas.ptrw(), 0, ntris * sizeof(unsigned char));
		rcMarkWalkableTriangles(
			&ctx, cfg.walkableSlopeAngle, verts, nverts, tris, ntris, tri_areas.ptrw());

		ERR_FAIL_COND(!rcRasterizeTriangles(
			&ctx, verts, nverts, tris, tri_areas.ptr(), ntris, *hf, cfg.walkableClimb));
	}

	if (p_navigation_mesh->get_filter_low_hanging_obstacles()) {
		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
	}
	if (p_navigation_mesh->get_filter_ledge_spans()) {
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
	}
	if (p_navigation_mesh->get_filter_walkable_low_height_spans()) {
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);
	}

	p_generator_task->bake_state =
		NavMeshBakeState::BAKE_STATE_CONSTRUCT_COMPACT_HEIGHTFIELD;

	chf = rcAllocCompactHeightfield();

	ERR_FAIL_NULL(chf);
	ERR_FAIL_COND(
		!rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf));

	rcFreeHeightField(hf);
	hf = nullptr;

	if (!projected_obstructions.is_empty()) {
		for (const NavigationMeshSourceGeometryData3D::ProjectedObstruction& projected_obstruction :
			projected_obstructions) {
			if (projected_obstruction.carve) {
				continue;
			}
			if (projected_obstruction.vertices.is_empty() ||
				projected_obstruction.vertices.size() % 3 != 0) {
				continue;
			}

			const float* projected_obstruction_verts = projected_obstruction.vertices.ptr();
			const int projected_obstruction_nverts = projected_obstruction.vertices.size() / 3;

			rcMarkConvexPolyArea(&ctx, projected_obstruction_verts, projected_obstruction_nverts,
				projected_obstruction.elevation,
				projected_obstruction.elevation + projected_obstruction.height, RC_NULL_AREA, *chf);
		}
	}

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_ERODE_WALKABLE_AREA;

	ERR_FAIL_COND(!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf));

	if (!projected_obstructions.is_empty()) {
		for (const NavigationMeshSourceGeometryData3D::ProjectedObstruction& projected_obstruction :
			projected_obstructions) {
			if (!projected_obstruction.carve) {
				continue;
			}
			if (projected_obstruction.vertices.is_empty() ||
				projected_obstruction.vertices.size() % 3 != 0) {
				continue;
			}

			const float* projected_obstruction_verts = projected_obstruction.vertices.ptr();
			const int projected_obstruction_nverts = projected_obstruction.vertices.size() / 3;

			rcMarkConvexPolyArea(&ctx, projected_obstruction_verts, projected_obstruction_nverts,
				projected_obstruction.elevation,
				projected_obstruction.elevation + projected_obstruction.height, RC_NULL_AREA, *chf);
		}
	}

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_SAMPLE_PARTITIONING;

	if (p_navigation_mesh->get_sample_partition_type() ==
		NavigationMesh::SAMPLE_PARTITION_WATERSHED) {
		ERR_FAIL_COND(!rcBuildDistanceField(&ctx, *chf));
		ERR_FAIL_COND(
			!rcBuildRegions(&ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea));
	}
	else if (p_navigation_mesh->get_sample_partition_type() ==
			   NavigationMesh::SAMPLE_PARTITION_MONOTONE) {
		ERR_FAIL_COND(!rcBuildRegionsMonotone(
			&ctx, *chf, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea));
	}
	else {
		ERR_FAIL_COND(!rcBuildLayerRegions(&ctx, *chf, cfg.borderSize, cfg.minRegionArea));
	}

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_CREATING_CONTOURS;

	cset = rcAllocContourSet();

	ERR_FAIL_NULL(cset);
	ERR_FAIL_COND(!rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset));

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_CREATING_POLYMESH;

	poly_mesh = rcAllocPolyMesh();
	ERR_FAIL_NULL(poly_mesh);
	ERR_FAIL_COND(!rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *poly_mesh));

	detail_mesh = rcAllocPolyMeshDetail();
	ERR_FAIL_NULL(detail_mesh);
	ERR_FAIL_COND(!rcBuildPolyMeshDetail(
		&ctx, *poly_mesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *detail_mesh));

	rcFreeCompactHeightfield(chf);
	chf = nullptr;
	rcFreeContourSet(cset);
	cset = nullptr;

	p_generator_task->bake_state =
		NavMeshBakeState::BAKE_STATE_CONVERTING_NATIVE_NAVMESH;

	Vector<Vector3> nav_vertices;
	Vector<Vector<int>> nav_polygons;

	HashMap<Vector3, int> recast_vertex_to_native_index;
	LocalVector<int> recast_index_to_native_index;
	recast_index_to_native_index.resize(detail_mesh->nverts);

	for (int i = 0; i < detail_mesh->nverts; i++) {
		const float* v = &detail_mesh->verts[i * 3];
		const Vector3 vertex = Vector3(v[0], v[1], v[2]);
		int* existing_index_ptr = recast_vertex_to_native_index.getptr(vertex);
		if (!existing_index_ptr) {
			int new_index = recast_vertex_to_native_index.size();
			recast_index_to_native_index[i] = new_index;
			recast_vertex_to_native_index[vertex] = new_index;
			nav_vertices.push_back(vertex);
		}
		else {
			recast_index_to_native_index[i] = *existing_index_ptr;
		}
	}

	for (int i = 0; i < detail_mesh->nmeshes; i++) {
		const unsigned int* detail_mesh_m = &detail_mesh->meshes[i * 4];
		const unsigned int detail_mesh_bverts = detail_mesh_m[0];
		const unsigned int detail_mesh_m_btris = detail_mesh_m[2];
		const unsigned int detail_mesh_ntris = detail_mesh_m[3];
		const unsigned char* detail_mesh_tris = &detail_mesh->tris[detail_mesh_m_btris * 4];
		for (unsigned int j = 0; j < detail_mesh_ntris; j++) {
			Vector<int> nav_indices;
			nav_indices.resize(3);
			int index1 = ((int)(detail_mesh_bverts + detail_mesh_tris[j * 4 + 0]));
			int index2 = ((int)(detail_mesh_bverts + detail_mesh_tris[j * 4 + 2]));
			int index3 = ((int)(detail_mesh_bverts + detail_mesh_tris[j * 4 + 1]));

			nav_indices.write[0] = recast_index_to_native_index[index1];
			nav_indices.write[1] = recast_index_to_native_index[index2];
			nav_indices.write[2] = recast_index_to_native_index[index3];

			nav_polygons.push_back(nav_indices);
		}
	}

	p_navigation_mesh->set_data(nav_vertices, nav_polygons);

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_BAKE_CLEANUP;

	rcFreePolyMesh(poly_mesh);
	poly_mesh = nullptr;
	rcFreePolyMeshDetail(detail_mesh);
	detail_mesh = nullptr;

	p_generator_task->bake_state = NavMeshBakeState::BAKE_STATE_BAKE_FINISHED;
}

void NavMeshGenerator3D::sync() {}

void NavMeshGenerator3D::cleanup() {}

#ifndef DISABLE_DEPRECATED
void NavigationMeshGenerator::initialize()
{
	_initialized = true;
}

void NavigationMeshGenerator::finalize()
{
	_initialized = false;
}

void NavigationMeshGenerator::bake(const Ref<NavigationMesh>& p_navigation_mesh, Node* p_root_node)
{
	WARN_PRINT_ONCE(
		"NavigationMeshGenerator::bake() is deprecated due to core threading changes. To upgrade "
		"existing code, first create a NavigationMeshSourceGeometryData3D resource. Use this "
		"resource with method parse_source_geometry_data() to parse the SceneTree for nodes that "
		"should contribute to the navigation mesh baking. The SceneTree parsing needs to happen on "
		"the main thread. After the parsing is finished use the resource with method "
		"bake_from_source_geometry_data() to bake a navigation mesh..");
}

void NavigationMeshGenerator::clear(Ref<NavigationMesh> p_navigation_mesh)
{
	if (p_navigation_mesh.is_valid()) {
		p_navigation_mesh->clear_polygons();
		p_navigation_mesh->set_vertices(Vector<Vector3>());
	}
}
#endif // DISABLE_DEPRECATED

void uninitialize_navigation_3d_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

#ifndef DISABLE_DEPRECATED
	NavigationMeshGenerator::finalize();
#endif // DISABLE_DEPRECATED
	NavMeshGenerator3D::finalize();
}

void initialize_navigation_3d_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		NavMeshGenerator3D::initialize();
#ifndef DISABLE_DEPRECATED
		NavigationMeshGenerator::initialize();
#endif // DISABLE_DEPRECATED
	}
}
