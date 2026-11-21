/**************************************************************************/
/*  physics_body_3d.cpp                                                   */
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

#include "scene/3d/physics/collision_object_3d.h"

#include "core/config/engine.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_3d/physics_server_3d.h"

namespace {

struct VehicleJacobianEntry {
	Vector3 m_linearJointAxis;
	Vector3 m_aJ;
	Vector3 m_bJ;
	Vector3 m_0MinvJt;
	Vector3 m_1MinvJt;
	real_t m_Adiag = 0.0f;

	VehicleJacobianEntry() = default;

	VehicleJacobianEntry(const Basis &world2A, const Basis &world2B, const Vector3 &rel_pos1,
			const Vector3 &rel_pos2, const Vector3 &jointAxis, const Vector3 &inertiaInvA,
			real_t massInvA, const Vector3 &inertiaInvB, real_t massInvB) :
			m_linearJointAxis(jointAxis) {
		m_aJ = world2A.xform(rel_pos1.cross(m_linearJointAxis));
		m_bJ = world2B.xform(rel_pos2.cross(-m_linearJointAxis));
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = inertiaInvB * m_bJ;
		m_Adiag = massInvA + m_0MinvJt.dot(m_aJ) + massInvB + m_1MinvJt.dot(m_bJ);
	}
};

} // namespace

// ============================================================================
// VehicleWheel3D
// ============================================================================

void VehicleWheel3D::FTIData::update_world_xform(Transform3D &r_xform, real_t p_interpolation_fraction) {
	if (!unset) {
		real_t f = p_interpolation_fraction;

		WheelXform i;
		i.up = prev.up.lerp(curr.up, f).normalized();
		i.right = prev.right.lerp(curr.right, f).normalized();
		Vector3 fwd = i.up.cross(i.right).normalized();

		i.origin = prev.origin.lerp(curr.origin, f);
		i.steering = Math::lerp(prev.steering, curr.steering, f);

		real_t rotation = Math::lerp(curr_rotation - curr_rotation_delta, curr_rotation, f);

		Basis steeringMat(i.up, i.steering);
		Basis rotatingMat(i.right, rotation);
		Basis basis2(i.right[0], i.up[0], fwd[0], i.right[1], i.up[1], fwd[1], i.right[2], i.up[2], fwd[2]);

		r_xform.set_basis(steeringMat * rotatingMat * basis2);
		r_xform.set_origin(i.origin);
	}
}

void VehicleWheel3D::_update(PhysicsDirectBodyState3D *s) {
	if (m_raycastInfo.m_isInContact) {
		real_t project = m_raycastInfo.m_contactNormalWS.dot(m_raycastInfo.m_wheelDirectionWS);
		Vector3 relpos = m_raycastInfo.m_contactPointWS - s->get_transform().origin;
		Vector3 chassis_velocity_at_contact = s->get_linear_velocity() + s->get_angular_velocity().cross(relpos);
		real_t projVel = m_raycastInfo.m_contactNormalWS.dot(chassis_velocity_at_contact);
		if (project >= real_t(-0.1)) {
			m_suspensionRelativeVelocity = 0.0f;
			m_clippedInvContactDotSuspension = 10.0f;
		} else {
			real_t inv = real_t(-1.0) / project;
			m_suspensionRelativeVelocity = projVel * inv;
			m_clippedInvContactDotSuspension = inv;
		}
	} else {
		m_raycastInfo.m_suspensionLength = m_suspensionRestLength;
		m_suspensionRelativeVelocity = 0.0f;
		m_raycastInfo.m_contactNormalWS = -m_raycastInfo.m_wheelDirectionWS;
		m_clippedInvContactDotSuspension = 1.0f;
	}
}

void VehicleWheel3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			PhysicsBody3D *parent_body = dynamic_cast<PhysicsBody3D *>(get_parent());
			if (parent_body && parent_body->get_body_mode() == PhysicsBody3D::MODE_VEHICLE) {
				body = parent_body;
				local_xform = get_transform();
				m_chassisConnectionPointCS = local_xform.origin;
				m_wheelDirectionCS = -local_xform.basis.get_column(Vector3::AXIS_Y).normalized();
				m_wheelAxleCS = local_xform.basis.get_column(Vector3::AXIS_X).normalized();
				body->vehicle.wheels.push_back(this);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (body) {
				body->vehicle.wheels.erase(this);
				body = nullptr;
			}
		} break;
	}
}

Node3D *VehicleWheel3D::get_contact_body() const {
	return m_raycastInfo.m_groundObject;
}

PackedStringArray VehicleWheel3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	const PhysicsBody3D *parent_body = dynamic_cast<const PhysicsBody3D *>(get_parent());
	if (!parent_body || parent_body->get_body_mode() != PhysicsBody3D::MODE_VEHICLE) {
		warnings.push_back(RTR("VehicleWheel3D serves to provide a wheel system to a PhysicsBody3D in MODE_VEHICLE. Please use it as a child of a vehicle PhysicsBody3D."));
	}
	return warnings;
}

// ============================================================================
// PhysicsBody3D
// ============================================================================

PhysicsBody3D::PhysicsBody3D(BodyMode p_mode) :
		mode(p_mode) {
	collision_state.init(false, PS3DE::BODY_MODE_STATIC);
	set_notify_transform(true);
	if (mode == MODE_VEHICLE) {
		mass = 40.0f;
		vehicle.exclude.insert(get_rid());
	} else if (mode == MODE_PHYSICAL_BONE) {
		bone_state.joint = PhysicsServer3D::joint_create();
	}
	_apply_server_body_mode();
}

PhysicsBody3D::~PhysicsBody3D() {
	if (bone_state.joint.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(bone_state.joint);
	}
	collision_state.free_rid();
}

void PhysicsBody3D::set_body_mode(BodyMode p_mode) {
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	if (mode == MODE_VEHICLE) {
		vehicle.exclude.insert(get_rid());
	} else if (mode == MODE_PHYSICAL_BONE && !bone_state.joint.is_valid()) {
		bone_state.joint = PhysicsServer3D::joint_create();
	}
	_apply_server_body_mode();
	update_configuration_warnings();
}

void PhysicsBody3D::_apply_server_body_mode() {
	switch (mode) {
		case MODE_STATIC: {
			collision_state.set_body_mode(this, PS3DE::BODY_MODE_STATIC);
		} break;
		case MODE_ANIMATABLE:
		case MODE_CHARACTER: {
			collision_state.set_body_mode(this, PS3DE::BODY_MODE_KINEMATIC);
		} break;
		case MODE_RIGID:
		case MODE_VEHICLE:
		case MODE_PHYSICAL_BONE: {
			if (mode == MODE_PHYSICAL_BONE && !bone_state.internal_simulate_physics) {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_STATIC);
				break;
			}
			if (freeze) {
				collision_state.set_body_mode(this, freeze_mode == FREEZE_MODE_STATIC ? PS3DE::BODY_MODE_STATIC : PS3DE::BODY_MODE_KINEMATIC);
			} else if (lock_rotation) {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_RIGID_LINEAR);
			} else {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_RIGID);
			}
		} break;
	}
}

void PhysicsBody3D::_update_kinematic_motion() {
#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint()) {
		return;
	}
#endif
	if (mode == MODE_ANIMATABLE && sync_to_physics) {
		collision_state.only_update_transform_changes = true;
		set_notify_local_transform(true);
	} else {
		collision_state.only_update_transform_changes = false;
		set_notify_local_transform(false);
	}
}

void PhysicsBody3D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint() && (mode == MODE_RIGID || mode == MODE_VEHICLE || mode == MODE_PHYSICAL_BONE)) {
				set_notify_local_transform(true);
			}
#endif
			if (mode == MODE_ANIMATABLE) {
				_update_kinematic_motion();
			} else if (mode == MODE_VEHICLE) {
				_update_vehicle_process_mode();
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (mode == MODE_VEHICLE) {
				real_t f = Engine::get_physics_interpolation_fraction();
				Transform3D xform;
				Transform3D inv_vehicle_xform = get_global_transform_interpolated().affine_inverse();
				for (int i = 0; i < vehicle.wheels.size(); i++) {
					VehicleWheel3D &w = *vehicle.wheels[i];
					w.fti_data.update_world_xform(xform, f);
					w.set_transform(inv_vehicle_xform * xform);
				}
			}
		} break;
		case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
			if (mode == MODE_VEHICLE) {
				_update_vehicle_process_mode();
				for (int i = 0; i < vehicle.wheels.size(); i++) {
					vehicle.wheels[i]->fti_data.reset_queued = true;
				}
			}
		} break;
	}
}

void PhysicsBody3D::_physics_interpolated_changed() {
	if (mode == MODE_VEHICLE) {
		_update_vehicle_process_mode();
	}
	Node3D::_physics_interpolated_changed();
}

void PhysicsBody3D::fti_pump_xform() {
	if (mode == MODE_VEHICLE) {
		for (int i = 0; i < vehicle.wheels.size(); i++) {
			vehicle.wheels[i]->fti_data.pump();
		}
	}
	Node3D::fti_pump_xform();
}

void PhysicsBody3D::_body_state_changed(PhysicsDirectBodyState3D *p_state) {
	if (mode == MODE_ANIMATABLE) {
		linear_velocity = p_state->get_linear_velocity();
		angular_velocity = p_state->get_angular_velocity();
		if (!sync_to_physics) {
			return;
		}
		last_valid_transform = p_state->get_transform();
		set_notify_local_transform(false);
		set_global_transform(last_valid_transform);
		set_notify_local_transform(true);
		collision_state.on_transform_changed(this);
		return;
	}

	Transform3D new_transform = p_state->get_transform();
	if (new_transform != get_global_transform()) {
		set_ignore_transform_notification(true);
		set_global_transform(new_transform);
		set_ignore_transform_notification(false);
	}

	linear_velocity = p_state->get_linear_velocity();
	angular_velocity = p_state->get_angular_velocity();
	inverse_inertia_tensor = p_state->get_inverse_inertia_tensor();
	contact_count = p_state->get_contact_count();
	sleeping = p_state->is_sleeping();

	if (mode == MODE_VEHICLE) {
		_step_vehicle(p_state);
	}
}

// Common motion, locks & exceptions.

bool PhysicsBody3D::move_and_collide(const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult &r_result, bool p_test_only, bool p_cancel_sliding) {
	bool colliding = PhysicsServer3D::body_test_motion(get_rid(), p_parameters, &r_result);

	if (p_cancel_sliding) {
		real_t motion_length = p_parameters.motion.length();
		real_t precision = 0.001f;

		if (colliding) {
			precision += motion_length * (r_result.collision_unsafe_fraction - r_result.collision_safe_fraction);
			if (r_result.collisions[0].depth > p_parameters.margin + precision) {
				p_cancel_sliding = false;
			}
		}

		if (p_cancel_sliding) {
			Vector3 motion_normal;
			if (motion_length > CMP_EPSILON) {
				motion_normal = p_parameters.motion / motion_length;
			}

			real_t projected_length = r_result.travel.dot(motion_normal);
			Vector3 recovery = r_result.travel - motion_normal * projected_length;
			if (recovery.length() < p_parameters.margin + precision) {
				r_result.travel = motion_normal * projected_length;
				r_result.remainder = p_parameters.motion - r_result.travel;
			}
		}
	}

	for (int i = 0; i < 3; i++) {
		if (locked_axis & (1 << i)) {
			r_result.travel[i] = 0;
		}
	}

	if (!p_test_only) {
		Transform3D gt = p_parameters.from;
		gt.origin += r_result.travel;
		set_global_transform(gt);
	}

	return colliding;
}

bool PhysicsBody3D::test_move(const Transform3D &p_from, const Vector3 &p_motion, const Ref<KinematicCollision3D> &r_collision, real_t p_margin, bool p_recovery_as_collision, int p_max_collisions) {
	ERR_FAIL_COND_V(!is_inside_tree(), false);

	PS3DT::MotionResult *r = nullptr;
	PS3DT::MotionResult temp_result;
	if (r_collision.is_valid()) {
		r = &r_collision->result;
	} else {
		r = &temp_result;
	}

	PS3DT::MotionParameters parameters(p_from, p_motion, p_margin);
	parameters.recovery_as_collision = p_recovery_as_collision;
	parameters.max_collisions = p_max_collisions;

	return PhysicsServer3D::body_test_motion(get_rid(), parameters, r);
}

Vector3 PhysicsBody3D::get_gravity() const {
	PhysicsDirectBodyState3D *state = PhysicsServer3D::body_get_direct_state(get_rid());
	ERR_FAIL_NULL_V(state, Vector3());
	return state->get_total_gravity();
}

void PhysicsBody3D::set_axis_lock(PS3DE::BodyAxis p_axis, bool p_lock) {
	if (p_lock) {
		locked_axis |= p_axis;
	} else {
		locked_axis &= (~p_axis);
	}
	PhysicsServer3D::body_set_axis_lock(get_rid(), p_axis, p_lock);
}

bool PhysicsBody3D::get_axis_lock(PS3DE::BodyAxis p_axis) const {
	return (locked_axis & p_axis) != 0;
}

void PhysicsBody3D::add_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody3D *other = dynamic_cast<PhysicsBody3D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody3D.");
	PhysicsServer3D::body_add_collision_exception(get_rid(), other->get_rid());
}

void PhysicsBody3D::remove_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody3D *other = dynamic_cast<PhysicsBody3D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody3D.");
	PhysicsServer3D::body_remove_collision_exception(get_rid(), other->get_rid());
}

// Static / Animatable / Rigid API.

void PhysicsBody3D::set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override) {
	physics_material_override = p_physics_material_override;
}

void PhysicsBody3D::set_sync_to_physics(bool p_enable) {
	if (sync_to_physics == p_enable) {
		return;
	}
	sync_to_physics = p_enable;
	if (is_inside_tree()) {
		_update_kinematic_motion();
	}
}

