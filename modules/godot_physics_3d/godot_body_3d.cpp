/**************************************************************************/
/*  godot_body_3d.cpp                                                     */
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

#include "godot_area_3d.h"
#include "godot_body_3d.h"
#include "godot_body_direct_state_3d.h"
#include "godot_constraint_3d.h"
#include "godot_space_3d.h"

void GodotBody3D::_mass_properties_changed()
{
	if (get_space() && !mass_properties_update_list.in_list()) {
		get_space()->body_add_to_mass_properties_update_list(&mass_properties_update_list);
	}
}

void GodotBody3D::_update_transform_dependent()
{
	center_of_mass = get_transform().basis.xform(center_of_mass_local);
	principal_inertia_axes = get_transform().basis * principal_inertia_axes_local;

	// Update inertia tensor.
	Basis tb = principal_inertia_axes;
	Basis tbt = tb.transposed();
	Basis diag;
	diag.scale(_inv_inertia);
	_inv_inertia_tensor = tb * diag * tbt;
}

void GodotBody3D::update_mass_properties()
{
	// Update shapes and motions.

	switch (mode) {
	case PS3DE::BODY_MODE_RIGID: {
		real_t total_area = 0;
		for (int i = 0; i < get_shape_count(); i++) {
			if (is_shape_disabled(i)) {
				continue;
			}

			total_area += get_shape_area(i);
		}

		if (calculate_center_of_mass) {
			// We have to recompute the center of mass.
			center_of_mass_local.zero();

			if (total_area != 0.0) {
				for (int i = 0; i < get_shape_count(); i++) {
					if (is_shape_disabled(i)) {
						continue;
					}

					real_t area = get_shape_area(i);

					real_t mass_new = area * mass / total_area;

					// NOTE: we assume that the shape origin is also its center of mass.
					center_of_mass_local += mass_new * get_shape_transform(i).origin;
				}

				center_of_mass_local /= mass;
			}
		}

		if (calculate_inertia) {
			// Recompute the inertia tensor.
			Basis inertia_tensor;
			inertia_tensor.set_zero();
			bool inertia_set = false;

			for (int i = 0; i < get_shape_count(); i++) {
				if (is_shape_disabled(i)) {
					continue;
				}

				real_t area = get_shape_area(i);
				if (area == 0.0) {
					continue;
				}

				inertia_set = true;

				const GodotShape3D* shape = get_shape(i);

				real_t mass_new = area * mass / total_area;

				Basis shape_inertia_tensor =
					Basis::from_scale(shape->get_moment_of_inertia(mass_new));
				Transform3D shape_transform = get_shape_transform(i);
				Basis shape_basis = shape_transform.basis.orthonormalized();

				// NOTE: we don't take the scale of collision shapes into account when computing the
				// inertia tensor!
				shape_inertia_tensor =
					shape_basis * shape_inertia_tensor * shape_basis.transposed();

				Vector3 shape_origin = shape_transform.origin - center_of_mass_local;
				inertia_tensor += shape_inertia_tensor + (Basis() * shape_origin.dot(shape_origin) -
															 shape_origin.outer(shape_origin)) *
															 mass_new;
			}

			// Set the inertia to a valid value when there are no valid shapes.
			if (!inertia_set) {
				inertia_tensor = Basis();
			}

			// Handle partial custom inertia.
			if (inertia.x > 0.0) {
				inertia_tensor[0][0] = inertia.x;
			}
			if (inertia.y > 0.0) {
				inertia_tensor[1][1] = inertia.y;
			}
			if (inertia.z > 0.0) {
				inertia_tensor[2][2] = inertia.z;
			}

			// Compute the principal axes of inertia.
			principal_inertia_axes_local = inertia_tensor.diagonalize().transposed();
			_inv_inertia = inertia_tensor.get_main_diagonal().inverse();
		}

		if (mass) {
			_inv_mass = 1.0 / mass;
		}
		else {
			_inv_mass = 0;
		}

	} break;
	case PS3DE::BODY_MODE_KINEMATIC:
	case PS3DE::BODY_MODE_STATIC: {
		_inv_inertia = Vector3();
		_inv_mass = 0;
	} break;
	case PS3DE::BODY_MODE_RIGID_LINEAR: {
		_inv_inertia_tensor.set_zero();
		_inv_mass = 1.0 / mass;

	} break;
	}

	_update_transform_dependent();
}

