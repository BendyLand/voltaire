/**************************************************************************/
/*  xr_nodes.cpp                                                          */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/types.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/xr/xr_interface.h"
#include "xr_nodes.h"

// XRNode3D is a node that has its transform updated by an XRTracker.
// Note that trackers are only available at runtime and only after an XRInterface registers one.
// So we bind by name and as long as a tracker isn't available, our node remains inactive.

void XRNode3D::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	update_configuration_warnings();
}

void XRNode3D::set_tracker(const String& p_tracker_name)
{
	if (tracker.is_valid() && tracker->get_tracker_name() == p_tracker_name) {
		return;
	}

	_unbind_tracker();

	tracker_name = p_tracker_name;
	pose_name = SceneString(default_);

	_bind_tracker();

	update_configuration_warnings();
}

String XRNode3D::get_tracker() const { return tracker_name; }

void XRNode3D::set_pose_name(const String& p_pose_name)
{
	pose_name = p_pose_name;

	// Update pose if we are bound to a tracker with a valid pose
	Ref<XRPose> pose = get_pose();
	if (pose.is_valid()) {
		set_transform(pose->get_adjusted_transform());
	}
}

String XRNode3D::get_pose_name() const { return pose_name; }

void XRNode3D::set_show_when_tracked(bool p_show)
{
	show_when_tracked = p_show;

	_update_visibility();
}

bool XRNode3D::get_show_when_tracked() const { return show_when_tracked; }

bool XRNode3D::get_is_active() const
{
	if (tracker.is_null()) {
		return false;
	}
	else if (!tracker->has_pose(pose_name)) {
		return false;
	}
	else {
		return true;
	}
}

bool XRNode3D::get_has_tracking_data() const { return has_tracking_data; }

void XRNode3D::trigger_haptic_pulse(const String& p_action_name, double p_frequency,
	double p_amplitude, double p_duration_sec, double p_delay_sec)
{
	// TODO need to link trackers to the interface that registered them so we can call this on the
	// correct interface. For now this works fine as in 99% of the cases we only have our primary
	// interface active
	if (XRServer::is_initialized()) {
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			xr_interface->trigger_haptic_pulse(
				p_action_name, tracker_name, p_frequency, p_amplitude, p_duration_sec, p_delay_sec);
		}
	}
}

Ref<XRPose> XRNode3D::get_pose()
{
	if (tracker.is_valid()) {
		return tracker->get_pose(pose_name);
	}
	else {
		return Ref<XRPose>();
	}
}

void XRNode3D::_bind_tracker()
{
	ERR_FAIL_COND(tracker.is_valid());

	if (XRServer::is_initialized()) {
		tracker = XRServer::get_tracker(tracker_name);
		if (tracker.is_null()) {
			return;
		}

		Ref<XRPose> pose = get_pose();
		if (pose.is_valid()) {
			set_transform(pose->get_adjusted_transform());
			_set_has_tracking_data(pose->get_has_tracking_data());
		}
		else {
			_set_has_tracking_data(false);
		}
	}
}

void XRNode3D::_unbind_tracker()
{
	if (tracker.is_valid()) {
		tracker.unref();
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_changed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// just in case unref our current tracker
		_unbind_tracker();

		// get our new tracker
		_bind_tracker();
	}
}

void XRNode3D::_removed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// unref our tracker, it's no longer available
		_unbind_tracker();
	}
}

void XRNode3D::_pose_changed(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		set_transform(p_pose->get_adjusted_transform());
		_set_has_tracking_data(p_pose->get_has_tracking_data());
	}
}

void XRNode3D::_pose_lost_tracking(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_set_has_tracking_data(bool p_has_tracking_data)
{
	if (has_tracking_data != p_has_tracking_data) {
		has_tracking_data = p_has_tracking_data;
		_update_visibility();
	}
}

void XRNode3D::_update_visibility()
{
	// If configured, show or hide the node based on tracking data.
	if (show_when_tracked) {
		// Only react to this if we have a primary interface.
		if (XRServer::is_initialized()) {
			Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
			if (xr_interface.is_valid()) {
				set_visible(has_tracking_data);
			}
		}
	}
}

void XRNode3D::_button_pressed(const String& p_name) {}

void XRNode3D::_button_released(const String& p_name) {}

void XRNode3D::_input_float_changed(const String& p_name, float p_value) {}

void XRNode3D::_input_vector2_changed(const String& p_name, Vector2 p_value) {}

void XRNode3D::_profile_changed(const String& p_role) {}

bool XRNode3D::is_button_pressed(const String& p_name) const
{
	if (tracker.is_null()) {
		return false;
	}
	return tracker->is_button_pressed(p_name);
}

XRTracker::InputValue XRNode3D::get_input(const String& p_name) const
{
	if (tracker.is_null()) {
		return XRTracker::InputValue();
	}
	return tracker->get_input(p_name);
}

float XRNode3D::get_float(const String& p_name) const
{
	if (tracker.is_null()) {
		return 0.0f;
	}
	return tracker->get_input_float(p_name);
}

Vector2 XRNode3D::get_vector2(const String& p_name) const
{
	if (tracker.is_null()) {
		return Vector2();
	}
	return tracker->get_input_vector2(p_name);
}

XRTracker::TrackerHand XRNode3D::get_tracker_hand() const
{
	if (tracker.is_null()) {
		return XRTracker::TRACKER_HAND_UNKNOWN;
	}

	return tracker->get_tracker_hand();
}

Vector3 XRNode3D::get_size() const { return size; }

Plane XRNode3D::get_plane() const
{
	Vector3 location = get_position();
	Basis orientation = get_transform().basis;

	Plane plane(orientation.get_column(1).normalized(), location);

	return plane;
}

PackedStringArray XRNode3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (is_visible() && is_inside_tree()) {
		const XROrigin3D* origin = dynamic_cast<const XROrigin3D*>(get_parent());
		if (origin == nullptr) {
			warnings.push_back(RTR("XRNode3D may not function as expected without an XROrigin3D node as its parent."));
		}
		if (tracker_name.is_empty()) {
			warnings.push_back(RTR("No tracker name is set."));
		}
		if (mode != MODE_ANCHOR && pose_name.is_empty()) {
			warnings.push_back(RTR("No pose is set."));
		}
	}

	if (get_physics_interpolation_mode() != Node::PHYSICS_INTERPOLATION_MODE_OFF) {
		warnings.push_back(RTR("XRNode3D should have physics_interpolation_mode set to OFF in order to avoid jitter."));
	}

	return warnings;
}

