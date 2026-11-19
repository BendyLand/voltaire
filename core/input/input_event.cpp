/**************************************************************************/
/*  input_event.cpp                                                       */
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

#include "input_event.h"

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/math/transform_2d.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

static const char* _mouse_button_descriptions[9] = {
	TTRC("Left Mouse Button"),
	TTRC("Right Mouse Button"),
	TTRC("Middle Mouse Button"),
	TTRC("Mouse Wheel Up"),
	TTRC("Mouse Wheel Down"),
	TTRC("Mouse Wheel Left"),
	TTRC("Mouse Wheel Right"),
	TTRC("Mouse Thumb Button 1"),
	TTRC("Mouse Thumb Button 2"),
};

static const char* _joy_axis_descriptions[(size_t)JoyAxis::MAX] = {
	TTRC("Left Stick X-Axis, Joystick 0 X-Axis"),
	TTRC("Left Stick Y-Axis, Joystick 0 Y-Axis"),
	TTRC("Right Stick X-Axis, Joystick 1 X-Axis"),
	TTRC("Right Stick Y-Axis, Joystick 1 Y-Axis"),
	TTRC("Joystick 2 X-Axis, Left Trigger, Sony L2, Xbox LT"),
	TTRC("Joystick 2 Y-Axis, Right Trigger, Sony R2, Xbox RT"),
	TTRC("Joystick 3 X-Axis"),
	TTRC("Joystick 3 Y-Axis"),
	TTRC("Joystick 4 X-Axis"),
	TTRC("Joystick 4 Y-Axis"),
};

static const char* _joy_button_descriptions[(size_t)JoyButton::SDL_MAX] = {
	TTRC("Bottom Action, Sony Cross, Xbox A, Nintendo B"),
	TTRC("Right Action, Sony Circle, Xbox B, Nintendo A"),
	TTRC("Left Action, Sony Square, Xbox X, Nintendo Y"),
	TTRC("Top Action, Sony Triangle, Xbox Y, Nintendo X"),
	TTRC("Back, Sony Select, Xbox Back, Nintendo -"),
	TTRC("Guide, Sony PS, Xbox Home"),
	TTRC("Start, Xbox Menu, Nintendo +"),
	TTRC("Left Stick, Sony L3, Xbox L/LS"),
	TTRC("Right Stick, Sony R3, Xbox R/RS"),
	TTRC("Left Shoulder, Sony L1, Xbox LB"),
	TTRC("Right Shoulder, Sony R1, Xbox RB"),
	TTRC("D-pad Up"),
	TTRC("D-pad Down"),
	TTRC("D-pad Left"),
	TTRC("D-pad Right"),
	TTRC("Xbox Share, PS5 Microphone, Nintendo Capture"),
	TTRC("Xbox Paddle 1"),
	TTRC("Xbox Paddle 2"),
	TTRC("Xbox Paddle 3"),
	TTRC("Xbox Paddle 4"),
	TTRC("PS4/5 Touchpad"),
};

InputEvent InputEvent::make_key(Key p_keycode, bool p_physical)
{
	InputEvent ie;
	ie.type = InputEventType::KEY;
	ie.device = DEVICE_ID_KEYBOARD;
	ie.key = KeyData{};

	if (p_keycode == Key::NONE) {
		return ie;
	}

	if (p_physical) {
		ie.key.physical_keycode = p_keycode & KeyModifierMask::CODE_MASK;
	}
	else {
		ie.key.keycode = p_keycode & KeyModifierMask::CODE_MASK;
	}

	char32_t ch = char32_t(p_keycode & KeyModifierMask::CODE_MASK);
	if (ch < 0xd800 || (ch > 0xdfff && ch <= 0x10ffff)) {
		ie.key.unicode = ch;
	}

	if ((p_keycode & KeyModifierMask::SHIFT) != Key::NONE) {
		ie.set_shift_pressed(true);
	}
	if ((p_keycode & KeyModifierMask::ALT) != Key::NONE) {
		ie.set_alt_pressed(true);
	}
	if ((p_keycode & KeyModifierMask::CMD_OR_CTRL) != Key::NONE) {
		ie.set_command_or_control_autoremap(true);
		if ((p_keycode & KeyModifierMask::CTRL) != Key::NONE ||
			(p_keycode & KeyModifierMask::META) != Key::NONE) {
			WARN_PRINT("Invalid Key Modifiers: Command or Control autoremapping is enabled, Meta "
					   "and Control values are ignored!");
		}
	}
	else {
		if ((p_keycode & KeyModifierMask::CTRL) != Key::NONE) {
			ie.set_ctrl_pressed(true);
		}
		if ((p_keycode & KeyModifierMask::META) != Key::NONE) {
			ie.set_meta_pressed(true);
		}
	}

	return ie;
}

