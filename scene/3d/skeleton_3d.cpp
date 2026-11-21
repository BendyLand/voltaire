/**************************************************************************/
/*  skeleton_3d.cpp                                                       */
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

#include "scene/3d/skeleton_modifier_3d.h"
// --- skeleton_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "skeleton_3d.h"

void Skeleton3D::_add_bone_bind_compat_88791(const String& p_name) { add_bone(p_name); }

void Skeleton3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "skeleton_3d.h"
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
#include "scene/3d/physics/physical_bone_simulator_3d.h"
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED
#include "servers/rendering/renderer.h"

void SkinReference::_skin_changed()
{
	if (skeleton_node) {
		skeleton_node->_make_dirty();
	}
	skeleton_version = 0;
}

void Skeleton3D::_make_dirty() {}

RID SkinReference::get_skeleton() const { return skeleton; }

Ref<Skin> SkinReference::get_skin() const { return skin; }

SkinReference::~SkinReference()
{
	ERR_FAIL_NULL(Renderer::data);
	if (skeleton_node) {
		skeleton_node->skin_bindings.erase(this);
	}
	Renderer::free_rid(skeleton);
}

void Skeleton3D::_update_bone_names() const
{
	String names;
	for (uint32_t i = 0; i < bones.size(); i++) {
		if (i > 0) {
			names += ",";
		}
		names += bones[i].name;
	}
	concatenated_bone_names = String(names);
}

String Skeleton3D::get_concatenated_bone_names() const
{
	if (concatenated_bone_names == String()) {
		_update_bone_names();
	}
	return concatenated_bone_names;
}

void Skeleton3D::set_modifier_callback_mode_process(Skeleton3D::ModifierCallbackModeProcess p_mode)
{
	if (modifier_callback_mode_process == p_mode) {
		return;
	}
	modifier_callback_mode_process = p_mode;
	_process_changed();
}

Skeleton3D::ModifierCallbackModeProcess Skeleton3D::get_modifier_callback_mode_process() const
{
	return modifier_callback_mode_process;
}

void Skeleton3D::_process_changed()
{
	if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_IDLE) {
		set_process_internal(true);
		set_physics_process_internal(false);
	}
	else if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS) {
		set_process_internal(false);
		set_physics_process_internal(true);
	}
	else {
		set_process_internal(false);
		set_physics_process_internal(false);
	}
}

void Skeleton3D::_make_modifiers_dirty()
{
	modifiers_dirty = true;
	_update_deferred(UPDATE_FLAG_MODIFIER);
}

void Skeleton3D::_update_bones_nested_set() const
{
	nested_set_offset_to_bone_index.resize(bones.size());
	bone_global_pose_dirty.resize(bones.size());
	_make_bone_global_poses_dirty();

	int offset = 0;
	for (int bone : parentless_bones) {
		offset += _update_bone_nested_set(bone, offset);
	}
}

int Skeleton3D::_update_bone_nested_set(int p_bone, int p_offset) const
{
	Bone& bone = bones[p_bone];
	int offset = p_offset + 1;
	int span = 1;

	for (int child_bone : bone.child_bones) {
		int subspan = _update_bone_nested_set(child_bone, offset);
		offset += subspan;
		span += subspan;
	}

	nested_set_offset_to_bone_index[p_offset] = p_bone;
	bone.nested_set_offset = p_offset;
	bone.nested_set_span = span;

	return span;
}

void Skeleton3D::_make_bone_global_poses_dirty() const
{
	for (uint32_t i = 0; i < bone_global_pose_dirty.size(); i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_make_bone_global_pose_subtree_dirty(int p_bone) const
{
	if (process_order_dirty) {
		return;
	}

	const Bone& bone = bones[p_bone];
	int span_offset = bone.nested_set_offset;
	// No need to make subtree dirty when bone is already dirty.
	if (bone_global_pose_dirty[span_offset]) {
		return;
	}

	// Make global poses of subtree dirty.
	int span_end = span_offset + bone.nested_set_span;
	for (int i = span_offset; i < span_end; i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_update_bone_global_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	// Global pose is already calculated.
	int nested_set_offset = bones[p_bone].nested_set_offset;
	if (!bone_global_pose_dirty[nested_set_offset]) {
		return;
	}

	thread_local LocalVector<int> bone_list;
	bone_list.clear();
	Transform3D global_pose;

	// Create list of parent bones for which the global pose needs to be recalculated.
	for (int bone = p_bone; bone >= 0; bone = bones[bone].parent) {
		int offset = bones[bone].nested_set_offset;
		// Stop searching when global pose is not dirty.
		if (!bone_global_pose_dirty[offset]) {
			global_pose = bones[bone].global_pose;
			break;
		}

		bone_list.push_back(bone);
	}

	// Calculate global poses for all parent bones and the current bone.
	for (int i = bone_list.size() - 1; i >= 0; i--) {
		int bone_idx = bone_list[i];
		Bone& bone = bones[bone_idx];
		bool bone_enabled = bone.enabled && !show_rest_only;
		Transform3D bone_pose = bone_enabled ? get_bone_pose(bone_idx) : get_bone_rest(bone_idx);

		global_pose *= bone_pose;
#ifndef DISABLE_DEPRECATED
		if (bone.global_pose_override_amount >= CMP_EPSILON) {
			global_pose = global_pose.interpolate_with(
				bone.global_pose_override, bone.global_pose_override_amount);
		}
#endif // _DISABLE_DEPRECATED

		bone.global_pose = global_pose;
		bone_global_pose_dirty[bone.nested_set_offset] = false;
	}
}

Transform3D Skeleton3D::get_bone_global_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_update_bone_global_pose(p_bone);
	return bones[p_bone].global_pose;
}

void Skeleton3D::set_bone_global_pose(int p_bone, const Transform3D& p_pose)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	Transform3D pt;
	if (bones[p_bone].parent >= 0) {
		pt = get_bone_global_pose(bones[p_bone].parent);
	}
	Transform3D t = pt.affine_inverse() * p_pose;
	set_bone_pose(p_bone, t);
}

void Skeleton3D::set_motion_scale(float p_motion_scale)
{
	if (p_motion_scale <= 0) {
		motion_scale = 1;
		ERR_FAIL_MSG("Motion scale must be larger than 0.");
	}
	motion_scale = p_motion_scale;
}

float Skeleton3D::get_motion_scale() const
{
	ERR_FAIL_COND_V(motion_scale <= 0, 1);
	return motion_scale;
}

// Skeleton creation api

uint64_t Skeleton3D::get_version() const { return version; }

int Skeleton3D::find_bone(const String& p_name) const
{
	const int* bone_index_ptr = name_to_bone_index.getptr(p_name);
	return bone_index_ptr != nullptr ? *bone_index_ptr : -1;
}

String Skeleton3D::get_bone_name(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, "");
	return bones[p_bone].name;
}

void Skeleton3D::set_bone_name(int p_bone, const String& p_name)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	const int* bone_index_ptr = name_to_bone_index.getptr(p_name);
	if (bone_index_ptr != nullptr) {
		ERR_FAIL_COND_MSG(*bone_index_ptr != p_bone,
			"Skeleton3D: '" + get_name() + "', bone name:  '" + p_name + "' already exists.");
		return; // No need to rename, the bone already has the given name.
	}

	name_to_bone_index.erase(bones[p_bone].name);
	bones[p_bone].name = p_name;
	name_to_bone_index.insert(p_name, p_bone);

	version++;
}

