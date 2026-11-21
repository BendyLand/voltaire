/**************************************************************************/
/*  jolt_shape_3d.cpp                                                     */
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

#include "jolt_shape_3d.h"

#include "../jolt_globals.h"
#include "modules/jolt_physics/shapes/jolt_shape_3d.h"

// keep includes...
#include <Jolt/Jolt.h>
// ...in this order.
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

// ============================================================================
// JoltShapeInstance3D
// ============================================================================

JoltShapeInstance3D::ShapeReference::ShapeReference(JoltObject3D *p_parent, JoltShape3D *p_shape) :
		parent(p_parent),
		shape(p_shape) {
	if (shape != nullptr) {
		shape->add_owner(parent);
	}
}

JoltShapeInstance3D::ShapeReference::ShapeReference(ShapeReference &&p_other) :
		parent(p_other.parent),
		shape(p_other.shape) {
	p_other.parent = nullptr;
	p_other.shape = nullptr;
}

JoltShapeInstance3D::ShapeReference::~ShapeReference() {
	if (shape != nullptr) {
		shape->remove_owner(parent);
	}
}

JoltShapeInstance3D::ShapeReference &JoltShapeInstance3D::ShapeReference::operator=(ShapeReference &&p_other) {
	if (this != &p_other) {
		SWAP(parent, p_other.parent);
		SWAP(shape, p_other.shape);
	}

	return *this;
}

JoltShapeInstance3D::JoltShapeInstance3D(JoltObject3D *p_parent, JoltShape3D *p_shape, const Transform3D &p_transform, const Vector3 &p_scale, bool p_disabled) :
		transform(p_transform),
		scale(p_scale),
		shape(p_parent, p_shape),
		disabled(p_disabled) {
}

AABB JoltShapeInstance3D::get_aabb() const {
	return get_transform_scaled().xform(shape->get_aabb());
}

bool JoltShapeInstance3D::try_build() {
	ERR_FAIL_COND_V(is_disabled(), false);

	const JPH::ShapeRefC maybe_new_shape = shape->try_build();

	if (maybe_new_shape == nullptr) {
		jolt_ref = nullptr;
		return false;
	}

	if (jolt_ref != nullptr) {
		const JPH::DecoratedShape *outer_shape = static_cast<const JPH::DecoratedShape *>(jolt_ref.GetPtr());

		if (outer_shape->GetInnerShape() == maybe_new_shape) {
			return true;
		}
	}

	jolt_ref = JoltShape3D::with_user_data(maybe_new_shape, (uint64_t)id);

	return true;
}

// ============================================================================
// JoltShape3D
// ============================================================================

namespace {

constexpr float DEFAULT_SOLVER_BIAS = 0.0f;

} // namespace

String JoltShape3D::_owners_to_string() const {
	const int owner_count = ref_counts_by_owner.size();

	if (owner_count == 0) {
		return "'<unknown>' and 0 other object(s)";
	}

	const JoltObject3D &random_owner = *ref_counts_by_owner.begin()->key;

	return vformat("'%s' and %d other object(s)", random_owner.to_string(), owner_count - 1);
}

JPH::ShapeRefC JoltShape3D::_build() const {
	switch (shape_type) {
		case ShapeType::SHAPE_WORLD_BOUNDARY:
		case ShapeType::SHAPE_BOX:
		case ShapeType::SHAPE_CYLINDER:
		case ShapeType::SHAPE_CONVEX_POLYGON:
		case ShapeType::SHAPE_CONCAVE_POLYGON:
			return JPH::ShapeRefC();
		case ShapeType::SHAPE_SEPARATION_RAY:
			return _build_separation_ray();
		case ShapeType::SHAPE_SPHERE:
			return _build_sphere();
		case ShapeType::SHAPE_CAPSULE:
			return _build_capsule();
		case ShapeType::SHAPE_HEIGHTMAP:
			return _build_height_map();
		default:
			return JPH::ShapeRefC();
	}
}

