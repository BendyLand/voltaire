/**************************************************************************/
/*  skeleton_modification_2d.cpp                                          */
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

#include "skeleton_modification_2d.h"

#include "core/config/engine.h"
#include "scene/2d/skeleton_2d.h"

// --- Per-mode payload structs ---

struct SkeletonModification2D::LookAtData {
	int bone_idx = -1;
	NodePath bone2d_node;
	float additional_rotation = 0.0f;
	bool enable_constraint = false;
	float constraint_angle_min = 0.0f;
	float constraint_angle_max = float(Math::TAU);
	bool constraint_angle_invert = false;
	bool constraint_in_localspace = true;
};

struct SkeletonModification2D::CCDIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool rotate_from_joint = false;
		bool enable_constraint = false;
		float constraint_angle_min = 0.0f;
		float constraint_angle_max = float(Math::TAU);
		bool constraint_angle_invert = false;
		bool constraint_in_localspace = true;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	NodePath tip_node;
};

struct SkeletonModification2D::FABRIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		Vector2 magnet_position;
		bool use_target_rotation = false;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	Vector<Transform2D> transform_chain;
	float chain_tolerance = 0.01f;
	int chain_max_iterations = 10;
	Transform2D target_global_pose;
	Transform2D origin_global_pose;
};

struct SkeletonModification2D::JiggleData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool override_defaults = false;
		float stiffness = 3.0f;
		float mass = 0.75f;
		float damping = 0.75f;
		bool use_gravity = false;
		Vector2 gravity = Vector2(0, 6.0f);
		Vector2 force;
		Vector2 acceleration;
		Vector2 velocity;
		Vector2 last_position;
		Vector2 dynamic_position;
		Vector2 last_noncollision_position;
	};

	Vector<JointData> chain;
	float stiffness = 3.0f;
	float mass = 0.75f;
	float damping = 0.75f;
	bool use_gravity = false;
	Vector2 gravity = Vector2(0, 6.0f);
	bool use_colliders = false;
	uint32_t collision_mask = 1;
};

struct SkeletonModification2D::TwoBoneIKData {
	float target_minimum_distance = 0.0f;
	float target_maximum_distance = 0.0f;
	bool flip_bend_direction = false;
	NodePath joint_one_bone2d_node;
	int joint_one_bone_idx = -1;
	NodePath joint_two_bone2d_node;
	int joint_two_bone_idx = -1;
#ifdef TOOLS_ENABLED
	bool editor_draw_min_max = false;
#endif // TOOLS_ENABLED
};

struct SkeletonModification2D::PhysicalBonesData {
	struct JointData {
		NodePath physical_bone_node;
		int bone_idx = -1;
		bool simulating = false;
	};

	Vector<JointData> chain;
	bool simulation_active = false;
};

struct SkeletonModification2D::StackHolderData {
	Ref<SkeletonModificationStack2D> held_modification_stack;
};

// --- Lifecycle & mode payload management ---

SkeletonModification2D::SkeletonModification2D(Type p_type) {
	stack = nullptr;
	is_setup = false;
	_alloc_mode_data(p_type);
}

SkeletonModification2D::~SkeletonModification2D() {
	_free_mode_data();
}

