/**************************************************************************/
/*  xr_body_modifier_3d.cpp                                               */
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

#include "xr_body_modifier_3d.h"

#include "scene/3d/skeleton_3d.h"
#include "servers/xr/xr_server.h"

XRBodyModifier3D::XRBodyModifier3D(TargetMode p_mode) :
		target_mode(p_mode) {
	if (p_mode == TARGET_HAND) {
		tracker_name = "/user/hand_tracker/left";
	} else {
		tracker_name = "/user/body_tracker";
	}
}

void XRBodyModifier3D::set_target_mode(TargetMode p_mode) {
	if (target_mode == p_mode) {
		return;
	}
	target_mode = p_mode;
	if (target_mode == TARGET_HAND && tracker_name == "/user/body_tracker") {
		tracker_name = "/user/hand_tracker/left";
	} else if (target_mode == TARGET_BODY && tracker_name.begins_with("/user/hand_tracker")) {
		tracker_name = "/user/body_tracker";
	}
	has_stored_previous_transforms = false;
	previous_relative_transforms.clear();
	if (is_inside_tree()) {
		_get_joint_data();
	}
	update_configuration_warnings();
}

void XRBodyModifier3D::set_tracker_name(const String &p_tracker_name) {
	tracker_name = p_tracker_name;
	if (is_inside_tree()) {
		_get_joint_data();
	}
}

String XRBodyModifier3D::get_tracker_name() const {
	return tracker_name;
}

void XRBodyModifier3D::set_body_tracker(const String &p_tracker_name) {
	set_tracker_name(p_tracker_name);
}

String XRBodyModifier3D::get_body_tracker() const {
	return tracker_name;
}

void XRBodyModifier3D::set_hand_tracker(const String &p_tracker_name) {
	set_tracker_name(p_tracker_name);
}

String XRBodyModifier3D::get_hand_tracker() const {
	return tracker_name;
}

void XRBodyModifier3D::set_body_update(uint32_t p_body_update) {
	body_update = p_body_update;
	if (is_inside_tree()) {
		_get_joint_data();
	}
}

uint32_t XRBodyModifier3D::get_body_update() const {
	return body_update;
}

void XRBodyModifier3D::set_bone_update(BoneUpdate p_bone_update) {
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);
	bone_update = p_bone_update;
}

XRBodyModifier3D::BoneUpdate XRBodyModifier3D::get_bone_update() const {
	return bone_update;
}

PackedStringArray XRBodyModifier3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (tracker_name.is_empty()) {
		warnings.push_back(RTR("No XR tracker name specified."));
	}
	return warnings;
}

void XRBodyModifier3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_get_joint_data();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			for (int i = 0; i < XRBodyTracker::JOINT_MAX; i++) {
				body_joints[i].bone = -1;
				body_joints[i].parent_joint = -1;
			}
			for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
				hand_joints[i].bone = -1;
				hand_joints[i].parent_joint = -1;
			}
			has_stored_previous_transforms = false;
			previous_relative_transforms.clear();
		} break;
		default:
			break;
	}
}

void XRBodyModifier3D::_tracker_changed(const String &p_tracker_name, XRServer::TrackerType p_tracker_type) {
	if (tracker_name == p_tracker_name) {
		_get_joint_data();
	}
}

void XRBodyModifier3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	has_stored_previous_transforms = false;
	previous_relative_transforms.clear();
	_get_joint_data();
}

void XRBodyModifier3D::_get_joint_data() {
	if (target_mode == TARGET_HAND) {
		_get_hand_joint_data();
	} else {
		_get_body_joint_data();
	}
}

