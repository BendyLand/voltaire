/**************************************************************************/
/*  physics_direct_space_state_3d.cpp                                     */
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

#include "physics_direct_space_state_3d.h"

Vector<real_t> PhysicsDirectSpaceState3D::_cast_motion(
	PhysicsShapeQueryParameters3D* rp_shape_query)
{
	real_t closest_safe = 1.0f, closest_unsafe = 1.0f;
	bool res = cast_motion(rp_shape_query->get_parameters(), closest_safe, closest_unsafe);
	if (!res) {
		return Vector<real_t>();
	}
	Vector<real_t> ret;
	ret.resize(2);
	ret.write[0] = closest_safe;
	ret.write[1] = closest_unsafe;
	return ret;
}

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D() {}

<<<<<<< HEAD
=======
bool PhysicsDirectSpaceState3D::intersect_ray(
	const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result)
{
	return true;
}

// servers/physics_3d/direct_states/physics_direct_space_state_3d.h / .cpp
int PhysicsDirectSpaceState3D::intersect_point(
	const PhysicsServer3DTypes::PointParameters& p_point_params,
	PhysicsServer3DTypes::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(
	const PhysicsServer3DTypes::ShapeParameters& p_shape_params,
	PhysicsServer3DTypes::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(
	const PhysicsServer3DTypes::ShapeParameters& p_shape_params, float& r_closest_safe,
	float& r_closest_unsafe, PhysicsServer3DTypes::ShapeRestInfo* r_info)
{
	return false;
}

bool PhysicsDirectSpaceState3D::collide_shape(
	const PhysicsServer3DTypes::ShapeParameters& p_shape_params, Vector3* r_results,
	int p_result_max, int& r_result_count)
{
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(
	const PhysicsServer3DTypes::ShapeParameters& p_shape_params,
	PhysicsServer3DTypes::ShapeRestInfo* r_info)
{
	return false;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(
	RID p_object, Vector3 p_point) const
{
	return Vector3();
}

>>>>>>> fix/remove-object

