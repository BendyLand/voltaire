/**************************************************************************/
/*  physics_body_2d.cpp                                                   */
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

#include "scene/2d/physics/collision_object_2d.h"

#include "core/config/engine.h"
// --- Inlined from scene/2d/physics/joints/joint_2d.h ---
#include "scene/2d/node_2d.h"

class PhysicsBody2D;

class Joint2D : public Node2D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_GROOVE,
		JOINT_DAMPED_SPRING,
	};

private:
	JointType joint_type = JOINT_PIN;
	RID joint;
	RID ba, bb;

	NodePath a;
	NodePath b;
	real_t bias = 0.0f;
	bool exclude_from_collision = true;
	bool configured = false;
	String warning;

	// Pin parameters.
	real_t softness = 0.0f;
	real_t angular_limit_lower = 0.0f;
	real_t angular_limit_upper = 0.0f;
	real_t motor_target_velocity = 0.0f;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove & Spring parameters.
	real_t length = 50.0f;
	real_t initial_offset = 25.0f;
	real_t rest_length = 0.0f;
	real_t stiffness = 20.0f;
	real_t damping = 1.0f;

	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b);

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint2D(JointType p_type = JOINT_PIN);
	~Joint2D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const;

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const;

	void set_bias(real_t p_bias);
	real_t get_bias() const;

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const;

	// Pin parameters.
	void set_softness(real_t p_softness);
	real_t get_softness() const { return softness; }
	void set_angular_limit_lower(real_t p_angular_limit_lower);
	real_t get_angular_limit_lower() const { return angular_limit_lower; }
	void set_angular_limit_upper(real_t p_angular_limit_upper);
	real_t get_angular_limit_upper() const { return angular_limit_upper; }
	void set_motor_target_velocity(real_t p_motor_target_velocity);
	real_t get_motor_target_velocity() const { return motor_target_velocity; }
	void set_motor_enabled(bool p_motor_enabled);
	bool is_motor_enabled() const { return motor_enabled; }
	void set_angular_limit_enabled(bool p_angular_limit_enabled);
	bool is_angular_limit_enabled() const { return angular_limit_enabled; }

	// Groove & DampedSpring parameters.
	void set_length(real_t p_length);
	real_t get_length() const { return length; }
	void set_initial_offset(real_t p_initial_offset);
	real_t get_initial_offset() const { return initial_offset; }
	void set_rest_length(real_t p_rest_length);
	real_t get_rest_length() const { return rest_length; }
	void set_stiffness(real_t p_stiffness);
	real_t get_stiffness() const { return stiffness; }
	void set_damping(real_t p_damping);
	real_t get_damping() const { return damping; }

	RID get_rid() const { return joint; }
};

// --- End scene/2d/physics/joints/joint_2d.h ---
#include "scene/2d/sprite_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

PhysicsBody2D::PhysicsBody2D(BodyMode p_mode) :
		mode(p_mode) {
	collision_state.init(false, PS2DE::BODY_MODE_STATIC);
	set_notify_transform(true);
	_apply_server_body_mode();
	if (mode == MODE_PHYSICAL_BONE) {
		PhysicsServer2D::body_set_collision_layer(get_rid(), 0);
		PhysicsServer2D::body_set_collision_mask(get_rid(), 0);
		PhysicsServer2D::body_set_mode(get_rid(), PS2DE::BODY_MODE_STATIC);
	}
}

PhysicsBody2D::~PhysicsBody2D() {
	collision_state.free_rid();
}

void PhysicsBody2D::set_body_mode(BodyMode p_mode) {
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	_apply_server_body_mode();
	update_configuration_warnings();
}

void PhysicsBody2D::_apply_server_body_mode() {
	switch (mode) {
		case MODE_STATIC: {
			collision_state.set_body_mode(this, PS2DE::BODY_MODE_STATIC);
		} break;
		case MODE_ANIMATABLE:
		case MODE_CHARACTER: {
			collision_state.set_body_mode(this, PS2DE::BODY_MODE_KINEMATIC);
		} break;
		case MODE_RIGID:
		case MODE_PHYSICAL_BONE: {
			if (mode == MODE_PHYSICAL_BONE && !bone_state.internal_simulate_physics) {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_STATIC);
				break;
			}
			if (freeze) {
				collision_state.set_body_mode(this, freeze_mode == FREEZE_MODE_STATIC ? PS2DE::BODY_MODE_STATIC : PS2DE::BODY_MODE_KINEMATIC);
			} else if (lock_rotation) {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_RIGID_LINEAR);
			} else {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_RIGID);
			}
		} break;
	}
}

void PhysicsBody2D::_reload_physics_characteristics() {}

void PhysicsBody2D::_body_state_changed_callback(void *p_instance, PhysicsDirectBodyState2D *p_state) {
	PhysicsBody2D *body = static_cast<PhysicsBody2D *>(p_instance);
	body->_body_state_changed(p_state);
}

void PhysicsBody2D::_body_state_changed(PhysicsDirectBodyState2D *p_state) {
	if (mode == MODE_ANIMATABLE) {
		if (!sync_to_physics) {
			return;
		}
		last_valid_transform = p_state->get_transform();
		set_notify_local_transform(false);
		set_global_transform(last_valid_transform);
		set_notify_local_transform(true);
		return;
	}

	linear_velocity = p_state->get_linear_velocity();
	angular_velocity = p_state->get_angular_velocity();
	sleeping = p_state->is_sleeping();
	contact_count = p_state->get_contact_count();

	Transform2D new_xform = p_state->get_transform();
	if (new_xform != get_global_transform()) {
		set_block_transform_notify(true);
		set_global_transform(new_xform);
		set_block_transform_notify(false);
	}
}

void PhysicsBody2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint() && (mode == MODE_RIGID || mode == MODE_PHYSICAL_BONE)) {
				set_notify_local_transform(true);
			}
#endif
		} break;
		case NOTIFICATION_READY: {
			if (mode == MODE_PHYSICAL_BONE) {
				if (bone_state.simulate_physics) {
					_start_bone_physics_simulation();
				} else {
					_stop_bone_physics_simulation();
				}
				set_physics_process_internal(true);
			}
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (mode == MODE_PHYSICAL_BONE) {
				if (bone_state.follow_bone_when_simulating) {
					_position_at_bone2d();
				}
				if (bone_state.child_joint && bone_state.auto_configure_joint) {
					bone_state.child_joint->set_global_position(get_global_position());
				}
			}
		} break;
	}
}

// Common motion & exceptions.

bool PhysicsBody2D::move_and_collide(const PS2DT::MotionParameters &p_parameters, PS2DT::MotionResult &r_result, bool p_test_only, bool p_cancel_sliding) {
	bool colliding = PhysicsServer2D::body_test_motion(get_rid(), p_parameters, &r_result);

	if (p_cancel_sliding) {
		real_t motion_length = p_parameters.motion.length();
		real_t precision = 0.001f;
		if (colliding) {
			precision += motion_length * (r_result.collision_unsafe_fraction - r_result.collision_safe_fraction);
			if (r_result.collision_depth > p_parameters.margin + precision) {
				p_cancel_sliding = false;
			}
		}
		if (p_cancel_sliding) {
			Vector2 motion_normal;
			if (motion_length > CMP_EPSILON) {
				motion_normal = p_parameters.motion / motion_length;
			}
			real_t projected_length = r_result.travel.dot(motion_normal);
			Vector2 recovery = r_result.travel - motion_normal * projected_length;
			if (recovery.length() < p_parameters.margin + precision) {
				r_result.travel = motion_normal * projected_length;
				r_result.remainder = p_parameters.motion - r_result.travel;
			}
		}
	}

	if (!p_test_only) {
		Transform2D gt = get_global_transform();
		gt.columns[2] += r_result.travel;
		set_global_transform(gt);
	}
	return colliding;
}

bool PhysicsBody2D::test_move(const Transform2D &p_from, const Vector2 &p_motion, const Ref<KinematicCollision2D> &r_collision, real_t p_margin, bool p_recovery_as_collision) {
	ERR_FAIL_COND_V(!is_inside_tree(), false);
	PS2DT::MotionResult temp_result;
	PS2DT::MotionResult *r = r_collision.is_valid() ? &r_collision->result : &temp_result;
	PS2DT::MotionParameters parameters;
	parameters.motion = p_motion;
	parameters.margin = p_margin;
	parameters.recovery_as_collision = p_recovery_as_collision;
	return PhysicsServer2D::body_test_motion(get_rid(), parameters, r);
}

Vector2 PhysicsBody2D::get_gravity() const {
	PhysicsDirectBodyState2D *state = PhysicsServer2D::body_get_direct_state(get_rid());
	ERR_FAIL_NULL_V(state, Vector2());
	return state->get_total_gravity();
}

