/**************************************************************************/
/*  skeleton_modification_2d_physicalbones.cpp                            */
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
#include "scene/2d/physics/physical_bone_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "skeleton_modification_2d_physicalbones.h"

void SkeletonModification2DPhysicalBones::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack) {
		is_setup = true;

		if (stack->skeleton) {
			for (int i = 0; i < physical_bone_chain.size(); i++) {
				_physical_bone_update_cache(i);
			}
		}
	}
}

int SkeletonModification2DPhysicalBones::get_physical_bone_chain_length()
{
	return physical_bone_chain.size();
}

void SkeletonModification2DPhysicalBones::set_physical_bone_node(
	int p_joint_idx, const NodePath& p_nodepath)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, physical_bone_chain.size(), "Joint index out of range!");
	physical_bone_chain.write[p_joint_idx].physical_bone_node = p_nodepath;
	_physical_bone_update_cache(p_joint_idx);
}

NodePath SkeletonModification2DPhysicalBones::get_physical_bone_node(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, physical_bone_chain.size(), NodePath(), "Joint index out of range!");
	return physical_bone_chain[p_joint_idx].physical_bone_node;
}

SkeletonModification2DPhysicalBones::SkeletonModification2DPhysicalBones()
{
	stack = nullptr;
	is_setup = false;
	physical_bone_chain = Vector<PhysicalBone_Data2D>();
	enabled = true;
	editor_draw_gizmo = false; // Nothing to really show in a gizmo right now.
}

SkeletonModification2DPhysicalBones::~SkeletonModification2DPhysicalBones() {}


