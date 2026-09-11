/**************************************************************************/
/*  grid_map.cpp                                                          */
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
#include "core/io/marshalls.h"
#include "core/templates/a_hash_map.h"
#include "grid_map.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/surface_tool.h"
#include "servers/rendering/rendering_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "core/math/convex_hull.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/height_map_shape_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/physics_material.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "servers/navigation_3d/navigation_server_3d.h"

RID GridMap::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_3D_DISABLED
void GridMap::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	_update_physics_bodies_collision_properties();
}

uint32_t GridMap::get_collision_layer() const { return collision_layer; }

void GridMap::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	_update_physics_bodies_collision_properties();
}

uint32_t GridMap::get_collision_mask() const { return collision_mask; }

void GridMap::set_collision_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t collision_layer_new = get_collision_layer();
	if (p_value) {
		collision_layer_new |= 1 << (p_layer_number - 1);
	}
	else {
		collision_layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(collision_layer_new);
}

bool GridMap::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void GridMap::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

void GridMap::set_collision_priority(real_t p_priority)
{
	collision_priority = p_priority;
	_update_physics_bodies_collision_properties();
}

real_t GridMap::get_collision_priority() const { return collision_priority; }

void GridMap::set_collision_visibility_mode(DebugVisibilityMode p_visibility_mode)
{
	if (collision_visibility_mode == p_visibility_mode) {
		return;
	}
	collision_visibility_mode = p_visibility_mode;
	_recreate_octant_data();
}

GridMap::DebugVisibilityMode GridMap::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

void GridMap::set_physics_material(Ref<PhysicsMaterial> p_material)
{
	physics_material = p_material;
	_update_physics_bodies_characteristics();
}

Ref<PhysicsMaterial> GridMap::get_physics_material() const { return physics_material; }

bool GridMap::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

RID GridMap::get_physics_body_from_octant_coord(const Vector3i& p_octant_coords) const
{
	OctantKey octantkey;
	octantkey.x = p_octant_coords.x;
	octantkey.y = p_octant_coords.y;
	octantkey.z = p_octant_coords.z;

	const HashMap<OctantKey, Octant*, OctantKey>::ConstIterator octant_kv =
		octant_map.find(octantkey);

	if (!octant_kv) {
		return RID();
	}

	Octant* g = octant_kv->value;
	RID body_rid = g->static_body;

	return body_rid;
}
#endif // PHYSICS_3D_DISABLED

void GridMap::set_bake_navigation(bool p_bake_navigation)
{
	bake_navigation = p_bake_navigation;
	_recreate_octant_data();
}

bool GridMap::is_baking_navigation() { return bake_navigation; }

#ifndef NAVIGATION_3D_DISABLED
void GridMap::set_navigation_map(RID p_navigation_map)
{
	map_override = p_navigation_map;
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		Octant& g = *octant_map[E.key];
		for (KeyValue<IndexKey, Octant::NavigationCell>& F : g.navigation_cell_ids) {
			if (F.value.region.is_valid()) {
				NavigationServer3D::get_singleton()->region_set_map(F.value.region, map_override);
			}
		}
	}
}

RID GridMap::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}
#endif // NAVIGATION_3D_DISABLED

Ref<MeshLibrary> GridMap::get_mesh_library() const { return mesh_library; }

Vector3 GridMap::get_cell_size() const { return cell_size; }

void GridMap::set_octant_size(int p_size)
{
	ERR_FAIL_COND(p_size == 0);
	octant_size = p_size;
	_recreate_octant_data();
}

int GridMap::get_octant_size() const { return octant_size; }

