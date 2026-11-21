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

#include "physics_body_3d.h"

#include "core/config/engine.h"
#include "scene/3d/physics/physical_bone_simulator_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_3d/physics_server_3d_constants.h"

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

#include "kinematic_collision_3d.h"
#include "scene/3d/physics/physics_body_3d.h"

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

#include "physical_bone_simulator_3d.h"

#include "core/config/engine.h"
#include "scene/3d/physics/physics_body_3d.h"

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