void SkeletonModification2D::_alloc_mode_data(Type p_type) {
	modification_type = p_type;
	mode_data = nullptr;
	switch (p_type) {
		case MODIFICATION_NONE: {
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_LOOKAT: {
			mode_data = memnew(LookAtData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_CCDIK: {
			mode_data = memnew(CCDIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_FABRIK: {
			mode_data = memnew(FABRIKData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_JIGGLE: {
			mode_data = memnew(JiggleData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_TWOBONEIK: {
			mode_data = memnew(TwoBoneIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_PHYSICALBONES: {
			mode_data = memnew(PhysicalBonesData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_STACKHOLDER: {
			mode_data = memnew(StackHolderData);
			editor_draw_gizmo = false;
		} break;
	}
}

void SkeletonModification2D::_free_mode_data() {
	if (!mode_data) {
		modification_type = MODIFICATION_NONE;
		return;
	}
	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			memdelete(static_cast<LookAtData *>(mode_data));
			break;
		case MODIFICATION_CCDIK:
			memdelete(static_cast<CCDIKData *>(mode_data));
			break;
		case MODIFICATION_FABRIK:
			memdelete(static_cast<FABRIKData *>(mode_data));
			break;
		case MODIFICATION_JIGGLE:
			memdelete(static_cast<JiggleData *>(mode_data));
			break;
		case MODIFICATION_TWOBONEIK:
			memdelete(static_cast<TwoBoneIKData *>(mode_data));
			break;
		case MODIFICATION_PHYSICALBONES:
			memdelete(static_cast<PhysicalBonesData *>(mode_data));
			break;
		case MODIFICATION_STACKHOLDER:
			memdelete(static_cast<StackHolderData *>(mode_data));
			break;
	}
	mode_data = nullptr;
	modification_type = MODIFICATION_NONE;
}

void SkeletonModification2D::set_modification_type(Type p_type) {
	if (modification_type == p_type) {
		return;
	}
	_free_mode_data();
	_alloc_mode_data(p_type);
	if (stack) {
		_setup_modification(stack);
	}
}

void SkeletonModification2D::_ensure_mode(Type p_expected) {
	if (modification_type == MODIFICATION_NONE) {
		set_modification_type(p_expected);
	}
}

SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<LookAtData *>(mode_data) : nullptr;
}

const SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() const {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<const LookAtData *>(mode_data) : nullptr;
}

SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() {
	return modification_type == MODIFICATION_CCDIK ? static_cast<CCDIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() const {
	return modification_type == MODIFICATION_CCDIK ? static_cast<const CCDIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() {
	return modification_type == MODIFICATION_FABRIK ? static_cast<FABRIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() const {
	return modification_type == MODIFICATION_FABRIK ? static_cast<const FABRIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<JiggleData *>(mode_data) : nullptr;
}

const SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() const {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<const JiggleData *>(mode_data) : nullptr;
}

SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<TwoBoneIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() const {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<const TwoBoneIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<PhysicalBonesData *>(mode_data) : nullptr;
}

const SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() const {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<const PhysicalBonesData *>(mode_data) : nullptr;
}

SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<StackHolderData *>(mode_data) : nullptr;
}

const SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() const {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<const StackHolderData *>(mode_data) : nullptr;
}

// --- Common base helpers ---

Node2D *SkeletonModification2D::_resolve_node_2d(const NodePath &p_path) const {
	if (!stack || !stack->skeleton || !stack->skeleton->is_inside_tree() || p_path.is_empty()) {
		return nullptr;
	}
	return dynamic_cast<Node2D *>(stack->skeleton->get_node_or_null(p_path));
}

Bone2D *SkeletonModification2D::_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const {
	Bone2D *bone_node = dynamic_cast<Bone2D *>(_resolve_node_2d(p_path));
	if (bone_node) {
		r_bone_idx = bone_node->get_index_in_skeleton();
		return bone_node;
	}
	if (stack && stack->skeleton && r_bone_idx >= 0 && r_bone_idx < stack->skeleton->get_bone_count()) {
		return stack->skeleton->get_bone(r_bone_idx);
	}
	return nullptr;
}

bool SkeletonModification2D::_print_execution_error(bool p_condition, const String &p_message) {
	if (p_condition) {
		ERR_PRINT_ONCE(p_message);
	}
	return p_condition;
}

void SkeletonModification2D::reset_state() {
	stack = nullptr;
	is_setup = false;
}

void SkeletonModification2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
#ifdef TOOLS_ENABLED
	if (editor_draw_gizmo && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enabled() const {
	return enabled;
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_modification_stack() {
	return stack;
}

void SkeletonModification2D::set_is_setup(bool p_setup) {
	is_setup = p_setup;
}

bool SkeletonModification2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModification2D::set_execution_mode(int p_mode) {
	execution_mode = p_mode;
}

int SkeletonModification2D::get_execution_mode() const {
	return execution_mode;
}

void SkeletonModification2D::set_editor_draw_gizmo(bool p_draw_gizmo) {
	editor_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (is_setup && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_editor_draw_gizmo() const {
	return editor_draw_gizmo;
}

float SkeletonModification2D::clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert) {
	if (p_angle < 0) {
		p_angle = Math::TAU + p_angle;
	}
	if (p_min_bound < 0) {
		p_min_bound = Math::TAU + p_min_bound;
	}
	if (p_max_bound < 0) {
		p_max_bound = Math::TAU + p_max_bound;
	}
	if (p_min_bound > p_max_bound) {
		SWAP(p_min_bound, p_max_bound);
	}

	bool is_beyond_bounds = (p_angle < p_min_bound || p_angle > p_max_bound);
	bool is_within_bounds = (p_angle > p_min_bound && p_angle < p_max_bound);

	if ((!p_invert && is_beyond_bounds) || (p_invert && is_within_bounds)) {
		Vector2 min_bound_vec = Vector2(Math::cos(p_min_bound), Math::sin(p_min_bound));
		Vector2 max_bound_vec = Vector2(Math::cos(p_max_bound), Math::sin(p_max_bound));
		Vector2 angle_vec = Vector2(Math::cos(p_angle), Math::sin(p_angle));

		if (angle_vec.distance_squared_to(min_bound_vec) <= angle_vec.distance_squared_to(max_bound_vec)) {
			p_angle = p_min_bound;
		} else {
			p_angle = p_max_bound;
		}
	}

	return p_angle;
}

void SkeletonModification2D::editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted) {
	if (!p_operation_bone || !p_constraint_enabled || !stack || !stack->skeleton) {
		return;
	}
	const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);

	float arc_angle_min = p_min_bound;
	float arc_angle_max = p_max_bound;
	if (arc_angle_min < 0) {
		arc_angle_min = Math::TAU + arc_angle_min;
	}
	if (arc_angle_max < 0) {
		arc_angle_max = Math::TAU + arc_angle_max;
	}
	if (arc_angle_min > arc_angle_max) {
		SWAP(arc_angle_min, arc_angle_max);
	}
	arc_angle_min += p_operation_bone->get_bone_angle();
	arc_angle_max += p_operation_bone->get_bone_angle();

	if (p_constraint_in_localspace) {
		Node2D *parent_node = dynamic_cast<Node2D *>(p_operation_bone->get_parent());
		if (parent_node) {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()), parent_node->get_global_rotation() - stack->skeleton->get_global_rotation());
		} else {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
		}
	} else {
		stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
	}

	float radius = p_operation_bone->get_length() * 0.6f;
	if (p_constraint_inverted) {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min + float(Math::TAU), arc_angle_max, 32, bone_ik_color, 1.5f);
	} else {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min, arc_angle_max, 32, bone_ik_color, 1.5f);
	}
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_min), Math::sin(arc_angle_min)) * radius, bone_ik_color, 1.5f);
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_max), Math::sin(arc_angle_max)) * radius, bone_ik_color, 1.5f);
}

// --- Non-virtual dispatch: setup, gizmos, execute ---

void SkeletonModification2D::_setup_modification(SkeletonModificationStack2D *p_stack) {
	stack = p_stack;
	if (!stack) {
		is_setup = false;
		WARN_PRINT("Could not setup modification with name " + get_name());
		return;
	}
	is_setup = true;

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT: {
			LookAtData *d = _get_lookat();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->bone2d_node, d->bone_idx);
			}
		} break;
		case MODIFICATION_CCDIK: {
			CCDIKData *d = _get_ccdik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_FABRIK: {
			FABRIKData *d = _get_fabrik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_JIGGLE: {
			JiggleData *d = _get_jiggle();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					Bone2D *bone_node = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
					if (bone_node) {
						Vector2 pos = bone_node->get_global_position();
						d->chain.write[i].dynamic_position = pos;
						d->chain.write[i].last_position = pos;
						d->chain.write[i].last_noncollision_position = pos;
					}
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
			TwoBoneIKData *d = _get_twoboneik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
				_resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
			}
		} break;
		case MODIFICATION_PHYSICALBONES: {
			PhysicalBonesData *d = _get_physicalbones();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].physical_bone_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_STACKHOLDER: {
			StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->set_skeleton(stack->get_skeleton());
				d->held_modification_stack->setup();
			}
		} break;
	}
}

void SkeletonModification2D::_draw_editor_gizmo() {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
		case MODIFICATION_FABRIK:
		case MODIFICATION_JIGGLE:
		case MODIFICATION_PHYSICALBONES:
			break;
		case MODIFICATION_LOOKAT: {
			const LookAtData *d = _get_lookat();
			if (d && d->bone_idx >= 0 && d->bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *operation_bone = stack->skeleton->get_bone(d->bone_idx);
				editor_draw_angle_constraints(operation_bone, d->constraint_angle_min, d->constraint_angle_max, d->enable_constraint, d->constraint_in_localspace, d->constraint_angle_invert);
			}
		} break;
		case MODIFICATION_CCDIK: {
			const CCDIKData *d = _get_ccdik();
			if (d) {
				for (int i = 0; i < d->chain.size(); i++) {
					if (!d->chain[i].joint_draw_gizmo || d->chain[i].bone_idx < 0 || d->chain[i].bone_idx >= stack->skeleton->get_bone_count()) {
						continue;
					}
					Bone2D *operation_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
					editor_draw_angle_constraints(operation_bone, d->chain[i].constraint_angle_min, d->chain[i].constraint_angle_max, d->chain[i].enable_constraint, d->chain[i].constraint_in_localspace, d->chain[i].constraint_angle_invert);
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
#ifdef TOOLS_ENABLED
			const TwoBoneIKData *d = _get_twoboneik();
			if (d && d->editor_draw_min_max && d->joint_one_bone_idx >= 0 && d->joint_one_bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *bone_one = stack->skeleton->get_bone(d->joint_one_bone_idx);
				if (bone_one) {
					const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);
					Vector2 origin = stack->skeleton->to_local(bone_one->get_global_position());
					stack->skeleton->draw_set_transform(origin);
					if (d->target_minimum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_minimum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
					if (d->target_maximum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_maximum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
				}
			}
#endif // TOOLS_ENABLED
		} break;
		case MODIFICATION_STACKHOLDER: {
			const StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->draw_editor_gizmos();
			}
		} break;
	}
}

void SkeletonModification2D::_execute(float p_delta) {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			_execute_lookat(p_delta);
			break;
		case MODIFICATION_CCDIK:
			_execute_ccdik(p_delta);
			break;
		case MODIFICATION_FABRIK:
			_execute_fabrik(p_delta);
			break;
		case MODIFICATION_JIGGLE:
			_execute_jiggle(p_delta);
			break;
		case MODIFICATION_TWOBONEIK:
			_execute_twoboneik(p_delta);
			break;
		case MODIFICATION_PHYSICALBONES:
			_execute_physicalbones(p_delta);
			break;
		case MODIFICATION_STACKHOLDER:
			_execute_stackholder(p_delta);
			break;
	}
}

// --- Solver implementations ---

void SkeletonModification2D::_execute_lookat(float p_delta) {
	LookAtData *d = _get_lookat();
	if (!d) {
		return;
	}
	Bone2D *operation_bone = _resolve_bone_2d(d->bone2d_node, d->bone_idx);
	Node2D *target = _resolve_node_2d(target_node);
	if (!operation_bone || !target) {
		return;
	}

	Transform2D operation_transform = operation_bone->get_global_transform();
	operation_bone->look_at(target->get_global_position());
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle() + d->additional_rotation);

	if (d->enable_constraint) {
		if (d->constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(operation_transform.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_ccdik(float p_delta) {
	CCDIKData *d = _get_ccdik();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Node2D *tip = _resolve_node_2d(d->tip_node);
	if (!target || !tip) {
		return;
	}

	for (int i = 0; i < d->chain.size(); i++) {
		_execute_ccdik_joint(i, target, tip);
	}
}

void SkeletonModification2D::_execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip) {
	CCDIKData *d = _get_ccdik();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	CCDIKData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Transform2D prev_global = operation_bone->get_global_transform();
	if (j.rotate_from_joint) {
		operation_bone->look_at(p_target->get_global_position());
		operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	} else {
		Vector2 joint_pos = operation_bone->get_global_position();
		float tip_angle = joint_pos.angle_to_point(p_tip->get_global_position());
		float target_angle = joint_pos.angle_to_point(p_target->get_global_position());
		operation_bone->set_global_rotation(operation_bone->get_global_rotation() + (target_angle - tip_angle));
	}

	if (j.enable_constraint) {
		if (j.constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_fabrik(float p_delta) {
	FABRIKData *d = _get_fabrik();
	if (!d || d->chain.size() < 2) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}

	const int chain_len = d->chain.size();
	d->transform_chain.resize(chain_len);
	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
		if (!b) {
			return;
		}
		d->transform_chain.write[i] = b->get_global_transform();
	}

	d->origin_global_pose = d->transform_chain[0];
	d->target_global_pose = target->get_global_transform();

	Bone2D *final_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	float final_length = final_bone->get_length();
	float final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
	Vector2 final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;

	int iterations = 0;
	while (final_tip.distance_to(d->target_global_pose.get_origin()) > d->chain_tolerance && iterations < d->chain_max_iterations) {
		_fabrik_chain_backwards();
		_fabrik_chain_forwards();
		final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
		final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;
		iterations++;
	}

	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Transform2D prev_global = b->get_global_transform();
		Transform2D next_global = d->transform_chain[i];
		if (i == chain_len - 1 && d->chain[i].use_target_rotation) {
			next_global.set_rotation(d->target_global_pose.get_rotation());
		}
		b->set_global_transform(prev_global.interpolate_with(next_global, stack->strength));
		stack->skeleton->set_bone_local_pose_override(d->chain[i].bone_idx, b->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_fabrik_chain_backwards() {
	FABRIKData *d = _get_fabrik();
	const int last_idx = d->chain.size() - 1;
	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[last_idx].bone_idx);
	float last_angle = d->transform_chain[last_idx].get_rotation() + last_bone->get_bone_angle();
	Vector2 dir = Vector2(Math::cos(last_angle), Math::sin(last_angle));
	d->transform_chain.write[last_idx].set_origin(d->target_global_pose.get_origin() - dir * last_bone->get_length());

	for (int i = last_idx; i >= 1; i--) {
		Vector2 next_origin = d->transform_chain[i].get_origin();
		Vector2 curr_origin = d->transform_chain[i - 1].get_origin() + d->chain[i - 1].magnet_position;
		Bone2D *prev_bone = stack->skeleton->get_bone(d->chain[i - 1].bone_idx);
		Vector2 to_prev = (curr_origin - next_origin).normalized();
		d->transform_chain.write[i - 1].set_origin(next_origin + to_prev * prev_bone->get_length());
	}
}

void SkeletonModification2D::_fabrik_chain_forwards() {
	FABRIKData *d = _get_fabrik();
	d->transform_chain.write[0].set_origin(d->origin_global_pose.get_origin());
	const int chain_len = d->chain.size();

	for (int i = 0; i < chain_len - 1; i++) {
		Bone2D *curr_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Vector2 curr_origin = d->transform_chain[i].get_origin();
		Vector2 next_origin = d->transform_chain[i + 1].get_origin();
		float angle = curr_origin.angle_to_point(next_origin);
		d->transform_chain.write[i].set_rotation(angle - curr_bone->get_bone_angle());
		d->transform_chain.write[i + 1].set_origin(curr_origin + Vector2(Math::cos(angle), Math::sin(angle)) * curr_bone->get_length());
	}

	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	Vector2 last_origin = d->transform_chain[chain_len - 1].get_origin();
	float target_angle = last_origin.angle_to_point(d->target_global_pose.get_origin());
	d->transform_chain.write[chain_len - 1].set_rotation(target_angle - last_bone->get_bone_angle());
}

void SkeletonModification2D::_execute_jiggle(float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		_execute_jiggle_joint(i, target, p_delta);
	}
}

void SkeletonModification2D::_execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	JiggleData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Vector2 target_pos = p_target->get_global_position();
	float eff_mass = MAX(j.mass, CMP_EPSILON);
	j.force = (target_pos - j.dynamic_position) * j.stiffness * p_delta;
	if (j.use_gravity) {
		j.force += j.gravity * p_delta;
	}
	j.acceleration = j.force / eff_mass;
	j.velocity += j.acceleration * (1.0f - j.damping);
	j.dynamic_position += j.velocity + j.force;
	j.dynamic_position = j.dynamic_position - j.last_position + operation_bone->get_global_position();
	j.last_position = operation_bone->get_global_position();

	Transform2D prev_global = operation_bone->get_global_transform();
	operation_bone->look_at(j.dynamic_position);
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_twoboneik(float p_delta) {
	TwoBoneIKData *d = _get_twoboneik();
	if (!d) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Bone2D *bone_one = _resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
	Bone2D *bone_two = _resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
	if (!target || !bone_one || !bone_two) {
		return;
	}

	Transform2D prev_one = bone_one->get_global_transform();
	Transform2D prev_two = bone_two->get_global_transform();

	Vector2 origin = bone_one->get_global_position();
	Vector2 target_diff = target->get_global_position() - origin;
	float target_dist = target_diff.length();
	if (d->target_minimum_distance > 0.0f && target_dist < d->target_minimum_distance) {
		target_dist = d->target_minimum_distance;
	}
	if (d->target_maximum_distance > 0.0f && target_dist > d->target_maximum_distance) {
		target_dist = d->target_maximum_distance;
	}

	float len_one = bone_one->get_length();
	float len_two = bone_two->get_length();
	float total_len = len_one + len_two;

	float base_angle = target_diff.angle();
	if (target_dist >= total_len || Math::is_zero_approx(len_one) || Math::is_zero_approx(len_two)) {
		bone_one->set_global_rotation(base_angle - bone_one->get_bone_angle());
		bone_two->set_global_rotation(base_angle - bone_two->get_bone_angle());
	} else {
		float clamped_dist = MAX(target_dist, Math::abs(len_one - len_two) + CMP_EPSILON);
		float cos_a = CLAMP((clamped_dist * clamped_dist + len_one * len_one - len_two * len_two) / (2.0f * clamped_dist * len_one), -1.0f, 1.0f);
		float cos_b = CLAMP((len_one * len_one + len_two * len_two - clamped_dist * clamped_dist) / (2.0f * len_one * len_two), -1.0f, 1.0f);
		float angle_a = Math::acos(cos_a);
		float angle_b = Math::acos(cos_b);
		if (d->flip_bend_direction) {
			angle_a = -angle_a;
			angle_b = -angle_b;
		}
		float rot_one = base_angle - angle_a;
		float rot_two = rot_one + float(Math::PI) - angle_b;
		bone_one->set_global_rotation(rot_one - bone_one->get_bone_angle());
		bone_two->set_global_rotation(rot_two - bone_two->get_bone_angle());
	}

	Transform2D solved_one = bone_one->get_transform();
	Transform2D solved_two = bone_two->get_transform();
	bone_one->set_global_transform(prev_one.interpolate_with(bone_one->get_global_transform(), stack->strength));
	bone_two->set_global_transform(prev_two.interpolate_with(bone_two->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->joint_one_bone_idx, solved_one, stack->strength, false);
	stack->skeleton->set_bone_local_pose_override(d->joint_two_bone_idx, solved_two, stack->strength, false);
}

void SkeletonModification2D::_execute_physicalbones(float p_delta) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !d->simulation_active) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		PhysicalBonesData::JointData &j = d->chain.write[i];
		if (!j.simulating) {
			continue;
		}
		Node2D *phys_node = _resolve_node_2d(j.physical_bone_node);
		if (!phys_node || j.bone_idx < 0 || j.bone_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *bone_node = stack->skeleton->get_bone(j.bone_idx);
		if (!bone_node) {
			continue;
		}
		Transform2D prev_global = bone_node->get_global_transform();
		bone_node->set_global_transform(prev_global.interpolate_with(phys_node->get_global_transform(), stack->strength));
		stack->skeleton->set_bone_local_pose_override(j.bone_idx, bone_node->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_execute_stackholder(float p_delta) {
	StackHolderData *d = _get_stackholder();
	if (d && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->execute(p_delta, execution_mode);
	}
}

// --- Shared target node ---

void SkeletonModification2D::set_target_node(const NodePath &p_target_node) {
	target_node = p_target_node;
}

NodePath SkeletonModification2D::get_target_node() const {
	return target_node;
}

// --- LookAt accessors ---

void SkeletonModification2D::set_bone2d_node(const NodePath &p_target_node) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(d->bone2d_node, d->bone_idx);
	}
}

NodePath SkeletonModification2D::get_bone2d_node() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone2d_node : NodePath();
}

void SkeletonModification2D::set_bone_index(int p_idx) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone_idx = p_idx;
	if (is_setup && stack && stack->skeleton && p_idx >= 0 && p_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_idx);
		if (b) {
			d->bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_bone_index() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone_idx : -1;
}

void SkeletonModification2D::set_additional_rotation(float p_rotation) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->additional_rotation = p_rotation;
}

float SkeletonModification2D::get_additional_rotation() const {
	const LookAtData *d = _get_lookat();
	return d ? d->additional_rotation : 0.0f;
}

void SkeletonModification2D::set_enable_constraint(bool p_constraint) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enable_constraint() const {
	const LookAtData *d = _get_lookat();
	return d ? d->enable_constraint : false;
}

void SkeletonModification2D::set_constraint_angle_min(float p_angle_min) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_min() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_min : 0.0f;
}

void SkeletonModification2D::set_constraint_angle_max(float p_angle_max) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_max() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_max : float(Math::TAU);
}

