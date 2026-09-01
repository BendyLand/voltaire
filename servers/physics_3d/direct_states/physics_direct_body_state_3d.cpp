/**************************************************************************/
/*  physics_direct_body_state_3d.cpp                                      */
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

#include "physics_direct_body_state_3d.h"

void PhysicsDirectBodyState3D::integrate_forces()
{
	real_t step = get_step();
	Vector3 lv = get_linear_velocity();
	lv += get_total_gravity() * step;

	Vector3 av = get_angular_velocity();

	real_t linear_damp = 1.0 - step * get_total_linear_damp();

	if (linear_damp < 0) { // reached zero in the given time
		linear_damp = 0;
	}

	real_t angular_damp = 1.0 - step * get_total_angular_damp();

	if (angular_damp < 0) { // reached zero in the given time
		angular_damp = 0;
	}

	lv *= linear_damp;
	av *= angular_damp;

	set_linear_velocity(lv);
	set_angular_velocity(av);
}

Object* PhysicsDirectBodyState3D::get_contact_collider_object(int p_contact_idx) const
{
	ObjectID objid = get_contact_collider_id(p_contact_idx);
	Object* obj = ObjectDB::get_instance(objid);
	return obj;
}

void PhysicsDirectBodyState3D::_bind_methods() {}

PhysicsDirectBodyState3D::PhysicsDirectBodyState3D() {}