bool Skeleton3D::is_bone_parent_of(int p_bone, int p_parent_bone_id) const
{
	int parent_of_bone = get_bone_parent(p_bone);

	if (-1 == parent_of_bone) {
		return false;
	}

	if (parent_of_bone == p_parent_bone_id) {
		return true;
	}

	return is_bone_parent_of(parent_of_bone, p_parent_bone_id);
}

int Skeleton3D::get_bone_count() const { return bones.size(); }

void Skeleton3D::set_bone_parent(int p_bone, int p_parent)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	ERR_FAIL_COND(p_parent != -1 && (p_parent < 0));
	ERR_FAIL_COND(p_bone == p_parent);

	bones[p_bone].parent = p_parent;
	process_order_dirty = true;
	rest_dirty = true;
	_make_dirty();
}

void Skeleton3D::unparent_bone_and_rest(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	int parent = bones[p_bone].parent;
	while (parent >= 0) {
		bones[p_bone].rest = bones[parent].rest * bones[p_bone].rest;
		parent = bones[parent].parent;
	}

	bones[p_bone].parent = -1;
	process_order_dirty = true;

	rest_dirty = true;
	_make_dirty();
}

int Skeleton3D::get_bone_parent(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, -1);
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].parent;
}

Vector<int> Skeleton3D::get_bone_children(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector<int>());
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].child_bones;
}

Vector<int> Skeleton3D::get_parentless_bones() const
{
	if (process_order_dirty) {
		_update_process_order();
	}
	return parentless_bones;
}

void Skeleton3D::set_bone_rest(int p_bone, const Transform3D& p_rest)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].rest = p_rest;
	rest_dirty = true;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

Transform3D Skeleton3D::get_bone_rest(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());

	return bones[p_bone].rest;
}

Transform3D Skeleton3D::get_bone_global_rest(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	if (rest_dirty) {
		_force_update_all_bone_transforms();
	}
	return bones[p_bone].global_rest;
}

bool Skeleton3D::is_bone_enabled(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, false);
	return bones[p_bone].enabled;
}

bool Skeleton3D::is_show_rest_only() const { return show_rest_only; }

void Skeleton3D::clear_bones()
{
	bones.clear();
	name_to_bone_index.clear();

	// All these structures contain references to now invalid bone indices.
	skin_bindings.clear();
	bone_global_pose_dirty.clear();
	parentless_bones.clear();
	nested_set_offset_to_bone_index.clear();

	process_order_dirty = true;
	version++;
	_make_dirty();
}

// Posing api

void Skeleton3D::set_bone_pose(int p_bone, const Transform3D& p_pose)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_position = p_pose.origin;
	bones[p_bone].pose_rotation = p_pose.basis.get_rotation_quaternion();
	bones[p_bone].pose_scale = p_pose.basis.get_scale();
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_position(int p_bone, const Vector3& p_position)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_position = p_position;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_rotation(int p_bone, const Quaternion& p_rotation)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_rotation = p_rotation;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_scale(int p_bone, const Vector3& p_scale)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_scale = p_scale;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

Vector3 Skeleton3D::get_bone_pose_position(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_position;
}

Quaternion Skeleton3D::get_bone_pose_rotation(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Quaternion());
	return bones[p_bone].pose_rotation;
}

Vector3 Skeleton3D::get_bone_pose_scale(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_scale;
}

void Skeleton3D::reset_bone_pose(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	set_bone_pose_position(p_bone, bones[p_bone].rest.origin);
	set_bone_pose_rotation(p_bone, bones[p_bone].rest.basis.get_rotation_quaternion());
	set_bone_pose_scale(p_bone, bones[p_bone].rest.basis.get_scale());
}

void Skeleton3D::reset_bone_poses()
{
	for (uint32_t i = 0; i < bones.size(); i++) {
		reset_bone_pose(i);
	}
}

Transform3D Skeleton3D::get_bone_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	bones[p_bone].update_pose_cache();
	return bones[p_bone].pose_cache;
}

void Skeleton3D::_update_deferred(UpdateFlag p_update_flag)
{
	if (is_inside_tree()) {
#ifdef TOOLS_ENABLED
		if (saving) {
			update_flags |= p_update_flag;
			_notification(NOTIFICATION_UPDATE_SKELETON);
			return;
		}
#endif // TOOLS_ENABLED
		if (update_flags == UPDATE_FLAG_NONE && !updating) {
			notify_deferred_thread_group(
				NOTIFICATION_UPDATE_SKELETON); // It must never be called more than once in a single
											   // frame.
		}
		update_flags |= p_update_flag;
	}
}

void Skeleton3D::localize_rests()
{
	Vector<int> bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		bones_to_process.erase(current_bone_idx);

		if (bones[current_bone_idx].parent >= 0) {
			set_bone_rest(
				current_bone_idx, bones[bones[current_bone_idx].parent].rest.affine_inverse() *
									  bones[current_bone_idx].rest);
		}

		// Add the bone's children to the list of bones to be processed.
		int child_bone_size = bones[current_bone_idx].child_bones.size();
		for (int i = 0; i < child_bone_size; i++) {
			bones_to_process.push_back(bones[current_bone_idx].child_bones[i]);
		}
	}
}

void Skeleton3D::_skin_changed() { _make_dirty(); }

Ref<Skin> Skeleton3D::create_skin_from_rest_transforms()
{
	Ref<Skin> skin;

	skin.instantiate();
	skin->set_bind_count(bones.size());

	// Pose changed, rebuild cache of inverses.
	const Bone* bonesptr = bones.ptr();
	uint32_t len = bones.size();

	// Calculate global rests and invert them.
	LocalVector<int> bones_to_process;
	bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		const Bone& b = bonesptr[current_bone_idx];
		bones_to_process.erase(current_bone_idx);
		LocalVector<int> child_bones_vector;
		child_bones_vector = get_bone_children(current_bone_idx);
		int child_bones_size = child_bones_vector.size();
		if (b.parent < 0) {
			skin->set_bind_pose(current_bone_idx, b.rest);
		}
		for (int i = 0; i < child_bones_size; i++) {
			int child_bone_idx = child_bones_vector[i];
			const Bone& cb = bonesptr[child_bone_idx];
			skin->set_bind_pose(child_bone_idx, skin->get_bind_pose(current_bone_idx) * cb.rest);
			// Add the bone's children to the list of bones to be processed.
			bones_to_process.push_back(child_bones_vector[i]);
		}
	}

	for (uint32_t i = 0; i < len; i++) {
		// The inverse is what is actually required.
		skin->set_bind_bone(i, i);
		skin->set_bind_pose(i, skin->get_bind_pose(i).affine_inverse());
	}

	return skin;
}

void Skeleton3D::force_update_deferred() { _make_dirty(); }

void Skeleton3D::force_update_all_dirty_bones() { _force_update_all_dirty_bones(); }

void Skeleton3D::_force_update_all_dirty_bones() const
{
	if (!dirty) {
		return;
	}
	_force_update_all_bone_transforms();
}

