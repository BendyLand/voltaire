/**************************************************************************/
/*  physics_server_3d.h                                                   */
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
// --- Folded from servers/physics_3d/direct_states/physics_direct_body_state_3d.h ---
// --- Folded from servers/physics_3d/direct_states/physics_direct_space_state_3d.h ---
// --- Folded from servers/physics_3d/physics_server_3d_types.h ---
#include "core/math/vector3.h"
#include "core/templates/hash_set.h"
#include "core/templates/rid.h"
#include "core/types.h"

namespace PhysicsServer3DTypes
{

struct RayParameters
{
	Vector3 from;
	Vector3 to;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;

	bool hit_from_inside = false;
	bool hit_back_faces = true;

	bool pick_ray = false;
};

struct RayResult
{
	Vector3 position;
	Vector3 normal;
	RID rid;
	int shape = 0;
	int face_index = -1;
};

struct ShapeResult
{
	RID rid;
	int shape = 0;
};

struct PointParameters
{
	Vector3 position;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;
};

struct ShapeParameters
{
	RID shape_rid;
	Transform3D transform;
	Vector3 motion;
	real_t margin = 0.0;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;
};

struct ShapeRestInfo
{
	Vector3 point;
	Vector3 normal;
	RID rid;
	int shape = 0;
	Vector3 linear_velocity; // Velocity at contact point.
};

struct MotionParameters
{
	Transform3D from;
	Vector3 motion;
	real_t margin = 0.001;
	int max_collisions = 1;
	bool collide_separation_ray = false;
	HashSet<RID> exclude_bodies;
	bool recovery_as_collision = false;

	MotionParameters() {}

	MotionParameters(const Transform3D& p_from, const Vector3& p_motion, real_t p_margin = 0.001)
		: from(p_from), motion(p_motion), margin(p_margin)
	{
	}
};

struct MotionCollision
{
	Vector3 position;
	Vector3 normal;
	Vector3 collider_velocity;
	Vector3 collider_angular_velocity;
	real_t depth = 0.0;
	int local_shape = 0;
	RID collider;
	int collider_shape = 0;

	real_t get_angle(Vector3 p_up_direction) const
	{
		return Math::acos(normal.dot(p_up_direction));
	}
};

struct MotionResult
{
	Vector3 travel;
	Vector3 remainder;
	real_t collision_depth = 0.0;
	real_t collision_safe_fraction = 0.0;
	real_t collision_unsafe_fraction = 0.0;

	static const int MAX_COLLISIONS = 32;
	MotionCollision collisions[MAX_COLLISIONS];
	int collision_count = 0;
};

#ifndef DISABLE_DEPRECATED
// Graveyard.
#endif

} // namespace PhysicsServer3DTypes

// Alias to make it easier to use.
#define PS3DT PhysicsServer3DTypes



// --- End servers/physics_3d/physics_server_3d_types.h ---
// --- Folded from servers/physics_3d/queries/physics_point_query_parameters_3d.h ---

#include "core/types.h"

class PhysicsPointQueryParameters3D : public RefCounted
{
	PS3DT::PointParameters parameters;

public:
	const PS3DT::PointParameters& get_parameters() const { return parameters; }

	void set_position(const Vector3& p_position) { parameters.position = p_position; }

	const Vector3& get_position() const { return parameters.position; }

	void set_collision_mask(uint32_t p_mask) { parameters.collision_mask = p_mask; }

	uint32_t get_collision_mask() const { return parameters.collision_mask; }

	void set_collide_with_bodies(bool p_enable) { parameters.collide_with_bodies = p_enable; }

	bool is_collide_with_bodies_enabled() const { return parameters.collide_with_bodies; }

	void set_collide_with_areas(bool p_enable) { parameters.collide_with_areas = p_enable; }

	bool is_collide_with_areas_enabled() const { return parameters.collide_with_areas; }
};



// --- End servers/physics_3d/queries/physics_point_query_parameters_3d.h ---
// --- Folded from servers/physics_3d/queries/physics_ray_query_parameters_3d.h ---

#include "core/types.h"

class PhysicsRayQueryParameters3D : public RefCounted
{
	PS3DT::RayParameters parameters;

public:
	const PS3DT::RayParameters& get_parameters() const { return parameters; }

	void set_from(const Vector3& p_from) { parameters.from = p_from; }

	const Vector3& get_from() const { return parameters.from; }

	void set_to(const Vector3& p_to) { parameters.to = p_to; }

