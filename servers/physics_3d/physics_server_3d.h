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
#include "core/math/plane.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"
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
#include "core/types.h"
#include "core/math/plane.h"
#include "core/templates/hashfuncs.h"
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

// --- Folded from modules/jolt_physics/jolt_physics_server_3d.h ---
#include "core/types.h"


class JoltArea3D;
class JoltBody3D;
class JoltJobSystem;
class JoltJoint3D;
class JoltShape3D;
class JoltSoftBody3D;
class JoltSpace3D;
class JoltTempAllocator;

class JoltPhysicsServer3D final
{
	mutable RID_PtrOwner<JoltSpace3D, true> space_owner;
	mutable RID_PtrOwner<JoltArea3D, true> area_owner;
	mutable RID_PtrOwner<JoltBody3D, true> body_owner{65536, 1048576};
	mutable RID_PtrOwner<JoltSoftBody3D, true> soft_body_owner;
	mutable RID_PtrOwner<JoltShape3D, true> shape_owner;
	mutable RID_PtrOwner<JoltJoint3D, true> joint_owner;

	HashSet<JoltSpace3D*> active_spaces;

	JoltJobSystem* job_system = nullptr;
	JoltTempAllocator* temp_allocator = nullptr;

	bool on_separate_thread = false;
	bool active = true;
	bool flushing_queries = false;
	bool doing_sync = false;

public:
	enum HingeJointParamJolt
	{
		HINGE_JOINT_LIMIT_SPRING_FREQUENCY = 100,
		HINGE_JOINT_LIMIT_SPRING_DAMPING,
		HINGE_JOINT_MOTOR_MAX_TORQUE,
	};

	enum HingeJointFlagJolt
	{
		HINGE_JOINT_FLAG_USE_LIMIT_SPRING = 100,
	};

	enum SliderJointParamJolt
	{
		SLIDER_JOINT_LIMIT_SPRING_FREQUENCY = 100,
		SLIDER_JOINT_LIMIT_SPRING_DAMPING,
		SLIDER_JOINT_MOTOR_TARGET_VELOCITY,
		SLIDER_JOINT_MOTOR_MAX_FORCE,
	};

	enum SliderJointFlagJolt
	{
		SLIDER_JOINT_FLAG_USE_LIMIT = 100,
		SLIDER_JOINT_FLAG_USE_LIMIT_SPRING,
		SLIDER_JOINT_FLAG_ENABLE_MOTOR,
	};

	enum ConeTwistJointParamJolt
	{
		CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Y = 100,
		CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Z,
		CONE_TWIST_JOINT_TWIST_MOTOR_TARGET_VELOCITY,
		CONE_TWIST_JOINT_SWING_MOTOR_MAX_TORQUE,
		CONE_TWIST_JOINT_TWIST_MOTOR_MAX_TORQUE,
	};

	enum ConeTwistJointFlagJolt
	{
		CONE_TWIST_JOINT_FLAG_USE_SWING_LIMIT = 100,
		CONE_TWIST_JOINT_FLAG_USE_TWIST_LIMIT,
		CONE_TWIST_JOINT_FLAG_ENABLE_SWING_MOTOR,
		CONE_TWIST_JOINT_FLAG_ENABLE_TWIST_MOTOR,
	};

	enum G6DOFJointAxisParamJolt
	{
		G6DOF_JOINT_LINEAR_SPRING_FREQUENCY = 100,
		G6DOF_JOINT_LINEAR_LIMIT_SPRING_FREQUENCY,
		G6DOF_JOINT_LINEAR_LIMIT_SPRING_DAMPING,
		G6DOF_JOINT_ANGULAR_SPRING_FREQUENCY,
	};

	enum G6DOFJointAxisFlagJolt
	{
		G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT_SPRING = 100,
		G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING_FREQUENCY,
		G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING_FREQUENCY,
	};

public:
	explicit JoltPhysicsServer3D(bool p_on_separate_thread);
	~JoltPhysicsServer3D();

	RID shape_create(PS3DE::ShapeType p_shape);

	RID world_boundary_shape_create();
	RID separation_ray_shape_create();
	RID sphere_shape_create();
	RID box_shape_create();
	RID capsule_shape_create();
	RID cylinder_shape_create();
	RID convex_polygon_shape_create();
	RID concave_polygon_shape_create();
	RID heightmap_shape_create();
	RID custom_shape_create();

