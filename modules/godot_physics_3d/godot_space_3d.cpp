/**************************************************************************/
/*  godot_space_3d.cpp                                                    */
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
#include "godot_area_pair_3d.h"
#include "godot_body_pair_3d.h"
#include "godot_collision_solver_3d.h"
#include "godot_physics_server_3d.h"
#include "godot_space_3d.h"

#define TEST_MOTION_MARGIN_MIN_VALUE 0.0001
#define TEST_MOTION_MIN_CONTACT_DEPTH_FACTOR 0.05

_FORCE_INLINE_ static bool _can_collide_with(GodotCollisionObject3D* p_object,
	uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas)
{
	if (!(p_object->get_collision_layer() & p_collision_mask)) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject3D::TYPE_AREA && !p_collide_with_areas) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject3D::TYPE_BODY && !p_collide_with_bodies) {
		return false;
	}

	if (p_object->get_type() == GodotCollisionObject3D::TYPE_SOFT_BODY && !p_collide_with_bodies) {
		return false;
	}

	return true;
}

bool GodotPhysicsDirectSpaceState3D::collide_shape(const PS3DT::ShapeParameters& p_parameters,
	Vector3* r_results, int p_result_max, int& r_result_count)
{
	if (p_result_max <= 0) {
		return false;
	}

	GodotShape3D* shape =
		GodotPhysicsServer3D::godot_singleton->shape_owner.get_or_null(p_parameters.shape_rid);
	ERR_FAIL_NULL_V(shape, false);

	AABB aabb = p_parameters.transform.xform(shape->get_aabb());
	aabb = aabb.grow(p_parameters.margin);

	int amount = space->broadphase->cull_aabb(aabb, space->intersection_query_results,
		GodotSpace3D::INTERSECTION_QUERY_MAX, space->intersection_query_subindex_results);

	bool collided = false;
	r_result_count = 0;

	GodotPhysicsServer3D::CollCbkData cbk;
	cbk.max = p_result_max;
	cbk.amount = 0;
	cbk.ptr = r_results;
	GodotCollisionSolver3D::CallbackResult cbkres = GodotPhysicsServer3D::_shape_col_cbk;

	GodotPhysicsServer3D::CollCbkData* cbkptr = &cbk;

	for (int i = 0; i < amount; i++) {
		if (!_can_collide_with(space->intersection_query_results[i], p_parameters.collision_mask,
				p_parameters.collide_with_bodies, p_parameters.collide_with_areas)) {
			continue;
		}

		const GodotCollisionObject3D* col_obj = space->intersection_query_results[i];

		if (p_parameters.exclude.has(col_obj->get_self())) {
			continue;
		}

		int shape_idx = space->intersection_query_subindex_results[i];

		if (GodotCollisionSolver3D::solve_static(shape, p_parameters.transform,
				col_obj->get_shape(shape_idx),
				col_obj->get_transform() * col_obj->get_shape_transform(shape_idx), cbkres, cbkptr,
				nullptr, p_parameters.margin)) {
			collided = true;
		}
	}

	r_result_count = cbk.amount;

	return collided;
}

struct _RestResultData
{
	const GodotCollisionObject3D* object = nullptr;
	int local_shape = 0;
	int shape = 0;
	Vector3 contact;
	Vector3 normal;
	real_t len = 0.0;
};

struct _RestCallbackData
{
	const GodotCollisionObject3D* object = nullptr;
	int local_shape = 0;
	int shape = 0;

	real_t min_allowed_depth = 0.0;

	_RestResultData best_result;

	int max_results = 0;
	int result_count = 0;
	_RestResultData* other_results = nullptr;
};

