/**************************************************************************/
/*  ik_modifier_3d.h                                                      */
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

#pragma once

#include "scene/3d/skeleton_modifier_3d.h"
#include "scene/resources/3d/joint_limitation_3d.h"

class Curve3D;

class IKModifier3D : public SkeletonModifier3D {
public:
	enum SolverType {
		SOLVER_TWO_BONE,
		SOLVER_SPLINE,
		SOLVER_CCD,
		SOLVER_FABRIK,
		SOLVER_JACOBIAN,
	};

	struct BoneJoint {
		String name;
		int bone = -1;
	};

	struct IKModifier3DSolverInfo {
		Quaternion current_lpose;
		Quaternion current_lrest;
		Quaternion current_gpose;
		Quaternion current_grest;
		Vector3 current_vector; // Global so needs xform_inv by gpose or grest in the process.
		Vector3 forward_vector; // Local.
		float length = 0.0f;
	};

	struct JointSetting {
		RotationAxis rotation_axis = ROTATION_AXIS_ALL;
		Vector3 rotation_axis_vector = Vector3(1, 0, 0);
		Ref<JointLimitation3D> limitation;
		SecondaryDirection limitation_right_axis = SECONDARY_DIRECTION_NONE;
		Vector3 limitation_right_axis_vector = Vector3(1, 0, 0);
		Quaternion limitation_rotation_offset;

		Vector3 get_rotation_axis_vector() const;
		Vector3 get_limitation_right_axis_vector() const;
		Quaternion get_limitation_space(const Vector3 &p_local_forward) const;
		Vector3 get_projected_rotation(const Quaternion &p_offset, const Vector3 &p_vector) const;
		Vector3 get_limited_rotation(const Quaternion &p_offset, const Vector3 &p_vector, const Vector3 &p_forward) const;
	};

	struct ChainSetting {
		bool simulation_dirty = true;
		bool joints_dirty = false;
		bool simulated = false;

#ifdef TOOLS_ENABLED
		Transform3D root_global_rest;
#endif // TOOLS_ENABLED

		BoneJoint root_bone;
		BoneJoint middle_bone; // Used by SOLVER_TWO_BONE.
		BoneJoint end_bone;

		bool use_virtual_end = false;
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float end_bone_length = 0.0f;

		// Target & Pole (IterateIK / TwoBoneIK).
		NodePath target_node;
		NodePath pole_node;
		SecondaryDirection pole_direction = SECONDARY_DIRECTION_NONE;
		Vector3 pole_direction_vector;

		// Spline (SplineIK).
		NodePath path_3d;
		bool tilt_enabled = true;
		int tilt_fade_in = 1;
		int tilt_fade_out = 1;
		LocalVector<double> chain_length_accum;
		LocalVector<double> twists;

		// Chain joints & solver info (IterateIK / SplineIK).
		LocalVector<BoneJoint> joints;
		LocalVector<JointSetting> joint_settings;
		LocalVector<IKModifier3DSolverInfo *> solver_info_list;
		LocalVector<Vector3> chain;

		// Two-bone state (TwoBoneIK).
		IKModifier3DSolverInfo *root_joint_solver_info = nullptr;
		IKModifier3DSolverInfo *mid_joint_solver_info = nullptr;
		Vector3 root_pos;
		Vector3 mid_pos;
		Vector3 end_pos;
		double cached_length_sq = 0.0;

		bool is_two_bone_valid() const { return root_joint_solver_info && mid_joint_solver_info; }
		bool is_two_bone_end_valid() const {
			return (!use_virtual_end && end_bone.bone != -1) || (use_virtual_end && !Math::is_zero_approx(end_bone_length));
		}
		int get_two_bone_end_bone() const { return use_virtual_end ? middle_bone.bone : end_bone.bone; }
		Vector3 get_pole_direction_vector() const;

		void clear_solver_info();

		void update_chain_coordinate(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void update_chain_coordinate_bw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void update_chain_coordinate_fw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void cache_current_vector(Skeleton3D *p_skeleton, int p_index);
		void cache_current_vectors(Skeleton3D *p_skeleton);
		void init_current_joint_rotations(Skeleton3D *p_skeleton);

		void cache_iterate_joint_rotations(Skeleton3D *p_skeleton, double p_angular_delta_limit = Math::PI);
		void init_iterate_joints(Skeleton3D *p_skeleton, bool p_mutable_bone_axes);

		int32_t find_nearest_point(const Vector3 &p_origin, double p_length_sq, const PackedVector3Array &p_points, bool p_is_path_closed, uint32_t p_first, double *r_ret) const;
		void cache_spline_joint_rotations(Skeleton3D *p_skeleton, bool p_use_tilt = false);

		void cache_two_bone_vectors(Skeleton3D *p_skeleton);
		void init_two_bone_joint_rotations(Skeleton3D *p_skeleton);
		void cache_two_bone_joint_rotations(Skeleton3D *p_skeleton, const Vector3 &p_pole_destination);

		~ChainSetting() { clear_solver_info(); }
	};

protected:
#ifdef TOOLS_ENABLED
	bool saving = false;
	bool gizmo_dirty = false;
	void _make_gizmo_dirty();
	void _update_mutable_info();
#endif // TOOLS_ENABLED

	SolverType solver_type = SOLVER_FABRIK;
	bool mutable_bone_axes = true;
	Transform3D cached_space;
	bool joints_dirty = false;

	// Iterative solver settings (CCD / FABRIK / Jacobian).
	int max_iterations = 4;
	double min_distance = 0.001;
	double min_distance_squared = min_distance * min_distance;
	double angular_delta_limit = Math::deg_to_rad(2.0);
	bool deterministic = false;

	LocalVector<ChainSetting *> settings;

