/**************************************************************************/
/*  shape_2d.cpp                                                          */
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

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "shape_2d.h"

RID Shape2D::get_rid() const { return shape; }

void Shape2D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer2D::get_singleton()->shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape2D::get_custom_solver_bias() const { return custom_bias; }

bool Shape2D::collide_with_motion(const Transform2D& p_local_xform, const Vector2& p_local_motion,
	Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	int r;
	return PhysicsServer2D::get_singleton()->shape_collide(get_rid(), p_local_xform, p_local_motion,
		rp_shape->get_rid(), p_shape_xform, p_shape_motion, nullptr, 0, r);
}

bool Shape2D::collide(
	const Transform2D& p_local_xform, Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	int r;
	return PhysicsServer2D::get_singleton()->shape_collide(get_rid(), p_local_xform, Vector2(),
		rp_shape->get_rid(), p_shape_xform, Vector2(), nullptr, 0, r);
}

PackedVector2Array Shape2D::collide_with_motion_and_get_contacts(const Transform2D& p_local_xform,
	const Vector2& p_local_motion, Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::get_singleton()->shape_collide(get_rid(), p_local_xform, p_local_motion,
			rp_shape->get_rid(), p_shape_xform, p_shape_motion, result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

PackedVector2Array Shape2D::collide_and_get_contacts(const Transform2D& p_local_xform,
	Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::get_singleton()->shape_collide(get_rid(), p_local_xform, Vector2(),
			rp_shape->get_rid(), p_shape_xform, Vector2(), result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

void Shape2D::_bind_methods() {}

bool Shape2D::is_collision_outline_enabled()
{
	return GLOBAL_GET_CACHED(bool, "debug/shapes/collision/draw_2d_outlines");
}

Shape2D::Shape2D(const RID& p_rid) { shape = p_rid; }

Shape2D::~Shape2D()
{
	ERR_FAIL_NULL(PhysicsServer2D::get_singleton());
	PhysicsServer2D::get_singleton()->free_rid(shape);
}


