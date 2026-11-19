/**************************************************************************/
/*  joint_3d.h                                                            */
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

#include "scene/3d/node_3d.h"
#include "scene/3d/physics/physics_body_3d.h"

class Joint3D : public Node3D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_HINGE,
		JOINT_SLIDER,
		JOINT_CONE_TWIST,
		JOINT_6DOF,
	};

	using PinParam = PS3DE::PinJointParam;
	using HingeParam = PS3DE::HingeJointParam;
	using HingeFlag = PS3DE::HingeJointFlag;
	using SliderParam = PS3DE::SliderJointParam;
	using ConeTwistParam = PS3DE::ConeTwistJointParam;
	using G6DOFParam = PS3DE::G6DOFJointAxisParam;
	using G6DOFFlag = PS3DE::G6DOFJointAxisFlag;

private:
	JointType joint_type = JOINT_PIN;
	RID ba, bb;
	RID joint;
	NodePath a;
	NodePath b;
	int solver_priority = 1;
	bool exclude_from_collision = true;
	String warning;
	bool configured = false;

	// Pin parameters.
	real_t pin_params[3] = { 0.3f, 1.0f, 0.0f };

	// Hinge parameters & flags.
	real_t hinge_params[PS3DE::HINGE_JOINT_MAX] = {};
	bool hinge_flags[PS3DE::HINGE_JOINT_FLAG_MAX] = {};

	// Slider parameters.
	real_t slider_params[PS3DE::SLIDER_JOINT_MAX] = {};

	// Cone twist parameters.
	static constexpr int CONE_TWIST_PARAM_MAX = 5;
	real_t cone_params[CONE_TWIST_PARAM_MAX] = {};

	// Generic 6DOF parameters & flags.
	real_t g6dof_params[3][PS3DE::G6DOF_JOINT_MAX] = {};
	bool g6dof_flags[3][PS3DE::G6DOF_JOINT_FLAG_MAX] = {};
	bool linear_drive_force_limit_set[3] = {};
	bool angular_drive_torque_limit_set[3] = {};
	Quaternion angular_target_rotation;
	bool has_angular_target_rotation = false;

	void _init_defaults();
	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b);
	bool _should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint3D(JointType p_type = JOINT_PIN);
	~Joint3D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const { return a; }

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const { return b; }

	void set_solver_priority(int p_priority);
	int get_solver_priority() const { return solver_priority; }

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const { return exclude_from_collision; }

	// Pin API.
	void set_pin_param(PinParam p_param, real_t p_value);
	real_t get_pin_param(PinParam p_param) const;

	// Hinge API.
	void set_hinge_param(HingeParam p_param, real_t p_value);
	real_t get_hinge_param(HingeParam p_param) const;
	void set_hinge_flag(HingeFlag p_flag, bool p_enabled);
	bool get_hinge_flag(HingeFlag p_flag) const;

	// Slider API.
	void set_slider_param(SliderParam p_param, real_t p_value);
	real_t get_slider_param(SliderParam p_param) const;

	// ConeTwist API.
	void set_cone_twist_param(ConeTwistParam p_param, real_t p_value);
	real_t get_cone_twist_param(ConeTwistParam p_param) const;

	// Generic 6DOF API.
	void set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value);
	real_t get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;
	void set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled);
	bool get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const;

	void set_param_x(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_X, p_param, p_value); }
	real_t get_param_x(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_X, p_param); }
	void set_param_y(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Y, p_param, p_value); }
	real_t get_param_y(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Y, p_param); }
	void set_param_z(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Z, p_param, p_value); }
	real_t get_param_z(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Z, p_param); }

	void set_flag_x(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_X, p_flag, p_enabled); }
	bool get_flag_x(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_X, p_flag); }
	void set_flag_y(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Y, p_flag, p_enabled); }
	bool get_flag_y(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Y, p_flag); }
	void set_flag_z(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Z, p_flag, p_enabled); }
	bool get_flag_z(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Z, p_flag); }

	void set_angular_target_rotation(const Quaternion &p_target_rotation);
	Quaternion get_angular_target_rotation() const;
	bool has_target_rotation() const { return has_angular_target_rotation; }
	void clear_angular_target_rotation();

	RID get_rid() const { return joint; }
};
