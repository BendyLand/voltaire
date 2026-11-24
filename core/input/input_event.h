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
#include "core/typedefs.h"

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
// --- Folded from core/os/keyboard.h ---
class String;

#include <cstdint>
#include "core/types.h"

// Keep the values in this enum in sync with `_keycodes` in `keyboard.cpp`,
// and the bindings in `core_constants.cpp`.
enum class Key
{
	NONE = 0,
	// Special key: The strategy here is similar to the one used by toolkits,
	// which consists in leaving the 21 bits unicode range for printable
	// characters, and use the upper 11 bits for special keys and modifiers.
	// This way everything (char/keycode) can fit nicely in one 32-bit
	// integer (the enum's underlying type is `int` by default).
	SPECIAL = (1 << 22),
	/* CURSOR/FUNCTION/BROWSER/MULTIMEDIA/MISC KEYS */
	ESCAPE = SPECIAL | 0x01,
	TAB = SPECIAL | 0x02,
	BACKTAB = SPECIAL | 0x03,
	BACKSPACE = SPECIAL | 0x04,
	ENTER = SPECIAL | 0x05,
	KP_ENTER = SPECIAL | 0x06,
	INSERT = SPECIAL | 0x07,
	KEY_DELETE = SPECIAL | 0x08, // "DELETE" is a reserved word on Windows.
	PAUSE = SPECIAL | 0x09,
	PRINT = SPECIAL | 0x0A,
	SYSREQ = SPECIAL | 0x0B,
	CLEAR = SPECIAL | 0x0C,
	HOME = SPECIAL | 0x0D,
	END = SPECIAL | 0x0E,
	LEFT = SPECIAL | 0x0F,
	UP = SPECIAL | 0x10,
	RIGHT = SPECIAL | 0x11,
	DOWN = SPECIAL | 0x12,
	PAGEUP = SPECIAL | 0x13,
	PAGEDOWN = SPECIAL | 0x14,
	SHIFT = SPECIAL | 0x15,
	CTRL = SPECIAL | 0x16,
	META = SPECIAL | 0x17,
#if defined(MACOS_ENABLED)
	CMD_OR_CTRL = META,
#else
	CMD_OR_CTRL = CTRL,
#endif
	ALT = SPECIAL | 0x18,
	CAPSLOCK = SPECIAL | 0x19,
	NUMLOCK = SPECIAL | 0x1A,
	SCROLLLOCK = SPECIAL | 0x1B,
	F1 = SPECIAL | 0x1C,
	F2 = SPECIAL | 0x1D,
	F3 = SPECIAL | 0x1E,
	F4 = SPECIAL | 0x1F,
	F5 = SPECIAL | 0x20,
	F6 = SPECIAL | 0x21,
	F7 = SPECIAL | 0x22,
	F8 = SPECIAL | 0x23,
	F9 = SPECIAL | 0x24,
	F10 = SPECIAL | 0x25,
	F11 = SPECIAL | 0x26,
	F12 = SPECIAL | 0x27,
	F13 = SPECIAL | 0x28,
	F14 = SPECIAL | 0x29,
	F15 = SPECIAL | 0x2A,
	F16 = SPECIAL | 0x2B,
	F17 = SPECIAL | 0x2C,
	F18 = SPECIAL | 0x2D,
	F19 = SPECIAL | 0x2E,
	F20 = SPECIAL | 0x2F,
	F21 = SPECIAL | 0x30,
	F22 = SPECIAL | 0x31,
	F23 = SPECIAL | 0x32,
	F24 = SPECIAL | 0x33,
	F25 = SPECIAL | 0x34,
	F26 = SPECIAL | 0x35,
	F27 = SPECIAL | 0x36,
	F28 = SPECIAL | 0x37,
	F29 = SPECIAL | 0x38,
	F30 = SPECIAL | 0x39,
	F31 = SPECIAL | 0x3A,
	F32 = SPECIAL | 0x3B,
	F33 = SPECIAL | 0x3C,
	F34 = SPECIAL | 0x3D,
	F35 = SPECIAL | 0x3E,
	KP_MULTIPLY = SPECIAL | 0x81,
	KP_DIVIDE = SPECIAL | 0x82,
	KP_SUBTRACT = SPECIAL | 0x83,
	KP_PERIOD = SPECIAL | 0x84,
	KP_ADD = SPECIAL | 0x85,
	KP_0 = SPECIAL | 0x86,
	KP_1 = SPECIAL | 0x87,
	KP_2 = SPECIAL | 0x88,
	KP_3 = SPECIAL | 0x89,
	KP_4 = SPECIAL | 0x8A,
	KP_5 = SPECIAL | 0x8B,
	KP_6 = SPECIAL | 0x8C,
	KP_7 = SPECIAL | 0x8D,
	KP_8 = SPECIAL | 0x8E,
	KP_9 = SPECIAL | 0x8F,
	MENU = SPECIAL | 0x42,
	HYPER = SPECIAL | 0x43,
	HELP = SPECIAL | 0x45,
	BACK = SPECIAL | 0x48,
	FORWARD = SPECIAL | 0x49,
	STOP = SPECIAL | 0x4A,
	REFRESH = SPECIAL | 0x4B,
	VOLUMEDOWN = SPECIAL | 0x4C,
	VOLUMEMUTE = SPECIAL | 0x4D,
	VOLUMEUP = SPECIAL | 0x4E,
	MEDIAPLAY = SPECIAL | 0x54,
	MEDIASTOP = SPECIAL | 0x55,
	MEDIAPREVIOUS = SPECIAL | 0x56,
	MEDIANEXT = SPECIAL | 0x57,
	MEDIARECORD = SPECIAL | 0x58,
	HOMEPAGE = SPECIAL | 0x59,
	FAVORITES = SPECIAL | 0x5A,
	SEARCH = SPECIAL | 0x5B,
	STANDBY = SPECIAL | 0x5C,
	OPENURL = SPECIAL | 0x5D,
	LAUNCHMAIL = SPECIAL | 0x5E,
	LAUNCHMEDIA = SPECIAL | 0x5F,
	LAUNCH0 = SPECIAL | 0x60,
	LAUNCH1 = SPECIAL | 0x61,
	LAUNCH2 = SPECIAL | 0x62,
	LAUNCH3 = SPECIAL | 0x63,
	LAUNCH4 = SPECIAL | 0x64,
	LAUNCH5 = SPECIAL | 0x65,
	LAUNCH6 = SPECIAL | 0x66,
	LAUNCH7 = SPECIAL | 0x67,
	LAUNCH8 = SPECIAL | 0x68,
	LAUNCH9 = SPECIAL | 0x69,
	LAUNCHA = SPECIAL | 0x6A,
	LAUNCHB = SPECIAL | 0x6B,
	LAUNCHC = SPECIAL | 0x6C,
	LAUNCHD = SPECIAL | 0x6D,
	LAUNCHE = SPECIAL | 0x6E,
	LAUNCHF = SPECIAL | 0x6F,

