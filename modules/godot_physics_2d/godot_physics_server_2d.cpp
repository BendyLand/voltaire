/**************************************************************************/
/*  godot_physics_server_2d.cpp                                           */
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

#include "core/os/os.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#include "godot_physics_server_2d.h"

#define FLUSH_QUERY_CHECK(m_object)                                                                \
	ERR_FAIL_COND_MSG(m_object->get_space() && flushing_queries,                                   \
		"Can't change this state while flushing queries. Use call_deferred() or set_deferred() "   \
		"to change monitoring state instead.");

RID GodotPhysicsServer2D::_shape_create(PS2DE::ShapeType p_shape)
{
	ERR_FAIL_COND_V(p_shape == PS2DE::SHAPE_CUSTOM, RID());

	GodotShape2D* shape = memnew(GodotShape2D(p_shape));

	RID id = shape_owner.make_rid(shape);
	shape->set_self(id);

	return id;
}

RID GodotPhysicsServer2D::world_boundary_shape_create()
{
	return _shape_create(PS2DE::SHAPE_WORLD_BOUNDARY);
}

RID GodotPhysicsServer2D::separation_ray_shape_create()
{
	return _shape_create(PS2DE::SHAPE_SEPARATION_RAY);
}

RID GodotPhysicsServer2D::segment_shape_create() { return _shape_create(PS2DE::SHAPE_SEGMENT); }

RID GodotPhysicsServer2D::circle_shape_create() { return _shape_create(PS2DE::SHAPE_CIRCLE); }

RID GodotPhysicsServer2D::rectangle_shape_create() { return _shape_create(PS2DE::SHAPE_RECTANGLE); }

RID GodotPhysicsServer2D::capsule_shape_create() { return _shape_create(PS2DE::SHAPE_CAPSULE); }

RID GodotPhysicsServer2D::convex_polygon_shape_create()
{
	return _shape_create(PS2DE::SHAPE_CONVEX_POLYGON);
}

RID GodotPhysicsServer2D::concave_polygon_shape_create()
{
	return _shape_create(PS2DE::SHAPE_CONCAVE_POLYGON);
}

void GodotPhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	shape->set_custom_bias(p_bias);
}

PS2DE::ShapeType GodotPhysicsServer2D::shape_get_type(RID p_shape) const
{
	const GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, PS2DE::SHAPE_CUSTOM);
	return shape->get_type();
}

real_t GodotPhysicsServer2D::shape_get_custom_solver_bias(RID p_shape) const
{
	const GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL_V(shape, 0);
	return shape->get_custom_bias();
}

void GodotPhysicsServer2D::_shape_col_cbk(
	const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata)
{
	CollCbkData* cbk = static_cast<CollCbkData*>(p_userdata);

	if (cbk->max == 0) {
		return;
	}

	Vector2 rel_dir = (p_point_A - p_point_B);
	real_t rel_length2 = rel_dir.length_squared();
	if (cbk->valid_dir != Vector2()) {
		if (cbk->valid_depth < 10e20) {
			if (rel_length2 > cbk->valid_depth * cbk->valid_depth ||
				(rel_length2 > CMP_EPSILON &&
					cbk->valid_dir.dot(rel_dir.normalized()) < CMP_EPSILON)) {
				cbk->invalid_by_dir++;
				return;
			}
		}
		else {
			if (rel_length2 > 0 && cbk->valid_dir.dot(rel_dir.normalized()) < CMP_EPSILON) {
				return;
			}
		}
	}

	if (cbk->amount == cbk->max) {
		// find least deep
		real_t min_depth = 1e20;
		int min_depth_idx = 0;
		for (int i = 0; i < cbk->amount; i++) {
			real_t d = cbk->ptr[i * 2 + 0].distance_squared_to(cbk->ptr[i * 2 + 1]);
			if (d < min_depth) {
				min_depth = d;
				min_depth_idx = i;
			}
		}

		if (rel_length2 < min_depth) {
			return;
		}
		cbk->ptr[min_depth_idx * 2 + 0] = p_point_A;
		cbk->ptr[min_depth_idx * 2 + 1] = p_point_B;
		cbk->passed++;

	}
	else {
		cbk->ptr[cbk->amount * 2 + 0] = p_point_A;
		cbk->ptr[cbk->amount * 2 + 1] = p_point_B;
		cbk->amount++;
		cbk->passed++;
	}
}

bool GodotPhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A,
	const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B,
	const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count)
{
	GodotShape2D* shape_A = shape_owner.get_or_null(p_shape_A);
	ERR_FAIL_NULL_V(shape_A, false);
	GodotShape2D* shape_B = shape_owner.get_or_null(p_shape_B);
	ERR_FAIL_NULL_V(shape_B, false);

	if (p_result_max == 0) {
		return GodotCollisionSolver2D::solve(
			shape_A, p_xform_A, p_motion_A, shape_B, p_xform_B, p_motion_B, nullptr, nullptr);
	}

	CollCbkData cbk;
	cbk.max = p_result_max;
	cbk.amount = 0;
	cbk.passed = 0;
	cbk.ptr = r_results;

	bool res = GodotCollisionSolver2D::solve(
		shape_A, p_xform_A, p_motion_A, shape_B, p_xform_B, p_motion_B, _shape_col_cbk, &cbk);
	r_result_count = cbk.amount;
	return res;
}

RID GodotPhysicsServer2D::space_create()
{
	GodotSpace2D* space = memnew(GodotSpace2D);
	RID id = space_owner.make_rid(space);
	space->set_self(id);
	RID area_id = area_create();
	GodotArea2D* area = area_owner.get_or_null(area_id);
	ERR_FAIL_NULL_V(area, RID());
	space->set_default_area(area);
	area->set_space(space);
	area->set_priority(-1);

	return id;
}

void GodotPhysicsServer2D::space_set_active(RID p_space, bool p_active)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);
	if (p_active) {
		active_spaces.insert(space);
	}
	else {
		active_spaces.erase(space);
	}
}

bool GodotPhysicsServer2D::space_is_active(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, false);

	return active_spaces.has(space);
}

void GodotPhysicsServer2D::space_set_param(
	RID p_space, PS2DE::SpaceParameter p_param, real_t p_value)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);

	space->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param) const
{
	const GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0);
	return space->get_param(p_param);
}

void GodotPhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL(space);
	space->set_debug_contacts(p_max_contacts);
}

Vector<Vector2> GodotPhysicsServer2D::space_get_contacts(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, Vector<Vector2>());
	return space->get_debug_contacts();
}

int GodotPhysicsServer2D::space_get_contact_count(RID p_space) const
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, 0);
	return space->get_debug_contact_count();
}

PhysicsDirectSpaceState2D* GodotPhysicsServer2D::space_get_direct_state(RID p_space)
{
	GodotSpace2D* space = space_owner.get_or_null(p_space);
	ERR_FAIL_NULL_V(space, nullptr);
	ERR_FAIL_COND_V_MSG((using_threads && !doing_sync) || space->is_locked(), nullptr,
		"Space state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return space->get_direct_state();
}

RID GodotPhysicsServer2D::area_create()
{
	GodotArea2D* area = memnew(GodotArea2D);
	RID rid = area_owner.make_rid(area);
	area->set_self(rid);
	return rid;
}

void GodotPhysicsServer2D::area_set_space(RID p_area, RID p_space)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotSpace2D* space = nullptr;
	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	if (area->get_space() == space) {
		return; // pointless
	}

	area->clear_constraints();
	area->set_space(space);
}

RID GodotPhysicsServer2D::area_get_space(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	GodotSpace2D* space = area->get_space();
	if (!space) {
		return RID();
	}
	return space->get_self();
}

void GodotPhysicsServer2D::area_add_shape(
	RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	area->add_shape(shape, p_transform, p_disabled);
}

void GodotPhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	ERR_FAIL_COND(!shape->is_configured());

	area->set_shape(p_shape_idx, shape);
}

void GodotPhysicsServer2D::area_set_shape_transform(
	RID p_area, int p_shape_idx, const Transform2D& p_transform)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_shape_transform(p_shape_idx, p_transform);
}

void GodotPhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	ERR_FAIL_INDEX(p_shape, area->get_shape_count());
	FLUSH_QUERY_CHECK(area);

	area->set_shape_disabled(p_shape, p_disabled);
}

int GodotPhysicsServer2D::area_get_shape_count(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, -1);

	return area->get_shape_count();
}

RID GodotPhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, RID());

	GodotShape2D* shape = area->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_self();
}

Transform2D GodotPhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform2D());

	return area->get_shape_transform(p_shape_idx);
}

void GodotPhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->remove_shape(p_shape_idx);
}

void GodotPhysicsServer2D::area_clear_shapes(RID p_area)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	while (area->get_shape_count()) {
		area->remove_shape(0);
	}
}

void GodotPhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	area->set_transform(p_transform);
}

Transform2D GodotPhysicsServer2D::area_get_transform(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, Transform2D());

	return area->get_transform();
}

void GodotPhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	area->set_pickable(p_pickable);
}

void GodotPhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);
	FLUSH_QUERY_CHECK(area);

	area->set_monitorable(p_monitorable);
}

void GodotPhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_layer(p_layer);
}

uint32_t GodotPhysicsServer2D::area_get_collision_layer(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_layer();
}

void GodotPhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL(area);

	area->set_collision_mask(p_mask);
}

uint32_t GodotPhysicsServer2D::area_get_collision_mask(RID p_area) const
{
	GodotArea2D* area = area_owner.get_or_null(p_area);
	ERR_FAIL_NULL_V(area, 0);

	return area->get_collision_mask();
}

/* BODY API */

RID GodotPhysicsServer2D::body_create()
{
	GodotBody2D* body = memnew(GodotBody2D);
	RID rid = body_owner.make_rid(body);
	body->set_self(rid);
	return rid;
}

void GodotPhysicsServer2D::body_set_space(RID p_body, RID p_space)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	GodotSpace2D* space = nullptr;
	if (p_space.is_valid()) {
		space = space_owner.get_or_null(p_space);
		ERR_FAIL_NULL(space);
	}

	if (body->get_space() == space) {
		return; // pointless
	}

	body->clear_constraint_list();
	body->set_space(space);
}

RID GodotPhysicsServer2D::body_get_space(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	GodotSpace2D* space = body->get_space();
	if (!space) {
		return RID();
	}
	return space->get_self();
}

void GodotPhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	FLUSH_QUERY_CHECK(body);

	body->set_mode(p_mode);
}

PS2DE::BodyMode GodotPhysicsServer2D::body_get_mode(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, PS2DE::BODY_MODE_STATIC);

	return body->get_mode();
}

void GodotPhysicsServer2D::body_add_shape(
	RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);

	body->add_shape(shape, p_transform, p_disabled);
}

void GodotPhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	GodotShape2D* shape = shape_owner.get_or_null(p_shape);
	ERR_FAIL_NULL(shape);
	ERR_FAIL_COND(!shape->is_configured());

	body->set_shape(p_shape_idx, shape);
}

void GodotPhysicsServer2D::body_set_shape_transform(
	RID p_body, int p_shape_idx, const Transform2D& p_transform)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_shape_transform(p_shape_idx, p_transform);
}

int GodotPhysicsServer2D::body_get_shape_count(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, -1);

	return body->get_shape_count();
}

RID GodotPhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, RID());

	GodotShape2D* shape = body->get_shape(p_shape_idx);
	ERR_FAIL_NULL_V(shape, RID());

	return shape->get_self();
}

Transform2D GodotPhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Transform2D());

	return body->get_shape_transform(p_shape_idx);
}

void GodotPhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_shape(p_shape_idx);
}

void GodotPhysicsServer2D::body_clear_shapes(RID p_body)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	while (body->get_shape_count()) {
		body->remove_shape(0);
	}
}

void GodotPhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	ERR_FAIL_INDEX(p_shape_idx, body->get_shape_count());
	FLUSH_QUERY_CHECK(body);

	body->set_shape_disabled(p_shape_idx, p_disabled);
}

void GodotPhysicsServer2D::body_set_shape_as_one_way_collision(
	RID p_body, int p_shape_idx, bool p_enable, real_t p_margin, const Vector2& p_direction)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	ERR_FAIL_INDEX(p_shape_idx, body->get_shape_count());
	FLUSH_QUERY_CHECK(body);

	body->set_shape_as_one_way_collision(p_shape_idx, p_enable, p_margin, p_direction);
}

void GodotPhysicsServer2D::body_set_continuous_collision_detection_mode(
	RID p_body, PS2DE::CCDMode p_mode)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_continuous_collision_detection_mode(p_mode);
}

PS2DE::CCDMode GodotPhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body) const
{
	const GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, PS2DE::CCD_MODE_DISABLED);

	return body->get_continuous_collision_detection_mode();
}

void GodotPhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_collision_layer(p_layer);
}

uint32_t GodotPhysicsServer2D::body_get_collision_layer(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_layer();
}

void GodotPhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_collision_mask(p_mask);
}

uint32_t GodotPhysicsServer2D::body_get_collision_mask(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_mask();
}

void GodotPhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_collision_priority(p_priority);
}

real_t GodotPhysicsServer2D::body_get_collision_priority(RID p_body) const
{
	const GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_collision_priority();
}

void GodotPhysicsServer2D::body_reset_mass_properties(RID p_body)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	return body->reset_mass_properties();
}

void GodotPhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_impulse(p_impulse);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	body->apply_torque_impulse(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_impulse(
	RID p_body, const Vector2& p_impulse, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	body->apply_impulse(p_impulse, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_central_force(p_force);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_force(
	RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_force(p_force, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->apply_torque(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_central_force(p_force);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_force(
	RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_force(p_force, p_position);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_constant_torque(p_torque);
	body->wakeup();
}

void GodotPhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_force(p_force);
	if (!p_force.is_zero_approx()) {
		body->wakeup();
	}
}

Vector2 GodotPhysicsServer2D::body_get_constant_force(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, Vector2());
	return body->get_constant_force();
}

void GodotPhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_constant_torque(p_torque);
	if (!Math::is_zero_approx(p_torque)) {
		body->wakeup();
	}
}

real_t GodotPhysicsServer2D::body_get_constant_torque(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);

	return body->get_constant_torque();
}

void GodotPhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	_update_shapes();

	Vector2 v = body->get_linear_velocity();
	Vector2 axis = p_axis_velocity.normalized();
	v -= axis * axis.dot(v);
	v += p_axis_velocity;
	body->set_linear_velocity(v);
	body->wakeup();
}

void GodotPhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->add_exception(p_body_b);
	body->wakeup();
}

void GodotPhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->remove_exception(p_body_b);
	body->wakeup();
}

void GodotPhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	for (int i = 0; i < body->get_exceptions().size(); i++) {
		p_exceptions->push_back(body->get_exceptions()[i]);
	}
}

void GodotPhysicsServer2D::body_set_contacts_reported_depth_threshold(
	RID p_body, real_t p_threshold)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
}

real_t GodotPhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, 0);
	return 0;
}

void GodotPhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);

	body->set_omit_force_integration(p_omit);
}

bool GodotPhysicsServer2D::body_is_omitting_force_integration(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	return body->get_omit_force_integration();
}

void GodotPhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_max_contacts_reported(p_contacts);
}

int GodotPhysicsServer2D::body_get_max_contacts_reported(RID p_body) const
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, -1);
	return body->get_max_contacts_reported();
}

bool GodotPhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape,
	const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max,
	int& r_result_count)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	ERR_FAIL_INDEX_V(p_body_shape, body->get_shape_count(), false);

	return shape_collide(body->get_shape(p_body_shape)->get_self(),
		body->get_transform() * body->get_shape_transform(p_body_shape), Vector2(), p_shape,
		p_shape_xform, p_motion, r_results, p_result_max, r_result_count);
}

void GodotPhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL(body);
	body->set_pickable(p_pickable);
}

bool GodotPhysicsServer2D::_body_test_motion(RID p_body,
	PhysicsTestMotionParameters2D* rp_parameters,
	const Ref<PhysicsTestMotionResult2D>& p_result)
{
	PS2DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

bool GodotPhysicsServer2D::body_test_motion(
	RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result)
{
	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, false);
	ERR_FAIL_NULL_V(body->get_space(), false);
	ERR_FAIL_COND_V(body->get_space()->is_locked(), false);

	_update_shapes();

	return body->get_space()->test_body_motion(body, p_parameters, r_result);
}

PhysicsDirectBodyState2D* GodotPhysicsServer2D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V_MSG((using_threads && !doing_sync), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	if (!body_owner.owns(p_body)) {
		return nullptr;
	}

	GodotBody2D* body = body_owner.get_or_null(p_body);
	ERR_FAIL_NULL_V(body, nullptr);

	if (!body->get_space()) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(body->get_space()->is_locked(), nullptr,
		"Body state is inaccessible right now, wait for iteration or physics process "
		"notification.");

	return body->get_direct_state();
}

/* JOINT API */

RID GodotPhysicsServer2D::joint_create()
{
	GodotJoint2D* joint = memnew(GodotJoint2D);
	RID joint_rid = joint_owner.make_rid(joint);
	joint->set_self(joint_rid);
	return joint_rid;
}

void GodotPhysicsServer2D::joint_clear(RID p_joint)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	if (joint->get_type() != PS2DE::JOINT_TYPE_MAX) {
		GodotJoint2D* empty_joint = memnew(GodotJoint2D);
		empty_joint->copy_settings_from(joint);

		joint_owner.replace(p_joint, empty_joint);
		memdelete(joint);
	}
}

void GodotPhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	switch (p_param) {
	case PS2DE::JOINT_PARAM_BIAS:
		joint->set_bias(p_value);
		break;
	case PS2DE::JOINT_PARAM_MAX_BIAS:
		joint->set_max_bias(p_value);
		break;
	case PS2DE::JOINT_PARAM_MAX_FORCE:
		joint->set_max_force(p_value);
		break;
	}
}

real_t GodotPhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param) const
{
	const GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, -1);

	switch (p_param) {
	case PS2DE::JOINT_PARAM_BIAS:
		return joint->get_bias();
		break;
	case PS2DE::JOINT_PARAM_MAX_BIAS:
		return joint->get_max_bias();
		break;
	case PS2DE::JOINT_PARAM_MAX_FORCE:
		return joint->get_max_force();
		break;
	}

	return 0;
}

void GodotPhysicsServer2D::joint_disable_collisions_between_bodies(
	RID p_joint, const bool p_disable)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);

	joint->disable_collisions_between_bodies(p_disable);

	if (2 == joint->get_body_count()) {
		GodotBody2D* body_a = *joint->get_body_ptr();
		GodotBody2D* body_b = *(joint->get_body_ptr() + 1);

		if (p_disable) {
			body_add_collision_exception(body_a->get_self(), body_b->get_self());
			body_add_collision_exception(body_b->get_self(), body_a->get_self());
		}
		else {
			body_remove_collision_exception(body_a->get_self(), body_b->get_self());
			body_remove_collision_exception(body_b->get_self(), body_a->get_self());
		}
	}
}

bool GodotPhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint) const
{
	const GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, true);

	return joint->is_disabled_collisions_between_bodies();
}

