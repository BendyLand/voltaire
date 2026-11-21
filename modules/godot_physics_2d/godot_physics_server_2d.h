/**************************************************************************/
/*  godot_physics_server_2d.h                                             */
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

#include "core/templates/rid_owner.h"
// --- Folded from modules/godot_physics_2d/godot_constraints_2d.h ---
// --- Folded from modules/godot_physics_2d/godot_objects_2d.h ---
#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/pair.h"
#include "core/templates/self_list.h"
#include "core/templates/vset.h"
#include "core/typedefs.h"
#include "godot_shape_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

class GodotSpace2D;
class GodotArea2D;
class GodotBody2D;
class GodotConstraint2D;
class PhysicsDirectBodyState2D;

class GodotCollisionObject2D
{
public:
	enum Type
	{
		TYPE_AREA,
		TYPE_BODY
	};

private:
	Type type;
	RID self;
	bool pickable = true;

	struct Shape
	{
		Transform2D xform;
		Transform2D xform_inv;
		uint32_t bpid = 0;
		Rect2 aabb_cache; // for rayqueries
		GodotShape2D* shape = nullptr;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0;
		Vector2 one_way_collision_direction = Vector2(0.0, 1.0);
	};

	Vector<Shape> shapes;
	GodotSpace2D* space = nullptr;
	Transform2D transform;
	Transform2D inv_transform;
	uint32_t collision_mask = 1;
	uint32_t collision_layer = 1;
	real_t collision_priority = 1.0;
	bool _static = true;

	SelfList<GodotCollisionObject2D> pending_shape_update_list;

	void _update_shapes();

protected:
	void _update_shapes_with_motion(const Vector2& p_motion);
	void _unregister_shapes();

	_FORCE_INLINE_ void _set_transform(const Transform2D& p_transform, bool p_update_shapes = true)
	{
		transform = p_transform;
		if (p_update_shapes) {
			_update_shapes();
		}
	}

	_FORCE_INLINE_ void _set_inv_transform(const Transform2D& p_transform)
	{
		inv_transform = p_transform;
	}

	void _set_static(bool p_static);

	virtual void _shapes_changed() = 0;
	void _set_space(GodotSpace2D* p_space);

	GodotCollisionObject2D(Type p_type);

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	void _shape_changed();

	_FORCE_INLINE_ Type get_type() const { return type; }

	void add_shape(GodotShape2D* p_shape, const Transform2D& p_transform = Transform2D(),
		bool p_disabled = false);
	void set_shape(int p_index, GodotShape2D* p_shape);
	void set_shape_transform(int p_index, const Transform2D& p_transform);

	_FORCE_INLINE_ int get_shape_count() const { return shapes.size(); }

	_FORCE_INLINE_ GodotShape2D* get_shape(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].shape;
	}

	_FORCE_INLINE_ const Transform2D& get_shape_transform(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].xform;
	}

	_FORCE_INLINE_ const Transform2D& get_shape_inv_transform(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].xform_inv;
	}

	_FORCE_INLINE_ const Rect2& get_shape_aabb(int p_index) const
	{
		CRASH_BAD_INDEX(p_index, shapes.size());
		return shapes[p_index].aabb_cache;
	}

	_FORCE_INLINE_ const Transform2D& get_transform() const { return transform; }

	_FORCE_INLINE_ const Transform2D& get_inv_transform() const { return inv_transform; }

	_FORCE_INLINE_ GodotSpace2D* get_space() const { return space; }

	void set_shape_disabled(int p_idx, bool p_disabled);

	_FORCE_INLINE_ bool is_shape_disabled(int p_idx) const
	{
		ERR_FAIL_INDEX_V(p_idx, shapes.size(), false);
		return shapes[p_idx].disabled;
	}

	_FORCE_INLINE_ void set_shape_as_one_way_collision(
		int p_idx, bool p_one_way_collision, real_t p_margin, const Vector2& p_direction)
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		shapes.write[p_idx].one_way_collision = p_one_way_collision;
		shapes.write[p_idx].one_way_collision_margin = p_margin;
		shapes.write[p_idx].one_way_collision_direction = p_direction;
	}

	_FORCE_INLINE_ bool is_shape_set_as_one_way_collision(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision;
	}

	_FORCE_INLINE_ real_t get_shape_one_way_collision_margin(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision_margin;
	}

	Vector2 get_shape_one_way_collision_direction(int p_idx) const
	{
		CRASH_BAD_INDEX(p_idx, shapes.size());
		return shapes[p_idx].one_way_collision_direction;
	}

	void set_collision_mask(uint32_t p_mask)
	{
		collision_mask = p_mask;
		_shape_changed();
	}

	_FORCE_INLINE_ uint32_t get_collision_mask() const { return collision_mask; }

	void set_collision_layer(uint32_t p_layer)
	{
		collision_layer = p_layer;
		_shape_changed();
	}

	_FORCE_INLINE_ uint32_t get_collision_layer() const { return collision_layer; }

	_FORCE_INLINE_ void set_collision_priority(real_t p_priority)
	{
		ERR_FAIL_COND_MSG(p_priority <= 0, "Priority must be greater than 0.");
		collision_priority = p_priority;
		_shape_changed();
	}

	_FORCE_INLINE_ real_t get_collision_priority() const { return collision_priority; }

	void remove_shape(GodotShape2D* p_shape);
	void remove_shape(int p_index);

	virtual void set_space(GodotSpace2D* p_space) = 0;

	_FORCE_INLINE_ bool is_static() const { return _static; }

	void set_pickable(bool p_pickable) { pickable = p_pickable; }

	_FORCE_INLINE_ bool is_pickable() const { return pickable; }

	_FORCE_INLINE_ bool collides_with(GodotCollisionObject2D* p_other) const
	{
		return p_other->collision_layer & collision_mask;
	}

	_FORCE_INLINE_ bool interacts_with(const GodotCollisionObject2D* p_other) const
	{
		return collision_layer & p_other->collision_mask ||
			   p_other->collision_layer & collision_mask;
	}

	virtual ~GodotCollisionObject2D() {}
};

class GodotArea2D final : public GodotCollisionObject2D
{
	PS2DE::AreaSpaceOverrideMode gravity_override_mode = PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	PS2DE::AreaSpaceOverrideMode linear_damping_override_mode = PS2DE::AREA_SPACE_OVERRIDE_DISABLED;
	PS2DE::AreaSpaceOverrideMode angular_damping_override_mode =
		PS2DE::AREA_SPACE_OVERRIDE_DISABLED;