	void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);

	void shape_set_margin(RID p_shape, real_t p_margin);
	real_t shape_get_margin(RID p_shape) const;

	PS3DE::ShapeType shape_get_type(RID p_shape) const;

	real_t shape_get_custom_solver_bias(RID p_shape) const;

	RID space_create();

	void space_set_active(RID p_space, bool p_active);
	bool space_is_active(RID p_space) const;

	void space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value);
	real_t space_get_param(RID p_space, PS3DE::SpaceParameter p_param) const;

	PhysicsDirectSpaceState3D* space_get_direct_state(RID p_space);

	void space_set_debug_contacts(RID p_space, int p_max_contacts);
	PackedVector3Array space_get_contacts(RID p_space) const;
	int space_get_contact_count(RID p_space) const;

	RID area_create();

	void area_set_space(RID p_area, RID p_space);
	RID area_get_space(RID p_area) const;

	void area_add_shape(RID p_area, RID p_shape,
		const Transform3D& p_transform = Transform3D(), bool p_disabled = false);

	void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	RID area_get_shape(RID p_area, int p_shape_idx) const;

	void area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform);
	Transform3D area_get_shape_transform(RID p_area, int p_shape_idx) const;

	void area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled);

	int area_get_shape_count(RID p_area) const;

	void area_remove_shape(RID p_area, int p_shape_idx);
	void area_clear_shapes(RID p_area);

	void area_set_transform(RID p_area, const Transform3D& p_transform);
	Transform3D area_get_transform(RID p_area) const;

	void area_set_collision_layer(RID p_area, uint32_t p_layer);
	uint32_t area_get_collision_layer(RID p_area) const;

	void area_set_collision_mask(RID p_area, uint32_t p_mask);
	uint32_t area_get_collision_mask(RID p_area) const;

	void area_set_monitorable(RID p_area, bool p_monitorable);

	void area_set_ray_pickable(RID p_area, bool p_enable);

	RID body_create();

	void body_set_space(RID p_body, RID p_space);
	RID body_get_space(RID p_body) const;

	void body_set_mode(RID p_body, PS3DE::BodyMode p_mode);
	PS3DE::BodyMode body_get_mode(RID p_body) const;

	void body_add_shape(RID p_body, RID p_shape,
		const Transform3D& p_transform = Transform3D(), bool p_disabled = false);

	void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	RID body_get_shape(RID p_body, int p_shape_idx) const;

	void body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform);
	Transform3D body_get_shape_transform(RID p_body, int p_shape_idx) const;

	void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);

	int body_get_shape_count(RID p_body) const;

	void body_remove_shape(RID p_body, int p_shape_idx);
	void body_clear_shapes(RID p_body);

	void body_set_enable_continuous_collision_detection(RID p_body, bool p_enable);
	bool body_is_continuous_collision_detection_enabled(RID p_body) const;

	void body_set_collision_layer(RID p_body, uint32_t p_layer);
	uint32_t body_get_collision_layer(RID p_body) const;

	void body_set_collision_mask(RID p_body, uint32_t p_mask);
	uint32_t body_get_collision_mask(RID p_body) const;

	void body_set_collision_priority(RID p_body, real_t p_priority);
	real_t body_get_collision_priority(RID p_body) const;

	void body_set_user_flags(RID p_body, uint32_t p_flags);
	uint32_t body_get_user_flags(RID p_body) const;

	void body_reset_mass_properties(RID p_body);

	void body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	void body_apply_impulse(
		RID p_body, const Vector3& p_impulse, const Vector3& p_position = Vector3());
	void body_apply_torque_impulse(RID p_body, const Vector3& p_impulse);

	void body_apply_central_force(RID p_body, const Vector3& p_force);
	void body_apply_force(
		RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	void body_apply_torque(RID p_body, const Vector3& p_torque);

	void body_add_constant_central_force(RID p_body, const Vector3& p_force);
	void body_add_constant_force(
		RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	void body_add_constant_torque(RID p_body, const Vector3& p_torque);

	void body_set_constant_force(RID p_body, const Vector3& p_force);
	Vector3 body_get_constant_force(RID p_body) const;

	void body_set_constant_torque(RID p_body, const Vector3& p_torque);
	Vector3 body_get_constant_torque(RID p_body) const;

	void body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity);

	void body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock);
	bool body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis) const;

	void body_add_collision_exception(RID p_body, RID p_excepted_body);
	void body_remove_collision_exception(RID p_body, RID p_excepted_body);
	void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);

	void body_set_max_contacts_reported(RID p_body, int p_amount);
	int body_get_max_contacts_reported(RID p_body) const;

	void body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold);
	real_t body_get_contacts_reported_depth_threshold(RID p_body) const;

	void body_set_omit_force_integration(RID p_body, bool p_enable);
	bool body_is_omitting_force_integration(RID p_body) const;

	void body_set_ray_pickable(RID p_body, bool p_enable);

	bool _body_test_motion(RID p_body, PhysicsTestMotionParameters3D* rp_parameters,
		const Ref<PhysicsTestMotionResult3D>& p_result = Ref<PhysicsTestMotionResult3D>());
	bool body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters,
		PS3DT::MotionResult* r_result = nullptr);

	PhysicsDirectBodyState3D* body_get_direct_state(RID p_body);

	RID soft_body_create();

	void soft_body_update_rendering_server(
		RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler);

	void soft_body_set_space(RID p_body, RID p_space);
	RID soft_body_get_space(RID p_body) const;

	void soft_body_set_ray_pickable(RID p_body, bool p_enable);

	void soft_body_set_collision_layer(RID p_body, uint32_t p_layer);
	uint32_t soft_body_get_collision_layer(RID p_body) const;

	void soft_body_set_collision_mask(RID p_body, uint32_t p_mask);
	uint32_t soft_body_get_collision_mask(RID p_body) const;

	void soft_body_add_collision_exception(RID p_body, RID p_excepted_body);
	void soft_body_remove_collision_exception(RID p_body, RID p_excepted_body);
	void soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);

	void soft_body_set_transform(RID p_body, const Transform3D& p_transform);

	void soft_body_apply_point_impulse(
		RID p_body, int p_point_index, const Vector3& p_impulse);
	void soft_body_apply_point_force(
		RID p_body, int p_point_index, const Vector3& p_force);
	void soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	void soft_body_apply_central_force(RID p_body, const Vector3& p_force);

	void soft_body_set_simulation_precision(RID p_body, int p_precision);
	int soft_body_get_simulation_precision(RID p_body) const;

	void soft_body_set_total_mass(RID p_body, real_t p_total_mass);
	real_t soft_body_get_total_mass(RID p_body) const;

	void soft_body_set_linear_stiffness(RID p_body, real_t p_coefficient);
	real_t soft_body_get_linear_stiffness(RID p_body) const;

	void soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor);
	real_t soft_body_get_shrinking_factor(RID p_body) const;

	void soft_body_set_pressure_coefficient(RID p_body, real_t p_coefficient);
	real_t soft_body_get_pressure_coefficient(RID p_body) const;

	void soft_body_set_damping_coefficient(RID p_body, real_t p_coefficient);
	real_t soft_body_get_damping_coefficient(RID p_body) const;

	void soft_body_set_drag_coefficient(RID p_body, real_t p_coefficient);
	real_t soft_body_get_drag_coefficient(RID p_body) const;

	void soft_body_set_mesh(RID p_body, RID p_mesh);

	AABB soft_body_get_bounds(RID p_body) const;

	void soft_body_move_point(
		RID p_body, int p_point_index, const Vector3& p_global_position);

	Vector3 soft_body_get_point_global_position(
		RID p_body, int p_point_index) const;

	void soft_body_remove_all_pinned_points(RID p_body);

	void soft_body_pin_point(RID p_body, int p_point_index, bool p_pin);
	bool soft_body_is_point_pinned(RID p_body, int p_point_index) const;

	RID joint_create();
	void joint_clear(RID p_joint);

	void joint_make_pin(RID p_joint, RID p_body_a, const Vector3& p_local_a, RID p_body_b,
		const Vector3& p_local_b);

	void pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value);
	real_t pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param) const;

	void pin_joint_set_local_a(RID p_joint, const Vector3& p_local_a);
	Vector3 pin_joint_get_local_a(RID p_joint) const;

	void pin_joint_set_local_b(RID p_joint, const Vector3& p_local_b);
	Vector3 pin_joint_get_local_b(RID p_joint) const;

	void joint_make_hinge(RID p_joint, RID p_body_a, const Transform3D& p_hinge_a,
		RID p_body_b, const Transform3D& p_hinge_b);

	void joint_make_hinge_simple(RID p_joint, RID p_body_a, const Vector3& p_pivot_a,
		const Vector3& p_axis_a, RID p_body_b, const Vector3& p_pivot_b,
		const Vector3& p_axis_b);

	void hinge_joint_set_param(
		RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value);
	real_t hinge_joint_get_param(
		RID p_joint, PS3DE::HingeJointParam p_param) const;

	void hinge_joint_set_flag(
		RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled);
	bool hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag) const;

	void joint_make_slider(RID p_joint, RID p_body_a, const Transform3D& p_local_ref_a,
		RID p_body_b, const Transform3D& p_local_ref_b);

	void slider_joint_set_param(
		RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value);
	real_t slider_joint_get_param(
		RID p_joint, PS3DE::SliderJointParam p_param) const;

	void joint_make_cone_twist(RID p_joint, RID p_body_a, const Transform3D& p_local_ref_a,
		RID p_body_b, const Transform3D& p_local_ref_b);

	void cone_twist_joint_set_param(
		RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value);
	real_t cone_twist_joint_get_param(
		RID p_joint, PS3DE::ConeTwistJointParam p_param) const;

	void joint_make_generic_6dof(RID p_joint, RID p_body_a,
		const Transform3D& p_local_ref_a, RID p_body_b, const Transform3D& p_local_ref_b);

	void generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_axis,
		PS3DE::G6DOFJointAxisParam p_param, real_t p_value);
	real_t generic_6dof_joint_get_param(
		RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param) const;

	void generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_axis,
		PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable);
	bool generic_6dof_joint_get_flag(
		RID p_joint, Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag) const;

	void generic_6dof_joint_set_angular_target_rotation(
		RID p_joint, const Quaternion& p_target_rotation);
	Quaternion generic_6dof_joint_get_angular_target_rotation(RID p_joint) const;

	PS3DE::JointType joint_get_type(RID p_joint) const;

	void joint_set_solver_priority(RID p_joint, int p_priority);
	int joint_get_solver_priority(RID p_joint) const;

	void joint_disable_collisions_between_bodies(RID p_joint, bool p_disable);
	bool joint_is_disabled_collisions_between_bodies(RID p_joint) const;

	void free_rid(RID p_rid);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	void set_active(bool p_active);

	void init();
	void finish();

	void step(real_t p_step);

	void sync();
	void end_sync();

	void flush_queries();
	bool is_flushing_queries() const;

	int get_process_info(PS3DE::ProcessInfo p_process_info);

	bool is_on_separate_thread() const { return on_separate_thread; }

	bool is_active() const { return active; }

	void free_space(JoltSpace3D* p_space);
	void free_area(JoltArea3D* p_area);
	void free_body(JoltBody3D* p_body);
	void free_soft_body(JoltSoftBody3D* p_body);
	void free_shape(JoltShape3D* p_shape);
	void free_joint(JoltJoint3D* p_joint);

	JoltSpace3D* get_space(RID p_rid) const { return space_owner.get_or_null(p_rid); }

	JoltArea3D* get_area(RID p_rid) const { return area_owner.get_or_null(p_rid); }

	JoltBody3D* get_body(RID p_rid) const { return body_owner.get_or_null(p_rid); }

	JoltShape3D* get_shape(RID p_rid) const { return shape_owner.get_or_null(p_rid); }

	JoltJoint3D* get_joint(RID p_rid) const { return joint_owner.get_or_null(p_rid); }