JPH::ShapeRefC JoltShape3D::_build_separation_ray() const {
	ERR_FAIL_COND_V_MSG(length <= 0.0f, nullptr,
			vformat("Failed to build Jolt Physics separation ray shape with %s. Its length must be "
					"greater than 0. This shape belongs to %s.",
					to_string(), _owners_to_string()));

	const JoltCustomRayShapeSettings shape_settings(length, slide_on_slope);
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to build Jolt Physics separation ray shape with %s. It returned the "
					"following error: '%s'. This shape belongs to %s.",
					to_string(), to_godot(shape_result.GetError()), _owners_to_string()));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::_build_sphere() const {
	ERR_FAIL_COND_V_MSG(radius <= 0.0f, nullptr,
			vformat("Failed to build Jolt Physics sphere shape with %s. Its radius must be greater "
					"than 0. This shape belongs to %s.",
					to_string(), _owners_to_string()));

	const JPH::SphereShapeSettings shape_settings(radius);
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to build Jolt Physics sphere shape with %s. It returned the following "
					"error: '%s'. This shape belongs to %s.",
					to_string(), to_godot(shape_result.GetError()), _owners_to_string()));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::_build_capsule() const {
	ERR_FAIL_COND_V_MSG(radius <= 0.0f, nullptr,
			vformat("Failed to build Jolt Physics capsule shape with %s. Its radius must be greater "
					"than 0. This shape belongs to %s.",
					to_string(), _owners_to_string()));
	ERR_FAIL_COND_V_MSG(height <= 0.0f, nullptr,
			vformat("Failed to build Jolt Physics capsule shape with %s. Its height must be greater "
					"than 0. This shape belongs to %s.",
					to_string(), _owners_to_string()));
	ERR_FAIL_COND_V_MSG(height < radius * 2.0f, nullptr,
			vformat("Failed to build Jolt Physics capsule shape with %s. Its height must be at least "
					"double that of its radius. This shape belongs to %s.",
					to_string(), _owners_to_string()));

	const float half_height = height / 2.0f;
	const float cylinder_height = half_height - radius;

	const JPH::CapsuleShapeSettings shape_settings(cylinder_height, radius);
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to build Jolt Physics capsule shape with %s. It returned the following "
					"error: '%s'. This shape belongs to %s.",
					to_string(), to_godot(shape_result.GetError()), _owners_to_string()));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::_build_height_map() const {
	const int height_count = (int)heights.size();
	if (unlikely(height_count == 0)) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(height_count != width * depth, nullptr,
			vformat("Failed to build Jolt Physics height map shape with %s. Height count must be the "
					"product of width and depth. This shape belongs to %s.",
					to_string(), _owners_to_string()));
	ERR_FAIL_COND_V_MSG(width < 2 || depth < 2, nullptr,
			vformat("Failed to build Jolt Physics height map shape with %s. The height map must be at "
					"least 2x2. This shape belongs to %s.",
					to_string(), _owners_to_string()));

	if (width != depth) {
		return JoltShape3D::with_double_sided(_build_mesh(), true);
	}

	const int block_size = 2;
	const int block_count = width / block_size;

	if (block_count < 2) {
		return JoltShape3D::with_double_sided(_build_mesh(), true);
	}

	return JoltShape3D::with_double_sided(_build_height_field(), true);
}

JPH::ShapeRefC JoltShape3D::_build_mesh() const {
	return JPH::ShapeRefC();
}

JPH::ShapeRefC JoltShape3D::_build_height_field() const {
	return JPH::ShapeRefC();
}

AABB JoltShape3D::_calculate_convex_aabb() const {
	AABB result;

	for (int i = 0; i < vertices.size(); ++i) {
		if (i == 0) {
			result.position = vertices[i];
		} else {
			result.expand_to(vertices[i]);
		}
	}

	return result;
}

AABB JoltShape3D::_calculate_heightmap_aabb() const {
	AABB result;

	const int quad_count_x = width - 1;
	const int quad_count_z = depth - 1;

	const float offset_x = (float)-quad_count_x / 2.0f;
	const float offset_z = (float)-quad_count_z / 2.0f;

	for (int z = 0; z < depth; ++z) {
		for (int x = 0; x < width; ++x) {
			const Vector3 vertex(offset_x + (float)x, (float)heights[z * width + x], offset_z + (float)z);

			if (x == 0 && z == 0) {
				result.position = vertex;
			} else {
				result.expand_to(vertex);
			}
		}
	}

	return result;
}

void JoltShape3D::add_owner(JoltObject3D *p_owner) {
	ref_counts_by_owner[p_owner]++;
}

void JoltShape3D::remove_owner(JoltObject3D *p_owner) {
	if (--ref_counts_by_owner[p_owner] <= 0) {
		ref_counts_by_owner.erase(p_owner);
	}
}

void JoltShape3D::remove_self() {
	const HashMap<JoltObject3D *, int> ref_counts_by_owner_copy(ref_counts_by_owner);

	for (const KeyValue<JoltObject3D *, int> &E : ref_counts_by_owner_copy) {
		E.key->remove_shape(this);
	}
}

bool JoltShape3D::is_convex() const {
	switch (shape_type) {
		case ShapeType::SHAPE_SEPARATION_RAY:
		case ShapeType::SHAPE_SPHERE:
		case ShapeType::SHAPE_BOX:
		case ShapeType::SHAPE_CAPSULE:
		case ShapeType::SHAPE_CYLINDER:
		case ShapeType::SHAPE_CONVEX_POLYGON:
			return true;
		default:
			return false;
	}
}

float JoltShape3D::get_margin() const {
	switch (shape_type) {
		case ShapeType::SHAPE_BOX:
		case ShapeType::SHAPE_CYLINDER:
		case ShapeType::SHAPE_CONVEX_POLYGON:
			return margin;
		default:
			return 0.0f;
	}
}

void JoltShape3D::set_margin(float p_margin) {
	switch (shape_type) {
		case ShapeType::SHAPE_BOX:
		case ShapeType::SHAPE_CYLINDER:
		case ShapeType::SHAPE_CONVEX_POLYGON: {
			if (unlikely(margin == p_margin)) {
				return;
			}
			margin = p_margin;
			destroy();
		} break;
		default:
			break;
	}
}

AABB JoltShape3D::get_aabb() const {
	switch (shape_type) {
		case ShapeType::SHAPE_WORLD_BOUNDARY:
			return AABB();
		case ShapeType::SHAPE_SEPARATION_RAY: {
			constexpr float size_xy = 0.1f;
			constexpr float half_size_xy = size_xy / 2.0f;
			return AABB(Vector3(-half_size_xy, -half_size_xy, 0.0f), Vector3(size_xy, size_xy, length));
		}
		case ShapeType::SHAPE_SPHERE: {
			const Vector3 sphere_half_extents(radius, radius, radius);
			return AABB(-sphere_half_extents, sphere_half_extents * 2.0f);
		}
		case ShapeType::SHAPE_BOX:
			return AABB(-half_extents, half_extents * 2.0f);
		case ShapeType::SHAPE_CAPSULE:
		case ShapeType::SHAPE_CYLINDER: {
			const Vector3 cyl_half_extents(radius, height / 2.0f, radius);
			return AABB(-cyl_half_extents, cyl_half_extents * 2.0f);
		}
		case ShapeType::SHAPE_CONVEX_POLYGON:
		case ShapeType::SHAPE_CONCAVE_POLYGON:
		case ShapeType::SHAPE_HEIGHTMAP:
			return aabb;
		default:
			return AABB();
	}
}

