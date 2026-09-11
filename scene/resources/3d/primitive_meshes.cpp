/**************************************************************************/
/*  primitive_meshes.cpp                                                  */
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

#include <thirdparty/misc/polypartition.h>
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/main_loop.h"
#include "primitive_meshes.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_enums.h"

#define PADDING_REF_SIZE 1024.0

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

StringName PrimitiveMesh::get_blend_shape_name(int p_index) const { return StringName(); }

void PrimitiveMesh::set_blend_shape_name(int p_index, const StringName& p_name) {}

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

Ref<Material> PrimitiveMesh::get_material() const { return material; }

void PrimitiveMesh::set_custom_aabb(const AABB& p_custom)
{
	if (p_custom.is_equal_approx(custom_aabb)) {
		return;
	}
	custom_aabb = p_custom;
	RS::get_singleton()->mesh_set_custom_aabb(mesh, custom_aabb);
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

	// Inverse it to turn our margin into a scale
	uv2_scale = Vector2(1.0, 1.0) - uv2_scale;

	return uv2_scale;
}

float PrimitiveMesh::get_lightmap_texel_size() const { return texel_size; }

void CapsuleMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		float radial_length = radius * Math::PI * 0.5; // circumference of 90 degree bend
		float vertical_length =
			radial_length * 2 + (height - 2.0 * radius); // total vertical length

		_lightmap_size_hint.x = MAX(1.0, 4.0 * radial_length / texel_size) + padding;
		_lightmap_size_hint.y = MAX(1.0, vertical_length / texel_size) + padding;

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void CapsuleMesh::set_radius(const float p_radius)
{
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;
	if (radius > height * 0.5) {
		height = radius * 2.0;
	}
	_update_lightmap_size();
	request_update();
}

float CapsuleMesh::get_radius() const { return radius; }

void CapsuleMesh::set_height(const float p_height)
{
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}

	height = p_height;
	if (radius > height * 0.5) {
		radius = height * 0.5;
	}
	_update_lightmap_size();
	request_update();
}

float CapsuleMesh::get_height() const { return height; }

void CapsuleMesh::set_radial_segments(const int p_segments)
{
	if (radial_segments == p_segments) {
		return;
	}

	radial_segments = p_segments > 4 ? p_segments : 4;
	request_update();
}

int CapsuleMesh::get_radial_segments() const { return radial_segments; }

void CapsuleMesh::set_rings(const int p_rings)
{
	if (rings == p_rings) {
		return;
	}

	ERR_FAIL_COND(p_rings < 0);
	rings = p_rings;
	request_update();
}

int CapsuleMesh::get_rings() const { return rings; }

/**
  BoxMesh
*/

void BoxMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		float width = (size.x + size.z) / texel_size;
		float length = (size.y + size.y + MAX(size.x, size.z)) / texel_size;

		_lightmap_size_hint.x = MAX(1.0, width) + 2.0 * padding;
		_lightmap_size_hint.y = MAX(1.0, length) + 3.0 * padding;

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void BoxMesh::set_size(const Vector3& p_size)
{
	if (p_size.is_equal_approx(size)) {
		return;
	}

	size = p_size;
	_update_lightmap_size();
	request_update();
}

Vector3 BoxMesh::get_size() const { return size; }

