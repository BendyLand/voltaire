/**************************************************************************/
/*  display_server_android.cpp                                            */
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

#include "platform/android/os_android.h"

#include "os_android.h"
#include "platform/android/os_android.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/os/os.h"
#include "servers/display/display_server.h"

#if defined(RD_ENABLED)
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/rendering_device.h"

#if defined(VULKAN_ENABLED)
#include "platform/android/os_android.h"
#endif
#endif

#ifdef GLES3_ENABLED
#include "drivers/gles3/rasterizer_gles3.h"

#include <EGL/egl.h>
#endif

#if defined(RD_ENABLED)
static RenderingContextDriver *rendering_context_global = nullptr;
static bool rendering_context_global_checked = false;
#endif

bool DisplayServerAndroid::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
		case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
			return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		} break;
#endif
		case DisplayServerEnums::FEATURE_PIP_MODE: {
			GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
			ERR_FAIL_NULL_V(godot_java, false);
			return godot_java->is_pip_mode_supported();
		} break;

		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		//case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
		//case DisplayServerEnums::FEATURE_HIDPI:
		//case DisplayServerEnums::FEATURE_ICON:
		//case DisplayServerEnums::FEATURE_IME:
		case DisplayServerEnums::FEATURE_MOUSE:
		//case DisplayServerEnums::FEATURE_MOUSE_WARP:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
		//case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
		case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME:
		//case DisplayServerEnums::FEATURE_NATIVE_ICON:
		case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
		case DisplayServerEnums::FEATURE_ORIENTATION:
		case DisplayServerEnums::FEATURE_TOUCHSCREEN:
		case DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD:
		case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH:
			return true;
		default:
			return false;
	}
}

String DisplayServerAndroid::get_name() const {
	return "Android";
}

bool DisplayServerAndroid::tts_is_speaking() const {
	return TTS_Android::is_speaking();
}

bool DisplayServerAndroid::tts_is_paused() const {
	return TTS_Android::is_paused();
}

TypedArray<Dictionary> DisplayServerAndroid::tts_get_voices() const {
	return TTS_Android::get_voices();
}

void DisplayServerAndroid::tts_speak(const String &p_text, const String &p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt) {
	TTS_Android::speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServerAndroid::tts_pause() {
	TTS_Android::pause();
}

void DisplayServerAndroid::tts_resume() {
	TTS_Android::resume();
}

void DisplayServerAndroid::tts_stop() {
	TTS_Android::stop();
}

bool DisplayServerAndroid::is_dark_mode_supported() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	return godot_java->is_dark_mode_supported();
}

bool DisplayServerAndroid::is_dark_mode() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	return godot_java->is_dark_mode();
}

void DisplayServerAndroid::set_system_theme_change_callback(const Callable &p_callable) {
	system_theme_changed = p_callable;
}

void DisplayServerAndroid::emit_system_theme_changed() {
	if (system_theme_changed.is_valid()) {
		system_theme_changed.call_deferred();
	}
}

void DisplayServerAndroid::set_hardware_keyboard_connection_change_callback(const Callable &p_callable) {
	hardware_keyboard_connection_changed = p_callable;
}

void DisplayServerAndroid::emit_hardware_keyboard_connection_changed(bool p_connected) {
	if (hardware_keyboard_connection_changed.is_valid()) {
		hardware_keyboard_connection_changed.call_deferred(p_connected);
	}
}

void DisplayServerAndroid::clipboard_set(const String &p_text) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);

	if (godot_java->has_set_clipboard()) {
		godot_java->set_clipboard(p_text);
	} else {
		DisplayServerDriver::clipboard_set(p_text);
	}
}

String DisplayServerAndroid::clipboard_get() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, String());

	if (godot_java->has_get_clipboard()) {
		return godot_java->get_clipboard();
	} else {
		return DisplayServerDriver::clipboard_get();
	}
}

bool DisplayServerAndroid::clipboard_has() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);

	if (godot_java->has_has_clipboard()) {
		return godot_java->has_clipboard();
	} else {
		return DisplayServerDriver::clipboard_has();
	}
}

Error DisplayServerAndroid::dialog_show(String p_title, String p_description, Vector<String> p_buttons, const Callable &p_callback) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	dialog_callback = p_callback;
	return godot_java->show_dialog(p_title, p_description, p_buttons);
}

void DisplayServerAndroid::emit_dialog_callback(int p_button_index) {
	if (dialog_callback.is_valid()) {
		dialog_callback.call_deferred(p_button_index);
	}
}

Error DisplayServerAndroid::dialog_input_text(String p_title, String p_description, String p_partial, const Callable &p_callback) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	input_dialog_callback = p_callback;
	return godot_java->show_input_dialog(p_title, p_description, p_partial);
}

void DisplayServerAndroid::emit_input_dialog_callback(String p_text) {
	if (input_dialog_callback.is_valid()) {
		input_dialog_callback.call_deferred(p_text);
	}
}

Error DisplayServerAndroid::file_dialog_show(const String &p_title, const String &p_current_directory, const String &p_filename, bool p_show_hidden, DisplayServerEnums::FileDialogMode p_mode, const Vector<String> &p_filters, const Callable &p_callback, DisplayServerEnums::WindowID p_window_id) {
	ERR_FAIL_COND_V(p_window_id != DisplayServerEnums::MAIN_WINDOW_ID, ERR_UNAVAILABLE);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, FAILED);
	file_picker_callback = p_callback;
	return godot_java->show_file_picker(p_current_directory, p_filename, p_mode, p_filters);
}

void DisplayServerAndroid::emit_file_picker_callback(bool p_ok, const Vector<String> &p_selected_paths) {
	if (file_picker_callback.is_valid()) {
		file_picker_callback.call_deferred(p_ok, p_selected_paths, 0);
	}
}