void PhysicsBody3D::set_lock_rotation_enabled(bool p_lock_rotation) {
	if (lock_rotation == p_lock_rotation) {
		return;
	}
	lock_rotation = p_lock_rotation;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_freeze_enabled(bool p_freeze) {
	if (freeze == p_freeze) {
		return;
	}
	freeze = p_freeze;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_freeze_mode(FreezeMode p_freeze_mode) {
	if (freeze_mode == p_freeze_mode) {
		return;
	}
	freeze_mode = p_freeze_mode;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_mass(real_t p_mass) {
	ERR_FAIL_COND(p_mass <= 0);
	mass = p_mass;
}

real_t PhysicsBody3D::get_inverse_mass() const {
	if (mode == MODE_RIGID || mode == MODE_VEHICLE || mode == MODE_PHYSICAL_BONE) {
		return mass > 0.0f ? (1.0f / mass) : 0.0f;
	}
	return 0.0f;
}

Vector3 PhysicsBody3D::get_linear_velocity() const {
	if (mode == MODE_CHARACTER) {
		return character.real_velocity;
	}
	if (mode == MODE_STATIC) {
		return constant_linear_velocity;
	}
	return linear_velocity;
}

void PhysicsBody3D::set_axis_velocity(const Vector3 &p_axis) {
	PhysicsServer3D::body_set_axis_velocity(get_rid(), p_axis);
}

Vector3 PhysicsBody3D::get_angular_velocity() const {
	if (mode == MODE_STATIC) {
		return constant_angular_velocity;
	}
	return angular_velocity;
}

void PhysicsBody3D::set_use_custom_integrator(bool p_enable) {
	if (custom_integrator == p_enable) {
		return;
	}
	custom_integrator = p_enable;
	PhysicsServer3D::body_set_omit_force_integration(get_rid(), p_enable);
}

void PhysicsBody3D::set_max_contacts_reported(int p_amount) {
	ERR_FAIL_INDEX_MSG(p_amount, PS3DC::MAX_CONTACTS_REPORTED_3D_MAX, "Max contacts reported allocates memory and must not be set too high.");
	max_contacts_reported = p_amount;
	PhysicsServer3D::body_set_max_contacts_reported(get_rid(), p_amount);
}

void PhysicsBody3D::set_use_continuous_collision_detection(bool p_enable) {
	ccd = p_enable;
	PhysicsServer3D::body_set_enable_continuous_collision_detection(get_rid(), p_enable);
}

void PhysicsBody3D::apply_central_impulse(const Vector3 &p_impulse) {
	PhysicsServer3D::body_apply_central_impulse(get_rid(), p_impulse);
}

void PhysicsBody3D::apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position) {
	PhysicsServer3D::body_apply_impulse(get_rid(), p_impulse, p_position);
}

void PhysicsBody3D::apply_torque_impulse(const Vector3 &p_impulse) {
	PhysicsServer3D::body_apply_torque_impulse(get_rid(), p_impulse);
}

void PhysicsBody3D::apply_central_force(const Vector3 &p_force) {
	PhysicsServer3D::body_apply_central_force(get_rid(), p_force);
}

void PhysicsBody3D::apply_force(const Vector3 &p_force, const Vector3 &p_position) {
	PhysicsServer3D::body_apply_force(get_rid(), p_force, p_position);
}

void PhysicsBody3D::apply_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_apply_torque(get_rid(), p_torque);
}

void PhysicsBody3D::add_constant_central_force(const Vector3 &p_force) {
	PhysicsServer3D::body_add_constant_central_force(get_rid(), p_force);
}

void PhysicsBody3D::add_constant_force(const Vector3 &p_force, const Vector3 &p_position) {
	PhysicsServer3D::body_add_constant_force(get_rid(), p_force, p_position);
}

void PhysicsBody3D::add_constant_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_add_constant_torque(get_rid(), p_torque);
}

void PhysicsBody3D::set_constant_force(const Vector3 &p_force) {
	PhysicsServer3D::body_set_constant_force(get_rid(), p_force);
}

Vector3 PhysicsBody3D::get_constant_force() const {
	return PhysicsServer3D::body_get_constant_force(get_rid());
}

void PhysicsBody3D::set_constant_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_set_constant_torque(get_rid(), p_torque);
}

Vector3 PhysicsBody3D::get_constant_torque() const {
	return PhysicsServer3D::body_get_constant_torque(get_rid());
}

// ============================================================================
// CharacterMover3D
// ============================================================================

void PhysicsBody3D::_set_collision_direction(const PS3DT::MotionResult &p_result, CharacterMover3D::FloorFlags &r_state, CharacterMover3D::FloorFlags p_apply_state) {
	r_state = CharacterMover3D::FloorFlags();
	if (character.motion_mode == MOTION_MODE_GROUNDED && character.up_direction != Vector3()) {
		for (int i = p_result.collision_count - 1; i >= 0; i--) {
			const PS3DT::MotionCollision &col = p_result.collisions[i];
			real_t floor_angle = Math::acos(col.normal.dot(character.up_direction));
			if (floor_angle <= character.floor_max_angle + 0.01f) {
				r_state.floor = true;
				if (p_apply_state.floor) {
					character.floor_normal = col.normal;
				}
			} else if (floor_angle >= (Math::PI - character.floor_max_angle - 0.01f)) {
				r_state.ceiling = true;
				if (p_apply_state.ceiling) {
					character.ceiling_normal = col.normal;
				}
			} else {
				r_state.wall = true;
				if (p_apply_state.wall) {
					character.wall_normal = col.normal;
				}
			}
		}
	} else {
		if (p_result.collision_count > 0) {
			r_state.wall = true;
			if (p_apply_state.wall) {
				character.wall_normal = p_result.collisions[0].normal;
			}
		}
	}
}

void PhysicsBody3D::apply_floor_snap() {
	if (character.flags.floor) {
		return;
	}

	real_t length = MAX(character.floor_snap_length, character.margin);
	PS3DT::MotionParameters parameters(get_global_transform(), -character.up_direction * length, character.margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true;
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CharacterMover3D::FloorFlags result_state;
		_set_collision_direction(result, result_state, CharacterMover3D::FloorFlags(true, false, false));

		if (result_state.floor) {
			if (result.travel.length() > character.margin) {
				result.travel = character.up_direction * character.up_direction.dot(result.travel);
			} else {
				result.travel = Vector3();
			}
			parameters.from.origin += result.travel;
			set_global_transform(parameters.from);
		}
	}
}

void PhysicsBody3D::_snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up) {
	if (character.flags.floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return;
	}
	apply_floor_snap();
}

bool PhysicsBody3D::_on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up) {
	if (character.up_direction == Vector3() || character.flags.floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return false;
	}

	real_t length = MAX(character.floor_snap_length, character.margin);
	PS3DT::MotionParameters parameters(get_global_transform(), -character.up_direction * length, character.margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true;
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CharacterMover3D::FloorFlags result_state;
		_set_collision_direction(result, result_state, CharacterMover3D::FloorFlags(false, false, false));
		return result_state.floor;
	}
	return false;
}

bool PhysicsBody3D::move_and_slide() {
	double delta = Engine::is_in_physics_frame() ? get_physics_process_delta_time() : get_process_delta_time();
	if (delta <= 0.0) {
		return false;
	}

	Transform3D gt = get_global_transform();
	character.previous_position = gt.origin;

	bool was_on_floor = character.flags.floor;
	character.flags = CharacterMover3D::FloorFlags();
	character.motion_results.clear();
	character.last_motion = Vector3();

	Vector3 motion = character.velocity * delta;
	for (int iteration = 0; iteration < character.max_slides; ++iteration) {
		PS3DT::MotionParameters parameters(get_global_transform(), motion, character.margin);
		parameters.max_collisions = 6;
		parameters.recovery_as_collision = true;

		PS3DT::MotionResult result;
		bool collided = move_and_collide(parameters, result, false, false);
		character.last_motion = result.travel;

		if (collided) {
			character.motion_results.push_back(result);
			CharacterMover3D::FloorFlags step_flags;
			_set_collision_direction(result, step_flags);
			character.flags.floor = character.flags.floor || step_flags.floor;
			character.flags.wall = character.flags.wall || step_flags.wall;
			character.flags.ceiling = character.flags.ceiling || step_flags.ceiling;

			if (result.remainder.is_zero_approx()) {
				break;
			}
			const Vector3 &hit_normal = result.collisions[0].normal;
			motion = result.remainder.slide(hit_normal);
			character.velocity = character.velocity.slide(hit_normal);
		} else {
			break;
		}
	}

	if (character.motion_mode == MOTION_MODE_GROUNDED) {
		bool vel_dir_facing_up = character.velocity.dot(character.up_direction) > 0;
		_snap_on_floor(was_on_floor, vel_dir_facing_up);
	}

	character.real_velocity = (get_global_transform().origin - character.previous_position) / delta;
	return !character.motion_results.is_empty();
}

void PhysicsBody3D::set_up_direction(const Vector3 &p_up_direction) {
	ERR_FAIL_COND_MSG(p_up_direction == Vector3(), "up_direction can't be equal to Vector3.ZERO, consider using Floating motion mode instead.");
	character.up_direction = p_up_direction.normalized();
}

real_t PhysicsBody3D::get_floor_angle(const Vector3 &p_up_direction) const {
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0.0f);
	return Math::acos(character.floor_normal.dot(p_up_direction));
}

PS3DT::MotionResult PhysicsBody3D::get_slide_collision(int p_bounce) const {
	ERR_FAIL_INDEX_V(p_bounce, character.motion_results.size(), PS3DT::MotionResult());
	return character.motion_results[p_bounce];
}

void PhysicsBody3D::set_max_slides(int p_max_slides) {
	ERR_FAIL_COND(p_max_slides < 1);
	character.max_slides = p_max_slides;
}

void PhysicsBody3D::set_floor_snap_length(real_t p_floor_snap_length) {
	ERR_FAIL_COND(p_floor_snap_length < 0);
	character.floor_snap_length = p_floor_snap_length;
}

// ============================================================================
// VehicleRig3D
// ============================================================================

PhysicsBody3D::VehicleWheelContactPoint::VehicleWheelContactPoint(PhysicsDirectBodyState3D *s,
		PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld,
		real_t maxImpulse) :
		m_s(s), m_body1(body1), m_frictionPositionWorld(frictionPosWorld),
		m_frictionDirectionWorld(frictionDirectionWorld), m_maxImpulse(maxImpulse) {
	Vector3 r0 = frictionPosWorld - s->get_transform().origin;
	Vector3 c0 = r0.cross(frictionDirectionWorld);
	Vector3 vec = s->get_inverse_inertia_tensor().xform_inv(c0).cross(r0);
	real_t denom0 = s->get_inverse_mass() + frictionDirectionWorld.dot(vec);
	m_jacDiagABInv = denom0 != 0.0f ? (1.0f / denom0) : 0.0f;
}

void PhysicsBody3D::_update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s) {
	wheel.m_raycastInfo.m_isInContact = false;
	Transform3D chassisTrans = s->get_transform();
	wheel.m_raycastInfo.m_hardPointWS = chassisTrans.xform(wheel.m_chassisConnectionPointCS);
	wheel.m_raycastInfo.m_wheelDirectionWS = chassisTrans.get_basis().xform(wheel.m_wheelDirectionCS).normalized();
	wheel.m_raycastInfo.m_wheelAxleWS = chassisTrans.get_basis().xform(wheel.m_wheelAxleCS).normalized();
}

void PhysicsBody3D::_update_wheel(int p_idx, PhysicsDirectBodyState3D *s) {
	VehicleWheel3D &wheel = *vehicle.wheels[p_idx];
	_update_wheel_transform(wheel, s);

	Vector3 up = -wheel.m_raycastInfo.m_wheelDirectionWS;
	const Vector3 &right = wheel.m_raycastInfo.m_wheelAxleWS;
	Vector3 fwd = up.cross(right).normalized();

	VehicleWheel3D::FTIData &id = wheel.fti_data;
	Vector3 origin = wheel.m_raycastInfo.m_hardPointWS + wheel.m_raycastInfo.m_wheelDirectionWS * wheel.m_raycastInfo.m_suspensionLength;

	if (is_physics_interpolated_and_enabled()) {
		id.curr.up = up;
		id.curr.right = right;
		id.curr.steering = wheel.m_steering;
		id.curr.origin = origin;
		if (id.unset || id.reset_queued) {
			id.unset = false;
			id.reset_queued = false;
			id.pump();
		}
	}

	Basis steeringMat(up, wheel.m_steering);
	Basis rotatingMat(right, wheel.m_rotation);
	Basis basis2(right[0], up[0], fwd[0], right[1], up[1], fwd[1], right[2], up[2], fwd[2]);

	wheel.m_worldTransform.set_basis(steeringMat * rotatingMat * basis2);
	wheel.m_worldTransform.set_origin(origin);
}

void PhysicsBody3D::_update_vehicle_suspension(PhysicsDirectBodyState3D *s) {
	for (int w_it = 0; w_it < vehicle.wheels.size(); w_it++) {
		VehicleWheel3D &wheel_info = *vehicle.wheels[w_it];
		if (wheel_info.m_raycastInfo.m_isInContact) {
			real_t length_diff = wheel_info.m_suspensionRestLength - wheel_info.m_raycastInfo.m_suspensionLength;
			real_t force = wheel_info.m_suspensionStiffness * length_diff * wheel_info.m_clippedInvContactDotSuspension;

			real_t projected_rel_vel = wheel_info.m_suspensionRelativeVelocity;
			real_t susp_damping = projected_rel_vel < 0.0f ? wheel_info.m_wheelsDampingCompression : wheel_info.m_wheelsDampingRelaxation;
			force -= susp_damping * projected_rel_vel;

			wheel_info.m_wheelsSuspensionForce = MAX(force * mass, real_t(0.0));
		} else {
			wheel_info.m_wheelsSuspensionForce = 0.0f;
		}
	}
}

void PhysicsBody3D::_resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1,
		PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, real_t p_rollInfluence) {
	if (normal.length_squared() > real_t(1.1)) {
		impulse = 0.0f;
		return;
	}

	Vector3 rel_pos1 = pos1 - s->get_transform().origin;
	Vector3 rel_pos2 = body2 ? (pos2 - body2->get_global_transform().origin) : Vector3();

	Vector3 vel1 = s->get_linear_velocity() + s->get_angular_velocity().cross(rel_pos1);
	Vector3 vel2 = body2 ? (body2->get_linear_velocity() + body2->get_angular_velocity().cross(rel_pos2)) : Vector3();
	real_t rel_vel = normal.dot(vel1 - vel2);

	real_t b2invmass = body2 ? body2->get_inverse_mass() : 0.0f;
	real_t contactDamping = 0.2f;
	if (p_rollInfluence > 0.0f) {
		contactDamping = MIN(contactDamping, s->get_step() / p_rollInfluence);
	}

	real_t massTerm = 1.0f / ((1.0f / mass) + b2invmass);
	impulse = -contactDamping * rel_vel * massTerm;
}

real_t PhysicsBody3D::_calc_rolling_friction(VehicleWheelContactPoint &contactPoint) {
	const Vector3 &contactPosWorld = contactPoint.m_frictionPositionWorld;
	Vector3 rel_pos1 = contactPosWorld - contactPoint.m_s->get_transform().origin;
	Vector3 rel_pos2 = contactPoint.m_body1 ? (contactPosWorld - contactPoint.m_body1->get_global_transform().origin) : Vector3();

	Vector3 vel1 = contactPoint.m_s->get_linear_velocity() + contactPoint.m_s->get_angular_velocity().cross(rel_pos1);
	Vector3 vel2 = contactPoint.m_body1 ? (contactPoint.m_body1->get_linear_velocity() + contactPoint.m_body1->get_angular_velocity().cross(rel_pos2)) : Vector3();

	real_t vrel = contactPoint.m_frictionDirectionWorld.dot(vel1 - vel2);
	real_t j1 = -vrel * contactPoint.m_jacDiagABInv;
	return CLAMP(j1, -contactPoint.m_maxImpulse, contactPoint.m_maxImpulse);
}

