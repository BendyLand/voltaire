/**************************************************************************/
/*  physical_bone_simulator_3d.cpp                                        */
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

#include "core/config/engine.h"
#include "physical_bone_simulator_3d.h"
#include "scene/3d/physics/physical_bone_3d.h"

void PhysicalBoneSimulator3D::_bone_list_changed()
{
	bones.clear();
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	for (int i = 0; i < skeleton->get_bone_count(); i++) {
		SimulatedBone sb;
		sb.parent = skeleton->get_bone_parent(i);
		sb.child_bones = skeleton->get_bone_children(i);
		bones.push_back(sb);
	}
	_rebuild_physical_bones_cache();
	_pose_updated();
}

void PhysicalBoneSimulator3D::_pose_updated()
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton || simulating) {
		return;
	}
	// If this triggers that means that we likely haven't rebuilt the bone list yet.
	if (skeleton->get_bone_count() != (int)bones.size()) {
		// NOTE: this is re-entrant and will call _pose_updated again.
		_bone_list_changed();
	}
	else {
		for (int i = 0; i < skeleton->get_bone_count(); i++) {
			_bone_pose_updated(skeleton, i);
		}
	}
}

void PhysicalBoneSimulator3D::_bone_pose_updated(Skeleton3D* p_skeleton, int p_bone_id)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_bone_id, bones.size());
	bones[p_bone_id].global_pose = p_skeleton->get_bone_global_pose(p_bone_id);
}

void PhysicalBoneSimulator3D::_set_active(bool p_active)
{
	if (!Engine::get_singleton()->is_editor_hint()) {
		_reset_physical_bones_state();
	}
}

void PhysicalBoneSimulator3D::_reset_physical_bones_state()
{
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		if (bones[i].physical_bone) {
			bones[i].physical_bone->reset_physics_simulation_state();
		}
	}
}

bool PhysicalBoneSimulator3D::is_simulating_physics() const { return simulating; }

int PhysicalBoneSimulator3D::find_bone(const String& p_name) const
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return -1;
	}
	return skeleton->find_bone(p_name);
}

String PhysicalBoneSimulator3D::get_bone_name(int p_bone) const
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return String();
	}
	return skeleton->get_bone_name(p_bone);
}

int PhysicalBoneSimulator3D::get_bone_count() const { return bones.size(); }

bool PhysicalBoneSimulator3D::is_bone_parent_of(int p_bone, int p_parent_bone_id) const
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return false;
	}
	return skeleton->is_bone_parent_of(p_bone, p_parent_bone_id);
}

void PhysicalBoneSimulator3D::bind_physical_bone_to_bone(
	int p_bone, PhysicalBone3D* p_physical_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	ERR_FAIL_COND(bones[p_bone].physical_bone);
	ERR_FAIL_NULL(p_physical_bone);
	bones[p_bone].physical_bone = p_physical_bone;

	_rebuild_physical_bones_cache();
}

void PhysicalBoneSimulator3D::unbind_physical_bone_from_bone(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].physical_bone = nullptr;

	_rebuild_physical_bones_cache();
}

PhysicalBone3D* PhysicalBoneSimulator3D::get_physical_bone(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	return bones[p_bone].physical_bone;
}

PhysicalBone3D* PhysicalBoneSimulator3D::get_physical_bone_parent(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	if (bones[p_bone].cache_parent_physical_bone) {
		return bones[p_bone].cache_parent_physical_bone;
	}

	return _get_physical_bone_parent(p_bone);
}

PhysicalBone3D* PhysicalBoneSimulator3D::_get_physical_bone_parent(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);

	const int parent_bone = bones[p_bone].parent;
	if (parent_bone < 0) {
		return nullptr;
	}

	PhysicalBone3D* pb = bones[parent_bone].physical_bone;
	if (pb) {
		return pb;
	}
	else {
		return get_physical_bone_parent(parent_bone);
	}
}

void PhysicalBoneSimulator3D::_rebuild_physical_bones_cache()
{
	const int b_size = bones.size();
	for (int i = 0; i < b_size; ++i) {
		PhysicalBone3D* parent_pb = _get_physical_bone_parent(i);
		if (parent_pb != bones[i].cache_parent_physical_bone) {
			bones[i].cache_parent_physical_bone = parent_pb;
			if (bones[i].physical_bone) {
				bones[i].physical_bone->_on_bone_parent_changed();
			}
		}
	}
}

Transform3D PhysicalBoneSimulator3D::get_bone_global_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose;
}

void PhysicalBoneSimulator3D::set_bone_global_pose(int p_bone, const Transform3D& p_pose)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose = p_pose;
}

void PhysicalBoneSimulator3D::_process_modification(double p_delta)
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	ERR_FAIL_COND(skeleton->get_bone_count() != (int)bones.size());
	for (int i = 0; i < skeleton->get_bone_count(); i++) {
		if (!bones[i].physical_bone) {
			continue;
		}
		if (bones[i].physical_bone->is_simulating_physics() == false) {
			_bone_pose_updated(skeleton, i);
			bones[i].physical_bone->reset_to_rest_position();
		}
		else if (simulating) {
			skeleton->set_bone_global_pose(i, bones[i].global_pose);
		}
	}
}


PhysicalBoneSimulator3D::PhysicalBoneSimulator3D() {}


