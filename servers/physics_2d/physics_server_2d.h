/**************************************************************************/
/*  physics_server_2d.h                                                   */
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
// --- Folded from servers/physics_2d/direct_states/physics_direct_body_state_2d.h ---
// --- Folded from servers/physics_2d/direct_states/physics_direct_space_state_2d.h ---
// --- Folded from servers/physics_2d/physics_server_2d_types.h ---
#include "core/math/vector2.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"

namespace PhysicsServer2DTypes
{

struct RayParameters
{
	Vector2 from;
	Vector2 to;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;

	bool hit_from_inside = false;
};

struct RayResult
{
	Vector2 position;
	Vector2 normal;
	RID rid;
	int shape = 0;
};

struct ShapeResult
{
	RID rid;
	int shape = 0;
};

struct PointParameters
{
	Vector2 position;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;

	bool pick_point = false;
};

struct ShapeParameters
{
	RID shape_rid;
	Vector2 motion;
	real_t margin = 0.0;
	HashSet<RID> exclude;
	uint32_t collision_mask = UINT32_MAX;

	bool collide_with_bodies = true;
	bool collide_with_areas = false;
};

struct ShapeRestInfo
{
	Vector2 point;
	Vector2 normal;
	RID rid;
	int shape = 0;
	Vector2 linear_velocity; // Velocity at contact point.
};

struct MotionParameters
{
	Vector2 motion;
	real_t margin = 0.08;
	bool collide_separation_ray = false;
	HashSet<RID> exclude_bodies;
	bool recovery_as_collision = false;

	MotionParameters() {}
};

struct MotionResult
{
	Vector2 travel;
	Vector2 remainder;

	Vector2 collision_point;
	Vector2 collision_normal;
	Vector2 collider_velocity;
	real_t collision_depth = 0.0;
	real_t collision_safe_fraction = 0.0;
	real_t collision_unsafe_fraction = 0.0;
	int collision_local_shape = 0;
	RID collider;
	int collider_shape = 0;

	real_t get_angle(Vector2 p_up_direction) const
	{
		return Math::acos(collision_normal.dot(p_up_direction));
	}
};

#ifndef DISABLE_DEPRECATED
// Graveyard.
#endif

} // namespace PhysicsServer2DTypes

// Alias to make it easier to use.
#define PS2DT PhysicsServer2DTypes



// --- End servers/physics_2d/physics_server_2d_types.h ---
// --- Folded from servers/physics_2d/queries/physics_ray_query_parameters_2d.h ---
#include "core/types.h"


template <typename T> class TypedArray;

class PhysicsRayQueryParameters2D : public RefCounted
{
	PS2DT::RayParameters parameters;

protected:

public:
	static Ref<PhysicsRayQueryParameters2D> create(
		Vector2 p_from, Vector2 p_to, uint32_t p_mask, const TypedArray<RID>& p_exclude);

	const PS2DT::RayParameters& get_parameters() const { return parameters; }

	void set_from(const Vector2& p_from) { parameters.from = p_from; }

	const Vector2& get_from() const { return parameters.from; }

	void set_to(const Vector2& p_to) { parameters.to = p_to; }

	const Vector2& get_to() const { return parameters.to; }

	void set_collision_mask(uint32_t p_mask) { parameters.collision_mask = p_mask; }

	uint32_t get_collision_mask() const { return parameters.collision_mask; }

	void set_collide_with_bodies(bool p_enable) { parameters.collide_with_bodies = p_enable; }

	bool is_collide_with_bodies_enabled() const { return parameters.collide_with_bodies; }

	void set_collide_with_areas(bool p_enable) { parameters.collide_with_areas = p_enable; }

	bool is_collide_with_areas_enabled() const { return parameters.collide_with_areas; }

	void set_hit_from_inside(bool p_enable) { parameters.hit_from_inside = p_enable; }

	bool is_hit_from_inside_enabled() const { return parameters.hit_from_inside; }

	void set_exclude(const TypedArray<RID>& p_exclude);
	TypedArray<RID> get_exclude() const;
};



// --- End servers/physics_2d/queries/physics_ray_query_parameters_2d.h ---
// --- Folded from servers/physics_2d/queries/physics_shape_query_parameters_2d.h ---
#include "core/types.h"


class Resource;
template <typename T> class Ref;

class PhysicsShapeQueryParameters2D : public RefCounted
{
	PS2DT::ShapeParameters parameters;

	Ref<Resource> shape_ref;

protected:

public:
	const PS2DT::ShapeParameters& get_parameters() const { return parameters; }

	void set_shape(const Ref<Resource>& p_shape_ref);
	Ref<Resource> get_shape() const;

	void set_shape_rid(const RID& p_shape);

	RID get_shape_rid() const { return parameters.shape_rid; }

	void set_motion(const Vector2& p_motion) { parameters.motion = p_motion; }

	const Vector2& get_motion() const { return parameters.motion; }

	void set_margin(real_t p_margin) { parameters.margin = p_margin; }

	real_t get_margin() const { return parameters.margin; }

	void set_collision_mask(uint32_t p_mask) { parameters.collision_mask = p_mask; }

	uint32_t get_collision_mask() const { return parameters.collision_mask; }

	void set_collide_with_bodies(bool p_enable) { parameters.collide_with_bodies = p_enable; }

	bool is_collide_with_bodies_enabled() const { return parameters.collide_with_bodies; }

	void set_collide_with_areas(bool p_enable) { parameters.collide_with_areas = p_enable; }

	bool is_collide_with_areas_enabled() const { return parameters.collide_with_areas; }
};



// --- End servers/physics_2d/queries/physics_shape_query_parameters_2d.h ---

class GodotSpace2D;

class PhysicsDirectSpaceState2D final
{
	GodotSpace2D* space = nullptr;

	Vector<real_t> _cast_motion(PhysicsShapeQueryParameters2D* rp_shape_query);

public:
	PhysicsDirectSpaceState2D() = default;
	explicit PhysicsDirectSpaceState2D(GodotSpace2D* p_space);

	bool intersect_ray(
		const PS2DT::RayParameters& p_parameters, PS2DT::RayResult& r_result);

	int intersect_point(const PS2DT::PointParameters& p_parameters,
		PS2DT::ShapeResult* r_results, int p_result_max);

	int intersect_shape(const PS2DT::ShapeParameters& p_parameters,
		PS2DT::ShapeResult* r_results, int p_result_max);
	bool cast_motion(const PS2DT::ShapeParameters& p_parameters, real_t& p_closest_safe,
		real_t& p_closest_unsafe);
	bool collide_shape(const PS2DT::ShapeParameters& p_parameters, Vector2* r_results,
		int p_result_max, int& r_result_count);
	bool rest_info(
		const PS2DT::ShapeParameters& p_parameters, PS2DT::ShapeRestInfo* r_info);

	GodotSpace2D& get_space() const { return *space; }
};

// --- End servers/physics_2d/direct_states/physics_direct_space_state_2d.h ---

class GodotBody2D;

class PhysicsDirectBodyState2D final
{
	GodotBody2D* body = nullptr;

public:
	PhysicsDirectBodyState2D() = default;
	explicit PhysicsDirectBodyState2D(GodotBody2D* p_body);

	Vector2 get_total_gravity() const;
	real_t get_total_linear_damp() const;
	real_t get_total_angular_damp() const;

	Vector2 get_center_of_mass() const;
	Vector2 get_center_of_mass_local() const;
	real_t get_inverse_mass() const;
	real_t get_inverse_inertia() const;

	void set_linear_velocity(const Vector2& p_velocity);
	Vector2 get_linear_velocity() const;

	void set_angular_velocity(real_t p_velocity);
	real_t get_angular_velocity() const;

	void set_transform(const Transform2D& p_transform);
	Transform2D get_transform() const;

	Vector2 get_velocity_at_local_position(const Vector2& p_position) const;

	void apply_central_impulse(const Vector2& p_impulse);
	void apply_torque_impulse(real_t p_torque);
	void apply_impulse(const Vector2& p_impulse, const Vector2& p_position = Vector2());

	void apply_central_force(const Vector2& p_force);
	void apply_force(const Vector2& p_force, const Vector2& p_position = Vector2());
	void apply_torque(real_t p_torque);

	void add_constant_central_force(const Vector2& p_force);
	void add_constant_force(const Vector2& p_force, const Vector2& p_position = Vector2());
	void add_constant_torque(real_t p_torque);

	void set_constant_force(const Vector2& p_force);
	Vector2 get_constant_force() const;

	void set_constant_torque(real_t p_torque);
	real_t get_constant_torque() const;

	void set_sleep_state(bool p_enable);
	bool is_sleeping() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	int get_contact_count() const;

	Vector2 get_contact_local_position(int p_contact_idx) const;
	Vector2 get_contact_local_normal(int p_contact_idx) const;
	int get_contact_local_shape(int p_contact_idx) const;
	Vector2 get_contact_local_velocity_at_position(int p_contact_idx) const;

	RID get_contact_collider(int p_contact_idx) const;
	Vector2 get_contact_collider_position(int p_contact_idx) const;
	int get_contact_collider_shape(int p_contact_idx) const;
	Vector2 get_contact_collider_velocity_at_position(int p_contact_idx) const;
	Vector2 get_contact_impulse(int p_contact_idx) const;

	real_t get_step() const;
	void integrate_forces();

	PhysicsDirectSpaceState2D* get_space_state();
};

// --- End servers/physics_2d/direct_states/physics_direct_body_state_2d.h ---

// --- Folded from servers/physics_2d/physics_server_2d_enums.h ---
namespace PhysicsServer2DEnums {

/* SHAPE API */

enum ShapeType {
	SHAPE_WORLD_BOUNDARY, ///< plane:"plane"
	SHAPE_SEPARATION_RAY, ///< float:"length"
	SHAPE_SEGMENT, ///< float:"length"
	SHAPE_CIRCLE, ///< float:"radius"
	SHAPE_RECTANGLE, ///< vec3:"extents"
	SHAPE_CAPSULE,
	SHAPE_CONVEX_POLYGON, ///< array of planes:"planes"
	SHAPE_CONCAVE_POLYGON, ///< Vector2 array:"triangles" , or Dictionary with "indices" (int array) and "triangles" (Vector2 array)
	SHAPE_CUSTOM, ///< Server-Implementation based custom shape, calling shape_create() with this value will result in an error
};

/* SPACE API */

enum SpaceParameter {
	SPACE_PARAM_CONTACT_RECYCLE_RADIUS,
	SPACE_PARAM_CONTACT_MAX_SEPARATION,
	SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION,
	SPACE_PARAM_CONTACT_DEFAULT_BIAS,
	SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD,
	SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD,
	SPACE_PARAM_BODY_TIME_TO_SLEEP,
	SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS,
	SPACE_PARAM_SOLVER_ITERATIONS,
};

/* AREA API */

enum AreaParameter {
	AREA_PARAM_GRAVITY_OVERRIDE_MODE,
	AREA_PARAM_GRAVITY,
	AREA_PARAM_GRAVITY_VECTOR,
	AREA_PARAM_GRAVITY_IS_POINT,
	AREA_PARAM_GRAVITY_POINT_UNIT_DISTANCE,
	AREA_PARAM_LINEAR_DAMP_OVERRIDE_MODE,
	AREA_PARAM_LINEAR_DAMP,
	AREA_PARAM_ANGULAR_DAMP_OVERRIDE_MODE,
	AREA_PARAM_ANGULAR_DAMP,
	AREA_PARAM_PRIORITY
};

enum AreaSpaceOverrideMode {
	AREA_SPACE_OVERRIDE_DISABLED,
	AREA_SPACE_OVERRIDE_COMBINE,
	AREA_SPACE_OVERRIDE_COMBINE_REPLACE, // Combines, then discards all subsequent calculations
	AREA_SPACE_OVERRIDE_REPLACE,
	AREA_SPACE_OVERRIDE_REPLACE_COMBINE // Discards all previous calculations, then keeps combining
};

/* BODY API */

enum BodyMode {
	BODY_MODE_STATIC,
	BODY_MODE_KINEMATIC,
	BODY_MODE_RIGID,
	BODY_MODE_RIGID_LINEAR,
};

enum BodyDampMode {
	BODY_DAMP_MODE_COMBINE,
	BODY_DAMP_MODE_REPLACE,
};

// common body variables
enum BodyParameter {
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

enum BodyState {
	BODY_STATE_TRANSFORM,
	BODY_STATE_LINEAR_VELOCITY,
	BODY_STATE_ANGULAR_VELOCITY,
	BODY_STATE_SLEEPING,
	BODY_STATE_CAN_SLEEP,
};

enum CCDMode {
	CCD_MODE_DISABLED,
	CCD_MODE_CAST_RAY,
	CCD_MODE_CAST_SHAPE,
};

/* JOINT API */

enum JointType {
	JOINT_TYPE_PIN,
	JOINT_TYPE_GROOVE,
	JOINT_TYPE_DAMPED_SPRING,
	JOINT_TYPE_MAX
};

enum JointParam {
	JOINT_PARAM_BIAS,
	JOINT_PARAM_MAX_BIAS,
	JOINT_PARAM_MAX_FORCE,
};

enum PinJointParam {
	PIN_JOINT_SOFTNESS,
	PIN_JOINT_LIMIT_UPPER,
	PIN_JOINT_LIMIT_LOWER,
	PIN_JOINT_MOTOR_TARGET_VELOCITY
};

enum PinJointFlag {
	PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED,
	PIN_JOINT_FLAG_MOTOR_ENABLED
};

enum DampedSpringParam {
	DAMPED_SPRING_REST_LENGTH,
	DAMPED_SPRING_STIFFNESS,
	DAMPED_SPRING_DAMPING
};

/* QUERY API */

enum AreaBodyStatus {
	AREA_BODY_ADDED,
	AREA_BODY_REMOVED
};

enum ProcessInfo {
	INFO_ACTIVE_OBJECTS,
	INFO_COLLISION_PAIRS,
	INFO_ISLAND_COUNT
};

#ifndef DISABLE_DEPRECATED
// Graveyard.
#endif

} // namespace PhysicsServer2DEnums

// Alias to make it easier to use.
#define PS2DE PhysicsServer2DEnums

// --- End servers/physics_2d/physics_server_2d_enums.h ---


// --- Folded from servers/physics_2d/queries/physics_testmotion_query_parameters_2d.h ---
#include "core/types.h"


class PhysicsTestMotionParameters2D : public RefCounted
{
	PS2DT::MotionParameters parameters;

protected:

public:
	const PS2DT::MotionParameters& get_parameters() const { return parameters; }

	const Vector2& get_motion() const { return parameters.motion; }

	void set_motion(const Vector2& p_motion) { parameters.motion = p_motion; }

	real_t get_margin() const { return parameters.margin; }

	void set_margin(real_t p_margin) { parameters.margin = p_margin; }

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



// --- End servers/physics_2d/queries/physics_testmotion_query_parameters_2d.h ---
// --- Folded from servers/physics_2d/queries/physics_testmotion_query_result_2d.h ---

#include "core/types.h"

class PhysicsTestMotionResult2D : public RefCounted
{
	PS2DT::MotionResult result;

protected:

public:
	PS2DT::MotionResult* get_result_ptr() { return &result; }

	Vector2 get_travel() const;
	Vector2 get_remainder() const;

	Vector2 get_collision_point() const;
	Vector2 get_collision_normal() const;
	Vector2 get_collider_velocity() const;
	RID get_collider_rid() const;
	int get_collider_shape() const;
	int get_collision_local_shape() const;
	real_t get_collision_depth() const;
	real_t get_collision_safe_fraction() const;
	real_t get_collision_unsafe_fraction() const;
};



// --- End servers/physics_2d/queries/physics_testmotion_query_result_2d.h ---

class GodotPhysicsServer2D;
class PhysicsDirectSpaceState2D;
template <typename T> class TypedArray;

template <typename T> class Ref;

class PhysicsTestMotionParameters2D;
class PhysicsTestMotionResult2D;

class PhysicsServer2D final
{
public:
	struct Data
	{
		GodotPhysicsServer2D* server = nullptr;
	};

	static inline Data* data = nullptr;

	static void initialize(bool p_using_threads = false);
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->server != nullptr;
	}
	_FORCE_INLINE_ static GodotPhysicsServer2D* get_driver() { return data ? data->server : nullptr; }

	static RID world_boundary_shape_create();
	static RID separation_ray_shape_create();
	static RID segment_shape_create();
	static RID circle_shape_create();
	static RID rectangle_shape_create();
	static RID capsule_shape_create();
	static RID convex_polygon_shape_create();
	static RID concave_polygon_shape_create();
	static void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);
	static PS2DE::ShapeType shape_get_type(RID p_shape);
	static real_t shape_get_custom_solver_bias(RID p_shape);
	static bool shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count);
	static RID space_create();
	static void space_set_active(RID p_space, bool p_active);
	static bool space_is_active(RID p_space);
	static void space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value);
	static real_t space_get_param(RID p_space, PS2DE::SpaceParameter p_param);
	static PhysicsDirectSpaceState2D* space_get_direct_state(RID p_space);
	static void space_set_debug_contacts(RID p_space, int p_max_contacts);
	static Vector<Vector2> space_get_contacts(RID p_space);
	static int space_get_contact_count(RID p_space);
	static RID area_create();
	static void area_set_space(RID p_area, RID p_space);
	static RID area_get_space(RID p_area);
	static void area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	static void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	static void area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform);
	static int area_get_shape_count(RID p_area);
	static RID area_get_shape(RID p_area, int p_shape_idx);
	static Transform2D area_get_shape_transform(RID p_area, int p_shape_idx);
	static void area_remove_shape(RID p_area, int p_shape_idx);
	static void area_clear_shapes(RID p_area);
	static void area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled);
	static void area_set_transform(RID p_area, const Transform2D& p_transform);
	static Transform2D area_get_transform(RID p_area);
	static void area_set_collision_layer(RID p_area, uint32_t p_layer);
	static uint32_t area_get_collision_layer(RID p_area);
	static void area_set_collision_mask(RID p_area, uint32_t p_mask);
	static uint32_t area_get_collision_mask(RID p_area);
	static void area_set_monitorable(RID p_area, bool p_monitorable);
	static void area_set_pickable(RID p_area, bool p_pickable);
	static RID body_create();
	static void body_set_space(RID p_body, RID p_space);
	static RID body_get_space(RID p_body);
	static void body_set_mode(RID p_body, PS2DE::BodyMode p_mode);
	static PS2DE::BodyMode body_get_mode(RID p_body);
	static void body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	static void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	static void body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform);
	static int body_get_shape_count(RID p_body);
	static RID body_get_shape(RID p_body, int p_shape_idx);
	static Transform2D body_get_shape_transform(RID p_body, int p_shape_idx);
	static void body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled);
	static void body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin = 0, const Vector2& p_direction = Vector2(0, 1));
	static void body_remove_shape(RID p_body, int p_shape_idx);
	static void body_clear_shapes(RID p_body);
	static void body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode);
	static PS2DE::CCDMode body_get_continuous_collision_detection_mode(RID p_body);
	static void body_set_collision_layer(RID p_body, uint32_t p_layer);
	static uint32_t body_get_collision_layer(RID p_body);
	static void body_set_collision_mask(RID p_body, uint32_t p_mask);
	static uint32_t body_get_collision_mask(RID p_body);
	static void body_set_collision_priority(RID p_body, real_t p_priority);
	static real_t body_get_collision_priority(RID p_body);
	static void body_reset_mass_properties(RID p_body);
	static void body_apply_central_impulse(RID p_body, const Vector2& p_impulse);
	static void body_apply_torque_impulse(RID p_body, real_t p_torque);
	static void body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position = Vector2());
	static void body_apply_central_force(RID p_body, const Vector2& p_force);
	static void body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	static void body_apply_torque(RID p_body, real_t p_torque);
	static void body_add_constant_central_force(RID p_body, const Vector2& p_force);
	static void body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	static void body_add_constant_torque(RID p_body, real_t p_torque);
	static void body_set_constant_force(RID p_body, const Vector2& p_force);
	static Vector2 body_get_constant_force(RID p_body);
	static void body_set_constant_torque(RID p_body, real_t p_torque);
	static real_t body_get_constant_torque(RID p_body);
	static void body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity);
	static void body_add_collision_exception(RID p_body, RID p_body_b);
	static void body_remove_collision_exception(RID p_body, RID p_body_b);
	static void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);
	static void body_set_max_contacts_reported(RID p_body, int p_contacts);
	static int body_get_max_contacts_reported(RID p_body);
	static void body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold);
	static real_t body_get_contacts_reported_depth_threshold(RID p_body);
	static void body_set_omit_force_integration(RID p_body, bool p_omit);
	static bool body_is_omitting_force_integration(RID p_body);
	static bool body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count);
	static void body_set_pickable(RID p_body, bool p_pickable);
	static PhysicsDirectBodyState2D* body_get_direct_state(RID p_body);
	static bool body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result = nullptr);
	static RID joint_create();
	static void joint_clear(RID p_joint);
	static void joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value);
	static real_t joint_get_param(RID p_joint, PS2DE::JointParam p_param);
	static void joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable);
	static bool joint_is_disabled_collisions_between_bodies(RID p_joint);
	static void joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b = RID());
	static void joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b);
	static void joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b = RID());
	static void pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value);
	static real_t pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param);
	static void pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled);
	static bool pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag);
	static void damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value);
	static real_t damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param);
	static PS2DE::JointType joint_get_type(RID p_joint);
	static void free_rid(RID p_rid);
	static void set_active(bool p_active);
	static void init();
	static void step(real_t p_step);
	static void sync();
	static void flush_queries();
	static void end_sync();
	static void finish();
	static bool is_flushing_queries();
	static int get_process_info(PS2DE::ProcessInfo p_info);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] static void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	PhysicsServer2D() = delete;
	PhysicsServer2D(const PhysicsServer2D&) = delete;
	PhysicsServer2D& operator=(const PhysicsServer2D&) = delete;
	~PhysicsServer2D() = delete;
};

// --- Folded from servers/physics_2d/physics_server_2d_constants.h ---
namespace PhysicsServer2DConstants {

constexpr int MAX_CONTACTS_REPORTED_2D_MAX = 4096;

} // namespace PhysicsServer2DConstants

// Alias to make it easier to use.
#define PS2DC PhysicsServer2DConstants

// --- End servers/physics_2d/physics_server_2d_constants.h ---

// --- Folded from modules/godot_physics_2d/godot_physics_server_2d.h ---
#include "core/types.h"
// --- Folded from modules/godot_physics_2d/godot_constraints_2d.h ---
// --- Folded from modules/godot_physics_2d/godot_objects_2d.h ---
#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/vector.h"
#include "core/typedefs.h"
// --- Folded from modules/godot_physics_2d/godot_shape_2d.h ---
#include "core/types.h"
#include "core/templates/hash_map.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/vector.h"