void GridMap::set_center_x(bool p_enable)
{
	center_x = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_x() const { return center_x; }

void GridMap::set_center_y(bool p_enable)
{
	center_y = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_y() const { return center_y; }

void GridMap::set_center_z(bool p_enable)
{
	center_z = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_z() const { return center_z; }

void GridMap::set_debug_octant_color(const Color& p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_octant_color == p_color) {
		return;
	}

	debug_octant_color = p_color;

	if (debug_octant_line_material.is_valid()) {
		debug_octant_line_material->set_albedo(debug_octant_color);
	}
#endif
}

Color GridMap::get_debug_octant_color() const { return debug_octant_color; }

#ifdef DEBUG_ENABLED

void GridMap::_debug_clear_octants()
{
	for (const KeyValue<OctantKey, Octant*>& ele : octant_map) {
		OctantKey octant_key = ele.key;
		HashMap<OctantKey, OctantDebug*, OctantKey>::Iterator E = debug_octant_map.find(octant_key);
		if (E) {
			OctantDebug& octant_debug = *E->value;

			if (octant_debug.debug_line_mesh_rid.is_valid()) {
				RS::get_singleton()->free_rid(octant_debug.debug_line_mesh_rid);
				octant_debug.debug_line_mesh_rid = RID();
			}
			if (octant_debug.debug_line_instance_rid.is_valid()) {
				RS::get_singleton()->free_rid(octant_debug.debug_line_instance_rid);
				octant_debug.debug_line_instance_rid = RID();
			}

			memdelete(&octant_debug);
		}
	}

	debug_octant_map.clear();
}

#endif // DEBUG_ENABLED

int GridMap::get_cell_item(const Vector3i& p_position) const
{
	ERR_FAIL_INDEX_V(Math::abs(p_position.x), 1 << 20, INVALID_CELL_ITEM);
	ERR_FAIL_INDEX_V(Math::abs(p_position.y), 1 << 20, INVALID_CELL_ITEM);
	ERR_FAIL_INDEX_V(Math::abs(p_position.z), 1 << 20, INVALID_CELL_ITEM);

	IndexKey key;
	key.x = p_position.x;
	key.y = p_position.y;
	key.z = p_position.z;

	if (!cell_map.has(key)) {
		return INVALID_CELL_ITEM;
	}
	return cell_map[key].item;
}

int GridMap::get_cell_item_orientation(const Vector3i& p_position) const
{
	ERR_FAIL_INDEX_V(Math::abs(p_position.x), 1 << 20, -1);
	ERR_FAIL_INDEX_V(Math::abs(p_position.y), 1 << 20, -1);
	ERR_FAIL_INDEX_V(Math::abs(p_position.z), 1 << 20, -1);

	IndexKey key;
	key.x = p_position.x;
	key.y = p_position.y;
	key.z = p_position.z;

	if (!cell_map.has(key)) {
		return -1;
	}
	return cell_map[key].rot;
}

static const Basis _ortho_bases[24] = {Basis(1, 0, 0, 0, 1, 0, 0, 0, 1),
	Basis(0, -1, 0, 1, 0, 0, 0, 0, 1), Basis(-1, 0, 0, 0, -1, 0, 0, 0, 1),
	Basis(0, 1, 0, -1, 0, 0, 0, 0, 1), Basis(1, 0, 0, 0, 0, -1, 0, 1, 0),
	Basis(0, 0, 1, 1, 0, 0, 0, 1, 0), Basis(-1, 0, 0, 0, 0, 1, 0, 1, 0),
	Basis(0, 0, -1, -1, 0, 0, 0, 1, 0), Basis(1, 0, 0, 0, -1, 0, 0, 0, -1),
	Basis(0, 1, 0, 1, 0, 0, 0, 0, -1), Basis(-1, 0, 0, 0, 1, 0, 0, 0, -1),
	Basis(0, -1, 0, -1, 0, 0, 0, 0, -1), Basis(1, 0, 0, 0, 0, 1, 0, -1, 0),
	Basis(0, 0, -1, 1, 0, 0, 0, -1, 0), Basis(-1, 0, 0, 0, 0, -1, 0, -1, 0),
	Basis(0, 0, 1, -1, 0, 0, 0, -1, 0), Basis(0, 0, 1, 0, 1, 0, -1, 0, 0),
	Basis(0, -1, 0, 0, 0, 1, -1, 0, 0), Basis(0, 0, -1, 0, -1, 0, -1, 0, 0),
	Basis(0, 1, 0, 0, 0, -1, -1, 0, 0), Basis(0, 0, 1, 0, -1, 0, 1, 0, 0),
	Basis(0, 1, 0, 0, 0, 1, 1, 0, 0), Basis(0, 0, -1, 0, 1, 0, 1, 0, 0),
	Basis(0, -1, 0, 0, 0, -1, 1, 0, 0)};

Basis GridMap::get_cell_item_basis(const Vector3i& p_position) const
{
	int orientation = get_cell_item_orientation(p_position);

	if (orientation == -1) {
		return Basis();
	}

	return get_basis_with_orthogonal_index(orientation);
}

Basis GridMap::get_basis_with_orthogonal_index(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, 24, Basis());

	return _ortho_bases[p_index];
}

int GridMap::get_orthogonal_index_from_basis(const Basis& p_basis) const
{
	Basis orth = p_basis;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			real_t v = orth[i][j];
			if (v > 0.5) {
				v = 1.0;
			}
			else if (v < -0.5) {
				v = -1.0;
			}
			else {
				v = 0;
			}

			orth[i][j] = v;
		}
	}

	for (int i = 0; i < 24; i++) {
		if (_ortho_bases[i] == orth) {
			return i;
		}
	}

	return 0;
}