	const Vector3& get_to() const { return parameters.to; }

	void set_collision_mask(uint32_t p_mask) { parameters.collision_mask = p_mask; }

	uint32_t get_collision_mask() const { return parameters.collision_mask; }

	void set_collide_with_bodies(bool p_enable) { parameters.collide_with_bodies = p_enable; }

	bool is_collide_with_bodies_enabled() const { return parameters.collide_with_bodies; }

	void set_collide_with_areas(bool p_enable) { parameters.collide_with_areas = p_enable; }

	bool is_collide_with_areas_enabled() const { return parameters.collide_with_areas; }

	void set_hit_from_inside(bool p_enable) { parameters.hit_from_inside = p_enable; }

	bool is_hit_from_inside_enabled() const { return parameters.hit_from_inside; }

	void set_hit_back_faces(bool p_enable) { parameters.hit_back_faces = p_enable; }

	bool is_hit_back_faces_enabled() const { return parameters.hit_back_faces; }
};



// --- End servers/physics_3d/queries/physics_ray_query_parameters_3d.h ---
// --- Folded from servers/physics_3d/queries/physics_shape_query_parameters_3d.h ---
#include "core/types.h"


class Resource;

class PhysicsShapeQueryParameters3D : public RefCounted
{
	PS3DT::ShapeParameters parameters;

	Ref<Resource> shape_ref;

protected:

public:
	const PS3DT::ShapeParameters& get_parameters() const { return parameters; }

	void set_shape(const Ref<Resource>& p_shape_ref);
	Ref<Resource> get_shape() const;

	void set_shape_rid(const RID& p_shape);

	RID get_shape_rid() const { return parameters.shape_rid; }

	void set_transform(const Transform3D& p_transform) { parameters.transform = p_transform; }

	const Transform3D& get_transform() const { return parameters.transform; }

	void set_motion(const Vector3& p_motion) { parameters.motion = p_motion; }

	const Vector3& get_motion() const { return parameters.motion; }

	void set_margin(real_t p_margin) { parameters.margin = p_margin; }

	real_t get_margin() const { return parameters.margin; }

	void set_collision_mask(uint32_t p_mask) { parameters.collision_mask = p_mask; }

	uint32_t get_collision_mask() const { return parameters.collision_mask; }

	void set_collide_with_bodies(bool p_enable) { parameters.collide_with_bodies = p_enable; }

	bool is_collide_with_bodies_enabled() const { return parameters.collide_with_bodies; }

	void set_collide_with_areas(bool p_enable) { parameters.collide_with_areas = p_enable; }

	bool is_collide_with_areas_enabled() const { return parameters.collide_with_areas; }
};



// --- End servers/physics_3d/queries/physics_shape_query_parameters_3d.h ---

class JoltBody3D;
class JoltSpace3D;

class PhysicsDirectSpaceState3D final
{
	JoltSpace3D* space = nullptr;

	Vector<real_t> _cast_motion(PhysicsShapeQueryParameters3D* rp_shape_query);

public:
	PhysicsDirectSpaceState3D() = default;
	explicit PhysicsDirectSpaceState3D(JoltSpace3D* p_space);

	bool intersect_ray(
		const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result);
	int intersect_point(const PS3DT::PointParameters& p_parameters,
		PS3DT::ShapeResult* r_results, int p_result_max);
	int intersect_shape(const PS3DT::ShapeParameters& p_parameters,
		PS3DT::ShapeResult* r_results, int p_result_max);
	bool cast_motion(const PS3DT::ShapeParameters& p_parameters, real_t& r_closest_safe,
		real_t& r_closest_unsafe, PS3DT::ShapeRestInfo* r_info = nullptr);
	bool collide_shape(const PS3DT::ShapeParameters& p_parameters, Vector3* r_results,
		int p_result_max, int& r_result_count);
	bool rest_info(
		const PS3DT::ShapeParameters& p_parameters, PS3DT::ShapeRestInfo* r_info);
	Vector3 get_closest_point_to_object_volume(
		RID p_object, Vector3 p_point) const;

	bool body_test_motion(const JoltBody3D& p_body, const PS3DT::MotionParameters& p_parameters,
		PS3DT::MotionResult* r_result) const;

	JoltSpace3D& get_space() const { return *space; }
};

// --- End servers/physics_3d/direct_states/physics_direct_space_state_3d.h ---

class JoltBody3D;

