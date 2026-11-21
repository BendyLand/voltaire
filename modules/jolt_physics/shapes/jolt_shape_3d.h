/**************************************************************************/
/*  jolt_shape_3d.h                                                       */
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

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "core/math/aabb.h"
#include "core/math/plane.h"
#include "core/math/transform_3d.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid.h"
#include "core/types.h"
#include "servers/physics_3d/physics_server_3d.h"

class JoltObject3D;
class JoltShape3D;

class JoltShapeInstance3D final {
	// This RAII helper exists solely to avoid needing to maintain move construction/assignment in `JoltShapeInstance3D`.
	struct ShapeReference {
		JoltObject3D *parent = nullptr;
		JoltShape3D *shape = nullptr;

		ShapeReference() = default;
		ShapeReference(JoltObject3D *p_parent, JoltShape3D *p_shape);
		ShapeReference(const ShapeReference &p_other) = delete;
		ShapeReference(ShapeReference &&p_other);
		~ShapeReference();

		ShapeReference &operator=(const ShapeReference &p_other) = delete;
		ShapeReference &operator=(ShapeReference &&p_other);

		JoltShape3D *operator*() const { return shape; }
		JoltShape3D *operator->() const { return shape; }
		operator JoltShape3D *() const { return shape; }
	};

	inline static uint32_t next_id = 1;

	Transform3D transform;
	Vector3 scale;
	ShapeReference shape;
	JPH::ShapeRefC jolt_ref;
	uint32_t id = next_id++;
	bool disabled = false;

public:
	JoltShapeInstance3D() = default;
	JoltShapeInstance3D(JoltObject3D *p_parent, JoltShape3D *p_shape, const Transform3D &p_transform = Transform3D(), const Vector3 &p_scale = Vector3(1.0f, 1.0f, 1.0f), bool p_disabled = false);

	uint32_t get_id() const { return id; }

	JoltShape3D *get_shape() const { return shape; }

	const JPH::Shape *get_jolt_ref() const { return jolt_ref; }

	const Transform3D &get_transform_unscaled() const { return transform; }
	Transform3D get_transform_scaled() const { return transform.scaled_local(scale); }
	void set_transform(const Transform3D &p_transform) { transform = p_transform; }

	const Vector3 &get_scale() const { return scale; }
	void set_scale(const Vector3 &p_scale) { scale = p_scale; }

	AABB get_aabb() const;

	bool is_built() const { return jolt_ref != nullptr; }

	bool is_enabled() const { return !disabled; }
	bool is_disabled() const { return disabled; }

	void enable() { disabled = false; }
	void disable() { disabled = true; }

	bool try_build();
};

class JoltShape3D final {
public:
	typedef PS3DE::ShapeType ShapeType;

#ifdef REAL_T_IS_DOUBLE
	typedef PackedFloat64Array HeightArray;
#else
	typedef PackedFloat32Array HeightArray;
#endif

private:
	HashMap<JoltObject3D *, int> ref_counts_by_owner;
	Mutex jolt_ref_mutex;
	RID rid;
	JPH::ShapeRefC jolt_ref;
	ShapeType shape_type = ShapeType::SHAPE_CUSTOM;

	AABB aabb;
	Plane plane;
	Vector3 half_extents;
	PackedVector3Array vertices;
	HeightArray heights;

	float margin = 0.04f;
	float radius = 0.0f;
	float height = 0.0f;
	float length = 0.0f;

	int width = 0;
	int depth = 0;

	bool slide_on_slope = false;
	bool back_face_collision = false;

	JPH::ShapeRefC _build() const;
	JPH::ShapeRefC _build_separation_ray() const;
	JPH::ShapeRefC _build_sphere() const;
	JPH::ShapeRefC _build_capsule() const;
	JPH::ShapeRefC _build_height_map() const;
	JPH::ShapeRefC _build_height_field() const;
	JPH::ShapeRefC _build_mesh() const;

	AABB _calculate_convex_aabb() const;
	AABB _calculate_heightmap_aabb() const;

	String _owners_to_string() const;

public:
	JoltShape3D() = default;
	explicit JoltShape3D(ShapeType p_type) :
			shape_type(p_type) {}
	~JoltShape3D() = default;

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	void add_owner(JoltObject3D *p_owner);
	void remove_owner(JoltObject3D *p_owner);
	void remove_self();

	ShapeType get_type() const { return shape_type; }
	bool is_convex() const;

	float get_margin() const;
	void set_margin(float p_margin);

	AABB get_aabb() const;

	float get_solver_bias() const;
	void set_solver_bias(float p_bias);

	String to_string() const;

	// Typed shape parameter getters/setters
	const Plane &get_plane() const { return plane; }
	void set_plane(const Plane &p_plane);

	float get_ray_length() const { return length; }
	void set_ray_length(float p_length);

	bool get_slide_on_slope() const { return slide_on_slope; }
	void set_slide_on_slope(bool p_slide_on_slope);

	float get_radius() const { return radius; }
	void set_radius(float p_radius);

	float get_height() const { return height; }
	void set_height(float p_height);

	const Vector3 &get_half_extents() const { return half_extents; }
	void set_half_extents(const Vector3 &p_half_extents);

	const PackedVector3Array &get_vertices() const { return vertices; }
	void set_vertices(const PackedVector3Array &p_vertices);

	bool get_back_face_collision() const { return back_face_collision; }
	void set_back_face_collision(bool p_back_face_collision);
	void set_concave_aabb(const AABB &p_aabb);

	const HeightArray &get_heights() const { return heights; }
	int get_width() const { return width; }
	int get_depth() const { return depth; }
	void set_heightmap(const HeightArray &p_heights, int p_width, int p_depth);

	JPH::ShapeRefC try_build();

	void destroy();

	const JPH::Shape *get_jolt_ref() const { return jolt_ref; }

	static JPH::ShapeRefC with_scale(const JPH::Shape *p_shape, const Vector3 &p_scale);
	static JPH::ShapeRefC with_basis_origin(const JPH::Shape *p_shape, const Basis &p_basis, const Vector3 &p_origin);
	static JPH::ShapeRefC with_center_of_mass_offset(const JPH::Shape *p_shape, const Vector3 &p_offset);
	static JPH::ShapeRefC with_center_of_mass(const JPH::Shape *p_shape, const Vector3 &p_center_of_mass);
	static JPH::ShapeRefC with_user_data(const JPH::Shape *p_shape, uint64_t p_user_data);
	static JPH::ShapeRefC with_double_sided(const JPH::Shape *p_shape, bool p_back_face_collision);
	static JPH::ShapeRefC without_custom_shapes(const JPH::Shape *p_shape);

	static Vector3 make_scale_valid(const JPH::Shape *p_shape, const Vector3 &p_scale);
	static bool is_scale_valid(const Vector3 &p_scale, const Vector3 &p_valid_scale, real_t p_tolerance = 0.01f);
};

#ifdef DEBUG_ENABLED

#define JOLT_ENSURE_SCALE_NOT_ZERO(m_transform, m_msg)                                             \
	if (unlikely((m_transform).basis.determinant() == 0.0f)) {                                     \
		WARN_PRINT(vformat(                                                                        \
			"%s "                                                                                  \
			"The basis of the transform was singular, which is not supported by Jolt Physics. "    \
			"This is likely caused by one or more axes having a scale of zero. "                   \
			"The basis (and thus its scale) will be treated as identity.",                         \
			m_msg));                                                                               \
                                                                                                   \
		(m_transform).basis = Basis();                                                             \
	} else                                                                                         \
		((void)0)

#define ERR_PRINT_INVALID_SCALE_MSG(m_scale, m_valid_scale, m_msg)                                 \
	if (unlikely(!JoltShape3D::is_scale_valid(m_scale, valid_scale))) {                            \
		ERR_PRINT(vformat("%s "                                                                    \
						  "A scale of %v is not supported by Jolt Physics for this shape/body. "   \
						  "The scale will instead be treated as %v.",                              \
			m_msg, m_scale, valid_scale));                                                         \
	} else                                                                                         \
		((void)0)

