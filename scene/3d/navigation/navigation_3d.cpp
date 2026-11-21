/**************************************************************************/
/*  navigation_3d.cpp                                                     */
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

#include "navigation_3d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "core/math/random_pcg.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/renderer.h"

void NavigationAgent3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are
		// ready. cannot use READY as ready does not get called if Node is re-added to SceneTree
		set_agent_parent(get_parent());
		set_physics_process_internal(true);

		if (agent_parent && avoidance_enabled) {
			NavigationServer3D::agent_set_position(
				agent, agent_parent->get_global_transform().origin);
		}

#ifdef DEBUG_ENABLED
		if (NavigationServer3D::get_debug_enabled()) {
			debug_path_dirty = true;
		}
#endif // DEBUG_ENABLED

	} break;

	case NOTIFICATION_PARENTED: {
		if (is_inside_tree() && (get_parent() != agent_parent)) {
			// only react to PARENTED notifications when already inside_tree and parent changed,
			// e.g. users switch nodes around PARENTED notification fires also when Node is added in
			// scripts to a parent this would spam transforms fails and world fails while Node is
			// outside SceneTree when node gets reparented when joining the tree POST_ENTER_TREE
			// takes care of this
			set_agent_parent(get_parent());
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_UNPARENTED: {
		// if agent has no parent no point in processing it until reparented
		set_agent_parent(nullptr);
		set_physics_process_internal(false);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_agent_parent(nullptr);
		set_physics_process_internal(false);

#ifdef DEBUG_ENABLED
		if (debug_path_instance.is_valid()) {
			Renderer::instance_set_visible(debug_path_instance, false);
		}
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (agent_parent) {
			NavigationServer3D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (agent_parent) {
			NavigationServer3D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (agent_parent && avoidance_enabled) {
			NavigationServer3D::agent_set_position(
				agent, agent_parent->get_global_position());
		}
		if (agent_parent && target_position_submitted) {
			if (velocity_submitted) {
				velocity_submitted = false;
				if (avoidance_enabled) {
					if (!use_3d_avoidance) {
						if (keep_y_velocity) {
							stored_y_velocity = velocity.y;
						}
						velocity.y = 0.0;
					}
					NavigationServer3D::agent_set_velocity(agent, velocity);
				}
			}
			if (velocity_forced_submitted) {
				velocity_forced_submitted = false;
				if (avoidance_enabled) {
					NavigationServer3D::agent_set_velocity_forced(
						agent, velocity_forced);
				}
			}
		}
#ifdef DEBUG_ENABLED
		if (debug_path_dirty) {
			_update_debug_path();
		}
#endif // DEBUG_ENABLED
	} break;
	}
}









bool NavigationAgent3D::get_avoidance_enabled() const { return avoidance_enabled; }



void NavigationAgent3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	if (target_position_submitted) {
		_request_repath();
	}
}

uint32_t NavigationAgent3D::get_navigation_layers() const { return navigation_layers; }

void NavigationAgent3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");
	uint32_t _navigation_layers = get_navigation_layers();
	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}
	set_navigation_layers(_navigation_layers);
}

bool NavigationAgent3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");
	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent3D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	if (pathfinding_algorithm == p_pathfinding_algorithm) {
		return;
	}

	pathfinding_algorithm = p_pathfinding_algorithm;

	navigation_query->set_pathfinding_algorithm(pathfinding_algorithm);
}

void NavigationAgent3D::set_path_postprocessing(
	const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing)
{
	if (path_postprocessing == p_path_postprocessing) {
		return;
	}

	path_postprocessing = p_path_postprocessing;

	navigation_query->set_path_postprocessing(path_postprocessing);
}

void NavigationAgent3D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
	navigation_query->set_simplify_path(simplify_path);
}

bool NavigationAgent3D::get_simplify_path() const { return simplify_path; }

void NavigationAgent3D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
	navigation_query->set_simplify_epsilon(simplify_epsilon);
}

real_t NavigationAgent3D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationAgent3D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
	navigation_query->set_path_return_max_length(path_return_max_length);
}

float NavigationAgent3D::get_path_return_max_length() const { return path_return_max_length; }

void NavigationAgent3D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
	navigation_query->set_path_return_max_radius(path_return_max_radius);
}

float NavigationAgent3D::get_path_return_max_radius() const { return path_return_max_radius; }

void NavigationAgent3D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
	navigation_query->set_path_search_max_polygons(path_search_max_polygons);
}

int NavigationAgent3D::get_path_search_max_polygons() const { return path_search_max_polygons; }

void NavigationAgent3D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
	navigation_query->set_path_search_max_distance(path_search_max_distance);
}