void PhysicsBody2D::add_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody2D *other = dynamic_cast<PhysicsBody2D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody2D.");
	PhysicsServer2D::body_add_collision_exception(get_rid(), other->get_rid());
}

void PhysicsBody2D::remove_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody2D *other = dynamic_cast<PhysicsBody2D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody2D.");
	PhysicsServer2D::body_remove_collision_exception(get_rid(), other->get_rid());
}

// Static / Animatable API.

void PhysicsBody2D::set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override) {
	physics_material_override = p_physics_material_override;
	_reload_physics_characteristics();
}

void PhysicsBody2D::set_constant_linear_velocity(const Vector2 &p_vel) {
	constant_linear_velocity = p_vel;
}

void PhysicsBody2D::set_constant_angular_velocity(real_t p_vel) {
	constant_angular_velocity = p_vel;
}

void PhysicsBody2D::set_sync_to_physics(bool p_enable) {
	sync_to_physics = p_enable;
	collision_state.only_update_transform_changes = p_enable && (mode == MODE_ANIMATABLE);
}

// RigidBody API.

void PhysicsBody2D::set_lock_rotation_enabled(bool p_lock_rotation) {
	if (lock_rotation == p_lock_rotation) {
		return;
	}
	lock_rotation = p_lock_rotation;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_freeze_enabled(bool p_freeze) {
	if (freeze == p_freeze) {
		return;
	}
	freeze = p_freeze;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_freeze_mode(FreezeMode p_freeze_mode) {
	if (freeze_mode == p_freeze_mode) {
		return;
	}
	freeze_mode = p_freeze_mode;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_mass(real_t p_mass) {
	ERR_FAIL_COND(p_mass <= 0);
	mass = p_mass;
}

void PhysicsBody2D::set_inertia(real_t p_inertia) {
	ERR_FAIL_COND(p_inertia < 0);
	inertia = p_inertia;
}

void PhysicsBody2D::set_center_of_mass_mode(CenterOfMassMode p_mode) {
	if (center_of_mass_mode == p_mode) {
		return;
	}
	center_of_mass_mode = p_mode;
	if (center_of_mass_mode == CENTER_OF_MASS_MODE_AUTO) {
		PhysicsServer2D::body_reset_mass_properties(get_rid());
	}
}

void PhysicsBody2D::set_center_of_mass(const Vector2 &p_center_of_mass) {
	center_of_mass = p_center_of_mass;
}

void PhysicsBody2D::set_gravity_scale(real_t p_gravity_scale) {
	gravity_scale = p_gravity_scale;
}

void PhysicsBody2D::set_linear_damp_mode(DampMode p_mode) {
	linear_damp_mode = p_mode;
}

void PhysicsBody2D::set_angular_damp_mode(DampMode p_mode) {
	angular_damp_mode = p_mode;
}

void PhysicsBody2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

void PhysicsBody2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

void PhysicsBody2D::set_linear_velocity(const Vector2 &p_velocity) {
	linear_velocity = p_velocity;
}

void PhysicsBody2D::set_axis_velocity(const Vector2 &p_axis) {
	PhysicsServer2D::body_set_axis_velocity(get_rid(), p_axis);
}

void PhysicsBody2D::set_angular_velocity(real_t p_velocity) {
	angular_velocity = p_velocity;
}

void PhysicsBody2D::set_use_custom_integrator(bool p_enable) {
	if (custom_integrator == p_enable) {
		return;
	}
	custom_integrator = p_enable;
	PhysicsServer2D::body_set_omit_force_integration(get_rid(), p_enable);
}

void PhysicsBody2D::set_sleeping(bool p_sleeping) {
	sleeping = p_sleeping;
}

void PhysicsBody2D::set_can_sleep(bool p_active) {
	can_sleep = p_active;
}

void PhysicsBody2D::set_max_contacts_reported(int p_amount) {
	ERR_FAIL_INDEX_MSG(p_amount, PS2DC::MAX_CONTACTS_REPORTED_2D_MAX, "Max contacts reported is too high.");
	max_contacts_reported = p_amount;
	PhysicsServer2D::body_set_max_contacts_reported(get_rid(), p_amount);
}

void PhysicsBody2D::set_continuous_collision_detection_mode(CCDMode p_mode) {
	ccd_mode = p_mode;
	PhysicsServer2D::body_set_continuous_collision_detection_mode(get_rid(), PS2DE::CCDMode(p_mode));
}

void PhysicsBody2D::apply_central_impulse(const Vector2 &p_impulse) {
	PhysicsServer2D::body_apply_central_impulse(get_rid(), p_impulse);
}

void PhysicsBody2D::apply_impulse(const Vector2 &p_impulse, const Vector2 &p_position) {
	PhysicsServer2D::body_apply_impulse(get_rid(), p_impulse, p_position);
}

void PhysicsBody2D::apply_torque_impulse(real_t p_torque) {
	PhysicsServer2D::body_apply_torque_impulse(get_rid(), p_torque);
}

void PhysicsBody2D::apply_central_force(const Vector2 &p_force) {
	PhysicsServer2D::body_apply_central_force(get_rid(), p_force);
}

void PhysicsBody2D::apply_force(const Vector2 &p_force, const Vector2 &p_position) {
	PhysicsServer2D::body_apply_force(get_rid(), p_force, p_position);
}

void PhysicsBody2D::apply_torque(real_t p_torque) {
	PhysicsServer2D::body_apply_torque(get_rid(), p_torque);
}

void PhysicsBody2D::add_constant_central_force(const Vector2 &p_force) {
	PhysicsServer2D::body_add_constant_central_force(get_rid(), p_force);
}

void PhysicsBody2D::add_constant_force(const Vector2 &p_force, const Vector2 &p_position) {
	PhysicsServer2D::body_add_constant_force(get_rid(), p_force, p_position);
}

void PhysicsBody2D::add_constant_torque(real_t p_torque) {
	PhysicsServer2D::body_add_constant_torque(get_rid(), p_torque);
}

void PhysicsBody2D::set_constant_force(const Vector2 &p_force) {
	PhysicsServer2D::body_set_constant_force(get_rid(), p_force);
}

Vector2 PhysicsBody2D::get_constant_force() const {
	return PhysicsServer2D::body_get_constant_force(get_rid());
}

void PhysicsBody2D::set_constant_torque(real_t p_torque) {
	PhysicsServer2D::body_set_constant_torque(get_rid(), p_torque);
}

real_t PhysicsBody2D::get_constant_torque() const {
	return PhysicsServer2D::body_get_constant_torque(get_rid());
}

// CharacterMover2D API.

bool PhysicsBody2D::move_and_slide() {
	double delta = Engine::is_in_physics_frame() ? get_physics_process_delta_time() : get_process_delta_time();
	if (Math::is_zero_approx(delta)) {
		return false;
	}
	character.previous_position = get_global_transform().columns[2];
	character.motion_results.clear();
	character.on_floor = false;
	character.on_wall = false;
	character.on_ceiling = false;
	character.last_motion = Vector2();

	Vector2 motion = character.velocity * delta;
	for (int slide = 0; slide < character.max_slides && !motion.is_zero_approx(); slide++) {
		PS2DT::MotionParameters params;
		params.motion = motion;
		params.margin = character.margin;
		PS2DT::MotionResult res;
		if (!move_and_collide(params, res, false, false)) {
			character.last_motion = motion;
			break;
		}
		character.motion_results.push_back(res);
		character.last_motion = res.travel;
		if (character.motion_mode == MOTION_MODE_GROUNDED && !character.up_direction.is_zero_approx()) {
			real_t angle = Math::acos(CLAMP(res.collision_normal.dot(character.up_direction), -1.0f, 1.0f));
			if (angle <= character.floor_max_angle + 0.01f) {
				character.on_floor = true;
				character.floor_normal = res.collision_normal;
			} else if (Math::acos(CLAMP(res.collision_normal.dot(-character.up_direction), -1.0f, 1.0f)) <= character.floor_max_angle + 0.01f) {
				character.on_ceiling = true;
			} else {
				character.on_wall = true;
				character.wall_normal = res.collision_normal;
			}
		}
		motion = res.remainder.slide(res.collision_normal);
		character.velocity = character.velocity.slide(res.collision_normal);
	}
	character.real_velocity = (get_global_transform().columns[2] - character.previous_position) / delta;
	return !character.motion_results.is_empty();
}

void PhysicsBody2D::apply_floor_snap() {
	if (character.on_floor || character.up_direction.is_zero_approx()) {
		return;
	}
	real_t len = MAX(character.floor_snap_length, character.margin);
	PS2DT::MotionParameters params;
	params.motion = -character.up_direction * len;
	params.margin = character.margin;
	params.recovery_as_collision = true;
	PS2DT::MotionResult res;
	if (move_and_collide(params, res, true, false)) {
		real_t angle = Math::acos(CLAMP(res.collision_normal.dot(character.up_direction), -1.0f, 1.0f));
		if (angle <= character.floor_max_angle + 0.01f) {
			character.on_floor = true;
			character.floor_normal = res.collision_normal;
			Transform2D gt = get_global_transform();
			gt.columns[2] += character.up_direction * character.up_direction.dot(res.travel);
			set_global_transform(gt);
		}
	}
}

void PhysicsBody2D::set_up_direction(const Vector2 &p_up_direction) {
	ERR_FAIL_COND_MSG(p_up_direction == Vector2(), "up_direction can't be equal to Vector2.ZERO.");
	character.up_direction = p_up_direction.normalized();
}

real_t PhysicsBody2D::get_floor_angle(const Vector2 &p_up_direction) const {
	ERR_FAIL_COND_V(p_up_direction == Vector2(), 0);
	return Math::acos(character.floor_normal.dot(p_up_direction));
}

PS2DT::MotionResult PhysicsBody2D::get_slide_collision(int p_bounce) const {
	ERR_FAIL_INDEX_V(p_bounce, character.motion_results.size(), PS2DT::MotionResult());
	return character.motion_results[p_bounce];
}

void PhysicsBody2D::set_max_slides(int p_max_slides) {
	ERR_FAIL_COND(p_max_slides < 1);
	character.max_slides = p_max_slides;
}

void PhysicsBody2D::set_floor_snap_length(real_t p_len) {
	ERR_FAIL_COND(p_len < 0);
	character.floor_snap_length = p_len;
}

// PhysicalBoneState2D API.

void PhysicsBody2D::_position_at_bone2d() {
	if (bone_state.parent_skeleton) {
		Bone2D *bone_to_use = bone_state.parent_skeleton->get_bone(bone_state.bone2d_index);
		ERR_FAIL_NULL(bone_to_use);
		set_global_transform(bone_to_use->get_global_transform());
	}
}

void PhysicsBody2D::_start_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		return;
	}
	_position_at_bone2d();
	PhysicsServer2D::body_set_collision_layer(get_rid(), get_collision_layer());
	PhysicsServer2D::body_set_collision_mask(get_rid(), get_collision_mask());
	PhysicsServer2D::body_set_collision_priority(get_rid(), get_collision_priority());
	bone_state.internal_simulate_physics = true;
	_apply_server_body_mode();
	set_physics_process_internal(true);
}

void PhysicsBody2D::_stop_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		bone_state.internal_simulate_physics = false;
		_position_at_bone2d();
		set_physics_process_internal(false);
		PhysicsServer2D::body_set_collision_layer(get_rid(), 0);
		PhysicsServer2D::body_set_collision_mask(get_rid(), 0);
		PhysicsServer2D::body_set_collision_priority(get_rid(), 1.0);
		PhysicsServer2D::body_set_mode(get_rid(), PS2DE::BODY_MODE_STATIC);
	}
}