InputEvent InputEvent::make_mouse_button()
{
	InputEvent ie;
	ie.type = InputEventType::MOUSE_BUTTON;
	ie.device = DEVICE_ID_MOUSE;
	ie.mouse_button = MouseButtonData{};
	return ie;
}

InputEvent InputEvent::make_mouse_motion()
{
	InputEvent ie;
	ie.type = InputEventType::MOUSE_MOTION;
	ie.device = DEVICE_ID_MOUSE;
	ie.mouse_motion = MouseMotionData{};
	return ie;
}

InputEvent InputEvent::make_joypad_button(JoyButton p_btn_index, int p_device)
{
	InputEvent ie;
	ie.type = InputEventType::JOY_BUTTON;
	ie.device = p_device;
	ie.joy_button = JoypadButtonData{};
	ie.joy_button.button_index = p_btn_index;
	return ie;
}

InputEvent InputEvent::make_joypad_motion(JoyAxis p_axis, float p_value, int p_device)
{
	InputEvent ie;
	ie.type = InputEventType::JOY_MOTION;
	ie.device = p_device;
	ie.joy_motion = JoypadMotionData{};
	ie.set_joy_axis(p_axis);
	ie.set_joy_axis_value(p_value);
	return ie;
}

InputEvent InputEvent::make_screen_touch()
{
	InputEvent ie;
	ie.type = InputEventType::SCREEN_TOUCH;
	ie.touch = TouchData{};
	return ie;
}

InputEvent InputEvent::make_screen_drag()
{
	InputEvent ie;
	ie.type = InputEventType::SCREEN_DRAG;
	ie.screen_drag = ScreenDragData{};
	return ie;
}

InputEvent InputEvent::make_magnify_gesture()
{
	InputEvent ie;
	ie.type = InputEventType::MAGNIFY_GESTURE;
	ie.gesture = GestureData{};
	return ie;
}

InputEvent InputEvent::make_pan_gesture()
{
	InputEvent ie;
	ie.type = InputEventType::PAN_GESTURE;
	ie.gesture = GestureData{};
	return ie;
}

InputEvent InputEvent::make_midi()
{
	InputEvent ie;
	ie.type = InputEventType::MIDI;
	ie.midi = MIDIData{};
	return ie;
}

InputEvent InputEvent::make_action(const String& p_action, bool p_pressed, float p_strength)
{
	InputEvent ie;
	ie.type = InputEventType::ACTION;
	ie.pressed = p_pressed;
	ie.action.action = p_action;
	ie.action.strength = CLAMP(p_strength, 0.0f, 1.0f);
	ie.action.event_index = -1;
	return ie;
}

InputEvent InputEvent::make_shortcut(const Shortcut* p_shortcut)
{
	InputEvent ie;
	ie.type = InputEventType::SHORTCUT;
	ie.pressed = true;
	ie.shortcut = ShortcutData{p_shortcut};
	return ie;
}

Vector2 InputEvent::get_mouse_position() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.position;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.position;
	}
	return Vector2();
}

void InputEvent::set_mouse_position(const Vector2& p_pos)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.position = p_pos;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.position = p_pos;
	}
}

Vector2 InputEvent::get_mouse_global_position() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.global_position;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.global_position;
	}
	return Vector2();
}

void InputEvent::set_mouse_global_position(const Vector2& p_global_pos)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.global_position = p_global_pos;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.global_position = p_global_pos;
	}
}

uint32_t InputEvent::get_mouse_button_mask() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.button_mask;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.button_mask;
	}
	return 0;
}

void InputEvent::set_mouse_button_mask(uint32_t p_mask)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.button_mask = p_mask;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.button_mask = p_mask;
	}
}

