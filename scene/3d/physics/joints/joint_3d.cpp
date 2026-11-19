/**************************************************************************/
/*  joint_3d.cpp                                                          */
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

#include "joint_3d.h"

void Joint3D::_init_defaults() {
	pin_params[PS3DE::PIN_JOINT_BIAS] = 0.3f;
	pin_params[PS3DE::PIN_JOINT_DAMPING] = 1.0f;
	pin_params[PS3DE::PIN_JOINT_IMPULSE_CLAMP] = 0.0f;

	hinge_params[PS3DE::HINGE_JOINT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_UPPER] = Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_LOWER] = -Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_SOFTNESS] = 0.9f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_RELAXATION] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE] = 1.0f;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_USE_LIMIT] = false;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR] = false;

	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER] = -1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING] = 1.0f;

	cone_params[PS3DE::CONE_TWIST_JOINT_SWING_SPAN] = Math::PI * 0.25f;
	cone_params[PS3DE::CONE_TWIST_JOINT_TWIST_SPAN] = Math::PI;
	cone_params[PS3DE::CONE_TWIST_JOINT_BIAS] = 0.3f;
	cone_params[PS3DE::CONE_TWIST_JOINT_SOFTNESS] = 0.8f;
	cone_params[PS3DE::CONE_TWIST_JOINT_RELAXATION] = 1.0f;

	for (int ax = 0; ax < 3; ax++) {
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS] = 0.7f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_ERP] = 0.5f;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT] = true;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT] = true;
	}
}

Joint3D::Joint3D(JointType p_type) :
		joint_type(p_type) {
	_init_defaults();
	set_notify_transform(true);
	joint = PhysicsServer3D::joint_create();
}

Joint3D::~Joint3D() {
	if (joint.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(joint);
	}
}

void Joint3D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	joint_type = p_type;
	_update_joint();
}

void Joint3D::_disconnect_signals() {}

void Joint3D::_update_joint(bool p_only_free) {
	if (configured) {
		PhysicsServer3D::joint_clear(joint);
		ba = RID();
		bb = RID();
		configured = false;
	}

	if (p_only_free || !is_inside_tree()) {
		warning = String();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody3D *body_a = dynamic_cast<PhysicsBody3D *>(node_a);
	PhysicsBody3D *body_b = dynamic_cast<PhysicsBody3D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a) {
		warning = RTR("Node A and Node B must be PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	if (body_a == body_b) {
		warning = RTR("Node A and Node B must be different PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	warning = String();
	update_configuration_warnings();

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();

	_configure_joint(joint, body_a, body_b);
	PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	PhysicsServer3D::joint_disable_collisions_between_bodies(joint, exclude_from_collision);
	configured = true;
}

bool Joint3D::_should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		return linear_drive_force_limit_set[p_axis];
	}
	if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		return angular_drive_torque_limit_set[p_axis];
	}
	return true;
}

void Joint3D::_configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b) {
	Transform3D gt = get_global_transform();

	if (joint_type == JOINT_PIN) {
		Vector3 pinpos = gt.origin;
		Vector3 local_a = body_a->to_local(pinpos);
		Vector3 local_b = body_b ? body_b->to_local(pinpos) : pinpos;
		PhysicsServer3D::joint_make_pin(p_joint, body_a->get_rid(), local_a, body_b ? body_b->get_rid() : RID(), local_b);
		for (int i = 0; i < 3; i++) {
			PhysicsServer3D::pin_joint_set_param(p_joint, PinParam(i), pin_params[i]);
		}
		return;
	}

	Transform3D local_a = (body_a->get_global_transform().affine_inverse() * gt).orthonormalized();
	Transform3D local_b = (body_b ? (body_b->get_global_transform().affine_inverse() * gt) : gt).orthonormalized();
	RID rid_b = body_b ? body_b->get_rid() : RID();

	switch (joint_type) {
		case JOINT_HINGE: {
			PhysicsServer3D::joint_make_hinge(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::HINGE_JOINT_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_param(p_joint, HingeParam(i), hinge_params[i]);
			}
			for (int i = 0; i < PS3DE::HINGE_JOINT_FLAG_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_flag(p_joint, HingeFlag(i), hinge_flags[i]);
			}
		} break;
		case JOINT_SLIDER: {
			PhysicsServer3D::joint_make_slider(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::SLIDER_JOINT_MAX; i++) {
				PhysicsServer3D::slider_joint_set_param(p_joint, SliderParam(i), slider_params[i]);
			}
		} break;
		case JOINT_CONE_TWIST: {
			PhysicsServer3D::joint_make_cone_twist(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < CONE_TWIST_PARAM_MAX; i++) {
				PhysicsServer3D::cone_twist_joint_set_param(p_joint, ConeTwistParam(i), cone_params[i]);
			}
		} break;
		case JOINT_6DOF: {
			PhysicsServer3D::joint_make_generic_6dof(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int ax = 0; ax < 3; ax++) {
				Vector3::Axis axis = Vector3::Axis(ax);
				for (int i = 0; i < PS3DE::G6DOF_JOINT_MAX; i++) {
					G6DOFParam param = G6DOFParam(i);
					if (_should_replay_g6dof_param(axis, param)) {
						PhysicsServer3D::generic_6dof_joint_set_param(p_joint, axis, param, g6dof_params[ax][i]);
					}
				}
				for (int i = 0; i < PS3DE::G6DOF_JOINT_FLAG_MAX; i++) {
					PhysicsServer3D::generic_6dof_joint_set_flag(p_joint, axis, G6DOFFlag(i), g6dof_flags[ax][i]);
				}
			}
			if (has_angular_target_rotation) {
				PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(p_joint, angular_target_rotation);
			}
		} break;
		default:
			break;
	}
}