void PhysicsBody2D::set_auto_configure_joint(bool p_auto_configure) {
	bone_state.auto_configure_joint = p_auto_configure;
}

void PhysicsBody2D::set_simulate_physics(bool p_simulate) {
	if (bone_state.simulate_physics == p_simulate) {
		return;
	}
	bone_state.simulate_physics = p_simulate;
	if (bone_state.simulate_physics) {
		_start_bone_physics_simulation();
	} else {
		_stop_bone_physics_simulation();
	}
}

void PhysicsBody2D::set_follow_bone_when_simulating(bool p_follow) {
	bone_state.follow_bone_when_simulating = p_follow;
	if (bone_state.internal_simulate_physics) {
		_stop_bone_physics_simulation();
		_start_bone_physics_simulation();
	}
}

PackedStringArray PhysicsBody2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	if (SceneTree::is_fti_enabled_in_project() && !is_physics_interpolated()) {
		warnings.push_back(RTR("PhysicsBody2D will not work correctly on a non-interpolated branch of the SceneTree."));
	}
	if (mode == MODE_RIGID) {
		Transform2D t = get_transform();
		if (Math::abs(t.columns[0].length() - 1.0) > 0.05 || Math::abs(t.columns[1].length() - 1.0) > 0.05) {
			warnings.push_back(RTR("Size changes to a rigid PhysicsBody2D will be overridden by the physics engine when running."));
		}
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/physics/kinematic_collision_2d.cpp
// ==========================================================================

#include "scene/2d/physics/collision_object_2d.h"

Vector2 KinematicCollision2D::get_position() const { return result.collision_point; }

Vector2 KinematicCollision2D::get_normal() const { return result.collision_normal; }

Vector2 KinematicCollision2D::get_travel() const { return result.travel; }

Vector2 KinematicCollision2D::get_remainder() const { return result.remainder; }

real_t KinematicCollision2D::get_angle(const Vector2& p_up_direction) const
{
	ERR_FAIL_COND_V(p_up_direction == Vector2(), 0);
	return result.get_angle(p_up_direction);
}

real_t KinematicCollision2D::get_depth() const { return result.collision_depth; }

RID KinematicCollision2D::get_collider_rid() const { return result.collider; }

int KinematicCollision2D::get_collider_shape_index() const { return result.collider_shape; }

Vector2 KinematicCollision2D::get_collider_velocity() const { return result.collider_velocity; }

// ============================================================================
// Merged from scene/2d/physics/collision_object_2d.cpp
// ============================================================================

#include "collision_object_2d.h"

#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

void CollisionState2D::init(bool p_area, PS2DE::BodyMode p_mode) {
	area = p_area;
	body_mode = p_mode;
	if (area) {
		rid = PhysicsServer2D::area_create();
	} else {
		rid = PhysicsServer2D::body_create();
		PhysicsServer2D::body_set_mode(rid, body_mode);
	}
}

void CollisionState2D::free_rid() {
	if (rid.is_valid() && PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::free_rid(rid);
	}
	rid = RID();
}

void CollisionState2D::handle_notification(Node2D *p_owner, int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			if (area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
			if (p_owner->is_enabled()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
			update_pickable(p_owner);
		} break;
		case CanvasItem::NOTIFICATION_TRANSFORM_CHANGED: {
			if (!only_update_transform_changes && area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
		} break;
		case CanvasItem::NOTIFICATION_VISIBILITY_CHANGED: {
			update_pickable(p_owner);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			if (callback_lock == 0) {
				if (area) {
					PhysicsServer2D::area_set_space(rid, RID());
				} else {
					PhysicsServer2D::body_set_space(rid, RID());
				}
			}
		} break;
		case Node::NOTIFICATION_DISABLED: {
			apply_disabled(p_owner);
		} break;
		case Node::NOTIFICATION_ENABLED: {
			apply_enabled(p_owner);
		} break;
	}
}

void CollisionState2D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	if (area) {
		PhysicsServer2D::area_set_collision_layer(rid, p_layer);
	} else {
		PhysicsServer2D::body_set_collision_layer(rid, p_layer);
	}
}

void CollisionState2D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	if (area) {
		PhysicsServer2D::area_set_collision_mask(rid, p_mask);
	} else {
		PhysicsServer2D::body_set_collision_mask(rid, p_mask);
	}
}

void CollisionState2D::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer_new = collision_layer;
	if (p_value) {
		layer_new |= 1 << (p_layer_number - 1);
	} else {
		layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer_new);
}

bool CollisionState2D::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_layer & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = collision_mask;
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CollisionState2D::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_mask & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer2D::body_set_collision_priority(rid, p_priority);
	}
}

void CollisionState2D::set_disable_mode(Node2D *p_owner, DisableMode p_mode) {
	if (disable_mode == p_mode) {
		return;
	}
	bool disabled = p_owner->is_inside_tree() && !p_owner->is_enabled();
	if (disabled) {
		apply_enabled(p_owner);
	}
	disable_mode = p_mode;
	if (disabled) {
		apply_disabled(p_owner);
	}
}

void CollisionState2D::apply_disabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				if (callback_lock > 0) {
					ERR_PRINT("Disabling a collision node during a physics callback is not allowed.");
				} else {
					if (area) {
						PhysicsServer2D::area_set_space(rid, RID());
					} else {
						PhysicsServer2D::body_set_space(rid, RID());
					}
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, PS2DE::BODY_MODE_STATIC);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::apply_enabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, body_mode);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode) {
	ERR_FAIL_COND(area);
	if (body_mode == p_mode) {
		return;
	}
	body_mode = p_mode;
	if (p_owner->is_inside_tree() && !p_owner->is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}
	PhysicsServer2D::body_set_mode(rid, p_mode);
}

void CollisionState2D::set_pickable(const Node2D *p_owner, bool p_enabled) {
	if (pickable == p_enabled) {
		return;
	}
	pickable = p_enabled;
	update_pickable(p_owner);
}

void CollisionState2D::update_pickable(const Node2D *p_owner) {
	if (!p_owner->is_inside_tree()) {
		return;
	}
	bool is_pick = pickable && p_owner->is_visible_in_tree();
	if (area) {
		PhysicsServer2D::area_set_pickable(rid, is_pick);
	} else {
		PhysicsServer2D::body_set_pickable(rid, is_pick);
	}
}