class PhysicsDirectBodyState3D final
{
	JoltBody3D* body = nullptr;

public:
	PhysicsDirectBodyState3D() = default;
	explicit PhysicsDirectBodyState3D(JoltBody3D* p_body);

	Vector3 get_total_gravity() const;
	real_t get_total_angular_damp() const;
	real_t get_total_linear_damp() const;

	Vector3 get_center_of_mass() const;
	Vector3 get_center_of_mass_local() const;
	Basis get_principal_inertia_axes() const;
	real_t get_inverse_mass() const;
	Vector3 get_inverse_inertia() const;
	Basis get_inverse_inertia_tensor() const;

	void set_linear_velocity(const Vector3& p_velocity);
	Vector3 get_linear_velocity() const;

	void set_angular_velocity(const Vector3& p_velocity);
	Vector3 get_angular_velocity() const;

	void set_transform(const Transform3D& p_transform);
	Transform3D get_transform() const;

	Vector3 get_velocity_at_local_position(const Vector3& p_position) const;

	void apply_central_impulse(const Vector3& p_impulse);
	void apply_impulse(const Vector3& p_impulse, const Vector3& p_position = Vector3());
	void apply_torque_impulse(const Vector3& p_impulse);

	void apply_central_force(const Vector3& p_force);
	void apply_force(const Vector3& p_force, const Vector3& p_position = Vector3());
	void apply_torque(const Vector3& p_torque);

	void add_constant_central_force(const Vector3& p_force);
	void add_constant_force(const Vector3& p_force, const Vector3& p_position = Vector3());
	void add_constant_torque(const Vector3& p_torque);

	void set_constant_force(const Vector3& p_force);
	Vector3 get_constant_force() const;

	void set_constant_torque(const Vector3& p_torque);
	Vector3 get_constant_torque() const;

	void set_sleep_state(bool p_sleep);
	bool is_sleeping() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	int get_contact_count() const;

	Vector3 get_contact_local_position(int p_contact_idx) const;
	Vector3 get_contact_local_normal(int p_contact_idx) const;
	Vector3 get_contact_impulse(int p_contact_idx) const;
	int get_contact_local_shape(int p_contact_idx) const;
	Vector3 get_contact_local_velocity_at_position(int p_contact_idx) const;

	RID get_contact_collider(int p_contact_idx) const;
	Vector3 get_contact_collider_position(int p_contact_idx) const;
	int get_contact_collider_shape(int p_contact_idx) const;
	Vector3 get_contact_collider_velocity_at_position(int p_contact_idx) const;

	real_t get_step() const;
	void integrate_forces();

	PhysicsDirectSpaceState3D* get_space_state();
};

// --- End servers/physics_3d/direct_states/physics_direct_body_state_3d.h ---

