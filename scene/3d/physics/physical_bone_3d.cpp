/**************************************************************************/
/*  physical_bone_3d.cpp                                                  */
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

#include "core/config/engine.h"
#include "physical_bone_3d.h"
#include "scene/3d/physics/physical_bone_simulator_3d.h"

#ifndef DISABLE_DEPRECATED
#include "scene/3d/skeleton_3d.h"
#endif //_DISABLE_DEPRECATED

void PhysicalBone3D::apply_central_impulse(const Vector3& p_impulse)
{
	PhysicsServer3D::get_singleton()->body_apply_central_impulse(get_rid(), p_impulse);
}

void PhysicalBone3D::apply_impulse(const Vector3& p_impulse, const Vector3& p_position)
{
	PhysicsServer3D::get_singleton()->body_apply_impulse(get_rid(), p_impulse, p_position);
}

Vector3 PhysicalBone3D::get_linear_velocity() const { return linear_velocity; }

Vector3 PhysicalBone3D::get_angular_velocity() const { return angular_velocity; }

void PhysicalBone3D::set_use_custom_integrator(bool p_enable)
{
	if (custom_integrator == p_enable) {
		return;
	}

	custom_integrator = p_enable;
	PhysicsServer3D::get_singleton()->body_set_omit_force_integration(get_rid(), p_enable);
}

bool PhysicalBone3D::is_using_custom_integrator() { return custom_integrator; }

void PhysicalBone3D::reset_physics_simulation_state()
{
	if (simulate_physics) {
		_start_physics_simulation();
	}
	else {
		_stop_physics_simulation();
	}
}

void PhysicalBone3D::reset_to_rest_position()
{
	PhysicalBoneSimulator3D* simulator = get_simulator();
	Skeleton3D* skeleton = get_skeleton();
	if (simulator && skeleton) {
		if (bone_id == -1) {
			set_global_transform(
				(skeleton->get_global_transform() * body_offset).orthonormalized());
		}
		else {
			set_global_transform((skeleton->get_global_transform() *
								  simulator->get_bone_global_pose(bone_id) * body_offset)
									 .orthonormalized());
		}
	}
}

void PhysicalBone3D::_notification(int p_what)
{
	switch (p_what) {
	// We need to wait until the bone has finished being added to the tree
	// or none of the global transform calls will work correctly.
	case NOTIFICATION_POST_ENTER_TREE:
		_update_simulator_path();
		update_bone_id();
		reset_to_rest_position();
		reset_physics_simulation_state();
		if (joint_data) {
			_reload_joint();
		}
		break;

	// If we're detached from the skeleton we need to
	// clear our references to it.
	case NOTIFICATION_UNPARENTED:
	case NOTIFICATION_EXIT_TREE: {
		PhysicalBoneSimulator3D* simulator = get_simulator();
		if (simulator) {
			if (bone_id != -1) {
				simulator->unbind_physical_bone_from_bone(bone_id);
				bone_id = -1;
			}
		}
		PhysicsServer3D::get_singleton()->joint_clear(joint);
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (Engine::get_singleton()->is_editor_hint()) {
			update_offset();
		}
	} break;
	}
}

void PhysicalBone3D::_sync_body_state(PhysicsDirectBodyState3D* p_state)
{
	Transform3D new_transform = p_state->get_transform();
	if (likely(new_transform != get_global_transform())) {
		set_ignore_transform_notification(true);
		set_global_transform(new_transform);
		set_ignore_transform_notification(false);
	}

	linear_velocity = p_state->get_linear_velocity();
	angular_velocity = p_state->get_angular_velocity();
}

void PhysicalBone3D::_body_state_changed(PhysicsDirectBodyState3D* p_state)
{
	if (!simulate_physics || !_internal_simulate_physics) {
		return;
	}

	_sync_body_state(p_state);
	_on_transform_changed();

	Transform3D global_transform(p_state->get_transform());

	// Update simulator
	PhysicalBoneSimulator3D* simulator = get_simulator();
	Skeleton3D* skeleton = get_skeleton();
	if (simulator && skeleton) {
		if (bone_id != -1) {
			simulator->set_bone_global_pose(
				bone_id, skeleton->get_global_transform().affine_inverse() *
							 (global_transform * body_offset_inverse));
		}
	}
}

void PhysicalBone3D::_bind_methods() {}

