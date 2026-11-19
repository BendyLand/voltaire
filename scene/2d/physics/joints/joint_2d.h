/**************************************************************************/
/*  joint_2d.h                                                            */
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

#include "scene/2d/node_2d.h"

class PhysicsBody2D;

class Joint2D : public Node2D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_GROOVE,
		JOINT_DAMPED_SPRING,
	};

private:
	JointType joint_type = JOINT_PIN;
	RID joint;
	RID ba, bb;

	NodePath a;
	NodePath b;
	real_t bias = 0.0f;
	bool exclude_from_collision = true;
	bool configured = false;
	String warning;

	// Pin parameters.
	real_t softness = 0.0f;
	real_t angular_limit_lower = 0.0f;
	real_t angular_limit_upper = 0.0f;
	real_t motor_target_velocity = 0.0f;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove & Spring parameters.
	real_t length = 50.0f;
	real_t initial_offset = 25.0f;
	real_t rest_length = 0.0f;
	real_t stiffness = 20.0f;
	real_t damping = 1.0f;

	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b);

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint2D(JointType p_type = JOINT_PIN);
	~Joint2D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const;

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const;

	void set_bias(real_t p_bias);
	real_t get_bias() const;

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const;

	// Pin parameters.
	void set_softness(real_t p_softness);
	real_t get_softness() const { return softness; }
	void set_angular_limit_lower(real_t p_angular_limit_lower);
	real_t get_angular_limit_lower() const { return angular_limit_lower; }
	void set_angular_limit_upper(real_t p_angular_limit_upper);
	real_t get_angular_limit_upper() const { return angular_limit_upper; }
	void set_motor_target_velocity(real_t p_motor_target_velocity);
	real_t get_motor_target_velocity() const { return motor_target_velocity; }
	void set_motor_enabled(bool p_motor_enabled);
	bool is_motor_enabled() const { return motor_enabled; }
	void set_angular_limit_enabled(bool p_angular_limit_enabled);
	bool is_angular_limit_enabled() const { return angular_limit_enabled; }

	// Groove & DampedSpring parameters.
	void set_length(real_t p_length);
	real_t get_length() const { return length; }
	void set_initial_offset(real_t p_initial_offset);
	real_t get_initial_offset() const { return initial_offset; }
	void set_rest_length(real_t p_rest_length);
	real_t get_rest_length() const { return rest_length; }
	void set_stiffness(real_t p_stiffness);
	real_t get_stiffness() const { return stiffness; }
	void set_damping(real_t p_damping);
	real_t get_damping() const { return damping; }

	RID get_rid() const { return joint; }
};
