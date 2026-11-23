/**************************************************************************/
/*  skeleton_3d.h                                                         */
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

#include "core/templates/a_hash_map.h"
#include "scene/3d/node_3d.h"
// --- Inlined/Folded from scene/resources/3d/skin.h ---
#include "core/io/resource.h"

class Skin : public Resource
{
	struct Bind
	{
		int bone = -1;
		String name;
		Transform3D pose;
	};

	Vector<Bind> binds;

	Bind* binds_ptr = nullptr;
	int bind_count = 0;

protected:
	virtual void reset_state() override;

public:
	void set_bind_count(int p_size);

	inline int get_bind_count() const { return bind_count; }

	void add_bind(int p_bone, const Transform3D& p_pose);
	void add_named_bind(const String& p_name, const Transform3D& p_pose);

	void set_bind_bone(int p_index, int p_bone);
	void set_bind_pose(int p_index, const Transform3D& p_pose);
	void set_bind_name(int p_index, const String& p_name);

	inline int get_bind_bone(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, -1);
		return binds_ptr[p_index].bone;
	}

	inline String get_bind_name(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, String());
		return binds_ptr[p_index].name;
	}

	inline Transform3D get_bind_pose(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, Transform3D());
		return binds_ptr[p_index].pose;
	}

	void clear_binds();

	Skin() = default;
};

typedef int BoneId;

class Skeleton3D;

class SkinReference : public RefCounted
{
	friend class Skeleton3D;

	Skeleton3D* skeleton_node = nullptr;
	RID skeleton;
	Ref<Skin> skin;
	uint32_t bind_count = 0;
	uint64_t skeleton_version = 0;
	Vector<uint32_t> skin_bone_indices;
	uint32_t* skin_bone_indices_ptrs = nullptr;

protected:

public:
	// Public for use as signal callback.
	void _skin_changed();

	RID get_skeleton() const;
	Ref<Skin> get_skin() const;
	~SkinReference();
};

class Skeleton3D : public Node3D
{
#ifdef TOOLS_ENABLED
	bool saving = false;
#endif // TOOLS_ENABLED

#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	bool animate_physical_bones = true;
	Node* simulator = nullptr;
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

public:
	enum ModifierCallbackModeProcess
	{
		MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS,
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE,
		MODIFIER_CALLBACK_MODE_PROCESS_MANUAL,
	};

private:
	friend class SkinReference;

	enum UpdateFlag
	{
		UPDATE_FLAG_NONE = 1,
		UPDATE_FLAG_MODIFIER = 2,
		UPDATE_FLAG_POSE = 4,
	};

	void _update_deferred(UpdateFlag p_update_flag = UPDATE_FLAG_POSE);
	uint8_t update_flags = UPDATE_FLAG_NONE;
	bool updating = false; // Is updating now?
	double update_delta = 0.0;

	struct Bone
	{
		String name;

		int parent = -1;
		Vector<int> child_bones;

		Transform3D rest;
		Transform3D global_rest;

		bool enabled = true;
		bool pose_cache_dirty = true;
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;
		int nested_set_offset = 0; // Offset in nested set of bone hierarchy.
		int nested_set_span = 0;   // Subtree span in nested set of bone hierarchy.

		bool modifier_applied = false;
		Transform3D modifier_pose_cache;

		void make_bone_modified()
		{
			modifier_applied = true;
			modifier_pose_cache = pose_cache;
		}

		void update_pose_cache()
		{
			if (pose_cache_dirty) {
				pose_cache.basis.set_quaternion_scale(pose_rotation, pose_scale);
				pose_cache.origin = pose_position;
				pose_cache_dirty = false;
			}
		}

#ifndef DISABLE_DEPRECATED
		Transform3D pose_global_no_override;
		real_t global_pose_override_amount = 0.0;
		bool global_pose_override_reset = false;
		Transform3D global_pose_override;
#endif // _DISABLE_DEPRECATED
	};

	struct BonePoseBackup
	{
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;

		void save(const Bone& p_bone)
		{
			pose_cache = p_bone.pose_cache;
			pose_position = p_bone.pose_position;
			pose_rotation = p_bone.pose_rotation;
			pose_scale = p_bone.pose_scale;
			global_pose = p_bone.global_pose;
		}

		void restore(Bone& r_bone)
		{
			r_bone.pose_cache = pose_cache;
			r_bone.pose_position = pose_position;
			r_bone.pose_rotation = pose_rotation;
			r_bone.pose_scale = pose_scale;
			r_bone.global_pose = global_pose;
		}
	};

	HashSet<SkinReference*> skin_bindings;
	void _skin_changed();

	mutable LocalVector<Bone> bones;
	mutable bool process_order_dirty = false;

	mutable Vector<int> parentless_bones;
	AHashMap<String, int> name_to_bone_index;

	mutable String concatenated_bone_names;
	void _update_bone_names() const;

	void _make_dirty();
	mutable bool dirty = false;
	mutable bool rest_dirty = false;

	bool show_rest_only = false;
	float motion_scale = 1.0;

	uint64_t version = 1;

	void _update_process_order() const;

	// To process modifiers.
	ModifierCallbackModeProcess modifier_callback_mode_process =
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE;
	bool modifiers_dirty = false;
	void _find_modifiers();
	void _process_modifiers();
	void _process_changed();
	void _make_modifiers_dirty();
	bool modifier_updating = false; // Is modifier updating now?

	// Global bone pose calculation.
	mutable LocalVector<int>
		nested_set_offset_to_bone_index; // Map from Bone::nested_set_offset to bone index.
	mutable LocalVector<bool> bone_global_pose_dirty; // Indexable with Bone::nested_set_offset.
	void _update_bones_nested_set() const;
	int _update_bone_nested_set(int p_bone, int p_offset) const;
	void _make_bone_global_poses_dirty() const;
	void _make_bone_global_pose_subtree_dirty(int p_bone) const;
	void _update_bone_global_pose(int p_bone) const;

#ifndef DISABLE_DEPRECATED
	void _add_bone_bind_compat_88791(const String& p_name);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

protected:
	void _notification(int p_what);
	TypedArray<String> _get_bone_meta_list_bind(int p_bone) const;

	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	enum
	{
		NOTIFICATION_UPDATE_SKELETON = 50
	};

	// Skeleton creation API
	uint64_t get_version() const;
	int add_bone(const String& p_name);
	int find_bone(const String& p_name) const;
	String get_bone_name(int p_bone) const;
	void set_bone_name(int p_bone, const String& p_name);
	String get_concatenated_bone_names() const;

	bool is_bone_parent_of(int p_bone_id, int p_parent_bone_id) const;

	void set_bone_parent(int p_bone, int p_parent);
	int get_bone_parent(int p_bone) const;

	void unparent_bone_and_rest(int p_bone);

	Vector<int> get_bone_children(int p_bone) const;
	Vector<int> get_parentless_bones() const;

	int get_bone_count() const;

	void set_bone_rest(int p_bone, const Transform3D& p_rest);
	Transform3D get_bone_rest(int p_bone) const;
	Transform3D get_bone_global_rest(int p_bone) const;

	void set_bone_enabled(int p_bone, bool p_enabled);
	bool is_bone_enabled(int p_bone) const;

	void set_show_rest_only(bool p_enabled);
	bool is_show_rest_only() const;
	void clear_bones();

	void set_motion_scale(float p_motion_scale);
	float get_motion_scale() const;

	// bone metadata
	void get_bone_meta_list(int p_bone, List<String>* p_list) const;
	bool has_bone_meta(int p_bone, const String& p_key) const;

	// Posing API
	Transform3D get_bone_pose(int p_bone) const;
	Vector3 get_bone_pose_position(int p_bone) const;
	Quaternion get_bone_pose_rotation(int p_bone) const;
	Vector3 get_bone_pose_scale(int p_bone) const;
	void set_bone_pose(int p_bone, const Transform3D& p_pose);
	void set_bone_pose_position(int p_bone, const Vector3& p_position);
	void set_bone_pose_rotation(int p_bone, const Quaternion& p_rotation);
	void set_bone_pose_scale(int p_bone, const Vector3& p_scale);

	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D& p_pose);

	void reset_bone_pose(int p_bone);
	void reset_bone_poses();

	void localize_rests(); // Used for loaders and tools.

	Ref<Skin> create_skin_from_rest_transforms();

	Ref<SkinReference> register_skin(const Ref<Skin>& p_skin);

	void force_update_all_dirty_bones();
	void _force_update_all_dirty_bones() const;
	void force_update_all_bone_transforms();
	void _force_update_all_bone_transforms() const;
	void force_update_bone_children_transforms(int bone_idx);
	void _force_update_bone_children_transforms(int bone_idx) const;
	void force_update_deferred();

	void set_modifier_callback_mode_process(ModifierCallbackModeProcess p_mode);
	ModifierCallbackModeProcess get_modifier_callback_mode_process() const;

	void advance(double p_delta);

