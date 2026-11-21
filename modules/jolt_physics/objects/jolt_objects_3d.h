/**************************************************************************/
/*  jolt_objects_3d.h                                                     */
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

#include "../shapes/jolt_shape_3d.h"

#include "core/math/vector3.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
#include "servers/physics_3d/direct_states/physics_direct_body_state_3d.h"
#include "servers/physics_3d/physics_server_3d_enums.h"

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>

class JoltArea3D;
class JoltBody3D;
class JoltJoint3D;
class JoltObject3D;
class JoltSoftBody3D;
class JoltSpace3D;
class PhysicsServer3DRenderingServerHandler;

class JoltGroupFilter final : public JPH::GroupFilter {
	virtual bool CanCollide(const JPH::CollisionGroup &p_group1, const JPH::CollisionGroup &p_group2) const override;

public:
	inline static JoltGroupFilter *instance = nullptr;

	static void encode_object(const JoltObject3D *p_object, JPH::CollisionGroup::GroupID &r_group_id, JPH::CollisionGroup::SubGroupID &r_sub_group_id);
	static const JoltObject3D *decode_object(JPH::CollisionGroup::GroupID p_group_id, JPH::CollisionGroup::SubGroupID p_sub_group_id);
};

class JoltObject3D {
	friend class JoltShape3D;

public:
	enum ObjectType : char {
		OBJECT_TYPE_INVALID,
		OBJECT_TYPE_BODY,
		OBJECT_TYPE_SOFT_BODY,
		OBJECT_TYPE_AREA,
	};

protected:
	RID rid;
	JoltSpace3D *space = nullptr;
	JPH::Body *jolt_body = nullptr;

	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;

	ObjectType object_type = OBJECT_TYPE_INVALID;

	bool pickable = false;

	// Shaped object state (used by JoltArea3D and JoltBody3D)
	SelfList<JoltObject3D> shapes_changed_element;
	SelfList<JoltObject3D> needs_optimization_element;

	Vector3 scale = Vector3(1, 1, 1);

	JPH::ShapeRefC jolt_shape;
	JPH::ShapeRefC previous_jolt_shape;

	JPH::BodyCreationSettings *jolt_settings = nullptr;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const = 0;
	virtual JPH::ObjectLayer _get_object_layer() const = 0;
	virtual JPH::EMotionType _get_motion_type() const { return JPH::EMotionType::Dynamic; }

	virtual void _add_to_space() = 0;
	virtual void _remove_from_space();

	void _reset_space();

	void _update_object_layer();

	virtual void _collision_layer_changed();
	virtual void _collision_mask_changed();

	bool _is_big() const;

	JPH::ShapeRefC _try_build_shape(bool p_optimize_compound);
	JPH::ShapeRefC _try_build_single_shape();
	JPH::ShapeRefC _try_build_compound_shape(bool p_optimize);

	void _enqueue_shapes_changed();
	void _dequeue_shapes_changed();

	void _enqueue_needs_optimization();
	void _dequeue_needs_optimization();

	virtual void _shapes_changed();
	virtual void _shapes_committed();
	virtual void _space_changing();
	virtual void _space_changed() {}

public:
	explicit JoltObject3D(ObjectType p_object_type);
	virtual ~JoltObject3D() = 0;

	ObjectType get_type() const { return object_type; }

	bool is_body() const { return object_type == OBJECT_TYPE_BODY; }
	bool is_soft_body() const { return object_type == OBJECT_TYPE_SOFT_BODY; }
	bool is_area() const { return object_type == OBJECT_TYPE_AREA; }
	bool is_shaped() const { return object_type != OBJECT_TYPE_SOFT_BODY; }

	JoltBody3D *as_body() { return is_body() ? reinterpret_cast<JoltBody3D *>(this) : nullptr; }
	const JoltBody3D *as_body() const { return is_body() ? reinterpret_cast<const JoltBody3D *>(this) : nullptr; }

	JoltSoftBody3D *as_soft_body() { return is_soft_body() ? reinterpret_cast<JoltSoftBody3D *>(this) : nullptr; }
	const JoltSoftBody3D *as_soft_body() const { return is_soft_body() ? reinterpret_cast<const JoltSoftBody3D *>(this) : nullptr; }

