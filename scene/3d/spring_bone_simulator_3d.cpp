/**************************************************************************/
/*  spring_bone_simulator_3d.cpp                                          */
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

#include "core/config/engine.h"
#include "scene/3d/spring_bone_collision_3d.h"
#include "spring_bone_simulator_3d.compat.inc"
#include "spring_bone_simulator_3d.h"

// Original VRM Spring Bone movement logic was distributed by (c) VRM Consortium. Licensed under the
// MIT license.

void SpringBoneSimulator3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint()) {
			set_notify_local_transform(true); // Used for updating gizmo in editor.
		}
#endif // TOOLS_ENABLED
		_make_collisions_dirty();
		_make_all_joints_dirty();
	} break;
#ifdef TOOLS_ENABLED
	case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
		_make_gizmo_dirty();
	} break;
	case NOTIFICATION_EDITOR_PRE_SAVE: {
		saving = true;
	} break;
	case NOTIFICATION_EDITOR_POST_SAVE: {
		saving = false;
	} break;
#endif // TOOLS_ENABLED
	}
}

// Setting.

void SpringBoneSimulator3D::set_root_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone_name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_root_bone(p_index, sk->find_bone(settings[p_index]->root_bone_name));
	}
}

String SpringBoneSimulator3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone_name;
}

void SpringBoneSimulator3D::set_root_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	bool changed = settings[p_index]->root_bone != p_bone;
	settings[p_index]->root_bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (settings[p_index]->root_bone <= -1 ||
			settings[p_index]->root_bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + ": Root bone index '" + itos(p_bone) +
						  "' is out of range!");
			settings[p_index]->root_bone = -1;
		}
		else {
			settings[p_index]->root_bone_name = sk->get_bone_name(settings[p_index]->root_bone);
		}
	}
	if (changed) {
		_update_joint_array(p_index);
	}
}

int SpringBoneSimulator3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone;
}

void SpringBoneSimulator3D::set_end_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_end_bone(p_index, sk->find_bone(settings[p_index]->end_bone_name));
	}
}

String SpringBoneSimulator3D::get_end_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->end_bone_name;
}

int SpringBoneSimulator3D::get_end_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->end_bone;
}

bool SpringBoneSimulator3D::is_end_bone_extended(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->extend_end_bone;
}

void SpringBoneSimulator3D::set_end_bone_direction(int p_index, BoneDirection p_bone_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_direction = p_bone_direction;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes) {
		return; // Chain dir will be recaluclated in _update_bone_axis().
	}
	_make_joints_dirty(p_index, true);
}

SkeletonModifier3D::BoneDirection SpringBoneSimulator3D::get_end_bone_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index]->end_bone_direction;
}

void SpringBoneSimulator3D::set_end_bone_length(int p_index, float p_length)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	float old = settings[p_index]->end_bone_length;
	settings[p_index]->end_bone_length = p_length;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes && Math::is_zero_approx(old) == Math::is_zero_approx(p_length)) {
		return; // If chain size is not changed, length will be recaluclated in _update_bone_axis().
	}
	_make_joints_dirty(p_index, true);
}

float SpringBoneSimulator3D::get_end_bone_length(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->end_bone_length;
}

Vector3 SpringBoneSimulator3D::get_end_bone_axis(int p_end_bone, BoneDirection p_direction) const
{
	Vector3 axis;
	if (p_direction == BONE_DIRECTION_FROM_PARENT) {
		Skeleton3D* sk = get_skeleton();
		if (sk) {
			axis = sk->get_bone_rest(p_end_bone)
					   .basis.xform_inv(mutable_bone_axes ? sk->get_bone_pose(p_end_bone).origin
														  : sk->get_bone_rest(p_end_bone).origin);
			axis.normalize();
		}
	}
	else {
		axis = get_vector_from_bone_axis(static_cast<BoneAxis>((int)p_direction));
	}
	return axis;
}

SpringBoneSimulator3D::CenterFrom SpringBoneSimulator3D::get_center_from(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), CENTER_FROM_WORLD_ORIGIN);
	return settings[p_index]->center_from;
}

NodePath SpringBoneSimulator3D::get_center_node(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->center_node;
}

void SpringBoneSimulator3D::set_center_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->center_bone_name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_center_bone(p_index, sk->find_bone(settings[p_index]->center_bone_name));
	}
}

String SpringBoneSimulator3D::get_center_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->center_bone_name;
}