#ifndef DISABLE_DEPRECATED
	Transform3D get_bone_global_pose_no_override(int p_bone) const;
	void clear_bones_global_pose_override();
	Transform3D get_bone_global_pose_override(int p_bone) const;
	void set_bone_global_pose_override(
		int p_bone, const Transform3D& p_pose, real_t p_amount, bool p_persistent = false);

	Node* get_simulator();
#ifndef PHYSICS_3D_DISABLED
	void set_animate_physical_bones(bool p_enabled);
	bool get_animate_physical_bones() const;
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String>& p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

public:
	Skeleton3D();
	~Skeleton3D();
};

// --- Folded from scene/3d/skeleton_modifier_3d.h ---
#include "scene/3d/node_3d.h"

class SkeletonModifier3D : public Node3D
{
	void rebind();

public:
	// For the case to indicate bone axis on basis without custom vector.
	enum BoneAxis
	{
		BONE_AXIS_PLUS_X,
		BONE_AXIS_MINUS_X,
		BONE_AXIS_PLUS_Y,
		BONE_AXIS_MINUS_Y,
		BONE_AXIS_PLUS_Z,
		BONE_AXIS_MINUS_Z,
	};

	static String get_hint_bone_axis() { return "+X,-X,+Y,-Y,+Z,-Z"; }

	// For the case to indicate Head-Tail of the bone.
	enum BoneDirection
	{
		BONE_DIRECTION_PLUS_X,
		BONE_DIRECTION_MINUS_X,
		BONE_DIRECTION_PLUS_Y,
		BONE_DIRECTION_MINUS_Y,
		BONE_DIRECTION_PLUS_Z,
		BONE_DIRECTION_MINUS_Z,
		BONE_DIRECTION_FROM_PARENT,
	};

	static String get_hint_bone_direction() { return "+X,-X,+Y,-Y,+Z,-Z,FromParent"; }

	// For the case to define secondary axis of the bone local space.
	enum SecondaryDirection
	{
		SECONDARY_DIRECTION_NONE,
		SECONDARY_DIRECTION_PLUS_X,
		SECONDARY_DIRECTION_MINUS_X,
		SECONDARY_DIRECTION_PLUS_Y,
		SECONDARY_DIRECTION_MINUS_Y,
		SECONDARY_DIRECTION_PLUS_Z,
		SECONDARY_DIRECTION_MINUS_Z,
		SECONDARY_DIRECTION_CUSTOM,
	};

	static String get_hint_secondary_direction() { return "None,+X,-X,+Y,-Y,+Z,-Z,Custom"; }

	// For the case to define rotation direction without identification plus/minus.
	enum RotationAxis
	{
		ROTATION_AXIS_X,
		ROTATION_AXIS_Y,
		ROTATION_AXIS_Z,
		ROTATION_AXIS_ALL,
		ROTATION_AXIS_CUSTOM,
	};

	static String get_hint_rotation_axis() { return "X,Y,Z,All,Custom"; }

protected:
	bool active = true;
	real_t influence = 1.0;

	void _update_skeleton_path();

	virtual void _skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new);
	virtual void _validate_bone_names();

	virtual void _set_active(bool p_active);
	virtual void _process_modification(double p_delta);
	bool should_check_node_path();

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton() const;

	virtual bool has_process() const
	{
		return false;
	} // Return true if modifier needs to modify bone pose without external animation such as
	  // physics, jiggle and etc.

	bool is_active() const;
	void set_influence(real_t p_influence);
	real_t get_influence() const;

	void process_modification(double p_delta);

	// Utility APIs.
	static Vector3 get_vector_from_bone_axis(BoneAxis p_axis);
	static Vector3 get_vector_from_axis(Vector3::Axis p_axis);
	static Vector3::Axis get_axis_from_bone_axis(BoneAxis p_axis);

	// 3D math.
	static Vector3 limit_length(
		const Vector3& p_origin, const Vector3& p_destination, float p_length);
	static Quaternion get_local_pose_rotation(
		Skeleton3D* p_skeleton, int p_bone, const Quaternion& p_global_pose_rotation);
	static Quaternion get_from_to_rotation(
		const Vector3& p_from, const Vector3& p_to, const Quaternion& p_prev_rot);
	static Quaternion get_from_to_rotation_by_axis(
		const Vector3& p_from, const Vector3& p_to, const Vector3& p_axis);
	static Quaternion get_swing(const Quaternion& p_rotation, const Vector3& p_axis);
	static Vector3 snap_vector_to_plane(const Vector3& p_plane_normal, const Vector3& p_vector);
	static double symmetrize_angle(double p_angle);
	static double get_roll_angle(const Quaternion& p_rotation, const Vector3& p_roll_axis);
	static Vector3 get_projected_normal(
		const Vector3& p_a, const Vector3& p_b, const Vector3& p_point);

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const { return false; }
#endif

	SkeletonModifier3D();
};

// --- Folded from scene/3d/bone_constraint_3d.h ---

#include "scene/resources/curve.h"

class BoneConstraint3D : public SkeletonModifier3D {
public:
	enum ConstraintType {
		CONSTRAINT_AIM,
		CONSTRAINT_CONVERT_TRANSFORM,
		CONSTRAINT_COPY_TRANSFORM,
		CONSTRAINT_BONE_TARGET,
		CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		CONSTRAINT_TWIST_DISPERSER,

		TYPE_AIM = CONSTRAINT_AIM,
		TYPE_CONVERT_TRANSFORM = CONSTRAINT_CONVERT_TRANSFORM,
		TYPE_COPY_TRANSFORM = CONSTRAINT_COPY_TRANSFORM,
		TYPE_BONE_TARGET = CONSTRAINT_BONE_TARGET,
		TYPE_LIMIT_ANGULAR_VELOCITY = CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		TYPE_TWIST_DISPERSER = CONSTRAINT_TWIST_DISPERSER,
	};

	enum ReferenceType {
		REFERENCE_TYPE_BONE,
		REFERENCE_TYPE_NODE,
	};

	enum TransformMode {
		TRANSFORM_MODE_POSITION,
		TRANSFORM_MODE_ROTATION,
		TRANSFORM_MODE_SCALE,
	};

	enum TransformFlag {
		TRANSFORM_FLAG_POSITION = 1,
		TRANSFORM_FLAG_ROTATION = 2,
		TRANSFORM_FLAG_SCALE = 4,
		TRANSFORM_FLAG_ALL = TRANSFORM_FLAG_POSITION | TRANSFORM_FLAG_ROTATION | TRANSFORM_FLAG_SCALE,
	};

	enum AxisFlag {
		AXIS_FLAG_X = 1,
		AXIS_FLAG_Y = 2,
		AXIS_FLAG_Z = 4,
		AXIS_FLAG_ALL = AXIS_FLAG_X | AXIS_FLAG_Y | AXIS_FLAG_Z,
	};

	enum DisperseMode {
		DISPERSE_MODE_EVEN,
		DISPERSE_MODE_WEIGHTED,
		DISPERSE_MODE_CUSTOM,
	};

	struct BoneJoint {
		String name;
		int bone = -1;
	};

	struct DisperseJointSetting {
		BoneJoint joint;
		double custom_amount = 1.0;
		double amount = 1.0;
		Vector3 axis;
	};

	typedef Pair<int, Quaternion> BoneRot;

	struct ConstraintSetting {
		float amount = 1.0f;

		String apply_bone_name;
		int apply_bone = -1;

		ReferenceType reference_type = REFERENCE_TYPE_BONE;
		String reference_bone_name;
		int reference_bone = -1;
		NodePath reference_node;

		bool relative = true;
		bool additive = false;