static void _rest_cbk_result(const Vector3& p_point_A, int p_index_A, const Vector3& p_point_B,
	int p_index_B, const Vector3& normal, void* p_userdata)
{
	_RestCallbackData* rd = static_cast<_RestCallbackData*>(p_userdata);

	Vector3 contact_rel = p_point_B - p_point_A;
	real_t len = contact_rel.length();
	if (len < rd->min_allowed_depth) {
		return;
	}

	bool is_best_result = (len > rd->best_result.len);

	if (rd->other_results && rd->result_count > 0) {
		// Consider as new result by default.
		int prev_result_count = rd->result_count++;

		int result_index = 0;
		real_t tested_len = is_best_result ? rd->best_result.len : len;
		for (; result_index < prev_result_count - 1; ++result_index) {
			if (tested_len > rd->other_results[result_index].len) {
				// Reusing a previous result.
				rd->result_count--;
				break;
			}
		}

		if (result_index < rd->max_results - 1) {
			_RestResultData& result = rd->other_results[result_index];

			if (is_best_result) {
				// Keep the previous best result as separate result.
				result = rd->best_result;
			}
			else {
				// Keep this result as separate result.
				result.len = len;
				result.contact = p_point_B;
				result.normal = normal;
				result.object = rd->object;
				result.shape = rd->shape;
				result.local_shape = rd->local_shape;
			}
		}
		else {
			// Discarding this result.
			rd->result_count--;
		}
	}
	else if (is_best_result) {
		rd->result_count = 1;
	}

	if (!is_best_result) {
		return;
	}

	rd->best_result.len = len;
	rd->best_result.contact = p_point_B;
	rd->best_result.normal = normal;
	rd->best_result.object = rd->object;
	rd->best_result.shape = rd->shape;
	rd->best_result.local_shape = rd->local_shape;
}

Vector3 GodotPhysicsDirectSpaceState3D::get_closest_point_to_object_volume(
	RID p_object, const Vector3 p_point) const
{
	GodotCollisionObject3D* obj =
		GodotPhysicsServer3D::godot_singleton->area_owner.get_or_null(p_object);
	if (!obj) {
		obj = GodotPhysicsServer3D::godot_singleton->body_owner.get_or_null(p_object);
	}
	ERR_FAIL_NULL_V(obj, Vector3());

	ERR_FAIL_COND_V(obj->get_space() != space, Vector3());

	real_t min_distance = 1e20;
	Vector3 min_point;

	bool shapes_found = false;

	for (int i = 0; i < obj->get_shape_count(); i++) {
		if (obj->is_shape_disabled(i)) {
			continue;
		}

		Transform3D shape_xform = obj->get_transform() * obj->get_shape_transform(i);
		GodotShape3D* shape = obj->get_shape(i);

		Vector3 point = shape->get_closest_point_to(shape_xform.affine_inverse().xform(p_point));
		point = shape_xform.xform(point);

		real_t dist = point.distance_to(p_point);
		if (dist < min_distance) {
			min_distance = dist;
			min_point = point;
		}
		shapes_found = true;
	}

	if (!shapes_found) {
		return obj->get_transform().origin; // no shapes found, use distance to origin.
	}
	else {
		return min_point;
	}
}

