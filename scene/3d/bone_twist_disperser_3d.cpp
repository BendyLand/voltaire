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

void BoneTwistDisperser3D::set_mutable_bone_axes(bool p_enabled) { mutable_bone_axes = p_enabled; }

bool BoneTwistDisperser3D::are_bone_axes_mutable() const { return mutable_bone_axes; }

String BoneTwistDisperser3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone.name;
}

int BoneTwistDisperser3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone.bone;
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

String BoneTwistDisperser3D::get_joint_bone_name(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<DisperseJointSetting>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].joint.name;
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

void BoneTwistDisperser3D::_make_all_joints_dirty()
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_joints_dirty(i);
	}
}

int BoneTwistDisperser3D::get_setting_count() const { return (int)settings.size(); }

void BoneTwistDisperser3D::clear_settings() { set_setting_count(0); }

BoneTwistDisperser3D::~BoneTwistDisperser3D() { clear_settings(); }