		// Aim settings.
		BoneAxis forward_axis = BONE_AXIS_PLUS_Y;
		bool use_euler = false;
		Vector3::Axis primary_rotation_axis = Vector3::AXIS_X;
		bool use_secondary_rotation = true;

		// Convert Transform settings.
		TransformMode apply_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis apply_axis = Vector3::AXIS_X;
		float apply_range_min = 0.0f;
		float apply_range_max = 0.0f;
		TransformMode reference_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis reference_axis = Vector3::AXIS_X;
		float reference_range_min = 0.0f;
		float reference_range_max = 0.0f;

		// Copy Transform settings.
		uint32_t copy_flags = TRANSFORM_FLAG_ALL;
		uint32_t axis_flags = AXIS_FLAG_ALL;
		uint32_t invert_flags = 0;

		// Chain & Twist Disperser settings (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
		BoneJoint root_bone;
		BoneJoint end_bone;
		bool joints_dirty = false;
		DisperseMode disperse_mode = DISPERSE_MODE_EVEN;
		bool twist_from_rest = true;
		Quaternion twist_from;
		LocalVector<DisperseJointSetting> joints;
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float weight_position = 0.5f;
		Ref<Curve> damping_curve;

		bool is_relative_effective(ConstraintType p_type) const {
			if (p_type != CONSTRAINT_AIM && reference_type == REFERENCE_TYPE_NODE) {
				return false;
			}
			return relative;
		}
	};

protected:
	ConstraintType constraint_type = CONSTRAINT_COPY_TRANSFORM;
	LocalVector<ConstraintSetting> settings;

	// BoneTarget state.
	String target_bone_name;
	int target_bone = -1;

	// LimitAngularVelocity state.
	bool exclude = false;
	double max_angular_velocity = Math::TAU;
	LocalVector<BoneJoint> velocity_joints;
	LocalVector<BoneRot> velocity_bones;
	bool velocity_joints_dirty = false;
	bool velocity_init_needed = true;

	// TwistDisperser state.
	bool mutable_bone_axes = true;

	void _notification(int p_what);
	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void _set_active(bool p_active) override;
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount);
	void _process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_bone_target(Skeleton3D *p_skeleton);
	void _process_limit_angular_velocity(Skeleton3D *p_skeleton, double p_delta);
	void _process_twist_disperser(Skeleton3D *p_skeleton);

	void _make_velocity_joints_dirty();
	void _update_velocity_joints();
	bool _is_velocity_joint_contained(int p_bone) const;

	void _make_all_twist_joints_dirty();
	void _make_twist_joints_dirty(int p_index);
	void _update_twist_joints(int p_index);
	void _update_twist_curve(int p_index);

public:
	BoneConstraint3D(ConstraintType p_type = CONSTRAINT_COPY_TRANSFORM);
	~BoneConstraint3D();

	void set_constraint_type(ConstraintType p_type);
	ConstraintType get_constraint_type() const { return constraint_type; }

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const override { return constraint_type == CONSTRAINT_BONE_TARGET; }
#endif // TOOLS_ENABLED

	virtual bool has_process() const override { return constraint_type == CONSTRAINT_LIMIT_ANGULAR_VELOCITY; }

	void set_setting_count(int p_count);
	int get_setting_count() const;
	void clear_settings();

	// Common setting properties.
	void set_amount(int p_index, float p_amount);
	float get_amount(int p_index) const;

	void set_apply_bone_name(int p_index, const String &p_name);
	String get_apply_bone_name(int p_index) const;
	void set_apply_bone(int p_index, int p_bone);
	int get_apply_bone(int p_index) const;

	void set_reference_type(int p_index, ReferenceType p_type);
	ReferenceType get_reference_type(int p_index) const;

	void set_reference_bone_name(int p_index, const String &p_name);
	String get_reference_bone_name(int p_index) const;
	void set_reference_bone(int p_index, int p_bone);
	int get_reference_bone(int p_index) const;

	void set_reference_node(int p_index, const NodePath &p_node);
	NodePath get_reference_node(int p_index) const;

	void set_relative(int p_index, bool p_enabled);
	bool is_relative(int p_index) const;
	void set_additive(int p_index, bool p_enabled);
	bool is_additive(int p_index) const;

	// Aim properties.
	void set_forward_axis(int p_index, BoneAxis p_axis);
	BoneAxis get_forward_axis(int p_index) const;
	void set_use_euler(int p_index, bool p_enabled);
	bool is_using_euler(int p_index) const;
	void set_primary_rotation_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_primary_rotation_axis(int p_index) const;
	void set_use_secondary_rotation(int p_index, bool p_enabled);
	bool is_using_secondary_rotation(int p_index) const;

	// Convert Transform properties.
	void set_apply_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_apply_transform_mode(int p_index) const;
	void set_apply_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_apply_axis(int p_index) const;
	void set_apply_range_min(int p_index, float p_range_min);
	float get_apply_range_min(int p_index) const;
	void set_apply_range_max(int p_index, float p_range_max);
	float get_apply_range_max(int p_index) const;

	void set_reference_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_reference_transform_mode(int p_index) const;
	void set_reference_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_reference_axis(int p_index) const;
	void set_reference_range_min(int p_index, float p_range_min);
	float get_reference_range_min(int p_index) const;
	void set_reference_range_max(int p_index, float p_range_max);
	float get_reference_range_max(int p_index) const;

	// Copy Transform properties.
	void set_copy_flags(int p_index, uint32_t p_copy_flags);
	uint32_t get_copy_flags(int p_index) const;
	void set_copy_position(int p_index, bool p_enabled);
	bool is_position_copying(int p_index) const;
	void set_copy_rotation(int p_index, bool p_enabled);
	bool is_rotation_copying(int p_index) const;
	void set_copy_scale(int p_index, bool p_enabled);
	bool is_scale_copying(int p_index) const;

	void set_axis_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_axis_flags(int p_index) const;
	void set_axis_x_enabled(int p_index, bool p_enabled);
	bool is_axis_x_enabled(int p_index) const;
	void set_axis_y_enabled(int p_index, bool p_enabled);
	bool is_axis_y_enabled(int p_index) const;
	void set_axis_z_enabled(int p_index, bool p_enabled);
	bool is_axis_z_enabled(int p_index) const;

	void set_invert_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_invert_flags(int p_index) const;
	void set_axis_x_inverted(int p_index, bool p_enabled);
	bool is_axis_x_inverted(int p_index) const;
	void set_axis_y_inverted(int p_index, bool p_enabled);
	bool is_axis_y_inverted(int p_index) const;
	void set_axis_z_inverted(int p_index, bool p_enabled);
	bool is_axis_z_inverted(int p_index) const;

	// BoneTarget properties (CONSTRAINT_BONE_TARGET / TYPE_BONE_TARGET).
	void set_bone_name(const String &p_bone_name);
	String get_bone_name() const;
	void set_bone(int p_bone);
	int get_bone() const;

	// Chain root/end properties (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
	void set_root_bone_name(int p_index, const String &p_bone_name);
	String get_root_bone_name(int p_index) const;
	void set_root_bone(int p_index, int p_bone);
	int get_root_bone(int p_index) const;

	void set_end_bone_name(int p_index, const String &p_bone_name);
	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	// LimitAngularVelocity properties (CONSTRAINT_LIMIT_ANGULAR_VELOCITY / TYPE_LIMIT_ANGULAR_VELOCITY).
	void set_chain_count(int p_count);
	int get_chain_count() const;
	void clear_chains();
	String get_joint_bone_name(int p_index) const;
	int get_joint_bone(int p_index) const;
	int get_velocity_joint_count() const;
	void set_max_angular_velocity(double p_angular_velocity);
	double get_max_angular_velocity() const;
	void set_exclude(bool p_exclude);
	bool is_exclude() const;
	void reset();

	// TwistDisperser properties (CONSTRAINT_TWIST_DISPERSER / TYPE_TWIST_DISPERSER).
	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;
	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_bone_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_twist_from_rest(int p_index, bool p_enabled);
	bool is_twist_from_rest(int p_index) const;
	void set_twist_from(int p_index, const Quaternion &p_from);
	Quaternion get_twist_from(int p_index) const;
	void set_disperse_mode(int p_index, DisperseMode p_disperse_mode);
	DisperseMode get_disperse_mode(int p_index) const;
	void set_weight_position(int p_index, float p_position);
	float get_weight_position(int p_index) const;
	void set_damping_curve(int p_index, const Ref<Curve> &p_damping_curve);
	Ref<Curve> get_damping_curve(int p_index) const;
	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;
	void set_joint_twist_amount(int p_index, int p_joint, float p_amount);
	float get_joint_twist_amount(int p_index, int p_joint) const;
	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;
};

