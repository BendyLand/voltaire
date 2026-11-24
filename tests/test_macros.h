/**************************************************************************/
/*  test_macros.h                                                         */
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

#include "core/types.h"
#include "core/types.h" // IWYU pragma: keep. Used in macro.

#if defined(_MSC_VER) && !defined(DOCTEST_THREAD_LOCAL)
// NOTE: We must disable the THREAD_LOCAL entirely in doctest to prevent crashes on debugging.
//  Since we link with /MT, thread_local is always expired when the header is used, so the
//  debugger crashes the engine and it causes weird errors.
// See: https://github.com/onqtam/doctest/issues/401
#define DOCTEST_THREAD_LOCAL
#endif

#if !__cpp_exceptions && !__EXCEPTIONS && !defined(DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#endif

// See documentation for doctest at:
// https://github.com/onqtam/doctest/blob/master/doc/markdown/readme.md#reference
#include <thirdparty/doctest/doctest.h>

// Forces a test file to be linked.
#define TEST_FORCE_LINK(m_name) \
	namespace ForceLink { \
	void force_link_##m_name() {} \
	}

// The test is skipped with this, run pending tests with `--test --no-skip`.
#define TEST_CASE_PENDING(name) TEST_CASE(name *doctest::skip())

// The test case is marked as failed, but does not fail the entire test run.
#define TEST_CASE_MAY_FAIL(name) TEST_CASE(name *doctest::may_fail())

// Provide aliases to conform with Godot naming conventions (see error macros).
#define TEST_COND(cond, ...) DOCTEST_CHECK_FALSE_MESSAGE(cond, __VA_ARGS__)
#define TEST_FAIL(cond, ...) DOCTEST_FAIL(cond, __VA_ARGS__)
#define TEST_FAIL_COND(cond, ...) DOCTEST_REQUIRE_FALSE_MESSAGE(cond, __VA_ARGS__)
#define TEST_FAIL_COND_WARN(cond, ...) DOCTEST_WARN_FALSE_MESSAGE(cond, __VA_ARGS__)

// Temporarily disable error prints to test failure paths.
// This allows to avoid polluting the test summary with error messages.
// The `print_error_enabled` boolean is defined in `core/core_globals.cpp` and
// works at global scope. It's used by various loggers in `should_log()` method,
// which are used by error macros which call into `OS::print_error`, effectively
// disabling any error messages to be printed from the engine side (not tests).
#define ERR_PRINT_OFF CoreGlobals::print_error_enabled = false;
#define ERR_PRINT_ON CoreGlobals::print_error_enabled = true;

// Stringify all `Variant` compatible types for doctest output by default.
// https://github.com/onqtam/doctest/blob/master/doc/markdown/stringification.md

#define DOCTEST_STRINGIFY_VARIANT(m_type) \
	template <> \
	struct doctest::StringMaker<m_type> { \
		static doctest::String convert(const m_type &p_val) { \
			const Variant val = p_val; \
			return val.operator ::String().utf8().get_data(); \
		} \
	};

#define DOCTEST_STRINGIFY_VARIANT_POINTER(m_type) \
	template <> \
	struct doctest::StringMaker<m_type> { \
		static doctest::String convert(const m_type *p_val) { \
			const Variant val = p_val; \
			return val.operator ::String().utf8().get_data(); \
		} \
	};

// Register test commands to be launched from the command-line.
// For instance: REGISTER_TEST_COMMAND("gdscript-parser" &test_parser_func).
// Example usage: `godot --test gdscript-parser`.

typedef void (*TestFunc)();
extern HashMap<String, TestFunc> *test_commands;
int register_test_command(String p_command, TestFunc p_function);

#define REGISTER_TEST_COMMAND(m_command, m_function) \
	DOCTEST_GLOBAL_NO_WARNINGS(DOCTEST_ANONYMOUS(GODOT_DOCTEST_ANON_VAR_), \
			register_test_command(m_command, m_function))

