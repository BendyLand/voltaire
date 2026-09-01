/**************************************************************************/
/*  bone_map.cpp                                                          */
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

#include "bone_map.h"
#include "core/config/engine.h"

Ref<SkeletonProfile> BoneMap::get_profile() const { return profile; }

StringName BoneMap::get_skeleton_bone_name(const StringName& p_profile_bone_name) const
{
	ERR_FAIL_COND_V(!bone_map.has(p_profile_bone_name), StringName());
	return bone_map.get(p_profile_bone_name);
}

void BoneMap::_set_skeleton_bone_name(
	const StringName& p_profile_bone_name, const StringName& p_skeleton_bone_name)
{
	ERR_FAIL_COND(!bone_map.has(p_profile_bone_name));
	bone_map.insert(p_profile_bone_name, p_skeleton_bone_name);
}

StringName BoneMap::find_profile_bone_name(const StringName& p_skeleton_bone_name) const
{
	StringName profile_bone_name;
	HashMap<StringName, StringName>::ConstIterator E = bone_map.begin();
	while (E) {
		if (E->value == p_skeleton_bone_name) {
			profile_bone_name = E->key;
			break;
		}
		++E;
	}
	return profile_bone_name;
}

int BoneMap::get_skeleton_bone_name_count(const StringName& p_skeleton_bone_name) const
{
	int count = 0;
	HashMap<StringName, StringName>::ConstIterator E = bone_map.begin();
	while (E) {
		if (E->value == p_skeleton_bone_name) {
			++count;
		}
		++E;
	}
	return count;
}

void BoneMap::_validate_bone_map()
{
	Ref<SkeletonProfile> current_profile = get_profile();
	if (current_profile.is_valid()) {
		// Insert missing profile bones into bone map.
		int len = current_profile->get_bone_size();
		StringName profile_bone_name;
		for (int i = 0; i < len; i++) {
			profile_bone_name = current_profile->get_bone_name(i);
			if (!bone_map.has(profile_bone_name)) {
				bone_map.insert(profile_bone_name, StringName());
			}
		}
		// Remove bones that do not exist in the profile from the map.
		Vector<StringName> delete_bones;
		StringName k;
		HashMap<StringName, StringName>::ConstIterator E = bone_map.begin();
		while (E) {
			k = E->key;
			if (!current_profile->has_bone(k)) {
				delete_bones.push_back(k);
			}
			++E;
		}
		len = delete_bones.size();
		for (int i = 0; i < len; i++) {
			bone_map.erase(delete_bones[i]);
		}
	}
	else {
		bone_map.clear();
	}
}

BoneMap::BoneMap() { _validate_bone_map(); }

BoneMap::~BoneMap() {}