#else

#define JOLT_ENSURE_SCALE_NOT_ZERO(m_transform, m_msg)

#define ERR_PRINT_INVALID_SCALE_MSG(m_scale, m_valid_scale, m_msg)

#endif

#define JOLT_ENSURE_SCALE_VALID(m_shape, m_scale, m_msg)                                           \
	if (true) {                                                                                    \
		const Vector3 valid_scale = JoltShape3D::make_scale_valid(m_shape, m_scale);               \
		ERR_PRINT_INVALID_SCALE_MSG(m_scale, valid_scale, m_msg);                                  \
		(m_scale) = valid_scale;                                                                   \
	} else                                                                                         \
		((void)0)

// --- Folded from modules/jolt_physics/shapes/jolt_custom_shapes.h ---
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

// --- End modules/jolt_physics/shapes/jolt_custom_shapes.h ---

// --- Folded from modules/jolt_physics/objects/jolt_objects_3d.h ---


#include "core/math/vector3.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
#include "servers/physics_3d/physics_server_3d.h"

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>

class JoltArea3D;
class JoltBody3D;
class JoltJoint3D;
class JoltObject3D;
class JoltSoftBody3D;
class JoltSpace3D;
class PhysicsServer3DRenderingServerHandler;

class JoltGroupFilter final : public JPH::GroupFilter {
	virtual bool CanCollide(const JPH::CollisionGroup &p_group1, const JPH::CollisionGroup &p_group2) const override;

public:
	inline static JoltGroupFilter *instance = nullptr;

	static void encode_object(const JoltObject3D *p_object, JPH::CollisionGroup::GroupID &r_group_id, JPH::CollisionGroup::SubGroupID &r_sub_group_id);
	static const JoltObject3D *decode_object(JPH::CollisionGroup::GroupID p_group_id, JPH::CollisionGroup::SubGroupID p_sub_group_id);
};

class JoltObject3D {
	friend class JoltShape3D;

public:
	enum ObjectType : char {
		OBJECT_TYPE_INVALID,
		OBJECT_TYPE_BODY,
		OBJECT_TYPE_SOFT_BODY,
		OBJECT_TYPE_AREA,
	};

protected:
	RID rid;
	JoltSpace3D *space = nullptr;
	JPH::Body *jolt_body = nullptr;

	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;

	ObjectType object_type = OBJECT_TYPE_INVALID;

	bool pickable = false;

	// Shaped object state (used by JoltArea3D and JoltBody3D)
	SelfList<JoltObject3D> shapes_changed_element;
	SelfList<JoltObject3D> needs_optimization_element;

	Vector3 scale = Vector3(1, 1, 1);

	JPH::ShapeRefC jolt_shape;
	JPH::ShapeRefC previous_jolt_shape;

	JPH::BodyCreationSettings *jolt_settings = nullptr;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const = 0;
	virtual JPH::ObjectLayer _get_object_layer() const = 0;
	virtual JPH::EMotionType _get_motion_type() const { return JPH::EMotionType::Dynamic; }

	virtual void _add_to_space() = 0;
	virtual void _remove_from_space();

	void _reset_space();

	void _update_object_layer();

	virtual void _collision_layer_changed();
	virtual void _collision_mask_changed();

	bool _is_big() const;

	JPH::ShapeRefC _try_build_shape(bool p_optimize_compound);
	JPH::ShapeRefC _try_build_single_shape();
	JPH::ShapeRefC _try_build_compound_shape(bool p_optimize);

	void _enqueue_shapes_changed();
	void _dequeue_shapes_changed();

	void _enqueue_needs_optimization();
	void _dequeue_needs_optimization();

	virtual void _shapes_changed();
	virtual void _shapes_committed();
	virtual void _space_changing();
	virtual void _space_changed() {}

public:
	explicit JoltObject3D(ObjectType p_object_type);
	virtual ~JoltObject3D() = 0;

	ObjectType get_type() const { return object_type; }

	bool is_body() const { return object_type == OBJECT_TYPE_BODY; }
	bool is_soft_body() const { return object_type == OBJECT_TYPE_SOFT_BODY; }
	bool is_area() const { return object_type == OBJECT_TYPE_AREA; }
	bool is_shaped() const { return object_type != OBJECT_TYPE_SOFT_BODY; }

	JoltBody3D *as_body() { return is_body() ? reinterpret_cast<JoltBody3D *>(this) : nullptr; }
	const JoltBody3D *as_body() const { return is_body() ? reinterpret_cast<const JoltBody3D *>(this) : nullptr; }

	JoltSoftBody3D *as_soft_body() { return is_soft_body() ? reinterpret_cast<JoltSoftBody3D *>(this) : nullptr; }
	const JoltSoftBody3D *as_soft_body() const { return is_soft_body() ? reinterpret_cast<const JoltSoftBody3D *>(this) : nullptr; }

	JoltArea3D *as_area() { return is_area() ? reinterpret_cast<JoltArea3D *>(this) : nullptr; }
	const JoltArea3D *as_area() const { return is_area() ? reinterpret_cast<const JoltArea3D *>(this) : nullptr; }

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	JPH::Body *get_jolt_body() const { return jolt_body; }
	JPH::BodyID get_jolt_id() const { return jolt_body->GetID(); }

	JoltSpace3D *get_space() const { return space; }
	void set_space(JoltSpace3D *p_space);
	bool in_space() const { return space != nullptr && jolt_body != nullptr; }

	uint32_t get_collision_layer() const { return collision_layer; }
	void set_collision_layer(uint32_t p_layer);

	uint32_t get_collision_mask() const { return collision_mask; }
	void set_collision_mask(uint32_t p_mask);

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const = 0;

	bool is_pickable() const { return pickable; }
	void set_pickable(bool p_enabled) { pickable = p_enabled; }

	bool can_collide_with(const JoltObject3D &p_other) const;
	bool can_interact_with(const JoltObject3D &p_other) const;

	virtual bool can_interact_with(const JoltBody3D &p_other) const = 0;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const = 0;
	virtual bool can_interact_with(const JoltArea3D &p_other) const = 0;

	virtual bool reports_contacts() const = 0;

	virtual void pre_step(float p_step) {}

	String to_string() const;

	// Shaped object API
	Transform3D get_transform_unscaled() const;
	Transform3D get_transform_scaled() const;

	Vector3 get_scale() const { return scale; }
	Basis get_basis() const;
	Vector3 get_position() const;

	Vector3 get_center_of_mass() const;
	Vector3 get_center_of_mass_relative() const;
	Vector3 get_center_of_mass_local() const;

	Vector3 get_linear_velocity() const;
	Vector3 get_angular_velocity() const;

	AABB get_aabb() const;

	virtual bool has_custom_center_of_mass() const { return false; }
	virtual Vector3 get_center_of_mass_custom() const { return Vector3(); }

	JPH::ShapeRefC build_shapes(bool p_optimize_compound);

	void commit_shapes(bool p_optimize_compound);

	const JPH::Shape *get_jolt_shape() const { return jolt_shape; }
	const JPH::Shape *get_previous_jolt_shape() const { return previous_jolt_shape; }

	void add_shape(JoltShape3D *p_shape, Transform3D p_transform, bool p_disabled);
	void remove_shape(const JoltShape3D *p_shape);
	void remove_shape(int p_index);

	JoltShape3D *get_shape(int p_index) const;
	void set_shape(int p_index, JoltShape3D *p_shape);

	void clear_shapes();
	void clear_previous_shape();

	int find_shape_index(uint32_t p_shape_instance_id) const;
	int find_shape_index(const JPH::SubShapeID &p_sub_shape_id) const;

	JoltShape3D *find_shape(uint32_t p_shape_instance_id) const;
	JoltShape3D *find_shape(const JPH::SubShapeID &p_sub_shape_id) const;

	Transform3D get_shape_transform_unscaled(int p_index) const;
	Transform3D get_shape_transform_scaled(int p_index) const;
	void set_shape_transform(int p_index, Transform3D p_transform);

