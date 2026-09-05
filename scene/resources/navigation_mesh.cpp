/**************************************************************************/
/*  navigation_mesh.cpp                                                   */
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

#include "navigation_mesh.h"

#ifdef DEBUG_ENABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // DEBUG_ENABLED

void NavigationMesh::create_from_mesh(const Ref<Mesh>& p_mesh)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_COND(p_mesh.is_null());

	vertices = Vector<Vector3>();
	polygons.clear();

	for (int i = 0; i < p_mesh->get_surface_count(); i++) {
		if (p_mesh->surface_get_primitive_type(i) != Mesh::PRIMITIVE_TRIANGLES) {
			WARN_PRINT(
				"A mesh surface was skipped when creating a NavigationMesh due to wrong primitive "
				"type in the source mesh. Mesh surface must be made out of triangles.");
			continue;
		}
		Array arr = p_mesh->surface_get_arrays(i);
		ERR_CONTINUE(arr.size() != Mesh::ARRAY_MAX);

		Vector<Vector3> varr = arr[Mesh::ARRAY_VERTEX];
		Vector<int> iarr = arr[Mesh::ARRAY_INDEX];
		if (varr.is_empty() || iarr.is_empty()) {
			WARN_PRINT("A mesh surface was skipped when creating a NavigationMesh due to an empty "
					   "vertex or index array.");
			continue;
		}

		int from = vertices.size();
		vertices.append_array(varr);
		int rlen = iarr.size();
		const int* r = iarr.ptr();

		Vector<int> polygon;
		for (int j = 0; j < rlen; j += 3) {
			polygon.resize(3);
			polygon.write[0] = r[j + 0] + from;
			polygon.write[1] = r[j + 1] + from;
			polygon.write[2] = r[j + 2] + from;
			polygons.push_back(polygon);
		}
	}
}

void NavigationMesh::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationMesh::SamplePartitionType NavigationMesh::get_sample_partition_type() const
{
	return partition_type;
}

void NavigationMesh::set_parsed_geometry_type(ParsedGeometryType p_value)
{
	ERR_FAIL_INDEX(p_value, PARSED_GEOMETRY_MAX);
	parsed_geometry_type = p_value;
	this->obj->notify_property_list_changed();
}

NavigationMesh::ParsedGeometryType NavigationMesh::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationMesh::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t NavigationMesh::get_collision_mask() const { return collision_mask; }

void NavigationMesh::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool NavigationMesh::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

void NavigationMesh::set_source_geometry_mode(SourceGeometryMode p_geometry_mode)
{
	ERR_FAIL_INDEX(p_geometry_mode, SOURCE_GEOMETRY_MAX);
	source_geometry_mode = p_geometry_mode;
	this->obj->notify_property_list_changed();
}

NavigationMesh::SourceGeometryMode NavigationMesh::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationMesh::set_source_group_name(const StringName& p_group_name)
{
	source_group_name = p_group_name;
}

StringName NavigationMesh::get_source_group_name() const { return source_group_name; }

void NavigationMesh::set_cell_size(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_size = p_value;
}

float NavigationMesh::get_cell_size() const { return cell_size; }

void NavigationMesh::set_cell_height(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_height = p_value;
}

float NavigationMesh::get_cell_height() const { return cell_height; }

void NavigationMesh::set_border_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	border_size = p_value;
}

float NavigationMesh::get_border_size() const { return border_size; }

void NavigationMesh::set_agent_height(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_height = p_value;
}

float NavigationMesh::get_agent_height() const { return agent_height; }

void NavigationMesh::set_agent_radius(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

float NavigationMesh::get_agent_radius() { return agent_radius; }

void NavigationMesh::set_agent_max_climb(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_max_climb = p_value;
}

float NavigationMesh::get_agent_max_climb() const { return agent_max_climb; }

void NavigationMesh::set_agent_max_slope(float p_value)
{
	ERR_FAIL_COND(p_value < 0 || p_value > 90);
	agent_max_slope = p_value;
}

float NavigationMesh::get_agent_max_slope() const { return agent_max_slope; }

void NavigationMesh::set_region_min_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_min_size = p_value;
}

float NavigationMesh::get_region_min_size() const { return region_min_size; }

void NavigationMesh::set_region_merge_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_merge_size = p_value;
}

float NavigationMesh::get_region_merge_size() const { return region_merge_size; }