void XRBodyModifier3D::_get_body_joint_data() {
	for (int i = 0; i < XRBodyTracker::JOINT_MAX; i++) {
		body_joints[i].bone = -1;
		body_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const bone_names[XRBodyTracker::JOINT_MAX] = {
		"Root",
		// Upper body.
		"Hips",
		"Spine",
		"Chest",
		"UpperChest",
		"Neck",
		"Head",
		"HeadTip",
		"LeftShoulder",
		"LeftUpperArm",
		"LeftLowerArm",
		"RightShoulder",
		"RightUpperArm",
		"RightLowerArm",
		// Lower body.
		"LeftUpperLeg",
		"LeftLowerLeg",
		"LeftFoot",
		"LeftToes",
		"RightUpperLeg",
		"RightLowerLeg",
		"RightFoot",
		"RightToes",
		// Left hand.
		"LeftHand",
		"LeftPalm",
		"LeftWrist",
		"LeftThumbMetacarpal",
		"LeftThumbProximal",
		"LeftThumbDistal",
		"LeftThumbTip",
		"LeftIndexMetacarpal",
		"LeftIndexProximal",
		"LeftIndexIntermediate",
		"LeftIndexDistal",
		"LeftIndexTip",
		"LeftMiddleMetacarpal",
		"LeftMiddleProximal",
		"LeftMiddleIntermediate",
		"LeftMiddleDistal",
		"LeftMiddleTip",
		"LeftRingMetacarpal",
		"LeftRingProximal",
		"LeftRingIntermediate",
		"LeftRingDistal",
		"LeftRingTip",
		"LeftLittleMetacarpal",
		"LeftLittleProximal",
		"LeftLittleIntermediate",
		"LeftLittleDistal",
		"LeftLittleTip",
		// Right hand.
		"RightHand",
		"RightPalm",
		"RightWrist",
		"RightThumbMetacarpal",
		"RightThumbProximal",
		"RightThumbDistal",
		"RightThumbTip",
		"RightIndexMetacarpal",
		"RightIndexProximal",
		"RightIndexIntermediate",
		"RightIndexDistal",
		"RightIndexTip",
		"RightMiddleMetacarpal",
		"RightMiddleProximal",
		"RightMiddleIntermediate",
		"RightMiddleDistal",
		"RightMiddleTip",
		"RightRingMetacarpal",
		"RightRingProximal",
		"RightRingIntermediate",
		"RightRingDistal",
		"RightRingTip",
		"RightLittleMetacarpal",
		"RightLittleProximal",
		"RightLittleIntermediate",
		"RightLittleDistal",
		"RightLittleTip",
		// Extra joints.
		"LowerChest",
		"LeftScapula",
		"LeftWristTwist",
		"RightScapula",
		"RightWristTwist",
		"LeftFootTwist",
		"LeftHeel",
		"LeftMiddleFoot",
		"RightFootTwist",
		"RightHeel",
		"RightMiddleFoot",
	};

	int bones[XRBodyTracker::JOINT_MAX];
	for (int i = 0; i < XRBodyTracker::JOINT_MAX; i++) {
		bones[i] = -1;
	}

	bones[XRBodyTracker::JOINT_ROOT] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_ROOT]);

	if (body_update & BODY_UPDATE_UPPER_BODY) {
		for (int i = XRBodyTracker::JOINT_HIPS; i <= XRBodyTracker::JOINT_RIGHT_LOWER_ARM; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRBodyTracker::JOINT_LOWER_CHEST] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LOWER_CHEST]);
		bones[XRBodyTracker::JOINT_LEFT_SCAPULA] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LEFT_SCAPULA]);
		bones[XRBodyTracker::JOINT_LEFT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LEFT_WRIST_TWIST]);
		bones[XRBodyTracker::JOINT_RIGHT_SCAPULA] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_RIGHT_SCAPULA]);
		bones[XRBodyTracker::JOINT_RIGHT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_RIGHT_WRIST_TWIST]);
	}

	if (body_update & BODY_UPDATE_LOWER_BODY) {
		for (int i = XRBodyTracker::JOINT_LEFT_UPPER_LEG; i <= XRBodyTracker::JOINT_RIGHT_TOES; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRBodyTracker::JOINT_LEFT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LEFT_FOOT_TWIST]);
		bones[XRBodyTracker::JOINT_LEFT_HEEL] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LEFT_HEEL]);
		bones[XRBodyTracker::JOINT_LEFT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_LEFT_MIDDLE_FOOT]);
		bones[XRBodyTracker::JOINT_RIGHT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_RIGHT_FOOT_TWIST]);
		bones[XRBodyTracker::JOINT_RIGHT_HEEL] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_RIGHT_HEEL]);
		bones[XRBodyTracker::JOINT_RIGHT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRBodyTracker::JOINT_RIGHT_MIDDLE_FOOT]);
	}

	if (body_update & BODY_UPDATE_HANDS) {
		for (int i = XRBodyTracker::JOINT_LEFT_HAND; i <= XRBodyTracker::JOINT_RIGHT_PINKY_FINGER_TIP; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
	}

	for (int i = 0; i < XRBodyTracker::JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		body_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRBodyTracker::JOINT_MAX; j++) {
				if (bones[j] == parent_bone) {
					body_joints[i].parent_joint = j;
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
			parent_bone = skeleton->get_bone_parent(parent_bone);
		}
	}
}