	GLOBE = SPECIAL | 0x70,
	KEYBOARD = SPECIAL | 0x71,
	JIS_EISU = SPECIAL | 0x72,
	JIS_KANA = SPECIAL | 0x73,

	UNKNOWN = SPECIAL | 0x7FFFFF,

	/* PRINTABLE LATIN 1 CODES */

	SPACE = 0x0020,
	EXCLAM = 0x0021,
	QUOTEDBL = 0x0022,
	NUMBERSIGN = 0x0023,
	DOLLAR = 0x0024,
	PERCENT = 0x0025,
	AMPERSAND = 0x0026,
	APOSTROPHE = 0x0027,
	PARENLEFT = 0x0028,
	PARENRIGHT = 0x0029,
	ASTERISK = 0x002A,
	PLUS = 0x002B,
	COMMA = 0x002C,
	MINUS = 0x002D,
	PERIOD = 0x002E,
	SLASH = 0x002F,
	KEY_0 = 0x0030,
	KEY_1 = 0x0031,
	KEY_2 = 0x0032,
	KEY_3 = 0x0033,
	KEY_4 = 0x0034,
	KEY_5 = 0x0035,
	KEY_6 = 0x0036,
	KEY_7 = 0x0037,
	KEY_8 = 0x0038,
	KEY_9 = 0x0039,
	COLON = 0x003A,
	SEMICOLON = 0x003B,
	LESS = 0x003C,
	EQUAL = 0x003D,
	GREATER = 0x003E,
	QUESTION = 0x003F,
	AT = 0x0040,
	A = 0x0041,
	B = 0x0042,
	C = 0x0043,
	D = 0x0044,
	E = 0x0045,
	F = 0x0046,
	G = 0x0047,
	H = 0x0048,
	I = 0x0049,
	J = 0x004A,
	K = 0x004B,
	L = 0x004C,
	M = 0x004D,
	N = 0x004E,
	O = 0x004F,
	P = 0x0050,
	Q = 0x0051,
	R = 0x0052,
	S = 0x0053,
	T = 0x0054,
	U = 0x0055,
	V = 0x0056,
	W = 0x0057,
	X = 0x0058,
	Y = 0x0059,
	Z = 0x005A,
	BRACKETLEFT = 0x005B,
	BACKSLASH = 0x005C,
	BRACKETRIGHT = 0x005D,
	ASCIICIRCUM = 0x005E,
	UNDERSCORE = 0x005F,
	QUOTELEFT = 0x0060,
	BRACELEFT = 0x007B,
	BAR = 0x007C,
	BRACERIGHT = 0x007D,
	ASCIITILDE = 0x007E,
	YEN = 0x00A5,
	SECTION = 0x00A7,
};

