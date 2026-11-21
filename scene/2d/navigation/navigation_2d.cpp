/**************************************************************************/
/*  navigation_2d.cpp                                                     */
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

#include "navigation_2d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "core/math/random_pcg.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "scene/resources/world_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/rendering/renderer.h"

void NavigationAgent2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are
		// ready. cannot use READY as ready does not get called if Node is re-added to SceneTree
		set_agent_parent(get_parent());
		set_physics_process_internal(true);

		if (agent_parent && avoidance_enabled) {
			NavigationServer2D::agent_set_position(
				agent, agent_parent->get_global_position());
		}

#ifdef DEBUG_ENABLED
		if (NavigationServer2D::get_debug_enabled()) {
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
			Renderer::canvas_item_set_visible(debug_path_instance, false);
		}
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (agent_parent) {
			NavigationServer2D::agent_set_paused(
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
			NavigationServer2D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (agent_parent && avoidance_enabled) {
			NavigationServer2D::agent_set_position(
				agent, agent_parent->get_global_position());
		}
		if (agent_parent && target_position_submitted) {
			if (velocity_submitted) {
				velocity_submitted = false;
				if (avoidance_enabled) {
					NavigationServer2D::agent_set_velocity(agent, velocity);
				}
			}
			if (velocity_forced_submitted) {
				velocity_forced_submitted = false;
				if (avoidance_enabled) {
					NavigationServer2D::agent_set_velocity_forced(
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

bool NavigationAgent2D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationAgent2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	if (target_position_submitted) {
		_request_repath();
	}
}

uint32_t NavigationAgent2D::get_navigation_layers() const { return navigation_layers; }

void NavigationAgent2D::set_navigation_layer_value(int p_layer_number, bool p_value)
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

bool NavigationAgent2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");
	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent2D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	if (pathfinding_algorithm == p_pathfinding_algorithm) {
		return;
	}

	pathfinding_algorithm = p_pathfinding_algorithm;

	navigation_query->set_pathfinding_algorithm(pathfinding_algorithm);
}

void NavigationAgent2D::set_path_postprocessing(
	const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing)
{
	if (path_postprocessing == p_path_postprocessing) {
		return;
	}

	path_postprocessing = p_path_postprocessing;

	navigation_query->set_path_postprocessing(path_postprocessing);
}

void NavigationAgent2D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
	navigation_query->set_simplify_path(simplify_path);
}

bool NavigationAgent2D::get_simplify_path() const { return simplify_path; }

void NavigationAgent2D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
	navigation_query->set_simplify_epsilon(simplify_epsilon);
}

real_t NavigationAgent2D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationAgent2D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
	navigation_query->set_path_return_max_length(path_return_max_length);
}

float NavigationAgent2D::get_path_return_max_length() const { return path_return_max_length; }

void NavigationAgent2D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
	navigation_query->set_path_return_max_radius(path_return_max_radius);
}

float NavigationAgent2D::get_path_return_max_radius() const { return path_return_max_radius; }

void NavigationAgent2D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
	navigation_query->set_path_search_max_polygons(path_search_max_polygons);
}

int NavigationAgent2D::get_path_search_max_polygons() const { return path_search_max_polygons; }

void NavigationAgent2D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
	navigation_query->set_path_search_max_distance(path_search_max_distance);
}

float NavigationAgent2D::get_path_search_max_distance() const { return path_search_max_distance; }

float NavigationAgent2D::get_path_length() const { return navigation_result->get_path_length(); }

void NavigationAgent2D::set_path_metadata_flags(uint32_t p_path_metadata_flags)
{
	if (path_metadata_flags == p_path_metadata_flags) {
		return;
	}

	path_metadata_flags = p_path_metadata_flags;
}

void NavigationAgent2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::agent_set_map(agent, map_override);
	if (target_position_submitted) {
		_request_repath();
	}
}

RID NavigationAgent2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (agent_parent != nullptr) {
		return agent_parent->get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationAgent2D::set_path_desired_distance(real_t p_path_desired_distance)
{
	if (Math::is_equal_approx(path_desired_distance, p_path_desired_distance)) {
		return;
	}

	path_desired_distance = p_path_desired_distance;
}

void NavigationAgent2D::set_target_desired_distance(real_t p_target_desired_distance)
{
	if (Math::is_equal_approx(target_desired_distance, p_target_desired_distance)) {
		return;
	}

	target_desired_distance = p_target_desired_distance;
}

void NavigationAgent2D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;

	NavigationServer2D::agent_set_radius(agent, radius);
}

void NavigationAgent2D::set_neighbor_distance(real_t p_distance)
{
	if (Math::is_equal_approx(neighbor_distance, p_distance)) {
		return;
	}

	neighbor_distance = p_distance;

	NavigationServer2D::agent_set_neighbor_distance(agent, neighbor_distance);
}

void NavigationAgent2D::set_max_neighbors(int p_count)
{
	if (max_neighbors == p_count) {
		return;
	}

	max_neighbors = p_count;

	NavigationServer2D::agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent2D::set_time_horizon_agents(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_agents, p_time_horizon)) {
		return;
	}
	time_horizon_agents = p_time_horizon;
	NavigationServer2D::agent_set_time_horizon_agents(agent, time_horizon_agents);
}

void NavigationAgent2D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_obstacles, p_time_horizon)) {
		return;
	}
	time_horizon_obstacles = p_time_horizon;
	NavigationServer2D::agent_set_time_horizon_obstacles(
		agent, time_horizon_obstacles);
}

void NavigationAgent2D::set_max_speed(real_t p_max_speed)
{
	ERR_FAIL_COND_MSG(p_max_speed < 0.0, "Max speed must be positive.");
	if (Math::is_equal_approx(max_speed, p_max_speed)) {
		return;
	}
	max_speed = p_max_speed;

	NavigationServer2D::agent_set_max_speed(agent, max_speed);
}

void NavigationAgent2D::set_path_max_distance(real_t p_path_max_distance)
{
	if (Math::is_equal_approx(path_max_distance, p_path_max_distance)) {
		return;
	}

	path_max_distance = p_path_max_distance;
}

real_t NavigationAgent2D::get_path_max_distance() { return path_max_distance; }

void NavigationAgent2D::set_target_position(Vector2 p_position)
{
	// Intentionally not checking for equality of the parameter, as we want to update the path even
	// if the target position is the same in case the world changed. Revisit later when the
	// navigation server can update the path without requesting a new path.

	target_position = p_position;
	target_position_submitted = true;

	_request_repath();
}

Vector2 NavigationAgent2D::get_target_position() const { return target_position; }

Vector2 NavigationAgent2D::get_next_path_position()
{
	_update_navigation();

	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		ERR_FAIL_NULL_V_MSG(agent_parent, Vector2(), "The agent has no parent.");
		return agent_parent->get_global_position();
	}
	else {
		return navigation_path[navigation_path_index];
	}
}

real_t NavigationAgent2D::distance_to_target() const
{
	ERR_FAIL_NULL_V_MSG(agent_parent, 0.0, "The agent has no parent.");
	return agent_parent->get_global_position().distance_to(target_position);
}

