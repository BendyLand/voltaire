/**************************************************************************/
/*  input_event_codec.cpp                                                 */
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

#include "input_event_codec.h"

#include "core/input/input.h"
#include "core/io/marshalls.h"

enum class BoolShift : uint8_t
{
	SHIFT = 0,
	CTRL,
	ALT,
	META,
	ECHO,
	PRESSED,
	DOUBLE_CLICK,
	PEN_INVERTED,
};

inline uint8_t operator<<(uint8_t p_left, BoolShift p_right)
{
	return p_left << static_cast<uint8_t>(p_right);
}

static uint8_t encode_key_modifier_state(const InputEvent& p_event)
{
	uint8_t bools = 0;
	bools |= (uint8_t)p_event.is_shift_pressed() << BoolShift::SHIFT;
	bools |= (uint8_t)p_event.is_ctrl_pressed() << BoolShift::CTRL;
	bools |= (uint8_t)p_event.is_alt_pressed() << BoolShift::ALT;
	bools |= (uint8_t)p_event.is_meta_pressed() << BoolShift::META;
	return bools;
}

static void decode_key_modifier_state(uint8_t p_bools, InputEvent& r_event)
{
	r_event.set_shift_pressed(p_bools & (1 << BoolShift::SHIFT));
	r_event.set_ctrl_pressed(p_bools & (1 << BoolShift::CTRL));
	r_event.set_alt_pressed(p_bools & (1 << BoolShift::ALT));
	r_event.set_meta_pressed(p_bools & (1 << BoolShift::META));
}

static int encode_vector2(const Vector2& p_vector, uint8_t* p_data)
{
	p_data += encode_float(p_vector.x, p_data);
	encode_float(p_vector.y, p_data);
	return sizeof(float) * 2;
}

static const uint8_t* decode_vector2(Vector2& r_vector, const uint8_t* p_data)
{
	r_vector.x = decode_float(p_data);
	p_data += sizeof(float);
	r_vector.y = decode_float(p_data);
	p_data += sizeof(float);
	return p_data;
}

void encode_input_event_key(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(19);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::KEY;
	data++;
	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.key.echo << BoolShift::ECHO;
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	*data = bools;
	data++;
	data += encode_uint32((uint32_t)p_event.key.keycode, data);
	data += encode_uint32((uint32_t)p_event.key.physical_keycode, data);
	data += encode_uint32((uint32_t)p_event.key.key_label, data);
	data += encode_uint32(p_event.key.unicode, data);
	*data = (uint8_t)p_event.key.location;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static Error decode_input_event_key(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::KEY);
	data++;

	r_event = InputEvent::make_key();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.key.echo = (bools & (1 << BoolShift::ECHO)) != 0;
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;

	r_event.key.keycode = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.physical_keycode = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.key_label = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.unicode = (char32_t)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.location = (KeyLocation)*data;

	return OK;
}

void encode_input_event_mouse_button(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(12);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MOUSE_BUTTON;
	data++;

	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	bools |= (uint8_t)p_event.mouse_button.double_click << BoolShift::DOUBLE_CLICK;
	*data = bools;
	data++;

	*data = (uint8_t)p_event.mouse_button.button_index;
	data++;

	data += encode_vector2(p_event.mouse_button.position, data);
	*data = (uint8_t)p_event.mouse_button.button_mask;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static Error decode_input_event_mouse_button(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MOUSE_BUTTON);
	data++;

	r_event = InputEvent::make_mouse_button();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;
	r_event.mouse_button.double_click = (bools & (1 << BoolShift::DOUBLE_CLICK)) != 0;

	r_event.mouse_button.button_index = (MouseButton)*data;
	data++;

	Vector2 pos;
	data = decode_vector2(pos, data);
	r_event.mouse_button.position = pos;
	r_event.mouse_button.global_position = pos;
	r_event.mouse_button.button_mask = (uint32_t)*data;

	return OK;
}

