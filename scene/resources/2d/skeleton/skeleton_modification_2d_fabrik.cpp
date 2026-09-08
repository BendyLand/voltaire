/**************************************************************************/
/*  skeleton_modification_2d_fabrik.cpp                                   */
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

#include "scene/2d/skeleton_2d.h"
#include "skeleton_modification_2d_fabrik.h"

void SkeletonModification2DFABRIK::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack != nullptr) {
		is_setup = true;

		if (stack->skeleton) {
			for (int i = 0; i < fabrik_data_chain.size(); i++) {
				fabrik_joint_update_bone2d_cache(i);
			}
		}
		update_target_cache();
	}
}

void SkeletonModification2DFABRIK::set_target_node(const NodePath& p_target_node)
{
	target_node = p_target_node;
	update_target_cache();
}

NodePath SkeletonModification2DFABRIK::get_target_node() const { return target_node; }

int SkeletonModification2DFABRIK::get_fabrik_data_chain_length()
{
	return fabrik_data_chain.size();
}

NodePath SkeletonModification2DFABRIK::get_fabrik_joint_bone2d_node(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, fabrik_data_chain.size(), NodePath(), "FABRIK joint out of range!");
	return fabrik_data_chain[p_joint_idx].bone2d_node;
}

int SkeletonModification2DFABRIK::get_fabrik_joint_bone_index(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, fabrik_data_chain.size(), -1, "FABRIK joint out of range!");
	return fabrik_data_chain[p_joint_idx].bone_idx;
}

void SkeletonModification2DFABRIK::set_fabrik_joint_magnet_position(
	int p_joint_idx, Vector2 p_magnet_position)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, fabrik_data_chain.size(), "FABRIK joint out of range!");
	fabrik_data_chain.write[p_joint_idx].magnet_position = p_magnet_position;
}

Vector2 SkeletonModification2DFABRIK::get_fabrik_joint_magnet_position(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, fabrik_data_chain.size(), Vector2(), "FABRIK joint out of range!");
	return fabrik_data_chain[p_joint_idx].magnet_position;
}

void SkeletonModification2DFABRIK::set_fabrik_joint_use_target_rotation(
	int p_joint_idx, bool p_use_target_rotation)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, fabrik_data_chain.size(), "FABRIK joint out of range!");
	fabrik_data_chain.write[p_joint_idx].use_target_rotation = p_use_target_rotation;
}

bool SkeletonModification2DFABRIK::get_fabrik_joint_use_target_rotation(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, fabrik_data_chain.size(), false, "FABRIK joint out of range!");
	return fabrik_data_chain[p_joint_idx].use_target_rotation;
}

SkeletonModification2DFABRIK::SkeletonModification2DFABRIK()
{
	stack = nullptr;
	is_setup = false;
	enabled = true;
	editor_draw_gizmo = false;
}

SkeletonModification2DFABRIK::~SkeletonModification2DFABRIK() {}