	JoltArea3D *as_area() { return is_area() ? reinterpret_cast<JoltArea3D *>(this) : nullptr; }
	const JoltArea3D *as_area() const { return is_area() ? reinterpret_cast<const JoltArea3D *>(this) : nullptr; }

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	JPH::Body *get_jolt_body() const { return jolt_body; }
	JPH::BodyID get_jolt_id() const { return jolt_body->GetID(); }

	JoltSpace3D *get_space() const { return space; }
	void set_space(JoltSpace3D *p_space);
	bool in_space() const { return space != nullptr && jolt_body != nullptr; }

	uint32_t get_collision_layer() const { return collision_layer; }
	void set_collision_layer(uint32_t p_layer);

	uint32_t get_collision_mask() const { return collision_mask; }
	void set_collision_mask(uint32_t p_mask);

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const = 0;

	bool is_pickable() const { return pickable; }
	void set_pickable(bool p_enabled) { pickable = p_enabled; }

	bool can_collide_with(const JoltObject3D &p_other) const;
	bool can_interact_with(const JoltObject3D &p_other) const;

	virtual bool can_interact_with(const JoltBody3D &p_other) const = 0;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const = 0;
	virtual bool can_interact_with(const JoltArea3D &p_other) const = 0;

	virtual bool reports_contacts() const = 0;

	virtual void pre_step(float p_step) {}

	String to_string() const;

	// Shaped object API
	Transform3D get_transform_unscaled() const;
	Transform3D get_transform_scaled() const;

	Vector3 get_scale() const { return scale; }
	Basis get_basis() const;
	Vector3 get_position() const;

	Vector3 get_center_of_mass() const;
	Vector3 get_center_of_mass_relative() const;
	Vector3 get_center_of_mass_local() const;

	Vector3 get_linear_velocity() const;
	Vector3 get_angular_velocity() const;

	AABB get_aabb() const;

	virtual bool has_custom_center_of_mass() const { return false; }
	virtual Vector3 get_center_of_mass_custom() const { return Vector3(); }

	JPH::ShapeRefC build_shapes(bool p_optimize_compound);

	void commit_shapes(bool p_optimize_compound);

	const JPH::Shape *get_jolt_shape() const { return jolt_shape; }
	const JPH::Shape *get_previous_jolt_shape() const { return previous_jolt_shape; }

	void add_shape(JoltShape3D *p_shape, Transform3D p_transform, bool p_disabled);
	void remove_shape(const JoltShape3D *p_shape);
	void remove_shape(int p_index);

	JoltShape3D *get_shape(int p_index) const;
	void set_shape(int p_index, JoltShape3D *p_shape);

	void clear_shapes();
	void clear_previous_shape();

	int find_shape_index(uint32_t p_shape_instance_id) const;
	int find_shape_index(const JPH::SubShapeID &p_sub_shape_id) const;

	JoltShape3D *find_shape(uint32_t p_shape_instance_id) const;
	JoltShape3D *find_shape(const JPH::SubShapeID &p_sub_shape_id) const;

	Transform3D get_shape_transform_unscaled(int p_index) const;
	Transform3D get_shape_transform_scaled(int p_index) const;
	void set_shape_transform(int p_index, Transform3D p_transform);

	Vector3 get_shape_scale(int p_index) const;

	bool is_shape_disabled(int p_index) const;
	void set_shape_disabled(int p_index, bool p_disabled);
};

class JoltArea3D final : public JoltObject3D {
public:
	typedef PS3DE::AreaSpaceOverrideMode OverrideMode;

private:
	struct BodyIDHasher {
		static uint32_t hash(const JPH::BodyID &p_id) {
			return hash_fmix32(p_id.GetIndexAndSequenceNumber());
		}
	};

	struct ShapeIDPair {
		JPH::SubShapeID other;
		JPH::SubShapeID self;

		ShapeIDPair(JPH::SubShapeID p_other, JPH::SubShapeID p_self) :
				other(p_other), self(p_self) {}

		static uint32_t hash(const ShapeIDPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.other.GetValue());
			hash = hash_murmur3_one_32(p_pair.self.GetValue(), hash);
			return hash_fmix32(hash);
		}