void InputEvent::set_joy_axis(JoyAxis p_axis)
{
	ERR_FAIL_COND(p_axis < JoyAxis::INVALID || p_axis > JoyAxis::MAX);
	joy_motion.axis = p_axis;
}

void InputEvent::set_joy_axis_value(float p_value)
{
	joy_motion.axis_value = p_value;
	pressed = Math::abs(p_value) >= InputMap::DEFAULT_TOGGLE_DEADZONE;
}

void InputEvent::set_command_or_control_autoremap(bool p_enabled)
{
	if (is_command_or_control_autoremap() == p_enabled) {
		return;
	}
	if (p_enabled) {
		modifiers.set_flag(KeyModifierMask::CMD_OR_CTRL);
		if (OS::prefer_meta_over_ctrl()) {
			modifiers.clear_flag(KeyModifierMask::CTRL);
			modifiers.set_flag(KeyModifierMask::META);
		}
		else {
			modifiers.set_flag(KeyModifierMask::CTRL);
			modifiers.clear_flag(KeyModifierMask::META);
		}
	}
	else {
		modifiers.clear_flag(KeyModifierMask::CMD_OR_CTRL);
		modifiers.clear_flag(KeyModifierMask::CTRL);
		modifiers.clear_flag(KeyModifierMask::META);
	}
}

bool InputEvent::is_command_or_control_pressed() const
{
	return OS::prefer_meta_over_ctrl() ? is_meta_pressed() : is_ctrl_pressed();
}

void InputEvent::set_shift_pressed(bool p_pressed)
{
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::SHIFT);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::SHIFT);
	}
}

void InputEvent::set_alt_pressed(bool p_pressed)
{
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::ALT);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::ALT);
	}
}

void InputEvent::set_ctrl_pressed(bool p_pressed)
{
	ERR_FAIL_COND_MSG(is_command_or_control_autoremap(),
		"Command or Control autoremapping is enabled, cannot set Control directly!");
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::CTRL);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::CTRL);
	}
}

void InputEvent::set_meta_pressed(bool p_pressed)
{
	ERR_FAIL_COND_MSG(is_command_or_control_autoremap(),
		"Command or Control autoremapping is enabled, cannot set Meta directly!");
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::META);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::META);
	}
}

void InputEvent::copy_modifiers_from(const InputEvent& p_event)
{
	modifiers = p_event.modifiers;
}

String InputEvent::modifiers_as_text() const
{
	Vector<String> mod_names;
	if (is_ctrl_pressed()) {
		mod_names.push_back(find_keycode_name(Key::CTRL));
	}
	if (is_alt_pressed()) {
		mod_names.push_back(find_keycode_name(Key::ALT));
	}
	if (is_shift_pressed()) {
		mod_names.push_back(find_keycode_name(Key::SHIFT));
	}
	if (is_meta_pressed()) {
		mod_names.push_back(find_keycode_name(Key::META));
	}
	return mod_names.is_empty() ? String() : String("+").join(mod_names);
}

Key InputEvent::get_keycode_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.keycode | get_modifiers_mask()) : Key::NONE;
}

Key InputEvent::get_physical_keycode_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.physical_keycode | get_modifiers_mask()) : Key::NONE;
}

Key InputEvent::get_key_label_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.key_label | get_modifiers_mask()) : Key::NONE;
}

String InputEvent::as_text_physical_keycode() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc = (key.physical_keycode != Key::NONE) ? keycode_get_string(key.physical_keycode)
													: "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_keycode() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc =
		(key.keycode != Key::NONE) ? keycode_get_string(key.keycode) : "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_key_label() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc =
		(key.key_label != Key::NONE) ? keycode_get_string(key.key_label) : "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_location() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	switch (key.location) {
	case KeyLocation::LEFT:
		return "left";
	case KeyLocation::RIGHT:
		return "right";
	default:
		return String();
	}
}

bool InputEvent::is_action(const String& p_action, bool p_exact_match) const
{
	return InputMap::event_is_action(*this, p_action, p_exact_match);
}

bool InputEvent::is_action_pressed(
	const String& p_action, bool p_allow_echo, bool p_exact_match) const
{
	bool pressed_state = false;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, &pressed_state, nullptr, nullptr);
	return valid && pressed_state && (p_allow_echo || !is_echo());
}

bool InputEvent::is_action_released(const String& p_action, bool p_exact_match) const
{
	bool pressed_state = false;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, &pressed_state, nullptr, nullptr);
	return valid && !pressed_state;
}

