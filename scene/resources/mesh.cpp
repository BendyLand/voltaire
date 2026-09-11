/**************************************************************************/
/*  mesh.cpp                                                              */
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

#include "core/math/convex_hull.h"
#include "core/templates/pair.h"
#include "mesh.h"
#include "scene/resources/surface_tool.h"
#include "servers/rendering/rendering_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#endif // PHYSICS_3D_DISABLED

void MeshConvexDecompositionSettings::set_max_concavity(real_t p_max_concavity)
{
	max_concavity = CLAMP(p_max_concavity, 0.001, 1.0);
}

real_t MeshConvexDecompositionSettings::get_max_concavity() const { return max_concavity; }

void MeshConvexDecompositionSettings::set_symmetry_planes_clipping_bias(
	real_t p_symmetry_planes_clipping_bias)
{
	symmetry_planes_clipping_bias = CLAMP(p_symmetry_planes_clipping_bias, 0.0, 1.0);
}

real_t MeshConvexDecompositionSettings::get_symmetry_planes_clipping_bias() const
{
	return symmetry_planes_clipping_bias;
}

void MeshConvexDecompositionSettings::set_revolution_axes_clipping_bias(
	real_t p_revolution_axes_clipping_bias)
{
	revolution_axes_clipping_bias = CLAMP(p_revolution_axes_clipping_bias, 0.0, 1.0);
}

real_t MeshConvexDecompositionSettings::get_revolution_axes_clipping_bias() const
{
	return revolution_axes_clipping_bias;
}

void MeshConvexDecompositionSettings::set_min_volume_per_convex_hull(
	real_t p_min_volume_per_convex_hull)
{
	min_volume_per_convex_hull = CLAMP(p_min_volume_per_convex_hull, 0.0001, 0.01);
}

real_t MeshConvexDecompositionSettings::get_min_volume_per_convex_hull() const
{
	return min_volume_per_convex_hull;
}

void MeshConvexDecompositionSettings::set_resolution(uint32_t p_resolution)
{
	resolution = p_resolution < 10'000 ? 10'000 : (p_resolution > 100'000 ? 100'000 : p_resolution);
}

uint32_t MeshConvexDecompositionSettings::get_resolution() const { return resolution; }

void MeshConvexDecompositionSettings::set_max_num_vertices_per_convex_hull(
	uint32_t p_max_num_vertices_per_convex_hull)
{
	max_num_vertices_per_convex_hull =
		p_max_num_vertices_per_convex_hull < 4
			? 4
			: (p_max_num_vertices_per_convex_hull > 1024 ? 1024
														 : p_max_num_vertices_per_convex_hull);
}

uint32_t MeshConvexDecompositionSettings::get_max_num_vertices_per_convex_hull() const
{
	return max_num_vertices_per_convex_hull;
}

void MeshConvexDecompositionSettings::set_plane_downsampling(uint32_t p_plane_downsampling)
{
	plane_downsampling =
		p_plane_downsampling < 1 ? 1 : (p_plane_downsampling > 16 ? 16 : p_plane_downsampling);
}

uint32_t MeshConvexDecompositionSettings::get_plane_downsampling() const
{
	return plane_downsampling;
}

void MeshConvexDecompositionSettings::set_convex_hull_downsampling(
	uint32_t p_convex_hull_downsampling)
{
	convex_hull_downsampling =
		p_convex_hull_downsampling < 1
			? 1
			: (p_convex_hull_downsampling > 16 ? 16 : p_convex_hull_downsampling);
}

uint32_t MeshConvexDecompositionSettings::get_convex_hull_downsampling() const
{
	return convex_hull_downsampling;
}

void MeshConvexDecompositionSettings::set_normalize_mesh(bool p_normalize_mesh)
{
	normalize_mesh = p_normalize_mesh;
}

bool MeshConvexDecompositionSettings::get_normalize_mesh() const { return normalize_mesh; }

void MeshConvexDecompositionSettings::set_mode(Mode p_mode) { mode = p_mode; }

MeshConvexDecompositionSettings::Mode MeshConvexDecompositionSettings::get_mode() const
{
	return mode;
}

void MeshConvexDecompositionSettings::set_convex_hull_approximation(
	bool p_convex_hull_approximation)
{
	convex_hull_approximation = p_convex_hull_approximation;
}

bool MeshConvexDecompositionSettings::get_convex_hull_approximation() const
{
	return convex_hull_approximation;
}

void MeshConvexDecompositionSettings::set_max_convex_hulls(uint32_t p_max_convex_hulls)
{
	max_convex_hulls =
		p_max_convex_hulls < 1 ? 1 : (p_max_convex_hulls > 32 ? 32 : p_max_convex_hulls);
}

uint32_t MeshConvexDecompositionSettings::get_max_convex_hulls() const { return max_convex_hulls; }

