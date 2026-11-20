/**************************************************************************/
/*  jolt_custom_shapes.h                                                  */
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

#pragma once

#include "core/error/error_macros.h"

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/TransformedShape.h>

namespace JoltCustomShapeSubType {

constexpr JPH::EShapeSubType OVERRIDE_USER_DATA = JPH::EShapeSubType::User1;
constexpr JPH::EShapeSubType DOUBLE_SIDED = JPH::EShapeSubType::User2;
constexpr JPH::EShapeSubType RAY = JPH::EShapeSubType::UserConvex1;
constexpr JPH::EShapeSubType MOTION = JPH::EShapeSubType::UserConvex2;

} // namespace JoltCustomShapeSubType

class JoltCustomUserDataShapeSettings final : public JPH::DecoratedShapeSettings {
public:
	using JPH::DecoratedShapeSettings::DecoratedShapeSettings;

	virtual ShapeResult Create() const override;
};

class JoltCustomUserDataShape final : public JPH::DecoratedShape {
public:
	static void register_type();

	JoltCustomUserDataShape() :
			JPH::DecoratedShape(JoltCustomShapeSubType::OVERRIDE_USER_DATA) {}

	JoltCustomUserDataShape(const JoltCustomUserDataShapeSettings &p_settings, ShapeResult &p_result) :
			JPH::DecoratedShape(JoltCustomShapeSubType::OVERRIDE_USER_DATA, p_settings, p_result) {
		if (!p_result.HasError()) {
			p_result.Set(this);
		}
	}

	virtual JPH::AABox GetLocalBounds() const override { return mInnerShape->GetLocalBounds(); }

	virtual JPH::AABox GetWorldSpaceBounds(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { return mInnerShape->GetWorldSpaceBounds(p_center_of_mass_transform, p_scale); }

	virtual float GetInnerRadius() const override { return mInnerShape->GetInnerRadius(); }

	virtual JPH::MassProperties GetMassProperties() const override { return mInnerShape->GetMassProperties(); }

	virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_local_surface_position) const override { return mInnerShape->GetSurfaceNormal(p_sub_shape_id, p_local_surface_position); }

	virtual JPH::uint64 GetSubShapeUserData(const JPH::SubShapeID &p_sub_shape_id) const override { return GetUserData(); }

	virtual JPH::TransformedShape GetSubShapeTransformedShape(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, JPH::SubShapeID &p_remainder) const override { return mInnerShape->GetSubShapeTransformedShape(p_sub_shape_id, p_position_com, p_rotation, p_scale, p_remainder); }

	virtual void GetSubmergedVolume(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::Plane &p_surface, float &p_total_volume, float &p_submerged_volume, JPH::Vec3 &p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_base_offset)) const override { mInnerShape->GetSubmergedVolume(p_center_of_mass_transform, p_scale, p_surface, p_total_volume, p_submerged_volume, p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, p_base_offset)); }