void PhysicalBone3D::_update_joint_offset()
{
	_fix_joint_offset();

	set_ignore_transform_notification(true);
	reset_to_rest_position();
	set_ignore_transform_notification(false);

#ifdef TOOLS_ENABLED
	update_gizmos();
#endif
}

void PhysicalBone3D::_fix_joint_offset()
{
	// Clamp joint origin to bone origin
	PhysicalBoneSimulator3D* simulator = get_simulator();
	if (simulator) {
		joint_offset.origin = body_offset.affine_inverse().origin;
	}
}

void PhysicalBone3D::_reload_joint()
{
	PhysicalBoneSimulator3D* simulator = get_simulator();
	if (!simulator || !simulator->get_skeleton()) {
		PhysicsServer3D::get_singleton()->joint_clear(joint);
		return;
	}

	PhysicalBone3D* body_a = simulator->get_physical_bone_parent(bone_id);
	if (!body_a) {
		PhysicsServer3D::get_singleton()->joint_clear(joint);
		return;
	}

	Transform3D joint_transf = get_global_transform() * joint_offset;
	Transform3D local_a = body_a->get_global_transform().affine_inverse() * joint_transf;
	local_a.orthonormalize();

	switch (get_joint_type()) {
	case JOINT_TYPE_PIN: {
		PhysicsServer3D::get_singleton()->joint_make_pin(
			joint, body_a->get_rid(), local_a.origin, get_rid(), joint_offset.origin);
		const PinJointData* pjd(static_cast<const PinJointData*>(joint_data));
		PhysicsServer3D::get_singleton()->pin_joint_set_param(
			joint, PS3DE::PIN_JOINT_BIAS, pjd->bias);
		PhysicsServer3D::get_singleton()->pin_joint_set_param(
			joint, PS3DE::PIN_JOINT_DAMPING, pjd->damping);
		PhysicsServer3D::get_singleton()->pin_joint_set_param(
			joint, PS3DE::PIN_JOINT_IMPULSE_CLAMP, pjd->impulse_clamp);

	} break;
	case JOINT_TYPE_CONE: {
		PhysicsServer3D::get_singleton()->joint_make_cone_twist(
			joint, body_a->get_rid(), local_a, get_rid(), joint_offset);
		const ConeJointData* cjd(static_cast<const ConeJointData*>(joint_data));
		PhysicsServer3D::get_singleton()->cone_twist_joint_set_param(
			joint, PS3DE::CONE_TWIST_JOINT_SWING_SPAN, cjd->swing_span);
		PhysicsServer3D::get_singleton()->cone_twist_joint_set_param(
			joint, PS3DE::CONE_TWIST_JOINT_TWIST_SPAN, cjd->twist_span);
		PhysicsServer3D::get_singleton()->cone_twist_joint_set_param(
			joint, PS3DE::CONE_TWIST_JOINT_BIAS, cjd->bias);
		PhysicsServer3D::get_singleton()->cone_twist_joint_set_param(
			joint, PS3DE::CONE_TWIST_JOINT_SOFTNESS, cjd->softness);
		PhysicsServer3D::get_singleton()->cone_twist_joint_set_param(
			joint, PS3DE::CONE_TWIST_JOINT_RELAXATION, cjd->relaxation);

	} break;
	case JOINT_TYPE_HINGE: {
		PhysicsServer3D::get_singleton()->joint_make_hinge(
			joint, body_a->get_rid(), local_a, get_rid(), joint_offset);
		const HingeJointData* hjd(static_cast<const HingeJointData*>(joint_data));
		PhysicsServer3D::get_singleton()->hinge_joint_set_flag(
			joint, PS3DE::HINGE_JOINT_FLAG_USE_LIMIT, hjd->angular_limit_enabled);
		PhysicsServer3D::get_singleton()->hinge_joint_set_param(
			joint, PS3DE::HINGE_JOINT_LIMIT_UPPER, hjd->angular_limit_upper);
		PhysicsServer3D::get_singleton()->hinge_joint_set_param(
			joint, PS3DE::HINGE_JOINT_LIMIT_LOWER, hjd->angular_limit_lower);
		PhysicsServer3D::get_singleton()->hinge_joint_set_param(
			joint, PS3DE::HINGE_JOINT_LIMIT_BIAS, hjd->angular_limit_bias);
		PhysicsServer3D::get_singleton()->hinge_joint_set_param(
			joint, PS3DE::HINGE_JOINT_LIMIT_SOFTNESS, hjd->angular_limit_softness);
		PhysicsServer3D::get_singleton()->hinge_joint_set_param(
			joint, PS3DE::HINGE_JOINT_LIMIT_RELAXATION, hjd->angular_limit_relaxation);

	} break;
	case JOINT_TYPE_SLIDER: {
		PhysicsServer3D::get_singleton()->joint_make_slider(
			joint, body_a->get_rid(), local_a, get_rid(), joint_offset);
		const SliderJointData* sjd(static_cast<const SliderJointData*>(joint_data));
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER, sjd->linear_limit_upper);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER, sjd->linear_limit_lower);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS, sjd->linear_limit_softness);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION, sjd->linear_limit_restitution);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING, sjd->linear_limit_restitution);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER, sjd->angular_limit_upper);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER, sjd->angular_limit_lower);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS, sjd->angular_limit_softness);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS, sjd->angular_limit_softness);
		PhysicsServer3D::get_singleton()->slider_joint_set_param(
			joint, PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING, sjd->angular_limit_damping);

	} break;
	case JOINT_TYPE_6DOF: {
		PhysicsServer3D::get_singleton()->joint_make_generic_6dof(
			joint, body_a->get_rid(), local_a, get_rid(), joint_offset);
		const SixDOFJointData* g6dofjd(static_cast<const SixDOFJointData*>(joint_data));
		for (int axis = 0; axis < 3; ++axis) {
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT,
				g6dofjd->axis_data[axis].linear_limit_enabled);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_UPPER_LIMIT,
				g6dofjd->axis_data[axis].linear_limit_upper);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_LOWER_LIMIT,
				g6dofjd->axis_data[axis].linear_limit_lower);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS,
				g6dofjd->axis_data[axis].linear_limit_softness);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING,
				g6dofjd->axis_data[axis].linear_spring_enabled);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_SPRING_STIFFNESS,
				g6dofjd->axis_data[axis].linear_spring_stiffness);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_SPRING_DAMPING,
				g6dofjd->axis_data[axis].linear_spring_damping);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis),
				PS3DE::G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT,
				g6dofjd->axis_data[axis].linear_equilibrium_point);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION,
				g6dofjd->axis_data[axis].linear_restitution);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_LINEAR_DAMPING,
				g6dofjd->axis_data[axis].linear_damping);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT,
				g6dofjd->axis_data[axis].angular_limit_enabled);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_UPPER_LIMIT,
				g6dofjd->axis_data[axis].angular_limit_upper);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_LOWER_LIMIT,
				g6dofjd->axis_data[axis].angular_limit_lower);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS,
				g6dofjd->axis_data[axis].angular_limit_softness);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_RESTITUTION,
				g6dofjd->axis_data[axis].angular_restitution);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_DAMPING,
				g6dofjd->axis_data[axis].angular_damping);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_ERP,
				g6dofjd->axis_data[axis].erp);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_flag(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING,
				g6dofjd->axis_data[axis].angular_spring_enabled);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS,
				g6dofjd->axis_data[axis].angular_spring_stiffness);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis), PS3DE::G6DOF_JOINT_ANGULAR_SPRING_DAMPING,
				g6dofjd->axis_data[axis].angular_spring_damping);
			PhysicsServer3D::get_singleton()->generic_6dof_joint_set_param(joint,
				static_cast<Vector3::Axis>(axis),
				PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
				g6dofjd->axis_data[axis].angular_equilibrium_point);
		}

	} break;
	case JOINT_TYPE_NONE: {
	} break;
	}
}