// Inline shape slots.

int CollisionState2D::add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform, bool p_disabled) {
	ShapeSlot slot;
	slot.local_xform = p_xform;
	slot.shape = p_shape;
	slot.disabled = p_disabled;
	slot.owner_id = create_shape_owner();
	shape_owner_set_transform(slot.owner_id, p_xform);
	shape_owner_set_disabled(slot.owner_id, p_disabled);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
	int idx = (int)inline_shapes.size();
	inline_shapes.push_back(slot);
	return idx;
}

void CollisionState2D::set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	ShapeSlot &slot = inline_shapes[p_index];
	slot.shape = p_shape;
	shape_owner_clear_shapes(slot.owner_id);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
}

Ref<Shape2D> CollisionState2D::get_inline_shape(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Ref<Shape2D>());
	return inline_shapes[p_index].shape;
}

void CollisionState2D::set_inline_shape_transform(int p_index, const Transform2D &p_xform) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].local_xform = p_xform;
	shape_owner_set_transform(inline_shapes[p_index].owner_id, p_xform);
}

Transform2D CollisionState2D::get_inline_shape_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Transform2D());
	return inline_shapes[p_index].local_xform;
}

void CollisionState2D::set_inline_shape_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].disabled = p_disabled;
	shape_owner_set_disabled(inline_shapes[p_index].owner_id, p_disabled);
}

bool CollisionState2D::is_inline_shape_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), false);
	return inline_shapes[p_index].disabled;
}

void CollisionState2D::remove_inline_shape(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	remove_shape_owner(inline_shapes[p_index].owner_id);
	inline_shapes.remove_at(p_index);
}

void CollisionState2D::clear_inline_shapes() {
	for (uint32_t i = 0; i < inline_shapes.size(); i++) {
		remove_shape_owner(inline_shapes[i].owner_id);
	}
	inline_shapes.clear();
}

// Shape owners.

uint32_t CollisionState2D::create_shape_owner(Node *p_owner) {
	uint32_t id = next_owner_id++;
	ShapeData sd;
	sd.owner_node = p_owner;
	shapes[id] = sd;
	return id;
}

void CollisionState2D::remove_shape_owner(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	shape_owner_clear_shapes(p_owner);
	shapes.erase(p_owner);
}

void CollisionState2D::get_shape_owners(List<uint32_t> *r_owners) const {
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionState2D::get_shape_owners_array() const {
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		ret.push_back(E.key);
	}
	return ret;
}

void CollisionState2D::shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		} else {
			PhysicsServer2D::body_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		}
	}
}

Transform2D CollisionState2D::shape_owner_get_transform(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform2D());
	return shapes[p_owner].xform;
}

void CollisionState2D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.disabled = p_disabled;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		} else {
			PhysicsServer2D::body_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		}
	}
}

bool CollisionState2D::is_shape_owner_disabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].disabled;
}

void CollisionState2D::shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision = p_enable;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

bool CollisionState2D::is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].one_way_collision;
}

void CollisionState2D::shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_margin = p_margin;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

real_t CollisionState2D::get_shape_owner_one_way_collision_margin(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].one_way_collision_margin;
}

void CollisionState2D::shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_direction = p_direction.normalized();
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

Vector2 CollisionState2D::get_shape_owner_one_way_collision_direction(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Vector2());
	return shapes[p_owner].one_way_collision_direction;
}

void CollisionState2D::shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_NULL(rp_shape);
	ShapeData &sd = shapes[p_owner];
	ShapeData::Shape s;
	s.index = total_subshapes;
	s.shape = rp_shape;
	if (area) {
		PhysicsServer2D::area_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
	} else {
		PhysicsServer2D::body_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
		if (sd.one_way_collision) {
			PhysicsServer2D::body_set_shape_as_one_way_collision(rid, s.index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
		}
	}
	sd.shapes.push_back(s);
	total_subshapes++;
}

int CollisionState2D::shape_owner_get_shape_count(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].shapes.size();
}

Ref<Shape2D> CollisionState2D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape2D>());
	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionState2D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);
	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionState2D::shape_owner_remove_shape(uint32_t p_owner, int p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_INDEX(p_shape, shapes[p_owner].shapes.size());

	int index_to_remove = shapes[p_owner].shapes[p_shape].index;
	if (area) {
		PhysicsServer2D::area_remove_shape(rid, index_to_remove);
	} else {
		PhysicsServer2D::body_remove_shape(rid, index_to_remove);
	}

	shapes[p_owner].shapes.remove_at(p_shape);

	for (KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index > index_to_remove) {
				E.value.shapes.write[i].index -= 1;
			}
		}
	}
	total_subshapes--;
}

void CollisionState2D::shape_owner_clear_shapes(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}
}

uint32_t CollisionState2D::shape_find_owner(int p_shape_index) const {
	ERR_FAIL_INDEX_V(p_shape_index, total_subshapes, UINT32_MAX);
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index == p_shape_index) {
				return E.key;
			}
		}
	}
	ERR_FAIL_V_MSG(UINT32_MAX, "Can't find owner for shape index " + itos(p_shape_index) + ".");
}

// ==========================================================================
// Merged from scene/2d/physics/area_2d.cpp
// ==========================================================================

#include "scene/2d/physics/collision_object_2d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

Area2D::Area2D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area2D::~Area2D() {
	collision_state.free_rid();
}

void Area2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	if (p_what == NOTIFICATION_EXIT_TREE || p_what == NOTIFICATION_DISABLED) {
		_clear_monitoring();
	}
}

void Area2D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area2D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area2D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area2D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area2D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area2D::set_gravity_point_center(const Vector2 &p_center) {
	gravity_vec = p_center;
}

const Vector2 &Area2D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area2D::set_gravity_direction(const Vector2 &p_direction) {
	gravity_vec = p_direction;
}

const Vector2 &Area2D::get_gravity_direction() const {
	return gravity_vec;
}

void Area2D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area2D::get_gravity() const {
	return gravity;
}

void Area2D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area2D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area2D::get_linear_damp() const {
	return linear_damp;
}

void Area2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area2D::get_angular_damp() const {
	return angular_damp;
}

void Area2D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area2D::get_priority() const {
	return priority;
}

void Area2D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const {
	return monitoring;
}

void Area2D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer2D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer2D::area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const {
	return monitorable;
}

bool Area2D::has_overlapping_bodies() const {
	return false;
}

bool Area2D::has_overlapping_areas() const {
	return false;
}

bool Area2D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area2D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area2D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area2D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area2D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area2D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area2D::_clear_monitoring() {}

PackedStringArray Area2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/physics/collision_shape_2d.cpp
// ==========================================================================

#include "scene/2d/physics/collision_object_2d.h"

#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"

void CollisionShape2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionShape2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape.ptr());
				}
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

void CollisionShape2D::_shape_changed() {
	queue_redraw();
}

void CollisionShape2D::set_shape(const Ref<Shape2D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape.ptr());
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

Ref<Shape2D> CollisionShape2D::get_shape() const {
	return shape;
}

bool CollisionShape2D::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	if (shape.is_null()) {
		return false;
	}
	return shape->_edit_is_selected_on_click(p_point, p_tolerance);
}

void CollisionShape2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape2D::is_disabled() const {
	return disabled;
}

void CollisionShape2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionShape2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionShape2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionShape2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionShape2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionShape2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

Color CollisionShape2D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape2D::set_debug_color(const Color &p_color) {
	debug_color = p_color;
	queue_redraw();
}

Color CollisionShape2D::get_debug_color() const {
	return debug_color;
}

#ifdef DEBUG_ENABLED
bool CollisionShape2D::_property_can_revert(const String &p_name) const {
	return p_name == "debug_color";
}
#endif // DEBUG_ENABLED

CollisionShape2D::CollisionShape2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionShape2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/collision_polygon_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/physics/collision_polygon_2d.h ---
#include "scene/2d/node_2d.h"

class CollisionPolygon2D : public Node2D {
public:
	enum BuildMode {
		BUILD_SOLIDS,
		BUILD_SEGMENTS,
	};

protected:
	Rect2 aabb = Rect2(-10, -10, 20, 20);
	BuildMode build_mode = BUILD_SOLIDS;
	Vector<Point2> polygon;
	uint32_t owner_id = 0;
	CollisionState2D *collision_state = nullptr;
	bool disabled = false;
	bool one_way_collision = false;
	real_t one_way_collision_margin = 1.0f;
	Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);

	Vector<Vector<Vector2>> _decompose_in_convex();
	void _build_polygon();
	void _update_in_shape_owner(bool p_xform_only = false);
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const override;
#endif

	void set_build_mode(BuildMode p_mode);
	BuildMode get_build_mode() const;

	void set_polygon(const Vector<Point2> &p_polygon);
	Vector<Point2> get_polygon() const;

	PackedStringArray get_configuration_warnings() const override;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision_enabled() const;

	void set_one_way_collision_margin(real_t p_margin);
	real_t get_one_way_collision_margin() const;

	void set_one_way_collision_direction(const Vector2 &p_direction);
	Vector2 get_one_way_collision_direction() const;

	CollisionPolygon2D();
};