#ifdef JPH_DEBUG_RENDERER
	virtual void Draw(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_use_material_colors, bool p_draw_wireframe) const override { mInnerShape->Draw(p_renderer, p_center_of_mass_transform, p_scale, p_color, p_use_material_colors, p_draw_wireframe); }

	virtual void DrawGetSupportFunction(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_draw_support_direction) const override { mInnerShape->DrawGetSupportFunction(p_renderer, p_center_of_mass_transform, p_scale, p_color, p_draw_support_direction); }

	virtual void DrawGetSupportingFace(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { mInnerShape->DrawGetSupportingFace(p_renderer, p_center_of_mass_transform, p_scale); }
#endif

	virtual bool CastRay(const JPH::RayCast &p_ray, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::RayCastResult &p_hit) const override { return mInnerShape->CastRay(p_ray, p_sub_shape_id_creator, p_hit); }

	virtual void CastRay(const JPH::RayCast &p_ray, const JPH::RayCastSettings &p_ray_cast_settings, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CastRayCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { return mInnerShape->CastRay(p_ray, p_ray_cast_settings, p_sub_shape_id_creator, p_collector, p_shape_filter); }

	virtual void CollidePoint(JPH::Vec3Arg p_point, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CollidePointCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { mInnerShape->CollidePoint(p_point, p_sub_shape_id_creator, p_collector, p_shape_filter); }

	virtual void CollideSoftBodyVertices(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::CollideSoftBodyVertexIterator &p_vertices, JPH::uint p_num_vertices, int p_colliding_shape_index) const override { mInnerShape->CollideSoftBodyVertices(p_center_of_mass_transform, p_scale, p_vertices, p_num_vertices, p_colliding_shape_index); }

	virtual void CollectTransformedShapes(const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::TransformedShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { mInnerShape->CollectTransformedShapes(p_box, p_position_com, p_rotation, p_scale, p_sub_shape_id_creator, p_collector, p_shape_filter); }

	virtual void TransformShape(JPH::Mat44Arg p_center_of_mass_transform, JPH::TransformedShapeCollector &p_collector) const override { mInnerShape->TransformShape(p_center_of_mass_transform, p_collector); }

	virtual void GetTrianglesStart(JPH::Shape::GetTrianglesContext &p_context, const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale) const override { mInnerShape->GetTrianglesStart(p_context, p_box, p_position_com, p_rotation, p_scale); }

	virtual int GetTrianglesNext(JPH::Shape::GetTrianglesContext &p_context, int p_max_triangles_requested, JPH::Float3 *p_triangle_vertices, const JPH::PhysicsMaterial **p_materials = nullptr) const override { return mInnerShape->GetTrianglesNext(p_context, p_max_triangles_requested, p_triangle_vertices, p_materials); }

	virtual JPH::Shape::Stats GetStats() const override { return JPH::Shape::Stats(sizeof(*this), 0); }

	virtual float GetVolume() const override { return mInnerShape->GetVolume(); }
};

class JoltCustomDoubleSidedShapeSettings final : public JPH::DecoratedShapeSettings {
public:
	bool back_face_collision = false;

	JoltCustomDoubleSidedShapeSettings() = default;

	JoltCustomDoubleSidedShapeSettings(const ShapeSettings *p_inner_settings, bool p_back_face_collision) :
			JPH::DecoratedShapeSettings(p_inner_settings), back_face_collision(p_back_face_collision) {}

	JoltCustomDoubleSidedShapeSettings(const JPH::Shape *p_inner_shape, bool p_back_face_collision) :
			JPH::DecoratedShapeSettings(p_inner_shape), back_face_collision(p_back_face_collision) {}

	virtual JPH::Shape::ShapeResult Create() const override;
};

class JoltCustomDoubleSidedShape final : public JPH::DecoratedShape {
	bool back_face_collision = false;

public:
	static void register_type();

	JoltCustomDoubleSidedShape() :
			JPH::DecoratedShape(JoltCustomShapeSubType::DOUBLE_SIDED) {}

	JoltCustomDoubleSidedShape(const JoltCustomDoubleSidedShapeSettings &p_settings, JPH::Shape::ShapeResult &p_result) :
			JPH::DecoratedShape(JoltCustomShapeSubType::DOUBLE_SIDED, p_settings, p_result), back_face_collision(p_settings.back_face_collision) {
		if (!p_result.HasError()) {
			p_result.Set(this);
		}
	}

	JoltCustomDoubleSidedShape(const JPH::Shape *p_inner_shape, bool p_back_face_collision) :
			JPH::DecoratedShape(JoltCustomShapeSubType::DOUBLE_SIDED, p_inner_shape), back_face_collision(p_back_face_collision) {}

	virtual JPH::AABox GetLocalBounds() const override { return mInnerShape->GetLocalBounds(); }

	virtual JPH::AABox GetWorldSpaceBounds(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { return mInnerShape->GetWorldSpaceBounds(p_center_of_mass_transform, p_scale); }

	virtual float GetInnerRadius() const override { return mInnerShape->GetInnerRadius(); }

	virtual JPH::MassProperties GetMassProperties() const override { return mInnerShape->GetMassProperties(); }

	virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_local_surface_position) const override { return mInnerShape->GetSurfaceNormal(p_sub_shape_id, p_local_surface_position); }

	virtual JPH::uint64 GetSubShapeUserData(const JPH::SubShapeID &p_sub_shape_id) const override { return mInnerShape->GetSubShapeUserData(p_sub_shape_id); }

	virtual JPH::TransformedShape GetSubShapeTransformedShape(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, JPH::SubShapeID &p_remainder) const override { return mInnerShape->GetSubShapeTransformedShape(p_sub_shape_id, p_position_com, p_rotation, p_scale, p_remainder); }

	virtual void GetSubmergedVolume(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::Plane &p_surface, float &p_total_volume, float &p_submerged_volume, JPH::Vec3 &p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_base_offset)) const override { mInnerShape->GetSubmergedVolume(p_center_of_mass_transform, p_scale, p_surface, p_total_volume, p_submerged_volume, p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, p_base_offset)); }

#ifdef JPH_DEBUG_RENDERER
	virtual void Draw(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_use_material_colors, bool p_draw_wireframe) const override { mInnerShape->Draw(p_renderer, p_center_of_mass_transform, p_scale, p_color, p_use_material_colors, p_draw_wireframe); }

	virtual void DrawGetSupportFunction(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_draw_support_direction) const override { mInnerShape->DrawGetSupportFunction(p_renderer, p_center_of_mass_transform, p_scale, p_color, p_draw_support_direction); }

	virtual void DrawGetSupportingFace(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { mInnerShape->DrawGetSupportingFace(p_renderer, p_center_of_mass_transform, p_scale); }
#endif

	virtual bool CastRay(const JPH::RayCast &p_ray, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::RayCastResult &p_hit) const override { return mInnerShape->CastRay(p_ray, p_sub_shape_id_creator, p_hit); }

	virtual void CastRay(const JPH::RayCast &p_ray, const JPH::RayCastSettings &p_ray_cast_settings, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CastRayCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override;

	virtual void CollidePoint(JPH::Vec3Arg p_point, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CollidePointCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { mInnerShape->CollidePoint(p_point, p_sub_shape_id_creator, p_collector, p_shape_filter); }

	virtual void CollideSoftBodyVertices(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::CollideSoftBodyVertexIterator &p_vertices, JPH::uint p_num_vertices, int p_colliding_shape_index) const override { mInnerShape->CollideSoftBodyVertices(p_center_of_mass_transform, p_scale, p_vertices, p_num_vertices, p_colliding_shape_index); }

	virtual void CollectTransformedShapes(const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::TransformedShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { mInnerShape->CollectTransformedShapes(p_box, p_position_com, p_rotation, p_scale, p_sub_shape_id_creator, p_collector, p_shape_filter); }

	virtual void TransformShape(JPH::Mat44Arg p_center_of_mass_transform, JPH::TransformedShapeCollector &p_collector) const override { mInnerShape->TransformShape(p_center_of_mass_transform, p_collector); }

	virtual void GetTrianglesStart(JPH::Shape::GetTrianglesContext &p_context, const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale) const override { mInnerShape->GetTrianglesStart(p_context, p_box, p_position_com, p_rotation, p_scale); }

	virtual int GetTrianglesNext(JPH::Shape::GetTrianglesContext &p_context, int p_max_triangles_requested, JPH::Float3 *p_triangle_vertices, const JPH::PhysicsMaterial **p_materials = nullptr) const override { return mInnerShape->GetTrianglesNext(p_context, p_max_triangles_requested, p_triangle_vertices, p_materials); }

	virtual JPH::Shape::Stats GetStats() const override { return JPH::Shape::Stats(sizeof(*this), 0); }

	virtual float GetVolume() const override { return mInnerShape->GetVolume(); }

	bool should_collide_with_back_faces() const { return back_face_collision; }
};

class JoltCustomRayShapeSettings final : public JPH::ConvexShapeSettings {
public:
	JPH::RefConst<JPH::PhysicsMaterial> material;
	float length = 1.0f;
	bool slide_on_slope = false;

	JoltCustomRayShapeSettings() = default;
	JoltCustomRayShapeSettings(float p_length, bool p_slide_on_slope, const JPH::PhysicsMaterial *p_material = nullptr) :
			material(p_material), length(p_length), slide_on_slope(p_slide_on_slope) {}

	virtual JPH::ShapeSettings::ShapeResult Create() const override;
};

class JoltCustomRayShape final : public JPH::ConvexShape {
public:
	JPH::RefConst<JPH::PhysicsMaterial> material;
	float length = 0.0f;
	bool slide_on_slope = false;

	static void register_type();

	JoltCustomRayShape() :
			JPH::ConvexShape(JoltCustomShapeSubType::RAY) {}

	JoltCustomRayShape(const JoltCustomRayShapeSettings &p_settings, JPH::Shape::ShapeResult &p_result) :
			JPH::ConvexShape(JoltCustomShapeSubType::RAY, p_settings, p_result), material(p_settings.material), length(p_settings.length), slide_on_slope(p_settings.slide_on_slope) {
		if (!p_result.HasError()) {
			p_result.Set(this);
		}
	}

	JoltCustomRayShape(float p_length, bool p_slide_on_slope, const JPH::PhysicsMaterial *p_material = nullptr) :
			JPH::ConvexShape(JoltCustomShapeSubType::RAY), material(p_material), length(p_length), slide_on_slope(p_slide_on_slope) {}

	virtual JPH::AABox GetLocalBounds() const override;

	virtual float GetInnerRadius() const override;

	virtual JPH::MassProperties GetMassProperties() const override;

	virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_local_surface_position) const override { return JPH::Vec3::sAxisZ(); }

	virtual void GetSubmergedVolume(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::Plane &p_surface, float &p_total_volume, float &p_submerged_volume, JPH::Vec3 &p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_base_offset)) const override {
		p_total_volume = 0.0f;
		p_submerged_volume = 0.0f;
		p_center_of_buoyancy = JPH::Vec3::sZero();
	}

#ifdef JPH_DEBUG_RENDERER
	virtual void Draw(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_use_material_colors, bool p_draw_wireframe) const override;
#endif

	virtual bool CastRay(const JPH::RayCast &p_ray, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::RayCastResult &p_hit) const override { return false; }

	virtual void CastRay(const JPH::RayCast &p_ray, const JPH::RayCastSettings &p_ray_cast_settings, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CastRayCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override {}

	virtual void CollidePoint(JPH::Vec3Arg p_point, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CollidePointCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override {}

	virtual void CollideSoftBodyVertices(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::CollideSoftBodyVertexIterator &p_vertices, JPH::uint p_num_vertices, int p_colliding_shape_index) const override {}

	virtual JPH::Shape::Stats GetStats() const override { return JPH::Shape::Stats(sizeof(*this), 0); }

	virtual float GetVolume() const override { return 0.0f; }

	virtual const JPH::ConvexShape::Support *GetSupportFunction(JPH::ConvexShape::ESupportMode p_mode, JPH::ConvexShape::SupportBuffer &p_buffer, JPH::Vec3Arg p_scale) const override;
};

class JoltCustomMotionShape final : public JPH::ConvexShape {
	mutable JPH::ConvexShape::SupportBuffer inner_support_buffer;

	JPH::Vec3 motion = JPH::Vec3::sZero();

	const JPH::ConvexShape &inner_shape;

public:
	explicit JoltCustomMotionShape(const JPH::ConvexShape &p_shape) :
			JPH::ConvexShape(JoltCustomShapeSubType::MOTION), inner_shape(p_shape) {}

	virtual bool MustBeStatic() const override { return false; }

	virtual JPH::Vec3 GetCenterOfMass() const override { ERR_FAIL_V_MSG(JPH::Vec3::sZero(), "Not implemented."); }

	virtual JPH::AABox GetLocalBounds() const override;

	virtual JPH::uint GetSubShapeIDBitsRecursive() const override { ERR_FAIL_V_MSG(0, "Not implemented."); }

	virtual JPH::AABox GetWorldSpaceBounds(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { ERR_FAIL_V_MSG(JPH::AABox(), "Not implemented."); }

	virtual float GetInnerRadius() const override { ERR_FAIL_V_MSG(0.0f, "Not implemented."); }

	virtual JPH::MassProperties GetMassProperties() const override { ERR_FAIL_V_MSG(JPH::MassProperties(), "Not implemented."); }

	virtual const JPH::PhysicsMaterial *GetMaterial(const JPH::SubShapeID &p_sub_shape_id) const override { ERR_FAIL_V_MSG(nullptr, "Not implemented."); }

	virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_local_surface_position) const override { ERR_FAIL_V_MSG(JPH::Vec3::sZero(), "Not implemented."); }

	virtual void GetSupportingFace(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_direction, JPH::Vec3Arg p_scale, JPH::Mat44Arg p_center_of_mass_transform, JPH::Shape::SupportingFace &p_vertices) const override;

	virtual JPH::uint64 GetSubShapeUserData(const JPH::SubShapeID &p_sub_shape_id) const override { ERR_FAIL_V_MSG(0, "Not implemented."); }

	virtual JPH::TransformedShape GetSubShapeTransformedShape(const JPH::SubShapeID &p_sub_shape_id, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, JPH::SubShapeID &p_remainder) const override { ERR_FAIL_V_MSG(JPH::TransformedShape(), "Not implemented."); }

	virtual void GetSubmergedVolume(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::Plane &p_surface, float &p_total_volume, float &p_submerged_volume, JPH::Vec3 &p_center_of_buoyancy JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_base_offset)) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual const JPH::ConvexShape::Support *GetSupportFunction(JPH::ConvexShape::ESupportMode p_mode, JPH::ConvexShape::SupportBuffer &p_buffer, JPH::Vec3Arg p_scale) const override;

#ifdef JPH_DEBUG_RENDERER
	virtual void Draw(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_use_material_colors, bool p_draw_wireframe) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void DrawGetSupportFunction(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, JPH::ColorArg p_color, bool p_draw_support_direction) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void DrawGetSupportingFace(JPH::DebugRenderer *p_renderer, JPH::RMat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale) const override { ERR_FAIL_MSG("Not implemented."); }
#endif

	virtual bool CastRay(const JPH::RayCast &p_ray, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::RayCastResult &p_hit) const override { ERR_FAIL_V_MSG(false, "Not implemented."); }

	virtual void CastRay(const JPH::RayCast &p_ray, const JPH::RayCastSettings &p_ray_cast_settings, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CastRayCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void CollidePoint(JPH::Vec3Arg p_point, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::CollidePointCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void CollideSoftBodyVertices(JPH::Mat44Arg p_center_of_mass_transform, JPH::Vec3Arg p_scale, const JPH::CollideSoftBodyVertexIterator &p_vertices, JPH::uint p_num_vertices, int p_colliding_shape_index) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void CollectTransformedShapes(const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale, const JPH::SubShapeIDCreator &p_sub_shape_id_creator, JPH::TransformedShapeCollector &p_collector, const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void TransformShape(JPH::Mat44Arg p_center_of_mass_transform, JPH::TransformedShapeCollector &p_collector) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual void GetTrianglesStart(JPH::Shape::GetTrianglesContext &p_context, const JPH::AABox &p_box, JPH::Vec3Arg p_position_com, JPH::QuatArg p_rotation, JPH::Vec3Arg p_scale) const override { ERR_FAIL_MSG("Not implemented."); }

	virtual int GetTrianglesNext(JPH::Shape::GetTrianglesContext &p_context, int p_max_triangles_requested, JPH::Float3 *p_triangle_vertices, const JPH::PhysicsMaterial **p_materials = nullptr) const override { ERR_FAIL_V_MSG(0, "Not implemented."); }

	virtual JPH::Shape::Stats GetStats() const override { return JPH::Shape::Stats(sizeof(*this), 0); }

	virtual float GetVolume() const override { ERR_FAIL_V_MSG(0.0f, "Not implemented."); }

	virtual bool IsValidScale(JPH::Vec3Arg p_scale) const override { ERR_FAIL_V_MSG(false, "Not implemented."); }

	const JPH::ConvexShape &get_inner_shape() const { return inner_shape; }

	void set_motion(JPH::Vec3Arg p_motion) { motion = p_motion; }
};
