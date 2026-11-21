/**************************************************************************/
/*  collision_object_2d.h                                                 */
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

#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/shape_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

// Plain non-virtual composed component for 2D collision state (shared by PhysicsBody2D and Area2D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape2D/CollisionPolygon2D owners.
struct CollisionState2D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform2D local_xform;
		Ref<Shape2D> shape;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform2D xform;
		Node *owner_node = nullptr;

		struct Shape {
			Ref<Shape2D> shape;
			int index = 0;
		};

		Vector<Shape> shapes;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	bool pickable = false;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS2DE::BodyMode body_mode = PS2DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	void init(bool p_area, PS2DE::BodyMode p_mode = PS2DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node2D *p_owner, int p_what);
	void apply_disabled(Node2D *p_owner);
	void apply_enabled(Node2D *p_owner);
	void update_pickable(const Node2D *p_owner);
	void set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node2D *p_owner, DisableMode p_mode);
	void set_pickable(const Node2D *p_owner, bool p_enabled);

	// Inline shape slots (no child CollisionShape2D node required).
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform2D &p_xform);
	Transform2D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape2D / CollisionPolygon2D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform);
	Transform2D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable);
	bool is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin);
	real_t get_shape_owner_one_way_collision_margin(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction);
	Vector2 get_shape_owner_one_way_collision_direction(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape2D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};

// --- Folded from scene/2d/physics/area_2d.h ---
#include "core/templates/vset.h"


class Area2D : public Node2D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState2D::DisableMode;

private:
	CollisionState2D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector2 gravity_vec = Vector2(0, 1);
	real_t gravity = 980.0f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t linear_damp = 0.1f;
	real_t angular_damp = 1.0f;

	int priority = 0;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	void _clear_monitoring();

protected:
	void _notification(int p_what);

public:
	Area2D();
	~Area2D();

	virtual CollisionState2D *get_collision_state_2d() override { return &collision_state; }
	virtual const CollisionState2D *get_collision_state_2d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_pickable(bool p_enabled) { collision_state.set_pickable(this, p_enabled); }
	bool is_pickable() const { return collision_state.pickable; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape2D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector2 &p_center);
	const Vector2 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector2 &p_direction);
	const Vector2 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	bool has_overlapping_bodies() const;
	bool has_overlapping_areas() const;

	bool overlaps_area(Node *rp_area) const;
	bool overlaps_body(Node *rp_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const String &p_audio_bus);
	String get_audio_bus_name() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/2d/physics/area_2d.h ---

// --- Folded from scene/2d/physics/collision_shape_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/shape_2d.h"

class CollisionShape2D : public Node2D {
	Ref<Shape2D> shape;
	Rect2 rect = Rect2(-Point2(10, 10), Point2(20, 20));
	uint32_t owner_id = 0;
	CollisionState2D *collision_state = nullptr;
	bool disabled = false;
	bool one_way_collision = false;
	real_t one_way_collision_margin = 1.0f;
	Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);

	void _shape_changed();
	void _update_in_shape_owner(bool p_xform_only = false);

	Color debug_color = Color(0.0, 0.0, 0.0, 0.0);
	Color _get_default_debug_color() const;

protected:
	void _notification(int p_what);

#ifdef DEBUG_ENABLED
	bool _property_can_revert(const String &p_name) const;
#endif // DEBUG_ENABLED

public:
#ifdef DEBUG_ENABLED
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const override;
#else
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;
#endif // DEBUG_ENABLED

	void set_shape(const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision_enabled() const;

	void set_one_way_collision_margin(real_t p_margin);
	real_t get_one_way_collision_margin() const;

	void set_one_way_collision_direction(const Vector2 &p_direction);
	Vector2 get_one_way_collision_direction() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionShape2D();
};

// --- End scene/2d/physics/collision_shape_2d.h ---

// --- Folded from scene/2d/physics/kinematic_collision_2d.h ---
#include "core/types.h"
#include "servers/physics_2d/physics_server_2d.h"

class PhysicsBody2D;

class KinematicCollision2D : public RefCounted
{
	friend class PhysicsBody2D;
	PS2DT::MotionResult result;

protected:

public:
	Vector2 get_position() const;
	Vector2 get_normal() const;
	Vector2 get_travel() const;
	Vector2 get_remainder() const;
	real_t get_angle(const Vector2& p_up_direction = Vector2(0.0, -1.0)) const;
	real_t get_depth() const;
	RID get_collider_rid() const;
	int get_collider_shape_index() const;
	Vector2 get_collider_velocity() const;
};



// --- End scene/2d/physics/kinematic_collision_2d.h ---

// --- Folded from scene/2d/physics/physics_body_2d.h ---


#include "scene/resources/physics_material.h"
#include "servers/physics_2d/physics_server_2d.h"