#ifdef DEBUG_ENABLED
	void dump_debug_snapshots(const String& p_dir);

	void space_dump_debug_snapshot(RID p_space, const String& p_dir);
#endif

	bool joint_get_enabled(RID p_joint) const;
	void joint_set_enabled(RID p_joint, bool p_enabled);

	int joint_get_solver_velocity_iterations(RID p_joint);
	void joint_set_solver_velocity_iterations(RID p_joint, int p_value);

	int joint_get_solver_position_iterations(RID p_joint);
	void joint_set_solver_position_iterations(RID p_joint, int p_value);

	float pin_joint_get_applied_force(RID p_joint);

	double hinge_joint_get_jolt_param(RID p_joint, HingeJointParamJolt p_param) const;
	void hinge_joint_set_jolt_param(RID p_joint, HingeJointParamJolt p_param, double p_value);

	bool hinge_joint_get_jolt_flag(RID p_joint, HingeJointFlagJolt p_flag) const;
	void hinge_joint_set_jolt_flag(RID p_joint, HingeJointFlagJolt p_flag, bool p_enabled);

	float hinge_joint_get_applied_force(RID p_joint);
	float hinge_joint_get_applied_torque(RID p_joint);

	double slider_joint_get_jolt_param(RID p_joint, SliderJointParamJolt p_param) const;
	void slider_joint_set_jolt_param(RID p_joint, SliderJointParamJolt p_param, double p_value);

	bool slider_joint_get_jolt_flag(RID p_joint, SliderJointFlagJolt p_flag) const;
	void slider_joint_set_jolt_flag(RID p_joint, SliderJointFlagJolt p_flag, bool p_enabled);

	float slider_joint_get_applied_force(RID p_joint);
	float slider_joint_get_applied_torque(RID p_joint);

	double cone_twist_joint_get_jolt_param(RID p_joint, ConeTwistJointParamJolt p_param) const;
	void cone_twist_joint_set_jolt_param(
		RID p_joint, ConeTwistJointParamJolt p_param, double p_value);

	bool cone_twist_joint_get_jolt_flag(RID p_joint, ConeTwistJointFlagJolt p_flag) const;
	void cone_twist_joint_set_jolt_flag(RID p_joint, ConeTwistJointFlagJolt p_flag, bool p_enabled);

	float cone_twist_joint_get_applied_force(RID p_joint);
	float cone_twist_joint_get_applied_torque(RID p_joint);

	double generic_6dof_joint_get_jolt_param(
		RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisParamJolt p_param) const;
	void generic_6dof_joint_set_jolt_param(
		RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisParamJolt p_param, double p_value);

	bool generic_6dof_joint_get_jolt_flag(
		RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisFlagJolt p_flag) const;
	void generic_6dof_joint_set_jolt_flag(
		RID p_joint, Vector3::Axis p_axis, G6DOFJointAxisFlagJolt p_flag, bool p_enabled);

	float generic_6dof_joint_get_applied_force(RID p_joint);
	float generic_6dof_joint_get_applied_torque(RID p_joint);
};

// --- Folded from modules/jolt_physics/jolt_globals.h ---
void jolt_initialize();
void jolt_deinitialize();
bool jolt_is_initialized();

#ifdef GODOT_MODULE

#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/math/plane.h"
#include "core/types.h"
#include "core/string/ustring.h"

#ifdef DEBUG_ENABLED
#include "core/io/file_access.h"
#endif

#include <Jolt/Jolt.h>

#include <Jolt/Core/Color.h>
#ifdef DEBUG_ENABLED
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#endif
#include <Jolt/Geometry/AABox.h>
#include <Jolt/Geometry/Plane.h>
#include <Jolt/Math/Mat44.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

class JoltMath {
public:
	// Note that `p_basis` is mutated to be right-handed orthonormal.
	static void decompose(Basis &p_basis, Vector3 &r_scale);

	// Note that `p_transform` is mutated to be right-handed orthonormal.
	static _FORCE_INLINE_ void decompose(Transform3D &p_transform, Vector3 &r_scale) {
		decompose(p_transform.basis, r_scale);
	}

