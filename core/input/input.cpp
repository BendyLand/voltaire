/**************************************************************************/
/*  input.cpp                                                             */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/default_controller_mappings.h"
#include "core/input/input_map.h"
#include "core/os/os.h"
#include "input.compat.inc"
#include "input.h"

#ifdef DEV_ENABLED
#include "core/os/thread.h"
#endif

#include <thirdparty/gamepadmotionhelpers/GamepadMotion.hpp>

#define STANDARD_GRAVITY 9.80665f

static const char* _joy_buttons[(size_t)JoyButton::SDL_MAX] = {
	"a",
	"b",
	"x",
	"y",
	"back",
	"guide",
	"start",
	"leftstick",
	"rightstick",
	"leftshoulder",
	"rightshoulder",
	"dpup",
	"dpdown",
	"dpleft",
	"dpright",
	"misc1",
	"paddle1",
	"paddle2",
	"paddle3",
	"paddle4",
	"touchpad",
	"misc2",
	"misc3",
	"misc4",
	"misc5",
	"misc6",
};

static const char* _joy_axes[(size_t)JoyAxis::SDL_MAX] = {
	"leftx",
	"lefty",
	"rightx",
	"righty",
	"lefttrigger",
	"righttrigger",
};

void (*Input::set_mouse_mode_func)(Input::MouseMode) = nullptr;
Input::MouseMode (*Input::get_mouse_mode_func)() = nullptr;
void (*Input::set_mouse_mode_override_func)(Input::MouseMode) = nullptr;
Input::MouseMode (*Input::get_mouse_mode_override_func)() = nullptr;
void (*Input::set_mouse_mode_override_enabled_func)(bool) = nullptr;
bool (*Input::is_mouse_mode_override_enabled_func)() = nullptr;
void (*Input::warp_mouse_func)(const Vector2& p_position) = nullptr;
Input::CursorShape (*Input::get_current_cursor_shape_func)() = nullptr;
void (*Input::set_custom_mouse_cursor_func)(
	const Ref<Resource>&, Input::CursorShape, const Vector2&) = nullptr;

void Input::set_mouse_mode(MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, MouseMode::MOUSE_MODE_MAX);
	if (set_mouse_mode_func) {
		set_mouse_mode_func(p_mode);
	}
}

Input::MouseMode Input::get_mouse_mode()
{
	return get_mouse_mode_func ? get_mouse_mode_func() : MouseMode::MOUSE_MODE_VISIBLE;
}

void Input::set_mouse_mode_override(MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, MouseMode::MOUSE_MODE_MAX);
	if (set_mouse_mode_override_func) {
		set_mouse_mode_override_func(p_mode);
	}
}

Input::MouseMode Input::get_mouse_mode_override()
{
	return get_mouse_mode_override_func ? get_mouse_mode_override_func()
										: MouseMode::MOUSE_MODE_VISIBLE;
}

void Input::set_mouse_mode_override_enabled(bool p_override_enabled)
{
	if (set_mouse_mode_override_enabled_func) {
		set_mouse_mode_override_enabled_func(p_override_enabled);
	}
}

bool Input::is_mouse_mode_override_enabled()
{
	return is_mouse_mode_override_enabled_func ? is_mouse_mode_override_enabled_func() : false;
}

void Input::VelocityTrack::update(const Vector2& p_delta_p, const Vector2& p_screen_delta_p)
{
	uint64_t tick = OS::get_ticks_usec();
	uint32_t tdiff = tick - last_tick;
	float delta_t = tdiff / 1000000.0;
	last_tick = tick;

	if (delta_t > max_ref_frame) {
		// First movement in a long time, reset and start again.
		velocity = Vector2();
		screen_velocity = Vector2();
		accum = p_delta_p;
		screen_accum = p_screen_delta_p;
		accum_t = 0;
		return;
	}

	accum += p_delta_p;
	screen_accum += p_screen_delta_p;
	accum_t += delta_t;

	if (accum_t < min_ref_frame) {
		// Not enough time has passed to calculate speed precisely.
		return;
	}

	velocity = accum / accum_t;
	screen_velocity = screen_accum / accum_t;
	accum = Vector2();
	screen_accum = Vector2();
	accum_t = 0;
}

void Input::VelocityTrack::reset()
{
	last_tick = OS::get_ticks_usec();
	velocity = Vector2();
	screen_velocity = Vector2();
	accum = Vector2();
	screen_accum = Vector2();
	accum_t = 0;
}

Input::VelocityTrack::VelocityTrack()
{
	min_ref_frame = 0.1;
	max_ref_frame = 3.0;
	reset();
}

bool Input::is_anything_pressed()
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	if (!data->keys_pressed.is_empty() || !data->joy_buttons_pressed.is_empty()) {
		return true;
	}

	for (const KeyValue<String, Input::ActionState>& E : data->action_states) {
		if (E.value.cache.pressed) {
			return true;
		}
	}

	return false;
}

bool Input::is_any_key_pressed()
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return !data->keys_pressed.is_empty();
}

bool Input::is_key_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->keys_pressed.has(p_keycode);
}

bool Input::is_physical_key_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->physical_keys_pressed.has(p_keycode);
}

bool Input::is_key_label_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->key_label_pressed.has(p_keycode);
}

bool Input::is_mouse_button_pressed(MouseButton p_button)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	if (data->disable_input) {
		return false;
	}

	return (data->mouse_button_mask & (uint32_t)mouse_button_to_mask(p_button)) != 0;
}

bool Input::_should_ignore_joypad_events()
{
	return data->ignore_joypad_on_unfocused_application && !data->application_focused && !data->embedder_focused;
}

static JoyAxis _combine_device(JoyAxis p_value, int p_device)
{
	return JoyAxis((int)p_value | (p_device << 20));
}

static JoyButton _combine_device(JoyButton p_value, int p_device)
{
	return JoyButton((int)p_value | (p_device << 20));
}

bool Input::is_joy_button_pressed(int p_device, JoyButton p_button)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->joy_buttons_pressed.has(_combine_device(p_button, p_device));
}

bool Input::is_action_pressed(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	return E->value.cache.pressed && (p_exact ? E->value.exact : true);
}

bool Input::is_action_just_pressed(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently pressed.
	bool pressed_requirement = data->legacy_just_pressed_behavior ? E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return pressed_requirement &&
			   E->value.pressed_physics_frame == Engine::get_physics_frames();
	}
	else {
		return pressed_requirement &&
			   E->value.pressed_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_pressed_by_event(
	const String& p_action, InputEvent* rp_event, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));
	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently pressed.
	bool pressed_requirement = data->legacy_just_pressed_behavior ? E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return pressed_requirement &&
			   E->value.pressed_physics_frame == Engine::get_physics_frames();
	}
	else {
		return pressed_requirement &&
			   E->value.pressed_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_released(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently released.
	bool released_requirement = data->legacy_just_pressed_behavior ? !E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return released_requirement &&
			   E->value.released_physics_frame == Engine::get_physics_frames();
	}
	else {
		return released_requirement &&
			   E->value.released_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_released_by_event(
	const String& p_action, InputEvent* rp_event, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));
	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently released.
	bool released_requirement = data->legacy_just_pressed_behavior ? !E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return released_requirement &&
			   E->value.released_physics_frame == Engine::get_physics_frames();
	}
	else {
		return released_requirement &&
			   E->value.released_process_frame == Engine::get_process_frames();
	}
}

float Input::get_action_strength(const String& p_action, bool p_exact)
{
	if (!data) {
		return 0.0f;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), 0.0, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return 0.0f;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return 0.0f;
	}

	if (p_exact && E->value.exact == false) {
		return 0.0f;
	}

	return E->value.cache.strength;
}

float Input::get_action_raw_strength(const String& p_action, bool p_exact)
{
	if (!data) {
		return 0.0f;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), 0.0, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return 0.0f;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return 0.0f;
	}

	if (p_exact && E->value.exact == false) {
		return 0.0f;
	}

	return E->value.cache.raw_strength;
}

float Input::get_axis(
	const String& p_negative_action, const String& p_positive_action)
{
	return get_action_strength(p_positive_action) - get_action_strength(p_negative_action);
}

