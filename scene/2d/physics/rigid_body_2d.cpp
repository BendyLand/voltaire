/**************************************************************************/
/*  rigid_body_2d.cpp                                                     */
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
#include "rigid_body_2d.h"
#include "scene/resources/physics_material.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "servers/physics_2d/physics_server_2d_constants.h"

struct _RigidBody2DInOut
{
	RID rid;
	int shape = 0;
	int local_shape = 0;
};

void RigidBody2D::_apply_body_mode()
{
	if (freeze) {
		switch (freeze_mode) {
		case FREEZE_MODE_STATIC: {
			set_body_mode(PS2DE::BODY_MODE_STATIC);
		} break;
		case FREEZE_MODE_KINEMATIC: {
			set_body_mode(PS2DE::BODY_MODE_KINEMATIC);
		} break;
		}
	}
	else if (lock_rotation) {
		set_body_mode(PS2DE::BODY_MODE_RIGID_LINEAR);
	}
	else {
		set_body_mode(PS2DE::BODY_MODE_RIGID);
	}
}

void RigidBody2D::set_lock_rotation_enabled(bool p_lock_rotation)
{
	if (p_lock_rotation == lock_rotation) {
		return;
	}

	lock_rotation = p_lock_rotation;
	_apply_body_mode();
}

bool RigidBody2D::is_lock_rotation_enabled() const { return lock_rotation; }

void RigidBody2D::set_freeze_enabled(bool p_freeze)
{
	if (p_freeze == freeze) {
		return;
	}

	freeze = p_freeze;
	_apply_body_mode();
}

bool RigidBody2D::is_freeze_enabled() const { return freeze; }

void RigidBody2D::set_freeze_mode(FreezeMode p_freeze_mode)
{
	if (p_freeze_mode == freeze_mode) {
		return;
	}

	freeze_mode = p_freeze_mode;
	_apply_body_mode();
}

RigidBody2D::FreezeMode RigidBody2D::get_freeze_mode() const { return freeze_mode; }

real_t RigidBody2D::get_mass() const { return mass; }

real_t RigidBody2D::get_inertia() const { return inertia; }

RigidBody2D::CenterOfMassMode RigidBody2D::get_center_of_mass_mode() const
{
	return center_of_mass_mode;
}

const Vector2& RigidBody2D::get_center_of_mass() const { return center_of_mass; }

Ref<PhysicsMaterial> RigidBody2D::get_physics_material_override() const
{
	return physics_material_override;
}

real_t RigidBody2D::get_gravity_scale() const { return gravity_scale; }

RigidBody2D::DampMode RigidBody2D::get_linear_damp_mode() const { return linear_damp_mode; }

RigidBody2D::DampMode RigidBody2D::get_angular_damp_mode() const { return angular_damp_mode; }

real_t RigidBody2D::get_linear_damp() const { return linear_damp; }

real_t RigidBody2D::get_angular_damp() const { return angular_damp; }

Vector2 RigidBody2D::get_linear_velocity() const { return linear_velocity; }

real_t RigidBody2D::get_angular_velocity() const { return angular_velocity; }

void RigidBody2D::set_use_custom_integrator(bool p_enable)
{
	if (custom_integrator == p_enable) {
		return;
	}

	custom_integrator = p_enable;
	PhysicsServer2D::get_singleton()->body_set_omit_force_integration(get_rid(), p_enable);
}

bool RigidBody2D::is_using_custom_integrator() { return custom_integrator; }

bool RigidBody2D::is_able_to_sleep() const { return can_sleep; }

bool RigidBody2D::is_sleeping() const { return sleeping; }

void RigidBody2D::set_max_contacts_reported(int p_amount)
{
	ERR_FAIL_INDEX_MSG(p_amount, PS2DC::MAX_CONTACTS_REPORTED_2D_MAX,
		"Max contacts reported allocates memory (about 100 bytes each), and therefore must not be "
		"set too high.");
	max_contacts_reported = p_amount;
	PhysicsServer2D::get_singleton()->body_set_max_contacts_reported(get_rid(), p_amount);
}

int RigidBody2D::get_max_contacts_reported() const { return max_contacts_reported; }

int RigidBody2D::get_contact_count() const { return contact_count; }

void RigidBody2D::apply_central_impulse(const Vector2& p_impulse)
{
	PhysicsServer2D::get_singleton()->body_apply_central_impulse(get_rid(), p_impulse);
}

void RigidBody2D::apply_impulse(const Vector2& p_impulse, const Vector2& p_position)
{
	PhysicsServer2D::get_singleton()->body_apply_impulse(get_rid(), p_impulse, p_position);
}

void RigidBody2D::apply_torque_impulse(real_t p_torque)
{
	PhysicsServer2D::get_singleton()->body_apply_torque_impulse(get_rid(), p_torque);
}

void RigidBody2D::apply_central_force(const Vector2& p_force)
{
	PhysicsServer2D::get_singleton()->body_apply_central_force(get_rid(), p_force);
}

void RigidBody2D::apply_force(const Vector2& p_force, const Vector2& p_position)
{
	PhysicsServer2D::get_singleton()->body_apply_force(get_rid(), p_force, p_position);
}

void RigidBody2D::apply_torque(real_t p_torque)
{
	PhysicsServer2D::get_singleton()->body_apply_torque(get_rid(), p_torque);
}

void RigidBody2D::add_constant_central_force(const Vector2& p_force)
{
	PhysicsServer2D::get_singleton()->body_add_constant_central_force(get_rid(), p_force);
}

void RigidBody2D::add_constant_force(const Vector2& p_force, const Vector2& p_position)
{
	PhysicsServer2D::get_singleton()->body_add_constant_force(get_rid(), p_force, p_position);
}

void RigidBody2D::add_constant_torque(const real_t p_torque)
{
	PhysicsServer2D::get_singleton()->body_add_constant_torque(get_rid(), p_torque);
}

void RigidBody2D::set_constant_force(const Vector2& p_force)
{
	PhysicsServer2D::get_singleton()->body_set_constant_force(get_rid(), p_force);
}

Vector2 RigidBody2D::get_constant_force() const
{
	return PhysicsServer2D::get_singleton()->body_get_constant_force(get_rid());
}

void RigidBody2D::set_constant_torque(real_t p_torque)
{
	PhysicsServer2D::get_singleton()->body_set_constant_torque(get_rid(), p_torque);
}

real_t RigidBody2D::get_constant_torque() const
{
	return PhysicsServer2D::get_singleton()->body_get_constant_torque(get_rid());
}

void RigidBody2D::set_continuous_collision_detection_mode(CCDMode p_mode)
{
	ccd_mode = p_mode;
	PhysicsServer2D::get_singleton()->body_set_continuous_collision_detection_mode(
		get_rid(), PS2DE::CCDMode(p_mode));
}

RigidBody2D::CCDMode RigidBody2D::get_continuous_collision_detection_mode() const
{
	return ccd_mode;
}

void RigidBody2D::_notification(int p_what)
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

PackedStringArray RigidBody2D::get_configuration_warnings() const
{
	Transform2D t = get_transform();

	PackedStringArray warnings = PhysicsBody2D::get_configuration_warnings();

	if (Math::abs(t.columns[0].length() - 1.0) > 0.05 ||
		Math::abs(t.columns[1].length() - 1.0) > 0.05) {
		warnings.push_back(
			RTR("Size changes to RigidBody2D will be overridden by the physics engine when "
				"running.\nChange the size in children collision shapes instead."));
	}

	return warnings;
}


