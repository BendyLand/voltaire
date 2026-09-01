/**************************************************************************/
/*  physics_shape_query_parameters_3d.cpp                                 */
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

#include "core/io/resource.h"
#include "physics_shape_query_parameters_3d.h"

Ref<Resource> PhysicsShapeQueryParameters3D::get_shape() const { return shape_ref; }

void PhysicsShapeQueryParameters3D::set_shape(const Ref<Resource>& p_shape_ref)
{
	ERR_FAIL_COND(p_shape_ref.is_null());
	shape_ref = p_shape_ref;
	parameters.shape_rid = p_shape_ref->get_rid();
}

void PhysicsShapeQueryParameters3D::set_shape_rid(const RID& p_shape)
{
	if (parameters.shape_rid != p_shape) {
		shape_ref = Ref<Resource>();
		parameters.shape_rid = p_shape;
	}
}

void PhysicsShapeQueryParameters3D::set_exclude(const TypedArray<RID>& p_exclude)
{
	parameters.exclude.clear();
	for (int i = 0; i < p_exclude.size(); i++) {
		parameters.exclude.insert(p_exclude[i]);
	}
}

TypedArray<RID> PhysicsShapeQueryParameters3D::get_exclude() const
{
	TypedArray<RID> ret;
	ret.resize(parameters.exclude.size());
	int idx = 0;
	for (const RID& E : parameters.exclude) {
		ret[idx++] = E;
	}
	return ret;
}

void PhysicsShapeQueryParameters3D::_bind_methods() {}