void SkeletonModification2D::set_constraint_angle_invert(bool p_invert) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_angle_invert() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_invert : false;
}

void SkeletonModification2D::set_constraint_in_localspace(bool p_constraint_in_localspace) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_in_localspace() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_in_localspace : true;
}

// --- CCDIK accessors ---

void SkeletonModification2D::set_tip_node(const NodePath &p_tip_node) {
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->tip_node = p_tip_node;
}

NodePath SkeletonModification2D::get_tip_node() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->tip_node : NodePath();
}

int SkeletonModification2D::get_ccdik_data_chain_length() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_ccdik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_ccdik_joint_bone2d_node(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_ccdik_joint_bone_index(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].rotate_from_joint = p_rotate_from_joint;
}

bool SkeletonModification2D::get_ccdik_joint_rotate_from_joint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].rotate_from_joint;
}

void SkeletonModification2D::set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_enable_constraint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].enable_constraint;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_min(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_min;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_max(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_max;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_invert;
}

void SkeletonModification2D::set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_in_localspace;
}

void SkeletonModification2D::set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].joint_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].joint_draw_gizmo;
}

// --- FABRIK accessors ---

int SkeletonModification2D::get_fabrik_data_chain_length() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_fabrik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_fabrik_joint_bone2d_node(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_fabrik_joint_bone_index(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].magnet_position = p_magnet_position;
}

