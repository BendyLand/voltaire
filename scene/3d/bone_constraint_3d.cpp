/**************************************************************************/
/*  bone_constraint_3d.cpp                                                */
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

#include "bone_constraint_3d.h"

#include "scene/3d/ik_modifier_3d.h"
#include "scene/3d/look_at_modifier_3d.h"

BoneConstraint3D::BoneConstraint3D(ConstraintType p_type) :
		constraint_type(p_type) {
}

BoneConstraint3D::~BoneConstraint3D() {
	clear_settings();
}

void BoneConstraint3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_make_velocity_joints_dirty();
			_make_all_twist_joints_dirty();
		} break;
		default:
			break;
	}
}

void BoneConstraint3D::_set_active(bool p_active) {
	if (p_active) {
		reset();
		_make_all_twist_joints_dirty();
	}
}

void BoneConstraint3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

void BoneConstraint3D::set_constraint_type(ConstraintType p_type) {
	if (constraint_type == p_type) {
		return;
	}
	constraint_type = p_type;
	reset();
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
	update_configuration_warnings();
}

PackedStringArray BoneConstraint3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (constraint_type == CONSTRAINT_AIM) {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i].use_euler && get_axis_from_bone_axis(settings[i].forward_axis) == settings[i].primary_rotation_axis) {
				warnings.push_back(vformat(RTR("Forward axis and primary rotation axis must not be parallel in setting %s."), itos(i)));
			}
		}
	}
	return warnings;
}

void BoneConstraint3D::set_setting_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	settings.resize(p_count);
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

int BoneConstraint3D::get_setting_count() const {
	return (int)settings.size();
}

void BoneConstraint3D::clear_settings() {
	settings.clear();
	velocity_joints.clear();
	velocity_bones.clear();
}

void BoneConstraint3D::_validate_bone_names() {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	if (!target_bone_name.is_empty()) {
		target_bone = sk->find_bone(target_bone_name);
	}
	for (uint32_t i = 0; i < settings.size(); i++) {
		if (!settings[i].apply_bone_name.is_empty()) {
			settings[i].apply_bone = sk->find_bone(settings[i].apply_bone_name);
		}
		if (!settings[i].reference_bone_name.is_empty()) {
			settings[i].reference_bone = sk->find_bone(settings[i].reference_bone_name);
		}
		if (!settings[i].root_bone.name.is_empty()) {
			settings[i].root_bone.bone = sk->find_bone(settings[i].root_bone.name);
		}
		if (!settings[i].end_bone.name.is_empty()) {
			settings[i].end_bone.bone = sk->find_bone(settings[i].end_bone.name);
		}
	}
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

void BoneConstraint3D::set_amount(int p_index, float p_amount) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].amount = p_amount;
}

float BoneConstraint3D::get_amount(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].amount;
}

void BoneConstraint3D::set_apply_bone_name(int p_index, const String &p_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].apply_bone = sk->find_bone(p_name);
	}
}

String BoneConstraint3D::get_apply_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].apply_bone_name;
}

void BoneConstraint3D::set_apply_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].apply_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_apply_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].apply_bone;
}

void BoneConstraint3D::set_reference_type(int p_index, ReferenceType p_type) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_type = p_type;
}

BoneConstraint3D::ReferenceType BoneConstraint3D::get_reference_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), REFERENCE_TYPE_BONE);
	return settings[p_index].reference_type;
}

void BoneConstraint3D::set_reference_bone_name(int p_index, const String &p_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].reference_bone = sk->find_bone(p_name);
	}
}

String BoneConstraint3D::get_reference_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].reference_bone_name;
}

void BoneConstraint3D::set_reference_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].reference_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_reference_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].reference_bone;
}

void BoneConstraint3D::set_reference_node(int p_index, const NodePath &p_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_node = p_node;
}

NodePath BoneConstraint3D::get_reference_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index].reference_node;
}

void BoneConstraint3D::set_relative(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].relative = p_enabled;
}

bool BoneConstraint3D::is_relative(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].is_relative_effective(constraint_type);
}

void BoneConstraint3D::set_additive(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].additive = p_enabled;
}

bool BoneConstraint3D::is_additive(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].additive;
}

// Aim properties.

void BoneConstraint3D::set_forward_axis(int p_index, BoneAxis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].forward_axis = p_axis;
	update_configuration_warnings();
}

SkeletonModifier3D::BoneAxis BoneConstraint3D::get_forward_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_AXIS_PLUS_Y);
	return settings[p_index].forward_axis;
}

void BoneConstraint3D::set_use_euler(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].use_euler = p_enabled;
	update_configuration_warnings();
}