void NavigationMesh::set_edge_max_length(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_length = p_value;
}

float NavigationMesh::get_edge_max_length() const { return edge_max_length; }

void NavigationMesh::set_edge_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_error = p_value;
}

float NavigationMesh::get_edge_max_error() const { return edge_max_error; }

void NavigationMesh::set_vertices_per_polygon(float p_value)
{
	ERR_FAIL_COND(p_value < 3);
	vertices_per_polygon = p_value;
}

float NavigationMesh::get_vertices_per_polygon() const { return vertices_per_polygon; }

void NavigationMesh::set_detail_sample_distance(float p_value)
{
	ERR_FAIL_COND(p_value < 0.1);
	detail_sample_distance = p_value;
}

float NavigationMesh::get_detail_sample_distance() const { return detail_sample_distance; }

void NavigationMesh::set_detail_sample_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	detail_sample_max_error = p_value;
}

float NavigationMesh::get_detail_sample_max_error() const { return detail_sample_max_error; }

void NavigationMesh::set_filter_low_hanging_obstacles(bool p_value)
{
	filter_low_hanging_obstacles = p_value;
}

bool NavigationMesh::get_filter_low_hanging_obstacles() const
{
	return filter_low_hanging_obstacles;
}

void NavigationMesh::set_filter_ledge_spans(bool p_value) { filter_ledge_spans = p_value; }

bool NavigationMesh::get_filter_ledge_spans() const { return filter_ledge_spans; }

void NavigationMesh::set_filter_walkable_low_height_spans(bool p_value)
{
	filter_walkable_low_height_spans = p_value;
}

bool NavigationMesh::get_filter_walkable_low_height_spans() const
{
	return filter_walkable_low_height_spans;
}

void NavigationMesh::set_filter_baking_aabb(const AABB& p_aabb)
{
	filter_baking_aabb = p_aabb;
	emit_changed();
}

AABB NavigationMesh::get_filter_baking_aabb() const { return filter_baking_aabb; }

void NavigationMesh::set_filter_baking_aabb_offset(const Vector3& p_aabb_offset)
{
	filter_baking_aabb_offset = p_aabb_offset;
	emit_changed();
}

Vector3 NavigationMesh::get_filter_baking_aabb_offset() const { return filter_baking_aabb_offset; }

void NavigationMesh::set_vertices(const Vector<Vector3>& p_vertices)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	this->obj->notify_property_list_changed();
}

Vector<Vector3> NavigationMesh::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

void NavigationMesh::_set_polygons(const Array& p_array)
{
	RWLockWrite write_lock(rwlock);
	polygons.resize(p_array.size());
	for (int i = 0; i < p_array.size(); i++) {
		polygons.write[i] = p_array[i];
	}
	this->obj->notify_property_list_changed();
}

Array NavigationMesh::_get_polygons() const
{
	RWLockRead read_lock(rwlock);
	Array ret;
	ret.resize(polygons.size());
	for (int i = 0; i < ret.size(); i++) {
		ret[i] = polygons[i];
	}

	return ret;
}

void NavigationMesh::set_polygons(const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	polygons = p_polygons;
	this->obj->notify_property_list_changed();
}

Vector<Vector<int>> NavigationMesh::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

void NavigationMesh::add_polygon(const Vector<int>& p_polygon)
{
	RWLockWrite write_lock(rwlock);
	polygons.push_back(p_polygon);
	this->obj->notify_property_list_changed();
}

int NavigationMesh::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationMesh::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationMesh::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
}

void NavigationMesh::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
}

void NavigationMesh::set_data(
	const Vector<Vector3>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
}

void NavigationMesh::get_data(Vector<Vector3>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}