bool NavigationAgent2D::is_target_reached() const { return target_reached; }

bool NavigationAgent2D::is_target_reachable()
{
	_update_navigation();
	return _is_target_reachable();
}

bool NavigationAgent2D::_is_target_reachable() const
{
	return target_desired_distance >= _get_final_position().distance_to(target_position);
}

bool NavigationAgent2D::is_navigation_finished()
{
	_update_navigation();
	return navigation_finished;
}

Vector2 NavigationAgent2D::get_final_position()
{
	_update_navigation();
	return _get_final_position();
}

Vector2 NavigationAgent2D::_get_final_position() const
{
	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		return Vector2();
	}
	return navigation_path[navigation_path.size() - 1];
}

void NavigationAgent2D::set_velocity_forced(Vector2 p_velocity)
{
	// Intentionally not checking for equality of the parameter.
	// We need to always submit the velocity to the navigation server, even when it is the same, in
	// order to run avoidance every frame. Revisit later when the navigation server can update
	// avoidance without users resubmitting the velocity.

	velocity_forced = p_velocity;
	velocity_forced_submitted = true;
}

void NavigationAgent2D::set_velocity(const Vector2 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationAgent2D::_advance_waypoints(const Vector2& p_origin)
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

void NavigationAgent2D::_request_repath()
{
	navigation_result->reset();
	target_reached = false;
	navigation_finished = false;
	last_waypoint_reached = false;
}

bool NavigationAgent2D::_is_last_waypoint() const
{
	return navigation_path_index == navigation_result->get_path().size() - 1;
}

void NavigationAgent2D::_move_to_next_waypoint() { navigation_path_index += 1; }

bool NavigationAgent2D::_is_within_waypoint_distance(const Vector2& p_origin) const
{
	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	return p_origin.distance_to(navigation_path[navigation_path_index]) < path_desired_distance;
}

bool NavigationAgent2D::_is_within_target_distance(const Vector2& p_origin) const
{
	return p_origin.distance_to(target_position) < target_desired_distance;
}

void NavigationAgent2D::_transition_to_navigation_finished()
{
	navigation_finished = true;
	target_position_submitted = false;

	if (avoidance_enabled) {
		NavigationServer2D::agent_set_position(
			agent, agent_parent->get_global_position());
		NavigationServer2D::agent_set_velocity(agent, Vector2(0.0, 0.0));
		NavigationServer2D::agent_set_velocity_forced(agent, Vector2(0.0, 0.0));
	}
}

void NavigationAgent2D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer2D::agent_set_avoidance_layers(get_rid(), avoidance_layers);
}

uint32_t NavigationAgent2D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationAgent2D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	NavigationServer2D::agent_set_avoidance_mask(get_rid(), p_mask);
}

uint32_t NavigationAgent2D::get_avoidance_mask() const { return avoidance_mask; }

void NavigationAgent2D::set_avoidance_layer_value(int p_layer_number, bool p_value)
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

bool NavigationAgent2D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent2D::set_avoidance_mask_value(int p_mask_number, bool p_value)
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

bool NavigationAgent2D::get_avoidance_mask_value(int p_mask_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_mask_number < 1, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_mask_number > 32, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	return get_avoidance_mask() & (1 << (p_mask_number - 1));
}

void NavigationAgent2D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	NavigationServer2D::agent_set_avoidance_priority(get_rid(), p_priority);
}

real_t NavigationAgent2D::get_avoidance_priority() const { return avoidance_priority; }

////////DEBUG////////////////////////////////////////////////////////////

void NavigationAgent2D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled == p_enabled) {
		return;
	}

	debug_enabled = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent2D::get_debug_enabled() const { return debug_enabled; }

void NavigationAgent2D::set_debug_use_custom(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_use_custom == p_enabled) {
		return;
	}

	debug_use_custom = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent2D::get_debug_use_custom() const { return debug_use_custom; }

void NavigationAgent2D::set_debug_path_custom_color(Color p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_path_custom_color == p_color) {
		return;
	}

	debug_path_custom_color = p_color;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

Color NavigationAgent2D::get_debug_path_custom_color() const { return debug_path_custom_color; }

void NavigationAgent2D::set_debug_path_custom_point_size(float p_point_size)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_point_size, p_point_size)) {
		return;
	}

	debug_path_custom_point_size = MAX(0.0, p_point_size);
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent2D::get_debug_path_custom_point_size() const
{
	return debug_path_custom_point_size;
}

void NavigationAgent2D::set_debug_path_custom_line_width(float p_line_width)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_line_width, p_line_width)) {
		return;
	}

	debug_path_custom_line_width = p_line_width;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent2D::get_debug_path_custom_line_width() const
{
	return debug_path_custom_line_width;
}

#ifdef DEBUG_ENABLED
void NavigationAgent2D::_navigation_debug_changed() { debug_path_dirty = true; }

void NavigationAgent2D::_update_debug_path()
{
	if (!debug_path_dirty) {
		return;
	}
	debug_path_dirty = false;

	if (!debug_path_instance.is_valid()) {
		debug_path_instance = Renderer::canvas_item_create();
	}

	Renderer::canvas_item_clear(debug_path_instance);

	if (!(debug_enabled &&
			NavigationServer2D::get_debug_navigation_enable_agent_paths())) {
		return;
	}

	if (!(agent_parent && agent_parent->is_inside_tree())) {
		return;
	}

	Renderer::canvas_item_set_parent(
		debug_path_instance, agent_parent->get_canvas());
	Renderer::canvas_item_set_z_index(
		debug_path_instance, RSE::CANVAS_ITEM_Z_MAX - 1);
	Renderer::canvas_item_set_visible(
		debug_path_instance, agent_parent->is_visible_in_tree());

	const Vector<Vector2>& navigation_path = navigation_result->get_path();

	if (navigation_path.size() <= 1) {
		return;
	}

	Color debug_path_color =
		NavigationServer2D::get_debug_navigation_agent_path_color();
	if (debug_use_custom) {
		debug_path_color = debug_path_custom_color;
	}

	Vector<Color> debug_path_colors;
	debug_path_colors.resize(navigation_path.size());
	debug_path_colors.fill(debug_path_color);

	Renderer::canvas_item_add_polyline(debug_path_instance, navigation_path,
		debug_path_colors, debug_path_custom_line_width, false);

	if (debug_path_custom_point_size <= 0.0) {
		return;
	}

	float point_size =
		NavigationServer2D::get_debug_navigation_agent_path_point_size();
	float half_point_size = point_size * 0.5;

	if (debug_use_custom) {
		point_size = debug_path_custom_point_size;
		half_point_size = debug_path_custom_point_size * 0.5;
	}

	for (int i = 0; i < navigation_path.size(); i++) {
		const Vector2& vert = navigation_path[i];
		Rect2 path_point_rect =
			Rect2(vert.x - half_point_size, vert.y - half_point_size, point_size, point_size);
		Renderer::canvas_item_add_rect(
			debug_path_instance, path_point_rect, debug_path_color);
	}
}
#endif // DEBUG_ENABLED