Vector2 Input::get_vector(const String& p_negative_x, const String& p_positive_x,
	const String& p_negative_y, const String& p_positive_y, float p_deadzone)
{
	Vector2 vector =
		Vector2(get_action_raw_strength(p_positive_x) - get_action_raw_strength(p_negative_x),
			get_action_raw_strength(p_positive_y) - get_action_raw_strength(p_negative_y));

	if (p_deadzone < 0.0f) {
		// If the deadzone isn't specified, get it from the average of the actions.
		p_deadzone = 0.25 * (InputMap::action_get_deadzone(p_positive_x) +
								InputMap::action_get_deadzone(p_negative_x) +
								InputMap::action_get_deadzone(p_positive_y) +
								InputMap::action_get_deadzone(p_negative_y));
	}

	// Circular length limiting and deadzone.
	float length = vector.length();
	if (length <= p_deadzone) {
		return Vector2();
	}
	else if (length > 1.0f) {
		return vector / length;
	}
	else {
		// Inverse lerp length to map (p_deadzone, 1) to (0, 1).
		return vector * (Math::inverse_lerp(p_deadzone, 1.0f, length) / length);
	}
}

float Input::get_joy_axis(int p_device, JoyAxis p_axis)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return 0;
	}

	JoyAxis c = _combine_device(p_axis, p_device);
	if (data->_joy_axis.has(c)) {
		return data->_joy_axis[c];
	}
	else {
		return 0;
	}
}

String Input::get_joy_name(int p_idx)
{
	if (!data) {
		return String();
	}
	_THREAD_SAFE_METHOD_
	return data->joy_names[p_idx].name;
}

Vector2 Input::get_joy_vibration_strength(int p_device)
{
	if (!data) {
		return Vector2();
	}
	if (data->joy_vibration.has(p_device)) {
		return Vector2(
			data->joy_vibration[p_device].weak_magnitude, data->joy_vibration[p_device].strong_magnitude);
	}
	else {
		return Vector2(0, 0);
	}
}

uint64_t Input::get_joy_vibration_timestamp(int p_device)
{
	if (!data) {
		return 0;
	}
	if (data->joy_vibration.has(p_device)) {
		return data->joy_vibration[p_device].timestamp;
	}
	else {
		return 0;
	}
}

float Input::get_joy_vibration_duration(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	if (data->joy_vibration.has(p_device)) {
		return data->joy_vibration[p_device].duration;
	}
	else {
		return 0.f;
	}
}

float Input::get_joy_vibration_remaining_duration(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const Joypad* joypad = data->joy_names.getptr(p_device);
	if (joypad == nullptr || !joypad->has_vibration) {
		return 0.f;
	}
	const VibrationInfo* vibration = data->joy_vibration.getptr(p_device);
	if (vibration == nullptr ||
		(vibration->weak_magnitude == 0.f && vibration->strong_magnitude == 0.f) ||
		vibration->duration < 0.f) {
		return 0.f;
	}
	float vibration_duration = vibration->duration;
	if (vibration_duration > 0xFFFF / 1000.f || vibration_duration == 0.f) {
		vibration_duration = 0xFFFF / 1000.f; // SDL_MAX_RUMBLE_DURATION_MS / 1000.f
	}
	return MAX(
		vibration_duration - (OS::get_ticks_usec() - vibration->timestamp) / 1e6,
		0.f);
}

bool Input::is_joy_vibrating(int p_device)
{
	return get_joy_vibration_remaining_duration(p_device) > 0.0f;
}

bool Input::has_joy_vibration(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const Joypad* joypad = data->joy_names.getptr(p_device);
	return joypad != nullptr && joypad->has_vibration;
}

static String _hex_str(uint8_t p_byte)
{
	static const char* dict = "0123456789abcdef";
	char ret[3];
	ret[2] = 0;

	ret[0] = dict[p_byte >> 4];
	ret[1] = dict[p_byte & 0xf];

	return ret;
}

Vector3 Input::get_gravity()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->gravity_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_gravity` is not enabled in project settings.");
	}
#endif

	return data->gravity;
}

Vector3 Input::get_accelerometer()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->accelerometer_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_accelerometer` is not enabled in project settings.");
	}
#endif

	return data->accelerometer;
}

Vector3 Input::get_magnetometer()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->magnetometer_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_magnetometer` is not enabled in project settings.");
	}
#endif

	return data->magnetometer;
}

Vector3 Input::get_gyroscope()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->gyroscope_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_gyroscope` is not enabled in project settings.");
	}
#endif

	return data->gyroscope;
}

Vector2 Input::get_joy_touchpad_finger_position(int p_device, int p_finger, int p_touchpad)
{
	if (!data) {
		return Vector2();
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return Vector2(-1, -1);
	}

	uint16_t index = p_finger | (p_touchpad << 8);
	const TouchpadFingerInfo* finger_info = touch->finger_info.getptr(index);
	if (finger_info == nullptr) {
		return Vector2(-1, -1);
	}

	return finger_info->position;
}

float Input::get_joy_touchpad_finger_pressure(int p_device, int p_finger, int p_touchpad)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return -1.0f;
	}

	uint16_t index = p_finger | (p_touchpad << 8);
	const TouchpadFingerInfo* finger_info = touch->finger_info.getptr(index);
	if (finger_info == nullptr) {
		return -1.0f;
	}

	return finger_info->pressure;
}

Vector<uint32_t> Input::get_joy_touchpad_fingers(int p_device, int p_touchpad)
{
	if (!data) {
		return Vector<uint32_t>();
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return Vector<uint32_t>();
	}

	Vector<uint32_t> result;
	for (const KeyValue<uint16_t, TouchpadFingerInfo>& index : touch->finger_info) {
		int touchpad = index.key >> 8;
		if (touchpad == p_touchpad) {
			result.append(index.key & 0xFF);
		}
	}
	return result;
}

int Input::get_joy_num_touchpads(int p_device)
{
	if (!data) {
		return 0;
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return 0;
	}
	return touch->num_touchpads;
}

