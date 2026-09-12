/**************************************************************************/
/*  bone_twist_disperser_3d.cpp                                           */
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

#include "bone_twist_disperser_3d.h"

void BoneTwistDisperser3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_make_all_joints_dirty();
	} break;
	}
}

void BoneTwistDisperser3D::_set_active(bool p_active)
{
	if (p_active) {
		_make_all_joints_dirty();
	}
}

void BoneTwistDisperser3D::_skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new)
{
	_make_all_joints_dirty();
}

// Setting.

void BoneTwistDisperser3D::set_mutable_bone_axes(bool p_enabled) { mutable_bone_axes = p_enabled; }

bool BoneTwistDisperser3D::are_bone_axes_mutable() const { return mutable_bone_axes; }

void BoneTwistDisperser3D::set_root_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone.name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_root_bone(p_index, sk->find_bone(settings[p_index]->root_bone.name));
	}
}

String BoneTwistDisperser3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone.name;
}

void BoneTwistDisperser3D::set_root_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	bool changed = settings[p_index]->root_bone.bone != p_bone;
	settings[p_index]->root_bone.bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (settings[p_index]->root_bone.bone <= -1 ||
			settings[p_index]->root_bone.bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + ": Root bone index '" + itos(p_bone) +
						  "' is out of range!");
			settings[p_index]->root_bone.bone = -1;
		}
		else {
			settings[p_index]->root_bone.name =
				sk->get_bone_name(settings[p_index]->root_bone.bone);
		}
	}
	if (changed) {
		_make_joints_dirty(p_index);
	}
}

int BoneTwistDisperser3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone.bone;
}

void BoneTwistDisperser3D::set_end_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone.name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_end_bone(p_index, sk->find_bone(settings[p_index]->end_bone.name));
	}
}

String BoneTwistDisperser3D::get_end_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->end_bone.name;
}

int BoneTwistDisperser3D::get_end_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->end_bone.bone;
}

bool BoneTwistDisperser3D::is_end_bone_extended(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->extend_end_bone;
}

void BoneTwistDisperser3D::set_end_bone_direction(int p_index, BoneDirection p_bone_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_direction = p_bone_direction;
}

SkeletonModifier3D::BoneDirection BoneTwistDisperser3D::get_end_bone_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index]->end_bone_direction;
}

bool BoneTwistDisperser3D::is_twist_from_rest(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), true);
	return settings[p_index]->twist_from_rest;
}

void BoneTwistDisperser3D::set_twist_from(int p_index, const Quaternion& p_from)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->twist_from = p_from;
}

Quaternion BoneTwistDisperser3D::get_twist_from(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	return settings[p_index]->twist_from;
}

void BoneTwistDisperser3D::_update_reference_bone(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	if (joints.size() >= 2) {
		if (settings[p_index]->extend_end_bone) {
			settings[p_index]->reference_bone = settings[p_index]->end_bone;
			_update_curve(p_index);
			return;
		}
		else {
			Skeleton3D* sk = get_skeleton();
			if (sk) {
				int parent = sk->get_bone_parent(settings[p_index]->end_bone.bone);
				if (parent >= 0) {
					settings[p_index]->reference_bone.bone = parent;
					settings[p_index]->reference_bone.name = sk->get_bone_name(parent);
					_update_curve(p_index);
					return;
				}
			}
		}
	}
	settings[p_index]->reference_bone.bone = -1;
	settings[p_index]->reference_bone.name = String();
}

void BoneTwistDisperser3D::_update_curve(int p_index)
{
	Ref<Curve> curve = settings[p_index]->damping_curve;
	if (curve.is_null()) {
		return;
	}
	LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	float unit = (int)joints.size() > 0 ? (1.0 / float((int)joints.size() - 1)) : 0.0;
	for (uint32_t i = 0; i < joints.size(); i++) {
		joints[i].custom_amount = curve->sample_baked(i * unit);
	}
}

String BoneTwistDisperser3D::get_reference_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->reference_bone.name;
}

int BoneTwistDisperser3D::get_reference_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->reference_bone.bone;
}

BoneTwistDisperser3D::DisperseMode BoneTwistDisperser3D::get_disperse_mode(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), DISPERSE_MODE_EVEN);
	return settings[p_index]->disperse_mode;
}

void BoneTwistDisperser3D::set_weight_position(int p_index, float p_position)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->weight_position = p_position;
}

float BoneTwistDisperser3D::get_weight_position(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0);
	return settings[p_index]->weight_position;
}

Ref<Curve> BoneTwistDisperser3D::get_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->damping_curve;
}

// Individual joints.

String BoneTwistDisperser3D::get_joint_bone_name(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].joint.name;
}