void NavigationAgent2D::_update_navigation() {}

void NavigationAgent2D::set_agent_parent(Node*) {}

void NavigationAgent2D::_trigger_waypoint_reached() {}


void NavigationLink2D::_notification(int p_what)
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
	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		_update_debug_mesh();
#endif // DEBUG_ENABLED
	} break;
	}
}

#ifdef DEBUG_ENABLED
Rect2 NavigationLink2D::_edit_get_rect() const
{
	if (!is_inside_tree()) {
		return Rect2();
	}

	real_t radius = NavigationServer2D::map_get_link_connection_radius(
		get_world_2d()->get_navigation_map());

	Rect2 rect(get_start_position(), Size2());
	rect.expand_to(get_end_position());
	rect.grow_by(radius);
	return rect;
}

bool NavigationLink2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	Vector2 closest_point =
		Geometry2D::get_closest_point_to_segment(p_point, get_start_position(), get_end_position());
	return p_point.distance_to(closest_point) < p_tolerance;
}
#endif // DEBUG_ENABLED

RID NavigationLink2D::get_rid() const { return link; }

void NavigationLink2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::link_set_map(link, map_override);
}

RID NavigationLink2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationLink2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer2D::link_set_navigation_layers(link, navigation_layers);
}

void NavigationLink2D::set_navigation_layer_value(int p_layer_number, bool p_value)
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

bool NavigationLink2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationLink2D::set_global_start_position(Vector2 p_position)
{
	if (is_inside_tree()) {
		set_start_position(to_local(p_position));
	}
	else {
		set_start_position(p_position);
	}
}

Vector2 NavigationLink2D::get_global_start_position() const
{
	if (is_inside_tree()) {
		return to_global(start_position);
	}
	else {
		return start_position;
	}
}

void NavigationLink2D::set_global_end_position(Vector2 p_position)
{
	if (is_inside_tree()) {
		set_end_position(to_local(p_position));
	}
	else {
		set_end_position(p_position);
	}
}

Vector2 NavigationLink2D::get_global_end_position() const
{
	if (is_inside_tree()) {
		return to_global(end_position);
	}
	else {
		return end_position;
	}
}

void NavigationLink2D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer2D::link_set_enter_cost(link, enter_cost);
}

void NavigationLink2D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer2D::link_set_travel_cost(link, travel_cost);
}

PackedStringArray NavigationLink2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (start_position.is_equal_approx(end_position)) {
		warnings.push_back(RTR("NavigationLink2D start position should be different than the end "
							   "position to be useful."));
	}

	return warnings;
}

void NavigationLink2D::_link_exit_navigation_map()
{
	NavigationServer2D::link_set_map(link, RID());
}

#ifdef DEBUG_ENABLED
void NavigationLink2D::_update_debug_mesh()
{
	if (!is_inside_tree()) {
		return;
	}

	if (!Engine::is_editor_hint() &&
		!NavigationServer2D::get_debug_enabled()) {
		return;
	}

	Color color;
	if (enabled) {
		color = NavigationServer2D::get_debug_navigation_link_connection_color();
	}
	else {
		color = NavigationServer2D::get_debug_navigation_link_connection_disabled_color();
	}

	real_t radius = NavigationServer2D::map_get_link_connection_radius(
		get_world_2d()->get_navigation_map());

	draw_line(get_start_position(), get_end_position(), color);
	draw_arc(get_start_position(), radius, 0, Math::TAU, 10, color);
	draw_arc(get_end_position(), radius, 0, Math::TAU, 10, color);

	const Vector2 link_segment = end_position - start_position;
	const float arror_len = 5.0;

	{
		Vector2 anchor = start_position + (link_segment * 0.75);
		Vector2 direction = start_position.direction_to(end_position);
		Vector2 arrow_dir = -direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);

		arrow_dir = direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);
	}

	if (is_bidirectional()) {
		Vector2 anchor = start_position + (link_segment * 0.25);
		Vector2 direction = end_position.direction_to(start_position);
		Vector2 arrow_dir = -direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);

		arrow_dir = direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);
	}
}
#endif // DEBUG_ENABLED

NavigationLink2D::NavigationLink2D()
{
	link = NavigationServer2D::link_create();

	NavigationServer2D::link_set_enter_cost(link, enter_cost);
	NavigationServer2D::link_set_travel_cost(link, travel_cost);
	NavigationServer2D::link_set_navigation_layers(link, navigation_layers);
	NavigationServer2D::link_set_bidirectional(link, bidirectional);
	NavigationServer2D::link_set_enabled(link, enabled);

	set_notify_transform(true);
	set_hide_clip_children(true);
}

NavigationLink2D::~NavigationLink2D()
{
	ERR_FAIL_NULL(NavigationServer2D::data);
	NavigationServer2D::free_rid(link);
	link = RID();
}

void NavigationLink2D::set_start_position(Vector2) {}

void NavigationLink2D::set_end_position(Vector2) {}

void NavigationLink2D::_link_update_transform() {}

void NavigationLink2D::_link_enter_navigation_map() {}


RID NavigationObstacle2D::_navmesh_source_geometry_parser;

void NavigationObstacle2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (map_override.is_valid()) {
			_update_map(map_override);
		}
		else if (is_inside_tree()) {
			_update_map(get_world_2d()->get_navigation_map());
		}
		else {
			_update_map(RID());
		}
		previous_transform = get_global_transform();
		// need to trigger map controlled agent assignment somehow for the fake_agent since
		// obstacles use no callback like regular agents
		NavigationServer2D::obstacle_set_avoidance_enabled(
			obstacle, avoidance_enabled);
		_update_transform();
		set_physics_process_internal(true);
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_parent(
			debug_canvas_item, get_world_2d()->get_canvas());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_physics_process_internal(false);
		_update_map(RID());
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_parent(debug_canvas_item, RID());
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
		NavigationServer2D::obstacle_set_paused(obstacle, !can_process());
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
		NavigationServer2D::obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_visible(debug_canvas_item, is_visible_in_tree());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_inside_tree()) {
			_update_transform();

			if (velocity_submitted) {
				velocity_submitted = false;
				// only update if there is a noticeable change, else the rvo agent preferred
				// velocity stays the same
				if (!previous_velocity.is_equal_approx(velocity)) {
					NavigationServer2D::obstacle_set_velocity(obstacle, velocity);
				}
				previous_velocity = velocity;
			}
		}
	} break;

	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		if (is_inside_tree()) {
			bool is_debug_enabled = false;
			if (Engine::is_editor_hint()) {
				is_debug_enabled = true;
			}
			else if (NavigationServer2D::get_debug_enabled() &&
					   NavigationServer2D::get_debug_avoidance_enabled()) {
				is_debug_enabled = true;
			}

			if (is_debug_enabled) {
				Renderer::canvas_item_clear(debug_canvas_item);
				Renderer::canvas_item_set_transform(debug_canvas_item, Transform2D());
				_update_fake_agent_radius_debug();
				_update_static_obstacle_debug();
			}
		}