void Input::_parse_input_event_impl(const Ref<InputEvent>& p_event, bool p_is_emulated)
{
	// This function does the final delivery of the input event to user land.
	// Regardless where the event came from originally, this has to happen on the main thread.
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());

	// Notes on mouse-touch emulation:
	// - Emulated mouse events are parsed, that is, re-routed to this method, so they make the same
	// effects
	//   as true mouse events. The only difference is the situation is flagged as emulated so they
	//   are not emulated back to touch events in an endless loop.
	// - Emulated touch events are handed right to the main loop (i.e., the SceneTree) because they
	// don't
	//   require additional handling by this class.

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && !k->is_echo() && k->get_keycode() != Key::NONE) {
		if (k->is_pressed()) {
			data->keys_pressed.insert(k->get_keycode());
		}
		else {
			data->keys_pressed.erase(k->get_keycode());
		}
	}
	if (k.is_valid() && !k->is_echo() && k->get_physical_keycode() != Key::NONE) {
		if (k->is_pressed()) {
			data->physical_keys_pressed.insert(k->get_physical_keycode());
		}
		else {
			data->physical_keys_pressed.erase(k->get_physical_keycode());
		}
	}
	if (k.is_valid() && !k->is_echo() && k->get_key_label() != Key::NONE) {
		if (k->is_pressed()) {
			data->key_label_pressed.insert(k->get_key_label());
		}
		else {
			data->key_label_pressed.erase(k->get_key_label());
		}
	}

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		if (mb->is_pressed()) {
			data->mouse_button_mask |= (uint32_t)mouse_button_to_mask(mb->get_button_index());
		}
		else {
			data->mouse_button_mask &= ~(uint32_t)(mouse_button_to_mask(mb->get_button_index()));
		}

		Point2 pos = mb->get_global_position();
		if (data->mouse_pos != pos) {
			set_mouse_position(pos);
		}

		if (data->event_dispatch_function && data->emulate_touch_from_mouse && !p_is_emulated &&
			mb->get_button_index() == MouseButton::LEFT) {
			Ref<InputEventScreenTouch> touch_event;
			touch_event.instantiate();
			touch_event->set_pressed(mb->is_pressed());
			touch_event->set_canceled(mb->is_canceled());
			touch_event->set_position(mb->get_position());
			touch_event->set_double_tap(mb->is_double_click());
			touch_event->set_window_id(mb->get_window_id());
			touch_event->set_device(InputEvent::DEVICE_ID_EMULATION);
			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(touch_event);
			_THREAD_SAFE_LOCK_
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		Point2 position = mm->get_global_position();
		if (data->mouse_pos != position) {
			set_mouse_position(position);
		}
		Vector2 relative = mm->get_relative();
		Vector2 screen_relative = mm->get_relative_screen_position();
		data->mouse_velocity_track.update(relative, screen_relative);

		if (data->event_dispatch_function && data->emulate_touch_from_mouse && !p_is_emulated &&
			(mm->get_button_mask() & (uint32_t)(MouseButtonMask::LEFT)) != 0) {
			Ref<InputEventScreenDrag> drag_event;
			drag_event.instantiate();

			drag_event->set_position(position);
			drag_event->set_relative(relative);
			drag_event->set_relative_screen_position(screen_relative);
			drag_event->set_tilt(mm->get_tilt());
			drag_event->set_pen_inverted(mm->get_pen_inverted());
			drag_event->set_pressure(mm->get_pressure());
			drag_event->set_velocity(get_last_mouse_velocity());
			drag_event->set_screen_velocity(get_last_mouse_screen_velocity());
			drag_event->set_device(InputEvent::DEVICE_ID_EMULATION);

			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(drag_event);
			_THREAD_SAFE_LOCK_
		}
	}

	Ref<InputEventScreenTouch> st = p_event;

	if (st.is_valid()) {
		if (st->is_pressed()) {
			VelocityTrack& track = data->touch_velocity_track[st->get_index()];
			track.reset();
		}
		else {
			// Since a pointer index may not occur again (OSs may or may not reuse them),
			// imperatively remove it from the map to keep no fossil entries in it
			data->touch_velocity_track.erase(st->get_index());
		}

		if (data->emulate_mouse_from_touch) {
			bool translate = false;
			if (st->is_pressed()) {
				if (data->mouse_from_touch_index == -1) {
					translate = true;
					data->mouse_from_touch_index = st->get_index();
				}
			}
			else {
				if (st->get_index() == data->mouse_from_touch_index) {
					translate = true;
					data->mouse_from_touch_index = -1;
				}
			}

			if (translate) {
				Ref<InputEventMouseButton> button_event;
				button_event.instantiate();

				button_event->set_device(InputEvent::DEVICE_ID_EMULATION);
				button_event->set_position(st->get_position());
				button_event->set_global_position(st->get_position());
				button_event->set_pressed(st->is_pressed());
				button_event->set_canceled(st->is_canceled());
				button_event->set_button_index(MouseButton::LEFT);
				button_event->set_double_click(st->is_double_tap());
				button_event->set_window_id(st->get_window_id());

				uint32_t ev_bm = data->mouse_button_mask;
				if (st->is_pressed()) {
					ev_bm |= (uint32_t)(MouseButtonMask::LEFT);
				}
				else {
					ev_bm &= ~(uint32_t)(MouseButtonMask::LEFT);
				}
				button_event->set_button_mask(ev_bm);

				_parse_input_event_impl(button_event, true);
			}
		}
	}

	Ref<InputEventScreenDrag> sd = p_event;

	if (sd.is_valid()) {
		VelocityTrack& track = data->touch_velocity_track[sd->get_index()];
		track.update(sd->get_relative(), sd->get_relative_screen_position());
		sd->set_velocity(track.velocity);
		sd->set_screen_velocity(track.screen_velocity);

		if (data->emulate_mouse_from_touch && sd->get_index() == data->mouse_from_touch_index) {
			Ref<InputEventMouseMotion> motion_event;
			motion_event.instantiate();

			motion_event->set_device(InputEvent::DEVICE_ID_EMULATION);
			motion_event->set_tilt(sd->get_tilt());
			motion_event->set_pen_inverted(sd->get_pen_inverted());
			motion_event->set_pressure(sd->get_pressure());
			motion_event->set_position(sd->get_position());
			motion_event->set_global_position(sd->get_position());
			motion_event->set_relative(sd->get_relative());
			motion_event->set_relative_screen_position(sd->get_relative_screen_position());
			motion_event->set_velocity(sd->get_velocity());
			motion_event->set_screen_velocity(sd->get_screen_velocity());
			motion_event->set_button_mask(data->mouse_button_mask);
			motion_event->set_window_id(sd->get_window_id());

			_parse_input_event_impl(motion_event, true);
		}
	}

	Ref<InputEventJoypadButton> jb = p_event;

	if (jb.is_valid()) {
		JoyButton c = _combine_device(jb->get_button_index(), jb->get_device());

		if (jb->is_pressed()) {
			data->joy_buttons_pressed.insert(c);
		}
		else {
			data->joy_buttons_pressed.erase(c);
		}
	}

	Ref<InputEventJoypadMotion> jm = p_event;

	if (jm.is_valid()) {
		set_joy_axis(jm->get_device(), jm->get_axis(), jm->get_axis_value());
	}

	Ref<InputEventGesture> ge = p_event;

	if (ge.is_valid()) {
		if (data->event_dispatch_function) {
			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(ge);
			_THREAD_SAFE_LOCK_
		}
	}

	for (const KeyValue<String, InputMap::Action>& E : InputMap::get_action_map()) {
		const int event_index = InputMap::event_get_index(p_event, E.key);
		if (event_index == -1) {
			continue;
		}
		ERR_FAIL_COND_MSG(event_index >= (int)MAX_EVENT,
			vformat("Input singleton does not support more than %d events assigned to an action.",
				MAX_EVENT));

		int device_id = p_event->get_device();
		bool is_pressed = p_event->is_action_pressed(E.key, true);
		ActionState& action_state = data->action_states[E.key];

		// Update the action's per-device state.
		ActionState::DeviceState& device_state = action_state.device_states[device_id];
		device_state.pressed[event_index] = is_pressed;
		device_state.strength[event_index] = p_event->get_action_strength(E.key);
		device_state.raw_strength[event_index] = p_event->get_action_raw_strength(E.key);
		device_state.event_type[event_index] = p_event->get_type();

		// Update the action's global state and cache.
		if (!is_pressed) {
			action_state.api_pressed =
				false; // Always release the event from action_press() method.
			action_state.api_strength = 0.0;
		}
		action_state.exact = InputMap::event_is_action(p_event.ptr(), E.key, true);

		bool was_pressed = action_state.cache.pressed;
		_update_action_cache(E.key, action_state);
		// As input may come in part way through a physics tick, the earliest we can react to it is
		// the next physics tick.
		if (action_state.cache.pressed && !was_pressed) {
			action_state.pressed_physics_frame = Engine::get_physics_frames() + 1;
			action_state.pressed_process_frame = Engine::get_process_frames();
		}
		if (!action_state.cache.pressed && was_pressed) {
			action_state.released_physics_frame = Engine::get_physics_frames() + 1;
			action_state.released_process_frame = Engine::get_process_frames();
		}
	}

	if (data->event_dispatch_function) {
		_THREAD_SAFE_UNLOCK_
		data->event_dispatch_function(p_event);
		_THREAD_SAFE_LOCK_
	}
}

void Input::set_joy_axis(int p_device, JoyAxis p_axis, float p_value)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	JoyAxis c = _combine_device(p_axis, p_device);
	data->_joy_axis[c] = p_value;
}

void Input::set_joy_features(int p_device, JoypadFeatures* p_features)
{
	if (!data) {
		return;
	}
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad) {
		return;
	}
	joypad->features = p_features;
	_update_joypad_features(p_device);
}

void Input::set_joy_light(int p_device, const Color& p_color)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad || !joypad->has_light || joypad->features == nullptr) {
		return;
	}
	Color linear = p_color.srgb_to_linear();
	joypad->features->set_joy_light(linear);
}

bool Input::has_joy_light(int p_device)
{
	if (!data) {
		return false;
	}
	const Joypad* joypad = data->joy_names.getptr(p_device);
	return joypad && joypad->has_light;
}

Vector3 Input::get_joy_accelerometer(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_acceleration_data[3];
	motion->gamepad_motion->GetProcessedAcceleration(
		joy_acceleration_data[0], joy_acceleration_data[1], joy_acceleration_data[2]);
	Vector3 joy_acceleration(
		joy_acceleration_data[0], joy_acceleration_data[1], joy_acceleration_data[2]);

	float joy_gravity_data[3];
	motion->gamepad_motion->GetGravity(
		joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);
	Vector3 joy_gravity(joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);

	return (-joy_acceleration + joy_gravity) * STANDARD_GRAVITY;
}