float NavigationAgent3D::get_path_search_max_distance() const { return path_search_max_distance; }

float NavigationAgent3D::get_path_length() const { return navigation_result->get_path_length(); }

void NavigationAgent3D::set_path_metadata_flags(
	uint32_t p_path_metadata_flags)
{
	if (path_metadata_flags == p_path_metadata_flags) {
		return;
	}

	path_metadata_flags = p_path_metadata_flags;
}

void NavigationAgent3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::agent_set_map(agent, map_override);
	if (target_position_submitted) {
		_request_repath();
	}
}

RID NavigationAgent3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (agent_parent != nullptr) {
		return agent_parent->get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationAgent3D::set_path_desired_distance(real_t p_path_desired_distance)
{
	if (Math::is_equal_approx(path_desired_distance, p_path_desired_distance)) {
		return;
	}

	path_desired_distance = p_path_desired_distance;
}

void NavigationAgent3D::set_target_desired_distance(real_t p_target_desired_distance)
{
	if (Math::is_equal_approx(target_desired_distance, p_target_desired_distance)) {
		return;
	}

	target_desired_distance = p_target_desired_distance;
}

void NavigationAgent3D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}
	radius = p_radius;

	NavigationServer3D::agent_set_radius(agent, radius);
}

void NavigationAgent3D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0, "Height must be positive.");
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}
	height = p_height;
	NavigationServer3D::agent_set_height(agent, height);
}

void NavigationAgent3D::set_path_height_offset(real_t p_path_height_offset)
{
	path_height_offset = p_path_height_offset;
}



void NavigationAgent3D::set_keep_y_velocity(bool p_enabled)
{
	keep_y_velocity = p_enabled;
	stored_y_velocity = 0.0;
}

bool NavigationAgent3D::get_keep_y_velocity() const { return keep_y_velocity; }

void NavigationAgent3D::set_neighbor_distance(real_t p_distance)
{
	if (Math::is_equal_approx(neighbor_distance, p_distance)) {
		return;
	}

	neighbor_distance = p_distance;

	NavigationServer3D::agent_set_neighbor_distance(agent, neighbor_distance);
}

void NavigationAgent3D::set_max_neighbors(int p_count)
{
	if (max_neighbors == p_count) {
		return;
	}

	max_neighbors = p_count;

	NavigationServer3D::agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent3D::set_time_horizon_agents(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_agents, p_time_horizon)) {
		return;
	}
	time_horizon_agents = p_time_horizon;
	NavigationServer3D::agent_set_time_horizon_agents(agent, time_horizon_agents);
}

void NavigationAgent3D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_obstacles, p_time_horizon)) {
		return;
	}
	time_horizon_obstacles = p_time_horizon;
	NavigationServer3D::agent_set_time_horizon_obstacles(
		agent, time_horizon_obstacles);
}

void NavigationAgent3D::set_max_speed(real_t p_max_speed)
{
	ERR_FAIL_COND_MSG(p_max_speed < 0.0, "Max speed must be positive.");
	if (Math::is_equal_approx(max_speed, p_max_speed)) {
		return;
	}
	max_speed = p_max_speed;

	NavigationServer3D::agent_set_max_speed(agent, max_speed);
}

void NavigationAgent3D::set_path_max_distance(real_t p_path_max_distance)
{
	if (Math::is_equal_approx(path_max_distance, p_path_max_distance)) {
		return;
	}

	path_max_distance = p_path_max_distance;
}

real_t NavigationAgent3D::get_path_max_distance() { return path_max_distance; }

void NavigationAgent3D::set_target_position(Vector3 p_position)
{
	// Intentionally not checking for equality of the parameter, as we want to update the path even
	// if the target position is the same in case the world changed. Revisit later when the
	// navigation server can update the path without requesting a new path.

	target_position = p_position;
	target_position_submitted = true;

	_request_repath();
}

Vector3 NavigationAgent3D::get_target_position() const { return target_position; }

Vector3 NavigationAgent3D::get_next_path_position()
{
	_update_navigation();

	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		ERR_FAIL_NULL_V_MSG(agent_parent, Vector3(), "The agent has no parent.");
		return agent_parent->get_global_position();
	}
	else {
		return navigation_path[navigation_path_index] - Vector3(0, path_height_offset, 0);
	}
}

real_t NavigationAgent3D::distance_to_target() const
{
	ERR_FAIL_NULL_V_MSG(agent_parent, 0.0, "The agent has no parent.");
	return agent_parent->get_global_position().distance_to(target_position);
}

bool NavigationAgent3D::is_target_reached() const { return target_reached; }

bool NavigationAgent3D::is_target_reachable()
{
	_update_navigation();
	return _is_target_reachable();
}