// --- Folded from servers/physics_3d/physics_server_3d_enums.h ---
namespace PhysicsServer3DEnums
{

/* SHAPE API */

enum ShapeType
{
	SHAPE_WORLD_BOUNDARY,  ///< plane:"plane"
	SHAPE_SEPARATION_RAY,  ///< float:"length"
	SHAPE_SPHERE,		   ///< float:"radius"
	SHAPE_BOX,			   ///< vec3:"extents"
	SHAPE_CAPSULE,		   ///< dict( float:"radius", float:"height"):capsule
	SHAPE_CYLINDER,		   ///< dict( float:"radius", float:"height"):cylinder
	SHAPE_CONVEX_POLYGON,  ///< array of planes:"planes"
	SHAPE_CONCAVE_POLYGON, ///< vector3 array:"triangles" , or Dictionary with "indices" (int array)
						   ///< and "triangles" (Vector3 array)
	SHAPE_HEIGHTMAP, ///< dict( int:"width", int:"depth",float:"cell_size", float_array:"heights"
	SHAPE_SOFT_BODY, ///< Used internally, can't be created from the physics server.
	SHAPE_CUSTOM,	 ///< Server-Implementation based custom shape, calling shape_create() with this
					 ///< value will result in an error
};

/* SPACE API */

enum SpaceParameter
{
	SPACE_PARAM_CONTACT_RECYCLE_RADIUS,
	SPACE_PARAM_CONTACT_MAX_SEPARATION,
	SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION,
	SPACE_PARAM_CONTACT_DEFAULT_BIAS,
	SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD,
	SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD,
	SPACE_PARAM_BODY_TIME_TO_SLEEP,
	SPACE_PARAM_SOLVER_ITERATIONS,
};

/* AREA API */

enum AreaParameter
{
	AREA_PARAM_GRAVITY_OVERRIDE_MODE,
	AREA_PARAM_GRAVITY,
	AREA_PARAM_GRAVITY_VECTOR,
	AREA_PARAM_GRAVITY_IS_POINT,
	AREA_PARAM_GRAVITY_POINT_UNIT_DISTANCE,
	AREA_PARAM_LINEAR_DAMP_OVERRIDE_MODE,
	AREA_PARAM_LINEAR_DAMP,
	AREA_PARAM_ANGULAR_DAMP_OVERRIDE_MODE,
	AREA_PARAM_ANGULAR_DAMP,
	AREA_PARAM_PRIORITY,
	AREA_PARAM_WIND_FORCE_MAGNITUDE,
	AREA_PARAM_WIND_SOURCE,
	AREA_PARAM_WIND_DIRECTION,
	AREA_PARAM_WIND_ATTENUATION_FACTOR,
};

enum AreaSpaceOverrideMode
{
	AREA_SPACE_OVERRIDE_DISABLED,
	AREA_SPACE_OVERRIDE_COMBINE,
	AREA_SPACE_OVERRIDE_COMBINE_REPLACE,
	AREA_SPACE_OVERRIDE_REPLACE,
	AREA_SPACE_OVERRIDE_REPLACE_COMBINE
};

/* BODY API */

enum BodyMode
{
	BODY_MODE_STATIC,
	BODY_MODE_KINEMATIC,
	BODY_MODE_RIGID,
	BODY_MODE_RIGID_LINEAR,
};

enum BodyDampMode
{
	BODY_DAMP_MODE_COMBINE,
	BODY_DAMP_MODE_REPLACE,
};

enum BodyParameter
{
	BODY_PARAM_BOUNCE,
	BODY_PARAM_FRICTION,
	BODY_PARAM_MASS, ///< unused for static, always infinite
	BODY_PARAM_INERTIA,
	BODY_PARAM_CENTER_OF_MASS,
	BODY_PARAM_GRAVITY_SCALE,
	BODY_PARAM_LINEAR_DAMP_MODE,
	BODY_PARAM_ANGULAR_DAMP_MODE,
	BODY_PARAM_LINEAR_DAMP,
	BODY_PARAM_ANGULAR_DAMP,
	BODY_PARAM_MAX,
};

enum BodyState
{
	BODY_STATE_TRANSFORM,
	BODY_STATE_LINEAR_VELOCITY,
	BODY_STATE_ANGULAR_VELOCITY,
	BODY_STATE_SLEEPING,
	BODY_STATE_CAN_SLEEP
};

enum BodyAxis
{
	BODY_AXIS_LINEAR_X = 1 << 0,
	BODY_AXIS_LINEAR_Y = 1 << 1,
	BODY_AXIS_LINEAR_Z = 1 << 2,
	BODY_AXIS_ANGULAR_X = 1 << 3,
	BODY_AXIS_ANGULAR_Y = 1 << 4,
	BODY_AXIS_ANGULAR_Z = 1 << 5
};

/* JOINT API */

enum JointType
{
	JOINT_TYPE_PIN,
	JOINT_TYPE_HINGE,
	JOINT_TYPE_SLIDER,
	JOINT_TYPE_CONE_TWIST,
	JOINT_TYPE_6DOF,
	JOINT_TYPE_MAX,

};

enum PinJointParam
{
	PIN_JOINT_BIAS,
	PIN_JOINT_DAMPING,
	PIN_JOINT_IMPULSE_CLAMP
};

enum HingeJointParam
{
	HINGE_JOINT_BIAS,
	HINGE_JOINT_LIMIT_UPPER,
	HINGE_JOINT_LIMIT_LOWER,
	HINGE_JOINT_LIMIT_BIAS,
	HINGE_JOINT_LIMIT_SOFTNESS,
	HINGE_JOINT_LIMIT_RELAXATION,
	HINGE_JOINT_MOTOR_TARGET_VELOCITY,
	HINGE_JOINT_MOTOR_MAX_IMPULSE,
	HINGE_JOINT_MAX
};

enum HingeJointFlag
{
	HINGE_JOINT_FLAG_USE_LIMIT,
	HINGE_JOINT_FLAG_ENABLE_MOTOR,
	HINGE_JOINT_FLAG_MAX
};

enum SliderJointParam
{
	SLIDER_JOINT_LINEAR_LIMIT_UPPER,
	SLIDER_JOINT_LINEAR_LIMIT_LOWER,
	SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS,
	SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION,
	SLIDER_JOINT_LINEAR_LIMIT_DAMPING,
	SLIDER_JOINT_LINEAR_MOTION_SOFTNESS,
	SLIDER_JOINT_LINEAR_MOTION_RESTITUTION,
	SLIDER_JOINT_LINEAR_MOTION_DAMPING,
	SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS,
	SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION,
	SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING,