Vector3 Input::get_joy_gravity(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_gravity_data[3];
	motion->gamepad_motion->GetGravity(
		joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);
	Vector3 joy_gravity(joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);

	return joy_gravity.normalized() * STANDARD_GRAVITY;
}

Vector3 Input::get_joy_gyroscope(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_gyro_data[3];
	motion->gamepad_motion->GetCalibratedGyro(joy_gyro_data[0], joy_gyro_data[1], joy_gyro_data[2]);
	Vector3 joy_gyro(joy_gyro_data[0], joy_gyro_data[1], joy_gyro_data[2]);

	return joy_gyro * M_PI / 180.0;
}

void Input::set_joy_motion_sensors_enabled(int p_device, bool p_enable)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (joypad == nullptr || joypad->features == nullptr) {
		return;
	}
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}
	joypad->features->set_joy_motion_sensors_enabled(p_enable);
	motion->sensors_enabled = p_enable;
}

bool Input::is_joy_motion_sensors_enabled(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	return motion != nullptr && motion->sensors_enabled;
}

bool Input::has_joy_motion_sensors(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	return data->joy_motion.has(p_device);
}

float Input::get_joy_motion_sensors_rate(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return 0.0f;
	}
	return motion->sensor_data_rate;
}

void Input::start_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(!motion->sensors_enabled, "Motion sensors are not enabled on the joypad.");
	ERR_FAIL_COND_MSG(motion->calibrating, "Calibration already in progress.");

	motion->gamepad_motion->ResetContinuousCalibration();
	motion->gamepad_motion->StartContinuousCalibration();

	motion->calibrating = true;
	motion->calibrated = false;
}

void Input::stop_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(!motion->sensors_enabled, "Motion sensors are not enabled on the joypad.");
	ERR_FAIL_COND_MSG(!motion->calibrating, "Calibration hasn't been started.");

	motion->gamepad_motion->PauseContinuousCalibration();

	motion->calibrating = false;
	motion->calibrated = true;
}

void Input::clear_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	// Calibration might be in progress and the developer or the user might want to reset it,
	// so no need to stop the calibration.

	motion->gamepad_motion->ResetContinuousCalibration();
}

bool Input::is_joy_motion_sensors_calibrating(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return false;
	}
	return motion->calibrating;
}

bool Input::is_joy_motion_sensors_calibrated(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return false;
	}
	return motion->calibrated;
}

void Input::set_joy_motion_sensors_rate(int p_device, float p_rate)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	motion->sensor_data_rate = p_rate;
}

void Input::start_joy_vibration(
	int p_device, float p_weak_magnitude, float p_strong_magnitude, float p_duration)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	if (p_weak_magnitude < 0.f || p_weak_magnitude > 1.f || p_strong_magnitude < 0.f ||
		p_strong_magnitude > 1.f) {
		return;
	}
	VibrationInfo vibration;
	vibration.weak_magnitude = p_weak_magnitude;
	vibration.strong_magnitude = p_strong_magnitude;
	vibration.duration = p_duration;
	vibration.timestamp = OS::get_ticks_usec();
	data->joy_vibration[p_device] = vibration;
}

void Input::stop_joy_vibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	VibrationInfo vibration;
	vibration.weak_magnitude = 0;
	vibration.strong_magnitude = 0;
	vibration.duration = 0;
	vibration.timestamp = OS::get_ticks_usec();
	data->joy_vibration[p_device] = vibration;
}

void Input::vibrate_handheld(int p_duration_ms, float p_amplitude)
{
	OS::vibrate_handheld(p_duration_ms, p_amplitude);
}

void Input::set_ignore_joypad_on_unfocused_application(bool p_ignore)
{
	if (!data) {
		return;
	}
	data->ignore_joypad_on_unfocused_application = p_ignore;
	if (_should_ignore_joypad_events()) {
		release_pressed_events();
	}
}

bool Input::is_ignoring_joypad_on_unfocused_application()
{
	return data ? data->ignore_joypad_on_unfocused_application : false;
}

void Input::set_application_focused(bool p_focused)
{
	if (data) {
		data->application_focused = p_focused;
	}
}

void Input::set_gravity(const Vector3& p_gravity)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->gravity = p_gravity;
}

void Input::set_accelerometer(const Vector3& p_accel)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->accelerometer = p_accel;
}

void Input::set_magnetometer(const Vector3& p_magnetometer)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->magnetometer = p_magnetometer;
}

void Input::set_gyroscope(const Vector3& p_gyroscope)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->gyroscope = p_gyroscope;
}

void Input::set_mouse_position(const Point2& p_posf)
{
	if (data) {
		data->mouse_pos = p_posf;
	}
}

Point2 Input::get_mouse_position() { return data ? data->mouse_pos : Point2(); }

Point2 Input::get_last_mouse_velocity()
{
	if (!data) {
		return Point2();
	}
	data->mouse_velocity_track.update(Vector2(), Vector2());
	return data->mouse_velocity_track.velocity;
}

Point2 Input::get_last_mouse_screen_velocity()
{
	if (!data) {
		return Point2();
	}
	data->mouse_velocity_track.update(Vector2(), Vector2());
	return data->mouse_velocity_track.screen_velocity;
}

uint32_t Input::get_mouse_button_mask()
{
	if (!data) {
		return 0;
	}
	return data->mouse_button_mask; // do not trust OS implementation, should remove it -
							  // OS::get_mouse_button_state();
}

void Input::warp_mouse(const Vector2& p_position)
{
	if (warp_mouse_func) {
		warp_mouse_func(p_position);
	}
}

Point2 Input::warp_mouse_motion(const Ref<InputEventMouseMotion>& p_motion, const Rect2& p_rect)
{
	// The relative distance reported for the next event after a warp is in the boundaries of the
	// size of the rect on that axis, but it may be greater, in which case there's no problem as
	// fmod() will warp it, but if the pointer has moved in the opposite direction between the
	// pointer relocation and the subsequent event, the reported relative distance will be less than
	// the size of the rect and thus fmod() will be disabled for handling the situation. And due to
	// this mouse warping mechanism being stateless, we need to apply some heuristics to detect the
	// warp: if the relative distance is greater than the half of the size of the relevant rect
	// (checked per each axis), it will be considered as the consequence of a former pointer warp.

	const Point2 rel_sign(
		p_motion->get_relative().x >= 0.0f ? 1 : -1, p_motion->get_relative().y >= 0.0 ? 1 : -1);
	const Size2 warp_margin = p_rect.size * 0.5f;
	const Point2 rel_warped(
		Math::fmod(p_motion->get_relative().x + rel_sign.x * warp_margin.x, p_rect.size.x) -
			rel_sign.x * warp_margin.x,
		Math::fmod(p_motion->get_relative().y + rel_sign.y * warp_margin.y, p_rect.size.y) -
			rel_sign.y * warp_margin.y);

	const Point2 pos_local = p_motion->get_global_position() - p_rect.position;
	const Point2 pos_warped(
		Math::fposmod(pos_local.x, p_rect.size.x), Math::fposmod(pos_local.y, p_rect.size.y));
	if (pos_warped != pos_local) {
		warp_mouse(pos_warped + p_rect.position);
	}

	return rel_warped;
}

void Input::action_press(const String& p_action, float p_strength)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(!InputMap::has_action(p_action), InputMap::suggest_actions(p_action));

	// Create or retrieve existing action.
	ActionState& action_state = data->action_states[p_action];

	// As input may come in part way through a physics tick, the earliest we can react to it is the
	// next physics tick.
	if (!action_state.cache.pressed) {
		action_state.pressed_physics_frame = Engine::get_physics_frames() + 1;
		action_state.pressed_process_frame = Engine::get_process_frames();
	}
	action_state.exact = true;
	action_state.api_pressed = true;
	action_state.api_strength = CLAMP(p_strength, 0.0f, 1.0f);
	_update_action_cache(p_action, action_state);
}