	real_t gravity = 9.80665;
	Vector2 gravity_vector = Vector2(0, -1);
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0;
	real_t linear_damp = 0.1;
	real_t angular_damp = 1.0;
	int priority = 0;
	bool monitorable = false;

	SelfList<GodotArea2D> monitor_query_list;
	SelfList<GodotArea2D> moved_list;

	struct BodyKey
	{
		RID rid;
		uint32_t body_shape = 0;
		uint32_t area_shape = 0;

		static uint32_t hash(const BodyKey& p_key)
		{
			uint32_t h = hash_one_uint64(p_key.rid.get_id());
			h = hash_murmur3_one_32(p_key.area_shape, h);
			return hash_fmix32(hash_murmur3_one_32(p_key.body_shape, h));
		}

		_FORCE_INLINE_ bool operator==(const BodyKey& p_key) const
		{
			return rid == p_key.rid && body_shape == p_key.body_shape &&
				   area_shape == p_key.area_shape;
		}

		_FORCE_INLINE_ BodyKey() {}

		BodyKey(GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
		BodyKey(GodotArea2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
	};

	struct BodyState
	{
		int state = 0;

		_FORCE_INLINE_ void inc() { state++; }

		_FORCE_INLINE_ void dec() { state--; }
	};

	HashMap<BodyKey, BodyState, BodyKey> monitored_bodies;
	HashMap<BodyKey, BodyState, BodyKey> monitored_areas;

	HashSet<GodotConstraint2D*> constraints;

	virtual void _shapes_changed() override;
	void _queue_monitor_update();

	void _set_space_override_mode(
		PS2DE::AreaSpaceOverrideMode& r_mode, PS2DE::AreaSpaceOverrideMode p_new_mode);

public:
	_FORCE_INLINE_ void add_body_to_query(
		GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);
	_FORCE_INLINE_ void remove_body_from_query(
		GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape);

	_FORCE_INLINE_ void add_area_to_query(
		GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape);
	_FORCE_INLINE_ void remove_area_from_query(
		GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape);

	_FORCE_INLINE_ void set_gravity(real_t p_gravity) { gravity = p_gravity; }

	_FORCE_INLINE_ real_t get_gravity() const { return gravity; }

	_FORCE_INLINE_ void set_gravity_vector(const Vector2& p_gravity) { gravity_vector = p_gravity; }

	_FORCE_INLINE_ Vector2 get_gravity_vector() const { return gravity_vector; }

	_FORCE_INLINE_ void set_gravity_as_point(bool p_enable) { gravity_is_point = p_enable; }

	_FORCE_INLINE_ bool is_gravity_point() const { return gravity_is_point; }

	_FORCE_INLINE_ void set_gravity_point_unit_distance(real_t scale)
	{
		gravity_point_unit_distance = scale;
	}

	_FORCE_INLINE_ real_t get_gravity_point_unit_distance() const
	{
		return gravity_point_unit_distance;
	}

	_FORCE_INLINE_ void set_linear_damp(real_t p_linear_damp) { linear_damp = p_linear_damp; }

	_FORCE_INLINE_ real_t get_linear_damp() const { return linear_damp; }

	_FORCE_INLINE_ void set_angular_damp(real_t p_angular_damp) { angular_damp = p_angular_damp; }

	_FORCE_INLINE_ real_t get_angular_damp() const { return angular_damp; }

	_FORCE_INLINE_ void set_priority(int p_priority) { priority = p_priority; }

	_FORCE_INLINE_ int get_priority() const { return priority; }

	_FORCE_INLINE_ void add_constraint(GodotConstraint2D* p_constraint)
	{
		constraints.insert(p_constraint);
	}

	_FORCE_INLINE_ void remove_constraint(GodotConstraint2D* p_constraint)
	{
		constraints.erase(p_constraint);
	}

	_FORCE_INLINE_ const HashSet<GodotConstraint2D*>& get_constraints() const
	{
		return constraints;
	}

	_FORCE_INLINE_ void clear_constraints() { constraints.clear(); }

	void set_monitorable(bool p_monitorable);

	_FORCE_INLINE_ bool is_monitorable() const { return monitorable; }

	void set_transform(const Transform2D& p_transform);

	void set_space(GodotSpace2D* p_space) override;

	void call_queries();

	void compute_gravity(const Vector2& p_position, Vector2& r_gravity) const;

	GodotArea2D();
	~GodotArea2D();
};

void GodotArea2D::add_body_to_query(
	GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	BodyKey bk(p_body, p_body_shape, p_area_shape);
	monitored_bodies[bk].inc();
	if (!monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::remove_body_from_query(
	GodotBody2D* p_body, uint32_t p_body_shape, uint32_t p_area_shape)
{
	BodyKey bk(p_body, p_body_shape, p_area_shape);
	monitored_bodies[bk].dec();
	if (get_space() && !monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::add_area_to_query(
	GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape)
{
	BodyKey bk(p_area, p_area_shape, p_self_shape);
	monitored_areas[bk].inc();
	if (!monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

void GodotArea2D::remove_area_from_query(
	GodotArea2D* p_area, uint32_t p_area_shape, uint32_t p_self_shape)
{
	BodyKey bk(p_area, p_area_shape, p_self_shape);
	monitored_areas[bk].dec();
	if (get_space() && !monitor_query_list.in_list()) {
		_queue_monitor_update();
	}
}

class GodotBody2D final : public GodotCollisionObject2D
{
	PS2DE::BodyMode mode = PS2DE::BODY_MODE_RIGID;

	Vector2 biased_linear_velocity;
	real_t biased_angular_velocity = 0.0;

	Vector2 linear_velocity;
	real_t angular_velocity = 0.0;

	Vector2 prev_linear_velocity;
	real_t prev_angular_velocity = 0.0;

	Vector2 constant_linear_velocity;
	real_t constant_angular_velocity = 0.0;

	PS2DE::BodyDampMode linear_damp_mode = PS2DE::BODY_DAMP_MODE_COMBINE;
	PS2DE::BodyDampMode angular_damp_mode = PS2DE::BODY_DAMP_MODE_COMBINE;

	real_t linear_damp = 0.0;
	real_t angular_damp = 0.0;

	real_t total_linear_damp = 0.0;
	real_t total_angular_damp = 0.0;

	real_t gravity_scale = 1.0;

	real_t bounce = 0.0;
	real_t friction = 1.0;

	real_t mass = 1.0;
	real_t _inv_mass = 1.0;

	real_t inertia = 0.0;
	real_t _inv_inertia = 0.0;

	Vector2 center_of_mass_local;
	Vector2 center_of_mass;

	bool calculate_inertia = true;
	bool calculate_center_of_mass = true;

	Vector2 gravity;

	real_t still_time = 0.0;

	Vector2 applied_force;
	real_t applied_torque = 0.0;

	Vector2 constant_force;
	real_t constant_torque = 0.0;

	SelfList<GodotBody2D> active_list;
	SelfList<GodotBody2D> mass_properties_update_list;
	SelfList<GodotBody2D> direct_state_query_list;

	VSet<RID> exceptions;
	PS2DE::CCDMode continuous_cd_mode = PS2DE::CCD_MODE_DISABLED;
	bool omit_force_integration = false;
	bool active = true;
	bool can_sleep = true;
	bool first_time_kinematic = false;
	void _mass_properties_changed();
	virtual void _shapes_changed() override;
	Transform2D new_transform;

	List<Pair<GodotConstraint2D*, int>> constraint_list;

	struct AreaCMP
	{
		GodotArea2D* area = nullptr;
		int refCount = 0;

		_FORCE_INLINE_ bool operator==(const AreaCMP& p_cmp) const
		{
			return area->get_self() == p_cmp.area->get_self();
		}

		_FORCE_INLINE_ bool operator<(const AreaCMP& p_cmp) const
		{
			return area->get_priority() < p_cmp.area->get_priority();
		}

		_FORCE_INLINE_ AreaCMP() {}

		_FORCE_INLINE_ AreaCMP(GodotArea2D* p_area)
		{
			area = p_area;
			refCount = 1;
		}
	};

	Vector<AreaCMP> areas;

	struct Contact
	{
		Vector2 local_pos;
		Vector2 local_normal;
		Vector2 local_velocity_at_pos;
		real_t depth = 0.0;
		int local_shape = 0;
		Vector2 collider_pos;
		int collider_shape = 0;
		RID collider;
		Vector2 collider_velocity_at_pos;
		Vector2 impulse;
	};

	Vector<Contact> contacts; // no contacts by default
	int contact_count = 0;

	PhysicsDirectBodyState2D* direct_state = nullptr;

	uint64_t island_step = 0;

	void _update_transform_dependent();

	friend class PhysicsDirectBodyState2D;

public:
	void add_contact(const Vector2& p_local_pos, const Vector2& p_local_normal, real_t p_depth,
		int p_local_shape, const Vector2& p_local_velocity_at_pos, const Vector2& p_collider_pos,
		int p_collider_shape, const RID& p_collider, const Vector2& p_collider_velocity_at_pos,
		const Vector2& p_impulse);

	PhysicsDirectBodyState2D* get_direct_state();

	_FORCE_INLINE_ void add_area(GodotArea2D* p_area)
	{
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount += 1;
		}
		else {
			areas.ordered_insert(AreaCMP(p_area));
		}
	}

	_FORCE_INLINE_ void remove_area(GodotArea2D* p_area)
	{
		int index = areas.find(AreaCMP(p_area));
		if (index > -1) {
			areas.write[index].refCount -= 1;
			if (areas[index].refCount < 1) {
				areas.remove_at(index);
			}
		}
	}

	_FORCE_INLINE_ void set_max_contacts_reported(int p_size)
	{
		ERR_FAIL_INDEX(p_size, PS2DC::MAX_CONTACTS_REPORTED_2D_MAX);
		contacts.resize(p_size);
		contact_count = 0;
		if (mode == PS2DE::BODY_MODE_KINEMATIC && p_size) {
			set_active(true);
		}
	}

	_FORCE_INLINE_ int get_max_contacts_reported() const { return contacts.size(); }

	_FORCE_INLINE_ bool can_report_contacts() const { return !contacts.is_empty(); }

	_FORCE_INLINE_ void add_exception(const RID& p_exception) { exceptions.insert(p_exception); }

	_FORCE_INLINE_ void remove_exception(const RID& p_exception) { exceptions.erase(p_exception); }

	_FORCE_INLINE_ bool has_exception(const RID& p_exception) const
	{
		return exceptions.has(p_exception);
	}

	_FORCE_INLINE_ const VSet<RID>& get_exceptions() const { return exceptions; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }

	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ void add_constraint(GodotConstraint2D* p_constraint, int p_pos)
	{
		constraint_list.push_back({p_constraint, p_pos});
	}

	_FORCE_INLINE_ void remove_constraint(GodotConstraint2D* p_constraint, int p_pos)
	{
		constraint_list.erase({p_constraint, p_pos});
	}

	const List<Pair<GodotConstraint2D*, int>>& get_constraint_list() const
	{
		return constraint_list;
	}

	_FORCE_INLINE_ void clear_constraint_list() { constraint_list.clear(); }

	_FORCE_INLINE_ void set_omit_force_integration(bool p_omit_force_integration)
	{
		omit_force_integration = p_omit_force_integration;
	}

	_FORCE_INLINE_ bool get_omit_force_integration() const { return omit_force_integration; }

	_FORCE_INLINE_ void set_linear_velocity(const Vector2& p_velocity)
	{
		linear_velocity = p_velocity;
	}

	_FORCE_INLINE_ Vector2 get_linear_velocity() const { return linear_velocity; }

	_FORCE_INLINE_ void set_angular_velocity(real_t p_velocity) { angular_velocity = p_velocity; }

	_FORCE_INLINE_ real_t get_angular_velocity() const { return angular_velocity; }

	_FORCE_INLINE_ Vector2 get_prev_linear_velocity() const { return prev_linear_velocity; }

	_FORCE_INLINE_ real_t get_prev_angular_velocity() const { return prev_angular_velocity; }

	_FORCE_INLINE_ void set_biased_linear_velocity(const Vector2& p_velocity)
	{
		biased_linear_velocity = p_velocity;
	}

	_FORCE_INLINE_ Vector2 get_biased_linear_velocity() const { return biased_linear_velocity; }

	_FORCE_INLINE_ void set_biased_angular_velocity(real_t p_velocity)
	{
		biased_angular_velocity = p_velocity;
	}

	_FORCE_INLINE_ real_t get_biased_angular_velocity() const { return biased_angular_velocity; }

	_FORCE_INLINE_ void apply_central_impulse(const Vector2& p_impulse)
	{
		linear_velocity += p_impulse * _inv_mass;
	}

	_FORCE_INLINE_ void apply_impulse(
		const Vector2& p_impulse, const Vector2& p_position = Vector2())
	{
		linear_velocity += p_impulse * _inv_mass;
		angular_velocity += _inv_inertia * (p_position - center_of_mass).cross(p_impulse);
	}

	_FORCE_INLINE_ void apply_torque_impulse(real_t p_torque)
	{
		angular_velocity += _inv_inertia * p_torque;
	}

	_FORCE_INLINE_ void apply_bias_impulse(const Vector2& p_impulse,
		const Vector2& p_position = Vector2(), real_t p_max_delta_av = -1.0)
	{
		biased_linear_velocity += p_impulse * _inv_mass;
		if (p_max_delta_av != 0.0) {
			real_t delta_av = _inv_inertia * (p_position - center_of_mass).cross(p_impulse);
			if (p_max_delta_av > 0 && delta_av > p_max_delta_av) {
				delta_av = p_max_delta_av;
			}
			biased_angular_velocity += delta_av;
		}
	}

	_FORCE_INLINE_ void apply_central_force(const Vector2& p_force) { applied_force += p_force; }

	_FORCE_INLINE_ void apply_force(const Vector2& p_force, const Vector2& p_position = Vector2())
	{
		applied_force += p_force;
		applied_torque += (p_position - center_of_mass).cross(p_force);
	}

	_FORCE_INLINE_ void apply_torque(real_t p_torque) { applied_torque += p_torque; }

	_FORCE_INLINE_ void add_constant_central_force(const Vector2& p_force)
	{
		constant_force += p_force;
	}

	_FORCE_INLINE_ void add_constant_force(
		const Vector2& p_force, const Vector2& p_position = Vector2())
	{
		constant_force += p_force;
		constant_torque += (p_position - center_of_mass).cross(p_force);
	}

	_FORCE_INLINE_ void add_constant_torque(real_t p_torque) { constant_torque += p_torque; }

	void set_constant_force(const Vector2& p_force) { constant_force = p_force; }

	Vector2 get_constant_force() const { return constant_force; }

	void set_constant_torque(real_t p_torque) { constant_torque = p_torque; }

	real_t get_constant_torque() const { return constant_torque; }

	void set_active(bool p_active);

	_FORCE_INLINE_ bool is_active() const { return active; }

	_FORCE_INLINE_ void wakeup()
	{
		if ((!get_space()) || mode == PS2DE::BODY_MODE_STATIC ||
			mode == PS2DE::BODY_MODE_KINEMATIC) {
			return;
		}
		set_active(true);
	}

	void set_mode(PS2DE::BodyMode p_mode);
	PS2DE::BodyMode get_mode() const;

	_FORCE_INLINE_ void set_continuous_collision_detection_mode(PS2DE::CCDMode p_mode)
	{
		continuous_cd_mode = p_mode;
	}

	_FORCE_INLINE_ PS2DE::CCDMode get_continuous_collision_detection_mode() const
	{
		return continuous_cd_mode;
	}

	void set_space(GodotSpace2D* p_space) override;

	void update_mass_properties();
	void reset_mass_properties();

	_FORCE_INLINE_ const Vector2& get_center_of_mass() const { return center_of_mass; }

	_FORCE_INLINE_ const Vector2& get_center_of_mass_local() const { return center_of_mass_local; }

	_FORCE_INLINE_ real_t get_inv_mass() const { return _inv_mass; }

	_FORCE_INLINE_ real_t get_inv_inertia() const { return _inv_inertia; }

	_FORCE_INLINE_ real_t get_friction() const { return friction; }

	_FORCE_INLINE_ real_t get_bounce() const { return bounce; }

	void integrate_forces(real_t p_step);
	void integrate_velocities(real_t p_step);

	_FORCE_INLINE_ Vector2 get_velocity_in_local_point(const Vector2& rel_pos) const
	{
		return linear_velocity + Vector2(-angular_velocity * (rel_pos.y - center_of_mass.y),
									 angular_velocity * (rel_pos.x - center_of_mass.x));
	}

	_FORCE_INLINE_ Vector2 get_motion() const
	{
		if (mode > PS2DE::BODY_MODE_KINEMATIC) {
			return new_transform.get_origin() - get_transform().get_origin();
		}
		else if (mode == PS2DE::BODY_MODE_KINEMATIC) {
			return get_transform().get_origin() -
				   new_transform.get_origin(); // kinematic simulates forward
		}
		return Vector2();
	}

	void call_queries();
	void wakeup_neighbours();

	bool sleep_test(real_t p_step);

	GodotBody2D();
	~GodotBody2D();
};

_FORCE_INLINE_ void GodotBody2D::add_contact(const Vector2& p_local_pos, const Vector2& p_local_normal,
	real_t p_depth, int p_local_shape, const Vector2& p_local_velocity_at_pos,
	const Vector2& p_collider_pos, int p_collider_shape, const RID& p_collider,
	const Vector2& p_collider_velocity_at_pos, const Vector2& p_impulse)
{
	int c_max = contacts.size();

	if (c_max == 0) {
		return;
	}

	Contact* c = contacts.ptrw();

	int idx = -1;

	if (contact_count < c_max) {
		idx = contact_count++;
	}
	else {
		real_t least_depth = 1e20;
		int least_deep = -1;
		for (int i = 0; i < c_max; i++) {
			if (i == 0 || c[i].depth < least_depth) {
				least_deep = i;
				least_depth = c[i].depth;
			}
		}

		if (least_deep >= 0 && least_depth < p_depth) {
			idx = least_deep;
		}
		if (idx == -1) {
			return; // none least deepe than this
		}
	}

	c[idx].local_pos = p_local_pos;
	c[idx].local_normal = p_local_normal;
	c[idx].local_velocity_at_pos = p_local_velocity_at_pos;
	c[idx].depth = p_depth;
	c[idx].local_shape = p_local_shape;
	c[idx].collider_pos = p_collider_pos;
	c[idx].collider_shape = p_collider_shape;
	c[idx].collider = p_collider;
	c[idx].collider_velocity_at_pos = p_collider_velocity_at_pos;
	c[idx].impulse = p_impulse;
}

// --- End modules/godot_physics_2d/godot_objects_2d.h ---

class GodotConstraint2D {
	GodotBody2D **_body_ptr;
	int _body_count;
	uint64_t island_step = 0;
	bool disabled_collisions_between_bodies = true;

	RID self;

protected:
	GodotConstraint2D(GodotBody2D **p_body_ptr = nullptr, int p_body_count = 0) {
		_body_ptr = p_body_ptr;
		_body_count = p_body_count;
	}

public:
	_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }

	_FORCE_INLINE_ uint64_t get_island_step() const { return island_step; }
	_FORCE_INLINE_ void set_island_step(uint64_t p_step) { island_step = p_step; }

	_FORCE_INLINE_ GodotBody2D **get_body_ptr() const { return _body_ptr; }
	_FORCE_INLINE_ int get_body_count() const { return _body_count; }

	_FORCE_INLINE_ void disable_collisions_between_bodies(const bool p_disabled) { disabled_collisions_between_bodies = p_disabled; }
	_FORCE_INLINE_ bool is_disabled_collisions_between_bodies() const { return disabled_collisions_between_bodies; }

	virtual bool setup(real_t p_step) = 0;
	virtual bool pre_solve(real_t p_step) = 0;
	virtual void solve(real_t p_step) = 0;

	virtual ~GodotConstraint2D() {}
};

class GodotBodyPair2D final : public GodotConstraint2D {
	enum {
		MAX_CONTACTS = 2
	};
	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	int shape_A = 0;
	int shape_B = 0;

	bool collide_A = false;
	bool collide_B = false;

	GodotSpace2D *space = nullptr;

	struct Contact {
		Vector2 position;
		Vector2 normal;
		Vector2 local_A, local_B;
		Vector2 acc_impulse; // accumulated impulse
		real_t acc_normal_impulse = 0.0; // accumulated normal impulse (Pn)
		real_t acc_tangent_impulse = 0.0; // accumulated tangent impulse (Pt)
		real_t acc_bias_impulse = 0.0; // accumulated normal impulse for position bias (Pnb)
		real_t acc_bias_impulse_center_of_mass = 0.0; // accumulated normal impulse for position bias applied to com
		real_t mass_normal = 0.0, mass_tangent = 0.0;
		real_t bias = 0.0;

		real_t depth = 0.0;
		bool active = false;
		bool used = false;
		Vector2 rA, rB;
		real_t bounce = 0.0;
	};

	Vector2 offset_B; //use local A coordinates to avoid numerical issues on collision detection

	Vector2 sep_axis;
	Contact contacts[MAX_CONTACTS];
	int contact_count = 0;
	bool collided = false;
	bool check_ccd = false;
	bool oneway_disabled = false;
	bool report_contacts_only = false;

	bool _test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B);
	void _validate_contacts();
	static void _add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self);
	_FORCE_INLINE_ void _contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B);

public:
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B);
	virtual ~GodotBodyPair2D() override;
};

class GodotAreaPair2D final : public GodotConstraint2D {
	GodotBody2D *body = nullptr;
	GodotArea2D *area = nullptr;
	int body_shape = 0;
	int area_shape = 0;
	bool colliding = false;
	bool has_space_override = false;
	bool process_collision = false;
	bool body_has_attached_area = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape);
	virtual ~GodotAreaPair2D() override = default;
};

class GodotArea2Pair2D final : public GodotConstraint2D {
	GodotArea2D *area_a = nullptr;
	GodotArea2D *area_b = nullptr;
	int shape_a = 0;
	int shape_b = 0;
	bool colliding_a = false;
	bool colliding_b = false;
	bool process_collision_a = false;
	bool process_collision_b = false;
	bool area_a_monitorable = false;
	bool area_b_monitorable = false;

public:
	virtual void solve(real_t p_step) override;
	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;

	GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b);
	virtual ~GodotArea2Pair2D() override = default;
};

class GodotJoint2D final : public GodotConstraint2D {
	PS2DE::JointType type = PS2DE::JOINT_TYPE_MAX;

	union {
		struct {
			GodotBody2D *A;
			GodotBody2D *B;
		};

		GodotBody2D *_arr[2] = { nullptr, nullptr };
	};

	real_t bias = 0.0;
	real_t max_bias = 3.40282e+38;
	real_t max_force = 3.40282e+38;

	bool dynamic_A = false;
	bool dynamic_B = false;

	// Shared working fields
	Vector2 rA, rB;
	Vector2 anchor_A;
	Vector2 anchor_B;
	real_t jn_max = 0.0;

	// Pin joint fields
	Transform2D M;
	Vector2 pin_bias;
	real_t initial_angle = 0.0;
	real_t bias_velocity = 0.0;
	real_t j_acc = 0.0;
	real_t i_sum = 0.0;
	Vector2 P;
	real_t softness = 0.0;
	real_t angular_limit_lower = 0.0;
	real_t angular_limit_upper = 0.0;
	real_t motor_target_velocity = 0.0;
	bool is_joint_at_limit = false;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove joint fields
	Vector2 A_groove_1;
	Vector2 A_groove_2;
	Vector2 A_groove_normal;
	Vector2 B_anchor;
	Vector2 jn_acc;
	Vector2 gbias;
	real_t clamp = 0.0;
	Vector2 xf_normal;
	Vector2 k1, k2;
	bool correct = false;

