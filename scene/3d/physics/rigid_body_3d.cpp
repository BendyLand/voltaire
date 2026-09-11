/**************************************************************************/
/*  rigid_body_3d.cpp                                                     */
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
#include "rigid_body_3d.h"
#include "scene/resources/physics_material.h"
#include "servers/physics_3d/physics_server_3d_constants.h"

struct _RigidBodyInOut
{
	RID rid;
	int shape = 0;
	int local_shape = 0;
};

void RigidBody3D::_notification(int p_what)
{
#ifdef TOOLS_ENABLED
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (Engine::get_singleton()->is_editor_hint()) {
			set_notify_local_transform(true); // Used for warnings and only in editor.
		}
	} break;

	case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
		update_configuration_warnings();
	} break;
	}
#endif
}

void RigidBody3D::_apply_body_mode()
{
	if (freeze) {
		switch (freeze_mode) {
		case FREEZE_MODE_STATIC: {
			set_body_mode(PS3DE::BODY_MODE_STATIC);
		} break;
		case FREEZE_MODE_KINEMATIC: {
			set_body_mode(PS3DE::BODY_MODE_KINEMATIC);
		} break;
		}
	}
	else if (lock_rotation) {
		set_body_mode(PS3DE::BODY_MODE_RIGID_LINEAR);
	}
	else {
		set_body_mode(PS3DE::BODY_MODE_RIGID);
	}
}

void RigidBody3D::set_lock_rotation_enabled(bool p_lock_rotation)
{
	if (p_lock_rotation == lock_rotation) {
		return;
	}

	lock_rotation = p_lock_rotation;
	_apply_body_mode();
}

bool RigidBody3D::is_lock_rotation_enabled() const { return lock_rotation; }

void RigidBody3D::set_freeze_enabled(bool p_freeze)
{
	if (p_freeze == freeze) {
		return;
	}

	freeze = p_freeze;
	_apply_body_mode();
}

bool RigidBody3D::is_freeze_enabled() const { return freeze; }

void RigidBody3D::set_freeze_mode(FreezeMode p_freeze_mode)
{
	if (p_freeze_mode == freeze_mode) {
		return;
	}

	freeze_mode = p_freeze_mode;
	_apply_body_mode();
}

RigidBody3D::FreezeMode RigidBody3D::get_freeze_mode() const { return freeze_mode; }

real_t RigidBody3D::get_mass() const { return mass; }

const Vector3& RigidBody3D::get_inertia() const { return inertia; }

RigidBody3D::CenterOfMassMode RigidBody3D::get_center_of_mass_mode() const
{
	return center_of_mass_mode;
}

const Vector3& RigidBody3D::get_center_of_mass() const { return center_of_mass; }

Ref<PhysicsMaterial> RigidBody3D::get_physics_material_override() const
{
	return physics_material_override;
}

real_t RigidBody3D::get_gravity_scale() const { return gravity_scale; }

RigidBody3D::DampMode RigidBody3D::get_linear_damp_mode() const { return linear_damp_mode; }

RigidBody3D::DampMode RigidBody3D::get_angular_damp_mode() const { return angular_damp_mode; }

real_t RigidBody3D::get_linear_damp() const { return linear_damp; }

real_t RigidBody3D::get_angular_damp() const { return angular_damp; }

Vector3 RigidBody3D::get_linear_velocity() const { return linear_velocity; }

Vector3 RigidBody3D::get_angular_velocity() const { return angular_velocity; }

Basis RigidBody3D::get_inverse_inertia_tensor() const { return inverse_inertia_tensor; }

void RigidBody3D::set_use_custom_integrator(bool p_enable)
{
	if (custom_integrator == p_enable) {
		return;
	}

	custom_integrator = p_enable;
	PhysicsServer3D::get_singleton()->body_set_omit_force_integration(get_rid(), p_enable);
}

bool RigidBody3D::is_using_custom_integrator() { return custom_integrator; }

bool RigidBody3D::is_able_to_sleep() const { return can_sleep; }

bool RigidBody3D::is_sleeping() const { return sleeping; }