float JoltShape3D::get_solver_bias() const {
	return DEFAULT_SOLVER_BIAS;
}

void JoltShape3D::set_solver_bias(float p_bias) {
	if (!Math::is_equal_approx(p_bias, DEFAULT_SOLVER_BIAS)) {
		WARN_PRINT(vformat("Custom solver bias for shapes is not supported when using Jolt "
						   "Physics. Any such value will be ignored. This shape belongs to %s.",
				_owners_to_string()));
	}
}

String JoltShape3D::to_string() const {
	switch (shape_type) {
		case ShapeType::SHAPE_WORLD_BOUNDARY:
			return vformat("{plane=%s}", plane);
		case ShapeType::SHAPE_SEPARATION_RAY:
			return vformat("{length=%f slide_on_slope=%s}", length, slide_on_slope);
		case ShapeType::SHAPE_SPHERE:
			return vformat("{radius=%f}", radius);
		case ShapeType::SHAPE_BOX:
			return vformat("{half_extents=%v margin=%f}", half_extents, margin);
		case ShapeType::SHAPE_CAPSULE:
			return vformat("{height=%f radius=%f}", height, radius);
		case ShapeType::SHAPE_CYLINDER:
			return vformat("{height=%f radius=%f margin=%f}", height, radius, margin);
		case ShapeType::SHAPE_CONVEX_POLYGON:
			return vformat("{vertex_count=%d margin=%f}", vertices.size(), margin);
		case ShapeType::SHAPE_CONCAVE_POLYGON:
			return vformat("{back_face_collision=%s}", back_face_collision);
		case ShapeType::SHAPE_HEIGHTMAP:
			return vformat("{height_count=%d width=%d depth=%d}", heights.size(), width, depth);
		default:
			return "{custom}";
	}
}

void JoltShape3D::set_plane(const Plane &p_plane) {
	if (unlikely(plane == p_plane)) {
		return;
	}
	plane = p_plane;
	destroy();
}

void JoltShape3D::set_ray_length(float p_length) {
	if (unlikely(length == p_length)) {
		return;
	}
	length = p_length;
	destroy();
}

void JoltShape3D::set_slide_on_slope(bool p_slide_on_slope) {
	if (unlikely(slide_on_slope == p_slide_on_slope)) {
		return;
	}
	slide_on_slope = p_slide_on_slope;
	destroy();
}

void JoltShape3D::set_radius(float p_radius) {
	if (unlikely(radius == p_radius)) {
		return;
	}
	radius = p_radius;
	destroy();
}

void JoltShape3D::set_height(float p_height) {
	if (unlikely(height == p_height)) {
		return;
	}
	height = p_height;
	destroy();
}

void JoltShape3D::set_half_extents(const Vector3 &p_half_extents) {
	if (unlikely(half_extents == p_half_extents)) {
		return;
	}
	half_extents = p_half_extents;
	destroy();
}

void JoltShape3D::set_vertices(const PackedVector3Array &p_vertices) {
	vertices = p_vertices;
	aabb = _calculate_convex_aabb();
	destroy();
}

void JoltShape3D::set_back_face_collision(bool p_back_face_collision) {
	if (unlikely(back_face_collision == p_back_face_collision)) {
		return;
	}
	back_face_collision = p_back_face_collision;
	destroy();
}

void JoltShape3D::set_concave_aabb(const AABB &p_aabb) {
	aabb = p_aabb;
}

void JoltShape3D::set_heightmap(const HeightArray &p_heights, int p_width, int p_depth) {
	heights = p_heights;
	width = p_width;
	depth = p_depth;
	aabb = _calculate_heightmap_aabb();
	destroy();
}

JPH::ShapeRefC JoltShape3D::try_build() {
	jolt_ref_mutex.lock();

	if (jolt_ref == nullptr) {
		jolt_ref = _build();
	}

	jolt_ref_mutex.unlock();

	return jolt_ref;
}

void JoltShape3D::destroy() {
	jolt_ref_mutex.lock();
	jolt_ref = nullptr;
	jolt_ref_mutex.unlock();

	for (const KeyValue<JoltObject3D *, int> &E : ref_counts_by_owner) {
		E.key->_shapes_changed();
	}
}