GridMap::OctantKey GridMap::get_octant_key_from_index_key(const IndexKey& p_index_key) const
{
	const int x = p_index_key.x > 0 ? p_index_key.x / octant_size
									: (p_index_key.x - (octant_size - 1)) / octant_size;
	const int y = p_index_key.y > 0 ? p_index_key.y / octant_size
									: (p_index_key.y - (octant_size - 1)) / octant_size;
	const int z = p_index_key.z > 0 ? p_index_key.z / octant_size
									: (p_index_key.z - (octant_size - 1)) / octant_size;

	OctantKey ok;
	ok.key = 0;
	ok.x = x;
	ok.y = y;
	ok.z = z;
	return ok;
}

GridMap::OctantKey GridMap::get_octant_key_from_cell_coords(const Vector3i& p_cell_coords) const
{
	const int x = p_cell_coords.x > 0 ? p_cell_coords.x / octant_size
									  : (p_cell_coords.x - (octant_size - 1)) / octant_size;
	const int y = p_cell_coords.y > 0 ? p_cell_coords.y / octant_size
									  : (p_cell_coords.y - (octant_size - 1)) / octant_size;
	const int z = p_cell_coords.z > 0 ? p_cell_coords.z / octant_size
									  : (p_cell_coords.z - (octant_size - 1)) / octant_size;

	OctantKey ok;
	ok.key = 0;
	ok.x = x;
	ok.y = y;
	ok.z = z;
	return ok;
}

Vector3i GridMap::local_to_map(const Vector3& p_world_position) const
{
	Vector3 map_position = (p_world_position / cell_size).floor();
	return Vector3i(map_position);
}

Vector3 GridMap::map_to_local(const Vector3i& p_map_position) const
{
	Vector3 offset = _get_offset();
	Vector3 local_position(p_map_position.x * cell_size.x + offset.x,
		p_map_position.y * cell_size.y + offset.y, p_map_position.z * cell_size.z + offset.z);
	return local_position;
}

#ifndef PHYSICS_3D_DISABLED
void GridMap::_update_physics_bodies_collision_properties()
{
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		PhysicsServer3D::get_singleton()->body_set_collision_layer(
			E.value->static_body, collision_layer);
		PhysicsServer3D::get_singleton()->body_set_collision_mask(
			E.value->static_body, collision_mask);
		PhysicsServer3D::get_singleton()->body_set_collision_priority(
			E.value->static_body, collision_priority);
	}
}

#endif // PHYSICS_3D_DISABLED

void GridMap::_octant_clean_up(const OctantKey& p_key)
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
#ifndef PHYSICS_3D_DISABLED
	ERR_FAIL_NULL(PhysicsServer3D::get_singleton());