#endif // DEBUG_ENABLED
	} break;
	}
}

NavigationObstacle2D::NavigationObstacle2D()
{
	obstacle = NavigationServer2D::obstacle_create();

	NavigationServer2D::obstacle_set_radius(obstacle, radius);
	NavigationServer2D::obstacle_set_vertices(obstacle, vertices);
	NavigationServer2D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
	NavigationServer2D::obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);

#ifdef DEBUG_ENABLED
	debug_canvas_item = Renderer::canvas_item_create();
	debug_mesh_rid = Renderer::mesh_create();
#endif // DEBUG_ENABLED
}

NavigationObstacle2D::~NavigationObstacle2D()
{
	ERR_FAIL_NULL(NavigationServer2D::data);

	NavigationServer2D::free_rid(obstacle);
	obstacle = RID();

#ifdef DEBUG_ENABLED
	if (debug_mesh_rid.is_valid()) {
		Renderer::free_rid(debug_mesh_rid);
		debug_mesh_rid = RID();
	}
	if (debug_canvas_item.is_valid()) {
		Renderer::free_rid(debug_canvas_item);
		debug_canvas_item = RID();
	}
#endif // DEBUG_ENABLED
}

void NavigationObstacle2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}
	map_override = p_navigation_map;
	_update_map(map_override);
}

RID NavigationObstacle2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationObstacle2D::set_avoidance_layers(uint32_t p_layers)
{
	if (avoidance_layers == p_layers) {
		return;
	}
	avoidance_layers = p_layers;
	NavigationServer2D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
}

uint32_t NavigationObstacle2D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationObstacle2D::set_avoidance_layer_value(int p_layer_number, bool p_value)
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

bool NavigationObstacle2D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

bool NavigationObstacle2D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationObstacle2D::set_velocity(const Vector2 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationObstacle2D::set_affect_navigation_mesh(bool p_enabled)
{
	affect_navigation_mesh = p_enabled;
}

bool NavigationObstacle2D::get_affect_navigation_mesh() const { return affect_navigation_mesh; }

void NavigationObstacle2D::set_carve_navigation_mesh(bool p_enabled)
{
	carve_navigation_mesh = p_enabled;
}

bool NavigationObstacle2D::get_carve_navigation_mesh() const { return carve_navigation_mesh; }

PackedStringArray NavigationObstacle2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	const Vector2 global_scale = get_global_scale();
	if (global_scale.x < 0.001 || global_scale.y < 0.001) {
		warnings.push_back(RTR("NavigationObstacle2D does not support negative or zero scaling."));
	}

	if (radius > 0.0 && !get_global_transform().is_conformal()) {
		warnings.push_back(
			RTR("The agent radius can only be scaled uniformly. The largest value along the two "
				"axes of the global scale will be used to scale the radius. This value may change "
				"in unexpected ways when the node is rotated."));
	}

	if (radius > 0.0 && get_global_skew() != 0.0) {
		warnings.push_back(RTR("Skew has no effect on the agent radius."));
	}

	return warnings;
}

void NavigationObstacle2D::_update_map(RID p_map)
{
	map_current = p_map;
	NavigationServer2D::obstacle_set_map(obstacle, p_map);
}

#ifdef DEBUG_ENABLED
void NavigationObstacle2D::_update_fake_agent_radius_debug()
{
	if (radius > 0.0 && NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_radius()) {
		Color debug_radius_color = NavigationServer2D::get_debug_navigation_avoidance_obstacles_radius_color();
		// Prevent non-positive scaling.
		const Vector2 safe_scale = get_global_scale().abs().maxf(0.001);
		// Agent radius is a scalar value and does not support non-uniform scaling, choose the
		// largest axis.
		const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
		Renderer::canvas_item_add_circle(debug_canvas_item, get_global_position(),
			scaling_max_value * radius, debug_radius_color);
	}
}
#endif // DEBUG_ENABLED



void NavigationObstacle2D::_update_transform() {}

#ifdef DEBUG_ENABLED
void NavigationObstacle2D::_update_static_obstacle_debug() {}
#endif // DEBUG_ENABLED


RID NavigationRegion2D::get_rid() const { return region; }

bool NavigationRegion2D::is_enabled() const { return enabled; }

void NavigationRegion2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer2D::region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion2D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer2D::region_set_navigation_layers(region, navigation_layers);
}

uint32_t NavigationRegion2D::get_navigation_layers() const { return navigation_layers; }

void NavigationRegion2D::set_navigation_layer_value(int p_layer_number, bool p_value)
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

bool NavigationRegion2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationRegion2D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer2D::region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion2D::get_enter_cost() const { return enter_cost; }

void NavigationRegion2D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer2D::region_set_travel_cost(region, travel_cost);
}

real_t NavigationRegion2D::get_travel_cost() const { return travel_cost; }

RID NavigationRegion2D::get_region_rid() const { return get_rid(); }

#ifdef DEBUG_ENABLED
Rect2 NavigationRegion2D::_edit_get_rect() const
{
	return navigation_polygon.is_valid() ? navigation_polygon->_edit_get_rect() : Rect2();
}

bool NavigationRegion2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return navigation_polygon.is_valid()
			   ? navigation_polygon->_edit_is_selected_on_click(p_point, p_tolerance)
			   : false;
}
#endif // DEBUG_ENABLED

void NavigationRegion2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_region_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_region_update_transform();
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
#ifdef DEBUG_ENABLED
		_set_debug_visible(is_visible_in_tree());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_region_exit_navigation_map();
#ifdef DEBUG_ENABLED
		_set_debug_visible(false);
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		if (is_inside_tree() &&
			(Engine::is_editor_hint() ||
				(NavigationServer2D::get_debug_enabled() &&
					NavigationServer2D::get_debug_navigation_enabled())) &&
			navigation_polygon.is_valid()) {
			_update_debug_mesh();
			_update_debug_edge_connections_mesh();
			_update_debug_baking_rect();
		}
#endif // DEBUG_ENABLED
	} break;
	}
}

Ref<NavigationPolygon> NavigationRegion2D::get_navigation_polygon() const
{
	return navigation_polygon;
}

void NavigationRegion2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::region_set_map(region, map_override);
}

RID NavigationRegion2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

PackedStringArray NavigationRegion2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (is_visible_in_tree() && is_inside_tree()) {
		if (navigation_polygon.is_null()) {
			warnings.push_back(RTR("A NavigationPolygon resource must be set or created for this "
								   "node to work. Please set a property or draw a polygon."));
		}
	}

	return warnings;
}