class Joint2D;
class Skeleton2D;

class PhysicsBody2D : public Node2D {
public:
	enum BodyMode {
		MODE_STATIC,
		MODE_ANIMATABLE,
		MODE_CHARACTER,
		MODE_RIGID,
		MODE_PHYSICAL_BONE,
	};

	enum FreezeMode {
		FREEZE_MODE_STATIC,
		FREEZE_MODE_KINEMATIC,
	};

	enum CenterOfMassMode {
		CENTER_OF_MASS_MODE_AUTO,
		CENTER_OF_MASS_MODE_CUSTOM,
	};

	enum DampMode {
		DAMP_MODE_COMBINE,
		DAMP_MODE_REPLACE,
	};

	enum CCDMode {
		CCD_MODE_DISABLED,
		CCD_MODE_CAST_RAY,
		CCD_MODE_CAST_SHAPE,
	};

	enum MotionMode {
		MOTION_MODE_GROUNDED,
		MOTION_MODE_FLOATING,
	};

	enum PlatformOnLeave {
		PLATFORM_ON_LEAVE_ADD_VELOCITY,
		PLATFORM_ON_LEAVE_ADD_UPWARD_VELOCITY,
		PLATFORM_ON_LEAVE_DO_NOTHING,
	};

	using DisableMode = CollisionState2D::DisableMode;

	// Plain composed character mover state (for MODE_CHARACTER).
	struct CharacterMover2D {
		real_t margin = 0.08f;
		MotionMode motion_mode = MOTION_MODE_GROUNDED;
		PlatformOnLeave platform_on_leave = PLATFORM_ON_LEAVE_ADD_VELOCITY;

		bool floor_constant_speed = false;
		bool floor_stop_on_slope = true;
		bool floor_block_on_wall = true;
		bool slide_on_ceiling = true;
		int max_slides = 4;
		int platform_layer = 0;
		real_t floor_max_angle = Math::deg_to_rad((real_t)45.0);
		real_t floor_snap_length = 1.0f;
		real_t wall_min_slide_angle = Math::deg_to_rad((real_t)15.0);
		Vector2 up_direction = Vector2(0.0f, -1.0f);
		uint32_t platform_floor_layers = UINT32_MAX;
		uint32_t platform_wall_layers = 0;
		Vector2 velocity;

		Vector2 floor_normal;
		Vector2 platform_velocity;
		Vector2 wall_normal;
		Vector2 last_motion;
		Vector2 previous_position;
		Vector2 real_velocity;

		RID platform_rid;
		bool on_floor = false;
		bool on_ceiling = false;
		bool on_wall = false;

		Vector<PS2DT::MotionResult> motion_results;
		Vector<Ref<KinematicCollision2D>> slide_colliders;
	};

	// Plain composed physical bone attachment state (for MODE_PHYSICAL_BONE).
	struct PhysicalBoneState2D {
		Skeleton2D *parent_skeleton = nullptr;
		int bone2d_index = -1;
		NodePath bone2d_nodepath;
		bool follow_bone_when_simulating = false;
		Joint2D *child_joint = nullptr;
		bool auto_configure_joint = true;
		bool simulate_physics = false;
		bool internal_simulate_physics = false;
	};

private:
	BodyMode mode = MODE_STATIC;
	CollisionState2D collision_state;
	Ref<KinematicCollision2D> motion_cache;

	// Static / Animatable properties.
	Vector2 constant_linear_velocity;
	real_t constant_angular_velocity = 0.0f;
	bool sync_to_physics = true;
	Transform2D last_valid_transform;

	// Rigid / PhysicalBone properties.
	Ref<PhysicsMaterial> physics_material_override;
	bool can_sleep = true;
	bool lock_rotation = false;
	bool freeze = false;
	FreezeMode freeze_mode = FREEZE_MODE_STATIC;

	real_t mass = 1.0f;
	real_t inertia = 0.0f;
	CenterOfMassMode center_of_mass_mode = CENTER_OF_MASS_MODE_AUTO;
	Vector2 center_of_mass;
	real_t gravity_scale = 1.0f;

	DampMode linear_damp_mode = DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = DAMP_MODE_COMBINE;
	real_t linear_damp = 0.0f;
	real_t angular_damp = 0.0f;

	Vector2 linear_velocity;
	real_t angular_velocity = 0.0f;
	bool sleeping = false;
	int max_contacts_reported = 0;
	int contact_count = 0;
	bool custom_integrator = false;
	CCDMode ccd_mode = CCD_MODE_DISABLED;

	CharacterMover2D character;
	PhysicalBoneState2D bone_state;

	void _apply_server_body_mode();
	void _reload_physics_characteristics();
	static void _body_state_changed_callback(void *p_instance, PhysicsDirectBodyState2D *p_state);
	void _body_state_changed(PhysicsDirectBodyState2D *p_state);