// --- End scene/3d/bone_constraint_3d.h ---

// --- Folded from scene/resources/3d/joint_limitation_3d.h ---
#include "core/io/resource.h"

#ifdef TOOLS_ENABLED
#include "scene/resources/mesh.h"
#endif // TOOLS_ENABLED

class JointLimitation3D : public Resource
{
	real_t angle = Math::TAU * 0.25;

	Vector3 _solve(const Vector3& p_direction) const;

public:
	void set_angle(real_t p_angle);
	real_t get_angle() const;

	Quaternion make_space(const Vector3& p_local_forward_vector,
		const Vector3& p_local_right_vector, const Quaternion& p_rotation_offset) const;

	Vector3 solve(const Vector3& p_local_forward_vector, const Vector3& p_local_right_vector,
		const Quaternion& p_rotation_offset, const Vector3& p_local_current_vector) const;

#ifdef TOOLS_ENABLED
	void draw_shape(Ref<SurfaceTool>& p_surface_tool, const Transform3D& p_transform,
		float p_bone_length, const Color& p_color) const;
#endif // TOOLS_ENABLED
};

// --- End scene/resources/3d/joint_limitation_3d.h ---

// --- Folded from scene/3d/physics/collision_object_3d.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

// Plain non-virtual composed component for 3D collision state (shared by PhysicsBody3D and Area3D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape3D/CollisionPolygon3D owners.
struct CollisionState3D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform3D local_xform;
		Ref<Shape3D> shape;
		bool disabled = false;
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform3D xform;
		Node *owner_node = nullptr;

		struct ShapeBase {
			RID debug_shape;
			Ref<Shape3D> shape;
			int index = 0;
		};

		Vector<ShapeBase> shapes;
		bool disabled = false;
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS3DE::BodyMode body_mode = PS3DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;
	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	int debug_shapes_count = 0;
	Transform3D debug_shape_old_transform;

	void init(bool p_area, PS3DE::BodyMode p_mode = PS3DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node3D *p_owner, int p_what);
	void on_transform_changed(const Node3D *p_owner);
	void clear_debug_shapes();

	void apply_disabled(Node3D *p_owner);
	void apply_enabled(Node3D *p_owner);
	void update_pickable(const Node3D *p_owner);
	void set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node3D *p_owner, DisableMode p_mode);
	void set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable);

	// Inline shape slots (no child CollisionShape3D node required).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform);
	Transform3D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape3D / CollisionPolygon3D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform);
	Transform3D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape3D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};

// --- Folded from scene/3d/physics/kinematic_collision_3d.h ---
#include "servers/physics_3d/physics_server_3d.h"

class KinematicCollision3D : public RefCounted
{
	friend class PhysicsBody3D;
	PS3DT::MotionResult result;

protected:

public:
	Vector3 get_travel() const;
	Vector3 get_remainder() const;
	int get_collision_count() const;
	real_t get_depth() const;
	Vector3 get_position(int p_collision_index = 0) const;
	Vector3 get_normal(int p_collision_index = 0) const;
	real_t get_angle(
		int p_collision_index = 0, const Vector3& p_up_direction = Vector3(0.0, 1.0, 0.0)) const;
	RID get_collider_rid(int p_collision_index = 0) const;
	int get_collider_shape_index(int p_collision_index = 0) const;
	Vector3 get_collider_velocity(int p_collision_index = 0) const;
};



// --- End scene/3d/physics/kinematic_collision_3d.h ---

// --- Folded from scene/3d/physics/physics_body_3d.h ---


#include "scene/resources/material.h"
#include "servers/physics_3d/physics_server_3d.h"

class PhysicsBody3D;
class PhysicalBoneSimulator3D;

class VehicleWheel3D : public Node3D {
	friend class PhysicsBody3D;

	struct WheelXform {
		Vector3 up;
		Vector3 right;
		Vector3 origin;
		real_t steering = 0.0f;
	};

	struct FTIData {
		real_t curr_rotation = 0.0f;
		real_t curr_rotation_delta = 0.0f;
		WheelXform curr;
		WheelXform prev;
		bool unset = true;
		bool reset_queued = false;

		void rotate(real_t p_delta) {
			curr_rotation += p_delta;
			curr_rotation_delta = p_delta;
			double wrapped = Math::fmod(curr_rotation + Math::PI, Math::TAU);
			if (wrapped < 0) {
				wrapped += Math::TAU;
			}
			curr_rotation = wrapped - Math::PI;
		}

		void pump() {
			prev = curr;
			curr_rotation_delta = 0.0f;
		}

		void update_world_xform(Transform3D &r_xform, real_t p_interpolation_fraction);
	} fti_data;

	Transform3D m_worldTransform;
	Transform3D local_xform;
	bool engine_traction = false;
	bool steers = false;

	Vector3 m_chassisConnectionPointCS;
	Vector3 m_wheelDirectionCS;
	Vector3 m_wheelAxleCS;

	real_t m_suspensionRestLength = 0.15f;
	real_t m_maxSuspensionTravel = 0.2f;
	real_t m_wheelRadius = 0.5f;

	real_t m_suspensionStiffness = 5.88f;
	real_t m_wheelsDampingCompression = 0.83f;
	real_t m_wheelsDampingRelaxation = 0.88f;
	real_t m_frictionSlip = 10.5f;
	real_t m_maxSuspensionForce = 6000.0f;
	bool m_bIsFrontWheel = false;

	PhysicsBody3D *body = nullptr;

	real_t m_steering = 0.0f;
	real_t m_rotation = 0.0f;
	real_t m_deltaRotation = 0.0f;
	real_t m_rpm = 0.0f;
	real_t m_rollInfluence = 0.1f;
	real_t m_engineForce = 0.0f;
	real_t m_brake = 0.0f;

	real_t m_clippedInvContactDotSuspension = 1.0f;
	real_t m_suspensionRelativeVelocity = 0.0f;
	real_t m_wheelsSuspensionForce = 0.0f;
	real_t m_skidInfo = 0.0f;

	struct RaycastInfo {
		Vector3 m_contactNormalWS;
		Vector3 m_contactPointWS;
		real_t m_suspensionLength = 0.0f;
		Vector3 m_hardPointWS;
		Vector3 m_wheelDirectionWS;
		Vector3 m_wheelAxleWS;
		bool m_isInContact = false;
		PhysicsBody3D *m_groundObject = nullptr;
	} m_raycastInfo;

	void _update(PhysicsDirectBodyState3D *s);

protected:
	void _notification(int p_what);

public:
	void set_radius(real_t p_radius) { m_wheelRadius = p_radius; }
	real_t get_radius() const { return m_wheelRadius; }

	void set_suspension_rest_length(real_t p_length) { m_suspensionRestLength = p_length; }
	real_t get_suspension_rest_length() const { return m_suspensionRestLength; }

	void set_suspension_travel(real_t p_length) { m_maxSuspensionTravel = p_length; }
	real_t get_suspension_travel() const { return m_maxSuspensionTravel; }

	void set_suspension_stiffness(real_t p_value) { m_suspensionStiffness = p_value; }
	real_t get_suspension_stiffness() const { return m_suspensionStiffness; }

	void set_suspension_max_force(real_t p_value) { m_maxSuspensionForce = p_value; }
	real_t get_suspension_max_force() const { return m_maxSuspensionForce; }

	void set_damping_compression(real_t p_value) { m_wheelsDampingCompression = p_value; }
	real_t get_damping_compression() const { return m_wheelsDampingCompression; }

	void set_damping_relaxation(real_t p_value) { m_wheelsDampingRelaxation = p_value; }
	real_t get_damping_relaxation() const { return m_wheelsDampingRelaxation; }

	void set_friction_slip(real_t p_value) { m_frictionSlip = p_value; }
	real_t get_friction_slip() const { return m_frictionSlip; }

	void set_use_as_traction(bool p_enable) { engine_traction = p_enable; }
	bool is_used_as_traction() const { return engine_traction; }

