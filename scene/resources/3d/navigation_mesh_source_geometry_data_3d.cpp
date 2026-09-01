/**************************************************************************/
/*  navigation_mesh_source_geometry_data_3d.cpp                           */
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
#include "navigation_mesh_source_geometry_data_3d.h"

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
	if (!Engine::get_singleton()->is_editor_hint()) {
		WARN_PRINT_ONCE(
			"Source geometry parsing for navigation mesh baking had to parse RenderingServer meshes at runtime.\n\
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