// --- End scene/2d/physics/collision_polygon_2d.h ---

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/shape_2d.h"

void CollisionPolygon2D::_build_polygon() {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_clear_shapes(owner_id);

	if (build_mode == BUILD_SOLIDS) {
		if (polygon.size() < 3) {
			return;
		}
		Vector<Vector<Vector2>> decomp = _decompose_in_convex();
		for (int i = 0; i < decomp.size(); i++) {
			Ref<Shape2D> convex = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			convex->set_points(decomp[i]);
			collision_state->shape_owner_add_shape(owner_id, convex.ptr());
		}
	} else {
		if (polygon.size() < 2) {
			return;
		}
		Ref<Shape2D> concave = memnew(Shape2D(Shape2D::SHAPE_CONCAVE_POLYGON));
		Vector<Vector2> segments;
		segments.resize(polygon.size() * 2);
		Vector2 *w = segments.ptrw();
		for (int i = 0; i < polygon.size(); i++) {
			w[(i << 1) + 0] = polygon[i];
			w[(i << 1) + 1] = polygon[(i + 1) % polygon.size()];
		}
		concave->set_segments(segments);
		collision_state->shape_owner_add_shape(owner_id, concave.ptr());
	}
}

Vector<Vector<Vector2>> CollisionPolygon2D::_decompose_in_convex() {
	return Geometry2D::decompose_polygon_in_convex(polygon);
}

void CollisionPolygon2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionPolygon2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				_build_polygon();
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

void CollisionPolygon2D::set_polygon(const Vector<Point2> &p_polygon) {
	polygon = p_polygon;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

Vector<Point2> CollisionPolygon2D::get_polygon() const {
	return polygon;
}

void CollisionPolygon2D::set_build_mode(BuildMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 2);
	build_mode = p_mode;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

CollisionPolygon2D::BuildMode CollisionPolygon2D::get_build_mode() const {
	return build_mode;
}

#ifdef DEBUG_ENABLED
Rect2 CollisionPolygon2D::_edit_get_rect() const {
	return aabb;
}

bool CollisionPolygon2D::_edit_use_rect() const {
	return true;
}

bool CollisionPolygon2D::_edit_is_selected_on_click(const Vector2 &p_point, double p_tolerance) const {
	return false;
}
#endif

void CollisionPolygon2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon2D::is_disabled() const {
	return disabled;
}

void CollisionPolygon2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionPolygon2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionPolygon2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionPolygon2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionPolygon2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionPolygon2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

CollisionPolygon2D::CollisionPolygon2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
}

PackedStringArray CollisionPolygon2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/ray_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/2d/physics/ray_cast_2d.h ---
#include "scene/2d/node_2d.h"

class RayCast2D : public Node2D
{
	bool enabled = true;
	bool collided = false;
	RID against_rid;
	int against_shape = 0;
	Vector2 collision_point;
	Vector2 collision_normal;
	HashSet<RID> exclude;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;

	Vector2 target_position = Vector2(0, 50);

	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	bool hit_from_inside = false;

	void _draw_debug_shape();

protected:
	void _notification(int p_what);
	void _update_raycast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	void set_hit_from_inside(bool p_enable);
	bool is_hit_from_inside_enabled() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_target_position(const Vector2& p_point);
	Vector2 get_target_position() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	void force_raycast_update();

	bool is_colliding() const;
	RID get_collider_rid() const;
	int get_collider_shape() const;
	Vector2 get_collision_point() const;
	Vector2 get_collision_normal() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node2D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node2D* rp_node);
	void clear_exceptions();

	RayCast2D();
};



// --- End scene/2d/physics/ray_cast_2d.h ---
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

Vector2 RayCast2D::get_target_position() const { return target_position; }

void RayCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast2D::get_collision_mask() const { return collision_mask; }

void RayCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool RayCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast2D::is_colliding() const { return collided; }

RID RayCast2D::get_collider_rid() const { return against_rid; }

int RayCast2D::get_collider_shape() const { return against_shape; }

Vector2 RayCast2D::get_collision_point() const { return collision_point; }

Vector2 RayCast2D::get_collision_normal() const { return collision_normal; }

bool RayCast2D::is_enabled() const { return enabled; }

bool RayCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast2D::_draw_debug_shape()
{
	Color draw_col = collided ? Color(1.0, 0.01, 0) : SceneTree::get_debug_collisions_color();
	if (!enabled) {
		const float g = draw_col.get_v();
		draw_col.r = g;
		draw_col.g = g;
		draw_col.b = g;
	}

	// Draw an arrow indicating where the RayCast is pointing to
	const real_t max_arrow_size = 6;
	const real_t line_width = 1.4;
	bool no_line = target_position.length() < line_width;
	real_t arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

	if (no_line) {
		arrow_size = target_position.length();
	}
	else {
		draw_line(Vector2(), target_position - target_position.normalized() * arrow_size, draw_col,
			line_width);
	}

	Transform2D xf;
	xf.rotate(target_position.angle());
	xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

	Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)), xf.xform(Vector2(0, 0.5 * arrow_size)),
		xf.xform(Vector2(0, -0.5 * arrow_size))};

	Vector<Color> cols = {draw_col, draw_col, draw_col};

	draw_primitive(pts, cols, Vector<Vector2>());
}

void RayCast2D::force_raycast_update() { _update_raycast_state(); }

void RayCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void RayCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void RayCast2D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast2D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast2D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast2D::is_hit_from_inside_enabled() const { return hit_from_inside; }

RayCast2D::RayCast2D() { set_hide_clip_children(true); }



void RayCast2D::_update_raycast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/shape_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"
// --- Inlined from scene/2d/physics/shape_cast_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/shape_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

class ShapeCast2D : public Node2D
{
	bool enabled = true;

	Ref<Shape2D> shape;
	RID shape_rid;
	Vector2 target_position = Vector2(0, 50);

	HashSet<RID> exclude;
	real_t margin = 0.0;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;
	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	// Result
	int max_results = 32;
	Vector<PS2DT::ShapeRestInfo> result;
	bool collided = false;
	real_t collision_safe_fraction = 1.0;
	real_t collision_unsafe_fraction = 1.0;

	void _shape_changed();

protected:
	void _notification(int p_what);
	void _update_shapecast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_shape(const Ref<Shape2D>& p_shape);
	Ref<Shape2D> get_shape() const;

	void set_target_position(const Vector2& p_point);
	Vector2 get_target_position() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;

	void set_max_results(int p_max_results);
	int get_max_results() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	void force_shapecast_update();
	bool is_colliding() const;

	int get_collision_count() const;
	RID get_collider_rid(int p_idx) const;
	int get_collider_shape(int p_idx) const;
	Vector2 get_collision_point(int p_idx) const;
	Vector2 get_collision_normal(int p_idx) const;

	real_t get_closest_collision_safe_fraction() const;
	real_t get_closest_collision_unsafe_fraction() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node2D* rp_node);
	void remove_exception(const Node2D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void clear_exceptions();

	PackedStringArray get_configuration_warnings() const override;

	ShapeCast2D();
};



// --- End scene/2d/physics/shape_cast_2d.h ---

Vector2 ShapeCast2D::get_target_position() const { return target_position; }

void ShapeCast2D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast2D::get_margin() const { return margin; }

void ShapeCast2D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast2D::get_max_results() const { return max_results; }

void ShapeCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast2D::get_collision_mask() const { return collision_mask; }

void ShapeCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool ShapeCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast2D::get_collision_count() const { return result.size(); }

bool ShapeCast2D::is_colliding() const { return collided; }

RID ShapeCast2D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast2D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector2 ShapeCast2D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision point found.");
	return result[p_idx].point;
}

Vector2 ShapeCast2D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast2D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast2D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

bool ShapeCast2D::is_enabled() const { return enabled; }

Ref<Shape2D> ShapeCast2D::get_shape() const { return shape; }

bool ShapeCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast2D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void ShapeCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void ShapeCast2D::clear_exceptions() { exclude.clear(); }

void ShapeCast2D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast2D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

PackedStringArray ShapeCast2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (shape.is_null()) {
		warnings.push_back(
			RTR("This node cannot interact with other objects unless a Shape2D is assigned."));
	}
	return warnings;
}

ShapeCast2D::ShapeCast2D() { set_hide_clip_children(true); }



