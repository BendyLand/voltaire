/**************************************************************************/
/*  pin_joint_2d.cpp                                                      */
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
#include "core/object/class_db.h"
#include "pin_joint_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

void PinJoint2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!is_inside_tree()) {
			break;
		}

		if (!Engine::get_singleton()->is_editor_hint() &&
			!get_tree()->is_debugging_collisions_hint()) {
			break;
		}

		draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
		draw_line(Point2(0, -10), Point2(0, +10), Color(0.7, 0.6, 0.0, 0.5), 3);
	} break;
	}
}

void PinJoint2D::_configure_joint(RID p_joint, PhysicsBody2D* body_a, PhysicsBody2D* body_b)
{
	PhysicsServer2D::get_singleton()->joint_make_pin(
		p_joint, get_global_position(), body_a->get_rid(), body_b ? body_b->get_rid() : RID());
	PhysicsServer2D::get_singleton()->pin_joint_set_param(
		p_joint, PS2DE::PIN_JOINT_SOFTNESS, softness);
	PhysicsServer2D::get_singleton()->pin_joint_set_param(
		p_joint, PS2DE::PIN_JOINT_LIMIT_UPPER, angular_limit_upper);
	PhysicsServer2D::get_singleton()->pin_joint_set_param(
		p_joint, PS2DE::PIN_JOINT_LIMIT_LOWER, angular_limit_lower);
	PhysicsServer2D::get_singleton()->pin_joint_set_param(
		p_joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, motor_target_velocity);
	PhysicsServer2D::get_singleton()->pin_joint_set_flag(
		p_joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, motor_enabled);
	PhysicsServer2D::get_singleton()->pin_joint_set_flag(
		p_joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, angular_limit_enabled);
}

void PinJoint2D::set_softness(real_t p_softness)
{
	if (softness == p_softness) {
		return;
	}
	softness = p_softness;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_param(
			get_rid(), PS2DE::PIN_JOINT_SOFTNESS, p_softness);
	}
}

real_t PinJoint2D::get_softness() const { return softness; }

void PinJoint2D::set_angular_limit_lower(real_t p_angular_limit_lower)
{
	if (angular_limit_lower == p_angular_limit_lower) {
		return;
	}
	angular_limit_lower = p_angular_limit_lower;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_param(
			get_rid(), PS2DE::PIN_JOINT_LIMIT_LOWER, p_angular_limit_lower);
	}
}

real_t PinJoint2D::get_angular_limit_lower() const { return angular_limit_lower; }

void PinJoint2D::set_angular_limit_upper(real_t p_angular_limit_upper)
{
	if (angular_limit_upper == p_angular_limit_upper) {
		return;
	}
	angular_limit_upper = p_angular_limit_upper;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_param(
			get_rid(), PS2DE::PIN_JOINT_LIMIT_UPPER, p_angular_limit_upper);
	}
}

real_t PinJoint2D::get_angular_limit_upper() const { return angular_limit_upper; }

void PinJoint2D::set_motor_target_velocity(real_t p_motor_target_velocity)
{
	if (motor_target_velocity == p_motor_target_velocity) {
		return;
	}
	motor_target_velocity = p_motor_target_velocity;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_param(
			get_rid(), PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, motor_target_velocity);
	}
}

real_t PinJoint2D::get_motor_target_velocity() const { return motor_target_velocity; }

void PinJoint2D::set_motor_enabled(bool p_motor_enabled)
{
	if (motor_enabled == p_motor_enabled) {
		return;
	}
	motor_enabled = p_motor_enabled;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_flag(
			get_rid(), PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, motor_enabled);
	}
}

bool PinJoint2D::is_motor_enabled() const { return motor_enabled; }

void PinJoint2D::set_angular_limit_enabled(bool p_angular_limit_enabled)
{
	if (angular_limit_enabled == p_angular_limit_enabled) {
		return;
	}
	angular_limit_enabled = p_angular_limit_enabled;
	queue_redraw();
	if (is_configured()) {
		PhysicsServer2D::get_singleton()->pin_joint_set_flag(
			get_rid(), PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, angular_limit_enabled);
	}
}

bool PinJoint2D::is_angular_limit_enabled() const { return angular_limit_enabled; }

void PinJoint2D::_bind_methods() {}

PinJoint2D::PinJoint2D() {}