XRNode3D::XRNode3D(Mode p_mode) : mode(p_mode) {}

XRNode3D::~XRNode3D() { _unbind_tracker(); }

// XROrigin3D

Vector<XROrigin3D*> XROrigin3D::origin_nodes;

real_t XROrigin3D::get_world_scale() const
{
	// get our XRServer
	ERR_FAIL_COND_V(!XRServer::is_initialized(), 1.0);

	return XRServer::get_world_scale();
}

void XROrigin3D::set_world_scale(real_t p_world_scale)
{
	// get our XRServer
	ERR_FAIL_COND(!XRServer::is_initialized());

	XRServer::set_world_scale(p_world_scale);
}

void XROrigin3D::_set_current(bool p_enabled, bool p_update_others)
{
	// We run this logic even if current already equals p_enabled as we may have set this previously
	// before we entered our tree. This is then called a second time on NOTIFICATION_ENTER_TREE
	// where we actually process activating this origin node.
	current = p_enabled;

	if (!is_inside_tree() || Engine::is_editor_hint()) {
		return;
	}

	// Notify us of any transform changes
	set_notify_local_transform(current);
	set_notify_transform(current);

	// update XRServer with our current position
	if (current) {
		ERR_FAIL_COND(!XRServer::is_initialized());

		XRServer::set_world_origin(get_global_transform());

		if (is_physics_interpolated()) {
			set_process_internal(true);
		}
	}
	else if (is_physics_interpolated()) {
		set_process_internal(false);
	}

	// Check if we need to update our other origin nodes accordingly
	if (p_update_others) {
		if (current) {
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this && origin_nodes[i]->current) {
					origin_nodes[i]->_set_current(false, false);
				}
			}
		}
		else {
			// We no longer have a current origin so find the first one we can make current
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this) {
					origin_nodes[i]->_set_current(true, false);
					return; // we are done.
				}
			}
		}
	}
}

void XROrigin3D::set_current(bool p_enabled) { _set_current(p_enabled, true); }

bool XROrigin3D::is_current() const
{
	if (Engine::is_editor_hint()) {
		// return as is
		return current;
	}
	else {
		return current && is_inside_tree();
	}
}

void XROrigin3D::_physics_interpolated_changed()
{
	if (current && !Engine::is_editor_hint()) {
		set_process_internal(is_physics_interpolated());
	}
}