void PhysicsBody3D::_update_vehicle_friction(PhysicsDirectBodyState3D *s) {
	int numWheel = vehicle.wheels.size();
	if (!numWheel) {
		return;
	}

	vehicle.forward_ws.resize(numWheel);
	vehicle.axle.resize(numWheel);
	vehicle.forward_impulse.resize(numWheel);
	vehicle.side_impulse.resize(numWheel);

	for (int i = 0; i < numWheel; i++) {
		vehicle.side_impulse.write[i] = 0.0f;
		vehicle.forward_impulse.write[i] = 0.0f;

		VehicleWheel3D &wheelInfo = *vehicle.wheels[i];
		if (wheelInfo.m_raycastInfo.m_isInContact) {
			Basis wheelBasis0 = wheelInfo.m_worldTransform.basis;
			vehicle.axle.write[i] = wheelBasis0.get_column(Vector3::AXIS_X);
			const Vector3 &surfNormalWS = wheelInfo.m_raycastInfo.m_contactNormalWS;
			real_t proj = vehicle.axle[i].dot(surfNormalWS);
			vehicle.axle.write[i] = (vehicle.axle[i] - surfNormalWS * proj).normalized();
			vehicle.forward_ws.write[i] = surfNormalWS.cross(vehicle.axle[i]).normalized();

			_resolve_single_bilateral(s, wheelInfo.m_raycastInfo.m_contactPointWS,
					wheelInfo.m_raycastInfo.m_groundObject,
					wheelInfo.m_raycastInfo.m_contactPointWS, vehicle.axle[i], vehicle.side_impulse.write[i],
					wheelInfo.m_rollInfluence);
		}
	}

	bool sliding = false;
	for (int wheel = 0; wheel < numWheel; wheel++) {
		VehicleWheel3D &wheelInfo = *vehicle.wheels[wheel];
		real_t rollingFriction = 0.0f;
		if (wheelInfo.m_raycastInfo.m_isInContact) {
			if (wheelInfo.m_engineForce != 0.0f) {
				rollingFriction = -wheelInfo.m_engineForce * s->get_step();
			} else {
				real_t maxImpulse = wheelInfo.m_brake ? wheelInfo.m_brake : 0.0f;
				VehicleWheelContactPoint contactPt(s, wheelInfo.m_raycastInfo.m_groundObject,
						wheelInfo.m_raycastInfo.m_contactPointWS, vehicle.forward_ws[wheel], maxImpulse);
				rollingFriction = _calc_rolling_friction(contactPt);
			}
		}

		vehicle.forward_impulse.write[wheel] = 0.0f;
		wheelInfo.m_skidInfo = 1.0f;

		if (wheelInfo.m_raycastInfo.m_isInContact) {
			real_t maximp = wheelInfo.m_wheelsSuspensionForce * s->get_step() * wheelInfo.m_frictionSlip;
			real_t maximpSquared = maximp * maximp;
			vehicle.forward_impulse.write[wheel] = rollingFriction;

			real_t x = vehicle.forward_impulse[wheel] * 0.5f;
			real_t y = vehicle.side_impulse[wheel];
			real_t impulseSquared = x * x + y * y;
			if (impulseSquared > maximpSquared) {
				sliding = true;
				wheelInfo.m_skidInfo *= maximp / Math::sqrt(impulseSquared);
			}
		}
	}

	if (sliding) {
		for (int wheel = 0; wheel < numWheel; wheel++) {
			if (vehicle.side_impulse[wheel] != 0.0f && vehicle.wheels[wheel]->m_skidInfo < 1.0f) {
				vehicle.forward_impulse.write[wheel] *= vehicle.wheels[wheel]->m_skidInfo;
				vehicle.side_impulse.write[wheel] *= vehicle.wheels[wheel]->m_skidInfo;
			}
		}
	}

	for (int wheel = 0; wheel < numWheel; wheel++) {
		VehicleWheel3D &wheelInfo = *vehicle.wheels[wheel];
		Vector3 rel_pos = wheelInfo.m_raycastInfo.m_contactPointWS - s->get_transform().origin;
		if (vehicle.forward_impulse[wheel] != 0.0f) {
			s->apply_impulse(vehicle.forward_ws[wheel] * vehicle.forward_impulse[wheel], rel_pos);
		}
		if (vehicle.side_impulse[wheel] != 0.0f) {
			Vector3 sideImp = vehicle.axle[wheel] * vehicle.side_impulse[wheel];
			Vector3 vChassisWorldUp = s->get_transform().basis.transposed()[1];
			rel_pos -= vChassisWorldUp * (vChassisWorldUp.dot(rel_pos) * (1.0f - wheelInfo.m_rollInfluence));
			s->apply_impulse(sideImp, rel_pos);
		}
	}
}

real_t PhysicsBody3D::_vehicle_ray_cast(int p_idx, PhysicsDirectBodyState3D *s) {
	VehicleWheel3D &wheel = *vehicle.wheels[p_idx];
	_update_wheel_transform(wheel, s);

	real_t raylen = wheel.m_suspensionRestLength + wheel.m_wheelRadius;
	Vector3 rayvector = wheel.m_raycastInfo.m_wheelDirectionWS * raylen;
	const Vector3 &source = wheel.m_raycastInfo.m_hardPointWS;
	wheel.m_raycastInfo.m_contactPointWS = source + rayvector;

	PhysicsDirectSpaceState3D *ss = s->get_space_state();
	if (ss) {
		PS3DT::RayParameters ray_params;
		ray_params.from = source;
		ray_params.to = source + rayvector;
		ray_params.exclude = vehicle.exclude;
		ray_params.collision_mask = collision_state.collision_mask;

		PS3DT::RayResult rr;
		if (ss->intersect_ray(ray_params, rr)) {
			real_t param = source.distance_to(rr.position) / raylen;
			wheel.m_raycastInfo.m_contactNormalWS = rr.normal;
			wheel.m_raycastInfo.m_isInContact = true;
			wheel.m_raycastInfo.m_groundObject = nullptr;
			real_t hit_dist = param * raylen;
			wheel.m_raycastInfo.m_suspensionLength = CLAMP(hit_dist - wheel.m_wheelRadius, wheel.m_suspensionRestLength - wheel.m_maxSuspensionTravel, wheel.m_suspensionRestLength + wheel.m_maxSuspensionTravel);
			wheel.m_raycastInfo.m_contactPointWS = rr.position;
			wheel._update(s);
			return hit_dist;
		}
	}

	wheel.m_raycastInfo.m_suspensionLength = wheel.m_suspensionRestLength;
	wheel.m_raycastInfo.m_groundObject = nullptr;
	wheel._update(s);
	return -1.0f;
}

void PhysicsBody3D::_update_vehicle_process_mode() {
	set_process_internal(is_physics_interpolated_and_enabled());
}

void PhysicsBody3D::_step_vehicle(PhysicsDirectBodyState3D *p_state) {
	real_t step = p_state->get_step();
	bool use_fti = is_physics_interpolated_and_enabled();

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		_update_wheel(i, p_state);
	}

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		_vehicle_ray_cast(i, p_state);
		if (!use_fti) {
			vehicle.wheels[i]->set_transform(p_state->get_transform().inverse() * vehicle.wheels[i]->m_worldTransform);
		}
	}

	_update_vehicle_suspension(p_state);

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		VehicleWheel3D &wheel = *vehicle.wheels[i];
		real_t suspensionForce = MIN(wheel.m_wheelsSuspensionForce, wheel.m_maxSuspensionForce);
		Vector3 impulse = wheel.m_raycastInfo.m_contactNormalWS * suspensionForce * step;
		Vector3 relative_position = wheel.m_raycastInfo.m_contactPointWS - p_state->get_transform().origin;
		p_state->apply_impulse(impulse, relative_position);
	}

	_update_vehicle_friction(p_state);

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		VehicleWheel3D &wheel = *vehicle.wheels[i];
		Vector3 relpos = wheel.m_raycastInfo.m_hardPointWS - p_state->get_transform().origin;
		Vector3 vel = p_state->get_linear_velocity() + p_state->get_angular_velocity().cross(relpos);

		if (wheel.m_raycastInfo.m_isInContact) {
			const Transform3D &chassisWorldTransform = p_state->get_transform();
			Vector3 fwd(chassisWorldTransform.basis[0][Vector3::AXIS_Z],
					chassisWorldTransform.basis[1][Vector3::AXIS_Z],
					chassisWorldTransform.basis[2][Vector3::AXIS_Z]);
			if (wheel.steers) {
				fwd = Basis(Vector3(0, 1, 0), wheel.m_steering).xform(fwd);
			}
			real_t proj = fwd.dot(wheel.m_raycastInfo.m_contactNormalWS);
			fwd = (fwd - wheel.m_raycastInfo.m_contactNormalWS * proj).normalized();
			wheel.m_deltaRotation = (fwd.dot(vel) * step) / wheel.m_wheelRadius;
		}

		wheel.m_rotation += wheel.m_deltaRotation;
		if (use_fti) {
			wheel.fti_data.rotate(wheel.m_deltaRotation);
		}
		wheel.m_rpm = ((wheel.m_deltaRotation / step) * 60.0f) / Math::TAU;
		wheel.m_deltaRotation *= 0.99f;
	}
}

void PhysicsBody3D::set_engine_force(real_t p_engine_force) {
	vehicle.engine_force = p_engine_force;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		if (vehicle.wheels[i]->engine_traction) {
			vehicle.wheels[i]->m_engineForce = p_engine_force;
		}
	}
}

void PhysicsBody3D::set_brake(real_t p_brake) {
	vehicle.brake = p_brake;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		vehicle.wheels[i]->m_brake = p_brake;
	}
}

void PhysicsBody3D::set_steering(real_t p_steering) {
	vehicle.steering = p_steering;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		if (vehicle.wheels[i]->steers) {
			vehicle.wheels[i]->m_steering = p_steering;
		}
	}
}

// ============================================================================
// PhysicalBoneState3D
// ============================================================================

PhysicalBoneSimulator3D *PhysicsBody3D::get_simulator() const {
	return dynamic_cast<PhysicalBoneSimulator3D *>(get_parent());
}

void PhysicsBody3D::set_body_offset(const Transform3D &p_offset) {
	bone_state.body_offset = p_offset;
	bone_state.body_offset_inverse = p_offset.affine_inverse();
	if (get_simulator()) {
		bone_state.joint_offset.origin = bone_state.body_offset_inverse.origin;
	}
}

void PhysicsBody3D::set_simulate_physics(bool p_simulate) {
	if (bone_state.simulate_physics == p_simulate) {
		return;
	}
	bone_state.simulate_physics = p_simulate;
	reset_physics_simulation_state();
}

void PhysicsBody3D::reset_physics_simulation_state() {
	if (bone_state.simulate_physics) {
		_start_bone_physics_simulation();
	} else {
		_stop_bone_physics_simulation();
	}
}

void PhysicsBody3D::_start_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		return;
	}
	bone_state.internal_simulate_physics = true;
	_apply_server_body_mode();
}

void PhysicsBody3D::_stop_bone_physics_simulation() {
	if (!bone_state.internal_simulate_physics) {
		return;
	}
	bone_state.internal_simulate_physics = false;
	_apply_server_body_mode();
}

#ifdef TOOLS_ENABLED
Transform3D PhysicsBody3D::get_global_gizmo_transform() const {
	if (mode == MODE_PHYSICAL_BONE && bone_state.gizmo_move_joint) {
		return get_global_transform() * bone_state.joint_offset;
	}
	return Node3D::get_global_gizmo_transform();
}

Transform3D PhysicsBody3D::get_local_gizmo_transform() const {
	if (mode == MODE_PHYSICAL_BONE && bone_state.gizmo_move_joint) {
		return get_transform() * bone_state.joint_offset;
	}
	return Node3D::get_local_gizmo_transform();
}
#endif

PackedStringArray PhysicsBody3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nConsider adding an inline Shape3D or a CollisionShape3D / CollisionPolygon3D child."));
	}

	if (SceneTree::is_fti_enabled_in_project() && !is_physics_interpolated()) {
		warnings.push_back(RTR("PhysicsBody3D will not work correctly on a non-interpolated branch of the SceneTree.\nCheck the node's inherited physics_interpolation_mode."));
	}

	if (mode == MODE_RIGID || mode == MODE_VEHICLE) {
		Vector3 scale = get_transform().get_basis().get_scale();
		if (Math::abs(scale.x - 1.0f) > 0.05f || Math::abs(scale.y - 1.0f) > 0.05f || Math::abs(scale.z - 1.0f) > 0.05f) {
			warnings.push_back(RTR("Scale changes to Rigid/Vehicle PhysicsBody3D will be overridden by the physics engine when running.\nPlease change the size in collision shapes instead."));
		}
	}

	return warnings;
}

// ==========================================================================
// Merged from scene/3d/physics/kinematic_collision_3d.cpp
// ==========================================================================

#include "scene/3d/physics/collision_object_3d.h"

Vector3 KinematicCollision3D::get_travel() const { return result.travel; }

Vector3 KinematicCollision3D::get_remainder() const { return result.remainder; }

int KinematicCollision3D::get_collision_count() const { return result.collision_count; }

real_t KinematicCollision3D::get_depth() const { return result.collision_depth; }

Vector3 KinematicCollision3D::get_position(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].position;
}

Vector3 KinematicCollision3D::get_normal(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].normal;
}

real_t KinematicCollision3D::get_angle(int p_collision_index, const Vector3& p_up_direction) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0.0);
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0);
	return result.collisions[p_collision_index].get_angle(p_up_direction);
}

RID KinematicCollision3D::get_collider_rid(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, RID());
	return result.collisions[p_collision_index].collider;
}

int KinematicCollision3D::get_collider_shape_index(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].collider_shape;
}

Vector3 KinematicCollision3D::get_collider_velocity(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].collider_velocity;
}

// ==========================================================================
// Merged from scene/3d/physics/physical_bone_simulator_3d.cpp
// ==========================================================================

#include "scene/3d/physics/collision_object_3d.h"

#include "core/config/engine.h"
#include "scene/3d/physics/collision_object_3d.h"

void PhysicalBoneSimulator3D::_bone_pose_updated(Skeleton3D *p_skeleton, int p_bone_id) {
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_bone_id, bones.size());
	bones[p_bone_id].global_pose = p_skeleton->get_bone_global_pose(p_bone_id);
}

void PhysicalBoneSimulator3D::_set_active(bool p_active) {
	if (!Engine::is_editor_hint()) {
		_reset_physical_bones_state();
	}
}

void PhysicalBoneSimulator3D::_reset_physical_bones_state() {
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		if (bones[i].physical_bone) {
			bones[i].physical_bone->reset_physics_simulation_state();
		}
	}
}

bool PhysicalBoneSimulator3D::is_simulating_physics() const {
	return simulating;
}

int PhysicalBoneSimulator3D::get_bone_count() const {
	return bones.size();
}

PhysicsBody3D *PhysicalBoneSimulator3D::get_physical_bone(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	return bones[p_bone].physical_bone;
}

PhysicsBody3D *PhysicalBoneSimulator3D::get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	if (bones[p_bone].cache_parent_physical_bone) {
		return bones[p_bone].cache_parent_physical_bone;
	}
	return _get_physical_bone_parent(p_bone);
}

PhysicsBody3D *PhysicalBoneSimulator3D::_get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	const int parent_bone = bones[p_bone].parent;
	if (parent_bone < 0) {
		return nullptr;
	}
	PhysicsBody3D *pb = bones[parent_bone].physical_bone;
	return pb ? pb : get_physical_bone_parent(parent_bone);
}

Transform3D PhysicalBoneSimulator3D::get_bone_global_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose;
}

void PhysicalBoneSimulator3D::set_bone_global_pose(int p_bone, const Transform3D &p_pose) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose = p_pose;
}

void PhysicalBoneSimulator3D::_skeleton_changed(Skeleton3D *p_old_skeleton, Skeleton3D *p_new_skeleton) {}

// ============================================================================
// Merged from scene/3d/physics/collision_object_3d.cpp
// ============================================================================

