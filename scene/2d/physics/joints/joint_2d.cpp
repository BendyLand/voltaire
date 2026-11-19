/**************************************************************************/
/*  joint_2d.cpp                                                          */
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

#include "joint_2d.h"

#include "core/config/engine.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

Joint2D::Joint2D(JointType p_type) :
		joint_type(p_type) {
	joint = PhysicsServer2D::joint_create();
	set_hide_clip_children(true);
}

Joint2D::~Joint2D() {
	ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
	PhysicsServer2D::free_rid(joint);
}

void Joint2D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	joint_type = p_type;
	_update_joint();
	queue_redraw();
}

void Joint2D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

NodePath Joint2D::get_node_a() const {
	return a;
}

void Joint2D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

NodePath Joint2D::get_node_b() const {
	return b;
}

void Joint2D::_disconnect_signals() {}

void Joint2D::_update_joint(bool p_only_free) {
	warning = String();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_remove_collision_exception(ba, bb);
	}
	ba = RID();
	bb = RID();
	configured = false;

	if (p_only_free || !is_inside_tree()) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody2D *body_a = dynamic_cast<PhysicsBody2D *>(node_a);
	PhysicsBody2D *body_b = dynamic_cast<PhysicsBody2D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a || (joint_type != JOINT_PIN && !body_b)) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	_configure_joint(joint, body_a, body_b);
	PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_add_collision_exception(ba, bb);
	}
	configured = true;
	update_configuration_warnings();
}

void Joint2D::_configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b) {
	switch (joint_type) {
		case JOINT_PIN: {
			PhysicsServer2D::joint_make_pin(p_joint, get_global_position(), body_a->get_rid(), body_b ? body_b->get_rid() : RID());
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_SOFTNESS, softness);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_UPPER, angular_limit_upper);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_LOWER, angular_limit_lower);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, motor_target_velocity);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, motor_enabled);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, angular_limit_enabled);
		} break;
		case JOINT_GROOVE: {
			Transform2D gt = get_global_transform();
			Vector2 groove_A1 = gt.get_origin();
			Vector2 groove_A2 = gt.xform(Vector2(0, length));
			Vector2 anchor_B = gt.xform(Vector2(0, initial_offset));
			PhysicsServer2D::joint_make_groove(p_joint, groove_A1, groove_A2, anchor_B, body_a->get_rid(), body_b->get_rid());
		} break;
		case JOINT_DAMPED_SPRING: {
			Transform2D gt = get_global_transform();
			Vector2 anchor_A = gt.get_origin();
			Vector2 anchor_B = gt.xform(Vector2(0, length));
			PhysicsServer2D::joint_make_damped_spring(p_joint, anchor_A, anchor_B, body_a->get_rid(), body_b->get_rid());
			if (rest_length) {
				PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_REST_LENGTH, rest_length);
			}
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_STIFFNESS, stiffness);
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_DAMPING, damping);
		} break;
	}
}

void Joint2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
		case NOTIFICATION_DRAW: {
			if (!is_inside_tree()) {
				break;
			}
			if (!Engine::is_editor_hint() && !SceneTree::is_debugging_collisions_hint()) {
				break;
			}
			if (joint_type == JOINT_PIN) {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, -10), Point2(0, +10), Color(0.7, 0.6, 0.0, 0.5), 3);
			} else {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(-10, length), Point2(+10, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, 0), Point2(0, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				if (joint_type == JOINT_GROOVE) {
					draw_line(Point2(-10, initial_offset), Point2(+10, initial_offset), Color(0.8, 0.8, 0.9, 0.5), 5);
				}
			}
		} break;
	}
}

void Joint2D::set_bias(real_t p_bias) {
	bias = p_bias;
	if (joint.is_valid()) {
		PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);
	}
}

real_t Joint2D::get_bias() const {
	return bias;
}

void Joint2D::set_exclude_nodes_from_collision(bool p_enable) {
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

bool Joint2D::get_exclude_nodes_from_collision() const {
	return exclude_from_collision;
}

void Joint2D::set_softness(real_t p_softness) {
	softness = p_softness;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_SOFTNESS, p_softness);
	}
}

void Joint2D::set_angular_limit_lower(real_t p_angular_limit_lower) {
	angular_limit_lower = p_angular_limit_lower;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_LOWER, p_angular_limit_lower);
	}
}

void Joint2D::set_angular_limit_upper(real_t p_angular_limit_upper) {
	angular_limit_upper = p_angular_limit_upper;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_UPPER, p_angular_limit_upper);
	}
}

void Joint2D::set_motor_target_velocity(real_t p_motor_target_velocity) {
	motor_target_velocity = p_motor_target_velocity;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, p_motor_target_velocity);
	}
}

void Joint2D::set_motor_enabled(bool p_motor_enabled) {
	motor_enabled = p_motor_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, p_motor_enabled);
	}
}

void Joint2D::set_angular_limit_enabled(bool p_angular_limit_enabled) {
	angular_limit_enabled = p_angular_limit_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, p_angular_limit_enabled);
	}
}

void Joint2D::set_length(real_t p_length) {
	length = p_length;
	queue_redraw();
	if (is_configured() && joint_type != JOINT_PIN) {
		_update_joint();
	}
}

void Joint2D::set_initial_offset(real_t p_initial_offset) {
	initial_offset = p_initial_offset;
	queue_redraw();
	if (is_configured() && joint_type == JOINT_GROOVE) {
		_update_joint();
	}
}

void Joint2D::set_rest_length(real_t p_rest_length) {
	rest_length = p_rest_length;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_REST_LENGTH, p_rest_length ? p_rest_length : length);
	}
}

void Joint2D::set_stiffness(real_t p_stiffness) {
	stiffness = p_stiffness;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_STIFFNESS, p_stiffness);
	}
}

void Joint2D::set_damping(real_t p_damping) {
	damping = p_damping;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_DAMPING, p_damping);
	}
}

PackedStringArray Joint2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}
