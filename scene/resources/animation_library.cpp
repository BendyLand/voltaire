/**************************************************************************/
/*  animation_library.cpp                                                 */
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

#include "animation_library.h"
#include "scene/scene_string_names.h"

bool AnimationLibrary::is_valid_animation_name(const String& p_name)
{
	return !(p_name.is_empty() || p_name.contains_char('/') || p_name.contains_char(':') ||
			 p_name.contains_char(',') || p_name.contains_char('['));
}

bool AnimationLibrary::is_valid_library_name(const String& p_name)
{
	return !(p_name.contains_char('/') || p_name.contains_char(':') || p_name.contains_char(',') ||
			 p_name.contains_char('['));
}

String AnimationLibrary::validate_library_name(const String& p_name)
{
	return p_name.replace_chars("/:,[", '_');
}

bool AnimationLibrary::has_animation(const StringName& p_name) const
{
	return animations.has(p_name);
}

Ref<Animation> AnimationLibrary::get_animation(const StringName& p_name) const
{
	ERR_FAIL_COND_V_MSG(
		!animations.has(p_name), Ref<Animation>(), vformat("Animation not found: \"%s\".", p_name));

	return animations[p_name];
}

void AnimationLibrary::get_animation_list(LocalVector<StringName>* p_animations) const
{
	LocalVector<StringName> anims;

	for (const KeyValue<StringName, Ref<Animation>>& E : animations) {
		anims.push_back(E.key);
	}

	anims.sort_custom<StringName::AlphCompare>();

	for (const StringName& E : anims) {
		p_animations->push_back(E);
	}
}

int AnimationLibrary::get_animation_list_size() const { return animations.size(); }

AnimationLibrary::AnimationLibrary() {}