#endif // PHYSICS_3D_DISABLED
#ifndef NAVIGATION_3D_DISABLED
	ERR_FAIL_NULL(NavigationServer3D::get_singleton());
#endif // NAVIGATION_3D_DISABLED

	ERR_FAIL_COND(!octant_map.has(p_key));
	Octant& g = *octant_map[p_key];

#ifndef PHYSICS_3D_DISABLED
	if (g.collision_debug.is_valid()) {
		RS::get_singleton()->free_rid(g.collision_debug);
	}
	if (g.collision_debug_instance.is_valid()) {
		RS::get_singleton()->free_rid(g.collision_debug_instance);
	}

	PhysicsServer3D::get_singleton()->free_rid(g.static_body);
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	// Erase navigation
	for (const KeyValue<IndexKey, Octant::NavigationCell>& E : g.navigation_cell_ids) {
		if (E.value.region.is_valid()) {
			NavigationServer3D::get_singleton()->free_rid(E.value.region);
		}
		if (E.value.navigation_mesh_debug_instance.is_valid()) {
			RS::get_singleton()->free_rid(E.value.navigation_mesh_debug_instance);
		}
	}
	g.navigation_cell_ids.clear();
#endif // NAVIGATION_3D_DISABLED

#ifdef DEBUG_ENABLED
	if (bake_navigation) {
		if (g.navigation_debug_edge_connections_instance.is_valid()) {
			RenderingServer::get_singleton()->free_rid(
				g.navigation_debug_edge_connections_instance);
			g.navigation_debug_edge_connections_instance = RID();
		}
		if (g.navigation_debug_edge_connections_mesh.is_valid()) {
			g.navigation_debug_edge_connections_mesh.unref();
		}
	}

	_debug_clear_octants();
#endif // DEBUG_ENABLED

	// Erase multimeshes.

	for (int i = 0; i < g.multimesh_instances.size(); i++) {
		RS::get_singleton()->free_rid(g.multimesh_instances[i].instance);
		RS::get_singleton()->free_rid(g.multimesh_instances[i].multimesh);
	}
	g.multimesh_instances.clear();
}

void GridMap::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_WORLD: {
		last_transform = get_global_transform();

		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_enter_world(E.key);
		}

		for (int i = 0; i < baked_meshes.size(); i++) {
			RS::get_singleton()->instance_set_scenario(
				baked_meshes[i].instance, get_world_3d()->get_scenario());
			RS::get_singleton()->instance_set_transform(
				baked_meshes[i].instance, get_global_transform());
		}
	} break;

	case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
		_debug_update();

#ifndef NAVIGATION_3D_DISABLED
		if (bake_navigation &&
			NavigationServer3D::get_singleton()->get_debug_navigation_enabled()) {
			_update_navigation_debug_edge_connections();
		}
#endif // NAVIGATION_3D_DISABLED

#endif // DEBUG_ENABLED

		_update_visibility();
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_EXIT_TREE: {
		_debug_clear_octants();
	} break;
#endif

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Transform3D new_xform = get_global_transform();
		if (new_xform == last_transform) {
			break;
		}

		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_transform(E.key);
		}

		last_transform = new_xform;

		for (int i = 0; i < baked_meshes.size(); i++) {
			RS::get_singleton()->instance_set_transform(
				baked_meshes[i].instance, get_global_transform());
		}

#ifdef DEBUG_ENABLED
		for (const KeyValue<OctantKey, OctantDebug*>& E : debug_octant_map) {
			OctantKey octant_key = E.key;
			OctantDebug& octant_debug = *E.value;
			if (octant_debug.debug_line_instance_rid.is_valid()) {
				const Transform3D octant_transform =
					new_xform *
					(Transform3D(Basis(), Vector3(octant_key.x, octant_key.y, octant_key.z) *
											  octant_size * cell_size));
				RS::get_singleton()->instance_set_transform(
					octant_debug.debug_line_instance_rid, octant_transform);
			}
		}
