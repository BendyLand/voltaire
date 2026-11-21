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

#include "immediate_mesh.h"
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

#include "mesh_data_tool.h"

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
