/**************************************************************************/
/*  physics_direct_space_state_3d.h                                       */
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

#include "servers/physics_3d/physics_server_3d_types.h"
#include "servers/physics_3d/queries/physics_point_query_parameters_3d.h"
#include "servers/physics_3d/queries/physics_ray_query_parameters_3d.h"
#include "servers/physics_3d/queries/physics_shape_query_parameters_3d.h"

class JoltBody3D;
class JoltSpace3D;

class PhysicsDirectSpaceState3D final
{
	JoltSpace3D* space = nullptr;

	Vector<real_t> _cast_motion(PhysicsShapeQueryParameters3D* rp_shape_query);

public:
	PhysicsDirectSpaceState3D() = default;
	explicit PhysicsDirectSpaceState3D(JoltSpace3D* p_space);

	bool intersect_ray(
		const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result);
	int intersect_point(const PS3DT::PointParameters& p_parameters,
		PS3DT::ShapeResult* r_results, int p_result_max);
	int intersect_shape(const PS3DT::ShapeParameters& p_parameters,
		PS3DT::ShapeResult* r_results, int p_result_max);
	bool cast_motion(const PS3DT::ShapeParameters& p_parameters, real_t& r_closest_safe,
		real_t& r_closest_unsafe, PS3DT::ShapeRestInfo* r_info = nullptr);
	bool collide_shape(const PS3DT::ShapeParameters& p_parameters, Vector3* r_results,
		int p_result_max, int& r_result_count);
	bool rest_info(
		const PS3DT::ShapeParameters& p_parameters, PS3DT::ShapeRestInfo* r_info);
	Vector3 get_closest_point_to_object_volume(
		RID p_object, Vector3 p_point) const;

	bool body_test_motion(const JoltBody3D& p_body, const PS3DT::MotionParameters& p_parameters,
		PS3DT::MotionResult* r_result) const;

	JoltSpace3D& get_space() const { return *space; }
};