bool NavigationAgent3D::_is_target_reachable() const
{
	return target_desired_distance >= _get_final_position().distance_to(target_position);
}

bool NavigationAgent3D::is_navigation_finished()
{
	_update_navigation();
	return navigation_finished;
}

Vector3 NavigationAgent3D::get_final_position()
{
	_update_navigation();
	return _get_final_position();
}

Vector3 NavigationAgent3D::_get_final_position() const
{
	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		return Vector3();
	}
	return navigation_path[navigation_path.size() - 1] - Vector3(0, path_height_offset, 0);
}

void NavigationAgent3D::set_velocity_forced(Vector3 p_velocity)
{
	// Intentionally not checking for equality of the parameter.
	// We need to always submit the velocity to the navigation server, even when it is the same, in
	// order to run avoidance every frame. Revisit later when the navigation server can update
	// avoidance without users resubmitting the velocity.

	velocity_forced = p_velocity;
	velocity_forced_submitted = true;
}

void NavigationAgent3D::set_velocity(const Vector3 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}







void NavigationAgent3D::_advance_waypoints(const Vector3& p_origin)
{
	if (last_waypoint_reached) {
		return;
	}

	// Advance to the farthest possible waypoint.
	while (_is_within_waypoint_distance(p_origin)) {
		_trigger_waypoint_reached();

		if (_is_last_waypoint()) {
			last_waypoint_reached = true;
			break;
		}

		_move_to_next_waypoint();
	}
}

void NavigationAgent3D::_request_repath()
{
	navigation_result->reset();
	target_reached = false;
	navigation_finished = false;
	last_waypoint_reached = false;
}

bool NavigationAgent3D::_is_last_waypoint() const
{
	return navigation_path_index == navigation_result->get_path().size() - 1;
}

void NavigationAgent3D::_move_to_next_waypoint() { navigation_path_index += 1; }

bool NavigationAgent3D::_is_within_waypoint_distance(const Vector3& p_origin) const
{
	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	Vector3 waypoint = navigation_path[navigation_path_index] - Vector3(0, path_height_offset, 0);
	return p_origin.distance_to(waypoint) < path_desired_distance;
}

bool NavigationAgent3D::_is_within_target_distance(const Vector3& p_origin) const
{
	return p_origin.distance_to(target_position) < target_desired_distance;
}







void NavigationAgent3D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer3D::agent_set_avoidance_layers(get_rid(), avoidance_layers);
}

uint32_t NavigationAgent3D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationAgent3D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	NavigationServer3D::agent_set_avoidance_mask(get_rid(), avoidance_mask);
}

uint32_t NavigationAgent3D::get_avoidance_mask() const { return avoidance_mask; }

void NavigationAgent3D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationAgent3D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent3D::set_avoidance_mask_value(int p_mask_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_mask_number < 1, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_mask_number > 32, "Avoidance mask number must be between 1 and 32 inclusive.");
	uint32_t mask = get_avoidance_mask();
	if (p_value) {
		mask |= 1 << (p_mask_number - 1);
	}
	else {
		mask &= ~(1 << (p_mask_number - 1));
	}
	set_avoidance_mask(mask);
}

bool NavigationAgent3D::get_avoidance_mask_value(int p_mask_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_mask_number < 1, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_mask_number > 32, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	return get_avoidance_mask() & (1 << (p_mask_number - 1));
}

void NavigationAgent3D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	NavigationServer3D::agent_set_avoidance_priority(get_rid(), p_priority);
}

real_t NavigationAgent3D::get_avoidance_priority() const { return avoidance_priority; }

////////DEBUG////////////////////////////////////////////////////////////

void NavigationAgent3D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled == p_enabled) {
		return;
	}

	debug_enabled = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent3D::get_debug_enabled() const { return debug_enabled; }

void NavigationAgent3D::set_debug_use_custom(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_use_custom == p_enabled) {
		return;
	}

	debug_use_custom = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent3D::get_debug_use_custom() const { return debug_use_custom; }

void NavigationAgent3D::set_debug_path_custom_color(Color p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_path_custom_color == p_color) {
		return;
	}

	debug_path_custom_color = p_color;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

Color NavigationAgent3D::get_debug_path_custom_color() const { return debug_path_custom_color; }

void NavigationAgent3D::set_debug_path_custom_point_size(float p_point_size)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_point_size, p_point_size)) {
		return;
	}

	debug_path_custom_point_size = MAX(0.0, p_point_size);
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent3D::get_debug_path_custom_point_size() const
{
	return debug_path_custom_point_size;
}

#ifdef DEBUG_ENABLED
void NavigationAgent3D::_navigation_debug_changed() { debug_path_dirty = true; }
#endif // DEBUG_ENABLED