JPH::ShapeRefC JoltShape3D::with_scale(const JPH::Shape *p_shape, const Vector3 &p_scale) {
	ERR_FAIL_NULL_V(p_shape, nullptr);

	const JPH::ScaledShapeSettings shape_settings(p_shape, to_jolt(p_scale));
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to scale shape with {scale=%v}. It returned the following error: '%s'.",
					p_scale, to_godot(shape_result.GetError())));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::with_basis_origin(const JPH::Shape *p_shape, const Basis &p_basis, const Vector3 &p_origin) {
	ERR_FAIL_NULL_V(p_shape, nullptr);

	const JPH::RotatedTranslatedShapeSettings shape_settings(to_jolt(p_origin), to_jolt(p_basis), p_shape);

	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to offset shape with {basis=%s origin=%v}. It returned the following "
					"error: '%s'.",
					p_basis, p_origin, to_godot(shape_result.GetError())));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::with_center_of_mass_offset(const JPH::Shape *p_shape, const Vector3 &p_offset) {
	ERR_FAIL_NULL_V(p_shape, nullptr);

	const JPH::OffsetCenterOfMassShapeSettings shape_settings(to_jolt(p_offset), p_shape);
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to offset center of mass with {offset=%v}. It returned the following "
					"error: '%s'.",
					p_offset, to_godot(shape_result.GetError())));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::with_center_of_mass(const JPH::Shape *p_shape, const Vector3 &p_center_of_mass) {
	ERR_FAIL_NULL_V(p_shape, nullptr);

	const Vector3 center_of_mass_inner = to_godot(p_shape->GetCenterOfMass());
	const Vector3 center_of_mass_offset = p_center_of_mass - center_of_mass_inner;

	if (center_of_mass_offset == Vector3()) {
		return p_shape;
	}

	return with_center_of_mass_offset(p_shape, center_of_mass_offset);
}

JPH::ShapeRefC JoltShape3D::with_user_data(const JPH::Shape *p_shape, uint64_t p_user_data) {
	JoltCustomUserDataShapeSettings shape_settings(p_shape);
	shape_settings.mUserData = (JPH::uint64)p_user_data;

	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to override user data. It returned the following error: '%s'.",
					to_godot(shape_result.GetError())));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::with_double_sided(const JPH::Shape *p_shape, bool p_back_face_collision) {
	ERR_FAIL_NULL_V(p_shape, nullptr);

	const JoltCustomDoubleSidedShapeSettings shape_settings(p_shape, p_back_face_collision);
	const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
	ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
			vformat("Failed to make shape double-sided. It returned the following error: '%s'.",
					to_godot(shape_result.GetError())));

	return shape_result.Get();
}

JPH::ShapeRefC JoltShape3D::without_custom_shapes(const JPH::Shape *p_shape) {
	switch (p_shape->GetSubType()) {
		case JoltCustomShapeSubType::RAY:
		case JoltCustomShapeSubType::MOTION: {
			// Replace unsupported shapes with a small sphere.
			return new JPH::SphereShape(0.1f);
		}

		case JoltCustomShapeSubType::OVERRIDE_USER_DATA:
		case JoltCustomShapeSubType::DOUBLE_SIDED: {
			const JPH::DecoratedShape *shape = static_cast<const JPH::DecoratedShape *>(p_shape);

			// Replace unsupported decorator shapes with the inner shape.
			return without_custom_shapes(shape->GetInnerShape());
		}

		case JPH::EShapeSubType::StaticCompound: {
			const JPH::StaticCompoundShape *shape = static_cast<const JPH::StaticCompoundShape *>(p_shape);

			JPH::StaticCompoundShapeSettings settings;

			for (const JPH::CompoundShape::SubShape &sub_shape : shape->GetSubShapes()) {
				settings.AddShape(shape->GetCenterOfMass() + sub_shape.GetPositionCOM() -
								sub_shape.GetRotation() * sub_shape.mShape->GetCenterOfMass(),
						sub_shape.GetRotation(), without_custom_shapes(sub_shape.mShape));
			}

			const JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
			ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
					vformat("Failed to recreate static compound shape during filtering of custom shapes. "
							"It returned the following error: '%s'.",
							to_godot(shape_result.GetError())));

			return shape_result.Get();
		}

		case JPH::EShapeSubType::MutableCompound: {
			const JPH::MutableCompoundShape *shape = static_cast<const JPH::MutableCompoundShape *>(p_shape);

			JPH::MutableCompoundShapeSettings settings;

			for (const JPH::MutableCompoundShape::SubShape &sub_shape : shape->GetSubShapes()) {
				settings.AddShape(shape->GetCenterOfMass() + sub_shape.GetPositionCOM() -
								sub_shape.GetRotation() * sub_shape.mShape->GetCenterOfMass(),
						sub_shape.GetRotation(), without_custom_shapes(sub_shape.mShape));
			}

			const JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
			ERR_FAIL_COND_V_MSG(shape_result.HasError(), nullptr,
					vformat("Failed to recreate mutable compound shape during filtering of custom shapes. "
							"It returned the following error: '%s'.",
							to_godot(shape_result.GetError())));

			return shape_result.Get();
		}

		case JPH::EShapeSubType::RotatedTranslated: {
			const JPH::RotatedTranslatedShape *shape = static_cast<const JPH::RotatedTranslatedShape *>(p_shape);

			const JPH::Shape *inner_shape = shape->GetInnerShape();
			const JPH::ShapeRefC new_inner_shape = without_custom_shapes(inner_shape);

			if (inner_shape == new_inner_shape) {
				return p_shape;
			}

			return new JPH::RotatedTranslatedShape(shape->GetPosition(), shape->GetRotation(), new_inner_shape);
		}

		case JPH::EShapeSubType::Scaled: {
			const JPH::ScaledShape *shape = static_cast<const JPH::ScaledShape *>(p_shape);

			const JPH::Shape *inner_shape = shape->GetInnerShape();
			const JPH::ShapeRefC new_inner_shape = without_custom_shapes(inner_shape);

			if (inner_shape == new_inner_shape) {
				return p_shape;
			}

			return new JPH::ScaledShape(new_inner_shape, shape->GetScale());
		}

		case JPH::EShapeSubType::OffsetCenterOfMass: {
			const JPH::OffsetCenterOfMassShape *shape = static_cast<const JPH::OffsetCenterOfMassShape *>(p_shape);

			const JPH::Shape *inner_shape = shape->GetInnerShape();
			const JPH::ShapeRefC new_inner_shape = without_custom_shapes(inner_shape);

			if (inner_shape == new_inner_shape) {
				return p_shape;
			}

			return new JPH::OffsetCenterOfMassShape(new_inner_shape, shape->GetOffset());
		}

		default: {
			return p_shape;
		}
	}
}

