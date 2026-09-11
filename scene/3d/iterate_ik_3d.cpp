/**************************************************************************/
/*  iterate_ik_3d.cpp                                                     */
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

#include "iterate_ik_3d.h"

PackedStringArray IterateIK3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	for (uint32_t i = 0; i < iterate_settings.size(); i++) {
		if (iterate_settings[i]->target_node.is_empty()) {
			warnings.push_back(RTR(
				"Detecting settings with no target set! IterateIK3D must have a target to work."));
			break;
		}
	}
	return warnings;
}

void IterateIK3D::set_max_iterations(int p_max_iterations) { max_iterations = p_max_iterations; }

int IterateIK3D::get_max_iterations() const { return max_iterations; }

void IterateIK3D::set_min_distance(double p_min_distance) { min_distance = p_min_distance; }

double IterateIK3D::get_min_distance() const { return min_distance; }

void IterateIK3D::set_angular_delta_limit(double p_angular_delta_limit)
{
	angular_delta_limit = p_angular_delta_limit;
}

double IterateIK3D::get_angular_delta_limit() const { return angular_delta_limit; }

void IterateIK3D::set_deterministic(bool p_deterministic) { deterministic = p_deterministic; }

bool IterateIK3D::is_deterministic() const { return deterministic; }

NodePath IterateIK3D::get_target_node(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return iterate_settings[p_index]->target_node;
}

SkeletonModifier3D::RotationAxis IterateIK3D::get_joint_rotation_axis(
	int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), ROTATION_AXIS_ALL);
	return joint_settings[p_joint]->rotation_axis;
}

void IterateIK3D::set_joint_rotation_axis_vector(int p_index, int p_joint, const Vector3& p_vector)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX(p_joint, (int)joint_settings.size());
	joint_settings[p_joint]->rotation_axis_vector = p_vector;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		_validate_axis(sk, p_index, p_joint);
	}
	_make_simulation_dirty(
		p_index); // Snapping to planes is needed in the initialization, so need to restructure.
}

Vector3 IterateIK3D::get_joint_rotation_axis_vector(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), Vector3());
	return joint_settings[p_joint]->get_rotation_axis_vector();
}

Quaternion IterateIK3D::get_joint_limitation_space(
	int p_index, int p_joint, const Vector3& p_forward) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), Quaternion());
	return joint_settings[p_joint]->get_limitation_space(p_forward);
}

Ref<JointLimitation3D> IterateIK3D::get_joint_limitation(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<JointLimitation3D>());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), Ref<JointLimitation3D>());
	return joint_settings[p_joint]->limitation;
}

IKModifier3D::SecondaryDirection IterateIK3D::get_joint_limitation_right_axis(
	int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), SECONDARY_DIRECTION_NONE);
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), SECONDARY_DIRECTION_NONE);
	return joint_settings[p_joint]->limitation_right_axis;
}

void IterateIK3D::set_joint_limitation_right_axis_vector(
	int p_index, int p_joint, const Vector3& p_vector)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX(p_joint, (int)joint_settings.size());
	joint_settings[p_joint]->limitation_right_axis_vector = p_vector;
	_update_joint_limitation(p_index, p_joint);
}

Vector3 IterateIK3D::get_joint_limitation_right_axis_vector(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), Vector3());
	return joint_settings[p_joint]->get_limitation_right_axis_vector();
}

void IterateIK3D::set_joint_limitation_rotation_offset(
	int p_index, int p_joint, const Quaternion& p_offset)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX(p_joint, (int)joint_settings.size());
	joint_settings[p_joint]->limitation_rotation_offset = p_offset;
	_update_joint_limitation(p_index, p_joint);
}

Quaternion IterateIK3D::get_joint_limitation_rotation_offset(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)joint_settings.size(), Quaternion());
	return joint_settings[p_joint]->limitation_rotation_offset;
}

void IterateIK3D::_set_joint_count(int p_index, int p_count)
{
	_unbind_joint_limitations(p_index);
	LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	int delta = p_count - joint_settings.size();
	if (delta < 0) {
		for (int i = delta; i < 0; i++) {
			memdelete(joint_settings[joint_settings.size() + i]);
			joint_settings[joint_settings.size() + i] = nullptr;
		}
	}
	joint_settings.resize(p_count);
	delta++;
	if (delta > 1) {
		for (int i = 1; i < delta; i++) {
			joint_settings[p_count - i] = memnew(IterateIK3DJointSetting);
		}
	}
}