void GodotPhysicsServer2D::joint_make_pin(
	RID p_joint, const Vector2& p_pos, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);
	GodotBody2D* B = nullptr;
	if (body_owner.owns(p_body_b)) {
		B = body_owner.get_or_null(p_body_b);
		ERR_FAIL_NULL(B);
	}

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_pos, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1,
	const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);

	GodotBody2D* B = body_owner.get_or_null(p_body_b);
	ERR_FAIL_NULL(B);

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_a_groove1, p_a_groove2, p_b_anchor, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::joint_make_damped_spring(
	RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b)
{
	GodotBody2D* A = body_owner.get_or_null(p_body_a);
	ERR_FAIL_NULL(A);

	GodotBody2D* B = body_owner.get_or_null(p_body_b);
	ERR_FAIL_NULL(B);

	GodotJoint2D* prev_joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(prev_joint);

	GodotJoint2D* joint = memnew(GodotJoint2D(p_anchor_a, p_anchor_b, A, B));

	joint_owner.replace(p_joint, joint);
	joint->copy_settings_from(prev_joint);
	memdelete(prev_joint);
}

void GodotPhysicsServer2D::pin_joint_set_flag(
	RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_PIN);

	joint->set_flag(p_flag, p_enabled);
}

bool GodotPhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, false);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_PIN, false);

	return joint->get_flag(p_flag);
}

void GodotPhysicsServer2D::pin_joint_set_param(
	RID p_joint, PS2DE::PinJointParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_PIN);

	joint->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_PIN, 0);

	return joint->get_param(p_param);
}

void GodotPhysicsServer2D::damped_spring_joint_set_param(
	RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value)
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL(joint);
	ERR_FAIL_COND(joint->get_type() != PS2DE::JOINT_TYPE_DAMPED_SPRING);

	joint->set_param(p_param, p_value);
}

real_t GodotPhysicsServer2D::damped_spring_joint_get_param(
	RID p_joint, PS2DE::DampedSpringParam p_param) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, 0);
	ERR_FAIL_COND_V(joint->get_type() != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);

	return joint->get_param(p_param);
}

PS2DE::JointType GodotPhysicsServer2D::joint_get_type(RID p_joint) const
{
	GodotJoint2D* joint = joint_owner.get_or_null(p_joint);
	ERR_FAIL_NULL_V(joint, PS2DE::JOINT_TYPE_PIN);

	return joint->get_type();
}

void GodotPhysicsServer2D::free_rid(RID p_rid)
{
	_update_shapes(); // just in case

	if (shape_owner.owns(p_rid)) {
		GodotShape2D* shape = shape_owner.get_or_null(p_rid);

		while (shape->get_owners().size()) {
			GodotCollisionObject2D* so = shape->get_owners().begin()->key;
			so->remove_shape(shape);
		}

		shape_owner.free(p_rid);
		memdelete(shape);
	}
	else if (body_owner.owns(p_rid)) {
		GodotBody2D* body = body_owner.get_or_null(p_rid);

		body_set_space(p_rid, RID());

		while (body->get_shape_count()) {
			body->remove_shape(0);
		}

		body_owner.free(p_rid);
		memdelete(body);

	}
	else if (area_owner.owns(p_rid)) {
		GodotArea2D* area = area_owner.get_or_null(p_rid);

		area->set_space(nullptr);

		while (area->get_shape_count()) {
			area->remove_shape(0);
		}

		area_owner.free(p_rid);
		memdelete(area);
	}
	else if (space_owner.owns(p_rid)) {
		GodotSpace2D* space = space_owner.get_or_null(p_rid);

		while (space->get_objects().size()) {
			GodotCollisionObject2D* co =
				static_cast<GodotCollisionObject2D*>(*space->get_objects().begin());
			co->set_space(nullptr);
		}

		active_spaces.erase(space);
		free_rid(space->get_default_area()->get_self());
		space_owner.free(p_rid);
		memdelete(space);
	}
	else if (joint_owner.owns(p_rid)) {
		GodotJoint2D* joint = joint_owner.get_or_null(p_rid);

		joint_owner.free(p_rid);
		memdelete(joint);

	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

void GodotPhysicsServer2D::set_active(bool p_active) { active = p_active; }

void GodotPhysicsServer2D::init()
{
	doing_sync = false;
	if (!stepper) {
		stepper = memnew(GodotStep2D);
	}
}

void GodotPhysicsServer2D::step(real_t p_step)
{
	if (!active || !stepper) {
		return;
	}

	_update_shapes();

	island_count = 0;
	active_objects = 0;
	collision_pairs = 0;
	for (GodotSpace2D* E : active_spaces) {
		stepper->step(E, p_step);
		island_count += E->get_island_count();
		active_objects += E->get_active_objects();
		collision_pairs += E->get_collision_pairs();
	}
}

void GodotPhysicsServer2D::sync() { doing_sync = true; }

void GodotPhysicsServer2D::end_sync() { doing_sync = false; }

void GodotPhysicsServer2D::finish()
{
	if (stepper) {
		memdelete(stepper);
		stepper = nullptr;
	}
}

void GodotPhysicsServer2D::_update_shapes()
{
	while (pending_shape_update_list.first()) {
		pending_shape_update_list.first()->self()->_shape_changed();
		pending_shape_update_list.remove(pending_shape_update_list.first());
	}
}

int GodotPhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info)
{
	switch (p_info) {
	case PS2DE::INFO_ACTIVE_OBJECTS: {
		return active_objects;
	} break;
	case PS2DE::INFO_COLLISION_PAIRS: {
		return collision_pairs;
	} break;
	case PS2DE::INFO_ISLAND_COUNT: {
		return island_count;
	} break;
	}

	return 0;
}

GodotPhysicsServer2D::GodotPhysicsServer2D(bool p_using_threads)
{
	using_threads = p_using_threads;
}

void GodotPhysicsServer2D::flush_queries() {}



// ============================================================================
// Merged from modules/godot_physics_2d/register_types.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "godot_physics_server_2d.h"
#include "register_types.h"
#include "servers/physics_2d/physics_server_2d.h"

void uninitialize_godot_physics_2d_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
}

void initialize_godot_physics_2d_module(ModuleInitializationLevel p_level) {}



// ============================================================================
// Merged from modules/godot_physics_2d/godot_space_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#include "godot_physics_server_2d.h"

// ============================================================================
// GodotBroadPhase2D
// ============================================================================

GodotBroadPhase2D::ID GodotBroadPhase2D::create(GodotCollisionObject2D* p_object, int p_subindex, const Rect2& p_aabb, bool p_static)
{
	uint32_t tree_id = p_static ? TREE_STATIC : TREE_DYNAMIC;
	uint32_t tree_collision_mask = p_static ? TREE_FLAG_DYNAMIC : (TREE_FLAG_STATIC | TREE_FLAG_DYNAMIC);
	ID oid = bvh.create(p_object, true, tree_id, tree_collision_mask, p_aabb, p_subindex);
	return oid + 1;
}

void GodotBroadPhase2D::move(ID p_id, const Rect2& p_aabb)
{
	ERR_FAIL_COND(!p_id);
	bvh.move(p_id - 1, p_aabb);
}

void GodotBroadPhase2D::set_static(ID p_id, bool p_static)
{
	ERR_FAIL_COND(!p_id);
	uint32_t tree_id = p_static ? TREE_STATIC : TREE_DYNAMIC;
	uint32_t tree_collision_mask = p_static ? TREE_FLAG_DYNAMIC : (TREE_FLAG_STATIC | TREE_FLAG_DYNAMIC);
	bvh.set_tree(p_id - 1, tree_id, tree_collision_mask, false);
}

void GodotBroadPhase2D::remove(ID p_id)
{
	ERR_FAIL_COND(!p_id);
	bvh.erase(p_id - 1);
}

GodotCollisionObject2D* GodotBroadPhase2D::get_object(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, nullptr);
	GodotCollisionObject2D* it = bvh.get(p_id - 1);
	ERR_FAIL_NULL_V(it, nullptr);
	return it;
}

bool GodotBroadPhase2D::is_static(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, false);
	uint32_t tree_id = bvh.get_tree_id(p_id - 1);
	return tree_id == 0;
}

int GodotBroadPhase2D::get_subindex(ID p_id) const
{
	ERR_FAIL_COND_V(!p_id, 0);
	return bvh.get_subindex(p_id - 1);
}

int GodotBroadPhase2D::cull_segment(const Vector2& p_from, const Vector2& p_to, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices)
{
	return bvh.cull_segment(p_from, p_to, p_results, p_max_results, nullptr, 0xFFFFFFFF, p_result_indices);
}

int GodotBroadPhase2D::cull_aabb(const Rect2& p_aabb, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices)
{
	return bvh.cull_aabb(p_aabb, p_results, p_max_results, nullptr, 0xFFFFFFFF, p_result_indices);
}

void* GodotBroadPhase2D::_pair_callback(void* self, uint32_t p_A, GodotCollisionObject2D* p_object_A, int subindex_A, uint32_t p_B, GodotCollisionObject2D* p_object_B, int subindex_B)
{
	GodotBroadPhase2D* bpo = static_cast<GodotBroadPhase2D*>(self);
	if (!bpo->pair_callback) {
		return nullptr;
	}

	return bpo->pair_callback(p_object_A, subindex_A, p_object_B, subindex_B, bpo->pair_userdata);
}

void GodotBroadPhase2D::_unpair_callback(void* self, uint32_t p_A, GodotCollisionObject2D* p_object_A, int subindex_A, uint32_t p_B, GodotCollisionObject2D* p_object_B, int subindex_B, void* pairdata)
{
	GodotBroadPhase2D* bpo = static_cast<GodotBroadPhase2D*>(self);
	if (!bpo->unpair_callback) {
		return;
	}

	bpo->unpair_callback(p_object_A, subindex_A, p_object_B, subindex_B, pairdata, bpo->unpair_userdata);
}

void GodotBroadPhase2D::set_pair_callback(PairCallback p_pair_callback, void* p_userdata)
{
	pair_callback = p_pair_callback;
	pair_userdata = p_userdata;
}

void GodotBroadPhase2D::set_unpair_callback(UnpairCallback p_unpair_callback, void* p_userdata)
{
	unpair_callback = p_unpair_callback;
	unpair_userdata = p_userdata;
}

void GodotBroadPhase2D::update()
{
	bvh.update();
}

GodotBroadPhase2D::GodotBroadPhase2D()
{
	bvh.set_pair_callback(_pair_callback, this);
	bvh.set_unpair_callback(_unpair_callback, this);
}

// ============================================================================
// PhysicsDirectSpaceState2D / GodotSpace2D
// ============================================================================

#define TEST_MOTION_MARGIN_MIN_VALUE 0.0001
#define TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR 0.05

_FORCE_INLINE_ static bool _can_collide_with(GodotCollisionObject2D* p_object,
	uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas)
{
	if (!(p_object->get_collision_layer() & p_collision_mask)) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject2D::TYPE_AREA && !p_collide_with_areas) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject2D::TYPE_BODY && !p_collide_with_bodies) {
		return false;
	}

	return true;
}

PhysicsDirectSpaceState2D::PhysicsDirectSpaceState2D(GodotSpace2D* p_space) : space(p_space) {}