	Vector3 get_shape_scale(int p_index) const;

	bool is_shape_disabled(int p_index) const;
	void set_shape_disabled(int p_index, bool p_disabled);
};

class JoltArea3D final : public JoltObject3D {
public:
	typedef PS3DE::AreaSpaceOverrideMode OverrideMode;

private:
	struct BodyIDHasher {
		static uint32_t hash(const JPH::BodyID &p_id) {
			return hash_fmix32(p_id.GetIndexAndSequenceNumber());
		}
	};

	struct ShapeIDPair {
		JPH::SubShapeID other;
		JPH::SubShapeID self;

		ShapeIDPair(JPH::SubShapeID p_other, JPH::SubShapeID p_self) :
				other(p_other), self(p_self) {}

		static uint32_t hash(const ShapeIDPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.other.GetValue());
			hash = hash_murmur3_one_32(p_pair.self.GetValue(), hash);
			return hash_fmix32(hash);
		}

		friend bool operator==(const ShapeIDPair &p_lhs, const ShapeIDPair &p_rhs) {
			return (p_lhs.other == p_rhs.other) && (p_lhs.self == p_rhs.self);
		}
	};

	struct ShapeIndexPair {
		int other = -1;
		int self = -1;

		ShapeIndexPair() = default;
		ShapeIndexPair(int p_other, int p_self) :
				other(p_other), self(p_self) {}

		static uint32_t hash(const ShapeIndexPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.other);
			hash = hash_murmur3_one_32(p_pair.self, hash);
			return hash_fmix32(hash);
		}

		friend bool operator==(const ShapeIndexPair &p_lhs, const ShapeIndexPair &p_rhs) {
			return (p_lhs.other == p_rhs.other) && (p_lhs.self == p_rhs.self);
		}
	};

	SelfList<JoltArea3D> call_queries_element;

	Vector3 gravity_vector = Vector3(0, -1, 0);
	Vector3 wind_source;
	Vector3 wind_direction;

	float priority = 0.0f;
	float gravity = 9.8f;
	float point_gravity_distance = 0.0f;
	float linear_damp = 0.1f;
	float angular_damp = 0.1f;
	float wind_pressure = 0.0f;
	float wind_attenuation_factor = 0.0f;

	OverrideMode gravity_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;
	OverrideMode linear_damp_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;
	OverrideMode angular_damp_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;

	bool monitorable = false;
	bool point_gravity = false;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual JPH::EMotionType _get_motion_type() const override {
		return JPH::EMotionType::Kinematic;
	}

	virtual void _add_to_space() override;

	void _enqueue_call_queries();
	void _dequeue_call_queries();

	void _notify_body_entered(const JPH::BodyID &p_body_id);
	void _notify_body_exited(const JPH::BodyID &p_body_id);
	void _notify_bodies_updated(bool p_priority_changed = false);

	void _remove_all_overlaps();

	void _update_sleeping();
	void _update_group_filter();

	virtual void _space_changing() override;
	virtual void _space_changed() override;
	void _events_changed();
	void _body_monitoring_changed();
	void _area_monitoring_changed();
	void _monitorable_changed();

public:
	JoltArea3D();

	void set_transform(Transform3D p_transform);

	bool is_monitorable() const { return monitorable; }
	void set_monitorable(bool p_monitorable);

	bool can_monitor(const JoltBody3D &p_other) const;
	bool can_monitor(const JoltSoftBody3D &p_other) const;
	bool can_monitor(const JoltArea3D &p_other) const;

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override {
		return Vector3();
	}

	virtual bool reports_contacts() const override { return false; }

	float get_priority() const { return priority; }
	void set_priority(float p_priority);

	float get_gravity() const { return gravity; }
	void set_gravity(float p_gravity);

	bool is_point_gravity() const { return point_gravity; }
	void set_point_gravity(bool p_enabled);

	float get_point_gravity_distance() const { return point_gravity_distance; }
	void set_point_gravity_distance(float p_distance);

	float get_linear_damp() const { return linear_damp; }
	void set_linear_damp(float p_damp);

	float get_angular_damp() const { return angular_damp; }
	void set_angular_damp(float p_damp);

	OverrideMode get_gravity_mode() const { return gravity_mode; }
	void set_gravity_mode(OverrideMode p_mode);

	OverrideMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_linear_damp_mode(OverrideMode p_mode);

	OverrideMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_angular_damp_mode(OverrideMode p_mode);

	Vector3 get_gravity_vector() const { return gravity_vector; }
	void set_gravity_vector(const Vector3 &p_vector);

	float get_wind_pressure() const { return wind_pressure; }
	void set_wind_pressure(float p_wind_pressure) { wind_pressure = p_wind_pressure; }

	float get_wind_attenuation_factor() const { return wind_attenuation_factor; }
	void set_wind_attenuation_factor(float p_wind_attenuation_factor) {
		wind_attenuation_factor = p_wind_attenuation_factor;
	}

	const Vector3 &get_wind_source() const { return wind_source; }
	void set_wind_source(const Vector3 &p_wind_source) { wind_source = p_wind_source; }

	const Vector3 &get_wind_direction() const { return wind_direction; }
	void set_wind_direction(const Vector3 &p_wind_direction) { wind_direction = p_wind_direction; }

	Vector3 compute_gravity(const Vector3 &p_position) const;

	void body_shape_entered(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);
	bool body_shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	void area_shape_entered(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);
	bool area_shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	bool shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	void call_queries();

	template <typename TValue, typename TGetter>
	static bool apply_override(TValue &p_value, PS3DE::AreaSpaceOverrideMode p_mode, TGetter &&p_getter);
};

