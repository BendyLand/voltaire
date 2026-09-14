/**************************************************************************/
/*  chain_ik_3d.cpp                                                       */
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

#include "chain_ik_3d.h"

String ChainIK3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return chain_settings[p_index]->root_bone.name;
}

int ChainIK3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return chain_settings[p_index]->root_bone.bone;
}

String ChainIK3D::get_end_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return chain_settings[p_index]->end_bone.name;
}

int ChainIK3D::get_end_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return chain_settings[p_index]->end_bone.bone;
}

bool ChainIK3D::is_end_bone_extended(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return chain_settings[p_index]->extend_end_bone;
}

SkeletonModifier3D::BoneDirection ChainIK3D::get_end_bone_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return chain_settings[p_index]->end_bone_direction;
}

void ChainIK3D::set_end_bone_length(int p_index, float p_length)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	float old = chain_settings[p_index]->end_bone_length;
	chain_settings[p_index]->end_bone_length = p_length;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes && Math::is_zero_approx(old) == Math::is_zero_approx(p_length)) {
		return; // If chain size is not changed, length will be recaluclated in _update_bone_axis().
	}
	_make_simulation_dirty(p_index);
}

float ChainIK3D::get_end_bone_length(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return chain_settings[p_index]->end_bone_length;
}

// Individual joints.

String ChainIK3D::get_joint_bone_name(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<BoneJoint>& joints = chain_settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].name;
}

int ChainIK3D::get_joint_bone(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<BoneJoint>& joints = chain_settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].bone;
}

int ChainIK3D::get_joint_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<BoneJoint>& joints = chain_settings[p_index]->joints;
	return joints.size();
}

void ChainIK3D::_validate_axes(Skeleton3D* p_skeleton) const
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		for (uint32_t j = 0; j < chain_settings[i]->joints.size(); j++) {
			_validate_axis(p_skeleton, i, j);
		}
	}
}

void ChainIK3D::_validate_axis(Skeleton3D* p_skeleton, int p_index, int p_joint) const
{
}

void ChainIK3D::_make_all_joints_dirty()
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		_update_joints(i);
	}
}

void ChainIK3D::_process_ik(Skeleton3D* p_skeleton, double p_delta)
{
}

#ifdef TOOLS_ENABLED
Transform3D ChainIK3D::get_bone_global_rest_mutable(Skeleton3D* p_skeleton, int p_bone)
{
	int current = p_bone;
	Transform3D accum;
	int parent = p_skeleton->get_bone_parent(current);
	if (parent >= 0) {
		accum = p_skeleton->get_bone_global_rest(parent);
	}
	Transform3D tr = p_skeleton->get_bone_rest(current);
	// Note:
	// Chain IK gizmo might not be able to retrieve this pose in SkeletonModifier update process.
	// So the gizmo uses bone_vector insteads but parent of root bone doesn't have bone_vector.
	// Then, we needs to cache this pose in IK node.
	tr.origin = p_skeleton->get_bone_pose_position(current);
	accum *= tr;
	return accum;
}

Transform3D ChainIK3D::get_chain_root_global_rest(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Transform3D());
	return chain_settings[p_index]->root_global_rest;
}

Vector3 ChainIK3D::get_bone_vector(int p_index, int p_joint) const { return Vector3(); }
#endif // TOOLS_ENABLED

ChainIK3D::~ChainIK3D() { clear_settings(); }


