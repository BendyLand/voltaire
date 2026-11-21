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