Color DisplayServerAndroid::get_accent_color() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, Color(0, 0, 0, 0));
	return godot_java->get_accent_color();
}

Color DisplayServerAndroid::get_base_color() const {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, Color(0, 0, 0, 0));
	return godot_java->get_base_color();
}

TypedArray<Rect2> DisplayServerAndroid::get_display_cutouts(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, TypedArray<Rect2>());

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, Array());
	return godot_io_java->get_display_cutouts();
}

Rect2i DisplayServerAndroid::get_display_safe_area(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, Rect2i());
	return godot_io_java->get_display_safe_area();
}

void DisplayServerAndroid::screen_set_keep_on(bool p_enable) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);

	godot_java->set_keep_screen_on(p_enable);
	keep_screen_on = p_enable;
}

bool DisplayServerAndroid::screen_is_kept_on() const {
	return keep_screen_on;
}

void DisplayServerAndroid::screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen) {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX(p_screen, screen_count);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	godot_io_java->set_screen_orientation(p_orientation);
}

DisplayServerEnums::ScreenOrientation DisplayServerAndroid::screen_get_orientation(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, DisplayServerEnums::SCREEN_LANDSCAPE);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, DisplayServerEnums::SCREEN_LANDSCAPE);

	const int orientation = godot_io_java->get_screen_orientation();
	ERR_FAIL_INDEX_V_MSG(orientation, 7, DisplayServerEnums::SCREEN_LANDSCAPE, "Unrecognized screen orientation");
	return (DisplayServerEnums::ScreenOrientation)orientation;
}

int DisplayServerAndroid::get_display_rotation() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	return godot_io_java->get_display_rotation();
}

int DisplayServerAndroid::get_screen_count() const {
	return 1;
}

int DisplayServerAndroid::get_primary_screen() const {
	return 0;
}

Point2i DisplayServerAndroid::screen_get_position(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return Point2i(0, 0);
}

Size2i DisplayServerAndroid::screen_get_size(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	return OS_Android::get_display_size();
}

Rect2i DisplayServerAndroid::screen_get_usable_rect(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	Size2i display_size = OS_Android::get_display_size();
	return Rect2i(0, 0, display_size.width, display_size.height);
}

int DisplayServerAndroid::screen_get_dpi(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 160);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 160);

	return godot_io_java->get_screen_dpi();
}

float DisplayServerAndroid::screen_get_scale(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 1.0f);

	float screen_scale = godot_io_java->get_scaled_density();

	// Update the scale to avoid cropping.
	Size2i screen_size = screen_get_size(p_screen);
	if (screen_size != Size2i()) {
		float width_scale = screen_size.width / (float)OS_Android::DEFAULT_WINDOW_WIDTH;
		float height_scale = screen_size.height / (float)OS_Android::DEFAULT_WINDOW_HEIGHT;
		screen_scale = MIN(screen_scale, MIN(width_scale, height_scale));
	}

	return screen_scale;
}

float DisplayServerAndroid::screen_get_refresh_rate(int p_screen) const {
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	if (!godot_io_java) {
		ERR_PRINT("An error occurred while trying to get the screen refresh rate.");
		return SCREEN_REFRESH_RATE_FALLBACK;
	}

	return godot_io_java->get_screen_refresh_rate(SCREEN_REFRESH_RATE_FALLBACK);
}

bool DisplayServerAndroid::is_touchscreen_available() const {
	return true;
}

void DisplayServerAndroid::virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end) {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	if (godot_io_java->has_vk()) {
		godot_io_java->show_vk(p_existing_text, (int)p_type, p_max_length, p_cursor_start, p_cursor_end);
	} else {
		ERR_PRINT("Virtual keyboard not available");
	}
}

void DisplayServerAndroid::virtual_keyboard_hide() {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL(godot_io_java);

	if (godot_io_java->has_vk()) {
		godot_io_java->hide_vk();
	} else {
		ERR_PRINT("Virtual keyboard not available");
	}
}

int DisplayServerAndroid::virtual_keyboard_get_height() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	return godot_io_java->get_vk_height();
}

bool DisplayServerAndroid::has_hardware_keyboard() const {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, false);

	return godot_io_java->has_hardware_keyboard();
}

void DisplayServerAndroid::window_set_window_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	window_event_callback = p_callable;
}

void DisplayServerAndroid::window_set_input_event_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	input_event_callback = p_callable;
}

void DisplayServerAndroid::window_set_input_text_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	input_text_callback = p_callable;
}

void DisplayServerAndroid::window_set_rect_changed_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	rect_changed_callback = p_callable;
}

