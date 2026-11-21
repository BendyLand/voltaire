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

#include "physics_body_2d.h"

#include "core/config/engine.h"
#include "scene/2d/physics/joints/joint_2d.h"
#include "scene/2d/skeleton_2d.h"
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

#include "kinematic_collision_2d.h"
#include "scene/2d/physics/physics_body_2d.h"

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