#endif
	} break;

	case NOTIFICATION_EXIT_WORLD: {
		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_exit_world(E.key);
		}

		for (int i = 0; i < baked_meshes.size(); i++) {
			RS::get_singleton()->instance_set_scenario(baked_meshes[i].instance, RID());
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_visibility();
	} break;
	}
}

void GridMap::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	for (KeyValue<OctantKey, Octant*>& e : octant_map) {
		Octant* octant = e.value;
		for (int i = 0; i < octant->multimesh_instances.size(); i++) {
			const Octant::MultimeshInstance& mi = octant->multimesh_instances[i];
			RS::get_singleton()->instance_set_visible(mi.instance, is_visible_in_tree());
		}
	}

	for (int i = 0; i < baked_meshes.size(); i++) {
		RS::get_singleton()->instance_set_visible(baked_meshes[i].instance, is_visible_in_tree());
	}

#ifdef DEBUG_ENABLED
	for (const KeyValue<OctantKey, OctantDebug*>& E : debug_octant_map) {
		OctantDebug& octant_debug = *E.value;
		if (octant_debug.debug_line_instance_rid.is_valid()) {
			RS::get_singleton()->instance_set_visible(
				octant_debug.debug_line_instance_rid, is_visible_in_tree());
		}
	}
#endif
}

void GridMap::_recreate_octant_data()
{
	recreating_octants = true;

	HashMap<IndexKey, Cell, IndexKey> cell_copy(cell_map);
	_clear_internal();
	for (const KeyValue<IndexKey, Cell>& E : cell_copy) {
		set_cell_item(Vector3i(E.key), E.value.item, E.value.rot);
	}

	recreating_octants = false;
#ifdef DEBUG_ENABLED
	_debug_update();
#endif
}

void GridMap::_clear_internal()
{
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		if (is_inside_world()) {
			_octant_exit_world(E.key);
		}

		_octant_clean_up(E.key);
		memdelete(E.value);
	}

	octant_map.clear();
	cell_map.clear();
#ifdef DEBUG_ENABLED
	_debug_clear_octants();
	_debug_update();
#endif
}

void GridMap::clear()
{
	_clear_internal();
	clear_baked_meshes();
}

#ifndef DISABLE_DEPRECATED
void GridMap::resource_changed(const Ref<Resource>& p_res) {}
#endif

void GridMap::_update_octants_callback()
{
	if (!awaiting_update) {
		return;
	}

	LocalVector<OctantKey> to_delete;
	to_delete.reserve(octant_map.size());
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		if (_octant_update(E.key)) {
			to_delete.push_back(E.key);
		}
	}

	while (!to_delete.is_empty()) {
		const OctantKey& octantkey = to_delete[0];
		memdelete(octant_map[octantkey]);
		octant_map.erase(octantkey);
		to_delete.remove_at_unordered(0);
	}

	_update_visibility();
	awaiting_update = false;

#ifdef DEBUG_ENABLED
	_debug_update_octants();
#endif
}

void GridMap::_bind_methods() {}

void GridMap::set_cell_scale(float p_scale)
{
	cell_scale = p_scale;
	_recreate_octant_data();
}

float GridMap::get_cell_scale() const { return cell_scale; }

Vector3i GridMap::get_octant_coords_from_cell_coords(const Vector3i& p_cell_coords) const
{
	return Vector3i(p_cell_coords.x > 0 ? p_cell_coords.x / octant_size
										: (p_cell_coords.x - (octant_size - 1)) / octant_size,
		p_cell_coords.y > 0 ? p_cell_coords.y / octant_size
							: (p_cell_coords.y - (octant_size - 1)) / octant_size,
		p_cell_coords.z > 0 ? p_cell_coords.z / octant_size
							: (p_cell_coords.z - (octant_size - 1)) / octant_size);
}

