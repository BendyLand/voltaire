/**************************************************************************/
/*  skeleton_modification_2d_twoboneik.cpp                                */
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
#include "skeleton_modification_2d_twoboneik.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

void SkeletonModification2DTwoBoneIK::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack) {
		is_setup = true;
		update_target_cache();
		update_joint_one_bone2d_cache();
		update_joint_two_bone2d_cache();
	}
}

void SkeletonModification2DTwoBoneIK::set_target_node(const NodePath& p_target_node)
{
	target_node = p_target_node;
	update_target_cache();
}

NodePath SkeletonModification2DTwoBoneIK::get_target_node() const { return target_node; }

void SkeletonModification2DTwoBoneIK::set_target_minimum_distance(float p_distance)
{
	ERR_FAIL_COND_MSG(p_distance < 0, "Target minimum distance cannot be less than zero!");
	target_minimum_distance = p_distance;
}

float SkeletonModification2DTwoBoneIK::get_target_minimum_distance() const
{
	return target_minimum_distance;
}

void SkeletonModification2DTwoBoneIK::set_target_maximum_distance(float p_distance)
{
	ERR_FAIL_COND_MSG(p_distance < 0, "Target maximum distance cannot be less than zero!");
	target_maximum_distance = p_distance;
}

float SkeletonModification2DTwoBoneIK::get_target_maximum_distance() const
{
	return target_maximum_distance;
}

void SkeletonModification2DTwoBoneIK::set_flip_bend_direction(bool p_flip_direction)
{
	flip_bend_direction = p_flip_direction;

#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DTwoBoneIK::get_flip_bend_direction() const
{
	return flip_bend_direction;
}

NodePath SkeletonModification2DTwoBoneIK::get_joint_one_bone2d_node() const
{
	return joint_one_bone2d_node;
}

NodePath SkeletonModification2DTwoBoneIK::get_joint_two_bone2d_node() const
{
	return joint_two_bone2d_node;
}

int SkeletonModification2DTwoBoneIK::get_joint_one_bone_idx() const { return joint_one_bone_idx; }

int SkeletonModification2DTwoBoneIK::get_joint_two_bone_idx() const { return joint_two_bone_idx; }

#ifdef TOOLS_ENABLED
void SkeletonModification2DTwoBoneIK::set_editor_draw_min_max(bool p_draw)
{
	editor_draw_min_max = p_draw;
}

bool SkeletonModification2DTwoBoneIK::get_editor_draw_min_max() const
{
	return editor_draw_min_max;
}
#endif // TOOLS_ENABLED

SkeletonModification2DTwoBoneIK::SkeletonModification2DTwoBoneIK()
{
	stack = nullptr;
	is_setup = false;
	enabled = true;
	editor_draw_gizmo = true;
}

SkeletonModification2DTwoBoneIK::~SkeletonModification2DTwoBoneIK() {}