Vector2 SkeletonModification2D::get_fabrik_joint_magnet_position(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, Vector2());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), Vector2(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].magnet_position;
}

void SkeletonModification2D::set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].use_target_rotation = p_use_target_rotation;
}

bool SkeletonModification2D::get_fabrik_joint_use_target_rotation(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].use_target_rotation;
}

void SkeletonModification2D::set_chain_tolerance(float p_tolerance) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_tolerance <= 0.0f);
	d->chain_tolerance = p_tolerance;
}

float SkeletonModification2D::get_chain_tolerance() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_tolerance : 0.01f;
}

void SkeletonModification2D::set_chain_max_iterations(int p_iterations) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_iterations < 1);
	d->chain_max_iterations = p_iterations;
}

int SkeletonModification2D::get_chain_max_iterations() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_max_iterations : 10;
}

// --- Jiggle accessors ---

void SkeletonModification2D::_update_jiggle_joint_data() {
	JiggleData *d = _get_jiggle();
	if (!d) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		if (!d->chain[i].override_defaults) {
			d->chain.write[i].stiffness = d->stiffness;
			d->chain.write[i].mass = d->mass;
			d->chain.write[i].damping = d->damping;
			d->chain.write[i].use_gravity = d->use_gravity;
			d->chain.write[i].gravity = d->gravity;
		}
	}
}