Vector3 JoltShape3D::make_scale_valid(const JPH::Shape *p_shape, const Vector3 &p_scale) {
	return to_godot(p_shape->MakeScaleValid(to_jolt(p_scale)));
}

bool JoltShape3D::is_scale_valid(const Vector3 &p_scale, const Vector3 &p_valid_scale, real_t p_tolerance) {
	return Math::is_equal_approx(p_scale.x, p_valid_scale.x, p_tolerance) &&
			Math::is_equal_approx(p_scale.y, p_valid_scale.y, p_tolerance) &&
			Math::is_equal_approx(p_scale.z, p_valid_scale.z, p_tolerance);
}

// ============================================================================
// Merged from modules/jolt_physics/shapes/jolt_custom_shapes.cpp
// ============================================================================

#include "modules/jolt_physics/shapes/jolt_shape_3d.h"

#include "modules/jolt_physics/shapes/jolt_shape_3d.h"

#include "core/error/error_macros.h"

#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/TransformedShape.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRenderer.h>
#endif

namespace {

// --- JoltCustomUserDataShape helpers ---

JPH::Shape *construct_override_user_data() {
	return new JoltCustomUserDataShape();
}

void collide_override_user_data_vs_shape(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
	ERR_FAIL_COND(p_shape1->GetSubType() != JoltCustomShapeSubType::OVERRIDE_USER_DATA);

	const JoltCustomUserDataShape *shape1 = static_cast<const JoltCustomUserDataShape *>(p_shape1);

	JPH::CollisionDispatch::sCollideShapeVsShape(shape1->GetInnerShape(), p_shape2, p_scale1, p_scale2, p_center_of_mass_transform1, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, p_collide_shape_settings, p_collector, p_shape_filter);
}

void collide_shape_vs_override_user_data(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
	ERR_FAIL_COND(p_shape2->GetSubType() != JoltCustomShapeSubType::OVERRIDE_USER_DATA);

	const JoltCustomUserDataShape *shape2 = static_cast<const JoltCustomUserDataShape *>(p_shape2);

	JPH::CollisionDispatch::sCollideShapeVsShape(p_shape1, shape2->GetInnerShape(), p_scale1, p_scale2, p_center_of_mass_transform1, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, p_collide_shape_settings, p_collector, p_shape_filter);
}

void cast_override_user_data_vs_shape(const JPH::ShapeCast &p_shape_cast, const JPH::ShapeCastSettings &p_shape_cast_settings, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, const JPH::ShapeFilter &p_shape_filter, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, JPH::CastShapeCollector &p_collector) {
	ERR_FAIL_COND(p_shape_cast.mShape->GetSubType() != JoltCustomShapeSubType::OVERRIDE_USER_DATA);

	const JoltCustomUserDataShape *shape = static_cast<const JoltCustomUserDataShape *>(p_shape_cast.mShape);
	const JPH::ShapeCast shape_cast(shape->GetInnerShape(), p_shape_cast.mScale, p_shape_cast.mCenterOfMassStart, p_shape_cast.mDirection);

	JPH::CollisionDispatch::sCastShapeVsShapeLocalSpace(shape_cast, p_shape_cast_settings, p_shape, p_scale, p_shape_filter, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, p_collector);
}

void cast_shape_vs_override_user_data(const JPH::ShapeCast &p_shape_cast, const JPH::ShapeCastSettings &p_shape_cast_settings, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, const JPH::ShapeFilter &p_shape_filter, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, JPH::CastShapeCollector &p_collector) {
	ERR_FAIL_COND(p_shape->GetSubType() != JoltCustomShapeSubType::OVERRIDE_USER_DATA);

	const JoltCustomUserDataShape *shape = static_cast<const JoltCustomUserDataShape *>(p_shape);

	JPH::CollisionDispatch::sCastShapeVsShapeLocalSpace(p_shape_cast, p_shape_cast_settings, shape->GetInnerShape(), p_scale, p_shape_filter, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, p_collector);
}

// --- JoltCustomDoubleSidedShape helpers ---

JPH::Shape *construct_double_sided() {
	return new JoltCustomDoubleSidedShape();
}

void collide_double_sided_vs_shape(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
	ERR_FAIL_COND(p_shape1->GetSubType() != JoltCustomShapeSubType::DOUBLE_SIDED);

	const JoltCustomDoubleSidedShape *shape1 = static_cast<const JoltCustomDoubleSidedShape *>(p_shape1);

	JPH::CollideShapeSettings new_collide_shape_settings = p_collide_shape_settings;
	if (shape1->should_collide_with_back_faces()) {
		new_collide_shape_settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
	}

	JPH::CollisionDispatch::sCollideShapeVsShape(shape1->GetInnerShape(), p_shape2, p_scale1, p_scale2, p_center_of_mass_transform1, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, new_collide_shape_settings, p_collector, p_shape_filter);
}

void collide_shape_vs_double_sided(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
	ERR_FAIL_COND(p_shape2->GetSubType() != JoltCustomShapeSubType::DOUBLE_SIDED);

	const JoltCustomDoubleSidedShape *shape2 = static_cast<const JoltCustomDoubleSidedShape *>(p_shape2);

	JPH::CollideShapeSettings new_collide_shape_settings = p_collide_shape_settings;
	if (shape2->should_collide_with_back_faces()) {
		new_collide_shape_settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
	}

	JPH::CollisionDispatch::sCollideShapeVsShape(p_shape1, shape2->GetInnerShape(), p_scale1, p_scale2, p_center_of_mass_transform1, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, new_collide_shape_settings, p_collector, p_shape_filter);
}

void cast_shape_vs_double_sided(const JPH::ShapeCast &p_shape_cast, const JPH::ShapeCastSettings &p_shape_cast_settings, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, const JPH::ShapeFilter &p_shape_filter, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, JPH::CastShapeCollector &p_collector) {
	ERR_FAIL_COND(p_shape->GetSubType() != JoltCustomShapeSubType::DOUBLE_SIDED);

	const JoltCustomDoubleSidedShape *shape = static_cast<const JoltCustomDoubleSidedShape *>(p_shape);

	JPH::ShapeCastSettings new_shape_cast_settings = p_shape_cast_settings;
	if (shape->should_collide_with_back_faces()) {
		new_shape_cast_settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;
	}

	JPH::CollisionDispatch::sCastShapeVsShapeLocalSpace(p_shape_cast, new_shape_cast_settings, shape->GetInnerShape(), p_scale, p_shape_filter, p_center_of_mass_transform2, p_sub_shape_id_creator1, p_sub_shape_id_creator2, p_collector);
}

// --- JoltCustomRayShape helpers ---

class JoltCustomRayShapeSupport final : public JPH::ConvexShape::Support {
public:
	explicit JoltCustomRayShapeSupport(float p_length) :
			length(p_length) {}

