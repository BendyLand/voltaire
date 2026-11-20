/**************************************************************************/
/*  skeleton_modification_2d.h                                            */
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

#include "scene/resources/2d/skeleton/skeleton_modification_stack_2d.h"

class Bone2D;
class Node2D;

class SkeletonModification2D final : public Resource {
	friend class Skeleton2D;
	friend class Bone2D;

public:
	enum Type {
		MODIFICATION_NONE,
		MODIFICATION_LOOKAT,
		MODIFICATION_CCDIK,
		MODIFICATION_FABRIK,
		MODIFICATION_JIGGLE,
		MODIFICATION_TWOBONEIK,
		MODIFICATION_PHYSICALBONES,
		MODIFICATION_STACKHOLDER,
	};

	struct LookAtData;
	struct CCDIKData;
	struct FABRIKData;
	struct JiggleData;
	struct TwoBoneIKData;
	struct PhysicalBonesData;
	struct StackHolderData;

private:
	Type modification_type = MODIFICATION_NONE;
	void *mode_data = nullptr;

	SkeletonModificationStack2D *stack = nullptr;
	int execution_mode = 0; // 0 = process
	NodePath target_node;

	bool enabled = true;
	bool is_setup = false;

	void _alloc_mode_data(Type p_type);
	void _free_mode_data();
	void _ensure_mode(Type p_expected);

	LookAtData *_get_lookat();
	const LookAtData *_get_lookat() const;
	CCDIKData *_get_ccdik();
	const CCDIKData *_get_ccdik() const;
	FABRIKData *_get_fabrik();
	const FABRIKData *_get_fabrik() const;
	JiggleData *_get_jiggle();
	const JiggleData *_get_jiggle() const;
	TwoBoneIKData *_get_twoboneik();
	const TwoBoneIKData *_get_twoboneik() const;
	PhysicalBonesData *_get_physicalbones();
	const PhysicalBonesData *_get_physicalbones() const;
	StackHolderData *_get_stackholder();
	const StackHolderData *_get_stackholder() const;

	Node2D *_resolve_node_2d(const NodePath &p_path) const;
	Bone2D *_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const;

	void _execute_lookat(float p_delta);
	void _execute_ccdik(float p_delta);
	void _execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip);
	void _execute_fabrik(float p_delta);
	void _fabrik_chain_backwards();
	void _fabrik_chain_forwards();
	void _execute_jiggle(float p_delta);
	void _execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta);
	void _update_jiggle_joint_data();
	void _execute_twoboneik(float p_delta);
	void _execute_physicalbones(float p_delta);
	void _execute_stackholder(float p_delta);

	bool _print_execution_error(bool p_condition, const String &p_message);

	virtual void reset_state() override;