int PhysicsDirectSpaceState2D::intersect_point(
	const PS2DT::PointParameters& p_parameters, PS2DT::ShapeResult* r_results, int p_result_max)
{
	if (p_result_max <= 0) {
		return 0;
	}

	Rect2 aabb;
	aabb.position = p_parameters.position - Vector2(0.00001, 0.00001);
	aabb.size = Vector2(0.00002, 0.00002);

	int amount = space->broadphase->cull_aabb(aabb, space->intersection_query_results,
		GodotSpace2D::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	int cc = 0;

	for (int i = 0; i < amount; i++) {
		if (!_can_collide_with(space->intersection_query_results[i], p_parameters.collision_mask,
				p_parameters.collide_with_bodies, p_parameters.collide_with_areas)) {
			continue;
		}

		if (p_parameters.exclude.has(space->intersection_query_results[i]->get_self())) {
			continue;
		}

		const GodotCollisionObject2D* col_obj = space->intersection_query_results[i];

		if (p_parameters.pick_point && !col_obj->is_pickable()) {
			continue;
		}

		int shape_idx = space->intersection_query_subindex_results[i];

		GodotShape2D* shape = col_obj->get_shape(shape_idx);

		Vector2 local_point = (col_obj->get_transform() * col_obj->get_shape_transform(shape_idx))
								  .affine_inverse()
								  .xform(p_parameters.position);

		if (!shape->contains_point(local_point)) {
			continue;
		}

		if (cc >= p_result_max) {
			continue;
		}

		r_results[cc].rid = col_obj->get_self();
		r_results[cc].shape = shape_idx;

		cc++;
	}

	return cc;
}

bool PhysicsDirectSpaceState2D::intersect_ray(
	const PS2DT::RayParameters& p_parameters, PS2DT::RayResult& r_result)
{
	ERR_FAIL_COND_V(space->locked, false);

	Vector2 begin, end;
	Vector2 normal;
	begin = p_parameters.from;
	end = p_parameters.to;
	normal = (end - begin).normalized();

	int amount = space->broadphase->cull_segment(begin, end, space->intersection_query_results,
		GodotSpace2D::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	bool collided = false;
	Vector2 res_point, res_normal;
	int res_shape = -1;
	const GodotCollisionObject2D* res_obj = nullptr;
	real_t min_d = 1e10;

	for (int i = 0; i < amount; i++) {
		if (!_can_collide_with(space->intersection_query_results[i], p_parameters.collision_mask,
				p_parameters.collide_with_bodies, p_parameters.collide_with_areas)) {
			continue;
		}

		if (p_parameters.exclude.has(space->intersection_query_results[i]->get_self())) {
			continue;
		}

		const GodotCollisionObject2D* col_obj = space->intersection_query_results[i];

		int shape_idx = space->intersection_query_subindex_results[i];
		Transform2D inv_xform =
			col_obj->get_shape_inv_transform(shape_idx) * col_obj->get_inv_transform();

		Vector2 local_from = inv_xform.xform(begin);
		Vector2 local_to = inv_xform.xform(end);

		const GodotShape2D* shape = col_obj->get_shape(shape_idx);

		Vector2 shape_point, shape_normal;

		if (shape->contains_point(local_from)) {
			if (p_parameters.hit_from_inside) {
				min_d = 0;
				res_point = begin;
				res_normal = Vector2();
				res_shape = shape_idx;
				res_obj = col_obj;
				collided = true;
				break;
			}
			else {
				continue;
			}
		}

		if (shape->intersect_segment(local_from, local_to, shape_point, shape_normal)) {
			Transform2D xform = col_obj->get_transform() * col_obj->get_shape_transform(shape_idx);
			shape_point = xform.xform(shape_point);

			real_t ld = normal.dot(shape_point);

			if (ld < min_d) {
				min_d = ld;
				res_point = shape_point;
				res_normal = inv_xform.basis_xform_inv(shape_normal).normalized();
				res_shape = shape_idx;
				res_obj = col_obj;
				collided = true;
			}
		}
	}

	if (!collided) {
		return false;
	}
	ERR_FAIL_NULL_V(res_obj, false);

	r_result.normal = res_normal;
	r_result.position = res_point;
	r_result.rid = res_obj->get_self();
	r_result.shape = res_shape;

	return true;
}

struct _RestCallbackData2D
{
	const GodotCollisionObject2D* object = nullptr;
	const GodotCollisionObject2D* best_object = nullptr;
	int local_shape = 0;
	int best_local_shape = 0;
	int shape = 0;
	int best_shape = 0;
	Vector2 best_contact;
	Vector2 best_normal;
	real_t best_len = 0.0;
	Vector2 valid_dir;
	real_t valid_depth = 0.0;
	real_t min_allowed_depth = 0.0;
};

static void _rest_cbk_result(const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata)
{
	_RestCallbackData2D* rd = static_cast<_RestCallbackData2D*>(p_userdata);

	Vector2 contact_rel = p_point_B - p_point_A;
	real_t len = contact_rel.length();

	if (len < rd->min_allowed_depth) {
		return;
	}

	if (len <= rd->best_len) {
		return;
	}

	Vector2 normal = contact_rel / len;

	if (rd->valid_dir != Vector2()) {
		if (len > rd->valid_depth) {
			return;
		}

		if (rd->valid_dir.dot(normal) > -CMP_EPSILON) {
			return;
		}
	}

	rd->best_len = len;
	rd->best_contact = p_point_B;
	rd->best_normal = normal;
	rd->best_object = rd->object;
	rd->best_shape = rd->shape;
	rd->best_local_shape = rd->local_shape;
}

int GodotSpace2D::_cull_aabb_for_body(GodotBody2D* p_body, const Rect2& p_aabb)
{
	int amount = broadphase->cull_aabb(p_aabb, intersection_query_results, INTERSECTION_QUERY_MAX,
		intersection_query_subindex_results);

	for (int i = 0; i < amount; i++) {
		bool keep = true;

		if (intersection_query_results[i] == p_body) {
			keep = false;
		}
		else if (intersection_query_results[i]->get_type() == GodotCollisionObject2D::TYPE_AREA) {
			keep = false;
		}
		else if (!p_body->collides_with(
					   static_cast<GodotBody2D*>(intersection_query_results[i]))) {
			keep = false;
		}
		else if (static_cast<GodotBody2D*>(intersection_query_results[i])
					   ->has_exception(p_body->get_self()) ||
				   p_body->has_exception(intersection_query_results[i]->get_self())) {
			keep = false;
		}

		if (!keep) {
			if (i < amount - 1) {
				SWAP(intersection_query_results[i], intersection_query_results[amount - 1]);
				SWAP(intersection_query_subindex_results[i],
					intersection_query_subindex_results[amount - 1]);
			}

			amount--;
			i--;
		}
	}

	return amount;
}

void* GodotSpace2D::_broadphase_pair(GodotCollisionObject2D* A, int p_subindex_A,
	GodotCollisionObject2D* B, int p_subindex_B, void* p_self)
{
	GodotCollisionObject2D::Type type_A = A->get_type();
	GodotCollisionObject2D::Type type_B = B->get_type();
	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(p_subindex_A, p_subindex_B);
		SWAP(type_A, type_B);
	}

	GodotSpace2D* self = static_cast<GodotSpace2D*>(p_self);
	self->collision_pairs++;

	if (type_A == GodotCollisionObject2D::TYPE_AREA) {
		GodotArea2D* area = static_cast<GodotArea2D*>(A);
		if (type_B == GodotCollisionObject2D::TYPE_AREA) {
			GodotArea2D* area_b = static_cast<GodotArea2D*>(B);
			GodotArea2Pair2D* area2_pair =
				memnew(GodotArea2Pair2D(area_b, p_subindex_B, area, p_subindex_A));
			return area2_pair;
		}
		else {
			GodotBody2D* body = static_cast<GodotBody2D*>(B);
			GodotAreaPair2D* area_pair =
				memnew(GodotAreaPair2D(body, p_subindex_B, area, p_subindex_A));
			return area_pair;
		}

	}
	else {
		GodotBodyPair2D* b = memnew(GodotBodyPair2D(static_cast<GodotBody2D*>(A), p_subindex_A,
			static_cast<GodotBody2D*>(B), p_subindex_B));
		return b;
	}
}

void GodotSpace2D::_broadphase_unpair(GodotCollisionObject2D* A, int p_subindex_A,
	GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_self)
{
	if (!p_data) {
		return;
	}

	GodotSpace2D* self = static_cast<GodotSpace2D*>(p_self);
	self->collision_pairs--;
	GodotConstraint2D* c = static_cast<GodotConstraint2D*>(p_data);
	memdelete(c);
}

const SelfList<GodotBody2D>::List& GodotSpace2D::get_active_body_list() const
{
	return active_list;
}

void GodotSpace2D::body_add_to_active_list(SelfList<GodotBody2D>* p_body)
{
	active_list.add(p_body);
}

void GodotSpace2D::body_remove_from_active_list(SelfList<GodotBody2D>* p_body)
{
	active_list.remove(p_body);
}

void GodotSpace2D::body_add_to_mass_properties_update_list(SelfList<GodotBody2D>* p_body)
{
	mass_properties_update_list.add(p_body);
}

void GodotSpace2D::body_remove_from_mass_properties_update_list(SelfList<GodotBody2D>* p_body)
{
	mass_properties_update_list.remove(p_body);
}

GodotBroadPhase2D* GodotSpace2D::get_broadphase() { return broadphase; }

void GodotSpace2D::add_object(GodotCollisionObject2D* p_object)
{
	ERR_FAIL_COND(objects.has(p_object));
	objects.insert(p_object);
}

void GodotSpace2D::remove_object(GodotCollisionObject2D* p_object)
{
	ERR_FAIL_COND(!objects.has(p_object));
	objects.erase(p_object);
}

const HashSet<GodotCollisionObject2D*>& GodotSpace2D::get_objects() const { return objects; }

void GodotSpace2D::body_add_to_state_query_list(SelfList<GodotBody2D>* p_body)
{
	state_query_list.add(p_body);
}

void GodotSpace2D::body_remove_from_state_query_list(SelfList<GodotBody2D>* p_body)
{
	state_query_list.remove(p_body);
}

void GodotSpace2D::area_add_to_monitor_query_list(SelfList<GodotArea2D>* p_area)
{
	monitor_query_list.add(p_area);
}

void GodotSpace2D::area_remove_from_monitor_query_list(SelfList<GodotArea2D>* p_area)
{
	monitor_query_list.remove(p_area);
}

void GodotSpace2D::area_add_to_moved_list(SelfList<GodotArea2D>* p_area)
{
	area_moved_list.add(p_area);
}

void GodotSpace2D::area_remove_from_moved_list(SelfList<GodotArea2D>* p_area)
{
	area_moved_list.remove(p_area);
}

const SelfList<GodotArea2D>::List& GodotSpace2D::get_moved_area_list() const
{
	return area_moved_list;
}

void GodotSpace2D::call_queries()
{
	while (state_query_list.first()) {
		GodotBody2D* b = state_query_list.first()->self();
		state_query_list.remove(state_query_list.first());
		b->call_queries();
	}

	while (monitor_query_list.first()) {
		GodotArea2D* a = monitor_query_list.first()->self();
		monitor_query_list.remove(monitor_query_list.first());
		a->call_queries();
	}
}

void GodotSpace2D::setup()
{
	contact_debug_count = 0;

	while (mass_properties_update_list.first()) {
		mass_properties_update_list.first()->self()->update_mass_properties();
		mass_properties_update_list.remove(mass_properties_update_list.first());
	}
}

void GodotSpace2D::update() { broadphase->update(); }

void GodotSpace2D::set_param(PS2DE::SpaceParameter p_param, real_t p_value)
{
	switch (p_param) {
	case PS2DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		contact_recycle_radius = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		contact_max_separation = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		contact_max_allowed_penetration = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		contact_bias = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		body_linear_velocity_sleep_threshold = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		body_angular_velocity_sleep_threshold = p_value;
		break;
	case PS2DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		body_time_to_sleep = p_value;
		break;
	case PS2DE::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
		constraint_bias = p_value;
		break;
	case PS2DE::SPACE_PARAM_SOLVER_ITERATIONS:
		solver_iterations = p_value;
		break;
	}
}

real_t GodotSpace2D::get_param(PS2DE::SpaceParameter p_param) const
{
	switch (p_param) {
	case PS2DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		return contact_recycle_radius;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		return contact_max_separation;
	case PS2DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		return contact_max_allowed_penetration;
	case PS2DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		return contact_bias;
	case PS2DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		return body_linear_velocity_sleep_threshold;
	case PS2DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		return body_angular_velocity_sleep_threshold;
	case PS2DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		return body_time_to_sleep;
	case PS2DE::SPACE_PARAM_CONSTRAINT_DEFAULT_BIAS:
		return constraint_bias;
	case PS2DE::SPACE_PARAM_SOLVER_ITERATIONS:
		return solver_iterations;
	}
	return 0;
}

void GodotSpace2D::lock() { locked = true; }

void GodotSpace2D::unlock() { locked = false; }

bool GodotSpace2D::is_locked() const { return locked; }

PhysicsDirectSpaceState2D* GodotSpace2D::get_direct_state() { return direct_access; }

GodotSpace2D::GodotSpace2D()
{
	body_linear_velocity_sleep_threshold = 2.0;
	body_angular_velocity_sleep_threshold = Math::deg_to_rad(8.0);
	body_time_to_sleep = 0.5;
	solver_iterations = 16;
	contact_recycle_radius = 1.0;
	contact_max_separation = 1.5;
	contact_max_allowed_penetration = 0.3;
	contact_bias = 0.8;
	constraint_bias = 0.2;

	broadphase = memnew(GodotBroadPhase2D);
	broadphase->set_pair_callback(_broadphase_pair, this);
	broadphase->set_unpair_callback(_broadphase_unpair, this);

	direct_access = memnew(PhysicsDirectSpaceState2D(this));
}

GodotSpace2D::~GodotSpace2D()
{
	memdelete(broadphase);
	memdelete(direct_access);
}

bool GodotSpace2D::test_body_motion(GodotBody2D* p_body,
	const PS2DT::MotionParameters& p_parameters,
	PS2DT::MotionResult* r_result)
{
	return false;
}

int PhysicsDirectSpaceState2D::intersect_shape(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState2D::cast_motion(
	const PS2DT::ShapeParameters& p_parameters, real_t& p_closest_safe,
	real_t& p_closest_unsafe)
{
	return false;
}

bool PhysicsDirectSpaceState2D::collide_shape(
	const PS2DT::ShapeParameters& p_parameters, Vector2* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState2D::rest_info(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeRestInfo* r_info)
{
	return false;
}

// ============================================================================
// GodotStep2D
// ============================================================================

#define BODY_ISLAND_COUNT_RESERVE 128
#define BODY_ISLAND_SIZE_RESERVE 512
#define ISLAND_COUNT_RESERVE 128
#define ISLAND_SIZE_RESERVE 512
#define CONSTRAINT_COUNT_RESERVE 1024

void GodotStep2D::_populate_island(GodotBody2D* p_body, LocalVector<GodotBody2D*>& p_body_island,
	LocalVector<GodotConstraint2D*>& p_constraint_island)
{
	p_body->set_island_step(_step);

	if (p_body->get_mode() > PS2DE::BODY_MODE_KINEMATIC) {
		p_body_island.push_back(p_body);
	}

	for (const Pair<GodotConstraint2D*, int>& E : p_body->get_constraint_list()) {
		GodotConstraint2D* constraint = E.first;
		if (constraint->get_island_step() == _step) {
			continue;
		}
		constraint->set_island_step(_step);
		p_constraint_island.push_back(constraint);
		all_constraints.push_back(constraint);

		for (int i = 0; i < constraint->get_body_count(); i++) {
			if (i == E.second) {
				continue;
			}
			GodotBody2D* other_body = constraint->get_body_ptr()[i];
			if (other_body->get_island_step() == _step) {
				continue;
			}
			if (other_body->get_mode() == PS2DE::BODY_MODE_STATIC) {
				continue;
			}
			_populate_island(other_body, p_body_island, p_constraint_island);
		}
	}
}

void GodotStep2D::_setup_constraint(uint32_t p_constraint_index, void* p_userdata)
{
	GodotConstraint2D* constraint = all_constraints[p_constraint_index];
	constraint->setup(delta);
}

void GodotStep2D::_pre_solve_island(LocalVector<GodotConstraint2D*>& p_constraint_island) const
{
	uint32_t constraint_count = p_constraint_island.size();
	uint32_t valid_constraint_count = 0;
	for (uint32_t constraint_index = 0; constraint_index < constraint_count; ++constraint_index) {
		GodotConstraint2D* constraint = p_constraint_island[constraint_index];
		if (p_constraint_island[constraint_index]->pre_solve(delta)) {
			p_constraint_island[valid_constraint_count++] = constraint;
		}
	}
	p_constraint_island.resize(valid_constraint_count);
}

void GodotStep2D::_solve_island(uint32_t p_island_index, void* p_userdata) const
{
	const LocalVector<GodotConstraint2D*>& constraint_island = constraint_islands[p_island_index];

	for (int i = 0; i < iterations; i++) {
		uint32_t constraint_count = constraint_island.size();
		for (uint32_t constraint_index = 0; constraint_index < constraint_count;
			 ++constraint_index) {
			constraint_island[constraint_index]->solve(delta);
		}
	}
}

void GodotStep2D::_check_suspend(LocalVector<GodotBody2D*>& p_body_island) const
{
	bool can_sleep = true;

	uint32_t body_count = p_body_island.size();
	for (uint32_t body_index = 0; body_index < body_count; ++body_index) {
		GodotBody2D* body = p_body_island[body_index];

		if (!body->sleep_test(delta)) {
			can_sleep = false;
		}
	}

	// Put all to sleep or wake up everyone.
	for (uint32_t body_index = 0; body_index < body_count; ++body_index) {
		GodotBody2D* body = p_body_island[body_index];

		bool active = body->is_active();

		if (active == can_sleep) {
			body->set_active(!can_sleep);
		}
	}
}

void GodotStep2D::step(GodotSpace2D* p_space, real_t p_delta)
{
	p_space->lock();

	p_space->setup();

	p_space->set_last_step(p_delta);

	iterations = p_space->get_solver_iterations();
	delta = p_delta;

	const SelfList<GodotBody2D>::List* body_list = &p_space->get_active_body_list();

	/* INTEGRATE FORCES */

	uint64_t profile_begtime = OS::get_ticks_usec();
	uint64_t profile_endtime = 0;

	int active_count = 0;

	const SelfList<GodotBody2D>* b = body_list->first();
	while (b) {
		b->self()->integrate_forces(p_delta);
		b = b->next();
		active_count++;
	}

	p_space->set_active_objects(active_count);

	p_space->update();

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_INTEGRATE_FORCES, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* GENERATE CONSTRAINT ISLANDS FOR MOVING AREAS */

	uint32_t island_count = 0;

	const SelfList<GodotArea2D>::List& aml = p_space->get_moved_area_list();

	while (aml.first()) {
		for (GodotConstraint2D* E : aml.first()->self()->get_constraints()) {
			GodotConstraint2D* constraint = E;
			if (constraint->get_island_step() == _step) {
				continue;
			}
			constraint->set_island_step(_step);

			++island_count;
			if (constraint_islands.size() < island_count) {
				constraint_islands.resize(island_count);
			}
			LocalVector<GodotConstraint2D*>& constraint_island =
				constraint_islands[island_count - 1];
			constraint_island.clear();

			all_constraints.push_back(constraint);
			constraint_island.push_back(constraint);
		}
		p_space->area_remove_from_moved_list(
			(SelfList<GodotArea2D>*)aml.first());
	}

	/* GENERATE CONSTRAINT ISLANDS FOR ACTIVE RIGID BODIES */

	b = body_list->first();

	uint32_t body_island_count = 0;

	while (b) {
		GodotBody2D* body = b->self();

		if (body->get_island_step() != _step) {
			++body_island_count;
			if (body_islands.size() < body_island_count) {
				body_islands.resize(body_island_count);
			}
			LocalVector<GodotBody2D*>& body_island = body_islands[body_island_count - 1];
			body_island.clear();
			body_island.reserve(BODY_ISLAND_SIZE_RESERVE);

			++island_count;
			if (constraint_islands.size() < island_count) {
				constraint_islands.resize(island_count);
			}
			LocalVector<GodotConstraint2D*>& constraint_island =
				constraint_islands[island_count - 1];
			constraint_island.clear();
			constraint_island.reserve(ISLAND_SIZE_RESERVE);

			_populate_island(body, body_island, constraint_island);

			if (body_island.is_empty()) {
				--body_island_count;
			}

			if (constraint_island.is_empty()) {
				--island_count;
			}
		}
		b = b->next();
	}

	p_space->set_island_count((int)island_count);

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_GENERATE_ISLANDS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* SETUP CONSTRAINTS / PROCESS COLLISIONS */

	uint32_t total_constraint_count = all_constraints.size();
	(void)total_constraint_count;

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_SETUP_CONSTRAINTS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* PRE-SOLVE CONSTRAINT ISLANDS */

	for (uint32_t island_index = 0; island_index < island_count; ++island_index) {
		_pre_solve_island(constraint_islands[island_index]);
	}

	/* SOLVE CONSTRAINT ISLANDS */

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_SOLVE_CONSTRAINTS, profile_endtime - profile_begtime);
		profile_begtime = profile_endtime;
	}

	/* INTEGRATE VELOCITIES */

	b = body_list->first();
	while (b) {
		const SelfList<GodotBody2D>* n = b->next();
		b->self()->integrate_velocities(p_delta);
		b = n;
	}

	/* SLEEP / WAKE UP ISLANDS */

	for (uint32_t island_index = 0; island_index < body_island_count; ++island_index) {
		_check_suspend(body_islands[island_index]);
	}

	{
		profile_endtime = OS::get_ticks_usec();
		p_space->set_elapsed_time(
			GodotSpace2D::ELAPSED_TIME_INTEGRATE_VELOCITIES, profile_endtime - profile_begtime);
	}

	all_constraints.clear();

	p_space->unlock();
	_step++;
}

GodotStep2D::GodotStep2D()
{
	body_islands.reserve(BODY_ISLAND_COUNT_RESERVE);
	constraint_islands.reserve(ISLAND_COUNT_RESERVE);
	all_constraints.reserve(CONSTRAINT_COUNT_RESERVE);
}

GodotStep2D::~GodotStep2D() {}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_objects_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#include "godot_physics_server_2d.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

// --- GodotCollisionObject2D ---

void GodotCollisionObject2D::add_shape(GodotShape2D *p_shape, const Transform2D &p_transform, bool p_disabled) {
	Shape s;
	s.shape = p_shape;
	s.xform = p_transform;
	s.xform_inv = s.xform.affine_inverse();
	s.bpid = 0; //needs update
	s.disabled = p_disabled;
	s.one_way_collision = false;
	s.one_way_collision_margin = 0;
	s.one_way_collision_direction = Vector2(0, 1);
	shapes.push_back(s);
	p_shape->add_owner(this);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape(int p_index, GodotShape2D *p_shape) {
	ERR_FAIL_INDEX(p_index, shapes.size());
	shapes[p_index].shape->remove_owner(this);
	shapes.write[p_index].shape = p_shape;

	p_shape->add_owner(this);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape_transform(int p_index, const Transform2D &p_transform) {
	ERR_FAIL_INDEX(p_index, shapes.size());

	shapes.write[p_index].xform = p_transform;
	shapes.write[p_index].xform_inv = p_transform.affine_inverse();

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::set_shape_disabled(int p_idx, bool p_disabled) {
	ERR_FAIL_INDEX(p_idx, shapes.size());

	GodotCollisionObject2D::Shape &shape = shapes.write[p_idx];
	if (shape.disabled == p_disabled) {
		return;
	}

	shape.disabled = p_disabled;

	if (!space) {
		return;
	}

	if (p_disabled && shape.bpid != 0) {
		space->get_broadphase()->remove(shape.bpid);
		shape.bpid = 0;
		if (!pending_shape_update_list.in_list()) {
			if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
				server->pending_shape_update_list.add(&pending_shape_update_list);
			}
		}
	} else if (!p_disabled && shape.bpid == 0) {
		if (!pending_shape_update_list.in_list()) {
			if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
				server->pending_shape_update_list.add(&pending_shape_update_list);
			}
		}
	}
}

void GodotCollisionObject2D::remove_shape(GodotShape2D *p_shape) {
	//remove a shape, all the times it appears
	for (int i = 0; i < shapes.size(); i++) {
		if (shapes[i].shape == p_shape) {
			remove_shape(i);
			i--;
		}
	}
}

void GodotCollisionObject2D::remove_shape(int p_index) {
	//remove anything from shape to be erased to end, so subindices don't change
	ERR_FAIL_INDEX(p_index, shapes.size());
	for (int i = p_index; i < shapes.size(); i++) {
		if (shapes[i].bpid == 0) {
			continue;
		}
		//should never get here with a null owner
		space->get_broadphase()->remove(shapes[i].bpid);
		shapes.write[i].bpid = 0;
	}
	shapes[p_index].shape->remove_owner(this);
	shapes.remove_at(p_index);

	if (!pending_shape_update_list.in_list()) {
		if (GodotPhysicsServer2D *server = PhysicsServer2D::get_driver()) {
			server->pending_shape_update_list.add(&pending_shape_update_list);
		}
	}
}

void GodotCollisionObject2D::_set_static(bool p_static) {
	if (_static == p_static) {
		return;
	}
	_static = p_static;

	if (!space) {
		return;
	}
	for (int i = 0; i < get_shape_count(); i++) {
		const Shape &s = shapes[i];
		if (s.bpid > 0) {
			space->get_broadphase()->set_static(s.bpid, _static);
		}
	}
}

void GodotCollisionObject2D::_unregister_shapes() {
	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.bpid > 0) {
			space->get_broadphase()->remove(s.bpid);
			s.bpid = 0;
		}
	}
}

void GodotCollisionObject2D::_update_shapes() {
	if (!space) {
		return;
	}

	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.disabled) {
			continue;
		}

		//not quite correct, should compute the next matrix..
		Rect2 shape_aabb = s.shape->get_aabb();
		Transform2D xform = transform * s.xform;
		shape_aabb = xform.xform(shape_aabb);
		shape_aabb.grow_by((s.aabb_cache.size.x + s.aabb_cache.size.y) * 0.5 * 0.05);
		s.aabb_cache = shape_aabb;

		if (s.bpid == 0) {
			s.bpid = space->get_broadphase()->create(this, i, shape_aabb, _static);
			space->get_broadphase()->set_static(s.bpid, _static);
		}

		space->get_broadphase()->move(s.bpid, shape_aabb);
	}
}

void GodotCollisionObject2D::_update_shapes_with_motion(const Vector2 &p_motion) {
	if (!space) {
		return;
	}

	for (int i = 0; i < shapes.size(); i++) {
		Shape &s = shapes.write[i];
		if (s.disabled) {
			continue;
		}

		//not quite correct, should compute the next matrix..
		Rect2 shape_aabb = s.shape->get_aabb();
		Transform2D xform = transform * s.xform;
		shape_aabb = xform.xform(shape_aabb);
		shape_aabb = shape_aabb.merge(Rect2(shape_aabb.position + p_motion, shape_aabb.size)); //use motion
		s.aabb_cache = shape_aabb;

		if (s.bpid == 0) {
			s.bpid = space->get_broadphase()->create(this, i, shape_aabb, _static);
			space->get_broadphase()->set_static(s.bpid, _static);
		}

		space->get_broadphase()->move(s.bpid, shape_aabb);
	}
}

void GodotCollisionObject2D::_set_space(GodotSpace2D *p_space) {
	GodotSpace2D *old_space = space;
	space = p_space;

	if (old_space) {
		old_space->remove_object(this);

		for (int i = 0; i < shapes.size(); i++) {
			Shape &s = shapes.write[i];
			if (s.bpid) {
				old_space->get_broadphase()->remove(s.bpid);
				s.bpid = 0;
			}
		}
	}

	if (space) {
		space->add_object(this);
		_update_shapes();
	}
}