bool BoneConstraint3D::is_using_euler(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].use_euler;
}

void BoneConstraint3D::set_primary_rotation_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].primary_rotation_axis = p_axis;
	update_configuration_warnings();
}

Vector3::Axis BoneConstraint3D::get_primary_rotation_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].primary_rotation_axis;
}

void BoneConstraint3D::set_use_secondary_rotation(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].use_secondary_rotation = p_enabled;
}

bool BoneConstraint3D::is_using_secondary_rotation(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].use_secondary_rotation;
}

// Convert Transform properties.

void BoneConstraint3D::set_apply_transform_mode(int p_index, TransformMode p_transform_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_transform_mode = p_transform_mode;
}

BoneConstraint3D::TransformMode BoneConstraint3D::get_apply_transform_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	return settings[p_index].apply_transform_mode;
}

void BoneConstraint3D::set_apply_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_axis = p_axis;
}

Vector3::Axis BoneConstraint3D::get_apply_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].apply_axis;
}

void BoneConstraint3D::set_apply_range_min(int p_index, float p_range_min) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_range_min = p_range_min;
}

float BoneConstraint3D::get_apply_range_min(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].apply_range_min;
}

void BoneConstraint3D::set_apply_range_max(int p_index, float p_range_max) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_range_max = p_range_max;
}

float BoneConstraint3D::get_apply_range_max(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].apply_range_max;
}

void BoneConstraint3D::set_reference_transform_mode(int p_index, TransformMode p_transform_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_transform_mode = p_transform_mode;
}

BoneConstraint3D::TransformMode BoneConstraint3D::get_reference_transform_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	return settings[p_index].reference_transform_mode;
}

void BoneConstraint3D::set_reference_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_axis = p_axis;
}

Vector3::Axis BoneConstraint3D::get_reference_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].reference_axis;
}

void BoneConstraint3D::set_reference_range_min(int p_index, float p_range_min) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_range_min = p_range_min;
}

float BoneConstraint3D::get_reference_range_min(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].reference_range_min;
}

void BoneConstraint3D::set_reference_range_max(int p_index, float p_range_max) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_range_max = p_range_max;
}

float BoneConstraint3D::get_reference_range_max(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].reference_range_max;
}

// Copy Transform properties.

void BoneConstraint3D::set_copy_flags(int p_index, uint32_t p_copy_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].copy_flags = p_copy_flags;
}

uint32_t BoneConstraint3D::get_copy_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].copy_flags;
}

void BoneConstraint3D::set_copy_position(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_POSITION;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_POSITION;
	}
}

bool BoneConstraint3D::is_position_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_POSITION) != 0;
}

void BoneConstraint3D::set_copy_rotation(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_ROTATION;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_ROTATION;
	}
}

bool BoneConstraint3D::is_rotation_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_ROTATION) != 0;
}

void BoneConstraint3D::set_copy_scale(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_SCALE;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_SCALE;
	}
}

bool BoneConstraint3D::is_scale_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_SCALE) != 0;
}

void BoneConstraint3D::set_axis_flags(int p_index, uint32_t p_axis_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].axis_flags = p_axis_flags;
}

uint32_t BoneConstraint3D::get_axis_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].axis_flags;
}

void BoneConstraint3D::set_axis_x_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_X;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_X;
	}
}

bool BoneConstraint3D::is_axis_x_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_X) != 0;
}

void BoneConstraint3D::set_axis_y_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_Y;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_Y;
	}
}

bool BoneConstraint3D::is_axis_y_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_Y) != 0;
}

void BoneConstraint3D::set_axis_z_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_Z;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_Z;
	}
}

bool BoneConstraint3D::is_axis_z_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_Z) != 0;
}

void BoneConstraint3D::set_invert_flags(int p_index, uint32_t p_axis_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].invert_flags = p_axis_flags;
}

uint32_t BoneConstraint3D::get_invert_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].invert_flags;
}

void BoneConstraint3D::set_axis_x_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_X;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_X;
	}
}

bool BoneConstraint3D::is_axis_x_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_X) != 0;
}

void BoneConstraint3D::set_axis_y_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_Y;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_Y;
	}
}

bool BoneConstraint3D::is_axis_y_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_Y) != 0;
}

void BoneConstraint3D::set_axis_z_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_Z;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_Z;
	}
}

bool BoneConstraint3D::is_axis_z_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_Z) != 0;
}

// --- BoneTarget properties ---

void BoneConstraint3D::set_bone_name(const String &p_bone_name) {
	target_bone_name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		target_bone = sk->find_bone(target_bone_name);
	}
}