class GodotCollisionObject2D;

class GodotShape2D final
{
public:
	// Returns true to stop the query.
	typedef bool (*QueryCallback)(void* p_userdata, GodotShape2D* p_convex);

	struct ConvexPoint
	{
		Vector2 pos;
		Vector2 normal; // normal to next segment
	};

	struct ConcaveSegment
	{
		int points[2] = {};
	};

	struct BVH
	{
		Rect2 aabb;
		int left = 0, right = 0;
	};

private:
	RID self;
	PS2DE::ShapeType type = PS2DE::SHAPE_CUSTOM;
	Rect2 aabb;
	bool configured = false;
	real_t custom_bias = 0.0;

	HashMap<GodotCollisionObject2D*, int> owners;

	// World boundary data
	Vector2 wb_normal;
	real_t wb_d = 0.0;

	// Separation ray data
	real_t ray_length = 0.0;
	bool slide_on_slope = false;

	// Segment data
	Vector2 seg_a;
	Vector2 seg_b;
	Vector2 seg_n;

	// Circle / Capsule data
	real_t radius = 0.0;
	real_t height = 0.0;

	// Rectangle data
	Vector2 half_extents;

	// Convex polygon data
	ConvexPoint* convex_points = nullptr;
	int convex_point_count = 0;

	// Concave polygon data
	Vector<ConcaveSegment> concave_segments;
	Vector<Point2> concave_points;
	Vector<BVH> concave_bvh;
	int concave_bvh_depth = 0;

	struct BVH_CompareX
	{
		_FORCE_INLINE_ bool operator()(const BVH& a, const BVH& b) const
		{
			return (a.aabb.position.x + a.aabb.size.x * 0.5) <
				   (b.aabb.position.x + b.aabb.size.x * 0.5);
		}
	};

	struct BVH_CompareY
	{
		_FORCE_INLINE_ bool operator()(const BVH& a, const BVH& b) const
		{
			return (a.aabb.position.y + a.aabb.size.y * 0.5) <
				   (b.aabb.position.y + b.aabb.size.y * 0.5);
		}
	};

	static constexpr double segment_is_valid_support_threshold = 0.99998;
	static inline const double segment_is_valid_support_threshold_lower =
		Math::sqrt(1.0 - segment_is_valid_support_threshold * segment_is_valid_support_threshold);

	void configure(const Rect2& p_aabb);
	int _generate_bvh(BVH* p_bvh, int p_len, int p_depth);

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	_FORCE_INLINE_ PS2DE::ShapeType get_type() const { return type; }

	_FORCE_INLINE_ Rect2 get_aabb() const { return aabb; }

	_FORCE_INLINE_ bool is_configured() const { return configured; }

	_FORCE_INLINE_ bool allows_one_way_collision() const
	{
		return type != PS2DE::SHAPE_SEPARATION_RAY;
	}

	_FORCE_INLINE_ bool is_concave() const { return type == PS2DE::SHAPE_CONCAVE_POLYGON; }

	_FORCE_INLINE_ void set_custom_bias(real_t p_bias) { custom_bias = p_bias; }

	_FORCE_INLINE_ real_t get_custom_bias() const { return custom_bias; }

	void add_owner(GodotCollisionObject2D* p_owner);
	void remove_owner(GodotCollisionObject2D* p_owner);
	bool is_owner(GodotCollisionObject2D* p_owner) const;
	const HashMap<GodotCollisionObject2D*, int>& get_owners() const;

	// Typed shape configuration setters
	void set_world_boundary(const Vector2& p_normal, real_t p_d);
	void set_separation_ray(real_t p_length, bool p_slide_on_slope);
	void set_segment(const Vector2& p_a, const Vector2& p_b);
	void set_circle(real_t p_radius);
	void set_rectangle(const Vector2& p_half_extents);
	void set_capsule(real_t p_height, real_t p_radius);
	void set_convex_polygon(const Vector<Vector2>& p_points);
	void set_concave_polygon(const Vector<Vector2>& p_segments);

	// Shape-specific inline accessors
	_FORCE_INLINE_ const Vector2& get_world_boundary_normal() const { return wb_normal; }
	_FORCE_INLINE_ real_t get_world_boundary_d() const { return wb_d; }

	_FORCE_INLINE_ real_t get_length() const { return ray_length; }
	_FORCE_INLINE_ bool get_slide_on_slope() const { return slide_on_slope; }

	_FORCE_INLINE_ const Vector2& get_a() const { return seg_a; }
	_FORCE_INLINE_ const Vector2& get_b() const { return seg_b; }
	_FORCE_INLINE_ const Vector2& get_segment_normal() const { return seg_n; }
	_FORCE_INLINE_ Vector2 get_xformed_normal(const Transform2D& p_xform) const
	{
		return (p_xform.xform(seg_b) - p_xform.xform(seg_a)).normalized().orthogonal();
	}

	_FORCE_INLINE_ const real_t& get_radius() const { return radius; }
	_FORCE_INLINE_ const real_t& get_height() const { return height; }

	_FORCE_INLINE_ const Vector2& get_half_extents() const { return half_extents; }

	_FORCE_INLINE_ Vector2 get_circle_axis(
		const Transform2D& p_xform, const Transform2D& p_xform_inv, const Vector2& p_circle) const
	{
		Vector2 local_v = p_xform_inv.xform(p_circle);

		Vector2 he((local_v.x < 0) ? -half_extents.x : half_extents.x,
			(local_v.y < 0) ? -half_extents.y : half_extents.y);

		return (p_xform.xform(he) - p_circle).normalized();
	}

	_FORCE_INLINE_ Vector2 get_box_axis(const Transform2D& p_xform, const Transform2D& p_xform_inv,
		const GodotShape2D* p_B, const Transform2D& p_B_xform,
		const Transform2D& p_B_xform_inv) const
	{
		Vector2 a, b;

		{
			Vector2 local_v = p_xform_inv.xform(p_B_xform.get_origin());

			Vector2 he((local_v.x < 0) ? -half_extents.x : half_extents.x,
				(local_v.y < 0) ? -half_extents.y : half_extents.y);

			a = p_xform.xform(he);
		}
		{
			Vector2 local_v = p_B_xform_inv.xform(p_xform.get_origin());

			Vector2 he((local_v.x < 0) ? -p_B->half_extents.x : p_B->half_extents.x,
				(local_v.y < 0) ? -p_B->half_extents.y : p_B->half_extents.y);

			b = p_B_xform.xform(he);
		}

		return (a - b).normalized();
	}

	_FORCE_INLINE_ int get_point_count() const { return convex_point_count; }
	_FORCE_INLINE_ const Vector2& get_point(int p_idx) const { return convex_points[p_idx].pos; }
	_FORCE_INLINE_ const Vector2& get_convex_segment_normal(int p_idx) const
	{
		return convex_points[p_idx].normal;
	}
	_FORCE_INLINE_ Vector2 get_xformed_segment_normal(const Transform2D& p_xform, int p_idx) const
	{
		Vector2 a = convex_points[p_idx].pos;
		p_idx++;
		Vector2 b = convex_points[p_idx == convex_point_count ? 0 : p_idx].pos;
		return (p_xform.xform(b) - p_xform.xform(a)).normalized().orthogonal();
	}

	void cull(const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const;

	// Non-virtual query methods
	bool contains_point(const Vector2& p_point) const;
	bool intersect_segment(
		const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const;
	real_t get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const;

	Vector2 get_support(const Vector2& p_normal) const;
	void get_supports(const Vector2& p_normal, Vector2* r_supports, int& r_amount) const;

	template <PS2DE::ShapeType T>
	_FORCE_INLINE_ void project_range_t(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		if constexpr (T == PS2DE::SHAPE_WORLD_BOUNDARY) {
			r_min = -1e10;
			r_max = 1e10;
		}
		else if constexpr (T == PS2DE::SHAPE_SEPARATION_RAY) {
			r_max = p_normal.dot(p_transform.get_origin());
			r_min = p_normal.dot(p_transform.xform(Vector2(0, ray_length)));
			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_SEGMENT) {
			r_max = p_normal.dot(p_transform.xform(seg_a));
			r_min = p_normal.dot(p_transform.xform(seg_b));
			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CIRCLE) {
			real_t d = p_normal.dot(p_transform.get_origin());
			Vector2 local_normal = p_transform.basis_xform_inv(p_normal);
			real_t scale = local_normal.length();
			r_min = d - radius * scale;
			r_max = d + radius * scale;
		}
		else if constexpr (T == PS2DE::SHAPE_RECTANGLE) {
			r_max = -1e20;
			r_min = 1e20;
			for (int i = 0; i < 4; i++) {
				real_t d = p_normal.dot(p_transform.xform(Vector2(
					((i & 1) * 2 - 1) * half_extents.x, ((i >> 1) * 2 - 1) * half_extents.y)));

				if (d > r_max) {
					r_max = d;
				}
				if (d < r_min) {
					r_min = d;
				}
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CAPSULE) {
			Vector2 n = p_transform.basis_xform_inv(p_normal).normalized();
			real_t h = height * 0.5 - radius;

			n *= radius;
			n.y += (n.y > 0) ? h : -h;

			r_max = p_normal.dot(p_transform.xform(n));
			r_min = p_normal.dot(p_transform.xform(-n));

			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CONVEX_POLYGON) {
			if (!convex_points || convex_point_count <= 0) {
				r_min = r_max = 0;
				return;
			}

			r_min = r_max = p_normal.dot(p_transform.xform(convex_points[0].pos));
			for (int i = 1; i < convex_point_count; i++) {
				real_t d = p_normal.dot(p_transform.xform(convex_points[i].pos));
				if (d > r_max) {
					r_max = d;
				}
				if (d < r_min) {
					r_min = d;
				}
			}
		}
		else {
			r_min = 0;
			r_max = 0;
			ERR_FAIL_MSG("Unsupported call to project_range in GodotShape2D");
		}
	}

	template <PS2DE::ShapeType T>
	_FORCE_INLINE_ void project_range_cast_t(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		if constexpr (T == PS2DE::SHAPE_WORLD_BOUNDARY) {
			r_min = -1e10;
			r_max = 1e10;
		}
		else {
			real_t mina, maxa;
			real_t minb, maxb;
			Transform2D ofsb = p_transform;
			ofsb.columns[2] += p_cast;
			project_range_t<T>(p_normal, p_transform, mina, maxa);
			project_range_t<T>(p_normal, ofsb, minb, maxb);
			r_min = MIN(mina, minb);
			r_max = MAX(maxa, maxb);
		}
	}

	void project_range(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const;
	void project_range_cast(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const;

	_FORCE_INLINE_ void project_rangev(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		project_range(p_normal, p_transform, r_min, r_max);
	}

	_FORCE_INLINE_ void project_range_castv(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		project_range_cast(p_cast, p_normal, p_transform, r_min, r_max);
	}

	_FORCE_INLINE_ void get_supports_transformed_cast(const Vector2& p_cast,
		const Vector2& p_normal, const Transform2D& p_xform, Vector2* r_supports,
		int& r_amount) const
	{
		get_supports(p_xform.basis_xform_inv(p_normal).normalized(), r_supports, r_amount);
		for (int i = 0; i < r_amount; i++) {
			r_supports[i] = p_xform.xform(r_supports[i]);
		}

		if (r_amount == 1) {
			if (Math::abs(p_normal.dot(p_cast.normalized())) <
				segment_is_valid_support_threshold_lower) {
				// make line because they are parallel
				r_amount = 2;
				r_supports[1] = r_supports[0] + p_cast;
			}
			else if (p_cast.dot(p_normal) > 0) {
				// normal points towards cast, add cast
				r_supports[0] += p_cast;
			}
		}
		else {
			if (Math::abs(p_normal.dot(p_cast.normalized())) <
				segment_is_valid_support_threshold_lower) {
				// optimize line and make it larger because they are parallel
				if ((r_supports[1] - r_supports[0]).dot(p_cast) > 0) {
					// larger towards 1
					r_supports[1] += p_cast;
				}
				else {
					// larger towards 0
					r_supports[0] += p_cast;
				}
			}
			else if (p_cast.dot(p_normal) > 0) {
				// normal points towards cast, add cast
				r_supports[0] += p_cast;
				r_supports[1] += p_cast;
			}
		}
	}

	explicit GodotShape2D(PS2DE::ShapeType p_type) : type(p_type) {}

	_FORCE_INLINE_ GodotShape2D(const Vector2& p_a, const Vector2& p_b, const Vector2& p_n)
		: type(PS2DE::SHAPE_SEGMENT), seg_a(p_a), seg_b(p_b), seg_n(p_n)
	{
	}

	GodotShape2D(const GodotShape2D&) = delete;
	GodotShape2D& operator=(const GodotShape2D&) = delete;

	~GodotShape2D();
};

// --- Folded from modules/godot_physics_2d/godot_collision_solver_2d.h ---


class GodotCollisionSolver2D {
public:
	typedef void (*CallbackResult)(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_userdata);

private:
	static bool solve_static_world_boundary(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, real_t p_margin = 0);
	static bool concave_callback(void *p_userdata, GodotShape2D *p_convex);
	static bool solve_concave(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis = nullptr, real_t p_margin_A = 0, real_t p_margin_B = 0);
	static bool solve_separation_ray(const GodotShape2D *p_shape_A, const Vector2 &p_motion_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis = nullptr, real_t p_margin = 0);

public:
	static bool solve(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, Vector2 *r_sep_axis = nullptr, real_t p_margin_A = 0, real_t p_margin_B = 0);
};

// --- End modules/godot_physics_2d/godot_collision_solver_2d.h ---

// --- End modules/godot_physics_2d/godot_shape_2d.h ---


class GodotSpace2D;
class GodotArea2D;
class GodotBody2D;
class GodotConstraint2D;
class PhysicsDirectBodyState2D;

class GodotCollisionObject2D
{
public:
	enum Type
	{
		TYPE_AREA,
		TYPE_BODY
	};

private:
	Type type;
	RID self;
	bool pickable = true;

	struct Shape
	{
		Transform2D xform;
		Transform2D xform_inv;
		uint32_t bpid = 0;
		Rect2 aabb_cache; // for rayqueries
		GodotShape2D* shape = nullptr;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0;
		Vector2 one_way_collision_direction = Vector2(0.0, 1.0);
	};

	Vector<Shape> shapes;
	GodotSpace2D* space = nullptr;
	Transform2D transform;
	Transform2D inv_transform;
	uint32_t collision_mask = 1;
	uint32_t collision_layer = 1;
	real_t collision_priority = 1.0;
	bool _static = true;

	SelfList<GodotCollisionObject2D> pending_shape_update_list;

	void _update_shapes();

protected:
	void _update_shapes_with_motion(const Vector2& p_motion);
	void _unregister_shapes();

	_FORCE_INLINE_ void _set_transform(const Transform2D& p_transform, bool p_update_shapes = true)
	{
		transform = p_transform;
		if (p_update_shapes) {
			_update_shapes();
		}
	}

	_FORCE_INLINE_ void _set_inv_transform(const Transform2D& p_transform)
	{
		inv_transform = p_transform;
	}

	void _set_static(bool p_static);

	virtual void _shapes_changed() = 0;
	void _set_space(GodotSpace2D* p_space);

	GodotCollisionObject2D(Type p_type);

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	void _shape_changed();

	_FORCE_INLINE_ Type get_type() const { return type; }

	void add_shape(GodotShape2D* p_shape, const Transform2D& p_transform = Transform2D(),
		bool p_disabled = false);
	void set_shape(int p_index, GodotShape2D* p_shape);
	void set_shape_transform(int p_index, const Transform2D& p_transform);

	_FORCE_INLINE_ int get_shape_count() const { return shapes.size(); }

	_FORCE_INLINE_ GodotShape2D* get_shape(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].shape;
	}

	_FORCE_INLINE_ const Transform2D& get_shape_transform(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].xform;
	}

	_FORCE_INLINE_ const Transform2D& get_shape_inv_transform(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].xform_inv;
	}

	_FORCE_INLINE_ const Rect2& get_shape_aabb(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].aabb_cache;
	}

	_FORCE_INLINE_ const Transform2D& get_transform() const { return transform; }

	_FORCE_INLINE_ const Transform2D& get_inv_transform() const { return inv_transform; }

	_FORCE_INLINE_ GodotSpace2D* get_space() const { return space; }

	void set_shape_disabled(int p_idx, bool p_disabled);

	_FORCE_INLINE_ bool is_shape_disabled(int p_idx) const
	{
		ERR_FAIL_INDEX_V(p_idx, shapes.size(), false);
		return shapes[p_idx].disabled;
	}

	_FORCE_INLINE_ void set_shape_as_one_way_collision(
		int p_idx, bool p_one_way_collision, real_t p_margin, const Vector2& p_direction)
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		shapes.write[p_idx].one_way_collision = p_one_way_collision;
		shapes.write[p_idx].one_way_collision_margin = p_margin;
		shapes.write[p_idx].one_way_collision_direction = p_direction;
	}

	_FORCE_INLINE_ bool is_shape_set_as_one_way_collision(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision;
	}

	_FORCE_INLINE_ real_t get_shape_one_way_collision_margin(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision_margin;
	}

	Vector2 get_shape_one_way_collision_direction(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision_direction;
	}

	void set_collision_mask(uint32_t p_mask)
	{
		collision_mask = p_mask;
		_shape_changed();
	}

	_FORCE_INLINE_ uint32_t get_collision_mask() const { return collision_mask; }

	void set_collision_layer(uint32_t p_layer)
	{
		collision_layer = p_layer;
		_shape_changed();
	}

	_FORCE_INLINE_ uint32_t get_collision_layer() const { return collision_layer; }

	_FORCE_INLINE_ void set_collision_priority(real_t p_priority)
	{
		ERR_FAIL_COND_MSG(p_priority <= 0, "Priority must be greater than 0.");
		collision_priority = p_priority;
		_shape_changed();
	}

	_FORCE_INLINE_ real_t get_collision_priority() const { return collision_priority; }

	void remove_shape(GodotShape2D* p_shape);
	void remove_shape(int p_index);

	virtual void set_space(GodotSpace2D* p_space) = 0;

	_FORCE_INLINE_ bool is_static() const { return _static; }

	void set_pickable(bool p_pickable) { pickable = p_pickable; }

	_FORCE_INLINE_ bool is_pickable() const { return pickable; }

	_FORCE_INLINE_ bool collides_with(GodotCollisionObject2D* p_other) const
	{
		return p_other->collision_layer & collision_mask;
	}

	_FORCE_INLINE_ bool interacts_with(const GodotCollisionObject2D* p_other) const
	{
		return collision_layer & p_other->collision_mask ||
			   p_other->collision_layer & collision_mask;
	}

	virtual ~GodotCollisionObject2D() {}
};

class GodotArea2D final : public GodotCollisionObject2D
{
	PS2DE::AreaSpaceOverrideMode gravity_override_mode = PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	PS2DE::AreaSpaceOverrideMode linear_damping_override_mode = PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	PS2DE::AreaSpaceOverrideMode angular_damping_override_mode =
		PS2DE::AREA_SPACE_OVERRIDE_DISABLED;

	real_t gravity = 9.80665;
	Vector2 gravity_vector = Vector2(0, -1);
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0;
	real_t linear_damp = 0.1;
	real_t angular_damp = 1.0;
	int priority = 0;
	bool monitorable = false;

	SelfList<GodotArea2D> monitor_query_list;
	SelfList<GodotArea2D> moved_list;

	struct BodyKey
	{
		RID rid;
		uint32_t body_shape = 0;
		uint32_t area_shape = 0;

		static uint32_t hash(const BodyKey& p_key)
		{
			uint32_t h = hash_one_uint64(p_key.rid.get_id());
			h = hash_murmur3_one_32(p_key.area_shape, h);
			return hash_fmix32(hash_murmur3_one_32(p_key.body_shape, h));
		}

		_FORCE_INLINE_ bool operator==(const BodyKey& p_key) const
		{
			return rid == p_key.rid && body_shape == p_key.body_shape &&
				   area_shape == p_key.area_shape;
		}

		_FORCE_INLINE_ BodyKey() {}