void DisplayServerAndroid::window_set_drop_files_callback(const Callable &p_callable, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

template <typename... Args>
void DisplayServerAndroid::_window_callback(const Callable &p_callable, bool p_deferred, const Args &...p_rest_args) const {
	if (p_callable.is_valid()) {
		if (p_deferred) {
			p_callable.call_deferred(p_rest_args...);
		} else {
			p_callable.call(p_rest_args...);
		}
	}
}

void DisplayServerAndroid::send_window_event(DisplayServerEnums::WindowEvent p_event, bool p_deferred) const {
	_window_callback(window_event_callback, p_deferred, int(p_event));
}

void DisplayServerAndroid::send_input_event(const Ref<InputEvent> &p_event) const {
	_window_callback(input_event_callback, false, p_event);
}

void DisplayServerAndroid::send_input_text(const String &p_text) const {
	_window_callback(input_text_callback, false, p_text, false);
}

void DisplayServerAndroid::_dispatch_input_events(const Ref<InputEvent> &p_event) {
	if (DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>()) {
		ds->send_input_event(p_event);
	}
}

Vector<DisplayServerEnums::WindowID> DisplayServerAndroid::get_window_list() const {
	Vector<DisplayServerEnums::WindowID> ret;
	ret.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
	return ret;
}

DisplayServerEnums::WindowID DisplayServerAndroid::get_window_at_screen_position(const Point2i &p_position) const {
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

int64_t DisplayServerAndroid::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, 0);
	switch (p_handle_type) {
		case DisplayServerEnums::WINDOW_HANDLE: {
			return reinterpret_cast<int64_t>(OS_Android::get_godot_java()->get_activity());
		}
		case DisplayServerEnums::WINDOW_VIEW: {
			return 0; // Not supported.
		}
#ifdef GLES3_ENABLED
		case DisplayServerEnums::DISPLAY_HANDLE: {
			if (rendering_driver == "opengl3") {
				return reinterpret_cast<int64_t>(eglGetCurrentDisplay());
			}
			return 0;
		}
		case DisplayServerEnums::OPENGL_CONTEXT: {
			if (rendering_driver == "opengl3") {
				return reinterpret_cast<int64_t>(eglGetCurrentContext());
			}
			return 0;
		}
		case DisplayServerEnums::EGL_DISPLAY: {
			// @todo Find a way to get this from the Java side.
			return 0;
		}
		case DisplayServerEnums::EGL_CONFIG: {
			// @todo Find a way to get this from the Java side.
			return 0;
		}
#endif
		default: {
			return 0;
		}
	}
}

void DisplayServerAndroid::window_attach_instance_id(ObjectID p_instance, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	window_attached_instance_id = p_instance;
}

ObjectID DisplayServerAndroid::window_get_attached_instance_id(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, ObjectID());
	return window_attached_instance_id;
}

void DisplayServerAndroid::window_set_title(const String &p_title, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

int DisplayServerAndroid::window_get_current_screen(DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, DisplayServerEnums::INVALID_SCREEN);
	return 0;
}

void DisplayServerAndroid::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Point2i DisplayServerAndroid::window_get_position(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

Point2i DisplayServerAndroid::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return Point2i();
}

void DisplayServerAndroid::window_set_position(const Point2i &p_position, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_max_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAndroid::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_min_size(DisplayServerEnums::WindowID p_window) const {
	return Size2i();
}

void DisplayServerAndroid::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

Size2i DisplayServerAndroid::window_get_size(DisplayServerEnums::WindowID p_window) const {
	return OS_Android::get_display_size();
}

Size2i DisplayServerAndroid::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window) const {
	return OS_Android::get_display_size();
}

void DisplayServerAndroid::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window) {
	OS_Android::get_godot_java()->enable_immersive_mode(p_mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || p_mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN);
}

DisplayServerEnums::WindowMode DisplayServerAndroid::window_get_mode(DisplayServerEnums::WindowID p_window) const {
	if (OS_Android::get_godot_java()->is_in_immersive_mode()) {
		return DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
	} else {
		return DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
	}
}

bool DisplayServerAndroid::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window) const {
	return false;
}

void DisplayServerAndroid::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

bool DisplayServerAndroid::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window) const {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, false);
	switch (p_flag) {
		case DisplayServerEnums::WindowFlags::WINDOW_FLAG_TRANSPARENT:
			return is_window_transparency_available();

		default:
			return false;
	}
}

void DisplayServerAndroid::window_request_attention(DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

void DisplayServerAndroid::window_move_to_foreground(DisplayServerEnums::WindowID p_window) {
	// Not supported on Android.
}

bool DisplayServerAndroid::window_is_focused(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerAndroid::window_can_draw(DisplayServerEnums::WindowID p_window) const {
	return true;
}

bool DisplayServerAndroid::can_any_window_draw() const {
	return true;
}

void DisplayServerAndroid::window_set_color(const Color &p_color) {
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_window_color(p_color);
}

void DisplayServerAndroid::process_events() {
	Input::flush_buffered_events();
}

Vector<String> DisplayServerAndroid::get_rendering_drivers_func() {
	Vector<String> drivers;

#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
#endif
#ifdef VULKAN_ENABLED
	drivers.push_back("vulkan");
#endif

	return drivers;
}

DisplayServerDriver *DisplayServerAndroid::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	DisplayServerDriver *ds = memnew(DisplayServerAndroid(p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen, p_context, p_parent_window, r_error));
	if (r_error != OK) {
		if (p_rendering_driver == "vulkan") {
			OS::alert(
					"Your device seems not to support the required Vulkan version.\n\n"
					"Please try exporting your game using the 'gl_compatibility' renderer.",
					"Unable to initialize Vulkan video driver");
		} else {
			OS::alert(
					"Your device seems not to support the required OpenGL ES 3.0 version.",
					"Unable to initialize OpenGL video driver");
		}
	}
	return ds;
}

void DisplayServerAndroid::register_android_driver() {
	DisplayServer::register_create_function("android", create_func, get_rendering_drivers_func);
}

#ifdef VULKAN_ENABLED
bool DisplayServerAndroid::check_vulkan_global_context(bool p_vulkan_requirements_met) {
	if (!rendering_context_global_checked) {
		bool fallback_to_opengl3 = GLOBAL_GET("rendering/rendering_device/fallback_to_opengl3");
		Error err = ERR_CANT_CREATE;
		if (p_vulkan_requirements_met) {
			rendering_context_global = memnew(RenderingContextDriverVulkanAndroid);
			err = rendering_context_global->initialize();
		}

		if (err != OK) {
			if (rendering_context_global != nullptr) {
				memdelete(rendering_context_global);
				rendering_context_global = nullptr;
			}

#if defined(GLES3_ENABLED)
			if (fallback_to_opengl3) {
				WARN_PRINT("Your device does not seem to support Vulkan, switching to OpenGL 3.");
				OS::set_current_rendering_driver_name("opengl3", OS::RENDERING_SOURCE_FALLBACK);
				OS::set_current_rendering_method("gl_compatibility", OS::RENDERING_SOURCE_FALLBACK);
			} else
#endif
			{
				ERR_PRINT("Failed to initialize Vulkan context.");
			}
		}

		rendering_context_global_checked = true;
	}

	return rendering_context_global != nullptr;
}

void DisplayServerAndroid::free_vulkan_global_context() {
	if (rendering_context_global != nullptr) {
		memdelete(rendering_context_global);
		rendering_context_global = nullptr;
		rendering_context_global_checked = false;
	}
}
#endif

void DisplayServerAndroid::reset_window() {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		if (rendering_device) {
			rendering_device->screen_free(DisplayServerEnums::MAIN_WINDOW_ID);
		}

		DisplayServerEnums::VSyncMode last_vsync_mode = rendering_context_global->window_get_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID);
		rendering_context_global->window_destroy(DisplayServerEnums::MAIN_WINDOW_ID);

		union {
#ifdef VULKAN_ENABLED
			RenderingContextDriverVulkanAndroid::WindowPlatformData vulkan;
#endif
		} wpd;
#ifdef VULKAN_ENABLED
		if (rendering_driver == "vulkan") {
			ANativeWindow *native_window = OS_Android::get_native_window();
			ERR_FAIL_NULL(native_window);
			wpd.vulkan.window = native_window;
		}
#endif

		if (rendering_context_global->window_create(DisplayServerEnums::MAIN_WINDOW_ID, &wpd) != OK) {
			ERR_PRINT(vformat("Failed to reset %s window.", rendering_driver));
			return;
		}

		Size2i display_size = OS_Android::get_display_size();
		rendering_context_global->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, display_size.width, display_size.height);
		rendering_context_global->window_set_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID, last_vsync_mode);

		if (rendering_device) {
			rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);
		}
	}