void NavigationAgent3D::_update_navigation() {}

void NavigationAgent3D::set_agent_parent(Node*) {}

#ifdef DEBUG_ENABLED
void NavigationAgent3D::_update_debug_path() {}
#endif // DEBUG_ENABLED

void NavigationAgent3D::_trigger_waypoint_reached() {}


void NavigationLink3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_link_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_link_update_transform();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_link_exit_navigation_map();
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_debug_mesh();
	} break;
#endif // DEBUG_ENABLED
	}
}

NavigationLink3D::~NavigationLink3D()
{
	ERR_FAIL_NULL(NavigationServer3D::data);
	NavigationServer3D::free_rid(link);
	link = RID();

#ifdef DEBUG_ENABLED
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
	}
#endif // DEBUG_ENABLED
}

RID NavigationLink3D::get_rid() const { return link; }

void NavigationLink3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::link_set_map(link, map_override);
}

RID NavigationLink3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationLink3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::link_set_navigation_layers(link, navigation_layers);
}

void NavigationLink3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationLink3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

Vector3 NavigationLink3D::get_global_start_position() const
{
	if (is_inside_tree()) {
		return to_global(start_position);
	}
	else {
		return start_position;
	}
}

Vector3 NavigationLink3D::get_global_end_position() const
{
	if (is_inside_tree()) {
		return to_global(end_position);
	}
	else {
		return end_position;
	}
}

void NavigationLink3D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer3D::link_set_enter_cost(link, enter_cost);
}

void NavigationLink3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::link_set_travel_cost(link, travel_cost);
}

PackedStringArray NavigationLink3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (start_position.is_equal_approx(end_position)) {
		warnings.push_back(RTR("NavigationLink3D start position should be different than the end "
							   "position to be useful."));
	}

	return warnings;
}

void NavigationLink3D::_link_enter_navigation_map()
{
	if (!is_inside_tree()) {
		return;
	}

	if (map_override.is_valid()) {
		NavigationServer3D::link_set_map(link, map_override);
	}
	else {
		NavigationServer3D::link_set_map(
			link, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::link_set_end_position(
		link, get_global_transform().xform(end_position));
	NavigationServer3D::link_set_enabled(link, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_exit_navigation_map()
{
	NavigationServer3D::link_set_map(link, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_visible(debug_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::link_set_end_position(
		link, get_global_transform().xform(end_position));
#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}



#ifdef DEBUG_ENABLED
void NavigationLink3D::_update_debug_mesh() {}
#endif // DEBUG_ENABLED


RID NavigationObstacle3D::_navmesh_source_geometry_parser;

void NavigationObstacle3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (map_override.is_valid()) {
			_update_map(map_override);
		}
		else if (is_inside_tree()) {
			_update_map(get_world_3d()->get_navigation_map());
		}
		else {
			_update_map(RID());
		}
		// need to trigger map controlled agent assignment somehow for the fake_agent since
		// obstacles use no callback like regular agents
		NavigationServer3D::obstacle_set_avoidance_enabled(
			obstacle, avoidance_enabled);
		_update_transform();
		set_physics_process_internal(true);
#ifdef DEBUG_ENABLED
		_update_debug();
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_physics_process_internal(false);
		_update_map(RID());
#ifdef DEBUG_ENABLED
		_clear_debug();
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer3D::obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer3D::obstacle_set_paused(obstacle, !can_process());
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_debug();
	} break;
#endif // DEBUG_ENABLED

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_inside_tree()) {
			_update_transform();

			if (velocity_submitted) {
				velocity_submitted = false;
				// only update if there is a noticeable change, else the rvo agent preferred
				// velocity stays the same
				if (!previous_velocity.is_equal_approx(velocity)) {
					NavigationServer3D::obstacle_set_velocity(obstacle, velocity);
				}
				previous_velocity = velocity;
			}
#ifdef DEBUG_ENABLED
			if (fake_agent_radius_debug_instance_rid.is_valid() && radius > 0.0) {
				// Prevent non-positive scaling.
				const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
				// Agent radius is a scalar value and does not support non-uniform scaling, choose
				// the largest axis.
				const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
				const Vector3 uniform_max_scale =
					Vector3(scaling_max_value, scaling_max_value, scaling_max_value);
				const Transform3D debug_transform =
					Transform3D(Basis().scaled(uniform_max_scale), get_global_position());

				Renderer::instance_set_transform(
					fake_agent_radius_debug_instance_rid, debug_transform);
			}
			if (static_obstacle_debug_instance_rid.is_valid() && get_vertices().size() > 0) {
				// Prevent non-positive scaling.
				const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
				// Obstacles are projected to the xz-plane, so only rotation around the y-axis can
				// be taken into account.
				const Transform3D debug_transform =
					Transform3D(Basis()
									.scaled(safe_scale)
									.rotated(Vector3(0.0, 1.0, 0.0), get_global_rotation().y),
						get_global_position());

				Renderer::instance_set_transform(
					static_obstacle_debug_instance_rid, debug_transform);
			}
#endif // DEBUG_ENABLED
		}
	} break;
	}
}

void NavigationObstacle3D::set_vertices(const Vector<Vector3>& p_vertices)
{
	vertices = p_vertices;

	Vector<Vector2> vertices_2d;
	vertices_2d.resize(vertices.size());

	const Vector3* vertices_ptr = vertices.ptr();
	Vector2* vertices_2d_ptrw = vertices_2d.ptrw();

	for (int i = 0; i < vertices.size(); i++) {
		vertices_2d_ptrw[i] = Vector2(vertices_ptr[i].x, vertices_ptr[i].z);
	}

	vertices_are_clockwise =
		!Geometry2D::is_polygon_clockwise(vertices_2d); // Geometry2D is inverted.
	vertices_are_valid = !Geometry2D::triangulate_polygon(vertices_2d).is_empty();

	const Basis basis = is_inside_tree() ? get_global_basis() : get_basis();
	const float rotation_y = is_inside_tree() ? get_global_rotation().y : get_rotation().y;
	const Vector3 safe_scale = basis.get_scale().abs().maxf(0.001);
	const Transform3D safe_transform = Transform3D(
		Basis().scaled(safe_scale).rotated(Vector3(0.0, 1.0, 0.0), rotation_y), Vector3());
	NavigationServer3D::obstacle_set_vertices(
		obstacle, safe_transform.xform(vertices));
}

void NavigationObstacle3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}
	map_override = p_navigation_map;
	_update_map(map_override);
}

