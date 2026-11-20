/**************************************************************************/
/*  jolt_custom_shapes.cpp                                                */
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

#include "jolt_custom_shapes.h"

#include "../spaces/jolt_query_collectors.h"

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