	// Damped spring joint fields
	real_t rest_length = 0.0;
	real_t damping = 1.5;
	real_t stiffness = 20.0;
	Vector2 n;
	Vector2 j;
	real_t n_mass = 0.0;
	real_t target_vrn = 0.0;
	real_t v_coef = 0.0;

	bool _setup_pin(real_t p_step);
	bool _pre_solve_pin(real_t p_step);
	void _solve_pin(real_t p_step);

	bool _setup_groove(real_t p_step);
	bool _pre_solve_groove(real_t p_step);
	void _solve_groove(real_t p_step);

	bool _setup_damped_spring(real_t p_step);
	bool _pre_solve_damped_spring(real_t p_step);
	void _solve_damped_spring(real_t p_step);

public:
	_FORCE_INLINE_ PS2DE::JointType get_type() const { return type; }

	_FORCE_INLINE_ void set_max_force(real_t p_force) { max_force = p_force; }
	_FORCE_INLINE_ real_t get_max_force() const { return max_force; }

	_FORCE_INLINE_ void set_bias(real_t p_bias) { bias = p_bias; }
	_FORCE_INLINE_ real_t get_bias() const { return bias; }

	_FORCE_INLINE_ void set_max_bias(real_t p_bias) { max_bias = p_bias; }
	_FORCE_INLINE_ real_t get_max_bias() const { return max_bias; }

