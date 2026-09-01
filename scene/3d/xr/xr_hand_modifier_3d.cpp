/**************************************************************************/
/*  xr_hand_modifier_3d.cpp                                               */
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

#include "core/config/project_settings.h"
#include "servers/xr/xr_server.h"
#include "xr_hand_modifier_3d.h"

void XRHandModifier3D::_bind_methods() {}

void XRHandModifier3D::set_hand_tracker(const StringName& p_tracker_name)
{
	tracker_name = p_tracker_name;

	if (is_inside_tree()) {
		_get_joint_data();
	}
}

StringName XRHandModifier3D::get_hand_tracker() const { return tracker_name; }

void XRHandModifier3D::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);
	bone_update = p_bone_update;
}

XRHandModifier3D::BoneUpdate XRHandModifier3D::get_bone_update() const { return bone_update; }

void XRHandModifier3D::_get_joint_data()
{
	if (!is_inside_tree()) {
		return;
	}

	if (has_stored_previous_transforms) {
		previous_relative_transforms.clear();
		has_stored_previous_transforms = false;
	}

	// Table of bone names for different rig types.
	static const String bone_names[XRHandTracker::HAND_JOINT_MAX] = {
		"Palm",
		"Hand",
		"ThumbMetacarpal",
		"ThumbProximal",
		"ThumbDistal",
		"ThumbTip",
		"IndexMetacarpal",
		"IndexProximal",
		"IndexIntermediate",
		"IndexDistal",
		"IndexTip",
		"MiddleMetacarpal",
		"MiddleProximal",
		"MiddleIntermediate",
		"MiddleDistal",
		"MiddleTip",
		"RingMetacarpal",
		"RingProximal",
		"RingIntermediate",
		"RingDistal",
		"RingTip",
		"LittleMetacarpal",
		"LittleProximal",
		"LittleIntermediate",
		"LittleDistal",
		"LittleTip",
	};

	static const String bone_name_format[2] = {
		"Left<bone>",
		"Right<bone>",
	};

	// reset JIC
	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		joints[i].bone = -1;
		joints[i].parent_joint = -1;
	}

	const Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	const XRServer* xr_server = XRServer::get_singleton();
	if (!xr_server) {
		return;
	}

	const Ref<XRHandTracker> tracker = xr_server->get_tracker(tracker_name);
	if (tracker.is_null()) {
		return;
	}

	// Verify we have a left or right hand tracker.
	const XRPositionalTracker::TrackerHand tracker_hand = tracker->get_tracker_hand();
	if (tracker_hand != XRPositionalTracker::TRACKER_HAND_LEFT &&
		tracker_hand != XRPositionalTracker::TRACKER_HAND_RIGHT) {
		return;
	}

	// Get the hand index (0 = left, 1 = right).
	const int hand = tracker_hand == XRPositionalTracker::TRACKER_HAND_LEFT ? 0 : 1;

	// Find the skeleton-bones associated with each joint.
	int bones[XRHandTracker::HAND_JOINT_MAX];
	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		// Construct the expected bone name.
		String bone_name = bone_name_format[hand].replace("<bone>", bone_names[i]);

		// Find the skeleton bone.
		bones[i] = skeleton->find_bone(bone_name);
		if (bones[i] == -1) {
			WARN_PRINT(vformat("Couldn't obtain bone for %s", bone_name));
		}
	}

	// Assemble the joint relationship to the available skeleton bones.
	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		// Get the skeleton bone (skip if not found).
		const int bone = bones[i];
		if (bone == -1) {
			continue;
		}

		// Find the parent skeleton-bone.
		const int parent_bone = skeleton->get_bone_parent(bone);
		if (parent_bone == -1) {
			// If no parent skeleton-bone exists then drive this relative to palm joint.
			joints[i].bone = bone;
			joints[i].parent_joint = XRHandTracker::HAND_JOINT_PALM;
			continue;
		}

		// Find the joint associated with the parent skeleton-bone.
		for (int j = 0; j < XRHandTracker::HAND_JOINT_MAX; ++j) {
			if (bones[j] == parent_bone) {
				// If a parent joint is found then drive this bone relative to it.
				joints[i].bone = bone;
				joints[i].parent_joint = j;
				break;
			}
		}
	}
}

void XRHandModifier3D::_tracker_changed(
	StringName p_tracker_name, XRServer::TrackerType p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		_get_joint_data();
	}
}

void XRHandModifier3D::_skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new)
{
	_get_joint_data();
}


