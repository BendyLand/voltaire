/**************************************************************************/
/*  skeleton_modification_2d_ccdik.cpp                                    */
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
#include "scene/2d/skeleton_2d.h"
#include "skeleton_modification_2d_ccdik.h"

void SkeletonModification2DCCDIK::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack != nullptr) {
		is_setup = true;
		update_target_cache();
		update_tip_cache();
	}
}

void SkeletonModification2DCCDIK::_draw_editor_gizmo()
{
	if (!enabled || !is_setup) {
		return;
	}

	for (int i = 0; i < ccdik_data_chain.size(); i++) {
		if (!ccdik_data_chain[i].editor_draw_gizmo) {
			continue;
		}
		if (ccdik_data_chain[i].bone_idx < 0) {
			continue;
		}

		Bone2D* operation_bone = stack->skeleton->get_bone(ccdik_data_chain[i].bone_idx);
		editor_draw_angle_constraints(operation_bone, ccdik_data_chain[i].constraint_angle_min,
			ccdik_data_chain[i].constraint_angle_max, ccdik_data_chain[i].enable_constraint,
			ccdik_data_chain[i].constraint_in_localspace,
			ccdik_data_chain[i].constraint_angle_invert);
	}
}

void SkeletonModification2DCCDIK::set_target_node(const NodePath& p_target_node)
{
	target_node = p_target_node;
	update_target_cache();
}

NodePath SkeletonModification2DCCDIK::get_target_node() const { return target_node; }

void SkeletonModification2DCCDIK::set_tip_node(const NodePath& p_tip_node)
{
	tip_node = p_tip_node;
	update_tip_cache();
}

NodePath SkeletonModification2DCCDIK::get_tip_node() const { return tip_node; }

int SkeletonModification2DCCDIK::get_ccdik_data_chain_length() { return ccdik_data_chain.size(); }

NodePath SkeletonModification2DCCDIK::get_ccdik_joint_bone2d_node(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, ccdik_data_chain.size(), NodePath(), "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].bone2d_node;
}

int SkeletonModification2DCCDIK::get_ccdik_joint_bone_index(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), -1, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].bone_idx;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_rotate_from_joint(
	int p_joint_idx, bool p_rotate_from_joint)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].rotate_from_joint = p_rotate_from_joint;
}

bool SkeletonModification2DCCDIK::get_ccdik_joint_rotate_from_joint(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), false, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].rotate_from_joint;
}

bool SkeletonModification2DCCDIK::get_ccdik_joint_enable_constraint(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), false, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].enable_constraint;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_constraint_angle_min(
	int p_joint_idx, float p_angle_min)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].constraint_angle_min = p_angle_min;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2DCCDIK::get_ccdik_joint_constraint_angle_min(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), 0.0, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].constraint_angle_min;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_constraint_angle_max(
	int p_joint_idx, float p_angle_max)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].constraint_angle_max = p_angle_max;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2DCCDIK::get_ccdik_joint_constraint_angle_max(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), 0.0, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].constraint_angle_max;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_constraint_angle_invert(
	int p_joint_idx, bool p_invert)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].constraint_angle_invert = p_invert;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DCCDIK::get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), false, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].constraint_angle_invert;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_constraint_in_localspace(
	int p_joint_idx, bool p_constraint_in_localspace)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].constraint_in_localspace = p_constraint_in_localspace;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DCCDIK::get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), false, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].constraint_in_localspace;
}

void SkeletonModification2DCCDIK::set_ccdik_joint_editor_draw_gizmo(
	int p_joint_idx, bool p_draw_gizmo)
{
	ERR_FAIL_INDEX_MSG(p_joint_idx, ccdik_data_chain.size(), "CCDIK joint out of range!");
	ccdik_data_chain.write[p_joint_idx].editor_draw_gizmo = p_draw_gizmo;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DCCDIK::get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, ccdik_data_chain.size(), false, "CCDIK joint out of range!");
	return ccdik_data_chain[p_joint_idx].editor_draw_gizmo;
}

SkeletonModification2DCCDIK::SkeletonModification2DCCDIK()
{
	stack = nullptr;
	is_setup = false;
	enabled = true;
	editor_draw_gizmo = true;
}

SkeletonModification2DCCDIK::~SkeletonModification2DCCDIK() {}


