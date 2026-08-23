/**************************************************************************/
/*  generic_6dof_joint_3d.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include <cfloat> // FLT_MAX
#include "core/object/class_db.h"
#include "generic_6dof_joint_3d.h"

void Generic6DOFJoint3D::_bind_methods() {}

void Generic6DOFJoint3D::set_param_x(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	params_x[p_param] = p_value;
	if (p_param == PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT) {
		has_angular_target_rotation = false;
	}
	if (!setting_default_params) {
		_set_drive_limit_explicit(Vector3::AXIS_X, p_param);
		_warn_if_deprecated_param(p_param);
	}
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
			get_rid(), Vector3::AXIS_X, PS3DE::G6DOFJointAxisParam(p_param), p_value);
	}

	update_gizmos();
}

real_t Generic6DOFJoint3D::get_param_x(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return params_x[p_param];
}

void Generic6DOFJoint3D::set_param_y(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	params_y[p_param] = p_value;
	if (p_param == PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT) {
		has_angular_target_rotation = false;
	}
	if (!setting_default_params) {
		_set_drive_limit_explicit(Vector3::AXIS_Y, p_param);
		_warn_if_deprecated_param(p_param);
	}
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
			get_rid(), Vector3::AXIS_Y, PS3DE::G6DOFJointAxisParam(p_param), p_value);
	}
	update_gizmos();
}

real_t Generic6DOFJoint3D::get_param_y(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return params_y[p_param];
}

void Generic6DOFJoint3D::set_param_z(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	params_z[p_param] = p_value;
	if (p_param == PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT) {
		has_angular_target_rotation = false;
	}
	if (!setting_default_params) {
		_set_drive_limit_explicit(Vector3::AXIS_Z, p_param);
		_warn_if_deprecated_param(p_param);
	}
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
			get_rid(), Vector3::AXIS_Z, PS3DE::G6DOFJointAxisParam(p_param), p_value);
	}
	update_gizmos();
}

void Generic6DOFJoint3D::_warn_if_deprecated_param(Param p_param)
{
	if (p_param == PARAM_LINEAR_MOTOR_FORCE_LIMIT) {
		WARN_PRINT_ONCE(
			"PARAM_LINEAR_MOTOR_FORCE_LIMIT is deprecated and will be removed in a future release. "
			"Use PARAM_LINEAR_DRIVE_FORCE_LIMIT, which applies in both spring and motor modes.");
	}
	else if (p_param == PARAM_ANGULAR_MOTOR_FORCE_LIMIT) {
		WARN_PRINT_ONCE("PARAM_ANGULAR_MOTOR_FORCE_LIMIT is deprecated and will be removed in a "
						"future release. Use PARAM_ANGULAR_DRIVE_TORQUE_LIMIT, which applies in "
						"both spring and motor modes.");
	}
}

void Generic6DOFJoint3D::_set_drive_limit_explicit(Vector3::Axis p_axis, Param p_param)
{
	if (p_param == PARAM_LINEAR_DRIVE_FORCE_LIMIT) {
		linear_drive_force_limit_set[p_axis] = true;
	}
	else if (p_param == PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) {
		angular_drive_torque_limit_set[p_axis] = true;
	}
}

bool Generic6DOFJoint3D::_should_replay_param(Vector3::Axis p_axis, Param p_param) const
{
	if (p_param == PARAM_LINEAR_DRIVE_FORCE_LIMIT) {
		return linear_drive_force_limit_set[p_axis];
	}
	else if (p_param == PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) {
		return angular_drive_torque_limit_set[p_axis];
	}
	return true;
}

real_t Generic6DOFJoint3D::get_param_z(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return params_z[p_param];
}

void Generic6DOFJoint3D::set_flag_x(Flag p_flag, bool p_enabled)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags_x[p_flag] = p_enabled;
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			get_rid(), Vector3::AXIS_X, PS3DE::G6DOFJointAxisFlag(p_flag), p_enabled);
	}
	update_gizmos();
}

bool Generic6DOFJoint3D::get_flag_x(Flag p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags_x[p_flag];
}

void Generic6DOFJoint3D::set_flag_y(Flag p_flag, bool p_enabled)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags_y[p_flag] = p_enabled;
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			get_rid(), Vector3::AXIS_Y, PS3DE::G6DOFJointAxisFlag(p_flag), p_enabled);
	}
	update_gizmos();
}

bool Generic6DOFJoint3D::get_flag_y(Flag p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags_y[p_flag];
}

void Generic6DOFJoint3D::set_flag_z(Flag p_flag, bool p_enabled)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags_z[p_flag] = p_enabled;
	if (is_configured()) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			get_rid(), Vector3::AXIS_Z, PS3DE::G6DOFJointAxisFlag(p_flag), p_enabled);
	}
	update_gizmos();
}

bool Generic6DOFJoint3D::get_flag_z(Flag p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags_z[p_flag];
}

bool Generic6DOFJoint3D::_is_valid_angular_target_rotation(const Quaternion& p_target_rotation)
{
	return p_target_rotation.is_finite() && p_target_rotation.length_squared() > CMP_EPSILON;
}

void Generic6DOFJoint3D::set_angular_target_rotation(const Quaternion& p_target_rotation)
{
	ERR_FAIL_COND_MSG(!_is_valid_angular_target_rotation(p_target_rotation),
		"Angular target rotation must be a finite, non-zero quaternion.");

	angular_target_rotation = p_target_rotation.normalized();
	has_angular_target_rotation = true;

	if (!is_configured()) {
		return;
	}

	PhysicsServer3D::get_singleton()->generic_6dof_joint_set_angular_target_rotation(
		get_rid(), angular_target_rotation);
}

Quaternion Generic6DOFJoint3D::get_angular_target_rotation() const
{
	if (has_angular_target_rotation) {
		return angular_target_rotation;
	}

	if (is_configured()) {
		return PhysicsServer3D::get_singleton()->generic_6dof_joint_get_angular_target_rotation(
			get_rid());
	}

	// Equilibrium point is in constraint space; body-space conversion requires the constraint
	// frame.
	ERR_PRINT(
		"Cannot derive a body-space angular target rotation from Generic6DOFJoint3D equilibrium "
		"points before the joint is configured. Returning the identity quaternion.");
	return Quaternion();
}

bool Generic6DOFJoint3D::has_target_rotation() const { return has_angular_target_rotation; }

void Generic6DOFJoint3D::clear_angular_target_rotation()
{
	if (!has_angular_target_rotation) {
		return;
	}
	has_angular_target_rotation = false;
	angular_target_rotation = Quaternion();

	if (!is_configured()) {
		return;
	}

	PhysicsServer3D* server = PhysicsServer3D::get_singleton();
	server->generic_6dof_joint_set_param(get_rid(), Vector3::AXIS_X,
		PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
		params_x[PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
	server->generic_6dof_joint_set_param(get_rid(), Vector3::AXIS_Y,
		PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
		params_y[PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
	server->generic_6dof_joint_set_param(get_rid(), Vector3::AXIS_Z,
		PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
		params_z[PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
}

void Generic6DOFJoint3D::_configure_joint(RID p_joint, PhysicsBody3D* body_a, PhysicsBody3D* body_b)
{
	Transform3D gt = get_global_transform();
	// Vector3 cone_twistpos = gt.origin;
	// Vector3 cone_twistdir = gt.basis.get_axis(2);

	Transform3D ainv = body_a->get_global_transform().affine_inverse();

	Transform3D local_a = ainv * gt;
	local_a.orthonormalize();
	Transform3D local_b = gt;

	if (body_b) {
		Transform3D binv = body_b->get_global_transform().affine_inverse();
		local_b = binv * gt;
	}

	local_b.orthonormalize();

	PhysicsServer3D::get_singleton()->joint_make_generic_6dof(
		p_joint, body_a->get_rid(), local_a, body_b ? body_b->get_rid() : RID(), local_b);
	for (int i = 0; i < PARAM_MAX; i++) {
		const Param param = static_cast<Param>(i);
		if (_should_replay_param(Vector3::AXIS_X, param)) {
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
				p_joint, Vector3::AXIS_X, PS3DE::G6DOFJointAxisParam(i), params_x[i]);
		}
		if (_should_replay_param(Vector3::AXIS_Y, param)) {
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
				p_joint, Vector3::AXIS_Y, PS3DE::G6DOFJointAxisParam(i), params_y[i]);
		}
		if (_should_replay_param(Vector3::AXIS_Z, param)) {
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(
				p_joint, Vector3::AXIS_Z, PS3DE::G6DOFJointAxisParam(i), params_z[i]);
		}
	}
	for (int i = 0; i < FLAG_MAX; i++) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			p_joint, Vector3::AXIS_X, PS3DE::G6DOFJointAxisFlag(i), flags_x[i]);
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			p_joint, Vector3::AXIS_Y, PS3DE::G6DOFJointAxisFlag(i), flags_y[i]);
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(
			p_joint, Vector3::AXIS_Z, PS3DE::G6DOFJointAxisFlag(i), flags_z[i]);
	}

	if (has_angular_target_rotation) {
		PhysicsServer3D::get_singleton()->generic_6dof_joint_set_angular_target_rotation(
			p_joint, angular_target_rotation);
	}
}

Generic6DOFJoint3D::Generic6DOFJoint3D()
{
	set_param_x(PARAM_LINEAR_LOWER_LIMIT, 0);
	set_param_x(PARAM_LINEAR_UPPER_LIMIT, 0);
	set_param_x(PARAM_LINEAR_LIMIT_SOFTNESS, 0.7);
	set_param_x(PARAM_LINEAR_RESTITUTION, 0.5);
	set_param_x(PARAM_LINEAR_DAMPING, 1.0);
	set_param_x(PARAM_LINEAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_x(PARAM_LINEAR_MOTOR_FORCE_LIMIT, 0);
	set_param_x(PARAM_LINEAR_SPRING_STIFFNESS, 0.01);
	set_param_x(PARAM_LINEAR_SPRING_DAMPING, 0.01);
	set_param_x(PARAM_LINEAR_SPRING_EQUILIBRIUM_POINT, 0.0);
	set_param_x(PARAM_LINEAR_DRIVE_FORCE_LIMIT, FLT_MAX);
	set_param_x(PARAM_ANGULAR_LOWER_LIMIT, 0);
	set_param_x(PARAM_ANGULAR_UPPER_LIMIT, 0);
	set_param_x(PARAM_ANGULAR_LIMIT_SOFTNESS, 0.5f);
	set_param_x(PARAM_ANGULAR_DAMPING, 1.0f);
	set_param_x(PARAM_ANGULAR_RESTITUTION, 0);
	set_param_x(PARAM_ANGULAR_FORCE_LIMIT, 0);
	set_param_x(PARAM_ANGULAR_ERP, 0.5);
	set_param_x(PARAM_ANGULAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_x(PARAM_ANGULAR_MOTOR_FORCE_LIMIT, 300);
	set_param_x(PARAM_ANGULAR_SPRING_STIFFNESS, 0);
	set_param_x(PARAM_ANGULAR_SPRING_DAMPING, 0);
	set_param_x(PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0);
	set_param_x(PARAM_ANGULAR_DRIVE_TORQUE_LIMIT, FLT_MAX);

	set_flag_x(FLAG_ENABLE_ANGULAR_LIMIT, true);
	set_flag_x(FLAG_ENABLE_LINEAR_LIMIT, true);
	set_flag_x(FLAG_ENABLE_ANGULAR_SPRING, false);
	set_flag_x(FLAG_ENABLE_LINEAR_SPRING, false);
	set_flag_x(FLAG_ENABLE_ANGULAR_MOTOR, false);
	set_flag_x(FLAG_ENABLE_LINEAR_MOTOR, false);

	set_param_y(PARAM_LINEAR_LOWER_LIMIT, 0);
	set_param_y(PARAM_LINEAR_UPPER_LIMIT, 0);
	set_param_y(PARAM_LINEAR_LIMIT_SOFTNESS, 0.7);
	set_param_y(PARAM_LINEAR_RESTITUTION, 0.5);
	set_param_y(PARAM_LINEAR_DAMPING, 1.0);
	set_param_y(PARAM_LINEAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_y(PARAM_LINEAR_MOTOR_FORCE_LIMIT, 0);
	set_param_y(PARAM_LINEAR_SPRING_STIFFNESS, 0.01);
	set_param_y(PARAM_LINEAR_SPRING_DAMPING, 0.01);
	set_param_y(PARAM_LINEAR_SPRING_EQUILIBRIUM_POINT, 0.0);
	set_param_y(PARAM_LINEAR_DRIVE_FORCE_LIMIT, FLT_MAX);
	set_param_y(PARAM_ANGULAR_LOWER_LIMIT, 0);
	set_param_y(PARAM_ANGULAR_UPPER_LIMIT, 0);
	set_param_y(PARAM_ANGULAR_LIMIT_SOFTNESS, 0.5f);
	set_param_y(PARAM_ANGULAR_DAMPING, 1.0f);
	set_param_y(PARAM_ANGULAR_RESTITUTION, 0);
	set_param_y(PARAM_ANGULAR_FORCE_LIMIT, 0);
	set_param_y(PARAM_ANGULAR_ERP, 0.5);
	set_param_y(PARAM_ANGULAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_y(PARAM_ANGULAR_MOTOR_FORCE_LIMIT, 300);
	set_param_y(PARAM_ANGULAR_SPRING_STIFFNESS, 0);
	set_param_y(PARAM_ANGULAR_SPRING_DAMPING, 0);
	set_param_y(PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0);
	set_param_y(PARAM_ANGULAR_DRIVE_TORQUE_LIMIT, FLT_MAX);

	set_flag_y(FLAG_ENABLE_ANGULAR_LIMIT, true);
	set_flag_y(FLAG_ENABLE_LINEAR_LIMIT, true);
	set_flag_y(FLAG_ENABLE_ANGULAR_SPRING, false);
	set_flag_y(FLAG_ENABLE_LINEAR_SPRING, false);
	set_flag_y(FLAG_ENABLE_ANGULAR_MOTOR, false);
	set_flag_y(FLAG_ENABLE_LINEAR_MOTOR, false);

	set_param_z(PARAM_LINEAR_LOWER_LIMIT, 0);
	set_param_z(PARAM_LINEAR_UPPER_LIMIT, 0);
	set_param_z(PARAM_LINEAR_LIMIT_SOFTNESS, 0.7);
	set_param_z(PARAM_LINEAR_RESTITUTION, 0.5);
	set_param_z(PARAM_LINEAR_DAMPING, 1.0);
	set_param_z(PARAM_LINEAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_z(PARAM_LINEAR_MOTOR_FORCE_LIMIT, 0);
	set_param_z(PARAM_LINEAR_SPRING_STIFFNESS, 0.01);
	set_param_z(PARAM_LINEAR_SPRING_DAMPING, 0.01);
	set_param_z(PARAM_LINEAR_SPRING_EQUILIBRIUM_POINT, 0.0);
	set_param_z(PARAM_LINEAR_DRIVE_FORCE_LIMIT, FLT_MAX);
	set_param_z(PARAM_ANGULAR_LOWER_LIMIT, 0);
	set_param_z(PARAM_ANGULAR_UPPER_LIMIT, 0);
	set_param_z(PARAM_ANGULAR_LIMIT_SOFTNESS, 0.5f);
	set_param_z(PARAM_ANGULAR_DAMPING, 1.0f);
	set_param_z(PARAM_ANGULAR_RESTITUTION, 0);
	set_param_z(PARAM_ANGULAR_FORCE_LIMIT, 0);
	set_param_z(PARAM_ANGULAR_ERP, 0.5);
	set_param_z(PARAM_ANGULAR_MOTOR_TARGET_VELOCITY, 0);
	set_param_z(PARAM_ANGULAR_MOTOR_FORCE_LIMIT, 300);
	set_param_z(PARAM_ANGULAR_SPRING_STIFFNESS, 0);
	set_param_z(PARAM_ANGULAR_SPRING_DAMPING, 0);
	set_param_z(PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0);
	set_param_z(PARAM_ANGULAR_DRIVE_TORQUE_LIMIT, FLT_MAX);

	set_flag_z(FLAG_ENABLE_ANGULAR_LIMIT, true);
	set_flag_z(FLAG_ENABLE_LINEAR_LIMIT, true);
	set_flag_z(FLAG_ENABLE_ANGULAR_SPRING, false);
	set_flag_z(FLAG_ENABLE_LINEAR_SPRING, false);
	set_flag_z(FLAG_ENABLE_ANGULAR_MOTOR, false);
	set_flag_z(FLAG_ENABLE_LINEAR_MOTOR, false);

	setting_default_params = false;
}