GodotPhysicsDirectSpaceState3D::GodotPhysicsDirectSpaceState3D() { space = nullptr; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GodotSpace3D::_cull_aabb_for_body(GodotBody3D* p_body, const AABB& p_aabb)
{
	int amount = broadphase->cull_aabb(p_aabb, intersection_query_results, INTERSECTION_QUERY_MAX,
		intersection_query_subindex_results);

	for (int i = 0; i < amount; i++) {
		bool keep = true;

		if (intersection_query_results[i] == p_body) {
			keep = false;
		}
		else if (intersection_query_results[i]->get_type() == GodotCollisionObject3D::TYPE_AREA) {
			keep = false;
		}
		else if (intersection_query_results[i]->get_type() ==
				   GodotCollisionObject3D::TYPE_SOFT_BODY) {
			keep = false;
		}
		else if (!p_body->collides_with(
					   static_cast<GodotBody3D*>(intersection_query_results[i]))) {
			keep = false;
		}
		else if (static_cast<GodotBody3D*>(intersection_query_results[i])
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
void* GodotSpace3D::_broadphase_pair(GodotCollisionObject3D* A, int p_subindex_A,
	GodotCollisionObject3D* B, int p_subindex_B, void* p_self)
{
	GodotCollisionObject3D::Type type_A = A->get_type();
	GodotCollisionObject3D::Type type_B = B->get_type();
	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(p_subindex_A, p_subindex_B);
		SWAP(type_A, type_B);
	}

	GodotSpace3D* self = static_cast<GodotSpace3D*>(p_self);

	self->collision_pairs++;

	if (type_A == GodotCollisionObject3D::TYPE_AREA) {
		GodotArea3D* area = static_cast<GodotArea3D*>(A);
		if (type_B == GodotCollisionObject3D::TYPE_AREA) {
			GodotArea3D* area_b = static_cast<GodotArea3D*>(B);
			GodotArea2Pair3D* area2_pair =
				memnew(GodotArea2Pair3D(area_b, p_subindex_B, area, p_subindex_A));
			return area2_pair;
		}
		else if (type_B == GodotCollisionObject3D::TYPE_SOFT_BODY) {
			GodotSoftBody3D* softbody = static_cast<GodotSoftBody3D*>(B);
			GodotAreaSoftBodyPair3D* soft_area_pair =
				memnew(GodotAreaSoftBodyPair3D(softbody, p_subindex_B, area, p_subindex_A));
			return soft_area_pair;
		}
		else {
			GodotBody3D* body = static_cast<GodotBody3D*>(B);
			GodotAreaPair3D* area_pair =
				memnew(GodotAreaPair3D(body, p_subindex_B, area, p_subindex_A));
			return area_pair;
		}
	}
	else if (type_A == GodotCollisionObject3D::TYPE_BODY) {
		if (type_B == GodotCollisionObject3D::TYPE_SOFT_BODY) {
			GodotBodySoftBodyPair3D* soft_pair = memnew(GodotBodySoftBodyPair3D(
				static_cast<GodotBody3D*>(A), p_subindex_A, static_cast<GodotSoftBody3D*>(B)));
			return soft_pair;
		}
		else {
			GodotBodyPair3D* b = memnew(GodotBodyPair3D(static_cast<GodotBody3D*>(A), p_subindex_A,
				static_cast<GodotBody3D*>(B), p_subindex_B));
			return b;
		}
	}
	else {
		// Soft Body/Soft Body, not supported.
	}

	return nullptr;
}

void GodotSpace3D::_broadphase_unpair(GodotCollisionObject3D* A, int p_subindex_A,
	GodotCollisionObject3D* B, int p_subindex_B, void* p_data, void* p_self)
{
	if (!p_data) {
		return;
	}

	GodotSpace3D* self = static_cast<GodotSpace3D*>(p_self);
	self->collision_pairs--;
	GodotConstraint3D* c = static_cast<GodotConstraint3D*>(p_data);
	memdelete(c);
}

const SelfList<GodotBody3D>::List& GodotSpace3D::get_active_body_list() const
{
	return active_list;
}

void GodotSpace3D::body_add_to_active_list(SelfList<GodotBody3D>* p_body)
{
	active_list.add(p_body);
}

void GodotSpace3D::body_remove_from_active_list(SelfList<GodotBody3D>* p_body)
{
	active_list.remove(p_body);
}

void GodotSpace3D::body_add_to_mass_properties_update_list(SelfList<GodotBody3D>* p_body)
{
	mass_properties_update_list.add(p_body);
}

void GodotSpace3D::body_remove_from_mass_properties_update_list(SelfList<GodotBody3D>* p_body)
{
	mass_properties_update_list.remove(p_body);
}

GodotBroadPhase3D* GodotSpace3D::get_broadphase() { return broadphase; }

void GodotSpace3D::add_object(GodotCollisionObject3D* p_object)
{
	ERR_FAIL_COND(objects.has(p_object));
	objects.insert(p_object);
}

void GodotSpace3D::remove_object(GodotCollisionObject3D* p_object)
{
	ERR_FAIL_COND(!objects.has(p_object));
	objects.erase(p_object);
}

const HashSet<GodotCollisionObject3D*>& GodotSpace3D::get_objects() const { return objects; }

void GodotSpace3D::body_add_to_state_query_list(SelfList<GodotBody3D>* p_body)
{
	state_query_list.add(p_body);
}

void GodotSpace3D::body_remove_from_state_query_list(SelfList<GodotBody3D>* p_body)
{
	state_query_list.remove(p_body);
}

void GodotSpace3D::area_add_to_monitor_query_list(SelfList<GodotArea3D>* p_area)
{
	monitor_query_list.add(p_area);
}

void GodotSpace3D::area_remove_from_monitor_query_list(SelfList<GodotArea3D>* p_area)
{
	monitor_query_list.remove(p_area);
}

void GodotSpace3D::area_add_to_moved_list(SelfList<GodotArea3D>* p_area)
{
	area_moved_list.add(p_area);
}

void GodotSpace3D::area_remove_from_moved_list(SelfList<GodotArea3D>* p_area)
{
	area_moved_list.remove(p_area);
}

const SelfList<GodotArea3D>::List& GodotSpace3D::get_moved_area_list() const
{
	return area_moved_list;
}

const SelfList<GodotSoftBody3D>::List& GodotSpace3D::get_active_soft_body_list() const
{
	return active_soft_body_list;
}

void GodotSpace3D::soft_body_add_to_active_list(SelfList<GodotSoftBody3D>* p_soft_body)
{
	active_soft_body_list.add(p_soft_body);
}

void GodotSpace3D::soft_body_remove_from_active_list(SelfList<GodotSoftBody3D>* p_soft_body)
{
	active_soft_body_list.remove(p_soft_body);
}

void GodotSpace3D::call_queries()
{
	while (state_query_list.first()) {
		GodotBody3D* b = state_query_list.first()->self();
		state_query_list.remove(state_query_list.first());
		b->call_queries();
	}

	while (monitor_query_list.first()) {
		GodotArea3D* a = monitor_query_list.first()->self();
		monitor_query_list.remove(monitor_query_list.first());
		a->call_queries();
	}
}

void GodotSpace3D::setup()
{
	contact_debug_count = 0;
	while (mass_properties_update_list.first()) {
		mass_properties_update_list.first()->self()->update_mass_properties();
		mass_properties_update_list.remove(mass_properties_update_list.first());
	}
}

void GodotSpace3D::update() { broadphase->update(); }

void GodotSpace3D::set_param(PS3DE::SpaceParameter p_param, real_t p_value)
{
	switch (p_param) {
	case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		contact_recycle_radius = p_value;
		break;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		contact_max_separation = p_value;
		break;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		contact_max_allowed_penetration = p_value;
		break;
	case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		contact_bias = p_value;
		break;
	case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		body_linear_velocity_sleep_threshold = p_value;
		break;
	case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		body_angular_velocity_sleep_threshold = p_value;
		break;
	case PS3DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		body_time_to_sleep = p_value;
		break;
	case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS:
		solver_iterations = p_value;
		break;
	}
}

real_t GodotSpace3D::get_param(PS3DE::SpaceParameter p_param) const
{
	switch (p_param) {
	case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS:
		return contact_recycle_radius;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION:
		return contact_max_separation;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION:
		return contact_max_allowed_penetration;
	case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS:
		return contact_bias;
	case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD:
		return body_linear_velocity_sleep_threshold;
	case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD:
		return body_angular_velocity_sleep_threshold;
	case PS3DE::SPACE_PARAM_BODY_TIME_TO_SLEEP:
		return body_time_to_sleep;
	case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS:
		return solver_iterations;
	}
	return 0;
}

void GodotSpace3D::lock() { locked = true; }

void GodotSpace3D::unlock() { locked = false; }

bool GodotSpace3D::is_locked() const { return locked; }

GodotPhysicsDirectSpaceState3D* GodotSpace3D::get_direct_state() { return direct_access; }

GodotSpace3D::~GodotSpace3D()
{
	memdelete(broadphase);
	memdelete(direct_access);
}