void Input::action_release(const String& p_action)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(!InputMap::has_action(p_action), InputMap::suggest_actions(p_action));

	// Create or retrieve existing action.
	ActionState& action_state = data->action_states[p_action];
	action_state.cache.pressed = false;
	action_state.cache.strength = 0.0;
	action_state.cache.raw_strength = 0.0;
	// As input may come in part way through a physics tick, the earliest we can react to it is the
	// next physics tick.
	action_state.released_physics_frame = Engine::get_physics_frames() + 1;
	action_state.released_process_frame = Engine::get_process_frames();
	action_state.device_states.clear();
	action_state.exact = true;
	action_state.api_pressed = false;
	action_state.api_strength = 0.0;
}

void Input::set_emulate_touch_from_mouse(bool p_emulate)
{
	if (data) {
		data->emulate_touch_from_mouse = p_emulate;
	}
}

bool Input::is_emulating_touch_from_mouse() { return data ? data->emulate_touch_from_mouse : false; }

// Calling this whenever the game window is focused helps unsticking the "touch mouse"
// if the OS or its abstraction class hasn't properly reported that touch pointers raised
void Input::ensure_touch_mouse_raised()
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	if (data->mouse_from_touch_index != -1) {
		data->mouse_from_touch_index = -1;

		Ref<InputEventMouseButton> button_event;
		button_event.instantiate();

		button_event->set_device(InputEvent::DEVICE_ID_EMULATION);
		button_event->set_position(data->mouse_pos);
		button_event->set_global_position(data->mouse_pos);
		button_event->set_pressed(false);
		button_event->set_button_index(MouseButton::LEFT);
		uint32_t ev_bm = data->mouse_button_mask;
		ev_bm &= ~(uint32_t)(MouseButtonMask::LEFT);
		button_event->set_button_mask(ev_bm);

		_parse_input_event_impl(button_event, true);
	}
}

void Input::set_emulate_mouse_from_touch(bool p_emulate)
{
	if (data) {
		data->emulate_mouse_from_touch = p_emulate;
	}
}

bool Input::is_emulating_mouse_from_touch() { return data ? data->emulate_mouse_from_touch : false; }

Input::CursorShape Input::get_default_cursor_shape() { return data ? data->default_shape : CursorShape::CURSOR_ARROW; }

void Input::set_default_cursor_shape(CursorShape p_shape)
{
	if (!data) {
		return;
	}
	if (data->default_shape == p_shape) {
		return;
	}

	data->default_shape = p_shape;
	// The default shape is set in Viewport::_gui_input_event. To instantly
	// see the shape in the viewport we need to trigger a mouse motion event.
	Ref<InputEventMouseMotion> mm;
	mm.instantiate();
	mm->set_position(data->mouse_pos);
	mm->set_global_position(data->mouse_pos);
	mm->set_device(InputEvent::DEVICE_ID_INTERNAL);
	parse_input_event(mm.ptr());
}

Input::CursorShape Input::get_current_cursor_shape()
{
	return get_current_cursor_shape_func ? get_current_cursor_shape_func()
										 : CursorShape::CURSOR_ARROW;
}

void Input::set_custom_mouse_cursor(
	const Ref<Resource>& p_cursor, CursorShape p_shape, const Vector2& p_hotspot)
{
	if (Engine::is_editor_hint()) {
		return;
	}

	ERR_FAIL_INDEX(p_shape, CursorShape::CURSOR_MAX);

	if (set_custom_mouse_cursor_func) {
		set_custom_mouse_cursor_func(p_cursor, p_shape, p_hotspot);
	}
}

void Input::parse_input_event(InputEvent* rp_event)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
#ifdef DEBUG_ENABLED
	uint64_t curr_frame = Engine::get_process_frames();
	if (curr_frame != data->last_parsed_frame) {
		data->frame_parsed_events.clear();
		data->last_parsed_frame = curr_frame;
		data->frame_parsed_events.insert(rp_event);
	}
	else if (data->frame_parsed_events.has(rp_event)) {
		// It would be technically safe to send the same event in cases such as:
		// - After an explicit flush.
		// - In platforms using buffering when agile flushing is enabled, after one of the mid-frame
		// flushes.
		// - If platform doesn't use buffering and event accumulation is disabled.
		// - If platform doesn't use buffering and the event type is not accumulable.
		// However, it wouldn't be reasonable to ask users to remember the full ruleset and be aware
		// at all times of the possibilities of the target platform, project settings and engine
		// internals, which may change without prior notice. Therefore, the guideline is, "don't
		// send the same event object more than once per frame".
		WARN_PRINT_ONCE(
			"An input event object is being parsed more than once in the same frame, which is "
			"unsafe.\n"
			"If you are generating events in a script, you have to instantiate a new event instead "
			"of sending the same one more than once, unless the original one was sent on an "
			"earlier frame.\n"
			"You can call duplicate() on the event to get a new instance with identical values.");
	}
	else {
		data->frame_parsed_events.insert(rp_event);
	}
#endif

	if (data->use_accumulated_input) {
		if (data->buffered_events.is_empty() || !data->buffered_events.back()->get()->accumulate(rp_event)) {
			data->buffered_events.push_back(rp_event);
		}
	}
	else if (data->agile_input_event_flushing) {
		data->buffered_events.push_back(rp_event);
	}
	else {
		_parse_input_event_impl(rp_event, false);
	}
}

#ifdef DEBUG_ENABLED
void Input::flush_frame_parsed_events()
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->frame_parsed_events.clear();
}
#endif

void Input::flush_buffered_events()
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	while (data->buffered_events.front()) {
		// The final delivery of the input event involves releasing the lock.
		// While the lock is released, another thread may lock it and add new events to the back.
		// Therefore, we get each event and pop it while we still have the lock,
		// to ensure the list is in a consistent state.
		List<Ref<InputEvent>>::Element* E = data->buffered_events.front();
		Ref<InputEvent> e = E->get();
		data->buffered_events.pop_front();

		_parse_input_event_impl(e, false);
	}
}

bool Input::is_agile_input_event_flushing() { return data ? data->agile_input_event_flushing : false; }

void Input::set_agile_input_event_flushing(bool p_enable)
{
	if (data) {
		data->agile_input_event_flushing = p_enable;
	}
}

void Input::set_use_accumulated_input(bool p_enable)
{
	if (data) {
		data->use_accumulated_input = p_enable;
	}
}

bool Input::is_using_accumulated_input() { return data ? data->use_accumulated_input : false; }

void Input::release_pressed_events()
{
	if (!data) {
		return;
	}
	// Don't release the events if the application (or the window it's embedded in) is still
	// focused.
	if (data->application_focused || data->embedder_focused) {
		return;
	}

	flush_buffered_events(); // this is needed to release actions strengths

	data->keys_pressed.clear();
	data->physical_keys_pressed.clear();
	data->key_label_pressed.clear();
	if (data->ignore_joypad_on_unfocused_application) {
		data->joy_buttons_pressed.clear();
		data->_joy_axis.clear();

		for (KeyValue<String, Input::ActionState>& E : data->action_states) {
			if (E.value.cache.pressed) {
				action_release(E.key);
			}
		}
	}
	else {
		for (KeyValue<String, Input::ActionState>& E : data->action_states) {
			if (E.value.cache.pressed) {
				bool is_only_joypad_pressed = true;
				int max_event = InputMap::action_get_events(E.key)->size() +
								1; // +1 comes from InputEventAction.

				for (const KeyValue<int, ActionState::DeviceState>& kv : E.value.device_states) {
					const ActionState::DeviceState& device_state = kv.value;
					for (int i = 0; i < max_event; i++) {
						if (device_state.event_type[i] != InputEventType::JOY_MOTION &&
							device_state.event_type[i] != InputEventType::JOY_BUTTON &&
							device_state.strength[i] > 0.0f) {
							is_only_joypad_pressed = false;
							action_release(E.key);
							break;
						}
					}
					if (!is_only_joypad_pressed) {
						break;
					}
				}
			}
		}
	}
}

void Input::set_event_dispatch_function(EventDispatchFunc p_function)
{
	if (!data) {
		return;
	}
	data->event_dispatch_function = p_function;
}

