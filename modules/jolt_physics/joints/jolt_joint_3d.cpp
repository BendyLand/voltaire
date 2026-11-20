/**************************************************************************/
/*  jolt_joint_3d.cpp                                                     */
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

#include "jolt_joint_3d.h"

#include "../jolt_physics_server_3d.h"
#include "../misc/jolt_type_conversions.h"
#include "../objects/jolt_body_3d.h"
#include "../spaces/jolt_space_3d.h"

#include "core/config/engine.h"

#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>

namespace {

constexpr double PIN_DEFAULT_BIAS = 0.3;
constexpr double PIN_DEFAULT_DAMPING = 1.0;
constexpr double PIN_DEFAULT_IMPULSE_CLAMP = 0.0;

constexpr double HINGE_DEFAULT_BIAS = 0.3;
constexpr double HINGE_DEFAULT_LIMIT_BIAS = 0.3;
constexpr double HINGE_DEFAULT_SOFTNESS = 0.9;
constexpr double HINGE_DEFAULT_RELAXATION = 1.0;

constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_MOTION_DAMPING = 0.0;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING = 0.0;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS = 1.0;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION = 0.7;
constexpr double SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING = 1.0;

constexpr double CONE_TWIST_DEFAULT_BIAS = 0.3;
constexpr double CONE_TWIST_DEFAULT_SOFTNESS = 0.8;
constexpr double CONE_TWIST_DEFAULT_RELAXATION = 1.0;

constexpr double G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS = 0.7;
constexpr double G6DOF_DEFAULT_LINEAR_RESTITUTION = 0.5;
constexpr double G6DOF_DEFAULT_LINEAR_DAMPING = 1.0;
constexpr double G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS = 0.5;
constexpr double G6DOF_DEFAULT_ANGULAR_DAMPING = 1.0;
constexpr double G6DOF_DEFAULT_ANGULAR_RESTITUTION = 0.0;
constexpr double G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT = 0.0;
constexpr double G6DOF_DEFAULT_ANGULAR_ERP = 0.5;

double estimate_physics_step() {
	const double step = 1.0 / Engine::get_user_physics_ticks_per_second();
	const double step_scaled = step * Engine::get_effective_time_scale();

	return step_scaled;
}

} // namespace

// --- Common JoltJoint3D methods ---

void JoltJoint3D::_wake_up_bodies() {
	if (body_a != nullptr) {
		body_a->wake_up();
	}

	if (body_b != nullptr) {
		body_b->wake_up();
	}
}

void JoltJoint3D::_update_enabled() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetEnabled(enabled);
	}
}

void JoltJoint3D::_update_iterations() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetNumVelocityStepsOverride((JPH::uint)velocity_iterations);
		jolt_ref->SetNumPositionStepsOverride((JPH::uint)position_iterations);
	}
}

void JoltJoint3D::_update_priority() {
	if (jolt_ref != nullptr) {
		jolt_ref->SetConstraintPriority((JPH::uint)solver_priority);
	}
}

void JoltJoint3D::_update_joint() {
	_update_enabled();
	_update_iterations();
	_update_priority();
}

void JoltJoint3D::_enabled_changed() {
	_update_enabled();
	_wake_up_bodies();
}

void JoltJoint3D::_iterations_changed() {
	_update_iterations();
	_wake_up_bodies();
}

String JoltJoint3D::_bodies_to_string() const {
	return vformat("'%s' and '%s'", body_a != nullptr ? body_a->to_string() : "<unknown>",
			body_b != nullptr ? body_b->to_string() : "<World>");
}

void JoltJoint3D::_reconfigure(PS3DE::JointType p_type, JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	destroy();

	if (body_a != nullptr) {
		body_a->remove_joint(this);
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
	}

	joint_type = p_type;
	velocity_iterations = 0;
	position_iterations = 0;
	solver_priority = 1;
	body_a = p_body_a;
	body_b = p_body_b;
	local_ref_a = p_local_ref_a;
	local_ref_b = p_local_ref_b;

	hinge = HingeData();
	slider = SliderData();
	cone_twist = ConeTwistData();
	g6dof = G6DOFData();

	if (body_a != nullptr) {
		body_a->add_joint(this);
	}

	if (body_b != nullptr) {
		body_b->add_joint(this);
	}

	rebuild();
}

JoltJoint3D::~JoltJoint3D() {
	if (body_a != nullptr) {
		body_a->remove_joint(this);
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
	}

	destroy();
}

JoltSpace3D *JoltJoint3D::get_space() const {
	if (body_a != nullptr && body_b != nullptr) {
		JoltSpace3D *space_a = body_a->get_space();
		JoltSpace3D *space_b = body_b->get_space();

		if (space_a == nullptr || space_b == nullptr) {
			return nullptr;
		}

		ERR_FAIL_COND_V_MSG(space_a != space_b, nullptr,
				vformat("Joint was found to connect bodies in different physics spaces. This joint "
						"will effectively be disabled. This joint connects %s.",
						_bodies_to_string()));

		return space_a;
	} else if (body_a != nullptr) {
		return body_a->get_space();
	} else if (body_b != nullptr) {
		return body_b->get_space();
	}

	return nullptr;
}

void JoltJoint3D::set_enabled(bool p_enabled) {
	if (enabled == p_enabled) {
		return;
	}

	enabled = p_enabled;

	_enabled_changed();
}

void JoltJoint3D::set_solver_priority(int p_priority) {
	p_priority = MAX(0, p_priority);

	if (solver_priority == p_priority) {
		return;
	}

	solver_priority = p_priority;

	_update_priority();
}

void JoltJoint3D::set_solver_velocity_iterations(int p_iterations) {
	if (velocity_iterations == p_iterations) {
		return;
	}

	velocity_iterations = p_iterations;

	_iterations_changed();
}

void JoltJoint3D::set_solver_position_iterations(int p_iterations) {
	if (position_iterations == p_iterations) {
		return;
	}

	position_iterations = p_iterations;

	_iterations_changed();
}

void JoltJoint3D::set_collision_disabled(bool p_disabled) {
	collision_disabled = p_disabled;

	if (body_a == nullptr || body_b == nullptr) {
		return;
	}

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL(physics_server);

	if (collision_disabled) {
		physics_server->body_add_collision_exception(body_a->get_rid(), body_b->get_rid());
		physics_server->body_add_collision_exception(body_b->get_rid(), body_a->get_rid());
	} else {
		physics_server->body_remove_collision_exception(body_a->get_rid(), body_b->get_rid());
		physics_server->body_remove_collision_exception(body_b->get_rid(), body_a->get_rid());
	}
}

