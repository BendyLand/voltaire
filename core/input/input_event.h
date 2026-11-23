/**************************************************************************/
/*  input_event.h                                                         */
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

#pragma once

// --- Inlined/Folded from core/input/input_enums.h ---
#include "core/error/error_macros.h"

enum class InputEventType
{
	INVALID = -1,
	KEY,
	MOUSE_BUTTON,
	MOUSE_MOTION,
	JOY_MOTION,
	JOY_BUTTON,
	SCREEN_TOUCH,
	SCREEN_DRAG,
	MAGNIFY_GESTURE,
	PAN_GESTURE,
	MIDI,
	SHORTCUT,
	ACTION,
	MAX,
};

enum class HatDir
{
	UP = 0,
	RIGHT = 1,
	DOWN = 2,
	LEFT = 3,
	MAX = 4,
};

enum class HatMask
{
	CENTER = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 4,
	LEFT = 8,
};

enum class JoyAxis
{
	INVALID = -1,
	LEFT_X = 0,
	LEFT_Y = 1,
	RIGHT_X = 2,
	RIGHT_Y = 3,
	TRIGGER_LEFT = 4,
	TRIGGER_RIGHT = 5,
	SDL_MAX = 6,
	MAX = 10, // OpenVR supports up to 5 Joysticks making a total of 10 axes.
};

enum class JoyButton
{
	INVALID = -1,
	A = 0,
	B = 1,
	X = 2,
	Y = 3,
	BACK = 4,
	GUIDE = 5,
	START = 6,
	LEFT_STICK = 7,
	RIGHT_STICK = 8,
	LEFT_SHOULDER = 9,
	RIGHT_SHOULDER = 10,
	DPAD_UP = 11,
	DPAD_DOWN = 12,
	DPAD_LEFT = 13,
	DPAD_RIGHT = 14,
	MISC1 = 15,
	PADDLE1 = 16,
	PADDLE2 = 17,
	PADDLE3 = 18,
	PADDLE4 = 19,
	TOUCHPAD = 20,
	MISC2 = 21,
	MISC3 = 22,
	MISC4 = 23,
	MISC5 = 24,
	MISC6 = 25,
	SDL_MAX = 26,
	MAX = 128, // Android supports up to 36 buttons. DirectInput supports up to 128 buttons.
};

enum class MIDIMessage
{
	NONE = 0,
	NOTE_OFF = 0x8,
	NOTE_ON = 0x9,
	AFTERTOUCH = 0xA,
	CONTROL_CHANGE = 0xB,
	PROGRAM_CHANGE = 0xC,
	CHANNEL_PRESSURE = 0xD,
	PITCH_BEND = 0xE,
	SYSTEM_EXCLUSIVE = 0xF0,
	QUARTER_FRAME = 0xF1,
	SONG_POSITION_POINTER = 0xF2,
	SONG_SELECT = 0xF3,
	TUNE_REQUEST = 0xF6,
	TIMING_CLOCK = 0xF8,
	START = 0xFA,
	CONTINUE = 0xFB,
	STOP = 0xFC,
	ACTIVE_SENSING = 0xFE,
	SYSTEM_RESET = 0xFF,
};

enum class MouseButton
{
	NONE = 0,
	LEFT = 1,
	RIGHT = 2,
	MIDDLE = 3,
	WHEEL_UP = 4,
	WHEEL_DOWN = 5,
	WHEEL_LEFT = 6,
	WHEEL_RIGHT = 7,
	MB_XBUTTON1 = 8, // "XBUTTON1" is a reserved word on Windows.
	MB_XBUTTON2 = 9, // "XBUTTON2" is a reserved word on Windows.
};

enum class MouseButtonMask
{
	NONE = 0,
	LEFT = (1 << (int(MouseButton::LEFT) - 1)),
	RIGHT = (1 << (int(MouseButton::RIGHT) - 1)),
	MIDDLE = (1 << (int(MouseButton::MIDDLE) - 1)),
	MB_XBUTTON1 = (1 << (int(MouseButton::MB_XBUTTON1) - 1)),
	MB_XBUTTON2 = (1 << (int(MouseButton::MB_XBUTTON2) - 1)),
};

