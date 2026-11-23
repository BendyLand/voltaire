/**************************************************************************/
/*  mesh.cpp                                                              */
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

#include "core/math/geometry_3d.h"
#include "core/templates/pair.h"
#include "mesh.h"
#include "scene/resources/surface_tool.h"
#include "servers/rendering/renderer.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
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

Ref<Shape3D> Mesh::create_trimesh_shape() const
{
	Vector<Face3> faces = get_faces();
	if (faces.is_empty()) {
		return Ref<Shape3D>();
	}

	Vector<Vector3> face_points;
	face_points.resize(faces.size() * 3);

	for (int i = 0; i < face_points.size(); i += 3) {
		Face3 f = faces.get(i / 3);
		face_points.set(i, f.vertex[0]);
		face_points.set(i + 1, f.vertex[1]);
		face_points.set(i + 2, f.vertex[2]);
	}

	Ref<Shape3D> shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
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
		Ref<Shape3D> shape = memnew(Shape3D(Shape3D::SHAPE_CONVEX_POLYGON));
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
	Renderer::mesh_surface_make_offsets_from_format(
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
		Renderer::mesh_set_blend_shape_count(mesh, blend_shapes.size());
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
		mesh = Renderer::mesh_create();
		Renderer::mesh_set_blend_shape_mode(mesh, (RSE::BlendShapeMode)blend_shape_mode);
		Renderer::mesh_set_blend_shape_count(mesh, blend_shapes.size());
		Renderer::mesh_set_path(mesh, get_path());
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

void ArrayMesh::add_blend_shape(const String& p_name)
{
	ERR_FAIL_COND_MSG(
		surfaces.size(), "Can't add a shape key count if surfaces are already created.");

	String shape_name = p_name;

	if (blend_shapes.has(shape_name)) {
		int count = 2;
		do {
			shape_name = String(p_name) + " " + itos(count);
			count++;
		} while (blend_shapes.has(shape_name));
	}

	blend_shapes.push_back(shape_name);

	if (mesh.is_valid()) {
		Renderer::mesh_set_blend_shape_count(mesh, blend_shapes.size());
	}
}

int ArrayMesh::get_blend_shape_count() const { return blend_shapes.size(); }

String ArrayMesh::get_blend_shape_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, blend_shapes.size(), String());
	return blend_shapes[p_index];
}

void ArrayMesh::set_blend_shape_name(int p_index, const String& p_name)
{
	ERR_FAIL_INDEX(p_index, blend_shapes.size());

	String shape_name = p_name;
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
		Renderer::mesh_set_blend_shape_count(mesh, 0);
	}
}

void ArrayMesh::set_blend_shape_mode(BlendShapeMode p_mode)
{
	blend_shape_mode = p_mode;
	if (mesh.is_valid()) {
		Renderer::mesh_set_blend_shape_mode(mesh, (RSE::BlendShapeMode)p_mode);
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
	Renderer::mesh_surface_set_material(
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
	Renderer::mesh_surface_update_vertex_region(mesh, p_surface, p_offset, p_data);
	emit_changed();
}

void ArrayMesh::surface_update_attribute_region(
	int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	Renderer::mesh_surface_update_attribute_region(mesh, p_surface, p_offset, p_data);
	emit_changed();
}

void ArrayMesh::surface_update_skin_region(
	int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	Renderer::mesh_surface_update_skin_region(mesh, p_surface, p_offset, p_data);
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
	Renderer::mesh_clear(mesh);
	surfaces.clear();
	aabb = AABB();
}

void ArrayMesh::set_custom_aabb(const AABB& p_custom)
{
	_create_if_empty();
	custom_aabb = p_custom;
	Renderer::mesh_set_custom_aabb(mesh, custom_aabb);
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
		Renderer::mesh_set_shadow_mesh(mesh, shadow_mesh->get_rid());
	}
	else {
		Renderer::mesh_set_shadow_mesh(mesh, RID());
	}
}

Ref<ArrayMesh> ArrayMesh::get_shadow_mesh() const { return shadow_mesh; }

ArrayMesh::ArrayMesh()
{
	// mesh is now created on demand
	// mesh = Renderer::mesh_create();
}

ArrayMesh::~ArrayMesh()
{
	if (mesh.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(mesh);
	}
}

PlaceholderMesh::PlaceholderMesh() { rid = Renderer::mesh_create(); }

PlaceholderMesh::~PlaceholderMesh()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(rid);
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

String Mesh::get_blend_shape_name(int p_idx) const { return String(); }

void Mesh::set_blend_shape_name(int p_idx, const String& p_name) {}

AABB Mesh::get_aabb() const { return AABB(); }

Ref<TriangleMesh> Mesh::generate_triangle_mesh() const
{
	Ref<TriangleMesh> tm = memnew(TriangleMesh);
	return tm;
}

#ifndef PHYSICS_3D_DISABLED
Ref<Shape3D> Mesh::create_convex_shape(bool, bool) const
{
	Ref<Shape3D> cps = memnew(Shape3D(Shape3D::SHAPE_CONVEX_POLYGON));
	return cps;
}
#endif // PHYSICS_3D_DISABLED

Ref<TriangleMesh> Mesh::generate_surface_triangle_mesh(int) const
{
	Ref<TriangleMesh> tm = memnew(TriangleMesh);
	return tm;
}

Error ArrayMesh::lightmap_unwrap_cached(const Transform3D& p_base_transform, float p_texel_size,
	const Vector<uint8_t>& p_src_cache, Vector<uint8_t>& r_dst_cache, bool p_generate_cache)
{
	return OK;
}

void ArrayMesh::reload_from_file() {}

// --- immediate_mesh.cpp ---

#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

void ImmediateMesh::surface_begin(PrimitiveType p_primitive, const Ref<Material>& p_material)
{
	ERR_FAIL_COND_MSG(surface_active, "Already creating a new surface.");
	active_surface_data.primitive = p_primitive;
	active_surface_data.material = p_material;
	surface_active = true;
}

void ImmediateMesh::surface_set_color(const Color& p_color)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");

	if (!uses_colors) {
		colors.resize(vertices.size());
		for (Color& color : colors) {
			color = p_color;
		}
		uses_colors = true;
	}

	current_color = p_color;
}

void ImmediateMesh::surface_set_normal(const Vector3& p_normal)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");

	if (!uses_normals) {
		normals.resize(vertices.size());
		for (Vector3& normal : normals) {
			normal = p_normal;
		}
		uses_normals = true;
	}

	current_normal = p_normal;
}

void ImmediateMesh::surface_set_tangent(const Plane& p_tangent)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	if (!uses_tangents) {
		tangents.resize(vertices.size());
		for (Plane& tangent : tangents) {
			tangent = p_tangent;
		}
		uses_tangents = true;
	}

	current_tangent = p_tangent;
}

void ImmediateMesh::surface_set_uv(const Vector2& p_uv)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	if (!uses_uvs) {
		uvs.resize(vertices.size());
		for (Vector2& uv : uvs) {
			uv = p_uv;
		}
		uses_uvs = true;
	}

	current_uv = p_uv;
}

void ImmediateMesh::surface_set_uv2(const Vector2& p_uv2)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	if (!uses_uv2s) {
		uv2s.resize(vertices.size());
		for (Vector2& uv : uv2s) {
			uv = p_uv2;
		}
		uses_uv2s = true;
	}

	current_uv2 = p_uv2;
}

void ImmediateMesh::surface_add_vertex(const Vector3& p_vertex)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	ERR_FAIL_COND_MSG(vertices.size() && active_surface_data.vertex_2d,
		"Can't mix 2D and 3D vertices in a surface.");

	if (uses_colors) {
		colors.push_back(current_color);
	}
	if (uses_normals) {
		normals.push_back(current_normal);
	}
	if (uses_tangents) {
		tangents.push_back(current_tangent);
	}
	if (uses_uvs) {
		uvs.push_back(current_uv);
	}
	if (uses_uv2s) {
		uv2s.push_back(current_uv2);
	}
	vertices.push_back(p_vertex);
}

void ImmediateMesh::surface_add_vertex_2d(const Vector2& p_vertex)
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	ERR_FAIL_COND_MSG(vertices.size() && !active_surface_data.vertex_2d,
		"Can't mix 2D and 3D vertices in a surface.");

	if (uses_colors) {
		colors.push_back(current_color);
	}
	if (uses_normals) {
		normals.push_back(current_normal);
	}
	if (uses_tangents) {
		tangents.push_back(current_tangent);
	}
	if (uses_uvs) {
		uvs.push_back(current_uv);
	}
	if (uses_uv2s) {
		uv2s.push_back(current_uv2);
	}
	Vector3 v(p_vertex.x, p_vertex.y, 0);
	vertices.push_back(v);

	active_surface_data.vertex_2d = true;
}

void ImmediateMesh::surface_end()
{
	ERR_FAIL_COND_MSG(!surface_active, "Not creating any surface. Use surface_begin() to do it.");
	ERR_FAIL_COND_MSG(vertices.is_empty(), "No vertices were added, surface can't be created.");

	uint64_t format = ARRAY_FORMAT_VERTEX | ARRAY_FLAG_FORMAT_CURRENT_VERSION;

	uint32_t vertex_stride = 0;
	if (active_surface_data.vertex_2d) {
		format |= ARRAY_FLAG_USE_2D_VERTICES;
		vertex_stride = sizeof(float) * 2;
	}
	else {
		vertex_stride = sizeof(float) * 3;
	}
	uint32_t normal_tangent_stride = 0;
	uint32_t normal_offset = 0;
	if (uses_normals) {
		format |= ARRAY_FORMAT_NORMAL;
		normal_offset = vertex_stride * vertices.size();
		normal_tangent_stride += sizeof(uint32_t);
	}
	uint32_t tangent_offset = 0;
	if (uses_tangents || uses_normals) {
		format |= ARRAY_FORMAT_TANGENT;
		tangent_offset = vertex_stride * vertices.size() + normal_tangent_stride;
		normal_tangent_stride += sizeof(uint32_t);
	}

	AABB aabb;

	{
		surface_vertex_create_cache.resize(
			(vertex_stride + normal_tangent_stride) * vertices.size());
		uint8_t* surface_vertex_ptr = surface_vertex_create_cache.ptrw();
		for (uint32_t i = 0; i < vertices.size(); i++) {
			{
				float* vtx = (float*)&surface_vertex_ptr[i * vertex_stride];
				vtx[0] = vertices[i].x;
				vtx[1] = vertices[i].y;
				if (!active_surface_data.vertex_2d) {
					vtx[2] = vertices[i].z;
				}
				if (i == 0) {
					aabb = AABB(vertices[i], SMALL_VEC3); // Must have a bit of size.
				}
				else {
					aabb.expand_to(vertices[i]);
				}
			}
			if (uses_normals) {
				uint32_t* normal =
					(uint32_t*)&surface_vertex_ptr[i * normal_tangent_stride + normal_offset];

				Vector2 n = normals[i].octahedron_encode();

				uint32_t value = 0;
				value |= (uint16_t)CLAMP(n.x * 65535, 0, 65535);
				value |= (uint16_t)CLAMP(n.y * 65535, 0, 65535) << 16;

				*normal = value;
			}
			if (uses_tangents || uses_normals) {
				uint32_t* tangent =
					(uint32_t*)&surface_vertex_ptr[i * normal_tangent_stride + tangent_offset];
				Vector2 t;
				if (uses_tangents) {
					t = tangents[i].normal.octahedron_tangent_encode(tangents[i].d);
				}
				else {
					Vector3 tan = Vector3(normals[i].z, -normals[i].x, normals[i].y)
									  .cross(normals[i].normalized())
									  .normalized();
					t = tan.octahedron_tangent_encode(1.0);
				}

				uint32_t value = 0;
				value |= (uint16_t)CLAMP(t.x * 65535, 0, 65535);
				value |= (uint16_t)CLAMP(t.y * 65535, 0, 65535) << 16;
				if (value == 4294901760) {
					// (1, 1) and (0, 1) decode to the same value, but (0, 1) messes with our
					// compression detection. So we sanitize here.
					value = 4294967295;
				}

				*tangent = value;
			}
		}
	}

	if (uses_colors || uses_uvs || uses_uv2s) {
		uint32_t attribute_stride = 0;

		if (uses_colors) {
			format |= ARRAY_FORMAT_COLOR;
			attribute_stride += sizeof(uint8_t) * 4;
		}
		uint32_t uv_offset = 0;
		if (uses_uvs) {
			format |= ARRAY_FORMAT_TEX_UV;
			uv_offset = attribute_stride;
			attribute_stride += sizeof(float) * 2;
		}
		uint32_t uv2_offset = 0;
		if (uses_uv2s) {
			format |= ARRAY_FORMAT_TEX_UV2;
			uv2_offset = attribute_stride;
			attribute_stride += sizeof(float) * 2;
		}

		surface_attribute_create_cache.resize(vertices.size() * attribute_stride);

		uint8_t* surface_attribute_ptr = surface_attribute_create_cache.ptrw();

		for (uint32_t i = 0; i < vertices.size(); i++) {
			if (uses_colors) {
				uint8_t* color8 = (uint8_t*)&surface_attribute_ptr[i * attribute_stride];

				color8[0] = uint8_t(CLAMP(colors[i].r * 255.0, 0.0, 255.0));
				color8[1] = uint8_t(CLAMP(colors[i].g * 255.0, 0.0, 255.0));
				color8[2] = uint8_t(CLAMP(colors[i].b * 255.0, 0.0, 255.0));
				color8[3] = uint8_t(CLAMP(colors[i].a * 255.0, 0.0, 255.0));
			}
			if (uses_uvs) {
				float* uv = (float*)&surface_attribute_ptr[i * attribute_stride + uv_offset];

				uv[0] = uvs[i].x;
				uv[1] = uvs[i].y;
			}

			if (uses_uv2s) {
				float* uv2 = (float*)&surface_attribute_ptr[i * attribute_stride + uv2_offset];

				uv2[0] = uv2s[i].x;
				uv2[1] = uv2s[i].y;
			}
		}
	}

	RenderingServerTypes::SurfaceData sd;

	sd.primitive = RSE::PrimitiveType(active_surface_data.primitive);
	sd.format = format;
	sd.vertex_data = surface_vertex_create_cache;
	if (uses_colors || uses_uvs || uses_uv2s) {
		sd.attribute_data = surface_attribute_create_cache;
	}
	sd.vertex_count = vertices.size();
	sd.aabb = aabb;
	if (active_surface_data.material.is_valid()) {
		sd.material = active_surface_data.material->get_rid();
	}

	Renderer::mesh_add_surface(mesh, sd);

	active_surface_data.aabb = aabb;

	active_surface_data.format = format;
	active_surface_data.array_len = vertices.size();

	surfaces.push_back(active_surface_data);

	colors.clear();
	normals.clear();
	tangents.clear();
	uvs.clear();
	uv2s.clear();
	vertices.clear();

	uses_colors = false;
	uses_normals = false;
	uses_tangents = false;
	uses_uvs = false;
	uses_uv2s = false;

	active_surface_data.vertex_2d = false;

	surface_active = false;

	emit_changed();
}

void ImmediateMesh::clear_surfaces()
{
	Renderer::mesh_clear(mesh);
	surfaces.clear();
	surface_active = false;

	colors.clear();
	normals.clear();
	tangents.clear();
	uvs.clear();
	uv2s.clear();
	vertices.clear();

	uses_colors = false;
	uses_normals = false;
	uses_tangents = false;
	uses_uvs = false;
	uses_uv2s = false;
}

int ImmediateMesh::get_surface_count() const { return surfaces.size(); }

int ImmediateMesh::surface_get_array_len(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, int(surfaces.size()), -1);
	return surfaces[p_idx].array_len;
}

int ImmediateMesh::surface_get_array_index_len(int p_idx) const { return 0; }

uint32_t ImmediateMesh::surface_get_format(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, int(surfaces.size()), 0);
	return surfaces[p_idx].format;
}

Mesh::PrimitiveType ImmediateMesh::surface_get_primitive_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, int(surfaces.size()), PRIMITIVE_MAX);
	return surfaces[p_idx].primitive;
}

void ImmediateMesh::surface_set_material(int p_idx, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_idx, int(surfaces.size()));
	surfaces[p_idx].material = p_material;
	RID mat;
	if (p_material.is_valid()) {
		mat = p_material->get_rid();
	}
	Renderer::mesh_surface_set_material(mesh, p_idx, mat);
}

Ref<Material> ImmediateMesh::surface_get_material(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, int(surfaces.size()), Ref<Material>());
	return surfaces[p_idx].material;
}

int ImmediateMesh::get_blend_shape_count() const { return 0; }

String ImmediateMesh::get_blend_shape_name(int p_index) const { return String(); }

void ImmediateMesh::set_blend_shape_name(int p_index, const String& p_name) {}

AABB ImmediateMesh::get_aabb() const
{
	AABB aabb;
	for (uint32_t i = 0; i < surfaces.size(); i++) {
		if (i == 0) {
			aabb = surfaces[i].aabb;
		}
		else {
			aabb = aabb.merge(surfaces[i].aabb);
		}
	}
	return aabb;
}


RID ImmediateMesh::get_rid() const { return mesh; }

ImmediateMesh::ImmediateMesh() { mesh = Renderer::mesh_create(); }

ImmediateMesh::~ImmediateMesh()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(mesh);
}

// --- multimesh.cpp ---

#include "scene/3d/visual_instance_3d.h"
#include "servers/rendering/renderer.h"

#ifndef DISABLE_DEPRECATED
// Kept for compatibility from 3.x to 4.0.

void MultiMesh::_set_transform_array(const Vector<Vector3>& p_array)
{
	if (transform_format != TRANSFORM_3D) {
		return;
	}

	const Vector<Vector3>& xforms = p_array;
	int len = xforms.size();
	ERR_FAIL_COND((len / 4) != instance_count);
	if (len == 0) {
		return;
	}

	const Vector3* r = xforms.ptr();

	for (int i = 0; i < len / 4; i++) {
		Transform3D t;
		t.basis[0] = r[i * 4 + 0];
		t.basis[1] = r[i * 4 + 1];
		t.basis[2] = r[i * 4 + 2];
		t.origin = r[i * 4 + 3];

		set_instance_transform(i, t);
	}
}

Vector<Vector3> MultiMesh::_get_transform_array() const
{
	if (transform_format != TRANSFORM_3D) {
		return Vector<Vector3>();
	}

	if (instance_count == 0) {
		return Vector<Vector3>();
	}

	Vector<Vector3> xforms;
	xforms.resize(instance_count * 4);

	Vector3* w = xforms.ptrw();

	for (int i = 0; i < instance_count; i++) {
		Transform3D t = get_instance_transform(i);
		w[i * 4 + 0] = t.basis[0];
		w[i * 4 + 1] = t.basis[1];
		w[i * 4 + 2] = t.basis[2];
		w[i * 4 + 3] = t.origin;
	}

	return xforms;
}

void MultiMesh::_set_transform_2d_array(const Vector<Vector2>& p_array)
{
	if (transform_format != TRANSFORM_2D) {
		return;
	}

	const Vector<Vector2>& xforms = p_array;
	int len = xforms.size();
	ERR_FAIL_COND((len / 3) != instance_count);
	if (len == 0) {
		return;
	}

	const Vector2* r = xforms.ptr();

	for (int i = 0; i < len / 3; i++) {
		Transform2D t;
		t.columns[0] = r[i * 3 + 0];
		t.columns[1] = r[i * 3 + 1];
		t.columns[2] = r[i * 3 + 2];

		set_instance_transform_2d(i, t);
	}
}

Vector<Vector2> MultiMesh::_get_transform_2d_array() const
{
	if (transform_format != TRANSFORM_2D) {
		return Vector<Vector2>();
	}

	if (instance_count == 0) {
		return Vector<Vector2>();
	}

	Vector<Vector2> xforms;
	xforms.resize(instance_count * 3);

	Vector2* w = xforms.ptrw();

	for (int i = 0; i < instance_count; i++) {
		Transform2D t = get_instance_transform_2d(i);
		w[i * 3 + 0] = t.columns[0];
		w[i * 3 + 1] = t.columns[1];
		w[i * 3 + 2] = t.columns[2];
	}

	return xforms;
}

void MultiMesh::_set_color_array(const Vector<Color>& p_array)
{
	const Vector<Color>& colors = p_array;
	int len = colors.size();
	if (len == 0) {
		return;
	}
	ERR_FAIL_COND(len != instance_count);

	const Color* r = colors.ptr();

	for (int i = 0; i < len; i++) {
		set_instance_color(i, r[i]);
	}
}

Vector<Color> MultiMesh::_get_color_array() const
{
	if (instance_count == 0 || !use_colors) {
		return Vector<Color>();
	}

	Vector<Color> colors;
	colors.resize(instance_count);

	for (int i = 0; i < instance_count; i++) {
		colors.set(i, get_instance_color(i));
	}

	return colors;
}

void MultiMesh::_set_custom_data_array(const Vector<Color>& p_array)
{
	const Vector<Color>& custom_datas = p_array;
	int len = custom_datas.size();
	if (len == 0) {
		return;
	}
	ERR_FAIL_COND(len != instance_count);

	const Color* r = custom_datas.ptr();

	for (int i = 0; i < len; i++) {
		set_instance_custom_data(i, r[i]);
	}
}

Vector<Color> MultiMesh::_get_custom_data_array() const
{
	if (instance_count == 0 || !use_custom_data) {
		return Vector<Color>();
	}

	Vector<Color> custom_datas;
	custom_datas.resize(instance_count);

	for (int i = 0; i < instance_count; i++) {
		custom_datas.set(i, get_instance_custom_data(i));
	}

	return custom_datas;
}
#endif // DISABLE_DEPRECATED

void MultiMesh::set_buffer(const Vector<float>& p_buffer)
{
	if (instance_count == 0) {
		return;
	}

	uint32_t stride = transform_format == TRANSFORM_2D ? 8 : 12;
	stride += use_colors ? 4 : 0;
	stride += use_custom_data ? 4 : 0;
	ERR_FAIL_COND_MSG(stride * instance_count != p_buffer.size(),
		"Cannot set a buffer on a Multimesh that is a different size from the Multimesh's existing "
		"buffer.");

	Renderer::multimesh_set_buffer(multimesh, p_buffer);
}

Vector<float> MultiMesh::get_buffer() const
{
	return Renderer::multimesh_get_buffer(multimesh);
}

void MultiMesh::set_buffer_interpolated(
	const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev)
{
	Renderer::multimesh_set_buffer_interpolated(multimesh, p_buffer_curr, p_buffer_prev);
}

void MultiMesh::set_mesh(const Ref<Mesh>& p_mesh)
{
	mesh = p_mesh;
	if (mesh.is_valid()) {
		Renderer::multimesh_set_mesh(multimesh, mesh->get_rid());
	}
	else {
		Renderer::multimesh_set_mesh(multimesh, RID());
	}
}

Ref<Mesh> MultiMesh::get_mesh() const { return mesh; }

void MultiMesh::set_instance_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0);
	Renderer::multimesh_allocate_data(multimesh, p_count,
		RSE::MultimeshTransformFormat(transform_format), use_colors, use_custom_data);
	instance_count = p_count;
}

int MultiMesh::get_instance_count() const { return instance_count; }

void MultiMesh::set_visible_instance_count(int p_count)
{
	ERR_FAIL_COND(p_count < -1);
	ERR_FAIL_COND(p_count > instance_count);
	Renderer::multimesh_set_visible_instances(multimesh, p_count);
	visible_instance_count = p_count;
}

int MultiMesh::get_visible_instance_count() const { return visible_instance_count; }

void MultiMesh::set_physics_interpolation_quality(PhysicsInterpolationQuality p_quality)
{
	_physics_interpolation_quality = p_quality;
	Renderer::multimesh_set_physics_interpolation_quality(
		multimesh, (RSE::MultimeshPhysicsInterpolationQuality)p_quality);
}

void MultiMesh::set_instance_transform(int p_instance, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX_MSG(p_instance, instance_count,
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_MSG(transform_format == TRANSFORM_2D,
		"Can't set Transform3D on a Multimesh configured to use Transform2D. Ensure that you have "
		"set the `transform_format` to `TRANSFORM_3D`.");
	Renderer::multimesh_instance_set_transform(
		multimesh, p_instance, p_transform);
}

void MultiMesh::set_instance_transform_2d(int p_instance, const Transform2D& p_transform)
{
	ERR_FAIL_INDEX_MSG(p_instance, instance_count,
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_MSG(transform_format == TRANSFORM_3D,
		"Can't set Transform2D on a Multimesh configured to use Transform3D. Ensure that you have "
		"set the `transform_format` to `TRANSFORM_2D`.");
	Renderer::multimesh_instance_set_transform_2d(
		multimesh, p_instance, p_transform);
	emit_changed();
}

Transform3D MultiMesh::get_instance_transform(int p_instance) const
{
	ERR_FAIL_INDEX_V_MSG(p_instance, instance_count, Transform3D(),
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_V_MSG(transform_format == TRANSFORM_2D, Transform3D(),
		"Can't get Transform3D on a Multimesh configured to use Transform2D. Ensure that you have "
		"set the `transform_format` to `TRANSFORM_3D`.");
	return Renderer::multimesh_instance_get_transform(
		multimesh, p_instance);
}

Transform2D MultiMesh::get_instance_transform_2d(int p_instance) const
{
	ERR_FAIL_INDEX_V_MSG(p_instance, instance_count, Transform2D(),
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_V_MSG(transform_format == TRANSFORM_3D, Transform2D(),
		"Can't get Transform2D on a Multimesh configured to use Transform3D. Ensure that you have "
		"set the `transform_format` to `TRANSFORM_2D`.");
	return Renderer::multimesh_instance_get_transform_2d(
		multimesh, p_instance);
}

void MultiMesh::set_instance_color(int p_instance, const Color& p_color)
{
	ERR_FAIL_INDEX_MSG(p_instance, instance_count,
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_MSG(
		!use_colors, "Can't set instance color on a Multimesh that isn't using colors. Ensure that "
					 "you have `use_colors` property of this Multimesh set to `true`.");
	Renderer::multimesh_instance_set_color(multimesh, p_instance, p_color);
}

Color MultiMesh::get_instance_color(int p_instance) const
{
	ERR_FAIL_INDEX_V_MSG(p_instance, instance_count, Color(),
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_V_MSG(!use_colors, Color(),
		"Can't get instance color on a Multimesh that isn't using colors. Ensure that you have "
		"`use_colors` property of this Multimesh set to `true`.");
	return Renderer::multimesh_instance_get_color(multimesh, p_instance);
}

void MultiMesh::set_instance_custom_data(int p_instance, const Color& p_custom_data)
{
	ERR_FAIL_INDEX_MSG(p_instance, instance_count,
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_MSG(!use_custom_data,
		"Can't get instance custom data on a Multimesh that isn't using custom data. Ensure that "
		"you have `use_custom_data` property of this Multimesh set to `true`.");
	Renderer::multimesh_instance_set_custom_data(
		multimesh, p_instance, p_custom_data);
}

Color MultiMesh::get_instance_custom_data(int p_instance) const
{
	ERR_FAIL_INDEX_V_MSG(p_instance, instance_count, Color(),
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	ERR_FAIL_COND_V_MSG(!use_custom_data, Color(),
		"Can't get instance custom data on a Multimesh that isn't using custom data. Ensure that "
		"you have `use_custom_data` property of this Multimesh set to `true`.");
	return Renderer::multimesh_instance_get_custom_data(
		multimesh, p_instance);
}

void MultiMesh::reset_instance_physics_interpolation(int p_instance)
{
	ERR_FAIL_INDEX_MSG(p_instance, instance_count,
		"Instance index out of bounds. Instance index must be less than `instance_count` and "
		"greater than or equal to zero.");
	Renderer::multimesh_instance_reset_physics_interpolation(
		multimesh, p_instance);
}

void MultiMesh::reset_instances_physics_interpolation()
{
	Renderer::multimesh_instances_reset_physics_interpolation(multimesh);
}

void MultiMesh::set_physics_interpolated(bool p_interpolated)
{
	Renderer::multimesh_set_physics_interpolated(multimesh, p_interpolated);
}

void MultiMesh::set_custom_aabb(const AABB& p_custom)
{
	custom_aabb = p_custom;
	Renderer::multimesh_set_custom_aabb(multimesh, custom_aabb);
	emit_changed();
}

AABB MultiMesh::get_custom_aabb() const { return custom_aabb; }

AABB MultiMesh::get_aabb() const
{
	return Renderer::multimesh_get_aabb(multimesh);
}

RID MultiMesh::get_rid() const { return multimesh; }

void MultiMesh::set_use_colors(bool p_enable)
{
	ERR_FAIL_COND_MSG(
		instance_count > 0, "Instance count must be 0 to toggle whether colors are used.");
	use_colors = p_enable;
}

bool MultiMesh::is_using_colors() const { return use_colors; }

void MultiMesh::set_use_custom_data(bool p_enable)
{
	ERR_FAIL_COND_MSG(
		instance_count > 0, "Instance count must be 0 to toggle whether custom data is used.");
	use_custom_data = p_enable;
}

bool MultiMesh::is_using_custom_data() const { return use_custom_data; }

void MultiMesh::set_transform_format(TransformFormat p_transform_format)
{
	ERR_FAIL_COND_MSG(
		instance_count > 0, "Instance count must be 0 to change the transform format.");
	transform_format = p_transform_format;
}

MultiMesh::TransformFormat MultiMesh::get_transform_format() const { return transform_format; }


MultiMesh::MultiMesh() { multimesh = Renderer::multimesh_create(); }

MultiMesh::~MultiMesh()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(multimesh);
}

// --- mesh_data_tool.cpp ---
// --- Inlined from scene/resources/mesh_data_tool.h ---
#include "scene/resources/mesh.h"

class MeshDataTool : public RefCounted
{
	uint64_t format = 0;

	struct Vertex
	{
		Vector3 vertex;
		Color color;
		Vector3 normal; // normal, binormal, tangent
		Plane tangent;
		Vector2 uv;
		Vector2 uv2;
		Vector<int> bones;
		Vector<float> weights;
		Vector<int> edges;
		Vector<int> faces;
	};

	Vector<Vertex> vertices;

	struct Edge
	{
		int vertex[2] = {};
		Vector<int> faces;
	};

	Vector<Edge> edges;

	struct Face
	{
		int v[3] = {};
		int edges[3] = {};
	};

	Vector<Face> faces;

	Ref<Material> material;

protected:

#ifndef DISABLE_DEPRECATED
	Error commit_to_surface_bind_compat_81138(const Ref<ArrayMesh>& p_mesh);
	static void _bind_compatibility_methods();
#endif

public:
	void clear();
	Error create_from_surface(const Ref<ArrayMesh>& p_mesh, int p_surface);
	Error commit_to_surface(const Ref<ArrayMesh>& p_mesh, uint64_t p_compression_flags = 0);

	uint64_t get_format() const;

	int get_vertex_count() const;
	int get_edge_count() const;
	int get_face_count() const;

	Vector3 get_vertex(int p_idx) const;
	void set_vertex(int p_idx, const Vector3& p_vertex);

	Vector3 get_vertex_normal(int p_idx) const;
	void set_vertex_normal(int p_idx, const Vector3& p_normal);

	Plane get_vertex_tangent(int p_idx) const;
	void set_vertex_tangent(int p_idx, const Plane& p_tangent);

	Vector2 get_vertex_uv(int p_idx) const;
	void set_vertex_uv(int p_idx, const Vector2& p_uv);

	Vector2 get_vertex_uv2(int p_idx) const;
	void set_vertex_uv2(int p_idx, const Vector2& p_uv2);

	Color get_vertex_color(int p_idx) const;
	void set_vertex_color(int p_idx, const Color& p_color);

	Vector<int> get_vertex_bones(int p_idx) const;
	void set_vertex_bones(int p_idx, const Vector<int>& p_bones);

	Vector<float> get_vertex_weights(int p_idx) const;
	void set_vertex_weights(int p_idx, const Vector<float>& p_weights);

	Vector<int> get_vertex_edges(int p_idx) const;
	Vector<int> get_vertex_faces(int p_idx) const;

	int get_edge_vertex(int p_edge, int p_vertex) const;
	Vector<int> get_edge_faces(int p_edge) const;

	int get_face_vertex(int p_face, int p_vertex) const;
	int get_face_edge(int p_face, int p_vertex) const;
	Vector3 get_face_normal(int p_face) const;

	Ref<Material> get_material() const;
	void set_material(const Ref<Material>& p_material);

	MeshDataTool();
};



// --- End scene/resources/mesh_data_tool.h ---

void MeshDataTool::clear()
{
	vertices.clear();
	edges.clear();
	faces.clear();
	material = Ref<Material>();
	format = 0;
}

uint64_t MeshDataTool::get_format() const { return format; }

int MeshDataTool::get_vertex_count() const { return vertices.size(); }

int MeshDataTool::get_edge_count() const { return edges.size(); }

int MeshDataTool::get_face_count() const { return faces.size(); }

Vector3 MeshDataTool::get_vertex(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector3());
	return vertices[p_idx].vertex;
}

void MeshDataTool::set_vertex(int p_idx, const Vector3& p_vertex)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].vertex = p_vertex;
}

Vector3 MeshDataTool::get_vertex_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector3());
	return vertices[p_idx].normal;
}

void MeshDataTool::set_vertex_normal(int p_idx, const Vector3& p_normal)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].normal = p_normal;
	format |= Mesh::ARRAY_FORMAT_NORMAL;
}

Plane MeshDataTool::get_vertex_tangent(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Plane());
	return vertices[p_idx].tangent;
}

void MeshDataTool::set_vertex_tangent(int p_idx, const Plane& p_tangent)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].tangent = p_tangent;
	format |= Mesh::ARRAY_FORMAT_TANGENT;
}

Vector2 MeshDataTool::get_vertex_uv(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector2());
	return vertices[p_idx].uv;
}

void MeshDataTool::set_vertex_uv(int p_idx, const Vector2& p_uv)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].uv = p_uv;
	format |= Mesh::ARRAY_FORMAT_TEX_UV;
}

Vector2 MeshDataTool::get_vertex_uv2(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector2());
	return vertices[p_idx].uv2;
}

void MeshDataTool::set_vertex_uv2(int p_idx, const Vector2& p_uv2)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].uv2 = p_uv2;
	format |= Mesh::ARRAY_FORMAT_TEX_UV2;
}

Color MeshDataTool::get_vertex_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Color());
	return vertices[p_idx].color;
}

void MeshDataTool::set_vertex_color(int p_idx, const Color& p_color)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	vertices.write[p_idx].color = p_color;
	format |= Mesh::ARRAY_FORMAT_COLOR;
}

Vector<int> MeshDataTool::get_vertex_bones(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].bones;
}

void MeshDataTool::set_vertex_bones(int p_idx, const Vector<int>& p_bones)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	ERR_FAIL_COND(p_bones.size() != 4);
	vertices.write[p_idx].bones = p_bones;
	format |= Mesh::ARRAY_FORMAT_BONES;
}

Vector<float> MeshDataTool::get_vertex_weights(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<float>());
	return vertices[p_idx].weights;
}

void MeshDataTool::set_vertex_weights(int p_idx, const Vector<float>& p_weights)
{
	ERR_FAIL_INDEX(p_idx, vertices.size());
	ERR_FAIL_COND(p_weights.size() != 4);
	vertices.write[p_idx].weights = p_weights;
	format |= Mesh::ARRAY_FORMAT_WEIGHTS;
}

Vector<int> MeshDataTool::get_vertex_edges(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].edges;
}

Vector<int> MeshDataTool::get_vertex_faces(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, vertices.size(), Vector<int>());
	return vertices[p_idx].faces;
}

int MeshDataTool::get_edge_vertex(int p_edge, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_edge, edges.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 2, -1);
	return edges[p_edge].vertex[p_vertex];
}

Vector<int> MeshDataTool::get_edge_faces(int p_edge) const
{
	ERR_FAIL_INDEX_V(p_edge, edges.size(), Vector<int>());
	return edges[p_edge].faces;
}

int MeshDataTool::get_face_vertex(int p_face, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 3, -1);
	return faces[p_face].v[p_vertex];
}

int MeshDataTool::get_face_edge(int p_face, int p_vertex) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), -1);
	ERR_FAIL_INDEX_V(p_vertex, 3, -1);
	return faces[p_face].edges[p_vertex];
}

Vector3 MeshDataTool::get_face_normal(int p_face) const
{
	ERR_FAIL_INDEX_V(p_face, faces.size(), Vector3());
	Vector3 v0 = vertices[faces[p_face].v[0]].vertex;
	Vector3 v1 = vertices[faces[p_face].v[1]].vertex;
	Vector3 v2 = vertices[faces[p_face].v[2]].vertex;

	return Plane(v0, v1, v2).normal;
}

Ref<Material> MeshDataTool::get_material() const { return material; }

void MeshDataTool::set_material(const Ref<Material>& p_material) { material = p_material; }

MeshDataTool::MeshDataTool() { clear(); }



Error MeshDataTool::commit_to_surface(Ref<ArrayMesh> const&, unsigned long) {}

// --- mesh_data_tool.compat.inc ---

#ifndef DISABLE_DEPRECATED
Error MeshDataTool::commit_to_surface_bind_compat_81138(const Ref<ArrayMesh>& p_mesh)
{
	return commit_to_surface(p_mesh, 0);
}

void MeshDataTool::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- surface_tool.cpp ---

#include "core/templates/a_hash_map.h"
#include "surface_tool.h"

#define EQ_VERTEX_DIST 0.00001

SurfaceTool::OptimizeVertexCacheFunc SurfaceTool::optimize_vertex_cache_func = nullptr;
SurfaceTool::OptimizeVertexFetchRemapFunc SurfaceTool::optimize_vertex_fetch_remap_func = nullptr;
SurfaceTool::SimplifyFunc SurfaceTool::simplify_func = nullptr;
SurfaceTool::SimplifyWithAttribFunc SurfaceTool::simplify_with_attrib_func = nullptr;
SurfaceTool::SimplifyScaleFunc SurfaceTool::simplify_scale_func = nullptr;
SurfaceTool::GenerateRemapFunc SurfaceTool::generate_remap_func = nullptr;
SurfaceTool::RemapVertexFunc SurfaceTool::remap_vertex_func = nullptr;
SurfaceTool::RemapIndexFunc SurfaceTool::remap_index_func = nullptr;
SurfaceTool::GenerateTangentsFunc SurfaceTool::generate_tangents_func = nullptr;

void SurfaceTool::strip_mesh_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices)
{
	ERR_FAIL_COND_MSG(!generate_remap_func || !remap_vertex_func || !remap_index_func,
		"Meshoptimizer library is not initialized.");

	Vector<uint32_t> remap;
	remap.resize(r_vertices.size());
	uint32_t new_vertex_count = generate_remap_func(remap.ptrw(), (unsigned int*)r_indices.ptr(),
		r_indices.size(), r_vertices.ptr(), r_vertices.size(), sizeof(Vector3));
	remap_vertex_func(
		r_vertices.ptrw(), r_vertices.ptr(), r_vertices.size(), sizeof(Vector3), remap.ptr());
	r_vertices.resize(new_vertex_count);
	remap_index_func((unsigned int*)r_indices.ptrw(), (unsigned int*)r_indices.ptr(),
		r_indices.size(), remap.ptr());

	AHashMap<const int*, bool, TriangleHasher, TriangleHasher> found_triangles;
	int* idx_ptr = r_indices.ptrw();

	int filtered_indices_count = 0;
	for (int i = 0; i < r_indices.size() / 3; i++) {
		const int* tri = idx_ptr + (i * 3);

		if (tri[0] == tri[1] || tri[1] == tri[2] || tri[2] == tri[0]) {
			continue;
		}

		if (found_triangles.has(tri)) {
			continue;
		}

		if (i != filtered_indices_count) {
			memcpy(idx_ptr + (filtered_indices_count * 3), tri, sizeof(int) * 3);
		}

		found_triangles.insert_new(tri, true);
		filtered_indices_count++;
	}
	r_indices.resize(filtered_indices_count * 3);
}

bool SurfaceTool::Vertex::operator==(const Vertex& p_vertex) const
{
	if (vertex != p_vertex.vertex) {
		return false;
	}

	if (uv != p_vertex.uv) {
		return false;
	}

	if (uv2 != p_vertex.uv2) {
		return false;
	}

	if (normal != p_vertex.normal) {
		return false;
	}

	if (tangent != p_vertex.tangent) {
		return false;
	}

	if (color != p_vertex.color) {
		return false;
	}

	if (bones.size() != p_vertex.bones.size()) {
		return false;
	}

	for (int i = 0; i < bones.size(); i++) {
		if (bones[i] != p_vertex.bones[i]) {
			return false;
		}
	}

	for (int i = 0; i < weights.size(); i++) {
		if (weights[i] != p_vertex.weights[i]) {
			return false;
		}
	}

	for (int i = 0; i < RSE::ARRAY_CUSTOM_COUNT; i++) {
		if (custom[i] != p_vertex.custom[i]) {
			return false;
		}
	}

	if (smooth_group != p_vertex.smooth_group) {
		return false;
	}

	return true;
}

uint32_t SurfaceTool::VertexHasher::hash(const Vertex& p_vtx)
{
	uint32_t h =
		hash_djb2_buffer((const uint8_t*)p_vtx.bones.ptr(), p_vtx.bones.size() * sizeof(int));
	h = hash_djb2_buffer(
		(const uint8_t*)p_vtx.weights.ptr(), p_vtx.weights.size() * sizeof(float), h);

	const int64_t length =
		(int64_t)&p_vtx.vertex - (int64_t)&p_vtx.smooth_group + sizeof(p_vtx.vertex);
	const void* key = &p_vtx.smooth_group;
	h = hash_murmur3_buffer(key, length, h);
	return h;
}

bool SurfaceTool::SmoothGroupVertex::operator==(const SmoothGroupVertex& p_vertex) const
{
	if (vertex != p_vertex.vertex) {
		return false;
	}

	if (smooth_group != p_vertex.smooth_group) {
		return false;
	}

	return true;
}

uint32_t SurfaceTool::SmoothGroupVertexHasher::hash(const SmoothGroupVertex& p_vtx)
{
	uint32_t h = HashMapHasherDefault::hash(p_vtx.vertex);
	h = hash_murmur3_one_32(p_vtx.smooth_group, h);
	h = hash_fmix32(h);
	return h;
}

uint32_t SurfaceTool::TriangleHasher::hash(const int* p_triangle)
{
	int t0 = p_triangle[0];
	int t1 = p_triangle[1];
	int t2 = p_triangle[2];

	if (t0 > t1) {
		SWAP(t0, t1);
	}
	if (t1 > t2) {
		SWAP(t1, t2);
	}
	if (t0 > t1) {
		SWAP(t0, t1);
	}

	return (t0 * 73856093) ^ (t1 * 19349663) ^ (t2 * 83492791);
}

bool SurfaceTool::TriangleHasher::compare(const int* p_lhs, const int* p_rhs)
{
	int r0 = p_rhs[0];
	int r1 = p_rhs[1];
	int r2 = p_rhs[2];

	if (r0 > r1) {
		SWAP(r0, r1);
	}
	if (r1 > r2) {
		SWAP(r1, r2);
	}
	if (r0 > r1) {
		SWAP(r0, r1);
	}

	int l0 = p_lhs[0];
	int l1 = p_lhs[1];
	int l2 = p_lhs[2];

	if (l0 > l1) {
		SWAP(l0, l1);
	}
	if (l1 > l2) {
		SWAP(l1, l2);
	}
	if (l0 > l1) {
		SWAP(l0, l1);
	}

	return l0 == r0 && l1 == r1 && l2 == r2;
}

void SurfaceTool::begin(Mesh::PrimitiveType p_primitive)
{
	clear();

	primitive = p_primitive;
	begun = true;
	first = true;
}

void SurfaceTool::add_vertex(const Vector3& p_vertex)
{
	ERR_FAIL_COND(!begun);

	Vertex vtx;
	vtx.vertex = p_vertex;
	vtx.color = last_color;
	vtx.normal = last_normal;
	vtx.uv = last_uv;
	vtx.uv2 = last_uv2;
	vtx.weights = last_weights;
	vtx.bones = last_bones;
	vtx.tangent = last_tangent;
	vtx.smooth_group = last_smooth_group;

	for (int i = 0; i < RSE::ARRAY_CUSTOM_COUNT; i++) {
		vtx.custom[i] = last_custom[i];
	}

	const int expected_vertices = skin_weights == SKIN_8_WEIGHTS ? 8 : 4;

	if ((format & Mesh::ARRAY_FORMAT_WEIGHTS || format & Mesh::ARRAY_FORMAT_BONES) &&
		(vtx.weights.size() != expected_vertices || vtx.bones.size() != expected_vertices)) {
		// ensure vertices are the expected amount
		ERR_FAIL_COND(vtx.weights.size() != vtx.bones.size());
		if (vtx.weights.size() < expected_vertices) {
			// less than required, fill
			for (int i = vtx.weights.size(); i < expected_vertices; i++) {
				vtx.weights.push_back(0);
				vtx.bones.push_back(0);
			}
		}
		else if (vtx.weights.size() > expected_vertices) {
			// more than required, sort, cap and normalize.
			Vector<WeightSort> weights;
			for (int i = 0; i < vtx.weights.size(); i++) {
				WeightSort ws;
				ws.index = vtx.bones[i];
				ws.weight = vtx.weights[i];
				weights.push_back(ws);
			}

			// sort
			weights.sort();
			// cap
			weights.resize(expected_vertices);
			// renormalize
			float total = 0.0;
			for (int i = 0; i < expected_vertices; i++) {
				total += weights[i].weight;
			}

			vtx.weights.resize(expected_vertices);
			vtx.bones.resize(expected_vertices);

			for (int i = 0; i < expected_vertices; i++) {
				if (total > 0) {
					vtx.weights.write[i] = weights[i].weight / total;
				}
				else {
					vtx.weights.write[i] = 0;
				}
				vtx.bones.write[i] = weights[i].index;
			}
		}
	}

	vertex_array.push_back(vtx);
	first = false;

	format |= Mesh::ARRAY_FORMAT_VERTEX;
}

void SurfaceTool::set_color(Color p_color)
{
	ERR_FAIL_COND(!begun);

	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_COLOR));

	format |= Mesh::ARRAY_FORMAT_COLOR;
	last_color = p_color;
}

void SurfaceTool::set_normal(const Vector3& p_normal)
{
	ERR_FAIL_COND(!begun);

	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_NORMAL));

	format |= Mesh::ARRAY_FORMAT_NORMAL;
	last_normal = p_normal;
}

void SurfaceTool::set_tangent(const Plane& p_tangent)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_TANGENT));

	format |= Mesh::ARRAY_FORMAT_TANGENT;
	last_tangent = Vector4(p_tangent.normal.x, p_tangent.normal.y, p_tangent.normal.z, p_tangent.d);
}

void SurfaceTool::set_uv(const Vector2& p_uv)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_TEX_UV));

	format |= Mesh::ARRAY_FORMAT_TEX_UV;
	last_uv = p_uv;
}

void SurfaceTool::set_uv2(const Vector2& p_uv2)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_TEX_UV2));

	format |= Mesh::ARRAY_FORMAT_TEX_UV2;
	last_uv2 = p_uv2;
}

void SurfaceTool::set_custom(int p_channel_index, const Color& p_custom)
{
	ERR_FAIL_INDEX(p_channel_index, RSE::ARRAY_CUSTOM_COUNT);
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(last_custom_format[p_channel_index] == CUSTOM_MAX);
	static const uint32_t mask[RSE::ARRAY_CUSTOM_COUNT] = {Mesh::ARRAY_FORMAT_CUSTOM0,
		Mesh::ARRAY_FORMAT_CUSTOM1, Mesh::ARRAY_FORMAT_CUSTOM2, Mesh::ARRAY_FORMAT_CUSTOM3};
	ERR_FAIL_COND(!first && !(format & mask[p_channel_index]));

	if (first) {
		format |= mask[p_channel_index];
	}
	last_custom[p_channel_index] = p_custom;
}

void SurfaceTool::set_bones(const Vector<int>& p_bones)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_BONES));

	format |= Mesh::ARRAY_FORMAT_BONES;
	if (skin_weights == SKIN_8_WEIGHTS) {
		format |= Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	}
	last_bones = p_bones;
}

void SurfaceTool::set_weights(const Vector<float>& p_weights)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(!first && !(format & Mesh::ARRAY_FORMAT_WEIGHTS));

	format |= Mesh::ARRAY_FORMAT_WEIGHTS;
	if (skin_weights == SKIN_8_WEIGHTS) {
		format |= Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
	}
	last_weights = p_weights;
}

void SurfaceTool::set_smooth_group(uint32_t p_group) { last_smooth_group = p_group; }

void SurfaceTool::add_triangle_fan(const Vector<Vector3>& p_vertices, const Vector<Vector2>& p_uvs,
	const Vector<Color>& p_colors, const Vector<Vector2>& p_uv2s, const Vector<Vector3>& p_normals,
	const Vector<Plane>& p_tangents)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(primitive != Mesh::PRIMITIVE_TRIANGLES);
	ERR_FAIL_COND(p_vertices.size() < 3);

#define ADD_POINT(n)                                                                               \
	{                                                                                              \
		if (p_colors.size() > n)                                                                   \
			set_color(p_colors[n]);                                                                \
		if (p_uvs.size() > n)                                                                      \
			set_uv(p_uvs[n]);                                                                      \
		if (p_uv2s.size() > n)                                                                     \
			set_uv2(p_uv2s[n]);                                                                    \
		if (p_normals.size() > n)                                                                  \
			set_normal(p_normals[n]);                                                              \
		if (p_tangents.size() > n)                                                                 \
			set_tangent(p_tangents[n]);                                                            \
		add_vertex(p_vertices[n]);                                                                 \
	}

	for (int i = 0; i < p_vertices.size() - 2; i++) {
		ADD_POINT(0);
		ADD_POINT(i + 1);
		ADD_POINT(i + 2);
	}

#undef ADD_POINT
}

void SurfaceTool::add_index(int p_index)
{
	ERR_FAIL_COND(!begun);
	ERR_FAIL_COND(p_index < 0);

	format |= Mesh::ARRAY_FORMAT_INDEX;
	index_array.push_back(p_index);
}

void SurfaceTool::index()
{
	if (index_array.size()) {
		return; // already indexed
	}

	AHashMap<Vertex&, int, VertexHasher> indices = vertex_array.size();

	uint32_t new_size = 0;
	for (Vertex& vertex : vertex_array) {
		int* idxptr = indices.getptr(vertex);
		int idx;
		if (!idxptr) {
			idx = indices.size();
			vertex_array[new_size] = vertex;
			indices.insert_new(vertex_array[new_size], idx);
			new_size++;
		}
		else {
			idx = *idxptr;
		}

		index_array.push_back(idx);
	}

	vertex_array.resize(new_size);
	format |= Mesh::ARRAY_FORMAT_INDEX;
}

void SurfaceTool::deindex()
{
	if (index_array.is_empty()) {
		return; // nothing to deindex
	}

	LocalVector<Vertex> old_vertex_array(vertex_array);
	vertex_array.clear();
	for (const int& index : index_array) {
		ERR_FAIL_COND(uint32_t(index) >= old_vertex_array.size());
		vertex_array.push_back(old_vertex_array[index]);
	}
	format &= ~Mesh::ARRAY_FORMAT_INDEX;
	index_array.clear();
}

const uint32_t SurfaceTool::custom_mask[RSE::ARRAY_CUSTOM_COUNT] = {Mesh::ARRAY_FORMAT_CUSTOM0,
	Mesh::ARRAY_FORMAT_CUSTOM1, Mesh::ARRAY_FORMAT_CUSTOM2, Mesh::ARRAY_FORMAT_CUSTOM3};
const uint32_t SurfaceTool::custom_shift[RSE::ARRAY_CUSTOM_COUNT] = {
	Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT, Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT,
	Mesh::ARRAY_FORMAT_CUSTOM2_SHIFT, Mesh::ARRAY_FORMAT_CUSTOM3_SHIFT};

void SurfaceTool::create_from(const Ref<Mesh>& p_existing, int p_surface)
{
	ERR_FAIL_COND_MSG(p_existing.is_null(),
		"First argument in SurfaceTool::create_from() must be a valid object of type Mesh");

	clear();
	primitive = p_existing->surface_get_primitive_type(p_surface);
	_create_list(p_existing, p_surface, &vertex_array, &index_array, format);
	material = p_existing->surface_get_material(p_surface);

	for (int j = 0; j < RSE::ARRAY_CUSTOM_COUNT; j++) {
		if (format & custom_mask[j]) {
			last_custom_format[j] =
				(CustomFormat)((format >> custom_shift[j]) & RSE::ARRAY_FORMAT_CUSTOM_MASK);
		}
	}
}

void SurfaceTool::append_from(
	const Ref<Mesh>& p_existing, int p_surface, const Transform3D& p_xform)
{
	ERR_FAIL_COND_MSG(p_existing.is_null(),
		"First argument in SurfaceTool::append_from() must be a valid object of type Mesh");

	if (vertex_array.is_empty()) {
		primitive = p_existing->surface_get_primitive_type(p_surface);
		format = 0;
	}

	uint64_t nformat = 0;
	LocalVector<Vertex> nvertices;
	LocalVector<int> nindices;
	_create_list(p_existing, p_surface, &nvertices, &nindices, nformat);
	format |= nformat;

	for (int j = 0; j < RSE::ARRAY_CUSTOM_COUNT; j++) {
		if (format & custom_mask[j]) {
			CustomFormat new_format =
				(CustomFormat)((format >> custom_shift[j]) & RSE::ARRAY_FORMAT_CUSTOM_MASK);
			last_custom_format[j] = new_format;
		}
	}
	int vfrom = vertex_array.size();
	float flip = p_xform.basis.determinant() < 0 ? -1.0f : 1.0f;

	for (Vertex& v : nvertices) {
		v.vertex = p_xform.xform(v.vertex);
		if (nformat & RSE::ARRAY_FORMAT_NORMAL) {
			v.normal = p_xform.basis.xform(v.normal);
		}
		if (nformat & RSE::ARRAY_FORMAT_TANGENT) {
			Vector3 tangent = p_xform.basis.xform(Vector3(v.tangent.x, v.tangent.y, v.tangent.z));
			v.tangent = Vector4(tangent.x, tangent.y, tangent.z, v.tangent.w * flip);
		}

		vertex_array.push_back(v);
	}

	for (const int& index : nindices) {
		int dst_index = index + vfrom;
		index_array.push_back(dst_index);
	}
	if (index_array.size() % 3) {
		WARN_PRINT("SurfaceTool: Index array not a multiple of 3.");
	}
}

struct TangentVertex
{
	float position[3];
	float normal[3];
	float uv[2];
};

static void _propagate_tangents_or_split(LocalVector<SurfaceTool::Vertex>& r_vertex_array,
	LocalVector<int>& r_index_array, const float* p_tangents, bool p_split)
{
	// Seed each vertex with one of its corner tangents; the loop below fixes any mismatches.
	for (size_t i = 0; i < r_index_array.size(); i++) {
		Vector4 tangent(p_tangents[i * 4 + 0], p_tangents[i * 4 + 1], p_tangents[i * 4 + 2],
			p_tangents[i * 4 + 3]);
		r_vertex_array[r_index_array[i]].tangent = Vector4(p_tangents[i * 4 + 0],
			p_tangents[i * 4 + 1], p_tangents[i * 4 + 2], p_tangents[i * 4 + 3]);
	}

	if (!p_split) {
		return;
	}

	LocalVector<uint32_t> splits;
	splits.resize(r_vertex_array.size());
	memset(splits.ptr(), -1, splits.size() * sizeof(uint32_t)); // ~0u means "no split copy"

	for (size_t i = 0; i < r_index_array.size(); i++) {
		// Walk the chain of split copies looking for a vertex whose tangent matches.
		Vector4 tangent(p_tangents[i * 4 + 0], p_tangents[i * 4 + 1], p_tangents[i * 4 + 2],
			p_tangents[i * 4 + 3]);
		uint32_t v = r_index_array[i];
		while (v != ~0u && r_vertex_array[v].tangent != tangent) {
			v = splits[v];
		}

		// No match in chain: append a new split copy with the target tangent and chain it.
		if (v == ~0u) {
			v = r_vertex_array.size();
			r_vertex_array.push_back(r_vertex_array[r_index_array[i]]);
			r_vertex_array[v].tangent = tangent;
			splits.push_back(splits[r_index_array[i]]);
			splits[r_index_array[i]] = v;
		}

		r_index_array[i] = int(v);
	}
}

void SurfaceTool::_generate_tangents_bind() { generate_tangents(/*split*/ false); }

void SurfaceTool::generate_tangents(bool p_split)
{
	ERR_FAIL_COND_MSG(!generate_tangents_func, "Meshoptimizer library is not initialized.");
	ERR_FAIL_COND_MSG(
		!(format & Mesh::ARRAY_FORMAT_TEX_UV), "UVs are required to generate tangents.");
	ERR_FAIL_COND(!(format & Mesh::ARRAY_FORMAT_NORMAL));
	ERR_FAIL_COND(primitive != Mesh::PRIMITIVE_TRIANGLES);

	if (vertex_array.is_empty()) {
		format |= Mesh::ARRAY_FORMAT_TANGENT;
		return;
	}

	size_t corner_count = index_array.size() > 0 ? index_array.size() : vertex_array.size();
	ERR_FAIL_COND(corner_count % 3 != 0);

	// We can't operate on SurfaceTool::Vertex directly because in double-precision builds, vectors
	// use double components So we convert the inputs to single precision floats before generating
	// tangents.
	LocalVector<TangentVertex> tangent_vertices;
	tangent_vertices.resize(vertex_array.size());

	for (size_t i = 0; i < vertex_array.size(); i++) {
		const Vertex& vertex = vertex_array[i];
		TangentVertex& tangent_vertex = tangent_vertices[i];

		tangent_vertex.position[0] = vertex.vertex.x;
		tangent_vertex.position[1] = vertex.vertex.y;
		tangent_vertex.position[2] = vertex.vertex.z;
		tangent_vertex.normal[0] = vertex.normal.x;
		tangent_vertex.normal[1] = vertex.normal.y;
		tangent_vertex.normal[2] = vertex.normal.z;
		tangent_vertex.uv[0] = vertex.uv.x;
		tangent_vertex.uv[1] = vertex.uv.y;
	}

	const TangentVertex* vertices = tangent_vertices.ptr();

	LocalVector<float> tangents;
	tangents.resize(corner_count * 4);

	generate_tangents_func(tangents.ptr(),
		index_array.size() > 0 ? reinterpret_cast<const unsigned int*>(index_array.ptr()) : nullptr,
		corner_count, vertices->position, tangent_vertices.size(), sizeof(TangentVertex),
		vertices->normal, sizeof(TangentVertex), vertices->uv, sizeof(TangentVertex), 0);

	if (index_array.size() > 0) {
		_propagate_tangents_or_split(vertex_array, index_array, tangents.ptr(), p_split);
	}
	else {
		for (size_t i = 0; i < corner_count; i++) {
			Vector4 tangent(
				tangents[i * 4 + 0], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]);
			vertex_array[i].tangent = tangent;
		}
	}

	format |= Mesh::ARRAY_FORMAT_TANGENT;
}

void SurfaceTool::generate_normals(bool p_flip)
{
	ERR_FAIL_COND(primitive != Mesh::PRIMITIVE_TRIANGLES);

	bool was_indexed = index_array.size();

	deindex();

	ERR_FAIL_COND((vertex_array.size() % 3) != 0);

	AHashMap<SmoothGroupVertex, Vector3, SmoothGroupVertexHasher> smooth_hash = vertex_array.size();

	for (uint32_t vi = 0; vi < vertex_array.size(); vi += 3) {
		Vertex* v = &vertex_array[vi];

		Vector3 normal;
		if (!p_flip) {
			normal = Plane(v[0].vertex, v[1].vertex, v[2].vertex).normal;
		}
		else {
			normal = Plane(v[2].vertex, v[1].vertex, v[0].vertex).normal;
		}

		for (int i = 0; i < 3; i++) {
			// Add face normal to smooth vertex influence if vertex is member of a smoothing group
			if (v[i].smooth_group != UINT32_MAX) {
				Vector3* lv = smooth_hash.getptr(v[i]);
				if (!lv) {
					smooth_hash.insert_new(v[i], normal);
				}
				else {
					(*lv) += normal;
				}
			}
			else {
				v[i].normal = normal;
			}
		}
	}

	for (Vertex& vertex : vertex_array) {
		if (vertex.smooth_group != UINT32_MAX) {
			Vector3* lv = smooth_hash.getptr(vertex);
			if (!lv) {
				vertex.normal = Vector3();
			}
			else {
				vertex.normal = lv->normalized();
			}
		}
	}

	format |= Mesh::ARRAY_FORMAT_NORMAL;

	if (was_indexed) {
		index();
	}
}

void SurfaceTool::set_material(const Ref<Material>& p_material) { material = p_material; }

Ref<Material> SurfaceTool::get_material() const { return material; }

void SurfaceTool::clear()
{
	begun = false;
	primitive = Mesh::PRIMITIVE_LINES;
	format = 0;
	last_bones.clear();
	last_weights.clear();
	index_array.clear();
	vertex_array.clear();
	material.unref();
	last_smooth_group = 0;
	for (int i = 0; i < RSE::ARRAY_CUSTOM_COUNT; i++) {
		last_custom_format[i] = CUSTOM_MAX;
	}
	skin_weights = SKIN_4_WEIGHTS;
}

void SurfaceTool::set_skin_weight_count(SkinWeightCount p_weights)
{
	ERR_FAIL_COND(begun);
	skin_weights = p_weights;
}

SurfaceTool::SkinWeightCount SurfaceTool::get_skin_weight_count() const { return skin_weights; }

void SurfaceTool::set_custom_format(int p_channel_index, CustomFormat p_format)
{
	ERR_FAIL_INDEX(p_channel_index, RSE::ARRAY_CUSTOM_COUNT);
	ERR_FAIL_COND(!begun);
	ERR_FAIL_INDEX(p_format, CUSTOM_MAX + 1);
	last_custom_format[p_channel_index] = p_format;
}

Mesh::PrimitiveType SurfaceTool::get_primitive_type() const { return primitive; }

SurfaceTool::CustomFormat SurfaceTool::get_custom_format(int p_channel_index) const
{
	ERR_FAIL_INDEX_V(p_channel_index, RSE::ARRAY_CUSTOM_COUNT, CUSTOM_MAX);
	return last_custom_format[p_channel_index];
}

void SurfaceTool::optimize_indices_for_cache()
{
	ERR_FAIL_NULL(optimize_vertex_cache_func);
	ERR_FAIL_COND(index_array.is_empty());
	ERR_FAIL_COND(primitive != Mesh::PRIMITIVE_TRIANGLES);
	ERR_FAIL_COND(index_array.size() % 3 != 0);

	LocalVector<int> old_index_array(index_array);
	memset(index_array.ptr(), 0, index_array.size() * sizeof(int));
	optimize_vertex_cache_func((unsigned int*)index_array.ptr(),
		(unsigned int*)old_index_array.ptr(), old_index_array.size(), vertex_array.size());
}

AABB SurfaceTool::get_aabb() const
{
	ERR_FAIL_COND_V(vertex_array.is_empty(), AABB());

	AABB aabb;
	for (uint32_t i = 0; i < vertex_array.size(); i++) {
		if (i == 0) {
			aabb.position = vertex_array[i].vertex;
		}
		else {
			aabb.expand_to(vertex_array[i].vertex);
		}
	}

	return aabb;
}

Vector<int> SurfaceTool::generate_lod(float p_threshold, int p_target_index_count)
{
	WARN_DEPRECATED_MSG(
		R"*(The "SurfaceTool.generate_lod()" method is deprecated. Consider using "ImporterMesh.generate_lods()" instead.)*");

	Vector<int> lod;

	ERR_FAIL_NULL_V(simplify_func, lod);
	ERR_FAIL_COND_V(p_target_index_count < 0, lod);
	ERR_FAIL_COND_V(vertex_array.is_empty(), lod);
	ERR_FAIL_COND_V(index_array.is_empty(), lod);
	ERR_FAIL_COND_V(index_array.size() % 3 != 0, lod);
	ERR_FAIL_COND_V(index_array.size() < (unsigned int)p_target_index_count, lod);

	lod.resize(index_array.size());
	LocalVector<float> vertices; // uses floats
	vertices.resize(vertex_array.size() * 3);
	for (uint32_t i = 0; i < vertex_array.size(); i++) {
		vertices[i * 3 + 0] = vertex_array[i].vertex.x;
		vertices[i * 3 + 1] = vertex_array[i].vertex.y;
		vertices[i * 3 + 2] = vertex_array[i].vertex.z;
	}

	float error;
	const int simplify_options = SIMPLIFY_LOCK_BORDER;
	uint32_t index_count = simplify_func((unsigned int*)lod.ptrw(),
		(unsigned int*)index_array.ptr(), index_array.size(), vertices.ptr(), vertex_array.size(),
		sizeof(float) * 3, p_target_index_count, p_threshold, simplify_options, &error);
	ERR_FAIL_COND_V(index_count == 0, lod);
	lod.resize(index_count);

	return lod;
}

SurfaceTool::SurfaceTool()
{
	for (int i = 0; i < RSE::ARRAY_CUSTOM_COUNT; i++) {
		last_custom_format[i] = CUSTOM_MAX;
	}
}



void SurfaceTool::_create_list(Ref<Mesh> const&, int, LocalVector<SurfaceTool::Vertex, unsigned int, false, false>*, LocalVector<int, unsigned int, false, false>*, unsigned long&) {}

Ref<ArrayMesh> SurfaceTool::commit(Ref<ArrayMesh> const&, unsigned long) {}

#ifdef EQ_VERTEX_DIST
#undef EQ_VERTEX_DIST
#endif

// ============================================================================
// Merged from scene/resources/packed_scene.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/templates/local_vector.h"
#include "packed_scene.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/property_utils.h"

#ifndef _3D_DISABLED
#include "scene/3d/node_3d.h"
#endif // _3D_DISABLED

#define PACKED_SCENE_VERSION 3

#ifdef TOOLS_ENABLED
SceneState::InstantiationWarningNotify SceneState::instantiation_warn_notify = nullptr;
#endif

bool SceneState::can_instantiate() const { return nodes.size() > 0; }

static Node* _find_node_by_id(Node* p_owner, Node* p_node, int32_t p_id)
{
	if (p_owner == p_node || p_node->get_owner() == p_owner) {
		if (p_node->get_unique_scene_id() == p_id) {
			return p_node;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* found = _find_node_by_id(p_owner, p_node->get_child(i), p_id);
		if (found) {
			return found;
		}
	}

	return nullptr;
}

static int _nm_get_string(const String& p_string, HashMap<String, int>& name_map)
{
	if (name_map.has(p_string)) {
		return name_map[p_string];
	}

	int idx = name_map.size();
	name_map[p_string] = idx;
	return idx;
}

void SceneState::set_path(const String& p_path) { path = p_path; }

String SceneState::get_path() const { return path; }

int SceneState::find_node_by_path(const NodePath& p_node) const
{
	ERR_FAIL_COND_V_MSG(node_path_cache.is_empty(), -1,
		"This operation requires the node cache to have been built.");

	if (!node_path_cache.has(p_node)) {
		// If not in this scene state, find node path by scene inheritance.
		if (get_base_scene_state().is_valid()) {
			int idx = get_base_scene_state()->find_node_by_path(p_node);
			if (idx != -1) {
				int rkey = _find_base_scene_node_remap_key(idx);
				if (rkey == -1) {
					rkey = nodes.size() + base_scene_node_remap.size();
					base_scene_node_remap[rkey] = idx;
				}
				return rkey;
			}
		}
		return -1;
	}

	int nid = node_path_cache[p_node];

	if (get_base_scene_state().is_valid() && !base_scene_node_remap.has(nid)) {
		// for nodes that _do_ exist in current scene, still try to look for
		// the node in the instantiated scene, as a property may be missing
		// from the local one
		int idx = get_base_scene_state()->find_node_by_path(p_node);
		if (idx != -1) {
			base_scene_node_remap[nid] = idx;
		}
	}

	return nid;
}

int SceneState::_find_base_scene_node_remap_key(int p_idx) const
{
	for (const KeyValue<int, int>& E : base_scene_node_remap) {
		if (E.value == p_idx) {
			return E.key;
		}
	}
	return -1;
}

bool SceneState::is_node_in_group(int p_node, const String& p_group) const
{
	ERR_FAIL_COND_V(p_node < 0, false);

	if (p_node < nodes.size()) {
		const String* namep = names.ptr();
		for (int i = 0; i < nodes[p_node].groups.size(); i++) {
			if (namep[nodes[p_node].groups[i]] == p_group) {
				return true;
			}
		}
	}

	if (base_scene_node_remap.has(p_node)) {
		return get_base_scene_state()->is_node_in_group(base_scene_node_remap[p_node], p_group);
	}

	return false;
}

bool SceneState::disable_placeholders = false;

void SceneState::set_disable_placeholders(bool p_disable) { disable_placeholders = p_disable; }

bool SceneState::is_connection(
	int p_node, const String& p_signal, int p_to_node, const String& p_to_method) const
{
	ERR_FAIL_COND_V(p_node < 0, false);
	ERR_FAIL_COND_V(p_to_node < 0, false);

	if (p_node < nodes.size() && p_to_node < nodes.size()) {
		int signal_idx = -1;
		int method_idx = -1;
		for (int i = 0; i < names.size(); i++) {
			if (names[i] == p_signal) {
				signal_idx = i;
			}
			else if (names[i] == p_to_method) {
				method_idx = i;
			}
		}

		if (signal_idx >= 0 && method_idx >= 0) {
			// signal and method strings are stored..

			for (int i = 0; i < connections.size(); i++) {
				if (connections[i].from == p_node && connections[i].to == p_to_node &&
					connections[i].signal == signal_idx && connections[i].method == method_idx) {
					return true;
				}
			}
		}
	}

	if (base_scene_node_remap.has(p_node) && base_scene_node_remap.has(p_to_node)) {
		return get_base_scene_state()->is_connection(
			base_scene_node_remap[p_node], p_signal, base_scene_node_remap[p_to_node], p_to_method);
	}

	return false;
}

int SceneState::get_node_count() const { return nodes.size(); }

String SceneState::get_node_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
	if (nodes[p_idx].type == TYPE_INSTANTIATED) {
		return String();
	}
	return names[nodes[p_idx].type];
}

String SceneState::get_node_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
	return names[nodes[p_idx].name];
}

int SceneState::get_node_index(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), -1);
	return nodes[p_idx].index;
}

bool SceneState::is_node_instance_placeholder(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), false);

	return nodes[p_idx].instance >= 0 && (nodes[p_idx].instance & FLAG_INSTANCE_IS_PLACEHOLDER);
}

Vector<String> SceneState::get_node_groups(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), Vector<String>());
	Vector<String> groups;
	for (int i = 0; i < nodes[p_idx].groups.size(); i++) {
		groups.push_back(names[nodes[p_idx].groups[i]]);
	}
	return groups;
}

int32_t SceneState::get_node_unique_id(int p_idx) const
{
	if (p_idx >= ids.size()) {
		return Node::UNIQUE_SCENE_ID_UNASSIGNED;
	}
	return ids[p_idx];
}

NodePath SceneState::get_node_path(int p_idx, bool p_for_parent) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), NodePath());

	if (nodes[p_idx].parent < 0 || nodes[p_idx].parent == NO_PARENT_SAVED) {
		if (p_for_parent) {
			return NodePath();
		}
		else {
			return NodePath(".");
		}
	}

	Vector<String> sub_path;
	NodePath base_path;
	int nidx = p_idx;
	while (true) {
		if (nodes[nidx].parent == NO_PARENT_SAVED || nodes[nidx].parent < 0) {
			sub_path.insert(0, ".");
			break;
		}

		if (!p_for_parent || p_idx != nidx) {
			sub_path.insert(0, names[nodes[nidx].name]);
		}

		if (nodes[nidx].parent & FLAG_ID_IS_PATH) {
			base_path = node_paths[nodes[nidx].parent & FLAG_MASK];
			break;
		}
		else {
			nidx = nodes[nidx].parent & FLAG_MASK;
		}
	}

	for (int i = base_path.get_name_count() - 1; i >= 0; i--) {
		sub_path.insert(0, base_path.get_name(i));
	}

	if (sub_path.is_empty()) {
		return NodePath(".");
	}

	return NodePath(sub_path, false);
}

PackedInt32Array SceneState::get_node_id_path(int p_idx) const
{
	PackedInt32Array pp = get_node_parent_id_path(p_idx);
	if (pp.is_empty()) {
		return pp;
	}

	if (p_idx < ids.size()) {
		pp.push_back(ids[p_idx]);
		return pp;
	}

	return PackedInt32Array();
}

PackedInt32Array SceneState::get_node_parent_id_path(int p_idx) const
{
	if (nodes[p_idx].parent < 0 || nodes[p_idx].parent == NO_PARENT_SAVED) {
		return PackedInt32Array();
	}

	if (nodes[p_idx].parent & FLAG_ID_IS_PATH) {
		int id = nodes[p_idx].parent & FLAG_MASK;
		if (id >= id_paths.size()) {
			return PackedInt32Array();
		}
		return id_paths[id];
	}

	return PackedInt32Array();
}

PackedInt32Array SceneState::get_node_owner_id_path(int p_idx) const
{
	if (nodes[p_idx].owner < 0) {
		return PackedInt32Array();
	}

	if (nodes[p_idx].owner & FLAG_ID_IS_PATH) {
		int id = nodes[p_idx].owner & FLAG_MASK;
		if (id >= id_paths.size()) {
			return PackedInt32Array();
		}
		return id_paths[id];
	}

	return PackedInt32Array();
}

int SceneState::get_node_property_count(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), -1);
	return nodes[p_idx].properties.size();
}

String SceneState::get_node_property_name(int p_idx, int p_prop) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
	ERR_FAIL_INDEX_V(p_prop, nodes[p_idx].properties.size(), String());
	return names[nodes[p_idx].properties[p_prop].name & FLAG_PROP_NAME_MASK];
}

Vector<String> SceneState::get_node_deferred_nodepath_properties(int p_idx) const
{
	Vector<String> ret;
	ERR_FAIL_COND_V(p_idx < 0, ret);

	if (p_idx < nodes.size()) {
		// Find in built-in nodes.
		for (int i = 0; i < nodes[p_idx].properties.size(); i++) {
			uint32_t idx = nodes[p_idx].properties[i].name;
			if (idx & FLAG_PATH_PROPERTY_IS_NODE) {
				ret.push_back(names[idx & FLAG_PROP_NAME_MASK]);
			}
		}
		return ret;
	}

	// Property not found, try on instance.
	HashMap<int, int>::ConstIterator I = base_scene_node_remap.find(p_idx);
	if (I) {
		return get_base_scene_state()->get_node_deferred_nodepath_properties(I->value);
	}

	return ret;
}

NodePath SceneState::get_node_owner_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), NodePath());
	if (nodes[p_idx].owner < 0 || nodes[p_idx].owner == NO_PARENT_SAVED) {
		return NodePath(); // root likely
	}
	if (nodes[p_idx].owner & FLAG_ID_IS_PATH) {
		return node_paths[nodes[p_idx].owner & FLAG_MASK];
	}
	else {
		return get_node_path(nodes[p_idx].owner & FLAG_MASK);
	}
}

int SceneState::get_connection_count() const { return connections.size(); }

NodePath SceneState::get_connection_source(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), NodePath());
	if (connections[p_idx].from & FLAG_ID_IS_PATH) {
		return node_paths[connections[p_idx].from & FLAG_MASK];
	}
	else {
		return get_node_path(connections[p_idx].from & FLAG_MASK);
	}
}

String SceneState::get_connection_signal(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), String());
	return names[connections[p_idx].signal];
}

NodePath SceneState::get_connection_target(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), NodePath());
	if (connections[p_idx].to & FLAG_ID_IS_PATH) {
		return node_paths[connections[p_idx].to & FLAG_MASK];
	}
	else {
		return get_node_path(connections[p_idx].to & FLAG_MASK);
	}
}

PackedInt32Array SceneState::get_connection_target_id_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), PackedInt32Array());
	if (connections[p_idx].to & FLAG_ID_IS_PATH && connections[p_idx].to < id_paths.size()) {
		return id_paths[connections[p_idx].to];
	}
	else {
		return PackedInt32Array();
	}
}

PackedInt32Array SceneState::get_connection_source_id_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), PackedInt32Array());
	if (connections[p_idx].from & FLAG_ID_IS_PATH && connections[p_idx].from < id_paths.size()) {
		return id_paths[connections[p_idx].from];
	}
	else {
		return PackedInt32Array();
	}
}

String SceneState::get_connection_method(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), String());
	return names[connections[p_idx].method];
}

int SceneState::get_connection_flags(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), -1);
	return connections[p_idx].flags;
}

int SceneState::get_connection_unbinds(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), -1);
	return connections[p_idx].unbinds;
}

bool SceneState::has_connection(const NodePath& p_node_from, const String& p_signal,
	const NodePath& p_node_to, const String& p_method, bool p_no_inheritance)
{
	// this method cannot be const because of this
	Ref<SceneState> ss = this;

	do {
		for (int i = 0; i < ss->connections.size(); i++) {
			const ConnectionData& c = ss->connections[i];

			NodePath np_from;

			if (c.from & FLAG_ID_IS_PATH) {
				np_from = ss->node_paths[c.from & FLAG_MASK];
			}
			else {
				np_from = ss->get_node_path(c.from);
			}

			NodePath np_to;

			if (c.to & FLAG_ID_IS_PATH) {
				np_to = ss->node_paths[c.to & FLAG_MASK];
			}
			else {
				np_to = ss->get_node_path(c.to);
			}

			String sn_signal = ss->names[c.signal];
			String sn_method = ss->names[c.method];

			if (np_from == p_node_from && sn_signal == p_signal && np_to == p_node_to &&
				sn_method == p_method) {
				return true;
			}
		}

		if (p_no_inheritance) {
			break;
		}

		ss = ss->get_base_scene_state();
	} while (ss.is_valid());

	return false;
}

Vector<NodePath> SceneState::get_editable_instances() const { return editable_instances; }

int SceneState::add_name(const String& p_name)
{
	names.push_back(p_name);
	return names.size() - 1;
}

int SceneState::add_node_path(const NodePath& p_path, const PackedInt32Array& p_uid_path)
{
	node_paths.push_back(p_path);
	id_paths.push_back(p_uid_path);
	return (node_paths.size() - 1) | FLAG_ID_IS_PATH;
}

int SceneState::add_node(int p_parent, int p_owner, int p_type, int p_name, int p_instance,
	int p_index, int32_t p_unique_id)
{
	NodeData nd;
	nd.parent = p_parent;
	nd.owner = p_owner;
	nd.type = p_type;
	nd.name = p_name;
	nd.instance = p_instance;
	nd.index = p_index;

	nodes.push_back(nd);

	ids.push_back(p_unique_id);

	return nodes.size() - 1;
}

void SceneState::add_node_group(int p_node, int p_group)
{
	ERR_FAIL_INDEX(p_node, nodes.size());
	ERR_FAIL_INDEX(p_group, names.size());
	nodes.write[p_node].groups.push_back(p_group);
}

void SceneState::add_editable_instance(const NodePath& p_path)
{
	editable_instances.push_back(p_path);
}

bool SceneState::remove_group_references(const String& p_name)
{
	bool edited = false;
	for (NodeData& node : nodes) {
		for (const int& group : node.groups) {
			if (names[group] == p_name) {
				node.groups.erase(group);
				edited = true;
				break;
			}
		}
	}
	return edited;
}

bool SceneState::rename_group_references(const String& p_old_name, const String& p_new_name)
{
	bool edited = false;
	for (const NodeData& node : nodes) {
		for (const int& group : node.groups) {
			if (names[group] == p_old_name) {
				names.write[group] = p_new_name;
				edited = true;
				break;
			}
		}
	}
	return edited;
}

HashSet<String> SceneState::get_all_groups()
{
	HashSet<String> ret;
	for (const NodeData& node : nodes) {
		for (const int& group : node.groups) {
			ret.insert(names[group]);
		}
	}
	return ret;
}

Vector<String> SceneState::_get_node_groups(int p_idx) const
{
	Vector<String> groups = get_node_groups(p_idx);
	Vector<String> ret;

	for (int i = 0; i < groups.size(); i++) {
		ret.push_back(groups[i]);
	}

	return ret;
}

Error PackedScene::pack(Node*
p_scene) { return state->pack(p_scene); }

void PackedScene::clear() { state->clear(); }

bool PackedScene::can_instantiate() const { return state->can_instantiate(); }

Ref<SceneState> PackedScene::get_state() const { return state; }

void PackedScene::set_path(const String& p_path, bool p_take_over)
{
	state->set_path(p_path);
	Resource::set_path(p_path, p_take_over);
}

void PackedScene::set_path_cache(const String& p_path)
{
	state->set_path(p_path);
	Resource::set_path_cache(p_path);
}

void PackedScene::reset_state() { clear(); }

PackedScene::PackedScene() { state.instantiate(); }

Ref<SceneState> SceneState::get_base_scene_state() const
{
	Ref<SceneState> ss = memnew(SceneState);
	return ss;
}

Node* PackedScene::instantiate(PackedScene::GenEditState) const {}

Error SceneState::pack(Node*) {}

void SceneState::clear() {}

void PackedScene::reload_from_file() {}

#ifdef PACKED_SCENE_VERSION
#undef PACKED_SCENE_VERSION
#endif

// --- resource_format_text.cpp ---

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "scene/resources/packed_scene.h"
#include "scene/property_utils.h"

void ResourceLoaderText::_printerr()
{
	ERR_PRINT(vformat("%s:%d - Parse Error: %s.", res_path, lines, error_text));
}

Ref<Resource> ResourceLoaderText::get_resource() { return resource; }

void ResourceLoaderText::_count_resources()
{
	Ref<FileAccess> scan_f = FileAccess::open(f->get_path(), FileAccess::READ);
	if (scan_f.is_null()) {
		return;
	}

	resources_total = 0;
	resource_current = 0;

	bool has_main_resource = false;
	while (!scan_f->eof_reached()) {
		String line = scan_f->get_line().strip_edges();

		// Only count resources that contribute to progress
		// (ext_resources are loaded asynchronously and don't count).
		// Note: nodes are all parsed together as part of the main resource (PackedScene),
		// so they only contribute 1 to the progress count, not one per node.
		if (line.begins_with("[sub_resource ")) {
			resources_total++;
		}
		else if (line.begins_with("[resource]") || line.begins_with("[node ")) {
			// Main resource or scene with nodes - only count once.
			if (!has_main_resource) {
				resources_total++;
				has_main_resource = true;
			}
		}
	}
}

int ResourceLoaderText::get_stage() const { return resource_current; }

int ResourceLoaderText::get_stage_count() const
{
	return resources_total; //+ext_resources;
}

Ref<Resource> ResourceFormatLoaderText::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Error err;

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot open file '" + p_path + "'.");

	ResourceLoaderText loader;
	String path = !p_original_path.is_empty() ? p_original_path : p_path;
	switch (p_cache_mode) {
	case CACHE_MODE_IGNORE:
	case CACHE_MODE_REUSE:
	case CACHE_MODE_REPLACE:
		loader.cache_mode = p_cache_mode;
		loader.cache_mode_for_external = CACHE_MODE_REUSE;
		break;
	case CACHE_MODE_IGNORE_DEEP:
		loader.cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	case CACHE_MODE_REPLACE_DEEP:
		loader.cache_mode = ResourceFormatLoader::CACHE_MODE_REPLACE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	}
	loader.use_sub_threads = p_use_sub_threads;
	loader.local_path = ProjectSettings::localize_path(path);
	loader.progress = r_progress;
	loader.res_path = loader.local_path;
	loader.open(f);
	err = loader.load();
	if (r_error) {
		*r_error = err;
	}
	if (err == OK) {
		return loader.get_resource();
	}
	else {
		return Ref<Resource>();
	}
}

void ResourceFormatLoaderText::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	// Don't allow .tres for PackedScenes or GDExtension.
	if (p_type != "PackedScene" && p_type != "GDExtension") {
		p_extensions->push_back("tres");
	}
}

void ResourceFormatLoaderText::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("tscn");
	p_extensions->push_back("tres");
}

bool ResourceFormatLoaderText::handles_type(const String& p_type) const { return true; }

void ResourceFormatLoaderText::get_classes_used(
	const String& p_path, HashSet<String>* r_classes)
{
	const String type = get_resource_type(p_path);
	if (!type.is_empty()) {
		r_classes->insert(type);
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return; // Could not read.
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.open(f);
	loader.get_classes_used(r_classes);
}

String ResourceFormatLoaderText::get_resource_type(const String& p_path) const
{
	const String ext = p_path.get_extension().to_lower();
	if (ext == "tscn") {
		return "PackedScene";
	}
	else if (ext != "tres") {
		return String();
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	String r = loader.recognize(f);
	return r;
}

String ResourceFormatLoaderText::get_resource_script_class(const String& p_path) const
{
	if (!p_path.has_extension("tres")) {
		return String();
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.recognize_script_class(f);
}

ResourceUID::ID ResourceFormatLoaderText::get_resource_uid(const String& p_path) const
{
	const String ext = p_path.get_extension().to_lower();
	if (ext != "tscn" && ext != "tres") {
		return ResourceUID::INVALID_ID;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ResourceUID::INVALID_ID; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.get_uid(f);
}

bool ResourceFormatLoaderText::has_custom_uid_support() const { return true; }

void ResourceFormatLoaderText::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		ERR_FAIL();
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_dependencies(f, p_dependencies, p_add_types);
}

Error ResourceFormatLoaderText::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	Error err = OK;
	{
		Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
		if (f.is_null()) {
			ERR_FAIL_V(ERR_CANT_OPEN);
		}

		ResourceLoaderText loader;
		loader.local_path = ProjectSettings::localize_path(p_path);
		loader.res_path = loader.local_path;
		err = loader.rename_dependencies(f, p_path, p_map);
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (err == OK && da->file_exists(p_path + ".depren")) {
		da->remove(p_path);
		da->rename(p_path + ".depren", p_path);
	}

	return err;
}

String ResourceFormatSaverTextInstance::_write_resources(void* ud, const Ref<Resource>& p_resource)
{
	ResourceFormatSaverTextInstance* rsi = static_cast<ResourceFormatSaverTextInstance*>(ud);
	return rsi->_write_resource(p_resource);
}

Error ResourceLoaderText::set_uid(Ref<FileAccess> p_f, ResourceUID::ID p_uid)
{
	open(p_f, true);
	ERR_FAIL_COND_V(error != OK, error);
	ignore_resource_parsing = true;

	Ref<FileAccess> fw;

	fw = FileAccess::open(local_path + ".uidren", FileAccess::WRITE);
	if (is_scene) {
		fw->store_string("[gd_scene format=" + itos(format_version) + " uid=\"" +
						 ResourceUID::id_to_text(p_uid) + "\"]");
	}
	else {
		String script_res_text;
		if (!script_class.is_empty()) {
			script_res_text = "script_class=\"" + script_class + "\" ";
		}

		fw->store_string("[gd_resource type=\"" + res_type + "\" " + script_res_text +
						 "format=" + itos(format_version) + " uid=\"" +
						 ResourceUID::id_to_text(p_uid) + "\"]");
	}

	uint8_t c = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(c);
		c = f->get_8();
	}

	bool all_ok = fw->get_error() == OK;

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Error ResourceFormatSaverText::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	if (p_path.ends_with(".tscn") && Ref<PackedScene>(p_resource).is_null()) {
		return ERR_FILE_UNRECOGNIZED;
	}

	ResourceFormatSaverTextInstance saver;
	return saver.save(p_path, p_resource, p_flags);
}

Error ResourceFormatSaverText::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String lc = p_path.to_lower();
	if (!lc.ends_with(".tscn") && !lc.ends_with(".tres")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	String local_path = ProjectSettings::localize_path(p_path);
	Error err = OK;
	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null()) {
			ERR_FAIL_V(ERR_CANT_OPEN);
		}

		ResourceLoaderText loader;
		loader.local_path = local_path;
		loader.res_path = loader.local_path;
		err = loader.set_uid(file, p_uid);
	}

	if (err == OK) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		da->remove(local_path);
		da->rename(local_path + ".uidren", local_path);
	}

	return err;
}

bool ResourceFormatSaverText::recognize(const Ref<Resource>& p_resource) const
{
	return true; // All resources recognized!
}

void ResourceFormatSaverText::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	if (Ref<PackedScene>(p_resource).is_valid()) {
		p_extensions->push_back("tscn"); // Text scene.
	}
	else {
		p_extensions->push_back("tres"); // Text resource.
	}
}

void ResourceLoaderText::open(Ref<FileAccess>, bool) {}

Error ResourceLoaderText::rename_dependencies(Ref<FileAccess>, String const&,
	HashMap<String, String, HashMapHasherDefault, HashMapComparatorDefault<String, void>,
		DefaultTypedAllocator<HashMapElement<String, String>>> const&)
{
	return OK;
}

String ResourceLoaderText::recognize_script_class(Ref<FileAccess>) { return String(); }

String ResourceLoaderText::recognize(Ref<FileAccess>) { return String(); }

Error ResourceLoaderText::load() { return OK; }

ResourceUID::ID ResourceLoaderText::get_uid(Ref<FileAccess>) { return ResourceUID::ID(); }

void ResourceLoaderText::get_dependencies(Ref<FileAccess>, List<String, DefaultAllocator>*, bool) {}

Error ResourceLoaderText::get_classes_used(
	HashSet<String, HashMapHasherDefault, HashMapComparatorDefault<String, void>>*)
{
	return OK;
}

String ResourceFormatSaverTextInstance::_write_resource(Ref<Resource> const&) { return String(); }

Error ResourceFormatSaverTextInstance::save(String const&, Ref<Resource> const&, unsigned int)
{
	return OK;
}

// --- audio_stream_wav.cpp ---

#include "audio_stream_wav.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/marshalls.h"

const float TRIM_DB_LIMIT = -50;
const int TRIM_FADE_OUT_FRAMES = 500;

void AudioStreamPlaybackWAV::start(double p_from_pos)
{
	if (base->format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		// no seeking in IMA_ADPCM
		for (int i = 0; i < 2; i++) {
			ima_adpcm[i].step_index = 0;
			ima_adpcm[i].predictor = 0;
			ima_adpcm[i].loop_step_index = 0;
			ima_adpcm[i].loop_predictor = 0;
			ima_adpcm[i].last_nibble = -1;
			ima_adpcm[i].loop_pos = 0x7FFFFFFF;
			ima_adpcm[i].window_ofs = 0;
		}

		offset = 0;
	}
	else {
		seek(p_from_pos);
	}

	sign = 1;
	active = true;
	begin_resample();
}

void AudioStreamPlaybackWAV::stop() { active = false; }

bool AudioStreamPlaybackWAV::is_playing() const { return active; }

int AudioStreamPlaybackWAV::get_loop_count() const { return 0; }

double AudioStreamPlaybackWAV::get_playback_position() const
{
	return double(offset) / base->mix_rate;
}

void AudioStreamPlaybackWAV::seek(double p_time)
{
	if (base->format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		return; // no seeking in ima-adpcm
	}

	double max = base->get_length();
	if (p_time < 0) {
		p_time = 0;
	}
	else if (p_time >= max) {
		p_time = max - 0.001;
	}

	offset = int64_t(p_time * base->mix_rate);
}

template <typename Depth, bool is_stereo, bool is_ima_adpcm, bool is_qoa>
void AudioStreamPlaybackWAV::decode_samples(const Depth* p_src, AudioFrame* p_dst,
	int64_t& p_offset, int8_t& p_increment, uint32_t p_amount, IMA_ADPCM_State* p_ima_adpcm,
	QOA_State* p_qoa)
{
	// this function will be compiled branchless by any decent compiler

	int32_t final = 0, final_r = 0;
	while (p_amount) {
		p_amount--;
		int64_t pos = p_offset << (is_stereo && !is_ima_adpcm && !is_qoa ? 1 : 0);

		if (is_ima_adpcm) {
			int64_t sample_pos = pos + p_ima_adpcm[0].window_ofs;

			while (sample_pos > p_ima_adpcm[0].last_nibble) {
				static const int16_t _ima_adpcm_step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16,
					17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107,
					118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494,
					544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
					2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
					7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
					22385, 24623, 27086, 29794, 32767};

				static const int8_t _ima_adpcm_index_table[16] = {
					-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

				for (int i = 0; i < (is_stereo ? 2 : 1); i++) {
					int16_t nibble, diff, step;

					p_ima_adpcm[i].last_nibble++;

					uint8_t nbb =
						p_src[(p_ima_adpcm[i].last_nibble >> 1) * (is_stereo ? 2 : 1) + i];
					nibble = (p_ima_adpcm[i].last_nibble & 1) ? (nbb >> 4) : (nbb & 0xF);
					step = _ima_adpcm_step_table[p_ima_adpcm[i].step_index];

					p_ima_adpcm[i].step_index += _ima_adpcm_index_table[nibble];
					if (p_ima_adpcm[i].step_index < 0) {
						p_ima_adpcm[i].step_index = 0;
					}
					if (p_ima_adpcm[i].step_index > 88) {
						p_ima_adpcm[i].step_index = 88;
					}

					diff = step >> 3;
					if (nibble & 1) {
						diff += step >> 2;
					}
					if (nibble & 2) {
						diff += step >> 1;
					}
					if (nibble & 4) {
						diff += step;
					}
					if (nibble & 8) {
						diff = -diff;
					}

					p_ima_adpcm[i].predictor += diff;
					if (p_ima_adpcm[i].predictor < -0x8000) {
						p_ima_adpcm[i].predictor = -0x8000;
					}
					else if (p_ima_adpcm[i].predictor > 0x7FFF) {
						p_ima_adpcm[i].predictor = 0x7FFF;
					}

					/* store loop if there */
					if (p_ima_adpcm[i].last_nibble == p_ima_adpcm[i].loop_pos) {
						p_ima_adpcm[i].loop_step_index = p_ima_adpcm[i].step_index;
						p_ima_adpcm[i].loop_predictor = p_ima_adpcm[i].predictor;
					}

					// printf("%i - %i - pred
					// %i\n",int(p_ima_adpcm[i].last_nibble),int(nibble),int(p_ima_adpcm[i].predictor));
				}
			}

			final = p_ima_adpcm[0].predictor;
			if (is_stereo) {
				final_r = p_ima_adpcm[1].predictor;
			}

		}
		else if (is_qoa) {
			uint64_t new_data_ofs = 8 + pos / QOA_FRAME_LEN * p_qoa->frame_len;

			if (p_qoa->data_ofs != new_data_ofs) {
				p_qoa->data_ofs = new_data_ofs;
				const uint8_t* ofs_src = (uint8_t*)p_src + p_qoa->data_ofs;
				qoa_decode_frame(
					ofs_src, p_qoa->frame_len, &p_qoa->desc, p_qoa->dec.ptr(), nullptr);
			}

			uint32_t dec_idx = pos % QOA_FRAME_LEN << (is_stereo ? 1 : 0);

			final = p_qoa->dec[dec_idx];
			if (is_stereo) {
				final_r = p_qoa->dec[dec_idx + 1];
			}

		}
		else {
			final = p_src[pos];
			if (is_stereo) {
				final_r = p_src[pos + 1];
			}
			if constexpr (sizeof(Depth) ==
						  1) { /* conditions will not exist anymore when compiled! */
				final <<= 8;
				if (is_stereo) {
					final_r <<= 8;
				}
			}
		}

		if (!is_stereo) {
			final_r = final; // copy to right channel if stereo
		}

		p_dst->left = final / 32767.0;
		p_dst->right = final_r / 32767.0;
		p_dst++;

		p_offset += p_increment;
	}
}

int AudioStreamPlaybackWAV::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (base->data.is_empty() || !active) {
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		return 0;
	}

	uint64_t len = base->data.size();
	switch (base->format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		len /= 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
		len /= 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		len *= 2;
		break;
	case AudioStreamWAV::FORMAT_QOA:
		len = qoa.desc.samples * qoa.desc.channels;
		break;
	}

	if (base->stereo) {
		len /= 2;
	}

	int64_t loop_begin = base->loop_begin;
	int64_t loop_end = base->loop_end;
	int64_t begin_limit = (base->loop_mode != AudioStreamWAV::LOOP_DISABLED) ? loop_begin : 0;
	int64_t end_limit = (base->loop_mode != AudioStreamWAV::LOOP_DISABLED) ? loop_end : len - 1;
	bool is_stereo = base->stereo;

	int32_t todo = p_frames;

	if (base->loop_mode == AudioStreamWAV::LOOP_BACKWARD) {
		sign = -1;
	}

	int8_t increment = sign;

	// looping

	AudioStreamWAV::LoopMode loop_format = base->loop_mode;
	AudioStreamWAV::Format format = base->format;

	/* audio data */

	const uint8_t* data = base->data.ptr();
	AudioFrame* dst_buff = p_buffer;

	if (format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		if (loop_format != AudioStreamWAV::LOOP_DISABLED) {
			ima_adpcm[0].loop_pos = loop_begin;
			ima_adpcm[1].loop_pos = loop_begin;
			loop_format = AudioStreamWAV::LOOP_FORWARD;
		}
	}

	while (todo > 0) {
		int64_t limit = 0;
		int32_t target = 0, aux = 0;

		/** LOOP CHECKING **/

		if (increment < 0) {
			/* going backwards */

			if (loop_format != AudioStreamWAV::LOOP_DISABLED && offset < loop_begin) {
				/* loopstart reached */
				if (loop_format == AudioStreamWAV::LOOP_PINGPONG) {
					/* bounce ping pong */
					offset = loop_begin + (loop_begin - offset);
					increment = -increment;
					sign *= -1;
				}
				else {
					/* go to loop-end */
					offset = loop_end - (loop_begin - offset);
				}
			}
			else {
				/* check for sample not reaching beginning */
				if (offset < 0) {
					active = false;
					break;
				}
			}
		}
		else {
			/* going forward */
			if (loop_format != AudioStreamWAV::LOOP_DISABLED && offset >= loop_end) {
				/* loopend reached */

				if (loop_format == AudioStreamWAV::LOOP_PINGPONG) {
					/* bounce ping pong */
					offset = loop_end - (offset - loop_end);
					increment = -increment;
					sign *= -1;
				}
				else {
					/* go to loop-begin */

					if (format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
						for (int i = 0; i < 2; i++) {
							ima_adpcm[i].step_index = ima_adpcm[i].loop_step_index;
							ima_adpcm[i].predictor = ima_adpcm[i].loop_predictor;
							ima_adpcm[i].last_nibble = loop_begin;
						}
						offset = loop_begin;
					}
					else {
						offset = loop_begin + (offset - loop_end);
					}
				}
			}
			else {
				/* no loop, check for end of sample */
				if ((uint64_t)offset >= len) {
					active = false;
					break;
				}
			}
		}

		/** MIXCOUNT COMPUTING **/

		/* next possible limit (looppoints or sample begin/end */
		limit = (increment < 0) ? begin_limit : end_limit;

		/* compute what is shorter, the todo or the limit? */
		aux = (limit - offset) / increment + 1;
		target = (aux < todo) ? aux : todo; /* mix target is the shorter buffer */

		/* check just in case */
		if (target <= 0) {
			active = false;
			break;
		}

		todo -= target;

		switch (base->format) {
		case AudioStreamWAV::FORMAT_8_BITS: {
			if (is_stereo) {
				decode_samples<int8_t, true, false, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int8_t, false, false, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
		} break;
		case AudioStreamWAV::FORMAT_16_BITS: {
			if (is_stereo) {
				decode_samples<int16_t, true, false, false>(
					(int16_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int16_t, false, false, false>(
					(int16_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}

		} break;
		case AudioStreamWAV::FORMAT_IMA_ADPCM: {
			if (is_stereo) {
				decode_samples<int8_t, true, true, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int8_t, false, true, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}

		} break;
		case AudioStreamWAV::FORMAT_QOA: {
			if (is_stereo) {
				decode_samples<uint8_t, true, false, true>(
					(uint8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<uint8_t, false, false, true>(
					(uint8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
		} break;
		}

		dst_buff += target;
	}

	if (todo) {
		int mixed_frames = p_frames - todo;
		// bit was missing from mix
		int todo_ofs = p_frames - todo;
		for (int i = todo_ofs; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		return mixed_frames;
	}
	return p_frames;
}

float AudioStreamPlaybackWAV::get_stream_sampling_rate() { return base->mix_rate; }

void AudioStreamPlaybackWAV::tag_used_streams() { base->tag_used(get_playback_position()); }

void AudioStreamPlaybackWAV::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackWAV::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackWAV::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackWAV::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

/////////////////////

void AudioStreamWAV::set_format(Format p_format) { format = p_format; }

AudioStreamWAV::Format AudioStreamWAV::get_format() const { return format; }

void AudioStreamWAV::set_loop_mode(LoopMode p_loop_mode) { loop_mode = p_loop_mode; }

AudioStreamWAV::LoopMode AudioStreamWAV::get_loop_mode() const { return loop_mode; }

void AudioStreamWAV::set_loop_begin(int p_frame) { loop_begin = p_frame; }

int AudioStreamWAV::get_loop_begin() const { return loop_begin; }

void AudioStreamWAV::set_loop_end(int p_frame) { loop_end = p_frame; }

int AudioStreamWAV::get_loop_end() const { return loop_end; }

void AudioStreamWAV::set_mix_rate(int p_hz)
{
	ERR_FAIL_COND(p_hz == 0);
	mix_rate = p_hz;
}

int AudioStreamWAV::get_mix_rate() const { return mix_rate; }

void AudioStreamWAV::set_stereo(bool p_enable) { stereo = p_enable; }

bool AudioStreamWAV::is_stereo() const { return stereo; }

double AudioStreamWAV::get_length() const
{
	uint64_t len = data.size();
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		len /= 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
		len /= 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		len *= 2;
		break;
	case AudioStreamWAV::FORMAT_QOA:
		qoa_desc desc = {};
		qoa_decode_header(data.ptr(), len, &desc);
		len = desc.samples * desc.channels;
		break;
	}

	if (stereo) {
		len /= 2;
	}

	return double(len) / mix_rate;
}

bool AudioStreamWAV::is_monophonic() const { return false; }

void AudioStreamWAV::set_data(const Vector<uint8_t>& p_data)
{
	AudioServer::lock();

	data = p_data;

	AudioServer::unlock();
}

Vector<uint8_t> AudioStreamWAV::get_data() const { return Vector<uint8_t>(data); }

Error AudioStreamWAV::save_to_wav(const String& p_path)
{
	if (format == AudioStreamWAV::FORMAT_IMA_ADPCM || format == AudioStreamWAV::FORMAT_QOA) {
		WARN_PRINT("Saving IMA_ADPCM and QOA samples is not supported yet");
		return ERR_UNAVAILABLE;
	}

	uint64_t sub_chunk_2_size = data.size(); // Subchunk2Size = Size of data in bytes
	ERR_FAIL_COND_V_MSG(sub_chunk_2_size > UINT32_MAX - 36, ERR_FILE_CANT_WRITE,
		"Data size exceeds maximum WAV file size of 4 GiB.");

	// Format code
	// 1:PCM format (for 8 or 16 bit)
	// 3:IEEE float format
	int format_code = (format == FORMAT_IMA_ADPCM) ? 3 : 1;

	int n_channels = stereo ? 2 : 1;

	long sample_rate = mix_rate;

	int byte_pr_sample = 0;
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		byte_pr_sample = 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
	case AudioStreamWAV::FORMAT_QOA:
		byte_pr_sample = 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		byte_pr_sample = 4;
		break;
	}

	String file_path = p_path;
	if (file_path.substr(file_path.length() - 4, 4).to_lower() != ".wav") {
		file_path += ".wav";
	}

	Ref<FileAccess> file =
		FileAccess::open(file_path, FileAccess::WRITE); // Overrides existing file if present

	ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_WRITE);

	// Create WAV Header
	file->store_string("RIFF");			   // ChunkID
	file->store_32(sub_chunk_2_size + 36); // ChunkSize = 36 + SubChunk2Size (size of entire file
										   // minus the 8 bytes for this and previous header)
	file->store_string("WAVE");			   // Format
	file->store_string("fmt ");			   // Subchunk1ID
	file->store_32(16);					   // Subchunk1Size = 16
	file->store_16(format_code);		   // AudioFormat
	file->store_16(n_channels);			   // Number of Channels
	file->store_32(sample_rate);		   // SampleRate
	file->store_32(sample_rate * n_channels * byte_pr_sample); // ByteRate
	file->store_16(n_channels * byte_pr_sample); // BlockAlign = NumChannels * BytePrSample
	file->store_16(byte_pr_sample * 8);			 // BitsPerSample
	file->store_string("data");					 // Subchunk2ID
	file->store_32(sub_chunk_2_size);			 // Subchunk2Size

	// Add data
	const uint8_t* read_data = data.ptr();
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		for (uint64_t i = 0; i < sub_chunk_2_size; i++) {
			uint8_t data_point = (read_data[i] + 128);
			file->store_8(data_point);
		}
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
	case AudioStreamWAV::FORMAT_QOA:
		for (uint64_t i = 0; i < sub_chunk_2_size / 2; i++) {
			uint16_t data_point = decode_uint16(&read_data[i * 2]);
			file->store_16(data_point);
		}
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		// Unimplemented
		break;
	}

	return OK;
}

Ref<AudioStreamPlayback> AudioStreamWAV::instantiate_playback()
{
	Ref<AudioStreamPlaybackWAV> sample;
	sample.instantiate();
	sample->base = Ref<AudioStreamWAV>(this);

	if (format == AudioStreamWAV::FORMAT_QOA) {
		uint32_t ffp = qoa_decode_header(data.ptr(), data.size(), &sample->qoa.desc);
		ERR_FAIL_COND_V(ffp != 8, Ref<AudioStreamPlaybackWAV>());
		sample->qoa.frame_len = qoa_max_frame_size(&sample->qoa.desc);
		uint32_t samples_len = MIN(sample->qoa.desc.samples + 1, (uint32_t)QOA_FRAME_LEN);
		uint32_t dec_len = sample->qoa.desc.channels * samples_len;
		sample->qoa.dec.resize(dec_len);
	}

	return sample;
}

Ref<AudioSample> AudioStreamWAV::generate_sample() const
{
	Ref<AudioSample> as = memnew(AudioSample);
	return as;
}

// --- video_stream.cpp ---

#include "scene/resources/packed_scene.h"

#include "modules/modules_enabled.gen.h"

void VideoStreamPlayback::set_mix_callback(AudioMixCallback p_callback, void* p_userdata)
{
	mix_callback = p_callback;
	mix_udata = p_userdata;
}

int VideoStreamPlayback::mix_audio(int num_frames, PackedFloat32Array buffer, int offset)
{
	if (num_frames <= 0) {
		return 0;
	}
	if (!mix_callback) {
		return -1;
	}
	ERR_FAIL_INDEX_V(offset, buffer.size(), -1);
	ERR_FAIL_INDEX_V(
		(_channel_count < 1 ? 1 : _channel_count) * num_frames - 1, buffer.size() - offset, -1);
	return mix_callback(mix_udata, buffer.ptr() + offset, num_frames);
}

void VideoStream::set_file(const String& p_file)
{
	file = p_file;
	emit_changed();
}

String VideoStream::get_file() { return file; }

VideoStream::VideoStream() {}

VideoStream::~VideoStream() {}

void VideoStream::set_audio_track(int p_track) { audio_track = p_track; }

#ifndef MODULE_THEORA_ENABLED

VideoStreamPlayback::VideoStreamPlayback() {}

VideoStreamPlayback::~VideoStreamPlayback() {}

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	ERR_FAIL_V_MSG(Ref<VideoStreamPlayback>(), "Theora module is disabled.");
}

void VideoStreamPlayback::stop() {}

void VideoStreamPlayback::play() {}

bool VideoStreamPlayback::is_playing() const { return false; }

void VideoStreamPlayback::set_paused(bool p_paused) {}

bool VideoStreamPlayback::is_paused() const { return false; }

double VideoStreamPlayback::get_length() const { return 0.0; }

double VideoStreamPlayback::get_playback_position() const { return 0.0; }

void VideoStreamPlayback::seek(double p_time) {}

void VideoStreamPlayback::set_file(const String& p_file) {}

void VideoStreamPlayback::set_audio_track(int p_idx) {}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return Ref<Texture2D>(); }

void VideoStreamPlayback::update(double p_delta) {}

int VideoStreamPlayback::get_channels() const { return 0; }

int VideoStreamPlayback::get_mix_rate() const { return 0; }

#endif // MODULE_THEORA_ENABLED

// ============================================================================
// Merged from scene/resources/2d/tile_set.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/io/marshalls.h"
#include "core/math/geometry_2d.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_set.h"
#include "scene/gui/control.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "scene/2d/line_2d.h"

Vector<int> TileMapPattern::_get_tile_data() const
{
	// Export tile data to raw format
	Vector<int> data;
	data.resize(pattern.size() * 3);
	int* w = data.ptrw();

	// Save in highest format

	int idx = 0;
	for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
		uint8_t* ptr = (uint8_t*)&w[idx];
		encode_uint16((int16_t)(E.key.x), &ptr[0]);
		encode_uint16((int16_t)(E.key.y), &ptr[2]);
		encode_uint16(E.value.source_id, &ptr[4]);
		encode_uint16(E.value.coord_x, &ptr[6]);
		encode_uint16(E.value.coord_y, &ptr[8]);
		encode_uint16(E.value.alternative_tile, &ptr[10]);
		idx += 3;
	}

	return data;
}

void TileMapPattern::set_cell(const Vector2i& p_coords, int p_source_id,
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_MSG(p_coords.x < 0 || p_coords.y < 0,
		vformat("Cannot set cell with negative coords in a TileMapPattern. Wrong coords: %s",
			p_coords));

	size = size.max(p_coords + Vector2i(1, 1));
	pattern[p_coords] = TileMapCell(p_source_id, p_atlas_coords, p_alternative_tile);
	emit_changed();
}

bool TileMapPattern::has_cell(const Vector2i& p_coords) const { return pattern.has(p_coords); }

void TileMapPattern::remove_cell(const Vector2i& p_coords, bool p_update_size)
{
	ERR_FAIL_COND(!pattern.has(p_coords));

	pattern.erase(p_coords);
	if (p_update_size) {
		size = Size2i();
		for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
			size = size.max(E.key + Vector2i(1, 1));
		}
	}
	emit_changed();
}

int TileMapPattern::get_cell_source_id(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSet::INVALID_SOURCE);

	return pattern[p_coords].source_id;
}

Vector2i TileMapPattern::get_cell_atlas_coords(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSetSource::INVALID_ATLAS_COORDS);

	return pattern[p_coords].get_atlas_coords();
}

int TileMapPattern::get_cell_alternative_tile(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSetSource::INVALID_TILE_ALTERNATIVE);

	return pattern[p_coords].alternative_tile;
}

Size2i TileMapPattern::get_size() const { return size; }

void TileMapPattern::set_size(const Size2i& p_size)
{
	for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
		Vector2i coords = E.key;
		if (p_size.x <= coords.x || p_size.y <= coords.y) {
			ERR_FAIL_MSG(vformat("Cannot set pattern size to %s, it contains a tile at %s. Size "
								 "can only be increased.",
				p_size, coords));
		};
	}

	size = p_size;
	emit_changed();
}

bool TileMapPattern::is_empty() const { return pattern.is_empty(); }

void TileMapPattern::clear()
{
	size = Size2i();
	pattern.clear();
	emit_changed();
}

bool TileSet::TerrainsPattern::is_valid() const { return valid; }

bool TileSet::TerrainsPattern::is_erase_pattern() const { return not_empty_terrains_count == 0; }

bool TileSet::TerrainsPattern::operator<(const TerrainsPattern& p_terrains_pattern) const
{
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] != p_terrains_pattern.is_valid_bit[i]) {
			return is_valid_bit[i] < p_terrains_pattern.is_valid_bit[i];
		}
	}
	if (terrain != p_terrains_pattern.terrain) {
		return terrain < p_terrains_pattern.terrain;
	}
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] && bits[i] != p_terrains_pattern.bits[i]) {
			return bits[i] < p_terrains_pattern.bits[i];
		}
	}
	return false;
}

bool TileSet::TerrainsPattern::operator==(const TerrainsPattern& p_terrains_pattern) const
{
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] != p_terrains_pattern.is_valid_bit[i]) {
			return false;
		}
		if (is_valid_bit[i] && bits[i] != p_terrains_pattern.bits[i]) {
			return false;
		}
	}
	if (terrain != p_terrains_pattern.terrain) {
		return false;
	}
	return true;
}

void TileSet::TerrainsPattern::set_terrain(int p_terrain)
{
	ERR_FAIL_COND(p_terrain < -1);

	terrain = p_terrain;
}

int TileSet::TerrainsPattern::get_terrain() const { return terrain; }

void TileSet::TerrainsPattern::set_terrain_peering_bit(
	TileSet::CellNeighbor p_peering_bit, int p_terrain)
{
	ERR_FAIL_COND(p_peering_bit == TileSet::CELL_NEIGHBOR_MAX);
	ERR_FAIL_COND(!is_valid_bit[p_peering_bit]);
	ERR_FAIL_COND(p_terrain < -1);

	// Update the "is_erase_pattern" status.
	if (p_terrain >= 0 && bits[p_peering_bit] < 0) {
		not_empty_terrains_count++;
	}
	else if (p_terrain < 0 && bits[p_peering_bit] >= 0) {
		not_empty_terrains_count--;
	}

	bits[p_peering_bit] = p_terrain;
}

int TileSet::TerrainsPattern::get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_COND_V(p_peering_bit == TileSet::CELL_NEIGHBOR_MAX, -1);
	ERR_FAIL_COND_V(!is_valid_bit[p_peering_bit], -1);
	return bits[p_peering_bit];
}

TileSet::TerrainsPattern::TerrainsPattern(const TileSet* p_tile_set, int p_terrain_set)
{
	ERR_FAIL_COND(p_terrain_set < 0);
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		is_valid_bit[i] =
			(p_tile_set->is_valid_terrain_peering_bit(p_terrain_set, TileSet::CellNeighbor(i)));
		bits[i] = -1;
	}
	valid = true;
}

const int TileSet::INVALID_SOURCE = -1;

const char* TileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[] = {
	PNAME("right_side"),
	PNAME("right_corner"),
	PNAME("bottom_right_side"),
	PNAME("bottom_right_corner"),
	PNAME("bottom_side"),
	PNAME("bottom_corner"),
	PNAME("bottom_left_side"),
	PNAME("bottom_left_corner"),
	PNAME("left_side"),
	PNAME("left_corner"),
	PNAME("top_left_side"),
	PNAME("top_left_corner"),
	PNAME("top_side"),
	PNAME("top_corner"),
	PNAME("top_right_side"),
	PNAME("top_right_corner"),
};

TileSet::TileShape TileSet::get_tile_shape() const { return tile_shape; }

void TileSet::set_tile_layout(TileSet::TileLayout p_layout)
{
	tile_layout = p_layout;
	emit_changed();
}

TileSet::TileLayout TileSet::get_tile_layout() const { return tile_layout; }

void TileSet::set_tile_offset_axis(TileSet::TileOffsetAxis p_alignment)
{
	tile_offset_axis = p_alignment;

	for (KeyValue<int, Ref<TileSetSource>>& E_source : sources) {
		E_source.value->notify_tile_data_properties_should_change();
	}

	terrain_bits_meshes_dirty = true;
	tile_meshes_dirty = true;
	emit_changed();
}

TileSet::TileOffsetAxis TileSet::get_tile_offset_axis() const { return tile_offset_axis; }

void TileSet::set_tile_size(Size2i p_size)
{
	ERR_FAIL_COND(p_size.x < 1 || p_size.y < 1);
	tile_size = p_size;
	terrain_bits_meshes_dirty = true;
	tile_meshes_dirty = true;
	emit_changed();
}

Size2i TileSet::get_tile_size() const { return tile_size; }

int TileSet::get_next_source_id() const { return next_source_id; }

void TileSet::_update_terrains_cache()
{
	if (terrains_cache_dirty) {
		// Organizes tiles into structures.
		per_terrain_pattern_tiles.resize(terrain_sets.size());
		for (RBMap<TileSet::TerrainsPattern, RBSet<TileMapCell>>& tiles :
			per_terrain_pattern_tiles) {
			tiles.clear();
		}

		for (const KeyValue<int, Ref<TileSetSource>>& kv : sources) {
			Ref<TileSetSource> source = kv.value;
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
					Vector2i tile_id = source->get_tile_id(tile_index);
					for (int alternative_index = 0;
						 alternative_index < source->get_alternative_tiles_count(tile_id);
						 alternative_index++) {
						int alternative_id =
							source->get_alternative_tile_id(tile_id, alternative_index);

						// Executed for each tile_data.
						TileData* tile_data = atlas_source->get_tile_data(tile_id, alternative_id);
						int terrain_set = tile_data->get_terrain_set();
						if (terrain_set >= 0) {
							TileMapCell cell;
							cell.source_id = kv.key;
							cell.set_atlas_coords(tile_id);
							cell.alternative_tile = alternative_id;

							TileSet::TerrainsPattern terrains_pattern =
								tile_data->get_terrains_pattern();

							// Main terrain.
							if (terrains_pattern.get_terrain() >= 0) {
								per_terrain_pattern_tiles[terrain_set][terrains_pattern].insert(
									cell);
							}

							// Terrain bits.
							for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
								CellNeighbor bit = CellNeighbor(i);
								if (is_valid_terrain_peering_bit(terrain_set, bit)) {
									int terrain = terrains_pattern.get_terrain_peering_bit(bit);
									if (terrain >= 0) {
										per_terrain_pattern_tiles[terrain_set][terrains_pattern]
											.insert(cell);
									}
								}
							}
						}
					}
				}
			}
		}

		// Add the empty cell in the possible patterns and cells.
		for (int i = 0; i < terrain_sets.size(); i++) {
			TileSet::TerrainsPattern empty_pattern(this, i);

			TileMapCell empty_cell;
			empty_cell.source_id = TileSet::INVALID_SOURCE;
			empty_cell.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
			empty_cell.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
			per_terrain_pattern_tiles[i][empty_pattern].insert(empty_cell);
		}
		terrains_cache_dirty = false;
	}
}

void TileSet::_compute_next_source_id()
{
	while (sources.has(next_source_id)) {
		next_source_id = (next_source_id + 1) % 1073741824; // 2 ** 30
	};
}

void TileSet::remove_source_ptr(TileSetSource* p_tile_set_source)
{
	for (const KeyValue<int, Ref<TileSetSource>>& kv : sources) {
		if (kv.value.ptr() == p_tile_set_source) {
			remove_source(kv.key);
			return;
		}
	}
	ERR_FAIL_MSG(
		vformat("Attempting to remove source from a tileset, but the tileset doesn't have it: %s",
			p_tile_set_source));
}

void TileSet::set_source_id(int p_source_id, int p_new_source_id)
{
	ERR_FAIL_COND(p_new_source_id < 0);
	ERR_FAIL_COND_MSG(!sources.has(p_source_id),
		vformat("Cannot change TileSet atlas source ID. No tileset atlas source with id %d.",
			p_source_id));
	if (p_source_id == p_new_source_id) {
		return;
	}

	ERR_FAIL_COND_MSG(sources.has(p_new_source_id),
		vformat("Cannot change TileSet atlas source ID. Another atlas source exists with id %d.",
			p_new_source_id));

	sources[p_new_source_id] = sources[p_source_id];
	sources.erase(p_source_id);

	source_ids.erase(p_source_id);
	source_ids.push_back(p_new_source_id);
	source_ids.sort();

	_compute_next_source_id();

	terrains_cache_dirty = true;
	emit_changed();
}

bool TileSet::has_source(int p_source_id) const { return sources.has(p_source_id); }

Ref<TileSetSource> TileSet::get_source(int p_source_id) const
{
	ERR_FAIL_COND_V_MSG(!sources.has(p_source_id), nullptr,
		vformat("No TileSet atlas source with id %d.", p_source_id));

	return sources[p_source_id];
}

int TileSet::get_source_count() const { return source_ids.size(); }

int TileSet::get_source_id(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, source_ids.size(), TileSet::INVALID_SOURCE);
	return source_ids[p_index];
}

void TileSet::set_uv_clipping(bool p_uv_clipping)
{
	if (uv_clipping == p_uv_clipping) {
		return;
	}
	uv_clipping = p_uv_clipping;
	emit_changed();
}

bool TileSet::is_uv_clipping() const { return uv_clipping; }

int TileSet::get_occlusion_layers_count() const { return occlusion_layers.size(); }

void TileSet::set_occlusion_layer_light_mask(int p_layer_index, int p_light_mask)
{
	ERR_FAIL_INDEX(p_layer_index, occlusion_layers.size());
	occlusion_layers.write[p_layer_index].light_mask = p_light_mask;
	emit_changed();
}

int TileSet::get_occlusion_layer_light_mask(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, occlusion_layers.size(), 0);
	return occlusion_layers[p_layer_index].light_mask;
}

void TileSet::set_occlusion_layer_sdf_collision(int p_layer_index, bool p_sdf_collision)
{
	ERR_FAIL_INDEX(p_layer_index, occlusion_layers.size());
	occlusion_layers.write[p_layer_index].sdf_collision = p_sdf_collision;
	emit_changed();
}

bool TileSet::get_occlusion_layer_sdf_collision(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, occlusion_layers.size(), false);
	return occlusion_layers[p_layer_index].sdf_collision;
}

#ifndef PHYSICS_2D_DISABLED
int TileSet::get_physics_layers_count() const { return physics_layers.size(); }

void TileSet::set_physics_layer_collision_layer(int p_layer_index, uint32_t p_layer)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_layer = p_layer;
	emit_changed();
}

uint32_t TileSet::get_physics_layer_collision_layer(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_layer;
}

void TileSet::set_physics_layer_collision_mask(int p_layer_index, uint32_t p_mask)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_mask = p_mask;
	emit_changed();
}

uint32_t TileSet::get_physics_layer_collision_mask(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_mask;
}

void TileSet::set_physics_layer_collision_priority(int p_layer_index, real_t p_priority)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_priority = p_priority;
	emit_changed();
}

real_t TileSet::get_physics_layer_collision_priority(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_priority;
}

void TileSet::set_physics_layer_physics_material(
	int p_layer_index, Ref<PhysicsMaterial> p_physics_material)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].physics_material = p_physics_material;
}

Ref<PhysicsMaterial> TileSet::get_physics_layer_physics_material(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), Ref<PhysicsMaterial>());
	return physics_layers[p_layer_index].physics_material;
}
#endif // PHYSICS_2D_DISABLED

int TileSet::get_terrain_sets_count() const { return terrain_sets.size(); }

TileSet::TerrainMode TileSet::get_terrain_set_mode(int p_terrain_set) const
{
	ERR_FAIL_INDEX_V(
		p_terrain_set, terrain_sets.size(), TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES);
	return terrain_sets[p_terrain_set].mode;
}

int TileSet::get_terrains_count(int p_terrain_set) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), -1);
	return terrain_sets[p_terrain_set].terrains.size();
}

void TileSet::set_terrain_name(int p_terrain_set, int p_terrain_index, String p_name)
{
	ERR_FAIL_INDEX(p_terrain_set, terrain_sets.size());
	ERR_FAIL_INDEX(p_terrain_index, terrain_sets[p_terrain_set].terrains.size());
	terrain_sets.write[p_terrain_set].terrains.write[p_terrain_index].name = p_name;
	emit_changed();
}

String TileSet::get_terrain_name(int p_terrain_set, int p_terrain_index) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), String());
	ERR_FAIL_INDEX_V(p_terrain_index, terrain_sets[p_terrain_set].terrains.size(), String());
	return terrain_sets[p_terrain_set].terrains[p_terrain_index].name;
}

void TileSet::set_terrain_color(int p_terrain_set, int p_terrain_index, Color p_color)
{
	ERR_FAIL_INDEX(p_terrain_set, terrain_sets.size());
	ERR_FAIL_INDEX(p_terrain_index, terrain_sets[p_terrain_set].terrains.size());
	if (p_color.a != 1.0) {
		WARN_PRINT("Terrain color should have alpha == 1.0");
		p_color.a = 1.0;
	}
	terrain_sets.write[p_terrain_set].terrains.write[p_terrain_index].color = p_color;
	emit_changed();
}

Color TileSet::get_terrain_color(int p_terrain_set, int p_terrain_index) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), Color());
	ERR_FAIL_INDEX_V(p_terrain_index, terrain_sets[p_terrain_set].terrains.size(), Color());
	return terrain_sets[p_terrain_set].terrains[p_terrain_index].color;
}

bool TileSet::is_valid_terrain_peering_bit_for_mode(
	TileSet::TerrainMode p_terrain_mode, TileSet::CellNeighbor p_peering_bit) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_SIDE) {
				return true;
			}
		}
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
				return true;
			}
		}
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
				return true;
			}
		}
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
				return true;
			}
		}
	}
	else {
		if (get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return true;
				}
			}
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
					return true;
				}
			}
		}
		else {
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return true;
				}
			}
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TileSet::is_valid_terrain_peering_bit(
	int p_terrain_set, TileSet::CellNeighbor p_peering_bit) const
{
	if (p_terrain_set < 0 || p_terrain_set >= get_terrain_sets_count()) {
		return false;
	}

	TileSet::TerrainMode terrain_mode = get_terrain_set_mode(p_terrain_set);
	return is_valid_terrain_peering_bit_for_mode(terrain_mode, p_peering_bit);
}

#ifndef NAVIGATION_2D_DISABLED
int TileSet::get_navigation_layers_count() const { return navigation_layers.size(); }

void TileSet::set_navigation_layer_layers(int p_layer_index, uint32_t p_layers)
{
	ERR_FAIL_INDEX(p_layer_index, navigation_layers.size());
	navigation_layers.write[p_layer_index].layers = p_layers;
	emit_changed();
}

uint32_t TileSet::get_navigation_layer_layers(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, navigation_layers.size(), 0);
	return navigation_layers[p_layer_index].layers;
}

void TileSet::set_navigation_layer_layer_value(int p_layer_index, int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layer_layers(p_layer_index);

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layer_layers(p_layer_index, _navigation_layers);
}

bool TileSet::get_navigation_layer_layer_value(int p_layer_index, int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layer_layers(p_layer_index) & (1 << (p_layer_number - 1));
}
#endif // NAVIGATION_2D_DISABLED

int TileSet::get_custom_data_layer_by_name(String p_value) const
{
	if (custom_data_layers_by_name.has(p_value)) {
		return custom_data_layers_by_name[p_value];
	}
	else {
		return -1;
	}
}

bool TileSet::has_custom_data_layer_by_name(const String& p_value) const
{
	return custom_data_layers_by_name.has(p_value);
}

void TileSet::set_source_level_tile_proxy(int p_source_from, int p_source_to)
{
	ERR_FAIL_COND(
		p_source_from == TileSet::INVALID_SOURCE || p_source_to == TileSet::INVALID_SOURCE);

	source_level_proxies[p_source_from] = p_source_to;

	emit_changed();
}

int TileSet::get_source_level_tile_proxy(int p_source_from)
{
	ERR_FAIL_COND_V(!source_level_proxies.has(p_source_from), TileSet::INVALID_SOURCE);

	return source_level_proxies[p_source_from];
}

bool TileSet::has_source_level_tile_proxy(int p_source_from)
{
	return source_level_proxies.has(p_source_from);
}

void TileSet::remove_source_level_tile_proxy(int p_source_from)
{
	ERR_FAIL_COND(!source_level_proxies.has(p_source_from));

	source_level_proxies.erase(p_source_from);

	emit_changed();
}

int TileSet::add_pattern(Ref<TileMapPattern> p_pattern, int p_index)
{
	ERR_FAIL_COND_V(p_pattern.is_null(), -1);
	ERR_FAIL_COND_V_MSG(p_pattern->is_empty(), -1, "Cannot add an empty pattern to the TileSet.");
	for (const Ref<TileMapPattern>& pattern : patterns) {
		ERR_FAIL_COND_V_MSG(pattern == p_pattern, -1, "TileSet has already this pattern.");
	}
	ERR_FAIL_COND_V(p_index > (int)patterns.size(), -1);
	if (p_index < 0) {
		p_index = patterns.size();
	}
	patterns.insert(p_index, p_pattern);
	emit_changed();
	return p_index;
}

Ref<TileMapPattern> TileSet::get_pattern(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, (int)patterns.size(), Ref<TileMapPattern>());
	return patterns[p_index];
}

void TileSet::remove_pattern(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)patterns.size());
	patterns.remove_at(p_index);
	emit_changed();
}

int TileSet::get_patterns_count() { return patterns.size(); }

RBSet<TileSet::TerrainsPattern> TileSet::get_terrains_pattern_set(int p_terrain_set)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), RBSet<TileSet::TerrainsPattern>());
	_update_terrains_cache();

	RBSet<TileSet::TerrainsPattern> output;
	for (KeyValue<TileSet::TerrainsPattern, RBSet<TileMapCell>> kv :
		per_terrain_pattern_tiles[p_terrain_set]) {
		output.insert(kv.key);
	}
	return output;
}

RBSet<TileMapCell> TileSet::get_tiles_for_terrains_pattern(
	int p_terrain_set, TerrainsPattern p_terrain_tile_pattern)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), RBSet<TileMapCell>());
	_update_terrains_cache();
	return RBSet<TileMapCell>(per_terrain_pattern_tiles[p_terrain_set][p_terrain_tile_pattern]);
}

TileMapCell TileSet::get_random_tile_from_terrains_pattern(
	int p_terrain_set, TileSet::TerrainsPattern p_terrain_tile_pattern)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), TileMapCell());
	_update_terrains_cache();

	// Count the sum of probabilities.
	double sum = 0.0;
	RBSet<TileMapCell> set(per_terrain_pattern_tiles[p_terrain_set][p_terrain_tile_pattern]);
	for (const TileMapCell& E : set) {
		if (E.source_id >= 0) {
			Ref<TileSetSource> source = sources[E.source_id];
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData* tile_data =
					atlas_source->get_tile_data(E.get_atlas_coords(), E.alternative_tile);
				sum += tile_data->get_probability();
			}
			else {
				sum += 1.0;
			}
		}
		else {
			sum += 1.0;
		}
	}

	// Generate a random number.
	double count = 0.0;
	double picked = Math::random(0.0, sum);

	// Pick the tile.
	for (const TileMapCell& E : set) {
		if (E.source_id >= 0) {
			Ref<TileSetSource> source = sources[E.source_id];

			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData* tile_data =
					atlas_source->get_tile_data(E.get_atlas_coords(), E.alternative_tile);
				count += tile_data->get_probability();
			}
			else {
				count += 1.0;
			}
		}
		else {
			count += 1.0;
		}

		if (count >= picked) {
			return E;
		}
	}

	ERR_FAIL_V(TileMapCell());
}

Vector<Vector2> TileSet::get_tile_shape_polygon() const
{
	Vector<Vector2> points;
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		points.push_back(Vector2(-0.5, -0.5));
		points.push_back(Vector2(0.5, -0.5));
		points.push_back(Vector2(0.5, 0.5));
		points.push_back(Vector2(-0.5, 0.5));
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		points.push_back(Vector2(0.0, -0.5));
		points.push_back(Vector2(-0.5, 0.0));
		points.push_back(Vector2(0.0, 0.5));
		points.push_back(Vector2(0.5, 0.0));
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}

		points.push_back(Vector2(0.0, -0.5));
		points.push_back(Vector2(-0.5, overlap - 0.5));
		points.push_back(Vector2(-0.5, 0.5 - overlap));
		points.push_back(Vector2(0.0, 0.5));
		points.push_back(Vector2(0.5, 0.5 - overlap));
		points.push_back(Vector2(0.5, overlap - 0.5));

		if (get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
			for (int i = 0; i < points.size(); i++) {
				points.write[i] = Vector2(points[i].y, points[i].x);
			}
		}
	}
	return points;
}

Vector2 TileSet::map_to_local(const Vector2i& p_pos) const
{
	// SHOULD RETURN THE CENTER OF THE CELL.
	Vector2 ret = p_pos;

	if (tile_shape == TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE ||
		tile_shape == TileSet::TILE_SHAPE_HEXAGON || tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		// Technically, those 3 shapes are equivalent, as they are basically half-offset, but with
		// different levels or overlap. square = no overlap, hexagon = 0.25 overlap, isometric = 0.5
		// overlap.
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = Vector2(ret.x + (Math::posmod(ret.y, 2) == 0 ? 0.0 : 0.5), ret.y);
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = Vector2(ret.x + (Math::posmod(ret.y, 2) == 1 ? 0.0 : 0.5), ret.y);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x + ret.y / 2, ret.y);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x / 2, ret.y * 2 + ret.x);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2((ret.x + ret.y) / 2, ret.y - ret.x);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2((ret.x - ret.y) / 2, ret.y + ret.x);
				break;
			}
		}
		else { // TILE_OFFSET_AXIS_VERTICAL.
			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = Vector2(ret.x, ret.y + (Math::posmod(ret.x, 2) == 0 ? 0.0 : 0.5));
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = Vector2(ret.x, ret.y + (Math::posmod(ret.x, 2) == 1 ? 0.0 : 0.5));
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x * 2 + ret.y, ret.y / 2);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x, ret.y + ret.x / 2);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x + ret.y, (ret.y - ret.x) / 2);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x - ret.y, (ret.y + ret.x) / 2);
				break;
			}
		}
	}

	// Multiply by the overlapping ratio.
	double overlapping_ratio = 1.0;
	if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.y *= overlapping_ratio;
	}
	else { // TILE_OFFSET_AXIS_VERTICAL.
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.x *= overlapping_ratio;
	}

	return (ret + Vector2(0.5, 0.5)) * tile_size;
}

Vector2i TileSet::local_to_map(const Vector2& p_local_position) const
{
	Vector2 ret = p_local_position;
	ret /= tile_size;

	// Divide by the overlapping ratio.
	double overlapping_ratio = 1.0;
	if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.y /= overlapping_ratio;
	}
	else { // TILE_OFFSET_AXIS_VERTICAL.
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.x /= overlapping_ratio;
	}

	// For each half-offset shape, we check if we are in the corner of the tile, and thus should
	// correct the local position accordingly.
	if (tile_shape == TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE ||
		tile_shape == TileSet::TILE_SHAPE_HEXAGON || tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		// Technically, those 3 shapes are equivalent, as they are basically half-offset, but with
		// different levels or overlap. square = no overlap, hexagon = 0.25 overlap, isometric = 0.5
		// overlap.
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			// Smart floor of the position
			Vector2 raw_pos = ret;
			if (Math::posmod(Math::floor(ret.y), 2) ^
				(tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET)) {
				ret = Vector2(Math::floor(ret.x + 0.5) - 0.5, Math::floor(ret.y));
			}
			else {
				ret = ret.floor();
			}

			// Compute the tile offset, and if we might the output for a neighbor top tile.
			Vector2 in_tile_pos = raw_pos - ret;
			bool in_top_left_triangle = (in_tile_pos - Vector2(0.5, 0.0))
											.cross(Vector2(-0.5, 1.0 / overlapping_ratio - 1)) <= 0;
			bool in_top_right_triangle =
				(in_tile_pos - Vector2(0.5, 0.0)).cross(Vector2(0.5, 1.0 / overlapping_ratio - 1)) >
				0;

			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 0 : -1, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 1 : 0, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? -1 : 0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 0 : 1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x - ret.y / 2, ret.y).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x * 2, ret.y / 2 - ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x - ret.y / 2, ret.y / 2 + ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, 0);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x + ret.y / 2, ret.y / 2 - ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(0, -1);
				}
				break;
			}
		}
		else { // TILE_OFFSET_AXIS_VERTICAL.
			// Smart floor of the position.
			Vector2 raw_pos = ret;
			if (Math::posmod(Math::floor(ret.x), 2) ^
				(tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET)) {
				ret = Vector2(Math::floor(ret.x), Math::floor(ret.y + 0.5) - 0.5);
			}
			else {
				ret = ret.floor();
			}

			// Compute the tile offset, and if we might the output for a neighbor top tile.
			Vector2 in_tile_pos = raw_pos - ret;
			bool in_top_left_triangle = (in_tile_pos - Vector2(0.0, 0.5))
											.cross(Vector2(1.0 / overlapping_ratio - 1, -0.5)) > 0;
			bool in_bottom_left_triangle =
				(in_tile_pos - Vector2(0.0, 0.5))
					.cross(Vector2(1.0 / overlapping_ratio - 1, 0.5)) <= 0;

			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 0 : -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 1 : 0);
				}
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? -1 : 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 0 : 1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x / 2 - ret.y, ret.y * 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x, ret.y - ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 1);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x / 2 - ret.y, ret.y + ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x / 2 + ret.y, ret.y - ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(0, 1);
				}
				break;
			}
		}
	}
	else {
		ret = (ret + Vector2(0.00005, 0.00005)).floor();
	}
	return Vector2i(ret);
}

bool TileSet::is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;

	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
	}
	else {
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
		}
		else {
			return p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
		}
	}
}

Vector2i TileSet::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (p_cell_neighbor) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			return p_coords + Vector2i(1, 0);
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			return p_coords + Vector2i(1, 1);
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			return p_coords + Vector2i(0, 1);
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			return p_coords + Vector2i(-1, 1);
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			return p_coords + Vector2i(-1, 0);
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			return p_coords + Vector2i(-1, -1);
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			return p_coords + Vector2i(0, -1);
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			return p_coords + Vector2i(1, -1);
		default:
			ERR_FAIL_V(p_coords);
		}
	}
	else { // Half-offset shapes (square and hexagon).
		switch (tile_layout) {
		case TileSet::TILE_LAYOUT_STACKED: {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				bool is_offset = p_coords.y % 2;
				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
					return p_coords + Vector2i(1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 1 : 0, 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
					return p_coords + Vector2i(0, 2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : -1, 1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
					return p_coords + Vector2i(-1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : -1, -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
					return p_coords + Vector2i(0, -2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 1 : 0, -1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
			else {
				bool is_offset = p_coords.x % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
					return p_coords + Vector2i(0, 1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 1 : 0);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
					return p_coords + Vector2i(2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 0 : -1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
					return p_coords + Vector2i(0, -1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 0 : -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
					return p_coords + Vector2i(-2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 1 : 0);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_STACKED_OFFSET: {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				bool is_offset = p_coords.y % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
					return p_coords + Vector2i(1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : 1, 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
					return p_coords + Vector2i(0, 2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? -1 : 0, 1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
					return p_coords + Vector2i(-1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? -1 : 0, -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
					return p_coords + Vector2i(0, -2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : 1, -1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
			else {
				bool is_offset = p_coords.x % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
					return p_coords + Vector2i(0, 1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 0 : 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
					return p_coords + Vector2i(2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? -1 : 0);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
					return p_coords + Vector2i(0, -1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? -1 : 0);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
					return p_coords + Vector2i(-2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 0 : 1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
		case TileSet::TILE_LAYOUT_STAIRS_DOWN: {
			if ((tile_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
				(tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(-1, 2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(1, -2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(0, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(2, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(0, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-2, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
			else {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(2, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(0, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-2, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(0, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(-1, 2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(1, -2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, 0);

					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
		case TileSet::TILE_LAYOUT_DIAMOND_DOWN: {
			if ((tile_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
				(tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
			else {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, -1);

					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
		} break;
		default:
			ERR_FAIL_V(p_coords);
		}
	}
}

Vector2i TileSet::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern) const
{
	ERR_FAIL_COND_V(p_pattern.is_null(), Vector2i());
	ERR_FAIL_COND_V(!p_pattern->has_cell(p_coords_in_pattern), Vector2i());

	Vector2i output = p_position_in_tilemap + p_coords_in_pattern;
	if (tile_shape != TileSet::TILE_SHAPE_SQUARE) {
		if (tile_layout == TileSet::TILE_LAYOUT_STACKED) {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL &&
				bool(p_position_in_tilemap.y % 2) && bool(p_coords_in_pattern.y % 2)) {
				output.x += 1;
			}
			else if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL &&
					   bool(p_position_in_tilemap.x % 2) && bool(p_coords_in_pattern.x % 2)) {
				output.y += 1;
			}
		}
		else if (tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET) {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL &&
				bool(p_position_in_tilemap.y % 2) && bool(p_coords_in_pattern.y % 2)) {
				output.x -= 1;
			}
			else if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL &&
					   bool(p_position_in_tilemap.x % 2) && bool(p_coords_in_pattern.x % 2)) {
				output.y -= 1;
			}
		}
	}

	return output;
}

void TileSet::draw_cells_outline(CanvasItem* p_canvas_item, const RBSet<Vector2i>& p_cells,
	Color p_color, Transform2D p_transform) const
{
	Vector<Vector2> polygon = get_tile_shape_polygon();
	for (const Vector2i& E : p_cells) {
		Vector2 center = map_to_local(E);

#define DRAW_SIDE_IF_NEEDED(side, polygon_index_from, polygon_index_to)                            \
	if (!p_cells.has(get_neighbor_cell(E, side))) {                                                \
		Vector2 from = p_transform.xform(center + polygon[polygon_index_from] * tile_size);        \
		Vector2 to = p_transform.xform(center + polygon[polygon_index_to] * tile_size);            \
		p_canvas_item->draw_line(from, to, p_color);                                               \
	}

		if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_RIGHT_SIDE, 1, 2);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, 2, 3);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_LEFT_SIDE, 3, 0);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_SIDE, 0, 1);
		}
		else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 2, 3);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 1, 2);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 3, 0);
		}
		else {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 3, 4);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 2, 3);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_LEFT_SIDE, 1, 2);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 5, 0);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_RIGHT_SIDE, 4, 5);
			}
			else {
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 3, 4);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, 4, 5);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 5, 0);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_SIDE, 1, 2);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 2, 3);
			}
		}
	}
#undef DRAW_SIDE_IF_NEEDED
}

Vector<Point2> TileSet::get_terrain_polygon(int p_terrain_set)
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		return _get_square_terrain_polygon(tile_size);
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		return _get_isometric_terrain_polygon(tile_size);
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}
		return _get_half_offset_terrain_polygon(tile_size, overlap, tile_offset_axis);
	}
}

Vector<Point2> TileSet::get_terrain_peering_bit_polygon(
	int p_terrain_set, TileSet::CellNeighbor p_bit)
{
	ERR_FAIL_COND_V(
		p_terrain_set < 0 || p_terrain_set >= get_terrain_sets_count(), Vector<Point2>());

	TileSet::TerrainMode terrain_mode = get_terrain_set_mode(p_terrain_set);

	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_square_corner_or_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_square_corner_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_square_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_isometric_corner_or_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_isometric_corner_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_isometric_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_half_offset_corner_or_side_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_half_offset_corner_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_half_offset_side_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
	}
}

#define TERRAIN_ALPHA 0.6

Vector<Vector<Ref<Texture2D>>> TileSet::generate_terrains_icons(Size2i p_size)
{
	// Counts the number of matching terrain tiles and find the best matching icon.
	struct Count
	{
		int count = 0;
		float probability = 0.0;
		Ref<Texture2D> texture;
		Rect2i region;
	};

	Vector<Vector<Ref<Texture2D>>> output;
	LocalVector<LocalVector<Count>> counts;
	output.resize(get_terrain_sets_count());
	counts.resize(get_terrain_sets_count());
	for (int terrain_set = 0; terrain_set < get_terrain_sets_count(); terrain_set++) {
		output.write[terrain_set].resize(get_terrains_count(terrain_set));
		counts[terrain_set].resize(get_terrains_count(terrain_set));
	}

	for (int source_index = 0; source_index < get_source_count(); source_index++) {
		int source_id = get_source_id(source_index);
		Ref<TileSetSource> source = get_source(source_id);

		Ref<TileSetAtlasSource> atlas_source = source;
		if (atlas_source.is_valid()) {
			for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
				Vector2i tile_id = source->get_tile_id(tile_index);
				for (int alternative_index = 0;
					 alternative_index < source->get_alternative_tiles_count(tile_id);
					 alternative_index++) {
					int alternative_id =
						source->get_alternative_tile_id(tile_id, alternative_index);

					TileData* tile_data = atlas_source->get_tile_data(tile_id, alternative_id);
					int terrain_set = tile_data->get_terrain_set();
					if (terrain_set >= 0) {
						ERR_FAIL_INDEX_V(terrain_set, get_terrain_sets_count(),
							Vector<Vector<Ref<Texture2D>>>());

						LocalVector<int> bit_counts;
						bit_counts.resize(get_terrains_count(terrain_set));
						for (int terrain = 0; terrain < get_terrains_count(terrain_set);
							 terrain++) {
							bit_counts[terrain] = 0;
						}
						if (tile_data->get_terrain() >= 0) {
							bit_counts[tile_data->get_terrain()] += 10;
						}
						for (int terrain_bit = 0; terrain_bit < TileSet::CELL_NEIGHBOR_MAX;
							 terrain_bit++) {
							TileSet::CellNeighbor cell_neighbor =
								TileSet::CellNeighbor(terrain_bit);
							if (is_valid_terrain_peering_bit(terrain_set, cell_neighbor)) {
								int terrain = tile_data->get_terrain_peering_bit(cell_neighbor);
								if (terrain >= 0) {
									if (terrain >= (int)bit_counts.size()) {
										WARN_PRINT(
											vformat("Invalid terrain peering bit: %d", terrain));
									}
									else {
										bit_counts[terrain] += 1;
									}
								}
							}
						}

						for (int terrain = 0; terrain < get_terrains_count(terrain_set);
							 terrain++) {
							if ((bit_counts[terrain] > counts[terrain_set][terrain].count) ||
								(bit_counts[terrain] == counts[terrain_set][terrain].count &&
									tile_data->get_probability() >
										counts[terrain_set][terrain].probability)) {
								counts[terrain_set][terrain].count = bit_counts[terrain];
								counts[terrain_set][terrain].probability =
									tile_data->get_probability();
								counts[terrain_set][terrain].texture = atlas_source->get_texture();
								counts[terrain_set][terrain].region =
									atlas_source->get_tile_texture_region(tile_id);
							}
						}
					}
				}
			}
		}
	}

	// Generate the icons.
	for (int terrain_set = 0; terrain_set < get_terrain_sets_count(); terrain_set++) {
		for (int terrain = 0; terrain < get_terrains_count(terrain_set); terrain++) {
			Ref<Image> dst_image;
			dst_image.instantiate();
			if (counts[terrain_set][terrain].count > 0) {
				// Get the best tile.
				Ref<Texture2D> src_texture = counts[terrain_set][terrain].texture;
				ERR_FAIL_COND_V(src_texture.is_null(), output);
				Ref<Image> src_image = src_texture->get_image();
				ERR_FAIL_COND_V(src_image.is_null(), output);
				Rect2i region = counts[terrain_set][terrain].region;

				dst_image->initialize_data(
					region.size.x, region.size.y, false, src_image->get_format());
				dst_image->blit_rect(src_image, region, Point2i());
				dst_image->convert(Image::FORMAT_RGBA8);
				dst_image->resize(p_size.x, p_size.y, Image::INTERPOLATE_NEAREST);
			}
			else {
				dst_image->initialize_data(1, 1, false, Image::FORMAT_RGBA8);
				dst_image->set_pixel(0, 0, get_terrain_color(terrain_set, terrain));
			}
			Ref<ImageTexture> icon = ImageTexture::create_from_image(dst_image);
			icon->set_size_override(p_size);
			output.write[terrain_set].write[terrain] = icon;
		}
	}
	return output;
}

void TileSet::_source_changed()
{
	terrains_cache_dirty = true;
	emit_changed();
}

Vector<Point2> TileSet::_get_square_terrain_polygon(Vector2i p_size)
{
	Rect2 rect(-Vector2(p_size) / 6.0, Vector2(p_size) / 3.0);
	return {rect.position, Vector2(rect.get_end().x, rect.position.y), rect.get_end(),
		Vector2(rect.position.x, rect.get_end().y)};
}

Vector<Point2> TileSet::_get_square_corner_or_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Rect2 bit_rect;
	bit_rect.size = Vector2(p_size) / 3;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
		bit_rect.position = Vector2(1, -1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
		bit_rect.position = Vector2(1, 1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
		bit_rect.position = Vector2(-1, 1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
		bit_rect.position = Vector2(-3, 1);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
		bit_rect.position = Vector2(-3, -1);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
		bit_rect.position = Vector2(-3, -3);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_SIDE:
		bit_rect.position = Vector2(-1, -3);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
		bit_rect.position = Vector2(1, -3);
		break;
	default:
		break;
	}
	bit_rect.position *= Vector2(p_size) / 6.0;

	Vector<Vector2> polygon = {bit_rect.position,
		Vector2(bit_rect.get_end().x, bit_rect.position.y), bit_rect.get_end(),
		Vector2(bit_rect.position.x, bit_rect.get_end().y)};

	return polygon;
}

Vector<Point2> TileSet::_get_square_corner_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-1, 1) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(1, -1) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_square_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
		polygon.push_back(Vector2(1, -1) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
		polygon.push_back(Vector2(-1, 1) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(-1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_SIDE:
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(1, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_terrain_polygon(Vector2i p_size)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	return {
		Vector2(1, 0) * unit,
		Vector2(0, 1) * unit,
		Vector2(-1, 0) * unit,
		Vector2(0, -1) * unit,
	};
}

Vector<Point2> TileSet::_get_isometric_corner_or_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(2, -1) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(2, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(1, 2) * unit);
		polygon.push_back(Vector2(2, 1) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(-1, 2) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(1, 2) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(-1, 2) * unit);
		polygon.push_back(Vector2(-2, 1) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-2, -1) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-2, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(-1, -2) * unit);
		polygon.push_back(Vector2(-2, -1) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_CORNER:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(-1, -2) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(1, -2) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(1, -2) * unit);
		polygon.push_back(Vector2(2, -1) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_corner_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
		polygon.push_back(Vector2(0.5, -0.5) * unit);
		polygon.push_back(Vector2(1.5, -1.5) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1.5, 1.5) * unit);
		polygon.push_back(Vector2(0.5, 0.5) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
		polygon.push_back(Vector2(-0.5, 0.5) * unit);
		polygon.push_back(Vector2(-1.5, 1.5) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(1.5, 1.5) * unit);
		polygon.push_back(Vector2(0.5, 0.5) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
		polygon.push_back(Vector2(-0.5, -0.5) * unit);
		polygon.push_back(Vector2(-1.5, -1.5) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1.5, 1.5) * unit);
		polygon.push_back(Vector2(-0.5, 0.5) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_CORNER:
		polygon.push_back(Vector2(-0.5, -0.5) * unit);
		polygon.push_back(Vector2(-1.5, -1.5) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(1.5, -1.5) * unit);
		polygon.push_back(Vector2(0.5, -0.5) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_terrain_polygon(
	Vector2i p_size, float p_overlap, TileSet::TileOffsetAxis p_offset_axis)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		return {
			Vector2(1, 1.0 - p_overlap * 2.0) * unit,
			Vector2(0, 1) * unit,
			Vector2(-1, 1.0 - p_overlap * 2.0) * unit,
			Vector2(-1, -1.0 + p_overlap * 2.0) * unit,
			Vector2(0, -1) * unit,
			Vector2(1, -1.0 + p_overlap * 2.0) * unit,
		};
	}
	else {
		return {
			Vector2(1, 0) * unit,
			Vector2(1.0 - p_overlap * 2.0, -1) * unit,
			Vector2(-1.0 + p_overlap * 2.0, -1) * unit,
			Vector2(-1, 0) * unit,
			Vector2(-1.0 + p_overlap * 2.0, 1) * unit,
			Vector2(1.0 - p_overlap * 2.0, 1) * unit,
		};
	}
}

Vector<Point2> TileSet::_get_half_offset_corner_or_side_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, (3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(2, 3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)), Vector2(1, 3.0 - p_overlap * 2.0),
		Vector2(0, 3), Vector2(-1, 3.0 - p_overlap * 2.0),
		Vector2(-2, 3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-3, (3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(-3, -(3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-2, -3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(-1, -(3.0 - p_overlap * 2.0)), Vector2(0, -3), Vector2(1, -(3.0 - p_overlap * 2.0)),
		Vector2(2, -3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(3, -(3.0 * (1.0 - p_overlap * 2.0)) / 2.0)};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			polygon.push_back(point_list[17]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[12]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			polygon.push_back(point_list[12]);
			polygon.push_back(point_list[13]);
			polygon.push_back(point_list[14]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[14]);
			polygon.push_back(point_list[15]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[15]);
			polygon.push_back(point_list[16]);
			polygon.push_back(point_list[17]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			polygon.push_back(point_list[17]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[15]);
			polygon.push_back(point_list[16]);
			polygon.push_back(point_list[17]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[14]);
			polygon.push_back(point_list[15]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			polygon.push_back(point_list[12]);
			polygon.push_back(point_list[13]);
			polygon.push_back(point_list[14]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[12]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_corner_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, 0), Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(1.5, (3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0), Vector2(0, 3),
		Vector2(-1.5, (3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(-3, 0),
		Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-1.5, -(3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0), Vector2(0, -3),
		Vector2(1.5, -(3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0),
		Vector2(3, -3.0 * (1.0 - p_overlap * 2.0))};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[0]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_side_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(0, 3),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(0, -3), Vector2(3, -3.0 * (1.0 - p_overlap * 2.0))};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector2i TileSet::transform_coords_layout(const Vector2i& p_coords,
	TileSet::TileOffsetAxis p_offset_axis, TileSet::TileLayout p_from_layout,
	TileSet::TileLayout p_to_layout)
{
	// Transform to stacked layout.
	Vector2i output = p_coords;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
		SWAP(output.x, output.y);
	}
	switch (p_from_layout) {
	case TileSet::TILE_LAYOUT_STACKED:
		break;
	case TileSet::TILE_LAYOUT_STACKED_OFFSET:
		if (output.y % 2) {
			output.x -= 1;
		}
		break;
	case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
	case TileSet::TILE_LAYOUT_STAIRS_DOWN:
		if ((p_from_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y < 0 && bool(output.y % 2)) {
				output = Vector2i(output.x + output.y / 2 - 1, output.y);
			}
			else {
				output = Vector2i(output.x + output.y / 2, output.y);
			}
		}
		else {
			if (output.x < 0 && bool(output.x % 2)) {
				output = Vector2i(output.x / 2 - 1, output.x + output.y * 2);
			}
			else {
				output = Vector2i(output.x / 2, output.x + output.y * 2);
			}
		}
		break;
	case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
	case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
		if ((p_from_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if ((output.x + output.y) < 0 && (output.x - output.y) % 2) {
				output = Vector2i((output.x + output.y) / 2 - 1, output.y - output.x);
			}
			else {
				output = Vector2i((output.x + output.y) / 2, -output.x + output.y);
			}
		}
		else {
			if ((output.x - output.y) < 0 && (output.x + output.y) % 2) {
				output = Vector2i((output.x - output.y) / 2 - 1, output.x + output.y);
			}
			else {
				output = Vector2i((output.x - output.y) / 2, output.x + output.y);
			}
		}
		break;
	}

	switch (p_to_layout) {
	case TileSet::TILE_LAYOUT_STACKED:
		break;
	case TileSet::TILE_LAYOUT_STACKED_OFFSET:
		if (output.y % 2) {
			output.x += 1;
		}
		break;
	case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
	case TileSet::TILE_LAYOUT_STAIRS_DOWN:
		if ((p_to_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y < 0 && (output.y % 2)) {
				output = Vector2i(output.x - output.y / 2 + 1, output.y);
			}
			else {
				output = Vector2i(output.x - output.y / 2, output.y);
			}
		}
		else {
			if (output.y % 2) {
				if (output.y < 0) {
					output = Vector2i(2 * output.x + 1, -output.x + output.y / 2 - 1);
				}
				else {
					output = Vector2i(2 * output.x + 1, -output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(2 * output.x, -output.x + output.y / 2);
			}
		}
		break;
	case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
	case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
		if ((p_to_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y % 2) {
				if (output.y > 0) {
					output = Vector2i(output.x - output.y / 2, output.x + output.y / 2 + 1);
				}
				else {
					output = Vector2i(output.x - output.y / 2 + 1, output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(output.x - output.y / 2, output.x + output.y / 2);
			}
		}
		else {
			if (output.y % 2) {
				if (output.y < 0) {
					output = Vector2i(output.x + output.y / 2, -output.x + output.y / 2 - 1);
				}
				else {
					output = Vector2i(output.x + output.y / 2 + 1, -output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(output.x + output.y / 2, -output.x + output.y / 2);
			}
		}
		break;
	}

	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
		SWAP(output.x, output.y);
	}

	return output;
}

const Vector2i TileSetSource::INVALID_ATLAS_COORDS = Vector2i(-1, -1);
const int TileSetSource::INVALID_TILE_ALTERNATIVE = -1;

void TileSet::reset_state() {}

TileSet::TileSet()
{
	// Instantiate the tile meshes.
	tile_lines_mesh.instantiate();
	tile_filled_mesh.instantiate();
}

TileSet::~TileSet()
{
#ifndef DISABLE_DEPRECATED
	for (const KeyValue<int, CompatibilityTileData*>& E : compatibility_data) {
		memdelete(E.value);
	}
#endif // DISABLE_DEPRECATED
	while (!source_ids.is_empty()) {
		remove_source(source_ids[0]);
	}
}

void TileSetSource::set_tile_set(const TileSet* p_tile_set) { tile_set = p_tile_set; }

TileSet* TileSetSource::get_tile_set() const { return (TileSet*)tile_set; }

void TileSetSource::reset_state() { tile_set = nullptr; }

void TileSetAtlasSource::set_tile_set(const TileSet* p_tile_set)
{
	tile_set = p_tile_set;

	// Set the TileSet on all TileData.
	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (KeyValue<int, TileData*>& E_alternative : E_tile.value.alternatives) {
			E_alternative.value->set_tile_set(tile_set);
		}
	}
}

const TileSet* TileSetAtlasSource::get_tile_set() const { return tile_set; }

void TileSetAtlasSource::notify_tile_data_properties_should_change()
{
	// Set the TileSet on all TileData.
	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (KeyValue<int, TileData*>& E_alternative : E_tile.value.alternatives) {
			E_alternative.value->notify_tile_data_properties_should_change();
		}
	}
}

void TileSetAtlasSource::add_occlusion_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_occlusion_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_occlusion_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_occlusion_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_occlusion_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_occlusion_layer(p_index);
		}
	}
}

#ifndef PHYSICS_2D_DISABLED
void TileSetAtlasSource::add_physics_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_physics_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_physics_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_physics_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_physics_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_physics_layer(p_index);
		}
	}
}
#endif // PHYSICS_2D_DISABLED

void TileSetAtlasSource::add_terrain_set(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_terrain_set(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_terrain_set(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_terrain_set(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_terrain_set(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_terrain_set(p_index);
		}
	}
}

void TileSetAtlasSource::add_terrain(int p_terrain_set, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_terrain(p_terrain_set, p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_terrain(int p_terrain_set, int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_terrain(p_terrain_set, p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_terrain(int p_terrain_set, int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_terrain(p_terrain_set, p_index);
		}
	}
}

#ifndef NAVIGATION_2D_DISABLED
void TileSetAtlasSource::add_navigation_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_navigation_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_navigation_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_navigation_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_navigation_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_navigation_layer(p_index);
		}
	}
}
#endif // NAVIGATION_2D_DISABLED

void TileSetAtlasSource::add_custom_data_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_custom_data_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_custom_data_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_custom_data_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_custom_data_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_custom_data_layer(p_index);
		}
	}
}

void TileSetAtlasSource::reset_state()
{
	tile_set = nullptr;

	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (const KeyValue<int, TileData*>& E_tile_data : E_tile.value.alternatives) {
			memdelete(E_tile_data.value);
		}
	}
	_coords_mapping_cache.clear();
	tiles.clear();
	tiles_ids.clear();
	_queue_update_padded_texture();
}

Ref<Texture2D> TileSetAtlasSource::get_texture() const { return texture; }

void TileSetAtlasSource::set_margins(Vector2i p_margins)
{
	if (p_margins.x < 0 || p_margins.y < 0) {
		WARN_PRINT("Atlas source margins should be positive.");
		margins = p_margins.maxi(0);
	}
	else {
		margins = p_margins;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_margins() const { return margins; }

void TileSetAtlasSource::set_separation(Vector2i p_separation)
{
	if (p_separation.x < 0 || p_separation.y < 0) {
		WARN_PRINT("Atlas source separation should be positive.");
		separation = p_separation.maxi(0);
	}
	else {
		separation = p_separation;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_separation() const { return separation; }

void TileSetAtlasSource::set_texture_region_size(Vector2i p_tile_size)
{
	if (p_tile_size.x <= 0 || p_tile_size.y <= 0) {
		WARN_PRINT("Atlas source tile_size should be strictly positive.");
		texture_region_size = p_tile_size.maxi(1);
	}
	else {
		texture_region_size = p_tile_size;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_texture_region_size() const { return texture_region_size; }

void TileSetAtlasSource::set_use_texture_padding(bool p_use_padding)
{
	if (use_texture_padding == p_use_padding) {
		return;
	}
	use_texture_padding = p_use_padding;
	_queue_update_padded_texture();
	emit_changed();
}

bool TileSetAtlasSource::get_use_texture_padding() const { return use_texture_padding; }

Vector2i TileSetAtlasSource::get_atlas_grid_size() const
{
	Ref<Texture2D> txt = get_texture();
	if (txt.is_null()) {
		return Vector2i();
	}

	ERR_FAIL_COND_V(texture_region_size.x <= 0 || texture_region_size.y <= 0, Vector2i());

	Size2i valid_area = txt->get_size() - margins;

	// Compute the number of valid tiles in the tiles atlas
	Size2i grid_size;
	if (valid_area.x >= texture_region_size.x && valid_area.y >= texture_region_size.y) {
		valid_area -= texture_region_size;
		grid_size = Size2i(1, 1) + valid_area / (texture_region_size + separation);
	}
	return grid_size;
}

void TileSetAtlasSource::remove_tile(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	// Remove all covered positions from the mapping cache
	_clear_coords_mapping_cache(p_atlas_coords);

	// Free tile data.
	for (const KeyValue<int, TileData*>& E_tile_data : tiles[p_atlas_coords].alternatives) {
		memdelete(E_tile_data.value);
	}

	// Delete the tile
	tiles.erase(p_atlas_coords);
	tiles_ids.erase(p_atlas_coords);
	tiles_ids.sort();

	_queue_update_padded_texture();

	_try_emit_changed();
}

bool TileSetAtlasSource::has_tile(Vector2i p_atlas_coords) const
{
	return tiles.has(p_atlas_coords);
}

Vector2i TileSetAtlasSource::get_tile_at_coords(Vector2i p_atlas_coords) const
{
	if (!_coords_mapping_cache.has(p_atlas_coords)) {
		return INVALID_ATLAS_COORDS;
	}

	return _coords_mapping_cache[p_atlas_coords];
}

void TileSetAtlasSource::set_tile_animation_columns(
	const Vector2i p_atlas_coords, int p_frame_columns)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_frame_columns < 0);

	TileAlternativesData& tad = tiles[p_atlas_coords];
	bool room_for_tile = has_room_for_tile(p_atlas_coords, tad.size_in_atlas, p_frame_columns,
		tad.animation_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile, "Cannot set animation columns count, tiles are already "
									  "present in the space the tile would cover.");

	_clear_coords_mapping_cache(p_atlas_coords);

	tiles[p_atlas_coords].animation_columns = p_frame_columns;

	_create_coords_mapping_cache(p_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

int TileSetAtlasSource::get_tile_animation_columns(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_columns;
}

void TileSetAtlasSource::set_tile_animation_separation(
	const Vector2i p_atlas_coords, const Vector2i p_separation)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_separation.x < 0 || p_separation.y < 0);

	TileAlternativesData& tad = tiles[p_atlas_coords];
	bool room_for_tile = has_room_for_tile(p_atlas_coords, tad.size_in_atlas, tad.animation_columns,
		p_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile, "Cannot set animation columns count, tiles are already "
									  "present in the space the tile would cover.");

	_clear_coords_mapping_cache(p_atlas_coords);

	tiles[p_atlas_coords].animation_separation = p_separation;

	_create_coords_mapping_cache(p_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

Vector2i TileSetAtlasSource::get_tile_animation_separation(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Vector2i(),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_separation;
}

void TileSetAtlasSource::set_tile_animation_speed(const Vector2i p_atlas_coords, real_t p_speed)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_speed <= 0);

	tiles[p_atlas_coords].animation_speed = p_speed;

	_try_emit_changed();
}

real_t TileSetAtlasSource::get_tile_animation_speed(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1.0,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_speed;
}

void TileSetAtlasSource::set_tile_animation_mode(
	const Vector2i p_atlas_coords, TileSetAtlasSource::TileAnimationMode p_mode)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	tiles[p_atlas_coords].animation_mode = p_mode;

	_try_emit_changed();
}

TileSetAtlasSource::TileAnimationMode TileSetAtlasSource::get_tile_animation_mode(
	const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TILE_ANIMATION_MODE_DEFAULT,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	return tiles[p_atlas_coords].animation_mode;
}

int TileSetAtlasSource::get_tile_animation_frames_count(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_frames_durations.size();
}

void TileSetAtlasSource::set_tile_animation_frame_duration(
	const Vector2i p_atlas_coords, int p_frame_index, real_t p_duration)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_INDEX(p_frame_index, (int)tiles[p_atlas_coords].animation_frames_durations.size());
	ERR_FAIL_COND(p_duration <= 0.0);

	tiles[p_atlas_coords].animation_frames_durations[p_frame_index] = p_duration;

	_try_emit_changed();
}

real_t TileSetAtlasSource::get_tile_animation_frame_duration(
	const Vector2i p_atlas_coords, int p_frame_index) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame_index, (int)tiles[p_atlas_coords].animation_frames_durations.size(), 0.0);
	return tiles[p_atlas_coords].animation_frames_durations[p_frame_index];
}

real_t TileSetAtlasSource::get_tile_animation_total_duration(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	real_t sum = 0.0;
	for (const real_t& duration : tiles[p_atlas_coords].animation_frames_durations) {
		sum += duration;
	}
	return sum;
}

Vector2i TileSetAtlasSource::get_tile_size_in_atlas(Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Vector2i(-1, -1),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	return tiles[p_atlas_coords].size_in_atlas;
}

int TileSetAtlasSource::get_tiles_count() const { return tiles_ids.size(); }

Vector2i TileSetAtlasSource::get_tile_id(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, tiles_ids.size(), TileSetSource::INVALID_ATLAS_COORDS);
	return tiles_ids[p_index];
}

bool TileSetAtlasSource::has_room_for_tile(Vector2i p_atlas_coords, Vector2i p_size,
	int p_animation_columns, Vector2i p_animation_separation, int p_frames_count,
	Vector2i p_ignored_tile) const
{
	if (p_atlas_coords.x < 0 || p_atlas_coords.y < 0) {
		return false;
	}
	if (p_size.x <= 0 || p_size.y <= 0) {
		return false;
	}
	if (p_frames_count <= 0) {
		return false;
	}
	Size2i atlas_grid_size = get_atlas_grid_size();
	for (int frame = 0; frame < p_frames_count; frame++) {
		Vector2i frame_coords =
			p_atlas_coords + (p_size + p_animation_separation) *
								 ((p_animation_columns > 0) ? Vector2i(frame % p_animation_columns,
																  frame / p_animation_columns)
															: Vector2i(frame, 0));
		for (int x = 0; x < p_size.x; x++) {
			for (int y = 0; y < p_size.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (_coords_mapping_cache.has(coords) &&
					_coords_mapping_cache[coords] != p_ignored_tile) {
					return false;
				}
				if (coords.x >= atlas_grid_size.x || coords.y >= atlas_grid_size.y) {
					return false;
				}
			}
		}
	}
	return true;
}

bool TileSetAtlasSource::has_tiles_outside_texture() const
{
	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			return true;
		}
	}
	return false;
}

Vector<Vector2i> TileSetAtlasSource::get_tiles_outside_texture() const
{
	Vector<Vector2i> to_return;

	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			to_return.push_back(E.key);
		}
	}
	return to_return;
}

void TileSetAtlasSource::clear_tiles_outside_texture()
{
	LocalVector<Vector2i> to_remove;

	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			to_remove.push_back(E.key);
		}
	}

	for (const Vector2i& v : to_remove) {
		remove_tile(v);
	}
}

PackedVector2Array TileSetAtlasSource::get_tiles_to_be_removed_on_change(Ref<Texture2D> p_texture,
	Vector2i p_margins, Vector2i p_separation, Vector2i p_texture_region_size)
{
	ERR_FAIL_COND_V(p_margins.x < 0 || p_margins.y < 0, PackedVector2Array());
	ERR_FAIL_COND_V(p_separation.x < 0 || p_separation.y < 0, PackedVector2Array());
	ERR_FAIL_COND_V(
		p_texture_region_size.x <= 0 || p_texture_region_size.y <= 0, PackedVector2Array());

	// Compute the new atlas grid size.
	Size2 new_grid_size;
	if (p_texture.is_valid()) {
		Size2i valid_area = p_texture->get_size() - p_margins;

		// Compute the number of valid tiles in the tiles atlas
		if (valid_area.x >= p_texture_region_size.x && valid_area.y >= p_texture_region_size.y) {
			valid_area -= p_texture_region_size;
			new_grid_size = Size2i(1, 1) + valid_area / (p_texture_region_size + p_separation);
		}
	}

	Vector<Vector2> output;
	for (KeyValue<Vector2i, TileAlternativesData>& E : tiles) {
		for (unsigned int frame = 0; frame < E.value.animation_frames_durations.size(); frame++) {
			Vector2i frame_coords =
				E.key + (E.value.size_in_atlas + E.value.animation_separation) *
							((E.value.animation_columns > 0)
									? Vector2i(frame % E.value.animation_columns,
										  frame / E.value.animation_columns)
									: Vector2i(frame, 0));
			frame_coords += E.value.size_in_atlas;
			if (frame_coords.x > new_grid_size.x || frame_coords.y > new_grid_size.y) {
				output.push_back(E.key);
				break;
			}
		}
	}
	return output;
}

Rect2i TileSetAtlasSource::get_tile_texture_region(Vector2i p_atlas_coords, int p_frame) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Rect2i(),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame, (int)tiles[p_atlas_coords].animation_frames_durations.size(), Rect2i());

	const TileAlternativesData& tad = tiles[p_atlas_coords];

	Vector2i size_in_atlas = tad.size_in_atlas;
	Vector2 region_size =
		texture_region_size * size_in_atlas + separation * (size_in_atlas - Vector2i(1, 1));

	Vector2i frame_coords = p_atlas_coords + (size_in_atlas + tad.animation_separation) *
												 ((tad.animation_columns > 0)
														 ? Vector2i(p_frame % tad.animation_columns,
															   p_frame / tad.animation_columns)
														 : Vector2i(p_frame, 0));
	Vector2 origin = margins + (frame_coords * (texture_region_size + separation));

	return Rect2(origin, region_size);
}

bool TileSetAtlasSource::is_position_in_tile_texture_region(
	const Vector2i p_atlas_coords, int p_alternative_tile, Vector2 p_position) const
{
	Size2 size = get_tile_texture_region(p_atlas_coords).size;
	TileData* tile_data = get_tile_data(p_atlas_coords, p_alternative_tile);
	if (tile_data->get_transpose()) {
		size = Size2(size.y, size.x);
	}
	Rect2 rect = Rect2(-size / 2 - tile_data->get_texture_origin(), size);

	return rect.has_point(p_position);
}

bool TileSetAtlasSource::is_rect_in_tile_texture_region(
	const Vector2i p_atlas_coords, int p_alternative_tile, Rect2 p_rect) const
{
	Size2 size = get_tile_texture_region(p_atlas_coords).size;
	TileData* tile_data = get_tile_data(p_atlas_coords, p_alternative_tile);

	if (tile_data->get_transpose()) {
		size = Size2(size.y, size.x);
	}
	Rect2 rect = Rect2(-size / 2 - tile_data->get_texture_origin(), size);

	return p_rect.intersection(rect) == p_rect;
}

int TileSetAtlasSource::alternative_no_transform(int p_alternative_id)
{
	return p_alternative_id & ~(TRANSFORM_FLIP_H | TRANSFORM_FLIP_V | TRANSFORM_TRANSPOSE);
}

// Getters for texture and tile region (padded or not)
Ref<Texture2D> TileSetAtlasSource::get_runtime_texture() const
{
	if (use_texture_padding) {
		return padded_texture;
	}
	else {
		return texture;
	}
}

Rect2i TileSetAtlasSource::get_runtime_tile_texture_region(
	Vector2i p_atlas_coords, int p_frame) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Rect2i(),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame, (int)tiles[p_atlas_coords].animation_frames_durations.size(), Rect2i());

	Rect2i src_rect = get_tile_texture_region(p_atlas_coords, p_frame);
	if (use_texture_padding) {
		const TileAlternativesData& tad = tiles[p_atlas_coords];
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(p_frame % tad.animation_columns, p_frame / tad.animation_columns)
						: Vector2i(p_frame, 0));
		Vector2i base_pos = frame_coords * (texture_region_size + Vector2i(2, 2)) + Vector2i(1, 1);

		return Rect2i(base_pos, src_rect.size);
	}
	else {
		return src_rect;
	}
}

void TileSetAtlasSource::move_tile_in_atlas(
	Vector2i p_atlas_coords, Vector2i p_new_atlas_coords, Vector2i p_new_size)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	TileAlternativesData& tad = tiles[p_atlas_coords];

	// Compute the actual new rect from arguments.
	Vector2i new_atlas_coords =
		(p_new_atlas_coords != INVALID_ATLAS_COORDS) ? p_new_atlas_coords : p_atlas_coords;
	Vector2i new_size = (p_new_size != Vector2i(-1, -1)) ? p_new_size : tad.size_in_atlas;

	if (new_atlas_coords == p_atlas_coords && new_size == tad.size_in_atlas) {
		return;
	}

	bool room_for_tile = has_room_for_tile(new_atlas_coords, new_size, tad.animation_columns,
		tad.animation_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile,
		vformat("Cannot move tile at position %s with size %s. Tile already present.",
			new_atlas_coords, new_size));

	_clear_coords_mapping_cache(p_atlas_coords);

	// Move the tile and update its size.
	if (new_atlas_coords != p_atlas_coords) {
		tiles[new_atlas_coords] = tiles[p_atlas_coords];
		tiles.erase(p_atlas_coords);

		tiles_ids.erase(p_atlas_coords);
		tiles_ids.push_back(new_atlas_coords);
		tiles_ids.sort();
	}
	tiles[new_atlas_coords].size_in_atlas = new_size;

	_create_coords_mapping_cache(new_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

void TileSetAtlasSource::remove_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile),
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_MSG(p_alternative_tile == 0,
		"Cannot remove the alternative with id 0, the base tile alternative cannot be removed.");

	memdelete(tiles[p_atlas_coords].alternatives[p_alternative_tile]);
	tiles[p_atlas_coords].alternatives.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.sort();

	_try_emit_changed();
}

void TileSetAtlasSource::set_alternative_tile_id(
	const Vector2i p_atlas_coords, int p_alternative_tile, int p_new_id)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile),
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_MSG(p_alternative_tile == 0,
		"Cannot change the alternative with id 0, the base tile alternative cannot be modified.");

	ERR_FAIL_COND_MSG(tiles[p_atlas_coords].alternatives.has(p_new_id),
		vformat("TileSetAtlasSource has already an alternative with id %d at %s.", p_new_id,
			String(p_atlas_coords)));

	tiles[p_atlas_coords].alternatives[p_new_id] =
		tiles[p_atlas_coords].alternatives[p_alternative_tile];
	tiles[p_atlas_coords].alternatives_ids.push_back(p_new_id);

	tiles[p_atlas_coords].alternatives.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.sort();

	_try_emit_changed();
}

bool TileSetAtlasSource::has_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), false,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].alternatives.has(alternative_no_transform(p_alternative_tile));
}

int TileSetAtlasSource::get_next_alternative_tile_id(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TileSetSource::INVALID_TILE_ALTERNATIVE,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].next_alternative_id;
}

int TileSetAtlasSource::get_alternative_tiles_count(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), -1,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].alternatives_ids.size();
}

int TileSetAtlasSource::get_alternative_tile_id(const Vector2i p_atlas_coords, int p_index) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TileSetSource::INVALID_TILE_ALTERNATIVE,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	p_index = alternative_no_transform(p_index);
	ERR_FAIL_INDEX_V(p_index, tiles[p_atlas_coords].alternatives_ids.size(),
		TileSetSource::INVALID_TILE_ALTERNATIVE);

	return tiles[p_atlas_coords].alternatives_ids[p_index];
}

TileData* TileSetAtlasSource::get_tile_data(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

TileSetAtlasSource::~TileSetAtlasSource()
{
	// Free everything needed.
	for (KeyValue<Vector2i, TileAlternativesData>& E_alternatives : tiles) {
		for (KeyValue<int, TileData*>& E_tile_data : E_alternatives.value.alternatives) {
			memdelete(E_tile_data.value);
		}
	}
}

TileData* TileSetAtlasSource::_get_atlas_tile_data(Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

const TileData* TileSetAtlasSource::_get_atlas_tile_data(
	Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

void TileSetAtlasSource::_compute_next_alternative_id(const Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	while (tiles[p_atlas_coords].alternatives.has(tiles[p_atlas_coords].next_alternative_id)) {
		tiles[p_atlas_coords].next_alternative_id =
			(tiles[p_atlas_coords].next_alternative_id % 1073741823) + 1; // 2 ** 30
	};
}

void TileSetAtlasSource::_clear_coords_mapping_cache(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	TileAlternativesData& tad = tiles[p_atlas_coords];
	for (int frame = 0; frame < (int)tad.animation_frames_durations.size(); frame++) {
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(frame % tad.animation_columns, frame / tad.animation_columns)
						: Vector2i(frame, 0));
		for (int x = 0; x < tad.size_in_atlas.x; x++) {
			for (int y = 0; y < tad.size_in_atlas.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (!_coords_mapping_cache.has(coords)) {
					WARN_PRINT(vformat("TileSetAtlasSource has no cached tile at position %s, the "
									   "position cache might be corrupted.",
						coords));
				}
				else {
					if (_coords_mapping_cache[coords] != p_atlas_coords) {
						WARN_PRINT(vformat("The position cache at position %s is pointing to a "
										   "wrong tile, the position cache might be corrupted.",
							coords));
					}
					_coords_mapping_cache.erase(coords);
				}
			}
		}
	}
}

void TileSetAtlasSource::_create_coords_mapping_cache(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	TileAlternativesData& tad = tiles[p_atlas_coords];
	for (int frame = 0; frame < (int)tad.animation_frames_durations.size(); frame++) {
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(frame % tad.animation_columns, frame / tad.animation_columns)
						: Vector2i(frame, 0));
		for (int x = 0; x < tad.size_in_atlas.x; x++) {
			for (int y = 0; y < tad.size_in_atlas.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (_coords_mapping_cache.has(coords)) {
					WARN_PRINT(vformat("The cache already has a tile for position %s, the position "
									   "cache might be corrupted.",
						coords));
				}
				_coords_mapping_cache[coords] = p_atlas_coords;
			}
		}
	}
}

void TileSetAtlasSource::_try_emit_changed()
{
	if (!initializing) {
		emit_changed();
	}
}

void TileSetScenesCollectionSource::_compute_next_alternative_id()
{
	while (scenes.has(next_scene_id)) {
		next_scene_id = (next_scene_id % 1073741823) + 1; // 2 ** 30
	};
}

void TileSetScenesCollectionSource::_try_emit_changed()
{
	if (!initializing) {
		emit_changed();
	}
}

int TileSetScenesCollectionSource::get_tiles_count() const { return 1; }

Vector2i TileSetScenesCollectionSource::get_tile_id(int p_tile_index) const
{
	ERR_FAIL_COND_V(p_tile_index != 0, TileSetSource::INVALID_ATLAS_COORDS);
	return Vector2i();
}

bool TileSetScenesCollectionSource::has_tile(Vector2i p_atlas_coords) const
{
	return p_atlas_coords == Vector2i();
}

int TileSetScenesCollectionSource::get_alternative_tiles_count(const Vector2i p_atlas_coords) const
{
	return scenes_ids.size();
}

int TileSetScenesCollectionSource::get_alternative_tile_id(
	const Vector2i p_atlas_coords, int p_index) const
{
	ERR_FAIL_COND_V(p_atlas_coords != Vector2i(), TileSetSource::INVALID_TILE_ALTERNATIVE);
	ERR_FAIL_INDEX_V(p_index, scenes_ids.size(), TileSetSource::INVALID_TILE_ALTERNATIVE);

	return scenes_ids[p_index];
}

bool TileSetScenesCollectionSource::has_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V(p_atlas_coords != Vector2i(), false);
	return scenes.has(TileSetAtlasSource::alternative_no_transform(p_alternative_tile));
}

int TileSetScenesCollectionSource::create_scene_tile(
	Ref<PackedScene> p_packed_scene, int p_id_override)
{
	ERR_FAIL_COND_V_MSG(p_id_override >= 0 && scenes.has(p_id_override), INVALID_TILE_ALTERNATIVE,
		vformat("Cannot create scene tile. Another scene tile exists with id %d.", p_id_override));

	int new_scene_id = p_id_override >= 0 ? p_id_override : next_scene_id;

	scenes[new_scene_id] = SceneData();
	scenes_ids.push_back(new_scene_id);
	scenes_ids.sort();
	set_scene_tile_scene(new_scene_id, p_packed_scene);
	_compute_next_alternative_id();

	_try_emit_changed();

	return new_scene_id;
}

void TileSetScenesCollectionSource::set_scene_tile_id(int p_id, int p_new_id)
{
	ERR_FAIL_COND(p_new_id < 0);
	ERR_FAIL_COND(!has_scene_tile_id(p_id));
	ERR_FAIL_COND(has_scene_tile_id(p_new_id));

	scenes[p_new_id] = SceneData();
	scenes[p_new_id] = scenes[p_id];
	scenes_ids.push_back(p_new_id);
	scenes_ids.sort();

	_compute_next_alternative_id();

	scenes.erase(p_id);
	scenes_ids.erase(p_id);

	_try_emit_changed();
}

void TileSetScenesCollectionSource::set_scene_tile_scene(int p_id, Ref<PackedScene> p_packed_scene)
{

	ERR_FAIL_COND(!scenes.has(p_id));
	if (p_packed_scene.is_valid()) {
		// Check if it extends CanvasItem.
		Ref<SceneState> scene_state = p_packed_scene->get_state();
		String type;
		while (scene_state.is_valid() && type.is_empty()) {
			// Make sure we have a root node. Supposed to be at 0 index because find_node_by_path()
			// does not seem to work.
			ERR_FAIL_COND(scene_state->get_node_count() < 1);

			type = scene_state->get_node_type(0);
			scene_state = scene_state->get_base_scene_state();
		}
		ERR_FAIL_COND_EDMSG(
			type.is_empty(), vformat("Invalid PackedScene for TileSetScenesCollectionSource: %s. "
									 "Could not get the type of the root node.",
								 p_packed_scene->get_path()));
		scenes[p_id].scene = p_packed_scene;
	}
	else {
		scenes[p_id].scene = Ref<PackedScene>();
	}
	_try_emit_changed();
}

Ref<PackedScene> TileSetScenesCollectionSource::get_scene_tile_scene(int p_id) const
{
	int scene_tile = TileSetAtlasSource::alternative_no_transform(p_id);
	ERR_FAIL_COND_V(!scenes.has(scene_tile), Ref<PackedScene>());
	return scenes[scene_tile].scene;
}

void TileSetScenesCollectionSource::set_scene_tile_display_placeholder(
	int p_id, bool p_display_placeholder)
{
	ERR_FAIL_COND(!scenes.has(p_id));

	scenes[p_id].display_placeholder = p_display_placeholder;

	_try_emit_changed();
}

bool TileSetScenesCollectionSource::get_scene_tile_display_placeholder(int p_id) const
{
	p_id = TileSetAtlasSource::alternative_no_transform(p_id);
	ERR_FAIL_COND_V(!scenes.has(p_id), false);
	return scenes[p_id].display_placeholder;
}

void TileSetScenesCollectionSource::remove_scene_tile(int p_id)
{
	ERR_FAIL_COND(!scenes.has(p_id));

	scenes.erase(p_id);
	scenes_ids.erase(p_id);
	_try_emit_changed();
}

int TileSetScenesCollectionSource::get_next_scene_tile_id() const { return next_scene_id; }

void TileData::set_tile_set(const TileSet* p_tile_set)
{
	tile_set = p_tile_set;
	notify_tile_data_properties_should_change();
}

void TileData::add_occlusion_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = occluders.size();
	}
	ERR_FAIL_INDEX(p_to_pos, occluders.size() + 1);
	occluders.insert(p_to_pos, OcclusionLayerTileData());
}

void TileData::move_occlusion_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, occluders.size());
	ERR_FAIL_INDEX(p_to_pos, occluders.size() + 1);
	occluders.insert(p_to_pos, occluders[p_from_index]);
	occluders.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_occlusion_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, occluders.size());
	occluders.remove_at(p_index);
}

#ifndef PHYSICS_2D_DISABLED
void TileData::add_physics_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = physics.size();
	}
	ERR_FAIL_INDEX(p_to_pos, physics.size() + 1);
	physics.insert(p_to_pos, PhysicsLayerTileData());
}

void TileData::move_physics_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, physics.size());
	ERR_FAIL_INDEX(p_to_pos, physics.size() + 1);
	physics.insert(p_to_pos, physics[p_from_index]);
	physics.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_physics_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, physics.size());
	physics.remove_at(p_index);
}
#endif // PHYSICS_2D_DISABLED

void TileData::add_terrain_set(int p_to_pos)
{
	if (p_to_pos >= 0 && p_to_pos <= terrain_set) {
		terrain_set += 1;
	}
}

void TileData::move_terrain_set(int p_from_index, int p_to_pos)
{
	if (p_from_index == terrain_set) {
		terrain_set = (p_from_index < p_to_pos) ? p_to_pos - 1 : p_to_pos;
	}
	else {
		if (p_from_index < terrain_set) {
			terrain_set -= 1;
		}
		if (p_to_pos <= terrain_set) {
			terrain_set += 1;
		}
	}
}

void TileData::remove_terrain_set(int p_index)
{
	if (p_index == terrain_set) {
		terrain_set = -1;
		for (int i = 0; i < 16; i++) {
			terrain_peering_bits[i] = -1;
		}
	}
	else if (terrain_set > p_index) {
		terrain_set -= 1;
	}
}

void TileData::add_terrain(int p_terrain_set, int p_to_pos)
{
	if (terrain_set == p_terrain_set) {
		for (int i = 0; i < 16; i++) {
			if (p_to_pos >= 0 && p_to_pos <= terrain_peering_bits[i]) {
				terrain_peering_bits[i] += 1;
			}
		}
	}
}

void TileData::move_terrain(int p_terrain_set, int p_from_index, int p_to_pos)
{
	if (terrain_set == p_terrain_set) {
		for (int i = 0; i < 16; i++) {
			if (p_from_index == terrain_peering_bits[i]) {
				terrain_peering_bits[i] = (p_from_index < p_to_pos) ? p_to_pos - 1 : p_to_pos;
			}
			else {
				if (p_from_index < terrain_peering_bits[i]) {
					terrain_peering_bits[i] -= 1;
				}
				if (p_to_pos <= terrain_peering_bits[i]) {
					terrain_peering_bits[i] += 1;
				}
			}
		}
	}
}

void TileData::remove_terrain(int p_terrain_set, int p_index)
{
	if (terrain_set == p_terrain_set) {
		if (terrain == p_index) {
			terrain = -1;
		}
		else if (terrain > p_index) {
			terrain -= 1;
		}

		for (int i = 0; i < 16; i++) {
			if (terrain_peering_bits[i] == p_index) {
				terrain_peering_bits[i] = -1;
			}
			else if (terrain_peering_bits[i] > p_index) {
				terrain_peering_bits[i] -= 1;
			}
		}
	}
}

#ifndef NAVIGATION_2D_DISABLED
void TileData::add_navigation_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = navigation.size();
	}
	ERR_FAIL_INDEX(p_to_pos, navigation.size() + 1);
	navigation.insert(p_to_pos, NavigationLayerTileData());
}

void TileData::move_navigation_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, navigation.size());
	ERR_FAIL_INDEX(p_to_pos, navigation.size() + 1);
	navigation.insert(p_to_pos, navigation[p_from_index]);
	navigation.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_navigation_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, navigation.size());
	navigation.remove_at(p_index);
}
#endif // NAVIGATION_2D_DISABLED

void TileData::set_allow_transform(bool p_allow_transform) { allow_transform = p_allow_transform; }

bool TileData::is_allowing_transform() const { return allow_transform; }

bool TileData::get_flip_h() const { return flip_h; }

bool TileData::get_flip_v() const { return flip_v; }

bool TileData::get_transpose() const { return transpose; }

Vector2i TileData::get_texture_origin() const { return texture_origin; }

Ref<Material> TileData::get_material() const { return material; }

Color TileData::get_modulate() const { return modulate; }

int TileData::get_z_index() const { return z_index; }

int TileData::get_y_sort_origin() const { return y_sort_origin; }

#ifndef DISABLE_DEPRECATED

Ref<OccluderPolygon2D> TileData::get_occluder(
	int p_layer_id, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), Ref<OccluderPolygon2D>());
	if (get_occluder_polygons_count(p_layer_id) == 0) {
		return Ref<OccluderPolygon2D>();
	}
	return get_occluder_polygon(p_layer_id, 0, p_flip_h, p_flip_v, p_transpose);
}
#endif // DISABLE_DEPRECATED

int TileData::get_occluder_polygons_count(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), 0);
	return occluders[p_layer_id].polygons.size();
}

Ref<OccluderPolygon2D> TileData::get_occluder_polygon(
	int p_layer_id, int p_polygon_index, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), Ref<OccluderPolygon2D>());
	ERR_FAIL_INDEX_V(
		p_polygon_index, occluders[p_layer_id].polygons.size(), Ref<OccluderPolygon2D>());

	const OcclusionLayerTileData& layer_tile_data = occluders[p_layer_id];
	const Ref<OccluderPolygon2D>& occluder_polygon =
		layer_tile_data.polygons[p_polygon_index].occluder_polygon;

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return occluder_polygon;
	}

	if (occluder_polygon.is_null()) {
		return Ref<OccluderPolygon2D>();
	}

	HashMap<int, Ref<OccluderPolygon2D>>::Iterator I =
		layer_tile_data.polygons[p_polygon_index].transformed_polygon_occluders.find(key);
	if (!I) {
		Ref<OccluderPolygon2D> transformed_polygon;
		transformed_polygon.instantiate();
		transformed_polygon->set_polygon(get_transformed_vertices(
			occluder_polygon->get_polygon(), p_flip_h, p_flip_v, p_transpose));
		layer_tile_data.polygons[p_polygon_index].transformed_polygon_occluders[key] =
			transformed_polygon;
		return transformed_polygon;
	}
	else {
		return I->value;
	}
}

#ifndef PHYSICS_2D_DISABLED

Vector2 TileData::get_constant_linear_velocity(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Vector2());
	return physics[p_layer_id].linear_velocity;
}

real_t TileData::get_constant_angular_velocity(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0.0);
	return physics[p_layer_id].angular_velocity;
}

int TileData::get_collision_polygons_count(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0);
	return physics[p_layer_id].polygons.size();
}

Vector<Vector2> TileData::get_collision_polygon_points(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Vector<Vector2>());
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), Vector<Vector2>());
	return Vector<Vector2>(physics[p_layer_id].polygons[p_polygon_index].polygon);
}

bool TileData::is_collision_polygon_one_way(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), false);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), false);
	return physics[p_layer_id].polygons[p_polygon_index].one_way;
}

float TileData::get_collision_polygon_one_way_margin(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0.0);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), 0.0);
	return physics[p_layer_id].polygons[p_polygon_index].one_way_margin;
}

int TileData::get_collision_polygon_shapes_count(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), 0);
	return physics[p_layer_id].polygons[p_polygon_index].shapes.size();
}

Ref<Shape2D> TileData::get_collision_polygon_shape(int p_layer_id, int p_polygon_index,
	int shape_index, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(
		p_polygon_index, physics[p_layer_id].polygons.size(), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(shape_index, (int)physics[p_layer_id].polygons[p_polygon_index].shapes.size(),
		Ref<Shape2D>());

	const PhysicsLayerTileData& layer_tile_data = physics[p_layer_id];
	const PhysicsLayerTileData::PolygonShapeTileData& shapes_data =
		layer_tile_data.polygons[p_polygon_index];

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return shapes_data.shapes[shape_index];
	}
	if (shapes_data.shapes[shape_index].is_null()) {
		return Ref<Shape2D>();
	}

	HashMap<int, LocalVector<Ref<Shape2D>>>::Iterator I =

		shapes_data.transformed_shapes.find(key);
	if (!I) {
		int size = shapes_data.shapes.size();
		shapes_data.transformed_shapes[key].resize(size);
		for (int i = 0; i < size; i++) {
			Ref<Shape2D> transformed_polygon = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			transformed_polygon->set_points(get_transformed_vertices(
				shapes_data.shapes[i]->get_points(), p_flip_h, p_flip_v, p_transpose));
			shapes_data.transformed_shapes[key][i] = transformed_polygon;
		}
		return shapes_data.transformed_shapes[key][shape_index];
	}
	else {
		return I->value[shape_index];
	}
}
#endif // PHYSICS_2D_DISABLED

int TileData::get_terrain_set() const { return terrain_set; }

int TileData::get_terrain() const { return terrain; }

int TileData::get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_COND_V(!is_valid_terrain_peering_bit(p_peering_bit), -1);
	return terrain_peering_bits[p_peering_bit];
}

bool TileData::is_valid_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_NULL_V(tile_set, false);

	return tile_set->is_valid_terrain_peering_bit(terrain_set, p_peering_bit);
}

TileSet::TerrainsPattern TileData::get_terrains_pattern() const
{
	ERR_FAIL_NULL_V(tile_set, TileSet::TerrainsPattern());

	TileSet::TerrainsPattern output(tile_set, terrain_set);
	output.set_terrain(terrain);
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (tile_set->is_valid_terrain_peering_bit(terrain_set, TileSet::CellNeighbor(i))) {
			output.set_terrain_peering_bit(
				TileSet::CellNeighbor(i), get_terrain_peering_bit(TileSet::CellNeighbor(i)));
		}
	}
	return output;
}

#ifndef NAVIGATION_2D_DISABLED

Ref<NavigationPolygon> TileData::get_navigation_polygon(
	int p_layer_id, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, navigation.size(), Ref<NavigationPolygon>());

	const NavigationLayerTileData& layer_tile_data = navigation[p_layer_id];

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return layer_tile_data.navigation_polygon;
	}

	if (layer_tile_data.navigation_polygon.is_null()) {
		return Ref<NavigationPolygon>();
	}

	HashMap<int, Ref<NavigationPolygon>>::Iterator I =
		layer_tile_data.transformed_navigation_polygon.find(key);
	if (!I) {
		Ref<NavigationPolygon> transformed_polygon;
		transformed_polygon.instantiate();

		// Winding order:
		// - Preserve for outlines.
		// - If there are no polygons provided, preserve for vertices.
		// - If there are polygons provided, preserve for polygons, don't preserve for vertices (so
		// the vertex order is unchanged and polygons don't need reindexing).

		Vector<Vector<int>> new_polygons = layer_tile_data.navigation_polygon->get_polygons();
		if ((p_flip_h != p_flip_v) != p_transpose) {
			for (Vector<int>& polygon : new_polygons) {
				polygon.reverse();
			}
		}

		PackedVector2Array new_points =
			get_transformed_vertices(layer_tile_data.navigation_polygon->get_vertices(), p_flip_h,
				p_flip_v, p_transpose, new_polygons.is_empty());

		const Vector<Vector<Vector2>> outlines = layer_tile_data.navigation_polygon->get_outlines();
		int outline_count = outlines.size();

		Vector<Vector<Vector2>> new_outlines;
		new_outlines.resize(outline_count);

		for (int i = 0; i < outline_count; i++) {
			new_outlines.write[i] =
				get_transformed_vertices(outlines[i], p_flip_h, p_flip_v, p_transpose, true);
		}

		transformed_polygon->set_data(new_points, new_polygons, new_outlines);

		layer_tile_data.transformed_navigation_polygon[key] = transformed_polygon;
		return transformed_polygon;
	}
	else {
		return I->value;
	}
}
#endif // NAVIGATION_2D_DISABLED

float TileData::get_probability() const { return probability; }

bool TileData::has_custom_data(const String& p_layer_name) const
{
	ERR_FAIL_NULL_V(tile_set, false);
	return tile_set->has_custom_data_layer_by_name(p_layer_name);
}

PackedVector2Array TileData::get_transformed_vertices(const PackedVector2Array& p_vertices,
	bool p_flip_h, bool p_flip_v, bool p_transpose, bool p_preserve_winding_order)
{
	const Vector2* r = p_vertices.ptr();
	int size = p_vertices.size();

	PackedVector2Array new_points;
	new_points.resize_uninitialized(size);
	Vector2* w = new_points.ptrw();

	bool reverse_vertex_order = p_preserve_winding_order && ((p_flip_h != p_flip_v) != p_transpose);
	for (int i = 0; i < size; i++) {
		Vector2 v;
		if (p_transpose) {
			v = Vector2(r[i].y, r[i].x);
		}
		else {
			v = r[i];
		}

		if (p_flip_h) {
			v.x *= -1;
		}
		if (p_flip_v) {
			v.y *= -1;
		}
		w[reverse_vertex_order ? (size - 1 - i) : i] = v;
	}
	return new_points;
}

void TileSetAtlasSource::_queue_update_padded_texture() {}

void TileData::notify_tile_data_properties_should_change() {}

void TileData::remove_custom_data_layer(int) {}

void TileData::move_custom_data_layer(int p_from_index, int p_to_pos) {}

void TileData::add_custom_data_layer(int p_index) {}

void TileSet::remove_source(int p_source_id) {}

// --- tile_set.compat.inc ---

#ifndef DISABLE_DEPRECATED
#ifndef NAVIGATION_2D_DISABLED
Ref<NavigationPolygon> TileData::_get_navigation_polygon_bind_compat_84660(int p_layer_id) const
{
	return get_navigation_polygon(p_layer_id, false, false, false);
}
#endif // NAVIGATION_2D_DISABLED

Ref<OccluderPolygon2D> TileData::_get_occluder_bind_compat_84660(int p_layer_id) const
{
	return get_occluder_polygon(p_layer_id, 0, false, false, false);
}

void TileData::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from scene/resources/2d/skeleton/skeleton_modification_2d.cpp
// ============================================================================

#include "scene/2d/node_2d.h"

#include "core/config/engine.h"
#include "scene/2d/node_2d.h"

// --- Per-mode payload structs ---

struct SkeletonModification2D::LookAtData {
	int bone_idx = -1;
	NodePath bone2d_node;
	float additional_rotation = 0.0f;
	bool enable_constraint = false;
	float constraint_angle_min = 0.0f;
	float constraint_angle_max = float(Math::TAU);
	bool constraint_angle_invert = false;
	bool constraint_in_localspace = true;
};

struct SkeletonModification2D::CCDIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool rotate_from_joint = false;
		bool enable_constraint = false;
		float constraint_angle_min = 0.0f;
		float constraint_angle_max = float(Math::TAU);
		bool constraint_angle_invert = false;
		bool constraint_in_localspace = true;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	NodePath tip_node;
};

struct SkeletonModification2D::FABRIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		Vector2 magnet_position;
		bool use_target_rotation = false;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	Vector<Transform2D> transform_chain;
	float chain_tolerance = 0.01f;
	int chain_max_iterations = 10;
	Transform2D target_global_pose;
	Transform2D origin_global_pose;
};

struct SkeletonModification2D::JiggleData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool override_defaults = false;
		float stiffness = 3.0f;
		float mass = 0.75f;
		float damping = 0.75f;
		bool use_gravity = false;
		Vector2 gravity = Vector2(0, 6.0f);
		Vector2 force;
		Vector2 acceleration;
		Vector2 velocity;
		Vector2 last_position;
		Vector2 dynamic_position;
		Vector2 last_noncollision_position;
	};

	Vector<JointData> chain;
	float stiffness = 3.0f;
	float mass = 0.75f;
	float damping = 0.75f;
	bool use_gravity = false;
	Vector2 gravity = Vector2(0, 6.0f);
	bool use_colliders = false;
	uint32_t collision_mask = 1;
};

struct SkeletonModification2D::TwoBoneIKData {
	float target_minimum_distance = 0.0f;
	float target_maximum_distance = 0.0f;
	bool flip_bend_direction = false;
	NodePath joint_one_bone2d_node;
	int joint_one_bone_idx = -1;
	NodePath joint_two_bone2d_node;
	int joint_two_bone_idx = -1;
#ifdef TOOLS_ENABLED
	bool editor_draw_min_max = false;
#endif // TOOLS_ENABLED
};

struct SkeletonModification2D::PhysicalBonesData {
	struct JointData {
		NodePath physical_bone_node;
		int bone_idx = -1;
		bool simulating = false;
	};

	Vector<JointData> chain;
	bool simulation_active = false;
};

struct SkeletonModification2D::StackHolderData {
	Ref<SkeletonModificationStack2D> held_modification_stack;
};

// --- Lifecycle & mode payload management ---

SkeletonModification2D::SkeletonModification2D(Type p_type) {
	stack = nullptr;
	is_setup = false;
	_alloc_mode_data(p_type);
}

SkeletonModification2D::~SkeletonModification2D() {
	_free_mode_data();
}

void SkeletonModification2D::_alloc_mode_data(Type p_type) {
	modification_type = p_type;
	mode_data = nullptr;
	switch (p_type) {
		case MODIFICATION_NONE: {
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_LOOKAT: {
			mode_data = memnew(LookAtData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_CCDIK: {
			mode_data = memnew(CCDIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_FABRIK: {
			mode_data = memnew(FABRIKData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_JIGGLE: {
			mode_data = memnew(JiggleData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_TWOBONEIK: {
			mode_data = memnew(TwoBoneIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_PHYSICALBONES: {
			mode_data = memnew(PhysicalBonesData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_STACKHOLDER: {
			mode_data = memnew(StackHolderData);
			editor_draw_gizmo = false;
		} break;
	}
}

void SkeletonModification2D::_free_mode_data() {
	if (!mode_data) {
		modification_type = MODIFICATION_NONE;
		return;
	}
	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			memdelete(static_cast<LookAtData *>(mode_data));
			break;
		case MODIFICATION_CCDIK:
			memdelete(static_cast<CCDIKData *>(mode_data));
			break;
		case MODIFICATION_FABRIK:
			memdelete(static_cast<FABRIKData *>(mode_data));
			break;
		case MODIFICATION_JIGGLE:
			memdelete(static_cast<JiggleData *>(mode_data));
			break;
		case MODIFICATION_TWOBONEIK:
			memdelete(static_cast<TwoBoneIKData *>(mode_data));
			break;
		case MODIFICATION_PHYSICALBONES:
			memdelete(static_cast<PhysicalBonesData *>(mode_data));
			break;
		case MODIFICATION_STACKHOLDER:
			memdelete(static_cast<StackHolderData *>(mode_data));
			break;
	}
	mode_data = nullptr;
	modification_type = MODIFICATION_NONE;
}

void SkeletonModification2D::set_modification_type(Type p_type) {
	if (modification_type == p_type) {
		return;
	}
	_free_mode_data();
	_alloc_mode_data(p_type);
	if (stack) {
		_setup_modification(stack);
	}
}

void SkeletonModification2D::_ensure_mode(Type p_expected) {
	if (modification_type == MODIFICATION_NONE) {
		set_modification_type(p_expected);
	}
}

SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<LookAtData *>(mode_data) : nullptr;
}

const SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() const {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<const LookAtData *>(mode_data) : nullptr;
}

SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() {
	return modification_type == MODIFICATION_CCDIK ? static_cast<CCDIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() const {
	return modification_type == MODIFICATION_CCDIK ? static_cast<const CCDIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() {
	return modification_type == MODIFICATION_FABRIK ? static_cast<FABRIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() const {
	return modification_type == MODIFICATION_FABRIK ? static_cast<const FABRIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<JiggleData *>(mode_data) : nullptr;
}

const SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() const {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<const JiggleData *>(mode_data) : nullptr;
}

SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<TwoBoneIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() const {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<const TwoBoneIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<PhysicalBonesData *>(mode_data) : nullptr;
}

const SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() const {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<const PhysicalBonesData *>(mode_data) : nullptr;
}

SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<StackHolderData *>(mode_data) : nullptr;
}

const SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() const {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<const StackHolderData *>(mode_data) : nullptr;
}

// --- Common base helpers ---

Node2D *SkeletonModification2D::_resolve_node_2d(const NodePath &p_path) const {
	if (!stack || !stack->skeleton || !stack->skeleton->is_inside_tree() || p_path.is_empty()) {
		return nullptr;
	}
	return dynamic_cast<Node2D *>(stack->skeleton->get_node_or_null(p_path));
}

Bone2D *SkeletonModification2D::_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const {
	Bone2D *bone_node = dynamic_cast<Bone2D *>(_resolve_node_2d(p_path));
	if (bone_node) {
		r_bone_idx = bone_node->get_index_in_skeleton();
		return bone_node;
	}
	if (stack && stack->skeleton && r_bone_idx >= 0 && r_bone_idx < stack->skeleton->get_bone_count()) {
		return stack->skeleton->get_bone(r_bone_idx);
	}
	return nullptr;
}

bool SkeletonModification2D::_print_execution_error(bool p_condition, const String &p_message) {
	if (p_condition) {
		ERR_PRINT_ONCE(p_message);
	}
	return p_condition;
}

void SkeletonModification2D::reset_state() {
	stack = nullptr;
	is_setup = false;
}

void SkeletonModification2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
#ifdef TOOLS_ENABLED
	if (editor_draw_gizmo && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enabled() const {
	return enabled;
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_modification_stack() {
	return stack;
}

void SkeletonModification2D::set_is_setup(bool p_setup) {
	is_setup = p_setup;
}

bool SkeletonModification2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModification2D::set_execution_mode(int p_mode) {
	execution_mode = p_mode;
}

int SkeletonModification2D::get_execution_mode() const {
	return execution_mode;
}

void SkeletonModification2D::set_editor_draw_gizmo(bool p_draw_gizmo) {
	editor_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (is_setup && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_editor_draw_gizmo() const {
	return editor_draw_gizmo;
}

float SkeletonModification2D::clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert) {
	if (p_angle < 0) {
		p_angle = Math::TAU + p_angle;
	}
	if (p_min_bound < 0) {
		p_min_bound = Math::TAU + p_min_bound;
	}
	if (p_max_bound < 0) {
		p_max_bound = Math::TAU + p_max_bound;
	}
	if (p_min_bound > p_max_bound) {
		SWAP(p_min_bound, p_max_bound);
	}

	bool is_beyond_bounds = (p_angle < p_min_bound || p_angle > p_max_bound);
	bool is_within_bounds = (p_angle > p_min_bound && p_angle < p_max_bound);

	if ((!p_invert && is_beyond_bounds) || (p_invert && is_within_bounds)) {
		Vector2 min_bound_vec = Vector2(Math::cos(p_min_bound), Math::sin(p_min_bound));
		Vector2 max_bound_vec = Vector2(Math::cos(p_max_bound), Math::sin(p_max_bound));
		Vector2 angle_vec = Vector2(Math::cos(p_angle), Math::sin(p_angle));

		if (angle_vec.distance_squared_to(min_bound_vec) <= angle_vec.distance_squared_to(max_bound_vec)) {
			p_angle = p_min_bound;
		} else {
			p_angle = p_max_bound;
		}
	}

	return p_angle;
}

void SkeletonModification2D::editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted) {
	if (!p_operation_bone || !p_constraint_enabled || !stack || !stack->skeleton) {
		return;
	}
	const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);

	float arc_angle_min = p_min_bound;
	float arc_angle_max = p_max_bound;
	if (arc_angle_min < 0) {
		arc_angle_min = Math::TAU + arc_angle_min;
	}
	if (arc_angle_max < 0) {
		arc_angle_max = Math::TAU + arc_angle_max;
	}
	if (arc_angle_min > arc_angle_max) {
		SWAP(arc_angle_min, arc_angle_max);
	}
	arc_angle_min += p_operation_bone->get_bone_angle();
	arc_angle_max += p_operation_bone->get_bone_angle();

	if (p_constraint_in_localspace) {
		Node2D *parent_node = dynamic_cast<Node2D *>(p_operation_bone->get_parent());
		if (parent_node) {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()), parent_node->get_global_rotation() - stack->skeleton->get_global_rotation());
		} else {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
		}
	} else {
		stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
	}

	float radius = p_operation_bone->get_length() * 0.6f;
	if (p_constraint_inverted) {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min + float(Math::TAU), arc_angle_max, 32, bone_ik_color, 1.5f);
	} else {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min, arc_angle_max, 32, bone_ik_color, 1.5f);
	}
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_min), Math::sin(arc_angle_min)) * radius, bone_ik_color, 1.5f);
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_max), Math::sin(arc_angle_max)) * radius, bone_ik_color, 1.5f);
}

// --- Non-virtual dispatch: setup, gizmos, execute ---

void SkeletonModification2D::_setup_modification(SkeletonModificationStack2D *p_stack) {
	stack = p_stack;
	if (!stack) {
		is_setup = false;
		WARN_PRINT("Could not setup modification with name " + get_name());
		return;
	}
	is_setup = true;

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT: {
			LookAtData *d = _get_lookat();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->bone2d_node, d->bone_idx);
			}
		} break;
		case MODIFICATION_CCDIK: {
			CCDIKData *d = _get_ccdik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_FABRIK: {
			FABRIKData *d = _get_fabrik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_JIGGLE: {
			JiggleData *d = _get_jiggle();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					Bone2D *bone_node = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
					if (bone_node) {
						Vector2 pos = bone_node->get_global_position();
						d->chain.write[i].dynamic_position = pos;
						d->chain.write[i].last_position = pos;
						d->chain.write[i].last_noncollision_position = pos;
					}
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
			TwoBoneIKData *d = _get_twoboneik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
				_resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
			}
		} break;
		case MODIFICATION_PHYSICALBONES: {
			PhysicalBonesData *d = _get_physicalbones();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].physical_bone_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_STACKHOLDER: {
			StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->set_skeleton(stack->get_skeleton());
				d->held_modification_stack->setup();
			}
		} break;
	}
}

void SkeletonModification2D::_draw_editor_gizmo() {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
		case MODIFICATION_FABRIK:
		case MODIFICATION_JIGGLE:
		case MODIFICATION_PHYSICALBONES:
			break;
		case MODIFICATION_LOOKAT: {
			const LookAtData *d = _get_lookat();
			if (d && d->bone_idx >= 0 && d->bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *operation_bone = stack->skeleton->get_bone(d->bone_idx);
				editor_draw_angle_constraints(operation_bone, d->constraint_angle_min, d->constraint_angle_max, d->enable_constraint, d->constraint_in_localspace, d->constraint_angle_invert);
			}
		} break;
		case MODIFICATION_CCDIK: {
			const CCDIKData *d = _get_ccdik();
			if (d) {
				for (int i = 0; i < d->chain.size(); i++) {
					if (!d->chain[i].joint_draw_gizmo || d->chain[i].bone_idx < 0 || d->chain[i].bone_idx >= stack->skeleton->get_bone_count()) {
						continue;
					}
					Bone2D *operation_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
					editor_draw_angle_constraints(operation_bone, d->chain[i].constraint_angle_min, d->chain[i].constraint_angle_max, d->chain[i].enable_constraint, d->chain[i].constraint_in_localspace, d->chain[i].constraint_angle_invert);
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
#ifdef TOOLS_ENABLED
			const TwoBoneIKData *d = _get_twoboneik();
			if (d && d->editor_draw_min_max && d->joint_one_bone_idx >= 0 && d->joint_one_bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *bone_one = stack->skeleton->get_bone(d->joint_one_bone_idx);
				if (bone_one) {
					const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);
					Vector2 origin = stack->skeleton->to_local(bone_one->get_global_position());
					stack->skeleton->draw_set_transform(origin);
					if (d->target_minimum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_minimum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
					if (d->target_maximum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_maximum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
				}
			}
#endif // TOOLS_ENABLED
		} break;
		case MODIFICATION_STACKHOLDER: {
			const StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->draw_editor_gizmos();
			}
		} break;
	}
}

void SkeletonModification2D::_execute(float p_delta) {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			_execute_lookat(p_delta);
			break;
		case MODIFICATION_CCDIK:
			_execute_ccdik(p_delta);
			break;
		case MODIFICATION_FABRIK:
			_execute_fabrik(p_delta);
			break;
		case MODIFICATION_JIGGLE:
			_execute_jiggle(p_delta);
			break;
		case MODIFICATION_TWOBONEIK:
			_execute_twoboneik(p_delta);
			break;
		case MODIFICATION_PHYSICALBONES:
			_execute_physicalbones(p_delta);
			break;
		case MODIFICATION_STACKHOLDER:
			_execute_stackholder(p_delta);
			break;
	}
}

// --- Solver implementations ---

void SkeletonModification2D::_execute_lookat(float p_delta) {
	LookAtData *d = _get_lookat();
	if (!d) {
		return;
	}
	Bone2D *operation_bone = _resolve_bone_2d(d->bone2d_node, d->bone_idx);
	Node2D *target = _resolve_node_2d(target_node);
	if (!operation_bone || !target) {
		return;
	}

	Transform2D operation_transform = operation_bone->get_global_transform();
	operation_bone->look_at(target->get_global_position());
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle() + d->additional_rotation);

	if (d->enable_constraint) {
		if (d->constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(operation_transform.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_ccdik(float p_delta) {
	CCDIKData *d = _get_ccdik();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Node2D *tip = _resolve_node_2d(d->tip_node);
	if (!target || !tip) {
		return;
	}

	for (int i = 0; i < d->chain.size(); i++) {
		_execute_ccdik_joint(i, target, tip);
	}
}

void SkeletonModification2D::_execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip) {
	CCDIKData *d = _get_ccdik();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	CCDIKData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Transform2D prev_global = operation_bone->get_global_transform();
	if (j.rotate_from_joint) {
		operation_bone->look_at(p_target->get_global_position());
		operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	} else {
		Vector2 joint_pos = operation_bone->get_global_position();
		float tip_angle = joint_pos.angle_to_point(p_tip->get_global_position());
		float target_angle = joint_pos.angle_to_point(p_target->get_global_position());
		operation_bone->set_global_rotation(operation_bone->get_global_rotation() + (target_angle - tip_angle));
	}

	if (j.enable_constraint) {
		if (j.constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_fabrik(float p_delta) {
	FABRIKData *d = _get_fabrik();
	if (!d || d->chain.size() < 2) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}

	const int chain_len = d->chain.size();
	d->transform_chain.resize(chain_len);
	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
		if (!b) {
			return;
		}
		d->transform_chain.write[i] = b->get_global_transform();
	}

	d->origin_global_pose = d->transform_chain[0];
	d->target_global_pose = target->get_global_transform();

	Bone2D *final_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	float final_length = final_bone->get_length();
	float final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
	Vector2 final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;

	int iterations = 0;
	while (final_tip.distance_to(d->target_global_pose.get_origin()) > d->chain_tolerance && iterations < d->chain_max_iterations) {
		_fabrik_chain_backwards();
		_fabrik_chain_forwards();
		final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
		final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;
		iterations++;
	}

	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Transform2D prev_global = b->get_global_transform();
		Transform2D next_global = d->transform_chain[i];
		if (i == chain_len - 1 && d->chain[i].use_target_rotation) {
			next_global.set_rotation(d->target_global_pose.get_rotation());
		}
		b->set_global_transform(prev_global.interpolate_with(next_global, stack->strength));
		stack->skeleton->set_bone_local_pose_override(d->chain[i].bone_idx, b->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_fabrik_chain_backwards() {
	FABRIKData *d = _get_fabrik();
	const int last_idx = d->chain.size() - 1;
	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[last_idx].bone_idx);
	float last_angle = d->transform_chain[last_idx].get_rotation() + last_bone->get_bone_angle();
	Vector2 dir = Vector2(Math::cos(last_angle), Math::sin(last_angle));
	d->transform_chain.write[last_idx].set_origin(d->target_global_pose.get_origin() - dir * last_bone->get_length());

	for (int i = last_idx; i >= 1; i--) {
		Vector2 next_origin = d->transform_chain[i].get_origin();
		Vector2 curr_origin = d->transform_chain[i - 1].get_origin() + d->chain[i - 1].magnet_position;
		Bone2D *prev_bone = stack->skeleton->get_bone(d->chain[i - 1].bone_idx);
		Vector2 to_prev = (curr_origin - next_origin).normalized();
		d->transform_chain.write[i - 1].set_origin(next_origin + to_prev * prev_bone->get_length());
	}
}

void SkeletonModification2D::_fabrik_chain_forwards() {
	FABRIKData *d = _get_fabrik();
	d->transform_chain.write[0].set_origin(d->origin_global_pose.get_origin());
	const int chain_len = d->chain.size();

	for (int i = 0; i < chain_len - 1; i++) {
		Bone2D *curr_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Vector2 curr_origin = d->transform_chain[i].get_origin();
		Vector2 next_origin = d->transform_chain[i + 1].get_origin();
		float angle = curr_origin.angle_to_point(next_origin);
		d->transform_chain.write[i].set_rotation(angle - curr_bone->get_bone_angle());
		d->transform_chain.write[i + 1].set_origin(curr_origin + Vector2(Math::cos(angle), Math::sin(angle)) * curr_bone->get_length());
	}

	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	Vector2 last_origin = d->transform_chain[chain_len - 1].get_origin();
	float target_angle = last_origin.angle_to_point(d->target_global_pose.get_origin());
	d->transform_chain.write[chain_len - 1].set_rotation(target_angle - last_bone->get_bone_angle());
}

void SkeletonModification2D::_execute_jiggle(float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		_execute_jiggle_joint(i, target, p_delta);
	}
}

void SkeletonModification2D::_execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	JiggleData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Vector2 target_pos = p_target->get_global_position();
	float eff_mass = MAX(j.mass, CMP_EPSILON);
	j.force = (target_pos - j.dynamic_position) * j.stiffness * p_delta;
	if (j.use_gravity) {
		j.force += j.gravity * p_delta;
	}
	j.acceleration = j.force / eff_mass;
	j.velocity += j.acceleration * (1.0f - j.damping);
	j.dynamic_position += j.velocity + j.force;
	j.dynamic_position = j.dynamic_position - j.last_position + operation_bone->get_global_position();
	j.last_position = operation_bone->get_global_position();

	Transform2D prev_global = operation_bone->get_global_transform();
	operation_bone->look_at(j.dynamic_position);
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_twoboneik(float p_delta) {
	TwoBoneIKData *d = _get_twoboneik();
	if (!d) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Bone2D *bone_one = _resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
	Bone2D *bone_two = _resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
	if (!target || !bone_one || !bone_two) {
		return;
	}

	Transform2D prev_one = bone_one->get_global_transform();
	Transform2D prev_two = bone_two->get_global_transform();

	Vector2 origin = bone_one->get_global_position();
	Vector2 target_diff = target->get_global_position() - origin;
	float target_dist = target_diff.length();
	if (d->target_minimum_distance > 0.0f && target_dist < d->target_minimum_distance) {
		target_dist = d->target_minimum_distance;
	}
	if (d->target_maximum_distance > 0.0f && target_dist > d->target_maximum_distance) {
		target_dist = d->target_maximum_distance;
	}

	float len_one = bone_one->get_length();
	float len_two = bone_two->get_length();
	float total_len = len_one + len_two;

	float base_angle = target_diff.angle();
	if (target_dist >= total_len || Math::is_zero_approx(len_one) || Math::is_zero_approx(len_two)) {
		bone_one->set_global_rotation(base_angle - bone_one->get_bone_angle());
		bone_two->set_global_rotation(base_angle - bone_two->get_bone_angle());
	} else {
		float clamped_dist = MAX(target_dist, Math::abs(len_one - len_two) + CMP_EPSILON);
		float cos_a = CLAMP((clamped_dist * clamped_dist + len_one * len_one - len_two * len_two) / (2.0f * clamped_dist * len_one), -1.0f, 1.0f);
		float cos_b = CLAMP((len_one * len_one + len_two * len_two - clamped_dist * clamped_dist) / (2.0f * len_one * len_two), -1.0f, 1.0f);
		float angle_a = Math::acos(cos_a);
		float angle_b = Math::acos(cos_b);
		if (d->flip_bend_direction) {
			angle_a = -angle_a;
			angle_b = -angle_b;
		}
		float rot_one = base_angle - angle_a;
		float rot_two = rot_one + float(Math::PI) - angle_b;
		bone_one->set_global_rotation(rot_one - bone_one->get_bone_angle());
		bone_two->set_global_rotation(rot_two - bone_two->get_bone_angle());
	}

	Transform2D solved_one = bone_one->get_transform();
	Transform2D solved_two = bone_two->get_transform();
	bone_one->set_global_transform(prev_one.interpolate_with(bone_one->get_global_transform(), stack->strength));
	bone_two->set_global_transform(prev_two.interpolate_with(bone_two->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->joint_one_bone_idx, solved_one, stack->strength, false);
	stack->skeleton->set_bone_local_pose_override(d->joint_two_bone_idx, solved_two, stack->strength, false);
}

void SkeletonModification2D::_execute_physicalbones(float p_delta) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !d->simulation_active) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		PhysicalBonesData::JointData &j = d->chain.write[i];
		if (!j.simulating) {
			continue;
		}
		Node2D *phys_node = _resolve_node_2d(j.physical_bone_node);
		if (!phys_node || j.bone_idx < 0 || j.bone_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *bone_node = stack->skeleton->get_bone(j.bone_idx);
		if (!bone_node) {
			continue;
		}
		Transform2D prev_global = bone_node->get_global_transform();
		bone_node->set_global_transform(prev_global.interpolate_with(phys_node->get_global_transform(), stack->strength));
		stack->skeleton->set_bone_local_pose_override(j.bone_idx, bone_node->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_execute_stackholder(float p_delta) {
	StackHolderData *d = _get_stackholder();
	if (d && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->execute(p_delta, execution_mode);
	}
}

// --- Shared target node ---

void SkeletonModification2D::set_target_node(const NodePath &p_target_node) {
	target_node = p_target_node;
}

NodePath SkeletonModification2D::get_target_node() const {
	return target_node;
}

// --- LookAt accessors ---

void SkeletonModification2D::set_bone2d_node(const NodePath &p_target_node) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(d->bone2d_node, d->bone_idx);
	}
}

NodePath SkeletonModification2D::get_bone2d_node() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone2d_node : NodePath();
}

void SkeletonModification2D::set_bone_index(int p_idx) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone_idx = p_idx;
	if (is_setup && stack && stack->skeleton && p_idx >= 0 && p_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_idx);
		if (b) {
			d->bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_bone_index() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone_idx : -1;
}

void SkeletonModification2D::set_additional_rotation(float p_rotation) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->additional_rotation = p_rotation;
}

float SkeletonModification2D::get_additional_rotation() const {
	const LookAtData *d = _get_lookat();
	return d ? d->additional_rotation : 0.0f;
}

void SkeletonModification2D::set_enable_constraint(bool p_constraint) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enable_constraint() const {
	const LookAtData *d = _get_lookat();
	return d ? d->enable_constraint : false;
}

void SkeletonModification2D::set_constraint_angle_min(float p_angle_min) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_min() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_min : 0.0f;
}

void SkeletonModification2D::set_constraint_angle_max(float p_angle_max) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_max() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_max : float(Math::TAU);
}

void SkeletonModification2D::set_constraint_angle_invert(bool p_invert) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_angle_invert() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_invert : false;
}

void SkeletonModification2D::set_constraint_in_localspace(bool p_constraint_in_localspace) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_in_localspace() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_in_localspace : true;
}

// --- CCDIK accessors ---

void SkeletonModification2D::set_tip_node(const NodePath &p_tip_node) {
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->tip_node = p_tip_node;
}

NodePath SkeletonModification2D::get_tip_node() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->tip_node : NodePath();
}

int SkeletonModification2D::get_ccdik_data_chain_length() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_ccdik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_ccdik_joint_bone2d_node(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_ccdik_joint_bone_index(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].rotate_from_joint = p_rotate_from_joint;
}

bool SkeletonModification2D::get_ccdik_joint_rotate_from_joint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].rotate_from_joint;
}

void SkeletonModification2D::set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_enable_constraint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].enable_constraint;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_min(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_min;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_max(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_max;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_invert;
}

void SkeletonModification2D::set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_in_localspace;
}

void SkeletonModification2D::set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].joint_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].joint_draw_gizmo;
}

// --- FABRIK accessors ---

int SkeletonModification2D::get_fabrik_data_chain_length() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_fabrik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_fabrik_joint_bone2d_node(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_fabrik_joint_bone_index(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].magnet_position = p_magnet_position;
}

Vector2 SkeletonModification2D::get_fabrik_joint_magnet_position(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, Vector2());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), Vector2(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].magnet_position;
}

void SkeletonModification2D::set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].use_target_rotation = p_use_target_rotation;
}

bool SkeletonModification2D::get_fabrik_joint_use_target_rotation(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].use_target_rotation;
}

void SkeletonModification2D::set_chain_tolerance(float p_tolerance) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_tolerance <= 0.0f);
	d->chain_tolerance = p_tolerance;
}

float SkeletonModification2D::get_chain_tolerance() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_tolerance : 0.01f;
}

void SkeletonModification2D::set_chain_max_iterations(int p_iterations) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_iterations < 1);
	d->chain_max_iterations = p_iterations;
}

int SkeletonModification2D::get_chain_max_iterations() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_max_iterations : 10;
}

// --- Jiggle accessors ---

void SkeletonModification2D::_update_jiggle_joint_data() {
	JiggleData *d = _get_jiggle();
	if (!d) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		if (!d->chain[i].override_defaults) {
			d->chain.write[i].stiffness = d->stiffness;
			d->chain.write[i].mass = d->mass;
			d->chain.write[i].damping = d->damping;
			d->chain.write[i].use_gravity = d->use_gravity;
			d->chain.write[i].gravity = d->gravity;
		}
	}
}

void SkeletonModification2D::reset() {
	JiggleData *d = _get_jiggle();
	if (!d || !is_setup || !stack || !stack->skeleton) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		const int b_idx = d->chain[i].bone_idx;
		if (b_idx < 0 || b_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *b = stack->skeleton->get_bone(b_idx);
		if (b) {
			Vector2 bone_pos = b->get_global_position();
			d->chain.write[i].dynamic_position = bone_pos;
			d->chain.write[i].last_position = bone_pos;
			d->chain.write[i].last_noncollision_position = bone_pos;
			d->chain.write[i].velocity = Vector2(0, 0);
			d->chain.write[i].acceleration = Vector2(0, 0);
			d->chain.write[i].force = Vector2(0, 0);
		}
	}
}

void SkeletonModification2D::set_stiffness(float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->stiffness = p_stiffness;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_stiffness() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->stiffness : 3.0f;
}

void SkeletonModification2D::set_mass(float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->mass = p_mass;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_mass() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->mass : 0.75f;
}

void SkeletonModification2D::set_damping(float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->damping = p_damping;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_damping() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->damping : 0.75f;
}

void SkeletonModification2D::set_use_gravity(bool p_use_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_gravity = p_use_gravity;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_use_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_gravity : false;
}

void SkeletonModification2D::set_gravity(Vector2 p_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->gravity = p_gravity;
	_update_jiggle_joint_data();
}

Vector2 SkeletonModification2D::get_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->gravity : Vector2(0, 6.0f);
}

void SkeletonModification2D::set_use_colliders(bool p_use_colliders) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_colliders = p_use_colliders;
}

bool SkeletonModification2D::get_use_colliders() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_colliders : false;
}

void SkeletonModification2D::set_collision_mask(int p_mask) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->collision_mask = uint32_t(p_mask);
}

int SkeletonModification2D::get_collision_mask() const {
	const JiggleData *d = _get_jiggle();
	return d ? int(d->collision_mask) : 1;
}

int SkeletonModification2D::get_jiggle_data_chain_length() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_jiggle_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
	_update_jiggle_joint_data();
}

void SkeletonModification2D::set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_jiggle_joint_bone2d_node(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_jiggle_joint_bone_index(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_jiggle_joint_override(int p_joint_idx, bool p_override) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].override_defaults = p_override;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_jiggle_joint_override(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].override_defaults;
}

void SkeletonModification2D::set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].stiffness = p_stiffness;
}

float SkeletonModification2D::get_jiggle_joint_stiffness(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].stiffness;
}

void SkeletonModification2D::set_jiggle_joint_mass(int p_joint_idx, float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].mass = p_mass;
}

float SkeletonModification2D::get_jiggle_joint_mass(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].mass;
}

void SkeletonModification2D::set_jiggle_joint_damping(int p_joint_idx, float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].damping = p_damping;
}

float SkeletonModification2D::get_jiggle_joint_damping(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].damping;
}

void SkeletonModification2D::set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].use_gravity = p_use_gravity;
}

bool SkeletonModification2D::get_jiggle_joint_use_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].use_gravity;
}

void SkeletonModification2D::set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].gravity = p_gravity;
}

Vector2 SkeletonModification2D::get_jiggle_joint_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, Vector2(0, 0));
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), Vector2(0, 0));
	return d->chain[p_joint_idx].gravity;
}

// --- TwoBoneIK accessors ---

void SkeletonModification2D::set_target_minimum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target minimum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_minimum_distance = p_distance;
}

float SkeletonModification2D::get_target_minimum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_minimum_distance : 0.0f;
}

void SkeletonModification2D::set_target_maximum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target maximum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_maximum_distance = p_distance;
}

float SkeletonModification2D::get_target_maximum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_maximum_distance : 0.0f;
}

void SkeletonModification2D::set_flip_bend_direction(bool p_flip_direction) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->flip_bend_direction = p_flip_direction;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_flip_bend_direction() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->flip_bend_direction : false;
}

void SkeletonModification2D::set_joint_one_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_one_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_one_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_one_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_one_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_one_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone_idx : -1;
}

void SkeletonModification2D::set_joint_two_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_two_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_two_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_two_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_two_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_two_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone_idx : -1;
}

#ifdef TOOLS_ENABLED
void SkeletonModification2D::set_editor_draw_min_max(bool p_draw) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->editor_draw_min_max = p_draw;
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
}

bool SkeletonModification2D::get_editor_draw_min_max() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->editor_draw_min_max : false;
}
#endif // TOOLS_ENABLED

// --- PhysicalBones accessors ---

int SkeletonModification2D::get_physical_bone_chain_length() const {
	const PhysicalBonesData *d = _get_physicalbones();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_physical_bone_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_physical_bone_node(int p_joint_idx, const NodePath &p_nodepath) {
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Joint index out of range!");
	d->chain.write[p_joint_idx].physical_bone_node = p_nodepath;
	if (is_setup) {
		_resolve_bone_2d(p_nodepath, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_physical_bone_node(int p_joint_idx) const {
	const PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Joint index out of range!");
	return d->chain[p_joint_idx].physical_bone_node;
}

void SkeletonModification2D::fetch_physical_bones() {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !stack || !stack->skeleton) {
		return;
	}
	d->chain.clear();
	for (int i = 0; i < stack->skeleton->get_bone_count(); i++) {
		Bone2D *b = stack->skeleton->get_bone(i);
		if (!b) {
			continue;
		}
		PhysicalBonesData::JointData jd;
		jd.physical_bone_node = stack->skeleton->get_path_to(b);
		jd.bone_idx = i;
		d->chain.push_back(jd);
	}
}

void SkeletonModification2D::start_simulation(const Vector<String> &p_bones) {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->simulation_active = true;
	for (int i = 0; i < d->chain.size(); i++) {
		if (p_bones.is_empty()) {
			d->chain.write[i].simulating = true;
		} else {
			Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
			d->chain.write[i].simulating = (n && p_bones.has(n->get_name()));
		}
	}
}

void SkeletonModification2D::stop_simulation(const Vector<String> &p_bones) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d) {
		return;
	}
	if (p_bones.is_empty()) {
		d->simulation_active = false;
		for (int i = 0; i < d->chain.size(); i++) {
			d->chain.write[i].simulating = false;
		}
		return;
	}
	bool any_active = false;
	for (int i = 0; i < d->chain.size(); i++) {
		Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
		if (n && p_bones.has(n->get_name())) {
			d->chain.write[i].simulating = false;
		}
		if (d->chain[i].simulating) {
			any_active = true;
		}
	}
	d->simulation_active = any_active;
}

// --- StackHolder accessors ---

void SkeletonModification2D::set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack) {
	_ensure_mode(MODIFICATION_STACKHOLDER);
	StackHolderData *d = _get_stackholder();
	ERR_FAIL_NULL(d);
	d->held_modification_stack = p_held_stack;
	if (is_setup && stack && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->set_skeleton(stack->get_skeleton());
		d->held_modification_stack->setup();
	}
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_held_modification_stack() const {
	const StackHolderData *d = _get_stackholder();
	return d ? d->held_modification_stack : Ref<SkeletonModificationStack2D>();
}

// --- skeleton_modification_stack_2d.cpp ---

#include "scene/2d/node_2d.h"

#include "scene/2d/node_2d.h"

void SkeletonModificationStack2D::setup() {
	if (is_setup) {
		return;
	}

	if (skeleton != nullptr) {
		is_setup = true;
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			modifications.get(i)->_setup_modification(this);
		}

#ifdef TOOLS_ENABLED
		set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	} else {
		WARN_PRINT("Cannot setup SkeletonModificationStack2D: no Skeleton2D set!");
	}
}

void SkeletonModificationStack2D::execute(float p_delta, int p_execution_mode) {
	if (!is_setup || !enabled || skeleton == nullptr) {
		return;
	}

	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		if (modifications[i]->get_enabled() && modifications[i]->get_execution_mode() == p_execution_mode) {
			modifications.get(i)->_execute(p_delta);
		}
	}
}

void SkeletonModificationStack2D::draw_editor_gizmos() {
	if (!is_setup || !skeleton) {
		return;
	}

	if (editor_gizmo_dirty) {
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			if (modifications[i]->editor_draw_gizmo) {
				modifications.get(i)->_draw_editor_gizmo();
			}
		}
		skeleton->draw_set_transform(Vector2(0, 0));
		editor_gizmo_dirty = false;
	}
}

void SkeletonModificationStack2D::enable_all_modifications(bool p_enabled) {
	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		modifications.get(i)->set_enabled(p_enabled);
	}
}

Ref<SkeletonModification2D> SkeletonModificationStack2D::get_modification(int p_mod_idx) const {
	ERR_FAIL_INDEX_V(p_mod_idx, modifications.size(), nullptr);
	return modifications[p_mod_idx];
}

void SkeletonModificationStack2D::add_modification(Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_COND(p_mod.is_null());

	p_mod->_setup_modification(this);
	modifications.push_back(p_mod);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::delete_modification(int p_mod_idx) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());
	modifications.remove_at(p_mod_idx);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification(int p_mod_idx, Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());

	if (p_mod.is_null()) {
		modifications.write[p_mod_idx] = Ref<SkeletonModification2D>();
	} else {
		modifications.write[p_mod_idx] = p_mod;
		p_mod->_setup_modification(this);
	}

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification_count(int p_count) {
	ERR_FAIL_COND_MSG(p_count < 0, "SkeletonModificationStack2D modification count cannot be less than zero.");
	modifications.resize(p_count);
#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

int SkeletonModificationStack2D::get_modification_count() const {
	return modifications.size();
}

void SkeletonModificationStack2D::set_skeleton(Skeleton2D *p_skeleton) {
	skeleton = p_skeleton;
}

Skeleton2D *SkeletonModificationStack2D::get_skeleton() const {
	return skeleton;
}

bool SkeletonModificationStack2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModificationStack2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

bool SkeletonModificationStack2D::get_enabled() const {
	return enabled;
}

void SkeletonModificationStack2D::set_editor_gizmos_dirty(bool p_dirty) {
	if (!is_setup) {
		return;
	}
	editor_gizmo_dirty = p_dirty;
	if (skeleton && editor_gizmo_dirty) {
		skeleton->queue_redraw();
	}
}

void SkeletonModificationStack2D::set_strength(float p_strength) {
	ERR_FAIL_COND_MSG(p_strength < 0, "Strength cannot be less than zero!");
	ERR_FAIL_COND_MSG(p_strength > 1, "Strength cannot be more than one!");
	strength = p_strength;
}

float SkeletonModificationStack2D::get_strength() const {
	return strength;
}

SkeletonModificationStack2D::SkeletonModificationStack2D() {}

void Skeleton2D::execute_modifications(real_t p_delta, int p_execution_mode) {
	if (modification_stack.is_null()) {
		return;
	}
	if (bone_setup_dirty) {
		_update_bone_setup();
	}
	modification_stack->execute(p_delta, p_execution_mode);
}

// ============================================================================
// Merged from scene/resources/animation.cpp
// ============================================================================

#include "animation.h"
#include "core/io/marshalls.h"

void Animation::reset_state() { clear(); }

int Animation::add_track(TrackType p_type, int p_at_pos)
{
	if ((uint32_t)p_at_pos >= tracks.size()) {
		p_at_pos = tracks.size();
	}

	switch (p_type) {
	case TYPE_POSITION_3D:
	case TYPE_ROTATION_3D:
	case TYPE_SCALE_3D:
	case TYPE_BLEND_SHAPE:
	case TYPE_VALUE:
	case TYPE_METHOD:
	case TYPE_BEZIER:
	case TYPE_AUDIO:
	case TYPE_ANIMATION: {
		tracks.insert(p_at_pos, memnew(Track(p_type)));
	} break;
	default: {
		ERR_PRINT("Unknown track type");
	}
	}
	emit_changed();
	return p_at_pos;
}

void Animation::remove_track(int p_track)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];

	if (t->type == TYPE_POSITION_3D || t->type == TYPE_ROTATION_3D ||
		t->type == TYPE_SCALE_3D || t->type == TYPE_BLEND_SHAPE) {
		ERR_FAIL_COND_MSG(t->compressed_track >= 0,
			"Compressed tracks can't be manually removed. Call clear() "
			"to get rid of compression first.");
	}

	memdelete(t);
	tracks.remove_at(p_track);
	emit_changed();
	_check_capture_included();
}

bool Animation::is_capture_included() const { return capture_included; }

void Animation::_check_capture_included()
{
	capture_included = false;
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->type == TYPE_VALUE && tracks[i]->update_mode == UPDATE_CAPTURE) {
			capture_included = true;
			break;
		}
	}
}

int Animation::get_track_count() const { return tracks.size(); }

Animation::TrackType Animation::track_get_type(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), TYPE_VALUE);
	return tracks[p_track]->type;
}

void Animation::track_set_path(int p_track, const NodePath& p_path)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->path = p_path;
	tracks[p_track]->concatenated_path = String(String(tracks[p_track]->path));
	emit_changed();
}

NodePath Animation::track_get_path(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), NodePath());
	return tracks[p_track]->path;
}

int Animation::find_track(const NodePath& p_path, const TrackType p_type) const
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->path == p_path && tracks[i]->type == p_type) {
			return i;
		}
	};
	return -1;
}

Animation::TrackType Animation::get_cache_type(TrackType p_type)
{
	if (p_type == Animation::TYPE_BEZIER) {
		return Animation::TYPE_VALUE;
	}
	if (p_type == Animation::TYPE_ROTATION_3D || p_type == Animation::TYPE_SCALE_3D) {
		return Animation::TYPE_POSITION_3D; // Reference them as position3D tracks, even if they
											// modify rotation or scale.
	}
	return p_type;
}

Animation::TrackCacheID Animation::track_get_unique_id(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	return tracks[p_track]->get_unique_id();
}

void Animation::track_set_interpolation_type(int p_track, InterpolationType p_interp)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->interpolation = p_interp;
	emit_changed();
}

Animation::InterpolationType Animation::track_get_interpolation_type(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), INTERPOLATION_NEAREST);
	return tracks[p_track]->interpolation;
}

void Animation::track_set_interpolation_loop_wrap(int p_track, bool p_enable)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->loop_wrap = p_enable;
	emit_changed();
}

bool Animation::track_get_interpolation_loop_wrap(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->loop_wrap;
}

template <typename T, typename V> int Animation::_insert(double p_time, T& p_keys, const V& p_value)
{
	int idx = p_keys.size();

	while (true) {
		// Condition for replacement.
		if (idx > 0 && Math::is_equal_approx((double)p_keys[idx - 1].time, p_time)) {
			float transition = p_keys[idx - 1].transition;
			p_keys[idx - 1] = p_value;
			p_keys[idx - 1].transition = transition;
			return idx - 1;

			// Condition for insert.
		}
		else if (idx == 0 || p_keys[idx - 1].time < p_time) {
			p_keys.insert(idx, p_value);
			return idx;
		}

		idx--;
	}

	return -1;
}

int Animation::_marker_insert(
	double p_time, LocalVector<MarkerKey>& p_keys, const MarkerKey& p_value)
{
	int idx = p_keys.size();

	while (true) {
		// Condition for replacement.
		if (idx > 0 && Math::is_equal_approx((double)p_keys[idx - 1].time, p_time)) {
			p_keys[idx - 1] = p_value;
			return idx - 1;

			// Condition for insert.
		}
		else if (idx == 0 || p_keys[idx - 1].time < p_time) {
			p_keys.insert(idx, p_value);
			return idx;
		}

		idx--;
	}

	return -1;
}

int Animation::position_track_insert_key(int p_track, double p_time, const Vector3& p_position)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, -1);
	ERR_FAIL_COND_V(t->compressed_track >= 0, -1);

	TKey<Vector3> tkey;
	tkey.time = p_time;
	tkey.value = p_position;

	int ret = _insert(p_time, t->positions, tkey);
	emit_changed();
	return ret;
}

Error Animation::position_track_get_key(int p_track, int p_key, Vector3* r_position) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(t->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_position = _uncompress_pos_scale(t->compressed_track, key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->positions.size(), ERR_INVALID_PARAMETER);

	*r_position = t->positions[p_key].value;

	return OK;
}

Error Animation::try_position_track_interpolate(
	int p_track, double p_time, Vector3* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		if (_pos_scale_interpolate_compressed(t->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Vector3 tk =
		_interpolate(t->positions, p_time, t->interpolation, t->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Vector3 Animation::position_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Vector3 ret = Vector3(0, 0, 0);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_position_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "3D Position Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

int Animation::rotation_track_insert_key(int p_track, double p_time, const Quaternion& p_rotation)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, -1);
	ERR_FAIL_COND_V(t->compressed_track >= 0, -1);

	TKey<Quaternion> tkey;
	tkey.time = p_time;
	tkey.value = p_rotation;

	int ret = _insert(p_time, t->rotations, tkey);
	emit_changed();
	return ret;
}

Error Animation::rotation_track_get_key(int p_track, int p_key, Quaternion* r_rotation) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(t->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_rotation = _uncompress_quaternion(key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->rotations.size(), ERR_INVALID_PARAMETER);

	*r_rotation = t->rotations[p_key].value;

	return OK;
}

Error Animation::try_rotation_track_interpolate(
	int p_track, double p_time, Quaternion* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		if (_rotation_interpolate_compressed(t->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Quaternion tk =
		_interpolate(t->rotations, p_time, t->interpolation, t->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Quaternion Animation::rotation_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Quaternion ret = Quaternion(0, 0, 0, 1);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	Error err = try_rotation_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(err != OK, ret,
		"3D Rotation Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

int Animation::scale_track_insert_key(int p_track, double p_time, const Vector3& p_scale)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, -1);
	ERR_FAIL_COND_V(t->compressed_track >= 0, -1);

	TKey<Vector3> tkey;
	tkey.time = p_time;
	tkey.value = p_scale;

	int ret = _insert(p_time, t->scales, tkey);
	emit_changed();
	return ret;
}

Error Animation::scale_track_get_key(int p_track, int p_key, Vector3* r_scale) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(t->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_scale = _uncompress_pos_scale(t->compressed_track, key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->scales.size(), ERR_INVALID_PARAMETER);

	*r_scale = t->scales[p_key].value;

	return OK;
}

Error Animation::try_scale_track_interpolate(
	int p_track, double p_time, Vector3* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		if (_pos_scale_interpolate_compressed(t->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Vector3 tk =
		_interpolate(t->scales, p_time, t->interpolation, t->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Vector3 Animation::scale_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Vector3 ret = Vector3(1, 1, 1);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_scale_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "3D Scale Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

int Animation::blend_shape_track_insert_key(int p_track, double p_time, float p_blend_shape)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, -1);
	ERR_FAIL_COND_V(t->compressed_track >= 0, -1);

	TKey<float> tkey;
	tkey.time = p_time;
	tkey.value = p_blend_shape;

	int ret = _insert(p_time, t->blend_shapes, tkey);
	emit_changed();
	return ret;
}

Error Animation::blend_shape_track_get_key(int p_track, int p_key, float* r_blend_shape) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<1>(t->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_blend_shape = _uncompress_blend_shape(key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->blend_shapes.size(), ERR_INVALID_PARAMETER);

	*r_blend_shape = t->blend_shapes[p_key].value;

	return OK;
}

Error Animation::try_blend_shape_track_interpolate(
	int p_track, double p_time, float* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, ERR_INVALID_PARAMETER);

	if (t->compressed_track >= 0) {
		if (_blend_shape_interpolate_compressed(t->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	float tk = _interpolate(
		t->blend_shapes, p_time, t->interpolation, t->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

float Animation::blend_shape_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	float ret = 0;
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_blend_shape_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "Blend Shape Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

int Animation::track_get_key_count(int p_track) const { return 0; }

void Animation::track_remove_key_at_time(int p_track, double p_time)
{
	int idx = track_find_key(p_track, p_time, FIND_MODE_APPROX);
	ERR_FAIL_COND(idx < 0);
	track_remove_key(p_track, idx);
}

bool Animation::track_is_compressed(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	const Track* t = tracks[p_track];

	switch (t->type) {
	case TYPE_POSITION_3D:
	case TYPE_ROTATION_3D:
	case TYPE_SCALE_3D:
	case TYPE_BLEND_SHAPE:
		return t->compressed_track >= 0;
	default:
		return false; // Animation does not really use transitions.
	}
}

template <typename K>
int Animation::_find(
	const LocalVector<K>& p_keys, double p_time, bool p_backward, bool p_limit) const
{
	int len = p_keys.size();
	if (len == 0) {
		return -2;
	}

	int low = 0;
	int high = len - 1;
	int middle = 0;

#ifdef DEBUG_ENABLED
	if (low > high) {
		ERR_PRINT("low > high, this may be a bug.");
	}
#endif

	const K* keys = &p_keys[0];

	while (low <= high) {
		middle = (low + high) / 2;

		if (Math::is_equal_approx(p_time, (double)keys[middle].time)) { // match
			return middle;
		}
		else if (p_time < keys[middle].time) {
			high = middle - 1; // search low end of array
		}
		else {
			low = middle + 1; // search high end of array
		}
	}

	if (!p_backward) {
		if (keys[middle].time > p_time) {
			middle--;
		}
	}
	else {
		if (keys[middle].time < p_time) {
			middle++;
		}
	}

	if (p_limit && middle > -1 && middle < len) {
		double diff = length - keys[middle].time;
		if ((std::signbit(keys[middle].time) && !Math::is_zero_approx(keys[middle].time)) ||
			(std::signbit(diff) && !Math::is_zero_approx(diff))) {
			ERR_PRINT_ONCE_ED("Found the key outside the animation range. Consider using the "
							  "clean-up option in AnimationTrackEditor to fix it.");
			return -1;
		}
	}

	return middle;
}

Vector3 Animation::_interpolate(const Vector3& p_a, const Vector3& p_b, real_t p_c) const
{
	return p_a.lerp(p_b, p_c);
}

Quaternion Animation::_interpolate(const Quaternion& p_a, const Quaternion& p_b, real_t p_c) const
{
	return p_a.slerp(p_b, p_c);
}

Vector3 Animation::_cubic_interpolate_in_time(const Vector3& p_pre_a, const Vector3& p_a,
	const Vector3& p_b, const Vector3& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return p_a.cubic_interpolate_in_time(p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

Quaternion Animation::_cubic_interpolate_in_time(const Quaternion& p_pre_a, const Quaternion& p_a,
	const Quaternion& p_b, const Quaternion& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return p_a.spherical_cubic_interpolate_in_time(
		p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

real_t Animation::_cubic_interpolate_in_time(const real_t& p_pre_a, const real_t& p_a,
	const real_t& p_b, const real_t& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return Math::cubic_interpolate_in_time(
		p_a, p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

template <typename T>
T Animation::_interpolate(const LocalVector<TKey<T>>& p_keys, double p_time,
	InterpolationType p_interp, bool p_loop_wrap, bool* p_ok, bool p_backward) const
{
	int len = _find(p_keys, length) + 1; // try to find last key (there may be more past the end)

	if (len <= 0) {
		// (-1 or -2 returned originally) (plus one above)
		// meaning no keys, or only key time is larger than length
		if (p_ok) {
			*p_ok = false;
		}
		return T();
	}
	else if (len == 1) { // one key found (0+1), return it

		if (p_ok) {
			*p_ok = true;
		}
		return p_keys[0].value;
	}

	int idx = _find(p_keys, p_time, p_backward);

	ERR_FAIL_COND_V(idx == -2, T());
	int maxi = len - 1;
	bool is_start_edge = p_backward ? idx >= len : idx == -1;
	bool is_end_edge = p_backward ? idx == 0 : idx >= maxi;

	real_t c = 0.0;
	// Prepare for all cases of interpolation.
	real_t delta = 0.0;
	real_t from = 0.0;

	int pre = -1;
	int next = -1;
	int post = -1;
	real_t pre_t = 0.0;
	real_t to_t = 0.0;
	real_t post_t = 0.0;

	bool use_cubic = p_interp == INTERPOLATION_CUBIC || p_interp == INTERPOLATION_CUBIC_ANGLE;

	if (!p_loop_wrap || loop_mode == LOOP_NONE) {
		if (is_start_edge) {
			idx = p_backward ? maxi : 0;
		}
		next = CLAMP(idx + (p_backward ? -1 : 1), 0, maxi);
		if (use_cubic) {
			pre = CLAMP(idx + (p_backward ? 1 : -1), 0, maxi);
			post = CLAMP(idx + (p_backward ? -2 : 2), 0, maxi);
		}
	}
	else if (loop_mode == LOOP_LINEAR) {
		if (is_start_edge) {
			idx = p_backward ? 0 : maxi;
		}
		next = Math::posmod(idx + (p_backward ? -1 : 1), len);
		if (use_cubic) {
			pre = Math::posmod(idx + (p_backward ? 1 : -1), len);
			post = Math::posmod(idx + (p_backward ? -2 : 2), len);
		}
		if (is_start_edge) {
			if (!p_backward) {
				real_t endtime = (length - p_keys[idx].time);
				if (endtime < 0) { // may be keys past the end
					endtime = 0;
				}
				delta = endtime + p_keys[next].time;
				from = endtime + p_time;
			}
			else {
				real_t endtime = p_keys[idx].time;
				if (endtime > length) { // may be keys past the end
					endtime = length;
				}
				delta = endtime + length - p_keys[next].time;
				from = endtime + length - p_time;
			}
		}
		else if (is_end_edge) {
			if (!p_backward) {
				delta = (length - p_keys[idx].time) + p_keys[next].time;
				from = p_time - p_keys[idx].time;
			}
			else {
				delta = p_keys[idx].time + (length - p_keys[next].time);
				from = (length - p_time) - (length - p_keys[idx].time);
			}
		}
	}
	else {
		if (is_start_edge) {
			idx = p_backward ? len : -1;
		}
		next = (int)Math::round(
			Math::pingpong((float)(idx + (p_backward ? -1 : 1)) + 0.5f, (float)len) - 0.5f);
		if (use_cubic) {
			pre = (int)Math::round(
				Math::pingpong((float)(idx + (p_backward ? 1 : -1)) + 0.5f, (float)len) - 0.5f);
			post = (int)Math::round(
				Math::pingpong((float)(idx + (p_backward ? -2 : 2)) + 0.5f, (float)len) - 0.5f);
		}
		idx = (int)Math::round(Math::pingpong((float)idx + 0.5f, (float)len) - 0.5f);
		if (is_start_edge) {
			if (!p_backward) {
				real_t endtime = p_keys[idx].time;
				if (endtime < 0) { // may be keys past the end
					endtime = 0;
				}
				delta = endtime + p_keys[next].time;
				from = endtime + p_time;
			}
			else {
				real_t endtime = length - p_keys[idx].time;
				if (endtime > length) { // may be keys past the end
					endtime = length;
				}
				delta = endtime + length - p_keys[next].time;
				from = endtime + length - p_time;
			}
		}
		else if (is_end_edge) {
			if (!p_backward) {
				delta = length * 2.0 - p_keys[idx].time - p_keys[next].time;
				from = p_time - p_keys[idx].time;
			}
			else {
				delta = p_keys[idx].time + p_keys[next].time;
				from = (length - p_time) - (length - p_keys[idx].time);
			}
		}
	}

	if (!is_start_edge && !is_end_edge) {
		if (!p_backward) {
			delta = p_keys[next].time - p_keys[idx].time;
			from = p_time - p_keys[idx].time;
		}
		else {
			delta = (length - p_keys[next].time) - (length - p_keys[idx].time);
			from = (length - p_time) - (length - p_keys[idx].time);
		}
	}

	if (Math::is_zero_approx(delta)) {
		c = 0;
	}
	else {
		c = from / delta;
	}

	if (p_ok) {
		*p_ok = true;
	}

	real_t tr = p_keys[idx].transition;
	if (tr == 0) {
		// Don't interpolate if not needed.
		return p_keys[idx].value;
	}

	if (tr != 1.0) {
		c = Math::ease(c, tr);
	}

	switch (p_interp) {
	case INTERPOLATION_NEAREST: {
		return p_keys[idx].value;
	} break;
	default:
		return p_keys[idx].value;
	}
	// do a barrel roll
}

void Animation::value_track_set_update_mode(int p_track, UpdateMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_VALUE);
	ERR_FAIL_INDEX((int)p_mode, 3);

	t->update_mode = p_mode;

	_check_capture_included();
	emit_changed();
}

Animation::UpdateMode Animation::value_track_get_update_mode(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), UPDATE_CONTINUOUS);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_VALUE, UPDATE_CONTINUOUS);

	return t->update_mode;
}

template <typename T>
void Animation::_track_get_key_indices_in_range(const LocalVector<T>& p_array, double from_time,
	double to_time, LocalVector<int>* r_indices, bool p_is_backward) const
{
	int len = p_array.size();
	if (len == 0) {
		return;
	}

	int from = 0;
	int to = len - 1;

	if (!p_is_backward) {
		while (p_array[from].time < from_time ||
			   Math::is_equal_approx(p_array[from].time, from_time)) {
			from++;
			if (to < from) {
				return;
			}
		}
		while (p_array[to].time > to_time && !Math::is_equal_approx(p_array[to].time, to_time)) {
			to--;
			if (to < from) {
				return;
			}
		}
	}
	else {
		while (p_array[from].time < from_time &&
			   !Math::is_equal_approx(p_array[from].time, from_time)) {
			from++;
			if (to < from) {
				return;
			}
		}
		while (p_array[to].time > to_time || Math::is_equal_approx(p_array[to].time, to_time)) {
			to--;
			if (to < from) {
				return;
			}
		}
	}

	if (from == to) {
		r_indices->push_back(from);
		return;
	}

	if (!p_is_backward) {
		for (int i = from; i <= to; i++) {
			r_indices->push_back(i);
		}
	}
	else {
		for (int i = to; i >= from; i--) {
			r_indices->push_back(i);
		}
	}
}

void Animation::add_marker(const String& p_name, double p_time)
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size() &&
		Math::is_equal_approx(p_time, marker_names[idx].time)) {
		marker_times.erase(marker_names[idx].name);
		marker_colors.erase(marker_names[idx].name);
		marker_names[idx].name = p_name;
		marker_times.insert(p_name, p_time);
		marker_colors.insert(p_name, Color(1, 1, 1));
	}
	else {
		_marker_insert(p_time, marker_names, MarkerKey(p_time, p_name));
		marker_times.insert(p_name, p_time);
		marker_colors.insert(p_name, Color(1, 1, 1));
	}
}

void Animation::remove_marker(const String& p_name)
{
	HashMap<String, double>::Iterator E = marker_times.find(p_name);
	ERR_FAIL_COND(!E);
	int idx = _find(marker_names, E->value);
	bool success = (uint32_t)idx < marker_names.size() &&
				   Math::is_equal_approx(marker_names[idx].time, E->value);
	ERR_FAIL_COND(!success);
	marker_names.remove_at(idx);
	marker_times.remove(E);
	marker_colors.erase(p_name);
}

bool Animation::has_marker(const String& p_name) const { return marker_times.has(p_name); }

String Animation::get_marker_at_time(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size() &&
		Math::is_equal_approx(marker_names[idx].time, p_time)) {
		return marker_names[idx].name;
	}

	return String();
}

String Animation::get_next_marker(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if (idx >= -1 && idx < (int)marker_names.size() - 1) {
		// _find ensures that the time at idx is always the closest time to p_time that is also
		// smaller to it. So we add 1 to get the next marker.
		return marker_names[idx + 1].name;
	}
	return String();
}

String Animation::get_prev_marker(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size()) {
		return marker_names[idx].name;
	}
	return String();
}

double Animation::get_marker_time(const String& p_name) const
{
	ERR_FAIL_COND_V(!marker_times.has(p_name), -1);
	return marker_times.get(p_name);
}

// TODO: This needs to be a TypedArray<String> see this PR for rationale
// https://github.com/godotengine/godot/pull/110767/
PackedStringArray Animation::get_marker_names() const
{
	PackedStringArray names;
	// We iterate on marker_names so the result is sorted by time.
	for (const MarkerKey& marker_name : marker_names) {
		names.push_back(marker_name.name);
	}
	return names;
}

Color Animation::get_marker_color(const String& p_name) const
{
	ERR_FAIL_COND_V(!marker_colors.has(p_name), Color());
	return marker_colors[p_name];
}

void Animation::set_marker_color(const String& p_name, const Color& p_color)
{
	marker_colors[p_name] = p_color;
}

String Animation::method_track_get_name(int p_track, int p_key_idx) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), String());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_METHOD, String());

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key_idx, t->methods.size(), String());

	return t->methods[p_key_idx].method;
}

int Animation::bezier_track_insert_key(int p_track, double p_time, real_t p_value,
	const Vector2& p_in_handle, const Vector2& p_out_handle)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, -1);

	TKey<BezierKey> k;
	k.time = p_time;
	k.value.value = p_value;
	k.value.in_handle = p_in_handle;
	if (k.value.in_handle.x > 0) {
		k.value.in_handle.x = 0;
	}
	k.value.out_handle = p_out_handle;
	if (k.value.out_handle.x < 0) {
		k.value.out_handle.x = 0;
	}

	int key = _insert(p_time, t->bezier_values, k);

	emit_changed();

	return key;
}

void Animation::bezier_track_set_key_value(int p_track, int p_index, real_t p_value)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, t->bezier_values.size());

	t->bezier_values[p_index].value.value = p_value;

	emit_changed();
}

void Animation::bezier_track_set_key_in_handle(
	int p_track, int p_index, const Vector2& p_handle, real_t p_balanced_value_time_ratio)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, t->bezier_values.size());

	Vector2 in_handle = p_handle;
	if (in_handle.x > 0) {
		in_handle.x = 0;
	}
	t->bezier_values[p_index].value.in_handle = in_handle;

#ifdef TOOLS_ENABLED
	if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_LINEAR) {
		t->bezier_values[p_index].value.in_handle = Vector2();
		t->bezier_values[p_index].value.out_handle = Vector2();
	}
	else if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_BALANCED) {
		Transform2D xform;
		xform.set_scale(Vector2(1.0, 1.0 / p_balanced_value_time_ratio));

		Vector2 vec_out = xform.xform(t->bezier_values[p_index].value.out_handle);
		Vector2 vec_in = xform.xform(in_handle);

		t->bezier_values[p_index].value.out_handle =
			xform.affine_inverse().xform(-vec_in.normalized() * vec_out.length());
	}
	else if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_MIRRORED) {
		t->bezier_values[p_index].value.out_handle = -in_handle;
	}
#endif // TOOLS_ENABLED

	emit_changed();
}

void Animation::bezier_track_set_key_out_handle(
	int p_track, int p_index, const Vector2& p_handle, real_t p_balanced_value_time_ratio)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, t->bezier_values.size());

	Vector2 out_handle = p_handle;
	if (out_handle.x < 0) {
		out_handle.x = 0;
	}
	t->bezier_values[p_index].value.out_handle = out_handle;

#ifdef TOOLS_ENABLED
	if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_LINEAR) {
		t->bezier_values[p_index].value.in_handle = Vector2();
		t->bezier_values[p_index].value.out_handle = Vector2();
	}
	else if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_BALANCED) {
		Transform2D xform;
		xform.set_scale(Vector2(1.0, 1.0 / p_balanced_value_time_ratio));

		Vector2 vec_in = xform.xform(t->bezier_values[p_index].value.in_handle);
		Vector2 vec_out = xform.xform(out_handle);

		t->bezier_values[p_index].value.in_handle =
			xform.affine_inverse().xform(-vec_out.normalized() * vec_in.length());
	}
	else if (t->bezier_values[p_index].value.handle_mode == HANDLE_MODE_MIRRORED) {
		t->bezier_values[p_index].value.in_handle = -out_handle;
	}
#endif // TOOLS_ENABLED

	emit_changed();
}

real_t Animation::bezier_track_get_key_value(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, 0);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, t->bezier_values.size(), 0);

	return t->bezier_values[p_index].value.value;
}

Vector2 Animation::bezier_track_get_key_in_handle(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Vector2());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, Vector2());

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, t->bezier_values.size(), Vector2());

	return t->bezier_values[p_index].value.in_handle;
}

Vector2 Animation::bezier_track_get_key_out_handle(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Vector2());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, Vector2());

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, t->bezier_values.size(), Vector2());

	return t->bezier_values[p_index].value.out_handle;
}

#ifdef TOOLS_ENABLED
void Animation::bezier_track_set_key_handle_mode(
	int p_track, int p_index, HandleMode p_mode, HandleSetMode p_set_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, t->bezier_values.size());

	t->bezier_values[p_index].value.handle_mode = p_mode;

	if (p_mode != HANDLE_MODE_FREE && p_set_mode != HANDLE_SET_MODE_NONE) {
		Vector2& in_handle = t->bezier_values[p_index].value.in_handle;
		Vector2& out_handle = t->bezier_values[p_index].value.out_handle;
		bezier_track_calculate_handles(
			p_track, p_index, p_mode, p_set_mode, &in_handle, &out_handle);
	}

	emit_changed();
}

Animation::HandleMode Animation::bezier_track_get_key_handle_mode(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), HANDLE_MODE_FREE);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, HANDLE_MODE_FREE);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, t->bezier_values.size(), HANDLE_MODE_FREE);

	return t->bezier_values[p_index].value.handle_mode;
}

bool Animation::bezier_track_calculate_handles(int p_track, int p_index, HandleMode p_mode,
	HandleSetMode p_set_mode, Vector2* r_in_handle, Vector2* r_out_handle)
{
	ERR_FAIL_INDEX_V(p_track, (int)tracks.size(), false);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, false);
	ERR_FAIL_INDEX_V(p_index, (int)t->bezier_values.size(), false);

	int prev_key = MAX(0, p_index - 1);
	int next_key = MIN((int)t->bezier_values.size() - 1, p_index + 1);
	if (prev_key == next_key) {
		return false;
	}

	float time = t->bezier_values[p_index].time;
	float prev_time = t->bezier_values[prev_key].time;
	float prev_value = t->bezier_values[prev_key].value.value;
	float next_time = t->bezier_values[next_key].time;
	float next_value = t->bezier_values[next_key].value.value;

	return bezier_track_calculate_handles(time, prev_time, prev_value, next_time, next_value,
		p_mode, p_set_mode, r_in_handle, r_out_handle);
}

bool Animation::bezier_track_calculate_handles(float p_time, float p_prev_time, float p_prev_value,
	float p_next_time, float p_next_value, HandleMode p_mode, HandleSetMode p_set_mode,
	Vector2* r_in_handle, Vector2* r_out_handle)
{
	ERR_FAIL_COND_V(p_mode == HANDLE_MODE_FREE, false);
	ERR_FAIL_COND_V(p_set_mode == HANDLE_SET_MODE_NONE, false);

	Vector2 in_handle;
	Vector2 out_handle;

	if (p_mode == HANDLE_MODE_LINEAR) {
		in_handle = Vector2(0, 0);
		out_handle = Vector2(0, 0);
	}
	else if (p_mode == HANDLE_MODE_BALANCED) {
		if (p_set_mode == HANDLE_SET_MODE_RESET) {
			real_t handle_length = 1.0 / 3.0;
			in_handle.x = (p_prev_time - p_time) * handle_length;
			in_handle.y = 0;
			out_handle.x = (p_next_time - p_time) * handle_length;
			out_handle.y = 0;
		}
		else if (p_set_mode == HANDLE_SET_MODE_AUTO) {
			real_t handle_length = 1.0 / 6.0;
			real_t tangent = (p_next_value - p_prev_value) / (p_next_time - p_prev_time);
			in_handle.x = (p_prev_time - p_time) * handle_length;
			in_handle.y = in_handle.x * tangent;
			out_handle.x = (p_next_time - p_time) * handle_length;
			out_handle.y = out_handle.x * tangent;
		}
	}
	else if (p_mode == HANDLE_MODE_MIRRORED) {
		real_t handle_length = 1.0 / 4.0;
		real_t prev_interval = Math::abs(p_time - p_prev_time);
		real_t next_interval = Math::abs(p_time - p_next_time);
		real_t min_time = 0;
		if (Math::is_zero_approx(prev_interval)) {
			min_time = next_interval;
		}
		else if (Math::is_zero_approx(next_interval)) {
			min_time = prev_interval;
		}
		else {
			min_time = MIN(prev_interval, next_interval);
		}
		if (p_set_mode == HANDLE_SET_MODE_RESET) {
			in_handle.x = -min_time * handle_length;
			in_handle.y = 0;
			out_handle.x = min_time * handle_length;
			out_handle.y = 0;
		}
		else if (p_set_mode == HANDLE_SET_MODE_AUTO) {
			real_t tangent = (p_next_value - p_prev_value) / min_time;
			in_handle.x = -min_time * handle_length;
			in_handle.y = in_handle.x * tangent;
			out_handle.x = min_time * handle_length;
			out_handle.y = out_handle.x * tangent;
		}
	}

	if (r_in_handle != nullptr) {
		*r_in_handle = in_handle;
	}

	if (r_out_handle != nullptr) {
		*r_out_handle = out_handle;
	}

	return true;
}

#endif // TOOLS_ENABLED

real_t Animation::bezier_track_interpolate(int p_track, double p_time) const
{
	// this uses a different interpolation scheme
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* track = tracks[p_track];
	ERR_FAIL_COND_V(track->type != TYPE_BEZIER, 0);

	int len = _find(track->bezier_values, length) +
			  1; // try to find last key (there may be more past the end)

	if (len <= 0) {
		// (-1 or -2 returned originally) (plus one above)
		return 0;
	}
	else if (len == 1) { // one key found (0+1), return it
		return track->bezier_values[0].value.value;
	}

	int idx = _find(track->bezier_values, p_time);

	ERR_FAIL_COND_V(idx == -2, 0);

	// there really is no looping interpolation on bezier

	if (idx < 0) {
		return track->bezier_values[0].value.value;
	}

	if (idx >= (int)track->bezier_values.size() - 1) {
		return track->bezier_values[track->bezier_values.size() - 1].value.value;
	}

	double t = p_time - track->bezier_values[idx].time;

	int iterations = 10;

	real_t duration = track->bezier_values[idx + 1].time -
					  track->bezier_values[idx].time; // time duration between our two keyframes
	real_t low = 0.0;								  // 0% of the current animation segment
	real_t high = 1.0;								  // 100% of the current animation segment

	Vector2 start(0, track->bezier_values[idx].value.value);
	Vector2 start_out = start + track->bezier_values[idx].value.out_handle;
	Vector2 end(duration, track->bezier_values[idx + 1].value.value);
	Vector2 end_in = end + track->bezier_values[idx + 1].value.in_handle;

	// narrow high and low as much as possible
	for (int i = 0; i < iterations; i++) {
		real_t middle = (low + high) / 2;

		Vector2 interp = start.bezier_interpolate(start_out, end_in, end, middle);

		if (interp.x < t) {
			low = middle;
		}
		else {
			high = middle;
		}
	}

	// interpolate the result:
	Vector2 low_pos = start.bezier_interpolate(start_out, end_in, end, low);
	Vector2 high_pos = start.bezier_interpolate(start_out, end_in, end, high);
	real_t c = (t - low_pos.x) / (high_pos.x - low_pos.x);

	return low_pos.lerp(high_pos, c).y;
}

int Animation::audio_track_insert_key(int p_track, double p_time, const Ref<Resource>& p_stream,
	real_t p_start_offset, real_t p_end_offset)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, -1);

	TKey<AudioKey> k;
	k.time = p_time;
	k.value.stream = p_stream;
	k.value.start_offset = p_start_offset;
	if (k.value.start_offset < 0) {
		k.value.start_offset = 0;
	}
	k.value.end_offset = p_end_offset;
	if (k.value.end_offset < 0) {
		k.value.end_offset = 0;
	}

	int key = _insert(p_time, t->audio_values, k);

	emit_changed();

	return key;
}

void Animation::audio_track_set_key_stream(int p_track, int p_key, const Ref<Resource>& p_stream)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, t->audio_values.size());

	t->audio_values[p_key].value.stream = p_stream;

	emit_changed();
}

void Animation::audio_track_set_key_start_offset(int p_track, int p_key, real_t p_offset)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, t->audio_values.size());

	if (p_offset < 0) {
		p_offset = 0;
	}

	t->audio_values[p_key].value.start_offset = p_offset;

	emit_changed();
}

void Animation::audio_track_set_key_end_offset(int p_track, int p_key, real_t p_offset)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, t->audio_values.size());

	if (p_offset < 0) {
		p_offset = 0;
	}

	t->audio_values[p_key].value.end_offset = p_offset;

	emit_changed();
}

Ref<Resource> Animation::audio_track_get_key_stream(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Ref<Resource>());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, Ref<Resource>());

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->audio_values.size(), Ref<Resource>());

	return t->audio_values[p_key].value.stream;
}

real_t Animation::audio_track_get_key_start_offset(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, 0);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->audio_values.size(), 0);

	return t->audio_values[p_key].value.start_offset;
}

real_t Animation::audio_track_get_key_end_offset(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, 0);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->audio_values.size(), 0);

	return t->audio_values[p_key].value.end_offset;
}

void Animation::audio_track_set_use_blend(int p_track, bool p_enable)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	t->use_blend = p_enable;
	emit_changed();
}

bool Animation::audio_track_is_use_blend(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, false);

	return t->use_blend;
}

int Animation::animation_track_insert_key(int p_track, double p_time, const String& p_animation)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ANIMATION, -1);

	TKey<String> k;
	k.time = p_time;
	k.value = p_animation;

	int key = _insert(p_time, t->animation_values, k);

	emit_changed();

	return key;
}

void Animation::animation_track_set_key_animation(
	int p_track, int p_key, const String& p_animation)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_ANIMATION);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, t->animation_values.size());

	t->animation_values[p_key].value = p_animation;

	emit_changed();
}

String Animation::animation_track_get_key_animation(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), String());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ANIMATION, String());

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, t->animation_values.size(), String());

	return t->animation_values[p_key].value;
}

void Animation::set_length(double p_length)
{
	if (p_length < ANIM_MIN_LENGTH) {
		p_length = ANIM_MIN_LENGTH;
	}
	length = p_length;
	emit_changed();
}

void Animation::set_loop_mode(Animation::LoopMode p_loop_mode)
{
	loop_mode = p_loop_mode;
	emit_changed();
}

void Animation::track_set_imported(int p_track, bool p_imported)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->imported = p_imported;
}

bool Animation::track_is_imported(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->imported;
}

void Animation::track_set_enabled(int p_track, bool p_enabled)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->enabled = p_enabled;
	emit_changed();
}

bool Animation::track_is_enabled(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->enabled;
}

void Animation::track_move_up(int p_track)
{
	if (p_track < ((int)tracks.size() - 1)) {
		SWAP(tracks[p_track], tracks[p_track + 1]);
	}

	emit_changed();
}

void Animation::track_move_down(int p_track)
{
	if ((uint32_t)p_track < tracks.size()) {
		SWAP(tracks[p_track], tracks[p_track - 1]);
	}

	emit_changed();
}

void Animation::track_move_to(int p_track, int p_to_index)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_to_index, tracks.size() + 1);
	if (p_track == p_to_index || p_track == p_to_index - 1) {
		return;
	}

	Track* track = tracks[p_track];
	tracks.remove_at(p_track);
	// Take into account that the position of the tracks that come after the one removed will
	// change.
	tracks.insert(p_to_index > p_track ? p_to_index - 1 : p_to_index, track);

	emit_changed();
}

void Animation::track_swap(int p_track, int p_with_track)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_with_track, tracks.size());
	if (p_track == p_with_track) {
		return;
	}
	SWAP(tracks[p_track], tracks[p_with_track]);

	emit_changed();
}

void Animation::set_step(real_t p_step)
{
	step = p_step;
	emit_changed();
}

real_t Animation::get_step() const { return step; }

void Animation::clear()
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		memdelete(tracks[i]);
	}
	tracks.clear();
	loop_mode = LOOP_NONE;
	length = 1;
	compression.enabled = false;
	compression.bounds.clear();
	compression.pages.clear();
	compression.fps = 120;
	emit_changed();
}

bool Animation::_float_track_optimize_key(const TKey<float> t0, const TKey<float> t1,
	const TKey<float> t2, real_t p_allowed_velocity_err, real_t p_allowed_precision_error,
	bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if (std::abs(t0.value - t1.value) < p_allowed_precision_error &&
		std::abs(t1.value - t2.value) < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Calc velocities.
	double v0 = (t1.value - t0.value) / (t1.time - t0.time);
	double v1 = (t2.value - t1.value) / (t2.time - t1.time);
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	if (!std::signbit(v0 * v1)) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_vector2_track_optimize_key(const TKey<Vector2> t0, const TKey<Vector2> t1,
	const TKey<Vector2> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Calc velocities.
	Vector2 vc0 = (t1.value - t0.value) / (t1.time - t0.time);
	Vector2 vc1 = (t2.value - t1.value) / (t2.time - t1.time);
	double v0 = vc0.length();
	double v1 = vc1.length();
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	// Check axis.
	if (vc0.normalized().dot(vc1.normalized()) >= 1.0 - p_allowed_angular_error * 2.0) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_vector3_track_optimize_key(const TKey<Vector3> t0, const TKey<Vector3> t1,
	const TKey<Vector3> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}

	// Calc velocities.
	Vector3 vc0 = (t1.value - t0.value) / (t1.time - t0.time);
	Vector3 vc1 = (t2.value - t1.value) / (t2.time - t1.time);
	double v0 = vc0.length();
	double v1 = vc1.length();
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	// Check axis.
	if (vc0.normalized().dot(vc1.normalized()) >= 1.0 - p_allowed_angular_error * 2.0) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_quaternion_track_optimize_key(const TKey<Quaternion> t0, const TKey<Quaternion> t1,
	const TKey<Quaternion> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Check axis.
	Quaternion q0 = t0.value * t1.value * t0.value.inverse();
	Quaternion q1 = t1.value * t2.value * t1.value.inverse();
	if (q0.get_axis().dot(q1.get_axis()) >= 1.0 - p_allowed_angular_error * 2.0) {
		double a0 = Math::acos(t0.value.dot(t1.value));
		double a1 = Math::acos(t1.value.dot(t2.value));
		if (a0 + a1 >= Math::PI / 2) {
			return false; // Rotation is more than 180 deg, keep key.
		}
		// Calc velocities.
		double v0 = a0 / (t1.time - t0.time);
		double v1 = a1 / (t2.time - t1.time);
		// Avoid zero div but check equality.
		if (std::abs(v0 - v1) < p_allowed_precision_error) {
			return true;
		}
		else if (std::abs(v0) < p_allowed_precision_error ||
				   std::abs(v1) < p_allowed_precision_error) {
			return false;
		}
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

void Animation::_position_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_POSITION_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	Track* tt = tracks[p_idx];
	int i = 0;
	while (i < (int)tt->positions.size() - 2) {
		TKey<Vector3> t0 = tt->positions[i];
		TKey<Vector3> t1 = tt->positions[i + 1];
		TKey<Vector3> t2 = tt->positions[i + 2];
		bool erase = _vector3_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			tt->positions.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (tt->positions.size() == 2) {
		if ((tt->positions[0].value - tt->positions[1].value).length() <
			p_allowed_precision_error) {
			tt->positions.remove_at(1);
		}
	}
}

void Animation::_rotation_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_ROTATION_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	Track* rt = tracks[p_idx];
	int i = 0;
	while (i < (int)rt->rotations.size() - 2) {
		TKey<Quaternion> t0 = rt->rotations[i];
		TKey<Quaternion> t1 = rt->rotations[i + 1];
		TKey<Quaternion> t2 = rt->rotations[i + 2];
		bool erase = _quaternion_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			rt->rotations.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (rt->rotations.size() == 2) {
		if ((rt->rotations[0].value - rt->rotations[1].value).length() <
			p_allowed_precision_error) {
			rt->rotations.remove_at(1);
		}
	}
}

void Animation::_scale_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_SCALE_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	Track* st = tracks[p_idx];
	int i = 0;
	while (i < (int)st->scales.size() - 2) {
		TKey<Vector3> t0 = st->scales[i];
		TKey<Vector3> t1 = st->scales[i + 1];
		TKey<Vector3> t2 = st->scales[i + 2];
		bool erase = _vector3_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			st->scales.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (st->scales.size() == 2) {
		if ((st->scales[0].value - st->scales[1].value).length() < p_allowed_precision_error) {
			st->scales.remove_at(1);
		}
	}
}

void Animation::_blend_shape_track_optimize(
	int p_idx, real_t p_allowed_velocity_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_BLEND_SHAPE);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	Track* bst = tracks[p_idx];
	int i = 0;
	while (i < (int)bst->blend_shapes.size() - 2) {
		TKey<float> t0 = bst->blend_shapes[i];
		TKey<float> t1 = bst->blend_shapes[i + 1];
		TKey<float> t2 = bst->blend_shapes[i + 2];

		bool erase = _float_track_optimize_key(
			t0, t1, t2, p_allowed_velocity_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			bst->blend_shapes.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (bst->blend_shapes.size() == 2) {
		if (std::abs(bst->blend_shapes[0].value - bst->blend_shapes[1].value) <
			p_allowed_precision_error) {
			bst->blend_shapes.remove_at(1);
		}
	}
}

void Animation::optimize(
	real_t p_allowed_velocity_err, real_t p_allowed_angular_err, int p_precision)
{
	real_t precision = Math::pow(0.1, p_precision);
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (track_is_compressed(i)) {
			continue; // not possible to optimize compressed track
		}
		if (tracks[i]->type == TYPE_POSITION_3D) {
			_position_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_ROTATION_3D) {
			_rotation_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_SCALE_3D) {
			_scale_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_BLEND_SHAPE) {
			_blend_shape_track_optimize(i, p_allowed_velocity_err, precision);
		}
		else if (tracks[i]->type == TYPE_VALUE) {
			_value_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
	}
}

#define print_animc(m_str)

// #define print_animc(m_str) print_line(m_str);

struct AnimationCompressionDataState
{
	enum
	{
		MIN_OPTIMIZE_PACKETS = 5,
		MAX_PACKETS = 16
	};

	uint32_t components = 3;
	LocalVector<uint8_t> data; // Committed packets.

	struct PacketData
	{
		int32_t data[3] = {0, 0, 0};
		uint32_t frame = 0;
	};

	float split_tolerance = 1.5;

	LocalVector<PacketData> temp_packets;

	// used for rollback if the new frame does not fit
	int32_t validated_packet_count = -1;

	static int32_t _compute_delta16_signed(int32_t p_from, int32_t p_to)
	{
		int32_t delta = p_to - p_from;
		if (delta > 32767) {
			return delta - 65536; // use wrap around
		}
		else if (delta < -32768) {
			return 65536 + delta; // use wrap around
		}
		return delta;
	}

	static uint32_t _compute_shift_bits_signed(int32_t p_delta)
	{
		if (p_delta == 0) {
			return 0;
		}
		else if (p_delta < 0) {
			p_delta = Math::abs(p_delta) - 1;
			if (p_delta == 0) {
				return 1;
			}
		}
		return Math::nearest_shift((uint32_t)p_delta);
	}

	void _compute_max_shifts(
		uint32_t p_from, uint32_t p_to, uint32_t* max_shifts, uint32_t& max_frame_delta_shift) const
	{
		for (uint32_t j = 0; j < components; j++) {
			max_shifts[j] = 0;
		}
		max_frame_delta_shift = 0;

		for (uint32_t i = p_from + 1; i <= p_to; i++) {
			int32_t frame_delta = temp_packets[i].frame - temp_packets[i - 1].frame;
			max_frame_delta_shift =
				MAX(max_frame_delta_shift, Math::nearest_shift((uint32_t)frame_delta));
			for (uint32_t j = 0; j < components; j++) {
				int32_t diff =
					_compute_delta16_signed(temp_packets[i - 1].data[j], temp_packets[i].data[j]);
				uint32_t shift = _compute_shift_bits_signed(diff);
				max_shifts[j] = MAX(shift, max_shifts[j]);
			}
		}
	}

	bool insert_key(uint32_t p_frame, const Vector3i& p_key)
	{
		if (temp_packets.size() == MAX_PACKETS) {
			commit_temp_packets();
		}
		PacketData packet;
		packet.frame = p_frame;
		for (int i = 0; i < 3; i++) {
			ERR_FAIL_COND_V(p_key[i] > 65535, false); // Safety checks.
			packet.data[i] = p_key[i];
		}

		temp_packets.push_back(packet);

		if (temp_packets.size() >= MIN_OPTIMIZE_PACKETS) {
			uint32_t max_shifts[3] = {0, 0, 0}; // Base sizes, 16 bit
			uint32_t max_frame_delta_shift = 0;
			// Compute the average shift before the packet was added
			_compute_max_shifts(0, temp_packets.size() - 2, max_shifts, max_frame_delta_shift);

			float prev_packet_size_avg = 0;
			prev_packet_size_avg = float(1 << max_frame_delta_shift);
			for (uint32_t i = 0; i < components; i++) {
				prev_packet_size_avg += float(1 << max_shifts[i]);
			}
			prev_packet_size_avg /= float(1 + components);

			_compute_max_shifts(temp_packets.size() - 2, temp_packets.size() - 1, max_shifts,
				max_frame_delta_shift);

			float new_packet_size_avg = 0;
			new_packet_size_avg = float(1 << max_frame_delta_shift);
			for (uint32_t i = 0; i < components; i++) {
				new_packet_size_avg += float(1 << max_shifts[i]);
			}
			new_packet_size_avg /= float(1 + components);

			print_animc("packet count: " + rtos(temp_packets.size() - 1) + " size avg " +
						rtos(prev_packet_size_avg) + " new avg " + rtos(new_packet_size_avg));
			float ratio = (prev_packet_size_avg < new_packet_size_avg)
							  ? (new_packet_size_avg / prev_packet_size_avg)
							  : (prev_packet_size_avg / new_packet_size_avg);

			if (ratio > split_tolerance) {
				print_animc("split!");
				temp_packets.resize(temp_packets.size() - 1);
				commit_temp_packets();
				temp_packets.push_back(packet);
			}
		}

		return temp_packets.size() == 1; // First key
	}

	uint32_t get_temp_packet_size() const
	{
		if (temp_packets.is_empty()) {
			return 0;
		}
		else if (temp_packets.size() == 1) {
			return components == 1 ? 4 : 8; // 1 component packet is 16 bits and 16 bits unused. 3
											// component packets is 48 bits and 16 bits unused
		}
		uint32_t max_shifts[3] = {0, 0, 0}; // base sizes, 16 bit
		uint32_t max_frame_delta_shift = 0;

		_compute_max_shifts(0, temp_packets.size() - 1, max_shifts, max_frame_delta_shift);

		uint32_t size_bits = 16; // base value (all 4 bits of shift sizes for x,y,z,time)
		size_bits += max_frame_delta_shift * (temp_packets.size() - 1); // times
		for (uint32_t j = 0; j < components; j++) {
			size_bits += 16; // base value
			uint32_t shift = max_shifts[j];
			if (shift > 0) {
				shift += 1; // if not zero, add sign bit
			}
			size_bits += shift * (temp_packets.size() - 1);
		}
		if (size_bits % 8 != 0) { // wrap to 8 bits
			size_bits += 8 - (size_bits % 8);
		}
		uint32_t size_bytes = size_bits / 8; // wrap to words
		if (size_bytes % 4 != 0) {
			size_bytes += 4 -
(size_bytes % 4);
		}
		return size_bytes;
	}

	static void _push_bits(LocalVector<uint8_t>& data, uint32_t& r_buffer, uint32_t& r_bits_used,
		uint32_t p_value, uint32_t p_bits)
	{
		r_buffer |= p_value << r_bits_used;
		r_bits_used += p_bits;
		while (r_bits_used >= 8) {
			uint8_t byte = r_buffer & 0xFF;
			data.push_back(byte);
			r_buffer >>= 8;
			r_bits_used -= 8;
		}
	}

	void commit_temp_packets()
	{
		if (temp_packets.is_empty()) {
			return; // Nothing to do.
		}
// #define DEBUG_PACKET_PUSH
#ifdef DEBUG_PACKET_PUSH
#ifndef _MSC_VER
#warning Debugging packet push, disable this code in production to gain a bit more import performance.
#endif
		uint32_t debug_packet_push = get_temp_packet_size();
		uint32_t debug_data_size = data.size();
#endif
		// Store header

		uint8_t header[8];
		uint32_t header_bytes = 0;
		for (uint32_t i = 0; i < components; i++) {
			encode_uint16(temp_packets[0].data[i], &header[header_bytes]);
			header_bytes += 2;
		}

		uint32_t max_shifts[3] = {0, 0, 0}; // base sizes, 16 bit
		uint32_t max_frame_delta_shift = 0;

		if (temp_packets.size() > 1) {
			_compute_max_shifts(0, temp_packets.size() - 1, max_shifts, max_frame_delta_shift);
			uint16_t shift_header = (max_frame_delta_shift - 1) << 12;
			for (uint32_t i = 0; i < components; i++) {
				shift_header |= max_shifts[i] << (4 * i);
			}

			encode_uint16(shift_header, &header[header_bytes]);
			header_bytes += 2;
		}

		while (header_bytes < 8 &&
			   header_bytes % 4 != 0) { // First cond needed to silence wrong GCC warning.
			header[header_bytes++] = 0;
		}

		for (uint32_t i = 0; i < header_bytes; i++) {
			data.push_back(header[i]);
		}

		if (temp_packets.size() == 1) {
			temp_packets.clear();
			validated_packet_count = 0;
			return; // only header stored, nothing else to do
		}

		uint32_t bit_buffer = 0;
		uint32_t bits_used = 0;

		for (uint32_t i = 1; i < temp_packets.size(); i++) {
			uint32_t frame_delta = temp_packets[i].frame - temp_packets[i - 1].frame;
			_push_bits(data, bit_buffer, bits_used, frame_delta, max_frame_delta_shift);

			for (uint32_t j = 0; j < components; j++) {
				if (max_shifts[j] == 0) {
					continue; // Zero delta, do not store
				}
				int32_t delta =
					_compute_delta16_signed(temp_packets[i - 1].data[j], temp_packets[i].data[j]);

				ERR_FAIL_COND(delta < -32768 || delta > 32767); // Safety check.

				uint16_t deltau;
				if (delta < 0) {
					deltau = (Math::abs(delta) - 1) | (1 << max_shifts[j]);
				}
				else {
					deltau = delta;
				}
				_push_bits(
					data, bit_buffer, bits_used, deltau, max_shifts[j] + 1); // Include sign bit
			}
		}
		if (bits_used != 0) {
			ERR_FAIL_COND(bit_buffer > 0xFF); // Safety check.
			data.push_back(bit_buffer);
		}

		while (data.size() % 4 != 0) {
			data.push_back(0); // pad to align with 4
		}

		temp_packets.clear();
		validated_packet_count = 0;

#ifdef DEBUG_PACKET_PUSH
		ERR_FAIL_COND((data.size() - debug_data_size) != debug_packet_push);
#endif
	}
};

struct AnimationCompressionTimeState
{
	struct Packet
	{
		uint32_t frame;
		uint32_t offset;
		uint32_t count;
	};

	LocalVector<Packet> packets;
	// used for rollback
	int32_t key_index = 0;
	int32_t validated_packet_count = 0;
	int32_t validated_key_index = -1;
	bool needs_start_frame = false;
};

Vector3i Animation::_compress_key(
	uint32_t p_track, const AABB& p_bounds, int32_t p_key, float p_time)
{
	Vector3i values;
	TrackType tt = track_get_type(p_track);
	switch (tt) {
	case TYPE_POSITION_3D: {
		Vector3 pos;
		if (p_key >= 0) {
			position_track_get_key(p_track, p_key, &pos);
		}
		else {
			try_position_track_interpolate(p_track, p_time, &pos);
		}
		pos = (pos - p_bounds.position) / p_bounds.size;
		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(pos[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_ROTATION_3D: {
		Quaternion rot;
		if (p_key >= 0) {
			rotation_track_get_key(p_track, p_key, &rot);
		}
		else {
			try_rotation_track_interpolate(p_track, p_time, &rot);
		}
		Vector3 axis = rot.get_axis();
		float angle = rot.get_angle();
		angle = Math::fposmod(double(angle), double(Math::PI * 2.0));
		Vector2 oct = axis.octahedron_encode();
		Vector3 rot_norm(
			oct.x, oct.y, angle / (Math::PI * 2.0)); // high resolution rotation in 0-1 angle.

		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(rot_norm[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_SCALE_3D: {
		Vector3 scale;
		if (p_key >= 0) {
			scale_track_get_key(p_track, p_key, &scale);
		}
		else {
			try_scale_track_interpolate(p_track, p_time, &scale);
		}
		scale = (scale - p_bounds.position) / p_bounds.size;
		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(scale[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_BLEND_SHAPE: {
		float blend;
		if (p_key >= 0) {
			blend_shape_track_get_key(p_track, p_key, &blend);
		}
		else {
			try_blend_shape_track_interpolate(p_track, p_time, &blend);
		}

		blend = (blend / float(Compression::BLEND_SHAPE_RANGE)) * 0.5 + 0.5;
		values[0] = CLAMP(int32_t(blend * 65535.0), 0, 65535);
	} break;
	default: {
		ERR_FAIL_V(Vector3i()); // Safety check.
	} break;
	}

	return values;
}

struct AnimationCompressionBufferBitsRead
{
	uint32_t buffer = 0;
	uint32_t used = 0;
	const uint8_t* src_data = nullptr;

	_FORCE_INLINE_ uint32_t read(uint32_t p_bits)
	{
		uint32_t output = 0;
		uint32_t written = 0;
		while (p_bits > 0) {
			if (used == 0) {
				used = 8;
				buffer = *src_data;
				src_data++;
			}
			uint32_t to_write = MIN(used, p_bits);
			output |= (buffer & ((1 << to_write) - 1)) << written;
			buffer >>= to_write;
			used -= to_write;
			p_bits -= to_write;
			written += to_write;
		}
		return output;
	}
};

bool Animation::_rotation_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, Quaternion& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<3>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_quaternion(current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_quaternion(next);
	}
	else {
		double c = (p_time - time_current) / (time_next - time_current);
		Quaternion from = _uncompress_quaternion(current);
		Quaternion to = _uncompress_quaternion(next);
		r_ret = from.slerp(to, c);
	}

	return true;
}

bool Animation::_pos_scale_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, Vector3& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<3>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_pos_scale(p_compressed_track, current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_pos_scale(p_compressed_track, next);
	}
	else {
		double c = (p_time - time_current) / (time_next - time_current);
		Vector3 from = _uncompress_pos_scale(p_compressed_track, current);
		Vector3 to = _uncompress_pos_scale(p_compressed_track, next);
		r_ret = from.lerp(to, c);
	}

	return true;
}

bool Animation::_blend_shape_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, float& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<1>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_blend_shape(current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_blend_shape(next);
	}
	else {
		float c = (p_time - time_current) / (time_next - time_current);
		float from = _uncompress_blend_shape(current);
		float to = _uncompress_blend_shape(next);
		r_ret = Math::lerp(from, to, c);
	}

	return true;
}

template <uint32_t COMPONENTS>
bool Animation::_fetch_compressed(uint32_t p_compressed_track, double p_time,
	Vector3i& r_current_value, double& r_current_time, Vector3i& r_next_value, double& r_next_time,
	uint32_t* key_index) const
{
	ERR_FAIL_COND_V(!compression.enabled, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), false);
	p_time = CLAMP(p_time, 0, length);
	if (key_index) {
		*key_index = 0;
	}

	double frame_to_sec = 1.0 / double(compression.fps);

	int32_t page_index = -1;
	for (uint32_t i = 0; i < compression.pages.size(); i++) {
		if (compression.pages[i].time_offset > p_time) {
			break;
		}
		page_index = i;
	}

	ERR_FAIL_COND_V(page_index == -1, false); // should not happen

	double page_base_time = compression.pages[page_index].time_offset;
	const uint8_t* page_data = compression.pages[page_index].data.ptr();
	// Little endian assumed. No major big endian hardware exists any longer, but in case it does it
	// will need to be supported.
	const uint32_t* indices = (const uint32_t*)page_data;
	const uint16_t* time_keys = (const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
	uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

	int32_t packet_idx = 0;
	double packet_time = double(time_keys[0]) * frame_to_sec + page_base_time;
	uint32_t base_frame = time_keys[0];

	for (uint32_t i = 1; i < time_key_count; i++) {
		uint32_t f = time_keys[i * 2 + 0];
		double frame_time = double(f) * frame_to_sec + page_base_time;

		if (frame_time > p_time) {
			break;
		}

		if (key_index) {
			(*key_index) += (time_keys[(i - 1) * 2 + 1] >> 12) + 1;
		}

		packet_idx = i;
		packet_time = frame_time;
		base_frame = f;
	}

	const uint8_t* data_keys_base = (const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

	uint16_t time_key_data = time_keys[packet_idx * 2 + 1];
	uint32_t data_offset = (time_key_data & 0xFFF) * 4; // lower 12 bits
	uint32_t data_count = (time_key_data >> 12) + 1;

	const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

	uint16_t decode[COMPONENTS];
	uint16_t decode_next[COMPONENTS];

	for (uint32_t i = 0; i < COMPONENTS; i++) {
		decode[i] = data_key[i];
		decode_next[i] = data_key[i];
	}

	double next_time = packet_time;

	if (p_time > packet_time) { // If its equal or less, then don't bother
		if (data_count > 1) {
			// decode forward
			uint32_t bit_width[COMPONENTS];
			for (uint32_t i = 0; i < COMPONENTS; i++) {
				bit_width[i] = (data_key[COMPONENTS] >> (i * 4)) & 0xF;
			}

			uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

			AnimationCompressionBufferBitsRead buffer;

			buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

			for (uint32_t i = 1; i < data_count; i++) {
				uint32_t frame_delta = buffer.read(frame_bit_width);
				base_frame += frame_delta;

				for (uint32_t j = 0; j < COMPONENTS; j++) {
					if (bit_width[j] == 0) {
						continue; // do none
					}
					uint32_t valueu = buffer.read(bit_width[j] + 1);
					bool sign = valueu & (1 << bit_width[j]);
					int16_t value = valueu & ((1 << bit_width[j]) - 1);
					if (sign) {
						value = -value - 1;
					}

					decode_next[j] += value;
				}

				next_time = double(base_frame) * frame_to_sec + page_base_time;
				if (p_time < next_time) {
					break;
				}

				packet_time = next_time;

				for (uint32_t j = 0; j < COMPONENTS; j++) {
					decode[j] = decode_next[j];
				}

				if (key_index) {
					(*key_index)++;
				}
			}
		}

		if (p_time > next_time) { // > instead of >= because if its equal, then it will be properly
								  // interpolated anyway
			// So, the last frame found still has a time that is less than the required frame,
			// will have to interpolate with the first frame of the next timekey.

			if ((uint32_t)packet_idx <
				time_key_count - 1) { // Safety check but should not matter much, otherwise current
									  // next packet is last packet.

				uint16_t time_key_data_next = time_keys[(packet_idx + 1) * 2 + 1];
				uint32_t data_offset_next = (time_key_data_next & 0xFFF) * 4; // Lower 12 bits

				const uint16_t* data_key_next =
					(const uint16_t*)(data_keys_base + data_offset_next);
				base_frame = time_keys[(packet_idx + 1) * 2 + 0];
				next_time = double(base_frame) * frame_to_sec + page_base_time;
				for (uint32_t i = 0; i < COMPONENTS; i++) {
					decode_next[i] = data_key_next[i];
				}
			}
		}
	}

	r_current_time = packet_time;
	r_next_time = next_time;

	for (uint32_t i = 0; i < COMPONENTS; i++) {
		r_current_value[i] = decode[i];
		r_next_value[i] = decode_next[i];
	}

	return true;
}

template <uint32_t COMPONENTS>
void Animation::_get_compressed_key_indices_in_range(
	uint32_t p_compressed_track, double p_time, double p_delta, LocalVector<int>* r_indices) const
{
	ERR_FAIL_COND(!compression.enabled);
	ERR_FAIL_UNSIGNED_INDEX(p_compressed_track, compression.bounds.size());

	double frame_to_sec = 1.0 / double(compression.fps);
	uint32_t key_index = 0;

	for (uint32_t p = 0; p < compression.pages.size(); p++) {
		if (compression.pages[p].time_offset >= p_time + p_delta) {
			// Page beyond range
			return;
		}

		// Page within range

		uint32_t page_index = p;

		double page_base_time = compression.pages[page_index].time_offset;
		const uint8_t* page_data = compression.pages[page_index].data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

		for (uint32_t i = 0; i < time_key_count; i++) {
			uint32_t f = time_keys[i * 2 + 0];
			double frame_time = f * frame_to_sec + page_base_time;
			if (frame_time >= p_time + p_delta) {
				return;
			}
			else if (frame_time >= p_time) {
				r_indices->push_back(key_index);
			}

			key_index++;

			const uint8_t* data_keys_base =
				(const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

			uint16_t time_key_data = time_keys[i * 2 + 1];
			uint32_t data_offset = (time_key_data & 0xFFF) * 4; // lower 12 bits
			uint32_t data_count = (time_key_data >> 12) + 1;

			const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

			if (data_count > 1) {
				// decode forward
				uint32_t bit_width[COMPONENTS];
				for (uint32_t j = 0; j < COMPONENTS; j++) {
					bit_width[j] = (data_key[COMPONENTS] >> (j * 4)) & 0xF;
				}

				uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

				AnimationCompressionBufferBitsRead buffer;

				buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

				for (uint32_t j = 1; j < data_count; j++) {
					uint32_t frame_delta = buffer.read(frame_bit_width);
					f += frame_delta;

					frame_time = f * frame_to_sec + page_base_time;
					if (frame_time >= p_time + p_delta) {
						return;
					}
					else if (frame_time >= p_time) {
						r_indices->push_back(key_index);
					}

					for (uint32_t k = 0; k < COMPONENTS; k++) {
						if (bit_width[k] == 0) {
							continue; // do none
						}
						buffer.read(bit_width[k] + 1); // skip
					}

					key_index++;
				}
			}
		}
	}
}

int Animation::_get_compressed_key_count(uint32_t p_compressed_track) const
{
	ERR_FAIL_COND_V(!compression.enabled, -1);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), -1);

	int key_count = 0;

	for (const Compression::Page& page : compression.pages) {
		const uint8_t* page_data = page.data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

		for (uint32_t j = 0; j < time_key_count; j++) {
			key_count += (time_keys[j * 2 + 1] >> 12) + 1;
		}
	}

	return key_count;
}

Quaternion Animation::_uncompress_quaternion(const Vector3i& p_value) const
{
	Vector3 axis =
		Vector3::octahedron_decode(Vector2(float(p_value.x) / 65535.0, float(p_value.y) / 65535.0));
	float angle = (float(p_value.z) / 65535.0) * 2.0 * Math::PI;
	return Quaternion(axis, angle);
}

Vector3 Animation::_uncompress_pos_scale(uint32_t p_compressed_track, const Vector3i& p_value) const
{
	Vector3 pos_norm(
		float(p_value.x) / 65535.0, float(p_value.y) / 65535.0, float(p_value.z) / 65535.0);
	return compression.bounds[p_compressed_track].position +
		   pos_norm * compression.bounds[p_compressed_track].size;
}

float Animation::_uncompress_blend_shape(const Vector3i& p_value) const
{
	float bsn = float(p_value.x) / 65535.0;
	return (bsn * 2.0 - 1.0) * float(Compression::BLEND_SHAPE_RANGE);
}

template <uint32_t COMPONENTS>
bool Animation::_fetch_compressed_by_index(
	uint32_t p_compressed_track, int p_index, Vector3i& r_value, double& r_time) const
{
	ERR_FAIL_COND_V(!compression.enabled, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), false);

	for (const Compression::Page& page : compression.pages) {
		const uint8_t* page_data = page.data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];
		const uint8_t* data_keys_base =
			(const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

		for (uint32_t j = 0; j < time_key_count; j++) {
			uint32_t subkeys = (time_keys[j * 2 + 1] >> 12) + 1;
			if ((uint32_t)p_index < subkeys) {
				uint16_t data_offset = (time_keys[j * 2 + 1] & 0xFFF) * 4;

				const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

				uint16_t frame = time_keys[j * 2 + 0];
				uint16_t decode[COMPONENTS];

				for (uint32_t k = 0; k < COMPONENTS; k++) {
					decode[k] = data_key[k];
				}

				if (p_index > 0) {
					uint32_t bit_width[COMPONENTS];
					for (uint32_t k = 0; k < COMPONENTS; k++) {
						bit_width[k] = (data_key[COMPONENTS] >> (k * 4)) & 0xF;
					}
					uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

					AnimationCompressionBufferBitsRead buffer;
					buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

					for (int k = 0; k < p_index; k++) {
						uint32_t frame_delta = buffer.read(frame_bit_width);
						frame += frame_delta;
						for (uint32_t l = 0; l < COMPONENTS; l++) {
							if (bit_width[l] == 0) {
								continue; // do none
							}
							uint32_t valueu = buffer.read(bit_width[l] + 1);
							bool sign = valueu & (1 << bit_width[l]);
							int16_t value = valueu & ((1 << bit_width[l]) - 1);
							if (sign) {
								value = -value - 1;
							}

							decode[l] += value;
						}
					}
				}

				r_time = page.time_offset + double(frame) / double(compression.fps);
				for (uint32_t l = 0; l < COMPONENTS; l++) {
					r_value[l] = decode[l];
				}

				return true;

			}
			else {
				p_index -= subkeys;
			}
		}
	}

	return false;
}

// Helper functions for Rotation.
double Animation::interpolate_via_rest(double p_from, double p_to, double p_weight, double p_rest)
{
	double rot_a = Math::fposmod(p_from, Math::TAU);
	double rot_b = Math::fposmod(p_to, Math::TAU);
	double rot_rest = Math::fposmod(p_rest, Math::TAU);
	if (rot_rest < Math::PI) {
		rot_a = rot_a > rot_rest + Math::PI ? rot_a - Math::TAU : rot_a;
		rot_b = rot_b > rot_rest + Math::PI ? rot_b - Math::TAU : rot_b;
	}
	else {
		rot_a = rot_a < rot_rest - Math::PI ? rot_a + Math::TAU : rot_a;
		rot_b = rot_b < rot_rest - Math::PI ? rot_b + Math::TAU : rot_b;
	}
	return Math::fposmod(rot_a + (rot_b - rot_rest) * p_weight, Math::TAU);
}

Quaternion Animation::interpolate_via_rest(
	const Quaternion& p_from, const Quaternion& p_to, real_t p_weight, const Quaternion& p_rest)
{
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(
		!p_from.is_normalized(), Quaternion(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(
		!p_to.is_normalized(), Quaternion(), "The end quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(
		!p_rest.is_normalized(), Quaternion(), "The rest quaternion must be normalized.");
#endif
	return (p_from * Quaternion().slerp(p_rest.inverse() * p_to, p_weight)).normalized();
}

bool Animation::inform_variant_array(int& r_min, int& r_max)
{
	if (r_min <= r_max) {
		return false;
	}
	SWAP(r_min, r_max);
	return true;
}

Animation::~Animation()
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		memdelete(tracks[i]);
	}
}

double Animation::track_get_key_time(int p_track, int p_key_idx) const { return 0.0; }

void Animation::track_remove_key(int p_track, int p_idx) {}

int Animation::track_find_key(
	int p_track, double p_time, FindMode p_find_mode, bool p_limit, bool p_backward) const
{
	return 0;
}

void Animation::_value_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
}

void Animation::compress(uint32_t p_page_size, uint32_t p_fps, float p_split_tolerance) {}

real_t Animation::track_get_key_transition(int p_track, int p_key_idx) const { return 0.0; }

// --- animation.compat.inc ---

#ifndef DISABLE_DEPRECATED
Vector3 Animation::_position_track_interpolate_bind_compat_86629(int p_track, double p_time) const
{
	return position_track_interpolate(p_track, p_time, false);
}

Quaternion Animation::_rotation_track_interpolate_bind_compat_86629(
	int p_track, double p_time) const
{
	return rotation_track_interpolate(p_track, p_time, false);
}

Vector3 Animation::_scale_track_interpolate_bind_compat_86629(int p_track, double p_time) const
{
	return scale_track_interpolate(p_track, p_time, false);
}

float Animation::_blend_shape_track_interpolate_bind_compat_86629(int p_track, double p_time) const
{
	return blend_shape_track_interpolate(p_track, p_time, false);
}

int Animation::_track_find_key_bind_compat_92861(
	int p_track, double p_time, FindMode p_find_mode) const
{
	return track_find_key(p_track, p_time, p_find_mode, false, false);
}

#endif // DISABLE_DEPRECATED

#ifdef print_animc
#undef print_animc
#endif

// --- animation_library.cpp ---

#include "scene/animation/animation_mixer.h"
#include "scene/scene_string_names.h"

bool AnimationLibrary::is_valid_animation_name(const String& p_name)
{
	return !(p_name.is_empty() || p_name.contains_char('/') || p_name.contains_char(':') ||
			 p_name.contains_char(',') || p_name.contains_char('['));
}

bool AnimationLibrary::is_valid_library_name(const String& p_name)
{
	return !(p_name.contains_char('/') || p_name.contains_char(':') || p_name.contains_char(',') ||
			 p_name.contains_char('['));
}

String AnimationLibrary::validate_library_name(const String& p_name)
{
	return p_name.replace_chars("/:,[", '_');
}

bool AnimationLibrary::has_animation(const String& p_name) const
{
	return animations.has(p_name);
}

Ref<Animation> AnimationLibrary::get_animation(const String& p_name) const
{
	ERR_FAIL_COND_V_MSG(
		!animations.has(p_name), Ref<Animation>(), vformat("Animation not found: \"%s\".", p_name));

	return animations[p_name];
}

void AnimationLibrary::get_animation_list(LocalVector<String>* p_animations) const
{
	LocalVector<String> anims;

	for (const KeyValue<String, Ref<Animation>>& E : animations) {
		anims.push_back(E.key);
	}

	anims.sort();

	for (const String& E : anims) {
		p_animations->push_back(E);
	}
}

int AnimationLibrary::get_animation_list_size() const { return animations.size(); }

AnimationLibrary::AnimationLibrary() {}

void AnimationLibrary::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
}

// --- bone_map.cpp ---

#include "scene/resources/animation.h"
#include "core/config/engine.h"

Ref<SkeletonProfile> BoneMap::get_profile() const { return profile; }

String BoneMap::get_skeleton_bone_name(const String& p_profile_bone_name) const
{
	ERR_FAIL_COND_V(!bone_map.has(p_profile_bone_name), String());
	return bone_map.get(p_profile_bone_name);
}

void BoneMap::_set_skeleton_bone_name(
	const String& p_profile_bone_name, const String& p_skeleton_bone_name)
{
	ERR_FAIL_COND(!bone_map.has(p_profile_bone_name));
	bone_map.insert(p_profile_bone_name, p_skeleton_bone_name);
}

String BoneMap::find_profile_bone_name(const String& p_skeleton_bone_name) const
{
	String profile_bone_name;
	HashMap<String, String>::ConstIterator E = bone_map.begin();
	while (E) {
		if (E->value == p_skeleton_bone_name) {
			profile_bone_name = E->key;
			break;
		}
		++E;
	}
	return profile_bone_name;
}

int BoneMap::get_skeleton_bone_name_count(const String& p_skeleton_bone_name) const
{
	int count = 0;
	HashMap<String, String>::ConstIterator E = bone_map.begin();
	while (E) {
		if (E->value == p_skeleton_bone_name) {
			++count;
		}
		++E;
	}
	return count;
}

void BoneMap::_validate_bone_map()
{
	Ref<SkeletonProfile> current_profile = get_profile();
	if (current_profile.is_valid()) {
		// Insert missing profile bones into bone map.
		int len = current_profile->get_bone_size();
		String profile_bone_name;
		for (int i = 0; i < len; i++) {
			profile_bone_name = current_profile->get_bone_name(i);
			if (!bone_map.has(profile_bone_name)) {
				bone_map.insert(profile_bone_name, String());
			}
		}
		// Remove bones that do not exist in the profile from the map.
		Vector<String> delete_bones;
		String k;
		HashMap<String, String>::ConstIterator E = bone_map.begin();
		while (E) {
			k = E->key;
			if (!current_profile->has_bone(k)) {
				delete_bones.push_back(k);
			}
			++E;
		}
		len = delete_bones.size();
		for (int i = 0; i < len; i++) {
			bone_map.erase(delete_bones[i]);
		}
	}
	else {
		bone_map.clear();
	}
}

BoneMap::BoneMap() { _validate_bone_map(); }

BoneMap::~BoneMap() {}



void BoneMap::set_skeleton_bone_name(String const&, String const&) {}

// --- skeleton_profile.cpp ---

#include "core/config/engine.h"
#include "scene/resources/animation.h"

String SkeletonProfile::get_root_bone() { return root_bone; }

void SkeletonProfile::set_root_bone(const String& p_bone_name)
{
	if (is_read_only) {
		return;
	}
	root_bone = p_bone_name;
}

String SkeletonProfile::get_scale_base_bone() { return scale_base_bone; }

void SkeletonProfile::set_scale_base_bone(const String& p_bone_name)
{
	if (is_read_only) {
		return;
	}
	scale_base_bone = p_bone_name;
}

int SkeletonProfile::get_group_size() { return groups.size(); }

String SkeletonProfile::get_group_name(int p_group_idx) const
{
	ERR_FAIL_INDEX_V(p_group_idx, groups.size(), String());
	return groups[p_group_idx].group_name;
}

Ref<Texture2D> SkeletonProfile::get_texture(int p_group_idx) const
{
	ERR_FAIL_INDEX_V(p_group_idx, groups.size(), Ref<Texture2D>());
	return groups[p_group_idx].texture;
}

int SkeletonProfile::get_bone_size() { return bones.size(); }

int SkeletonProfile::find_bone(const String& p_bone_name) const
{
	if (p_bone_name == String()) {
		return -1;
	}
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i].bone_name == p_bone_name) {
			return i;
		}
	}
	return -1;
}

PackedStringArray SkeletonProfile::get_bone_names()
{
	PackedStringArray s;
	for (const SkeletonProfileBone& bone : bones) {
		s.push_back(bone.bone_name);
	}
	return s;
}

String SkeletonProfile::get_bone_name(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), String());
	return bones[p_bone_idx].bone_name;
}

String SkeletonProfile::get_bone_parent(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), String());
	return bones[p_bone_idx].bone_parent;
}

SkeletonProfile::TailDirection SkeletonProfile::get_tail_direction(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), TAIL_DIRECTION_AVERAGE_CHILDREN);
	return bones[p_bone_idx].tail_direction;
}

String SkeletonProfile::get_bone_tail(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), String());
	return bones[p_bone_idx].bone_tail;
}

Transform3D SkeletonProfile::get_reference_pose(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), Transform3D());
	return bones[p_bone_idx].reference_pose;
}

Vector2 SkeletonProfile::get_handle_offset(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), Vector2());
	return bones[p_bone_idx].handle_offset;
}

String SkeletonProfile::get_group(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), String());
	return bones[p_bone_idx].group;
}

bool SkeletonProfile::is_required(int p_bone_idx) const
{
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), false);
	return bones[p_bone_idx].required;
}

bool SkeletonProfile::has_bone(const String& p_bone_name)
{
	bool is_found = false;
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i].bone_name == p_bone_name) {
			is_found = true;
			break;
		}
	}
	return is_found;
}

Ref<SkeletonProfile> SkeletonProfile::create_humanoid()
{
	return Ref<SkeletonProfile>(memnew(SkeletonProfile(true)));
}

SkeletonProfile::SkeletonProfile(bool p_humanoid)
{
	if (!p_humanoid) {
		return;
	}

	is_read_only = true;

	root_bone = "Root";
	scale_base_bone = "Hips";

	groups.resize(4);

	groups.write[0].group_name = "Body";
	groups.write[1].group_name = "Face";
	groups.write[2].group_name = "LeftHand";
	groups.write[3].group_name = "RightHand";

	bones.resize(56);

	bones.write[0].bone_name = "Root";
	bones.write[0].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0);
	bones.write[0].handle_offset = Vector2(0.5, 0.91);
	bones.write[0].group = "Body";

	bones.write[1].bone_name = "Hips";
	bones.write[1].bone_parent = "Root";
	bones.write[1].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.write[1].bone_tail = "Spine";
	bones.write[1].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.75, 0);
	bones.write[1].handle_offset = Vector2(0.5, 0.5);
	bones.write[1].group = "Body";
	bones.write[1].required = true;

	bones.write[2].bone_name = "Spine";
	bones.write[2].bone_parent = "Hips";
	bones.write[2].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.write[2].handle_offset = Vector2(0.5, 0.43);
	bones.write[2].group = "Body";
	bones.write[2].required = true;

	bones.write[3].bone_name = "Chest";
	bones.write[3].bone_parent = "Spine";
	bones.write[3].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.write[3].handle_offset = Vector2(0.5, 0.36);
	bones.write[3].group = "Body";

	bones.write[4].bone_name = "UpperChest";
	bones.write[4].bone_parent = "Chest";
	bones.write[4].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.write[4].handle_offset = Vector2(0.5, 0.29);
	bones.write[4].group = "Body";

	bones.write[5].bone_name = "Neck";
	bones.write[5].bone_parent = "UpperChest";
	bones.write[5].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.write[5].bone_tail = "Head";
	bones.write[5].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.write[5].handle_offset = Vector2(0.5, 0.23);
	bones.write[5].group = "Body";
	bones.write[5].required = false;

	bones.write[6].bone_name = "Head";
	bones.write[6].bone_parent = "Neck";
	bones.write[6].tail_direction = TAIL_DIRECTION_END;
	bones.write[6].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.write[6].handle_offset = Vector2(0.5, 0.18);
	bones.write[6].group = "Body";
	bones.write[6].required = true;

	bones.write[7].bone_name = "LeftEye";
	bones.write[7].bone_parent = "Head";
	bones.write[7].reference_pose = Transform3D(1, 0, 0, 0, 0, -1, 0, 1, 0, 0.05, 0.15, 0);
	bones.write[7].handle_offset = Vector2(0.6, 0.46);
	bones.write[7].group = "Face";

	bones.write[8].bone_name = "RightEye";
	bones.write[8].bone_parent = "Head";
	bones.write[8].reference_pose = Transform3D(1, 0, 0, 0, 0, -1, 0, 1, 0, -0.05, 0.15, 0);
	bones.write[8].handle_offset = Vector2(0.37, 0.46);
	bones.write[8].group = "Face";

	bones.write[9].bone_name = "Jaw";
	bones.write[9].bone_parent = "Head";
	bones.write[9].reference_pose = Transform3D(-1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0.05, 0.05);
	bones.write[9].handle_offset = Vector2(0.46, 0.75);
	bones.write[9].group = "Face";

	bones.write[10].bone_name = "LeftShoulder";
	bones.write[10].bone_parent = "UpperChest";
	bones.write[10].reference_pose = Transform3D(0, 1, 0, 0, 0, 1, 1, 0, 0, 0.05, 0.1, 0);
	bones.write[10].handle_offset = Vector2(0.55, 0.235);
	bones.write[10].group = "Body";
	bones.write[10].required = true;

	bones.write[11].bone_name = "LeftUpperArm";
	bones.write[11].bone_parent = "LeftShoulder";
	bones.write[11].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.05, 0);
	bones.write[11].handle_offset = Vector2(0.6, 0.24);
	bones.write[11].group = "Body";
	bones.write[11].required = true;

	bones.write[12].bone_name = "LeftLowerArm";
	bones.write[12].bone_parent = "LeftUpperArm";
	bones.write[12].reference_pose = Transform3D(0, 0, -1, 0, 1, 0, 1, 0, 0, 0, 0.25, 0);
	bones.write[12].handle_offset = Vector2(0.7, 0.24);
	bones.write[12].group = "Body";
	bones.write[12].required = true;

	bones.write[13].bone_name = "LeftHand";
	bones.write[13].bone_parent = "LeftLowerArm";
	bones.write[13].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.write[13].bone_tail = "LeftMiddleProximal";
	bones.write[13].reference_pose = Transform3D(0, 0, 1, 0, 1, 0, -1, 0, 0, 0, 0.25, 0);
	bones.write[13].handle_offset = Vector2(0.82, 0.235);
	bones.write[13].group = "Body";
	bones.write[13].required = true;

	bones.write[14].bone_name = "LeftThumbMetacarpal";
	bones.write[14].bone_parent = "LeftHand";
	bones.write[14].reference_pose =
		Transform3D(0, -0.577, 0.816, 0, 0.816, 0.577, -1, 0, 0, -0.025, 0.025, 0);
	bones.write[14].handle_offset = Vector2(0.4, 0.8);
	bones.write[14].group = "LeftHand";

	bones.write[15].bone_name = "LeftThumbProximal";
	bones.write[15].bone_parent = "LeftThumbMetacarpal";
	bones.write[15].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.write[15].handle_offset = Vector2(0.3, 0.69);
	bones.write[15].group = "LeftHand";

	bones.write[16].bone_name = "LeftThumbDistal";
	bones.write[16].bone_parent = "LeftThumbProximal";
	bones.write[16].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.write[16].handle_offset = Vector2(0.23, 0.555);
	bones.write[16].group = "LeftHand";

	bones.write[17].bone_name = "LeftIndexProximal";
	bones.write[17].bone_parent = "LeftHand";
	bones.write[17].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.025, 0.075, 0);
	bones.write[17].handle_offset = Vector2(0.413, 0.52);
	bones.write[17].group = "LeftHand";

	bones.write[18].bone_name = "LeftIndexIntermediate";
	bones.write[18].bone_parent = "LeftIndexProximal";
	bones.write[18].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[18].handle_offset = Vector2(0.403, 0.36);
	bones.write[18].group = "LeftHand";

	bones.write[19].bone_name = "LeftIndexDistal";
	bones.write[19].bone_parent = "LeftIndexIntermediate";
	bones.write[19].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[19].handle_offset = Vector2(0.403, 0.255);
	bones.write[19].group = "LeftHand";

	bones.write[20].bone_name = "LeftMiddleProximal";
	bones.write[20].bone_parent = "LeftHand";
	bones.write[20].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.write[20].handle_offset = Vector2(0.5, 0.51);
	bones.write[20].group = "LeftHand";

	bones.write[21].bone_name = "LeftMiddleIntermediate";
	bones.write[21].bone_parent = "LeftMiddleProximal";
	bones.write[21].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.write[21].handle_offset = Vector2(0.5, 0.345);
	bones.write[21].group = "LeftHand";

	bones.write[22].bone_name = "LeftMiddleDistal";
	bones.write[22].bone_parent = "LeftMiddleIntermediate";
	bones.write[22].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[22].handle_offset = Vector2(0.5, 0.22);
	bones.write[22].group = "LeftHand";

	bones.write[23].bone_name = "LeftRingProximal";
	bones.write[23].bone_parent = "LeftHand";
	bones.write[23].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.025, 0.075, 0);
	bones.write[23].handle_offset = Vector2(0.586, 0.52);
	bones.write[23].group = "LeftHand";

	bones.write[24].bone_name = "LeftRingIntermediate";
	bones.write[24].bone_parent = "LeftRingProximal";
	bones.write[24].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[24].handle_offset = Vector2(0.59, 0.36);
	bones.write[24].group = "LeftHand";

	bones.write[25].bone_name = "LeftRingDistal";
	bones.write[25].bone_parent = "LeftRingIntermediate";
	bones.write[25].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[25].handle_offset = Vector2(0.591, 0.25);
	bones.write[25].group = "LeftHand";

	bones.write[26].bone_name = "LeftLittleProximal";
	bones.write[26].bone_parent = "LeftHand";
	bones.write[26].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.05, 0.05, 0);
	bones.write[26].handle_offset = Vector2(0.663, 0.543);
	bones.write[26].group = "LeftHand";

	bones.write[27].bone_name = "LeftLittleIntermediate";
	bones.write[27].bone_parent = "LeftLittleProximal";
	bones.write[27].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[27].handle_offset = Vector2(0.672, 0.415);
	bones.write[27].group = "LeftHand";

	bones.write[28].bone_name = "LeftLittleDistal";
	bones.write[28].bone_parent = "LeftLittleIntermediate";
	bones.write[28].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[28].handle_offset = Vector2(0.672, 0.32);
	bones.write[28].group = "LeftHand";

	bones.write[29].bone_name = "RightShoulder";
	bones.write[29].bone_parent = "UpperChest";
	bones.write[29].reference_pose = Transform3D(0, -1, 0, 0, 0, 1, -1, 0, 0, -0.05, 0.1, 0);
	bones.write[29].handle_offset = Vector2(0.45, 0.235);
	bones.write[29].group = "Body";
	bones.write[29].required = true;

	bones.write[30].bone_name = "RightUpperArm";
	bones.write[30].bone_parent = "RightShoulder";
	bones.write[30].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.05, 0);
	bones.write[30].handle_offset = Vector2(0.4, 0.24);
	bones.write[30].group = "Body";
	bones.write[30].required = true;

	bones.write[31].bone_name = "RightLowerArm";
	bones.write[31].bone_parent = "RightUpperArm";
	bones.write[31].reference_pose = Transform3D(0, 0, 1, 0, 1, 0, -1, 0, 0, 0, 0.25, 0);
	bones.write[31].handle_offset = Vector2(0.3, 0.24);
	bones.write[31].group = "Body";
	bones.write[31].required = true;

	bones.write[32].bone_name = "RightHand";
	bones.write[32].bone_parent = "RightLowerArm";
	bones.write[32].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.write[32].bone_tail = "RightMiddleProximal";
	bones.write[32].reference_pose = Transform3D(0, 0, -1, 0, 1, 0, 1, 0, 0, 0, 0.25, 0);
	bones.write[32].handle_offset = Vector2(0.18, 0.235);
	bones.write[32].group = "Body";
	bones.write[32].required = true;

	bones.write[33].bone_name = "RightThumbMetacarpal";
	bones.write[33].bone_parent = "RightHand";
	bones.write[33].reference_pose =
		Transform3D(0, 0.577, -0.816, 0, 0.816, 0.577, 1, 0, 0, 0.025, 0.025, 0);
	bones.write[33].handle_offset = Vector2(0.6, 0.8);
	bones.write[33].group = "RightHand";

	bones.write[34].bone_name = "RightThumbProximal";
	bones.write[34].bone_parent = "RightThumbMetacarpal";
	bones.write[34].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.write[34].handle_offset = Vector2(0.7, 0.69);
	bones.write[34].group = "RightHand";

	bones.write[35].bone_name = "RightThumbDistal";
	bones.write[35].bone_parent = "RightThumbProximal";
	bones.write[35].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.write[35].handle_offset = Vector2(0.77, 0.555);
	bones.write[35].group = "RightHand";

	bones.write[36].bone_name = "RightIndexProximal";
	bones.write[36].bone_parent = "RightHand";
	bones.write[36].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.025, 0.075, 0);
	bones.write[36].handle_offset = Vector2(0.587, 0.52);
	bones.write[36].group = "RightHand";

	bones.write[37].bone_name = "RightIndexIntermediate";
	bones.write[37].bone_parent = "RightIndexProximal";
	bones.write[37].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[37].handle_offset = Vector2(0.597, 0.36);
	bones.write[37].group = "RightHand";

	bones.write[38].bone_name = "RightIndexDistal";
	bones.write[38].bone_parent = "RightIndexIntermediate";
	bones.write[38].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[38].handle_offset = Vector2(0.597, 0.255);
	bones.write[38].group = "RightHand";

	bones.write[39].bone_name = "RightMiddleProximal";
	bones.write[39].bone_parent = "RightHand";
	bones.write[39].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.write[39].handle_offset = Vector2(0.5, 0.51);
	bones.write[39].group = "RightHand";

	bones.write[40].bone_name = "RightMiddleIntermediate";
	bones.write[40].bone_parent = "RightMiddleProximal";
	bones.write[40].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.write[40].handle_offset = Vector2(0.5, 0.345);
	bones.write[40].group = "RightHand";

	bones.write[41].bone_name = "RightMiddleDistal";
	bones.write[41].bone_parent = "RightMiddleIntermediate";
	bones.write[41].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[41].handle_offset = Vector2(0.5, 0.22);
	bones.write[41].group = "RightHand";

	bones.write[42].bone_name = "RightRingProximal";
	bones.write[42].bone_parent = "RightHand";
	bones.write[42].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.025, 0.075, 0);
	bones.write[42].handle_offset = Vector2(0.414, 0.52);
	bones.write[42].group = "RightHand";

	bones.write[43].bone_name = "RightRingIntermediate";
	bones.write[43].bone_parent = "RightRingProximal";
	bones.write[43].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[43].handle_offset = Vector2(0.41, 0.36);
	bones.write[43].group = "RightHand";

	bones.write[44].bone_name = "RightRingDistal";
	bones.write[44].bone_parent = "RightRingIntermediate";
	bones.write[44].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[44].handle_offset = Vector2(0.409, 0.25);
	bones.write[44].group = "RightHand";

	bones.write[45].bone_name = "RightLittleProximal";
	bones.write[45].bone_parent = "RightHand";
	bones.write[45].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.05, 0.05, 0);
	bones.write[45].handle_offset = Vector2(0.337, 0.543);
	bones.write[45].group = "RightHand";

	bones.write[46].bone_name = "RightLittleIntermediate";
	bones.write[46].bone_parent = "RightLittleProximal";
	bones.write[46].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.write[46].handle_offset = Vector2(0.328, 0.415);
	bones.write[46].group = "RightHand";

	bones.write[47].bone_name = "RightLittleDistal";
	bones.write[47].bone_parent = "RightLittleIntermediate";
	bones.write[47].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.write[47].handle_offset = Vector2(0.328, 0.32);
	bones.write[47].group = "RightHand";

	bones.write[48].bone_name = "LeftUpperLeg";
	bones.write[48].bone_parent = "Hips";
	bones.write[48].reference_pose = Transform3D(-1, 0, 0, 0, -1, 0, 0, 0, 1, 0.1, 0, 0);
	bones.write[48].handle_offset = Vector2(0.549, 0.49);
	bones.write[48].group = "Body";
	bones.write[48].required = true;

	bones.write[49].bone_name = "LeftLowerLeg";
	bones.write[49].bone_parent = "LeftUpperLeg";
	bones.write[49].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.375, 0);
	bones.write[49].handle_offset = Vector2(0.548, 0.683);
	bones.write[49].group = "Body";
	bones.write[49].required = true;

	bones.write[50].bone_name = "LeftFoot";
	bones.write[50].bone_parent = "LeftLowerLeg";
	bones.write[50].reference_pose = Transform3D(-1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0.375, 0);
	bones.write[50].handle_offset = Vector2(0.545, 0.9);
	bones.write[50].group = "Body";
	bones.write[50].required = true;

	bones.write[51].bone_name = "LeftToes";
	bones.write[51].bone_parent = "LeftFoot";
	bones.write[51].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.15, 0);
	bones.write[51].handle_offset = Vector2(0.545, 0.95);
	bones.write[51].group = "Body";

	bones.write[52].bone_name = "RightUpperLeg";
	bones.write[52].bone_parent = "Hips";
	bones.write[52].reference_pose = Transform3D(-1, 0, 0, 0, -1, 0, 0, 0, 1, -0.1, 0, 0);
	bones.write[52].handle_offset = Vector2(0.451, 0.49);
	bones.write[52].group = "Body";
	bones.write[52].required = true;

	bones.write[53].bone_name = "RightLowerLeg";
	bones.write[53].bone_parent = "RightUpperLeg";
	bones.write[53].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.375, 0);
	bones.write[53].handle_offset = Vector2(0.452, 0.683);
	bones.write[53].group = "Body";
	bones.write[53].required = true;

	bones.write[54].bone_name = "RightFoot";
	bones.write[54].bone_parent = "RightLowerLeg";
	bones.write[54].reference_pose = Transform3D(-1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0.375, 0);
	bones.write[54].handle_offset = Vector2(0.455, 0.9);
	bones.write[54].group = "Body";
	bones.write[54].required = true;

	bones.write[55].bone_name = "RightToes";
	bones.write[55].bone_parent = "RightFoot";
	bones.write[55].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.15, 0);
	bones.write[55].handle_offset = Vector2(0.455, 0.95);
	bones.write[55].group = "Body";
}

SkeletonProfile::~SkeletonProfile() {}

//////////////////////////////////////



void SkeletonProfile::set_reference_pose(int, Transform3D const&) {}

void SkeletonProfile::set_handle_offset(int, Vector2 const&) {}

void SkeletonProfile::set_group_size(int) {}

void SkeletonProfile::set_bone_size(int) {}

void SkeletonProfile::set_bone_parent(int, String const&) {}

void SkeletonProfile::set_bone_name(int, String const&) {}


// --- Merged from scene/resources/3d/primitive_meshes.cpp ---
#if !defined(_3D_DISABLED)
#include "scene/resources/3d/primitive_meshes.h"

#include <thirdparty/misc/polypartition.h>

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/main_loop.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

#define PADDING_REF_SIZE 1024.0

RID PrimitiveMesh::TextMeshState::ensure_shaped_text() const
{
	if (!text_rid.is_valid() && TS.is_valid()) {
		text_rid = TS->create_shaped_text();
	}
	return text_rid;
}

PrimitiveMesh::TextMeshState::~TextMeshState()
{
	if ((lines_rid.size() > 0 || text_rid.is_valid()) && TS.is_valid()) {
		for (int i = 0; i < lines_rid.size(); i++) {
			TS->free_rid(lines_rid[i]);
		}
		if (text_rid.is_valid()) {
			TS->free_rid(text_rid);
		}
	}
	lines_rid.clear();
}

PrimitiveMesh::PrimitiveMesh(MeshType p_type)
{
	_apply_mesh_type_defaults(p_type);
}

PrimitiveMesh::~PrimitiveMesh() = default;

void PrimitiveMesh::_apply_mesh_type_defaults(MeshType p_type)
{
	mesh_type = p_type;
	primitive_type = Mesh::PRIMITIVE_TRIANGLES;

	switch (p_type) {
		case MESH_BOX: {
			box_prism.size = Vector3(1.0f, 1.0f, 1.0f);
		} break;
		case MESH_CAPSULE: {
			radial.radius = 0.5f;
			radial.height = 2.0f;
			radial.radial_segments = 64;
			radial.rings = 8;
		} break;
		case MESH_CYLINDER: {
			radial.top_radius = 0.5f;
			radial.bottom_radius = 0.5f;
			radial.height = 2.0f;
			radial.radial_segments = 64;
			radial.rings = 4;
			radial.cap_top = true;
			radial.cap_bottom = true;
		} break;
		case MESH_PLANE: {
			plane.size = Size2(2.0f, 2.0f);
			plane.orientation = FACE_Y;
		} break;
		case MESH_QUAD: {
			plane.size = Size2(1.0f, 1.0f);
			plane.orientation = FACE_Z;
		} break;
		case MESH_PRISM: {
			box_prism.left_to_right = 0.5f;
			box_prism.size = Vector3(1.0f, 1.0f, 1.0f);
		} break;
		case MESH_SPHERE: {
			radial.radius = 0.5f;
			radial.height = 1.0f;
			radial.radial_segments = 64;
			radial.rings = 32;
			radial.is_hemisphere = false;
		} break;
		case MESH_TORUS: {
			radial.inner_radius = 0.5f;
			radial.outer_radius = 1.0f;
			radial.rings = 64;
			radial.ring_segments = 32;
		} break;
		case MESH_POINT: {
			primitive_type = Mesh::PRIMITIVE_POINTS;
		} break;
		case MESH_TUBE_TRAIL: {
			radial.radius = 0.5f;
			radial.cap_top = true;
			radial.cap_bottom = true;
			trail.radial_steps = 8;
			trail.sections = 5;
			trail.section_length = 0.2f;
			trail.section_rings = 3;
		} break;
		case MESH_RIBBON_TRAIL: {
			trail.ribbon_size = 1.0f;
			trail.sections = 5;
			trail.section_length = 0.2f;
			trail.section_segments = 3;
			trail.ribbon_shape = SHAPE_CROSS;
		} break;
		case MESH_TEXT: {
			_ensure_text_state();
		} break;
	}
}

void PrimitiveMesh::set_mesh_type(MeshType p_type)
{
	if (mesh_type == p_type) {
		return;
	}
	_apply_mesh_type_defaults(p_type);
	_update_lightmap_size();
	request_update();
}

PrimitiveMesh::TextMeshState& PrimitiveMesh::_ensure_text_state()
{
	if (!text_state) {
		text_state = std::make_unique<TextMeshState>();
	}
	return *text_state;
}

const PrimitiveMesh::TextMeshState& PrimitiveMesh::_get_text_state() const
{
	if (text_state) {
		return *text_state;
	}
	static const TextMeshState default_state{};
	return default_state;
}

void PrimitiveMesh::request_update()
{
	if (pending_request) {
		return;
	}
	_update();
}

int PrimitiveMesh::get_surface_count() const
{
	if (pending_request) {
		_update();
	}
	return 1;
}

int PrimitiveMesh::surface_get_array_len(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, 1, -1);
	if (pending_request) {
		_update();
	}

	return array_len;
}

int PrimitiveMesh::surface_get_array_index_len(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, 1, -1);
	if (pending_request) {
		_update();
	}

	return index_array_len;
}

uint32_t PrimitiveMesh::surface_get_format(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, 1, 0);

	uint64_t mesh_format = RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_NORMAL |
						   RSE::ARRAY_FORMAT_TANGENT | RSE::ARRAY_FORMAT_TEX_UV |
						   RSE::ARRAY_FORMAT_INDEX;
	if (add_uv2) {
		mesh_format |= RSE::ARRAY_FORMAT_TEX_UV2;
	}

	return mesh_format;
}

Mesh::PrimitiveType PrimitiveMesh::surface_get_primitive_type(int p_idx) const
{
	return primitive_type;
}

void PrimitiveMesh::surface_set_material(int p_idx, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_idx, 1);

	set_material(p_material);
}

Ref<Material> PrimitiveMesh::surface_get_material(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, 1, nullptr);

	return material;
}

int PrimitiveMesh::get_blend_shape_count() const { return 0; }

String PrimitiveMesh::get_blend_shape_name(int p_index) const { return String(); }

void PrimitiveMesh::set_blend_shape_name(int p_index, const String& p_name) {}

AABB PrimitiveMesh::get_aabb() const
{
	if (pending_request) {
		_update();
	}

	return aabb;
}

RID PrimitiveMesh::get_rid() const
{
	if (pending_request) {
		_update();
	}
	return mesh;
}

void PrimitiveMesh::set_material(const Ref<Material>& p_material)
{
	if (material == p_material) {
		return;
	}
	material = p_material;
	if (!pending_request && mesh.is_valid()) {
		Renderer::mesh_surface_set_material(
			mesh, 0, material.is_null() ? RID() : material->get_rid());
	}
	emit_changed();
}

Ref<Material> PrimitiveMesh::get_material() const { return material; }

void PrimitiveMesh::set_custom_aabb(const AABB& p_custom)
{
	if (p_custom.is_equal_approx(custom_aabb)) {
		return;
	}
	custom_aabb = p_custom;
	if (mesh.is_valid()) {
		Renderer::mesh_set_custom_aabb(mesh, custom_aabb);
	}
	emit_changed();
}

AABB PrimitiveMesh::get_custom_aabb() const { return custom_aabb; }

void PrimitiveMesh::set_flip_faces(bool p_enable)
{
	if (p_enable == flip_faces) {
		return;
	}
	flip_faces = p_enable;
	request_update();
}

bool PrimitiveMesh::get_flip_faces() const { return flip_faces; }

void PrimitiveMesh::set_add_uv2(bool p_enable)
{
	if (p_enable == add_uv2) {
		return;
	}
	add_uv2 = p_enable;
	_update_lightmap_size();
	request_update();
}

void PrimitiveMesh::set_uv2_padding(float p_padding)
{
	if (Math::is_equal_approx(p_padding, uv2_padding)) {
		return;
	}
	uv2_padding = p_padding;
	_update_lightmap_size();
	request_update();
}

Vector2 PrimitiveMesh::get_uv2_scale(Vector2 p_margin_scale) const
{
	Vector2 uv2_scale;
	Vector2 lightmap_size = get_lightmap_size_hint();

	// Calculate it as a margin, if no lightmap size hint is given we assume "PADDING_REF_SIZE" as
	// our texture size.
	uv2_scale.x = p_margin_scale.x * uv2_padding /
				  (lightmap_size.x == 0.0 ? PADDING_REF_SIZE : lightmap_size.x);
	uv2_scale.y = p_margin_scale.y * uv2_padding /
				  (lightmap_size.y == 0.0 ? PADDING_REF_SIZE : lightmap_size.y);

	// Inverse it to turn our margin into a scale.
	uv2_scale = Vector2(1.0, 1.0) - uv2_scale;

	return uv2_scale;
}

float PrimitiveMesh::get_lightmap_texel_size() const { return texel_size; }

void PrimitiveMesh::_on_settings_changed()
{
	_update_lightmap_size();
	request_update();
}

void PrimitiveMesh::_update_lightmap_size()
{
	if (!get_add_uv2()) {
		return;
	}

	Size2i lightmap_hint;
	float padding = get_uv2_padding();

	switch (mesh_type) {
		case MESH_BOX: {
			float width = (box_prism.size.x + box_prism.size.z) / texel_size;
			float length =
				(box_prism.size.y + box_prism.size.y + MAX(box_prism.size.x, box_prism.size.z)) /
				texel_size;

			lightmap_hint.x = MAX(1.0, width) + 2.0 * padding;
			lightmap_hint.y = MAX(1.0, length) + 3.0 * padding;
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_CAPSULE: {
			float radial_length = radial.radius * Math::PI * 0.5;
			float vertical_length =
				radial_length * 2 + (radial.height - 2.0 * radial.radius);

			lightmap_hint.x = MAX(1.0, 4.0 * radial_length / texel_size) + padding;
			lightmap_hint.y = MAX(1.0, vertical_length / texel_size) + padding;
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_CYLINDER: {
			float top_circumference = radial.top_radius * Math::PI * 2.0;
			float bottom_circumference = radial.bottom_radius * Math::PI * 2.0;

			float width = MAX(top_circumference, bottom_circumference) / texel_size + padding;
			width = MAX(
				width, (((radial.top_radius + radial.bottom_radius) / texel_size) + padding) * 2.0);
			lightmap_hint.x = MAX(1.0, width);

			float height =
				((radial.height + (MAX(radial.top_radius, radial.bottom_radius) * 2.0)) /
					texel_size) +
				(2.0 * padding);
			lightmap_hint.y = MAX(1.0, height);
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_PLANE:
		case MESH_QUAD: {
			lightmap_hint.x = MAX(1.0, (plane.size.x / texel_size) + padding);
			lightmap_hint.y = MAX(1.0, (plane.size.y / texel_size) + padding);
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_PRISM: {
			float width = (box_prism.size.x + box_prism.size.z) / texel_size;
			float length = (box_prism.size.y + box_prism.size.y + box_prism.size.z) / texel_size;

			lightmap_hint.x = MAX(1.0, width) + 2.0 * padding;
			lightmap_hint.y = MAX(1.0, length) + 3.0 * padding;
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_SPHERE: {
			float width = radial.radius * Math::TAU;
			lightmap_hint.x = MAX(1.0, (width / texel_size) + padding);
			float height = (radial.is_hemisphere ? 1.0 : 0.5) * radial.height * Math::PI;
			lightmap_hint.y = MAX(1.0, (height / texel_size) + padding);
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_TORUS: {
			float min_radius = radial.inner_radius;
			float max_radius = radial.outer_radius;
			if (min_radius > max_radius) {
				SWAP(min_radius, max_radius);
			}
			float torus_radius = (max_radius - min_radius) * 0.5;
			float width = max_radius * Math::TAU;
			lightmap_hint.x = MAX(1.0, (width / texel_size) + padding);
			float height = torus_radius * Math::TAU;
			lightmap_hint.y = MAX(1.0, (height / texel_size) + padding);
			set_lightmap_size_hint(lightmap_hint);
		} break;
		case MESH_POINT:
		case MESH_TUBE_TRAIL:
		case MESH_RIBBON_TRAIL:
		case MESH_TEXT:
			break;
	}
}

void PrimitiveMesh::_update() const
{
	pending_request = false;

	switch (mesh_type) {
		case MESH_BOX:
		case MESH_PRISM: {
			aabb = AABB(-box_prism.size * 0.5f, box_prism.size);
		} break;
		case MESH_CAPSULE: {
			aabb = AABB(
				Vector3(-radial.radius, -radial.height * 0.5f, -radial.radius),
				Vector3(radial.radius * 2.0f, radial.height, radial.radius * 2.0f));
		} break;
		case MESH_CYLINDER: {
			float max_r = MAX(radial.top_radius, radial.bottom_radius);
			aabb = AABB(
				Vector3(-max_r, -radial.height * 0.5f, -max_r),
				Vector3(max_r * 2.0f, radial.height, max_r * 2.0f));
		} break;
		case MESH_PLANE:
		case MESH_QUAD: {
			switch (plane.orientation) {
				case FACE_X:
					aabb = AABB(
						Vector3(0.0f, -plane.size.y * 0.5f, -plane.size.x * 0.5f) +
							plane.center_offset,
						Vector3(0.0f, plane.size.y, plane.size.x));
					break;
				case FACE_Y:
					aabb = AABB(
						Vector3(-plane.size.x * 0.5f, 0.0f, -plane.size.y * 0.5f) +
							plane.center_offset,
						Vector3(plane.size.x, 0.0f, plane.size.y));
					break;
				case FACE_Z:
					aabb = AABB(
						Vector3(-plane.size.x * 0.5f, -plane.size.y * 0.5f, 0.0f) +
							plane.center_offset,
						Vector3(plane.size.x, plane.size.y, 0.0f));
					break;
			}
		} break;
		case MESH_SPHERE: {
			if (radial.is_hemisphere) {
				aabb = AABB(
					Vector3(-radial.radius, 0.0f, -radial.radius),
					Vector3(radial.radius * 2.0f, radial.height, radial.radius * 2.0f));
			} else {
				aabb = AABB(
					Vector3(-radial.radius, -radial.height * 0.5f, -radial.radius),
					Vector3(radial.radius * 2.0f, radial.height, radial.radius * 2.0f));
			}
		} break;
		case MESH_TORUS: {
			float max_r = MAX(radial.inner_radius, radial.outer_radius);
			float min_r = MIN(radial.inner_radius, radial.outer_radius);
			float h = (max_r - min_r) * 0.5f;
			aabb = AABB(Vector3(-max_r, -h, -max_r), Vector3(max_r * 2.0f, h * 2.0f, max_r * 2.0f));
		} break;
		case MESH_POINT: {
			aabb = AABB();
		} break;
		case MESH_TUBE_TRAIL: {
			float total_depth = trail.section_length * trail.sections;
			aabb = AABB(
				Vector3(-radial.radius, -total_depth * 0.5f, -radial.radius),
				Vector3(radial.radius * 2.0f, total_depth, radial.radius * 2.0f));
		} break;
		case MESH_RIBBON_TRAIL: {
			float total_depth = trail.section_length * trail.sections;
			float half_s = trail.ribbon_size * 0.5f;
			if (trail.ribbon_shape == SHAPE_CROSS) {
				aabb = AABB(
					Vector3(-half_s, -total_depth * 0.5f, -half_s),
					Vector3(trail.ribbon_size, total_depth, trail.ribbon_size));
			} else {
				aabb = AABB(
					Vector3(-half_s, -total_depth * 0.5f, 0.0f),
					Vector3(trail.ribbon_size, total_depth, 0.0f));
			}
		} break;
		case MESH_TEXT: {
			aabb = AABB();
		} break;
	}
}

int PrimitiveMesh::get_builtin_bind_pose_count() const
{
	switch (mesh_type) {
		case MESH_TUBE_TRAIL:
		case MESH_RIBBON_TRAIL:
			return trail.sections + 1;
		default:
			return 0;
	}
}

Transform3D PrimitiveMesh::get_builtin_bind_pose(int p_index) const
{
	switch (mesh_type) {
		case MESH_TUBE_TRAIL:
		case MESH_RIBBON_TRAIL: {
			float depth = trail.section_length * trail.sections;

			Transform3D xform;
			xform.origin.y = depth / 2.0f - trail.section_length * float(p_index);
			xform.origin.y = -xform.origin.y; // bind is an inverse transform, so negate y

			return xform;
		}
		default:
			return Transform3D();
	}
}

// Box / Prism / Plane / Quad parameters.

void PrimitiveMesh::set_size(const Vector3& p_size)
{
	if (p_size.is_equal_approx(box_prism.size)) {
		return;
	}

	box_prism.size = p_size;
	_update_lightmap_size();
	request_update();
}

Vector3 PrimitiveMesh::get_size() const { return box_prism.size; }

void PrimitiveMesh::set_plane_size(const Size2& p_size)
{
	if (p_size == plane.size) {
		return;
	}
	plane.size = p_size;
	_update_lightmap_size();
	request_update();
}

Size2 PrimitiveMesh::get_plane_size() const { return plane.size; }

void PrimitiveMesh::set_subdivide_width(int p_divisions)
{
	if (p_divisions == box_prism.subdivide_w || (box_prism.subdivide_w == 0 && p_divisions < 0)) {
		return;
	}
	box_prism.subdivide_w = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrimitiveMesh::get_subdivide_width() const { return box_prism.subdivide_w; }

void PrimitiveMesh::set_subdivide_height(int p_divisions)
{
	if (p_divisions == box_prism.subdivide_h || (box_prism.subdivide_h == 0 && p_divisions < 0)) {
		return;
	}
	box_prism.subdivide_h = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrimitiveMesh::get_subdivide_height() const { return box_prism.subdivide_h; }

void PrimitiveMesh::set_subdivide_depth(int p_divisions)
{
	if (p_divisions == box_prism.subdivide_d || (box_prism.subdivide_d == 0 && p_divisions < 0)) {
		return;
	}
	box_prism.subdivide_d = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrimitiveMesh::get_subdivide_depth() const { return box_prism.subdivide_d; }

void PrimitiveMesh::set_center_offset(const Vector3& p_offset)
{
	if (p_offset.is_equal_approx(plane.center_offset)) {
		return;
	}
	plane.center_offset = p_offset;
	request_update();
}

Vector3 PrimitiveMesh::get_center_offset() const { return plane.center_offset; }

void PrimitiveMesh::set_orientation(Orientation p_orientation)
{
	if (p_orientation == plane.orientation) {
		return;
	}
	plane.orientation = p_orientation;
	request_update();
}

PrimitiveMesh::Orientation PrimitiveMesh::get_orientation() const { return plane.orientation; }

void PrimitiveMesh::set_left_to_right(float p_left_to_right)
{
	if (Math::is_equal_approx(p_left_to_right, box_prism.left_to_right)) {
		return;
	}
	box_prism.left_to_right = p_left_to_right;
	request_update();
}

float PrimitiveMesh::get_left_to_right() const { return box_prism.left_to_right; }

// Capsule / Cylinder / Sphere / Torus parameters.

void PrimitiveMesh::set_radius(float p_radius)
{
	if (Math::is_equal_approx(radial.radius, p_radius)) {
		return;
	}

	radial.radius = p_radius;
	if (mesh_type == MESH_CAPSULE && radial.radius > radial.height * 0.5f) {
		radial.height = radial.radius * 2.0f;
	}
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_radius() const { return radial.radius; }

void PrimitiveMesh::set_top_radius(float p_radius)
{
	if (Math::is_equal_approx(p_radius, radial.top_radius)) {
		return;
	}

	radial.top_radius = p_radius;
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_top_radius() const { return radial.top_radius; }

void PrimitiveMesh::set_bottom_radius(float p_radius)
{
	if (Math::is_equal_approx(p_radius, radial.bottom_radius)) {
		return;
	}

	radial.bottom_radius = p_radius;
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_bottom_radius() const { return radial.bottom_radius; }

void PrimitiveMesh::set_inner_radius(float p_inner_radius)
{
	if (Math::is_equal_approx(p_inner_radius, radial.inner_radius)) {
		return;
	}
	radial.inner_radius = p_inner_radius;
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_inner_radius() const { return radial.inner_radius; }

void PrimitiveMesh::set_outer_radius(float p_outer_radius)
{
	if (Math::is_equal_approx(p_outer_radius, radial.outer_radius)) {
		return;
	}
	radial.outer_radius = p_outer_radius;
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_outer_radius() const { return radial.outer_radius; }

void PrimitiveMesh::set_height(float p_height)
{
	if (Math::is_equal_approx(radial.height, p_height)) {
		return;
	}

	radial.height = p_height;
	if (mesh_type == MESH_CAPSULE && radial.radius > radial.height * 0.5f) {
		radial.radius = radial.height * 0.5f;
	}
	_update_lightmap_size();
	request_update();
}

float PrimitiveMesh::get_height() const { return radial.height; }

void PrimitiveMesh::set_radial_segments(int p_segments)
{
	if (p_segments == radial.radial_segments || (radial.radial_segments == 4 && p_segments < 4)) {
		return;
	}
	radial.radial_segments = p_segments > 4 ? p_segments : 4;
	request_update();
}

int PrimitiveMesh::get_radial_segments() const { return radial.radial_segments; }

void PrimitiveMesh::set_rings(int p_rings)
{
	if (p_rings == radial.rings) {
		return;
	}
	if (mesh_type == MESH_TORUS) {
		ERR_FAIL_COND(p_rings < 3);
	} else if (mesh_type == MESH_SPHERE) {
		ERR_FAIL_COND(p_rings < 1);
	} else {
		ERR_FAIL_COND(p_rings < 0);
	}
	radial.rings = p_rings;
	request_update();
}

int PrimitiveMesh::get_rings() const { return radial.rings; }

void PrimitiveMesh::set_ring_segments(int p_ring_segments)
{
	if (p_ring_segments == radial.ring_segments) {
		return;
	}
	ERR_FAIL_COND(p_ring_segments < 3);
	radial.ring_segments = p_ring_segments;
	request_update();
}

int PrimitiveMesh::get_ring_segments() const { return radial.ring_segments; }

void PrimitiveMesh::set_cap_top(bool p_cap_top)
{
	if (p_cap_top == radial.cap_top) {
		return;
	}
	radial.cap_top = p_cap_top;
	request_update();
}

bool PrimitiveMesh::is_cap_top() const { return radial.cap_top; }

void PrimitiveMesh::set_cap_bottom(bool p_cap_bottom)
{
	if (p_cap_bottom == radial.cap_bottom) {
		return;
	}
	radial.cap_bottom = p_cap_bottom;
	request_update();
}

bool PrimitiveMesh::is_cap_bottom() const { return radial.cap_bottom; }

void PrimitiveMesh::set_is_hemisphere(bool p_is_hemisphere)
{
	if (p_is_hemisphere == radial.is_hemisphere) {
		return;
	}
	radial.is_hemisphere = p_is_hemisphere;
	_update_lightmap_size();
	request_update();
}

bool PrimitiveMesh::get_is_hemisphere() const { return radial.is_hemisphere; }

// TubeTrail / RibbonTrail parameters.

void PrimitiveMesh::set_radial_steps(int p_radial_steps)
{
	if (p_radial_steps == trail.radial_steps) {
		return;
	}
	ERR_FAIL_COND(p_radial_steps < 3 || p_radial_steps > 128);
	trail.radial_steps = p_radial_steps;
	request_update();
}

int PrimitiveMesh::get_radial_steps() const { return trail.radial_steps; }

void PrimitiveMesh::set_sections(int p_sections)
{
	if (p_sections == trail.sections) {
		return;
	}
	ERR_FAIL_COND(p_sections < 2 || p_sections > 128);
	trail.sections = p_sections;
	request_update();
}

int PrimitiveMesh::get_sections() const { return trail.sections; }

void PrimitiveMesh::set_section_length(float p_section_length)
{
	if (p_section_length == trail.section_length) {
		return;
	}
	trail.section_length = p_section_length;
	request_update();
}

float PrimitiveMesh::get_section_length() const { return trail.section_length; }

void PrimitiveMesh::set_section_rings(int p_section_rings)
{
	if (p_section_rings == trail.section_rings) {
		return;
	}
	ERR_FAIL_COND(p_section_rings < 1 || p_section_rings > 1024);
	trail.section_rings = p_section_rings;
	request_update();
}

int PrimitiveMesh::get_section_rings() const { return trail.section_rings; }

void PrimitiveMesh::set_section_segments(int p_section_segments)
{
	if (p_section_segments == trail.section_segments) {
		return;
	}
	ERR_FAIL_COND(p_section_segments < 1 || p_section_segments > 1024);
	trail.section_segments = p_section_segments;
	request_update();
}

int PrimitiveMesh::get_section_segments() const { return trail.section_segments; }

void PrimitiveMesh::set_ribbon_size(float p_size)
{
	if (Math::is_equal_approx(p_size, trail.ribbon_size)) {
		return;
	}
	trail.ribbon_size = p_size;
	request_update();
}

float PrimitiveMesh::get_ribbon_size() const { return trail.ribbon_size; }

void PrimitiveMesh::set_ribbon_shape(RibbonShape p_shape)
{
	if (p_shape == trail.ribbon_shape) {
		return;
	}
	trail.ribbon_shape = p_shape;
	request_update();
}

PrimitiveMesh::RibbonShape PrimitiveMesh::get_ribbon_shape() const { return trail.ribbon_shape; }

void PrimitiveMesh::set_curve(const Ref<Curve>& p_curve)
{
	if (trail.curve == p_curve) {
		return;
	}
	trail.curve = p_curve;
	request_update();
}

Ref<Curve> PrimitiveMesh::get_curve() const { return trail.curve; }

void PrimitiveMesh::_curve_changed() { request_update(); }

// Text parameters.

void PrimitiveMesh::_font_changed()
{
	if (text_state) {
		text_state->dirty_font = true;
		text_state->dirty_cache = true;
	}
	request_update();
}

void PrimitiveMesh::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	TextMeshState& ts = _ensure_text_state();
	if (ts.horizontal_alignment != p_alignment) {
		if (ts.horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL ||
			p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			ts.dirty_lines = true;
		}
		ts.horizontal_alignment = p_alignment;
		request_update();
	}
}

HorizontalAlignment PrimitiveMesh::get_horizontal_alignment() const
{
	return _get_text_state().horizontal_alignment;
}

void PrimitiveMesh::set_vertical_alignment(VerticalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	TextMeshState& ts = _ensure_text_state();
	if (ts.vertical_alignment != p_alignment) {
		ts.vertical_alignment = p_alignment;
		request_update();
	}
}

VerticalAlignment PrimitiveMesh::get_vertical_alignment() const
{
	return _get_text_state().vertical_alignment;
}

void PrimitiveMesh::set_text(const String& p_string)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.text != p_string) {
		ts.text = p_string;
		ts.xl_text = p_string;
		ts.dirty_text = true;
		request_update();
	}
}

String PrimitiveMesh::get_text() const { return _get_text_state().text; }

void PrimitiveMesh::set_font(const Ref<Font>& p_font)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.font_override != p_font) {
		ts.font_override = p_font;
		ts.dirty_font = true;
		ts.dirty_cache = true;
		request_update();
	}
}

Ref<Font> PrimitiveMesh::get_font() const { return _get_text_state().font_override; }

Ref<Font> PrimitiveMesh::_get_font_or_default() const
{
	if (text_state && text_state->font_override.is_valid()) {
		return text_state->font_override;
	}
	return ThemeDB::get_fallback_font();
}

void PrimitiveMesh::set_font_size(int p_size)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.font_size != p_size) {
		ts.font_size = CLAMP(p_size, 1, 127);
		ts.dirty_font = true;
		ts.dirty_cache = true;
		request_update();
	}
}

int PrimitiveMesh::get_font_size() const { return _get_text_state().font_size; }

void PrimitiveMesh::set_line_spacing(float p_line_spacing)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.line_spacing != p_line_spacing) {
		ts.line_spacing = p_line_spacing;
		request_update();
	}
}

float PrimitiveMesh::get_line_spacing() const { return _get_text_state().line_spacing; }

void PrimitiveMesh::set_autowrap_mode(TextServer::AutowrapMode p_mode)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.autowrap_mode != p_mode) {
		ts.autowrap_mode = p_mode;
		ts.dirty_lines = true;
		request_update();
	}
}

TextServer::AutowrapMode PrimitiveMesh::get_autowrap_mode() const
{
	return _get_text_state().autowrap_mode;
}

void PrimitiveMesh::set_justification_flags(uint32_t p_flags)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.jst_flags != p_flags) {
		ts.jst_flags = p_flags;
		ts.dirty_lines = true;
		request_update();
	}
}

uint32_t PrimitiveMesh::get_justification_flags() const { return _get_text_state().jst_flags; }

void PrimitiveMesh::set_depth(real_t p_depth)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.depth != p_depth) {
		ts.depth = MAX(p_depth, 0.0);
		request_update();
	}
}

real_t PrimitiveMesh::get_depth() const { return _get_text_state().depth; }

void PrimitiveMesh::set_width(real_t p_width)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.width != p_width) {
		ts.width = p_width;
		ts.dirty_lines = true;
		request_update();
	}
}

real_t PrimitiveMesh::get_width() const { return _get_text_state().width; }

void PrimitiveMesh::set_pixel_size(real_t p_amount)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.pixel_size != p_amount) {
		ts.pixel_size = CLAMP(p_amount, 0.0001, 128.0);
		ts.dirty_cache = true;
		request_update();
	}
}

real_t PrimitiveMesh::get_pixel_size() const { return _get_text_state().pixel_size; }

void PrimitiveMesh::set_offset(const Point2& p_offset)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.lbl_offset != p_offset) {
		ts.lbl_offset = p_offset;
		request_update();
	}
}

Point2 PrimitiveMesh::get_offset() const { return _get_text_state().lbl_offset; }

void PrimitiveMesh::set_curve_step(real_t p_step)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.curve_step != p_step) {
		ts.curve_step = CLAMP(p_step, 0.1, 10.0);
		ts.dirty_cache = true;
		request_update();
	}
}

real_t PrimitiveMesh::get_curve_step() const { return _get_text_state().curve_step; }

void PrimitiveMesh::set_text_direction(TextServer::Direction p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	TextMeshState& ts = _ensure_text_state();
	if (ts.text_direction != p_text_direction) {
		ts.text_direction = p_text_direction;
		ts.dirty_text = true;
		request_update();
	}
}

TextServer::Direction PrimitiveMesh::get_text_direction() const
{
	return _get_text_state().text_direction;
}

void PrimitiveMesh::set_language(const String& p_language)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.language != p_language) {
		ts.language = p_language;
		ts.dirty_text = true;
		request_update();
	}
}

String PrimitiveMesh::get_language() const { return _get_text_state().language; }

void PrimitiveMesh::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.st_parser != p_parser) {
		ts.st_parser = p_parser;
		ts.dirty_text = true;
		request_update();
	}
}

TextServer::StructuredTextParser PrimitiveMesh::get_structured_text_bidi_override() const
{
	return _get_text_state().st_parser;
}

void PrimitiveMesh::set_uppercase(bool p_uppercase)
{
	TextMeshState& ts = _ensure_text_state();
	if (ts.uppercase != p_uppercase) {
		ts.uppercase = p_uppercase;
		ts.dirty_text = true;
		request_update();
	}
}

bool PrimitiveMesh::is_uppercase() const { return _get_text_state().uppercase; }

#ifdef PADDING_REF_SIZE
#undef PADDING_REF_SIZE
#endif

// --- importer_mesh.cpp ---

#include "core/io/marshalls.h"
#include "core/math/random_pcg.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/surface_tool.h"

#ifndef PHYSICS_3D_DISABLED
#include "core/math/geometry_3d.h"
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

Vector<Face3> ImporterMesh::get_faces() const {}

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
		Ref<Shape3D> shape = memnew(Shape3D(Shape3D::SHAPE_CONVEX_POLYGON));
		shape->set_points(decomposed[i]);
		ret.push_back(shape);
	}

	return ret;
}

Ref<Shape3D> ImporterMesh::create_trimesh_shape() const
{
	Vector<Face3> faces = get_faces();
	if (faces.is_empty()) {
		return Ref<Shape3D>();
	}

	Vector<Vector3> face_points;
	face_points.resize(faces.size() * 3);

	for (int i = 0; i < face_points.size(); i += 3) {
		Face3 f = faces.get(i / 3);
		face_points.set(i, f.vertex[0]);
		face_points.set(i + 1, f.vertex[1]);
		face_points.set(i + 2, f.vertex[2]);
	}

	Ref<Shape3D> shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
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

Ref<ArrayMesh> ImporterMesh::get_mesh(Ref<ArrayMesh> const&)
{
	Ref<ArrayMesh> am = memnew(ArrayMesh);
	return am;
}

Ref<ImporterMesh> ImporterMesh::from_mesh(Ref<Mesh> const&)
{
	Ref<ImporterMesh> im = memnew(ImporterMesh);
	return im;
}

#ifdef VERTEX_SKIN_FUNC
#undef VERTEX_SKIN_FUNC
#endif

// --- mesh_library.cpp ---

#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep // Needed to bind RSE enums.

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif // PHYSICS_3D_DISABLED

bool MeshLibrary::_validate_index(int p_idx)
{
	if (unlikely(!item_map.has(p_idx))) {
		if (!init_property) {
			ERR_FAIL_V_MSG(
				false, vformat("Requested for nonexistent MeshLibrary item '%d'.", p_idx));
		}
		create_item(p_idx);
	}

	return true;
}

void MeshLibrary::set_item_name(int p_item, const String& p_name)
{
	if (_validate_index(p_item)) {
		item_map[p_item].name = p_name;
		emit_changed();
	}
}

void MeshLibrary::set_item_category(int p_item, const String& p_category)
{
	if (_validate_index(p_item)) {
		item_map[p_item].category = p_category;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh(int p_item, const Ref<Mesh>& p_mesh)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh = p_mesh;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh_transform(int p_item, const Transform3D& p_transform)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh_transform = p_transform;
		emit_changed();
	}
}

void MeshLibrary::set_item_mesh_cast_shadow(
	int p_item, RSE::ShadowCastingSetting p_shadow_casting_setting)
{
	if (_validate_index(p_item)) {
		item_map[p_item].mesh_cast_shadow = p_shadow_casting_setting;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_mesh(int p_item, const Ref<NavigationMesh>& p_navigation_mesh)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_mesh = p_navigation_mesh;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_mesh_transform(int p_item, const Transform3D& p_transform)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_mesh_transform = p_transform;
		emit_changed();
	}
}

void MeshLibrary::set_item_navigation_layers(int p_item, uint32_t p_navigation_layers)
{
	if (_validate_index(p_item)) {
		item_map[p_item].navigation_layers = p_navigation_layers;
		emit_changed();
	}
}

void MeshLibrary::set_item_preview(int p_item, const Ref<Texture2D>& p_preview)
{
	if (_validate_index(p_item)) {
		item_map[p_item].preview = p_preview;
		emit_changed();
	}
}

String MeshLibrary::get_item_name(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), "",
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].name;
}

String MeshLibrary::get_item_category(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), String(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].category;
}

Ref<Mesh> MeshLibrary::get_item_mesh(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<Mesh>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh;
}

Transform3D MeshLibrary::get_item_mesh_transform(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Transform3D(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh_transform;
}

RSE::ShadowCastingSetting MeshLibrary::get_item_mesh_cast_shadow(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), RSE::ShadowCastingSetting::SHADOW_CASTING_SETTING_ON,
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].mesh_cast_shadow;
}

#ifndef PHYSICS_3D_DISABLED
Vector<MeshLibrary::ShapeData> MeshLibrary::get_item_shapes(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Vector<ShapeData>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].shapes;
}
#endif // PHYSICS_3D_DISABLED

Ref<NavigationMesh> MeshLibrary::get_item_navigation_mesh(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<NavigationMesh>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_mesh;
}

Transform3D MeshLibrary::get_item_navigation_mesh_transform(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Transform3D(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_mesh_transform;
}

uint32_t MeshLibrary::get_item_navigation_layers(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), 0,
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].navigation_layers;
}

Ref<Texture2D> MeshLibrary::get_item_preview(int p_item) const
{
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), Ref<Texture2D>(),
		"Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item].preview;
}

bool MeshLibrary::has_item(int p_item) const { return item_map.has(p_item); }

Vector<int> MeshLibrary::get_item_list() const
{
	Vector<int> ret;
	ret.resize(item_map.size());
	int idx = 0;
	for (const KeyValue<int, Item>& E : item_map) {
		ret.write[idx++] = E.key;
	}

	return ret;
}

int MeshLibrary::find_item_by_name(const String& p_name) const
{
	for (const KeyValue<int, Item>& E : item_map) {
		if (E.value.name == p_name) {
			return E.key;
		}
	}
	return -1;
}

int MeshLibrary::get_last_unused_item_id() const
{
	if (!item_map.size()) {
		return 0;
	}
	else {
		return item_map.back()->key() + 1;
	}
}

void MeshLibrary::reset_state() { clear(); }



void MeshLibrary::create_item(int) {}

void MeshLibrary::clear() {}

// --- skin.cpp ---

#include "scene/resources/3d/skin.h"

void Skin::set_bind_count(int p_size)
{
	ERR_FAIL_COND(p_size < 0);
	binds.resize(p_size);
	binds_ptr = binds.ptrw();
	bind_count = p_size;
	emit_changed();
}

void Skin::add_bind(int p_bone, const Transform3D& p_pose)
{
	uint32_t index = bind_count;
	set_bind_count(bind_count + 1);
	set_bind_bone(index, p_bone);
	set_bind_pose(index, p_pose);
}

void Skin::add_named_bind(const String& p_name, const Transform3D& p_pose)
{
	uint32_t index = bind_count;
	set_bind_count(bind_count + 1);
	set_bind_name(index, p_name);
	set_bind_pose(index, p_pose);
}

void Skin::set_bind_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, bind_count);
	binds_ptr[p_index].bone = p_bone;
	emit_changed();
}

void Skin::set_bind_pose(int p_index, const Transform3D& p_pose)
{
	ERR_FAIL_INDEX(p_index, bind_count);
	binds_ptr[p_index].pose = p_pose;
	emit_changed();
}

void Skin::clear_binds()
{
	binds.clear();
	binds_ptr = nullptr;
	bind_count = 0;
	emit_changed();
}

void Skin::reset_state() { clear_binds(); }

void Skin::set_bind_name(int p_index, const String& p_name) {}

// ============================================================================
// Merged from scene/resources/3d/sky_material.cpp
// ============================================================================
// --- Inlined from scene/resources/3d/sky_material.h ---
#include "core/templates/rid.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

class SkyMaterial final : public Material
{
public:
	enum Mode
	{
		MODE_PROCEDURAL,
		MODE_PANORAMA,
		MODE_PHYSICAL,
		MODE_FOG,
	};

private:
	Mode mode = MODE_PROCEDURAL;

	// Procedural sky properties
	Color sky_top_color = Color(0.385, 0.454, 0.55);
	Color sky_horizon_color = Color(0.6463, 0.6558, 0.6708);
	float sky_curve = 0.15f;
	float sky_energy_multiplier = 1.0f;
	Ref<Texture2D> sky_cover;
	Color sky_cover_modulate = Color(1, 1, 1);

	Color ground_bottom_color = Color(0.2, 0.169, 0.133);
	Color ground_horizon_color = Color(0.6463, 0.6558, 0.6708);
	float ground_curve = 0.02f;
	float ground_energy_multiplier = 1.0f;

	float sun_angle_max = 30.0f;
	float sun_curve = 0.15f;

	// Shared sky properties
	bool use_debanding = true;
	float energy_multiplier = 1.0f;

	// Panorama sky properties
	Ref<Texture2D> panorama;
	bool filter = true;

	// Physical sky properties
	float rayleigh = 2.0f;
	Color rayleigh_color = Color(0.3, 0.405, 0.6);
	float mie = 0.005f;
	float mie_eccentricity = 0.8f;
	Color mie_color = Color(0.69, 0.729, 0.812);
	float turbidity = 10.0f;
	float sun_disk_scale = 1.0f;
	Color ground_color = Color(0.1, 0.07, 0.034);
	Ref<Texture2D> night_sky;

	// Fog properties
	float density = 1.0f;
	Color albedo = Color(1, 1, 1, 1);
	Color emission = Color(0, 0, 0, 1);
	float height_falloff = 0.0f;
	float edge_fade = 0.1f;
	Ref<Texture3D> density_texture;

	static Mutex shader_mutex;
	static RID procedural_shader_cache[4];
	static RID panorama_shader_cache[2];
	static RID physical_shader_cache[4];
	static RID fog_shader;

	static void _update_procedural_shader(bool p_use_debanding, bool p_use_sky_cover);
	static void _update_panorama_shader(bool p_filter);
	static void _update_physical_shader(bool p_use_debanding, bool p_use_night_sky);
	static void _update_fog_shader();

	RID _get_procedural_shader_cache() const;
	RID _get_physical_shader_cache() const;

	mutable bool shader_set = false;

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }

	// Procedural sky
	void set_sky_top_color(const Color& p_sky_top);
	Color get_sky_top_color() const;

	void set_sky_horizon_color(const Color& p_sky_horizon);
	Color get_sky_horizon_color() const;

	void set_sky_curve(float p_curve);
	float get_sky_curve() const;

	void set_sky_energy_multiplier(float p_multiplier);
	float get_sky_energy_multiplier() const;

	void set_sky_cover(const Ref<Texture2D>& p_sky_cover);
	Ref<Texture2D> get_sky_cover() const;

	void set_sky_cover_modulate(const Color& p_sky_cover_modulate);
	Color get_sky_cover_modulate() const;

	void set_ground_bottom_color(const Color& p_ground_bottom);
	Color get_ground_bottom_color() const;

	void set_ground_horizon_color(const Color& p_ground_horizon);
	Color get_ground_horizon_color() const;

	void set_ground_curve(float p_curve);
	float get_ground_curve() const;

	void set_ground_energy_multiplier(float p_energy);
	float get_ground_energy_multiplier() const;

	void set_sun_angle_max(float p_angle);
	float get_sun_angle_max() const;

	void set_sun_curve(float p_curve);
	float get_sun_curve() const;

	// Shared sky
	void set_use_debanding(bool p_use_debanding);
	bool get_use_debanding() const;

	void set_energy_multiplier(float p_multiplier);
	float get_energy_multiplier() const;

	void set_exposure_value(float p_exposure);
	float get_exposure_value() const;

	// Panorama sky
	void set_panorama(const Ref<Texture2D>& p_panorama);
	Ref<Texture2D> get_panorama() const;

	void set_filtering_enabled(bool p_enabled);
	bool is_filtering_enabled() const;

	// Physical sky
	void set_rayleigh_coefficient(float p_rayleigh);
	float get_rayleigh_coefficient() const;

	void set_rayleigh_color(Color p_rayleigh_color);
	Color get_rayleigh_color() const;

	void set_turbidity(float p_turbidity);
	float get_turbidity() const;

	void set_mie_coefficient(float p_mie);
	float get_mie_coefficient() const;

	void set_mie_eccentricity(float p_eccentricity);
	float get_mie_eccentricity() const;

	void set_mie_color(Color p_mie_color);
	Color get_mie_color() const;

	void set_sun_disk_scale(float p_sun_disk_scale);
	float get_sun_disk_scale() const;

	void set_ground_color(Color p_ground_color);
	Color get_ground_color() const;

	void set_night_sky(const Ref<Texture2D>& p_night_sky);
	Ref<Texture2D> get_night_sky() const;

	// Fog
	void set_density(float p_density);
	float get_density() const;

	void set_albedo(Color p_color);
	Color get_albedo() const;

	void set_emission(Color p_color);
	Color get_emission() const;

	void set_height_falloff(float p_falloff);
	float get_height_falloff() const;

	void set_edge_fade(float p_edge_fade);
	float get_edge_fade() const;

	void set_density_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_density_texture() const;

	virtual Shader::Mode get_shader_mode() const override;
	virtual RID get_shader_rid() const override;
	virtual RID get_rid() const override;

	static void cleanup_shader();

	SkyMaterial(Mode p_mode = MODE_PROCEDURAL);
	virtual ~SkyMaterial();
};

// --- End scene/resources/3d/sky_material.h ---

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/version.h"
#include "servers/rendering/renderer.h"

Mutex SkyMaterial::shader_mutex;
RID SkyMaterial::procedural_shader_cache[4];
RID SkyMaterial::panorama_shader_cache[2];
RID SkyMaterial::physical_shader_cache[4];
RID SkyMaterial::fog_shader;

void SkyMaterial::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	if (shader_set) {
		shader_set = false;
		Renderer::material_set_shader(_get_material(), get_shader_rid());
		shader_set = true;
	}
}

// Procedural sky

void SkyMaterial::set_sky_top_color(const Color& p_sky_top) { sky_top_color = p_sky_top; }

Color SkyMaterial::get_sky_top_color() const { return sky_top_color; }

void SkyMaterial::set_sky_horizon_color(const Color& p_sky_horizon)
{
	sky_horizon_color = p_sky_horizon;
}

Color SkyMaterial::get_sky_horizon_color() const { return sky_horizon_color; }

void SkyMaterial::set_sky_curve(float p_curve) { sky_curve = p_curve; }

float SkyMaterial::get_sky_curve() const { return sky_curve; }

void SkyMaterial::set_sky_energy_multiplier(float p_multiplier)
{
	sky_energy_multiplier = p_multiplier;
}

float SkyMaterial::get_sky_energy_multiplier() const { return sky_energy_multiplier; }

void SkyMaterial::set_sky_cover(const Ref<Texture2D>& p_sky_cover)
{
	sky_cover = p_sky_cover;
	if (mode == MODE_PROCEDURAL) {
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_procedural_shader_cache());
		}
	}
}

Ref<Texture2D> SkyMaterial::get_sky_cover() const { return sky_cover; }

void SkyMaterial::set_sky_cover_modulate(const Color& p_sky_cover_modulate)
{
	sky_cover_modulate = p_sky_cover_modulate;
}

Color SkyMaterial::get_sky_cover_modulate() const { return sky_cover_modulate; }

void SkyMaterial::set_ground_bottom_color(const Color& p_ground_bottom)
{
	ground_bottom_color = p_ground_bottom;
}

Color SkyMaterial::get_ground_bottom_color() const { return ground_bottom_color; }

void SkyMaterial::set_ground_horizon_color(const Color& p_ground_horizon)
{
	ground_horizon_color = p_ground_horizon;
}

Color SkyMaterial::get_ground_horizon_color() const { return ground_horizon_color; }

void SkyMaterial::set_ground_curve(float p_curve) { ground_curve = p_curve; }

float SkyMaterial::get_ground_curve() const { return ground_curve; }

void SkyMaterial::set_ground_energy_multiplier(float p_energy)
{
	ground_energy_multiplier = p_energy;
}

float SkyMaterial::get_ground_energy_multiplier() const { return ground_energy_multiplier; }

void SkyMaterial::set_sun_angle_max(float p_angle) { sun_angle_max = p_angle; }

float SkyMaterial::get_sun_angle_max() const { return sun_angle_max; }

void SkyMaterial::set_sun_curve(float p_curve) { sun_curve = p_curve; }

float SkyMaterial::get_sun_curve() const { return sun_curve; }

// Shared sky

void SkyMaterial::set_use_debanding(bool p_use_debanding)
{
	use_debanding = p_use_debanding;
	if (mode == MODE_PROCEDURAL) {
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_procedural_shader_cache());
		}
	}
	else if (mode == MODE_PHYSICAL) {
		_update_physical_shader(use_debanding, night_sky.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_physical_shader_cache());
		}
	}
}

bool SkyMaterial::get_use_debanding() const { return use_debanding; }

void SkyMaterial::set_energy_multiplier(float p_multiplier) { energy_multiplier = p_multiplier; }

float SkyMaterial::get_energy_multiplier() const { return energy_multiplier; }

void SkyMaterial::set_exposure_value(float p_exposure)
{
	set_energy_multiplier(Math::pow(2.0f, p_exposure));
}

float SkyMaterial::get_exposure_value() const
{
	return Math::log(get_energy_multiplier()) / Math::log(2.0f);
}

// Panorama sky

void SkyMaterial::set_panorama(const Ref<Texture2D>& p_panorama) { panorama = p_panorama; }

Ref<Texture2D> SkyMaterial::get_panorama() const { return panorama; }

void SkyMaterial::set_filtering_enabled(bool p_enabled)
{
	filter = p_enabled;
	if (mode == MODE_PANORAMA) {
		_update_panorama_shader(filter);
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), panorama_shader_cache[int(filter)]);
		}
	}
}

bool SkyMaterial::is_filtering_enabled() const { return filter; }

// Physical sky

void SkyMaterial::set_rayleigh_coefficient(float p_rayleigh) { rayleigh = p_rayleigh; }

float SkyMaterial::get_rayleigh_coefficient() const { return rayleigh; }

void SkyMaterial::set_rayleigh_color(Color p_rayleigh_color) { rayleigh_color = p_rayleigh_color; }

Color SkyMaterial::get_rayleigh_color() const { return rayleigh_color; }

void SkyMaterial::set_turbidity(float p_turbidity) { turbidity = p_turbidity; }

float SkyMaterial::get_turbidity() const { return turbidity; }

void SkyMaterial::set_mie_coefficient(float p_mie) { mie = p_mie; }

float SkyMaterial::get_mie_coefficient() const { return mie; }

void SkyMaterial::set_mie_eccentricity(float p_eccentricity) { mie_eccentricity = p_eccentricity; }

float SkyMaterial::get_mie_eccentricity() const { return mie_eccentricity; }

void SkyMaterial::set_mie_color(Color p_mie_color) { mie_color = p_mie_color; }

Color SkyMaterial::get_mie_color() const { return mie_color; }

void SkyMaterial::set_sun_disk_scale(float p_sun_disk_scale) { sun_disk_scale = p_sun_disk_scale; }

float SkyMaterial::get_sun_disk_scale() const { return sun_disk_scale; }

void SkyMaterial::set_ground_color(Color p_ground_color) { ground_color = p_ground_color; }

Color SkyMaterial::get_ground_color() const { return ground_color; }

void SkyMaterial::set_night_sky(const Ref<Texture2D>& p_night_sky)
{
	night_sky = p_night_sky;
	if (mode == MODE_PHYSICAL) {
		_update_physical_shader(use_debanding, night_sky.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_physical_shader_cache());
		}
	}
}

Ref<Texture2D> SkyMaterial::get_night_sky() const { return night_sky; }

// Fog

void SkyMaterial::set_density(float p_density) { density = p_density; }

float SkyMaterial::get_density() const { return density; }

void SkyMaterial::set_albedo(Color p_color) { albedo = p_color; }

Color SkyMaterial::get_albedo() const { return albedo; }

void SkyMaterial::set_emission(Color p_color) { emission = p_color; }

Color SkyMaterial::get_emission() const { return emission; }

void SkyMaterial::set_height_falloff(float p_falloff) { height_falloff = p_falloff; }

float SkyMaterial::get_height_falloff() const { return height_falloff; }

void SkyMaterial::set_edge_fade(float p_edge_fade) { edge_fade = p_edge_fade; }

float SkyMaterial::get_edge_fade() const { return edge_fade; }

void SkyMaterial::set_density_texture(const Ref<Texture3D>& p_texture)
{
	density_texture = p_texture;
}

Ref<Texture3D> SkyMaterial::get_density_texture() const { return density_texture; }

// Shader management

Shader::Mode SkyMaterial::get_shader_mode() const
{
	return mode == MODE_FOG ? Shader::MODE_FOG : Shader::MODE_SKY;
}

RID SkyMaterial::_get_procedural_shader_cache() const
{
	return procedural_shader_cache[int(use_debanding) + (sky_cover.is_valid() ? 2 : 0)];
}

RID SkyMaterial::_get_physical_shader_cache() const
{
	return physical_shader_cache[int(use_debanding) + (night_sky.is_valid() ? 2 : 0)];
}

RID SkyMaterial::get_shader_rid() const
{
	switch (mode) {
	case MODE_PROCEDURAL:
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		return _get_procedural_shader_cache();
	case MODE_PANORAMA:
		_update_panorama_shader(filter);
		return panorama_shader_cache[int(filter)];
	case MODE_PHYSICAL:
		_update_physical_shader(use_debanding, night_sky.is_valid());
		return _get_physical_shader_cache();
	case MODE_FOG:
		_update_fog_shader();
		return fog_shader;
	}
	return RID();
}

RID SkyMaterial::get_rid() const
{
	RID shader_rid = get_shader_rid();
	if (!shader_set) {
		Renderer::material_set_shader(_get_material(), shader_rid);
		shader_set = true;
	}
	return _get_material();
}

void SkyMaterial::cleanup_shader()
{
	for (int i = 0; i < 4; i++) {
		if (procedural_shader_cache[i].is_valid()) {
			Renderer::free_rid(procedural_shader_cache[i]);
			procedural_shader_cache[i] = RID();
		}
	}
	for (int i = 0; i < 2; i++) {
		if (panorama_shader_cache[i].is_valid()) {
			Renderer::free_rid(panorama_shader_cache[i]);
			panorama_shader_cache[i] = RID();
		}
	}
	for (int i = 0; i < 4; i++) {
		if (physical_shader_cache[i].is_valid()) {
			Renderer::free_rid(physical_shader_cache[i]);
			physical_shader_cache[i] = RID();
		}
	}
	if (fog_shader.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(fog_shader);
		fog_shader = RID();
	}
}

void SkyMaterial::_update_procedural_shader(bool p_use_debanding, bool p_use_sky_cover)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_sky_cover) * 2;
	if (procedural_shader_cache[index].is_null()) {
		procedural_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(procedural_shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s SkyMaterial (Procedural).

shader_type sky;
%s

uniform vec4 sky_top_color : source_color = vec4(0.385, 0.454, 0.55, 1.0);
uniform vec4 sky_horizon_color : source_color = vec4(0.646, 0.656, 0.67, 1.0);
uniform float inv_sky_curve : hint_range(1, 100) = 4.0;
uniform vec4 ground_bottom_color : source_color = vec4(0.2, 0.169, 0.133, 1.0);
uniform vec4 ground_horizon_color : source_color = vec4(0.646, 0.656, 0.67, 1.0);
uniform float inv_ground_curve : hint_range(1, 100) = 30.0;
uniform float sun_angle_max = 0.877;
uniform float inv_sun_curve : hint_range(1, 100) = 22.78;
uniform float exposure : hint_range(0, 128) = 1.0;

uniform sampler2D sky_cover : filter_linear, source_color, hint_default_black;
uniform vec4 sky_cover_modulate : source_color = vec4(1.0, 1.0, 1.0, 1.0);

void sky() {
	float v_angle = clamp(EYEDIR.y, -1.0, 1.0);
	vec3 sky = mix(sky_top_color.rgb, sky_horizon_color.rgb, clamp(pow(1.0 - v_angle, inv_sky_curve), 0.0, 1.0));

	if (LIGHT0_ENABLED) {
		float sun_angle = dot(LIGHT0_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT0_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT0_COLOR * LIGHT0_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT0_COLOR * LIGHT0_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT1_ENABLED) {
		float sun_angle = dot(LIGHT1_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT1_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT1_COLOR * LIGHT1_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT1_COLOR * LIGHT1_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT2_ENABLED) {
		float sun_angle = dot(LIGHT2_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT2_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT2_COLOR * LIGHT2_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT2_COLOR * LIGHT2_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT3_ENABLED) {
		float sun_angle = dot(LIGHT3_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT3_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT3_COLOR * LIGHT3_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT3_COLOR * LIGHT3_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	%s
	%s
	vec3 ground = mix(ground_bottom_color.rgb, ground_horizon_color.rgb, clamp(pow(1.0 + v_angle, inv_ground_curve), 0.0, 1.0));

	COLOR = mix(ground, sky, step(0.0, EYEDIR.y)) * exposure;
}
)",
				p_use_debanding ? "render_mode use_debanding;" : "",
				p_use_sky_cover ? "vec4 sky_cover_texture = texture(sky_cover, SKY_COORDS);" : "",
				p_use_sky_cover ? "sky += (sky_cover_texture.rgb * sky_cover_modulate.rgb) * "
								  "sky_cover_texture.a * sky_cover_modulate.a;"
								: ""));
	}
}

void SkyMaterial::_update_panorama_shader(bool p_filter)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_filter);
	if (panorama_shader_cache[index].is_null()) {
		panorama_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(
			panorama_shader_cache[index], vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
												  R"('s SkyMaterial (Panorama).

shader_type sky;

uniform sampler2D source_panorama : %s, source_color, hint_default_black;
uniform float exposure : hint_range(0, 128) = 1.0;

void sky() {
	COLOR = texture(source_panorama, SKY_COORDS).rgb * exposure;
}
)",
											  p_filter ? "filter_linear" : "filter_nearest"));
	}
}

void SkyMaterial::_update_physical_shader(bool p_use_debanding, bool p_use_night_sky)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_night_sky) * 2;
	if (physical_shader_cache[index].is_null()) {
		physical_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(physical_shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s SkyMaterial (Physical).

shader_type sky;
%s

uniform float rayleigh : hint_range(0, 64) = 2.0;
uniform vec4 rayleigh_color : source_color = vec4(0.3, 0.405, 0.6, 1.0);
uniform float mie : hint_range(0, 1) = 0.005;
uniform float mie_eccentricity : hint_range(-1, 1) = 0.8;
uniform vec4 mie_color : source_color = vec4(0.69, 0.729, 0.812, 1.0);

uniform float turbidity : hint_range(0, 1000) = 10.0;
uniform float sun_disk_scale : hint_range(0, 360) = 1.0;
uniform vec4 ground_color : source_color = vec4(0.1, 0.07, 0.034, 1.0);
uniform float exposure : hint_range(0, 128) = 1.0;

uniform sampler2D night_sky : filter_linear, source_color, hint_default_black;

const vec3 UP = vec3( 0.0, 1.0, 0.0 );

// Optical length at zenith for molecules.
const float rayleigh_zenith_size = 8.4e3;
const float mie_zenith_size = 1.25e3;

float henyey_greenstein(float cos_theta, float g) {
	const float k = 0.0795774715459;
	return k * (1.0 - g * g) / (pow(1.0 + g * g - 2.0 * g * cos_theta, 1.5));
}

void sky() {
	if (LIGHT0_ENABLED) {
		float zenith_angle = clamp( dot(UP, normalize(LIGHT0_DIRECTION)), -1.0, 1.0 );
		float sun_energy = max(0.0, 0.757 * zenith_angle) * LIGHT0_ENERGY;
		float sun_fade = 1.0 - clamp(1.0 - exp(LIGHT0_DIRECTION.y), 0.0, 1.0);

		// Rayleigh coefficients.
		float rayleigh_coefficient = rayleigh - ( 1.0 * ( 1.0 - sun_fade ) );
		vec3 rayleigh_beta = rayleigh_coefficient * rayleigh_color.rgb * 0.0001;
		// mie coefficients from Preetham
		vec3 mie_beta = turbidity * mie * mie_color.rgb * 0.000434;

		// Optical length.
		float zenith = max(0.0, dot(UP, EYEDIR));
		float optical_mass = 1.0 / (zenith + 0.15 * pow(3.885 + 54.5 * zenith, -1.253));
		float rayleigh_scatter = rayleigh_zenith_size * optical_mass;
		float mie_scatter = mie_zenith_size * optical_mass;

		// Light extinction based on thickness of atmosphere.
		vec3 extinction = exp(-(rayleigh_beta * rayleigh_scatter + mie_beta * mie_scatter));

		// In scattering.
		float cos_theta = dot(EYEDIR, normalize(LIGHT0_DIRECTION));

		float rayleigh_phase = (3.0 / (16.0 * PI)) * (1.0 + pow(cos_theta * 0.5 + 0.5, 2.0));
		vec3 betaRTheta = rayleigh_beta * rayleigh_phase;

		float mie_phase = henyey_greenstein(cos_theta, mie_eccentricity);
		vec3 betaMTheta = mie_beta * mie_phase;

		vec3 Lin = pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * (1.0 - extinction), vec3(1.5));
		// Hack from https://github.com/mrdoob/three.js/blob/master/examples/jsm/objects/Sky.js
		Lin *= mix(vec3(1.0), pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * extinction, vec3(0.5)), clamp(pow(1.0 - zenith_angle, 5.0), 0.0, 1.0));

		// Hack in the ground color.
		Lin  *= mix(ground_color.rgb, vec3(1.0), smoothstep(-0.1, 0.1, dot(UP, EYEDIR)));

		// Solar disk and out-scattering.
		float sunAngularDiameterCos = cos(LIGHT0_SIZE * sun_disk_scale);
		float sunAngularDiameterCos2 = cos(LIGHT0_SIZE * sun_disk_scale * 0.5);
		float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos2, cos_theta);
		vec3 L0 = (sun_energy * extinction) * sundisk * LIGHT0_COLOR;
		%s

		vec3 color = Lin + L0;
		COLOR = pow(color, vec3(1.0 / (1.2 + (1.2 * sun_fade))));
		COLOR *= exposure;
	}
	else {
		// There is no sun, so display night_sky and nothing else.
		%s
		COLOR *= exposure;
	}
}
)",
				p_use_debanding ? "render_mode use_debanding;" : "",
				p_use_night_sky ? "L0 += texture(night_sky, SKY_COORDS).xyz * extinction;" : "",
				p_use_night_sky ? "COLOR = texture(night_sky, SKY_COORDS).xyz;" : ""));
	}
}

void SkyMaterial::_update_fog_shader()
{
	MutexLock shader_lock(shader_mutex);
	if (fog_shader.is_null()) {
		fog_shader = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(fog_shader, R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
											  R"('s SkyMaterial (Fog).

shader_type fog;

uniform float density : hint_range(0, 1, 0.0001) = 1.0;
uniform vec4 albedo : source_color = vec4(1.0);
uniform vec4 emission : source_color = vec4(0, 0, 0, 1);
uniform float height_falloff = 0.0;
uniform float edge_fade = 0.1;
uniform sampler3D density_texture: hint_default_white;


void fog() {
    DENSITY = density * clamp(exp2(-height_falloff * (WORLD_POSITION.y - OBJECT_POSITION.y)), 0.0, 1.0);
    DENSITY *= texture(density_texture, UVW).r;
    DENSITY *= pow(clamp(-2.0 * SDF / min(min(SIZE.x, SIZE.y), SIZE.z), 0.0, 1.0), edge_fade);
    ALBEDO = albedo.rgb;
    EMISSION = emission.rgb;
}
)");
	}
}

SkyMaterial::SkyMaterial(Mode p_mode) : mode(p_mode)
{
	_set_material(Renderer::material_create());

	set_sky_top_color(Color(0.385, 0.454, 0.55));
	set_sky_horizon_color(Color(0.6463, 0.6558, 0.6708));
	set_sky_curve(0.15);
	set_sky_energy_multiplier(1.0);
	set_sky_cover_modulate(Color(1, 1, 1));

	set_ground_bottom_color(Color(0.2, 0.169, 0.133));
	set_ground_horizon_color(Color(0.6463, 0.6558, 0.6708));
	set_ground_curve(0.02);
	set_ground_energy_multiplier(1.0);

	set_sun_angle_max(30.0);
	set_sun_curve(0.15);

	set_rayleigh_coefficient(2.0);
	set_rayleigh_color(Color(0.3, 0.405, 0.6));
	set_mie_coefficient(0.005);
	set_mie_eccentricity(0.8);
	set_mie_color(Color(0.69, 0.729, 0.812));
	set_turbidity(10.0);
	set_sun_disk_scale(1.0);
	set_ground_color(Color(0.1, 0.07, 0.034));

	set_density(1.0);
	set_albedo(Color(1, 1, 1, 1));
	set_emission(Color(0, 0, 0, 1));
	set_height_falloff(0.0);
	set_edge_fade(0.1);

	set_energy_multiplier(1.0);
	if (mode == MODE_PROCEDURAL || mode == MODE_PHYSICAL) {
		set_use_debanding(true);
	}
}

SkyMaterial::~SkyMaterial()
{
	if (mode == MODE_FOG) {
		Renderer::material_set_shader(_get_material(), RID());
	}
}

// --- joint_limitation_3d.cpp ---

#include "scene/3d/skeleton_3d.h"

#include "core/templates/local_vector.h"

void JointLimitation3D::set_angle(real_t p_angle)
{
	angle = p_angle;
	emit_changed();
}

real_t JointLimitation3D::get_angle() const { return angle; }

Quaternion JointLimitation3D::make_space(const Vector3& p_local_forward_vector,
	const Vector3& p_local_right_vector, const Quaternion& p_rotation_offset) const
{
	const double ALMOST_ONE = 1.0 - CMP_EPSILON;
	// The default is to interpret the forward vector as the +Y axis.
	Vector3 axis_y = p_local_forward_vector.normalized();
	Vector3 axis_x = p_local_right_vector.normalized();
	if (axis_x.is_zero_approx() || Math::abs(axis_x.dot(axis_y)) > ALMOST_ONE) {
		return (Quaternion(Vector3(0, 1, 0), axis_y) * p_rotation_offset.normalized()).normalized();
	}
	// Prior X axis.
	Vector3 axis_z = axis_x.cross(axis_y);
	axis_z.normalize();
	axis_x = axis_y.cross(axis_z);
	axis_x.normalize();
	return (Basis(axis_x, axis_y, axis_z).get_rotation_quaternion() * p_rotation_offset.normalized())
		.normalized();
}

Vector3 JointLimitation3D::_solve(const Vector3& p_direction) const
{
	// Assume the central (forward of the cone) axis is the +Y.
	// This is based on the coordinate system set by JointLimitation3D::make_space().
	Vector3 center_axis = Vector3(0, 1, 0);

	real_t current_angle = p_direction.angle_to(center_axis);
	real_t max_angle = angle * 0.5;

	if (current_angle <= max_angle) {
		return p_direction;
	}

	Vector3 plane_normal;
	if (Math::is_equal_approx((double)current_angle, Math::PI)) {
		plane_normal = center_axis.get_any_perpendicular();
	}
	else {
		plane_normal = center_axis.cross(p_direction).normalized();
	}

	Quaternion rotation = Quaternion(plane_normal, max_angle);
	Vector3 limited_dir = rotation.xform(center_axis);

	Vector3 projection = p_direction - center_axis * p_direction.dot(center_axis);
	if (projection.length_squared() > CMP_EPSILON) {
		Vector3 side_dir = projection.normalized();
		Quaternion side_rotation = Quaternion(center_axis.cross(side_dir).normalized(), max_angle);
		limited_dir = side_rotation.xform(center_axis);
	}

	return limited_dir.normalized();
}

Vector3 JointLimitation3D::solve(const Vector3& p_local_forward_vector,
	const Vector3& p_local_right_vector, const Quaternion& p_rotation_offset,
	const Vector3& p_local_current_vector) const
{
	Quaternion space = make_space(p_local_forward_vector, p_local_right_vector, p_rotation_offset);
	Vector3 dir = p_local_current_vector.normalized();
	return space.xform(_solve(space.xform_inv(dir)));
}

#ifdef TOOLS_ENABLED
void JointLimitation3D::draw_shape(Ref<SurfaceTool>& p_surface_tool, const Transform3D& p_transform,
	float p_bone_length, const Color& p_color) const
{
	static const int N = 16;
	static const real_t DP = Math::TAU / (real_t)N;

	real_t sphere_r = p_bone_length * (real_t)0.25;
	if (sphere_r <= CMP_EPSILON) {
		return;
	}
	real_t alpha = CLAMP((real_t)angle, (real_t)0.0, (real_t)Math::TAU) * 0.5;
	real_t y_cap = sphere_r * Math::cos(alpha);
	real_t r_cap = sphere_r * Math::sin(alpha);

	LocalVector<Vector3> vts;

	// Cone bottom.
	if (r_cap > CMP_EPSILON) {
		for (int i = 0; i < N; i++) {
			real_t a0 = (real_t)i * DP;
			real_t a1 = (real_t)((i + 1) % N) * DP;
			Vector3 p0 = Vector3(r_cap * Math::cos(a0), y_cap, r_cap * Math::sin(a0));
			Vector3 p1 = Vector3(r_cap * Math::cos(a1), y_cap, r_cap * Math::sin(a1));
			vts.push_back(p0);
			vts.push_back(p1);
		}
	}

	// Rotate arcs around Y-axis.
	real_t t_start;
	real_t arc_len;
	if (alpha <= (real_t)1e-6) {
		t_start = (real_t)0.5 * Math::PI;
		arc_len = Math::PI;
	}
	else {
		t_start = (real_t)0.5 * Math::PI + alpha;
		arc_len = Math::PI - alpha;
	}
	real_t dt = arc_len / (real_t)N;

	for (int k = 0; k < N; k++) {
		Basis ry(Vector3(0, 1, 0), (real_t)k * DP);

		Vector3 prev =
			ry.xform(Vector3(sphere_r * Math::cos(t_start), sphere_r * Math::sin(t_start), 0));

		for (int s = 1; s <= N; s++) {
			real_t t = t_start + dt * (real_t)s;
			Vector3 cur = ry.xform(Vector3(sphere_r * Math::cos(t), sphere_r * Math::sin(t), 0));

			vts.push_back(prev);
			vts.push_back(cur);

			prev = cur;
		}

		Vector3 mouth =
			ry.xform(Vector3(sphere_r * Math::cos(t_start), sphere_r * Math::sin(t_start), 0));
		Vector3 center = Vector3();

		vts.push_back(center);
		vts.push_back(mouth);
	}

	// Stack rings.
	for (int i = 1; i <= 3; i++) {
		for (int sgn = -1; sgn <= 1; sgn += 2) {
			real_t y = (real_t)sgn * sphere_r * ((real_t)i / (real_t)4.0);
			if (y >= y_cap - CMP_EPSILON) {
				continue;
			}
			real_t ring_r2 = sphere_r * sphere_r - y * y;
			if (ring_r2 <= (real_t)0.0) {
				continue;
			}
			real_t ring_r = Math::sqrt(ring_r2);

			for (int j = 0; j < N; j++) {
				real_t a0 = (real_t)j * DP;
				real_t a1 = (real_t)((j + 1) % N) * DP;
				Vector3 p0 = Vector3(ring_r * Math::cos(a0), y, ring_r * Math::sin(a0));
				Vector3 p1 = Vector3(ring_r * Math::cos(a1), y, ring_r * Math::sin(a1));

				vts.push_back(p0);
				vts.push_back(p1);
			}
		}
	}

	for (int64_t i = 0; i < vts.size(); i++) {
		p_surface_tool->set_color(p_color);
		p_surface_tool->add_vertex(p_transform.xform(vts[i]));
	}
}
#endif // TOOLS_ENABLED

// --- world_3d.cpp ---

#include "core/config/project_settings.h"
#include "scene/3d/camera_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"
#include "scene/3d/node_3d.h"

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

void World3D::_register_camera(Camera3D* p_camera) { cameras.insert(p_camera); }

void World3D::_remove_camera(Camera3D* p_camera) { cameras.erase(p_camera); }

RID World3D::get_scenario() const { return scenario; }

void World3D::set_environment(const Ref<Environment>& p_environment)
{
	if (environment == p_environment) {
		return;
	}

	environment = p_environment;
	if (environment.is_valid()) {
		Renderer::scenario_set_environment(scenario, environment->get_rid());
	}
	else {
		Renderer::scenario_set_environment(scenario, RID());
	}

	emit_changed();
}

Ref<Environment> World3D::get_environment() const { return environment; }

void World3D::set_fallback_environment(const Ref<Environment>& p_environment)
{
	if (fallback_environment == p_environment) {
		return;
	}

	fallback_environment = p_environment;
	if (fallback_environment.is_valid()) {
		Renderer::scenario_set_fallback_environment(scenario, p_environment->get_rid());
	}
	else {
		Renderer::scenario_set_fallback_environment(scenario, RID());
	}

	emit_changed();
}

Ref<Environment> World3D::get_fallback_environment() const { return fallback_environment; }

void World3D::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;
	if (camera_attributes.is_valid()) {
		Renderer::scenario_set_camera_attributes(scenario, camera_attributes->get_rid());
	}
	else {
		Renderer::scenario_set_camera_attributes(scenario, RID());
	}
}

Ref<CameraAttributes> World3D::get_camera_attributes() const { return camera_attributes; }

void World3D::set_compositor(const Ref<Compositor>& p_compositor)
{
	compositor = p_compositor;
	if (compositor.is_valid()) {
		Renderer::scenario_set_compositor(scenario, compositor->get_rid());
	}
	else {
		Renderer::scenario_set_compositor(scenario, RID());
	}
}

Ref<Compositor> World3D::get_compositor() const { return compositor; }

#ifndef PHYSICS_3D_DISABLED
PhysicsDirectSpaceState3D* World3D::get_direct_space_state()
{
	return PhysicsServer3D::space_get_direct_state(get_space());
}
#endif // PHYSICS_3D_DISABLED


World3D::World3D() { scenario = Renderer::scenario_create(); }

World3D::~World3D()
{
	ERR_FAIL_NULL(Renderer::data);

#ifndef NAVIGATION_3D_DISABLED
	ERR_FAIL_NULL(NavigationServer3D::data);
#endif // NAVIGATION_3D_DISABLED

	Renderer::free_rid(scenario);

#ifndef PHYSICS_3D_DISABLED
	if (space.is_valid()) {
		ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
		PhysicsServer3D::free_rid(space);
	}
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	if (navigation_map.is_valid()) {
		NavigationServer3D::free_rid(navigation_map);
	}
#endif // NAVIGATION_3D_DISABLED
}

#ifndef NAVIGATION_3D_DISABLED
RID World3D::get_navigation_map() const
{
	return RID();
}
#endif // NAVIGATION_3D_DISABLED

RID World3D::get_space() const
{
	return RID();
}
#endif // !defined(_3D_DISABLED)