RID NavigationObstacle3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationObstacle3D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;

	// Prevent non-positive or non-uniform scaling of dynamic obstacle radius.
	const Vector3 safe_scale =
		(is_inside_tree() ? get_global_basis() : get_basis()).get_scale().abs().maxf(0.001);
	NavigationServer3D::obstacle_set_radius(
		obstacle, safe_scale[safe_scale.max_axis_index()] * radius);
}

void NavigationObstacle3D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0, "Height must be positive.");
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}

	height = p_height;
	const float scale_factor =
		MAX(Math::abs((is_inside_tree() ? get_global_basis() : get_basis()).get_scale().y), 0.001);
	NavigationServer3D::obstacle_set_height(obstacle, scale_factor * height);
}

void NavigationObstacle3D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer3D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
}

uint32_t NavigationObstacle3D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationObstacle3D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationObstacle3D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationObstacle3D::set_avoidance_enabled(bool p_enabled)
{
	if (avoidance_enabled == p_enabled) {
		return;
	}

	avoidance_enabled = p_enabled;
	NavigationServer3D::obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);
}

bool NavigationObstacle3D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationObstacle3D::set_velocity(const Vector3 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationObstacle3D::set_affect_navigation_mesh(bool p_enabled)
{
	affect_navigation_mesh = p_enabled;
}

bool NavigationObstacle3D::get_affect_navigation_mesh() const { return affect_navigation_mesh; }

void NavigationObstacle3D::set_carve_navigation_mesh(bool p_enabled)
{
	carve_navigation_mesh = p_enabled;
}

bool NavigationObstacle3D::get_carve_navigation_mesh() const { return carve_navigation_mesh; }

PackedStringArray NavigationObstacle3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (get_global_rotation().x != 0.0 || get_global_rotation().z != 0.0) {
		warnings.push_back(
			RTR("NavigationObstacle3D only takes global rotation around the y-axis into account. "
				"Rotations around the x-axis or z-axis might lead to unexpected results."));
	}

	const Vector3 global_scale = get_global_basis().get_scale();
	if (global_scale.x < 0.001 || global_scale.y < 0.001 || global_scale.z < 0.001) {
		warnings.push_back(RTR("NavigationObstacle3D does not support negative or zero scaling."));
	}

	if (radius > 0.0 && !get_global_basis().is_conformal()) {
		warnings.push_back(RTR("The agent radius can only be scaled uniformly. The largest scale "
							   "value along the three axes will be used."));
	}

	return warnings;
}

void NavigationObstacle3D::_update_map(RID p_map)
{
	NavigationServer3D::obstacle_set_map(obstacle, p_map);
	map_current = p_map;
}

void NavigationObstacle3D::_update_position(const Vector3 p_position)
{
	NavigationServer3D::obstacle_set_position(obstacle, p_position);
}