	void copy_settings_from(GodotJoint2D *p_joint);

	virtual bool setup(real_t p_step) override;
	virtual bool pre_solve(real_t p_step) override;
	virtual void solve(real_t p_step) override;

	// Pin joint parameters & flags
	void set_param(PS2DE::PinJointParam p_param, real_t p_value);
	real_t get_param(PS2DE::PinJointParam p_param) const;
	void set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool get_flag(PS2DE::PinJointFlag p_flag) const;

	// Damped spring parameters
	void set_param(PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t get_param(PS2DE::DampedSpringParam p_param) const;

	// Empty joint (JOINT_TYPE_MAX)
	GodotJoint2D() :
			GodotConstraint2D(_arr, 0) {}

	// Pin joint (JOINT_TYPE_PIN)
	GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b = nullptr);

	// Groove joint (JOINT_TYPE_GROOVE)
	GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	// Damped spring joint (JOINT_TYPE_DAMPED_SPRING)
	GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b);

	GodotJoint2D(const GodotJoint2D &) = delete;
	GodotJoint2D &operator=(const GodotJoint2D &) = delete;

	virtual ~GodotJoint2D() override {
		for (int i = 0; i < get_body_count(); i++) {
			GodotBody2D *body = get_body_ptr()[i];
			if (body) {
				body->remove_constraint(this, i);
			}
		}
	}
};