void GodotCollisionObject2D::_shape_changed() {
	_update_shapes();
	_shapes_changed();
}

GodotCollisionObject2D::GodotCollisionObject2D(Type p_type) :
		pending_shape_update_list(this) {
	type = p_type;
}

// --- GodotArea2D ---

GodotArea2D::BodyKey::BodyKey(GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

GodotArea2D::BodyKey::BodyKey(GodotArea2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	rid = p_body->get_self();
	body_shape = p_body_shape;
	area_shape = p_area_shape;
}

void GodotArea2D::_shapes_changed()
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}
}

void GodotArea2D::set_transform(const Transform2D& p_transform)
{
	if (!moved_list.in_list() && get_space()) {
		get_space()->area_add_to_moved_list(&moved_list);
	}

	_set_transform(p_transform);
	_set_inv_transform(p_transform.affine_inverse());
}

void GodotArea2D::set_space(GodotSpace2D* p_space)
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

void GodotArea2D::_set_space_override_mode(
	PS2DE::AreaSpaceOverrideMode& r_mode, PS2DE::AreaSpaceOverrideMode p_new_mode)
{
	bool do_override = p_new_mode != PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	if (do_override == (r_mode != PS2DE::AREA_SPACE_OVERRIDE_DISABLED)) {
		return;
	}
	_unregister_shapes();
	r_mode = p_new_mode;
	_shape_changed();
}

void GodotArea2D::_queue_monitor_update()
{
	ERR_FAIL_NULL(get_space());

	if (!monitor_query_list.in_list()) {
		get_space()->area_add_to_monitor_query_list(&monitor_query_list);
	}
}

void GodotArea2D::set_monitorable(bool p_monitorable)
{
	if (monitorable == p_monitorable) {
		return;
	}

	monitorable = p_monitorable;
	_set_static(!monitorable);
	_shapes_changed();
}

void GodotArea2D::compute_gravity(const Vector2& p_position, Vector2& r_gravity) const
{
	if (is_gravity_point()) {
		const real_t gr_unit_dist = get_gravity_point_unit_distance();
		Vector2 v = get_transform().xform(get_gravity_vector()) - p_position;
		if (gr_unit_dist > 0) {
			const real_t v_length_sq = v.length_squared();
			if (v_length_sq > 0) {
				const real_t gravity_strength =
					get_gravity() * gr_unit_dist * gr_unit_dist / v_length_sq;
				r_gravity = v.normalized() * gravity_strength;
			}
			else {
				r_gravity = Vector2();
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

GodotArea2D::GodotArea2D()
	: GodotCollisionObject2D(TYPE_AREA), monitor_query_list(this), moved_list(this)
{
	_set_static(true); // areas are not active by default
}

GodotArea2D::~GodotArea2D() {}

void GodotArea2D::call_queries() {}

// --- GodotBody2D ---

void GodotBody2D::_mass_properties_changed()
{
	if (get_space() && !mass_properties_update_list.in_list()) {
		get_space()->body_add_to_mass_properties_update_list(&mass_properties_update_list);
	}
}

void GodotBody2D::update_mass_properties()
{
	switch (mode) {
	case PS2DE::BODY_MODE_RIGID: {
		real_t total_area = 0;
		for (int i = 0; i < get_shape_count(); i++) {
			if (is_shape_disabled(i)) {
				continue;
			}
			total_area += get_shape_aabb(i).get_area();
		}

		if (calculate_center_of_mass) {
			center_of_mass_local = Vector2();

			if (total_area != 0.0) {
				for (int i = 0; i < get_shape_count(); i++) {
					if (is_shape_disabled(i)) {
						continue;
					}

					real_t area = get_shape_aabb(i).get_area();

					real_t mass_new = area * mass / total_area;

					// NOTE: we assume that the shape origin is also its center of mass.
					center_of_mass_local += mass_new * get_shape_transform(i).get_origin();
				}

				center_of_mass_local /= mass;
			}
		}

		if (calculate_inertia) {
			inertia = 0;

			for (int i = 0; i < get_shape_count(); i++) {
				if (is_shape_disabled(i)) {
					continue;
				}

				const GodotShape2D* shape = get_shape(i);

				real_t area = get_shape_aabb(i).get_area();
				if (area == 0.0) {
					continue;
				}

				real_t mass_new = area * mass / total_area;

				Transform2D mtx = get_shape_transform(i);
				Vector2 scale = mtx.get_scale();
				Vector2 shape_origin = mtx.get_origin() - center_of_mass_local;
				inertia += shape->get_moment_of_inertia(mass_new, scale) +
						   mass_new * shape_origin.length_squared();
			}
		}

		_inv_inertia = inertia > 0.0 ? (1.0 / inertia) : 0.0;

		if (mass) {
			_inv_mass = 1.0 / mass;
		}
		else {
			_inv_mass = 0;
		}

	} break;
	case PS2DE::BODY_MODE_KINEMATIC:
	case PS2DE::BODY_MODE_STATIC: {
		_inv_inertia = 0;
		_inv_mass = 0;
	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_inertia = 0;
		_inv_mass = 1.0 / mass;

	} break;
	}

	_update_transform_dependent();
}

void GodotBody2D::reset_mass_properties()
{
	calculate_inertia = true;
	calculate_center_of_mass = true;
	_mass_properties_changed();
}

void GodotBody2D::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;

	if (active) {
		if (mode == PS2DE::BODY_MODE_STATIC) {
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

void GodotBody2D::set_mode(PS2DE::BodyMode p_mode)
{
	PS2DE::BodyMode prev = mode;
	mode = p_mode;

	switch (p_mode) {
	case PS2DE::BODY_MODE_STATIC:
	case PS2DE::BODY_MODE_KINEMATIC: {
		_set_inv_transform(get_transform().affine_inverse());
		_inv_mass = 0;
		_inv_inertia = 0;
		_set_static(p_mode == PS2DE::BODY_MODE_STATIC);
		set_active(p_mode == PS2DE::BODY_MODE_KINEMATIC && contacts.size());
		linear_velocity = Vector2();
		angular_velocity = 0;
		if (mode == PS2DE::BODY_MODE_KINEMATIC && prev != mode) {
			first_time_kinematic = true;
		}
	} break;
	case PS2DE::BODY_MODE_RIGID: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		if (!calculate_inertia) {
			_inv_inertia = 1.0 / inertia;
		}
		_mass_properties_changed();
		_set_static(false);
		set_active(true);

	} break;
	case PS2DE::BODY_MODE_RIGID_LINEAR: {
		_inv_mass = mass > 0 ? (1.0 / mass) : 0;
		_inv_inertia = 0;
		angular_velocity = 0;
		_set_static(false);
		set_active(true);
	}
	}
}

PS2DE::BodyMode GodotBody2D::get_mode() const { return mode; }

void GodotBody2D::_shapes_changed()
{
	_mass_properties_changed();
	wakeup();
	wakeup_neighbours();
}

void GodotBody2D::set_space(GodotSpace2D* p_space)
{
	if (get_space()) {
		wakeup_neighbours();

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

void GodotBody2D::_update_transform_dependent()
{
	center_of_mass = get_transform().basis_xform(center_of_mass_local);
}

void GodotBody2D::wakeup_neighbours()
{
	for (const Pair<GodotConstraint2D*, int>& E : constraint_list) {
		const GodotConstraint2D* c = E.first;
		GodotBody2D** n = c->get_body_ptr();
		int bc = c->get_body_count();

		for (int i = 0; i < bc; i++) {
			if (i == E.second) {
				continue;
			}
			GodotBody2D* b = n[i];
			if (b->mode < PS2DE::BODY_MODE_RIGID) {
				continue;
			}

			if (!b->is_active()) {
				b->set_active(true);
			}
		}
	}
}

bool GodotBody2D::sleep_test(real_t p_step)
{
	if (mode == PS2DE::BODY_MODE_STATIC || mode == PS2DE::BODY_MODE_KINEMATIC) {
		return true;
	}
	else if (!can_sleep) {
		return false;
	}

	ERR_FAIL_NULL_V(get_space(), true);

	if (Math::abs(angular_velocity) < get_space()->get_body_angular_velocity_sleep_threshold() &&
		Math::abs(linear_velocity.length_squared()) <
			get_space()->get_body_linear_velocity_sleep_threshold() *
				get_space()->get_body_linear_velocity_sleep_threshold()) {
		still_time += p_step;

		return still_time > get_space()->get_body_time_to_sleep();
	}
	else {
		still_time = 0;
		return false;
	}
}

PhysicsDirectBodyState2D* GodotBody2D::get_direct_state()
{
	if (!direct_state) {
		direct_state = memnew(PhysicsDirectBodyState2D(this));
	}
	return direct_state;
}

GodotBody2D::GodotBody2D()
	: GodotCollisionObject2D(TYPE_BODY), active_list(this), mass_properties_update_list(this),
	  direct_state_query_list(this)
{
	_set_static(false);
}

GodotBody2D::~GodotBody2D() { memdelete(direct_state); }

void GodotBody2D::integrate_velocities(float) {}

void GodotBody2D::integrate_forces(float) {}

void GodotBody2D::call_queries() {}

// --- PhysicsDirectBodyState2D ---

PhysicsDirectBodyState2D::PhysicsDirectBodyState2D(GodotBody2D* p_body) : body(p_body) {}

Vector2 PhysicsDirectBodyState2D::get_total_gravity() const { return body->gravity; }

real_t PhysicsDirectBodyState2D::get_total_angular_damp() const
{
	return body->total_angular_damp;
}

real_t PhysicsDirectBodyState2D::get_total_linear_damp() const
{
	return body->total_linear_damp;
}

Vector2 PhysicsDirectBodyState2D::get_center_of_mass() const
{
	return body->get_center_of_mass();
}

Vector2 PhysicsDirectBodyState2D::get_center_of_mass_local() const
{
	return body->get_center_of_mass_local();
}

real_t PhysicsDirectBodyState2D::get_inverse_mass() const { return body->get_inv_mass(); }

real_t PhysicsDirectBodyState2D::get_inverse_inertia() const
{
	return body->get_inv_inertia();
}

void PhysicsDirectBodyState2D::set_linear_velocity(const Vector2& p_velocity)
{
	body->wakeup();
	body->set_linear_velocity(p_velocity);
}

Vector2 PhysicsDirectBodyState2D::get_linear_velocity() const
{
	return body->get_linear_velocity();
}

void PhysicsDirectBodyState2D::set_angular_velocity(real_t p_velocity)
{
	body->wakeup();
	body->set_angular_velocity(p_velocity);
}

real_t PhysicsDirectBodyState2D::get_angular_velocity() const
{
	return body->get_angular_velocity();
}

void PhysicsDirectBodyState2D::set_transform(const Transform2D& p_transform) {}

Transform2D PhysicsDirectBodyState2D::get_transform() const { return body->get_transform(); }

Vector2 PhysicsDirectBodyState2D::get_velocity_at_local_position(
	const Vector2& p_position) const
{
	return body->get_velocity_in_local_point(p_position);
}

void PhysicsDirectBodyState2D::apply_central_impulse(const Vector2& p_impulse)
{
	body->wakeup();
	body->apply_central_impulse(p_impulse);
}

void PhysicsDirectBodyState2D::apply_impulse(
	const Vector2& p_impulse, const Vector2& p_position)
{
	body->wakeup();
	body->apply_impulse(p_impulse, p_position);
}

void PhysicsDirectBodyState2D::apply_torque_impulse(real_t p_torque)
{
	body->wakeup();
	body->apply_torque_impulse(p_torque);
}

void PhysicsDirectBodyState2D::apply_central_force(const Vector2& p_force)
{
	body->wakeup();
	body->apply_central_force(p_force);
}

void PhysicsDirectBodyState2D::apply_force(const Vector2& p_force, const Vector2& p_position)
{
	body->wakeup();
	body->apply_force(p_force, p_position);
}

void PhysicsDirectBodyState2D::apply_torque(real_t p_torque)
{
	body->wakeup();
	body->apply_torque(p_torque);
}

void PhysicsDirectBodyState2D::add_constant_central_force(const Vector2& p_force)
{
	body->wakeup();
	body->add_constant_central_force(p_force);
}

void PhysicsDirectBodyState2D::add_constant_force(
	const Vector2& p_force, const Vector2& p_position)
{
	body->wakeup();
	body->add_constant_force(p_force, p_position);
}

void PhysicsDirectBodyState2D::add_constant_torque(real_t p_torque)
{
	body->wakeup();
	body->add_constant_torque(p_torque);
}

void PhysicsDirectBodyState2D::set_constant_force(const Vector2& p_force)
{
	if (!p_force.is_zero_approx()) {
		body->wakeup();
	}
	body->set_constant_force(p_force);
}

Vector2 PhysicsDirectBodyState2D::get_constant_force() const
{
	return body->get_constant_force();
}

void PhysicsDirectBodyState2D::set_constant_torque(real_t p_torque)
{
	if (!Math::is_zero_approx(p_torque)) {
		body->wakeup();
	}
	body->set_constant_torque(p_torque);
}

real_t PhysicsDirectBodyState2D::get_constant_torque() const
{
	return body->get_constant_torque();
}

void PhysicsDirectBodyState2D::set_sleep_state(bool p_enable) { body->set_active(!p_enable); }

bool PhysicsDirectBodyState2D::is_sleeping() const { return !body->is_active(); }

void PhysicsDirectBodyState2D::set_collision_layer(uint32_t p_layer)
{
	body->set_collision_layer(p_layer);
}

uint32_t PhysicsDirectBodyState2D::get_collision_layer() const
{
	return body->get_collision_layer();
}

void PhysicsDirectBodyState2D::set_collision_mask(uint32_t p_mask)
{
	body->set_collision_mask(p_mask);
}

uint32_t PhysicsDirectBodyState2D::get_collision_mask() const
{
	return body->get_collision_mask();
}

int PhysicsDirectBodyState2D::get_contact_count() const { return body->contact_count; }

Vector2 PhysicsDirectBodyState2D::get_contact_local_position(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_pos;
}

Vector2 PhysicsDirectBodyState2D::get_contact_local_normal(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_normal;
}

int PhysicsDirectBodyState2D::get_contact_local_shape(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, -1);
	return body->contacts[p_contact_idx].local_shape;
}

Vector2 PhysicsDirectBodyState2D::get_contact_local_velocity_at_position(
	int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].local_velocity_at_pos;
}

RID PhysicsDirectBodyState2D::get_contact_collider(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, RID());
	return body->contacts[p_contact_idx].collider;
}

Vector2 PhysicsDirectBodyState2D::get_contact_collider_position(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].collider_pos;
}

int PhysicsDirectBodyState2D::get_contact_collider_shape(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, 0);
	return body->contacts[p_contact_idx].collider_shape;
}

Vector2 PhysicsDirectBodyState2D::get_contact_collider_velocity_at_position(
	int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].collider_velocity_at_pos;
}

Vector2 PhysicsDirectBodyState2D::get_contact_impulse(int p_contact_idx) const
{
	ERR_FAIL_INDEX_V(p_contact_idx, body->contact_count, Vector2());
	return body->contacts[p_contact_idx].impulse;
}

PhysicsDirectSpaceState2D* PhysicsDirectBodyState2D::get_space_state()
{
	return body->get_space()->get_direct_state();
}

real_t PhysicsDirectBodyState2D::get_step() const
{
	return body->get_space()->get_last_step();
}

void PhysicsDirectBodyState2D::integrate_forces()
{
	real_t step = get_step();
	Vector2 lv = get_linear_velocity();
	lv += get_total_gravity() * step;

	real_t av = get_angular_velocity();

	real_t damp = 1.0 - step * get_total_linear_damp();

	if (damp < 0) { // reached zero in the given time
		damp = 0;
	}

	lv *= damp;

	damp = 1.0 - step * get_total_angular_damp();

	if (damp < 0) { // reached zero in the given time
		damp = 0;
	}

	av *= damp;

	set_linear_velocity(lv);
	set_angular_velocity(av);
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_shape_2d.cpp
// ============================================================================

#include "core/math/geometry_2d.h"
#include "core/templates/sort_array.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"

void GodotShape2D::configure(const Rect2& p_aabb)
{
	aabb = p_aabb;
	configured = true;
	for (const KeyValue<GodotCollisionObject2D*, int>& E : owners) {
		E.key->_shape_changed();
	}
}

Vector2 GodotShape2D::get_support(const Vector2& p_normal) const
{
	Vector2 res[2];
	int amnt;
	get_supports(p_normal, res, amnt);
	return res[0];
}

void GodotShape2D::add_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	if (E) {
		E->value++;
	}
	else {
		owners[p_owner] = 1;
	}
}

void GodotShape2D::remove_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	ERR_FAIL_COND(!E);
	E->value--;
	if (E->value == 0) {
		owners.remove(E);
	}
}

bool GodotShape2D::is_owner(GodotCollisionObject2D* p_owner) const { return owners.has(p_owner); }

const HashMap<GodotCollisionObject2D*, int>& GodotShape2D::get_owners() const { return owners; }

GodotShape2D::~GodotShape2D()
{
	ERR_FAIL_COND(owners.size());
	if (convex_points) {
		memdelete_arr(convex_points);
	}
}

void GodotShape2D::set_world_boundary(const Vector2& p_normal, real_t p_d)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_WORLD_BOUNDARY);
	wb_normal = p_normal;
	wb_d = p_d;
	configure(Rect2(Vector2(-1e15, -1e15), Vector2(1e15 * 2.0, 1e15 * 2.0)));
}

void GodotShape2D::set_separation_ray(real_t p_length, bool p_slide_on_slope)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEPARATION_RAY);
	ray_length = p_length;
	slide_on_slope = p_slide_on_slope;
	configure(Rect2(0, 0, 0.001, MAX(ray_length, (real_t)0.1)));
}

void GodotShape2D::set_segment(const Vector2& p_a, const Vector2& p_b)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEGMENT);
	seg_a = p_a;
	seg_b = p_b;
	seg_n = (seg_b - seg_a).orthogonal().normalized();

	Rect2 r(seg_a, Size2());
	r.expand_to(seg_b);
	if (r.size.x == 0) {
		r.size.x = 0.001;
	}
	if (r.size.y == 0) {
		r.size.y = 0.001;
	}
	configure(r);
}

void GodotShape2D::set_circle(real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CIRCLE);
	radius = p_radius;
	configure(Rect2(-radius, -radius, radius * 2.0, radius * 2.0));
}

void GodotShape2D::set_rectangle(const Vector2& p_half_extents)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_RECTANGLE);
	half_extents = p_half_extents;
	configure(Rect2(-half_extents, half_extents * 2.0));
}

void GodotShape2D::set_capsule(real_t p_height, real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CAPSULE);
	height = p_height;
	radius = p_radius;
	if (height < radius * 2.0) {
		height = radius * 2.0;
	}
	configure(Rect2(Point2(-radius, -height * 0.5), Size2(radius * 2.0, height)));
}

void GodotShape2D::set_convex_polygon(const Vector<Vector2>& p_points)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONVEX_POLYGON);
	if (convex_points) {
		memdelete_arr(convex_points);
		convex_points = nullptr;
	}
	convex_point_count = 0;

	int count = p_points.size();
	ERR_FAIL_COND(count < 3);

	Vector<Vector2> hull = Geometry2D::convex_hull(p_points);
	// convex_hull duplicates the first point at the end in counter-clockwise order.
	if (hull.size() > 1 && hull[0] == hull[hull.size() - 1]) {
		hull.resize(hull.size() - 1);
	}
	ERR_FAIL_COND(hull.size() < 3);

	convex_point_count = hull.size();
	convex_points = memnew_arr(ConvexPoint, convex_point_count);

	Rect2 r(hull[0], Size2());
	for (int i = 0; i < convex_point_count; i++) {
		convex_points[i].pos = hull[i];
		r.expand_to(hull[i]);
	}
	for (int i = 0; i < convex_point_count; i++) {
		Vector2 next = convex_points[(i + 1) % convex_point_count].pos;
		convex_points[i].normal = (next - convex_points[i].pos).orthogonal().normalized();
	}
	configure(r);
}