template <typename TValue, typename TGetter>
inline bool JoltArea3D::apply_override(TValue &p_value, PS3DE::AreaSpaceOverrideMode p_mode, TGetter &&p_getter) {
	switch (p_mode) {
		case PS3DE::AREA_SPACE_OVERRIDE_DISABLED: {
			return false;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_COMBINE: {
			p_value += p_getter();
			return false;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_COMBINE_REPLACE: {
			p_value += p_getter();
			return true;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_REPLACE: {
			p_value = p_getter();
			return true;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_REPLACE_COMBINE: {
			p_value = p_getter();
			return false;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled override mode: '%d'. This should not happen. Please report this.", p_mode));
		}
	}
}

class JoltBody3D final : public JoltObject3D {
public:
	typedef PS3DE::BodyDampMode DampMode;

	struct Contact {
		Vector3 normal;
		Vector3 position;
		Vector3 collider_position;
		Vector3 velocity;
		Vector3 collider_velocity;
		Vector3 impulse;
		RID collider_rid;
		float depth = 0.0f;
		int shape_index = 0;
		int collider_shape_index = 0;
	};

private:
	friend class JoltBodyActivationListener3D;

	SelfList<JoltBody3D> call_queries_element;

	LocalVector<RID> exceptions;
	LocalVector<Contact> contacts;
	LocalVector<JoltArea3D *> areas;
	LocalVector<JoltJoint3D *> joints;

	Transform3D kinematic_transform;

	Vector3 inertia;
	Vector3 center_of_mass_custom;
	Vector3 constant_force;
	Vector3 constant_torque;
	Vector3 linear_surface_velocity;
	Vector3 angular_surface_velocity;
	Vector3 total_gravity;

	PhysicsDirectBodyState3D *direct_state = nullptr;

	PS3DE::BodyMode mode = PS3DE::BODY_MODE_RIGID;

	DampMode linear_damp_mode = PS3DE::BODY_DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = PS3DE::BODY_DAMP_MODE_COMBINE;

	float mass = 1.0f;
	float linear_damp = 0.0f;
	float angular_damp = 0.0f;
	float total_linear_damp = 0.0f;
	float total_angular_damp = 0.0f;
	float collision_priority = 1.0f;

	int contact_count = 0;

	uint32_t locked_axes = 0;

	uint16_t default_area_changed_count = 0;

	bool sleep_allowed = true;
	bool sleep_initially = false;
	bool custom_center_of_mass = false;
	bool custom_integrator = false;
	bool has_point_gravity = false;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual JPH::EMotionType _get_motion_type() const override;

	virtual void _add_to_space() override;

	void _enqueue_call_queries();
	void _dequeue_call_queries();

	void _integrate_forces(float p_step);
	void _move_kinematic(float p_step);

	JPH::EAllowedDOFs _calculate_allowed_dofs() const;

	JPH::MassProperties _calculate_mass_properties(const JPH::Shape &p_shape) const;
	JPH::MassProperties _calculate_mass_properties() const;

	void _on_wake_up();

	void _update_mass_properties();
	bool _needs_update_environmental_properties() const;
	void _update_environmental_properties();
	void _update_kinematic_transform();
	void _update_group_filter();
	void _update_joint_constraints();
	void _update_possible_kinematic_contacts();
	void _update_sleep_allowed();

	void _destroy_joint_constraints();

	void _clear_areas();

	void _mode_changed();
	virtual void _shapes_committed() override;
	virtual void _space_changing() override;
	virtual void _space_changed() override;
	void _areas_changed();
	void _joints_changed();
	void _transform_changed();
	void _motion_changed();
	void _exceptions_changed();
	void _axis_lock_changed();
	void _contact_reporting_changed();
	void _sleep_allowed_changed();

public:
	JoltBody3D();
	virtual ~JoltBody3D() override;

	void set_transform(Transform3D p_transform);

	bool has_custom_integrator() const { return custom_integrator; }
	void set_custom_integrator(bool p_enabled);

	bool is_sleeping() const;
	void set_is_sleeping(bool p_enabled);

	void put_to_sleep() { set_is_sleeping(true); }
	void wake_up() { set_is_sleeping(false); }

	bool is_sleep_allowed() const { return sleep_allowed; }
	bool is_sleep_actually_allowed() const;
	void set_is_sleep_allowed(bool p_enabled);

	Basis get_principal_inertia_axes() const;
	Vector3 get_inverse_inertia() const;
	Basis get_inverse_inertia_tensor() const;

	void set_linear_velocity(const Vector3 &p_velocity);
	void set_angular_velocity(const Vector3 &p_velocity);
	void set_axis_velocity(const Vector3 &p_axis_velocity);

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override;

	virtual bool has_custom_center_of_mass() const override { return custom_center_of_mass; }
	virtual Vector3 get_center_of_mass_custom() const override { return center_of_mass_custom; }
	void set_center_of_mass_custom(const Vector3 &p_center_of_mass);

	int get_max_contacts_reported() const { return contacts.size(); }
	void set_max_contacts_reported(int p_count);

	int get_contact_count() const { return contact_count; }
	const Contact &get_contact(int p_index) { return contacts[p_index]; }

	virtual bool reports_contacts() const override { return !contacts.is_empty(); }
	bool reports_all_kinematic_contacts() const;

	void add_contact(const JoltBody3D *p_collider, float p_depth, int p_shape_index, int p_collider_shape_index, const Vector3 &p_normal, const Vector3 &p_position, const Vector3 &p_collider_position, const Vector3 &p_velocity, const Vector3 &p_collider_velocity, const Vector3 &p_impulse);

	void reset_mass_properties();

	void apply_force(const Vector3 &p_force, const Vector3 &p_position);
	void apply_central_force(const Vector3 &p_force);
	void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position);

	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_torque(const Vector3 &p_torque);
	void apply_torque_impulse(const Vector3 &p_impulse);

	void add_constant_central_force(const Vector3 &p_force);
	void add_constant_force(const Vector3 &p_force, const Vector3 &p_position);
	void add_constant_torque(const Vector3 &p_torque);

	Vector3 get_constant_force() const;
	void set_constant_force(const Vector3 &p_force);

	Vector3 get_constant_torque() const;
	void set_constant_torque(const Vector3 &p_torque);

	Vector3 get_linear_surface_velocity() const { return linear_surface_velocity; }
	Vector3 get_angular_surface_velocity() const { return angular_surface_velocity; }

	void add_collision_exception(const RID &p_excepted_body);
	void remove_collision_exception(const RID &p_excepted_body);
	bool has_collision_exception(const RID &p_excepted_body) const;

	const LocalVector<RID> &get_collision_exceptions() const { return exceptions; }

	void add_area(JoltArea3D *p_area);
	void remove_area(JoltArea3D *p_area);
	void update_area(JoltArea3D *p_area, bool p_priority_changed = false);

	void add_joint(JoltJoint3D *p_joint);
	void remove_joint(JoltJoint3D *p_joint);

	void call_queries();

	virtual void pre_step(float p_step) override;

	PhysicsDirectBodyState3D *get_direct_state();

	PS3DE::BodyMode get_mode() const { return mode; }
	void set_mode(PS3DE::BodyMode p_mode);

	bool is_static() const { return mode == PS3DE::BODY_MODE_STATIC; }
	bool is_kinematic() const { return mode == PS3DE::BODY_MODE_KINEMATIC; }
	bool is_rigid_free() const { return mode == PS3DE::BODY_MODE_RIGID; }
	bool is_rigid_linear() const { return mode == PS3DE::BODY_MODE_RIGID_LINEAR; }
	bool is_rigid() const { return is_rigid_free() || is_rigid_linear(); }

	bool is_ccd_enabled() const;
	void set_ccd_enabled(bool p_enabled);

	float get_mass() const { return mass; }
	void set_mass(float p_mass);

	Vector3 get_inertia() const { return inertia; }
	void set_inertia(const Vector3 &p_inertia);

	float get_bounce() const;
	void set_bounce(float p_bounce);

	float get_friction() const;
	void set_friction(float p_friction);

	float get_gravity_scale() const;
	void set_gravity_scale(float p_scale);

	Vector3 get_total_gravity() const { return total_gravity; }

	float get_linear_damp() const { return linear_damp; }
	void set_linear_damp(float p_damp);

	float get_angular_damp() const { return angular_damp; }
	void set_angular_damp(float p_damp);

	float get_total_linear_damp() const { return total_linear_damp; }
	float get_total_angular_damp() const { return total_angular_damp; }

	float get_collision_priority() const { return collision_priority; }
	void set_collision_priority(float p_priority) { collision_priority = p_priority; }

	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_linear_damp_mode(DampMode p_mode);

	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode);

	bool is_axis_locked(PS3DE::BodyAxis p_axis) const;
	void set_axis_lock(PS3DE::BodyAxis p_axis, bool p_enabled);

	bool are_axes_locked() const { return locked_axes != 0; }

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;
};

class JoltSoftBody3D final : public JoltObject3D {
	RID mesh;

	JPH::SoftBodyCreationSettings *jolt_soft_settings = new JPH::SoftBodyCreationSettings();

	float mass = 1.0f;
	float pressure = 0.0f;
	float linear_damping = 0.01f;
	float stiffness_coefficient = 0.5f;
	float shrinking_factor = 0.0f;

	int simulation_precision = 5;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual void _space_changing() override;
	virtual void _space_changed() override;

	virtual void _add_to_space() override;

	JPH::SoftBodySharedSettings *_create_shared_settings();

	void _apply_environmental_forces(float p_step);

	void _update_mass();
	void _update_pressure();
	void _update_damping();
	void _update_simulation_precision();
	void _update_group_filter();

	void _try_rebuild();

	void _mesh_changed();
	void _simulation_precision_changed();
	void _mass_changed();
	void _pressure_changed();
	void _damping_changed();
	void _pins_changed();
	void _vertices_changed();
	void _exceptions_changed();
	void _motion_changed();
	void _transform_changed();
	void _areas_changed();

public:
	JoltSoftBody3D();
	virtual ~JoltSoftBody3D() override;

	void add_collision_exception(const RID &p_excepted_body);
	void remove_collision_exception(const RID &p_excepted_body);
	bool has_collision_exception(const RID &p_excepted_body) const;

	void add_area(JoltArea3D *p_area);
	void remove_area(JoltArea3D *p_area);

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;

	virtual bool reports_contacts() const override { return false; }

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override;

	virtual void pre_step(float p_step) override;

	void set_mesh(const RID &p_mesh);

	bool is_sleeping() const;
	void set_is_sleeping(bool p_enabled);

	bool is_sleep_allowed() const;
	void set_is_sleep_allowed(bool p_enabled);

	void put_to_sleep() { set_is_sleeping(true); }
	void wake_up() { set_is_sleeping(false); }

	int get_simulation_precision() const { return simulation_precision; }
	void set_simulation_precision(int p_precision);

	float get_mass() const { return mass; }
	void set_mass(float p_mass);

	float get_stiffness_coefficient() const;
	void set_stiffness_coefficient(float p_coefficient);

	float get_shrinking_factor() const;
	void set_shrinking_factor(float p_shrinking_factor);

	float get_pressure() const { return pressure; }
	void set_pressure(float p_pressure);

	float get_linear_damping() const { return linear_damping; }
	void set_linear_damping(float p_damping);

	float get_drag() const;
	void set_drag(float p_drag);

	Transform3D get_transform() const;
	void set_transform(const Transform3D &p_transform);

	AABB get_bounds() const;

	void update_rendering_server(PhysicsServer3DRenderingServerHandler *p_rendering_server_handler);

	Vector3 get_vertex_position(int p_index);
	void set_vertex_position(int p_index, const Vector3 &p_position);

	void pin_vertex(int p_index);
	void unpin_vertex(int p_index);

	void unpin_all_vertices();

	bool is_vertex_pinned(int p_index) const;

	void apply_vertex_impulse(int p_index, const Vector3 &p_impulse);
	void apply_vertex_force(int p_index, const Vector3 &p_force);
	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_central_force(const Vector3 &p_force);
};

// --- End modules/jolt_physics/objects/jolt_objects_3d.h ---

// --- Folded from modules/jolt_physics/spaces/jolt_space_3d.h ---
#include "modules/jolt_physics/jolt_globals.h"

#include "core/math/vector3.h"
#include "core/os/mutex.h"
#include "core/os/spin_lock.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
#include "core/types.h"
#include "servers/physics_3d/physics_server_3d.h"

// includes must stay...
#include <Jolt/Jolt.h>
// ... in this order
#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <Jolt/Core/STLLocalAllocator.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseQuery.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/InternalEdgeRemovingCollector.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/SoftBody/SoftBodyContactListener.h>

#include <atomic>
#include <cstdint>

class JoltArea3D;
class JoltBody3D;
class JoltJoint3D;
class JoltObject3D;
class JoltPhysicsServer3D;
class JoltSoftBody3D;
class JoltSpace3D;
class PhysicsDirectSpaceState3D;

// ============================================================================
// JoltBroadPhaseLayer
// ============================================================================

namespace JoltBroadPhaseLayer {

constexpr JPH::BroadPhaseLayer BODY_STATIC(0);
constexpr JPH::BroadPhaseLayer BODY_STATIC_BIG(1);
constexpr JPH::BroadPhaseLayer BODY_DYNAMIC(2);
constexpr JPH::BroadPhaseLayer AREA_DETECTABLE(3);
constexpr JPH::BroadPhaseLayer AREA_UNDETECTABLE(4);

constexpr uint32_t COUNT = 5;

} // namespace JoltBroadPhaseLayer

// ============================================================================
// Query Collectors
// ============================================================================

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorAll : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity>> HitArray;

private:
	HitArray hits;

public:
	JoltQueryCollectorAll() {
		hits.reserve(TDefaultCapacity);
	}

	bool had_hit() const { return !hits.is_empty(); }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		hits.push_back(p_hit);
	}
};

