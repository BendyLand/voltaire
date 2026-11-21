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
#include "../objects/jolt_objects_3d.h"
#include "jolt_custom_shapes.h"

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