#include "collision_object_3d.h"

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

void CollisionState3D::init(bool p_area, PS3DE::BodyMode p_mode) {
	area = p_area;
	body_mode = p_mode;
	if (area) {
		rid = PhysicsServer3D::area_create();
	} else {
		rid = PhysicsServer3D::body_create();
		PhysicsServer3D::body_set_mode(rid, body_mode);
	}
}

void CollisionState3D::free_rid() {
	clear_debug_shapes();
	if (rid.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(rid);
	}
	rid = RID();
}

void CollisionState3D::clear_debug_shapes() {
	if (!Renderer::data) {
		debug_shapes_count = 0;
		return;
	}
	for (KeyValue<uint32_t, ShapeData> &E : shapes) {
		ShapeData &sd = E.value;
		for (int i = 0; i < sd.shapes.size(); i++) {
			if (sd.shapes[i].debug_shape.is_valid()) {
				Renderer::free_rid(sd.shapes[i].debug_shape);
				sd.shapes.write[i].debug_shape = RID();
			}
		}
	}
	debug_shapes_count = 0;
}

void CollisionState3D::on_transform_changed(const Node3D *p_owner) {
	if (debug_shapes_count > 0 && !debug_shape_old_transform.is_equal_approx(p_owner->get_global_transform())) {
		debug_shape_old_transform = p_owner->get_global_transform();
		for (KeyValue<uint32_t, ShapeData> &E : shapes) {
			ShapeData &sd = E.value;
			if (sd.disabled) {
				continue;
			}
			const ShapeData::ShapeBase *shape_bases = sd.shapes.ptr();
			for (int i = 0; i < sd.shapes.size(); i++) {
				if (shape_bases[i].debug_shape.is_null()) {
					continue;
				}
				Renderer::instance_set_transform(shape_bases[i].debug_shape, debug_shape_old_transform * sd.xform);
			}
		}
	}
}

void CollisionState3D::handle_notification(Node3D *p_owner, int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			if (area) {
				PhysicsServer3D::area_set_transform(rid, p_owner->get_global_transform());
			}
			if (p_owner->is_enabled() && p_owner->get_world_3d().is_valid()) {
				RID space = p_owner->get_world_3d()->get_space();
				if (area) {
					PhysicsServer3D::area_set_space(rid, space);
				} else {
					PhysicsServer3D::body_set_space(rid, space);
				}
			}
			update_pickable(p_owner);
		} break;
		case Node3D::NOTIFICATION_TRANSFORM_CHANGED: {
			on_transform_changed(p_owner);
			if (!only_update_transform_changes && area) {
				PhysicsServer3D::area_set_transform(rid, p_owner->get_global_transform());
			}
		} break;
		case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
			update_pickable(p_owner);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			clear_debug_shapes();
			if (callback_lock == 0) {
				if (area) {
					PhysicsServer3D::area_set_space(rid, RID());
				} else {
					PhysicsServer3D::body_set_space(rid, RID());
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

void CollisionState3D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	if (area) {
		PhysicsServer3D::area_set_collision_layer(rid, p_layer);
	} else {
		PhysicsServer3D::body_set_collision_layer(rid, p_layer);
	}
}

void CollisionState3D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	if (area) {
		PhysicsServer3D::area_set_collision_mask(rid, p_mask);
	} else {
		PhysicsServer3D::body_set_collision_mask(rid, p_mask);
	}
}

void CollisionState3D::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer_new = collision_layer;
	if (p_value) {
		layer_new |= 1 << (p_layer_number - 1);
	} else {
		layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer_new);
}

bool CollisionState3D::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_layer & (1 << (p_layer_number - 1));
}

void CollisionState3D::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = collision_mask;
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CollisionState3D::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_mask & (1 << (p_layer_number - 1));
}

void CollisionState3D::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer3D::body_set_collision_priority(rid, p_priority);
	}
}

void CollisionState3D::set_disable_mode(Node3D *p_owner, DisableMode p_mode) {
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

void CollisionState3D::apply_disabled(Node3D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				if (callback_lock > 0) {
					ERR_PRINT("Disabling a collision node during a physics callback is not allowed.");
				} else {
					if (area) {
						PhysicsServer3D::area_set_space(rid, RID());
					} else {
						PhysicsServer3D::body_set_space(rid, RID());
					}
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
				PhysicsServer3D::body_set_mode(rid, PS3DE::BODY_MODE_STATIC);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState3D::apply_enabled(Node3D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree() && p_owner->get_world_3d().is_valid()) {
				RID space = p_owner->get_world_3d()->get_space();
				if (area) {
					PhysicsServer3D::area_set_space(rid, space);
				} else {
					PhysicsServer3D::body_set_space(rid, space);
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
				PhysicsServer3D::body_set_mode(rid, body_mode);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState3D::set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode) {
	ERR_FAIL_COND(area);
	if (body_mode == p_mode) {
		return;
	}
	body_mode = p_mode;
	if (p_owner->is_inside_tree() && !p_owner->is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}
	PhysicsServer3D::body_set_mode(rid, p_mode);
}

void CollisionState3D::set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable) {
	ray_pickable = p_ray_pickable;
	update_pickable(p_owner);
}

void CollisionState3D::update_pickable(const Node3D *p_owner) {
	if (!p_owner->is_inside_tree()) {
		return;
	}
	bool pick = ray_pickable && p_owner->is_visible_in_tree();
	if (area) {
		PhysicsServer3D::area_set_ray_pickable(rid, pick);
	} else {
		PhysicsServer3D::body_set_ray_pickable(rid, pick);
	}
}

// Inline shape slots.

int CollisionState3D::add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform, bool p_disabled) {
	ShapeSlot slot;
	slot.local_xform = p_xform;
	slot.shape = p_shape;
	slot.disabled = p_disabled;
	slot.owner_id = create_shape_owner();
	shape_owner_set_transform(slot.owner_id, p_xform);
	shape_owner_set_disabled(slot.owner_id, p_disabled);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, p_shape);
	}
	int idx = (int)inline_shapes.size();
	inline_shapes.push_back(slot);
	return idx;
}

void CollisionState3D::set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	ShapeSlot &slot = inline_shapes[p_index];
	slot.shape = p_shape;
	shape_owner_clear_shapes(slot.owner_id);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, p_shape);
	}
}

Ref<Shape3D> CollisionState3D::get_inline_shape(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Ref<Shape3D>());
	return inline_shapes[p_index].shape;
}

void CollisionState3D::set_inline_shape_transform(int p_index, const Transform3D &p_xform) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].local_xform = p_xform;
	shape_owner_set_transform(inline_shapes[p_index].owner_id, p_xform);
}

Transform3D CollisionState3D::get_inline_shape_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Transform3D());
	return inline_shapes[p_index].local_xform;
}

void CollisionState3D::set_inline_shape_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].disabled = p_disabled;
	shape_owner_set_disabled(inline_shapes[p_index].owner_id, p_disabled);
}

bool CollisionState3D::is_inline_shape_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), false);
	return inline_shapes[p_index].disabled;
}

void CollisionState3D::remove_inline_shape(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	remove_shape_owner(inline_shapes[p_index].owner_id);
	inline_shapes.remove_at(p_index);
}

void CollisionState3D::clear_inline_shapes() {
	for (uint32_t i = 0; i < inline_shapes.size(); i++) {
		remove_shape_owner(inline_shapes[i].owner_id);
	}
	inline_shapes.clear();
}

// Shape owners.

uint32_t CollisionState3D::create_shape_owner(Node *p_owner) {
	uint32_t id = next_owner_id++;
	ShapeData sd;
	sd.owner_node = p_owner;
	shapes[id] = sd;
	return id;
}

void CollisionState3D::remove_shape_owner(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	shape_owner_clear_shapes(p_owner);
	shapes.erase(p_owner);
}

void CollisionState3D::get_shape_owners(List<uint32_t> *r_owners) const {
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionState3D::get_shape_owners_array() const {
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		ret.push_back(E.key);
	}
	return ret;
}

void CollisionState3D::shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::area_set_shape_transform(rid, sd.shapes[i].index, p_transform);
		} else {
			PhysicsServer3D::body_set_shape_transform(rid, sd.shapes[i].index, p_transform);
		}
	}
}

Transform3D CollisionState3D::shape_owner_get_transform(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform3D());
	return shapes[p_owner].xform;
}

void CollisionState3D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	if (sd.disabled == p_disabled) {
		return;
	}
	sd.disabled = p_disabled;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::area_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		} else {
			PhysicsServer3D::body_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		}
	}
}

bool CollisionState3D::is_shape_owner_disabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].disabled;
}

void CollisionState3D::shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_COND(p_shape.is_null());
	ShapeData &sd = shapes[p_owner];
	ShapeData::ShapeBase s;
	s.index = total_subshapes;
	s.shape = p_shape;
	if (area) {
		PhysicsServer3D::area_add_shape(rid, p_shape->get_rid(), sd.xform, sd.disabled);
	} else {
		PhysicsServer3D::body_add_shape(rid, p_shape->get_rid(), sd.xform, sd.disabled);
	}
	sd.shapes.push_back(s);
	total_subshapes++;
}

int CollisionState3D::shape_owner_get_shape_count(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].shapes.size();
}

Ref<Shape3D> CollisionState3D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape3D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape3D>());
	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionState3D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);
	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionState3D::shape_owner_remove_shape(uint32_t p_owner, int p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_INDEX(p_shape, shapes[p_owner].shapes.size());

	int index_to_remove = shapes[p_owner].shapes[p_shape].index;
	RID debug_rid = shapes[p_owner].shapes[p_shape].debug_shape;
	if (debug_rid.is_valid() && Renderer::data) {
		Renderer::free_rid(debug_rid);
		debug_shapes_count--;
	}
	if (area) {
		PhysicsServer3D::area_remove_shape(rid, index_to_remove);
	} else {
		PhysicsServer3D::body_remove_shape(rid, index_to_remove);
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

void CollisionState3D::shape_owner_clear_shapes(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}
}

uint32_t CollisionState3D::shape_find_owner(int p_shape_index) const {
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
// Merged from scene/3d/physics/area_3d.cpp
// ==========================================================================

#include "scene/3d/physics/collision_object_3d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"

Area3D::Area3D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area3D::~Area3D() {
	collision_state.free_rid();
}

void Area3D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_initialize_wind();
		} break;
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_DISABLED: {
			_clear_monitoring();
		} break;
	}
}

void Area3D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area3D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area3D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area3D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area3D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area3D::set_gravity_point_center(const Vector3 &p_center) {
	gravity_vec = p_center;
}

const Vector3 &Area3D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area3D::set_gravity_direction(const Vector3 &p_direction) {
	gravity_vec = p_direction;
}

const Vector3 &Area3D::get_gravity_direction() const {
	return gravity_vec;
}

void Area3D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area3D::get_gravity() const {
	return gravity;
}

void Area3D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area3D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area3D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area3D::get_linear_damp() const {
	return linear_damp;
}

void Area3D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area3D::get_angular_damp() const {
	return angular_damp;
}

void Area3D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area3D::get_priority() const {
	return priority;
}

void Area3D::set_wind_force_magnitude(real_t p_wind_force_magnitude) {
	wind_force_magnitude = p_wind_force_magnitude;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_force_magnitude() const {
	return wind_force_magnitude;
}

void Area3D::set_wind_attenuation_factor(real_t p_wind_force_attenuation_factor) {
	wind_attenuation_factor = p_wind_force_attenuation_factor;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_attenuation_factor() const {
	return wind_attenuation_factor;
}

void Area3D::set_wind_source_path(const NodePath &p_wind_source_path) {
	wind_source_path = p_wind_source_path;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

const NodePath &Area3D::get_wind_source_path() const {
	return wind_source_path;
}

void Area3D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area3D::is_monitoring() const {
	return monitoring;
}

void Area3D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer3D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer3D::area_set_monitorable(get_rid(), monitorable);
}

bool Area3D::is_monitorable() const {
	return monitorable;
}

bool Area3D::has_overlapping_bodies() const {
	return false;
}

bool Area3D::has_overlapping_areas() const {
	return false;
}

bool Area3D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area3D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area3D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area3D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area3D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area3D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area3D::set_use_reverb_bus(bool p_enable) {
	use_reverb_bus = p_enable;
}

bool Area3D::is_using_reverb_bus() const {
	return use_reverb_bus;
}

void Area3D::set_reverb_bus_name(const String &p_audio_bus) {
	reverb_bus = p_audio_bus;
}

String Area3D::get_reverb_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == reverb_bus) {
			return reverb_bus;
		}
	}
	return SceneString(Master);
}

void Area3D::set_reverb_amount(float p_amount) {
	reverb_amount = p_amount;
}

float Area3D::get_reverb_amount() const {
	return reverb_amount;
}

void Area3D::set_reverb_uniformity(float p_uniformity) {
	reverb_uniformity = p_uniformity;
}

float Area3D::get_reverb_uniformity() const {
	return reverb_uniformity;
}

void Area3D::_initialize_wind() {}

void Area3D::_clear_monitoring() {}

PackedStringArray Area3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape3D / CollisionPolygon3D child."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/physics/collision_shape_3d.cpp
// ==========================================================================

#include "scene/3d/physics/collision_object_3d.h"

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"

void CollisionShape3D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionShape3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node3D *parent_3d = dynamic_cast<Node3D *>(get_parent());
			collision_state = parent_3d ? parent_3d->get_collision_state_3d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape);
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

#ifndef DISABLE_DEPRECATED
void CollisionShape3D::resource_changed(Ref<Resource> res) {}
#endif

Ref<Shape3D> CollisionShape3D::get_shape() const {
	return shape;
}

void CollisionShape3D::set_shape(const Ref<Shape3D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	if (shape.is_valid()) {
		shape->set_debug_color(debug_color);
		shape->set_debug_fill(debug_fill);
	}
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape);
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

void CollisionShape3D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape3D::is_disabled() const {
	return disabled;
}

Color CollisionShape3D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape3D::set_debug_color(const Color &p_color) {
	if (debug_color == p_color) {
		return;
	}
	debug_color = p_color;
	if (shape.is_valid()) {
		shape->set_debug_color(p_color);
	}
}

Color CollisionShape3D::get_debug_color() const {
	return debug_color;
}

void CollisionShape3D::set_debug_fill_enabled(bool p_enable) {
	if (debug_fill == p_enable) {
		return;
	}
	debug_fill = p_enable;
	if (shape.is_valid()) {
		shape->set_debug_fill(p_enable);
	}
}

bool CollisionShape3D::get_debug_fill_enabled() const {
	return debug_fill;
}

#ifdef DEBUG_ENABLED
void CollisionShape3D::_shape_changed() {
	if (shape.is_valid()) {
		if (shape->get_debug_color() != debug_color) {
			set_debug_color(shape->get_debug_color());
		}
		if (shape->get_debug_fill() != debug_fill) {
			set_debug_fill_enabled(shape->get_debug_fill());
		}
	}
}
#endif // DEBUG_ENABLED

