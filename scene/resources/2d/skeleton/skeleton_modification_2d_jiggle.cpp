/**************************************************************************/
/*  skeleton_modification_2d_jiggle.cpp                                   */
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

#include "scene/2d/skeleton_2d.h"
#include "scene/resources/world_2d.h"
#include "skeleton_modification_2d_jiggle.h"

void SkeletonModification2DJiggle::_update_jiggle_joint_data()
{
	for (int i = 0; i < jiggle_data_chain.size(); i++) {
		if (!jiggle_data_chain[i].override_defaults) {
			set_jiggle_joint_stiffness(i, stiffness);
			set_jiggle_joint_mass(i, mass);
			set_jiggle_joint_damping(i, damping);
			set_jiggle_joint_use_gravity(i, use_gravity);
			set_jiggle_joint_gravity(i, gravity);
		}
	}
}

void SkeletonModification2DJiggle::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack) {
		is_setup = true;

		if (stack->skeleton) {
			for (int i = 0; i < jiggle_data_chain.size(); i++) {
				int bone_idx = jiggle_data_chain[i].bone_idx;
				if (bone_idx > 0 && bone_idx < stack->skeleton->get_bone_count()) {
					Bone2D* bone2d_node = stack->skeleton->get_bone(bone_idx);
					jiggle_data_chain.write[i].dynamic_position =
						bone2d_node->get_global_position();
				}

				jiggle_joint_update_bone2d_cache(i);
			}
		}

		update_target_cache();
	}
}

void SkeletonModification2DJiggle::set_target_node(const NodePath& p_target_node)
{
	target_node = p_target_node;
	update_target_cache();
}

NodePath SkeletonModification2DJiggle::get_target_node() const { return target_node; }

void SkeletonModification2DJiggle::set_stiffness(float p_stiffness)
{
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	stiffness = p_stiffness;
	_update_jiggle_joint_data();
}

float SkeletonModification2DJiggle::get_stiffness() const { return stiffness; }

void SkeletonModification2DJiggle::set_mass(float p_mass)
{
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	mass = p_mass;
	_update_jiggle_joint_data();
}

float SkeletonModification2DJiggle::get_mass() const { return mass; }

void SkeletonModification2DJiggle::set_damping(float p_damping)
{
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	damping = p_damping;
	_update_jiggle_joint_data();
}

float SkeletonModification2DJiggle::get_damping() const { return damping; }

void SkeletonModification2DJiggle::set_use_gravity(bool p_use_gravity)
{
	use_gravity = p_use_gravity;
	_update_jiggle_joint_data();
}

bool SkeletonModification2DJiggle::get_use_gravity() const { return use_gravity; }

void SkeletonModification2DJiggle::set_gravity(Vector2 p_gravity)
{
	gravity = p_gravity;
	_update_jiggle_joint_data();
}

Vector2 SkeletonModification2DJiggle::get_gravity() const { return gravity; }

bool SkeletonModification2DJiggle::get_use_colliders() const { return use_colliders; }

void SkeletonModification2DJiggle::set_collision_mask(int p_mask) { collision_mask = p_mask; }

int SkeletonModification2DJiggle::get_collision_mask() const { return collision_mask; }

// Jiggle joint data functions
int SkeletonModification2DJiggle::get_jiggle_data_chain_length()
{
	return jiggle_data_chain.size();
}

NodePath SkeletonModification2DJiggle::get_jiggle_joint_bone2d_node(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(
		p_joint_idx, jiggle_data_chain.size(), NodePath(), "Jiggle joint out of range!");
	return jiggle_data_chain[p_joint_idx].bone2d_node;
}

int SkeletonModification2DJiggle::get_jiggle_joint_bone_index(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, jiggle_data_chain.size(), -1, "Jiggle joint out of range!");
	return jiggle_data_chain[p_joint_idx].bone_idx;
}

bool SkeletonModification2DJiggle::get_jiggle_joint_override(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), false);
	return jiggle_data_chain[p_joint_idx].override_defaults;
}

void SkeletonModification2DJiggle::set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness)
{
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	ERR_FAIL_INDEX(p_joint_idx, jiggle_data_chain.size());
	jiggle_data_chain.write[p_joint_idx].stiffness = p_stiffness;
}

float SkeletonModification2DJiggle::get_jiggle_joint_stiffness(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), -1);
	return jiggle_data_chain[p_joint_idx].stiffness;
}

void SkeletonModification2DJiggle::set_jiggle_joint_mass(int p_joint_idx, float p_mass)
{
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	ERR_FAIL_INDEX(p_joint_idx, jiggle_data_chain.size());
	jiggle_data_chain.write[p_joint_idx].mass = p_mass;
}

float SkeletonModification2DJiggle::get_jiggle_joint_mass(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), -1);
	return jiggle_data_chain[p_joint_idx].mass;
}

void SkeletonModification2DJiggle::set_jiggle_joint_damping(int p_joint_idx, float p_damping)
{
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_INDEX(p_joint_idx, jiggle_data_chain.size());
	jiggle_data_chain.write[p_joint_idx].damping = p_damping;
}

float SkeletonModification2DJiggle::get_jiggle_joint_damping(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), -1);
	return jiggle_data_chain[p_joint_idx].damping;
}

bool SkeletonModification2DJiggle::get_jiggle_joint_use_gravity(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), false);
	return jiggle_data_chain[p_joint_idx].use_gravity;
}

void SkeletonModification2DJiggle::set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity)
{
	ERR_FAIL_INDEX(p_joint_idx, jiggle_data_chain.size());
	jiggle_data_chain.write[p_joint_idx].gravity = p_gravity;
}

Vector2 SkeletonModification2DJiggle::get_jiggle_joint_gravity(int p_joint_idx) const
{
	ERR_FAIL_INDEX_V(p_joint_idx, jiggle_data_chain.size(), Vector2(0, 0));
	return jiggle_data_chain[p_joint_idx].gravity;
}

SkeletonModification2DJiggle::SkeletonModification2DJiggle()
{
	stack = nullptr;
	is_setup = false;
	jiggle_data_chain = Vector<Jiggle_Joint_Data2D>();
	stiffness = 3;
	mass = 0.75;
	damping = 0.75;
	use_gravity = false;
	gravity = Vector2(0, 6.0);
	enabled = true;
	editor_draw_gizmo = false; // Nothing to really show in a gizmo right now.
}

void SkeletonModification2DJiggle::reset()
{
	if (!is_setup || !stack || !stack->skeleton) {
		return;
	}

	for (int i = 0; i < jiggle_data_chain.size(); i++) {
		const int bone_idx = jiggle_data_chain[i].bone_idx;
		if (bone_idx <= -1 || bone_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D* bone = stack->skeleton->get_bone(bone_idx);
		if (bone) {
			Vector2 bone_pos = bone->get_global_position();
			jiggle_data_chain.write[i].dynamic_position = bone_pos;
			jiggle_data_chain.write[i].last_position = bone_pos;
			jiggle_data_chain.write[i].last_noncollision_position = bone_pos;
			jiggle_data_chain.write[i].velocity = Vector2(0, 0);
			jiggle_data_chain.write[i].acceleration = Vector2(0, 0);
			jiggle_data_chain.write[i].force = Vector2(0, 0);
		}
	}
}

SkeletonModification2DJiggle::~SkeletonModification2DJiggle() {}