void PhysicalBone3D::_on_bone_parent_changed() { _reload_joint(); }

Skeleton3D* PhysicalBone3D::get_skeleton() const
{
	PhysicalBoneSimulator3D* simulator = get_simulator();
	if (simulator) {
		return simulator->get_skeleton();
	}
	return nullptr;
}

#ifdef TOOLS_ENABLED
void PhysicalBone3D::_set_gizmo_move_joint(bool p_move_joint) { gizmo_move_joint = p_move_joint; }

Transform3D PhysicalBone3D::get_global_gizmo_transform() const
{
	return gizmo_move_joint ? get_global_transform() * joint_offset : get_global_transform();
}

Transform3D PhysicalBone3D::get_local_gizmo_transform() const
{
	return gizmo_move_joint ? get_transform() * joint_offset : get_transform();
}
#endif

const PhysicalBone3D::JointData* PhysicalBone3D::get_joint_data() const { return joint_data; }

PhysicalBone3D::JointType PhysicalBone3D::get_joint_type() const
{
	return joint_data ? joint_data->get_joint_type() : JOINT_TYPE_NONE;
}

void PhysicalBone3D::set_joint_offset(const Transform3D& p_offset)
{
	joint_offset = p_offset;

	_update_joint_offset();
}

const Transform3D& PhysicalBone3D::get_joint_offset() const { return joint_offset; }