// --- End modules/godot_physics_2d/godot_constraints_2d.h ---
#include "godot_shape_2d.h"
// --- Folded from modules/godot_physics_2d/godot_space_2d.h ---
#include "core/math/bvh.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/templates/local_vector.h"
#include "core/typedefs.h"

#include "servers/physics_2d/physics_server_2d.h"

class GodotConstraint2D;

class GodotBroadPhase2D final
{
public:
	typedef uint32_t ID;

	typedef void* (*PairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_userdata);
	typedef void (*UnpairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_userdata);

private:
	template <typename T>
	class UserPairTestFunction
	{
	public:
		static bool user_pair_check(const T* p_a, const T* p_b)
		{
			// return false if no collision, decided by masks etc
			return p_a->interacts_with(p_b);
		}
	};

	template <typename T>
	class UserCullTestFunction
	{
	public:
		static bool user_cull_check(const T* p_a, const T* p_b)
		{
			return true;
		}
	};

	enum Tree
	{
		TREE_STATIC = 0,
		TREE_DYNAMIC = 1,
	};

	enum TreeFlag
	{
		TREE_FLAG_STATIC = 1 << TREE_STATIC,
		TREE_FLAG_DYNAMIC = 1 << TREE_DYNAMIC,
	};

	BVH_Manager<GodotCollisionObject2D, 2, true, 128, UserPairTestFunction<GodotCollisionObject2D>, UserCullTestFunction<GodotCollisionObject2D>, Rect2, Vector2> bvh;

	static void* _pair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int);
	static void _unpair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int, void*);

	PairCallback pair_callback = nullptr;
	void* pair_userdata = nullptr;
	UnpairCallback unpair_callback = nullptr;
	void* unpair_userdata = nullptr;

public:
	// 0 is an invalid ID
	ID create(GodotCollisionObject2D* p_object, int p_subindex = 0, const Rect2& p_aabb = Rect2(), bool p_static = false);
	void move(ID p_id, const Rect2& p_aabb);
	void set_static(ID p_id, bool p_static);
	void remove(ID p_id);

	GodotCollisionObject2D* get_object(ID p_id) const;
	bool is_static(ID p_id) const;
	int get_subindex(ID p_id) const;

	int cull_segment(const Vector2& p_from, const Vector2& p_to, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);
	int cull_aabb(const Rect2& p_aabb, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);

	void set_pair_callback(PairCallback p_pair_callback, void* p_userdata);
	void set_unpair_callback(UnpairCallback p_unpair_callback, void* p_userdata);

	void update();

	GodotBroadPhase2D();
	~GodotBroadPhase2D() = default;
};

class GodotSpace2D final
{
public:
	enum ElapsedTime
	{
		ELAPSED_TIME_INTEGRATE_FORCES,
		ELAPSED_TIME_GENERATE_ISLANDS,
		ELAPSED_TIME_SETUP_CONSTRAINTS,
		ELAPSED_TIME_SOLVE_CONSTRAINTS,
		ELAPSED_TIME_INTEGRATE_VELOCITIES,
		ELAPSED_TIME_MAX
	};

private:
	struct ExcludedShapeSW
	{
		GodotShape2D* local_shape = nullptr;
		const GodotCollisionObject2D* against_object = nullptr;
		int against_shape_index = 0;
	};

	uint64_t elapsed_time[ELAPSED_TIME_MAX] = {};

	PhysicsDirectSpaceState2D* direct_access = nullptr;
	RID self;

	GodotBroadPhase2D* broadphase = nullptr;
	SelfList<GodotBody2D>::List active_list;
	SelfList<GodotBody2D>::List mass_properties_update_list;
	SelfList<GodotBody2D>::List state_query_list;
	SelfList<GodotArea2D>::List monitor_query_list;
	SelfList<GodotArea2D>::List area_moved_list;