void IterateIK3D::_validate_axis(Skeleton3D* p_skeleton, int p_index, int p_joint) const
{
	RotationAxis axis = iterate_settings[p_index]->joint_settings[p_joint]->rotation_axis;
	if (axis == ROTATION_AXIS_ALL) {
		return;
	}
	Vector3 rot = get_joint_rotation_axis_vector(p_index, p_joint).normalized();
	Vector3 fwd;
	if (p_joint < (int)iterate_settings[p_index]->joints.size() - 1) {
		fwd = p_skeleton->get_bone_rest(iterate_settings[p_index]->joints[p_joint + 1].bone).origin;
	}
	else if (iterate_settings[p_index]->extend_end_bone) {
		fwd = IKModifier3D::get_bone_axis(p_skeleton, iterate_settings[p_index]->end_bone.bone,
			iterate_settings[p_index]->end_bone_direction, mutable_bone_axes);
		if (fwd.is_zero_approx()) {
			return;
		}
	}
	fwd.normalize();
	if (Math::is_equal_approx(Math::abs(rot.dot(fwd)), 1)) {
		WARN_PRINT_ED("Setting: " + itos(p_index) + " Joint: " + itos(p_joint) +
					  ": Rotation axis and forward vector are colinear. This is not advised as it "
					  "may cause unwanted rotation.");
	}
}

void IterateIK3D::_clear_joints(int p_index)
{
	IterateIK3DSetting* setting = iterate_settings[p_index];
	if (!setting) {
		return;
	}
	_unbind_joint_limitations(p_index);
	for (uint32_t i = 0; i < setting->solver_info_list.size(); i++) {
		if (setting->solver_info_list[i]) {
			memdelete(setting->solver_info_list[i]);
			setting->solver_info_list[i] = nullptr;
		}
	}
	setting->solver_info_list.clear();
	setting->solver_info_list.resize_initialized(setting->joints.size());
	_bind_joint_limitations(p_index);
}

void IterateIK3D::_init_joints(Skeleton3D* p_skeleton, int p_index)
{
	IterateIK3DSetting* setting = iterate_settings[p_index];
	if (!setting) {
		return;
	}
	cached_space = p_skeleton->get_global_transform_interpolated();
	if (setting->simulation_dirty) {
		_clear_joints(p_index);
		setting->init_joints(p_skeleton, mutable_bone_axes);
		setting->simulation_dirty = false;
	}
	else if (deterministic) {
		setting->init_joints(p_skeleton, mutable_bone_axes);
	}

	if (mutable_bone_axes) {
#ifdef TOOLS_ENABLED
		_update_mutable_info();
#endif // TOOLS_ENABLED
		_update_bone_axis(p_skeleton, p_index);
	}
	setting->simulated = false;
}

void IterateIK3D::_make_simulation_dirty(int p_index)
{
	IterateIK3DSetting* setting = iterate_settings[p_index];
	if (!setting) {
		return;
	}
	setting->simulation_dirty = true;
#ifdef TOOLS_ENABLED
	if (!mutable_bone_axes) {
		_make_gizmo_dirty();
	}
#endif // TOOLS_ENABLED
}

void IterateIK3D::_update_bone_axis(Skeleton3D* p_skeleton, int p_index)
{
#ifdef TOOLS_ENABLED
	bool changed = false;
#endif // TOOLS_ENABLED
	IterateIK3DSetting* setting = iterate_settings[p_index];
	const LocalVector<BoneJoint>& joints = setting->joints;
	const LocalVector<IKModifier3DSolverInfo*>& solver_info_list = setting->solver_info_list;
	int len = (int)solver_info_list.size() - 1;
	for (int j = 0; j < len; j++) {
		IterateIK3DJointSetting* joint_setting = setting->joint_settings[j];
		if (!joint_setting || !solver_info_list[j]) {
			continue;
		}
		Vector3 axis = p_skeleton->get_bone_pose(joints[j + 1].bone).origin;
		if (axis.is_zero_approx()) {
			continue;
		}
		// Less computing.
#ifdef TOOLS_ENABLED
		if (!changed) {
			Vector3 old_v = solver_info_list[j]->forward_vector;
			solver_info_list[j]->forward_vector =
				snap_vector_to_plane(joint_setting->get_rotation_axis_vector(), axis.normalized());
			changed = changed || !old_v.is_equal_approx(solver_info_list[j]->forward_vector);
			float old_l = solver_info_list[j]->length;
			solver_info_list[j]->length = axis.length();
			changed = changed || !Math::is_equal_approx(old_l, solver_info_list[j]->length);
		}
		else {
			solver_info_list[j]->forward_vector =
				snap_vector_to_plane(joint_setting->get_rotation_axis_vector(), axis.normalized());
			solver_info_list[j]->length = axis.length();
		}
#else
		solver_info_list[j]->forward_vector =
			snap_vector_to_plane(joint_setting->get_rotation_axis_vector(), axis.normalized());
		solver_info_list[j]->length = axis.length();
#endif // TOOLS_ENABLED
	}
	if (setting->extend_end_bone && len >= 0) {
		IterateIK3DJointSetting* joint_setting = setting->joint_settings[len];
		if (joint_setting && solver_info_list[len]) {
			Vector3 axis = IKModifier3D::get_bone_axis(
				p_skeleton, setting->end_bone.bone, setting->end_bone_direction, mutable_bone_axes);
			if (!axis.is_zero_approx()) {
				solver_info_list[len]->forward_vector = snap_vector_to_plane(
					joint_setting->get_rotation_axis_vector(), axis.normalized());
				solver_info_list[len]->length = setting->end_bone_length;
			}
		}
	}
#ifdef TOOLS_ENABLED
	if (changed) {
		_make_gizmo_dirty();
	}
#endif // TOOLS_ENABLED
}