void encode_input_event_mouse_motion(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(31);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MOUSE_MOTION;
	data++;

	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.mouse_motion.pen_inverted << BoolShift::PEN_INVERTED;
	*data = bools;
	data++;

	data += encode_vector2(p_event.mouse_motion.position, data);
	data += encode_float(p_event.mouse_motion.pressure, data);
	data += encode_vector2(p_event.mouse_motion.tilt, data);
	data += encode_vector2(p_event.mouse_motion.relative, data);
	*data = (uint8_t)p_event.mouse_motion.button_mask;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_mouse_motion(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MOUSE_MOTION);
	data++;

	r_event = InputEvent::make_mouse_motion();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.mouse_motion.pen_inverted = (bools & (1 << BoolShift::PEN_INVERTED)) != 0;

	{
		Vector2 pos;
		data = decode_vector2(pos, data);
		r_event.mouse_motion.position = pos;
		r_event.mouse_motion.global_position = pos;
	}
	r_event.mouse_motion.pressure = decode_float(data);
	data += sizeof(float);
	data = decode_vector2(r_event.mouse_motion.tilt, data);
	r_event.mouse_motion.velocity = Input::get_last_mouse_velocity();
	r_event.mouse_motion.screen_velocity = Input::get_last_mouse_screen_velocity();
	{
		Vector2 relative;
		data = decode_vector2(relative, data);
		r_event.mouse_motion.relative = relative;
		r_event.mouse_motion.screen_relative = relative;
	}
	r_event.mouse_motion.button_mask = (uint32_t)*data;
	data++;

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_joypad_button(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(11);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::JOY_BUTTON;
	data++;

	uint8_t bools = 0;
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	*data = bools;
	data++;

	data += encode_uint64(p_event.device, data);
	*data = (uint8_t)p_event.joy_button.button_index;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_joypad_button(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::JOY_BUTTON);
	data++;

	r_event = InputEvent::make_joypad_button();
	uint8_t bools = *data;
	data++;
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;
	r_event.device = decode_uint64(data);
	data += sizeof(uint64_t);
	r_event.joy_button.button_index = (JoyButton)*data;
	data++;

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_joypad_motion(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(14);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::JOY_MOTION;
	data++;

	data += encode_uint64(p_event.device, data);
	*data = (uint8_t)p_event.joy_motion.axis;
	data++;
	data += encode_float(p_event.joy_motion.axis_value, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_joypad_motion(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::JOY_MOTION);
	data++;

	int dev = decode_uint64(data);
	data += sizeof(uint64_t);
	JoyAxis axis = (JoyAxis)*data;
	data++;
	float val = decode_float(data);
	data += sizeof(float);

	r_event = InputEvent::make_joypad_motion(axis, val, dev);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_gesture_pan(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(18);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::PAN_GESTURE;
	data++;

	*data = encode_key_modifier_state(p_event);
	data++;
	data += encode_vector2(p_event.gesture.position, data);
	data += encode_vector2(p_event.gesture.delta, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_gesture_pan(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::PAN_GESTURE);
	data++;

	r_event = InputEvent::make_pan_gesture();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);

	data = decode_vector2(r_event.gesture.position, data);
	data = decode_vector2(r_event.gesture.delta, data);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_gesture_magnify(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(14);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MAGNIFY_GESTURE;
	data++;

	*data = encode_key_modifier_state(p_event);
	data++;
	data += encode_vector2(p_event.gesture.position, data);
	data += encode_float(p_event.gesture.factor, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_gesture_magnify(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MAGNIFY_GESTURE);
	data++;

	r_event = InputEvent::make_magnify_gesture();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);

	data = decode_vector2(r_event.gesture.position, data);
	r_event.gesture.factor = decode_float(data);
	data += sizeof(float);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

bool encode_input_event(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	switch (p_event.type) {
	case InputEventType::KEY:
		encode_input_event_key(p_event, r_data);
		break;
	case InputEventType::MOUSE_BUTTON:
		encode_input_event_mouse_button(p_event, r_data);
		break;
	case InputEventType::MOUSE_MOTION:
		encode_input_event_mouse_motion(p_event, r_data);
		break;
	case InputEventType::JOY_MOTION:
		encode_input_event_joypad_motion(p_event, r_data);
		break;
	case InputEventType::JOY_BUTTON:
		encode_input_event_joypad_button(p_event, r_data);
		break;
	case InputEventType::MAGNIFY_GESTURE:
		encode_input_event_gesture_magnify(p_event, r_data);
		break;
	case InputEventType::PAN_GESTURE:
		encode_input_event_gesture_pan(p_event, r_data);
		break;
	default:
		return false;
	}
	return true;
}

void decode_input_event(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();

	switch (static_cast<InputEventType>(*data)) {
	case InputEventType::KEY:
		decode_input_event_key(p_data, r_event);
		break;
	case InputEventType::MOUSE_BUTTON:
		decode_input_event_mouse_button(p_data, r_event);
		break;
	case InputEventType::MOUSE_MOTION:
		decode_input_event_mouse_motion(p_data, r_event);
		break;
	case InputEventType::JOY_BUTTON:
		decode_input_event_joypad_button(p_data, r_event);
		break;
	case InputEventType::JOY_MOTION:
		decode_input_event_joypad_motion(p_data, r_event);
		break;
	case InputEventType::PAN_GESTURE:
		decode_input_event_gesture_pan(p_data, r_event);
		break;
	case InputEventType::MAGNIFY_GESTURE:
		decode_input_event_gesture_magnify(p_data, r_event);
		break;
	default:
		r_event = InputEvent();
		WARN_PRINT(vformat("Unknown event type %d.", static_cast<int>(*data)));
		break;
	}
}