bool InputEvent::is_action_just_pressed_or_echo(
	const String& p_action, bool p_exact_match) const
{
	return is_action(p_action, p_exact_match) &&
		   (is_echo() || Input::is_action_just_pressed_by_event(p_action, *this, p_exact_match));
}

float InputEvent::get_action_strength(const String& p_action, bool p_exact_match) const
{
	float strength = 0.0f;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, nullptr, &strength, nullptr);
	return valid ? strength : 0.0f;
}

float InputEvent::get_action_raw_strength(const String& p_action, bool p_exact_match) const
{
	float raw_strength = 0.0f;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, nullptr, nullptr, &raw_strength);
	return valid ? raw_strength : 0.0f;
}

bool InputEvent::is_action_type() const
{
	switch (type) {
	case InputEventType::KEY:
	case InputEventType::MOUSE_BUTTON:
	case InputEventType::JOY_BUTTON:
	case InputEventType::JOY_MOTION:
	case InputEventType::ACTION:
		return true;
	default:
		return false;
	}
}

bool InputEvent::action_match(const InputEvent& p_event, bool p_exact_match, float p_deadzone,
	bool* r_pressed, float* r_strength, float* r_raw_strength) const
{
	if (type != p_event.type) {
		return false;
	}

	switch (type) {
	case InputEventType::KEY: {
		bool match = false;
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			match = (key.key_label == p_event.key.key_label);
		}
		else if (key.keycode != Key::NONE) {
			match = (key.keycode == p_event.key.keycode);
		}
		else if (key.physical_keycode != Key::NONE) {
			match = (key.physical_keycode == p_event.key.physical_keycode);
			if (key.location != KeyLocation::UNSPECIFIED) {
				match &= (key.location == p_event.key.location);
			}
		}

		uint32_t action_mask = get_modifiers_mask();
		uint32_t key_mask = p_event.get_modifiers_mask();
		if (p_event.is_pressed()) {
			match &= ((action_mask & key_mask) == action_mask);
		}
		if (p_exact_match) {
			match &= (action_mask == key_mask);
		}
		if (match) {
			bool key_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = key_pressed;
			}
			float strength = key_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::MOUSE_BUTTON: {
		bool match = (mouse_button.button_index == p_event.mouse_button.button_index);
		uint32_t action_mask = get_modifiers_mask();
		uint32_t btn_mask = p_event.get_modifiers_mask();
		if (p_event.is_pressed()) {
			match &= ((action_mask & btn_mask) == action_mask);
		}
		if (p_exact_match) {
			match &= (action_mask == btn_mask);
		}
		if (match) {
			bool mb_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = mb_pressed;
			}
			float strength = mb_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::JOY_MOTION: {
		bool match = (joy_motion.axis == p_event.joy_motion.axis);
		if (p_exact_match) {
			match &= ((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0));
		}
		if (match) {
			float jm_abs = Math::abs(p_event.joy_motion.axis_value);
			bool same_direction =
				(((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0)) ||
					p_event.joy_motion.axis_value == 0);
			bool pressed_state = same_direction && jm_abs >= p_deadzone;
			if (r_pressed) {
				*r_pressed = pressed_state;
			}
			if (r_strength) {
				if (pressed_state) {
					*r_strength = (p_deadzone == 1.0f)
									  ? 1.0f
									  : CLAMP(Math::inverse_lerp(p_deadzone, 1.0f, jm_abs), 0.0f,
											1.0f);
				}
				else {
					*r_strength = 0.0f;
				}
			}
			if (r_raw_strength) {
				*r_raw_strength = same_direction ? jm_abs : 0.0f;
			}
		}
		return match;
	}

	case InputEventType::JOY_BUTTON: {
		bool match = (joy_button.button_index == p_event.joy_button.button_index);
		if (match) {
			bool jb_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = jb_pressed;
			}
			float strength = jb_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::ACTION: {
		bool match = (action.action == p_event.action.action);
		if (match) {
			bool act_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = act_pressed;
			}
			float act_strength = act_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = act_strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = act_strength;
			}
		}
		return match;
	}

	default:
		return false;
	}
}