void RigidBody3D::set_max_contacts_reported(int p_amount)
{
	ERR_FAIL_INDEX_MSG(p_amount, PS3DC::MAX_CONTACTS_REPORTED_3D_MAX,
		"Max contacts reported allocates memory (about 80 bytes each), and therefore must not be "
		"set too high.");
	max_contacts_reported = p_amount;
	PhysicsServer3D::get_singleton()->body_set_max_contacts_reported(get_rid(), p_amount);
}

int RigidBody3D::get_max_contacts_reported() const { return max_contacts_reported; }

int RigidBody3D::get_contact_count() const { return contact_count; }

void RigidBody3D::apply_central_impulse(const Vector3& p_impulse)
{
	PhysicsServer3D::get_singleton()->body_apply_central_impulse(get_rid(), p_impulse);
}

void RigidBody3D::apply_impulse(const Vector3& p_impulse, const Vector3& p_position)
{
	PhysicsServer3D* singleton = PhysicsServer3D::get_singleton();
	singleton->body_apply_impulse(get_rid(), p_impulse, p_position);
}

void RigidBody3D::apply_torque_impulse(const Vector3& p_impulse)
{
	PhysicsServer3D::get_singleton()->body_apply_torque_impulse(get_rid(), p_impulse);
}

void RigidBody3D::apply_central_force(const Vector3& p_force)
{
	PhysicsServer3D::get_singleton()->body_apply_central_force(get_rid(), p_force);
}

void RigidBody3D::apply_force(const Vector3& p_force, const Vector3& p_position)
{
	PhysicsServer3D* singleton = PhysicsServer3D::get_singleton();
	singleton->body_apply_force(get_rid(), p_force, p_position);
}

void RigidBody3D::apply_torque(const Vector3& p_torque)
{
	PhysicsServer3D::get_singleton()->body_apply_torque(get_rid(), p_torque);
}

void RigidBody3D::add_constant_central_force(const Vector3& p_force)
{
	PhysicsServer3D::get_singleton()->body_add_constant_central_force(get_rid(), p_force);
}

void RigidBody3D::add_constant_force(const Vector3& p_force, const Vector3& p_position)
{
	PhysicsServer3D* singleton = PhysicsServer3D::get_singleton();
	singleton->body_add_constant_force(get_rid(), p_force, p_position);
}

void RigidBody3D::add_constant_torque(const Vector3& p_torque)
{
	PhysicsServer3D::get_singleton()->body_add_constant_torque(get_rid(), p_torque);
}

void RigidBody3D::set_constant_force(const Vector3& p_force)
{
	PhysicsServer3D::get_singleton()->body_set_constant_force(get_rid(), p_force);
}

Vector3 RigidBody3D::get_constant_force() const
{
	return PhysicsServer3D::get_singleton()->body_get_constant_force(get_rid());
}

void RigidBody3D::set_constant_torque(const Vector3& p_torque)
{
	PhysicsServer3D::get_singleton()->body_set_constant_torque(get_rid(), p_torque);
}

Vector3 RigidBody3D::get_constant_torque() const
{
	return PhysicsServer3D::get_singleton()->body_get_constant_torque(get_rid());
}

void RigidBody3D::set_use_continuous_collision_detection(bool p_enable)
{
	ccd = p_enable;
	PhysicsServer3D::get_singleton()->body_set_enable_continuous_collision_detection(
		get_rid(), p_enable);
}

bool RigidBody3D::is_using_continuous_collision_detection() const { return ccd; }

bool RigidBody3D::is_contact_monitor_enabled() const { return contact_monitor != nullptr; }

PackedStringArray RigidBody3D::get_configuration_warnings() const
{
	PackedStringArray warnings = PhysicsBody3D::get_configuration_warnings();

	Vector3 scale = get_transform().get_basis().get_scale();
	if (Math::abs(scale.x - 1.0) > 0.05 || Math::abs(scale.y - 1.0) > 0.05 ||
		Math::abs(scale.z - 1.0) > 0.05) {
		warnings.push_back(
			RTR("Scale changes to RigidBody3D will be overridden by the physics engine when "
				"running.\nPlease change the size in children collision shapes instead."));
	}

	return warnings;
}

RigidBody3D::~RigidBody3D() { memdelete(contact_monitor); }