void ShapeCast2D::_update_shapecast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/touch_screen_button.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
// --- Inlined from scene/2d/physics/touch_screen_button.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/shape_2d.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/texture.h"

class TouchScreenButton : public Node2D
{
public:
	enum VisibilityMode
	{
		VISIBILITY_ALWAYS,
		VISIBILITY_TOUCHSCREEN_ONLY
	};

private:
	Ref<Texture2D> texture_normal;
	Ref<Texture2D> texture_pressed;
	Ref<BitMap> bitmask;
	Ref<Shape2D> shape;
	bool shape_centered = true;
	bool shape_visible = true;

	Ref<Shape2D> unit_rect;

	String action;
	bool passby_press = false;
	int finger_pressed = -1;

	VisibilityMode visibility = VISIBILITY_ALWAYS;

	virtual void input(const InputEvent& p_event) override;

	bool _is_point_inside(const Point2& p_point);

	void _press(int p_finger_pressed);
	void _release(bool p_exiting_tree = false);

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_texture_normal(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture_normal() const;

	void set_texture_pressed(const Ref<Texture2D>& p_texture_pressed);
	Ref<Texture2D> get_texture_pressed() const;

	void set_bitmask(const Ref<BitMap>& p_bitmask);
	Ref<BitMap> get_bitmask() const;

	void set_shape(const Ref<Shape2D>& p_shape);
	Ref<Shape2D> get_shape() const;

	void set_shape_centered(bool p_shape_centered);
	bool is_shape_centered() const;

	void set_shape_visible(bool p_shape_visible);
	bool is_shape_visible() const;

	void set_action(const String& p_action);
	String get_action() const;

	void set_passby_press(bool p_enable);
	bool is_passby_press_enabled() const;

	void set_visibility_mode(VisibilityMode p_mode);
	VisibilityMode get_visibility_mode() const;

	bool is_pressed() const;

	virtual Rect2 get_anchorable_rect() const override;

	TouchScreenButton();
};



// --- End scene/2d/physics/touch_screen_button.h ---

Ref<Texture2D> TouchScreenButton::get_texture_normal() const { return texture_normal; }

Ref<Texture2D> TouchScreenButton::get_texture_pressed() const { return texture_pressed; }

void TouchScreenButton::set_bitmask(const Ref<BitMap>& p_bitmask) { bitmask = p_bitmask; }

Ref<BitMap> TouchScreenButton::get_bitmask() const { return bitmask; }

Ref<Shape2D> TouchScreenButton::get_shape() const { return shape; }

bool TouchScreenButton::is_shape_visible() const { return shape_visible; }

bool TouchScreenButton::is_shape_centered() const { return shape_centered; }

bool TouchScreenButton::is_pressed() const { return finger_pressed != -1; }

void TouchScreenButton::set_action(const String& p_action) { action = p_action; }

String TouchScreenButton::get_action() const { return action; }

bool TouchScreenButton::_is_point_inside(const Point2& p_point)
{
	Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(p_point);

	bool touched = false;
	bool check_rect = true;

	if (shape.is_valid()) {
		check_rect = false;

		Vector2 pos;
		if (shape_centered && texture_normal.is_valid()) {
			pos = texture_normal->get_size() * 0.5;
		}

		touched = shape->collide(Transform2D().translated_local(pos), unit_rect.ptr(),
			Transform2D(0, coord + Vector2(0.5, 0.5)));
	}

	if (bitmask.is_valid()) {
		check_rect = false;
		if (!touched && Rect2(Point2(), bitmask->get_size()).has_point(coord)) {
			if (bitmask->get_bitv(coord)) {
				touched = true;
			}
		}
	}

	if (!touched && check_rect) {
		if (texture_normal.is_valid()) {
			touched = Rect2(Size2(), texture_normal->get_size()).has_point(coord);
		}
	}

	return touched;
}

#ifdef DEBUG_ENABLED
Rect2 TouchScreenButton::_edit_get_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::_edit_get_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

bool TouchScreenButton::_edit_use_rect() const { return texture_normal.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 TouchScreenButton::get_anchorable_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::get_anchorable_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

TouchScreenButton::VisibilityMode TouchScreenButton::get_visibility_mode() const
{
	return visibility;
}

void TouchScreenButton::set_passby_press(bool p_enable) { passby_press = p_enable; }

bool TouchScreenButton::is_passby_press_enabled() const { return passby_press; }

TouchScreenButton::TouchScreenButton()
{
	unit_rect = memnew(Shape2D(Shape2D::SHAPE_RECTANGLE));
	unit_rect->set_size(Vector2(1, 1));
}

void TouchScreenButton::input(const InputEvent& p_event) {}

// ============================================================================
// Merged from scene/2d/physics/joints/joint_2d.cpp
// ============================================================================


#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

Joint2D::Joint2D(JointType p_type) :
		joint_type(p_type) {
	joint = PhysicsServer2D::joint_create();
	set_hide_clip_children(true);
}

Joint2D::~Joint2D() {
	ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
	PhysicsServer2D::free_rid(joint);
}

void Joint2D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	joint_type = p_type;
	_update_joint();
	queue_redraw();
}

void Joint2D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

NodePath Joint2D::get_node_a() const {
	return a;
}

void Joint2D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

NodePath Joint2D::get_node_b() const {
	return b;
}

void Joint2D::_disconnect_signals() {}

void Joint2D::_update_joint(bool p_only_free) {
	warning = String();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_remove_collision_exception(ba, bb);
	}
	ba = RID();
	bb = RID();
	configured = false;

	if (p_only_free || !is_inside_tree()) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody2D *body_a = dynamic_cast<PhysicsBody2D *>(node_a);
	PhysicsBody2D *body_b = dynamic_cast<PhysicsBody2D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a || (joint_type != JOINT_PIN && !body_b)) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	_configure_joint(joint, body_a, body_b);
	PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_add_collision_exception(ba, bb);
	}
	configured = true;
	update_configuration_warnings();
}

void Joint2D::_configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b) {
	switch (joint_type) {
		case JOINT_PIN: {
			PhysicsServer2D::joint_make_pin(p_joint, get_global_position(), body_a->get_rid(), body_b ? body_b->get_rid() : RID());
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_SOFTNESS, softness);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_UPPER, angular_limit_upper);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_LOWER, angular_limit_lower);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, motor_target_velocity);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, motor_enabled);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, angular_limit_enabled);
		} break;
		case JOINT_GROOVE: {
			Transform2D gt = get_global_transform();
			Vector2 groove_A1 = gt.get_origin();
			Vector2 groove_A2 = gt.xform(Vector2(0, length));
			Vector2 anchor_B = gt.xform(Vector2(0, initial_offset));
			PhysicsServer2D::joint_make_groove(p_joint, groove_A1, groove_A2, anchor_B, body_a->get_rid(), body_b->get_rid());
		} break;
		case JOINT_DAMPED_SPRING: {
			Transform2D gt = get_global_transform();
			Vector2 anchor_A = gt.get_origin();
			Vector2 anchor_B = gt.xform(Vector2(0, length));
			PhysicsServer2D::joint_make_damped_spring(p_joint, anchor_A, anchor_B, body_a->get_rid(), body_b->get_rid());
			if (rest_length) {
				PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_REST_LENGTH, rest_length);
			}
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_STIFFNESS, stiffness);
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_DAMPING, damping);
		} break;
	}
}

void Joint2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
		case NOTIFICATION_DRAW: {
			if (!is_inside_tree()) {
				break;
			}
			if (!Engine::is_editor_hint() && !SceneTree::is_debugging_collisions_hint()) {
				break;
			}
			if (joint_type == JOINT_PIN) {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, -10), Point2(0, +10), Color(0.7, 0.6, 0.0, 0.5), 3);
			} else {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(-10, length), Point2(+10, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, 0), Point2(0, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				if (joint_type == JOINT_GROOVE) {
					draw_line(Point2(-10, initial_offset), Point2(+10, initial_offset), Color(0.8, 0.8, 0.9, 0.5), 5);
				}
			}
		} break;
	}
}

void Joint2D::set_bias(real_t p_bias) {
	bias = p_bias;
	if (joint.is_valid()) {
		PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);
	}
}

real_t Joint2D::get_bias() const {
	return bias;
}

void Joint2D::set_exclude_nodes_from_collision(bool p_enable) {
	if (exclude_from_collision == p_enable) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	exclude_from_collision = p_enable;
	_update_joint();
}

bool Joint2D::get_exclude_nodes_from_collision() const {
	return exclude_from_collision;
}

void Joint2D::set_softness(real_t p_softness) {
	softness = p_softness;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_SOFTNESS, p_softness);
	}
}

void Joint2D::set_angular_limit_lower(real_t p_angular_limit_lower) {
	angular_limit_lower = p_angular_limit_lower;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_LOWER, p_angular_limit_lower);
	}
}

