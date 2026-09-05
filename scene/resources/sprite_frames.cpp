/**************************************************************************/
/*  sprite_frames.cpp                                                     */
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

#include "scene/scene_string_names.h"
#include "sprite_frames.h"

void SpriteFrames::add_frame(
	const StringName& p_anim, const Ref<Texture2D>& p_texture, float p_duration, int p_at_pos)
{
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	p_duration = MAX(SPRITE_FRAME_MINIMUM_DURATION, p_duration);

	Frame frame = {p_texture, p_duration};

	if (p_at_pos >= 0 && p_at_pos < E->value.frames.size()) {
		E->value.frames.insert(p_at_pos, frame);
	}
	else {
		E->value.frames.push_back(frame);
	}

	emit_changed();
}

void SpriteFrames::set_frame(
	const StringName& p_anim, int p_idx, const Ref<Texture2D>& p_texture, float p_duration)
{
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	ERR_FAIL_COND(p_idx < 0);
	if (p_idx >= E->value.frames.size()) {
		return;
	}

	p_duration = MAX(SPRITE_FRAME_MINIMUM_DURATION, p_duration);

	Frame frame = {p_texture, p_duration};

	E->value.frames.write[p_idx] = frame;

	emit_changed();
}

int SpriteFrames::get_frame_count(const StringName& p_anim) const
{
	HashMap<StringName, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(!E, 0, "Animation '" + String(p_anim) + "' doesn't exist.");

	return E->value.frames.size();
}

void SpriteFrames::remove_frame(const StringName& p_anim, int p_idx)
{
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	E->value.frames.remove_at(p_idx);

	emit_changed();
}

void SpriteFrames::clear(const StringName& p_anim)
{
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	E->value.frames.clear();

	emit_changed();
}

void SpriteFrames::clear_all()
{
	animations.clear();
	add_animation(SceneStringName(default_));
}

void SpriteFrames::add_animation(const StringName& p_anim)
{
	ERR_FAIL_COND_MSG(
		animations.has(p_anim), "SpriteFrames already has animation '" + p_anim + "'.");

	animations[p_anim] = Anim();
}

bool SpriteFrames::has_animation(const StringName& p_anim) const { return animations.has(p_anim); }

void SpriteFrames::duplicate_animation(const StringName& p_from, const StringName& p_to)
{
	ERR_FAIL_COND_MSG(
		!animations.has(p_from), vformat("SpriteFrames doesn't have animation '%s'.", p_from));
	ERR_FAIL_COND_MSG(animations.has(p_to), vformat("Animation '%s' already exists.", p_to));
	animations[p_to] = animations[p_from];
}

void SpriteFrames::remove_animation(const StringName& p_anim) { animations.erase(p_anim); }

void SpriteFrames::rename_animation(const StringName& p_prev, const StringName& p_next)
{
	ERR_FAIL_COND_MSG(
		!animations.has(p_prev), "SpriteFrames doesn't have animation '" + String(p_prev) + "'.");
	ERR_FAIL_COND_MSG(animations.has(p_next), "Animation '" + String(p_next) + "' already exists.");

	Anim anim = animations[p_prev];
	animations.erase(p_prev);
	animations[p_next] = anim;
}

void SpriteFrames::get_animation_list(List<StringName>* r_animations) const
{
	for (const KeyValue<StringName, Anim>& E : animations) {
		r_animations->push_back(E.key);
	}
}

