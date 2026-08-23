/**************************************************************************/
/*  virtual_joystick.cpp                                                  */
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
#include "core/input/input.h"
#include "core/object/class_db.h"
#include "scene/theme/theme_db.h"
#include "virtual_joystick.h"

void VirtualJoystick::gui_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventScreenTouch> touch = p_event;
	if (touch.is_valid()) {
		if (touch->is_pressed()) {
			if (touch_index == -1 && has_point(touch->get_position())) {
				Rect2 base_rect = Rect2(joystick_pos - Vector2(0.5, 0.5) * joystick_size,
					Vector2(joystick_size, joystick_size));
				if (joystick_mode == JOYSTICK_DYNAMIC || joystick_mode == JOYSTICK_FOLLOWING ||
					(base_rect.has_point(touch->get_position()) &&
						joystick_mode == JOYSTICK_FIXED)) {
					if (joystick_mode == JOYSTICK_DYNAMIC || joystick_mode == JOYSTICK_FOLLOWING) {
						joystick_pos = touch->get_position();
					}

					this->obj->emit_signal(SceneStringName(pressed));

					is_pressed = true;
					touch_index = touch->get_index();
					_update_joystick(touch->get_position());
				}
			}
		}
		else if (touch->get_index() == touch_index) {
			is_pressed = false;
			this->obj->emit_signal(SNAME("released"), input_vector);

			if (!is_flick_canceled && !has_moved) {
				this->obj->emit_signal(SNAME("tapped"));
			}
			else if (has_input && has_moved) {
				this->obj->emit_signal(SNAME("flicked"), input_vector);
			}
			_reset();
		}
	}

	Ref<InputEventScreenDrag> drag = p_event;
	if (drag.is_valid() && drag->get_index() == touch_index) {
		has_moved = true;
		_update_joystick(drag->get_position());
	}
}

void VirtualJoystick::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!Engine::get_singleton()->is_editor_hint() && visibility == VISIBILITY_WHEN_TOUCHED &&
			!is_pressed) {
			return;
		}

		Rect2 rect_joystick = Rect2(joystick_pos - Vector2(0.5, 0.5) * joystick_size,
			Vector2(joystick_size, joystick_size));
		draw_style_box(
			is_pressed ? theme_cache.pressed_joystick.ptr() : theme_cache.normal_joystick.ptr(), rect_joystick);

		Rect2 rect_tip = Rect2(tip_pos - Vector2(0.5, 0.5) * tip_size, Vector2(tip_size, tip_size));
		draw_style_box(is_pressed ? theme_cache.pressed_tip.ptr() : theme_cache.normal_tip.ptr(), rect_tip);
	} break;

	case NOTIFICATION_ENTER_TREE: {
		joystick_pos = get_size() * initial_offset_ratio;
		tip_pos = joystick_pos;
	} break;

	case NOTIFICATION_RESIZED: {
		_reset();
	} break;
	}
}

void VirtualJoystick::_update_joystick(const Vector2& p_pos)
{
	Vector2 offset = p_pos - joystick_pos;
	float length = offset.length();
	Vector2 direction = offset.normalized();

	float clampzone_radius = joystick_size * 0.5f * clampzone_ratio;

	if (joystick_mode == JOYSTICK_FOLLOWING && length > clampzone_radius && has_point(p_pos)) {
		joystick_pos = p_pos - direction * clampzone_radius;
	}

	if (length > clampzone_radius) {
		length = clampzone_radius;
		offset = direction * length;
	}

	tip_pos = joystick_pos + offset;

	bool was_pressed = has_input;
	raw_input_vector = offset / clampzone_radius;
	if (length > deadzone_ratio * clampzone_radius) {
		has_input = true;
		float scaled =
			Math::inverse_lerp(deadzone_ratio * clampzone_radius, clampzone_radius, length);
		input_vector = direction * scaled;
	}
	else {
		has_input = false;
		input_vector = Vector2();
	}

	if (!is_flick_canceled && was_pressed && !has_input) {
		is_flick_canceled = true;
		this->obj->emit_signal(SNAME("flick_canceled"));
	}
	else if (is_flick_canceled && !was_pressed && has_input) {
		is_flick_canceled = false;
	}

	_handle_input_actions();

	queue_redraw();
}