void SkeletonModification2D::reset() {
	JiggleData *d = _get_jiggle();
	if (!d || !is_setup || !stack || !stack->skeleton) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		const int b_idx = d->chain[i].bone_idx;
		if (b_idx < 0 || b_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *b = stack->skeleton->get_bone(b_idx);
		if (b) {
			Vector2 bone_pos = b->get_global_position();
			d->chain.write[i].dynamic_position = bone_pos;
			d->chain.write[i].last_position = bone_pos;
			d->chain.write[i].last_noncollision_position = bone_pos;
			d->chain.write[i].velocity = Vector2(0, 0);
			d->chain.write[i].acceleration = Vector2(0, 0);
			d->chain.write[i].force = Vector2(0, 0);
		}
	}
}

void SkeletonModification2D::set_stiffness(float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->stiffness = p_stiffness;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_stiffness() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->stiffness : 3.0f;
}

void SkeletonModification2D::set_mass(float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->mass = p_mass;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_mass() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->mass : 0.75f;
}

void SkeletonModification2D::set_damping(float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->damping = p_damping;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_damping() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->damping : 0.75f;
}

void SkeletonModification2D::set_use_gravity(bool p_use_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_gravity = p_use_gravity;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_use_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_gravity : false;
}

void SkeletonModification2D::set_gravity(Vector2 p_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->gravity = p_gravity;
	_update_jiggle_joint_data();
}

