/**************************************************************************/
/*  godot_objects_2d.cpp                                                  */
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

#include "godot_objects_2d.h"

#include "godot_constraints_2d.h"
#include "godot_physics_server_2d.h"
#include "godot_space_2d.h"
#include "servers/physics_2d/direct_states/physics_direct_body_state_2d.h"

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