#endif
}

void DisplayServerAndroid::notify_surface_changed(int p_width, int p_height) {
	if (rect_changed_callback.is_valid()) {
		rect_changed_callback.call(Rect2i(0, 0, p_width, p_height));
	}
}

void DisplayServerAndroid::notify_application_paused() {
#if defined(RD_ENABLED)
	if (rendering_device) {
		rendering_device->update_pipeline_cache();
	}
#endif // defined(RD_ENABLED)
}

DisplayServerAndroid::DisplayServerAndroid(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	rendering_driver = p_rendering_driver;

	keep_screen_on = GLOBAL_GET("display/window/energy_saving/keep_screen_on");

	NativeMenu::initialize();

#ifdef VULKAN_ENABLED
	if (rendering_driver == "vulkan") {
		check_vulkan_global_context(true);
		if (rendering_context_global == nullptr) {
			ERR_PRINT("Can't initialize display server with Vulkan driver because no Vulkan context is available.");
			r_error = ERR_UNAVAILABLE;
			return;
		}

		ANativeWindow *native_window = OS_Android::get_native_window();
		ERR_FAIL_NULL(native_window);

		RenderingContextDriverVulkanAndroid::WindowPlatformData wpd;
		wpd.window = native_window;

		if (rendering_context_global->window_create(DisplayServerEnums::MAIN_WINDOW_ID, &wpd) != OK) {
			ERR_PRINT(vformat("Failed to create %s window.", rendering_driver));
			r_error = ERR_UNAVAILABLE;
			return;
		}

		Size2i display_size = OS_Android::get_display_size();
		rendering_context_global->window_set_size(DisplayServerEnums::MAIN_WINDOW_ID, display_size.width, display_size.height);
		rendering_context_global->window_set_vsync_mode(DisplayServerEnums::MAIN_WINDOW_ID, p_vsync_mode);

		rendering_device = memnew(RenderingDevice);
		if (rendering_device->initialize(rendering_context_global, DisplayServerEnums::MAIN_WINDOW_ID) != OK) {
			rendering_device = nullptr;
			r_error = ERR_UNAVAILABLE;
			return;
		}

		rendering_device->screen_create(DisplayServerEnums::MAIN_WINDOW_ID);

		RendererCompositorRD::make_current();
	}
#endif

#if defined(GLES3_ENABLED)
	if (rendering_driver == "opengl3") {
		RasterizerGLES3::make_current(false);
	}
#endif

	Input::set_event_dispatch_function(_dispatch_input_events);

	r_error = OK;
}

DisplayServerAndroid::~DisplayServerAndroid() {
	NativeMenu::finalize();

#if defined(RD_ENABLED)
	memdelete(rendering_device);

	free_vulkan_global_context();
#endif
}

void DisplayServerAndroid::process_accelerometer(const Vector3 &p_accelerometer) {
	Input::set_accelerometer(p_accelerometer);
}

void DisplayServerAndroid::process_gravity(const Vector3 &p_gravity) {
	Input::set_gravity(p_gravity);
}

void DisplayServerAndroid::process_magnetometer(const Vector3 &p_magnetometer) {
	Input::set_magnetometer(p_magnetometer);
}

void DisplayServerAndroid::process_gyroscope(const Vector3 &p_gyroscope) {
	Input::set_gyroscope(p_gyroscope);
}