CollisionShape3D::CollisionShape3D() {
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

CollisionShape3D::~CollisionShape3D() {}

PackedStringArray CollisionShape3D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/physics/collision_polygon_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/physics/collision_polygon_3d.h ---
#include "scene/3d/node_3d.h"

class CollisionPolygon3D : public Node3D {
	real_t margin = 0.04f;

protected:
	real_t depth = 1.0f;
	AABB aabb = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	Vector<Point2> polygon;

	uint32_t owner_id = 0;
	CollisionState3D *collision_state = nullptr;

	Color debug_color;
	bool debug_fill = true;
	bool disabled = false;

	Color _get_default_debug_color() const;
	void _update_in_shape_owner(bool p_xform_only = false);
	bool _is_editable_3d_polygon() const;
	void _notification(int p_what);

public:
	real_t get_depth() const;
	Vector<Point2> get_polygon() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	Color get_debug_color() const;
	bool get_debug_fill_enabled() const;

	virtual AABB get_item_rect() const;
	real_t get_margin() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionPolygon3D();
};

// --- End scene/3d/physics/collision_polygon_3d.h ---

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"

void CollisionPolygon3D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionPolygon3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node3D *parent_3d = dynamic_cast<Node3D *>(get_parent());
			collision_state = parent_3d ? parent_3d->get_collision_state_3d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
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

Vector<Point2> CollisionPolygon3D::get_polygon() const {
	return polygon;
}

AABB CollisionPolygon3D::get_item_rect() const {
	return aabb;
}

real_t CollisionPolygon3D::get_depth() const {
	return depth;
}

void CollisionPolygon3D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon3D::is_disabled() const {
	return disabled;
}

Color CollisionPolygon3D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

Color CollisionPolygon3D::get_debug_color() const {
	return debug_color;
}

bool CollisionPolygon3D::get_debug_fill_enabled() const {
	return debug_fill;
}

real_t CollisionPolygon3D::get_margin() const {
	return margin;
}

bool CollisionPolygon3D::_is_editable_3d_polygon() const {
	return true;
}

CollisionPolygon3D::CollisionPolygon3D() {
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionPolygon3D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/physics/ray_cast_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/3d/physics/ray_cast_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/material.h"

class ArrayMesh;

class RayCast3D : public Node3D
{
	bool enabled = true;
	bool collided = false;
	RID against_rid;
	int against_shape = 0;
	Vector3 collision_point;
	Vector3 collision_normal;
	int collision_face_index = -1;

	Vector3 target_position = Vector3(0, -1, 0);
	HashSet<RID> exclude;

	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;

	Ref<Material> debug_material;
	Color debug_shape_custom_color = Color(0.0, 0.0, 0.0);
	int debug_shape_thickness = 2;
	Vector<Vector3> debug_shape_vertices;
	Vector<Vector3> debug_line_vertices;

	void _create_debug_shape();
	void _update_debug_shape();
	void _update_debug_shape_material(bool p_check_collision = false);
	void _update_debug_shape_vertices();
	void _clear_debug_shape();

	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	bool hit_from_inside = false;
	bool hit_back_faces = true;

	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

protected:
	void _notification(int p_what);
	void _update_raycast_state();

public:
	void set_collide_with_areas(bool p_enabled);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_enabled);
	bool is_collide_with_bodies_enabled() const;

	void set_hit_from_inside(bool p_enabled);
	bool is_hit_from_inside_enabled() const;

	void set_hit_back_faces(bool p_enabled);
	bool is_hit_back_faces_enabled() const;

	bool is_enabled() const;

	Vector3 get_target_position() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	const Color& get_debug_shape_custom_color() const;
	void set_debug_shape_custom_color(const Color& p_color);

	const Vector<Vector3>& get_debug_shape_vertices() const;
	const Vector<Vector3>& get_debug_line_vertices() const;

	Ref<StandardMaterial3D> get_debug_material();

	int get_debug_shape_thickness() const;

	void force_raycast_update();
	bool is_colliding() const;
	RID get_collider_rid() const;
	int get_collider_shape() const;
	Vector3 get_collision_point() const;
	Vector3 get_collision_normal() const;
	int get_collision_face_index() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node3D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node3D* rp_node);
	void clear_exceptions();

	RayCast3D() = default;
};



// --- End scene/3d/physics/ray_cast_3d.h ---
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

Vector3 RayCast3D::get_target_position() const { return target_position; }

void RayCast3D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast3D::get_collision_mask() const { return collision_mask; }

void RayCast3D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool RayCast3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast3D::is_colliding() const { return collided; }

RID RayCast3D::get_collider_rid() const { return against_rid; }

int RayCast3D::get_collider_shape() const { return against_shape; }

Vector3 RayCast3D::get_collision_point() const { return collision_point; }

Vector3 RayCast3D::get_collision_normal() const { return collision_normal; }

int RayCast3D::get_collision_face_index() const { return collision_face_index; }

bool RayCast3D::is_enabled() const { return enabled; }

bool RayCast3D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast3D::force_raycast_update() { _update_raycast_state(); }

void RayCast3D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast3D::add_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	add_exception_rid(state->rid);
}

void RayCast3D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast3D::remove_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	remove_exception_rid(state->rid);
}

void RayCast3D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast3D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast3D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast3D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast3D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast3D::is_hit_from_inside_enabled() const { return hit_from_inside; }

void RayCast3D::set_hit_back_faces(bool p_enabled) { hit_back_faces = p_enabled; }

bool RayCast3D::is_hit_back_faces_enabled() const { return hit_back_faces; }

int RayCast3D::get_debug_shape_thickness() const { return debug_shape_thickness; }

void RayCast3D::_update_debug_shape_vertices()
{
	debug_shape_vertices.clear();
	debug_line_vertices.clear();

	if (target_position == Vector3()) {
		return;
	}

	debug_line_vertices.push_back(Vector3());
	debug_line_vertices.push_back(target_position);

	if (debug_shape_thickness > 1) {
		float scale_factor = 100.0;
		Vector3 dir = Vector3(target_position).normalized();
		// Draw truncated pyramid
		Vector3 normal = (std::abs(dir.x) + std::abs(dir.y) > CMP_EPSILON)
							 ? Vector3(-dir.y, dir.x, 0).normalized()
							 : Vector3(0, -dir.z, dir.y).normalized();
		normal *= debug_shape_thickness / scale_factor;
		int vertices_strip_order[14] = {4, 5, 0, 1, 2, 5, 6, 4, 7, 0, 3, 2, 7, 6};
		for (int v = 0; v < 14; v++) {
			Vector3 vertex = vertices_strip_order[v] < 4 ? normal : normal / 3.0 + target_position;
			debug_shape_vertices.push_back(
				vertex.rotated(dir, Math::PI * (0.5 * (vertices_strip_order[v] % 4) + 0.25)));
		}
	}
}

const Vector<Vector3>& RayCast3D::get_debug_shape_vertices() const { return debug_shape_vertices; }

const Vector<Vector3>& RayCast3D::get_debug_line_vertices() const { return debug_line_vertices; }

void RayCast3D::set_debug_shape_custom_color(const Color& p_color)
{
	debug_shape_custom_color = p_color;
	if (debug_material.is_valid()) {
		_update_debug_shape_material();
	}
}

Ref<StandardMaterial3D> RayCast3D::get_debug_material()
{
	_update_debug_shape_material();
	return debug_material;
}

const Color& RayCast3D::get_debug_shape_custom_color() const { return debug_shape_custom_color; }

void RayCast3D::_create_debug_shape()
{
	_update_debug_shape_material();

	if (!debug_instance.is_valid()) {
		debug_instance = Renderer::instance_create();
	}

	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
}

void RayCast3D::_update_debug_shape_material(bool p_check_collision)
{
	if (debug_material.is_null()) {
		Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
		debug_material = material;

		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		// Use double-sided rendering so that the RayCast can be seen if the camera is inside.
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
		material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	}

	Color color = debug_shape_custom_color;
	if (color == Color(0.0, 0.0, 0.0)) {
		// Use the default debug shape color defined in the Project Settings.
		color = SceneTree::get_debug_collisions_color();
	}

	if (p_check_collision && collided) {
		if ((color.get_h() < 0.055 || color.get_h() > 0.945) && color.get_s() > 0.5 &&
			color.get_v() > 0.5) {
			// If base color is already quite reddish, highlight collision with green color
			color = Color(0.0, 1.0, 0.0, color.a);
		}
		else {
			// Else, highlight collision with red color
			color = Color(1.0, 0, 0, color.a);
		}
	}

	Ref<StandardMaterial3D> material = static_cast<Ref<StandardMaterial3D>>(debug_material);
}

void RayCast3D::_clear_debug_shape()
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
		debug_mesh = Ref<ArrayMesh>();
	}
}

void RayCast3D::_update_raycast_state() {}

// ==========================================================================
// Merged from scene/3d/physics/shape_cast_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/physics/shape_cast_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/material.h"

class ShapeCast3D : public Node3D
{
	bool enabled = true;
#ifndef DISABLE_DEPRECATED
	void resource_changed(Ref<Resource> p_res);
#endif

	Ref<Shape3D> shape;
	RID shape_rid;
	Vector3 target_position = Vector3(0, -1, 0);

	HashSet<RID> exclude;
	real_t margin = 0.0;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;
	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	Ref<Material> debug_material;
	Color debug_shape_custom_color = Color(0.0, 0.0, 0.0);
	Vector<Vector3> debug_shape_vertices;
	Vector<Vector3> debug_line_vertices;

	void _create_debug_shape();
	void _update_debug_shape();
	void _update_debug_shape_material(bool p_check_collision = false);
	void _update_debug_shape_vertices();
	void _clear_debug_shape();

	// Result
	int max_results = 32;
	Vector<PS3DT::ShapeRestInfo> result;
	bool collided = false;
	real_t collision_safe_fraction = 1.0;
	real_t collision_unsafe_fraction = 1.0;

	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

protected:
	void _notification(int p_what);
	void _update_shapecast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	bool is_enabled() const;

	void set_shape(const Ref<Shape3D>& p_shape);
	Ref<Shape3D> get_shape() const;

	Vector3 get_target_position() const;

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

	const Color& get_debug_shape_custom_color() const;
	void set_debug_shape_custom_color(const Color& p_color);

	const Vector<Vector3>& get_debug_shape_vertices() const;
	const Vector<Vector3>& get_debug_line_vertices() const;

	Ref<StandardMaterial3D> get_debug_material();

	int get_collision_count() const;
	RID get_collider_rid(int p_idx) const;
	int get_collider_shape(int p_idx) const;
	Vector3 get_collision_point(int p_idx) const;
	Vector3 get_collision_normal(int p_idx) const;

	real_t get_closest_collision_safe_fraction() const;
	real_t get_closest_collision_unsafe_fraction() const;

	void force_shapecast_update();
	bool is_colliding() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node3D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node3D* rp_node);
	void clear_exceptions();

	virtual PackedStringArray get_configuration_warnings() const override;
};



// --- End scene/3d/physics/shape_cast_3d.h ---

bool ShapeCast3D::is_enabled() const { return enabled; }

Vector3 ShapeCast3D::get_target_position() const { return target_position; }

void ShapeCast3D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast3D::get_margin() const { return margin; }

void ShapeCast3D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast3D::get_max_results() const { return max_results; }

void ShapeCast3D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast3D::get_collision_mask() const { return collision_mask; }

void ShapeCast3D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool ShapeCast3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast3D::get_collision_count() const { return result.size(); }

bool ShapeCast3D::is_colliding() const { return collided; }

RID ShapeCast3D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast3D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector3 ShapeCast3D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector3(), "No collision point found.");
	return result[p_idx].point;
}

Vector3 ShapeCast3D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector3(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast3D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast3D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

#ifndef DISABLE_DEPRECATED
void ShapeCast3D::resource_changed(Ref<Resource> p_res) {}
#endif

Ref<Shape3D> ShapeCast3D::get_shape() const { return shape; }

bool ShapeCast3D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast3D::_update_shapecast_state()
{
	result.clear();

	ERR_FAIL_COND_MSG(shape.is_null(),
		"Null reference to shape. ShapeCast3D requires a Shape3D to sweep for collisions.");

	Ref<World3D> w3d = get_world_3d();
	ERR_FAIL_COND(w3d.is_null());

	PhysicsDirectSpaceState3D* dss =
		PhysicsServer3D::space_get_direct_state(w3d->get_space());
	ERR_FAIL_NULL(dss);

	Transform3D gt = get_global_transform();

	PS3DT::ShapeParameters params;
	params.shape_rid = shape_rid;
	params.transform = gt;
	params.motion = gt.basis.xform(target_position);
	params.margin = margin;
	params.exclude = exclude;
	params.collision_mask = collision_mask;
	params.collide_with_bodies = collide_with_bodies;
	params.collide_with_areas = collide_with_areas;

	collision_safe_fraction = 0.0;
	collision_unsafe_fraction = 0.0;

	if (target_position != Vector3()) {
		dss->cast_motion(params, collision_safe_fraction, collision_unsafe_fraction);
		if (collision_unsafe_fraction < 1.0) {
			// Move shape transform to the point of impact,
			// so we can collect contact info at that point.
			gt.set_origin(
				gt.get_origin() + params.motion * (collision_unsafe_fraction + CMP_EPSILON));
			params.transform = gt;
		}
	}
	// Regardless of whether the shape is stuck or it's moved along
	// the motion vector, we'll only consider static collisions from now on.
	params.motion = Vector3();

	bool intersected = true;
	while (intersected && result.size() < max_results) {
		PS3DT::ShapeRestInfo info;
		intersected = dss->rest_info(params, &info);
		if (intersected) {
			result.push_back(info);
			params.exclude.insert(info.rid);
		}
	}
	collided = !result.is_empty();
}

void ShapeCast3D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast3D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast3D::add_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	add_exception_rid(state->rid);
}

void ShapeCast3D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast3D::remove_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	remove_exception_rid(state->rid);
}

void ShapeCast3D::clear_exceptions() { exclude.clear(); }

void ShapeCast3D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast3D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast3D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast3D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void ShapeCast3D::_update_debug_shape_vertices()
{
	debug_shape_vertices.clear();
	debug_line_vertices.clear();

	if (shape.is_valid()) {
		debug_shape_vertices.append_array(shape->get_debug_mesh_lines());
		for (int i = 0; i < debug_shape_vertices.size(); i++) {
			debug_shape_vertices.set(
				i, debug_shape_vertices[i] +
					   Vector3(target_position * get_closest_collision_safe_fraction()));
		}
	}

	if (target_position == Vector3()) {
		return;
	}

	debug_line_vertices.push_back(Vector3());
	debug_line_vertices.push_back(target_position);
}

const Vector<Vector3>& ShapeCast3D::get_debug_shape_vertices() const
{
	return debug_shape_vertices;
}

const Vector<Vector3>& ShapeCast3D::get_debug_line_vertices() const { return debug_line_vertices; }

void ShapeCast3D::set_debug_shape_custom_color(const Color& p_color)
{
	debug_shape_custom_color = p_color;
	if (debug_material.is_valid()) {
		_update_debug_shape_material();
	}
}

Ref<StandardMaterial3D> ShapeCast3D::get_debug_material()
{
	_update_debug_shape_material();
	return debug_material;
}

const Color& ShapeCast3D::get_debug_shape_custom_color() const { return debug_shape_custom_color; }

void ShapeCast3D::_create_debug_shape()
{
	_update_debug_shape_material();

	if (!debug_instance.is_valid()) {
		debug_instance = Renderer::instance_create();
	}

	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
}