String BoneConstraint3D::get_bone_name() const {
	return target_bone_name;
}

void BoneConstraint3D::set_bone(int p_bone) {
	target_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		target_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_bone() const {
	return target_bone;
}

// --- Chain root/end properties (LimitAngularVelocity & TwistDisperser) ---

void BoneConstraint3D::set_root_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].root_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].root_bone.bone = sk->find_bone(p_bone_name);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

String BoneConstraint3D::get_root_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].root_bone.name;
}

void BoneConstraint3D::set_root_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].root_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].root_bone.name = sk->get_bone_name(p_bone);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

int BoneConstraint3D::get_root_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].root_bone.bone;
}

void BoneConstraint3D::set_end_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].end_bone.bone = sk->find_bone(p_bone_name);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

String BoneConstraint3D::get_end_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].end_bone.name;
}

void BoneConstraint3D::set_end_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].end_bone.name = sk->get_bone_name(p_bone);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

int BoneConstraint3D::get_end_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].end_bone.bone;
}

// --- LimitAngularVelocity properties & helpers ---

void BoneConstraint3D::set_chain_count(int p_count) {
	set_setting_count(p_count);
}

int BoneConstraint3D::get_chain_count() const {
	return get_setting_count();
}

void BoneConstraint3D::clear_chains() {
	set_chain_count(0);
}

String BoneConstraint3D::get_joint_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)velocity_joints.size(), String());
	return velocity_joints[p_index].name;
}

int BoneConstraint3D::get_joint_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)velocity_joints.size(), -1);
	return velocity_joints[p_index].bone;
}

int BoneConstraint3D::get_velocity_joint_count() const {
	return (int)velocity_joints.size();
}

void BoneConstraint3D::set_max_angular_velocity(double p_angular_velocity) {
	max_angular_velocity = p_angular_velocity;
}

double BoneConstraint3D::get_max_angular_velocity() const {
	return max_angular_velocity;
}

void BoneConstraint3D::set_exclude(bool p_exclude) {
	exclude = p_exclude;
	reset();
}

bool BoneConstraint3D::is_exclude() const {
	return exclude;
}

void BoneConstraint3D::reset() {
	velocity_init_needed = true;
}

void BoneConstraint3D::_make_velocity_joints_dirty() {
	velocity_joints_dirty = true;
	velocity_init_needed = true;
}

bool BoneConstraint3D::_is_velocity_joint_contained(int p_bone) const {
	for (const BoneJoint &joint : velocity_joints) {
		if (joint.bone == p_bone) {
			return true;
		}
	}
	return false;
}

void BoneConstraint3D::_update_velocity_joints() {
	velocity_joints.clear();
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		velocity_joints_dirty = false;
		return;
	}

	for (uint32_t i = 0; i < settings.size(); i++) {
		int root = settings[i].root_bone.bone;
		int end = settings[i].end_bone.bone;
		if (root < 0 || end < 0 || root >= sk->get_bone_count() || end >= sk->get_bone_count()) {
			continue;
		}
		if (root != end && !sk->is_bone_parent_of(end, root)) {
			continue;
		}
		Vector<int> chain_bones;
		int cur = end;
		while (cur != -1) {
			chain_bones.push_back(cur);
			if (cur == root) {
				break;
			}
			cur = sk->get_bone_parent(cur);
		}
		for (int j = chain_bones.size() - 1; j >= 0; j--) {
			int b = chain_bones[j];
			if (!_is_velocity_joint_contained(b)) {
				BoneJoint bj;
				bj.bone = b;
				bj.name = sk->get_bone_name(b);
				velocity_joints.push_back(bj);
			}
		}
	}

	velocity_bones.clear();
	if (exclude) {
		for (int b = 0; b < sk->get_bone_count(); b++) {
			if (!_is_velocity_joint_contained(b)) {
				velocity_bones.push_back(BoneRot(b, Quaternion()));
			}
		}
	} else {
		for (const BoneJoint &joint : velocity_joints) {
			velocity_bones.push_back(BoneRot(joint.bone, Quaternion()));
		}
	}

	velocity_joints_dirty = false;
	velocity_init_needed = true;
}

// --- TwistDisperser properties & helpers ---

void BoneConstraint3D::set_mutable_bone_axes(bool p_enabled) {
	mutable_bone_axes = p_enabled;
	_make_all_twist_joints_dirty();
}

bool BoneConstraint3D::are_bone_axes_mutable() const {
	return mutable_bone_axes;
}

void BoneConstraint3D::set_extend_end_bone(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].extend_end_bone = p_enabled;
	_make_twist_joints_dirty(p_index);
}