void XRBodyModifier3D::_get_hand_joint_data() {
	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		hand_joints[i].bone = -1;
		hand_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const left_hand_names[XRHandTracker::HAND_JOINT_MAX] = {
		"LeftPalm",
		"LeftHand",
		"LeftThumbMetacarpal",
		"LeftThumbProximal",
		"LeftThumbDistal",
		"LeftThumbTip",
		"LeftIndexMetacarpal",
		"LeftIndexProximal",
		"LeftIndexIntermediate",
		"LeftIndexDistal",
		"LeftIndexTip",
		"LeftMiddleMetacarpal",
		"LeftMiddleProximal",
		"LeftMiddleIntermediate",
		"LeftMiddleDistal",
		"LeftMiddleTip",
		"LeftRingMetacarpal",
		"LeftRingProximal",
		"LeftRingIntermediate",
		"LeftRingDistal",
		"LeftRingTip",
		"LeftLittleMetacarpal",
		"LeftLittleProximal",
		"LeftLittleIntermediate",
		"LeftLittleDistal",
		"LeftLittleTip",
	};

	static const char *const right_hand_names[XRHandTracker::HAND_JOINT_MAX] = {
		"RightPalm",
		"RightHand",
		"RightThumbMetacarpal",
		"RightThumbProximal",
		"RightThumbDistal",
		"RightThumbTip",
		"RightIndexMetacarpal",
		"RightIndexProximal",
		"RightIndexIntermediate",
		"RightIndexDistal",
		"RightIndexTip",
		"RightMiddleMetacarpal",
		"RightMiddleProximal",
		"RightMiddleIntermediate",
		"RightMiddleDistal",
		"RightMiddleTip",
		"RightRingMetacarpal",
		"RightRingProximal",
		"RightRingIntermediate",
		"RightRingDistal",
		"RightRingTip",
		"RightLittleMetacarpal",
		"RightLittleProximal",
		"RightLittleIntermediate",
		"RightLittleDistal",
		"RightLittleTip",
	};

	const bool is_right = tracker_name.contains("right");
	const char *const *names = is_right ? right_hand_names : left_hand_names;

	int bones[XRHandTracker::HAND_JOINT_MAX];
	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		bones[i] = skeleton->find_bone(names[i]);
	}

	for (int i = 0; i < XRHandTracker::HAND_JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		hand_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRHandTracker::HAND_JOINT_MAX; j++) {
				if (bones[j] == parent_bone) {
					hand_joints[i].parent_joint = j;
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
			parent_bone = skeleton->get_bone_parent(parent_bone);
		}
	}
}

void XRBodyModifier3D::_process_modification(double p_delta) {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton || !XRServer::is_initialized()) {
		return;
	}

	if (target_mode == TARGET_HAND) {
		_process_hand_modification(skeleton);
	} else {
		_process_body_modification(skeleton);
	}
}

