/**************************************************************************/
/*  jolt_area_3d.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "../misc/jolt_math_funcs.h"
#include "../misc/jolt_type_conversions.h"
#include "../shapes/jolt_shape_3d.h"
#include "../spaces/jolt_broad_phase_layer.h"
#include "../spaces/jolt_space_3d.h"
#include "jolt_area_3d.h"
#include "jolt_body_3d.h"
#include "jolt_group_filter.h"
#include "jolt_soft_body_3d.h"

JPH::BroadPhaseLayer JoltArea3D::_get_broad_phase_layer() const
{
	return monitorable ? JoltBroadPhaseLayer::AREA_DETECTABLE
					   : JoltBroadPhaseLayer::AREA_UNDETECTABLE;
}

JPH::ObjectLayer JoltArea3D::_get_object_layer() const
{
	ERR_FAIL_NULL_V(space, 0);

	if (jolt_shape == nullptr || jolt_shape->GetType() == JPH::EShapeType::Empty) {
		// No point doing collision checks against a shapeless object.
		return space->map_to_object_layer(_get_broad_phase_layer(), 0, 0);
	}

	return space->map_to_object_layer(_get_broad_phase_layer(), collision_layer, collision_mask);
}

void JoltArea3D::_enqueue_call_queries()
{
	if (space != nullptr) {
		space->enqueue_call_queries(&call_queries_element);
	}
}

void JoltArea3D::_dequeue_call_queries()
{
	if (space != nullptr) {
		space->dequeue_call_queries(&call_queries_element);
	}
}

void JoltArea3D::_notify_body_entered(const JPH::BodyID& p_body_id)
{
	if (JoltBody3D* body = space->try_get_body(p_body_id)) {
		body->add_area(this);
	}
	else if (JoltSoftBody3D* soft_body = space->try_get_soft_body(p_body_id)) {
		soft_body->add_area(this);
	}
}

void JoltArea3D::_notify_body_exited(const JPH::BodyID& p_body_id)
{
	if (JoltBody3D* body = space->try_get_body(p_body_id)) {
		body->remove_area(this);
	}
	else if (JoltSoftBody3D* soft_body = space->try_get_soft_body(p_body_id)) {
		soft_body->remove_area(this);
	}
}

void JoltArea3D::_update_group_filter()
{
	if (!in_space()) {
		return;
	}

	jolt_body->GetCollisionGroup().SetGroupFilter(JoltGroupFilter::instance);
}

void JoltArea3D::_space_changing()
{
	JoltShapedObject3D::_space_changing();

	_remove_all_overlaps();
	_dequeue_call_queries();
}

void JoltArea3D::_space_changed()
{
	JoltShapedObject3D::_space_changed();

	_update_group_filter();
}

void JoltArea3D::_events_changed() { _enqueue_call_queries(); }

void JoltArea3D::_body_monitoring_changed() { _update_sleeping(); }

void JoltArea3D::_area_monitoring_changed() { _update_sleeping(); }

void JoltArea3D::_monitorable_changed() { _update_object_layer(); }

JoltArea3D::JoltArea3D() : JoltShapedObject3D(OBJECT_TYPE_AREA), call_queries_element(this) {}

void JoltArea3D::set_transform(Transform3D p_transform)
{
	JOLT_ENSURE_SCALE_NOT_ZERO(
		p_transform, vformat("An invalid transform was passed to area '%s'.", to_string()));

	Vector3 new_scale;
	JoltMath::decompose(p_transform, new_scale);

	// Ideally we would do an exact comparison here, but due to floating-point precision this would
	// be invalidated very often.
	if (!scale.is_equal_approx(new_scale)) {
		scale = new_scale;
		_shapes_changed();
	}

	if (!in_space()) {
		jolt_settings->mPosition = to_jolt_r(p_transform.origin);
		jolt_settings->mRotation = to_jolt(p_transform.basis);
	}
	else {
		space->get_body_iface().SetPositionAndRotation(jolt_body->GetID(),
			to_jolt_r(p_transform.origin), to_jolt(p_transform.basis),
			JPH::EActivation::DontActivate);
	}
}

void JoltArea3D::set_monitorable(bool p_monitorable)
{
	if (p_monitorable == monitorable) {
		return;
	}

	monitorable = p_monitorable;

	_monitorable_changed();
}

bool JoltArea3D::can_interact_with(const JoltBody3D& p_other) const { return can_monitor(p_other); }

bool JoltArea3D::can_interact_with(const JoltSoftBody3D& p_other) const
{
	return can_monitor(p_other);
}

bool JoltArea3D::can_interact_with(const JoltArea3D& p_other) const
{
	return can_monitor(p_other) || p_other.can_monitor(*this);
}

void JoltArea3D::set_priority(float p_priority)
{
	if (p_priority == priority) {
		return;
	}

	priority = p_priority;

	_notify_bodies_updated(true);
}

void JoltArea3D::set_gravity(float p_gravity)
{
	if (p_gravity == gravity) {
		return;
	}

	gravity = p_gravity;

	_notify_bodies_updated();
}

void JoltArea3D::set_point_gravity(bool p_enabled)
{
	if (p_enabled == point_gravity) {
		return;
	}

	point_gravity = p_enabled;

	_notify_bodies_updated();
}

void JoltArea3D::set_point_gravity_distance(float p_distance)
{
	if (p_distance == point_gravity_distance) {
		return;
	}

	point_gravity_distance = p_distance;

	_notify_bodies_updated();
}

void JoltArea3D::set_linear_damp(float p_damp)
{
	if (p_damp == linear_damp) {
		return;
	}

	linear_damp = p_damp;

	_notify_bodies_updated();
}

void JoltArea3D::set_angular_damp(float p_damp)
{
	if (p_damp == angular_damp) {
		return;
	}

	angular_damp = p_damp;

	_notify_bodies_updated();
}

void JoltArea3D::set_gravity_mode(OverrideMode p_mode)
{
	if (p_mode == gravity_mode) {
		return;
	}

	gravity_mode = p_mode;

	_notify_bodies_updated();
}

void JoltArea3D::set_linear_damp_mode(OverrideMode p_mode)
{
	if (p_mode == linear_damp_mode) {
		return;
	}

	linear_damp_mode = p_mode;

	_notify_bodies_updated();
}

void JoltArea3D::set_angular_damp_mode(OverrideMode p_mode)
{
	if (p_mode == angular_damp_mode) {
		return;
	}

	angular_damp_mode = p_mode;

	_notify_bodies_updated();
}

void JoltArea3D::set_gravity_vector(const Vector3& p_vector)
{
	if (p_vector == gravity_vector) {
		return;
	}

	gravity_vector = p_vector;

	_notify_bodies_updated();
}

Vector3 JoltArea3D::compute_gravity(const Vector3& p_position) const
{
	if (!point_gravity) {
		return gravity_vector * gravity;
	}

	const Vector3 point = get_transform_scaled().xform(gravity_vector);
	const Vector3 to_point = point - p_position;
	const real_t to_point_dist_sq = MAX(to_point.length_squared(), (real_t)CMP_EPSILON);
	const Vector3 to_point_dir = to_point / Math::sqrt(to_point_dist_sq);

	if (point_gravity_distance == 0.0f) {
		return to_point_dir * gravity;
	}

	const float gravity_dist_sq = point_gravity_distance * point_gravity_distance;

	return to_point_dir * (gravity * gravity_dist_sq / to_point_dist_sq);
}

bool JoltArea3D::shape_exited(const JPH::BodyID& p_body_id, const JPH::SubShapeID& p_other_shape_id,
	const JPH::SubShapeID& p_self_shape_id)
{
	return body_shape_exited(p_body_id, p_other_shape_id, p_self_shape_id) ||
		   area_shape_exited(p_body_id, p_other_shape_id, p_self_shape_id);
}