	static _FORCE_INLINE_ void decomposed(const Basis &p_basis, Basis &r_new_basis, Vector3 &r_scale) {
		Basis new_basis = p_basis;
		decompose(new_basis, r_scale);
		r_new_basis = new_basis;
	}

	static _FORCE_INLINE_ void decomposed(const Transform3D &p_transform, Transform3D &r_new_transform, Vector3 &r_scale) {
		Transform3D new_transform;
		decompose(new_transform, r_scale);
		r_new_transform = new_transform;
	}
};

_FORCE_INLINE_ Vector3 to_godot(const JPH::Vec3 &p_vec) {
	return Vector3((real_t)p_vec.GetX(), (real_t)p_vec.GetY(), (real_t)p_vec.GetZ());
}

_FORCE_INLINE_ Vector3 to_godot(const JPH::DVec3 &p_vec) {
	return Vector3((real_t)p_vec.GetX(), (real_t)p_vec.GetY(), (real_t)p_vec.GetZ());
}

_FORCE_INLINE_ Basis to_godot(const JPH::Quat &p_quat) {
	return Basis(Quaternion(p_quat.GetX(), p_quat.GetY(), p_quat.GetZ(), p_quat.GetW()));
}

_FORCE_INLINE_ Transform3D to_godot(const JPH::Mat44 &p_mat) {
	return Transform3D(Vector3(p_mat(0, 0), p_mat(1, 0), p_mat(2, 0)),
			Vector3(p_mat(0, 1), p_mat(1, 1), p_mat(2, 1)),
			Vector3(p_mat(0, 2), p_mat(1, 2), p_mat(2, 2)),
			Vector3(p_mat(0, 3), p_mat(1, 3), p_mat(2, 3)));
}

_FORCE_INLINE_ Color to_godot(const JPH::Color &p_color) {
	const float r = (float)p_color.r;
	const float g = (float)p_color.g;
	const float b = (float)p_color.b;
	const float a = (float)p_color.a;

	return Color(r == 0.0f ? 0.0f : 255.0f / r, g == 0.0f ? 0.0f : 255.0f / g,
			b == 0.0f ? 0.0f : 255.0f / b, a == 0.0f ? 0.0f : 255.0f / a);
}

_FORCE_INLINE_ String to_godot(const JPH::String &p_str) {
	return String::utf8(p_str.c_str(), (int)p_str.length());
}

_FORCE_INLINE_ AABB to_godot(const JPH::AABox &p_aabb) {
	return AABB(to_godot(p_aabb.mMin), to_godot(p_aabb.mMax - p_aabb.mMin));
}

_FORCE_INLINE_ Plane to_godot(const JPH::Plane &p_plane) {
	return Plane(to_godot(p_plane.GetNormal()), (real_t)p_plane.GetConstant());
}

_FORCE_INLINE_ JPH::Vec3 to_jolt(const Vector3 &p_vec) {
	return JPH::Vec3((float)p_vec.x, (float)p_vec.y, (float)p_vec.z);
}

_FORCE_INLINE_ JPH::Quat to_jolt(const Quaternion &p_quat) {
	return JPH::Quat((float)p_quat.x, (float)p_quat.y, (float)p_quat.z, (float)p_quat.w);
}

_FORCE_INLINE_ JPH::Quat to_jolt(const Basis &p_basis) {
	return to_jolt(p_basis.get_quaternion().normalized());
}

_FORCE_INLINE_ JPH::Mat44 to_jolt(const Transform3D &p_transform) {
	const Basis &b = p_transform.basis;
	const Vector3 &o = p_transform.origin;

	return JPH::Mat44(JPH::Vec4(b[0][0], b[1][0], b[2][0], 0.0f),
			JPH::Vec4(b[0][1], b[1][1], b[2][1], 0.0f), JPH::Vec4(b[0][2], b[1][2], b[2][2], 0.0f),
			JPH::Vec3(o.x, o.y, o.z));
}

_FORCE_INLINE_ JPH::Color to_jolt(const Color &p_color) {
	return JPH::Color((JPH::uint32)p_color.to_abgr32());
}

_FORCE_INLINE_ JPH::String to_jolt(const String &p_str) {
	const CharString str_utf8 = p_str.utf8();
	return JPH::String(str_utf8.get_data(), (size_t)str_utf8.length());
}

_FORCE_INLINE_ JPH::AABox to_jolt(const AABB &p_aabb) {
	return JPH::AABox(to_jolt(p_aabb.position), to_jolt(p_aabb.position + p_aabb.size));
}

_FORCE_INLINE_ JPH::Plane to_jolt(const Plane &p_plane) {
	return JPH::Plane(to_jolt(p_plane.normal), (float)-p_plane.d);
}

_FORCE_INLINE_ JPH::RVec3 to_jolt_r(const Vector3 &p_vec) {
	return JPH::RVec3(p_vec.x, p_vec.y, p_vec.z);
}

_FORCE_INLINE_ JPH::RMat44 to_jolt_r(const Transform3D &p_transform) {
	const Basis &b = p_transform.basis;
	const Vector3 &o = p_transform.origin;

	return JPH::RMat44(JPH::Vec4(b[0][0], b[1][0], b[2][0], 0.0f),
			JPH::Vec4(b[0][1], b[1][1], b[2][1], 0.0f), JPH::Vec4(b[0][2], b[1][2], b[2][2], 0.0f),
			JPH::RVec3(o.x, o.y, o.z));
}

#ifdef DEBUG_ENABLED

class JoltStreamOutputWrapper final : public JPH::StreamOut {
	Ref<FileAccess> file_access;

public:
	explicit JoltStreamOutputWrapper(const Ref<FileAccess> &p_file_access) :
			file_access(p_file_access) {}

	virtual void WriteBytes(const void *p_data, size_t p_bytes) override {
		file_access->store_buffer(static_cast<const uint8_t *>(p_data), static_cast<uint64_t>(p_bytes));
	}

	virtual bool IsFailed() const override {
		return file_access->get_error() != OK;
	}
};

class JoltStreamInputWrapper final : public JPH::StreamIn {
	Ref<FileAccess> file_access;

public:
	explicit JoltStreamInputWrapper(const Ref<FileAccess> &p_file_access) :
			file_access(p_file_access) {}

	virtual void ReadBytes(void *p_data, size_t p_bytes) override {
		file_access->get_buffer(static_cast<uint8_t *>(p_data), static_cast<uint64_t>(p_bytes));
	}

	virtual bool IsEOF() const override {
		return file_access->eof_reached();
	}

	virtual bool IsFailed() const override {
		return file_access->get_error() != OK;
	}
};

#endif // DEBUG_ENABLED

#endif // GODOT_MODULE
// --- End modules/jolt_physics/jolt_physics_server_3d.h ---