void SpringBoneSimulator3D::set_center_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	bool center_changed = settings[p_index]->center_bone != p_bone;
	settings[p_index]->center_bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (settings[p_index]->center_bone <= -1 ||
			settings[p_index]->center_bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + ": Center bone index '" + itos(p_bone) +
						  "' is out of range!");
			settings[p_index]->center_bone = -1;
		}
		else {
			settings[p_index]->center_bone_name = sk->get_bone_name(settings[p_index]->center_bone);
		}
	}
	if (center_changed) {
		reset();
	}
}

int SpringBoneSimulator3D::get_center_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->center_bone;
}

void SpringBoneSimulator3D::set_radius(int p_index, float p_radius)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->radius = p_radius;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_radius(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->radius;
}

Ref<Curve> SpringBoneSimulator3D::get_radius_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->radius_damping_curve;
}

void SpringBoneSimulator3D::set_stiffness(int p_index, float p_stiffness)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->stiffness = p_stiffness;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_stiffness(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->stiffness;
}

Ref<Curve> SpringBoneSimulator3D::get_stiffness_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->stiffness_damping_curve;
}

void SpringBoneSimulator3D::set_drag(int p_index, float p_drag)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->drag = p_drag;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_drag(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->drag;
}

Ref<Curve> SpringBoneSimulator3D::get_drag_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->drag_damping_curve;
}

void SpringBoneSimulator3D::set_gravity(int p_index, float p_gravity)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->gravity = p_gravity;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_gravity(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->gravity;
}

Ref<Curve> SpringBoneSimulator3D::get_gravity_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->gravity_damping_curve;
}

void SpringBoneSimulator3D::set_gravity_direction(int p_index, const Vector3& p_gravity_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_gravity_direction.is_zero_approx());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->gravity_direction = p_gravity_direction;
	_make_joints_dirty(p_index);
}

Vector3 SpringBoneSimulator3D::get_gravity_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3(0, -1, 0));
	return settings[p_index]->gravity_direction;
}

SkeletonModifier3D::RotationAxis SpringBoneSimulator3D::get_rotation_axis(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	return settings[p_index]->rotation_axis;
}

void SpringBoneSimulator3D::set_rotation_axis_vector(int p_index, const Vector3& p_vector)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index) || settings[p_index]->rotation_axis != ROTATION_AXIS_CUSTOM) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->rotation_axis_vector = p_vector;
	_make_joints_dirty(p_index);
}

Vector3 SpringBoneSimulator3D::get_rotation_axis_vector(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	Vector3 ret;
	switch (settings[p_index]->rotation_axis) {
	case ROTATION_AXIS_X:
		ret = Vector3(1, 0, 0);
		break;
	case ROTATION_AXIS_Y:
		ret = Vector3(0, 1, 0);
		break;
	case ROTATION_AXIS_Z:
		ret = Vector3(0, 0, 1);
		break;
	case ROTATION_AXIS_ALL:
		ret = Vector3(0, 0, 0);
		break;
	case ROTATION_AXIS_CUSTOM:
		ret = settings[p_index]->rotation_axis_vector;
		break;
	}
	return ret;
}

int SpringBoneSimulator3D::get_setting_count() const { return settings.size(); }

void SpringBoneSimulator3D::clear_settings() { set_setting_count(0); }

// Individual joints.

bool SpringBoneSimulator3D::is_config_individual(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->individual_config;
}

String SpringBoneSimulator3D::get_joint_bone_name(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint]->bone_name;
}

void SpringBoneSimulator3D::_set_joint_bone(int p_index, int p_joint, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (joints[p_joint]->bone <= -1 || joints[p_joint]->bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + " : Joint: " + itos(p_joint) +
						  ": bone index '" + itos(p_bone) + "' is out of range!");
			joints[p_joint]->bone = -1;
		}
		else {
			joints[p_joint]->bone_name = sk->get_bone_name(joints[p_joint]->bone);
		}
	}
}

int SpringBoneSimulator3D::get_joint_bone(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint]->bone;
}

void SpringBoneSimulator3D::set_joint_radius(int p_index, int p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->radius = p_radius;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
}

float SpringBoneSimulator3D::get_joint_radius(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->radius;
}

void SpringBoneSimulator3D::set_joint_stiffness(int p_index, int p_joint, float p_stiffness)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->stiffness = p_stiffness;
}

float SpringBoneSimulator3D::get_joint_stiffness(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->stiffness;
}

void SpringBoneSimulator3D::set_joint_drag(int p_index, int p_joint, float p_drag)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->drag = p_drag;
}

float SpringBoneSimulator3D::get_joint_drag(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->drag;
}

