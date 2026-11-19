/**************************************************************************/
/*  bone_constraint_3d.h                                                  */
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

class BoneConstraint3D : public SkeletonModifier3D {
public:
	enum ConstraintType {
		CONSTRAINT_AIM,
		CONSTRAINT_CONVERT_TRANSFORM,
		CONSTRAINT_COPY_TRANSFORM,
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

	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount);
	void _process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);

public:
	BoneConstraint3D(ConstraintType p_type = CONSTRAINT_COPY_TRANSFORM);
	~BoneConstraint3D();

	void set_constraint_type(ConstraintType p_type);
	ConstraintType get_constraint_type() const { return constraint_type; }

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
};