// --- Folded from servers/navigation_3d/navigation_server_3d.h ---
#include "core/types.h"
#include "core/math/plane.h"
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/vector.h"
#include "core/types.h"
// --- Inlined/Folded from scene/resources/3d/navigation_mesh_source_geometry_data_3d.h ---
#include "core/os/mutex.h"
#include "scene/resources/mesh.h"

class NavigationMeshSourceGeometryData3D : public Resource
{
	RWLock geometry_rwlock;

	Vector<float> vertices;
	Vector<int> indices;

	AABB bounds;
	bool bounds_dirty = true;

public:
	struct ProjectedObstruction;

private:
	Vector<ProjectedObstruction> _projected_obstructions;

protected:

private:
	void _add_vertex(const Vector3& p_vec3);
	void _add_mesh(const Ref<Mesh>& p_mesh, const Transform3D& p_xform);
	void _add_faces(const PackedVector3Array& p_faces, const Transform3D& p_xform);

public:
	struct ProjectedObstruction
	{
		static inline uint32_t VERSION = 1; // Increase when format changes so we can detect
											// outdated formats and provide compatibility.

		Vector<float> vertices;
		float elevation = 0.0;
		float height = 0.0;
		bool carve = false;
	};

	// kept root node transform here on the geometry data
	// if we add this transform to all exposed functions we need to break comp on all functions
	// later when navmesh changes from global transform to relative to navregion but if it stays
	// here we can just remove it and change the internal functions only
	Transform3D root_node_transform;

	void set_vertices(const Vector<float>& p_vertices);
	const Vector<float>& get_vertices() const;

	void set_indices(const Vector<int>& p_indices);
	const Vector<int>& get_indices() const;

	void append_arrays(const Vector<float>& p_vertices, const Vector<int>& p_indices);

	bool has_data();
	void clear();
	void clear_projected_obstructions();

	void add_mesh(const Ref<Mesh>& p_mesh, const Transform3D& p_xform);
	void add_faces(const PackedVector3Array& p_faces, const Transform3D& p_xform);

	void merge(const Ref<NavigationMeshSourceGeometryData3D>& p_other_geometry);

	void add_projected_obstruction(
		const Vector<Vector3>& p_vertices, float p_elevation, float p_height, bool p_carve);
	Vector<ProjectedObstruction> _get_projected_obstructions() const;

	void set_data(const Vector<float>& p_vertices, const Vector<int>& p_indices,
		Vector<ProjectedObstruction>& p_projected_obstructions);
	void get_data(Vector<float>& r_vertices, Vector<int>& r_indices,
		Vector<ProjectedObstruction>& r_projected_obstructions);

	AABB get_bounds();

	~NavigationMeshSourceGeometryData3D() { clear(); }
};
#include "scene/resources/mesh.h"

namespace NavigationEnums3D {

enum PathfindingAlgorithm {
	PATHFINDING_ALGORITHM_ASTAR = 0,
};

enum PathPostProcessing {
	PATH_POSTPROCESSING_CORRIDORFUNNEL = 0,
	PATH_POSTPROCESSING_EDGECENTERED,
	PATH_POSTPROCESSING_NONE,
};

enum PathSegmentType {
	PATH_SEGMENT_TYPE_REGION = 0,
	PATH_SEGMENT_TYPE_LINK
};

enum PathMetadataFlags {
	PATH_INCLUDE_NONE = 0,
	PATH_INCLUDE_TYPES = 1,
	PATH_INCLUDE_RIDS = 2,
	PATH_INCLUDE_OWNERS = 4,
	PATH_INCLUDE_ALL = PATH_INCLUDE_TYPES | PATH_INCLUDE_RIDS | PATH_INCLUDE_OWNERS
};

} // namespace NavigationEnums3D

namespace NavigationDefaults3D {

// Rasterization.

// To find the polygons edges the vertices are displaced in a grid where
// each cell has the following cell_size and cell_height.
constexpr float NAV_MESH_CELL_HEIGHT = 0.25f; // Must match ProjectSettings default 3D cell_height and NavigationMesh cell_height.
constexpr float NAV_MESH_CELL_SIZE = 0.25f; // Must match ProjectSettings default 3D cell_size and NavigationMesh cell_size.
constexpr float NAV_MESH_CELL_SIZE_MIN = 0.01f;
constexpr const char *const NAV_MESH_CELL_SIZE_HINT = "0.001,100,0.001,or_greater";

// Map.

constexpr float EDGE_CONNECTION_MARGIN = 0.25f;
constexpr float LINK_CONNECTION_RADIUS = 1.0f;
constexpr int path_search_max_polygons = 4096;

// Agent.

constexpr float AVOIDANCE_AGENT_HEIGHT = 1.0;
constexpr float AVOIDANCE_AGENT_RADIUS = 0.5;
constexpr float AVOIDANCE_AGENT_MAX_SPEED = 10.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_AGENTS = 1.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES = 0.0;
constexpr int AVOIDANCE_AGENT_MAX_NEIGHBORS = 10;
constexpr float AVOIDANCE_AGENT_NEIGHBOR_DISTANCE = 50.0;

} // namespace NavigationDefaults3D

class NavigationPathQueryParameters3D final : public RefCounted
{
public:
	enum PathfindingAlgorithm
	{
		PATHFINDING_ALGORITHM_ASTAR = NavigationEnums3D::PATHFINDING_ALGORITHM_ASTAR,
	};

	enum PathPostProcessing
	{
		PATH_POSTPROCESSING_CORRIDORFUNNEL = NavigationEnums3D::PATH_POSTPROCESSING_CORRIDORFUNNEL,
		PATH_POSTPROCESSING_EDGECENTERED = NavigationEnums3D::PATH_POSTPROCESSING_EDGECENTERED,
		PATH_POSTPROCESSING_NONE = NavigationEnums3D::PATH_POSTPROCESSING_NONE,
	};

	enum PathMetadataFlags
	{
		PATH_METADATA_INCLUDE_NONE = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_NONE,
		PATH_METADATA_INCLUDE_TYPES = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_TYPES,
		PATH_METADATA_INCLUDE_RIDS = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_RIDS,
		PATH_METADATA_INCLUDE_OWNERS = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_OWNERS,
		PATH_METADATA_INCLUDE_ALL = NavigationEnums3D::PathMetadataFlags::PATH_INCLUDE_ALL
	};

private:
	PathfindingAlgorithm pathfinding_algorithm = PATHFINDING_ALGORITHM_ASTAR;
	PathPostProcessing path_postprocessing = PATH_POSTPROCESSING_CORRIDORFUNNEL;
	RID map;
	Vector3 start_position;
	Vector3 target_position;
	uint32_t navigation_layers = 1;
	uint32_t metadata_flags = PATH_METADATA_INCLUDE_ALL;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;

