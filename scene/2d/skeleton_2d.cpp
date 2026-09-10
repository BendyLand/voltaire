/**************************************************************************/
/*  skeleton_2d.cpp                                                       */
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
#include "core/math/transform_interpolator.h"
#include "servers/rendering/rendering_server.h"
#include "skeleton_2d.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_data.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
bool Bone2D::_editor_get_show_bone_gizmo() const { return _editor_show_bone_gizmo; }
#endif // TOOLS_ENABLED

void Bone2D::set_rest(const Transform2D& p_rest)
{
	rest = p_rest;
	if (skeleton) {
		skeleton->_make_bone_setup_dirty();
	}

	update_configuration_warnings();
}

Transform2D Bone2D::get_rest() const { return rest; }

Transform2D Bone2D::get_skeleton_rest() const
{
	if (parent_bone) {
		return parent_bone->get_skeleton_rest() * rest;
	}
	else {
		return rest;
	}
}

void Bone2D::apply_rest() { set_transform(rest); }

int Bone2D::get_index_in_skeleton() const
{
	ERR_FAIL_NULL_V(skeleton, -1);
	skeleton->_update_bone_setup();
	return skeleton_index;
}

PackedStringArray Bone2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!skeleton) {
		if (parent_bone) {
			warnings.push_back(RTR("This Bone2D chain should end at a Skeleton2D node."));
		}
		else {
			warnings.push_back(
				RTR("A Bone2D only works with a Skeleton2D or another Bone2D as parent node."));
		}
	}

	if (rest == Transform2D(0, 0, 0, 0, 0, 0)) {
		warnings.push_back(
			RTR("This bone lacks a proper REST pose. Go to the Skeleton2D node and set one."));
	}

	return warnings;
}

bool Bone2D::get_autocalculate_length_and_angle() const { return autocalculate_length_and_angle; }

real_t Bone2D::get_length() const { return length; }

real_t Bone2D::get_bone_angle() const { return bone_angle; }

Bone2D::Bone2D()
{
	skeleton = nullptr;
	parent_bone = nullptr;
	skeleton_index = -1;
	length = 16;
	bone_angle = 0;
	autocalculate_length_and_angle = true;
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	// this is a clever hack so the bone knows no rest has been set yet, allowing to show an error.
	for (int i = 0; i < 3; i++) {
		rest[i] = Vector2(0, 0);
	}
	copy_transform_to_cache = true;
}

Bone2D::~Bone2D()
{
#ifdef TOOLS_ENABLED
	if (!editor_gizmo_rid.is_null()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RenderingServer::get_singleton()->free_rid(editor_gizmo_rid);
	}
#endif // TOOLS_ENABLED
}

//////////////////////////////////////

void Skeleton2D::_update_transform()
{
	if (bone_setup_dirty) {
		_update_bone_setup();
		return; // above will update transform anyway
	}
	if (!transform_dirty) {
		return;
	}

	transform_dirty = false;

	for (uint32_t i = 0; i < bones.size(); i++) {
		ERR_CONTINUE(bones[i].parent_index >= (int)i);
		if (bones[i].parent_index >= 0) {
			bones[i].accum_transform =
				bones[bones[i].parent_index].accum_transform * bones[i].bone->get_transform();
		}
		else {
			bones[i].accum_transform = bones[i].bone->get_transform();
		}
	}

	for (uint32_t i = 0; i < bones.size(); i++) {
		Transform2D final_xform = bones[i].accum_transform * bones[i].rest_inverse;
		RS::get_singleton()->skeleton_bone_set_transform_2d(skeleton, i, final_xform);
	}
}

int Skeleton2D::get_bone_count() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), 0);

	if (bone_setup_dirty) {
		// TODO: Is this necessary? It doesn't seem to change bones.size()
		const_cast<Skeleton2D*>(this)->_update_bone_setup();
	}

	return bones.size();
}

Bone2D* Skeleton2D::get_bone(int p_idx)
{
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
	ERR_FAIL_INDEX_V(p_idx, (int)bones.size(), nullptr);

	return bones[p_idx].bone;
}