void PhysicalBone3D::set_joint_rotation(const Vector3& p_euler_rad)
{
	joint_offset.basis.set_euler_scale(p_euler_rad, joint_offset.basis.get_scale());

	_update_joint_offset();
}

Vector3 PhysicalBone3D::get_joint_rotation() const
{
	return joint_offset.basis.get_euler_normalized();
}

const Transform3D& PhysicalBone3D::get_body_offset() const { return body_offset; }

void PhysicalBone3D::set_body_offset(const Transform3D& p_offset)
{
	body_offset = p_offset;
	body_offset_inverse = body_offset.affine_inverse();

	_update_joint_offset();
}

void PhysicalBone3D::set_simulate_physics(bool p_simulate)
{
	if (simulate_physics == p_simulate) {
		return;
	}

	simulate_physics = p_simulate;
	reset_physics_simulation_state();
}

bool PhysicalBone3D::get_simulate_physics() { return simulate_physics; }

bool PhysicalBone3D::is_simulating_physics() { return _internal_simulate_physics; }

void PhysicalBone3D::set_bone_name(const String& p_name)
{
	bone_name = p_name;
	bone_id = -1;

	update_bone_id();
	reset_to_rest_position();
}

const String& PhysicalBone3D::get_bone_name() const { return bone_name; }

real_t PhysicalBone3D::get_mass() const { return mass; }

real_t PhysicalBone3D::get_friction() const { return friction; }

real_t PhysicalBone3D::get_bounce() const { return bounce; }

real_t PhysicalBone3D::get_gravity_scale() const { return gravity_scale; }

PhysicalBone3D::DampMode PhysicalBone3D::get_linear_damp_mode() const { return linear_damp_mode; }

PhysicalBone3D::DampMode PhysicalBone3D::get_angular_damp_mode() const { return angular_damp_mode; }

real_t PhysicalBone3D::get_linear_damp() const { return linear_damp; }

real_t PhysicalBone3D::get_angular_damp() const { return angular_damp; }

bool PhysicalBone3D::is_able_to_sleep() const { return can_sleep; }

PhysicalBone3D::PhysicalBone3D() : PhysicsBody3D(PS3DE::BODY_MODE_STATIC)
{
	joint = PhysicsServer3D::get_singleton()->joint_create();
	reset_physics_simulation_state();
}

PhysicalBone3D::~PhysicalBone3D()
{
	memdelete(joint_data);
	ERR_FAIL_NULL(PhysicsServer3D::get_singleton());
	PhysicsServer3D::get_singleton()->free_rid(joint);
}

void PhysicalBone3D::update_bone_id()
{
	PhysicalBoneSimulator3D* simulator = get_simulator();
	if (!simulator) {
		return;
	}

	const int new_bone_id = simulator->find_bone(bone_name);

	if (new_bone_id != bone_id) {
		if (bone_id != -1) {
			// Assert the unbind from old node
			simulator->unbind_physical_bone_from_bone(bone_id);
		}

		bone_id = new_bone_id;

		simulator->bind_physical_bone_to_bone(bone_id, this);

		_fix_joint_offset();
		reset_physics_simulation_state();
	}
}

void PhysicalBone3D::update_offset()
{
#ifdef TOOLS_ENABLED
	PhysicalBoneSimulator3D* simulator = get_simulator();
	Skeleton3D* skeleton = get_skeleton();
	if (simulator && skeleton) {
		Transform3D bone_transform(skeleton->get_global_transform());
		if (bone_id != -1) {
			bone_transform *= simulator->get_bone_global_pose(bone_id);
		}

		if (gizmo_move_joint) {
			bone_transform *= body_offset;
			set_joint_offset(bone_transform.affine_inverse() * get_global_transform());
		}
		else {
			set_body_offset(bone_transform.affine_inverse() * get_global_transform());
		}
	}
#endif
}