	LocalVector<RID> _excluded_regions;
	LocalVector<RID> _included_regions;

	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults3D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

public:
	void set_pathfinding_algorithm(const PathfindingAlgorithm p_pathfinding_algorithm);
	PathfindingAlgorithm get_pathfinding_algorithm() const;

	void set_path_postprocessing(const PathPostProcessing p_path_postprocessing);
	PathPostProcessing get_path_postprocessing() const;

	void set_map(RID p_map);
	RID get_map() const;

	void set_start_position(Vector3 p_start_position);
	Vector3 get_start_position() const;

	void set_target_position(Vector3 p_target_position);
	Vector3 get_target_position() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_metadata_flags(uint32_t p_flags);
	uint32_t get_metadata_flags() const;

	void set_simplify_path(bool p_enabled);
	bool get_simplify_path() const;

	void set_simplify_epsilon(real_t p_epsilon);
	real_t get_simplify_epsilon() const;

	void set_path_return_max_length(float p_length);
	float get_path_return_max_length() const;

	void set_path_return_max_radius(float p_radius);
	float get_path_return_max_radius() const;

	void set_path_search_max_polygons(int p_max_polygons);
	int get_path_search_max_polygons() const;

	void set_path_search_max_distance(float p_distance);
	float get_path_search_max_distance() const;
};

class NavigationPathQueryResult3D final : public RefCounted
{
	Vector<Vector3> path;
	Vector<int32_t> path_types;
	Vector<int64_t> path_owner_ids;
	float path_length = 0.0;

public:
	enum PathSegmentType
	{
		PATH_SEGMENT_TYPE_REGION = NavigationEnums3D::PathSegmentType::PATH_SEGMENT_TYPE_REGION,
		PATH_SEGMENT_TYPE_LINK = NavigationEnums3D::PathSegmentType::PATH_SEGMENT_TYPE_LINK,
	};

	void set_path(const Vector<Vector3>& p_path);
	const Vector<Vector3>& get_path() const;

	void set_path_types(const Vector<int32_t>& p_path_types);
	const Vector<int32_t>& get_path_types() const;

	void set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids);
	const Vector<int64_t>& get_path_owner_ids() const;

	void set_path_length(float p_length);
	float get_path_length() const;

	void reset();

	void set_data(const LocalVector<Vector3>& p_path, const LocalVector<int32_t>& p_path_types,
		const LocalVector<RID>& p_path_rids, const LocalVector<int64_t>& p_path_owner_ids);
};

class NavigationServer3D final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	enum ProcessInfo
	{
		INFO_ACTIVE_MAPS,
		INFO_REGION_COUNT,
		INFO_AGENT_COUNT,
		INFO_LINK_COUNT,
		INFO_POLYGON_COUNT,
		INFO_EDGE_COUNT,
		INFO_EDGE_MERGE_COUNT,
		INFO_EDGE_CONNECTION_COUNT,
		INFO_EDGE_FREE_COUNT,
		INFO_OBSTACLE_COUNT,
	};

private:
	struct NavMap;
	struct NavRegion;
	struct NavLink;
	struct NavAgent;
	struct NavObstacle;

	struct Data
	{
		bool active = true;
		bool debug_enabled = false;

		LocalVector<NavMap*> active_maps;
		LocalVector<NavMap*> iteration_maps;

		RID_Owner<NavMap> map_owner;
		RID_Owner<NavRegion> region_owner;
		RID_Owner<NavLink> link_owner;
		RID_Owner<NavAgent> agent_owner;
		RID_Owner<NavObstacle> obstacle_owner;

#ifdef DEBUG_ENABLED
		bool debug_dirty = true;
		bool debug_navigation_enabled = false;
		bool navigation_debug_dirty = true;
		bool debug_avoidance_enabled = false;
		bool avoidance_debug_dirty = true;

		Color debug_navigation_edge_connection_color = Color(1.0, 0.0, 1.0, 1.0);
		Color debug_navigation_geometry_edge_color = Color(0.5, 1.0, 1.0, 1.0);
		Color debug_navigation_geometry_face_color = Color(0.5, 1.0, 1.0, 0.4);
		Color debug_navigation_geometry_edge_disabled_color = Color(0.5, 0.5, 0.5, 1.0);
		Color debug_navigation_geometry_face_disabled_color = Color(0.5, 0.5, 0.5, 0.4);
		Color debug_navigation_link_connection_color = Color(1.0, 0.5, 1.0, 1.0);
		Color debug_navigation_link_connection_disabled_color = Color(0.5, 0.5, 0.5, 1.0);
		Color debug_navigation_agent_path_color = Color(1.0, 0.0, 0.0, 1.0);

		real_t debug_navigation_agent_path_point_size = 4.0;

		Color debug_navigation_avoidance_agents_radius_color = Color(1.0, 1.0, 0.0, 0.25);
		Color debug_navigation_avoidance_obstacles_radius_color = Color(1.0, 0.5, 0.0, 0.25);

		Color debug_navigation_avoidance_static_obstacle_pushin_face_color =
			Color(1.0, 0.0, 0.0, 0.0);
		Color debug_navigation_avoidance_static_obstacle_pushout_face_color =
			Color(1.0, 1.0, 0.0, 0.5);
		Color debug_navigation_avoidance_static_obstacle_pushin_edge_color =
			Color(1.0, 0.0, 0.0, 1.0);
		Color debug_navigation_avoidance_static_obstacle_pushout_edge_color =
			Color(1.0, 1.0, 0.0, 1.0);

		bool debug_navigation_enable_edge_connections = true;
		bool debug_navigation_enable_edge_connections_xray = true;
		bool debug_navigation_enable_edge_lines = true;
		bool debug_navigation_enable_edge_lines_xray = true;
		bool debug_navigation_enable_geometry_face_random_color = true;
		bool debug_navigation_enable_link_connections = true;
		bool debug_navigation_enable_link_connections_xray = true;
		bool debug_navigation_enable_agent_paths = true;
		bool debug_navigation_enable_agent_paths_xray = true;

		bool debug_navigation_avoidance_enable_agents_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_static = true;

		Ref<StandardMaterial3D> debug_navigation_geometry_edge_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_face_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_edge_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_geometry_face_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_edge_connections_material;
		Ref<StandardMaterial3D> debug_navigation_link_connections_material;
		Ref<StandardMaterial3D> debug_navigation_link_connections_disabled_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_agents_radius_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_obstacles_radius_material;

		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushin_face_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushout_face_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushin_edge_material;
		Ref<StandardMaterial3D> debug_navigation_avoidance_static_obstacle_pushout_edge_material;

		Ref<StandardMaterial3D> debug_navigation_agent_path_line_material;
		Ref<StandardMaterial3D> debug_navigation_agent_path_point_material;
#endif // DEBUG_ENABLED
	};

