/**************************************************************************/
/*  animation_player.compat.inc                                           */
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

#ifndef DISABLE_DEPRECATED

#include "animation_player.h"
#include "core/object/class_db.h"

void AnimationPlayer::_set_process_callback_bind_compat_80813(
	AnimationPlayer::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationPlayer::AnimationProcessCallback
AnimationPlayer::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

void AnimationPlayer::_set_method_call_mode_bind_compat_80813(
	AnimationPlayer::AnimationMethodCallMode p_mode)
{
	set_callback_mode_method(
		static_cast<AnimationMixer::AnimationCallbackModeMethod>(static_cast<int>(p_mode)));
}

AnimationPlayer::AnimationMethodCallMode
AnimationPlayer::_get_method_call_mode_bind_compat_80813() const
{
	return static_cast<AnimationMethodCallMode>(static_cast<int>(get_callback_mode_method()));
}

void AnimationPlayer::_set_root_bind_compat_80813(const NodePath& p_root) { set_root_node(p_root); }

NodePath AnimationPlayer::_get_root_bind_compat_80813() const { return get_root_node(); }

void AnimationPlayer::_seek_bind_compat_80813(double p_time, bool p_update)
{
	seek(p_time, p_update, false);
}

Vector<String> AnimationPlayer::_get_queue_bind_compat_110767()
{
	Vector<String> queue;
	for (const Variant& E : get_queue()) {
		queue.push_back(E);
	}
	return queue;
}

String AnimationPlayer::_get_current_animation_bind_compat_110767() const
{
	return get_current_animation();
}

void AnimationPlayer::_set_current_animation_bind_compat_110767(const String& p_animation)
{
	set_current_animation(p_animation);
}

String AnimationPlayer::_get_assigned_animation_bind_compat_110767() const
{
	return get_assigned_animation();
}

void AnimationPlayer::_set_assigned_animation_bind_compat_110767(const String& p_animation)
{
	set_assigned_animation(p_animation);
}

String AnimationPlayer::_get_autoplay_bind_compat_110767() const { return get_autoplay(); }

void AnimationPlayer::_set_autoplay_bind_compat_110767(const String& p_name)
{
	set_autoplay(p_name);
}

void AnimationPlayer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