void DisplayServerAndroid::_mouse_update_mode() {
	DisplayServerEnums::MouseMode wanted_mouse_mode = mouse_mode_override_enabled
			? mouse_mode_override
			: mouse_mode_base;

	if (!OS_Android::get_godot_java()->get_godot_view()->can_update_pointer_icon() || !OS_Android::get_godot_java()->get_godot_view()->can_capture_pointer()) {
		return;
	}
	if (mouse_mode == wanted_mouse_mode) {
		return;
	}

	if (wanted_mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_HIDDEN) {
		OS_Android::get_godot_java()->get_godot_view()->set_pointer_icon(CURSOR_TYPE_NULL);
	} else {
		cursor_set_shape(cursor_shape);
	}

	if (wanted_mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_CAPTURED) {
		OS_Android::get_godot_java()->get_godot_view()->request_pointer_capture();
	} else {
		OS_Android::get_godot_java()->get_godot_view()->release_pointer_capture();
	}

	mouse_mode = wanted_mouse_mode;
}

void DisplayServerAndroid::mouse_set_mode(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_base) {
		return;
	}
	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerAndroid::mouse_get_mode() const {
	return mouse_mode;
}

void DisplayServerAndroid::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) {
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_override) {
		return;
	}
	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerAndroid::mouse_get_mode_override() const {
	return mouse_mode_override;
}

void DisplayServerAndroid::mouse_set_mode_override_enabled(bool p_override_enabled) {
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerAndroid::mouse_is_mode_override_enabled() const {
	return mouse_mode_override_enabled;
}

Point2i DisplayServerAndroid::mouse_get_position() const {
	return Input::get_mouse_position();
}

uint32_t DisplayServerAndroid::mouse_get_button_state() const {
	return Input::get_mouse_button_mask();
}

void DisplayServerAndroid::_cursor_set_shape_helper(DisplayServerEnums::CursorShape p_shape, bool force) {
	if (!OS_Android::get_godot_java()->get_godot_view()->can_update_pointer_icon()) {
		return;
	}
	if (cursor_shape == p_shape && !force) {
		return;
	}

	cursor_shape = p_shape;

	if (mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_VISIBLE || mouse_mode == DisplayServerEnums::MouseMode::MOUSE_MODE_CONFINED) {
		OS_Android::get_godot_java()->get_godot_view()->set_pointer_icon(android_cursors[cursor_shape]);
	}
}

void DisplayServerAndroid::cursor_set_shape(DisplayServerEnums::CursorShape p_shape) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	_cursor_set_shape_helper(p_shape);
}

DisplayServerEnums::CursorShape DisplayServerAndroid::cursor_get_shape() const {
	return cursor_shape;
}

void DisplayServerAndroid::cursor_set_custom_image(const Ref<Resource> &p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2 &p_hotspot) {
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);
	String cursor_path = p_cursor.is_valid() ? p_cursor->get_path() : "";
	if (!cursor_path.is_empty()) {
		cursor_path = ProjectSettings::globalize_path(cursor_path);
	}
	OS_Android::get_godot_java()->get_godot_view()->configure_pointer_icon(android_cursors[cursor_shape], cursor_path, p_hotspot);
	_cursor_set_shape_helper(p_shape, true);
}

void DisplayServerAndroid::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window) {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		rendering_context_global->window_set_vsync_mode(p_window, p_vsync_mode);
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerAndroid::window_get_vsync_mode(DisplayServerEnums::WindowID p_window) const {
#if defined(RD_ENABLED)
	if (rendering_context_global) {
		return rendering_context_global->window_get_vsync_mode(p_window);
	}
#endif
	return DisplayServerEnums::VSYNC_ENABLED;
}

void DisplayServerAndroid::reset_swap_buffers_flag() {
	swap_buffers_flag = false;
}

bool DisplayServerAndroid::should_swap_buffers() const {
	return swap_buffers_flag;
}

void DisplayServerAndroid::swap_buffers() {
	swap_buffers_flag = true;
}

void DisplayServerAndroid::set_native_icon(const String &p_filename) {
	// NOT SUPPORTED
}

void DisplayServerAndroid::set_icon(const Ref<Image> &p_icon) {
	// NOT SUPPORTED
}

bool DisplayServerAndroid::is_window_transparency_available() const {
	return GLOBAL_GET_CACHED(bool, "display/window/per_pixel_transparency/allowed");
}

bool DisplayServerAndroid::is_in_pip_mode(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND_V(p_window != DisplayServerEnums::MAIN_WINDOW_ID, false);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL_V(godot_java, false);
	return godot_java->is_in_pip_mode();
}

void DisplayServerAndroid::pip_mode_enter(DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->enter_pip_mode();
}

void DisplayServerAndroid::pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_pip_mode_aspect_ratio(p_numerator, p_denominator);
}

void DisplayServerAndroid::pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window) {
	ERR_FAIL_COND(p_window != DisplayServerEnums::MAIN_WINDOW_ID);
	GodotJavaWrapper *godot_java = OS_Android::get_godot_java();
	ERR_FAIL_NULL(godot_java);
	godot_java->set_auto_enter_pip_mode_on_background(p_auto_enter_on_background);
}


// --- Merged from platform/android/android_input_handler.cpp ---
#include "platform/android/os_android.h"


#include "core/input/input.h"

void AndroidInputHandler::process_joy_event(AndroidInputHandler::JoypadEvent p_event) {
	switch (p_event.type) {
		case JOY_EVENT_BUTTON:
			Input::joy_button(p_event.device, (JoyButton)p_event.index, p_event.pressed);
			break;
		case JOY_EVENT_AXIS:
			Input::joy_axis(p_event.device, (JoyAxis)p_event.index, p_event.value);
			break;
		case JOY_EVENT_HAT:
			Input::joy_hat(p_event.device, p_event.hat);
			break;
		default:
			return;
	}
}

