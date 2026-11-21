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

#include "core/math/convex_hull.h"
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

#include "multimesh.h"
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
#include "core/io/missing_resource.h"
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
#include "core/io/missing_resource.h"
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
