/**************************************************************************/
/*  navigation_region_2d.cpp                                              */
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
#include "core/math/random_pcg.h"
#include "navigation_region_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/rendering/rendering_server.h"

RID NavigationRegion2D::get_rid() const { return region; }

void NavigationRegion2D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;

	NavigationServer2D::get_singleton()->region_set_enabled(region, enabled);

#ifdef DEBUG_ENABLED
	if (Engine::get_singleton()->is_editor_hint() ||
		NavigationServer2D::get_singleton()->get_debug_navigation_enabled()) {
		queue_redraw();
	}
#endif // DEBUG_ENABLED
}

bool NavigationRegion2D::is_enabled() const { return enabled; }

void NavigationRegion2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer2D::get_singleton()->region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion2D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer2D::get_singleton()->region_set_navigation_layers(region, navigation_layers);
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

	NavigationServer2D::get_singleton()->region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion2D::get_enter_cost() const { return enter_cost; }

void NavigationRegion2D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer2D::get_singleton()->region_set_travel_cost(region, travel_cost);
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
			(Engine::get_singleton()->is_editor_hint() ||
				(NavigationServer2D::get_singleton()->get_debug_enabled() &&
					NavigationServer2D::get_singleton()->get_debug_navigation_enabled())) &&
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

	NavigationServer2D::get_singleton()->region_set_map(region, map_override);
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









#ifdef DEBUG_ENABLED
void NavigationRegion2D::_navigation_map_changed(RID p_map)
{
	if (is_inside_tree() && get_world_2d()->get_navigation_map() == p_map) {
		queue_redraw();
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_navigation_debug_changed()
{
	if (is_inside_tree()) {
		queue_redraw();
	}
}
#endif // DEBUG_ENABLED

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



#ifndef DISABLE_DEPRECATED
// Compatibility with earlier 4.0 betas.



#endif // DISABLE_DEPRECATED





void NavigationRegion2D::_region_enter_navigation_map()
{
	if (!is_inside_tree()) {
		return;
	}

	if (map_override.is_valid()) {
		NavigationServer2D::get_singleton()->region_set_map(region, map_override);
	}
	else {
		NavigationServer2D::get_singleton()->region_set_map(
			region, get_world_2d()->get_navigation_map());
	}

	NavigationServer2D::get_singleton()->region_set_transform(region, get_global_transform());
	NavigationServer2D::get_singleton()->region_set_enabled(region, enabled);

	queue_redraw();
}

void NavigationRegion2D::_region_exit_navigation_map()
{
	NavigationServer2D::get_singleton()->region_set_map(region, RID());
}

void NavigationRegion2D::_region_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer2D::get_singleton()->region_set_transform(region, get_global_transform());

	queue_redraw();
}

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_edge_connections_mesh()
{
	const NavigationServer2D* ns2d = NavigationServer2D::get_singleton();
	bool enable_edge_connections =
		use_edge_connections && ns2d->get_debug_navigation_enable_edge_connections() &&
		ns2d->map_get_use_edge_connections(get_world_2d()->get_navigation_map());

	if (enable_edge_connections) {
		Color debug_edge_connection_color = ns2d->get_debug_navigation_edge_connection_color();
		// Draw the region edge connections.
		Transform2D xform = get_global_transform();
		real_t radius =
			ns2d->map_get_edge_connection_margin(get_world_2d()->get_navigation_map()) / 2.0;
		for (int i = 0; i < ns2d->region_get_connections_count(region); i++) {
			// Two main points
			Vector2 a = ns2d->region_get_connection_pathway_start(region, i);
			a = xform.affine_inverse().xform(a);
			Vector2 b = ns2d->region_get_connection_pathway_end(region, i);
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
	RenderingServer* rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);
	if (debug_instance_rid.is_valid()) {
		RS::get_singleton()->canvas_item_set_visible(debug_instance_rid, p_visible);
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


