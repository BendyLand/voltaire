/**************************************************************************/
/*  xr_hand_tracker.cpp                                                   */
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

#include "core/object/class_db.h"
#include "xr_hand_tracker.h"

void XRHandTracker::_bind_methods() {}

void XRHandTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	ERR_FAIL_COND_MSG(
		p_type != XRServer::TRACKER_HAND, "XRHandTracker must be of type TRACKER_HAND.");
}

void XRHandTracker::set_tracker_hand(const XRPositionalTracker::TrackerHand p_hand)
{
	ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_LEFT && p_hand != TRACKER_HAND_RIGHT,
		"XRHandTracker must specify hand.");
	tracker_hand = p_hand;
}

void XRHandTracker::set_has_tracking_data(bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRHandTracker::get_has_tracking_data() const { return has_tracking_data; }

void XRHandTracker::set_hand_tracking_source(XRHandTracker::HandTrackingSource p_source)
{
	hand_tracking_source = p_source;
}

XRHandTracker::HandTrackingSource XRHandTracker::get_hand_tracking_source() const
{
	return hand_tracking_source;
}

void XRHandTracker::set_hand_joint_flags(
	XRHandTracker::HandJoint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	hand_joint_flags[p_joint] = p_flags;
}

uint32_t XRHandTracker::get_hand_joint_flags(
	XRHandTracker::HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, uint32_t());
	return hand_joint_flags[p_joint];
}

void XRHandTracker::set_hand_joint_transform(
	XRHandTracker::HandJoint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	hand_joint_transforms[p_joint] = p_transform;
}

Transform3D XRHandTracker::get_hand_joint_transform(XRHandTracker::HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Transform3D());
	return hand_joint_transforms[p_joint];
}

void XRHandTracker::set_hand_joint_radius(XRHandTracker::HandJoint p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	hand_joint_radii[p_joint] = p_radius;
}

float XRHandTracker::get_hand_joint_radius(XRHandTracker::HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, 0.0);
	return hand_joint_radii[p_joint];
}

void XRHandTracker::set_hand_joint_linear_velocity(
	XRHandTracker::HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	hand_joint_linear_velocities[p_joint] = p_velocity;
}

Vector3 XRHandTracker::get_hand_joint_linear_velocity(XRHandTracker::HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_joint_linear_velocities[p_joint];
}

void XRHandTracker::set_hand_joint_angular_velocity(
	XRHandTracker::HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	hand_joint_angular_velocities[p_joint] = p_velocity;
}

Vector3 XRHandTracker::get_hand_joint_angular_velocity(XRHandTracker::HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_joint_angular_velocities[p_joint];
}

XRHandTracker::XRHandTracker()
{
	type = XRServer::TRACKER_HAND;
	tracker_hand = TRACKER_HAND_LEFT;
}


