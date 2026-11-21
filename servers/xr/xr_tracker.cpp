/**************************************************************************/
/*  xr_tracker.cpp                                                        */
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

#include "xr_tracker.h"

// XRPose

void XRPose::set_has_tracking_data(const bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRPose::get_has_tracking_data() const { return has_tracking_data; }

void XRPose::set_name(const String& p_name) { name = p_name; }

String XRPose::get_name() const { return name; }

void XRPose::set_transform(const Transform3D p_transform) { transform = p_transform; }

Transform3D XRPose::get_transform() const { return transform; }

Transform3D XRPose::get_adjusted_transform() const
{
	Transform3D adjusted_transform = transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform);

	// apply world scale
	adjusted_transform.origin *= XRServer::get_world_scale();

	// apply reference frame
	adjusted_transform = XRServer::get_reference_frame() * adjusted_transform;

	return adjusted_transform;
}

void XRPose::set_linear_velocity(const Vector3 p_velocity) { linear_velocity = p_velocity; }

Vector3 XRPose::get_linear_velocity() const { return linear_velocity; }

void XRPose::set_angular_velocity(const Vector3 p_velocity) { angular_velocity = p_velocity; }

Vector3 XRPose::get_angular_velocity() const { return angular_velocity; }

void XRPose::set_tracking_confidence(const XRPose::TrackingConfidence p_tracking_confidence)
{
	tracking_confidence = p_tracking_confidence;
}

XRPose::TrackingConfidence XRPose::get_tracking_confidence() const { return tracking_confidence; }

// XRTracker

XRTracker::BodyData* XRTracker::_ensure_body_data()
{
	if (!body_data) {
		body_data = memnew(BodyData);
	}
	return body_data;
}

XRTracker::HandData* XRTracker::_ensure_hand_data()
{
	if (!hand_data) {
		hand_data = memnew(HandData);
	}
	return hand_data;
}

void XRTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	type = p_type;
	if (type == XRServer::TRACKER_BODY) {
		_ensure_body_data();
	}
	else if (type == XRServer::TRACKER_HAND) {
		_ensure_hand_data();
		if (tracker_hand == TRACKER_HAND_UNKNOWN) {
			tracker_hand = TRACKER_HAND_LEFT;
		}
	}
}

XRServer::TrackerType XRTracker::get_tracker_type() const { return type; }

void XRTracker::set_tracker_name(const String& p_name)
{
	// Note: this should not be changed after the tracker is registered with the XRServer!
	name = p_name;
}

String XRTracker::get_tracker_name() const { return name; }

void XRTracker::set_tracker_desc(const String& p_desc) { description = p_desc; }

String XRTracker::get_tracker_desc() const { return description; }

void XRTracker::set_tracker_profile(const String& p_profile) { profile = p_profile; }

String XRTracker::get_tracker_profile() const { return profile; }

XRTracker::TrackerHand XRTracker::get_tracker_hand() const { return tracker_hand; }

void XRTracker::set_tracker_hand(const XRTracker::TrackerHand p_hand)
{
	ERR_FAIL_INDEX(p_hand, TRACKER_HAND_MAX);
	if (type == XRServer::TRACKER_BODY) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_UNKNOWN, "Body tracker cannot specify hand.");
	}
	else if (type == XRServer::TRACKER_HAND) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_LEFT && p_hand != TRACKER_HAND_RIGHT,
			"Hand tracker must specify hand.");
	}
	tracker_hand = p_hand;
}

void XRTracker::set_has_tracking_data(bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRTracker::get_has_tracking_data() const { return has_tracking_data; }

bool XRTracker::has_pose(const String& p_action_name) const
{
	return poses.has(p_action_name);
}

Ref<XRPose> XRTracker::get_pose(const String& p_action_name) const
{
	Ref<XRPose> pose;
	if (poses.has(p_action_name)) {
		pose = poses[p_action_name];
	}
	return pose;
}

void XRTracker::invalidate_pose(const String& p_action_name)
{
	if (poses.has(p_action_name)) {
		poses[p_action_name]->set_has_tracking_data(false);
	}
}

void XRTracker::set_pose(const String& p_action_name, const Transform3D& p_transform,
	const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
	const XRPose::TrackingConfidence p_tracking_confidence)
{
	Ref<XRPose> new_pose;
	if (poses.has(p_action_name)) {
		new_pose = poses[p_action_name];
	}
	else {
		new_pose.instantiate();
		new_pose->set_name(p_action_name);
		poses[p_action_name] = new_pose;
	}

	new_pose->set_has_tracking_data(p_tracking_confidence != XRPose::XR_TRACKING_CONFIDENCE_NONE);
	new_pose->set_transform(p_transform);
	new_pose->set_linear_velocity(p_linear_velocity);
	new_pose->set_angular_velocity(p_angular_velocity);
	new_pose->set_tracking_confidence(p_tracking_confidence);
}

void XRTracker::set_input(const String& p_action_name, bool p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_BOOL;
	val.bool_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, float p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_FLOAT;
	val.float_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, const Vector2& p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_VECTOR2;
	val.vec2_val.x = p_value.x;
	val.vec2_val.y = p_value.y;
	inputs[p_action_name] = val;
}

XRTracker::InputValue XRTracker::get_input(const String& p_action_name) const
{
	if (inputs.has(p_action_name)) {
		return inputs[p_action_name];
	}
	return InputValue();
}

bool XRTracker::is_button_pressed(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return false;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val > 0.0f;
	}
	return false;
}