void MeshConvexDecompositionSettings::set_project_hull_vertices(bool p_project_hull_vertices)
{
	project_hull_vertices = p_project_hull_vertices;
}

bool MeshConvexDecompositionSettings::get_project_hull_vertices() const
{
	return project_hull_vertices;
}

void MeshConvexDecompositionSettings::_bind_methods() {}

#ifndef PHYSICS_3D_DISABLED
Mesh::ConvexDecompositionFunc Mesh::convex_decomposition_function = nullptr;
#endif // PHYSICS_3D_DISABLED

void Mesh::generate_debug_mesh_lines(Vector<Vector3>& r_lines)
{
	if (debug_lines.size() > 0) {
		r_lines = debug_lines;
		return;
	}

	Ref<TriangleMesh> tm = generate_triangle_mesh();
	if (tm.is_null()) {
		return;
	}

	Vector<int> triangle_indices;
	tm->get_indices(&triangle_indices);
	const int triangles_num = tm->get_triangles().size();
	Vector<Vector3> vertices = tm->get_vertices();

	debug_lines.resize(tm->get_triangles().size() * 6); // 3 lines x 2 points each line

	const int* ind_r = triangle_indices.ptr();
	const Vector3* ver_r = vertices.ptr();
	for (int j = 0, x = 0, i = 0; i < triangles_num; j += 6, x += 3, ++i) {
		// Triangle line 1
		debug_lines.write[j + 0] = ver_r[ind_r[x + 0]];
		debug_lines.write[j + 1] = ver_r[ind_r[x + 1]];

		// Triangle line 2
		debug_lines.write[j + 2] = ver_r[ind_r[x + 1]];
		debug_lines.write[j + 3] = ver_r[ind_r[x + 2]];

		// Triangle line 3
		debug_lines.write[j + 4] = ver_r[ind_r[x + 2]];
		debug_lines.write[j + 5] = ver_r[ind_r[x + 0]];
	}

	r_lines = debug_lines;
}

void Mesh::generate_debug_mesh_indices(Vector<Vector3>& r_points)
{
	Ref<TriangleMesh> tm = generate_triangle_mesh();
	if (tm.is_null()) {
		return;
	}

	Vector<Vector3> vertices = tm->get_vertices();

	int vertices_size = vertices.size();
	r_points.resize(vertices_size);
	for (int i = 0; i < vertices_size; ++i) {
		r_points.write[i] = vertices[i];
	}
}

Vector<Face3> Mesh::get_faces() const
{
	Ref<TriangleMesh> tm = generate_triangle_mesh();
	if (tm.is_valid()) {
		return tm->get_faces();
	}
	return Vector<Face3>();
}

Vector<Face3> Mesh::get_surface_faces(int p_surface) const
{
	Ref<TriangleMesh> tm = generate_surface_triangle_mesh(p_surface);
	if (tm.is_valid()) {
		return tm->get_faces();
	}
	return Vector<Face3>();
}

#ifndef PHYSICS_3D_DISABLED

Ref<ConcavePolygonShape3D> Mesh::create_trimesh_shape() const
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

void Mesh::set_lightmap_size_hint(const Size2i& p_size) { lightmap_size_hint = p_size; }

Size2i Mesh::get_lightmap_size_hint() const { return lightmap_size_hint; }

Ref<Resource> Mesh::create_placeholder() const
{
	Ref<PlaceholderMesh> placeholder;
	placeholder.instantiate();
	placeholder->set_aabb(get_aabb());
	return placeholder;
}

void Mesh::_bind_methods() {}

void Mesh::clear_cache() const
{
	triangle_mesh.unref();
	debug_lines.clear();
}

#ifndef PHYSICS_3D_DISABLED
Vector<Ref<Shape3D>> Mesh::convex_decompose(
	const Ref<MeshConvexDecompositionSettings>& p_settings) const
{
	ERR_FAIL_NULL_V(convex_decomposition_function, Vector<Ref<Shape3D>>());

	Ref<TriangleMesh> tm = generate_triangle_mesh();
	ERR_FAIL_COND_V(tm.is_null(), Vector<Ref<Shape3D>>());

	const Vector<TriangleMesh::Triangle>& triangles = tm->get_triangles();
	int triangle_count = triangles.size();

	Vector<uint32_t> indices;
	{
		indices.resize(triangle_count * 3);
		uint32_t* w = indices.ptrw();
		for (int i = 0; i < triangle_count; i++) {
			for (int j = 0; j < 3; j++) {
				w[i * 3 + j] = triangles[i].indices[j];
			}
		}
	}

	const Vector<Vector3>& vertices = tm->get_vertices();
	int vertex_count = vertices.size();

	Vector<Vector<Vector3>> decomposed = convex_decomposition_function(
		(real_t*)vertices.ptr(), vertex_count, indices.ptr(), triangle_count, p_settings, nullptr);

	Vector<Ref<Shape3D>> ret;

	for (int i = 0; i < decomposed.size(); i++) {
		Ref<ConvexPolygonShape3D> shape;
		shape.instantiate();
		shape->set_points(decomposed[i]);
		ret.push_back(shape);
	}

	return ret;
}
#endif // PHYSICS_3D_DISABLED