void AndroidInputHandler::_set_key_modifier_state(Ref<InputEventWithModifiers> ev, Key p_keycode) {
	if (p_keycode != Key::SHIFT) {
		ev->set_shift_pressed(shift_mem);
	}
	if (p_keycode != Key::ALT) {
		ev->set_alt_pressed(alt_mem);
	}
	if (p_keycode != Key::META) {
		ev->set_meta_pressed(meta_mem);
	}
	if (p_keycode != Key::CTRL) {
		ev->set_ctrl_pressed(control_mem);
	}
}

void AndroidInputHandler::process_key_event(int p_physical_keycode, int p_unicode, int p_key_label, bool p_pressed, bool p_echo) {
	static char32_t prev_wc = 0;
	char32_t unicode = p_unicode;
	if ((p_unicode & 0xfffffc00) == 0xd800) {
		if (prev_wc != 0) {
			ERR_PRINT("invalid utf16 surrogate input");
		}
		prev_wc = unicode;
		return; // Skip surrogate.
	} else if ((unicode & 0xfffffc00) == 0xdc00) {
		if (prev_wc == 0) {
			ERR_PRINT("invalid utf16 surrogate input");
			return; // Skip invalid surrogate.
		}
		unicode = (prev_wc << 10UL) + unicode - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
		prev_wc = 0;
	} else {
		prev_wc = 0;
	}

	Ref<InputEventKey> ev;
	ev.instantiate();

	Key physical_keycode = godot_code_from_android_code(p_physical_keycode);
	Key keycode;
	if (unicode == '\b') { // 0x08
		keycode = Key::BACKSPACE;
	} else if (unicode == '\t') { // 0x09
		keycode = Key::TAB;
	} else if (unicode == '\n') { // 0x0A
		keycode = Key::ENTER;
	} else if (unicode == 0x1B) {
		keycode = Key::ESCAPE;
	} else if (unicode == 0x1F) {
		keycode = Key::KEY_DELETE;
	} else {
		keycode = fix_keycode(unicode, physical_keycode);
	}

	switch (physical_keycode) {
		case Key::SHIFT: {
			shift_mem = p_pressed;
		} break;
		case Key::ALT: {
			alt_mem = p_pressed;
		} break;
		case Key::CTRL: {
			control_mem = p_pressed;
		} break;
		case Key::META: {
			meta_mem = p_pressed;
		} break;
		default:
			break;
	}

	ev->set_keycode(keycode);
	ev->set_physical_keycode(physical_keycode);
	ev->set_key_label(fix_key_label(p_key_label, keycode));
	ev->set_unicode(fix_unicode(unicode));
	ev->set_location(godot_location_from_android_code(p_physical_keycode));
	ev->set_pressed(p_pressed);
	ev->set_echo(p_echo);

	_set_key_modifier_state(ev, keycode);

	if (p_physical_keycode == AKEYCODE_BACK && p_pressed) {
		if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
			dsa->send_window_event(DisplayServerEnums::WINDOW_EVENT_GO_BACK_REQUEST, true);
		}
	}

	Input::parse_input_event(ev);
}

void AndroidInputHandler::_cancel_all_touch() {
	_parse_all_touch(false, true);
	touch.clear();
}

void AndroidInputHandler::_parse_all_touch(bool p_pressed, bool p_canceled) {
	if (touch.size()) {
		//end all if exist
		for (int i = 0; i < touch.size(); i++) {
			Ref<InputEventScreenTouch> ev;
			ev.instantiate();
			ev->set_index(touch[i].id);
			ev->set_pressed(p_pressed);
			ev->set_canceled(p_canceled);
			ev->set_position(touch[i].pos);
			ev->set_double_tap(touch[i].double_tap);
			Input::parse_input_event(ev);
		}
	}
}

void AndroidInputHandler::_release_all_touch() {
	_parse_all_touch(false, false);
	touch.clear();
}

void AndroidInputHandler::process_touch_event(int p_event, int p_pointer, const Vector<TouchPos> &p_points) {
	switch (p_event) {
		case AMOTION_EVENT_ACTION_DOWN: { //gesture begin
			// Release any remaining touches or mouse event
			_release_mouse_event_info();
			_release_all_touch();

			touch.resize(p_points.size());
			for (int i = 0; i < p_points.size(); i++) {
				touch.write[i].id = p_points[i].id;
				touch.write[i].pos = p_points[i].pos;
				touch.write[i].pressure = p_points[i].pressure;
				touch.write[i].tilt = p_points[i].tilt;
				touch.write[i].double_tap = p_points[i].double_tap;
			}

			//send touch
			_parse_all_touch(true, false);

		} break;
		case AMOTION_EVENT_ACTION_MOVE: { //motion
			if (touch.size() != p_points.size()) {
				return;
			}

			for (int i = 0; i < touch.size(); i++) {
				int idx = -1;
				for (int j = 0; j < p_points.size(); j++) {
					if (touch[i].id == p_points[j].id) {
						idx = j;
						break;
					}
				}

				ERR_CONTINUE(idx == -1);

				if (touch[i].pos == p_points[idx].pos) {
					continue; // Don't move unnecessarily.
				}

				Ref<InputEventScreenDrag> ev;
				ev.instantiate();
				ev->set_index(touch[i].id);
				ev->set_position(p_points[idx].pos);
				ev->set_relative(p_points[idx].pos - touch[i].pos);
				ev->set_relative_screen_position(ev->get_relative());
				ev->set_pressure(p_points[idx].pressure);
				ev->set_tilt(p_points[idx].tilt);
				Input::parse_input_event(ev);
				touch.write[i].pos = p_points[idx].pos;
			}

		} break;
		case AMOTION_EVENT_ACTION_CANCEL: {
			_cancel_all_touch();
		} break;
		case AMOTION_EVENT_ACTION_UP: { //release
			_release_all_touch();
		} break;
		case AMOTION_EVENT_ACTION_POINTER_DOWN: { // add touch
			for (int i = 0; i < p_points.size(); i++) {
				if (p_points[i].id == p_pointer) {
					TouchPos tp = p_points[i];
					touch.push_back(tp);

					Ref<InputEventScreenTouch> ev;
					ev.instantiate();

					ev->set_index(tp.id);
					ev->set_pressed(true);
					ev->set_position(tp.pos);
					Input::parse_input_event(ev);

					break;
				}
			}
		} break;
		case AMOTION_EVENT_ACTION_POINTER_UP: { // remove touch
			for (int i = 0; i < touch.size(); i++) {
				if (touch[i].id == p_pointer) {
					Ref<InputEventScreenTouch> ev;
					ev.instantiate();
					ev->set_index(touch[i].id);
					ev->set_pressed(false);
					ev->set_position(touch[i].pos);
					Input::parse_input_event(ev);
					touch.remove_at(i);

					break;
				}
			}
		} break;
	}
}