void Joint2D::set_angular_limit_upper(real_t p_angular_limit_upper) {
	angular_limit_upper = p_angular_limit_upper;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_UPPER, p_angular_limit_upper);
	}
}

void Joint2D::set_motor_target_velocity(real_t p_motor_target_velocity) {
	motor_target_velocity = p_motor_target_velocity;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, p_motor_target_velocity);
	}
}

void Joint2D::set_motor_enabled(bool p_motor_enabled) {
	motor_enabled = p_motor_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, p_motor_enabled);
	}
}

void Joint2D::set_angular_limit_enabled(bool p_angular_limit_enabled) {
	angular_limit_enabled = p_angular_limit_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, p_angular_limit_enabled);
	}
}

void Joint2D::set_length(real_t p_length) {
	length = p_length;
	queue_redraw();
	if (is_configured() && joint_type != JOINT_PIN) {
		_update_joint();
	}
}

void Joint2D::set_initial_offset(real_t p_initial_offset) {
	initial_offset = p_initial_offset;
	queue_redraw();
	if (is_configured() && joint_type == JOINT_GROOVE) {
		_update_joint();
	}
}

void Joint2D::set_rest_length(real_t p_rest_length) {
	rest_length = p_rest_length;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_REST_LENGTH, p_rest_length ? p_rest_length : length);
	}
}

void Joint2D::set_stiffness(real_t p_stiffness) {
	stiffness = p_stiffness;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_STIFFNESS, p_stiffness);
	}
}

void Joint2D::set_damping(real_t p_damping) {
	damping = p_damping;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_DAMPING, p_damping);
	}
}

PackedStringArray Joint2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}

// ============================================================================
// Merged from scene/resources/2d/shape_2d.cpp
// ============================================================================

#include "scene/resources/2d/shape_2d.h"

#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "servers/rendering/renderer.h"

RID Shape2D::_create_shape_rid(ShapeType p_type)
{
	switch (p_type) {
	case SHAPE_WORLD_BOUNDARY:
		return PhysicsServer2D::world_boundary_shape_create();
	case SHAPE_SEPARATION_RAY:
		return PhysicsServer2D::separation_ray_shape_create();
	case SHAPE_SEGMENT:
		return PhysicsServer2D::segment_shape_create();
	case SHAPE_CIRCLE:
		return PhysicsServer2D::circle_shape_create();
	case SHAPE_RECTANGLE:
		return PhysicsServer2D::rectangle_shape_create();
	case SHAPE_CAPSULE:
		return PhysicsServer2D::capsule_shape_create();
	case SHAPE_CONVEX_POLYGON:
		return PhysicsServer2D::convex_polygon_shape_create();
	case SHAPE_CONCAVE_POLYGON:
		return PhysicsServer2D::concave_polygon_shape_create();
	}
	return RID();
}

void Shape2D::_update_shape()
{
	emit_changed();
}

void Shape2D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	if (shape.is_valid() && PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::free_rid(shape);
	}
	shape_type = p_type;
	shape = _create_shape_rid(shape_type);
	if (custom_bias != 0.0) {
		PhysicsServer2D::shape_set_custom_solver_bias(shape, custom_bias);
	}
	_update_shape();
}

RID Shape2D::get_rid() const { return shape; }

// World boundary

void Shape2D::set_normal(const Vector2& p_normal)
{
	ERR_FAIL_COND(p_normal.is_zero_approx());
	if (normal == p_normal) {
		return;
	}
	normal = p_normal;
	_update_shape();
}

Vector2 Shape2D::get_normal() const { return normal; }

void Shape2D::set_distance(real_t p_distance)
{
	if (distance == p_distance) {
		return;
	}
	distance = p_distance;
	_update_shape();
}

real_t Shape2D::get_distance() const { return distance; }

// Separation ray

void Shape2D::set_length(real_t p_length)
{
	if (length == p_length) {
		return;
	}
	length = p_length;
	_update_shape();
}

real_t Shape2D::get_length() const { return length; }

void Shape2D::set_slide_on_slope(bool p_active)
{
	if (slide_on_slope == p_active) {
		return;
	}
	slide_on_slope = p_active;
	_update_shape();
}

bool Shape2D::get_slide_on_slope() const { return slide_on_slope; }

// Segment

void Shape2D::set_a(const Vector2& p_a)
{
	if (a == p_a) {
		return;
	}
	a = p_a;
	_update_shape();
}

Vector2 Shape2D::get_a() const { return a; }

void Shape2D::set_b(const Vector2& p_b)
{
	if (b == p_b) {
		return;
	}
	b = p_b;
	_update_shape();
}

Vector2 Shape2D::get_b() const { return b; }

// Circle / Capsule

Vector<Vector2> Shape2D::_get_capsule_points() const
{
	Vector<Vector2> capsule_points;
	const real_t turn_step = Math::TAU / 24.0;
	for (int i = 0; i < 24; i++) {
		Vector2 ofs =
			Vector2(0, (i > 6 && i <= 18) ? -height * 0.5f + radius : height * 0.5f - radius);

		capsule_points.push_back(
			Vector2(Math::sin(i * turn_step), Math::cos(i * turn_step)) * radius + ofs);
		if (i == 6 || i == 18) {
			capsule_points.push_back(
				Vector2(Math::sin(i * turn_step), Math::cos(i * turn_step)) * radius - ofs);
		}
	}

	return capsule_points;
}

void Shape2D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0f, "Shape2D radius cannot be negative.");
	if (radius == p_radius) {
		return;
	}
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && height < radius * 2.0f) {
		height = radius * 2.0f;
	}
	_update_shape();
}

real_t Shape2D::get_radius() const { return radius; }

void Shape2D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0f, "Shape2D height cannot be negative.");
	if (height == p_height) {
		return;
	}
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
	_update_shape();
}

real_t Shape2D::get_height() const { return height; }

void Shape2D::set_mid_height(real_t p_mid_height)
{
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "Shape2D mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
	_update_shape();
}

real_t Shape2D::get_mid_height() const { return height - radius * 2.0f; }

// Rectangle

void Shape2D::set_size(const Size2& p_size)
{
	ERR_FAIL_COND_MSG(p_size.x < 0 || p_size.y < 0, "Shape2D size cannot be negative.");
	if (size == p_size) {
		return;
	}
	size = p_size;
	_update_shape();
}

Size2 Shape2D::get_size() const { return size; }

// Convex polygon

void Shape2D::set_point_cloud(const Vector<Vector2>& p_points)
{
	Vector<Point2> hull = Geometry2D::convex_hull(p_points);
	ERR_FAIL_COND(hull.size() < 3);
	set_points(hull);
}

void Shape2D::set_points(const Vector<Vector2>& p_points)
{
	points = p_points;
	_update_shape();
}

Vector<Vector2> Shape2D::get_points() const { return points; }

// Concave polygon

void Shape2D::set_segments(const Vector<Vector2>& p_segments)
{
	segments = p_segments;
	_update_shape();
}

Vector<Vector2> Shape2D::get_segments() const { return segments; }

// Solver bias & collision

void Shape2D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer2D::shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape2D::get_custom_solver_bias() const { return custom_bias; }

bool Shape2D::collide_with_motion(const Transform2D& p_local_xform, const Vector2& p_local_motion,
	Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	int r;
	return PhysicsServer2D::shape_collide(get_rid(), p_local_xform, p_local_motion,
		rp_shape->get_rid(), p_shape_xform, p_shape_motion, nullptr, 0, r);
}

bool Shape2D::collide(
	const Transform2D& p_local_xform, Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	int r;
	return PhysicsServer2D::shape_collide(get_rid(), p_local_xform, Vector2(),
		rp_shape->get_rid(), p_shape_xform, Vector2(), nullptr, 0, r);
}

PackedVector2Array Shape2D::collide_with_motion_and_get_contacts(const Transform2D& p_local_xform,
	const Vector2& p_local_motion, Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::shape_collide(get_rid(), p_local_xform, p_local_motion,
			rp_shape->get_rid(), p_shape_xform, p_shape_motion, result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

PackedVector2Array Shape2D::collide_and_get_contacts(const Transform2D& p_local_xform,
	Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::shape_collide(get_rid(), p_local_xform, Vector2(),
			rp_shape->get_rid(), p_shape_xform, Vector2(), result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

// Selection, drawing, bounds

bool Shape2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		const Vector2 shape_center = distance * normal;
		const Vector2 ortho_segment_a = shape_center - normal.orthogonal() * 100;
		const Vector2 ortho_segment_b = shape_center + normal.orthogonal() * 100;
		const Vector2 ortho_closest =
			Geometry2D::get_closest_point_to_segment(p_point, ortho_segment_a, ortho_segment_b);
		if (p_point.distance_to(ortho_closest) < p_tolerance) {
			return true;
		}
		const Vector2 normal_segment_a = shape_center;
		const Vector2 normal_segment_b = shape_center + normal * 30;
		const Vector2 normal_closest =
			Geometry2D::get_closest_point_to_segment(p_point, normal_segment_a, normal_segment_b);
		if (p_point.distance_to(normal_closest) < p_tolerance) {
			return true;
		}
		return false;
	}
	case SHAPE_SEGMENT: {
		Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, a, b);
		return p_point.distance_to(closest) < p_tolerance;
	}
	case SHAPE_CIRCLE: {
		return p_point.length() < radius + p_tolerance;
	}
	case SHAPE_CAPSULE: {
		return Geometry2D::is_point_in_polygon(p_point, _get_capsule_points());
	}
	case SHAPE_CONVEX_POLYGON: {
		return Geometry2D::is_point_in_polygon(p_point, points);
	}
	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0 || (len % 2) == 1) {
			return false;
		}
		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i += 2) {
			Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, r[i], r[i + 1]);
			if (p_point.distance_to(closest) < p_tolerance) {
				return true;
			}
		}
		return false;
	}
	case SHAPE_SEPARATION_RAY:
	case SHAPE_RECTANGLE:
	default:
		return get_rect().has_point(p_point);
	}
}

