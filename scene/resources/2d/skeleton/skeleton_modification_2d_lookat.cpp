/**************************************************************************/
/*  skeleton_modification_2d_lookat.cpp                                   */
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
#include "skeleton_modification_2d_lookat.h"

void SkeletonModification2DLookAt::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack != nullptr) {
		is_setup = true;
		update_target_cache();
		update_bone2d_cache();
	}
}

void SkeletonModification2DLookAt::_draw_editor_gizmo()
{
	if (!enabled || !is_setup || bone_idx < 0) {
		return;
	}

	Bone2D* operation_bone = stack->skeleton->get_bone(bone_idx);
	editor_draw_angle_constraints(operation_bone, constraint_angle_min, constraint_angle_max,
		enable_constraint, constraint_in_localspace, constraint_angle_invert);
}

void SkeletonModification2DLookAt::set_bone2d_node(const NodePath& p_target_node)
{
	bone2d_node = p_target_node;
	update_bone2d_cache();
}

NodePath SkeletonModification2DLookAt::get_bone2d_node() const { return bone2d_node; }

int SkeletonModification2DLookAt::get_bone_index() const { return bone_idx; }

void SkeletonModification2DLookAt::set_target_node(const NodePath& p_target_node)
{
	target_node = p_target_node;
	update_target_cache();
}

NodePath SkeletonModification2DLookAt::get_target_node() const { return target_node; }

float SkeletonModification2DLookAt::get_additional_rotation() const { return additional_rotation; }

void SkeletonModification2DLookAt::set_additional_rotation(float p_rotation)
{
	additional_rotation = p_rotation;
}

bool SkeletonModification2DLookAt::get_enable_constraint() const { return enable_constraint; }

void SkeletonModification2DLookAt::set_constraint_angle_min(float p_angle_min)
{
	constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2DLookAt::get_constraint_angle_min() const
{
	return constraint_angle_min;
}

void SkeletonModification2DLookAt::set_constraint_angle_max(float p_angle_max)
{
	constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2DLookAt::get_constraint_angle_max() const
{
	return constraint_angle_max;
}

void SkeletonModification2DLookAt::set_constraint_angle_invert(bool p_invert)
{
	constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DLookAt::get_constraint_angle_invert() const
{
	return constraint_angle_invert;
}

void SkeletonModification2DLookAt::set_constraint_in_localspace(bool p_constraint_in_localspace)
{
	constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2DLookAt::get_constraint_in_localspace() const
{
	return constraint_in_localspace;
}

SkeletonModification2DLookAt::SkeletonModification2DLookAt()
{
	stack = nullptr;
	is_setup = false;
	bone_idx = -1;
	additional_rotation = 0;
	enable_constraint = false;
	constraint_angle_min = 0;
	constraint_angle_max = Math::PI * 2;
	constraint_angle_invert = false;
	enabled = true;

	editor_draw_gizmo = true;
}

SkeletonModification2DLookAt::~SkeletonModification2DLookAt() {}