		BodyKey(GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
		BodyKey(GodotArea2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
	};

	struct BodyState
	{
		int state = 0;

		_FORCE_INLINE_ void inc() { state++; }

		_FORCE_INLINE_ void dec() { state--; }
	};

	HashMap<BodyKey, BodyState, BodyKey> monitored_bodies;
	HashMap<BodyKey, BodyState, BodyKey> monitored_areas;

	HashSet<GodotConstraint2D*> constraints;

	virtual void _shapes_changed() override;
	void _queue_monitor_update();

	void _set_space_override_mode(
		PS2DE::AreaSpaceOverrideMode& r_mode, PS2DE::AreaSpaceOverrideMode p_new_mode);

public:
	_FORCE_INLINE_ void add_body_to_query(
		GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
	_FORCE_INLINE_ void remove_body_from_query(
		GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);

	_FORCE_INLINE_ void add_area_to_query(
		GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape);
	_FORCE_INLINE_ void remove_area_from_query(
		GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape);

	_FORCE_INLINE_ void set_gravity(real_t p_gravity) { gravity = p_gravity; }

	_FORCE_INLINE_ real_t get_gravity() const { return gravity; }

	_FORCE_INLINE_ void set_gravity_vector(const Vector2& p_gravity) { gravity_vector = p_gravity; }

	_FORCE_INLINE_ Vector2 get_gravity_vector() const { return gravity_vector; }

	_FORCE_INLINE_ void set_gravity_as_point(bool p_enable) { gravity_is_point = p_enable; }

	_FORCE_INLINE_ bool is_gravity_point() const { return gravity_is_point; }

	_FORCE_INLINE_ void set_gravity_point_unit_distance(real_t scale)
	{
		gravity_point_unit_distance = scale;
	}

	_FORCE_INLINE_ real_t get_gravity_point_unit_distance() const
	{
		return gravity_point_unit_distance;
	}

	_FORCE_INLINE_ void set_linear_damp(real_t p_linear_damp) { linear_damp = p_linear_damp; }

	_FORCE_INLINE_ real_t get_linear_damp() const { return linear_damp; }

	_FORCE_INLINE_ void set_angular_damp(real_t p_angular_damp) { angular_damp = p_angular_damp; }

	_FORCE_INLINE_ real_t get_angular_damp() const { return angular_damp; }

	_FORCE_INLINE_ void set_priority(int p_priority) { priority = p_priority; }

	_FORCE_INLINE_ int get_priority() const { return priority; }

	_FORCE_INLINE_ void add_constraint(GodotConstraint2D* p_constraint)
	{
		constraints.insert(p_constraint);
	}

	_FORCE_INLINE_ void remove_constraint(GodotConstraint2D* p_constraint)
	{
		constraints.erase(p_constraint);
	}

	_FORCE_INLINE_ const HashSet<GodotConstraint2D*>& get_constraints() const
	{
		return constraints;
	}

	_FORCE_INLINE_ void clear_constraints() { constraints.clear(); }

	void set_monitorable(bool p_monitorable);

	_FORCE_INLINE_ bool is_monitorable() const { return monitorable; }

	void set_transform(const Transform2D& p_transform);

	void set_space(GodotSpace2D* p_space) override;

	void call_queries();

	void compute_gravity(const Vector2& p_position, Vector2& r_gravity) const;

	GodotArea2D();
	~GodotArea2D();
};

void GodotArea2D::add_body_to_query(
	GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	BodyKey bk(p_body, p_body_shape, p_area_shape);
	monitored_bodies[bk].inc();
	if (!monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::remove_body_from_query(
	GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	BodyKey bk(p_body, p_body_shape, p_area_shape);
	monitored_bodies[bk].dec();
	if (get_space() && !monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::add_area_to_query(
	GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape)
{
	BodyKey bk(p_area, p_area_shape, p_self_shape);
	monitored_areas[bk].inc();
	if (!monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::remove_area_from_query(
	GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape)
{
	BodyKey bk(p_area, p_area_shape, p_self_shape);
	monitored_areas[bk].dec();
	if (get_space() && !monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

class GodotBody2D final : public GodotCollisionObject2D
{
	PS2DE::BodyMode mode = PS2DE::BODY_MODE_RIGID;

	Vector2 biased_linear_velocity;
	real_t biased_angular_velocity = 0.0;

	Vector2 linear_velocity;
	real_t angular_velocity = 0.0;

	Vector2 prev_linear_velocity;
	real_t prev_angular_velocity = 0.0;

	Vector2 constant_linear_velocity;
	real_t constant_angular_velocity = 0.0;

	PS2DE::BodyDampMode linear_damp_mode = PS2DE::BODY_DAMP_MODE_COMBINE;
	PS2DE::BodyDampMode angular_damp_mode = PS2DE::BODY_DAMP_MODE_COMBINE;

	real_t linear_damp = 0.0;
	real_t angular_damp = 0.0;

	real_t total_linear_damp = 0.0;
	real_t total_angular_damp = 0.0;

	real_t gravity_scale = 1.0;

	real_t bounce = 0.0;
	real_t friction = 1.0;

	real_t mass = 1.0;
	real_t _inv_mass = 1.0;

	real_t inertia = 0.0;
	real_t _inv_inertia = 0.0;

	Vector2 center_of_mass_local;
	Vector2 center_of_mass;

	bool calculate_inertia = true;
	bool calculate_center_of_mass = true;

	Vector2 gravity;

	real_t still_time = 0.0;

	Vector2 applied_force;
	real_t applied_torque = 0.0;

	Vector2 constant_force;
	real_t constant_torque = 0.0;

	SelfList<GodotBody2D> active_list;
	SelfList<GodotBody2D> mass_properties_update_list;
	SelfList<GodotBody2D> direct_state_query_list;

	VSet<RID> exceptions;
	PS2DE::CCDMode continuous_cd_mode = PS2DE::CCD_MODE_DISABLED;
	bool omit_force_integration = false;
	bool active = true;
	bool can_sleep = true;
	bool first_time_kinematic = false;
	void _mass_properties_changed();
	virtual void _shapes_changed() override;
	Transform2D new_transform;

	List<Pair<GodotConstraint2D*, int>> constraint_list;

	struct AreaCMP
	{
		GodotArea2D* area = nullptr;
		int refCount = 0;

		_FORCE_INLINE_ bool operator==(const AreaCMP& p_cmp) const
		{
			return area->get_self() == p_cmp.area->get_self();
		}

		_FORCE_INLINE_ bool operator<(const AreaCMP& p_cmp) const
		{
			return area->get_priority() < p_cmp.area->get_priority();
		}

		_FORCE_INLINE_ AreaCMP() {}

		_FORCE_INLINE_ AreaCMP(GodotArea2D* p_area)
		{
			area = p_area;
			refCount = 1;
		}
	};

	Vector<AreaCMP> areas;

	struct Contact
	{
		Vector2 local_pos;
		Vector2 local_normal;
		Vector2 local_velocity_at_pos;
		real_t depth = 0.0;
		int local_shape = 0;
		Vector2 collider_pos;
		int collider_shape = 0;
		RID collider;
		Vector2 collider_velocity_at_pos;
		Vector2 impulse;
	};

	Vector<Contact> contacts; // no contacts by default
	int contact_count = 0;

	PhysicsDirectBodyState2D* direct_state = nullptr;

	uint64_t island_step = 0;

	void _update_transform_dependent();

	friend class PhysicsDirectBodyState2D;

public:
	void add_contact(const Vector2& p_local_pos, const Vector2& p_local_normal, real_t p_depth,
		int p_local_shape, const Vector2& p_local_velocity_at_pos, const Vector2& p_collider_pos,
		int p_collider_shape, const RID& p_collider, const Vector2& p_collider_velocity_at_pos,
		const Vector2& p_impulse);

	PhysicsDirectBodyState2D* get_direct_state();

	_FORCE_INLINE_ void add_area(GodotArea2D* p_area)
	{
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount += 1;
		}
		else {
			areas.ordered_insert(AreaCMP(p_area));
		}
	}

	_FORCE_INLINE_ void remove_area(GodotArea2D* p_area)
	{
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount -= 1;
			if (areas[index].refCount < 1) {
				areas.remove_at(index);
			}
		}
	}

	_FORCE_INLINE_ void set_max_contacts_reported(int p_size)
	{
		ERR_FAIL_INDEX(p_size, PS2DC::MAX_CONTACTS_REPORTED_2D_MAX);
		contacts.resize(p_size);
		contact_count = 0;
		if (mode == PS2DE::BODY_MODE_KINEMATIC && p_size) {
			set_active(true);
		}
	}

	_FORCE_INLINE_ int get_max_contacts_reported() const { return contacts.size(); }

	_FORCE_INLINE_ bool can_report_contacts() const { return !contacts.is_empty(); }

	_FORCE_INLINE_ void add_exception(const RID& p_exception) { exceptions.insert(p_exception); }

	_FORCE_INLINE_ void remove_exception(const RID& p_exception) { exceptions.erase(p_exception); }

	_FORCE_INLINE_ bool has_exception(const RID& p_exception) const
	{
		return exceptions.has(p_exception);
	}

	_FORCE_INLINE_ const VSet<RID>& get_exceptions() const { return exceptions; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }

	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ void add_constraint(GodotConstraint2D* p_constraint, int p_pos)
	{
		constraint_list.push_back({p_constraint, p_pos});
	}

	_FORCE_INLINE_ void remove_constraint(GodotConstraint2D* p_constraint, int p_pos)
	{
		constraint_list.erase({p_constraint, p_pos});
	}

	const List<Pair<GodotConstraint2D*, int>>& get_constraint_list() const
	{
		return constraint_list;
	}

	_FORCE_INLINE_ void clear_constraint_list() { constraint_list.clear(); }

	_FORCE_INLINE_ void set_omit_force_integration(bool p_omit_force_integration)
	{
		omit_force_integration = p_omit_force_integration;
	}

	_FORCE_INLINE_ bool get_omit_force_integration() const { return omit_force_integration; }

	_FORCE_INLINE_ void set_linear_velocity(const Vector2& p_velocity)
	{
		linear_velocity = p_velocity;
	}

	_FORCE_INLINE_ Vector2 get_linear_velocity() const { return linear_velocity; }

	_FORCE_INLINE_ void set_angular_velocity(real_t p_velocity) { angular_velocity = p_velocity; }

	_FORCE_INLINE_ real_t get_angular_velocity() const { return angular_velocity; }

	_FORCE_INLINE_ Vector2 get_prev_linear_velocity() const { return prev_linear_velocity; }

	_FORCE_INLINE_ real_t get_prev_angular_velocity() const { return prev_angular_velocity; }

	_FORCE_INLINE_ void set_biased_linear_velocity(const Vector2& p_velocity)
	{
		biased_linear_velocity = p_velocity;
	}

	_FORCE_INLINE_ Vector2 get_biased_linear_velocity() const { return biased_linear_velocity; }

	_FORCE_INLINE_ void set_biased_angular_velocity(real_t p_velocity)
	{
		biased_angular_velocity = p_velocity;
	}

	_FORCE_INLINE_ real_t get_biased_angular_velocity() const { return biased_angular_velocity; }

	_FORCE_INLINE_ void apply_central_impulse(const Vector2& p_impulse)
	{
		linear_velocity += p_impulse * _inv_mass;
	}

	_FORCE_INLINE_ void apply_impulse(
		const Vector2& p_impulse, const Vector2& p_position = Vector2())
	{
		linear_velocity += p_impulse * _inv_mass;
		angular_velocity += _inv_inertia * (p_position - center_of_mass).cross(p_impulse);
	}

	_FORCE_INLINE_ void apply_torque_impulse(real_t p_torque)
	{
		angular_velocity += _inv_inertia * p_torque;
	}

	_FORCE_INLINE_ void apply_bias_impulse(const Vector2& p_impulse,
		const Vector2& p_position = Vector2(), real_t p_max_delta_av = -1.0)
	{
		biased_linear_velocity += p_impulse * _inv_mass;
		if (p_max_delta_av != 0.0) {
			real_t delta_av = _inv_inertia * (p_position - center_of_mass).cross(p_impulse);
			if (p_max_delta_av > 0 && delta_av > p_max_delta_av) {
				delta_av = p_max_delta_av;
			}
			biased_angular_velocity += delta_av;
		}
	}

	_FORCE_INLINE_ void apply_central_force(const Vector2& p_force) { applied_force += p_force; }

	_FORCE_INLINE_ void apply_force(const Vector2& p_force, const Vector2& p_position = Vector2())
	{
		applied_force += p_force;
		applied_torque += (p_position - center_of_mass).cross(p_force);
	}

	_FORCE_INLINE_ void apply_torque(real_t p_torque) { applied_torque += p_torque; }

	_FORCE_INLINE_ void add_constant_central_force(const Vector2& p_force)
	{
		constant_force += p_force;
	}

	_FORCE_INLINE_ void add_constant_force(
		const Vector2& p_force, const Vector2& p_position = Vector2())
	{
		constant_force += p_force;
		constant_torque += (p_position - center_of_mass).cross(p_force);
	}

	_FORCE_INLINE_ void add_constant_torque(real_t p_torque) { constant_torque += p_torque; }

	void set_constant_force(const Vector2& p_force) { constant_force = p_force; }

	Vector2 get_constant_force() const { return constant_force; }

	void set_constant_torque(real_t p_torque) { constant_torque = p_torque; }

	real_t get_constant_torque() const { return constant_torque; }

	void set_active(bool p_active);

	_FORCE_INLINE_ bool is_active() const { return active; }

	_FORCE_INLINE_ void wakeup()
	{
		if ((!get_space()) || mode == PS2DE::BODY_MODE_STATIC ||
			mode == PS2DE::BODY_MODE_KINEMATIC) {
			return;
		}
		set_active(true);
	}

	void set_mode(PS2DE::BodyMode p_mode);
	PS2DE::BodyMode get_mode() const;

	_FORCE_INLINE_ void set_continuous_collision_detection_mode(PS2DE::CCDMode p_mode)
	{
		continuous_cd_mode = p_mode;
	}

	_FORCE_INLINE_ PS2DE::CCDMode get_continuous_collision_detection_mode() const
	{
		return continuous_cd_mode;
	}

	void set_space(GodotSpace2D* p_space) override;

	void update_mass_properties();
	void reset_mass_properties();

	_FORCE_INLINE_ const Vector2& get_center_of_mass() const { return center_of_mass; }

	_FORCE_INLINE_ const Vector2& get_center_of_mass_local() const { return center_of_mass_local; }

	_FORCE_INLINE_ real_t get_inv_mass() const { return _inv_mass; }

	_FORCE_INLINE_ real_t get_inv_inertia() const { return _inv_inertia; }

	_FORCE_INLINE_ real_t get_friction() const { return friction; }

	_FORCE_INLINE_ real_t get_bounce() const { return bounce; }

	void integrate_forces(real_t p_step);
	void integrate_velocities(real_t p_step);

	_FORCE_INLINE_ Vector2 get_velocity_in_local_point(const Vector2& rel_pos) const
	{
		return linear_velocity + Vector2(-angular_velocity * (rel_pos.y - center_of_mass.y),
									 angular_velocity * (rel_pos.x - center_of_mass.x));
	}

	_FORCE_INLINE_ Vector2 get_motion() const
	{
		if (mode > PS2DE::BODY_MODE_KINEMATIC) {
			return new_transform.get_origin() - get_transform().get_origin();
		}
		else if (mode == PS2DE::BODY_MODE_KINEMATIC) {
			return get_transform().get_origin() -
				   new_transform.get_origin(); // kinematic simulates forward
		}
		return Vector2();
	}

	void call_queries();
	void wakeup_neighbours();

	bool sleep_test(real_t p_step);

	GodotBody2D();
	~GodotBody2D();
};

_FORCE_INLINE_ void GodotBody2D::add_contact(const Vector2& p_local_pos, const Vector2& p_local_normal,
	real_t p_depth, int p_local_shape, const Vector2& p_local_velocity_at_pos,
	const Vector2& p_collider_pos, int p_collider_shape, const RID& p_collider,
	const Vector2& p_collider_velocity_at_pos, const Vector2& p_impulse)
{
	int c_max = contacts.size();

	if (c_max == 0) {
		return;
	}

	Contact* c = contacts.ptrw();

	int idx = -1;

	if (contact_count < c_max) {
		idx = contact_count++;
	}
	else {
		real_t least_depth = 1e20;
		int least_deep = -1;
		for (int i = 0; i < c_max; i++) {
			if (i == 0 || c[i].depth < least_depth) {
				least_deep = i;
				least_depth = c[i].depth;
			}
		}

		if (least_deep >= 0 && least_depth < p_depth) {
			idx = least_deep;
		}
		if (idx == -1) {
			return; // none least deepe than this
		}
	}

	c[idx].local_pos = p_local_pos;
	c[idx].local_normal = p_local_normal;
	c[idx].local_velocity_at_pos = p_local_velocity_at_pos;
	c[idx].depth = p_depth;
	c[idx].local_shape = p_local_shape;
	c[idx].collider_pos = p_collider_pos;
	c[idx].collider_shape = p_collider_shape;
	c[idx].collider = p_collider;
	c[idx].collider_velocity_at_pos = p_collider_velocity_at_pos;
	c[idx].impulse = p_impulse;
}

// --- End modules/godot_physics_2d/godot_objects_2d.h ---

class GodotConstraint2D {
	GodotBody2D **_body_ptr;
	int _body_count;
	uint64_t island_step = 0;
	bool disabled_collisions_between_bodies = true;

	RID self;

protected:
	GodotConstraint2D(GodotBody2D **p_body_ptr = nullptr, int p_body_count = 0) {
		_body_ptr = p_body_ptr;
		_body_count = p_body_count;
	}

public:
	_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }
	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ GodotBody2D **get_body_ptr() const { return _body_ptr; }
	_FORCE_INLINE_ int get_body_count() const { return _body_count; }

	_FORCE_INLINE_ void disable_collisions_between_bodies(const bool p_disabled) { disabled_collisions_between_bodies = p_disabled; }
	_FORCE_INLINE_ bool is_disabled_collisions_between_bodies() const { return disabled_collisions_between_bodies; }

	virtual bool setup(real_t p_step) = 0;
	virtual bool pre_solve(real_t p_step) = 0;
	virtual void solve(real_t p_step) = 0;

	virtual ~GodotConstraint2D() {}
};

class GodotBodyPair2D final : public GodotConstraint2D {
	enum {
		MAX_CONTACTS = 2
	};
	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	int shape_A = 0;
	int shape_B = 0;

	bool collide_A = false;
	bool collide_B = false;

	GodotSpace2D *space = nullptr;

	struct Contact {
		Vector2 position;
		Vector2 normal;
		Vector2 local_A, local_B;
		Vector2 acc_impulse; // accumulated impulse
		real_t acc_normal_impulse = 0.0; // accumulated normal impulse (Pn)
		real_t acc_tangent_impulse = 0.0; // accumulated tangent impulse (Pt)
		real_t acc_bias_impulse = 0.0; // accumulated normal impulse for position bias (Pnb)
		real_t acc_bias_impulse_center_of_mass = 0.0; // accumulated normal impulse for position bias applied to com
		real_t mass_normal = 0.0, mass_tangent = 0.0;
		real_t bias = 0.0;

		real_t depth = 0.0;
		bool active = false;
		bool used = false;
		Vector2 rA, rB;
		real_t bounce = 0.0;
	};

	Vector2 offset_B; //use local A coordinates to avoid numerical issues on collision detection

	Vector2 sep_axis;
	Contact contacts[MAX_CONTACTS];
	int contact_count = 0;
	bool collided = false;
	bool check_ccd = false;
	bool oneway_disabled = false;
	bool report_contacts_only = false;

	bool _test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B);
	void _validate_contacts();
	static void _add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self);
	_FORCE_INLINE_ void _contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B);

public:
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B);
	virtual ~GodotBodyPair2D() override;
};

class GodotAreaPair2D final : public GodotConstraint2D {
	GodotBody2D *body = nullptr;
	GodotArea2D *area = nullptr;
	int body_shape = 0;
	int area_shape = 0;
	bool colliding = false;
	bool has_space_override = false;
	bool process_collision = false;
	bool body_has_attached_area = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape);
	virtual ~GodotAreaPair2D() override = default;
};

class GodotArea2Pair2D final : public GodotConstraint2D {
	GodotArea2D *area_a = nullptr;
	GodotArea2D *area_b = nullptr;
	int shape_a = 0;
	int shape_b = 0;
	bool colliding_a = false;
	bool colliding_b = false;
	bool process_collision_a = false;
	bool process_collision_b = false;
	bool area_a_monitorable = false;
	bool area_b_monitorable = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b);
	virtual ~GodotArea2Pair2D() override = default;
};

class GodotJoint2D final : public GodotConstraint2D {
	PS2DE::JointType type = PS2DE::JOINT_TYPE_MAX;

	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	real_t bias = 0.0;
	real_t max_bias = 3.40282e+38;
	real_t max_force = 3.40282e+38;

	bool dynamic_A = false;
	bool dynamic_B = false;

	// Shared working fields
	Vector2 rA, rB;
	Vector2 anchor_A;
	Vector2 anchor_B;
	real_t jn_max = 0.0;

	// Pin joint fields
	Transform2D M;
	Vector2 pin_bias;
	real_t initial_angle = 0.0;
	real_t bias_velocity = 0.0;
	real_t j_acc = 0.0;
	real_t i_sum = 0.0;
	Vector2 P;
	real_t softness = 0.0;
	real_t angular_limit_lower = 0.0;
	real_t angular_limit_upper = 0.0;
	real_t motor_target_velocity = 0.0;
	bool is_joint_at_limit = false;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove joint fields
	Vector2 A_groove_1;
	Vector2 A_groove_2;
	Vector2 A_groove_normal;
	Vector2 B_anchor;
	Vector2 jn_acc;
	Vector2 gbias;
	real_t clamp = 0.0;
	Vector2 xf_normal;
	Vector2 k1, k2;
	bool correct = false;

	// Damped spring joint fields
	real_t rest_length = 0.0;
	real_t damping = 1.5;
	real_t stiffness = 20.0;
	Vector2 n;
	Vector2 j;
	real_t n_mass = 0.0;
	real_t target_vrn = 0.0;
	real_t v_coef = 0.0;

	bool _setup_pin(real_t p_step);
	bool _pre_solve_pin(real_t p_step);
	void _solve_pin(real_t p_step);

	bool _setup_groove(real_t p_step);
	bool _pre_solve_groove(real_t p_step);
	void _solve_groove(real_t p_step);

	bool _setup_damped_spring(real_t p_step);
	bool _pre_solve_damped_spring(real_t p_step);
	void _solve_damped_spring(real_t p_step);

public:
	_FORCE_INLINE_ PS2DE::JointType get_type() const { return type; }

	_FORCE_INLINE_ void set_max_force(real_t p_force) { max_force = p_force; }
	_FORCE_INLINE_ real_t get_max_force() const { return max_force; }

	_FORCE_INLINE_ void set_bias(real_t p_bias) { bias = p_bias; }
	_FORCE_INLINE_ real_t get_bias() const { return bias; }

	_FORCE_INLINE_ void set_max_bias(real_t p_bias) { max_bias = p_bias; }
	_FORCE_INLINE_ real_t get_max_bias() const { return max_bias; }

	void copy_settings_from(GodotJoint2D *p_joint);

	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	// Pin joint parameters & flags
	void set_param(PS2DE::PinJointParam p_param, real_t p_value);
	real_t get_param(PS2DE::PinJointParam p_param) const;
	void set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool get_flag(PS2DE::PinJointFlag p_flag) const;

	// Damped spring parameters
	void set_param(PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t get_param(PS2DE::DampedSpringParam p_param) const;

	// Empty joint (JOINT_TYPE_MAX)
	GodotJoint2D() :
			GodotConstraint2D(_arr, 0) {}

	// Pin joint (JOINT_TYPE_PIN)
	GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b = nullptr);

	// Groove joint (JOINT_TYPE_GROOVE)
	GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	// Damped spring joint (JOINT_TYPE_DAMPED_SPRING)
	GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	GodotJoint2D(const GodotJoint2D &) = delete;
	GodotJoint2D &operator=(const GodotJoint2D &) = delete;

