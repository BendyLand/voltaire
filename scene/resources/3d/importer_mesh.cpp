/**************************************************************************/
/*  importer_mesh.cpp                                                     */
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

#include "core/io/marshalls.h"
#include "core/math/random_pcg.h"
#include "importer_mesh.h"
#include "scene/resources/surface_tool.h"

#ifndef PHYSICS_3D_DISABLED
#include "core/math/convex_hull.h"
#endif // PHYSICS_3D_DISABLED

#include <cfloat> // FLT_EPSILON

String ImporterMesh::validate_blend_shape_name(const String& p_name)
{
	return p_name.replace_char(':', '_');
}

void ImporterMesh::add_blend_shape(const String& p_name)
{
	ERR_FAIL_COND(surfaces.size() > 0);
	blend_shapes.push_back(validate_blend_shape_name(p_name));
}

int ImporterMesh::get_blend_shape_count() const { return blend_shapes.size(); }

String ImporterMesh::get_blend_shape_name(int p_blend_shape) const
{
	ERR_FAIL_INDEX_V(p_blend_shape, blend_shapes.size(), String());
	return blend_shapes[p_blend_shape];
}

void ImporterMesh::set_blend_shape_mode(Mesh::BlendShapeMode p_blend_shape_mode)
{
	blend_shape_mode = p_blend_shape_mode;
}

Mesh::BlendShapeMode ImporterMesh::get_blend_shape_mode() const { return blend_shape_mode; }

int ImporterMesh::get_surface_count() const { return surfaces.size(); }

Mesh::PrimitiveType ImporterMesh::get_surface_primitive_type(int p_surface)
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Mesh::PRIMITIVE_MAX);
	return surfaces[p_surface].primitive;
}

String ImporterMesh::get_surface_name(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}

void ImporterMesh::set_surface_name(int p_surface, const String& p_name)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
	mesh.unref();
}

int ImporterMesh::get_surface_lod_count(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	return surfaces[p_surface].lods.size();
}

Vector<int> ImporterMesh::get_surface_lod_indices(int p_surface, int p_lod) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Vector<int>());
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), Vector<int>());

	return surfaces[p_surface].lods[p_lod].indices;
}

float ImporterMesh::get_surface_lod_size(int p_surface, int p_lod) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), 0);
	return surfaces[p_surface].lods[p_lod].distance;
}

uint64_t ImporterMesh::get_surface_format(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	return surfaces[p_surface].flags;
}

Ref<Material> ImporterMesh::get_surface_material(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Ref<Material>());
	return surfaces[p_surface].material;
}

void ImporterMesh::set_surface_material(int p_surface, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].material = p_material;
	mesh.unref();
}

template <typename T>
static Vector<T> _remap_array(
	Vector<T> p_array, const Vector<uint32_t>& p_remap, uint32_t p_vertex_count)
{
	ERR_FAIL_COND_V(p_array.size() % p_remap.size() != 0, p_array);
	int num_elements = p_array.size() / p_remap.size();
	T* data = p_array.ptrw();
	SurfaceTool::remap_vertex_func(
		data, data, p_remap.size(), sizeof(T) * num_elements, p_remap.ptr());
	p_array.resize(p_vertex_count * num_elements);
	return p_array;
}

#define VERTEX_SKIN_FUNC(                                                                          \
	bone_count, vert_idx, read_array, write_array, transform_array, bone_array, weight_array)      \
	Vector3 transformed_vert;                                                                      \
	for (unsigned int weight_idx = 0; weight_idx < bone_count; weight_idx++) {                     \
		int bone_idx = bone_array[vert_idx * bone_count + weight_idx];                             \
		float w = weight_array[vert_idx * bone_count + weight_idx];                                \
		if (w < FLT_EPSILON) {                                                                     \
			continue;                                                                              \
		}                                                                                          \
		ERR_FAIL_INDEX(bone_idx, static_cast<int>(transform_array.size()));                        \
		transformed_vert += transform_array[bone_idx].xform(read_array[vert_idx]) * w;             \
	}                                                                                              \
	write_array[vert_idx] = transformed_vert;

bool ImporterMesh::has_mesh() const { return mesh.is_valid(); }

void ImporterMesh::clear()
{
	surfaces.clear();
	blend_shapes.clear();
	mesh.unref();
}

Ref<ImporterMesh> ImporterMesh::get_shadow_mesh() const { return shadow_mesh; }