bool BoneConstraint3D::is_end_bone_extended(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].extend_end_bone;
}

void BoneConstraint3D::set_end_bone_direction(int p_index, BoneDirection p_bone_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone_direction = p_bone_direction;
	_make_twist_joints_dirty(p_index);
}

SkeletonModifier3D::BoneDirection BoneConstraint3D::get_end_bone_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index].end_bone_direction;
}

void BoneConstraint3D::set_twist_from_rest(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].twist_from_rest = p_enabled;
}

bool BoneConstraint3D::is_twist_from_rest(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), true);
	return settings[p_index].twist_from_rest;
}

void BoneConstraint3D::set_twist_from(int p_index, const Quaternion &p_from) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].twist_from = p_from;
}

Quaternion BoneConstraint3D::get_twist_from(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	return settings[p_index].twist_from;
}

void BoneConstraint3D::set_disperse_mode(int p_index, DisperseMode p_disperse_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].disperse_mode = p_disperse_mode;
	_make_twist_joints_dirty(p_index);
}

BoneConstraint3D::DisperseMode BoneConstraint3D::get_disperse_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), DISPERSE_MODE_EVEN);
	return settings[p_index].disperse_mode;
}

void BoneConstraint3D::set_weight_position(int p_index, float p_position) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].weight_position = p_position;
	_make_twist_joints_dirty(p_index);
}

float BoneConstraint3D::get_weight_position(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].weight_position;
}

void BoneConstraint3D::set_damping_curve(int p_index, const Ref<Curve> &p_damping_curve) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].damping_curve = p_damping_curve;
	_update_twist_curve(p_index);
}

Ref<Curve> BoneConstraint3D::get_damping_curve(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index].damping_curve;
}

String BoneConstraint3D::get_joint_bone_name(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].joint.name;
}

int BoneConstraint3D::get_joint_bone(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].joint.bone;
}

void BoneConstraint3D::set_joint_twist_amount(int p_index, int p_joint, float p_amount) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint].custom_amount = p_amount;
}

float BoneConstraint3D::get_joint_twist_amount(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0.0f);
	return (float)joints[p_joint].custom_amount;
}

void BoneConstraint3D::set_joint_count(int p_index, int p_count) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_count < 0);
	settings[p_index].joints.resize(p_count);
}

int BoneConstraint3D::get_joint_count(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return (int)settings[p_index].joints.size();
}

void BoneConstraint3D::_make_all_twist_joints_dirty() {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_twist_joints_dirty((int)i);
	}
}

void BoneConstraint3D::_make_twist_joints_dirty(int p_index) {
	if (p_index >= 0 && p_index < (int)settings.size()) {
		settings[p_index].joints_dirty = true;
	}
}

void BoneConstraint3D::_update_twist_curve(int p_index) {
	Ref<Curve> curve = settings[p_index].damping_curve;
	if (curve.is_null()) {
		return;
	}
	LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	float unit = (int)joints.size() > 1 ? (1.0f / float((int)joints.size() - 1)) : 0.0f;
	for (uint32_t i = 0; i < joints.size(); i++) {
		joints[i].custom_amount = curve->sample(i * unit);
	}
}