void IterateIK3D::_process_joints(double p_delta, Skeleton3D* p_skeleton,
	IterateIK3DSetting* p_setting, const Vector3& p_destination)
{
	double distance_to_target_sq = INFINITY;
	int iteration_count = 0;

	// To prevent oscillation, if it has been processed at least once and target was reached, abort
	// iterating.
	if (p_setting->simulated) {
		distance_to_target_sq =
			p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
	}

	while (distance_to_target_sq > min_distance_squared && iteration_count < max_iterations) {
		// Solve the IK for this iteration.
		_solve_iteration(p_delta, p_skeleton, p_setting, p_destination);

		// Update virtual bone rest/poses.
		p_setting->cache_current_joint_rotations(p_skeleton, angular_delta_limit);
		distance_to_target_sq =
			p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
		iteration_count++;
	}

	// Apply the virtual bone rest/poses to the actual bones.
	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo* solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		p_skeleton->set_bone_pose_rotation(p_setting->joints[i].bone, solver_info->current_lpose);
	}

	p_setting->simulated = true;
}

void IterateIK3D::_solve_iteration(double p_delta, Skeleton3D* p_skeleton,
	IterateIK3DSetting* p_setting, const Vector3& p_destination)
{
	//
}

void IterateIK3D::_update_joint_limitation(int p_index, int p_joint)
{
	ERR_FAIL_INDEX(p_index, (int)iterate_settings.size());
	iterate_settings[p_index]->simulated = false;
	const LocalVector<IterateIK3DJointSetting*>& joint_settings =
		iterate_settings[p_index]->joint_settings;
	ERR_FAIL_INDEX(p_joint,
		(int)
			joint_settings.size()); // p_joint is unused directly, but need to identify bound index.
#ifdef TOOLS_ENABLED
	update_gizmos();
#endif // TOOLS_ENABLED
}

#ifdef TOOLS_ENABLED
Vector3 IterateIK3D::get_bone_vector(int p_index, int p_joint) const
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return Vector3();
	}
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	IterateIK3DSetting* setting = iterate_settings[p_index];
	if (!setting) {
		return Vector3();
	}
	const LocalVector<BoneJoint>& joints = setting->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3());
	const LocalVector<IKModifier3DSolverInfo*>& solver_info_list = setting->solver_info_list;
	if (p_joint >= (int)solver_info_list.size() || !solver_info_list[p_joint]) {
		if (p_joint == (int)joints.size() - 1) {
			return IKModifier3D::get_bone_axis(skeleton, setting->end_bone.bone,
					   setting->end_bone_direction, mutable_bone_axes) *
				   setting->end_bone_length;
		}
		return mutable_bone_axes ? skeleton->get_bone_pose(joints[p_joint + 1].bone).origin
								 : skeleton->get_bone_rest(joints[p_joint + 1].bone).origin;
	}
	return solver_info_list[p_joint]->forward_vector * solver_info_list[p_joint]->length;
}
#endif // TOOLS_ENABLED

IterateIK3D::~IterateIK3D()
{
	for (uint32_t i = 0; i < iterate_settings.size(); i++) {
		_unbind_joint_limitations(i);
	}
	clear_settings();
}