float XRTracker::get_input_float(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return 0.0f;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val ? 1.0f : 0.0f;
	}
	return 0.0f;
}

Vector2 XRTracker::get_input_vector2(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (val && val->type == InputValue::INPUT_VECTOR2) {
		return Vector2(val->vec2_val.x, val->vec2_val.y);
	}
	return Vector2();
}

void XRTracker::set_body_flags(uint32_t p_body_flags)
{
	_ensure_body_data()->body_flags = p_body_flags;
}

uint32_t XRTracker::get_body_flags() const
{
	return body_data ? body_data->body_flags : 0;
}

void XRTracker::set_joint_flags(Joint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_joint_flags(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, uint32_t());
	return body_data ? body_data->joint_flags[p_joint] : 0;
}

void XRTracker::set_joint_transform(Joint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_joint_transform(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, Transform3D());
	return body_data ? body_data->joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_tracking_source(HandTrackingSource p_source)
{
	_ensure_hand_data()->hand_tracking_source = p_source;
}

XRTracker::HandTrackingSource XRTracker::get_hand_tracking_source() const
{
	return hand_data ? hand_data->hand_tracking_source : HAND_TRACKING_SOURCE_UNKNOWN;
}

void XRTracker::set_hand_joint_flags(HandJoint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_hand_joint_flags(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, uint32_t());
	return hand_data ? hand_data->hand_joint_flags[p_joint] : 0;
}

void XRTracker::set_hand_joint_transform(HandJoint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_hand_joint_transform(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Transform3D());
	return hand_data ? hand_data->hand_joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_joint_radius(HandJoint p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_radii[p_joint] = p_radius;
}

float XRTracker::get_hand_joint_radius(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, 0.0f);
	return hand_data ? hand_data->hand_joint_radii[p_joint] : 0.0f;
}

void XRTracker::set_hand_joint_linear_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_linear_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_linear_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_linear_velocities[p_joint] : Vector3();
}

void XRTracker::set_hand_joint_angular_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_angular_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_angular_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_angular_velocities[p_joint] : Vector3();
}

float XRTracker::get_blend_shape(BlendShapeEntry p_blend_shape) const
{
	ERR_FAIL_INDEX_V(p_blend_shape, FT_MAX, 0.0f);
	if (blend_shapes.size() != FT_MAX) {
		return 0.0f;
	}
	return blend_shapes[p_blend_shape];
}

void XRTracker::set_blend_shape(BlendShapeEntry p_blend_shape, float p_value)
{
	ERR_FAIL_INDEX(p_blend_shape, FT_MAX);
	if (blend_shapes.size() != FT_MAX) {
		blend_shapes.resize_initialized(FT_MAX);
	}
	blend_shapes.set(p_blend_shape, p_value);
}

PackedFloat32Array XRTracker::get_blend_shapes() const
{
	if (blend_shapes.size() == FT_MAX) {
		return blend_shapes;
	}
	PackedFloat32Array data;
	data.resize_initialized(FT_MAX);
	return data;
}

void XRTracker::set_blend_shapes(const PackedFloat32Array& p_blend_shapes)
{
	ERR_FAIL_COND(p_blend_shapes.size() != FT_MAX);
	blend_shapes = p_blend_shapes;
}

XRTracker::XRTracker(XRServer::TrackerType p_type)
{
	if (p_type != XRServer::TRACKER_UNKNOWN) {
		set_tracker_type(p_type);
	}
}

XRTracker::~XRTracker()
{
	if (body_data) {
		memdelete(body_data);
		body_data = nullptr;
	}
	if (hand_data) {
		memdelete(hand_data);
		hand_data = nullptr;
	}
}