void BoneConstraint3D::_update_twist_joints(int p_index) {
	ConstraintSetting &s = settings[p_index];
	s.joints_dirty = false;

	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}

	int root = s.root_bone.bone;
	int end = s.end_bone.bone;
	if (root < 0 || end < 0 || root >= sk->get_bone_count() || end >= sk->get_bone_count()) {
		s.joints.clear();
		return;
	}
	if (root != end && !sk->is_bone_parent_of(end, root)) {
		s.joints.clear();
		return;
	}

	Vector<int> chain_bones;
	int cur = end;
	while (cur != -1) {
		chain_bones.push_back(cur);
		if (cur == root) {
			break;
		}
		cur = sk->get_bone_parent(cur);
	}

	// If not extending end bone, the end bone itself is the reference bone and not twisted as an intermediate joint.
	int ref_bone = end;
	if (s.extend_end_bone) {
		Vector<int> children = sk->get_bone_children(end);
		if (children.size() == 1) {
			ref_bone = children[0];
		}
	} else if (chain_bones.size() > 1) {
		chain_bones.remove_at(0);
	}

	s.reference_bone = ref_bone;
	s.reference_bone_name = (ref_bone >= 0 && ref_bone < sk->get_bone_count()) ? sk->get_bone_name(ref_bone) : String();

	LocalVector<DisperseJointSetting> old_joints = std::move(s.joints);
	s.joints.resize(chain_bones.size());

	for (int i = 0; i < chain_bones.size(); i++) {
		int b = chain_bones[chain_bones.size() - 1 - i];
		s.joints[i].joint.bone = b;
		s.joints[i].joint.name = sk->get_bone_name(b);
		if ((uint32_t)i < old_joints.size()) {
			s.joints[i].custom_amount = old_joints[i].custom_amount;
		} else {
			s.joints[i].custom_amount = float(i + 1) / float(chain_bones.size());
		}
	}

	const uint32_t j_count = s.joints.size();
	if (j_count == 0) {
		return;
	}

	if (s.disperse_mode == DISPERSE_MODE_EVEN) {
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = double(i + 1) / double(j_count + (s.extend_end_bone ? 0 : 1));
		}
	} else if (s.disperse_mode == DISPERSE_MODE_WEIGHTED) {
		double total_len = 0.0;
		LocalVector<double> accum;
		accum.resize(j_count);
		for (uint32_t i = 0; i < j_count; i++) {
			double seg = (i + 1 < j_count) ? (double)sk->get_bone_rest(s.joints[i + 1].joint.bone).origin.length() : (double)sk->get_bone_rest(ref_bone).origin.length();
			accum[i] = total_len + seg * s.weight_position;
			total_len += seg;
		}
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = total_len > CMP_EPSILON ? CLAMP(accum[i] / total_len, 0.0, 1.0) : double(i + 1) / double(j_count);
		}
	} else {
		_update_twist_curve(p_index);
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = s.joints[i].custom_amount;
		}
	}
}

// --- Processing ---

void BoneConstraint3D::_process_modification(double p_delta) {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}

	if (constraint_type == CONSTRAINT_BONE_TARGET) {
		_process_bone_target(sk);
		return;
	}
	if (constraint_type == CONSTRAINT_LIMIT_ANGULAR_VELOCITY) {
		_process_limit_angular_velocity(sk, p_delta);
		return;
	}
	if (constraint_type == CONSTRAINT_TWIST_DISPERSER) {
		_process_twist_disperser(sk);
		return;
	}

	Transform3D skel_inv = sk->get_global_transform_interpolated().affine_inverse();

	for (uint32_t i = 0; i < settings.size(); i++) {
		const ConstraintSetting &s = settings[i];
		if (s.apply_bone < 0 || s.apply_bone >= sk->get_bone_count() || Math::is_zero_approx(s.amount)) {
			continue;
		}

		if (s.reference_type == REFERENCE_TYPE_BONE) {
			if (s.reference_bone < 0 || s.reference_bone >= sk->get_bone_count()) {
				continue;
			}
			if (constraint_type == CONSTRAINT_AIM) {
				if (s.apply_bone == s.reference_bone) {
					continue;
				}
				Vector3 ref_origin = sk->get_bone_global_pose(s.reference_bone).origin;
				_process_aim(i, sk, s.apply_bone, ref_origin, s.amount);
			} else {
				Transform3D dest = sk->get_bone_pose(s.reference_bone);
				if (s.is_relative_effective(constraint_type)) {
					Vector3 scl_rel = dest.basis.get_scale() / sk->get_bone_rest(s.reference_bone).basis.get_scale();
					dest.basis = sk->get_bone_rest(s.reference_bone).basis.get_rotation_quaternion().inverse() * dest.basis.get_rotation_quaternion();
					dest.basis.scale_local(scl_rel);
					dest.origin = dest.origin - sk->get_bone_rest(s.reference_bone).origin;
				}
				if (constraint_type == CONSTRAINT_CONVERT_TRANSFORM) {
					_process_convert(i, sk, s.apply_bone, dest, s.amount);
				} else {
					_process_copy(i, sk, s.apply_bone, dest, s.amount);
				}
			}
		} else {
			Node3D *ref_node = dynamic_cast<Node3D *>(get_node_or_null(s.reference_node));
			if (!ref_node) {
				continue;
			}
			Transform3D ref_local = skel_inv * ref_node->get_global_transform_interpolated();
			if (constraint_type == CONSTRAINT_AIM) {
				_process_aim(i, sk, s.apply_bone, ref_local.origin, s.amount);
			} else if (constraint_type == CONSTRAINT_CONVERT_TRANSFORM) {
				_process_convert(i, sk, s.apply_bone, ref_local, s.amount);
			} else {
				_process_copy(i, sk, s.apply_bone, ref_local, s.amount);
			}
		}
	}
}

