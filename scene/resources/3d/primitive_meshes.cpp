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