LocalVector<GridMap::IndexKey> GridMap::get_index_keys_in_bounds(
	const AABB& p_bounds, bool p_used_only) const
{
	LocalVector<IndexKey> index_keys;
	if (!p_bounds.has_volume()) {
		return index_keys;
	}

	Vector3i cell_coords_start = (p_bounds.position / cell_size).floor();
	// -CMP_EPSILON because we don't want the octants that are just starting at the edge of the
	// bounds.
	Vector3i cell_coords_end =
		((p_bounds.get_end() - Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON)) / cell_size).floor();

	for (int z = cell_coords_start.z; z < cell_coords_end.z + 1; z++) {
		for (int y = cell_coords_start.y; y < cell_coords_end.y + 1; y++) {
			for (int x = cell_coords_start.x; x < cell_coords_end.x + 1; x++) {
				IndexKey index_key;
				index_key.x = x;
				index_key.y = y;
				index_key.z = z;

				if (p_used_only) {
					const HashMap<IndexKey, Cell, IndexKey>::ConstIterator index_key_kv =
						cell_map.find(index_key);

					if (index_key_kv) {
						index_keys.push_back(index_key);
					}
				}
				else {
					index_keys.push_back(index_key);
				}
			}
		}
	}

	return index_keys;
}

LocalVector<GridMap::OctantKey> GridMap::get_octant_keys_in_bounds(
	const AABB& p_bounds, bool p_used_only) const
{
	LocalVector<OctantKey> octant_keys;
	if (!p_bounds.has_volume()) {
		return octant_keys;
	}

	Vector3i cell_coords_start = (p_bounds.position / cell_size).floor();
	// -CMP_EPSILON because we don't want the octants that are just starting at the edge of the
	// bounds.
	Vector3i cell_coords_end =
		((p_bounds.get_end() - Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON)) / cell_size).floor();

	OctantKey octant_coords_start = get_octant_key_from_cell_coords(cell_coords_start);
	OctantKey octant_coords_end = get_octant_key_from_cell_coords(cell_coords_end);

	for (int z = octant_coords_start.z; z < octant_coords_end.z + 1; z++) {
		for (int y = octant_coords_start.y; y < octant_coords_end.y + 1; y++) {
			for (int x = octant_coords_start.x; x < octant_coords_end.x + 1; x++) {
				OctantKey octant_key;
				octant_key.x = x;
				octant_key.y = y;
				octant_key.z = z;

				if (p_used_only) {
					const HashMap<OctantKey, Octant*, OctantKey>::ConstIterator octant_kv =
						octant_map.find(octant_key);

					if (octant_kv) {
						Vector3i octant_coord(x, y, z);
						octant_keys.push_back(octant_key);
					}
				}
				else {
					octant_keys.push_back(octant_key);
				}
			}
		}
	}

	return octant_keys;
}

Vector3 GridMap::_get_offset() const
{
	return Vector3(cell_size.x * 0.5 * int(center_x), cell_size.y * 0.5 * int(center_y),
		cell_size.z * 0.5 * int(center_z));
}

void GridMap::clear_baked_meshes()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	for (int i = 0; i < baked_meshes.size(); i++) {
		RS::get_singleton()->free_rid(baked_meshes[i].instance);
	}
	baked_meshes.clear();

	_recreate_octant_data();
}

RID GridMap::get_bake_mesh_instance(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, baked_meshes.size(), RID());
	return baked_meshes[p_idx].instance;
}

void GridMap::set_debug_show_octants(bool p_enable)
{
#ifdef DEBUG_ENABLED
	if (debug_show_octants == p_enable) {
		return;
	}

	debug_show_octants = p_enable;
	_debug_update();
#endif
}

bool GridMap::get_debug_show_octants() const { return debug_show_octants; }

#if defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)
void GridMap::_update_navigation_debug_edge_connections()
{
	if (bake_navigation) {
		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_update_octant_navigation_debug_edge_connections_mesh(E.key);
		}
	}
}

void GridMap::_navigation_map_changed(RID p_map)
{
	if (bake_navigation && is_inside_tree() && p_map == get_world_3d()->get_navigation_map()) {
		_update_navigation_debug_edge_connections();
	}
}
#endif // defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)