void BoneConstraint3D::_process_bone_target(Skeleton3D *p_skeleton) {
	if (target_bone < 0 || target_bone >= p_skeleton->get_bone_count()) {
		return;
	}
	set_global_transform(p_skeleton->get_global_transform_interpolated() * p_skeleton->get_bone_global_pose(target_bone));
}

void BoneConstraint3D::_process_limit_angular_velocity(Skeleton3D *p_skeleton, double p_delta) {
	if (velocity_joints_dirty) {
		_update_velocity_joints();
	}
	if (velocity_bones.is_empty()) {
		return;
	}
	if (velocity_init_needed || p_delta <= 0.0) {
		for (uint32_t i = 0; i < velocity_bones.size(); i++) {
			int b = velocity_bones[i].first;
			if (b >= 0 && b < p_skeleton->get_bone_count()) {
				velocity_bones[i].second = p_skeleton->get_bone_pose_rotation(b);
			}
		}
		velocity_init_needed = false;
		return;
	}

	double limit_angle = max_angular_velocity * p_delta;
	for (uint32_t i = 0; i < velocity_bones.size(); i++) {
		int b = velocity_bones[i].first;
		if (b < 0 || b >= p_skeleton->get_bone_count()) {
			continue;
		}
		Quaternion prev_rot = velocity_bones[i].second;
		Quaternion curr_rot = p_skeleton->get_bone_pose_rotation(b);
		double angle_diff = prev_rot.angle_to(curr_rot);
		if (angle_diff > limit_angle && angle_diff > CMP_EPSILON) {
			curr_rot = prev_rot.slerp(curr_rot, real_t(limit_angle / angle_diff));
			p_skeleton->set_bone_pose_rotation(b, curr_rot);
		}
		velocity_bones[i].second = curr_rot;
	}
}

void BoneConstraint3D::_process_twist_disperser(Skeleton3D *p_skeleton) {
	for (uint32_t i = 0; i < settings.size(); i++) {
		if (settings[i].joints_dirty) {
			_update_twist_joints((int)i);
		}
		ConstraintSetting &s = settings[i];
		if (s.joints.is_empty() || s.reference_bone < 0 || s.reference_bone >= p_skeleton->get_bone_count()) {
			continue;
		}

		Vector3 ref_axis = IKModifier3D::get_bone_axis(p_skeleton, s.end_bone.bone, s.end_bone_direction, mutable_bone_axes);
		if (ref_axis.is_zero_approx()) {
			continue;
		}
		ref_axis.normalize();

		Quaternion base_rot = s.twist_from_rest ? p_skeleton->get_bone_rest(s.reference_bone).basis.get_rotation_quaternion() : s.twist_from;
		Quaternion ref_delta = base_rot.inverse() * p_skeleton->get_bone_pose_rotation(s.reference_bone);
		double total_twist = get_roll_angle(ref_delta, ref_axis);

		for (uint32_t j = 0; j < s.joints.size(); j++) {
			int b = s.joints[j].joint.bone;
			if (b < 0 || b >= p_skeleton->get_bone_count()) {
				continue;
			}
			int next_b = (j + 1 < s.joints.size()) ? s.joints[j + 1].joint.bone : s.reference_bone;
			Vector3 joint_axis = (next_b != b) ? (mutable_bone_axes ? p_skeleton->get_bone_pose_position(next_b) : p_skeleton->get_bone_rest(next_b).origin) : ref_axis;
			if (joint_axis.is_zero_approx()) {
				joint_axis = ref_axis;
			} else {
				joint_axis.normalize();
			}
			s.joints[j].axis = joint_axis;

			double prev_amount = j > 0 ? s.joints[j - 1].amount : 0.0;
			double delta_twist = total_twist * ((s.disperse_mode == DISPERSE_MODE_CUSTOM ? s.joints[j].custom_amount : s.joints[j].amount) - prev_amount);
			Quaternion swing = get_swing(p_skeleton->get_bone_pose_rotation(b), joint_axis);
			Quaternion twist_rot(joint_axis, real_t(delta_twist));
			p_skeleton->set_bone_pose_rotation(b, swing * twist_rot);
		}

		if (!s.extend_end_bone && s.reference_bone == s.end_bone.bone) {
			double last_amount = s.disperse_mode == DISPERSE_MODE_CUSTOM ? s.joints[s.joints.size() - 1].custom_amount : s.joints[s.joints.size() - 1].amount;
			double remaining_twist = total_twist * (1.0 - last_amount);
			Quaternion end_swing = get_swing(p_skeleton->get_bone_pose_rotation(s.end_bone.bone), ref_axis);
			p_skeleton->set_bone_pose_rotation(s.end_bone.bone, end_swing * Quaternion(ref_axis, real_t(remaining_twist)));
		}
	}
}