	void _notification(int p_what);
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _make_all_joints_dirty();
	void _init_joints(Skeleton3D *p_skeleton, int p_index);
	void _update_joints(int p_index);
	void _clear_joints(int p_index);
	void _make_simulation_dirty(int p_index);
	void _update_bone_axis(Skeleton3D *p_skeleton, int p_index);

	void _validate_axes(Skeleton3D *p_skeleton) const;
	void _validate_axis(Skeleton3D *p_skeleton, int p_index, int p_joint) const;
	void _validate_pole_directions(Skeleton3D *p_skeleton) const;
	void _validate_pole_direction(Skeleton3D *p_skeleton, int p_index) const;
	Transform3D _get_two_bone_global_rest(Skeleton3D *p_skeleton, int p_bone, int p_root) const;

	void _process_ik(Skeleton3D *p_skeleton, double p_delta);
	void _process_iterate_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_ccd_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_fabrik_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_jacobian_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _process_spline_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, Ref<Curve3D> p_curve, const Transform3D &p_curve_space);
	void _process_two_bone_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination, const Vector3 &p_pole_destination);

public:
	IKModifier3D(SolverType p_solver_type = SOLVER_FABRIK);
	~IKModifier3D();

	void set_solver_type(SolverType p_solver_type);
	SolverType get_solver_type() const { return solver_type; }
	bool is_iterative_solver() const {
		return solver_type == SOLVER_CCD || solver_type == SOLVER_FABRIK || solver_type == SOLVER_JACOBIAN;
	}

	virtual PackedStringArray get_configuration_warnings() const override;

	int get_setting_count() const;
	void set_setting_count(int p_count);
	void clear_settings();

	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;

	// Iterative solver settings.
	void set_max_iterations(int p_max_iterations);
	int get_max_iterations() const;
	void set_min_distance(double p_min_distance);
	double get_min_distance() const;
	void set_angular_delta_limit(double p_angular_delta_limit);
	double get_angular_delta_limit() const;
	void set_deterministic(bool p_deterministic);
	bool is_deterministic() const;

	// Chain / TwoBone bone configuration.
	void set_root_bone_name(int p_index, const String &p_bone_name);
	String get_root_bone_name(int p_index) const;
	void set_root_bone(int p_index, int p_bone);
	int get_root_bone(int p_index) const;

	void set_middle_bone_name(int p_index, const String &p_bone_name);
	String get_middle_bone_name(int p_index) const;
	void set_middle_bone(int p_index, int p_bone);
	int get_middle_bone(int p_index) const;

	void set_end_bone_name(int p_index, const String &p_bone_name);
	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	void set_use_virtual_end(int p_index, bool p_enabled);
	bool is_using_virtual_end(int p_index) const;
	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_end_bone_length(int p_index, float p_length);
	float get_end_bone_length(int p_index) const;

	// Target / Pole / Path3D settings.
	void set_target_node(int p_index, const NodePath &p_target_node);
	NodePath get_target_node(int p_index) const;

	void set_pole_node(int p_index, const NodePath &p_pole_node);
	NodePath get_pole_node(int p_index) const;
	void set_pole_direction(int p_index, SecondaryDirection p_axis);
	SecondaryDirection get_pole_direction(int p_index) const;
	void set_pole_direction_vector(int p_index, const Vector3 &p_vector);
	Vector3 get_pole_direction_vector(int p_index) const;
	bool is_valid(int p_index) const;

	void set_path_3d(int p_index, const NodePath &p_path_3d);
	NodePath get_path_3d(int p_index) const;
	void set_tilt_enabled(int p_index, bool p_enabled);
	bool is_tilt_enabled(int p_index) const;
	void set_tilt_fade_in(int p_index, int p_size);
	int get_tilt_fade_in(int p_index) const;
	void set_tilt_fade_out(int p_index, int p_size);
	int get_tilt_fade_out(int p_index) const;

	// Individual joints.
	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;
	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;

	void set_joint_rotation_axis(int p_index, int p_joint, RotationAxis p_axis);
	RotationAxis get_joint_rotation_axis(int p_index, int p_joint) const;
	void set_joint_rotation_axis_vector(int p_index, int p_joint, const Vector3 &p_vector);
	Vector3 get_joint_rotation_axis_vector(int p_index, int p_joint) const;
	void set_joint_limitation(int p_index, int p_joint, const Ref<JointLimitation3D> &p_limitation);
	Ref<JointLimitation3D> get_joint_limitation(int p_index, int p_joint) const;
	void set_joint_limitation_right_axis(int p_index, int p_joint, SecondaryDirection p_direction);
	SecondaryDirection get_joint_limitation_right_axis(int p_index, int p_joint) const;
	void set_joint_limitation_right_axis_vector(int p_index, int p_joint, const Vector3 &p_vector);
	Vector3 get_joint_limitation_right_axis_vector(int p_index, int p_joint) const;
	void set_joint_limitation_rotation_offset(int p_index, int p_joint, const Quaternion &p_offset);
	Quaternion get_joint_limitation_rotation_offset(int p_index, int p_joint) const;
	Quaternion get_joint_limitation_space(int p_index, int p_joint, const Vector3 &p_forward) const;

#ifdef TOOLS_ENABLED
	static Transform3D get_bone_global_rest_mutable(Skeleton3D *p_skeleton, int p_bone);
	Transform3D get_chain_root_global_rest(int p_index) const;
	Vector3 get_bone_vector(int p_index, int p_joint) const;
#endif // TOOLS_ENABLED

	static Quaternion get_local_pose_rotation(Skeleton3D *p_skeleton, int p_bone, const Quaternion &p_global_pose_rotation);
	static Vector3 get_bone_axis(Skeleton3D *p_skeleton, int p_end_bone, BoneDirection p_direction, bool p_mutable_bone_axes);
};