	static void* _broadphase_pair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_self);
	static void _broadphase_unpair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_self);

	HashSet<GodotCollisionObject2D*> objects;

	GodotArea2D* area = nullptr;

	int solver_iterations = 0;

	real_t contact_recycle_radius = 0.0;
	real_t contact_max_separation = 0.0;
	real_t contact_max_allowed_penetration = 0.0;
	real_t contact_bias = 0.0;
	real_t constraint_bias = 0.0;

	enum
	{
		INTERSECTION_QUERY_MAX = 2048
	};

	GodotCollisionObject2D* intersection_query_results[INTERSECTION_QUERY_MAX];
	int intersection_query_subindex_results[INTERSECTION_QUERY_MAX];

	real_t body_linear_velocity_sleep_threshold = 0.0;
	real_t body_angular_velocity_sleep_threshold = 0.0;
	real_t body_time_to_sleep = 0.0;

	bool locked = false;

	real_t last_step = 0.001;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	int _cull_aabb_for_body(GodotBody2D* p_body, const Rect2& p_aabb);

	Vector<Vector2> contact_debug;
	int contact_debug_count = 0;

	friend class PhysicsDirectSpaceState2D;

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_default_area(GodotArea2D* p_area) { area = p_area; }

	GodotArea2D* get_default_area() const { return area; }

	const SelfList<GodotBody2D>::List& get_active_body_list() const;
	void body_add_to_active_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_active_list(SelfList<GodotBody2D>* p_body);
	void body_add_to_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void area_add_to_moved_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_moved_list(SelfList<GodotArea2D>* p_area);
	const SelfList<GodotArea2D>::List& get_moved_area_list() const;

	void body_add_to_state_query_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_state_query_list(SelfList<GodotBody2D>* p_body);

	void area_add_to_monitor_query_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_monitor_query_list(SelfList<GodotArea2D>* p_area);

	GodotBroadPhase2D* get_broadphase();

	void add_object(GodotCollisionObject2D* p_object);
	void remove_object(GodotCollisionObject2D* p_object);
	const HashSet<GodotCollisionObject2D*>& get_objects() const;

	_FORCE_INLINE_ int get_solver_iterations() const { return solver_iterations; }

	_FORCE_INLINE_ real_t get_contact_recycle_radius() const { return contact_recycle_radius; }

	_FORCE_INLINE_ real_t get_contact_max_separation() const { return contact_max_separation; }

	_FORCE_INLINE_ real_t get_contact_max_allowed_penetration() const
	{
		return contact_max_allowed_penetration;
	}

	_FORCE_INLINE_ real_t get_contact_bias() const { return contact_bias; }

	_FORCE_INLINE_ real_t get_constraint_bias() const { return constraint_bias; }

	_FORCE_INLINE_ real_t get_body_linear_velocity_sleep_threshold() const
	{
		return body_linear_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_angular_velocity_sleep_threshold() const
	{
		return body_angular_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_time_to_sleep() const { return body_time_to_sleep; }

	void update();
	void setup();
	void call_queries();

	bool is_locked() const;
	void lock();
	void unlock();

	real_t get_last_step() const { return last_step; }

	void set_last_step(real_t p_step) { last_step = p_step; }

	void set_param(PS2DE::SpaceParameter p_param, real_t p_value);
	real_t get_param(PS2DE::SpaceParameter p_param) const;

	void set_island_count(int p_island_count) { island_count = p_island_count; }

	int get_island_count() const { return island_count; }

	void set_active_objects(int p_active_objects) { active_objects = p_active_objects; }

	int get_active_objects() const { return active_objects; }

	int get_collision_pairs() const { return collision_pairs; }

	bool test_body_motion(GodotBody2D* p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result);

	void set_debug_contacts(int p_amount) { contact_debug.resize(p_amount); }

	_FORCE_INLINE_ bool is_debugging_contacts() const { return !contact_debug.is_empty(); }

	_FORCE_INLINE_ void add_debug_contact(const Vector2& p_contact)
	{
		if (contact_debug_count < contact_debug.size()) {
			contact_debug.write[contact_debug_count++] = p_contact;
		}
	}

	_FORCE_INLINE_ Vector<Vector2> get_debug_contacts() { return contact_debug; }

	_FORCE_INLINE_ int get_debug_contact_count() { return contact_debug_count; }

	PhysicsDirectSpaceState2D* get_direct_state();

	void set_elapsed_time(ElapsedTime p_time, uint64_t p_msec) { elapsed_time[p_time] = p_msec; }

	uint64_t get_elapsed_time(ElapsedTime p_time) const { return elapsed_time[p_time]; }

	GodotSpace2D();
	~GodotSpace2D();
};

class GodotStep2D final
{
	uint64_t _step = 1;

	int iterations = 0;
	real_t delta = 0.0;

	LocalVector<LocalVector<GodotBody2D*>> body_islands;
	LocalVector<LocalVector<GodotConstraint2D*>> constraint_islands;
	LocalVector<GodotConstraint2D*> all_constraints;

	void _populate_island(GodotBody2D* p_body, LocalVector<GodotBody2D*>& p_body_island, LocalVector<GodotConstraint2D*>& p_constraint_island);
	void _setup_constraint(uint32_t p_constraint_index, void* p_userdata = nullptr);
	void _pre_solve_island(LocalVector<GodotConstraint2D*>& p_constraint_island) const;
	void _solve_island(uint32_t p_island_index, void* p_userdata = nullptr) const;
	void _check_suspend(LocalVector<GodotBody2D*>& p_body_island) const;

public:
	void step(GodotSpace2D* p_space, real_t p_delta);
	GodotStep2D();
	~GodotStep2D();
};

// --- End modules/godot_physics_2d/godot_space_2d.h ---
#include "servers/physics_2d/physics_server_2d.h"

class GodotPhysicsServer2D final
{
	friend class PhysicsDirectSpaceState2D;
	friend class PhysicsDirectBodyState2D;
	bool active = true;
	bool doing_sync = false;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	bool using_threads = false;

	bool flushing_queries = false;

	GodotStep2D* stepper = nullptr;
	HashSet<GodotSpace2D*> active_spaces;

	mutable RID_PtrOwner<GodotShape2D, true> shape_owner;
	mutable RID_PtrOwner<GodotSpace2D, true> space_owner;
	mutable RID_PtrOwner<GodotArea2D, true> area_owner;
	mutable RID_PtrOwner<GodotBody2D, true> body_owner{65536, 1048576};
	mutable RID_PtrOwner<GodotJoint2D, true> joint_owner;

	friend class GodotCollisionObject2D;
	SelfList<GodotCollisionObject2D>::List pending_shape_update_list;
	void _update_shapes();

	RID _shape_create(PS2DE::ShapeType p_shape);

public:
	struct CollCbkData
	{
		Vector2 valid_dir;
		real_t valid_depth = 0.0;
		int max = 0;
		int amount = 0;
		int passed = 0;
		int invalid_by_dir = 0;
		Vector2* ptr = nullptr;
	};

	RID world_boundary_shape_create();
	RID separation_ray_shape_create();
	RID segment_shape_create();
	RID circle_shape_create();
	RID rectangle_shape_create();
	RID capsule_shape_create();
	RID convex_polygon_shape_create();
	RID concave_polygon_shape_create();

	static void _shape_col_cbk(
		const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata);

	void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);

	PS2DE::ShapeType shape_get_type(RID p_shape) const;
	real_t shape_get_custom_solver_bias(RID p_shape) const;

	bool shape_collide(RID p_shape_A, const Transform2D& p_xform_A,
		const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B,
		const Vector2& p_motion_B, Vector2* r_results, int p_result_max,
		int& r_result_count);

	/* SPACE API */

	RID space_create();
	void space_set_active(RID p_space, bool p_active);
	bool space_is_active(RID p_space) const;

	void space_set_param(
		RID p_space, PS2DE::SpaceParameter p_param, real_t p_value);
	real_t space_get_param(RID p_space, PS2DE::SpaceParameter p_param) const;

	void space_set_debug_contacts(RID p_space, int p_max_contacts);
	Vector<Vector2> space_get_contacts(RID p_space) const;
	int space_get_contact_count(RID p_space) const;

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectSpaceState2D* space_get_direct_state(RID p_space);

	/* AREA API */

	RID area_create();

	void area_set_space(RID p_area, RID p_space);
	RID area_get_space(RID p_area) const;

	void area_add_shape(RID p_area, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	void area_set_shape_transform(
		RID p_area, int p_shape_idx, const Transform2D& p_transform);

	int area_get_shape_count(RID p_area) const;
	RID area_get_shape(RID p_area, int p_shape_idx) const;
	Transform2D area_get_shape_transform(RID p_area, int p_shape_idx) const;

	void area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled);

	void area_remove_shape(RID p_area, int p_shape_idx);
	void area_clear_shapes(RID p_area);

	void area_set_transform(RID p_area, const Transform2D& p_transform);

	Transform2D area_get_transform(RID p_area) const;
	void area_set_monitorable(RID p_area, bool p_monitorable);

	void area_set_collision_layer(RID p_area, uint32_t p_layer);
	uint32_t area_get_collision_layer(RID p_area) const;

	void area_set_collision_mask(RID p_area, uint32_t p_mask);
	uint32_t area_get_collision_mask(RID p_area) const;

	void area_set_pickable(RID p_area, bool p_pickable);

	/* BODY API */

	RID body_create();

	void body_set_space(RID p_body, RID p_space);
	RID body_get_space(RID p_body) const;

	void body_set_mode(RID p_body, PS2DE::BodyMode p_mode);
	PS2DE::BodyMode body_get_mode(RID p_body) const;

	void body_add_shape(RID p_body, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	void body_set_shape_transform(
		RID p_body, int p_shape_idx, const Transform2D& p_transform);

	int body_get_shape_count(RID p_body) const;
	RID body_get_shape(RID p_body, int p_shape_idx) const;
	Transform2D body_get_shape_transform(RID p_body, int p_shape_idx) const;

	void body_remove_shape(RID p_body, int p_shape_idx);
	void body_clear_shapes(RID p_body);

	void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);
	void body_set_shape_as_one_way_collision(RID p_body, int p_shape_idx, bool p_enable,
		real_t p_margin = 0, const Vector2& p_direction = Vector2(0, 1));

	void body_set_continuous_collision_detection_mode(
		RID p_body, PS2DE::CCDMode p_mode);
	PS2DE::CCDMode body_get_continuous_collision_detection_mode(RID p_body) const;

	void body_set_collision_layer(RID p_body, uint32_t p_layer);
	uint32_t body_get_collision_layer(RID p_body) const;

	void body_set_collision_mask(RID p_body, uint32_t p_mask);
	uint32_t body_get_collision_mask(RID p_body) const;

	void body_set_collision_priority(RID p_body, real_t p_priority);
	real_t body_get_collision_priority(RID p_body) const;

	void body_reset_mass_properties(RID p_body);

	void body_apply_central_impulse(RID p_body, const Vector2& p_impulse);
	void body_apply_torque_impulse(RID p_body, real_t p_torque);
	void body_apply_impulse(
		RID p_body, const Vector2& p_impulse, const Vector2& p_position = Vector2());

	void body_apply_central_force(RID p_body, const Vector2& p_force);
	void body_apply_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_apply_torque(RID p_body, real_t p_torque);

	void body_add_constant_central_force(RID p_body, const Vector2& p_force);
	void body_add_constant_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_add_constant_torque(RID p_body, real_t p_torque);

	void body_set_constant_force(RID p_body, const Vector2& p_force);
	Vector2 body_get_constant_force(RID p_body) const;

	void body_set_constant_torque(RID p_body, real_t p_torque);
	real_t body_get_constant_torque(RID p_body) const;

	void body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity);

	void body_add_collision_exception(RID p_body, RID p_body_b);
	void body_remove_collision_exception(RID p_body, RID p_body_b);
	void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);

	void body_set_contacts_reported_depth_threshold(
		RID p_body, real_t p_threshold);
	real_t body_get_contacts_reported_depth_threshold(RID p_body) const;

	void body_set_omit_force_integration(RID p_body, bool p_omit);
	bool body_is_omitting_force_integration(RID p_body) const;

	void body_set_max_contacts_reported(RID p_body, int p_contacts);
	int body_get_max_contacts_reported(RID p_body) const;

	bool body_collide_shape(RID p_body, int p_body_shape, RID p_shape,
		const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results,
		int p_result_max, int& r_result_count);

	void body_set_pickable(RID p_body, bool p_pickable);

	bool _body_test_motion(RID p_body, PhysicsTestMotionParameters2D* rp_parameters,
		const Ref<PhysicsTestMotionResult2D>& p_result = Ref<PhysicsTestMotionResult2D>());
	bool body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result = nullptr);

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectBodyState2D* body_get_direct_state(RID p_body);

	/* JOINT API */

	RID joint_create();

	void joint_clear(RID p_joint);

	void joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value);
	real_t joint_get_param(RID p_joint, PS2DE::JointParam p_param) const;

	void joint_disable_collisions_between_bodies(
		RID p_joint, const bool p_disabled);
	bool joint_is_disabled_collisions_between_bodies(RID p_joint) const;

	void joint_make_pin(
		RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b = RID());
	void joint_make_groove(RID p_joint, const Vector2& p_a_groove1,
		const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b);
	void joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a,
		const Vector2& p_anchor_b, RID p_body_a, RID p_body_b = RID());

	void pin_joint_set_flag(
		RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) const;
	void pin_joint_set_param(
		RID p_joint, PS2DE::PinJointParam p_param, real_t p_value);
	real_t pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) const;
	void damped_spring_joint_set_param(
		RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t damped_spring_joint_get_param(
		RID p_joint, PS2DE::DampedSpringParam p_param) const;

	PS2DE::JointType joint_get_type(RID p_joint) const;

	/* MISC */

	void free_rid(RID p_rid);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	void set_active(bool p_active);
	void init();
	void step(real_t p_step);
	void sync();
	void flush_queries();
	void end_sync();
	void finish();

	bool is_flushing_queries() const { return flushing_queries; }

	int get_process_info(PS2DE::ProcessInfo p_info);

	explicit GodotPhysicsServer2D(bool p_using_threads = false);

	~GodotPhysicsServer2D()
	{
		finish();
	}
};