void BoneConstraint3D::_process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];

	Transform3D src_bone_rest = setting.relative ? p_skeleton->get_bone_pose(p_apply_bone) : p_skeleton->get_bone_rest(p_apply_bone);
	Transform3D bone_rest_space;
	int parent_bone = p_skeleton->get_bone_parent(p_apply_bone);
	if (parent_bone < 0) {
		bone_rest_space.translate_local(src_bone_rest.origin);
	} else {
		bone_rest_space = p_skeleton->get_bone_global_pose(parent_bone);
		bone_rest_space.translate_local(src_bone_rest.origin);
	}
	Vector3 forward_vector = bone_rest_space.basis.get_rotation_quaternion().xform_inv(p_target - bone_rest_space.origin);
	if (forward_vector.is_zero_approx()) {
		return;
	}
	forward_vector.normalize();

	Quaternion destination;
	if (setting.use_euler) {
		Vector3 current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(src_bone_rest.basis, setting.forward_axis).normalized();
		Vector2 src_vec2 = LookAtModifier3D::get_projection_vector(src_bone_rest.basis.xform_inv(forward_vector), setting.primary_rotation_axis).normalized();
		Vector2 dst_vec2 = LookAtModifier3D::get_projection_vector(src_bone_rest.basis.xform_inv(current_vector), setting.primary_rotation_axis).normalized();
		real_t calculated_angle = src_vec2.angle_to(dst_vec2);
		Transform3D primary_result = src_bone_rest.rotated_local(get_vector_from_axis(setting.primary_rotation_axis), calculated_angle);
		Transform3D current_result = primary_result;
		if (setting.use_secondary_rotation) {
			Vector3::Axis secondary_rotation_axis = LookAtModifier3D::get_secondary_rotation_axis(setting.forward_axis, setting.primary_rotation_axis);
			current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(primary_result.basis, setting.forward_axis).normalized();
			src_vec2 = LookAtModifier3D::get_projection_vector(primary_result.basis.xform_inv(forward_vector), secondary_rotation_axis).normalized();
			dst_vec2 = LookAtModifier3D::get_projection_vector(primary_result.basis.xform_inv(current_vector), secondary_rotation_axis).normalized();
			calculated_angle = src_vec2.angle_to(dst_vec2);
			current_result = primary_result.rotated_local(get_vector_from_axis(secondary_rotation_axis), calculated_angle);
		}
		destination = current_result.basis.get_rotation_quaternion();
	} else {
		Vector3 current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(src_bone_rest.basis, setting.forward_axis).normalized();
		destination = Quaternion(current_vector, forward_vector) * src_bone_rest.basis.get_rotation_quaternion();
	}

	p_skeleton->set_bone_pose_rotation(p_apply_bone, p_skeleton->get_bone_pose_rotation(p_apply_bone).slerp(destination, p_amount));
}