void Joint3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (is_inside_tree()) {
				_update_joint();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
	}
}

void Joint3D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

void Joint3D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

void Joint3D::set_solver_priority(int p_priority) {
	solver_priority = p_priority;
	if (joint.is_valid()) {
		PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	}
}

void Joint3D::set_exclude_nodes_from_collision(bool p_enable) {
	if (exclude_from_collision == p_enable) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	exclude_from_collision = p_enable;
	_update_joint();
}

void Joint3D::set_pin_param(PinParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, 3);
	pin_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer3D::pin_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_pin_param(PinParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, 3, 0.0f);
	return pin_params[p_param];
}

void Joint3D::set_hinge_param(HingeParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::HINGE_JOINT_MAX);
	hinge_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_hinge_param(HingeParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::HINGE_JOINT_MAX, 0.0f);
	return hinge_params[p_param];
}

void Joint3D::set_hinge_flag(HingeFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX);
	hinge_flags[p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_flag(joint, p_flag, p_enabled);
	}
}

bool Joint3D::get_hinge_flag(HingeFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX, false);
	return hinge_flags[p_flag];
}

void Joint3D::set_slider_param(SliderParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::SLIDER_JOINT_MAX);
	slider_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_SLIDER) {
		PhysicsServer3D::slider_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_slider_param(SliderParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::SLIDER_JOINT_MAX, 0.0f);
	return slider_params[p_param];
}

void Joint3D::set_cone_twist_param(ConeTwistParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, CONE_TWIST_PARAM_MAX);
	cone_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_CONE_TWIST) {
		PhysicsServer3D::cone_twist_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_cone_twist_param(ConeTwistParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, CONE_TWIST_PARAM_MAX, 0.0f);
	return cone_params[p_param];
}

void Joint3D::set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_param, PS3DE::G6DOF_JOINT_MAX);
	g6dof_params[p_axis][p_param] = p_value;
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		linear_drive_force_limit_set[p_axis] = true;
	} else if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		angular_drive_torque_limit_set[p_axis] = true;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_param(joint, p_axis, p_param, p_value);
	}
}

real_t Joint3D::get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	ERR_FAIL_INDEX_V(p_axis, 3, 0.0f);
	ERR_FAIL_INDEX_V(p_param, PS3DE::G6DOF_JOINT_MAX, 0.0f);
	return g6dof_params[p_axis][p_param];
}

void Joint3D::set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX);
	g6dof_flags[p_axis][p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_flag(joint, p_axis, p_flag, p_enabled);
	}
}

bool Joint3D::get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_axis, 3, false);
	ERR_FAIL_INDEX_V(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX, false);
	return g6dof_flags[p_axis][p_flag];
}

void Joint3D::set_angular_target_rotation(const Quaternion &p_target_rotation) {
	ERR_FAIL_COND_MSG(!(p_target_rotation.is_finite() && p_target_rotation.length_squared() > CMP_EPSILON),
			"Angular target rotation must be a finite, non-zero quaternion.");
	angular_target_rotation = p_target_rotation.normalized();
	has_angular_target_rotation = true;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(joint, angular_target_rotation);
	}
}

Quaternion Joint3D::get_angular_target_rotation() const {
	if (has_angular_target_rotation) {
		return angular_target_rotation;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		return PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(joint);
	}
	return Quaternion();
}

void Joint3D::clear_angular_target_rotation() {
	if (!has_angular_target_rotation) {
		return;
	}
	has_angular_target_rotation = false;
	angular_target_rotation = Quaternion();
	if (is_configured() && joint_type == JOINT_6DOF) {
		for (int ax = 0; ax < 3; ax++) {
			PhysicsServer3D::generic_6dof_joint_set_param(joint, Vector3::Axis(ax),
					PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
					g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
		}
	}
}

PackedStringArray Joint3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}
