#ifndef PHYSICS_3D_DISABLED
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_JOLT_PHYSICS_ENABLED)
// --- Inlined from modules/jolt_physics/shapes/jolt_shape_3d.h ---
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
#include "modules/jolt_physics/jolt_physics_server_3d.h"

#include "core/math/vector3.h"
#include "core/os/mutex.h"
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

// --- End modules/jolt_physics/shapes/jolt_shape_3d.h ---
#include "jolt_physics_server_3d.h"




JoltPhysicsServer3D::JoltPhysicsServer3D(bool p_on_separate_thread)
	: on_separate_thread(p_on_separate_thread)
{
}

JoltPhysicsServer3D::~JoltPhysicsServer3D() = default;

RID JoltPhysicsServer3D::shape_create(PS3DE::ShapeType p_shape)
{
	switch (p_shape) {
	case PS3DE::SHAPE_WORLD_BOUNDARY:
		return world_boundary_shape_create();
	case PS3DE::SHAPE_SEPARATION_RAY:
		return separation_ray_shape_create();
	case PS3DE::SHAPE_SPHERE:
		return sphere_shape_create();
	case PS3DE::SHAPE_BOX:
		return box_shape_create();
	case PS3DE::SHAPE_CAPSULE:
		return capsule_shape_create();
	case PS3DE::SHAPE_CYLINDER:
		return cylinder_shape_create();
	case PS3DE::SHAPE_CONVEX_POLYGON:
		return convex_polygon_shape_create();
	case PS3DE::SHAPE_CONCAVE_POLYGON:
		return concave_polygon_shape_create();
	case PS3DE::SHAPE_HEIGHTMAP:
		return heightmap_shape_create();
	case PS3DE::SHAPE_CUSTOM:
		return custom_shape_create();
	default:
		return RID();
	}
}

bool JoltPhysicsServer3D::_body_test_motion(RID p_body, PhysicsTestMotionParameters3D* rp_parameters,
	const Ref<PhysicsTestMotionResult3D>& p_result)
{
	PS3DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

RID JoltPhysicsServer3D::world_boundary_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_WORLD_BOUNDARY));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::separation_ray_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_SEPARATION_RAY));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::sphere_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_SPHERE));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::box_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_BOX));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::capsule_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_CAPSULE));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::cylinder_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_CYLINDER));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::convex_polygon_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_CONVEX_POLYGON));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::concave_polygon_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_CONCAVE_POLYGON));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::heightmap_shape_create()
{
	JoltShape3D* shape = memnew(JoltShape3D(PS3DE::SHAPE_HEIGHTMAP));
	RID rid = shape_owner.make_rid(shape);
	shape->set_rid(rid);
	return rid;
}

RID JoltPhysicsServer3D::custom_shape_create()
{
	ERR_FAIL_V_MSG(RID(), "Custom shapes are not supported.");
}

void JoltPhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	shape->set_solver_bias((float)p_bias);
}

PS3DE::ShapeType JoltPhysicsServer3D::shape_get_type(RID p_shape) const
{
	const JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, PS3DE::SHAPE_CUSTOM);

	return shape->get_type();
}

void JoltPhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin)
{
	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	shape->set_margin((float)p_margin);
}

real_t JoltPhysicsServer3D::shape_get_margin(RID p_shape) const
{
	const JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, 0.0);

	return (real_t)shape->get_margin();
}

real_t JoltPhysicsServer3D::shape_get_custom_solver_bias(RID p_shape) const
{
	const JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, 0.0);

	return (real_t)shape->get_solver_bias();
}

RID JoltPhysicsServer3D::space_create()
{
	JoltSpace3D* space = memnew(JoltSpace3D(job_system, temp_allocator));
	RID rid = space_owner.make_rid(space);
	space->set_rid(rid);

	const RID default_area_rid = area_create();
	JoltArea3D* default_area = area_owner.get_or_null(default_area_rid);
	ERR_FAIL_NULL_V(default_area, RID());
	space->set_default_area(default_area);
	default_area->set_space(space);

	return rid;
}

void JoltPhysicsServer3D::space_set_active(RID p_space, bool p_active)
{
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	if (p_active) {
		space->set_active(true);
		active_spaces.insert(space);
	}
	else {
		space->set_active(false);
		active_spaces.erase(space);
	}
}

bool JoltPhysicsServer3D::space_is_active(RID p_space) const
{
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, false);

	return active_spaces.has(space);
}

void JoltPhysicsServer3D::space_set_param(
	RID p_space, PS3DE::SpaceParameter p_param, real_t p_value)
{
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	space->set_param(p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param) const
{
	const JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0.0);

	return (real_t)space->get_param(p_param);
}

PhysicsDirectSpaceState3D* JoltPhysicsServer3D::space_get_direct_state(RID p_space)
{
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, nullptr);
	ERR_FAIL_COND_V_MSG((on_separate_thread && !doing_sync) || space->is_stepping(), nullptr,
		"Space state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return space->get_direct_state();
}

void JoltPhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
#ifdef DEBUG_ENABLED
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	space->set_max_debug_contacts(p_max_contacts);
#endif
}

PackedVector3Array JoltPhysicsServer3D::space_get_contacts(RID p_space) const
{
#ifdef DEBUG_ENABLED
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, PackedVector3Array());

	return space->get_debug_contacts();
#else
	return PackedVector3Array();
#endif
}

int JoltPhysicsServer3D::space_get_contact_count(RID p_space) const
{
#ifdef DEBUG_ENABLED
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0);

	return space->get_debug_contact_count();
#else
	return 0;
#endif
}

RID JoltPhysicsServer3D::area_create()
{
	JoltArea3D* area = memnew(JoltArea3D);
	RID rid = area_owner.make_rid(area);
	area->set_rid(rid);
	return rid;
}

void JoltPhysicsServer3D::area_set_space(RID p_area, RID p_space)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	JoltSpace3D* space = nullptr;

	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	area->set_space(space);
}

void JoltPhysicsServer3D::soft_body_apply_point_impulse(
	RID p_body, int p_point_index, const Vector3& p_impulse)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_vertex_impulse(p_point_index, p_impulse);
}

void JoltPhysicsServer3D::soft_body_apply_point_force(
	RID p_body, int p_point_index, const Vector3& p_force)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_vertex_force(p_point_index, p_force);
}

void JoltPhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_impulse(p_impulse);
}

void JoltPhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_force(p_force);
}

RID JoltPhysicsServer3D::area_get_space(RID p_area) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	const JoltSpace3D* space = area->get_space();

	if (space == nullptr) {
		return RID();
	}

	return space->get_rid();
}

void JoltPhysicsServer3D::area_add_shape(
	RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	area->add_shape(shape, p_transform, p_disabled);
}

void JoltPhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	area->set_shape(p_shape_idx, shape);
}

RID JoltPhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	const JoltShape3D* shape = area->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_rid();
}

void JoltPhysicsServer3D::area_set_shape_transform(
	RID p_area, int p_shape_idx, const Transform3D& p_transform)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_shape_transform(p_shape_idx, p_transform);
}

Transform3D JoltPhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform3D());

	return area->get_shape_transform_scaled(p_shape_idx);
}

void JoltPhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->remove_shape(p_shape_idx);
}

void JoltPhysicsServer3D::area_clear_shapes(RID p_area)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->clear_shapes();
}

void JoltPhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_shape_disabled(p_shape_idx, p_disabled);
}

void JoltPhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	return area->set_transform(p_transform);
}

Transform3D JoltPhysicsServer3D::area_get_transform(RID p_area) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform3D());

	return area->get_transform_scaled();
}

void JoltPhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_mask(p_mask);
}

uint32_t JoltPhysicsServer3D::area_get_collision_mask(RID p_area) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_mask();
}

void JoltPhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_layer(p_layer);
}

uint32_t JoltPhysicsServer3D::area_get_collision_layer(RID p_area) const
{
	const JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_layer();
}

void JoltPhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_monitorable(p_monitorable);
}

void JoltPhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable)
{
	JoltArea3D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_pickable(p_enable);
}

RID JoltPhysicsServer3D::body_create()
{
	JoltBody3D* body = memnew(JoltBody3D);
	RID rid = body_owner.make_rid(body);
	body->set_rid(rid);
	return rid;
}

void JoltPhysicsServer3D::body_set_space(RID p_body, RID p_space)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	JoltSpace3D* space = nullptr;

	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	body->set_space(space);
}

RID JoltPhysicsServer3D::body_get_space(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	const JoltSpace3D* space = body->get_space();

	if (space == nullptr) {
		return RID();
	}

	return space->get_rid();
}

void JoltPhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_mode(p_mode);
}

PS3DE::BodyMode JoltPhysicsServer3D::body_get_mode(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, PS3DE::BODY_MODE_STATIC);

	return body->get_mode();
}

void JoltPhysicsServer3D::body_add_shape(
	RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	body->add_shape(shape, p_transform, p_disabled);
}

void JoltPhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	JoltShape3D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	body->set_shape(p_shape_idx, shape);
}

RID JoltPhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	const JoltShape3D* shape = body->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_rid();
}

void JoltPhysicsServer3D::body_set_shape_transform(
	RID p_body, int p_shape_idx, const Transform3D& p_transform)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_shape_transform(p_shape_idx, p_transform);
}

Transform3D JoltPhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Transform3D());

	return body->get_shape_transform_scaled(p_shape_idx);
}

void JoltPhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_shape(p_shape_idx);
}

void JoltPhysicsServer3D::body_clear_shapes(RID p_body)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->clear_shapes();
}

void JoltPhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_shape_disabled(p_shape_idx, p_disabled);
}

void JoltPhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_ccd_enabled(p_enable);
}

bool JoltPhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);

	return body->is_ccd_enabled();
}

void JoltPhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_layer(p_layer);
}

uint32_t JoltPhysicsServer3D::body_get_collision_layer(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_layer();
}

void JoltPhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_mask(p_mask);
}

uint32_t JoltPhysicsServer3D::body_get_collision_mask(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_mask();
}

void JoltPhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_priority((float)p_priority);
}

real_t JoltPhysicsServer3D::body_get_collision_priority(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_collision_priority();
}

void JoltPhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags)
{
	WARN_PRINT("Body user flags are not supported. Any such value will be ignored.");
}

uint32_t JoltPhysicsServer3D::body_get_user_flags(RID p_body) const { return 0; }

void JoltPhysicsServer3D::body_reset_mass_properties(RID p_body)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->reset_mass_properties();
}

void JoltPhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_central_impulse(p_impulse);
}

void JoltPhysicsServer3D::body_apply_impulse(
	RID p_body, const Vector3& p_impulse, const Vector3& p_position)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_impulse(p_impulse, p_position);
}

void JoltPhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_torque_impulse(p_impulse);
}

void JoltPhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_central_force(p_force);
}

void JoltPhysicsServer3D::body_apply_force(
	RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_force(p_force, p_position);
}

void JoltPhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->apply_torque(p_torque);
}

void JoltPhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_central_force(p_force);
}

void JoltPhysicsServer3D::body_add_constant_force(
	RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_force(p_force, p_position);
}

void JoltPhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_torque(p_torque);
}

void JoltPhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_force(p_force);
}

Vector3 JoltPhysicsServer3D::body_get_constant_force(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Vector3());

	return body->get_constant_force();
}

void JoltPhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_torque(p_torque);
}

Vector3 JoltPhysicsServer3D::body_get_constant_torque(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Vector3());

	return body->get_constant_torque();
}

void JoltPhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_axis_velocity(p_axis_velocity);
}

void JoltPhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_axis_lock(p_axis, p_lock);
}

bool JoltPhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);

	return body->is_axis_locked(p_axis);
}

void JoltPhysicsServer3D::body_add_collision_exception(RID p_body, RID p_excepted_body)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_collision_exception(p_excepted_body);
}

void JoltPhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_excepted_body)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_collision_exception(p_excepted_body);
}

void JoltPhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	for (const RID& exception : body->get_collision_exceptions()) {
		p_exceptions->push_back(exception);
	}
}

void JoltPhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_amount)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_max_contacts_reported(p_amount);
}

int JoltPhysicsServer3D::body_get_max_contacts_reported(RID p_body) const
{
	const JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_max_contacts_reported();
}

void JoltPhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	WARN_PRINT(
		"Per-body contact depth threshold is not supported. Any such value will be ignored.");
}

real_t JoltPhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body) const
{
	return 0.0;
}

void JoltPhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_enable)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_custom_integrator(p_enable);
}

bool JoltPhysicsServer3D::body_is_omitting_force_integration(RID p_body) const
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);

	return body->has_custom_integrator();
}

void JoltPhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_pickable(p_enable);
}

bool JoltPhysicsServer3D::body_test_motion(
	RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result)
{
	JoltBody3D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);

	JoltSpace3D* space = body->get_space();
	ERR_FAIL_NULL_V(space, false);

	return space->get_direct_state()->body_test_motion(*body, p_parameters, r_result);
}

PhysicsDirectBodyState3D* JoltPhysicsServer3D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V_MSG((on_separate_thread && !doing_sync), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	JoltBody3D* body = body_owner.get_or_null(p_body);
	if (unlikely(body == nullptr || body->get_space() == nullptr)) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(body->get_space()->is_stepping(), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return body->get_direct_state();
}

RID JoltPhysicsServer3D::soft_body_create()
{
	JoltSoftBody3D* body = memnew(JoltSoftBody3D);
	RID rid = soft_body_owner.make_rid(body);
	body->set_rid(rid);
	return rid;
}

void JoltPhysicsServer3D::soft_body_update_rendering_server(
	RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	return body->update_rendering_server(rp_rendering_server_handler);
}

void JoltPhysicsServer3D::soft_body_set_space(RID p_body, RID p_space)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	JoltSpace3D* space = nullptr;

	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	body->set_space(space);
}

RID JoltPhysicsServer3D::soft_body_get_space(RID p_body) const
{
	const JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	const JoltSpace3D* space = body->get_space();

	if (space == nullptr) {
		return RID();
	}

	return space->get_rid();
}

void JoltPhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_mesh(p_mesh);
}

AABB JoltPhysicsServer3D::soft_body_get_bounds(RID p_body) const
{
	const JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, AABB());

	return body->get_bounds();
}

void JoltPhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_layer(p_layer);
}

uint32_t JoltPhysicsServer3D::soft_body_get_collision_layer(RID p_body) const
{
	const JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_layer();
}

void JoltPhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_mask(p_mask);
}

uint32_t JoltPhysicsServer3D::soft_body_get_collision_mask(RID p_body) const
{
	const JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_mask();
}

void JoltPhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_excepted_body)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_collision_exception(p_excepted_body);
}

void JoltPhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_excepted_body)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_collision_exception(p_excepted_body);
}

void JoltPhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_transform(p_transform);
}

void JoltPhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_pickable(p_enable);
}

void JoltPhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_precision)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_simulation_precision(p_precision);
}

int JoltPhysicsServer3D::soft_body_get_simulation_precision(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_simulation_precision();
}

void JoltPhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_mass((float)p_total_mass);
}

real_t JoltPhysicsServer3D::soft_body_get_total_mass(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_mass();
}

void JoltPhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_coefficient)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_stiffness_coefficient((float)p_coefficient);
}

real_t JoltPhysicsServer3D::soft_body_get_linear_stiffness(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_stiffness_coefficient();
}

void JoltPhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_shrinking_factor((float)p_shrinking_factor);
}

real_t JoltPhysicsServer3D::soft_body_get_shrinking_factor(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_shrinking_factor();
}

void JoltPhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_coefficient)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_pressure((float)p_coefficient);
}

real_t JoltPhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_pressure();
}

void JoltPhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_coefficient)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_linear_damping((float)p_coefficient);
}

real_t JoltPhysicsServer3D::soft_body_get_damping_coefficient(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_linear_damping();
}

void JoltPhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_coefficient)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->set_drag((float)p_coefficient);
}

real_t JoltPhysicsServer3D::soft_body_get_drag_coefficient(RID p_body) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0.0);

	return (real_t)body->get_drag();
}

void JoltPhysicsServer3D::soft_body_move_point(
	RID p_body, int p_point_index, const Vector3& p_global_position)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_vertex_position(p_point_index, p_global_position);
}

Vector3 JoltPhysicsServer3D::soft_body_get_point_global_position(
	RID p_body, int p_point_index) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Vector3());

	return body->get_vertex_position(p_point_index);
}

void JoltPhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->unpin_all_vertices();
}

void JoltPhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin)
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	if (p_pin) {
		body->pin_vertex(p_point_index);
	}
	else {
		body->unpin_vertex(p_point_index);
	}
}

bool JoltPhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index) const
{
	JoltSoftBody3D* body = soft_body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);

	return body->is_vertex_pinned(p_point_index);
}

RID JoltPhysicsServer3D::joint_create()
{
	JoltJoint3D* joint = memnew(JoltJoint3D);
	RID rid = joint_owner.make_rid(joint);
	joint->set_rid(rid);
	return rid;
}

void JoltPhysicsServer3D::joint_clear(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	if (joint->get_type() != PS3DE::JOINT_TYPE_MAX) {
		joint->clear();
	}
}

void JoltPhysicsServer3D::joint_make_pin(
	RID p_joint, RID p_body_a, const Vector3& p_local_a, RID p_body_b, const Vector3& p_local_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	JoltBody3D* body_a = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(body_a);

	JoltBody3D* body_b = body_owner.get_or_null(p_body_b);
	ERR_FAIL_COND(body_a == body_b);

	joint->make_pin(body_a, body_b, p_local_a, p_local_b);
}

void JoltPhysicsServer3D::pin_joint_set_param(
	RID p_joint, PS3DE::PinJointParam p_param, real_t p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_PIN);

	joint->set_pin_param(p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_PIN, 0.0);

	return (real_t)joint->get_pin_param(p_param);
}

void JoltPhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_local_a)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_PIN);

	joint->set_pin_local_a(p_local_a);
}

Vector3 JoltPhysicsServer3D::pin_joint_get_local_a(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, Vector3());

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_PIN, Vector3());

	return joint->get_pin_local_a();
}

void JoltPhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_local_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_PIN);

	joint->set_pin_local_b(p_local_b);
}

Vector3 JoltPhysicsServer3D::pin_joint_get_local_b(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, Vector3());

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_PIN, Vector3());

	return joint->get_pin_local_b();
}

void JoltPhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_a, const Transform3D& p_hinge_a,
	RID p_body_b, const Transform3D& p_hinge_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	JoltBody3D* body_a = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(body_a);

	JoltBody3D* body_b = body_owner.get_or_null(p_body_b);
	ERR_FAIL_COND(body_a == body_b);

	joint->make_hinge(body_a, body_b, p_hinge_a, p_hinge_b);
}

void JoltPhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_a,
	const Vector3& p_pivot_a, const Vector3& p_axis_a, RID p_body_b, const Vector3& p_pivot_b,
	const Vector3& p_axis_b)
{
	ERR_FAIL_MSG("Simple hinge joints are not supported when using Jolt Physics.");
}

void JoltPhysicsServer3D::hinge_joint_set_param(
	RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_HINGE);

	return joint->set_hinge_param(p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, 0.0);

	return (real_t)joint->get_hinge_param(p_param);
}

void JoltPhysicsServer3D::hinge_joint_set_flag(
	RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_HINGE);

	return joint->set_hinge_flag(p_flag, p_enabled);
}

bool JoltPhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, false);

	return joint->get_hinge_flag(p_flag);
}

void JoltPhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_a,
	const Transform3D& p_local_ref_a, RID p_body_b, const Transform3D& p_local_ref_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	JoltBody3D* body_a = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(body_a);

	JoltBody3D* body_b = body_owner.get_or_null(p_body_b);
	ERR_FAIL_COND(body_a == body_b);

	joint->make_slider(body_a, body_b, p_local_ref_a, p_local_ref_b);
}

void JoltPhysicsServer3D::slider_joint_set_param(
	RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER);

	return joint->set_slider_param(p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::slider_joint_get_param(
	RID p_joint, PS3DE::SliderJointParam p_param) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER, 0.0);

	return (real_t)joint->get_slider_param(p_param);
}

void JoltPhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_a,
	const Transform3D& p_local_ref_a, RID p_body_b, const Transform3D& p_local_ref_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	JoltBody3D* body_a = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(body_a);

	JoltBody3D* body_b = body_owner.get_or_null(p_body_b);
	ERR_FAIL_COND(body_a == body_b);

	joint->make_cone_twist(body_a, body_b, p_local_ref_a, p_local_ref_b);
}

void JoltPhysicsServer3D::cone_twist_joint_set_param(
	RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST);

	return joint->set_cone_twist_param(p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::cone_twist_joint_get_param(
	RID p_joint, PS3DE::ConeTwistJointParam p_param) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST, 0.0);

	return (real_t)joint->get_cone_twist_param(p_param);
}

void JoltPhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_a,
	const Transform3D& p_local_ref_a, RID p_body_b, const Transform3D& p_local_ref_b)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	JoltBody3D* body_a = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(body_a);

	JoltBody3D* body_b = body_owner.get_or_null(p_body_b);
	ERR_FAIL_COND(body_a == body_b);

	joint->make_generic_6dof(body_a, body_b, p_local_ref_a, p_local_ref_b);
}

void JoltPhysicsServer3D::generic_6dof_joint_set_param(
	RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param, real_t p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_6DOF);

	return joint->set_6dof_param(p_axis, p_param, (double)p_value);
}

real_t JoltPhysicsServer3D::generic_6dof_joint_get_param(
	RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, 0.0);

	return (real_t)joint->get_6dof_param(p_axis, p_param);
}

void JoltPhysicsServer3D::generic_6dof_joint_set_flag(
	RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_6DOF);

	return joint->set_6dof_flag(p_axis, p_flag, p_enable);
}

bool JoltPhysicsServer3D::generic_6dof_joint_get_flag(
	RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, false);

	return joint->get_6dof_flag(p_axis, p_flag);
}

void JoltPhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(
	RID p_joint, const Quaternion& p_target_rotation)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_6DOF);

	joint->set_6dof_angular_target_rotation(p_target_rotation);
}

Quaternion JoltPhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, Quaternion());

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, Quaternion());

	return joint->get_6dof_angular_target_rotation();
}

PS3DE::JointType JoltPhysicsServer3D::joint_get_type(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, PS3DE::JOINT_TYPE_PIN);

	return joint->get_type();
}

void JoltPhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	joint->set_solver_priority(p_priority);
}

int JoltPhysicsServer3D::joint_get_solver_priority(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);

	return joint->get_solver_priority();
}

void JoltPhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	joint->set_collision_disabled(p_disable);
}

bool JoltPhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	return joint->is_collision_disabled();
}

void JoltPhysicsServer3D::free_rid(RID p_rid)
{
	if (JoltShape3D* shape = shape_owner.get_or_null(p_rid)) {
		free_shape(shape);
	}
	else if (JoltBody3D* body = body_owner.get_or_null(p_rid)) {
		free_body(body);
	}
	else if (JoltJoint3D* joint = joint_owner.get_or_null(p_rid)) {
		free_joint(joint);
	}
	else if (JoltArea3D* area = area_owner.get_or_null(p_rid)) {
		free_area(area);
	}
	else if (JoltSoftBody3D* soft_body = soft_body_owner.get_or_null(p_rid)) {
		free_soft_body(soft_body);
	}
	else if (JoltSpace3D* space = space_owner.get_or_null(p_rid)) {
		free_space(space);
	}
	else {
		ERR_FAIL_MSG("Failed to free RID: The specified RID has no owner.");
	}
}

void JoltPhysicsServer3D::set_active(bool p_active) { active = p_active; }

void JoltPhysicsServer3D::init()
{
	job_system = new JoltJobSystem();
	temp_allocator = new JoltTempAllocator();
}

void JoltPhysicsServer3D::finish()
{
	if (temp_allocator != nullptr) {
		delete temp_allocator;
		temp_allocator = nullptr;
	}

	if (job_system != nullptr) {
		delete job_system;
		job_system = nullptr;
	}
}

void JoltPhysicsServer3D::step(real_t p_step)
{
	if (!active) {
		return;
	}

	for (JoltSpace3D* active_space : active_spaces) {
		job_system->pre_step();

		active_space->step((float)p_step);

		job_system->post_step();
	}
}

void JoltPhysicsServer3D::sync() { doing_sync = true; }

void JoltPhysicsServer3D::end_sync() { doing_sync = false; }

void JoltPhysicsServer3D::flush_queries()
{
	if (!active) {
		return;
	}

	flushing_queries = true;

	for (JoltSpace3D* space : active_spaces) {
		space->call_queries();
	}

	flushing_queries = false;

#ifdef DEBUG_ENABLED
	job_system->flush_timings();
#endif
}

bool JoltPhysicsServer3D::is_flushing_queries() const { return flushing_queries; }

int JoltPhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_process_info) { return 0; }

void JoltPhysicsServer3D::free_space(JoltSpace3D* p_space)
{
	ERR_FAIL_NULL(p_space);

	free_area(p_space->get_default_area());
	space_set_active(p_space->get_rid(), false);
	space_owner.free(p_space->get_rid());
	memdelete(p_space);
}

void JoltPhysicsServer3D::free_area(JoltArea3D* p_area)
{
	ERR_FAIL_NULL(p_area);

	p_area->set_space(nullptr);
	area_owner.free(p_area->get_rid());
	memdelete(p_area);
}

void JoltPhysicsServer3D::free_body(JoltBody3D* p_body)
{
	ERR_FAIL_NULL(p_body);

	p_body->set_space(nullptr);
	body_owner.free(p_body->get_rid());
	memdelete(p_body);
}

void JoltPhysicsServer3D::free_soft_body(JoltSoftBody3D* p_body)
{
	ERR_FAIL_NULL(p_body);

	p_body->set_space(nullptr);
	soft_body_owner.free(p_body->get_rid());
	memdelete(p_body);
}

void JoltPhysicsServer3D::free_shape(JoltShape3D* p_shape)
{
	ERR_FAIL_NULL(p_shape);

	p_shape->remove_self();
	shape_owner.free(p_shape->get_rid());
	memdelete(p_shape);
}

void JoltPhysicsServer3D::free_joint(JoltJoint3D* p_joint)
{
	ERR_FAIL_NULL(p_joint);

	joint_owner.free(p_joint->get_rid());
	memdelete(p_joint);
}

#ifdef DEBUG_ENABLED

void JoltPhysicsServer3D::dump_debug_snapshots(const String& p_dir)
{
	for (JoltSpace3D* space : active_spaces) {
		space->dump_debug_snapshot(p_dir);
	}
}

void JoltPhysicsServer3D::space_dump_debug_snapshot(RID p_space, const String& p_dir)
{
	JoltSpace3D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	space->dump_debug_snapshot(p_dir);
}

#endif

bool JoltPhysicsServer3D::joint_get_enabled(RID p_joint) const
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	return joint->is_enabled();
}

void JoltPhysicsServer3D::joint_set_enabled(RID p_joint, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	joint->set_enabled(p_enabled);
}

int JoltPhysicsServer3D::joint_get_solver_velocity_iterations(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);

	return joint->get_solver_velocity_iterations();
}

void JoltPhysicsServer3D::joint_set_solver_velocity_iterations(RID p_joint, int p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	return joint->set_solver_velocity_iterations(p_value);
}

int JoltPhysicsServer3D::joint_get_solver_position_iterations(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);

	return joint->get_solver_position_iterations();
}

void JoltPhysicsServer3D::joint_set_solver_position_iterations(RID p_joint, int p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	return joint->set_solver_position_iterations(p_value);
}

float JoltPhysicsServer3D::pin_joint_get_applied_force(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_PIN, 0.0f);

	return joint->get_applied_force();
}

double JoltPhysicsServer3D::hinge_joint_get_jolt_param(
	RID p_joint, HingeJointParamJolt p_param) const
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, 0.0);

	return joint->get_hinge_jolt_param(p_param);
}

void JoltPhysicsServer3D::hinge_joint_set_jolt_param(
	RID p_joint, HingeJointParamJolt p_param, double p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_HINGE);

	return joint->set_hinge_jolt_param(p_param, p_value);
}

bool JoltPhysicsServer3D::hinge_joint_get_jolt_flag(RID p_joint, HingeJointFlagJolt p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, false);

	return joint->get_hinge_jolt_flag(p_flag);
}

void JoltPhysicsServer3D::hinge_joint_set_jolt_flag(
	RID p_joint, HingeJointFlagJolt p_flag, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_HINGE);

	return joint->set_hinge_jolt_flag(p_flag, p_enabled);
}

float JoltPhysicsServer3D::hinge_joint_get_applied_force(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, 0.0f);

	return joint->get_applied_force();
}

float JoltPhysicsServer3D::hinge_joint_get_applied_torque(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_HINGE, 0.0f);

	return joint->get_applied_torque();
}

double JoltPhysicsServer3D::slider_joint_get_jolt_param(
	RID p_joint, SliderJointParamJolt p_param) const
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER, 0.0);

	return joint->get_slider_jolt_param(p_param);
}

void JoltPhysicsServer3D::slider_joint_set_jolt_param(
	RID p_joint, SliderJointParamJolt p_param, double p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER);

	return joint->set_slider_jolt_param(p_param, p_value);
}

bool JoltPhysicsServer3D::slider_joint_get_jolt_flag(RID p_joint, SliderJointFlagJolt p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER, false);

	return joint->get_slider_jolt_flag(p_flag);
}

void JoltPhysicsServer3D::slider_joint_set_jolt_flag(
	RID p_joint, SliderJointFlagJolt p_flag, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER);

	return joint->set_slider_jolt_flag(p_flag, p_enabled);
}

float JoltPhysicsServer3D::slider_joint_get_applied_force(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER, 0.0f);

	return joint->get_applied_force();
}

float JoltPhysicsServer3D::slider_joint_get_applied_torque(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_SLIDER, 0.0f);

	return joint->get_applied_torque();
}

double JoltPhysicsServer3D::cone_twist_joint_get_jolt_param(
	RID p_joint, ConeTwistJointParamJolt p_param) const
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST, 0.0);

	return joint->get_cone_twist_jolt_param(p_param);
}

void JoltPhysicsServer3D::cone_twist_joint_set_jolt_param(
	RID p_joint, ConeTwistJointParamJolt p_param, double p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST);

	return joint->set_cone_twist_jolt_param(p_param, p_value);
}

bool JoltPhysicsServer3D::cone_twist_joint_get_jolt_flag(
	RID p_joint, ConeTwistJointFlagJolt p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST, false);

	return joint->get_cone_twist_jolt_flag(p_flag);
}

void JoltPhysicsServer3D::cone_twist_joint_set_jolt_flag(
	RID p_joint, ConeTwistJointFlagJolt p_flag, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST);

	return joint->set_cone_twist_jolt_flag(p_flag, p_enabled);
}

float JoltPhysicsServer3D::cone_twist_joint_get_applied_force(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST, 0.0f);

	return joint->get_applied_force();
}

float JoltPhysicsServer3D::cone_twist_joint_get_applied_torque(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_CONE_TWIST, 0.0f);

	return joint->get_applied_torque();
}

double JoltPhysicsServer3D::generic_6dof_joint_get_jolt_param(
	RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisParamJolt p_param) const
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, 0.0);

	return joint->get_6dof_jolt_param(p_axis, p_param);
}

void JoltPhysicsServer3D::generic_6dof_joint_set_jolt_param(
	RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisParamJolt p_param, double p_value)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_6DOF);

	return joint->set_6dof_jolt_param(p_axis, p_param, p_value);
}

bool JoltPhysicsServer3D::generic_6dof_joint_get_jolt_flag(
	RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisFlagJolt p_flag) const
{
	const JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, false);

	return joint->get_6dof_jolt_flag(p_axis, p_flag);
}

void JoltPhysicsServer3D::generic_6dof_joint_set_jolt_flag(
	RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisFlagJolt p_flag, bool p_enabled)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	ERR_FAIL_COND(joint->get_type() != PS3DE::JOINT_TYPE_6DOF);

	return joint->set_6dof_jolt_flag(p_axis, p_flag, p_enabled);
}

float JoltPhysicsServer3D::generic_6dof_joint_get_applied_force(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, 0.0f);

	return joint->get_applied_force();
}

float JoltPhysicsServer3D::generic_6dof_joint_get_applied_torque(RID p_joint)
{
	JoltJoint3D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0.0f);

	ERR_FAIL_COND_V(joint->get_type() != PS3DE::JOINT_TYPE_6DOF, 0.0f);

	return joint->get_applied_torque();
}

int JoltPhysicsServer3D::area_get_shape_count(RID p_area) const { return 0; }

int JoltPhysicsServer3D::body_get_shape_count(RID p_body) const { return 0; }

void JoltPhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}



// ============================================================================
// Merged from modules/jolt_physics/register_types.cpp
// ============================================================================

#include "modules/register_module_types.h"

#include "modules/jolt_physics/jolt_physics_server_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

void uninitialize_jolt_physics_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	if (PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::finalize();
	}
	jolt_deinitialize();
}

void initialize_jolt_physics_module(ModuleInitializationLevel p_level) {}



// ============================================================================
// Merged from modules/jolt_physics/jolt_globals.cpp
// ============================================================================


#include "modules/jolt_physics/jolt_physics_server_3d.h"


#include "core/os/memory.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>

void JoltMath::decompose(Basis &p_basis, Vector3 &r_scale) {
	Vector3 x = p_basis.get_column(Vector3::AXIS_X);
	Vector3 y = p_basis.get_column(Vector3::AXIS_Y);
	Vector3 z = p_basis.get_column(Vector3::AXIS_Z);

	const real_t x_dot_x = x.dot(x);
	y -= x * (y.dot(x) / x_dot_x);
	z -= x * (z.dot(x) / x_dot_x);
	const real_t y_dot_y = y.dot(y);
	z -= y * (z.dot(y) / y_dot_y);
	const real_t z_dot_z = z.dot(z);

	const real_t det = x.cross(y).dot(z);

	r_scale = SIGN(det) * Vector3(Math::sqrt(x_dot_x), Math::sqrt(y_dot_y), Math::sqrt(z_dot_z));

	p_basis.set_column(Vector3::AXIS_X, x / r_scale.x);
	p_basis.set_column(Vector3::AXIS_Y, y / r_scale.y);
	p_basis.set_column(Vector3::AXIS_Z, z / r_scale.z);
}

void *jolt_alloc(size_t p_size) {
	return Memory::alloc_static(p_size);
}

void *jolt_realloc(void *p_mem, size_t p_old_size, size_t p_new_size) {
	return Memory::realloc_static(p_mem, p_new_size);
}

void jolt_free(void *p_mem) {
	if (unlikely(p_mem == nullptr)) {
		return;
	}
	Memory::free_static(p_mem);
}

void *jolt_aligned_alloc(size_t p_size, size_t p_alignment) {
	return Memory::alloc_aligned_static(p_size, p_alignment);
}

void jolt_aligned_free(void *p_mem) {
	if (unlikely(p_mem == nullptr)) {
		return;
	}
	Memory::free_aligned_static(p_mem);
}

#ifdef JPH_ENABLE_ASSERTS

void jolt_trace(const char *p_format, ...) {
	va_list args;
	va_start(args, p_format);
	char buffer[1024] = { '\0' };
	vsnprintf(buffer, sizeof(buffer), p_format, args);
	va_end(args);
	print_verbose(buffer);
}

bool jolt_assert(const char *p_expr, const char *p_msg, const char *p_file, uint32_t p_line) {
	ERR_PRINT(vformat("Jolt Physics assertion '%s' failed with message '%s' at '%s:%d'", p_expr, p_msg != nullptr ? p_msg : "", p_file, p_line));
	return false;
}

#endif

static bool jolt_initialized = false;

bool jolt_is_initialized() {
	return jolt_initialized;
}

void jolt_initialize() {
	if (jolt_initialized) {
		return;
	}

	JPH::Allocate = &jolt_alloc;
	JPH::Reallocate = &jolt_realloc;
	JPH::Free = &jolt_free;
	JPH::AlignedAllocate = &jolt_aligned_alloc;
	JPH::AlignedFree = &jolt_aligned_free;

#ifdef JPH_ENABLE_ASSERTS
	JPH::Trace = &jolt_trace;
	JPH::AssertFailed = &jolt_assert;
#endif

	JPH::Factory::sInstance = new JPH::Factory();

	JPH::RegisterTypes();

	JoltCustomRayShape::register_type();
	JoltCustomUserDataShape::register_type();
	JoltCustomDoubleSidedShape::register_type();

	JoltGroupFilter::instance = new JoltGroupFilter();
	JoltGroupFilter::instance->SetEmbedded();

	jolt_initialized = true;
}

void jolt_deinitialize() {
	if (!jolt_initialized) {
		return;
	}

	if (JoltGroupFilter::instance != nullptr) {
		delete JoltGroupFilter::instance;
		JoltGroupFilter::instance = nullptr;
	}

	JPH::UnregisterTypes();

	if (JPH::Factory::sInstance != nullptr) {
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	jolt_initialized = false;
}

// ============================================================================
// Merged from modules/jolt_physics/objects/jolt_objects_3d.cpp
// ============================================================================


#include "modules/jolt_physics/jolt_physics_server_3d.h"


#include "core/config/engine.h"
#include "servers/physics_3d/physics_server_3d.h"
#include "servers/rendering/renderer.h"

// must stay last
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>

// ============================================================================
// JoltGroupFilter
// ============================================================================

bool JoltGroupFilter::CanCollide(const JPH::CollisionGroup &p_group1, const JPH::CollisionGroup &p_group2) const {
	const JoltObject3D *object1 = decode_object(p_group1.GetGroupID(), p_group1.GetSubGroupID());
	const JoltObject3D *object2 = decode_object(p_group2.GetGroupID(), p_group2.GetSubGroupID());
	return object1->can_interact_with(*object2);
}

void JoltGroupFilter::encode_object(const JoltObject3D *p_object, JPH::CollisionGroup::GroupID &r_group_id, JPH::CollisionGroup::SubGroupID &r_sub_group_id) {
	const uint64_t address = reinterpret_cast<uint64_t>(p_object);
	r_group_id = JPH::CollisionGroup::GroupID(address >> 32U);
	r_sub_group_id = JPH::CollisionGroup::SubGroupID(address & 0xFFFFFFFFULL);
}

const JoltObject3D *JoltGroupFilter::decode_object(JPH::CollisionGroup::GroupID p_group_id, JPH::CollisionGroup::SubGroupID p_sub_group_id) {
	const uint64_t upper_bits = (uint64_t)p_group_id << 32U;
	const uint64_t lower_bits = (uint64_t)p_sub_group_id;
	const uint64_t address = uint64_t(upper_bits | lower_bits);
	return reinterpret_cast<const JoltObject3D *>(address);
}

static_assert(sizeof(JoltObject3D *) <= 8, "Pointer size greater than expected.");
static_assert(sizeof(JPH::CollisionGroup::GroupID) == 4, "Size of Jolt's collision group ID has changed.");
static_assert(sizeof(JPH::CollisionGroup::SubGroupID) == 4, "Size of Jolt's collision sub-group ID has changed.");

// ============================================================================
// JoltObject3D
// ============================================================================

void JoltObject3D::_remove_from_space() {
	if (!in_space()) {
		return;
	}

	space->remove_object(jolt_body->GetID());
	jolt_body = nullptr;
}

void JoltObject3D::_reset_space() {
	ERR_FAIL_NULL(space);

	_space_changing();
	_remove_from_space();
	_add_to_space();
	_space_changed();
}

void JoltObject3D::_update_object_layer() {
	if (!in_space()) {
		return;
	}

	space->get_body_iface().SetObjectLayer(jolt_body->GetID(), _get_object_layer());
}

void JoltObject3D::_collision_layer_changed() {
	_update_object_layer();
}

void JoltObject3D::_collision_mask_changed() {
	_update_object_layer();
}

bool JoltObject3D::_is_big() const {
	return get_aabb().get_longest_axis_size() >= 1000.0f;
}

void JoltObject3D::_enqueue_shapes_changed() {
	if (space != nullptr) {
		space->enqueue_shapes_changed(&shapes_changed_element);
	}
}

void JoltObject3D::_dequeue_shapes_changed() {
	if (space != nullptr) {
		space->dequeue_shapes_changed(&shapes_changed_element);
	}
}

void JoltObject3D::_enqueue_needs_optimization() {
	if (space != nullptr) {
		space->enqueue_needs_optimization(&needs_optimization_element);
	}
}

void JoltObject3D::_dequeue_needs_optimization() {
	if (space != nullptr) {
		space->dequeue_needs_optimization(&needs_optimization_element);
	}
}

void JoltObject3D::_shapes_changed() {
	commit_shapes(false);
}

void JoltObject3D::_shapes_committed() {
	_update_object_layer();
}

void JoltObject3D::_space_changing() {
	if (is_shaped()) {
		_dequeue_shapes_changed();
		_dequeue_needs_optimization();

		previous_jolt_shape = nullptr;

		if (in_space()) {
			jolt_settings = new JPH::BodyCreationSettings(jolt_body->GetBodyCreationSettings());
		}
	}
}

JoltObject3D::JoltObject3D(ObjectType p_object_type) :
		object_type(p_object_type),
		shapes_changed_element(this),
		needs_optimization_element(this) {
	if (is_shaped()) {
		jolt_settings = new JPH::BodyCreationSettings();
		jolt_settings->mAllowSleeping = true;
		jolt_settings->mFriction = 1.0f;
		jolt_settings->mRestitution = 0.0f;
		jolt_settings->mLinearDamping = 0.0f;
		jolt_settings->mAngularDamping = 0.0f;
	}
}

JoltObject3D::~JoltObject3D() {
	if (jolt_settings != nullptr) {
		delete jolt_settings;
		jolt_settings = nullptr;
	}
}

void JoltObject3D::set_space(JoltSpace3D *p_space) {
	if (space == p_space) {
		return;
	}

	_space_changing();

	if (space != nullptr) {
		_remove_from_space();
	}

	space = p_space;

	if (space != nullptr) {
		_add_to_space();
	}

	_space_changed();
}

void JoltObject3D::set_collision_layer(uint32_t p_layer) {
	if (p_layer == collision_layer) {
		return;
	}

	collision_layer = p_layer;

	_collision_layer_changed();
}

void JoltObject3D::set_collision_mask(uint32_t p_mask) {
	if (p_mask == collision_mask) {
		return;
	}

	collision_mask = p_mask;

	_collision_mask_changed();
}

bool JoltObject3D::can_collide_with(const JoltObject3D &p_other) const {
	return (collision_mask & p_other.get_collision_layer()) != 0;
}

String JoltObject3D::to_string() const {
	return String();
}

bool JoltObject3D::can_interact_with(const JoltObject3D &p_other) const {
	if (const JoltBody3D *other_body = p_other.as_body()) {
		return can_interact_with(*other_body);
	} else if (const JoltArea3D *other_area = p_other.as_area()) {
		return can_interact_with(*other_area);
	} else if (const JoltSoftBody3D *other_soft_body = p_other.as_soft_body()) {
		return can_interact_with(*other_soft_body);
	} else {
		ERR_FAIL_V_MSG(false, vformat("Unhandled object type: '%d'. This should not happen. Please report this.", p_other.get_type()));
	}
}

Transform3D JoltObject3D::get_transform_unscaled() const {
	if (!in_space()) {
		return Transform3D(to_godot(jolt_settings->mRotation), to_godot(jolt_settings->mPosition));
	} else {
		return Transform3D(to_godot(jolt_body->GetRotation()), to_godot(jolt_body->GetPosition()));
	}
}

Transform3D JoltObject3D::get_transform_scaled() const {
	return get_transform_unscaled().scaled_local(scale);
}

Basis JoltObject3D::get_basis() const {
	if (!in_space()) {
		return to_godot(jolt_settings->mRotation);
	} else {
		return to_godot(jolt_body->GetRotation());
	}
}

Vector3 JoltObject3D::get_position() const {
	if (!in_space()) {
		return to_godot(jolt_settings->mPosition);
	} else {
		return to_godot(jolt_body->GetPosition());
	}
}

Vector3 JoltObject3D::get_center_of_mass() const {
	ERR_FAIL_COND_V_MSG(!in_space(), Vector3(),
			vformat("Failed to retrieve center-of-mass of '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, try adding the "
					"node to a scene tree first.",
					to_string()));
	return to_godot(jolt_body->GetCenterOfMassPosition());
}

Vector3 JoltObject3D::get_center_of_mass_relative() const {
	return get_center_of_mass() - get_position();
}

Vector3 JoltObject3D::get_center_of_mass_local() const {
	ERR_FAIL_NULL_V_MSG(space, Vector3(),
			vformat("Failed to retrieve local center-of-mass of '%s'. Doing so without a physics space "
					"is not supported when using Jolt Physics. If this relates to a node, try adding "
					"the node to a scene tree first.",
					to_string()));

	return get_transform_scaled().xform_inv(get_center_of_mass());
}

Vector3 JoltObject3D::get_linear_velocity() const {
	if (!in_space()) {
		return to_godot(jolt_settings->mLinearVelocity);
	} else {
		return to_godot(jolt_body->GetLinearVelocity());
	}
}

Vector3 JoltObject3D::get_angular_velocity() const {
	if (!in_space()) {
		return to_godot(jolt_settings->mAngularVelocity);
	} else {
		return to_godot(jolt_body->GetAngularVelocity());
	}
}

JPH::ShapeRefC JoltObject3D::build_shapes(bool p_optimize_compound) {
	JPH::ShapeRefC new_shape = _try_build_shape(p_optimize_compound);

	if (new_shape == nullptr) {
		if (has_custom_center_of_mass()) {
			new_shape = new JPH::EmptyShape(to_jolt(get_center_of_mass_custom()));
		} else {
			new_shape = new JPH::EmptyShape();
		}
	}

	return new_shape;
}

void JoltObject3D::commit_shapes(bool p_optimize_compound) {
	if (!in_space()) {
		_shapes_committed();
		return;
	}

	JPH::ShapeRefC new_shape = build_shapes(p_optimize_compound);
	if (new_shape == jolt_shape) {
		return;
	}

	if (previous_jolt_shape == nullptr) {
		previous_jolt_shape = jolt_shape;
	}

	jolt_shape = new_shape;

	space->get_body_iface().SetShape(jolt_body->GetID(), jolt_shape, false, JPH::EActivation::DontActivate);

	_enqueue_shapes_changed();

	if (!p_optimize_compound && jolt_shape->GetType() == JPH::EShapeType::Compound) {
		_enqueue_needs_optimization();
	} else {
		_dequeue_needs_optimization();
	}

	_shapes_committed();
}

void JoltObject3D::clear_previous_shape() {
	previous_jolt_shape = nullptr;
}

int JoltObject3D::find_shape_index(const JPH::SubShapeID &p_sub_shape_id) const {
	ERR_FAIL_NULL_V(jolt_shape, -1);
	return find_shape_index((uint32_t)jolt_shape->GetSubShapeUserData(p_sub_shape_id));
}

void JoltObject3D::set_shape_transform(int, Transform3D) {}
void JoltObject3D::set_shape(int, JoltShape3D *) {}
void JoltObject3D::set_shape_disabled(int, bool) {}
void JoltObject3D::remove_shape(int) {}
Transform3D JoltObject3D::get_shape_transform_scaled(int) const { return Transform3D(); }
JoltShape3D *JoltObject3D::get_shape(int) const { return nullptr; }
void JoltObject3D::clear_shapes() {}
void JoltObject3D::add_shape(JoltShape3D *, Transform3D, bool) {}
JPH::ShapeRefC JoltObject3D::_try_build_shape(bool) { return JPH::ShapeRefC(); }
void JoltObject3D::remove_shape(const JoltShape3D *) {}
AABB JoltObject3D::get_aabb() const { return AABB(); }
int JoltObject3D::find_shape_index(unsigned int) const { return -1; }

// ============================================================================
// JoltArea3D
// ============================================================================

JPH::BroadPhaseLayer JoltArea3D::_get_broad_phase_layer() const {
	return monitorable ? JoltBroadPhaseLayer::AREA_DETECTABLE : JoltBroadPhaseLayer::AREA_UNDETECTABLE;
}

JPH::ObjectLayer JoltArea3D::_get_object_layer() const {
	ERR_FAIL_NULL_V(space, 0);

	if (jolt_shape == nullptr || jolt_shape->GetType() == JPH::EShapeType::Empty) {
		return space->map_to_object_layer(_get_broad_phase_layer(), 0, 0);
	}

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

void JoltArea3D::_enqueue_call_queries() {
	if (space != nullptr) {
		space->enqueue_call_queries(&call_queries_element);
	}
}

void JoltArea3D::_dequeue_call_queries() {
	if (space != nullptr) {
		space->dequeue_call_queries(&call_queries_element);
	}
}

void JoltArea3D::_notify_body_entered(const JPH::BodyID &p_body_id) {
	if (JoltBody3D *body = space->try_get_body(p_body_id)) {
		body->add_area(this);
	} else if (JoltSoftBody3D *soft_body = space->try_get_soft_body(p_body_id)) {
		soft_body->add_area(this);
	}
}

void JoltArea3D::_notify_body_exited(const JPH::BodyID &p_body_id) {
	if (JoltBody3D *body = space->try_get_body(p_body_id)) {
		body->remove_area(this);
	} else if (JoltSoftBody3D *soft_body = space->try_get_soft_body(p_body_id)) {
		soft_body->remove_area(this);
	}
}

void JoltArea3D::_update_group_filter() {
	if (!in_space()) {
		return;
	}

	jolt_body->GetCollisionGroup().SetGroupFilter(JoltGroupFilter::instance);
}

void JoltArea3D::_space_changing() {
	JoltObject3D::_space_changing();

	_remove_all_overlaps();
	_dequeue_call_queries();
}

void JoltArea3D::_space_changed() {
	JoltObject3D::_space_changed();

	_update_group_filter();
}

void JoltArea3D::_events_changed() { _enqueue_call_queries(); }
void JoltArea3D::_body_monitoring_changed() { _update_sleeping(); }
void JoltArea3D::_area_monitoring_changed() { _update_sleeping(); }
void JoltArea3D::_monitorable_changed() { _update_object_layer(); }

JoltArea3D::JoltArea3D() :
		JoltObject3D(OBJECT_TYPE_AREA),
		call_queries_element(this) {}

void JoltArea3D::set_transform(Transform3D p_transform) {
	JOLT_ENSURE_SCALE_NOT_ZERO(p_transform, vformat("An invalid transform was passed to area '%s'.", to_string()));

	Vector3 new_scale;
	JoltMath::decompose(p_transform, new_scale);

	if (!scale.is_equal_approx(new_scale)) {
		scale = new_scale;
		_shapes_changed();
	}

	if (!in_space()) {
		jolt_settings->mPosition = to_jolt_r(p_transform.origin);
		jolt_settings->mRotation = to_jolt(p_transform.basis);
	} else {
		space->get_body_iface().SetPositionAndRotation(jolt_body->GetID(), to_jolt_r(p_transform.origin), to_jolt(p_transform.basis), JPH::EActivation::DontActivate);
	}
}

void JoltArea3D::set_monitorable(bool p_monitorable) {
	if (p_monitorable == monitorable) {
		return;
	}

	monitorable = p_monitorable;

	_monitorable_changed();
}

bool JoltArea3D::can_interact_with(const JoltBody3D &p_other) const { return can_monitor(p_other); }
bool JoltArea3D::can_interact_with(const JoltSoftBody3D &p_other) const { return can_monitor(p_other); }
bool JoltArea3D::can_interact_with(const JoltArea3D &p_other) const { return can_monitor(p_other) || p_other.can_monitor(*this); }

void JoltArea3D::set_priority(float p_priority) {
	if (p_priority == priority) {
		return;
	}
	priority = p_priority;
	_notify_bodies_updated(true);
}

void JoltArea3D::set_gravity(float p_gravity) {
	if (p_gravity == gravity) {
		return;
	}
	gravity = p_gravity;
	_notify_bodies_updated();
}

void JoltArea3D::set_point_gravity(bool p_enabled) {
	if (p_enabled == point_gravity) {
		return;
	}
	point_gravity = p_enabled;
	_notify_bodies_updated();
}

void JoltArea3D::set_point_gravity_distance(float p_distance) {
	if (p_distance == point_gravity_distance) {
		return;
	}
	point_gravity_distance = p_distance;
	_notify_bodies_updated();
}

void JoltArea3D::set_linear_damp(float p_damp) {
	if (p_damp == linear_damp) {
		return;
	}
	linear_damp = p_damp;
	_notify_bodies_updated();
}

void JoltArea3D::set_angular_damp(float p_damp) {
	if (p_damp == angular_damp) {
		return;
	}
	angular_damp = p_damp;
	_notify_bodies_updated();
}

void JoltArea3D::set_gravity_mode(OverrideMode p_mode) {
	if (p_mode == gravity_mode) {
		return;
	}
	gravity_mode = p_mode;
	_notify_bodies_updated();
}

void JoltArea3D::set_linear_damp_mode(OverrideMode p_mode) {
	if (p_mode == linear_damp_mode) {
		return;
	}
	linear_damp_mode = p_mode;
	_notify_bodies_updated();
}

void JoltArea3D::set_angular_damp_mode(OverrideMode p_mode) {
	if (p_mode == angular_damp_mode) {
		return;
	}
	angular_damp_mode = p_mode;
	_notify_bodies_updated();
}

void JoltArea3D::set_gravity_vector(const Vector3 &p_vector) {
	if (p_vector == gravity_vector) {
		return;
	}
	gravity_vector = p_vector;
	_notify_bodies_updated();
}

Vector3 JoltArea3D::compute_gravity(const Vector3 &p_position) const {
	if (!point_gravity) {
		return gravity_vector * gravity;
	}

	const Vector3 point = get_transform_scaled().xform(gravity_vector);
	const Vector3 to_point = point - p_position;
	const real_t to_point_dist_sq = MAX(to_point.length_squared(), (real_t)CMP_EPSILON);
	const Vector3 to_point_dir = to_point / Math::sqrt(to_point_dist_sq);

	if (point_gravity_distance == 0.0f) {
		return to_point_dir * gravity;
	}

	const float gravity_dist_sq = point_gravity_distance * point_gravity_distance;

	return to_point_dir * (gravity * gravity_dist_sq / to_point_dist_sq);
}

bool JoltArea3D::shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id) {
	return body_shape_exited(p_body_id, p_other_shape_id, p_self_shape_id) || area_shape_exited(p_body_id, p_other_shape_id, p_self_shape_id);
}

void JoltArea3D::_notify_bodies_updated(bool p_priority_changed) {}
bool JoltArea3D::can_monitor(const JoltArea3D &) const { return true; }
bool JoltArea3D::body_shape_exited(const JPH::BodyID &, const JPH::SubShapeID &, const JPH::SubShapeID &) { return true; }
void JoltArea3D::_update_sleeping() {}
bool JoltArea3D::can_monitor(const JoltSoftBody3D &) const { return true; }
bool JoltArea3D::can_monitor(const JoltBody3D &) const { return true; }
void JoltArea3D::body_shape_entered(const JPH::BodyID &, const JPH::SubShapeID &, const JPH::SubShapeID &) {}
bool JoltArea3D::area_shape_exited(const JPH::BodyID &, const JPH::SubShapeID &, const JPH::SubShapeID &) { return true; }
void JoltArea3D::_remove_all_overlaps() {}
void JoltArea3D::call_queries() {}
void JoltArea3D::area_shape_entered(const JPH::BodyID &, const JPH::SubShapeID &, const JPH::SubShapeID &) {}
void JoltArea3D::_add_to_space() {}

// ============================================================================
// JoltBody3D
// ============================================================================

JPH::BroadPhaseLayer JoltBody3D::_get_broad_phase_layer() const {
	switch (mode) {
		case PS3DE::BODY_MODE_STATIC: {
			return _is_big() ? JoltBroadPhaseLayer::BODY_STATIC_BIG : JoltBroadPhaseLayer::BODY_STATIC;
		}
		case PS3DE::BODY_MODE_KINEMATIC:
		case PS3DE::BODY_MODE_RIGID:
		case PS3DE::BODY_MODE_RIGID_LINEAR: {
			return JoltBroadPhaseLayer::BODY_DYNAMIC;
		}
		default: {
			ERR_FAIL_V_MSG(JoltBroadPhaseLayer::BODY_STATIC, vformat("Unhandled body mode: '%d'. This should not happen. Please report this.", mode));
		}
	}
}

JPH::ObjectLayer JoltBody3D::_get_object_layer() const {
	ERR_FAIL_NULL_V(space, 0);

	if (jolt_shape == nullptr || jolt_shape->GetType() == JPH::EShapeType::Empty) {
		return space->map_to_object_layer(_get_broad_phase_layer(), 0, 0);
	}

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

JPH::EMotionType JoltBody3D::_get_motion_type() const {
	switch (mode) {
		case PS3DE::BODY_MODE_STATIC: {
			return JPH::EMotionType::Static;
		}
		case PS3DE::BODY_MODE_KINEMATIC: {
			return JPH::EMotionType::Kinematic;
		}
		case PS3DE::BODY_MODE_RIGID:
		case PS3DE::BODY_MODE_RIGID_LINEAR: {
			return JPH::EMotionType::Dynamic;
		}
		default: {
			ERR_FAIL_V_MSG(JPH::EMotionType::Static, vformat("Unhandled body mode: '%d'. This should not happen. Please report this.", mode));
		}
	}
}

void JoltBody3D::_add_to_space() {
	jolt_shape = build_shapes(true);

	JPH::CollisionGroup::GroupID group_id = 0;
	JPH::CollisionGroup::SubGroupID sub_group_id = 0;
	JoltGroupFilter::encode_object(this, group_id, sub_group_id);

	jolt_settings->mUserData = reinterpret_cast<JPH::uint64>(this);
	jolt_settings->mObjectLayer = _get_object_layer();
	jolt_settings->mCollisionGroup = JPH::CollisionGroup(nullptr, group_id, sub_group_id);
	jolt_settings->mMotionType = _get_motion_type();
	jolt_settings->mAllowedDOFs = _calculate_allowed_dofs();
	jolt_settings->mAllowDynamicOrKinematic = true;
	jolt_settings->mCollideKinematicVsNonDynamic = reports_all_kinematic_contacts();
	jolt_settings->mUseManifoldReduction = !reports_contacts();
	jolt_settings->mAllowSleeping = is_sleep_actually_allowed();
	jolt_settings->mLinearDamping = 0.0f;
	jolt_settings->mAngularDamping = 0.0f;
	jolt_settings->mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
	jolt_settings->mMassPropertiesOverride = _calculate_mass_properties();

	jolt_settings->SetShape(jolt_shape);

	JPH::Body *new_jolt_body = space->add_object(*this, *jolt_settings, sleep_initially);
	if (new_jolt_body == nullptr) {
		return;
	}

	jolt_body = new_jolt_body;

	delete jolt_settings;
	jolt_settings = nullptr;
}

void JoltBody3D::_enqueue_call_queries() {
	if (space != nullptr) {
		space->enqueue_call_queries(&call_queries_element);
	}
}

void JoltBody3D::_dequeue_call_queries() {
	if (space != nullptr) {
		space->dequeue_call_queries(&call_queries_element);
	}
}

void JoltBody3D::_integrate_forces(float p_step) {
	if (custom_integrator) {
		return;
	}

	JPH::MotionProperties &motion_properties = *jolt_body->GetMotionPropertiesUnchecked();

	JPH::Vec3 linear_velocity = motion_properties.GetLinearVelocity();
	linear_velocity *= MAX(1.0f - total_linear_damp * p_step, 0.0f);
	motion_properties.SetLinearVelocity(linear_velocity);

	JPH::Vec3 angular_velocity = motion_properties.GetAngularVelocity();
	angular_velocity *= MAX(1.0f - total_angular_damp * p_step, 0.0f);
	motion_properties.SetAngularVelocity(angular_velocity);

	jolt_body->AddForce(to_jolt(total_gravity / motion_properties.GetInverseMass() + constant_force));
	jolt_body->AddTorque(to_jolt(constant_torque));
}

void JoltBody3D::_move_kinematic(float p_step) {
	jolt_body->SetLinearVelocity(JPH::Vec3::sZero());
	jolt_body->SetAngularVelocity(JPH::Vec3::sZero());

	const JPH::RVec3 current_position = jolt_body->GetPosition();
	const JPH::Quat current_rotation = jolt_body->GetRotation();

	const JPH::RVec3 new_position = to_jolt_r(kinematic_transform.origin);
	const JPH::Quat new_rotation = to_jolt(kinematic_transform.basis);

	if (new_position == current_position && new_rotation == current_rotation) {
		return;
	}

	jolt_body->MoveKinematic(new_position, new_rotation, p_step);
}

JPH::EAllowedDOFs JoltBody3D::_calculate_allowed_dofs() const {
	if (is_static()) {
		return JPH::EAllowedDOFs::All;
	}

	JPH::EAllowedDOFs allowed_dofs = JPH::EAllowedDOFs::All;

	if (is_axis_locked(PS3DE::BODY_AXIS_LINEAR_X)) {
		allowed_dofs &= ~JPH::EAllowedDOFs::TranslationX;
	}

	if (is_axis_locked(PS3DE::BODY_AXIS_LINEAR_Y)) {
		allowed_dofs &= ~JPH::EAllowedDOFs::TranslationY;
	}

	if (is_axis_locked(PS3DE::BODY_AXIS_LINEAR_Z)) {
		allowed_dofs &= ~JPH::EAllowedDOFs::TranslationZ;
	}

	if (is_axis_locked(PS3DE::BODY_AXIS_ANGULAR_X) || is_rigid_linear()) {
		allowed_dofs &= ~JPH::EAllowedDOFs::RotationX;
	}

	if (is_axis_locked(PS3DE::BODY_AXIS_ANGULAR_Y) || is_rigid_linear()) {
		allowed_dofs &= ~JPH::EAllowedDOFs::RotationY;
	}

	if (is_axis_locked(PS3DE::BODY_AXIS_ANGULAR_Z) || is_rigid_linear()) {
		allowed_dofs &= ~JPH::EAllowedDOFs::RotationZ;
	}

	ERR_FAIL_COND_V_MSG(allowed_dofs == JPH::EAllowedDOFs::None, JPH::EAllowedDOFs::All,
			vformat("Invalid axis locks for '%s'. Locking all axes is not supported when using Jolt "
					"Physics. All axes will be unlocked. Considering freezing the body instead.",
					to_string()));

	return allowed_dofs;
}

JPH::MassProperties JoltBody3D::_calculate_mass_properties(const JPH::Shape &p_shape) const {
	const bool calculate_mass = mass <= 0;
	const bool calculate_inertia = inertia.x <= 0 || inertia.y <= 0 || inertia.z <= 0;

	JPH::MassProperties mass_properties = p_shape.GetMassProperties();

	if (calculate_mass && calculate_inertia) {
		// Use the mass properties calculated by the shape
	} else if (calculate_inertia) {
		mass_properties.ScaleToMass(mass);
	} else {
		mass_properties.mMass = mass;
	}

	if (inertia.x > 0) {
		mass_properties.mInertia(0, 0) = (float)inertia.x;
		mass_properties.mInertia(0, 1) = 0;
		mass_properties.mInertia(0, 2) = 0;
		mass_properties.mInertia(1, 0) = 0;
		mass_properties.mInertia(2, 0) = 0;
	}

	if (inertia.y > 0) {
		mass_properties.mInertia(1, 1) = (float)inertia.y;
		mass_properties.mInertia(1, 0) = 0;
		mass_properties.mInertia(1, 2) = 0;
		mass_properties.mInertia(0, 1) = 0;
		mass_properties.mInertia(2, 1) = 0;
	}

	if (inertia.z > 0) {
		mass_properties.mInertia(2, 2) = (float)inertia.z;
		mass_properties.mInertia(2, 0) = 0;
		mass_properties.mInertia(2, 1) = 0;
		mass_properties.mInertia(0, 2) = 0;
		mass_properties.mInertia(1, 2) = 0;
	}

	mass_properties.mInertia(3, 3) = 1.0f;

	return mass_properties;
}

JPH::MassProperties JoltBody3D::_calculate_mass_properties() const {
	return _calculate_mass_properties(*jolt_shape);
}

void JoltBody3D::_update_mass_properties() {
	if (in_space()) {
		jolt_body->GetMotionPropertiesUnchecked()->SetMassProperties(_calculate_allowed_dofs(), _calculate_mass_properties());
	}
}

bool JoltBody3D::_needs_update_environmental_properties() const {
	if (unlikely(!in_space())) {
		return false;
	}

	return has_point_gravity || space->get_default_area_changed_count() != default_area_changed_count;
}

void JoltBody3D::_update_environmental_properties() {
	if (unlikely(!in_space())) {
		return;
	}

	total_gravity = Vector3();
	total_linear_damp = 0.0f;
	total_angular_damp = 0.0f;

	const Vector3 position = to_godot(jolt_body->GetPosition());

	bool gravity_done = false;
	bool linear_damp_done = linear_damp_mode == PS3DE::BODY_DAMP_MODE_REPLACE;
	bool angular_damp_done = angular_damp_mode == PS3DE::BODY_DAMP_MODE_REPLACE;

	for (const JoltArea3D *area : areas) {
		if (!gravity_done) {
			gravity_done = JoltArea3D::apply_override(total_gravity, area->get_gravity_mode(), [&]() { return area->compute_gravity(position); });
		}

		if (!linear_damp_done) {
			linear_damp_done = JoltArea3D::apply_override(total_linear_damp, area->get_linear_damp_mode(), [&]() { return area->get_linear_damp(); });
		}

		if (!angular_damp_done) {
			angular_damp_done = JoltArea3D::apply_override(total_angular_damp, area->get_angular_damp_mode(), [&]() { return area->get_angular_damp(); });
		}

		if (gravity_done && linear_damp_done && angular_damp_done) {
			break;
		}
	}

	if (!gravity_done) {
		total_gravity += space->get_default_area()->compute_gravity(position);
	}

	if (!linear_damp_done) {
		total_linear_damp += space->get_default_area()->get_linear_damp();
	}

	if (!angular_damp_done) {
		total_angular_damp += space->get_default_area()->get_angular_damp();
	}

	total_gravity *= jolt_body->GetMotionPropertiesUnchecked()->GetGravityFactor();

	switch (linear_damp_mode) {
		case PS3DE::BODY_DAMP_MODE_COMBINE: {
			total_linear_damp += linear_damp;
		} break;
		case PS3DE::BODY_DAMP_MODE_REPLACE: {
			total_linear_damp = linear_damp;
		} break;
	}

	switch (angular_damp_mode) {
		case PS3DE::BODY_DAMP_MODE_COMBINE: {
			total_angular_damp += angular_damp;
		} break;
		case PS3DE::BODY_DAMP_MODE_REPLACE: {
			total_angular_damp = angular_damp;
		} break;
	}

	default_area_changed_count = space->get_default_area_changed_count();
}

void JoltBody3D::_update_kinematic_transform() {
	if (is_kinematic()) {
		kinematic_transform = get_transform_unscaled();
	}
}

void JoltBody3D::_update_joint_constraints() {
	for (JoltJoint3D *joint : joints) {
		joint->rebuild();
	}
}

void JoltBody3D::_update_possible_kinematic_contacts() {
	const bool value = reports_all_kinematic_contacts();

	if (!in_space()) {
		jolt_settings->mCollideKinematicVsNonDynamic = value;
	} else {
		jolt_body->SetCollideKinematicVsNonDynamic(value);
	}
}

void JoltBody3D::_update_sleep_allowed() {
	const bool value = is_sleep_actually_allowed();

	if (!in_space()) {
		jolt_settings->mAllowSleeping = value;
	} else {
		jolt_body->SetAllowSleeping(value);
	}
}

void JoltBody3D::_destroy_joint_constraints() {
	for (JoltJoint3D *joint : joints) {
		joint->destroy();
	}
}

void JoltBody3D::_clear_areas() {
	if (!in_space()) {
		return;
	}

	areas.clear();
	_areas_changed();
}

void JoltBody3D::_update_group_filter() {
	JPH::GroupFilter *group_filter = !exceptions.is_empty() ? JoltGroupFilter::instance : nullptr;

	if (!in_space()) {
		jolt_settings->mCollisionGroup.SetGroupFilter(group_filter);
	} else {
		jolt_body->GetCollisionGroup().SetGroupFilter(group_filter);
	}
}

void JoltBody3D::_mode_changed() {
	_update_object_layer();
	_update_kinematic_transform();
	_update_mass_properties();
	_update_sleep_allowed();
	wake_up();
}

void JoltBody3D::_shapes_committed() {
	JoltObject3D::_shapes_committed();

	_update_mass_properties();
	_update_joint_constraints();
	wake_up();
}

void JoltBody3D::_space_changing() {
	JoltObject3D::_space_changing();

	sleep_initially = is_sleeping();

	_destroy_joint_constraints();
	_clear_areas();
	_dequeue_call_queries();
}

void JoltBody3D::_space_changed() {
	JoltObject3D::_space_changed();

	_update_kinematic_transform();
	_update_group_filter();
	_update_joint_constraints();
	_update_sleep_allowed();
	_update_environmental_properties();
}

void JoltBody3D::_areas_changed() {
	has_point_gravity = false;
	for (JoltArea3D *area : areas) {
		if (area->is_point_gravity()) {
			has_point_gravity = true;
			break;
		}
	}

	Vector3 previous_total_gravity = total_gravity;
	_update_environmental_properties();

	if (!total_gravity.is_equal_approx(previous_total_gravity)) {
		wake_up();
	}
}

void JoltBody3D::_joints_changed() { wake_up(); }
void JoltBody3D::_transform_changed() { wake_up(); }
void JoltBody3D::_motion_changed() { wake_up(); }
void JoltBody3D::_exceptions_changed() { _update_group_filter(); }

void JoltBody3D::_axis_lock_changed() {
	_update_mass_properties();
	wake_up();
}

void JoltBody3D::_contact_reporting_changed() {
	_update_possible_kinematic_contacts();
	_update_sleep_allowed();
	wake_up();
}

void JoltBody3D::_sleep_allowed_changed() {
	_update_sleep_allowed();
	wake_up();
}

JoltBody3D::JoltBody3D() :
		JoltObject3D(OBJECT_TYPE_BODY),
		call_queries_element(this) {}

JoltBody3D::~JoltBody3D() {
	if (direct_state != nullptr) {
		memdelete(direct_state);
		direct_state = nullptr;
	}
}

void JoltBody3D::set_transform(Transform3D p_transform) {
	JOLT_ENSURE_SCALE_NOT_ZERO(p_transform, vformat("An invalid transform was passed to physics body '%s'.", to_string()));

	Vector3 new_scale;
	JoltMath::decompose(p_transform, new_scale);

	if (!scale.is_equal_approx(new_scale)) {
		scale = new_scale;
		_shapes_changed();
	}

	if (!in_space()) {
		jolt_settings->mPosition = to_jolt_r(p_transform.origin);
		jolt_settings->mRotation = to_jolt(p_transform.basis);
	} else if (is_kinematic()) {
		kinematic_transform = p_transform;
	} else {
		space->get_body_iface().SetPositionAndRotation(jolt_body->GetID(), to_jolt_r(p_transform.origin), to_jolt(p_transform.basis), JPH::EActivation::DontActivate);
	}

	_transform_changed();
}

void JoltBody3D::set_custom_integrator(bool p_enabled) {
	if (custom_integrator == p_enabled) {
		return;
	}

	custom_integrator = p_enabled;

	if (in_space()) {
		jolt_body->ResetForce();
		jolt_body->ResetTorque();
	}

	_motion_changed();
}

bool JoltBody3D::is_sleeping() const {
	if (!in_space()) {
		return sleep_initially;
	} else {
		return !jolt_body->IsActive();
	}
}

bool JoltBody3D::is_sleep_actually_allowed() const {
	return sleep_allowed && !(is_kinematic() && reports_contacts());
}

void JoltBody3D::set_is_sleeping(bool p_enabled) {
	if (!in_space()) {
		sleep_initially = p_enabled;
	} else {
		space->set_is_object_sleeping(jolt_body->GetID(), p_enabled);
	}
}

void JoltBody3D::set_is_sleep_allowed(bool p_enabled) {
	if (sleep_allowed == p_enabled) {
		return;
	}

	sleep_allowed = p_enabled;

	_sleep_allowed_changed();
}

Basis JoltBody3D::get_principal_inertia_axes() const {
	ERR_FAIL_COND_V_MSG(!in_space(), Basis(),
			vformat("Failed to retrieve principal inertia axes of '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a node, try "
					"adding the node to a scene tree first.",
					to_string()));

	if (unlikely(is_static() || is_kinematic())) {
		return Basis();
	}

	return to_godot(jolt_body->GetRotation() * jolt_body->GetMotionPropertiesUnchecked()->GetInertiaRotation());
}

Vector3 JoltBody3D::get_inverse_inertia() const {
	ERR_FAIL_COND_V_MSG(!in_space(), Vector3(),
			vformat("Failed to retrieve inverse inertia of '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, try adding the "
					"node to a scene tree first.",
					to_string()));

	if (unlikely(is_static() || is_kinematic())) {
		return Vector3();
	}

	return to_godot(jolt_body->GetMotionPropertiesUnchecked()->GetLocalSpaceInverseInertia().GetDiagonal3());
}

Basis JoltBody3D::get_inverse_inertia_tensor() const {
	ERR_FAIL_COND_V_MSG(!in_space(), Basis(),
			vformat("Failed to retrieve inverse inertia tensor of '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a node, try "
					"adding the node to a scene tree first.",
					to_string()));

	if (unlikely(is_static() || is_kinematic())) {
		return Basis();
	}

	return to_godot(jolt_body->GetInverseInertia()).basis;
}

void JoltBody3D::set_linear_velocity(const Vector3 &p_velocity) {
	if (is_static() || is_kinematic()) {
		linear_surface_velocity = p_velocity;
	} else {
		if (!in_space()) {
			jolt_settings->mLinearVelocity = to_jolt(p_velocity);
		} else {
			jolt_body->GetMotionPropertiesUnchecked()->SetLinearVelocityClamped(to_jolt(p_velocity));
		}
	}

	_motion_changed();
}

void JoltBody3D::set_angular_velocity(const Vector3 &p_velocity) {
	if (is_static() || is_kinematic()) {
		angular_surface_velocity = p_velocity;
	} else {
		if (!in_space()) {
			jolt_settings->mAngularVelocity = to_jolt(p_velocity);
		} else {
			jolt_body->GetMotionPropertiesUnchecked()->SetAngularVelocityClamped(to_jolt(p_velocity));
		}
	}

	_motion_changed();
}

void JoltBody3D::set_axis_velocity(const Vector3 &p_axis_velocity) {
	const Vector3 axis = p_axis_velocity.normalized();

	if (!in_space()) {
		Vector3 linear_velocity = to_godot(jolt_settings->mLinearVelocity);
		linear_velocity -= axis * axis.dot(linear_velocity);
		linear_velocity += p_axis_velocity;
		jolt_settings->mLinearVelocity = to_jolt(linear_velocity);
	} else {
		Vector3 linear_velocity = get_linear_velocity();
		linear_velocity -= axis * axis.dot(linear_velocity);
		linear_velocity += p_axis_velocity;
		set_linear_velocity(linear_velocity);
	}

	_motion_changed();
}

Vector3 JoltBody3D::get_velocity_at_position(const Vector3 &p_position) const {
	if (unlikely(!in_space())) {
		return Vector3();
	}

	const JPH::MotionProperties &motion_properties = *jolt_body->GetMotionPropertiesUnchecked();
	const Vector3 total_linear_velocity = to_godot(motion_properties.GetLinearVelocity()) + linear_surface_velocity;
	const Vector3 total_angular_velocity = to_godot(motion_properties.GetAngularVelocity()) + angular_surface_velocity;
	const Vector3 com_to_pos = p_position - to_godot(jolt_body->GetCenterOfMassPosition());

	return total_linear_velocity + total_angular_velocity.cross(com_to_pos);
}

void JoltBody3D::set_center_of_mass_custom(const Vector3 &p_center_of_mass) {
	if (custom_center_of_mass && p_center_of_mass == center_of_mass_custom) {
		return;
	}

	custom_center_of_mass = true;
	center_of_mass_custom = p_center_of_mass;

	_shapes_changed();
}

void JoltBody3D::set_max_contacts_reported(int p_count) {
	ERR_FAIL_INDEX(p_count, PS3DC::MAX_CONTACTS_REPORTED_3D_MAX);

	if (unlikely((int)contacts.size() == p_count)) {
		return;
	}

	contacts.resize(p_count);
	contact_count = MIN(contact_count, p_count);

	const bool use_manifold_reduction = !reports_contacts();

	if (!in_space()) {
		jolt_settings->mUseManifoldReduction = use_manifold_reduction;
	} else {
		space->get_body_iface().SetUseManifoldReduction(jolt_body->GetID(), use_manifold_reduction);
	}

	_contact_reporting_changed();
}

void JoltBody3D::reset_mass_properties() {
	if (custom_center_of_mass) {
		custom_center_of_mass = false;
		center_of_mass_custom.zero();

		_shapes_changed();
	}

	inertia.zero();

	_update_mass_properties();
}

void JoltBody3D::apply_force(const Vector3 &p_force, const Vector3 &p_position) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply force to '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, "
					"try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || custom_integrator || p_force == Vector3()) {
		return;
	}

	jolt_body->AddForce(to_jolt(p_force), jolt_body->GetPosition() + to_jolt(p_position));

	_motion_changed();
}

void JoltBody3D::apply_central_force(const Vector3 &p_force) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply central force to '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a "
					"node, try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || custom_integrator || p_force == Vector3()) {
		return;
	}

	jolt_body->AddForce(to_jolt(p_force));

	_motion_changed();
}

void JoltBody3D::apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply impulse to '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, "
					"try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || p_impulse == Vector3()) {
		return;
	}

	jolt_body->AddImpulse(to_jolt(p_impulse), jolt_body->GetPosition() + to_jolt(p_position));

	_motion_changed();
}

void JoltBody3D::apply_central_impulse(const Vector3 &p_impulse) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply central impulse to '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a "
					"node, try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || p_impulse == Vector3()) {
		return;
	}

	jolt_body->AddImpulse(to_jolt(p_impulse));

	_motion_changed();
}

void JoltBody3D::apply_torque(const Vector3 &p_torque) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply torque to '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, "
					"try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || custom_integrator || p_torque == Vector3()) {
		return;
	}

	jolt_body->AddTorque(to_jolt(p_torque));

	_motion_changed();
}

void JoltBody3D::apply_torque_impulse(const Vector3 &p_impulse) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply torque impulse to '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a "
					"node, try adding the node to a scene tree first.",
					to_string()));

	if (unlikely(!is_rigid()) || p_impulse == Vector3()) {
		return;
	}

	jolt_body->AddAngularImpulse(to_jolt(p_impulse));

	_motion_changed();
}

void JoltBody3D::add_constant_central_force(const Vector3 &p_force) {
	if (p_force == Vector3()) {
		return;
	}

	constant_force += p_force;

	_motion_changed();
}

void JoltBody3D::add_constant_force(const Vector3 &p_force, const Vector3 &p_position) {
	if (p_force == Vector3()) {
		return;
	}

	constant_force += p_force;
	constant_torque += (p_position - get_center_of_mass_relative()).cross(p_force);

	_motion_changed();
}

void JoltBody3D::add_constant_torque(const Vector3 &p_torque) {
	if (p_torque == Vector3()) {
		return;
	}

	constant_torque += p_torque;

	_motion_changed();
}

Vector3 JoltBody3D::get_constant_force() const { return constant_force; }

void JoltBody3D::set_constant_force(const Vector3 &p_force) {
	if (constant_force == p_force) {
		return;
	}

	constant_force = p_force;

	_motion_changed();
}

Vector3 JoltBody3D::get_constant_torque() const { return constant_torque; }

void JoltBody3D::set_constant_torque(const Vector3 &p_torque) {
	if (constant_torque == p_torque) {
		return;
	}

	constant_torque = p_torque;

	_motion_changed();
}

void JoltBody3D::add_collision_exception(const RID &p_excepted_body) {
	exceptions.push_back(p_excepted_body);

	_exceptions_changed();
}

void JoltBody3D::remove_collision_exception(const RID &p_excepted_body) {
	exceptions.erase(p_excepted_body);

	_exceptions_changed();
}

bool JoltBody3D::has_collision_exception(const RID &p_excepted_body) const {
	return exceptions.find(p_excepted_body) >= 0;
}

void JoltBody3D::add_area(JoltArea3D *p_area) {
	int i = 0;
	for (; i < (int)areas.size(); i++) {
		if (p_area->get_priority() > areas[i]->get_priority()) {
			break;
		}
	}

	areas.insert(i, p_area);

	_areas_changed();
}

void JoltBody3D::remove_area(JoltArea3D *p_area) {
	areas.erase(p_area);

	_areas_changed();
}

void JoltBody3D::update_area(JoltArea3D *p_area, bool p_priority_changed) {
	if (p_priority_changed) {
		areas.erase(p_area);
		add_area(p_area);
	} else {
		_areas_changed();
	}
}

void JoltBody3D::add_joint(JoltJoint3D *p_joint) {
	joints.push_back(p_joint);

	_joints_changed();
}

void JoltBody3D::remove_joint(JoltJoint3D *p_joint) {
	joints.erase(p_joint);

	_joints_changed();
}

PhysicsDirectBodyState3D *JoltBody3D::get_direct_state() {
	if (direct_state == nullptr) {
		direct_state = memnew(PhysicsDirectBodyState3D(this));
	}

	return direct_state;
}

void JoltBody3D::set_mode(PS3DE::BodyMode p_mode) {
	if (p_mode == mode) {
		return;
	}

	mode = p_mode;

	if (!in_space()) {
		_mode_changed();
		return;
	}

	const JPH::EMotionType motion_type = _get_motion_type();

	if (motion_type == JPH::EMotionType::Static) {
		put_to_sleep();
	}

	jolt_body->SetMotionType(motion_type);

	if (motion_type != JPH::EMotionType::Static) {
		wake_up();
	}

	if (motion_type == JPH::EMotionType::Kinematic) {
		jolt_body->SetLinearVelocity(JPH::Vec3::sZero());
		jolt_body->SetAngularVelocity(JPH::Vec3::sZero());
	}

	linear_surface_velocity = Vector3();
	angular_surface_velocity = Vector3();

	_mode_changed();
}

bool JoltBody3D::is_ccd_enabled() const {
	if (!in_space()) {
		return jolt_settings->mMotionQuality == JPH::EMotionQuality::LinearCast;
	} else {
		return !is_static() && jolt_body->GetMotionProperties()->GetMotionQuality() == JPH::EMotionQuality::LinearCast;
	}
}

void JoltBody3D::set_ccd_enabled(bool p_enabled) {
	const JPH::EMotionQuality motion_quality = p_enabled ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;

	if (!in_space()) {
		jolt_settings->mMotionQuality = motion_quality;
	} else {
		space->get_body_iface().SetMotionQuality(jolt_body->GetID(), motion_quality);
	}
}

void JoltBody3D::set_mass(float p_mass) {
	if (p_mass != mass) {
		mass = p_mass;
		_update_mass_properties();
	}
}

void JoltBody3D::set_inertia(const Vector3 &p_inertia) {
	if (p_inertia != inertia) {
		inertia = p_inertia;
		_update_mass_properties();
	}
}

float JoltBody3D::get_bounce() const {
	if (!in_space()) {
		return jolt_settings->mRestitution;
	} else {
		return jolt_body->GetRestitution();
	}
}

void JoltBody3D::set_bounce(float p_bounce) {
	if (!in_space()) {
		jolt_settings->mRestitution = p_bounce;
	} else {
		jolt_body->SetRestitution(p_bounce);
	}
}

float JoltBody3D::get_friction() const {
	if (!in_space()) {
		return jolt_settings->mFriction;
	} else {
		return jolt_body->GetFriction();
	}
}

void JoltBody3D::set_friction(float p_friction) {
	if (!in_space()) {
		jolt_settings->mFriction = p_friction;
	} else {
		jolt_body->SetFriction(p_friction);
	}
}

float JoltBody3D::get_gravity_scale() const {
	if (!in_space()) {
		return jolt_settings->mGravityFactor;
	} else {
		return jolt_body->GetMotionPropertiesUnchecked()->GetGravityFactor();
	}
}

void JoltBody3D::set_gravity_scale(float p_scale) {
	if (!in_space()) {
		jolt_settings->mGravityFactor = p_scale;
	} else {
		jolt_body->GetMotionPropertiesUnchecked()->SetGravityFactor(p_scale);
		_update_environmental_properties();
	}
}

void JoltBody3D::set_linear_damp(float p_damp) {
	p_damp = MAX(0.0f, p_damp);

	if (p_damp == linear_damp) {
		return;
	}

	linear_damp = p_damp;

	_update_environmental_properties();
}

void JoltBody3D::set_angular_damp(float p_damp) {
	p_damp = MAX(0.0f, p_damp);

	if (p_damp == angular_damp) {
		return;
	}

	angular_damp = p_damp;

	_update_environmental_properties();
}

void JoltBody3D::set_linear_damp_mode(DampMode p_mode) {
	if (p_mode == linear_damp_mode) {
		return;
	}

	linear_damp_mode = p_mode;

	_update_environmental_properties();
}

void JoltBody3D::set_angular_damp_mode(DampMode p_mode) {
	if (p_mode == angular_damp_mode) {
		return;
	}

	angular_damp_mode = p_mode;

	_update_environmental_properties();
}

bool JoltBody3D::is_axis_locked(PS3DE::BodyAxis p_axis) const {
	return (locked_axes & (uint32_t)p_axis) != 0;
}

void JoltBody3D::set_axis_lock(PS3DE::BodyAxis p_axis, bool p_enabled) {
	const uint32_t previous_locked_axes = locked_axes;

	if (p_enabled) {
		locked_axes |= (uint32_t)p_axis;
	} else {
		locked_axes &= ~(uint32_t)p_axis;
	}

	if (previous_locked_axes != locked_axes) {
		_axis_lock_changed();
	}
}

bool JoltBody3D::can_interact_with(const JoltBody3D &p_other) const {
	return (can_collide_with(p_other) || p_other.can_collide_with(*this)) &&
			!has_collision_exception(p_other.get_rid()) && !p_other.has_collision_exception(rid);
}

bool JoltBody3D::can_interact_with(const JoltSoftBody3D &p_other) const {
	return p_other.can_interact_with(*this);
}

bool JoltBody3D::can_interact_with(const JoltArea3D &p_other) const {
	return p_other.can_interact_with(*this);
}

bool JoltBody3D::reports_all_kinematic_contacts() const { return false; }

void JoltBody3D::add_contact(const JoltBody3D *, float, int, int, const Vector3 &, const Vector3 &, const Vector3 &, const Vector3 &, const Vector3 &, const Vector3 &) {}

void JoltBody3D::_on_wake_up() {}
void JoltBody3D::call_queries() {}
void JoltBody3D::pre_step(float p_step) {}

// ============================================================================
// JoltSoftBody3D
// ============================================================================

namespace {

template <typename TJoltVertex>
void pin_vertices(const JoltSoftBody3D &p_body, const HashSet<int> &p_pinned_vertices, const LocalVector<int> &p_mesh_to_physics, JPH::Array<TJoltVertex> &r_physics_vertices) {
	const int mesh_vertex_count = p_mesh_to_physics.size();
	const int physics_vertex_count = (int)r_physics_vertices.size();

	for (int mesh_index : p_pinned_vertices) {
		ERR_CONTINUE_MSG(mesh_index < 0 || mesh_index >= mesh_vertex_count,
				vformat("Index %d of pinned vertex in soft body '%s' is out of bounds. There are only "
						"%d vertices in the current mesh.",
						mesh_index, p_body.to_string(), mesh_vertex_count));

		const int physics_index = p_mesh_to_physics[mesh_index];
		ERR_CONTINUE_MSG(physics_index < 0 || physics_index >= physics_vertex_count,
				vformat("Index %d of pinned vertex in soft body '%s' is out of bounds. There are only "
						"%d vertices in the current mesh. This should not happen. Please report this.",
						physics_index, p_body.to_string(), physics_vertex_count));

		r_physics_vertices[physics_index].mInvMass = 0.0f;
	}
}

} // namespace

JPH::BroadPhaseLayer JoltSoftBody3D::_get_broad_phase_layer() const {
	return JoltBroadPhaseLayer::BODY_DYNAMIC;
}

JPH::ObjectLayer JoltSoftBody3D::_get_object_layer() const {
	ERR_FAIL_NULL_V(space, 0);

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

void JoltSoftBody3D::_space_changed() {
	JoltObject3D::_space_changed();

	_update_mass();
	_update_pressure();
	_update_damping();
	_update_simulation_precision();
	_update_group_filter();
}

void JoltSoftBody3D::_update_pressure() {
	if (!in_space()) {
		jolt_soft_settings->mPressure = pressure;
		return;
	}

	JPH::SoftBodyMotionProperties &motion_properties = static_cast<JPH::SoftBodyMotionProperties &>(*jolt_body->GetMotionPropertiesUnchecked());
	motion_properties.SetPressure(pressure);
}

void JoltSoftBody3D::_update_simulation_precision() {
	if (!in_space()) {
		jolt_soft_settings->mNumIterations = (JPH::uint32)simulation_precision;
		return;
	}

	JPH::SoftBodyMotionProperties &motion_properties = static_cast<JPH::SoftBodyMotionProperties &>(*jolt_body->GetMotionPropertiesUnchecked());
	motion_properties.SetNumIterations((JPH::uint32)simulation_precision);
}

void JoltSoftBody3D::_try_rebuild() {
	if (space != nullptr) {
		_reset_space();
	}
}

void JoltSoftBody3D::_mesh_changed() { _try_rebuild(); }
void JoltSoftBody3D::_simulation_precision_changed() { wake_up(); }

void JoltSoftBody3D::_mass_changed() {
	_update_mass();
	wake_up();
}

void JoltSoftBody3D::_pressure_changed() {
	_update_pressure();
	wake_up();
}

void JoltSoftBody3D::_damping_changed() {
	_update_damping();
	wake_up();
}

void JoltSoftBody3D::_pins_changed() {
	_update_mass();
	wake_up();
}

void JoltSoftBody3D::_vertices_changed() { wake_up(); }
void JoltSoftBody3D::_exceptions_changed() { _update_group_filter(); }
void JoltSoftBody3D::_motion_changed() { wake_up(); }
void JoltSoftBody3D::_transform_changed() { wake_up(); }
void JoltSoftBody3D::_areas_changed() { wake_up(); }

JoltSoftBody3D::JoltSoftBody3D() :
		JoltObject3D(OBJECT_TYPE_SOFT_BODY) {
	jolt_soft_settings->mRestitution = 0.0f;
	jolt_soft_settings->mFriction = 1.0f;
	jolt_soft_settings->mUpdatePosition = true;
	jolt_soft_settings->mMakeRotationIdentity = false;
}

JoltSoftBody3D::~JoltSoftBody3D() {
	if (jolt_soft_settings != nullptr) {
		delete jolt_soft_settings;
		jolt_soft_settings = nullptr;
	}
}

bool JoltSoftBody3D::can_interact_with(const JoltSoftBody3D &p_other) const {
	return (can_collide_with(p_other) || p_other.can_collide_with(*this)) &&
			!has_collision_exception(p_other.get_rid()) && !p_other.has_collision_exception(rid);
}

bool JoltSoftBody3D::can_interact_with(const JoltArea3D &p_other) const {
	return p_other.can_interact_with(*this);
}

Vector3 JoltSoftBody3D::get_velocity_at_position(const Vector3 &p_position) const {
	return Vector3();
}

void JoltSoftBody3D::pre_step(float p_step) { _apply_environmental_forces(p_step); }

void JoltSoftBody3D::set_mesh(const RID &p_mesh) {
	if (unlikely(mesh == p_mesh)) {
		return;
	}

	mesh = p_mesh;
	_mesh_changed();
}

bool JoltSoftBody3D::is_sleeping() const {
	if (!in_space()) {
		return false;
	} else {
		return !jolt_body->IsActive();
	}
}

void JoltSoftBody3D::apply_vertex_force(int p_index, const Vector3 &p_force) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply force to '%s'. Doing so without a physics space is "
					"not supported when using Jolt Physics. If this relates to a node, "
					"try adding the node to a scene tree first.",
					to_string()));

	apply_vertex_impulse(p_index, p_force * space->get_last_step());
}

void JoltSoftBody3D::apply_central_impulse(const Vector3 &p_impulse) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply central impulse to '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a "
					"node, try adding the node to a scene tree first.",
					to_string()));

	JPH::SoftBodyMotionProperties &motion_properties = static_cast<JPH::SoftBodyMotionProperties &>(*jolt_body->GetMotionPropertiesUnchecked());
	JPH::Array<JPH::SoftBodyVertex> &physics_vertices = motion_properties.GetVertices();

	const JPH::Vec3 impulse = to_jolt(p_impulse) / physics_vertices.size();

	for (JPH::SoftBodyVertex &physics_vertex : physics_vertices) {
		if (physics_vertex.mInvMass > 0.0f) {
			physics_vertex.mVelocity += impulse * physics_vertex.mInvMass;
		}
	}

	_motion_changed();
}

void JoltSoftBody3D::apply_central_force(const Vector3 &p_force) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to apply central force to '%s'. Doing so without a physics "
					"space is not supported when using Jolt Physics. If this relates to a "
					"node, try adding the node to a scene tree first.",
					to_string()));

	jolt_body->AddForce(to_jolt(p_force));

	_motion_changed();
}

void JoltSoftBody3D::set_is_sleeping(bool p_enabled) {
	if (!in_space()) {
		return;
	}

	space->set_is_object_sleeping(jolt_body->GetID(), p_enabled);
}

bool JoltSoftBody3D::is_sleep_allowed() const {
	if (!in_space()) {
		return jolt_soft_settings->mAllowSleeping;
	} else {
		return jolt_body->GetAllowSleeping();
	}
}

void JoltSoftBody3D::set_is_sleep_allowed(bool p_enabled) {
	if (!in_space()) {
		jolt_soft_settings->mAllowSleeping = p_enabled;
	} else {
		jolt_body->SetAllowSleeping(p_enabled);
	}
}

void JoltSoftBody3D::set_simulation_precision(int p_precision) {
	if (unlikely(simulation_precision == p_precision)) {
		return;
	}

	simulation_precision = MAX(p_precision, 0);

	_simulation_precision_changed();
}

void JoltSoftBody3D::set_mass(float p_mass) {
	ERR_FAIL_COND(p_mass <= 0.0);

	if (unlikely(mass == p_mass)) {
		return;
	}

	mass = p_mass;

	_mass_changed();
}

float JoltSoftBody3D::get_stiffness_coefficient() const { return stiffness_coefficient; }

void JoltSoftBody3D::set_stiffness_coefficient(float p_coefficient) {
	stiffness_coefficient = CLAMP(p_coefficient, 0.0f, 1.0f);
}

float JoltSoftBody3D::get_shrinking_factor() const { return shrinking_factor; }

void JoltSoftBody3D::set_shrinking_factor(float p_shrinking_factor) {
	shrinking_factor = p_shrinking_factor;
}

void JoltSoftBody3D::set_pressure(float p_pressure) {
	if (unlikely(pressure == p_pressure)) {
		return;
	}

	pressure = MAX(p_pressure, 0.0f);

	_pressure_changed();
}

void JoltSoftBody3D::set_linear_damping(float p_damping) {
	if (unlikely(linear_damping == p_damping)) {
		return;
	}

	linear_damping = MAX(p_damping, 0.0f);

	_damping_changed();
}

float JoltSoftBody3D::get_drag() const { return 0.0f; }
void JoltSoftBody3D::set_drag(float p_drag) {}

Transform3D JoltSoftBody3D::get_transform() const {
	return Transform3D();
}

void JoltSoftBody3D::set_transform(const Transform3D &p_transform) {
	ERR_FAIL_COND_MSG(!in_space(),
			vformat("Failed to set transform for '%s'. Doing so without a physics space "
					"is not supported when using Jolt Physics. If this relates to a node, "
					"try adding the node to a scene tree first.",
					to_string()));

	const JPH::Mat44 relative_transform = to_jolt(p_transform.orthonormalized());

	JPH::BodyInterface &body_iface = space->get_body_iface();
	body_iface.SetPosition(jolt_body->GetID(), jolt_body->GetPosition() + relative_transform.GetTranslation(), JPH::EActivation::DontActivate);

	JPH::SoftBodyMotionProperties &motion_properties = static_cast<JPH::SoftBodyMotionProperties &>(*jolt_body->GetMotionPropertiesUnchecked());
	JPH::Array<JPH::SoftBodyVertex> &physics_vertices = motion_properties.GetVertices();

	for (JPH::SoftBodyVertex &vertex : physics_vertices) {
		vertex.mPosition = vertex.mPreviousPosition = relative_transform.Multiply3x3(vertex.mPosition);
		vertex.mVelocity = JPH::Vec3::sZero();
	}

	_transform_changed();
}

AABB JoltSoftBody3D::get_bounds() const {
	ERR_FAIL_COND_V_MSG(!in_space(), AABB(),
			vformat("Failed to retrieve world bounds of '%s'. Doing so without a physics space is not "
					"supported when using Jolt Physics. If this relates to a node, try adding the node "
					"to a scene tree first.",
					to_string()));
	return to_godot(jolt_body->GetWorldSpaceBounds());
}

void JoltSoftBody3D::_update_mass() {}
void JoltSoftBody3D::_update_damping() {}
void JoltSoftBody3D::_update_group_filter() {}
bool JoltSoftBody3D::has_collision_exception(const RID &) const { return false; }
void JoltSoftBody3D::apply_vertex_impulse(int, const Vector3 &) {}
void JoltSoftBody3D::update_rendering_server(PhysicsServer3DRenderingServerHandler *) {}
void JoltSoftBody3D::unpin_vertex(int) {}
void JoltSoftBody3D::unpin_all_vertices() {}
void JoltSoftBody3D::set_vertex_position(int, const Vector3 &) {}
void JoltSoftBody3D::remove_collision_exception(const RID &) {}
void JoltSoftBody3D::remove_area(JoltArea3D *) {}
void JoltSoftBody3D::pin_vertex(int) {}
bool JoltSoftBody3D::is_vertex_pinned(int) const { return false; }
Vector3 JoltSoftBody3D::get_vertex_position(int) { return Vector3(); }
bool JoltSoftBody3D::can_interact_with(const JoltBody3D &) const { return false; }
void JoltSoftBody3D::_apply_environmental_forces(float) {}
void JoltSoftBody3D::add_collision_exception(const RID &) {}
void JoltSoftBody3D::add_area(JoltArea3D *) {}
void JoltSoftBody3D::_add_to_space() {}
void JoltSoftBody3D::_space_changing() {}

// ============================================================================
// PhysicsDirectBodyState3D
// ============================================================================

PhysicsDirectBodyState3D::PhysicsDirectBodyState3D(JoltBody3D *p_body) :
		body(p_body) {}

Vector3 PhysicsDirectBodyState3D::get_total_gravity() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_total_gravity();
}

real_t PhysicsDirectBodyState3D::get_total_angular_damp() const {
	ERR_FAIL_NULL_V(body, 0.0);
	return (real_t)body->get_total_angular_damp();
}

real_t PhysicsDirectBodyState3D::get_total_linear_damp() const {
	ERR_FAIL_NULL_V(body, 0.0);
	return (real_t)body->get_total_linear_damp();
}

Vector3 PhysicsDirectBodyState3D::get_center_of_mass() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_center_of_mass_relative();
}

Vector3 PhysicsDirectBodyState3D::get_center_of_mass_local() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_center_of_mass_local();
}

Basis PhysicsDirectBodyState3D::get_principal_inertia_axes() const {
	ERR_FAIL_NULL_V(body, Basis());
	return body->get_principal_inertia_axes();
}

real_t PhysicsDirectBodyState3D::get_inverse_mass() const {
	ERR_FAIL_NULL_V(body, 0.0);
	return 1.0 / body->get_mass();
}

Vector3 PhysicsDirectBodyState3D::get_inverse_inertia() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_inverse_inertia();
}

Basis PhysicsDirectBodyState3D::get_inverse_inertia_tensor() const {
	ERR_FAIL_NULL_V(body, Basis());
	return body->get_inverse_inertia_tensor();
}

Vector3 PhysicsDirectBodyState3D::get_linear_velocity() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_linear_velocity();
}

void PhysicsDirectBodyState3D::set_linear_velocity(const Vector3 &p_velocity) {
	ERR_FAIL_NULL(body);
	body->set_linear_velocity(p_velocity);
}

Vector3 PhysicsDirectBodyState3D::get_angular_velocity() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_angular_velocity();
}

void PhysicsDirectBodyState3D::set_angular_velocity(const Vector3 &p_velocity) {
	ERR_FAIL_NULL(body);
	body->set_angular_velocity(p_velocity);
}

void PhysicsDirectBodyState3D::set_transform(const Transform3D &p_transform) {
	ERR_FAIL_NULL(body);
	body->set_transform(p_transform);
}

Transform3D PhysicsDirectBodyState3D::get_transform() const {
	ERR_FAIL_NULL_V(body, Transform3D());
	return body->get_transform_scaled();
}

Vector3 PhysicsDirectBodyState3D::get_velocity_at_local_position(const Vector3 &p_local_position) const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_velocity_at_position(body->get_position() + p_local_position);
}

void PhysicsDirectBodyState3D::apply_central_impulse(const Vector3 &p_impulse) {
	ERR_FAIL_NULL(body);
	body->apply_central_impulse(p_impulse);
}

void PhysicsDirectBodyState3D::apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position) {
	ERR_FAIL_NULL(body);
	body->apply_impulse(p_impulse, p_position);
}

void PhysicsDirectBodyState3D::apply_torque_impulse(const Vector3 &p_impulse) {
	ERR_FAIL_NULL(body);
	body->apply_torque_impulse(p_impulse);
}

void PhysicsDirectBodyState3D::apply_central_force(const Vector3 &p_force) {
	ERR_FAIL_NULL(body);
	body->apply_central_force(p_force);
}

void PhysicsDirectBodyState3D::apply_force(const Vector3 &p_force, const Vector3 &p_position) {
	ERR_FAIL_NULL(body);
	body->apply_force(p_force, p_position);
}

void PhysicsDirectBodyState3D::apply_torque(const Vector3 &p_torque) {
	ERR_FAIL_NULL(body);
	body->apply_torque(p_torque);
}

void PhysicsDirectBodyState3D::add_constant_central_force(const Vector3 &p_force) {
	ERR_FAIL_NULL(body);
	body->add_constant_central_force(p_force);
}

void PhysicsDirectBodyState3D::add_constant_force(const Vector3 &p_force, const Vector3 &p_position) {
	ERR_FAIL_NULL(body);
	body->add_constant_force(p_force, p_position);
}

void PhysicsDirectBodyState3D::add_constant_torque(const Vector3 &p_torque) {
	ERR_FAIL_NULL(body);
	body->add_constant_torque(p_torque);
}

Vector3 PhysicsDirectBodyState3D::get_constant_force() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_constant_force();
}

void PhysicsDirectBodyState3D::set_constant_force(const Vector3 &p_force) {
	ERR_FAIL_NULL(body);
	body->set_constant_force(p_force);
}

Vector3 PhysicsDirectBodyState3D::get_constant_torque() const {
	ERR_FAIL_NULL_V(body, Vector3());
	return body->get_constant_torque();
}

void PhysicsDirectBodyState3D::set_constant_torque(const Vector3 &p_torque) {
	ERR_FAIL_NULL(body);
	body->set_constant_torque(p_torque);
}

bool PhysicsDirectBodyState3D::is_sleeping() const {
	ERR_FAIL_NULL_V(body, false);
	return body->is_sleeping();
}

void PhysicsDirectBodyState3D::set_sleep_state(bool p_enabled) {
	ERR_FAIL_NULL(body);
	body->set_is_sleeping(p_enabled);
}

void PhysicsDirectBodyState3D::set_collision_layer(uint32_t p_layer) {
	ERR_FAIL_NULL(body);
	body->set_collision_layer(p_layer);
}

uint32_t PhysicsDirectBodyState3D::get_collision_layer() const {
	ERR_FAIL_NULL_V(body, 0);
	return body->get_collision_layer();
}

void PhysicsDirectBodyState3D::set_collision_mask(uint32_t p_mask) {
	ERR_FAIL_NULL(body);
	body->set_collision_mask(p_mask);
}

uint32_t PhysicsDirectBodyState3D::get_collision_mask() const {
	ERR_FAIL_NULL_V(body, 0);
	return body->get_collision_mask();
}

int PhysicsDirectBodyState3D::get_contact_count() const {
	ERR_FAIL_NULL_V(body, 0);
	return body->get_contact_count();
}

Vector3 PhysicsDirectBodyState3D::get_contact_local_position(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).position;
}

Vector3 PhysicsDirectBodyState3D::get_contact_local_normal(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).normal;
}

Vector3 PhysicsDirectBodyState3D::get_contact_impulse(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).impulse;
}

int PhysicsDirectBodyState3D::get_contact_local_shape(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, 0);
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), 0);
	return body->get_contact(p_contact_idx).shape_index;
}

Vector3 PhysicsDirectBodyState3D::get_contact_local_velocity_at_position(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).velocity;
}

RID PhysicsDirectBodyState3D::get_contact_collider(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, RID());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), RID());
	return body->get_contact(p_contact_idx).collider_rid;
}

Vector3 PhysicsDirectBodyState3D::get_contact_collider_position(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).collider_position;
}

int PhysicsDirectBodyState3D::get_contact_collider_shape(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, 0);
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), 0);
	return body->get_contact(p_contact_idx).collider_shape_index;
}

Vector3 PhysicsDirectBodyState3D::get_contact_collider_velocity_at_position(int p_contact_idx) const {
	ERR_FAIL_NULL_V(body, Vector3());
	ERR_FAIL_INDEX_V(p_contact_idx, (int)body->get_contact_count(), Vector3());
	return body->get_contact(p_contact_idx).collider_velocity;
}

real_t PhysicsDirectBodyState3D::get_step() const {
	ERR_FAIL_NULL_V(body, 0.0);
	ERR_FAIL_NULL_V(body->get_space(), 0.0);
	return (real_t)body->get_space()->get_last_step();
}

void PhysicsDirectBodyState3D::integrate_forces() {
	ERR_FAIL_NULL(body);
	const float step = (float)get_step();

	Vector3 linear_velocity = get_linear_velocity();
	Vector3 angular_velocity = get_angular_velocity();

	linear_velocity *= MAX(1.0f - (float)get_total_linear_damp() * step, 0.0f);
	angular_velocity *= MAX(1.0f - (float)get_total_angular_damp() * step, 0.0f);

	linear_velocity += get_total_gravity() * step;

	set_linear_velocity(linear_velocity);
	set_angular_velocity(angular_velocity);
}

PhysicsDirectSpaceState3D *PhysicsDirectBodyState3D::get_space_state() {
	ERR_FAIL_NULL_V(body, nullptr);
	ERR_FAIL_NULL_V(body->get_space(), nullptr);
	return body->get_space()->get_direct_state();
}

// ============================================================================
// Merged from modules/jolt_physics/joints/jolt_joint_3d.cpp
// ============================================================================


#include "modules/jolt_physics/jolt_physics_server_3d.h"


#include "core/config/engine.h"

#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>

namespace {

constexpr double PIN_DEFAULT_BIAS = 0.3;
constexpr double PIN_DEFAULT_DAMPING = 1.0;
constexpr double PIN_DEFAULT_IMPULSE_CLAMP = 0.0;

constexpr double HINGE_DEFAULT_BIAS = 0.3;
constexpr double HINGE_DEFAULT_LIMIT_BIAS = 0.3;
constexpr double HINGE_DEFAULT_SOFTNESS = 0.9;
constexpr double HINGE_DEFAULT_RELAXATION = 1.0;

constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_DAMPING = 0.0;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING = 1.0;

constexpr double CONE_TWIST_DEFAULT_BIAS = 0.3;
constexpr double CONE_TWIST_DEFAULT_SOFTNESS = 0.8;
constexpr double CONE_TWIST_DEFAULT_RELAXATION = 1.0;

constexpr double G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS = 0.7;
constexpr double G6DOF_DEFAULT_LINEAR_RESTITUTION = 0.5;
constexpr double G6DOF_DEFAULT_LINEAR_DAMPING = 1.0;
constexpr double G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS = 0.5;
constexpr double G6DOF_DEFAULT_ANGULAR_DAMPING = 1.0;
constexpr double G6DOF_DEFAULT_ANGULAR_RESTITUTION = 0.0;
constexpr double G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT = 0.0;
constexpr double G6DOF_DEFAULT_ANGULAR_ERP = 0.5;

double estimate_physics_step() {
	const double step = 1.0 / Engine::get_user_physics_ticks_per_second();
	const double step_scaled = step * Engine::get_effective_time_scale();

	return step_scaled;
}

} // namespace

// --- Common JoltJoint3D methods ---

void JoltJoint3D::_wake_up_bodies() {
	if (body_a != nullptr) {
		body_a->wake_up();
	}

	if (body_b != nullptr) {
		body_b->wake_up();
	}
}

void JoltJoint3D::_update_enabled() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetEnabled(enabled);
	}
}

void JoltJoint3D::_update_iterations() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetNumVelocityStepsOverride((JPH::uint)velocity_iterations);
		jolt_ref->SetNumPositionStepsOverride((JPH::uint)position_iterations);
	}
}

void JoltJoint3D::_update_priority() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetConstraintPriority((JPH::uint)solver_priority);
	}
}

void JoltJoint3D::_update_joint() {
	_update_enabled();
	_update_iterations();
	_update_priority();
}

void JoltJoint3D::_enabled_changed() {
	_update_enabled();
	_wake_up_bodies();
}

void JoltJoint3D::_iterations_changed() {
	_update_iterations();
	_wake_up_bodies();
}

String JoltJoint3D::_bodies_to_string() const {
	return vformat("'%s' and '%s'", body_a != nullptr ? body_a->to_string() : "<unknown>",
			body_b != nullptr ? body_b->to_string() : "<World>");
}

void JoltJoint3D::_reconfigure(PS3DE::JointType p_type, JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	destroy();

	if (body_a != nullptr) {
		body_a->remove_joint(this);
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
	}

	joint_type = p_type;
	velocity_iterations = 0;
	position_iterations = 0;
	solver_priority = 1;
	body_a = p_body_a;
	body_b = p_body_b;
	local_ref_a = p_local_ref_a;
	local_ref_b = p_local_ref_b;

	hinge = HingeData();
	slider = SliderData();
	cone_twist = ConeTwistData();
	g6dof = G6DOFData();

	if (body_a != nullptr) {
		body_a->add_joint(this);
	}

	if (body_b != nullptr) {
		body_b->add_joint(this);
	}

	rebuild();
}

JoltJoint3D::~JoltJoint3D() {
	if (body_a != nullptr) {
		body_a->remove_joint(this);
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
	}

	destroy();
}

JoltSpace3D *JoltJoint3D::get_space() const {
	if (body_a != nullptr && body_b != nullptr) {
		JoltSpace3D *space_a = body_a->get_space();
		JoltSpace3D *space_b = body_b->get_space();

		if (space_a == nullptr || space_b == nullptr) {
			return nullptr;
		}

		ERR_FAIL_COND_V_MSG(space_a != space_b, nullptr,
				vformat("Joint was found to connect bodies in different physics spaces. This joint "
						"will effectively be disabled. This joint connects %s.",
						_bodies_to_string()));

		return space_a;
	} else if (body_a != nullptr) {
		return body_a->get_space();
	} else if (body_b != nullptr) {
		return body_b->get_space();
	}

	return nullptr;
}

void JoltJoint3D::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;

	_enabled_changed();
}

void JoltJoint3D::set_solver_priority(int p_priority) {
	p_priority = MAX(0, p_priority);

	if (solver_priority == p_priority) {
		return;
	}

	solver_priority = p_priority;

	_update_priority();
}

void JoltJoint3D::set_solver_velocity_iterations(int p_iterations) {
	if (velocity_iterations == p_iterations) {
		return;
	}

	velocity_iterations = p_iterations;

	_iterations_changed();
}

void JoltJoint3D::set_solver_position_iterations(int p_iterations) {
	if (position_iterations == p_iterations) {
		return;
	}

	position_iterations = p_iterations;

	_iterations_changed();
}

void JoltJoint3D::set_collision_disabled(bool p_disabled) {
	collision_disabled = p_disabled;

	if (body_a == nullptr || body_b == nullptr) {
		return;
	}

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL(physics_server);

	if (collision_disabled) {
		physics_server->body_add_collision_exception(body_a->get_rid(), body_b->get_rid());
		physics_server->body_add_collision_exception(body_b->get_rid(), body_a->get_rid());
	} else {
		physics_server->body_remove_collision_exception(body_a->get_rid(), body_b->get_rid());
		physics_server->body_remove_collision_exception(body_b->get_rid(), body_a->get_rid());
	}
}

void JoltJoint3D::clear() {
	destroy();

	if (body_a != nullptr) {
		body_a->remove_joint(this);
		body_a = nullptr;
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
		body_b = nullptr;
	}

	joint_type = PS3DE::JOINT_TYPE_MAX;
	enabled = true;
	collision_disabled = false;
	velocity_iterations = 0;
	position_iterations = 0;
	solver_priority = 1;
	local_ref_a = Transform3D();
	local_ref_b = Transform3D();

	hinge = HingeData();
	slider = SliderData();
	cone_twist = ConeTwistData();
	g6dof = G6DOFData();
}

void JoltJoint3D::make_pin(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Vector3 &p_local_a, const Vector3 &p_local_b) {
	_reconfigure(PS3DE::JOINT_TYPE_PIN, p_body_a, p_body_b, Transform3D({}, p_local_a), Transform3D({}, p_local_b));
}

void JoltJoint3D::make_hinge(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_HINGE, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_slider(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_SLIDER, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_cone_twist(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_CONE_TWIST, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_generic_6dof(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_6DOF, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::destroy() {
	if (jolt_ref == nullptr) {
		return;
	}

	JoltSpace3D *space = get_space();

	if (space != nullptr) {
		space->remove_joint(this);
	}

	jolt_ref = nullptr;
}

void JoltJoint3D::rebuild() {
	switch (joint_type) {
		case PS3DE::JOINT_TYPE_PIN:
			_rebuild_pin();
			break;
		case PS3DE::JOINT_TYPE_HINGE:
			_rebuild_hinge();
			break;
		case PS3DE::JOINT_TYPE_SLIDER:
			_rebuild_slider();
			break;
		case PS3DE::JOINT_TYPE_CONE_TWIST:
			_rebuild_cone_twist();
			break;
		case PS3DE::JOINT_TYPE_6DOF:
			_rebuild_6dof();
			break;
		default:
			break;
	}
}

void JoltJoint3D::_shift_reference_frames(const Vector3 &p_linear_shift, const Vector3 &p_angular_shift, Transform3D &r_shifted_ref_a, Transform3D &r_shifted_ref_b) {
}

JPH::Constraint *JoltJoint3D::_build_fixed(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) {
	JPH::FixedConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mAutoDetectPoint = false;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mAxisX1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mAxisX2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Y));

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

float JoltJoint3D::get_applied_force() const {
	ERR_FAIL_NULL_V(jolt_ref, 0.0f);

	JoltSpace3D *space = get_space();
	ERR_FAIL_NULL_V(space, 0.0f);

	const float last_step = space->get_last_step();
	if (unlikely(last_step == 0.0f)) {
		return 0.0f;
	}

	switch (joint_type) {
		case PS3DE::JOINT_TYPE_PIN: {
			JPH::PointConstraint *constraint = static_cast<JPH::PointConstraint *>(jolt_ref.GetPtr());
			return constraint->GetTotalLambdaPosition().Length() / last_step;
		}
		case PS3DE::JOINT_TYPE_HINGE: {
			if (_is_hinge_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaPosition().Length() / last_step;
			} else {
				JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr());
				const JPH::Vec3 total_lambda = JPH::Vec3(
						constraint->GetTotalLambdaRotation()[0],
						constraint->GetTotalLambdaRotation()[1],
						constraint->GetTotalLambdaRotationLimits() + constraint->GetTotalLambdaMotor());
				return total_lambda.Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_SLIDER: {
			if (_is_slider_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaPosition().Length() / last_step;
			} else {
				JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr());
				const JPH::Vec3 total_lambda = JPH::Vec3(
						constraint->GetTotalLambdaPosition()[0],
						constraint->GetTotalLambdaPosition()[1],
						constraint->GetTotalLambdaPositionLimits() + constraint->GetTotalLambdaMotor());
				return total_lambda.Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_CONE_TWIST: {
			JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr());
			return constraint->GetTotalLambdaPosition().Length() / last_step;
		}
		case PS3DE::JOINT_TYPE_6DOF: {
			JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 total_lambda = constraint->GetTotalLambdaPosition() + constraint->GetTotalLambdaMotorTranslation();
			return total_lambda.Length() / last_step;
		}
		default:
			return 0.0f;
	}
}

float JoltJoint3D::get_applied_torque() const {
	ERR_FAIL_NULL_V(jolt_ref, 0.0f);

	JoltSpace3D *space = get_space();
	ERR_FAIL_NULL_V(space, 0.0f);

	const float last_step = space->get_last_step();
	if (unlikely(last_step == 0.0f)) {
		return 0.0f;
	}

	switch (joint_type) {
		case PS3DE::JOINT_TYPE_HINGE: {
			if (_is_hinge_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			} else {
				JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_SLIDER: {
			if (_is_slider_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			} else {
				JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_CONE_TWIST: {
			JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 swing_twist_lambda = JPH::Vec3(
					constraint->GetTotalLambdaTwist(),
					constraint->GetTotalLambdaSwingY(),
					constraint->GetTotalLambdaSwingZ());
			const float total_lambda = swing_twist_lambda.Length() + constraint->GetTotalLambdaMotor().Length();
			return total_lambda / last_step;
		}
		case PS3DE::JOINT_TYPE_6DOF: {
			JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 total_lambda = constraint->GetTotalLambdaRotation() + constraint->GetTotalLambdaMotorRotation();
			return total_lambda.Length() / last_step;
		}
		default:
			return 0.0f;
	}
}

// --- Pin Joint ---

JPH::Constraint *JoltJoint3D::_build_pin(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) {
	JPH::PointConstraintSettings constraint_settings;
	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_pin_points_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::set_pin_local_a(const Vector3 &p_local_a) {
	local_ref_a = Transform3D({}, p_local_a);
	_pin_points_changed();
}

void JoltJoint3D::set_pin_local_b(const Vector3 &p_local_b) {
	local_ref_b = Transform3D({}, p_local_b);
	_pin_points_changed();
}

double JoltJoint3D::get_pin_param(PS3DE::PinJointParam p_param) const {
	switch (p_param) {
		case PS3DE::PIN_JOINT_BIAS: {
			return PIN_DEFAULT_BIAS;
		}
		case PS3DE::PIN_JOINT_DAMPING: {
			return PIN_DEFAULT_DAMPING;
		}
		case PS3DE::PIN_JOINT_IMPULSE_CLAMP: {
			return PIN_DEFAULT_IMPULSE_CLAMP;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled pin joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_pin_param(PS3DE::PinJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::PIN_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Pin joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::PIN_JOINT_DAMPING: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_DAMPING)) {
				WARN_PRINT(vformat("Pin joint damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::PIN_JOINT_IMPULSE_CLAMP: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_IMPULSE_CLAMP)) {
				WARN_PRINT(vformat("Pin joint impulse clamp is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled pin joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

void JoltJoint3D::_rebuild_pin() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_pin(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);

	space->add_joint(this);

	_update_joint();
}

// --- Hinge Joint ---

JPH::Constraint *JoltJoint3D::_build_hinge(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const {
	JPH::HingeConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mHingeAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mNormalAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mHingeAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mNormalAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mLimitsMin = -p_limit;
	constraint_settings.mLimitsMax = p_limit;

	if (hinge.limit_spring_enabled) {
		constraint_settings.mLimitsSpringSettings.mFrequency = (float)hinge.limit_spring_frequency;
		constraint_settings.mLimitsSpringSettings.mDamping = (float)hinge.limit_spring_damping;
	}

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_hinge_motor_state() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetMotorState(hinge.motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_hinge_motor_velocity() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocity((float)-hinge.motor_target_speed);
	}
}

void JoltJoint3D::_update_hinge_motor_limit() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-hinge.motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)hinge.motor_max_torque;
	}
}

void JoltJoint3D::_hinge_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_limit_spring_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_state_changed() {
	_update_hinge_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_speed_changed() {
	_update_hinge_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_limit_changed() {
	_update_hinge_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_hinge_param(PS3DE::HingeJointParam p_param) const {
	switch (p_param) {
		case PS3DE::HINGE_JOINT_BIAS: {
			return HINGE_DEFAULT_BIAS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_UPPER: {
			return hinge.limit_upper;
		}
		case PS3DE::HINGE_JOINT_LIMIT_LOWER: {
			return hinge.limit_lower;
		}
		case PS3DE::HINGE_JOINT_LIMIT_BIAS: {
			return HINGE_DEFAULT_LIMIT_BIAS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_SOFTNESS: {
			return HINGE_DEFAULT_SOFTNESS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_RELAXATION: {
			return HINGE_DEFAULT_RELAXATION;
		}
		case PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY: {
			return hinge.motor_target_speed;
		}
		case PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE: {
			return hinge.motor_max_torque * estimate_physics_step();
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_hinge_param(PS3DE::HingeJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::HINGE_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Hinge joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_UPPER: {
			hinge.limit_upper = p_value;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_LOWER: {
			hinge.limit_lower = p_value;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_BIAS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_LIMIT_BIAS)) {
				WARN_PRINT(vformat("Hinge joint bias limit is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_SOFTNESS)) {
				WARN_PRINT(vformat("Hinge joint softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_RELAXATION: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_RELAXATION)) {
				WARN_PRINT(vformat("Hinge joint relaxation is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY: {
			hinge.motor_target_speed = p_value;
			_hinge_motor_speed_changed();
		} break;
		case PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE: {
			hinge.motor_max_torque = p_value / estimate_physics_step();
			_hinge_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_FREQUENCY: {
			return hinge.limit_spring_frequency;
		}
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_DAMPING: {
			return hinge.limit_spring_damping;
		}
		case JoltPhysicsServer3D::HINGE_JOINT_MOTOR_MAX_TORQUE: {
			return hinge.motor_max_torque;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_FREQUENCY: {
			hinge.limit_spring_frequency = p_value;
			_hinge_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_DAMPING: {
			hinge.limit_spring_damping = p_value;
			_hinge_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::HINGE_JOINT_MOTOR_MAX_TORQUE: {
			hinge.motor_max_torque = p_value;
			_hinge_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_hinge_flag(PS3DE::HingeJointFlag p_flag) const {
	switch (p_flag) {
		case PS3DE::HINGE_JOINT_FLAG_USE_LIMIT: {
			return hinge.limits_enabled;
		}
		case PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR: {
			return hinge.motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_hinge_flag(PS3DE::HingeJointFlag p_flag, bool p_enabled) {
	switch (p_flag) {
		case PS3DE::HINGE_JOINT_FLAG_USE_LIMIT: {
			hinge.limits_enabled = p_enabled;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR: {
			hinge.motor_enabled = p_enabled;
			_hinge_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

bool JoltJoint3D::get_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT_SPRING: {
			return hinge.limit_spring_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT_SPRING: {
			hinge.limit_spring_enabled = p_enabled;
			_hinge_limit_spring_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_hinge() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	float ref_shift = 0.0f;
	float limit = JPH::JPH_PI;

	if (hinge.limits_enabled && hinge.limit_lower <= hinge.limit_upper) {
		const double limit_midpoint = (hinge.limit_lower + hinge.limit_upper) / 2.0f;

		ref_shift = float(-limit_midpoint);
		limit = float(hinge.limit_upper - limit_midpoint);
	}

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(0.0f, 0.0f, ref_shift), shifted_ref_a, shifted_ref_b);

	if (_is_hinge_fixed()) {
		jolt_ref = _build_fixed(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);
	} else {
		jolt_ref = _build_hinge(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, limit);
	}

	space->add_joint(this);

	_update_joint();

	_update_hinge_motor_state();
	_update_hinge_motor_velocity();
	_update_hinge_motor_limit();
}

// --- Slider Joint ---

JPH::Constraint *JoltJoint3D::_build_slider(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const {
	JPH::SliderConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mAutoDetectPoint = false;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mSliderAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mNormalAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mSliderAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mNormalAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mLimitsMin = -p_limit;
	constraint_settings.mLimitsMax = p_limit;

	if (slider.limit_spring_enabled) {
		constraint_settings.mLimitsSpringSettings.mFrequency = (float)slider.limit_spring_frequency;
		constraint_settings.mLimitsSpringSettings.mDamping = (float)slider.limit_spring_damping;
	}

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_slider_motor_state() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetMotorState(slider.motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_slider_motor_velocity() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetTargetVelocity((float)slider.motor_target_speed);
	}
}

void JoltJoint3D::_update_slider_motor_limit() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetMotorSettings();
		motor_settings.mMinForceLimit = (float)-slider.motor_max_force;
		motor_settings.mMaxForceLimit = (float)slider.motor_max_force;
	}
}

void JoltJoint3D::_slider_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_limit_spring_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_state_changed() {
	_update_slider_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_speed_changed() {
	_update_slider_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_limit_changed() {
	_update_slider_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_slider_param(PS3DE::SliderJointParam p_param) const {
	switch (p_param) {
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER: {
			return slider.limit_upper;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER: {
			return slider.limit_lower;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_MOTION_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled slider joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_slider_param(PS3DE::SliderJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER: {
			slider.limit_upper = p_value;
			_slider_limits_changed();
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER: {
			slider.limit_lower = p_value;
			_slider_limits_changed();
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear limit restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear limit damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear motion softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear motion restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear motion damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear ortho softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear ortho restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear ortho damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER)) {
				WARN_PRINT(vformat("Slider joint angular limits are not supported when using Jolt Physics. Any such value will be ignored. Try using Generic6DOFJoint3D instead. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER)) {
				WARN_PRINT(vformat("Slider joint angular limits are not supported when using Jolt Physics. Any such value will be ignored. Try using Generic6DOFJoint3D instead. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular limit restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular limit damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular motion softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular motion restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular motion damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular ortho softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular ortho restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular ortho damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled slider joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_FREQUENCY: {
			return slider.limit_spring_frequency;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_DAMPING: {
			return slider.limit_spring_damping;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_TARGET_VELOCITY: {
			return slider.motor_target_speed;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_MAX_FORCE: {
			return slider.motor_max_force;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_FREQUENCY: {
			slider.limit_spring_frequency = p_value;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_DAMPING: {
			slider.limit_spring_damping = p_value;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_TARGET_VELOCITY: {
			slider.motor_target_speed = p_value;
			_slider_motor_speed_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_MAX_FORCE: {
			slider.motor_max_force = p_value;
			_slider_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT: {
			return slider.limits_enabled;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT_SPRING: {
			return slider.limit_spring_enabled;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_ENABLE_MOTOR: {
			return slider.motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT: {
			slider.limits_enabled = p_enabled;
			_slider_limits_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT_SPRING: {
			slider.limit_spring_enabled = p_enabled;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_ENABLE_MOTOR: {
			slider.motor_enabled = p_enabled;
			_slider_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_slider() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	float ref_shift = 0.0f;
	float limit = FLT_MAX;

	if (slider.limits_enabled && slider.limit_lower <= slider.limit_upper) {
		const double limit_midpoint = (slider.limit_lower + slider.limit_upper) / 2.0f;

		ref_shift = float(-limit_midpoint);
		limit = float(slider.limit_upper - limit_midpoint);
	}

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(ref_shift, 0.0f, 0.0f), Vector3(), shifted_ref_a, shifted_ref_b);

	if (_is_slider_fixed()) {
		jolt_ref = _build_fixed(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);
	} else {
		jolt_ref = _build_slider(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, limit);
	}

	space->add_joint(this);

	_update_joint();

	_update_slider_motor_state();
	_update_slider_motor_velocity();
	_update_slider_motor_limit();
}

// --- Cone Twist Joint ---

JPH::Constraint *JoltJoint3D::_build_swing_twist(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_swing_limit_span, float p_twist_limit_span) const {
	JPH::SwingTwistConstraintSettings constraint_settings;

	const bool twist_span_valid = p_twist_limit_span >= 0 && p_twist_limit_span <= JPH::JPH_PI;
	const bool swing_span_valid = p_swing_limit_span >= 0 && p_swing_limit_span <= JPH::JPH_PI;

	if (cone_twist.twist_limit_enabled && twist_span_valid) {
		constraint_settings.mTwistMinAngle = -p_twist_limit_span;
		constraint_settings.mTwistMaxAngle = p_twist_limit_span;
	} else {
		constraint_settings.mTwistMinAngle = -JPH::JPH_PI;
		constraint_settings.mTwistMaxAngle = JPH::JPH_PI;
	}

	if (cone_twist.swing_limit_enabled && swing_span_valid) {
		constraint_settings.mNormalHalfConeAngle = p_swing_limit_span;
		constraint_settings.mPlaneHalfConeAngle = p_swing_limit_span;
	} else {
		constraint_settings.mNormalHalfConeAngle = JPH::JPH_PI;
		constraint_settings.mPlaneHalfConeAngle = JPH::JPH_PI;

		if (!swing_span_valid) {
			constraint_settings.mTwistMinAngle = -JPH::JPH_PI;
			constraint_settings.mTwistMaxAngle = JPH::JPH_PI;
		}
	}

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPosition1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mTwistAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPlaneAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mPosition2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mTwistAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPlaneAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_swing_motor_state() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetSwingMotorState(cone_twist.swing_motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_twist_motor_state() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetTwistMotorState(cone_twist.twist_motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_cone_twist_motor_velocity() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocityCS({ (float)-cone_twist.twist_motor_target_speed,
				(float)-cone_twist.swing_motor_target_speed_y,
				(float)-cone_twist.swing_motor_target_speed_z });
	}
}

void JoltJoint3D::_update_swing_motor_limit() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetSwingMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-cone_twist.swing_motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)cone_twist.swing_motor_max_torque;
	}
}

void JoltJoint3D::_update_twist_motor_limit() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetTwistMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-cone_twist.twist_motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)cone_twist.twist_motor_max_torque;
	}
}

void JoltJoint3D::_cone_twist_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_swing_motor_state_changed() {
	_update_swing_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_twist_motor_state_changed() {
	_update_twist_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_cone_twist_motor_velocity_changed() {
	_update_cone_twist_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_swing_motor_limit_changed() {
	_update_swing_motor_limit();
	_wake_up_bodies();
}

void JoltJoint3D::_twist_motor_limit_changed() {
	_update_twist_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_cone_twist_param(PS3DE::ConeTwistJointParam p_param) const {
	switch (p_param) {
		case PS3DE::CONE_TWIST_JOINT_SWING_SPAN: {
			return cone_twist.swing_limit_span;
		}
		case PS3DE::CONE_TWIST_JOINT_TWIST_SPAN: {
			return cone_twist.twist_limit_span;
		}
		case PS3DE::CONE_TWIST_JOINT_BIAS: {
			return CONE_TWIST_DEFAULT_BIAS;
		}
		case PS3DE::CONE_TWIST_JOINT_SOFTNESS: {
			return CONE_TWIST_DEFAULT_SOFTNESS;
		}
		case PS3DE::CONE_TWIST_JOINT_RELAXATION: {
			return CONE_TWIST_DEFAULT_RELAXATION;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled cone twist joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_cone_twist_param(PS3DE::ConeTwistJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::CONE_TWIST_JOINT_SWING_SPAN: {
			cone_twist.swing_limit_span = p_value;
			_cone_twist_limits_changed();
		} break;
		case PS3DE::CONE_TWIST_JOINT_TWIST_SPAN: {
			cone_twist.twist_limit_span = p_value;
			_cone_twist_limits_changed();
		} break;
		case PS3DE::CONE_TWIST_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Cone twist joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::CONE_TWIST_JOINT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_SOFTNESS)) {
				WARN_PRINT(vformat("Cone twist joint softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::CONE_TWIST_JOINT_RELAXATION: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_RELAXATION)) {
				WARN_PRINT(vformat("Cone twist joint relaxation is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled cone twist joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Y: {
			return cone_twist.swing_motor_target_speed_y;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Z: {
			return cone_twist.swing_motor_target_speed_z;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_TARGET_VELOCITY: {
			return cone_twist.twist_motor_target_speed;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_MAX_TORQUE: {
			return cone_twist.swing_motor_max_torque;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_MAX_TORQUE: {
			return cone_twist.twist_motor_max_torque;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Y: {
			cone_twist.swing_motor_target_speed_y = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Z: {
			cone_twist.swing_motor_target_speed_z = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_TARGET_VELOCITY: {
			cone_twist.twist_motor_target_speed = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_MAX_TORQUE: {
			cone_twist.swing_motor_max_torque = p_value;
			_swing_motor_limit_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_MAX_TORQUE: {
			cone_twist.twist_motor_max_torque = p_value;
			_twist_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_SWING_LIMIT: {
			return cone_twist.swing_limit_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_TWIST_LIMIT: {
			return cone_twist.twist_limit_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_SWING_MOTOR: {
			return cone_twist.swing_motor_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_TWIST_MOTOR: {
			return cone_twist.twist_motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_SWING_LIMIT: {
			cone_twist.swing_limit_enabled = p_enabled;
			_cone_twist_limits_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_TWIST_LIMIT: {
			cone_twist.twist_limit_enabled = p_enabled;
			_cone_twist_limits_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_SWING_MOTOR: {
			cone_twist.swing_motor_enabled = p_enabled;
			_swing_motor_state_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_TWIST_MOTOR: {
			cone_twist.twist_motor_enabled = p_enabled;
			_twist_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_cone_twist() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_swing_twist(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, (float)cone_twist.swing_limit_span, (float)cone_twist.twist_limit_span);

	space->add_joint(this);

	_update_joint();

	_update_swing_motor_state();
	_update_twist_motor_state();
	_update_cone_twist_motor_velocity();
	_update_swing_motor_limit();
	_update_twist_motor_limit();
}

// --- Generic 6DOF Joint ---

JPH::Constraint *JoltJoint3D::_build_6dof(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) const {
	JPH::SixDOFConstraintSettings constraint_settings;

	for (int axis = 0; axis < AXIS_COUNT; ++axis) {
		double lower = g6dof.limit_lower[axis];
		double upper = g6dof.limit_upper[axis];

		if (axis >= AXIS_ANGULAR_X && axis <= AXIS_ANGULAR_Z) {
			const double temp = lower;
			lower = -upper;
			upper = -temp;
		}

		if (!g6dof.limit_enabled[axis] || lower > upper) {
			constraint_settings.MakeFreeAxis((JoltAxis)axis);
		} else {
			constraint_settings.SetLimitedAxis((JoltAxis)axis, (float)lower, (float)upper);
		}
	}

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPosition1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mAxisX1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mPosition2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mAxisX2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mSwingType = JPH::ESwingType::Pyramid;

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_6dof_limit_spring_parameters(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::SpringSettings settings = constraint->GetLimitsSpringSettings((JoltAxis)p_axis);

	settings.mMode = JPH::ESpringMode::FrequencyAndDamping;

	if (g6dof.limit_spring_enabled[p_axis]) {
		settings.mFrequency = (float)g6dof.limit_spring_frequency[p_axis];
		settings.mDamping = (float)g6dof.limit_spring_damping[p_axis];
	} else {
		settings.mFrequency = 0.0f;
		settings.mDamping = 0.0f;
	}

	constraint->SetLimitsSpringSettings((JoltAxis)p_axis, settings);
}

void JoltJoint3D::_update_6dof_motor_state(int p_axis) {
	if (JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr())) {
		if (g6dof.motor_enabled[p_axis]) {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Velocity);
		} else if (g6dof.spring_enabled[p_axis]) {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Position);
		} else {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Off);
		}
	}
}

void JoltJoint3D::_update_6dof_motor_velocity(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		constraint->SetTargetVelocityCS(JPH::Vec3(
				(float)g6dof.motor_speed[AXIS_LINEAR_X],
				(float)g6dof.motor_speed[AXIS_LINEAR_Y],
				(float)g6dof.motor_speed[AXIS_LINEAR_Z]));
	} else {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocityCS(JPH::Vec3(
				(float)-g6dof.motor_speed[AXIS_ANGULAR_X],
				(float)-g6dof.motor_speed[AXIS_ANGULAR_Y],
				(float)-g6dof.motor_speed[AXIS_ANGULAR_Z]));
	}
}

void JoltJoint3D::_update_6dof_motor_limit(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::MotorSettings &motor_settings = constraint->GetMotorSettings((JoltAxis)p_axis);

	float limit = FLT_MAX;

	if (g6dof.drive_limit_set[p_axis]) {
		limit = (float)g6dof.drive_limit[p_axis];
	} else if (g6dof.motor_enabled[p_axis]) {
		limit = (float)g6dof.motor_limit[p_axis];
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		motor_settings.SetForceLimit(limit);
	} else {
		motor_settings.SetTorqueLimit(limit);
	}
}

void JoltJoint3D::_update_6dof_spring_parameters(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::MotorSettings &motor_settings = constraint->GetMotorSettings((JoltAxis)p_axis);

	if (g6dof.spring_use_frequency[p_axis]) {
		motor_settings.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		motor_settings.mSpringSettings.mFrequency = (float)g6dof.spring_frequency[p_axis];
	} else {
		motor_settings.mSpringSettings.mMode = JPH::ESpringMode::StiffnessAndDamping;
		motor_settings.mSpringSettings.mStiffness = (float)g6dof.spring_stiffness[p_axis];
	}

	motor_settings.mSpringSettings.mDamping = (float)g6dof.spring_damping[p_axis];
}

void JoltJoint3D::_update_6dof_spring_equilibrium(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		const Vector3 target_position = Vector3(
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_X],
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_Y],
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_Z]);

		constraint->SetTargetPositionCS(to_jolt(target_position));
	} else {
		// We flip the direction since Jolt is CCW but Godot is CW.
		const Basis target_orientation = Basis::from_euler(
				Vector3((float)-g6dof.spring_equilibrium[AXIS_ANGULAR_X],
						(float)-g6dof.spring_equilibrium[AXIS_ANGULAR_Y],
						(float)-g6dof.spring_equilibrium[AXIS_ANGULAR_Z]),
				EulerOrder::ZYX);

		constraint->SetTargetOrientationCS(to_jolt(target_orientation));
	}
}

void JoltJoint3D::_update_6dof_angular_target_rotation() {
	if (!g6dof.has_angular_target_rotation) {
		return;
	}

	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	const JPH::Quat target_orientation = to_jolt(g6dof.angular_target_rotation);
	constraint->SetTargetOrientationBS(target_orientation);
}

void JoltJoint3D::_6dof_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_limit_spring_parameters_changed(int p_axis) {
	_update_6dof_limit_spring_parameters(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_state_changed(int p_axis) {
	_update_6dof_motor_state(p_axis);
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_speed_changed(int p_axis) {
	_update_6dof_motor_velocity(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_limit_changed(int p_axis) {
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_state_changed(int p_axis) {
	_update_6dof_motor_state(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_parameters_changed(int p_axis) {
	_update_6dof_spring_parameters(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_equilibrium_changed(int p_axis) {
	_update_6dof_spring_equilibrium(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_drive_limit_changed(int p_axis) {
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

double JoltJoint3D::get_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case PS3DE::G6DOF_JOINT_LINEAR_LOWER_LIMIT: {
			return g6dof.limit_lower[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_UPPER_LIMIT: {
			return g6dof.limit_upper[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS: {
			return G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION: {
			return G6DOF_DEFAULT_LINEAR_RESTITUTION;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_DAMPING: {
			return G6DOF_DEFAULT_LINEAR_DAMPING;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_TARGET_VELOCITY: {
			return g6dof.motor_speed[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_FORCE_LIMIT: {
			return g6dof.motor_limit[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_STIFFNESS: {
			return g6dof.spring_stiffness[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_DAMPING: {
			return g6dof.spring_damping[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT: {
			return g6dof.spring_equilibrium[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT: {
			return g6dof.drive_limit[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_LOWER_LIMIT: {
			return g6dof.limit_lower[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_UPPER_LIMIT: {
			return g6dof.limit_upper[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			return G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_DAMPING: {
			return G6DOF_DEFAULT_ANGULAR_DAMPING;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_RESTITUTION: {
			return G6DOF_DEFAULT_ANGULAR_RESTITUTION;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_FORCE_LIMIT: {
			return G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_ERP: {
			return G6DOF_DEFAULT_ANGULAR_ERP;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY: {
			return g6dof.motor_speed[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT: {
			return g6dof.motor_limit[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS: {
			return g6dof.spring_stiffness[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_DAMPING: {
			return g6dof.spring_damping[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT: {
			return g6dof.spring_equilibrium[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT: {
			return g6dof.drive_limit[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param, double p_value) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case PS3DE::G6DOF_JOINT_LINEAR_LOWER_LIMIT: {
			g6dof.limit_lower[axis_lin] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_UPPER_LIMIT: {
			g6dof.limit_upper[axis_lin] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("6DOF joint linear limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_RESTITUTION)) {
				WARN_PRINT(vformat("6DOF joint linear restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_DAMPING: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_DAMPING)) {
				WARN_PRINT(vformat("6DOF joint linear damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_TARGET_VELOCITY: {
			g6dof.motor_speed[axis_lin] = p_value;
			_6dof_motor_speed_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_FORCE_LIMIT: {
			g6dof.motor_limit[axis_lin] = p_value;
			_6dof_motor_limit_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_STIFFNESS: {
			g6dof.spring_stiffness[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_DAMPING: {
			g6dof.spring_damping[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT: {
			g6dof.spring_equilibrium[axis_lin] = p_value;
			_6dof_spring_equilibrium_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT: {
			g6dof.drive_limit[axis_lin] = p_value;
			g6dof.drive_limit_set[axis_lin] = true;
			_6dof_drive_limit_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_LOWER_LIMIT: {
			g6dof.limit_lower[axis_ang] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_UPPER_LIMIT: {
			g6dof.limit_upper[axis_ang] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("6DOF joint angular limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_DAMPING: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_DAMPING)) {
				WARN_PRINT(vformat("6DOF joint angular damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_RESTITUTION)) {
				WARN_PRINT(vformat("6DOF joint angular restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_FORCE_LIMIT: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT)) {
				WARN_PRINT(vformat("6DOF joint angular force limit is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_ERP: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_ERP)) {
				WARN_PRINT(vformat("6DOF joint angular ERP is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY: {
			g6dof.motor_speed[axis_ang] = p_value;
			_6dof_motor_speed_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT: {
			g6dof.motor_limit[axis_ang] = p_value;
			_6dof_motor_limit_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS: {
			g6dof.spring_stiffness[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_DAMPING: {
			g6dof.spring_damping[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT: {
			g6dof.spring_equilibrium[axis_ang] = p_value;
			g6dof.has_angular_target_rotation = false;
			_6dof_spring_equilibrium_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT: {
			g6dof.drive_limit[axis_ang] = p_value;
			g6dof.drive_limit_set[axis_ang] = true;
			_6dof_drive_limit_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT: {
			return g6dof.limit_enabled[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT: {
			return g6dof.limit_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING: {
			return g6dof.spring_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING: {
			return g6dof.spring_enabled[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR: {
			return g6dof.motor_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_MOTOR: {
			return g6dof.motor_enabled[axis_lin];
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enabled) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT: {
			g6dof.limit_enabled[axis_lin] = p_enabled;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT: {
			g6dof.limit_enabled[axis_ang] = p_enabled;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING: {
			g6dof.spring_enabled[axis_ang] = p_enabled;
			_6dof_spring_state_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING: {
			g6dof.spring_enabled[axis_lin] = p_enabled;
			_6dof_spring_state_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR: {
			g6dof.motor_enabled[axis_ang] = p_enabled;
			_6dof_motor_state_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_MOTOR: {
			g6dof.motor_enabled[axis_lin] = p_enabled;
			_6dof_motor_state_changed(axis_lin);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::set_6dof_angular_target_rotation(const Quaternion &p_target_rotation) {
	ERR_FAIL_COND_MSG(!p_target_rotation.is_finite() || p_target_rotation.length_squared() <= CMP_EPSILON, "Angular target rotation must be a finite, non-zero quaternion.");

	g6dof.angular_target_rotation = p_target_rotation.normalized();
	g6dof.has_angular_target_rotation = true;
	_update_6dof_angular_target_rotation();
	_wake_up_bodies();
}

Quaternion JoltJoint3D::get_6dof_angular_target_rotation() const {
	if (g6dof.has_angular_target_rotation) {
		return g6dof.angular_target_rotation;
	}

	const JPH::SixDOFConstraint *constraint = static_cast<const JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return Quaternion();
	}

	const Basis target_orientation_cs = Basis::from_euler(
			Vector3(static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_X]),
					static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_Y]),
					static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_Z])),
			EulerOrder::ZYX);
	const Basis constraint_to_body_1 = to_godot(constraint->GetConstraintToBody1Matrix()).basis;
	const Basis constraint_to_body_2 = to_godot(constraint->GetConstraintToBody2Matrix()).basis;

	return (constraint_to_body_1 * target_orientation_cs * constraint_to_body_2.inverse()).get_quaternion();
}

double JoltJoint3D::get_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_SPRING_FREQUENCY: {
			return g6dof.spring_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_FREQUENCY: {
			return g6dof.limit_spring_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_DAMPING: {
			return g6dof.limit_spring_damping[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_ANGULAR_SPRING_FREQUENCY: {
			return g6dof.spring_frequency[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param, double p_value) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_SPRING_FREQUENCY: {
			g6dof.spring_frequency[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_FREQUENCY: {
			g6dof.limit_spring_frequency[axis_lin] = p_value;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_DAMPING: {
			g6dof.limit_spring_damping[axis_lin] = p_value;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_ANGULAR_SPRING_FREQUENCY: {
			g6dof.spring_frequency[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT_SPRING: {
			return g6dof.limit_spring_enabled[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING_FREQUENCY: {
			return g6dof.spring_use_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING_FREQUENCY: {
			return g6dof.spring_use_frequency[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag, bool p_enabled) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT_SPRING: {
			g6dof.limit_spring_enabled[axis_lin] = p_enabled;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING_FREQUENCY: {
			g6dof.spring_use_frequency[axis_lin] = p_enabled;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING_FREQUENCY: {
			g6dof.spring_use_frequency[axis_ang] = p_enabled;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_6dof() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_6dof(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);

	space->add_joint(this);

	_update_joint();

	_update_6dof_limit_spring_parameters(AXIS_LINEAR_X);
	_update_6dof_limit_spring_parameters(AXIS_LINEAR_Y);
	_update_6dof_limit_spring_parameters(AXIS_LINEAR_Z);

	for (int axis = 0; axis < AXIS_COUNT; ++axis) {
		_update_6dof_motor_state(axis);
		_update_6dof_motor_velocity(axis);
		_update_6dof_motor_limit(axis);
		_update_6dof_spring_parameters(axis);
		_update_6dof_spring_equilibrium(axis);
	}

	_update_6dof_angular_target_rotation();
}

// ============================================================================
// Merged from modules/jolt_physics/spaces/jolt_space_3d.cpp
// ============================================================================


#include "modules/jolt_physics/jolt_physics_server_3d.h"


#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "servers/physics_3d/physics_server_3d.h"

// must stay last
#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Geometry/GJKClosestPoint.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/CollideShapeVsShapePerLeaf.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/EstimateCollisionResponse.h>
#include <Jolt/Physics/Collision/ManifoldBetweenTwoFaces.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyManifold.h>

// ============================================================================
// JoltTempAllocator
// ============================================================================

namespace {

template <typename TValue, typename TAlignment>
constexpr TValue align_up(TValue p_value, TAlignment p_alignment) {
	return (p_value + p_alignment - 1) & ~(p_alignment - 1);
}

} // namespace

JoltTempAllocator::JoltTempAllocator() {}

JoltTempAllocator::~JoltTempAllocator() {
	JPH::Free(base);
}

void *JoltTempAllocator::Allocate(uint32_t p_size) {
	if (p_size == 0) {
		return nullptr;
	}

	p_size = align_up(p_size, 16U);

	const uint64_t new_top = top + p_size;

	void *ptr = nullptr;

	if (new_top <= capacity) {
		ptr = base + top;
	} else {
		ptr = JPH::Allocate(p_size);
	}

	top = new_top;

	return ptr;
}

void JoltTempAllocator::Free(void *p_ptr, uint32_t p_size) {
	if (p_ptr == nullptr) {
		return;
	}

	p_size = align_up(p_size, 16U);

	const uint64_t new_top = top - p_size;

	if (top <= capacity) {
		if (base + new_top != p_ptr) {
			// CRASH_NOW_MSG("Jolt Physics temporary memory was freed in the wrong order.");
		}
	} else {
		JPH::Free(p_ptr);
	}

	top = new_top;
}

// ============================================================================
// JoltJobSystem
// ============================================================================

void JoltJobSystem::Job::_execute(void *p_user_data) {
	Job *job = static_cast<Job *>(p_user_data);

#ifdef DEBUG_ENABLED
	const uint64_t time_start = Time::get_ticks_usec();
#endif

	job->Execute();

#ifdef DEBUG_ENABLED
	const uint64_t time_end = Time::get_ticks_usec();
	const uint64_t time_elapsed = time_end - time_start;

	timings_lock.lock();
	timings_by_job[job->name] += time_elapsed;
	timings_lock.unlock();
#endif

	job->Release();
}

JoltJobSystem::Job::Job(const char *p_name, JPH::ColorArg p_color, JPH::JobSystem *p_job_system, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count) :
		JPH::JobSystem::Job(p_name, p_color, p_job_system, p_job_function, p_dependency_count)
#ifdef DEBUG_ENABLED
		,
		name(p_name)
#endif
{
}

void JoltJobSystem::Job::push_completed(Job *p_job) {
	Job *prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		p_job->completed_next = prev_head;
	} while (!completed_head.compare_exchange_weak(prev_head, p_job, std::memory_order_release, std::memory_order_relaxed));
}

JoltJobSystem::Job *JoltJobSystem::Job::pop_completed() {
	Job *prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		if (prev_head == nullptr) {
			return nullptr;
		}
	} while (!completed_head.compare_exchange_weak(prev_head, prev_head->completed_next, std::memory_order_acquire, std::memory_order_relaxed));

	return prev_head;
}

int JoltJobSystem::GetMaxConcurrency() const { return thread_count; }

JPH::JobHandle JoltJobSystem::CreateJob(const char *p_name, JPH::ColorArg p_color, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count) {
	Job *job = nullptr;

	while (true) {
		JPH::uint32 job_index = jobs.ConstructObject(p_name, p_color, this, p_job_function, p_dependency_count);

		if (job_index != JPH::FixedSizeFreeList<Job>::cInvalidObjectIndex) {
			job = &jobs.Get(job_index);
			break;
		}

		WARN_PRINT_ONCE("Jolt Physics job system exceeded the maximum number of jobs. This should "
						"not happen. Please report this. Waiting for jobs to become available...");

		OS::delay_usec(100);

		_reclaim_jobs();
	}

	JPH::JobHandle job_handle(job);

	if (p_dependency_count == 0) {
		QueueJob(job);
	}

	return job_handle;
}

void JoltJobSystem::QueueJob(JPH::JobSystem::Job *p_job) { static_cast<Job *>(p_job)->queue(); }

void JoltJobSystem::QueueJobs(JPH::JobSystem::Job **p_jobs, JPH::uint p_job_count) {
	for (JPH::uint i = 0; i < p_job_count; ++i) {
		QueueJob(p_jobs[i]);
	}
}

void JoltJobSystem::FreeJob(JPH::JobSystem::Job *p_job) {
	Job::push_completed(static_cast<Job *>(p_job));
}

void JoltJobSystem::_reclaim_jobs() {
	while (Job *job = Job::pop_completed()) {
		jobs.DestructObject(job);
	}
}

void JoltJobSystem::post_step() { _reclaim_jobs(); }
void JoltJobSystem::Job::queue() {}
void JoltJobSystem::pre_step() {}
JoltJobSystem::JoltJobSystem() {}
JoltJobSystem::Job::~Job() {}
#ifdef DEBUG_ENABLED
void JoltJobSystem::flush_timings() {}
#endif

// ============================================================================
// JoltLayers
// ============================================================================

static_assert(sizeof(JPH::ObjectLayer) == 2, "Size of Jolt's object layer has changed.");
static_assert(sizeof(JPH::BroadPhaseLayer::Type) == 1, "Size of Jolt's broadphase layer has changed.");
static_assert(JoltBroadPhaseLayer::COUNT <= 8, "Maximum number of broadphase layers exceeded.");

namespace {

template <uint8_t TSize = JoltBroadPhaseLayer::COUNT>
class JoltBroadPhaseMatrix {
	typedef JPH::BroadPhaseLayer LayerType;
	typedef LayerType::Type UnderlyingType;

public:
	JoltBroadPhaseMatrix() {
		using namespace JoltBroadPhaseLayer;

		allow_collision(BODY_STATIC, BODY_DYNAMIC);
		allow_collision(BODY_STATIC, AREA_DETECTABLE);
		allow_collision(BODY_STATIC, AREA_UNDETECTABLE);
		allow_collision(BODY_STATIC_BIG, BODY_DYNAMIC);
		allow_collision(BODY_STATIC_BIG, AREA_DETECTABLE);
		allow_collision(BODY_STATIC_BIG, AREA_UNDETECTABLE);
		allow_collision(BODY_DYNAMIC, BODY_STATIC);
		allow_collision(BODY_DYNAMIC, BODY_STATIC_BIG);
		allow_collision(BODY_DYNAMIC, BODY_DYNAMIC);
		allow_collision(BODY_DYNAMIC, AREA_DETECTABLE);
		allow_collision(BODY_DYNAMIC, AREA_UNDETECTABLE);
		allow_collision(AREA_DETECTABLE, BODY_DYNAMIC);
		allow_collision(AREA_DETECTABLE, BODY_STATIC);
		allow_collision(AREA_DETECTABLE, BODY_STATIC_BIG);
		allow_collision(AREA_DETECTABLE, AREA_DETECTABLE);
		allow_collision(AREA_DETECTABLE, AREA_UNDETECTABLE);
		allow_collision(AREA_UNDETECTABLE, BODY_DYNAMIC);
		allow_collision(AREA_UNDETECTABLE, BODY_STATIC);
		allow_collision(AREA_UNDETECTABLE, BODY_STATIC_BIG);
		allow_collision(AREA_UNDETECTABLE, AREA_DETECTABLE);
	}

	void allow_collision(UnderlyingType p_layer1, UnderlyingType p_layer2) {
		masks[p_layer1] |= uint8_t(1U << p_layer2);
	}

	void allow_collision(LayerType p_layer1, LayerType p_layer2) {
		allow_collision((UnderlyingType)p_layer1, (UnderlyingType)p_layer2);
	}

	bool should_collide(UnderlyingType p_layer1, UnderlyingType p_layer2) const {
		return (masks[p_layer1] & uint8_t(1U << p_layer2)) != 0;
	}

	bool should_collide(LayerType p_layer1, LayerType p_layer2) const {
		return should_collide((UnderlyingType)p_layer1, (UnderlyingType)p_layer2);
	}

private:
	uint8_t masks[TSize] = {};
};

constexpr JPH::ObjectLayer encode_layers(JPH::BroadPhaseLayer p_broad_phase_layer, JPH::ObjectLayer p_object_layer) {
	const uint16_t upper_bits = uint16_t((uint8_t)p_broad_phase_layer << 13U);
	const uint16_t lower_bits = uint16_t(p_object_layer);
	return JPH::ObjectLayer(upper_bits | lower_bits);
}

constexpr void decode_layers(JPH::ObjectLayer p_encoded_layers, JPH::BroadPhaseLayer &r_broad_phase_layer, JPH::ObjectLayer &r_object_layer) {
	r_broad_phase_layer = JPH::BroadPhaseLayer(uint8_t(p_encoded_layers >> 13U));
	r_object_layer = JPH::ObjectLayer(p_encoded_layers & 0b0001'1111'1111'1111U);
}

constexpr uint64_t encode_collision(uint32_t p_collision_layer, uint32_t p_collision_mask) {
	const uint64_t upper_bits = (uint64_t)p_collision_layer << 32U;
	const uint64_t lower_bits = (uint64_t)p_collision_mask;
	return upper_bits | lower_bits;
}

constexpr void decode_collision(uint64_t p_collision, uint32_t &r_collision_layer, uint32_t &r_collision_mask) {
	r_collision_layer = uint32_t(p_collision >> 32U);
	r_collision_mask = uint32_t(p_collision & 0xFFFFFFFFU);
}

} // namespace

uint32_t JoltLayers::GetNumBroadPhaseLayers() const { return JoltBroadPhaseLayer::COUNT; }

JPH::BroadPhaseLayer JoltLayers::GetBroadPhaseLayer(JPH::ObjectLayer p_layer) const {
	JPH::BroadPhaseLayer broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	JPH::ObjectLayer object_layer = 0;
	decode_layers(p_layer, broad_phase_layer, object_layer);

	return broad_phase_layer;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)

const char *JoltLayers::GetBroadPhaseLayerName(JPH::BroadPhaseLayer p_layer) const {
	switch ((JPH::BroadPhaseLayer::Type)p_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC: {
			return "BODY_STATIC";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG: {
			return "BODY_STATIC_BIG";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return "BODY_DYNAMIC";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE: {
			return "AREA_DETECTABLE";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return "AREA_UNDETECTABLE";
		}
		default: {
			return "UNKNOWN";
		}
	}
}

#endif

bool JoltLayers::ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::ObjectLayer p_encoded_layer2) const {
	JPH::BroadPhaseLayer broad_phase_layer1 = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t collision_layer1 = 0;
	uint32_t collision_mask1 = 0;
	from_object_layer(p_encoded_layer1, broad_phase_layer1, collision_layer1, collision_mask1);

	JPH::BroadPhaseLayer broad_phase_layer2 = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t collision_layer2 = 0;
	uint32_t collision_mask2 = 0;
	from_object_layer(p_encoded_layer2, broad_phase_layer2, collision_layer2, collision_mask2);

	const bool first_scans_second = (collision_mask1 & collision_layer2) != 0;
	const bool second_scans_first = (collision_mask2 & collision_layer1) != 0;

	return first_scans_second || second_scans_first;
}

bool JoltLayers::ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::BroadPhaseLayer p_broad_phase_layer2) const {
	static const JoltBroadPhaseMatrix matrix;

	JPH::BroadPhaseLayer broad_phase_layer1 = JoltBroadPhaseLayer::BODY_STATIC;
	JPH::ObjectLayer object_layer1 = 0;
	decode_layers(p_encoded_layer1, broad_phase_layer1, object_layer1);

	return matrix.should_collide(broad_phase_layer1, p_broad_phase_layer2);
}

JPH::ObjectLayer JoltLayers::_allocate_object_layer(uint64_t p_collision) {
	const JPH::ObjectLayer new_object_layer = next_object_layer++;

	collisions_by_layer.resize(new_object_layer + 1);
	collisions_by_layer[new_object_layer] = p_collision;

	layers_by_collision[p_collision] = new_object_layer;

	return new_object_layer;
}

JoltLayers::JoltLayers() { _allocate_object_layer(0); }
JoltLayers::~JoltLayers() = default;

JPH::ObjectLayer JoltLayers::to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask) {
	const uint64_t collision = encode_collision(p_collision_layer, p_collision_mask);

	JPH::ObjectLayer object_layer = 0;

	HashMap<uint64_t, JPH::ObjectLayer>::Iterator iter = layers_by_collision.find(collision);
	if (iter != layers_by_collision.end()) {
		object_layer = iter->value;
	} else {
		object_layer = _allocate_object_layer(collision);
	}

	return encode_layers(p_broad_phase_layer, object_layer);
}

void JoltLayers::from_object_layer(JPH::ObjectLayer p_encoded_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const {
	JPH::ObjectLayer object_layer = 0;
	decode_layers(p_encoded_layer, r_broad_phase_layer, object_layer);

	const uint64_t collision = collisions_by_layer[object_layer];
	decode_collision(collision, r_collision_layer, r_collision_mask);
}

// ============================================================================
// JoltBodyActivationListener3D
// ============================================================================

void JoltBodyActivationListener3D::OnBodyActivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) {
	if (JoltBody3D *body = reinterpret_cast<JoltObject3D *>(p_body_user_data)->as_body()) {
		body->_on_wake_up();
	}
}

// ============================================================================
// JoltContactListener3D
// ============================================================================

void JoltContactListener3D::OnContactAdded(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	_try_override_collision_response(p_body1, p_body2, p_settings);
	_try_apply_surface_velocities(p_body1, p_body2, p_settings);
	_try_add_contacts(p_body1, p_body2, p_manifold, p_settings);
	_try_evaluate_area_overlap(p_body1, p_body2, p_manifold.mSubShapeID1, p_manifold.mSubShapeID2);

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_body1, p_body2, p_manifold);
#endif
}

void JoltContactListener3D::OnContactPersisted(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	_try_override_collision_response(p_body1, p_body2, p_settings);
	_try_apply_surface_velocities(p_body1, p_body2, p_settings);
	_try_add_contacts(p_body1, p_body2, p_manifold, p_settings);
	_try_evaluate_area_overlap(p_body1, p_body2, p_manifold.mSubShapeID1, p_manifold.mSubShapeID2);

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_body1, p_body2, p_manifold);
#endif
}

void JoltContactListener3D::OnContactRemoved(const JPH::SubShapeIDPair &p_shape_pair) {
	if (_try_remove_contacts(p_shape_pair)) {
		return;
	}

	if (_try_remove_area_overlap(p_shape_pair)) {
		return;
	}
}

JPH::SoftBodyValidateResult JoltContactListener3D::OnSoftBodyContactValidate(const JPH::Body &p_soft_body, const JPH::Body &p_other_body, JPH::SoftBodyContactSettings &p_settings) {
	_try_override_collision_response(p_soft_body, p_other_body, p_settings);
	return JPH::SoftBodyValidateResult::AcceptContact;
}

void JoltContactListener3D::OnSoftBodyContactAdded(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) {
	for (JPH::uint i = 0; i < p_manifold.GetNumSensorContacts(); i++) {
		if (JPH::Body *other_jolt_body = space->try_get_jolt_body(p_manifold.GetSensorContactBodyID(i))) {
			_try_evaluate_area_overlap(p_soft_body, *other_jolt_body, JPH::SubShapeID(), JPH::SubShapeID());
		}
	}

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_soft_body, p_manifold);
#endif
}

bool JoltContactListener3D::_try_override_collision_response(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	if (!p_jolt_body1.IsDynamic() && !p_jolt_body2.IsDynamic()) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	const bool can_collide1 = body1->can_collide_with(*body2);
	const bool can_collide2 = body2->can_collide_with(*body1);

	if (can_collide1 && !can_collide2) {
		p_settings.mInvMassScale2 = 0.0f;
		p_settings.mInvInertiaScale2 = 0.0f;
	} else if (can_collide2 && !can_collide1) {
		p_settings.mInvMassScale1 = 0.0f;
		p_settings.mInvInertiaScale1 = 0.0f;
	}

	return true;
}

bool JoltContactListener3D::_try_override_collision_response(const JPH::Body &p_jolt_soft_body, const JPH::Body &p_jolt_other_body, JPH::SoftBodyContactSettings &p_settings) {
	if (p_jolt_other_body.IsSensor()) {
		return false;
	}

	const JoltSoftBody3D *soft_body = reinterpret_cast<JoltSoftBody3D *>(p_jolt_soft_body.GetUserData());
	const JoltBody3D *other_body = reinterpret_cast<JoltBody3D *>(p_jolt_other_body.GetUserData());

	const bool can_collide1 = soft_body->can_collide_with(*other_body);
	const bool can_collide2 = other_body->can_collide_with(*soft_body);

	if (can_collide1 && !can_collide2) {
		p_settings.mInvMassScale2 = 0.0f;
		p_settings.mInvInertiaScale2 = 0.0f;
	} else if (can_collide2 && !can_collide1) {
		p_settings.mInvMassScale1 = 0.0f;
	}

	return true;
}

bool JoltContactListener3D::_try_apply_surface_velocities(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	const bool supports_surface_velocity1 = !p_jolt_body1.IsDynamic();
	const bool supports_surface_velocity2 = !p_jolt_body2.IsDynamic();

	if (supports_surface_velocity1 == supports_surface_velocity2) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	const bool has_surface_velocity1 = supports_surface_velocity1 && (body1->get_linear_surface_velocity() != Vector3() || body1->get_angular_surface_velocity() != Vector3());
	const bool has_surface_velocity2 = supports_surface_velocity2 && (body2->get_linear_surface_velocity() != Vector3() || body2->get_angular_surface_velocity() != Vector3());

	if (has_surface_velocity1 == has_surface_velocity2) {
		return false;
	}

	const JPH::Vec3 linear_velocity1 = to_jolt(body1->get_linear_surface_velocity());
	const JPH::Vec3 angular_velocity1 = to_jolt(body1->get_angular_surface_velocity());

	const JPH::Vec3 linear_velocity2 = to_jolt(body2->get_linear_surface_velocity());
	const JPH::Vec3 angular_velocity2 = to_jolt(body2->get_angular_surface_velocity());

	const JPH::RVec3 com1 = p_jolt_body1.GetCenterOfMassPosition();
	const JPH::RVec3 com2 = p_jolt_body2.GetCenterOfMassPosition();
	const JPH::Vec3 rel_com2 = JPH::Vec3(com2 - com1);

	const JPH::Vec3 angular_linear_velocity2 = rel_com2.Cross(angular_velocity2);
	const JPH::Vec3 total_linear_velocity2 = linear_velocity2 + angular_linear_velocity2;

	p_settings.mRelativeLinearSurfaceVelocity = total_linear_velocity2 - linear_velocity1;
	p_settings.mRelativeAngularSurfaceVelocity = angular_velocity2 - angular_velocity1;

	return true;
}

bool JoltContactListener3D::_try_add_contacts(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	if (!body1->reports_contacts() && !body2->reports_contacts()) {
		return false;
	}

	const JPH::SubShapeIDPair shape_pair(p_jolt_body1.GetID(), p_manifold.mSubShapeID1, p_jolt_body2.GetID(), p_manifold.mSubShapeID2);

	Manifold &manifold = [&]() -> Manifold & {
		const MutexLock write_lock(write_mutex);
		return manifolds_by_shape_pair[shape_pair];
	}();

	if (unlikely(!manifold.contacts1.is_empty())) {
		return false;
	}

	const JPH::uint contact_count = p_manifold.mRelativeContactPointsOn1.size();

	manifold.contacts1.reserve((uint32_t)contact_count);
	manifold.contacts2.reserve((uint32_t)contact_count);
	manifold.depth = p_manifold.mPenetrationDepth;

	JPH::CollisionEstimationResult collision;
	const JPH::Vec3 friction_impulse1 = contact_count > 0 ? (collision.mTangent1 * collision.mFrictionImpulse1) / contact_count : JPH::Vec3::sZero();
	const JPH::Vec3 friction_impulse2 = contact_count > 0 ? (collision.mTangent2 * collision.mFrictionImpulse2) / contact_count : JPH::Vec3::sZero();

	for (JPH::uint i = 0; i < contact_count; ++i) {
		const JPH::RVec3 relative_point1 = JPH::RVec3(p_manifold.mRelativeContactPointsOn1[i]);
		const JPH::RVec3 relative_point2 = JPH::RVec3(p_manifold.mRelativeContactPointsOn2[i]);

		const JPH::RVec3 world_point1 = p_manifold.mBaseOffset + relative_point1;
		const JPH::RVec3 world_point2 = p_manifold.mBaseOffset + relative_point2;

		const JPH::Vec3 velocity1 = p_jolt_body1.GetPointVelocity(world_point1);
		const JPH::Vec3 velocity2 = p_jolt_body2.GetPointVelocity(world_point2);

		const JPH::Vec3 contact_impulse = p_manifold.mWorldSpaceNormal * collision.mContactImpulse[i];
		const JPH::Vec3 combined_impulse = contact_impulse + friction_impulse1 + friction_impulse2;

		Contact contact1;
		contact1.point_self = to_godot(world_point1);
		contact1.point_other = to_godot(world_point2);
		contact1.normal = to_godot(-p_manifold.mWorldSpaceNormal);
		contact1.velocity_self = to_godot(velocity1);
		contact1.velocity_other = to_godot(velocity2);
		contact1.impulse = to_godot(-combined_impulse);
		manifold.contacts1.push_back(contact1);

		Contact contact2;
		contact2.point_self = to_godot(world_point2);
		contact2.point_other = to_godot(world_point1);
		contact2.normal = to_godot(p_manifold.mWorldSpaceNormal);
		contact2.velocity_self = to_godot(velocity2);
		contact2.velocity_other = to_godot(velocity1);
		contact2.impulse = to_godot(combined_impulse);
		manifold.contacts2.push_back(contact2);
	}

	return true;
}

bool JoltContactListener3D::_try_evaluate_area_overlap(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::SubShapeID &p_shape_id1, const JPH::SubShapeID &p_shape_id2) {
	if (!p_body1.IsSensor() && !p_body2.IsSensor()) {
		return false;
	}

	const JPH::SubShapeIDPair shape_pair1(p_body1.GetID(), p_shape_id1, p_body2.GetID(), p_shape_id2);
	const JPH::SubShapeIDPair shape_pair2(p_body2.GetID(), p_shape_id2, p_body1.GetID(), p_shape_id1);

	const JoltObject3D *object1 = reinterpret_cast<JoltObject3D *>(p_body1.GetUserData());
	const JoltObject3D *object2 = reinterpret_cast<JoltObject3D *>(p_body2.GetUserData());

	const JoltArea3D *area1 = object1->as_area();
	const JoltArea3D *area2 = object2->as_area();

	const JoltBody3D *body1 = object1->as_body();
	const JoltBody3D *body2 = object2->as_body();

	const JoltSoftBody3D *soft_body1 = object1->as_soft_body();
	const JoltSoftBody3D *soft_body2 = object2->as_soft_body();

	if (area1 != nullptr && area2 != nullptr) {
		_evaluate_area_overlap(*area1, *area2, shape_pair1);
		_evaluate_area_overlap(*area2, *area1, shape_pair2);
	} else if (area1 != nullptr && body2 != nullptr) {
		_evaluate_area_overlap(*area1, *body2, shape_pair1);
	} else if (area2 != nullptr && body1 != nullptr) {
		_evaluate_area_overlap(*area2, *body1, shape_pair2);
	} else if (area1 != nullptr && soft_body2 != nullptr) {
		_evaluate_area_overlap(*area1, *soft_body2, shape_pair1);
	} else if (area2 != nullptr && soft_body1 != nullptr) {
		_evaluate_area_overlap(*area2, *soft_body1, shape_pair2);
	}

	return true;
}

bool JoltContactListener3D::_try_remove_contacts(const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);
	return manifolds_by_shape_pair.erase(p_shape_pair);
}

bool JoltContactListener3D::_try_remove_area_overlap(const JPH::SubShapeIDPair &p_shape_pair) {
	const JPH::SubShapeIDPair swapped_shape_pair(p_shape_pair.GetBody2ID(), p_shape_pair.GetSubShapeID2(), p_shape_pair.GetBody1ID(), p_shape_pair.GetSubShapeID1());

	const MutexLock write_lock(write_mutex);

	bool removed = false;

	if (area_overlaps.erase(p_shape_pair)) {
		area_exits.insert(p_shape_pair);
		removed = true;
	}

	if (area_overlaps.erase(swapped_shape_pair)) {
		area_exits.insert(swapped_shape_pair);
		removed = true;
	}

	return removed;
}

#ifdef DEBUG_ENABLED

bool JoltContactListener3D::_try_add_debug_contacts(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold) {
	if (p_body1.IsSensor() || p_body2.IsSensor()) {
		return false;
	}

	const int64_t max_count = debug_contacts.size();

	if (max_count == 0) {
		return false;
	}

	const int additional_pairs = (int)p_manifold.mRelativeContactPointsOn1.size();
	const int additional_contacts = additional_pairs * 2;

	int current_count = debug_contact_count.load(std::memory_order_relaxed);
	bool exchanged = false;

	do {
		const int new_count = current_count + additional_contacts;

		if (new_count > max_count) {
			return false;
		}

		exchanged = debug_contact_count.compare_exchange_weak(current_count, new_count, std::memory_order_release, std::memory_order_relaxed);
	} while (!exchanged);

	for (int i = 0; i < additional_pairs; ++i) {
		const int pair_index = current_count + i * 2;

		const JPH::RVec3 point_on_1 = p_manifold.GetWorldSpaceContactPointOn1((JPH::uint)i);
		const JPH::RVec3 point_on_2 = p_manifold.GetWorldSpaceContactPointOn2((JPH::uint)i);

		debug_contacts.write[pair_index + 0] = to_godot(point_on_1);
		debug_contacts.write[pair_index + 1] = to_godot(point_on_2);
	}

	return true;
}

bool JoltContactListener3D::_try_add_debug_contacts(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) {
	const int64_t max_count = debug_contacts.size();
	if (max_count == 0) {
		return false;
	}

	int additional_contacts = 0;

	for (const JPH::SoftBodyVertex &vertex : p_manifold.GetVertices()) {
		if (p_manifold.HasContact(vertex)) {
			additional_contacts += 1;
		}
	}

	int current_count = debug_contact_count.load(std::memory_order_relaxed);
	bool exchanged = false;

	do {
		const int new_count = current_count + additional_contacts;

		if (new_count > max_count) {
			return false;
		}

		exchanged = debug_contact_count.compare_exchange_weak(current_count, new_count, std::memory_order_release, std::memory_order_relaxed);
	} while (!exchanged);

	int contact_index = current_count;

	for (const JPH::SoftBodyVertex &vertex : p_manifold.GetVertices()) {
		if (!p_manifold.HasContact(vertex)) {
			continue;
		}

		const JPH::RMat44 body_com_transform = p_soft_body.GetCenterOfMassTransform();
		const JPH::Vec3 local_contact_point = p_manifold.GetLocalContactPoint(vertex);
		const JPH::RVec3 contact_point = body_com_transform * local_contact_point;

		debug_contacts.write[contact_index++] = to_godot(contact_point);
	}

	return true;
}

#endif

bool JoltContactListener3D::_has_shape_shifted(const JoltObject3D &p_object, const JPH::SubShapeID &p_sub_shape_id) {
	return p_object.get_previous_jolt_shape() != nullptr &&
			p_object.get_jolt_shape()->GetSubShapeUserData(p_sub_shape_id) != p_object.get_previous_jolt_shape()->GetSubShapeUserData(p_sub_shape_id);
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltArea3D &p_other_area, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_other_area)) {
		if (!area_overlaps.has(p_shape_pair)) {
			area_overlaps.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		} else if (_has_shape_shifted(p_area, p_shape_pair.GetSubShapeID1()) || _has_shape_shifted(p_other_area, p_shape_pair.GetSubShapeID2())) {
			area_exits.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		}
	} else {
		if (area_overlaps.erase(p_shape_pair)) {
			area_exits.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_body)) {
		if (!area_overlaps.has(p_shape_pair)) {
			area_overlaps.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		} else if (_has_shape_shifted(p_area, p_shape_pair.GetSubShapeID1()) || _has_shape_shifted(p_body, p_shape_pair.GetSubShapeID2())) {
			area_exits.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		}
	} else {
		if (area_overlaps.erase(p_shape_pair)) {
			area_exits.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltSoftBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_body)) {
		area_soft_body_overlaps.push_back(p_shape_pair);
		if (!area_exits.erase(p_shape_pair)) {
			area_enters.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_flush_contacts() {
	for (KeyValue<JPH::SubShapeIDPair, Manifold> &E : manifolds_by_shape_pair) {
		const JPH::SubShapeIDPair &shape_pair = E.key;
		Manifold &manifold = E.value;

		JoltBody3D *body1 = space->try_get_body(shape_pair.GetBody1ID());
		ERR_FAIL_NULL(body1);

		JoltBody3D *body2 = space->try_get_body(shape_pair.GetBody2ID());
		ERR_FAIL_NULL(body2);

		const int shape_index1 = body1->find_shape_index(shape_pair.GetSubShapeID1());
		const int shape_index2 = body2->find_shape_index(shape_pair.GetSubShapeID2());

		for (const Contact &contact : manifold.contacts1) {
			body1->add_contact(body2, manifold.depth, shape_index1, shape_index2, contact.normal, contact.point_self, contact.point_other, contact.velocity_self, contact.velocity_other, contact.impulse);
		}

		for (const Contact &contact : manifold.contacts2) {
			body2->add_contact(body1, manifold.depth, shape_index2, shape_index1, contact.normal, contact.point_self, contact.point_other, contact.velocity_self, contact.velocity_other, contact.impulse);
		}

		manifold.contacts1.clear();
		manifold.contacts2.clear();
	}
}

void JoltContactListener3D::_flush_area_enters() {
	for (const JPH::SubShapeIDPair &shape_pair : area_enters) {
		const JPH::BodyID &body_id1 = shape_pair.GetBody1ID();
		const JPH::BodyID &body_id2 = shape_pair.GetBody2ID();

		JoltObject3D *object1 = space->try_get_object(body_id1);
		JoltObject3D *object2 = space->try_get_object(body_id2);

		if (object1 == nullptr || object2 == nullptr) {
			continue;
		}

		JoltArea3D *area1 = object1->as_area();
		JoltArea3D *area2 = object2->as_area();

		const JPH::SubShapeID &sub_shape_id1 = shape_pair.GetSubShapeID1();
		const JPH::SubShapeID &sub_shape_id2 = shape_pair.GetSubShapeID2();

		if (area1 != nullptr && area2 != nullptr) {
			area1->area_shape_entered(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 != nullptr && area2 == nullptr) {
			area1->body_shape_entered(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 == nullptr && area2 != nullptr) {
			area2->body_shape_entered(body_id1, sub_shape_id1, sub_shape_id2);
		}
	}

	area_enters.clear();
}

void JoltContactListener3D::_flush_area_exits() {
	for (const JPH::SubShapeIDPair &shape_pair : area_exits) {
		const JPH::BodyID &body_id1 = shape_pair.GetBody1ID();
		const JPH::BodyID &body_id2 = shape_pair.GetBody2ID();

		JoltObject3D *object1 = space->try_get_object(body_id1);
		JoltObject3D *object2 = space->try_get_object(body_id2);

		JoltArea3D *area1 = object1 != nullptr ? object1->as_area() : nullptr;
		JoltArea3D *area2 = object2 != nullptr ? object2->as_area() : nullptr;

		const JoltBody3D *body1 = object1 != nullptr ? object1->as_body() : nullptr;
		const JoltBody3D *body2 = object2 != nullptr ? object2->as_body() : nullptr;

		const JPH::SubShapeID &sub_shape_id1 = shape_pair.GetSubShapeID1();
		const JPH::SubShapeID &sub_shape_id2 = shape_pair.GetSubShapeID2();

		if (area1 != nullptr && area2 != nullptr) {
			area1->area_shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 != nullptr && body2 != nullptr) {
			area1->body_shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (body1 != nullptr && area2 != nullptr) {
			area2->body_shape_exited(body_id1, sub_shape_id1, sub_shape_id2);
		} else if (area1 != nullptr) {
			area1->shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area2 != nullptr) {
			area2->shape_exited(body_id1, sub_shape_id1, sub_shape_id2);
		}
	}

	area_exits.clear();
}

void JoltContactListener3D::_clear_area_soft_body_overlaps() {
	area_exits.reserve(area_soft_body_overlaps.size());
	for (const JPH::SubShapeIDPair &shape_pair : area_soft_body_overlaps) {
		area_exits.insert(shape_pair);
	}

	area_soft_body_overlaps.clear();
}

void JoltContactListener3D::pre_step() {
	_clear_area_soft_body_overlaps();

#ifdef DEBUG_ENABLED
	debug_contact_count = 0;
#endif
}

void JoltContactListener3D::post_step() {
	_flush_contacts();
	_flush_area_exits();
	_flush_area_enters();
}

// ============================================================================
// JoltMotionFilter3D / JoltQueryFilter3D
// ============================================================================

bool JoltMotionFilter3D::ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const {
	const JPH::BroadPhaseLayer::Type broad_phase_layer = (JPH::BroadPhaseLayer::Type)p_broad_phase_layer;

	switch (broad_phase_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return true;
		} break;
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return false;
		} break;
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled broad phase layer: '%d'. This should not happen. Please report this.", broad_phase_layer));
		}
	}
}

bool JoltMotionFilter3D::ShouldCollide(JPH::ObjectLayer p_object_layer) const {
	JPH::BroadPhaseLayer object_broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t object_collision_layer = 0;
	uint32_t object_collision_mask = 0;

	space.map_from_object_layer(p_object_layer, object_broad_phase_layer, object_collision_layer, object_collision_mask);

	return (body_self.get_collision_mask() & object_collision_layer) != 0;
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::BodyID &p_jolt_id) const {
	return p_jolt_id != body_self.get_jolt_id();
}

bool JoltMotionFilter3D::ShouldCollideLocked(const JPH::Body &p_jolt_body) const {
	if (p_jolt_body.IsSoftBody()) {
		return false;
	}

	const JoltObject3D *object = reinterpret_cast<const JoltObject3D *>(p_jolt_body.GetUserData());
	if (excluded_bodies.has(object->get_rid())) {
		return false;
	}

	return body_self.get_jolt_body()->GetCollisionGroup().CanCollide(p_jolt_body.GetCollisionGroup());
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::Shape *p_jolt_shape, const JPH::SubShapeID &p_jolt_shape_id) const {
	return true;
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::Shape *p_jolt_shape_self, const JPH::SubShapeID &p_jolt_shape_id_self, const JPH::Shape *p_jolt_shape_other, const JPH::SubShapeID &p_jolt_shape_id_other) const {
	if (collide_separation_ray) {
		return true;
	}

	const JoltCustomMotionShape *motion_shape = static_cast<const JoltCustomMotionShape *>(p_jolt_shape_self);
	const JPH::ConvexShape &actual_shape_self = motion_shape->get_inner_shape();
	if (actual_shape_self.GetSubType() == JoltCustomShapeSubType::RAY) {
		return static_cast<const JoltCustomRayShape &>(actual_shape_self).slide_on_slope;
	}

	return true;
}

JoltQueryFilter3D::JoltQueryFilter3D(const PhysicsDirectSpaceState3D &p_space_state, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, const HashSet<RID> &p_excluded, bool p_picking) :
		space(p_space_state.get_space()),
		excluded(p_excluded),
		collision_mask(p_collision_mask),
		collide_with_bodies(p_collide_with_bodies),
		collide_with_areas(p_collide_with_areas),
		picking(p_picking) {
}

bool JoltQueryFilter3D::ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const {
	const JPH::BroadPhaseLayer::Type broad_phase_layer = (JPH::BroadPhaseLayer::Type)p_broad_phase_layer;

	switch (broad_phase_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return collide_with_bodies;
		} break;
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return collide_with_areas;
		} break;
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled broad phase layer: '%d'. This should not happen. Please report this.", broad_phase_layer));
		}
	}
}

bool JoltQueryFilter3D::ShouldCollide(JPH::ObjectLayer p_object_layer) const {
	JPH::BroadPhaseLayer object_broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t object_collision_layer = 0;
	uint32_t object_collision_mask = 0;

	space.map_from_object_layer(p_object_layer, object_broad_phase_layer, object_collision_layer, object_collision_mask);

	return (collision_mask & object_collision_layer) != 0;
}

bool JoltQueryFilter3D::ShouldCollide(const JPH::BodyID &p_body_id) const {
	return true;
}

bool JoltQueryFilter3D::ShouldCollideLocked(const JPH::Body &p_body) const {
	JoltObject3D *object = reinterpret_cast<JoltObject3D *>(p_body.GetUserData());
	return (!picking || object->is_pickable()) && !excluded.has(object->get_rid());
}

// ============================================================================
// JoltSpace3D
// ============================================================================

namespace {

constexpr double SPACE_DEFAULT_CONTACT_RECYCLE_RADIUS = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_MAX_SEPARATION = 0.05;
constexpr double SPACE_DEFAULT_CONTACT_MAX_ALLOWED_PENETRATION = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_DEFAULT_BIAS = 0.8;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_LINEAR = 0.1;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_ANGULAR = 8.0 * Math::PI / 180;
constexpr double SPACE_DEFAULT_SOLVER_ITERATIONS = 8;

} // namespace

void JoltSpace3D::_pre_step(float p_step) {
	flush_pending_objects();

	while (needs_optimization_list.first()) {
		JoltObject3D *object = needs_optimization_list.first()->self();
		needs_optimization_list.remove(needs_optimization_list.first());
		object->commit_shapes(true);
	}

	contact_listener->pre_step();

	const JPH::BodyLockInterface &lock_iface = get_lock_iface();
	const JPH::BodyID *active_rigid_bodies = physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::RigidBody);
	const JPH::uint32 active_rigid_body_count = physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);

	for (JPH::uint32 i = 0; i < active_rigid_body_count; i++) {
		JPH::Body *jolt_body = lock_iface.TryGetBody(active_rigid_bodies[i]);
		JoltObject3D *object = reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	const JPH::BodyID *active_soft_bodies = physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::SoftBody);
	const JPH::uint32 active_soft_body_count = physics_system->GetNumActiveBodies(JPH::EBodyType::SoftBody);

	for (JPH::uint32 i = 0; i < active_soft_body_count; i++) {
		JPH::Body *jolt_body = lock_iface.TryGetBody(active_soft_bodies[i]);
		JoltObject3D *object = reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	physics_system->SetBodyActivationListener(body_activation_listener);
}

void JoltSpace3D::_post_step(float p_step) {
	physics_system->SetBodyActivationListener(nullptr);

	contact_listener->post_step();

	while (shapes_changed_list.first()) {
		JoltObject3D *object = shapes_changed_list.first()->self();
		shapes_changed_list.remove(shapes_changed_list.first());
		object->clear_previous_shape();
	}
}

JoltSpace3D::~JoltSpace3D() {
	if (direct_state != nullptr) {
		memdelete(direct_state);
		direct_state = nullptr;
	}

	if (physics_system != nullptr) {
		delete physics_system;
		physics_system = nullptr;
	}

	if (body_activation_listener != nullptr) {
		delete body_activation_listener;
		body_activation_listener = nullptr;
	}

	if (contact_listener != nullptr) {
		delete contact_listener;
		contact_listener = nullptr;
	}

	if (layers != nullptr) {
		delete layers;
		layers = nullptr;
	}
}

void JoltSpace3D::call_queries() {
	while (body_call_queries_list.first()) {
		JoltBody3D *body = body_call_queries_list.first()->self();
		body_call_queries_list.remove(body_call_queries_list.first());
		body->call_queries();
	}

	while (area_call_queries_list.first()) {
		JoltArea3D *body = area_call_queries_list.first()->self();
		area_call_queries_list.remove(area_call_queries_list.first());
		body->call_queries();
	}
}

double JoltSpace3D::get_param(PS3DE::SpaceParameter p_param) const {
	switch (p_param) {
		case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS: {
			return SPACE_DEFAULT_CONTACT_RECYCLE_RADIUS;
		}
		case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION: {
			return SPACE_DEFAULT_CONTACT_MAX_SEPARATION;
		}
		case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION: {
			return SPACE_DEFAULT_CONTACT_MAX_ALLOWED_PENETRATION;
		}
		case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS: {
			return SPACE_DEFAULT_CONTACT_DEFAULT_BIAS;
		}
		case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD: {
			return SPACE_DEFAULT_SLEEP_THRESHOLD_LINEAR;
		}
		case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD: {
			return SPACE_DEFAULT_SLEEP_THRESHOLD_ANGULAR;
		}
		case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS: {
			return SPACE_DEFAULT_SOLVER_ITERATIONS;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltSpace3D::set_param(PS3DE::SpaceParameter p_param, double p_value) {
	switch (p_param) {
		case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS: {
			WARN_PRINT("Space-specific contact recycle radius is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION: {
			WARN_PRINT("Space-specific contact max separation is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION: {
			WARN_PRINT("Space-specific contact max allowed penetration is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS: {
			WARN_PRINT("Space-specific contact default bias is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD: {
			WARN_PRINT("Space-specific linear velocity sleep threshold is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD: {
			WARN_PRINT("Space-specific angular velocity sleep threshold is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_TIME_TO_SLEEP: {
			WARN_PRINT("Space-specific body sleep time is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS: {
			WARN_PRINT("Space-specific solver iterations is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

JPH::BodyInterface &JoltSpace3D::get_body_iface() {
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyInterface &JoltSpace3D::get_body_iface() const {
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyLockInterface &JoltSpace3D::get_lock_iface() const {
	return physics_system->GetBodyLockInterfaceNoLock();
}

const JPH::BroadPhaseQuery &JoltSpace3D::get_broad_phase_query() const {
	return physics_system->GetBroadPhaseQuery();
}

const JPH::NarrowPhaseQuery &JoltSpace3D::get_narrow_phase_query() const {
	return physics_system->GetNarrowPhaseQueryNoLock();
}

JPH::ObjectLayer JoltSpace3D::map_to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask) {
	return layers->to_object_layer(p_broad_phase_layer, p_collision_layer, p_collision_mask);
}

void JoltSpace3D::map_from_object_layer(JPH::ObjectLayer p_object_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const {
	layers->from_object_layer(p_object_layer, r_broad_phase_layer, r_collision_layer, r_collision_mask);
}

JPH::Body *JoltSpace3D::try_get_jolt_body(const JPH::BodyID &p_body_id) const {
	return get_lock_iface().TryGetBody(p_body_id);
}

JoltObject3D *JoltSpace3D::try_get_object(const JPH::BodyID &p_body_id) const {
	const JPH::Body *jolt_body = try_get_jolt_body(p_body_id);
	if (unlikely(jolt_body == nullptr)) {
		return nullptr;
	}

	return reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
}

JoltBody3D *JoltSpace3D::try_get_body(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_body();
}

JoltArea3D *JoltSpace3D::try_get_area(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_area();
}

JoltSoftBody3D *JoltSpace3D::try_get_soft_body(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_soft_body();
}

PhysicsDirectSpaceState3D *JoltSpace3D::get_direct_state() {
	if (direct_state == nullptr) {
		direct_state = memnew(PhysicsDirectSpaceState3D(this));
	}

	return direct_state;
}

void JoltSpace3D::remove_object(const JPH::BodyID &p_jolt_id) {
	JPH::BodyInterface &body_iface = get_body_iface();

	if (!pending_objects_sleeping.erase_unordered(p_jolt_id) && !pending_objects_awake.erase_unordered(p_jolt_id)) {
		body_iface.RemoveBody(p_jolt_id);
	}

	body_iface.DestroyBody(p_jolt_id);

	if (PhysicsServer3D::get_driver() && !PhysicsServer3D::get_driver()->is_active() && physics_system->GetNumBodies() <= 1) {
		physics_system->OptimizeBroadPhase();
	}
}

void JoltSpace3D::flush_pending_objects() {
	if (pending_objects_sleeping.is_empty() && pending_objects_awake.is_empty()) {
		return;
	}

	MutexLock pending_objects_lock(pending_objects_mutex);

	JPH::BodyInterface &body_iface = get_body_iface();

	if (!pending_objects_sleeping.is_empty()) {
		JPH::BodyInterface::AddState add_state = body_iface.AddBodiesPrepare(pending_objects_sleeping.ptr(), pending_objects_sleeping.size());
		body_iface.AddBodiesFinalize(pending_objects_sleeping.ptr(), pending_objects_sleeping.size(), add_state, JPH::EActivation::DontActivate);
		pending_objects_sleeping.reset();
	}

	if (!pending_objects_awake.is_empty()) {
		JPH::BodyInterface::AddState add_state = body_iface.AddBodiesPrepare(pending_objects_awake.ptr(), pending_objects_awake.size());
		body_iface.AddBodiesFinalize(pending_objects_awake.ptr(), pending_objects_awake.size(), add_state, JPH::EActivation::Activate);
		pending_objects_awake.reset();
	}
}

void JoltSpace3D::set_is_object_sleeping(const JPH::BodyID &p_jolt_id, bool p_enable) {
	if (p_enable) {
		if (pending_objects_awake.erase_unordered(p_jolt_id)) {
			pending_objects_sleeping.push_back(p_jolt_id);
		} else if (pending_objects_sleeping.has(p_jolt_id)) {
			// Do nothing.
		} else {
			get_body_iface().DeactivateBody(p_jolt_id);
		}
	} else {
		if (pending_objects_sleeping.erase_unordered(p_jolt_id)) {
			pending_objects_awake.push_back(p_jolt_id);
		} else if (pending_objects_awake.has(p_jolt_id)) {
			// Do nothing.
		} else {
			get_body_iface().ActivateBody(p_jolt_id);
		}
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltBody3D> *p_body) {
	MutexLock body_call_queries_lock(body_call_queries_mutex);

	if (!p_body->in_list()) {
		body_call_queries_list.add(p_body);
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltArea3D> *p_area) {
	if (!p_area->in_list()) {
		area_call_queries_list.add(p_area);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltBody3D> *p_body) {
	if (p_body->in_list()) {
		body_call_queries_list.remove(p_body);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltArea3D> *p_area) {
	if (p_area->in_list()) {
		area_call_queries_list.remove(p_area);
	}
}

void JoltSpace3D::enqueue_shapes_changed(SelfList<JoltObject3D> *p_object) {
	if (!p_object->in_list()) {
		shapes_changed_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_shapes_changed(SelfList<JoltObject3D> *p_object) {
	if (p_object->in_list()) {
		shapes_changed_list.remove(p_object);
	}
}

void JoltSpace3D::enqueue_needs_optimization(SelfList<JoltObject3D> *p_object) {
	if (!p_object->in_list()) {
		needs_optimization_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_needs_optimization(SelfList<JoltObject3D> *p_object) {
	if (p_object->in_list()) {
		needs_optimization_list.remove(p_object);
	}
}

void JoltSpace3D::add_joint(JPH::Constraint *p_jolt_ref) {
	physics_system->AddConstraint(p_jolt_ref);
}

void JoltSpace3D::add_joint(JoltJoint3D *p_joint) { add_joint(p_joint->get_jolt_ref()); }

void JoltSpace3D::remove_joint(JPH::Constraint *p_jolt_ref) {
	physics_system->RemoveConstraint(p_jolt_ref);
}

void JoltSpace3D::remove_joint(JoltJoint3D *p_joint) { remove_joint(p_joint->get_jolt_ref()); }

#ifdef DEBUG_ENABLED

const PackedVector3Array &JoltSpace3D::get_debug_contacts() const {
	return contact_listener->get_debug_contacts();
}

int JoltSpace3D::get_debug_contact_count() const {
	return contact_listener->get_debug_contact_count();
}

int JoltSpace3D::get_max_debug_contacts() const {
	return contact_listener->get_max_debug_contacts();
}

void JoltSpace3D::set_max_debug_contacts(int p_count) {
	contact_listener->set_max_debug_contacts(p_count);
}

void JoltSpace3D::dump_debug_snapshot(const String &) {}

#endif

void JoltSpace3D::step(float) {}
JoltSpace3D::JoltSpace3D(JPH::JobSystem *, JPH::TempAllocator *) {}
JPH::Body *JoltSpace3D::add_object(const JoltObject3D &, const JPH::BodyCreationSettings &, bool) { return nullptr; }

// ============================================================================
// PhysicsDirectSpaceState3D
// ============================================================================

namespace {

bool _cast_motion_impl(JoltSpace3D *p_space, const JPH::Shape &p_jolt_shape, const Transform3D &p_transform_com, const Vector3 &p_scale, const Vector3 &p_motion, bool p_use_edge_removal, bool p_ignore_overlaps, const JPH::CollideShapeSettings &p_settings, const JPH::BroadPhaseLayerFilter &p_broad_phase_layer_filter, const JPH::ObjectLayerFilter &p_object_layer_filter, const JPH::BodyFilter &p_body_filter, const JPH::ShapeFilter &p_shape_filter, real_t &r_closest_safe, real_t &r_closest_unsafe) {
	r_closest_safe = 1.0f;
	r_closest_unsafe = 1.0f;

	ERR_FAIL_NULL_V(p_space, false);
	ERR_FAIL_COND_V_MSG(p_jolt_shape.GetType() != JPH::EShapeType::Convex, false, "Shape-casting with non-convex shapes is not supported.");

	const float motion_length = (float)p_motion.length();

	if (p_ignore_overlaps && motion_length == 0.0f) {
		return false;
	}

	const JPH::RMat44 transform_com = to_jolt_r(p_transform_com);
	const JPH::Vec3 scale = to_jolt(p_scale);
	const JPH::Vec3 motion = to_jolt(p_motion);
	const JPH::Vec3 motion_local = transform_com.Multiply3x3Transposed(motion);

	JPH::AABox aabb = p_jolt_shape.GetWorldSpaceBounds(transform_com, scale);
	JPH::AABox aabb_translated = aabb;
	aabb_translated.Translate(motion);
	aabb.Encapsulate(aabb_translated);

	JoltQueryCollectorAnyMulti<JPH::CollideShapeBodyCollector, 1024> aabb_collector;
	p_space->get_broad_phase_query().CollideAABox(aabb, aabb_collector, p_broad_phase_layer_filter, p_object_layer_filter);

	if (!aabb_collector.had_hit()) {
		return false;
	}

	const JPH::RVec3 base_offset = transform_com.GetTranslation();

	JoltCustomMotionShape motion_shape(static_cast<const JPH::ConvexShape &>(p_jolt_shape));

	auto collides = [&](const JPH::Body &p_other_body, float p_fraction) {
		motion_shape.set_motion(motion_local * p_fraction);

		const JPH::TransformedShape other_shape = p_other_body.GetTransformedShape();

		JoltQueryCollectorAny<JPH::CollideShapeCollector> collector;

		if (p_use_edge_removal) {
			JPH::CollideShapeSettings eier_settings = p_settings;
			eier_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
			eier_settings.mCollectFacesMode = JPH::ECollectFacesMode::CollectFaces;

			JPH::InternalEdgeRemovingCollector eier_collector(collector, JPH::cDefaultInternalEdgeRemovalVertexToleranceSq);
			other_shape.CollideShape(&motion_shape, scale, transform_com, eier_settings, base_offset, eier_collector, p_shape_filter);
			eier_collector.Flush();
		} else {
			other_shape.CollideShape(&motion_shape, scale, transform_com, p_settings, base_offset, collector, p_shape_filter);
		}

		return collector.had_hit();
	};

	const int step_count = CLAMP(int(std::log(1000.0f * motion_length) / (float)Math::LN2), 4, 16);

	bool collided = false;

	for (int i = 0; i < aabb_collector.get_hit_count(); ++i) {
		const JPH::BodyID other_jolt_id = aabb_collector.get_hit(i);
		if (!p_body_filter.ShouldCollide(other_jolt_id)) {
			continue;
		}

		const JPH::Body *other_jolt_body = p_space->try_get_jolt_body(other_jolt_id);
		if (!p_body_filter.ShouldCollideLocked(*other_jolt_body)) {
			continue;
		}

		if (!collides(*other_jolt_body, 1.0f)) {
			continue;
		}

		if (p_ignore_overlaps && collides(*other_jolt_body, 0.0f)) {
			continue;
		}

		float lo = 0.0f;
		float hi = 1.0f;
		float coeff = 0.5f;

		for (int j = 0; j < step_count; ++j) {
			const float fraction = lo + (hi - lo) * coeff;

			if (collides(*other_jolt_body, fraction)) {
				collided = true;

				hi = fraction;

				if (j == 0 || lo > 0.0f) {
					coeff = 0.5f;
				} else {
					coeff = 0.25f;
				}
			} else {
				lo = fraction;

				if (j == 0 || hi < 1.0f) {
					coeff = 0.5f;
				} else {
					coeff = 0.75f;
				}
			}
		}

		if (lo < r_closest_safe) {
			r_closest_safe = lo;
			r_closest_unsafe = hi;
		}
	}

	return collided;
}

void _generate_manifold(JoltSpace3D *p_space, const JPH::CollideShapeResult &p_hit, JPH::ContactPoints &r_contact_points1, JPH::ContactPoints &r_contact_points2 JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_center_of_mass)) {
	const JPH::PhysicsSystem &physics_system = p_space->get_physics_system();
	const JPH::PhysicsSettings &physics_settings = physics_system.GetPhysicsSettings();
	const JPH::Vec3 penetration_axis = p_hit.mPenetrationAxis.Normalized();

	JPH::ManifoldBetweenTwoFaces(p_hit.mContactPointOn1, p_hit.mContactPointOn2, penetration_axis, physics_settings.mManifoldTolerance, p_hit.mShape1Face, p_hit.mShape2Face, r_contact_points1, r_contact_points2 JPH_IF_DEBUG_RENDERER(, p_center_of_mass));

	if (r_contact_points1.size() > 4) {
		JPH::PruneContactPoints(penetration_axis, r_contact_points1, r_contact_points2 JPH_IF_DEBUG_RENDERER(, p_center_of_mass));
	}
}

void _collide_shape_queries(JoltSpace3D *p_space, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, JPH::RMat44Arg p_transform_com, const JPH::CollideShapeSettings &p_settings, JPH::RVec3Arg p_base_offset, JPH::CollideShapeCollector &p_collector, const JPH::BroadPhaseLayerFilter &p_broad_phase_layer_filter = JPH::BroadPhaseLayerFilter(), const JPH::ObjectLayerFilter &p_object_layer_filter = JPH::ObjectLayerFilter(), const JPH::BodyFilter &p_body_filter = JPH::BodyFilter(), const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) {
}

} // namespace

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D(JoltSpace3D *p_space) :
		space(p_space) {
}

bool PhysicsDirectSpaceState3D::collide_shape(const PS3DT::ShapeParameters &p_parameters, Vector3 *r_results, int p_result_max, int &r_result_count) {
	r_result_count = 0;

	ERR_FAIL_NULL_V(space, false);
	ERR_FAIL_COND_V_MSG(space->is_stepping(), false, "collide_shape must not be called while the physics space is being stepped.");

	if (p_result_max == 0) {
		return false;
	}

	space->flush_pending_objects();

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL_V(physics_server, false);
	JoltShape3D *shape = physics_server->get_shape(p_parameters.shape_rid);
	ERR_FAIL_NULL_V(shape, false);

	const JPH::ShapeRefC jolt_shape = shape->try_build();
	ERR_FAIL_NULL_V(jolt_shape, false);

	Transform3D transform = p_parameters.transform;
	JOLT_ENSURE_SCALE_NOT_ZERO(transform, "collide_shape was passed an invalid transform.");

	Vector3 scale;
	JoltMath::decompose(transform, scale);
	JOLT_ENSURE_SCALE_VALID(jolt_shape, scale, "collide_shape was passed an invalid transform.");

	const Vector3 com_scaled = to_godot(jolt_shape->GetCenterOfMass());
	const Transform3D transform_com = transform.translated_local(com_scaled);

	JPH::CollideShapeSettings settings;
	settings.mCollectFacesMode = JPH::ECollectFacesMode::CollectFaces;
	settings.mMaxSeparationDistance = (float)p_parameters.margin;

	const Vector3 &base_offset = transform_com.origin;

	const JoltQueryFilter3D query_filter(*this, p_parameters.collision_mask, p_parameters.collide_with_bodies, p_parameters.collide_with_areas, p_parameters.exclude);
	JoltQueryCollectorAnyMulti<JPH::CollideShapeCollector, 32> collector(p_result_max);
	_collide_shape_queries(space, jolt_shape, to_jolt(scale), to_jolt_r(transform_com), settings, to_jolt_r(base_offset), collector, query_filter, query_filter, query_filter);

	if (!collector.had_hit()) {
		return false;
	}

	const int max_points = p_result_max * 2;

	int point_count = 0;

	for (int i = 0; i < collector.get_hit_count(); ++i) {
		const JPH::CollideShapeResult &hit = collector.get_hit(i);

		const Vector3 penetration_axis = to_godot(hit.mPenetrationAxis.Normalized());
		const Vector3 margin_offset = penetration_axis * (float)p_parameters.margin;

		JPH::ContactPoints contact_points1;
		JPH::ContactPoints contact_points2;

		_generate_manifold(space, hit, contact_points1, contact_points2 JPH_IF_DEBUG_RENDERER(, to_jolt_r(base_offset)));

		for (JPH::uint j = 0; j < contact_points1.size(); ++j) {
			r_results[point_count++] = base_offset + to_godot(contact_points1[j]) + margin_offset;
			r_results[point_count++] = base_offset + to_godot(contact_points2[j]);

			if (point_count >= max_points) {
				break;
			}
		}

		if (point_count >= max_points) {
			break;
		}
	}

	r_result_count = point_count / 2;

	return true;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(RID p_object, Vector3 p_point) const {
	ERR_FAIL_NULL_V(space, Vector3());
	ERR_FAIL_COND_V_MSG(space->is_stepping(), Vector3(), "get_closest_point_to_object_volume must not be called while the physics space is being stepped.");

	space->flush_pending_objects();

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL_V(physics_server, Vector3());
	JoltObject3D *object = physics_server->get_area(p_object);

	if (object == nullptr) {
		object = physics_server->get_body(p_object);
	}

	ERR_FAIL_NULL_V(object, Vector3());
	ERR_FAIL_COND_V(object->get_space() != space, Vector3());

	JoltQueryCollectorAll<JPH::TransformedShapeCollector, 32> collector;
	const JPH::TransformedShape root_shape = object->get_jolt_body()->GetTransformedShape();
	root_shape.CollectTransformedShapes(object->get_jolt_body()->GetWorldSpaceBounds(), collector);

	const JPH::RVec3 point = to_jolt_r(p_point);

	float closest_distance_sq = FLT_MAX;
	JPH::RVec3 closest_point = JPH::RVec3::sZero();

	bool found_point = false;

	for (int i = 0; i < collector.get_hit_count(); ++i) {
		const JPH::TransformedShape &shape_transformed = collector.get_hit(i);
		const JPH::Shape &shape = *shape_transformed.mShape;

		if (shape.GetType() != JPH::EShapeType::Convex) {
			continue;
		}

		const JPH::ConvexShape &shape_convex = static_cast<const JPH::ConvexShape &>(shape);

		JPH::GJKClosestPoint gjk;

		JPH::ConvexShape::SupportBuffer shape_support_buffer;
		const JPH::ConvexShape::Support *shape_support = shape_convex.GetSupportFunction(JPH::ConvexShape::ESupportMode::IncludeConvexRadius, shape_support_buffer, shape_transformed.GetShapeScale());

		const JPH::RMat44 shape_rotation = JPH::RMat44::sRotation(shape_transformed.mShapeRotation);
		const JPH::Vec3 shape_com = shape_rotation.Multiply3x3(shape.GetCenterOfMass());
		const JPH::RVec3 shape_pos = shape_transformed.mShapePositionCOM - JPH::RVec3(shape_com);
		const JPH::RMat44 shape_xform = shape_rotation.PostTranslated(shape_pos);
		const JPH::RMat44 shape_xform_inv = shape_xform.InversedRotationTranslation();

		JPH::PointConvexSupport point_support;
		point_support.mPoint = JPH::Vec3(shape_xform_inv * point);

		JPH::Vec3 separating_axis = JPH::Vec3::sAxisX();
		JPH::Vec3 point_on_a = JPH::Vec3::sZero();
		JPH::Vec3 point_on_b = JPH::Vec3::sZero();

		const float distance_sq = gjk.GetClosestPoints(*shape_support, point_support, JPH::cDefaultCollisionTolerance, FLT_MAX, separating_axis, point_on_a, point_on_b);

		if (distance_sq == 0.0f) {
			closest_point = point;
			found_point = true;
			break;
		}

		if (distance_sq < closest_distance_sq) {
			closest_distance_sq = distance_sq;
			closest_point = shape_xform * point_on_a;
			found_point = true;
		}
	}

	if (found_point) {
		return to_godot(closest_point);
	} else {
		return to_godot(object->get_jolt_body()->GetPosition());
	}
}

bool PhysicsDirectSpaceState3D::body_test_motion(const JoltBody3D &p_body, const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult *r_result) const {
	return false;
}

bool PhysicsDirectSpaceState3D::intersect_ray(const PS3DT::RayParameters &p_ray_params, PS3DT::RayResult &r_result) {
	return false;
}

int PhysicsDirectSpaceState3D::intersect_point(const PS3DT::PointParameters &p_point_params, PS3DT::ShapeResult *r_results, int p_result_max) {
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(const PS3DT::ShapeParameters &p_shape_params, PS3DT::ShapeResult *r_results, int p_result_max) {
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(const PS3DT::ShapeParameters &p_shape_params, real_t &r_closest_safe, real_t &r_closest_unsafe, PS3DT::ShapeRestInfo *r_info) {
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(const PS3DT::ShapeParameters &p_shape_params, PS3DT::ShapeRestInfo *r_info) {
	return false;
}

// ============================================================================
// Merged from modules/jolt_physics/shapes/jolt_shape_3d.cpp
// ============================================================================


#include "modules/jolt_physics/jolt_physics_server_3d.h"


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
#endif // defined(MODULE_JOLT_PHYSICS_ENABLED)


// --- Merged from modules/vhacd/register_types.cpp ---
#if defined(MODULE_VHACD_ENABLED)
#include "modules/register_module_types.h"

#include "scene/resources/mesh.h"

#include <thirdparty/vhacd/public/VHACD.h>

static Vector<Vector<Vector3>> convex_decompose(const real_t *p_vertices, int p_vertex_count, const uint32_t *p_triangles, int p_triangle_count, const Ref<MeshConvexDecompositionSettings> &p_settings, Vector<Vector<uint32_t>> *r_convex_indices) {
	VHACD::IVHACD::Parameters params;
	params.m_concavity = p_settings->get_max_concavity();
	params.m_alpha = p_settings->get_symmetry_planes_clipping_bias();
	params.m_beta = p_settings->get_revolution_axes_clipping_bias();
	params.m_minVolumePerCH = p_settings->get_min_volume_per_convex_hull();
	params.m_resolution = p_settings->get_resolution();
	params.m_maxNumVerticesPerCH = p_settings->get_max_num_vertices_per_convex_hull();
	params.m_planeDownsampling = p_settings->get_plane_downsampling();
	params.m_convexhullDownsampling = p_settings->get_convex_hull_downsampling();
	params.m_pca = p_settings->get_normalize_mesh();
	params.m_mode = p_settings->get_mode();
	params.m_convexhullApproximation = p_settings->get_convex_hull_approximation();
	params.m_oclAcceleration = true;
	params.m_maxConvexHulls = p_settings->get_max_convex_hulls();
	params.m_projectHullVertices = p_settings->get_project_hull_vertices();

	VHACD::IVHACD *decomposer = VHACD::CreateVHACD();
	decomposer->Compute(p_vertices, p_vertex_count, p_triangles, p_triangle_count, params);

	int hull_count = decomposer->GetNConvexHulls();

	Vector<Vector<Vector3>> ret;
	ret.resize(hull_count);

	if (r_convex_indices) {
		r_convex_indices->resize(hull_count);
	}

	for (int i = 0; i < hull_count; i++) {
		VHACD::IVHACD::ConvexHull hull;
		decomposer->GetConvexHull(i, hull);

		Vector<Vector3> &points = ret.write[i];
		points.resize(hull.m_nPoints);

		Vector3 *w = points.ptrw();
		for (uint32_t j = 0; j < hull.m_nPoints; ++j) {
			for (int k = 0; k < 3; k++) {
				w[j][k] = hull.m_points[j * 3 + k];
			}
		}

		if (r_convex_indices) {
			Vector<uint32_t> &indices = r_convex_indices->write[i];
			indices.resize(hull.m_nTriangles * 3);

			memcpy(indices.ptrw(), hull.m_triangles, hull.m_nTriangles * 3 * sizeof(uint32_t));
		}
	}

	decomposer->Clean();
	decomposer->Release();

	return ret;
}

void initialize_vhacd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Mesh::convex_decomposition_function = convex_decompose;
}

void uninitialize_vhacd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Mesh::convex_decomposition_function = nullptr;
}
#endif // defined(MODULE_VHACD_ENABLED)

#endif // PHYSICS_3D_DISABLED

// === Merged from modules/godot_physics_2d/godot_physics_server_2d.cpp ===
#ifndef PHYSICS_2D_DISABLED
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

#include "core/os/os.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#define FLUSH_QUERY_CHECK(m_object)                                                                \
	ERR_FAIL_COND_MSG(m_object->get_space() && flushing_queries,                                   \
		"Can't change this state while flushing queries. Use call_deferred() or set_deferred() "   \
		"to change monitoring state instead.");

RID GodotPhysicsServer2D::_shape_create(PS2DE::ShapeType p_shape)
{
	ERR_FAIL_COND_V(p_shape == PS2DE::SHAPE_CUSTOM, RID());

	GodotShape2D* shape = memnew(GodotShape2D(p_shape));

	RID id = shape_owner.make_rid(shape);
	shape->set_self(id);

	return id;
}

RID GodotPhysicsServer2D::world_boundary_shape_create()
{
	return _shape_create(PS2DE::SHAPE_WORLD_BOUNDARY);
}

RID GodotPhysicsServer2D::separation_ray_shape_create()
{
	return _shape_create(PS2DE::SHAPE_SEPARATION_RAY);
}

RID GodotPhysicsServer2D::segment_shape_create() { return _shape_create(PS2DE::SHAPE_SEGMENT); }

RID GodotPhysicsServer2D::circle_shape_create() { return _shape_create(PS2DE::SHAPE_CIRCLE); }

RID GodotPhysicsServer2D::rectangle_shape_create() { return _shape_create(PS2DE::SHAPE_RECTANGLE); }

RID GodotPhysicsServer2D::capsule_shape_create() { return _shape_create(PS2DE::SHAPE_CAPSULE); }

RID GodotPhysicsServer2D::convex_polygon_shape_create()
{
	return _shape_create(PS2DE::SHAPE_CONVEX_POLYGON);
}

RID GodotPhysicsServer2D::concave_polygon_shape_create()
{
	return _shape_create(PS2DE::SHAPE_CONCAVE_POLYGON);
}

void GodotPhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	shape->set_custom_bias(p_bias);
}

PS2DE::ShapeType GodotPhysicsServer2D::shape_get_type(RID p_shape) const
{
	const GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, PS2DE::SHAPE_CUSTOM);
	return shape->get_type();
}

real_t GodotPhysicsServer2D::shape_get_custom_solver_bias(RID p_shape) const
{
	const GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, 0);
	return shape->get_custom_bias();
}

void GodotPhysicsServer2D::_shape_col_cbk(
	const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata)
{
	CollCbkData* cbk = static_cast<CollCbkData*>(p_userdata);

	if (cbk->max == 0) {
		return;
	}

	Vector2 rel_dir = (p_point_A - p_point_B);
	real_t rel_length2 = rel_dir.length_squared();
	if (cbk->valid_dir != Vector2()) {
		if (cbk->valid_depth < 10e20) {
			if (rel_length2 > cbk->valid_depth * cbk->valid_depth ||
				(rel_length2 > CMP_EPSILON &&
					cbk->valid_dir.dot(rel_dir.normalized()) < CMP_EPSILON)) {
				cbk->invalid_by_dir++;
				return;
			}
		}
		else {
			if (rel_length2 > 0 && cbk->valid_dir.dot(rel_dir.normalized()) < CMP_EPSILON) {
				return;
			}
		}
	}

	if (cbk->amount == cbk->max) {
		// find least deep
		real_t min_depth = 1e20;
		int min_depth_idx = 0;
		for (int i = 0; i < cbk->amount; i++) {
			real_t d = cbk->ptr[i * 2 + 0].distance_squared_to(cbk->ptr[i * 2 + 1]);
			if (d < min_depth) {
				min_depth = d;
				min_depth_idx = i;
			}
		}

		if (rel_length2 < min_depth) {
			return;
		}
		cbk->ptr[min_depth_idx * 2 + 0] = p_point_A;
		cbk->ptr[min_depth_idx * 2 + 1] = p_point_B;
		cbk->passed++;

	}
	else {
		cbk->ptr[cbk->amount * 2 + 0] = p_point_A;
		cbk->ptr[cbk->amount * 2 + 1] = p_point_B;
		cbk->amount++;
		cbk->passed++;
	}
}

bool GodotPhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A,
	const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B,
	const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count)
{
	GodotShape2D* shape_A = shape_owner.get_or_null(p_shape_A);
	ERR_FAIL_NULL_V(shape_A, false);
	GodotShape2D* shape_B = shape_owner.get_or_null(p_shape_B);
	ERR_FAIL_NULL_V(shape_B, false);

	if (p_result_max == 0) {
		return GodotCollisionSolver2D::solve(
			shape_A, p_xform_A, p_motion_A, shape_B, p_xform_B, p_motion_B, nullptr, nullptr);
	}

	CollCbkData cbk;
	cbk.max = p_result_max;
	cbk.amount = 0;
	cbk.passed = 0;
	cbk.ptr = r_results;

	bool res = GodotCollisionSolver2D::solve(
		shape_A, p_xform_A, p_motion_A, shape_B, p_xform_B, p_motion_B, _shape_col_cbk, &cbk);
	r_result_count = cbk.amount;
	return res;
}

RID GodotPhysicsServer2D::space_create()
{
	GodotSpace2D* space = memnew(GodotSpace2D);
	RID id = space_owner.make_rid(space);
	space->set_self(id);
	RID area_id = area_create();
	GodotArea2D* area = area_owner.get_or_null(area_id);
	ERR_FAIL_NULL_V(area, RID());
	space->set_default_area(area);
	area->set_space(space);
	area->set_priority(-1);

	return id;
}

void GodotPhysicsServer2D::space_set_active(RID p_space, bool p_active)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);
	if (p_active) {
		active_spaces.insert(space);
	}
	else {
		active_spaces.erase(space);
	}
}

bool GodotPhysicsServer2D::space_is_active(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, false);

	return active_spaces.has(space);
}

void GodotPhysicsServer2D::space_set_param(
	RID p_space, PS2DE::SpaceParameter p_param, real_t p_value)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	space->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param) const
{
	const GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0);
	return space->get_param(p_param);
}

void GodotPhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);
	space->set_debug_contacts(p_max_contacts);
}

Vector<Vector2> GodotPhysicsServer2D::space_get_contacts(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, Vector<Vector2>());
	return space->get_debug_contacts();
}

int GodotPhysicsServer2D::space_get_contact_count(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0);
	return space->get_debug_contact_count();
}

PhysicsDirectSpaceState2D* GodotPhysicsServer2D::space_get_direct_state(RID p_space)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, nullptr);
	ERR_FAIL_COND_V_MSG((using_threads && !doing_sync) || space->is_locked(), nullptr,
		"Space state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return space->get_direct_state();
}

RID GodotPhysicsServer2D::area_create()
{
	GodotArea2D* area = memnew(GodotArea2D);
	RID rid = area_owner.make_rid(area);
	area->set_self(rid);
	return rid;
}

void GodotPhysicsServer2D::area_set_space(RID p_area, RID p_space)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotSpace2D* space = nullptr;
	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	if (area->get_space() == space) {
		return; // pointless
	}

	area->clear_constraints();
	area->set_space(space);
}

RID GodotPhysicsServer2D::area_get_space(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	GodotSpace2D* space = area->get_space();
	if (!space) {
		return RID();
	}
	return space->get_self();
}

void GodotPhysicsServer2D::area_add_shape(
	RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	area->add_shape(shape, p_transform, p_disabled);
}

void GodotPhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	ERR_FAIL_COND(!shape->is_configured());

	area->set_shape(p_shape_idx, shape);
}

void GodotPhysicsServer2D::area_set_shape_transform(
	RID p_area, int p_shape_idx, const Transform2D& p_transform)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_shape_transform(p_shape_idx, p_transform);
}

void GodotPhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	ERR_FAIL_INDEX(p_shape, area->get_shape_count());
	FLUSH_QUERY_CHECK(area);

	area->set_shape_disabled(p_shape, p_disabled);
}

int GodotPhysicsServer2D::area_get_shape_count(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, -1);

	return area->get_shape_count();
}

RID GodotPhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	GodotShape2D* shape = area->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_self();
}

Transform2D GodotPhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform2D());

	return area->get_shape_transform(p_shape_idx);
}

void GodotPhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->remove_shape(p_shape_idx);
}

void GodotPhysicsServer2D::area_clear_shapes(RID p_area)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	while (area->get_shape_count()) {
		area->remove_shape(0);
	}
}

void GodotPhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	area->set_transform(p_transform);
}

Transform2D GodotPhysicsServer2D::area_get_transform(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform2D());

	return area->get_transform();
}

void GodotPhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	area->set_pickable(p_pickable);
}

void GodotPhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	FLUSH_QUERY_CHECK(area);

	area->set_monitorable(p_monitorable);
}

void GodotPhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_layer(p_layer);
}

uint32_t GodotPhysicsServer2D::area_get_collision_layer(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_layer();
}

void GodotPhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_mask(p_mask);
}

uint32_t GodotPhysicsServer2D::area_get_collision_mask(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_mask();
}

/* BODY API */

RID GodotPhysicsServer2D::body_create()
{
	GodotBody2D* body = memnew(GodotBody2D);
	RID rid = body_owner.make_rid(body);
	body->set_self(rid);
	return rid;
}

void GodotPhysicsServer2D::body_set_space(RID p_body, RID p_space)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	GodotSpace2D* space = nullptr;
	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	if (body->get_space() == space) {
		return; // pointless
	}

	body->clear_constraint_list();
	body->set_space(space);
}

RID GodotPhysicsServer2D::body_get_space(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	GodotSpace2D* space = body->get_space();
	if (!space) {
		return RID();
	}
	return space->get_self();
}

void GodotPhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	FLUSH_QUERY_CHECK(body);

	body->set_mode(p_mode);
}

PS2DE::BodyMode GodotPhysicsServer2D::body_get_mode(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, PS2DE::BODY_MODE_STATIC);

	return body->get_mode();
}

void GodotPhysicsServer2D::body_add_shape(
	RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	body->add_shape(shape, p_transform, p_disabled);
}

void GodotPhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	ERR_FAIL_COND(!shape->is_configured());

	body->set_shape(p_shape_idx, shape);
}

void GodotPhysicsServer2D::body_set_shape_transform(
	RID p_body, int p_shape_idx, const Transform2D& p_transform)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_shape_transform(p_shape_idx, p_transform);
}

int GodotPhysicsServer2D::body_get_shape_count(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, -1);

	return body->get_shape_count();
}

RID GodotPhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	GodotShape2D* shape = body->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_self();
}

Transform2D GodotPhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Transform2D());

	return body->get_shape_transform(p_shape_idx);
}

void GodotPhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_shape(p_shape_idx);
}

void GodotPhysicsServer2D::body_clear_shapes(RID p_body)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	while (body->get_shape_count()) {
		body->remove_shape(0);
	}
}

void GodotPhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	ERR_FAIL_INDEX(p_shape_idx, body->get_shape_count());
	FLUSH_QUERY_CHECK(body);

	body->set_shape_disabled(p_shape_idx, p_disabled);
}

void GodotPhysicsServer2D::body_set_shape_as_one_way_collision(
	RID p_body, int p_shape_idx, bool p_enable, real_t p_margin, const Vector2& p_direction)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	ERR_FAIL_INDEX(p_shape_idx, body->get_shape_count());
	FLUSH_QUERY_CHECK(body);

	body->set_shape_as_one_way_collision(p_shape_idx, p_enable, p_margin, p_direction);
}

void GodotPhysicsServer2D::body_set_continuous_collision_detection_mode(
	RID p_body, PS2DE::CCDMode p_mode)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_continuous_collision_detection_mode(p_mode);
}

PS2DE::CCDMode GodotPhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body) const
{
	const GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, PS2DE::CCD_MODE_DISABLED);

	return body->get_continuous_collision_detection_mode();
}

void GodotPhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_collision_layer(p_layer);
}

uint32_t GodotPhysicsServer2D::body_get_collision_layer(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_layer();
}

void GodotPhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_collision_mask(p_mask);
}

uint32_t GodotPhysicsServer2D::body_get_collision_mask(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_mask();
}

void GodotPhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_priority(p_priority);
}

real_t GodotPhysicsServer2D::body_get_collision_priority(RID p_body) const
{
	const GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_priority();
}

void GodotPhysicsServer2D::body_reset_mass_properties(RID p_body)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->reset_mass_properties();
}

void GodotPhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_impulse(p_impulse);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	body->apply_torque_impulse(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_impulse(
	RID p_body, const Vector2& p_impulse, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	body->apply_impulse(p_impulse, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_force(p_force);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_force(
	RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_force(p_force, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_torque(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_central_force(p_force);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_force(
	RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_force(p_force, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_torque(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_force(p_force);
	if (!p_force.is_zero_approx()) {
		body->wakeup();
	}
}

Vector2 GodotPhysicsServer2D::body_get_constant_force(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Vector2());
	return body->get_constant_force();
}

void GodotPhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_torque(p_torque);
	if (!Math::is_zero_approx(p_torque)) {
		body->wakeup();
	}
}

real_t GodotPhysicsServer2D::body_get_constant_torque(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_constant_torque();
}

void GodotPhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	Vector2 v = body->get_linear_velocity();
	Vector2 axis = p_axis_velocity.normalized();
	v -= axis * axis.dot(v);
	v += p_axis_velocity;
	body->set_linear_velocity(v);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_exception(p_body_b);
	body->wakeup();
}

void GodotPhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_exception(p_body_b);
	body->wakeup();
}

void GodotPhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	for (int i = 0; i < body->get_exceptions().size(); i++) {
		p_exceptions->push_back(body->get_exceptions()[i]);
	}
}

void GodotPhysicsServer2D::body_set_contacts_reported_depth_threshold(
	RID p_body, real_t p_threshold)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
}

real_t GodotPhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);
	return 0;
}

void GodotPhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_omit_force_integration(p_omit);
}

bool GodotPhysicsServer2D::body_is_omitting_force_integration(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	return body->get_omit_force_integration();
}

void GodotPhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_max_contacts_reported(p_contacts);
}

int GodotPhysicsServer2D::body_get_max_contacts_reported(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, -1);
	return body->get_max_contacts_reported();
}

bool GodotPhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape,
	const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max,
	int& r_result_count)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	ERR_FAIL_INDEX_V(p_body_shape, body->get_shape_count(), false);

	return shape_collide(body->get_shape(p_body_shape)->get_self(),
		body->get_transform() * body->get_shape_transform(p_body_shape), Vector2(), p_shape,
		p_shape_xform, p_motion, r_results, p_result_max, r_result_count);
}

void GodotPhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_pickable(p_pickable);
}

bool GodotPhysicsServer2D::_body_test_motion(RID p_body,
	PhysicsTestMotionParameters2D* rp_parameters,
	const Ref<PhysicsTestMotionResult2D>& p_result)
{
	PS2DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

bool GodotPhysicsServer2D::body_test_motion(
	RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	ERR_FAIL_NULL_V(body->get_space(), false);
	ERR_FAIL_COND_V(body->get_space()->is_locked(), false);

	_update_shapes();

	return body->get_space()->test_body_motion(body, p_parameters, r_result);
}

PhysicsDirectBodyState2D* GodotPhysicsServer2D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V_MSG((using_threads && !doing_sync), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	if (!body_owner.owns(p_body)) {
		return nullptr;
	}

	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, nullptr);

	if (!body->get_space()) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(body->get_space()->is_locked(), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return body->get_direct_state();
}

/* JOINT API */

RID GodotPhysicsServer2D::joint_create()
{
	GodotJoint2D* joint = memnew(GodotJoint2D);
	RID joint_rid = joint_owner.make_rid(joint);
	joint->set_self(joint_rid);
	return joint_rid;
}

void GodotPhysicsServer2D::joint_clear(RID p_joint)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	if (joint->get_type() != PS2DE::JOINT_TYPE_MAX) {
		GodotJoint2D* empty_joint = memnew(GodotJoint2D);
		empty_joint->copy_settings_from(joint);

		joint_owner.replace(p_joint, empty_joint);
		memdelete(joint);
	}
}

void GodotPhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	switch (p_param) {
	case PS2DE::JOINT_PARAM_BIAS:
		joint->set_bias(p_value);
		break;
	case PS2DE::JOINT_PARAM_MAX_BIAS:
		joint->set_max_bias(p_value);
		break;
	case PS2DE::JOINT_PARAM_MAX_FORCE:
		joint->set_max_force(p_value);
		break;
	}
}

real_t GodotPhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param) const
{
	const GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, -1);

	switch (p_param) {
	case PS2DE::JOINT_PARAM_BIAS:
		return joint->get_bias();
		break;
	case PS2DE::JOINT_PARAM_MAX_BIAS:
		return joint->get_max_bias();
		break;
	case PS2DE::JOINT_PARAM_MAX_FORCE:
		return joint->get_max_force();
		break;
	}

	return 0;
}

void GodotPhysicsServer2D::joint_disable_collisions_between_bodies(
	RID p_joint, const bool p_disable)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	joint->disable_collisions_between_bodies(p_disable);

	if (2 == joint->get_body_count()) {
		GodotBody2D* body_a = *joint->get_body_ptr();
		GodotBody2D* body_b = *(joint->get_body_ptr() + 1);

		if (p_disable) {
			body_add_collision_exception(body_a->get_self(), body_b->get_self());
			body_add_collision_exception(body_b->get_self(), body_a->get_self());
		}
		else {
			body_remove_collision_exception(body_a->get_self(), body_b->get_self());
			body_remove_collision_exception(body_b->get_self(), body_a->get_self());
		}
	}
}

bool GodotPhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint) const
{
	const GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, true);

	return joint->is_disabled_collisions_between_bodies();
}

void GodotPhysicsServer2D::joint_make_pin(
	RID p_joint, const Vector2& p_pos, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);
	GodotBody2D* B = nullptr;
	if (body_owner.owns(p_body_b)) {
		B = body_owner.get_or_null(p_body_b);
		ERR_FAIL_NULL(B);
	}

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_pos, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1,
	const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);

	GodotBody2D* B = body_owner.get_or_null(p_body_b);
	ERR_FAIL_NULL(B);

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_a_groove1, p_a_groove2, p_b_anchor, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::joint_make_damped_spring(
	RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);

	GodotBody2D* B = body_owner.get_or_null(p_body_b);
	ERR_FAIL_NULL(B);

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_anchor_a, p_anchor_b, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::pin_joint_set_flag(
	RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_PIN);

	joint->set_flag(p_flag, p_enabled);
}

bool GodotPhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_PIN, false);

	return joint->get_flag(p_flag);
}

void GodotPhysicsServer2D::pin_joint_set_param(
	RID p_joint, PS2DE::PinJointParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_PIN);

	joint->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_PIN, 0);

	return joint->get_param(p_param);
}

void GodotPhysicsServer2D::damped_spring_joint_set_param(
	RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_DAMPED_SPRING);

	joint->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::damped_spring_joint_get_param(
	RID p_joint, PS2DE::DampedSpringParam p_param) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);

	return joint->get_param(p_param);
}

PS2DE::JointType GodotPhysicsServer2D::joint_get_type(RID p_joint) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, PS2DE::JOINT_TYPE_PIN);

	return joint->get_type();
}

void GodotPhysicsServer2D::free_rid(RID p_rid)
{
	_update_shapes(); // just in case

	if (shape_owner.owns(p_rid)) {
		GodotShape2D* shape = shape_owner.get_or_null(p_rid);

		while (shape->get_owners().size()) {
			GodotCollisionObject2D* so = shape->get_owners().begin()->key;
			so->remove_shape(shape);
		}

		shape_owner.free(p_rid);
		memdelete(shape);
	}
	else if (body_owner.owns(p_rid)) {
		GodotBody2D* body = body_owner.get_or_null(p_rid);

		body_set_space(p_rid, RID());

		while (body->get_shape_count()) {
			body->remove_shape(0);
		}

		body_owner.free(p_rid);
		memdelete(body);

	}
	else if (area_owner.owns(p_rid)) {
		GodotArea2D* area = area_owner.get_or_null(p_rid);

		area->set_space(nullptr);

		while (area->get_shape_count()) {
			area->remove_shape(0);
		}

		area_owner.free(p_rid);
		memdelete(area);
	}
	else if (space_owner.owns(p_rid)) {
		GodotSpace2D* space = space_owner.get_or_null(p_rid);

		while (space->get_objects().size()) {
			GodotCollisionObject2D* co =
				static_cast<GodotCollisionObject2D*>(*space->get_objects().begin());
			co->set_space(nullptr);
		}

		active_spaces.erase(space);
		free_rid(space->get_default_area()->get_self());
		space_owner.free(p_rid);
		memdelete(space);
	}
	else if (joint_owner.owns(p_rid)) {
		GodotJoint2D* joint = joint_owner.get_or_null(p_rid);

		joint_owner.free(p_rid);
		memdelete(joint);

	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

void GodotPhysicsServer2D::set_active(bool p_active) { active = p_active; }

void GodotPhysicsServer2D::init()
{
	doing_sync = false;
	if (!stepper) {
		stepper = memnew(GodotStep2D);
	}
}

void GodotPhysicsServer2D::step(real_t p_step)
{
	if (!active || !stepper) {
		return;
	}

	_update_shapes();

	island_count = 0;
	active_objects = 0;
	collision_pairs = 0;
	for (GodotSpace2D* E : active_spaces) {
		stepper->step(E, p_step);
		island_count += E->get_island_count();
		active_objects += E->get_active_objects();
		collision_pairs += E->get_collision_pairs();
	}
}

void GodotPhysicsServer2D::sync() { doing_sync = true; }

void GodotPhysicsServer2D::end_sync() { doing_sync = false; }

void GodotPhysicsServer2D::finish()
{
	if (stepper) {
		memdelete(stepper);
		stepper = nullptr;
	}
}

void GodotPhysicsServer2D::_update_shapes()
{
	while (pending_shape_update_list.first()) {
		pending_shape_update_list.first()->self()->_shape_changed();
		pending_shape_update_list.remove(pending_shape_update_list.first());
	}
}

int GodotPhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info)
{
	switch (p_info) {
	case PS2DE::INFO_ACTIVE_OBJECTS: {
		return active_objects;
	} break;
	case PS2DE::INFO_COLLISION_PAIRS: {
		return collision_pairs;
	} break;
	case PS2DE::INFO_ISLAND_COUNT: {
		return island_count;
	} break;
	}

	return 0;
}

GodotPhysicsServer2D::GodotPhysicsServer2D(bool p_using_threads)
{
	using_threads = p_using_threads;
}

void GodotPhysicsServer2D::flush_queries() {}



// ============================================================================
// Merged from modules/godot_physics_2d/register_types.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#include "modules/register_module_types.h"
#include "servers/physics_2d/physics_server_2d.h"

void uninitialize_godot_physics_2d_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
}

void initialize_godot_physics_2d_module(ModuleInitializationLevel p_level) {}



// ============================================================================
// Merged from modules/godot_physics_2d/godot_space_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"

// ============================================================================
// GodotBroadPhase2D
// ============================================================================

GodotBroadPhase2D::ID GodotBroadPhase2D::create(GodotCollisionObject2D* p_object, int p_subindex, const Rect2& p_aabb, bool p_static)
{
	uint32_t tree_id = p_static ? TREE_STATIC : TREE_DYNAMIC;
	uint32_t tree_collision_mask = p_static ? TREE_FLAG_DYNAMIC : (TREE_FLAG_STATIC | TREE_FLAG_DYNAMIC);
	ID oid = bvh.create(p_object, true, tree_id, tree_collision_mask, p_aabb, p_subindex);
	return oid + 1;
}

void GodotBroadPhase2D::move(ID p_id, const Rect2& p_aabb)
{
	ERR_FAIL_COND(!p_id);
	bvh.move(p_id - 1, p_aabb);
}

void GodotBroadPhase2D::set_static(ID p_id, bool p_static)
{
	ERR_FAIL_COND(!p_id);
	uint32_t tree_id = p_static ? TREE_STATIC : TREE_DYNAMIC;
	uint32_t tree_collision_mask = p_static ? TREE_FLAG_DYNAMIC : (TREE_FLAG_STATIC | TREE_FLAG_DYNAMIC);
	bvh.set_tree(p_id - 1, tree_id, tree_collision_mask, false);
}

void GodotBroadPhase2D::remove(ID p_id)
{
	ERR_FAIL_COND(!p_id);
	bvh.erase(p_id - 1);
}

GodotCollisionObject2D* GodotBroadPhase2D::get_object(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, nullptr);
	GodotCollisionObject2D* it = bvh.get(p_id - 1);
	ERR_FAIL_NULL_V(it, nullptr);
	return it;
}

bool GodotBroadPhase2D::is_static(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, false);
	uint32_t tree_id = bvh.get_tree_id(p_id - 1);
	return tree_id == 0;
}

int GodotBroadPhase2D::get_subindex(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, 0);
	return bvh.get_subindex(p_id - 1);
}

int GodotBroadPhase2D::cull_segment(const Vector2& p_from, const Vector2& p_to, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices)
{
	return bvh.cull_segment(p_from, p_to, p_results, p_max_results, nullptr, 0xFFFFFFFF, p_result_indices);
}

int GodotBroadPhase2D::cull_aabb(const Rect2& p_aabb, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices)
{
	return bvh.cull_aabb(p_aabb, p_results, p_max_results, nullptr, 0xFFFFFFFF, p_result_indices);
}

void* GodotBroadPhase2D::_pair_callback(void* self, uint32_t p_A, GodotCollisionObject2D* p_object_A, int subindex_A, uint32_t p_B, GodotCollisionObject2D* p_object_B, int subindex_B)
{
	GodotBroadPhase2D* bpo = static_cast<GodotBroadPhase2D*>(self);
	if (!bpo->pair_callback) {
		return nullptr;
	}

	return bpo->pair_callback(p_object_A, subindex_A, p_object_B, subindex_B, bpo->pair_userdata);
}

void GodotBroadPhase2D::_unpair_callback(void* self, uint32_t p_A, GodotCollisionObject2D* p_object_A, int subindex_A, uint32_t p_B, GodotCollisionObject2D* p_object_B, int subindex_B, void* pairdata)
{
	GodotBroadPhase2D* bpo = static_cast<GodotBroadPhase2D*>(self);
	if (!bpo->unpair_callback) {
		return;
	}

	bpo->unpair_callback(p_object_A, subindex_A, p_object_B, subindex_B, pairdata, bpo->unpair_userdata);
}

void GodotBroadPhase2D::set_pair_callback(PairCallback p_pair_callback, void* p_userdata)
{
	pair_callback = p_pair_callback;
	pair_userdata = p_userdata;
}

void GodotBroadPhase2D::set_unpair_callback(UnpairCallback p_unpair_callback, void* p_userdata)
{
	unpair_callback = p_unpair_callback;
	unpair_userdata = p_userdata;
}

void GodotBroadPhase2D::update()
{
	bvh.update();
}

GodotBroadPhase2D::GodotBroadPhase2D()
{
	bvh.set_pair_callback(_pair_callback, this);
	bvh.set_unpair_callback(_unpair_callback, this);
}

// ============================================================================
// PhysicsDirectSpaceState2D / GodotSpace2D
// ============================================================================

#define TEST_MOTION_MARGIN_MIN_VALUE 0.0001
#define TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR 0.05

_FORCE_INLINE_ static bool _can_collide_with(GodotCollisionObject2D* p_object,
	uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas)
{
	if (!(p_object->get_collision_layer() & p_collision_mask)) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject2D::TYPE_AREA && !p_collide_with_areas) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject2D::TYPE_BODY && !p_collide_with_bodies) {
		return false;
	}

	return true;
}

PhysicsDirectSpaceState2D::PhysicsDirectSpaceState2D(GodotSpace2D* p_space) : space(p_space) {}

int PhysicsDirectSpaceState2D::intersect_point(
	const PS2DT::PointParameters& p_parameters, PS2DT::ShapeResult* r_results, int p_result_max)
{
	if (p_result_max <= 0) {
		return 0;
	}

	Rect2 aabb;
	aabb.position = p_parameters.position - Vector2(0.00001, 0.00001);
	aabb.size = Vector2(0.00002, 0.00002);

	int amount = space->broadphase->cull_aabb(aabb, space->intersection_query_results,
		GodotSpace2D::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	int cc = 0;

	for (int i = 0; i < amount; i++) {
		if (!_can_collide_with(space->intersection_query_results[i], p_parameters.collision_mask,
				p_parameters.collide_with_bodies, p_parameters.collide_with_areas)) {
			continue;
		}

		if (p_parameters.exclude.has(space->intersection_query_results[i]->get_self())) {
			continue;
		}

		const GodotCollisionObject2D* col_obj = space->intersection_query_results[i];

		if (p_parameters.pick_point && !col_obj->is_pickable()) {
			continue;
		}

		int shape_idx = space->intersection_query_subindex_results[i];

		GodotShape2D* shape = col_obj->get_shape(shape_idx);

		Vector2 local_point = (col_obj->get_transform() * col_obj->get_shape_transform(shape_idx))
								  .affine_inverse()
								  .xform(p_parameters.position);

		if (!shape->contains_point(local_point)) {
			continue;
		}

		if (cc >= p_result_max) {
			continue;
		}

		r_results[cc].rid = col_obj->get_self();
		r_results[cc].shape = shape_idx;

		cc++;
	}

	return cc;
}

bool PhysicsDirectSpaceState2D::intersect_ray(
	const PS2DT::RayParameters& p_parameters, PS2DT::RayResult& r_result)
{
	ERR_FAIL_COND_V(space->locked, false);

	Vector2 begin, end;
	Vector2 normal;
	begin = p_parameters.from;
	end = p_parameters.to;
	normal = (end - begin).normalized();

	int amount = space->broadphase->cull_segment(begin, end, space->intersection_query_results,
		GodotSpace2D::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	bool collided = false;
	Vector2 res_point, res_normal;
	int res_shape = -1;
	const GodotCollisionObject2D* res_obj = nullptr;
	real_t min_d = 1e10;

	for (int i = 0; i < amount; i++) {
		if (!_can_collide_with(space->intersection_query_results[i], p_parameters.collision_mask,
				p_parameters.collide_with_bodies, p_parameters.collide_with_areas)) {
			continue;
		}

		if (p_parameters.exclude.has(space->intersection_query_results[i]->get_self())) {
			continue;
		}

		const GodotCollisionObject2D* col_obj = space->intersection_query_results[i];

		int shape_idx = space->intersection_query_subindex_results[i];
		Transform2D inv_xform =
			col_obj->get_shape_inv_transform(shape_idx) * col_obj->get_inv_transform();

		Vector2 local_from = inv_xform.xform(begin);
		Vector2 local_to = inv_xform.xform(end);

		const GodotShape2D* shape = col_obj->get_shape(shape_idx);

		Vector2 shape_point, shape_normal;

		if (shape->contains_point(local_from)) {
			if (p_parameters.hit_from_inside) {
				min_d = 0;
				res_point = begin;
				res_normal = Vector2();
				res_shape = shape_idx;
				res_obj = col_obj;
				collided = true;
				break;
			}
			else {
				continue;
			}
		}

		if (shape->intersect_segment(local_from, local_to, shape_point, shape_normal)) {
			Transform2D xform = col_obj->get_transform() * col_obj->get_shape_transform(shape_idx);
			shape_point = xform.xform(shape_point);

			real_t ld = normal.dot(shape_point);

			if (ld < min_d) {
				min_d = ld;
				res_point = shape_point;
				res_normal = inv_xform.basis_xform_inv(shape_normal).normalized();
				res_shape = shape_idx;
				res_obj = col_obj;
				collided = true;
			}
		}
	}

	if (!collided) {
		return false;
	}
	ERR_FAIL_NULL_V(res_obj, false);

	r_result.normal = res_normal;
	r_result.position = res_point;
	r_result.rid = res_obj->get_self();
	r_result.shape = res_shape;

	return true;
}

struct _RestCallbackData2D
{
	const GodotCollisionObject2D* object = nullptr;
	const GodotCollisionObject2D* best_object = nullptr;
	int local_shape = 0;
	int best_local_shape = 0;
	int shape = 0;
	int best_shape = 0;
	Vector2 best_contact;
	Vector2 best_normal;
	real_t best_len = 0.0;
	Vector2 valid_dir;
	real_t valid_depth = 0.0;
	real_t min_allowed_depth = 0.0;
};

static void _rest_cbk_result(const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata)
{
	_RestCallbackData2D* rd = static_cast<_RestCallbackData2D*>(p_userdata);

	Vector2 contact_rel = p_point_B - p_point_A;
	real_t len = contact_rel.length();

	if (len < rd->min_allowed_depth) {
		return;
	}

	if (len <= rd->best_len) {
		return;
	}

	Vector2 normal = contact_rel / len;

	if (rd->valid_dir != Vector2()) {
		if (len > rd->valid_depth) {
			return;
		}

		if (rd->valid_dir.dot(normal) > -CMP_EPSILON) {
			return;
		}
	}

	rd->best_len = len;
	rd->best_contact = p_point_B;
	rd->best_normal = normal;
	rd->best_object = rd->object;
	rd->best_shape = rd->shape;
	rd->best_local_shape = rd->local_shape;
}

int GodotSpace2D::_cull_aabb_for_body(GodotBody2D* p_body, const Rect2& p_aabb)
{
	int amount = broadphase->cull_aabb(p_aabb, intersection_query_results, INTERSECTION_QUERY_MAX,
		intersection_query_subindex_results);

	for (int i = 0; i < amount; i++) {
		bool keep = true;

		if (intersection_query_results[i] == p_body) {
			keep = false;
		}
		else if (intersection_query_results[i]->get_type() == GodotCollisionObject2D::TYPE_AREA) {
			keep = false;
		}
		else if (!p_body->collides_with(
					   static_cast<GodotBody2D*>(intersection_query_results[i]))) {
			keep = false;
		}
		else if (static_cast<GodotBody2D*>(intersection_query_results[i])
					   ->has_exception(p_body->get_self()) ||
				   p_body->has_exception(intersection_query_results[i]->get_self())) {
			keep = false;
		}

		if (!keep) {
			if (i < amount - 1) {
				SWAP(intersection_query_results[i], intersection_query_results[amount - 1]);
				SWAP(intersection_query_subindex_results[i],
					intersection_query_subindex_results[amount - 1]);
			}

			amount--;
			i--;
		}
	}

	return amount;
}

void* GodotSpace2D::_broadphase_pair(GodotCollisionObject2D* A, int p_subindex_A,
	GodotCollisionObject2D* B, int p_subindex_B, void* p_self)
{
	GodotCollisionObject2D::Type type_A = A->get_type();
	GodotCollisionObject2D::Type type_B = B->get_type();
	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(p_subindex_A, p_subindex_B);
		SWAP(type_A, type_B);
	}

	GodotSpace2D* self = static_cast<GodotSpace2D*>(p_self);
	self->collision_pairs++;

	if (type_A == GodotCollisionObject2D::TYPE_AREA) {
		GodotArea2D* area = static_cast<GodotArea2D*>(A);
		if (type_B == GodotCollisionObject2D::TYPE_AREA) {
			GodotArea2D* area_b = static_cast<GodotArea2D*>(B);
			GodotArea2Pair2D* area2_pair =
				memnew(GodotArea2Pair2D(area_b, p_subindex_B, area, p_subindex_A));
			return area2_pair;
		}
		else {
			GodotBody2D* body = static_cast<GodotBody2D*>(B);
			GodotAreaPair2D* area_pair =
				memnew(GodotAreaPair2D(body, p_subindex_B, area, p_subindex_A));
			return area_pair;
		}

	}
	else {
		GodotBodyPair2D* b = memnew(GodotBodyPair2D(static_cast<GodotBody2D*>(A), p_subindex_A,
			static_cast<GodotBody2D*>(B), p_subindex_B));
		return b;
	}
}

void GodotSpace2D::_broadphase_unpair(GodotCollisionObject2D* A, int p_subindex_A,
	GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_self)
{
	if (!p_data) {
		return;
	}

	GodotSpace2D* self = static_cast<GodotSpace2D*>(p_self);
	self->collision_pairs--;
	GodotConstraint2D* c = static_cast<GodotConstraint2D*>(p_data);
	memdelete(c);
}

const SelfList<GodotBody2D>::List& GodotSpace2D::get_active_body_list() const
{
	return active_list;
}

void GodotSpace2D::body_add_to_active_list(SelfList<GodotBody2D>* p_body)
{
	active_list.add(p_body);
}

void GodotSpace2D::body_remove_from_active_list(SelfList<GodotBody2D>* p_body)
{
	active_list.remove(p_body);
}

void GodotSpace2D::body_add_to_mass_properties_update_list(SelfList<GodotBody2D>* p_body)
{
	mass_properties_update_list.add(p_body);
}

void GodotSpace2D::body_remove_from_mass_properties_update_list(SelfList<GodotBody2D>* p_body)
{
	mass_properties_update_list.remove(p_body);
}

GodotBroadPhase2D* GodotSpace2D::get_broadphase() { return broadphase; }

void GodotSpace2D::add_object(GodotCollisionObject2D* p_object)
{
	ERR_FAIL_COND(objects.has(p_object));
	objects.insert(p_object);
}

void GodotSpace2D::remove_object(GodotCollisionObject2D* p_object)
{
	ERR_FAIL_COND(!objects.has(p_object));
	objects.erase(p_object);
}

const HashSet<GodotCollisionObject2D*>& GodotSpace2D::get_objects() const { return objects; }

void GodotSpace2D::body_add_to_state_query_list(SelfList<GodotBody2D>* p_body)
{
	state_query_list.add(p_body);
}

void GodotSpace2D::body_remove_from_state_query_list(SelfList<GodotBody2D>* p_body)
{
	state_query_list.remove(p_body);
}

void GodotSpace2D::area_add_to_monitor_query_list(SelfList<GodotArea2D>* p_area)
{
	monitor_query_list.add(p_area);
}

void GodotSpace2D::area_remove_from_monitor_query_list(SelfList<GodotArea2D>* p_area)
{
	monitor_query_list.remove(p_area);
}

void GodotSpace2D::area_add_to_moved_list(SelfList<GodotArea2D>* p_area)
{
	area_moved_list.add(p_area);
}

void GodotSpace2D::area_remove_from_moved_list(SelfList<GodotArea2D>* p_area)
{
	area_moved_list.remove(p_area);
}

const SelfList<GodotArea2D>::List& GodotSpace2D::get_moved_area_list() const
{
	return area_moved_list;
}

void GodotSpace2D::call_queries()
{
	while (state_query_list.first()) {
		GodotBody2D* b = state_query_list.first()->self();
		state_query_list.remove(state_query_list.first());
		b->call_queries();
	}

	while (monitor_query_list.first()) {
		GodotArea2D* a = monitor_query_list.first()->self();
		monitor_query_list.remove(monitor_query_list.first());
		a->call_queries();
	}
}

void GodotSpace2D::setup()
{
	contact_debug_count = 0;

	while (mass_properties_update_list.first()) {
		mass_properties_update_list.first()->self()->update_mass_properties();
		mass_properties_update_list.remove(mass_properties_update_list.first());
	}
}

void GodotSpace2D::update() { broadphase->update(); }

void GodotSpace2D::set_param(PS2DE::SpaceParameter p_param, real_t p_value)
{
	switch (p_param) {
	case PS2DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		contact_recycle_radius = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		contact_max_separation = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		contact_max_allowed_penetration = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		contact_bias = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		body_linear_velocity_sleep_threshold = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		body_angular_velocity_sleep_threshold = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		body_time_to_sleep = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
		constraint_bias = p_value;
		break;
	case PS2DE::SPACE_PARAM_SOLVER_ITERATIONS:
		solver_iterations = p_value;
		break;
	}
}

real_t GodotSpace2D::get_param(PS2DE::SpaceParameter p_param) const
{
	switch (p_param) {
	case PS2DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		return contact_recycle_radius;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		return contact_max_separation;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		return contact_max_allowed_penetration;
	case PS2DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		return contact_bias;
	case PS2DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		return body_linear_velocity_sleep_threshold;
	case PS2DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		return body_angular_velocity_sleep_threshold;
	case PS2DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		return body_time_to_sleep;
	case PS2DE::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
		return constraint_bias;
	case PS2DE::SPACE_PARAM_SOLVER_ITERATIONS:
		return solver_iterations;
	}
	return 0;
}

void GodotSpace2D::lock() { locked = true; }

void GodotSpace2D::unlock() { locked = false; }

bool GodotSpace2D::is_locked() const { return locked; }

PhysicsDirectSpaceState2D* GodotSpace2D::get_direct_state() { return direct_access; }

GodotSpace2D::GodotSpace2D()
{
	body_linear_velocity_sleep_threshold = 2.0;
	body_angular_velocity_sleep_threshold = Math::deg_to_rad(8.0);
	body_time_to_sleep = 0.5;
	solver_iterations = 16;
	contact_recycle_radius = 1.0;
	contact_max_separation = 1.5;
	contact_max_allowed_penetration = 0.3;
	contact_bias = 0.8;
	constraint_bias = 0.2;

	broadphase = memnew(GodotBroadPhase2D);
	broadphase->set_pair_callback(_broadphase_pair, this);
	broadphase->set_unpair_callback(_broadphase_unpair, this);

	direct_access = memnew(PhysicsDirectSpaceState2D(this));
}

GodotSpace2D::~GodotSpace2D()
{
	memdelete(broadphase);
	memdelete(direct_access);
}

bool GodotSpace2D::test_body_motion(GodotBody2D* p_body,
	const PS2DT::MotionParameters& p_parameters,
	PS2DT::MotionResult* r_result)
{
	return false;
}

int PhysicsDirectSpaceState2D::intersect_shape(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState2D::cast_motion(
	const PS2DT::ShapeParameters& p_parameters, real_t& p_closest_safe,
	real_t& p_closest_unsafe)
{
	return false;
}

bool PhysicsDirectSpaceState2D::collide_shape(
	const PS2DT::ShapeParameters& p_parameters, Vector2* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState2D::rest_info(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeRestInfo* r_info)
{
	return false;
}

// ============================================================================
// GodotStep2D
// ============================================================================

#define BODY_ISLAND_COUNT_RESERVE 128
#define BODY_ISLAND_SIZE_RESERVE 512
#define ISLAND_COUNT_RESERVE 128
#define ISLAND_SIZE_RESERVE 512
#define CONSTRAINT_COUNT_RESERVE 1024

void GodotStep2D::_populate_island(GodotBody2D* p_body, LocalVector<GodotBody2D*>& p_body_island,
	LocalVector<GodotConstraint2D*>& p_constraint_island)
{
	p_body->set_island_step(_step);

	if (p_body->get_mode() > PS2DE::BODY_MODE_KINEMATIC) {
		p_body_island.push_back(p_body);
	}

	for (const Pair<GodotConstraint2D*, int>& E : p_body->get_constraint_list()) {
		GodotConstraint2D* constraint = E.first;
		if (constraint->get_island_step() == _step) {
			continue;
		}
		constraint->set_island_step(_step);
		p_constraint_island.push_back(constraint);
		all_constraints.push_back(constraint);

		for (int i = 0; i < constraint->get_body_count(); i++) {
			if (i == E.second) {
				continue;
			}
			GodotBody2D* other_body = constraint->get_body_ptr()[i];
			if (other_body->get_island_step() == _step) {
				continue;
			}
			if (other_body->get_mode() == PS2DE::BODY_MODE_STATIC) {
				continue;
			}
			_populate_island(other_body, p_body_island, p_constraint_island);
		}
	}
}

void GodotStep2D::_setup_constraint(uint32_t p_constraint_index, void* p_userdata)
{
	GodotConstraint2D* constraint = all_constraints[p_constraint_index];
	constraint->setup(delta);
}

void GodotStep2D::_pre_solve_island(LocalVector<GodotConstraint2D*>& p_constraint_island) const
{
	uint32_t constraint_count = p_constraint_island.size();
	uint32_t valid_constraint_count = 0;
	for (uint32_t constraint_index = 0; constraint_index < constraint_count; ++constraint_index) {
		GodotConstraint2D* constraint = p_constraint_island[constraint_index];
		if (p_constraint_island[constraint_index]->pre_solve(delta)) {
			p_constraint_island[valid_constraint_count++] = constraint;
		}
	}
	p_constraint_island.resize(valid_constraint_count);
}

void GodotStep2D::_solve_island(uint32_t p_island_index, void* p_userdata) const
{
	const LocalVector<GodotConstraint2D*>& constraint_island = constraint_islands[p_island_index];

	for (int i = 0; i < iterations; i++) {
		uint32_t constraint_count = constraint_island.size();
		for (uint32_t constraint_index = 0; constraint_index < constraint_count;
			 ++constraint_index) {
			constraint_island[constraint_index]->solve(delta);
		}
	}
}

void GodotStep2D::_check_suspend(LocalVector<GodotBody2D*>& p_body_island) const
{
	bool can_sleep = true;

	uint32_t body_count = p_body_island.size();
	for (uint32_t body_index = 0; body_index < body_count; ++body_index) {
		GodotBody2D* body = p_body_island[body_index];

		if (!body->sleep_test(delta)) {
			can_sleep = false;
		}
	}

	// Put all to sleep or wake up everyone.
	for (uint32_t body_index = 0; body_index < body_count; ++body_index) {
		GodotBody2D* body = p_body_island[body_index];

		bool active = body->is_active();

		if (active == can_sleep) {
			body->set_active(!can_sleep);
		}
	}
}

void GodotStep2D::step(GodotSpace2D* p_space, real_t p_delta)
{
	p_space->lock();

	p_space->setup();

	p_space->set_last_step(p_delta);

	iterations = p_space->get_solver_iterations();
	delta = p_delta;

	const SelfList<GodotBody2D>::List* body_list = &p_space->get_active_body_list();

	/* INTEGRATE FORCES */

	uint64_t profile_begtime = OS::get_ticks_usec();
	uint64_t profile_endtime = 0;

	int active_count = 0;

	const SelfList<GodotBody2D>* b = body_list->first();
	while (b) {
		b->self()->integrate_forces(p_delta);
		b = b->next();
		active_count++;
	}

	p_space->set_active_objects(active_count);

	p_space->update();

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_INTEGRATE_FORCES, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* GENERATE CONSTRAINT ISLANDS FOR MOVING AREAS */

	uint32_t island_count = 0;

	const SelfList<GodotArea2D>::List& aml = p_space->get_moved_area_list();

	while (aml.first()) {
		for (GodotConstraint2D* E : aml.first()->self()->get_constraints()) {
			GodotConstraint2D* constraint = E;
			if (constraint->get_island_step() == _step) {
				continue;
			}
			constraint->set_island_step(_step);

			++island_count;
			if (constraint_islands.size() < island_count) {
				constraint_islands.resize(island_count);
			}
			LocalVector<GodotConstraint2D*>& constraint_island =
				constraint_islands[island_count - 1];
			constraint_island.clear();

			all_constraints.push_back(constraint);
			constraint_island.push_back(constraint);
		}
		p_space->area_remove_from_moved_list(
			(SelfList<GodotArea2D>*)aml.first());
	}

	/* GENERATE CONSTRAINT ISLANDS FOR ACTIVE RIGID BODIES */

	b = body_list->first();

	uint32_t body_island_count = 0;

	while (b) {
		GodotBody2D* body = b->self();

		if (body->get_island_step() != _step) {
			++body_island_count;
			if (body_islands.size() < body_island_count) {
				body_islands.resize(body_island_count);
			}
			LocalVector<GodotBody2D*>& body_island = body_islands[body_island_count - 1];
			body_island.clear();
			body_island.reserve(BODY_ISLAND_SIZE_RESERVE);

			++island_count;
			if (constraint_islands.size() < island_count) {
				constraint_islands.resize(island_count);
			}
			LocalVector<GodotConstraint2D*>& constraint_island =
				constraint_islands[island_count - 1];
			constraint_island.clear();
			constraint_island.reserve(ISLAND_SIZE_RESERVE);

			_populate_island(body, body_island, constraint_island);

			if (body_island.is_empty()) {
				--body_island_count;
			}

			if (constraint_island.is_empty()) {
				--island_count;
			}
		}
		b = b->next();
	}

	p_space->set_island_count((int)island_count);

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_GENERATE_ISLANDS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* SETUP CONSTRAINTS / PROCESS COLLISIONS */

	uint32_t total_constraint_count = all_constraints.size();
	(void)total_constraint_count;

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_SETUP_CONSTRAINTS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* PRE-SOLVE CONSTRAINT ISLANDS */

	for (uint32_t island_index = 0; island_index < island_count; ++island_index) {
		_pre_solve_island(constraint_islands[island_index]);
	}

	/* SOLVE CONSTRAINT ISLANDS */

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_SOLVE_CONSTRAINTS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* INTEGRATE VELOCITIES */

	b = body_list->first();
	while (b) {
		const SelfList<GodotBody2D>* n = b->next();
		b->self()->integrate_velocities(p_delta);
		b = n;
	}

	/* SLEEP / WAKE UP ISLANDS */

	for (uint32_t island_index = 0; island_index < body_island_count; ++island_index) {
		_check_suspend(body_islands[island_index]);
	}

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_INTEGRATE_VELOCITIES, profile_endtime - profile_begtime);
	}

	all_constraints.clear();

	p_space->unlock();
	_step++;
}

GodotStep2D::GodotStep2D()
{
	body_islands.reserve(BODY_ISLAND_COUNT_RESERVE);
	constraint_islands.reserve(ISLAND_COUNT_RESERVE);
	all_constraints.reserve(CONSTRAINT_COUNT_RESERVE);
}

GodotStep2D::~GodotStep2D() {}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_objects_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "servers/physics_2d/physics_server_2d.h"

// --- GodotCollisionObject2D ---

void GodotCollisionObject2D::add_shape(GodotShape2D *p_shape, const Transform2D &p_transform, bool p_disabled) {
	Shape s;
	s.shape = p_shape;
	s.xform = p_transform;
	s.xform_inv = s.xform.affine_inverse();
	s.bpid = 0; //needs update
	s.disabled = p_disabled;
	s.one_way_collision = false;
	s.one_way_collision_margin = 0;
	s.one_way_collision_direction = Vector2(0, 1);
	shapes.push_back(s);
	p_shape->add_owner(this);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape(int p_index, GodotShape2D *p_shape) {
	ERR_FAIL_INDEX(p_index, shapes.size());
	shapes[p_index].shape->remove_owner(this);
	shapes.write[p_index].shape = p_shape;

	p_shape->add_owner(this);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape_transform(int p_index, const Transform2D &p_transform) {
	ERR_FAIL_INDEX(p_index, shapes.size());

	shapes.write[p_index].xform = p_transform;
	shapes.write[p_index].xform_inv = p_transform.affine_inverse();

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape_disabled(int p_idx, bool p_disabled) {
	ERR_FAIL_INDEX(p_idx, shapes.size());

	GodotCollisionObject2D::Shape &shape = shapes.write[p_idx];
	if (shape.disabled == p_disabled) {
		return;
	}

	shape.disabled = p_disabled;

	if (!space) {
		return;
	}

	if (p_disabled && shape.bpid != 0) {
		space->get_broadphase()->remove(shape.bpid);
		shape.bpid = 0;
		if (!pending_shape_update_list.in_list()) {
			if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
				server->pending_shape_update_list.add(&pending_shape_update_list);
			}
		}
	} else if (!p_disabled && shape.bpid == 0) {
		if (!pending_shape_update_list.in_list()) {
			if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
				server->pending_shape_update_list.add(&pending_shape_update_list);
			}
		}
	}
}

void GodotCollisionObject2D::remove_shape(GodotShape2D *p_shape) {
	//remove a shape, all the times it appears
	for (int i = 0; i < shapes.size(); i++) {
		if (shapes[i].shape == p_shape) {
			remove_shape(i);
			i--;
		}
	}
}

void GodotCollisionObject2D::remove_shape(int p_index) {
	//remove anything from shape to be erased to end, so subindices don't change
	ERR_FAIL_INDEX(p_index, shapes.size());
	for (int i = p_index; i < shapes.size(); i++) {
		if (shapes[i].bpid == 0) {
			continue;
		}
		//should never get here with a null owner
		space->get_broadphase()->remove(shapes[i].bpid);
		shapes.write[i].bpid = 0;
	}
	shapes[p_index].shape->remove_owner(this);
	shapes.remove_at(p_index);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::_set_static(bool p_static) {
	if (_static == p_static) {
		return;
	}
	_static = p_static;

	if (!space) {
		return;
	}
	for (int i = 0; i < get_shape_count(); i++) {
		const Shape &s = shapes[i];
		if (s.bpid > 0) {
			space->get_broadphase()->set_static(s.bpid, _static);
		}
	}
}

void GodotCollisionObject2D::_unregister_shapes() {
	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.bpid > 0) {
			space->get_broadphase()->remove(s.bpid);
			s.bpid = 0;
		}
	}
}

void GodotCollisionObject2D::_update_shapes() {
	if (!space) {
		return;
	}

	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.disabled) {
			continue;
		}

		//not quite correct, should compute the next matrix..
		Rect2 shape_aabb = s.shape->get_aabb();
		Transform2D xform = transform * s.xform;
		shape_aabb = xform.xform(shape_aabb);
		shape_aabb.grow_by((s.aabb_cache.size.x + s.aabb_cache.size.y) * 0.5 * 0.05);
		s.aabb_cache = shape_aabb;

		if (s.bpid == 0) {
			s.bpid = space->get_broadphase()->create(this, i, shape_aabb, _static);
			space->get_broadphase()->set_static(s.bpid, _static);
		}

		space->get_broadphase()->move(s.bpid, shape_aabb);
	}
}

void GodotCollisionObject2D::_update_shapes_with_motion(const Vector2 &p_motion) {
	if (!space) {
		return;
	}

	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.disabled) {
			continue;
		}

		//not quite correct, should compute the next matrix..
		Rect2 shape_aabb = s.shape->get_aabb();
		Transform2D xform = transform * s.xform;
		shape_aabb = xform.xform(shape_aabb);
		shape_aabb = shape_aabb.merge(Rect2(shape_aabb.position + p_motion, shape_aabb.size)); //use motion
		s.aabb_cache = shape_aabb;

		if (s.bpid == 0) {
			s.bpid = space->get_broadphase()->create(this, i, shape_aabb, _static);
			space->get_broadphase()->set_static(s.bpid, _static);
		}

		space->get_broadphase()->move(s.bpid, shape_aabb);
	}
}

void GodotCollisionObject2D::_set_space(GodotSpace2D *p_space) {
	GodotSpace2D *old_space = space;
	space = p_space;

	if (old_space) {
		old_space->remove_object(this);

		for (int i = 0; i < shapes.size(); i++) {
			Shape &s = shapes.write[i];
			if (s.bpid) {
				old_space->get_broadphase()->remove(s.bpid);
				s.bpid = 0;
			}
		}
	}

	if (space) {
		space->add_object(this);
		_update_shapes();
	}
}

void GodotCollisionObject2D::_shape_changed() {
	_update_shapes();
	_shapes_changed();
}

GodotCollisionObject2D::GodotCollisionObject2D(Type p_type) :
		pending_shape_update_list(this) {
	type = p_type;
}

// --- GodotArea2D ---

GodotArea2D::BodyKey::BodyKey(GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

GodotArea2D::BodyKey::BodyKey(GodotArea2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

void GodotArea2D::_shapes_changed()
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}
}

void GodotArea2D::set_transform(const Transform2D& p_transform)
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}

	_set_transform(p_transform);
	_set_inv_transform(p_transform.affine_inverse());
}

void GodotArea2D::set_space(GodotSpace2D* p_space)
{
	if (get_space()) {
		if (monitor_query_list.in_list()) {
			get_space()->area_remove_from_monitor_query_list(&monitor_query_list);
		}
		if (moved_list.in_list()) {
			get_space()->area_remove_from_moved_list(&moved_list);
		}
	}

	monitored_bodies.clear();
	monitored_areas.clear();

	_set_space(p_space);

	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}
}

void GodotArea2D::_set_space_override_mode(
	PS2DE::AreaSpaceOverrideMode& r_mode, PS2DE::AreaSpaceOverrideMode p_new_mode)
{
	bool do_override = p_new_mode != PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	if (do_override == (r_mode != PS2DE::AREA_SPACE_OVERRIDE_DISABLED)) {
		return;
	}
	_unregister_shapes();
	r_mode = p_new_mode;
	_shape_changed();
}

void GodotArea2D::_queue_monitor_update()
{
	ERR_FAIL_NULL(get_space());

	if (!monitor_query_list.in_list()) {
		get_space()->area_add_to_monitor_query_list(&monitor_query_list);
	}
}

void GodotArea2D::set_monitorable(bool p_monitorable)
{
	if (monitorable == p_monitorable) {
		return;
	}

	monitorable = p_monitorable;
	_set_static(!monitorable);
	_shapes_changed();
}

void GodotArea2D::compute_gravity(const Vector2& p_position, Vector2& r_gravity) const
{
	if (is_gravity_point()) {
		const real_t gr_unit_dist = get_gravity_point_unit_distance();
		Vector2 v = get_transform().xform(get_gravity_vector()) - p_position;
		if (gr_unit_dist > 0) {
			const real_t v_length_sq = v.length_squared();
			if (v_length_sq > 0) {
				const real_t gravity_strength =
					get_gravity() * gr_unit_dist * gr_unit_dist / v_length_sq;
				r_gravity = v.normalized() * gravity_strength;
			}
			else {
				r_gravity = Vector2();
			}
		}
		else {
			r_gravity = v.normalized() * get_gravity();
		}
	}
	else {
		r_gravity = get_gravity_vector() * get_gravity();
	}
}

GodotArea2D::GodotArea2D()
	: GodotCollisionObject2D(TYPE_AREA), monitor_query_list(this), moved_list(this)
{
	_set_static(true); // areas are not active by default
}

GodotArea2D::~GodotArea2D() {}

void GodotArea2D::call_queries() {}

// --- GodotBody2D ---

void GodotBody2D::_mass_properties_changed()
{
	if (get_space() && !mass_properties_update_list.in_list()) {
		get_space()->body_add_to_mass_properties_update_list(&mass_properties_update_list);
	}
}

void GodotBody2D::update_mass_properties()
{
	switch (mode) {
	case PS2DE::BODY_MODE_RIGID: {
		real_t total_area = 0;
		for (int i = 0; i < get_shape_count(); i++) {
			if (is_shape_disabled(i)) {
				continue;
			}
			total_area += get_shape_aabb(i).get_area();
		}

		if (calculate_center_of_mass) {
			center_of_mass_local = Vector2();

			if (total_area != 0.0) {
				for (int i = 0; i < get_shape_count(); i++) {
					if (is_shape_disabled(i)) {
						continue;
					}

					real_t area = get_shape_aabb(i).get_area();

					real_t mass_new = area * mass / total_area;

					// NOTE: we assume that the shape origin is also its center of mass.
					center_of_mass_local += mass_new * get_shape_transform(i).get_origin();
				}

				center_of_mass_local /= mass;
			}
		}

		if (calculate_inertia) {
			inertia = 0;

			for (int i = 0; i < get_shape_count(); i++) {
				if (is_shape_disabled(i)) {
					continue;
				}

				const GodotShape2D* shape = get_shape(i);

				real_t area = get_shape_aabb(i).get_area();
				if (area == 0.0) {
					continue;
				}

				real_t mass_new = area * mass / total_area;

				Transform2D mtx = get_shape_transform(i);
				Vector2 scale = mtx.get_scale();
				Vector2 shape_origin = mtx.get_origin() - center_of_mass_local;
				inertia += shape->get_moment_of_inertia(mass_new, scale) +
						   mass_new * shape_origin.length_squared();
			}
		}

		_inv_inertia = inertia > 0.0 ? (1.0 / inertia) : 0.0;

		if (mass) {
			_inv_mass = 1.0 / mass;
		}
		else {
			_inv_mass = 0;
		}

	} break;
	case PS2DE::BODY_MODE_KINEMATIC:
	case PS2DE::BODY_MODE_STATIC: {
		_inv_inertia = 0;
		_inv_mass = 0;
	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_inertia = 0;
		_inv_mass = 1.0 / mass;

	} break;
	}

	_update_transform_dependent();
}

void GodotBody2D::reset_mass_properties()
{
	calculate_inertia = true;
	calculate_center_of_mass = true;
	_mass_properties_changed();
}

void GodotBody2D::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;

	if (active) {
		if (mode == PS2DE::BODY_MODE_STATIC) {
			// Static bodies can't be active.
			active = false;
		}
		else if (get_space()) {
			get_space()->body_add_to_active_list(&active_list);
		}
	}
	else if (get_space()) {
		get_space()->body_remove_from_active_list(&active_list);
	}
}

void GodotBody2D::set_mode(PS2DE::BodyMode p_mode)
{
	PS2DE::BodyMode prev = mode;
	mode = p_mode;

	switch (p_mode) {
	case PS2DE::BODY_MODE_STATIC:
	case PS2DE::BODY_MODE_KINEMATIC: {
		_set_inv_transform(get_transform().affine_inverse());
		_inv_mass = 0;
		_inv_inertia = 0;
		_set_static(p_mode == PS2DE::BODY_MODE_STATIC);
		set_active(p_mode == PS2DE::BODY_MODE_KINEMATIC && contacts.size());
		linear_velocity = Vector2();
		angular_velocity = 0;
		if (mode == PS2DE::BODY_MODE_KINEMATIC && prev != mode) {
			first_time_kinematic = true;
		}
	} break;
	case PS2DE::BODY_MODE_RIGID: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		if (!calculate_inertia) {
			_inv_inertia = 1.0 / inertia;
		}
		_mass_properties_changed();
		_set_static(false);
		set_active(true);

	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		_inv_inertia = 0;
		angular_velocity = 0;
		_set_static(false);
		set_active(true);
	}
	}
}

PS2DE::BodyMode GodotBody2D::get_mode() const { return mode; }

void GodotBody2D::_shapes_changed()
{
	_mass_properties_changed();
	wakeup();
	wakeup_neighbours();
}

void GodotBody2D::set_space(GodotSpace2D* p_space)
{
	if (get_space()) {
		wakeup_neighbours();

		if (mass_properties_update_list.in_list()) {
			get_space()->body_remove_from_mass_properties_update_list(&mass_properties_update_list);
		}
		if (active_list.in_list()) {
			get_space()->body_remove_from_active_list(&active_list);
		}
		if (direct_state_query_list.in_list()) {
			get_space()->body_remove_from_state_query_list(&direct_state_query_list);
		}
	}

	_set_space(p_space);

	if (get_space()) {
		_mass_properties_changed();

		if (active && !active_list.in_list()) {
			get_space()->body_add_to_active_list(&active_list);
		}
	}
}

void GodotBody2D::_update_transform_dependent()
{
	center_of_mass = get_transform().basis_xform(center_of_mass_local);
}

void GodotBody2D::wakeup_neighbours()
{
	for (const Pair<GodotConstraint2D*, int>& E : constraint_list) {
		const GodotConstraint2D* c = E.first;
		GodotBody2D** n = c->get_body_ptr();
		int bc = c->get_body_count();

		for (int i = 0; i < bc; i++) {
			if (i == E.second) {
				continue;
			}
			GodotBody2D* b = n[i];
			if (b->mode < PS2DE::BODY_MODE_RIGID) {
				continue;
			}

			if (!b->is_active()) {
				b->set_active(true);
			}
		}
	}
}

bool GodotBody2D::sleep_test(real_t p_step)
{
	if (mode == PS2DE::BODY_MODE_STATIC || mode == PS2DE::BODY_MODE_KINEMATIC) {
		return true;
	}
	else if (!can_sleep) {
		return false;
	}

	ERR_FAIL_NULL_V(get_space(), true);

	if (Math::abs(angular_velocity) < get_space()->get_body_angular_velocity_sleep_threshold() &&
		Math::abs(linear_velocity.length_squared()) <
			get_space()->get_body_linear_velocity_sleep_threshold() *
				get_space()->get_body_linear_velocity_sleep_threshold()) {
		still_time += p_step;

		return still_time > get_space()->get_body_time_to_sleep();
	}
	else {
		still_time = 0;
		return false;
	}
}

PhysicsDirectBodyState2D* GodotBody2D::get_direct_state()
{
	if (!direct_state) {
		direct_state = memnew(PhysicsDirectBodyState2D(this));
	}
	return direct_state;
}

GodotBody2D::GodotBody2D()
	: GodotCollisionObject2D(TYPE_BODY), active_list(this), mass_properties_update_list(this),
	  direct_state_query_list(this)
{
	_set_static(false);
}

GodotBody2D::~GodotBody2D() { memdelete(direct_state); }

void GodotBody2D::integrate_velocities(float) {}

void GodotBody2D::integrate_forces(float) {}

void GodotBody2D::call_queries() {}

// --- PhysicsDirectBodyState2D ---

PhysicsDirectBodyState2D::PhysicsDirectBodyState2D(GodotBody2D* p_body) : body(p_body) {}

Vector2 PhysicsDirectBodyState2D::get_total_gravity() const { return body->gravity; }

real_t PhysicsDirectBodyState2D::get_total_angular_damp() const
{
	return body->total_angular_damp;
}

real_t PhysicsDirectBodyState2D::get_total_linear_damp() const
{
	return body->total_linear_damp;
}

Vector2 PhysicsDirectBodyState2D::get_center_of_mass() const
{
	return body->get_center_of_mass();
}

Vector2 PhysicsDirectBodyState2D::get_center_of_mass_local() const
{
	return body->get_center_of_mass_local();
}

real_t PhysicsDirectBodyState2D::get_inverse_mass() const { return body->get_inv_mass(); }

real_t PhysicsDirectBodyState2D::get_inverse_inertia() const
{
	return body->get_inv_inertia();
}

void PhysicsDirectBodyState2D::set_linear_velocity(const Vector2& p_velocity)
{
	body->wakeup();
	body->set_linear_velocity(p_velocity);
}

Vector2 PhysicsDirectBodyState2D::get_linear_velocity() const
{
	return body->get_linear_velocity();
}

void PhysicsDirectBodyState2D::set_angular_velocity(real_t p_velocity)
{
	body->wakeup();
	body->set_angular_velocity(p_velocity);
}

real_t PhysicsDirectBodyState2D::get_angular_velocity() const
{
	return body->get_angular_velocity();
}

void PhysicsDirectBodyState2D::set_transform(const Transform2D& p_transform) {}

Transform2D PhysicsDirectBodyState2D::get_transform() const { return body->get_transform(); }

Vector2 PhysicsDirectBodyState2D::get_velocity_at_local_position(
	const Vector2& p_position) const
{
	return body->get_velocity_in_local_point(p_position);
}

void PhysicsDirectBodyState2D::apply_central_impulse(const Vector2& p_impulse)
{
	body->wakeup();
	body->apply_central_impulse(p_impulse);
}

void PhysicsDirectBodyState2D::apply_impulse(
	const Vector2& p_impulse, const Vector2& p_position)
{
	body->wakeup();
	body->apply_impulse(p_impulse, p_position);
}

void PhysicsDirectBodyState2D::apply_torque_impulse(real_t p_torque)
{
	body->wakeup();
	body->apply_torque_impulse(p_torque);
}

void PhysicsDirectBodyState2D::apply_central_force(const Vector2& p_force)
{
	body->wakeup();
	body->apply_central_force(p_force);
}

void PhysicsDirectBodyState2D::apply_force(const Vector2& p_force, const Vector2& p_position)
{
	body->wakeup();
	body->apply_force(p_force, p_position);
}

void PhysicsDirectBodyState2D::apply_torque(real_t p_torque)
{
	body->wakeup();
	body->apply_torque(p_torque);
}

void PhysicsDirectBodyState2D::add_constant_central_force(const Vector2& p_force)
{
	body->wakeup();
	body->add_constant_central_force(p_force);
}

void PhysicsDirectBodyState2D::add_constant_force(
	const Vector2& p_force, const Vector2& p_position)
{
	body->wakeup();
	body->add_constant_force(p_force, p_position);
}

void PhysicsDirectBodyState2D::add_constant_torque(real_t p_torque)
{
	body->wakeup();
	body->add_constant_torque(p_torque);
}

void PhysicsDirectBodyState2D::set_constant_force(const Vector2& p_force)
{
	if (!p_force.is_zero_approx()) {
		body->wakeup();
	}
	body->set_constant_force(p_force);
}

Vector2 PhysicsDirectBodyState2D::get_constant_force() const
{
	return body->get_constant_force();
}

void PhysicsDirectBodyState2D::set_constant_torque(real_t p_torque)
{
	if (!Math::is_zero_approx(p_torque)) {
		body->wakeup();
	}
	body->set_constant_torque(p_torque);
}

real_t PhysicsDirectBodyState2D::get_constant_torque() const
{
	return body->get_constant_torque();
}

void PhysicsDirectBodyState2D::set_sleep_state(bool p_enable) { body->set_active(!p_enable); }

bool PhysicsDirectBodyState2D::is_sleeping() const { return !body->is_active(); }

void PhysicsDirectBodyState2D::set_collision_layer(uint32_t p_layer)
{
	body->set_collision_layer(p_layer);
}

uint32_t PhysicsDirectBodyState2D::get_collision_layer() const
{
	return body->get_collision_layer();
}

void PhysicsDirectBodyState2D::set_collision_mask(uint32_t p_mask)
{
	body->set_collision_mask(p_mask);
}

uint32_t PhysicsDirectBodyState2D::get_collision_mask() const
{
	return body->get_collision_mask();
}

int PhysicsDirectBodyState2D::get_contact_count() const { return body->contact_count; }

Vector2 PhysicsDirectBodyState2D::get_contact_local_position(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_pos;
}

Vector2 PhysicsDirectBodyState2D::get_contact_local_normal(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_normal;
}

int PhysicsDirectBodyState2D::get_contact_local_shape(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, -1);
	return body->contacts[p_contact_idx].local_shape;
}

Vector2 PhysicsDirectBodyState2D::get_contact_local_velocity_at_position(
	int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_velocity_at_pos;
}

RID PhysicsDirectBodyState2D::get_contact_collider(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, RID());
	return body->contacts[p_contact_idx].collider;
}

Vector2 PhysicsDirectBodyState2D::get_contact_collider_position(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].collider_pos;
}

int PhysicsDirectBodyState2D::get_contact_collider_shape(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, 0);
	return body->contacts[p_contact_idx].collider_shape;
}

Vector2 PhysicsDirectBodyState2D::get_contact_collider_velocity_at_position(
	int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].collider_velocity_at_pos;
}

Vector2 PhysicsDirectBodyState2D::get_contact_impulse(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].impulse;
}

PhysicsDirectSpaceState2D* PhysicsDirectBodyState2D::get_space_state()
{
	return body->get_space()->get_direct_state();
}

real_t PhysicsDirectBodyState2D::get_step() const
{
	return body->get_space()->get_last_step();
}

void PhysicsDirectBodyState2D::integrate_forces()
{
	real_t step = get_step();
	Vector2 lv = get_linear_velocity();
	lv += get_total_gravity() * step;

	real_t av = get_angular_velocity();

	real_t damp = 1.0 - step * get_total_linear_damp();

	if (damp < 0) { // reached zero in the given time
		damp = 0;
	}

	lv *= damp;

	damp = 1.0 - step * get_total_angular_damp();

	if (damp < 0) { // reached zero in the given time
		damp = 0;
	}

	av *= damp;

	set_linear_velocity(lv);
	set_angular_velocity(av);
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_shape_2d.cpp
// ============================================================================

#include "core/math/geometry_2d.h"
#include "core/templates/sort_array.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"

void GodotShape2D::configure(const Rect2& p_aabb)
{
	aabb = p_aabb;
	configured = true;
	for (const KeyValue<GodotCollisionObject2D*, int>& E : owners) {
		E.key->_shape_changed();
	}
}

Vector2 GodotShape2D::get_support(const Vector2& p_normal) const
{
	Vector2 res[2];
	int amnt;
	get_supports(p_normal, res, amnt);
	return res[0];
}

void GodotShape2D::add_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	if (E) {
		E->value++;
	}
	else {
		owners[p_owner] = 1;
	}
}

void GodotShape2D::remove_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	ERR_FAIL_COND(!E);
	E->value--;
	if (E->value == 0) {
		owners.remove(E);
	}
}

bool GodotShape2D::is_owner(GodotCollisionObject2D* p_owner) const { return owners.has(p_owner); }

const HashMap<GodotCollisionObject2D*, int>& GodotShape2D::get_owners() const { return owners; }

GodotShape2D::~GodotShape2D()
{
	ERR_FAIL_COND(owners.size());
	if (convex_points) {
		memdelete_arr(convex_points);
	}
}

void GodotShape2D::set_world_boundary(const Vector2& p_normal, real_t p_d)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_WORLD_BOUNDARY);
	wb_normal = p_normal;
	wb_d = p_d;
	configure(Rect2(Vector2(-1e15, -1e15), Vector2(1e15 * 2.0, 1e15 * 2.0)));
}

void GodotShape2D::set_separation_ray(real_t p_length, bool p_slide_on_slope)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEPARATION_RAY);
	ray_length = p_length;
	slide_on_slope = p_slide_on_slope;
	configure(Rect2(0, 0, 0.001, MAX(ray_length, (real_t)0.1)));
}

void GodotShape2D::set_segment(const Vector2& p_a, const Vector2& p_b)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEGMENT);
	seg_a = p_a;
	seg_b = p_b;
	seg_n = (seg_b - seg_a).orthogonal().normalized();

	Rect2 r(seg_a, Size2());
	r.expand_to(seg_b);
	if (r.size.x == 0) {
		r.size.x = 0.001;
	}
	if (r.size.y == 0) {
		r.size.y = 0.001;
	}
	configure(r);
}

void GodotShape2D::set_circle(real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CIRCLE);
	radius = p_radius;
	configure(Rect2(-radius, -radius, radius * 2.0, radius * 2.0));
}

void GodotShape2D::set_rectangle(const Vector2& p_half_extents)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_RECTANGLE);
	half_extents = p_half_extents;
	configure(Rect2(-half_extents, half_extents * 2.0));
}

void GodotShape2D::set_capsule(real_t p_height, real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CAPSULE);
	height = p_height;
	radius = p_radius;
	if (height < radius * 2.0) {
		height = radius * 2.0;
	}
	configure(Rect2(Point2(-radius, -height * 0.5), Size2(radius * 2.0, height)));
}

void GodotShape2D::set_convex_polygon(const Vector<Vector2>& p_points)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONVEX_POLYGON);
	if (convex_points) {
		memdelete_arr(convex_points);
		convex_points = nullptr;
	}
	convex_point_count = 0;

	int count = p_points.size();
	ERR_FAIL_COND(count < 3);

	Vector<Vector2> hull = Geometry2D::convex_hull(p_points);
	// convex_hull duplicates the first point at the end in counter-clockwise order.
	if (hull.size() > 1 && hull[0] == hull[hull.size() - 1]) {
		hull.resize(hull.size() - 1);
	}
	ERR_FAIL_COND(hull.size() < 3);

	convex_point_count = hull.size();
	convex_points = memnew_arr(ConvexPoint, convex_point_count);

	Rect2 r(hull[0], Size2());
	for (int i = 0; i < convex_point_count; i++) {
		convex_points[i].pos = hull[i];
		r.expand_to(hull[i]);
	}
	for (int i = 0; i < convex_point_count; i++) {
		Vector2 next = convex_points[(i + 1) % convex_point_count].pos;
		convex_points[i].normal = (next - convex_points[i].pos).orthogonal().normalized();
	}
	configure(r);
}

void GodotShape2D::set_concave_polygon(const Vector<Vector2>& p_segments)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);
	int len = p_segments.size();
	ERR_FAIL_COND(len % 2 != 0);

	concave_segments.clear();
	concave_points.clear();
	concave_bvh.clear();
	concave_bvh_depth = 0;

	if (len == 0) {
		configure(Rect2());
		return;
	}

	const Vector2* arr = p_segments.ptr();
	HashMap<Point2, int> pointmap;
	concave_segments.resize(len / 2);

	Rect2 global_aabb(arr[0], Size2());
	Vector<BVH> bvh_list;
	bvh_list.resize(len / 2);

	for (int i = 0; i < len / 2; i++) {
		Point2 a = arr[i * 2 + 0];
		Point2 b = arr[i * 2 + 1];

		int idx_a;
		HashMap<Point2, int>::Iterator E_a = pointmap.find(a);
		if (E_a) {
			idx_a = E_a->value;
		}
		else {
			idx_a = pointmap.size();
			pointmap[a] = idx_a;
		}

		int idx_b;
		HashMap<Point2, int>::Iterator E_b = pointmap.find(b);
		if (E_b) {
			idx_b = E_b->value;
		}
		else {
			idx_b = pointmap.size();
			pointmap[b] = idx_b;
		}

		concave_segments.write[i].points[0] = idx_a;
		concave_segments.write[i].points[1] = idx_b;

		Rect2 s_aabb(a, Size2());
		s_aabb.expand_to(b);
		global_aabb.expand_to(a);
		global_aabb.expand_to(b);

		bvh_list.write[i].aabb = s_aabb;
		bvh_list.write[i].left = -1;
		bvh_list.write[i].right = i;
	}

	concave_points.resize(pointmap.size());
	for (const KeyValue<Point2, int>& E : pointmap) {
		concave_points.write[E.value] = E.key;
	}

	concave_bvh_depth = 1;
	_generate_bvh(bvh_list.ptrw(), bvh_list.size(), 1);
	configure(global_aabb);
}

void GodotShape2D::project_range(
	const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_t<PS2DE::SHAPE_WORLD_BOUNDARY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_t<PS2DE::SHAPE_SEPARATION_RAY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_t<PS2DE::SHAPE_SEGMENT>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_t<PS2DE::SHAPE_CIRCLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_t<PS2DE::SHAPE_RECTANGLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_t<PS2DE::SHAPE_CAPSULE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_t<PS2DE::SHAPE_CONVEX_POLYGON>(p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range in GodotShape2D");
	}
}

void GodotShape2D::project_range_cast(const Vector2& p_cast, const Vector2& p_normal,
	const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_cast_t<PS2DE::SHAPE_WORLD_BOUNDARY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_cast_t<PS2DE::SHAPE_SEPARATION_RAY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_cast_t<PS2DE::SHAPE_SEGMENT>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_cast_t<PS2DE::SHAPE_CIRCLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_cast_t<PS2DE::SHAPE_RECTANGLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_cast_t<PS2DE::SHAPE_CAPSULE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_cast_t<PS2DE::SHAPE_CONVEX_POLYGON>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range_cast in GodotShape2D");
	}
}

void GodotShape2D::get_supports(const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		r_amount = 0;
	} break;
	case PS2DE::SHAPE_SEPARATION_RAY: {
		r_amount = 1;
		if (p_normal.y > 0) {
			*r_supports = Vector2(0, ray_length);
		}
		else {
			*r_supports = Vector2();
		}
	} break;
	case PS2DE::SHAPE_SEGMENT: {
		if (Math::abs(p_normal.dot(seg_n)) > segment_is_valid_support_threshold) {
			r_supports[0] = seg_a;
			r_supports[1] = seg_b;
			r_amount = 2;
			return;
		}

		real_t dp = p_normal.dot(seg_b - seg_a);
		if (dp > 0) {
			*r_supports = seg_b;
		}
		else {
			*r_supports = seg_a;
		}
		r_amount = 1;
	} break;
	case PS2DE::SHAPE_CIRCLE: {
		r_amount = 1;
		*r_supports = p_normal * radius;
	} break;
	case PS2DE::SHAPE_RECTANGLE: {
		for (int i = 0; i < 2; i++) {
			Vector2 ag;
			ag[i] = 1.0;
			real_t dp = ag.dot(p_normal);
			if (Math::abs(dp) <= segment_is_valid_support_threshold) {
				continue;
			}

			real_t sgn = dp > 0 ? 1.0 : -1.0;

			r_amount = 2;

			r_supports[0][i] = half_extents[i] * sgn;
			r_supports[0][i ^ 1] = half_extents[i ^ 1];

			r_supports[1][i] = half_extents[i] * sgn;
			r_supports[1][i ^ 1] = -half_extents[i ^ 1];

			return;
		}

		r_amount = 1;
		r_supports[0] = Vector2((p_normal.x < 0) ? -half_extents.x : half_extents.x,
			(p_normal.y < 0) ? -half_extents.y : half_extents.y);
	} break;
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 n = p_normal;

		real_t h = height * 0.5 - radius; // half-height of the rectangle part

		if (h > 0 && Math::abs(n.x) > segment_is_valid_support_threshold) {
			// make it flat
			n.y = 0.0;
			n.x = SIGN(n.x) * radius;

			r_amount = 2;
			r_supports[0] = n;
			r_supports[0].y += h;
			r_supports[1] = n;
			r_supports[1].y -= h;
		}
		else {
			n *= radius;
			n.y += (n.y > 0) ? h : -h;
			r_amount = 1;
			*r_supports = n;
		}
	} break;
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		int support_idx = -1;
		real_t d = -1e10;
		r_amount = 0;

		for (int i = 0; i < convex_point_count; i++) {
			// test point
			real_t ld = p_normal.dot(convex_points[i].pos);
			if (ld > d) {
				support_idx = i;
				d = ld;
			}

			// test segment
			if (convex_points[i].normal.dot(p_normal) > segment_is_valid_support_threshold) {
				r_amount = 2;
				r_supports[0] = convex_points[i].pos;
				r_supports[1] = convex_points[(i + 1) % convex_point_count].pos;
				return;
			}
		}

		ERR_FAIL_COND_MSG(support_idx == -1, "Convex polygon shape support not found.");

		r_amount = 1;
		r_supports[0] = convex_points[support_idx].pos;
	} break;
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		real_t d = -1e10;
		int idx = -1;
		for (int i = 0; i < concave_points.size(); i++) {
			real_t ld = p_normal.dot(concave_points[i]);
			if (ld > d) {
				d = ld;
				idx = i;
			}
		}

		r_amount = 1;
		ERR_FAIL_COND(idx == -1);
		*r_supports = concave_points[idx];
	} break;
	default: {
		r_amount = 0;
	} break;
	}
}

bool GodotShape2D::contains_point(const Vector2& p_point) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		return wb_normal.dot(p_point) < wb_d;
	}
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_SEGMENT:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return false;
	}
	case PS2DE::SHAPE_CIRCLE: {
		return p_point.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		real_t x = p_point.x;
		real_t y = p_point.y;
		real_t edge_x = half_extents.x;
		real_t edge_y = half_extents.y;
		return (x >= -edge_x) && (x < edge_x) && (y >= -edge_y) && (y < edge_y);
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 p = p_point;
		p.y = Math::abs(p.y);
		p.y -= height * 0.5 - radius;
		if (p.y < 0) {
			p.y = 0;
		}

		return p.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		bool out = false;
		bool in = false;

		for (int i = 0; i < convex_point_count; i++) {
			real_t d = convex_points[i].normal.dot(p_point) -
					   convex_points[i].normal.dot(convex_points[i].pos);
			if (d > 0) {
				out = true;
			}
			else {
				in = true;
			}
		}

		return in != out;
	}
	default:
		return false;
	}
}

bool GodotShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		Vector2 segment = p_begin - p_end;
		real_t den = wb_normal.dot(segment);

		if (Math::abs(den) <= CMP_EPSILON) {
			return false;
		}

		real_t dist = (wb_normal.dot(p_begin) - wb_d) / den;

		if (dist < -CMP_EPSILON || dist > (1.0 + CMP_EPSILON)) {
			return false;
		}

		r_point = p_begin + segment * -dist;
		r_normal = wb_normal;

		return true;
	}
	case PS2DE::SHAPE_SEPARATION_RAY: {
		return false; // rays can't be intersected
	}
	case PS2DE::SHAPE_SEGMENT: {
		if (!Geometry2D::segment_intersects_segment(p_begin, p_end, seg_a, seg_b, &r_point)) {
			return false;
		}

		if (seg_n.dot(p_begin) > seg_n.dot(seg_a)) {
			r_normal = seg_n;
		}
		else {
			r_normal = -seg_n;
		}

		return true;
	}
	case PS2DE::SHAPE_CIRCLE: {
		Vector2 line_vec = p_end - p_begin;

		real_t a = line_vec.dot(line_vec);
		real_t b = 2 * p_begin.dot(line_vec);
		real_t c = p_begin.dot(p_begin) - radius * radius;

		real_t sqrtterm = b * b - 4 * a * c;

		if (sqrtterm < 0) {
			return false;
		}
		sqrtterm = Math::sqrt(sqrtterm);
		real_t res = (-b - sqrtterm) / (2 * a);

		if (res < 0 || res > 1 + CMP_EPSILON) {
			return false;
		}

		r_point = p_begin + line_vec * res;
		r_normal = r_point.normalized();
		return true;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		return get_aabb().intersects_segment(p_begin, p_end, &r_point, &r_normal);
	}
	case PS2DE::SHAPE_CAPSULE: {
		real_t d = 1e10;
		Vector2 n = (p_end - p_begin).normalized();
		bool collided = false;

		// try spheres
		for (int i = 0; i < 2; i++) {
			Vector2 begin = p_begin;
			Vector2 end = p_end;
			real_t ofs = (i == 0) ? -height * 0.5 + radius : height * 0.5 - radius;
			begin.y += ofs;
			end.y += ofs;

			Vector2 line_vec = end - begin;

			real_t a = line_vec.dot(line_vec);
			real_t b = 2 * begin.dot(line_vec);
			real_t c = begin.dot(begin) - radius * radius;

			real_t sqrtterm = b * b - 4 * a * c;

			if (sqrtterm < 0) {
				continue;
			}

			sqrtterm = Math::sqrt(sqrtterm);
			real_t res = (-b - sqrtterm) / (2 * a);

			if (res < 0 || res > 1 + CMP_EPSILON) {
				continue;
			}

			Vector2 point = begin + line_vec * res;
			Vector2 pointf(point.x, point.y - ofs);
			real_t pd = n.dot(pointf);
			if (pd < d) {
				r_point = pointf;
				r_normal = point.normalized();
				d = pd;
				collided = true;
			}
		}

		Vector2 rpos, rnorm;
		if (Rect2(Point2(-radius, -height * 0.5 + radius), Size2(radius * 2.0, height - radius * 2))
				.intersects_segment(p_begin, p_end, &rpos, &rnorm)) {
			real_t pd = n.dot(rpos);
			if (pd < d) {
				r_point = rpos;
				r_normal = rnorm;
				d = pd;
				collided = true;
			}
		}

		return collided;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		for (int i = 0; i < convex_point_count; i++) {
			Vector2 res;

			if (!Geometry2D::segment_intersects_segment(p_begin, p_end, convex_points[i].pos,
					convex_points[(i + 1) % convex_point_count].pos, &res)) {
				continue;
			}

			real_t nd = n.dot(res);
			if (nd < d) {
				d = nd;
				r_point = res;
				r_normal = convex_points[i].normal;
				inters = true;
			}
		}

		return inters;
	}
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
			return false;
		}

		uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

		enum
		{
			TEST_AABB_BIT = 0,
			VISIT_LEFT_BIT = 1,
			VISIT_RIGHT_BIT = 2,
			VISIT_DONE_BIT = 3,
			VISITED_BIT_SHIFT = 29,
			NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
			VISITED_BIT_MASK = ~NODE_IDX_MASK,
		};

		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		int level = 0;

		const ConcaveSegment* segmentptr = &concave_segments[0];
		const Vector2* pointptr = &concave_points[0];
		const BVH* bvhptr = &concave_bvh[0];

		stack[0] = 0;
		while (true) {
			uint32_t node = stack[level] & NODE_IDX_MASK;
			const BVH& bvh2 = bvhptr[node];
			bool done = false;

			switch (stack[level] >> VISITED_BIT_SHIFT) {
			case TEST_AABB_BIT: {
				bool valid = bvh2.aabb.intersects_segment(p_begin, p_end);
				if (!valid) {
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					if (bvh2.left < 0) {
						const ConcaveSegment& s = segmentptr[bvh2.right];
						Vector2 a = pointptr[s.points[0]];
						Vector2 b = pointptr[s.points[1]];

						Vector2 res;

						if (Geometry2D::segment_intersects_segment(p_begin, p_end, a, b, &res)) {
							real_t nd = n.dot(res);
							if (nd < d) {
								d = nd;
								r_point = res;
								r_normal = (b - a).orthogonal().normalized();
								inters = true;
							}
						}

						stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
					}
					else {
						stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
					}
				}
			}
				continue;
			case VISIT_LEFT_BIT: {
				stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.left | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_RIGHT_BIT: {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.right | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_DONE_BIT: {
				if (level == 0) {
					done = true;
					break;
				}
				else {
					level--;
				}
			}
				continue;
			}

			if (done) {
				break;
			}
		}

		if (inters) {
			if (n.dot(r_normal) > 0) {
				r_normal = -r_normal;
			}
		}

		return inters;
	}
	default:
		return false;
	}
}

real_t GodotShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return 0;
	}
	case PS2DE::SHAPE_SEGMENT: {
		return p_mass * ((seg_a * p_scale).distance_squared_to(seg_b * p_scale)) / 12;
	}
	case PS2DE::SHAPE_CIRCLE: {
		real_t a = radius * p_scale.x;
		real_t b = radius * p_scale.y;
		return p_mass * (a * a + b * b) / 4;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		Vector2 he2 = half_extents * 2 * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 he2 = Vector2(radius * 2, height) * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		ERR_FAIL_COND_V_MSG(convex_point_count == 0, 0, "Convex polygon shape has no points.");
		Rect2 aabb_new;
		aabb_new.position = convex_points[0].pos * p_scale;
		for (int i = 0; i < convex_point_count; i++) {
			aabb_new.expand_to(convex_points[i].pos * p_scale);
		}

		return p_mass * aabb_new.size.dot(aabb_new.size) / 12.0;
	}
	default:
		return 0;
	}
}

int GodotShape2D::_generate_bvh(BVH* p_bvh, int p_len, int p_depth)
{
	if (p_len == 1) {
		concave_bvh_depth = MAX(p_depth, concave_bvh_depth);
		concave_bvh.push_back(*p_bvh);
		return concave_bvh.size() - 1;
	}

	Rect2 global_aabb = p_bvh[0].aabb;
	for (int i = 1; i < p_len; i++) {
		global_aabb = global_aabb.merge(p_bvh[i].aabb);
	}

	if (global_aabb.size.x > global_aabb.size.y) {
		SortArray<BVH, BVH_CompareX> sort;
		sort.sort(p_bvh, p_len);
	}
	else {
		SortArray<BVH, BVH_CompareY> sort;
		sort.sort(p_bvh, p_len);
	}

	int median = p_len / 2;

	BVH node;
	node.aabb = global_aabb;
	int node_idx = concave_bvh.size();
	concave_bvh.push_back(node);

	int l = _generate_bvh(p_bvh, median, p_depth + 1);
	int r = _generate_bvh(&p_bvh[median], p_len - median, p_depth + 1);
	concave_bvh.write[node_idx].left = l;
	concave_bvh.write[node_idx].right = r;

	return node_idx;
}

void GodotShape2D::cull(
	const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);

	if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
		return;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,
	};

	int level = 0;

	const ConcaveSegment* segmentptr = &concave_segments[0];
	const Vector2* pointptr = &concave_points[0];
	const BVH* bvhptr = &concave_bvh[0];

	stack[0] = 0;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& bvh2 = bvhptr[node];

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			bool valid = p_local_aabb.intersects(bvh2.aabb);
			if (!valid) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (bvh2.left < 0) {
					const ConcaveSegment& s = segmentptr[bvh2.right];
					Vector2 a = pointptr[s.points[0]];
					Vector2 b = pointptr[s.points[1]];

					GodotShape2D ss(a, b, (b - a).orthogonal().normalized());

					if (p_callback(p_userdata, &ss)) {
						return;
					}
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
		}
			continue;
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.left | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.right | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_DONE_BIT: {
			if (level == 0) {
				return;
			}
			else {
				level--;
			}
		}
			continue;
		}
	}
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_collision_solver_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "core/math/geometry_2d.h"
#include "core/templates/sort_array.h"

struct _CollectorCallback2D {
	GodotCollisionSolver2D::CallbackResult callback = nullptr;
	void *userdata = nullptr;
	bool swap = false;
	bool collided = false;
	Vector2 normal;
	Vector2 *sep_axis = nullptr;

	_FORCE_INLINE_ void call(const Vector2 &p_point_A, const Vector2 &p_point_B) {
		if (swap) {
			callback(p_point_B, p_point_A, userdata);
		} else {
			callback(p_point_A, p_point_B, userdata);
		}
	}
};

typedef void (*GenerateContactsFunc)(const Vector2 *, int, const Vector2 *, int, _CollectorCallback2D *);

_FORCE_INLINE_ static void _generate_contacts_point_point(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 1);
#endif

	p_collector->call(*p_points_A, *p_points_B);
}

_FORCE_INLINE_ static void _generate_contacts_point_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 2);
#endif

	Vector2 closest_B = Geometry2D::get_closest_point_to_segment_uncapped(*p_points_A, p_points_B[0], p_points_B[1]);
	p_collector->call(*p_points_A, closest_B);
}

struct _generate_contacts_Pair {
	bool a = false;
	int idx = 0;
	real_t d = 0.0;
	_FORCE_INLINE_ bool operator<(const _generate_contacts_Pair &l) const { return d < l.d; }
};

_FORCE_INLINE_ static void _generate_contacts_edge_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 2);
	ERR_FAIL_COND(p_point_count_B != 2); // circle is actually a 4x3 matrix
#endif

	Vector2 n = p_collector->normal;
	Vector2 t = n.orthogonal();
	real_t dA = n.dot(p_points_A[0]);
	real_t dB = n.dot(p_points_B[0]);

	_generate_contacts_Pair dvec[4];

	dvec[0].d = t.dot(p_points_A[0]);
	dvec[0].a = true;
	dvec[0].idx = 0;
	dvec[1].d = t.dot(p_points_A[1]);
	dvec[1].a = true;
	dvec[1].idx = 1;
	dvec[2].d = t.dot(p_points_B[0]);
	dvec[2].a = false;
	dvec[2].idx = 0;
	dvec[3].d = t.dot(p_points_B[1]);
	dvec[3].a = false;
	dvec[3].idx = 1;

	SortArray<_generate_contacts_Pair> sa;
	sa.sort(dvec, 4);

	for (int i = 1; i <= 2; i++) {
		if (dvec[i].a) {
			Vector2 a = p_points_A[dvec[i].idx];
			Vector2 b = n.plane_project(dB, a);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		} else {
			Vector2 b = p_points_B[dvec[i].idx];
			Vector2 a = n.plane_project(dA, b);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		}
	}
}

static void _generate_contacts_from_supports(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A < 1);
	ERR_FAIL_COND(p_point_count_B < 1);
#endif

	static const GenerateContactsFunc generate_contacts_func_table[2][2] = {
		{
				_generate_contacts_point_point,
				_generate_contacts_point_edge,
		},
		{
				nullptr,
				_generate_contacts_edge_edge,
		}
	};

	int pointcount_B = 0;
	int pointcount_A = 0;
	const Vector2 *points_A = nullptr;
	const Vector2 *points_B = nullptr;

	if (p_point_count_A > p_point_count_B) {
		//swap
		p_collector->swap = !p_collector->swap;
		p_collector->normal = -p_collector->normal;

		pointcount_B = p_point_count_A;
		pointcount_A = p_point_count_B;
		points_A = p_points_B;
		points_B = p_points_A;
	} else {
		pointcount_B = p_point_count_B;
		pointcount_A = p_point_count_A;
		points_A = p_points_A;
		points_B = p_points_B;
	}

	int version_A = (pointcount_A > 2 ? 2 : pointcount_A) - 1;
	int version_B = (pointcount_B > 2 ? 2 : pointcount_B) - 1;

	GenerateContactsFunc contacts_func = generate_contacts_func_table[version_A][version_B];
	ERR_FAIL_NULL(contacts_func);
	contacts_func(points_A, pointcount_A, points_B, pointcount_B, p_collector);
}

template <PS2DE::ShapeType typeA, PS2DE::ShapeType typeB, bool castA = false, bool castB = false, bool withMargin = false>
class SeparatorAxisTest2D {
	const GodotShape2D *shape_A = nullptr;
	const GodotShape2D *shape_B = nullptr;
	const Transform2D *transform_A = nullptr;
	const Transform2D *transform_B = nullptr;
	real_t best_depth = 1e15;
	Vector2 best_axis;
#ifdef DEBUG_ENABLED
	int best_axis_count = 0;
	int best_axis_index = -1;
#endif
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	_CollectorCallback2D *callback;

public:
	_FORCE_INLINE_ bool test_previous_axis() {
		if (callback && callback->sep_axis && *callback->sep_axis != Vector2()) {
			return test_axis(*callback->sep_axis);
		} else {
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif
		}
		return true;
	}

	_FORCE_INLINE_ bool test_cast() {
		if (castA) {
			Vector2 na = motion_A.normalized();
			if (!test_axis(na)) {
				return false;
			}
			if (!test_axis(na.orthogonal())) {
				return false;
			}
		}

		if (castB) {
			Vector2 nb = motion_B.normalized();
			if (!test_axis(nb)) {
				return false;
			}
			if (!test_axis(nb.orthogonal())) {
				return false;
			}
		}

		return true;
	}

	_FORCE_INLINE_ bool test_axis(const Vector2 &p_axis) {
		Vector2 axis = p_axis;

		if (Math::is_zero_approx(axis.x) &&
				Math::is_zero_approx(axis.y)) {
			// strange case, try an upwards separator
			axis = Vector2(0.0, 1.0);
		}

		real_t min_A = 0.0, max_A = 0.0, min_B = 0.0, max_B = 0.0;

		if constexpr (castA) {
			shape_A->project_range_cast_t<typeA>(motion_A, axis, *transform_A, min_A, max_A);
		} else {
			shape_A->project_range_t<typeA>(axis, *transform_A, min_A, max_A);
		}

		if constexpr (castB) {
			shape_B->project_range_cast_t<typeB>(motion_B, axis, *transform_B, min_B, max_B);
		} else {
			shape_B->project_range_t<typeB>(axis, *transform_B, min_B, max_B);
		}

		if (withMargin) {
			min_A -= margin_A;
			max_A += margin_A;
			min_B -= margin_B;
			max_B += margin_B;
		}

		min_B -= (max_A - min_A) * 0.5;
		max_B += (max_A - min_A) * 0.5;

		real_t dmin = min_B - (min_A + max_A) * 0.5;
		real_t dmax = max_B - (min_A + max_A) * 0.5;

		if (dmin > 0.0 || dmax < 0.0) {
			if (callback && callback->sep_axis) {
				*callback->sep_axis = axis;
			}
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif

			return false; // doesn't contain 0
		}

		//use the smallest depth

		dmin = Math::abs(dmin);

		if (dmax < dmin) {
			if (dmax < best_depth) {
				best_depth = dmax;
				best_axis = axis;
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		} else {
			if (dmin < best_depth) {
				best_depth = dmin;
				best_axis = -axis; // keep it as A axis
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		}

#ifdef DEBUG_ENABLED
		best_axis_count++;
#endif

		return true;
	}

	_FORCE_INLINE_ void generate_contacts() {
		// nothing to do, don't generate
		if (best_axis == Vector2(0.0, 0.0)) {
			return;
		}

		if (callback) {
			callback->collided = true;

			if (!callback->callback) {
				return; //only collide, no callback
			}
		}
		static const int max_supports = 2;

		Vector2 supports_A[max_supports];
		int support_count_A;
		if (castA) {
			shape_A->get_supports_transformed_cast(motion_A, -best_axis, *transform_A, supports_A, support_count_A);
		} else {
			shape_A->get_supports(transform_A->basis_xform_inv(-best_axis).normalized(), supports_A, support_count_A);
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] = transform_A->xform(supports_A[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] += -best_axis * margin_A;
			}
		}

		Vector2 supports_B[max_supports];
		int support_count_B;
		if (castB) {
			shape_B->get_supports_transformed_cast(motion_B, best_axis, *transform_B, supports_B, support_count_B);
		} else {
			shape_B->get_supports(transform_B->basis_xform_inv(best_axis).normalized(), supports_B, support_count_B);
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] = transform_B->xform(supports_B[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] += best_axis * margin_B;
			}
		}
		if (callback) {
			callback->normal = best_axis;
			_generate_contacts_from_supports(supports_A, support_count_A, supports_B, support_count_B, callback);

			if (callback->sep_axis && *callback->sep_axis != Vector2()) {
				*callback->sep_axis = Vector2(); //invalidate previous axis (no test)
			}
		}
	}

	_FORCE_INLINE_ SeparatorAxisTest2D(const GodotShape2D *p_shape_A, const Transform2D &p_transform_a, const GodotShape2D *p_shape_B, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_A = Vector2(), const Vector2 &p_motion_B = Vector2(), real_t p_margin_A = 0, real_t p_margin_B = 0) {
		margin_A = p_margin_A;
		margin_B = p_margin_B;
		shape_A = p_shape_A;
		shape_B = p_shape_B;
		transform_A = &p_transform_a;
		transform_B = &p_transform_b;
		motion_A = p_motion_A;
		motion_B = p_motion_B;
		callback = p_collector;
	}
};

/****** SAT TESTS *******/

#define TEST_POINT(m_a, m_b) \
	((!separator.test_axis(((m_a) - (m_b)).normalized())) || \
			(castA && !separator.test_axis(((m_a) + p_motion_a - (m_b)).normalized())) || \
			(castB && !separator.test_axis(((m_a) - ((m_b) + p_motion_b)).normalized())) || \
			(castA && castB && !separator.test_axis(((m_a) + p_motion_a - ((m_b) + p_motion_b)).normalized())))

typedef void (*CollisionFunc)(const GodotShape2D *, const Transform2D &, const GodotShape2D *, const Transform2D &, _CollectorCallback2D *p_collector, const Vector2 &, const Vector2 &, real_t, real_t);

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_segment(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *segment_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_SEGMENT, castA, castB, withMargin> separator(segment_A, p_transform_a, segment_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}
	//this collision is kind of pointless

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}
	if (!separator.test_axis(segment_B->get_xformed_normal(p_transform_b))) {
		return;
	}

	if (withMargin) {
		//points grow to circles

		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(segment_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//segment normal
	if (!separator.test_axis(
				(p_transform_a.xform(segment_A->get_b()) - p_transform_a.xform(segment_A->get_a())).normalized().orthogonal())) {
		return;
	}

	//endpoint a vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.get_origin())) {
		return;
	}
	//endpoint b vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(segment_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if (withMargin) {
		Transform2D inv = p_transform_b.affine_inverse();

		Vector2 a = p_transform_a.xform(segment_A->get_a());
		Vector2 b = p_transform_a.xform(segment_A->get_b());

		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a))) {
			return;
		}
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b))) {
			return;
		}

		if constexpr (castA) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b + p_motion_a))) {
				return;
			}
		}

		if constexpr (castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b + p_motion_a))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(segment_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(segment_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if (withMargin) {
			if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
			if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(circle_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(circle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	const Vector2 &sphere = p_transform_a.columns[2];
	const Vector2 *axis = &p_transform_b.columns[0];
	//const Vector2& half_extents = rectangle_B->get_half_extents();

	if (!separator.test_axis(axis[0].normalized())) {
		return;
	}

	if (!separator.test_axis(axis[1].normalized())) {
		return;
	}

	Transform2D binv = p_transform_b.affine_inverse();
	{
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphere))) {
			return;
		}
	}

	if constexpr (castA) {
		Vector2 sphereofs = sphere + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castB) {
		Vector2 sphereofs = sphere - p_motion_b;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castA && castB) {
		Vector2 sphereofs = sphere - p_motion_b + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(circle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	//capsule endpoints
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(circle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//poly faces and poly points vs circle
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.xform(convex_B->get_point(i)))) {
			return;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces A
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//box faces B
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if constexpr (withMargin) {
		Transform2D invA = p_transform_a.affine_inverse();
		Transform2D invB = p_transform_b.affine_inverse();

		if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, p_transform_b, invB))) {
			return;
		}

		if constexpr (castA || castB) {
			Transform2D aofs = p_transform_a;
			aofs.columns[2] += p_motion_a;

			Transform2D bofs = p_transform_b;
			bofs.columns[2] += p_motion_b;

			[[maybe_unused]] Transform2D aofsinv = aofs.affine_inverse();
			[[maybe_unused]] Transform2D bofsinv = bofs.affine_inverse();

			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, p_transform_b, invB))) {
					return;
				}
			}

			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}

			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	//box endpoints to capsule circles

	Transform2D boxinv = p_transform_a.affine_inverse();

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	for (int i = 0; i < 2; i++) {
		{
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		capsule_dir *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(rectangle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//convex faces
	Transform2D boxinv;
	if constexpr (withMargin) {
		boxinv = p_transform_a.affine_inverse();
	}
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if constexpr (withMargin) {
			//all points vs all points need to be tested if margin exist
			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i))))) {
				return;
			}
			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) - p_motion_a))) {
					return;
				}
			}
			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b))) {
					return;
				}
			}
			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b - p_motion_a))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(capsule_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//capsule endpoints

	real_t capsule_dir_A = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
	for (int i = 0; i < 2; i++) {
		Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir_A;

		real_t capsule_dir_B = capsule_B->get_height() * 0.5 - capsule_B->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_B = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir_B;

			if (TEST_POINT(capsule_endpoint_A, capsule_endpoint_B)) {
				return;
			}

			capsule_dir_B *= -1.0;
		}

		capsule_dir_A *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(capsule_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//poly vs capsule
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		Vector2 cpoint = p_transform_b.xform(convex_B->get_point(i));

		real_t capsule_dir = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir;

			if (TEST_POINT(capsule_endpoint_A, cpoint)) {
				return;
			}

			capsule_dir *= -1.0;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_convex_polygon_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *convex_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CONVEX_POLYGON, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(convex_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	for (int i = 0; i < convex_A->get_point_count(); i++) {
		if (!separator.test_axis(convex_A->get_xformed_segment_normal(p_transform_a, i))) {
			return;
		}
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	if (withMargin) {
		for (int i = 0; i < convex_A->get_point_count(); i++) {
			for (int j = 0; j < convex_B->get_point_count(); j++) {
				if (TEST_POINT(p_transform_a.xform(convex_A->get_point(i)), p_transform_b.xform(convex_B->get_point(j)))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

////////

bool sat_2d_calculate_penetration(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, GodotCollisionSolver2D::CallbackResult p_result_callback, void *p_userdata, bool p_swap, Vector2 *sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();

	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_A->is_concave(), false);

	PS2DE::ShapeType type_B = p_shape_B->get_type();

	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_B->is_concave(), false);

	static const CollisionFunc collision_table[5][5] = {
		{ _collision_segment_segment<false, false, false>,
				_collision_segment_circle<false, false, false>,
				_collision_segment_rectangle<false, false, false>,
				_collision_segment_capsule<false, false, false>,
				_collision_segment_convex_polygon<false, false, false> },
		{ nullptr,
				_collision_circle_circle<false, false, false>,
				_collision_circle_rectangle<false, false, false>,
				_collision_circle_capsule<false, false, false>,
				_collision_circle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, false>,
				_collision_rectangle_capsule<false, false, false>,
				_collision_rectangle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, false>,
				_collision_capsule_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, false> }

	};

	static const CollisionFunc collision_table_castA[5][5] = {
		{ _collision_segment_segment<true, false, false>,
				_collision_segment_circle<true, false, false>,
				_collision_segment_rectangle<true, false, false>,
				_collision_segment_capsule<true, false, false>,
				_collision_segment_convex_polygon<true, false, false> },
		{ nullptr,
				_collision_circle_circle<true, false, false>,
				_collision_circle_rectangle<true, false, false>,
				_collision_circle_capsule<true, false, false>,
				_collision_circle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, false>,
				_collision_rectangle_capsule<true, false, false>,
				_collision_rectangle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, false>,
				_collision_capsule_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, false> }

	};

	static const CollisionFunc collision_table_castB[5][5] = {
		{ _collision_segment_segment<false, true, false>,
				_collision_segment_circle<false, true, false>,
				_collision_segment_rectangle<false, true, false>,
				_collision_segment_capsule<false, true, false>,
				_collision_segment_convex_polygon<false, true, false> },
		{ nullptr,
				_collision_circle_circle<false, true, false>,
				_collision_circle_rectangle<false, true, false>,
				_collision_circle_capsule<false, true, false>,
				_collision_circle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, false>,
				_collision_rectangle_capsule<false, true, false>,
				_collision_rectangle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, false>,
				_collision_capsule_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, false> }

	};

	static const CollisionFunc collision_table_castA_castB[5][5] = {
		{ _collision_segment_segment<true, true, false>,
				_collision_segment_circle<true, true, false>,
				_collision_segment_rectangle<true, true, false>,
				_collision_segment_capsule<true, true, false>,
				_collision_segment_convex_polygon<true, true, false> },
		{ nullptr,
				_collision_circle_circle<true, true, false>,
				_collision_circle_rectangle<true, true, false>,
				_collision_circle_capsule<true, true, false>,
				_collision_circle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, false>,
				_collision_rectangle_capsule<true, true, false>,
				_collision_rectangle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, false>,
				_collision_capsule_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, false> }

	};

	static const CollisionFunc collision_table_margin[5][5] = {
		{ _collision_segment_segment<false, false, true>,
				_collision_segment_circle<false, false, true>,
				_collision_segment_rectangle<false, false, true>,
				_collision_segment_capsule<false, false, true>,
				_collision_segment_convex_polygon<false, false, true> },
		{ nullptr,
				_collision_circle_circle<false, false, true>,
				_collision_circle_rectangle<false, false, true>,
				_collision_circle_capsule<false, false, true>,
				_collision_circle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, true>,
				_collision_rectangle_capsule<false, false, true>,
				_collision_rectangle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, true>,
				_collision_capsule_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, true> }

	};

	static const CollisionFunc collision_table_castA_margin[5][5] = {
		{ _collision_segment_segment<true, false, true>,
				_collision_segment_circle<true, false, true>,
				_collision_segment_rectangle<true, false, true>,
				_collision_segment_capsule<true, false, true>,
				_collision_segment_convex_polygon<true, false, true> },
		{ nullptr,
				_collision_circle_circle<true, false, true>,
				_collision_circle_rectangle<true, false, true>,
				_collision_circle_capsule<true, false, true>,
				_collision_circle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, true>,
				_collision_rectangle_capsule<true, false, true>,
				_collision_rectangle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, true>,
				_collision_capsule_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, true> }

	};

	static const CollisionFunc collision_table_castB_margin[5][5] = {
		{ _collision_segment_segment<false, true, true>,
				_collision_segment_circle<false, true, true>,
				_collision_segment_rectangle<false, true, true>,
				_collision_segment_capsule<false, true, true>,
				_collision_segment_convex_polygon<false, true, true> },
		{ nullptr,
				_collision_circle_circle<false, true, true>,
				_collision_circle_rectangle<false, true, true>,
				_collision_circle_capsule<false, true, true>,
				_collision_circle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, true>,
				_collision_rectangle_capsule<false, true, true>,
				_collision_rectangle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, true>,
				_collision_capsule_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, true> }

	};

	static const CollisionFunc collision_table_castA_castB_margin[5][5] = {
		{ _collision_segment_segment<true, true, true>,
				_collision_segment_circle<true, true, true>,
				_collision_segment_rectangle<true, true, true>,
				_collision_segment_capsule<true, true, true>,
				_collision_segment_convex_polygon<true, true, true> },
		{ nullptr,
				_collision_circle_circle<true, true, true>,
				_collision_circle_rectangle<true, true, true>,
				_collision_circle_capsule<true, true, true>,
				_collision_circle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, true>,
				_collision_rectangle_capsule<true, true, true>,
				_collision_rectangle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, true>,
				_collision_capsule_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, true> }

	};

	_CollectorCallback2D callback;
	callback.callback = p_result_callback;
	callback.swap = p_swap;
	callback.userdata = p_userdata;
	callback.collided = false;
	callback.sep_axis = sep_axis;

	const GodotShape2D *A = p_shape_A;
	const GodotShape2D *B = p_shape_B;
	const Transform2D *transform_A = &p_transform_A;
	const Transform2D *transform_B = &p_transform_B;
	const Vector2 *motion_A = &p_motion_A;
	const Vector2 *motion_B = &p_motion_B;
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(transform_A, transform_B);
		SWAP(type_A, type_B);
		SWAP(motion_A, motion_B);
		SWAP(margin_A, margin_B);
		callback.swap = !callback.swap;
	}

	CollisionFunc collision_func;

	if (p_margin_A || p_margin_B) {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_margin[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA_margin[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB_margin[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB_margin[type_A - 2][type_B - 2];
		}
	} else {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB[type_A - 2][type_B - 2];
		}
	}

	ERR_FAIL_NULL_V(collision_func, false);

	collision_func(A, *transform_A, B, *transform_B, &callback, *motion_A, *motion_B, margin_A, margin_B);

	return callback.collided;
}

#define collision_solver sat_2d_calculate_penetration
//#define collision_solver gjk_epa_calculate_penetration

bool GodotCollisionSolver2D::solve_static_world_boundary(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_WORLD_BOUNDARY) {
		return false;
	}

	Vector2 n = p_transform_A.basis_xform(p_shape_A->get_world_boundary_normal()).normalized();
	Vector2 p = p_transform_A.xform(p_shape_A->get_world_boundary_normal() * p_shape_A->get_world_boundary_d());
	real_t d = n.dot(p);

	Vector2 supports[2];
	int support_count;

	p_shape_B->get_supports(p_transform_B.affine_inverse().basis_xform(-n).normalized(), supports, support_count);

	bool found = false;

	for (int i = 0; i < support_count; i++) {
		supports[i] += p_margin * supports[i].normalized();
		supports[i] = p_transform_B.xform(supports[i]);
		supports[i] += p_motion_B;
		real_t pd = n.dot(supports[i]);
		if (pd >= d) {
			continue;
		}
		found = true;

		Vector2 support_A = supports[i] - n * (pd - d);

		if (p_result_callback) {
			if (p_swap_result) {
				p_result_callback(supports[i], support_A, p_userdata);
			} else {
				p_result_callback(support_A, supports[i], p_userdata);
			}
		}
	}

	return found;
}

bool GodotCollisionSolver2D::solve_separation_ray(const GodotShape2D *p_shape_A, const Vector2 &p_motion_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_SEPARATION_RAY) {
		return false;
	}

	Vector2 from = p_transform_A.get_origin();
	Vector2 to = from + p_transform_A[1] * (p_shape_A->get_length() + p_margin);
	if (p_motion_A != Vector2()) {
		//not the best but should be enough
		Vector2 normal = (to - from).normalized();
		to += normal * MAX(0.0, normal.dot(p_motion_A));
	}
	Vector2 support_A = to;

	Transform2D invb = p_transform_B.affine_inverse();
	from = invb.xform(from);
	to = invb.xform(to);

	Vector2 p, n;
	if (!p_shape_B->intersect_segment(from, to, p, n)) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts when the ray is fully contained inside the shape.
	if (n == Vector2()) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts in the wrong direction.
	if (n.dot(from - to) < CMP_EPSILON) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	Vector2 support_B = p_transform_B.xform(p);
	if (p_shape_A->get_slide_on_slope()) {
		Vector2 global_n = invb.basis_xform_inv(n).normalized();
		support_B = support_A + (support_B - support_A).length() * global_n;
	}

	if (p_result_callback) {
		if (p_swap_result) {
			p_result_callback(support_B, support_A, p_userdata);
		} else {
			p_result_callback(support_A, support_B, p_userdata);
		}
	}
	return true;
}

struct _ConcaveCollisionInfo2D {
	const Transform2D *transform_A = nullptr;
	const GodotShape2D *shape_A = nullptr;
	const Transform2D *transform_B = nullptr;
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	GodotCollisionSolver2D::CallbackResult result_callback = nullptr;
	void *userdata = nullptr;
	bool swap_result = false;
	bool collided = false;
	int aabb_tests = 0;
	int collisions = 0;
	Vector2 *sep_axis = nullptr;
};

bool GodotCollisionSolver2D::concave_callback(void *p_userdata, GodotShape2D *p_convex) {
	_ConcaveCollisionInfo2D &cinfo = *(static_cast<_ConcaveCollisionInfo2D *>(p_userdata));
	cinfo.aabb_tests++;

	bool collided = collision_solver(cinfo.shape_A, *cinfo.transform_A, cinfo.motion_A, p_convex, *cinfo.transform_B, cinfo.motion_B, cinfo.result_callback, cinfo.userdata, cinfo.swap_result, cinfo.sep_axis, cinfo.margin_A, cinfo.margin_B);
	if (!collided) {
		return false;
	}

	cinfo.collided = true;
	cinfo.collisions++;

	// Stop at first collision if contacts are not needed.
	return !cinfo.result_callback;
}

bool GodotCollisionSolver2D::solve_concave(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	_ConcaveCollisionInfo2D cinfo;
	cinfo.transform_A = &p_transform_A;
	cinfo.shape_A = p_shape_A;
	cinfo.transform_B = &p_transform_B;
	cinfo.motion_A = p_motion_A;
	cinfo.result_callback = p_result_callback;
	cinfo.userdata = p_userdata;
	cinfo.swap_result = p_swap_result;
	cinfo.collided = false;
	cinfo.collisions = 0;
	cinfo.sep_axis = r_sep_axis;
	cinfo.margin_A = p_margin_A;
	cinfo.margin_B = p_margin_B;

	cinfo.aabb_tests = 0;

	Transform2D rel_transform = p_transform_A;
	rel_transform.columns[2] -= p_transform_B.get_origin();

	// Quickly compute a local Rect2.
	Rect2 local_aabb;
	for (int i = 0; i < 2; i++) {
		Vector2 axis(p_transform_B.columns[i]);
		real_t axis_scale = 1.0 / axis.length();
		axis *= axis_scale;

		real_t smin = 0.0, smax = 0.0;
		p_shape_A->project_rangev(axis, rel_transform, smin, smax);
		smin *= axis_scale;
		smax *= axis_scale;

		local_aabb.position[i] = smin;
		local_aabb.size[i] = smax - smin;
	}
	// In case of motion, expand the Rect2 in the motion direction.
	if (p_motion_A != Vector2()) {
		Rect2 moved_aabb = local_aabb;
		moved_aabb.position += p_motion_A;
		local_aabb = local_aabb.merge(moved_aabb);
	}

	p_shape_B->cull(local_aabb, concave_callback, &cinfo);

	return cinfo.collided;
}

bool GodotCollisionSolver2D::solve(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();
	PS2DE::ShapeType type_B = p_shape_B->get_type();
	bool concave_A = p_shape_A->is_concave();
	bool concave_B = p_shape_B->is_concave();
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	bool swap = false;

	if (type_A > type_B) {
		SWAP(type_A, type_B);
		SWAP(concave_A, concave_B);
		SWAP(margin_A, margin_B);
		swap = true;
	}

	if (type_A == PS2DE::SHAPE_WORLD_BOUNDARY) {
		if (type_B == PS2DE::SHAPE_WORLD_BOUNDARY) {
			WARN_PRINT_ONCE("Collisions between world boundaries are not supported.");
			return false;
		}

		if (swap) {
			return solve_static_world_boundary(p_shape_B, p_transform_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, p_margin_A);
		} else {
			return solve_static_world_boundary(p_shape_A, p_transform_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, p_margin_B);
		}

	} else if (type_A == PS2DE::SHAPE_SEPARATION_RAY) {
		if (type_B == PS2DE::SHAPE_SEPARATION_RAY) {
			WARN_PRINT_ONCE("Collisions between two rays are not supported.");
			return false; //no ray-ray
		}

		if (swap) {
			return solve_separation_ray(p_shape_B, p_motion_B, p_transform_B, p_shape_A, p_transform_A, p_result_callback, p_userdata, true, r_sep_axis, p_margin_B);
		} else {
			return solve_separation_ray(p_shape_A, p_motion_A, p_transform_A, p_shape_B, p_transform_B, p_result_callback, p_userdata, false, r_sep_axis, p_margin_A);
		}

	} else if (concave_B) {
		if (concave_A) {
			WARN_PRINT_ONCE("Collisions between two concave shapes are not supported.");
			return false;
		}

		if (!swap) {
			return solve_concave(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
		} else {
			return solve_concave(p_shape_B, p_transform_B, p_motion_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, r_sep_axis, margin_A, margin_B);
		}

	} else {
		return collision_solver(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
	}
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_constraints_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"


// ============================================================================
// GodotAreaPair2D / GodotArea2Pair2D
// ============================================================================

void GodotAreaPair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotAreaPair2D::GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape) {
	body = p_body;
	area = p_area;
	body_shape = p_body_shape;
	area_shape = p_area_shape;
	body->add_constraint(this, 0);
	area->add_constraint(this);
	if (p_body->get_mode() == PS2DE::BODY_MODE_KINEMATIC) { // need to be active to process pair
		p_body->set_active(true);
	}
}

bool GodotAreaPair2D::setup(real_t p_step) {
	return false;
}

bool GodotAreaPair2D::pre_solve(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::setup(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::pre_solve(real_t p_step) {
	if (process_collision_a) {
		if (colliding_a) {
			area_a->add_area_to_query(area_b, shape_b, shape_a);
		} else {
			area_a->remove_area_from_query(area_b, shape_b, shape_a);
		}
	}

	if (process_collision_b) {
		if (colliding_b) {
			area_b->add_area_to_query(area_a, shape_a, shape_b);
		} else {
			area_b->remove_area_from_query(area_a, shape_a, shape_b);
		}
	}

	return false; // Never do any post solving.
}

void GodotArea2Pair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotArea2Pair2D::GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b) {
	area_a = p_area_a;
	area_b = p_area_b;
	shape_a = p_shape_a;
	shape_b = p_shape_b;
	area_a_monitorable = area_a->is_monitorable();
	area_b_monitorable = area_b->is_monitorable();
	area_a->add_constraint(this);
	area_b->add_constraint(this);
}

// ============================================================================
// GodotBodyPair2D
// ============================================================================

#define ACCUMULATE_IMPULSES

#define MIN_VELOCITY 0.001
#define MAX_BIAS_ROTATION (Math::PI / 8)

void GodotBodyPair2D::_add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self) {
	GodotBodyPair2D *self = static_cast<GodotBodyPair2D *>(p_self);

	self->_contact_added_callback(p_point_A, p_point_B);
}

void GodotBodyPair2D::_contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B) {
	Vector2 local_A = A->get_inv_transform().basis_xform(p_point_A);
	Vector2 local_B = B->get_inv_transform().basis_xform(p_point_B - offset_B);

	int new_index = contact_count;

	ERR_FAIL_COND(new_index >= (MAX_CONTACTS + 1));

	Contact contact;
	contact.local_A = local_A;
	contact.local_B = local_B;
	contact.normal = (p_point_A - p_point_B).normalized();
	contact.used = true;

	// Attempt to determine if the contact will be reused.
	real_t recycle_radius_2 = space->get_contact_recycle_radius() * space->get_contact_recycle_radius();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		if (c.local_A.distance_squared_to(local_A) < (recycle_radius_2) &&
				c.local_B.distance_squared_to(local_B) < (recycle_radius_2)) {
			contact.acc_normal_impulse = c.acc_normal_impulse;
			contact.acc_tangent_impulse = c.acc_tangent_impulse;
			contact.acc_bias_impulse = c.acc_bias_impulse;
			contact.acc_bias_impulse_center_of_mass = c.acc_bias_impulse_center_of_mass;
			c = contact;
			return;
		}
	}

	// Figure out if the contact amount must be reduced to fit the new contact.
	if (new_index == MAX_CONTACTS) {
		// Remove the contact with the minimum depth.
		const Transform2D &transform_A = A->get_transform();
		const Transform2D &transform_B = B->get_transform();

		int least_deep = -1;
		real_t min_depth;

		// Start with depth for new contact.
		{
			Vector2 global_A = transform_A.basis_xform(contact.local_A);
			Vector2 global_B = transform_B.basis_xform(contact.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			min_depth = axis.dot(contact.normal);
		}

		for (int i = 0; i < contact_count; i++) {
			const Contact &c = contacts[i];
			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < min_depth) {
				min_depth = depth;
				least_deep = i;
			}
		}

		if (least_deep > -1) {
			// Replace the least deep contact by the new one.
			contacts[least_deep] = contact;
		}

		return;
	}

	contacts[new_index] = contact;
	contact_count++;
}

void GodotBodyPair2D::_validate_contacts() {
	// Make sure to erase contacts that are no longer valid.
	real_t max_separation = space->get_contact_max_separation();
	real_t max_separation2 = max_separation * max_separation;

	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];

		bool erase = false;
		if (!c.used) {
			// Was left behind in previous frame.
			erase = true;
		} else {
			c.used = false;

			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;
			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < -max_separation || (global_B + c.normal * depth - global_A).length_squared() > max_separation2) {
				erase = true;
			}
		}

		if (erase) {
			// Contact no longer needed, remove.
			if ((i + 1) < contact_count) {
				// Swap with the last one.
				SWAP(contacts[i], contacts[contact_count - 1]);
			}

			i--;
			contact_count--;
		}
	}
}

bool GodotBodyPair2D::_test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B) {
	Vector2 motion = p_A->get_linear_velocity() * p_step;
	real_t mlen = motion.length();
	if (mlen < CMP_EPSILON) {
		return false;
	}

	Vector2 mnormal = motion / mlen;

	real_t min = 0.0, max = 0.0;
	p_A->get_shape(p_shape_A)->project_rangev(mnormal, p_xform_A, min, max);

	bool fast_object = mlen > (max - min) * 0.3;
	if (!fast_object) {
		return false;
	}

	Transform2D predicted_xform_B = p_xform_B.translated(p_B->get_linear_velocity() * p_step);

	int a;
	Vector2 s[2];
	p_A->get_shape(p_shape_A)->get_supports(p_xform_A.basis_xform_inv(mnormal).normalized(), s, a);
	Vector2 from = p_xform_A.xform(s[0]);
	Vector2 to = from + motion;

	Transform2D from_inv = predicted_xform_B.affine_inverse();

	Vector2 local_from = from_inv.xform(from - motion * 0.1);
	Vector2 local_to = from_inv.xform(to);

	Vector2 rpos, rnorm;
	if (!p_B->get_shape(p_shape_B)->intersect_segment(local_from, local_to, rpos, rnorm)) {
		return false;
	}

	// Check one-way collision based on motion direction.
	if (p_A->get_shape(p_shape_A)->allows_one_way_collision() && p_B->is_shape_set_as_one_way_collision(p_shape_B)) {
		Vector2 direction = predicted_xform_B.basis_xform(p_B->get_shape_one_way_collision_direction(p_shape_B)).normalized();

		if (direction.dot(mnormal) < CMP_EPSILON) {
			collided = false;
			oneway_disabled = true;
			return false;
		}
	}

	// Shorten the linear velocity so it does not hit, but gets close enough,
	// next frame will hit softly or soft enough.
	Vector2 hitpos = predicted_xform_B.xform(rpos);

	real_t newlen = hitpos.distance_to(from) + (max - min) * 0.01;
	p_A->set_linear_velocity(mnormal * (newlen / p_step));

	return true;
}

static real_t combine_bounce(GodotBody2D *A, GodotBody2D *B) {
	return CLAMP(A->get_bounce() + B->get_bounce(), 0, 1);
}

static real_t combine_friction(GodotBody2D *A, GodotBody2D *B) {
	return Math::abs(MIN(A->get_friction(), B->get_friction()));
}

bool GodotBodyPair2D::setup(real_t p_step) {
	check_ccd = false;

	if (!A->interacts_with(B) || A->has_exception(B->get_self()) || B->has_exception(A->get_self())) {
		collided = false;
		return false;
	}

	collide_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && A->collides_with(B);
	collide_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && B->collides_with(A);

	report_contacts_only = false;
	if (!collide_A && !collide_B) {
		if ((A->get_max_contacts_reported() > 0) || (B->get_max_contacts_reported() > 0)) {
			report_contacts_only = true;
		} else {
			collided = false;
			return false;
		}
	}

	// use local A coordinates to avoid numerical issues on collision detection
	offset_B = B->get_transform().get_origin() - A->get_transform().get_origin();

	_validate_contacts();

	const Vector2 &offset_A = A->get_transform().get_origin();
	Transform2D xform_Au = A->get_transform().untranslated();
	Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

	Transform2D xform_Bu = B->get_transform();
	xform_Bu.columns[2] -= offset_A;
	Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	Vector2 motion_A, motion_B;

	if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_A = A->get_motion();
	}
	if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_B = B->get_motion();
	}

	bool prev_collided = collided;

	collided = GodotCollisionSolver2D::solve(shape_A_ptr, xform_A, motion_A, shape_B_ptr, xform_B, motion_B, _add_contact, this, &sep_axis);
	if (!collided) {
		oneway_disabled = false;

		if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
			check_ccd = true;
			return true;
		}

		if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
			check_ccd = true;
			return true;
		}

		return false;
	}

	if (oneway_disabled) {
		return false;
	}

	if (!prev_collided) {
		if (shape_B_ptr->allows_one_way_collision() && A->is_shape_set_as_one_way_collision(shape_A)) {
			Vector2 direction = xform_A.basis_xform(A->get_shape_one_way_collision_direction(shape_A)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) > -CMP_EPSILON) { // Greater (normal inverted).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}

		if (shape_A_ptr->allows_one_way_collision() && B->is_shape_set_as_one_way_collision(shape_B)) {
			Vector2 direction = xform_B.basis_xform(B->get_shape_one_way_collision_direction(shape_B)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) < CMP_EPSILON) { // Less (normal ok).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}
	}

	return true;
}

bool GodotBodyPair2D::pre_solve(real_t p_step) {
	if (oneway_disabled) {
		return false;
	}

	if (!collided) {
		if (check_ccd) {
			const Vector2 &offset_A = A->get_transform().get_origin();
			Transform2D xform_Au = A->get_transform().untranslated();
			Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

			Transform2D xform_Bu = B->get_transform();
			xform_Bu.columns[2] -= offset_A;
			Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

			if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
				_test_ccd(p_step, A, shape_A, xform_A, B, shape_B, xform_B);
			}

			if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
				_test_ccd(p_step, B, shape_B, xform_B, A, shape_A, xform_A);
			}
		}

		return false;
	}

	real_t max_penetration = space->get_contact_max_allowed_penetration();
	real_t bias = space->get_contact_bias();

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	if (shape_A_ptr->get_custom_bias() || shape_B_ptr->get_custom_bias()) {
		if (shape_A_ptr->get_custom_bias() == 0) {
			bias = shape_B_ptr->get_custom_bias();
		} else if (shape_B_ptr->get_custom_bias() == 0) {
			bias = shape_A_ptr->get_custom_bias();
		} else {
			bias = (shape_B_ptr->get_custom_bias() + shape_A_ptr->get_custom_bias()) * 0.5;
		}
	}

	real_t inv_dt = 1.0 / p_step;

	bool do_process = false;

	const Vector2 &offset_A = A->get_transform().get_origin();
	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	real_t inv_inertia_A = collide_A ? A->get_inv_inertia() : 0.0;
	real_t inv_inertia_B = collide_B ? B->get_inv_inertia() : 0.0;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		c.active = false;

		Vector2 global_A = transform_A.basis_xform(c.local_A);
		Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

		Vector2 axis = global_A - global_B;
		real_t depth = axis.dot(c.normal);

		if (depth <= 0.0) {
			continue;
		}

#ifdef DEBUG_ENABLED
		if (space->is_debugging_contacts()) {
			space->add_debug_contact(global_A + offset_A);
			space->add_debug_contact(global_B + offset_A);
		}
#endif

		c.rA = global_A - A->get_center_of_mass();
		c.rB = global_B - B->get_center_of_mass() - offset_B;

		// Precompute normal mass, tangent mass, and bias.
		real_t rnA = c.rA.dot(c.normal);
		real_t rnB = c.rB.dot(c.normal);
		real_t kNormal = inv_mass_A + inv_mass_B;
		kNormal += inv_inertia_A * (c.rA.dot(c.rA) - rnA * rnA) + inv_inertia_B * (c.rB.dot(c.rB) - rnB * rnB);
		c.mass_normal = 1.0f / kNormal;

		Vector2 tangent = c.normal.orthogonal();
		real_t rtA = c.rA.dot(tangent);
		real_t rtB = c.rB.dot(tangent);
		real_t kTangent = inv_mass_A + inv_mass_B;
		kTangent += inv_inertia_A * (c.rA.dot(c.rA) - rtA * rtA) + inv_inertia_B * (c.rB.dot(c.rB) - rtB * rtB);
		c.mass_tangent = 1.0f / kTangent;

		c.bias = -bias * inv_dt * MIN(0.0f, -depth + max_penetration);
		c.depth = depth;

		Vector2 P = c.acc_normal_impulse * c.normal + c.acc_tangent_impulse * tangent;

		c.acc_impulse -= P;

		if (report_contacts_only) {
			collided = false;
			continue;
		}

#ifdef ACCUMULATE_IMPULSES
		{
			// Apply normal + friction impulse
			if (collide_A) {
				A->apply_impulse(-P, c.rA + A->get_center_of_mass());
			}
			if (collide_B) {
				B->apply_impulse(P, c.rB + B->get_center_of_mass());
			}
		}
#endif

		c.bounce = combine_bounce(A, B);
		if (c.bounce) {
			Vector2 crA(-A->get_prev_angular_velocity() * c.rA.y, A->get_prev_angular_velocity() * c.rA.x);
			Vector2 crB(-B->get_prev_angular_velocity() * c.rB.y, B->get_prev_angular_velocity() * c.rB.x);
			Vector2 dv = B->get_prev_linear_velocity() + crB - A->get_prev_linear_velocity() - crA;
			c.bounce = c.bounce * dv.dot(c.normal);
		}

		c.active = true;
		do_process = true;
	}

	return do_process;
}

void GodotBodyPair2D::solve(real_t p_step) {
	if (!collided || oneway_disabled) {
		return;
	}

	const real_t max_bias_av = MAX_BIAS_ROTATION / p_step;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; ++i) {
		Contact &c = contacts[i];

		if (!c.active) {
			continue;
		}

		// Relative velocity at contact
		Vector2 crA(-A->get_angular_velocity() * c.rA.y, A->get_angular_velocity() * c.rA.x);
		Vector2 crB(-B->get_angular_velocity() * c.rB.y, B->get_angular_velocity() * c.rB.x);
		Vector2 dv = B->get_linear_velocity() + crB - A->get_linear_velocity() - crA;

		Vector2 crbA(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		Vector2 crbB(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		Vector2 dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		real_t vn = dv.dot(c.normal);
		real_t vbn = dbv.dot(c.normal);

		Vector2 tangent = c.normal.orthogonal();
		real_t vt = dv.dot(tangent);

		real_t jbn = (c.bias - vbn) * c.mass_normal;
		real_t jbnOld = c.acc_bias_impulse;
		c.acc_bias_impulse = MAX(jbnOld + jbn, 0.0f);

		Vector2 jb = c.normal * (c.acc_bias_impulse - jbnOld);

		if (collide_A) {
			A->apply_bias_impulse(-jb, c.rA + A->get_center_of_mass(), max_bias_av);
		}
		if (collide_B) {
			B->apply_bias_impulse(jb, c.rB + B->get_center_of_mass(), max_bias_av);
		}

		crbA = Vector2(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		crbB = Vector2(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		vbn = dbv.dot(c.normal);

		if (Math::abs(-vbn + c.bias) > MIN_VELOCITY) {
			real_t jbn_com = (-vbn + c.bias) / (inv_mass_A + inv_mass_B);
			real_t jbnOld_com = c.acc_bias_impulse_center_of_mass;
			c.acc_bias_impulse_center_of_mass = MAX(jbnOld_com + jbn_com, 0.0f);

			Vector2 jb_com = c.normal * (c.acc_bias_impulse_center_of_mass - jbnOld_com);

			if (collide_A) {
				A->apply_bias_impulse(-jb_com, A->get_center_of_mass(), 0.0f);
			}
			if (collide_B) {
				B->apply_bias_impulse(jb_com, B->get_center_of_mass(), 0.0f);
			}
		}

		real_t jn = -(c.bounce + vn) * c.mass_normal;
		real_t jnOld = c.acc_normal_impulse;
		c.acc_normal_impulse = MAX(jnOld + jn, 0.0f);

		real_t friction = combine_friction(A, B);

		real_t jtMax = friction * c.acc_normal_impulse;
		real_t jt = -vt * c.mass_tangent;
		real_t jtOld = c.acc_tangent_impulse;
		c.acc_tangent_impulse = CLAMP(jtOld + jt, -jtMax, jtMax);

		Vector2 j = c.normal * (c.acc_normal_impulse - jnOld) + tangent * (c.acc_tangent_impulse - jtOld);

		if (collide_A) {
			A->apply_impulse(-j, c.rA + A->get_center_of_mass());
		}
		if (collide_B) {
			B->apply_impulse(j, c.rB + B->get_center_of_mass());
		}
		c.acc_impulse -= j;
	}
}

GodotBodyPair2D::GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B) :
		GodotConstraint2D(_arr, 2) {
	A = p_A;
	B = p_B;
	shape_A = p_shape_A;
	shape_B = p_shape_B;
	space = A->get_space();
	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

GodotBodyPair2D::~GodotBodyPair2D() {
	A->remove_constraint(this, 0);
	B->remove_constraint(this, 1);
}

// ============================================================================
// GodotJoint2D (based on Chipmunk joint constraints)
// ============================================================================

/* Copyright (c) 2007 Scott Lembcke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

void GodotJoint2D::copy_settings_from(GodotJoint2D *p_joint) {
	set_self(p_joint->get_self());
	set_max_force(p_joint->get_max_force());
	set_bias(p_joint->get_bias());
	set_max_bias(p_joint->get_max_bias());
	disable_collisions_between_bodies(p_joint->is_disabled_collisions_between_bodies());
}

static inline real_t k_scalar(GodotBody2D *a, GodotBody2D *b, const Vector2 &rA, const Vector2 &rB, const Vector2 &n) {
	real_t value = 0.0;

	{
		value += a->get_inv_mass();
		real_t rcn = (rA - a->get_center_of_mass()).cross(n);
		value += a->get_inv_inertia() * rcn * rcn;
	}

	if (b) {
		value += b->get_inv_mass();
		real_t rcn = (rB - b->get_center_of_mass()).cross(n);
		value += b->get_inv_inertia() * rcn * rcn;
	}

	return value;
}

static inline Vector2 relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB) {
	Vector2 sum = a->get_linear_velocity() - (rA - a->get_center_of_mass()).orthogonal() * a->get_angular_velocity();
	if (b) {
		return (b->get_linear_velocity() - (rB - b->get_center_of_mass()).orthogonal() * b->get_angular_velocity()) - sum;
	} else {
		return -sum;
	}
}

static inline real_t normal_relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB, Vector2 n) {
	return relative_velocity(a, b, rA, rB).dot(n);
}

static inline Vector2 custom_cross(const Vector2 &p_vec, real_t p_other) {
	return Vector2(p_other * p_vec.y, -p_other * p_vec.x);
}

static inline void k_tensor(GodotBody2D *a, GodotBody2D *b, Vector2 r1, Vector2 r2, Vector2 *k1, Vector2 *k2) {
	real_t k11, k12, k21, k22;
	real_t m_sum = a->get_inv_mass() + b->get_inv_mass();

	// start with I*m_sum
	k11 = m_sum;
	k12 = 0.0f;
	k21 = 0.0f;
	k22 = m_sum;

	r1 -= a->get_center_of_mass();
	r2 -= b->get_center_of_mass();

	// add the influence from r1
	real_t a_i_inv = a->get_inv_inertia();
	real_t r1xsq = r1.x * r1.x * a_i_inv;
	real_t r1ysq = r1.y * r1.y * a_i_inv;
	real_t r1nxy = -r1.x * r1.y * a_i_inv;
	k11 += r1ysq;
	k12 += r1nxy;
	k21 += r1nxy;
	k22 += r1xsq;

	// add the influence from r2
	real_t b_i_inv = b->get_inv_inertia();
	real_t r2xsq = r2.x * r2.x * b_i_inv;
	real_t r2ysq = r2.y * r2.y * b_i_inv;
	real_t r2nxy = -r2.x * r2.y * b_i_inv;
	k11 += r2ysq;
	k12 += r2nxy;
	k21 += r2nxy;
	k22 += r2xsq;

	// invert
	real_t determinant = k11 * k22 - k12 * k21;
	ERR_FAIL_COND(determinant == 0.0);

	real_t det_inv = 1.0f / determinant;
	*k1 = Vector2(k22 * det_inv, -k12 * det_inv);
	*k2 = Vector2(-k21 * det_inv, k11 * det_inv);
}

static _FORCE_INLINE_ Vector2 mult_k(const Vector2 &vr, const Vector2 &k1, const Vector2 &k2) {
	return Vector2(vr.dot(k1), vr.dot(k2));
}

bool GodotJoint2D::setup(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _setup_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _setup_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _setup_damped_spring(p_step);
		default:
			return false;
	}
}

bool GodotJoint2D::pre_solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _pre_solve_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _pre_solve_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _pre_solve_damped_spring(p_step);
		default:
			return false;
	}
}

void GodotJoint2D::solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			_solve_pin(p_step);
			break;
		case PS2DE::JOINT_TYPE_GROOVE:
			_solve_groove(p_step);
			break;
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			_solve_damped_spring(p_step);
			break;
		default:
			break;
	}
}

// --- Pin Joint ---

bool GodotJoint2D::_setup_pin(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B && B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B ? B->get_transform().basis_xform(anchor_B) : anchor_B;

	real_t B_inv_mass = B ? B->get_inv_mass() : 0.0;

	Transform2D K1;
	K1[0].x = A->get_inv_mass() + B_inv_mass;
	K1[1].x = 0.0f;
	K1[0].y = 0.0f;
	K1[1].y = A->get_inv_mass() + B_inv_mass;

	Vector2 r1 = rA - A->get_center_of_mass();

	Transform2D K2;
	K2[0].x = A->get_inv_inertia() * r1.y * r1.y;
	K2[1].x = -A->get_inv_inertia() * r1.x * r1.y;
	K2[0].y = -A->get_inv_inertia() * r1.x * r1.y;
	K2[1].y = A->get_inv_inertia() * r1.x * r1.x;

	Transform2D K;
	K[0] = K1[0] + K2[0];
	K[1] = K1[1] + K2[1];

	if (B) {
		Vector2 r2 = rB - B->get_center_of_mass();

		Transform2D K3;
		K3[0].x = B->get_inv_inertia() * r2.y * r2.y;
		K3[1].x = -B->get_inv_inertia() * r2.x * r2.y;
		K3[0].y = -B->get_inv_inertia() * r2.x * r2.y;
		K3[1].y = B->get_inv_inertia() * r2.x * r2.x;

		K[0] += K3[0];
		K[1] += K3[1];
	}

	K[0].x += softness;
	K[1].y += softness;

	M = K.affine_inverse();

	Vector2 gA = rA + A->get_transform().get_origin();
	Vector2 gB = B ? rB + B->get_transform().get_origin() : rB;

	Vector2 delta = gB - gA;

	pin_bias = delta * -(get_bias() == 0 ? space->get_constraint_bias() : get_bias()) * (1.0 / p_step);

	// Compute max impulse.
	jn_max = get_max_force() * p_step;

	return true;
}

bool GodotJoint2D::_pre_solve_pin(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-P, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(P, rB);
	}
	real_t i_sum_local = A->get_inv_inertia();
	if (B) {
		i_sum_local += B->get_inv_inertia();
	}
	i_sum = 1.0 / (i_sum_local);
	if (angular_limit_enabled && B) {
		Vector2 diff_vector = B->get_transform().get_origin() - A->get_transform().get_origin();
		diff_vector = diff_vector.rotated(-initial_angle);
		real_t dist = diff_vector.angle();
		real_t pdist = 0.0;
		if (dist > angular_limit_upper) {
			pdist = dist - angular_limit_upper;
		} else if (dist < angular_limit_lower) {
			pdist = dist - angular_limit_lower;
		}
		real_t error_bias = Math::pow(1.0 - 0.15, 60.0);
		// Calculate bias velocity.
		bias_velocity = -CLAMP((-1.0 - Math::pow(error_bias, p_step)) * pdist / p_step, -get_max_bias(), get_max_bias());
		// If the bias velocity is 0, the joint is not at a limit.
		if (bias_velocity >= -CMP_EPSILON && bias_velocity <= CMP_EPSILON) {
			j_acc = 0;
			is_joint_at_limit = false;
		} else {
			is_joint_at_limit = true;
		}
	} else {
		bias_velocity = 0.0;
	}

	return true;
}

void GodotJoint2D::_solve_pin(real_t p_step) {
	// Compute relative velocity.
	Vector2 vA = A->get_linear_velocity() - custom_cross(rA - A->get_center_of_mass(), A->get_angular_velocity());

	Vector2 rel_vel;
	if (B) {
		rel_vel = B->get_linear_velocity() - custom_cross(rB - B->get_center_of_mass(), B->get_angular_velocity()) - vA;
	} else {
		rel_vel = -vA;
	}
	if ((angular_limit_enabled || motor_enabled) && B) {
		// Compute relative rotational velocity.
		real_t wr = B->get_angular_velocity() - A->get_angular_velocity();
		if (motor_enabled) {
			wr -= motor_target_velocity;
		}
		real_t j_max = jn_max;

		// Compute normal impulse.
		real_t j_pin = -(bias_velocity + wr) * i_sum;
		real_t j_old = j_acc;
		// Only enable the limits if we have to.
		if (angular_limit_enabled && is_joint_at_limit) {
			if (bias_velocity < 0.0) {
				j_acc = CLAMP(j_old + j_pin, 0.0, j_max);
			} else {
				j_acc = CLAMP(j_old + j_pin, -j_max, 0.0);
			}
		} else {
			j_acc = CLAMP(j_old + j_pin, -j_max, j_max);
		}
		j_pin = j_acc - j_old;
		A->apply_torque_impulse(-j_pin * A->get_inv_inertia());
		B->apply_torque_impulse(j_pin * B->get_inv_inertia());
	}

	Vector2 impulse = M.basis_xform(pin_bias - rel_vel - Vector2(softness, softness) * P);

	if (dynamic_A) {
		A->apply_impulse(-impulse, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(impulse, rB);
	}

	P += impulse;
}

void GodotJoint2D::set_param(PS2DE::PinJointParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			softness = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			angular_limit_upper = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			angular_limit_lower = p_value;
		} break;
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			motor_target_velocity = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::PinJointParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, 0);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			return softness;
		}
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			return angular_limit_upper;
		}
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			return angular_limit_lower;
		}
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			return motor_target_velocity;
		}
	}
	ERR_FAIL_V(0);
}

void GodotJoint2D::set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			angular_limit_enabled = p_enabled;
		} break;
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			motor_enabled = p_enabled;
		} break;
	}
}

bool GodotJoint2D::get_flag(PS2DE::PinJointFlag p_flag) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, false);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			return angular_limit_enabled;
		}
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			return motor_enabled;
		}
	}
	ERR_FAIL_V(false);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, p_body_b ? 2 : 1),
		type(PS2DE::JOINT_TYPE_PIN) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = p_body_a->get_inv_transform().xform(p_pos);
	anchor_B = p_body_b ? p_body_b->get_inv_transform().xform(p_pos) : p_pos;

	p_body_a->add_constraint(this, 0);
	if (p_body_b) {
		p_body_b->add_constraint(this, 1);
		initial_angle = A->get_transform().get_origin().angle_to_point(B->get_transform().get_origin());
	}
}

// --- Groove Joint ---

bool GodotJoint2D::_setup_groove(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	// calculate endpoints in worldspace
	Vector2 ta = A->get_transform().xform(A_groove_1);
	Vector2 tb = A->get_transform().xform(A_groove_2);

	// calculate axis
	Vector2 axis_n = -(tb - ta).orthogonal().normalized();
	real_t d = ta.dot(axis_n);

	xf_normal = axis_n;
	rB = B->get_transform().basis_xform(B_anchor);

	// calculate tangential distance along the axis of rB
	real_t td = (B->get_transform().get_origin() + rB).cross(axis_n);
	// calculate clamping factor and rB
	if (td <= ta.cross(axis_n)) {
		clamp = 1.0f;
		rA = ta - A->get_transform().get_origin();
	} else if (td >= tb.cross(axis_n)) {
		clamp = -1.0f;
		rA = tb - A->get_transform().get_origin();
	} else {
		clamp = 0.0f;
		rA = ((-axis_n.orthogonal() * -td) + axis_n * d) - A->get_transform().get_origin();
	}

	// Calculate mass tensor
	k_tensor(A, B, rA, rB, &k1, &k2);

	// compute max impulse
	jn_max = get_max_force() * p_step;

	// calculate bias velocity
	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);

	real_t _b = get_bias();
	gbias = (delta * -(_b == 0 ? space->get_constraint_bias() : _b) * (1.0 / p_step)).limit_length(get_max_bias());

	correct = true;
	return true;
}

bool GodotJoint2D::_pre_solve_groove(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-jn_acc, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(jn_acc, rB);
	}

	return true;
}

void GodotJoint2D::_solve_groove(real_t p_step) {
	// compute impulse
	Vector2 vr = relative_velocity(A, B, rA, rB);

	Vector2 j_groove = mult_k(gbias - vr, k1, k2);
	Vector2 jOld = jn_acc;
	j_groove += jOld;

	jn_acc = (((clamp * j_groove.cross(xf_normal)) > 0) ? j_groove : j_groove.project(xf_normal)).limit_length(jn_max);

	j_groove = jn_acc - jOld;

	if (dynamic_A) {
		A->apply_impulse(-j_groove, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_groove, rB);
	}
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_GROOVE) {
	A = p_body_a;
	B = p_body_b;

	A_groove_1 = A->get_inv_transform().xform(p_a_groove1);
	A_groove_2 = A->get_inv_transform().xform(p_a_groove2);
	B_anchor = B->get_inv_transform().xform(p_b_anchor);
	A_groove_normal = -(A_groove_2 - A_groove_1).normalized().orthogonal();

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

// --- Damped Spring Joint ---

bool GodotJoint2D::_setup_damped_spring(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B->get_transform().basis_xform(anchor_B);

	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);
	real_t dist = delta.length();

	if (dist) {
		n = delta / dist;
	} else {
		n = Vector2();
	}

	real_t k = k_scalar(A, B, rA, rB, n);
	n_mass = 1.0f / k;

	target_vrn = 0.0f;
	v_coef = 1.0f - Math::exp(-damping * (p_step)*k);

	// Calculate spring force.
	real_t f_spring = (rest_length - dist) * stiffness;
	j = n * f_spring * (p_step);

	return true;
}

bool GodotJoint2D::_pre_solve_damped_spring(real_t p_step) {
	// Apply spring force.
	if (dynamic_A) {
		A->apply_impulse(-j, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j, rB);
	}

	return true;
}

void GodotJoint2D::_solve_damped_spring(real_t p_step) {
	// compute relative velocity
	real_t vrn = normal_relative_velocity(A, B, rA, rB, n) - target_vrn;

	// compute velocity loss from drag
	real_t v_damp = -vrn * v_coef;
	target_vrn = vrn + v_damp;
	Vector2 j_new = n * v_damp * n_mass;

	if (dynamic_A) {
		A->apply_impulse(-j_new, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_new, rB);
	}
}

void GodotJoint2D::set_param(PS2DE::DampedSpringParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_DAMPED_SPRING);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			rest_length = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			damping = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			stiffness = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::DampedSpringParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			return rest_length;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			return damping;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			return stiffness;
		} break;
	}

	ERR_FAIL_V(0);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_DAMPED_SPRING) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = A->get_inv_transform().xform(p_anchor_a);
	anchor_B = B->get_inv_transform().xform(p_anchor_b);

	rest_length = p_anchor_a.distance_to(p_anchor_b);

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}
#endif // PHYSICS_2D_DISABLED