void JoltJoint3D::clear() {
	destroy();

	if (body_a != nullptr) {
		body_a->remove_joint(this);
		body_a = nullptr;
	}

	if (body_b != nullptr) {
		body_b->remove_joint(this);
		body_b = nullptr;
	}

	joint_type = PS3DE::JOINT_TYPE_MAX;
	enabled = true;
	collision_disabled = false;
	velocity_iterations = 0;
	position_iterations = 0;
	solver_priority = 1;
	local_ref_a = Transform3D();
	local_ref_b = Transform3D();

	hinge = HingeData();
	slider = SliderData();
	cone_twist = ConeTwistData();
	g6dof = G6DOFData();
}

void JoltJoint3D::make_pin(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Vector3 &p_local_a, const Vector3 &p_local_b) {
	_reconfigure(PS3DE::JOINT_TYPE_PIN, p_body_a, p_body_b, Transform3D({}, p_local_a), Transform3D({}, p_local_b));
}

void JoltJoint3D::make_hinge(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_HINGE, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_slider(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_SLIDER, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_cone_twist(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_CONE_TWIST, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::make_generic_6dof(JoltBody3D *p_body_a, JoltBody3D *p_body_b, const Transform3D &p_local_ref_a, const Transform3D &p_local_ref_b) {
	_reconfigure(PS3DE::JOINT_TYPE_6DOF, p_body_a, p_body_b, p_local_ref_a, p_local_ref_b);
}

void JoltJoint3D::destroy() {
	if (jolt_ref == nullptr) {
		return;
	}

	JoltSpace3D *space = get_space();

	if (space != nullptr) {
		space->remove_joint(this);
	}

	jolt_ref = nullptr;
}

void JoltJoint3D::rebuild() {
	switch (joint_type) {
		case PS3DE::JOINT_TYPE_PIN:
			_rebuild_pin();
			break;
		case PS3DE::JOINT_TYPE_HINGE:
			_rebuild_hinge();
			break;
		case PS3DE::JOINT_TYPE_SLIDER:
			_rebuild_slider();
			break;
		case PS3DE::JOINT_TYPE_CONE_TWIST:
			_rebuild_cone_twist();
			break;
		case PS3DE::JOINT_TYPE_6DOF:
			_rebuild_6dof();
			break;
		default:
			break;
	}
}

void JoltJoint3D::_shift_reference_frames(const Vector3 &p_linear_shift, const Vector3 &p_angular_shift, Transform3D &r_shifted_ref_a, Transform3D &r_shifted_ref_b) {
}

JPH::Constraint *JoltJoint3D::_build_fixed(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) {
	JPH::FixedConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mAutoDetectPoint = false;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mAxisX1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mAxisX2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Y));

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

float JoltJoint3D::get_applied_force() const {
	ERR_FAIL_NULL_V(jolt_ref, 0.0f);

	JoltSpace3D *space = get_space();
	ERR_FAIL_NULL_V(space, 0.0f);

	const float last_step = space->get_last_step();
	if (unlikely(last_step == 0.0f)) {
		return 0.0f;
	}

	switch (joint_type) {
		case PS3DE::JOINT_TYPE_PIN: {
			JPH::PointConstraint *constraint = static_cast<JPH::PointConstraint *>(jolt_ref.GetPtr());
			return constraint->GetTotalLambdaPosition().Length() / last_step;
		}
		case PS3DE::JOINT_TYPE_HINGE: {
			if (_is_hinge_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaPosition().Length() / last_step;
			} else {
				JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr());
				const JPH::Vec3 total_lambda = JPH::Vec3(
						constraint->GetTotalLambdaRotation()[0],
						constraint->GetTotalLambdaRotation()[1],
						constraint->GetTotalLambdaRotationLimits() + constraint->GetTotalLambdaMotor());
				return total_lambda.Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_SLIDER: {
			if (_is_slider_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaPosition().Length() / last_step;
			} else {
				JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr());
				const JPH::Vec3 total_lambda = JPH::Vec3(
						constraint->GetTotalLambdaPosition()[0],
						constraint->GetTotalLambdaPosition()[1],
						constraint->GetTotalLambdaPositionLimits() + constraint->GetTotalLambdaMotor());
				return total_lambda.Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_CONE_TWIST: {
			JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr());
			return constraint->GetTotalLambdaPosition().Length() / last_step;
		}
		case PS3DE::JOINT_TYPE_6DOF: {
			JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 total_lambda = constraint->GetTotalLambdaPosition() + constraint->GetTotalLambdaMotorTranslation();
			return total_lambda.Length() / last_step;
		}
		default:
			return 0.0f;
	}
}

float JoltJoint3D::get_applied_torque() const {
	ERR_FAIL_NULL_V(jolt_ref, 0.0f);

	JoltSpace3D *space = get_space();
	ERR_FAIL_NULL_V(space, 0.0f);

	const float last_step = space->get_last_step();
	if (unlikely(last_step == 0.0f)) {
		return 0.0f;
	}

	switch (joint_type) {
		case PS3DE::JOINT_TYPE_HINGE: {
			if (_is_hinge_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			} else {
				JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_SLIDER: {
			if (_is_slider_fixed()) {
				JPH::FixedConstraint *constraint = static_cast<JPH::FixedConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			} else {
				JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr());
				return constraint->GetTotalLambdaRotation().Length() / last_step;
			}
		}
		case PS3DE::JOINT_TYPE_CONE_TWIST: {
			JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 swing_twist_lambda = JPH::Vec3(
					constraint->GetTotalLambdaTwist(),
					constraint->GetTotalLambdaSwingY(),
					constraint->GetTotalLambdaSwingZ());
			const float total_lambda = swing_twist_lambda.Length() + constraint->GetTotalLambdaMotor().Length();
			return total_lambda / last_step;
		}
		case PS3DE::JOINT_TYPE_6DOF: {
			JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
			const JPH::Vec3 total_lambda = constraint->GetTotalLambdaRotation() + constraint->GetTotalLambdaMotorRotation();
			return total_lambda.Length() / last_step;
		}
		default:
			return 0.0f;
	}
}

// --- Pin Joint ---

JPH::Constraint *JoltJoint3D::_build_pin(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) {
	JPH::PointConstraintSettings constraint_settings;
	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_pin_points_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::set_pin_local_a(const Vector3 &p_local_a) {
	local_ref_a = Transform3D({}, p_local_a);
	_pin_points_changed();
}

void JoltJoint3D::set_pin_local_b(const Vector3 &p_local_b) {
	local_ref_b = Transform3D({}, p_local_b);
	_pin_points_changed();
}

double JoltJoint3D::get_pin_param(PS3DE::PinJointParam p_param) const {
	switch (p_param) {
		case PS3DE::PIN_JOINT_BIAS: {
			return PIN_DEFAULT_BIAS;
		}
		case PS3DE::PIN_JOINT_DAMPING: {
			return PIN_DEFAULT_DAMPING;
		}
		case PS3DE::PIN_JOINT_IMPULSE_CLAMP: {
			return PIN_DEFAULT_IMPULSE_CLAMP;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled pin joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_pin_param(PS3DE::PinJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::PIN_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Pin joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::PIN_JOINT_DAMPING: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_DAMPING)) {
				WARN_PRINT(vformat("Pin joint damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::PIN_JOINT_IMPULSE_CLAMP: {
			if (!Math::is_equal_approx(p_value, PIN_DEFAULT_IMPULSE_CLAMP)) {
				WARN_PRINT(vformat("Pin joint impulse clamp is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled pin joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

void JoltJoint3D::_rebuild_pin() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_pin(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);

	space->add_joint(this);

	_update_joint();
}

// --- Hinge Joint ---

JPH::Constraint *JoltJoint3D::_build_hinge(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const {
	JPH::HingeConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mHingeAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mNormalAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mHingeAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mNormalAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mLimitsMin = -p_limit;
	constraint_settings.mLimitsMax = p_limit;

	if (hinge.limit_spring_enabled) {
		constraint_settings.mLimitsSpringSettings.mFrequency = (float)hinge.limit_spring_frequency;
		constraint_settings.mLimitsSpringSettings.mDamping = (float)hinge.limit_spring_damping;
	}

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_hinge_motor_state() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetMotorState(hinge.motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_hinge_motor_velocity() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocity((float)-hinge.motor_target_speed);
	}
}

void JoltJoint3D::_update_hinge_motor_limit() {
	if (unlikely(_is_hinge_fixed())) {
		return;
	}

	if (JPH::HingeConstraint *constraint = static_cast<JPH::HingeConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-hinge.motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)hinge.motor_max_torque;
	}
}

void JoltJoint3D::_hinge_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_limit_spring_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_state_changed() {
	_update_hinge_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_speed_changed() {
	_update_hinge_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_hinge_motor_limit_changed() {
	_update_hinge_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_hinge_param(PS3DE::HingeJointParam p_param) const {
	switch (p_param) {
		case PS3DE::HINGE_JOINT_BIAS: {
			return HINGE_DEFAULT_BIAS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_UPPER: {
			return hinge.limit_upper;
		}
		case PS3DE::HINGE_JOINT_LIMIT_LOWER: {
			return hinge.limit_lower;
		}
		case PS3DE::HINGE_JOINT_LIMIT_BIAS: {
			return HINGE_DEFAULT_LIMIT_BIAS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_SOFTNESS: {
			return HINGE_DEFAULT_SOFTNESS;
		}
		case PS3DE::HINGE_JOINT_LIMIT_RELAXATION: {
			return HINGE_DEFAULT_RELAXATION;
		}
		case PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY: {
			return hinge.motor_target_speed;
		}
		case PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE: {
			return hinge.motor_max_torque * estimate_physics_step();
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_hinge_param(PS3DE::HingeJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::HINGE_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Hinge joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_UPPER: {
			hinge.limit_upper = p_value;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_LOWER: {
			hinge.limit_lower = p_value;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_BIAS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_LIMIT_BIAS)) {
				WARN_PRINT(vformat("Hinge joint bias limit is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_SOFTNESS)) {
				WARN_PRINT(vformat("Hinge joint softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_LIMIT_RELAXATION: {
			if (!Math::is_equal_approx(p_value, HINGE_DEFAULT_RELAXATION)) {
				WARN_PRINT(vformat("Hinge joint relaxation is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY: {
			hinge.motor_target_speed = p_value;
			_hinge_motor_speed_changed();
		} break;
		case PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE: {
			hinge.motor_max_torque = p_value / estimate_physics_step();
			_hinge_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_FREQUENCY: {
			return hinge.limit_spring_frequency;
		}
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_DAMPING: {
			return hinge.limit_spring_damping;
		}
		case JoltPhysicsServer3D::HINGE_JOINT_MOTOR_MAX_TORQUE: {
			return hinge.motor_max_torque;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_hinge_jolt_param(JoltPhysicsServer3D::HingeJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_FREQUENCY: {
			hinge.limit_spring_frequency = p_value;
			_hinge_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::HINGE_JOINT_LIMIT_SPRING_DAMPING: {
			hinge.limit_spring_damping = p_value;
			_hinge_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::HINGE_JOINT_MOTOR_MAX_TORQUE: {
			hinge.motor_max_torque = p_value;
			_hinge_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_hinge_flag(PS3DE::HingeJointFlag p_flag) const {
	switch (p_flag) {
		case PS3DE::HINGE_JOINT_FLAG_USE_LIMIT: {
			return hinge.limits_enabled;
		}
		case PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR: {
			return hinge.motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_hinge_flag(PS3DE::HingeJointFlag p_flag, bool p_enabled) {
	switch (p_flag) {
		case PS3DE::HINGE_JOINT_FLAG_USE_LIMIT: {
			hinge.limits_enabled = p_enabled;
			_hinge_limits_changed();
		} break;
		case PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR: {
			hinge.motor_enabled = p_enabled;
			_hinge_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

bool JoltJoint3D::get_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT_SPRING: {
			return hinge.limit_spring_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_hinge_jolt_flag(JoltPhysicsServer3D::HingeJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT_SPRING: {
			hinge.limit_spring_enabled = p_enabled;
			_hinge_limit_spring_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_hinge() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	float ref_shift = 0.0f;
	float limit = JPH::JPH_PI;

	if (hinge.limits_enabled && hinge.limit_lower <= hinge.limit_upper) {
		const double limit_midpoint = (hinge.limit_lower + hinge.limit_upper) / 2.0f;

		ref_shift = float(-limit_midpoint);
		limit = float(hinge.limit_upper - limit_midpoint);
	}

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(0.0f, 0.0f, ref_shift), shifted_ref_a, shifted_ref_b);

	if (_is_hinge_fixed()) {
		jolt_ref = _build_fixed(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);
	} else {
		jolt_ref = _build_hinge(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, limit);
	}

	space->add_joint(this);

	_update_joint();

	_update_hinge_motor_state();
	_update_hinge_motor_velocity();
	_update_hinge_motor_limit();
}

// --- Slider Joint ---

JPH::Constraint *JoltJoint3D::_build_slider(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_limit) const {
	JPH::SliderConstraintSettings constraint_settings;

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mAutoDetectPoint = false;
	constraint_settings.mPoint1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mSliderAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mNormalAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mPoint2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mSliderAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mNormalAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mLimitsMin = -p_limit;
	constraint_settings.mLimitsMax = p_limit;

	if (slider.limit_spring_enabled) {
		constraint_settings.mLimitsSpringSettings.mFrequency = (float)slider.limit_spring_frequency;
		constraint_settings.mLimitsSpringSettings.mDamping = (float)slider.limit_spring_damping;
	}

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_slider_motor_state() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetMotorState(slider.motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_slider_motor_velocity() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetTargetVelocity((float)slider.motor_target_speed);
	}
}

void JoltJoint3D::_update_slider_motor_limit() {
	if (unlikely(_is_slider_fixed())) {
		return;
	}

	if (JPH::SliderConstraint *constraint = static_cast<JPH::SliderConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetMotorSettings();
		motor_settings.mMinForceLimit = (float)-slider.motor_max_force;
		motor_settings.mMaxForceLimit = (float)slider.motor_max_force;
	}
}

void JoltJoint3D::_slider_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_limit_spring_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_state_changed() {
	_update_slider_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_speed_changed() {
	_update_slider_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_slider_motor_limit_changed() {
	_update_slider_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_slider_param(PS3DE::SliderJointParam p_param) const {
	switch (p_param) {
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER: {
			return slider.limit_upper;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER: {
			return slider.limit_lower;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_MOTION_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING: {
			return SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION;
		}
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING: {
			return SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled slider joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_slider_param(PS3DE::SliderJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER: {
			slider.limit_upper = p_value;
			_slider_limits_changed();
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER: {
			slider.limit_lower = p_value;
			_slider_limits_changed();
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear limit restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_LIMIT_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear limit damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear motion softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear motion restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_MOTION_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear motion damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint linear ortho softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint linear ortho restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_LINEAR_ORTHO_DAMPING)) {
				WARN_PRINT(vformat("Slider joint linear ortho damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_UPPER)) {
				WARN_PRINT(vformat("Slider joint angular limits are not supported when using Jolt Physics. Any such value will be ignored. Try using Generic6DOFJoint3D instead. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_LOWER)) {
				WARN_PRINT(vformat("Slider joint angular limits are not supported when using Jolt Physics. Any such value will be ignored. Try using Generic6DOFJoint3D instead. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular limit restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_LIMIT_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular limit damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular motion softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular motion restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_MOTION_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular motion damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_SOFTNESS)) {
				WARN_PRINT(vformat("Slider joint angular ortho softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_RESTITUTION)) {
				WARN_PRINT(vformat("Slider joint angular ortho restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING: {
			if (!Math::is_equal_approx(p_value, SLIDER_DEFAULT_ANGULAR_ORTHO_DAMPING)) {
				WARN_PRINT(vformat("Slider joint angular ortho damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled slider joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_FREQUENCY: {
			return slider.limit_spring_frequency;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_DAMPING: {
			return slider.limit_spring_damping;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_TARGET_VELOCITY: {
			return slider.motor_target_speed;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_MAX_FORCE: {
			return slider.motor_max_force;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_slider_jolt_param(JoltPhysicsServer3D::SliderJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_FREQUENCY: {
			slider.limit_spring_frequency = p_value;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_LIMIT_SPRING_DAMPING: {
			slider.limit_spring_damping = p_value;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_TARGET_VELOCITY: {
			slider.motor_target_speed = p_value;
			_slider_motor_speed_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_MOTOR_MAX_FORCE: {
			slider.motor_max_force = p_value;
			_slider_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT: {
			return slider.limits_enabled;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT_SPRING: {
			return slider.limit_spring_enabled;
		}
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_ENABLE_MOTOR: {
			return slider.motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_slider_jolt_flag(JoltPhysicsServer3D::SliderJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT: {
			slider.limits_enabled = p_enabled;
			_slider_limits_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_USE_LIMIT_SPRING: {
			slider.limit_spring_enabled = p_enabled;
			_slider_limit_spring_changed();
		} break;
		case JoltPhysicsServer3D::SLIDER_JOINT_FLAG_ENABLE_MOTOR: {
			slider.motor_enabled = p_enabled;
			_slider_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_slider() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	float ref_shift = 0.0f;
	float limit = FLT_MAX;

	if (slider.limits_enabled && slider.limit_lower <= slider.limit_upper) {
		const double limit_midpoint = (slider.limit_lower + slider.limit_upper) / 2.0f;

		ref_shift = float(-limit_midpoint);
		limit = float(slider.limit_upper - limit_midpoint);
	}

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(ref_shift, 0.0f, 0.0f), Vector3(), shifted_ref_a, shifted_ref_b);

	if (_is_slider_fixed()) {
		jolt_ref = _build_fixed(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);
	} else {
		jolt_ref = _build_slider(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, limit);
	}

	space->add_joint(this);

	_update_joint();

	_update_slider_motor_state();
	_update_slider_motor_velocity();
	_update_slider_motor_limit();
}

// --- Cone Twist Joint ---

JPH::Constraint *JoltJoint3D::_build_swing_twist(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b, float p_swing_limit_span, float p_twist_limit_span) const {
	JPH::SwingTwistConstraintSettings constraint_settings;

	const bool twist_span_valid = p_twist_limit_span >= 0 && p_twist_limit_span <= JPH::JPH_PI;
	const bool swing_span_valid = p_swing_limit_span >= 0 && p_swing_limit_span <= JPH::JPH_PI;

	if (cone_twist.twist_limit_enabled && twist_span_valid) {
		constraint_settings.mTwistMinAngle = -p_twist_limit_span;
		constraint_settings.mTwistMaxAngle = p_twist_limit_span;
	} else {
		constraint_settings.mTwistMinAngle = -JPH::JPH_PI;
		constraint_settings.mTwistMaxAngle = JPH::JPH_PI;
	}

	if (cone_twist.swing_limit_enabled && swing_span_valid) {
		constraint_settings.mNormalHalfConeAngle = p_swing_limit_span;
		constraint_settings.mPlaneHalfConeAngle = p_swing_limit_span;
	} else {
		constraint_settings.mNormalHalfConeAngle = JPH::JPH_PI;
		constraint_settings.mPlaneHalfConeAngle = JPH::JPH_PI;

		if (!swing_span_valid) {
			constraint_settings.mTwistMinAngle = -JPH::JPH_PI;
			constraint_settings.mTwistMaxAngle = JPH::JPH_PI;
		}
	}

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPosition1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mTwistAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPlaneAxis1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Z));
	constraint_settings.mPosition2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mTwistAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mPlaneAxis2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Z));

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_swing_motor_state() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetSwingMotorState(cone_twist.swing_motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_twist_motor_state() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		constraint->SetTwistMotorState(cone_twist.twist_motor_enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
	}
}

void JoltJoint3D::_update_cone_twist_motor_velocity() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocityCS({ (float)-cone_twist.twist_motor_target_speed,
				(float)-cone_twist.swing_motor_target_speed_y,
				(float)-cone_twist.swing_motor_target_speed_z });
	}
}

void JoltJoint3D::_update_swing_motor_limit() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetSwingMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-cone_twist.swing_motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)cone_twist.swing_motor_max_torque;
	}
}

void JoltJoint3D::_update_twist_motor_limit() {
	if (JPH::SwingTwistConstraint *constraint = static_cast<JPH::SwingTwistConstraint *>(jolt_ref.GetPtr())) {
		JPH::MotorSettings &motor_settings = constraint->GetTwistMotorSettings();
		motor_settings.mMinTorqueLimit = (float)-cone_twist.twist_motor_max_torque;
		motor_settings.mMaxTorqueLimit = (float)cone_twist.twist_motor_max_torque;
	}
}

void JoltJoint3D::_cone_twist_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_swing_motor_state_changed() {
	_update_swing_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_twist_motor_state_changed() {
	_update_twist_motor_state();
	_wake_up_bodies();
}

void JoltJoint3D::_cone_twist_motor_velocity_changed() {
	_update_cone_twist_motor_velocity();
	_wake_up_bodies();
}

void JoltJoint3D::_swing_motor_limit_changed() {
	_update_swing_motor_limit();
	_wake_up_bodies();
}

void JoltJoint3D::_twist_motor_limit_changed() {
	_update_twist_motor_limit();
	_wake_up_bodies();
}

double JoltJoint3D::get_cone_twist_param(PS3DE::ConeTwistJointParam p_param) const {
	switch (p_param) {
		case PS3DE::CONE_TWIST_JOINT_SWING_SPAN: {
			return cone_twist.swing_limit_span;
		}
		case PS3DE::CONE_TWIST_JOINT_TWIST_SPAN: {
			return cone_twist.twist_limit_span;
		}
		case PS3DE::CONE_TWIST_JOINT_BIAS: {
			return CONE_TWIST_DEFAULT_BIAS;
		}
		case PS3DE::CONE_TWIST_JOINT_SOFTNESS: {
			return CONE_TWIST_DEFAULT_SOFTNESS;
		}
		case PS3DE::CONE_TWIST_JOINT_RELAXATION: {
			return CONE_TWIST_DEFAULT_RELAXATION;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled cone twist joint parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_cone_twist_param(PS3DE::ConeTwistJointParam p_param, double p_value) {
	switch (p_param) {
		case PS3DE::CONE_TWIST_JOINT_SWING_SPAN: {
			cone_twist.swing_limit_span = p_value;
			_cone_twist_limits_changed();
		} break;
		case PS3DE::CONE_TWIST_JOINT_TWIST_SPAN: {
			cone_twist.twist_limit_span = p_value;
			_cone_twist_limits_changed();
		} break;
		case PS3DE::CONE_TWIST_JOINT_BIAS: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_BIAS)) {
				WARN_PRINT(vformat("Cone twist joint bias is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::CONE_TWIST_JOINT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_SOFTNESS)) {
				WARN_PRINT(vformat("Cone twist joint softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::CONE_TWIST_JOINT_RELAXATION: {
			if (!Math::is_equal_approx(p_value, CONE_TWIST_DEFAULT_RELAXATION)) {
				WARN_PRINT(vformat("Cone twist joint relaxation is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled cone twist joint parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

double JoltJoint3D::get_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param) const {
	switch (p_param) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Y: {
			return cone_twist.swing_motor_target_speed_y;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Z: {
			return cone_twist.swing_motor_target_speed_z;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_TARGET_VELOCITY: {
			return cone_twist.twist_motor_target_speed;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_MAX_TORQUE: {
			return cone_twist.swing_motor_max_torque;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_MAX_TORQUE: {
			return cone_twist.twist_motor_max_torque;
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_cone_twist_jolt_param(JoltPhysicsServer3D::ConeTwistJointParamJolt p_param, double p_value) {
	switch (p_param) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Y: {
			cone_twist.swing_motor_target_speed_y = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_TARGET_VELOCITY_Z: {
			cone_twist.swing_motor_target_speed_z = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_TARGET_VELOCITY: {
			cone_twist.twist_motor_target_speed = p_value;
			_cone_twist_motor_velocity_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_SWING_MOTOR_MAX_TORQUE: {
			cone_twist.swing_motor_max_torque = p_value;
			_swing_motor_limit_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_TWIST_MOTOR_MAX_TORQUE: {
			cone_twist.twist_motor_max_torque = p_value;
			_twist_motor_limit_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag) const {
	switch (p_flag) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_SWING_LIMIT: {
			return cone_twist.swing_limit_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_TWIST_LIMIT: {
			return cone_twist.twist_limit_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_SWING_MOTOR: {
			return cone_twist.swing_motor_enabled;
		}
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_TWIST_MOTOR: {
			return cone_twist.twist_motor_enabled;
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_cone_twist_jolt_flag(JoltPhysicsServer3D::ConeTwistJointFlagJolt p_flag, bool p_enabled) {
	switch (p_flag) {
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_SWING_LIMIT: {
			cone_twist.swing_limit_enabled = p_enabled;
			_cone_twist_limits_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_USE_TWIST_LIMIT: {
			cone_twist.twist_limit_enabled = p_enabled;
			_cone_twist_limits_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_SWING_MOTOR: {
			cone_twist.swing_motor_enabled = p_enabled;
			_swing_motor_state_changed();
		} break;
		case JoltPhysicsServer3D::CONE_TWIST_JOINT_FLAG_ENABLE_TWIST_MOTOR: {
			cone_twist.twist_motor_enabled = p_enabled;
			_twist_motor_state_changed();
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_cone_twist() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_swing_twist(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b, (float)cone_twist.swing_limit_span, (float)cone_twist.twist_limit_span);

	space->add_joint(this);

	_update_joint();

	_update_swing_motor_state();
	_update_twist_motor_state();
	_update_cone_twist_motor_velocity();
	_update_swing_motor_limit();
	_update_twist_motor_limit();
}

// --- Generic 6DOF Joint ---

JPH::Constraint *JoltJoint3D::_build_6dof(JPH::Body *p_jolt_body_a, JPH::Body *p_jolt_body_b, const Transform3D &p_shifted_ref_a, const Transform3D &p_shifted_ref_b) const {
	JPH::SixDOFConstraintSettings constraint_settings;

	for (int axis = 0; axis < AXIS_COUNT; ++axis) {
		double lower = g6dof.limit_lower[axis];
		double upper = g6dof.limit_upper[axis];

		if (axis >= AXIS_ANGULAR_X && axis <= AXIS_ANGULAR_Z) {
			const double temp = lower;
			lower = -upper;
			upper = -temp;
		}

		if (!g6dof.limit_enabled[axis] || lower > upper) {
			constraint_settings.MakeFreeAxis((JoltAxis)axis);
		} else {
			constraint_settings.SetLimitedAxis((JoltAxis)axis, (float)lower, (float)upper);
		}
	}

	constraint_settings.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
	constraint_settings.mPosition1 = to_jolt_r(p_shifted_ref_a.origin);
	constraint_settings.mAxisX1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY1 = to_jolt(p_shifted_ref_a.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mPosition2 = to_jolt_r(p_shifted_ref_b.origin);
	constraint_settings.mAxisX2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_X));
	constraint_settings.mAxisY2 = to_jolt(p_shifted_ref_b.basis.get_column(Vector3::AXIS_Y));
	constraint_settings.mSwingType = JPH::ESwingType::Pyramid;

	if (p_jolt_body_a == nullptr) {
		return constraint_settings.Create(JPH::Body::sFixedToWorld, *p_jolt_body_b);
	} else if (p_jolt_body_b == nullptr) {
		return constraint_settings.Create(*p_jolt_body_a, JPH::Body::sFixedToWorld);
	} else {
		return constraint_settings.Create(*p_jolt_body_a, *p_jolt_body_b);
	}
}

void JoltJoint3D::_update_6dof_limit_spring_parameters(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::SpringSettings settings = constraint->GetLimitsSpringSettings((JoltAxis)p_axis);

	settings.mMode = JPH::ESpringMode::FrequencyAndDamping;

	if (g6dof.limit_spring_enabled[p_axis]) {
		settings.mFrequency = (float)g6dof.limit_spring_frequency[p_axis];
		settings.mDamping = (float)g6dof.limit_spring_damping[p_axis];
	} else {
		settings.mFrequency = 0.0f;
		settings.mDamping = 0.0f;
	}

	constraint->SetLimitsSpringSettings((JoltAxis)p_axis, settings);
}

void JoltJoint3D::_update_6dof_motor_state(int p_axis) {
	if (JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr())) {
		if (g6dof.motor_enabled[p_axis]) {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Velocity);
		} else if (g6dof.spring_enabled[p_axis]) {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Position);
		} else {
			constraint->SetMotorState((JoltAxis)p_axis, JPH::EMotorState::Off);
		}
	}
}

void JoltJoint3D::_update_6dof_motor_velocity(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		constraint->SetTargetVelocityCS(JPH::Vec3(
				(float)g6dof.motor_speed[AXIS_LINEAR_X],
				(float)g6dof.motor_speed[AXIS_LINEAR_Y],
				(float)g6dof.motor_speed[AXIS_LINEAR_Z]));
	} else {
		// We flip the direction since Jolt is CCW but Godot is CW.
		constraint->SetTargetAngularVelocityCS(JPH::Vec3(
				(float)-g6dof.motor_speed[AXIS_ANGULAR_X],
				(float)-g6dof.motor_speed[AXIS_ANGULAR_Y],
				(float)-g6dof.motor_speed[AXIS_ANGULAR_Z]));
	}
}

void JoltJoint3D::_update_6dof_motor_limit(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::MotorSettings &motor_settings = constraint->GetMotorSettings((JoltAxis)p_axis);

	float limit = FLT_MAX;

	if (g6dof.drive_limit_set[p_axis]) {
		limit = (float)g6dof.drive_limit[p_axis];
	} else if (g6dof.motor_enabled[p_axis]) {
		limit = (float)g6dof.motor_limit[p_axis];
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		motor_settings.SetForceLimit(limit);
	} else {
		motor_settings.SetTorqueLimit(limit);
	}
}

void JoltJoint3D::_update_6dof_spring_parameters(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	JPH::MotorSettings &motor_settings = constraint->GetMotorSettings((JoltAxis)p_axis);

	if (g6dof.spring_use_frequency[p_axis]) {
		motor_settings.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		motor_settings.mSpringSettings.mFrequency = (float)g6dof.spring_frequency[p_axis];
	} else {
		motor_settings.mSpringSettings.mMode = JPH::ESpringMode::StiffnessAndDamping;
		motor_settings.mSpringSettings.mStiffness = (float)g6dof.spring_stiffness[p_axis];
	}

	motor_settings.mSpringSettings.mDamping = (float)g6dof.spring_damping[p_axis];
}

void JoltJoint3D::_update_6dof_spring_equilibrium(int p_axis) {
	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	if (p_axis >= AXIS_LINEAR_X && p_axis <= AXIS_LINEAR_Z) {
		const Vector3 target_position = Vector3(
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_X],
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_Y],
				(float)g6dof.spring_equilibrium[AXIS_LINEAR_Z]);

		constraint->SetTargetPositionCS(to_jolt(target_position));
	} else {
		// We flip the direction since Jolt is CCW but Godot is CW.
		const Basis target_orientation = Basis::from_euler(
				Vector3((float)-g6dof.spring_equilibrium[AXIS_ANGULAR_X],
						(float)-g6dof.spring_equilibrium[AXIS_ANGULAR_Y],
						(float)-g6dof.spring_equilibrium[AXIS_ANGULAR_Z]),
				EulerOrder::ZYX);

		constraint->SetTargetOrientationCS(to_jolt(target_orientation));
	}
}

void JoltJoint3D::_update_6dof_angular_target_rotation() {
	if (!g6dof.has_angular_target_rotation) {
		return;
	}

	JPH::SixDOFConstraint *constraint = static_cast<JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return;
	}

	const JPH::Quat target_orientation = to_jolt(g6dof.angular_target_rotation);
	constraint->SetTargetOrientationBS(target_orientation);
}

void JoltJoint3D::_6dof_limits_changed() {
	rebuild();
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_limit_spring_parameters_changed(int p_axis) {
	_update_6dof_limit_spring_parameters(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_state_changed(int p_axis) {
	_update_6dof_motor_state(p_axis);
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_speed_changed(int p_axis) {
	_update_6dof_motor_velocity(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_motor_limit_changed(int p_axis) {
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_state_changed(int p_axis) {
	_update_6dof_motor_state(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_parameters_changed(int p_axis) {
	_update_6dof_spring_parameters(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_spring_equilibrium_changed(int p_axis) {
	_update_6dof_spring_equilibrium(p_axis);
	_wake_up_bodies();
}

void JoltJoint3D::_6dof_drive_limit_changed(int p_axis) {
	_update_6dof_motor_limit(p_axis);
	_wake_up_bodies();
}

double JoltJoint3D::get_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case PS3DE::G6DOF_JOINT_LINEAR_LOWER_LIMIT: {
			return g6dof.limit_lower[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_UPPER_LIMIT: {
			return g6dof.limit_upper[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS: {
			return G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION: {
			return G6DOF_DEFAULT_LINEAR_RESTITUTION;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_DAMPING: {
			return G6DOF_DEFAULT_LINEAR_DAMPING;
		}
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_TARGET_VELOCITY: {
			return g6dof.motor_speed[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_FORCE_LIMIT: {
			return g6dof.motor_limit[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_STIFFNESS: {
			return g6dof.spring_stiffness[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_DAMPING: {
			return g6dof.spring_damping[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT: {
			return g6dof.spring_equilibrium[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT: {
			return g6dof.drive_limit[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_LOWER_LIMIT: {
			return g6dof.limit_lower[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_UPPER_LIMIT: {
			return g6dof.limit_upper[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			return G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_DAMPING: {
			return G6DOF_DEFAULT_ANGULAR_DAMPING;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_RESTITUTION: {
			return G6DOF_DEFAULT_ANGULAR_RESTITUTION;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_FORCE_LIMIT: {
			return G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_ERP: {
			return G6DOF_DEFAULT_ANGULAR_ERP;
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY: {
			return g6dof.motor_speed[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT: {
			return g6dof.motor_limit[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS: {
			return g6dof.spring_stiffness[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_DAMPING: {
			return g6dof.spring_damping[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT: {
			return g6dof.spring_equilibrium[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT: {
			return g6dof.drive_limit[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_6dof_param(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisParam p_param, double p_value) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case PS3DE::G6DOF_JOINT_LINEAR_LOWER_LIMIT: {
			g6dof.limit_lower[axis_lin] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_UPPER_LIMIT: {
			g6dof.limit_upper[axis_lin] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("6DOF joint linear limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_RESTITUTION)) {
				WARN_PRINT(vformat("6DOF joint linear restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_DAMPING: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_LINEAR_DAMPING)) {
				WARN_PRINT(vformat("6DOF joint linear damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_TARGET_VELOCITY: {
			g6dof.motor_speed[axis_lin] = p_value;
			_6dof_motor_speed_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_MOTOR_FORCE_LIMIT: {
			g6dof.motor_limit[axis_lin] = p_value;
			_6dof_motor_limit_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_STIFFNESS: {
			g6dof.spring_stiffness[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_DAMPING: {
			g6dof.spring_damping[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_SPRING_EQUILIBRIUM_POINT: {
			g6dof.spring_equilibrium[axis_lin] = p_value;
			_6dof_spring_equilibrium_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT: {
			g6dof.drive_limit[axis_lin] = p_value;
			g6dof.drive_limit_set[axis_lin] = true;
			_6dof_drive_limit_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_LOWER_LIMIT: {
			g6dof.limit_lower[axis_ang] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_UPPER_LIMIT: {
			g6dof.limit_upper[axis_ang] = p_value;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_LIMIT_SOFTNESS)) {
				WARN_PRINT(vformat("6DOF joint angular limit softness is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_DAMPING: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_DAMPING)) {
				WARN_PRINT(vformat("6DOF joint angular damping is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_RESTITUTION: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_RESTITUTION)) {
				WARN_PRINT(vformat("6DOF joint angular restitution is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_FORCE_LIMIT: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_FORCE_LIMIT)) {
				WARN_PRINT(vformat("6DOF joint angular force limit is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_ERP: {
			if (!Math::is_equal_approx(p_value, G6DOF_DEFAULT_ANGULAR_ERP)) {
				WARN_PRINT(vformat("6DOF joint angular ERP is not supported when using Jolt Physics. Any such value will be ignored. This joint connects %s.", _bodies_to_string()));
			}
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY: {
			g6dof.motor_speed[axis_ang] = p_value;
			_6dof_motor_speed_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT: {
			g6dof.motor_limit[axis_ang] = p_value;
			_6dof_motor_limit_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_STIFFNESS: {
			g6dof.spring_stiffness[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_DAMPING: {
			g6dof.spring_damping[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT: {
			g6dof.spring_equilibrium[axis_ang] = p_value;
			g6dof.has_angular_target_rotation = false;
			_6dof_spring_equilibrium_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT: {
			g6dof.drive_limit[axis_ang] = p_value;
			g6dof.drive_limit_set[axis_ang] = true;
			_6dof_drive_limit_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT: {
			return g6dof.limit_enabled[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT: {
			return g6dof.limit_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING: {
			return g6dof.spring_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING: {
			return g6dof.spring_enabled[axis_lin];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR: {
			return g6dof.motor_enabled[axis_ang];
		}
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_MOTOR: {
			return g6dof.motor_enabled[axis_lin];
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_6dof_flag(Vector3::Axis p_axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enabled) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT: {
			g6dof.limit_enabled[axis_lin] = p_enabled;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT: {
			g6dof.limit_enabled[axis_ang] = p_enabled;
			_6dof_limits_changed();
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING: {
			g6dof.spring_enabled[axis_ang] = p_enabled;
			_6dof_spring_state_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING: {
			g6dof.spring_enabled[axis_lin] = p_enabled;
			_6dof_spring_state_changed(axis_lin);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_MOTOR: {
			g6dof.motor_enabled[axis_ang] = p_enabled;
			_6dof_motor_state_changed(axis_ang);
		} break;
		case PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_MOTOR: {
			g6dof.motor_enabled[axis_lin] = p_enabled;
			_6dof_motor_state_changed(axis_lin);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::set_6dof_angular_target_rotation(const Quaternion &p_target_rotation) {
	ERR_FAIL_COND_MSG(!p_target_rotation.is_finite() || p_target_rotation.length_squared() <= CMP_EPSILON, "Angular target rotation must be a finite, non-zero quaternion.");

	g6dof.angular_target_rotation = p_target_rotation.normalized();
	g6dof.has_angular_target_rotation = true;
	_update_6dof_angular_target_rotation();
	_wake_up_bodies();
}

Quaternion JoltJoint3D::get_6dof_angular_target_rotation() const {
	if (g6dof.has_angular_target_rotation) {
		return g6dof.angular_target_rotation;
	}

	const JPH::SixDOFConstraint *constraint = static_cast<const JPH::SixDOFConstraint *>(jolt_ref.GetPtr());
	if (unlikely(constraint == nullptr)) {
		return Quaternion();
	}

	const Basis target_orientation_cs = Basis::from_euler(
			Vector3(static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_X]),
					static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_Y]),
					static_cast<real_t>(-g6dof.spring_equilibrium[AXIS_ANGULAR_Z])),
			EulerOrder::ZYX);
	const Basis constraint_to_body_1 = to_godot(constraint->GetConstraintToBody1Matrix()).basis;
	const Basis constraint_to_body_2 = to_godot(constraint->GetConstraintToBody2Matrix()).basis;

	return (constraint_to_body_1 * target_orientation_cs * constraint_to_body_2.inverse()).get_quaternion();
}

double JoltJoint3D::get_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_SPRING_FREQUENCY: {
			return g6dof.spring_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_FREQUENCY: {
			return g6dof.limit_spring_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_DAMPING: {
			return g6dof.limit_spring_damping[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_ANGULAR_SPRING_FREQUENCY: {
			return g6dof.spring_frequency[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltJoint3D::set_6dof_jolt_param(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisParamJolt p_param, double p_value) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_param) {
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_SPRING_FREQUENCY: {
			g6dof.spring_frequency[axis_lin] = p_value;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_FREQUENCY: {
			g6dof.limit_spring_frequency[axis_lin] = p_value;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_LINEAR_LIMIT_SPRING_DAMPING: {
			g6dof.limit_spring_damping[axis_lin] = p_value;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_ANGULAR_SPRING_FREQUENCY: {
			g6dof.spring_frequency[axis_ang] = p_value;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

bool JoltJoint3D::get_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag) const {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT_SPRING: {
			return g6dof.limit_spring_enabled[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING_FREQUENCY: {
			return g6dof.spring_use_frequency[axis_lin];
		}
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING_FREQUENCY: {
			return g6dof.spring_use_frequency[axis_ang];
		}
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		}
	}
}

void JoltJoint3D::set_6dof_jolt_flag(Vector3::Axis p_axis, JoltPhysicsServer3D::G6DOFJointAxisFlagJolt p_flag, bool p_enabled) {
	const int axis_lin = AXES_LINEAR + (int)p_axis;
	const int axis_ang = AXES_ANGULAR + (int)p_axis;

	switch ((int)p_flag) {
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT_SPRING: {
			g6dof.limit_spring_enabled[axis_lin] = p_enabled;
			_6dof_limit_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_LINEAR_SPRING_FREQUENCY: {
			g6dof.spring_use_frequency[axis_lin] = p_enabled;
			_6dof_spring_parameters_changed(axis_lin);
		} break;
		case JoltPhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_SPRING_FREQUENCY: {
			g6dof.spring_use_frequency[axis_ang] = p_enabled;
			_6dof_spring_parameters_changed(axis_ang);
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled flag: '%d'. This should not happen. Please report this.", p_flag));
		} break;
	}
}

void JoltJoint3D::_rebuild_6dof() {
	destroy();

	JoltSpace3D *space = get_space();
	if (space == nullptr) {
		return;
	}

	JPH::Body *jolt_body_a = body_a != nullptr ? body_a->get_jolt_body() : nullptr;
	JPH::Body *jolt_body_b = body_b != nullptr ? body_b->get_jolt_body() : nullptr;
	ERR_FAIL_COND(jolt_body_a == nullptr && jolt_body_b == nullptr);

	Transform3D shifted_ref_a;
	Transform3D shifted_ref_b;

	_shift_reference_frames(Vector3(), Vector3(), shifted_ref_a, shifted_ref_b);

	jolt_ref = _build_6dof(jolt_body_a, jolt_body_b, shifted_ref_a, shifted_ref_b);

	space->add_joint(this);

	_update_joint();

	_update_6dof_limit_spring_parameters(AXIS_LINEAR_X);
	_update_6dof_limit_spring_parameters(AXIS_LINEAR_Y);
	_update_6dof_limit_spring_parameters(AXIS_LINEAR_Z);

	for (int axis = 0; axis < AXIS_COUNT; ++axis) {
		_update_6dof_motor_state(axis);
		_update_6dof_motor_velocity(axis);
		_update_6dof_motor_limit(axis);
		_update_6dof_spring_parameters(axis);
		_update_6dof_spring_equilibrium(axis);
	}

	_update_6dof_angular_target_rotation();
}
