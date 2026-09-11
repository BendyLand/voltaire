/**************************************************************************/
/*  copy_transform_modifier_3d.cpp                                        */
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

#include "copy_transform_modifier_3d.h"

void CopyTransformModifier3D::_validate_setting(int p_index)
{
	settings[p_index] = memnew(CopyTransform3DSetting);
}

uint32_t CopyTransformModifier3D::get_copy_flags(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	return setting->copy_flags;
}

uint32_t CopyTransformModifier3D::get_axis_flags(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	return setting->axis_flags;
}

uint32_t CopyTransformModifier3D::get_invert_flags(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	return setting->invert_flags;
}

void CopyTransformModifier3D::set_relative(int p_index, bool p_enabled)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	setting->relative = p_enabled;
}

bool CopyTransformModifier3D::is_relative(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	return setting->is_relative();
}

void CopyTransformModifier3D::set_additive(int p_index, bool p_enabled)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	setting->additive = p_enabled;
}

bool CopyTransformModifier3D::is_additive(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	return setting->additive;
}

void CopyTransformModifier3D::_process_constraint_by_bone(
	int p_index, Skeleton3D* p_skeleton, int p_apply_bone, int p_reference_bone, float p_amount)
{
	CopyTransform3DSetting* setting = static_cast<CopyTransform3DSetting*>(settings[p_index]);
	Transform3D destination = p_skeleton->get_bone_pose(p_reference_bone);
	if (setting->is_relative()) {
		Vector3 scl_relative = destination.basis.get_scale() /
							   p_skeleton->get_bone_rest(p_reference_bone).basis.get_scale();
		destination.basis =
			p_skeleton->get_bone_rest(p_reference_bone).basis.get_rotation_quaternion().inverse() *
			destination.basis.get_rotation_quaternion();
		destination.basis.scale_local(scl_relative);
		destination.origin =
			destination.origin - p_skeleton->get_bone_rest(p_reference_bone).origin;
	}
	_process_copy(p_index, p_skeleton, p_apply_bone, destination, p_amount);
}

CopyTransformModifier3D::~CopyTransformModifier3D() { clear_settings(); }