	virtual JPH::Vec3 GetSupport(JPH::Vec3Arg p_direction) const override {
		if (p_direction.GetZ() > 0.0f) {
			return JPH::Vec3(0.0f, 0.0f, length);
		} else {
			return JPH::Vec3::sZero();
		}
	}

	virtual float GetConvexRadius() const override { return 0.0f; }

private:
	float length = 0.0f;
};

static_assert(sizeof(JoltCustomRayShapeSupport) <= sizeof(JPH::ConvexShape::SupportBuffer), "Size of SeparationRayShape3D support is larger than size of support buffer.");

JPH::Shape *construct_ray() {
	return new JoltCustomRayShape();
}

void collide_ray_vs_shape(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
	ERR_FAIL_COND(p_shape1->GetSubType() != JoltCustomShapeSubType::RAY);

	const JoltCustomRayShape *shape1 = static_cast<const JoltCustomRayShape *>(p_shape1);

	const float margin = p_collide_shape_settings.mMaxSeparationDistance;
	const float ray_length = shape1->length;
	const float ray_length_padded = ray_length + margin;

	const JPH::Mat44 transform1 = p_center_of_mass_transform1 * JPH::Mat44::sScale(p_scale1);
	const JPH::Mat44 transform2 = p_center_of_mass_transform2 * JPH::Mat44::sScale(p_scale2);
	const JPH::Mat44 transform_inv2 = transform2.Inversed();

	const JPH::Vec3 ray_start = transform1.GetTranslation();
	const JPH::Vec3 ray_direction = transform1.GetAxisZ();
	const JPH::Vec3 ray_vector = ray_direction * ray_length;
	const JPH::Vec3 ray_vector_padded = ray_direction * ray_length_padded;

	const JPH::Vec3 ray_start2 = transform_inv2 * ray_start;
	const JPH::Vec3 ray_direction2 = transform_inv2.Multiply3x3(ray_direction);
	const JPH::Vec3 ray_vector_padded2 = transform_inv2.Multiply3x3(ray_vector_padded);

	const JPH::RayCast ray_cast(ray_start2, ray_vector_padded2);

	JPH::RayCastSettings ray_cast_settings;
	ray_cast_settings.mTreatConvexAsSolid = false;
	ray_cast_settings.mBackFaceModeTriangles = p_collide_shape_settings.mBackFaceMode;

	JoltQueryCollectorClosest<JPH::CastRayCollector> ray_collector;

	p_shape2->CastRay(ray_cast, ray_cast_settings, p_sub_shape_id_creator2, ray_collector);

	if (!ray_collector.had_hit()) {
		return;
	}

	const JPH::RayCastResult &hit = ray_collector.get_hit();

	const float hit_distance = ray_length_padded * hit.mFraction;
	const float hit_depth = ray_length - hit_distance;

	if (-hit_depth >= p_collector.GetEarlyOutFraction()) {
		return;
	}

	JPH::SubShapeID local_sub_shape_id2;
	hit.mSubShapeID2.PopID(p_sub_shape_id_creator2.GetNumBitsWritten(), local_sub_shape_id2);

	const JPH::Vec3 hit_point2 = ray_cast.GetPointOnRay(hit.mFraction);

	const JPH::Vec3 hit_point_on_1 = ray_start + ray_vector;
	const JPH::Vec3 hit_point_on_2 = transform2 * hit_point2;

	JPH::Vec3 hit_normal2 = JPH::Vec3::sZero();

	if (shape1->slide_on_slope) {
		hit_normal2 = p_shape2->GetSurfaceNormal(local_sub_shape_id2, hit_point2);

		if (hit_normal2.Dot(ray_direction2) > 0) {
			hit_normal2 = -hit_normal2;
		}
	} else {
		hit_normal2 = -ray_direction2;
	}

	const JPH::Vec3 hit_normal = transform2.Multiply3x3(hit_normal2);

	JPH::CollideShapeResult result(hit_point_on_1, hit_point_on_2, -hit_normal, hit_depth, p_sub_shape_id_creator1.GetID(), hit.mSubShapeID2, JPH::TransformedShape::sGetBodyID(p_collector.GetContext()));

	if (p_collide_shape_settings.mCollectFacesMode == JPH::ECollectFacesMode::CollectFaces) {
		p_shape2->GetSupportingFace(local_sub_shape_id2, ray_direction2, p_scale2, p_center_of_mass_transform2, result.mShape2Face);
	}

	p_collector.AddHit(result);
}

void collide_noop(const JPH::Shape *p_shape1, const JPH::Shape *p_shape2, JPH::Vec3Arg p_scale1, JPH::Vec3Arg p_scale2, JPH::Mat44Arg p_center_of_mass_transform1, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, const JPH::CollideShapeSettings &p_collide_shape_settings, JPH::CollideShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) {
}

void cast_noop(const JPH::ShapeCast &p_shape_cast, const JPH::ShapeCastSettings &p_shape_cast_settings, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, const JPH::ShapeFilter &p_shape_filter, JPH::Mat44Arg p_center_of_mass_transform2, const JPH::SubShapeIDCreator &p_sub_shape_id_creator1, const JPH::SubShapeIDCreator &p_sub_shape_id_creator2, JPH::CastShapeCollector &p_collector) {
}

// --- JoltCustomMotionShape helpers ---

class JoltMotionConvexSupport final : public JPH::ConvexShape::Support {
public:
	JoltMotionConvexSupport(JPH::Vec3Arg p_motion, const JPH::ConvexShape::Support *p_inner_support) :
			motion(p_motion),
			inner_support(p_inner_support) {}