		friend bool operator==(const ShapeIDPair &p_lhs, const ShapeIDPair &p_rhs) {
			return (p_lhs.other == p_rhs.other) && (p_lhs.self == p_rhs.self);
		}
	};

	struct ShapeIndexPair {
		int other = -1;
		int self = -1;

		ShapeIndexPair() = default;
		ShapeIndexPair(int p_other, int p_self) :
				other(p_other), self(p_self) {}

		static uint32_t hash(const ShapeIndexPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.other);
			hash = hash_murmur3_one_32(p_pair.self, hash);
			return hash_fmix32(hash);
		}

		friend bool operator==(const ShapeIndexPair &p_lhs, const ShapeIndexPair &p_rhs) {
			return (p_lhs.other == p_rhs.other) && (p_lhs.self == p_rhs.self);
		}
	};

	SelfList<JoltArea3D> call_queries_element;

	Vector3 gravity_vector = Vector3(0, -1, 0);
	Vector3 wind_source;
	Vector3 wind_direction;

	float priority = 0.0f;
	float gravity = 9.8f;
	float point_gravity_distance = 0.0f;
	float linear_damp = 0.1f;
	float angular_damp = 0.1f;
	float wind_pressure = 0.0f;
	float wind_attenuation_factor = 0.0f;

	OverrideMode gravity_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;
	OverrideMode linear_damp_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;
	OverrideMode angular_damp_mode = PS3DE::AREA_SPACE_OVERRIDE_DISABLED;

	bool monitorable = false;
	bool point_gravity = false;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual JPH::EMotionType _get_motion_type() const override {
		return JPH::EMotionType::Kinematic;
	}

	virtual void _add_to_space() override;

	void _enqueue_call_queries();
	void _dequeue_call_queries();

	void _notify_body_entered(const JPH::BodyID &p_body_id);
	void _notify_body_exited(const JPH::BodyID &p_body_id);
	void _notify_bodies_updated(bool p_priority_changed = false);

	void _remove_all_overlaps();

	void _update_sleeping();
	void _update_group_filter();

	virtual void _space_changing() override;
	virtual void _space_changed() override;
	void _events_changed();
	void _body_monitoring_changed();
	void _area_monitoring_changed();
	void _monitorable_changed();

public:
	JoltArea3D();

	void set_transform(Transform3D p_transform);

	bool is_monitorable() const { return monitorable; }
	void set_monitorable(bool p_monitorable);

	bool can_monitor(const JoltBody3D &p_other) const;
	bool can_monitor(const JoltSoftBody3D &p_other) const;
	bool can_monitor(const JoltArea3D &p_other) const;

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override {
		return Vector3();
	}

	virtual bool reports_contacts() const override { return false; }

	float get_priority() const { return priority; }
	void set_priority(float p_priority);

	float get_gravity() const { return gravity; }
	void set_gravity(float p_gravity);

	bool is_point_gravity() const { return point_gravity; }
	void set_point_gravity(bool p_enabled);

	float get_point_gravity_distance() const { return point_gravity_distance; }
	void set_point_gravity_distance(float p_distance);

	float get_linear_damp() const { return linear_damp; }
	void set_linear_damp(float p_damp);

	float get_angular_damp() const { return angular_damp; }
	void set_angular_damp(float p_damp);

	OverrideMode get_gravity_mode() const { return gravity_mode; }
	void set_gravity_mode(OverrideMode p_mode);

	OverrideMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_linear_damp_mode(OverrideMode p_mode);

	OverrideMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_angular_damp_mode(OverrideMode p_mode);

	Vector3 get_gravity_vector() const { return gravity_vector; }
	void set_gravity_vector(const Vector3 &p_vector);

	float get_wind_pressure() const { return wind_pressure; }
	void set_wind_pressure(float p_wind_pressure) { wind_pressure = p_wind_pressure; }

	float get_wind_attenuation_factor() const { return wind_attenuation_factor; }
	void set_wind_attenuation_factor(float p_wind_attenuation_factor) {
		wind_attenuation_factor = p_wind_attenuation_factor;
	}

	const Vector3 &get_wind_source() const { return wind_source; }
	void set_wind_source(const Vector3 &p_wind_source) { wind_source = p_wind_source; }

	const Vector3 &get_wind_direction() const { return wind_direction; }
	void set_wind_direction(const Vector3 &p_wind_direction) { wind_direction = p_wind_direction; }

	Vector3 compute_gravity(const Vector3 &p_position) const;

	void body_shape_entered(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);
	bool body_shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	void area_shape_entered(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);
	bool area_shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	bool shape_exited(const JPH::BodyID &p_body_id, const JPH::SubShapeID &p_other_shape_id, const JPH::SubShapeID &p_self_shape_id);

	void call_queries();

	template <typename TValue, typename TGetter>
	static bool apply_override(TValue &p_value, PS3DE::AreaSpaceOverrideMode p_mode, TGetter &&p_getter);
};