void NavigationObstacle3D::_update_transform()
{
	_update_position(get_global_position());

	// Prevent non-positive or non-uniform scaling of dynamic obstacle radius.
	const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
	const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
	NavigationServer3D::obstacle_set_radius(obstacle, scaling_max_value * radius);

	// Apply modified node transform which only takes y-axis rotation into account to vertices.
	const Transform3D safe_transform = Transform3D(
		Basis().scaled(safe_scale).rotated(Vector3(0.0, 1.0, 0.0), get_global_rotation().y),
		Vector3());
	NavigationServer3D::obstacle_set_vertices(
		obstacle, safe_transform.xform(vertices));
	NavigationServer3D::obstacle_set_height(obstacle, safe_scale.y * height);
}

void NavigationObstacle3D::_update_use_3d_avoidance(bool p_use_3d_avoidance)
{
	NavigationServer3D::obstacle_set_use_3d_avoidance(obstacle, use_3d_avoidance);
	_update_map(map_current);
}

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_update_debug()
{
	if (is_inside_tree()) {
		Renderer::instance_set_visible(fake_agent_radius_debug_instance_rid, is_visible_in_tree());
		Renderer::instance_set_visible(static_obstacle_debug_instance_rid, is_visible_in_tree());
		Renderer::instance_set_scenario(
			fake_agent_radius_debug_instance_rid, get_world_3d()->get_scenario());
		Renderer::instance_set_scenario(
			static_obstacle_debug_instance_rid, get_world_3d()->get_scenario());
		Renderer::instance_set_transform(
			fake_agent_radius_debug_instance_rid, Transform3D(Basis(), get_global_position()));
		Renderer::instance_set_transform(
			static_obstacle_debug_instance_rid, Transform3D(Basis(), get_global_position()));
		_update_fake_agent_radius_debug();
		_update_static_obstacle_debug();
	}
	else {
		Renderer::mesh_clear(fake_agent_radius_debug_mesh_rid);
		Renderer::mesh_clear(static_obstacle_debug_mesh_rid);
		Renderer::instance_set_scenario(fake_agent_radius_debug_instance_rid, RID());
		Renderer::instance_set_scenario(static_obstacle_debug_instance_rid, RID());
	}
}

#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_clear_debug()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::mesh_clear(fake_agent_radius_debug_mesh_rid);
	Renderer::mesh_clear(static_obstacle_debug_mesh_rid);
	Renderer::instance_set_scenario(fake_agent_radius_debug_instance_rid, RID());
	Renderer::instance_set_scenario(static_obstacle_debug_instance_rid, RID());
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_update_static_obstacle_debug() {}

void NavigationObstacle3D::_update_fake_agent_radius_debug() {}
#endif // DEBUG_ENABLED


RID NavigationRegion3D::get_rid() const { return region; }

bool NavigationRegion3D::is_enabled() const { return enabled; }

void NavigationRegion3D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer3D::region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion3D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::region_set_navigation_layers(region, navigation_layers);
}

uint32_t NavigationRegion3D::get_navigation_layers() const { return navigation_layers; }

void NavigationRegion3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationRegion3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationRegion3D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer3D::region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion3D::get_enter_cost() const { return enter_cost; }

void NavigationRegion3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::region_set_travel_cost(region, travel_cost);
}

real_t NavigationRegion3D::get_travel_cost() const { return travel_cost; }

RID NavigationRegion3D::get_region_rid() const { return get_rid(); }

void NavigationRegion3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_region_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_region_update_transform();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_region_exit_navigation_map();
	} break;
	}
}

Ref<NavigationMesh> NavigationRegion3D::get_navigation_mesh() const { return navigation_mesh; }

void NavigationRegion3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::region_set_map(region, map_override);
}

RID NavigationRegion3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

PackedStringArray NavigationRegion3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (is_visible_in_tree() && is_inside_tree()) {
		if (navigation_mesh.is_null()) {
			warnings.push_back(
				RTR("A NavigationMesh resource must be set or created for this node to work."));
		}
	}

	return warnings;
}