void Input::joy_button(int p_device, JoyButton p_button, bool p_pressed)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	Joypad& joy = data->joy_names[p_device];
	ERR_FAIL_INDEX((int)p_button, (int)JoyButton::MAX);

	if (joy.last_buttons[(size_t)p_button] == p_pressed) {
		return;
	}
	joy.last_buttons[(size_t)p_button] = p_pressed;
	if (joy.mapping == -1) {
		_button_event(p_device, p_button, p_pressed);
		return;
	}

	JoyEvent map = _get_mapped_button_event(data->map_db[joy.mapping], p_button);

	if (map.type == TYPE_BUTTON) {
		_button_event(p_device, (JoyButton)map.index, p_pressed);
		return;
	}

	if (map.type == TYPE_AXIS) {
		_axis_event(p_device, (JoyAxis)map.index, p_pressed ? map.value : 0.0);
	}
	// no event?
}

void Input::joy_axis(int p_device, JoyAxis p_axis, float p_value)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	ERR_FAIL_INDEX((int)p_axis, (int)JoyAxis::MAX);

	Joypad& joy = data->joy_names[p_device];

	if (joy.last_axis[(size_t)p_axis] == p_value) {
		return;
	}

	joy.last_axis[(size_t)p_axis] = p_value;

	if (joy.mapping == -1) {
		_axis_event(p_device, p_axis, p_value);
		return;
	}

	JoyAxisRange range;
	JoyEvent map = _get_mapped_axis_event(data->map_db[joy.mapping], p_axis, p_value, range);

	if (map.type == TYPE_BUTTON) {
		bool pressed = map.value > 0.5;
		if (pressed != data->joy_buttons_pressed.has(_combine_device((JoyButton)map.index, p_device))) {
			_button_event(p_device, (JoyButton)map.index, pressed);
		}

		// Ensure opposite D-Pad button is also released.
		switch ((JoyButton)map.index) {
		case JoyButton::DPAD_UP:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_DOWN, p_device))) {
				_button_event(p_device, JoyButton::DPAD_DOWN, false);
			}
			break;
		case JoyButton::DPAD_DOWN:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_UP, p_device))) {
				_button_event(p_device, JoyButton::DPAD_UP, false);
			}
			break;
		case JoyButton::DPAD_LEFT:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_RIGHT, p_device))) {
				_button_event(p_device, JoyButton::DPAD_RIGHT, false);
			}
			break;
		case JoyButton::DPAD_RIGHT:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_LEFT, p_device))) {
				_button_event(p_device, JoyButton::DPAD_LEFT, false);
			}
			break;
		default:
			// Nothing to do.
			break;
		}
		return;
	}

	if (map.type == TYPE_AXIS) {
		JoyAxis axis = JoyAxis(map.index);
		float value = map.value;
#ifndef ANDROID_ENABLED // Android trigger values are already between 0.0f and 1.0f.
		if (range == FULL_AXIS &&
			(axis == JoyAxis::TRIGGER_LEFT || axis == JoyAxis::TRIGGER_RIGHT)) {
			// Convert to a value between 0.0f and 1.0f.
			value = 0.5f + value / 2.0f;
		}
#endif
		_axis_event(p_device, axis, value);
		return;
	}
}

void Input::joy_hat(int p_device, uint32_t p_val)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	const Joypad& joy = data->joy_names[p_device];

	JoyEvent map[(size_t)HatDir::MAX];

	map[(size_t)HatDir::UP].type = TYPE_BUTTON;
	map[(size_t)HatDir::UP].index = (int)JoyButton::DPAD_UP;
	map[(size_t)HatDir::UP].value = 0;

	map[(size_t)HatDir::RIGHT].type = TYPE_BUTTON;
	map[(size_t)HatDir::RIGHT].index = (int)JoyButton::DPAD_RIGHT;
	map[(size_t)HatDir::RIGHT].value = 0;

	map[(size_t)HatDir::DOWN].type = TYPE_BUTTON;
	map[(size_t)HatDir::DOWN].index = (int)JoyButton::DPAD_DOWN;
	map[(size_t)HatDir::DOWN].value = 0;

	map[(size_t)HatDir::LEFT].type = TYPE_BUTTON;
	map[(size_t)HatDir::LEFT].index = (int)JoyButton::DPAD_LEFT;
	map[(size_t)HatDir::LEFT].value = 0;

	if (joy.mapping != -1) {
		_get_mapped_hat_events(data->map_db[joy.mapping], (HatDir)0, map);
	}

	int cur_val = data->joy_names[p_device].hat_current;

	for (int hat_direction = 0, hat_mask = 1; hat_direction < (int)HatDir::MAX;
		 hat_direction++, hat_mask <<= 1) {
		if (((int)p_val & hat_mask) != (cur_val & hat_mask)) {
			if (map[hat_direction].type == TYPE_BUTTON) {
				_button_event(p_device, (JoyButton)map[hat_direction].index, (int)p_val & hat_mask);
			}
			if (map[hat_direction].type == TYPE_AXIS) {
				_axis_event(p_device, (JoyAxis)map[hat_direction].index,
					((int)p_val & hat_mask) ? map[hat_direction].value : 0.0);
			}
		}
	}

	data->joy_names[p_device].hat_current = (int)p_val;
}

void Input::joy_motion_sensors(
	int p_device, const Vector3& p_accelerometer, const Vector3& p_gyroscope)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	// TODO: events
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	Vector3 gyro_degrees = p_gyroscope * 180.0 / M_PI;
	Vector3 accel_g = -p_accelerometer / STANDARD_GRAVITY;
	uint64_t new_timestamp = OS::get_ticks_msec();
	float delta_time = (new_timestamp - motion->last_timestamp) / 1000.0f;
	motion->last_timestamp = new_timestamp;
	motion->gamepad_motion->ProcessMotion(gyro_degrees.x, gyro_degrees.y, gyro_degrees.z, accel_g.x,
		accel_g.y, accel_g.z, delta_time);
}

void Input::joy_touchpad(int p_device, int p_touchpad, int p_finger, const Vector2& p_position,
	float p_pressure, bool p_pressed)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	TouchpadInfo& touch = data->joy_touch[p_device];
	uint16_t index = p_finger | (p_touchpad << 8);
	if (p_pressed) {
		touch.finger_info[index] = TouchpadFingerInfo{p_position, p_pressure};
	}
	else {
		touch.finger_info.erase(index);
	}
}

void Input::_button_event(int p_device, JoyButton p_index, bool p_pressed)
{
	Ref<InputEventJoypadButton> ievent;
	ievent.instantiate();
	ievent->set_device(p_device);
	ievent->set_button_index(p_index);
	ievent->set_pressed(p_pressed);

	parse_input_event(ievent.ptr());
}

void Input::_axis_event(int p_device, JoyAxis p_axis, float p_value)
{
	Ref<InputEventJoypadMotion> ievent;
	ievent.instantiate();
	ievent->set_device(p_device);
	ievent->set_axis(p_axis);
	ievent->set_axis_value(p_value);

	parse_input_event(ievent.ptr());
}

void Input::_update_action_cache(const String& p_action_name, ActionState& r_action_state)
{
	// Update the action cache, computed from the per-device and per-event states.
	r_action_state.cache.pressed = false;
	r_action_state.cache.strength = 0.0;
	r_action_state.cache.raw_strength = 0.0;

	int max_event = InputMap::action_get_events(p_action_name)->size() +
					1; // +1 comes from InputEventAction.
	for (const KeyValue<int, ActionState::DeviceState>& kv : r_action_state.device_states) {
		const ActionState::DeviceState& device_state = kv.value;
		for (int i = 0; i < max_event; i++) {
			r_action_state.cache.pressed = r_action_state.cache.pressed || device_state.pressed[i];
			r_action_state.cache.strength =
				MAX(r_action_state.cache.strength, device_state.strength[i]);
			r_action_state.cache.raw_strength =
				MAX(r_action_state.cache.raw_strength, device_state.raw_strength[i]);
		}
	}

	if (r_action_state.api_pressed) {
		r_action_state.cache.pressed = true;
		r_action_state.cache.strength =
			MAX(r_action_state.cache.strength, r_action_state.api_strength);
		r_action_state.cache.raw_strength = MAX(r_action_state.cache.raw_strength,
			r_action_state
				.api_strength); // Use the strength as raw_strength for API-pressed states.
	}
}