int Mesh::get_builtin_bind_pose_count() const { return 0; }

Transform3D Mesh::get_builtin_bind_pose(int p_index) const { return Transform3D(); }

Mesh::Mesh() {}

enum OldArrayType
{
	OLD_ARRAY_VERTEX,
	OLD_ARRAY_NORMAL,
	OLD_ARRAY_TANGENT,
	OLD_ARRAY_COLOR,
	OLD_ARRAY_TEX_UV,
	OLD_ARRAY_TEX_UV2,
	OLD_ARRAY_BONES,
	OLD_ARRAY_WEIGHTS,
	OLD_ARRAY_INDEX,
	OLD_ARRAY_MAX,
};

enum OldArrayFormat
{
	/* OLD_ARRAY FORMAT FLAGS */
	OLD_ARRAY_FORMAT_VERTEX = 1 << OLD_ARRAY_VERTEX, // mandatory
	OLD_ARRAY_FORMAT_NORMAL = 1 << OLD_ARRAY_NORMAL,
	OLD_ARRAY_FORMAT_TANGENT = 1 << OLD_ARRAY_TANGENT,
	OLD_ARRAY_FORMAT_COLOR = 1 << OLD_ARRAY_COLOR,
	OLD_ARRAY_FORMAT_TEX_UV = 1 << OLD_ARRAY_TEX_UV,
	OLD_ARRAY_FORMAT_TEX_UV2 = 1 << OLD_ARRAY_TEX_UV2,
	OLD_ARRAY_FORMAT_BONES = 1 << OLD_ARRAY_BONES,
	OLD_ARRAY_FORMAT_WEIGHTS = 1 << OLD_ARRAY_WEIGHTS,
	OLD_ARRAY_FORMAT_INDEX = 1 << OLD_ARRAY_INDEX,