void BoxMesh::set_subdivide_width(const int p_divisions)
{
	if (p_divisions == subdivide_w) {
		return;
	}

	subdivide_w = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int BoxMesh::get_subdivide_width() const { return subdivide_w; }

void BoxMesh::set_subdivide_height(const int p_divisions)
{
	if (p_divisions == subdivide_h) {
		return;
	}

	subdivide_h = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int BoxMesh::get_subdivide_height() const { return subdivide_h; }

void BoxMesh::set_subdivide_depth(const int p_divisions)
{
	if (p_divisions == subdivide_d) {
		return;
	}

	subdivide_d = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int BoxMesh::get_subdivide_depth() const { return subdivide_d; }

/**
	CylinderMesh
*/

void CylinderMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		float top_circumference = top_radius * Math::PI * 2.0;
		float bottom_circumference = bottom_radius * Math::PI * 2.0;

		float _width = MAX(top_circumference, bottom_circumference) / texel_size + padding;
		_width = MAX(_width, (((top_radius + bottom_radius) / texel_size) + padding) *
								 2.0); // this is extremely unlikely to be larger, will only happen
									   // if padding is larger then our diameter.
		_lightmap_size_hint.x = MAX(1.0, _width);

		float _height =
			((height + (MAX(top_radius, bottom_radius) * 2.0)) / texel_size) + (2.0 * padding);

		_lightmap_size_hint.y = MAX(1.0, _height);

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void CylinderMesh::set_top_radius(const float p_radius)
{
	if (Math::is_equal_approx(p_radius, top_radius)) {
		return;
	}

	top_radius = p_radius;
	_update_lightmap_size();
	request_update();
}

float CylinderMesh::get_top_radius() const { return top_radius; }

void CylinderMesh::set_bottom_radius(const float p_radius)
{
	if (Math::is_equal_approx(p_radius, bottom_radius)) {
		return;
	}

	bottom_radius = p_radius;
	_update_lightmap_size();
	request_update();
}

float CylinderMesh::get_bottom_radius() const { return bottom_radius; }

void CylinderMesh::set_height(const float p_height)
{
	if (Math::is_equal_approx(p_height, height)) {
		return;
	}

	height = p_height;
	_update_lightmap_size();
	request_update();
}

float CylinderMesh::get_height() const { return height; }

void CylinderMesh::set_radial_segments(const int p_segments)
{
	if (p_segments == radial_segments) {
		return;
	}

	radial_segments = p_segments > 4 ? p_segments : 4;
	request_update();
}

int CylinderMesh::get_radial_segments() const { return radial_segments; }

void CylinderMesh::set_rings(const int p_rings)
{
	if (p_rings == rings) {
		return;
	}

	ERR_FAIL_COND(p_rings < 0);
	rings = p_rings;
	request_update();
}

int CylinderMesh::get_rings() const { return rings; }

void CylinderMesh::set_cap_top(bool p_cap_top)
{
	if (p_cap_top == cap_top) {
		return;
	}

	cap_top = p_cap_top;
	request_update();
}

bool CylinderMesh::is_cap_top() const { return cap_top; }

void CylinderMesh::set_cap_bottom(bool p_cap_bottom)
{
	if (p_cap_bottom == cap_bottom) {
		return;
	}

	cap_bottom = p_cap_bottom;
	request_update();
}

bool CylinderMesh::is_cap_bottom() const { return cap_bottom; }

/**
  PlaneMesh
*/

void PlaneMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		_lightmap_size_hint.x = MAX(1.0, (size.x / texel_size) + padding);
		_lightmap_size_hint.y = MAX(1.0, (size.y / texel_size) + padding);

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void PlaneMesh::set_size(const Size2& p_size)
{
	if (p_size == size) {
		return;
	}
	size = p_size;
	_update_lightmap_size();
	request_update();
}

Size2 PlaneMesh::get_size() const { return size; }

void PlaneMesh::set_subdivide_width(const int p_divisions)
{
	if (p_divisions == subdivide_w || (subdivide_w == 0 && p_divisions < 0)) {
		return;
	}
	subdivide_w = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PlaneMesh::get_subdivide_width() const { return subdivide_w; }

void PlaneMesh::set_subdivide_depth(const int p_divisions)
{
	if (p_divisions == subdivide_d || (subdivide_d == 0 && p_divisions < 0)) {
		return;
	}
	subdivide_d = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PlaneMesh::get_subdivide_depth() const { return subdivide_d; }

void PlaneMesh::set_center_offset(const Vector3 p_offset)
{
	if (p_offset.is_equal_approx(center_offset)) {
		return;
	}
	center_offset = p_offset;
	request_update();
}

Vector3 PlaneMesh::get_center_offset() const { return center_offset; }

void PlaneMesh::set_orientation(const Orientation p_orientation)
{
	if (p_orientation == orientation) {
		return;
	}
	orientation = p_orientation;
	request_update();
}

PlaneMesh::Orientation PlaneMesh::get_orientation() const { return orientation; }

/**
  PrismMesh
*/

void PrismMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		// left_to_right does not effect the surface area of the prism so we ignore that.
		// TODO we could combine the two triangles and save some space but we need to re-align the
		// uv1 and adjust the tangent.

		float width = (size.x + size.z) / texel_size;
		float length = (size.y + size.y + size.z) / texel_size;

		_lightmap_size_hint.x = MAX(1.0, width) + 2.0 * padding;
		_lightmap_size_hint.y = MAX(1.0, length) + 3.0 * padding;

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void PrismMesh::set_left_to_right(const float p_left_to_right)
{
	if (Math::is_equal_approx(p_left_to_right, left_to_right)) {
		return;
	}
	left_to_right = p_left_to_right;
	request_update();
}

float PrismMesh::get_left_to_right() const { return left_to_right; }

void PrismMesh::set_size(const Vector3& p_size)
{
	if (p_size.is_equal_approx(size)) {
		return;
	}
	size = p_size;
	_update_lightmap_size();
	request_update();
}

Vector3 PrismMesh::get_size() const { return size; }

void PrismMesh::set_subdivide_width(const int p_divisions)
{
	if (p_divisions == subdivide_w || (p_divisions < 0 && subdivide_w == 0)) {
		return;
	}
	subdivide_w = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrismMesh::get_subdivide_width() const { return subdivide_w; }

void PrismMesh::set_subdivide_height(const int p_divisions)
{
	if (p_divisions == subdivide_h || (p_divisions < 0 && subdivide_h == 0)) {
		return;
	}
	subdivide_h = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrismMesh::get_subdivide_height() const { return subdivide_h; }

void PrismMesh::set_subdivide_depth(const int p_divisions)
{
	if (p_divisions == subdivide_d || (p_divisions < 0 && subdivide_d == 0)) {
		return;
	}
	subdivide_d = p_divisions > 0 ? p_divisions : 0;
	request_update();
}

int PrismMesh::get_subdivide_depth() const { return subdivide_d; }

/**
  SphereMesh
*/

void SphereMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		float _width = radius * Math::TAU;
		_lightmap_size_hint.x = MAX(1.0, (_width / texel_size) + padding);
		float _height = (is_hemisphere ? 1.0 : 0.5) * height *
						Math::PI; // note, with hemisphere height is our radius, while with a full
								  // sphere it is the diameter..
		_lightmap_size_hint.y = MAX(1.0, (_height / texel_size) + padding);

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void SphereMesh::set_radius(const float p_radius)
{
	if (Math::is_equal_approx(p_radius, radius)) {
		return;
	}
	radius = p_radius;
	_update_lightmap_size();
	request_update();
}

float SphereMesh::get_radius() const { return radius; }

void SphereMesh::set_height(const float p_height)
{
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}
	height = p_height;
	_update_lightmap_size();
	request_update();
}

float SphereMesh::get_height() const { return height; }

void SphereMesh::set_radial_segments(const int p_radial_segments)
{
	if (p_radial_segments == radial_segments || (radial_segments == 4 && p_radial_segments < 4)) {
		return;
	}
	radial_segments = p_radial_segments > 4 ? p_radial_segments : 4;
	request_update();
}

int SphereMesh::get_radial_segments() const { return radial_segments; }

void SphereMesh::set_rings(const int p_rings)
{
	if (p_rings == rings) {
		return;
	}
	ERR_FAIL_COND(p_rings < 1);
	rings = p_rings;
	request_update();
}

int SphereMesh::get_rings() const { return rings; }

void SphereMesh::set_is_hemisphere(const bool p_is_hemisphere)
{
	if (p_is_hemisphere == is_hemisphere) {
		return;
	}
	is_hemisphere = p_is_hemisphere;
	_update_lightmap_size();
	request_update();
}

bool SphereMesh::get_is_hemisphere() const { return is_hemisphere; }

/**
  TorusMesh
*/

void TorusMesh::_update_lightmap_size()
{
	if (get_add_uv2()) {
		// size must have changed, update lightmap size hint
		Size2i _lightmap_size_hint;
		float padding = get_uv2_padding();

		float min_radius = inner_radius;
		float max_radius = outer_radius;

		if (min_radius > max_radius) {
			SWAP(min_radius, max_radius);
		}

		float radius = (max_radius - min_radius) * 0.5;

		float _width = max_radius * Math::TAU;
		_lightmap_size_hint.x = MAX(1.0, (_width / texel_size) + padding);
		float _height = radius * Math::TAU;
		_lightmap_size_hint.y = MAX(1.0, (_height / texel_size) + padding);

		set_lightmap_size_hint(_lightmap_size_hint);
	}
}

void TorusMesh::set_inner_radius(const float p_inner_radius)
{
	if (Math::is_equal_approx(p_inner_radius, inner_radius)) {
		return;
	}
	inner_radius = p_inner_radius;
	request_update();
}

float TorusMesh::get_inner_radius() const { return inner_radius; }

void TorusMesh::set_outer_radius(const float p_outer_radius)
{
	if (Math::is_equal_approx(p_outer_radius, outer_radius)) {
		return;
	}
	outer_radius = p_outer_radius;
	request_update();
}

float TorusMesh::get_outer_radius() const { return outer_radius; }

void TorusMesh::set_rings(const int p_rings)
{
	if (p_rings == rings) {
		return;
	}
	ERR_FAIL_COND(p_rings < 3);
	rings = p_rings;
	request_update();
}

int TorusMesh::get_rings() const { return rings; }

void TorusMesh::set_ring_segments(const int p_ring_segments)
{
	if (p_ring_segments == ring_segments) {
		return;
	}
	ERR_FAIL_COND(p_ring_segments < 3);
	ring_segments = p_ring_segments;
	request_update();
}

int TorusMesh::get_ring_segments() const { return ring_segments; }

PointMesh::PointMesh() { primitive_type = PRIMITIVE_POINTS; }

void TubeTrailMesh::set_radius(const float p_radius)
{
	if (Math::is_equal_approx(p_radius, radius)) {
		return;
	}
	radius = p_radius;
	request_update();
}

float TubeTrailMesh::get_radius() const { return radius; }

void TubeTrailMesh::set_radial_steps(const int p_radial_steps)
{
	if (p_radial_steps == radial_steps) {
		return;
	}
	ERR_FAIL_COND(p_radial_steps < 3 || p_radial_steps > 128);
	radial_steps = p_radial_steps;
	request_update();
}

int TubeTrailMesh::get_radial_steps() const { return radial_steps; }

void TubeTrailMesh::set_sections(const int p_sections)
{
	if (p_sections == sections) {
		return;
	}
	ERR_FAIL_COND(p_sections < 2 || p_sections > 128);
	sections = p_sections;
	request_update();
}

int TubeTrailMesh::get_sections() const { return sections; }

void TubeTrailMesh::set_section_length(float p_section_length)
{
	if (p_section_length == section_length) {
		return;
	}
	section_length = p_section_length;
	request_update();
}

float TubeTrailMesh::get_section_length() const { return section_length; }

void TubeTrailMesh::set_section_rings(const int p_section_rings)
{
	if (p_section_rings == section_rings) {
		return;
	}
	ERR_FAIL_COND(p_section_rings < 1 || p_section_rings > 1024);
	section_rings = p_section_rings;
	request_update();
}

int TubeTrailMesh::get_section_rings() const { return section_rings; }

void TubeTrailMesh::set_cap_top(bool p_cap_top)
{
	if (p_cap_top == cap_top) {
		return;
	}
	cap_top = p_cap_top;
	request_update();
}

bool TubeTrailMesh::is_cap_top() const { return cap_top; }

void TubeTrailMesh::set_cap_bottom(bool p_cap_bottom)
{
	if (p_cap_bottom == cap_bottom) {
		return;
	}
	cap_bottom = p_cap_bottom;
	request_update();
}

bool TubeTrailMesh::is_cap_bottom() const { return cap_bottom; }

Ref<Curve> TubeTrailMesh::get_curve() const { return curve; }

void TubeTrailMesh::_curve_changed() { request_update(); }

int TubeTrailMesh::get_builtin_bind_pose_count() const { return sections + 1; }

Transform3D TubeTrailMesh::get_builtin_bind_pose(int p_index) const
{
	float depth = section_length * sections;

	Transform3D xform;
	xform.origin.y = depth / 2.0 - section_length * float(p_index);
	xform.origin.y = -xform.origin.y; // bind is an inverse transform, so negate y

	return xform;
}

TubeTrailMesh::TubeTrailMesh() {}

// RIBBON TRAIL

void RibbonTrailMesh::set_shape(Shape p_shape)
{
	if (p_shape == shape) {
		return;
	}
	shape = p_shape;
	request_update();
}

RibbonTrailMesh::Shape RibbonTrailMesh::get_shape() const { return shape; }

void RibbonTrailMesh::set_size(const float p_size)
{
	if (Math::is_equal_approx(p_size, size)) {
		return;
	}
	size = p_size;
	request_update();
}

float RibbonTrailMesh::get_size() const { return size; }

void RibbonTrailMesh::set_sections(const int p_sections)
{
	if (p_sections == sections) {
		return;
	}
	ERR_FAIL_COND(p_sections < 2 || p_sections > 128);
	sections = p_sections;
	request_update();
}

int RibbonTrailMesh::get_sections() const { return sections; }

void RibbonTrailMesh::set_section_length(float p_section_length)
{
	if (p_section_length == section_length) {
		return;
	}
	section_length = p_section_length;
	request_update();
}

float RibbonTrailMesh::get_section_length() const { return section_length; }

void RibbonTrailMesh::set_section_segments(const int p_section_segments)
{
	if (p_section_segments == section_segments) {
		return;
	}
	ERR_FAIL_COND(p_section_segments < 1 || p_section_segments > 1024);
	section_segments = p_section_segments;
	request_update();
}

int RibbonTrailMesh::get_section_segments() const { return section_segments; }

Ref<Curve> RibbonTrailMesh::get_curve() const { return curve; }

void RibbonTrailMesh::_curve_changed() { request_update(); }

int RibbonTrailMesh::get_builtin_bind_pose_count() const { return sections + 1; }

Transform3D RibbonTrailMesh::get_builtin_bind_pose(int p_index) const
{
	float depth = section_length * sections;

	Transform3D xform;
	xform.origin.y = depth / 2.0 - section_length * float(p_index);
	xform.origin.y = -xform.origin.y; // bind is an inverse transform, so negate y

	return xform;
}

RibbonTrailMesh::RibbonTrailMesh() {}

TextMesh::TextMesh()
{
	primitive_type = PRIMITIVE_TRIANGLES;
	text_rid = TS->create_shaped_text();
}

TextMesh::~TextMesh()
{
	for (int i = 0; i < lines_rid.size(); i++) {
		TS->free_rid(lines_rid[i]);
	}
	lines_rid.clear();

	TS->free_rid(text_rid);
}

void TextMesh::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (horizontal_alignment != p_alignment) {
		if (horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL ||
			p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			dirty_lines = true;
		}
		horizontal_alignment = p_alignment;
		request_update();
	}
}

HorizontalAlignment TextMesh::get_horizontal_alignment() const { return horizontal_alignment; }

void TextMesh::set_vertical_alignment(VerticalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (vertical_alignment != p_alignment) {
		vertical_alignment = p_alignment;
		request_update();
	}
}

VerticalAlignment TextMesh::get_vertical_alignment() const { return vertical_alignment; }

String TextMesh::get_text() const { return text; }

Ref<Font> TextMesh::get_font() const { return font_override; }

void TextMesh::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = CLAMP(p_size, 1, 127);
		dirty_font = true;
		dirty_cache = true;
		request_update();
	}
}

int TextMesh::get_font_size() const { return font_size; }

void TextMesh::set_line_spacing(float p_line_spacing)
{
	if (line_spacing != p_line_spacing) {
		line_spacing = p_line_spacing;
		request_update();
	}
}

float TextMesh::get_line_spacing() const { return line_spacing; }

void TextMesh::set_autowrap_mode(TextServer::AutowrapMode p_mode)
{
	if (autowrap_mode != p_mode) {
		autowrap_mode = p_mode;
		dirty_lines = true;
		request_update();
	}
}

TextServer::AutowrapMode TextMesh::get_autowrap_mode() const { return autowrap_mode; }

void TextMesh::set_justification_flags(uint32_t p_flags)
{
	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		dirty_lines = true;
		request_update();
	}
}

uint32_t TextMesh::get_justification_flags() const { return jst_flags; }

void TextMesh::set_depth(real_t p_depth)
{
	if (depth != p_depth) {
		depth = MAX(p_depth, 0.0);
		request_update();
	}
}

real_t TextMesh::get_depth() const { return depth; }

void TextMesh::set_width(real_t p_width)
{
	if (width != p_width) {
		width = p_width;
		dirty_lines = true;
		request_update();
	}
}

real_t TextMesh::get_width() const { return width; }

void TextMesh::set_pixel_size(real_t p_amount)
{
	if (pixel_size != p_amount) {
		pixel_size = CLAMP(p_amount, 0.0001, 128.0);
		dirty_cache = true;
		request_update();
	}
}

real_t TextMesh::get_pixel_size() const { return pixel_size; }

void TextMesh::set_offset(const Point2& p_offset)
{
	if (lbl_offset != p_offset) {
		lbl_offset = p_offset;
		request_update();
	}
}

Point2 TextMesh::get_offset() const { return lbl_offset; }

void TextMesh::set_curve_step(real_t p_step)
{
	if (curve_step != p_step) {
		curve_step = CLAMP(p_step, 0.1, 10.0);
		dirty_cache = true;
		request_update();
	}
}

real_t TextMesh::get_curve_step() const { return curve_step; }

void TextMesh::set_text_direction(TextServer::Direction p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		dirty_text = true;
		request_update();
	}
}

TextServer::Direction TextMesh::get_text_direction() const { return text_direction; }

void TextMesh::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		dirty_text = true;
		request_update();
	}
}

String TextMesh::get_language() const { return language; }

void TextMesh::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		st_parser = p_parser;
		dirty_text = true;
		request_update();
	}
}

TextServer::StructuredTextParser TextMesh::get_structured_text_bidi_override() const
{
	return st_parser;
}

void TextMesh::set_uppercase(bool p_uppercase)
{
	if (uppercase != p_uppercase) {
		uppercase = p_uppercase;
		dirty_text = true;
		request_update();
	}
}

bool TextMesh::is_uppercase() const { return uppercase; }


