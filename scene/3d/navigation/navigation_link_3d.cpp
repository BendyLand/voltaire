/**************************************************************************/
/*  navigation_link_3d.cpp                                                */
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
#include "navigation_link_3d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

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
	ERR_FAIL_NULL(NavigationServer3D::get_singleton());
	NavigationServer3D::get_singleton()->free_rid(link);
	link = RID();

#ifdef DEBUG_ENABLED
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	if (debug_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(debug_instance);
	}
	if (debug_mesh.is_valid()) {
		RenderingServer::get_singleton()->free_rid(debug_mesh->get_rid());
	}
#endif // DEBUG_ENABLED
}

RID NavigationLink3D::get_rid() const { return link; }

void NavigationLink3D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;

	NavigationServer3D::get_singleton()->link_set_enabled(link, enabled);

#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid() && debug_mesh.is_valid()) {
		if (enabled) {
			Ref<StandardMaterial3D> link_material =
				NavigationServer3D::get_singleton()
					->get_debug_navigation_link_connections_material();
			RS::get_singleton()->instance_set_surface_override_material(
				debug_instance, 0, link_material->get_rid());
		}
		else {
			Ref<StandardMaterial3D> disabled_link_material =
				NavigationServer3D::get_singleton()
					->get_debug_navigation_link_connections_disabled_material();
			RS::get_singleton()->instance_set_surface_override_material(
				debug_instance, 0, disabled_link_material->get_rid());
		}
	}
#endif // DEBUG_ENABLED

	update_gizmos();
}

void NavigationLink3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::get_singleton()->link_set_map(link, map_override);
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

void NavigationLink3D::set_bidirectional(bool p_bidirectional)
{
	if (bidirectional == p_bidirectional) {
		return;
	}

	bidirectional = p_bidirectional;

	NavigationServer3D::get_singleton()->link_set_bidirectional(link, bidirectional);

#ifdef DEBUG_ENABLED
	_update_debug_mesh();
#endif // DEBUG_ENABLED

	update_gizmos();
}

void NavigationLink3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::get_singleton()->link_set_navigation_layers(link, navigation_layers);
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

void NavigationLink3D::set_start_position(Vector3 p_position)
{
	if (start_position.is_equal_approx(p_position)) {
		return;
	}

	start_position = p_position;

	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::get_singleton()->link_set_start_position(
		link, get_global_transform().xform(start_position));

#ifdef DEBUG_ENABLED
	_update_debug_mesh();
#endif // DEBUG_ENABLED

	update_gizmos();
	update_configuration_warnings();
}

void NavigationLink3D::set_end_position(Vector3 p_position)
{
	if (end_position.is_equal_approx(p_position)) {
		return;
	}

	end_position = p_position;

	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::get_singleton()->link_set_end_position(
		link, get_global_transform().xform(end_position));

#ifdef DEBUG_ENABLED
	_update_debug_mesh();
#endif // DEBUG_ENABLED

	update_gizmos();
	update_configuration_warnings();
}

void NavigationLink3D::set_global_start_position(Vector3 p_position)
{
	if (is_inside_tree()) {
		set_start_position(to_local(p_position));
	}
	else {
		set_start_position(p_position);
	}
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

void NavigationLink3D::set_global_end_position(Vector3 p_position)
{
	if (is_inside_tree()) {
		set_end_position(to_local(p_position));
	}
	else {
		set_end_position(p_position);
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

	NavigationServer3D::get_singleton()->link_set_enter_cost(link, enter_cost);
}

void NavigationLink3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::get_singleton()->link_set_travel_cost(link, travel_cost);
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
		NavigationServer3D::get_singleton()->link_set_map(link, map_override);
	}
	else {
		NavigationServer3D::get_singleton()->link_set_map(
			link, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::get_singleton()->link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::get_singleton()->link_set_end_position(
		link, get_global_transform().xform(end_position));
	NavigationServer3D::get_singleton()->link_set_enabled(link, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_singleton()->get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_exit_navigation_map()
{
	NavigationServer3D::get_singleton()->link_set_map(link, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		RS::get_singleton()->instance_set_visible(debug_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::get_singleton()->link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::get_singleton()->link_set_end_position(
		link, get_global_transform().xform(end_position));
#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_singleton()->get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}