#ifdef DEBUG_ENABLED
Ref<ArrayMesh> NavigationMesh::get_debug_mesh()
{
	if (debug_mesh.is_valid()) {
		// Blocks further updates for now, code below is intended for dynamic updates e.g. when
		// settings change.
		return debug_mesh;
	}

	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
	else {
		debug_mesh->clear_surfaces();
	}

	if (vertices.is_empty()) {
		return debug_mesh;
	}

	RWLockRead read_lock(rwlock);

	int polygon_count = get_polygon_count();

	if (polygon_count < 1) {
		// no face, no play
		return debug_mesh;
	}

	// build geometry face surface
	Vector<Vector3> face_vertex_array;
	face_vertex_array.resize(polygon_count * 3);

	for (int i = 0; i < polygon_count; i++) {
		Vector<int> polygon = get_polygon(i);

		face_vertex_array.push_back(vertices[polygon[0]]);
		face_vertex_array.push_back(vertices[polygon[1]]);
		face_vertex_array.push_back(vertices[polygon[2]]);
	}

	Array face_mesh_array;
	face_mesh_array.resize(Mesh::ARRAY_MAX);
	face_mesh_array[Mesh::ARRAY_VERTEX] = face_vertex_array;

	// if enabled add vertex colors to colorize each face individually
	bool enabled_geometry_face_random_color =
		NavigationServer3D::get_singleton()
			->get_debug_navigation_enable_geometry_face_random_color();
	if (enabled_geometry_face_random_color) {
		Color debug_navigation_geometry_face_color =
			NavigationServer3D::get_singleton()->get_debug_navigation_geometry_face_color();
		Color polygon_color = debug_navigation_geometry_face_color;

		Vector<Color> face_color_array;
		face_color_array.resize(polygon_count * 3);

		for (int i = 0; i < polygon_count; i++) {
			polygon_color = debug_navigation_geometry_face_color *
							(Color(Math::randf(), Math::randf(), Math::randf()));

			face_color_array.push_back(polygon_color);
			face_color_array.push_back(polygon_color);
			face_color_array.push_back(polygon_color);
		}
		face_mesh_array[Mesh::ARRAY_COLOR] = face_color_array;
	}

	debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, face_mesh_array);
	Ref<StandardMaterial3D> debug_geometry_face_material =
		NavigationServer3D::get_singleton()->get_debug_navigation_geometry_face_material();
	debug_mesh->surface_set_material(0, debug_geometry_face_material);

	// if enabled build geometry edge line surface
	bool enabled_edge_lines =
		NavigationServer3D::get_singleton()->get_debug_navigation_enable_edge_lines();

	if (enabled_edge_lines) {
		Vector<Vector3> line_vertex_array;
		line_vertex_array.resize(polygon_count * 6);

		for (int i = 0; i < polygon_count; i++) {
			Vector<int> polygon = get_polygon(i);

			line_vertex_array.push_back(vertices[polygon[0]]);
			line_vertex_array.push_back(vertices[polygon[1]]);
			line_vertex_array.push_back(vertices[polygon[1]]);
			line_vertex_array.push_back(vertices[polygon[2]]);
			line_vertex_array.push_back(vertices[polygon[2]]);
			line_vertex_array.push_back(vertices[polygon[0]]);
		}

		Array line_mesh_array;
		line_mesh_array.resize(Mesh::ARRAY_MAX);
		line_mesh_array[Mesh::ARRAY_VERTEX] = line_vertex_array;
		debug_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, line_mesh_array);
		Ref<StandardMaterial3D> debug_geometry_edge_material =
			NavigationServer3D::get_singleton()->get_debug_navigation_geometry_edge_material();
		debug_mesh->surface_set_material(1, debug_geometry_edge_material);
	}

	return debug_mesh;
}
#endif // DEBUG_ENABLED

void NavigationMesh::_bind_methods() {}

void NavigationMesh::_validate_property(PropertyInfo& p_property) const
{
	if (p_property.name == "geometry_collision_mask") {
		if (parsed_geometry_type == PARSED_GEOMETRY_MESH_INSTANCES) {
			p_property.usage = PROPERTY_USAGE_NONE;
			return;
		}
	}
	else if (p_property.name == "geometry_source_group_name") {
		if (source_geometry_mode == SOURCE_GEOMETRY_ROOT_NODE_CHILDREN) {
			p_property.usage = PROPERTY_USAGE_NONE;
			return;
		}
	}
}

#ifndef DISABLE_DEPRECATED
bool NavigationMesh::_set(const StringName& p_name, const Variant& p_value)
{
	if (p_name == "polygon_verts_per_poly") { // Renamed in 4.0 beta 9.
		set_vertices_per_polygon(p_value);
		return true;
	}
	return false;
}

bool NavigationMesh::_get(const StringName& p_name, Variant& r_ret) const
{
	if (p_name == "polygon_verts_per_poly") { // Renamed in 4.0 beta 9.
		r_ret = get_vertices_per_polygon();
		return true;
	}
	return false;
}
#endif // DISABLE_DEPRECATED