	OLD_ARRAY_COMPRESS_BASE = (OLD_ARRAY_INDEX + 1),
	OLD_ARRAY_COMPRESS_VERTEX =
		1 << (OLD_ARRAY_VERTEX + (int32_t)OLD_ARRAY_COMPRESS_BASE), // mandatory
	OLD_ARRAY_COMPRESS_NORMAL = 1 << (OLD_ARRAY_NORMAL + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_TANGENT = 1 << (OLD_ARRAY_TANGENT + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_COLOR = 1 << (OLD_ARRAY_COLOR + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_TEX_UV = 1 << (OLD_ARRAY_TEX_UV + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_TEX_UV2 = 1 << (OLD_ARRAY_TEX_UV2 + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_BONES = 1 << (OLD_ARRAY_BONES + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_WEIGHTS = 1 << (OLD_ARRAY_WEIGHTS + (int32_t)OLD_ARRAY_COMPRESS_BASE),
	OLD_ARRAY_COMPRESS_INDEX = 1 << (OLD_ARRAY_INDEX + (int32_t)OLD_ARRAY_COMPRESS_BASE),

	OLD_ARRAY_FLAG_USE_2D_VERTICES = OLD_ARRAY_COMPRESS_INDEX << 1,
	OLD_ARRAY_FLAG_USE_16_BIT_BONES = OLD_ARRAY_COMPRESS_INDEX << 2,
	OLD_ARRAY_FLAG_USE_DYNAMIC_UPDATE = OLD_ARRAY_COMPRESS_INDEX << 3,
	OLD_ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION = OLD_ARRAY_COMPRESS_INDEX << 4,
};

#ifndef DISABLE_DEPRECATED

static Mesh::PrimitiveType _old_primitives[7] = {Mesh::PRIMITIVE_POINTS, Mesh::PRIMITIVE_LINES,
	Mesh::PRIMITIVE_LINE_STRIP, Mesh::PRIMITIVE_LINES, Mesh::PRIMITIVE_TRIANGLES,
	Mesh::PRIMITIVE_TRIANGLE_STRIP, Mesh::PRIMITIVE_TRIANGLE_STRIP};
#endif // DISABLE_DEPRECATED

void _fix_array_compatibility(const Vector<uint8_t>& p_src, uint64_t p_old_format,
	uint64_t p_new_format, uint32_t p_elements, Vector<uint8_t>& vertex_data,
	Vector<uint8_t>& attribute_data, Vector<uint8_t>& skin_data)
{
	uint32_t dst_vertex_stride;
	uint32_t dst_normal_tangent_stride;
	uint32_t dst_attribute_stride;
	uint32_t dst_skin_stride;
	uint32_t dst_offsets[Mesh::ARRAY_MAX];
	RenderingServer::get_singleton()->mesh_surface_make_offsets_from_format(
		p_new_format & (~RSE::ARRAY_FORMAT_INDEX), p_elements, 0, dst_offsets, dst_vertex_stride,
		dst_normal_tangent_stride, dst_attribute_stride, dst_skin_stride);

	vertex_data.resize((dst_vertex_stride + dst_normal_tangent_stride) * p_elements);
	attribute_data.resize(dst_attribute_stride * p_elements);
	skin_data.resize(dst_skin_stride * p_elements);

	uint8_t* dst_vertex_ptr = vertex_data.ptrw();
	uint8_t* dst_attribute_ptr = attribute_data.ptrw();
	uint8_t* dst_skin_ptr = skin_data.ptrw();

	const uint8_t* src_vertex_ptr = p_src.ptr();
	uint32_t src_vertex_stride = p_src.size() / p_elements;

	uint32_t src_offset = 0;
	for (uint32_t j = 0; j < OLD_ARRAY_INDEX; j++) {
		if (!(p_old_format & (1ULL << j))) {
			continue;
		}
		switch (j) {
		case OLD_ARRAY_VERTEX: {
			if (p_old_format & OLD_ARRAY_FLAG_USE_2D_VERTICES) {
				if (p_old_format & OLD_ARRAY_COMPRESS_VERTEX) {
					for (uint32_t i = 0; i < p_elements; i++) {
						const uint16_t* src =
							(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride];
						float* dst = (float*)&dst_vertex_ptr[i * dst_vertex_stride];
						dst[0] = Math::half_to_float(src[0]);
						dst[1] = Math::half_to_float(src[1]);
					}
					src_offset += sizeof(uint16_t) * 2;
				}
				else {
					for (uint32_t i = 0; i < p_elements; i++) {
						const float* src = (const float*)&src_vertex_ptr[i * src_vertex_stride];
						float* dst = (float*)&dst_vertex_ptr[i * dst_vertex_stride];
						dst[0] = src[0];
						dst[1] = src[1];
					}
					src_offset += sizeof(float) * 2;
				}
			}
			else {
				if (p_old_format & OLD_ARRAY_COMPRESS_VERTEX) {
					for (uint32_t i = 0; i < p_elements; i++) {
						const uint16_t* src =
							(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride];
						float* dst = (float*)&dst_vertex_ptr[i * dst_vertex_stride];
						dst[0] = Math::half_to_float(src[0]);
						dst[1] = Math::half_to_float(src[1]);
						dst[2] = Math::half_to_float(src[2]);
					}
					src_offset += sizeof(uint16_t) * 4; //+pad
				}
				else {
					for (uint32_t i = 0; i < p_elements; i++) {
						const float* src = (const float*)&src_vertex_ptr[i * src_vertex_stride];
						float* dst = (float*)&dst_vertex_ptr[i * dst_vertex_stride];
						dst[0] = src[0];
						dst[1] = src[1];
						dst[2] = src[2];
					}
					src_offset += sizeof(float) * 3;
				}
			}
		} break;
		case OLD_ARRAY_NORMAL: {
			if (p_old_format & OLD_ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
				if ((p_old_format & OLD_ARRAY_COMPRESS_NORMAL) &&
					(p_old_format & OLD_ARRAY_FORMAT_TANGENT) &&
					(p_old_format & OLD_ARRAY_COMPRESS_TANGENT)) {
					for (uint32_t i = 0; i < p_elements; i++) {
						const int8_t* src =
							(const int8_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						uint16_t* dst = (uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
																   dst_offsets[Mesh::ARRAY_NORMAL]];

						// 4.x requires biasing the octahedron components to a 0.0 <-> 1.0 range,
						// whereas in 3.x they were stored in the -1.0 <-> 1.0 range
						dst[0] = (uint16_t)CLAMP((src[0] / 127.0f * .5f + .5f) * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP((src[1] / 127.0f * .5f + .5f) * 65535, 0, 65535);
					}
					src_offset += sizeof(int8_t) * 2;
				}
				else {
					for (uint32_t i = 0; i < p_elements; i++) {
						const int16_t* src =
							(const int16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						uint16_t* dst = (uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
																   dst_offsets[Mesh::ARRAY_NORMAL]];

						dst[0] = (uint16_t)CLAMP((src[0] / 32767.0f * .5f + .5f) * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP((src[1] / 32767.0f * .5f + .5f) * 65535, 0, 65535);
					}
					src_offset += sizeof(int16_t) * 2;
				}
			}
			else { // No Octahedral compression
				if (p_old_format & OLD_ARRAY_COMPRESS_NORMAL) {
					for (uint32_t i = 0; i < p_elements; i++) {
						const int8_t* src =
							(const int8_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						const Vector3 original_normal(
							src[0] / 127.0f, src[1] / 127.0f, src[2] / 127.0f);
						Vector2 res = original_normal.octahedron_encode();

						uint16_t* dst = (uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
																   dst_offsets[Mesh::ARRAY_NORMAL]];
						dst[0] = (uint16_t)CLAMP(res.x * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP(res.y * 65535, 0, 65535);
					}
					src_offset += sizeof(uint8_t) * 4; // 1 byte padding
				}
				else {
					for (uint32_t i = 0; i < p_elements; i++) {
						const float* src =
							(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						const Vector3 original_normal(src[0], src[1], src[2]);
						Vector2 res = original_normal.octahedron_encode();

						uint16_t* dst = (uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
																   dst_offsets[Mesh::ARRAY_NORMAL]];
						dst[0] = (uint16_t)CLAMP(res.x * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP(res.y * 65535, 0, 65535);
					}
					src_offset += sizeof(float) * 3;
				}
			}

		} break;
		case OLD_ARRAY_TANGENT: {
			if (p_old_format & OLD_ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION) {
				if (p_old_format & OLD_ARRAY_COMPRESS_TANGENT) { // int8 SNORM -> uint16 UNORM
					for (uint32_t i = 0; i < p_elements; i++) {
						const int8_t* src =
							(const int8_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						uint16_t* dst =
							(uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
													   dst_offsets[Mesh::ARRAY_TANGENT]];

						dst[0] = (uint16_t)CLAMP((src[0] / 127.0f * .5f + .5f) * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP((src[1] / 127.0f * .5f + .5f) * 65535, 0, 65535);
					}
					src_offset += sizeof(uint8_t) * 2;
				}
				else { // int16 SNORM -> uint16 UNORM
					for (uint32_t i = 0; i < p_elements; i++) {
						const int16_t* src =
							(const int16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						uint16_t* dst =
							(uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
													   dst_offsets[Mesh::ARRAY_TANGENT]];

						dst[0] = (uint16_t)CLAMP((src[0] / 32767.0f * .5f + .5f) * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP((src[1] / 32767.0f * .5f + .5f) * 65535, 0, 65535);
					}
					src_offset += sizeof(uint16_t) * 2;
				}
			}
			else { // No Octahedral compression
				if (p_old_format & OLD_ARRAY_COMPRESS_TANGENT) {
					for (uint32_t i = 0; i < p_elements; i++) {
						const int8_t* src =
							(const int8_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						const Vector3 original_tangent(
							src[0] / 127.0f, src[1] / 127.0f, src[2] / 127.0f);
						Vector2 res = original_tangent.octahedron_tangent_encode(src[3] / 127.0f);

						uint16_t* dst =
							(uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
													   dst_offsets[Mesh::ARRAY_TANGENT]];
						dst[0] = (uint16_t)CLAMP(res.x * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP(res.y * 65535, 0, 65535);
						if (dst[0] == 0 && dst[1] == 65535) {
							// (1, 1) and (0, 1) decode to the same value, but (0, 1) messes with
							// our compression detection. So we sanitize here.
							dst[0] = 65535;
						}
					}
					src_offset += sizeof(uint8_t) * 4;
				}
				else {
					for (uint32_t i = 0; i < p_elements; i++) {
						const float* src =
							(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
						const Vector3 original_tangent(src[0], src[1], src[2]);
						Vector2 res = original_tangent.octahedron_tangent_encode(src[3]);

						uint16_t* dst =
							(uint16_t*)&dst_vertex_ptr[i * dst_normal_tangent_stride +
													   dst_offsets[Mesh::ARRAY_TANGENT]];
						dst[0] = (uint16_t)CLAMP(res.x * 65535, 0, 65535);
						dst[1] = (uint16_t)CLAMP(res.y * 65535, 0, 65535);
						if (dst[0] == 0 && dst[1] == 65535) {
							// (1, 1) and (0, 1) decode to the same value, but (0, 1) messes with
							// our compression detection. So we sanitize here.
							dst[0] = 65535;
						}
					}
					src_offset += sizeof(float) * 4;
				}
			}
		} break;
		case OLD_ARRAY_COLOR: {
			if (p_old_format & OLD_ARRAY_COMPRESS_COLOR) {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint32_t* src =
						(const uint32_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint32_t* dst = (uint32_t*)&dst_attribute_ptr[i * dst_attribute_stride +
																  dst_offsets[Mesh::ARRAY_COLOR]];

					*dst = *src;
				}
				src_offset += sizeof(uint32_t);
			}
			else {
				for (uint32_t i = 0; i < p_elements; i++) {
					const float* src =
						(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint8_t* dst = (uint8_t*)&dst_attribute_ptr[i * dst_attribute_stride +
																dst_offsets[Mesh::ARRAY_COLOR]];

					dst[0] = uint8_t(CLAMP(src[0] * 255.0, 0.0, 255.0));
					dst[1] = uint8_t(CLAMP(src[1] * 255.0, 0.0, 255.0));
					dst[2] = uint8_t(CLAMP(src[2] * 255.0, 0.0, 255.0));
					dst[3] = uint8_t(CLAMP(src[3] * 255.0, 0.0, 255.0));
				}
				src_offset += sizeof(float) * 4;
			}
		} break;
		case OLD_ARRAY_TEX_UV: {
			if (p_old_format & OLD_ARRAY_COMPRESS_TEX_UV) {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint16_t* src =
						(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					float* dst = (float*)&dst_attribute_ptr[i * dst_attribute_stride +
															dst_offsets[Mesh::ARRAY_TEX_UV]];

					dst[0] = Math::half_to_float(src[0]);
					dst[1] = Math::half_to_float(src[1]);
				}
				src_offset += sizeof(uint16_t) * 2;
			}
			else {
				for (uint32_t i = 0; i < p_elements; i++) {
					const float* src =
						(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					float* dst = (float*)&dst_attribute_ptr[i * dst_attribute_stride +
															dst_offsets[Mesh::ARRAY_TEX_UV]];

					dst[0] = src[0];
					dst[1] = src[1];
				}
				src_offset += sizeof(float) * 2;
			}

		} break;
		case OLD_ARRAY_TEX_UV2: {
			if (p_old_format & OLD_ARRAY_COMPRESS_TEX_UV2) {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint16_t* src =
						(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					float* dst = (float*)&dst_attribute_ptr[i * dst_attribute_stride +
															dst_offsets[Mesh::ARRAY_TEX_UV2]];

					dst[0] = Math::half_to_float(src[0]);
					dst[1] = Math::half_to_float(src[1]);
				}
				src_offset += sizeof(uint16_t) * 2;
			}
			else {
				for (uint32_t i = 0; i < p_elements; i++) {
					const float* src =
						(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					float* dst = (float*)&dst_attribute_ptr[i * dst_attribute_stride +
															dst_offsets[Mesh::ARRAY_TEX_UV2]];

					dst[0] = src[0];
					dst[1] = src[1];
				}
				src_offset += sizeof(float) * 2;
			}
		} break;
		case OLD_ARRAY_BONES: {
			if (p_old_format & OLD_ARRAY_FLAG_USE_16_BIT_BONES) {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint16_t* src =
						(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint16_t* dst = (uint16_t*)&dst_skin_ptr[i * dst_skin_stride +
															 dst_offsets[Mesh::ARRAY_BONES]];

					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
				}
				src_offset += sizeof(uint16_t) * 4;
			}
			else {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint8_t* src =
						(const uint8_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint16_t* dst = (uint16_t*)&dst_skin_ptr[i * dst_skin_stride +
															 dst_offsets[Mesh::ARRAY_BONES]];

					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
				}
				src_offset += sizeof(uint8_t) * 4;
			}
		} break;
		case OLD_ARRAY_WEIGHTS: {
			if (p_old_format & OLD_ARRAY_COMPRESS_WEIGHTS) {
				for (uint32_t i = 0; i < p_elements; i++) {
					const uint16_t* src =
						(const uint16_t*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint16_t* dst = (uint16_t*)&dst_skin_ptr[i * dst_skin_stride +
															 dst_offsets[Mesh::ARRAY_WEIGHTS]];

					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
				}
				src_offset += sizeof(uint16_t) * 4;
			}
			else {
				for (uint32_t i = 0; i < p_elements; i++) {
					const float* src =
						(const float*)&src_vertex_ptr[i * src_vertex_stride + src_offset];
					uint16_t* dst = (uint16_t*)&dst_skin_ptr[i * dst_skin_stride +
															 dst_offsets[Mesh::ARRAY_WEIGHTS]];

					dst[0] = uint16_t(CLAMP(src[0] * 65535.0, 0, 65535.0));
					dst[1] = uint16_t(CLAMP(src[1] * 65535.0, 0, 65535.0));
					dst[2] = uint16_t(CLAMP(src[2] * 65535.0, 0, 65535.0));
					dst[3] = uint16_t(CLAMP(src[3] * 65535.0, 0, 65535.0));
				}
				src_offset += sizeof(float) * 4;
			}
		} break;
		default: {
		}
		}
	}
}

void ArrayMesh::_set_blend_shape_names(const PackedStringArray& p_names)
{
	ERR_FAIL_COND(surfaces.size() > 0);

	blend_shapes.resize(p_names.size());
	for (int i = 0; i < p_names.size(); i++) {
		blend_shapes.write[i] = p_names[i];
	}

	if (mesh.is_valid()) {
		RS::get_singleton()->mesh_set_blend_shape_count(mesh, blend_shapes.size());
	}
}

PackedStringArray ArrayMesh::_get_blend_shape_names() const
{
	PackedStringArray sarr;
	sarr.resize(blend_shapes.size());
	for (int i = 0; i < blend_shapes.size(); i++) {
		sarr.write[i] = blend_shapes[i];
	}
	return sarr;
}

void ArrayMesh::_create_if_empty() const
{
	if (!mesh.is_valid()) {
		mesh = RS::get_singleton()->mesh_create();
		RS::get_singleton()->mesh_set_blend_shape_mode(mesh, (RSE::BlendShapeMode)blend_shape_mode);
		RS::get_singleton()->mesh_set_blend_shape_count(mesh, blend_shapes.size());
		RS::get_singleton()->mesh_set_path(mesh, get_path());
	}
}

void ArrayMesh::reset_state()
{
	clear_surfaces();
	clear_blend_shapes();

	aabb = AABB();
	blend_shape_mode = BLEND_SHAPE_MODE_RELATIVE;
	custom_aabb = AABB();
}

void ArrayMesh::_recompute_aabb()
{
	// regenerate AABB
	aabb = AABB();

	for (int i = 0; i < surfaces.size(); i++) {
		if (i == 0) {
			aabb = surfaces[i].aabb;
		}
		else {
			aabb.merge_with(surfaces[i].aabb);
		}
	}
}

int ArrayMesh::get_surface_count() const { return surfaces.size(); }

void ArrayMesh::add_blend_shape(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(
		surfaces.size(), "Can't add a shape key count if surfaces are already created.");

	StringName shape_name = p_name;

	if (blend_shapes.has(shape_name)) {
		int count = 2;
		do {
			shape_name = String(p_name) + " " + itos(count);
			count++;
		} while (blend_shapes.has(shape_name));
	}

	blend_shapes.push_back(shape_name);

	if (mesh.is_valid()) {
		RS::get_singleton()->mesh_set_blend_shape_count(mesh, blend_shapes.size());
	}
}

int ArrayMesh::get_blend_shape_count() const { return blend_shapes.size(); }

StringName ArrayMesh::get_blend_shape_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, blend_shapes.size(), StringName());
	return blend_shapes[p_index];
}

void ArrayMesh::set_blend_shape_name(int p_index, const StringName& p_name)
{
	ERR_FAIL_INDEX(p_index, blend_shapes.size());

	StringName shape_name = p_name;
	int found = blend_shapes.find(shape_name);
	if (found != -1 && found != p_index) {
		int count = 2;
		do {
			shape_name = String(p_name) + " " + itos(count);
			count++;
		} while (blend_shapes.has(shape_name));
	}

	blend_shapes.write[p_index] = shape_name;
}

void ArrayMesh::clear_blend_shapes()
{
	ERR_FAIL_COND_MSG(
		surfaces.size(), "Can't set shape key count if surfaces are already created.");

	blend_shapes.clear();

	if (mesh.is_valid()) {
		RS::get_singleton()->mesh_set_blend_shape_count(mesh, 0);
	}
}

void ArrayMesh::set_blend_shape_mode(BlendShapeMode p_mode)
{
	blend_shape_mode = p_mode;
	if (mesh.is_valid()) {
		RS::get_singleton()->mesh_set_blend_shape_mode(mesh, (RSE::BlendShapeMode)p_mode);
	}
}

ArrayMesh::BlendShapeMode ArrayMesh::get_blend_shape_mode() const { return blend_shape_mode; }

int ArrayMesh::surface_get_array_len(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), -1);
	return surfaces[p_idx].array_length;
}

int ArrayMesh::surface_get_array_index_len(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), -1);
	return surfaces[p_idx].index_array_length;
}

uint32_t ArrayMesh::surface_get_format(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), 0);
	return surfaces[p_idx].format;
}

ArrayMesh::PrimitiveType ArrayMesh::surface_get_primitive_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), PRIMITIVE_LINES);
	return surfaces[p_idx].primitive;
}

void ArrayMesh::surface_set_material(int p_idx, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_idx, surfaces.size());
	if (surfaces[p_idx].material == p_material) {
		return;
	}
	surfaces.write[p_idx].material = p_material;
	RenderingServer::get_singleton()->mesh_surface_set_material(
		mesh, p_idx, p_material.is_null() ? RID() : p_material->get_rid());

	emit_changed();
}

int ArrayMesh::surface_find_by_name(const String& p_name) const
{
	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

void ArrayMesh::surface_set_name(int p_idx, const String& p_name)
{
	ERR_FAIL_INDEX(p_idx, surfaces.size());

	surfaces.write[p_idx].name = p_name;
	emit_changed();
}

String ArrayMesh::surface_get_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), String());
	return surfaces[p_idx].name;
}

void ArrayMesh::surface_update_vertex_region(
	int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	RS::get_singleton()->mesh_surface_update_vertex_region(mesh, p_surface, p_offset, p_data);
	emit_changed();
}

void ArrayMesh::surface_update_attribute_region(
	int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	RS::get_singleton()->mesh_surface_update_attribute_region(mesh, p_surface, p_offset, p_data);
	emit_changed();
}

void ArrayMesh::surface_update_skin_region(
	int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	RS::get_singleton()->mesh_surface_update_skin_region(mesh, p_surface, p_offset, p_data);
	emit_changed();
}

void ArrayMesh::surface_set_custom_aabb(int p_idx, const AABB& p_aabb)
{
	ERR_FAIL_INDEX(p_idx, surfaces.size());
	surfaces.write[p_idx].aabb = p_aabb;
	// set custom aabb too?
	emit_changed();
}

Ref<Material> ArrayMesh::surface_get_material(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, surfaces.size(), Ref<Material>());
	return surfaces[p_idx].material;
}

RID ArrayMesh::get_rid() const
{
	_create_if_empty();
	return mesh;
}

AABB ArrayMesh::get_aabb() const { return aabb; }

void ArrayMesh::clear_surfaces()
{
	if (!mesh.is_valid()) {
		return;
	}
	RS::get_singleton()->mesh_clear(mesh);
	surfaces.clear();
	aabb = AABB();
}

void ArrayMesh::set_custom_aabb(const AABB& p_custom)
{
	_create_if_empty();
	custom_aabb = p_custom;
	RS::get_singleton()->mesh_set_custom_aabb(mesh, custom_aabb);
	emit_changed();
}

AABB ArrayMesh::get_custom_aabb() const { return custom_aabb; }

void ArrayMesh::regen_normal_maps()
{
	if (surfaces.is_empty()) {
		return;
	}
	Vector<Ref<SurfaceTool>> surfs;
	Vector<uint64_t> formats;
	for (int i = 0; i < get_surface_count(); i++) {
		Ref<SurfaceTool> st = memnew(SurfaceTool);
		st->create_from(Ref<ArrayMesh>(this), i);
		surfs.push_back(st);
		formats.push_back(surface_get_format(i));
	}

	clear_surfaces();

	for (int i = 0; i < surfs.size(); i++) {
		surfs.write[i]->generate_tangents();
		surfs.write[i]->commit(Ref<ArrayMesh>(this), formats[i]);
	}
}

// dirty hack
bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float* p_vertices,
	const float* p_normals, int p_vertex_count, const int* p_indices, int p_index_count,
	const uint8_t* p_cache_data, bool* r_use_cache, uint8_t** r_mesh_cache, int* r_mesh_cache_size,
	float** r_uv, int** r_vertex, int* r_vertex_count, int** r_index, int* r_index_count,
	int* r_size_hint_x, int* r_size_hint_y) = nullptr;

struct ArrayMeshLightmapSurface
{
	Ref<Material> material;
	LocalVector<SurfaceTool::Vertex> vertices;
	Mesh::PrimitiveType primitive = Mesh::PrimitiveType::PRIMITIVE_MAX;
	uint64_t format = 0;
};

Error ArrayMesh::lightmap_unwrap(const Transform3D& p_base_transform, float p_texel_size)
{
	Vector<uint8_t> null_cache;
	return lightmap_unwrap_cached(p_base_transform, p_texel_size, null_cache, null_cache, false);
}

void ArrayMesh::set_shadow_mesh(const Ref<ArrayMesh>& p_mesh)
{
	ERR_FAIL_COND_MSG(p_mesh == this, "Cannot set a mesh as its own shadow mesh.");
	shadow_mesh = p_mesh;
	if (shadow_mesh.is_valid()) {
		RS::get_singleton()->mesh_set_shadow_mesh(mesh, shadow_mesh->get_rid());
	}
	else {
		RS::get_singleton()->mesh_set_shadow_mesh(mesh, RID());
	}
}

Ref<ArrayMesh> ArrayMesh::get_shadow_mesh() const { return shadow_mesh; }

ArrayMesh::ArrayMesh()
{
	// mesh is now created on demand
	// mesh = RenderingServer::get_singleton()->mesh_create();
}

ArrayMesh::~ArrayMesh()
{
	if (mesh.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RenderingServer::get_singleton()->free_rid(mesh);
	}
}

PlaceholderMesh::PlaceholderMesh() { rid = RS::get_singleton()->mesh_create(); }

PlaceholderMesh::~PlaceholderMesh()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(rid);
}

int Mesh::get_surface_count() const { return 0; }

int Mesh::surface_get_array_len(int p_idx) const { return 0; }

int Mesh::surface_get_array_index_len(int p_idx) const { return 0; }

uint32_t Mesh::surface_get_format(int p_idx) const { return 0; }

Mesh::PrimitiveType Mesh::surface_get_primitive_type(int p_idx) const
{
	return PRIMITIVE_TRIANGLES;
}

void Mesh::surface_set_material(int p_idx, const Ref<Material>& p_material) {}

Ref<Material> Mesh::surface_get_material(int p_idx) const { return Ref<Material>(); }

int Mesh::get_blend_shape_count() const { return 0; }

StringName Mesh::get_blend_shape_name(int p_idx) const { return StringName(); }

void Mesh::set_blend_shape_name(int p_idx, const StringName& p_name) {}

AABB Mesh::get_aabb() const { return AABB(); }


