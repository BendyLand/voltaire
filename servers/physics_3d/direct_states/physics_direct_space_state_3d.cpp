/**************************************************************************/
/*  physics_direct_space_state_3d.cpp                                     */
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

#include "physics_direct_space_state_3d.h"

#include "modules/modules_enabled.gen.h"

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

#ifndef MODULE_JOLT_PHYSICS_ENABLED

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D(JoltSpace3D* p_space) : space(p_space) {}

bool PhysicsDirectSpaceState3D::intersect_ray(
	const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result)
{
	return false;
}

int PhysicsDirectSpaceState3D::intersect_point(
	const PS3DT::PointParameters& p_point_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(
	const PS3DT::ShapeParameters& p_shape_params, real_t& r_closest_safe,
	real_t& r_closest_unsafe, PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

bool PhysicsDirectSpaceState3D::collide_shape(
	const PS3DT::ShapeParameters& p_shape_params, Vector3* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(
	RID p_object, Vector3 p_point) const
{
	return Vector3();
}

bool PhysicsDirectSpaceState3D::body_test_motion(const JoltBody3D& p_body,
	const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result) const
{
	return false;
}

#endif // !MODULE_JOLT_PHYSICS_ENABLED