void Skeleton3D::force_update_all_bone_transforms() { _force_update_all_bone_transforms(); }

void Skeleton3D::force_update_bone_children_transforms(int p_bone_idx)
{
	_force_update_bone_children_transforms(p_bone_idx);
}

void Skeleton3D::_force_update_bone_children_transforms(int p_bone_idx) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone_idx, bone_size);

	_update_process_order();

	Bone* bonesptr = bones.ptr();

	// Loop through nested set.
	for (int offset = 0; offset < bone_size; offset++) {
		if (rest_dirty) {
			int current_bone_idx = nested_set_offset_to_bone_index[offset];
			Bone& b = bonesptr[current_bone_idx];
			b.global_rest = b.parent >= 0 ? bonesptr[b.parent].global_rest * b.rest
										  : b.rest; // Rest needs update apert from pose.
		}

		if (!bone_global_pose_dirty[offset]) {
			continue;
		}

		int current_bone_idx = nested_set_offset_to_bone_index[offset];
		Bone& b = bonesptr[current_bone_idx];
		bool bone_enabled = b.enabled && !show_rest_only;

		if (bone_enabled) {
			b.update_pose_cache();
			Transform3D pose = b.pose_cache;

			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * pose;
			}
			else {
				b.global_pose = pose;
			}
		}
		else {
			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * b.rest;
			}
			else {
				b.global_pose = b.rest;
			}
		}

#ifndef DISABLE_DEPRECATED
		if (bone_enabled) {
			Transform3D pose = b.pose_cache;
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * pose;
			}
			else {
				b.pose_global_no_override = pose;
			}
		}
		else {
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * b.rest;
			}
			else {
				b.pose_global_no_override = b.rest;
			}
		}
		if (b.global_pose_override_amount >= CMP_EPSILON) {
			b.global_pose = b.global_pose.interpolate_with(
				b.global_pose_override, b.global_pose_override_amount);
		}
		if (b.global_pose_override_reset) {
			b.global_pose_override_amount = 0.0;
		}
#endif // _DISABLE_DEPRECATED

		bone_global_pose_dirty[offset] = false;
	}
}

#ifndef DISABLE_DEPRECATED
void Skeleton3D::clear_bones_global_pose_override()
{
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		bones[i].global_pose_override_amount = 0;
		bones[i].global_pose_override_reset = true;
	}
	_make_dirty();
	_make_bone_global_poses_dirty();
}

void Skeleton3D::set_bone_global_pose_override(
	int p_bone, const Transform3D& p_pose, real_t p_amount, bool p_persistent)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose_override_amount = p_amount;
	bones[p_bone].global_pose_override = p_pose;
	bones[p_bone].global_pose_override_reset = !p_persistent;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

Transform3D Skeleton3D::get_bone_global_pose_override(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose_override;
}

Transform3D Skeleton3D::get_bone_global_pose_no_override(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_force_update_all_dirty_bones();
	return bones[p_bone].pose_global_no_override;
}

#ifndef PHYSICS_3D_DISABLED
Node* Skeleton3D::get_simulator() { return simulator; }

bool Skeleton3D::get_animate_physical_bones() const { return animate_physical_bones; }

#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

Skeleton3D::Skeleton3D() {}

Skeleton3D::~Skeleton3D()
{
	// Some skins may remain bound.
	for (SkinReference* E : skin_bindings) {
		E->skeleton_node = nullptr;
	}
}

void Skeleton3D::_update_process_order() const {}

void Skeleton3D::_force_update_all_bone_transforms() const {}

void Skeleton3D::_notification(int) {}

int Skeleton3D::add_bone(String const&) { return 0; }

Ref<SkinReference> Skeleton3D::register_skin(Ref<Skin> const&)
{
	Ref<SkinReference> sr = memnew(SkinReference);
	return sr;
}

void Skeleton3D::add_child_notify(Node* p_child) {}

void Skeleton3D::remove_child_notify(Node* p_child) {}

void Skeleton3D::move_child_notify(Node* p_child) {}

// ==========================================================================
// Merged from scene/3d/skeleton_modifier_3d.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "skeleton_modifier_3d.h"

bool SkeletonModifier3D::should_check_node_path()
{
	return (bool)GLOBAL_GET_CACHED(
			   bool, "animation/warnings/check_invalid_skeleton_modifier_node_paths") &&
		   is_inside_tree();
}

bool SkeletonModifier3D::is_active() const { return active; }

void SkeletonModifier3D::_set_active(bool p_active) {}

void SkeletonModifier3D::set_influence(real_t p_influence) { influence = p_influence; }

PackedStringArray SkeletonModifier3D::get_configuration_warnings() const
{
	return PackedStringArray();
}

Skeleton3D* SkeletonModifier3D::get_skeleton() const
{
	Skeleton3D s = Skeleton3D();
	return &s;
}

real_t SkeletonModifier3D::get_influence() const { return influence; }

Vector3 SkeletonModifier3D::get_vector_from_bone_axis(BoneAxis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X: {
		ret = Vector3(1, 0, 0);
	} break;
	case BONE_AXIS_MINUS_X: {
		ret = Vector3(-1, 0, 0);
	} break;
	case BONE_AXIS_PLUS_Y: {
		ret = Vector3(0, 1, 0);
	} break;
	case BONE_AXIS_MINUS_Y: {
		ret = Vector3(0, -1, 0);
	} break;
	case BONE_AXIS_PLUS_Z: {
		ret = Vector3(0, 0, 1);
	} break;
	case BONE_AXIS_MINUS_Z: {
		ret = Vector3(0, 0, -1);
	} break;
	}
	return ret;
}

Vector3 SkeletonModifier3D::get_vector_from_axis(Vector3::Axis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case Vector3::AXIS_X: {
		ret = Vector3(1, 0, 0);
	} break;
	case Vector3::AXIS_Y: {
		ret = Vector3(0, 1, 0);
	} break;
	case Vector3::AXIS_Z: {
		ret = Vector3(0, 0, 1);
	} break;
	}
	return ret;
}

Vector3::Axis SkeletonModifier3D::get_axis_from_bone_axis(BoneAxis p_axis)
{
	Vector3::Axis ret = Vector3::AXIS_X;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X:
	case BONE_AXIS_MINUS_X: {
		ret = Vector3::AXIS_X;
	} break;
	case BONE_AXIS_PLUS_Y:
	case BONE_AXIS_MINUS_Y: {
		ret = Vector3::AXIS_Y;
	} break;
	case BONE_AXIS_PLUS_Z:
	case BONE_AXIS_MINUS_Z: {
		ret = Vector3::AXIS_Z;
	} break;
	}
	return ret;
}

Vector3 SkeletonModifier3D::limit_length(
	const Vector3& p_origin, const Vector3& p_destination, float p_length)
{
	return p_origin + (p_destination - p_origin).normalized() * p_length;
}

Quaternion SkeletonModifier3D::get_local_pose_rotation(
	Skeleton3D* p_skeleton, int p_bone, const Quaternion& p_global_pose_rotation)
{
	int parent = p_skeleton->get_bone_parent(p_bone);
	if (parent < 0) {
		return p_global_pose_rotation;
	}
	return (p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion().inverse() *
			p_global_pose_rotation)
		.normalized();
}