inline MouseButtonMask mouse_button_to_mask(MouseButton p_button)
{
	ERR_FAIL_COND_V(p_button == MouseButton::NONE, MouseButtonMask::NONE);

	return MouseButtonMask(1 << ((int)p_button - 1));
}

constexpr MouseButtonMask operator|(MouseButtonMask p_a, MouseButtonMask p_b)
{
	return static_cast<MouseButtonMask>(static_cast<int>(p_a) | static_cast<int>(p_b));
}

constexpr MouseButtonMask& operator|=(MouseButtonMask& p_a, MouseButtonMask p_b)
{
	return p_a = p_a | p_b;
}
#include "core/math/vector2.h"
#include "core/os/keyboard.h"
#include "core/string/ustring.h"
// --- Folded from core/templates/bit_field.h ---
#include "core/typedefs.h"

#include <type_traits>

// TODO: Replace `typename` with enum concept once C++20 concepts/constraints are allowed.

template <typename T>
class BitField {
	static_assert(std::is_enum_v<T>);
	uint64_t value;

public:
	_ALWAYS_INLINE_ constexpr void set_flag(BitField p_flag) { value |= p_flag.value; }
	_ALWAYS_INLINE_ constexpr bool has_flag(BitField p_flag) const { return value & p_flag.value; }
	_ALWAYS_INLINE_ constexpr bool is_empty() const { return value == 0; }
	_ALWAYS_INLINE_ constexpr void clear_flag(BitField p_flag) { value &= ~p_flag.value; }
	_ALWAYS_INLINE_ constexpr void clear() { value = 0; }

	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_combined(BitField p_other) const { return BitField(value | p_other.value); }
	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_shared(BitField p_other) const { return BitField(value & p_other.value); }
	[[nodiscard]] _ALWAYS_INLINE_ constexpr BitField get_different(BitField p_other) const { return BitField(value ^ p_other.value); }

	_ALWAYS_INLINE_ constexpr BitField() = default;
	_ALWAYS_INLINE_ constexpr BitField(T p_value) :
			value(static_cast<uint64_t>(p_value)) {}
	_ALWAYS_INLINE_ constexpr operator T() const { return static_cast<T>(value); }

	// TODO: Unify as single constructor once C++20 `explicit` conditionals are allowed.

	template <typename V, std::enable_if_t<std::is_arithmetic_v<V> && std::is_convertible_v<T, int>, int> = 0>
	_ALWAYS_INLINE_ constexpr BitField(V p_value) :
			value(static_cast<uint64_t>(p_value)) {}
	template <typename V, std::enable_if_t<std::is_arithmetic_v<V> && !std::is_convertible_v<T, int>, int> = 0>
	_ALWAYS_INLINE_ constexpr explicit BitField(V p_value) :
			value(static_cast<uint64_t>(p_value)) {}
	template <typename V, std::enable_if_t<std::is_arithmetic_v<V>, int> = 0>
	_ALWAYS_INLINE_ constexpr explicit operator V() const { return static_cast<V>(value); }
};

// Implicitly zero-constructible as a trivially-constructible type.
static_assert(is_zero_constructible_v<BitField<Error>>);

// --- End core/templates/bit_field.h ---
#include "core/typedefs.h"

class Shortcut;
struct Transform2D;

/**
 * Plain value-type input event with composed per-event payload structs.
 * Replaces the former 6-level, 16-class RefCounted Resource hierarchy.
 */
struct InputEvent
{
	static constexpr int DEVICE_ID_EMULATION = -1;
	static constexpr int DEVICE_ID_INTERNAL = -2;
	static constexpr int DEVICE_ID_KEYBOARD = 16; // IDs 0-15 are reserved for joypads.
	static constexpr int DEVICE_ID_MOUSE = 32;    // IDs 17-31 are reserved for multiple keyboards.

	struct KeyData
	{
		Key keycode = Key::NONE;
		Key physical_keycode = Key::NONE;
		Key key_label = Key::NONE;
		char32_t unicode = 0;
		KeyLocation location = KeyLocation::UNSPECIFIED;
		bool echo = false;
	};

	struct MouseButtonData
	{
		Vector2 position;
		Vector2 global_position;
		uint32_t button_mask = 0;
		float factor = 1.0f;
		MouseButton button_index = MouseButton::NONE;
		bool double_click = false;
	};