Vector<String> SpriteFrames::get_animation_names() const
{
	Vector<String> names;
	for (const KeyValue<StringName, Anim>& E : animations) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

void SpriteFrames::set_animation_speed(const StringName& p_anim, double p_fps)
{
	ERR_FAIL_COND_MSG(p_fps < 0, "Animation speed cannot be negative (" + itos(p_fps) + ").");
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	E->value.speed = p_fps;
}

double SpriteFrames::get_animation_speed(const StringName& p_anim) const
{
	HashMap<StringName, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(!E, 0, "Animation '" + String(p_anim) + "' doesn't exist.");
	return E->value.speed;
}

#ifndef DISABLE_DEPRECATED
void SpriteFrames::set_animation_loop(const StringName& p_anim, bool p_loop)
{
	set_animation_loop_mode(p_anim, p_loop ? LOOP_LINEAR : LOOP_NONE);
}

bool SpriteFrames::get_animation_loop(const StringName& p_anim) const
{
	return get_animation_loop_mode(p_anim) == LOOP_LINEAR;
}
#endif

void SpriteFrames::set_animation_loop_mode(const StringName& p_anim, LoopMode p_loop_mode)
{
	HashMap<StringName, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	E->value.loop = p_loop_mode;
}

SpriteFrames::LoopMode SpriteFrames::get_animation_loop_mode(const StringName& p_anim) const
{
	HashMap<StringName, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(
		!E, LoopMode::LOOP_NONE, "Animation '" + String(p_anim) + "' doesn't exist.");
	return E->value.loop;
}

Array SpriteFrames::_get_animations() const
{
	Array anims;

	List<StringName> sorted_names;
	get_animation_list(&sorted_names);
	sorted_names.sort_custom<StringName::AlphCompare>();

	for (const StringName& anim_name : sorted_names) {
		const Anim& anim = animations[anim_name];
		Dictionary d;
		d["name"] = anim_name;
		d["speed"] = anim.speed;
		d["loop"] = anim.loop;
		Array frames;
		for (int i = 0; i < anim.frames.size(); i++) {
			Dictionary f;
			f["texture"] = anim.frames[i].texture;
			f["duration"] = anim.frames[i].duration;
			frames.push_back(f);
		}
		d["frames"] = frames;
		anims.push_back(d);
	}

	return anims;
}

void SpriteFrames::_set_animations(const Array& p_animations)
{
	animations.clear();
	for (int i = 0; i < p_animations.size(); i++) {
		Dictionary d = p_animations[i];

		ERR_CONTINUE(!d.has("name"));
		ERR_CONTINUE(!d.has("speed"));
		ERR_CONTINUE(!d.has("loop"));
		ERR_CONTINUE(!d.has("frames"));

		Anim anim;
		anim
.speed = d["speed"];
		Array frames = d["frames"];
		Variant loop = d["loop"];
		anim.loop = static_cast<LoopMode>((int)loop);

		for (int j = 0; j < frames.size(); j++) {
#ifndef DISABLE_DEPRECATED
			// For compatibility.
			Ref<Resource> res = frames[j];
			if (res.is_valid()) {
				Frame frame = {res, 1.0};
				anim.frames.push_back(frame);
				continue;
			}
#endif

			Dictionary f = frames[j];

			ERR_CONTINUE(!f.has("texture"));
			ERR_CONTINUE(!f.has("duration"));

			Frame frame = {f["texture"], MAX(SPRITE_FRAME_MINIMUM_DURATION, (float)f["duration"])};
			anim.frames.push_back(frame);
		}

		animations[d["name"]] = anim;
	}
}

#ifdef TOOLS_ENABLED
void SpriteFrames::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "has_animation" || pf == "remove_animation" || pf == "rename_animation" ||
			pf == "set_animation_speed" || pf == "get_animation_speed" ||
			pf == "set_animation_loop" || pf == "get_animation_loop" || pf == "add_frame" ||
			pf == "set_frame" || pf == "remove_frame" || pf == "get_frame_count" ||
			pf == "get_frame_texture" || pf == "get_frame_duration" || pf == "clear") {
			for (const String& E : get_animation_names()) {
				r_options->push_back(E.quote());
			}
		}
	}
	this->obj->get_argument_options(p_function, p_idx, r_options);
}
#endif

void SpriteFrames::_bind_methods() {}

SpriteFrames::SpriteFrames() { add_animation(SceneStringName(default_)); }