void SpringBoneSimulator3D::set_joint_gravity(int p_index, int p_joint, float p_gravity)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->gravity = p_gravity;
}

float SpringBoneSimulator3D::get_joint_gravity(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->gravity;
}

void SpringBoneSimulator3D::set_joint_gravity_direction(
	int p_index, int p_joint, const Vector3& p_gravity_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_gravity_direction.is_zero_approx());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->gravity_direction = p_gravity_direction;
}

Vector3 SpringBoneSimulator3D::get_joint_gravity_direction(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3(0, -1, 0));
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3(0, -1, 0));
	return joints[p_joint]->gravity_direction;
}

SkeletonModifier3D::RotationAxis SpringBoneSimulator3D::get_joint_rotation_axis(
	int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), ROTATION_AXIS_ALL);
	return joints[p_joint]->rotation_axis;
}

void SpringBoneSimulator3D::set_joint_rotation_axis_vector(
	int p_index, int p_joint, const Vector3& p_vector)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index) ||
		settings[p_index]->rotation_axis != ROTATION_AXIS_CUSTOM) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->rotation_axis_vector = p_vector;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		_validate_rotation_axis(sk, p_index, p_joint);
	}
	settings[p_index]->simulation_dirty = true;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
}

Vector3 SpringBoneSimulator3D::get_joint_rotation_axis_vector(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3());
	return joints[p_joint]->get_rotation_axis_vector();
}

int SpringBoneSimulator3D::get_joint_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	return joints.size();
}

bool SpringBoneSimulator3D::are_all_child_collisions_enabled(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->enable_all_child_collisions;
}

void SpringBoneSimulator3D::set_exclude_collision_path(
	int p_index, int p_collision, const NodePath& p_node_path)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!are_all_child_collisions_enabled(p_index)) {
		return; // Exclude collision list is disabled.
	}
	LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	ERR_FAIL_INDEX(p_collision, (int)setting_exclude_collisions.size());
	setting_exclude_collisions[p_collision] = NodePath(); // Reset first.
	if (is_inside_tree()) {
		Node* node = get_node_or_null(p_node_path);
		if (!node) {
			_make_collisions_dirty();
			return;
		}
		node = node->get_parent();
		if (!node || node != this) {
			_make_collisions_dirty();
			ERR_FAIL_EDMSG("Collision must be child of current SpringBoneSimulator3D.");
		}
	}
	setting_exclude_collisions[p_collision] = p_node_path;
	_make_collisions_dirty();
}

NodePath SpringBoneSimulator3D::get_exclude_collision_path(int p_index, int p_collision) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	const LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	ERR_FAIL_INDEX_V(p_collision, (int)setting_exclude_collisions.size(), NodePath());
	return setting_exclude_collisions[p_collision];
}

int SpringBoneSimulator3D::get_exclude_collision_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	return setting_exclude_collisions.size();
}

void SpringBoneSimulator3D::clear_exclude_collisions(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!are_all_child_collisions_enabled(p_index)) {
		return; // Exclude collision list is disabled.
	}
	set_exclude_collision_count(p_index, 0);
}

void SpringBoneSimulator3D::set_collision_path(
	int p_index, int p_collision, const NodePath& p_node_path)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (are_all_child_collisions_enabled(p_index)) {
		return; // Collision list is disabled.
	}
	LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	ERR_FAIL_INDEX(p_collision, (int)setting_collisions.size());
	setting_collisions[p_collision] = NodePath(); // Reset first.
	if (is_inside_tree()) {
		Node* node = get_node_or_null(p_node_path);
		if (!node) {
			_make_collisions_dirty();
			return;
		}
		node = node->get_parent();
		if (!node || node != this) {
			_make_collisions_dirty();
			ERR_FAIL_EDMSG("Collision must be child of current SpringBoneSimulator3D.");
		}
	}
	setting_collisions[p_collision] = p_node_path;
	_make_collisions_dirty();
}

NodePath SpringBoneSimulator3D::get_collision_path(int p_index, int p_collision) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	const LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	ERR_FAIL_INDEX_V(p_collision, (int)setting_collisions.size(), NodePath());
	return setting_collisions[p_collision];
}

int SpringBoneSimulator3D::get_collision_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	return setting_collisions.size();
}

void SpringBoneSimulator3D::clear_collisions(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (are_all_child_collisions_enabled(p_index)) {
		return; // Collision list is disabled.
	}
	set_collision_count(p_index, 0);
}

void SpringBoneSimulator3D::set_external_force(const Vector3& p_force) { external_force = p_force; }

