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
#include "modules/godot_physics_2d/godot_shape_2d.h"
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
#include "modules/godot_physics_2d/godot_shape_2d.h"
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