void GodotShape2D::set_concave_polygon(const Vector<Vector2>& p_segments)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);
	int len = p_segments.size();
	ERR_FAIL_COND(len % 2 != 0);

	concave_segments.clear();
	concave_points.clear();
	concave_bvh.clear();
	concave_bvh_depth = 0;

	if (len == 0) {
		configure(Rect2());
		return;
	}

	const Vector2* arr = p_segments.ptr();
	HashMap<Point2, int> pointmap;
	concave_segments.resize(len / 2);

	Rect2 global_aabb(arr[0], Size2());
	Vector<BVH> bvh_list;
	bvh_list.resize(len / 2);

	for (int i = 0; i < len / 2; i++) {
		Point2 a = arr[i * 2 + 0];
		Point2 b = arr[i * 2 + 1];

		int idx_a;
		HashMap<Point2, int>::Iterator E_a = pointmap.find(a);
		if (E_a) {
			idx_a = E_a->value;
		}
		else {
			idx_a = pointmap.size();
			pointmap[a] = idx_a;
		}

		int idx_b;
		HashMap<Point2, int>::Iterator E_b = pointmap.find(b);
		if (E_b) {
			idx_b = E_b->value;
		}
		else {
			idx_b = pointmap.size();
			pointmap[b] = idx_b;
		}

		concave_segments.write[i].points[0] = idx_a;
		concave_segments.write[i].points[1] = idx_b;

		Rect2 s_aabb(a, Size2());
		s_aabb.expand_to(b);
		global_aabb.expand_to(a);
		global_aabb.expand_to(b);

		bvh_list.write[i].aabb = s_aabb;
		bvh_list.write[i].left = -1;
		bvh_list.write[i].right = i;
	}

	concave_points.resize(pointmap.size());
	for (const KeyValue<Point2, int>& E : pointmap) {
		concave_points.write[E.value] = E.key;
	}

	concave_bvh_depth = 1;
	_generate_bvh(bvh_list.ptrw(), bvh_list.size(), 1);
	configure(global_aabb);
}

void GodotShape2D::project_range(
	const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_t<PS2DE::SHAPE_WORLD_BOUNDARY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_t<PS2DE::SHAPE_SEPARATION_RAY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_t<PS2DE::SHAPE_SEGMENT>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_t<PS2DE::SHAPE_CIRCLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_t<PS2DE::SHAPE_RECTANGLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_t<PS2DE::SHAPE_CAPSULE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_t<PS2DE::SHAPE_CONVEX_POLYGON>(p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range in GodotShape2D");
	}
}

void GodotShape2D::project_range_cast(const Vector2& p_cast, const Vector2& p_normal,
	const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_cast_t<PS2DE::SHAPE_WORLD_BOUNDARY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_cast_t<PS2DE::SHAPE_SEPARATION_RAY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_cast_t<PS2DE::SHAPE_SEGMENT>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_cast_t<PS2DE::SHAPE_CIRCLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_cast_t<PS2DE::SHAPE_RECTANGLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_cast_t<PS2DE::SHAPE_CAPSULE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_cast_t<PS2DE::SHAPE_CONVEX_POLYGON>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range_cast in GodotShape2D");
	}
}

void GodotShape2D::get_supports(const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		r_amount = 0;
	} break;
	case PS2DE::SHAPE_SEPARATION_RAY: {
		r_amount = 1;
		if (p_normal.y > 0) {
			*r_supports = Vector2(0, ray_length);
		}
		else {
			*r_supports = Vector2();
		}
	} break;
	case PS2DE::SHAPE_SEGMENT: {
		if (Math::abs(p_normal.dot(seg_n)) > segment_is_valid_support_threshold) {
			r_supports[0] = seg_a;
			r_supports[1] = seg_b;
			r_amount = 2;
			return;
		}

		real_t dp = p_normal.dot(seg_b - seg_a);
		if (dp > 0) {
			*r_supports = seg_b;
		}
		else {
			*r_supports = seg_a;
		}
		r_amount = 1;
	} break;
	case PS2DE::SHAPE_CIRCLE: {
		r_amount = 1;
		*r_supports = p_normal * radius;
	} break;
	case PS2DE::SHAPE_RECTANGLE: {
		for (int i = 0; i < 2; i++) {
			Vector2 ag;
			ag[i] = 1.0;
			real_t dp = ag.dot(p_normal);
			if (Math::abs(dp) <= segment_is_valid_support_threshold) {
				continue;
			}

			real_t sgn = dp > 0 ? 1.0 : -1.0;

			r_amount = 2;

			r_supports[0][i] = half_extents[i] * sgn;
			r_supports[0][i ^ 1] = half_extents[i ^ 1];

			r_supports[1][i] = half_extents[i] * sgn;
			r_supports[1][i ^ 1] = -half_extents[i ^ 1];

			return;
		}

		r_amount = 1;
		r_supports[0] = Vector2((p_normal.x < 0) ? -half_extents.x : half_extents.x,
			(p_normal.y < 0) ? -half_extents.y : half_extents.y);
	} break;
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 n = p_normal;

		real_t h = height * 0.5 - radius; // half-height of the rectangle part

		if (h > 0 && Math::abs(n.x) > segment_is_valid_support_threshold) {
			// make it flat
			n.y = 0.0;
			n.x = SIGN(n.x) * radius;

			r_amount = 2;
			r_supports[0] = n;
			r_supports[0].y += h;
			r_supports[1] = n;
			r_supports[1].y -= h;
		}
		else {
			n *= radius;
			n.y += (n.y > 0) ? h : -h;
			r_amount = 1;
			*r_supports = n;
		}
	} break;
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		int support_idx = -1;
		real_t d = -1e10;
		r_amount = 0;

		for (int i = 0; i < convex_point_count; i++) {
			// test point
			real_t ld = p_normal.dot(convex_points[i].pos);
			if (ld > d) {
				support_idx = i;
				d = ld;
			}

			// test segment
			if (convex_points[i].normal.dot(p_normal) > segment_is_valid_support_threshold) {
				r_amount = 2;
				r_supports[0] = convex_points[i].pos;
				r_supports[1] = convex_points[(i + 1) % convex_point_count].pos;
				return;
			}
		}

		ERR_FAIL_COND_MSG(support_idx == -1, "Convex polygon shape support not found.");

		r_amount = 1;
		r_supports[0] = convex_points[support_idx].pos;
	} break;
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		real_t d = -1e10;
		int idx = -1;
		for (int i = 0; i < concave_points.size(); i++) {
			real_t ld = p_normal.dot(concave_points[i]);
			if (ld > d) {
				d = ld;
				idx = i;
			}
		}

		r_amount = 1;
		ERR_FAIL_COND(idx == -1);
		*r_supports = concave_points[idx];
	} break;
	default: {
		r_amount = 0;
	} break;
	}
}

bool GodotShape2D::contains_point(const Vector2& p_point) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		return wb_normal.dot(p_point) < wb_d;
	}
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_SEGMENT:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return false;
	}
	case PS2DE::SHAPE_CIRCLE: {
		return p_point.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		real_t x = p_point.x;
		real_t y = p_point.y;
		real_t edge_x = half_extents.x;
		real_t edge_y = half_extents.y;
		return (x >= -edge_x) && (x < edge_x) && (y >= -edge_y) && (y < edge_y);
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 p = p_point;
		p.y = Math::abs(p.y);
		p.y -= height * 0.5 - radius;
		if (p.y < 0) {
			p.y = 0;
		}

		return p.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		bool out = false;
		bool in = false;

		for (int i = 0; i < convex_point_count; i++) {
			real_t d = convex_points[i].normal.dot(p_point) -
					   convex_points[i].normal.dot(convex_points[i].pos);
			if (d > 0) {
				out = true;
			}
			else {
				in = true;
			}
		}

		return in != out;
	}
	default:
		return false;
	}
}

bool GodotShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		Vector2 segment = p_begin - p_end;
		real_t den = wb_normal.dot(segment);

		if (Math::abs(den) <= CMP_EPSILON) {
			return false;
		}

		real_t dist = (wb_normal.dot(p_begin) - wb_d) / den;

		if (dist < -CMP_EPSILON || dist > (1.0 + CMP_EPSILON)) {
			return false;
		}

		r_point = p_begin + segment * -dist;
		r_normal = wb_normal;

		return true;
	}
	case PS2DE::SHAPE_SEPARATION_RAY: {
		return false; // rays can't be intersected
	}
	case PS2DE::SHAPE_SEGMENT: {
		if (!Geometry2D::segment_intersects_segment(p_begin, p_end, seg_a, seg_b, &r_point)) {
			return false;
		}

		if (seg_n.dot(p_begin) > seg_n.dot(seg_a)) {
			r_normal = seg_n;
		}
		else {
			r_normal = -seg_n;
		}

		return true;
	}
	case PS2DE::SHAPE_CIRCLE: {
		Vector2 line_vec = p_end - p_begin;

		real_t a = line_vec.dot(line_vec);
		real_t b = 2 * p_begin.dot(line_vec);
		real_t c = p_begin.dot(p_begin) - radius * radius;

		real_t sqrtterm = b * b - 4 * a * c;

		if (sqrtterm < 0) {
			return false;
		}
		sqrtterm = Math::sqrt(sqrtterm);
		real_t res = (-b - sqrtterm) / (2 * a);

		if (res < 0 || res > 1 + CMP_EPSILON) {
			return false;
		}

		r_point = p_begin + line_vec * res;
		r_normal = r_point.normalized();
		return true;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		return get_aabb().intersects_segment(p_begin, p_end, &r_point, &r_normal);
	}
	case PS2DE::SHAPE_CAPSULE: {
		real_t d = 1e10;
		Vector2 n = (p_end - p_begin).normalized();
		bool collided = false;

		// try spheres
		for (int i = 0; i < 2; i++) {
			Vector2 begin = p_begin;
			Vector2 end = p_end;
			real_t ofs = (i == 0) ? -height * 0.5 + radius : height * 0.5 - radius;
			begin.y += ofs;
			end.y += ofs;

			Vector2 line_vec = end - begin;

			real_t a = line_vec.dot(line_vec);
			real_t b = 2 * begin.dot(line_vec);
			real_t c = begin.dot(begin) - radius * radius;

			real_t sqrtterm = b * b - 4 * a * c;

			if (sqrtterm < 0) {
				continue;
			}

			sqrtterm = Math::sqrt(sqrtterm);
			real_t res = (-b - sqrtterm) / (2 * a);

			if (res < 0 || res > 1 + CMP_EPSILON) {
				continue;
			}

			Vector2 point = begin + line_vec * res;
			Vector2 pointf(point.x, point.y - ofs);
			real_t pd = n.dot(pointf);
			if (pd < d) {
				r_point = pointf;
				r_normal = point.normalized();
				d = pd;
				collided = true;
			}
		}

		Vector2 rpos, rnorm;
		if (Rect2(Point2(-radius, -height * 0.5 + radius), Size2(radius * 2.0, height - radius * 2))
				.intersects_segment(p_begin, p_end, &rpos, &rnorm)) {
			real_t pd = n.dot(rpos);
			if (pd < d) {
				r_point = rpos;
				r_normal = rnorm;
				d = pd;
				collided = true;
			}
		}

		return collided;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		for (int i = 0; i < convex_point_count; i++) {
			Vector2 res;

			if (!Geometry2D::segment_intersects_segment(p_begin, p_end, convex_points[i].pos,
					convex_points[(i + 1) % convex_point_count].pos, &res)) {
				continue;
			}

			real_t nd = n.dot(res);
			if (nd < d) {
				d = nd;
				r_point = res;
				r_normal = convex_points[i].normal;
				inters = true;
			}
		}

		return inters;
	}
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
			return false;
		}

		uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

		enum
		{
			TEST_AABB_BIT = 0,
			VISIT_LEFT_BIT = 1,
			VISIT_RIGHT_BIT = 2,
			VISIT_DONE_BIT = 3,
			VISITED_BIT_SHIFT = 29,
			NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
			VISITED_BIT_MASK = ~NODE_IDX_MASK,
		};

		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		int level = 0;

		const ConcaveSegment* segmentptr = &concave_segments[0];
		const Vector2* pointptr = &concave_points[0];
		const BVH* bvhptr = &concave_bvh[0];

		stack[0] = 0;
		while (true) {
			uint32_t node = stack[level] & NODE_IDX_MASK;
			const BVH& bvh2 = bvhptr[node];
			bool done = false;

			switch (stack[level] >> VISITED_BIT_SHIFT) {
			case TEST_AABB_BIT: {
				bool valid = bvh2.aabb.intersects_segment(p_begin, p_end);
				if (!valid) {
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					if (bvh2.left < 0) {
						const ConcaveSegment& s = segmentptr[bvh2.right];
						Vector2 a = pointptr[s.points[0]];
						Vector2 b = pointptr[s.points[1]];

						Vector2 res;

						if (Geometry2D::segment_intersects_segment(p_begin, p_end, a, b, &res)) {
							real_t nd = n.dot(res);
							if (nd < d) {
								d = nd;
								r_point = res;
								r_normal = (b - a).orthogonal().normalized();
								inters = true;
							}
						}

						stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
					}
					else {
						stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
					}
				}
			}
				continue;
			case VISIT_LEFT_BIT: {
				stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.left | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_RIGHT_BIT: {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.right | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_DONE_BIT: {
				if (level == 0) {
					done = true;
					break;
				}
				else {
					level--;
				}
			}
				continue;
			}

			if (done) {
				break;
			}
		}

		if (inters) {
			if (n.dot(r_normal) > 0) {
				r_normal = -r_normal;
			}
		}

		return inters;
	}
	default:
		return false;
	}
}

real_t GodotShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return 0;
	}
	case PS2DE::SHAPE_SEGMENT: {
		return p_mass * ((seg_a * p_scale).distance_squared_to(seg_b * p_scale)) / 12;
	}
	case PS2DE::SHAPE_CIRCLE: {
		real_t a = radius * p_scale.x;
		real_t b = radius * p_scale.y;
		return p_mass * (a * a + b * b) / 4;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		Vector2 he2 = half_extents * 2 * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 he2 = Vector2(radius * 2, height) * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		ERR_FAIL_COND_V_MSG(convex_point_count == 0, 0, "Convex polygon shape has no points.");
		Rect2 aabb_new;
		aabb_new.position = convex_points[0].pos * p_scale;
		for (int i = 0; i < convex_point_count; i++) {
			aabb_new.expand_to(convex_points[i].pos * p_scale);
		}

		return p_mass * aabb_new.size.dot(aabb_new.size) / 12.0;
	}
	default:
		return 0;
	}
}

int GodotShape2D::_generate_bvh(BVH* p_bvh, int p_len, int p_depth)
{
	if (p_len == 1) {
		concave_bvh_depth = MAX(p_depth, concave_bvh_depth);
		concave_bvh.push_back(*p_bvh);
		return concave_bvh.size() - 1;
	}

	Rect2 global_aabb = p_bvh[0].aabb;
	for (int i = 1; i < p_len; i++) {
		global_aabb = global_aabb.merge(p_bvh[i].aabb);
	}

	if (global_aabb.size.x > global_aabb.size.y) {
		SortArray<BVH, BVH_CompareX> sort;
		sort.sort(p_bvh, p_len);
	}
	else {
		SortArray<BVH, BVH_CompareY> sort;
		sort.sort(p_bvh, p_len);
	}

	int median = p_len / 2;

	BVH node;
	node.aabb = global_aabb;
	int node_idx = concave_bvh.size();
	concave_bvh.push_back(node);

	int l = _generate_bvh(p_bvh, median, p_depth + 1);
	int r = _generate_bvh(&p_bvh[median], p_len - median, p_depth + 1);
	concave_bvh.write[node_idx].left = l;
	concave_bvh.write[node_idx].right = r;

	return node_idx;
}

void GodotShape2D::cull(
	const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);

	if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
		return;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,
	};

	int level = 0;

	const ConcaveSegment* segmentptr = &concave_segments[0];
	const Vector2* pointptr = &concave_points[0];
	const BVH* bvhptr = &concave_bvh[0];

	stack[0] = 0;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& bvh2 = bvhptr[node];

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			bool valid = p_local_aabb.intersects(bvh2.aabb);
			if (!valid) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (bvh2.left < 0) {
					const ConcaveSegment& s = segmentptr[bvh2.right];
					Vector2 a = pointptr[s.points[0]];
					Vector2 b = pointptr[s.points[1]];

					GodotShape2D ss(a, b, (b - a).orthogonal().normalized());

					if (p_callback(p_userdata, &ss)) {
						return;
					}
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
		}
			continue;
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.left | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.right | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_DONE_BIT: {
			if (level == 0) {
				return;
			}
			else {
				level--;
			}
		}
			continue;
		}
	}
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_collision_solver_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "core/math/geometry_2d.h"
#include "core/templates/sort_array.h"

struct _CollectorCallback2D {
	GodotCollisionSolver2D::CallbackResult callback = nullptr;
	void *userdata = nullptr;
	bool swap = false;
	bool collided = false;
	Vector2 normal;
	Vector2 *sep_axis = nullptr;

	_FORCE_INLINE_ void call(const Vector2 &p_point_A, const Vector2 &p_point_B) {
		if (swap) {
			callback(p_point_B, p_point_A, userdata);
		} else {
			callback(p_point_A, p_point_B, userdata);
		}
	}
};

typedef void (*GenerateContactsFunc)(const Vector2 *, int, const Vector2 *, int, _CollectorCallback2D *);

_FORCE_INLINE_ static void _generate_contacts_point_point(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 1);
#endif

	p_collector->call(*p_points_A, *p_points_B);
}

_FORCE_INLINE_ static void _generate_contacts_point_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 2);
#endif

	Vector2 closest_B = Geometry2D::get_closest_point_to_segment_uncapped(*p_points_A, p_points_B[0], p_points_B[1]);
	p_collector->call(*p_points_A, closest_B);
}

struct _generate_contacts_Pair {
	bool a = false;
	int idx = 0;
	real_t d = 0.0;
	_FORCE_INLINE_ bool operator<(const _generate_contacts_Pair &l) const { return d < l.d; }
};

_FORCE_INLINE_ static void _generate_contacts_edge_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 2);
	ERR_FAIL_COND(p_point_count_B != 2); // circle is actually a 4x3 matrix
#endif

	Vector2 n = p_collector->normal;
	Vector2 t = n.orthogonal();
	real_t dA = n.dot(p_points_A[0]);
	real_t dB = n.dot(p_points_B[0]);

	_generate_contacts_Pair dvec[4];

	dvec[0].d = t.dot(p_points_A[0]);
	dvec[0].a = true;
	dvec[0].idx = 0;
	dvec[1].d = t.dot(p_points_A[1]);
	dvec[1].a = true;
	dvec[1].idx = 1;
	dvec[2].d = t.dot(p_points_B[0]);
	dvec[2].a = false;
	dvec[2].idx = 0;
	dvec[3].d = t.dot(p_points_B[1]);
	dvec[3].a = false;
	dvec[3].idx = 1;

	SortArray<_generate_contacts_Pair> sa;
	sa.sort(dvec, 4);

	for (int i = 1; i <= 2; i++) {
		if (dvec[i].a) {
			Vector2 a = p_points_A[dvec[i].idx];
			Vector2 b = n.plane_project(dB, a);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		} else {
			Vector2 b = p_points_B[dvec[i].idx];
			Vector2 a = n.plane_project(dA, b);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		}
	}
}