Vector3 SpringBoneSimulator3D::get_external_force() const { return external_force; }

void SpringBoneSimulator3D::set_mutable_bone_axes(bool p_enabled)
{
	mutable_bone_axes = p_enabled;
	for (SpringBone3DSetting* setting : settings) {
		setting->simulation_dirty = true;
	}
}

bool SpringBoneSimulator3D::are_bone_axes_mutable() const { return mutable_bone_axes; }

void SpringBoneSimulator3D::_make_all_joints_dirty()
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		_update_joint_array(i);
	}
}

void SpringBoneSimulator3D::_validate_rotation_axes(Skeleton3D* p_skeleton) const
{
	for (uint32_t i = 0; i < settings.size(); i++) {
		for (uint32_t j = 0; j < settings[i]->joints.size(); j++) {
			_validate_rotation_axis(p_skeleton, i, j);
		}
	}
}

void SpringBoneSimulator3D::_validate_rotation_axis(
	Skeleton3D* p_skeleton, int p_index, int p_joint) const
{
	RotationAxis axis = settings[p_index]->joints[p_joint]->rotation_axis;
	if (axis == ROTATION_AXIS_ALL) {
		return;
	}
	Vector3 rot = get_joint_rotation_axis_vector(p_index, p_joint).normalized();
	Vector3 fwd;
	if (p_joint < (int)settings[p_index]->joints.size() - 1) {
		fwd = p_skeleton->get_bone_rest(settings[p_index]->joints[p_joint + 1]->bone).origin;
	}
	else if (settings[p_index]->extend_end_bone) {
		fwd = get_end_bone_axis(settings[p_index]->end_bone, settings[p_index]->end_bone_direction);
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

void SpringBoneSimulator3D::_make_collisions_dirty() { collisions_dirty = true; }

void SpringBoneSimulator3D::_update_joint_array(int p_index)
{
	_make_joints_dirty(p_index, true);

	Skeleton3D* sk = get_skeleton();
	int current_bone = settings[p_index]->end_bone;
	int root_bone = settings[p_index]->root_bone;
	if (!sk || current_bone < 0 || root_bone < 0) {
		set_joint_count(p_index, 0);
		return;
	}

	// Validation.
	bool valid = false;
	while (current_bone >= 0) {
		if (current_bone == root_bone) {
			valid = true;
			break;
		}
		current_bone = sk->get_bone_parent(current_bone);
	}

	if (!valid) {
		set_joint_count(p_index, 0);
		ERR_FAIL_EDMSG("End bone must be the same as or a child of the root bone.");
	}

	LocalVector<int> new_joints;
	current_bone = settings[p_index]->end_bone;
	while (current_bone != root_bone) {
		new_joints.push_back(current_bone);
		current_bone = sk->get_bone_parent(current_bone);
	}
	new_joints.push_back(current_bone);
	new_joints.reverse();

	set_joint_count(p_index, new_joints.size());
	for (uint32_t i = 0; i < new_joints.size(); i++) {
		_set_joint_bone(p_index, i, new_joints[i]);
	}
}

void SpringBoneSimulator3D::_update_joints(bool p_reset)
{
	if (!joints_dirty) {
		return;
	}
	for (uint32_t i = 0; i < settings.size(); i++) {
		if (!settings[i]->joints_dirty) {
			continue;
		}
		if (settings[i]->individual_config) {
			settings[i]->simulation_dirty = p_reset;
			settings[i]->joints_dirty = false;
			continue; // Abort.
		}
		LocalVector<SpringBone3DJointSetting*>& joints = settings[i]->joints;
		float unit = joints.size() > 0 ? (1.0 / float(joints.size() - 1)) : 0.0;
		for (uint32_t j = 0; j < joints.size(); j++) {
			float offset = j * unit;

			if (settings[i]->radius_damping_curve.is_valid()) {
				joints[j]->radius =
					settings[i]->radius * settings[i]->radius_damping_curve->sample_baked(offset);
			}
			else {
				joints[j]->radius = settings[i]->radius;
			}

			if (settings[i]->stiffness_damping_curve.is_valid()) {
				joints[j]->stiffness = settings[i]->stiffness *
									   settings[i]->stiffness_damping_curve->sample_baked(offset);
			}
			else {
				joints[j]->stiffness = settings[i]->stiffness;
			}

			if (settings[i]->drag_damping_curve.is_valid()) {
				joints[j]->drag =
					settings[i]->drag * settings[i]->drag_damping_curve->sample_baked(offset);
			}
			else {
				joints[j]->drag = settings[i]->drag;
			}

			if (settings[i]->gravity_damping_curve.is_valid()) {
				joints[j]->gravity =
					settings[i]->gravity * settings[i]->gravity_damping_curve->sample_baked(offset);
			}
			else {
				joints[j]->gravity = settings[i]->gravity;
			}

			joints[j]->gravity_direction = settings[i]->gravity_direction;
			joints[j]->rotation_axis = settings[i]->rotation_axis;
			joints[j]->rotation_axis_vector = settings[i]->rotation_axis_vector;
		}
		settings[i]->simulation_dirty = p_reset;
		settings[i]->joints_dirty = false;
	}
	joints_dirty = false;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		_validate_rotation_axes(sk);
	}
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
}

void SpringBoneSimulator3D::_update_bone_axis(
	Skeleton3D* p_skeleton, SpringBone3DSetting* p_setting)
{
#ifdef TOOLS_ENABLED
	bool changed = false;
#endif // TOOLS_ENABLED
	const LocalVector<SpringBone3DJointSetting*>& joints = p_setting->joints;
	int len = (int)joints.size() - 1;
	for (int j = 0; j < len; j++) {
		if (!joints[j]->verlet) {
			continue;
		}
		Vector3 axis = p_skeleton->get_bone_pose(joints[j + 1]->bone).origin;
		if (axis.is_zero_approx()) {
			continue;
		}
		// Less computing.
#ifdef TOOLS_ENABLED
		if (!changed) {
			Vector3 old_v = joints[j]->verlet->forward_vector;
			joints[j]->verlet->forward_vector =
				snap_vector_to_plane(joints[j]->get_rotation_axis_vector(), axis.normalized());
			changed = changed || !old_v.is_equal_approx(joints[j]->verlet->forward_vector);
			float old_l = joints[j]->verlet->length;
			joints[j]->verlet->length = axis.length();
			changed = changed || !Math::is_equal_approx(old_l, joints[j]->verlet->length);
		}
		else {
			joints[j]->verlet->forward_vector =
				snap_vector_to_plane(joints[j]->get_rotation_axis_vector(), axis.normalized());
			joints[j]->verlet->length = axis.length();
		}
#else
		joints[j]->verlet->forward_vector =
			snap_vector_to_plane(joints[j]->get_rotation_axis_vector(), axis.normalized());
		joints[j]->verlet->length = axis.length();
#endif // TOOLS_ENABLED
	}
	if (p_setting->extend_end_bone && len >= 0) {
		if (joints[len]->verlet) {
			Vector3 axis = get_end_bone_axis(p_setting->end_bone, p_setting->end_bone_direction);
			if (!axis.is_zero_approx()) {
				joints[len]->verlet->forward_vector = snap_vector_to_plane(
					joints[len]->get_rotation_axis_vector(), axis.normalized());
				joints[len]->verlet->length = p_setting->end_bone_length;
			}
		}
	}
#ifdef TOOLS_ENABLED
	if (changed) {
		_make_gizmo_dirty();
	}
#endif // TOOLS_ENABLED
}

#ifdef TOOLS_ENABLED
Vector3 SpringBoneSimulator3D::get_bone_vector(int p_index, int p_joint) const
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return Vector3();
	}
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	SpringBone3DSetting* setting = settings[p_index];
	if (!setting) {
		return Vector3();
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = setting->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3());
	if (!joints[p_joint]->verlet) {
		if (p_joint == (int)joints.size() - 1) {
			return get_end_bone_axis(setting->end_bone, setting->end_bone_direction) *
				   setting->end_bone_length;
		}
		return mutable_bone_axes ? skeleton->get_bone_pose(joints[p_joint + 1]->bone).origin
								 : skeleton->get_bone_rest(joints[p_joint + 1]->bone).origin;
	}
	return joints[p_joint]->verlet->forward_vector * joints[p_joint]->verlet->length;
}

void SpringBoneSimulator3D::_redraw_gizmo()
{
	update_gizmos();
	gizmo_dirty = false;
}
#endif

void SpringBoneSimulator3D::_set_active(bool p_active)
{
	if (p_active) {
		reset();
	}
}

void SpringBoneSimulator3D::reset()
{
	if (!is_inside_tree()) {
		return;
	}
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}
	_find_collisions();
	_process_collisions();
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_joints_dirty(i, true);
		_init_joints(skeleton, settings[i]);
	}
}

SpringBoneSimulator3D::~SpringBoneSimulator3D() { clear_settings(); }