template <typename TBase>
class JoltQueryCollectorAny : public TBase {
public:
	typedef typename TBase::ResultType Hit;

private:
	Hit hit;
	bool valid = false;

public:
	bool had_hit() const { return valid; }
	const Hit &get_hit() const { return hit; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		valid = false;
	}

	virtual void AddHit(const Hit &p_hit) override {
		hit = p_hit;
		valid = true;

		TBase::ForceEarlyOut();
	}
};

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorAnyMulti : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity>> HitArray;

private:
	HitArray hits;
	int max_hits = 0;

public:
	explicit JoltQueryCollectorAnyMulti(int p_max_hits = TDefaultCapacity) :
			max_hits(p_max_hits) {
		hits.reserve(TDefaultCapacity);
	}

	bool had_hit() const { return hits.size() > 0; }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		if ((int)hits.size() < max_hits) {
			hits.push_back(p_hit);
		}

		if ((int)hits.size() == max_hits) {
			TBase::ForceEarlyOut();
		}
	}
};

template <typename TBase>
class JoltQueryCollectorClosest : public TBase {
public:
	typedef typename TBase::ResultType Hit;

private:
	Hit hit;
	bool valid = false;

public:
	bool had_hit() const { return valid; }
	const Hit &get_hit() const { return hit; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		valid = false;
	}

	virtual void AddHit(const Hit &p_hit) override {
		const float early_out = p_hit.GetEarlyOutFraction();

		if (!valid || early_out < hit.GetEarlyOutFraction()) {
			TBase::UpdateEarlyOutFraction(early_out);

			hit = p_hit;
			valid = true;
		}
	}
};

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorClosestMulti : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity + 1>> HitArray;

private:
	HitArray hits;
	int max_hits = 0;

public:
	explicit JoltQueryCollectorClosestMulti(int p_max_hits = TDefaultCapacity) :
			max_hits(p_max_hits) {
		hits.reserve(TDefaultCapacity + 1);
	}

	bool had_hit() const { return hits.size() > 0; }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		typename HitArray::const_iterator E = hits.cbegin();
		for (; E != hits.cend(); ++E) {
			if (p_hit.GetEarlyOutFraction() < E->GetEarlyOutFraction()) {
				break;
			}
		}

		hits.insert(E, p_hit);

		if ((int)hits.size() > max_hits) {
			hits.resize(max_hits);
		}
	}
};

template <typename TCollector, int TDefaultCapacity>
class JoltQueryCollectorMotion final : public JoltQueryCollectorClosestMulti<TCollector, TDefaultCapacity> {
public:
	typedef JoltQueryCollectorClosestMulti<TCollector, TDefaultCapacity> Base;
	typedef typename Base::Hit Hit;

private:
	Vector3 direction;
	float distance_sq = 0.0f;
	float margin = 0.0f;

public:
	JoltQueryCollectorMotion(const Vector3 &p_motion, float p_margin, int p_max_hits = TDefaultCapacity) :
			Base(p_max_hits),
			direction(p_motion.normalized()),
			distance_sq(p_motion.length_squared()),
			margin(p_margin) {}

	virtual void AddHit(const Hit &p_hit) override {
		const float penetration_depth = p_hit.mPenetrationDepth + margin;
		if (penetration_depth <= 0.0f) {
			return;
		}

		if (distance_sq > 0) {
			const Vector3 normal = to_godot(-p_hit.mPenetrationAxis.Normalized());
			if (direction.dot(normal) >= -CMP_EPSILON) {
				return;
			}
		}

		Base::AddHit(p_hit);
	}
};

// ============================================================================
// Space Subsystems (Allocator, JobSystem, Layers, Listeners, Filters)
// ============================================================================

class JoltTempAllocator final : public JPH::TempAllocator {
	uint64_t capacity = 0;
	uint64_t top = 0;
	uint8_t *base = nullptr;

public:
	explicit JoltTempAllocator();
	virtual ~JoltTempAllocator() override;

	virtual void *Allocate(JPH::uint p_size) override;
	virtual void Free(void *p_ptr, JPH::uint p_size) override;
};

class JoltJobSystem final : public JPH::JobSystemWithBarrier {
	class Job final : public JPH::JobSystem::Job {
		inline static std::atomic<Job *> completed_head = nullptr;

#ifdef DEBUG_ENABLED
		const char *name = nullptr;
#endif

		int64_t task_id = -1;

