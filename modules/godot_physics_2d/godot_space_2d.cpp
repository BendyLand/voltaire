/**************************************************************************/
/*  godot_space_2d.cpp                                                    */
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

#include "godot_space_2d.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "godot_collision_solver_2d.h"
#include "godot_constraints_2d.h"
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
