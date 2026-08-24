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

#include "core/object/class_db.h"
#include "core/variant/typed_array.h"
#include "physics_direct_space_state_3d.h"

Dictionary PhysicsDirectSpaceState3D::_intersect_ray(PhysicsRayQueryParameters3D* rp_ray_query)
{
	PS3DT::RayResult result;
	bool res = intersect_ray(rp_ray_query->get_parameters(), result);

	if (!res) {
		return Dictionary();
	}

	Dictionary d;
	d["position"] = result.position;
	d["normal"] = result.normal;
	d["face_index"] = result.face_index;
	d["collider_id"] = result.collider_id;
	d["collider"] = result.collider;
	d["shape"] = result.shape;
	d["rid"] = result.rid;

	return d;
}

Array PhysicsDirectSpaceState3D::_intersect_point(
	PhysicsPointQueryParameters3D* rp_point_query, int p_max_results)
{
	Vector<PS3DT::ShapeResult> ret;
	ret.resize(p_max_results);
	int rc = intersect_point(rp_point_query->get_parameters(), ret.ptrw(), ret.size());

	if (rc == 0) {
		return TypedArray<Dictionary>();
	}

	TypedArray<Dictionary> r;
	r.resize(rc);
	for (int i = 0; i < rc; i++) {
		Dictionary d;
		d["rid"] = ret[i].rid;
		d["collider_id"] = ret[i].collider_id;
		d["collider"] = ret[i].collider;
		d["shape"] = ret[i].shape;
		r[i] = d;
	}
	return r;
}

Array PhysicsDirectSpaceState3D::_intersect_shape(
	PhysicsShapeQueryParameters3D* rp_shape_query, int p_max_results)
{
	Vector<PS3DT::ShapeResult> sr;
	sr.resize(p_max_results);
	int rc = intersect_shape(rp_shape_query->get_parameters(), sr.ptrw(), sr.size());
	TypedArray<Dictionary> ret;
	ret.resize(rc);
	for (int i = 0; i < rc; i++) {
		Dictionary d;
		d["rid"] = sr[i].rid;
		d["collider_id"] = sr[i].collider_id;
		d["collider"] = sr[i].collider;
		d["shape"] = sr[i].shape;
		ret[i] = d;
	}

	return ret;
}

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

Array PhysicsDirectSpaceState3D::_collide_shape(
	PhysicsShapeQueryParameters3D* rp_shape_query, int p_max_results)
{
	Vector<Vector3> ret;
	ret.resize(p_max_results * 2);
	int rc = 0;
	bool res = collide_shape(rp_shape_query->get_parameters(), ret.ptrw(), p_max_results, rc);
	if (!res) {
		return TypedArray<Vector3>();
	}
	TypedArray<Vector3> r;
	r.resize(rc * 2);
	for (int i = 0; i < rc * 2; i++) {
		r[i] = ret[i];
	}
	return r;
}

Dictionary PhysicsDirectSpaceState3D::_get_rest_info(PhysicsShapeQueryParameters3D* rp_shape_query)
{
	PS3DT::ShapeRestInfo sri;
	bool res = rest_info(rp_shape_query->get_parameters(), &sri);
	Dictionary r;
	if (!res) {
		return r;
	}

	r["point"] = sri.point;
	r["normal"] = sri.normal;
	r["rid"] = sri.rid;
	r["collider_id"] = sri.collider_id;
	r["shape"] = sri.shape;
	r["linear_velocity"] = sri.linear_velocity;

	return r;
}

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D() {}

void PhysicsDirectSpaceState3D::_bind_methods() {}


