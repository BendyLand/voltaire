/**************************************************************************/
/*  godot_constraints_2d.h                                                */
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

#include "godot_objects_2d.h"

class GodotConstraint2D {
	GodotBody2D **_body_ptr;
	int _body_count;
	uint64_t island_step = 0;
	bool disabled_collisions_between_bodies = true;

	RID self;

protected:
	GodotConstraint2D(GodotBody2D **p_body_ptr = nullptr, int p_body_count = 0) {
		_body_ptr = p_body_ptr;
		_body_count = p_body_count;
	}

public:
	_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }
	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ GodotBody2D **get_body_ptr() const { return _body_ptr; }
	_FORCE_INLINE_ int get_body_count() const { return _body_count; }

	_FORCE_INLINE_ void disable_collisions_between_bodies(const bool p_disabled) { disabled_collisions_between_bodies = p_disabled; }
	_FORCE_INLINE_ bool is_disabled_collisions_between_bodies() const { return disabled_collisions_between_bodies; }

	virtual bool setup(real_t p_step) = 0;
	virtual bool pre_solve(real_t p_step) = 0;
	virtual void solve(real_t p_step) = 0;

	virtual ~GodotConstraint2D() {}
};

class GodotBodyPair2D final : public GodotConstraint2D {
	enum {
		MAX_CONTACTS = 2
	};
	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	int shape_A = 0;
	int shape_B = 0;

	bool collide_A = false;
	bool collide_B = false;

	GodotSpace2D *space = nullptr;

	struct Contact {
		Vector2 position;
		Vector2 normal;
		Vector2 local_A, local_B;
		Vector2 acc_impulse; // accumulated impulse
		real_t acc_normal_impulse = 0.0; // accumulated normal impulse (Pn)
		real_t acc_tangent_impulse = 0.0; // accumulated tangent impulse (Pt)
		real_t acc_bias_impulse = 0.0; // accumulated normal impulse for position bias (Pnb)
		real_t acc_bias_impulse_center_of_mass = 0.0; // accumulated normal impulse for position bias applied to com
		real_t mass_normal = 0.0, mass_tangent = 0.0;
		real_t bias = 0.0;

		real_t depth = 0.0;
		bool active = false;
		bool used = false;
		Vector2 rA, rB;
		real_t bounce = 0.0;
	};

	Vector2 offset_B; //use local A coordinates to avoid numerical issues on collision detection

	Vector2 sep_axis;
	Contact contacts[MAX_CONTACTS];
	int contact_count = 0;
	bool collided = false;
	bool check_ccd = false;
	bool oneway_disabled = false;
	bool report_contacts_only = false;

	bool _test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B);
	void _validate_contacts();
	static void _add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self);
	_FORCE_INLINE_ void _contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B);

public:
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B);
	virtual ~GodotBodyPair2D() override;
};

class GodotAreaPair2D final : public GodotConstraint2D {
	GodotBody2D *body = nullptr;
	GodotArea2D *area = nullptr;
	int body_shape = 0;
	int area_shape = 0;
	bool colliding = false;
	bool has_space_override = false;
	bool process_collision = false;
	bool body_has_attached_area = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape);
	virtual ~GodotAreaPair2D() override = default;
};

class GodotArea2Pair2D final : public GodotConstraint2D {
	GodotArea2D *area_a = nullptr;
	GodotArea2D *area_b = nullptr;
	int shape_a = 0;
	int shape_b = 0;
	bool colliding_a = false;
	bool colliding_b = false;
	bool process_collision_a = false;
	bool process_collision_b = false;
	bool area_a_monitorable = false;
	bool area_b_monitorable = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b);
	virtual ~GodotArea2Pair2D() override = default;
};

class GodotJoint2D final : public GodotConstraint2D {
	PS2DE::JointType type = PS2DE::JOINT_TYPE_MAX;

	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	real_t bias = 0.0;
	real_t max_bias = 3.40282e+38;
	real_t max_force = 3.40282e+38;

	bool dynamic_A = false;
	bool dynamic_B = false;

	// Shared working fields
	Vector2 rA, rB;
	Vector2 anchor_A;
	Vector2 anchor_B;
	real_t jn_max = 0.0;

	// Pin joint fields
	Transform2D M;
	Vector2 pin_bias;
	real_t initial_angle = 0.0;
	real_t bias_velocity = 0.0;
	real_t j_acc = 0.0;
	real_t i_sum = 0.0;
	Vector2 P;
	real_t softness = 0.0;
	real_t angular_limit_lower = 0.0;
	real_t angular_limit_upper = 0.0;
	real_t motor_target_velocity = 0.0;
	bool is_joint_at_limit = false;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove joint fields
	Vector2 A_groove_1;
	Vector2 A_groove_2;
	Vector2 A_groove_normal;
	Vector2 B_anchor;
	Vector2 jn_acc;
	Vector2 gbias;
	real_t clamp = 0.0;
	Vector2 xf_normal;
	Vector2 k1, k2;
	bool correct = false;

	// Damped spring joint fields
	real_t rest_length = 0.0;
	real_t damping = 1.5;
	real_t stiffness = 20.0;
	Vector2 n;
	Vector2 j;
	real_t n_mass = 0.0;
	real_t target_vrn = 0.0;
	real_t v_coef = 0.0;

	bool _setup_pin(real_t p_step);
	bool _pre_solve_pin(real_t p_step);
	void _solve_pin(real_t p_step);

	bool _setup_groove(real_t p_step);
	bool _pre_solve_groove(real_t p_step);
	void _solve_groove(real_t p_step);

	bool _setup_damped_spring(real_t p_step);
	bool _pre_solve_damped_spring(real_t p_step);
	void _solve_damped_spring(real_t p_step);

public:
	_FORCE_INLINE_ PS2DE::JointType get_type() const { return type; }

	_FORCE_INLINE_ void set_max_force(real_t p_force) { max_force = p_force; }
	_FORCE_INLINE_ real_t get_max_force() const { return max_force; }

	_FORCE_INLINE_ void set_bias(real_t p_bias) { bias = p_bias; }
	_FORCE_INLINE_ real_t get_bias() const { return bias; }

	_FORCE_INLINE_ void set_max_bias(real_t p_bias) { max_bias = p_bias; }
	_FORCE_INLINE_ real_t get_max_bias() const { return max_bias; }

	void copy_settings_from(GodotJoint2D *p_joint);

	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	// Pin joint parameters & flags
	void set_param(PS2DE::PinJointParam p_param, real_t p_value);
	real_t get_param(PS2DE::PinJointParam p_param) const;
	void set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool get_flag(PS2DE::PinJointFlag p_flag) const;

	// Damped spring parameters
	void set_param(PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t get_param(PS2DE::DampedSpringParam p_param) const;

	// Empty joint (JOINT_TYPE_MAX)
	GodotJoint2D() :
			GodotConstraint2D(_arr, 0) {}

	// Pin joint (JOINT_TYPE_PIN)
	GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b = nullptr);

	// Groove joint (JOINT_TYPE_GROOVE)
	GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	// Damped spring joint (JOINT_TYPE_DAMPED_SPRING)
	GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	GodotJoint2D(const GodotJoint2D &) = delete;
	GodotJoint2D &operator=(const GodotJoint2D &) = delete;

	virtual ~GodotJoint2D() override {
		for (int i = 0; i < get_body_count(); i++) {
			GodotBody2D *body = get_body_ptr()[i];
			if (body) {
				body->remove_constraint(this, i);
			}
		}
	}
};