static void _generate_contacts_from_supports(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A < 1);
	ERR_FAIL_COND(p_point_count_B < 1);
#endif

	static const GenerateContactsFunc generate_contacts_func_table[2][2] = {
		{
				_generate_contacts_point_point,
				_generate_contacts_point_edge,
		},
		{
				nullptr,
				_generate_contacts_edge_edge,
		}
	};

	int pointcount_B = 0;
	int pointcount_A = 0;
	const Vector2 *points_A = nullptr;
	const Vector2 *points_B = nullptr;

	if (p_point_count_A > p_point_count_B) {
		//swap
		p_collector->swap = !p_collector->swap;
		p_collector->normal = -p_collector->normal;

		pointcount_B = p_point_count_A;
		pointcount_A = p_point_count_B;
		points_A = p_points_B;
		points_B = p_points_A;
	} else {
		pointcount_B = p_point_count_B;
		pointcount_A = p_point_count_A;
		points_A = p_points_A;
		points_B = p_points_B;
	}

	int version_A = (pointcount_A > 2 ? 2 : pointcount_A) - 1;
	int version_B = (pointcount_B > 2 ? 2 : pointcount_B) - 1;

	GenerateContactsFunc contacts_func = generate_contacts_func_table[version_A][version_B];
	ERR_FAIL_NULL(contacts_func);
	contacts_func(points_A, pointcount_A, points_B, pointcount_B, p_collector);
}

template <PS2DE::ShapeType typeA, PS2DE::ShapeType typeB, bool castA = false, bool castB = false, bool withMargin = false>
class SeparatorAxisTest2D {
	const GodotShape2D *shape_A = nullptr;
	const GodotShape2D *shape_B = nullptr;
	const Transform2D *transform_A = nullptr;
	const Transform2D *transform_B = nullptr;
	real_t best_depth = 1e15;
	Vector2 best_axis;
#ifdef DEBUG_ENABLED
	int best_axis_count = 0;
	int best_axis_index = -1;
#endif
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	_CollectorCallback2D *callback;

public:
	_FORCE_INLINE_ bool test_previous_axis() {
		if (callback && callback->sep_axis && *callback->sep_axis != Vector2()) {
			return test_axis(*callback->sep_axis);
		} else {
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif
		}
		return true;
	}

	_FORCE_INLINE_ bool test_cast() {
		if (castA) {
			Vector2 na = motion_A.normalized();
			if (!test_axis(na)) {
				return false;
			}
			if (!test_axis(na.orthogonal())) {
				return false;
			}
		}

		if (castB) {
			Vector2 nb = motion_B.normalized();
			if (!test_axis(nb)) {
				return false;
			}
			if (!test_axis(nb.orthogonal())) {
				return false;
			}
		}

		return true;
	}

	_FORCE_INLINE_ bool test_axis(const Vector2 &p_axis) {
		Vector2 axis = p_axis;

		if (Math::is_zero_approx(axis.x) &&
				Math::is_zero_approx(axis.y)) {
			// strange case, try an upwards separator
			axis = Vector2(0.0, 1.0);
		}

		real_t min_A = 0.0, max_A = 0.0, min_B = 0.0, max_B = 0.0;

		if constexpr (castA) {
			shape_A->project_range_cast_t<typeA>(motion_A, axis, *transform_A, min_A, max_A);
		} else {
			shape_A->project_range_t<typeA>(axis, *transform_A, min_A, max_A);
		}

		if constexpr (castB) {
			shape_B->project_range_cast_t<typeB>(motion_B, axis, *transform_B, min_B, max_B);
		} else {
			shape_B->project_range_t<typeB>(axis, *transform_B, min_B, max_B);
		}

		if (withMargin) {
			min_A -= margin_A;
			max_A += margin_A;
			min_B -= margin_B;
			max_B += margin_B;
		}

		min_B -= (max_A - min_A) * 0.5;
		max_B += (max_A - min_A) * 0.5;

		real_t dmin = min_B - (min_A + max_A) * 0.5;
		real_t dmax = max_B - (min_A + max_A) * 0.5;

		if (dmin > 0.0 || dmax < 0.0) {
			if (callback && callback->sep_axis) {
				*callback->sep_axis = axis;
			}
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif

			return false; // doesn't contain 0
		}

		//use the smallest depth

		dmin = Math::abs(dmin);

		if (dmax < dmin) {
			if (dmax < best_depth) {
				best_depth = dmax;
				best_axis = axis;
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		} else {
			if (dmin < best_depth) {
				best_depth = dmin;
				best_axis = -axis; // keep it as A axis
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		}

#ifdef DEBUG_ENABLED
		best_axis_count++;
#endif

		return true;
	}

	_FORCE_INLINE_ void generate_contacts() {
		// nothing to do, don't generate
		if (best_axis == Vector2(0.0, 0.0)) {
			return;
		}

		if (callback) {
			callback->collided = true;

			if (!callback->callback) {
				return; //only collide, no callback
			}
		}
		static const int max_supports = 2;

		Vector2 supports_A[max_supports];
		int support_count_A;
		if (castA) {
			shape_A->get_supports_transformed_cast(motion_A, -best_axis, *transform_A, supports_A, support_count_A);
		} else {
			shape_A->get_supports(transform_A->basis_xform_inv(-best_axis).normalized(), supports_A, support_count_A);
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] = transform_A->xform(supports_A[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] += -best_axis * margin_A;
			}
		}

		Vector2 supports_B[max_supports];
		int support_count_B;
		if (castB) {
			shape_B->get_supports_transformed_cast(motion_B, best_axis, *transform_B, supports_B, support_count_B);
		} else {
			shape_B->get_supports(transform_B->basis_xform_inv(best_axis).normalized(), supports_B, support_count_B);
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] = transform_B->xform(supports_B[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] += best_axis * margin_B;
			}
		}
		if (callback) {
			callback->normal = best_axis;
			_generate_contacts_from_supports(supports_A, support_count_A, supports_B, support_count_B, callback);

			if (callback->sep_axis && *callback->sep_axis != Vector2()) {
				*callback->sep_axis = Vector2(); //invalidate previous axis (no test)
			}
		}
	}

	_FORCE_INLINE_ SeparatorAxisTest2D(const GodotShape2D *p_shape_A, const Transform2D &p_transform_a, const GodotShape2D *p_shape_B, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_A = Vector2(), const Vector2 &p_motion_B = Vector2(), real_t p_margin_A = 0, real_t p_margin_B = 0) {
		margin_A = p_margin_A;
		margin_B = p_margin_B;
		shape_A = p_shape_A;
		shape_B = p_shape_B;
		transform_A = &p_transform_a;
		transform_B = &p_transform_b;
		motion_A = p_motion_A;
		motion_B = p_motion_B;
		callback = p_collector;
	}
};

/****** SAT TESTS *******/

#define TEST_POINT(m_a, m_b) \
	((!separator.test_axis(((m_a) - (m_b)).normalized())) || \
			(castA && !separator.test_axis(((m_a) + p_motion_a - (m_b)).normalized())) || \
			(castB && !separator.test_axis(((m_a) - ((m_b) + p_motion_b)).normalized())) || \
			(castA && castB && !separator.test_axis(((m_a) + p_motion_a - ((m_b) + p_motion_b)).normalized())))