	struct MouseMotionData
	{
		Vector2 position;
		Vector2 global_position;
		uint32_t button_mask = 0;
		Vector2 tilt;
		float pressure = 0.0f;
		Vector2 relative;
		Vector2 screen_relative;
		Vector2 velocity;
		Vector2 screen_velocity;
		bool pen_inverted = false;
	};

	struct JoypadButtonData
	{
		JoyButton button_index = (JoyButton)0;
		float pressure = 0.0f;
	};

	struct JoypadMotionData
	{
		JoyAxis axis = (JoyAxis)0;
		float axis_value = 0.0f;
	};

	struct TouchData
	{
		int index = 0;
		Vector2 position;
		bool double_tap = false;
	};

	struct ScreenDragData
	{
		int index = 0;
		Vector2 position;
		Vector2 relative;
		Vector2 screen_relative;
		Vector2 velocity;
		Vector2 screen_velocity;
		Vector2 tilt;
		float pressure = 0.0f;
		bool pen_inverted = false;
	};

	struct GestureData
	{
		Vector2 position;
		Vector2 delta;
		real_t factor = 1.0;
	};

	struct MIDIData
	{
		int channel = 0;
		MIDIMessage message = MIDIMessage::NONE;
		int pitch = 0;
		int velocity = 0;
		int instrument = 0;
		int pressure = 0;
		int controller_number = 0;
		int controller_value = 0;
	};

	struct ActionData
	{
		String action;
		float strength = 1.0f;
		int event_index = -1;
	};

	struct ShortcutData
	{
		const Shortcut* shortcut = nullptr;
	};

	// Common header fields
	InputEventType type = InputEventType::INVALID;
	int device = 0;
	int64_t window_id = 0;
	BitField<KeyModifierMask> modifiers = KeyModifierMask(0);
	bool pressed = false;
	bool canceled = false;

	// Composed POD union payload + action metadata
	union
	{
		KeyData key;
		MouseButtonData mouse_button;
		MouseMotionData mouse_motion = {};
		JoypadButtonData joy_button;
		JoypadMotionData joy_motion;
		TouchData touch;
		ScreenDragData screen_drag;
		GestureData gesture;
		MIDIData midi;
		ShortcutData shortcut;
	};
	ActionData action;

	InputEvent() = default;

	// Factory constructors
	static InputEvent make_key(Key p_keycode_with_modifier_masks = Key::NONE, bool p_physical = false);
	static InputEvent make_mouse_button();
	static InputEvent make_mouse_motion();
	static InputEvent make_joypad_button(JoyButton p_btn_index = (JoyButton)0, int p_device = -1);
	static InputEvent make_joypad_motion(
		JoyAxis p_axis = (JoyAxis)0, float p_value = 0.0f, int p_device = -1);
	static InputEvent make_screen_touch();
	static InputEvent make_screen_drag();
	static InputEvent make_magnify_gesture();
	static InputEvent make_pan_gesture();
	static InputEvent make_midi();
	static InputEvent make_action(
		const String& p_action = String(), bool p_pressed = false, float p_strength = 1.0f);
	static InputEvent make_shortcut(const Shortcut* p_shortcut = nullptr);

	// Type & state queries
	bool is_valid() const { return type != InputEventType::INVALID; }
	bool is_null() const { return type == InputEventType::INVALID; }
	InputEventType get_type() const { return type; }

	bool is_mouse() const
	{
		return type == InputEventType::MOUSE_BUTTON || type == InputEventType::MOUSE_MOTION;
	}
	bool is_gesture() const
	{
		return type == InputEventType::MAGNIFY_GESTURE || type == InputEventType::PAN_GESTURE;
	}
	bool is_from_window() const
	{
		return type == InputEventType::KEY || type == InputEventType::MOUSE_BUTTON ||
			   type == InputEventType::MOUSE_MOTION || type == InputEventType::SCREEN_TOUCH ||
			   type == InputEventType::SCREEN_DRAG || type == InputEventType::MAGNIFY_GESTURE ||
			   type == InputEventType::PAN_GESTURE;
	}

	void set_device(int p_device) { device = p_device; }
	int get_device() const { return device; }

	void set_window_id(int64_t p_id) { window_id = p_id; }
	int64_t get_window_id() const { return window_id; }

