/**************************************************************************/
/*  godot_body_2d.cpp                                                     */
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

#include "godot_area_2d.h"
#include "godot_body_2d.h"
#include "godot_body_direct_state_2d.h"
#include "godot_constraint_2d.h"
#include "godot_space_2d.h"

void GodotBody2D::_mass_properties_changed()
{
	if (get_space() && !mass_properties_update_list.in_list()) {
		get_space()->body_add_to_mass_properties_update_list(&mass_properties_update_list);
	}
}

void GodotBody2D::update_mass_properties()
{
	// update shapes and motions

	switch (mode) {
	case PS2DE::BODY_MODE_RIGID: {
		real_t total_area = 0;
		for (int i = 0; i < get_shape_count(); i++) {
			if (is_shape_disabled(i)) {
				continue;
			}
			total_area += get_shape_aabb(i).get_area();
		}

		if (calculate_center_of_mass) {
			// We have to recompute the center of mass.
			center_of_mass_local = Vector2();

			if (total_area != 0.0) {
				for (int i = 0; i < get_shape_count(); i++) {
					if (is_shape_disabled(i)) {
						continue;
					}

					real_t area = get_shape_aabb(i).get_area();

					real_t mass_new = area * mass / total_area;

					// NOTE: we assume that the shape origin is also its center of mass.
					center_of_mass_local += mass_new * get_shape_transform(i).get_origin();
				}

				center_of_mass_local /= mass;
			}
		}

		if (calculate_inertia) {
			inertia = 0;

			for (int i = 0; i < get_shape_count(); i++) {
				if (is_shape_disabled(i)) {
					continue;
				}

				const GodotShape2D* shape = get_shape(i);

				real_t area = get_shape_aabb(i).get_area();
				if (area == 0.0) {
					continue;
				}

				real_t mass_new = area * mass / total_area;

				Transform2D mtx = get_shape_transform(i);
				Vector2 scale = mtx.get_scale();
				Vector2 shape_origin = mtx.get_origin() - center_of_mass_local;
				inertia += shape->get_moment_of_inertia(mass_new, scale) +
						   mass_new * shape_origin.length_squared();
			}
		}

		_inv_inertia = inertia > 0.0 ? (1.0 / inertia) : 0.0;

		if (mass) {
			_inv_mass = 1.0 / mass;
		}
		else {
			_inv_mass = 0;
		}

	} break;
	case PS2DE::BODY_MODE_KINEMATIC:
	case PS2DE::BODY_MODE_STATIC: {
		_inv_inertia = 0;
		_inv_mass = 0;
	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_inertia = 0;
		_inv_mass = 1.0 / mass;

	} break;
	}

	_update_transform_dependent();
}

void GodotBody2D::reset_mass_properties()
{
	calculate_inertia = true;
	calculate_center_of_mass = true;
	_mass_properties_changed();
}

void GodotBody2D::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;

	if (active) {
		if (mode == PS2DE::BODY_MODE_STATIC) {
			// Static bodies can't be active.
			active = false;
		}
		else if (get_space()) {
			get_space()->body_add_to_active_list(&active_list);
		}
	}
	else if (get_space()) {
		get_space()->body_remove_from_active_list(&active_list);
	}
}

void GodotBody2D::set_mode(PS2DE::BodyMode p_mode)
{
	PS2DE::BodyMode prev = mode;
	mode = p_mode;

	switch (p_mode) {
	// CLEAR UP EVERYTHING IN CASE IT NOT WORKS!
	case PS2DE::BODY_MODE_STATIC:
	case PS2DE::BODY_MODE_KINEMATIC: {
		_set_inv_transform(get_transform().affine_inverse());
		_inv_mass = 0;
		_inv_inertia = 0;
		_set_static(p_mode == PS2DE::BODY_MODE_STATIC);
		set_active(p_mode == PS2DE::BODY_MODE_KINEMATIC && contacts.size());
		linear_velocity = Vector2();
		angular_velocity = 0;
		if (mode == PS2DE::BODY_MODE_KINEMATIC && prev != mode) {
			first_time_kinematic = true;
		}
	} break;
	case PS2DE::BODY_MODE_RIGID: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		if (!calculate_inertia) {
			_inv_inertia = 1.0 / inertia;
		}
		_mass_properties_changed();
		_set_static(false);
		set_active(true);

	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		_inv_inertia = 0;
		angular_velocity = 0;
		_set_static(false);
		set_active(true);
	}
	}
}

PS2DE::BodyMode GodotBody2D::get_mode() const { return mode; }

void GodotBody2D::_shapes_changed()
{
	_mass_properties_changed();
	wakeup();
	wakeup_neighbours();
}

void GodotBody2D::set_space(GodotSpace2D* p_space)
{
	if (get_space()) {
		wakeup_neighbours();

		if (mass_properties_update_list.in_list()) {
			get_space()->body_remove_from_mass_properties_update_list(&mass_properties_update_list);
		}
		if (active_list.in_list()) {
			get_space()->body_remove_from_active_list(&active_list);
		}
		if (direct_state_query_list.in_list()) {
			get_space()->body_remove_from_state_query_list(&direct_state_query_list);
		}
	}

	_set_space(p_space);

	if (get_space()) {
		_mass_properties_changed();

		if (active && !active_list.in_list()) {
			get_space()->body_add_to_active_list(&active_list);
		}
	}
}

void GodotBody2D::_update_transform_dependent()
{
	center_of_mass = get_transform().basis_xform(center_of_mass_local);
}

void GodotBody2D::wakeup_neighbours()
{
	for (const Pair<GodotConstraint2D*, int>& E : constraint_list) {
		const GodotConstraint2D* c = E.first;
		GodotBody2D** n = c->get_body_ptr();
		int bc = c->get_body_count();

		for (int i = 0; i < bc; i++) {
			if (i == E.second) {
				continue;
			}
			GodotBody2D* b = n[i];
			if (b->mode < PS2DE::BODY_MODE_RIGID) {
				continue;
			}

			if (!b->is_active()) {
				b->set_active(true);
			}
		}
	}
}

bool GodotBody2D::sleep_test(real_t p_step)
{
	if (mode == PS2DE::BODY_MODE_STATIC || mode == PS2DE::BODY_MODE_KINEMATIC) {
		return true;
	}
	else if (!can_sleep) {
		return false;
	}

	ERR_FAIL_NULL_V(get_space(), true);

	if (Math::abs(angular_velocity) < get_space()->get_body_angular_velocity_sleep_threshold() &&
		Math::abs(linear_velocity.length_squared()) <
			get_space()->get_body_linear_velocity_sleep_threshold() *
				get_space()->get_body_linear_velocity_sleep_threshold()) {
		still_time += p_step;

		return still_time > get_space()->get_body_time_to_sleep();
	}
	else {
		still_time = 0; // maybe this should be set to 0 on set_active?
		return false;
	}
}

GodotPhysicsDirectBodyState2D* GodotBody2D::get_direct_state()
{
	if (!direct_state) {
		direct_state = memnew(GodotPhysicsDirectBodyState2D);
		direct_state->body = this;
	}
	return direct_state;
}

GodotBody2D::GodotBody2D()
	: GodotCollisionObject2D(TYPE_BODY), active_list(this), mass_properties_update_list(this),
	  direct_state_query_list(this)
{
	_set_static(false);
}

GodotBody2D::~GodotBody2D() { memdelete(direct_state); }