Quaternion SkeletonModifier3D::get_from_to_rotation(
	const Vector3& p_from, const Vector3& p_to, const Quaternion& p_prev_rot)
{
	if (Math::is_equal_approx((float)p_from.dot(p_to), -1.0f)) {
		return p_prev_rot; // For preventing to glitch, checking dot for detecting flip is more
						   // accurate than checking cross.
	}
	Vector3 axis = p_from.cross(p_to);
	if (axis.is_zero_approx()) {
		return p_prev_rot;
	}
	float angle = p_from.angle_to(p_to);
	if (Math::is_zero_approx(angle)) {
		angle = 0.0;
	}
	return Quaternion(axis.normalized(), angle);
}

Quaternion SkeletonModifier3D::get_from_to_rotation_by_axis(
	const Vector3& p_from, const Vector3& p_to, const Vector3& p_axis)
{
	const double ALMOST_ONE = 1.0 - CMP_EPSILON;
	double dot = p_from.dot(p_to);
	if (dot > ALMOST_ONE) {
		return Quaternion();
	}
	if (dot < -ALMOST_ONE) {
		return Quaternion(p_axis, Math::PI);
	}
	double angle = p_from.angle_to(p_to);
	Vector3 cross = p_from.cross(p_to);
	if (std::signbit(cross.dot(p_axis))) {
		angle = -angle;
	}
	return Quaternion(p_axis, angle);
}

Quaternion SkeletonModifier3D::get_swing(const Quaternion& p_rotation, const Vector3& p_axis)
{
	if (p_axis.is_zero_approx()) {
		return p_rotation;
	}
	Quaternion rot = p_rotation;
	if (!rot.is_normalized()) {
		rot.normalize();
	}
	Vector3 axis = p_axis.normalized();
	const Vector3 v(rot.x, rot.y, rot.z);
	const real_t proj_len = v.dot(axis);
	const Vector3 twist_vec = axis * proj_len;
	Quaternion twist(twist_vec.x, twist_vec.y, twist_vec.z, rot.w);
	if (!twist.is_normalized()) {
		if (Math::is_zero_approx(twist.length_squared())) {
			return rot;
		}
		twist.normalize();
	}
	Quaternion swing = rot * twist.inverse();
	swing.normalize();
	return swing;
}

Vector3 SkeletonModifier3D::snap_vector_to_plane(
	const Vector3& p_plane_normal, const Vector3& p_vector)
{
	if (Math::is_zero_approx(p_plane_normal.length_squared())) {
		return p_vector;
	}
	double length = p_vector.length();
	Vector3 normalized_vec = p_vector.normalized();
	Vector3 normal = p_plane_normal.normalized();
	return normalized_vec.slide(normal) * length;
}

double SkeletonModifier3D::symmetrize_angle(double p_angle)
{
	double angle = Math::fposmod(p_angle, Math::TAU);
	return angle > Math::PI ? angle - Math::TAU : angle;
}

double SkeletonModifier3D::get_roll_angle(const Quaternion& p_rotation, const Vector3& p_roll_axis)
{
	// Ensure roll axis is normalized.
	Vector3 roll_axis = p_roll_axis.normalized();

	// Project the quaternion rotation onto the roll axis.
	// This gives us the component of rotation around that axis.
	double dot =
		p_rotation.x * roll_axis.x + p_rotation.y * roll_axis.y + p_rotation.z * roll_axis.z;

	// Create a quaternion representing just the roll component.
	Quaternion roll_component;
	roll_component.x = roll_axis.x * dot;
	roll_component.y = roll_axis.y * dot;
	roll_component.z = roll_axis.z * dot;
	roll_component.w = p_rotation.w;

	// Normalize this component.
	double length = roll_component.length();
	if (length > CMP_EPSILON) {
		roll_component = roll_component / length;
	}
	else {
		return 0.0;
	}

	// Extract the angle.
	double angle = 2.0 * Math::acos(CLAMP(roll_component.w, -1.0, 1.0));

	// Determine the sign.
	double direction = (roll_component.x * roll_axis.x + roll_component.y * roll_axis.y +
							   roll_component.z * roll_axis.z >
						   0)
						   ? 1.0
						   : -1.0;

	return symmetrize_angle(angle * direction);
}

Vector3 SkeletonModifier3D::get_projected_normal(
	const Vector3& p_a, const Vector3& p_b, const Vector3& p_point)
{
	// Get nearest normal vector to p_point from the infinite line p_a to p_b.
	const Vector3 dir = p_b - p_a;
	const real_t denom = dir.length_squared();
	if (Math::is_zero_approx(denom)) {
		return Vector3();
	}
	const Vector3 w = p_point - p_a;
	const real_t t = w.dot(dir) / denom;
	const Vector3 h = p_a + dir * t;
	return (p_point - h).normalized();
}

SkeletonModifier3D::SkeletonModifier3D() {}

void SkeletonModifier3D::_skeleton_changed(Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

void SkeletonModifier3D::_validate_bone_names() {}

void SkeletonModifier3D::_process_modification(double p_delta) {}



void SkeletonModifier3D::process_modification(double) {}

// ==========================================================================
// Merged from scene/3d/bone_attachment_3d.cpp
// ==========================================================================

// --- bone_attachment_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "bone_attachment_3d.h"

void BoneAttachment3D::_on_bone_pose_update_bind_compat_90575(int p_bone_index)
{
	return on_skeleton_update();
}

void BoneAttachment3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "bone_attachment_3d.h"
#include "core/config/engine.h"

void BoneAttachment3D::_transform_changed()
{
	if (!is_inside_tree()) {
		return;
	}

	if (override_pose && !overriding) {
		Skeleton3D* sk = get_skeleton();

		ERR_FAIL_NULL_MSG(sk, "Cannot override pose: Skeleton not found!");
		ERR_FAIL_INDEX_MSG(
			bone_idx, sk->get_bone_count(), "Cannot override pose: Bone index is out of range!");

		Transform3D our_trans = get_transform();
		if (use_external_skeleton) {
			if (!sk->is_inside_tree()) {
				return;
			}
			our_trans = sk->get_global_transform().affine_inverse() * get_global_transform();
		}

		overriding = true;
		sk->set_bone_global_pose(bone_idx, our_trans);
		sk->force_update_all_dirty_bones();
	}
	overriding = false;
}

void BoneAttachment3D::set_bone_name(const String& p_name)
{
	bone_name = p_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_bone_idx(sk->find_bone(bone_name));
	}
}

String BoneAttachment3D::get_bone_name() const { return bone_name; }

int BoneAttachment3D::get_bone_idx() const { return bone_idx; }

bool BoneAttachment3D::get_override_pose() const { return override_pose; }

bool BoneAttachment3D::get_use_external_skeleton() const { return use_external_skeleton; }

NodePath BoneAttachment3D::get_external_skeleton() const { return external_skeleton_node; }

void BoneAttachment3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (use_external_skeleton) {
			_update_external_skeleton_cache();
		}
		_check_bind();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_check_unbind();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_transform_changed();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (_override_dirty) {
			_override_dirty = false;
		}
	} break;
	}
}