	virtual ~GodotJoint2D() override {
		for (int i = 0; i < get_body_count(); i++) {
			GodotBody2D *body = get_body_ptr()[i];
			if (body) {
				body->remove_constraint(this, i);
			}
		}
	}
};

// --- End modules/godot_physics_2d/godot_constraints_2d.h ---

// --- Folded from modules/godot_physics_2d/godot_space_2d.h ---
// --- Folded from core/math/bvh.h ---
// BVH
// This class provides a wrapper around BVH tree, which contains most of the functionality
// for a dynamic BVH with templated leaf size.
// However BVH also adds facilities for pairing, to maintain compatibility with Godot 3.2.
// Pairing is a collision pairing system, on top of the basic BVH.

// Some notes on the use of BVH / Octree from Godot 3.2.
// This is not well explained elsewhere.
// The rendering tree mask and types that are sent to the BVH are NOT layer masks.
// They are INSTANCE_TYPES (defined in visual_server.h), e.g. MESH, MULTIMESH, PARTICLES etc.
// Thus the lights do no cull by layer mask in the BVH.

// Layer masks are implemented in the renderers as a later step, and light_cull_mask appears to be
// implemented in GLES3 but not GLES2. Layer masks are not yet implemented for directional lights.

// In the physics, the pairable_type is based on 1 << p_object->get_type() where:
// TYPE_AREA,
// TYPE_BODY
// and pairable_mask is either 0 if static, or set to all if non static
// --- Folded from core/math/bvh_tree.h ---
// BVH Tree
// This is an implementation of a dynamic BVH with templated leaf size.
// This differs from most dynamic BVH in that it can handle more than 1 object
// in leaf nodes. This can make it far more efficient in certain circumstances.
// It also means that the splitting logic etc have to be completely different
// to a simpler tree.
// Note that MAX_CHILDREN should be fixed at 2 for now.

#include "core/types.h"
// --- Folded from core/math/bvh_abb.h ---
#include "core/types.h"

#include <cfloat> // FLT_MAX

// special optimized version of axis aligned bounding box
template <typename BOUNDS = AABB, typename POINT = Vector3>
struct BVH_ABB {
	struct ConvexHull {
		// convex hulls (optional)
		const Plane *planes;
		int num_planes;
		const Vector3 *points;
		int num_points;
	};

	struct Segment {
		POINT from;
		POINT to;
	};

	enum IntersectResult {
		IR_MISS = 0,
		IR_PARTIAL,
		IR_FULL,
	};

	// we store mins with a negative value in order to test them with SIMD
	POINT min;
	POINT neg_max;

	bool operator==(const BVH_ABB &p_other) const { return (min == p_other.min) && (neg_max == p_other.neg_max); }
	bool operator!=(const BVH_ABB &p_other) const { return (*this == p_other) == false; }

	void set(const POINT &p_min, const POINT &p_max) {
		min = p_min;
		neg_max = -p_max;
	}

	// to and from standard AABB
	void from(const BOUNDS &p_aabb) {
		min = p_aabb.position;
		neg_max = -(p_aabb.position + p_aabb.size);
	}

	void to(BOUNDS &r_aabb) const {
		r_aabb.position = min;
		r_aabb.size = calculate_size();
	}

	void merge(const BVH_ABB &p_o) {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			neg_max[axis] = MIN(neg_max[axis], p_o.neg_max[axis]);
			min[axis] = MIN(min[axis], p_o.min[axis]);
		}
	}

	POINT calculate_size() const {
		return -neg_max - min;
	}

	POINT calculate_center() const {
		return POINT((calculate_size() * 0.5) + min);
	}

	real_t get_proximity_to(const BVH_ABB &p_b) const {
		const POINT d = (min - neg_max) - (p_b.min - p_b.neg_max);
		real_t proximity = 0.0;
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			proximity += Math::abs(d[axis]);
		}
		return proximity;
	}

	int select_by_proximity(const BVH_ABB &p_a, const BVH_ABB &p_b) const {
		return (get_proximity_to(p_a) < get_proximity_to(p_b) ? 0 : 1);
	}

	uint32_t find_cutting_planes(const typename BVH_ABB::ConvexHull &p_hull, uint32_t *p_plane_ids) const {
		uint32_t count = 0;

		for (int n = 0; n < p_hull.num_planes; n++) {
			const Plane &p = p_hull.planes[n];
			if (intersects_plane(p)) {
				p_plane_ids[count++] = n;
			}
		}

		return count;
	}

	bool intersects_plane(const Plane &p_p) const {
		Vector3 size = calculate_size();
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = min + half_extents;

		// forward side of plane?
		Vector3 point_offset(
				(p_p.normal.x < 0) ? -half_extents.x : half_extents.x,
				(p_p.normal.y < 0) ? -half_extents.y : half_extents.y,
				(p_p.normal.z < 0) ? -half_extents.z : half_extents.z);
		Vector3 point = point_offset + ofs;

		if (!p_p.is_point_over(point)) {
			return false;
		}

		point = -point_offset + ofs;
		if (p_p.is_point_over(point)) {
			return false;
		}

		return true;
	}

	bool intersects_convex_optimized(const ConvexHull &p_hull, const uint32_t *p_plane_ids, uint32_t p_num_planes) const {
		Vector3 size = calculate_size();
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = min + half_extents;

		for (unsigned int i = 0; i < p_num_planes; i++) {
			const Plane &p = p_hull.planes[p_plane_ids[i]];
			Vector3 point(
					(p.normal.x > 0) ? -half_extents.x : half_extents.x,
					(p.normal.y > 0) ? -half_extents.y : half_extents.y,
					(p.normal.z > 0) ? -half_extents.z : half_extents.z);
			point += ofs;
			if (p.is_point_over(point)) {
				return false;
			}
		}

		return true;
	}

	bool intersects_convex_partial(const ConvexHull &p_hull) const {
		BOUNDS bb;
		to(bb);
		return bb.intersects_convex_shape(p_hull.planes, p_hull.num_planes, p_hull.points, p_hull.num_points);
	}

	IntersectResult intersects_convex(const ConvexHull &p_hull) const {
		if (intersects_convex_partial(p_hull)) {
			// fully within? very important for tree checks
			if (is_within_convex(p_hull)) {
				return IR_FULL;
			}

			return IR_PARTIAL;
		}

		return IR_MISS;
	}

	bool is_within_convex(const ConvexHull &p_hull) const {
		// use half extents routine
		BOUNDS bb;
		to(bb);
		return bb.inside_convex_shape(p_hull.planes, p_hull.num_planes);
	}

	bool is_point_within_hull(const ConvexHull &p_hull, const Vector3 &p_pt) const {
		for (int n = 0; n < p_hull.num_planes; n++) {
			if (p_hull.planes[n].distance_to(p_pt) > 0.0f) {
				return false;
			}
		}
		return true;
	}

	bool intersects_segment(const Segment &p_s) const {
		BOUNDS bb;
		to(bb);
		return bb.intersects_segment(p_s.from, p_s.to);
	}

	bool intersects_point(const POINT &p_pt) const {
		if (_any_lessthan(-p_pt, neg_max)) {
			return false;
		}
		if (_any_lessthan(p_pt, min)) {
			return false;
		}
		return true;
	}

	// Very hot in profiling, make sure optimized
	bool intersects(const BVH_ABB &p_o) const {
		if (_any_morethan(p_o.min, -neg_max)) {
			return false;
		}
		if (_any_morethan(min, -p_o.neg_max)) {
			return false;
		}
		return true;
	}

	// for pre-swizzled tester (this object)
	bool intersects_swizzled(const BVH_ABB &p_o) const {
		if (_any_lessthan(min, p_o.min)) {
			return false;
		}
		if (_any_lessthan(neg_max, p_o.neg_max)) {
			return false;
		}
		return true;
	}

	bool is_other_within(const BVH_ABB &p_o) const {
		if (_any_lessthan(p_o.neg_max, neg_max)) {
			return false;
		}
		if (_any_lessthan(p_o.min, min)) {
			return false;
		}
		return true;
	}

	void grow(const POINT &p_change) {
		neg_max -= p_change;
		min -= p_change;
	}

	void expand(real_t p_change) {
		POINT change;
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			change[axis] = p_change;
		}
		grow(change);
	}

	// Actually surface area metric.
	real_t get_area() const {
		POINT d = calculate_size();
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	void set_to_max_opposite_extents() {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			neg_max[axis] = FLT_MAX;
		}
		min = neg_max;
	}

	bool _any_morethan(const POINT &p_a, const POINT &p_b) const {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			if (p_a[axis] > p_b[axis]) {
				return true;
			}
		}
		return false;
	}

	bool _any_lessthan(const POINT &p_a, const POINT &p_b) const {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			if (p_a[axis] < p_b[axis]) {
				return true;
			}
		}
		return false;
	}
};

// --- End core/math/bvh_abb.h ---
#include "core/math/plane.h"
#include "core/templates/local_vector.h"

#define BVHABB_CLASS BVH_ABB<BOUNDS, POINT>

// not sure if this is better yet so making optional
#define BVH_EXPAND_LEAF_AABBS

// never do these checks in release
#ifdef DEV_ENABLED
//#define BVH_VERBOSE
//#define BVH_VERBOSE_TREE
//#define BVH_VERBOSE_PAIRING
//#define BVH_VERBOSE_MOVES

//#define BVH_VERBOSE_FRAME
//#define BVH_CHECKS
//#define BVH_INTEGRITY_CHECKS
#endif

// debug only assert
#ifdef BVH_CHECKS
#define BVH_ASSERT(a) CRASH_COND((a) == false)
#else
#define BVH_ASSERT(a)
#endif

#ifdef BVH_VERBOSE
#define VERBOSE_PRINT print_line
#else
#define VERBOSE_PRINT(a)
#endif

// really just a namespace
struct BVHCommon {
	// these could possibly also be the same constant,
	// although this may be useful for debugging.
	// or use zero for invalid and +1 based indices.
	static const uint32_t INVALID = (0xffffffff);
	static const uint32_t INACTIVE = (0xfffffffe);
};

// really a handle, can be anything
// note that zero is a valid reference for the BVH .. this may involve using
// a plus one based ID for clients that expect 0 to be invalid.
struct BVHHandle {
	// conversion operator
	operator uint32_t() const { return _data; }
	void set(uint32_t p_value) { _data = p_value; }

	uint32_t _data;

	void set_invalid() { _data = BVHCommon::INVALID; }
	bool is_invalid() const { return _data == BVHCommon::INVALID; }
	uint32_t id() const { return _data; }
	void set_id(uint32_t p_id) { _data = p_id; }

	bool operator==(const BVHHandle &p_h) const { return _data == p_h._data; }
	bool operator!=(const BVHHandle &p_h) const { return (*this == p_h) == false; }
};

// helper class to make iterative versions of recursive functions
template <typename T>
class BVH_IterativeInfo {
public:
	constexpr static const size_t ALLOCA_STACK_SIZE = 128;

	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;
	T *stack;
	//only used in rare occasions when you run out of alloca memory
	// because tree is too unbalanced.
	LocalVector<T> aux_stack;
	int32_t get_alloca_stacksize() const { return ALLOCA_STACK_SIZE * sizeof(T); }

	T *get_first() const {
		return &stack[0];
	}

	// pop the last member of the stack, or return false
	bool pop(T &r_value) {
		if (!depth) {
			return false;
		}

		depth--;
		r_value = stack[depth];
		return true;
	}

	// request new addition to stack
	T *request() {
		if (depth > threshold) {
			if (aux_stack.is_empty()) {
				aux_stack.resize(ALLOCA_STACK_SIZE * 2);
				memcpy(aux_stack.ptr(), stack, get_alloca_stacksize());
			} else {
				aux_stack.resize(aux_stack.size() * 2);
			}
			stack = aux_stack.ptr();
			threshold = aux_stack.size() - 2;
		}
		return &stack[depth++];
	}
};

template <typename T>
class BVH_DummyPairTestFunction {
public:
	static bool user_collision_check(T *p_a, T *p_b) {
		// return false if no collision, decided by masks etc
		return true;
	}
};

template <typename T>
class BVH_DummyCullTestFunction {
public:
	static bool user_cull_check(T *p_a, T *p_b) {
		// return false if no collision
		return true;
	}
};

template <typename T, int NUM_TREES, int MAX_CHILDREN, int MAX_ITEMS, typename USER_PAIR_TEST_FUNCTION = BVH_DummyPairTestFunction<T>, typename USER_CULL_TEST_FUNCTION = BVH_DummyCullTestFunction<T>, bool USE_PAIRS = false, typename BOUNDS = AABB, typename POINT = Vector3>
class BVH_Tree {
	friend class BVH;

#include "core/math/bvh_pair.inc"
#include "core/math/bvh_structs.inc"

public:
	BVH_Tree() {
		for (int n = 0; n < NUM_TREES; n++) {
			_root_node_id[n] = BVHCommon::INVALID;
		}

		// disallow zero leaf ids
		// (as these ids are stored as negative numbers in the node)
		uint32_t dummy_leaf_id;
		_leaves.request(dummy_leaf_id);

		// In many cases you may want to change this default in the client code,
		// or expose this value to the user.
		// This default may make sense for a typically scaled 3d game, but maybe not for 2d on a pixel scale.
		params_set_pairing_expansion(0.1);
	}

private:
	bool node_add_child(uint32_t p_node_id, uint32_t p_child_node_id) {
		TNode &tnode = _nodes[p_node_id];
		if (tnode.is_full_of_children()) {
			return false;
		}

		tnode.children[tnode.num_children] = p_child_node_id;
		tnode.num_children += 1;

		// back link in the child to the parent
		TNode &tnode_child = _nodes[p_child_node_id];
		tnode_child.parent_id = p_node_id;

		return true;
	}

	void node_replace_child(uint32_t p_parent_id, uint32_t p_old_child_id, uint32_t p_new_child_id) {
		TNode &parent = _nodes[p_parent_id];
		BVH_ASSERT(!parent.is_leaf());

		int child_num = parent.find_child(p_old_child_id);
		BVH_ASSERT(child_num != -1);
		parent.children[child_num] = p_new_child_id;

		TNode &new_child = _nodes[p_new_child_id];
		new_child.parent_id = p_parent_id;
	}

	void node_remove_child(uint32_t p_parent_id, uint32_t p_child_id, uint32_t p_tree_id, bool p_prevent_sibling = false) {
		TNode &parent = _nodes[p_parent_id];
		BVH_ASSERT(!parent.is_leaf());

		int child_num = parent.find_child(p_child_id);
		BVH_ASSERT(child_num != -1);

		parent.remove_child_internal(child_num);

		// no need to keep back references for children at the moment

		uint32_t sibling_id = 0; // always a node id, as tnode is never a leaf
		bool sibling_present = false;

		// if there are more children, or this is the root node, don't try and delete
		if (parent.num_children > 1) {
			return;
		}

		// if there is 1 sibling, it can be moved to be a child of the
		if (parent.num_children == 1) {
			// else there is now a redundant node with one child, which can be removed
			sibling_id = parent.children[0];
			sibling_present = true;
		}

		// now there may be no children in this node .. in which case it can be deleted
		// remove node if empty
		// remove link from parent
		uint32_t grandparent_id = parent.parent_id;

		// special case for root node
		if (grandparent_id == BVHCommon::INVALID) {
			if (sibling_present) {
				// change the root node
				change_root_node(sibling_id, p_tree_id);

				// delete the old root node as no longer needed
				node_free_node_and_leaf(p_parent_id);
			}

			return;
		}

		if (sibling_present) {
			node_replace_child(grandparent_id, p_parent_id, sibling_id);
		} else {
			node_remove_child(grandparent_id, p_parent_id, p_tree_id, true);
		}

		// put the node on the free list to recycle
		node_free_node_and_leaf(p_parent_id);
	}

	// A node can either be a node, or a node AND a leaf combo.
	// Both must be deleted to prevent a leak.
	void node_free_node_and_leaf(uint32_t p_node_id) {
		TNode &node = _nodes[p_node_id];
		if (node.is_leaf()) {
			int leaf_id = node.get_leaf_id();
			_leaves.free(leaf_id);
		}

		_nodes.free(p_node_id);
	}

	void change_root_node(uint32_t p_new_root_id, uint32_t p_tree_id) {
		_root_node_id[p_tree_id] = p_new_root_id;
		TNode &root = _nodes[p_new_root_id];

		// mark no parent
		root.parent_id = BVHCommon::INVALID;
	}

	void node_make_leaf(uint32_t p_node_id) {
		uint32_t child_leaf_id;
		TLeaf *child_leaf = _leaves.request(child_leaf_id);
		child_leaf->clear();

		// zero is reserved at startup, to prevent this id being used
		// (as they are stored as negative values in the node, and zero is already taken)
		BVH_ASSERT(child_leaf_id != 0);

		TNode &node = _nodes[p_node_id];
		node.neg_leaf_id = -(int)child_leaf_id;
	}

	void node_remove_item(uint32_t p_ref_id, uint32_t p_tree_id, BVHABB_CLASS *r_old_aabb = nullptr) {
		// get the reference
		ItemRef &ref = _refs[p_ref_id];
		uint32_t owner_node_id = ref.tnode_id;

		// debug draw special
		// This may not be needed
		if (owner_node_id == BVHCommon::INVALID) {
			return;
		}

		TNode &tnode = _nodes[owner_node_id];
		CRASH_COND(!tnode.is_leaf());

		TLeaf &leaf = _node_get_leaf(tnode);

		// if the aabb is not determining the corner size, then there is no need to refit!
		// (optimization, as merging AABBs takes a lot of time)
		const BVHABB_CLASS &old_aabb = leaf.get_aabb(ref.item_id);

		// shrink a little to prevent using corner aabbs
		// in order to miss the corners first we shrink by node_expansion
		// (which is added to the overall bound of the leaf), then we also
		// shrink by an epsilon, in order to miss out the very corner aabbs
		// which are important in determining the bound. Any other aabb
		// within this can be removed and not affect the overall bound.
		BVHABB_CLASS node_bound = tnode.aabb;
		node_bound.expand(-_node_expansion - 0.001f);
		bool refit = true;

		if (node_bound.is_other_within(old_aabb)) {
			refit = false;
		}

		// record the old aabb if required (for incremental remove_and_reinsert)
		if (r_old_aabb) {
			*r_old_aabb = old_aabb;
		}

		leaf.remove_item_unordered(ref.item_id);

		if (leaf.num_items) {
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = leaf.get_item_ref_id(ref.item_id);

			ItemRef &swapped_ref = _refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;

			// only have to refit if it is an edge item
			// This is a VERY EXPENSIVE STEP
			// we defer the refit updates until the update function is called once per frame
			if (refit) {
				leaf.set_dirty(true);
			}
		} else {
			// remove node if empty
			// remove link from parent
			if (tnode.parent_id != BVHCommon::INVALID) {
				// DANGER .. this can potentially end up with root node with 1 child ...
				// we don't want this and must check for it

				uint32_t parent_id = tnode.parent_id;

				node_remove_child(parent_id, owner_node_id, p_tree_id);
				refit_upward(parent_id);

				// put the node on the free list to recycle
				node_free_node_and_leaf(owner_node_id);
			}

			// else if no parent, it is the root node. Do not delete
		}

		ref.tnode_id = BVHCommon::INVALID;
		ref.item_id = BVHCommon::INVALID; // unset
	}

	// returns true if needs refit of PARENT tree only, the node itself AABB is calculated
	// within this routine
	bool _node_add_item(uint32_t p_node_id, uint32_t p_ref_id, const BVHABB_CLASS &p_aabb) {
		ItemRef &ref = _refs[p_ref_id];
		ref.tnode_id = p_node_id;

		TNode &node = _nodes[p_node_id];
		BVH_ASSERT(node.is_leaf());
		TLeaf &leaf = _node_get_leaf(node);

		// optimization - we only need to do a refit
		// if the added item is changing the AABB of the node.
		// in most cases it won't.
		bool needs_refit = true;

		// expand bound now
		BVHABB_CLASS expanded = p_aabb;
		expanded.expand(_node_expansion);

		// the bound will only be valid if there is an item in there already
		if (leaf.num_items) {
			if (node.aabb.is_other_within(expanded)) {
				// no change to node AABBs
				needs_refit = false;
			} else {
				node.aabb.merge(expanded);
			}
		} else {
			// bound of the node = the new aabb
			node.aabb = expanded;
		}

		ref.item_id = leaf.request_item();
		BVH_ASSERT(ref.item_id != BVHCommon::INVALID);

		// set the aabb of the new item
		leaf.get_aabb(ref.item_id) = p_aabb;

		// back reference on the item back to the item reference
		leaf.get_item_ref_id(ref.item_id) = p_ref_id;

		return needs_refit;
	}

	uint32_t _node_create_another_child(uint32_t p_node_id, const BVHABB_CLASS &p_aabb) {
		uint32_t child_node_id;
		TNode *child_node = _nodes.request(child_node_id);
		child_node->clear();

		// may not be necessary
		child_node->aabb = p_aabb;

		node_add_child(p_node_id, child_node_id);

		return child_node_id;
	}

#include "core/math/bvh_cull.inc"
#include "core/math/bvh_debug.inc"
#include "core/math/bvh_integrity.inc"
#include "core/math/bvh_logic.inc"
#include "core/math/bvh_misc.inc"
#include "core/math/bvh_public.inc"
#include "core/math/bvh_refit.inc"
#include "core/math/bvh_split.inc"
};

#undef VERBOSE_PRINT

// --- End core/math/bvh_tree.h ---
#include "core/math/geometry_3d.h"
#include "core/os/mutex.h"

#include <climits> // INT_MAX

#define BVHTREE_CLASS BVH_Tree<T, NUM_TREES, 2, MAX_ITEMS, USER_PAIR_TEST_FUNCTION, USER_CULL_TEST_FUNCTION, USE_PAIRS, BOUNDS, POINT>
#define BVH_LOCKED_FUNCTION BVHLockedFunction _lock_guard(&_mutex, BVH_THREAD_SAFE &&_thread_safe);

template <typename T, int NUM_TREES = 1, bool USE_PAIRS = false, int MAX_ITEMS = 32, typename USER_PAIR_TEST_FUNCTION = BVH_DummyPairTestFunction<T>, typename USER_CULL_TEST_FUNCTION = BVH_DummyCullTestFunction<T>, typename BOUNDS = AABB, typename POINT = Vector3, bool BVH_THREAD_SAFE = true>
class BVH_Manager {
public:
	// note we are using uint32_t instead of BVHHandle, losing type safety, but this
	// is for compatibility with octree
	typedef void *(*PairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int);
	typedef void (*UnpairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int, void *);
	typedef void *(*CheckPairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int, void *);

	// allow locally toggling thread safety if the template has been compiled with BVH_THREAD_SAFE
	void params_set_thread_safe(bool p_enable) {
		_thread_safe = p_enable;
	}

	// these 2 are crucial for fine tuning, and can be applied manually
	// see the variable declarations for more info.
	void params_set_node_expansion(real_t p_value) {
		BVH_LOCKED_FUNCTION
		if (p_value >= 0.0) {
			tree._node_expansion = p_value;
			tree._auto_node_expansion = false;
		} else {
			tree._auto_node_expansion = true;
		}
	}

	void params_set_pairing_expansion(real_t p_value) {
		BVH_LOCKED_FUNCTION
		tree.params_set_pairing_expansion(p_value);
	}

	void set_pair_callback(PairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		pair_callback = p_callback;
		pair_callback_userdata = p_userdata;
	}
	void set_unpair_callback(UnpairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		unpair_callback = p_callback;
		unpair_callback_userdata = p_userdata;
	}
	void set_check_pair_callback(CheckPairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		check_pair_callback = p_callback;
		check_pair_callback_userdata = p_userdata;
	}