public:
	bool editor_draw_gizmo = false;

	SkeletonModification2D(Type p_type = MODIFICATION_NONE);
	~SkeletonModification2D();

	void set_modification_type(Type p_type);
	Type get_modification_type() const { return modification_type; }

	void _execute(float p_delta);
	void _setup_modification(SkeletonModificationStack2D *p_stack);
	void _draw_editor_gizmo();

	void set_editor_draw_gizmo(bool p_draw_gizmo);
	bool get_editor_draw_gizmo() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	Ref<SkeletonModificationStack2D> get_modification_stack();
	void set_is_setup(bool p_setup);
	bool get_is_setup() const;

	void set_execution_mode(int p_mode);
	int get_execution_mode() const;

	float clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert_clamp = false);
	void editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted);

	// Shared target node (LookAt, CCDIK, FABRIK, Jiggle, TwoBoneIK).
	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	// LookAt accessors.
	void set_bone2d_node(const NodePath &p_target_node);
	NodePath get_bone2d_node() const;
	void set_bone_index(int p_idx);
	int get_bone_index() const;
	void set_additional_rotation(float p_rotation);
	float get_additional_rotation() const;
	void set_enable_constraint(bool p_constraint);
	bool get_enable_constraint() const;
	void set_constraint_angle_min(float p_angle_min);
	float get_constraint_angle_min() const;
	void set_constraint_angle_max(float p_angle_max);
	float get_constraint_angle_max() const;
	void set_constraint_angle_invert(bool p_invert);
	bool get_constraint_angle_invert() const;
	void set_constraint_in_localspace(bool p_constraint_in_localspace);
	bool get_constraint_in_localspace() const;

	// CCDIK accessors.
	void set_tip_node(const NodePath &p_tip_node);
	NodePath get_tip_node() const;
	int get_ccdik_data_chain_length() const;
	void set_ccdik_data_chain_length(int p_new_length);
	void set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_ccdik_joint_bone2d_node(int p_joint_idx) const;
	void set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_ccdik_joint_bone_index(int p_joint_idx) const;
	void set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint);
	bool get_ccdik_joint_rotate_from_joint(int p_joint_idx) const;
	void set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint);
	bool get_ccdik_joint_enable_constraint(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min);
	float get_ccdik_joint_constraint_angle_min(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max);
	float get_ccdik_joint_constraint_angle_max(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert);
	bool get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const;
	void set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace);
	bool get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const;
	void set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo);
	bool get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const;

	// FABRIK accessors.
	int get_fabrik_data_chain_length() const;
	void set_fabrik_data_chain_length(int p_new_length);
	void set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_fabrik_joint_bone2d_node(int p_joint_idx) const;
	void set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_fabrik_joint_bone_index(int p_joint_idx) const;
	void set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position);
	Vector2 get_fabrik_joint_magnet_position(int p_joint_idx) const;
	void set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation);
	bool get_fabrik_joint_use_target_rotation(int p_joint_idx) const;
	void set_chain_tolerance(float p_tolerance);
	float get_chain_tolerance() const;
	void set_chain_max_iterations(int p_iterations);
	int get_chain_max_iterations() const;

	// Jiggle accessors.
	void reset();
	void set_stiffness(float p_stiffness);
	float get_stiffness() const;
	void set_mass(float p_mass);
	float get_mass() const;
	void set_damping(float p_damping);
	float get_damping() const;
	void set_use_gravity(bool p_use_gravity);
	bool get_use_gravity() const;
	void set_gravity(Vector2 p_gravity);
	Vector2 get_gravity() const;
	void set_use_colliders(bool p_use_colliders);
	bool get_use_colliders() const;
	void set_collision_mask(int p_mask);
	int get_collision_mask() const;
	int get_jiggle_data_chain_length() const;
	void set_jiggle_data_chain_length(int p_new_length);
	void set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_jiggle_joint_bone2d_node(int p_joint_idx) const;
	void set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_jiggle_joint_bone_index(int p_joint_idx) const;
	void set_jiggle_joint_override(int p_joint_idx, bool p_override);
	bool get_jiggle_joint_override(int p_joint_idx) const;
	void set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness);
	float get_jiggle_joint_stiffness(int p_joint_idx) const;
	void set_jiggle_joint_mass(int p_joint_idx, float p_mass);
	float get_jiggle_joint_mass(int p_joint_idx) const;
	void set_jiggle_joint_damping(int p_joint_idx, float p_damping);
	float get_jiggle_joint_damping(int p_joint_idx) const;
	void set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity);
	bool get_jiggle_joint_use_gravity(int p_joint_idx) const;
	void set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity);
	Vector2 get_jiggle_joint_gravity(int p_joint_idx) const;

	// TwoBoneIK accessors.
	void set_target_minimum_distance(float p_minimum_distance);
	float get_target_minimum_distance() const;
	void set_target_maximum_distance(float p_maximum_distance);
	float get_target_maximum_distance() const;
	void set_flip_bend_direction(bool p_flip_direction);
	bool get_flip_bend_direction() const;
	void set_joint_one_bone2d_node(const NodePath &p_node);
	NodePath get_joint_one_bone2d_node() const;
	void set_joint_one_bone_idx(int p_bone_idx);
	int get_joint_one_bone_idx() const;
	void set_joint_two_bone2d_node(const NodePath &p_node);
	NodePath get_joint_two_bone2d_node() const;
	void set_joint_two_bone_idx(int p_bone_idx);
	int get_joint_two_bone_idx() const;
#ifdef TOOLS_ENABLED
	void set_editor_draw_min_max(bool p_draw);
	bool get_editor_draw_min_max() const;
#endif // TOOLS_ENABLED

	// PhysicalBones accessors.
	int get_physical_bone_chain_length() const;
	void set_physical_bone_chain_length(int p_new_length);
	void set_physical_bone_node(int p_joint_idx, const NodePath &p_path);
	NodePath get_physical_bone_node(int p_joint_idx) const;
	void fetch_physical_bones();
	void start_simulation(const Vector<String> &p_bones = Vector<String>());
	void stop_simulation(const Vector<String> &p_bones = Vector<String>());

	// StackHolder accessors.
	void set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack);
	Ref<SkeletonModificationStack2D> get_held_modification_stack() const;
};