void NavigationRegion2D::_region_exit_navigation_map()
{
	NavigationServer2D::region_set_map(region, RID());
}

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_edge_connections_mesh()
{
	bool enable_edge_connections =
		use_edge_connections && NavigationServer2D::get_debug_navigation_enable_edge_connections() &&
		NavigationServer2D::map_get_use_edge_connections(get_world_2d()->get_navigation_map());

	if (enable_edge_connections) {
		Color debug_edge_connection_color = NavigationServer2D::get_debug_navigation_edge_connection_color();
		// Draw the region edge connections.
		Transform2D xform = get_global_transform();
		real_t radius =
			NavigationServer2D::map_get_edge_connection_margin(get_world_2d()->get_navigation_map()) / 2.0;
		for (int i = 0; i < NavigationServer2D::region_get_connections_count(region); i++) {
			// Two main points
			Vector2 a = NavigationServer2D::region_get_connection_pathway_start(region, i);
			a = xform.affine_inverse().xform(a);
			Vector2 b = NavigationServer2D::region_get_connection_pathway_end(region, i);
			b = xform.affine_inverse().xform(b);
			draw_line(a, b, debug_edge_connection_color);

			// Draw a circle to illustrate the margins.
			real_t angle = a.angle_to_point(b);
			draw_arc(a, radius, angle + Math::PI / 2.0, angle - Math::PI / 2.0 + Math::TAU, 10,
				debug_edge_connection_color);
			draw_arc(b, radius, angle - Math::PI / 2.0, angle + Math::PI / 2.0, 10,
				debug_edge_connection_color);
		}
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_baking_rect()
{
	Rect2 baking_rect = get_navigation_polygon()->get_baking_rect();
	if (baking_rect.has_area()) {
		Vector2 baking_rect_offset = get_navigation_polygon()->get_baking_rect_offset();
		Rect2 debug_baking_rect = Rect2(baking_rect.position.x + baking_rect_offset.x,
			baking_rect.position.y + baking_rect_offset.y, baking_rect.size.x, baking_rect.size.y);
		Color debug_baking_rect_color = Color(0.8, 0.5, 0.7, 0.1);
		draw_rect(debug_baking_rect, debug_baking_rect_color);
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_set_debug_visible(bool p_visible)
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance_rid.is_valid()) {
		Renderer::canvas_item_set_visible(debug_instance_rid, p_visible);
	}
}
#endif // DEBUG_ENABLED

void NavigationRegion2D::_update_bounds()
{
	if (navigation_polygon.is_null()) {
		bounds = Rect2();
		return;
	}

	const Vector<Vector2>& vertices = navigation_polygon->get_vertices();
	if (vertices.is_empty()) {
		bounds = Rect2();
		return;
	}

	const Transform2D gt = is_inside_tree() ? get_global_transform() : get_transform();

	Rect2 new_bounds;
	new_bounds.position = gt.xform(vertices[0]);

	for (const Vector2& vertex : vertices) {
		new_bounds.expand_to(gt.xform(vertex));
	}
	bounds = new_bounds;
}



#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_mesh() {}
#endif // DEBUG_ENABLED

void NavigationRegion2D::set_navigation_polygon(Ref<NavigationPolygon> const&) {}

void NavigationRegion2D::_region_update_transform() {}

void NavigationRegion2D::_region_enter_navigation_map() {}

// ============================================================================
// Merged from scene/resources/2d/navigation_polygon.cpp
// ============================================================================

#include <thirdparty/misc/polypartition.h>
#include "core/math/geometry_2d.h"
#include "core/os/mutex.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"

NavigationPolygon::NavigationPolygon() :
		cell_size(NavigationDefaults2D::NAV_MESH_CELL_SIZE) {
}

#ifdef DEBUG_ENABLED
Rect2 NavigationPolygon::_edit_get_rect() const
{
	RWLockRead read_lock(rwlock);
	if (rect_cache_dirty) {
		item_rect = Rect2();
		bool first = true;

		for (int i = 0; i < outlines.size(); i++) {
			const Vector<Vector2>& outline = outlines[i];
			const int outline_size = outline.size();
			if (outline_size < 3) {
				continue;
			}
			const Vector2* p = outline.ptr();
			for (int j = 0; j < outline_size; j++) {
				if (first) {
					item_rect = Rect2(p[j], Vector2(0, 0));
					first = false;
				}
				else {
					item_rect.expand_to(p[j]);
				}
			}
		}

		rect_cache_dirty = false;
	}
	return item_rect;
}

#endif // DEBUG_ENABLED

void NavigationPolygon::set_vertices(const Vector<Vector2>& p_vertices)
{
	RWLockWrite write_lock(rwlock);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	vertices = p_vertices;
	rect_cache_dirty = true;
}

const Vector<Vector2>& NavigationPolygon::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

void NavigationPolygon::add_polygon(const Vector<int>& p_polygon)
{
	RWLockWrite write_lock(rwlock);
	polygons.push_back(p_polygon);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::add_outline_at_index(const Vector<Vector2>& p_outline, int p_index)
{
	RWLockWrite write_lock(rwlock);
	outlines.insert(p_index, p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationPolygon::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationPolygon::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(
	const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(const Vector<Vector2>& p_vertices,
	const Vector<Vector<int>>& p_polygons, const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	outlines = p_outlines;
	rect_cache_dirty = true;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons,
	Vector<Vector<Vector2>>& r_outlines)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
	r_outlines = outlines;
}

Ref<NavigationMesh> NavigationPolygon::get_navigation_mesh()
{
	MutexLock lock(navigation_mesh_generation);

	if (navigation_mesh.is_null()) {
		navigation_mesh.instantiate();
		Vector<Vector3> verts;
		Vector<Vector<int>> polys;
		{
			verts.resize(get_vertices().size());
			Vector3* w = verts.ptrw();

			const Vector2* r = get_vertices().ptr();

			for (int i(0); i < get_vertices().size(); i++) {
				w[i] = Vector3(r[i].x, 0.0, r[i].y);
			}
		}

		for (int i(0); i < get_polygon_count(); i++) {
			polys.push_back(get_polygon(i));
		}

		navigation_mesh->set_data(verts, polys);
		navigation_mesh->set_cell_size(
			cell_size); // Needed to not fail the cell size check on the server
	}

	return navigation_mesh;
}

void NavigationPolygon::set_outlines(const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	outlines = p_outlines;
	rect_cache_dirty = true;
}

Vector<Vector<Vector2>> NavigationPolygon::get_outlines() const
{
	RWLockRead read_lock(rwlock);
	return outlines;
}

void NavigationPolygon::set_polygons(const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

Vector<Vector<int>> NavigationPolygon::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

void NavigationPolygon::add_outline(const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	outlines.push_back(p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_outline_count() const
{
	RWLockRead read_lock(rwlock);
	return outlines.size();
}

void NavigationPolygon::set_outline(int p_idx, const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.write[p_idx] = p_outline;
	rect_cache_dirty = true;
}

void NavigationPolygon::remove_outline(int p_idx)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.remove_at(p_idx);
	rect_cache_dirty = true;
}

Vector<Vector2> NavigationPolygon::get_outline(int p_idx) const
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, outlines.size(), Vector<Vector2>());
	return outlines[p_idx];
}

void NavigationPolygon::clear_outlines()
{
	RWLockWrite write_lock(rwlock);
	outlines.clear();
	rect_cache_dirty = true;
}

#ifndef DISABLE_DEPRECATED
void NavigationPolygon::make_polygons_from_outlines()
{
	RWLockWrite write_lock(rwlock);
	WARN_PRINT("Function make_polygons_from_outlines() is deprecated."
			   "\nUse NavigationServer2D.parse_source_geometry_data() and "
			   "NavigationServer2D.bake_from_source_geometry_data() instead.");

	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	List<TPPLPoly> in_poly, out_poly;

	Vector2 outside_point(-1e10, -1e10);

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();
		for (int j = 0; j < olsize; j++) {
			outside_point = outside_point.max(r[j]);
		}
	}

	outside_point += Vector2(0.7239784, 0.819238); // avoid precision issues

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();

		int interscount = 0;
		// test if this is an outer outline
		for (int k = 0; k < outlines.size(); k++) {
			if (i == k) {
				continue; // no self intersect
			}

			Vector<Vector2> ol2 = outlines[k];
			int olsize2 = ol2.size();
			if (olsize2 < 3) {
				continue;
			}
			const Vector2* r2 = ol2.ptr();

			for (int l = 0; l < olsize2; l++) {
				if (Geometry2D::segment_intersects_segment(
						r[0], outside_point, r2[l], r2[(l + 1) % olsize2], nullptr)) {
					interscount++;
				}
			}
		}

		bool outer = (interscount % 2) == 0;

		TPPLPoly tp;
		tp.Init(olsize);
		for (int j = 0; j < olsize; j++) {
			tp[j] = r[j];
		}

		if (outer) {
			tp.SetOrientation(TPPL_ORIENTATION_CCW);
		}
		else {
			tp.SetOrientation(TPPL_ORIENTATION_CW);
			tp.SetHole(true);
		}

		in_poly.push_back(tp);
	}

	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // failed!
		ERR_PRINT("NavigationPolygon: Convex partition failed! Failed to convert outlines to a "
				  "valid NavigationPolygon."
				  "\nNavigationPolygon outlines can not overlap vertices or edges inside same "
				  "outline or with other outlines or have any intersections."
				  "\nAdd the outmost and largest outline first. To add holes inside this outline "
				  "add the smaller outlines with same winding order.");
		return;
	}

	polygons.clear();
	vertices.clear();

	HashMap<Vector2, int> points;
	for (const TPPLPoly& tp : out_poly) {
		Vector<int> p;

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			HashMap<Vector2, int>::Iterator E = points.find(tp[i]);
			if (!E) {
				E = points.insert(tp[i], vertices.size());
				vertices.push_back(tp[i]);
			}
			p.push_back(E->value);
		}

		polygons.push_back(p);
	}

	emit_changed();
}
#endif // DISABLE_DEPRECATED

void NavigationPolygon::set_cell_size(real_t p_cell_size)
{
	cell_size = p_cell_size;
	get_navigation_mesh()->set_cell_size(cell_size);
}

real_t NavigationPolygon::get_cell_size() const { return cell_size; }

void NavigationPolygon::set_border_size(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0.0);
	border_size = p_value;
}