void BoneTwistDisperser3D::_set_joint_bone(int p_index, int p_joint, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint].joint.bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (joints[p_joint].joint.bone <= -1 ||
			joints[p_joint].joint.bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + " : Joint: " + itos(p_joint) +
						  ": bone index '" + itos(p_bone) + "' is out of range!");
			joints[p_joint].joint.bone = -1;
		}
		else {
			joints[p_joint].joint.name = sk->get_bone_name(joints[p_joint].joint.bone);
		}
	}
}

int BoneTwistDisperser3D::get_joint_bone(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].joint.bone;
}

int BoneTwistDisperser3D::get_joint_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	return joints.size();
}

void BoneTwistDisperser3D::set_joint_twist_amount(int p_index, int p_joint, float p_amount)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint].custom_amount = p_amount;
}

float BoneTwistDisperser3D::get_joint_twist_amount(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint].custom_amount;
}


void BoneTwistDisperser3D::_validate_bone_names()
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		// Prior bone name.
		if (!settings[i]->root_bone.name.is_empty()) {
			set_root_bone_name(i, settings[i]->root_bone.name);
		}
		else if (settings[i]->root_bone.bone != -1) {
			set_root_bone(i, settings[i]->root_bone.bone);
		}
		// Prior bone name.
		if (!settings[i]->end_bone.name.is_empty()) {
			set_end_bone_name(i, settings[i]->end_bone.name);
		}
		else if (settings[i]->end_bone.bone != -1) {
			set_end_bone(i, settings[i]->end_bone.bone);
		}
	}
}

void BoneTwistDisperser3D::_make_all_joints_dirty()
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_joints_dirty(i);
	}
}

void BoneTwistDisperser3D::_update_joints(int p_index)
{
	Skeleton3D* sk = get_skeleton();
	int current_bone = settings[p_index]->end_bone.bone;
	int root_bone = settings[p_index]->root_bone.bone;
	if (!sk || current_bone < 0 || root_bone < 0) {
		set_joint_count(p_index, 0);
		settings[p_index]->joints_dirty = false;
		return;
	}

	// Validation.
	bool valid = false;
	while (current_bone >= 0) {
		current_bone = sk->get_bone_parent(current_bone);
		if (current_bone == root_bone) {
			valid = true;
			break;
		}
	}

	if (!valid) {
		set_joint_count(p_index, 0);
		_update_reference_bone(p_index);
		settings[p_index]->joints_dirty = false;
		ERR_FAIL_EDMSG("End bone must be a child of the root bone.");
	}

	Vector<int> new_joints;
	current_bone = settings[p_index]->end_bone.bone;
	while (current_bone != root_bone) {
		new_joints.push_back(current_bone);
		current_bone = sk->get_bone_parent(current_bone);
	}
	new_joints.push_back(current_bone);
	new_joints.reverse();

	set_joint_count(p_index, new_joints.size());
	for (uint32_t i = 0; i < new_joints.size(); i++) {
		_set_joint_bone(p_index, i, new_joints[i]);
	}

	_update_reference_bone(p_index);
	settings[p_index]->joints_dirty = false;
}

int BoneTwistDisperser3D::get_setting_count() const { return (int)settings.size(); }

void BoneTwistDisperser3D::clear_settings() { set_setting_count(0); }