void Shape2D::draw(const RID& p_to_rid, const Color& p_color)
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Vector2 point = distance * normal;
		real_t line_width = 3.0;

		PackedVector2Array line_points = {point - normal.orthogonal() * 100,
			point - normal.orthogonal() * 60, point + normal.orthogonal() * 60,
			point + normal.orthogonal() * 100};

		Color transparent_color = Color(p_color, 0);
		PackedColorArray line_colors = {transparent_color, p_color, p_color, transparent_color};

		Renderer::canvas_item_add_polyline(p_to_rid, line_points, line_colors, line_width);

		Color arrow_color = p_color.inverted();

		Transform2D xf;
		xf.rotate(normal.angle());

		Vector<Vector2> arrow_points = {
			xf.xform(Vector2(distance + line_width / 2, -2.5)),
			xf.xform(Vector2(distance + 20, -2.5)),
			xf.xform(Vector2(distance + 20, -10)),
			xf.xform(Vector2(distance + 40, 0)),
			xf.xform(Vector2(distance + 20, 10)),
			xf.xform(Vector2(distance + 20, 2.5)),
			xf.xform(Vector2(distance + line_width / 2, 2.5)),
		};

		Renderer::canvas_item_add_polyline(
			p_to_rid, arrow_points, {arrow_color}, line_width / 2);
	} break;

	case SHAPE_SEPARATION_RAY: {
		const Vector2 target_position = Vector2(0, length);

		const float max_arrow_size = 6;
		const float line_width = 1.4;
		bool no_line = target_position.length() < line_width;
		float arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

		if (no_line) {
			arrow_size = target_position.length();
		}
		else {
			Renderer::canvas_item_add_line(p_to_rid, Vector2(),
				target_position - target_position.normalized() * arrow_size, p_color, line_width);
		}

		Transform2D xf;
		xf.rotate(target_position.angle());
		xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

		Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)),
			xf.xform(Vector2(0, 0.5 * arrow_size)),
			xf.xform(Vector2(0, -0.5 * arrow_size))};

		Vector<Color> cols = {p_color, p_color, p_color};

		Renderer::canvas_item_add_primitive(p_to_rid, pts, cols, Vector<Point2>(), RID());
	} break;

	case SHAPE_SEGMENT: {
		Renderer::canvas_item_add_line(p_to_rid, a, b, p_color, 3);
	} break;

	case SHAPE_CIRCLE: {
		Vector<Vector2> circle_points;
		circle_points.resize(24);

		const real_t turn_step = Math::TAU / 24.0;
		for (int i = 0; i < 24; i++) {
			circle_points.write[i] =
				Vector2(Math::cos(i * turn_step), Math::sin(i * turn_step)) * radius;
		}

		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, circle_points, col);

		if (is_collision_outline_enabled()) {
			circle_points.push_back(circle_points[0]);
			col = {Color(p_color, 1.0)};
			Renderer::canvas_item_add_polyline(p_to_rid, circle_points, col);
		}
	} break;

	case SHAPE_RECTANGLE: {
		Renderer::canvas_item_add_rect(
			p_to_rid, Rect2(-size * 0.5, size), p_color);
		if (is_collision_outline_enabled()) {
			Vector<Vector2> stroke_points;
			stroke_points.resize(5);
			stroke_points.write[0] = -size * 0.5;
			stroke_points.write[1] = Vector2(size.x, -size.y) * 0.5;
			stroke_points.write[2] = size * 0.5;
			stroke_points.write[3] = Vector2(-size.x, size.y) * 0.5;
			stroke_points.write[4] = -size * 0.5;

			Vector<Color> stroke_colors = {Color(p_color, 1.0)};

			Renderer::canvas_item_add_polyline(
				p_to_rid, stroke_points, stroke_colors);
		}
	} break;

	case SHAPE_CAPSULE: {
		Vector<Vector2> capsule_points = _get_capsule_points();
		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, capsule_points, col);

		if (is_collision_outline_enabled()) {
			capsule_points.push_back(capsule_points[0]);
			col = {Color(p_color, 1.0f)};
			Renderer::canvas_item_add_polyline(p_to_rid, capsule_points, col);
		}
	} break;

	case SHAPE_CONVEX_POLYGON: {
		if (points.size() < 3) {
			return;
		}

		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, points, col);

		if (is_collision_outline_enabled()) {
			col = {Color(p_color, 1.0)};
			Renderer::canvas_item_add_polyline(p_to_rid, points, col);
			Renderer::canvas_item_add_line(
				p_to_rid, points[points.size() - 1], points[0], Color(p_color, 1.0));
		}
	} break;

	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0 || (len % 2) == 1) {
			return;
		}

		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i += 2) {
			Renderer::canvas_item_add_line(
				p_to_rid, r[i], r[i + 1], p_color, 2);
		}
	} break;
	}
}

Rect2 Shape2D::get_rect() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Vector2 point = distance * normal;
		Vector2 l1[2] = {point - normal.orthogonal() * 100, point + normal.orthogonal() * 100};
		Vector2 l2[2] = {point, point + normal * 30};
		Rect2 rect;
		rect.position = l1[0];
		rect.expand_to(l1[1]);
		rect.expand_to(l2[0]);
		rect.expand_to(l2[1]);
		return rect;
	}
	case SHAPE_SEPARATION_RAY: {
		Rect2 rect;
		rect.position = Vector2();
		rect.expand_to(Vector2(0, length));
		rect = rect.grow(Math::SQRT12 * 4);
		return rect;
	}
	case SHAPE_SEGMENT: {
		Rect2 rect;
		rect.position = a;
		rect.expand_to(b);
		return rect;
	}
	case SHAPE_CIRCLE: {
		Rect2 rect;
		rect.position = -Point2(radius, radius);
		rect.size = Point2(radius, radius) * 2.0;
		return rect;
	}
	case SHAPE_RECTANGLE: {
		return Rect2(-size * 0.5, size);
	}
	case SHAPE_CAPSULE: {
		const Vector2 half_size = Vector2(radius, height * 0.5f);
		return Rect2(-half_size, half_size * 2.0f);
	}
	case SHAPE_CONVEX_POLYGON: {
		Rect2 rect;
		for (int i = 0; i < points.size(); i++) {
			if (i == 0) {
				rect.position = points[i];
			}
			else {
				rect.expand_to(points[i]);
			}
		}
		return rect;
	}
	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0) {
			return Rect2();
		}
		Rect2 rect;
		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i++) {
			if (i == 0) {
				rect.position = r[i];
			}
			else {
				rect.expand_to(r[i]);
			}
		}
		return rect;
	}
	}
	return Rect2();
}

real_t Shape2D::get_enclosing_radius() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
		return distance;
	case SHAPE_SEPARATION_RAY:
		return length;
	case SHAPE_SEGMENT:
		return (a + b).length();
	case SHAPE_CIRCLE:
		return radius;
	case SHAPE_RECTANGLE:
		return size.length() / 2;
	case SHAPE_CAPSULE:
		return height * 0.5f;
	case SHAPE_CONVEX_POLYGON: {
		real_t r = 0.0;
		for (int i = 0; i < points.size(); i++) {
			r = MAX(points[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_CONCAVE_POLYGON: {
		const Vector2* read = segments.ptr();
		real_t r = 0.0;
		for (int i = 0; i < segments.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	}
	return 0.0;
}

bool Shape2D::is_collision_outline_enabled()
{
	return GLOBAL_GET_CACHED(bool, "debug/shapes/collision/draw_2d_outlines");
}

Shape2D::Shape2D(ShapeType p_type) : shape_type(p_type)
{
	shape = _create_shape_rid(shape_type);
	_update_shape();
}

Shape2D::~Shape2D()
{
	ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
	PhysicsServer2D::free_rid(shape);
}