PackedStringArray XROrigin3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (is_visible() && is_inside_tree()) {
		bool has_xr_camera = false;
		for (int i = 0; i < get_child_count(); i++) {
			const Camera3D* camera = dynamic_cast<const Camera3D*>(get_child(i));
			if (camera && camera->is_xr_enabled()) {
				has_xr_camera = true;
				break;
			}
		}
		if (!has_xr_camera) {
			warnings.push_back(RTR("XROrigin3D requires an XR-enabled Camera3D child node."));
		}
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/xr/xr_body_modifier_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/xr/xr_body_modifier_3d.h ---
#include "scene/3d/skeleton_modifier_3d.h"
#include "servers/xr/xr_tracker.h"

class Skeleton3D;

/**
	The XRBodyModifier3D node drives a body or hand skeleton using tracking
	data from an XRTracker instance.
 */

class XRBodyModifier3D final : public SkeletonModifier3D {
public:
	enum TargetMode {
		TARGET_BODY,
		TARGET_HAND,
	};

	enum BodyUpdate {
		BODY_UPDATE_UPPER_BODY = 1,
		BODY_UPDATE_LOWER_BODY = 2,
		BODY_UPDATE_HANDS = 4,
	};

	enum BoneUpdate {
		BONE_UPDATE_FULL,
		BONE_UPDATE_ROTATION_ONLY,
		BONE_UPDATE_MAX,
	};

	XRBodyModifier3D(TargetMode p_mode = TARGET_BODY);

	void set_target_mode(TargetMode p_mode);
	TargetMode get_target_mode() const { return target_mode; }

	void set_tracker_name(const String &p_tracker_name);
	String get_tracker_name() const;

	void set_body_tracker(const String &p_tracker_name);
	String get_body_tracker() const;

	void set_hand_tracker(const String &p_tracker_name);
	String get_hand_tracker() const;

	void set_body_update(uint32_t p_body_update);
	uint32_t get_body_update() const;

	void set_bone_update(BoneUpdate p_bone_update);
	BoneUpdate get_bone_update() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	void _notification(int p_what);

protected:
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _process_modification(double p_delta) override;

private:
	struct JointData {
		int bone = -1;
		int parent_joint = -1;
	};

	TargetMode target_mode = TARGET_BODY;
	String tracker_name = "/user/body_tracker";
	uint32_t body_update = BODY_UPDATE_UPPER_BODY | BODY_UPDATE_LOWER_BODY | BODY_UPDATE_HANDS;
	BoneUpdate bone_update = BONE_UPDATE_FULL;
	JointData body_joints[XRTracker::JOINT_MAX];
	JointData hand_joints[XRTracker::HAND_JOINT_MAX];

	bool has_stored_previous_transforms = false;
	Vector<Transform3D> previous_relative_transforms;

	void _get_joint_data();
	void _get_body_joint_data();
	void _get_hand_joint_data();
	void _process_body_modification(Skeleton3D *p_skeleton);
	void _process_hand_modification(Skeleton3D *p_skeleton);
	void _tracker_changed(const String &p_tracker_name, XRServer::TrackerType p_tracker_type);
};

// --- End scene/3d/xr/xr_body_modifier_3d.h ---

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
			for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
				body_joints[i].bone = -1;
				body_joints[i].parent_joint = -1;
			}
			for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
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
	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		body_joints[i].bone = -1;
		body_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const bone_names[XRTracker::JOINT_MAX] = {
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

	int bones[XRTracker::JOINT_MAX];
	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		bones[i] = -1;
	}

	bones[XRTracker::JOINT_ROOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_ROOT]);

	if (body_update & BODY_UPDATE_UPPER_BODY) {
		for (int i = XRTracker::JOINT_HIPS; i <= XRTracker::JOINT_RIGHT_LOWER_ARM; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRTracker::JOINT_LOWER_CHEST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LOWER_CHEST]);
		bones[XRTracker::JOINT_LEFT_SCAPULA] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_SCAPULA]);
		bones[XRTracker::JOINT_LEFT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_WRIST_TWIST]);
		bones[XRTracker::JOINT_RIGHT_SCAPULA] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_SCAPULA]);
		bones[XRTracker::JOINT_RIGHT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_WRIST_TWIST]);
	}

	if (body_update & BODY_UPDATE_LOWER_BODY) {
		for (int i = XRTracker::JOINT_LEFT_UPPER_LEG; i <= XRTracker::JOINT_RIGHT_TOES; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRTracker::JOINT_LEFT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_FOOT_TWIST]);
		bones[XRTracker::JOINT_LEFT_HEEL] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_HEEL]);
		bones[XRTracker::JOINT_LEFT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_MIDDLE_FOOT]);
		bones[XRTracker::JOINT_RIGHT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_FOOT_TWIST]);
		bones[XRTracker::JOINT_RIGHT_HEEL] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_HEEL]);
		bones[XRTracker::JOINT_RIGHT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_MIDDLE_FOOT]);
	}

	if (body_update & BODY_UPDATE_HANDS) {
		for (int i = XRTracker::JOINT_LEFT_HAND; i <= XRTracker::JOINT_RIGHT_PINKY_FINGER_TIP; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
	}

	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		body_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRTracker::JOINT_MAX; j++) {
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
	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		hand_joints[i].bone = -1;
		hand_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const left_hand_names[XRTracker::HAND_JOINT_MAX] = {
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

	static const char *const right_hand_names[XRTracker::HAND_JOINT_MAX] = {
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

	int bones[XRTracker::HAND_JOINT_MAX];
	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		bones[i] = skeleton->find_bone(names[i]);
	}

	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		hand_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRTracker::HAND_JOINT_MAX; j++) {
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
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_BODY || !tracker->get_has_tracking_data()) {
		return;
	}

	const float ss = p_skeleton->get_motion_scale();
	Transform3D transforms[XRTracker::JOINT_MAX];
	Transform3D inv_transforms[XRTracker::JOINT_MAX];
	bool valid[XRTracker::JOINT_MAX] = {};

	for (int joint = 0; joint < XRTracker::JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_joint_flags(static_cast<XRTracker::Joint>(joint));
		if ((flags & XRTracker::JOINT_FLAG_ORIENTATION_VALID) && (flags & XRTracker::JOINT_FLAG_POSITION_VALID)) {
			valid[joint] = true;
			transforms[joint] = tracker->get_joint_transform(static_cast<XRTracker::Joint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRTracker::JOINT_ROOT]) {
		return;
	}

	for (int joint = 0; joint < XRTracker::JOINT_MAX; joint++) {
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
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_HAND) {
		return;
	}

	if (!tracker->get_has_tracking_data()) {
		if (!has_stored_previous_transforms) {
			return;
		}
		for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
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
	Transform3D transforms[XRTracker::HAND_JOINT_MAX];
	Transform3D inv_transforms[XRTracker::HAND_JOINT_MAX];
	bool valid[XRTracker::HAND_JOINT_MAX] = {};

	previous_relative_transforms.resize(XRTracker::HAND_JOINT_MAX);

	for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_hand_joint_flags(static_cast<XRTracker::HandJoint>(joint));
		if (flags & XRTracker::HAND_JOINT_FLAG_ORIENTATION_VALID) {
			valid[joint] = true;
			transforms[joint] = tracker->get_hand_joint_transform(static_cast<XRTracker::HandJoint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRTracker::HAND_JOINT_WRIST]) {
		return;
	}

	const Transform3D skel_inv = p_skeleton->get_global_transform_interpolated().affine_inverse();

	for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
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

// ==========================================================================
// Merged from scene/3d/xr/xr_face_modifier_3d.cpp
// ==========================================================================

#include "servers/xr/xr_tracker.h"
#include "servers/xr/xr_server.h"
// --- Inlined from scene/3d/xr/xr_face_modifier_3d.h ---
#include "core/templates/rb_map.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"

/**
	The XRFaceModifier3D node drives the blend shapes of a MeshInstance3D
	with facial expressions from an XRFaceTracking instance.

	The blend shapes provided by the mesh are interrogated, and used to
	deduce an optimal mapping from the Unified Expressions blend shapes
	provided by the	XRFaceTracking instance to drive the face.
 */

class XRFaceModifier3D final : public Node3D {

private:
	String tracker_name = "/user/face_tracker";
	NodePath target;

	// Map from XRFaceTracker blend shape index to mesh blend shape index.
	RBMap<int, int> blend_mapping;

	MeshInstance3D *get_mesh_instance() const;
	void _get_blend_data();
	void _update_face_blends() const;

protected:

public:
	void set_face_tracker(const String &p_tracker_name);
	String get_face_tracker() const;

	void set_target(const NodePath &p_target);
	NodePath get_target() const;

	void _notification(int p_what);
};

// --- End scene/3d/xr/xr_face_modifier_3d.h ---

// This method takes the name of a mesh blend shape and returns the
// corresponding XRFaceTracker blend shape. If no match is
// found then the function returns -1.
static int find_face_blend_shape(const String& p_name)
{
	// Entry for blend shape name table.
	struct blend_map_entry
	{
		int blend;
		const char* name[4];
	};

	// Table of blend shape names.
	//
	// This table consists of the XRFaceTracker blend shape and
	// the corresponding names (lowercase and no underscore) of:
	// - The Unified Expression blend shape name.
	// - The ARKit blend shape name (if present and different).
	// - The SRanipal blend shape name (if present and different).
	// - The Meta blend shape name (if present and different).
	static constexpr blend_map_entry blend_map[] = {
		{XRTracker::FT_EYE_LOOK_OUT_RIGHT,
			{"eyelookoutright", "eyerightright", "eyeslookoutr"}},
		{XRTracker::FT_EYE_LOOK_IN_RIGHT, {"eyelookinright", "eyerightleft", "eyeslookinr"}},
		{XRTracker::FT_EYE_LOOK_UP_RIGHT, {"eyelookupright", "eyerightlookup", "eyeslookupr"}},
		{XRTracker::FT_EYE_LOOK_DOWN_RIGHT,
			{"eyelookdownright", "eyerightlookdown", "eyeslookdownr"}},
		{XRTracker::FT_EYE_LOOK_OUT_LEFT, {"eyelookoutleft", "eyeleftleft", "eyeslookoutl"}},
		{XRTracker::FT_EYE_LOOK_IN_LEFT, {"eyelookinleft", "eyeleftright", "eyeslookinl"}},
		{XRTracker::FT_EYE_LOOK_UP_LEFT, {"eyelookupleft", "eyeleftlookup", "eyeslookupl"}},
		{XRTracker::FT_EYE_LOOK_DOWN_LEFT,
			{"eyelookdownleft", "eyeleftlookdown", "eyeslookdownl"}},
		{XRTracker::FT_EYE_CLOSED_RIGHT,
			{"eyeclosedright", "eyeblinkright", "eyerightblink", "eyesclosedr"}},
		{XRTracker::FT_EYE_CLOSED_LEFT,
			{"eyeclosedleft", "eyeblinkleft", "eyeleftblink", "eyesclosedl"}},
		{XRTracker::FT_EYE_SQUINT_RIGHT, {"eyesquintright", "eyessquintr"}},
		{XRTracker::FT_EYE_SQUINT_LEFT, {"eyesquintleft", "eyessquintl"}},
		{XRTracker::FT_EYE_WIDE_RIGHT, {"eyewideright", "eyerightwide", "eyeswidenr"}},
		{XRTracker::FT_EYE_WIDE_LEFT, {"eyewideleft", "eyeleftwide", "eyeswidenl"}},
		{XRTracker::FT_EYE_DILATION_RIGHT, {"eyedilationright", "eyerightdilation"}},
		{XRTracker::FT_EYE_DILATION_LEFT, {"eyedilationleft", "eyeleftdilation"}},
		{XRTracker::FT_EYE_CONSTRICT_RIGHT, {"eyeconstrictright", "eyerightconstrict"}},
		{XRTracker::FT_EYE_CONSTRICT_LEFT, {"eyeconstrictleft", "eyeleftconstrict"}},
		{XRTracker::FT_BROW_PINCH_RIGHT, {"browpinchright"}},
		{XRTracker::FT_BROW_PINCH_LEFT, {"browpinchleft"}},
		{XRTracker::FT_BROW_LOWERER_RIGHT, {"browlowererright"}},
		{XRTracker::FT_BROW_LOWERER_LEFT, {"browlowererleft"}},
		{XRTracker::FT_BROW_INNER_UP_RIGHT, {"browinnerupright", "innerbrowraiserr"}},
		{XRTracker::FT_BROW_INNER_UP_LEFT, {"browinnerupleft", "innerbrowraiserl"}},
		{XRTracker::FT_BROW_OUTER_UP_RIGHT, {"browouterupright", "outerbrowraiserr"}},
		{XRTracker::FT_BROW_OUTER_UP_LEFT, {"browouterupleft", "outerbrowraiserl"}},
		{XRTracker::FT_NOSE_SNEER_RIGHT, {"nosesneerright", "nosewrinklerr"}},
		{XRTracker::FT_NOSE_SNEER_LEFT, {"nosesneerleft", "nosewrinklerl"}},
		{XRTracker::FT_NASAL_DILATION_RIGHT, {"nasaldilationright"}},
		{XRTracker::FT_NASAL_DILATION_LEFT, {"nasaldilationleft"}},
		{XRTracker::FT_NASAL_CONSTRICT_RIGHT, {"nasalconstrictright"}},
		{XRTracker::FT_NASAL_CONSTRICT_LEFT, {"nasalconstrictleft"}},
		{XRTracker::FT_CHEEK_SQUINT_RIGHT, {"cheeksquintright", "cheekraiserr"}},
		{XRTracker::FT_CHEEK_SQUINT_LEFT, {"cheeksquintleft", "cheekraiserl"}},
		{XRTracker::FT_CHEEK_PUFF_RIGHT, {"cheekpuffright", "cheekpuffr"}},
		{XRTracker::FT_CHEEK_PUFF_LEFT, {"cheekpuffleft", "cheekpuffl"}},
		{XRTracker::FT_CHEEK_SUCK_RIGHT, {"cheeksuckright", "cheeksuckr"}},
		{XRTracker::FT_CHEEK_SUCK_LEFT, {"cheeksuckleft", "cheeksuckl"}},
		{XRTracker::FT_JAW_OPEN, {"jawopen", "jawdrop"}},
		{XRTracker::FT_MOUTH_CLOSED,
			{"mouthclosed", "mouthclose", "mouthapeshape", "lipstoward"}},
		{XRTracker::FT_JAW_RIGHT, {"jawright", "jawsidewaysright"}},
		{XRTracker::FT_JAW_LEFT, {"jawleft", "jawsidewaysleft"}},
		{XRTracker::FT_JAW_FORWARD, {"jawforward", "jawthrust"}},
		{XRTracker::FT_JAW_BACKWARD, {"jawbackward"}},
		{XRTracker::FT_JAW_CLENCH, {"jawclench"}},
		{XRTracker::FT_JAW_MANDIBLE_RAISE, {"jawmandibleraise"}},
		{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, {"lipsuckupperright", "lipsuckrt"}},
		{XRTracker::FT_LIP_SUCK_UPPER_LEFT, {"lipsuckupperleft", "lipsucklt"}},
		{XRTracker::FT_LIP_SUCK_LOWER_RIGHT, {"lipsucklowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_SUCK_LOWER_LEFT, {"lipsucklowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_SUCK_CORNER_RIGHT, {"lipsuckcornerright"}},
		{XRTracker::FT_LIP_SUCK_CORNER_LEFT, {"lipsuckcornerleft"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, {"lipfunnelupperright", "lipfunnelerrt"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, {"lipfunnelupperleft", "lipfunnelerlt"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, {"lipfunnellowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, {"lipfunnellowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, {"lippuckerupperright"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_LEFT, {"lippuckerupperleft"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, {"lippuckerlowerright"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_LEFT, {"lippuckerlowerleft"}},
		{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, {"mouthupperupright", "upperlipraiserr"}},
		{XRTracker::FT_MOUTH_UPPER_UP_LEFT, {"mouthupperupleft", "upperlipraiserl"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
			{"mouthlowerdownright", "mouthlowerupright", "lowerlipdepressorr"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_LEFT,
			{"mouthlowerdownleft", "mouthlowerupleft", "lowerlipdepressorl"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_RIGHT, {"mouthupperdeepenright"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_LEFT, {"mouthupperdeepenleft"}},
		{XRTracker::FT_MOUTH_UPPER_RIGHT, {"mouthupperright"}},
		{XRTracker::FT_MOUTH_UPPER_LEFT, {"mouthupperleft"}},
		{XRTracker::FT_MOUTH_LOWER_RIGHT, {"mouthlowerright"}},
		{XRTracker::FT_MOUTH_LOWER_LEFT, {"mouthlowerleft"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, {"mouthcornerpullright"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, {"mouthcornerpullleft"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT, {"mouthcornerslantright"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_LEFT, {"mouthcornerslantleft"}},
		{XRTracker::FT_MOUTH_FROWN_RIGHT, {"mouthfrownright", "lipcornerdepressorr"}},
		{XRTracker::FT_MOUTH_FROWN_LEFT, {"mouthfrownleft", "lipcornerdepressorl"}},
		{XRTracker::FT_MOUTH_STRETCH_RIGHT, {"mouthstretchright", "lipstretcherr"}},
		{XRTracker::FT_MOUTH_STRETCH_LEFT, {"mouthstretchleft", "lipstretcherl"}},
		{XRTracker::FT_MOUTH_DIMPLE_RIGHT,
			{"mouthdimplerright", "mouthdimpleright", "dimplerr"}},
		{XRTracker::FT_MOUTH_DIMPLE_LEFT, {"mouthdimplerleft", "mouthdimpleleft", "dimplerl"}},
		{XRTracker::FT_MOUTH_RAISER_UPPER,
			{"mouthraiserupper", "mouthshrugupper", "chinraisert"}},
		{XRTracker::FT_MOUTH_RAISER_LOWER,
			{"mouthraiserlower", "mouthshruglower", "mouthloweroverlay", "chinraiserb"}},
		{XRTracker::FT_MOUTH_PRESS_RIGHT, {"mouthpressright", "lippressorr"}},
		{XRTracker::FT_MOUTH_PRESS_LEFT, {"mouthpressleft", "lippressorl"}},
		{XRTracker::FT_MOUTH_TIGHTENER_RIGHT, {"mouthtightenerright", "liptightenerr"}},
		{XRTracker::FT_MOUTH_TIGHTENER_LEFT, {"mouthtightenerleft", "liptightenerl"}},
		{XRTracker::FT_TONGUE_OUT, {"tongueout", "tonguelongstep2"}},
		{XRTracker::FT_TONGUE_UP, {"tongueup"}},
		{XRTracker::FT_TONGUE_DOWN, {"tonguedown"}},
		{XRTracker::FT_TONGUE_RIGHT, {"tongueright"}},
		{XRTracker::FT_TONGUE_LEFT, {"tongueleft"}},
		{XRTracker::FT_TONGUE_ROLL, {"tongueroll"}},
		{XRTracker::FT_TONGUE_BLEND_DOWN, {"tongueblenddown"}},
		{XRTracker::FT_TONGUE_CURL_UP, {"tonguecurlup"}},
		{XRTracker::FT_TONGUE_SQUISH, {"tonguesquish"}},
		{XRTracker::FT_TONGUE_FLAT, {"tongueflat"}},
		{XRTracker::FT_TONGUE_TWIST_RIGHT, {"tonguetwistright"}},
		{XRTracker::FT_TONGUE_TWIST_LEFT, {"tonguetwistleft"}},
		{XRTracker::FT_SOFT_PALATE_CLOSE, {"softpalateclose"}},
		{XRTracker::FT_THROAT_SWALLOW, {"throatswallow"}},
		{XRTracker::FT_NECK_FLEX_RIGHT, {"neckflexright"}},
		{XRTracker::FT_NECK_FLEX_LEFT, {"neckflexleft"}},
		{XRTracker::FT_EYE_CLOSED, {"eyeclosed"}}, {XRTracker::FT_EYE_WIDE, {"eyewide"}},
		{XRTracker::FT_EYE_SQUINT, {"eyesquint"}},
		{XRTracker::FT_EYE_DILATION, {"eyedilation"}},
		{XRTracker::FT_EYE_CONSTRICT, {"eyeconstrict"}},
		{XRTracker::FT_BROW_DOWN_RIGHT, {"browdownright", "browlowererr"}},
		{XRTracker::FT_BROW_DOWN_LEFT, {"browdownleft", "browlowererl"}},
		{XRTracker::FT_BROW_DOWN, {"browdown"}},
		{XRTracker::FT_BROW_UP_RIGHT, {"browupright"}},
		{XRTracker::FT_BROW_UP_LEFT, {"browupleft"}}, {XRTracker::FT_BROW_UP, {"browup"}},
		{XRTracker::FT_NOSE_SNEER, {"nosesneer"}},
		{XRTracker::FT_NASAL_DILATION, {"nasaldilation"}},
		{XRTracker::FT_NASAL_CONSTRICT, {"nasalconstrict"}},
		{XRTracker::FT_CHEEK_PUFF, {"cheekpuff"}},
		{XRTracker::FT_CHEEK_SUCK, {"cheeksuck"}},
		{XRTracker::FT_CHEEK_SQUINT, {"cheeksquint"}},
		{XRTracker::FT_LIP_SUCK_UPPER, {"lipsuckupper", "mouthrollupper", "mouthupperinside"}},
		{XRTracker::FT_LIP_SUCK_LOWER, {"lipsucklower", "mouthrolllower", "mouthlowerinside"}},
		{XRTracker::FT_LIP_SUCK, {"lipsuck"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {"lipfunnelupper", "mouthupperoverturn"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {"lipfunnellower", "mouthloweroverturn"}},
		{XRTracker::FT_LIP_FUNNEL, {"lipfunnel", "mouthfunnel"}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {"lippuckerupper"}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {"lippuckerlower"}},
		{XRTracker::FT_LIP_PUCKER, {"lippucker", "mouthpucker", "mouthpout"}},
		{XRTracker::FT_MOUTH_UPPER_UP, {"mouthupperup"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {"mouthlowerdown"}},
		{XRTracker::FT_MOUTH_OPEN, {"mouthopen"}},
		{XRTracker::FT_MOUTH_RIGHT, {"mouthright"}},
		{XRTracker::FT_MOUTH_LEFT, {"mouthleft"}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT, {"mouthsmileright", "lipcornerpullerr"}},
		{XRTracker::FT_MOUTH_SMILE_LEFT, {"mouthsmileleft", "lipcornerpullerl"}},
		{XRTracker::FT_MOUTH_SMILE, {"mouthsmile"}},
		{XRTracker::FT_MOUTH_SAD_RIGHT, {"mouthsadright"}},
		{XRTracker::FT_MOUTH_SAD_LEFT, {"mouthsadleft"}},
		{XRTracker::FT_MOUTH_SAD, {"mouthsad"}},
		{XRTracker::FT_MOUTH_STRETCH, {"mouthstretch"}},
		{XRTracker::FT_MOUTH_DIMPLE, {"mouthdimple"}},
		{XRTracker::FT_MOUTH_TIGHTENER, {"mouthtightener"}},
		{XRTracker::FT_MOUTH_PRESS, {"mouthpress"}}};

	// Convert the name to lower-case and strip non-alphanumeric characters.
	const String name = String(p_name).to_lower().remove_char('_');

	// Iterate through the blend map.
	for (const blend_map_entry& entry : blend_map) {
		for (const char* n : entry.name) {
			if (n == nullptr) {
				break;
			}

			if (name == n) {
				return entry.blend;
			}
		}
	}

	// Blend shape not found.
	return -1;
}

// This method adds all the identified XRFaceTracker blend shapes of
// the mesh to the p_blend_mapping map. The map is indexed by the
// XRFaceTracker blend shape, and the value is the index of the mesh
// blend shape.
static void identify_face_blend_shapes(RBMap<int, int>& p_blend_mapping, const Ref<Mesh>& mesh)
{
	// Find all blend shapes.
	const int count = mesh->get_blend_shape_count();
	for (int i = 0; i < count; i++) {
		const int blend = find_face_blend_shape(mesh->get_blend_shape_name(i));
		if (blend >= 0) {
			p_blend_mapping[blend] = i;
		}
	}
}

// This method removes any unified blend shapes from the p_blend_mapping map
// if all the individual blend shapes are found and going to be driven.
static void remove_driven_unified_blend_shapes(RBMap<int, int>& p_blend_mapping)
{
	// Entry for unified blend table.
	struct unified_blend_entry
	{
		int unified;
		int individual[4];
	};

	// Table of unified blend shapes.
	//
	// This table consists of:
	// - The XRFaceTracker unified blend shape.
	// - The individual blend shapes that make up the unified blend shape.
	static constexpr unified_blend_entry unified_blends[] = {
		{XRTracker::FT_EYE_CLOSED,
			{XRTracker::FT_EYE_CLOSED_RIGHT, XRTracker::FT_EYE_CLOSED_LEFT, -1, -1}},
		{XRTracker::FT_EYE_WIDE,
			{XRTracker::FT_EYE_WIDE_RIGHT, XRTracker::FT_EYE_WIDE_LEFT, -1, -1}},
		{XRTracker::FT_EYE_SQUINT,
			{XRTracker::FT_EYE_SQUINT_RIGHT, XRTracker::FT_EYE_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_EYE_DILATION,
			{XRTracker::FT_EYE_DILATION_RIGHT, XRTracker::FT_EYE_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_EYE_CONSTRICT,
			{XRTracker::FT_EYE_CONSTRICT_RIGHT, XRTracker::FT_EYE_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_RIGHT,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_LEFT,
			{XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT,
				XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT}},
		{XRTracker::FT_BROW_UP_RIGHT,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_UP_LEFT,
			{XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT, -1, -1}},
		{XRTracker::FT_BROW_UP,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT,
				XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT}},
		{XRTracker::FT_NOSE_SNEER,
			{XRTracker::FT_NOSE_SNEER_RIGHT, XRTracker::FT_NOSE_SNEER_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_DILATION, {XRTracker::FT_NASAL_DILATION_RIGHT,
											   XRTracker::FT_NASAL_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_CONSTRICT, {XRTracker::FT_NASAL_CONSTRICT_RIGHT,
												XRTracker::FT_NASAL_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_PUFF,
			{XRTracker::FT_CHEEK_PUFF_RIGHT, XRTracker::FT_CHEEK_PUFF_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SUCK,
			{XRTracker::FT_CHEEK_SUCK_RIGHT, XRTracker::FT_CHEEK_SUCK_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SQUINT,
			{XRTracker::FT_CHEEK_SQUINT_RIGHT, XRTracker::FT_CHEEK_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_UPPER, {XRTracker::FT_LIP_SUCK_UPPER_RIGHT,
											   XRTracker::FT_LIP_SUCK_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_LOWER, {XRTracker::FT_LIP_SUCK_LOWER_RIGHT,
											   XRTracker::FT_LIP_SUCK_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK,
			{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, XRTracker::FT_LIP_SUCK_UPPER_LEFT,
				XRTracker::FT_LIP_SUCK_LOWER_RIGHT, XRTracker::FT_LIP_SUCK_LOWER_LEFT}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL,
			{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, XRTracker::FT_LIP_FUNNEL_UPPER_LEFT,
				XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, XRTracker::FT_LIP_FUNNEL_LOWER_LEFT}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {XRTracker::FT_LIP_PUCKER_UPPER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {XRTracker::FT_LIP_PUCKER_LOWER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER,
			{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, XRTracker::FT_LIP_PUCKER_UPPER_LEFT,
				XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, XRTracker::FT_LIP_PUCKER_LOWER_LEFT}},
		{XRTracker::FT_MOUTH_UPPER_UP, {XRTracker::FT_MOUTH_UPPER_UP_RIGHT,
											   XRTracker::FT_MOUTH_UPPER_UP_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
												 XRTracker::FT_MOUTH_LOWER_DOWN_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_OPEN,
			{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, XRTracker::FT_MOUTH_UPPER_UP_LEFT,
				XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT, XRTracker::FT_MOUTH_LOWER_DOWN_LEFT}},
		{XRTracker::FT_MOUTH_RIGHT,
			{XRTracker::FT_MOUTH_UPPER_RIGHT, XRTracker::FT_MOUTH_LOWER_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_LEFT,
			{XRTracker::FT_MOUTH_UPPER_LEFT, XRTracker::FT_MOUTH_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE_LEFT,
			{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, XRTracker::FT_MOUTH_CORNER_SLANT_LEFT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				XRTracker::FT_MOUTH_CORNER_PULL_LEFT,
				XRTracker::FT_MOUTH_CORNER_SLANT_LEFT}},
		{XRTracker::FT_MOUTH_SAD_RIGHT,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD_LEFT,
			{XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT,
				XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT}},
		{XRTracker::FT_MOUTH_STRETCH,
			{XRTracker::FT_MOUTH_STRETCH_RIGHT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_DIMPLE,
			{XRTracker::FT_MOUTH_DIMPLE_RIGHT, XRTracker::FT_MOUTH_DIMPLE_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_TIGHTENER, {XRTracker::FT_MOUTH_TIGHTENER_RIGHT,
												XRTracker::FT_MOUTH_TIGHTENER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_PRESS,
			{XRTracker::FT_MOUTH_PRESS_RIGHT, XRTracker::FT_MOUTH_PRESS_LEFT, -1, -1}}};

	// Remove unified blend shapes if individual blend shapes are found.
	for (const unified_blend_entry& entry : unified_blends) {
		// Check if all individual blend shapes are found.
		bool found = true;
		for (const int i : entry.individual) {
			if (i >= 0 && !p_blend_mapping.find(i)) {
				found = false;
				break;
			}
		}

		// If all individual blend shapes are found then remove the unified blend shape.
		if (found) {
			p_blend_mapping.erase(entry.unified);
		}
	}
}


void XRFaceModifier3D::set_face_tracker(const String& p_tracker_name)
{
	tracker_name = p_tracker_name;
	if (is_inside_tree()) {
		_get_blend_data();
	}
}

String XRFaceModifier3D::get_face_tracker() const { return tracker_name; }

void XRFaceModifier3D::set_target(const NodePath& p_target)
{
	target = p_target;

	if (is_inside_tree()) {
		_get_blend_data();
	}
}

NodePath XRFaceModifier3D::get_target() const { return target; }

void XRFaceModifier3D::_get_blend_data()
{
	// This method constructs the blend mapping from the XRFaceTracker
	// blend shapes to the available blend shapes of the target mesh. It does this
	// by:
	//
	// 1. Identifying the blend shapes of the target mesh and identifying what
	//    XRFaceTracker blend shape they correspond to. The results are
	//    placed in the blend_mapping map.
	// 2. Prevent over-driving facial blend-shapes by removing any unified blend
	//    shapes from the map if all the individual blend shapes are already
	//    found and going to be driven.

	blend_mapping.clear();

	// Get the target MeshInstance3D.
	const MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the mesh.
	const Ref<Mesh> mesh = mesh_instance->get_mesh();
	if (mesh.is_null()) {
		return;
	}

	// Identify all face blend shapes and populate the map.
	identify_face_blend_shapes(blend_mapping, mesh);

	// Remove the unified blend shapes if all the individual blend shapes are found.
	remove_driven_unified_blend_shapes(blend_mapping);
}

void XRFaceModifier3D::_update_face_blends() const
{
	// Get the XR Server.
	if (!XRServer::is_initialized()) {
		return;
	}

	// Get the face tracker.
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_FACE) {
		return;
	}

	// Get the face mesh.
	MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the blend weights.
	const PackedFloat32Array weights = tracker->get_blend_shapes();

	// Apply all the face blend weights to the mesh.
	for (const KeyValue<int, int>& it : blend_mapping) {
		mesh_instance->set_blend_shape_value(it.value, weights[it.key]);
	}
}

void XRFaceModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_blend_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		blend_mapping.clear();
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_face_blends();
	} break;
	default: {
	} break;
	}
}



MeshInstance3D* XRFaceModifier3D::get_mesh_instance() const { return nullptr; }