void BoneAttachment3D::on_skeleton_update()
{
	if (updating) {
		return;
	}
	updating = true;
	if (bone_idx >= 0) {
		Skeleton3D* sk = get_skeleton();
		if (sk) {
			if (!override_pose) {
				if (use_external_skeleton) {
					if (sk->is_inside_tree()) {
						set_global_transform(
							sk->get_global_transform() * sk->get_bone_global_pose(bone_idx));
						// Else, do nothing, the transform will be set when the skeleton enters the
						// tree: Skeleton3D::_notification(NOTIFICATION_ENTER_TREE) -> calls
						// Skeleton3D::_notification(NOTIFICATION_UPDATE_SKELETON)
						// -> emits skeleton_updated signal -> connected to
						// BoneAttachment3D::on_skeleton_update()
					}
				}
				else {
					set_transform(sk->get_bone_global_pose(bone_idx));
				}
			}
			else {
				if (!_override_dirty) {
					_transform_changed();
					_override_dirty = true;
				}
			}
		}
	}
	updating = false;
}

#ifdef TOOLS_ENABLED

void BoneAttachment3D::notify_rebind_required()
{
	// Ensures bindings are properly updated after a scene reload.
	_check_unbind();
	if (use_external_skeleton) {
		_update_external_skeleton_cache();
	}
	bone_idx = -1;
	_check_bind();
}
#endif // TOOLS_ENABLED

Skeleton3D* BoneAttachment3D::get_skeleton() {}

void BoneAttachment3D::_check_bind() {}

void BoneAttachment3D::_update_external_skeleton_cache() {}

void BoneAttachment3D::_check_unbind() {}

void BoneAttachment3D::set_bone_idx(int const&) {}

PackedStringArray BoneAttachment3D::get_configuration_warnings() const
{
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/look_at_modifier_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "look_at_modifier_3d.h"
#include "scene/resources/animation.h"

PackedStringArray LookAtModifier3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (get_axis_from_bone_axis(forward_axis) == primary_rotation_axis) {
		warnings.push_back(RTR("Forward axis and primary rotation axis must not be parallel."));
	}
	return warnings;
}

String LookAtModifier3D::get_bone_name() const { return bone_name; }

int LookAtModifier3D::get_bone() const { return bone; }

SkeletonModifier3D::BoneAxis LookAtModifier3D::get_forward_axis() const { return forward_axis; }

Vector3::Axis LookAtModifier3D::get_primary_rotation_axis() const { return primary_rotation_axis; }

bool LookAtModifier3D::is_using_secondary_rotation() const { return use_secondary_rotation; }

void LookAtModifier3D::set_relative(bool p_enabled) { relative = p_enabled; }

bool LookAtModifier3D::is_relative() const { return relative; }

NodePath LookAtModifier3D::get_target_node() const { return target_node; }

LookAtModifier3D::OriginFrom LookAtModifier3D::get_origin_from() const { return origin_from; }

String LookAtModifier3D::get_origin_bone_name() const { return origin_bone_name; }

int LookAtModifier3D::get_origin_bone() const { return origin_bone; }

NodePath LookAtModifier3D::get_origin_external_node() const { return origin_external_node; }

void LookAtModifier3D::set_origin_offset(const Vector3& p_offset) { origin_offset = p_offset; }

Vector3 LookAtModifier3D::get_origin_offset() const { return origin_offset; }

void LookAtModifier3D::set_origin_safe_margin(float p_margin) { origin_safe_margin = p_margin; }

float LookAtModifier3D::get_origin_safe_margin() const { return origin_safe_margin; }

void LookAtModifier3D::set_duration(float p_duration)
{
	duration = p_duration;
	if (Math::is_zero_approx(p_duration)) {
		time_step = 0;
		remaining = 0;
	}
	else {
		time_step = 1.0 / p_duration; // Cache to avoid division.
	}
}

float LookAtModifier3D::get_duration() const { return duration; }

void LookAtModifier3D::set_transition_type(Tween::TransitionType p_transition_type)
{
	transition_type = p_transition_type;
}

Tween::TransitionType LookAtModifier3D::get_transition_type() const { return transition_type; }

void LookAtModifier3D::set_ease_type(Tween::EaseType p_ease_type) { ease_type = p_ease_type; }

Tween::EaseType LookAtModifier3D::get_ease_type() const { return ease_type; }

bool LookAtModifier3D::is_using_angle_limitation() const { return use_angle_limitation; }

bool LookAtModifier3D::is_limitation_symmetry() const { return symmetry_limitation; }

void LookAtModifier3D::set_primary_limit_angle(float p_angle) { primary_limit_angle = p_angle; }

float LookAtModifier3D::get_primary_limit_angle() const { return primary_limit_angle; }