public:
	static inline Data* data = nullptr;

	/* SERVER LIFECYCLE */

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	static void set_active(bool p_active);
	static void sync();
	static void process(double p_delta_time);
	static void physics_process(double p_delta_time);
	static void free_rid(RID p_rid);

	static int get_process_info(ProcessInfo p_info);

	/* MAP API */

	static RID map_create();
	static void map_set_active(RID p_map, bool p_active);
	static bool map_is_active(RID p_map);

	static void map_set_up(RID p_map, Vector3 p_up);
	static Vector3 map_get_up(RID p_map);

	static void map_set_cell_size(RID p_map, real_t p_cell_size);
	static real_t map_get_cell_size(RID p_map);

	static void map_set_cell_height(RID p_map, real_t p_height);
	static real_t map_get_cell_height(RID p_map);

	static void map_set_merge_rasterizer_cell_scale(RID p_map, float p_value);
	static float map_get_merge_rasterizer_cell_scale(RID p_map);

	static void map_set_use_edge_connections(RID p_map, bool p_enabled);
	static bool map_get_use_edge_connections(RID p_map);

	static void map_set_edge_connection_margin(RID p_map, real_t p_connection_margin);
	static real_t map_get_edge_connection_margin(RID p_map);

	static void map_set_link_connection_radius(RID p_map, real_t p_connection_radius);
	static real_t map_get_link_connection_radius(RID p_map);

	static Vector<Vector3> map_get_path(RID p_map, Vector3 p_origin, Vector3 p_destination,
		bool p_optimize, uint32_t p_navigation_layers = 1);

	static Vector3 map_get_closest_point_to_segment(
		RID p_map, const Vector3& p_from, const Vector3& p_to, const bool p_use_collision = false);
	static Vector3 map_get_closest_point(RID p_map, const Vector3& p_point);
	static Vector3 map_get_closest_point_normal(RID p_map, const Vector3& p_point);
	static RID map_get_closest_point_owner(RID p_map, const Vector3& p_point);

	static void map_force_update(RID p_map);
	static uint32_t map_get_iteration_id(RID p_map);

	static void map_set_use_async_iterations(RID p_map, bool p_enabled);
	static bool map_get_use_async_iterations(RID p_map);

	static Vector3 map_get_random_point(RID p_map, uint32_t p_navigation_layers, bool p_uniformly);

	/* REGION API */

	static RID region_create();
	static uint32_t region_get_iteration_id(RID p_region);

	static void region_set_use_async_iterations(RID p_region, bool p_enabled);
	static bool region_get_use_async_iterations(RID p_region);

	static void region_set_enabled(RID p_region, bool p_enabled);
	static bool region_get_enabled(RID p_region);

	static void region_set_use_edge_connections(RID p_region, bool p_enabled);
	static bool region_get_use_edge_connections(RID p_region);

	static void region_set_enter_cost(RID p_region, real_t p_enter_cost);
	static real_t region_get_enter_cost(RID p_region);

	static void region_set_travel_cost(RID p_region, real_t p_travel_cost);
	static real_t region_get_travel_cost(RID p_region);

	static bool region_owns_point(RID p_region, const Vector3& p_point);

	static void region_set_map(RID p_region, RID p_map);
	static RID region_get_map(RID p_region);

	static void region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers);
	static uint32_t region_get_navigation_layers(RID p_region);

	static void region_set_transform(RID p_region, Transform3D p_transform);
	static Transform3D region_get_transform(RID p_region);

	static void region_set_navigation_mesh(RID p_region, Ref<NavigationMesh> p_navigation_mesh);

	static int region_get_connections_count(RID p_region);
	static Vector3 region_get_connection_pathway_start(RID p_region, int p_connection_id);
	static Vector3 region_get_connection_pathway_end(RID p_region, int p_connection_id);

	static Vector3 region_get_closest_point_to_segment(
		RID p_region, const Vector3& p_from, const Vector3& p_to, bool p_use_collision = false);
	static Vector3 region_get_closest_point(RID p_region, const Vector3& p_point);
	static Vector3 region_get_closest_point_normal(RID p_region, const Vector3& p_point);
	static Vector3 region_get_random_point(
		RID p_region, uint32_t p_navigation_layers, bool p_uniformly);

	static AABB region_get_bounds(RID p_region);

	/* LINK API */

	static RID link_create();
	static uint32_t link_get_iteration_id(RID p_link);

	static void link_set_map(RID p_link, RID p_map);
	static RID link_get_map(RID p_link);

	static void link_set_enabled(RID p_link, bool p_enabled);
	static bool link_get_enabled(RID p_link);

	static void link_set_bidirectional(RID p_link, bool p_bidirectional);
	static bool link_is_bidirectional(RID p_link);

	static void link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers);
	static uint32_t link_get_navigation_layers(RID p_link);

	static void link_set_start_position(RID p_link, Vector3 p_position);
	static Vector3 link_get_start_position(RID p_link);

	static void link_set_end_position(RID p_link, Vector3 p_position);
	static Vector3 link_get_end_position(RID p_link);

	static void link_set_enter_cost(RID p_link, real_t p_enter_cost);
	static real_t link_get_enter_cost(RID p_link);

	static void link_set_travel_cost(RID p_link, real_t p_travel_cost);
	static real_t link_get_travel_cost(RID p_link);

	/* AGENT API */

	static RID agent_create();

	static void agent_set_map(RID p_agent, RID p_map);
	static RID agent_get_map(RID p_agent);

	static void agent_set_paused(RID p_agent, bool p_paused);
	static bool agent_get_paused(RID p_agent);

	static void agent_set_avoidance_enabled(RID p_agent, bool p_enabled);
	static bool agent_get_avoidance_enabled(RID p_agent);

	static void agent_set_use_3d_avoidance(RID p_agent, bool p_enabled);
	static bool agent_get_use_3d_avoidance(RID p_agent);

	static void agent_set_neighbor_distance(RID p_agent, real_t p_distance);
	static real_t agent_get_neighbor_distance(RID p_agent);

	static void agent_set_max_neighbors(RID p_agent, int p_count);
	static int agent_get_max_neighbors(RID p_agent);

	static void agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon);
	static real_t agent_get_time_horizon_agents(RID p_agent);

	static void agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon);
	static real_t agent_get_time_horizon_obstacles(RID p_agent);

	static void agent_set_radius(RID p_agent, real_t p_radius);
	static real_t agent_get_radius(RID p_agent);

	static void agent_set_height(RID p_agent, real_t p_height);
	static real_t agent_get_height(RID p_agent);

	static void agent_set_max_speed(RID p_agent, real_t p_max_speed);
	static real_t agent_get_max_speed(RID p_agent);

	static void agent_set_velocity_forced(RID p_agent, Vector3 p_velocity);
	static void agent_set_velocity(RID p_agent, Vector3 p_velocity);
	static Vector3 agent_get_velocity(RID p_agent);

	static void agent_set_position(RID p_agent, Vector3 p_position);
	static Vector3 agent_get_position(RID p_agent);

	static bool agent_is_map_changed(RID p_agent);
	static bool agent_has_avoidance_callback(RID p_agent);

	static void agent_set_avoidance_layers(RID p_agent, uint32_t p_layers);
	static uint32_t agent_get_avoidance_layers(RID p_agent);

	static void agent_set_avoidance_mask(RID p_agent, uint32_t p_mask);
	static uint32_t agent_get_avoidance_mask(RID p_agent);

	static void agent_set_avoidance_priority(RID p_agent, real_t p_priority);
	static real_t agent_get_avoidance_priority(RID p_agent);

	/* OBSTACLE API */

	static RID obstacle_create();

	static void obstacle_set_map(RID p_obstacle, RID p_map);
	static RID obstacle_get_map(RID p_obstacle);

	static void obstacle_set_paused(RID p_obstacle, bool p_paused);
	static bool obstacle_get_paused(RID p_obstacle);

	static void obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_avoidance_enabled(RID p_obstacle);

	static void obstacle_set_use_3d_avoidance(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_use_3d_avoidance(RID p_obstacle);

	static void obstacle_set_radius(RID p_obstacle, real_t p_radius);
	static real_t obstacle_get_radius(RID p_obstacle);

	static void obstacle_set_height(RID p_obstacle, real_t p_height);
	static real_t obstacle_get_height(RID p_obstacle);

	static void obstacle_set_velocity(RID p_obstacle, Vector3 p_velocity);
	static Vector3 obstacle_get_velocity(RID p_obstacle);

	static void obstacle_set_position(RID p_obstacle, Vector3 p_position);
	static Vector3 obstacle_get_position(RID p_obstacle);

	static void obstacle_set_vertices(RID p_obstacle, const Vector<Vector3>& p_vertices);
	static Vector<Vector3> obstacle_get_vertices(RID p_obstacle);

	static void obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers);
	static uint32_t obstacle_get_avoidance_layers(RID p_obstacle);

	/* PATH QUERY & UTILITIES */

	static void query_path(const Ref<NavigationPathQueryParameters3D>& p_query_parameters,
		Ref<NavigationPathQueryResult3D> p_query_result);
	static Vector<Vector3> simplify_path(const Vector<Vector3>& p_path, real_t p_epsilon);

	/* DEBUG API */

	static void set_debug_enabled(bool p_enabled);
	static bool get_debug_enabled();

