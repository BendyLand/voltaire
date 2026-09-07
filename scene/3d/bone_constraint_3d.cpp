/**************************************************************************/
/*  bone_constraint_3d.cpp                                                */
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

#include "bone_constraint_3d.h"

int BoneConstraint3D::get_setting_count() const { return (int)settings.size(); }

void BoneConstraint3D::_validate_setting(int p_index)
{
	settings[p_index] = memnew(BoneConstraint3DSetting);
}

void BoneConstraint3D::clear_settings() { set_setting_count(0); }

void BoneConstraint3D::set_amount(int p_index, float p_amount)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->amount = p_amount;
}

float BoneConstraint3D::get_amount(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0);
	return settings[p_index]->amount;
}

void BoneConstraint3D::set_apply_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->apply_bone_name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_apply_bone(p_index, sk->find_bone(settings[p_index]->apply_bone_name));
	}
}

String BoneConstraint3D::get_apply_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->apply_bone_name;
}

void BoneConstraint3D::set_apply_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->apply_bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (settings[p_index]->apply_bone <= -1 ||
			settings[p_index]->apply_bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + ": Apply bone index '" + itos(p_bone) +
						  "' is out of range!");
			settings[p_index]->apply_bone = -1;
		}
		else {
			settings[p_index]->apply_bone_name = sk->get_bone_name(settings[p_index]->apply_bone);
		}
	}
}

int BoneConstraint3D::get_apply_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->apply_bone;
}

BoneConstraint3D::ReferenceType BoneConstraint3D::get_reference_type(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), REFERENCE_TYPE_BONE);
	return settings[p_index]->reference_type;
}

void BoneConstraint3D::set_reference_bone_name(int p_index, const String& p_bone_name)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->reference_bone_name = p_bone_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_reference_bone(p_index, sk->find_bone(settings[p_index]->reference_bone_name));
	}
}

String BoneConstraint3D::get_reference_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->reference_bone_name;
}

void BoneConstraint3D::set_reference_bone(int p_index, int p_bone)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->reference_bone = p_bone;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		if (settings[p_index]->reference_bone <= -1 ||
			settings[p_index]->reference_bone >= sk->get_bone_count()) {
			WARN_PRINT_ED("Setting: " + itos(p_index) + ": Reference bone index '" + itos(p_bone) +
						  "' is out of range!");
			settings[p_index]->reference_bone = -1;
		}
		else {
			settings[p_index]->reference_bone_name =
				sk->get_bone_name(settings[p_index]->reference_bone);
		}
	}
}

int BoneConstraint3D::get_reference_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->reference_bone;
}

NodePath BoneConstraint3D::get_reference_node(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->reference_node;
}

void BoneConstraint3D::_validate_bone_names()
{
	for (int i = 0; i < (int)settings.size(); i++) {
		// Prior bone name.
		if (!settings[i]->apply_bone_name.is_empty()) {
			set_apply_bone_name(i, settings[i]->apply_bone_name);
		}
		else if (settings[i]->apply_bone != -1) {
			set_apply_bone(i, settings[i]->apply_bone);
		}
		// Prior bone name.
		if (!settings[i]->reference_bone_name.is_empty()) {
			set_reference_bone_name(i, settings[i]->reference_bone_name);
		}
		else if (settings[i]->reference_bone != -1) {
			set_reference_bone(i, settings[i]->reference_bone);
		}
	}
}

void BoneConstraint3D::_process_modification(double p_delta)
{
	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	for (int i = 0; i < (int)settings.size(); i++) {
		float amount = settings[i]->amount;
		if (amount <= 0) {
			continue;
		}

		int apply_bone = settings[i]->apply_bone;
		if (apply_bone < 0) {
			continue;
		}

		if (settings[i]->reference_type == REFERENCE_TYPE_BONE) {
			int reference_bone = settings[i]->reference_bone;
			if (reference_bone < 0) {
				continue;
			}
			_process_constraint_by_bone(i, skeleton, apply_bone, reference_bone, amount);
		}
		else {
			NodePath pt = settings[i]->reference_node;
			if (pt.is_empty()) {
				continue;
			}
			_process_constraint_by_node(i, skeleton, apply_bone, pt, amount);
		}
	}
}

void BoneConstraint3D::_process_constraint_by_bone(
	int p_index, Skeleton3D* p_skeleton, int p_apply_bone, int p_reference_bone, float p_amount)
{
	//
}

void BoneConstraint3D::_process_constraint_by_node(int p_index, Skeleton3D* p_skeleton,
	int p_apply_bone, const NodePath& p_reference_node, float p_amount)
{
	//
}

BoneConstraint3D::~BoneConstraint3D() { clear_settings(); }