	BVHHandle create(T *p_userdata, bool p_active = true, uint32_t p_tree_id = 0, uint32_t p_tree_collision_mask = 1, const BOUNDS &p_aabb = BOUNDS(), int p_subindex = 0) {
		BVH_LOCKED_FUNCTION

		// not sure if absolutely necessary to flush collisions here. It will cost performance to, instead
		// of waiting for update, so only uncomment this if there are bugs.
		if (USE_PAIRS) {
			//_check_for_collisions();
		}

		BVHHandle h = tree.item_add(p_userdata, p_active, p_aabb, p_subindex, p_tree_id, p_tree_collision_mask);

		if (USE_PAIRS) {
			// for safety initialize the expanded AABB
			BOUNDS &expanded_aabb = tree._pairs[h.id()].expanded_aabb;
			expanded_aabb = p_aabb;
			expanded_aabb.grow_by(tree._pairing_expansion);

			// force a collision check no matter the AABB
			if (p_active) {
				_add_changed_item(h, p_aabb, false);
				_check_for_collisions(true);
			}
		}

		return h;
	}

	////////////////////////////////////////////////////
	// wrapper versions that use uint32_t instead of handle
	// for backward compatibility. Less type safe
	void move(uint32_t p_handle, const BOUNDS &p_aabb) {
		BVHHandle h;
		h.set(p_handle);
		move(h, p_aabb);
	}

	void recheck_pairs(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		recheck_pairs(h);
	}

	void erase(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		erase(h);
	}

	void force_collision_check(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		force_collision_check(h);
	}

	bool activate(uint32_t p_handle, const BOUNDS &p_aabb, bool p_delay_collision_check = false) {
		BVHHandle h;
		h.set(p_handle);
		return activate(h, p_aabb, p_delay_collision_check);
	}

	bool deactivate(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		return deactivate(h);
	}

	void set_tree(uint32_t p_handle, uint32_t p_tree_id, uint32_t p_tree_collision_mask, bool p_force_collision_check = true) {
		BVHHandle h;
		h.set(p_handle);
		set_tree(h, p_tree_id, p_tree_collision_mask, p_force_collision_check);
	}

	uint32_t get_tree_id(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_tree_id(h);
	}
	int get_subindex(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_subindex(h);
	}

	T *get(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_userdata(h);
	}

	////////////////////////////////////////////////////

	void move(BVHHandle p_handle, const BOUNDS &p_aabb) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		if (tree.item_move(p_handle, p_aabb)) {
			if (USE_PAIRS) {
				_add_changed_item(p_handle, p_aabb);
			}
		}
	}

	void recheck_pairs(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		force_collision_check(p_handle);
	}

	void erase(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// call unpair and remove all references to the item
		// before deleting from the tree
		if (USE_PAIRS) {
			_remove_changed_item(p_handle);
		}

		tree.item_remove(p_handle);

		_check_for_collisions(true);
	}

	// use in conjunction with activate if you have deferred the collision check, and
	// set pairable has never been called.
	// (deferred collision checks are a workaround for visual server for historical reasons)
	void force_collision_check(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		if (USE_PAIRS) {
			// the aabb should already be up to date in the BVH
			BOUNDS aabb;
			item_get_AABB(p_handle, aabb);

			// add it as changed even if aabb not different
			_add_changed_item(p_handle, aabb, false);

			// force an immediate full collision check, much like calls to set_pairable
			_check_for_collisions(true);
		}
	}

	// these should be read as set_visible for render trees,
	// but generically this makes items add or remove from the
	// tree internally, to speed things up by ignoring inactive items
	bool activate(BVHHandle p_handle, const BOUNDS &p_aabb, bool p_delay_collision_check = false) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// sending the aabb here prevents the need for the BVH to maintain
		// a redundant copy of the aabb.
		// returns success
		if (tree.item_activate(p_handle, p_aabb)) {
			if (USE_PAIRS) {
				// in the special case of the render tree, when setting visibility we are using the combination of
				// activate then set_pairable. This would case 2 sets of collision checks. For efficiency here we allow
				// deferring to have a single collision check at the set_pairable call.
				// Watch for bugs! This may cause bugs if set_pairable is not called.
				if (!p_delay_collision_check) {
					_add_changed_item(p_handle, p_aabb, false);

					// force an immediate collision check, much like calls to set_pairable
					_check_for_collisions(true);
				}
			}
			return true;
		}

		return false;
	}

	bool deactivate(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// returns success
		if (tree.item_deactivate(p_handle)) {
			// call unpair and remove all references to the item
			// before deleting from the tree
			if (USE_PAIRS) {
				_remove_changed_item(p_handle);

				// force check for collisions, much like an erase was called
				_check_for_collisions(true);
			}
			return true;
		}

		return false;
	}

	bool get_active(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		return tree.item_get_active(p_handle);
	}

	// call e.g. once per frame (this does a trickle optimize)
	void update() {
		BVH_LOCKED_FUNCTION
		tree.update();
		_check_for_collisions();
#ifdef BVH_INTEGRITY_CHECKS
		tree._integrity_check_all();
#endif
	}

	// this can be called more frequently than per frame if necessary
	void update_collisions() {
		BVH_LOCKED_FUNCTION
		_check_for_collisions();
	}

	// prefer calling this directly as type safe
	void set_tree(const BVHHandle &p_handle, uint32_t p_tree_id, uint32_t p_tree_collision_mask, bool p_force_collision_check = true) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// Returns true if the pairing state has changed.
		bool state_changed = tree.item_set_tree(p_handle, p_tree_id, p_tree_collision_mask);

		if (USE_PAIRS) {
			// not sure if absolutely necessary to flush collisions here. It will cost performance to, instead
			// of waiting for update, so only uncomment this if there are bugs.
			//_check_for_collisions();

			if ((p_force_collision_check || state_changed) && tree.item_get_active(p_handle)) {
				// when the pairable state changes, we need to force a collision check because newly pairable
				// items may be in collision, and unpairable items might move out of collision.
				// We cannot depend on waiting for the next update, because that may come much later.
				BOUNDS aabb;
				item_get_AABB(p_handle, aabb);

				// passing false disables the optimization which prevents collision checks if
				// the aabb hasn't changed
				_add_changed_item(p_handle, aabb, false);

				// force an immediate collision check (probably just for this one item)
				// but it must be a FULL collision check, also checking pairable state and masks.
				// This is because AABB intersecting objects may have changed pairable state / mask
				// such that they should no longer be paired. E.g. lights.
				_check_for_collisions(true);
			} // only if active
		}
	}

	// cull tests
	int cull_aabb(const BOUNDS &p_aabb, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tree_collision_mask = p_tree_collision_mask;
		params.abb.from(p_aabb);
		params.tester = p_tester;

		tree.cull_aabb(params);

		return params.result_count_overall;
	}

	int cull_segment(const POINT &p_from, const POINT &p_to, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.segment.from = p_from;
		params.segment.to = p_to;

		tree.cull_segment(params);

		return params.result_count_overall;
	}

	int cull_point(const POINT &p_point, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.point = p_point;

		tree.cull_point(params);
		return params.result_count_overall;
	}

	int cull_convex(const Vector<Plane> &p_convex, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF) {
		BVH_LOCKED_FUNCTION
		if (!p_convex.size()) {
			return 0;
		}

		Vector<Vector3> convex_points = Geometry3D::compute_convex_mesh_points(&p_convex[0], p_convex.size());
		if (convex_points.is_empty()) {
			return 0;
		}

		typename BVHTREE_CLASS::CullParams params;
		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = nullptr;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.hull.planes = &p_convex[0];
		params.hull.num_planes = p_convex.size();
		params.hull.points = &convex_points[0];
		params.hull.num_points = convex_points.size();

		tree.cull_convex(params);

		return params.result_count_overall;
	}

private:
	// do this after moving etc.
	void _check_for_collisions(bool p_full_check = false) {
		if (!changed_items.size()) {
			// noop
			return;
		}

		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = INT_MAX;
		params.result_array = nullptr;
		params.subindex_array = nullptr;

		for (const BVHHandle &h : changed_items) {
			// use the expanded aabb for pairing
			const BOUNDS &expanded_aabb = tree._pairs[h.id()].expanded_aabb;
			BVHABB_CLASS abb;
			abb.from(expanded_aabb);

			tree.item_fill_cullparams(h, params);

			// find all the existing paired aabbs that are no longer
			// paired, and send callbacks
			_find_leavers(h, abb, p_full_check);

			uint32_t changed_item_ref_id = h.id();

			params.abb = abb;

			params.result_count_overall = 0; // might not be needed
			tree.cull_aabb(params, false);

			for (const uint32_t ref_id : tree._cull_hits) {
				// don't collide against ourself
				if (ref_id == changed_item_ref_id) {
					continue;
				}

				// checkmasks is already done in the cull routine.
				BVHHandle h_collidee;
				h_collidee.set_id(ref_id);

				// find NEW enterers, and send callbacks for them only
				_collide(h, h_collidee);
			}
		}
		_reset();
	}

public:
	void item_get_AABB(BVHHandle p_handle, BOUNDS &r_aabb) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVHABB_CLASS abb;
		tree.item_get_ABB(p_handle, abb);
		abb.to(r_aabb);
	}

private:
	// supplemental funcs
	uint32_t item_get_tree_id(BVHHandle p_handle) const { return _get_extra(p_handle).tree_id; }
	T *item_get_userdata(BVHHandle p_handle) const { return _get_extra(p_handle).userdata; }
	int item_get_subindex(BVHHandle p_handle) const { return _get_extra(p_handle).subindex; }

	void _unpair(BVHHandle p_from, BVHHandle p_to) {
		tree._handle_sort(p_from, p_to);

		typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[p_from.id()];
		typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[p_to.id()];

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return;
		}

		typename BVHTREE_CLASS::ItemPairs &pairs_from = tree._pairs[p_from.id()];
		typename BVHTREE_CLASS::ItemPairs &pairs_to = tree._pairs[p_to.id()];

		void *ud_from = pairs_from.remove_pair_to(p_to);
		pairs_to.remove_pair_to(p_from);

#ifdef BVH_VERBOSE_PAIRING
		print_line("_unpair " + itos(p_from.id()) + " from " + itos(p_to.id()));
#endif

		// callback
		if (unpair_callback) {
			unpair_callback(pair_callback_userdata, p_from, exa.userdata, exa.subindex, p_to, exb.userdata, exb.subindex, ud_from);
		}
	}

	void *_recheck_pair(BVHHandle p_from, BVHHandle p_to, void *p_pair_data) {
		tree._handle_sort(p_from, p_to);

		typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[p_from.id()];
		typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[p_to.id()];

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return p_pair_data;
		}

		// callback
		if (check_pair_callback) {
			return check_pair_callback(check_pair_callback_userdata, p_from, exa.userdata, exa.subindex, p_to, exb.userdata, exb.subindex, p_pair_data);
		}

		return p_pair_data;
	}

	// returns true if unpair
	bool _find_leavers_process_pair(typename BVHTREE_CLASS::ItemPairs &p_pairs_from, const BVHABB_CLASS &p_abb_from, BVHHandle p_from, BVHHandle p_to, bool p_full_check) {
		BVHABB_CLASS abb_to;
		tree.item_get_ABB(p_to, abb_to);

		// do they overlap?
		if (p_abb_from.intersects(abb_to)) {
			// the full check for pairable / non pairable (i.e. tree_id and tree_masks) and mask changes is extra expense
			// this need not be done in most cases (for speed) except in the case where set_tree is called
			// where the masks etc of the objects in question may have changed
			if (!p_full_check) {
				return false;
			}
			const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(p_from);
			const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(p_to);

			// Checking tree_ids and tree_collision_masks
			if (exa.are_item_trees_compatible(exb)) {
				bool pair_allowed = USER_PAIR_TEST_FUNCTION::user_pair_check(exa.userdata, exb.userdata);

				// the masks must still be compatible to pair
				// i.e. if there is a hit between the two and they intersect, then they should stay paired
				if (pair_allowed) {
					return false;
				}
			}
		}

		_unpair(p_from, p_to);
		return true;
	}

	// find all the existing paired aabbs that are no longer
	// paired, and send callbacks
	void _find_leavers(BVHHandle p_handle, const BVHABB_CLASS &p_expanded_abb_from, bool p_full_check) {
		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		BVHABB_CLASS abb_from = p_expanded_abb_from;

		// remove from pairing list for every partner
		for (unsigned int n = 0; n < p_from.extended_pairs.size(); n++) {
			BVHHandle h_to = p_from.extended_pairs[n].handle;
			if (_find_leavers_process_pair(p_from, abb_from, p_handle, h_to, p_full_check)) {
				// we need to keep the counter n up to date if we deleted a pair
				// as the number of items in p_from.extended_pairs will have decreased by 1
				// and we don't want to miss an item
				n--;
			}
		}
	}

	// find NEW enterers, and send callbacks for them only
	// handle a and b
	void _collide(BVHHandle p_ha, BVHHandle p_hb) {
		// only have to do this oneway, lower ID then higher ID
		tree._handle_sort(p_ha, p_hb);

		const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(p_ha);
		const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(p_hb);

		// user collision callback
		if (!USER_PAIR_TEST_FUNCTION::user_pair_check(exa.userdata, exb.userdata)) {
			return;
		}

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return;
		}

		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_ha.id()];
		typename BVHTREE_CLASS::ItemPairs &p_to = tree._pairs[p_hb.id()];

		// does this pair exist already?
		// or only check the one with lower number of pairs for greater speed
		if (p_from.num_pairs <= p_to.num_pairs) {
			if (p_from.contains_pair_to(p_hb)) {
				return;
			}
		} else {
			if (p_to.contains_pair_to(p_ha)) {
				return;
			}
		}

		// callback
		void *callback_userdata = nullptr;

#ifdef BVH_VERBOSE_PAIRING
		print_line("_pair " + itos(p_ha.id()) + " to " + itos(p_hb.id()));
#endif

		if (pair_callback) {
			callback_userdata = pair_callback(pair_callback_userdata, p_ha, exa.userdata, exa.subindex, p_hb, exb.userdata, exb.subindex);
		}

		// new pair! .. only really need to store the userdata on the lower handle, but both have storage so...
		p_from.add_pair_to(p_hb, callback_userdata);
		p_to.add_pair_to(p_ha, callback_userdata);
	}

	// if we remove an item, we need to immediately remove the pairs, to prevent reading the pair after deletion
	void _remove_pairs_containing(BVHHandle p_handle) {
		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		// remove from pairing list for every partner.
		// can't easily use a for loop here, because removing changes the size of the list
		while (p_from.extended_pairs.size()) {
			BVHHandle h_to = p_from.extended_pairs[0].handle;
			_unpair(p_handle, h_to);
		}
	}

	// Send pair callbacks again for all existing pairs for the given handle.
	void _recheck_pairs(BVHHandle p_handle) {
		typename BVHTREE_CLASS::ItemPairs &from = tree._pairs[p_handle.id()];

		// checking pair for every partner.
		for (unsigned int n = 0; n < from.extended_pairs.size(); n++) {
			typename BVHTREE_CLASS::ItemPairs::Link &pair = from.extended_pairs[n];
			BVHHandle h_to = pair.handle;
			void *new_pair_data = _recheck_pair(p_handle, h_to, pair.userdata);

			if (new_pair_data != pair.userdata) {
				pair.userdata = new_pair_data;

				// Update pair data for the second item.
				typename BVHTREE_CLASS::ItemPairs &to = tree._pairs[h_to.id()];
				for (unsigned int to_index = 0; to_index < to.extended_pairs.size(); to_index++) {
					typename BVHTREE_CLASS::ItemPairs::Link &to_pair = to.extended_pairs[to_index];
					if (to_pair.handle == p_handle) {
						to_pair.userdata = new_pair_data;
						break;
					}
				}
			}
		}
	}

private:
	const typename BVHTREE_CLASS::ItemExtra &_get_extra(BVHHandle p_handle) const {
		return tree._extra[p_handle.id()];
	}
	const typename BVHTREE_CLASS::ItemRef &_get_ref(BVHHandle p_handle) const {
		return tree._refs[p_handle.id()];
	}

	void _reset() {
		changed_items.clear();
		_tick++;
	}

	void _add_changed_item(BVHHandle p_handle, const BOUNDS &p_aabb, bool p_check_aabb = true) {
		// Note that non pairable items can pair with pairable,
		// so all types must be added to the list

#ifdef BVH_EXPAND_LEAF_AABBS
		// if using expanded AABB in the leaf, the redundancy check will already have been made
		BOUNDS &expanded_aabb = tree._pairs[p_handle.id()].expanded_aabb;
		item_get_AABB(p_handle, expanded_aabb);
#else
		// aabb check with expanded aabb. This greatly decreases processing
		// at the cost of slightly less accurate pairing checks
		// Note this pairing AABB is separate from the AABB in the actual tree
		BOUNDS &expanded_aabb = tree._pairs[p_handle.id()].expanded_aabb;

		// passing p_check_aabb false disables the optimization which prevents collision checks if
		// the aabb hasn't changed. This is needed where set_pairable has been called, but the position
		// has not changed.
		if (p_check_aabb && tree.expanded_aabb_encloses_not_shrink(expanded_aabb, p_aabb)) {
			return;
		}

		// ALWAYS update the new expanded aabb, even if already changed once
		// this tick, because it is vital that the AABB is kept up to date
		expanded_aabb = p_aabb;
		expanded_aabb.grow_by(tree._pairing_expansion);
#endif

		// this code is to ensure that changed items only appear once on the updated list
		// collision checking them multiple times is not needed, and repeats the same thing
		uint32_t &last_updated_tick = tree._extra[p_handle.id()].last_updated_tick;

		if (last_updated_tick == _tick) {
			return; // already on changed list
		}

		// mark as on list
		last_updated_tick = _tick;

		// add to the list
		changed_items.push_back(p_handle);
	}

	void _remove_changed_item(BVHHandle p_handle) {
		// Care has to be taken here for items that are deleted. The ref ID
		// could be reused on the same tick for new items. This is probably
		// rare but should be taken into consideration

		// callbacks
		_remove_pairs_containing(p_handle);

		// remove from changed items (not very efficient yet)
		for (int n = 0; n < (int)changed_items.size(); n++) {
			if (changed_items[n] == p_handle) {
				changed_items.remove_at_unordered(n);

				// because we are using an unordered remove,
				// the last changed item will now be at spot 'n',
				// and we need to redo it, so we prevent moving on to
				// the next n at the next for iteration.
				n--;
			}
		}

		// reset the last updated tick (may not be necessary but just in case)
		tree._extra[p_handle.id()].last_updated_tick = 0;
	}

	PairCallback pair_callback = nullptr;
	UnpairCallback unpair_callback = nullptr;
	CheckPairCallback check_pair_callback = nullptr;
	void *pair_callback_userdata = nullptr;
	void *unpair_callback_userdata = nullptr;
	void *check_pair_callback_userdata = nullptr;

	BVHTREE_CLASS tree;

	// for collision pairing,
	// maintain a list of all items moved etc on each frame / tick
	LocalVector<BVHHandle> changed_items;
	uint32_t _tick = 1; // Start from 1 so items with 0 indicate never updated.

	class BVHLockedFunction {
	public:
		BVHLockedFunction(Mutex *p_mutex, bool p_thread_safe) {
			// will be compiled out if not set in template
			if (p_thread_safe) {
				_mutex = p_mutex;
				_mutex->lock();

			} else {
				_mutex = nullptr;
			}
		}
		~BVHLockedFunction() {
			// will be compiled out if not set in template
			if (_mutex) {
				_mutex->unlock();
			}
		}

	private:
		Mutex *_mutex = nullptr;
	};

	Mutex _mutex;

	// local toggle for turning on and off thread safety in project settings
	bool _thread_safe = BVH_THREAD_SAFE;
};

#undef BVHTREE_CLASS

// --- End core/math/bvh.h ---
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/templates/local_vector.h"
#include "core/typedefs.h"



class GodotConstraint2D;

class GodotBroadPhase2D final
{
public:
	typedef uint32_t ID;

	typedef void* (*PairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_userdata);
	typedef void (*UnpairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_userdata);

private:
	template <typename T>
	class UserPairTestFunction
	{
	public:
		static bool user_pair_check(const T* p_a, const T* p_b)
		{
			// return false if no collision, decided by masks etc
			return p_a->interacts_with(p_b);
		}
	};

	template <typename T>
	class UserCullTestFunction
	{
	public:
		static bool user_cull_check(const T* p_a, const T* p_b)
		{
			return true;
		}
	};

	enum Tree
	{
		TREE_STATIC = 0,
		TREE_DYNAMIC = 1,
	};

	enum TreeFlag
	{
		TREE_FLAG_STATIC = 1 << TREE_STATIC,
		TREE_FLAG_DYNAMIC = 1 << TREE_DYNAMIC,
	};

	BVH_Manager<GodotCollisionObject2D, 2, true, 128, UserPairTestFunction<GodotCollisionObject2D>, UserCullTestFunction<GodotCollisionObject2D>, Rect2, Vector2> bvh;

	static void* _pair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int);
	static void _unpair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int, void*);

	PairCallback pair_callback = nullptr;
	void* pair_userdata = nullptr;
	UnpairCallback unpair_callback = nullptr;
	void* unpair_userdata = nullptr;

public:
	// 0 is an invalid ID
	ID create(GodotCollisionObject2D* p_object, int p_subindex = 0, const Rect2& p_aabb = Rect2(), bool p_static = false);
	void move(ID p_id, const Rect2& p_aabb);
	void set_static(ID p_id, bool p_static);
	void remove(ID p_id);

	GodotCollisionObject2D* get_object(ID p_id) const;
	bool is_static(ID p_id) const;
	int get_subindex(ID p_id) const;

	int cull_segment(const Vector2& p_from, const Vector2& p_to, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);
	int cull_aabb(const Rect2& p_aabb, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);

	void set_pair_callback(PairCallback p_pair_callback, void* p_userdata);
	void set_unpair_callback(UnpairCallback p_unpair_callback, void* p_userdata);

	void update();

	GodotBroadPhase2D();
	~GodotBroadPhase2D() = default;
};

class GodotSpace2D final
{
public:
	enum ElapsedTime
	{
		ELAPSED_TIME_INTEGRATE_FORCES,
		ELAPSED_TIME_GENERATE_ISLANDS,
		ELAPSED_TIME_SETUP_CONSTRAINTS,
		ELAPSED_TIME_SOLVE_CONSTRAINTS,
		ELAPSED_TIME_INTEGRATE_VELOCITIES,
		ELAPSED_TIME_MAX
	};

private:
	struct ExcludedShapeSW
	{
		GodotShape2D* local_shape = nullptr;
		const GodotCollisionObject2D* against_object = nullptr;
		int against_shape_index = 0;
	};

	uint64_t elapsed_time[ELAPSED_TIME_MAX] = {};