void LookAtModifier3D::set_primary_damp_threshold(float p_power)
{
	primary_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_damp_threshold() const { return primary_damp_threshold; }

void LookAtModifier3D::set_primary_positive_limit_angle(float p_angle)
{
	primary_positive_limit_angle = p_angle;
}

float LookAtModifier3D::get_primary_positive_limit_angle() const
{
	return primary_positive_limit_angle;
}

void LookAtModifier3D::set_primary_positive_damp_threshold(float p_power)
{
	primary_positive_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_positive_damp_threshold() const
{
	return primary_positive_damp_threshold;
}

void LookAtModifier3D::set_primary_negative_limit_angle(float p_angle)
{
	primary_negative_limit_angle = p_angle;
}

float LookAtModifier3D::get_primary_negative_limit_angle() const
{
	return primary_negative_limit_angle;
}

void LookAtModifier3D::set_primary_negative_damp_threshold(float p_power)
{
	primary_negative_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_negative_damp_threshold() const
{
	return primary_negative_damp_threshold;
}

void LookAtModifier3D::set_secondary_limit_angle(float p_angle) { secondary_limit_angle = p_angle; }

float LookAtModifier3D::get_secondary_limit_angle() const { return secondary_limit_angle; }

void LookAtModifier3D::set_secondary_damp_threshold(float p_power)
{
	secondary_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_damp_threshold() const { return secondary_damp_threshold; }

void LookAtModifier3D::set_secondary_positive_limit_angle(float p_angle)
{
	secondary_positive_limit_angle = p_angle;
}

float LookAtModifier3D::get_secondary_positive_limit_angle() const
{
	return secondary_positive_limit_angle;
}

void LookAtModifier3D::set_secondary_positive_damp_threshold(float p_power)
{
	secondary_positive_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_positive_damp_threshold() const
{
	return secondary_positive_damp_threshold;
}

void LookAtModifier3D::set_secondary_negative_limit_angle(float p_angle)
{
	secondary_negative_limit_angle = p_angle;
}

float LookAtModifier3D::get_secondary_negative_limit_angle() const
{
	return secondary_negative_limit_angle;
}

void LookAtModifier3D::set_secondary_negative_damp_threshold(float p_power)
{
	secondary_negative_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_negative_damp_threshold() const
{
	return secondary_negative_damp_threshold;
}

bool LookAtModifier3D::is_target_within_limitation() const { return is_within_limitations; }

float LookAtModifier3D::get_interpolation_remaining() const { return remaining * duration; }

bool LookAtModifier3D::is_interpolating() const { return Math::is_zero_approx(remaining); }

bool LookAtModifier3D::is_intersecting_axis(const Vector3& p_prev, const Vector3& p_current,
	Vector3::Axis p_flipping_axis, Vector3::Axis p_check_axis, bool p_check_plane) const
{
	// Prevent that the angular velocity does not become too large.
	// Check that is p_flipping_axis flipped nearby p_check_axis (close than origin_safe_margin) or
	// not. If p_check_plane is true, check two axes of crossed plane.
	if (p_check_plane) {
		if (get_projection_vector(p_prev, p_check_axis).length() > origin_safe_margin &&
			get_projection_vector(p_current, p_check_axis).length() > origin_safe_margin) {
			return false;
		}
	}
	else if (Math::abs(p_prev[p_check_axis]) > origin_safe_margin &&
			   Math::abs(p_current[p_check_axis]) > origin_safe_margin) {
		return false;
	}

	return std::signbit(p_prev[p_flipping_axis]) != std::signbit(p_current[p_flipping_axis]);
}

Vector3 LookAtModifier3D::get_basis_vector_from_bone_axis(const Basis& p_basis, BoneAxis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X: {
		ret = p_basis.get_column(0);
	} break;
	case BONE_AXIS_MINUS_X: {
		ret = -p_basis.get_column(0);
	} break;
	case BONE_AXIS_PLUS_Y: {
		ret = p_basis.get_column(1);
	} break;
	case BONE_AXIS_MINUS_Y: {
		ret = -p_basis.get_column(1);
	} break;
	case BONE_AXIS_PLUS_Z: {
		ret = p_basis.get_column(2);
	} break;
	case BONE_AXIS_MINUS_Z: {
		ret = -p_basis.get_column(2);
	} break;
	}
	return ret;
}

Vector3::Axis LookAtModifier3D::get_secondary_rotation_axis(
	BoneAxis p_forward_axis, Vector3::Axis p_primary_rotation_axis)
{
	Vector3 secondary_plane =
		get_vector_from_bone_axis(p_forward_axis) + get_vector_from_axis(p_primary_rotation_axis);
	return Math::is_zero_approx(secondary_plane.x)
			   ? Vector3::AXIS_X
			   : (Math::is_zero_approx(secondary_plane.y) ? Vector3::AXIS_Y : Vector3::AXIS_Z);
}

Vector2 LookAtModifier3D::get_projection_vector(const Vector3& p_vector, Vector3::Axis p_axis)
{
	// NOTE: axis is swapped between 2D and 3D.
	Vector2 ret;
	switch (p_axis) {
	case Vector3::AXIS_X: {
		ret = Vector2(p_vector.z, p_vector.y);
	} break;
	case Vector3::AXIS_Y: {
		ret = Vector2(p_vector.x, p_vector.z);
	} break;
	case Vector3::AXIS_Z: {
		ret = Vector2(p_vector.y, p_vector.x);
	} break;
	}
	return ret;
}

float LookAtModifier3D::remap_damped(
	float p_from, float p_to, float p_damp_threshold, float p_value) const
{
	float sign = std::signbit(p_value) ? -1.0f : 1.0f;
	float abs_value = Math::abs(p_value);

	if (Math::is_equal_approx(p_damp_threshold, 1.0f) || Math::is_zero_approx(p_to)) {
		return sign * CLAMP(abs_value, p_from, p_to); // Avoid division by zero.
	}

	float value = Math::inverse_lerp(p_from, p_to, abs_value);

	if (value <= p_damp_threshold) {
		return sign * CLAMP(abs_value, p_from, p_to);
	}

	double limit = Math::PI;
	double inv_to = 1.0 / p_to;
	double end_x = limit * inv_to;
	double position = abs_value * inv_to;
	Vector2 start = Vector2(p_damp_threshold, p_damp_threshold);
	Vector2 mid = Vector2(1.0, 1.0);
	Vector2 end = Vector2(end_x, 1.0);
	value = get_bspline_y(start, mid, end, position);

	return sign * Math::lerp(p_from, p_to, value);
}

double LookAtModifier3D::get_bspline_y(
	const Vector2& p_from, const Vector2& p_control, const Vector2& p_to, double p_x) const
{
	double a = p_from.x - 2.0 * p_control.x + p_to.x;
	double b = -2.0 * p_from.x + 2.0 * p_control.x;
	double c = p_from.x - p_x;
	double t = 0.0;
	if (Math::is_zero_approx(a)) {
		t = -c / b; // Almost linear.
	}
	else {
		double discriminant = b * b - 4.0 * a * c;
		double sqrt_discriminant = Math::sqrt(discriminant);
		double e = 1.0 / (2.0 * a);
		double t1 = (-b + sqrt_discriminant) * e;
		t = (0.0 <= t1 && t1 <= 1.0) ? t1 : (-b - sqrt_discriminant) * e;
	}
	double u = 1.0 - t;
	double y = u * u * p_from.y + 2.0 * u * t * p_control.y + t * t * p_to.y;
	return y;
}

Transform3D LookAtModifier3D::look_at_with_axes(const Transform3D& p_rest)
{
	// Primary rotation by projection to 2D plane by xform_inv and picking elements.
	Vector3 current_vector =
		get_basis_vector_from_bone_axis(p_rest.basis, forward_axis).normalized();
	Vector2 src_vec2 =
		get_projection_vector(p_rest.basis.xform_inv(forward_vector_nrm), primary_rotation_axis)
			.normalized();
	Vector2 dst_vec2 =
		get_projection_vector(p_rest.basis.xform_inv(current_vector), primary_rotation_axis)
			.normalized();
	real_t calculated_angle = src_vec2.angle_to(dst_vec2);
	Transform3D primary_result
=
		p_rest.rotated_local(get_vector_from_axis(primary_rotation_axis), calculated_angle);
	Transform3D current_result =
		primary_result; // primary_result will be used by calculation of secondary rotation,
						// current_result is rotated by that.
	float limit_angle = 0.0;
	float damp_threshold = 0.0;

	if (use_angle_limitation) {
		if (symmetry_limitation) {
			limit_angle = primary_limit_angle * 0.5f;
			damp_threshold = primary_damp_threshold;
		}
		else {
			if (std::signbit(calculated_angle)) {
				limit_angle = primary_negative_limit_angle;
				damp_threshold = primary_negative_damp_threshold;
			}
			else {
				limit_angle = primary_positive_limit_angle;
				damp_threshold = primary_positive_damp_threshold;
			}
		}
		if (Math::abs(calculated_angle) > limit_angle) {
			is_within_limitations = false;
		}
		calculated_angle = remap_damped(0, limit_angle, damp_threshold, calculated_angle);
		current_result =
			p_rest.rotated_local(get_vector_from_axis(primary_rotation_axis), calculated_angle);
	}

	// Needs for detecting flipping even if use_secondary_rotation is false.
	secondary_rotation_axis = get_secondary_rotation_axis(forward_axis, primary_rotation_axis);

	if (!use_secondary_rotation) {
		return current_result;
	}

	// Secondary rotation by projection to 2D plane by xform_inv and picking elements.
	current_vector =
		get_basis_vector_from_bone_axis(primary_result.basis, forward_axis).normalized();
	src_vec2 = get_projection_vector(
		primary_result.basis.xform_inv(forward_vector_nrm), secondary_rotation_axis)
				   .normalized();
	dst_vec2 = get_projection_vector(
		primary_result.basis.xform_inv(current_vector), secondary_rotation_axis)
				   .normalized();
	calculated_angle = src_vec2.angle_to(dst_vec2);

	if (use_angle_limitation) {
		if (symmetry_limitation) {
			limit_angle = secondary_limit_angle * 0.5f;
			damp_threshold = secondary_damp_threshold;
		}
		else {
			if (std::signbit(calculated_angle)) {
				limit_angle = secondary_negative_limit_angle;
				damp_threshold = secondary_negative_damp_threshold;
			}
			else {
				limit_angle = secondary_positive_limit_angle;
				damp_threshold = secondary_positive_damp_threshold;
			}
		}
		if (Math::abs(calculated_angle) > limit_angle) {
			is_within_limitations = false;
		}
		calculated_angle = remap_damped(0, limit_angle, damp_threshold, calculated_angle);
	}
	current_result = current_result.rotated_local(
		get_vector_from_axis(secondary_rotation_axis), calculated_angle);

	return current_result;
}

void LookAtModifier3D::init_transition()
{
	if (Math::is_zero_approx(duration)) {
		return;
	}
	from_q = prev_q;
	remaining = 1.0;
}

void LookAtModifier3D::_process_modification(double p_delta) {}

// ==========================================================================
// Merged from scene/3d/retarget_modifier_3d.cpp
// ==========================================================================

#include "retarget_modifier_3d.h"

PackedStringArray RetargetModifier3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (child_skeletons.is_empty()) {
		warnings.push_back(RTR("There is no child Skeleton3D!"));
	}
	return warnings;
}

void RetargetModifier3D::_reset_child_skeletons()
{
	_reset_child_skeleton_poses();
	child_skeletons.clear();
}

void RetargetModifier3D::_set_active(bool p_active)
{
	if (!p_active) {
		_reset_child_skeleton_poses();
	}
}

void RetargetModifier3D::_process_modification(double p_delta)
{
	if (use_global_pose) {
		_retarget_global_pose();
	}
	else {
		_retarget_pose();
	}
}

void RetargetModifier3D::set_profile(Ref<SkeletonProfile> p_profile)
{
	if (profile == p_profile) {
		return;
	}
	_profile_changed(profile, p_profile);
}

Ref<SkeletonProfile> RetargetModifier3D::get_profile() const { return profile; }

bool RetargetModifier3D::is_using_global_pose() const { return use_global_pose; }

void RetargetModifier3D::set_enable_flags(uint32_t p_enable_flag)
{
	if (enable_flags != p_enable_flag) {
		_reset_child_skeleton_poses();
	}
	enable_flags = p_enable_flag;
}

uint32_t RetargetModifier3D::get_enable_flags() const { return enable_flags; }

void RetargetModifier3D::_reset_child_skeleton_poses() {}

void RetargetModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_update_child_skeletons();
	} break;
	case NOTIFICATION_EXIT_TREE: {
		_reset_child_skeletons();
	} break;
	}
}

RetargetModifier3D::RetargetModifier3D() {}

RetargetModifier3D::~RetargetModifier3D() {}

void RetargetModifier3D::_update_child_skeletons() {}

void RetargetModifier3D::_retarget_pose() {}

void RetargetModifier3D::_retarget_global_pose() {}

void RetargetModifier3D::_profile_changed(Ref<SkeletonProfile>, Ref<SkeletonProfile>) {}

void RetargetModifier3D::add_child_notify(Node* p_child) {}

void RetargetModifier3D::remove_child_notify(Node* p_child) {}

void RetargetModifier3D::move_child_notify(Node* p_child) {}

void RetargetModifier3D::_skeleton_changed(Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

// ==========================================================================
// Merged from scene/3d/spring_bone_collision_3d.cpp
// ==========================================================================

#include "spring_bone_collision_3d.h"

#include "core/config/engine.h"
#include "scene/3d/spring_bone_simulator_3d.h"

SpringBoneCollision3D::SpringBoneCollision3D(ShapeType p_shape_type) :
		shape_type(p_shape_type) {
}

void SpringBoneCollision3D::set_shape_type(ShapeType p_shape_type) {
	shape_type = p_shape_type;
}

void SpringBoneCollision3D::set_radius(float p_radius) {
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		height = radius * 2.0f;
	}
}

void SpringBoneCollision3D::set_height(float p_height) {
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
}

void SpringBoneCollision3D::set_mid_height(real_t p_mid_height) {
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "SpringBoneCollision3D capsule mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
}

void SpringBoneCollision3D::set_inside(bool p_enabled) {
	inside = p_enabled;
}

Pair<Vector3, Vector3> SpringBoneCollision3D::get_head_and_tail(const Transform3D &p_center) const {
	Transform3D tr = get_transform_from_skeleton(p_center);
	return Pair<Vector3, Vector3>(
			tr.origin + tr.basis.xform(Vector3::UP * (height * 0.5f - radius)),
			tr.origin + tr.basis.xform(Vector3::DOWN * (height * 0.5f - radius)));
}

void SpringBoneCollision3D::_validate_bone_name() {
	if (!bone_name.is_empty()) {
		set_bone_name(bone_name);
	} else if (bone != -1) {
		set_bone(bone);
	}
}

Skeleton3D *SpringBoneCollision3D::get_skeleton() const {
	SpringBoneSimulator3D *sim = dynamic_cast<SpringBoneSimulator3D *>(get_parent());
	if (sim) {
		return sim->get_skeleton();
	}
	return nullptr;
}

void SpringBoneCollision3D::set_bone_name(const String &p_name) {
	bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_bone(sk->find_bone(bone_name));
	}
}

