/**************************************************************************/
/*  bone_attachment_3d.cpp                                                */
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

#include "bone_attachment_3d.compat.inc"
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

BoneAttachment3D::BoneAttachment3D()
{
	set_physics_interpolation_mode(PHYSICS_INTERPOLATION_MODE_OFF);
}