		std::atomic<Job *> completed_next = nullptr;

		static void _execute(void *p_user_data);

	public:
		Job(const char *p_name, JPH::ColorArg p_color, JPH::JobSystem *p_job_system, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count);
		Job(const Job &p_other) = delete;
		Job(Job &&p_other) = delete;
		~Job();

		static void push_completed(Job *p_job);
		static Job *pop_completed();

		void queue();

		Job &operator=(const Job &p_other) = delete;
		Job &operator=(Job &&p_other) = delete;
	};

#ifdef DEBUG_ENABLED
	inline static HashMap<const void *, uint64_t> timings_by_job;
	inline static SpinLock timings_lock;
#endif

	JPH::FixedSizeFreeList<Job> jobs;

	int thread_count = 0;

	virtual int GetMaxConcurrency() const override;

	virtual JPH::JobHandle CreateJob(const char *p_name, JPH::ColorArg p_color, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count = 0) override;
	virtual void QueueJob(JPH::JobSystem::Job *p_job) override;
	virtual void QueueJobs(JPH::JobSystem::Job **p_jobs, JPH::uint p_job_count) override;
	virtual void FreeJob(JPH::JobSystem::Job *p_job) override;

	void _reclaim_jobs();

public:
	JoltJobSystem();

	void pre_step();
	void post_step();

#ifdef DEBUG_ENABLED
	void flush_timings();
#endif
};

class JoltLayers final
		: public JPH::BroadPhaseLayerInterface,
		  public JPH::ObjectLayerPairFilter,
		  public JPH::ObjectVsBroadPhaseLayerFilter {
	LocalVector<uint64_t> collisions_by_layer;
	HashMap<uint64_t, JPH::ObjectLayer> layers_by_collision;
	JPH::ObjectLayer next_object_layer = 0;

	virtual uint32_t GetNumBroadPhaseLayers() const override;
	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer p_layer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer p_layer) const override;
#endif

	virtual bool ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::ObjectLayer p_encoded_layer2) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::BroadPhaseLayer p_broad_phase_layer2) const override;

	JPH::ObjectLayer _allocate_object_layer(uint64_t p_collision);

public:
	JoltLayers();
	virtual ~JoltLayers();

	JPH::ObjectLayer to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask);
	void from_object_layer(JPH::ObjectLayer p_encoded_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const;
};

class JoltBodyActivationListener3D final : public JPH::BodyActivationListener {
public:
	virtual void OnBodyActivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) override;
	virtual void OnBodyDeactivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) override {}
};

class JoltContactListener3D final : public JPH::ContactListener, public JPH::SoftBodyContactListener {
	struct BodyIDHasher {
		static uint32_t hash(const JPH::BodyID &p_id) {
			return hash_fmix32(p_id.GetIndexAndSequenceNumber());
		}
	};

	struct ShapePairHasher {
		static uint32_t hash(const JPH::SubShapeIDPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.GetBody1ID().GetIndexAndSequenceNumber());
			hash = hash_murmur3_one_32(p_pair.GetSubShapeID1().GetValue(), hash);
			hash = hash_murmur3_one_32(p_pair.GetBody2ID().GetIndexAndSequenceNumber(), hash);
			hash = hash_murmur3_one_32(p_pair.GetSubShapeID2().GetValue(), hash);
			return hash_fmix32(hash);
		}
	};

	struct Contact {
		Vector3 point_self;
		Vector3 point_other;
		Vector3 normal;
		Vector3 velocity_self;
		Vector3 velocity_other;
		Vector3 impulse;
	};

	typedef LocalVector<Contact> Contacts;

	struct Manifold {
		Contacts contacts1;
		Contacts contacts2;
		float depth = 0.0f;
	};

	HashMap<JPH::SubShapeIDPair, Manifold, ShapePairHasher> manifolds_by_shape_pair;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_overlaps;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_enters;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_exits;
	LocalVector<JPH::SubShapeIDPair> area_soft_body_overlaps;
	Mutex write_mutex;
	JoltSpace3D *space = nullptr;

#ifdef DEBUG_ENABLED
	PackedVector3Array debug_contacts;
	std::atomic_int debug_contact_count = 0;
#endif

	virtual void OnContactAdded(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) override;
	virtual void OnContactPersisted(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) override;
	virtual void OnContactRemoved(const JPH::SubShapeIDPair &p_shape_pair) override;

	virtual JPH::SoftBodyValidateResult OnSoftBodyContactValidate(const JPH::Body &p_soft_body, const JPH::Body &p_other_body, JPH::SoftBodyContactSettings &p_settings) override;
	virtual void OnSoftBodyContactAdded(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) override;

	bool _try_override_collision_response(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings);
	bool _try_override_collision_response(const JPH::Body &p_jolt_soft_body, const JPH::Body &p_jolt_other_body, JPH::SoftBodyContactSettings &p_settings);
	bool _try_apply_surface_velocities(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings);
	bool _try_add_contacts(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings);
	bool _try_evaluate_area_overlap(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::SubShapeID &p_shape_id1, const JPH::SubShapeID &p_shape_id2);
	bool _try_remove_contacts(const JPH::SubShapeIDPair &p_shape_pair);
	bool _try_remove_area_overlap(const JPH::SubShapeIDPair &p_shape_pair);

#ifdef DEBUG_ENABLED
	bool _try_add_debug_contacts(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold);
	bool _try_add_debug_contacts(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold);
#endif

	bool _has_shape_shifted(const JoltObject3D &p_object, const JPH::SubShapeID &p_sub_shape_id);

	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltArea3D &p_other_area, const JPH::SubShapeIDPair &p_shape_pair);
	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair);
	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltSoftBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair);

	void _flush_contacts();
	void _flush_area_enters();
	void _flush_area_exits();
	void _clear_area_soft_body_overlaps();

public:
	explicit JoltContactListener3D(JoltSpace3D *p_space) :
			space(p_space) {}

	void pre_step();
	void post_step();

#ifdef DEBUG_ENABLED
	const PackedVector3Array &get_debug_contacts() const { return debug_contacts; }
	int get_debug_contact_count() const { return debug_contact_count.load(std::memory_order_acquire); }
	int get_max_debug_contacts() const { return (int)debug_contacts.size(); }
	void set_max_debug_contacts(int p_count) { debug_contacts.resize(p_count); }
#endif
};

class JoltMotionFilter3D final
		: public JPH::BroadPhaseLayerFilter,
		  public JPH::ObjectLayerFilter,
		  public JPH::BodyFilter,
		  public JPH::ShapeFilter {
	const JoltBody3D &body_self;
	const JoltSpace3D &space;
	const HashSet<RID> &excluded_bodies;
	bool collide_separation_ray = false;

public:
	virtual bool ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_object_layer) const override;
	virtual bool ShouldCollide(const JPH::BodyID &p_jolt_id) const override;
	virtual bool ShouldCollideLocked(const JPH::Body &p_jolt_body) const override;
	virtual bool ShouldCollide(const JPH::Shape *p_jolt_shape, const JPH::SubShapeID &p_jolt_shape_id) const override;
	virtual bool ShouldCollide(const JPH::Shape *p_jolt_shape_self, const JPH::SubShapeID &p_jolt_shape_id_self, const JPH::Shape *p_jolt_shape_other, const JPH::SubShapeID &p_jolt_shape_id_other) const override;
};

class JoltQueryFilter3D final
		: public JPH::BroadPhaseLayerFilter,
		  public JPH::ObjectLayerFilter,
		  public JPH::BodyFilter {
	const JoltSpace3D &space;
	const HashSet<RID> &excluded;
	uint32_t collision_mask = 0;
	bool collide_with_bodies = false;
	bool collide_with_areas = false;
	bool picking = false;

public:
	JoltQueryFilter3D(const PhysicsDirectSpaceState3D &p_space_state, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, const HashSet<RID> &p_excluded, bool p_picking = false);

	virtual bool ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_object_layer) const override;
	virtual bool ShouldCollide(const JPH::BodyID &p_body_id) const override;
	virtual bool ShouldCollideLocked(const JPH::Body &p_body) const override;
};

