/**************************************************************************/
/*  xr_positional_tracker.cpp                                             */
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

#include "servers/xr/xr_controller_tracker.h"
#include "xr_positional_tracker.h"

String XRPositionalTracker::get_tracker_profile() const { return profile; }

XRPositionalTracker::TrackerHand XRPositionalTracker::get_tracker_hand() const
{
	return tracker_hand;
}

void XRPositionalTracker::set_tracker_hand(const XRPositionalTracker::TrackerHand p_hand)
{
	ERR_FAIL_INDEX(p_hand, TRACKER_HAND_MAX);
	tracker_hand = p_hand;
}

bool XRPositionalTracker::has_pose(const StringName& p_action_name) const
{
	return poses.has(p_action_name);
}

Ref<XRPose> XRPositionalTracker::get_pose(const StringName& p_action_name) const
{
	Ref<XRPose> pose;

	if (poses.has(p_action_name)) {
		pose = poses[p_action_name];
	}

	return pose;
}


