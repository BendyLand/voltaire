/**************************************************************************/
/*  openxr_hand.cpp                                                       */
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

#include "../extensions/openxr_hand_tracking_extension.h"
#include "../openxr_api.h"
#include "openxr_hand.h"
#include "scene/3d/skeleton_3d.h"
#include "servers/xr/xr_server.h"


OpenXRHand::OpenXRHand()
{
	openxr_api = OpenXRAPI::get_singleton();
	hand_tracking_ext = OpenXRHandTrackingExtension::get_singleton();
}

void OpenXRHand::set_hand(Hands p_hand)
{
	ERR_FAIL_INDEX(p_hand, HAND_MAX);

	hand = p_hand;
}

OpenXRHand::Hands OpenXRHand::get_hand() const { return hand; }

void OpenXRHand::set_hand_skeleton(const NodePath& p_hand_skeleton)
{
	hand_skeleton = p_hand_skeleton;

	// TODO if inside tree call _get_bones()
}

void OpenXRHand::set_motion_range(MotionRange p_motion_range)
{
	ERR_FAIL_INDEX(p_motion_range, MOTION_RANGE_MAX);
	motion_range = p_motion_range;

	_set_motion_range();
}

OpenXRHand::MotionRange OpenXRHand::get_motion_range() const { return motion_range; }

NodePath OpenXRHand::get_hand_skeleton() const { return hand_skeleton; }



void OpenXRHand::set_skeleton_rig(SkeletonRig p_skeleton_rig)
{
	ERR_FAIL_INDEX(p_skeleton_rig, SKELETON_RIG_MAX);

	skeleton_rig = p_skeleton_rig;
}

OpenXRHand::SkeletonRig OpenXRHand::get_skeleton_rig() const { return skeleton_rig; }

void OpenXRHand::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);

	bone_update = p_bone_update;
}

OpenXRHand::BoneUpdate OpenXRHand::get_bone_update() const { return bone_update; }

void OpenXRHand::_get_joint_data()
{
	// Table of bone names for different rig types.
	static const String bone_names[SKELETON_RIG_MAX][XR_HAND_JOINT_COUNT_EXT] = {
		// SKELETON_RIG_OPENXR bone names.
		{"Palm", "Wrist", "Thumb_Metacarpal", "Thumb_Proximal", "Thumb_Distal", "Thumb_Tip",
			"Index_Metacarpal", "Index_Proximal", "Index_Intermediate", "Index_Distal", "Index_Tip",
			"Middle_Metacarpal", "Middle_Proximal", "Middle_Intermediate", "Middle_Distal",
			"Middle_Tip", "Ring_Metacarpal", "Ring_Proximal", "Ring_Intermediate", "Ring_Distal",
			"Ring_Tip", "Little_Metacarpal", "Little_Proximal", "Little_Intermediate",
			"Little_Distal", "Little_Tip"},

		// SKELETON_RIG_HUMANOID bone names.
		{"Palm", "Hand", "ThumbMetacarpal", "ThumbProximal", "ThumbDistal", "ThumbTip",
			"IndexMetacarpal", "IndexProximal", "IndexIntermediate", "IndexDistal", "IndexTip",
			"MiddleMetacarpal", "MiddleProximal", "MiddleIntermediate", "MiddleDistal", "MiddleTip",
			"RingMetacarpal", "RingProximal", "RingIntermediate", "RingDistal", "RingTip",
			"LittleMetacarpal", "LittleProximal", "LittleIntermediate", "LittleDistal",
			"LittleTip"}};

	// Table of bone name formats for different rig types and left/right hands.
	static const String bone_name_formats[SKELETON_RIG_MAX][2] = {
		// SKELETON_RIG_OPENXR bone name format.
		{"<bone>_L", "<bone>_R"},

		// SKELETON_RIG_HUMANOID bone name format.
		{"Left<bone>", "Right<bone>"}};

	// reset JIC
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i].bone = -1;
		joints[i].parent_joint = -1;
	}

	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	// Find the skeleton-bones associated with each OpenXR joint.
	int bones[XR_HAND_JOINT_COUNT_EXT];
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		// Construct the expected bone name.
		String bone_name =
			bone_name_formats[skeleton_rig][hand].replace("<bone>", bone_names[skeleton_rig][i]);

		// Find the skeleton bone.
		bones[i] = skeleton->find_bone(bone_name);
		if (bones[i] == -1) {
			// print_line("Couldn't obtain bone for", bone_name);
		}
	}

	// Assemble the OpenXR joint relationship to the available skeleton bones.
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
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
			joints[i].parent_joint = XR_HAND_JOINT_PALM_EXT;
			continue;
		}

		// Find the OpenXR joint associated with the parent skeleton-bone.
		for (int j = 0; j < XR_HAND_JOINT_COUNT_EXT; ++j) {
			if (bones[j] == parent_bone) {
				// If a parent joint is found then drive this bone relative to it.
				joints[i].bone = bone;
				joints[i].parent_joint = j;
				break;
			}
		}
	}
}

void OpenXRHand::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_joint_data();

		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);

		// reset
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			joints[i].bone = -1;
			joints[i].parent_joint = -1;
		}
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_skeleton();
	} break;
	default: {
	} break;
	}
}