void ShapeCast3D::_update_debug_shape_material(bool p_check_collision)
{
	if (debug_material.is_null()) {
		Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
		debug_material = material;

		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		// Use double-sided rendering so that the RayCast can be seen if the camera is inside.
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
		material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	}

	Color color = debug_shape_custom_color;
	if (color == Color(0.0, 0.0, 0.0)) {
		// Use the default debug shape color defined in the Project Settings.
		color = SceneTree::get_debug_collisions_color();
	}

	if (p_check_collision && collided) {
		if ((color.get_h() < 0.055 || color.get_h() > 0.945) && color.get_s() > 0.5 &&
			color.get_v() > 0.5) {
			// If base color is already quite reddish, highlight collision with green color
			color = Color(0.0, 1.0, 0.0, color.a);
		}
		else {
			// Else, highlight collision with red color
			color = Color(1.0, 0, 0, color.a);
		}
	}

	Ref<StandardMaterial3D> material = static_cast<Ref<StandardMaterial3D>>(debug_material);
}

void ShapeCast3D::_clear_debug_shape()
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
		debug_mesh = Ref<ArrayMesh>();
	}
}

// ==========================================================================
// Merged from scene/3d/physics/spring_arm_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
// --- Inlined from scene/3d/physics/spring_arm_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"

class SpringArm3D : public Node3D
{
	Ref<Shape3D> shape;
	HashSet<RID> excluded_objects;
	real_t spring_length = 1.0;
	real_t current_spring_length = 0.0;
	uint32_t mask = 1;
	real_t margin = 0.01;

protected:
	void _notification(int p_what);

public:
	real_t get_length() const;
	void set_shape(Ref<Shape3D> p_shape);
	Ref<Shape3D> get_shape() const;
	void set_mask(uint32_t p_mask);
	uint32_t get_mask();
	void add_excluded_object(RID p_rid);
	bool remove_excluded_object(RID p_rid);
	void clear_excluded_objects();
	real_t get_hit_length();
	void set_margin(real_t p_margin);
	real_t get_margin();

private:
	void process_spring();
};



// --- End scene/3d/physics/spring_arm_3d.h ---

void SpringArm3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!Engine::is_editor_hint()) {
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (!Engine::is_editor_hint()) {
			set_physics_process_internal(false);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		process_spring();
	} break;
	}
}


real_t SpringArm3D::get_length() const { return spring_length; }

void SpringArm3D::set_shape(Ref<Shape3D> p_shape) { shape = p_shape; }

Ref<Shape3D> SpringArm3D::get_shape() const { return shape; }

void SpringArm3D::set_mask(uint32_t p_mask) { mask = p_mask; }

uint32_t SpringArm3D::get_mask() { return mask; }

real_t SpringArm3D::get_margin() { return margin; }

void SpringArm3D::set_margin(real_t p_margin) { margin = p_margin; }

void SpringArm3D::add_excluded_object(RID p_rid) { excluded_objects.insert(p_rid); }

bool SpringArm3D::remove_excluded_object(RID p_rid) { return excluded_objects.erase(p_rid); }

void SpringArm3D::clear_excluded_objects() { excluded_objects.clear(); }

real_t SpringArm3D::get_hit_length() { return current_spring_length; }



void SpringArm3D::process_spring() {}

// ============================================================================
// Merged from scene/3d/physics/joints/joint_3d.cpp
// ============================================================================
// --- Inlined from scene/3d/physics/joints/joint_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/3d/physics/collision_object_3d.h"

class Joint3D : public Node3D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_HINGE,
		JOINT_SLIDER,
		JOINT_CONE_TWIST,
		JOINT_6DOF,
	};

	using PinParam = PS3DE::PinJointParam;
	using HingeParam = PS3DE::HingeJointParam;
	using HingeFlag = PS3DE::HingeJointFlag;
	using SliderParam = PS3DE::SliderJointParam;
	using ConeTwistParam = PS3DE::ConeTwistJointParam;
	using G6DOFParam = PS3DE::G6DOFJointAxisParam;
	using G6DOFFlag = PS3DE::G6DOFJointAxisFlag;

private:
	JointType joint_type = JOINT_PIN;
	RID ba, bb;
	RID joint;
	NodePath a;
	NodePath b;
	int solver_priority = 1;
	bool exclude_from_collision = true;
	String warning;
	bool configured = false;

	// Pin parameters.
	real_t pin_params[3] = { 0.3f, 1.0f, 0.0f };

	// Hinge parameters & flags.
	real_t hinge_params[PS3DE::HINGE_JOINT_MAX] = {};
	bool hinge_flags[PS3DE::HINGE_JOINT_FLAG_MAX] = {};

	// Slider parameters.
	real_t slider_params[PS3DE::SLIDER_JOINT_MAX] = {};

	// Cone twist parameters.
	static constexpr int CONE_TWIST_PARAM_MAX = 5;
	real_t cone_params[CONE_TWIST_PARAM_MAX] = {};

	// Generic 6DOF parameters & flags.
	real_t g6dof_params[3][PS3DE::G6DOF_JOINT_MAX] = {};
	bool g6dof_flags[3][PS3DE::G6DOF_JOINT_FLAG_MAX] = {};
	bool linear_drive_force_limit_set[3] = {};
	bool angular_drive_torque_limit_set[3] = {};
	Quaternion angular_target_rotation;
	bool has_angular_target_rotation = false;

	void _init_defaults();
	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b);
	bool _should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint3D(JointType p_type = JOINT_PIN);
	~Joint3D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const { return a; }

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const { return b; }

	void set_solver_priority(int p_priority);
	int get_solver_priority() const { return solver_priority; }

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const { return exclude_from_collision; }

	// Pin API.
	void set_pin_param(PinParam p_param, real_t p_value);
	real_t get_pin_param(PinParam p_param) const;

	// Hinge API.
	void set_hinge_param(HingeParam p_param, real_t p_value);
	real_t get_hinge_param(HingeParam p_param) const;
	void set_hinge_flag(HingeFlag p_flag, bool p_enabled);
	bool get_hinge_flag(HingeFlag p_flag) const;

	// Slider API.
	void set_slider_param(SliderParam p_param, real_t p_value);
	real_t get_slider_param(SliderParam p_param) const;

	// ConeTwist API.
	void set_cone_twist_param(ConeTwistParam p_param, real_t p_value);
	real_t get_cone_twist_param(ConeTwistParam p_param) const;

	// Generic 6DOF API.
	void set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value);
	real_t get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;
	void set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled);
	bool get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const;

	void set_param_x(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_X, p_param, p_value); }
	real_t get_param_x(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_X, p_param); }
	void set_param_y(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Y, p_param, p_value); }
	real_t get_param_y(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Y, p_param); }
	void set_param_z(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Z, p_param, p_value); }
	real_t get_param_z(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Z, p_param); }

	void set_flag_x(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_X, p_flag, p_enabled); }
	bool get_flag_x(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_X, p_flag); }
	void set_flag_y(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Y, p_flag, p_enabled); }
	bool get_flag_y(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Y, p_flag); }
	void set_flag_z(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Z, p_flag, p_enabled); }
	bool get_flag_z(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Z, p_flag); }

	void set_angular_target_rotation(const Quaternion &p_target_rotation);
	Quaternion get_angular_target_rotation() const;
	bool has_target_rotation() const { return has_angular_target_rotation; }
	void clear_angular_target_rotation();

	RID get_rid() const { return joint; }
};

// --- End scene/3d/physics/joints/joint_3d.h ---

void Joint3D::_init_defaults() {
	pin_params[PS3DE::PIN_JOINT_BIAS] = 0.3f;
	pin_params[PS3DE::PIN_JOINT_DAMPING] = 1.0f;
	pin_params[PS3DE::PIN_JOINT_IMPULSE_CLAMP] = 0.0f;

	hinge_params[PS3DE::HINGE_JOINT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_UPPER] = Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_LOWER] = -Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_SOFTNESS] = 0.9f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_RELAXATION] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE] = 1.0f;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_USE_LIMIT] = false;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR] = false;

	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER] = -1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING] = 1.0f;

	cone_params[PS3DE::CONE_TWIST_JOINT_SWING_SPAN] = Math::PI * 0.25f;
	cone_params[PS3DE::CONE_TWIST_JOINT_TWIST_SPAN] = Math::PI;
	cone_params[PS3DE::CONE_TWIST_JOINT_BIAS] = 0.3f;
	cone_params[PS3DE::CONE_TWIST_JOINT_SOFTNESS] = 0.8f;
	cone_params[PS3DE::CONE_TWIST_JOINT_RELAXATION] = 1.0f;

	for (int ax = 0; ax < 3; ax++) {
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS] = 0.7f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_ERP] = 0.5f;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT] = true;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT] = true;
	}
}

Joint3D::Joint3D(JointType p_type) :
		joint_type(p_type) {
	_init_defaults();
	set_notify_transform(true);
	joint = PhysicsServer3D::joint_create();
}

Joint3D::~Joint3D() {
	if (joint.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(joint);
	}
}

void Joint3D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	joint_type = p_type;
	_update_joint();
}

void Joint3D::_disconnect_signals() {}

void Joint3D::_update_joint(bool p_only_free) {
	if (configured) {
		PhysicsServer3D::joint_clear(joint);
		ba = RID();
		bb = RID();
		configured = false;
	}

	if (p_only_free || !is_inside_tree()) {
		warning = String();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody3D *body_a = dynamic_cast<PhysicsBody3D *>(node_a);
	PhysicsBody3D *body_b = dynamic_cast<PhysicsBody3D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a) {
		warning = RTR("Node A and Node B must be PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	if (body_a == body_b) {
		warning = RTR("Node A and Node B must be different PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	warning = String();
	update_configuration_warnings();

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();

	_configure_joint(joint, body_a, body_b);
	PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	PhysicsServer3D::joint_disable_collisions_between_bodies(joint, exclude_from_collision);
	configured = true;
}

bool Joint3D::_should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		return linear_drive_force_limit_set[p_axis];
	}
	if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		return angular_drive_torque_limit_set[p_axis];
	}
	return true;
}

void Joint3D::_configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b) {
	Transform3D gt = get_global_transform();

	if (joint_type == JOINT_PIN) {
		Vector3 pinpos = gt.origin;
		Vector3 local_a = body_a->to_local(pinpos);
		Vector3 local_b = body_b ? body_b->to_local(pinpos) : pinpos;
		PhysicsServer3D::joint_make_pin(p_joint, body_a->get_rid(), local_a, body_b ? body_b->get_rid() : RID(), local_b);
		for (int i = 0; i < 3; i++) {
			PhysicsServer3D::pin_joint_set_param(p_joint, PinParam(i), pin_params[i]);
		}
		return;
	}

	Transform3D local_a = (body_a->get_global_transform().affine_inverse() * gt).orthonormalized();
	Transform3D local_b = (body_b ? (body_b->get_global_transform().affine_inverse() * gt) : gt).orthonormalized();
	RID rid_b = body_b ? body_b->get_rid() : RID();

	switch (joint_type) {
		case JOINT_HINGE: {
			PhysicsServer3D::joint_make_hinge(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::HINGE_JOINT_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_param(p_joint, HingeParam(i), hinge_params[i]);
			}
			for (int i = 0; i < PS3DE::HINGE_JOINT_FLAG_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_flag(p_joint, HingeFlag(i), hinge_flags[i]);
			}
		} break;
		case JOINT_SLIDER: {
			PhysicsServer3D::joint_make_slider(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::SLIDER_JOINT_MAX; i++) {
				PhysicsServer3D::slider_joint_set_param(p_joint, SliderParam(i), slider_params[i]);
			}
		} break;
		case JOINT_CONE_TWIST: {
			PhysicsServer3D::joint_make_cone_twist(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < CONE_TWIST_PARAM_MAX; i++) {
				PhysicsServer3D::cone_twist_joint_set_param(p_joint, ConeTwistParam(i), cone_params[i]);
			}
		} break;
		case JOINT_6DOF: {
			PhysicsServer3D::joint_make_generic_6dof(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int ax = 0; ax < 3; ax++) {
				Vector3::Axis axis = Vector3::Axis(ax);
				for (int i = 0; i < PS3DE::G6DOF_JOINT_MAX; i++) {
					G6DOFParam param = G6DOFParam(i);
					if (_should_replay_g6dof_param(axis, param)) {
						PhysicsServer3D::generic_6dof_joint_set_param(p_joint, axis, param, g6dof_params[ax][i]);
					}
				}
				for (int i = 0; i < PS3DE::G6DOF_JOINT_FLAG_MAX; i++) {
					PhysicsServer3D::generic_6dof_joint_set_flag(p_joint, axis, G6DOFFlag(i), g6dof_flags[ax][i]);
				}
			}
			if (has_angular_target_rotation) {
				PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(p_joint, angular_target_rotation);
			}
		} break;
		default:
			break;
	}
}

void Joint3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (is_inside_tree()) {
				_update_joint();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
	}
}

void Joint3D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

void Joint3D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

void Joint3D::set_solver_priority(int p_priority) {
	solver_priority = p_priority;
	if (joint.is_valid()) {
		PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	}
}

void Joint3D::set_exclude_nodes_from_collision(bool p_enable) {
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

void Joint3D::set_pin_param(PinParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, 3);
	pin_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer3D::pin_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_pin_param(PinParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, 3, 0.0f);
	return pin_params[p_param];
}

void Joint3D::set_hinge_param(HingeParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::HINGE_JOINT_MAX);
	hinge_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_hinge_param(HingeParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::HINGE_JOINT_MAX, 0.0f);
	return hinge_params[p_param];
}

void Joint3D::set_hinge_flag(HingeFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX);
	hinge_flags[p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_flag(joint, p_flag, p_enabled);
	}
}

bool Joint3D::get_hinge_flag(HingeFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX, false);
	return hinge_flags[p_flag];
}

void Joint3D::set_slider_param(SliderParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::SLIDER_JOINT_MAX);
	slider_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_SLIDER) {
		PhysicsServer3D::slider_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_slider_param(SliderParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::SLIDER_JOINT_MAX, 0.0f);
	return slider_params[p_param];
}

void Joint3D::set_cone_twist_param(ConeTwistParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, CONE_TWIST_PARAM_MAX);
	cone_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_CONE_TWIST) {
		PhysicsServer3D::cone_twist_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_cone_twist_param(ConeTwistParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, CONE_TWIST_PARAM_MAX, 0.0f);
	return cone_params[p_param];
}

void Joint3D::set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_param, PS3DE::G6DOF_JOINT_MAX);
	g6dof_params[p_axis][p_param] = p_value;
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		linear_drive_force_limit_set[p_axis] = true;
	} else if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		angular_drive_torque_limit_set[p_axis] = true;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_param(joint, p_axis, p_param, p_value);
	}
}

real_t Joint3D::get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	ERR_FAIL_INDEX_V(p_axis, 3, 0.0f);
	ERR_FAIL_INDEX_V(p_param, PS3DE::G6DOF_JOINT_MAX, 0.0f);
	return g6dof_params[p_axis][p_param];
}

void Joint3D::set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX);
	g6dof_flags[p_axis][p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_flag(joint, p_axis, p_flag, p_enabled);
	}
}

bool Joint3D::get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_axis, 3, false);
	ERR_FAIL_INDEX_V(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX, false);
	return g6dof_flags[p_axis][p_flag];
}