void AndroidInputHandler::_cancel_mouse_event_info(bool p_source_mouse_relative) {
	buttons_state = uint32_t();
	_parse_mouse_event_info(uint32_t(), false, true, false, p_source_mouse_relative);
	mouse_event_info.valid = false;
}

void AndroidInputHandler::_parse_mouse_event_info(uint32_t event_buttons_mask, bool p_pressed, bool p_canceled, bool p_double_click, bool p_source_mouse_relative) {
	if (!mouse_event_info.valid) {
		return;
	}

	Ref<InputEventMouseButton> ev;
	ev.instantiate();
	_set_key_modifier_state(ev, Key::NONE);
	if (p_source_mouse_relative) {
		ev->set_position(hover_prev_pos);
		ev->set_global_position(hover_prev_pos);
	} else {
		ev->set_position(mouse_event_info.pos);
		ev->set_global_position(mouse_event_info.pos);
		hover_prev_pos = mouse_event_info.pos;
	}
	ev->set_pressed(p_pressed);
	ev->set_canceled(p_canceled);
	uint32_t changed_button_mask = buttons_state.get_different(event_buttons_mask);

	buttons_state = event_buttons_mask;

	ev->set_button_index(_button_index_from_mask(changed_button_mask));
	ev->set_button_mask(event_buttons_mask);
	ev->set_double_click(p_double_click);
	Input::parse_input_event(ev);
}

void AndroidInputHandler::_release_mouse_event_info(bool p_source_mouse_relative) {
	_parse_mouse_event_info(uint32_t(), false, false, false, p_source_mouse_relative);
	mouse_event_info.valid = false;
}