void Input::_update_joypad_features(int p_device)
{
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad || joypad->features == nullptr) {
		return;
	}
	if (joypad->features->has_joy_vibration()) {
		joypad->has_vibration = true;
	}
	if (joypad->features->has_joy_light()) {
		joypad->has_light = true;
	}
	if (joypad->features->has_joy_motion_sensors()) {
		MotionInfo& motion = data->joy_motion[p_device];

		if (!motion.gamepad_motion) {
			motion.gamepad_motion = new GamepadMotion();
		}
		else {
			motion.gamepad_motion->Reset();
		}
		motion.last_timestamp = OS::get_ticks_msec();
	}
	if (joypad->features->get_joy_num_touchpads() > 0) {
		data->joy_touch[p_device].num_touchpads = joypad->features->get_joy_num_touchpads();
	}
}

Input::JoyEvent Input::_get_mapped_button_event(
	const JoyDeviceMapping& p_mapping, JoyButton p_button)
{
	JoyEvent event;

	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_BUTTON && binding.input.button == p_button) {
			event.type = binding.outputType;
			switch (binding.outputType) {
			case TYPE_BUTTON:
				event.index = (int)binding.output.button;
				return event;
			case TYPE_AXIS:
				event.index = (int)binding.output.axis.axis;
				switch (binding.output.axis.range) {
				case POSITIVE_HALF_AXIS:
					event.value = 1;
					break;
				case NEGATIVE_HALF_AXIS:
					event.value = -1;
					break;
				case FULL_AXIS:
					// It doesn't make sense for a button to map to a full axis,
					// but keeping as a default for a trigger with a positive half-axis.
					event.value = 1;
					break;
				}
				return event;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
			}
		}
	}
	return event;
}

Input::JoyEvent Input::_get_mapped_axis_event(
	const JoyDeviceMapping& p_mapping, JoyAxis p_axis, float p_value, JoyAxisRange& r_range)
{
	JoyEvent event;

	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_AXIS && binding.input.axis.axis == p_axis) {
			float value = p_value;
			if (binding.input.axis.invert) {
				value = -value;
			}
			if (binding.input.axis.range == FULL_AXIS ||
				(binding.input.axis.range == POSITIVE_HALF_AXIS && value >= 0) ||
				(binding.input.axis.range == NEGATIVE_HALF_AXIS && value < 0)) {
				event.type = binding.outputType;
				float shifted_positive_value = 0;
				switch (binding.input.axis.range) {
				case POSITIVE_HALF_AXIS:
					shifted_positive_value = value;
					break;
				case NEGATIVE_HALF_AXIS:
					shifted_positive_value = value + 1;
					break;
				case FULL_AXIS:
					shifted_positive_value = (value + 1) / 2;
					break;
				}
				switch (binding.outputType) {
				case TYPE_BUTTON:
					event.index = (int)binding.output.button;
					switch (binding.input.axis.range) {
					case POSITIVE_HALF_AXIS:
						event.value = shifted_positive_value;
						break;
					case NEGATIVE_HALF_AXIS:
						event.value = 1 - shifted_positive_value;
						break;
					case FULL_AXIS:
						// It doesn't make sense for a full axis to map to a button,
						// but keeping as a default for a trigger with a positive half-axis.
						event.value = (shifted_positive_value * 2) - 1;
						break;
					}
					return event;
				case TYPE_AXIS:
					event.index = (int)binding.output.axis.axis;
					event.value = value;
					r_range = binding.output.axis.range;
					if (binding.output.axis.range != binding.input.axis.range) {
						switch (binding.output.axis.range) {
						case POSITIVE_HALF_AXIS:
							event.value = shifted_positive_value;
							break;
						case NEGATIVE_HALF_AXIS:
							event.value = shifted_positive_value - 1;
							break;
						case FULL_AXIS:
							event.value = (shifted_positive_value * 2) - 1;
							break;
						}
					}
					return event;
				default:
					ERR_PRINT_ONCE("Joypad axis mapping error.");
				}
			}
		}
	}
	return event;
}

void Input::_get_mapped_hat_events(
	const JoyDeviceMapping& p_mapping, HatDir p_hat, JoyEvent r_events[(size_t)HatDir::MAX])
{
	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_HAT && binding.input.hat.hat == p_hat) {
			HatDir hat_direction;
			switch (binding.input.hat.hat_mask) {
			case HatMask::UP:
				hat_direction = HatDir::UP;
				break;
			case HatMask::RIGHT:
				hat_direction = HatDir::RIGHT;
				break;
			case HatMask::DOWN:
				hat_direction = HatDir::DOWN;
				break;
			case HatMask::LEFT:
				hat_direction = HatDir::LEFT;
				break;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
				continue;
			}

			r_events[(size_t)hat_direction].type = binding.outputType;
			switch (binding.outputType) {
			case TYPE_BUTTON:
				r_events[(size_t)hat_direction].index = (int)binding.output.button;
				break;
			case TYPE_AXIS:
				r_events[(size_t)hat_direction].index = (int)binding.output.axis.axis;
				switch (binding.output.axis.range) {
				case POSITIVE_HALF_AXIS:
					r_events[(size_t)hat_direction].value = 1;
					break;
				case NEGATIVE_HALF_AXIS:
					r_events[(size_t)hat_direction].value = -1;
					break;
				case FULL_AXIS:
					// It doesn't make sense for a hat direction to map to a full axis,
					// but keeping as a default for a trigger with a positive half-axis.
					r_events[(size_t)hat_direction].value = 1;
					break;
				}
				break;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
			}
		}
	}
}

JoyButton Input::_get_output_button(const String& p_output)
{
	for (int i = 0; i < (int)JoyButton::SDL_MAX; i++) {
		if (p_output == _joy_buttons[i]) {
			return JoyButton(i);
		}
	}
	return JoyButton::INVALID;
}

JoyAxis Input::_get_output_axis(const String& p_output)
{
	for (int i = 0; i < (int)JoyAxis::SDL_MAX; i++) {
		if (p_output == _joy_axes[i]) {
			return JoyAxis(i);
		}
	}
	return JoyAxis::INVALID;
}

void Input::parse_mapping(const String& p_mapping)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;
	JoyDeviceMapping mapping;

	Vector<String> entry = p_mapping.split(",");
	if (entry.size() < 2) {
		return;
	}

	mapping.uid = entry[0];
	mapping.name = entry[1];

	int idx = 1;
	while (++idx < entry.size()) {
		if (entry[idx].is_empty()) {
			continue;
		}

		String output = entry[idx].get_slicec(':', 0).remove_char(' ');
		String input = entry[idx].get_slicec(':', 1).remove_char(' ');
		if (output.length() < 1 || input.length() < 2) {
			continue;
		}

		if (output == "platform" || output == "hint") {
			continue;
		}

		JoyAxisRange output_range = FULL_AXIS;
		if (output[0] == '+' || output[0] == '-') {
			ERR_CONTINUE_MSG(output.length() < 2,
				vformat("Invalid output entry \"%s\" in mapping:\n%s", entry[idx], p_mapping));
			if (output[0] == '+') {
				output_range = POSITIVE_HALF_AXIS;
			}
			else if (output[0] == '-') {
				output_range = NEGATIVE_HALF_AXIS;
			}
			output = output.substr(1);
		}

		JoyAxisRange input_range = FULL_AXIS;
		if (input[0] == '+') {
			input_range = POSITIVE_HALF_AXIS;
			input = input.substr(1);
		}
		else if (input[0] == '-') {
			input_range = NEGATIVE_HALF_AXIS;
			input = input.substr(1);
		}
		bool invert_axis = false;
		if (input[input.length() - 1] == '~') {
			invert_axis = true;
			input = input.left(input.length() - 1);
		}

		JoyButton output_button = _get_output_button(output);
		JoyAxis output_axis = _get_output_axis(output);
		if (output_button == JoyButton::INVALID && output_axis == JoyAxis::INVALID) {
			print_verbose(
				vformat("Unrecognized output string \"%s\" in mapping:\n%s", output, p_mapping));
			continue;
		}
		ERR_CONTINUE_MSG(output_button != JoyButton::INVALID && output_axis != JoyAxis::INVALID,
			vformat("Output string \"%s\" matched both button and axis in mapping:\n%s", output,
				p_mapping));

		JoyBinding binding;
		if (output_button != JoyButton::INVALID) {
			binding.outputType = TYPE_BUTTON;
			binding.output.button = output_button;
		}
		else if (output_axis != JoyAxis::INVALID) {
			binding.outputType = TYPE_AXIS;
			binding.output.axis.axis = output_axis;
			binding.output.axis.range = output_range;
		}

		switch (input[0]) {
		case 'b':
			binding.inputType = TYPE_BUTTON;
			binding.input.button = (JoyButton)input.substr(1).to_int();
			break;
		case 'a':
			binding.inputType = TYPE_AXIS;
			binding.input.axis.axis = (JoyAxis)input.substr(1).to_int();
			binding.input.axis.range = input_range;
			binding.input.axis.invert = invert_axis;
			break;
		case 'h':
			ERR_CONTINUE_MSG(input.length() != 4 || input[2] != '.',
				vformat("Invalid had input \"%s\" in mapping:\n%s", input, p_mapping));
			binding.inputType = TYPE_HAT;
			binding.input.hat.hat = (HatDir)input.substr(1, 1).to_int();
			binding.input.hat.hat_mask = static_cast<HatMask>(input.substr(3).to_int());
			break;
		default:
			ERR_CONTINUE_MSG(true,
				vformat("Unrecognized input string \"%s\" in mapping:\n%s", input, p_mapping));
		}

		mapping.bindings.push_back(binding);
	}

	data->map_db.push_back(mapping);
}