	virtual JPH::Vec3 GetSupport(JPH::Vec3Arg p_direction) const override {
		JPH::Vec3 support = inner_support->GetSupport(p_direction);

		if (p_direction.Dot(motion) > 0) {
			support += motion;
		}

		return support;
	}

	virtual float GetConvexRadius() const override { return inner_support->GetConvexRadius(); }

private:
	JPH::Vec3 motion = JPH::Vec3::sZero();

	const JPH::ConvexShape::Support *inner_support = nullptr;
};

} // namespace

// --- JoltCustomUserDataShape ---

JPH::ShapeSettings::ShapeResult JoltCustomUserDataShapeSettings::Create() const {
	if (mCachedResult.IsEmpty()) {
		new JoltCustomUserDataShape(*this, mCachedResult);
	}

	return mCachedResult;
}

void JoltCustomUserDataShape::register_type() {
	JPH::ShapeFunctions &shape_functions = JPH::ShapeFunctions::sGet(JoltCustomShapeSubType::OVERRIDE_USER_DATA);

	shape_functions.mConstruct = construct_override_user_data;
	shape_functions.mColor = JPH::Color::sCyan;

	for (const JPH::EShapeSubType sub_type : JPH::sAllSubShapeTypes) {
		JPH::CollisionDispatch::sRegisterCollideShape(JoltCustomShapeSubType::OVERRIDE_USER_DATA, sub_type, collide_override_user_data_vs_shape);
		JPH::CollisionDispatch::sRegisterCollideShape(sub_type, JoltCustomShapeSubType::OVERRIDE_USER_DATA, collide_shape_vs_override_user_data);
		JPH::CollisionDispatch::sRegisterCastShape(JoltCustomShapeSubType::OVERRIDE_USER_DATA, sub_type, cast_override_user_data_vs_shape);
		JPH::CollisionDispatch::sRegisterCastShape(sub_type, JoltCustomShapeSubType::OVERRIDE_USER_DATA, cast_shape_vs_override_user_data);
	}
}

// --- JoltCustomDoubleSidedShape ---

JPH::ShapeSettings::ShapeResult JoltCustomDoubleSidedShapeSettings::Create() const {
	if (mCachedResult.IsEmpty()) {
		new JoltCustomDoubleSidedShape(*this, mCachedResult);
	}

	return mCachedResult;
}

void JoltCustomDoubleSidedShape::register_type() {
	JPH::ShapeFunctions &shape_functions = JPH::ShapeFunctions::sGet(JoltCustomShapeSubType::DOUBLE_SIDED);

	shape_functions.mConstruct = construct_double_sided;
	shape_functions.mColor = JPH::Color::sPurple;

	for (const JPH::EShapeSubType sub_type : JPH::sAllSubShapeTypes) {
		JPH::CollisionDispatch::sRegisterCollideShape(JoltCustomShapeSubType::DOUBLE_SIDED, sub_type, collide_double_sided_vs_shape);
		JPH::CollisionDispatch::sRegisterCollideShape(sub_type, JoltCustomShapeSubType::DOUBLE_SIDED, collide_shape_vs_double_sided);
	}

	for (const JPH::EShapeSubType sub_type : JPH::sConvexSubShapeTypes) {
		JPH::CollisionDispatch::sRegisterCastShape(sub_type, JoltCustomShapeSubType::DOUBLE_SIDED, cast_shape_vs_double_sided);
	}
}

