/**************************************************************************/
/*  convert_transform_modifier_3d.cpp                                     */
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

#include "convert_transform_modifier_3d.h"

constexpr const char* HINT_POSITION = "-10,10,0.01,or_greater,or_less,suffix:m";
constexpr const char* HINT_ROTATION = "-180,180,0.01,radians_as_degrees";
constexpr const char* HINT_SCALE = "0,10,0.01,or_greater";

void ConvertTransformModifier3D::_validate_setting(int p_index)
{
	settings[p_index] = memnew(ConvertTransform3DSetting);
}

ConvertTransformModifier3D::TransformMode ConvertTransformModifier3D::get_apply_transform_mode(
	int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->apply_transform_mode;
}

void ConvertTransformModifier3D::set_apply_axis(int p_index, Vector3::Axis p_axis)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->apply_axis = p_axis;
}

Vector3::Axis ConvertTransformModifier3D::get_apply_axis(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->apply_axis;
}

void ConvertTransformModifier3D::set_apply_range_min(int p_index, float p_range_min)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->apply_range_min = p_range_min;
}

float ConvertTransformModifier3D::get_apply_range_min(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->apply_range_min;
}

void ConvertTransformModifier3D::set_apply_range_max(int p_index, float p_range_max)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->apply_range_max = p_range_max;
}

float ConvertTransformModifier3D::get_apply_range_max(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->apply_range_max;
}

ConvertTransformModifier3D::TransformMode ConvertTransformModifier3D::get_reference_transform_mode(
	int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->reference_transform_mode;
}

void ConvertTransformModifier3D::set_reference_axis(int p_index, Vector3::Axis p_axis)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->reference_axis = p_axis;
}

Vector3::Axis ConvertTransformModifier3D::get_reference_axis(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->reference_axis;
}

void ConvertTransformModifier3D::set_reference_range_min(int p_index, float p_range_min)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->reference_range_min = p_range_min;
}

float ConvertTransformModifier3D::get_reference_range_min(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->reference_range_min;
}

void ConvertTransformModifier3D::set_reference_range_max(int p_index, float p_range_max)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->reference_range_max = p_range_max;
}

float ConvertTransformModifier3D::get_reference_range_max(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->reference_range_max;
}

void ConvertTransformModifier3D::set_relative(int p_index, bool p_enabled)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->relative = p_enabled;
}

bool ConvertTransformModifier3D::is_relative(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->is_relative();
}

void ConvertTransformModifier3D::set_additive(int p_index, bool p_enabled)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	setting->additive = p_enabled;
}

bool ConvertTransformModifier3D::is_additive(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
	return setting->additive;
}

void ConvertTransformModifier3D::_process_constraint_by_bone(
	int p_index, Skeleton3D* p_skeleton, int p_apply_bone, int p_reference_bone, float p_amount)
{
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);
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
	_process_convert(p_index, p_skeleton, p_apply_bone, destination, p_amount);
}

void ConvertTransformModifier3D::_process_convert(int p_index, Skeleton3D* p_skeleton,
	int p_apply_bone, const Transform3D& p_destination, float p_amount)
{
	ConvertTransform3DSetting* setting = static_cast<ConvertTransform3DSetting*>(settings[p_index]);

	Transform3D destination = p_destination;

	// Retrieve point from reference.
	double point = 0.0;
	int axis = (int)setting->reference_axis;
	switch (setting->reference_transform_mode) {
	case TRANSFORM_MODE_POSITION: {
		point = destination.origin[axis];
	} break;
	case TRANSFORM_MODE_ROTATION: {
		Quaternion tgt_rot = destination.basis.get_rotation_quaternion();
		point = get_roll_angle(tgt_rot, get_vector_from_axis(setting->reference_axis));
	} break;
	case TRANSFORM_MODE_SCALE: {
		point = destination.basis.get_scale()[axis];
	} break;
	}
	// Convert point to apply.
	destination = p_skeleton->get_bone_pose(p_apply_bone);
	if (Math::is_equal_approx(setting->reference_range_min, setting->reference_range_max)) {
		point = point <= (double)setting->reference_range_min ? 0 : 1;
	}
	else {
		point = Math::inverse_lerp(
			(double)setting->reference_range_min, (double)setting->reference_range_max, point);
	}
	point = Math::lerp(
		(double)setting->apply_range_min, (double)setting->apply_range_max, CLAMP(point, 0, 1));
	axis = (int)setting->apply_axis;
	switch (setting->apply_transform_mode) {
	case TRANSFORM_MODE_POSITION: {
		if (setting->additive) {
			point = p_skeleton->get_bone_pose(p_apply_bone).origin[axis] + point;
		}
		else if (setting->is_relative()) {
			point = p_skeleton->get_bone_rest(p_apply_bone).origin[axis] + point;
		}
		destination.origin[axis] = point;
	} break;
	case TRANSFORM_MODE_ROTATION: {
		Vector3 rot_axis = get_vector_from_axis(setting->apply_axis);
		Vector3 dest_scl = destination.basis.get_scale();
		if (influence < 1.0 || p_amount < 1.0) {
			point = CLAMP(point, CMP_EPSILON - Math::PI,
				Math::PI - CMP_EPSILON); // Hack to consistent slerp (interpolate_with) orientation
										 // since -180/180 deg rot is mixed in slerp.
		}
		Quaternion rot = Quaternion(rot_axis, point);
		if (setting->additive) {
			destination.basis =
				p_skeleton->get_bone_pose(p_apply_bone).basis.get_rotation_quaternion() * rot;
		}
		else if (setting->is_relative()) {
			destination.basis =
				p_skeleton->get_bone_rest(p_apply_bone).basis.get_rotation_quaternion() * rot;
		}
		else {
			destination.basis = rot;
		}
		// Scale may not have meaning, but it might affect when it is negative.
		destination.basis.scale_local(dest_scl);
	} break;
	case TRANSFORM_MODE_SCALE: {
		Vector3 dest_scl = Vector3(1, 1, 1);
		if (setting->additive) {
			dest_scl = p_skeleton->get_bone_pose(p_apply_bone).basis.get_scale();
			dest_scl[axis] = dest_scl[axis] * point;
		}
		else if (setting->is_relative()) {
			dest_scl = p_skeleton->get_bone_rest(p_apply_bone).basis.get_scale();
			dest_scl[axis] = dest_scl[axis] * point;
		}
		else {
			dest_scl = p_skeleton->get_bone_pose(p_apply_bone).basis.get_scale();
			dest_scl[axis] = point;
		}
		destination.basis = destination.basis.orthonormalized().scaled_local(dest_scl);
	} break;
	}
	// Process interpolation depends on the amount.
	destination = p_skeleton->get_bone_pose(p_apply_bone).interpolate_with(destination, p_amount);
	// Apply transform depends on the mode.
	switch (setting->apply_transform_mode) {
	case TRANSFORM_MODE_POSITION: {
		p_skeleton->set_bone_pose_position(p_apply_bone, destination.origin);
	} break;
	case TRANSFORM_MODE_ROTATION: {
		p_skeleton->set_bone_pose_rotation(
			p_apply_bone, destination.basis.get_rotation_quaternion());
	} break;
	case TRANSFORM_MODE_SCALE: {
		p_skeleton->set_bone_pose_scale(p_apply_bone, destination.basis.get_scale());
	} break;
	}
}

ConvertTransformModifier3D::~ConvertTransformModifier3D() { clear_settings(); }