real_t NavigationPolygon::get_border_size() const { return border_size; }

void NavigationPolygon::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationPolygon::SamplePartitionType NavigationPolygon::get_sample_partition_type() const
{
	return partition_type;
}

NavigationPolygon::ParsedGeometryType NavigationPolygon::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationPolygon::set_parsed_collision_mask(uint32_t p_mask)
{
	parsed_collision_mask = p_mask;
}

uint32_t NavigationPolygon::get_parsed_collision_mask() const { return parsed_collision_mask; }

void NavigationPolygon::set_parsed_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_parsed_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_parsed_collision_mask(mask);
}

bool NavigationPolygon::get_parsed_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_parsed_collision_mask() & (1 << (p_layer_number - 1));
}

NavigationPolygon::SourceGeometryMode NavigationPolygon::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationPolygon::set_source_geometry_group_name(const String& p_group_name)
{
	source_geometry_group_name = p_group_name;
}

String NavigationPolygon::get_source_geometry_group_name() const
{
	return source_geometry_group_name;
}

void NavigationPolygon::set_agent_radius(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

real_t NavigationPolygon::get_agent_radius() const { return agent_radius; }

void NavigationPolygon::set_baking_rect(const Rect2& p_rect)
{
	baking_rect = p_rect;
	emit_changed();
}

Rect2 NavigationPolygon::get_baking_rect() const { return baking_rect; }

void NavigationPolygon::set_baking_rect_offset(const Vector2& p_rect_offset)
{
	baking_rect_offset = p_rect_offset;
	emit_changed();
}

Vector2 NavigationPolygon::get_baking_rect_offset() const { return baking_rect_offset; }



bool NavigationPolygon::_edit_is_selected_on_click(Vector2 const&, double) const { return false; }

// --- navigation_mesh_source_geometry_data_2d.cpp ---

#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"

void NavigationMeshSourceGeometryData2D::clear()
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.clear();
	obstruction_outlines.clear();
	_projected_obstructions.clear();
	bounds_dirty = true;
}

bool NavigationMeshSourceGeometryData2D::has_data()
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines.size();
}

