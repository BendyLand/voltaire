/**************************************************************************/
/*  navigation_region_3d.cpp                                              */
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
#include "navigation_region_3d.h"
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

RID NavigationRegion3D::get_rid() const { return region; }

void NavigationRegion3D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;

	NavigationServer3D::get_singleton()->region_set_enabled(region, enabled);

#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		if (!is_enabled()) {
			if (debug_mesh.is_valid()) {
				if (debug_mesh->get_surface_count() > 0) {
					RS::get_singleton()->instance_set_surface_override_material(debug_instance, 0,
						NavigationServer3D::get_singleton()
							->get_debug_navigation_geometry_face_disabled_material()
							->get_rid());
				}
				if (debug_mesh->get_surface_count() > 1) {
					RS::get_singleton()->instance_set_surface_override_material(debug_instance, 1,
						NavigationServer3D::get_singleton()
							->get_debug_navigation_geometry_edge_disabled_material()
							->get_rid());
				}
			}
		}
		else {
			if (debug_mesh.is_valid()) {
				if (debug_mesh->get_surface_count() > 0) {
					RS::get_singleton()->instance_set_surface_override_material(
						debug_instance, 0, RID());
				}
				if (debug_mesh->get_surface_count() > 1) {
					RS::get_singleton()->instance_set_surface_override_material(
						debug_instance, 1, RID());
				}
			}
		}
	}
#endif // DEBUG_ENABLED

	update_gizmos();
}

bool NavigationRegion3D::is_enabled() const { return enabled; }

void NavigationRegion3D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer3D::get_singleton()->region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion3D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::get_singleton()->region_set_navigation_layers(region, navigation_layers);
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

	NavigationServer3D::get_singleton()->region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion3D::get_enter_cost() const { return enter_cost; }

void NavigationRegion3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::get_singleton()->region_set_travel_cost(region, travel_cost);
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

	NavigationServer3D::get_singleton()->region_set_map(region, map_override);
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
		NavigationServer3D::get_singleton()->region_set_map(region, map_override);
	}
	else {
		NavigationServer3D::get_singleton()->region_set_map(
			region, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::get_singleton()->region_set_transform(region, get_global_transform());
	NavigationServer3D::get_singleton()->region_set_enabled(region, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_singleton()->get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_exit_navigation_map()
{
	NavigationServer3D::get_singleton()->region_set_map(region, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		RS::get_singleton()->instance_set_visible(debug_instance, false);
	}
	if (debug_edge_connections_instance.is_valid()) {
		RS::get_singleton()->instance_set_visible(debug_edge_connections_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::get_singleton()->region_set_transform(region, get_global_transform());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		RS::get_singleton()->instance_set_transform(debug_instance, get_global_transform());
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