void Joint3D::set_angular_target_rotation(const Quaternion &p_target_rotation) {
	ERR_FAIL_COND_MSG(!(p_target_rotation.is_finite() && p_target_rotation.length_squared() > CMP_EPSILON),
			"Angular target rotation must be a finite, non-zero quaternion.");
	angular_target_rotation = p_target_rotation.normalized();
	has_angular_target_rotation = true;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(joint, angular_target_rotation);
	}
}

Quaternion Joint3D::get_angular_target_rotation() const {
	if (has_angular_target_rotation) {
		return angular_target_rotation;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		return PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(joint);
	}
	return Quaternion();
}

void Joint3D::clear_angular_target_rotation() {
	if (!has_angular_target_rotation) {
		return;
	}
	has_angular_target_rotation = false;
	angular_target_rotation = Quaternion();
	if (is_configured() && joint_type == JOINT_6DOF) {
		for (int ax = 0; ax < 3; ax++) {
			PhysicsServer3D::generic_6dof_joint_set_param(joint, Vector3::Axis(ax),
					PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
					g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
		}
	}
}

PackedStringArray Joint3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}

// ============================================================================
// Merged from scene/3d/physics/soft_body_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/physics/soft_body_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

class MeshInstance3D;
class PhysicsBody3D;

class SoftBody3D : public Node3D
{
public:
	enum DisableMode
	{
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct PinnedPoint
	{
		int point_index = -1;
		NodePath spatial_attachment_path;
		Node3D* spatial_attachment = nullptr; // Cache
		Vector3 offset;

		PinnedPoint();
		PinnedPoint(const PinnedPoint& obj_tocopy);
		void operator=(const PinnedPoint& obj);
	};

	RenderInstance3D render_instance;

private:
	Ref<Mesh> mesh;
	PhysicsServer3DRenderingServerHandler* rendering_server_handler = nullptr;

	RID physics_rid;

	DisableMode disable_mode = DISABLE_MODE_REMOVE;

	RID owned_mesh;
	uint32_t collision_mask = 1;
	uint32_t collision_layer = 1;
	NodePath parent_collision_ignore;
	Vector<PinnedPoint> pinned_points;
	bool simulation_started = false;
	bool pinned_points_cache_dirty = true;

	Ref<ArrayMesh> debug_mesh_cache;
	MeshInstance3D* debug_mesh = nullptr;

	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	void _update_pickable();

	void _update_physics_server();
	void _draw_soft_mesh();

	void _prepare_physics_server();
	void _become_mesh_owner();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	PackedStringArray get_configuration_warnings() const override;

public:
	virtual bool has_render_instance() const override { return true; }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const { return mesh; }
	AABB get_aabb() const { return mesh.is_valid() ? mesh->get_aabb() : AABB(); }

	RID get_physics_rid() const { return physics_rid; }

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_disable_mode(DisableMode p_mode);
	DisableMode get_disable_mode() const;

	void set_parent_collision_ignore(const NodePath& p_parent_collision_ignore);
	const NodePath& get_parent_collision_ignore() const;
	int get_point_count() const;

	void set_pinned_points_indices(Vector<PinnedPoint> p_pinned_points_indices);
	Vector<PinnedPoint> get_pinned_points_indices();

	void set_simulation_precision(int p_simulation_precision);
	int get_simulation_precision();

	void set_total_mass(real_t p_total_mass);
	real_t get_total_mass();

	void set_linear_stiffness(real_t p_linear_stiffness);
	real_t get_linear_stiffness();

	void set_shrinking_factor(real_t p_shrinking_factor);
	real_t get_shrinking_factor();

	void set_pressure_coefficient(real_t p_pressure_coefficient);
	real_t get_pressure_coefficient();

	void set_damping_coefficient(real_t p_damping_coefficient);
	real_t get_damping_coefficient();

	void set_drag_coefficient(real_t p_drag_coefficient);
	real_t get_drag_coefficient();

	void add_collision_exception_with(Node* rp_node);
	void remove_collision_exception_with(Node* rp_node);

	Vector3 get_point_transform(int p_point_index);

	void pin_point(int p_point_index, bool pin,
		const NodePath& p_spatial_attachment_path = NodePath(), int p_insert_at = -1);
	bool is_point_pinned(int p_point_index) const;

	void _pin_point_deferred(int p_point_index, bool pin, const NodePath p_spatial_attachment_path);

	void set_ray_pickable(bool p_ray_pickable);
	bool is_ray_pickable() const;

	void apply_impulse(int p_point_index, const Vector3& p_impulse);
	void apply_force(int p_point_index, const Vector3& p_force);
	void apply_central_impulse(const Vector3& p_impulse);
	void apply_central_force(const Vector3& p_force);

	SoftBody3D();
	~SoftBody3D();

private:
	void _make_cache_dirty();
	void _update_cache_pin_points_datas();

	void _pin_point_on_physics_server(int p_point_index, bool pin);
	void _add_pinned_point(
		int p_point_index, const NodePath& p_spatial_attachment_path, int p_insert_at = -1);
	void _reset_points_offsets();

	void _remove_pinned_point(int p_point_index);
	int _get_pinned_point(int p_point_index, PinnedPoint*& r_point) const;
	int _has_pinned_point(int p_point_index) const;
};

// --- End scene/3d/physics/soft_body_3d.h ---

SoftBody3D::PinnedPoint::PinnedPoint() {}

SoftBody3D::PinnedPoint::PinnedPoint(const PinnedPoint& obj_tocopy)
{
	point_index = obj_tocopy.point_index;
	spatial_attachment_path = obj_tocopy.spatial_attachment_path;
	spatial_attachment = obj_tocopy.spatial_attachment;
	offset = obj_tocopy.offset;
}

void SoftBody3D::PinnedPoint::operator=(const PinnedPoint& obj)
{
	point_index = obj.point_index;
	spatial_attachment_path = obj.spatial_attachment_path;
	spatial_attachment = obj.spatial_attachment;
	offset = obj.offset;
}

void SoftBody3D::_update_pickable()
{
	if (!is_inside_tree()) {
		return;
	}
	bool pickable = ray_pickable && is_visible_in_tree();
	PhysicsServer3D::soft_body_set_ray_pickable(physics_rid, pickable);
}

void SoftBody3D::set_mesh(const Ref<Mesh>& p_mesh)
{
	mesh = p_mesh;
	render_instance.set_base(mesh.is_valid() ? mesh->get_rid() : RID());
	if (is_inside_tree()) {
		_prepare_physics_server();
	}
}

void SoftBody3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void SoftBody3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_WORLD: {
		if (Engine::is_editor_hint()) {
			// I have no idea what this is supposed to do, it's really weird
			// leaving for upcoming PK work on physics
			// add_change_receptor(this);
		}

		RID space = get_world_3d()->get_space();
		PhysicsServer3D::soft_body_set_space(physics_rid, space);
		_prepare_physics_server();
	} break;

	case NOTIFICATION_READY: {
		if (!parent_collision_ignore.is_empty()) {
			add_collision_exception_with(get_node(parent_collision_ignore));
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (Engine::is_editor_hint()) {
			_reset_points_offsets();
			return;
		}

		PhysicsServer3D::soft_body_set_transform(
			physics_rid, get_global_transform());

		set_notify_transform(false);
		// Required to be top level with Transform at center of world in order to modify
		// Renderer only to support custom Transform
		set_as_top_level(true);
		set_transform(Transform3D());
		set_notify_transform(true);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_pickable();
	} break;

	case NOTIFICATION_EXIT_WORLD: {
		PhysicsServer3D::soft_body_set_space(physics_rid, RID());
	} break;

	case NOTIFICATION_DISABLED: {
		if (is_inside_tree() && (disable_mode == DISABLE_MODE_REMOVE)) {
			_prepare_physics_server();
		}
	} break;

	case NOTIFICATION_ENABLED: {
		if (is_inside_tree() && (disable_mode == DISABLE_MODE_REMOVE)) {
			_prepare_physics_server();
		}
	} break;
	}
}

PackedStringArray SoftBody3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);

	if (mesh.is_null()) {
		warnings.push_back(RTR("This body will be ignored until you set a mesh."));
	}

	return warnings;
}

void SoftBody3D::_update_physics_server()
{
	if (!simulation_started) {
		return;
	}

	_update_cache_pin_points_datas();
	// Submit bone attachment
	const int pinned_points_indices_size = pinned_points.size();
	const PinnedPoint* r = pinned_points.ptr();
	for (int i = 0; i < pinned_points_indices_size; ++i) {
		if (r[i].spatial_attachment) {
			PhysicsServer3D::soft_body_move_point(physics_rid, r[i].point_index,
				r[i].spatial_attachment->get_global_transform().xform(r[i].offset));
		}
	}
}

void SoftBody3D::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	PhysicsServer3D::soft_body_set_collision_mask(physics_rid, p_mask);
}

uint32_t SoftBody3D::get_collision_mask() const { return collision_mask; }

void SoftBody3D::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	PhysicsServer3D::soft_body_set_collision_layer(physics_rid, p_layer);
}

uint32_t SoftBody3D::get_collision_layer() const { return collision_layer; }

void SoftBody3D::set_collision_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t collision_layer_new = get_collision_layer();
	if (p_value) {
		collision_layer_new |= 1 << (p_layer_number - 1);
	}
	else {
		collision_layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(collision_layer_new);
}

bool SoftBody3D::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void SoftBody3D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool SoftBody3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

void SoftBody3D::set_disable_mode(DisableMode p_mode)
{
	if (disable_mode == p_mode) {
		return;
	}

	disable_mode = p_mode;

	if (mesh.is_valid() && is_inside_tree() && !is_enabled()) {
		_prepare_physics_server();
	}
}

SoftBody3D::DisableMode SoftBody3D::get_disable_mode() const { return disable_mode; }

void SoftBody3D::set_parent_collision_ignore(const NodePath& p_parent_collision_ignore)
{
	parent_collision_ignore = p_parent_collision_ignore;
}

const NodePath& SoftBody3D::get_parent_collision_ignore() const { return parent_collision_ignore; }

void SoftBody3D::set_pinned_points_indices(Vector<SoftBody3D::PinnedPoint> p_pinned_points_indices)
{
	pinned_points = p_pinned_points_indices;
	for (int i = pinned_points.size() - 1; 0 <= i; --i) {
		pin_point(p_pinned_points_indices[i].point_index, true);
	}
}

Vector<SoftBody3D::PinnedPoint> SoftBody3D::get_pinned_points_indices() { return pinned_points; }

int SoftBody3D::get_simulation_precision()
{
	return PhysicsServer3D::soft_body_get_simulation_precision(physics_rid);
}

void SoftBody3D::set_simulation_precision(int p_simulation_precision)
{
	PhysicsServer3D::soft_body_set_simulation_precision(
		physics_rid, p_simulation_precision);
}

real_t SoftBody3D::get_total_mass()
{
	return PhysicsServer3D::soft_body_get_total_mass(physics_rid);
}

void SoftBody3D::set_total_mass(real_t p_total_mass)
{
	PhysicsServer3D::soft_body_set_total_mass(physics_rid, p_total_mass);
}

void SoftBody3D::set_linear_stiffness(real_t p_linear_stiffness)
{
	PhysicsServer3D::soft_body_set_linear_stiffness(
		physics_rid, p_linear_stiffness);
}

real_t SoftBody3D::get_linear_stiffness()
{
	return PhysicsServer3D::soft_body_get_linear_stiffness(physics_rid);
}

void SoftBody3D::set_shrinking_factor(real_t p_shrinking_factor)
{
	PhysicsServer3D::soft_body_set_shrinking_factor(
		physics_rid, p_shrinking_factor);
}

real_t SoftBody3D::get_shrinking_factor()
{
	return PhysicsServer3D::soft_body_get_shrinking_factor(physics_rid);
}

real_t SoftBody3D::get_pressure_coefficient()
{
	return PhysicsServer3D::soft_body_get_pressure_coefficient(physics_rid);
}

void SoftBody3D::set_pressure_coefficient(real_t p_pressure_coefficient)
{
	PhysicsServer3D::soft_body_set_pressure_coefficient(
		physics_rid, p_pressure_coefficient);
}

real_t SoftBody3D::get_damping_coefficient()
{
	return PhysicsServer3D::soft_body_get_damping_coefficient(physics_rid);
}

void SoftBody3D::set_damping_coefficient(real_t p_damping_coefficient)
{
	PhysicsServer3D::soft_body_set_damping_coefficient(
		physics_rid, p_damping_coefficient);
}

real_t SoftBody3D::get_drag_coefficient()
{
	return PhysicsServer3D::soft_body_get_drag_coefficient(physics_rid);
}

void SoftBody3D::set_drag_coefficient(real_t p_drag_coefficient)
{
	PhysicsServer3D::soft_body_set_drag_coefficient(
		physics_rid, p_drag_coefficient);
}

Vector3 SoftBody3D::get_point_transform(int p_point_index)
{
	return PhysicsServer3D::soft_body_get_point_global_position(
		physics_rid, p_point_index);
}

int SoftBody3D::get_point_count() const
{
	if (mesh.is_null()) {
		return 0;
	}

	const int vertex_count = mesh->surface_get_array_len(0);
	ERR_FAIL_COND_V(vertex_count < 0, 0);
	return vertex_count;
}

void SoftBody3D::apply_impulse(int p_point_index, const Vector3& p_impulse)
{
	PhysicsServer3D::soft_body_apply_point_impulse(
		physics_rid, p_point_index, p_impulse);
}

void SoftBody3D::apply_force(int p_point_index, const Vector3& p_force)
{
	PhysicsServer3D::soft_body_apply_point_force(
		physics_rid, p_point_index, p_force);
}

void SoftBody3D::apply_central_impulse(const Vector3& p_impulse)
{
	PhysicsServer3D::soft_body_apply_central_impulse(physics_rid, p_impulse);
}

void SoftBody3D::apply_central_force(const Vector3& p_force)
{
	PhysicsServer3D::soft_body_apply_central_force(physics_rid, p_force);
}

void SoftBody3D::pin_point(
	int p_point_index, bool pin, const NodePath& p_spatial_attachment_path, int p_insert_at)
{
	ERR_FAIL_COND_MSG(p_insert_at < -1 || p_insert_at >= pinned_points.size(),
		"Invalid index for pin point insertion position.");
	_pin_point_on_physics_server(p_point_index, pin);
	if (pin) {
		_add_pinned_point(p_point_index, p_spatial_attachment_path, p_insert_at);
	}
	else {
		_remove_pinned_point(p_point_index);
	}
}

void SoftBody3D::_pin_point_deferred(
	int p_point_index, bool pin, const NodePath p_spatial_attachment_path)
{
	pin_point(p_point_index, pin, p_spatial_attachment_path);
	_make_cache_dirty();
}

bool SoftBody3D::is_point_pinned(int p_point_index) const
{
	return -1 != _has_pinned_point(p_point_index);
}

void SoftBody3D::set_ray_pickable(bool p_ray_pickable)
{
	ray_pickable = p_ray_pickable;
	_update_pickable();
}

bool SoftBody3D::is_ray_pickable() const { return ray_pickable; }

SoftBody3D::SoftBody3D()
{
	rendering_server_handler = memnew(PhysicsServer3DRenderingServerHandler);
	if (PhysicsServer3D::is_initialized()) {
		physics_rid = PhysicsServer3D::soft_body_create();
	}
}