void NavigationMeshSourceGeometryData2D::clear_projected_obstructions()
{
	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_traversable_outlines(
	const Vector<Vector<Vector2>>& p_traversable_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_obstruction_outlines(
	const Vector<Vector<Vector2>>& p_obstruction_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	obstruction_outlines = p_obstruction_outlines;
	bounds_dirty = true;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_traversable_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_obstruction_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return obstruction_outlines;
}

void NavigationMeshSourceGeometryData2D::_add_traversable_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		traversable_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::_add_obstruction_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		obstruction_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_traversable_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> traversable_outline;
		traversable_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			traversable_outline.write[i] = p_shape_outline[i];
		}
		traversable_outlines.push_back(traversable_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_obstruction_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> obstruction_outline;
		obstruction_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			obstruction_outline.write[i] = p_shape_outline[i];
		}
		obstruction_outlines.push_back(obstruction_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::merge(
	const Ref<NavigationMeshSourceGeometryData2D>& p_other_geometry)
{
	ERR_FAIL_COND(p_other_geometry.is_null());

	Vector<Vector<Vector2>> other_traversable_outlines;
	Vector<Vector<Vector2>> other_obstruction_outlines;
	Vector<ProjectedObstruction> other_projected_obstructions;

	p_other_geometry->get_data(
		other_traversable_outlines, other_obstruction_outlines, other_projected_obstructions);

	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.append_array(other_traversable_outlines);
	obstruction_outlines.append_array(other_obstruction_outlines);
	_projected_obstructions.append_array(other_projected_obstructions);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::add_projected_obstruction(
	const Vector<Vector2>& p_vertices, bool p_carve)
{
	ERR_FAIL_COND(p_vertices.size() < 2);

	ProjectedObstruction projected_obstruction;
	projected_obstruction.vertices.resize(p_vertices.size() * 2);
	projected_obstruction.carve = p_carve;

	float* obstruction_vertices_ptrw = projected_obstruction.vertices.ptrw();

	int vertex_index = 0;
	for (const Vector2& vertex : p_vertices) {
		obstruction_vertices_ptrw[vertex_index++] = vertex.x;
		obstruction_vertices_ptrw[vertex_index++] = vertex.y;
	}

	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.push_back(projected_obstruction);
	bounds_dirty = true;
}

Vector<NavigationMeshSourceGeometryData2D::ProjectedObstruction>
NavigationMeshSourceGeometryData2D::_get_projected_obstructions() const
{
	RWLockRead read_lock(geometry_rwlock);
	return _projected_obstructions;
}

void NavigationMeshSourceGeometryData2D::set_data(
	const Vector<Vector<Vector2>>& p_traversable_outlines,
	const Vector<Vector<Vector2>>& p_obstruction_outlines,
	Vector<ProjectedObstruction>& p_projected_obstructions)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	obstruction_outlines = p_obstruction_outlines;
	_projected_obstructions = p_projected_obstructions;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::get_data(Vector<Vector<Vector2>>& r_traversable_outlines,
	Vector<Vector<Vector2>>& r_obstruction_outlines,
	Vector<ProjectedObstruction>& r_projected_obstructions)
{
	RWLockRead read_lock(geometry_rwlock);
	r_traversable_outlines = traversable_outlines;
	r_obstruction_outlines = obstruction_outlines;
	r_projected_obstructions = _projected_obstructions;
}

Rect2 NavigationMeshSourceGeometryData2D::get_bounds()
{
	geometry_rwlock.read_lock();

	if (bounds_dirty) {
		geometry_rwlock.read_unlock();
		RWLockWrite write_lock(geometry_rwlock);

		bounds_dirty = false;
		bounds = Rect2();
		bool first_vertex = true;

		for (const Vector<Vector2>& traversable_outline : traversable_outlines) {
			for (const Vector2& traversable_point : traversable_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = traversable_point;
				}
				else {
					bounds.expand_to(traversable_point);
				}
			}
		}

		for (const Vector<Vector2>& obstruction_outline : obstruction_outlines) {
			for (const Vector2& obstruction_point : obstruction_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = obstruction_point;
				}
				else {
					bounds.expand_to(obstruction_point);
				}
			}
		}

		for (const ProjectedObstruction& projected_obstruction : _projected_obstructions) {
			for (int i = 0; i < projected_obstruction.vertices.size() / 2; i++) {
				const Vector2 vertex = Vector2(projected_obstruction.vertices[i * 2],
					projected_obstruction.vertices[i * 2 + 1]);
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

// --- polygon_path_finder.cpp ---

#include "core/math/geometry_2d.h"
// --- Inlined from scene/resources/2d/polygon_path_finder.h ---
#include "core/io/resource.h"

class PolygonPathFinder : public Resource {

	struct Point {
		Vector2 pos;
		HashSet<int> connections;
		float distance = 0.0;
		float penalty = 0.0;
		int prev = 0;
	};

	union Edge {
		struct {
			int32_t points[2];
		};
		uint64_t key = 0;

		_FORCE_INLINE_ bool operator==(const Edge &p_edge) const {
			return key == p_edge.key;
		}
		_FORCE_INLINE_ static uint32_t hash(const Edge &p_edge) {
			return hash_one_uint64(p_edge.key);
		}

		Edge(int a = 0, int b = 0) {
			if (a > b) {
				SWAP(a, b);
			}
			points[0] = a;
			points[1] = b;
		}
	};

	Vector2 outside_point;
	Rect2 bounds;

	Vector<Point> points;
	HashSet<Edge, Edge> edges;

	bool _is_point_inside(const Vector2 &p_point) const;

public:
	void setup(const Vector<Vector2> &p_points, const Vector<int> &p_connections);
	Vector<Vector2> find_path(const Vector2 &p_from, const Vector2 &p_to);

	void set_point_penalty(int p_point, float p_penalty);
	float get_point_penalty(int p_point) const;

	bool is_point_inside(const Vector2 &p_point) const;
	Vector2 get_closest_point(const Vector2 &p_point) const;
	Vector<Vector2> get_intersections(const Vector2 &p_from, const Vector2 &p_to) const;
	Rect2 get_bounds() const;

	PolygonPathFinder();
};

// --- End scene/resources/2d/polygon_path_finder.h ---

bool PolygonPathFinder::_is_point_inside(const Vector2& p_point) const
{
	int crosses = 0;

	for (const Edge& E : edges) {
		const Edge& e = E;

		Vector2 a = points[e.points[0]].pos;
		Vector2 b = points[e.points[1]].pos;

		if (Geometry2D::segment_intersects_segment(a, b, p_point, outside_point, nullptr)) {
			crosses++;
		}
	}

	return crosses & 1;
}

void PolygonPathFinder::setup(const Vector<Vector2>& p_points, const Vector<int>& p_connections)
{
	ERR_FAIL_COND(p_connections.size() & 1);

	points.clear();
	edges.clear();

	// insert points

	int point_count = p_points.size();
	points.resize(point_count + 2);
	bounds = Rect2();

	for (int i = 0; i < p_points.size(); i++) {
		points.write[i].pos = p_points[i];
		points.write[i].penalty = 0;

		outside_point = i == 0 ? p_points[0] : p_points[i].max(outside_point);

		if (i == 0) {
			bounds.position = points[i].pos;
		}
		else {
			bounds.expand_to(points[i].pos);
		}
	}

	outside_point.x += 20.451 + Math::randf() * 10.2039;
	outside_point.y += 21.193 + Math::randf() * 12.5412;

	// insert edges (which are also connections)

	for (int i = 0; i < p_connections.size(); i += 2) {
		Edge e(p_connections[i], p_connections[i + 1]);
		ERR_FAIL_INDEX(e.points[0], point_count);
		ERR_FAIL_INDEX(e.points[1], point_count);
		points.write[p_connections[i]].connections.insert(p_connections[i + 1]);
		points.write[p_connections[i + 1]].connections.insert(p_connections[i]);
		edges.insert(e);
	}

	// fill the remaining connections based on visibility

	for (int i = 0; i < point_count; i++) {
		for (int j = i + 1; j < point_count; j++) {
			if (edges.has(Edge(i, j))) {
				continue; // if in edge ignore
			}

			Vector2 from = points[i].pos;
			Vector2 to = points[j].pos;

			if (!_is_point_inside(
					from * 0.5 + to * 0.5)) { // connection between points in inside space
				continue;
			}

			bool valid = true;

			for (const Edge& E : edges) {
				const Edge& e = E;
				if (e.points[0] == i || e.points[1] == i || e.points[0] == j || e.points[1] == j) {
					continue;
				}

				Vector2 a = points[e.points[0]].pos;
				Vector2 b = points[e.points[1]].pos;

				if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				points.write[i].connections.insert(j);
				points.write[j].connections.insert(i);
			}
		}
	}
}

Vector<Vector2> PolygonPathFinder::find_path(const Vector2& p_from, const Vector2& p_to)
{
	Vector<Vector2> path;

	Vector2 from = p_from;
	Vector2 to = p_to;
	Edge ignore_from_edge(-1, -1);
	Edge ignore_to_edge(-1, -1);

	if (!_is_point_inside(from)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(from, segment_a, segment_b);
			float d = from.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_from_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		from = closest_point;
	};

	if (!_is_point_inside(to)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(to, segment_a, segment_b);
			float d = to.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_to_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		to = closest_point;
	};

	// test direct connection
	{
		bool can_see_eachother = true;

		for (const Edge& E : edges) {
			const Edge& e = E;
			if (e.points[0] == ignore_from_edge.points[0] &&
				e.points[1] == ignore_from_edge.points[1]) {
				continue;
			}
			if (e.points[0] == ignore_to_edge.points[0] &&
				e.points[1] == ignore_to_edge.points[1]) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
				can_see_eachother = false;
				break;
			}
		}

		if (can_see_eachother) {
			path.push_back(from);
			path.push_back(to);
			return path;
		}
	}

	// add to graph

	int aidx = points.size() - 2;
	int bidx = points.size() - 1;
	points.write[aidx].pos = from;
	points.write[bidx].pos = to;
	points.write[aidx].distance = 0;
	points.write[bidx].distance = 0;
	points.write[aidx].prev = -1;
	points.write[bidx].prev = -1;
	points.write[aidx].penalty = 0;
	points.write[bidx].penalty = 0;

	for (int i = 0; i < points.size() - 2; i++) {
		bool valid_a = true;
		bool valid_b = true;
		points.write[i].prev = -1;
		points.write[i].distance = 0;

		if (!_is_point_inside(from * 0.5 + points[i].pos * 0.5)) {
			valid_a = false;
		}

		if (!_is_point_inside(to * 0.5 + points[i].pos * 0.5)) {
			valid_b = false;
		}

		for (const Edge& E : edges) {
			const Edge& e = E;

			if (e.points[0] == i || e.points[1] == i) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (valid_a) {
				if (e.points[0] != ignore_from_edge.points[1] &&
					e.points[1] != ignore_from_edge.points[1] &&
					e.points[0] != ignore_from_edge.points[0] &&
					e.points[1] != ignore_from_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(
							a, b, from, points[i].pos, nullptr)) {
						valid_a = false;
					}
				}
			}

			if (valid_b) {
				if (e.points[0] != ignore_to_edge.points[1] &&
					e.points[1] != ignore_to_edge.points[1] &&
					e.points[0] != ignore_to_edge.points[0] &&
					e.points[1] != ignore_to_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(a, b, to, points[i].pos, nullptr)) {
						valid_b = false;
					}
				}
			}

			if (!valid_a && !valid_b) {
				break;
			}
		}

		if (valid_a) {
			points.write[i].connections.insert(aidx);
			points.write[aidx].connections.insert(i);
		}

		if (valid_b) {
			points.write[i].connections.insert(bidx);
			points.write[bidx].connections.insert(i);
		}
	}
	// solve graph

	HashSet<int> open_list;

	points.write[aidx].distance = 0;
	points.write[aidx].prev = aidx;
	for (const int& E : points[aidx].connections) {
		open_list.insert(E);
		points.write[E].distance = from.distance_to(points[E].pos);
		points.write[E].prev = aidx;
	}

	bool found_route = false;

	while (true) {
		if (open_list.is_empty()) {
			print_verbose("Open list empty.");
			break;
		}
		// check open list

		int least_cost_point = -1;
		float least_cost = 1e30;

		// this could be faster (cache previous results)
		for (const int& E : open_list) {
			const Point& p = points[E];
			float cost = p.distance;
			cost += p.pos.distance_to(to);
			cost += p.penalty;

			if (cost < least_cost) {
				least_cost_point = E;
				least_cost = cost;
			}
		}

		const Point& np = points[least_cost_point];
		// open the neighbors for search

		for (const int& E : np.connections) {
			Point& p = points.write[E];
			float distance = np.pos.distance_to(p.pos) + np.distance;

			if (p.prev != -1) {
				// oh this was visited already, can we win the cost?

				if (p.distance > distance) {
					p.prev = least_cost_point; // reassign previous
					p.distance = distance;
				}
			}
			else {
				// add to open neighbors

				p.prev = least_cost_point;
				p.distance = distance;
				open_list.insert(E);

				if (E == bidx) {
					// oh my reached end! stop algorithm
					found_route = true;
					break;
				}
			}
		}

		if (found_route) {
			break;
		}

		open_list.erase(least_cost_point);
	}

	if (found_route) {
		int at = bidx;
		path.push_back(points[at].pos);
		do {
			at = points[at].prev;
			path.push_back(points[at].pos);
		} while (at != aidx);

		path.reverse();
	}

	for (int i = 0; i < points.size() - 2; i++) {
		points.write[i].connections.erase(aidx);
		points.write[i].connections.erase(bidx);
		points.write[i].prev = -1;
		points.write[i].distance = 0;
	}

	points.write[aidx].connections.clear();
	points.write[aidx].prev = -1;
	points.write[aidx].distance = 0;
	points.write[bidx].connections.clear();
	points.write[bidx].prev = -1;
	points.write[bidx].distance = 0;

	return path;
}

bool PolygonPathFinder::is_point_inside(const Vector2& p_point) const
{
	return _is_point_inside(p_point);
}

Vector2 PolygonPathFinder::get_closest_point(const Vector2& p_point) const
{
	float closest_dist = 1e20f;
	Vector2 closest_point;

	for (const Edge& E : edges) {
		const Edge& e = E;
		const Vector2 segment_a = points[e.points[0]].pos;
		const Vector2 segment_b = points[e.points[1]].pos;
		Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
		float d = p_point.distance_squared_to(closest);

		if (d < closest_dist) {
			closest_dist = d;
			closest_point = closest;
		}
	}

	ERR_FAIL_COND_V(Math::is_equal_approx(closest_dist, 1e20f), Vector2());

	return closest_point;
}

Vector<Vector2> PolygonPathFinder::get_intersections(
	const Vector2& p_from, const Vector2& p_to) const
{
	Vector<Vector2> inters;

	for (const Edge& E : edges) {
		Vector2 a = points[E.points[0]].pos;
		Vector2 b = points[E.points[1]].pos;

		Vector2 res;
		if (Geometry2D::segment_intersects_segment(a, b, p_from, p_to, &res)) {
			inters.push_back(res);
		}
	}

	return inters;
}

Rect2 PolygonPathFinder::get_bounds() const { return bounds; }

void PolygonPathFinder::set_point_penalty(int p_point, float p_penalty)
{
	ERR_FAIL_INDEX(p_point, points.size() - 2);
	points.write[p_point].penalty = p_penalty;
}

float PolygonPathFinder::get_point_penalty(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, points.size() - 2, 0);
	return points[p_point].penalty;
}

PolygonPathFinder::PolygonPathFinder() {}