template <typename TValue, typename TGetter>
inline bool JoltArea3D::apply_override(TValue &p_value, PS3DE::AreaSpaceOverrideMode p_mode, TGetter &&p_getter) {
	switch (p_mode) {
		case PS3DE::AREA_SPACE_OVERRIDE_DISABLED: {
			return false;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_COMBINE: {
			p_value += p_getter();
			return false;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_COMBINE_REPLACE: {
			p_value += p_getter();
			return true;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_REPLACE: {
			p_value = p_getter();
			return true;
		}
		case PS3DE::AREA_SPACE_OVERRIDE_REPLACE_COMBINE: {
			p_value = p_getter();
			return false;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled override mode: '%d'. This should not happen. Please report this.", p_mode));
		}
	}
}

class JoltBody3D final : public JoltObject3D {
public:
	typedef PS3DE::BodyDampMode DampMode;

	struct Contact {
		Vector3 normal;
		Vector3 position;
		Vector3 collider_position;
		Vector3 velocity;
		Vector3 collider_velocity;
		Vector3 impulse;
		RID collider_rid;
		float depth = 0.0f;
		int shape_index = 0;
		int collider_shape_index = 0;
	};

private:
	friend class JoltBodyActivationListener3D;

	SelfList<JoltBody3D> call_queries_element;

	LocalVector<RID> exceptions;
	LocalVector<Contact> contacts;
	LocalVector<JoltArea3D *> areas;
	LocalVector<JoltJoint3D *> joints;

	Transform3D kinematic_transform;

	Vector3 inertia;
	Vector3 center_of_mass_custom;
	Vector3 constant_force;
	Vector3 constant_torque;
	Vector3 linear_surface_velocity;
	Vector3 angular_surface_velocity;
	Vector3 total_gravity;

	PhysicsDirectBodyState3D *direct_state = nullptr;

	PS3DE::BodyMode mode = PS3DE::BODY_MODE_RIGID;

	DampMode linear_damp_mode = PS3DE::BODY_DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = PS3DE::BODY_DAMP_MODE_COMBINE;

	float mass = 1.0f;
	float linear_damp = 0.0f;
	float angular_damp = 0.0f;
	float total_linear_damp = 0.0f;
	float total_angular_damp = 0.0f;
	float collision_priority = 1.0f;

	int contact_count = 0;

	uint32_t locked_axes = 0;

	uint16_t default_area_changed_count = 0;

	bool sleep_allowed = true;
	bool sleep_initially = false;
	bool custom_center_of_mass = false;
	bool custom_integrator = false;
	bool has_point_gravity = false;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual JPH::EMotionType _get_motion_type() const override;

	virtual void _add_to_space() override;

	void _enqueue_call_queries();
	void _dequeue_call_queries();

	void _integrate_forces(float p_step);
	void _move_kinematic(float p_step);

	JPH::EAllowedDOFs _calculate_allowed_dofs() const;

	JPH::MassProperties _calculate_mass_properties(const JPH::Shape &p_shape) const;
	JPH::MassProperties _calculate_mass_properties() const;

	void _on_wake_up();

	void _update_mass_properties();
	bool _needs_update_environmental_properties() const;
	void _update_environmental_properties();
	void _update_kinematic_transform();
	void _update_group_filter();
	void _update_joint_constraints();
	void _update_possible_kinematic_contacts();
	void _update_sleep_allowed();

	void _destroy_joint_constraints();

	void _clear_areas();

	void _mode_changed();
	virtual void _shapes_committed() override;
	virtual void _space_changing() override;
	virtual void _space_changed() override;
	void _areas_changed();
	void _joints_changed();
	void _transform_changed();
	void _motion_changed();
	void _exceptions_changed();
	void _axis_lock_changed();
	void _contact_reporting_changed();
	void _sleep_allowed_changed();

public:
	JoltBody3D();
	virtual ~JoltBody3D() override;

	void set_transform(Transform3D p_transform);

	bool has_custom_integrator() const { return custom_integrator; }
	void set_custom_integrator(bool p_enabled);

	bool is_sleeping() const;
	void set_is_sleeping(bool p_enabled);

	void put_to_sleep() { set_is_sleeping(true); }
	void wake_up() { set_is_sleeping(false); }

	bool is_sleep_allowed() const { return sleep_allowed; }
	bool is_sleep_actually_allowed() const;
	void set_is_sleep_allowed(bool p_enabled);

	Basis get_principal_inertia_axes() const;
	Vector3 get_inverse_inertia() const;
	Basis get_inverse_inertia_tensor() const;

	void set_linear_velocity(const Vector3 &p_velocity);
	void set_angular_velocity(const Vector3 &p_velocity);
	void set_axis_velocity(const Vector3 &p_axis_velocity);

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override;

	virtual bool has_custom_center_of_mass() const override { return custom_center_of_mass; }
	virtual Vector3 get_center_of_mass_custom() const override { return center_of_mass_custom; }
	void set_center_of_mass_custom(const Vector3 &p_center_of_mass);

	int get_max_contacts_reported() const { return contacts.size(); }
	void set_max_contacts_reported(int p_count);

	int get_contact_count() const { return contact_count; }
	const Contact &get_contact(int p_index) { return contacts[p_index]; }

	virtual bool reports_contacts() const override { return !contacts.is_empty(); }
	bool reports_all_kinematic_contacts() const;

	void add_contact(const JoltBody3D *p_collider, float p_depth, int p_shape_index, int p_collider_shape_index, const Vector3 &p_normal, const Vector3 &p_position, const Vector3 &p_collider_position, const Vector3 &p_velocity, const Vector3 &p_collider_velocity, const Vector3 &p_impulse);

	void reset_mass_properties();

	void apply_force(const Vector3 &p_force, const Vector3 &p_position);
	void apply_central_force(const Vector3 &p_force);
	void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position);

	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_torque(const Vector3 &p_torque);
	void apply_torque_impulse(const Vector3 &p_impulse);

	void add_constant_central_force(const Vector3 &p_force);
	void add_constant_force(const Vector3 &p_force, const Vector3 &p_position);
	void add_constant_torque(const Vector3 &p_torque);

	Vector3 get_constant_force() const;
	void set_constant_force(const Vector3 &p_force);

	Vector3 get_constant_torque() const;
	void set_constant_torque(const Vector3 &p_torque);

	Vector3 get_linear_surface_velocity() const { return linear_surface_velocity; }
	Vector3 get_angular_surface_velocity() const { return angular_surface_velocity; }

	void add_collision_exception(const RID &p_excepted_body);
	void remove_collision_exception(const RID &p_excepted_body);
	bool has_collision_exception(const RID &p_excepted_body) const;

	const LocalVector<RID> &get_collision_exceptions() const { return exceptions; }

	void add_area(JoltArea3D *p_area);
	void remove_area(JoltArea3D *p_area);
	void update_area(JoltArea3D *p_area, bool p_priority_changed = false);

	void add_joint(JoltJoint3D *p_joint);
	void remove_joint(JoltJoint3D *p_joint);

	void call_queries();

	virtual void pre_step(float p_step) override;

	PhysicsDirectBodyState3D *get_direct_state();

	PS3DE::BodyMode get_mode() const { return mode; }
	void set_mode(PS3DE::BodyMode p_mode);

	bool is_static() const { return mode == PS3DE::BODY_MODE_STATIC; }
	bool is_kinematic() const { return mode == PS3DE::BODY_MODE_KINEMATIC; }
	bool is_rigid_free() const { return mode == PS3DE::BODY_MODE_RIGID; }
	bool is_rigid_linear() const { return mode == PS3DE::BODY_MODE_RIGID_LINEAR; }
	bool is_rigid() const { return is_rigid_free() || is_rigid_linear(); }

	bool is_ccd_enabled() const;
	void set_ccd_enabled(bool p_enabled);

	float get_mass() const { return mass; }
	void set_mass(float p_mass);

	Vector3 get_inertia() const { return inertia; }
	void set_inertia(const Vector3 &p_inertia);

	float get_bounce() const;
	void set_bounce(float p_bounce);

	float get_friction() const;
	void set_friction(float p_friction);

	float get_gravity_scale() const;
	void set_gravity_scale(float p_scale);

	Vector3 get_total_gravity() const { return total_gravity; }

	float get_linear_damp() const { return linear_damp; }
	void set_linear_damp(float p_damp);

	float get_angular_damp() const { return angular_damp; }
	void set_angular_damp(float p_damp);

	float get_total_linear_damp() const { return total_linear_damp; }
	float get_total_angular_damp() const { return total_angular_damp; }

	float get_collision_priority() const { return collision_priority; }
	void set_collision_priority(float p_priority) { collision_priority = p_priority; }

	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_linear_damp_mode(DampMode p_mode);

	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode);

	bool is_axis_locked(PS3DE::BodyAxis p_axis) const;
	void set_axis_lock(PS3DE::BodyAxis p_axis, bool p_enabled);

	bool are_axes_locked() const { return locked_axes != 0; }

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;
};

class JoltSoftBody3D final : public JoltObject3D {
	RID mesh;

	JPH::SoftBodyCreationSettings *jolt_soft_settings = new JPH::SoftBodyCreationSettings();

	float mass = 1.0f;
	float pressure = 0.0f;
	float linear_damping = 0.01f;
	float stiffness_coefficient = 0.5f;
	float shrinking_factor = 0.0f;

	int simulation_precision = 5;

	virtual JPH::BroadPhaseLayer _get_broad_phase_layer() const override;
	virtual JPH::ObjectLayer _get_object_layer() const override;

	virtual void _space_changing() override;
	virtual void _space_changed() override;

	virtual void _add_to_space() override;

	JPH::SoftBodySharedSettings *_create_shared_settings();

	void _apply_environmental_forces(float p_step);

	void _update_mass();
	void _update_pressure();
	void _update_damping();
	void _update_simulation_precision();
	void _update_group_filter();

	void _try_rebuild();

	void _mesh_changed();
	void _simulation_precision_changed();
	void _mass_changed();
	void _pressure_changed();
	void _damping_changed();
	void _pins_changed();
	void _vertices_changed();
	void _exceptions_changed();
	void _motion_changed();
	void _transform_changed();
	void _areas_changed();

public:
	JoltSoftBody3D();
	virtual ~JoltSoftBody3D() override;

	void add_collision_exception(const RID &p_excepted_body);
	void remove_collision_exception(const RID &p_excepted_body);
	bool has_collision_exception(const RID &p_excepted_body) const;

	void add_area(JoltArea3D *p_area);
	void remove_area(JoltArea3D *p_area);

	virtual bool can_interact_with(const JoltBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltSoftBody3D &p_other) const override;
	virtual bool can_interact_with(const JoltArea3D &p_other) const override;

	virtual bool reports_contacts() const override { return false; }

	virtual Vector3 get_velocity_at_position(const Vector3 &p_position) const override;

	virtual void pre_step(float p_step) override;

	void set_mesh(const RID &p_mesh);

	bool is_sleeping() const;
	void set_is_sleeping(bool p_enabled);

	bool is_sleep_allowed() const;
	void set_is_sleep_allowed(bool p_enabled);

	void put_to_sleep() { set_is_sleeping(true); }
	void wake_up() { set_is_sleeping(false); }

	int get_simulation_precision() const { return simulation_precision; }
	void set_simulation_precision(int p_precision);

	float get_mass() const { return mass; }
	void set_mass(float p_mass);

	float get_stiffness_coefficient() const;
	void set_stiffness_coefficient(float p_coefficient);

	float get_shrinking_factor() const;
	void set_shrinking_factor(float p_shrinking_factor);

	float get_pressure() const { return pressure; }
	void set_pressure(float p_pressure);

	float get_linear_damping() const { return linear_damping; }
	void set_linear_damping(float p_damping);

	float get_drag() const;
	void set_drag(float p_drag);

	Transform3D get_transform() const;
	void set_transform(const Transform3D &p_transform);

	AABB get_bounds() const;

	void update_rendering_server(PhysicsServer3DRenderingServerHandler *p_rendering_server_handler);

	Vector3 get_vertex_position(int p_index);
	void set_vertex_position(int p_index, const Vector3 &p_position);

	void pin_vertex(int p_index);
	void unpin_vertex(int p_index);

	void unpin_all_vertices();

	bool is_vertex_pinned(int p_index) const;

	void apply_vertex_impulse(int p_index, const Vector3 &p_impulse);
	void apply_vertex_force(int p_index, const Vector3 &p_force);
	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_central_force(const Vector3 &p_force);
};
