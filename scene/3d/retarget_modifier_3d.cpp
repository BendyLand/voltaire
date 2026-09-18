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

void RetargetModifier3D::_reset_child_skeletons()
{
	_reset_child_skeleton_poses();
	child_skeletons.clear();
}

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

void RetargetModifier3D::_reset_child_skeleton_poses() {}

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


