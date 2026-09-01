/**************************************************************************/
/*  retarget_modifier_3d.cpp                                              */
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

#include "retarget_modifier_3d.h"

PackedStringArray RetargetModifier3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (child_skeletons.is_empty()) {
		warnings.push_back(RTR("There is no child Skeleton3D!"));
	}
	return warnings;
}

void RetargetModifier3D::cache_rests_with_reset()
{
	_reset_child_skeleton_poses();
	cache_rests();
}

void RetargetModifier3D::cache_rests()
{
	source_bone_ids.clear();

	Skeleton3D* source_skeleton = get_skeleton();
	if (profile.is_null() || !source_skeleton) {
		return;
	}

	PackedStringArray bone_names = profile->get_bone_names();
	for (const String& E : bone_names) {
		source_bone_ids.push_back(source_skeleton->find_bone(E));
	}

	for (int i = 0; i < child_skeletons.size(); i++) {
		_update_child_skeleton_rests(i);
	}
}

Vector<RetargetModifier3D::RetargetBoneInfo> RetargetModifier3D::cache_bone_global_rests(
	Skeleton3D* p_skeleton)
{
	// Retarget global pose in model space:
	// tgt_global_pose.basis = src_global_pose.basis * src_rest.basis.inv *
	// src_parent_global_rest.basis.inv * tgt_parent_global_rest.basis * tgt_rest.basis
	// tgt_global_pose.origin = src_global_pose.origin
	Skeleton3D* source_skeleton = get_skeleton();
	Vector<RetargetBoneInfo> bone_rests;
	if (profile.is_null() || !source_skeleton) {
		return bone_rests;
	}
	PackedStringArray bone_names = profile->get_bone_names();
	for (const String& E : bone_names) {
		RetargetBoneInfo rbi;
		int source_bone_id = source_skeleton->find_bone(E);
		if (source_bone_id >= 0) {
			Transform3D parent_global_rest;
			int bone_parent = source_skeleton->get_bone_parent(source_bone_id);
			if (bone_parent >= 0) {
				parent_global_rest = source_skeleton->get_bone_global_rest(bone_parent);
			}
			rbi.post_basis = source_skeleton->get_bone_rest(source_bone_id).basis.inverse() *
							 parent_global_rest.basis.inverse();
		}
		int target_bone_id = p_skeleton->find_bone(E);
		rbi.bone_id = target_bone_id;
		if (target_bone_id >= 0) {
			Transform3D parent_global_rest;
			int bone_parent = p_skeleton->get_bone_parent(target_bone_id);
			if (bone_parent >= 0) {
				parent_global_rest = p_skeleton->get_bone_global_rest(bone_parent);
			}
			rbi.post_basis = rbi.post_basis * parent_global_rest.basis *
							 p_skeleton->get_bone_rest(target_bone_id).basis;
		}
		bone_rests.push_back(rbi);
	}
	return bone_rests;
}

Vector<RetargetModifier3D::RetargetBoneInfo> RetargetModifier3D::cache_bone_rests(
	Skeleton3D* p_skeleton)
{
	// Retarget pose in model space:
	// tgt_pose.basis = tgt_parent_global_rest.basis.inv * src_parent_global_rest.basis *
	// src_pose.basis * src_rest.basis.inv * src_parent_global_rest.basis.inv *
	// tgt_parent_global_rest.basis * tgt_rest.basis tgt_pose.origin =
	// tgt_parent_global_rest.basis.inv.xform(src_parent_global_rest.basis.xform(src_pose.origin -
	// src_rest.origin)) + tgt_rest.origin
	Skeleton3D* source_skeleton = get_skeleton();
	Vector<RetargetBoneInfo> bone_rests;
	if (profile.is_null() || !source_skeleton) {
		return bone_rests;
	}
	PackedStringArray bone_names = profile->get_bone_names();
	for (const String& E : bone_names) {
		RetargetBoneInfo rbi;
		int source_bone_id = source_skeleton->find_bone(E);
		if (source_bone_id >= 0) {
			Transform3D parent_global_rest;
			int bone_parent = source_skeleton->get_bone_parent(source_bone_id);
			if (bone_parent >= 0) {
				parent_global_rest = source_skeleton->get_bone_global_rest(bone_parent);
			}
			rbi.pre_basis = parent_global_rest.basis;
			rbi.post_basis = source_skeleton->get_bone_rest(source_bone_id).basis.inverse() *
							 parent_global_rest.basis.inverse();
		}

		int target_bone_id = p_skeleton->find_bone(E);
		rbi.bone_id = target_bone_id;
		if (target_bone_id >= 0) {
			Transform3D parent_global_rest;
			int bone_parent = p_skeleton->get_bone_parent(target_bone_id);
			if (bone_parent >= 0) {
				parent_global_rest = p_skeleton->get_bone_global_rest(bone_parent);
			}
			rbi.pre_basis = parent_global_rest.basis.inverse() * rbi.pre_basis;
			rbi.post_basis = rbi.post_basis * parent_global_rest.basis *
							 p_skeleton->get_bone_rest(target_bone_id).basis;
		}
		bone_rests.push_back(rbi);
	}
	return bone_rests;
}

void RetargetModifier3D::_reset_child_skeletons()
{
	_reset_child_skeleton_poses();
	child_skeletons.clear();
}

/// General functions

void RetargetModifier3D::_set_active(bool p_active)
{
	if (!p_active) {
		_reset_child_skeleton_poses();
	}
}

void RetargetModifier3D::_process_modification(double p_delta)
{
	if (use_global_pose) {
		_retarget_global_pose();
	}
	else {
		_retarget_pose();
	}
}

void RetargetModifier3D::set_profile(Ref<SkeletonProfile> p_profile)
{
	if (profile == p_profile) {
		return;
	}
	_profile_changed(profile, p_profile);
}

Ref<SkeletonProfile> RetargetModifier3D::get_profile() const { return profile; }

bool RetargetModifier3D::is_using_global_pose() const { return use_global_pose; }

void RetargetModifier3D::set_enable_flags(uint32_t p_enable_flag)
{
	if (enable_flags != p_enable_flag) {
		_reset_child_skeleton_poses();
	}
	enable_flags = p_enable_flag;
}

uint32_t RetargetModifier3D::get_enable_flags() const { return enable_flags; }

void RetargetModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_update_child_skeletons();
	} break;
	case NOTIFICATION_EXIT_TREE: {
		_reset_child_skeletons();
	} break;
	}
}

RetargetModifier3D::RetargetModifier3D() {}

RetargetModifier3D::~RetargetModifier3D() {}


