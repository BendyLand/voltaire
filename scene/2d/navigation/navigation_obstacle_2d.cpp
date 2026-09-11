/**************************************************************************/
/*  navigation_obstacle_2d.cpp                                            */
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

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "navigation_obstacle_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "scene/resources/world_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/rendering/rendering_server.h"

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
		NavigationServer2D::get_singleton()->obstacle_set_avoidance_enabled(
			obstacle, avoidance_enabled);
		_update_transform();
		set_physics_process_internal(true);
#ifdef DEBUG_ENABLED
		RS::get_singleton()->canvas_item_set_parent(
			debug_canvas_item, get_world_2d()->get_canvas());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_physics_process_internal(false);
		_update_map(RID());
#ifdef DEBUG_ENABLED
		RS::get_singleton()->canvas_item_set_parent(debug_canvas_item, RID());
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
		NavigationServer2D::get_singleton()->obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (get_tree()->is_paused()) {
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
		NavigationServer2D::get_singleton()->obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
#ifdef DEBUG_ENABLED
		RS::get_singleton()->canvas_item_set_visible(debug_canvas_item, is_visible_in_tree());
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
					NavigationServer2D::get_singleton()->obstacle_set_velocity(obstacle, velocity);
				}
				previous_velocity = velocity;
			}
		}
	} break;

	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		if (is_inside_tree()) {
			bool is_debug_enabled = false;
			if (Engine::get_singleton()->is_editor_hint()) {
				is_debug_enabled = true;
			}
			else if (NavigationServer2D::get_singleton()->get_debug_enabled() &&
					   NavigationServer2D::get_singleton()->get_debug_avoidance_enabled()) {
				is_debug_enabled = true;
			}

			if (is_debug_enabled) {
				RS::get_singleton()->canvas_item_clear(debug_canvas_item);
				RS::get_singleton()->canvas_item_set_transform(debug_canvas_item, Transform2D());
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
	obstacle = NavigationServer2D::get_singleton()->obstacle_create();

	NavigationServer2D::get_singleton()->obstacle_set_radius(obstacle, radius);
	NavigationServer2D::get_singleton()->obstacle_set_vertices(obstacle, vertices);
	NavigationServer2D::get_singleton()->obstacle_set_avoidance_layers(obstacle, avoidance_layers);
	NavigationServer2D::get_singleton()->obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);

#ifdef DEBUG_ENABLED
	debug_canvas_item = RenderingServer::get_singleton()->canvas_item_create();
	debug_mesh_rid = RenderingServer::get_singleton()->mesh_create();
#endif // DEBUG_ENABLED
}

NavigationObstacle2D::~NavigationObstacle2D()
{
	ERR_FAIL_NULL(NavigationServer2D::get_singleton());

	NavigationServer2D::get_singleton()->free_rid(obstacle);
	obstacle = RID();

#ifdef DEBUG_ENABLED
	if (debug_mesh_rid.is_valid()) {
		RenderingServer::get_singleton()->free_rid(debug_mesh_rid);
		debug_mesh_rid = RID();
	}
	if (debug_canvas_item.is_valid()) {
		RenderingServer::get_singleton()->free_rid(debug_canvas_item);
		debug_canvas_item = RID();
	}
#endif // DEBUG_ENABLED
}

void NavigationObstacle2D::set_vertices(const Vector<Vector2>& p_vertices)
{
	vertices = p_vertices;

	vertices_are_clockwise = !Geometry2D::is_polygon_clockwise(vertices); // Geometry2D is inverted.
	vertices_are_valid = !Geometry2D::triangulate_polygon(vertices).is_empty();

	const Transform2D node_transform = is_inside_tree() ? get_global_transform() : Transform2D();
	NavigationServer2D::get_singleton()->obstacle_set_vertices(
		obstacle, node_transform.xform(vertices));
#ifdef DEBUG_ENABLED
	queue_redraw();
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

void NavigationObstacle2D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;

	const Vector2 safe_scale =
		(is_inside_tree() ? get_global_scale() : get_scale()).abs().maxf(0.001);
	NavigationServer2D::get_singleton()->obstacle_set_radius(
		obstacle, safe_scale[safe_scale.max_axis_index()] * radius);
#ifdef DEBUG_ENABLED
	queue_redraw();
#endif // DEBUG_ENABLED
}

void NavigationObstacle2D::set_avoidance_layers(uint32_t p_layers)
{
	if (avoidance_layers == p_layers) {
		return;
	}
	avoidance_layers = p_layers;
	NavigationServer2D::get_singleton()->obstacle_set_avoidance_layers(obstacle, avoidance_layers);
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

void NavigationObstacle2D::set_avoidance_enabled(bool p_enabled)
{
	if (avoidance_enabled == p_enabled) {
		return;
	}

	avoidance_enabled = p_enabled;
	NavigationServer2D::get_singleton()->obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);
#ifdef DEBUG_ENABLED
	queue_redraw();
#endif // DEBUG_ENABLED
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
	NavigationServer2D::get_singleton()->obstacle_set_map(obstacle, p_map);
}

void NavigationObstacle2D::_update_position(const Vector2 p_position)
{
	NavigationServer2D::get_singleton()->obstacle_set_position(obstacle, p_position);
#ifdef DEBUG_ENABLED
	queue_redraw();
#endif // DEBUG_ENABLED
}

void NavigationObstacle2D::_update_transform()
{
	_update_position(get_global_position());
	// Prevent non-positive or non-uniform scaling of dynamic obstacle radius.
	const Vector2 safe_scale = get_global_scale().abs().maxf(0.001);
	const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
	NavigationServer2D::get_singleton()->obstacle_set_radius(obstacle, scaling_max_value * radius);
	NavigationServer2D::get_singleton()->obstacle_set_vertices(
		obstacle, get_global_transform().translated(-get_global_position()).xform(vertices));
#ifdef DEBUG_ENABLED
	queue_redraw();
#endif // DEBUG_ENABLED
}

#ifdef DEBUG_ENABLED
void NavigationObstacle2D::_update_fake_agent_radius_debug()
{
	if (radius > 0.0 && NavigationServer2D::get_singleton()
							->get_debug_navigation_avoidance_enable_obstacles_radius()) {
		Color debug_radius_color = NavigationServer2D::get_singleton()
									   ->get_debug_navigation_avoidance_obstacles_radius_color();
		// Prevent non-positive scaling.
		const Vector2 safe_scale = get_global_scale().abs().maxf(0.001);
		// Agent radius is a scalar value and does not support non-uniform scaling, choose the
		// largest axis.
		const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
		RS::get_singleton()->canvas_item_add_circle(debug_canvas_item, get_global_position(),
			scaling_max_value * radius, debug_radius_color);
	}
}
#endif // DEBUG_ENABLED