	void set_use_as_steering(bool p_enabled) { steers = p_enabled; }
	bool is_used_as_steering() const { return steers; }

	bool is_in_contact() const { return m_raycastInfo.m_isInContact; }
	Vector3 get_contact_point() const { return m_raycastInfo.m_contactPointWS; }
	Vector3 get_contact_normal() const { return m_raycastInfo.m_contactNormalWS; }
	Node3D *get_contact_body() const;

	void set_roll_influence(real_t p_value) { m_rollInfluence = p_value; }
	real_t get_roll_influence() const { return m_rollInfluence; }

	real_t get_skidinfo() const { return m_skidInfo; }
	real_t get_rpm() const { return m_rpm; }

	void set_engine_force(real_t p_engine_force) { m_engineForce = p_engine_force; }
	real_t get_engine_force() const { return m_engineForce; }

	void set_brake(real_t p_brake) { m_brake = p_brake; }
	real_t get_brake() const { return m_brake; }

	void set_steering(real_t p_steering) { m_steering = p_steering; }
	real_t get_steering() const { return m_steering; }

	virtual PackedStringArray get_configuration_warnings() const override;

	VehicleWheel3D() = default;
};

class PhysicsBody3D : public Node3D {
	friend class VehicleWheel3D;

public:
	enum BodyMode {
		MODE_STATIC,
		MODE_ANIMATABLE,
		MODE_CHARACTER,
		MODE_RIGID,
		MODE_VEHICLE,
		MODE_PHYSICAL_BONE,
	};

	enum FreezeMode {
		FREEZE_MODE_STATIC,
		FREEZE_MODE_KINEMATIC,
	};

	enum CenterOfMassMode {
		CENTER_OF_MASS_MODE_AUTO,
		CENTER_OF_MASS_MODE_CUSTOM,
	};

	enum DampMode {
		DAMP_MODE_COMBINE,
		DAMP_MODE_REPLACE,
	};

	enum MotionMode {
		MOTION_MODE_GROUNDED,
		MOTION_MODE_FLOATING,
	};

	enum PlatformOnLeave {
		PLATFORM_ON_LEAVE_ADD_VELOCITY,
		PLATFORM_ON_LEAVE_ADD_UPWARD_VELOCITY,
		PLATFORM_ON_LEAVE_DO_NOTHING,
	};

	enum BoneJointType {
		BONE_JOINT_NONE,
		BONE_JOINT_PIN,
		BONE_JOINT_CONE,
		BONE_JOINT_HINGE,
		BONE_JOINT_SLIDER,
		BONE_JOINT_6DOF,
	};

	using DisableMode = CollisionState3D::DisableMode;

	// Plain composed character mover state (for MODE_CHARACTER).
	struct CharacterMover3D {
		struct FloorFlags {
			bool floor = false;
			bool wall = false;
			bool ceiling = false;

			FloorFlags() = default;
			FloorFlags(bool p_floor, bool p_wall, bool p_ceiling) :
					floor(p_floor), wall(p_wall), ceiling(p_ceiling) {}
		};

		real_t margin = 0.001f;
		MotionMode motion_mode = MOTION_MODE_GROUNDED;
		PlatformOnLeave platform_on_leave = PLATFORM_ON_LEAVE_ADD_VELOCITY;

		FloorFlags flags;
		bool floor_constant_speed = false;
		bool floor_stop_on_slope = true;
		bool floor_block_on_wall = true;
		bool slide_on_ceiling = true;
		int max_slides = 6;
		int platform_layer = 0;
		RID platform_rid;
		uint32_t platform_floor_layers = UINT32_MAX;
		uint32_t platform_wall_layers = 0;
		real_t floor_snap_length = 0.1f;
		real_t floor_max_angle = Math::deg_to_rad((real_t)45.0);
		real_t wall_min_slide_angle = Math::deg_to_rad((real_t)15.0);
		Vector3 up_direction = Vector3(0.0f, 1.0f, 0.0f);
		Vector3 velocity;
		Vector3 floor_normal;
		Vector3 wall_normal;
		Vector3 ceiling_normal;
		Vector3 last_motion;
		Vector3 platform_velocity;
		Vector3 platform_angular_velocity;
		Vector3 platform_ceiling_velocity;
		Vector3 previous_position;
		Vector3 real_velocity;

		Vector<PS3DT::MotionResult> motion_results;
		Vector<Ref<KinematicCollision3D>> slide_colliders;
	};

	// Plain composed vehicle rig state (for MODE_VEHICLE).
	struct VehicleRig3D {
		real_t engine_force = 0.0f;
		real_t brake = 0.0f;
		real_t pitch_control = 0.0f;
		real_t steering = 0.0f;
		real_t speed_km_hour = 0.0f;
		HashSet<RID> exclude;
		Vector<Vector3> forward_ws;
		Vector<Vector3> axle;
		Vector<real_t> forward_impulse;
		Vector<real_t> side_impulse;
		Vector<VehicleWheel3D *> wheels;
	};

	// Plain value-type joint configuration & bone attachment state (for MODE_PHYSICAL_BONE).
	struct PhysicalBoneState3D {
		struct SixDOFAxisParams {
			bool linear_limit_enabled = true;
			real_t linear_limit_upper = 0.0f;
			real_t linear_limit_lower = 0.0f;
			real_t linear_limit_softness = 0.7f;
			real_t linear_restitution = 0.5f;
			real_t linear_damping = 1.0f;
			bool linear_spring_enabled = false;
			real_t linear_spring_stiffness = 0.0f;
			real_t linear_spring_damping = 0.0f;
			real_t linear_equilibrium_point = 0.0f;
			bool angular_limit_enabled = true;
			real_t angular_limit_upper = 0.0f;
			real_t angular_limit_lower = 0.0f;
			real_t angular_limit_softness = 0.5f;
			real_t angular_restitution = 0.0f;
			real_t angular_damping = 1.0f;
			real_t erp = 0.5f;
			bool angular_spring_enabled = false;
			real_t angular_spring_stiffness = 0.0f;
			real_t angular_spring_damping = 0.0f;
			real_t angular_equilibrium_point = 0.0f;
		};

		struct JointParams {
			// Pin / Cone / Hinge shared parameters.
			real_t bias = 0.3f;
			real_t damping = 1.0f;
			real_t impulse_clamp = 0.0f;
			real_t swing_span = Math::PI * 0.25f;
			real_t twist_span = Math::PI;
			real_t softness = 0.8f;
			real_t relaxation = 1.0f;

			// Hinge / Slider limits.
			bool angular_limit_enabled = false;
			real_t angular_limit_upper = Math::PI * 0.5f;
			real_t angular_limit_lower = -Math::PI * 0.5f;
			real_t angular_limit_bias = 0.3f;
			real_t angular_limit_softness = 0.9f;
			real_t angular_limit_relaxation = 1.0f;
			real_t angular_limit_restitution = 0.7f;
			real_t angular_limit_damping = 1.0f;

			real_t linear_limit_upper = 1.0f;
			real_t linear_limit_lower = -1.0f;
			real_t linear_limit_softness = 1.0f;
			real_t linear_limit_restitution = 0.7f;
			real_t linear_limit_damping = 1.0f;

			SixDOFAxisParams six_dof_axes[3];
		};

		BoneJointType joint_type = BONE_JOINT_NONE;
		JointParams joint_params;
		Transform3D joint_offset;
		RID joint;
		Transform3D body_offset;
		Transform3D body_offset_inverse;
		bool simulate_physics = false;
		bool internal_simulate_physics = false;
		int bone_id = -1;
		String bone_name;
		real_t bounce = 0.0f;
		real_t friction = 1.0f;
#ifdef TOOLS_ENABLED
		bool gizmo_move_joint = false;
#endif
	};

private:
	BodyMode mode = MODE_STATIC;
	CollisionState3D collision_state;
	Ref<KinematicCollision3D> motion_cache;
	uint16_t locked_axis = 0;

	// Static / Animatable properties.
	Vector3 constant_linear_velocity;
	Vector3 constant_angular_velocity;
	bool sync_to_physics = true;
	Transform3D last_valid_transform;

	// Rigid / Vehicle / PhysicalBone properties.
	Ref<PhysicsMaterial> physics_material_override;
	bool can_sleep = true;
	bool lock_rotation = false;
	bool freeze = false;
	FreezeMode freeze_mode = FREEZE_MODE_STATIC;