#define MULTICHECK_STRING_EQ(m_obj, m_func, m_param1, m_eq) \
	CHECK(m_obj.m_func(m_param1) == m_eq); \
	CHECK(m_obj.m_func(U##m_param1) == m_eq); \
	CHECK(m_obj.m_func(L##m_param1) == m_eq); \
	CHECK(m_obj.m_func(String(m_param1)) == m_eq);

#define MULTICHECK_STRING_INT_EQ(m_obj, m_func, m_param1, m_param2, m_eq) \
	CHECK(m_obj.m_func(m_param1, m_param2) == m_eq); \
	CHECK(m_obj.m_func(U##m_param1, m_param2) == m_eq); \
	CHECK(m_obj.m_func(L##m_param1, m_param2) == m_eq); \
	CHECK(m_obj.m_func(String(m_param1), m_param2) == m_eq);

#define MULTICHECK_STRING_INT_INT_EQ(m_obj, m_func, m_param1, m_param2, m_param3, m_eq) \
	CHECK(m_obj.m_func(m_param1, m_param2, m_param3) == m_eq); \
	CHECK(m_obj.m_func(U##m_param1, m_param2, m_param3) == m_eq); \
	CHECK(m_obj.m_func(L##m_param1, m_param2, m_param3) == m_eq); \
	CHECK(m_obj.m_func(String(m_param1), m_param2, m_param3) == m_eq);

#define MULTICHECK_STRING_STRING_EQ(m_obj, m_func, m_param1, m_param2, m_eq) \
	CHECK(m_obj.m_func(m_param1, m_param2) == m_eq); \
	CHECK(m_obj.m_func(U##m_param1, U##m_param2) == m_eq); \
	CHECK(m_obj.m_func(L##m_param1, L##m_param2) == m_eq); \
	CHECK(m_obj.m_func(String(m_param1), String(m_param2)) == m_eq);

#define MULTICHECK_GET_SLICE(m_obj, m_param1, m_slices) \
	for (int i = 0; i < m_obj.get_slice_count(m_param1); ++i) { \
		CHECK(m_obj.get_slice(m_param1, i) == m_slices[i]); \
	} \
	for (int i = 0; i < m_obj.get_slice_count(U##m_param1); ++i) { \
		CHECK(m_obj.get_slice(U##m_param1, i) == m_slices[i]); \
	} \
	for (int i = 0; i < m_obj.get_slice_count(L##m_param1); ++i) { \
		CHECK(m_obj.get_slice(L##m_param1, i) == m_slices[i]); \
	} \
	for (int i = 0; i < m_obj.get_slice_count(String(m_param1)); ++i) { \
		CHECK(m_obj.get_slice(String(m_param1), i) == m_slices[i]); \
	}

#define MULTICHECK_SPLIT(m_obj, m_func, m_param1, m_param2, m_param3, m_slices, m_expected_size) \
	do { \
		Vector<String> string_list; \
\
		string_list = m_obj.m_func(m_param1, m_param2, m_param3); \
		CHECK(m_expected_size == string_list.size()); \
		for (int i = 0; i < string_list.size(); ++i) { \
			CHECK(string_list[i] == m_slices[i]); \
		} \
\
		string_list = m_obj.m_func(U##m_param1, m_param2, m_param3); \
		CHECK(m_expected_size == string_list.size()); \
		for (int i = 0; i < string_list.size(); ++i) { \
			CHECK(string_list[i] == m_slices[i]); \
		} \
\
		string_list = m_obj.m_func(L##m_param1, m_param2, m_param3); \
		CHECK(m_expected_size == string_list.size()); \
		for (int i = 0; i < string_list.size(); ++i) { \
			CHECK(string_list[i] == m_slices[i]); \
		} \
\
		string_list = m_obj.m_func(String(m_param1), m_param2, m_param3); \
		CHECK(m_expected_size == string_list.size()); \
		for (int i = 0; i < string_list.size(); ++i) { \
			CHECK(string_list[i] == m_slices[i]); \
		} \
	} while (false)

// --- Folded from tests/test_utils.h ---
class String;

namespace TestUtils {

String get_data_path(const String &p_file);
String get_executable_dir();
String get_temp_path(const String &p_suffix);
} // namespace TestUtils

// FIXME: This was originally constrained to `tests/core/config/test_project_settings.h`, but that
//  file is no longer a header. Some other tests relied on the ability to override the resource
//  path, so relocating the accessor is the least-intrusive workaround.
class TestProjectSettingsInternalsAccessor {
public:
	static String &resource_path();
};

// --- Folded from tests/test_tools.h ---
#include "core/typedefs.h"

struct ErrorDetector {
	ErrorDetector() {
		eh.errfunc = _detect_error;
		eh.userdata = this;

		add_error_handler(&eh);
	}

	~ErrorDetector() {
		remove_error_handler(&eh);
	}

	void clear() {
		has_error = false;
	}

	static void _detect_error(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type) {
		ErrorDetector *self = (ErrorDetector *)p_self;
		self->has_error = true;
	}

	ErrorHandlerList eh;
	bool has_error = false;
};
// --- End tests/test_tools.h ---
// --- End tests/test_utils.h ---

// --- Folded from tests/signal_watcher.h ---


// Utility class / macros for testing signals
//
// Use SIGNAL_WATCH(*object, "signal_name") to start watching
// Makes sure to call SIGNAL_UNWATCH(*object, "signal_name") to stop watching in cleanup, this is
// not done automatically.
//
// The SignalWatcher will capture all signals and their args sent between checks.
//
// Use SIGNAL_CHECK("signal_name"), Vector<Vector<Variant>>), to check the arguments of all fired
// signals. The outer vector is each fired signal, the inner vector the list of arguments for that
// signal. Order does matter.
//
// Use SIGNAL_CHECK_FALSE("signal_name") to check if a signal was not fired.
//
// Use SIGNAL_DISCARD("signal_name") to discard records all of the given signal, use only in placed
// you don't need to check.
//
// All signals are automatically discarded between test/sub test cases.

class SignalWatcher final
{
	struct Data
	{
		HashMap<String, Array> _signals;
		mem_unique_ptr<Object> obj;
	};

	static inline Data* data = nullptr;

	static void _add_signal_entry(const Array& p_args, const String& p_name);
	static void _signal_callback_zero(const String& p_name);
	static void _signal_callback_one(Variant p_arg1, const String& p_name);
	static void _signal_callback_two(Variant p_arg1, Variant p_arg2, const String& p_name);
	static void _signal_callback_three(
		Variant p_arg1, Variant p_arg2, Variant p_arg3, const String& p_name);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void watch_signal(Object* p_object, const String& p_signal);
	static void unwatch_signal(Object* p_object, const String& p_signal);
	static bool check(const String& p_name, const Array& p_args);
	static bool check_false(const String& p_name);
	static void discard_signal(const String& p_name);

	static void _clear_signals();

	SignalWatcher() = delete;
	SignalWatcher(const SignalWatcher&) = delete;
	SignalWatcher& operator=(const SignalWatcher&) = delete;
	~SignalWatcher() = delete;
};

#define SIGNAL_WATCH(m_object, m_signal)                                                           \
	SignalWatcher::watch_signal(m_object, m_signal);
#define SIGNAL_UNWATCH(m_object, m_signal)                                                         \
	SignalWatcher::unwatch_signal(m_object, m_signal);

#define SIGNAL_CHECK(m_signal, m_args)                                                             \
	CHECK(SignalWatcher::check(m_signal, m_args));
#define SIGNAL_CHECK_FALSE(m_signal) CHECK(SignalWatcher::check_false(m_signal));
#define SIGNAL_DISCARD(m_signal) SignalWatcher::discard_signal(m_signal);
// --- End tests/signal_watcher.h ---

// --- Folded from tests/display_server_mock.h ---
#include "servers/display/display_server.h"

// Specialized DisplayServer for unittests based on DisplayServerHeadless, that
// additionally supports things like mouse enter/exit events and clipboard.
class DisplayServerMock : public DisplayServerHeadless {

private:
	friend class DisplayServer;

	Point2i mouse_position = Point2i(-1, -1); // Outside of Window.
	DisplayServerEnums::CursorShape cursor_shape = DisplayServerEnums::CursorShape::CURSOR_ARROW;
	bool window_over = false;
	Callable event_callback;

	String clipboard_text;
	String primary_clipboard_text;

	static Vector<String> get_rendering_drivers_func();
	static DisplayServerDriver *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);

	void _set_mouse_position(const Point2i &p_position);
	void _set_window_over(bool p_over);
	void _send_window_event(DisplayServerEnums::WindowEvent p_event);

public:
	bool has_feature(DisplayServerEnums::Feature p_feature) const override;

	String get_name() const override { return "mock"; }

	// You can simulate DisplayServer-events by calling this function.
	// The events will be delivered to Godot's Input-system.
	// Mouse-events (Button & Motion) will additionally update the DisplayServer's mouse position.
	// For Mouse motion events, the `relative`-property is set based on the distance to the previous mouse position.
	void simulate_event(Ref<InputEvent> p_event);

	// Returns the current cursor shape.
	DisplayServerEnums::CursorShape get_cursor_shape() {
		return cursor_shape;
	}

	virtual Point2i mouse_get_position() const override { return mouse_position; }

	virtual void clipboard_set(const String &p_text) override { clipboard_text = p_text; }
	virtual String clipboard_get() const override { return clipboard_text; }
	virtual void clipboard_set_primary(const String &p_text) override { primary_clipboard_text = p_text; }
	virtual String clipboard_get_primary() const override { return primary_clipboard_text; }

	virtual Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override {
		return Size2i(1920, 1080);
	}

	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override {
		cursor_shape = p_shape;
	}

	virtual void window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override {
		event_callback = p_callable;
	}

	static void register_mock_driver() {
		DisplayServer::register_create_function("mock", create_func, get_rendering_drivers_func);
	}
};

// Utility macros to send an event actions to a given object
// Requires Message Queue and InputMap to be setup.
// SEND_GUI_ACTION    - takes an input map key. e.g SEND_GUI_ACTION("ui_text_newline").
// SEND_GUI_KEY_EVENT - takes a keycode set.   e.g SEND_GUI_KEY_EVENT(Key::A | KeyModifierMask::META).
// SEND_GUI_KEY_UP_EVENT - takes a keycode set.   e.g SEND_GUI_KEY_UP_EVENT(Key::A | KeyModifierMask::META).
// SEND_GUI_MOUSE_BUTTON_EVENT - takes a position, mouse button, mouse mask and modifiers e.g SEND_GUI_MOUSE_BUTTON_EVENT(Vector2(50, 50), MOUSE_BUTTON_NONE, MOUSE_BUTTON_NONE, Key::None);
// SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT - takes a position, mouse button, mouse mask and modifiers e.g SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Vector2(50, 50), MOUSE_BUTTON_NONE, MOUSE_BUTTON_NONE, Key::None);
// SEND_GUI_MOUSE_MOTION_EVENT - takes a position, mouse mask and modifiers e.g SEND_GUI_MOUSE_MOTION_EVENT(Vector2(50, 50), MouseButtonMask::LEFT, KeyModifierMask::META);
// SEND_GUI_DOUBLE_CLICK - takes a position and modifiers. e.g SEND_GUI_DOUBLE_CLICK(Vector2(50, 50), KeyModifierMask::META);

#define _SEND_DISPLAYSERVER_EVENT(m_event) DisplayServer::driver_as<DisplayServerMock>()->simulate_event(m_event);

#define SEND_GUI_ACTION(m_action) \
	{ \
		const List<Ref<InputEvent>> *events = InputMap::action_get_events(m_action); \
		const List<Ref<InputEvent>>::Element *first_event = events->front(); \
		Ref<InputEventKey> event = first_event->get()->duplicate(); \
		event->set_pressed(true); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

#define SEND_GUI_KEY_EVENT(m_input) \
	{ \
		Ref<InputEventKey> event = InputEventKey::create_reference(m_input); \
		event->set_pressed(true); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

#define SEND_GUI_KEY_UP_EVENT(m_input) \
	{ \
		Ref<InputEventKey> event = InputEventKey::create_reference(m_input); \
		event->set_pressed(false); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

#define _UPDATE_EVENT_MODIFIERS(m_event, m_modifiers) \
	m_event->set_shift_pressed(((m_modifiers) & KeyModifierMask::SHIFT) != Key::NONE); \
	m_event->set_alt_pressed(((m_modifiers) & KeyModifierMask::ALT) != Key::NONE); \
	m_event->set_ctrl_pressed(((m_modifiers) & KeyModifierMask::CTRL) != Key::NONE); \
	m_event->set_meta_pressed(((m_modifiers) & KeyModifierMask::META) != Key::NONE);

#define _CREATE_GUI_MOUSE_EVENT(m_screen_pos, m_input, m_mask, m_modifiers) \
	Ref<InputEventMouseButton> event; \
	event.instantiate(); \
	event->set_position(m_screen_pos); \
	event->set_button_index(m_input); \
	event->set_button_mask(m_mask); \
	event->set_factor(1); \
	_UPDATE_EVENT_MODIFIERS(event, m_modifiers); \
	event->set_pressed(true);

#define _CREATE_GUI_TOUCH_EVENT(m_screen_pos, m_pressed, m_double) \
	Ref<InputEventScreenTouch> event; \
	event.instantiate(); \
	event->set_position(m_screen_pos); \
	event->set_pressed(m_pressed); \
	event->set_double_tap(m_double);

#define SEND_GUI_MOUSE_BUTTON_EVENT(m_screen_pos, m_input, m_mask, m_modifiers) \
	{ \
		_CREATE_GUI_MOUSE_EVENT(m_screen_pos, m_input, m_mask, m_modifiers); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

#define SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(m_screen_pos, m_input, m_mask, m_modifiers) \
	{ \
		_CREATE_GUI_MOUSE_EVENT(m_screen_pos, m_input, m_mask, m_modifiers); \
		event->set_pressed(false); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

#define SEND_GUI_DOUBLE_CLICK(m_screen_pos, m_modifiers) \
	{ \
		_CREATE_GUI_MOUSE_EVENT(m_screen_pos, MouseButton::LEFT, MouseButtonMask::NONE, m_modifiers); \
		event->set_double_click(true); \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}

// We toggle _print_error_enabled to prevent display server not supported warnings.
#define SEND_GUI_MOUSE_MOTION_EVENT(m_screen_pos, m_mask, m_modifiers) \
	{ \
		bool errors_enabled = CoreGlobals::print_error_enabled; \
		CoreGlobals::print_error_enabled = false; \
		Ref<InputEventMouseMotion> event; \
		event.instantiate(); \
		event->set_position(m_screen_pos); \
		event->set_button_mask(m_mask); \
		_UPDATE_EVENT_MODIFIERS(event, m_modifiers); \
		_SEND_DISPLAYSERVER_EVENT(event); \
		CoreGlobals::print_error_enabled = errors_enabled; \
	}

#define SEND_GUI_TOUCH_EVENT(m_screen_pos, m_pressed, m_double) \
	{ \
		_CREATE_GUI_TOUCH_EVENT(m_screen_pos, m_pressed, m_double) \
		_SEND_DISPLAYSERVER_EVENT(event); \
	}
// --- End tests/display_server_mock.h ---