	SLIDER_JOINT_ANGULAR_LIMIT_UPPER,
	SLIDER_JOINT_ANGULAR_LIMIT_LOWER,
	SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS,
	SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION,
	SLIDER_JOINT_ANGULAR_LIMIT_DAMPING,
	SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS,
	SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION,
	SLIDER_JOINT_ANGULAR_MOTION_DAMPING,
	SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS,
	SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION,
	SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING,
	SLIDER_JOINT_MAX
};

enum ConeTwistJointParam
{
	CONE_TWIST_JOINT_SWING_SPAN,
	CONE_TWIST_JOINT_TWIST_SPAN,
	CONE_TWIST_JOINT_BIAS,
	CONE_TWIST_JOINT_SOFTNESS,
	CONE_TWIST_JOINT_RELAXATION,
	CONE_TWIST_MAX
};

enum G6DOFJointAxisParam
{
	G6DOF_JOINT_LINEAR_LOWER_LIMIT,
	G6DOF_JOINT_LINEAR_UPPER_LIMIT,
	G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS,
	G6DOF_JOINT_LINEAR_RESTITUTION,
	G6DOF_JOINT_LINEAR_DAMPING,
	G6DOF_JOINT_LINEAR_MOTOR_TARGET_VELOCITY,
	G6DOF_JOINT_LINEAR_MOTOR_FORCE_LIMIT,
	G6DOF_JOINT_LINEAR_SPRING_STIFFNESS,
	G6DOF_JOINT_LINEAR_SPRING_DAMPING,
	G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT,
	G6DOF_JOINT_ANGULAR_LOWER_LIMIT,
	G6DOF_JOINT_ANGULAR_UPPER_LIMIT,
	G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS,
	G6DOF_JOINT_ANGULAR_DAMPING,
	G6DOF_JOINT_ANGULAR_RESTITUTION,
	G6DOF_JOINT_ANGULAR_FORCE_LIMIT,
	G6DOF_JOINT_ANGULAR_ERP,
	G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY,
	G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT,
	G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS,
	G6DOF_JOINT_ANGULAR_SPRING_DAMPING,
	G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
	G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT,
	G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT,
	G6DOF_JOINT_MAX
};

enum G6DOFJointAxisFlag
{
	G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT,
	G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT,
	G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING,
	G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING,
	G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR,
#ifndef DISABLE_DEPRECATED
	G6DOF_JOINT_FLAG_ENABLE_MOTOR = G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR,
#endif // DISABLE_DEPRECATED
	G6DOF_JOINT_FLAG_ENABLE_LINEAR_MOTOR,
	G6DOF_JOINT_FLAG_MAX
};

/* QUERY API */

enum AreaBodyStatus
{
	AREA_BODY_ADDED,
	AREA_BODY_REMOVED
};

enum ProcessInfo
{
	INFO_ACTIVE_OBJECTS,
	INFO_COLLISION_PAIRS,
	INFO_ISLAND_COUNT
};

#ifndef DISABLE_DEPRECATED
// Graveyard.
#endif

} // namespace PhysicsServer3DEnums

// Alias to make it easier to use.
#define PS3DE PhysicsServer3DEnums



// --- End servers/physics_3d/physics_server_3d_enums.h ---
// --- Folded from servers/physics_3d/physics_server_3d_rendering_server_handler.h ---
#include "core/math/aabb.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"

class PhysicsServer3DRenderingServerHandler final
{
	RID mesh;
	int surface = 0;
	Vector<uint8_t> buffer;
	uint32_t stride = 0;
	uint32_t normal_stride = 0;
	uint32_t offset_vertices = 0;
	uint32_t offset_normal = 0;

	uint8_t* write_buffer = nullptr;

public:
	PhysicsServer3DRenderingServerHandler() = default;
	~PhysicsServer3DRenderingServerHandler() = default;

	bool is_ready(RID p_mesh_rid) const { return mesh.is_valid() && mesh == p_mesh_rid; }