SoftBody3D::~SoftBody3D()
{
	memdelete(rendering_server_handler);
	if (physics_rid.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(physics_rid);
	}
}

void SoftBody3D::_make_cache_dirty() { pinned_points_cache_dirty = true; }

void SoftBody3D::_pin_point_on_physics_server(int p_point_index, bool pin)
{
	PhysicsServer3D::soft_body_pin_point(physics_rid, p_point_index, pin);
}

void SoftBody3D::_remove_pinned_point(int p_point_index)
{
	const int id(_has_pinned_point(p_point_index));
	if (-1 != id) {
		pinned_points.remove_at(id);
	}
}

int SoftBody3D::_get_pinned_point(int p_point_index, SoftBody3D::PinnedPoint*& r_point) const
{
	const int id = _has_pinned_point(p_point_index);
	if (-1 == id) {
		r_point = nullptr;
		return -1;
	}
	else {
		r_point = const_cast<SoftBody3D::PinnedPoint*>(&pinned_points.ptr()[id]);
		return id;
	}
}

int SoftBody3D::_has_pinned_point(int p_point_index) const
{
	const PinnedPoint* r = pinned_points.ptr();
	for (int i = pinned_points.size() - 1; 0 <= i; --i) {
		if (p_point_index == r[i].point_index) {
			return i;
		}
	}
	return -1;
}

void SoftBody3D::_prepare_physics_server() {}

void SoftBody3D::_update_cache_pin_points_datas() {}

void SoftBody3D::_reset_points_offsets() {}

void SoftBody3D::_add_pinned_point(int, NodePath const&, int) {}

void SoftBody3D::add_collision_exception_with(Node*) {}

// ============================================================================
// Merged from scene/resources/3d/shape_3d.cpp
// ============================================================================

#include "scene/resources/3d/shape_3d.h"

#include <cfloat> // FLT_MAX

#include "core/io/image.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

RID Shape3D::_create_shape_rid(ShapeType p_type)
{
	switch (p_type) {
	case SHAPE_WORLD_BOUNDARY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_WORLD_BOUNDARY);
	case SHAPE_SEPARATION_RAY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SEPARATION_RAY);
	case SHAPE_SPHERE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SPHERE);
	case SHAPE_BOX:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_BOX);
	case SHAPE_CAPSULE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CAPSULE);
	case SHAPE_CYLINDER:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CYLINDER);
	case SHAPE_CONVEX_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONVEX_POLYGON);
	case SHAPE_CONCAVE_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONCAVE_POLYGON);
	case SHAPE_HEIGHTMAP:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_HEIGHTMAP);
	}
	return RID();
}

void Shape3D::_ensure_heightmap_initialized()
{
	if (shape_type == SHAPE_HEIGHTMAP && map_data.size() != map_width * map_depth) {
		map_data.resize(map_width * map_depth);
		real_t* w = map_data.ptrw();
		for (int i = 0; i < map_width * map_depth; i++) {
			w[i] = 0.0;
		}
	}
}

void Shape3D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	if (shape.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(shape);
	}
	shape_type = p_type;
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	if (custom_bias != 0.0) {
		PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
	}
	if (margin != 0.04) {
		PhysicsServer3D::shape_set_margin(shape, margin);
	}
	_update_shape();
}

// World boundary

void Shape3D::set_plane(const Plane& p_plane)
{
	plane = p_plane;
	_update_shape();
}

const Plane& Shape3D::get_plane() const { return plane; }

// Separation ray

void Shape3D::set_length(float p_length)
{
	length = p_length;
	_update_shape();
}

float Shape3D::get_length() const { return length; }

void Shape3D::set_slide_on_slope(bool p_active)
{
	slide_on_slope = p_active;
	_update_shape();
}

bool Shape3D::get_slide_on_slope() const { return slide_on_slope; }

// Sphere / Capsule / Cylinder

void Shape3D::set_radius(float p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0f, "Shape3D radius cannot be negative.");
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && height < radius * 2.0f) {
		height = radius * 2.0f;
	}
	_update_shape();
}

float Shape3D::get_radius() const { return radius; }

void Shape3D::set_height(float p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0f, "Shape3D height cannot be negative.");
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
	_update_shape();
}

float Shape3D::get_height() const { return height; }

void Shape3D::set_mid_height(real_t p_mid_height)
{
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "Shape3D mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
	_update_shape();
}

real_t Shape3D::get_mid_height() const { return height - radius * 2.0f; }

// Box

void Shape3D::set_size(const Vector3& p_size)
{
	ERR_FAIL_COND_MSG(
		p_size.x < 0 || p_size.y < 0 || p_size.z < 0, "Shape3D size cannot be negative.");
	size = p_size;
	_update_shape();
}

Vector3 Shape3D::get_size() const { return size; }

// Convex polygon

void Shape3D::set_points(const Vector<Vector3>& p_points)
{
	points = p_points;
	_update_shape();
}

Vector<Vector3> Shape3D::get_points() const { return points; }

// Concave polygon

void Shape3D::set_faces(const Vector<Vector3>& p_faces)
{
	faces = p_faces;
	_update_shape();
}

Vector<Vector3> Shape3D::get_faces() const { return faces; }

void Shape3D::set_backface_collision_enabled(bool p_enabled)
{
	backface_collision = p_enabled;
	if (!faces.is_empty()) {
		_update_shape();
	}
}

bool Shape3D::is_backface_collision_enabled() const { return backface_collision; }

// Heightmap

void Shape3D::set_map_width(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_width != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_width = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_width() const { return map_width; }

void Shape3D::set_map_depth(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_depth != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_depth = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_depth() const { return map_depth; }

void Shape3D::set_map_data(Vector<real_t> p_new)
{
	int expected_size = (map_width * map_depth);
	if (p_new.size() != expected_size) {
		return;
	}

	map_data.resize(expected_size);
	real_t* w = map_data.ptrw();
	const real_t* r = p_new.ptr();
	for (int i = 0; i < expected_size; i++) {
		real_t val = r[i];
		w[i] = val;
		if (i == 0) {
			min_height = val;
			max_height = val;
		}
		else {
			if (min_height > val) {
				min_height = val;
			}
			if (max_height < val) {
				max_height = val;
			}
		}
	}

	_update_shape();
}

Vector<real_t> Shape3D::get_map_data() const { return map_data; }

real_t Shape3D::get_min_height() const { return min_height; }

real_t Shape3D::get_max_height() const { return max_height; }

void Shape3D::update_map_data_from_image(
	const Ref<Image>& p_image, real_t p_height_min, real_t p_height_max)
{
	ERR_FAIL_COND_MSG(
		p_image.is_null(), "Heightmap update image requires a valid Image reference.");
	ERR_FAIL_COND_MSG(p_image->get_format() != Image::FORMAT_RF &&
						  p_image->get_format() != Image::FORMAT_RH &&
						  p_image->get_format() != Image::FORMAT_R8,
		"Heightmap update image requires Image in format FORMAT_RF (32 bit), FORMAT_RH (16 bit), "
		"or FORMAT_R8 (8 bit).");
	ERR_FAIL_COND_MSG(
		p_image->get_width() < 2, "Heightmap update image requires a minimum Image width of 2.");
	ERR_FAIL_COND_MSG(
		p_image->get_height() < 2, "Heightmap update image requires a minimum Image height of 2.");
	ERR_FAIL_COND_MSG(p_height_min > p_height_max,
		"Heightmap update image requires height_max to be greater than height_min.");

	map_width = p_image->get_width();
	map_depth = p_image->get_height();
	map_data.resize(map_width * map_depth);

	real_t new_min_height = FLT_MAX;
	real_t new_max_height = -FLT_MAX;

	float remap_height_min = float(p_height_min);
	float remap_height_max = float(p_height_max);

	real_t* map_data_ptrw = map_data.ptrw();

	switch (p_image->get_format()) {
	case Image::FORMAT_RF: {
		const float* image_data_ptr = (float*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = image_data_ptr[i];

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_RH: {
		const uint16_t* image_data_ptr = (uint16_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = Math::half_to_float(image_data_ptr[i]);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_R8: {
		const uint8_t* image_data_ptr = (uint8_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = float(image_data_ptr[i] / 255.0);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	default: {
		return;
	}
	}

	min_height = new_min_height;
	max_height = new_max_height;

	_update_shape();
}

// Debug & bounds

Vector<Vector3> Shape3D::get_debug_mesh_lines() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Plane p = plane;
		Vector3 n1 = p.get_any_perpendicular_normal();
		Vector3 n2 = p.normal.cross(n1).normalized();

		Vector3 pface[4] = {
			p.normal * p.d + n1 * 10.0 + n2 * 10.0,
			p.normal * p.d + n1 * 10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * 10.0,
		};

		Vector<Vector3> wb_points = {pface[0], pface[1], pface[1], pface[2], pface[2], pface[3],
			pface[3], pface[0], p.normal * p.d, p.normal * p.d + p.normal * 3};
		return wb_points;
	}
	case SHAPE_SEPARATION_RAY: {
		return {Vector3(), Vector3(0, 0, length)};
	}
	case SHAPE_SPHERE: {
		Vector<Vector3> sphere_points;
		for (int i = 0; i <= 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			sphere_points.push_back(Vector3(a.x, 0, a.y));
			sphere_points.push_back(Vector3(b.x, 0, b.y));
			sphere_points.push_back(Vector3(0, a.x, a.y));
			sphere_points.push_back(Vector3(0, b.x, b.y));
			sphere_points.push_back(Vector3(a.x, a.y, 0));
			sphere_points.push_back(Vector3(b.x, b.y, 0));
		}
		return sphere_points;
	}
	case SHAPE_BOX: {
		return Vector<Vector3>();
	}
	case SHAPE_CAPSULE: {
		Vector<Vector3> capsule_points;
		Vector3 d(0, height * 0.5f - radius, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) + d);

			capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
				capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 180 ? d : -d;

			capsule_points.push_back(Vector3(0, a.x, a.y) + dud);
			capsule_points.push_back(Vector3(0, b.x, b.y) + dud);
			capsule_points.push_back(Vector3(a.y, a.x, 0) + dud);
			capsule_points.push_back(Vector3(b.y, b.x, 0) + dud);
		}
		return capsule_points;
	}
	case SHAPE_CYLINDER: {
		Vector<Vector3> cylinder_points;
		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) + d);

			cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
				cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			}
		}
		return cylinder_points;
	}
	case SHAPE_CONVEX_POLYGON: {
		return Vector<Vector3>();
	}
	case SHAPE_CONCAVE_POLYGON: {
		HashSet<DrawEdge, DrawEdge> edges;

		int index_count = faces.size();
		ERR_FAIL_COND_V((index_count % 3) != 0, Vector<Vector3>());

		const Vector3* r = faces.ptr();

		for (int i = 0; i < index_count; i += 3) {
			for (int j = 0; j < 3; j++) {
				DrawEdge de(r[i + j], r[i + ((j + 1) % 3)]);
				edges.insert(de);
			}
		}

		Vector<Vector3> concave_points;
		concave_points.resize(edges.size() * 2);
		int idx = 0;
		for (const DrawEdge& E : edges) {
			concave_points.write[idx + 0] = E.a;
			concave_points.write[idx + 1] = E.b;
			idx += 2;
		}
		return concave_points;
	}
	case SHAPE_HEIGHTMAP: {
		Vector<Vector3> hm_points;
		if ((map_width != 0) && (map_depth != 0) && map_data.size() >= map_width * map_depth) {
			Vector2 hm_size(map_width - 1, map_depth - 1);
			Vector2 start = hm_size * -0.5;

			const real_t* r = map_data.ptr();

			hm_points.resize(((map_width - 1) * map_depth * 2) + (map_width * (map_depth - 1) * 2) +
							 ((map_width - 1) * (map_depth - 1) * 2));

			int r_offset = 0;
			int w_offset = 0;
			for (int d = 0; d < map_depth; d++) {
				Vector3 h_pt(start.x, 0.0, start.y);

				for (int w = 0; w < map_width; w++) {
					h_pt.y = r[r_offset++];

					if (w != map_width - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
					}

					if (d != map_depth - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					if ((w != map_width - 1) && (d != map_depth - 1)) {
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					h_pt.x += 1.0;
				}

				start.y += 1.0;
			}
		}
		return hm_points;
	}
	}
	return Vector<Vector3>();
}

Ref<ArrayMesh> Shape3D::get_debug_arraymesh_faces(const Color& p_modulate) const
{
	(void)p_modulate;
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
	case SHAPE_SEPARATION_RAY:
	case SHAPE_SPHERE:
	case SHAPE_BOX:
	case SHAPE_CAPSULE:
	case SHAPE_CYLINDER:
	case SHAPE_CONVEX_POLYGON:
	case SHAPE_CONCAVE_POLYGON:
	case SHAPE_HEIGHTMAP:
	default:
		return memnew(ArrayMesh);
	}
}

real_t Shape3D::get_enclosing_radius() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
		return 0;
	case SHAPE_SEPARATION_RAY:
		return length;
	case SHAPE_SPHERE:
		return radius;
	case SHAPE_BOX:
		return size.length() / 2;
	case SHAPE_CAPSULE:
		return height * 0.5f;
	case SHAPE_CYLINDER:
		return Vector2(radius, height * 0.5).length();
	case SHAPE_CONVEX_POLYGON: {
		const Vector3* read = points.ptr();
		real_t r = 0.0;
		for (int i = 0; i < points.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_CONCAVE_POLYGON: {
		const Vector3* read = faces.ptr();
		real_t r = 0.0;
		for (int i = 0; i < faces.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_HEIGHTMAP:
		return Vector3(real_t(map_width), max_height - min_height, real_t(map_depth)).length();
	}
	return 0;
}

void Shape3D::add_vertices_to_array(Vector<Vector3>& array, const Transform3D& p_xform)
{
	Vector<Vector3> toadd = get_debug_mesh_lines();

	if (toadd.size()) {
		int base = array.size();
		array.resize(base + toadd.size());
		Vector3* w = array.ptrw();
		for (int i = 0; i < toadd.size(); i++) {
			w[i + base] = p_xform.xform(toadd[i]);
		}
	}
}

void Shape3D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape3D::get_custom_solver_bias() const { return custom_bias; }

real_t Shape3D::get_margin() const { return margin; }

void Shape3D::set_margin(real_t p_margin)
{
	margin = p_margin;
	PhysicsServer3D::shape_set_margin(shape, margin);
}

void Shape3D::set_debug_color(const Color& p_color)
{
	if (p_color == debug_color) {
		return;
	}

	debug_color = p_color;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

Color Shape3D::get_debug_color() const { return debug_color; }

void Shape3D::set_debug_fill(bool p_fill)
{
	if (p_fill == debug_fill) {
		return;
	}

	debug_fill = p_fill;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

bool Shape3D::get_debug_fill() const { return debug_fill; }

void Shape3D::_update_shape()
{
	emit_changed();
	debug_mesh_cache.unref();
}

Shape3D::Shape3D(ShapeType p_type) : shape_type(p_type)
{
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	_update_shape();
}

Shape3D::~Shape3D()
{
	ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
	PhysicsServer3D::free_rid(shape);
}

Ref<ArrayMesh> Shape3D::get_debug_mesh()
{
	if (debug_mesh_cache.is_null()) {
		debug_mesh_cache.instantiate();
	}
	return debug_mesh_cache;
}
