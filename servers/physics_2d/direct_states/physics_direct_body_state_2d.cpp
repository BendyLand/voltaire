/**************************************************************************/
/*  physics_direct_body_state_2d.cpp                                      */
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

#include "physics_direct_body_state_2d.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_GODOT_PHYSICS_2D_ENABLED

PhysicsDirectBodyState2D::PhysicsDirectBodyState2D(GodotBody2D* p_body) : body(p_body) {}

Vector2 PhysicsDirectBodyState2D::get_total_gravity() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_total_linear_damp() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_total_angular_damp() const { return 0.0; }

Vector2 PhysicsDirectBodyState2D::get_center_of_mass() const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_center_of_mass_local() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_inverse_mass() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_inverse_inertia() const { return 0.0; }

void PhysicsDirectBodyState2D::set_linear_velocity(const Vector2& p_velocity) {}
Vector2 PhysicsDirectBodyState2D::get_linear_velocity() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_angular_velocity(real_t p_velocity) {}
real_t PhysicsDirectBodyState2D::get_angular_velocity() const { return 0.0; }

void PhysicsDirectBodyState2D::set_transform(const Transform2D& p_transform) {}
Transform2D PhysicsDirectBodyState2D::get_transform() const { return Transform2D(); }

Vector2 PhysicsDirectBodyState2D::get_velocity_at_local_position(const Vector2& p_position) const { return Vector2(); }

void PhysicsDirectBodyState2D::apply_central_impulse(const Vector2& p_impulse) {}
void PhysicsDirectBodyState2D::apply_torque_impulse(real_t p_torque) {}
void PhysicsDirectBodyState2D::apply_impulse(const Vector2& p_impulse, const Vector2& p_position) {}

void PhysicsDirectBodyState2D::apply_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::apply_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::apply_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::add_constant_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::add_constant_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::add_constant_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::set_constant_force(const Vector2& p_force) {}
Vector2 PhysicsDirectBodyState2D::get_constant_force() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_constant_torque(real_t p_torque) {}
real_t PhysicsDirectBodyState2D::get_constant_torque() const { return 0.0; }

void PhysicsDirectBodyState2D::set_sleep_state(bool p_enable) {}
bool PhysicsDirectBodyState2D::is_sleeping() const { return false; }

void PhysicsDirectBodyState2D::set_collision_layer(uint32_t p_layer) {}
uint32_t PhysicsDirectBodyState2D::get_collision_layer() const { return 0; }

void PhysicsDirectBodyState2D::set_collision_mask(uint32_t p_mask) {}
uint32_t PhysicsDirectBodyState2D::get_collision_mask() const { return 0; }

int PhysicsDirectBodyState2D::get_contact_count() const { return 0; }

Vector2 PhysicsDirectBodyState2D::get_contact_local_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_local_normal(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_local_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_local_velocity_at_position(int p_contact_idx) const { return Vector2(); }

RID PhysicsDirectBodyState2D::get_contact_collider(int p_contact_idx) const { return RID(); }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_position(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_collider_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_velocity_at_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_impulse(int p_contact_idx) const { return Vector2(); }

real_t PhysicsDirectBodyState2D::get_step() const { return 0.0; }

void PhysicsDirectBodyState2D::integrate_forces() {}

PhysicsDirectSpaceState2D* PhysicsDirectBodyState2D::get_space_state() { return nullptr; }

#endif // !MODULE_GODOT_PHYSICS_2D_ENABLED
