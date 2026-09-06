/**************************************************************************/
/*  godot_space_2d.cpp                                                    */
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
#include "godot_area_pair_2d.h"
#include "godot_body_pair_2d.h"
#include "godot_collision_solver_2d.h"
#include "godot_physics_server_2d.h"
#include "godot_space_2d.h"

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

int GodotPhysicsDirectSpaceState2D::intersect_point(
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

bool GodotPhysicsDirectSpaceState2D::intersect_ray(
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

	// todo, create another array that references results, compute AABBs and check closest point to
	// ray origin, sort, and stop evaluating results when beyond first collision

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
				// Hit shape at starting point.
				min_d = 0;
				res_point = begin;
				res_normal = Vector2();
				res_shape = shape_idx;
				res_obj = col_obj;
				collided = true;
				break;
			}
			else {
				// Ignore shape when starting inside.
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
	ERR_FAIL_NULL_V(res_obj, false); // Shouldn't happen but silences warning.

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

// Assumes a valid collision pair, this should have been checked beforehand in the BVH or octree.
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

GodotPhysicsDirectSpaceState2D* GodotSpace2D::get_direct_state() { return direct_access; }

GodotSpace2D::~GodotSpace2D()
{
	memdelete(broadphase);
	memdelete(direct_access);
}