typedef void (*CollisionFunc)(const GodotShape2D *, const Transform2D &, const GodotShape2D *, const Transform2D &, _CollectorCallback2D *p_collector, const Vector2 &, const Vector2 &, real_t, real_t);

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_segment(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *segment_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_SEGMENT, castA, castB, withMargin> separator(segment_A, p_transform_a, segment_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}
	//this collision is kind of pointless

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}
	if (!separator.test_axis(segment_B->get_xformed_normal(p_transform_b))) {
		return;
	}

	if (withMargin) {
		//points grow to circles

		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(segment_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//segment normal
	if (!separator.test_axis(
				(p_transform_a.xform(segment_A->get_b()) - p_transform_a.xform(segment_A->get_a())).normalized().orthogonal())) {
		return;
	}

	//endpoint a vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.get_origin())) {
		return;
	}
	//endpoint b vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(segment_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if (withMargin) {
		Transform2D inv = p_transform_b.affine_inverse();

		Vector2 a = p_transform_a.xform(segment_A->get_a());
		Vector2 b = p_transform_a.xform(segment_A->get_b());

		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a))) {
			return;
		}
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b))) {
			return;
		}

		if constexpr (castA) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b + p_motion_a))) {
				return;
			}
		}

		if constexpr (castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b + p_motion_a))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(segment_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(segment_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if (withMargin) {
			if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
			if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(circle_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(circle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	const Vector2 &sphere = p_transform_a.columns[2];
	const Vector2 *axis = &p_transform_b.columns[0];
	//const Vector2& half_extents = rectangle_B->get_half_extents();

	if (!separator.test_axis(axis[0].normalized())) {
		return;
	}

	if (!separator.test_axis(axis[1].normalized())) {
		return;
	}

	Transform2D binv = p_transform_b.affine_inverse();
	{
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphere))) {
			return;
		}
	}

	if constexpr (castA) {
		Vector2 sphereofs = sphere + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castB) {
		Vector2 sphereofs = sphere - p_motion_b;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castA && castB) {
		Vector2 sphereofs = sphere - p_motion_b + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(circle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	//capsule endpoints
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(circle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//poly faces and poly points vs circle
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.xform(convex_B->get_point(i)))) {
			return;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces A
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//box faces B
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if constexpr (withMargin) {
		Transform2D invA = p_transform_a.affine_inverse();
		Transform2D invB = p_transform_b.affine_inverse();

		if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, p_transform_b, invB))) {
			return;
		}

		if constexpr (castA || castB) {
			Transform2D aofs = p_transform_a;
			aofs.columns[2] += p_motion_a;

			Transform2D bofs = p_transform_b;
			bofs.columns[2] += p_motion_b;

			[[maybe_unused]] Transform2D aofsinv = aofs.affine_inverse();
			[[maybe_unused]] Transform2D bofsinv = bofs.affine_inverse();

			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, p_transform_b, invB))) {
					return;
				}
			}

			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}

			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	//box endpoints to capsule circles

	Transform2D boxinv = p_transform_a.affine_inverse();

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	for (int i = 0; i < 2; i++) {
		{
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		capsule_dir *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(rectangle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//convex faces
	Transform2D boxinv;
	if constexpr (withMargin) {
		boxinv = p_transform_a.affine_inverse();
	}
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if constexpr (withMargin) {
			//all points vs all points need to be tested if margin exist
			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i))))) {
				return;
			}
			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) - p_motion_a))) {
					return;
				}
			}
			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b))) {
					return;
				}
			}
			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b - p_motion_a))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(capsule_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//capsule endpoints

	real_t capsule_dir_A = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
	for (int i = 0; i < 2; i++) {
		Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir_A;

		real_t capsule_dir_B = capsule_B->get_height() * 0.5 - capsule_B->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_B = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir_B;

			if (TEST_POINT(capsule_endpoint_A, capsule_endpoint_B)) {
				return;
			}

			capsule_dir_B *= -1.0;
		}

		capsule_dir_A *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(capsule_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//poly vs capsule
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		Vector2 cpoint = p_transform_b.xform(convex_B->get_point(i));

		real_t capsule_dir = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir;

			if (TEST_POINT(capsule_endpoint_A, cpoint)) {
				return;
			}

			capsule_dir *= -1.0;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_convex_polygon_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *convex_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CONVEX_POLYGON, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(convex_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	for (int i = 0; i < convex_A->get_point_count(); i++) {
		if (!separator.test_axis(convex_A->get_xformed_segment_normal(p_transform_a, i))) {
			return;
		}
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	if (withMargin) {
		for (int i = 0; i < convex_A->get_point_count(); i++) {
			for (int j = 0; j < convex_B->get_point_count(); j++) {
				if (TEST_POINT(p_transform_a.xform(convex_A->get_point(i)), p_transform_b.xform(convex_B->get_point(j)))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

////////

bool sat_2d_calculate_penetration(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, GodotCollisionSolver2D::CallbackResult p_result_callback, void *p_userdata, bool p_swap, Vector2 *sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();

	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_A->is_concave(), false);

	PS2DE::ShapeType type_B = p_shape_B->get_type();

	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_B->is_concave(), false);

	static const CollisionFunc collision_table[5][5] = {
		{ _collision_segment_segment<false, false, false>,
				_collision_segment_circle<false, false, false>,
				_collision_segment_rectangle<false, false, false>,
				_collision_segment_capsule<false, false, false>,
				_collision_segment_convex_polygon<false, false, false> },
		{ nullptr,
				_collision_circle_circle<false, false, false>,
				_collision_circle_rectangle<false, false, false>,
				_collision_circle_capsule<false, false, false>,
				_collision_circle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, false>,
				_collision_rectangle_capsule<false, false, false>,
				_collision_rectangle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, false>,
				_collision_capsule_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, false> }

	};

	static const CollisionFunc collision_table_castA[5][5] = {
		{ _collision_segment_segment<true, false, false>,
				_collision_segment_circle<true, false, false>,
				_collision_segment_rectangle<true, false, false>,
				_collision_segment_capsule<true, false, false>,
				_collision_segment_convex_polygon<true, false, false> },
		{ nullptr,
				_collision_circle_circle<true, false, false>,
				_collision_circle_rectangle<true, false, false>,
				_collision_circle_capsule<true, false, false>,
				_collision_circle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, false>,
				_collision_rectangle_capsule<true, false, false>,
				_collision_rectangle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, false>,
				_collision_capsule_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, false> }

	};

	static const CollisionFunc collision_table_castB[5][5] = {
		{ _collision_segment_segment<false, true, false>,
				_collision_segment_circle<false, true, false>,
				_collision_segment_rectangle<false, true, false>,
				_collision_segment_capsule<false, true, false>,
				_collision_segment_convex_polygon<false, true, false> },
		{ nullptr,
				_collision_circle_circle<false, true, false>,
				_collision_circle_rectangle<false, true, false>,
				_collision_circle_capsule<false, true, false>,
				_collision_circle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, false>,
				_collision_rectangle_capsule<false, true, false>,
				_collision_rectangle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, false>,
				_collision_capsule_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, false> }

	};

	static const CollisionFunc collision_table_castA_castB[5][5] = {
		{ _collision_segment_segment<true, true, false>,
				_collision_segment_circle<true, true, false>,
				_collision_segment_rectangle<true, true, false>,
				_collision_segment_capsule<true, true, false>,
				_collision_segment_convex_polygon<true, true, false> },
		{ nullptr,
				_collision_circle_circle<true, true, false>,
				_collision_circle_rectangle<true, true, false>,
				_collision_circle_capsule<true, true, false>,
				_collision_circle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, false>,
				_collision_rectangle_capsule<true, true, false>,
				_collision_rectangle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, false>,
				_collision_capsule_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, false> }

	};

	static const CollisionFunc collision_table_margin[5][5] = {
		{ _collision_segment_segment<false, false, true>,
				_collision_segment_circle<false, false, true>,
				_collision_segment_rectangle<false, false, true>,
				_collision_segment_capsule<false, false, true>,
				_collision_segment_convex_polygon<false, false, true> },
		{ nullptr,
				_collision_circle_circle<false, false, true>,
				_collision_circle_rectangle<false, false, true>,
				_collision_circle_capsule<false, false, true>,
				_collision_circle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, true>,
				_collision_rectangle_capsule<false, false, true>,
				_collision_rectangle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, true>,
				_collision_capsule_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, true> }

	};

	static const CollisionFunc collision_table_castA_margin[5][5] = {
		{ _collision_segment_segment<true, false, true>,
				_collision_segment_circle<true, false, true>,
				_collision_segment_rectangle<true, false, true>,
				_collision_segment_capsule<true, false, true>,
				_collision_segment_convex_polygon<true, false, true> },
		{ nullptr,
				_collision_circle_circle<true, false, true>,
				_collision_circle_rectangle<true, false, true>,
				_collision_circle_capsule<true, false, true>,
				_collision_circle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, true>,
				_collision_rectangle_capsule<true, false, true>,
				_collision_rectangle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, true>,
				_collision_capsule_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, true> }

	};

	static const CollisionFunc collision_table_castB_margin[5][5] = {
		{ _collision_segment_segment<false, true, true>,
				_collision_segment_circle<false, true, true>,
				_collision_segment_rectangle<false, true, true>,
				_collision_segment_capsule<false, true, true>,
				_collision_segment_convex_polygon<false, true, true> },
		{ nullptr,
				_collision_circle_circle<false, true, true>,
				_collision_circle_rectangle<false, true, true>,
				_collision_circle_capsule<false, true, true>,
				_collision_circle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, true>,
				_collision_rectangle_capsule<false, true, true>,
				_collision_rectangle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, true>,
				_collision_capsule_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, true> }

	};

	static const CollisionFunc collision_table_castA_castB_margin[5][5] = {
		{ _collision_segment_segment<true, true, true>,
				_collision_segment_circle<true, true, true>,
				_collision_segment_rectangle<true, true, true>,
				_collision_segment_capsule<true, true, true>,
				_collision_segment_convex_polygon<true, true, true> },
		{ nullptr,
				_collision_circle_circle<true, true, true>,
				_collision_circle_rectangle<true, true, true>,
				_collision_circle_capsule<true, true, true>,
				_collision_circle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, true>,
				_collision_rectangle_capsule<true, true, true>,
				_collision_rectangle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, true>,
				_collision_capsule_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, true> }

	};

	_CollectorCallback2D callback;
	callback.callback = p_result_callback;
	callback.swap = p_swap;
	callback.userdata = p_userdata;
	callback.collided = false;
	callback.sep_axis = sep_axis;

	const GodotShape2D *A = p_shape_A;
	const GodotShape2D *B = p_shape_B;
	const Transform2D *transform_A = &p_transform_A;
	const Transform2D *transform_B = &p_transform_B;
	const Vector2 *motion_A = &p_motion_A;
	const Vector2 *motion_B = &p_motion_B;
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(transform_A, transform_B);
		SWAP(type_A, type_B);
		SWAP(motion_A, motion_B);
		SWAP(margin_A, margin_B);
		callback.swap = !callback.swap;
	}

	CollisionFunc collision_func;

	if (p_margin_A || p_margin_B) {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_margin[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA_margin[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB_margin[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB_margin[type_A - 2][type_B - 2];
		}
	} else {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB[type_A - 2][type_B - 2];
		}
	}

	ERR_FAIL_NULL_V(collision_func, false);

	collision_func(A, *transform_A, B, *transform_B, &callback, *motion_A, *motion_B, margin_A, margin_B);

	return callback.collided;
}

#define collision_solver sat_2d_calculate_penetration
//#define collision_solver gjk_epa_calculate_penetration

bool GodotCollisionSolver2D::solve_static_world_boundary(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_WORLD_BOUNDARY) {
		return false;
	}

	Vector2 n = p_transform_A.basis_xform(p_shape_A->get_world_boundary_normal()).normalized();
	Vector2 p = p_transform_A.xform(p_shape_A->get_world_boundary_normal() * p_shape_A->get_world_boundary_d());
	real_t d = n.dot(p);

	Vector2 supports[2];
	int support_count;

	p_shape_B->get_supports(p_transform_B.affine_inverse().basis_xform(-n).normalized(), supports, support_count);

	bool found = false;

	for (int i = 0; i < support_count; i++) {
		supports[i] += p_margin * supports[i].normalized();
		supports[i] = p_transform_B.xform(supports[i]);
		supports[i] += p_motion_B;
		real_t pd = n.dot(supports[i]);
		if (pd >= d) {
			continue;
		}
		found = true;

		Vector2 support_A = supports[i] - n * (pd - d);

		if (p_result_callback) {
			if (p_swap_result) {
				p_result_callback(supports[i], support_A, p_userdata);
			} else {
				p_result_callback(support_A, supports[i], p_userdata);
			}
		}
	}

	return found;
}

bool GodotCollisionSolver2D::solve_separation_ray(const GodotShape2D *p_shape_A, const Vector2 &p_motion_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_SEPARATION_RAY) {
		return false;
	}

	Vector2 from = p_transform_A.get_origin();
	Vector2 to = from + p_transform_A[1] * (p_shape_A->get_length() + p_margin);
	if (p_motion_A != Vector2()) {
		//not the best but should be enough
		Vector2 normal = (to - from).normalized();
		to += normal * MAX(0.0, normal.dot(p_motion_A));
	}
	Vector2 support_A = to;

	Transform2D invb = p_transform_B.affine_inverse();
	from = invb.xform(from);
	to = invb.xform(to);

	Vector2 p, n;
	if (!p_shape_B->intersect_segment(from, to, p, n)) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts when the ray is fully contained inside the shape.
	if (n == Vector2()) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts in the wrong direction.
	if (n.dot(from - to) < CMP_EPSILON) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	Vector2 support_B = p_transform_B.xform(p);
	if (p_shape_A->get_slide_on_slope()) {
		Vector2 global_n = invb.basis_xform_inv(n).normalized();
		support_B = support_A + (support_B - support_A).length() * global_n;
	}

	if (p_result_callback) {
		if (p_swap_result) {
			p_result_callback(support_B, support_A, p_userdata);
		} else {
			p_result_callback(support_A, support_B, p_userdata);
		}
	}
	return true;
}

struct _ConcaveCollisionInfo2D {
	const Transform2D *transform_A = nullptr;
	const GodotShape2D *shape_A = nullptr;
	const Transform2D *transform_B = nullptr;
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	GodotCollisionSolver2D::CallbackResult result_callback = nullptr;
	void *userdata = nullptr;
	bool swap_result = false;
	bool collided = false;
	int aabb_tests = 0;
	int collisions = 0;
	Vector2 *sep_axis = nullptr;
};

bool GodotCollisionSolver2D::concave_callback(void *p_userdata, GodotShape2D *p_convex) {
	_ConcaveCollisionInfo2D &cinfo = *(static_cast<_ConcaveCollisionInfo2D *>(p_userdata));
	cinfo.aabb_tests++;

	bool collided = collision_solver(cinfo.shape_A, *cinfo.transform_A, cinfo.motion_A, p_convex, *cinfo.transform_B, cinfo.motion_B, cinfo.result_callback, cinfo.userdata, cinfo.swap_result, cinfo.sep_axis, cinfo.margin_A, cinfo.margin_B);
	if (!collided) {
		return false;
	}

	cinfo.collided = true;
	cinfo.collisions++;

	// Stop at first collision if contacts are not needed.
	return !cinfo.result_callback;
}

bool GodotCollisionSolver2D::solve_concave(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	_ConcaveCollisionInfo2D cinfo;
	cinfo.transform_A = &p_transform_A;
	cinfo.shape_A = p_shape_A;
	cinfo.transform_B = &p_transform_B;
	cinfo.motion_A = p_motion_A;
	cinfo.result_callback = p_result_callback;
	cinfo.userdata = p_userdata;
	cinfo.swap_result = p_swap_result;
	cinfo.collided = false;
	cinfo.collisions = 0;
	cinfo.sep_axis = r_sep_axis;
	cinfo.margin_A = p_margin_A;
	cinfo.margin_B = p_margin_B;

	cinfo.aabb_tests = 0;

	Transform2D rel_transform = p_transform_A;
	rel_transform.columns[2] -= p_transform_B.get_origin();

	// Quickly compute a local Rect2.
	Rect2 local_aabb;
	for (int i = 0; i < 2; i++) {
		Vector2 axis(p_transform_B.columns[i]);
		real_t axis_scale = 1.0 / axis.length();
		axis *= axis_scale;

		real_t smin = 0.0, smax = 0.0;
		p_shape_A->project_rangev(axis, rel_transform, smin, smax);
		smin *= axis_scale;
		smax *= axis_scale;

		local_aabb.position[i] = smin;
		local_aabb.size[i] = smax - smin;
	}
	// In case of motion, expand the Rect2 in the motion direction.
	if (p_motion_A != Vector2()) {
		Rect2 moved_aabb = local_aabb;
		moved_aabb.position += p_motion_A;
		local_aabb = local_aabb.merge(moved_aabb);
	}

	p_shape_B->cull(local_aabb, concave_callback, &cinfo);

	return cinfo.collided;
}

bool GodotCollisionSolver2D::solve(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();
	PS2DE::ShapeType type_B = p_shape_B->get_type();
	bool concave_A = p_shape_A->is_concave();
	bool concave_B = p_shape_B->is_concave();
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	bool swap = false;

	if (type_A > type_B) {
		SWAP(type_A, type_B);
		SWAP(concave_A, concave_B);
		SWAP(margin_A, margin_B);
		swap = true;
	}

	if (type_A == PS2DE::SHAPE_WORLD_BOUNDARY) {
		if (type_B == PS2DE::SHAPE_WORLD_BOUNDARY) {
			WARN_PRINT_ONCE("Collisions between world boundaries are not supported.");
			return false;
		}

		if (swap) {
			return solve_static_world_boundary(p_shape_B, p_transform_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, p_margin_A);
		} else {
			return solve_static_world_boundary(p_shape_A, p_transform_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, p_margin_B);
		}

	} else if (type_A == PS2DE::SHAPE_SEPARATION_RAY) {
		if (type_B == PS2DE::SHAPE_SEPARATION_RAY) {
			WARN_PRINT_ONCE("Collisions between two rays are not supported.");
			return false; //no ray-ray
		}

		if (swap) {
			return solve_separation_ray(p_shape_B, p_motion_B, p_transform_B, p_shape_A, p_transform_A, p_result_callback, p_userdata, true, r_sep_axis, p_margin_B);
		} else {
			return solve_separation_ray(p_shape_A, p_motion_A, p_transform_A, p_shape_B, p_transform_B, p_result_callback, p_userdata, false, r_sep_axis, p_margin_A);
		}

	} else if (concave_B) {
		if (concave_A) {
			WARN_PRINT_ONCE("Collisions between two concave shapes are not supported.");
			return false;
		}

		if (!swap) {
			return solve_concave(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
		} else {
			return solve_concave(p_shape_B, p_transform_B, p_motion_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, r_sep_axis, margin_A, margin_B);
		}

	} else {
		return collision_solver(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
	}
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_constraints_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

// ============================================================================
// GodotAreaPair2D / GodotArea2Pair2D
// ============================================================================

void GodotAreaPair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotAreaPair2D::GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape) {
	body = p_body;
	area = p_area;
	body_shape = p_body_shape;
	area_shape = p_area_shape;
	body->add_constraint(this, 0);
	area->add_constraint(this);
	if (p_body->get_mode() == PS2DE::BODY_MODE_KINEMATIC) { // need to be active to process pair
		p_body->set_active(true);
	}
}

bool GodotAreaPair2D::setup(real_t p_step) {
	return false;
}

bool GodotAreaPair2D::pre_solve(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::setup(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::pre_solve(real_t p_step) {
	if (process_collision_a) {
		if (colliding_a) {
			area_a->add_area_to_query(area_b, shape_b, shape_a);
		} else {
			area_a->remove_area_from_query(area_b, shape_b, shape_a);
		}
	}

	if (process_collision_b) {
		if (colliding_b) {
			area_b->add_area_to_query(area_a, shape_a, shape_b);
		} else {
			area_b->remove_area_from_query(area_a, shape_a, shape_b);
		}
	}

	return false; // Never do any post solving.
}

void GodotArea2Pair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotArea2Pair2D::GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b) {
	area_a = p_area_a;
	area_b = p_area_b;
	shape_a = p_shape_a;
	shape_b = p_shape_b;
	area_a_monitorable = area_a->is_monitorable();
	area_b_monitorable = area_b->is_monitorable();
	area_a->add_constraint(this);
	area_b->add_constraint(this);
}

// ============================================================================
// GodotBodyPair2D
// ============================================================================

#define ACCUMULATE_IMPULSES

#define MIN_VELOCITY 0.001
#define MAX_BIAS_ROTATION (Math::PI / 8)

void GodotBodyPair2D::_add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self) {
	GodotBodyPair2D *self = static_cast<GodotBodyPair2D *>(p_self);

	self->_contact_added_callback(p_point_A, p_point_B);
}

void GodotBodyPair2D::_contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B) {
	Vector2 local_A = A->get_inv_transform().basis_xform(p_point_A);
	Vector2 local_B = B->get_inv_transform().basis_xform(p_point_B - offset_B);

	int new_index = contact_count;

	ERR_FAIL_COND(new_index >= (MAX_CONTACTS + 1));

	Contact contact;
	contact.local_A = local_A;
	contact.local_B = local_B;
	contact.normal = (p_point_A - p_point_B).normalized();
	contact.used = true;

	// Attempt to determine if the contact will be reused.
	real_t recycle_radius_2 = space->get_contact_recycle_radius() * space->get_contact_recycle_radius();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		if (c.local_A.distance_squared_to(local_A) < (recycle_radius_2) &&
				c.local_B.distance_squared_to(local_B) < (recycle_radius_2)) {
			contact.acc_normal_impulse = c.acc_normal_impulse;
			contact.acc_tangent_impulse = c.acc_tangent_impulse;
			contact.acc_bias_impulse = c.acc_bias_impulse;
			contact.acc_bias_impulse_center_of_mass = c.acc_bias_impulse_center_of_mass;
			c = contact;
			return;
		}
	}

	// Figure out if the contact amount must be reduced to fit the new contact.
	if (new_index == MAX_CONTACTS) {
		// Remove the contact with the minimum depth.
		const Transform2D &transform_A = A->get_transform();
		const Transform2D &transform_B = B->get_transform();

		int least_deep = -1;
		real_t min_depth;

		// Start with depth for new contact.
		{
			Vector2 global_A = transform_A.basis_xform(contact.local_A);
			Vector2 global_B = transform_B.basis_xform(contact.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			min_depth = axis.dot(contact.normal);
		}

		for (int i = 0; i < contact_count; i++) {
			const Contact &c = contacts[i];
			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < min_depth) {
				min_depth = depth;
				least_deep = i;
			}
		}

		if (least_deep > -1) {
			// Replace the least deep contact by the new one.
			contacts[least_deep] = contact;
		}

		return;
	}

	contacts[new_index] = contact;
	contact_count++;
}

void GodotBodyPair2D::_validate_contacts() {
	// Make sure to erase contacts that are no longer valid.
	real_t max_separation = space->get_contact_max_separation();
	real_t max_separation2 = max_separation * max_separation;

	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];

		bool erase = false;
		if (!c.used) {
			// Was left behind in previous frame.
			erase = true;
		} else {
			c.used = false;

			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;
			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < -max_separation || (global_B + c.normal * depth - global_A).length_squared() > max_separation2) {
				erase = true;
			}
		}

		if (erase) {
			// Contact no longer needed, remove.
			if ((i + 1) < contact_count) {
				// Swap with the last one.
				SWAP(contacts[i], contacts[contact_count - 1]);
			}

			i--;
			contact_count--;
		}
	}
}

bool GodotBodyPair2D::_test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B) {
	Vector2 motion = p_A->get_linear_velocity() * p_step;
	real_t mlen = motion.length();
	if (mlen < CMP_EPSILON) {
		return false;
	}

	Vector2 mnormal = motion / mlen;

	real_t min = 0.0, max = 0.0;
	p_A->get_shape(p_shape_A)->project_rangev(mnormal, p_xform_A, min, max);

	bool fast_object = mlen > (max - min) * 0.3;
	if (!fast_object) {
		return false;
	}

	Transform2D predicted_xform_B = p_xform_B.translated(p_B->get_linear_velocity() * p_step);

	int a;
	Vector2 s[2];
	p_A->get_shape(p_shape_A)->get_supports(p_xform_A.basis_xform_inv(mnormal).normalized(), s, a);
	Vector2 from = p_xform_A.xform(s[0]);
	Vector2 to = from + motion;

	Transform2D from_inv = predicted_xform_B.affine_inverse();

	Vector2 local_from = from_inv.xform(from - motion * 0.1);
	Vector2 local_to = from_inv.xform(to);

	Vector2 rpos, rnorm;
	if (!p_B->get_shape(p_shape_B)->intersect_segment(local_from, local_to, rpos, rnorm)) {
		return false;
	}

	// Check one-way collision based on motion direction.
	if (p_A->get_shape(p_shape_A)->allows_one_way_collision() && p_B->is_shape_set_as_one_way_collision(p_shape_B)) {
		Vector2 direction = predicted_xform_B.basis_xform(p_B->get_shape_one_way_collision_direction(p_shape_B)).normalized();

		if (direction.dot(mnormal) < CMP_EPSILON) {
			collided = false;
			oneway_disabled = true;
			return false;
		}
	}

	// Shorten the linear velocity so it does not hit, but gets close enough,
	// next frame will hit softly or soft enough.
	Vector2 hitpos = predicted_xform_B.xform(rpos);

	real_t newlen = hitpos.distance_to(from) + (max - min) * 0.01;
	p_A->set_linear_velocity(mnormal * (newlen / p_step));

	return true;
}

static real_t combine_bounce(GodotBody2D *A, GodotBody2D *B) {
	return CLAMP(A->get_bounce() + B->get_bounce(), 0, 1);
}

static real_t combine_friction(GodotBody2D *A, GodotBody2D *B) {
	return Math::abs(MIN(A->get_friction(), B->get_friction()));
}

bool GodotBodyPair2D::setup(real_t p_step) {
	check_ccd = false;

	if (!A->interacts_with(B) || A->has_exception(B->get_self()) || B->has_exception(A->get_self())) {
		collided = false;
		return false;
	}

	collide_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && A->collides_with(B);
	collide_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && B->collides_with(A);

	report_contacts_only = false;
	if (!collide_A && !collide_B) {
		if ((A->get_max_contacts_reported() > 0) || (B->get_max_contacts_reported() > 0)) {
			report_contacts_only = true;
		} else {
			collided = false;
			return false;
		}
	}

	// use local A coordinates to avoid numerical issues on collision detection
	offset_B = B->get_transform().get_origin() - A->get_transform().get_origin();

	_validate_contacts();

	const Vector2 &offset_A = A->get_transform().get_origin();
	Transform2D xform_Au = A->get_transform().untranslated();
	Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

	Transform2D xform_Bu = B->get_transform();
	xform_Bu.columns[2] -= offset_A;
	Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	Vector2 motion_A, motion_B;

	if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_A = A->get_motion();
	}
	if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_B = B->get_motion();
	}

	bool prev_collided = collided;

	collided = GodotCollisionSolver2D::solve(shape_A_ptr, xform_A, motion_A, shape_B_ptr, xform_B, motion_B, _add_contact, this, &sep_axis);
	if (!collided) {
		oneway_disabled = false;

		if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
			check_ccd = true;
			return true;
		}

		if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
			check_ccd = true;
			return true;
		}

		return false;
	}

	if (oneway_disabled) {
		return false;
	}

	if (!prev_collided) {
		if (shape_B_ptr->allows_one_way_collision() && A->is_shape_set_as_one_way_collision(shape_A)) {
			Vector2 direction = xform_A.basis_xform(A->get_shape_one_way_collision_direction(shape_A)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) > -CMP_EPSILON) { // Greater (normal inverted).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}

		if (shape_A_ptr->allows_one_way_collision() && B->is_shape_set_as_one_way_collision(shape_B)) {
			Vector2 direction = xform_B.basis_xform(B->get_shape_one_way_collision_direction(shape_B)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) < CMP_EPSILON) { // Less (normal ok).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}
	}

	return true;
}

bool GodotBodyPair2D::pre_solve(real_t p_step) {
	if (oneway_disabled) {
		return false;
	}

	if (!collided) {
		if (check_ccd) {
			const Vector2 &offset_A = A->get_transform().get_origin();
			Transform2D xform_Au = A->get_transform().untranslated();
			Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

			Transform2D xform_Bu = B->get_transform();
			xform_Bu.columns[2] -= offset_A;
			Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

			if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
				_test_ccd(p_step, A, shape_A, xform_A, B, shape_B, xform_B);
			}

			if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
				_test_ccd(p_step, B, shape_B, xform_B, A, shape_A, xform_A);
			}
		}

		return false;
	}

	real_t max_penetration = space->get_contact_max_allowed_penetration();
	real_t bias = space->get_contact_bias();

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	if (shape_A_ptr->get_custom_bias() || shape_B_ptr->get_custom_bias()) {
		if (shape_A_ptr->get_custom_bias() == 0) {
			bias = shape_B_ptr->get_custom_bias();
		} else if (shape_B_ptr->get_custom_bias() == 0) {
			bias = shape_A_ptr->get_custom_bias();
		} else {
			bias = (shape_B_ptr->get_custom_bias() + shape_A_ptr->get_custom_bias()) * 0.5;
		}
	}

	real_t inv_dt = 1.0 / p_step;

	bool do_process = false;

	const Vector2 &offset_A = A->get_transform().get_origin();
	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	real_t inv_inertia_A = collide_A ? A->get_inv_inertia() : 0.0;
	real_t inv_inertia_B = collide_B ? B->get_inv_inertia() : 0.0;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		c.active = false;

		Vector2 global_A = transform_A.basis_xform(c.local_A);
		Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

		Vector2 axis = global_A - global_B;
		real_t depth = axis.dot(c.normal);

		if (depth <= 0.0) {
			continue;
		}

#ifdef DEBUG_ENABLED
		if (space->is_debugging_contacts()) {
			space->add_debug_contact(global_A + offset_A);
			space->add_debug_contact(global_B + offset_A);
		}
#endif

		c.rA = global_A - A->get_center_of_mass();
		c.rB = global_B - B->get_center_of_mass() - offset_B;

		// Precompute normal mass, tangent mass, and bias.
		real_t rnA = c.rA.dot(c.normal);
		real_t rnB = c.rB.dot(c.normal);
		real_t kNormal = inv_mass_A + inv_mass_B;
		kNormal += inv_inertia_A * (c.rA.dot(c.rA) - rnA * rnA) + inv_inertia_B * (c.rB.dot(c.rB) - rnB * rnB);
		c.mass_normal = 1.0f / kNormal;

		Vector2 tangent = c.normal.orthogonal();
		real_t rtA = c.rA.dot(tangent);
		real_t rtB = c.rB.dot(tangent);
		real_t kTangent = inv_mass_A + inv_mass_B;
		kTangent += inv_inertia_A * (c.rA.dot(c.rA) - rtA * rtA) + inv_inertia_B * (c.rB.dot(c.rB) - rtB * rtB);
		c.mass_tangent = 1.0f / kTangent;

		c.bias = -bias * inv_dt * MIN(0.0f, -depth + max_penetration);
		c.depth = depth;

		Vector2 P = c.acc_normal_impulse * c.normal + c.acc_tangent_impulse * tangent;

		c.acc_impulse -= P;

		if (report_contacts_only) {
			collided = false;
			continue;
		}

