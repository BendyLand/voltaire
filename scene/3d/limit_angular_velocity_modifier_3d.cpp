/**************************************************************************/
/*  limit_angular_velocity_modifier_3d.cpp                                */
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

#include "limit_angular_velocity_modifier_3d.h"

void LimitAngularVelocityModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_make_joints_dirty();
	} break;
	}
}

String LimitAngularVelocityModifier3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)chains.size(), String());
	return chains[p_index].root_bone.name;
}

int LimitAngularVelocityModifier3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)chains.size(), -1);
	return chains[p_index].root_bone.bone;
}

String LimitAngularVelocityModifier3D::get_end_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)chains.size(), String());
	return chains[p_index].end_bone.name;
}

int LimitAngularVelocityModifier3D::get_end_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)chains.size(), -1);
	return chains[p_index].end_bone.bone;
}

void LimitAngularVelocityModifier3D::set_chain_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0);
	chains.resize(p_count);
	_make_joints_dirty();
}

int LimitAngularVelocityModifier3D::get_chain_count() const { return chains.size(); }

void LimitAngularVelocityModifier3D::clear_chains() { set_chain_count(0); }

String LimitAngularVelocityModifier3D::get_joint_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)joints.size(), String());
	return joints[p_index].name;
}

int LimitAngularVelocityModifier3D::get_joint_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)joints.size(), -1);
	return joints[p_index].bone;
}

int LimitAngularVelocityModifier3D::_get_joint_count() const { return joints.size(); }

void LimitAngularVelocityModifier3D::set_max_angular_velocity(double p_angular_velocity)
{
	max_angular_velocity = p_angular_velocity;
}

double LimitAngularVelocityModifier3D::get_max_angular_velocity() const
{
	return max_angular_velocity;
}

void LimitAngularVelocityModifier3D::set_exclude(bool p_exclude) { exclude = p_exclude; }

bool LimitAngularVelocityModifier3D::is_exclude() const { return exclude; }

void LimitAngularVelocityModifier3D::_set_active(bool p_active)
{
	if (p_active) {
		reset();
	}
}

void LimitAngularVelocityModifier3D::_skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new)
{
	_make_joints_dirty();
}

bool LimitAngularVelocityModifier3D::_is_joint_contained(int p_bone)
{
	bool ret = false;
	for (const BoneJoint& joint : joints) {
		if (joint.bone == p_bone) {
			ret = true;
			break;
		}
	}
	return ret;
}

void LimitAngularVelocityModifier3D::reset() { init_needed = true; }

LimitAngularVelocityModifier3D::~LimitAngularVelocityModifier3D() { clear_chains(); }



void LimitAngularVelocityModifier3D::_make_joints_dirty() {}