enum class KeyModifierMask
{
	CODE_MASK = ((1 << 23) - 1),  ///< Apply this mask to any keycode to remove modifiers.
	MODIFIER_MASK = (0x7F << 24), ///< Apply this mask to isolate modifiers.
	// RESERVED = (1 << 23),
	CMD_OR_CTRL = (1 << 24),
	SHIFT = (1 << 25),
	ALT = (1 << 26),
	META = (1 << 27),
	CTRL = (1 << 28),
	KPAD = (1 << 29),
	GROUP_SWITCH = (1 << 30)
};

enum class KeyLocation
{
	UNSPECIFIED,
	LEFT,
	RIGHT
};

// To avoid having unnecessary operators, only define the ones that are needed.

constexpr Key operator-(uint32_t p_left, Key p_right) { return (Key)(p_left - (uint32_t)p_right); }

constexpr Key& operator-=(Key& p_left, int p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) - static_cast<int>(p_right));
	return p_left;
}

constexpr Key operator+(Key p_left, int p_right) { return (Key)((int)p_left + (int)p_right); }

constexpr Key operator+(Key p_left, Key p_right)
 { return (Key)((int)p_left + (int)p_right); }

constexpr Key operator-(Key p_left, Key p_right) { return (Key)((int)p_left - (int)p_right); }

constexpr Key operator&(Key p_left, Key p_right) { return (Key)((int)p_left & (int)p_right); }

constexpr Key operator|(Key p_left, Key p_right) { return (Key)((int)p_left | (int)p_right); }

constexpr Key& operator|=(Key& p_left, Key p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) | static_cast<int>(p_right));
	return p_left;
}

constexpr Key& operator|=(Key& p_left, KeyModifierMask p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) | static_cast<int>(p_right));
	return p_left;
}

constexpr Key& operator&=(Key& p_left, KeyModifierMask p_right)
{
	p_left = static_cast<Key>(static_cast<int>(p_left) & static_cast<int>(p_right));
	return p_left;
}

constexpr Key operator|(Key p_left, KeyModifierMask p_right)
{
	return (Key)((int)p_left | (int)p_right);
}

constexpr Key operator&(Key p_left, KeyModifierMask p_right)
{
	return (Key)((int)p_left & (int)p_right);
}

constexpr Key operator+(KeyModifierMask p_left, Key p_right)
{
	return (Key)((int)p_left + (int)p_right);
}

constexpr Key operator|(KeyModifierMask p_left, Key p_right)
{
	return (Key)((int)p_left | (int)p_right);
}

constexpr KeyModifierMask operator+(KeyModifierMask p_left, KeyModifierMask p_right)
{
	return (KeyModifierMask)((int)p_left + (int)p_right);
}

constexpr KeyModifierMask operator|(KeyModifierMask p_left, KeyModifierMask p_right)
{
	return (KeyModifierMask)((int)p_left | (int)p_right);
}

constexpr Key operator|(Key p_left, uint32_t p_right)
{
	return static_cast<Key>(static_cast<uint32_t>(p_left) | p_right);
}

constexpr Key operator|(uint32_t p_left, Key p_right)
{
	return static_cast<Key>(p_left | static_cast<uint32_t>(p_right));
}

String keycode_get_string(Key p_code);
bool keycode_has_unicode(Key p_keycode);
Key find_keycode(const String& p_codestr);
const char* find_keycode_name(Key p_keycode);
int keycode_get_count();
int keycode_get_value_by_index(int p_index);
const char* keycode_get_name_by_index(int p_index);

char32_t fix_unicode(char32_t p_char);
Key fix_keycode(char32_t p_char, Key p_key);
Key fix_key_label(char32_t p_char, Key p_key);
// --- End core/os/keyboard.h ---
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
