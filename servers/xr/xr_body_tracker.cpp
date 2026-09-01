/**************************************************************************/
/*  xr_body_tracker.cpp                                                   */
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
#include "xr_body_tracker.h"

void XRBodyTracker::_bind_methods() {}

void XRBodyTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	ERR_FAIL_COND_MSG(
		p_type != XRServer::TRACKER_BODY, "XRBodyTracker must be of type TRACKER_BODY.");
}

void XRBodyTracker::set_tracker_hand(const XRPositionalTracker::TrackerHand p_hand)
{
	ERR_FAIL_COND_MSG(
		p_hand != XRPositionalTracker::TRACKER_HAND_UNKNOWN, "XRBodyTracker cannot specify hand.");
}

void XRBodyTracker::set_has_tracking_data(bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRBodyTracker::get_has_tracking_data() const { return has_tracking_data; }

void XRBodyTracker::set_body_flags(uint32_t p_body_flags) { body_flags = p_body_flags; }

uint32_t XRBodyTracker::get_body_flags() const { return body_flags; }

void XRBodyTracker::set_joint_flags(Joint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	joint_flags[p_joint] = p_flags;
}

uint32_t XRBodyTracker::get_joint_flags(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, uint32_t());
	return joint_flags[p_joint];
}

void XRBodyTracker::set_joint_transform(Joint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	joint_transforms[p_joint] = p_transform;
}

Transform3D XRBodyTracker::get_joint_transform(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, Transform3D());
	return joint_transforms[p_joint];
}

XRBodyTracker::XRBodyTracker() { type = XRServer::TRACKER_BODY; }