void BoneTwistDisperser3D::_process_modification(double p_delta)
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	for (BoneTwistDisperser3DSetting* setting : settings) {
		if (!setting || setting->reference_bone.bone < 0) {
			continue;
		}
		LocalVector<DisperseJointSetting>& joints = setting->joints;
		// Calc amount.
		int actual_joint_size =
			setting->extend_end_bone ? (int)joints.size() : (int)joints.size() - 1;
		if (actual_joint_size <= 1) {
			continue;
		}
		if (setting->disperse_mode == DISPERSE_MODE_EVEN) {
			double div = 1.0 / actual_joint_size;
			for (int i = 0; i < actual_joint_size; i++) {
				joints[i].amount = ((double)i + 1.0) * div;
			}
		}
		else if (setting->disperse_mode == DISPERSE_MODE_WEIGHTED) {
			// Assign length for each bone.
			double total_length = 0.0;
			double weight_sub = 1.0 - setting->weight_position;
			if (mutable_bone_axes) {
				for (int i = 0; i < actual_joint_size; i++) {
					double length = 0.0;
					if (i == 0) {
						length =
							skeleton->get_bone_pose_position(joints[i + 1].joint.bone).length() *
							setting->weight_position;
					}
					else if (i == actual_joint_size - 1) {
						length = skeleton->get_bone_pose_position(joints[i].joint.bone).length() *
								 weight_sub;
					}
					else {
						length =
							skeleton->get_bone_pose_position(joints[i].joint.bone).length() *
								setting->weight_position +
							skeleton->get_bone_pose_position(joints[i + 1].joint.bone).length() *
								weight_sub;
					}
					total_length += length;
					joints[i].amount = total_length;
				}
			}
			else {
				for (int i = 0; i < actual_joint_size; i++) {
					double length = 0.0;
					if (i == 0) {
						length = skeleton->get_bone_rest(joints[i + 1].joint.bone).origin.length() *
								 setting->weight_position;
					}
					else if (i == actual_joint_size - 1) {
						length = skeleton->get_bone_rest(joints[i].joint.bone).origin.length() *
								 weight_sub;
					}
					else {
						length = skeleton->get_bone_rest(joints[i].joint.bone).origin.length() *
									 setting->weight_position +
								 skeleton->get_bone_rest(joints[i + 1].joint.bone).origin.length() *
									 weight_sub;
					}
					total_length += length;
					joints[i].amount = total_length;
				}
			}
			if (Math::is_zero_approx(total_length)) {
				continue;
			}
			// Normalize.
			double div = 1.0 / total_length;
			for (int i = 0; i < actual_joint_size; i++) {
				joints[i].amount *= div;
			}
		}
		else {
			for (int i = 0; i < actual_joint_size; i++) {
				joints[i].amount = joints[i].custom_amount;
			}
		}
		int end = actual_joint_size - 1;
		joints[end].amount -= 1.0; // Remove twist from current pose.

		// Retrieve axes.
		if (mutable_bone_axes) {
			for (int i = 0; i < end; i++) {
				joints[i].axis =
					skeleton->get_bone_pose_position(joints[i + 1].joint.bone).normalized();
				if (joints[i].axis.is_zero_approx() && i > 0) {
					joints[i].axis = joints[i - 1].axis;
				}
			}
		}
		else {
			for (int i = 0; i < end; i++) {
				joints[i].axis =
					skeleton->get_bone_rest(joints[i + 1].joint.bone).origin.normalized();
				if (joints[i].axis.is_zero_approx() && i > 0) {
					joints[i].axis = joints[i - 1].axis;
				}
			}
		}

		if (!setting->extend_end_bone) {
			joints[end].axis = mutable_bone_axes
								   ? skeleton->get_bone_pose_position(setting->end_bone.bone)
								   : skeleton->get_bone_rest(setting->end_bone.bone).origin;
			joints[end].axis.normalize();
		}
		else if (setting->end_bone_direction == BONE_DIRECTION_FROM_PARENT) {
			joints[end].axis =
				skeleton->get_bone_rest(setting->end_bone.bone)
					.basis.xform_inv(mutable_bone_axes
										 ? skeleton->get_bone_pose_position(setting->end_bone.bone)
										 : skeleton->get_bone_rest(setting->end_bone.bone).origin);
			joints[end].axis.normalize();
		}
		else {
			joints[end].axis =
				get_vector_from_bone_axis(static_cast<BoneAxis>((int)setting->end_bone_direction));
		}
		if (joints[end].axis.is_zero_approx() && end > 0) {
			joints[end].axis = joints[end - 1].axis;
		}

		// Extract twist.
		Quaternion twist_rest = setting->twist_from_rest
									? skeleton->get_bone_rest(setting->reference_bone.bone)
										  .basis.get_rotation_quaternion()
									: setting->twist_from.normalized();
		Quaternion ref_rot =
			twist_rest.inverse() * skeleton->get_bone_pose_rotation(setting->reference_bone.bone);
		ref_rot.normalize();
		double twist = get_roll_angle(ref_rot, joints[end].axis);

		// Apply twist for each bone by their amount.
		// Twist parent, then cancel all twists caused by this modifier in child, and re-apply
		// accumulated twist.
		Quaternion prev_rot;
		if (mutable_bone_axes) {
			for (int i = 0; i < actual_joint_size; i++) {
				int bn = joints[i].joint.bone;
				Quaternion cur_rot = Quaternion(joints[i].axis, twist * joints[i].amount);
				skeleton->set_bone_pose_rotation(
					bn, prev_rot.inverse() * skeleton->get_bone_pose_rotation(bn) * cur_rot);
				prev_rot = cur_rot;
			}
		}
		else {
			for (int i = 0; i < actual_joint_size; i++) {
				int bn = joints[i].joint.bone;
				Quaternion cur_rot = Quaternion(joints[i].axis, twist * joints[i].amount);
				skeleton->set_bone_pose_rotation(

					bn, prev_rot.inverse() * skeleton->get_bone_pose_rotation(bn) * cur_rot);
				prev_rot = cur_rot;
			}
		}
	}
}

BoneTwistDisperser3D::~BoneTwistDisperser3D() { clear_settings(); }


