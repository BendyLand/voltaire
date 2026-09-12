/**************************************************************************/
/*  physical_bone_2d.cpp                                                  */
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

#include "physical_bone_2d.h"
#include "scene/2d/physics/joints/joint_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

void PhysicalBone2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		// Position the RigidBody in the correct position.
		if (follow_bone_when_simulating) {
			_position_at_bone2d();
		}

		// Keep the child joint in the correct position.
		if (child_joint && auto_configure_joint) {
			child_joint->set_global_position(get_global_position());
		}
	} break;

	case NOTIFICATION_READY: {
		_find_skeleton_parent();
		_find_joint_child();

		// Configure joint.
		if (child_joint && auto_configure_joint) {
			_auto_configure_joint();
		}

		// Simulate physics if set.
		if (simulate_physics) {
			_start_physics_simulation();
		}
		else {
			_stop_physics_simulation();
		}

		set_physics_process_internal(true);
	} break;
	}
}

void PhysicalBone2D::_position_at_bone2d()
{
	// Reset to Bone2D position
	if (parent_skeleton) {
		Bone2D* bone_to_use = parent_skeleton->get_bone(bone2d_index);
		ERR_FAIL_NULL_MSG(bone_to_use,
			"It's not possible to position the bone with ID: " + itos(bone2d_index) + ".");
		set_global_transform(bone_to_use->get_global_transform());
	}
}

void PhysicalBone2D::_start_physics_simulation()
{
	if (_internal_simulate_physics) {
		return;
	}

	// Reset to Bone2D position.
	_position_at_bone2d();

	// Apply the layers and masks.
	PhysicsServer2D::get_singleton()->body_set_collision_layer(get_rid(), get_collision_layer());
	PhysicsServer2D::get_singleton()->body_set_collision_mask(get_rid(), get_collision_mask());
	PhysicsServer2D::get_singleton()->body_set_collision_priority(
		get_rid(), get_collision_priority());

	// Apply the correct mode.
	_apply_body_mode();

	_internal_simulate_physics = true;
	set_physics_process_internal(true);
}

void PhysicalBone2D::_stop_physics_simulation()
{
	if (_internal_simulate_physics) {
		_internal_simulate_physics = false;

		// Reset to Bone2D position
		_position_at_bone2d();

		set_physics_process_internal(false);
		PhysicsServer2D::get_singleton()->body_set_collision_layer(get_rid(), 0);
		PhysicsServer2D::get_singleton()->body_set_collision_mask(get_rid(), 0);
		PhysicsServer2D::get_singleton()->body_set_collision_priority(get_rid(), 1.0);
		PhysicsServer2D::get_singleton()->body_set_mode(
			get_rid(), PS2DE::BodyMode::BODY_MODE_STATIC);
	}
}

Joint2D* PhysicalBone2D::get_joint() const { return child_joint; }

bool PhysicalBone2D::get_auto_configure_joint() const { return auto_configure_joint; }

void PhysicalBone2D::set_auto_configure_joint(bool p_auto_configure)
{
	auto_configure_joint = p_auto_configure;
	_auto_configure_joint();
}

void PhysicalBone2D::set_simulate_physics(bool p_simulate)
{
	if (p_simulate == simulate_physics) {
		return;
	}
	simulate_physics = p_simulate;

	if (simulate_physics) {
		_start_physics_simulation();
	}
	else {
		_stop_physics_simulation();
	}
}

bool PhysicalBone2D::get_simulate_physics() const { return simulate_physics; }

bool PhysicalBone2D::is_simulating_physics() const { return _internal_simulate_physics; }

NodePath PhysicalBone2D::get_bone2d_nodepath() const { return bone2d_nodepath; }

int PhysicalBone2D::get_bone2d_index() const { return bone2d_index; }

void PhysicalBone2D::set_follow_bone_when_simulating(bool p_follow_bone)
{
	follow_bone_when_simulating = p_follow_bone;

	if (_internal_simulate_physics) {
		_stop_physics_simulation();
		_start_physics_simulation();
	}
}

bool PhysicalBone2D::get_follow_bone_when_simulating() const { return follow_bone_when_simulating; }


PhysicalBone2D::PhysicalBone2D()
{
	// Stop the RigidBody from executing its force integration.
	PhysicsServer2D::get_singleton()->body_set_collision_layer(get_rid(), 0);
	PhysicsServer2D::get_singleton()->body_set_collision_mask(get_rid(), 0);
	PhysicsServer2D::get_singleton()->body_set_mode(get_rid(), PS2DE::BodyMode::BODY_MODE_STATIC);

	child_joint = nullptr;
}

PhysicalBone2D::~PhysicalBone2D() {}