	real_t mass = 1.0f;
	Vector3 inertia;
	CenterOfMassMode center_of_mass_mode = CENTER_OF_MASS_MODE_AUTO;
	Vector3 center_of_mass;
	Basis inverse_inertia_tensor;
	real_t gravity_scale = 1.0f;

	DampMode linear_damp_mode = DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = DAMP_MODE_COMBINE;
	real_t linear_damp = 0.0f;
	real_t angular_damp = 0.0f;

	Vector3 linear_velocity;
	Vector3 angular_velocity;
	bool sleeping = false;
	bool ccd = false;
	int max_contacts_reported = 0;
	int contact_count = 0;
	bool custom_integrator = false;
	bool contact_monitor = false;

	CharacterMover3D character;
	VehicleRig3D vehicle;
	PhysicalBoneState3D bone_state;

	void _apply_server_body_mode();
	void _update_kinematic_motion();
	void _body_state_changed(PhysicsDirectBodyState3D *p_state);

	// Character helpers.
	void _set_collision_direction(const PS3DT::MotionResult &p_result, CharacterMover3D::FloorFlags &r_state, CharacterMover3D::FloorFlags p_apply_state = CharacterMover3D::FloorFlags(true, true, true));
	bool _on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up);
	void _snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up);

	// Vehicle helpers.
	struct VehicleWheelContactPoint {
		PhysicsDirectBodyState3D *m_s = nullptr;
		PhysicsBody3D *m_body1 = nullptr;
		Vector3 m_frictionPositionWorld;
		Vector3 m_frictionDirectionWorld;
		real_t m_jacDiagABInv = 0.0f;
		real_t m_maxImpulse = 0.0f;

		VehicleWheelContactPoint(PhysicsDirectBodyState3D *s, PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld, real_t maxImpulse);
	};

	void _resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1, PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, real_t p_rollInfluence);
	real_t _calc_rolling_friction(VehicleWheelContactPoint &contactPoint);
	void _update_vehicle_friction(PhysicsDirectBodyState3D *s);
	void _update_vehicle_suspension(PhysicsDirectBodyState3D *s);
	real_t _vehicle_ray_cast(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s);
	void _update_wheel(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_vehicle_process_mode();
	void _step_vehicle(PhysicsDirectBodyState3D *p_state);

	// Physical bone helpers.
	void _start_bone_physics_simulation();
	void _stop_bone_physics_simulation();

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;
	virtual void fti_pump_xform() override;

public:
	PhysicsBody3D(BodyMode p_mode = MODE_STATIC);
	~PhysicsBody3D();

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const { return mode; }

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	// Collision state forwarding.
	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_collision_priority(real_t p_priority) { collision_state.set_collision_priority(p_priority); }
	real_t get_collision_priority() const { return collision_state.collision_priority; }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape slots (direct collision shapes without child CollisionShape3D).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform) { collision_state.set_inline_shape_transform(p_index, p_xform); }
	Transform3D get_inline_shape_transform(int p_index) const { return collision_state.get_inline_shape_transform(p_index); }
	void set_inline_shape_disabled(int p_index, bool p_disabled) { collision_state.set_inline_shape_disabled(p_index, p_disabled); }
	bool is_inline_shape_disabled(int p_index) const { return collision_state.is_inline_shape_disabled(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	// Common PhysicsBody3D motion, locks & exceptions.
	bool move_and_collide(const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult &r_result, bool p_test_only = false, bool p_cancel_sliding = true);
	bool test_move(const Transform3D &p_from, const Vector3 &p_motion, const Ref<KinematicCollision3D> &r_collision = Ref<KinematicCollision3D>(), real_t p_margin = 0.001f, bool p_recovery_as_collision = false, int p_max_collisions = 1);
	Vector3 get_gravity() const;

	void set_axis_lock(PS3DE::BodyAxis p_axis, bool p_lock);
	bool get_axis_lock(PS3DE::BodyAxis p_axis) const;

	void add_collision_exception_with(Node *rp_node);
	void remove_collision_exception_with(Node *rp_node);

	// Static / Animatable API.
	void set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override);
	Ref<PhysicsMaterial> get_physics_material_override() const { return physics_material_override; }
	void set_constant_linear_velocity(const Vector3 &p_vel) { constant_linear_velocity = p_vel; }
	Vector3 get_constant_linear_velocity() const { return constant_linear_velocity; }
	void set_constant_angular_velocity(const Vector3 &p_vel) { constant_angular_velocity = p_vel; }
	Vector3 get_constant_angular_velocity() const { return constant_angular_velocity; }
	void set_sync_to_physics(bool p_enable);
	bool is_sync_to_physics_enabled() const { return sync_to_physics; }

	// Rigid / Dynamic API.
	void set_lock_rotation_enabled(bool p_lock_rotation);
	bool is_lock_rotation_enabled() const { return lock_rotation; }
	void set_freeze_enabled(bool p_freeze);
	bool is_freeze_enabled() const { return freeze; }
	void set_freeze_mode(FreezeMode p_freeze_mode);
	FreezeMode get_freeze_mode() const { return freeze_mode; }

	void set_mass(real_t p_mass);
	real_t get_mass() const { return mass; }
	real_t get_inverse_mass() const;

	void set_inertia(const Vector3 &p_inertia) { inertia = p_inertia; }
	const Vector3 &get_inertia() const { return inertia; }
	Basis get_inverse_inertia_tensor() const { return inverse_inertia_tensor; }

	void set_center_of_mass_mode(CenterOfMassMode p_mode) { center_of_mass_mode = p_mode; }
	CenterOfMassMode get_center_of_mass_mode() const { return center_of_mass_mode; }
	void set_center_of_mass(const Vector3 &p_center_of_mass) { center_of_mass = p_center_of_mass; }
	const Vector3 &get_center_of_mass() const { return center_of_mass; }

	void set_linear_velocity(const Vector3 &p_velocity) { linear_velocity = p_velocity; }
	Vector3 get_linear_velocity() const;
	void set_axis_velocity(const Vector3 &p_axis);
	void set_angular_velocity(const Vector3 &p_velocity) { angular_velocity = p_velocity; }
	Vector3 get_angular_velocity() const;

	void set_gravity_scale(real_t p_gravity_scale) { gravity_scale = p_gravity_scale; }
	real_t get_gravity_scale() const { return gravity_scale; }

	void set_linear_damp_mode(DampMode p_mode) { linear_damp_mode = p_mode; }
	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode) { angular_damp_mode = p_mode; }
	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_linear_damp(real_t p_linear_damp) { linear_damp = p_linear_damp; }
	real_t get_linear_damp() const { return linear_damp; }
	void set_angular_damp(real_t p_angular_damp) { angular_damp = p_angular_damp; }
	real_t get_angular_damp() const { return angular_damp; }

	void set_use_custom_integrator(bool p_enable);
	bool is_using_custom_integrator() const { return custom_integrator; }
	void set_sleeping(bool p_sleeping) { sleeping = p_sleeping; }
	bool is_sleeping() const { return sleeping; }
	void set_can_sleep(bool p_active) { can_sleep = p_active; }
	bool is_able_to_sleep() const { return can_sleep; }
	void set_contact_monitor(bool p_enabled) { contact_monitor = p_enabled; }
	bool is_contact_monitor_enabled() const { return contact_monitor; }
	void set_max_contacts_reported(int p_amount);
	int get_max_contacts_reported() const { return max_contacts_reported; }
	int get_contact_count() const { return contact_count; }
	void set_use_continuous_collision_detection(bool p_enable);
	bool is_using_continuous_collision_detection() const { return ccd; }

	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3());
	void apply_torque_impulse(const Vector3 &p_impulse);
	void apply_central_force(const Vector3 &p_force);
	void apply_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void apply_torque(const Vector3 &p_torque);
	void add_constant_central_force(const Vector3 &p_force);
	void add_constant_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void add_constant_torque(const Vector3 &p_torque);
	void set_constant_force(const Vector3 &p_force);
	Vector3 get_constant_force() const;
	void set_constant_torque(const Vector3 &p_torque);
	Vector3 get_constant_torque() const;

	// CharacterMover3D API.
	CharacterMover3D &get_character_mover() { return character; }
	const CharacterMover3D &get_character_mover() const { return character; }
	bool move_and_slide();
	void apply_floor_snap();
	const Vector3 &get_velocity() const { return character.velocity; }
	void set_velocity(const Vector3 &p_velocity) { character.velocity = p_velocity; }
	const Vector3 &get_up_direction() const { return character.up_direction; }
	void set_up_direction(const Vector3 &p_up_direction);
	bool is_on_floor() const { return character.flags.floor; }
	bool is_on_floor_only() const { return character.flags.floor && !character.flags.wall && !character.flags.ceiling; }
	bool is_on_wall() const { return character.flags.wall; }
	bool is_on_wall_only() const { return character.flags.wall && !character.flags.floor && !character.flags.ceiling; }
	bool is_on_ceiling() const { return character.flags.ceiling; }
	bool is_on_ceiling_only() const { return character.flags.ceiling && !character.flags.floor && !character.flags.wall; }
	const Vector3 &get_last_motion() const { return character.last_motion; }
	Vector3 get_position_delta() const { return get_global_transform().origin - character.previous_position; }
	const Vector3 &get_floor_normal() const { return character.floor_normal; }
	const Vector3 &get_wall_normal() const { return character.wall_normal; }
	const Vector3 &get_real_velocity() const { return character.real_velocity; }
	real_t get_floor_angle(const Vector3 &p_up_direction = Vector3(0.0f, 1.0f, 0.0f)) const;
	const Vector3 &get_platform_velocity() const { return character.platform_velocity; }
	const Vector3 &get_platform_angular_velocity() const { return character.platform_angular_velocity; }
	int get_slide_collision_count() const { return character.motion_results.size(); }
	PS3DT::MotionResult get_slide_collision(int p_bounce) const;
	void set_safe_margin(real_t p_margin) { character.margin = p_margin; }
	real_t get_safe_margin() const { return character.margin; }
	bool is_floor_stop_on_slope_enabled() const { return character.floor_stop_on_slope; }
	void set_floor_stop_on_slope_enabled(bool p_enabled) { character.floor_stop_on_slope = p_enabled; }
	bool is_floor_constant_speed_enabled() const { return character.floor_constant_speed; }
	void set_floor_constant_speed_enabled(bool p_enabled) { character.floor_constant_speed = p_enabled; }
	bool is_floor_block_on_wall_enabled() const { return character.floor_block_on_wall; }
	void set_floor_block_on_wall_enabled(bool p_enabled) { character.floor_block_on_wall = p_enabled; }
	bool is_slide_on_ceiling_enabled() const { return character.slide_on_ceiling; }
	void set_slide_on_ceiling_enabled(bool p_enabled) { character.slide_on_ceiling = p_enabled; }
	int get_max_slides() const { return character.max_slides; }
	void set_max_slides(int p_max_slides);
	real_t get_floor_max_angle() const { return character.floor_max_angle; }
	void set_floor_max_angle(real_t p_radians) { character.floor_max_angle = p_radians; }
	real_t get_floor_snap_length() const { return character.floor_snap_length; }
	void set_floor_snap_length(real_t p_floor_snap_length);
	real_t get_wall_min_slide_angle() const { return character.wall_min_slide_angle; }
	void set_wall_min_slide_angle(real_t p_radians) { character.wall_min_slide_angle = p_radians; }
	uint32_t get_platform_floor_layers() const { return character.platform_floor_layers; }
	void set_platform_floor_layers(uint32_t p_exclude_layer) { character.platform_floor_layers = p_exclude_layer; }
	uint32_t get_platform_wall_layers() const { return character.platform_wall_layers; }
	void set_platform_wall_layers(uint32_t p_exclude_layer) { character.platform_wall_layers = p_exclude_layer; }
	void set_motion_mode(MotionMode p_mode) { character.motion_mode = p_mode; }
	MotionMode get_motion_mode() const { return character.motion_mode; }
	void set_platform_on_leave(PlatformOnLeave p_on_leave) { character.platform_on_leave = p_on_leave; }
	PlatformOnLeave get_platform_on_leave() const { return character.platform_on_leave; }

	// VehicleRig3D API.
	VehicleRig3D &get_vehicle_rig() { return vehicle; }
	const VehicleRig3D &get_vehicle_rig() const { return vehicle; }
	void set_engine_force(real_t p_engine_force);
	real_t get_engine_force() const { return vehicle.engine_force; }
	void set_brake(real_t p_brake);
	real_t get_brake() const { return vehicle.brake; }
	void set_steering(real_t p_steering);
	real_t get_steering() const { return vehicle.steering; }

	// PhysicalBoneState3D API.
	PhysicalBoneState3D &get_physical_bone_state() { return bone_state; }
	const PhysicalBoneState3D &get_physical_bone_state() const { return bone_state; }
	PhysicalBoneSimulator3D *get_simulator() const;
	int get_bone_id() const { return bone_state.bone_id; }
	RID get_joint_rid() const { return bone_state.joint; }
	void set_bone_joint_type(BoneJointType p_joint_type) { bone_state.joint_type = p_joint_type; }
	BoneJointType get_bone_joint_type() const { return bone_state.joint_type; }
	const PhysicalBoneState3D::JointParams &get_bone_joint_params() const { return bone_state.joint_params; }
	PhysicalBoneState3D::JointParams &get_bone_joint_params() { return bone_state.joint_params; }
	void set_joint_offset(const Transform3D &p_offset) { bone_state.joint_offset = p_offset; }
	const Transform3D &get_joint_offset() const { return bone_state.joint_offset; }
	Vector3 get_joint_rotation() const { return bone_state.joint_offset.basis.get_euler_normalized(); }
	void set_body_offset(const Transform3D &p_offset);
	const Transform3D &get_body_offset() const { return bone_state.body_offset; }
	void set_simulate_physics(bool p_simulate);
	bool get_simulate_physics() const { return bone_state.simulate_physics; }
	bool is_simulating_physics() const { return bone_state.internal_simulate_physics; }
	void set_bone_name(const String &p_name) { bone_state.bone_name = p_name; }
	const String &get_bone_name() const { return bone_state.bone_name; }
	void set_friction(real_t p_friction) { bone_state.friction = p_friction; }
	real_t get_friction() const { return bone_state.friction; }
	void set_bounce(real_t p_bounce) { bone_state.bounce = p_bounce; }
	real_t get_bounce() const { return bone_state.bounce; }
	void reset_physics_simulation_state();