void XRBodyModifier3D::_process_body_modification(Skeleton3D *p_skeleton) {
	const Ref<XRBodyTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || !tracker->get_has_tracking_data()) {
		return;
	}

	const float ss = p_skeleton->get_motion_scale();
	Transform3D transforms[XRBodyTracker::JOINT_MAX];
	Transform3D inv_transforms[XRBodyTracker::JOINT_MAX];
	bool valid[XRBodyTracker::JOINT_MAX] = {};

	for (int joint = 0; joint < XRBodyTracker::JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_joint_flags(static_cast<XRBodyTracker::Joint>(joint));
		if ((flags & XRBodyTracker::JOINT_FLAG_ORIENTATION_VALID) && (flags & XRBodyTracker::JOINT_FLAG_POSITION_VALID)) {
			valid[joint] = true;
			transforms[joint] = tracker->get_joint_transform(static_cast<XRBodyTracker::Joint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRBodyTracker::JOINT_ROOT]) {
		return;
	}

	for (int joint = 0; joint < XRBodyTracker::JOINT_MAX; joint++) {
		const int b = body_joints[joint].bone;
		if (b == -1 || !valid[joint]) {
			continue;
		}
		const int parent_joint = body_joints[joint].parent_joint;
		Transform3D rel_transform;
		if (parent_joint != -1 && valid[parent_joint]) {
			rel_transform = inv_transforms[parent_joint] * transforms[joint];
		} else {
			rel_transform = transforms[joint];
		}

		p_skeleton->set_bone_pose_rotation(b, Quaternion(rel_transform.basis));
		if (bone_update == BONE_UPDATE_FULL) {
			p_skeleton->set_bone_pose_position(b, rel_transform.origin);
		}
	}
}

void XRBodyModifier3D::_process_hand_modification(Skeleton3D *p_skeleton) {
	const Ref<XRHandTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null()) {
		return;
	}

	if (!tracker->get_has_tracking_data()) {
		if (!has_stored_previous_transforms) {
			return;
		}
		for (int joint = 0; joint < XRHandTracker::HAND_JOINT_MAX; joint++) {
			const int b = hand_joints[joint].bone;
			if (b == -1) {
				continue;
			}
			p_skeleton->set_bone_pose_rotation(b, Quaternion(previous_relative_transforms[joint].basis));
			if (bone_update == BONE_UPDATE_FULL) {
				p_skeleton->set_bone_pose_position(b, previous_relative_transforms[joint].origin);
			}
		}
		return;
	}

	const float ss = p_skeleton->get_motion_scale();
	Transform3D transforms[XRHandTracker::HAND_JOINT_MAX];
	Transform3D inv_transforms[XRHandTracker::HAND_JOINT_MAX];
	bool valid[XRHandTracker::HAND_JOINT_MAX] = {};

	previous_relative_transforms.resize(XRHandTracker::HAND_JOINT_MAX);

	for (int joint = 0; joint < XRHandTracker::HAND_JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_hand_joint_flags(static_cast<XRHandTracker::HandJoint>(joint));
		if (flags & XRHandTracker::HAND_JOINT_FLAG_ORIENTATION_VALID) {
			valid[joint] = true;
			transforms[joint] = tracker->get_hand_joint_transform(static_cast<XRHandTracker::HandJoint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRHandTracker::HAND_JOINT_WRIST]) {
		return;
	}

	const Transform3D skel_inv = p_skeleton->get_global_transform_interpolated().affine_inverse();

	for (int joint = 0; joint < XRHandTracker::HAND_JOINT_MAX; joint++) {
		const int b = hand_joints[joint].bone;
		if (b == -1 || !valid[joint]) {
			continue;
		}
		const int parent_joint = hand_joints[joint].parent_joint;
		Transform3D rel_transform;
		if (parent_joint != -1 && valid[parent_joint]) {
			rel_transform = inv_transforms[parent_joint] * transforms[joint];
		} else {
			rel_transform = skel_inv * transforms[joint];
		}

		previous_relative_transforms.write[joint] = rel_transform;
		p_skeleton->set_bone_pose_rotation(b, Quaternion(rel_transform.basis));
		if (bone_update == BONE_UPDATE_FULL) {
			p_skeleton->set_bone_pose_position(b, rel_transform.origin);
		}
	}

	has_stored_previous_transforms = true;
}