	void prepare(RID p_mesh_rid, int p_surface);
	void clear();
	void open();
	void close();
	void commit_changes();

	void set_vertex(int p_vertex_id, const Vector3& p_vertex);
	void set_normal(int p_vertex_id, const Vector3& p_normal);
	void set_aabb(const AABB& p_aabb);
};

// --- End servers/physics_3d/physics_server_3d_rendering_server_handler.h ---

// --- Folded from servers/physics_3d/queries/physics_testmotion_query_parameters_3d.h ---
#include "core/types.h"


class PhysicsTestMotionParameters3D : public RefCounted
{
	PS3DT::MotionParameters parameters;

public:
	const PS3DT::MotionParameters& get_parameters() const { return parameters; }

	const Transform3D& get_from() const { return parameters.from; }

	void set_from(const Transform3D& p_from) { parameters.from = p_from; }

	const Vector3& get_motion() const { return parameters.motion; }

	void set_motion(const Vector3& p_motion) { parameters.motion = p_motion; }

	real_t get_margin() const { return parameters.margin; }

	void set_margin(real_t p_margin) { parameters.margin = p_margin; }

	int get_max_collisions() const { return parameters.max_collisions; }

	void set_max_collisions(int p_max_collisions) { parameters.max_collisions = p_max_collisions; }

	bool is_collide_separation_ray_enabled() const { return parameters.collide_separation_ray; }

	void set_collide_separation_ray_enabled(bool p_enabled)
	{
		parameters.collide_separation_ray = p_enabled;
	}

	bool is_recovery_as_collision_enabled() const { return parameters.recovery_as_collision; }

	void set_recovery_as_collision_enabled(bool p_enabled)
	{
		parameters.recovery_as_collision = p_enabled;
	}
};



// --- End servers/physics_3d/queries/physics_testmotion_query_parameters_3d.h ---
// --- Folded from servers/physics_3d/queries/physics_testmotion_query_result_3d.h ---
#include "core/types.h"


class PhysicsTestMotionResult3D : public RefCounted
{
	PS3DT::MotionResult result;

protected:

public:
	PS3DT::MotionResult* get_result_ptr() { return &result; }

	Vector3 get_travel() const;
	Vector3 get_remainder() const;
	real_t get_collision_safe_fraction() const;
	real_t get_collision_unsafe_fraction() const;

	int get_collision_count() const;

	Vector3 get_collision_point(int p_collision_index = 0) const;
	Vector3 get_collision_normal(int p_collision_index = 0) const;
	Vector3 get_collider_velocity(int p_collision_index = 0) const;
	RID get_collider_rid(int p_collision_index = 0) const;
	int get_collider_shape(int p_collision_index = 0) const;
	int get_collision_local_shape(int p_collision_index = 0) const;
	real_t get_collision_depth(int p_collision_index = 0) const;
};



// --- End servers/physics_3d/queries/physics_testmotion_query_result_3d.h ---

class JoltPhysicsServer3D;
class PhysicsDirectSpaceState3D;
template <typename T> class TypedArray;

template <typename T> class Ref;

class PhysicsTestMotionParameters3D;
class PhysicsTestMotionResult3D;

class PhysicsServer3D final
{
public:
	struct Data
	{
		JoltPhysicsServer3D* server = nullptr;
	};

	static inline Data* data = nullptr;