#ifdef TOOLS_ENABLED
	void _set_gizmo_move_joint(bool p_move_joint) { bone_state.gizmo_move_joint = p_move_joint; }
	virtual Transform3D get_global_gizmo_transform() const override;
	virtual Transform3D get_local_gizmo_transform() const override;
#endif

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/physics_body_3d.h ---

// --- Folded from scene/3d/physics/physical_bone_simulator_3d.h ---

class PhysicsBody3D;

class PhysicalBoneSimulator3D : public SkeletonModifier3D {
	bool simulating = false;

	struct SimulatedBone {
		int parent = -1;
		Vector<int> child_bones;
		Transform3D global_pose;
		PhysicsBody3D *physical_bone = nullptr;
		PhysicsBody3D *cache_parent_physical_bone = nullptr;
	};

	LocalVector<SimulatedBone> bones;

	PhysicsBody3D *_get_physical_bone_parent(int p_bone);
	void _reset_physical_bones_state();

protected:
	virtual void _set_active(bool p_active) override;
	void _bone_pose_updated(Skeleton3D *skeleton, int p_bone_id);
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;

public:
#ifndef DISABLE_DEPRECATED
	bool is_compat = false;
#endif // _DISABLE_DEPRECATED
	bool is_simulating_physics() const;
	int get_bone_count() const;
	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D &p_pose);
	PhysicsBody3D *get_physical_bone(int p_bone);
	PhysicsBody3D *get_physical_bone_parent(int p_bone);
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String> &p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
	PhysicalBoneSimulator3D() = default;
};

// --- End scene/3d/physics/physical_bone_simulator_3d.h ---

// --- Folded from scene/3d/physics/area_3d.h ---
#include "core/templates/vector.h"


class Area3D : public Node3D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState3D::DisableMode;