// ============================================================================
// JoltSpace3D
// ============================================================================

class JoltSpace3D final {
	Mutex pending_objects_mutex;
	Mutex body_call_queries_mutex;

	SelfList<JoltBody3D>::List body_call_queries_list;
	SelfList<JoltArea3D>::List area_call_queries_list;
	SelfList<JoltObject3D>::List shapes_changed_list;
	SelfList<JoltObject3D>::List needs_optimization_list;

	LocalVector<JPH::BodyID> pending_objects_sleeping;
	LocalVector<JPH::BodyID> pending_objects_awake;

	RID rid;

	JPH::JobSystem *job_system = nullptr;
	JPH::TempAllocator *temp_allocator = nullptr;
	JoltLayers *layers = nullptr;
	JoltContactListener3D *contact_listener = nullptr;
	JoltBodyActivationListener3D *body_activation_listener = nullptr;
	JPH::PhysicsSystem *physics_system = nullptr;
	PhysicsDirectSpaceState3D *direct_state = nullptr;
	JoltArea3D *default_area = nullptr;

	float last_step = 0.0f;

	uint16_t default_area_changed_count = 0;

	bool active = false;
	bool stepping = false;

	void _pre_step(float p_step);
	void _post_step(float p_step);

public:
	explicit JoltSpace3D(JPH::JobSystem *p_job_system, JPH::TempAllocator *p_temp_allocator);
	~JoltSpace3D();

	void step(float p_step);

	void call_queries();

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	bool is_active() const { return active; }
	void set_active(bool p_active) { active = p_active; }

	bool is_stepping() const { return stepping; }

	double get_param(PS3DE::SpaceParameter p_param) const;
	void set_param(PS3DE::SpaceParameter p_param, double p_value);

	JPH::PhysicsSystem &get_physics_system() const { return *physics_system; }
	JPH::TempAllocator &get_temp_allocator() const { return *temp_allocator; }

	JPH::BodyInterface &get_body_iface();
	const JPH::BodyInterface &get_body_iface() const;
	const JPH::BodyLockInterface &get_lock_iface() const;

	const JPH::BroadPhaseQuery &get_broad_phase_query() const;
	const JPH::NarrowPhaseQuery &get_narrow_phase_query() const;

	JPH::ObjectLayer map_to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask);
	void map_from_object_layer(JPH::ObjectLayer p_object_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const;

	JPH::Body *try_get_jolt_body(const JPH::BodyID &p_body_id) const;
	JoltObject3D *try_get_object(const JPH::BodyID &p_body_id) const;
	JoltBody3D *try_get_body(const JPH::BodyID &p_body_id) const;
	JoltArea3D *try_get_area(const JPH::BodyID &p_body_id) const;
	JoltSoftBody3D *try_get_soft_body(const JPH::BodyID &p_body_id) const;

	PhysicsDirectSpaceState3D *get_direct_state();

	JoltArea3D *get_default_area() const { return default_area; }
	void set_default_area(JoltArea3D *p_area) { default_area = p_area; }

	void increment_default_area_changed_count() { default_area_changed_count++; }
	uint16_t get_default_area_changed_count() const { return default_area_changed_count; }

	float get_last_step() const { return last_step; }

	JPH::Body *add_object(const JoltObject3D &p_object, const JPH::BodyCreationSettings &p_settings, bool p_sleeping = false);
	JPH::Body *add_object(const JoltObject3D &p_object, const JPH::SoftBodyCreationSettings &p_settings, bool p_sleeping = false);
	void remove_object(const JPH::BodyID &p_jolt_id);
	void flush_pending_objects();

	void set_is_object_sleeping(const JPH::BodyID &p_jolt_id, bool p_enable);

	void enqueue_call_queries(SelfList<JoltBody3D> *p_body);
	void enqueue_call_queries(SelfList<JoltArea3D> *p_area);
	void dequeue_call_queries(SelfList<JoltBody3D> *p_body);
	void dequeue_call_queries(SelfList<JoltArea3D> *p_area);

	void enqueue_shapes_changed(SelfList<JoltObject3D> *p_object);
	void dequeue_shapes_changed(SelfList<JoltObject3D> *p_object);

	void enqueue_needs_optimization(SelfList<JoltObject3D> *p_object);
	void dequeue_needs_optimization(SelfList<JoltObject3D> *p_object);

	void add_joint(JPH::Constraint *p_jolt_ref);
	void add_joint(JoltJoint3D *p_joint);
	void remove_joint(JPH::Constraint *p_jolt_ref);
	void remove_joint(JoltJoint3D *p_joint);

#ifdef DEBUG_ENABLED
	void dump_debug_snapshot(const String &p_dir);
	const PackedVector3Array &get_debug_contacts() const;
	int get_debug_contact_count() const;
	int get_max_debug_contacts() const;
	void set_max_debug_contacts(int p_count);
#endif
};

// --- End modules/jolt_physics/spaces/jolt_space_3d.h ---

// --- Folded from modules/jolt_physics/joints/jolt_joint_3d.h ---
#include "modules/jolt_physics/jolt_physics_server_3d.h"

#include "core/math/quaternion.h"
#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "servers/physics_3d/physics_server_3d.h"

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

class JoltBody3D;
class JoltSpace3D;

class JoltJoint3D final {
	typedef JPH::SixDOFConstraintSettings::EAxis JoltAxis;

	enum {
		AXIS_LINEAR_X = JoltAxis::TranslationX,
		AXIS_LINEAR_Y = JoltAxis::TranslationY,
		AXIS_LINEAR_Z = JoltAxis::TranslationZ,
		AXIS_ANGULAR_X = JoltAxis::RotationX,
		AXIS_ANGULAR_Y = JoltAxis::RotationY,
		AXIS_ANGULAR_Z = JoltAxis::RotationZ,
		AXIS_COUNT = JoltAxis::Num,
		AXES_LINEAR = AXIS_LINEAR_X,
		AXES_ANGULAR = AXIS_ANGULAR_X,
	};

	struct HingeData {
		double limit_lower = 0.0;
		double limit_upper = 0.0;
		double limit_spring_frequency = 0.0;
		double limit_spring_damping = 0.0;
		double motor_target_speed = 0.0;
		double motor_max_torque = FLT_MAX;
		bool limits_enabled = false;
		bool limit_spring_enabled = false;
		bool motor_enabled = false;
	};

	struct SliderData {
		double limit_upper = 0.0;
		double limit_lower = 0.0;
		double limit_spring_frequency = 0.0;
		double limit_spring_damping = 0.0;
		double motor_target_speed = 0.0;
		double motor_max_force = FLT_MAX;
		bool limits_enabled = true;
		bool limit_spring_enabled = false;
		bool motor_enabled = false;
	};

	struct ConeTwistData {
		double swing_limit_span = 0.0;
		double twist_limit_span = 0.0;
		double swing_motor_target_speed_y = 0.0;
		double swing_motor_target_speed_z = 0.0;
		double twist_motor_target_speed = 0.0;
		double swing_motor_max_torque = FLT_MAX;
		double twist_motor_max_torque = FLT_MAX;
		bool swing_limit_enabled = true;
		bool twist_limit_enabled = true;
		bool swing_motor_enabled = false;
		bool twist_motor_enabled = false;
	};

	struct G6DOFData {
		double limit_lower[AXIS_COUNT] = {};
		double limit_upper[AXIS_COUNT] = {};
		double limit_spring_frequency[AXIS_COUNT] = {};
		double limit_spring_damping[AXIS_COUNT] = {};
		double motor_speed[AXIS_COUNT] = {};
		double motor_limit[AXIS_COUNT] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
		double spring_stiffness[AXIS_COUNT] = {};
		double spring_frequency[AXIS_COUNT] = {};
		double spring_damping[AXIS_COUNT] = {};
		double spring_equilibrium[AXIS_COUNT] = {};
		double drive_limit[AXIS_COUNT] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
		bool drive_limit_set[AXIS_COUNT] = {};
		Quaternion angular_target_rotation;
		bool has_angular_target_rotation = false;
		bool limit_enabled[AXIS_COUNT] = {};
		bool limit_spring_enabled[AXIS_COUNT] = {};
		bool motor_enabled[AXIS_COUNT] = {};
		bool spring_enabled[AXIS_COUNT] = {};
		bool spring_use_frequency[AXIS_COUNT] = {};
	};

