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

int PhysicalBoneSimulator3D::get_bone_count() const { return bones.size(); }

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

void PhysicalBoneSimulator3D::_skeleton_changed(
	Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}