void AndroidInputHandler::process_mouse_event(int p_event_action, int p_event_android_buttons_mask, Point2 p_event_pos, Vector2 p_delta, bool p_double_click, bool p_source_mouse_relative, float p_pressure, Vector2 p_tilt) {
	uint32_t event_buttons_mask = _android_button_mask_to_godot_button_mask(p_event_android_buttons_mask);
	switch (p_event_action) {
		case AMOTION_EVENT_ACTION_HOVER_MOVE: // hover move
		case AMOTION_EVENT_ACTION_HOVER_ENTER: // hover enter
		case AMOTION_EVENT_ACTION_HOVER_EXIT: { // hover exit
			// https://developer.android.com/reference/android/view/MotionEvent.html#ACTION_HOVER_ENTER
			Ref<InputEventMouseMotion> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			ev->set_position(p_event_pos);
			ev->set_global_position(p_event_pos);
			ev->set_relative(p_event_pos - hover_prev_pos);
			ev->set_relative_screen_position(ev->get_relative());
			Input::parse_input_event(ev);
			hover_prev_pos = p_event_pos;
		} break;

		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_BUTTON_PRESS: {
			// Release any remaining touches or mouse event
			_release_mouse_event_info();
			_release_all_touch();

			mouse_event_info.valid = true;
			mouse_event_info.pos = p_event_pos;
			_parse_mouse_event_info(event_buttons_mask, true, false, p_double_click, p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_CANCEL: {
			_cancel_mouse_event_info(p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_BUTTON_RELEASE: {
			_release_mouse_event_info(p_source_mouse_relative);
		} break;

		case AMOTION_EVENT_ACTION_MOVE: {
			if (!p_source_mouse_relative && !mouse_event_info.valid) {
				return;
			}

			Ref<InputEventMouseMotion> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			if (p_source_mouse_relative) {
				ev->set_position(hover_prev_pos);
				ev->set_global_position(hover_prev_pos);
				ev->set_relative(p_event_pos);
				ev->set_relative_screen_position(p_event_pos);
			} else {
				ev->set_position(p_event_pos);
				ev->set_global_position(p_event_pos);
				ev->set_relative(p_event_pos - hover_prev_pos);
				ev->set_relative_screen_position(ev->get_relative());
				mouse_event_info.pos = p_event_pos;
				hover_prev_pos = p_event_pos;
			}
			ev->set_button_mask(event_buttons_mask);
			ev->set_pressure(p_pressure);
			ev->set_tilt(p_tilt);
			Input::parse_input_event(ev);
		} break;

		case AMOTION_EVENT_ACTION_SCROLL: {
			Ref<InputEventMouseButton> ev;
			ev.instantiate();
			_set_key_modifier_state(ev, Key::NONE);
			if (p_source_mouse_relative) {
				ev->set_position(hover_prev_pos);
				ev->set_global_position(hover_prev_pos);
			} else {
				ev->set_position(p_event_pos);
				ev->set_global_position(p_event_pos);
			}
			ev->set_pressed(true);
			buttons_state = event_buttons_mask;
			if (p_delta.y > 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_UP, p_delta.y);
			} else if (p_delta.y < 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_DOWN, -p_delta.y);
			}

			if (p_delta.x > 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_RIGHT, p_delta.x);
			} else if (p_delta.x < 0) {
				_wheel_button_click(event_buttons_mask, ev, MouseButton::WHEEL_LEFT, -p_delta.x);
			}
		} break;
	}
}

void AndroidInputHandler::_wheel_button_click(uint32_t &ev, MouseButton wheel_button, float factor) {
	Ref<InputEventMouseButton> evd = ev->duplicate();
	_set_key_modifier_state(evd, Key::NONE);
	evd->set_button_index(wheel_button);
	evd->set_button_mask(event_buttons_mask.get_different(mouse_button_to_mask(wheel_button)));
	evd->set_factor(factor);
	Input::parse_input_event(evd);
	Ref<InputEventMouseButton> evdd = evd->duplicate();
	evdd->set_pressed(false);
	evdd->set_button_mask(event_buttons_mask);
	Input::parse_input_event(evdd);
}

void AndroidInputHandler::process_magnify(Point2 p_pos, float p_factor) {
	Ref<InputEventMagnifyGesture> magnify_event;
	magnify_event.instantiate();
	_set_key_modifier_state(magnify_event, Key::NONE);
	magnify_event->set_position(p_pos);
	magnify_event->set_factor(p_factor);
	Input::parse_input_event(magnify_event);
}

void AndroidInputHandler::process_pan(Point2 p_pos, Vector2 p_delta) {
	Ref<InputEventPanGesture> pan_event;
	pan_event.instantiate();
	_set_key_modifier_state(pan_event, Key::NONE);
	pan_event->set_position(p_pos);
	pan_event->set_delta(p_delta);
	Input::parse_input_event(pan_event);
}

MouseButton AndroidInputHandler::_button_index_from_mask(uint32_t button_mask) {
	switch (button_mask) {
		case MouseButtonMask::LEFT:
			return MouseButton::LEFT;
		case MouseButtonMask::RIGHT:
			return MouseButton::RIGHT;
		case MouseButtonMask::MIDDLE:
			return MouseButton::MIDDLE;
		case MouseButtonMask::MB_XBUTTON1:
			return MouseButton::MB_XBUTTON1;
		case MouseButtonMask::MB_XBUTTON2:
			return MouseButton::MB_XBUTTON2;
		default:
			return MouseButton::NONE;
	}
}

uint32_t AndroidInputHandler::_android_button_mask_to_godot_button_mask(int android_button_mask) {
	uint32_t godot_button_mask = MouseButtonMask::NONE;
	if (android_button_mask & AMOTION_EVENT_BUTTON_PRIMARY) {
		godot_button_mask.set_flag(MouseButtonMask::LEFT);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_SECONDARY) {
		godot_button_mask.set_flag(MouseButtonMask::RIGHT);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_TERTIARY) {
		godot_button_mask.set_flag(MouseButtonMask::MIDDLE);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_BACK) {
		godot_button_mask.set_flag(MouseButtonMask::MB_XBUTTON1);
	}
	if (android_button_mask & AMOTION_EVENT_BUTTON_FORWARD) {
		godot_button_mask.set_flag(MouseButtonMask::MB_XBUTTON2);
	}

	return godot_button_mask;
}


// --- Merged from platform/android/android_keys_utils.cpp ---
#include "platform/android/os_android.h"

Key godot_code_from_android_code(unsigned int p_code) {
	for (int i = 0; android_godot_code_pairs[i].android_code != AKEYCODE_MAX; i++) {
		if (android_godot_code_pairs[i].android_code == p_code) {
			return android_godot_code_pairs[i].godot_code;
		}
	}
	return Key::UNKNOWN;
}

KeyLocation godot_location_from_android_code(unsigned int p_code) {
	for (int i = 0; android_godot_location_pairs[i].android_code != AKEYCODE_MAX; i++) {
		if (android_godot_location_pairs[i].android_code == p_code) {
			return android_godot_location_pairs[i].godot_code;
		}
	}
	return KeyLocation::UNSPECIFIED;
}


// --- Merged from platform/android/rendering_context_driver_vulkan_android.cpp ---
#include "platform/android/os_android.h"

#ifdef VULKAN_ENABLED

#include <drivers/vulkan/godot_vulkan.h>

const char *RenderingContextDriverVulkanAndroid::_get_platform_surface_extension() const {
	return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkanAndroid::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkAndroidSurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	create_info.window = wpd->window;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateAndroidSurfaceKHR(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

bool RenderingContextDriverVulkanAndroid::_use_validation_layers() const {
	TightLocalVector<const char *> layer_names;
	Error err = _find_validation_layers(layer_names);

	// On Android, we use validation layers automatically if they were explicitly linked with the app.
	return (err == OK) && !layer_names.is_empty();
}

#endif // VULKAN_ENABLED


// --- Merged from platform/android/dialog_utils_jni.cpp ---
// --- Inlined from platform/android/dialog_utils_jni.h ---
#include <jni.h>

extern "C" {
JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_dialogCallback(JNIEnv *env, jclass clazz, jint p_button_index);
JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_inputDialogCallback(JNIEnv *env, jclass clazz, jstring p_text);
}

// --- End platform/android/dialog_utils_jni.h ---

#include "platform/android/os_android.h"

extern "C" {

JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_dialogCallback(JNIEnv *env, jclass clazz, jint p_button_index) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_dialog_callback(p_button_index);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_utils_DialogUtils_inputDialogCallback(JNIEnv *env, jclass clazz, jstring p_text) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		String text = jstring_to_string(p_text, env);
		ds->emit_input_dialog_callback(text);
	}
}
}