void VirtualJoystick::_handle_input_actions()
{
	Input* input = Input::get_singleton();

	if (input_vector.x >= 0.0f && input->is_action_pressed(action_left)) {
		input->action_release(action_left);
	}
	if (input_vector.x <= 0.0f && input->is_action_pressed(action_right)) {
		input->action_release(action_right);
	}
	if (input_vector.y >= 0.0f && input->is_action_pressed(action_up)) {
		input->action_release(action_up);
	}
	if (input_vector.y <= 0.0f && input->is_action_pressed(action_down)) {
		input->action_release(action_down);
	}

	if (input_vector.x < 0.0f) {
		input->action_press(action_left, -input_vector.x);
	}
	else if (input_vector.x > 0.0f) {
		input->action_press(action_right, input_vector.x);
	}
	if (input_vector.y < 0.0f) {
		input->action_press(action_up, -input_vector.y);
	}
	else if (input_vector.y > 0.0f) {
		input->action_press(action_down, input_vector.y);
	}
}

void VirtualJoystick::_reset()
{
	is_pressed = false;
	has_input = false;
	has_moved = false;
	raw_input_vector = Vector2();
	input_vector = Vector2();
	is_flick_canceled = false;
	touch_index = -1;
	joystick_pos = get_size() * initial_offset_ratio;
	tip_pos = joystick_pos;

	if (!Engine::get_singleton()->is_editor_hint()) {
		// Only release actions when not currently in the editor.
		// Custom input actions are not defined while in the editor,
		// so this would lead to error spam due to unknown input actions.
		Input* input = Input::get_singleton();
		for (const StringName& action : {action_left, action_right, action_down, action_up}) {
			if (input->is_action_pressed(action)) {
				input->action_release(action);
			}
		}
	}

	queue_redraw();
}

void VirtualJoystick::_bind_methods() {}

Vector2 VirtualJoystick::get_joystick_position() const { return joystick_pos; }

void VirtualJoystick::set_joystick_size(float p_size)
{
	if (joystick_size == p_size) {
		return;
	}
	joystick_size = p_size;
	_reset();
}

float VirtualJoystick::get_joystick_size() const { return joystick_size; }

void VirtualJoystick::set_tip_size(float p_size)
{
	if (tip_size == p_size) {
		return;
	}
	tip_size = p_size;
	_reset();
}

float VirtualJoystick::get_tip_size() const { return tip_size; }

void VirtualJoystick::set_deadzone_ratio(float p_ratio)
{
	deadzone_ratio = p_ratio;
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

float VirtualJoystick::get_deadzone_ratio() const { return deadzone_ratio; }

void VirtualJoystick::set_clampzone_ratio(float p_ratio)
{
	clampzone_ratio = p_ratio;
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

float VirtualJoystick::get_clampzone_ratio() const { return clampzone_ratio; }

void VirtualJoystick::set_initial_offset_ratio(const Vector2& p_ratio)
{
	if (initial_offset_ratio == p_ratio) {
		return;
	}
	initial_offset_ratio = p_ratio;
	_reset();
}

Vector2 VirtualJoystick::get_initial_offset_ratio() const { return initial_offset_ratio; }

void VirtualJoystick::set_joystick_mode(JoystickMode p_mode) { joystick_mode = p_mode; }

VirtualJoystick::JoystickMode VirtualJoystick::get_joystick_mode() const { return joystick_mode; }

void VirtualJoystick::set_action_left(const StringName& p_action) { action_left = p_action; }

StringName VirtualJoystick::get_action_left() const { return action_left; }

void VirtualJoystick::set_action_right(const StringName& p_action) { action_right = p_action; }

StringName VirtualJoystick::get_action_right() const { return action_right; }

void VirtualJoystick::set_action_up(const StringName& p_action) { action_up = p_action; }

StringName VirtualJoystick::get_action_up() const { return action_up; }

void VirtualJoystick::set_action_down(const StringName& p_action) { action_down = p_action; }

StringName VirtualJoystick::get_action_down() const { return action_down; }

void VirtualJoystick::set_visibility_mode(VisibilityMode p_mode) { visibility = p_mode; }

VirtualJoystick::VisibilityMode VirtualJoystick::get_visibility_mode() const { return visibility; }