void JoltCustomDoubleSidedShape::CastRay(const JPH::RayCast &p_ray, const JPH::RayCastSettings &p_ray_cast_settings, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CastRayCollector &p_collector, const JPH::ShapeFilter &p_shape_filter) const {
	JPH::RayCastSettings new_ray_cast_settings = p_ray_cast_settings;
	if (!should_collide_with_back_faces()) {
		new_ray_cast_settings.SetBackFaceMode(JPH::EBackFaceMode::IgnoreBackFaces);
	}

	return mInnerShape->CastRay(p_ray, new_ray_cast_settings, p_sub_shape_id_creator, p_collector, p_shape_filter);
}

// --- JoltCustomRayShape ---

JPH::ShapeSettings::ShapeResult JoltCustomRayShapeSettings::Create() const {
	if (mCachedResult.IsEmpty()) {
		new JoltCustomRayShape(*this, mCachedResult);
	}

	return mCachedResult;
}

void JoltCustomRayShape::register_type() {
	JPH::ShapeFunctions &shape_functions = JPH::ShapeFunctions::sGet(JoltCustomShapeSubType::RAY);

	shape_functions.mConstruct = construct_ray;
	shape_functions.mColor = JPH::Color::sDarkRed;

	static constexpr JPH::EShapeSubType concrete_sub_types[] = {
		JPH::EShapeSubType::Sphere,
		JPH::EShapeSubType::Box,
		JPH::EShapeSubType::Triangle,
		JPH::EShapeSubType::Capsule,
		JPH::EShapeSubType::TaperedCapsule,
		JPH::EShapeSubType::Cylinder,
		JPH::EShapeSubType::ConvexHull,
		JPH::EShapeSubType::Mesh,
		JPH::EShapeSubType::HeightField,
		JPH::EShapeSubType::Plane,
		JPH::EShapeSubType::TaperedCylinder
	};

	for (const JPH::EShapeSubType concrete_sub_type : concrete_sub_types) {
		JPH::CollisionDispatch::sRegisterCollideShape(JoltCustomShapeSubType::RAY, concrete_sub_type, collide_ray_vs_shape);
		JPH::CollisionDispatch::sRegisterCollideShape(concrete_sub_type, JoltCustomShapeSubType::RAY, JPH::CollisionDispatch::sReversedCollideShape);
	}

	JPH::CollisionDispatch::sRegisterCollideShape(JoltCustomShapeSubType::RAY, JoltCustomShapeSubType::RAY, collide_noop);

	for (const JPH::EShapeSubType sub_type : JPH::sAllSubShapeTypes) {
		JPH::CollisionDispatch::sRegisterCastShape(JoltCustomShapeSubType::RAY, sub_type, cast_noop);
		JPH::CollisionDispatch::sRegisterCastShape(sub_type, JoltCustomShapeSubType::RAY, cast_noop);
	}
}

JPH::AABox JoltCustomRayShape::GetLocalBounds() const {
	const float radius = GetInnerRadius();
	return JPH::AABox(JPH::Vec3(-radius, -radius, 0.0f), JPH::Vec3(radius, radius, length));
}

float JoltCustomRayShape::GetInnerRadius() const {
	return 0.0001f;
}

JPH::MassProperties JoltCustomRayShape::GetMassProperties() const {
	JPH::MassProperties mass_properties;

	mass_properties.mMass = 1.0f;
	mass_properties.mInertia = JPH::Mat44::sIdentity();

	return mass_properties;
}

#ifdef JPH_DEBUG_RENDERER

void JoltCustomRayShape::Draw(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_use_material_colors, bool p_draw_wireframe) const {
	p_renderer->DrawArrow(p_center_of_mass_transform.GetTranslation(), p_center_of_mass_transform * JPH::Vec3(0, 0, length * p_scale.GetZ()), p_use_material_colors ? GetMaterial()->GetDebugColor() : p_color, 0.1f);
}

#endif

const JPH::ConvexShape::Support *JoltCustomRayShape::GetSupportFunction(JPH::ConvexShape::ESupportMode p_mode, JPH::ConvexShape::SupportBuffer &p_buffer, JPH::Vec3Arg p_scale) const {
	return new (&p_buffer) JoltCustomRayShapeSupport(p_scale.GetZ() * length);
}

// --- JoltCustomMotionShape ---

JPH::AABox JoltCustomMotionShape::GetLocalBounds() const {
	JPH::AABox aabb = inner_shape.GetLocalBounds();
	JPH::AABox aabb_translated = aabb;
	aabb_translated.Translate(motion);
	aabb.Encapsulate(aabb_translated);

	return aabb;
}

void JoltCustomMotionShape::GetSupportingFace(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_direction, JPH::Vec3Arg p_scale, JPH::Mat44Arg p_center_of_mass_transform, JPH::Shape::SupportingFace &p_vertices) const {
}

const JPH::ConvexShape::Support *JoltCustomMotionShape::GetSupportFunction(JPH::ConvexShape::ESupportMode p_mode, JPH::ConvexShape::SupportBuffer &p_buffer, JPH::Vec3Arg p_scale) const {
	return new (&p_buffer) JoltMotionConvexSupport(motion, inner_shape.GetSupportFunction(p_mode, inner_support_buffer, p_scale));
}