	void set_pressed(bool p_pressed) { pressed = p_pressed; }
	void set_canceled(bool p_canceled) { canceled = p_canceled; }
	bool is_canceled() const { return canceled; }
	bool is_pressed() const { return pressed && !canceled; }
	bool is_released() const { return !pressed && !canceled; }
	bool is_echo() const { return type == InputEventType::KEY && key.echo; }

	// Shared mouse helpers (MOUSE_BUTTON & MOUSE_MOTION)
	Vector2 get_mouse_position() const;
	void set_mouse_position(const Vector2& p_pos);
	Vector2 get_mouse_global_position() const;
	void set_mouse_global_position(const Vector2& p_global_pos);
	uint32_t get_mouse_button_mask() const;
	void set_mouse_button_mask(uint32_t p_mask);

	// Joypad motion helper (updates `pressed` threshold as well)
	void set_joy_axis(JoyAxis p_axis);
	void set_joy_axis_value(float p_value);

	// Modifier helpers
	uint32_t get_modifiers_mask() const
	{
		return static_cast<uint32_t>(modifiers) &
			   (static_cast<uint32_t>(KeyModifierMask::SHIFT) |
				   static_cast<uint32_t>(KeyModifierMask::ALT) |
				   static_cast<uint32_t>(KeyModifierMask::CTRL) |
				   static_cast<uint32_t>(KeyModifierMask::META));
	}

	void set_command_or_control_autoremap(bool p_enabled);
	bool is_command_or_control_autoremap() const
	{
		return modifiers.has_flag(KeyModifierMask::CMD_OR_CTRL);
	}
	bool is_command_or_control_pressed() const;

	void set_shift_pressed(bool p_pressed);
	bool is_shift_pressed() const { return modifiers.has_flag(KeyModifierMask::SHIFT); }

	void set_alt_pressed(bool p_pressed);
	bool is_alt_pressed() const { return modifiers.has_flag(KeyModifierMask::ALT); }

	void set_ctrl_pressed(bool p_pressed);
	bool is_ctrl_pressed() const { return modifiers.has_flag(KeyModifierMask::CTRL); }

	void set_meta_pressed(bool p_pressed);
	bool is_meta_pressed() const { return modifiers.has_flag(KeyModifierMask::META); }

	void copy_modifiers_from(const InputEvent& p_event);
	String modifiers_as_text() const;

	// Key helpers (InputEventType::KEY)
	Key get_keycode_with_modifiers() const;
	Key get_physical_keycode_with_modifiers() const;
	Key get_key_label_with_modifiers() const;
	String as_text_physical_keycode() const;
	String as_text_keycode() const;
	String as_text_key_label() const;
	String as_text_location() const;

	// Action queries & matching
	bool is_action(const String& p_action, bool p_exact_match = false) const;
	bool is_action_pressed(
		const String& p_action, bool p_allow_echo = false, bool p_exact_match = false) const;
	bool is_action_released(const String& p_action, bool p_exact_match = false) const;
	bool is_action_just_pressed_or_echo(
		const String& p_action, bool p_exact_match = false) const;
	float get_action_strength(const String& p_action, bool p_exact_match = false) const;
	float get_action_raw_strength(const String& p_action, bool p_exact_match = false) const;

	bool is_action_type() const;
	bool action_match(const InputEvent& p_event, bool p_exact_match, float p_deadzone,
		bool* r_pressed, float* r_strength, float* r_raw_strength) const;
	bool is_match(const InputEvent& p_event, bool p_exact_match = true) const;

	// Transform, accumulation, and formatting
	InputEvent xformed_by(
		const Transform2D& p_xform, const Vector2& p_local_ofs = Vector2()) const;
	bool accumulate(const InputEvent& p_event);

	String as_text() const;
	String to_string() const;
};

// --- Folded from core/input/shortcut.h ---
#include "core/io/resource.h"

class Shortcut : public Resource
{
	Vector<InputEvent> events;

public:
	void set_events(const Vector<InputEvent>& p_events);
	const Vector<InputEvent>& get_events() const;

	void set_events_list(const List<InputEvent>* p_events);

	bool matches_event(const InputEvent& p_event) const;
	bool has_valid_event() const;

	String get_as_text() const;

	static Ref<Shortcut> make_from_action(const String& p_action);

	static bool is_event_array_equal(
		const Vector<InputEvent>& p_event_array1, const Vector<InputEvent>& p_event_array2);
};