	static void initialize(bool p_on_separate_thread = false);
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->server != nullptr;
	}
	_FORCE_INLINE_ static JoltPhysicsServer3D* get_driver() { return data ? data->server : nullptr; }

	static void soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler);
	static RID shape_create(PS3DE::ShapeType p_shape);
	static RID world_boundary_shape_create();
	static RID separation_ray_shape_create();
	static RID sphere_shape_create();
	static RID box_shape_create();
	static RID capsule_shape_create();
	static RID cylinder_shape_create();
	static RID convex_polygon_shape_create();
	static RID concave_polygon_shape_create();
	static RID heightmap_shape_create();
	static RID custom_shape_create();
	static void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);
	static PS3DE::ShapeType shape_get_type(RID p_shape);
	static void shape_set_margin(RID p_shape, real_t p_margin);
	static real_t shape_get_margin(RID p_shape);
	static real_t shape_get_custom_solver_bias(RID p_shape);
	static RID space_create();
	static void space_set_active(RID p_space, bool p_active);
	static bool space_is_active(RID p_space);
	static void space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value);
	static real_t space_get_param(RID p_space, PS3DE::SpaceParameter p_param);
	static PhysicsDirectSpaceState3D* space_get_direct_state(RID p_space);
	static void space_set_debug_contacts(RID p_space, int p_max_contacts);
	static Vector<Vector3> space_get_contacts(RID p_space);
	static int space_get_contact_count(RID p_space);
	static RID area_create();
	static void area_set_space(RID p_area, RID p_space);
	static RID area_get_space(RID p_area);
	static void area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform = Transform3D(), bool p_disabled = false);
	static void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	static void area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform);
	static int area_get_shape_count(RID p_area);
	static RID area_get_shape(RID p_area, int p_shape_idx);
	static Transform3D area_get_shape_transform(RID p_area, int p_shape_idx);
	static void area_remove_shape(RID p_area, int p_shape_idx);
	static void area_clear_shapes(RID p_area);
	static void area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled);
	static void area_set_transform(RID p_area, const Transform3D& p_transform);
	static Transform3D area_get_transform(RID p_area);
	static void area_set_collision_layer(RID p_area, uint32_t p_layer);
	static uint32_t area_get_collision_layer(RID p_area);
	static void area_set_collision_mask(RID p_area, uint32_t p_mask);
	static uint32_t area_get_collision_mask(RID p_area);
	static void area_set_monitorable(RID p_area, bool p_monitorable);
	static void area_set_ray_pickable(RID p_area, bool p_enable);
	static RID body_create();
	static void body_set_space(RID p_body, RID p_space);
	static RID body_get_space(RID p_body);
	static void body_set_mode(RID p_body, PS3DE::BodyMode p_mode);
	static PS3DE::BodyMode body_get_mode(RID p_body);
	static void body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform = Transform3D(), bool p_disabled = false);
	static void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	static void body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform);
	static int body_get_shape_count(RID p_body);
	static RID body_get_shape(RID p_body, int p_shape_idx);
	static Transform3D body_get_shape_transform(RID p_body, int p_shape_idx);
	static void body_remove_shape(RID p_body, int p_shape_idx);
	static void body_clear_shapes(RID p_body);
	static void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);
	static void body_set_enable_continuous_collision_detection(RID p_body, bool p_enable);
	static bool body_is_continuous_collision_detection_enabled(RID p_body);
	static void body_set_collision_layer(RID p_body, uint32_t p_layer);
	static uint32_t body_get_collision_layer(RID p_body);
	static void body_set_collision_mask(RID p_body, uint32_t p_mask);
	static uint32_t body_get_collision_mask(RID p_body);
	static void body_set_collision_priority(RID p_body, real_t p_priority);
	static real_t body_get_collision_priority(RID p_body);
	static void body_set_user_flags(RID p_body, uint32_t p_flags);
	static uint32_t body_get_user_flags(RID p_body);
	static void body_reset_mass_properties(RID p_body);
	static void body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	static void body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position = Vector3());
	static void body_apply_torque_impulse(RID p_body, const Vector3& p_impulse);
	static void body_apply_central_force(RID p_body, const Vector3& p_force);
	static void body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	static void body_apply_torque(RID p_body, const Vector3& p_torque);
	static void body_add_constant_central_force(RID p_body, const Vector3& p_force);
	static void body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	static void body_add_constant_torque(RID p_body, const Vector3& p_torque);
	static void body_set_constant_force(RID p_body, const Vector3& p_force);
	static Vector3 body_get_constant_force(RID p_body);
	static void body_set_constant_torque(RID p_body, const Vector3& p_torque);
	static Vector3 body_get_constant_torque(RID p_body);
	static void body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity);
	static void body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock);
	static bool body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis);
	static void body_add_collision_exception(RID p_body, RID p_body_b);
	static void body_remove_collision_exception(RID p_body, RID p_body_b);
	static void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);
	static void body_set_max_contacts_reported(RID p_body, int p_contacts);
	static int body_get_max_contacts_reported(RID p_body);
	static void body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold);
	static real_t body_get_contacts_reported_depth_threshold(RID p_body);
	static void body_set_omit_force_integration(RID p_body, bool p_omit);
	static bool body_is_omitting_force_integration(RID p_body);
	static void body_set_ray_pickable(RID p_body, bool p_enable);
	static PhysicsDirectBodyState3D* body_get_direct_state(RID p_body);
	static bool body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result = nullptr);
	static RID soft_body_create();
	static void soft_body_set_space(RID p_body, RID p_space);
	static RID soft_body_get_space(RID p_body);
	static void soft_body_set_mesh(RID p_body, RID p_mesh);
	static AABB soft_body_get_bounds(RID p_body);
	static void soft_body_set_collision_layer(RID p_body, uint32_t p_layer);
	static uint32_t soft_body_get_collision_layer(RID p_body);
	static void soft_body_set_collision_mask(RID p_body, uint32_t p_mask);
	static uint32_t soft_body_get_collision_mask(RID p_body);
	static void soft_body_add_collision_exception(RID p_body, RID p_body_b);
	static void soft_body_remove_collision_exception(RID p_body, RID p_body_b);
	static void soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);
	static void soft_body_set_transform(RID p_body, const Transform3D& p_transform);
	static void soft_body_set_ray_pickable(RID p_body, bool p_enable);
	static void soft_body_set_simulation_precision(RID p_body, int p_simulation_precision);
	static int soft_body_get_simulation_precision(RID p_body);
	static void soft_body_set_total_mass(RID p_body, real_t p_total_mass);
	static real_t soft_body_get_total_mass(RID p_body);
	static void soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness);
	static real_t soft_body_get_linear_stiffness(RID p_body);
	static void soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor);
	static real_t soft_body_get_shrinking_factor(RID p_body);
	static void soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient);
	static real_t soft_body_get_pressure_coefficient(RID p_body);
	static void soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient);
	static real_t soft_body_get_damping_coefficient(RID p_body);
	static void soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient);
	static real_t soft_body_get_drag_coefficient(RID p_body);
	static void soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position);
	static Vector3 soft_body_get_point_global_position(RID p_body, int p_point_index);
	static void soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse);
	static void soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force);
	static void soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	static void soft_body_apply_central_force(RID p_body, const Vector3& p_force);
	static void soft_body_remove_all_pinned_points(RID p_body);
	static void soft_body_pin_point(RID p_body, int p_point_index, bool p_pin);
	static bool soft_body_is_point_pinned(RID p_body, int p_point_index);
	static RID joint_create();
	static void joint_clear(RID p_joint);
	static PS3DE::JointType joint_get_type(RID p_joint);
	static void joint_set_solver_priority(RID p_joint, int p_priority);
	static int joint_get_solver_priority(RID p_joint);
	static void joint_disable_collisions_between_bodies(RID p_joint, bool p_disable);
	static bool joint_is_disabled_collisions_between_bodies(RID p_joint);
	static void joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B);
	static void pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value);
	static real_t pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param);
	static void pin_joint_set_local_a(RID p_joint, const Vector3& p_A);
	static Vector3 pin_joint_get_local_a(RID p_joint);
	static void pin_joint_set_local_b(RID p_joint, const Vector3& p_B);
	static Vector3 pin_joint_get_local_b(RID p_joint);
	static void joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B);
	static void joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B);
	static void hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value);
	static real_t hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param);
	static void hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled);
	static bool hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag);
	static void joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value);
	static real_t slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param);
	static void joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value);
	static real_t cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param);
	static void joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void generic_6dof_joint_set_param(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisParam p_param, real_t p_value);
	static real_t generic_6dof_joint_get_param(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisParam p_param);
	static void generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable);
	static bool generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisFlag p_flag);
	static void generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation);
	static Quaternion generic_6dof_joint_get_angular_target_rotation(RID p_joint);
	static void free_rid(RID p_rid);
	static void set_active(bool p_active);
	static void init();
	static void step(real_t p_step);
	static void sync();
	static void flush_queries();
	static void end_sync();
	static void finish();
	static bool is_flushing_queries();
	static int get_process_info(PS3DE::ProcessInfo p_info);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] static void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	PhysicsServer3D() = delete;
	PhysicsServer3D(const PhysicsServer3D&) = delete;
	PhysicsServer3D& operator=(const PhysicsServer3D&) = delete;
	~PhysicsServer3D() = delete;
};

// --- Folded from servers/physics_3d/physics_server_3d_constants.h ---
namespace PhysicsServer3DConstants {

constexpr int MAX_CONTACTS_REPORTED_3D_MAX = 4096;

} // namespace PhysicsServer3DConstants

// Alias to make it easier to use.
#define PS3DC PhysicsServer3DConstants

// --- End servers/physics_3d/physics_server_3d_constants.h ---
