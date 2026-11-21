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
#include "servers/physics_2d/physics_server_2d_types.h"
// --- Folded from servers/physics_2d/queries/physics_ray_query_parameters_2d.h ---
#include "core/types.h"
#include "servers/physics_2d/physics_server_2d_types.h"

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
#include "servers/physics_2d/physics_server_2d_types.h"

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
#include "servers/physics_2d/physics_server_2d_types.h"

// --- Folded from servers/physics_2d/queries/physics_testmotion_query_parameters_2d.h ---
#include "core/types.h"
#include "servers/physics_2d/physics_server_2d_types.h"

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
#include "servers/physics_2d/physics_server_2d_types.h"
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