#ifdef ACCUMULATE_IMPULSES
		{
			// Apply normal + friction impulse
			if (collide_A) {
				A->apply_impulse(-P, c.rA + A->get_center_of_mass());
			}
			if (collide_B) {
				B->apply_impulse(P, c.rB + B->get_center_of_mass());
			}
		}
#endif

		c.bounce = combine_bounce(A, B);
		if (c.bounce) {
			Vector2 crA(-A->get_prev_angular_velocity() * c.rA.y, A->get_prev_angular_velocity() * c.rA.x);
			Vector2 crB(-B->get_prev_angular_velocity() * c.rB.y, B->get_prev_angular_velocity() * c.rB.x);
			Vector2 dv = B->get_prev_linear_velocity() + crB - A->get_prev_linear_velocity() - crA;
			c.bounce = c.bounce * dv.dot(c.normal);
		}

		c.active = true;
		do_process = true;
	}

	return do_process;
}

void GodotBodyPair2D::solve(real_t p_step) {
	if (!collided || oneway_disabled) {
		return;
	}

	const real_t max_bias_av = MAX_BIAS_ROTATION / p_step;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; ++i) {
		Contact &c = contacts[i];

		if (!c.active) {
			continue;
		}

		// Relative velocity at contact
		Vector2 crA(-A->get_angular_velocity() * c.rA.y, A->get_angular_velocity() * c.rA.x);
		Vector2 crB(-B->get_angular_velocity() * c.rB.y, B->get_angular_velocity() * c.rB.x);
		Vector2 dv = B->get_linear_velocity() + crB - A->get_linear_velocity() - crA;

		Vector2 crbA(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		Vector2 crbB(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		Vector2 dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		real_t vn = dv.dot(c.normal);
		real_t vbn = dbv.dot(c.normal);

		Vector2 tangent = c.normal.orthogonal();
		real_t vt = dv.dot(tangent);

		real_t jbn = (c.bias - vbn) * c.mass_normal;
		real_t jbnOld = c.acc_bias_impulse;
		c.acc_bias_impulse = MAX(jbnOld + jbn, 0.0f);

		Vector2 jb = c.normal * (c.acc_bias_impulse - jbnOld);

		if (collide_A) {
			A->apply_bias_impulse(-jb, c.rA + A->get_center_of_mass(), max_bias_av);
		}
		if (collide_B) {
			B->apply_bias_impulse(jb, c.rB + B->get_center_of_mass(), max_bias_av);
		}

		crbA = Vector2(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		crbB = Vector2(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		vbn = dbv.dot(c.normal);

		if (Math::abs(-vbn + c.bias) > MIN_VELOCITY) {
			real_t jbn_com = (-vbn + c.bias) / (inv_mass_A + inv_mass_B);
			real_t jbnOld_com = c.acc_bias_impulse_center_of_mass;
			c.acc_bias_impulse_center_of_mass = MAX(jbnOld_com + jbn_com, 0.0f);

			Vector2 jb_com = c.normal * (c.acc_bias_impulse_center_of_mass - jbnOld_com);

			if (collide_A) {
				A->apply_bias_impulse(-jb_com, A->get_center_of_mass(), 0.0f);
			}
			if (collide_B) {
				B->apply_bias_impulse(jb_com, B->get_center_of_mass(), 0.0f);
			}
		}

		real_t jn = -(c.bounce + vn) * c.mass_normal;
		real_t jnOld = c.acc_normal_impulse;
		c.acc_normal_impulse = MAX(jnOld + jn, 0.0f);

		real_t friction = combine_friction(A, B);

		real_t jtMax = friction * c.acc_normal_impulse;
		real_t jt = -vt * c.mass_tangent;
		real_t jtOld = c.acc_tangent_impulse;
		c.acc_tangent_impulse = CLAMP(jtOld + jt, -jtMax, jtMax);

		Vector2 j = c.normal * (c.acc_normal_impulse - jnOld) + tangent * (c.acc_tangent_impulse - jtOld);

		if (collide_A) {
			A->apply_impulse(-j, c.rA + A->get_center_of_mass());
		}
		if (collide_B) {
			B->apply_impulse(j, c.rB + B->get_center_of_mass());
		}
		c.acc_impulse -= j;
	}
}

GodotBodyPair2D::GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B) :
		GodotConstraint2D(_arr, 2) {
	A = p_A;
	B = p_B;
	shape_A = p_shape_A;
	shape_B = p_shape_B;
	space = A->get_space();
	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

GodotBodyPair2D::~GodotBodyPair2D() {
	A->remove_constraint(this, 0);
	B->remove_constraint(this, 1);
}

// ============================================================================
// GodotJoint2D (based on Chipmunk joint constraints)
// ============================================================================

/* Copyright (c) 2007 Scott Lembcke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

void GodotJoint2D::copy_settings_from(GodotJoint2D *p_joint) {
	set_self(p_joint->get_self());
	set_max_force(p_joint->get_max_force());
	set_bias(p_joint->get_bias());
	set_max_bias(p_joint->get_max_bias());
	disable_collisions_between_bodies(p_joint->is_disabled_collisions_between_bodies());
}

static inline real_t k_scalar(GodotBody2D *a, GodotBody2D *b, const Vector2 &rA, const Vector2 &rB, const Vector2 &n) {
	real_t value = 0.0;

	{
		value += a->get_inv_mass();
		real_t rcn = (rA - a->get_center_of_mass()).cross(n);
		value += a->get_inv_inertia() * rcn * rcn;
	}

	if (b) {
		value += b->get_inv_mass();
		real_t rcn = (rB - b->get_center_of_mass()).cross(n);
		value += b->get_inv_inertia() * rcn * rcn;
	}

	return value;
}

static inline Vector2 relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB) {
	Vector2 sum = a->get_linear_velocity() - (rA - a->get_center_of_mass()).orthogonal() * a->get_angular_velocity();
	if (b) {
		return (b->get_linear_velocity() - (rB - b->get_center_of_mass()).orthogonal() * b->get_angular_velocity()) - sum;
	} else {
		return -sum;
	}
}

static inline real_t normal_relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB, Vector2 n) {
	return relative_velocity(a, b, rA, rB).dot(n);
}

static inline Vector2 custom_cross(const Vector2 &p_vec, real_t p_other) {
	return Vector2(p_other * p_vec.y, -p_other * p_vec.x);
}

static inline void k_tensor(GodotBody2D *a, GodotBody2D *b, Vector2 r1, Vector2 r2, Vector2 *k1, Vector2 *k2) {
	real_t k11, k12, k21, k22;
	real_t m_sum = a->get_inv_mass() + b->get_inv_mass();

	// start with I*m_sum
	k11 = m_sum;
	k12 = 0.0f;
	k21 = 0.0f;
	k22 = m_sum;

	r1 -= a->get_center_of_mass();
	r2 -= b->get_center_of_mass();

	// add the influence from r1
	real_t a_i_inv = a->get_inv_inertia();
	real_t r1xsq = r1.x * r1.x * a_i_inv;
	real_t r1ysq = r1.y * r1.y * a_i_inv;
	real_t r1nxy = -r1.x * r1.y * a_i_inv;
	k11 += r1ysq;
	k12 += r1nxy;
	k21 += r1nxy;
	k22 += r1xsq;

	// add the influence from r2
	real_t b_i_inv = b->get_inv_inertia();
	real_t r2xsq = r2.x * r2.x * b_i_inv;
	real_t r2ysq = r2.y * r2.y * b_i_inv;
	real_t r2nxy = -r2.x * r2.y * b_i_inv;
	k11 += r2ysq;
	k12 += r2nxy;
	k21 += r2nxy;
	k22 += r2xsq;

	// invert
	real_t determinant = k11 * k22 - k12 * k21;
	ERR_FAIL_COND(determinant == 0.0);

	real_t det_inv = 1.0f / determinant;
	*k1 = Vector2(k22 * det_inv, -k12 * det_inv);
	*k2 = Vector2(-k21 * det_inv, k11 * det_inv);
}

static _FORCE_INLINE_ Vector2 mult_k(const Vector2 &vr, const Vector2 &k1, const Vector2 &k2) {
	return Vector2(vr.dot(k1), vr.dot(k2));
}

bool GodotJoint2D::setup(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _setup_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _setup_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _setup_damped_spring(p_step);
		default:
			return false;
	}
}

bool GodotJoint2D::pre_solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _pre_solve_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _pre_solve_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _pre_solve_damped_spring(p_step);
		default:
			return false;
	}
}

void GodotJoint2D::solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			_solve_pin(p_step);
			break;
		case PS2DE::JOINT_TYPE_GROOVE:
			_solve_groove(p_step);
			break;
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			_solve_damped_spring(p_step);
			break;
		default:
			break;
	}
}

// --- Pin Joint ---

bool GodotJoint2D::_setup_pin(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B && B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B ? B->get_transform().basis_xform(anchor_B) : anchor_B;

	real_t B_inv_mass = B ? B->get_inv_mass() : 0.0;

	Transform2D K1;
	K1[0].x = A->get_inv_mass() + B_inv_mass;
	K1[1].x = 0.0f;
	K1[0].y = 0.0f;
	K1[1].y = A->get_inv_mass() + B_inv_mass;

	Vector2 r1 = rA - A->get_center_of_mass();

	Transform2D K2;
	K2[0].x = A->get_inv_inertia() * r1.y * r1.y;
	K2[1].x = -A->get_inv_inertia() * r1.x * r1.y;
	K2[0].y = -A->get_inv_inertia() * r1.x * r1.y;
	K2[1].y = A->get_inv_inertia() * r1.x * r1.x;

	Transform2D K;
	K[0] = K1[0] + K2[0];
	K[1] = K1[1] + K2[1];

	if (B) {
		Vector2 r2 = rB - B->get_center_of_mass();

		Transform2D K3;
		K3[0].x = B->get_inv_inertia() * r2.y * r2.y;
		K3[1].x = -B->get_inv_inertia() * r2.x * r2.y;
		K3[0].y = -B->get_inv_inertia() * r2.x * r2.y;
		K3[1].y = B->get_inv_inertia() * r2.x * r2.x;

		K[0] += K3[0];
		K[1] += K3[1];
	}

	K[0].x += softness;
	K[1].y += softness;

	M = K.affine_inverse();

	Vector2 gA = rA + A->get_transform().get_origin();
	Vector2 gB = B ? rB + B->get_transform().get_origin() : rB;

	Vector2 delta = gB - gA;

	pin_bias = delta * -(get_bias() == 0 ? space->get_constraint_bias() : get_bias()) * (1.0 / p_step);

	// Compute max impulse.
	jn_max = get_max_force() * p_step;

	return true;
}

bool GodotJoint2D::_pre_solve_pin(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-P, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(P, rB);
	}
	real_t i_sum_local = A->get_inv_inertia();
	if (B) {
		i_sum_local += B->get_inv_inertia();
	}
	i_sum = 1.0 / (i_sum_local);
	if (angular_limit_enabled && B) {
		Vector2 diff_vector = B->get_transform().get_origin() - A->get_transform().get_origin();
		diff_vector = diff_vector.rotated(-initial_angle);
		real_t dist = diff_vector.angle();
		real_t pdist = 0.0;
		if (dist > angular_limit_upper) {
			pdist = dist - angular_limit_upper;
		} else if (dist < angular_limit_lower) {
			pdist = dist - angular_limit_lower;
		}
		real_t error_bias = Math::pow(1.0 - 0.15, 60.0);
		// Calculate bias velocity.
		bias_velocity = -CLAMP((-1.0 - Math::pow(error_bias, p_step)) * pdist / p_step, -get_max_bias(), get_max_bias());
		// If the bias velocity is 0, the joint is not at a limit.
		if (bias_velocity >= -CMP_EPSILON && bias_velocity <= CMP_EPSILON) {
			j_acc = 0;
			is_joint_at_limit = false;
		} else {
			is_joint_at_limit = true;
		}
	} else {
		bias_velocity = 0.0;
	}

	return true;
}

void GodotJoint2D::_solve_pin(real_t p_step) {
	// Compute relative velocity.
	Vector2 vA = A->get_linear_velocity() - custom_cross(rA - A->get_center_of_mass(), A->get_angular_velocity());

	Vector2 rel_vel;
	if (B) {
		rel_vel = B->get_linear_velocity() - custom_cross(rB - B->get_center_of_mass(), B->get_angular_velocity()) - vA;
	} else {
		rel_vel = -vA;
	}
	if ((angular_limit_enabled || motor_enabled) && B) {
		// Compute relative rotational velocity.
		real_t wr = B->get_angular_velocity() - A->get_angular_velocity();
		if (motor_enabled) {
			wr -= motor_target_velocity;
		}
		real_t j_max = jn_max;

		// Compute normal impulse.
		real_t j_pin = -(bias_velocity + wr) * i_sum;
		real_t j_old = j_acc;
		// Only enable the limits if we have to.
		if (angular_limit_enabled && is_joint_at_limit) {
			if (bias_velocity < 0.0) {
				j_acc = CLAMP(j_old + j_pin, 0.0, j_max);
			} else {
				j_acc = CLAMP(j_old + j_pin, -j_max, 0.0);
			}
		} else {
			j_acc = CLAMP(j_old + j_pin, -j_max, j_max);
		}
		j_pin = j_acc - j_old;
		A->apply_torque_impulse(-j_pin * A->get_inv_inertia());
		B->apply_torque_impulse(j_pin * B->get_inv_inertia());
	}

	Vector2 impulse = M.basis_xform(pin_bias - rel_vel - Vector2(softness, softness) * P);

	if (dynamic_A) {
		A->apply_impulse(-impulse, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(impulse, rB);
	}

	P += impulse;
}

void GodotJoint2D::set_param(PS2DE::PinJointParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			softness = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			angular_limit_upper = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			angular_limit_lower = p_value;
		} break;
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			motor_target_velocity = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::PinJointParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, 0);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			return softness;
		}
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			return angular_limit_upper;
		}
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			return angular_limit_lower;
		}
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			return motor_target_velocity;
		}
	}
	ERR_FAIL_V(0);
}

void GodotJoint2D::set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			angular_limit_enabled = p_enabled;
		} break;
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			motor_enabled = p_enabled;
		} break;
	}
}

bool GodotJoint2D::get_flag(PS2DE::PinJointFlag p_flag) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, false);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			return angular_limit_enabled;
		}
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			return motor_enabled;
		}
	}
	ERR_FAIL_V(false);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, p_body_b ? 2 : 1),
		type(PS2DE::JOINT_TYPE_PIN) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = p_body_a->get_inv_transform().xform(p_pos);
	anchor_B = p_body_b ? p_body_b->get_inv_transform().xform(p_pos) : p_pos;

	p_body_a->add_constraint(this, 0);
	if (p_body_b) {
		p_body_b->add_constraint(this, 1);
		initial_angle = A->get_transform().get_origin().angle_to_point(B->get_transform().get_origin());
	}
}

// --- Groove Joint ---

bool GodotJoint2D::_setup_groove(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	// calculate endpoints in worldspace
	Vector2 ta = A->get_transform().xform(A_groove_1);
	Vector2 tb = A->get_transform().xform(A_groove_2);

	// calculate axis
	Vector2 axis_n = -(tb - ta).orthogonal().normalized();
	real_t d = ta.dot(axis_n);

	xf_normal = axis_n;
	rB = B->get_transform().basis_xform(B_anchor);

	// calculate tangential distance along the axis of rB
	real_t td = (B->get_transform().get_origin() + rB).cross(axis_n);
	// calculate clamping factor and rB
	if (td <= ta.cross(axis_n)) {
		clamp = 1.0f;
		rA = ta - A->get_transform().get_origin();
	} else if (td >= tb.cross(axis_n)) {
		clamp = -1.0f;
		rA = tb - A->get_transform().get_origin();
	} else {
		clamp = 0.0f;
		rA = ((-axis_n.orthogonal() * -td) + axis_n * d) - A->get_transform().get_origin();
	}

	// Calculate mass tensor
	k_tensor(A, B, rA, rB, &k1, &k2);

	// compute max impulse
	jn_max = get_max_force() * p_step;

	// calculate bias velocity
	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);

	real_t _b = get_bias();
	gbias = (delta * -(_b == 0 ? space->get_constraint_bias() : _b) * (1.0 / p_step)).limit_length(get_max_bias());

	correct = true;
	return true;
}

bool GodotJoint2D::_pre_solve_groove(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-jn_acc, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(jn_acc, rB);
	}

	return true;
}

void GodotJoint2D::_solve_groove(real_t p_step) {
	// compute impulse
	Vector2 vr = relative_velocity(A, B, rA, rB);

	Vector2 j_groove = mult_k(gbias - vr, k1, k2);
	Vector2 jOld = jn_acc;
	j_groove += jOld;

	jn_acc = (((clamp * j_groove.cross(xf_normal)) > 0) ? j_groove : j_groove.project(xf_normal)).limit_length(jn_max);

	j_groove = jn_acc - jOld;

	if (dynamic_A) {
		A->apply_impulse(-j_groove, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_groove, rB);
	}
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_GROOVE) {
	A = p_body_a;
	B = p_body_b;

	A_groove_1 = A->get_inv_transform().xform(p_a_groove1);
	A_groove_2 = A->get_inv_transform().xform(p_a_groove2);
	B_anchor = B->get_inv_transform().xform(p_b_anchor);
	A_groove_normal = -(A_groove_2 - A_groove_1).normalized().orthogonal();

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

// --- Damped Spring Joint ---

bool GodotJoint2D::_setup_damped_spring(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B->get_transform().basis_xform(anchor_B);

	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);
	real_t dist = delta.length();

	if (dist) {
		n = delta / dist;
	} else {
		n = Vector2();
	}

	real_t k = k_scalar(A, B, rA, rB, n);
	n_mass = 1.0f / k;

	target_vrn = 0.0f;
	v_coef = 1.0f - Math::exp(-damping * (p_step)*k);

	// Calculate spring force.
	real_t f_spring = (rest_length - dist) * stiffness;
	j = n * f_spring * (p_step);

	return true;
}

bool GodotJoint2D::_pre_solve_damped_spring(real_t p_step) {
	// Apply spring force.
	if (dynamic_A) {
		A->apply_impulse(-j, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j, rB);
	}

	return true;
}

void GodotJoint2D::_solve_damped_spring(real_t p_step) {
	// compute relative velocity
	real_t vrn = normal_relative_velocity(A, B, rA, rB, n) - target_vrn;

	// compute velocity loss from drag
	real_t v_damp = -vrn * v_coef;
	target_vrn = vrn + v_damp;
	Vector2 j_new = n * v_damp * n_mass;

	if (dynamic_A) {
		A->apply_impulse(-j_new, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_new, rB);
	}
}

void GodotJoint2D::set_param(PS2DE::DampedSpringParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_DAMPED_SPRING);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			rest_length = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			damping = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			stiffness = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::DampedSpringParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			return rest_length;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			return damping;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			return stiffness;
		} break;
	}

	ERR_FAIL_V(0);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_DAMPED_SPRING) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = A->get_inv_transform().xform(p_anchor_a);
	anchor_B = B->get_inv_transform().xform(p_anchor_b);

	rest_length = p_anchor_a.distance_to(p_anchor_b);

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}