	void _position_at_bone2d();
	void _start_bone_physics_simulation();
	void _stop_bone_physics_simulation();

protected:
	void _notification(int p_what);

public:
	PhysicsBody2D(BodyMode p_mode = MODE_STATIC);
	~PhysicsBody2D();

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const { return mode; }

	virtual CollisionState2D *get_collision_state_2d() override { return &collision_state; }
	virtual const CollisionState2D *get_collision_state_2d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	// Collision state forwarding.
	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_collision_priority(real_t p_priority) { collision_state.set_collision_priority(p_priority); }
	real_t get_collision_priority() const { return collision_state.collision_priority; }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_pickable(bool p_enabled) { collision_state.set_pickable(this, p_enabled); }
	bool is_pickable() const { return collision_state.pickable; }

	// Inline shape slots (direct collision shapes without child CollisionShape2D).
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape2D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void set_inline_shape_transform(int p_index, const Transform2D &p_xform) { collision_state.set_inline_shape_transform(p_index, p_xform); }
	Transform2D get_inline_shape_transform(int p_index) const { return collision_state.get_inline_shape_transform(p_index); }
	void set_inline_shape_disabled(int p_index, bool p_disabled) { collision_state.set_inline_shape_disabled(p_index, p_disabled); }
	bool is_inline_shape_disabled(int p_index) const { return collision_state.is_inline_shape_disabled(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	// Common PhysicsBody2D motion & exceptions.
	bool move_and_collide(const PS2DT::MotionParameters &p_parameters, PS2DT::MotionResult &r_result, bool p_test_only = false, bool p_cancel_sliding = true);
	bool test_move(const Transform2D &p_from, const Vector2 &p_motion, const Ref<KinematicCollision2D> &r_collision = Ref<KinematicCollision2D>(), real_t p_margin = 0.08f, bool p_recovery_as_collision = false);
	Vector2 get_gravity() const;
	void add_collision_exception_with(Node *rp_node);
	void remove_collision_exception_with(Node *rp_node);

	// Static / Animatable API.
	void set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override);
	Ref<PhysicsMaterial> get_physics_material_override() const { return physics_material_override; }
	void set_constant_linear_velocity(const Vector2 &p_vel);
	Vector2 get_constant_linear_velocity() const { return constant_linear_velocity; }
	void set_constant_angular_velocity(real_t p_vel);
	real_t get_constant_angular_velocity() const { return constant_angular_velocity; }
	void set_sync_to_physics(bool p_enable);
	bool is_sync_to_physics_enabled() const { return sync_to_physics; }

	// RigidBody API.
	void set_lock_rotation_enabled(bool p_lock_rotation);
	bool is_lock_rotation_enabled() const { return lock_rotation; }
	void set_freeze_enabled(bool p_freeze);
	bool is_freeze_enabled() const { return freeze; }
	void set_freeze_mode(FreezeMode p_freeze_mode);
	FreezeMode get_freeze_mode() const { return freeze_mode; }

	void set_mass(real_t p_mass);
	real_t get_mass() const { return mass; }
	void set_inertia(real_t p_inertia);
	real_t get_inertia() const { return inertia; }
	void set_center_of_mass_mode(CenterOfMassMode p_mode);
	CenterOfMassMode get_center_of_mass_mode() const { return center_of_mass_mode; }
	void set_center_of_mass(const Vector2 &p_center_of_mass);
	const Vector2 &get_center_of_mass() const { return center_of_mass; }

	void set_gravity_scale(real_t p_gravity_scale);
	real_t get_gravity_scale() const { return gravity_scale; }
	void set_linear_damp_mode(DampMode p_mode);
	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode);
	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const { return linear_damp; }
	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const { return angular_damp; }

	void set_linear_velocity(const Vector2 &p_velocity);
	Vector2 get_linear_velocity() const { return linear_velocity; }
	void set_axis_velocity(const Vector2 &p_axis);
	void set_angular_velocity(real_t p_velocity);
	real_t get_angular_velocity() const { return angular_velocity; }

	void set_use_custom_integrator(bool p_enable);
	bool is_using_custom_integrator() const { return custom_integrator; }
	void set_sleeping(bool p_sleeping);
	bool is_sleeping() const { return sleeping; }
	void set_can_sleep(bool p_active);
	bool is_able_to_sleep() const { return can_sleep; }
	void set_max_contacts_reported(int p_amount);
	int get_max_contacts_reported() const { return max_contacts_reported; }
	int get_contact_count() const { return contact_count; }
	void set_continuous_collision_detection_mode(CCDMode p_mode);
	CCDMode get_continuous_collision_detection_mode() const { return ccd_mode; }