#ifdef DEBUG_ENABLED
	static void set_debug_navigation_enabled(bool p_enabled);
	static bool get_debug_navigation_enabled();

	static void set_debug_avoidance_enabled(bool p_enabled);
	static bool get_debug_avoidance_enabled();

	static void set_debug_navigation_edge_connection_color(const Color& p_color);
	static Color get_debug_navigation_edge_connection_color();

	static void set_debug_navigation_geometry_edge_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_color();

	static void set_debug_navigation_geometry_face_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_color();

	static void set_debug_navigation_geometry_edge_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_disabled_color();

	static void set_debug_navigation_geometry_face_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_disabled_color();

	static void set_debug_navigation_link_connection_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_color();

	static void set_debug_navigation_link_connection_disabled_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_disabled_color();

	static void set_debug_navigation_agent_path_color(const Color& p_color);
	static Color get_debug_navigation_agent_path_color();

	static void set_debug_navigation_avoidance_agents_radius_color(const Color& p_color);
	static Color get_debug_navigation_avoidance_agents_radius_color();

	static void set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color);
	static Color get_debug_navigation_avoidance_obstacles_radius_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushin_face_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushout_face_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushin_edge_color();

	static void set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
		const Color& p_color);
	static Color get_debug_navigation_avoidance_static_obstacle_pushout_edge_color();

	static void set_debug_navigation_enable_edge_connections(bool p_value);
	static bool get_debug_navigation_enable_edge_connections();

	static void set_debug_navigation_enable_edge_connections_xray(bool p_value);
	static bool get_debug_navigation_enable_edge_connections_xray();

	static void set_debug_navigation_enable_edge_lines(bool p_value);
	static bool get_debug_navigation_enable_edge_lines();

	static void set_debug_navigation_enable_edge_lines_xray(bool p_value);
	static bool get_debug_navigation_enable_edge_lines_xray();

	static void set_debug_navigation_enable_geometry_face_random_color(bool p_value);
	static bool get_debug_navigation_enable_geometry_face_random_color();

	static void set_debug_navigation_enable_link_connections(bool p_value);
	static bool get_debug_navigation_enable_link_connections();

	static void set_debug_navigation_enable_link_connections_xray(bool p_value);
	static bool get_debug_navigation_enable_link_connections_xray();

	static void set_debug_navigation_enable_agent_paths(bool p_value);
	static bool get_debug_navigation_enable_agent_paths();

	static void set_debug_navigation_enable_agent_paths_xray(bool p_value);
	static bool get_debug_navigation_enable_agent_paths_xray();

	static void set_debug_navigation_agent_path_point_size(real_t p_point_size);
	static real_t get_debug_navigation_agent_path_point_size();

	static void set_debug_navigation_avoidance_enable_agents_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_agents_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_static(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_static();

	static Ref<StandardMaterial3D> get_debug_navigation_geometry_face_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_edge_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_face_disabled_material();
	static Ref<StandardMaterial3D> get_debug_navigation_geometry_edge_disabled_material();
	static Ref<StandardMaterial3D> get_debug_navigation_edge_connections_material();
	static Ref<StandardMaterial3D> get_debug_navigation_link_connections_material();
	static Ref<StandardMaterial3D> get_debug_navigation_link_connections_disabled_material();

	static Ref<StandardMaterial3D> get_debug_navigation_agent_path_line_material();
	static Ref<StandardMaterial3D> get_debug_navigation_agent_path_point_material();

	static Ref<StandardMaterial3D> get_debug_navigation_avoidance_agents_radius_material();
	static Ref<StandardMaterial3D> get_debug_navigation_avoidance_obstacles_radius_material();

	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushin_face_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushout_face_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushin_edge_material();
	static Ref<StandardMaterial3D>
	get_debug_navigation_avoidance_static_obstacle_pushout_edge_material();
#endif // DEBUG_ENABLED

	/* LIFECYCLE DISALLOWANCE */

	NavigationServer3D() = delete;
	NavigationServer3D(const NavigationServer3D&) = delete;
	NavigationServer3D& operator=(const NavigationServer3D&) = delete;
	~NavigationServer3D() = delete;
};
// --- End servers/navigation_3d/navigation_server_3d.h ---