private:
	CollisionState3D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector3 gravity_vec = Vector3(0, -1, 0);
	real_t gravity = 9.8f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t angular_damp = 0.1f;
	real_t linear_damp = 0.1f;

	int priority = 0;

	real_t wind_force_magnitude = 0.0f;
	real_t wind_attenuation_factor = 0.0f;
	NodePath wind_source_path;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	bool use_reverb_bus = false;
	String reverb_bus;
	float reverb_amount = 0.0f;
	float reverb_uniformity = 0.0f;

	void _clear_monitoring();
	void _initialize_wind();

protected:
	void _notification(int p_what);

public:
	Area3D();
	~Area3D();

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector3 &p_center);
	const Vector3 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector3 &p_direction);
	const Vector3 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_wind_force_magnitude(real_t p_wind_force_magnitude);
	real_t get_wind_force_magnitude() const;

	void set_wind_attenuation_factor(real_t p_wind_attenuation_factor);
	real_t get_wind_attenuation_factor() const;

	void set_wind_source_path(const NodePath &p_wind_source_path);
	const NodePath &get_wind_source_path() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	bool has_overlapping_bodies() const;
	bool has_overlapping_areas() const;

	bool overlaps_area(Node *rp_area) const;
	bool overlaps_body(Node *rp_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const String &p_audio_bus);
	String get_audio_bus_name() const;

	void set_use_reverb_bus(bool p_enable);
	bool is_using_reverb_bus() const;

	void set_reverb_bus_name(const String &p_audio_bus);
	String get_reverb_bus_name() const;

	void set_reverb_amount(float p_amount);
	float get_reverb_amount() const;

	void set_reverb_uniformity(float p_uniformity);
	float get_reverb_uniformity() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/area_3d.h ---

// --- Folded from scene/3d/physics/collision_shape_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"

class CollisionShape3D : public Node3D {
	Ref<Shape3D> shape;

	uint32_t owner_id = 0;
	CollisionState3D *collision_state = nullptr;

	Color debug_color;
	bool debug_fill = true;
	bool disabled = false;

	Color _get_default_debug_color() const;

#ifdef DEBUG_ENABLED
	void _shape_changed();
#endif // DEBUG_ENABLED

#ifndef DISABLE_DEPRECATED
	void resource_changed(Ref<Resource> res);
#endif

protected:
	void _update_in_shape_owner(bool p_xform_only = false);
	void _notification(int p_what);

public:
	void make_convex_from_siblings();

	void set_shape(const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	void set_debug_fill_enabled(bool p_enable);
	bool get_debug_fill_enabled() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionShape3D();
	~CollisionShape3D();
};

// --- End scene/3d/physics/collision_shape_3d.h ---

// --- Folded from scene/3d/bone_attachment_3d.h ---
class BoneAttachment3D : public Node3D
{
	bool bound = false;
	String bone_name;
	int bone_idx = -1;

	bool override_pose = false;
	bool _override_dirty = false;
	bool overriding = false;

	bool use_external_skeleton = false;
	NodePath external_skeleton_node;

	void _check_bind();
	void _check_unbind();

	bool updating = false;
	void _transform_changed();
	void _update_external_skeleton_cache();

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	virtual void _on_bone_pose_update_bind_compat_90575(int p_bone_index);
	static void _bind_compatibility_methods();
#endif

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton();

	void set_bone_name(const String& p_name);
	String get_bone_name() const;

	void set_bone_idx(const int& p_idx);
	int get_bone_idx() const;

	void set_override_pose(bool p_override_pose);
	bool get_override_pose() const;

	void set_use_external_skeleton(bool p_use_external_skeleton);
	bool get_use_external_skeleton() const;
	void set_external_skeleton(NodePath p_external_skeleton);
	NodePath get_external_skeleton() const;

	virtual void on_skeleton_update();

#ifdef TOOLS_ENABLED
	virtual void notify_rebind_required();
#endif

	BoneAttachment3D() = default;
};

// --- Folded from scene/3d/importer_mesh_instance_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"

class ImporterMesh;

class ImporterMeshInstance3D : public Node3D
{
	Ref<ImporterMesh> mesh;
	Ref<Skin> skin;
	NodePath skeleton_path;
	Vector<Ref<Material>> surface_materials;
	uint32_t layer_mask = 1;
	RenderInstance3D::ShadowCastingSetting shadow_casting_setting =
		RenderInstance3D::SHADOW_CASTING_SETTING_ON;
	float visibility_range_begin = 0.0;
	float visibility_range_end = 0.0;
	float visibility_range_begin_margin = 0.0;
	float visibility_range_end_margin = 0.0;
	RenderInstance3D::VisibilityRangeFadeMode visibility_range_fade_mode =
		RenderInstance3D::VISIBILITY_RANGE_FADE_DISABLED;

public:
	void set_mesh(const Ref<ImporterMesh>& p_mesh);
	Ref<ImporterMesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_surface_material(int p_idx, const Ref<Material>& p_material);
	Ref<Material> get_surface_material(int p_idx) const;

	void set_skeleton_path(const NodePath& p_path);
	NodePath get_skeleton_path() const;

	void set_layer_mask(const uint32_t p_layer_mask);
	uint32_t get_layer_mask() const;

	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_shadow_casting_setting);
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const;

	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;

	RenderInstance3D::VisibilityRangeFadeMode get_visibility_range_fade_mode() const;
};

// --- Folded from scene/3d/mesh_instance_3d.h ---
#include "core/templates/local_vector.h"

#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/environment.h"
#include "scene/resources/mesh.h"

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED
class Skin;
class SkinReference;

class MeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

protected:
	Ref<Mesh> mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path;

	LocalVector<float> blend_shape_tracks;
	HashMap<String, int> blend_shape_properties;
	Vector<Ref<Material>> surface_override_materials;

	void _resolve_skeleton_path();

	bool surface_index_0 = false;

	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	bool _property_can_revert(const String& p_name) const;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_layer_mask(uint32_t p_mask) { render_instance.set_layer_mask(p_mask); }
	uint32_t get_layer_mask() const { return render_instance.get_layer_mask(); }
	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_setting)
	{
		render_instance.set_cast_shadows_setting(p_setting);
	}
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const
	{
		return render_instance.get_cast_shadows_setting();
	}
	void set_material_override(const Ref<Material>& p_material)
	{
		render_instance.set_material_override(p_material);
	}
	Ref<Material> get_material_override() const { return render_instance.get_material_override(); }
	void set_material_overlay(const Ref<Material>& p_material)
	{
		render_instance.set_material_overlay(p_material);
	}
	Ref<Material> get_material_overlay() const { return render_instance.get_material_overlay(); }
	void set_gi_mode(RenderInstance3D::GIMode p_mode) { render_instance.set_gi_mode(p_mode); }
	RenderInstance3D::GIMode get_gi_mode() const { return render_instance.get_gi_mode(); }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_skeleton_path(const NodePath& p_skeleton);
	NodePath get_skeleton_path();

	Ref<SkinReference> get_skin_reference() const;

	int get_blend_shape_count() const;
	int find_blend_shape_by_name(const String& p_name);
	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	int get_surface_override_material_count() const;
	void set_surface_override_material(int p_surface, const Ref<Material>& p_material);
	Ref<Material> get_surface_override_material(int p_surface) const;
	Ref<Material> get_active_material(int p_surface) const;

#ifndef PHYSICS_3D_DISABLED
	Node* create_trimesh_collision_node();
	void create_trimesh_collision();

	Node* create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
	void create_convex_collision(bool p_clean = true, bool p_simplify = false);

	Node* create_multiple_convex_collisions_node(
		const Ref<MeshConvexDecompositionSettings>& p_settings =
			Ref<MeshConvexDecompositionSettings>());
	void create_multiple_convex_collisions(const Ref<MeshConvexDecompositionSettings>& p_settings =
											   Ref<MeshConvexDecompositionSettings>());
#endif // PHYSICS_3D_DISABLED

	MeshInstance3D* create_debug_tangents_node();
	void create_debug_tangents();

	AABB get_aabb() const;

	Ref<ArrayMesh> bake_mesh_from_current_blend_shape_mix(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());
	Ref<ArrayMesh> bake_mesh_from_current_skeleton_pose(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());

	Ref<TriangleMesh> generate_triangle_mesh() const;

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	virtual PackedStringArray get_configuration_warnings() const override;

	MeshInstance3D();
};
// --- End scene/3d/mesh_instance_3d.h ---
