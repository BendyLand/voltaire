/**************************************************************************/
/*  godot_area_3d.cpp                                                     */
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
#include "godot_soft_body_3d.h"
#include "godot_space_3d.h"

GodotArea3D::BodyKey::BodyKey(GodotSoftBody3D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

GodotArea3D::BodyKey::BodyKey(GodotBody3D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

GodotArea3D::BodyKey::BodyKey(GodotArea3D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

void GodotArea3D::_shapes_changed()
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}
}

void GodotArea3D::set_transform(const Transform3D& p_transform)
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}

	_set_transform(p_transform);
	_set_inv_transform(p_transform.affine_inverse());
}

void GodotArea3D::set_space(GodotSpace3D* p_space)
{
	if (get_space()) {
		if (monitor_query_list.in_list()) {
			get_space()->area_remove_from_monitor_query_list(&monitor_query_list);
		}
		if (moved_list.in_list()) {
			get_space()->area_remove_from_moved_list(&moved_list);
		}
	}

	monitored_bodies.clear();
	monitored_areas.clear();

	_set_space(p_space);

	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}
}

void GodotArea3D::_set_space_override_mode(
	PS3DE::AreaSpaceOverrideMode& r_mode, PS3DE::AreaSpaceOverrideMode p_new_mode)
{
	bool do_override = p_new_mode != PS3DE::AREA_SPACE_OVERRIDE_DISABLED;
	if (do_override == (r_mode != PS3DE::AREA_SPACE_OVERRIDE_DISABLED)) {
		return;
	}
	_unregister_shapes();
	r_mode = p_new_mode;
	_shape_changed();
}

void GodotArea3D::_queue_monitor_update()
{
	ERR_FAIL_NULL(get_space());

	if (!monitor_query_list.in_list()) {
		get_space()->area_add_to_monitor_query_list(&monitor_query_list);
	}
}

void GodotArea3D::set_monitorable(bool p_monitorable)
{
	if (monitorable == p_monitorable) {
		return;
	}

	monitorable = p_monitorable;
	_set_static(!monitorable);
	_shapes_changed();
}

void GodotArea3D::compute_gravity(const Vector3& p_position, Vector3& r_gravity) const
{
	if (is_gravity_point()) {
		const real_t gr_unit_dist = get_gravity_point_unit_distance();
		Vector3 v = get_transform().xform(get_gravity_vector()) - p_position;
		if (gr_unit_dist > 0) {
			const real_t v_length_sq = v.length_squared();
			if (v_length_sq > 0) {
				const real_t gravity_strength =
					get_gravity() * gr_unit_dist * gr_unit_dist / v_length_sq;
				r_gravity = v.normalized() * gravity_strength;
			}
			else {
				r_gravity = Vector3();
			}
		}
		else {
			r_gravity = v.normalized() * get_gravity();
		}
	}
	else {
		r_gravity = get_gravity_vector() * get_gravity();
	}
}

GodotArea3D::GodotArea3D()
	: GodotCollisionObject3D(TYPE_AREA), monitor_query_list(this), moved_list(this)
{
	_set_static(true); // areas are never active
	set_ray_pickable(false);
}

GodotArea3D::~GodotArea3D() {}