#ifdef DEBUG_ENABLED
void NavigationRegion3D::_navigation_map_changed(RID p_map)
{
	if (is_inside_tree() && p_map == get_world_3d()->get_navigation_map()) {
		_update_debug_edge_connections_mesh();
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion3D::_navigation_debug_changed()
{
	if (is_inside_tree()) {
		_update_debug_mesh();
		_update_debug_edge_connections_mesh();
	}
}
#endif // DEBUG_ENABLED

void NavigationRegion3D::_region_enter_navigation_map()
{
	if (!is_inside_tree()) {
		return;
	}

	if (map_override.is_valid()) {
		NavigationServer3D::region_set_map(region, map_override);
	}
	else {
		NavigationServer3D::region_set_map(
			region, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::region_set_transform(region, get_global_transform());
	NavigationServer3D::region_set_enabled(region, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_exit_navigation_map()
{
	NavigationServer3D::region_set_map(region, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_visible(debug_instance, false);
	}
	if (debug_edge_connections_instance.is_valid()) {
		Renderer::instance_set_visible(debug_edge_connections_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::region_set_transform(region, get_global_transform());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_transform(debug_instance, get_global_transform());
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_update_bounds()
{
	if (navigation_mesh.is_null()) {
		bounds = AABB();
		return;
	}

	const Vector<Vector3>& vertices = navigation_mesh->get_vertices();
	if (vertices.is_empty()) {
		bounds = AABB();
		return;
	}

	const Transform3D gt = is_inside_tree() ? get_global_transform() : get_transform();

	AABB new_bounds;
	new_bounds.position = gt.xform(vertices[0]);

	for (const Vector3& vertex : vertices) {
		new_bounds.expand_to(gt.xform(vertex));
	}
	bounds = new_bounds;
}



#ifdef DEBUG_ENABLED
void NavigationRegion3D::_update_debug_mesh() {}

void NavigationRegion3D::_update_debug_edge_connections_mesh() {}
#endif // DEBUG_ENABLED

// ============================================================================
// Merged from scene/resources/3d/navigation_mesh_source_geometry_data_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"

void NavigationMeshSourceGeometryData3D::set_vertices(const Vector<float>& p_vertices)
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices = p_vertices;
	bounds_dirty = true;
}

const Vector<float>& NavigationMeshSourceGeometryData3D::get_vertices() const
{
	RWLockRead read_lock(geometry_rwlock);
	return vertices;
}

void NavigationMeshSourceGeometryData3D::set_indices(const Vector<int>& p_indices)
{
	ERR_FAIL_COND(vertices.size() < p_indices.size());
	RWLockWrite write_lock(geometry_rwlock);
	indices = p_indices;
	bounds_dirty = true;
}

const Vector<int>& NavigationMeshSourceGeometryData3D::get_indices() const
{
	RWLockRead read_lock(geometry_rwlock);
	return indices;
}

void NavigationMeshSourceGeometryData3D::append_arrays(
	const Vector<float>& p_vertices, const Vector<int>& p_indices)
{
	RWLockWrite write_lock(geometry_rwlock);

	const int64_t number_of_vertices_before_merge = vertices.size();
	const int64_t number_of_indices_before_merge = indices.size();

	vertices.append_array(p_vertices);
	indices.append_array(p_indices);

	for (int64_t i = number_of_indices_before_merge; i < indices.size(); i++) {
		indices.set(i, indices[i] + number_of_vertices_before_merge / 3);
	}
	bounds_dirty = true;
}

bool NavigationMeshSourceGeometryData3D::has_data()
{
	RWLockRead read_lock(geometry_rwlock);
	return vertices.size() && indices.size();
}

void NavigationMeshSourceGeometryData3D::clear()
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices.clear();
	indices.clear();
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::clear_projected_obstructions()
{
	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::_add_vertex(const Vector3& p_vec3)
{
	vertices.push_back(p_vec3.x);
	vertices.push_back(p_vec3.y);
	vertices.push_back(p_vec3.z);
}

void NavigationMeshSourceGeometryData3D::_add_faces(
	const PackedVector3Array& p_faces, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_faces.is_empty());
	ERR_FAIL_COND(p_faces.size() % 3 != 0);
	int face_count = p_faces.size() / 3;
	int current_vertex_count = vertices.size() / 3;

	for (int j = 0; j < face_count; j++) {
		_add_vertex(p_xform.xform(p_faces[j * 3 + 0]));
		_add_vertex(p_xform.xform(p_faces[j * 3 + 1]));
		_add_vertex(p_xform.xform(p_faces[j * 3 + 2]));

		indices.push_back(current_vertex_count + (j * 3 + 0));
		indices.push_back(current_vertex_count + (j * 3 + 2));
		indices.push_back(current_vertex_count + (j * 3 + 1));
	}
}

void NavigationMeshSourceGeometryData3D::add_mesh(
	const Ref<Mesh>& p_mesh, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_mesh.is_null());

#ifdef DEBUG_ENABLED
	if (!Engine::is_editor_hint()) {
		WARN_PRINT_ONCE(
			"Source geometry parsing for navigation mesh baking had to parse Renderer meshes at runtime.\n\
		This poses a significant performance issues as visual meshes store geometry data on the GPU and transferring this data back to the CPU blocks the rendering.\n\
		For runtime (re)baking navigation meshes use and parse collision shapes as source geometry or create geometry data procedurally in scripts.");
	}
#endif

	_add_mesh(p_mesh, root_node_transform * p_xform);
}

void NavigationMeshSourceGeometryData3D::add_faces(
	const PackedVector3Array& p_faces, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_faces.size() % 3 != 0);
	RWLockWrite write_lock(geometry_rwlock);
	_add_faces(p_faces, root_node_transform * p_xform);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::merge(
	const Ref<NavigationMeshSourceGeometryData3D>& p_other_geometry)
{
	ERR_FAIL_COND(p_other_geometry.is_null());

	Vector<float> other_vertices;
	Vector<int> other_indices;
	Vector<ProjectedObstruction> other_projected_obstructions;

	p_other_geometry->get_data(other_vertices, other_indices, other_projected_obstructions);

	RWLockWrite write_lock(geometry_rwlock);
	const int64_t number_of_vertices_before_merge = vertices.size();
	const int64_t number_of_indices_before_merge = indices.size();

	vertices.append_array(other_vertices);
	indices.append_array(other_indices);

	for (int64_t i = number_of_indices_before_merge; i < indices.size(); i++) {
		indices.set(i, indices[i] + number_of_vertices_before_merge / 3);
	}

	_projected_obstructions.append_array(other_projected_obstructions);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::add_projected_obstruction(
	const Vector<Vector3>& p_vertices, float p_elevation, float p_height, bool p_carve)
{
	ERR_FAIL_COND(p_vertices.size() < 3);
	ERR_FAIL_COND(p_height < 0.0);

	ProjectedObstruction projected_obstruction;
	projected_obstruction.vertices.resize(p_vertices.size() * 3);
	projected_obstruction.elevation = p_elevation;
	projected_obstruction.height = p_height;
	projected_obstruction.carve = p_carve;

	float* obstruction_vertices_ptrw = projected_obstruction.vertices.ptrw();

	int vertex_index = 0;
	for (const Vector3& vertex : p_vertices) {
		obstruction_vertices_ptrw[vertex_index++] = vertex.x;
		obstruction_vertices_ptrw[vertex_index++] = vertex.y;
		obstruction_vertices_ptrw[vertex_index++] = vertex.z;
	}

	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.push_back(projected_obstruction);
	bounds_dirty = true;
}

Vector<NavigationMeshSourceGeometryData3D::ProjectedObstruction>
NavigationMeshSourceGeometryData3D::_get_projected_obstructions() const
{
	RWLockRead read_lock(geometry_rwlock);
	return _projected_obstructions;
}

void NavigationMeshSourceGeometryData3D::set_data(const Vector<float>& p_vertices,
	const Vector<int>& p_indices, Vector<ProjectedObstruction>& p_projected_obstructions)
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices = p_vertices;
	indices = p_indices;
	_projected_obstructions = p_projected_obstructions;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::get_data(Vector<float>& r_vertices, Vector<int>& r_indices,
	Vector<ProjectedObstruction>& r_projected_obstructions)
{
	RWLockRead read_lock(geometry_rwlock);
	r_vertices = vertices;
	r_indices = indices;
	r_projected_obstructions = _projected_obstructions;
}

AABB NavigationMeshSourceGeometryData3D::get_bounds()
{
	geometry_rwlock.read_lock();

	if (bounds_dirty) {
		geometry_rwlock.read_unlock();
		RWLockWrite write_lock(geometry_rwlock);

		bounds_dirty = false;
		bounds = AABB();
		bool first_vertex = true;

		for (int i = 0; i < vertices.size() / 3; i++) {
			const Vector3 vertex =
				Vector3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
			if (first_vertex) {
				first_vertex = false;
				bounds.position = vertex;
			}
			else {
				bounds.expand_to(vertex);
			}
		}
		for (const ProjectedObstruction& projected_obstruction : _projected_obstructions) {
			for (int i = 0; i < projected_obstruction.vertices.size() / 3; i++) {
				const Vector3 vertex = Vector3(projected_obstruction.vertices[i * 3],
					projected_obstruction.vertices[i * 3 + 1],
					projected_obstruction.vertices[i * 3 + 2]);
				if (first_vertex) {
					first_vertex = false;
					bounds.position = vertex;
				}
				else {
					bounds.expand_to(vertex);
				}
			}
		}
	}
	else {
		geometry_rwlock.read_unlock();
	}

	RWLockRead read_lock(geometry_rwlock);
	return bounds;
}



void NavigationMeshSourceGeometryData3D::_add_mesh(Ref<Mesh> const&, Transform3D const&) {}