bool InputEvent::is_match(const InputEvent& p_event, bool p_exact_match) const
{
	if (type == InputEventType::ACTION) {
		return p_event.is_valid() && p_event.is_action(action.action, p_exact_match);
	}
	if (type != p_event.type) {
		return false;
	}

	switch (type) {
	case InputEventType::KEY:
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			return (key.key_label == p_event.key.key_label) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		else if (key.keycode != Key::NONE) {
			return (key.keycode == p_event.key.keycode) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		else if (key.physical_keycode != Key::NONE) {
			if (key.location != KeyLocation::UNSPECIFIED && key.location != p_event.key.location) {
				return false;
			}
			return (key.physical_keycode == p_event.key.physical_keycode) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		return false;

	case InputEventType::MOUSE_BUTTON:
		return (mouse_button.button_index == p_event.mouse_button.button_index) &&
			   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());

	case InputEventType::JOY_MOTION:
		return (joy_motion.axis == p_event.joy_motion.axis) &&
			   (!p_exact_match || ((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0)));

	case InputEventType::JOY_BUTTON:
		return joy_button.button_index == p_event.joy_button.button_index;

	default:
		return false;
	}
}

InputEvent InputEvent::xformed_by(const Transform2D& p_xform, const Vector2& p_local_ofs) const
{
	InputEvent ev = *this;
	switch (type) {
	case InputEventType::MOUSE_BUTTON:
		ev.mouse_button.position = p_xform.xform(mouse_button.position + p_local_ofs);
		break;
	case InputEventType::MOUSE_MOTION:
		ev.mouse_motion.position = p_xform.xform(mouse_motion.position + p_local_ofs);
		ev.mouse_motion.relative = p_xform.basis_xform(mouse_motion.relative);
		ev.mouse_motion.velocity = p_xform.basis_xform(mouse_motion.velocity);
		break;
	case InputEventType::SCREEN_TOUCH:
		ev.touch.position = p_xform.xform(touch.position + p_local_ofs);
		break;
	case InputEventType::SCREEN_DRAG:
		ev.screen_drag.position = p_xform.xform(screen_drag.position + p_local_ofs);
		ev.screen_drag.relative = p_xform.basis_xform(screen_drag.relative);
		ev.screen_drag.velocity = p_xform.basis_xform(screen_drag.velocity);
		break;
	case InputEventType::MAGNIFY_GESTURE:
	case InputEventType::PAN_GESTURE:
		ev.gesture.position = p_xform.xform(gesture.position + p_local_ofs);
		break;
	default:
		break;
	}
	return ev;
}

bool InputEvent::accumulate(const InputEvent& p_event)
{
	if (type != p_event.type) {
		return false;
	}

	if (type == InputEventType::MOUSE_MOTION) {
		if (window_id != p_event.window_id || is_canceled() != p_event.is_canceled() ||
			is_pressed() != p_event.is_pressed() ||
			mouse_motion.button_mask != p_event.mouse_motion.button_mask ||
			get_modifiers_mask() != p_event.get_modifiers_mask()) {
			return false;
		}
		mouse_motion.position = p_event.mouse_motion.position;
		mouse_motion.global_position = p_event.mouse_motion.global_position;
		mouse_motion.velocity = p_event.mouse_motion.velocity;
		mouse_motion.screen_velocity = p_event.mouse_motion.screen_velocity;
		mouse_motion.relative += p_event.mouse_motion.relative;
		mouse_motion.screen_relative += p_event.mouse_motion.screen_relative;
		return true;
	}

	if (type == InputEventType::SCREEN_DRAG) {
		if (screen_drag.index != p_event.screen_drag.index) {
			return false;
		}
		screen_drag.position = p_event.screen_drag.position;
		screen_drag.velocity = p_event.screen_drag.velocity;
		screen_drag.screen_velocity = p_event.screen_drag.screen_velocity;
		screen_drag.relative += p_event.screen_drag.relative;
		screen_drag.screen_relative += p_event.screen_drag.screen_relative;
		return true;
	}

	return false;
}

String InputEvent::as_text() const
{
	switch (type) {
	case InputEventType::KEY: {
		String kc;
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			kc = keycode_get_string(key.key_label) + " - Unicode";
		}
		else if (key.keycode != Key::NONE) {
			kc = keycode_get_string(key.keycode);
		}
		else if (key.physical_keycode != Key::NONE) {
			kc = keycode_get_string(key.physical_keycode);
			if ((key.physical_keycode & Key::SPECIAL) != Key::SPECIAL) {
				kc += " - " + RTR("Physical");
			}
		}
		else {
			kc = "(" + RTR("unset") + ")";
		}
		if (kc.is_empty()) {
			return kc;
		}
		String mods_text = modifiers_as_text();
		return mods_text.is_empty() ? kc : mods_text + "+" + kc;
	}

	case InputEventType::MOUSE_BUTTON: {
		String mods_text = modifiers_as_text();
		String full_string = mods_text.is_empty() ? "" : mods_text + "+";
		MouseButton idx = mouse_button.button_index;
		switch (idx) {
		case MouseButton::LEFT:
		case MouseButton::RIGHT:
		case MouseButton::MIDDLE:
		case MouseButton::WHEEL_UP:
		case MouseButton::WHEEL_DOWN:
		case MouseButton::WHEEL_LEFT:
		case MouseButton::WHEEL_RIGHT:
		case MouseButton::MB_XBUTTON1:
		case MouseButton::MB_XBUTTON2:
			full_string += RTR(_mouse_button_descriptions[(size_t)idx - 1]);
			break;
		default:
			full_string += RTR("Button") + " #" + itos((int64_t)idx);
			break;
		}
		if (mouse_button.double_click) {
			full_string += " (" + RTR("Double Click") + ")";
		}
		return full_string;
	}

	case InputEventType::MOUSE_MOTION:
		return vformat(RTR("Mouse motion at position (%s) with velocity (%s)"),
			String(mouse_motion.position), String(mouse_motion.velocity));

	case InputEventType::JOY_MOTION: {
		String desc = joy_motion.axis < JoyAxis::MAX
						  ? TTRGET(_joy_axis_descriptions[(size_t)joy_motion.axis])
						  : RTR("Unknown Joypad Axis");
		return vformat(RTR("Joypad Motion on Axis %d (%s) with Value %.2f"), joy_motion.axis, desc,
			joy_motion.axis_value);
	}

	case InputEventType::JOY_BUTTON: {
		String text = vformat(RTR("Joypad Button %d"), (int64_t)joy_button.button_index);
		if (joy_button.button_index > JoyButton::INVALID &&
			joy_button.button_index < JoyButton::SDL_MAX) {
			text +=
				vformat(" (%s)", TTRGET(_joy_button_descriptions[(size_t)joy_button.button_index]));
		}
		if (joy_button.pressure != 0) {
			text += ", " + RTR("Pressure:") + " " + String(joy_button.pressure);
		}
		return text;
	}

	case InputEventType::SCREEN_TOUCH: {
		String status = canceled ? RTR("canceled") : (pressed ? RTR("touched") : RTR("released"));
		return vformat(RTR("Screen %s at (%s) with %s touch points"), status,
			String(touch.position), itos(touch.index));
	}

	case InputEventType::SCREEN_DRAG:
		return vformat(
			RTR("Screen dragged with %s touch points at position (%s) with velocity of (%s)"),
			itos(screen_drag.index), String(screen_drag.position), String(screen_drag.velocity));

	case InputEventType::ACTION: {
		const List<InputEvent>* events = InputMap::action_get_events(action.action);
		if (!events) {
			return String();
		}
		for (const InputEvent& E : *events) {
			if (E.is_valid()) {
				return E.as_text();
			}
		}
		return String();
	}

	case InputEventType::MAGNIFY_GESTURE:
		return vformat(RTR("Magnify Gesture at (%s) with factor %s"), String(gesture.position),
			rtos(gesture.factor));

	case InputEventType::PAN_GESTURE:
		return vformat(RTR("Pan Gesture at (%s) with delta (%s)"), String(gesture.position),
			String(gesture.delta));

	case InputEventType::MIDI:
		return vformat(RTR("MIDI Input on Channel=%s Message=%s"), itos(midi.channel),
			itos((int64_t)midi.message));

	case InputEventType::SHORTCUT:
		ERR_FAIL_NULL_V(shortcut.shortcut, "None");
		return vformat(RTR("Input Event with Shortcut=%s"), shortcut.shortcut->get_as_text());

	default:
		return String();
	}
}

String InputEvent::to_string() const { return as_text(); }