Vector2 SkeletonModification2D::get_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->gravity : Vector2(0, 6.0f);
}

void SkeletonModification2D::set_use_colliders(bool p_use_colliders) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_colliders = p_use_colliders;
}

bool SkeletonModification2D::get_use_colliders() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_colliders : false;
}

void SkeletonModification2D::set_collision_mask(int p_mask) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->collision_mask = uint32_t(p_mask);
}

int SkeletonModification2D::get_collision_mask() const {
	const JiggleData *d = _get_jiggle();
	return d ? int(d->collision_mask) : 1;
}

int SkeletonModification2D::get_jiggle_data_chain_length() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_jiggle_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
	_update_jiggle_joint_data();
}

void SkeletonModification2D::set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_jiggle_joint_bone2d_node(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_jiggle_joint_bone_index(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_jiggle_joint_override(int p_joint_idx, bool p_override) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].override_defaults = p_override;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_jiggle_joint_override(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].override_defaults;
}

void SkeletonModification2D::set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].stiffness = p_stiffness;
}

float SkeletonModification2D::get_jiggle_joint_stiffness(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].stiffness;
}

void SkeletonModification2D::set_jiggle_joint_mass(int p_joint_idx, float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].mass = p_mass;
}

float SkeletonModification2D::get_jiggle_joint_mass(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].mass;
}