	PhysicsDirectSpaceState2D* direct_access = nullptr;
	RID self;

	GodotBroadPhase2D* broadphase = nullptr;
	SelfList<GodotBody2D>::List active_list;
	SelfList<GodotBody2D>::List mass_properties_update_list;
	SelfList<GodotBody2D>::List state_query_list;
	SelfList<GodotArea2D>::List monitor_query_list;
	SelfList<GodotArea2D>::List area_moved_list;

	static void* _broadphase_pair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_self);
	static void _broadphase_unpair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_self);

	HashSet<GodotCollisionObject2D*> objects;

	GodotArea2D* area = nullptr;

	int solver_iterations = 0;

	real_t contact_recycle_radius = 0.0;
	real_t contact_max_separation = 0.0;
	real_t contact_max_allowed_penetration = 0.0;
	real_t contact_bias = 0.0;
	real_t constraint_bias = 0.0;

	enum
	{
		INTERSECTION_QUERY_MAX = 2048
	};

	GodotCollisionObject2D* intersection_query_results[INTERSECTION_QUERY_MAX];
	int intersection_query_subindex_results[INTERSECTION_QUERY_MAX];

	real_t body_linear_velocity_sleep_threshold = 0.0;
	real_t body_angular_velocity_sleep_threshold = 0.0;
	real_t body_time_to_sleep = 0.0;

	bool locked = false;

	real_t last_step = 0.001;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	int _cull_aabb_for_body(GodotBody2D* p_body, const Rect2& p_aabb);

	Vector<Vector2> contact_debug;
	int contact_debug_count = 0;

	friend class PhysicsDirectSpaceState2D;

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_default_area(GodotArea2D* p_area) { area = p_area; }

	GodotArea2D* get_default_area() const { return area; }

	const SelfList<GodotBody2D>::List& get_active_body_list() const;
	void body_add_to_active_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_active_list(SelfList<GodotBody2D>* p_body);
	void body_add_to_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void area_add_to_moved_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_moved_list(SelfList<GodotArea2D>* p_area);
	const SelfList<GodotArea2D>::List& get_moved_area_list() const;

	void body_add_to_state_query_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_state_query_list(SelfList<GodotBody2D>* p_body);

	void area_add_to_monitor_query_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_monitor_query_list(SelfList<GodotArea2D>* p_area);

	GodotBroadPhase2D* get_broadphase();

	void add_object(GodotCollisionObject2D* p_object);
	void remove_object(GodotCollisionObject2D* p_object);
	const HashSet<GodotCollisionObject2D*>& get_objects() const;

	_FORCE_INLINE_ int get_solver_iterations() const { return solver_iterations; }

	_FORCE_INLINE_ real_t get_contact_recycle_radius() const { return contact_recycle_radius; }

	_FORCE_INLINE_ real_t get_contact_max_separation() const { return contact_max_separation; }

	_FORCE_INLINE_ real_t get_contact_max_allowed_penetration() const
	{
		return contact_max_allowed_penetration;
	}

	_FORCE_INLINE_ real_t get_contact_bias() const { return contact_bias; }

	_FORCE_INLINE_ real_t get_constraint_bias() const { return constraint_bias; }

	_FORCE_INLINE_ real_t get_body_linear_velocity_sleep_threshold() const
	{
		return body_linear_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_angular_velocity_sleep_threshold() const
	{
		return body_angular_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_time_to_sleep() const { return body_time_to_sleep; }

	void update();
	void setup();
	void call_queries();

	bool is_locked() const;
	void lock();
	void unlock();

	real_t get_last_step() const { return last_step; }

	void set_last_step(real_t p_step) { last_step = p_step; }

	void set_param(PS2DE::SpaceParameter p_param, real_t p_value);
	real_t get_param(PS2DE::SpaceParameter p_param) const;

	void set_island_count(int p_island_count) { island_count = p_island_count; }

	int get_island_count() const { return island_count; }

	void set_active_objects(int p_active_objects) { active_objects = p_active_objects; }

	int get_active_objects() const { return active_objects; }

	int get_collision_pairs() const { return collision_pairs; }

	bool test_body_motion(GodotBody2D* p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result);

	void set_debug_contacts(int p_amount) { contact_debug.resize(p_amount); }

	_FORCE_INLINE_ bool is_debugging_contacts() const { return !contact_debug.is_empty(); }

	_FORCE_INLINE_ void add_debug_contact(const Vector2& p_contact)
	{
		if (contact_debug_count < contact_debug.size()) {
			contact_debug.write[contact_debug_count++] = p_contact;
		}
	}

	_FORCE_INLINE_ Vector<Vector2> get_debug_contacts() { return contact_debug; }

	_FORCE_INLINE_ int get_debug_contact_count() { return contact_debug_count; }

	PhysicsDirectSpaceState2D* get_direct_state();

	void set_elapsed_time(ElapsedTime p_time, uint64_t p_msec) { elapsed_time[p_time] = p_msec; }

	uint64_t get_elapsed_time(ElapsedTime p_time) const { return elapsed_time[p_time]; }

	GodotSpace2D();
	~GodotSpace2D();
};

class GodotStep2D final
{
	uint64_t _step = 1;

	int iterations = 0;
	real_t delta = 0.0;

	LocalVector<LocalVector<GodotBody2D*>> body_islands;
	LocalVector<LocalVector<GodotConstraint2D*>> constraint_islands;
	LocalVector<GodotConstraint2D*> all_constraints;

	void _populate_island(GodotBody2D* p_body, LocalVector<GodotBody2D*>& p_body_island, LocalVector<GodotConstraint2D*>& p_constraint_island);
	void _setup_constraint(uint32_t p_constraint_index, void* p_userdata = nullptr);
	void _pre_solve_island(LocalVector<GodotConstraint2D*>& p_constraint_island) const;
	void _solve_island(uint32_t p_island_index, void* p_userdata = nullptr) const;
	void _check_suspend(LocalVector<GodotBody2D*>& p_body_island) const;

public:
	void step(GodotSpace2D* p_space, real_t p_delta);
	GodotStep2D();
	~GodotStep2D();
};

// --- End modules/godot_physics_2d/godot_space_2d.h ---


class GodotPhysicsServer2D final
{
	friend class PhysicsDirectSpaceState2D;
	friend class PhysicsDirectBodyState2D;
	bool active = true;
	bool doing_sync = false;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	bool using_threads = false;

	bool flushing_queries = false;

	GodotStep2D* stepper = nullptr;
	HashSet<GodotSpace2D*> active_spaces;

	mutable RID_PtrOwner<GodotShape2D, true> shape_owner;
	mutable RID_PtrOwner<GodotSpace2D, true> space_owner;
	mutable RID_PtrOwner<GodotArea2D, true> area_owner;
	mutable RID_PtrOwner<GodotBody2D, true> body_owner{65536, 1048576};
	mutable RID_PtrOwner<GodotJoint2D, true> joint_owner;

	friend class GodotCollisionObject2D;
	SelfList<GodotCollisionObject2D>::List pending_shape_update_list;
	void _update_shapes();

	RID _shape_create(PS2DE::ShapeType p_shape);

public:
	struct CollCbkData
	{
		Vector2 valid_dir;
		real_t valid_depth = 0.0;
		int max = 0;
		int amount = 0;
		int passed = 0;
		int invalid_by_dir = 0;
		Vector2* ptr = nullptr;
	};

	RID world_boundary_shape_create();
	RID separation_ray_shape_create();
	RID segment_shape_create();
	RID circle_shape_create();
	RID rectangle_shape_create();
	RID capsule_shape_create();
	RID convex_polygon_shape_create();
	RID concave_polygon_shape_create();

	static void _shape_col_cbk(
		const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata);

	void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);

	PS2DE::ShapeType shape_get_type(RID p_shape) const;
	real_t shape_get_custom_solver_bias(RID p_shape) const;

	bool shape_collide(RID p_shape_A, const Transform2D& p_xform_A,
		const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B,
		const Vector2& p_motion_B, Vector2* r_results, int p_result_max,
		int& r_result_count);

	/* SPACE API */

	RID space_create();
	void space_set_active(RID p_space, bool p_active);
	bool space_is_active(RID p_space) const;

	void space_set_param(
		RID p_space, PS2DE::SpaceParameter p_param, real_t p_value);
	real_t space_get_param(RID p_space, PS2DE::SpaceParameter p_param) const;

	void space_set_debug_contacts(RID p_space, int p_max_contacts);
	Vector<Vector2> space_get_contacts(RID p_space) const;
	int space_get_contact_count(RID p_space) const;

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectSpaceState2D* space_get_direct_state(RID p_space);

	/* AREA API */

	RID area_create();

	void area_set_space(RID p_area, RID p_space);
	RID area_get_space(RID p_area) const;

	void area_add_shape(RID p_area, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	void area_set_shape_transform(
		RID p_area, int p_shape_idx, const Transform2D& p_transform);

	int area_get_shape_count(RID p_area) const;
	RID area_get_shape(RID p_area, int p_shape_idx) const;
	Transform2D area_get_shape_transform(RID p_area, int p_shape_idx) const;

	void area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled);

	void area_remove_shape(RID p_area, int p_shape_idx);
	void area_clear_shapes(RID p_area);

	void area_set_transform(RID p_area, const Transform2D& p_transform);

	Transform2D area_get_transform(RID p_area) const;
	void area_set_monitorable(RID p_area, bool p_monitorable);

	void area_set_collision_layer(RID p_area, uint32_t p_layer);
	uint32_t area_get_collision_layer(RID p_area) const;

	void area_set_collision_mask(RID p_area, uint32_t p_mask);
	uint32_t area_get_collision_mask(RID p_area) const;

	void area_set_pickable(RID p_area, bool p_pickable);

	/* BODY API */

	RID body_create();

	void body_set_space(RID p_body, RID p_space);
	RID body_get_space(RID p_body) const;

	void body_set_mode(RID p_body, PS2DE::BodyMode p_mode);
	PS2DE::BodyMode body_get_mode(RID p_body) const;

	void body_add_shape(RID p_body, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	void body_set_shape_transform(
		RID p_body, int p_shape_idx, const Transform2D& p_transform);

	int body_get_shape_count(RID p_body) const;
	RID body_get_shape(RID p_body, int p_shape_idx) const;
	Transform2D body_get_shape_transform(RID p_body, int p_shape_idx) const;

	void body_remove_shape(RID p_body, int p_shape_idx);
	void body_clear_shapes(RID p_body);

	void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);
	void body_set_shape_as_one_way_collision(RID p_body, int p_shape_idx, bool p_enable,
		real_t p_margin = 0, const Vector2& p_direction = Vector2(0, 1));

	void body_set_continuous_collision_detection_mode(
		RID p_body, PS2DE::CCDMode p_mode);
	PS2DE::CCDMode body_get_continuous_collision_detection_mode(RID p_body) const;

	void body_set_collision_layer(RID p_body, uint32_t p_layer);
	uint32_t body_get_collision_layer(RID p_body) const;

	void body_set_collision_mask(RID p_body, uint32_t p_mask);
	uint32_t body_get_collision_mask(RID p_body) const;

	void body_set_collision_priority(RID p_body, real_t p_priority);
	real_t body_get_collision_priority(RID p_body) const;

	void body_reset_mass_properties(RID p_body);

	void body_apply_central_impulse(RID p_body, const Vector2& p_impulse);
	void body_apply_torque_impulse(RID p_body, real_t p_torque);
	void body_apply_impulse(
		RID p_body, const Vector2& p_impulse, const Vector2& p_position = Vector2());

	void body_apply_central_force(RID p_body, const Vector2& p_force);
	void body_apply_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_apply_torque(RID p_body, real_t p_torque);

	void body_add_constant_central_force(RID p_body, const Vector2& p_force);
	void body_add_constant_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_add_constant_torque(RID p_body, real_t p_torque);

	void body_set_constant_force(RID p_body, const Vector2& p_force);
	Vector2 body_get_constant_force(RID p_body) const;

	void body_set_constant_torque(RID p_body, real_t p_torque);
	real_t body_get_constant_torque(RID p_body) const;

	void body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity);

	void body_add_collision_exception(RID p_body, RID p_body_b);
	void body_remove_collision_exception(RID p_body, RID p_body_b);
	void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);

	void body_set_contacts_reported_depth_threshold(
		RID p_body, real_t p_threshold);
	real_t body_get_contacts_reported_depth_threshold(RID p_body) const;

	void body_set_omit_force_integration(RID p_body, bool p_omit);
	bool body_is_omitting_force_integration(RID p_body) const;

	void body_set_max_contacts_reported(RID p_body, int p_contacts);
	int body_get_max_contacts_reported(RID p_body) const;

	bool body_collide_shape(RID p_body, int p_body_shape, RID p_shape,
		const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results,
		int p_result_max, int& r_result_count);

	void body_set_pickable(RID p_body, bool p_pickable);

	bool _body_test_motion(RID p_body, PhysicsTestMotionParameters2D* rp_parameters,
		const Ref<PhysicsTestMotionResult2D>& p_result = Ref<PhysicsTestMotionResult2D>());
	bool body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result = nullptr);

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectBodyState2D* body_get_direct_state(RID p_body);

	/* JOINT API */

	RID joint_create();

	void joint_clear(RID p_joint);

	void joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value);
	real_t joint_get_param(RID p_joint, PS2DE::JointParam p_param) const;

	void joint_disable_collisions_between_bodies(
		RID p_joint, const bool p_disabled);
	bool joint_is_disabled_collisions_between_bodies(RID p_joint) const;

	void joint_make_pin(
		RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b = RID());
	void joint_make_groove(RID p_joint, const Vector2& p_a_groove1,
		const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b);
	void joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a,
		const Vector2& p_anchor_b, RID p_body_a, RID p_body_b = RID());

	void pin_joint_set_flag(
		RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) const;
	void pin_joint_set_param(
		RID p_joint, PS2DE::PinJointParam p_param, real_t p_value);
	real_t pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) const;
	void damped_spring_joint_set_param(
		RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t damped_spring_joint_get_param(
		RID p_joint, PS2DE::DampedSpringParam p_param) const;

	PS2DE::JointType joint_get_type(RID p_joint) const;

	/* MISC */

	void free_rid(RID p_rid);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	void set_active(bool p_active);
	void init();
	void step(real_t p_step);
	void sync();
	void flush_queries();
	void end_sync();
	void finish();

	bool is_flushing_queries() const { return flushing_queries; }

	int get_process_info(PS2DE::ProcessInfo p_info);

	explicit GodotPhysicsServer2D(bool p_using_threads = false);

	~GodotPhysicsServer2D()
	{
		finish();
	}
};
// --- End modules/godot_physics_2d/godot_physics_server_2d.h ---

// --- Folded from servers/navigation_2d/navigation_server_2d.h ---
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/vector.h"
#include "core/types.h"
// --- Inlined/Folded from scene/resources/2d/navigation_mesh_source_geometry_data_2d.h ---
#include "core/io/resource.h"
#include "core/os/mutex.h"

class NavigationMeshSourceGeometryData2D : public Resource
{
	friend class NavMeshGenerator2D;

	RWLock geometry_rwlock;

	Vector<Vector<Vector2>> traversable_outlines;
	Vector<Vector<Vector2>> obstruction_outlines;

	Rect2 bounds;
	bool bounds_dirty = true;

public:
	struct ProjectedObstruction;

private:
	Vector<ProjectedObstruction> _projected_obstructions;

protected:

public:
	struct ProjectedObstruction
	{
		static inline uint32_t VERSION = 1; // Increase when format changes so we can detect
											// outdated formats and provide compatibility.

		Vector<float> vertices;
		bool carve = false;
	};

	void _set_traversable_outlines(const Vector<Vector<Vector2>>& p_traversable_outlines);
	const Vector<Vector<Vector2>>& _get_traversable_outlines() const;

	void _set_obstruction_outlines(const Vector<Vector<Vector2>>& p_obstruction_outlines);
	const Vector<Vector<Vector2>>& _get_obstruction_outlines() const;

	void _add_traversable_outline(const Vector<Vector2>& p_shape_outline);
	void _add_obstruction_outline(const Vector<Vector2>& p_shape_outline);

	// kept root node transform here on the geometry data
	// if we add this transform to all exposed functions we need to break comp on all functions
	// later when navmesh changes from global transform to relative to navregion but if it stays
	// here we can just remove it and change the internal functions only
	Transform2D root_node_transform;

	void add_traversable_outline(const PackedVector2Array& p_shape_outline);
	void add_obstruction_outline(const PackedVector2Array& p_shape_outline);

	bool has_data();
	void clear();
	void clear_projected_obstructions();

	void add_projected_obstruction(const Vector<Vector2>& p_vertices, bool p_carve);
	Vector<ProjectedObstruction> _get_projected_obstructions() const;

	void merge(const Ref<NavigationMeshSourceGeometryData2D>& p_other_geometry);

	void set_data(const Vector<Vector<Vector2>>& p_traversable_outlines,
		const Vector<Vector<Vector2>>& p_obstruction_outlines,
		Vector<ProjectedObstruction>& p_projected_obstructions);
	void get_data(Vector<Vector<Vector2>>& r_traversable_outlines,
		Vector<Vector<Vector2>>& r_obstruction_outlines,
		Vector<ProjectedObstruction>& r_projected_obstructions);

	Rect2 get_bounds();

	~NavigationMeshSourceGeometryData2D() { clear(); }
};
// --- Inlined/Folded from scene/resources/2d/navigation_polygon.h ---
#include "scene/resources/mesh.h"

class NavigationPolygon : public Resource
{
	RWLock rwlock;

	Vector<Vector2> vertices;
	Vector<Vector<int>> polygons;
	Vector<Vector<Vector2>> outlines;

	mutable Rect2 item_rect;
	mutable bool rect_cache_dirty = true;

	Mutex navigation_mesh_generation;
	// Navigation mesh
	Ref<NavigationMesh> navigation_mesh;

	real_t cell_size = 1.0f;
	real_t border_size = 0.0f;

	Rect2 baking_rect;
	Vector2 baking_rect_offset;

protected:

public:
	NavigationPolygon();

#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const;
	bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;
#endif // DEBUG_ENABLED

	enum SamplePartitionType
	{
		SAMPLE_PARTITION_CONVEX_PARTITION = 0,
		SAMPLE_PARTITION_TRIANGULATE,
		SAMPLE_PARTITION_MAX
	};

	enum ParsedGeometryType
	{
		PARSED_GEOMETRY_MESH_INSTANCES = 0,
		PARSED_GEOMETRY_STATIC_COLLIDERS,
		PARSED_GEOMETRY_BOTH,
		PARSED_GEOMETRY_MAX
	};

	enum SourceGeometryMode
	{
		SOURCE_GEOMETRY_ROOT_NODE_CHILDREN = 0,
		SOURCE_GEOMETRY_GROUPS_WITH_CHILDREN,
		SOURCE_GEOMETRY_GROUPS_EXPLICIT,
		SOURCE_GEOMETRY_MAX
	};

	real_t agent_radius = 10.0f;

	SamplePartitionType partition_type = SAMPLE_PARTITION_CONVEX_PARTITION;
	ParsedGeometryType parsed_geometry_type = PARSED_GEOMETRY_BOTH;
	uint32_t parsed_collision_mask = 0xFFFFFFFF;

	SourceGeometryMode source_geometry_mode = SOURCE_GEOMETRY_ROOT_NODE_CHILDREN;
	String source_geometry_group_name = "navigation_polygon_source_geometry_group";

	void set_vertices(const Vector<Vector2>& p_vertices);
	const Vector<Vector2>& get_vertices() const _LIFETIME_BOUND_;

	void add_polygon(const Vector<int>& p_polygon);
	int get_polygon_count() const;

	void add_outline(const Vector<Vector2>& p_outline);
	void add_outline_at_index(const Vector<Vector2>& p_outline, int p_index);
	void set_outline(int p_idx, const Vector<Vector2>& p_outline);
	Vector<Vector2> get_outline(int p_idx) const;
	void remove_outline(int p_idx);
	int get_outline_count() const;
	void set_outlines(const Vector<Vector<Vector2>>& p_outlines);
	Vector<Vector<Vector2>> get_outlines() const;

	void clear_outlines();
#ifndef DISABLE_DEPRECATED
	void make_polygons_from_outlines();
#endif // DISABLE_DEPRECATED

	void set_polygons(const Vector<Vector<int>>& p_polygons);
	Vector<Vector<int>> get_polygons() const;
	Vector<int> get_polygon(int p_idx);
	void clear_polygons();

	void set_sample_partition_type(SamplePartitionType p_value);
	SamplePartitionType get_sample_partition_type() const;

	void set_parsed_geometry_type(ParsedGeometryType p_geometry_type);
	ParsedGeometryType get_parsed_geometry_type() const;

	void set_parsed_collision_mask(uint32_t p_mask);
	uint32_t get_parsed_collision_mask() const;

	void set_parsed_collision_mask_value(int p_layer_number, bool p_value);
	bool get_parsed_collision_mask_value(int p_layer_number) const;

	void set_source_geometry_mode(SourceGeometryMode p_geometry_mode);
	SourceGeometryMode get_source_geometry_mode() const;

	void set_source_geometry_group_name(const String& p_group_name);
	String get_source_geometry_group_name() const;

	void set_agent_radius(real_t p_value);
	real_t get_agent_radius() const;

	Ref<NavigationMesh> get_navigation_mesh();

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const;

	void set_border_size(real_t p_value);
	real_t get_border_size() const;

	void set_baking_rect(const Rect2& p_rect);
	Rect2 get_baking_rect() const;

	void set_baking_rect_offset(const Vector2& p_rect_offset);
	Vector2 get_baking_rect_offset() const;

	void clear();

	void set_data(const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons);
	void set_data(const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons,
		const Vector<Vector<Vector2>>& p_outlines);
	void get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons);
	void get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons,
		Vector<Vector<Vector2>>& r_outlines);
};

namespace NavigationEnums2D {

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

} // namespace NavigationEnums2D

namespace NavigationDefaults2D {

// Rasterization.

// Same as in 3D but larger since 1px is treated as 1m.
constexpr float NAV_MESH_CELL_SIZE = 1.0f; // Must match ProjectSettings default 2D cell_size.
constexpr float NAV_MESH_CELL_SIZE_MIN = 0.01f;
constexpr const char *const NAV_MESH_CELL_SIZE_HINT = "0.001,100,0.001,or_greater";

// Map.

constexpr float EDGE_CONNECTION_MARGIN = 1.0f;
constexpr float LINK_CONNECTION_RADIUS = 4.0f;
constexpr int path_search_max_polygons = 4096;

// Agent.

constexpr float AVOIDANCE_AGENT_RADIUS = 10.0;
constexpr float AVOIDANCE_AGENT_MAX_SPEED = 100.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_AGENTS = 1.0;
constexpr float AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES = 0.0;
constexpr int AVOIDANCE_AGENT_MAX_NEIGHBORS = 10;
constexpr float AVOIDANCE_AGENT_NEIGHBOR_DISTANCE = 500.0;

} // namespace NavigationDefaults2D