	void apply_central_impulse(const Vector2 &p_impulse);
	void apply_impulse(const Vector2 &p_impulse, const Vector2 &p_position = Vector2());
	void apply_torque_impulse(real_t p_torque);
	void apply_central_force(const Vector2 &p_force);
	void apply_force(const Vector2 &p_force, const Vector2 &p_position = Vector2());
	void apply_torque(real_t p_torque);
	void add_constant_central_force(const Vector2 &p_force);
	void add_constant_force(const Vector2 &p_force, const Vector2 &p_position = Vector2());
	void add_constant_torque(real_t p_torque);
	void set_constant_force(const Vector2 &p_force);
	Vector2 get_constant_force() const;
	void set_constant_torque(real_t p_torque);
	real_t get_constant_torque() const;

	// CharacterMover2D API.
	CharacterMover2D &get_character_mover() { return character; }
	const CharacterMover2D &get_character_mover() const { return character; }
	bool move_and_slide();
	void apply_floor_snap();
	const Vector2 &get_velocity() const { return character.velocity; }
	void set_velocity(const Vector2 &p_velocity) { character.velocity = p_velocity; }
	const Vector2 &get_up_direction() const { return character.up_direction; }
	void set_up_direction(const Vector2 &p_up_direction);
	bool is_on_floor() const { return character.on_floor; }
	bool is_on_floor_only() const { return character.on_floor && !character.on_wall && !character.on_ceiling; }
	bool is_on_wall() const { return character.on_wall; }
	bool is_on_wall_only() const { return character.on_wall && !character.on_floor && !character.on_ceiling; }
	bool is_on_ceiling() const { return character.on_ceiling; }
	bool is_on_ceiling_only() const { return character.on_ceiling && !character.on_floor && !character.on_wall; }
	const Vector2 &get_last_motion() const { return character.last_motion; }
	Vector2 get_position_delta() const { return get_global_transform().columns[2] - character.previous_position; }
	const Vector2 &get_floor_normal() const { return character.floor_normal; }
	const Vector2 &get_wall_normal() const { return character.wall_normal; }
	const Vector2 &get_real_velocity() const { return character.real_velocity; }
	real_t get_floor_angle(const Vector2 &p_up_direction = Vector2(0.0f, -1.0f)) const;
	const Vector2 &get_platform_velocity() const { return character.platform_velocity; }
	int get_slide_collision_count() const { return character.motion_results.size(); }
	PS2DT::MotionResult get_slide_collision(int p_bounce) const;
	void set_safe_margin(real_t p_margin) { character.margin = p_margin; }
	real_t get_safe_margin() const { return character.margin; }
	void set_motion_mode(MotionMode p_mode) { character.motion_mode = p_mode; }
	MotionMode get_motion_mode() const { return character.motion_mode; }
	void set_platform_on_leave(PlatformOnLeave p_on_leave) { character.platform_on_leave = p_on_leave; }
	PlatformOnLeave get_platform_on_leave() const { return character.platform_on_leave; }
	void set_max_slides(int p_max_slides);
	int get_max_slides() const { return character.max_slides; }
	void set_floor_max_angle(real_t p_radians) { character.floor_max_angle = p_radians; }
	real_t get_floor_max_angle() const { return character.floor_max_angle; }
	void set_floor_snap_length(real_t p_len);
	real_t get_floor_snap_length() const { return character.floor_snap_length; }
	void set_wall_min_slide_angle(real_t p_radians) { character.wall_min_slide_angle = p_radians; }
	real_t get_wall_min_slide_angle() const { return character.wall_min_slide_angle; }

	// PhysicalBoneState2D API.
	PhysicalBoneState2D &get_physical_bone_state() { return bone_state; }
	const PhysicalBoneState2D &get_physical_bone_state() const { return bone_state; }
	Joint2D *get_joint() const { return bone_state.child_joint; }
	bool get_auto_configure_joint() const { return bone_state.auto_configure_joint; }
	void set_auto_configure_joint(bool p_auto_configure);
	void set_simulate_physics(bool p_simulate);
	bool get_simulate_physics() const { return bone_state.simulate_physics; }
	bool is_simulating_physics() const { return bone_state.internal_simulate_physics; }
	void set_bone2d_nodepath(const NodePath &p_nodepath) { bone_state.bone2d_nodepath = p_nodepath; }
	NodePath get_bone2d_nodepath() const { return bone_state.bone2d_nodepath; }
	void set_bone2d_index(int p_bone_idx) { bone_state.bone2d_index = p_bone_idx; }
	int get_bone2d_index() const { return bone_state.bone2d_index; }
	void set_follow_bone_when_simulating(bool p_follow);
	bool get_follow_bone_when_simulating() const { return bone_state.follow_bone_when_simulating; }

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/2d/physics/physics_body_2d.h ---