void BoneConstraint3D::_process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];
	bool rel = setting.is_relative_effective(constraint_type);

	Transform3D destination = p_destination;
	double point = 0.0;
	int axis = (int)setting.reference_axis;
	switch (setting.reference_transform_mode) {
		case TRANSFORM_MODE_POSITION: {
			point = destination.origin[axis];
		} break;
		case TRANSFORM_MODE_ROTATION: {
			Quaternion tgt_rot = destination.basis.get_rotation_quaternion();
			point = get_roll_angle(tgt_rot, get_vector_from_axis(setting.reference_axis));
		} break;
		case TRANSFORM_MODE_SCALE: {
			point = destination.basis.get_scale()[axis];
		} break;
	}

	destination = p_skeleton->get_bone_pose(p_apply_bone);
	if (Math::is_equal_approx(setting.reference_range_min, setting.reference_range_max)) {
		point = point <= (double)setting.reference_range_min ? 0 : 1;
	} else {
		point = Math::inverse_lerp((double)setting.reference_range_min, (double)setting.reference_range_max, point);
	}
	point = Math::lerp((double)setting.apply_range_min, (double)setting.apply_range_max, CLAMP(point, 0, 1));
	axis = (int)setting.apply_axis;

	switch (setting.apply_transform_mode) {
		case TRANSFORM_MODE_POSITION: {
			if (setting.additive) {
				point = p_skeleton->get_bone_pose(p_apply_bone).origin[axis] + point;
			} else if (rel) {
				point = p_skeleton->get_bone_rest(p_apply_bone).origin[axis] + point;
			}
			destination.origin[axis] = point;
		} break;
		case TRANSFORM_MODE_ROTATION: {
			Vector3 rot_axis = get_vector_from_axis(setting.apply_axis);
			Vector3 dest_scl = destination.basis.get_scale();
			if (influence < 1.0 || p_amount < 1.0) {
				point = CLAMP(point, CMP_EPSILON - Math::PI, Math::PI - CMP_EPSILON);
			}
			Quaternion rot(rot_axis, point);
			if (setting.additive) {
				destination.basis = p_skeleton->get_bone_pose(p_apply_bone).basis.get_rotation_quaternion() * rot;
			} else if (rel) {
				destination.basis = p_skeleton->get_bone_rest(p_apply_bone).basis.get_rotation_quaternion() * rot;
			} else {
				destination.basis = rot;
			}
			destination.basis.scale_local(dest_scl);
		} break;
		case TRANSFORM_MODE_SCALE: {
			Vector3 dest_scl = p_skeleton->get_bone_pose(p_apply_bone).basis.get_scale();
			if (setting.additive) {
				dest_scl[axis] = dest_scl[axis] * point;
			} else if (rel) {
				dest_scl = p_skeleton->get_bone_rest(p_apply_bone).basis.get_scale();
				dest_scl[axis] = dest_scl[axis] * point;
			} else {
				dest_scl[axis] = point;
			}
			destination.basis = destination.basis.orthonormalized().scaled_local(dest_scl);
		} break;
	}

	destination = p_skeleton->get_bone_pose(p_apply_bone).interpolate_with(destination, p_amount);
	switch (setting.apply_transform_mode) {
		case TRANSFORM_MODE_POSITION:
			p_skeleton->set_bone_pose_position(p_apply_bone, destination.origin);
			break;
		case TRANSFORM_MODE_ROTATION:
			p_skeleton->set_bone_pose_rotation(p_apply_bone, destination.basis.get_rotation_quaternion());
			break;
		case TRANSFORM_MODE_SCALE:
			p_skeleton->set_bone_pose_scale(p_apply_bone, destination.basis.get_scale());
			break;
	}
}

void BoneConstraint3D::_process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];
	bool rel = setting.is_relative_effective(constraint_type);
	Transform3D cur_pose = p_skeleton->get_bone_pose(p_apply_bone);
	Transform3D rest_pose = p_skeleton->get_bone_rest(p_apply_bone);
	Transform3D target = cur_pose;

	if (setting.copy_flags & TRANSFORM_FLAG_POSITION) {
		Vector3 pos = p_destination.origin;
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				pos[a] = 0.0f;
				continue;
			}
			if (setting.invert_flags & (1 << a)) {
				pos[a] = -pos[a];
			}
			if (setting.additive) {
				target.origin[a] = cur_pose.origin[a] + pos[a];
			} else if (rel) {
				target.origin[a] = rest_pose.origin[a] + pos[a];
			} else {
				target.origin[a] = pos[a];
			}
		}
		p_skeleton->set_bone_pose_position(p_apply_bone, cur_pose.origin.lerp(target.origin, p_amount));
	}

	if (setting.copy_flags & TRANSFORM_FLAG_ROTATION) {
		Vector3 euler = p_destination.basis.get_euler();
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				euler[a] = 0.0f;
			} else if (setting.invert_flags & (1 << a)) {
				euler[a] = -euler[a];
			}
		}
		Quaternion rot = Quaternion::from_euler(euler);
		Quaternion base_rot = cur_pose.basis.get_rotation_quaternion();
		Quaternion dst_rot = rot;
		if (setting.additive) {
			dst_rot = base_rot * rot;
		} else if (rel) {
			dst_rot = rest_pose.basis.get_rotation_quaternion() * rot;
		}
		p_skeleton->set_bone_pose_rotation(p_apply_bone, base_rot.slerp(dst_rot, p_amount));
	}

	if (setting.copy_flags & TRANSFORM_FLAG_SCALE) {
		Vector3 scl = p_destination.basis.get_scale();
		Vector3 cur_scl = cur_pose.basis.get_scale();
		Vector3 rest_scl = rest_pose.basis.get_scale();
		Vector3 dst_scl = cur_scl;
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				continue;
			}
			real_t v = scl[a];
			if ((setting.invert_flags & (1 << a)) && !Math::is_zero_approx(v)) {
				v = 1.0f / v;
			}
			if (setting.additive) {
				dst_scl[a] = cur_scl[a] * v;
			} else if (rel) {
				dst_scl[a] = rest_scl[a] * v;
			} else {
				dst_scl[a] = v;
			}
		}
		p_skeleton->set_bone_pose_scale(p_apply_bone, cur_scl.lerp(dst_scl, p_amount));
	}
}