class NavigationPathQueryParameters2D final : public RefCounted
{
public:
	enum PathfindingAlgorithm
	{
		PATHFINDING_ALGORITHM_ASTAR = NavigationEnums2D::PATHFINDING_ALGORITHM_ASTAR,
	};

	enum PathPostProcessing
	{
		PATH_POSTPROCESSING_CORRIDORFUNNEL = NavigationEnums2D::PATH_POSTPROCESSING_CORRIDORFUNNEL,
		PATH_POSTPROCESSING_EDGECENTERED = NavigationEnums2D::PATH_POSTPROCESSING_EDGECENTERED,
		PATH_POSTPROCESSING_NONE = NavigationEnums2D::PATH_POSTPROCESSING_NONE,
	};

	enum PathMetadataFlags
	{
		PATH_METADATA_INCLUDE_NONE = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_NONE,
		PATH_METADATA_INCLUDE_TYPES = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_TYPES,
		PATH_METADATA_INCLUDE_RIDS = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_RIDS,
		PATH_METADATA_INCLUDE_OWNERS = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_OWNERS,
		PATH_METADATA_INCLUDE_ALL = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_ALL
	};

private:
	PathfindingAlgorithm pathfinding_algorithm = PATHFINDING_ALGORITHM_ASTAR;
	PathPostProcessing path_postprocessing = PATH_POSTPROCESSING_CORRIDORFUNNEL;
	RID map;
	Vector2 start_position;
	Vector2 target_position;
	uint32_t navigation_layers = 1;
	uint32_t metadata_flags = PATH_METADATA_INCLUDE_ALL;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;

	LocalVector<RID> _excluded_regions;
	LocalVector<RID> _included_regions;

	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

public:
	void set_pathfinding_algorithm(const PathfindingAlgorithm p_pathfinding_algorithm);
	PathfindingAlgorithm get_pathfinding_algorithm() const;

	void set_path_postprocessing(const PathPostProcessing p_path_postprocessing);
	PathPostProcessing get_path_postprocessing() const;

	void set_map(RID p_map);
	RID get_map() const;

	void set_start_position(const Vector2 p_start_position);
	Vector2 get_start_position() const;

	void set_target_position(const Vector2 p_target_position);
	Vector2 get_target_position() const;

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

class NavigationPathQueryResult2D final : public RefCounted
{
	Vector<Vector2> path;
	Vector<int32_t> path_types;
	Vector<int64_t> path_owner_ids;
	float path_length = 0.0;

public:
	enum PathSegmentType
	{
		PATH_SEGMENT_TYPE_REGION = NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_REGION,
		PATH_SEGMENT_TYPE_LINK = NavigationEnums2D::PathSegmentType::PATH_SEGMENT_TYPE_LINK,
	};

	void set_path(const Vector<Vector2>& p_path);
	const Vector<Vector2>& get_path() const;

	void set_path_types(const Vector<int32_t>& p_path_types);
	const Vector<int32_t>& get_path_types() const;

	void set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids);
	const Vector<int64_t>& get_path_owner_ids() const;

	void set_path_length(float p_length);
	float get_path_length() const;

	void reset();

	void set_data(const LocalVector<Vector2>& p_path, const LocalVector<int32_t>& p_path_types,
		const LocalVector<RID>& p_path_rids, const LocalVector<int64_t>& p_path_owner_ids);
};

class NavigationServer2D final
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
		bool debug_navigation_enable_agent_radius;
		bool debug_navigation_enable_obstacles;
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
		bool debug_navigation_enable_edge_lines = true;
		bool debug_navigation_enable_geometry_face_random_color = true;
		bool debug_navigation_enable_link_connections = true;
		bool debug_navigation_enable_agent_paths = true;

		bool debug_navigation_avoidance_enable_agents_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_radius = true;
		bool debug_navigation_avoidance_enable_obstacles_static = true;
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

	static void map_set_cell_size(RID p_map, real_t p_cell_size);
	static real_t map_get_cell_size(RID p_map);

	static void map_set_merge_rasterizer_cell_scale(RID p_map, float p_value);
	static float map_get_merge_rasterizer_cell_scale(RID p_map);

	static void map_set_use_edge_connections(RID p_map, bool p_enabled);
	static bool map_get_use_edge_connections(RID p_map);

	static void map_set_edge_connection_margin(RID p_map, real_t p_connection_margin);
	static real_t map_get_edge_connection_margin(RID p_map);

	static void map_set_link_connection_radius(RID p_map, real_t p_connection_radius);
	static real_t map_get_link_connection_radius(RID p_map);

	static Vector<Vector2> map_get_path(RID p_map, Vector2 p_origin, Vector2 p_destination,
		bool p_optimize, uint32_t p_navigation_layers = 1);
	static Vector2 map_get_closest_point(RID p_map, const Vector2& p_point);
	static RID map_get_closest_point_owner(RID p_map, const Vector2& p_point);

	static void map_force_update(RID p_map);
	static uint32_t map_get_iteration_id(RID p_map);

	static void map_set_use_async_iterations(RID p_map, bool p_enabled);
	static bool map_get_use_async_iterations(RID p_map);

	static Vector2 map_get_random_point(RID p_map, uint32_t p_navigation_layers, bool p_uniformly);

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

	static bool region_owns_point(RID p_region, const Vector2& p_point);

	static void region_set_map(RID p_region, RID p_map);
	static RID region_get_map(RID p_region);

	static void region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers);
	static uint32_t region_get_navigation_layers(RID p_region);

	static void region_set_transform(RID p_region, Transform2D p_transform);
	static Transform2D region_get_transform(RID p_region);

	static void region_set_navigation_polygon(
		RID p_region, Ref<NavigationPolygon> p_navigation_polygon);

	static int region_get_connections_count(RID p_region);
	static Vector2 region_get_connection_pathway_start(RID p_region, int p_connection_id);
	static Vector2 region_get_connection_pathway_end(RID p_region, int p_connection_id);

	static Vector2 region_get_closest_point(RID p_region, const Vector2& p_point);
	static Vector2 region_get_random_point(
		RID p_region, uint32_t p_navigation_layers, bool p_uniformly);
	static Rect2 region_get_bounds(RID p_region);

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

	static void link_set_start_position(RID p_link, Vector2 p_position);
	static Vector2 link_get_start_position(RID p_link);

	static void link_set_end_position(RID p_link, Vector2 p_position);
	static Vector2 link_get_end_position(RID p_link);

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

	static void agent_set_max_speed(RID p_agent, real_t p_max_speed);
	static real_t agent_get_max_speed(RID p_agent);

	static void agent_set_velocity_forced(RID p_agent, Vector2 p_velocity);
	static void agent_set_velocity(RID p_agent, Vector2 p_velocity);
	static Vector2 agent_get_velocity(RID p_agent);

	static void agent_set_position(RID p_agent, Vector2 p_position);
	static Vector2 agent_get_position(RID p_agent);

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

	static void obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled);
	static bool obstacle_get_avoidance_enabled(RID p_obstacle);

	static void obstacle_set_map(RID p_obstacle, RID p_map);
	static RID obstacle_get_map(RID p_obstacle);

	static void obstacle_set_paused(RID p_obstacle, bool p_paused);
	static bool obstacle_get_paused(RID p_obstacle);

	static void obstacle_set_radius(RID p_obstacle, real_t p_radius);
	static real_t obstacle_get_radius(RID p_obstacle);

	static void obstacle_set_velocity(RID p_obstacle, Vector2 p_velocity);
	static Vector2 obstacle_get_velocity(RID p_obstacle);

	static void obstacle_set_position(RID p_obstacle, Vector2 p_position);
	static Vector2 obstacle_get_position(RID p_obstacle);

	static void obstacle_set_vertices(RID p_obstacle, const Vector<Vector2>& p_vertices);
	static Vector<Vector2> obstacle_get_vertices(RID p_obstacle);

	static void obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers);
	static uint32_t obstacle_get_avoidance_layers(RID p_obstacle);

	/* PATH QUERY & UTILITIES */

	static void query_path(const Ref<NavigationPathQueryParameters2D>& p_query_parameters,
		Ref<NavigationPathQueryResult2D> p_query_result);
	static Vector<Vector2> simplify_path(const Vector<Vector2>& p_path, real_t p_epsilon);

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

	static void set_debug_navigation_geometry_face_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_color();

	static void set_debug_navigation_geometry_face_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_face_disabled_color();

	static void set_debug_navigation_geometry_edge_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_color();

	static void set_debug_navigation_enable_obstacles(bool p_enable);
	static bool get_debug_navigation_enable_obstacles();

	static void set_debug_navigation_geometry_edge_disabled_color(const Color& p_color);
	static Color get_debug_navigation_geometry_edge_disabled_color();

	static void set_debug_navigation_link_connection_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_color();

	static void set_debug_navigation_link_connection_disabled_color(const Color& p_color);
	static Color get_debug_navigation_link_connection_disabled_color();

	static void set_debug_navigation_enable_edge_connections(bool p_value);
	static bool get_debug_navigation_enable_edge_connections();

	static void set_debug_navigation_enable_geometry_face_random_color(bool p_value);
	static bool get_debug_navigation_enable_geometry_face_random_color();

	static void set_debug_navigation_enable_link_connections(bool p_enable);
	static bool get_debug_navigation_enable_link_connections();

	static void set_debug_navigation_enable_edge_lines(bool p_value);
	static bool get_debug_navigation_enable_edge_lines();

	static void set_debug_navigation_agent_path_color(const Color& p_color);
	static Color get_debug_navigation_agent_path_color();

	static void set_debug_navigation_enable_agent_radius(bool p_enable);
	static bool get_debug_navigation_enable_agent_radius();

	static void set_debug_navigation_enable_agent_paths(bool p_value);
	static bool get_debug_navigation_enable_agent_paths();

	static void set_debug_navigation_agent_path_point_size(real_t p_point_size);
	static real_t get_debug_navigation_agent_path_point_size();

	static void set_debug_navigation_avoidance_enable_agents_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_agents_radius();

	static void set_debug_navigation_avoidance_enable_obstacles_radius(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_radius();

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

	static void set_debug_navigation_avoidance_enable_obstacles_static(bool p_value);
	static bool get_debug_navigation_avoidance_enable_obstacles_static();
#endif // DEBUG_ENABLED

	/* LIFECYCLE DISALLOWANCE */

	NavigationServer2D() = delete;
	NavigationServer2D(const NavigationServer2D&) = delete;
	NavigationServer2D& operator=(const NavigationServer2D&) = delete;
	~NavigationServer2D() = delete;
};

// --- Folded from servers/nav_heap.h ---
#include "core/templates/local_vector.h"

// This file contains Heap which is used by both 2D and 3D navigation.

template <typename T>
struct NoopIndexer {
	void operator()(const T &p_value, uint32_t p_index) {}
};

/**
 * A max-heap implementation that notifies of element index changes.
 */
template <typename T, typename LessThan = Comparator<T>, typename Indexer = NoopIndexer<T>>
class Heap {
	LocalVector<T> _buffer;

	LessThan _less_than;
	Indexer _indexer;

public:
	static constexpr uint32_t INVALID_INDEX = UINT32_MAX;
	void reserve(uint32_t p_size) {
		_buffer.reserve(p_size);
	}

	uint32_t size() const {
		return _buffer.size();
	}

	bool is_empty() const {
		return _buffer.is_empty();
	}

	void push(const T &p_element) {
		_buffer.push_back(p_element);
		_indexer(p_element, _buffer.size() - 1);
		_shift_up(_buffer.size() - 1);
	}

	T pop() {
		ERR_FAIL_COND_V_MSG(_buffer.is_empty(), T(), "Can't pop an empty heap.");
		T value = _buffer[0];
		_indexer(value, INVALID_INDEX);
		if (_buffer.size() > 1) {
			_buffer[0] = _buffer[_buffer.size() - 1];
			_indexer(_buffer[0], 0);
			_buffer.remove_at(_buffer.size() - 1);
			_shift_down(0);
		} else {
			_buffer.remove_at(_buffer.size() - 1);
		}
		return value;
	}

	/**
	 * Update the position of the element in the heap if necessary.
	 */
	void shift(uint32_t p_index) {
		ERR_FAIL_UNSIGNED_INDEX_MSG(p_index, _buffer.size(), "Heap element index is out of range.");
		if (!_shift_up(p_index)) {
			_shift_down(p_index);
		}
	}

	void clear() {
		for (const T &value : _buffer) {
			_indexer(value, INVALID_INDEX);
		}
		_buffer.clear();
	}

	Heap() {}

	Heap(const LessThan &p_less_than) :
			_less_than(p_less_than) {}

	Heap(const Indexer &p_indexer) :
			_indexer(p_indexer) {}

	Heap(const LessThan &p_less_than, const Indexer &p_indexer) :
			_less_than(p_less_than), _indexer(p_indexer) {}

private:
	bool _shift_up(uint32_t p_index) {
		T value = _buffer[p_index];
		uint32_t current_index = p_index;
		uint32_t parent_index = (current_index - 1) / 2;
		while (current_index > 0 && _less_than(_buffer[parent_index], value)) {
			_buffer[current_index] = _buffer[parent_index];
			_indexer(_buffer[current_index], current_index);
			current_index = parent_index;
			parent_index = (current_index - 1) / 2;
		}
		if (current_index != p_index) {
			_buffer[current_index] = value;
			_indexer(value, current_index);
			return true;
		} else {
			return false;
		}
	}

	bool _shift_down(uint32_t p_index) {
		T value = _buffer[p_index];
		uint32_t current_index = p_index;
		uint32_t child_index = 2 * current_index + 1;
		while (child_index < _buffer.size()) {
			if (child_index + 1 < _buffer.size() &&
					_less_than(_buffer[child_index], _buffer[child_index + 1])) {
				child_index++;
			}
			if (_less_than(_buffer[child_index], value)) {
				break;
			}
			_buffer[current_index] = _buffer[child_index];
			_indexer(_buffer[current_index], current_index);
			current_index = child_index;
			child_index = 2 * current_index + 1;
		}
		if (current_index != p_index) {
			_buffer[current_index] = value;
			_indexer(value, current_index);
			return true;
		} else {
			return false;
		}
	}
};
// --- End servers/nav_heap.h ---

// --- Folded from modules/navigation_2d/nav_map_2d.h ---
#include <cfloat>

#include "core/typedefs.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/os/mutex.h"
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/vector.h"
#include "core/types.h"



#include "thirdparty/rvo2/rvo2_2d/Agent2d.h"
#include "thirdparty/rvo2/rvo2_2d/KdTree2d.h"
#include "thirdparty/rvo2/rvo2_2d/RVOSimulator2d.h"

class NavIteration2D;
class NavMap2D;
class NavRegion2D;
class NavLink2D;
class NavAgent2D;
class NavObstacle2D;
struct NavMapIteration2D;
struct NavMapIterationBuild2D;
struct NavRegionIterationBuild2D;

struct Triangle2
{
	Vector2 vertex[3];

	real_t get_area() const
	{
		return Math::abs((vertex[0] - vertex[1]).cross(vertex[0] - vertex[2])) * 0.5f;
	}

	Vector2 get_random_point_inside() const;

	Vector2 get_closest_point_to(const Vector2& p_point) const;

	Triangle2() {}
	Triangle2(const Vector2& p_v1, const Vector2& p_v2, const Vector2& p_v3)
	{
		vertex[0] = p_v1;
		vertex[1] = p_v2;
		vertex[2] = p_v3;
	}
};

namespace Nav2D {

struct Polygon;

union PointKey {
	struct {
		int64_t x : 32;
		int64_t y : 32;
	};

	uint64_t key = 0;
};

struct EdgeKey {
	PointKey a;
	PointKey b;

	static uint32_t hash(const EdgeKey& p_val)
	{
		return hash_one_uint64(p_val.a.key) ^ hash_one_uint64(p_val.b.key);
	}

	bool operator==(const EdgeKey& p_key) const
	{
		return (a.key == p_key.a.key) && (b.key == p_key.b.key);
	}

	EdgeKey(const PointKey& p_a = PointKey(), const PointKey& p_b = PointKey()) : a(p_a), b(p_b)
	{
		if (a.key > b.key) {
			SWAP(a, b);
		}
	}
};

struct ConnectableEdge {
	EdgeKey ek;
	uint32_t polygon_index;
	int edge = -1;
	Vector2 pathway_start;
	Vector2 pathway_end;
};

struct Connection {
	Polygon* polygon = nullptr;
	int edge = -1;
	Vector2 pathway_start;
	Vector2 pathway_end;
};

struct Polygon {
	uint32_t id = UINT32_MAX;
	const NavIteration2D* owner = nullptr;
	LocalVector<Vector2> vertices;
	real_t surface_area = 0.0;
};

struct NavigationPoly {
	const Polygon* poly = nullptr;
	uint32_t traversable_poly_index = UINT32_MAX;

	int back_navigation_poly_id = -1;
	int back_navigation_edge = -1;
	Vector2 back_navigation_edge_pathway_start;
	Vector2 back_navigation_edge_pathway_end;

	Vector2 entry;
	real_t traveled_distance = 0.0;
	real_t distance_to_destination = 0.0;

	real_t total_travel_cost() const { return traveled_distance + distance_to_destination; }

	bool operator==(const NavigationPoly& p_other) const { return poly == p_other.poly; }

	bool operator!=(const NavigationPoly& p_other) const { return !(*this == p_other); }

	void reset()
	{
		poly = nullptr;
		traversable_poly_index = UINT32_MAX;
		back_navigation_poly_id = -1;
		back_navigation_edge = -1;
		traveled_distance = FLT_MAX;
		distance_to_destination = 0.0;
	}
};

struct NavPolyTravelCostGreaterThan {
	bool operator()(const NavigationPoly* p_poly_a, const NavigationPoly* p_poly_b) const
	{
		real_t f_cost_a = p_poly_a->total_travel_cost();
		real_t h_cost_a = p_poly_a->distance_to_destination;
		real_t f_cost_b = p_poly_b->total_travel_cost();
		real_t h_cost_b = p_poly_b->distance_to_destination;

		if (f_cost_a != f_cost_b) {
			return f_cost_a > f_cost_b;
		}
		else {
			return h_cost_a > h_cost_b;
		}
	}
};

struct NavPolyHeapIndexer {
	void operator()(NavigationPoly* p_poly, uint32_t p_heap_index) const
	{
		p_poly->traversable_poly_index = p_heap_index;
	}
};

struct ClosestPointQueryResult {
	Vector2 point;
	RID owner;
};

struct EdgeConnectionPair {
	Connection connections[2];
	int size = 0;
};

struct PerformanceData {
	int pm_region_count = 0;
	int pm_agent_count = 0;
	int pm_link_count = 0;
	int pm_polygon_count = 0;
	int pm_edge_count = 0;
	int pm_edge_merge_count = 0;
	int pm_edge_connection_count = 0;
	int pm_edge_free_count = 0;
	int pm_obstacle_count = 0;

	void reset()
	{
		pm_region_count = 0;
		pm_agent_count = 0;
		pm_link_count = 0;
		pm_polygon_count = 0;
		pm_edge_count = 0;
		pm_edge_merge_count = 0;
		pm_edge_connection_count = 0;
		pm_edge_free_count = 0;
		pm_obstacle_count = 0;
	}
};

} // namespace Nav2D

class NavIteration2D final : public RefCounted
{
public:
	bool enabled = true;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	NavigationEnums2D::PathSegmentType owner_type = NavigationEnums2D::PATH_SEGMENT_TYPE_REGION;
	RID owner_rid;
	bool owner_use_edge_connections = false;
	LocalVector<Nav2D::Polygon> navmesh_polygons;
	LocalVector<LocalVector<Nav2D::Connection>> internal_connections;

	// Region iteration data.
	Transform2D transform;
	real_t surface_area = 0.0;
	Rect2 bounds;
	LocalVector<Nav2D::ConnectableEdge> external_edges;

	// Link iteration data.
	bool bidirectional = true;
	Vector2 start_position;
	Vector2 end_position;

	bool get_enabled() const { return enabled; }
	NavigationEnums2D::PathSegmentType get_type() const { return owner_type; }
	RID get_self() const { return owner_rid; }
	uint32_t get_navigation_layers() const { return navigation_layers; }
	real_t get_enter_cost() const { return enter_cost; }
	real_t get_travel_cost() const { return travel_cost; }
	bool get_use_edge_connections() const { return owner_use_edge_connections; }

	const LocalVector<Nav2D::Polygon>& get_navmesh_polygons() const { return navmesh_polygons; }
	const LocalVector<LocalVector<Nav2D::Connection>>& get_internal_connections() const
	{
		return internal_connections;
	}

	const Transform2D& get_transform() const { return transform; }
	real_t get_surface_area() const { return surface_area; }
	Rect2 get_bounds() const { return bounds; }
	const LocalVector<Nav2D::ConnectableEdge>& get_external_edges() const { return external_edges; }

	Vector2 get_start_position() const { return start_position; }
	Vector2 get_end_position() const { return end_position; }
	bool is_bidirectional() const { return bidirectional; }

	~NavIteration2D() override
	{
		external_edges.clear();
		navmesh_polygons.clear();
		internal_connections.clear();
	}
};

struct NavRegionIterationBuild2D
{
	Nav2D::PerformanceData performance_data;

	NavRegion2D* region = nullptr;

	Vector2 map_cell_size;
	Transform2D region_transform;

	struct NavMeshData
	{
		Vector<Vector2> vertices;
		Vector<Vector<int>> polygons;

		void clear()
		{
			vertices.clear();
			polygons.clear();
		}
	} navmesh_data;

	Ref<NavIteration2D> region_iteration;

	HashMap<Nav2D::EdgeKey, Nav2D::EdgeConnectionPair, Nav2D::EdgeKey> iter_connection_pairs_map;

	void reset()
	{
		performance_data.reset();

		navmesh_data.clear();
		region_iteration = Ref<NavIteration2D>();
		iter_connection_pairs_map.clear();
	}
};

class NavMeshQueries2D final
{
public:
	struct PathQuerySlot
	{
		LocalVector<Nav2D::NavigationPoly> path_corridor;
		Heap<Nav2D::NavigationPoly*, Nav2D::NavPolyTravelCostGreaterThan, Nav2D::NavPolyHeapIndexer>
			traversable_polys;
		bool in_use = false;
		uint32_t slot_index = 0;
		AHashMap<const Nav2D::Polygon*, uint32_t> poly_to_id;
	};

