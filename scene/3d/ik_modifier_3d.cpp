/**************************************************************************/
/*  ik_modifier_3d.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "ik_modifier_3d.h"

#include "core/config/engine.h"
#include "scene/3d/path_3d.h"

// --- JointSetting ---

Vector3 IKModifier3D::JointSetting::get_rotation_axis_vector() const {
	switch (rotation_axis) {
		case ROTATION_AXIS_X:
			return Vector3(1, 0, 0);
		case ROTATION_AXIS_Y:
			return Vector3(0, 1, 0);
		case ROTATION_AXIS_Z:
			return Vector3(0, 0, 1);
		case ROTATION_AXIS_ALL:
			return Vector3(0, 0, 0);
		case ROTATION_AXIS_CUSTOM:
			return rotation_axis_vector;
	}
	return Vector3(0, 0, 0);
}

Vector3 IKModifier3D::JointSetting::get_limitation_right_axis_vector() const {
	switch (limitation_right_axis) {
		case SECONDARY_DIRECTION_NONE:
			return Vector3(0, 0, 0);
		case SECONDARY_DIRECTION_PLUS_X:
			return Vector3(1, 0, 0);
		case SECONDARY_DIRECTION_MINUS_X:
			return Vector3(-1, 0, 0);
		case SECONDARY_DIRECTION_PLUS_Y:
			return Vector3(0, 1, 0);
		case SECONDARY_DIRECTION_MINUS_Y:
			return Vector3(0, -1, 0);
		case SECONDARY_DIRECTION_PLUS_Z:
			return Vector3(0, 0, 1);
		case SECONDARY_DIRECTION_MINUS_Z:
			return Vector3(0, 0, -1);
		case SECONDARY_DIRECTION_CUSTOM:
			return limitation_right_axis_vector;
	}
	return Vector3(0, 0, 0);
}

Quaternion IKModifier3D::JointSetting::get_limitation_space(const Vector3 &p_local_forward) const {
	if (limitation.is_null()) {
		return Quaternion();
	}
	return limitation->make_space(p_local_forward, get_limitation_right_axis_vector(), limitation_rotation_offset);
}

Vector3 IKModifier3D::JointSetting::get_projected_rotation(const Quaternion &p_offset, const Vector3 &p_vector) const {
	ERR_FAIL_COND_V(rotation_axis == ROTATION_AXIS_ALL, p_vector);
	const double ALMOST_ONE = 1.0 - CMP_EPSILON;
	Vector3 axis = get_rotation_axis_vector().normalized();
	Vector3 local_vector = p_offset.xform_inv(p_vector);
	double length = local_vector.length();
	Vector3 projected = snap_vector_to_plane(axis, local_vector.normalized());
	if (!Math::is_zero_approx(length)) {
		projected = projected.normalized() * length;
	}
	if (Math::abs(local_vector.normalized().dot(axis)) > ALMOST_ONE) {
		return p_vector;
	}
	return p_offset.xform(projected);
}

Vector3 IKModifier3D::JointSetting::get_limited_rotation(const Quaternion &p_offset, const Vector3 &p_vector, const Vector3 &p_forward) const {
	ERR_FAIL_COND_V(limitation.is_null(), p_vector);
	Vector3 local_vector = p_offset.xform_inv(p_vector);
	float length = local_vector.length();
	if (Math::is_zero_approx(length)) {
		return p_vector;
	}
	Vector3 limited = limitation->solve(p_forward, get_limitation_right_axis_vector(), limitation_rotation_offset, local_vector.normalized()) * length;
	return p_offset.xform(limited);
}

// --- ChainSetting ---

Vector3 IKModifier3D::ChainSetting::get_pole_direction_vector() const {
	switch (pole_direction) {
		case SECONDARY_DIRECTION_NONE:
			return Vector3(0, 0, 0);
		case SECONDARY_DIRECTION_PLUS_X:
			return Vector3(1, 0, 0);
		case SECONDARY_DIRECTION_MINUS_X:
			return Vector3(-1, 0, 0);
		case SECONDARY_DIRECTION_PLUS_Y:
			return Vector3(0, 1, 0);
		case SECONDARY_DIRECTION_MINUS_Y:
			return Vector3(0, -1, 0);
		case SECONDARY_DIRECTION_PLUS_Z:
			return Vector3(0, 0, 1);
		case SECONDARY_DIRECTION_MINUS_Z:
			return Vector3(0, 0, -1);
		case SECONDARY_DIRECTION_CUSTOM:
			return pole_direction_vector;
	}
	return Vector3(0, 0, 0);
}

void IKModifier3D::ChainSetting::clear_solver_info() {
	for (uint32_t i = 0; i < solver_info_list.size(); i++) {
		if (solver_info_list[i]) {
			memdelete(solver_info_list[i]);
			solver_info_list[i] = nullptr;
		}
	}
	solver_info_list.clear();
	if (root_joint_solver_info) {
		memdelete(root_joint_solver_info);
		root_joint_solver_info = nullptr;
	}
	if (mid_joint_solver_info) {
		memdelete(mid_joint_solver_info);
		mid_joint_solver_info = nullptr;
	}
}

void IKModifier3D::ChainSetting::update_chain_coordinate(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	chain[p_index] = p_position;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::update_chain_coordinate_bw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	Vector3 result = p_position;
	int HEAD = p_index - 1;
	int TAIL = p_index;
	if (HEAD >= 0 && HEAD < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (solver_info) {
			Vector3 old_head_to_tail = solver_info->current_vector;
			Vector3 new_head_to_tail = (result - chain[HEAD]).normalized();
			if (Math::is_equal_approx((double)old_head_to_tail.dot(new_head_to_tail), -1.0)) {
				chain[TAIL] = chain[HEAD] + old_head_to_tail * solver_info->length;
				return;
			}
		}
	}
	chain[p_index] = result;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::update_chain_coordinate_fw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	Vector3 result = p_position;
	int HEAD = p_index;
	int TAIL = p_index + 1;
	if (TAIL >= 0 && TAIL < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (solver_info) {
			Vector3 old_head_to_tail = solver_info->current_vector;
			Vector3 new_head_to_tail = (chain[TAIL] - result).normalized();
			if (Math::is_equal_approx((double)old_head_to_tail.dot(new_head_to_tail), -1.0)) {
				chain[HEAD] = chain[TAIL] - old_head_to_tail * solver_info->length;
				return;
			}
		}
	}
	chain[p_index] = result;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::cache_current_vector(Skeleton3D *p_skeleton, int p_index) {
	int cur_head = p_index - 1;
	int cur_tail = p_index;
	if (cur_head >= 0 && cur_head < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[cur_head];
		if (solver_info) {
			solver_info->current_vector = (chain[cur_tail] - chain[cur_head]).normalized();
		}
	}
	cur_head = p_index;
	cur_tail = p_index + 1;
	if (cur_tail < (int)chain.size() && cur_head < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[cur_head];
		if (solver_info) {
			solver_info->current_vector = (chain[cur_tail] - chain[cur_head]).normalized();
		}
	}
}

void IKModifier3D::ChainSetting::cache_current_vectors(Skeleton3D *p_skeleton) {
	for (uint32_t i = 0; i < joints.size(); i++) {
		int HEAD = i;
		int TAIL = i + 1;
		if (HEAD >= (int)solver_info_list.size() || TAIL >= (int)chain.size()) {
			break;
		}
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info) {
			continue;
		}
		solver_info->current_vector = (chain[TAIL] - chain[HEAD]).normalized();
	}
}

void IKModifier3D::ChainSetting::init_current_joint_rotations(Skeleton3D *p_skeleton) {
	if (root_bone.bone < 0) {
		return;
	}
	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}
	for (uint32_t i = 0; i < joints.size(); i++) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[i];
		if (!solver_info) {
			continue;
		}
		solver_info->current_lrest = p_skeleton->get_bone_pose(joints[i].bone).basis.get_rotation_quaternion();
		solver_info->current_grest = parent_gpose * solver_info->current_lrest;
		solver_info->current_grest.normalize();
		solver_info->current_lpose = solver_info->current_lrest;
		solver_info->current_gpose = solver_info->current_grest;
		parent_gpose = solver_info->current_gpose;
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_iterate_joint_rotations(Skeleton3D *p_skeleton, double p_angular_delta_limit) {
	Transform3D parent_gpose_tr;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose_tr = p_skeleton->get_bone_global_pose(parent);
	}
	Quaternion parent_gpose = parent_gpose_tr.basis.get_rotation_quaternion();

	for (uint32_t i = 0; i < joints.size(); i++) {
		int HEAD = i;
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info) {
			continue;
		}
		solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
		solver_info->current_grest = parent_gpose * solver_info->current_lrest;
		solver_info->current_grest.normalize();
		Vector3 from = solver_info->forward_vector;
		Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();
		Quaternion prev = solver_info->current_lpose;
		if (joint_settings[HEAD].rotation_axis == ROTATION_AXIS_ALL) {
			solver_info->current_lpose = solver_info->current_lrest * get_swing(Quaternion(from, to), from);
		} else {
			solver_info->current_lpose = solver_info->current_lrest * get_from_to_rotation_by_axis(from, to, joint_settings[HEAD].get_rotation_axis_vector().normalized());
		}
		double diff = prev.angle_to(solver_info->current_lpose);
		if (!Math::is_zero_approx(diff)) {
			solver_info->current_lpose = prev.slerp(solver_info->current_lpose, MIN(1.0, p_angular_delta_limit / diff));
		}
		solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
		solver_info->current_gpose.normalize();
		parent_gpose = solver_info->current_gpose;
	}

	if (chain.is_empty()) {
		return;
	}
	chain[0] = p_skeleton->get_bone_global_pose(root_bone.bone).origin;
	for (uint32_t i = 0; i < solver_info_list.size(); i++) {
		int HEAD = i;
		int TAIL = i + 1;
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info || TAIL >= (int)chain.size()) {
			continue;
		}
		chain[TAIL] = chain[HEAD] + solver_info->current_gpose.xform(solver_info->forward_vector) * solver_info->length;
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::init_iterate_joints(Skeleton3D *p_skeleton, bool p_mutable_bone_axes) {
	chain.clear();
	bool extends_end = extend_end_bone && end_bone_length > 0;
	for (uint32_t i = 0; i < joints.size(); i++) {
		chain.push_back(p_skeleton->get_bone_global_pose(joints[i].bone).origin);
		bool last = i == joints.size() - 1;
		if (last && extends_end) {
			Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, end_bone.bone, end_bone_direction, p_mutable_bone_axes);
			if (axis.is_zero_approx()) {
				continue;
			}
			if (!solver_info_list[i]) {
				solver_info_list[i] = memnew(IKModifier3DSolverInfo);
			}
			solver_info_list[i]->forward_vector = snap_vector_to_plane(joint_settings[i].get_rotation_axis_vector(), axis.normalized());
			solver_info_list[i]->length = end_bone_length;
			chain.push_back(p_skeleton->get_bone_global_pose(joints[i].bone).xform(axis * end_bone_length));
		} else if (!last) {
			Vector3 axis = p_skeleton->get_bone_rest(joints[i + 1].bone).origin;
			if (axis.is_zero_approx()) {
				continue;
			}
			if (!solver_info_list[i]) {
				solver_info_list[i] = memnew(IKModifier3DSolverInfo);
			}
			solver_info_list[i]->forward_vector = snap_vector_to_plane(joint_settings[i].get_rotation_axis_vector(), axis.normalized());
			solver_info_list[i]->length = axis.length();
		}
	}
	init_current_joint_rotations(p_skeleton);
}

int32_t IKModifier3D::ChainSetting::find_nearest_point(const Vector3 &p_origin, double p_length_sq, const PackedVector3Array &p_points, bool p_is_path_closed, uint32_t p_first, double *r_ret) const {
	ERR_FAIL_COND_V(p_first >= p_points.size(), p_points.size() - 1);
	uint32_t i = p_first;
	if (p_is_path_closed) {
		while (true) {
			if (p_origin.distance_squared_to(p_points[i]) >= p_length_sq) {
				break;
			}
			i++;
			i = Math::posmod(i, p_points.size());
			if (i == p_first) {
				i = p_points.size();
				break;
			}
		}
	} else {
		while (i < p_points.size()) {
			if (p_origin.distance_squared_to(p_points[i]) >= p_length_sq) {
				break;
			}
			i++;
		}
	}
	if (i == 0) {
		*r_ret = 0.0;
		return p_first;
	} else if (i >= p_points.size()) {
		*r_ret = 1.0;
		return i;
	}
	*r_ret = Math::inverse_lerp((double)p_origin.distance_squared_to(p_points[i - 1]), (double)p_origin.distance_squared_to(p_points[i]), p_length_sq);
	return i - 1;
}

void IKModifier3D::ChainSetting::cache_spline_joint_rotations(Skeleton3D *p_skeleton, bool p_use_tilt) {
	Transform3D parent_gpose_tr;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose_tr = p_skeleton->get_bone_global_pose(parent);
	}
	Quaternion parent_gpose = parent_gpose_tr.basis.get_rotation_quaternion();

	if (p_use_tilt) {
		double parent_twist = 0.0;
		for (uint32_t i = 0; i < joints.size(); i++) {
			int HEAD = i;
			IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
			if (!solver_info) {
				continue;
			}
			solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
			solver_info->current_grest = parent_gpose * solver_info->current_lrest;
			solver_info->current_grest.normalize();
			Vector3 from = solver_info->forward_vector;
			Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();

			Basis b = get_swing(Quaternion(from, to), from);
			b.rotate_local(from, twists[HEAD] - parent_twist);
			parent_twist = twists[HEAD];
			solver_info->current_lpose = solver_info->current_lrest * b.get_rotation_quaternion();

			solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
			solver_info->current_gpose.normalize();
			parent_gpose = solver_info->current_gpose;
		}
	} else {
		for (uint32_t i = 0; i < joints.size(); i++) {
			int HEAD = i;
			IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
			if (!solver_info) {
				continue;
			}
			solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
			solver_info->current_grest = parent_gpose * solver_info->current_lrest;
			solver_info->current_grest.normalize();
			Vector3 from = solver_info->forward_vector;
			Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();

			solver_info->current_lpose = solver_info->current_lrest * get_swing(Quaternion(from, to), from);

			solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
			solver_info->current_gpose.normalize();
			parent_gpose = solver_info->current_gpose;
		}
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_two_bone_vectors(Skeleton3D *p_skeleton) {
	if (!is_two_bone_valid()) {
		return;
	}
	root_joint_solver_info->current_vector = (mid_pos - root_pos).normalized();
	mid_joint_solver_info->current_vector = (end_pos - mid_pos).normalized();
}

void IKModifier3D::ChainSetting::init_two_bone_joint_rotations(Skeleton3D *p_skeleton) {
	if (!is_two_bone_valid()) {
		return;
	}

	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}
	root_joint_solver_info->current_lrest = p_skeleton->get_bone_pose(root_bone.bone).basis.get_rotation_quaternion();
	root_joint_solver_info->current_grest = parent_gpose * root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_grest.normalize();
	root_joint_solver_info->current_lpose = root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_gpose = root_joint_solver_info->current_grest;
	parent_gpose = root_joint_solver_info->current_gpose;

	mid_joint_solver_info->current_lrest = p_skeleton->get_bone_global_pose(root_bone.bone).basis.get_rotation_quaternion().inverse() *
			p_skeleton->get_bone_global_pose(middle_bone.bone).basis.get_rotation_quaternion();
	mid_joint_solver_info->current_grest = parent_gpose * mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_grest.normalize();
	mid_joint_solver_info->current_lpose = mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_gpose = mid_joint_solver_info->current_grest;

	cache_two_bone_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_two_bone_joint_rotations(Skeleton3D *p_skeleton, const Vector3 &p_pole_destination) {
	if (!is_two_bone_valid()) {
		return;
	}

	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}

	root_joint_solver_info->current_lrest = p_skeleton->get_bone_pose(root_bone.bone).basis.get_rotation_quaternion();
	root_joint_solver_info->current_grest = parent_gpose * root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_grest.normalize();

	Vector3 from = root_joint_solver_info->forward_vector;
	Vector3 to = root_joint_solver_info->current_grest.xform_inv(root_joint_solver_info->current_vector).normalized();
	root_joint_solver_info->current_lpose = root_joint_solver_info->current_lrest * get_swing(Quaternion(from, to), from);

	root_joint_solver_info->current_gpose = parent_gpose * root_joint_solver_info->current_lpose;
	root_joint_solver_info->current_gpose.normalize();
	Quaternion root_gpose = root_joint_solver_info->current_gpose;

	mid_joint_solver_info->current_lrest = p_skeleton->get_bone_global_pose(root_bone.bone).basis.get_rotation_quaternion().inverse() *
			p_skeleton->get_bone_global_pose(middle_bone.bone).basis.get_rotation_quaternion();
	mid_joint_solver_info->current_grest = root_gpose * mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_grest.normalize();

	from = mid_joint_solver_info->forward_vector;
	to = mid_joint_solver_info->current_grest.xform_inv(mid_joint_solver_info->current_vector).normalized();
	mid_joint_solver_info->current_lpose = mid_joint_solver_info->current_lrest * get_swing(Quaternion(from, to), from);

	mid_joint_solver_info->current_gpose = root_gpose * mid_joint_solver_info->current_lpose;
	mid_joint_solver_info->current_gpose.normalize();

	bool is_pole_defined = pole_direction != SECONDARY_DIRECTION_NONE &&
			(pole_direction != SECONDARY_DIRECTION_CUSTOM || !pole_direction_vector.is_zero_approx());
	if (is_pole_defined) {
		Vector3 pole_dir = get_projected_normal(root_pos, end_pos, p_pole_destination);
		if (pole_dir.is_zero_approx()) {
			return;
		}
		Vector3 a = mid_joint_solver_info->current_vector.normalized();
		Vector3 k = mid_joint_solver_info->current_gpose.xform(get_pole_direction_vector()).normalized();
		Vector3 n = pole_dir.cross((mid_pos - root_pos).normalized()).normalized();

		if (a.is_zero_approx() || k.is_zero_approx() || n.is_zero_approx() || Math::is_zero_approx(n.dot(k))) {
			return;
		}
		double c0 = n.dot(k - a * k.dot(a));
		double c1 = n.dot(a.cross(k));
		double c2 = n.dot(a) * k.dot(a);
		double r = Math::sqrt(c0 * c0 + c1 * c1);
		double cos_arg = CLAMP(-c2 / r, -1.0, 1.0);
		double phi = Math::atan2(c1, c0);
		double acosv = Math::acos(cos_arg);

		double t1 = phi + acosv;
		double t2 = phi - acosv;
		Quaternion q1(a, t1);
		Quaternion q2(a, t2);

		Vector3 pole_proj = snap_vector_to_plane(n, pole_dir).normalized();
		Vector3 k1p = snap_vector_to_plane(n, q1.xform(k)).normalized();
		Vector3 k2p = snap_vector_to_plane(n, q2.xform(k)).normalized();
		double s1 = pole_proj.is_zero_approx() ? Math::abs(t1) : k1p.dot(pole_proj);
		double s2 = pole_proj.is_zero_approx() ? Math::abs(t2) : k2p.dot(pole_proj);

		double t = s1 >= s2 ? t1 : t2;
		Quaternion root_roll_rot(root_joint_solver_info->forward_vector, t);
		Quaternion mid_roll_rot(mid_joint_solver_info->forward_vector, t);

		root_joint_solver_info->current_lpose = root_joint_solver_info->current_lpose * root_roll_rot;
		root_joint_solver_info->current_gpose = parent_gpose * root_joint_solver_info->current_lpose;
		root_joint_solver_info->current_gpose.normalize();
		root_gpose = root_joint_solver_info->current_gpose;

		mid_joint_solver_info->current_lpose = root_roll_rot.inverse() * mid_joint_solver_info->current_lpose * mid_roll_rot;
		mid_joint_solver_info->current_gpose = root_gpose * mid_joint_solver_info->current_lpose;
		mid_joint_solver_info->current_gpose.normalize();
	}
}

// --- IKModifier3D ---

IKModifier3D::IKModifier3D(SolverType p_solver_type) :
		solver_type(p_solver_type) {
}

IKModifier3D::~IKModifier3D() {
	clear_settings();
}

void IKModifier3D::set_solver_type(SolverType p_solver_type) {
	if (solver_type == p_solver_type) {
		return;
	}
	solver_type = p_solver_type;
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_simulation_dirty(i);
	}
	update_configuration_warnings();
}

#ifdef TOOLS_ENABLED
void IKModifier3D::_make_gizmo_dirty() {
	gizmo_dirty = true;
}

void IKModifier3D::_update_mutable_info() {}
#endif // TOOLS_ENABLED

void IKModifier3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint()) {
				set_notify_local_transform(true);
			}
			_update_mutable_info();
			_make_gizmo_dirty();
#endif // TOOLS_ENABLED
			_make_all_joints_dirty();
		} break;
#ifdef TOOLS_ENABLED
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			_make_gizmo_dirty();
		} break;
#endif // TOOLS_ENABLED
	}
}

PackedStringArray IKModifier3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (solver_type == SOLVER_SPLINE) {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i]->path_3d.is_empty()) {
				warnings.push_back(RTR("Detecting settings with no Path3D set! Spline IK must have a Path3D to work."));
				break;
			}
		}
	} else {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i]->target_node.is_empty()) {
				warnings.push_back(RTR("Detecting settings with no target set! IKModifier3D must have a target to work."));
				break;
			}
		}
		if (solver_type == SOLVER_TWO_BONE) {
			for (uint32_t i = 0; i < settings.size(); i++) {
				if (settings[i]->pole_node.is_empty()) {
					warnings.push_back(RTR("Detecting settings with no pole target set! Two-Bone IK must have a pole target to work."));
					break;
				}
			}
		}
	}
	return warnings;
}

int IKModifier3D::get_setting_count() const {
	return settings.size();
}

void IKModifier3D::set_setting_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	int delta = p_count - (int)settings.size();
	if (delta < 0) {
		for (int i = delta; i < 0; i++) {
			memdelete(settings[settings.size() + i]);
			settings[settings.size() + i] = nullptr;
		}
	}
	settings.resize(p_count);
	if (delta > 0) {
		for (int i = 1; i <= delta; i++) {
			settings[p_count - i] = memnew(ChainSetting);
		}
	}
}

void IKModifier3D::clear_settings() {
	set_setting_count(0);
}

void IKModifier3D::set_mutable_bone_axes(bool p_enabled) {
	mutable_bone_axes = p_enabled;
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_simulation_dirty(i);
	}
#ifdef TOOLS_ENABLED
	_update_mutable_info();
#endif // TOOLS_ENABLED
}

bool IKModifier3D::are_bone_axes_mutable() const {
	return mutable_bone_axes;
}

void IKModifier3D::set_max_iterations(int p_max_iterations) {
	max_iterations = p_max_iterations;
}

int IKModifier3D::get_max_iterations() const {
	return max_iterations;
}

void IKModifier3D::set_min_distance(double p_min_distance) {
	min_distance = p_min_distance;
	min_distance_squared = min_distance * min_distance;
}

double IKModifier3D::get_min_distance() const {
	return min_distance;
}

void IKModifier3D::set_angular_delta_limit(double p_angular_delta_limit) {
	angular_delta_limit = p_angular_delta_limit;
}

double IKModifier3D::get_angular_delta_limit() const {
	return angular_delta_limit;
}

void IKModifier3D::set_deterministic(bool p_deterministic) {
	deterministic = p_deterministic;
}

bool IKModifier3D::is_deterministic() const {
	return deterministic;
}

// Bone getters & setters.

void IKModifier3D::set_root_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_root_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_root_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone.name;
}

void IKModifier3D::set_root_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->root_bone.name = sk->get_bone_name(p_bone);
	}
	_update_joints(p_index);
}

int IKModifier3D::get_root_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone.bone;
}

void IKModifier3D::set_middle_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->middle_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_middle_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_middle_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->middle_bone.name;
}

void IKModifier3D::set_middle_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->middle_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->middle_bone.name = sk->get_bone_name(p_bone);
	}
	_make_simulation_dirty(p_index);
}

int IKModifier3D::get_middle_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->middle_bone.bone;
}

void IKModifier3D::set_end_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_end_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_end_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->end_bone.name;
}

void IKModifier3D::set_end_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->end_bone.name = sk->get_bone_name(p_bone);
	}
	_update_joints(p_index);
}

int IKModifier3D::get_end_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	if (solver_type == SOLVER_TWO_BONE) {
		return settings[p_index]->get_two_bone_end_bone();
	}
	return settings[p_index]->end_bone.bone;
}

void IKModifier3D::set_use_virtual_end(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->use_virtual_end = p_enabled;
	_make_simulation_dirty(p_index);
}

bool IKModifier3D::is_using_virtual_end(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->use_virtual_end;
}

void IKModifier3D::set_extend_end_bone(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->extend_end_bone = p_enabled;
	_make_simulation_dirty(p_index);
}

bool IKModifier3D::is_end_bone_extended(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->extend_end_bone;
}

void IKModifier3D::set_end_bone_direction(int p_index, BoneDirection p_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_direction = p_direction;
	_make_simulation_dirty(p_index);
}

SkeletonModifier3D::BoneDirection IKModifier3D::get_end_bone_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index]->end_bone_direction;
}

void IKModifier3D::set_end_bone_length(int p_index, float p_length) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	float old = settings[p_index]->end_bone_length;
	settings[p_index]->end_bone_length = p_length;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes && Math::is_zero_approx(old) == Math::is_zero_approx(p_length)) {
		return;
	}
	_make_simulation_dirty(p_index);
}

float IKModifier3D::get_end_bone_length(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index]->end_bone_length;
}

void IKModifier3D::set_target_node(int p_index, const NodePath &p_target_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->target_node = p_target_node;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_target_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->target_node;
}

void IKModifier3D::set_pole_node(int p_index, const NodePath &p_pole_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_node = p_pole_node;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_pole_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->pole_node;
}

void IKModifier3D::set_pole_direction(int p_index, SecondaryDirection p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_direction = p_axis;
}

SkeletonModifier3D::SecondaryDirection IKModifier3D::get_pole_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), SECONDARY_DIRECTION_NONE);
	return settings[p_index]->pole_direction;
}

void IKModifier3D::set_pole_direction_vector(int p_index, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_direction_vector = p_vector;
}

Vector3 IKModifier3D::get_pole_direction_vector(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	return settings[p_index]->get_pole_direction_vector();
}

bool IKModifier3D::is_valid(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	if (solver_type == SOLVER_TWO_BONE) {
		return settings[p_index]->root_bone.bone != -1 &&
				settings[p_index]->middle_bone.bone != -1 &&
				settings[p_index]->is_two_bone_end_valid();
	}
	return settings[p_index]->root_bone.bone != -1 && settings[p_index]->end_bone.bone != -1;
}

void IKModifier3D::set_path_3d(int p_index, const NodePath &p_path_3d) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->path_3d = p_path_3d;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_path_3d(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->path_3d;
}

void IKModifier3D::set_tilt_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_enabled = p_enabled;
}

bool IKModifier3D::is_tilt_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->tilt_enabled;
}

void IKModifier3D::set_tilt_fade_in(int p_index, int p_size) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_fade_in = p_size;
}

int IKModifier3D::get_tilt_fade_in(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->tilt_fade_in;
}

void IKModifier3D::set_tilt_fade_out(int p_index, int p_size) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_fade_out = p_size;
}

int IKModifier3D::get_tilt_fade_out(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->tilt_fade_out;
}

// Individual joints.

String IKModifier3D::get_joint_bone_name(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<BoneJoint> &joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].name;
}

int IKModifier3D::get_joint_bone(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<BoneJoint> &joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].bone;
}

void IKModifier3D::set_joint_count(int p_index, int p_count) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_count < 0);
	settings[p_index]->joints.resize(p_count);
	settings[p_index]->joint_settings.resize(p_count);
	settings[p_index]->twists.resize(p_count);
	settings[p_index]->chain_length_accum.resize(p_count);
	_make_simulation_dirty(p_index);
}

int IKModifier3D::get_joint_count(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->joints.size();
}

void IKModifier3D::set_joint_rotation_axis(int p_index, int p_joint, RotationAxis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].rotation_axis = p_axis;
	_make_simulation_dirty(p_index);
}

SkeletonModifier3D::RotationAxis IKModifier3D::get_joint_rotation_axis(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), ROTATION_AXIS_ALL);
	return js[p_joint].rotation_axis;
}

void IKModifier3D::set_joint_rotation_axis_vector(int p_index, int p_joint, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].rotation_axis_vector = p_vector;
	_make_simulation_dirty(p_index);
}

Vector3 IKModifier3D::get_joint_rotation_axis_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Vector3());
	return js[p_joint].get_rotation_axis_vector();
}

void IKModifier3D::set_joint_limitation(int p_index, int p_joint, const Ref<JointLimitation3D> &p_limitation) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation = p_limitation;
	_make_simulation_dirty(p_index);
}

Ref<JointLimitation3D> IKModifier3D::get_joint_limitation(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<JointLimitation3D>());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Ref<JointLimitation3D>());
	return js[p_joint].limitation;
}

void IKModifier3D::set_joint_limitation_right_axis(int p_index, int p_joint, SecondaryDirection p_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_right_axis = p_direction;
}

SkeletonModifier3D::SecondaryDirection IKModifier3D::get_joint_limitation_right_axis(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), SECONDARY_DIRECTION_NONE);
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), SECONDARY_DIRECTION_NONE);
	return js[p_joint].limitation_right_axis;
}

void IKModifier3D::set_joint_limitation_right_axis_vector(int p_index, int p_joint, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_right_axis_vector = p_vector;
}

Vector3 IKModifier3D::get_joint_limitation_right_axis_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Vector3());
	return js[p_joint].get_limitation_right_axis_vector();
}

void IKModifier3D::set_joint_limitation_rotation_offset(int p_index, int p_joint, const Quaternion &p_offset) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_rotation_offset = p_offset;
}

Quaternion IKModifier3D::get_joint_limitation_rotation_offset(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Quaternion());
	return js[p_joint].limitation_rotation_offset;
}

Quaternion IKModifier3D::get_joint_limitation_space(int p_index, int p_joint, const Vector3 &p_forward) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Quaternion());
	return js[p_joint].get_limitation_space(p_forward);
}

Quaternion IKModifier3D::get_local_pose_rotation(Skeleton3D *p_skeleton, int p_bone, const Quaternion &p_global_pose_rotation) {
	int parent = p_skeleton->get_bone_parent(p_bone);
	if (parent < 0) {
		return p_global_pose_rotation;
	}
	return p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion().inverse() * p_global_pose_rotation;
}

Vector3 IKModifier3D::get_bone_axis(Skeleton3D *p_skeleton, int p_end_bone, BoneDirection p_direction, bool p_mutable_bone_axes) {
	if (!p_skeleton || !p_skeleton->is_inside_tree()) {
		return Vector3();
	}
	Vector3 axis;
	if (p_direction == BONE_DIRECTION_FROM_PARENT) {
		axis = p_skeleton->get_bone_rest(p_end_bone).basis.xform_inv(p_mutable_bone_axes ? p_skeleton->get_bone_pose(p_end_bone).origin : p_skeleton->get_bone_rest(p_end_bone).origin);
		axis.normalize();
	} else {
		axis = get_vector_from_bone_axis(static_cast<BoneAxis>((int)p_direction));
	}
	return axis;
}

#ifdef TOOLS_ENABLED
Transform3D IKModifier3D::get_bone_global_rest_mutable(Skeleton3D *p_skeleton, int p_bone) {
	int current = p_bone;
	Transform3D accum;
	int parent = p_skeleton->get_bone_parent(current);
	if (parent >= 0) {
		accum = p_skeleton->get_bone_global_rest(parent);
	}
	Transform3D tr = p_skeleton->get_bone_rest(current);
	tr.origin = p_skeleton->get_bone_pose_position(current);
	accum *= tr;
	return accum;
}

Transform3D IKModifier3D::get_chain_root_global_rest(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Transform3D());
	return settings[p_index]->root_global_rest;
}

Vector3 IKModifier3D::get_bone_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<IKModifier3DSolverInfo *> &infos = settings[p_index]->solver_info_list;
	ERR_FAIL_INDEX_V(p_joint, (int)infos.size(), Vector3());
	if (!infos[p_joint]) {
		return Vector3();
	}
	return infos[p_joint]->forward_vector * infos[p_joint]->length;
}
#endif // TOOLS_ENABLED

// Internal lifecycle & validation.

void IKModifier3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	_make_all_joints_dirty();
}

void IKModifier3D::_validate_bone_names() {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	for (uint32_t i = 0; i < settings.size(); i++) {
		ChainSetting *s = settings[i];
		if (!s->root_bone.name.is_empty()) {
			s->root_bone.bone = sk->find_bone(s->root_bone.name);
		}
		if (!s->middle_bone.name.is_empty()) {
			s->middle_bone.bone = sk->find_bone(s->middle_bone.name);
		}
		if (!s->end_bone.name.is_empty()) {
			s->end_bone.bone = sk->find_bone(s->end_bone.name);
		}
		_update_joints(i);
	}
}

void IKModifier3D::_make_all_joints_dirty() {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_update_joints(i);
	}
}

void IKModifier3D::_update_joints(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ChainSetting *s = settings[p_index];
	Skeleton3D *sk = get_skeleton();
	if (!sk || s->root_bone.bone < 0 || s->end_bone.bone < 0) {
		_make_simulation_dirty(p_index);
		return;
	}
	if (solver_type != SOLVER_TWO_BONE) {
		LocalVector<int> chain_bones;
		int cur = s->end_bone.bone;
		while (cur >= 0) {
			chain_bones.push_back(cur);
			if (cur == s->root_bone.bone) {
				break;
			}
			cur = sk->get_bone_parent(cur);
		}
		if (chain_bones.is_empty() || chain_bones[chain_bones.size() - 1] != s->root_bone.bone) {
			set_joint_count(p_index, 0);
			return;
		}
		chain_bones.reverse();
		set_joint_count(p_index, chain_bones.size());
		for (uint32_t j = 0; j < chain_bones.size(); j++) {
			s->joints[j].bone = chain_bones[j];
			s->joints[j].name = sk->get_bone_name(chain_bones[j]);
		}
	}
	_make_simulation_dirty(p_index);
}

void IKModifier3D::_clear_joints(int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}
	setting->clear_solver_info();
	setting->root_pos = Vector3();
	setting->mid_pos = Vector3();
	setting->end_pos = Vector3();
	if (solver_type != SOLVER_TWO_BONE) {
		setting->solver_info_list.resize_initialized(setting->joints.size());
	}
}

void IKModifier3D::_make_simulation_dirty(int p_index) {
	ChainSetting *setting = settings[p_index];
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

void IKModifier3D::_validate_axes(Skeleton3D *p_skeleton) const {
	for (uint32_t i = 0; i < settings.size(); i++) {
		for (uint32_t j = 0; j < settings[i]->joints.size(); j++) {
			_validate_axis(p_skeleton, i, j);
		}
	}
}

void IKModifier3D::_validate_axis(Skeleton3D *p_skeleton, int p_index, int p_joint) const {
	RotationAxis axis = settings[p_index]->joint_settings[p_joint].rotation_axis;
	if (axis == ROTATION_AXIS_ALL) {
		return;
	}
	Vector3 rot = get_joint_rotation_axis_vector(p_index, p_joint).normalized();
	Vector3 fwd;
	if (p_joint < (int)settings[p_index]->joints.size() - 1) {
		fwd = p_skeleton->get_bone_rest(settings[p_index]->joints[p_joint + 1].bone).origin;
	} else if (settings[p_index]->extend_end_bone) {
		fwd = IKModifier3D::get_bone_axis(p_skeleton, settings[p_index]->end_bone.bone, settings[p_index]->end_bone_direction, mutable_bone_axes);
		if (fwd.is_zero_approx()) {
			return;
		}
	}
	fwd.normalize();
	if (Math::is_equal_approx(Math::abs(rot.dot(fwd)), 1)) {
		WARN_PRINT_ED("Setting: " + itos(p_index) + " Joint: " + itos(p_joint) + ": Rotation axis and forward vector are colinear.");
	}
}

void IKModifier3D::_validate_pole_directions(Skeleton3D *p_skeleton) const {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_validate_pole_direction(p_skeleton, i);
	}
}

void IKModifier3D::_validate_pole_direction(Skeleton3D *p_skeleton, int p_index) const {
	ChainSetting *setting = settings[p_index];
	SecondaryDirection dir = setting->pole_direction;
	if (!is_valid(p_index) || dir == SECONDARY_DIRECTION_NONE) {
		return;
	}
	Vector3 kv = get_pole_direction_vector(p_index).normalized();
	int valid_end_bone = setting->get_two_bone_end_bone();
	Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, valid_end_bone, setting->end_bone_direction, mutable_bone_axes);
	Vector3 global_rest_origin;
	if (setting->extend_end_bone && setting->end_bone_length > 0 && !axis.is_zero_approx()) {
		global_rest_origin = p_skeleton->get_bone_global_rest(valid_end_bone).xform(axis * setting->end_bone_length);
	} else {
		global_rest_origin = p_skeleton->get_bone_global_rest(valid_end_bone).origin;
	}
	axis = global_rest_origin - p_skeleton->get_bone_global_rest(setting->middle_bone.bone).origin;
	if (axis.is_zero_approx()) {
		return;
	}
	Vector3 fwd = p_skeleton->get_bone_global_rest(setting->middle_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
	if (Math::is_equal_approx(Math::abs(kv.dot(fwd)), 1)) {
		WARN_PRINT_ED("Setting: " + itos(p_index) + ": Pole direction and forward vector are colinear.");
	}
}

Transform3D IKModifier3D::_get_two_bone_global_rest(Skeleton3D *p_skeleton, int p_bone, int p_root) const {
	if (!mutable_bone_axes) {
		return p_skeleton->get_bone_global_rest(p_bone);
	}
	Transform3D tr = p_skeleton->get_bone_global_rest(p_root);
	tr.origin = tr.origin - p_skeleton->get_bone_rest(p_root).origin + p_skeleton->get_bone_pose(p_root).origin;
	LocalVector<int> path;
	for (int bone = p_bone; bone != p_root && bone != -1; bone = p_skeleton->get_bone_parent(bone)) {
		path.push_back(bone);
	}
	path.reverse();
	for (int bone : path) {
		tr = tr * Transform3D(p_skeleton->get_bone_rest(bone).basis, p_skeleton->get_bone_pose(bone).origin);
	}
	return tr;
}

void IKModifier3D::_init_joints(Skeleton3D *p_skeleton, int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}
	cached_space = p_skeleton->get_global_transform_interpolated();

	if (solver_type == SOLVER_TWO_BONE) {
		if (!setting->simulation_dirty) {
			if (mutable_bone_axes) {
				_update_bone_axis(p_skeleton, p_index);
			}
			return;
		}
		_clear_joints(p_index);
		if (setting->root_bone.bone == -1 || setting->middle_bone.bone == -1 || !setting->is_two_bone_end_valid()) {
			return;
		}
		_update_bone_axis(p_skeleton, p_index);
		setting->simulation_dirty = false;
		return;
	}

	if (solver_type == SOLVER_SPLINE) {
		if (!setting->simulation_dirty) {
			if (mutable_bone_axes) {
				_update_bone_axis(p_skeleton, p_index);
			}
			return;
		}
		_clear_joints(p_index);
		setting->chain.clear();
		bool extends_end = setting->extend_end_bone && setting->end_bone_length > 0;
		double accum = 0.0;
		for (uint32_t i = 0; i < setting->joints.size(); i++) {
			setting->chain.push_back(p_skeleton->get_bone_global_pose(setting->joints[i].bone).origin);
			bool last = i == setting->joints.size() - 1;
			if (last && extends_end) {
				Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, setting->end_bone.bone, setting->end_bone_direction, mutable_bone_axes);
				if (axis.is_zero_approx()) {
					setting->chain_length_accum[i] = accum;
					continue;
				}
				setting->solver_info_list[i] = memnew(IKModifier3DSolverInfo);
				setting->solver_info_list[i]->forward_vector = axis.normalized();
				setting->solver_info_list[i]->length = setting->end_bone_length;
				setting->chain.push_back(p_skeleton->get_bone_global_pose(setting->joints[i].bone).xform(axis * setting->end_bone_length));
			} else if (!last) {
				Vector3 axis = p_skeleton->get_bone_rest(setting->joints[i + 1].bone).origin;
				if (axis.is_zero_approx()) {
					setting->chain_length_accum[i] = accum;
					continue;
				}
				setting->solver_info_list[i] = memnew(IKModifier3DSolverInfo);
				setting->solver_info_list[i]->forward_vector = axis.normalized();
				setting->solver_info_list[i]->length = axis.length();
			}
			if (setting->solver_info_list[i]) {
				accum += setting->solver_info_list[i]->length;
			}
			setting->chain_length_accum[i] = accum;
		}
		if (mutable_bone_axes) {
			_update_bone_axis(p_skeleton, p_index);
#ifdef TOOLS_ENABLED
		} else {
			_make_gizmo_dirty();
#endif // TOOLS_ENABLED
		}
		setting->init_current_joint_rotations(p_skeleton);
		setting->simulation_dirty = false;
		return;
	}

	// Iterative solvers (CCD, FABRIK, Jacobian).
	if (setting->simulation_dirty) {
		_clear_joints(p_index);
		setting->init_iterate_joints(p_skeleton, mutable_bone_axes);
		setting->simulation_dirty = false;
	} else if (deterministic) {
		setting->init_iterate_joints(p_skeleton, mutable_bone_axes);
	}

	if (mutable_bone_axes) {
#ifdef TOOLS_ENABLED
		_update_mutable_info();
#endif // TOOLS_ENABLED
		_update_bone_axis(p_skeleton, p_index);
	}
	setting->simulated = false;
}

void IKModifier3D::_update_bone_axis(Skeleton3D *p_skeleton, int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}

	if (solver_type == SOLVER_TWO_BONE) {
		if (!setting->mid_joint_solver_info) {
			setting->mid_joint_solver_info = memnew(IKModifier3DSolverInfo);
		}
		if (!setting->root_joint_solver_info) {
			setting->root_joint_solver_info = memnew(IKModifier3DSolverInfo);
		}
		int valid_end_bone = setting->get_two_bone_end_bone();
		Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, valid_end_bone, setting->end_bone_direction, mutable_bone_axes);
		Vector3 global_rest_origin;
		if (setting->extend_end_bone && setting->end_bone_length > 0 && !axis.is_zero_approx()) {
			setting->end_pos = p_skeleton->get_bone_global_pose(valid_end_bone).xform(axis * setting->end_bone_length);
			global_rest_origin = _get_two_bone_global_rest(p_skeleton, valid_end_bone, setting->root_bone.bone).xform(axis * setting->end_bone_length);
		} else {
			setting->end_pos = p_skeleton->get_bone_global_pose(valid_end_bone).origin;
			global_rest_origin = _get_two_bone_global_rest(p_skeleton, valid_end_bone, setting->root_bone.bone).origin;
		}

		Vector3 orig = _get_two_bone_global_rest(p_skeleton, setting->middle_bone.bone, setting->root_bone.bone).origin;
		axis = global_rest_origin - orig;
		global_rest_origin = orig;
		if (!axis.is_zero_approx()) {
			setting->mid_pos = p_skeleton->get_bone_global_pose(setting->middle_bone.bone).origin;
			setting->mid_joint_solver_info->forward_vector = p_skeleton->get_bone_global_rest(setting->middle_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
			setting->mid_joint_solver_info->length = axis.length();
		} else {
			_clear_joints(p_index);
			return;
		}

		orig = _get_two_bone_global_rest(p_skeleton, setting->root_bone.bone, setting->root_bone.bone).origin;
		axis = global_rest_origin - orig;
		if (!axis.is_zero_approx()) {
			setting->root_pos = p_skeleton->get_bone_global_pose(setting->root_bone.bone).origin;
			setting->root_joint_solver_info->forward_vector = p_skeleton->get_bone_global_rest(setting->root_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
			setting->root_joint_solver_info->length = axis.length();
		} else {
			_clear_joints(p_index);
			return;
		}

		setting->init_two_bone_joint_rotations(p_skeleton);
		double total_length = setting->root_joint_solver_info->length + setting->mid_joint_solver_info->length;
		setting->cached_length_sq = total_length * total_length;
		return;
	}

	const LocalVector<BoneJoint> &joints = setting->joints;
	const LocalVector<IKModifier3DSolverInfo *> &solver_info_list = setting->solver_info_list;
	int len = (int)solver_info_list.size() - 1;
	bool use_rot_axis = is_iterative_solver();

	for (int j = 0; j < len; j++) {
		if (!solver_info_list[j]) {
			continue;
		}
		Vector3 axis = p_skeleton->get_bone_pose(joints[j + 1].bone).origin;
		if (axis.is_zero_approx()) {
			continue;
		}
		if (use_rot_axis) {
			solver_info_list[j]->forward_vector = snap_vector_to_plane(setting->joint_settings[j].get_rotation_axis_vector(), axis.normalized());
		} else {
			solver_info_list[j]->forward_vector = axis.normalized();
		}
		solver_info_list[j]->length = axis.length();
	}
	if (setting->extend_end_bone && len >= 0 && solver_info_list[len]) {
		Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, setting->end_bone.bone, setting->end_bone_direction, mutable_bone_axes);
		if (!axis.is_zero_approx()) {
			if (use_rot_axis) {
				solver_info_list[len]->forward_vector = snap_vector_to_plane(setting->joint_settings[len].get_rotation_axis_vector(), axis.normalized());
			} else {
				solver_info_list[len]->forward_vector = axis.normalized();
			}
			solver_info_list[len]->length = setting->end_bone_length;
		}
	}
}

void IKModifier3D::_process_modification(double p_delta) {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	_process_ik(sk, p_delta);
}

void IKModifier3D::_process_ik(Skeleton3D *p_skeleton, double p_delta) {
	Transform3D skel_inv = p_skeleton->get_global_transform_interpolated().affine_inverse();
	for (uint32_t i = 0; i < settings.size(); i++) {
		ChainSetting *setting = settings[i];
		if (!setting) {
			continue;
		}
		_init_joints(p_skeleton, i);

		if (solver_type == SOLVER_TWO_BONE) {
			if (!setting->is_two_bone_valid()) {
				continue;
			}
			Node3D *target = dynamic_cast<Node3D *>(get_node_or_null(setting->target_node));
			if (!target) {
				continue;
			}
			Vector3 dest = skel_inv.xform(target->get_global_transform_interpolated().origin);
			Vector3 pole_dest;
			Node3D *pole = dynamic_cast<Node3D *>(get_node_or_null(setting->pole_node));
			if (pole) {
				pole_dest = skel_inv.xform(pole->get_global_transform_interpolated().origin);
			}
			_process_two_bone_joints(p_delta, p_skeleton, setting, dest, pole_dest);
		} else if (solver_type == SOLVER_SPLINE) {
			Path3D *path = dynamic_cast<Path3D *>(get_node_or_null(setting->path_3d));
			if (!path || path->get_curve().is_null() || path->get_curve()->get_point_count() == 0) {
				continue;
			}
			Transform3D curve_space = skel_inv * path->get_global_transform_interpolated();
			_process_spline_joints(p_delta, p_skeleton, setting, path->get_curve(), curve_space);
		} else {
			if (setting->chain.is_empty()) {
				continue;
			}
			Node3D *target = dynamic_cast<Node3D *>(get_node_or_null(setting->target_node));
			if (!target) {
				continue;
			}
			Vector3 dest = skel_inv.xform(target->get_global_transform_interpolated().origin);
			_process_iterate_joints(p_delta, p_skeleton, setting, dest);
		}
	}
}

void IKModifier3D::_process_iterate_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	double distance_to_target_sq = INFINITY;
	int iteration_count = 0;

	if (p_setting->simulated) {
		distance_to_target_sq = p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
	}

	while (distance_to_target_sq > min_distance_squared && iteration_count < max_iterations) {
		switch (solver_type) {
			case SOLVER_CCD:
				_solve_ccd_iteration(p_skeleton, p_setting, p_destination);
				break;
			case SOLVER_FABRIK:
				_solve_fabrik_iteration(p_skeleton, p_setting, p_destination);
				break;
			case SOLVER_JACOBIAN:
				_solve_jacobian_iteration(p_skeleton, p_setting, p_destination);
				break;
			default:
				break;
		}
		p_setting->cache_iterate_joint_rotations(p_skeleton, angular_delta_limit);
		distance_to_target_sq = p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
		iteration_count++;
	}

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		p_skeleton->set_bone_pose_rotation(p_setting->joints[i].bone, solver_info->current_lpose);
	}

	p_setting->simulated = true;
}

void IKModifier3D::_solve_ccd_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();
	int chain_size = (int)p_setting->chain.size();

	for (int ancestor = joint_size - 1; ancestor >= 0; ancestor--) {
		for (int i = ancestor; i < joint_size; i++) {
			IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
			if (!solver_info || Math::is_zero_approx(solver_info->length)) {
				continue;
			}
			int HEAD = i;
			int TAIL = i + 1;

			Vector3 current_head = p_setting->chain[HEAD];
			Vector3 current_effector = p_setting->chain[chain_size - 1];
			Vector3 head_to_effector = current_effector - current_head;
			Vector3 head_to_destination = p_destination - current_head;

			if (Math::is_zero_approx(head_to_destination.length_squared() * head_to_effector.length_squared())) {
				continue;
			}

			Quaternion to_rot = Quaternion(head_to_effector.normalized(), head_to_destination.normalized());
			Vector3 to_tail = p_setting->chain[TAIL] - current_head;

			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, current_head + to_rot.xform(to_tail));

			if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
				p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD]));
			}
			if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
				p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD], solver_info->forward_vector));
			}
		}
	}
}

void IKModifier3D::_solve_fabrik_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();

	// Backwards.
	bool first = true;
	for (int i = joint_size - 1; i >= 0; i--) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		if (first) {
			p_setting->update_chain_coordinate_bw(p_skeleton, TAIL, p_destination);
			first = false;
		}

		p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, limit_length(p_setting->chain[TAIL], p_setting->chain[HEAD], solver_info->length));

		if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
			p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, p_setting->chain[TAIL] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[HEAD] - p_setting->chain[TAIL]));
		}
		if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
			p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, p_setting->chain[TAIL] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[HEAD] - p_setting->chain[TAIL], solver_info->forward_vector));
		}
	}

	// Forwards.
	first = true;
	for (int i = 0; i < joint_size; i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		if (first) {
			p_setting->update_chain_coordinate_fw(p_skeleton, HEAD, p_skeleton->get_bone_global_pose(p_setting->joints[HEAD].bone).origin);
			first = false;
		}

		p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[TAIL], solver_info->length));

		if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD]));
		}
		if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD], solver_info->forward_vector));
		}
	}
}

void IKModifier3D::_solve_jacobian_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();
	int chain_size = (int)p_setting->chain.size();

	for (int i = 0; i < joint_size; i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		Vector3 current_head = p_setting->chain[HEAD];
		Vector3 current_effector = p_setting->chain[chain_size - 1];
		Vector3 head_to_effector = current_effector - current_head;
		Vector3 effector_to_destination = p_destination - current_effector;
		Vector3 axis = head_to_effector.cross(effector_to_destination);

		if (Math::is_zero_approx(axis.length_squared())) {
			continue;
		}

		Quaternion to_rot = Quaternion(axis.normalized(), MIN(axis.length() / MAX(CMP_EPSILON, head_to_effector.length_squared()), angular_delta_limit));

		for (int j = TAIL; j < chain_size; j++) {
			Vector3 to_tail = p_setting->chain[j] - current_head;
			p_setting->update_chain_coordinate_fw(p_skeleton, j, current_head + to_rot.xform(to_tail));

			int k = j - 1;
			if (p_setting->joint_settings[k].rotation_axis != ROTATION_AXIS_ALL) {
				p_setting->update_chain_coordinate_fw(p_skeleton, j, p_setting->chain[k] + p_setting->joint_settings[k].get_projected_rotation(solver_info->current_grest, p_setting->chain[j] - p_setting->chain[k]));
			}
			if (p_setting->joint_settings[k].limitation.is_valid()) {
				p_setting->update_chain_coordinate_fw(p_skeleton, j, p_setting->chain[k] + p_setting->joint_settings[k].get_limited_rotation(solver_info->current_grest, p_setting->chain[j] - p_setting->chain[k], solver_info->forward_vector));
			}
		}
	}
}

void IKModifier3D::_process_spline_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, Ref<Curve3D> p_curve, const Transform3D &p_curve_space) {
	if (p_setting->solver_info_list.is_empty()) {
		return;
	}
	uint32_t joint_count = p_setting->joints.size();
	uint32_t joint_last = joint_count - 1;

	double path_length = p_curve->get_baked_length();
	PackedVector3Array points = p_curve->get_baked_points();
	Vector<real_t> tilts = p_curve->get_baked_tilts();
	Vector<real_t> dists = p_curve->get_baked_dist_cache();
	uint32_t point_count = points.size();
	if (point_count == 0) {
		return;
	}
	uint32_t point_last = point_count - 1;

	Vector3 start_point = p_curve_space.xform(points[0]);
	Vector3 start_vector = start_point - p_skeleton->get_bone_global_pose(p_setting->joints[0].bone).origin;
	double start_dist = start_vector.length();

	uint32_t chain_path_start = 0;
	while (chain_path_start < joint_count) {
		if (p_setting->chain_length_accum[chain_path_start] >= start_dist) {
			break;
		}
		chain_path_start++;
	}
	chain_path_start = (uint32_t)CLAMP((int)chain_path_start, 0, (int)joint_last);

	double fade_in_denom = 0.0;
	int denom_start = p_setting->tilt_fade_in > 0 ? CLAMP(p_setting->tilt_fade_in - 1, (int)chain_path_start, (int)joint_count) : -1;
	int denom_start_to = denom_start - p_setting->tilt_fade_in;
	if (denom_start >= 0) {
		for (int i = denom_start; i > denom_start_to; i--) {
			if (i < 0) {
				break;
			}
			IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
			if (!solver_info || Math::is_zero_approx(solver_info->length)) {
				continue;
			}
			fade_in_denom += solver_info->length;
		}
	}

	uint32_t ended = 0;
	Vector3 end_point = p_curve_space.xform(points[point_last]);
	Vector3 end_vector;
	double end_to_end_length = 0.0;
	double fade_out_denom = 0.0;

	uint32_t last_nearest = 0;
	uint32_t last_nearest_next = 0;
	double last_interpolate = 0.0;

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		uint32_t HEAD = i;
		uint32_t TAIL = i + 1;
		bool is_fitting_first = HEAD == chain_path_start;

		if (ended > 0) {
			p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[HEAD] + end_vector, solver_info->length));
			if (p_setting->tilt_enabled) {
				if (p_setting->tilt_fade_out < 0) {
					p_setting->twists[HEAD] = 0.0;
				} else if (p_setting->tilt_fade_out == 0) {
					p_setting->twists[HEAD] = tilts[point_last];
				} else {
					if (ended == 1) {
						end_to_end_length = p_setting->chain[TAIL].distance_to(end_point);
					} else {
						end_to_end_length += solver_info->length;
					}
					double fade_out_dumping = CLAMP(end_to_end_length / fade_out_denom, 0.0, 1.0);
					p_setting->twists[HEAD] = Math::lerp(ended == 1 ? Math::lerp((double)tilts[last_nearest], (double)tilts[last_nearest_next], last_interpolate) : (double)tilts[point_last], 0.0, fade_out_dumping);
					ended = 2;
				}
			}
			continue;
		} else if (point_count == 1 || HEAD <= chain_path_start) {
			bool update_coordinate = !is_fitting_first || point_count == 1;
			if (update_coordinate) {
				p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[HEAD] + start_vector, solver_info->length));
			}
			if (p_setting->tilt_enabled) {
				if (p_setting->tilt_fade_in < 0) {
					p_setting->twists[HEAD] = 0.0;
				} else if (p_setting->tilt_fade_in == 0) {
					p_setting->twists[HEAD] = tilts[0];
				} else {
					double fade_in_dumping = CLAMP((double)(p_setting->chain[HEAD].distance_to(start_point) / fade_in_denom), 0.0, 1.0);
					p_setting->twists[HEAD] = Math::lerp((double)tilts[0], 0.0, fade_in_dumping);
				}
			}
			if (update_coordinate) {
				continue;
			}
		}

		double lsq = solver_info->length * solver_info->length;
		Vector3 head_in_chain_space = p_curve_space.xform_inv(p_setting->chain[HEAD]);
		double interpolate = 0.0;
		uint32_t nearest = p_setting->find_nearest_point(head_in_chain_space, lsq, points, p_curve->is_closed(), last_nearest, &interpolate);
		if (nearest >= point_count) {
			if (HEAD == 0) {
				nearest = point_count - 2;
				interpolate = 1.0;
			} else {
				Vector3 chain_end = (p_setting->chain[HEAD] - p_setting->chain[HEAD - 1]).normalized();
				Vector3 path_end = (p_curve_space.xform(points[point_last]) - p_setting->chain[HEAD]).normalized();
				double rest_path_length = path_length - Math::lerp((double)dists[last_nearest], (double)dists[last_nearest_next], last_interpolate);
				interpolate = CLAMP(rest_path_length / solver_info->length, 0.0, 1.0);
				end_vector = chain_end.lerp(path_end, interpolate);

				int denom_end = p_setting->tilt_fade_out > 0 ? CLAMP((int)joint_last - p_setting->tilt_fade_out, 0, (int)last_nearest) : -1;
				int denom_end_to = denom_end + p_setting->tilt_fade_out;
				if (denom_end >= 0) {
					for (int e = denom_end; e < denom_end_to; e++) {
						if (e >= (int)joint_count) {
							break;
						}
						IKModifier3DSolverInfo *end_solver_info = p_setting->solver_info_list[e];
						if (!end_solver_info || Math::is_zero_approx(end_solver_info->length)) {
							continue;
						}
						fade_out_denom += end_solver_info->length;
					}
				}
				ended = 1;
				i--;
				continue;
			}
		}
		uint32_t nearest_next = p_curve->is_closed() ? Math::posmod(nearest + 1, point_count) : CLAMP(nearest, (uint32_t)0, point_last);
		p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_curve_space.xform(points[nearest].lerp(points[nearest_next], interpolate)), solver_info->length));
		if (!is_fitting_first) {
			p_setting->twists[HEAD] = Math::lerp((double)tilts[last_nearest], (double)tilts[last_nearest_next], last_interpolate);
		}
		last_nearest = nearest;
		last_nearest_next = nearest_next;
		last_interpolate = interpolate;
	}

	p_setting->cache_spline_joint_rotations(p_skeleton, p_setting->tilt_enabled);

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		p_skeleton->set_bone_pose_rotation(p_setting->joints[i].bone, solver_info->current_lpose);
	}
}

void IKModifier3D::_process_two_bone_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination, const Vector3 &p_pole_destination) {
	Vector3 destination = p_destination;
	p_setting->root_pos = p_skeleton->get_bone_global_pose(p_setting->root_bone.bone).origin;
	Vector3 root_to_destination = destination - p_setting->root_pos;
	if (root_to_destination.is_zero_approx()) {
		return;
	}

	double rd_len_sq = root_to_destination.length_squared();
	if (rd_len_sq >= p_setting->cached_length_sq) {
		Vector3 rd_nrm = root_to_destination.normalized();
		p_setting->mid_pos = p_setting->root_pos + rd_nrm * p_setting->root_joint_solver_info->length;
		p_setting->end_pos = p_setting->mid_pos + rd_nrm * p_setting->mid_joint_solver_info->length;
	} else {
		double sub = p_setting->root_joint_solver_info->length - p_setting->mid_joint_solver_info->length;
		if (rd_len_sq < sub * sub) {
			Vector3 push_nrm = (destination - p_setting->root_pos).normalized();
			destination = p_setting->root_pos + push_nrm * Math::abs(sub);
			root_to_destination = destination - p_setting->root_pos;
		}
		p_setting->end_pos = destination;

		double l_chain = root_to_destination.length();
		Vector3 u = root_to_destination.normalized();
		Vector3 pole_vec = get_projected_normal(p_setting->root_pos, p_setting->end_pos, p_pole_destination);

		double r_root = p_setting->root_joint_solver_info->length;
		double r_mid = p_setting->mid_joint_solver_info->length;

		double a = (l_chain * l_chain + r_root * r_root - r_mid * r_mid) / (2.0 * l_chain);
		double h2 = MAX(0.0, r_root * r_root - a * a);
		double h = Math::sqrt(h2);

		Vector3 det_plus = (p_setting->root_pos + u * a) + pole_vec * h;
		Vector3 det_minus = (p_setting->root_pos + u * a) - pole_vec * h;

		p_setting->mid_pos = p_pole_destination.distance_squared_to(det_plus) < p_pole_destination.distance_squared_to(det_minus) ? det_plus : det_minus;
	}

	p_setting->cache_two_bone_vectors(p_skeleton);
	p_setting->cache_two_bone_joint_rotations(p_skeleton, p_pole_destination);

	p_skeleton->set_bone_pose_rotation(p_setting->root_bone.bone, p_setting->root_joint_solver_info->current_lpose);
	p_skeleton->set_bone_pose_rotation(p_setting->middle_bone.bone, get_local_pose_rotation(p_skeleton, p_setting->middle_bone.bone, p_setting->mid_joint_solver_info->current_gpose));
}