void Skeleton2D::_update_process_mode()
{
	bool process = modification_stack.is_valid() && is_inside_tree();
	if (!process) {
		// We might have another reason to process.
		process = is_physics_interpolated_and_enabled() && is_visible_in_tree();
	}

	set_process_internal(process);
	set_physics_process_internal(process);
}

void Skeleton2D::_ensure_update_interpolation_data()
{
	uint64_t tick = Engine::get_singleton()->get_physics_frames();

	if (_interpolation_data.last_update_physics_tick != tick) {
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		_interpolation_data.last_update_physics_tick = tick;
	}
}

void Skeleton2D::_physics_interpolated_changed() { _update_process_mode(); }

void Skeleton2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (bone_setup_dirty) {
			_update_bone_setup();
		}
		if (transform_dirty) {
			_update_transform();
		}
		request_ready();
	} break;

	case NOTIFICATION_ENTER_TREE: {
		_update_process_mode();

		if (is_physics_interpolated_and_enabled()) {
			_interpolation_data.xform_curr = get_global_transform();
			_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			if (Engine::get_singleton()->is_in_physics_frame()) {
				_interpolation_data.xform_curr = get_global_transform();
			}
		}
		else {
			RS::get_singleton()->skeleton_set_base_transform_2d(skeleton, get_global_transform());
		}
	} break;

	case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		_interpolation_data.xform_curr = get_global_transform();
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			Transform2D res;
			TransformInterpolator::interpolate_transform_2d(_interpolation_data.xform_prev,
				_interpolation_data.xform_curr, res,
				Engine::get_singleton()->get_physics_interpolation_fraction());
			RS::get_singleton()->skeleton_set_base_transform_2d(skeleton, res);
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_process);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			_interpolation_data.xform_curr = get_global_transform();
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_physics_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_physics_process);
		}
	} break;

	case NOTIFICATION_POST_ENTER_TREE: {
		set_modification_stack(modification_stack);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_process_mode();
	} break;

#ifdef TOOLS_ENABLED
	case NOTIFICATION_DRAW: {
		if (Engine::get_singleton()->is_editor_hint()) {
			if (modification_stack.is_valid()) {
				modification_stack->draw_editor_gizmos();
			}
		}
	} break;
#endif // TOOLS_ENABLED
	}
}

RID Skeleton2D::get_skeleton() const { return skeleton; }

void Skeleton2D::set_bone_local_pose_override(
	int p_bone_idx, Transform2D p_override, real_t p_amount, bool p_persistent)
{
	ERR_FAIL_INDEX_MSG(p_bone_idx, (int)bones.size(), "Bone index is out of range!");
	bones[p_bone_idx].local_pose_override = p_override;
	bones[p_bone_idx].local_pose_override_amount = p_amount;
	bones[p_bone_idx].local_pose_override_persistent = p_persistent;
}

Transform2D Skeleton2D::get_bone_local_pose_override(int p_bone_idx)
{
	ERR_FAIL_INDEX_V_MSG(
		p_bone_idx, (int)bones.size(), Transform2D(), "Bone index is out of range!");
	return bones[p_bone_idx].local_pose_override;
}

void Skeleton2D::set_modification_stack(Ref<SkeletonModificationStack2D> p_stack)
{
	if (modification_stack.is_valid()) {
		modification_stack->is_setup = false;
		modification_stack->set_skeleton(nullptr);
	}
	modification_stack = p_stack;
	if (modification_stack.is_valid() && is_inside_tree()) {
		modification_stack->set_skeleton(this);
		modification_stack->setup();

#ifdef TOOLS_ENABLED
		modification_stack->set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	}
	_update_process_mode();
}

Ref<SkeletonModificationStack2D> Skeleton2D::get_modification_stack() const
{
	return modification_stack;
}

Skeleton2D::Skeleton2D()
{
	skeleton = RS::get_singleton()->skeleton_create();
	set_notify_transform(true);
	set_hide_clip_children(true);
}

Skeleton2D::~Skeleton2D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(skeleton);
}