void Input::add_joy_mapping(const String& p_mapping, bool p_update_existing)
{
	if (!data) {
		return;
	}
	parse_mapping(p_mapping);
	if (p_update_existing) {
		const String uid = p_mapping.get_slicec(',', 0);
		for (KeyValue<int, Joypad>& E : data->joy_names) {
			Joypad& joy = E.value;
			if (joy.uid == uid) {
				_set_joypad_mapping(joy, data->map_db.size() - 1);
			}
		}
	}
}

void Input::remove_joy_mapping(const String& p_guid)
{
	if (!data) {
		return;
	}
	// One GUID can exist multiple times in `map_db`, and
	// `add_joy_mapping` can choose not to update the existing mapping,
	// so the indices can be all over the place. Therefore we need to remember them.
	Vector<int> removed_idx;
	int min_removed_idx = -1;
	int max_removed_idx = -1;
	int fallback_mapping_offset = 0;

	for (int i = data->map_db.size() - 1; i >= 0; i--) {
		if (p_guid == data->map_db[i].uid) {
			data->map_db.remove_at(i);

			if (max_removed_idx == -1) {
				max_removed_idx = i;
			}
			min_removed_idx = i;
			removed_idx.push_back(i);

			if (i < data->fallback_mapping) {
				fallback_mapping_offset++;
			}
			else if (i == data->fallback_mapping) {
				data->fallback_mapping = -1;
				WARN_PRINT_ONCE(vformat("Removed fallback joypad input mapping \"%s\". This could "
										"lead to joypads not working as intended.",
					p_guid));
			}
		}
	}

	if (min_removed_idx == -1) {
		return; // Nothing removed.
	}

	if (data->fallback_mapping > 0) {
		// Fix the shifted index.
		data->fallback_mapping -= fallback_mapping_offset;
	}

	int removed_idx_size = removed_idx.size();

	// Update joypad mapping references: some
	// * should use the fallback_mapping (if set; if not, they get unmapped), or
	// * need their mapping reference fixed, because the deletion(s) offset them.
	for (KeyValue<int, Joypad>& E : data->joy_names) {
		Joypad& joy = E.value;
		if (joy.mapping < min_removed_idx) {
			continue; // Not affected.
		}

		if (joy.mapping > max_removed_idx) {
			_set_joypad_mapping(joy, joy.mapping - removed_idx_size);
			continue; // Simple offset fix.
		}

		// removed_idx is in reverse order (ie. high to low), because the first loop is in reverse
		// order.
		for (int i = 0; i < removed_idx.size(); i++) {
			if (removed_idx[i] == joy.mapping) {
				// Set to fallback_mapping, if defined, else unmap the joypad.
				// Currently, the fallback_mapping is only set internally, and only for Android.
				_set_joypad_mapping(joy, data->fallback_mapping);
				break;
			}
			if (removed_idx[i] < joy.mapping) {
				// Complex offset fix:
				// This mapping was shifted by `(removed_idx_size - i)` deletions.
				_set_joypad_mapping(joy, joy.mapping - (removed_idx_size - i));
				break;
			}
		}
	}
}

void Input::_set_joypad_mapping(Joypad& p_js, int p_map_index)
{
	if (p_map_index != data->fallback_mapping && p_map_index >= 0 && p_map_index < data->map_db.size() &&
		p_js.uid != "__XINPUT_DEVICE__") {
		// Prefer the joypad name defined in the mapping.
		// Exceptions:
		// * On Windows for XInput devices the mapping would change the joypad's name to a
		// collective name.
		// * A fallback mapping is not allowed to override the joypad's name.
		p_js.name = data->map_db[p_map_index].name;
	}
	p_js.mapping = p_map_index;
}

void Input::set_fallback_mapping(const String& p_guid)
{
	if (!data) {
		return;
	}
	for (int i = 0; i < data->map_db.size(); i++) {
		if (data->map_db[i].uid == p_guid) {
			data->fallback_mapping = i;
			return;
		}
	}
}

// platforms that use the remapping system can override and call to these ones
bool Input::is_joy_known(int p_device)
{
	if (!data) {
		return false;
	}
	return data->joy_names.has(p_device) && data->joy_names[p_device].is_known;
}

String Input::get_joy_guid(int p_device)
{
	if (!data) {
		return String();
	}
	ERR_FAIL_COND_V(!data->joy_names.has(p_device), "");
	return data->joy_names[p_device].uid;
}

bool Input::should_ignore_device(int p_vendor_id, int p_product_id)
{
	if (!data) {
		return false;
	}
	uint32_t full_id = (((uint32_t)p_vendor_id) << 16) | ((uint16_t)p_product_id);
	return data->ignored_device_ids.has(full_id);
}

int Input::get_unused_joy_id()
{
	if (!data) {
		return -1;
	}
	for (int i = 0; i < JOYPADS_MAX; i++) {
		if (!data->joy_names.has(i) || !data->joy_names[i].connected) {
			return i;
		}
	}
	return -1;
}

void Input::set_disable_input(bool p_disable)
{
	if (data) {
		data->disable_input = p_disable;
	}
}

bool Input::is_input_disabled() { return data ? data->disable_input : false; }

void Input::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);

	// Parse default mappings.
	{
		int i = 0;
		while (DefaultControllerMappings::mappings[i]) {
			parse_mapping(DefaultControllerMappings::mappings[i++]);
		}
	}

	// If defined, parse SDL_GAMECONTROLLERCONFIG for possible new mappings/overrides.
	String env_mapping = OS::get_environment("SDL_GAMECONTROLLERCONFIG");
	if (!env_mapping.is_empty()) {
		Vector<String> entries = env_mapping.split("\n");
		for (int i = 0; i < entries.size(); i++) {
			if (entries[i].is_empty()) {
				continue;
			}
			parse_mapping(entries[i]);
		}
	}

	String env_ignore_devices =
		OS::get_environment("SDL_GAMECONTROLLER_IGNORE_DEVICES");
	if (!env_ignore_devices.is_empty()) {
		Vector<String> entries = env_ignore_devices.split(",");
		for (int i = 0; i < entries.size(); i++) {
			Vector<String> vid_pid = entries[i].split("/");

			if (vid_pid.size() < 2) {
				continue;
			}

			print_verbose(
				vformat("Device Ignored -- Vendor: %s Product: %s", vid_pid[0], vid_pid[1]));
			const uint16_t vid_unswapped = vid_pid[0].hex_to_int();
			const uint16_t pid_unswapped = vid_pid[1].hex_to_int();
			const uint16_t vid = BSWAP16(vid_unswapped);
			const uint16_t pid = BSWAP16(pid_unswapped);

			uint32_t full_id = (((uint32_t)vid) << 16) | ((uint16_t)pid);
			data->ignored_device_ids.insert(full_id);
		}
	}

	if (Engine::is_editor_hint()) {
		// Always use standard behavior in the editor.
		data->legacy_just_pressed_behavior = false;
	}
}

void Input::finalize()
{
	if (data == nullptr) {
		return;
	}
	for (KeyValue<int, MotionInfo>& E : data->joy_motion) {
		delete E.value.gamepad_motion;
		E.value.gamepad_motion = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

//////////////////////////////////////////////////////////