String SpringBoneCollision3D::get_bone_name() const {
	return bone_name;
}

void SpringBoneCollision3D::set_bone(int p_bone) {
	bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && bone >= 0 && bone < sk->get_bone_count()) {
		bone_name = sk->get_bone_name(bone);
	}
	sync_pose();
}

int SpringBoneCollision3D::get_bone() const {
	return bone;
}

void SpringBoneCollision3D::set_position_offset(const Vector3 &p_offset) {
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	sync_pose();
}

Vector3 SpringBoneCollision3D::get_position_offset() const {
	return position_offset;
}

void SpringBoneCollision3D::set_rotation_offset(const Quaternion &p_offset) {
	if (rotation_offset == p_offset) {
		return;
	}
	rotation_offset = p_offset;
	sync_pose();
}

Quaternion SpringBoneCollision3D::get_rotation_offset() const {
	return rotation_offset;
}

void SpringBoneCollision3D::sync_pose() {
	if (bone >= 0) {
		Skeleton3D *sk = get_skeleton();
		if (sk) {
			Transform3D tr = sk->get_global_transform() * sk->get_bone_global_pose(bone);
			tr.origin += tr.basis.get_rotation_quaternion().xform(position_offset);
			tr.basis *= Basis(rotation_offset);
			set_global_transform(tr);
		}
	}
}

Transform3D SpringBoneCollision3D::get_transform_from_skeleton(const Transform3D &p_center) const {
	Transform3D gtr = get_global_transform();
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		Transform3D tr = sk->get_global_transform();
		gtr = tr.affine_inverse() * p_center * gtr;
	}
	return gtr;
}

void SpringBoneCollision3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_PARENTED: {
			_validate_bone_name();
		} break;
	}
}