	PS3DE::JointType joint_type = PS3DE::JOINT_TYPE_MAX;

	bool enabled = true;
	bool collision_disabled = false;

	int velocity_iterations = 0;
	int position_iterations = 0;
	int solver_priority = 1;

	JPH::Ref<JPH::Constraint> jolt_ref;

	JoltBody3D *body_a = nullptr;
	JoltBody3D *body_b = nullptr;

	RID rid;

	Transform3D local_ref_a;
	Transform3D local_ref_b;

	HingeData hinge;
	SliderData slider;
	ConeTwistData cone_twist;
	G6DOFData g6dof;

	void _reconfigure(PS3DE::JointType p_type, JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b);

	void _shift_reference_frames(const Vector3 &p_linear_shift, const Vector3 &p_angular_shift, Transform3D &r_shifted_ref_a, Transform3D &r_shifted_ref_b);

	void _wake_up_bodies();

	void _update_enabled();
	void _update_iterations();
	void _update_priority();
	void _update_joint();

	void _enabled_changed();
	void _iterations_changed();

	String _bodies_to_string() const;

	static JPH::Constraint *_build_fixed(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b);

	// Pin helpers
	static JPH::Constraint *_build_pin(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b);
	void _pin_points_changed();
	void _rebuild_pin();

	// Hinge helpers
	JPH::Constraint *_build_hinge(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const;
	bool _is_hinge_sprung() const { return hinge.limit_spring_enabled && hinge.limit_spring_frequency > 0.0; }
	bool _is_hinge_fixed() const { return hinge.limits_enabled && hinge.limit_lower == hinge.limit_upper && !_is_hinge_sprung(); }
	void _update_hinge_motor_state();
	void _update_hinge_motor_velocity();
	void _update_hinge_motor_limit();
	void _hinge_limits_changed();
	void _hinge_limit_spring_changed();
	void _hinge_motor_state_changed();
	void _hinge_motor_speed_changed();
	void _hinge_motor_limit_changed();
	void _rebuild_hinge();

	// Slider helpers
	JPH::Constraint *_build_slider(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const;
	bool _is_slider_sprung() const { return slider.limit_spring_enabled && slider.limit_spring_frequency > 0.0; }
	bool _is_slider_fixed() const { return slider.limits_enabled && slider.limit_lower == slider.limit_upper && !_is_slider_sprung(); }
	void _update_slider_motor_state();
	void _update_slider_motor_velocity();
	void _update_slider_motor_limit();
	void _slider_limits_changed();
	void _slider_limit_spring_changed();
	void _slider_motor_state_changed();
	void _slider_motor_speed_changed();
	void _slider_motor_limit_changed();
	void _rebuild_slider();

	// Cone twist helpers
	JPH::Constraint *_build_swing_twist(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_swing_limit_span, float p_twist_limit_span) const;
	void _update_swing_motor_state();
	void _update_twist_motor_state();
	void _update_cone_twist_motor_velocity();
	void _update_swing_motor_limit();
	void _update_twist_motor_limit();
	void _cone_twist_limits_changed();
	void _swing_motor_state_changed();
	void _twist_motor_state_changed();
	void _cone_twist_motor_velocity_changed();
	void _swing_motor_limit_changed();
	void _twist_motor_limit_changed();
	void _rebuild_cone_twist();

	// Generic 6DOF helpers
	JPH::Constraint *_build_6dof(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) const;
	void _update_6dof_limit_spring_parameters(int p_axis);
	void _update_6dof_motor_state(int p_axis);
	void _update_6dof_motor_velocity(int p_axis);
	void _update_6dof_motor_limit(int p_axis);
	void _update_6dof_spring_parameters(int p_axis);
	void _update_6dof_spring_equilibrium(int p_axis);
	void _update_6dof_angular_target_rotation();
	void _6dof_limits_changed();
	void _6dof_limit_spring_parameters_changed(int p_axis);
	void _6dof_motor_state_changed(int p_axis);
	void _6dof_motor_speed_changed(int p_axis);
	void _6dof_motor_limit_changed(int p_axis);
	void _6dof_spring_state_changed(int p_axis);
	void _6dof_spring_parameters_changed(int p_axis);
	void _6dof_spring_equilibrium_changed(int p_axis);
	void _6dof_drive_limit_changed(int p_axis);
	void _rebuild_6dof();

public:
	JoltJoint3D() = default;
	explicit JoltJoint3D(PS3DE::JointType p_type) :
			joint_type(p_type) {}
	~JoltJoint3D();

	PS3DE::JointType get_type() const { return joint_type; }

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	JoltSpace3D *get_space() const;

	JPH::Constraint *get_jolt_ref() const { return jolt_ref; }

	bool is_enabled() const { return enabled; }
	void set_enabled(bool p_enabled);

	int get_solver_priority() const { return solver_priority; }
	void set_solver_priority(int p_priority);

	int get_solver_velocity_iterations() const { return velocity_iterations; }
	void set_solver_velocity_iterations(int p_iterations);

	int get_solver_position_iterations() const { return position_iterations; }
	void set_solver_position_iterations(int p_iterations);

	bool is_collision_disabled() const { return collision_disabled; }
	void set_collision_disabled(bool p_disabled);

	void clear();
	void make_pin(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Vector3 &p_local_a, const Vector3 &p_local_b);
	void make_hinge(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b);
	void make_slider(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b);
	void make_cone_twist(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b);
	void make_generic_6dof(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b);

	void destroy();
	void rebuild();

	float get_applied_force() const;
	float get_applied_torque() const;

	// Pin joint API
	Vector3 get_pin_local_a() const { return local_ref_a.origin; }
	void set_pin_local_a(const Vector3 &p_local_a);
	Vector3 get_pin_local_b() const { return local_ref_b.origin; }
	void set_pin_local_b(const Vector3 &p_local_b);
	double get_pin_param(PS3DE::PinJointParam p_param) const;
	void set_pin_param(PS3DE::PinJointParam p_param, double p_value);

	// Hinge joint API
	double get_hinge_param(PS3DE::HingeJointParam p_param) const;
	void set_hinge_param(PS3DE::HingeJointParam p_param, double p_value);
	double get_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param) const;
	void set_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param, double p_value);
	bool get_hinge_flag(PS3DE::HingeJointFlag p_flag) const;
	void set_hinge_flag(PS3DE::HingeJointFlag p_flag, bool p_enabled);
	bool get_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag) const;
	void set_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag, bool p_enabled);

	// Slider joint API
	double get_slider_param(PS3DE::SliderJointParam p_param) const;
	void set_slider_param(PS3DE::SliderJointParam p_param, double p_value);
	double get_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param) const;
	void set_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param, double p_value);
	bool get_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag) const;
	void set_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag, bool p_enabled);

	// Cone twist joint API
	double get_cone_twist_param(PS3DE::ConeTwistJointParam p_param) const;
	void set_cone_twist_param(PS3DE::ConeTwistJointParam p_param, double p_value);
	double get_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param) const;
	void set_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param, double p_value);
	bool get_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag) const;
	void set_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag, bool p_enabled);

	// Generic 6DOF joint API
	double get_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param) const;
	void set_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param, double p_value);
	bool get_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag) const;
	void set_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enabled);
	void set_6dof_angular_target_rotation(const Quaternion &p_target_rotation);
	Quaternion get_6dof_angular_target_rotation() const;
	double get_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param) const;
	void set_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param, double p_value);
	bool get_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag) const;
	void set_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag, bool p_enabled);
};

// --- End modules/jolt_physics/joints/jolt_joint_3d.h ---
