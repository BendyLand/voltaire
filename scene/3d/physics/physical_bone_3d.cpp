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

void PhysicalBone3D::_fix_joint_offset()
{
	// Clamp joint origin to bone origin
	PhysicalBoneSimulator3D* simulator = get_simulator();
	if (simulator) {
		joint_offset.origin = body_offset.affine_inverse().origin;
	}
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

const Transform3D& PhysicalBone3D::get_joint_offset() const { return joint_offset; }

Vector3 PhysicalBone3D::get_joint_rotation() const
{
	return joint_offset.basis.get_euler_normalized();
}

const Transform3D& PhysicalBone3D::get_body_offset() const { return body_offset; }

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


