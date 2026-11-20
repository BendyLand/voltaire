/**************************************************************************/
/*  physics_direct_body_state_2d.h                                        */
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

#include "servers/physics_2d/direct_states/physics_direct_space_state_2d.h"

class GodotBody2D;

class PhysicsDirectBodyState2D final
{
	GodotBody2D* body = nullptr;

public:
	PhysicsDirectBodyState2D() = default;
	explicit PhysicsDirectBodyState2D(GodotBody2D* p_body);

	Vector2 get_total_gravity() const;
	real_t get_total_linear_damp() const;
	real_t get_total_angular_damp() const;

	Vector2 get_center_of_mass() const;
	Vector2 get_center_of_mass_local() const;
	real_t get_inverse_mass() const;
	real_t get_inverse_inertia() const;

	void set_linear_velocity(const Vector2& p_velocity);
	Vector2 get_linear_velocity() const;

	void set_angular_velocity(real_t p_velocity);
	real_t get_angular_velocity() const;

	void set_transform(const Transform2D& p_transform);
	Transform2D get_transform() const;

	Vector2 get_velocity_at_local_position(const Vector2& p_position) const;

	void apply_central_impulse(const Vector2& p_impulse);
	void apply_torque_impulse(real_t p_torque);
	void apply_impulse(const Vector2& p_impulse, const Vector2& p_position = Vector2());

	void apply_central_force(const Vector2& p_force);
	void apply_force(const Vector2& p_force, const Vector2& p_position = Vector2());
	void apply_torque(real_t p_torque);

	void add_constant_central_force(const Vector2& p_force);
	void add_constant_force(const Vector2& p_force, const Vector2& p_position = Vector2());
	void add_constant_torque(real_t p_torque);

	void set_constant_force(const Vector2& p_force);
	Vector2 get_constant_force() const;

	void set_constant_torque(real_t p_torque);
	real_t get_constant_torque() const;

	void set_sleep_state(bool p_enable);
	bool is_sleeping() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	int get_contact_count() const;

	Vector2 get_contact_local_position(int p_contact_idx) const;
	Vector2 get_contact_local_normal(int p_contact_idx) const;
	int get_contact_local_shape(int p_contact_idx) const;
	Vector2 get_contact_local_velocity_at_position(int p_contact_idx) const;

	RID get_contact_collider(int p_contact_idx) const;
	Vector2 get_contact_collider_position(int p_contact_idx) const;
	int get_contact_collider_shape(int p_contact_idx) const;
	Vector2 get_contact_collider_velocity_at_position(int p_contact_idx) const;
	Vector2 get_contact_impulse(int p_contact_idx) const;

	real_t get_step() const;
	void integrate_forces();

	PhysicsDirectSpaceState2D* get_space_state();
};