void SkeletonModification2D::set_jiggle_joint_damping(int p_joint_idx, float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].damping = p_damping;
}

float SkeletonModification2D::get_jiggle_joint_damping(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].damping;
}

void SkeletonModification2D::set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].use_gravity = p_use_gravity;
}

bool SkeletonModification2D::get_jiggle_joint_use_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].use_gravity;
}

void SkeletonModification2D::set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].gravity = p_gravity;
}

Vector2 SkeletonModification2D::get_jiggle_joint_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, Vector2(0, 0));
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), Vector2(0, 0));
	return d->chain[p_joint_idx].gravity;
}

// --- TwoBoneIK accessors ---

void SkeletonModification2D::set_target_minimum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target minimum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_minimum_distance = p_distance;
}

float SkeletonModification2D::get_target_minimum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_minimum_distance : 0.0f;
}

void SkeletonModification2D::set_target_maximum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target maximum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_maximum_distance = p_distance;
}

float SkeletonModification2D::get_target_maximum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_maximum_distance : 0.0f;
}

void SkeletonModification2D::set_flip_bend_direction(bool p_flip_direction) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->flip_bend_direction = p_flip_direction;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_flip_bend_direction() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->flip_bend_direction : false;
}

void SkeletonModification2D::set_joint_one_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_one_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_one_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_one_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_one_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_one_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone_idx : -1;
}

void SkeletonModification2D::set_joint_two_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_two_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_two_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_two_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_two_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_two_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone_idx : -1;
}

#ifdef TOOLS_ENABLED
void SkeletonModification2D::set_editor_draw_min_max(bool p_draw) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->editor_draw_min_max = p_draw;
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
}

bool SkeletonModification2D::get_editor_draw_min_max() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->editor_draw_min_max : false;
}
#endif // TOOLS_ENABLED

// --- PhysicalBones accessors ---