#ifndef PHYSICS_3D_DISABLED
Vector<Ref<Shape3D>> ImporterMesh::convex_decompose(
	const Ref<MeshConvexDecompositionSettings>& p_settings) const
{
	ERR_FAIL_NULL_V(Mesh::convex_decomposition_function, Vector<Ref<Shape3D>>());

	const Vector<Face3> faces = get_faces();
	int face_count = faces.size();

	Vector<Vector3> vertices;
	uint32_t vertex_count = 0;
	vertices.resize(face_count * 3);
	Vector<uint32_t> indices;
	indices.resize(face_count * 3);
	{
		HashMap<Vector3, uint32_t> vertex_map;
		Vector3* vertex_w = vertices.ptrw();
		uint32_t* index_w = indices.ptrw();
		for (int i = 0; i < face_count; i++) {
			for (int j = 0; j < 3; j++) {
				const Vector3& vertex = faces[i].vertex[j];
				HashMap<Vector3, uint32_t>::Iterator found_vertex = vertex_map.find(vertex);
				uint32_t index;
				if (found_vertex) {
					index = found_vertex->value;
				}
				else {
					index = vertex_count++;
					vertex_map[vertex] = index;
					vertex_w[index] = vertex;
				}
				index_w[i * 3 + j] = index;
			}
		}
	}
	vertices.resize(vertex_count);

	Vector<Vector<Vector3>> decomposed = Mesh::convex_decomposition_function(
		(real_t*)vertices.ptr(), vertex_count, indices.ptr(), face_count, p_settings, nullptr);

	Vector<Ref<Shape3D>> ret;

	for (int i = 0; i < decomposed.size(); i++) {
		Ref<ConvexPolygonShape3D> shape;
		shape.instantiate();
		shape->set_points(decomposed[i]);
		ret.push_back(shape);
	}

	return ret;
}

Ref<ConcavePolygonShape3D> ImporterMesh::create_trimesh_shape() const
{
	Vector<Face3> faces = get_faces();
	if (faces.is_empty()) {
		return Ref<ConcavePolygonShape3D>();
	}

	Vector<Vector3> face_points;
	face_points.resize(faces.size() * 3);

	for (int i = 0; i < face_points.size(); i += 3) {
		Face3 f = faces.get(i / 3);
		face_points.set(i, f.vertex[0]);
		face_points.set(i + 1, f.vertex[1]);
		face_points.set(i + 2, f.vertex[2]);
	}

	Ref<ConcavePolygonShape3D> shape = memnew(ConcavePolygonShape3D);
	shape->set_faces(face_points);
	return shape;
}
#endif // PHYSICS_3D_DISABLED

Ref<NavigationMesh> ImporterMesh::create_navigation_mesh()
{
	Vector<Face3> faces = get_faces();
	if (faces.is_empty()) {
		return Ref<NavigationMesh>();
	}

	HashMap<Vector3, int> unique_vertices;
	Vector<Vector<int>> face_polygons;
	face_polygons.resize(faces.size());

	for (int i = 0; i < faces.size(); i++) {
		Vector<int> face_indices;
		face_indices.resize(3);
		for (int j = 0; j < 3; j++) {
			Vector3 v = faces[i].vertex[j];
			int idx;
			if (unique_vertices.has(v)) {
				idx = unique_vertices[v];
			}
			else {
				idx = unique_vertices.size();
				unique_vertices[v] = idx;
			}
			face_indices.write[j] = idx;
		}
		face_polygons.write[i] = face_indices;
	}

	Vector<Vector3> vertices;
	vertices.resize(unique_vertices.size());
	for (const KeyValue<Vector3, int>& E : unique_vertices) {
		vertices.write[E.value] = E.key;
	}

	Ref<NavigationMesh> nm;
	nm.instantiate();
	nm->set_data(vertices, face_polygons);

	return nm;
}

extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float* p_vertices,
	const float* p_normals, int p_vertex_count, const int* p_indices, int p_index_count,
	const uint8_t* p_cache_data, bool* r_use_cache, uint8_t** r_mesh_cache, int* r_mesh_cache_size,
	float** r_uv, int** r_vertex, int* r_vertex_count, int** r_index, int* r_index_count,
	int* r_size_hint_x, int* r_size_hint_y);

struct EditorSceneFormatImporterMeshLightmapSurface
{
	Ref<Material> material;
	LocalVector<SurfaceTool::Vertex> vertices;
	Mesh::PrimitiveType primitive = Mesh::PrimitiveType::PRIMITIVE_MAX;
	uint64_t format = 0;
	String name;
};

static const uint32_t custom_shift[RSE::ARRAY_CUSTOM_COUNT] = {Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT,
	Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT, Mesh::ARRAY_FORMAT_CUSTOM2_SHIFT,
	Mesh::ARRAY_FORMAT_CUSTOM3_SHIFT};

void ImporterMesh::set_lightmap_size_hint(const Size2i& p_size) { lightmap_size_hint = p_size; }

Size2i ImporterMesh::get_lightmap_size_hint() const { return lightmap_size_hint; }


