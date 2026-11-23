/**************************************************************************/
/*  jolt_physics_server_3d.h                                              */
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

#include "core/templates/rid_owner.h"
#include "servers/physics_3d/physics_server_3d.h"

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

#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/plane.h"
#include "core/math/transform_3d.h"
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
