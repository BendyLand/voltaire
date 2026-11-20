/**************************************************************************/
/*  jolt_joint_3d.h                                                       */
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

#include "../jolt_physics_server_3d.h"

#include "core/math/quaternion.h"
#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "servers/physics_3d/physics_server_3d_enums.h"

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