void GodotBody3D::reset_mass_properties()
{
	calculate_inertia = true;
	calculate_center_of_mass = true;
	_mass_properties_changed();
}

void GodotBody3D::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;

	if (active) {
		if (mode == PS3DE::BODY_MODE_STATIC) {
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

void GodotBody3D::set_mode(PS3DE::BodyMode p_mode)
{
	PS3DE::BodyMode prev = mode;
	mode = p_mode;

	switch (p_mode) {
	case PS3DE::BODY_MODE_STATIC:
	case PS3DE::BODY_MODE_KINEMATIC: {
		_set_inv_transform(get_transform().affine_inverse());
		_inv_mass = 0;
		_inv_inertia = Vector3();
		_set_static(p_mode == PS3DE::BODY_MODE_STATIC);
		set_active(p_mode == PS3DE::BODY_MODE_KINEMATIC && contacts.size());
		linear_velocity = Vector3();
		angular_velocity = Vector3();
		if (mode == PS3DE::BODY_MODE_KINEMATIC && prev != mode) {
			first_time_kinematic = true;
		}
		_update_transform_dependent();

	} break;
	case PS3DE::BODY_MODE_RIGID: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		if (!calculate_inertia) {
			principal_inertia_axes_local = Basis();
			_inv_inertia = inertia.inverse();
			_update_transform_dependent();
		}
		_mass_properties_changed();
		_set_static(false);
		set_active(true);

	} break;
	case PS3DE::BODY_MODE_RIGID_LINEAR: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		_inv_inertia = Vector3();
		angular_velocity = Vector3();
		_update_transform_dependent();
		_set_static(false);
		set_active(true);
	}
	}
}

PS3DE::BodyMode GodotBody3D::get_mode() const { return mode; }

void GodotBody3D::_shapes_changed()
{
	_mass_properties_changed();
	wakeup();
	wakeup_neighbours();
}

void GodotBody3D::set_space(GodotSpace3D* p_space)
{
	if (get_space()) {
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

void GodotBody3D::set_axis_lock(PS3DE::BodyAxis p_axis, bool lock)
{
	if (lock) {
		locked_axis |= p_axis;
	}
	else {
		locked_axis &= ~p_axis;
	}
}

bool GodotBody3D::is_axis_locked(PS3DE::BodyAxis p_axis) const { return locked_axis & p_axis; }

void GodotBody3D::wakeup_neighbours()
{
	for (const KeyValue<GodotConstraint3D*, int>& E : constraint_map) {
		const GodotConstraint3D* c = E.key;
		GodotBody3D** n = c->get_body_ptr();
		int bc = c->get_body_count();

		for (int i = 0; i < bc; i++) {
			if (i == E.value) {
				continue;
			}
			GodotBody3D* b = n[i];
			if (b->mode < PS3DE::BODY_MODE_RIGID) {
				continue;
			}

			if (!b->is_active()) {
				b->set_active(true);
			}
		}
	}
}

bool GodotBody3D::sleep_test(real_t p_step)
{
	if (mode == PS3DE::BODY_MODE_STATIC || mode == PS3DE::BODY_MODE_KINEMATIC) {
		return true;
	}
	else if (!can_sleep) {
		return false;
	}

	ERR_FAIL_NULL_V(get_space(), true);

	if (Math::abs(angular_velocity.length()) <
			get_space()->get_body_angular_velocity_sleep_threshold() &&
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

GodotPhysicsDirectBodyState3D* GodotBody3D::get_direct_state()
{
	if (!direct_state) {
		direct_state = memnew(GodotPhysicsDirectBodyState3D);
		direct_state->body = this;
	}
	return direct_state;
}

GodotBody3D::GodotBody3D()
	: GodotCollisionObject3D(TYPE_BODY), active_list(this), mass_properties_update_list(this),
	  direct_state_query_list(this)
{
	_set_static(false);
}

GodotBody3D::~GodotBody3D() { memdelete(direct_state); }