int SkeletonModification2D::get_physical_bone_chain_length() const {
	const PhysicalBonesData *d = _get_physicalbones();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_physical_bone_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_physical_bone_node(int p_joint_idx, const NodePath &p_nodepath) {
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Joint index out of range!");
	d->chain.write[p_joint_idx].physical_bone_node = p_nodepath;
	if (is_setup) {
		_resolve_bone_2d(p_nodepath, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_physical_bone_node(int p_joint_idx) const {
	const PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Joint index out of range!");
	return d->chain[p_joint_idx].physical_bone_node;
}

void SkeletonModification2D::fetch_physical_bones() {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !stack || !stack->skeleton) {
		return;
	}
	d->chain.clear();
	for (int i = 0; i < stack->skeleton->get_bone_count(); i++) {
		Bone2D *b = stack->skeleton->get_bone(i);
		if (!b) {
			continue;
		}
		PhysicalBonesData::JointData jd;
		jd.physical_bone_node = stack->skeleton->get_path_to(b);
		jd.bone_idx = i;
		d->chain.push_back(jd);
	}
}

void SkeletonModification2D::start_simulation(const Vector<String> &p_bones) {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->simulation_active = true;
	for (int i = 0; i < d->chain.size(); i++) {
		if (p_bones.is_empty()) {
			d->chain.write[i].simulating = true;
		} else {
			Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
			d->chain.write[i].simulating = (n && p_bones.has(n->get_name()));
		}
	}
}

void SkeletonModification2D::stop_simulation(const Vector<String> &p_bones) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d) {
		return;
	}
	if (p_bones.is_empty()) {
		d->simulation_active = false;
		for (int i = 0; i < d->chain.size(); i++) {
			d->chain.write[i].simulating = false;
		}
		return;
	}
	bool any_active = false;
	for (int i = 0; i < d->chain.size(); i++) {
		Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
		if (n && p_bones.has(n->get_name())) {
			d->chain.write[i].simulating = false;
		}
		if (d->chain[i].simulating) {
			any_active = true;
		}
	}
	d->simulation_active = any_active;
}

// --- StackHolder accessors ---

void SkeletonModification2D::set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack) {
	_ensure_mode(MODIFICATION_STACKHOLDER);
	StackHolderData *d = _get_stackholder();
	ERR_FAIL_NULL(d);
	d->held_modification_stack = p_held_stack;
	if (is_setup && stack && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->set_skeleton(stack->get_skeleton());
		d->held_modification_stack->setup();
	}
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_held_modification_stack() const {
	const StackHolderData *d = _get_stackholder();
	return d ? d->held_modification_stack : Ref<SkeletonModificationStack2D>();
}

// --- skeleton_modification_stack_2d.cpp ---

#include "skeleton_modification_stack_2d.h"

#include "scene/2d/skeleton_2d.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"

void SkeletonModificationStack2D::setup() {
	if (is_setup) {
		return;
	}

	if (skeleton != nullptr) {
		is_setup = true;
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			modifications.get(i)->_setup_modification(this);
		}

#ifdef TOOLS_ENABLED
		set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	} else {
		WARN_PRINT("Cannot setup SkeletonModificationStack2D: no Skeleton2D set!");
	}
}

void SkeletonModificationStack2D::execute(float p_delta, int p_execution_mode) {
	if (!is_setup || !enabled || skeleton == nullptr) {
		return;
	}

	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		if (modifications[i]->get_enabled() && modifications[i]->get_execution_mode() == p_execution_mode) {
			modifications.get(i)->_execute(p_delta);
		}
	}
}

void SkeletonModificationStack2D::draw_editor_gizmos() {
	if (!is_setup || !skeleton) {
		return;
	}

	if (editor_gizmo_dirty) {
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			if (modifications[i]->editor_draw_gizmo) {
				modifications.get(i)->_draw_editor_gizmo();
			}
		}
		skeleton->draw_set_transform(Vector2(0, 0));
		editor_gizmo_dirty = false;
	}
}

void SkeletonModificationStack2D::enable_all_modifications(bool p_enabled) {
	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		modifications.get(i)->set_enabled(p_enabled);
	}
}

Ref<SkeletonModification2D> SkeletonModificationStack2D::get_modification(int p_mod_idx) const {
	ERR_FAIL_INDEX_V(p_mod_idx, modifications.size(), nullptr);
	return modifications[p_mod_idx];
}

void SkeletonModificationStack2D::add_modification(Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_COND(p_mod.is_null());

	p_mod->_setup_modification(this);
	modifications.push_back(p_mod);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::delete_modification(int p_mod_idx) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());
	modifications.remove_at(p_mod_idx);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification(int p_mod_idx, Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());

	if (p_mod.is_null()) {
		modifications.write[p_mod_idx] = Ref<SkeletonModification2D>();
	} else {
		modifications.write[p_mod_idx] = p_mod;
		p_mod->_setup_modification(this);
	}

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification_count(int p_count) {
	ERR_FAIL_COND_MSG(p_count < 0, "SkeletonModificationStack2D modification count cannot be less than zero.");
	modifications.resize(p_count);
#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

int SkeletonModificationStack2D::get_modification_count() const {
	return modifications.size();
}

void SkeletonModificationStack2D::set_skeleton(Skeleton2D *p_skeleton) {
	skeleton = p_skeleton;
}

Skeleton2D *SkeletonModificationStack2D::get_skeleton() const {
	return skeleton;
}

bool SkeletonModificationStack2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModificationStack2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

bool SkeletonModificationStack2D::get_enabled() const {
	return enabled;
}

void SkeletonModificationStack2D::set_editor_gizmos_dirty(bool p_dirty) {
	if (!is_setup) {
		return;
	}
	editor_gizmo_dirty = p_dirty;
	if (skeleton && editor_gizmo_dirty) {
		skeleton->queue_redraw();
	}
}

void SkeletonModificationStack2D::set_strength(float p_strength) {
	ERR_FAIL_COND_MSG(p_strength < 0, "Strength cannot be less than zero!");
	ERR_FAIL_COND_MSG(p_strength > 1, "Strength cannot be more than one!");
	strength = p_strength;
}

float SkeletonModificationStack2D::get_strength() const {
	return strength;
}

SkeletonModificationStack2D::SkeletonModificationStack2D() {}

void Skeleton2D::execute_modifications(real_t p_delta, int p_execution_mode) {
	if (modification_stack.is_null()) {
		return;
	}
	if (bone_setup_dirty) {
		_update_bone_setup();
	}
	modification_stack->execute(p_delta, p_execution_mode);
}
