/**************************************************************************/
/*  primitive_meshes.cpp                                                  */
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

#include "primitive_meshes.h"

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
	static const TextMeshState default_state;
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

#include "mesh_library.h"
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

#include "skin.h"

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
