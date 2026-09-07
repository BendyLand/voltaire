/**************************************************************************/
/*  animated_sprite_2d.cpp                                                */
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

#include "animated_sprite_2d.h"
#include "core/config/engine.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"

#ifdef DEBUG_ENABLED
Rect2 AnimatedSprite2D::_edit_get_rect() const { return _get_rect(); }

bool AnimatedSprite2D::_edit_use_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return false;
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return false;
	}

	Ref<Texture2D> t;
	if (animation) {
		t = frames->get_frame_texture(animation, frame);
	}
	return t.is_valid();
}
#endif // DEBUG_ENABLED

Rect2 AnimatedSprite2D::get_anchorable_rect() const { return _get_rect(); }

Rect2 AnimatedSprite2D::_get_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return Rect2();
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return Rect2();
	}

	Ref<Texture2D> t;
	if (animation) {
		t = frames->get_frame_texture(animation, frame);
	}
	if (t.is_null()) {
		return Rect2();
	}
	Size2 s = t->get_size();

	Point2 ofs = offset;
	if (centered) {
		ofs -= s / 2;
	}

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}







Ref<SpriteFrames> AnimatedSprite2D::get_sprite_frames() const { return frames; }

void AnimatedSprite2D::set_frame(int p_frame)
{
	set_frame_and_progress(p_frame, std::signbit(get_playing_speed()) ? 1.0 : 0.0);
}

int AnimatedSprite2D::get_frame() const { return frame; }

void AnimatedSprite2D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t AnimatedSprite2D::get_frame_progress() const { return frame_progress; }

void AnimatedSprite2D::set_frame_and_progress(int p_frame, real_t p_progress)
{
	if (frames.is_null()) {
		return;
	}

	bool has_animation = frames->has_animation(animation);
	int end_frame = has_animation ? MAX(0, frames->get_frame_count(animation) - 1) : 0;
	bool is_changed = frame != p_frame;

	if (p_frame < 0) {
		frame = 0;
	}
	else if (has_animation && p_frame > end_frame) {
		frame = end_frame;
	}
	else {
		frame = p_frame;
	}

	_calc_frame_speed_scale();
	frame_progress = p_progress;

	if (!is_changed) {
		return; // No change, don't redraw.
	}
	queue_redraw();
}

void AnimatedSprite2D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float AnimatedSprite2D::get_speed_scale() const { return speed_scale; }

float AnimatedSprite2D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

void AnimatedSprite2D::set_centered(bool p_center)
{
	if (centered == p_center) {
		return;
	}

	centered = p_center;
	queue_redraw();
	item_rect_changed();
}

bool AnimatedSprite2D::is_centered() const { return centered; }

void AnimatedSprite2D::set_offset(const Point2& p_offset)
{
	if (offset == p_offset) {
		return;
	}

	offset = p_offset;
	queue_redraw();
	item_rect_changed();
}

Point2 AnimatedSprite2D::get_offset() const { return offset; }

void AnimatedSprite2D::set_flip_h(bool p_flip)
{
	if (hflip == p_flip) {
		return;
	}

	hflip = p_flip;
	queue_redraw();
}

bool AnimatedSprite2D::is_flipped_h() const { return hflip; }

void AnimatedSprite2D::set_flip_v(bool p_flip)
{
	if (vflip == p_flip) {
		return;
	}

	vflip = p_flip;
	queue_redraw();
}

bool AnimatedSprite2D::is_flipped_v() const { return vflip; }

void AnimatedSprite2D::_res_changed()
{
	set_frame_and_progress(frame, frame_progress);
	queue_redraw();
}

bool AnimatedSprite2D::is_playing() const { return playing; }

void AnimatedSprite2D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimatedSprite2D::get_autoplay() const { return autoplay; }



void AnimatedSprite2D::play_backwards(const StringName& p_name) { play(p_name, -1, true); }

void AnimatedSprite2D::pause() { _stop_internal(false); }

void AnimatedSprite2D::stop() { _stop_internal(true); }

double AnimatedSprite2D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void AnimatedSprite2D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

StringName AnimatedSprite2D::get_animation() const { return animation; }

PackedStringArray AnimatedSprite2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (frames.is_null()) {
		warnings.push_back(
			RTR("A SpriteFrames resource must be created or set in the \"Sprite Frames\" property "
				"in order for AnimatedSprite2D to display frames."));
	}
	return warnings;
}

#ifdef TOOLS_ENABLED
void AnimatedSprite2D::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && frames.is_valid()) {
		if (pf == "play" || pf == "play_backwards" || pf == "set_animation" ||
			pf == "set_autoplay") {
			List<StringName> al;
			frames->get_animation_list(&al);
			for (const StringName& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node2D::get_argument_options(p_function, p_idx, r_options);
}
#endif // TOOLS_ENABLED

AnimatedSprite2D::AnimatedSprite2D() {}