	struct NavMeshPathQueryTask2D
	{
		enum TaskStatus
		{
			QUERY_STARTED,
			QUERY_FINISHED,
			QUERY_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Vector2 start_position;
		Vector2 target_position;
		uint32_t navigation_layers;
		uint32_t metadata_flags = NavigationEnums2D::PathMetadataFlags::PATH_INCLUDE_ALL;
		NavigationEnums2D::PathfindingAlgorithm pathfinding_algorithm =
			NavigationEnums2D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
		NavigationEnums2D::PathPostProcessing path_postprocessing =
			NavigationEnums2D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
		bool simplify_path = false;
		real_t simplify_epsilon = 0.0;

		bool exclude_regions = false;
		bool include_regions = false;
		LocalVector<RID> excluded_regions;
		LocalVector<RID> included_regions;

		float path_return_max_length = 0.0;
		float path_return_max_radius = 0.0;
		int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
		float path_search_max_distance = 0.0;

		Vector2 begin_position;
		Vector2 end_position;
		const Nav2D::Polygon* begin_polygon = nullptr;
		const Nav2D::Polygon* end_polygon = nullptr;
		uint32_t least_cost_id = 0;

		NavMap2D* map = nullptr;
		PathQuerySlot* path_query_slot = nullptr;

		LocalVector<Vector2> path_points;
		LocalVector<int32_t> path_meta_point_types;
		LocalVector<RID> path_meta_point_rids;
		LocalVector<int64_t> path_meta_point_owners;
		float path_length = 0.0;

		Ref<NavigationPathQueryParameters2D> query_parameters;
		Ref<NavigationPathQueryResult2D> query_result;
		NavMeshPathQueryTask2D::TaskStatus status =
			NavMeshPathQueryTask2D::TaskStatus::QUERY_STARTED;

		void path_clear()
		{
			path_points.clear();
			path_meta_point_types.clear();
			path_meta_point_rids.clear();
			path_meta_point_owners.clear();
		}

		void path_reverse()
		{
			path_points.reverse();
			path_meta_point_types.reverse();
			path_meta_point_rids.reverse();
			path_meta_point_owners.reverse();
		}
	};

	static Vector2 polygons_get_random_point(const LocalVector<Nav2D::Polygon>& p_polygons,
		uint32_t p_navigation_layers, bool p_uniformly);

	static Vector2 polygons_get_closest_point(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);
	static Nav2D::ClosestPointQueryResult polygons_get_closest_point_info(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);
	static RID polygons_get_closest_point_owner(
		const LocalVector<Nav2D::Polygon>& p_polygons, const Vector2& p_point);

	static Vector2 map_iteration_get_closest_point(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static RID map_iteration_get_closest_point_owner(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static Nav2D::ClosestPointQueryResult map_iteration_get_closest_point_info(
		const NavMapIteration2D& p_map_iteration, const Vector2& p_point);
	static Vector2 map_iteration_get_random_point(
		const NavMapIteration2D& p_map_iteration, uint32_t p_navigation_layers, bool p_uniformly);

	static void query_task_map_iteration_get_path(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_push_back_point_with_metadata(NavMeshPathQueryTask2D& p_query_task,
		const Vector2& p_point, const Nav2D::Polygon* p_point_polygon);
	static void _query_task_find_start_end_positions(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_build_path_corridor(
		NavMeshPathQueryTask2D& p_query_task, const NavMapIteration2D& p_map_iteration);
	static void _query_task_post_process_corridorfunnel(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_post_process_edgecentered(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_post_process_nopostprocessing(NavMeshPathQueryTask2D& p_query_task);
	static void _query_task_clip_path(NavMeshPathQueryTask2D& p_query_task,
		const Nav2D::NavigationPoly* p_from_poly, const Vector2& p_to_point,
		const Nav2D::NavigationPoly* p_to_poly);
	static void _query_task_simplified_path_points(NavMeshPathQueryTask2D& p_query_task);
	static bool _query_task_is_connection_owner_usable(
		const NavMeshPathQueryTask2D& p_query_task, const NavIteration2D* p_owner);
	static void _query_task_process_path_result_limits(NavMeshPathQueryTask2D& p_query_task);

	static void _query_task_search_polygon_connections(NavMeshPathQueryTask2D& p_query_task,
		const Nav2D::Connection& p_connection, uint32_t p_least_cost_id,
		const Nav2D::NavigationPoly& p_least_cost_poly, real_t p_poly_enter_cost,
		const Vector2& p_end_point);

	static void simplify_path_segment(int p_start_inx, int p_end_inx,
		const LocalVector<Vector2>& p_points, real_t p_epsilon,
		LocalVector<uint32_t>& r_simplified_path_indices);
	static LocalVector<uint32_t> get_simplified_path_indices(
		const LocalVector<Vector2>& p_path, real_t p_epsilon);

	static float _calculate_path_length(
		const LocalVector<Vector2>& p_path, uint32_t p_start_index, uint32_t p_end_index);
};

struct NavMapIterationBuild2D
{
	Vector2 merge_rasterizer_cell_size;
	bool use_edge_connections = true;
	real_t edge_connection_margin;
	real_t link_connection_radius;
	Nav2D::PerformanceData performance_data;
	int polygon_count = 0;
	int free_edge_count = 0;

	HashMap<Nav2D::EdgeKey, Nav2D::EdgeConnectionPair, Nav2D::EdgeKey> iter_connection_pairs_map;
	LocalVector<Nav2D::Connection> iter_free_edges;

	NavMapIteration2D* map_iteration = nullptr;

	int navmesh_polygon_count = 0;

	void reset()
	{
		performance_data.reset();

		iter_connection_pairs_map.clear();
		iter_free_edges.clear();
		polygon_count = 0;
		free_edge_count = 0;

		navmesh_polygon_count = 0;
	}
};

struct NavMapIteration2D
{
	mutable SafeNumeric<uint32_t> users;
	RWLock rwlock;

	LocalVector<Ref<NavIteration2D>> region_iterations;
	LocalVector<Ref<NavIteration2D>> link_iterations;

	int navmesh_polygon_count = 0;

	HashMap<const NavIteration2D*, LocalVector<Nav2D::Connection>> external_region_connections;
	HashMap<const NavIteration2D*, LocalVector<LocalVector<Nav2D::Connection>>>
		navbases_polygons_external_connections;

	LocalVector<Nav2D::Polygon> navlink_polygons;

	HashMap<NavRegion2D*, Ref<NavIteration2D>> region_ptr_to_region_iteration;

	LocalVector<NavMeshQueries2D::PathQuerySlot> path_query_slots;
	Mutex path_query_slots_mutex;
	Semaphore path_query_slots_semaphore;

	void clear()
	{
		navmesh_polygon_count = 0;

		region_iterations.clear();
		link_iterations.clear();
		external_region_connections.clear();
		navbases_polygons_external_connections.clear();
		navlink_polygons.clear();
		region_ptr_to_region_iteration.clear();
	}
};

class NavMapIterationRead2D final
{
	const NavMapIteration2D& map_iteration;

public:
	_ALWAYS_INLINE_ NavMapIterationRead2D(const NavMapIteration2D& p_iteration)
		: map_iteration(p_iteration)
	{
		map_iteration.rwlock.read_lock();
		map_iteration.users.increment();
	}

	_ALWAYS_INLINE_ ~NavMapIterationRead2D()
	{
		map_iteration.users.decrement();
		map_iteration.rwlock.read_unlock();
	}
};

class NavMapBuilder2D final
{
	static void _build_step_gather_region_polygons(NavMapIterationBuild2D& r_build);
	static void _build_step_find_edge_connection_pairs(NavMapIterationBuild2D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavMapIterationBuild2D& r_build);
	static void _build_step_edge_connection_margin_connections(NavMapIterationBuild2D& r_build);
	static void _build_step_navlink_connections(NavMapIterationBuild2D& r_build);
	static void _build_update_map_iteration(NavMapIterationBuild2D& r_build);

public:
	static Nav2D::PointKey get_point_key(const Vector2& p_pos, const Vector2& p_cell_size);

	static void build_navmap_iteration(NavMapIterationBuild2D& r_build);
};

class NavRegionBuilder2D final
{
	static void _build_step_process_navmesh_data(NavRegionIterationBuild2D& r_build);
	static void _build_step_find_edge_connection_pairs(NavRegionIterationBuild2D& r_build);
	static void _build_step_merge_edge_connection_pairs(NavRegionIterationBuild2D& r_build);
	static void _build_update_iteration(NavRegionIterationBuild2D& r_build);

public:
	static Nav2D::PointKey get_point_key(const Vector2& p_pos, const Vector2& p_cell_size);
	static Nav2D::EdgeKey get_edge_key(
		const Vector2& p_vertex1, const Vector2& p_vertex2, const Vector2& p_cell_size);

	static void build_iteration(NavRegionIterationBuild2D& r_build);
};

class NavAgent2D final
{
	RID self;

	Vector2 position;
	Vector2 target_position;
	Vector2 velocity;
	Vector2 velocity_forced;
	real_t radius = NavigationDefaults2D::AVOIDANCE_AGENT_RADIUS;
	real_t max_speed = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t time_horizon_agents = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	int max_neighbors = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t neighbor_distance = NavigationDefaults2D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	Vector2 safe_velocity;
	bool clamp_speed = true;

	NavMap2D* map = nullptr;

	RVO2D::Agent2D rvo_agent;
	bool avoidance_enabled = false;

	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;

	bool agent_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavAgent2D> sync_dirty_request_list_element;

public:
	NavAgent2D();
	~NavAgent2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() { return map; }

	bool is_map_changed();

	RVO2D::Agent2D* get_rvo_agent() { return &rvo_agent; }

	bool has_avoidance_callback() const;
	void dispatch_avoidance_callback();

	void set_neighbor_distance(real_t p_neighbor_distance);
	real_t get_neighbor_distance() const { return neighbor_distance; }

	void set_max_neighbors(int p_max_neighbors);
	int get_max_neighbors() const { return max_neighbors; }

	void set_time_horizon_agents(real_t p_time_horizon);
	real_t get_time_horizon_agents() const { return time_horizon_agents; }

	void set_time_horizon_obstacles(real_t p_time_horizon);
	real_t get_time_horizon_obstacles() const { return time_horizon_obstacles; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_max_speed(real_t p_max_speed);
	real_t get_max_speed() const { return max_speed; }

	void set_position(const Vector2& p_position);
	Vector2 get_position() const { return position; }

	void set_target_position(const Vector2& p_target_position);
	Vector2 get_target_position() const { return target_position; }

	void set_velocity(const Vector2& p_velocity);
	Vector2 get_velocity() const { return velocity; }

	void set_velocity_forced(const Vector2& p_velocity);
	Vector2 get_velocity_forced() const { return velocity_forced; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const { return avoidance_layers; }

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const { return avoidance_mask; }

	void set_avoidance_priority(real_t p_priority);
	real_t get_avoidance_priority() const { return avoidance_priority; }

	void set_paused(bool p_paused);
	bool get_paused() const;

	bool is_dirty() const;
	void sync();
	void request_sync();
	void cancel_sync_request();

	void update();

private:
	void _update_rvo_agent_properties();
};

class NavObstacle2D final
{
	RID self;

	NavAgent2D* agent = nullptr;
	NavMap2D* map = nullptr;
	Vector2 velocity;
	Vector2 position;
	Vector<Vector2> vertices;

	real_t radius = 0.0;

	bool avoidance_enabled = false;
	uint32_t avoidance_layers = 1;

	bool obstacle_dirty = true;

	uint32_t last_map_iteration_id = 0;
	bool paused = false;

	SelfList<NavObstacle2D> sync_dirty_request_list_element;

public:
	NavObstacle2D();
	~NavObstacle2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_avoidance_enabled(bool p_enabled);
	bool is_avoidance_enabled() { return avoidance_enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() { return map; }

	void set_agent(NavAgent2D* p_agent);
	NavAgent2D* get_agent() { return agent; }

	void set_position(const Vector2& p_position);
	Vector2 get_position() const { return position; }

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_velocity(const Vector2& p_velocity);
	Vector2 get_velocity() const { return velocity; }

	void set_vertices(const Vector<Vector2>& p_vertices);
	const Vector<Vector2>& get_vertices() const { return vertices; }

	bool is_map_changed();

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const { return avoidance_layers; }

	void set_paused(bool p_paused);
	bool get_paused() const;

	bool is_dirty() const;
	void sync();
	void request_sync();
	void cancel_sync_request();

private:
	void internal_update_agent();
};

class NavLink2D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	NavMap2D* map = nullptr;
	bool bidirectional = true;
	Vector2 start_position;
	Vector2 end_position;
	bool enabled = true;

	SelfList<NavLink2D> sync_dirty_request_list_element;

	uint32_t iteration_id = 0;

	mutable RWLock iteration_rwlock;
	Ref<NavIteration2D> iteration;

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavLink2D();
	~NavLink2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums2D::PathSegmentType get_type() const
	{
		return NavigationEnums2D::PATH_SEGMENT_TYPE_LINK;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() const { return map; }

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_bidirectional(bool p_bidirectional);
	bool is_bidirectional() const { return bidirectional; }

	void set_start_position(Vector2 p_position);
	Vector2 get_start_position() const { return start_position; }

	void set_end_position(Vector2 p_position);
	Vector2 get_end_position() const { return end_position; }

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const { return travel_cost; }

	bool sync();
	void request_sync();
	void cancel_sync_request();

	Ref<NavIteration2D> get_iteration();
};

class NavRegion2D final
{
	RID self;

	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

	RWLock region_rwlock;

	NavMap2D* map = nullptr;
	Transform2D transform;
	bool enabled = true;

	bool use_edge_connections = true;

	Rect2 bounds;

	Ref<NavigationPolygon> navmesh;

	Nav2D::PerformanceData performance_data;

	uint32_t iteration_id = 0;

	SelfList<NavRegion2D> sync_dirty_request_list_element;
	mutable RWLock iteration_rwlock;
	Ref<NavIteration2D> iteration;

	NavRegionIterationBuild2D iteration_build;
	bool use_async_iterations = true;
	SelfList<NavRegion2D> async_list_element;
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavRegion2D();
	~NavRegion2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	NavigationEnums2D::PathSegmentType get_type() const
	{
		return NavigationEnums2D::PATH_SEGMENT_TYPE_REGION;
	}

	uint32_t get_iteration_id() const { return iteration_id; }

	void scratch_polygons();

	void set_enabled(bool p_enabled);
	bool get_enabled() const { return enabled; }

	void set_map(NavMap2D* p_map);
	NavMap2D* get_map() const { return map; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_transform(Transform2D transform);
	const Transform2D& get_transform() const { return transform; }

	void set_navigation_mesh(Ref<NavigationPolygon> p_navigation_mesh);
	Ref<NavigationPolygon> get_navigation_mesh() const { return navmesh; }

	const LocalVector<Nav2D::Polygon>& get_polygons() const;

	Nav2D::ClosestPointQueryResult get_closest_point_info(const Vector2& p_point) const;
	Vector2 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	real_t get_surface_area() const;
	Rect2 get_bounds() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const { return travel_cost; }

	bool sync();
	void request_sync();
	void cancel_sync_request();

	void sync_async_tasks();
	void request_async_thread_join();
	void cancel_async_thread_join();

	Ref<NavIteration2D> get_iteration();

	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;
};

class NavMap2D final
{
	RID self;

	real_t cell_size = NavigationDefaults2D::NAV_MESH_CELL_SIZE;
	Vector2 merge_rasterizer_cell_size = Vector2(cell_size, cell_size);
	float merge_rasterizer_cell_scale = 0.1;

	bool use_edge_connections = true;
	real_t edge_connection_margin = NavigationDefaults2D::EDGE_CONNECTION_MARGIN;
	real_t link_connection_radius = NavigationDefaults2D::LINK_CONNECTION_RADIUS;

	bool map_settings_dirty = true;

	LocalVector<NavRegion2D*> regions;
	LocalVector<NavLink2D*> links;

	RVO2D::RVOSimulator2D rvo_simulation;

	LocalVector<NavAgent2D*> active_avoidance_agents;
	bool agents_dirty = true;
	LocalVector<NavAgent2D*> agents;

	LocalVector<NavObstacle2D*> obstacles;
	bool obstacles_dirty = true;

	uint32_t iteration_id = 0;

	bool use_threads = true;
	bool avoidance_use_multiple_threads = true;
	bool avoidance_use_high_priority_threads = true;

	Nav2D::PerformanceData performance_data;

	int path_query_slots_max = 4;

	bool use_async_iterations = true;

	uint32_t iteration_slot_index = 0;
	LocalVector<NavMapIteration2D> iteration_slots;
	mutable RWLock iteration_slot_rwlock;

	NavMapIterationBuild2D iteration_build;
	static void _build_iteration_threaded(void* p_arg);

	bool iteration_dirty = true;
	bool iteration_building = false;
	bool iteration_ready = false;

	void _build_iteration();
	void _sync_iteration();

public:
	NavMap2D();
	~NavMap2D();

	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	uint32_t get_iteration_id() const { return iteration_id; }

	void set_cell_size(real_t p_cell_size);
	real_t get_cell_size() const { return cell_size; }

	void set_merge_rasterizer_cell_scale(float p_value);
	float get_merge_rasterizer_cell_scale() const { return merge_rasterizer_cell_scale; }

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const { return use_edge_connections; }

	void set_edge_connection_margin(real_t p_edge_connection_margin);
	real_t get_edge_connection_margin() const { return edge_connection_margin; }

	void set_link_connection_radius(real_t p_link_connection_radius);
	real_t get_link_connection_radius() const { return link_connection_radius; }

	Nav2D::PointKey get_point_key(const Vector2& p_pos) const;
	const Vector2& get_merge_rasterizer_cell_size() const;

	void query_path(NavMeshQueries2D::NavMeshPathQueryTask2D& p_query_task);

	Vector2 get_closest_point(const Vector2& p_point) const;
	Nav2D::ClosestPointQueryResult get_closest_point_info(const Vector2& p_point) const;
	RID get_closest_point_owner(const Vector2& p_point) const;

	void add_region(NavRegion2D* p_region);
	void remove_region(NavRegion2D* p_region);
	const LocalVector<NavRegion2D*>& get_regions() const { return regions; }

	void add_link(NavLink2D* p_link);
	void remove_link(NavLink2D* p_link);
	const LocalVector<NavLink2D*>& get_links() const { return links; }

	bool has_agent(NavAgent2D* p_agent) const;
	void add_agent(NavAgent2D* p_agent);
	void remove_agent(NavAgent2D* p_agent);
	const LocalVector<NavAgent2D*>& get_agents() const { return agents; }

	void set_agent_as_controlled(NavAgent2D* p_agent);
	void remove_agent_as_controlled(NavAgent2D* p_agent);

	bool has_obstacle(NavObstacle2D* p_obstacle) const;
	void add_obstacle(NavObstacle2D* p_obstacle);
	void remove_obstacle(NavObstacle2D* p_obstacle);
	const LocalVector<NavObstacle2D*>& get_obstacles() const { return obstacles; }

	Vector2 get_random_point(uint32_t p_navigation_layers, bool p_uniformly) const;

	void sync();
	void step(double p_delta_time);
	void dispatch_callbacks();

	int get_pm_region_count() const { return performance_data.pm_region_count; }
	int get_pm_agent_count() const { return performance_data.pm_agent_count; }
	int get_pm_link_count() const { return performance_data.pm_link_count; }
	int get_pm_polygon_count() const { return performance_data.pm_polygon_count; }
	int get_pm_edge_count() const { return performance_data.pm_edge_count; }
	int get_pm_edge_merge_count() const { return performance_data.pm_edge_merge_count; }
	int get_pm_edge_connection_count() const { return performance_data.pm_edge_connection_count; }
	int get_pm_edge_free_count() const { return performance_data.pm_edge_free_count; }
	int get_pm_obstacle_count() const { return performance_data.pm_obstacle_count; }

	int get_region_connections_count(NavRegion2D* p_region) const;
	Vector2 get_region_connection_pathway_start(NavRegion2D* p_region, int p_connection_id) const;
	Vector2 get_region_connection_pathway_end(NavRegion2D* p_region, int p_connection_id) const;

	void set_use_async_iterations(bool p_enabled);
	bool get_use_async_iterations() const;

private:
	void _sync_dirty_map_update_requests();
	void _sync_dirty_avoidance_update_requests();
	void _sync_async_tasks();

	void compute_single_step(uint32_t p_index, NavAgent2D** p_agent);
	void compute_single_avoidance_step(uint32_t p_index, NavAgent2D** p_agent);

	void _sync_avoidance();
	void _update_rvo_simulation();
	void _update_rvo_obstacles_tree();
	void _update_rvo_agents_tree();

	void _update_merge_rasterizer_cell_dimensions();
};

#ifdef CLIPPER2_ENABLED

class Node;

struct NavMeshGeometryParser2D
{
	RID self;
};

class NavMeshGenerator2D final
{
	static inline bool _initialized = false;

	static Mutex baking_navmesh_mutex;
	static Mutex generator_task_mutex;

	static RWLock generator_parsers_rwlock;
	static LocalVector<NavMeshGeometryParser2D*> generator_parsers;

	static bool use_threads;
	static bool baking_use_multiple_threads;
	static bool baking_use_high_priority_threads;

	struct NavMeshGeneratorTask2D
	{
		enum TaskStatus
		{
			BAKING_STARTED,
			BAKING_FINISHED,
			BAKING_FAILED,
			CALLBACK_DISPATCHED,
			CALLBACK_FAILED,
		};

		Ref<NavigationPolygon> navigation_mesh;
		Ref<NavigationMeshSourceGeometryData2D> source_geometry_data;
		NavMeshGeneratorTask2D::TaskStatus status =
			NavMeshGeneratorTask2D::TaskStatus::BAKING_STARTED;
	};

	static void generator_thread_bake(void* p_arg);

	static HashSet<Ref<NavigationPolygon>> baking_navmeshes;

	static void generator_parse_geometry_node(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node,
		bool p_recurse_children);
	static void generator_parse_source_geometry_data(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_root_node);
	static void generator_bake_from_source_geometry_data(Ref<NavigationPolygon> p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return _initialized; }

	static void sync();
	static void cleanup();
	static void finish();

	static void set_generator_parsers(const LocalVector<NavMeshGeometryParser2D*>& p_parsers);

	static bool is_baking(Ref<NavigationPolygon> p_navigation_polygon);

	NavMeshGenerator2D() = delete;
	NavMeshGenerator2D(const NavMeshGenerator2D&) = delete;
	NavMeshGenerator2D& operator=(const NavMeshGenerator2D&) = delete;
	~NavMeshGenerator2D() = delete;
};

#endif // CLIPPER2_ENABLED
// --- End modules/navigation_2d/nav_map_2d.h ---
// --- End servers/navigation_2d/navigation_server_2d.h ---