Vector3 SpringBoneCollision3D::_collide_sphere(const Vector3 &p_origin, float p_radius, bool p_inside, float p_bone_radius, float p_bone_length, const Vector3 &p_current) {
	Vector3 diff = p_current - p_origin;
	float length = diff.length();
	float r = p_inside ? p_radius - p_bone_radius : p_bone_radius + p_radius;
	float distance = p_inside ? r - length : length - r;
	if (distance > 0) {
		return p_current;
	}
	return p_origin + diff.normalized() * r;
}

Vector3 SpringBoneCollision3D::collide(const Transform3D &p_center, float p_bone_radius, float p_bone_length, const Vector3 &p_current) const {
	switch (shape_type) {
		case SHAPE_SPHERE: {
			return _collide_sphere(get_transform_from_skeleton(p_center).origin, radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_CAPSULE: {
			Pair<Vector3, Vector3> head_tail = get_head_and_tail(p_center);
			Vector3 head = head_tail.first;
			Vector3 tail = head_tail.second;
			Vector3 p = tail - head;
			Vector3 q = p_current - head;
			float dot = p.dot(q);
			if (dot <= 0) {
				return _collide_sphere(head, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			float pls = p.length_squared();
			if (Math::is_zero_approx(pls)) {
				return p_current;
			}
			if (pls <= dot) {
				return _collide_sphere(head + p, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			return _collide_sphere(head + p * (dot / pls), radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_PLANE: {
			Transform3D tr = get_transform_from_skeleton(p_center);
			Vector3 pos = tr.origin;
			Vector3 normal = tr.basis.get_rotation_quaternion().xform(Vector3::UP);
			Vector3 to_vec = p_current - pos;
			float distance = to_vec.dot(normal) - p_bone_radius;
			if (distance > 0) {
				return p_current;
			}
			return p_current + normal * -distance;
		}
	}
	return p_current;
}

PackedStringArray SpringBoneCollision3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!dynamic_cast<SpringBoneSimulator3D *>(get_parent())) {
		warnings.push_back(RTR("SpringBoneCollision3D must be a child of SpringBoneSimulator3D."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/spring_bone_simulator_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/spring_bone_collision_3d.h"
// --- spring_bone_simulator_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "spring_bone_simulator_3d.h"

namespace compat::SpringBoneSimulator3D
{
enum BoneDirection : int
{
	BONE_DIRECTION_PLUS_X = SkeletonModifier3D::BONE_DIRECTION_PLUS_X,
	BONE_DIRECTION_MINUS_X = SkeletonModifier3D::BONE_DIRECTION_MINUS_X,
	BONE_DIRECTION_PLUS_Y = SkeletonModifier3D::BONE_DIRECTION_PLUS_Y,
	BONE_DIRECTION_MINUS_Y = SkeletonModifier3D::BONE_DIRECTION_MINUS_Y,
	BONE_DIRECTION_PLUS_Z = SkeletonModifier3D::BONE_DIRECTION_PLUS_Z,
	BONE_DIRECTION_MINUS_Z = SkeletonModifier3D::BONE_DIRECTION_MINUS_Z,
	BONE_DIRECTION_FROM_PARENT = SkeletonModifier3D::BONE_DIRECTION_FROM_PARENT,
};

enum RotationAxis : int
{
	ROTATION_AXIS_X = SkeletonModifier3D::ROTATION_AXIS_X,
	ROTATION_AXIS_Y = SkeletonModifier3D::ROTATION_AXIS_Y,
	ROTATION_AXIS_Z = SkeletonModifier3D::ROTATION_AXIS_Z,
	ROTATION_AXIS_ALL = SkeletonModifier3D::ROTATION_AXIS_ALL,
	ROTATION_AXIS_CUSTOM = SkeletonModifier3D::ROTATION_AXIS_CUSTOM,
};
} // namespace compat::SpringBoneSimulator3D

compat::SpringBoneSimulator3D::BoneDirection
SpringBoneSimulator3D::_get_end_bone_direction_bind_compat_110120(int p_index) const
{
	return static_cast<compat::SpringBoneSimulator3D::BoneDirection>(
		(int)get_end_bone_direction(p_index));
}

void SpringBoneSimulator3D::_set_end_bone_direction_bind_compat_110120(
	int p_index, compat::SpringBoneSimulator3D::BoneDirection p_bone_direction)
{
	set_end_bone_direction(
		p_index, static_cast<SkeletonModifier3D::BoneDirection>((int)p_bone_direction));
}

compat::SpringBoneSimulator3D::RotationAxis
SpringBoneSimulator3D::_get_rotation_axis_bind_compat_110120(int p_index) const
{
	return static_cast<compat::SpringBoneSimulator3D::RotationAxis>(
		(int)get_rotation_axis(p_index));
}

void SpringBoneSimulator3D::_set_rotation_axis_bind_compat_110120(
	int p_index, compat::SpringBoneSimulator3D::RotationAxis p_axis)
{
	set_rotation_axis(p_index, static_cast<SkeletonModifier3D::RotationAxis>((int)p_axis));
}

compat::SpringBoneSimulator3D::RotationAxis
SpringBoneSimulator3D::_get_joint_rotation_axis_bind_compat_110120(int p_index, int p_joint) const
{
	return static_cast<compat::SpringBoneSimulator3D::RotationAxis>(
		(int)get_joint_rotation_axis(p_index, p_joint));
}

void SpringBoneSimulator3D::_set_joint_rotation_axis_bind_compat_110120(
	int p_index, int p_joint, compat::SpringBoneSimulator3D::RotationAxis p_axis)
{
	set_joint_rotation_axis(
		p_index, p_joint, static_cast<SkeletonModifier3D::RotationAxis>((int)p_axis));
}

void SpringBoneSimulator3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "spring_bone_simulator_3d.h"

// Original VRM Spring Bone movement logic was distributed by (c) VRM Consortium. Licensed under the
// MIT license.

void SpringBoneSimulator3D::_notification(int p_what)
{
	switch (p_what) {
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

String SpringBoneSimulator3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone_name;
}

int SpringBoneSimulator3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone;
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

String SpringBoneSimulator3D::get_center_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->center_bone_name;
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

void SpringBoneSimulator3D::_make_collisions_dirty() { collisions_dirty = true; }

SpringBoneSimulator3D::~SpringBoneSimulator3D() { clear_settings(); }

void SpringBoneSimulator3D::_validate_bone_names() {}

void SpringBoneSimulator3D::_make_joints_dirty(int p_index, bool p_reset) {}

void SpringBoneSimulator3D::_make_gizmo_dirty() {}

void SpringBoneSimulator3D::set_setting_count(int) {}

void SpringBoneSimulator3D::set_rotation_axis(int, SkeletonModifier3D::RotationAxis) {}

void SpringBoneSimulator3D::set_joint_rotation_axis(int, int, SkeletonModifier3D::RotationAxis) {}

void SpringBoneSimulator3D::set_exclude_collision_count(int, int) {}

void SpringBoneSimulator3D::set_collision_count(int, int) {}

void SpringBoneSimulator3D::add_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::remove_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::move_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::_skeleton_changed(
	Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

void SpringBoneSimulator3D::_process_modification(double p_delta) {}
