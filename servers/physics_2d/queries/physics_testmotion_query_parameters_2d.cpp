/**************************************************************************/
/*  physics_testmotion_query_parameters_2d.cpp                            */
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

#include "core/object/class_db.h"
#include "core/variant/typed_array.h"
#include "physics_testmotion_query_parameters_2d.h"

TypedArray<RID> PhysicsTestMotionParameters2D::get_exclude_bodies() const
{
	TypedArray<RID> exclude;
	exclude.resize(parameters.exclude_bodies.size());

	int body_index = 0;
	for (RID body : parameters.exclude_bodies) {
		exclude[body_index++] = body;
	}

	return exclude;
}

void PhysicsTestMotionParameters2D::set_exclude_bodies(const TypedArray<RID>& p_exclude)
{
	parameters.exclude_bodies.clear();
	for (int i = 0; i < p_exclude.size(); i++) {
		parameters.exclude_bodies.insert(p_exclude[i]);
	}
}

TypedArray<uint64_t> PhysicsTestMotionParameters2D::get_exclude_objects() const
{
	TypedArray<uint64_t> exclude;
	exclude.resize(parameters.exclude_objects.size());

	int object_index = 0;
	for (ObjectID object_id : parameters.exclude_objects) {
		exclude[object_index++] = object_id;
	}

	return exclude;
}

void PhysicsTestMotionParameters2D::set_exclude_objects(const TypedArray<uint64_t>& p_exclude)
{
	parameters.exclude_objects.clear();
	for (int i = 0; i < p_exclude.size(); ++i) {
		ObjectID object_id = p_exclude[i];
		ERR_CONTINUE(object_id.is_null());
		parameters.exclude_objects.insert(object_id);
	}
}

void PhysicsTestMotionParameters2D::_bind_methods() {}


