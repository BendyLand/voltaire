/**************************************************************************/
/*  nav_agent_2d.cpp                                                      */
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

#include "nav_agent_2d.h"
#include "nav_map_2d.h"

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


