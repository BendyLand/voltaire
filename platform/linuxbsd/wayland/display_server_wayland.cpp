/**************************************************************************/
/*  display_server_wayland.cpp                                            */
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

#include "display_server_wayland.h"

#ifdef WAYLAND_ENABLED

#define WAYLAND_DISPLAY_SERVER_DEBUG_LOGS_ENABLED
#ifdef WAYLAND_DISPLAY_SERVER_DEBUG_LOGS_ENABLED
#define DEBUG_LOG_WAYLAND(...) print_verbose(__VA_ARGS__)
#else
#define DEBUG_LOG_WAYLAND(...)
#endif

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"
#include "servers/rendering/renderer.h"

#ifdef RD_ENABLED
#ifdef VULKAN_ENABLED
#include "platform/linuxbsd/wayland/display_server_wayland.h"
#endif

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#endif

#ifdef GLES3_ENABLED
#include "core/io/file_access.h"
#include "platform/linuxbsd/wayland/display_server_wayland.h"
#include "drivers/egl/egl_manager.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "platform/linuxbsd/wayland/display_server_wayland.h"
#endif

#ifdef DBUS_ENABLED
#include "platform/linuxbsd/os_linuxbsd.h"

#ifdef SOWRAP_ENABLED
#include "dbus-so_wrap.h"
#else
#include <dbus/dbus.h>
#endif
#endif

#ifdef SPEECHD_ENABLED
#include "platform/linuxbsd/os_linuxbsd.h"
#endif

#define WAYLAND_MAX_FRAME_TIME_US (1'000'000)
#define WINDOW_READY_TIMEOUT_MS (10'000)

String DisplayServerWayland::_get_app_id_from_context(DisplayServerEnums::Context p_context)
{
	String app_id;

	switch (p_context) {
	case DisplayServerEnums::CONTEXT_EDITOR: {
		app_id = "org.godotengine.Editor";
	} break;

	case DisplayServerEnums::CONTEXT_PROJECTMAN: {
		app_id = "org.godotengine.ProjectManager";
	} break;

	case DisplayServerEnums::CONTEXT_ENGINE:
	default: {
		app_id = "org.godotengine.Voltaire";
	}
	}

	return app_id;
}

void DisplayServerWayland::_send_window_event(
	DisplayServerEnums::WindowEvent p_event, DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	if (p_window_id == DisplayServerEnums::MAIN_WINDOW_ID &&
		(p_event == DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST ||
			p_event == DisplayServerEnums::WINDOW_EVENT_FORCE_CLOSE)) {
		extern bool s_quit_requested;
		s_quit_requested = true;
	}
}

void DisplayServerWayland::dispatch_input_events(const InputEvent& p_event)
{
	if (DisplayServerWayland* ds = DisplayServer::driver_as<DisplayServerWayland>()) {
		ds->_dispatch_input_event(p_event);
	}
}

void DisplayServerWayland::_dispatch_input_event(const InputEvent& p_event)
{
	if (p_event.is_from_window()) {
		DisplayServerEnums::WindowID window_id = p_event.window_id;

		if (!popup_menu_list.is_empty() && p_event.type == InputEventType::KEY) {
			window_id = popup_menu_list.back()->get();
		}

		if (windows.has(window_id) && p_event.is_mouse()) {
			mouse_pos = p_event.get_mouse_position() + windows[window_id].rect.position;
		}
	}
}

void DisplayServerWayland::_delete_window(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	ERR_FAIL_COND(!windows.has(wd.root_id));
	WindowData& root_wd = windows[wd.root_id];

	if (hovered_window_id == p_window_id) {
		_hover_window(DisplayServerEnums::INVALID_WINDOW_ID);
	}

	while (!root_wd.popup_stack.is_empty() && root_wd.popup_stack.back()->get() != p_window_id) {
		_send_window_event(
			DisplayServerEnums::WINDOW_EVENT_FORCE_CLOSE, root_wd.popup_stack.back()->get());
	}

	if (root_wd.popup_stack.back() && root_wd.popup_stack.back()->get() == p_window_id) {
		root_wd.popup_stack.pop_back();
	}

	if (popup_menu_list.back() && popup_menu_list.back()->get() == p_window_id) {
		popup_menu_list.pop_back();
	}

	AccessibilityServer::window_destroy(p_window_id);

	if (wd.visible) {
#ifdef VULKAN_ENABLED
		if (rendering_context) {
			rendering_context->window_destroy(p_window_id);
		}
#endif

#ifdef GLES3_ENABLED
		if (egl_manager) {
			egl_manager->window_destroy(p_window_id);
		}
#endif
	}

	if (wd.created) {
		wayland_thread.window_destroy(p_window_id);
	}

	windows.erase(p_window_id);

	DEBUG_LOG_WAYLAND(vformat("Destroyed window %d", p_window_id));
}

void DisplayServerWayland::_update_window_rect(
	const Rect2i& p_rect, DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));

	WindowData& wd = windows[p_window_id];

	if (wd.rect == p_rect) {
		return;
	}

	wd.rect = p_rect;

#ifdef RD_ENABLED
	if (wd.visible && rendering_context) {
		rendering_context->window_set_size(p_window_id, wd.rect.size.width, wd.rect.size.height);
	}
#endif

#ifdef GLES3_ENABLED
	if (wd.visible && egl_manager) {
		wl_egl_window_resize(wd.wl_egl_window, wd.rect.size.width, wd.rect.size.height, 0, 0);
	}
#endif
}

void DisplayServerWayland::_hover_window(DisplayServerEnums::WindowID p_window_id)
{
	if (hovered_window_id == p_window_id) {
		return;
	}

	DisplayServerEnums::WindowID old_hover = hovered_window_id;

	if (old_hover != DisplayServerEnums::INVALID_WINDOW_ID) {
		hovered_window_id = DisplayServerEnums::INVALID_WINDOW_ID;

		DEBUG_LOG_WAYLAND(vformat("[DEBUG] Notifying mouse exit to window %d.", old_hover));
		_send_window_event(DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT, old_hover);
	}

	if (p_window_id != DisplayServerEnums::INVALID_WINDOW_ID && windows.has(p_window_id)) {
		hovered_window_id = p_window_id;

		DEBUG_LOG_WAYLAND(vformat("[DEBUG] Notifying mouse enter to window %d.", p_window_id));
		_send_window_event(DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER, p_window_id);
	}
}

// Interface methods.

bool DisplayServerWayland::has_feature(DisplayServerEnums::Feature p_feature) const
{
	switch (p_feature) {
#ifndef DISABLE_DEPRECATED
	case DisplayServerEnums::FEATURE_GLOBAL_MENU: {
		return NativeMenu::is_initialized() && NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
	} break;
#endif
	case DisplayServerEnums::FEATURE_TOUCHSCREEN:
	case DisplayServerEnums::FEATURE_MOUSE:
	case DisplayServerEnums::FEATURE_MOUSE_WARP:
	case DisplayServerEnums::FEATURE_CLIPBOARD:
	case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
	case DisplayServerEnums::FEATURE_CUSTOM_CURSOR_SHAPE:
	case DisplayServerEnums::FEATURE_WINDOW_TRANSPARENCY:
	case DisplayServerEnums::FEATURE_ICON:
	case DisplayServerEnums::FEATURE_HIDPI:
	case DisplayServerEnums::FEATURE_SWAP_BUFFERS:
	case DisplayServerEnums::FEATURE_KEEP_SCREEN_ON:
	case DisplayServerEnums::FEATURE_IME:
	case DisplayServerEnums::FEATURE_WINDOW_DRAG:
	case DisplayServerEnums::FEATURE_CLIPBOARD_PRIMARY:
	case DisplayServerEnums::FEATURE_SUBWINDOWS:
	case DisplayServerEnums::FEATURE_WINDOW_EMBEDDING:
	case DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS:
	case DisplayServerEnums::FEATURE_HDR_OUTPUT: {
		return true;
	} break;

#ifdef SPEECHD_ENABLED
	case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH: {
		return true;
	} break;
#endif

	case DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER: {
		return AccessibilityServer::is_supported();
	} break;

	default: {
		return false;
	}
	}
}

String DisplayServerWayland::get_name() const { return "Wayland"; }

#ifdef SPEECHD_ENABLED

void DisplayServerWayland::initialize_tts() const
{
	TTS_Linux::initialize();
}

bool DisplayServerWayland::tts_is_speaking() const
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND_V(!TTS_Linux::is_initialized(), false);
	return TTS_Linux::is_speaking();
}

bool DisplayServerWayland::tts_is_paused() const
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND_V(!TTS_Linux::is_initialized(), false);
	return TTS_Linux::is_paused();
}

void DisplayServerWayland::tts_speak(const String& p_text, const String& p_voice, int p_volume,
	float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Linux::is_initialized());
	TTS_Linux::speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServerWayland::tts_pause()
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Linux::is_initialized());
	TTS_Linux::pause();
}

void DisplayServerWayland::tts_resume()
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Linux::is_initialized());
	TTS_Linux::resume();
}

void DisplayServerWayland::tts_stop()
{
	if (unlikely(!TTS_Linux::is_initialized())) {
		initialize_tts();
	}
	ERR_FAIL_COND(!TTS_Linux::is_initialized());
	TTS_Linux::stop();
}

#endif

#ifdef DBUS_ENABLED

bool DisplayServerWayland::is_dark_mode_supported() const
{
	return portal_desktop && portal_desktop->is_supported() &&
		   portal_desktop->is_settings_supported();
}

bool DisplayServerWayland::is_dark_mode() const
{
	if (!is_dark_mode_supported()) {
		return false;
	}
	switch (portal_desktop->get_appearance_color_scheme()) {
	case 1:
		return true;
	case 2:
		return false;
	default:
		return false;
	}
}

Color DisplayServerWayland::get_accent_color() const
{
	if (!portal_desktop) {
		return Color();
	}
	return portal_desktop->get_appearance_accent_color();
}

#endif

void DisplayServerWayland::beep() const { wayland_thread.beep(); }

void DisplayServerWayland::_mouse_update_mode()
{
	DisplayServerEnums::MouseMode wanted_mouse_mode =
		mouse_mode_override_enabled ? mouse_mode_override : mouse_mode_base;

	if (wanted_mouse_mode == mouse_mode) {
		return;
	}

	MutexLock mutex_lock(wayland_thread.mutex);

	bool show_cursor = (wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_VISIBLE ||
						wanted_mouse_mode == DisplayServerEnums::MOUSE_MODE_CONFINED);

	wayland_thread.cursor_set_visible(show_cursor);

	WaylandThread::PointerConstraint constraint = WaylandThread::PointerConstraint::NONE;

	switch (wanted_mouse_mode) {
	case DisplayServerEnums::MOUSE_MODE_CAPTURED: {
		constraint = WaylandThread::PointerConstraint::LOCKED;
	} break;

	case DisplayServerEnums::MOUSE_MODE_CONFINED:
	case DisplayServerEnums::MOUSE_MODE_CONFINED_HIDDEN: {
		constraint = WaylandThread::PointerConstraint::CONFINED;
	} break;

	default: {
	}
	}

	wayland_thread.pointer_set_constraint(constraint);

	mouse_mode = wanted_mouse_mode;
}

void DisplayServerWayland::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_base) {
		return;
	}
	mouse_mode_base = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWayland::mouse_get_mode() const { return mouse_mode; }

void DisplayServerWayland::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, DisplayServerEnums::MouseMode::MOUSE_MODE_MAX);
	if (p_mode == mouse_mode_override) {
		return;
	}
	mouse_mode_override = p_mode;
	_mouse_update_mode();
}

DisplayServerEnums::MouseMode DisplayServerWayland::mouse_get_mode_override() const
{
	return mouse_mode_override;
}

void DisplayServerWayland::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	if (p_override_enabled == mouse_mode_override_enabled) {
		return;
	}
	mouse_mode_override_enabled = p_override_enabled;
	_mouse_update_mode();
}

bool DisplayServerWayland::mouse_is_mode_override_enabled() const
{
	return mouse_mode_override_enabled;
}

void DisplayServerWayland::warp_mouse(const Point2i& p_to)
{
	MutexLock mutex_lock(wayland_thread.mutex);
	wayland_thread.pointer_warp(p_to);
}

Point2i DisplayServerWayland::mouse_get_position() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return mouse_pos;
}

uint32_t DisplayServerWayland::mouse_get_button_state() const
{
	MutexLock mutex_lock(wayland_thread.mutex);
	return (uint32_t)wayland_thread.pointer_get_button_mask();
}

void DisplayServerWayland::clipboard_set(const String& p_text)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.selection_set_text(p_text);
}

String DisplayServerWayland::clipboard_get() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	Vector<uint8_t> data;

	const String text_mimes[] = {
		"text/plain;charset=utf-8",
		"text/plain",
	};

	for (String mime : text_mimes) {
		if (wayland_thread.selection_has_mime(mime)) {
			print_verbose(vformat("Selecting media type \"%s\" from offered types.", mime));
			data = wayland_thread.selection_get_mime(mime);
			break;
		}
	}

	return String::utf8((const char*)data.ptr(), data.size());
}

Ref<Image> DisplayServerWayland::clipboard_get_image() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	Ref<Image> image;
	image.instantiate();

	Error err = OK;

	if (wayland_thread.selection_has_mime("image/png")) {
		err = image->load_png_from_buffer(wayland_thread.selection_get_mime("image/png"));
	}
	else if (wayland_thread.selection_has_mime("image/jpeg")) {
		err = image->load_jpg_from_buffer(wayland_thread.selection_get_mime("image/jpeg"));
	}
	else if (wayland_thread.selection_has_mime("image/webp")) {
		err = image->load_webp_from_buffer(wayland_thread.selection_get_mime("image/webp"));
	}
	else if (wayland_thread.selection_has_mime("image/svg+xml")) {
		err = image->load_svg_from_buffer(wayland_thread.selection_get_mime("image/svg+xml"));
	}
	else if (wayland_thread.selection_has_mime("image/bmp")) {
		err = image->load_bmp_from_buffer(wayland_thread.selection_get_mime("image/bmp"));
	}
	else if (wayland_thread.selection_has_mime("image/x-tga")) {
		err = image->load_tga_from_buffer(wayland_thread.selection_get_mime("image/x-tga"));
	}
	else if (wayland_thread.selection_has_mime("image/x-targa")) {
		err = image->load_tga_from_buffer(wayland_thread.selection_get_mime("image/x-targa"));
	}
	else if (wayland_thread.selection_has_mime("image/ktx")) {
		err = image->load_ktx_from_buffer(wayland_thread.selection_get_mime("image/ktx"));
	}
	else if (wayland_thread.selection_has_mime("image/x-exr")) {
		err = image->load_exr_from_buffer(wayland_thread.selection_get_mime("image/x-exr"));
	}

	ERR_FAIL_COND_V(err != OK, Ref<Image>());

	return image;
}

bool DisplayServerWayland::clipboard_has_image() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.selection_has_mime("image/png") ||
		   wayland_thread.selection_has_mime("image/jpeg") ||
		   wayland_thread.selection_has_mime("image/webp") ||
		   wayland_thread.selection_has_mime("image/svg+xml") ||
		   wayland_thread.selection_has_mime("image/bmp") ||
		   wayland_thread.selection_has_mime("image/x-tga") ||
		   wayland_thread.selection_has_mime("image/x-targa") ||
		   wayland_thread.selection_has_mime("image/ktx") ||
		   wayland_thread.selection_has_mime("image/x-exr");
}

void DisplayServerWayland::clipboard_set_primary(const String& p_text)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.primary_set_text(p_text);
}

String DisplayServerWayland::clipboard_get_primary() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	Vector<uint8_t> data;

	const String text_mimes[] = {
		"text/plain;charset=utf-8",
		"text/plain",
	};

	for (String mime : text_mimes) {
		if (wayland_thread.primary_has_mime(mime)) {
			print_verbose(vformat("Selecting media type \"%s\" from offered types.", mime));
			data = wayland_thread.primary_get_mime(mime);
			break;
		}
	}

	return String::utf8((const char*)data.ptr(), data.size());
}

int DisplayServerWayland::get_screen_count() const
{
	MutexLock mutex_lock(wayland_thread.mutex);
	return wayland_thread.get_screen_count();
}

int DisplayServerWayland::get_primary_screen() const { return 0; }

Point2i DisplayServerWayland::screen_get_position(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Point2i());

	return wayland_thread.screen_get_data(p_screen).position;
}

Size2i DisplayServerWayland::screen_get_size(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Size2i());

	return wayland_thread.screen_get_data(p_screen).size;
}

Rect2i DisplayServerWayland::screen_get_usable_rect(int p_screen) const
{
	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, Rect2i());

	return Rect2i(screen_get_position(p_screen), screen_get_size(p_screen));
}

int DisplayServerWayland::screen_get_dpi(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 96);

	const WaylandThread::ScreenData& data = wayland_thread.screen_get_data(p_screen);

	int width_mm = data.physical_size.width;
	int height_mm = data.physical_size.height;

	double xdpi = (width_mm ? data.size.width / (double)width_mm * 25.4 : 0);
	double ydpi = (height_mm ? data.size.height / (double)height_mm * 25.4 : 0);

	if (xdpi || ydpi) {
		return (xdpi + ydpi) / (xdpi && ydpi ? 2 : 1);
	}

	return 96;
}

float DisplayServerWayland::screen_get_scale(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (p_screen == DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) {
		struct wl_surface* wl_surface =
			wayland_thread.window_get_wl_surface(DisplayServerEnums::MAIN_WINDOW_ID);
		WaylandThread::WindowState* ws = wayland_thread.wl_surface_get_window_state(wl_surface);

		return wayland_thread.window_state_get_scale_factor(ws);
	}

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, 1.0f);

	return wayland_thread.screen_get_data(p_screen).scale;
}

float DisplayServerWayland::screen_get_refresh_rate(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	p_screen = _get_screen_index(p_screen);
	int screen_count = get_screen_count();
	ERR_FAIL_INDEX_V(p_screen, screen_count, SCREEN_REFRESH_RATE_FALLBACK);

	return wayland_thread.screen_get_data(p_screen).refresh_rate;
}

bool DisplayServerWayland::is_touchscreen_available() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.input_has_touch() ||
		   (Input::is_initialized() && Input::is_emulating_touch_from_mouse());
}

void DisplayServerWayland::screen_set_keep_on(bool p_enable)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (screen_is_kept_on() == p_enable) {
		return;
	}

	wayland_thread.window_set_idle_inhibition(DisplayServerEnums::MAIN_WINDOW_ID, p_enable);

#ifdef DBUS_ENABLED
	if (portal_desktop && portal_desktop->is_inhibit_supported()) {
		if (p_enable) {
			DisplayServerEnums::WindowID window_id = DisplayServerEnums::MAIN_WINDOW_ID;
			WaylandThread::WindowState* ws = wayland_thread.wl_surface_get_window_state(
				wayland_thread.window_get_wl_surface(window_id));
			screensaver_inhibited = portal_desktop->inhibit(ws ? ws->exported_handle : String());
		}
		else {
			portal_desktop->uninhibit();
			screensaver_inhibited = false;
		}
	}
	else if (screensaver) {
		if (p_enable) {
			screensaver->inhibit();
		}
		else {
			screensaver->uninhibit();
		}

		screensaver_inhibited = p_enable;
	}
#endif
}

bool DisplayServerWayland::screen_is_kept_on() const
{
#ifdef DBUS_ENABLED
	return wayland_thread.window_get_idle_inhibition(DisplayServerEnums::MAIN_WINDOW_ID) ||
		   screensaver_inhibited;
#else
	return wayland_thread.window_get_idle_inhibition(DisplayServerEnums::MAIN_WINDOW_ID);
#endif
}

Vector<DisplayServerEnums::WindowID> DisplayServerWayland::get_window_list() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	Vector<int> ret;
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData>& E : windows) {
		ret.push_back(E.key);
	}
	return ret;
}

DisplayServerEnums::WindowID DisplayServerWayland::create_sub_window(
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Rect2i& p_rect, bool p_exclusive,
	DisplayServerEnums::WindowID p_transient_parent)
{
	DisplayServerEnums::WindowID id = ++window_id_counter;
	WindowData& wd = windows[id];

	wd.id = id;
	wd.mode = p_mode;
	wd.flags = p_flags;
	wd.vsync_mode = p_vsync_mode;

	if (!AccessibilityServer::window_create(wd.id, nullptr)) {
		if (OS::is_stdout_verbose()) {
			ERR_PRINT("Can't create an accessibility adapter for window, accessibility support "
					  "disabled!");
		}
	}

	wd.rect = p_rect;
	wd.title = "Voltaire";
	wd.parent_id = p_transient_parent;
	return id;
}

void DisplayServerWayland::show_window(DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));

	WindowData& wd = windows[p_window_id];

	if (!wd.created) {
		DEBUG_LOG_WAYLAND(vformat("Showing window %d", p_window_id));
		DisplayServerEnums::WindowMode setup_mode = wd.mode;

		DisplayServerEnums::WindowID root_id = wd.id;
		while (root_id != DisplayServerEnums::INVALID_WINDOW_ID &&
			   window_get_flag(DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT, root_id)) {
			root_id = windows[root_id].parent_id;
		}
		ERR_FAIL_COND(root_id == DisplayServerEnums::INVALID_WINDOW_ID);

		wd.root_id = root_id;

		if (!window_get_flag(DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT, p_window_id)) {
			wd.rect.position = Point2i();

			DEBUG_LOG_WAYLAND(vformat("Creating regular window of size %s", wd.rect.size));
			wayland_thread.window_create(p_window_id, wd.rect.size, wd.parent_id);
			wayland_thread.window_set_min_size(p_window_id, wd.min_size);
			wayland_thread.window_set_max_size(p_window_id, wd.max_size);
			wayland_thread.window_set_app_id(p_window_id, _get_app_id_from_context(context));
			wayland_thread.window_set_borderless(p_window_id,
				window_get_flag(DisplayServerEnums::WINDOW_FLAG_BORDERLESS, p_window_id));
		}
		else {
			DEBUG_LOG_WAYLAND("Making popup window.");

			windows[root_id].popup_stack.push_back(p_window_id);

			if (window_get_flag(DisplayServerEnums::WINDOW_FLAG_POPUP, p_window_id)) {
				popup_menu_list.push_back(p_window_id);
			}

			wayland_thread.window_create_popup(p_window_id, wd.parent_id, wd.rect);
		}

		wd.created = true;

		bool ready = wayland_thread.window_wait_ready(p_window_id, WINDOW_READY_TIMEOUT_MS);
		if (!ready) {
			ERR_PRINT(vformat("Could not create window %d: timeout.", p_window_id));
			_send_window_event(DisplayServerEnums::WINDOW_EVENT_FORCE_CLOSE, p_window_id);
			return;
		}

#ifdef RD_ENABLED
		if (rendering_context) {
			union
			{
#ifdef VULKAN_ENABLED
				RenderingContextDriverVulkanWayland::WindowPlatformData vulkan;
#endif
			} wpd;
#ifdef VULKAN_ENABLED
			if (rendering_driver == "vulkan") {
				wpd.vulkan.surface = wayland_thread.window_get_wl_surface(wd.id);
				ERR_FAIL_NULL(wpd.vulkan.surface);
				wpd.vulkan.display = wayland_thread.get_wl_display();
			}
#endif
			Error err = rendering_context->window_create(wd.id, &wpd);
			ERR_FAIL_COND_MSG(err != OK, vformat("Can't create a %s window", rendering_driver));

			rendering_context->window_set_size(wd.id, wd.rect.size.width, wd.rect.size.height);
			window_set_vsync_mode(wd.vsync_mode, p_window_id);
		}
#endif

#ifdef GLES3_ENABLED
		if (egl_manager) {
			struct wl_surface* wl_surface = wayland_thread.window_get_wl_surface(wd.id);
			ERR_FAIL_NULL(wl_surface);
			wd.wl_egl_window =
				wl_egl_window_create(wl_surface, wd.rect.size.width, wd.rect.size.height);

			Error err = egl_manager->window_create(p_window_id, wayland_thread.get_wl_display(),
				wd.wl_egl_window, wd.rect.size.width, wd.rect.size.height);
			ERR_FAIL_COND_MSG(err == ERR_CANT_CREATE, "Can't show a GLES3 window.");

			window_set_vsync_mode(wd.vsync_mode, p_window_id);
		}
#endif

		wd.visible = true;
		window_set_mode(setup_mode, wd.id);
		wayland_thread.window_set_title(p_window_id, wd.title);
	}
}

void DisplayServerWayland::delete_sub_window(DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_MSG(
		p_window_id == DisplayServerEnums::MAIN_WINDOW_ID, "Main window can't be deleted");

	_delete_window(p_window_id);
}

DisplayServerEnums::WindowID DisplayServerWayland::window_get_active_popup() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (!popup_menu_list.is_empty()) {
		return popup_menu_list.back()->get();
	}

	return DisplayServerEnums::INVALID_WINDOW_ID;
}

void DisplayServerWayland::window_set_popup_safe_rect(
	DisplayServerEnums::WindowID p_window, const Rect2i& p_rect)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window));

	windows[p_window].safe_rect = p_rect;
}

Rect2i DisplayServerWayland::window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window), Rect2i());

	return windows[p_window].safe_rect;
}

int64_t DisplayServerWayland::window_get_native_handle(
	DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	switch (p_handle_type) {
	case DisplayServerEnums::DISPLAY_HANDLE: {
		return (int64_t)wayland_thread.get_wl_display();
	} break;

	case DisplayServerEnums::WINDOW_HANDLE: {
		return (int64_t)wayland_thread.window_get_wl_surface(p_window);
	} break;

	case DisplayServerEnums::WINDOW_VIEW: {
		return 0;
	} break;

#ifdef GLES3_ENABLED
	case DisplayServerEnums::OPENGL_CONTEXT: {
		if (egl_manager) {
			return (int64_t)egl_manager->get_context(p_window);
		}
		return 0;
	} break;
	case DisplayServerEnums::EGL_DISPLAY: {
		if (egl_manager) {
			return (int64_t)egl_manager->get_display(p_window);
		}
		return 0;
	}
	case DisplayServerEnums::EGL_CONFIG: {
		if (egl_manager) {
			return (int64_t)egl_manager->get_config(p_window);
		}
		return 0;
	}
#endif

	default: {
		return 0;
	} break;
	}
}

DisplayServerEnums::WindowID DisplayServerWayland::get_window_at_screen_position(
	const Point2i& p_position) const
{
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

void DisplayServerWayland::window_set_title(
	const String& p_title, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));

	WindowData& wd = windows[p_window_id];

	wd.title = p_title;

	if (wd.created) {
		wayland_thread.window_set_title(p_window_id, wd.title);
	}
}

void DisplayServerWayland::window_set_mouse_passthrough(
	const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window_id)
{
}

int DisplayServerWayland::window_get_current_screen(DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), DisplayServerEnums::INVALID_SCREEN);
	return 0;
}

void DisplayServerWayland::window_set_current_screen(
	int p_screen, DisplayServerEnums::WindowID p_window_id)
{
}

Point2i DisplayServerWayland::window_get_position(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return windows[p_window_id].rect.position;
}

Point2i DisplayServerWayland::window_get_position_with_decorations(
	DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return windows[p_window_id].rect.position;
}

void DisplayServerWayland::window_set_position(
	const Point2i& p_position, DisplayServerEnums::WindowID p_window_id)
{
}

void DisplayServerWayland::window_set_max_size(
	const Size2i p_size, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	DEBUG_LOG_WAYLAND(vformat("window max size set to %s", p_size));

	if (p_size.x < 0 || p_size.y < 0) {
		ERR_FAIL_MSG("Maximum window size can't be negative!");
	}

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	if ((p_size != Size2i()) && ((p_size.x < wd.min_size.x) || (p_size.y < wd.min_size.y))) {
		ERR_PRINT("Maximum window size can't be smaller than minimum window size!");
		return;
	}

	wd.max_size = p_size;

	if (wd.created) {
		wayland_thread.window_set_max_size(p_window_id, p_size);
	}
}

Size2i DisplayServerWayland::window_get_max_size(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), Size2i());
	return windows[p_window_id].max_size;
}

void DisplayServerWayland::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
#ifdef GLES3_ENABLED
	if (egl_manager) {
		egl_manager->window_make_current(p_window_id);
	}
#endif
}

void DisplayServerWayland::window_set_transient(
	DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowID p_parent)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	ERR_FAIL_COND(wd.parent_id == p_parent);

	if (p_parent != DisplayServerEnums::INVALID_WINDOW_ID) {
		ERR_FAIL_COND(!windows.has(p_parent));
		ERR_FAIL_COND_MSG(wd.parent_id != DisplayServerEnums::INVALID_WINDOW_ID,
			"Window already has a transient parent");
		wd.parent_id = p_parent;

		if (wd.created) {
			wayland_thread.window_set_parent(p_window_id, p_parent);
		}
	}
}

void DisplayServerWayland::window_set_min_size(
	const Size2i p_size, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	DEBUG_LOG_WAYLAND(vformat("window minsize set to %s", p_size));

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	if (p_size.x < 0 || p_size.y < 0) {
		ERR_FAIL_MSG("Minimum window size can't be negative!");
	}

	if ((p_size != Size2i()) && (wd.max_size != Size2i()) &&
		((p_size.x > wd.max_size.x) || (p_size.y > wd.max_size.y))) {
		ERR_PRINT("Minimum window size can't be larger than maximum window size!");
		return;
	}

	wd.min_size = p_size;

	if (wd.created) {
		wayland_thread.window_set_min_size(p_window_id, p_size);
	}
}

Size2i DisplayServerWayland::window_get_min_size(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), Size2i());
	return windows[p_window_id].min_size;
}

void DisplayServerWayland::window_set_size(
	const Size2i p_size, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	if (wd.rect.size == p_size) {
		return;
	}

	Size2i new_size = p_size.maxi(1);

	if (wd.created) {
		new_size = wayland_thread.window_set_size(p_window_id, p_size);
	}

	_update_window_rect(Rect2i(wd.rect.position, new_size), p_window_id);
}

Size2i DisplayServerWayland::window_get_size(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), Size2i());
	return windows[p_window_id].rect.size;
}

Size2i DisplayServerWayland::window_get_size_with_decorations(
	DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), Size2i());
	return windows[p_window_id].rect.size;
}

float DisplayServerWayland::window_get_scale(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	const WaylandThread::WindowState* ws = wayland_thread.window_get_state(p_window_id);
	ERR_FAIL_NULL_V(ws, 1);

	return wayland_thread.window_state_get_scale_factor(ws);
}

void DisplayServerWayland::window_set_mode(
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	if (!wd.created) {
		return;
	}

	wayland_thread.window_try_set_mode(p_window_id, p_mode);
}

void DisplayServerWayland::window_set_icon(
	const Ref<Image>& p_icon, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));
	wayland_thread.set_icon(p_icon, p_window_id);
}

DisplayServerEnums::WindowMode DisplayServerWayland::window_get_mode(
	DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	const WindowData& wd = windows[p_window_id];

	if (!wd.created) {
		return DisplayServerEnums::WINDOW_MODE_WINDOWED;
	}

	return wayland_thread.window_get_mode(p_window_id);
}

bool DisplayServerWayland::window_is_maximize_allowed(
	DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.window_can_set_mode(
		p_window_id, DisplayServerEnums::WINDOW_MODE_MAXIMIZED);
}

void DisplayServerWayland::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled,
	DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	DEBUG_LOG_WAYLAND(vformat("Window set flag %d", p_flag));

	switch (p_flag) {
	case DisplayServerEnums::WINDOW_FLAG_BORDERLESS: {
		wayland_thread.window_set_borderless(p_window_id, p_enabled);
	} break;

	case DisplayServerEnums::WINDOW_FLAG_POPUP: {
		ERR_FAIL_COND_MSG(
			p_window_id == DisplayServerEnums::MAIN_WINDOW_ID, "Main window can't be popup.");
		ERR_FAIL_COND_MSG(
			wd.created && (wd.flags & DisplayServerEnums::WINDOW_FLAG_POPUP_BIT) != p_enabled,
			"Popup flag can't changed while window is opened.");
	} break;

	case DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT: {
		ERR_FAIL_COND_MSG(p_window_id == DisplayServerEnums::MAIN_WINDOW_ID,
			"Main window can't have popup hint.");
		ERR_FAIL_COND_MSG(
			wd.created &&
				(wd.flags & DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT_BIT) != p_enabled,
			"Popup hint can't changed while window is opened.");
	} break;

	case DisplayServerEnums::WINDOW_FLAG_MOUSE_PASSTHROUGH:
	default: {
	}
	}

	if (p_enabled) {
		wd.flags |= 1 << p_flag;
	}
	else {
		wd.flags &= ~(1 << p_flag);
	}
}

bool DisplayServerWayland::window_get_flag(
	DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	return windows[p_window_id].flags & (1 << p_flag);
}

void DisplayServerWayland::window_request_attention(DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	DEBUG_LOG_WAYLAND("Requested attention.");

	wayland_thread.window_request_attention(p_window_id);
}

void DisplayServerWayland::window_move_to_foreground(DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);
	wayland_thread.window_request_attention(p_window_id);
}

bool DisplayServerWayland::window_is_focused(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.pointer_get_pointed_window_id() == p_window_id;
}

bool DisplayServerWayland::window_can_draw(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (!wayland_thread.window_is_ready(p_window_id)) {
		return false;
	}

	uint64_t last_frame_time = wayland_thread.window_get_last_frame_time(p_window_id);
	if (last_frame_time == 0) {
		return true;
	}

	uint64_t time_since_frame = OS::get_ticks_usec() - last_frame_time;

	if (time_since_frame > WAYLAND_MAX_FRAME_TIME_US) {
		return false;
	}

	if (wayland_thread.window_is_suspended(p_window_id)) {
		return false;
	}

	return suspend_state == SuspendState::NONE;
}

bool DisplayServerWayland::can_any_window_draw() const
{
	for (const KeyValue<DisplayServerEnums::WindowID, WindowData>& E : windows) {
		if (window_can_draw(E.key)) {
			return true;
		}
	}
	return false;
}

void DisplayServerWayland::window_set_ime_active(
	const bool p_active, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.window_set_ime_active(p_active, p_window_id);
}

void DisplayServerWayland::window_set_ime_position(
	const Point2i& p_pos, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.window_set_ime_position(p_pos, p_window_id);
}

int DisplayServerWayland::accessibility_should_increase_contrast() const
{
#ifdef DBUS_ENABLED
	if (!portal_desktop) {
		return -1;
	}
	return portal_desktop->get_high_contrast();
#endif
	return -1;
}

int DisplayServerWayland::accessibility_screen_reader_active() const
{
#ifdef DBUS_ENABLED
	if (atspi_monitor && atspi_monitor->is_supported()) {
		return atspi_monitor->is_active();
	}
#endif
	return -1;
}

Point2i DisplayServerWayland::ime_get_selection() const { return ime_selection; }

String DisplayServerWayland::ime_get_text() const { return ime_text; }

void DisplayServerWayland::window_set_vsync_mode(
	DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window_id)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	WindowData& wd = windows[p_window_id];

#ifdef RD_ENABLED
	if (rendering_context) {
		rendering_context->window_set_vsync_mode(p_window_id, p_vsync_mode);

		wd.emulate_vsync = (!wayland_thread.is_fifo_available() &&
							rendering_context->window_get_vsync_mode(p_window_id) ==
								DisplayServerEnums::VSYNC_ENABLED);

		if (wd.emulate_vsync) {
			print_verbose("VSYNC: manually throttling frames using MAILBOX.");
			rendering_context->window_set_vsync_mode(
				p_window_id, DisplayServerEnums::VSYNC_MAILBOX);
		}
	}
#endif

#ifdef GLES3_ENABLED
	if (egl_manager) {
		egl_manager->set_use_vsync(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);

		wd.emulate_vsync = egl_manager->is_using_vsync();

		if (wd.emulate_vsync) {
			print_verbose("VSYNC: manually throttling frames with swap delay 0.");
			egl_manager->set_use_vsync(false);
		}
	}
#endif
}

DisplayServerEnums::VSyncMode DisplayServerWayland::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window_id) const
{
	const WindowData& wd = windows[p_window_id];
	if (wd.emulate_vsync) {
		return DisplayServerEnums::VSYNC_ENABLED;
	}

#ifdef VULKAN_ENABLED
	if (rendering_context) {
		return rendering_context->window_get_vsync_mode(p_window_id);
	}
#endif

#ifdef GLES3_ENABLED
	if (egl_manager) {
		return egl_manager->is_using_vsync() ? DisplayServerEnums::VSYNC_ENABLED
											 : DisplayServerEnums::VSYNC_DISABLED;
	}
#endif

	return DisplayServerEnums::VSYNC_ENABLED;
}

void DisplayServerWayland::window_start_drag(DisplayServerEnums::WindowID p_window)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.window_start_drag(p_window);
}

void DisplayServerWayland::window_start_resize(
	DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ERR_FAIL_INDEX(int(p_edge), DisplayServerEnums::WINDOW_EDGE_MAX);
	wayland_thread.window_start_resize(p_edge, p_window);
}

void DisplayServerWayland::_window_update_hdr_state(WindowData& p_window)
{
#if defined(RD_ENABLED)
	if (rendering_context) {
		DisplayServerEnums::WindowID window_id = p_window.id;
		bool hdr_preferred = p_window.color_profile.target_max_luminance >
							 p_window.color_profile.reference_luminance;
		bool hdr_desired = wayland_thread.supports_hdr() && hdr_preferred && p_window.hdr_requested;

		if (rendering_context->window_get_hdr_output_enabled(window_id) != hdr_desired) {
			rendering_context->window_set_hdr_output_enabled(window_id, hdr_desired);
			_send_window_event(
				DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED, window_id);
		}

		if (hdr_desired) {
			rendering_context->window_set_hdr_output_max_luminance(
				window_id, p_window.color_profile.target_max_luminance);
			rendering_context->window_set_hdr_output_reference_luminance(
				window_id, p_window.color_profile.reference_luminance);
			rendering_context->window_set_hdr_output_linear_luminance_scale(
				window_id, p_window.color_profile.target_max_luminance);

			p_window.color_profile.named_primary = WP_COLOR_MANAGER_V1_PRIMARIES_SRGB;
			p_window.color_profile.named_transfer_function =
				WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_EXT_LINEAR;
		}
		else {
			p_window.color_profile.named_primary = WP_COLOR_MANAGER_V1_PRIMARIES_SRGB;
			p_window.color_profile.named_transfer_function =
				WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_GAMMA22;
		}

		if (p_window.visible) {
			MutexLock mutex_lock(wayland_thread.mutex);
			wayland_thread.window_set_color_profile(window_id, p_window.color_profile);
		}
	}
#endif
}

bool DisplayServerWayland::window_is_hdr_output_supported(
	DisplayServerEnums::WindowID p_window_id) const
{
	return false;
}

void DisplayServerWayland::window_request_hdr_output(
	const bool p_enabled, DisplayServerEnums::WindowID p_window_id)
{
}

bool DisplayServerWayland::window_is_hdr_output_requested(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	const WindowData& wd = windows[p_window_id];
	return wd.hdr_requested;
}

bool DisplayServerWayland::window_is_hdr_output_enabled(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_enabled(p_window_id);
	}
#endif
	return false;
}

void DisplayServerWayland::window_set_hdr_output_reference_luminance(
	const float p_reference_luminance, DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	if (p_reference_luminance >= 0.0f) {
		ERR_PRINT_ONCE("Manually setting reference white luminance is not supported on Linux "
					   "devices, as they provide a user-facing brightness setting that directly "
					   "controls reference white luminance.");
	}
}

float DisplayServerWayland::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window_id) const
{
	return -1.0;
}

float DisplayServerWayland::window_get_hdr_output_current_reference_luminance(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), 0.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_reference_luminance(p_window_id);
	}
#endif
	return 0.0f;
}

void DisplayServerWayland::window_set_hdr_output_max_luminance(
	const float p_max_luminance, DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	if (p_max_luminance >= 0.0f) {
		ERR_PRINT_ONCE("Manually setting max luminance is not supported on Linux devices as they "
					   "provide a built-in method of calibrating max luminance without the need "
					   "for additional apps or tools.");
	}
}

float DisplayServerWayland::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window_id) const
{
	return -1.0;
}

float DisplayServerWayland::window_get_hdr_output_current_max_luminance(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), 0.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_hdr_output_max_luminance(p_window_id);
	}
#endif
	return 0.0f;
}

float DisplayServerWayland::window_get_output_max_linear_value(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), 1.0);
#if defined(RD_ENABLED)
	if (rendering_context) {
		return rendering_context->window_get_output_max_linear_value(p_window_id);
	}
#endif

	return 1.0f;
}

void DisplayServerWayland::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	ERR_FAIL_INDEX(p_shape, DisplayServerEnums::CURSOR_MAX);

	MutexLock mutex_lock(wayland_thread.mutex);

	if (p_shape == cursor_shape) {
		return;
	}

	cursor_shape = p_shape;

	if (mouse_mode != DisplayServerEnums::MOUSE_MODE_VISIBLE &&
		mouse_mode != DisplayServerEnums::MOUSE_MODE_CONFINED) {
		return;
	}

	wayland_thread.cursor_set_shape(p_shape);
}

DisplayServerEnums::CursorShape DisplayServerWayland::cursor_get_shape() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return cursor_shape;
}

void DisplayServerWayland::cursor_set_custom_image(const Ref<Resource>& p_cursor,
	DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (p_cursor.is_valid()) {
		HashMap<DisplayServerEnums::CursorShape, CustomCursor>::Iterator cursor_c =
			custom_cursors.find(p_shape);

		if (cursor_c) {
			if (cursor_c->value.resource == p_cursor && cursor_c->value.hotspot == p_hotspot) {
				wayland_thread.cursor_set_shape(p_shape);
				return;
			}

			custom_cursors.erase(p_shape);
			wayland_thread.cursor_shape_clear_custom_image(p_shape);
		}

		Ref<Image> image = _get_cursor_image_from_resource(p_cursor, p_hotspot);
		ERR_FAIL_COND(image.is_null());

		CustomCursor& cursor = custom_cursors[p_shape];

		cursor.resource = p_cursor;
		cursor.hotspot = p_hotspot;

		wayland_thread.cursor_shape_set_custom_image(p_shape, image, p_hotspot);

		wayland_thread.cursor_set_shape(p_shape);
	}
	else {
		wayland_thread.cursor_shape_clear_custom_image(p_shape);

		if (cursor_shape == p_shape) {
			wayland_thread.cursor_set_shape(p_shape);
		}

		if (custom_cursors.has(p_shape)) {
			custom_cursors.erase(p_shape);
		}
	}
}

bool DisplayServerWayland::get_swap_cancel_ok() { return swap_cancel_ok; }

Error DisplayServerWayland::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid,
	const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	struct godot_embedding_compositor* ec = wayland_thread.get_embedding_compositor();
	ERR_FAIL_NULL_V_MSG(ec, ERR_BUG, "Missing embedded compositor interface");

	struct WaylandThread::EmbeddingCompositorState* ecs =
		WaylandThread::godot_embedding_compositor_get_state(ec);
	ERR_FAIL_NULL_V(ecs, ERR_BUG);

	if (!ecs->mapped_clients.has(p_pid)) {
		return ERR_DOES_NOT_EXIST;
	}

	struct godot_embedded_client* embedded_client = ecs->mapped_clients[p_pid];
	WaylandThread::EmbeddedClientState* client_data =
		(WaylandThread::EmbeddedClientState*)godot_embedded_client_get_user_data(embedded_client);
	ERR_FAIL_NULL_V(client_data, ERR_BUG);

	if (p_grab_focus) {
		godot_embedded_client_focus_window(embedded_client);
	}

	if (p_visible) {
		WaylandThread::WindowState* ws = wayland_thread.window_get_state(p_window);
		ERR_FAIL_NULL_V(ws, ERR_BUG);

		struct xdg_toplevel* toplevel = ws->xdg_toplevel;
#ifdef LIBDECOR_ENABLED
		if (toplevel == nullptr && ws->libdecor_frame) {
			toplevel = libdecor_frame_get_xdg_toplevel(ws->libdecor_frame);
		}
#endif

		ERR_FAIL_NULL_V(toplevel, ERR_CANT_CREATE);

		godot_embedded_client_set_embedded_window_parent(embedded_client, toplevel);

		double window_scale = WaylandThread::window_state_get_scale_factor(ws);

		Rect2i scaled_rect = p_rect;
		scaled_rect.position =
			WaylandThread::scale_vector2i(scaled_rect.position, 1 / window_scale);
		scaled_rect.size = WaylandThread::scale_vector2i(scaled_rect.size, 1 / window_scale);

		print_verbose(vformat(
			"Scaling embedded rect down by %f from %s to %s.", window_scale, p_rect, scaled_rect));

		godot_embedded_client_set_embedded_window_rect(embedded_client, scaled_rect.position.x,
			scaled_rect.position.y, scaled_rect.size.width, scaled_rect.size.height);
	}
	else {
		godot_embedded_client_set_embedded_window_parent(embedded_client, nullptr);
	}

	return OK;
}

Error DisplayServerWayland::request_close_embedded_process(ProcessID p_pid)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	struct godot_embedding_compositor* ec = wayland_thread.get_embedding_compositor();
	ERR_FAIL_NULL_V_MSG(ec, ERR_BUG, "Missing embedded compositor interface");

	struct WaylandThread::EmbeddingCompositorState* ecs =
		WaylandThread::godot_embedding_compositor_get_state(ec);
	ERR_FAIL_NULL_V(ecs, ERR_BUG);

	if (!ecs->mapped_clients.has(p_pid)) {
		return ERR_DOES_NOT_EXIST;
	}

	struct godot_embedded_client* embedded_client = ecs->mapped_clients[p_pid];
	WaylandThread::EmbeddedClientState* client_data =
		(WaylandThread::EmbeddedClientState*)godot_embedded_client_get_user_data(embedded_client);
	ERR_FAIL_NULL_V(client_data, ERR_BUG);

	godot_embedded_client_embedded_window_request_close(embedded_client);
	return OK;
}

Error DisplayServerWayland::remove_embedded_process(ProcessID p_pid)
{
	return request_close_embedded_process(p_pid);
}

ProcessID DisplayServerWayland::get_focused_process_id()
{
	MutexLock mutex_lock(wayland_thread.mutex);

	ProcessID embedded_pid = wayland_thread.embedded_compositor_get_focused_pid();

	if (embedded_pid < 0) {
		return OS::get_process_id();
	}

	return embedded_pid;
}

int DisplayServerWayland::keyboard_get_layout_count() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.keyboard_get_layout_count();
}

int DisplayServerWayland::keyboard_get_current_layout() const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.keyboard_get_current_layout_index();
}

void DisplayServerWayland::keyboard_set_current_layout(int p_index)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.keyboard_set_current_layout_index(p_index);
}

String DisplayServerWayland::keyboard_get_layout_language(int p_index) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.keyboard_get_layout_name(p_index);
}

String DisplayServerWayland::keyboard_get_layout_name(int p_index) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.keyboard_get_layout_name(p_index);
}

Key DisplayServerWayland::keyboard_get_keycode_from_physical(Key p_keycode) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	Key key = wayland_thread.keyboard_get_key_from_physical(p_keycode);

	if (key == Key::NONE) {
		return p_keycode;
	}

	if (key >= Key::A + 32 && key <= Key::Z + 32) {
		key -= 'a' - 'A';
	}

	if (key == Key::BACKTAB) {
		key = Key::TAB;
	}

	return key;
}

Key DisplayServerWayland::keyboard_get_label_from_physical(Key p_keycode) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.keyboard_get_label_from_physical(p_keycode);
}

void DisplayServerWayland::try_suspend()
{
	bool frame = wayland_thread.wait_frame_suspend_ms(WAYLAND_MAX_FRAME_TIME_US / 1000);
	if (!frame) {
		suspend_state = SuspendState::TIMEOUT;
	}
}

void DisplayServerWayland::process_events()
{
	wayland_thread.mutex.lock();

	wayland_thread.main_loop_callback();

	while (wayland_thread.has_message()) {
		Ref<WaylandThread::Message> msg = wayland_thread.pop_message();
		Ref<WaylandThread::WindowMessage> win_msg = msg;
		if (win_msg.is_valid()) {
			ERR_CONTINUE_MSG(win_msg->id == DisplayServerEnums::INVALID_WINDOW_ID,
				"Invalid window ID received from Wayland thread.");

			if (!windows.has(win_msg->id)) {
				continue;
			}
		}

		Ref<WaylandThread::WindowRectMessage> winrect_msg = msg;
		if (winrect_msg.is_valid()) {
			int scale = winrect_msg->buffer_scale;

			WaylandThread::WindowState* ws = wayland_thread.window_get_state(winrect_msg->id);
			ERR_CONTINUE(ws == nullptr);

			if (scale == 0) {
				ERR_PRINT("Wayland Thread did not report buffer scale at the time of resize.");
				scale = wayland_thread.window_state_get_preferred_buffer_scale(ws);
			}

			wayland_thread.window_state_set_buffer_scale(ws, scale);
			_update_window_rect(winrect_msg->rect, winrect_msg->id);
			continue;
		}

		Ref<WaylandThread::WindowEventMessage> winev_msg = msg;
		if (winev_msg.is_valid() && windows.has(winev_msg->id)) {
			_send_window_event(winev_msg->event, winev_msg->id);

			if (winev_msg->event == DisplayServerEnums::WINDOW_EVENT_FOCUS_IN) {
				AccessibilityServer::set_window_focused(winev_msg->id, true);
			}
			else if (winev_msg->event == DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT) {
				AccessibilityServer::set_window_focused(winev_msg->id, false);
				Input::release_pressed_events();
			}
			continue;
		}

		Ref<WaylandThread::InputEventMessage> inputev_msg = msg;
		if (inputev_msg.is_valid()) {
			const InputEvent& ev = inputev_msg->event;
			bool process_popups = false;
			Point2 position;
			DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;

			if (ev.type == InputEventType::MOUSE_BUTTON) {
				process_popups = (ev.is_pressed() &&
								  ev.mouse_button.button_mask != last_mouse_monitor_mask);
				position = ev.mouse_button.position;
				window_id = ev.window_id;

				last_mouse_monitor_mask = ev.mouse_button.button_mask;
			}
			else if (ev.type == InputEventType::SCREEN_TOUCH) {
				process_popups = ev.is_pressed() && (ev.is_pressed() != last_touch_monitor_pressed);
				position = ev.touch.position;
				window_id = ev.window_id;

				last_touch_monitor_pressed = ev.is_pressed();
			}

			bool handled = false;

			if (!popup_menu_list.is_empty() && process_popups) {
				List<DisplayServerEnums::WindowID>::Element* E = popup_menu_list.back();
				List<DisplayServerEnums::WindowID>::Element* C = nullptr;

				Point2 global_pos = position + window_get_position(window_id);

				while (E) {
					WindowData& wd = windows[E->get()];
					if (wd.rect.has_point(global_pos)) {
						break;
					}
					else if (wd.safe_rect.has_point(global_pos)) {
						break;
					}

					C = E;
					E = E->prev();
				}

				if (C) {
					handled = true;
					_send_window_event(DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST, C->get());
				}
			}

			if (!handled) {
				Input::parse_input_event(ev);
			}

			continue;
		}

		Ref<WaylandThread::DropFilesEventMessage> dropfiles_msg = msg;
		if (dropfiles_msg.is_valid()) {
			continue;
		}

		Ref<WaylandThread::IMECommitEventMessage> ime_commit_msg = msg;
		if (ime_commit_msg.is_valid()) {
			for (int i = 0; i < ime_commit_msg->text.length(); i++) {
				const char32_t codepoint = ime_commit_msg->text[i];

				InputEvent ke = InputEvent::make_key();
				ke.window_id = ime_commit_msg->id;
				ke.pressed = true;
				ke.key.echo = false;
				ke.key.keycode = Key::NONE;
				ke.key.physical_keycode = Key::NONE;
				ke.key.key_label = Key::NONE;
				ke.key.unicode = codepoint;

				Input::parse_input_event(ke);
			}
			ime_text = String();
			ime_selection = Vector2i();
			continue;
		}

		Ref<WaylandThread::IMEUpdateEventMessage> ime_update_msg = msg;
		if (ime_update_msg.is_valid()) {
			if (ime_text != ime_update_msg->text || ime_selection != ime_update_msg->selection) {
				ime_text = ime_update_msg->text;
				ime_selection = ime_update_msg->selection;
			}
			continue;
		}

		Ref<WaylandThread::ColorProfileMessage> color_profile_msg = msg;
		if (color_profile_msg.is_valid()) {
			WindowData& wd = windows[color_profile_msg->id];
			wd.color_profile = color_profile_msg->color_profile;

			_window_update_hdr_state(wd);
			_send_window_event(
				DisplayServerEnums::WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED, wd.id);
			continue;
		}
	}

	switch (suspend_state) {
	case SuspendState::NONE: {
		bool emulate_vsync = false;
		for (KeyValue<DisplayServerEnums::WindowID, WindowData>& pair : windows) {
			if (pair.value.emulate_vsync) {
				emulate_vsync = true;
				break;
			}
		}

		if (emulate_vsync) {
			if (OS::is_in_low_processor_usage_mode()) {
				if (!Renderer::has_changed()) {
					wayland_thread.commit_surfaces();
					try_suspend();
				}
			}
			else {
				try_suspend();
			}
		}

		if (wayland_thread.is_suspended()) {
			suspend_state = SuspendState::CAPABILITY;
		}

		if (suspend_state == SuspendState::TIMEOUT) {
			DEBUG_LOG_WAYLAND("Suspending. Reason: timeout.");
		}
		else if (suspend_state == SuspendState::CAPABILITY) {
			DEBUG_LOG_WAYLAND("Suspending. Reason: capability.");
		}
	} break;

	case SuspendState::TIMEOUT: {
		if (wayland_thread.get_reset_frame()) {
			suspend_state = SuspendState::NONE;
			DEBUG_LOG_WAYLAND("Unsuspending from timeout.");
		}

		wayland_thread.commit_surfaces();
	} break;

	case SuspendState::CAPABILITY: {
		if (!wayland_thread.is_suspended()) {
			suspend_state = SuspendState::NONE;
			DEBUG_LOG_WAYLAND("Unsuspending from capability.");
		}
	} break;
	}

	wayland_thread.mutex.unlock();

	Input::flush_buffered_events();
}

void DisplayServerWayland::release_rendering_thread()
{
#ifdef GLES3_ENABLED
	if (egl_manager) {
		egl_manager->release_current();
	}
#endif
}

void DisplayServerWayland::swap_buffers()
{
#ifdef GLES3_ENABLED
	if (egl_manager) {
		egl_manager->swap_buffers();
	}
#endif
}

void DisplayServerWayland::set_icon(const Ref<Image>& p_icon)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	wayland_thread.set_default_icon(p_icon);
}

void DisplayServerWayland::set_context(DisplayServerEnums::Context p_context)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	DEBUG_LOG_WAYLAND(vformat("Setting context %d.", p_context));

	context = p_context;

	String app_id = _get_app_id_from_context(p_context);
	wayland_thread.window_set_app_id(DisplayServerEnums::MAIN_WINDOW_ID, app_id);
}

bool DisplayServerWayland::is_window_transparency_available() const
{
#if defined(RD_ENABLED)
	if (rendering_device && !rendering_device->is_composite_alpha_supported()) {
		return false;
	}
#endif
	return OS::is_layered_allowed();
}

Vector<String> DisplayServerWayland::get_rendering_drivers_func()
{
	Vector<String> drivers;

#ifdef VULKAN_ENABLED
	drivers.push_back("vulkan");
#endif

#ifdef GLES3_ENABLED
	drivers.push_back("opengl3");
	drivers.push_back("opengl3_es");
#endif
	drivers.push_back("dummy");

	return drivers;
}

DisplayServerDriver* DisplayServerWayland::create_func(const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Point2i* p_position, const Size2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	// Reject native Wayland and bail early
	// WARN_PRINT("Native Wayland support is currently disabled. Please run through X11/Xwayland via "
	// 		   "'--display-driver x11'.");
	r_error = ERR_UNAVAILABLE;
	return nullptr;
}

DisplayServerWayland::DisplayServerWayland(const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i& p_resolution, DisplayServerEnums::Context p_context,
	int64_t p_parent_window, Error& r_error)
{
#if defined(GLES3_ENABLED) || defined(DBUS_ENABLED)
#ifdef SOWRAP_ENABLED
#ifdef DEBUG_ENABLED
	int dylibloader_verbose = 1;
#else
	int dylibloader_verbose = 0;
#endif // DEBUG_ENABLED
#endif // SOWRAP_ENABLED
#endif // defined(GLES3_ENABLED) || defined(DBUS_ENABLED)

	r_error = ERR_UNAVAILABLE;
	context = p_context;

	String current_desk = OS::get_environment("XDG_CURRENT_DESKTOP").to_lower();
	String session_desk = OS::get_environment("XDG_SESSION_DESKTOP").to_lower();
	swap_cancel_ok = (current_desk.contains("kde") || session_desk.contains("kde") ||
					  current_desk.contains("lxqt") || session_desk.contains("lxqt"));

	{
		MutexLock mutex_lock(wayland_thread.mutex);
		Error thread_err = wayland_thread.init();
		if (thread_err != OK) {
			r_error = thread_err;
			ERR_FAIL_MSG("Could not initialize the Wayland thread.");
		}
	}

	Input::set_event_dispatch_function(dispatch_input_events);

	NativeMenu::initialize();

#ifdef SPEECHD_ENABLED
	bool tts_enabled = false;
	if (tts_enabled) {
		initialize_tts();
	}
#endif

	rendering_driver = p_rendering_driver;

	bool driver_found = false;
	String executable_name = OS::get_executable_path().get_file();

	if (rendering_driver == "dummy") {
		RasterizerDummy::make_current();
		driver_found = true;
	}

#ifdef RD_ENABLED
#ifdef VULKAN_ENABLED
	if (rendering_driver == "vulkan") {
		rendering_context = memnew(RenderingContextDriverVulkanWayland);
	}
#endif // VULKAN_ENABLED

	if (rendering_context) {
		if (rendering_context->initialize() != OK) {
			memdelete(rendering_context);
			rendering_context = nullptr;
			r_error = ERR_CANT_CREATE;

			if (p_rendering_driver == "vulkan") {
				OS::alert(
					vformat("Your video card drivers seem not to support the required Vulkan "
							"version.\n\n"
							"Executable: %s",
						executable_name),
					"Unable to initialize Vulkan video driver");
			}

			ERR_FAIL_MSG(vformat("Could not initialize %s", rendering_driver));
		}

		Error err =
			RenderingDevice::initialize(rendering_context, rendering_context->device_get(0));
		if (err != OK) {
			ERR_PRINT("Failed to initialize RenderingDevice.");
			r_error = ERR_UNAVAILABLE;
			return;
		}

		driver_found = true;
	}
#endif // RD_ENABLED

#ifdef GLES3_ENABLED
	if (rendering_driver == "opengl3" || rendering_driver == "opengl3_es") {
#ifdef SOWRAP_ENABLED
		if (initialize_wayland_egl(dylibloader_verbose) != 0) {
			WARN_PRINT("Can't load the Wayland EGL library.");
			return;
		}
#endif // SOWRAP_ENABLED

		if (getenv("DRI_PRIME") == nullptr) {
			int prime_idx = -1;

			if (getenv("PRIMUS_DISPLAY") || getenv("PRIMUS_libGLd") || getenv("PRIMUS_libGLa") ||
				getenv("PRIMUS_libGL") || getenv("PRIMUS_LOAD_GLOBAL") ||
				getenv("BUMBLEBEE_SOCKET") || getenv("__NV_PRIME_RENDER_OFFLOAD")) {
				print_verbose("Optirun/primusrun detected. Skipping GPU detection");
				prime_idx = 0;
			}

			if (prime_idx == -1 && getenv("LD_LIBRARY_PATH") &&
				!getenv("STEAM_RUNTIME_LIBRARY_PATH")) {
				String ld_library_path(getenv("LD_LIBRARY_PATH"));
				Vector<String> libraries = ld_library_path.split(":");

				for (int i = 0; i < libraries.size(); ++i) {
					if (FileAccess::exists(libraries[i] + "/libGL.so.1") ||
						FileAccess::exists(libraries[i] + "/libGL.so")) {
						print_verbose("Custom libGL override detected. Skipping GPU detection");
						prime_idx = 0;
					}
				}
			}

			if (prime_idx == -1) {
				print_verbose("Detecting GPUs, set DRI_PRIME in the environment to override GPU "
							  "detection logic.");
				prime_idx = DetectPrimeEGL::detect_prime(EGL_PLATFORM_WAYLAND_KHR);
			}

			if (prime_idx) {
				__print_line(
					vformat("Found discrete GPU, setting DRI_PRIME=%d to use it.", prime_idx));
				__print_line("Note: Set DRI_PRIME=0 in the environment to disable Godot from using "
							 "the discrete GPU.");
				setenv("DRI_PRIME", itos(prime_idx).utf8().ptr(), 1);
			}
		}

		if (rendering_driver == "opengl3") {
			egl_manager = memnew(EGLManagerWayland);

			if (egl_manager->initialize(wayland_thread.get_wl_display()) != OK ||
				egl_manager->open_display(wayland_thread.get_wl_display()) != OK) {
				memdelete(egl_manager);
				egl_manager = nullptr;
				r_error = ERR_UNAVAILABLE;

				OS::alert(
					vformat("Your video card drivers seem not to support the required OpenGL "
							"3.3 version.\n\n"
							"Executable: %s",
						executable_name),
					"Unable to initialize OpenGL video driver");

				ERR_FAIL_MSG("Could not initialize OpenGL.");
			}
			else {
				RasterizerGLES3::make_current(true);
				driver_found = true;
			}
		}

		if (rendering_driver == "opengl3_es") {
			egl_manager = memnew(EGLManagerWaylandGLES);

			if (egl_manager->initialize(wayland_thread.get_wl_display()) != OK ||
				egl_manager->open_display(wayland_thread.get_wl_display()) != OK) {
				memdelete(egl_manager);
				egl_manager = nullptr;
				r_error = ERR_CANT_CREATE;

				OS::alert(
					vformat(
						"Your video card drivers seem not to support the required OpenGL ES 3.0 "
						"version.\n\n"
						"Executable: %s",
						executable_name),
					"Unable to initialize OpenGL ES video driver");

				ERR_FAIL_MSG("Could not initialize OpenGL ES.");
			}

			RasterizerGLES3::make_current(false);
			driver_found = true;
		}
	}
#endif // GLES3_ENABLED

	if (!driver_found) {
		r_error = ERR_UNAVAILABLE;
		ERR_FAIL_MSG("Video driver not found.");
	}

	cursor_set_shape(DisplayServerEnums::CURSOR_BUSY);

	WindowData& wd = windows[DisplayServerEnums::MAIN_WINDOW_ID];

	wd.id = DisplayServerEnums::MAIN_WINDOW_ID;
	wd.mode = p_mode;
	wd.flags = p_flags;
	wd.vsync_mode = p_vsync_mode;
	wd.rect.size = p_resolution;
	wd.title = "Voltaire";

	if (!AccessibilityServer::window_create(wd.id, nullptr)) {
		if (OS::is_stdout_verbose()) {
			ERR_PRINT("Can't create an accessibility adapter for window, accessibility support "
					  "disabled!");
		}
	}

	show_window(DisplayServerEnums::MAIN_WINDOW_ID);

	if (!windows.has(DisplayServerEnums::MAIN_WINDOW_ID) ||
		!windows[DisplayServerEnums::MAIN_WINDOW_ID].visible) {
		ERR_PRINT("Could not map the main window.");
		r_error = ERR_CANT_CREATE;
		return;
	}

#ifdef RD_ENABLED
	if (rendering_context) {
		RenderingDevice::screen_create(DisplayServerEnums::MAIN_WINDOW_ID);
		RendererCompositorRD::make_current();
	}
#endif // RD_ENABLED

#ifdef DBUS_ENABLED
	bool dbus_ok = true;
#ifdef SOWRAP_ENABLED
	if (initialize_dbus(dylibloader_verbose) != 0) {
		print_verbose("Failed to load DBus library!");
		dbus_ok = false;
	}
#endif
	if (dbus_ok) {
		bool ver_ok = false;
		int version_major = 0;
		int version_minor = 0;
		int version_rev = 0;
		dbus_get_version(&version_major, &version_minor, &version_rev);
		ver_ok = (version_major == 1 && version_minor >= 10) || (version_major > 1);
		print_verbose(
			vformat("DBus %d.%d.%d detected.", version_major, version_minor, version_rev));
		if (!ver_ok) {
			print_verbose("Unsupported DBus library version!");
			dbus_ok = false;
		}
	}
	if (dbus_ok) {
		screensaver = memnew(FreeDesktopScreenSaver);
		portal_desktop = memnew(FreeDesktopPortalDesktop);
		atspi_monitor = memnew(FreeDesktopAtSPIMonitor);
	}
#endif // DBUS_ENABLED

	screen_set_keep_on(false);

	r_error = OK;
}

DisplayServerWayland::~DisplayServerWayland()
{
	wayland_thread.mutex.lock();

	NativeMenu::finalize();

	List<DisplayServerEnums::WindowID> toplevels;

	for (const KeyValue<DisplayServerEnums::WindowID, WindowData>& pair : windows) {
		DisplayServerEnums::WindowID id = pair.key;

		if (!window_get_flag(DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT, id)) {
			toplevels.push_back(id);
		}
		else {
			AccessibilityServer::window_destroy(id);
		}
	}

	for (DisplayServerEnums::WindowID& id : toplevels) {
		_delete_window(id);
	}
	windows.clear();

	wayland_thread.mutex.unlock();
	wayland_thread.destroy();

#ifdef RD_ENABLED
	if (rendering_context) {
		memdelete(rendering_context);
		rendering_context = nullptr;
	}
#endif

#ifdef SPEECHD_ENABLED
	TTS_Linux::finalize();
#endif

#ifdef DBUS_ENABLED
	memdelete(portal_desktop);
	memdelete(screensaver);
	memdelete(atspi_monitor);
#endif
}

void DisplayServerWayland::register_wayland_driver()
{
	DisplayServer::register_create_function("wayland", create_func, get_rendering_drivers_func);
}

#endif // WAYLAND_ENABLED

#ifdef WAYLAND_DISPLAY_SERVER_DEBUG_LOGS_ENABLED
#undef WAYLAND_DISPLAY_SERVER_DEBUG_LOGS_ENABLED
#endif
#ifdef DEBUG_LOG_WAYLAND
#undef DEBUG_LOG_WAYLAND
#endif
#ifdef WAYLAND_MAX_FRAME_TIME_US
#undef WAYLAND_MAX_FRAME_TIME_US
#endif
#ifdef WINDOW_READY_TIMEOUT_MS
#undef WINDOW_READY_TIMEOUT_MS
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/detect_prime_egl.cpp
// ============================================================================

#ifdef GLES3_ENABLED
#ifdef EGL_ENABLED

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#include "core/core_globals.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"

#ifdef GLAD_ENABLED
#include <platform_gl.h>
#else
#include <GL/glcorearb.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>

// To prevent shadowing warnings.
#undef glGetString

// Runs inside a child. Exiting will not quit the engine.
void DetectPrimeEGL::create_context(EGLenum p_platform_enum) {
#if defined(GLAD_ENABLED)
	if (!gladLoaderLoadEGL(nullptr)) {
		print_verbose("Unable to load EGL, GPU detection skipped.");
		quick_exit(1);
	}
#endif

	EGLDisplay egl_display = EGL_NO_DISPLAY;

	if (GLAD_EGL_VERSION_1_5) {
		egl_display = eglGetPlatformDisplay(p_platform_enum, nullptr, nullptr);
	} else if (GLAD_EGL_EXT_platform_base) {
#ifdef EGL_EXT_platform_base
		egl_display = eglGetPlatformDisplayEXT(p_platform_enum, nullptr, nullptr);
#endif
	} else {
		egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}

	EGLConfig egl_config;
	EGLContext egl_context = EGL_NO_CONTEXT;

	eglInitialize(egl_display, nullptr, nullptr);

#if defined(GLAD_ENABLED)
	if (!gladLoaderLoadEGL(egl_display)) {
		print_verbose("Unable to load EGL, GPU detection skipped.");
		quick_exit(1);
	}
#endif

	eglBindAPI(EGL_OPENGL_API);

	EGLint attribs[] = {
		EGL_RED_SIZE,
		1,
		EGL_BLUE_SIZE,
		1,
		EGL_GREEN_SIZE,
		1,
		EGL_DEPTH_SIZE,
		24,
		EGL_NONE,
	};

	EGLint config_count = 0;
	eglChooseConfig(egl_display, attribs, &egl_config, 1, &config_count);

	EGLint context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_NONE
	};

	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
	if (egl_context == EGL_NO_CONTEXT) {
		print_verbose("Unable to create an EGL context, GPU detection skipped.");
		quick_exit(1);
	}

	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

int DetectPrimeEGL::detect_prime(EGLenum p_platform_enum) {
	pid_t p;
	int priorities[4] = {};
	String vendors[4];
	String renderers[4];

	for (int i = 0; i < 4; ++i) {
		vendors[i] = "Unknown";
		renderers[i] = "Unknown";
	}

	for (int i = 0; i < 4; ++i) {
		int fdset[2];

		if (pipe(fdset) == -1) {
			print_verbose("Failed to pipe(), using default GPU");
			return 0;
		}

		// Fork so the driver initialization can crash without taking down the engine.
		p = fork();

		if (p > 0) {
			// Main thread

			int stat_loc = 0;
			char string[201];
			string[200] = '\0';

			close(fdset[1]);

			waitpid(p, &stat_loc, 0);

			if (!stat_loc) {
				// No need to do anything complicated here. Anything less than
				// PIPE_BUF will be delivered in one read() call.
				// Leave it 'Unknown' otherwise.
				if (read(fdset[0], string, sizeof(string) - 1) > 0) {
					vendors[i] = string;
					renderers[i] = string + strlen(string) + 1;
				}
			}

			close(fdset[0]);
		} else {
			// In child, exit() here will not quit the engine.

			// Prevent false leak reports as we will not be properly
			// cleaning up these processes, and fork() makes a copy
			// of all globals.
			CoreGlobals::leak_reporting_enabled = false;

			char string[201];

			close(fdset[0]);

			setenv("DRI_PRIME", itos(i).utf8().ptr(), 1);

			create_context(p_platform_enum);

			PFNGLGETSTRINGPROC glGetString = (PFNGLGETSTRINGPROC)eglGetProcAddress("glGetString");
			const char *vendor = (const char *)glGetString(GL_VENDOR);
			const char *renderer = (const char *)glGetString(GL_RENDERER);

			unsigned int vendor_len = strlen(vendor) + 1;
			unsigned int renderer_len = strlen(renderer) + 1;

			if (vendor_len + renderer_len >= sizeof(string)) {
				renderer_len = 200 - vendor_len;
			}

			memcpy(&string, vendor, vendor_len);
			memcpy(&string[vendor_len], renderer, renderer_len);

			if (write(fdset[1], string, vendor_len + renderer_len) == -1) {
				print_verbose("Couldn't write vendor/renderer string.");
			}
			close(fdset[1]);

			// The function quick_exit() is used because exit() will call destructors on static objects copied by fork().
			// These objects will be freed anyway when the process finishes execution.
			quick_exit(0);
		}
	}

	int preferred = 0;
	int priority = 0;

	if (vendors[0] == vendors[1]) {
		print_verbose("Only one GPU found, using default.");
		return 0;
	}

	for (int i = 3; i >= 0; --i) {
		const Vendor *v = vendor_map;
		while (v->glxvendor) {
			if (v->glxvendor == vendors[i]) {
				priorities[i] = v->priority;

				if (v->priority >= priority) {
					priority = v->priority;
					preferred = i;
				}
			}
			++v;
		}
	}

	print_verbose("Found renderers:");
	for (int i = 0; i < 4; ++i) {
		print_verbose("Renderer " + itos(i) + ": " + renderers[i] + " with priority: " + itos(priorities[i]));
	}

	print_verbose("Using renderer: " + renderers[preferred]);
	return preferred;
}

#endif // EGL_ENABLED
#endif // GLES3_ENABLED

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/key_mapping_xkb.cpp
// ============================================================================

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#ifdef SOWRAP_ENABLED
#include "xkbcommon-so_wrap.h"
#else
#include <xkbcommon/xkbcommon.h>
#endif // SOWRAP_ENABLED

void KeyMappingXKB::initialize() {
	// XKB keycode to Godot Key map.

	xkb_keycode_map[XKB_KEY_Escape] = Key::ESCAPE;
	xkb_keycode_map[XKB_KEY_Tab] = Key::TAB;
	xkb_keycode_map[XKB_KEY_ISO_Left_Tab] = Key::BACKTAB;
	xkb_keycode_map[XKB_KEY_BackSpace] = Key::BACKSPACE;
	xkb_keycode_map[XKB_KEY_Return] = Key::ENTER;
	xkb_keycode_map[XKB_KEY_Insert] = Key::INSERT;
	xkb_keycode_map[XKB_KEY_Delete] = Key::KEY_DELETE;
	xkb_keycode_map[XKB_KEY_Clear] = Key::KEY_DELETE;
	xkb_keycode_map[XKB_KEY_Pause] = Key::PAUSE;
	xkb_keycode_map[XKB_KEY_Print] = Key::PRINT;
	xkb_keycode_map[XKB_KEY_Home] = Key::HOME;
	xkb_keycode_map[XKB_KEY_End] = Key::END;
	xkb_keycode_map[XKB_KEY_Left] = Key::LEFT;
	xkb_keycode_map[XKB_KEY_Up] = Key::UP;
	xkb_keycode_map[XKB_KEY_Right] = Key::RIGHT;
	xkb_keycode_map[XKB_KEY_Down] = Key::DOWN;
	xkb_keycode_map[XKB_KEY_Prior] = Key::PAGEUP;
	xkb_keycode_map[XKB_KEY_Next] = Key::PAGEDOWN;
	xkb_keycode_map[XKB_KEY_Shift_L] = Key::SHIFT;
	xkb_keycode_map[XKB_KEY_Shift_R] = Key::SHIFT;
	xkb_keycode_map[XKB_KEY_Shift_Lock] = Key::SHIFT;
	xkb_keycode_map[XKB_KEY_Control_L] = Key::CTRL;
	xkb_keycode_map[XKB_KEY_Control_R] = Key::CTRL;
	xkb_keycode_map[XKB_KEY_Meta_L] = Key::META;
	xkb_keycode_map[XKB_KEY_Meta_R] = Key::META;
	xkb_keycode_map[XKB_KEY_Alt_L] = Key::ALT;
	xkb_keycode_map[XKB_KEY_Alt_R] = Key::ALT;
	xkb_keycode_map[XKB_KEY_Caps_Lock] = Key::CAPSLOCK;
	xkb_keycode_map[XKB_KEY_Num_Lock] = Key::NUMLOCK;
	xkb_keycode_map[XKB_KEY_Scroll_Lock] = Key::SCROLLLOCK;
	xkb_keycode_map[XKB_KEY_less] = Key::QUOTELEFT;
	xkb_keycode_map[XKB_KEY_grave] = Key::SECTION;
	xkb_keycode_map[XKB_KEY_Super_L] = Key::META;
	xkb_keycode_map[XKB_KEY_Super_R] = Key::META;
	xkb_keycode_map[XKB_KEY_Menu] = Key::MENU;
	xkb_keycode_map[XKB_KEY_Hyper_L] = Key::HYPER;
	xkb_keycode_map[XKB_KEY_Hyper_R] = Key::HYPER;
	xkb_keycode_map[XKB_KEY_Help] = Key::HELP;
	xkb_keycode_map[XKB_KEY_KP_Space] = Key::SPACE;
	xkb_keycode_map[XKB_KEY_KP_Tab] = Key::TAB;
	xkb_keycode_map[XKB_KEY_KP_Enter] = Key::KP_ENTER;
	xkb_keycode_map[XKB_KEY_Home] = Key::HOME;
	xkb_keycode_map[XKB_KEY_Left] = Key::LEFT;
	xkb_keycode_map[XKB_KEY_Up] = Key::UP;
	xkb_keycode_map[XKB_KEY_Right] = Key::RIGHT;
	xkb_keycode_map[XKB_KEY_Down] = Key::DOWN;
	xkb_keycode_map[XKB_KEY_Prior] = Key::PAGEUP;
	xkb_keycode_map[XKB_KEY_Next] = Key::PAGEDOWN;
	xkb_keycode_map[XKB_KEY_End] = Key::END;
	xkb_keycode_map[XKB_KEY_Begin] = Key::CLEAR;
	xkb_keycode_map[XKB_KEY_Insert] = Key::INSERT;
	xkb_keycode_map[XKB_KEY_Delete] = Key::KEY_DELETE;
	xkb_keycode_map[XKB_KEY_KP_Equal] = Key::EQUAL;
	xkb_keycode_map[XKB_KEY_KP_Separator] = Key::COMMA;
	xkb_keycode_map[XKB_KEY_KP_Decimal] = Key::KP_PERIOD;
	xkb_keycode_map[XKB_KEY_KP_Multiply] = Key::KP_MULTIPLY;
	xkb_keycode_map[XKB_KEY_KP_Divide] = Key::KP_DIVIDE;
	xkb_keycode_map[XKB_KEY_KP_Subtract] = Key::KP_SUBTRACT;
	xkb_keycode_map[XKB_KEY_KP_Add] = Key::KP_ADD;
	xkb_keycode_map[XKB_KEY_KP_0] = Key::KP_0;
	xkb_keycode_map[XKB_KEY_KP_1] = Key::KP_1;
	xkb_keycode_map[XKB_KEY_KP_2] = Key::KP_2;
	xkb_keycode_map[XKB_KEY_KP_3] = Key::KP_3;
	xkb_keycode_map[XKB_KEY_KP_4] = Key::KP_4;
	xkb_keycode_map[XKB_KEY_KP_5] = Key::KP_5;
	xkb_keycode_map[XKB_KEY_KP_6] = Key::KP_6;
	xkb_keycode_map[XKB_KEY_KP_7] = Key::KP_7;
	xkb_keycode_map[XKB_KEY_KP_8] = Key::KP_8;
	xkb_keycode_map[XKB_KEY_KP_9] = Key::KP_9;
	// Same keys but with numlock off.
	xkb_keycode_map[XKB_KEY_KP_Insert] = Key::INSERT;
	xkb_keycode_map[XKB_KEY_KP_Delete] = Key::KEY_DELETE;
	xkb_keycode_map[XKB_KEY_KP_End] = Key::END;
	xkb_keycode_map[XKB_KEY_KP_Down] = Key::DOWN;
	xkb_keycode_map[XKB_KEY_KP_Page_Down] = Key::PAGEDOWN;
	xkb_keycode_map[XKB_KEY_KP_Left] = Key::LEFT;
	// X11 documents this (numpad 5) as "begin of line" but no toolkit seems to interpret it this way.
	// On Windows this is emitting Key::Clear so for consistency it will be mapped to Key::Clear
	xkb_keycode_map[XKB_KEY_KP_Begin] = Key::CLEAR;
	xkb_keycode_map[XKB_KEY_KP_Right] = Key::RIGHT;
	xkb_keycode_map[XKB_KEY_KP_Home] = Key::HOME;
	xkb_keycode_map[XKB_KEY_KP_Up] = Key::UP;
	xkb_keycode_map[XKB_KEY_KP_Page_Up] = Key::PAGEUP;
	xkb_keycode_map[XKB_KEY_F1] = Key::F1;
	xkb_keycode_map[XKB_KEY_F2] = Key::F2;
	xkb_keycode_map[XKB_KEY_F3] = Key::F3;
	xkb_keycode_map[XKB_KEY_F4] = Key::F4;
	xkb_keycode_map[XKB_KEY_F5] = Key::F5;
	xkb_keycode_map[XKB_KEY_F6] = Key::F6;
	xkb_keycode_map[XKB_KEY_F7] = Key::F7;
	xkb_keycode_map[XKB_KEY_F8] = Key::F8;
	xkb_keycode_map[XKB_KEY_F9] = Key::F9;
	xkb_keycode_map[XKB_KEY_F10] = Key::F10;
	xkb_keycode_map[XKB_KEY_F11] = Key::F11;
	xkb_keycode_map[XKB_KEY_F12] = Key::F12;
	xkb_keycode_map[XKB_KEY_F13] = Key::F13;
	xkb_keycode_map[XKB_KEY_F14] = Key::F14;
	xkb_keycode_map[XKB_KEY_F15] = Key::F15;
	xkb_keycode_map[XKB_KEY_F16] = Key::F16;
	xkb_keycode_map[XKB_KEY_F17] = Key::F17;
	xkb_keycode_map[XKB_KEY_F18] = Key::F18;
	xkb_keycode_map[XKB_KEY_F19] = Key::F19;
	xkb_keycode_map[XKB_KEY_F20] = Key::F20;
	xkb_keycode_map[XKB_KEY_F21] = Key::F21;
	xkb_keycode_map[XKB_KEY_F22] = Key::F22;
	xkb_keycode_map[XKB_KEY_F23] = Key::F23;
	xkb_keycode_map[XKB_KEY_F24] = Key::F24;
	xkb_keycode_map[XKB_KEY_F25] = Key::F25;
	xkb_keycode_map[XKB_KEY_F26] = Key::F26;
	xkb_keycode_map[XKB_KEY_F27] = Key::F27;
	xkb_keycode_map[XKB_KEY_F28] = Key::F28;
	xkb_keycode_map[XKB_KEY_F29] = Key::F29;
	xkb_keycode_map[XKB_KEY_F30] = Key::F30;
	xkb_keycode_map[XKB_KEY_F31] = Key::F31;
	xkb_keycode_map[XKB_KEY_F32] = Key::F32;
	xkb_keycode_map[XKB_KEY_F33] = Key::F33;
	xkb_keycode_map[XKB_KEY_F34] = Key::F34;
	xkb_keycode_map[XKB_KEY_F35] = Key::F35;
	xkb_keycode_map[XKB_KEY_yen] = Key::YEN;
	xkb_keycode_map[XKB_KEY_section] = Key::SECTION;
	// Media keys.
	xkb_keycode_map[XKB_KEY_XF86Back] = Key::BACK;
	xkb_keycode_map[XKB_KEY_XF86Forward] = Key::FORWARD;
	xkb_keycode_map[XKB_KEY_XF86Stop] = Key::STOP;
	xkb_keycode_map[XKB_KEY_XF86Refresh] = Key::REFRESH;
	xkb_keycode_map[XKB_KEY_XF86Favorites] = Key::FAVORITES;
	xkb_keycode_map[XKB_KEY_XF86OpenURL] = Key::OPENURL;
	xkb_keycode_map[XKB_KEY_XF86HomePage] = Key::HOMEPAGE;
	xkb_keycode_map[XKB_KEY_XF86Search] = Key::SEARCH;
	xkb_keycode_map[XKB_KEY_XF86AudioLowerVolume] = Key::VOLUMEDOWN;
	xkb_keycode_map[XKB_KEY_XF86AudioMute] = Key::VOLUMEMUTE;
	xkb_keycode_map[XKB_KEY_XF86AudioRaiseVolume] = Key::VOLUMEUP;
	xkb_keycode_map[XKB_KEY_XF86AudioPlay] = Key::MEDIAPLAY;
	xkb_keycode_map[XKB_KEY_XF86AudioStop] = Key::MEDIASTOP;
	xkb_keycode_map[XKB_KEY_XF86AudioPrev] = Key::MEDIAPREVIOUS;
	xkb_keycode_map[XKB_KEY_XF86AudioNext] = Key::MEDIANEXT;
	xkb_keycode_map[XKB_KEY_XF86AudioRecord] = Key::MEDIARECORD;
	xkb_keycode_map[XKB_KEY_XF86Standby] = Key::STANDBY;
	// Launch keys.
	xkb_keycode_map[XKB_KEY_XF86Mail] = Key::LAUNCHMAIL;
	xkb_keycode_map[XKB_KEY_XF86AudioMedia] = Key::LAUNCHMEDIA;
	xkb_keycode_map[XKB_KEY_XF86MyComputer] = Key::LAUNCH0;
	xkb_keycode_map[XKB_KEY_XF86Calculator] = Key::LAUNCH1;
	xkb_keycode_map[XKB_KEY_XF86Launch0] = Key::LAUNCH2;
	xkb_keycode_map[XKB_KEY_XF86Launch1] = Key::LAUNCH3;
	xkb_keycode_map[XKB_KEY_XF86Launch2] = Key::LAUNCH4;
	xkb_keycode_map[XKB_KEY_XF86Launch3] = Key::LAUNCH5;
	xkb_keycode_map[XKB_KEY_XF86Launch4] = Key::LAUNCH6;
	xkb_keycode_map[XKB_KEY_XF86Launch5] = Key::LAUNCH7;
	xkb_keycode_map[XKB_KEY_XF86Launch6] = Key::LAUNCH8;
	xkb_keycode_map[XKB_KEY_XF86Launch7] = Key::LAUNCH9;
	xkb_keycode_map[XKB_KEY_XF86Launch8] = Key::LAUNCHA;
	xkb_keycode_map[XKB_KEY_XF86Launch9] = Key::LAUNCHB;
	xkb_keycode_map[XKB_KEY_XF86LaunchA] = Key::LAUNCHC;
	xkb_keycode_map[XKB_KEY_XF86LaunchB] = Key::LAUNCHD;
	xkb_keycode_map[XKB_KEY_XF86LaunchC] = Key::LAUNCHE;
	xkb_keycode_map[XKB_KEY_XF86LaunchD] = Key::LAUNCHF;

	// Scancode to Godot Key map.
	scancode_map[0x09] = Key::ESCAPE;
	scancode_map[0x0A] = Key::KEY_1;
	scancode_map[0x0B] = Key::KEY_2;
	scancode_map[0x0C] = Key::KEY_3;
	scancode_map[0x0D] = Key::KEY_4;
	scancode_map[0x0E] = Key::KEY_5;
	scancode_map[0x0F] = Key::KEY_6;
	scancode_map[0x10] = Key::KEY_7;
	scancode_map[0x11] = Key::KEY_8;
	scancode_map[0x12] = Key::KEY_9;
	scancode_map[0x13] = Key::KEY_0;
	scancode_map[0x14] = Key::MINUS;
	scancode_map[0x15] = Key::EQUAL;
	scancode_map[0x16] = Key::BACKSPACE;
	scancode_map[0x17] = Key::TAB;
	scancode_map[0x18] = Key::Q;
	scancode_map[0x19] = Key::W;
	scancode_map[0x1A] = Key::E;
	scancode_map[0x1B] = Key::R;
	scancode_map[0x1C] = Key::T;
	scancode_map[0x1D] = Key::Y;
	scancode_map[0x1E] = Key::U;
	scancode_map[0x1F] = Key::I;
	scancode_map[0x20] = Key::O;
	scancode_map[0x21] = Key::P;
	scancode_map[0x22] = Key::BRACELEFT;
	scancode_map[0x23] = Key::BRACERIGHT;
	scancode_map[0x24] = Key::ENTER;
	scancode_map[0x25] = Key::CTRL; // Left
	scancode_map[0x26] = Key::A;
	scancode_map[0x27] = Key::S;
	scancode_map[0x28] = Key::D;
	scancode_map[0x29] = Key::F;
	scancode_map[0x2A] = Key::G;
	scancode_map[0x2B] = Key::H;
	scancode_map[0x2C] = Key::J;
	scancode_map[0x2D] = Key::K;
	scancode_map[0x2E] = Key::L;
	scancode_map[0x2F] = Key::SEMICOLON;
	scancode_map[0x30] = Key::APOSTROPHE;
	scancode_map[0x31] = Key::SECTION;
	scancode_map[0x32] = Key::SHIFT; // Left
	scancode_map[0x33] = Key::BACKSLASH;
	scancode_map[0x34] = Key::Z;
	scancode_map[0x35] = Key::X;
	scancode_map[0x36] = Key::C;
	scancode_map[0x37] = Key::V;
	scancode_map[0x38] = Key::B;
	scancode_map[0x39] = Key::N;
	scancode_map[0x3A] = Key::M;
	scancode_map[0x3B] = Key::COMMA;
	scancode_map[0x3C] = Key::PERIOD;
	scancode_map[0x3D] = Key::SLASH;
	scancode_map[0x3E] = Key::SHIFT; // Right
	scancode_map[0x3F] = Key::KP_MULTIPLY;
	scancode_map[0x40] = Key::ALT; // Left
	scancode_map[0x41] = Key::SPACE;
	scancode_map[0x42] = Key::CAPSLOCK;
	scancode_map[0x43] = Key::F1;
	scancode_map[0x44] = Key::F2;
	scancode_map[0x45] = Key::F3;
	scancode_map[0x46] = Key::F4;
	scancode_map[0x47] = Key::F5;
	scancode_map[0x48] = Key::F6;
	scancode_map[0x49] = Key::F7;
	scancode_map[0x4A] = Key::F8;
	scancode_map[0x4B] = Key::F9;
	scancode_map[0x4C] = Key::F10;
	scancode_map[0x4D] = Key::NUMLOCK;
	scancode_map[0x4E] = Key::SCROLLLOCK;
	scancode_map[0x4F] = Key::KP_7;
	scancode_map[0x50] = Key::KP_8;
	scancode_map[0x51] = Key::KP_9;
	scancode_map[0x52] = Key::KP_SUBTRACT;
	scancode_map[0x53] = Key::KP_4;
	scancode_map[0x54] = Key::KP_5;
	scancode_map[0x55] = Key::KP_6;
	scancode_map[0x56] = Key::KP_ADD;
	scancode_map[0x57] = Key::KP_1;
	scancode_map[0x58] = Key::KP_2;
	scancode_map[0x59] = Key::KP_3;
	scancode_map[0x5A] = Key::KP_0;
	scancode_map[0x5B] = Key::KP_PERIOD;
	//scancode_map[0x5C]
	//scancode_map[0x5D] // Zenkaku Hankaku
	scancode_map[0x5E] = Key::QUOTELEFT;
	scancode_map[0x5F] = Key::F11;
	scancode_map[0x60] = Key::F12;
	//scancode_map[0x61] // Romaji
	//scancode_map[0x62] // Katakana
	//scancode_map[0x63] // Hiragana
	//scancode_map[0x64] // Henkan
	//scancode_map[0x65] // Hiragana Katakana
	//scancode_map[0x66] // Muhenkan
	scancode_map[0x67] = Key::COMMA; // KP_Separator
	scancode_map[0x68] = Key::KP_ENTER;
	scancode_map[0x69] = Key::CTRL; // Right
	scancode_map[0x6A] = Key::KP_DIVIDE;
	scancode_map[0x6B] = Key::PRINT;
	scancode_map[0x6C] = Key::ALT; // Right
	scancode_map[0x6D] = Key::ENTER;
	scancode_map[0x6E] = Key::HOME;
	scancode_map[0x6F] = Key::UP;
	scancode_map[0x70] = Key::PAGEUP;
	scancode_map[0x71] = Key::LEFT;
	scancode_map[0x72] = Key::RIGHT;
	scancode_map[0x73] = Key::END;
	scancode_map[0x74] = Key::DOWN;
	scancode_map[0x75] = Key::PAGEDOWN;
	scancode_map[0x76] = Key::INSERT;
	scancode_map[0x77] = Key::KEY_DELETE;
	//scancode_map[0x78] // Macro
	scancode_map[0x79] = Key::VOLUMEMUTE;
	scancode_map[0x7A] = Key::VOLUMEDOWN;
	scancode_map[0x7B] = Key::VOLUMEUP;
	//scancode_map[0x7C] // Power
	scancode_map[0x7D] = Key::EQUAL; // KP_Equal
	//scancode_map[0x7E] // KP_PlusMinus
	scancode_map[0x7F] = Key::PAUSE;
	scancode_map[0x80] = Key::LAUNCH0;
	scancode_map[0x81] = Key::COMMA; // KP_Comma
	//scancode_map[0x82] // Hangul
	//scancode_map[0x83] // Hangul_Hanja
	scancode_map[0x84] = Key::YEN;
	scancode_map[0x85] = Key::META; // Left
	scancode_map[0x86] = Key::META; // Right
	scancode_map[0x87] = Key::MENU;

	scancode_map[0xA6] = Key::BACK; // On Chromebooks
	scancode_map[0xA7] = Key::FORWARD; // On Chromebooks

	scancode_map[0xB5] = Key::REFRESH; // On Chromebooks

	scancode_map[0xBF] = Key::F13;
	scancode_map[0xC0] = Key::F14;
	scancode_map[0xC1] = Key::F15;
	scancode_map[0xC2] = Key::F16;
	scancode_map[0xC3] = Key::F17;
	scancode_map[0xC4] = Key::F18;
	scancode_map[0xC5] = Key::F19;
	scancode_map[0xC6] = Key::F20;
	scancode_map[0xC7] = Key::F21;
	scancode_map[0xC8] = Key::F22;
	scancode_map[0xC9] = Key::F23;
	scancode_map[0xCA] = Key::F24;
	scancode_map[0xCB] = Key::F25;
	scancode_map[0xCC] = Key::F26;
	scancode_map[0xCD] = Key::F27;
	scancode_map[0xCE] = Key::F28;
	scancode_map[0xCF] = Key::F29;
	scancode_map[0xD0] = Key::F30;
	scancode_map[0xD1] = Key::F31;
	scancode_map[0xD2] = Key::F32;
	scancode_map[0xD3] = Key::F33;
	scancode_map[0xD4] = Key::F34;
	scancode_map[0xD5] = Key::F35;

	// Godot to scancode map.
	for (const KeyValue<unsigned int, Key> &E : scancode_map) {
		scancode_map_inv[E.value] = E.key;
	}

	// Scancode to physical location map.
	// Ctrl.
	location_map[0x25] = KeyLocation::LEFT;
	location_map[0x69] = KeyLocation::RIGHT;
	// Shift.
	location_map[0x32] = KeyLocation::LEFT;
	location_map[0x3E] = KeyLocation::RIGHT;
	// Alt.
	location_map[0x40] = KeyLocation::LEFT;
	location_map[0x6C] = KeyLocation::RIGHT;
	// Meta.
	location_map[0x85] = KeyLocation::LEFT;
	location_map[0x86] = KeyLocation::RIGHT;
}

Key KeyMappingXKB::get_keycode(xkb_keycode_t p_keysym) {
	if (p_keysym >= 0x20 && p_keysym < 0x7E) { // ASCII, maps 1-1
		if (p_keysym > 0x60 && p_keysym < 0x7B) { // Lowercase ASCII.
			return (Key)(p_keysym - 32);
		} else {
			return (Key)p_keysym;
		}
	}

	const Key *key = xkb_keycode_map.getptr(p_keysym);
	if (key) {
		return *key;
	}
	return Key::NONE;
}

Key KeyMappingXKB::get_scancode(unsigned int p_code) {
	const Key *key = scancode_map.getptr(p_code);
	if (key) {
		return *key;
	}

	return Key::NONE;
}

xkb_keycode_t KeyMappingXKB::get_xkb_keycode(Key p_key) {
	const unsigned int *key = scancode_map_inv.getptr(p_key);
	if (key) {
		return *key;
	}
	return 0x00;
}

KeyLocation KeyMappingXKB::get_location(unsigned int p_code) {
	const KeyLocation *location = location_map.getptr(p_code);
	if (location) {
		return *location;
	}
	return KeyLocation::UNSPECIFIED;
}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/wayland_thread.cpp
// ============================================================================

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#ifdef WAYLAND_ENABLED

#include "core/config/engine.h"
#include "core/io/image.h"
#include "core/os/os.h"

#ifdef __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
// Assume Linux.
#include <linux/input-event-codes.h>
#endif

// For the actual polling thread.
#include <poll.h>

// For shared memory buffer creation.
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Fix the wl_array_for_each macro to work with C++. This is based on the
// original from `wayland-util.h` in the Wayland client library.
#undef wl_array_for_each
#define wl_array_for_each(pos, array)                                                              \
	for (pos = (decltype(pos))(array)->data;                                                       \
		 (const char*)pos < ((const char*)(array)->data + (array)->size); (pos)++)

#define WAYLAND_THREAD_DEBUG_LOGS_ENABLED
#ifdef WAYLAND_THREAD_DEBUG_LOGS_ENABLED
#define DEBUG_LOG_WAYLAND_THREAD(...) print_verbose(__VA_ARGS__)
#else
#define DEBUG_LOG_WAYLAND_THREAD(...)
#endif

// Since we're never going to use this interface directly, it's not worth
// generating the whole deal.
#define FIFO_INTERFACE_NAME "wp_fifo_manager_v1"

// Read the content pointed by fd into a Vector<uint8_t>.
Vector<uint8_t> WaylandThread::_read_fd(int fd)
{
	// This is pretty much an arbitrary size.
	uint32_t chunk_size = 2048;

	LocalVector<uint8_t> data;
	data.resize(chunk_size);

	uint32_t bytes_read = 0;

	while (true) {
		ssize_t last_bytes_read = read(fd, data.ptr() + bytes_read, chunk_size);
		if (last_bytes_read < 0) {
			ERR_PRINT(vformat("Read error %d.", errno));

			data.clear();
			break;
		}

		if (last_bytes_read == 0) {
			// We're done, we've reached the EOF.
			DEBUG_LOG_WAYLAND_THREAD(vformat("Done reading %d bytes.", bytes_read));

			close(fd);

			data.resize(bytes_read);
			break;
		}

		DEBUG_LOG_WAYLAND_THREAD(vformat("Read chunk of %d bytes.", last_bytes_read));

		bytes_read += last_bytes_read;

		// Increase the buffer size by one chunk in preparation of the next read.
		data.resize(bytes_read + chunk_size);
	}

	return Vector<uint8_t>(data);
}

// Based on the wayland book's shared memory boilerplate (PD/CC0).
// See: https://wayland-book.com/surfaces/shared-memory.html
int WaylandThread::_allocate_shm_file(size_t size)
{
	int retries = 100;

	do {
		// Generate a random name.
		char name[] = "/wl_shm-godot-XXXXXX";
		for (long unsigned int i = sizeof(name) - 7; i < sizeof(name) - 1; i++) {
			name[i] = Math::random('A', 'Z');
		}

		// Try to open a shared memory object with that name.
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			// Success, unlink its name as we just need the file descriptor.
			shm_unlink(name);

			// Resize the file to the requested length.
			int ret;
			do {
				ret = ftruncate(fd, size);
			} while (ret < 0 && errno == EINTR);

			if (ret < 0) {
				close(fd);
				return -1;
			}

			return fd;
		}

		retries--;
	} while (retries > 0 && errno == EEXIST);

	return -1;
}

// Return the content of a wl_data_offer.
Vector<uint8_t> WaylandThread::_wl_data_offer_read(
	struct wl_display* p_display, const char* p_mime, struct wl_data_offer* p_offer)
{
	if (!p_offer) {
		return Vector<uint8_t>();
	}

	int fds[2];
	if (pipe(fds) == 0) {
		wl_data_offer_receive(p_offer, p_mime, fds[1]);

		// Let the compositor know about the pipe.
		// NOTE: It's important to just flush and not roundtrip here as we would risk
		// running some cleanup event, like for example `wl_data_device::leave`. We're
		// going to wait for the message anyways as the read will probably block if
		// the compositor doesn't read from the other end of the pipe.
		wl_display_flush(p_display);

		// Close the write end of the pipe, which we don't need and would otherwise
		// just stall our next `read`s.
		close(fds[1]);

		return _read_fd(fds[0]);
	}

	return Vector<uint8_t>();
}

// Read the content of a wp_primary_selection_offer.
Vector<uint8_t> WaylandThread::_wp_primary_selection_offer_read(struct wl_display* p_display,
	const char* p_mime, struct zwp_primary_selection_offer_v1* p_offer)
{
	if (!p_offer) {
		return Vector<uint8_t>();
	}

	int fds[2];
	if (pipe(fds) == 0) {
		zwp_primary_selection_offer_v1_receive(p_offer, p_mime, fds[1]);

		// NOTE: It's important to just flush and not roundtrip here as we would risk
		// running some cleanup event, like for example `wl_data_device::leave`. We're
		// going to wait for the message anyways as the read will probably block if
		// the compositor doesn't read from the other end of the pipe.
		wl_display_flush(p_display);

		// Close the write end of the pipe, which we don't need and would otherwise
		// just stall our next `read`s.
		close(fds[1]);

		return _read_fd(fds[0]);
	}

	return Vector<uint8_t>();
}

void WaylandThread::_wl_display_check_error(struct wl_display* wl_display)
{
	int werror = wl_display_get_error(wl_display);
	if (werror) {
		if (werror == EPROTO) {
			struct wl_interface* wl_interface = nullptr;
			uint32_t id = 0;

			int error_code = wl_display_get_protocol_error(
				wl_display, (const struct wl_interface**)&wl_interface, &id);
			CRASH_NOW_MSG(vformat("Wayland protocol error %d on interface %s@%d.", error_code,
				wl_interface ? wl_interface->name : "unknown", id));
		}
		else {
			CRASH_NOW_MSG(vformat("Wayland client error code %d.", werror));
		}
	}
}

InputEvent WaylandThread::_seat_state_get_key_event(
	SeatState* p_ss, xkb_keycode_t p_keycode, bool p_pressed)
{
	ERR_FAIL_NULL_V(p_ss, InputEvent());

	Key shifted_key =
		KeyMappingXKB::get_keycode(xkb_state_key_get_one_sym(p_ss->xkb_state, p_keycode));

	Key plain_key = Key::NONE;
	const xkb_keysym_t* syms = nullptr;
	int num_sys = xkb_keymap_key_get_syms_by_level(
		p_ss->xkb_keymap, p_keycode, p_ss->current_layout_index, 0, &syms);
	if (num_sys > 0 && syms) {
		plain_key = KeyMappingXKB::get_keycode(syms[0]);
	}

	Key physical_keycode = KeyMappingXKB::get_scancode(p_keycode);
	KeyLocation key_location = KeyMappingXKB::get_location(p_keycode);
	uint32_t unicode = xkb_state_key_get_utf32(p_ss->xkb_state, p_keycode);

	Key keycode = Key::NONE;

	if ((shifted_key & Key::SPECIAL) != Key::NONE || (plain_key & Key::SPECIAL) != Key::NONE) {
		keycode = shifted_key;
	}

	if (keycode == Key::NONE) {
		keycode = plain_key;
	}

	if (keycode == Key::NONE) {
		keycode = physical_keycode;
	}

	if (keycode >= Key::A + 32 && keycode <= Key::Z + 32) {
		keycode -= 'a' - 'A';
	}

	if (physical_keycode == Key::NONE && keycode == Key::NONE && unicode == 0) {
		return InputEvent();
	}

	InputEvent event = InputEvent::make_key();
	event.window_id = p_ss->focused_id;

	event.set_shift_pressed(p_ss->shift_pressed);
	event.set_ctrl_pressed(p_ss->ctrl_pressed);
	event.set_alt_pressed(p_ss->alt_pressed);
	event.set_meta_pressed(p_ss->meta_pressed);

	event.pressed = p_pressed;
	event.key.keycode = keycode;
	event.key.physical_keycode = physical_keycode;
	event.key.location = key_location;
	event.key.key_label = (unicode != 0) ? fix_key_label(unicode, keycode) : keycode;

	if (p_pressed) {
		event.key.unicode = fix_unicode(unicode);
	}

	if (event.key.keycode == Key::BACKTAB) {
		event.key.keycode = Key::TAB;
		event.key.physical_keycode = Key::TAB;
		event.set_shift_pressed(true);
	}

	return event;
}

InputEvent WaylandThread::_seat_state_get_unstuck_key_event(
	SeatState* p_ss, xkb_keycode_t p_keycode, bool p_pressed, Key p_key)
{
	InputEvent event;

	if (p_pressed) {
		Key* old_key = p_ss->pressed_keycodes.getptr(p_keycode);
		if (old_key != nullptr && *old_key != p_key) {
			print_verbose(vformat("%s and %s have same keycode. Generating release event for %s",
				keycode_get_string(*old_key), keycode_get_string(p_key),
				keycode_get_string(*old_key)));
			event = _seat_state_get_key_event(p_ss, p_keycode, false);
			if (event.is_valid()) {
				event.key.keycode = *old_key;
			}
		}
		p_ss->pressed_keycodes[p_keycode] = p_key;
	}
	else {
		p_ss->pressed_keycodes.erase(p_keycode);
	}

	return event;
}

void WaylandThread::_seat_state_handle_xkb_keycode(
	SeatState* p_ss, xkb_keycode_t p_xkb_keycode, bool p_pressed, bool p_echo)
{
	ERR_FAIL_NULL(p_ss);

	WaylandThread* wayland_thread = p_ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	Key last_key = Key::NONE;
	xkb_compose_status compose_status = xkb_compose_state_get_status(p_ss->xkb_compose_state);

	if (p_pressed) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym(p_ss->xkb_state, p_xkb_keycode);
		xkb_compose_feed_result compose_result =
			xkb_compose_state_feed(p_ss->xkb_compose_state, keysym);
		compose_status = xkb_compose_state_get_status(p_ss->xkb_compose_state);

		if (compose_result == XKB_COMPOSE_FEED_ACCEPTED && compose_status == XKB_COMPOSE_COMPOSED) {
			char str_xkb[256] = {};
			int str_xkb_size = xkb_compose_state_get_utf8(p_ss->xkb_compose_state, str_xkb, 255);

			String decoded_str = String::utf8(str_xkb, str_xkb_size);
			for (int i = 0; i < decoded_str.length(); ++i) {
				InputEvent k = _seat_state_get_key_event(p_ss, p_xkb_keycode, p_pressed);
				if (k.is_null()) {
					continue;
				}

				k.key.unicode = decoded_str[i];
				k.key.echo = p_echo;

				Ref<InputEventMessage> msg;
				msg.instantiate();
				msg->event = k;
				wayland_thread->push_message(msg);

				last_key = k.key.keycode;
			}
		}
	}

	if (last_key == Key::NONE && compose_status == XKB_COMPOSE_NOTHING) {
		InputEvent k = _seat_state_get_key_event(p_ss, p_xkb_keycode, p_pressed);
		if (k.is_valid()) {
			k.key.echo = p_echo;

			Ref<InputEventMessage> msg;
			msg.instantiate();
			msg->event = k;
			wayland_thread->push_message(msg);

			last_key = k.key.keycode;
		}
	}

	if (last_key != Key::NONE) {
		InputEvent uk = _seat_state_get_unstuck_key_event(p_ss, p_xkb_keycode, p_pressed, last_key);
		if (uk.is_valid()) {
			Ref<InputEventMessage> u_msg;
			u_msg.instantiate();
			u_msg->event = uk;
			wayland_thread->push_message(u_msg);
		}
	}
}

void WaylandThread::_set_current_seat(struct wl_seat* p_seat)
{
	if (p_seat == wl_seat_current) {
		return;
	}

	SeatState* old_state = wl_seat_get_seat_state(wl_seat_current);

	if (old_state) {
		seat_state_unlock_pointer(old_state);
	}

	SeatState* new_state = wl_seat_get_seat_state(p_seat);
	seat_state_unlock_pointer(new_state);

	wl_seat_current = p_seat;
}

// Most of the times we just want to update the hover to the currently pointed
// window. Making this the default helps with enforcing the correct behavior.
void WaylandThread::_window_hover()
{
	DisplayServerEnums::WindowID pointed_id = pointer_get_pointed_window_id();
	if (window_exists(pointed_id)) {
		_window_hover(pointed_id);
	}
	else {
		_window_hover(DisplayServerEnums::INVALID_WINDOW_ID);
	}
}

// Returns whether it loaded the theme or not.
bool WaylandThread::_load_cursor_theme(int p_cursor_size)
{
	if (wl_cursor_theme) {
		wl_cursor_theme_destroy(wl_cursor_theme);
		wl_cursor_theme = nullptr;
	}

	if (cursor_theme_name.is_empty()) {
		cursor_theme_name = "default";
	}

	print_verbose(
		vformat("Loading cursor theme \"%s\" size %d.", cursor_theme_name, p_cursor_size));

	wl_cursor_theme =
		wl_cursor_theme_load(cursor_theme_name.utf8().get_data(), p_cursor_size, registry.wl_shm);

	ERR_FAIL_NULL_V_MSG(wl_cursor_theme, false, "Can't load any cursor theme.");

	static const char* cursor_names[] = {"left_ptr", "xterm", "hand2", "cross", "watch",
		"left_ptr_watch", "fleur", "dnd-move", "crossed_circle", "v_double_arrow", "h_double_arrow",
		"size_bdiag", "size_fdiag", "move", "row_resize", "col_resize", "question_arrow"};

	static const char* cursor_names_fallback[] = {nullptr, nullptr, "pointer", "cross", "wait",
		"progress", "grabbing", "hand1", "forbidden", "ns-resize", "ew-resize", "fd_double_arrow",
		"bd_double_arrow", "fleur", "sb_v_double_arrow", "sb_h_double_arrow", "help"};

	for (int i = 0; i < DisplayServerEnums::CURSOR_MAX; i++) {
		struct wl_cursor* cursor = wl_cursor_theme_get_cursor(wl_cursor_theme, cursor_names[i]);

		if (!cursor && cursor_names_fallback[i]) {
			cursor = wl_cursor_theme_get_cursor(wl_cursor_theme, cursor_names_fallback[i]);
		}

		if (cursor && cursor->image_count > 0) {
			wl_cursors[i] = cursor;
		}
		else {
			wl_cursors[i] = nullptr;
			print_verbose("Failed loading cursor: " + String(cursor_names[i]));
		}
	}

	return true;
}

void WaylandThread::_update_scale(int p_scale)
{
	if (p_scale <= cursor_scale) {
		return;
	}

	print_verbose(vformat("Bumping cursor scale to %d", p_scale));

	// There's some display that's bigger than the cache, let's update it.
	cursor_scale = p_scale;

	if (wl_cursor_theme == nullptr) {
		// Ugh. Either we're still initializing (this must've been called from the
		// first roundtrips) or we had some error while doing so. We'll trust that it
		// will be updated for us if needed.
		return;
	}

	int cursor_size = unscaled_cursor_size * p_scale;

	if (_load_cursor_theme(cursor_size)) {
		for (struct wl_seat* wl_seat : registry.wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			seat_state_update_cursor(ss);
		}
	}
}

void WaylandThread::_wl_registry_on_global(void* data, struct wl_registry* wl_registry,
	uint32_t name, const char* interface, uint32_t version)
{
	RegistryState* registry = (RegistryState*)data;
	ERR_FAIL_NULL(registry);

	if (registry->global_names.has(name)) {
		WARN_PRINT("Received duplicate Wayland global announcement!");
	}

	registry->global_names.insert(name);

	if (strcmp(interface, wl_shm_interface.name) == 0) {
		registry->wl_shm =
			(struct wl_shm*)wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
		registry->wl_shm_name = name;
		return;
	}

	// NOTE: Deprecated.
	if (strcmp(interface, zxdg_exporter_v1_interface.name) == 0) {
		registry->xdg_exporter_v1 = (struct zxdg_exporter_v1*)wl_registry_bind(
			wl_registry, name, &zxdg_exporter_v1_interface, 1);
		registry->xdg_exporter_v1_name = name;
		return;
	}

	if (strcmp(interface, zxdg_exporter_v2_interface.name) == 0) {
		registry->xdg_exporter_v2 = (struct zxdg_exporter_v2*)wl_registry_bind(
			wl_registry, name, &zxdg_exporter_v2_interface, 1);
		registry->xdg_exporter_v2_name = name;
		return;
	}

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		registry->wl_compositor = (struct wl_compositor*)wl_registry_bind(
			wl_registry, name, &wl_compositor_interface, CLAMP((int)version, 1, 6));
		registry->wl_compositor_name = name;
		return;
	}

	if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
		registry->wl_data_device_manager = (struct wl_data_device_manager*)wl_registry_bind(
			wl_registry, name, &wl_data_device_manager_interface, CLAMP((int)version, 1, 3));
		registry->wl_data_device_manager_name = name;

		// This global creates some seat data. Let's do that for the ones already available.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wl_data_device == nullptr) {
				ss->wl_data_device = wl_data_device_manager_get_data_device(
					registry->wl_data_device_manager, wl_seat);
				wl_data_device_add_listener(ss->wl_data_device, &wl_data_device_listener, ss);
			}
		}
		return;
	}

	if (strcmp(interface, wl_output_interface.name) == 0) {
		struct wl_output* wl_output = (struct wl_output*)wl_registry_bind(
			wl_registry, name, &wl_output_interface, CLAMP((int)version, 1, 4));
		wl_proxy_tag_godot((struct wl_proxy*)wl_output);

		registry->wl_outputs.push_back(wl_output);

		ScreenState* ss = memnew(ScreenState);
		ss->wl_output_name = name;
		ss->wayland_thread = registry->wayland_thread;

		wl_proxy_tag_godot((struct wl_proxy*)wl_output);
		wl_output_add_listener(wl_output, &wl_output_listener, ss);
		return;
	}

	if (strcmp(interface, wl_seat_interface.name) == 0) {
		struct wl_seat* wl_seat = (struct wl_seat*)wl_registry_bind(
			wl_registry, name, &wl_seat_interface, CLAMP((int)version, 1, 9));
		wl_proxy_tag_godot((struct wl_proxy*)wl_seat);

		SeatState* ss = memnew(SeatState);
		ss->wl_seat = wl_seat;
		ss->wl_seat_name = name;

		ss->registry = registry;
		ss->wayland_thread = registry->wayland_thread;

		// Some extra stuff depends on other globals. We'll initialize them if the
		// globals are already there, otherwise we'll have to do that once and if they
		// get announced.
		//
		// NOTE: Don't forget to also bind/destroy with the respective global.
		if (!ss->wl_data_device && registry->wl_data_device_manager) {
			// Clipboard & DnD.
			ss->wl_data_device =
				wl_data_device_manager_get_data_device(registry->wl_data_device_manager, wl_seat);
			wl_data_device_add_listener(ss->wl_data_device, &wl_data_device_listener, ss);
		}

		if (!ss->wp_primary_selection_device && registry->wp_primary_selection_device_manager) {
			// Primary selection.
			ss->wp_primary_selection_device = zwp_primary_selection_device_manager_v1_get_device(
				registry->wp_primary_selection_device_manager, wl_seat);
			zwp_primary_selection_device_v1_add_listener(
				ss->wp_primary_selection_device, &wp_primary_selection_device_listener, ss);
		}

		if (!ss->wp_tablet_seat && registry->wp_tablet_manager) {
			// Tablet.
			ss->wp_tablet_seat =
				zwp_tablet_manager_v2_get_tablet_seat(registry->wp_tablet_manager, wl_seat);
			zwp_tablet_seat_v2_add_listener(ss->wp_tablet_seat, &wp_tablet_seat_listener, ss);
		}

		if (!ss->wp_text_input && registry->wp_text_input_manager) {
			// IME.
			ss->wp_text_input =
				zwp_text_input_manager_v3_get_text_input(registry->wp_text_input_manager, wl_seat);
			zwp_text_input_v3_add_listener(ss->wp_text_input, &wp_text_input_listener, ss);
		}

		registry->wl_seats.push_back(wl_seat);

		wl_seat_add_listener(wl_seat, &wl_seat_listener, ss);

		if (registry->wayland_thread->wl_seat_current == nullptr) {
			registry->wayland_thread->_set_current_seat(wl_seat);
		}

		return;
	}

	if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		registry->xdg_wm_base = (struct xdg_wm_base*)wl_registry_bind(
			wl_registry, name, &xdg_wm_base_interface, CLAMP((int)version, 1, 6));
		registry->xdg_wm_base_name = name;

		xdg_wm_base_add_listener(registry->xdg_wm_base, &xdg_wm_base_listener, nullptr);
		return;
	}

	if (strcmp(interface, wp_viewporter_interface.name) == 0) {
		registry->wp_viewporter =
			(struct wp_viewporter*)wl_registry_bind(wl_registry, name, &wp_viewporter_interface, 1);
		registry->wp_viewporter_name = name;
	}

	if (strcmp(interface, wp_color_manager_v1_interface.name) == 0) {
		registry->wp_color_manager = (struct wp_color_manager_v1*)wl_registry_bind(
			wl_registry, name, &wp_color_manager_v1_interface, 1);
		registry->wp_color_manager_name = name;
		wl_proxy_tag_godot((struct wl_proxy*)registry->wp_color_manager);

		WaylandThread::ColorManagementState* color_state =
			memnew(WaylandThread::ColorManagementState);
		wp_color_manager_v1_add_listener(
			registry->wp_color_manager, &wp_color_manager_listener, color_state);
		return;
	}

	if (strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0) {
		registry->wp_cursor_shape_manager = (struct wp_cursor_shape_manager_v1*)wl_registry_bind(
			wl_registry, name, &wp_cursor_shape_manager_v1_interface, 1);
		registry->wp_cursor_shape_manager_name = name;
		return;
	}

	if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
		registry->wp_fractional_scale_manager =
			(struct wp_fractional_scale_manager_v1*)wl_registry_bind(
				wl_registry, name, &wp_fractional_scale_manager_v1_interface, 1);
		registry->wp_fractional_scale_manager_name = name;

		// NOTE: We're not mapping the fractional scale object here because this is
		// supposed to be a "startup global". If for some reason this isn't true (who
		// knows), add a conditional branch for creating the add-on object.
	}

	if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
		registry->xdg_decoration_manager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(
			wl_registry, name, &zxdg_decoration_manager_v1_interface, 1);
		registry->xdg_decoration_manager_name = name;
		return;
	}

	if (strcmp(interface, xdg_system_bell_v1_interface.name) == 0) {
		registry->xdg_system_bell = (struct xdg_system_bell_v1*)wl_registry_bind(
			wl_registry, name, &xdg_system_bell_v1_interface, 1);
		registry->xdg_system_bell_name = name;
		return;
	}

	if (strcmp(interface, xdg_toplevel_icon_manager_v1_interface.name) == 0) {
		registry->xdg_toplevel_icon_manager =
			(struct xdg_toplevel_icon_manager_v1*)wl_registry_bind(
				wl_registry, name, &xdg_toplevel_icon_manager_v1_interface, 1);
		registry->xdg_toplevel_icon_manager_name = name;

		if (registry->wayland_thread->xdg_icon) {
			xdg_toplevel_icon_v1_destroy(registry->wayland_thread->xdg_icon);
			registry->wayland_thread->xdg_icon = nullptr;
		}

		if (registry->wayland_thread->icon_buffer) {
			wl_buffer_destroy(registry->wayland_thread->icon_buffer);
			registry->wayland_thread->icon_buffer = nullptr;
		}

		return;
	}

	if (strcmp(interface, xdg_activation_v1_interface.name) == 0) {
		registry->xdg_activation = (struct xdg_activation_v1*)wl_registry_bind(
			wl_registry, name, &xdg_activation_v1_interface, 1);
		registry->xdg_activation_name = name;
		return;
	}

	if (strcmp(interface, zwp_primary_selection_device_manager_v1_interface.name) == 0) {
		registry->wp_primary_selection_device_manager =
			(struct zwp_primary_selection_device_manager_v1*)wl_registry_bind(
				wl_registry, name, &zwp_primary_selection_device_manager_v1_interface, 1);

		// This global creates some seat data. Let's do that for the ones already available.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (!ss->wp_primary_selection_device && registry->wp_primary_selection_device_manager) {
				ss->wp_primary_selection_device =
					zwp_primary_selection_device_manager_v1_get_device(
						registry->wp_primary_selection_device_manager, wl_seat);
				zwp_primary_selection_device_v1_add_listener(
					ss->wp_primary_selection_device, &wp_primary_selection_device_listener, ss);
			}
		}
	}

	if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
		registry->wp_relative_pointer_manager =
			(struct zwp_relative_pointer_manager_v1*)wl_registry_bind(
				wl_registry, name, &zwp_relative_pointer_manager_v1_interface, 1);
		registry->wp_relative_pointer_manager_name = name;
		return;
	}

	if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0) {
		registry->wp_pointer_constraints = (struct zwp_pointer_constraints_v1*)wl_registry_bind(
			wl_registry, name, &zwp_pointer_constraints_v1_interface, 1);
		registry->wp_pointer_constraints_name = name;
		return;
	}

	if (strcmp(interface, zwp_pointer_gestures_v1_interface.name) == 0) {
		registry->wp_pointer_gestures = (struct zwp_pointer_gestures_v1*)wl_registry_bind(
			wl_registry, name, &zwp_pointer_gestures_v1_interface, 1);
		registry->wp_pointer_gestures_name = name;
		return;
	}

	if (strcmp(interface, zwp_idle_inhibit_manager_v1_interface.name) == 0) {
		registry->wp_idle_inhibit_manager = (struct zwp_idle_inhibit_manager_v1*)wl_registry_bind(
			wl_registry, name, &zwp_idle_inhibit_manager_v1_interface, 1);
		registry->wp_idle_inhibit_manager_name = name;
		return;
	}

	if (strcmp(interface, zwp_tablet_manager_v2_interface.name) == 0) {
		registry->wp_tablet_manager = (struct zwp_tablet_manager_v2*)wl_registry_bind(
			wl_registry, name, &zwp_tablet_manager_v2_interface, 1);
		registry->wp_tablet_manager_name = name;

		// This global creates some seat data. Let's do that for the ones already available.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			ss->wp_tablet_seat =
				zwp_tablet_manager_v2_get_tablet_seat(registry->wp_tablet_manager, wl_seat);
			zwp_tablet_seat_v2_add_listener(ss->wp_tablet_seat, &wp_tablet_seat_listener, ss);
		}

		return;
	}

	if (strcmp(interface, zwp_text_input_manager_v3_interface.name) == 0) {
		registry->wp_text_input_manager = (struct zwp_text_input_manager_v3*)wl_registry_bind(
			wl_registry, name, &zwp_text_input_manager_v3_interface, 1);
		registry->wp_text_input_manager_name = name;

		// This global creates some seat data. Let's do that for the ones already available.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			ss->wp_text_input =
				zwp_text_input_manager_v3_get_text_input(registry->wp_text_input_manager, wl_seat);
			zwp_text_input_v3_add_listener(ss->wp_text_input, &wp_text_input_listener, ss);
		}

		return;
	}

	if (strcmp(interface, wp_pointer_warp_v1_interface.name) == 0) {
		registry->wp_pointer_warp = (struct wp_pointer_warp_v1*)wl_registry_bind(
			wl_registry, name, &wp_pointer_warp_v1_interface, 1);
		registry->wp_pointer_warp_name = name;
		return;
	}

	if (strcmp(interface, FIFO_INTERFACE_NAME) == 0) {
		registry->wp_fifo_manager_name = name;
	}

	if (strcmp(interface, godot_embedding_compositor_interface.name) == 0) {
		registry->godot_embedding_compositor = (struct godot_embedding_compositor*)wl_registry_bind(
			wl_registry, name, &godot_embedding_compositor_interface, 1);
		registry->godot_embedding_compositor_name = name;

		godot_embedding_compositor_add_listener(registry->godot_embedding_compositor,
			&godot_embedding_compositor_listener, memnew(EmbeddingCompositorState));
	}
}

void WaylandThread::_wl_registry_on_global_remove(
	void* data, struct wl_registry* wl_registry, uint32_t name)
{
	RegistryState* registry = (RegistryState*)data;
	ERR_FAIL_NULL(registry);

	if (!registry->global_names.erase(name)) {
		WARN_PRINT("Received remove event for unknown Wayland global!");
	}

	if (name == registry->wl_shm_name) {
		if (registry->wl_shm) {
			wl_shm_destroy(registry->wl_shm);
			registry->wl_shm = nullptr;
		}

		registry->wl_shm_name = 0;

		return;
	}

	// NOTE: Deprecated.
	if (name == registry->xdg_exporter_v1_name) {
		if (registry->xdg_exporter_v1) {
			zxdg_exporter_v1_destroy(registry->xdg_exporter_v1);
			registry->xdg_exporter_v1 = nullptr;
		}

		registry->xdg_exporter_v1_name = 0;

		return;
	}

	if (name == registry->xdg_exporter_v2_name) {
		if (registry->xdg_exporter_v2) {
			zxdg_exporter_v2_destroy(registry->xdg_exporter_v2);
			registry->xdg_exporter_v2 = nullptr;
		}

		registry->xdg_exporter_v2_name = 0;

		return;
	}

	if (name == registry->wl_compositor_name) {
		if (registry->wl_compositor) {
			wl_compositor_destroy(registry->wl_compositor);
			registry->wl_compositor = nullptr;
		}

		registry->wl_compositor_name = 0;

		return;
	}

	if (name == registry->wl_data_device_manager_name) {
		if (registry->wl_data_device_manager) {
			wl_data_device_manager_destroy(registry->wl_data_device_manager);
			registry->wl_data_device_manager = nullptr;
		}

		registry->wl_data_device_manager_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wl_data_device) {
				wl_data_device_destroy(ss->wl_data_device);
				ss->wl_data_device = nullptr;
			}

			ss->wl_data_device = nullptr;
		}

		return;
	}

	if (name == registry->xdg_wm_base_name) {
		if (registry->xdg_wm_base) {
			xdg_wm_base_destroy(registry->xdg_wm_base);
			registry->xdg_wm_base = nullptr;
		}

		registry->xdg_wm_base_name = 0;

		return;
	}

	if (name == registry->wp_viewporter_name) {
		for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair :
			registry->wayland_thread->windows) {
			WindowState& ws = pair.value;
			if (registry->wp_viewporter) {
				wp_viewporter_destroy(registry->wp_viewporter);
				registry->wp_viewporter = nullptr;
			}

			if (ws.wp_viewport) {
				wp_viewport_destroy(ws.wp_viewport);
				ws.wp_viewport = nullptr;
			}
		}

		registry->wp_viewporter_name = 0;

		return;
	}

	if (name == registry->wp_color_manager_name) {
		for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair :
			registry->wayland_thread->windows) {
			WindowState ws = pair.value;

			if (ws.wp_color_management_surface) {
				wp_color_management_surface_v1_destroy(ws.wp_color_management_surface);
				ws.wp_color_management_surface = nullptr;
			}

			if (ws.wp_color_management_surface_feedback) {
				wp_color_management_surface_feedback_v1_destroy(
					ws.wp_color_management_surface_feedback);
				ws.wp_color_management_surface_feedback = nullptr;
			}
		}

		if (registry->wp_color_manager) {
			ColorManagementState* color_state =
				wp_color_manager_get_state(registry->wp_color_manager);
			memdelete(color_state);

			wp_color_manager_v1_destroy(registry->wp_color_manager);
			registry->wp_color_manager = nullptr;
		}

		registry->wp_color_manager_name = 0;
	}

	if (name == registry->wp_cursor_shape_manager_name) {
		if (registry->wp_cursor_shape_manager) {
			wp_cursor_shape_manager_v1_destroy(registry->wp_cursor_shape_manager);
			registry->wp_cursor_shape_manager = nullptr;
		}

		registry->wp_cursor_shape_manager_name = 0;

		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wp_cursor_shape_device) {
				wp_cursor_shape_device_v1_destroy(ss->wp_cursor_shape_device);
				ss->wp_cursor_shape_device = nullptr;
			}
		}
	}

	if (name == registry->wp_fractional_scale_manager_name) {
		for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair :
			registry->wayland_thread->windows) {
			WindowState& ws = pair.value;

			if (registry->wp_fractional_scale_manager) {
				wp_fractional_scale_manager_v1_destroy(registry->wp_fractional_scale_manager);
				registry->wp_fractional_scale_manager = nullptr;
			}

			if (ws.wp_fractional_scale) {
				wp_fractional_scale_v1_destroy(ws.wp_fractional_scale);
				ws.wp_fractional_scale = nullptr;
			}
		}

		registry->wp_fractional_scale_manager_name = 0;
	}

	if (name == registry->xdg_decoration_manager_name) {
		if (registry->xdg_decoration_manager) {
			zxdg_decoration_manager_v1_destroy(registry->xdg_decoration_manager);
			registry->xdg_decoration_manager = nullptr;
		}

		registry->xdg_decoration_manager_name = 0;

		return;
	}

	if (name == registry->xdg_system_bell_name) {
		if (registry->xdg_system_bell) {
			xdg_system_bell_v1_destroy(registry->xdg_system_bell);
			registry->xdg_system_bell = nullptr;
		}

		registry->xdg_system_bell_name = 0;

		return;
	}

	if (name == registry->xdg_toplevel_icon_manager_name) {
		if (registry->xdg_toplevel_icon_manager) {
			xdg_toplevel_icon_manager_v1_destroy(registry->xdg_toplevel_icon_manager);
			registry->xdg_toplevel_icon_manager = nullptr;
		}

		if (registry->wayland_thread->xdg_icon) {
			xdg_toplevel_icon_v1_destroy(registry->wayland_thread->xdg_icon);
		}

		if (registry->wayland_thread->icon_buffer) {
			wl_buffer_destroy(registry->wayland_thread->icon_buffer);
		}

		registry->xdg_toplevel_icon_manager_name = 0;

		return;
	}

	if (name == registry->xdg_activation_name) {
		if (registry->xdg_activation) {
			xdg_activation_v1_destroy(registry->xdg_activation);
			registry->xdg_activation = nullptr;
		}

		registry->xdg_activation_name = 0;

		return;
	}

	if (name == registry->wp_primary_selection_device_manager_name) {
		if (registry->wp_primary_selection_device_manager) {
			zwp_primary_selection_device_manager_v1_destroy(
				registry->wp_primary_selection_device_manager);
			registry->wp_primary_selection_device_manager = nullptr;
		}

		registry->wp_primary_selection_device_manager_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wp_primary_selection_device) {
				zwp_primary_selection_device_v1_destroy(ss->wp_primary_selection_device);
				ss->wp_primary_selection_device = nullptr;
			}

			if (ss->wp_primary_selection_source) {
				zwp_primary_selection_source_v1_destroy(ss->wp_primary_selection_source);
				ss->wp_primary_selection_source = nullptr;
			}

			if (ss->wp_primary_selection_offer) {
				memfree(wp_primary_selection_offer_get_offer_state(ss->wp_primary_selection_offer));
				zwp_primary_selection_offer_v1_destroy(ss->wp_primary_selection_offer);
				ss->wp_primary_selection_offer = nullptr;
			}
		}

		return;
	}

	if (name == registry->wp_relative_pointer_manager_name) {
		if (registry->wp_relative_pointer_manager) {
			zwp_relative_pointer_manager_v1_destroy(registry->wp_relative_pointer_manager);
			registry->wp_relative_pointer_manager = nullptr;
		}

		registry->wp_relative_pointer_manager_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wp_relative_pointer) {
				zwp_relative_pointer_v1_destroy(ss->wp_relative_pointer);
				ss->wp_relative_pointer = nullptr;
			}
		}

		return;
	}

	if (name == registry->wp_pointer_constraints_name) {
		if (registry->wp_pointer_constraints) {
			zwp_pointer_constraints_v1_destroy(registry->wp_pointer_constraints);
			registry->wp_pointer_constraints = nullptr;
		}

		registry->wp_pointer_constraints_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wp_relative_pointer) {
				zwp_relative_pointer_v1_destroy(ss->wp_relative_pointer);
				ss->wp_relative_pointer = nullptr;
			}

			if (ss->wp_locked_pointer) {
				zwp_locked_pointer_v1_destroy(ss->wp_locked_pointer);
				ss->wp_locked_pointer = nullptr;
			}

			if (ss->wp_confined_pointer) {
				zwp_confined_pointer_v1_destroy(ss->wp_confined_pointer);
				ss->wp_confined_pointer = nullptr;
			}
		}

		return;
	}

	if (name == registry->wp_pointer_gestures_name) {
		if (registry->wp_pointer_gestures) {
			zwp_pointer_gestures_v1_destroy(registry->wp_pointer_gestures);
		}

		registry->wp_pointer_gestures = nullptr;
		registry->wp_pointer_gestures_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wp_pointer_gesture_pinch) {
				zwp_pointer_gesture_pinch_v1_destroy(ss->wp_pointer_gesture_pinch);
				ss->wp_pointer_gesture_pinch = nullptr;
			}
		}

		return;
	}

	if (name == registry->wp_idle_inhibit_manager_name) {
		if (registry->wp_idle_inhibit_manager) {
			zwp_idle_inhibit_manager_v1_destroy(registry->wp_idle_inhibit_manager);
			registry->wp_idle_inhibit_manager = nullptr;
		}

		registry->wp_idle_inhibit_manager_name = 0;

		return;
	}

	if (name == registry->wp_tablet_manager_name) {
		if (registry->wp_tablet_manager) {
			zwp_tablet_manager_v2_destroy(registry->wp_tablet_manager);
			registry->wp_tablet_manager = nullptr;
		}

		registry->wp_tablet_manager_name = 0;

		// This global is used to create some seat data. Let's clean it.
		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			for (struct zwp_tablet_tool_v2* tool : ss->tablet_tools) {
				TabletToolState* state = wp_tablet_tool_get_state(tool);
				memdelete(state);

				zwp_tablet_tool_v2_destroy(tool);
			}

			ss->tablet_tools.clear();
		}

		return;
	}

	if (name == registry->wp_text_input_manager_name) {
		if (registry->wp_text_input_manager) {
			zwp_text_input_manager_v3_destroy(registry->wp_text_input_manager);
			registry->wp_text_input_manager = nullptr;
		}

		registry->wp_text_input_manager_name = 0;

		for (struct wl_seat* wl_seat : registry->wl_seats) {
			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			zwp_text_input_v3_destroy(ss->wp_text_input);
			ss->wp_text_input = nullptr;
		}

		return;
	}

	if (name == registry->wp_pointer_warp_name) {
		if (registry->wp_pointer_warp) {
			wp_pointer_warp_v1_destroy(registry->wp_pointer_warp);
			registry->wp_pointer_warp = nullptr;
		}

		registry->wp_pointer_warp_name = 0;

		return;
	}

	{
		// Iterate through all of the seats to find if any got removed.
		List<struct wl_seat*>::Element* E = registry->wl_seats.front();
		while (E) {
			struct wl_seat* wl_seat = E->get();
			List<struct wl_seat*>::Element* N = E->next();

			SeatState* ss = wl_seat_get_seat_state(wl_seat);
			ERR_FAIL_NULL(ss);

			if (ss->wl_seat_name == name) {
				if (wl_seat) {
					wl_seat_destroy(wl_seat);
				}

				if (ss->wl_data_device) {
					wl_data_device_destroy(ss->wl_data_device);
				}

				if (ss->wp_tablet_seat) {
					zwp_tablet_seat_v2_destroy(ss->wp_tablet_seat);

					for (struct zwp_tablet_tool_v2* tool : ss->tablet_tools) {
						TabletToolState* state = wp_tablet_tool_get_state(tool);
						memdelete(state);

						zwp_tablet_tool_v2_destroy(tool);
					}
				}

				memdelete(ss);

				registry->wl_seats.erase(E);
				return;
			}

			E = N;
		}
	}

	{
		// Iterate through all of the outputs to find if any got removed.
		// FIXME: This is a very bruteforce approach.
		List<struct wl_output*>::Element* it = registry->wl_outputs.front();
		while (it) {
			// Iterate through all of the screens to find if any got removed.
			struct wl_output* wl_output = it->get();
			ERR_FAIL_NULL(wl_output);

			ScreenState* ss = wl_output_get_screen_state(wl_output);

			if (ss->wl_output_name == name) {
				registry->wl_outputs.erase(it);

				memdelete(ss);
				wl_output_destroy(wl_output);

				return;
			}

			it = it->next();
		}
	}

	if (name == registry->wp_fifo_manager_name) {
		registry->wp_fifo_manager_name = 0;
	}

	if (name == registry->godot_embedding_compositor_name) {
		registry->godot_embedding_compositor_name = 0;

		EmbeddingCompositorState* es =
			godot_embedding_compositor_get_state(registry->godot_embedding_compositor);
		ERR_FAIL_NULL(es);

		es->mapped_clients.clear();

		for (struct godot_embedded_client* client : es->clients) {
			godot_embedded_client_destroy(client);
		}
		es->clients.clear();

		memdelete(es);

		godot_embedding_compositor_destroy(registry->godot_embedding_compositor);
		registry->godot_embedding_compositor = nullptr;
	}
}

void WaylandThread::_wl_surface_on_enter(
	void* data, struct wl_surface* wl_surface, struct wl_output* wl_output)
{
	if (!wl_output || !wl_proxy_is_godot((struct wl_proxy*)wl_output)) {
		// This won't have the right data bound to it. Not worth it and would probably
		// just break everything.
		return;
	}

	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Window entered output %x.", (size_t)wl_output));

	ws->wl_outputs.insert(wl_output);

	// Workaround for buffer scaling as there's no guaranteed way of knowing the
	// preferred scale.
	// TODO: Skip this branch for newer `wl_surface`s once we add support for
	// `wl_surface::preferred_buffer_scale`
	if (ws->preferred_fractional_scale == 0) {
		window_state_update_size(ws, ws->rect.size.width, ws->rect.size.height);
	}
}

void WaylandThread::_frame_wl_callback_on_done(
	void* data, struct wl_callback* wl_callback, uint32_t callback_data)
{
	wl_callback_destroy(wl_callback);

	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);
	ERR_FAIL_NULL(ws->wayland_thread);
	ERR_FAIL_NULL(ws->wl_surface);

	ws->last_frame_time = OS::get_ticks_usec();
	ws->wayland_thread->set_frame();

	ws->frame_callback = wl_surface_frame(ws->wl_surface);
	wl_callback_add_listener(ws->frame_callback, &frame_wl_callback_listener, ws);
}

void WaylandThread::_wl_surface_on_leave(
	void* data, struct wl_surface* wl_surface, struct wl_output* wl_output)
{
	if (!wl_output || !wl_proxy_is_godot((struct wl_proxy*)wl_output)) {
		// This won't have the right data bound to it. Not worth it and would probably
		// just break everything.
		return;
	}

	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->wl_outputs.erase(wl_output);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Window left output %x.\n", (size_t)wl_output));
}

// TODO: Add support to this event.
void WaylandThread::_wl_surface_on_preferred_buffer_scale(
	void* data, struct wl_surface* wl_surface, int32_t factor)
{
}

// TODO: Add support to this event.
void WaylandThread::_wl_surface_on_preferred_buffer_transform(
	void* data, struct wl_surface* wl_surface, uint32_t transform)
{
}

void WaylandThread::_wl_output_on_geometry(void* data, struct wl_output* wl_output, int32_t x,
	int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make,
	const char* model, int32_t transform)
{
	ScreenState* ss = (ScreenState*)data;
	ERR_FAIL_NULL(ss);

	ss->pending_data.position.x = x;

	ss->pending_data.position.x = x;
	ss->pending_data.position.y = y;

	ss->pending_data.physical_size.width = physical_width;
	ss->pending_data.physical_size.height = physical_height;

	ss->pending_data.make.clear();
	ss->pending_data.make.append_utf8(make);
	ss->pending_data.model.clear();
	ss->pending_data.model.append_utf8(model);

	// `wl_output::done` is a version 2 addition. We'll directly update the data
	// for compatibility.
	if (wl_output_get_version(wl_output) == 1) {
		ss->data = ss->pending_data;
	}
}

void WaylandThread::_wl_output_on_mode(void* data, struct wl_output* wl_output, uint32_t flags,
	int32_t width, int32_t height, int32_t refresh)
{
	ScreenState* ss = (ScreenState*)data;
	ERR_FAIL_NULL(ss);

	if (!(flags & WL_OUTPUT_MODE_CURRENT)) {
		return;
	}

	ss->pending_data.size.width = width;
	ss->pending_data.size.height = height;

	ss->pending_data.refresh_rate = refresh ? refresh / 1000.0f : -1;

	// `wl_output::done` is a version 2 addition. We'll directly update the data
	// for compatibility.
	if (wl_output_get_version(wl_output) == 1) {
		ss->data = ss->pending_data;
	}
}

// NOTE: The following `wl_output` events are only for version 2 onwards, so we
// can assume that they're "atomic" (i.e. rely on the `wl_output::done` event).

void WaylandThread::_wl_output_on_done(void* data, struct wl_output* wl_output)
{
	ScreenState* ss = (ScreenState*)data;
	ERR_FAIL_NULL(ss);

	ss->data = ss->pending_data;

	ss->wayland_thread->_update_scale(ss->data.scale);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Output %x done.", (size_t)wl_output));
}

void WaylandThread::_wl_output_on_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
	ScreenState* ss = (ScreenState*)data;
	ERR_FAIL_NULL(ss);

	ss->pending_data.scale = factor;

	DEBUG_LOG_WAYLAND_THREAD(vformat("Output %x scale %d", (size_t)wl_output, factor));
}

void WaylandThread::_wl_output_on_name(void* data, struct wl_output* wl_output, const char* name) {}

void WaylandThread::_wl_output_on_description(
	void* data, struct wl_output* wl_output, const char* description)
{
}

void WaylandThread::_xdg_wm_base_on_ping(
	void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandThread::_xdg_surface_on_configure(
	void* data, struct xdg_surface* xdg_surface, uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);

	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->ready = true;
	printf("[WAYLAND] _xdg_surface_on_configure called! ws=%p, ready=true\n", ws);

	DEBUG_LOG_WAYLAND_THREAD(vformat("xdg surface on configure rect %s", ws->rect));
}

void WaylandThread::_xdg_toplevel_on_configure(void* data, struct xdg_toplevel* xdg_toplevel,
	int32_t width, int32_t height, struct wl_array* states)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	if (width == 0) {
		width = ws->rect.size.width;
	}

	if (height == 0) {
		height = ws->rect.size.height;
	}

	// Expect the window to be in a plain state. It will get properly set if the
	// compositor reports otherwise below.
	ws->mode = DisplayServerEnums::WINDOW_MODE_WINDOWED;
	ws->maximized = false;
	ws->fullscreen = false;
	ws->resizing = false;
	ws->tiled_left = false;
	ws->tiled_right = false;
	ws->tiled_top = false;
	ws->tiled_bottom = false;
	ws->suspended = false;

	uint32_t* state = nullptr;
	wl_array_for_each(state, states)
	{
		switch (*state) {
		case XDG_TOPLEVEL_STATE_MAXIMIZED: {
			ws->mode = DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
			ws->maximized = true;
		} break;

		case XDG_TOPLEVEL_STATE_FULLSCREEN: {
			ws->mode = DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
			ws->fullscreen = true;
		} break;

		case XDG_TOPLEVEL_STATE_RESIZING: {
			ws->resizing = true;
		} break;

		case XDG_TOPLEVEL_STATE_TILED_LEFT: {
			ws->tiled_left = true;
		} break;

		case XDG_TOPLEVEL_STATE_TILED_RIGHT: {
			ws->tiled_right = true;
		} break;

		case XDG_TOPLEVEL_STATE_TILED_TOP: {
			ws->tiled_top = true;
		} break;

		case XDG_TOPLEVEL_STATE_TILED_BOTTOM: {
			ws->tiled_bottom = true;
		} break;

		case XDG_TOPLEVEL_STATE_SUSPENDED: {
			ws->suspended = true;
		} break;

		default: {
			// We don't care about the other states (for now).
		} break;
		}
	}

	window_state_update_size(ws, width, height);

	DEBUG_LOG_WAYLAND_THREAD(
		vformat("XDG toplevel on configure width %d height %d.", width, height));
}

void WaylandThread::_xdg_toplevel_on_close(void* data, struct xdg_toplevel* xdg_toplevel)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	Ref<WindowEventMessage> msg;
	msg.instantiate();
	msg->id = ws->id;
	msg->event = DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST;
	ws->wayland_thread->push_message(msg);
}

void WaylandThread::_xdg_toplevel_on_configure_bounds(
	void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height)
{
}

void WaylandThread::_xdg_toplevel_on_wm_capabilities(
	void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->can_maximize = false;
	ws->can_fullscreen = false;
	ws->can_minimize = false;

	uint32_t* capability = nullptr;
	wl_array_for_each(capability, capabilities)
	{
		switch (*capability) {
		case XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE: {
			ws->can_maximize = true;
		} break;
		case XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN: {
			ws->can_fullscreen = true;
		} break;

		case XDG_TOPLEVEL_WM_CAPABILITIES_MINIMIZE: {
			ws->can_minimize = true;
		} break;

		default: {
		} break;
		}
	}
}

void WaylandThread::_xdg_popup_on_configure(
	void* data, struct xdg_popup* xdg_popup, int32_t x, int32_t y, int32_t width, int32_t height)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	if (width == 0) {
		width = ws->rect.size.width;
	}

	if (height == 0) {
		height = ws->rect.size.height;
	}

	window_state_update_size(ws, width, height);

	WindowState* parent = ws->wayland_thread->window_get_state(ws->parent_id);
	ERR_FAIL_NULL(parent);

	Point2i pos = Point2i(x, y);
#ifdef LIBDECOR_ENABLED
	if (parent->libdecor_frame) {
		int translated_x = x;
		int translated_y = y;
		libdecor_frame_translate_coordinate(
			parent->libdecor_frame, x, y, &translated_x, &translated_y);

		pos.x = translated_x;
		pos.y = translated_y;
	}
#endif

	// Looks like the position returned here is relative to the parent. We have to
	// accumulate it or there's gonna be a lot of confusion godot-side.
	pos += parent->rect.position;

	if (ws->rect.position != pos) {
		DEBUG_LOG_WAYLAND_THREAD(
			vformat("Repositioning popup %d from %s to %s", ws->id, ws->rect.position, pos));

		double parent_scale = window_state_get_scale_factor(parent);

		ws->rect.position = pos;

		Ref<WindowRectMessage> rect_msg;
		rect_msg.instantiate();
		rect_msg->id = ws->id;
		rect_msg->rect.position = scale_vector2i(ws->rect.position, parent_scale);
		rect_msg->rect.size = scale_vector2i(ws->rect.size, parent_scale);
		rect_msg->buffer_scale = window_state_get_preferred_buffer_scale(ws);

		ws->wayland_thread->push_message(rect_msg);
	}

	DEBUG_LOG_WAYLAND_THREAD(
		vformat("xdg popup on configure x%d y%d w%d h%d", x, y, width, height));
}

void WaylandThread::_xdg_popup_on_popup_done(void* data, struct xdg_popup* xdg_popup)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	Ref<WindowEventMessage> ev_msg;
	ev_msg.instantiate();
	ev_msg->id = ws->id;
	ev_msg->event = DisplayServerEnums::WINDOW_EVENT_FORCE_CLOSE;

	ws->wayland_thread->push_message(ev_msg);
}

void WaylandThread::_xdg_popup_on_repositioned(
	void* data, struct xdg_popup* xdg_popup, uint32_t token)
{
	DEBUG_LOG_WAYLAND_THREAD(vformat("stub xdg popup repositioned %x", token));
}

// NOTE: Deprecated.
void WaylandThread::_xdg_exported_v1_on_handle(
	void* data, zxdg_exported_v1* exported, const char* handle)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->exported_handle = vformat("wayland:%s", String::utf8(handle));
}

void WaylandThread::_xdg_exported_v2_on_handle(
	void* data, zxdg_exported_v2* exported, const char* handle)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->exported_handle = vformat("wayland:%s", String::utf8(handle));
}

void WaylandThread::_xdg_toplevel_decoration_on_configure(
	void* data, struct zxdg_toplevel_decoration_v1* xdg_toplevel_decoration, uint32_t mode)
{
	if (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE) {
#ifdef LIBDECOR_ENABLED
		WARN_PRINT_ONCE("Native client side decorations are not yet supported without libdecor!");
#else
		WARN_PRINT_ONCE("Native client side decorations are not yet supported!");
#endif // LIBDECOR_ENABLED
	}
}

#ifdef LIBDECOR_ENABLED
void WaylandThread::libdecor_on_error(
	struct libdecor* context, enum libdecor_error error, const char* message)
{
	ERR_PRINT(vformat("libdecor error %d: %s", error, message));
}

// NOTE: This is pretty much a reimplementation of _xdg_surface_on_configure
// and _xdg_toplevel_on_configure. Libdecor really likes wrapping everything,
// forcing us to do stuff like this.
void WaylandThread::libdecor_frame_on_configure(
	struct libdecor_frame* frame, struct libdecor_configuration* configuration, void* user_data)
{
	WindowState* ws = (WindowState*)user_data;
	ERR_FAIL_NULL(ws);

	ws->ready = true;

	int width = 0;
	int height = 0;

	ws->pending_libdecor_configuration = configuration;

	if (!libdecor_configuration_get_content_size(configuration, frame, &width, &height)) {
		// The configuration doesn't have a size. We'll use the one already set in the window.
		width = ws->rect.size.width;
		height = ws->rect.size.height;
	}

	ERR_FAIL_COND_MSG(width == 0 || height == 0, "Window has invalid size.");

	libdecor_window_state window_state = LIBDECOR_WINDOW_STATE_NONE;

	// Expect the window to be in a plain state. It will get properly set if the
	// compositor reports otherwise below.
	ws->mode = DisplayServerEnums::WINDOW_MODE_WINDOWED;
	ws->maximized = false;
	ws->fullscreen = false;
	ws->resizing = false;
	ws->tiled_left = false;
	ws->tiled_right = false;
	ws->tiled_top = false;
	ws->tiled_bottom = false;
	ws->suspended = false;

	if (libdecor_configuration_get_window_state(configuration, &window_state)) {
		if (window_state & LIBDECOR_WINDOW_STATE_MAXIMIZED) {
			ws->mode = DisplayServerEnums::WINDOW_MODE_MAXIMIZED;
			ws->maximized = true;
		}

		if (window_state & LIBDECOR_WINDOW_STATE_FULLSCREEN) {
			ws->mode = DisplayServerEnums::WINDOW_MODE_FULLSCREEN;
			ws->fullscreen = true;
		}

		// libdecor doesn't have the resizing state for whatever reason.

		if (window_state & LIBDECOR_WINDOW_STATE_TILED_LEFT) {
			ws->tiled_left = true;
		}

		if (window_state & LIBDECOR_WINDOW_STATE_TILED_RIGHT) {
			ws->tiled_right = true;
		}

		if (window_state & LIBDECOR_WINDOW_STATE_TILED_TOP) {
			ws->tiled_top = true;
		}

		if (window_state & LIBDECOR_WINDOW_STATE_TILED_BOTTOM) {
			ws->tiled_bottom = true;
		}

		if (window_state & LIBDECOR_WINDOW_STATE_SUSPENDED) {
			ws->suspended = true;
		}
	}

	window_state_update_size(ws, width, height);

	DEBUG_LOG_WAYLAND_THREAD(vformat("libdecor frame on configure rect %s", ws->rect));
}

void WaylandThread::libdecor_frame_on_close(struct libdecor_frame* frame, void* user_data)
{
	WindowState* ws = (WindowState*)user_data;
	ERR_FAIL_NULL(ws);

	Ref<WindowEventMessage> winevent_msg;
	winevent_msg.instantiate();
	winevent_msg->id = ws->id;
	winevent_msg->event = DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST;

	ws->wayland_thread->push_message(winevent_msg);

	DEBUG_LOG_WAYLAND_THREAD("libdecor frame on close");
}

void WaylandThread::libdecor_frame_on_commit(struct libdecor_frame* frame, void* user_data)
{
	// We're skipping this as we don't really care about libdecor's commit for
	// atomicity reasons. See `_frame_wl_callback_on_done` for more info.

	DEBUG_LOG_WAYLAND_THREAD("libdecor frame on commit");
}

void WaylandThread::libdecor_frame_on_dismiss_popup(
	struct libdecor_frame* frame, const char* seat_name, void* user_data)
{
}
#endif // LIBDECOR_ENABLED

void WaylandThread::_wl_seat_on_capabilities(
	void* data, struct wl_seat* wl_seat, uint32_t capabilities)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	// Pointer handling.
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		if (!ss->wl_pointer) {
			ss->cursor_surface = wl_compositor_create_surface(ss->registry->wl_compositor);
			wl_surface_commit(ss->cursor_surface);

			ss->wl_pointer = wl_seat_get_pointer(wl_seat);
			wl_pointer_add_listener(ss->wl_pointer, &wl_pointer_listener, ss);

			if (ss->registry->wp_cursor_shape_manager) {
				ss->wp_cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(
					ss->registry->wp_cursor_shape_manager, ss->wl_pointer);
			}

			if (ss->registry->wp_relative_pointer_manager) {
				ss->wp_relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
					ss->registry->wp_relative_pointer_manager, ss->wl_pointer);
				zwp_relative_pointer_v1_add_listener(
					ss->wp_relative_pointer, &wp_relative_pointer_listener, ss);
			}

			if (ss->registry->wp_pointer_gestures) {
				ss->wp_pointer_gesture_pinch = zwp_pointer_gestures_v1_get_pinch_gesture(
					ss->registry->wp_pointer_gestures, ss->wl_pointer);
				zwp_pointer_gesture_pinch_v1_add_listener(
					ss->wp_pointer_gesture_pinch, &wp_pointer_gesture_pinch_listener, ss);
			}

			// TODO: Constrain new pointers if the global mouse mode is constrained.
		}
	}
	else {
		if (ss->cursor_frame_callback) {
			// Just in case. I got bitten by weird race-like conditions already.
			wl_callback_set_user_data(ss->cursor_frame_callback, nullptr);

			wl_callback_destroy(ss->cursor_frame_callback);
			ss->cursor_frame_callback = nullptr;
		}

		if (ss->cursor_surface) {
			wl_surface_destroy(ss->cursor_surface);
			ss->cursor_surface = nullptr;
		}

		if (ss->wl_pointer) {
			wl_pointer_destroy(ss->wl_pointer);
			ss->wl_pointer = nullptr;
		}

		if (ss->wp_cursor_shape_device) {
			wp_cursor_shape_device_v1_destroy(ss->wp_cursor_shape_device);
			ss->wp_cursor_shape_device = nullptr;
		}

		if (ss->wp_relative_pointer) {
			zwp_relative_pointer_v1_destroy(ss->wp_relative_pointer);
			ss->wp_relative_pointer = nullptr;
		}

		if (ss->wp_confined_pointer) {
			zwp_confined_pointer_v1_destroy(ss->wp_confined_pointer);
			ss->wp_confined_pointer = nullptr;
		}

		if (ss->wp_locked_pointer) {
			zwp_locked_pointer_v1_destroy(ss->wp_locked_pointer);
			ss->wp_locked_pointer = nullptr;
		}
	}

	// Keyboard handling.
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		if (!ss->wl_keyboard) {
			ss->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
			ERR_FAIL_NULL(ss->xkb_context);

			ss->wl_keyboard = wl_seat_get_keyboard(wl_seat);
			wl_keyboard_add_listener(ss->wl_keyboard, &wl_keyboard_listener, ss);
		}
	}
	else {
		if (ss->xkb_context) {
			xkb_context_unref(ss->xkb_context);
			ss->xkb_context = nullptr;
		}

		if (ss->xkb_compose_table) {
			xkb_compose_table_unref(ss->xkb_compose_table);
			ss->xkb_compose_table = nullptr;
		}

		if (ss->xkb_compose_state) {
			xkb_compose_state_unref(ss->xkb_compose_state);
			ss->xkb_compose_state = nullptr;
		}

		if (ss->xkb_keymap) {
			xkb_keymap_unref(ss->xkb_keymap);
			ss->xkb_keymap = nullptr;
		}

		if (ss->xkb_state) {
			xkb_state_unref(ss->xkb_state);
			ss->xkb_state = nullptr;
		}

		if (ss->wl_keyboard) {
			wl_keyboard_destroy(ss->wl_keyboard);
			ss->wl_keyboard = nullptr;
		}
	}

	if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
		if (!ss->wl_touch) {
			ss->wl_touch = wl_seat_get_touch(wl_seat);
			wl_touch_add_listener(ss->wl_touch, &wl_touch_listener, ss);
		}

		wayland_thread->has_touch = true;
	}
	else {
		if (ss->wl_touch) {
			wl_touch_destroy(ss->wl_touch);
			ss->wl_touch = nullptr;
		}

		wayland_thread->has_touch = false;
		for (struct wl_seat* i : wayland_thread->registry.wl_seats) {
			SeatState* i_ss = wl_seat_get_seat_state(i);
			ERR_CONTINUE(i_ss == nullptr);

			if (i_ss->wl_touch != nullptr) {
				wayland_thread->has_touch = true;
				break;
			}
		}
	}
}

void WaylandThread::_wl_seat_on_name(void* data, struct wl_seat* wl_seat, const char* name) {}

void WaylandThread::_cursor_frame_callback_on_done(
	void* data, struct wl_callback* wl_callback, uint32_t time_ms)
{
	wl_callback_destroy(wl_callback);

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->cursor_frame_callback = nullptr;

	ss->cursor_time_ms = time_ms;

	seat_state_update_cursor(ss);
}

void WaylandThread::_wl_pointer_on_enter(void* data, struct wl_pointer* wl_pointer, uint32_t serial,
	struct wl_surface* surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ERR_FAIL_NULL(ss->cursor_surface);

	PointerData& pd = ss->pointer_data_buffer;

	ss->pointer_enter_serial = serial;
	pd.pointed_id = ws->id;
	pd.last_pointed_id = ws->id;
	pd.position.x = wl_fixed_to_double(surface_x);
	pd.position.y = wl_fixed_to_double(surface_y);

	seat_state_update_cursor(ss);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Pointer entered window %d.", ws->id));

	if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
		_wl_pointer_on_frame(data, wl_pointer);
	}
}

void WaylandThread::_wl_pointer_on_leave(
	void* data, struct wl_pointer* wl_pointer, uint32_t serial, struct wl_surface* surface)
{
	// NOTE: `surface` will probably be null when the surface is destroyed.
	// See: https://gitlab.freedesktop.org/wayland/wayland/-/issues/366
	// See: https://gitlab.freedesktop.org/wayland/wayland/-/issues/465

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	if (pd.pointed_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		// We're probably on a decoration or some other third-party thing.
		return;
	}

	DisplayServerEnums::WindowID id = pd.pointed_id;

	pd.pointed_id = DisplayServerEnums::INVALID_WINDOW_ID;

	DEBUG_LOG_WAYLAND_THREAD(vformat("Pointer left window %d.", id));

	if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
		_wl_pointer_on_frame(data, wl_pointer);
	}
}

void WaylandThread::_wl_pointer_on_motion(void* data, struct wl_pointer* wl_pointer, uint32_t time,
	wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	pd.position.x = wl_fixed_to_double(surface_x);
	pd.position.y = wl_fixed_to_double(surface_y);

	pd.motion_time = time;

	if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
		_wl_pointer_on_frame(data, wl_pointer);
	}
}

void WaylandThread::_wl_pointer_on_frame(void* data, struct wl_pointer* wl_pointer) {}

void WaylandThread::_wl_pointer_on_axis(
	void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	switch (axis) {
	case WL_POINTER_AXIS_VERTICAL_SCROLL: {
		pd.scroll_vector.y = wl_fixed_to_double(value);
	} break;

	case WL_POINTER_AXIS_HORIZONTAL_SCROLL: {
		pd.scroll_vector.x = wl_fixed_to_double(value);
	} break;
	}

	pd.button_time = time;

	if (wl_pointer_get_version(wl_pointer) < WL_POINTER_FRAME_SINCE_VERSION) {
		_wl_pointer_on_frame(data, wl_pointer);
	}
}

void WaylandThread::_wl_pointer_on_axis_source(
	void* data, struct wl_pointer* wl_pointer, uint32_t axis_source)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->pointer_data_buffer.scroll_type = axis_source;
}

void WaylandThread::_wl_pointer_on_axis_stop(
	void* data, struct wl_pointer* wl_pointer, uint32_t time, uint32_t axis)
{
}

// NOTE: This event is deprecated since version 8 and superseded by
// `wl_pointer::axis_value120`. This thus converts the data to its
// fraction-of-120 format.
void WaylandThread::_wl_pointer_on_axis_discrete(
	void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t discrete)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	// NOTE: We can allow ourselves to not accumulate this data (and thus just
	// assign it) as the spec guarantees only one event per axis type.

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		pd.discrete_scroll_vector_120.y = discrete * 120;
	}

	if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
		pd.discrete_scroll_vector_120.x = discrete * 120;
	}
}

// Supersedes `wl_pointer::axis_discrete` Since version 8.
void WaylandThread::_wl_pointer_on_axis_value120(
	void* data, struct wl_pointer* wl_pointer, uint32_t axis, int32_t value120)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		pd.discrete_scroll_vector_120.y += value120;
	}

	if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
		pd.discrete_scroll_vector_120.x += value120;
	}
}

// TODO: Add support to this event.
void WaylandThread::_wl_pointer_on_axis_relative_direction(
	void* data, struct wl_pointer* wl_pointer, uint32_t axis, uint32_t direction)
{
}

void WaylandThread::_wl_keyboard_on_keymap(
	void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
	ERR_FAIL_COND_MSG(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
		"Unsupported keymap format announced from the Wayland compositor.");

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (ss->keymap_buffer) {
		// We have already a mapped buffer, so we unmap it. There's no need to reset
		// its pointer or size, as we're gonna set them below.
		munmap((void*)ss->keymap_buffer, ss->keymap_buffer_size);
		ss->keymap_buffer = nullptr;
	}

	ss->keymap_buffer = (const char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
	ss->keymap_buffer_size = size;

	xkb_keymap_unref(ss->xkb_keymap);
	ss->xkb_keymap = xkb_keymap_new_from_string(
		ss->xkb_context, ss->keymap_buffer, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

	xkb_state_unref(ss->xkb_state);
	ss->xkb_state = xkb_state_new(ss->xkb_keymap);

	xkb_compose_table_unref(ss->xkb_compose_table);
	const char* locale = getenv("LC_ALL");
	if (!locale || !*locale) {
		locale = getenv("LC_CTYPE");
	}
	if (!locale || !*locale) {
		locale = getenv("LANG");
	}
	if (!locale || !*locale) {
		locale = "C";
	}
	ss->xkb_compose_table =
		xkb_compose_table_new_from_locale(ss->xkb_context, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);

	xkb_compose_state_unref(ss->xkb_compose_state);
	ss->xkb_compose_state =
		xkb_compose_state_new(ss->xkb_compose_table, XKB_COMPOSE_STATE_NO_FLAGS);

	xkb_state_update_mask(ss->xkb_state, ss->mods_depressed, ss->mods_latched, ss->mods_locked, 0,
		0, ss->current_layout_index);
}

void WaylandThread::_wl_keyboard_on_enter(void* data, struct wl_keyboard* wl_keyboard,
	uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	ss->focused_id = ws->id;

	wayland_thread->_set_current_seat(ss->wl_seat);

	Ref<WindowEventMessage> msg;
	msg.instantiate();
	msg->id = ws->id;
	msg->event = DisplayServerEnums::WINDOW_EVENT_FOCUS_IN;
	wayland_thread->push_message(msg);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Keyboard focused window %d.", ws->id));
}

void WaylandThread::_wl_keyboard_on_leave(
	void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface)
{
	// NOTE: `surface` will probably be null when the surface is destroyed.
	// See: https://gitlab.freedesktop.org/wayland/wayland/-/issues/366
	// See: https://gitlab.freedesktop.org/wayland/wayland/-/issues/465

	if (surface && !wl_proxy_is_godot((struct wl_proxy*)surface)) {
		return;
	}

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	ss->repeating_keycode = XKB_KEYCODE_INVALID;

	if (ss->focused_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		// We're probably on a decoration or some other third-party thing.
		return;
	}

	WindowState* ws = wayland_thread->window_get_state(ss->focused_id);
	ERR_FAIL_NULL(ws);

	ss->focused_id = DisplayServerEnums::INVALID_WINDOW_ID;

	Ref<WindowEventMessage> msg;
	msg.instantiate();
	msg->id = ws->id;
	msg->event = DisplayServerEnums::WINDOW_EVENT_FOCUS_OUT;
	wayland_thread->push_message(msg);

	ss->shift_pressed = false;
	ss->ctrl_pressed = false;
	ss->alt_pressed = false;
	ss->meta_pressed = false;

	if (ss->xkb_state != nullptr) {
		xkb_state_update_mask(ss->xkb_state, 0, 0, 0, 0, 0, 0);
	}

	DEBUG_LOG_WAYLAND_THREAD(vformat("Keyboard unfocused window %d.", ws->id));
}

void WaylandThread::_wl_keyboard_on_key(void* data, struct wl_keyboard* wl_keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (ss->focused_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	// We have to add 8 to the scancode to get an XKB-compatible keycode.
	xkb_keycode_t xkb_keycode = key + 8;

	bool pressed = state & WL_KEYBOARD_KEY_STATE_PRESSED;

	if (pressed) {
		if (xkb_keymap_key_repeats(ss->xkb_keymap, xkb_keycode)) {
			ss->last_repeat_start_msec = OS::get_ticks_msec();
			ss->repeating_keycode = xkb_keycode;
		}

		ss->last_key_pressed_serial = serial;
	}
	else if (ss->repeating_keycode == xkb_keycode) {
		ss->repeating_keycode = XKB_KEYCODE_INVALID;
	}

	_seat_state_handle_xkb_keycode(ss, xkb_keycode, pressed);
}

void WaylandThread::_wl_keyboard_on_modifiers(void* data, struct wl_keyboard* wl_keyboard,
	uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
	uint32_t group)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->mods_depressed = mods_depressed;
	ss->mods_latched = mods_latched;
	ss->mods_locked = mods_locked;
	ss->current_layout_index = group;

	if (ss->xkb_state == nullptr) {
		return;
	}

	xkb_state_update_mask(ss->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

	ss->shift_pressed =
		xkb_state_mod_name_is_active(ss->xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE);
	ss->ctrl_pressed =
		xkb_state_mod_name_is_active(ss->xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
	ss->alt_pressed =
		xkb_state_mod_name_is_active(ss->xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE);
	ss->meta_pressed =
		xkb_state_mod_name_is_active(ss->xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE);
}

void WaylandThread::_wl_keyboard_on_repeat_info(
	void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->repeat_key_delay_msec = rate ? 1000 / rate : 0;
	ss->repeat_start_delay_msec = delay;
}

void WaylandThread::_wl_touch_on_down(void* data, struct wl_touch* wl_touch, uint32_t serial,
	uint32_t time, struct wl_surface* surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->last_touch_id = id;

	TouchPoint& tp = ss->touch_points_buffer[id];

	tp.down_time = time;

	tp.touched_id = ws->id;

	tp.position.x = wl_fixed_to_double(x);
	tp.position.y = wl_fixed_to_double(y);
}

void WaylandThread::_wl_touch_on_up(
	void* data, struct wl_touch* wl_touch, uint32_t serial, uint32_t time, int32_t id)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (!ss->touch_points_buffer.has(id)) {
		return;
	}

	TouchPoint& tp = ss->touch_points_buffer[id];

	tp.touched_id = DisplayServerEnums::INVALID_WINDOW_ID;
	ss->touch_points_buffer.erase(id);

	// Some compositors (e.g. mutter) do not send a frame event after "up" events.
	// AFAICT, this is an acceptable interpretation of the protocol ("No further
	// events will be sent for this touch point[...]"). Let's just emulate it here.
	_wl_touch_on_frame(data, wl_touch);
}

void WaylandThread::_wl_touch_on_motion(
	void* data, struct wl_touch* wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->last_touch_id = id;

	TouchPoint& tp = ss->touch_points_buffer[id];

	tp.motion_time = time;

	tp.position.x = wl_fixed_to_double(x);
	tp.position.y = wl_fixed_to_double(y);
}

void WaylandThread::_wl_touch_on_frame(void* data, struct wl_touch* wl_touch)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	bool hover_changed = false;

	// Release handling.
	for (KeyValue<int32_t, TouchPoint>& pair : ss->touch_points) {
		int32_t id = pair.key;
		TouchPoint& tp = pair.value;

		WindowState* ws = wayland_thread->window_get_state(tp.touched_id);
		if (tp.touched_id == DisplayServerEnums::INVALID_WINDOW_ID || ws == nullptr) {
			continue;
		}

		double scale = window_state_get_scale_factor(ws);

		if (!ss->touch_points_buffer.has(id)) {
			hover_changed = true;

			InputEvent st = InputEvent::make_screen_touch();
			st.touch.index = id;
			st.touch.position = tp.position * scale;
			st.window_id = tp.touched_id;
			st.pressed = false;

			Ref<InputEventMessage> iev_msg;
			iev_msg.instantiate();
			iev_msg->event = st;

			wayland_thread->push_message(iev_msg);
		}
	}

	for (KeyValue<int32_t, TouchPoint>& pair : ss->touch_points_buffer) {
		int32_t id = pair.key;
		TouchPoint& tp = pair.value;

		WindowState* ws = wayland_thread->window_get_state(tp.touched_id);
		if (tp.touched_id == DisplayServerEnums::INVALID_WINDOW_ID || ws == nullptr) {
			continue;
		}

		double scale = window_state_get_scale_factor(ws);

		// Press handling.
		if (!ss->touch_points.has(id)) {
			hover_changed = true;

			InputEvent st = InputEvent::make_screen_touch();
			st.touch.index = id;
			st.touch.position = tp.position * scale;
			st.window_id = tp.touched_id;
			st.pressed = true;

			Ref<InputEventMessage> iev_msg;
			iev_msg.instantiate();
			iev_msg->event = st;

			wayland_thread->push_message(iev_msg);

			continue;
		}

		TouchPoint& tp_old = ss->touch_points[id];

		if (tp.touched_id == DisplayServerEnums::INVALID_WINDOW_ID) {
			ws = wayland_thread->window_get_state(tp.touched_id);
		}

		if (wayland_thread->window_get_state(tp.touched_id) == nullptr) {
			continue;
		}

		if (tp.position != tp_old.position) {
			InputEvent sd = InputEvent::make_screen_drag();
			sd.screen_drag.index = id;
			sd.window_id = tp.touched_id;
			sd.screen_drag.position = tp.position * scale;
			sd.screen_drag.relative = (tp.position - tp_old.position) * scale;
			sd.screen_drag.screen_relative = sd.screen_drag.relative;

			Ref<InputEventMessage> iev_msg;
			iev_msg.instantiate();
			iev_msg->event = sd;

			wayland_thread->push_message(iev_msg);
		}
	}

	ss->touch_points = ss->touch_points_buffer;

	if (hover_changed) {
		wayland_thread->_window_hover();
	}
}

// NOTE: Per the spec, a frame event is not required after this event, so let's
// assume that it's not going to happen.
void WaylandThread::_wl_touch_on_cancel(void* data, struct wl_touch* wl_touch)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	for (KeyValue<int32_t, TouchPoint>& pair : ss->touch_points_buffer) {
		int32_t id = pair.key;
		TouchPoint& tp = pair.value;

		WindowState* ws = wayland_thread->window_get_state(tp.touched_id);
		double scale = window_state_get_scale_factor(ws);

		InputEvent st = InputEvent::make_screen_touch();
		st.touch.index = id;
		st.touch.position = tp.position * scale;
		st.window_id = tp.touched_id;
		st.pressed = false;
		st.canceled = true;

		Ref<InputEventMessage> cancel_msg;
		cancel_msg.instantiate();
		cancel_msg->event = st;

		wayland_thread->push_message(cancel_msg);

		wayland_thread->_window_hover(DisplayServerEnums::INVALID_WINDOW_ID);
	}

	ss->touch_points_buffer.clear();
	ss->touch_points.clear();
}

void WaylandThread::_wl_touch_on_shape(
	void* data, struct wl_touch* wl_touch, int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
}

void WaylandThread::_wl_touch_on_orientation(
	void* data, struct wl_touch* wl_touch, int32_t id, wl_fixed_t orientation)
{
}

// NOTE: Don't forget to `memfree` the offer's state.
void WaylandThread::_wl_data_device_on_data_offer(
	void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id)
{
	wl_proxy_tag_godot((struct wl_proxy*)id);
	wl_data_offer_add_listener(id, &wl_data_offer_listener, memnew(OfferState));
}

void WaylandThread::_wl_data_device_on_enter(void* data, struct wl_data_device* wl_data_device,
	uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
	struct wl_data_offer* id)
{
	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->dnd_id = ws->id;

	ss->dnd_enter_serial = serial;
	ss->wl_data_offer_dnd = id;

	// Godot only supports DnD file copying for now.
	wl_data_offer_accept(id, serial, "text/uri-list");
	wl_data_offer_set_actions(
		id, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
}

void WaylandThread::_wl_data_device_on_leave(void* data, struct wl_data_device* wl_data_device)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (ss->wl_data_offer_dnd) {
		memdelete(wl_data_offer_get_offer_state(ss->wl_data_offer_dnd));
		wl_data_offer_destroy(ss->wl_data_offer_dnd);
		ss->wl_data_offer_dnd = nullptr;
		ss->dnd_id = DisplayServerEnums::INVALID_WINDOW_ID;
	}
}

void WaylandThread::_wl_data_device_on_motion(
	void* data, struct wl_data_device* wl_data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
}

void WaylandThread::_wl_data_device_on_drop(void* data, struct wl_data_device* wl_data_device)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	OfferState* os = wl_data_offer_get_offer_state(ss->wl_data_offer_dnd);
	ERR_FAIL_NULL(os);

	if (os) {
		Ref<DropFilesEventMessage> msg;
		msg.instantiate();
		msg->id = ss->dnd_id;

		Vector<uint8_t> list_data =
			_wl_data_offer_read(wayland_thread->wl_display, "text/uri-list", ss->wl_data_offer_dnd);

		msg->files =
			String::utf8((const char*)list_data.ptr(), list_data.size()).split("\r\n", false);
		for (int i = 0; i < msg->files.size(); i++) {
			msg->files.write[i] = msg->files[i].replace("file://", "").uri_file_decode();
		}

		wayland_thread->push_message(msg);

		wl_data_offer_finish(ss->wl_data_offer_dnd);
	}

	memdelete(wl_data_offer_get_offer_state(ss->wl_data_offer_dnd));
	wl_data_offer_destroy(ss->wl_data_offer_dnd);
	ss->wl_data_offer_dnd = nullptr;
	ss->dnd_id = DisplayServerEnums::INVALID_WINDOW_ID;
}

void WaylandThread::_wl_data_device_on_selection(
	void* data, struct wl_data_device* wl_data_device, struct wl_data_offer* id)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (ss->wl_data_offer_selection) {
		memdelete(wl_data_offer_get_offer_state(ss->wl_data_offer_selection));
		wl_data_offer_destroy(ss->wl_data_offer_selection);
	}

	ss->wl_data_offer_selection = id;
}

void WaylandThread::_wl_data_offer_on_offer(
	void* data, struct wl_data_offer* wl_data_offer, const char* mime_type)
{
	OfferState* os = (OfferState*)data;
	ERR_FAIL_NULL(os);

	if (os) {
		os->mime_types.insert(String::utf8(mime_type));
	}
}

void WaylandThread::_wl_data_offer_on_source_actions(
	void* data, struct wl_data_offer* wl_data_offer, uint32_t source_actions)
{
}

void WaylandThread::_wl_data_offer_on_action(
	void* data, struct wl_data_offer* wl_data_offer, uint32_t dnd_action)
{
}

void WaylandThread::_wl_data_source_on_target(
	void* data, struct wl_data_source* wl_data_source, const char* mime_type)
{
}

void WaylandThread::_wl_data_source_on_send(
	void* data, struct wl_data_source* wl_data_source, const char* mime_type, int32_t fd)
{
	SeatState* ss = (SeatState*)data;
	if (ss == nullptr) {
		ERR_PRINT("Seat state not set.");
		close(fd);
		return;
	}

	if (wl_data_source == ss->wl_data_source_selection) {
		DEBUG_LOG_WAYLAND_THREAD("Clipboard: requested selection.");
		_clipboard_send(ss->selection_data, mime_type, fd);
	}

	close(fd);
}

void WaylandThread::_wl_data_source_on_cancelled(void* data, struct wl_data_source* wl_data_source)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	wl_data_source_destroy(wl_data_source);

	if (wl_data_source == ss->wl_data_source_selection) {
		ss->wl_data_source_selection = nullptr;
		ss->selection_data.clear();

		DEBUG_LOG_WAYLAND_THREAD("Clipboard: selection set by another program.");
		return;
	}
}

void WaylandThread::_wl_data_source_on_dnd_drop_performed(
	void* data, struct wl_data_source* wl_data_source)
{
}

void WaylandThread::_wl_data_source_on_dnd_finished(
	void* data, struct wl_data_source* wl_data_source)
{
}

void WaylandThread::_wl_data_source_on_action(
	void* data, struct wl_data_source* wl_data_source, uint32_t dnd_action)
{
}

void WaylandThread::_zwp_locked_pointer_v1_on_locked(
	void* data, struct zwp_locked_pointer_v1* zwp_locked_pointer_v1)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->pointer_locked = true;

	// Maybe by the time we lock the legacy constraint warp has already been
	// committed. Let's give it a shot.
	if (ss->constraint_warp_committed) {
		PointerConstraint old_constraint = ss->wayland_thread->pointer_constraint;
		ss->wayland_thread->pointer_set_constraint(PointerConstraint::NONE);
		ss->wayland_thread->pointer_set_constraint(old_constraint);

		ss->pointer_locked = false;
		ss->constraint_warping = false;
		ss->constraint_warp_committed = false;
	}
}

void WaylandThread::_zwp_locked_pointer_v1_on_unlocked(
	void* data, struct zwp_locked_pointer_v1* zwp_locked_pointer_v1)
{
}

void WaylandThread::_wp_color_manager_on_supported_intent(
	void* data, struct wp_color_manager_v1* wp_color_manager_v1, uint32_t render_intent)
{
	ColorManagementState* cms = (ColorManagementState*)data;
	ERR_FAIL_NULL(cms);

	cms->supported_render_intents |= 1 << render_intent;
}

void WaylandThread::_wp_color_manager_on_supported_feature(
	void* data, struct wp_color_manager_v1* wp_color_manager_v1, uint32_t feature)
{
	ColorManagementState* cms = (ColorManagementState*)data;
	ERR_FAIL_NULL(cms);

	cms->supported_render_feature |= 1 << feature;
}

void WaylandThread::_wp_color_manager_on_supported_tf_named(
	void* data, struct wp_color_manager_v1* wp_color_manager_v1, uint32_t tf)
{
	ColorManagementState* cms = (ColorManagementState*)data;
	ERR_FAIL_NULL(cms);

	cms->supported_transfer_function |= 1 << tf;
}

void WaylandThread::_wp_color_manager_on_supported_primaries_named(
	void* data, struct wp_color_manager_v1* wp_color_manager_v1, uint32_t primaries)
{
	ColorManagementState* cms = (ColorManagementState*)data;
	ERR_FAIL_NULL(cms);

	cms->supported_primaries |= 1 << primaries;
}

void WaylandThread::_wp_color_manager_on_done(
	void* data, struct wp_color_manager_v1* wp_color_manager_v1)
{
	ColorManagementState* cms = (ColorManagementState*)data;
	ERR_FAIL_NULL(cms);

	// We require parametric profiles until we can support reading ICC files.
	if ((cms->supported_render_feature & (1 << WP_COLOR_MANAGER_V1_FEATURE_PARAMETRIC)) == 0) {
		return;
	}

	// We require the compositor to support extended linear sRGB.
	// sRGB primaries support is assumed.
	cms->supports_hdr =
		cms->supported_transfer_function & (1 << WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_EXT_LINEAR);
}

void WaylandThread::_wp_color_management_surface_feedback_on_preferred_changed(void* data,
	struct wp_color_management_surface_feedback_v1* wp_color_management_surface_feedback_v1,
	uint32_t identity)
{
	_wp_color_management_surface_feedback_on_preferred_changed2(
		data, wp_color_management_surface_feedback_v1, 0, identity);
}

void WaylandThread::_wp_color_management_surface_feedback_on_preferred_changed2(void* data,
	struct wp_color_management_surface_feedback_v1* wp_color_management_surface_feedback_v1,
	uint32_t identity_high, uint32_t identity_low)
{
	struct wp_image_description_v1* image_description =
		wp_color_management_surface_feedback_v1_get_preferred_parametric(
			wp_color_management_surface_feedback_v1);

	wp_image_description_v1_add_listener(image_description, &wp_image_description_listener, data);
}

void WaylandThread::_wp_image_description_on_failed(
	void* data, struct wp_image_description_v1* image_descrptor, uint32_t cause, const char* msg)
{
	WARN_PRINT(msg);
}

void WaylandThread::_wp_image_description_on_ready(
	void* data, struct wp_image_description_v1* image_descriptor, uint32_t identity)
{
	_wp_image_description_on_ready2(data, image_descriptor, 0, identity);
}

void WaylandThread::_wp_image_description_on_ready2(void* data,
	struct wp_image_description_v1* image_descriptor, uint32_t identity_high, uint32_t identity_low)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	struct wp_image_description_info_v1* image_info =
		wp_image_description_v1_get_information(image_descriptor);
	if (image_info != nullptr) {
		// The wp_image_description_info_v1 listener takes ownership of this msg.
		// We need to add a virtual reference so this msg is not freed when we leave the scope.
		Ref<ColorProfileMessage> msg = memnew(ColorProfileMessage);
		msg->reference();
		msg->id = ws->id;
		msg->wayland_thread = ws->wayland_thread;

		wp_image_description_info_v1_add_listener(
			image_info, &wp_image_description_info_listener, msg.ptr());
		wp_image_description_v1_destroy(image_descriptor);
	}
}

void WaylandThread::_wp_image_description_info_on_done(
	void* data, struct wp_image_description_info_v1* wp_image_description_info_v1)
{
	wp_image_description_info_v1_destroy(wp_image_description_info_v1);

	ERR_FAIL_NULL(data);
	// Now that we have claimed ownership of the msg, remove the virtual reference.
	Ref<ColorProfileMessage> msg = (ColorProfileMessage*)data;
	msg->unreference();

	msg->wayland_thread->push_message(msg);
}

void WaylandThread::_wp_image_description_info_on_icc_file(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, int32_t icc,
	uint32_t icc_size)
{
	::close(icc);
}

void WaylandThread::_wp_image_description_info_on_primaries(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, int32_t r_x, int32_t r_y,
	int32_t g_x, int32_t g_y, int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y)
{
}

void WaylandThread::_wp_image_description_info_on_primaries_named(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t primaries)
{
	ColorProfileMessage* msg = (ColorProfileMessage*)data;
	ERR_FAIL_NULL(msg);

	msg->color_profile.named_primary = primaries;
}

void WaylandThread::_wp_image_description_info_on_tf_power(
	void* data, struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t eexp)
{
}

void WaylandThread::_wp_image_description_info_on_tf_named(
	void* data, struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t tf)
{
	ColorProfileMessage* msg = (ColorProfileMessage*)data;
	ERR_FAIL_NULL(msg);

	msg->color_profile.named_transfer_function = tf;
}

void WaylandThread::_wp_image_description_info_on_luminances(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t min_lum,
	uint32_t max_lum, uint32_t reference_lum)
{
	ColorProfileMessage* msg = (ColorProfileMessage*)data;
	ERR_FAIL_NULL(msg);

	msg->color_profile.reference_luminance = reference_lum;
}

void WaylandThread::_wp_image_description_info_on_target_primaries(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, int32_t r_x, int32_t r_y,
	int32_t g_x, int32_t g_y, int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y)
{
}

void WaylandThread::_wp_image_description_info_on_target_luminance(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t min_lum,
	uint32_t max_lum)
{
	ColorProfileMessage* msg = (ColorProfileMessage*)data;
	ERR_FAIL_NULL(msg);

	// The uint32 is multiplied by 10000 for precision.
	msg->color_profile.target_min_luminance = static_cast<float>(min_lum) / 10000;
	msg->color_profile.target_max_luminance = max_lum;
}

void WaylandThread::_wp_image_description_info_on_target_max_cll(
	void* data, struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t max_cll)
{
}

void WaylandThread::_wp_image_description_info_on_target_max_fall(void* data,
	struct wp_image_description_info_v1* wp_image_description_info_v1, uint32_t max_fall)
{
}

void WaylandThread::_wp_fractional_scale_on_preferred_scale(
	void* data, struct wp_fractional_scale_v1* wp_fractional_scale_v1, uint32_t scale)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);

	ws->preferred_fractional_scale = (double)scale / 120;

	window_state_update_size(ws, ws->rect.size.width, ws->rect.size.height);
}

void WaylandThread::_wp_relative_pointer_on_relative_motion(void* data,
	struct zwp_relative_pointer_v1* wp_relative_pointer, uint32_t uptime_hi, uint32_t uptime_lo,
	wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel, wl_fixed_t dy_unaccel)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	PointerData& pd = ss->pointer_data_buffer;

	pd.relative_motion.x = wl_fixed_to_double(dx);
	pd.relative_motion.y = wl_fixed_to_double(dy);

	pd.relative_motion_time = uptime_lo;
}

void WaylandThread::_wp_pointer_gesture_pinch_on_begin(void* data,
	struct zwp_pointer_gesture_pinch_v1* wp_pointer_gesture_pinch_v1, uint32_t serial,
	uint32_t time, struct wl_surface* surface, uint32_t fingers)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (fingers == 2) {
		ss->old_pinch_scale = wl_fixed_from_int(1);
		ss->active_gesture = Gesture::MAGNIFY;
	}
}

void WaylandThread::_wp_pointer_gesture_pinch_on_update(void* data,
	struct zwp_pointer_gesture_pinch_v1* wp_pointer_gesture_pinch_v1, uint32_t time, wl_fixed_t dx,
	wl_fixed_t dy, wl_fixed_t scale, wl_fixed_t rotation)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	// NOTE: From what I can tell, this and all other pointer gestures are separate
	// from the "frame" mechanism of regular pointers. Thus, let's just assume we
	// can read from the "committed" state.
	const PointerData& pd = ss->pointer_data;

	WaylandThread* wayland_thread = ss->wayland_thread;
	ERR_FAIL_NULL(wayland_thread);

	WindowState* ws = wayland_thread->window_get_state(pd.pointed_id);
	ERR_FAIL_NULL(ws);

	double win_scale = window_state_get_scale_factor(ws);

	if (ss->active_gesture == Gesture::MAGNIFY) {
		InputEvent mg = InputEvent::make_magnify_gesture();
		mg.window_id = ws ? ws->id : pd.pointed_id;
		mg.set_shift_pressed(ss->shift_pressed);
		mg.set_ctrl_pressed(ss->ctrl_pressed);
		mg.set_alt_pressed(ss->alt_pressed);
		mg.set_meta_pressed(ss->meta_pressed);
		mg.gesture.position = pd.position * win_scale;

		wl_fixed_t scale_delta = scale - ss->old_pinch_scale;
		mg.gesture.factor = 1 + wl_fixed_to_double(scale_delta);

		Ref<InputEventMessage> magnify_msg;
		magnify_msg.instantiate();
		magnify_msg->event = mg;

		InputEvent pg = InputEvent::make_pan_gesture();
		pg.window_id = ws ? ws->id : pd.pointed_id;
		pg.set_shift_pressed(ss->shift_pressed);
		pg.set_ctrl_pressed(ss->ctrl_pressed);
		pg.set_alt_pressed(ss->alt_pressed);
		pg.set_meta_pressed(ss->meta_pressed);
		pg.gesture.position = pd.position * win_scale;
		pg.gesture.delta = Vector2(wl_fixed_to_double(dx), wl_fixed_to_double(dy));

		Ref<InputEventMessage> pan_msg;
		pan_msg.instantiate();
		pan_msg->event = pg;

		wayland_thread->push_message(magnify_msg);
		wayland_thread->push_message(pan_msg);

		ss->old_pinch_scale = scale;
	}
}

void WaylandThread::_wp_pointer_gesture_pinch_on_end(void* data,
	struct zwp_pointer_gesture_pinch_v1* wp_pointer_gesture_pinch_v1, uint32_t serial,
	uint32_t time, int32_t cancelled)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	ss->active_gesture = Gesture::NONE;
}

// NOTE: Don't forget to `memfree` the offer's state.
void WaylandThread::_wp_primary_selection_device_on_data_offer(void* data,
	struct zwp_primary_selection_device_v1* wp_primary_selection_device_v1,
	struct zwp_primary_selection_offer_v1* offer)
{
	wl_proxy_tag_godot((struct wl_proxy*)offer);
	zwp_primary_selection_offer_v1_add_listener(
		offer, &wp_primary_selection_offer_listener, memnew(OfferState));
}

void WaylandThread::_wp_primary_selection_device_on_selection(void* data,
	struct zwp_primary_selection_device_v1* wp_primary_selection_device_v1,
	struct zwp_primary_selection_offer_v1* id)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (ss->wp_primary_selection_offer) {
		memfree(wp_primary_selection_offer_get_offer_state(ss->wp_primary_selection_offer));
		zwp_primary_selection_offer_v1_destroy(ss->wp_primary_selection_offer);
	}

	ss->wp_primary_selection_offer = id;
}

void WaylandThread::_wp_primary_selection_offer_on_offer(void* data,
	struct zwp_primary_selection_offer_v1* wp_primary_selection_offer_v1, const char* mime_type)
{
	OfferState* os = (OfferState*)data;
	ERR_FAIL_NULL(os);

	if (os) {
		os->mime_types.insert(String::utf8(mime_type));
	}
}

void WaylandThread::_wp_primary_selection_source_on_send(void* data,
	struct zwp_primary_selection_source_v1* wp_primary_selection_source_v1, const char* mime_type,
	int32_t fd)
{
	SeatState* ss = (SeatState*)data;
	if (ss == nullptr) {
		ERR_PRINT("Seat state not set.");
		close(fd);
		return;
	}

	if (wp_primary_selection_source_v1 == ss->wp_primary_selection_source) {
		DEBUG_LOG_WAYLAND_THREAD("Clipboard: requested primary selection.");
		_clipboard_send(ss->primary_data, mime_type, fd);
	}

	close(fd);
}

void WaylandThread::_wp_primary_selection_source_on_cancelled(
	void* data, struct zwp_primary_selection_source_v1* wp_primary_selection_source_v1)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	if (wp_primary_selection_source_v1 == ss->wp_primary_selection_source) {
		zwp_primary_selection_source_v1_destroy(ss->wp_primary_selection_source);
		ss->wp_primary_selection_source = nullptr;

		ss->primary_data.clear();

		DEBUG_LOG_WAYLAND_THREAD("Clipboard: primary selection set by another program.");
		return;
	}
}

void WaylandThread::_wp_tablet_seat_on_tablet_added(
	void* data, struct zwp_tablet_seat_v2* wp_tablet_seat_v2, struct zwp_tablet_v2* id)
{
}

void WaylandThread::_wp_tablet_seat_on_tool_added(
	void* data, struct zwp_tablet_seat_v2* wp_tablet_seat_v2, struct zwp_tablet_tool_v2* id)
{
	SeatState* ss = (SeatState*)data;
	ERR_FAIL_NULL(ss);

	TabletToolState* state = memnew(TabletToolState);
	state->wl_seat = ss->wl_seat;

	wl_proxy_tag_godot((struct wl_proxy*)id);
	zwp_tablet_tool_v2_add_listener(id, &wp_tablet_tool_listener, state);
	ss->tablet_tools.push_back(id);
}

void WaylandThread::_wp_tablet_seat_on_pad_added(
	void* data, struct zwp_tablet_seat_v2* wp_tablet_seat_v2, struct zwp_tablet_pad_v2* id)
{
}

void WaylandThread::_wp_tablet_tool_on_type(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t tool_type)
{
	TabletToolState* state = wp_tablet_tool_get_state(wp_tablet_tool_v2);

	if (state && tool_type == ZWP_TABLET_TOOL_V2_TYPE_ERASER) {
		state->is_eraser = true;
	}
}

void WaylandThread::_wp_tablet_tool_on_hardware_serial(void* data,
	struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t hardware_serial_hi,
	uint32_t hardware_serial_lo)
{
}

void WaylandThread::_wp_tablet_tool_on_hardware_id_wacom(void* data,
	struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t hardware_id_hi, uint32_t hardware_id_lo)
{
}

void WaylandThread::_wp_tablet_tool_on_capability(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t capability)
{
}

void WaylandThread::_wp_tablet_tool_on_done(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2)
{
}

void WaylandThread::_wp_tablet_tool_on_removed(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2)
{
	TabletToolState* ts = wp_tablet_tool_get_state(wp_tablet_tool_v2);
	if (!ts) {
		return;
	}

	SeatState* ss = wl_seat_get_seat_state(ts->wl_seat);
	if (!ss) {
		return;
	}

	List<struct zwp_tablet_tool_v2*>::Element* E = ss->tablet_tools.find(wp_tablet_tool_v2);

	if (E && E->get()) {
		struct zwp_tablet_tool_v2* tool = E->get();
		TabletToolState* state = wp_tablet_tool_get_state(tool);
		memdelete(state);

		zwp_tablet_tool_v2_destroy(tool);
		ss->tablet_tools.erase(E);
	}
}

void WaylandThread::_wp_tablet_tool_on_proximity_in(void* data,
	struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t serial, struct zwp_tablet_v2* tablet,
	struct wl_surface* surface)
{
	// NOTE: Works pretty much like wl_pointer::enter.

	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	TabletToolState* ts = wp_tablet_tool_get_state(wp_tablet_tool_v2);
	ERR_FAIL_NULL(ts);

	ts->data_pending.proximity_serial = serial;
	ts->data_pending.proximal_id = ws->id;
	ts->data_pending.last_proximal_id = ws->id;

	DEBUG_LOG_WAYLAND_THREAD(
		vformat("Tablet tool entered window %d.", ts->data_pending.proximal_id));
}

void WaylandThread::_wp_tablet_tool_on_motion(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, wl_fixed_t x, wl_fixed_t y)
{
	// NOTE: Works pretty much like wl_pointer::motion.

	TabletToolState* ts = wp_tablet_tool_get_state(wp_tablet_tool_v2);
	ERR_FAIL_NULL(ts);

	TabletToolData& td = ts->data_pending;

	td.position.x = wl_fixed_to_double(x);
	td.position.y = wl_fixed_to_double(y);
}

void WaylandThread::_wp_tablet_tool_on_pressure(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t pressure)
{
	TabletToolState* ts = wp_tablet_tool_get_state(wp_tablet_tool_v2);
	ERR_FAIL_NULL(ts);

	ts->data_pending.pressure = pressure;
}

void WaylandThread::_wp_tablet_tool_on_distance(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t distance)
{
	// Unsupported
}

void WaylandThread::_wp_tablet_tool_on_tilt(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
	TabletToolState* ts = wp_tablet_tool_get_state(wp_tablet_tool_v2);
	ERR_FAIL_NULL(ts);

	TabletToolData& td = ts->data_pending;

	td.tilt.x = wl_fixed_to_double(tilt_x);
	td.tilt.y = wl_fixed_to_double(tilt_y);
}

void WaylandThread::_wp_tablet_tool_on_rotation(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, wl_fixed_t degrees)
{
	// Unsupported.
}

void WaylandThread::_wp_tablet_tool_on_slider(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, int32_t position)
{
	// Unsupported.
}

void WaylandThread::_wp_tablet_tool_on_wheel(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, wl_fixed_t degrees, int32_t clicks)
{
	// TODO
}

void WaylandThread::_wp_text_input_on_enter(
	void* data, struct zwp_text_input_v3* wp_text_input_v3, struct wl_surface* surface)
{
	SeatState* ss = (SeatState*)data;
	if (!ss) {
		return;
	}

	WindowState* ws = wl_surface_get_window_state(surface);
	if (!ws) {
		return;
	}

	ss->ime_window_id = ws->id;
	ss->ime_enabled = true;
}

// NOTE: From now on, we must ignore all further events until an enter event.
void WaylandThread::_wp_text_input_on_leave(
	void* data, struct zwp_text_input_v3* wp_text_input_v3, struct wl_surface* surface)
{
	SeatState* ss = (SeatState*)data;
	if (!ss) {
		return;
	}

	if (ss->ime_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	Ref<IMEUpdateEventMessage> msg;
	msg.instantiate();
	msg->id = ss->ime_window_id;
	msg->text = String();
	msg->selection = Vector2i();
	ss->wayland_thread->push_message(msg);

	ss->ime_window_id = DisplayServerEnums::INVALID_WINDOW_ID;
	ss->ime_enabled = false;
	ss->ime_active = false;
	ss->ime_text = String();
	ss->ime_text_commit = String();
	ss->ime_cursor = Vector2i();
}

void WaylandThread::_wp_text_input_on_preedit_string(void* data,
	struct zwp_text_input_v3* wp_text_input_v3, const char* text, int32_t cursor_begin,
	int32_t cursor_end)
{
	SeatState* ss = (SeatState*)data;
	if (!ss) {
		return;
	}

	if (ss->ime_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	ss->ime_text = String::utf8(text);

	// Convert cursor positions from UTF-8 to UTF-32 offset.
	int32_t cursor_begin_utf32 = 0;
	int32_t cursor_end_utf32 = 0;
	for (int i = 0; i < ss->ime_text.length(); i++) {
		uint32_t c = ss->ime_text[i];
		if (c <= 0x7f) { // 7 bits.
			cursor_begin -= 1;
			cursor_end -= 1;
		}
		else if (c <= 0x7ff) { // 11 bits
			cursor_begin -= 2;
			cursor_end -= 2;
		}
		else if (c <= 0xffff) { // 16 bits
			cursor_begin -= 3;
			cursor_end -= 3;
		}
		else if (c <= 0x001fffff) { // 21 bits
			cursor_begin -= 4;
			cursor_end -= 4;
		}
		else if (c <= 0x03ffffff) { // 26 bits
			cursor_begin -= 5;
			cursor_end -= 5;
		}
		else if (c <= 0x7fffffff) { // 31 bits
			cursor_begin -= 6;
			cursor_end -= 6;
		}
		else {
			cursor_begin -= 1;
			cursor_end -= 1;
		}
		if (cursor_begin == 0) {
			cursor_begin_utf32 = i + 1;
		}
		if (cursor_end == 0) {
			cursor_end_utf32 = i + 1;
		}
		if (cursor_begin <= 0 && cursor_end <= 0) {
			break;
		}
	}
	ss->ime_cursor = Vector2i(cursor_begin_utf32, cursor_end_utf32 - cursor_begin_utf32);
}

void WaylandThread::_wp_text_input_on_commit_string(
	void* data, struct zwp_text_input_v3* wp_text_input_v3, const char* text)
{
	SeatState* ss = (SeatState*)data;
	if (!ss) {
		return;
	}

	if (ss->ime_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	ss->ime_text_commit = String::utf8(text);
}

void WaylandThread::_wp_text_input_on_delete_surrounding_text(void* data,
	struct zwp_text_input_v3* wp_text_input_v3, uint32_t before_length, uint32_t after_length)
{
	// Not implemented.
}

void WaylandThread::_wp_text_input_on_done(
	void* data, struct zwp_text_input_v3* wp_text_input_v3, uint32_t serial)
{
	SeatState* ss = (SeatState*)data;
	if (!ss) {
		return;
	}

	if (ss->ime_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	if (!ss->ime_text_commit.is_empty()) {
		Ref<IMECommitEventMessage> msg;
		msg.instantiate();
		msg->id = ss->ime_window_id;
		msg->text = ss->ime_text_commit;
		ss->wayland_thread->push_message(msg);
	}
	else {
		Ref<IMEUpdateEventMessage> msg;
		msg.instantiate();
		msg->id = ss->ime_window_id;
		msg->text = ss->ime_text;
		msg->selection = ss->ime_cursor;
		ss->wayland_thread->push_message(msg);
	}

	ss->ime_text = String();
	ss->ime_text_commit = String();
	ss->ime_cursor = Vector2i();
}

void WaylandThread::_xdg_activation_token_on_done(
	void* data, struct xdg_activation_token_v1* xdg_activation_token, const char* token)
{
	WindowState* ws = (WindowState*)data;
	ERR_FAIL_NULL(ws);
	ERR_FAIL_NULL(ws->wayland_thread);
	ERR_FAIL_NULL(ws->wl_surface);

	xdg_activation_v1_activate(ws->wayland_thread->registry.xdg_activation, token, ws->wl_surface);
	xdg_activation_token_v1_destroy(xdg_activation_token);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Received activation token and requested window activation."));
}

void WaylandThread::_godot_embedding_compositor_on_client(void* data,
	struct godot_embedding_compositor* godot_embedding_compositor,
	struct godot_embedded_client* godot_embedded_client, int32_t pid)
{
	EmbeddingCompositorState* state = (EmbeddingCompositorState*)data;
	ERR_FAIL_NULL(state);

	EmbeddedClientState* client_state = memnew(EmbeddedClientState);
	client_state->embedding_compositor = godot_embedding_compositor;
	client_state->pid = pid;
	godot_embedded_client_add_listener(
		godot_embedded_client, &godot_embedded_client_listener, client_state);

	DEBUG_LOG_WAYLAND_THREAD(vformat("New client %d.", pid));
	state->clients.push_back(godot_embedded_client);
}

void WaylandThread::_godot_embedded_client_on_disconnected(
	void* data, struct godot_embedded_client* godot_embedded_client)
{
	EmbeddedClientState* state = (EmbeddedClientState*)data;
	ERR_FAIL_NULL(state);

	EmbeddingCompositorState* ecomp_state =
		godot_embedding_compositor_get_state(state->embedding_compositor);
	ERR_FAIL_NULL(ecomp_state);

	ecomp_state->clients.erase_unordered(godot_embedded_client);
	ecomp_state->mapped_clients.erase(state->pid);

	memfree(state);
	godot_embedded_client_destroy(godot_embedded_client);

	DEBUG_LOG_WAYLAND_THREAD(vformat("Client %d disconnected.", state->pid));
}

void WaylandThread::_godot_embedded_client_on_window_embedded(
	void* data, struct godot_embedded_client* godot_embedded_client)
{
	EmbeddedClientState* state = (EmbeddedClientState*)data;
	ERR_FAIL_NULL(state);

	EmbeddingCompositorState* ecomp_state =
		godot_embedding_compositor_get_state(state->embedding_compositor);
	ERR_FAIL_NULL(ecomp_state);

	state->window_mapped = true;

	ERR_FAIL_COND_MSG(ecomp_state->mapped_clients.has(state->pid),
		"More than one Wayland client per PID tried to create a window.");

	ecomp_state->mapped_clients[state->pid] = godot_embedded_client;
}

void WaylandThread::_godot_embedded_client_on_window_focus_in(
	void* data, struct godot_embedded_client* godot_embedded_client)
{
	EmbeddedClientState* state = (EmbeddedClientState*)data;
	ERR_FAIL_NULL(state);

	EmbeddingCompositorState* ecomp_state =
		godot_embedding_compositor_get_state(state->embedding_compositor);
	ERR_FAIL_NULL(ecomp_state);

	ecomp_state->focused_pid = state->pid;
	DEBUG_LOG_WAYLAND_THREAD(vformat("Embedded client pid %d focus in", state->pid));
}

void WaylandThread::_godot_embedded_client_on_window_focus_out(
	void* data, struct godot_embedded_client* godot_embedded_client)
{
	EmbeddedClientState* state = (EmbeddedClientState*)data;
	ERR_FAIL_NULL(state);

	EmbeddingCompositorState* ecomp_state =
		godot_embedding_compositor_get_state(state->embedding_compositor);
	ERR_FAIL_NULL(ecomp_state);

	ecomp_state->focused_pid = -1;
	DEBUG_LOG_WAYLAND_THREAD(vformat("Embedded client pid %d focus out", state->pid));
}

void WaylandThread::_clipboard_send(Vector<uint8_t>& p_data, const char* p_media_type, int32_t p_fd)
{
	ssize_t written_bytes = 0;

	bool valid_mime = false;

	if (strcmp(p_media_type, "text/plain;charset=utf-8") == 0) {
		valid_mime = true;
	}
	else if (strcmp(p_media_type, "text/plain") == 0) {
		valid_mime = true;
	}

	if (!valid_mime) {
		DEBUG_LOG_WAYLAND_THREAD(
			vformat("Clipboard: Media type '%s' unknown, skipping.", p_media_type));
		return;
	}

	written_bytes = write(p_fd, p_data.ptr(), p_data.size());

	if (written_bytes > 0) {
		DEBUG_LOG_WAYLAND_THREAD(vformat("Clipboard: sent %d bytes.", written_bytes));
	}
	else if (written_bytes == 0) {
		DEBUG_LOG_WAYLAND_THREAD("Clipboard: no bytes sent.");
	}
	else {
		ERR_PRINT(vformat("Clipboard: write error %d.", errno));
	}
}

// NOTE: This must be started after a valid wl_display is loaded.
void WaylandThread::_poll_events_thread(void* p_data)
{
	Thread::set_name("Wayland Events");

	ThreadData* data = (ThreadData*)p_data;
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(data->wl_display);

	struct pollfd poll_fd = {};
	poll_fd.fd = wl_display_get_fd(data->wl_display);
	poll_fd.events = POLLIN;

	while (true) {
		// Empty the event queue while it's full.
		while (wl_display_prepare_read(data->wl_display) != 0) {
			// We aren't using wl_display_dispatch(), instead "manually" handling events
			// through wl_display_dispatch_pending so that we can use a global mutex and
			// be sure that this and the main thread won't race over stuff, as long as
			// the main thread locks it too.
			//
			// Note that the main thread can still call wl_display_roundtrip as that
			// method directly handles all events, effectively bypassing this polling
			// loop and thus the mutex locking, avoiding a deadlock.
			//
			// WARNING: Never call `wl_display_roundtrip` inside event handlers or while
			// this mutex isn't held! `wl_display_roundtrip` manually handles new events
			// and if not properly gated it _will_ cause potentially stall-inducing race
			// conditions. Ask me how I know.
			MutexLock mutex_lock(data->mutex);

			if (wl_display_dispatch_pending(data->wl_display) == -1) {
				// Oh no. We'll check and handle any display error below.
				break;
			}
		}

		_wl_display_check_error(data->wl_display);

		wl_display_flush(data->wl_display);

		// Wait for the event file descriptor to have new data.
		poll(&poll_fd, 1, -1);

		if (data->thread_done.is_set()) {
			wl_display_cancel_read(data->wl_display);
			break;
		}

		if (poll_fd.revents | POLLIN) {
			// Load the queues with fresh new data.
			wl_display_read_events(data->wl_display);
		}
		else {
			// Oh well... Stop signaling that we want to read.
			wl_display_cancel_read(data->wl_display);
		}

		// The docs advise to redispatch unconditionally and it looks like that if we
		// don't do this we can't catch protocol errors, which is bad.
		MutexLock mutex_lock(data->mutex);
		wl_display_dispatch_pending(data->wl_display);
	}
}

struct wl_display* WaylandThread::get_wl_display() const { return wl_display; }

// NOTE: Stuff like libdecor can (and will) register foreign proxies which
// aren't formatted as we like. This method is needed to detect whether a proxy
// has our tag. Also, be careful! The proxy has to be manually tagged or it
// won't be recognized.
bool WaylandThread::wl_proxy_is_godot(struct wl_proxy* p_proxy)
{
	ERR_FAIL_NULL_V(p_proxy, false);

	return wl_proxy_get_tag(p_proxy) == &proxy_tag;
}

void WaylandThread::wl_proxy_tag_godot(struct wl_proxy* p_proxy)
{
	ERR_FAIL_NULL(p_proxy);

	wl_proxy_set_tag(p_proxy, &proxy_tag);
}

// Returns the wl_surface's `WindowState`, otherwise `nullptr`.
// NOTE: This will fail if the surface isn't tagged as ours.
WaylandThread::WindowState* WaylandThread::wl_surface_get_window_state(struct wl_surface* p_surface)
{
	if (p_surface && wl_proxy_is_godot((wl_proxy*)p_surface)) {
		return (WindowState*)wl_surface_get_user_data(p_surface);
	}

	return nullptr;
}

// Returns the wl_outputs's `ScreenState`, otherwise `nullptr`.
// NOTE: This will fail if the output isn't tagged as ours.
WaylandThread::ScreenState* WaylandThread::wl_output_get_screen_state(struct wl_output* p_output)
{
	if (p_output && wl_proxy_is_godot((wl_proxy*)p_output)) {
		return (ScreenState*)wl_output_get_user_data(p_output);
	}

	return nullptr;
}

// Returns the wl_seat's `SeatState`, otherwise `nullptr`.
// NOTE: This will fail if the output isn't tagged as ours.
WaylandThread::SeatState* WaylandThread::wl_seat_get_seat_state(struct wl_seat* p_seat)
{
	if (p_seat && wl_proxy_is_godot((wl_proxy*)p_seat)) {
		return (SeatState*)wl_seat_get_user_data(p_seat);
	}

	return nullptr;
}

// Returns the wp_tablet_tool's `TabletToolState`, otherwise `nullptr`.
// NOTE: This will fail if the output isn't tagged as ours.
WaylandThread::TabletToolState* WaylandThread::wp_tablet_tool_get_state(
	struct zwp_tablet_tool_v2* p_tool)
{
	if (p_tool && wl_proxy_is_godot((wl_proxy*)p_tool)) {
		return (TabletToolState*)zwp_tablet_tool_v2_get_user_data(p_tool);
	}

	return nullptr;
}

// Returns the wl_data_offer's `OfferState`, otherwise `nullptr`.
// NOTE: This will fail if the output isn't tagged as ours.
WaylandThread::OfferState* WaylandThread::wl_data_offer_get_offer_state(
	struct wl_data_offer* p_offer)
{
	if (p_offer && wl_proxy_is_godot((wl_proxy*)p_offer)) {
		return (OfferState*)wl_data_offer_get_user_data(p_offer);
	}

	return nullptr;
}

// Returns the wl_data_offer's `OfferState`, otherwise `nullptr`.
// NOTE: This will fail if the output isn't tagged as ours.
WaylandThread::OfferState* WaylandThread::wp_primary_selection_offer_get_offer_state(
	struct zwp_primary_selection_offer_v1* p_offer)
{
	if (p_offer && wl_proxy_is_godot((wl_proxy*)p_offer)) {
		return (OfferState*)zwp_primary_selection_offer_v1_get_user_data(p_offer);
	}

	return nullptr;
}

WaylandThread::ColorManagementState* WaylandThread::wp_color_manager_get_state(
	wp_color_manager_v1* p_color_manager)
{
	if (p_color_manager && wl_proxy_is_godot((wl_proxy*)p_color_manager)) {
		return (ColorManagementState*)wp_color_manager_v1_get_user_data(p_color_manager);
	}

	return nullptr;
}

WaylandThread::EmbeddingCompositorState* WaylandThread::godot_embedding_compositor_get_state(
	struct godot_embedding_compositor* p_compositor)
{
	// NOTE: No need for tag check as it's a "fake" interface - nothing else exposes it.
	if (p_compositor) {
		return (EmbeddingCompositorState*)godot_embedding_compositor_get_user_data(p_compositor);
	}

	return nullptr;
}

// This is implemented as a method because this is the simplest way of
// accounting for dynamic output scale changes.
int WaylandThread::window_state_get_preferred_buffer_scale(WindowState* p_ws)
{
	ERR_FAIL_NULL_V(p_ws, 1);

	if (p_ws->preferred_fractional_scale > 0) {
		// We're scaling fractionally. Per spec, the buffer scale is always 1.
		return 1;
	}

	if (p_ws->wl_outputs.is_empty()) {
		DEBUG_LOG_WAYLAND_THREAD("Window has no output associated, returning buffer scale of 1.");
		return 1;
	}

	// TODO: Cache value?
	int max_size = 1;

	// ================================ IMPORTANT =================================
	// NOTE: Due to a Godot limitation, we can't really rescale the whole UI yet.
	// Because of this reason, all platforms have resorted to forcing the highest
	// scale possible of a system on any window, despite of what screen it's onto.
	// On this backend everything's already in place for dynamic window scale
	// handling, but in the meantime we'll just select the biggest _global_ output.
	// To restore dynamic scale selection, simply iterate over `p_ws->wl_outputs`
	// instead.
	for (struct wl_output* wl_output : p_ws->registry->wl_outputs) {
		ScreenState* ss = wl_output_get_screen_state(wl_output);

		if (ss && ss->pending_data.scale > max_size) {
			// NOTE: For some mystical reason, wl_output.done is emitted _after_ windows
			// get resized but the scale event gets sent _before_ that. I'm still leaning
			// towards the idea that rescaling when a window gets a resolution change is a
			// pretty good approach, but this means that we'll have to use the screen data
			// before it's "committed".
			// FIXME: Use the committed data. Somehow.
			max_size = ss->pending_data.scale;
		}
	}

	return max_size;
}

double WaylandThread::window_state_get_scale_factor(const WindowState* p_ws)
{
	ERR_FAIL_NULL_V(p_ws, 1);

	if (p_ws->fractional_scale > 0) {
		// The fractional scale amount takes priority.
		return p_ws->fractional_scale;
	}

	return p_ws->buffer_scale;
}

void WaylandThread::window_state_update_size(WindowState* p_ws, int p_width, int p_height)
{
	ERR_FAIL_NULL(p_ws);

	// Failsafe.
	p_width = MAX(p_width, 1);
	p_height = MAX(p_height, 1);

	int preferred_buffer_scale = window_state_get_preferred_buffer_scale(p_ws);
	bool using_fractional = p_ws->preferred_fractional_scale > 0;

	// If neither is true we no-op.
	bool scale_changed = false;
	bool size_changed = false;

	if (p_ws->rect.size.width != p_width || p_ws->rect.size.height != p_height) {
		p_ws->rect.size.width = p_width;
		p_ws->rect.size.height = p_height;

		size_changed = true;
	}

	if (using_fractional && p_ws->fractional_scale != p_ws->preferred_fractional_scale) {
		p_ws->fractional_scale = p_ws->preferred_fractional_scale;
		scale_changed = true;
	}

	if (p_ws->buffer_scale != preferred_buffer_scale) {
		// The buffer scale is always important, even if we use frac scaling.
		p_ws->buffer_scale = preferred_buffer_scale;

		if (!using_fractional) {
			// We don't bother updating everything else if it's turned on though.
			scale_changed = true;
		}
	}

	if (p_ws->wl_surface) {
		if (p_ws->wp_viewport) {
			wp_viewport_set_destination(p_ws->wp_viewport, p_width, p_height);
		}

		if (p_ws->xdg_surface) {
			xdg_surface_set_window_geometry(p_ws->xdg_surface, 0, 0, p_width, p_height);
		}
	}

#ifdef LIBDECOR_ENABLED
	if (p_ws->libdecor_frame) {
		struct libdecor_state* state = libdecor_state_new(p_width, p_height);
		libdecor_frame_commit(p_ws->libdecor_frame, state, p_ws->pending_libdecor_configuration);
		libdecor_state_free(state);
		p_ws->pending_libdecor_configuration = nullptr;
	}
#endif

	if (size_changed || scale_changed) {
		double win_scale = window_state_get_scale_factor(p_ws);
		Size2i scaled_size = scale_vector2i(p_ws->rect.size, win_scale);

		if (using_fractional) {
			DEBUG_LOG_WAYLAND_THREAD(
				vformat("Resizing the window from %s to %s (fractional scale x%f).",
					p_ws->rect.size, scaled_size, p_ws->fractional_scale));
		}
		else {
			DEBUG_LOG_WAYLAND_THREAD(
				vformat("Resizing the window from %s to %s (buffer scale x%d).", p_ws->rect.size,
					scaled_size, p_ws->buffer_scale));
		}

		// FIXME: Actually resize the hint instead of centering it.
		p_ws->wayland_thread->pointer_set_hint(scaled_size / 2);

		Ref<WindowRectMessage> rect_msg;
		rect_msg.instantiate();
		rect_msg->id = p_ws->id;
		rect_msg->rect.position = scale_vector2i(p_ws->rect.position, win_scale);
		rect_msg->rect.size = scaled_size;
		rect_msg->buffer_scale = preferred_buffer_scale;
		p_ws->wayland_thread->push_message(rect_msg);
	}

	if (scale_changed) {
		Ref<WindowEventMessage> dpi_msg;
		dpi_msg.instantiate();
		dpi_msg->id = p_ws->id;
		dpi_msg->event = DisplayServerEnums::WINDOW_EVENT_DPI_CHANGE;
		p_ws->wayland_thread->push_message(dpi_msg);
	}
}

void WaylandThread::window_state_set_buffer_scale(WindowState* p_ws, int p_buffer_scale)
{
	ERR_FAIL_NULL(p_ws);
	ERR_FAIL_COND(p_buffer_scale <= 0);

	ERR_FAIL_NULL(p_ws->wl_surface);

	if (p_ws->buffer_scale != p_buffer_scale) {
		p_ws->buffer_scale = p_buffer_scale;
		wl_surface_set_buffer_scale(p_ws->wl_surface, p_buffer_scale);
	}
}

// Scales a vector according to wp_fractional_scale's rules, where coordinates
// must be scaled with away from zero half-rounding.
Vector2i WaylandThread::scale_vector2i(const Vector2i& p_vector, double p_amount)
{
	// This snippet is tiny, I know, but this is done a lot.
	int x = std::round(p_vector.x * p_amount);
	int y = std::round(p_vector.y * p_amount);

	return Vector2i(x, y);
}

void WaylandThread::seat_state_unlock_pointer(SeatState* p_ss)
{
	ERR_FAIL_NULL(p_ss);

	if (p_ss->wl_pointer == nullptr) {
		return;
	}

	if (p_ss->wp_locked_pointer) {
		zwp_locked_pointer_v1_destroy(p_ss->wp_locked_pointer);
		p_ss->wp_locked_pointer = nullptr;
	}

	if (p_ss->wp_confined_pointer) {
		zwp_confined_pointer_v1_destroy(p_ss->wp_confined_pointer);
		p_ss->wp_confined_pointer = nullptr;
	}

	p_ss->pointer_locked = false;
}

void WaylandThread::seat_state_lock_pointer(SeatState* p_ss, struct wl_surface* p_surface)
{
	ERR_FAIL_NULL(p_ss);
	ERR_FAIL_NULL(p_surface);

	if (p_ss->wl_pointer == nullptr) {
		WARN_PRINT("Can't lock - no pointer?");
		return;
	}

	if (registry.wp_pointer_constraints == nullptr) {
		WARN_PRINT("Can't lock - no constraints global.");
		return;
	}

	p_ss->pointer_locked = false;

	// We always set a listener that tracks the locking status. This is useful as
	// this constraint might be reused for the legacy warp hack.
	p_ss->wp_locked_pointer =
		zwp_pointer_constraints_v1_lock_pointer(registry.wp_pointer_constraints, p_surface,
			p_ss->wl_pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
	zwp_locked_pointer_v1_add_listener(
		p_ss->wp_locked_pointer, &zwp_locked_pointer_v1_listener, p_ss);
}

void WaylandThread::seat_state_set_hint(SeatState* p_ss, int p_x, int p_y)
{
	if (p_ss->wp_locked_pointer == nullptr) {
		return;
	}

	zwp_locked_pointer_v1_set_cursor_position_hint(
		p_ss->wp_locked_pointer, wl_fixed_from_int(p_x), wl_fixed_from_int(p_y));
}

void WaylandThread::seat_state_warp_pointer(SeatState* p_ss, int p_x, int p_y)
{
	if (registry.wp_pointer_warp == nullptr) {
		return;
	}

	if (p_ss->pointer_data.pointed_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	struct wl_surface* surface = window_get_wl_surface(p_ss->pointer_data.pointed_id);
	ERR_FAIL_NULL(surface);

	wp_pointer_warp_v1_warp_pointer(registry.wp_pointer_warp, surface, p_ss->wl_pointer,
		wl_fixed_from_int(p_x), wl_fixed_from_int(p_y), p_ss->pointer_enter_serial);
}

void WaylandThread::seat_state_confine_pointer(SeatState* p_ss, struct wl_surface* p_surface)
{
	ERR_FAIL_NULL(p_ss);
	ERR_FAIL_NULL(p_surface);

	if (p_ss->wl_pointer == nullptr) {
		return;
	}

	if (registry.wp_pointer_constraints == nullptr) {
		return;
	}

	if (p_ss->wp_confined_pointer == nullptr) {
		p_ss->wp_confined_pointer =
			zwp_pointer_constraints_v1_confine_pointer(registry.wp_pointer_constraints, p_surface,
				p_ss->wl_pointer, nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
	}
}

void WaylandThread::seat_state_update_cursor(SeatState* p_ss)
{
	ERR_FAIL_NULL(p_ss);

	WaylandThread* thread = p_ss->wayland_thread;
	ERR_FAIL_NULL(p_ss->wayland_thread);

	if (!p_ss->wl_pointer || !p_ss->cursor_surface) {
		return;
	}

	// NOTE: Those values are valid by default and will hide the cursor when
	// unchanged.
	struct wl_buffer* cursor_buffer = nullptr;
	uint32_t hotspot_x = 0;
	uint32_t hotspot_y = 0;
	int scale = 1;

	if (thread->cursor_visible) {
		DisplayServerEnums::CursorShape shape = thread->cursor_shape;

		struct CustomCursor* custom_cursor = thread->custom_cursors.getptr(shape);

		if (custom_cursor) {
			cursor_buffer = custom_cursor->wl_buffer;
			hotspot_x = custom_cursor->hotspot.x;
			hotspot_y = custom_cursor->hotspot.y;

			// We can't really reasonably scale custom cursors, so we'll let the
			// compositor do it for us (badly).
			scale = 1;
		}
		else if (thread->registry.wp_cursor_shape_manager) {
			wp_cursor_shape_device_v1_shape wp_shape = thread->standard_cursors[shape];
			wp_cursor_shape_device_v1_set_shape(
				p_ss->wp_cursor_shape_device, p_ss->pointer_enter_serial, wp_shape);

			// We should avoid calling the `wl_pointer_set_cursor` at the end of this method.
			return;
		}
		else {
			struct wl_cursor* wl_cursor = thread->wl_cursors[shape];

			if (!wl_cursor) {
				return;
			}

			int frame_idx = 0;

			if (wl_cursor->image_count > 1) {
				// The cursor is animated.
				frame_idx = wl_cursor_frame(wl_cursor, p_ss->cursor_time_ms);

				if (!p_ss->cursor_frame_callback) {
					// Since it's animated, we'll re-update it the next frame.
					p_ss->cursor_frame_callback = wl_surface_frame(p_ss->cursor_surface);
					wl_callback_add_listener(
						p_ss->cursor_frame_callback, &cursor_frame_callback_listener, p_ss);
				}
			}

			struct wl_cursor_image* wl_cursor_image = wl_cursor->images[frame_idx];

			scale = thread->cursor_scale;

			cursor_buffer = wl_cursor_image_get_buffer(wl_cursor_image);

			// As the surface's buffer is scaled (thus the surface is smaller) and the
			// hotspot must be expressed in surface-local coordinates, we need to scale
			// it down accordingly.
			hotspot_x = wl_cursor_image->hotspot_x / scale;
			hotspot_y = wl_cursor_image->hotspot_y / scale;
		}
	}

	wl_pointer_set_cursor(
		p_ss->wl_pointer, p_ss->pointer_enter_serial, p_ss->cursor_surface, hotspot_x, hotspot_y);
	wl_surface_set_buffer_scale(p_ss->cursor_surface, scale);
	wl_surface_attach(p_ss->cursor_surface, cursor_buffer, 0, 0);
	wl_surface_damage_buffer(p_ss->cursor_surface, 0, 0, INT_MAX, INT_MAX);

	wl_surface_commit(p_ss->cursor_surface);
}

void WaylandThread::seat_state_echo_keys(SeatState* p_ss)
{
	ERR_FAIL_NULL(p_ss);

	if (p_ss->wl_keyboard == nullptr) {
		return;
	}

	// TODO: Comment and document out properly this block of code.
	// In short, this implements key repeating.
	if (p_ss->repeat_key_delay_msec && p_ss->repeating_keycode != XKB_KEYCODE_INVALID) {
		uint64_t current_ticks = OS::get_ticks_msec();
		uint64_t delayed_start_ticks = p_ss->last_repeat_start_msec + p_ss->repeat_start_delay_msec;

		if (p_ss->last_repeat_msec < delayed_start_ticks) {
			p_ss->last_repeat_msec = delayed_start_ticks;
		}

		if (current_ticks >= delayed_start_ticks) {
			uint64_t ticks_delta = current_ticks - p_ss->last_repeat_msec;

			int keys_amount = (ticks_delta / p_ss->repeat_key_delay_msec);

			for (int i = 0; i < keys_amount; i++) {
				_seat_state_handle_xkb_keycode(p_ss, p_ss->repeating_keycode, true, true);
			}

			p_ss->last_repeat_msec += ticks_delta - (ticks_delta % p_ss->repeat_key_delay_msec);
		}
	}
}

void WaylandThread::push_message(Ref<Message> message) { messages.push_back(message); }

bool WaylandThread::has_message() { return messages.front() != nullptr; }

Ref<WaylandThread::Message> WaylandThread::pop_message()
{
	if (messages.front() != nullptr) {
		Ref<Message> msg = messages.front()->get();
		messages.pop_front();
		return msg;
	}

	// This method should only be called if `has_messages` returns true but if
	// that isn't the case we'll just return an invalid `Ref`. After all, due to
	// its `InputEvent`-like interface, we still have to dynamically cast and check
	// the `Ref`'s validity anyways.
	return Ref<Message>();
}

void WaylandThread::window_create(DisplayServerEnums::WindowID p_window_id, const Size2i& p_size,
	DisplayServerEnums::WindowID p_parent_id)
{
	ERR_FAIL_COND(windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	ws.id = p_window_id;

	ws.registry = &registry;
	ws.wayland_thread = this;

	ws.rect.size = p_size.maxi(1);

	ws.wl_surface = wl_compositor_create_surface(registry.wl_compositor);
	wl_proxy_tag_godot((struct wl_proxy*)ws.wl_surface);
	wl_surface_add_listener(ws.wl_surface, &wl_surface_listener, &ws);

	if (registry.wp_viewporter) {
		ws.wp_viewport = wp_viewporter_get_viewport(registry.wp_viewporter, ws.wl_surface);

		if (registry.wp_fractional_scale_manager) {
			ws.wp_fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(
				registry.wp_fractional_scale_manager, ws.wl_surface);
			wp_fractional_scale_v1_add_listener(
				ws.wp_fractional_scale, &wp_fractional_scale_listener, &ws);
		}
	}

	if (supports_hdr()) {
		ws.wp_color_management_surface_feedback =
			wp_color_manager_v1_get_surface_feedback(registry.wp_color_manager, ws.wl_surface);
		wp_color_management_surface_feedback_v1_add_listener(
			ws.wp_color_management_surface_feedback, &wp_color_management_surface_feedback_listener,
			&ws);

		struct wp_image_description_v1* image_description =
			wp_color_management_surface_feedback_v1_get_preferred_parametric(
				ws.wp_color_management_surface_feedback);
		wp_image_description_v1_add_listener(
			image_description, &wp_image_description_listener, &ws);

		// NOTE: requires vulkan to use the VK_COLOR_SPACE_PASSTHROUGH_EXT colorspace to not raise
		// protocol errors
		ws.wp_color_management_surface =
			wp_color_manager_v1_get_surface(registry.wp_color_manager, ws.wl_surface);
	}

	bool decorated = false;

#ifdef LIBDECOR_ENABLED
	if (!decorated && libdecor_context) {
		ws.libdecor_frame = libdecor_decorate(libdecor_context, ws.wl_surface,
			(struct libdecor_frame_interface*)&libdecor_frame_interface, &ws);
		libdecor_frame_map(ws.libdecor_frame);

		if (registry.xdg_toplevel_icon_manager) {
			xdg_toplevel* toplevel = libdecor_frame_get_xdg_toplevel(ws.libdecor_frame);
			if (toplevel != nullptr) {
				xdg_toplevel_icon_manager_v1_set_icon(
					registry.xdg_toplevel_icon_manager, toplevel, xdg_icon);
			}
		}

		decorated = true;
	}
#endif

	if (!decorated) {
		// libdecor has failed loading or is disabled, we shall handle xdg_toplevel
		// creation and decoration ourselves (and by decorating for now I just mean
		// asking for SSDs and hoping for the best).
		ws.xdg_surface = xdg_wm_base_get_xdg_surface(registry.xdg_wm_base, ws.wl_surface);
		xdg_surface_add_listener(ws.xdg_surface, &xdg_surface_listener, &ws);

		ws.xdg_toplevel = xdg_surface_get_toplevel(ws.xdg_surface);
		xdg_toplevel_add_listener(ws.xdg_toplevel, &xdg_toplevel_listener, &ws);

		if (registry.xdg_decoration_manager) {
			ws.xdg_toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
				registry.xdg_decoration_manager, ws.xdg_toplevel);
			zxdg_toplevel_decoration_v1_add_listener(
				ws.xdg_toplevel_decoration, &xdg_toplevel_decoration_listener, &ws);

			decorated = true;
		}

		if (registry.xdg_toplevel_icon_manager) {
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, ws.xdg_toplevel, xdg_icon);
		}
	}

	if (p_parent_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		// NOTE: It's important to set the parent ASAP to avoid misunderstandings with
		// the compositor. For example, niri immediately resizes the window to full
		// size as soon as it's configured if it's not parented to another toplevel.
		window_set_parent(p_window_id, p_parent_id);
	}

	ws.frame_callback = wl_surface_frame(ws.wl_surface);
	wl_callback_add_listener(ws.frame_callback, &frame_wl_callback_listener, &ws);

	if (registry.xdg_exporter_v2) {
		ws.xdg_exported_v2 =
			zxdg_exporter_v2_export_toplevel(registry.xdg_exporter_v2, ws.wl_surface);
		zxdg_exported_v2_add_listener(ws.xdg_exported_v2, &xdg_exported_v2_listener, &ws);
	}
	else if (registry.xdg_exporter_v1) {
		ws.xdg_exported_v1 = zxdg_exporter_v1_export(registry.xdg_exporter_v1, ws.wl_surface);
		zxdg_exported_v1_add_listener(ws.xdg_exported_v1, &xdg_exported_v1_listener, &ws);
	}

	wl_surface_commit(ws.wl_surface);
}

void WaylandThread::window_create_popup(DisplayServerEnums::WindowID p_window_id,
	DisplayServerEnums::WindowID p_parent_id, Rect2i p_rect)
{
	ERR_FAIL_COND(windows.has(p_window_id));
	ERR_FAIL_COND(!windows.has(p_parent_id));

	WindowState& ws = windows[p_window_id];
	WindowState& parent = windows[p_parent_id];

	double parent_scale = window_state_get_scale_factor(&parent);

	p_rect.position = scale_vector2i(p_rect.position, 1.0 / parent_scale);
	p_rect.size = scale_vector2i(p_rect.size, 1.0 / parent_scale);
	// We manually scaled based on the parent. If we don't set the relevant fields,
	// the resizing routines will get confused and scale once more.
	ws.preferred_fractional_scale = parent.preferred_fractional_scale;
	ws.fractional_scale = parent.fractional_scale;
	ws.buffer_scale = parent.buffer_scale;

	ws.id = p_window_id;
	ws.parent_id = p_parent_id;
	ws.registry = &registry;
	ws.wayland_thread = this;

	ws.rect = p_rect;

	ws.wl_surface = wl_compositor_create_surface(registry.wl_compositor);
	wl_proxy_tag_godot((struct wl_proxy*)ws.wl_surface);
	wl_surface_add_listener(ws.wl_surface, &wl_surface_listener, &ws);

	if (registry.wp_viewporter) {
		ws.wp_viewport = wp_viewporter_get_viewport(registry.wp_viewporter, ws.wl_surface);

		if (registry.wp_fractional_scale_manager) {
			ws.wp_fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(
				registry.wp_fractional_scale_manager, ws.wl_surface);
			wp_fractional_scale_v1_add_listener(
				ws.wp_fractional_scale, &wp_fractional_scale_listener, &ws);
		}
	}

	ws.xdg_surface = xdg_wm_base_get_xdg_surface(registry.xdg_wm_base, ws.wl_surface);
	xdg_surface_add_listener(ws.xdg_surface, &xdg_surface_listener, &ws);

	Rect2i positioner_rect;
	positioner_rect.size = parent.rect.size;
	struct xdg_surface* parent_xdg_surface = parent.xdg_surface;

	Point2i offset = ws.rect.position - parent.rect.position;

#ifdef LIBDECOR_ENABLED
	if (!parent_xdg_surface && parent.libdecor_frame) {
		parent_xdg_surface = libdecor_frame_get_xdg_surface(parent.libdecor_frame);

		int corner_x = 0;
		int corner_y = 0;
		libdecor_frame_translate_coordinate(parent.libdecor_frame, 0, 0, &corner_x, &corner_y);

		positioner_rect.position.x = corner_x;
		positioner_rect.position.y = corner_y;

		positioner_rect.size.width -= corner_x;
		positioner_rect.size.height -= corner_y;
	}
#endif

	ERR_FAIL_NULL(parent_xdg_surface);

	struct xdg_positioner* xdg_positioner = xdg_wm_base_create_positioner(registry.xdg_wm_base);
	xdg_positioner_set_size(xdg_positioner, ws.rect.size.width, ws.rect.size.height);
	xdg_positioner_set_anchor(xdg_positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
	xdg_positioner_set_gravity(xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
	xdg_positioner_set_constraint_adjustment(
		xdg_positioner, XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
							XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y |
							XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_X |
							XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y);
	xdg_positioner_set_anchor_rect(xdg_positioner, positioner_rect.position.x,
		positioner_rect.position.y, positioner_rect.size.width, positioner_rect.size.height);
	xdg_positioner_set_offset(xdg_positioner, offset.x, offset.y);

	ws.xdg_popup = xdg_surface_get_popup(ws.xdg_surface, parent_xdg_surface, xdg_positioner);
	xdg_popup_add_listener(ws.xdg_popup, &xdg_popup_listener, &ws);

	xdg_positioner_destroy(xdg_positioner);

	ws.frame_callback = wl_surface_frame(ws.wl_surface);
	wl_callback_add_listener(ws.frame_callback, &frame_wl_callback_listener, &ws);

	wl_surface_commit(ws.wl_surface);
}

void WaylandThread::window_destroy(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (ws.xdg_popup) {
		xdg_popup_destroy(ws.xdg_popup);
	}

	if (ws.xdg_toplevel_decoration) {
		zxdg_toplevel_decoration_v1_destroy(ws.xdg_toplevel_decoration);
	}

	if (ws.xdg_toplevel) {
		xdg_toplevel_destroy(ws.xdg_toplevel);
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_unref(ws.libdecor_frame);
	}
#endif // LIBDECOR_ENABLED

	if (ws.wp_fractional_scale) {
		wp_fractional_scale_v1_destroy(ws.wp_fractional_scale);
	}

	if (ws.wp_color_management_surface) {
		wp_color_management_surface_v1_destroy(ws.wp_color_management_surface);
	}

	if (ws.wp_color_management_surface_feedback) {
		wp_color_management_surface_feedback_v1_destroy(ws.wp_color_management_surface_feedback);
	}

	if (ws.wp_viewport) {
		wp_viewport_destroy(ws.wp_viewport);
	}

	if (ws.frame_callback) {
		wl_callback_destroy(ws.frame_callback);
	}

	if (ws.xdg_surface) {
		xdg_surface_destroy(ws.xdg_surface);
	}

	if (ws.wl_surface) {
		wl_surface_destroy(ws.wl_surface);
	}

	if (ws.xdg_icon) {
		xdg_toplevel_icon_v1_destroy(ws.xdg_icon);
	}

	if (ws.icon_buffer) {
		wl_buffer_destroy(ws.icon_buffer);
	}

	// Before continuing, let's handle any leftover event that might still refer to
	// this window.
	wl_display_roundtrip(wl_display);

	// We can already clean up here, we're done.
	windows.erase(p_window_id);

	// Let's update the window hover.
	_window_hover();
}

bool WaylandThread::window_exists(DisplayServerEnums::WindowID p_window_id) const
{
	if (p_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return false;
	}

	return windows.has(p_window_id);
}

struct wl_surface* WaylandThread::window_get_wl_surface(
	DisplayServerEnums::WindowID p_window_id) const
{
	const WindowState* ws = windows.getptr(p_window_id);
	if (ws) {
		return ws->wl_surface;
	}

	return nullptr;
}

WaylandThread::WindowState* WaylandThread::window_get_state(
	DisplayServerEnums::WindowID p_window_id)
{
	return windows.getptr(p_window_id);
}

const WaylandThread::WindowState* WaylandThread::window_get_state(
	DisplayServerEnums::WindowID p_window_id) const
{
	return windows.getptr(p_window_id);
}

Size2i WaylandThread::window_set_size(
	DisplayServerEnums::WindowID p_window_id, const Size2i& p_size)
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), p_size);
	WindowState& ws = windows[p_window_id];

	double window_scale = window_state_get_scale_factor(&ws);

	if (ws.maximized) {
		// Can't do anything.
		return scale_vector2i(ws.rect.size, window_scale);
	}

	Size2i new_size = scale_vector2i(p_size, 1 / window_scale);

	if (ws.tiled_left && ws.tiled_right) {
		// Tiled left and right, we shouldn't change from our current width or else
		// it'll look wonky.
		new_size.width = ws.rect.size.width;
	}

	if (ws.tiled_top && ws.tiled_bottom) {
		// Tiled top and bottom. Same as above, but for the height.
		new_size.height = ws.rect.size.height;
	}

	if (ws.resizing && ws.rect.size.width > 0 && ws.rect.size.height > 0) {
		// The spec says that we shall not resize further than the config size. We can
		// resize less than that though.
		new_size = new_size.min(ws.rect.size);
	}

#ifdef LIBDECOR_ENABLED
	// NOTE: Older versions of libdecor (~2022) do not have a way to get the max
	// content size. Let's also check for its pointer so that we can preserve
	// compatibility with older distros.
	if (ws.libdecor_frame && libdecor_frame_get_max_content_size) {
		int max_width = new_size.width;
		int max_height = new_size.height;

		// NOTE: Max content size is dynamic on libdecor, as plugins can override it
		// to accommodate their decorations.
		libdecor_frame_get_max_content_size(ws.libdecor_frame, &max_width, &max_height);

		if (max_width > 0 && max_height > 0) {
			new_size.width = MIN(new_size.width, max_width);
			new_size.height = MIN(new_size.height, max_height);
		}
	}
#endif

	window_state_update_size(&ws, new_size.width, new_size.height);

	return scale_vector2i(new_size, window_scale).maxi(1);
}

void WaylandThread::beep() const
{
	if (registry.xdg_system_bell) {
		xdg_system_bell_v1_ring(registry.xdg_system_bell, nullptr);
	}
}

void WaylandThread::window_start_drag(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ws.xdg_toplevel) {
		xdg_toplevel_move(ws.xdg_toplevel, ss->wl_seat, ss->pointer_data.button_serial);
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_move(ws.libdecor_frame, ss->wl_seat, ss->pointer_data.button_serial);
	}
#endif
}

void WaylandThread::window_start_resize(
	DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!windows.has(p_window));
	WindowState& ws = windows[p_window];
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ws.xdg_toplevel) {
		xdg_toplevel_resize_edge edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
		switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
		} break;
		default:
			break;
		}
		xdg_toplevel_resize(ws.xdg_toplevel, ss->wl_seat, ss->pointer_data.button_serial, edge);
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_resize_edge edge = LIBDECOR_RESIZE_EDGE_NONE;
		switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			edge = LIBDECOR_RESIZE_EDGE_TOP_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			edge = LIBDECOR_RESIZE_EDGE_TOP;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			edge = LIBDECOR_RESIZE_EDGE_TOP_RIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			edge = LIBDECOR_RESIZE_EDGE_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			edge = LIBDECOR_RESIZE_EDGE_RIGHT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			edge = LIBDECOR_RESIZE_EDGE_BOTTOM_LEFT;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			edge = LIBDECOR_RESIZE_EDGE_BOTTOM;
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			edge = LIBDECOR_RESIZE_EDGE_BOTTOM_RIGHT;
		} break;
		default:
			break;
		}
		libdecor_frame_resize(ws.libdecor_frame, ss->wl_seat, ss->pointer_data.button_serial, edge);
	}
#endif
}

void WaylandThread::window_set_parent(
	DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowID p_parent_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	ERR_FAIL_COND(!windows.has(p_parent_id));

	WindowState& child = windows[p_window_id];
	child.parent_id = p_parent_id;

	WindowState& parent = windows[p_parent_id];

	// NOTE: We can't really unparent as, at the time of writing, libdecor
	// segfaults when trying to set a null parent. Hopefully unparenting is not
	// that common. Bummer.

#ifdef LIBDECOR_ENABLED
	if (child.libdecor_frame && parent.libdecor_frame) {
		libdecor_frame_set_parent(child.libdecor_frame, parent.libdecor_frame);
		return;
	}
#endif

	if (child.xdg_toplevel && parent.xdg_toplevel) {
		xdg_toplevel_set_parent(child.xdg_toplevel, parent.xdg_toplevel);
	}
}

void WaylandThread::window_set_max_size(
	DisplayServerEnums::WindowID p_window_id, const Size2i& p_size)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	Vector2i logical_max_size = scale_vector2i(p_size, 1 / window_state_get_scale_factor(&ws));

	if (ws.wl_surface && ws.xdg_toplevel) {
		xdg_toplevel_set_max_size(ws.xdg_toplevel, logical_max_size.width, logical_max_size.height);
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_set_max_content_size(
			ws.libdecor_frame, logical_max_size.width, logical_max_size.height);
	}

	// FIXME: I'm not sure whether we have to commit the surface for this to apply.
#endif
}

void WaylandThread::window_set_min_size(
	DisplayServerEnums::WindowID p_window_id, const Size2i& p_size)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	Size2i logical_min_size = scale_vector2i(p_size, 1 / window_state_get_scale_factor(&ws));

	if (ws.wl_surface && ws.xdg_toplevel) {
		xdg_toplevel_set_min_size(ws.xdg_toplevel, logical_min_size.width, logical_min_size.height);
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_set_min_content_size(
			ws.libdecor_frame, logical_min_size.width, logical_min_size.height);
	}

	// FIXME: I'm not sure whether we have to commit the surface for this to apply.
#endif
}

bool WaylandThread::window_can_set_mode(
	DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowMode p_window_mode) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	const WindowState& ws = windows[p_window_id];

	switch (p_window_mode) {
	case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
		// Looks like it's guaranteed.
		return true;
	};

	case DisplayServerEnums::WINDOW_MODE_MINIMIZED: {
#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			return libdecor_frame_has_capability(ws.libdecor_frame, LIBDECOR_ACTION_MINIMIZE);
		}
#endif // LIBDECOR_ENABLED

		return ws.can_minimize;
	};

	case DisplayServerEnums::WINDOW_MODE_MAXIMIZED: {
#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			// NOTE: libdecor doesn't seem to have a maximize capability query?
			// The fact that there's a fullscreen one makes me suspicious. Anyways,
			// let's act as if we always can.
			return true;
		}
#endif
		return ws.can_maximize;
	};

	case DisplayServerEnums::WINDOW_MODE_FULLSCREEN:
	case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN: {
#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			return libdecor_frame_has_capability(ws.libdecor_frame, LIBDECOR_ACTION_FULLSCREEN);
		}
#endif // LIBDECOR_ENABLED

		return ws.can_fullscreen;
	};
	}

	return false;
}

void WaylandThread::window_try_set_mode(
	DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::WindowMode p_window_mode)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (ws.mode == p_window_mode) {
		return;
	}

	// Don't waste time with hidden windows and whatnot. Behave like it worked.
#ifdef LIBDECOR_ENABLED
	if ((!ws.wl_surface || !ws.xdg_toplevel) && !ws.libdecor_frame) {
#else
	if (!ws.wl_surface || !ws.xdg_toplevel) {
#endif // LIBDECOR_ENABLED
		ws.mode = p_window_mode;
		return;
	}

	// Return back to a windowed state so that we can apply what the user asked.
	switch (ws.mode) {
	case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
		// Do nothing.
	} break;

	case DisplayServerEnums::WINDOW_MODE_MINIMIZED: {
		// We can't do much according to the xdg_shell protocol. I have no idea
		// whether this implies that we should return or who knows what. For now
		// we'll do nothing.
		// TODO: Test this properly.
	} break;

	case DisplayServerEnums::WINDOW_MODE_MAXIMIZED: {
		// Try to unmaximize. This isn't garaunteed to work actually, so we'll have
		// to check whether something changed.
		if (ws.xdg_toplevel) {
			xdg_toplevel_unset_maximized(ws.xdg_toplevel);
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			libdecor_frame_unset_maximized(ws.libdecor_frame);
		}
#endif // LIBDECOR_ENABLED
	} break;

	case DisplayServerEnums::WINDOW_MODE_FULLSCREEN:
	case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN: {
		// Same thing as above, unset fullscreen and check later if it worked.
		if (ws.xdg_toplevel) {
			xdg_toplevel_unset_fullscreen(ws.xdg_toplevel);
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			libdecor_frame_unset_fullscreen(ws.libdecor_frame);
		}
#endif // LIBDECOR_ENABLED
	} break;
	}
	// Roundtrip and hope that something changed.
	// TODO: Async?
	wl_display_roundtrip(wl_display);

	if (ws.mode != DisplayServerEnums::WINDOW_MODE_WINDOWED) {
		// The compositor refused our "normalization" request. It'd be useless or
		// unpredictable to attempt setting a new state. We're done.
		return;
	}

	// Ask the compositor to set the state indicated by the new mode.
	switch (p_window_mode) {
	case DisplayServerEnums::WINDOW_MODE_WINDOWED: {
		// Do nothing. We're already windowed.
	} break;

	case DisplayServerEnums::WINDOW_MODE_MINIMIZED: {
		if (!window_can_set_mode(p_window_id, p_window_mode)) {
			// Minimization is special (read below). Better not mess with it if the
			// compositor explicitly announces that it doesn't support it.
			break;
		}

		if (ws.xdg_toplevel) {
			xdg_toplevel_set_minimized(ws.xdg_toplevel);
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			libdecor_frame_set_minimized(ws.libdecor_frame);
		}
#endif // LIBDECOR_ENABLED
	   // We have no way to actually detect this state, so we'll have to report it
	   // manually to the engine (hoping that it worked). In the worst case it'll
	   // get reset by the next configure event.
		ws.mode = DisplayServerEnums::WINDOW_MODE_MINIMIZED;
	} break;

	case DisplayServerEnums::WINDOW_MODE_MAXIMIZED: {
		if (ws.xdg_toplevel) {
			xdg_toplevel_set_maximized(ws.xdg_toplevel);
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			libdecor_frame_set_maximized(ws.libdecor_frame);
		}
#endif // LIBDECOR_ENABLED
	} break;

	case DisplayServerEnums::WINDOW_MODE_FULLSCREEN:
	case DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN: {
		if (ws.xdg_toplevel) {
			xdg_toplevel_set_fullscreen(ws.xdg_toplevel, nullptr);
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			libdecor_frame_set_fullscreen(ws.libdecor_frame, nullptr);
		}
#endif // LIBDECOR_ENABLED
	} break;

	default: {
	} break;
	}
}

void WaylandThread::window_set_borderless(
	DisplayServerEnums::WindowID p_window_id, bool p_borderless)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (ws.xdg_toplevel_decoration) {
		if (p_borderless) {
			// We implement borderless windows by simply asking the compositor to let
			// us handle decorations (we don't).
			zxdg_toplevel_decoration_v1_set_mode(
				ws.xdg_toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
		}
		else {
			zxdg_toplevel_decoration_v1_set_mode(
				ws.xdg_toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
	}

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		bool visible_current = libdecor_frame_is_visible(ws.libdecor_frame);
		bool visible_target = !p_borderless;

		// NOTE: We have to do this otherwise we trip on a libdecor bug where it's
		// possible to destroy the frame more than once, by setting the visibility
		// to false multiple times and thus crashing.
		if (visible_current != visible_target) {
			print_verbose(vformat("Setting libdecor frame visibility to %s", visible_target));
			libdecor_frame_set_visibility(ws.libdecor_frame, visible_target);
		}
	}
#endif // LIBDECOR_ENABLED
}

void WaylandThread::window_set_title(
	DisplayServerEnums::WindowID p_window_id, const String& p_title)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_set_title(ws.libdecor_frame, p_title.utf8().get_data());
	}
#endif // LIBDECOR_ENABLE

	if (ws.xdg_toplevel) {
		xdg_toplevel_set_title(ws.xdg_toplevel, p_title.utf8().get_data());
	}
}

void WaylandThread::window_set_app_id(
	DisplayServerEnums::WindowID p_window_id, const String& p_app_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

#ifdef LIBDECOR_ENABLED
	if (ws.libdecor_frame) {
		libdecor_frame_set_app_id(ws.libdecor_frame, p_app_id.utf8().get_data());
		return;
	}
#endif // LIBDECOR_ENABLED

	if (ws.xdg_toplevel) {
		xdg_toplevel_set_app_id(ws.xdg_toplevel, p_app_id.utf8().get_data());
		return;
	}
}

void WaylandThread::set_icon(const Ref<Image>& p_icon, DisplayServerEnums::WindowID p_window_id)
{
	if (!registry.xdg_toplevel_icon_manager) {
		return;
	}

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (ws.xdg_icon) {
		xdg_toplevel_icon_v1_destroy(ws.xdg_icon);
	}
	if (ws.icon_buffer) {
		wl_buffer_destroy(ws.icon_buffer);
	}
	ws.icon_set = true;

	if (p_icon.is_valid()) {
		Size2i icon_size = p_icon->get_size();
		ERR_FAIL_COND(icon_size.width != icon_size.height);

		// NOTE: The stride is the width of the icon in bytes.
		uint32_t icon_stride = icon_size.width * 4;
		uint32_t data_size = icon_stride * icon_size.height;

		// We need a shared memory object file descriptor in order to create a
		// wl_buffer through wl_shm.
		int fd = WaylandThread::_allocate_shm_file(data_size);
		ERR_FAIL_COND(fd == -1);

		uint32_t* buffer_data =
			(uint32_t*)mmap(nullptr, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		// Create the Wayland buffer.
		struct wl_shm_pool* shm_pool = wl_shm_create_pool(registry.wl_shm, fd, data_size);
		ws.icon_buffer = wl_shm_pool_create_buffer(
			shm_pool, 0, icon_size.width, icon_size.height, icon_stride, WL_SHM_FORMAT_ARGB8888);
		wl_shm_pool_destroy(shm_pool);

		// Fill the cursor buffer with the image data.
		for (uint32_t index = 0; index < (uint32_t)(icon_size.width * icon_size.height); index++) {
			int row_index = index / icon_size.width;
			int column_index = (index % icon_size.width);

			buffer_data[index] = p_icon->get_pixel(column_index, row_index).to_argb32();

			// Wayland buffers, unless specified, require associated alpha, so we'll just
			// associate the alpha in-place.
			uint8_t* pixel_data = (uint8_t*)&buffer_data[index];
			pixel_data[0] = pixel_data[0] * pixel_data[3] / 255;
			pixel_data[1] = pixel_data[1] * pixel_data[3] / 255;
			pixel_data[2] = pixel_data[2] * pixel_data[3] / 255;
		}

		ws.xdg_icon = xdg_toplevel_icon_manager_v1_create_icon(registry.xdg_toplevel_icon_manager);
		xdg_toplevel_icon_v1_add_buffer(ws.xdg_icon, ws.icon_buffer, icon_size.width);

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			xdg_toplevel* toplevel = libdecor_frame_get_xdg_toplevel(ws.libdecor_frame);
			ERR_FAIL_NULL(toplevel);
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, toplevel, ws.xdg_icon);
		}
#endif
		if (ws.xdg_toplevel) {
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, ws.xdg_toplevel, ws.xdg_icon);
		}
	}
	else {
#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			xdg_toplevel* toplevel = libdecor_frame_get_xdg_toplevel(ws.libdecor_frame);
			ERR_FAIL_NULL(toplevel);
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, toplevel, nullptr);
		}
#endif
		if (ws.xdg_toplevel) {
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, ws.xdg_toplevel, nullptr);
		}
	}
}

void WaylandThread::set_default_icon(const Ref<Image>& p_icon)
{
	ERR_FAIL_COND(p_icon.is_null());

	Size2i icon_size = p_icon->get_size();
	ERR_FAIL_COND(icon_size.width != icon_size.height);

	if (!registry.xdg_toplevel_icon_manager) {
		return;
	}

	if (xdg_icon) {
		xdg_toplevel_icon_v1_destroy(xdg_icon);
	}

	if (icon_buffer) {
		wl_buffer_destroy(icon_buffer);
	}

	// NOTE: The stride is the width of the icon in bytes.
	uint32_t icon_stride = icon_size.width * 4;
	uint32_t data_size = icon_stride * icon_size.height;

	// We need a shared memory object file descriptor in order to create a
	// wl_buffer through wl_shm.
	int fd = WaylandThread::_allocate_shm_file(data_size);
	ERR_FAIL_COND(fd == -1);

	uint32_t* buffer_data =
		(uint32_t*)mmap(nullptr, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// Create the Wayland buffer.
	struct wl_shm_pool* shm_pool = wl_shm_create_pool(registry.wl_shm, fd, data_size);
	icon_buffer = wl_shm_pool_create_buffer(
		shm_pool, 0, icon_size.width, icon_size.height, icon_stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(shm_pool);

	// Fill the cursor buffer with the image data.
	for (uint32_t index = 0; index < (uint32_t)(icon_size.width * icon_size.height); index++) {
		int row_index = index / icon_size.width;
		int column_index = (index % icon_size.width);

		buffer_data[index] = p_icon->get_pixel(column_index, row_index).to_argb32();

		// Wayland buffers, unless specified, require associated alpha, so we'll just
		// associate the alpha in-place.
		uint8_t* pixel_data = (uint8_t*)&buffer_data[index];
		pixel_data[0] = pixel_data[0] * pixel_data[3] / 255;
		pixel_data[1] = pixel_data[1] * pixel_data[3] / 255;
		pixel_data[2] = pixel_data[2] * pixel_data[3] / 255;
	}

	xdg_icon = xdg_toplevel_icon_manager_v1_create_icon(registry.xdg_toplevel_icon_manager);
	xdg_toplevel_icon_v1_add_buffer(xdg_icon, icon_buffer, icon_size.width);

	if (Engine::is_editor_hint() || Engine::is_project_manager_hint()) {
		// Setting a name allows the godot icon to be overridden by a system theme.
		// We only want the project manager and editor to get themed,
		// Games will get icons with the protocol and themed icons with .desktop entries.
		// NOTE: should be synced with the icon name in misc/dist/linuxbsd/Godot.desktop
		xdg_toplevel_icon_v1_set_name(xdg_icon, "voltaire");
	}

	for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair : windows) {
		WindowState& ws = pair.value;
		if (ws.icon_set) {
			continue;
		}

#ifdef LIBDECOR_ENABLED
		if (ws.libdecor_frame) {
			xdg_toplevel* toplevel = libdecor_frame_get_xdg_toplevel(ws.libdecor_frame);
			ERR_FAIL_NULL(toplevel);
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, toplevel, xdg_icon);
		}
#endif
		if (ws.xdg_toplevel) {
			xdg_toplevel_icon_manager_v1_set_icon(
				registry.xdg_toplevel_icon_manager, ws.xdg_toplevel, xdg_icon);
		}
	}
}

DisplayServerEnums::WindowMode WaylandThread::window_get_mode(
	DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	const WindowState& ws = windows[p_window_id];

	return ws.mode;
}

void WaylandThread::window_request_attention(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (registry.xdg_activation) {
		// Window attention requests are done through the XDG activation protocol.
		xdg_activation_token_v1* xdg_activation_token =
			xdg_activation_v1_get_activation_token(registry.xdg_activation);
		xdg_activation_token_v1_add_listener(
			xdg_activation_token, &xdg_activation_token_listener, &ws);
		xdg_activation_token_v1_commit(xdg_activation_token);
	}
}

void WaylandThread::window_set_idle_inhibition(
	DisplayServerEnums::WindowID p_window_id, bool p_enable)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowState& ws = windows[p_window_id];

	if (p_enable) {
		if (ws.registry->wp_idle_inhibit_manager && !ws.wp_idle_inhibitor) {
			ERR_FAIL_NULL(ws.wl_surface);
			ws.wp_idle_inhibitor = zwp_idle_inhibit_manager_v1_create_inhibitor(
				ws.registry->wp_idle_inhibit_manager, ws.wl_surface);
		}
	}
	else {
		if (ws.wp_idle_inhibitor) {
			zwp_idle_inhibitor_v1_destroy(ws.wp_idle_inhibitor);
			ws.wp_idle_inhibitor = nullptr;
		}
	}
}

bool WaylandThread::window_get_idle_inhibition(DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	const WindowState& ws = windows[p_window_id];

	return ws.wp_idle_inhibitor != nullptr;
}

void WaylandThread::window_set_color_profile(
	DisplayServerEnums::WindowID p_window_id, ColorProfile p_profile)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	const WindowState& ws = windows[p_window_id];

	if (!ws.wp_color_management_surface) {
		return;
	}

	if (p_profile.named_transfer_function == WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_GAMMA22) {
		// This is largely a problem with some HDR monitors applying "glare compensation" or not.
		// For monitors which do not apply glare compensation, it is correct to report gamma22 like
		// we would on SDR. But there also exist monitors which apply glare compensation, on these
		// monitors it is correct to request compound_2_4. Since we have no way to know on which
		// type of monitor we are in, we unset the image description in the hopes that future
		// compositor features may include choosing a default transfer function.
		wp_color_management_surface_v1_unset_image_description(ws.wp_color_management_surface);
		return;
	}

	ColorManagementState* cms = wp_color_manager_get_state(registry.wp_color_manager);

	struct wp_image_description_creator_params_v1* builder =
		wp_color_manager_v1_create_parametric_creator(registry.wp_color_manager);
	wp_image_description_creator_params_v1_set_primaries_named(builder, p_profile.named_primary);
	wp_image_description_creator_params_v1_set_tf_named(builder, p_profile.named_transfer_function);

	if ((cms->supported_render_feature & WP_COLOR_MANAGER_V1_FEATURE_SET_LUMINANCES) > 0) {
		uint32_t min_luminance = static_cast<uint32_t>(p_profile.target_min_luminance * 10000);
		wp_image_description_creator_params_v1_set_luminances(
			builder, min_luminance, p_profile.target_max_luminance, p_profile.reference_luminance);
	}

	struct wp_image_description_v1* image_desc =
		wp_image_description_creator_params_v1_create(builder);
	wp_color_management_surface_v1_set_image_description(
		ws.wp_color_management_surface, image_desc, WP_COLOR_MANAGER_V1_RENDER_INTENT_PERCEPTUAL);
	wp_image_description_v1_destroy(image_desc);
}

WaylandThread::ScreenData WaylandThread::screen_get_data(int p_screen) const
{
	ERR_FAIL_INDEX_V(p_screen, registry.wl_outputs.size(), ScreenData());

	return wl_output_get_screen_state(registry.wl_outputs.get(p_screen))->data;
}

int WaylandThread::get_screen_count() const { return registry.wl_outputs.size(); }

bool WaylandThread::input_has_touch() const { return has_touch; }

DisplayServerEnums::WindowID WaylandThread::pointer_get_pointed_window_id() const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		TouchPoint* tp = ss->touch_points.getptr(ss->last_touch_id);

		// Let's determine the most recently used tablet tool.
		TabletToolState* max_ts = nullptr;
		for (struct zwp_tablet_tool_v2* tool : ss->tablet_tools) {
			TabletToolState* ts = wp_tablet_tool_get_state(tool);
			ERR_CONTINUE(ts == nullptr);

			TabletToolData& td = ts->data;

			if (!max_ts) {
				max_ts = ts;
				continue;
			}

			if (MAX(td.button_time, td.motion_time) >
				MAX(max_ts->data.button_time, max_ts->data.motion_time)) {
				max_ts = ts;
			}
		}

		const PointerData& pd = ss->pointer_data;

		Pair<uint32_t, DisplayServerEnums::WindowID> best;
		best.first = MAX(pd.button_time, pd.motion_time);
		best.second = pd.pointed_id;

		if (max_ts) {
			TabletToolData& td = max_ts->data;

			Pair<uint32_t, DisplayServerEnums::WindowID> cur;
			cur.first = MAX(td.button_time, td.motion_time);
			cur.second = td.proximal_id;

			if (cur.first > best.first && window_exists(cur.second)) {
				best = cur;
			}
		}

		if (tp) {
			Pair<uint32_t, DisplayServerEnums::WindowID> cur;
			cur.first = MAX(tp->down_time, tp->motion_time);
			cur.second = tp->touched_id;

			if (cur.first > best.first && window_exists(cur.second)) {
				best = cur;
			}
		}

		return best.second;
	}

	return DisplayServerEnums::INVALID_WINDOW_ID;
}

DisplayServerEnums::WindowID WaylandThread::pointer_get_last_pointed_window_id() const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		// Let's determine the most recently used tablet tool.
		TabletToolState* max_ts = nullptr;
		for (struct zwp_tablet_tool_v2* tool : ss->tablet_tools) {
			TabletToolState* ts = wp_tablet_tool_get_state(tool);
			ERR_CONTINUE(ts == nullptr);

			TabletToolData& td = ts->data;

			if (!max_ts) {
				max_ts = ts;
				continue;
			}

			if (MAX(td.button_time, td.motion_time) >
				MAX(max_ts->data.button_time, max_ts->data.motion_time)) {
				max_ts = ts;
			}
		}

		const PointerData& pd = ss->pointer_data;

		if (max_ts) {
			TabletToolData& td = max_ts->data;
			if (MAX(td.button_time, td.motion_time) > MAX(pd.button_time, pd.motion_time)) {
				return td.last_proximal_id;
			}
		}

		return ss->pointer_data.last_pointed_id;
	}

	return DisplayServerEnums::INVALID_WINDOW_ID;
}

void WaylandThread::pointer_set_constraint(PointerConstraint p_constraint)
{
	pointer_constraint = p_constraint;

	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);
	ERR_FAIL_NULL(ss);

	WindowState* ws = window_get_state(ss->pointer_data.pointed_id);
	if (ws) {
		seat_state_unlock_pointer(ss);
		if (pointer_constraint == PointerConstraint::LOCKED) {
			seat_state_lock_pointer(ss, ws->wl_surface);
			// Godot always expects a centered pointer when locked.
			seat_state_set_hint(ss, ws->rect.size.x / 2, ws->rect.size.y / 2);
		}
		else if (pointer_constraint == PointerConstraint::CONFINED) {
			seat_state_confine_pointer(ss, ws->wl_surface);
		}
	}
}

void WaylandThread::pointer_set_hint(const Point2i& p_hint)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);
	if (!ss) {
		return;
	}

	WindowState* ws = window_get_state(ss->pointer_data.pointed_id);
	if (!ws) {
		return;
	}

	// NOTE: It looks like it's not really recommended to convert from
	// "godot-space" to "wayland-space" and in general I received mixed feelings
	// discussing about this. I'm not really sure about the math behind this but,
	// oh well, we're setting a cursor hint. ¯\_(ツ)_/¯
	// See: https://oftc.irclog.whitequark.org/wayland/2023-08-23#1692756914-1692816818
	int hint_x = Math::round(p_hint.x / window_state_get_scale_factor(ws));
	int hint_y = Math::round(p_hint.y / window_state_get_scale_factor(ws));

	if (ss) {
		seat_state_set_hint(ss, hint_x, hint_y);
	}
}

void WaylandThread::pointer_warp(const Point2i& p_to)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);
	if (!ss) {
		return;
	}

	WindowState* ws = window_get_state(ss->pointer_data.pointed_id);
	if (!ws) {
		return;
	}

	// NOTE: This is for compositors that don't support the pointer-warp protocol.
	// It's hacked together and not guaranteed to work.
	if (registry.wp_pointer_warp == nullptr) {
		if (ss->constraint_warping) {
			return;
		}

		if (!ss->wp_locked_pointer) {
			seat_state_unlock_pointer(ss);
			seat_state_lock_pointer(ss, ws->wl_surface);
		}

		pointer_set_hint(p_to);

		ss->constraint_warping = true;
		ss->constraint_warp_committed = false;

		if (!ss->wp_locked_pointer) {
			seat_state_unlock_pointer(ss);
			seat_state_lock_pointer(ss, ws->wl_surface);
		}

		pointer_set_hint(p_to);

		// Gotta wait a frame... Not ideal but, y'know.

		return;
	}

	// NOTE: It looks like it's not really recommended to convert from
	// "godot-space" to "wayland-space" and in general I received mixed feelings
	// discussing about this. I'm not really sure about the math behind this but,
	// oh well. ¯\_(ツ)_/¯
	// See: https://oftc.irclog.whitequark.org/wayland/2023-08-23#1692756914-1692816818
	int wl_pos_x = Math::round(p_to.x / window_state_get_scale_factor(ws));
	int wl_pos_y = Math::round(p_to.y / window_state_get_scale_factor(ws));

	if (ss) {
		seat_state_warp_pointer(ss, wl_pos_x, wl_pos_y);
	}
}

WaylandThread::PointerConstraint WaylandThread::pointer_get_constraint() const
{
	return pointer_constraint;
}

uint32_t WaylandThread::pointer_get_button_mask() const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		return ss->pointer_data.pressed_button_mask;
	}

	return uint32_t();
}

Error WaylandThread::init()
{
#ifdef SOWRAP_ENABLED
#ifdef DEBUG_ENABLED
	int dylibloader_verbose = 1;
#else
	int dylibloader_verbose = 0;
#endif // DEBUG_ENABLED

	if (initialize_wayland_client(dylibloader_verbose) != 0) {
		WARN_PRINT("Can't load the Wayland client library.");
		return ERR_CANT_CREATE;
	}

	if (initialize_wayland_cursor(dylibloader_verbose) != 0) {
		WARN_PRINT("Can't load the Wayland cursor library.");
		return ERR_CANT_CREATE;
	}

	if (initialize_xkbcommon(dylibloader_verbose) != 0) {
		WARN_PRINT("Can't load the XKBcommon library.");
		return ERR_CANT_CREATE;
	}
#endif // SOWRAP_ENABLED

	KeyMappingXKB::initialize();

	String embedder_socket_path;

#ifdef TOOLS_ENABLED
	bool embedder_enabled = true;

	if (OS::get_environment("GODOT_WAYLAND_DISABLE_EMBEDDER") == "1") {
		print_verbose("Disabling Wayland embedder as per GODOT_WAYLAND_DISABLE_EMBEDDER.");
		embedder_enabled = false;
	}

	if (embedder_enabled && Engine::is_editor_hint() && !Engine::is_project_manager_hint()) {
		print_verbose("Initializing Wayland embedder.");
		bool embedder_debug =
			OS::get_environment("GODOT_WAYLAND_EMBEDDER_DEBUG") == "1";
		Error embedder_status = embedder.init(embedder_debug);
		ERR_FAIL_COND_V_MSG(
			embedder_status != OK, ERR_CANT_CREATE, "Can't initialize Wayland embedder.");

		embedder_socket_path = embedder.get_socket_path();
		ERR_FAIL_COND_V_MSG(embedder_socket_path.is_empty(), ERR_CANT_CREATE,
			"Wayland embedder returned invalid path.");

		OS::set_environment("GODOT_WAYLAND_DISPLAY", embedder_socket_path);
	}
#endif // TOOLS_ENABLED

	if (Engine::is_embedded_in_editor()) {
		embedder_socket_path = OS::get_environment("GODOT_WAYLAND_DISPLAY");
#if 0
		// Debug
		OS::set_environment("WAYLAND_DEBUG", "1");
		int fd = open("/tmp/gdembedded.log", O_CREAT | O_RDWR, 0666);
		dup2(fd, 1);
		dup2(fd, 2);
#endif
	}

	if (embedder_socket_path.is_empty()) {
		print_verbose("Connecting to the default Wayland display.");
		wl_display = wl_display_connect(nullptr);
	}
	else {
		print_verbose("Connecting to the Wayland embedder display.");
		wl_display = wl_display_connect(embedder_socket_path.utf8().get_data());
	}

	ERR_FAIL_NULL_V_MSG(wl_display, ERR_CANT_CREATE, "Can't connect to a Wayland display.");

	thread_data.wl_display = wl_display;

#ifdef LIBDECOR_ENABLED
	bool libdecor_found = true;

	bool skip_libdecor =
		OS::get_environment("GODOT_WAYLAND_DISABLE_LIBDECOR") == "1";

#ifdef SOWRAP_ENABLED
	if (!skip_libdecor && initialize_libdecor(dylibloader_verbose) != 0) {
		libdecor_found = false;
	}
#endif // SOWRAP_ENABLED

	if (skip_libdecor) {
		print_verbose(
			"Skipping libdecor check because GODOT_WAYLAND_DISABLE_LIBDECOR is set to 1.");
	}
	else {
		if (libdecor_found) {
			libdecor_context =
				libdecor_new(wl_display, (struct libdecor_interface*)&libdecor_interface);
		}
		else {
			print_verbose("libdecor not found. Client-side decorations disabled.");
		}
	}
#endif // LIBDECOR_ENABLED

	wl_registry = wl_display_get_registry(wl_display);

	ERR_FAIL_NULL_V_MSG(wl_registry, ERR_UNAVAILABLE, "Can't obtain the Wayland registry global.");

	registry.wayland_thread = this;

	wl_registry_add_listener(wl_registry, &wl_registry_listener, &registry);

	// Wait for registry to get notified from the compositor.
	// TODO: Async?
	wl_display_roundtrip(wl_display);

	ERR_FAIL_NULL_V_MSG(
		registry.wl_shm, ERR_UNAVAILABLE, "Can't obtain the Wayland shared memory global.");
	ERR_FAIL_NULL_V_MSG(
		registry.wl_compositor, ERR_UNAVAILABLE, "Can't obtain the Wayland compositor global.");
	ERR_FAIL_NULL_V_MSG(
		registry.xdg_wm_base, ERR_UNAVAILABLE, "Can't obtain the Wayland XDG shell global.");

	// Embedded games can't access the decoration and icon protocol.
	if (!Engine::is_embedded_in_editor()) {
		if (!registry.xdg_decoration_manager) {
#ifdef LIBDECOR_ENABLED
			WARN_PRINT("Can't obtain the XDG decoration manager. Libdecor will be used for drawing "
					   "CSDs, if available.");
#else
			WARN_PRINT("Can't obtain the XDG decoration manager. Decorations won't show up.");
#endif // LIBDECOR_ENABLED
		}

		if (!registry.xdg_toplevel_icon_manager_name) {
			WARN_PRINT("xdg-toplevel-icon protocol not found! Cannot set window icon.");
		}
	}

	if (!registry.xdg_activation) {
		WARN_PRINT("Can't obtain the XDG activation global. Attention requesting won't work!");
	}

#ifndef DBUS_ENABLED
	if (!registry.wp_idle_inhibit_manager) {
		WARN_PRINT("Can't obtain the idle inhibition manager. The screen might turn off even after "
				   "calling screen_set_keep_on()!");
	}
#endif // DBUS_ENABLED

	if (!registry.wp_fifo_manager_name) {
		WARN_PRINT("FIFO protocol not found! Frame pacing will be degraded.");
	}

	// Wait for seat capabilities.
	// TODO: Async?
	wl_display_roundtrip(wl_display);

	cursor_theme_name = OS::get_environment("XCURSOR_THEME");

	unscaled_cursor_size = OS::get_environment("XCURSOR_SIZE").to_int();
	if (unscaled_cursor_size <= 0) {
		print_verbose("Detected invalid cursor size preference, defaulting to 24.");
		unscaled_cursor_size = 24;
	}

	// NOTE: The scale is useful here as it might've been updated by _update_scale.
	bool cursor_theme_loaded = _load_cursor_theme(unscaled_cursor_size * cursor_scale);

	if (!cursor_theme_loaded) {
		return ERR_CANT_CREATE;
	}

	// Update the cursor.
	cursor_set_shape(DisplayServerEnums::CURSOR_ARROW);

	events_thread.start(_poll_events_thread, &thread_data);

	initialized = true;
	return OK;
}

void WaylandThread::cursor_set_visible(bool p_visible)
{
	cursor_visible = p_visible;

	for (struct wl_seat* wl_seat : registry.wl_seats) {
		SeatState* ss = wl_seat_get_seat_state(wl_seat);
		ERR_FAIL_NULL(ss);

		seat_state_update_cursor(ss);
	}
}

void WaylandThread::cursor_set_shape(DisplayServerEnums::CursorShape p_cursor_shape)
{
	cursor_shape = p_cursor_shape;

	for (struct wl_seat* wl_seat : registry.wl_seats) {
		SeatState* ss = wl_seat_get_seat_state(wl_seat);
		ERR_FAIL_NULL(ss);

		seat_state_update_cursor(ss);
	}
}

void WaylandThread::cursor_shape_set_custom_image(
	DisplayServerEnums::CursorShape p_cursor_shape, Ref<Image> p_image, const Point2i& p_hotspot)
{
	ERR_FAIL_COND(p_image.is_null());

	Size2i image_size = p_image->get_size();

	// NOTE: The stride is the width of the image in bytes.
	unsigned int image_stride = image_size.width * 4;
	unsigned int data_size = image_stride * image_size.height;

	// We need a shared memory object file descriptor in order to create a
	// wl_buffer through wl_shm.
	int fd = WaylandThread::_allocate_shm_file(data_size);
	ERR_FAIL_COND(fd == -1);

	CustomCursor& cursor = custom_cursors[p_cursor_shape];
	cursor.hotspot = p_hotspot;

	if (cursor.wl_buffer) {
		// Clean up the old Wayland buffer.
		wl_buffer_destroy(cursor.wl_buffer);
	}

	if (cursor.buffer_data) {
		// Clean up the old buffer data.
		munmap(cursor.buffer_data, cursor.buffer_data_size);
	}

	cursor.buffer_data =
		(uint32_t*)mmap(nullptr, data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	cursor.buffer_data_size = data_size;

	// Create the Wayland buffer.
	struct wl_shm_pool* wl_shm_pool = wl_shm_create_pool(registry.wl_shm, fd, data_size);
	// TODO: Make sure that WL_SHM_FORMAT_ARGB8888 format is supported. It
	// technically isn't garaunteed to be supported, but I think that'd be a
	// pretty unlikely thing to stumble upon.
	cursor.wl_buffer = wl_shm_pool_create_buffer(
		wl_shm_pool, 0, image_size.width, image_size.height, image_stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(wl_shm_pool);

	// Fill the cursor buffer with the image data.
	for (unsigned int index = 0; index < (unsigned int)(image_size.width * image_size.height);
		 index++) {
		int row_index = std::floor(index / image_size.width);
		int column_index = (index % int(image_size.width));

		cursor.buffer_data[index] = p_image->get_pixel(column_index, row_index).to_argb32();

		// Wayland buffers, unless specified, require associated alpha, so we'll just
		// associate the alpha in-place.
		uint8_t* pixel_data = (uint8_t*)&cursor.buffer_data[index];
		pixel_data[0] = pixel_data[0] * pixel_data[3] / 255;
		pixel_data[1] = pixel_data[1] * pixel_data[3] / 255;
		pixel_data[2] = pixel_data[2] * pixel_data[3] / 255;
	}
}

void WaylandThread::cursor_shape_clear_custom_image(DisplayServerEnums::CursorShape p_cursor_shape)
{
	if (custom_cursors.has(p_cursor_shape)) {
		CustomCursor cursor = custom_cursors[p_cursor_shape];
		custom_cursors.erase(p_cursor_shape);

		if (cursor.wl_buffer) {
			wl_buffer_destroy(cursor.wl_buffer);
		}

		if (cursor.buffer_data) {
			munmap(cursor.buffer_data, cursor.buffer_data_size);
		}
	}
}

void WaylandThread::window_set_ime_active(
	const bool p_active, DisplayServerEnums::WindowID p_window_id)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->wp_text_input && ss->ime_enabled) {
		if (p_active) {
			ss->ime_active = true;
			zwp_text_input_v3_enable(ss->wp_text_input);
			zwp_text_input_v3_set_cursor_rectangle(ss->wp_text_input, ss->ime_rect.position.x,
				ss->ime_rect.position.y, ss->ime_rect.size.x, ss->ime_rect.size.y);
		}
		else {
			ss->ime_active = false;
			ss->ime_text = String();
			ss->ime_text_commit = String();
			ss->ime_cursor = Vector2i();
			zwp_text_input_v3_disable(ss->wp_text_input);
		}
		zwp_text_input_v3_commit(ss->wp_text_input);
	}
}

void WaylandThread::window_set_ime_position(
	const Point2i& p_pos, DisplayServerEnums::WindowID p_window_id)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->wp_text_input && ss->ime_enabled) {
		ss->ime_rect = Rect2i(p_pos, Size2i(1, 10));
		zwp_text_input_v3_set_cursor_rectangle(ss->wp_text_input, ss->ime_rect.position.x,
			ss->ime_rect.position.y, ss->ime_rect.size.x, ss->ime_rect.size.y);
		zwp_text_input_v3_commit(ss->wp_text_input);
	}
}

int WaylandThread::keyboard_get_layout_count() const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->xkb_keymap) {
		return xkb_keymap_num_layouts(ss->xkb_keymap);
	}

	return 0;
}

int WaylandThread::keyboard_get_current_layout_index() const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		return ss->current_layout_index;
	}

	return 0;
}

void WaylandThread::keyboard_set_current_layout_index(int p_index)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		ss->current_layout_index = p_index;
	}
}

String WaylandThread::keyboard_get_layout_name(int p_index) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->xkb_keymap) {
		return String::utf8(xkb_keymap_layout_get_name(ss->xkb_keymap, p_index));
	}

	return "";
}

Key WaylandThread::keyboard_get_key_from_physical(Key p_key) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->xkb_state) {
		Key modifiers = p_key & KeyModifierMask::MODIFIER_MASK;
		Key keycode_no_mod = p_key & KeyModifierMask::CODE_MASK;

		xkb_keycode_t xkb_keycode = KeyMappingXKB::get_xkb_keycode(keycode_no_mod);
		Key key = KeyMappingXKB::get_keycode(xkb_state_key_get_one_sym(ss->xkb_state, xkb_keycode));
		return (Key)(key | modifiers);
	}

	return p_key;
}

Key WaylandThread::keyboard_get_label_from_physical(Key p_key) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss && ss->xkb_state) {
		Key modifiers = p_key & KeyModifierMask::MODIFIER_MASK;
		Key keycode_no_mod = p_key & KeyModifierMask::CODE_MASK;

		xkb_keycode_t xkb_keycode = KeyMappingXKB::get_xkb_keycode(keycode_no_mod);
		xkb_keycode_t xkb_keysym = xkb_state_key_get_one_sym(ss->xkb_state, xkb_keycode);
		char32_t chr = xkb_keysym_to_utf32(xkb_keysym_to_upper(xkb_keysym));
		if (chr != 0) {
			String keysym = String::chr(chr);
			Key key = fix_key_label(keysym[0], KeyMappingXKB::get_keycode(xkb_keysym));
			return (Key)(key | modifiers);
		}
	}

	return p_key;
}

void WaylandThread::keyboard_echo_keys()
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		seat_state_echo_keys(ss);
	}
}

void WaylandThread::selection_set_text(const String& p_text)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (registry.wl_data_device_manager == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD(
			"Couldn't set selection, wl_data_device_manager global not available.");
		return;
	}

	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't set selection, current seat not set.");
		return;
	}

	if (ss->wl_data_device == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't set selection, seat doesn't have wl_data_device.");
		return;
	}

	ss->selection_data = p_text.to_utf8_buffer();

	if (ss->wl_data_source_selection != nullptr) {
		wl_data_source_destroy(ss->wl_data_source_selection);
		ss->wl_data_source_selection = nullptr;
	}

	ss->wl_data_source_selection =
		wl_data_device_manager_create_data_source(registry.wl_data_device_manager);
	wl_data_source_add_listener(ss->wl_data_source_selection, &wl_data_source_listener, ss);
	wl_data_source_offer(ss->wl_data_source_selection, "text/plain;charset=utf-8");
	wl_data_source_offer(ss->wl_data_source_selection, "text/plain");

	// TODO: Implement a good way of getting the latest serial from the user.
	wl_data_device_set_selection(ss->wl_data_device, ss->wl_data_source_selection,
		MAX(ss->pointer_data.button_serial, ss->last_key_pressed_serial));

	// Wait for the message to get to the server before continuing, otherwise the
	// clipboard update might come with a delay.
	wl_display_roundtrip(wl_display);
}

bool WaylandThread::selection_has_mime(const String& p_mime) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't get selection, current seat not set.");
		return false;
	}

	OfferState* os = wl_data_offer_get_offer_state(ss->wl_data_offer_selection);
	if (!os) {
		return false;
	}

	return os->mime_types.has(p_mime);
}

Vector<uint8_t> WaylandThread::selection_get_mime(const String& p_mime) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);
	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't get selection, current seat not set.");
		return Vector<uint8_t>();
	}

	if (ss->wl_data_source_selection) {
		// We have a source so the stuff we're pasting is ours. We'll have to pass the
		// data directly or we'd stall waiting for Godot (ourselves) to send us the
		// data :P

		OfferState* os = wl_data_offer_get_offer_state(ss->wl_data_offer_selection);
		ERR_FAIL_NULL_V(os, Vector<uint8_t>());

		if (os->mime_types.has(p_mime)) {
			// All righty, we're offering this type. Let's just return the data as is.
			return ss->selection_data;
		}

		// ... we don't offer that type. Oh well.
		return Vector<uint8_t>();
	}

	return _wl_data_offer_read(wl_display, p_mime.utf8().get_data(), ss->wl_data_offer_selection);
}

bool WaylandThread::primary_has_mime(const String& p_mime) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't get selection, current seat not set.");
		return false;
	}

	OfferState* os = wp_primary_selection_offer_get_offer_state(ss->wp_primary_selection_offer);
	if (!os) {
		return false;
	}

	return os->mime_types.has(p_mime);
}

Vector<uint8_t> WaylandThread::primary_get_mime(const String& p_mime) const
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);
	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't get primary, current seat not set.");
		return Vector<uint8_t>();
	}

	if (ss->wp_primary_selection_source) {
		// We have a source so the stuff we're pasting is ours. We'll have to pass the
		// data directly or we'd stall waiting for Godot (ourselves) to send us the
		// data :P

		OfferState* os = wp_primary_selection_offer_get_offer_state(ss->wp_primary_selection_offer);
		ERR_FAIL_NULL_V(os, Vector<uint8_t>());

		if (os->mime_types.has(p_mime)) {
			// All righty, we're offering this type. Let's just return the data as is.
			return ss->primary_data;
		}

		// ... we don't offer that type. Oh well.
		return Vector<uint8_t>();
	}

	return _wp_primary_selection_offer_read(
		wl_display, p_mime.utf8().get_data(), ss->wp_primary_selection_offer);
}

void WaylandThread::primary_set_text(const String& p_text)
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (registry.wp_primary_selection_device_manager == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't set primary, protocol not available.");
		return;
	}

	if (ss == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD("Couldn't set primary, current seat not set.");
		return;
	}

	if (ss->wp_primary_selection_device == nullptr) {
		DEBUG_LOG_WAYLAND_THREAD(
			"Couldn't set primary selection, seat doesn't have wp_primary_selection_device.");
		return;
	}

	ss->primary_data = p_text.to_utf8_buffer();

	if (ss->wp_primary_selection_source != nullptr) {
		zwp_primary_selection_source_v1_destroy(ss->wp_primary_selection_source);
		ss->wp_primary_selection_source = nullptr;
	}

	ss->wp_primary_selection_source = zwp_primary_selection_device_manager_v1_create_source(
		registry.wp_primary_selection_device_manager);
	zwp_primary_selection_source_v1_add_listener(
		ss->wp_primary_selection_source, &wp_primary_selection_source_listener, ss);
	zwp_primary_selection_source_v1_offer(
		ss->wp_primary_selection_source, "text/plain;charset=utf-8");
	zwp_primary_selection_source_v1_offer(ss->wp_primary_selection_source, "text/plain");

	// TODO: Implement a good way of getting the latest serial from the user.
	zwp_primary_selection_device_v1_set_selection(ss->wp_primary_selection_device,
		ss->wp_primary_selection_source,
		MAX(ss->pointer_data.button_serial, ss->last_key_pressed_serial));

	// Wait for the message to get to the server before continuing, otherwise the
	// clipboard update might come with a delay.
	wl_display_roundtrip(wl_display);
}

bool WaylandThread::supports_hdr() const
{
	ColorManagementState* color_state = wp_color_manager_get_state(registry.wp_color_manager);
	if (!color_state) {
		return false;
	}

	return color_state->supports_hdr;
}

void WaylandThread::commit_surfaces()
{
	for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair : windows) {
		wl_surface_commit(pair.value.wl_surface);
	}
}

void WaylandThread::set_frame() { frame = true; }

bool WaylandThread::get_reset_frame()
{
	bool old_frame = frame;
	frame = false;

	return old_frame;
}

// Wraps around `wl_display_dispatch_pending`. Judging from the docs, this
// should work pretty much like `wl_display_dispatch_timeout`, which is only
// available on somewhat recent versions of libwayland.
//
// This implementation is NOT based on libwayland's code.
int WaylandThread::wait_events(int p_timeout_ms)
{
	struct pollfd poll_fd;
	poll_fd.fd = wl_display_get_fd(wl_display);
	poll_fd.events = POLLIN | POLLHUP;

	while (wl_display_prepare_read(wl_display) != 0) {
		// Event queue got already something.
		return wl_display_dispatch_pending(wl_display);
	}

	wl_display_flush(wl_display);

	// Wait for the event file descriptor to have new data.
	poll(&poll_fd, 1, p_timeout_ms);

	if (poll_fd.revents | POLLIN) {
		// Load the queues with fresh new data.
		wl_display_read_events(wl_display);
	}
	else {
		// Oh well... Stop signaling that we want to read.
		wl_display_cancel_read(wl_display);

		// We've got no new events :(
		return 0;
	}

	// Let's try dispatching now...
	return wl_display_dispatch_pending(wl_display);
}

// Dispatches events until a frame event is received, a window is reported as
// suspended or the timeout expires.
bool WaylandThread::wait_frame_suspend_ms(int p_timeout)
{
	// This is a bit of a chicken and egg thing... Looks like the main event loop
	// has to call its rightfully forever-blocking poll right in between
	// `wl_display_prepare_read` and `wl_display_read`. This means, that it will
	// basically be guaranteed to stay stuck in a "prepare read" state, where it
	// will block any other attempt at reading the display fd, such as ours. The
	// solution? Let's make sure the mutex is locked (it should)...
	MutexLock mutex_lock(mutex);

	if (is_suspended()) {
		return false;
	}

	if (frame) {
		frame = false;
		return true;
	}

	ERR_FAIL_COND_V(Thread::get_caller_id() == events_thread.get_id(), false);

	// ...and unblock the main thread with a roundtrip!
	wl_display_roundtrip(wl_display);

	if (is_suspended()) {
		// All windows are suspended! The compositor is telling us _explicitly_ that
		// we don't need to draw, without letting us guess through the frame event's
		// timing and stuff like that. Our job here is done.
		return false;
	}

	if (frame) {
		// We already have a frame! Probably it got there while the caller locked :D
		frame = false;
		return true;
	}

	for (int remaining_ms = p_timeout; remaining_ms > 0;) {
		int begin_ms = OS::get_ticks_msec();

		wait_events(remaining_ms);

		_wl_display_check_error(wl_display);

		if (is_suspended()) {
			return false;
		}

		if (frame) {
			frame = false;
			return true;
		}

		remaining_ms -= OS::get_ticks_msec() - begin_ms;
	}

	DEBUG_LOG_WAYLAND_THREAD("Frame timeout.");
	return false;
}

uint64_t WaylandThread::window_get_last_frame_time(DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	return windows[p_window_id].last_frame_time;
}

bool WaylandThread::window_is_suspended(DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	return windows[p_window_id].suspended;
}

bool WaylandThread::is_fifo_available() const { return registry.wp_fifo_manager_name != 0; }

void WaylandThread::main_loop_callback()
{
	SeatState* ss = wl_seat_get_seat_state(wl_seat_current);

	if (ss) {
		seat_state_echo_keys(ss);

		// We need to wait a commit before "finalizing" the "legacy warp" hack. This
		// callback is called *after* a commit, but we don't know when the warp
		// request has been made, so we first wait one commit, then "finalize" on the
		// next. Additionally, we also wait for the pointer to be locked, as otherwise
		// it's not guaranteed for the position hint to even be acknowledged, AFAICT.
		if (ss->constraint_warping && ss->pointer_locked) {
			if (ss->constraint_warp_committed) {
				PointerConstraint old_constraint = pointer_constraint;
				pointer_set_constraint(PointerConstraint::NONE);
				pointer_set_constraint(old_constraint);

				ss->constraint_warping = false;
				ss->constraint_warp_committed = false;
			}
			else {
				ss->constraint_warp_committed = true;
			}
		}
	}
}

bool WaylandThread::is_suspended() const
{
	for (const KeyValue<DisplayServerEnums::WindowID, WindowState>& E : windows) {
		if (!E.value.suspended) {
			return false;
		}
	}

	return true;
}

bool WaylandThread::window_wait_ready(DisplayServerEnums::WindowID p_window_id, int p_timeout_ms)
{
	MutexLock mutex_lock(mutex);

	WindowState* ws = windows.getptr(p_window_id);
	ERR_FAIL_NULL_V(ws, false);

	if (ws->ready) {
		return true;
	}

	// See _poll_events_thread.
	ERR_FAIL_COND_V(Thread::get_caller_id() == events_thread.get_id(), ws->ready);

	// Unlock the thread.
	wl_display_roundtrip(wl_display);

	if (ws->ready) {
		return true;
	}

	for (int remaining_ms = p_timeout_ms; remaining_ms > 0;) {
		int begin_ms = OS::get_ticks_msec();

		wait_events(remaining_ms);

		_wl_display_check_error(wl_display);

		if (ws->ready) {
			return true;
		}

		remaining_ms -= OS::get_ticks_msec() - begin_ms;
	}

	return false;
}

struct godot_embedding_compositor* WaylandThread::get_embedding_compositor()
{
	return registry.godot_embedding_compositor;
}

ProcessID WaylandThread::embedded_compositor_get_focused_pid()
{
	EmbeddingCompositorState* ecomp_state =
		godot_embedding_compositor_get_state(registry.godot_embedding_compositor);
	ERR_FAIL_NULL_V(ecomp_state, -1);

	return ecomp_state->focused_pid;
}

void WaylandThread::destroy()
{
	if (!initialized) {
		return;
	}

	if (wl_display && events_thread.is_started()) {
		thread_data.thread_done.set();

		// By sending a roundtrip message we're unblocking the polling thread so that
		// it can realize that it's done and also handle every event that's left.
		wl_display_roundtrip(wl_display);

		events_thread.wait_to_finish();
	}

	if (!windows.is_empty()) {
		WARN_PRINT("Display server did not destroy all windows!");
		// Destroy all remaining windows.
		// FIXME: This code does not delete popups in order and might cause protocol
		// errors, as all the popup tree tracking and signaling is done in the display
		// server. We should figure whether the Wayland thread is the sole responsible
		// for cleanup.
		LocalVector<DisplayServerEnums::WindowID> window_ids;
		for (KeyValue<DisplayServerEnums::WindowID, WindowState>& pair : windows) {
			print_verbose(vformat("Window %d still allocated", pair.key));
			window_ids.push_back(pair.key);
		}

		for (LocalVector<DisplayServerEnums::WindowID>::Iterator E = window_ids.end();
			 E != window_ids.begin(); --E) {
			window_destroy(*E);
		}
	}

	// We can't iterate directly on `global_names` as it gets mutated by the global
	// remove handler.
	LocalVector<uint32_t> global_names_to_delete;
	for (uint32_t name : registry.global_names) {
		global_names_to_delete.push_back(name);
	}

	for (uint32_t name : global_names_to_delete) {
		_wl_registry_on_global_remove(wl_registry_get_user_data(wl_registry), wl_registry, name);
	}

	if (wl_cursor_theme) {
		wl_cursor_theme_destroy(wl_cursor_theme);
	}

	if (wl_registry) {
		wl_registry_destroy(wl_registry);
	}

	wl_display_roundtrip(wl_display);

	if (wl_display) {
		wl_display_disconnect(wl_display);
	}
}

void WaylandThread::_wp_tablet_tool_on_frame(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t time)
{
}

void WaylandThread::_wp_tablet_tool_on_proximity_out(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2)
{
}

void WaylandThread::_wp_tablet_tool_on_up(void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2)
{
}

void WaylandThread::_wp_tablet_tool_on_down(
	void* data, struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t serial)
{
}

void WaylandThread::_wp_tablet_tool_on_button(void* data,
	struct zwp_tablet_tool_v2* wp_tablet_tool_v2, uint32_t serial, uint32_t button, uint32_t state)
{
}

void WaylandThread::_wl_pointer_on_button(void* data, struct wl_pointer* wl_pointer,
	uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
}

void WaylandThread::_window_hover(int p_window) {}

bool WaylandThread::window_is_ready(DisplayServerEnums::WindowID p_window_id) const
{
	const WindowState* ws = window_get_state(p_window_id);
	if (!ws) {
		return false;
	}
	return ws->ready;
}

#endif // WAYLAND_ENABLED

#ifdef wl_array_for_each
#undef wl_array_for_each
#endif
#ifdef WAYLAND_THREAD_DEBUG_LOGS_ENABLED
#undef WAYLAND_THREAD_DEBUG_LOGS_ENABLED
#endif
#ifdef DEBUG_LOG_WAYLAND_THREAD
#undef DEBUG_LOG_WAYLAND_THREAD
#endif
#ifdef FIFO_INTERFACE_NAME
#undef FIFO_INTERFACE_NAME
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/wayland_embedder.cpp
// ============================================================================

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#ifdef WAYLAND_ENABLED

#ifdef TOOLS_ENABLED

#include <sys/stat.h>

#ifdef __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
// Assume Linux.
#include <linux/input-event-codes.h>
#endif

#include "core/os/os.h"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#define WAYLAND_EMBED_ID_MAX 1000

static bool debug_print_enabled = false;

#define DEBUG_LOG_WAYLAND_EMBED(...) \
	if (debug_print_enabled) { \
		fprintf(stderr, "[PROXY] %s\n", vformat(__VA_ARGS__).utf8().ptr()); \
	} else \
		((void)0)

//#define WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
#define DEBUG_LOG_WAYLAND_EMBED_VERBOSE(...) \
	if (1) { \
		fprintf(stderr, "[PROXY] %s\n", vformat(__VA_ARGS__).utf8().ptr()); \
		fflush(stdout); \
	} else \
		((void)0)

#else
#define DEBUG_LOG_WAYLAND_EMBED_VERBOSE(...)
#endif

// Wayland messages are structured with 32-bit words.
#define WL_WORD_SIZE (sizeof(uint32_t))

// Event opcodes. Request opcodes are defined in the generated client headers.
// We could generate server headers but they would clash (without modifications)
// and we use just a few constants anyways.

#define WL_DISPLAY_ERROR 0
#define WL_DISPLAY_DELETE_ID 1

#define WL_REGISTRY_GLOBAL 0
#define WL_REGISTRY_GLOBAL_REMOVE 1

#define WL_CALLBACK_DONE 0

#define WL_KEYBOARD_ENTER 1
#define WL_KEYBOARD_LEAVE 2
#define WL_KEYBOARD_KEY 3
#define WL_KEYBOARD_MODIFIERS 4

#define WL_POINTER_ENTER 0
#define WL_POINTER_LEAVE 1
#define WL_POINTER_BUTTON 3

#define WL_TOUCH_DOWN 0
#define WL_TOUCH_UP 1
#define WL_TOUCH_MOTION 2
#define WL_TOUCH_FRAME 3
#define WL_TOUCH_CANCEL 4
#define WL_TOUCH_SHAPE 5
#define WL_TOUCH_ORIENTATION 6

#define WL_SHM_FORMAT 0

#define WL_DRM_DEVICE 0
#define WL_DRM_FORMAT 1
#define WL_DRM_AUTHENTICATED 2
#define WL_DRM_CAPABILITIES 3

#define XDG_POPUP_CONFIGURE 0
#define XDG_SURFACE_CONFIGURE 0

#define XDG_TOPLEVEL_CONFIGURE 0
#define XDG_TOPLEVEL_CLOSE 1

#define ZWP_TABLET_SEAT_V2_TABLET_ADDED 0
#define ZWP_TABLET_SEAT_V2_TOOL_ADDED 1
#define ZWP_TABLET_SEAT_V2_PAD_ADDED 2

#define ZWP_TABLET_V2_REMOVED 4

#define ZWP_TABLET_TOOL_V2_PROXIMITY_IN 6
#define ZWP_TABLET_TOOL_V2_PROXIMITY_OUT 7
#define ZWP_TABLET_TOOL_V2_DOWN 8

#define GODOT_EMBEDDED_CLIENT_DISCONNECTED 0
#define GODOT_EMBEDDED_CLIENT_WINDOW_EMBEDDED 1
#define GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_IN 2
#define GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_OUT 3

#define GODOT_EMBEDDING_COMPOSITOR_CLIENT 0

size_t WaylandEmbedder::wl_array_word_offset(uint32_t p_size) {
	uint32_t pad = (WL_WORD_SIZE - (p_size % WL_WORD_SIZE)) % WL_WORD_SIZE;
	return (p_size + pad) / WL_WORD_SIZE;
}

const struct wl_interface *WaylandEmbedder::wl_interface_from_string(const char *name, size_t size) {
	for (size_t i = 0; i < (sizeof interfaces / sizeof *interfaces); ++i) {
		if (strncmp(name, interfaces[i]->name, size) == 0) {
			return interfaces[i];
		}
	}

	return nullptr;
}

int WaylandEmbedder::wl_interface_get_destructor_opcode(const struct wl_interface *p_iface, uint32_t version) {
	ERR_FAIL_NULL_V(p_iface, -1);

	// FIXME: Figure out how to extract the destructor from the XML files. This
	// value is not currently exposed by wayland-scanner.
	for (int i = 0; i < p_iface->method_count; ++i) {
		const struct wl_message &m = p_iface->methods[i];
		uint32_t destructor_version = String::to_int(m.signature);
		if (destructor_version <= version && (strcmp(m.name, "destroy") == 0 || strcmp(m.name, "release") == 0)) {
			return i;
		}
	}

	return -1;
}

struct WaylandEmbedder::WaylandObject *WaylandEmbedder::get_object(uint32_t p_global_id) {
	if (p_global_id == 0) {
		return nullptr;
	}

	// Server-allocated stuff starts at 0xff000000.
	bool is_server = p_global_id & 0xff000000;
	if (is_server) {
		p_global_id &= ~(0xff000000);
	}

#ifdef DEV_ENABLED
	if (p_global_id >= WAYLAND_EMBED_ID_MAX) {
		// Oh no. Time for debug info!

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
		for (uint32_t id = 1; id < objects.reserved_size(); ++id) {
			WaylandObject &object = objects[id];
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat(" - g0x%x (#%d): %s version %d, data 0x%x", id, id, object.interface->name, object.version, (uintptr_t)object.data));
		}
#endif // WAYLAND_EMBED_VERBOSE_LOGS_ENABLED

		CRASH_NOW_MSG(vformat("Tried to access ID bigger than debug cap (%d > %d).", p_global_id, WAYLAND_EMBED_ID_MAX));
	}
#endif // DEV_ENABLED

	if (is_server) {
		if (server_objects.size() <= p_global_id) {
			return nullptr;
		}

		return &server_objects[p_global_id];
	} else {
		if (objects.reserved_size() <= p_global_id) {
			return nullptr;
		}

		return &objects[p_global_id];
	}
}

Error WaylandEmbedder::delete_object(uint32_t p_global_id) {
	WaylandObject *object = get_object(p_global_id);
	ERR_FAIL_NULL_V(object, ERR_DOES_NOT_EXIST);

	if (object->shared) {
		ERR_FAIL_V_MSG(FAILED, vformat("Tried to delete shared object g0x%x.", p_global_id));
	}

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Deleting object %s g0x%x", object->interface ? object->interface->name : "UNKNOWN", p_global_id));

	if (object->data) {
		memdelete(object->data);
		object->data = nullptr;
	}

	bool is_server = p_global_id & 0xff000000;
	if (is_server) {
		server_objects[p_global_id & ~(0xff000000)] = WaylandObject();
	} else {
		objects.free(p_global_id);
	}

	registry_globals_names.erase(p_global_id);

	return OK;
}

uint32_t WaylandEmbedder::Client::allocate_server_id() {
	uint32_t new_id = INVALID_ID;

	if (free_server_ids.size() > 0) {
		int new_size = free_server_ids.size() - 1;
		new_id = free_server_ids[new_size] | 0xff000000;
		free_server_ids.resize_uninitialized(new_size);
	} else {
		new_id = allocated_server_ids | 0xff000000;

		++allocated_server_ids;
#ifdef DEV_ENABLED
		CRASH_COND_MSG(allocated_server_ids > WAYLAND_EMBED_ID_MAX, "Max server ID reached. This might indicate a leak.");
#endif // DEV_ENABLED
	}

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Allocated server-side id 0x%x.", new_id));

	return new_id;
}

struct WaylandEmbedder::WaylandObject *WaylandEmbedder::Client::get_object(uint32_t p_local_id) {
	if (p_local_id == INVALID_ID) {
		return nullptr;
	}

	if (global_instances.has(p_local_id)) {
		return &global_instances[p_local_id];
	}

	if (fake_objects.has(p_local_id)) {
		return &fake_objects[p_local_id];
	}

	if (!global_ids.has(p_local_id)) {
		return nullptr;
	}

	ERR_FAIL_NULL_V(embedder, nullptr);
	return embedder->get_object(get_global_id(p_local_id));
}

Error WaylandEmbedder::Client::bind_global_id(uint32_t p_global_id, uint32_t p_local_id) {
	ERR_FAIL_COND_V(local_ids.has(p_global_id), ERR_ALREADY_EXISTS);
	ERR_FAIL_COND_V(global_ids.has(p_local_id), ERR_ALREADY_EXISTS);

	GlobalIdInfo gid_info;
	gid_info.id = p_global_id;
	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Pushing g0x%x in the global id history", p_global_id));
	gid_info.history_elem = global_id_history.push_back(p_global_id);
	global_ids[p_local_id] = gid_info;

	local_ids[p_global_id] = p_local_id;

	return OK;
}

Error WaylandEmbedder::Client::delete_object(uint32_t p_local_id) {
	if (fake_objects.has(p_local_id)) {
#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
		WaylandObject *object = &fake_objects[p_local_id];
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Deleting fake object %s l0x%x", object->interface ? object->interface->name : "UNKNOWN", p_local_id));
#endif

		if (!(p_local_id & 0xff000000)) {
			send_wl_display_delete_id(DISPLAY_ID, p_local_id);
		}

		fake_objects.erase(p_local_id);

		// We can skip everything else below, as fake objects don't have a global id.
		return OK;
	}

	ERR_FAIL_COND_V(!global_ids.has(p_local_id), ERR_DOES_NOT_EXIST);
	GlobalIdInfo gid_info = global_ids[p_local_id];
	uint32_t global_id = gid_info.id;

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Erasing g0x%x from the global id history", global_id));
	global_id_history.erase(gid_info.history_elem);

	if (global_instances.has(p_local_id)) {
#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
		WaylandObject *object = &global_instances[p_local_id];
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Deleting global instance %s l0x%x", object->interface ? object->interface->name : "UNKNOWN", p_local_id));
#endif

		send_wl_display_delete_id(DISPLAY_ID, p_local_id);

		// We don't want to delete the global object tied to this instance, so we'll only get rid of the local stuff.
		global_instances.erase(p_local_id);
		global_ids.erase(p_local_id);

		if (global_id != INVALID_ID) {
			local_ids.erase(global_id);
		}

		// We're done here.
		return OK;
	}

	if (wl_registry_instances.has(p_local_id)) {
		wl_registry_instances.erase(p_local_id);
	}

	WaylandObject *object = embedder->get_object(global_id);
	ERR_FAIL_NULL_V(object, ERR_DOES_NOT_EXIST);

	ERR_FAIL_COND_V_MSG(object->shared, ERR_INVALID_PARAMETER, vformat("Tried to delete shared object g0x%x.", global_id));

	global_ids.erase(p_local_id);
	local_ids.erase(global_id);

	if (p_local_id & 0xff000000) {
		free_server_ids.push_back(p_local_id & ~(0xff000000));
	}

	uint32_t *global_name = embedder->registry_globals_names.getptr(global_id);
	if (global_name) {
		{
			RegistryGlobalInfo &info = embedder->registry_globals[*global_name];
			ERR_FAIL_COND_V_MSG(info.instance_counter == 0, ERR_BUG, "Instance counter inconsistency.");
			--info.instance_counter;

			if (info.destroyed && info.instance_counter == 0) {
				embedder->registry_globals.erase(*global_name);
			}
		}

		registry_globals_instances[*global_name].erase(p_local_id);
	}

	return embedder->delete_object(global_id);
}

String WaylandEmbedder::stringify_message_body(ProxyDirection p_direction, const struct wl_interface *p_interface, const struct wl_message *p_message, const uint32_t *body, const size_t body_bytes, bool normalized) {
	ERR_FAIL_NULL_V(p_interface, "[ERROR]");
	ERR_FAIL_NULL_V(p_message, "[ERROR]");

	const struct wl_interface &iface = *p_interface;
	const struct wl_message &msg = *p_message;
	const struct wl_message *message = p_message;

	String out = vformat("%s::%s(", iface.name, msg.name);

	size_t arg_idx = 0;
	size_t buf_idx = 0;
	size_t last_str_buf_idx = -1;
	uint32_t last_str_len = 0;
	for (size_t i = 0; i < strlen(message->signature); ++i) {
		ERR_FAIL_COND_V(buf_idx > (body_bytes / WL_WORD_SIZE), out + "[ERROR]");

		char sym = message->signature[i];
		if (sym >= '0' && sym <= '?') {
			// We don't care about version notices and nullability symbols. We can skip
			// those.
			continue;
		}

		switch (sym) {
			case 'i': {
				out += String::num_int64((int64_t)body[buf_idx]);
			} break;

			case 'u': {
				out += String::num_uint64(body[buf_idx]);
			} break;

			case 'f': {
				out += String::num(wl_fixed_to_double(body[buf_idx]));
			} break;

			case 'a': {
				uint32_t array_len = body[buf_idx];

				out += vformat("array[%s]", array_len);

				// We can't obviously go forward by just one byte. Let's skip to the end of
				// the array.
				buf_idx += wl_array_word_offset(array_len);
			} break;

			case 's': {
				uint32_t string_len = body[buf_idx];

				last_str_buf_idx = buf_idx;
				last_str_len = string_len;

				out += '"';
				out += String::utf8((const char *)(body + buf_idx + 1), string_len);
				out += '"';

				// Same as the array.
				buf_idx += wl_array_word_offset(string_len);
			} break;

			case 'n': {
				const wl_interface *new_interface = message->types[arg_idx];

				String new_interface_name = "[ERROR]";

				//uint32_t new_version = p_object->version;

				if (new_interface) {
					new_interface_name = new_interface->name;
				} else if (last_str_len != 0) {
					// When the protocol definition does not define an interface it reports a
					// string and an unsigned integer representing the interface and the
					// version requested.
					new_interface_name = String::utf8((char *)(body + last_str_buf_idx + 1), last_str_len);
					//new_version = body[arg_idx - 1];
				}

				bool is_local = false;

				// See below.
				if (p_direction == ProxyDirection::COMPOSITOR) {
					// From client. If not normalized (raw), is local, else global.
					is_local = !normalized;
				} else if (p_direction == ProxyDirection::CLIENT) {
					// From compositor. If not normalized (raw), is global, else local.
					is_local = normalized;
				}

				if (is_local) {
					out += vformat("new %s l0x%x", new_interface_name, body[buf_idx]);
				} else {
					out += vformat("new %s g0x%x", new_interface_name, body[buf_idx]);
				}
			} break;

			case 'o': {
				bool is_local = false;

				// This is technically an XNOR operation. I'd rather keep it "explicit" for
				// readability sake.
				//
				// In simple words, we're considering the possibility that a message is
				// "raw" from the client (and thus "untranslated"), or ready to be sent
				// as-is. We *could* only invoke this before sending, but doing it earlier
				// will allow us to catch weird behavior earlier, hopefully.
				if (p_direction == ProxyDirection::COMPOSITOR) {
					// From client. If not normalized (raw), is local, else global.
					is_local = !normalized;
				} else if (p_direction == ProxyDirection::CLIENT) {
					// From compositor. If not normalized (raw), is global, else local.
					is_local = normalized;
				}

				if (is_local) {
					out += vformat("l0x%x", body[buf_idx]);
				} else {
					out += vformat("g0x%x", body[buf_idx]);
				}
			} break;

			case 'h': {
				out += "fd";
			} break;
		}

		if (i != (strlen(message->signature) - 1)) {
			out += ",";
		}

		++arg_idx;
		++buf_idx;
	}

	out += ")";

	return out;
}

// Returns INVALID_ID if the creation fails. In that case, the user can assume
// that the client got kicked out.
uint32_t WaylandEmbedder::Client::new_object(uint32_t p_local_id, const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	if (embedder == nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, "No embedder set.");
		ERR_FAIL_V(INVALID_ID);
	}

	if (get_object(p_local_id) != nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, vformat("Tried to create %s l0x%x but it already exists as %s", p_interface->name, p_local_id, get_object(p_local_id)->interface->name));
		ERR_FAIL_V(INVALID_ID);
	}

	uint32_t new_global_id = embedder->new_object(p_interface, p_version, p_data);

	bind_global_id(new_global_id, p_local_id);

	return new_global_id;
}

uint32_t WaylandEmbedder::Client::new_server_object(uint32_t p_global_id, const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	if (embedder == nullptr) {
		socket_error(socket, get_local_id(p_global_id), WL_DISPLAY_ERROR_IMPLEMENTATION, "No embedder set.");
		ERR_FAIL_V(INVALID_ID);
	}

	uint32_t new_local_id = allocate_server_id();

	embedder->new_server_object(p_global_id, p_interface, p_version, p_data);

	bind_global_id(p_global_id, new_local_id);

	return new_local_id;
}

WaylandEmbedder::WaylandObject *WaylandEmbedder::Client::new_fake_object(uint32_t p_local_id, const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	if (embedder == nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, "No embedder set.");
		ERR_FAIL_V(nullptr);
	}

	if (get_object(p_local_id) != nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, vformat("Object l0x%x already exists", p_local_id));
		ERR_FAIL_V(nullptr);
	}

	WaylandObject &new_object = fake_objects[p_local_id];
	new_object.interface = p_interface;
	new_object.version = p_version;
	new_object.data = p_data;

	return &new_object;
}

WaylandEmbedder::WaylandObject *WaylandEmbedder::Client::new_global_instance(uint32_t p_local_id, uint32_t p_global_id, const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	if (embedder == nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, "No embedder set.");
		ERR_FAIL_V(nullptr);
	}

	if (get_object(p_local_id) != nullptr) {
		socket_error(socket, p_local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, vformat("Object l0x%x already exists", p_local_id));
		ERR_FAIL_V(nullptr);
	}

	WaylandObject &new_object = global_instances[p_local_id];
	new_object.interface = p_interface;
	new_object.version = p_version;
	new_object.data = p_data;

	// FIXME: Track each instance properly. Global instances (the compatibility
	// mechanism) are particular as they're the only case where a global ID might
	// map to multiple local objects. In that case we need to mirror each event
	// which passes a registry object as an argument for each instance.
	GlobalIdInfo gid_info;
	gid_info.id = p_global_id;
	gid_info.history_elem = global_id_history.push_back(p_global_id);
	global_ids[p_local_id] = gid_info;

	// NOTE: Normally, for each client, there's a single local object per global
	// object, but global instances break this expectation. This is technically
	// wrong but should work fine, as we have special logic whenever needed.
	//
	// TODO: it might be nice to enforce that this table is never looked up for
	// global instances or even just log attempts.
	local_ids[p_global_id] = p_local_id;

	return &new_object;
}

Error WaylandEmbedder::Client::handle_wl_drm_bind(uint32_t p_id, WaylandDrmGlobalData *p_state) {
	ERR_FAIL_NULL_V(p_state, ERR_INVALID_PARAMETER);

	if (p_state->device.is_empty()) {
		// Not yet initialized.
		return OK;
	}

	send_wl_drm_device(p_id, p_state->device);

	for (uint32_t format : p_state->formats) {
		Error err = send_wl_drm_format(p_id, format);
		ERR_FAIL_COND_V(err != OK, err);
	}

	if (p_state->authenticated) {
		Error err = send_wl_drm_authenticated(p_id);
		ERR_FAIL_COND_V(err != OK, err);
	}

	Error err = send_wl_drm_capabilities(p_id, p_state->capabilities);
	ERR_FAIL_COND_V(err != OK, err);

	return OK;
}

Error WaylandEmbedder::Client::send_wl_display_delete_id(uint32_t wl_display, uint32_t id) {
	return send_wayland_event(socket, wl_display, wl_display_interface, WL_DISPLAY_DELETE_ID, { wl_arg_uint(id) });
}

Error WaylandEmbedder::Client::send_wl_display_error(uint32_t wl_display, uint32_t object_id, uint32_t code, String &message) {
	return send_wayland_event(socket, wl_display, wl_display_interface, WL_DISPLAY_ERROR, { wl_arg_object(object_id), wl_arg_uint(code), wl_arg_string(message) });
}

Error WaylandEmbedder::Client::send_wl_registry_global(uint32_t wl_registry, uint32_t name, const char *interface, uint32_t version) {
	return send_wayland_event(socket, wl_registry, wl_registry_interface, WL_REGISTRY_GLOBAL, { wl_arg_uint(name), wl_arg_string(interface), wl_arg_uint(version) });
}

Error WaylandEmbedder::Client::send_wl_registry_global_remove(uint32_t wl_registry, uint32_t name) {
	return send_wayland_event(socket, wl_registry, wl_registry_interface, WL_REGISTRY_GLOBAL_REMOVE, { wl_arg_uint(name) });
}

Error WaylandEmbedder::Client::send_wl_keyboard_enter(uint32_t wl_keyboard, uint32_t serial, uint32_t surface, struct wl_array *keys) {
	return send_wayland_event(socket, wl_keyboard, wl_keyboard_interface, WL_KEYBOARD_ENTER, { wl_arg_uint(serial), wl_arg_object(surface), wl_arg_array(keys) });
}

Error WaylandEmbedder::Client::send_wl_keyboard_leave(uint32_t wl_keyboard, uint32_t serial, uint32_t surface) {
	return send_wayland_event(socket, wl_keyboard, wl_keyboard_interface, WL_KEYBOARD_LEAVE, { wl_arg_uint(serial), wl_arg_object(surface) });
}

// FIXME: If I format this "properly" (not that this matters as it's gonna be
// autogenerated), clang-format blasts this initializer list into space with a
// ton of indents. Just add a comma on the last element, you'll see.
Error WaylandEmbedder::Client::send_wl_keyboard_modifiers(uint32_t wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	return send_wayland_event(socket, wl_keyboard, wl_keyboard_interface, WL_KEYBOARD_MODIFIERS, { wl_arg_uint(serial), wl_arg_uint(mods_depressed), wl_arg_uint(mods_latched), wl_arg_uint(mods_locked), wl_arg_uint(group) });
}

Error WaylandEmbedder::Client::send_wl_shm_format(uint32_t wl_shm, uint32_t format) {
	return send_wayland_event(socket, wl_shm, wl_shm_interface, WL_SHM_FORMAT, { wl_arg_uint(format) });
}

Error WaylandEmbedder::Client::send_zwp_tablet_seat_v2_tablet_added(uint32_t zwp_tablet_seat_v2, uint32_t id) {
	return send_wayland_event(socket, zwp_tablet_seat_v2, zwp_tablet_seat_v2_interface, ZWP_TABLET_SEAT_V2_TABLET_ADDED, { wl_arg_new_id(id) });
}

Error WaylandEmbedder::Client::send_zwp_tablet_seat_v2_tool_added(uint32_t zwp_tablet_seat_v2, uint32_t id) {
	return send_wayland_event(socket, zwp_tablet_seat_v2, zwp_tablet_seat_v2_interface, ZWP_TABLET_SEAT_V2_TOOL_ADDED, { wl_arg_new_id(id) });
}

Error WaylandEmbedder::Client::send_zwp_tablet_tool_v2_proximity_in(uint32_t zwp_tablet_tool_v2, uint32_t serial, uint32_t tablet, uint32_t surface) {
	return send_wayland_event(socket, zwp_tablet_tool_v2, zwp_tablet_tool_v2_interface, ZWP_TABLET_TOOL_V2_PROXIMITY_IN, { wl_arg_uint(serial), wl_arg_object(tablet), wl_arg_object(surface) });
}

Error WaylandEmbedder::Client::send_xdg_surface_configure(uint32_t xdg_surface, uint32_t serial) {
	return send_wayland_event(socket, xdg_surface, xdg_surface_interface, XDG_SURFACE_CONFIGURE, { wl_arg_uint(serial) });
}

Error WaylandEmbedder::Client::send_xdg_toplevel_configure(uint32_t xdg_toplevel, uint32_t width, uint32_t height, struct wl_array *states) {
	return send_wayland_event(socket, xdg_toplevel, xdg_toplevel_interface, XDG_TOPLEVEL_CONFIGURE, { { wl_arg_uint(width), wl_arg_uint(height), wl_arg_array(states) } });
}

Error WaylandEmbedder::Client::send_xdg_toplevel_close(uint32_t xdg_toplevel) {
	return send_wayland_event(socket, xdg_toplevel, xdg_toplevel_interface, XDG_TOPLEVEL_CLOSE, {});
}

Error WaylandEmbedder::Client::send_xdg_popup_configure(uint32_t xdg_popup, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	return send_wayland_event(socket, xdg_popup, xdg_popup_interface, XDG_POPUP_CONFIGURE, { wl_arg_uint(x), wl_arg_uint(y), wl_arg_uint(width), wl_arg_uint(height) });
}

Error WaylandEmbedder::Client::send_wl_drm_device(uint32_t wl_drm, String &device) {
	return send_wayland_event(socket, wl_drm, wl_drm_interface, WL_DRM_DEVICE, { wl_arg_string(device) });
}

Error WaylandEmbedder::Client::send_wl_drm_format(uint32_t wl_drm, uint32_t format) {
	return send_wayland_event(socket, wl_drm, wl_drm_interface, WL_DRM_FORMAT, { wl_arg_uint(format) });
}

Error WaylandEmbedder::Client::send_wl_drm_authenticated(uint32_t wl_drm) {
	return send_wayland_event(socket, wl_drm, wl_drm_interface, WL_DRM_AUTHENTICATED, {});
}

Error WaylandEmbedder::Client::send_wl_drm_capabilities(uint32_t wl_drm, uint32_t capabilities) {
	return send_wayland_event(socket, wl_drm, wl_drm_interface, WL_DRM_CAPABILITIES, { wl_arg_uint(capabilities) });
}

// NOTE: Had to add `p_` prefix to `pid` due to shadowing issues. We'll likely
// have to generate static *functions* that accept a LocalObjectHandle instead
// of doing a Client *method*.
Error WaylandEmbedder::Client::send_godot_embedding_compositor_client(uint32_t godot_embedding_compositor, uint32_t client, uint32_t p_pid) {
	return send_wayland_event(socket, godot_embedding_compositor, godot_embedding_compositor_interface, GODOT_EMBEDDING_COMPOSITOR_CLIENT, { wl_arg_uint(client), wl_arg_uint(p_pid) });
}

Error WaylandEmbedder::Client::send_godot_embedded_client_disconnected(uint32_t godot_embedded_client) {
	return send_wayland_event(socket, godot_embedded_client, godot_embedded_client_interface, GODOT_EMBEDDED_CLIENT_DISCONNECTED, {});
}

Error WaylandEmbedder::Client::send_godot_embedded_client_window_embedded(uint32_t godot_embedded_client) {
	return send_wayland_event(socket, godot_embedded_client, godot_embedded_client_interface, GODOT_EMBEDDED_CLIENT_WINDOW_EMBEDDED, {});
}

Error WaylandEmbedder::Client::send_godot_embedded_client_window_focus_in(uint32_t godot_embedded_client) {
	return send_wayland_event(socket, godot_embedded_client, godot_embedded_client_interface, GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_IN, {});
}

Error WaylandEmbedder::Client::send_godot_embedded_client_window_focus_out(uint32_t godot_embedded_client) {
	return send_wayland_event(socket, godot_embedded_client, godot_embedded_client_interface, GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_OUT, {});
}

void WaylandEmbedder::cleanup_socket(int p_socket) {
	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Cleaning up socket %d.", p_socket));

	close(p_socket);

	for (size_t i = 0; i < pollfds.size(); ++i) {
		if (pollfds[i].fd == p_socket) {
			pollfds.remove_at_unordered(i);
			break;
		}
	}

	ERR_FAIL_COND(!clients.has(p_socket));

	Client &client = clients[p_socket];

	for (KeyValue<uint32_t, WaylandObject> &pair : client.fake_objects) {
		WaylandObject &object = pair.value;

		if (object.interface == &xdg_toplevel_interface) {
			XdgToplevelData *data = (XdgToplevelData *)object.data;
			CRASH_COND(data == nullptr);

			if (data->wl_subsurface_id != INVALID_ID) {
				// `xdg_toplevel`s are mapped to subsurfaces.
				send_wl_subsurface_destroy(data->wl_subsurface_id);
			}

			if (!data->xdg_surface_handle.get()) {
				continue;
			}

			XdgSurfaceData *xdg_surf_data = (XdgSurfaceData *)data->xdg_surface_handle.get()->data;
			if (xdg_surf_data == nullptr) {
				continue;
			}

			if (!data->parent_handle.get()) {
				continue;
			}

			XdgToplevelData *parent_data = (XdgToplevelData *)data->parent_handle.get()->data;
			if (parent_data == nullptr) {
				continue;
			}

			if (!parent_data->xdg_surface_handle.get()) {
				continue;
			}

			XdgSurfaceData *parent_xdg_surf_data = (XdgSurfaceData *)parent_data->xdg_surface_handle.get()->data;
			if (parent_xdg_surf_data == nullptr) {
				continue;
			}

			for (uint32_t wl_seat_name : wl_seat_names) {
				WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)registry_globals[wl_seat_name].data;
				if (global_seat_data == nullptr) {
					continue;
				}

				if (global_seat_data->focused_surface_id == xdg_surf_data->wl_surface_id) {
					seat_name_leave_surface(wl_seat_name, xdg_surf_data->wl_surface_id);
					seat_name_enter_surface(wl_seat_name, parent_xdg_surf_data->wl_surface_id);
				}
			}
		}
	}

	for (List<uint32_t>::Element *E = client.global_id_history.back(); E;) {
		uint32_t global_id = E->get();
		E = E->prev();

		WaylandObject *object = get_object(global_id);
		if (object == nullptr) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Skipping deletability check of object g0x%x as it's null.", global_id));
			continue;
		}

		if (object->interface == nullptr) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Skipping deletability check of object g0x%x as it's invalid.", global_id));
			continue;
		}

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Checking deletability of %s#g0x%x version %s", object->interface->name, global_id, object->version));

		if (object->shared) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Shared, skipping.");
			continue;
		}

		if (object->interface == &wl_callback_interface) {
			// Those things self-destruct.
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("wl_callback self destructs.");
			continue;
		}

		if (object->destroyed) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Already destroyed, skipping.");
			continue;
		}

		int destructor = wl_interface_get_destructor_opcode(object->interface, object->version);
		if (destructor >= 0) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Destroying %s#g0x%x", object->interface->name, global_id));

			if (object->interface == &wl_surface_interface) {
				for (uint32_t wl_seat_name : wl_seat_names) {
					WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)registry_globals[wl_seat_name].data;
					if (global_seat_data) {
						if (global_seat_data->pointed_surface_id == global_id) {
							global_seat_data->pointed_surface_id = INVALID_ID;
						}

						if (global_seat_data->focused_surface_id == global_id) {
							global_seat_data->focused_surface_id = INVALID_ID;
						}
					}
				}
			}

			send_wayland_method(compositor_socket, global_id, *object->interface, destructor, {});
			object->destroyed = true;

			if (global_id & 0xff000000) {
				delete_object(global_id);
				object = nullptr;
			}
		}

		if (object && !object->destroyed) {
			ERR_PRINT(vformat("Unreferenced object %s g0x%x (leak!)", object->interface->name, global_id));
		}
	}

	uint32_t eclient_id = client.embedded_client_id;

	clients.erase(client.socket);

	WaylandObject *eclient = main_client->get_object(eclient_id);

	if (eclient) {
		EmbeddedClientData *eclient_data = (EmbeddedClientData *)eclient->data;
		ERR_FAIL_NULL(eclient_data);

		if (!eclient_data->disconnected) {
			main_client->send_godot_embedded_client_disconnected(eclient_id);
		}

		eclient_data->disconnected = true;
	}
}

void WaylandEmbedder::socket_error(int p_socket, uint32_t p_object_id, uint32_t p_code, const String &p_message) {
	const char *err_name = "unknown";
	switch (p_code) {
		case WL_DISPLAY_ERROR_INVALID_OBJECT: {
			err_name = "invalid_object";
		} break;

		case WL_DISPLAY_ERROR_INVALID_METHOD: {
			err_name = "invalid_method";
		} break;

		case WL_DISPLAY_ERROR_NO_MEMORY: {
			err_name = "no_memory";
		} break;

		case WL_DISPLAY_ERROR_IMPLEMENTATION: {
			err_name = "implementation";
		} break;
	}

	ERR_PRINT(vformat("Socket %d %s error: %s", p_socket, err_name, p_message));

	LocalVector<union wl_argument> args;
	args.push_back(wl_arg_object(p_object_id));
	args.push_back(wl_arg_uint(p_code));
	args.push_back(wl_arg_string(vformat("[Godot Embedder] %s", p_message).utf8().get_data()));

	send_wayland_event(p_socket, DISPLAY_ID, wl_display_interface, WL_DISPLAY_ERROR, args);

	// So, here's the deal: from some extensive research I did, there are
	// absolutely zero safeguards for ensuring that the error message ends to the
	// client. It's absolutely tiny and takes _nothing_ to get there (less than
	// 4µs with a debug build on my machine), but still enough to get truncated in
	// the distance between `send_wayland_event` and `close`.
	//
	// Because of this we're going to give the client some slack: we're going to
	// wait for its socket to close (or whatever) or 1s, whichever happens first.
	//
	// Hopefully it's good enough for <1000 bytes :P
	struct pollfd pollfd = {};
	pollfd.fd = p_socket;

	int ret = poll(&pollfd, 1, 1'000);
	if (ret == 0) {
		ERR_PRINT("Client timeout while disconnecting.");
	}
	if (ret < 0) {
		ERR_PRINT(vformat("Client error while disconnecting: %s", strerror(errno)));
	}

	close(p_socket);
}

void WaylandEmbedder::poll_sockets() {
	if (poll(pollfds.ptr(), pollfds.size(), -1) == -1) {
		CRASH_NOW_MSG(vformat("poll() failed, errno %d.", errno));
	}

	// First handle everything but the listening socket (which is always the first
	// element), so that we can cleanup closed sockets before accidentally reusing
	// them (and breaking everything).
	for (size_t i = 1; i < pollfds.size(); ++i) {
		handle_fd(pollfds[i].fd, pollfds[i].revents);
	}

	handle_fd(pollfds[0].fd, pollfds[0].revents);
}

Error WaylandEmbedder::send_raw_message(int p_socket, std::initializer_list<struct iovec> p_vecs, const LocalVector<int> &p_fds) {
	struct msghdr msg = {};
	msg.msg_iov = (struct iovec *)p_vecs.begin();
	msg.msg_iovlen = p_vecs.size();

	if (!p_fds.is_empty()) {
		size_t data_size = p_fds.size() * sizeof(int);

		msg.msg_control = Memory::alloc_aligned_static(CMSG_SPACE(data_size), CMSG_ALIGN(1));
		msg.msg_controllen = CMSG_SPACE(data_size);

		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(data_size);

		// NOTE: According to the linux man page cmsg(5), we shall not access the
		// pointer returned CMSG_DATA directly, due to alignment concerns. We should
		// copy data from a suitably aligned object instead.
		memcpy(CMSG_DATA(cmsg), p_fds.ptr(), data_size);
	}

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
	printf("[PROXY] Sending: ");

	for (const struct iovec &vec : p_vecs) {
		for (size_t i = 0; i < vec.iov_len; ++i) {
			printf("%.2x", ((const uint8_t *)vec.iov_base)[i]);
		}
	}
	printf("\n");
#endif

	sendmsg(p_socket, &msg, MSG_NOSIGNAL);

	if (msg.msg_control) {
		Memory::free_aligned_static(msg.msg_control);
	}

	return OK;
}

Error WaylandEmbedder::send_raw_wayland_message(int p_socket, uint32_t p_id, uint32_t p_opcode, const uint32_t *p_args, const size_t p_args_words, bool p_log) {
	ERR_FAIL_COND_V(p_socket < 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_id == INVALID_ID, ERR_INVALID_PARAMETER);

	uint32_t args_size = p_args_words * sizeof *p_args;

	// Header is always 8 bytes long.
	uint32_t total_size = 8 + (args_size);

	uint32_t header[2] = { p_id, (total_size << 16) + p_opcode };

	struct iovec vecs[2] = {
		{ header, 8 },
		// According to the sendmsg manual, these buffers should never be written to,
		// so this cast should be safe.
		{ (void *)p_args, args_size },
	};

	struct msghdr msg = {};
	msg.msg_iov = vecs;
	msg.msg_iovlen = std_size(vecs);

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
	if (p_log) {
		printf("[PROXY] Sending: ");

		for (struct iovec &vec : vecs) {
			for (size_t i = 0; i < vec.iov_len; ++i) {
				printf("%.2x", ((const uint8_t *)vec.iov_base)[i]);
			}
		}
		printf("\n");
	}
#endif

	if (sendmsg(p_socket, &msg, MSG_NOSIGNAL) < 0) {
		return FAILED;
	}

	return OK;
}

Error WaylandEmbedder::send_wayland_message(ProxyDirection p_direction, int p_socket, uint32_t p_id, const struct wl_interface &p_interface, uint32_t p_opcode, const LocalVector<union wl_argument> &p_args) {
	ERR_FAIL_COND_V(p_direction == ProxyDirection::CLIENT && p_opcode >= (uint32_t)p_interface.event_count, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_direction == ProxyDirection::COMPOSITOR && p_opcode >= (uint32_t)p_interface.method_count, ERR_INVALID_PARAMETER);

	const struct wl_message &msg = p_direction == ProxyDirection::CLIENT ? p_interface.events[p_opcode] : p_interface.methods[p_opcode];

	LocalVector<uint32_t> arg_buf;

	size_t arg_idx = 0;
	for (size_t sig_idx = 0; sig_idx < strlen(msg.signature); ++sig_idx) {
		char sym = msg.signature[sig_idx];
		if (sym >= '0' && sym <= '?') {
			// We don't care about version notices and nullability symbols. We can skip
			// those.
			continue;
		}

		if (arg_idx >= p_args.size()) {
			String err_msg = vformat("Not enough arguments for r0x%d %s.%s(%s). Only got %d.", p_id, p_interface.name, msg.name, msg.signature, p_args.size());
			ERR_FAIL_COND_V_MSG(arg_idx >= p_args.size(), ERR_INVALID_PARAMETER, err_msg);
		}

		const union wl_argument &arg = p_args[arg_idx];

		switch (sym) {
			case 'i': {
				arg_buf.push_back((uint32_t)arg.i);
			} break;

			case 'u': {
				arg_buf.push_back(arg.u);
			} break;

			case 'f': {
				arg_buf.push_back((uint32_t)arg.f);
			} break;

			case 'o': {
				// We're encoding object arguments as uints because I don't think we can
				// reuse the whole opaque struct thing.
				arg_buf.push_back(arg.u);
			} break;

			case 'n': {
				arg_buf.push_back(arg.n);
			} break;

			case 's': {
				const char *str = p_args[arg_idx].s;
				// Wayland requires the string length to include the null terminator.
				uint32_t str_len = strlen(str) + 1;

				arg_buf.push_back(str_len);

				size_t data_begin_idx = arg_buf.size();

				uint32_t str_words = wl_array_word_offset(str_len);

				arg_buf.resize(arg_buf.size() + str_words);
				strcpy((char *)(arg_buf.ptr() + data_begin_idx), str);
			} break;

			case 'a': {
				const wl_array *arr = p_args[arg_idx].a;

				arg_buf.push_back(arr->size);

				size_t data_begin_idx = arg_buf.size();

				uint32_t words = wl_array_word_offset(arr->size);

				arg_buf.resize(arg_buf.size() + words);
				memcpy(arg_buf.ptr() + data_begin_idx, arr->data, arr->size);
			} break;

				// FDs (h) are encoded out-of-band.
		}

		++arg_idx;
	}

	DEBUG_LOG_WAYLAND_EMBED(vformat("Embedder -> #%s %s", p_socket, stringify_message_body(p_direction, &p_interface, &msg, arg_buf.ptr(), arg_buf.size() * WL_WORD_SIZE, true)));

	send_raw_wayland_message(p_socket, p_id, p_opcode, arg_buf.ptr(), arg_buf.size(), false);

	return OK;
}

uint32_t WaylandEmbedder::new_object(const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	uint32_t new_global_id = allocate_global_id();

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("New object g0x%x %s", new_global_id, p_interface->name));

	WaylandObject *new_object = get_object(new_global_id);
	new_object->interface = p_interface;
	new_object->version = p_version;
	new_object->data = p_data;

	return new_global_id;
}

WaylandEmbedder::WaylandObject *WaylandEmbedder::new_server_object(uint32_t p_global_id, const struct wl_interface *p_interface, int p_version, WaylandObjectData *p_data) {
	// The max ID will never increment more than one at a time, due to the
	// packed nature of IDs. libwayland already does similar assertions so it
	// just makes sense to double-check to avoid messing memory up or
	// allocating a huge buffer for nothing.
	uint32_t stripped_id = p_global_id & ~(0xff000000);

	ERR_FAIL_COND_V_MSG(stripped_id > server_objects.size(), nullptr, "Invalid new server id requested.");
	ERR_FAIL_COND_V_MSG(get_object(p_global_id) && get_object(p_global_id)->interface, nullptr, vformat("Tried to create %s g0x%x but it already exists as %s.", p_interface->name, p_global_id, get_object(p_global_id)->interface->name));

	if (stripped_id == server_objects.size()) {
		server_objects.resize(server_objects.size() + 1);
	}

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("New server object %s g0x%x", p_interface->name, p_global_id));

	WaylandObject *new_object = get_object(p_global_id);
	new_object->interface = p_interface;
	new_object->version = p_version;
	new_object->data = p_data;

	return new_object;
}

void WaylandEmbedder::sync() {
	CRASH_COND_MSG(sync_callback_id, "Sync already in progress.");

	sync_callback_id = allocate_global_id();
	get_object(sync_callback_id)->interface = &wl_callback_interface;
	get_object(sync_callback_id)->version = 1;
	send_wayland_method(compositor_socket, DISPLAY_ID, wl_display_interface, WL_DISPLAY_SYNC, { wl_arg_object(sync_callback_id) });

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Synchronizing");

	while (true) {
		poll_sockets();

		if (!sync_callback_id) {
			// Obj got deleted - sync is done.
			return;
		}
	}
}

// Returns the gid for the newly bound object, or an existing shared object if
// necessary.
uint32_t WaylandEmbedder::wl_registry_bind(uint32_t p_registry_id, uint32_t p_name, int p_version) {
	RegistryGlobalInfo &info = registry_globals[p_name];

	uint32_t id = INVALID_ID;

	if (wl_interface_get_destructor_opcode(info.interface, p_version) < 0) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Binding instanced global %s %d", info.interface->name, p_version));

		// Reusable object.
		if (info.reusable_objects.has(p_version) && info.reusable_objects[p_version] != INVALID_ID) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Already bound.");
			return info.reusable_objects[p_version];
		}

		id = new_object(info.interface, p_version);
		ERR_FAIL_COND_V(id == INVALID_ID, INVALID_ID);

		info.reusable_objects[p_version] = id;
		get_object(id)->shared = true;
	} else {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Binding global %s as g0x%x version %d", info.interface->name, id, p_version));
		id = new_object(info.interface, p_version);
	}

	ERR_FAIL_COND_V(id == INVALID_ID, INVALID_ID);

	registry_globals_names[id] = p_name;

	LocalVector<union wl_argument> args;
	args.push_back(wl_arg_uint(info.compositor_name));
	args.push_back(wl_arg_string(info.interface->name));
	args.push_back(wl_arg_int(p_version));
	args.push_back(wl_arg_new_id(id));

	Error err = send_wayland_method(compositor_socket, p_registry_id, wl_registry_interface, WL_REGISTRY_BIND, args);
	ERR_FAIL_COND_V_MSG(err != OK, INVALID_ID, "Error while sending bind request.");

	return id;
}

void WaylandEmbedder::seat_name_enter_surface(uint32_t p_seat_name, uint32_t p_wl_surface_id) {
	WaylandSurfaceData *surf_data = (WaylandSurfaceData *)get_object(p_wl_surface_id)->data;
	CRASH_COND(surf_data == nullptr);

	Client *client = surf_data->client;
	CRASH_COND(client == nullptr);

	if (!client->local_ids.has(p_wl_surface_id)) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Called seat_name_enter_surface with an unknown surface");
		return;
	}

	uint32_t local_surface_id = client->get_local_id(p_wl_surface_id);

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("KB: Entering surface g0x%x", p_wl_surface_id));

	WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)registry_globals[p_seat_name].data;
	ERR_FAIL_NULL(global_seat_data);

	for (uint32_t local_seat_id : client->registry_globals_instances[p_seat_name]) {
		WaylandSeatInstanceData *seat_data = (WaylandSeatInstanceData *)client->get_object(local_seat_id)->data;
		CRASH_COND(seat_data == nullptr);

		uint32_t local_keyboard_id = client->get_local_id(seat_data->wl_keyboard_id);

		if (local_keyboard_id != INVALID_ID) {
			// TODO: track keys. Not super important at the time of writing, since we
			// don't use that in the engine, although we should.
			// TODO: Accept nullptr for empty array.
			struct wl_array keys;
			wl_array_init(&keys);
			client->send_wl_keyboard_enter(local_keyboard_id, serial_counter++, local_surface_id, &keys);
			wl_array_release(&keys);

			// wl_keyboard::modifiers
			client->send_wl_keyboard_modifiers(local_keyboard_id, serial_counter++, global_seat_data->mods_depressed, global_seat_data->mods_latched, global_seat_data->mods_locked, global_seat_data->group);
		}
	}

	if (client->socket != main_client->socket) {
		main_client->send_godot_embedded_client_window_focus_in(client->embedded_client_id);
	}
}

void WaylandEmbedder::seat_name_leave_surface(uint32_t p_seat_name, uint32_t p_wl_surface_id) {
	WaylandSurfaceData *surf_data = (WaylandSurfaceData *)get_object(p_wl_surface_id)->data;
	CRASH_COND(surf_data == nullptr);

	Client *client = surf_data->client;
	CRASH_COND(client == nullptr);

	if (!client->local_ids.has(p_wl_surface_id)) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Called seat_name_leave_surface with an unknown surface!");
		return;
	}

	uint32_t local_surface_id = client->get_local_id(p_wl_surface_id);

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("KB: Leaving surface g0x%x", p_wl_surface_id));

	for (uint32_t local_seat_id : client->registry_globals_instances[p_seat_name]) {
		WaylandSeatInstanceData *seat_data = (WaylandSeatInstanceData *)client->get_object(local_seat_id)->data;
		CRASH_COND(seat_data == nullptr);

		uint32_t local_keyboard_id = client->get_local_id(seat_data->wl_keyboard_id);

		if (local_keyboard_id != INVALID_ID) {
			client->send_wl_keyboard_leave(local_keyboard_id, serial_counter++, local_surface_id);
		}
	}

	if (client != main_client) {
		main_client->send_godot_embedded_client_window_focus_out(client->embedded_client_id);
	}
}

int WaylandEmbedder::allocate_global_id() {
	uint32_t id = INVALID_ID;
	objects.request(id);
	objects[id] = WaylandObject();

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Allocated new global id g0x%x", id));

#ifdef DEV_ENABLED
	if (id > WAYLAND_EMBED_ID_MAX) {
		// Oh no. Time for debug info!

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
		for (uint32_t i = 1; i < objects.reserved_size(); ++i) {
			WaylandObject &object = objects[id];
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat(" - g0x%x (#%d): %s version %d, data 0x%x", i, i, object.interface->name, object.version, (uintptr_t)object.data));
		}
#endif // WAYLAND_EMBED_VERBOSE_LOGS_ENABLED

		CRASH_NOW_MSG("Max ID reached. This might indicate a leak.");
	}
#endif // DEV_ENABLED

	return id;
}

bool WaylandEmbedder::global_surface_is_window(uint32_t p_wl_surface_id) {
	WaylandObject *surface_object = get_object(p_wl_surface_id);
	ERR_FAIL_NULL_V(surface_object, false);
	if (surface_object->interface != &wl_surface_interface || surface_object->data == nullptr) {
		return false;
	}

	WaylandSurfaceData *surface_data = (WaylandSurfaceData *)surface_object->data;
	if (!surface_data->role_object_handle.get()) {
		return false;
	}

	WaylandObject *role_object = surface_data->role_object_handle.get();

	return (role_object && role_object->interface == &xdg_toplevel_interface);
}

bool WaylandEmbedder::handle_generic_msg(Client *client, const WaylandObject *p_object, const struct wl_message *message, const struct msg_info *info, uint32_t *buf, uint32_t instance_id) {
	// We allow client-less events.
	CRASH_COND(client == nullptr && info->direction == ProxyDirection::COMPOSITOR);

	ERR_FAIL_NULL_V(p_object, false);

	bool valid = true;

	// Let's strip the header.
	uint32_t *body = buf + 2;

	size_t arg_idx = 0;
	size_t buf_idx = 0;
	size_t last_str_buf_idx = -1;
	uint32_t last_str_len = 0;
	for (size_t i = 0; i < strlen(message->signature); ++i) {
		ERR_FAIL_COND_V(buf_idx > (info->size / sizeof *body), false);

		char sym = message->signature[i];
		if (sym >= '0' && sym <= '?') {
			// We don't care about version notices and nullability symbols. We can skip
			// those.
			continue;
		}

		switch (sym) {
			case 'a': {
				uint32_t array_len = body[buf_idx];

				// We can't obviously go forward by just one byte. Let's skip to the end of
				// the array.
				buf_idx += wl_array_word_offset(array_len);
			} break;

			case 's': {
				uint32_t string_len = body[buf_idx];

				last_str_buf_idx = buf_idx;
				last_str_len = string_len;

				// Same as the array.
				buf_idx += wl_array_word_offset(string_len);
			} break;

			case 'n': {
				uint32_t arg = body[buf_idx];

				const struct wl_interface *new_interface = message->types[arg_idx];
				uint32_t new_version = p_object->version;

				if (!new_interface && last_str_len != 0) {
					// When the protocol definition does not define an interface it reports a
					// string and an unsigned integer representing the interface and the
					// version requested.
					new_interface = wl_interface_from_string((char *)(body + last_str_buf_idx + 1), last_str_len);
					new_version = body[arg_idx - 1];
				}

				if (new_interface == nullptr) {
#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
					if (last_str_len > 0) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Unknown interface %s, marking packet as invalid.", (char *)(body + last_str_buf_idx + 1)));
					} else {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Unknown interface, marking packet as invalid.");
					}
#endif
					valid = false;
					break;
				}

				if (info->direction == ProxyDirection::COMPOSITOR) {
					// FIXME: Create objects only if the packet is valid.
					uint32_t new_local_id = arg;
					body[buf_idx] = client->new_object(new_local_id, new_interface, new_version);

					if (body[buf_idx] == INVALID_ID) {
						valid = false;
						break;
					}

				} else if (info->direction == ProxyDirection::CLIENT) {
					uint32_t new_global_id = arg;

					if (client) {
						body[buf_idx] = client->new_server_object(new_global_id, new_interface, new_version);
					} else {
						new_server_object(new_global_id, new_interface, new_version);
					}

					if (body[buf_idx] == INVALID_ID) {
						valid = false;
						break;
					}
				}
			} break;

			case 'o': {
				if (!client) {
					break;
				}

				uint32_t obj_id = body[buf_idx];
				if (obj_id == 0) {
					// Object arguments can be nil.
					break;
				}

				if (info->direction == ProxyDirection::CLIENT) {
					if (!client->local_ids.has(obj_id)) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Object argument g0x%x not found, marking packet as invalid.", obj_id));
						valid = false;
						break;
					}
					body[buf_idx] = instance_id != INVALID_ID ? instance_id : client->get_local_id(obj_id);
				} else if (info->direction == ProxyDirection::COMPOSITOR) {
					if (!client->global_ids.has(obj_id)) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Object argument l0x%x not found, marking packet as invalid.", obj_id));
						valid = false;
						break;
					}
					body[buf_idx] = client->get_global_id(obj_id);
				}
			} break;
		}

		++arg_idx;
		++buf_idx;
	}

	return valid;
}

WaylandEmbedder::MessageStatus WaylandEmbedder::handle_request(LocalObjectHandle p_object, uint32_t p_opcode, const uint32_t *msg_data, size_t msg_len) {
	ERR_FAIL_COND_V(!p_object.is_valid(), MessageStatus::HANDLED);

	WaylandObject *object = p_object.get();
	Client *client = p_object.get_client();

	ERR_FAIL_NULL_V(object, MessageStatus::HANDLED);

	// NOTE: Global ID may be null.
	uint32_t global_id = p_object.get_global_id();
	uint32_t local_id = p_object.get_local_id();

	ERR_FAIL_NULL_V(object->interface, MessageStatus::ERROR);
	const struct wl_interface *interface = object->interface;

	ERR_FAIL_COND_V((int)p_opcode >= interface->method_count, MessageStatus::ERROR);
	const struct wl_message message = interface->methods[p_opcode];

	const uint32_t *body = msg_data + 2;
	size_t body_len = msg_len - (WL_WORD_SIZE * 2);

	DEBUG_LOG_WAYLAND_EMBED(vformat("Client #%d -> %s l0x%x g0x%x", client->socket, stringify_message_body(ProxyDirection::COMPOSITOR, interface, &message, body, body_len), local_id, global_id));

	if (registry_globals_names.has(global_id)) {
		int global_name = registry_globals_names[global_id];
		ERR_FAIL_COND_V(!registry_globals.has(global_name), MessageStatus::ERROR);
		RegistryGlobalInfo &global_info = registry_globals[global_name];

		if (global_info.destroyed) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Skipping request for destroyed global object");
			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &wl_display_interface && p_opcode == WL_DISPLAY_GET_REGISTRY) {
		// The gist of this is that the registry is a global and the compositor can
		// quite simply take for granted that a single client can access any global
		// bound from any registry. Let's remove all doubts by using a single
		// registry (also for efficiency) and doing fancy remaps.
		uint32_t local_registry_id = body[0];

		// Note that the registry has already been allocated in the initialization
		// routine.

		for (KeyValue<uint32_t, RegistryGlobalInfo> &pair : registry_globals) {
			uint32_t global_name = pair.key;
			RegistryGlobalInfo &global_info = pair.value;

			if (global_info.destroyed) {
				continue;
			}

			const struct wl_interface *global_interface = global_info.interface;

			if (client != main_client && embedded_interface_deny_list.has(global_interface)) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Skipped global announcement %s for embedded client.", global_interface->name));
				continue;
			}

			client->send_wl_registry_global(local_registry_id, global_name, global_interface->name, global_info.version);
		}

		client->wl_registry_instances.insert(local_registry_id);
		client->new_global_instance(local_registry_id, REGISTRY_ID, &wl_registry_interface, 1);

		return MessageStatus::HANDLED;
	}

	if (object->interface == &wl_registry_interface) {
		if (p_opcode == WL_REGISTRY_BIND) {
			// [Request] wl_registry::bind(usun)
			uint32_t global_name = body[0];
			uint32_t interface_name_len = body[1];
			//const char *interface_name = (const char *)(body + 2);
			uint32_t version = body[2 + wl_array_word_offset(interface_name_len)];
			uint32_t new_local_id_idx = 2 + wl_array_word_offset(interface_name_len) + 1;
			uint32_t new_local_id = body[new_local_id_idx];

			if (!registry_globals.has(global_name)) {
				socket_error(client->socket, local_id, WL_DISPLAY_ERROR_INVALID_METHOD, vformat("Invalid global object #%d", global_name));
				return MessageStatus::HANDLED;
			}

			RegistryGlobalInfo &global_info = registry_globals[global_name];
			ERR_FAIL_NULL_V(global_info.interface, MessageStatus::ERROR);

			version = MIN(global_info.version, version);

			if (global_info.interface == &godot_embedding_compositor_interface) {
				if (!client->registry_globals_instances.has(global_name)) {
					client->registry_globals_instances[global_name] = {};
				}

				client->registry_globals_instances[global_name].insert(new_local_id);
				++global_info.instance_counter;
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Bound embedded compositor interface.");
				client->new_fake_object(new_local_id, &godot_embedding_compositor_interface, 1);
				return MessageStatus::HANDLED;
			}

			WaylandObject *instance = nullptr;

			client->registry_globals_instances[global_name].insert(new_local_id);
			++global_info.instance_counter;

			if (!client->registry_globals_instances.has(global_name)) {
				client->registry_globals_instances[global_name] = {};
			}

			uint32_t bind_gid = wl_registry_bind(REGISTRY_ID, global_name, version);
			if (bind_gid == INVALID_ID) {
				socket_error(client->socket, local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, "Bind failed.");
				return MessageStatus::HANDLED;
			}

			WaylandObject *bind_obj = get_object(bind_gid);
			if (bind_obj == nullptr) {
				socket_error(client->socket, local_id, WL_DISPLAY_ERROR_IMPLEMENTATION, "Bind failed.");
				return MessageStatus::HANDLED;
			}

			if (!bind_obj->shared) {
				client->bind_global_id(bind_gid, new_local_id);
				instance = bind_obj;
			} else {
				instance = client->new_global_instance(new_local_id, global_info.reusable_objects[version], global_info.interface, version);
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Instancing global #%d iface %s ver %d new id l0x%x g0x%x", global_name, global_info.interface->name, version, new_local_id, global_info.reusable_objects[version]));

				// Some interfaces report their state as soon as they're bound. Since
				// instances are handled by us, we need to track and report the relevant
				// data ourselves.
				if (global_info.interface == &wl_drm_interface) {
					Error err = client->handle_wl_drm_bind(new_local_id, (WaylandDrmGlobalData *)global_info.data);
					if (err != OK) {
						return MessageStatus::ERROR;
					}
				} else if (global_info.interface == &wl_shm_interface) {
					WaylandShmGlobalData *global_data = (WaylandShmGlobalData *)global_info.data;
					ERR_FAIL_NULL_V(global_data, MessageStatus::ERROR);

					for (uint32_t format : global_data->formats) {
						client->send_wl_shm_format(new_local_id, format);
					}
				}
			}

			ERR_FAIL_NULL_V(instance, MessageStatus::UNHANDLED);

			if (global_info.interface == &wl_seat_interface) {
				WaylandSeatInstanceData *new_data = memnew(WaylandSeatInstanceData);
				instance->data = new_data;
			}

			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &wl_compositor_interface && p_opcode == WL_COMPOSITOR_CREATE_SURFACE) {
		uint32_t new_local_id = body[0];

		WaylandSurfaceData *data = memnew(WaylandSurfaceData);
		data->client = client;

		uint32_t new_global_id = client->new_object(new_local_id, &wl_surface_interface, object->version, data);
		ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Keeping track of surface l0x%x g0x%x.", new_local_id, new_global_id));

		send_wayland_method(compositor_socket, global_id, wl_compositor_interface, WL_COMPOSITOR_CREATE_SURFACE, { wl_arg_object(new_global_id) });
		return MessageStatus::HANDLED;
	}

	if (object->interface == &wl_surface_interface) {
		WaylandSurfaceData *surface_data = (WaylandSurfaceData *)object->data;
		ERR_FAIL_NULL_V(surface_data, MessageStatus::ERROR);

		if (p_opcode == WL_SURFACE_DESTROY) {
			for (uint32_t wl_seat_name : wl_seat_names) {
				WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)registry_globals[wl_seat_name].data;
				ERR_FAIL_NULL_V(global_seat_data, MessageStatus::ERROR);

				if (global_seat_data->pointed_surface_id == global_id) {
					global_seat_data->pointed_surface_id = INVALID_ID;
				}

				if (global_seat_data->focused_surface_id == global_id) {
					global_seat_data->focused_surface_id = INVALID_ID;
				}
			}
		} else if (p_opcode == WL_SURFACE_COMMIT) {
			if (surface_data->role_object_handle.is_valid()) {
				WaylandObject *role_object = surface_data->role_object_handle.get();
				if (role_object && role_object->interface) {
					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("!!!!! Committed surface g0x%x with role object %s id l0x%x", global_id, role_object->interface->name, surface_data->role_object_handle.get_local_id()));
				}

				if (role_object && role_object->interface == &xdg_toplevel_interface) {
					XdgToplevelData *toplevel_data = (XdgToplevelData *)role_object->data;
					ERR_FAIL_NULL_V(toplevel_data, MessageStatus::ERROR);
					// xdg shell spec requires clients to first send data and then commit the
					// surface.

					if (toplevel_data->is_embedded() && !toplevel_data->configured) {
						toplevel_data->configured = true;
						client->send_xdg_surface_configure(toplevel_data->xdg_surface_handle.get_local_id(), serial_counter++);
					}
				}
			}

			send_wayland_method(compositor_socket, global_id, wl_surface_interface, WL_SURFACE_COMMIT, {});
			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &wl_seat_interface) {
		uint32_t global_seat_name = registry_globals_names[global_id];

		RegistryGlobalInfo &seat_global_info = registry_globals[global_seat_name];
		WaylandSeatGlobalData *global_data = (WaylandSeatGlobalData *)seat_global_info.data;
		ERR_FAIL_NULL_V(global_data, MessageStatus::ERROR);

		WaylandSeatInstanceData *instance_data = (WaylandSeatInstanceData *)object->data;
		ERR_FAIL_NULL_V(instance_data, MessageStatus::ERROR);

		if (p_opcode == WL_SEAT_GET_POINTER) {
			ERR_FAIL_COND_V(global_id == INVALID_ID, MessageStatus::ERROR);
			// [Request] wl_seat::get_pointer(n);
			uint32_t new_local_id = body[0];

			WaylandPointerData *new_data = memnew(WaylandPointerData);
			new_data->wl_seat_id = global_id;

			uint32_t new_global_id = client->new_object(new_local_id, &wl_pointer_interface, object->version, new_data);
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

			instance_data->wl_pointer_id = new_global_id;

			send_wayland_method(compositor_socket, global_id, wl_seat_interface, WL_SEAT_GET_POINTER, { wl_arg_new_id(new_global_id) });

			return MessageStatus::HANDLED;
		}

		if (p_opcode == WL_SEAT_GET_KEYBOARD) {
			ERR_FAIL_COND_V(global_id == INVALID_ID, MessageStatus::ERROR);
			// [Request] wl_seat::get_keyboard(n);
			uint32_t new_local_id = body[0];

			WaylandKeyboardData *new_data = memnew(WaylandKeyboardData);
			new_data->wl_seat_id = global_id;

			uint32_t new_global_id = client->new_object(new_local_id, &wl_keyboard_interface, object->version, new_data);
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

			instance_data->wl_keyboard_id = new_global_id;

			send_wayland_method(compositor_socket, global_id, wl_seat_interface, WL_SEAT_GET_KEYBOARD, { wl_arg_new_id(new_global_id) });

			return MessageStatus::HANDLED;
		}

		if (p_opcode == WL_SEAT_GET_TOUCH) {
			ERR_FAIL_COND_V(global_id == INVALID_ID, MessageStatus::ERROR);
			// [Request] wl_seat::get_touch(n);
			uint32_t new_local_id = body[0];

			WaylandTouchData *new_data = memnew(WaylandTouchData);
			new_data->wl_seat_id = global_id;

			uint32_t new_global_id = client->new_object(new_local_id, &wl_touch_interface, object->version, new_data);
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

			instance_data->wl_touch_id = new_global_id;

			send_wayland_method(compositor_socket, global_id, wl_seat_interface, WL_SEAT_GET_TOUCH, { wl_arg_new_id(new_global_id) });

			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &xdg_wm_base_interface) {
		if (p_opcode == XDG_WM_BASE_CREATE_POSITIONER) {
			uint32_t new_local_id = body[0];
			uint32_t new_global_id = client->new_object(new_local_id, &xdg_positioner_interface, object->version, memnew(XdgPositionerData));
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

			send_wayland_method(compositor_socket, global_id, xdg_wm_base_interface, XDG_WM_BASE_CREATE_POSITIONER, { wl_arg_new_id(new_global_id) });
			return MessageStatus::HANDLED;
		}

		if (p_opcode == XDG_WM_BASE_GET_XDG_SURFACE) {
			// [Request] xdg_wm_base::get_xdg_surface(no).
			uint32_t new_local_id = body[0];
			uint32_t surface_id = body[1];

			uint32_t global_surface_id = client->get_global_id(surface_id);

			bool fake = (client != main_client);

			XdgSurfaceData *data = memnew(XdgSurfaceData);
			data->wl_surface_id = global_surface_id;

			if (fake) {
				client->new_fake_object(new_local_id, &xdg_surface_interface, object->version, data);
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Created fake xdg_surface l0x%x for surface l0x%x", new_local_id, surface_id));
			} else {
				uint32_t new_global_id = client->new_object(new_local_id, &xdg_surface_interface, object->version, data);
				ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Created real xdg_surface l0x%x g0x%x for surface l0x%x", new_local_id, new_global_id, surface_id));

				send_wayland_method(compositor_socket, global_id, xdg_wm_base_interface, XDG_WM_BASE_GET_XDG_SURFACE, { wl_arg_new_id(new_global_id), wl_arg_object(global_surface_id) });
			}

			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &xdg_surface_interface) {
		XdgSurfaceData *xdg_surf_data = (XdgSurfaceData *)object->data;
		ERR_FAIL_NULL_V(xdg_surf_data, MessageStatus::ERROR);

		WaylandSurfaceData *surface_data = (WaylandSurfaceData *)get_object(xdg_surf_data->wl_surface_id)->data;
		ERR_FAIL_NULL_V(surface_data, MessageStatus::ERROR);

		bool is_embedded = client->fake_objects.has(local_id);

		if (p_opcode == XDG_SURFACE_GET_POPUP) {
			// [Request] xdg_surface::get_popup(no?o).

			uint32_t new_local_id = body[0];
			uint32_t local_parent_id = body[1];
			uint32_t local_positioner_id = body[2];

			surface_data->role_object_handle = LocalObjectHandle(client, new_local_id);

			XdgPopupData *popup_data = memnew(XdgPopupData);
			popup_data->parent_handle = LocalObjectHandle(client, local_parent_id);

			if (!is_embedded) {
				uint32_t new_global_id = client->new_object(new_local_id, &xdg_popup_interface, object->version, popup_data);
				ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

				uint32_t global_parent_id = client->get_global_id(local_parent_id);
				uint32_t global_positioner_id = client->get_global_id(local_positioner_id);
				send_wayland_method(compositor_socket, global_id, xdg_surface_interface, XDG_SURFACE_GET_POPUP, { wl_arg_new_id(new_global_id), wl_arg_object(global_parent_id), wl_arg_object(global_positioner_id) });

				return MessageStatus::HANDLED;
			}

			{
				// Popups are real, time to actually instantiate an xdg_surface.
				WaylandObject copy = *object;
				client->fake_objects.erase(local_id);

				global_id = client->new_object(local_id, copy.interface, copy.version, copy.data);
				ERR_FAIL_COND_V(global_id == INVALID_ID, MessageStatus::HANDLED);
				object = get_object(global_id);

				// xdg_wm_base::get_xdg_surface(no);
				send_wayland_method(compositor_socket, xdg_wm_base_id, xdg_wm_base_interface, XDG_WM_BASE_GET_XDG_SURFACE, { wl_arg_new_id(global_id), wl_arg_object(xdg_surf_data->wl_surface_id) });
			}

			uint32_t new_global_id = client->new_object(new_local_id, &xdg_popup_interface, object->version, popup_data);
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

			uint32_t global_parent_id = INVALID_ID;
			if (local_parent_id != INVALID_ID) {
				XdgSurfaceData *parent_xdg_surf_data = (XdgSurfaceData *)client->get_object(local_parent_id)->data;
				ERR_FAIL_NULL_V(parent_xdg_surf_data, MessageStatus::ERROR);

				WaylandSurfaceData *parent_surface_data = (WaylandSurfaceData *)get_object(parent_xdg_surf_data->wl_surface_id)->data;
				ERR_FAIL_NULL_V(parent_surface_data, MessageStatus::ERROR);

				WaylandObject *parent_role_obj = parent_surface_data->role_object_handle.get();
				ERR_FAIL_NULL_V(parent_role_obj, MessageStatus::ERROR);

				XdgPositionerData *pos_data = (XdgPositionerData *)client->get_object(local_positioner_id)->data;
				ERR_FAIL_NULL_V(pos_data, MessageStatus::ERROR);

				if (parent_role_obj->interface == &xdg_toplevel_interface) {
					XdgToplevelData *parent_toplevel_data = (XdgToplevelData *)parent_role_obj->data;
					ERR_FAIL_NULL_V(parent_toplevel_data, MessageStatus::ERROR);

					if (parent_toplevel_data->is_embedded()) {
						// Embedded windows are subsurfaces of a parent window. We need to
						// "redirect" the popup request on the parent window and adjust the
						// positioner properly if needed.

						XdgToplevelData *main_parent_toplevel_data = (XdgToplevelData *)parent_toplevel_data->parent_handle.get()->data;
						ERR_FAIL_NULL_V(main_parent_toplevel_data, MessageStatus::ERROR);

						global_parent_id = main_parent_toplevel_data->xdg_surface_handle.get_global_id();

						WaylandSubsurfaceData *subsurf_data = (WaylandSubsurfaceData *)get_object(parent_toplevel_data->wl_subsurface_id)->data;
						ERR_FAIL_NULL_V(subsurf_data, MessageStatus::ERROR);

						Point2i adj_pos = subsurf_data->position + pos_data->anchor_rect.position;

						// xdg_positioner::set_anchor_rect
						send_wayland_method(compositor_socket, client->get_global_id(local_positioner_id), xdg_positioner_interface, XDG_POSITIONER_SET_ANCHOR_RECT, { wl_arg_uint(adj_pos.x), wl_arg_uint(adj_pos.y), wl_arg_uint(pos_data->anchor_rect.size.width), wl_arg_uint(pos_data->anchor_rect.size.height) });
					}
				} else {
					global_parent_id = client->get_global_id(local_parent_id);
				}
			}

			send_wayland_method(compositor_socket, global_id, xdg_surface_interface, XDG_SURFACE_GET_POPUP, { wl_arg_new_id(new_global_id), wl_arg_object(global_parent_id), wl_arg_object(client->get_global_id(local_positioner_id)) });
			return MessageStatus::HANDLED;
		}

		if (p_opcode == XDG_SURFACE_GET_TOPLEVEL) {
			// [Request] xdg_surface::get_toplevel(n).
			uint32_t new_local_id = body[0];

			surface_data->role_object_handle = LocalObjectHandle(client, new_local_id);

			XdgToplevelData *data = memnew(XdgToplevelData);
			data->xdg_surface_handle = LocalObjectHandle(client, local_id);

			if (is_embedded) {
				client->new_fake_object(new_local_id, &xdg_toplevel_interface, object->version, data);
				client->embedded_window_id = new_local_id;

				main_client->send_godot_embedded_client_window_embedded(client->embedded_client_id);
			} else {
				uint32_t new_global_id = client->new_object(new_local_id, &xdg_toplevel_interface, object->version, data);
				ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::HANDLED);

				if (main_toplevel_id == 0) {
					main_toplevel_id = new_global_id;
					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("main toplevel set to gx0%x.", main_toplevel_id));
				}

				send_wayland_method(compositor_socket, global_id, xdg_surface_interface, XDG_SURFACE_GET_TOPLEVEL, { wl_arg_new_id(new_global_id) });
			}

			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &xdg_positioner_interface) {
		XdgPositionerData *pos_data = (XdgPositionerData *)object->data;
		ERR_FAIL_NULL_V(pos_data, MessageStatus::ERROR);

		if (p_opcode == XDG_POSITIONER_SET_ANCHOR_RECT) {
			// Args: int x, int y, int width, int height.
			pos_data->anchor_rect = Rect2i(body[0], body[1], body[2], body[3]);

			send_wayland_method(compositor_socket, global_id, xdg_positioner_interface, XDG_POSITIONER_SET_ANCHOR_RECT, { wl_arg_int(body[0]), wl_arg_int(body[1]), wl_arg_int(body[2]), wl_arg_int(body[3]) });
			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &xdg_toplevel_interface && p_opcode == XDG_TOPLEVEL_DESTROY) {
		if (client->fake_objects.has(local_id)) {
			XdgToplevelData *data = (XdgToplevelData *)object->data;
			ERR_FAIL_NULL_V(data, MessageStatus::ERROR);

			XdgSurfaceData *xdg_surf_data = nullptr;
			if (data->xdg_surface_handle.is_valid()) {
				xdg_surf_data = (XdgSurfaceData *)data->xdg_surface_handle.get()->data;
				ERR_FAIL_NULL_V(xdg_surf_data, MessageStatus::ERROR);
			}
			ERR_FAIL_NULL_V(xdg_surf_data, MessageStatus::ERROR);

			XdgSurfaceData *parent_xdg_surf_data = nullptr;
			{
				XdgToplevelData *parent_data = nullptr;
				if (data->parent_handle.get()) {
					parent_data = (XdgToplevelData *)data->parent_handle.get()->data;
					ERR_FAIL_NULL_V(parent_data, MessageStatus::ERROR);
				}

				if (parent_data && parent_data->xdg_surface_handle.get()) {
					parent_xdg_surf_data = (XdgSurfaceData *)parent_data->xdg_surface_handle.get()->data;
					ERR_FAIL_NULL_V(parent_xdg_surf_data, MessageStatus::ERROR);
				}
			}

			for (uint32_t wl_seat_name : wl_seat_names) {
				WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)registry_globals[wl_seat_name].data;
				ERR_FAIL_NULL_V(global_seat_data, MessageStatus::ERROR);

				if (global_seat_data->focused_surface_id == xdg_surf_data->wl_surface_id) {
					if (xdg_surf_data) {
						seat_name_leave_surface(wl_seat_name, xdg_surf_data->wl_surface_id);
					}

					if (parent_xdg_surf_data) {
						seat_name_enter_surface(wl_seat_name, parent_xdg_surf_data->wl_surface_id);
					}
				}
			}

			// FIXME: ??????
			send_wayland_method(client->socket, local_id, xdg_toplevel_interface, XDG_TOPLEVEL_DESTROY, {});

			if (local_id == client->embedded_window_id) {
				client->embedded_window_id = 0;
			}

			if (data->wl_subsurface_id != INVALID_ID) {
				send_wayland_method(compositor_socket, data->wl_subsurface_id, wl_subsurface_interface, WL_SUBSURFACE_DESTROY, {});
			}

			client->delete_object(local_id);

			return MessageStatus::HANDLED;
		}
	}

	if (interface == &zwp_tablet_manager_v2_interface) {
		if (p_opcode == ZWP_TABLET_MANAGER_V2_GET_TABLET_SEAT) {
			uint32_t new_local_id = body[0];
			LocalObjectHandle seat = LocalObjectHandle(client, body[1]);

			TabletSeatData *seat_data = memnew(TabletSeatData);
			seat_data->wl_seat_name = registry_globals_names[seat.get_global_id()];

			uint32_t new_global_id = client->new_object(new_local_id, &zwp_tablet_seat_v2_interface, object->version, seat_data);
			ERR_FAIL_COND_V(new_global_id == INVALID_ID, MessageStatus::ERROR);

			send_wayland_method(compositor_socket, global_id, zwp_tablet_manager_v2_interface, ZWP_TABLET_MANAGER_V2_GET_TABLET_SEAT, { wl_arg_new_id(new_global_id), wl_arg_object(seat.get_global_id()) });
			return MessageStatus::HANDLED;
		}
	}

	if (interface == &godot_embedded_client_interface) {
		EmbeddedClientData *eclient_data = (EmbeddedClientData *)object->data;
		ERR_FAIL_NULL_V(eclient_data, MessageStatus::ERROR);

		Client *eclient = eclient_data->client;
		ERR_FAIL_NULL_V(eclient, MessageStatus::ERROR);

		if (p_opcode == GODOT_EMBEDDED_CLIENT_DESTROY) {
			if (!eclient_data->disconnected) {
				close(eclient->socket);
			}

			client->delete_object(local_id);

			return MessageStatus::HANDLED;
		}

		if (eclient_data->disconnected) {
			// Object is inert.
			return MessageStatus::HANDLED;
		}

		ERR_FAIL_COND_V(eclient->embedded_window_id == 0, MessageStatus::ERROR);

		XdgToplevelData *toplevel_data = (XdgToplevelData *)eclient->get_object(eclient->embedded_window_id)->data;
		ERR_FAIL_NULL_V(toplevel_data, MessageStatus::ERROR);

		if (p_opcode == GODOT_EMBEDDED_CLIENT_SET_EMBEDDED_WINDOW_RECT && toplevel_data->wl_subsurface_id != INVALID_ID) {
			uint32_t x = body[0];
			uint32_t y = body[1];
			uint32_t width = body[2];
			uint32_t height = body[3];

			WaylandSubsurfaceData *subsurf_data = (WaylandSubsurfaceData *)get_object(toplevel_data->wl_subsurface_id)->data;
			ERR_FAIL_NULL_V(subsurf_data, MessageStatus::ERROR);

			toplevel_data->size.width = width;
			toplevel_data->size.height = height;

			subsurf_data->position.x = x;
			subsurf_data->position.y = y;

			send_wayland_method(compositor_socket, toplevel_data->wl_subsurface_id, wl_subsurface_interface, WL_SUBSURFACE_SET_POSITION, { wl_arg_int(x), wl_arg_int(y) });

			// TODO: Accept nullptr for empty array.
			struct wl_array states;
			wl_array_init(&states);
			eclient->send_xdg_toplevel_configure(eclient->embedded_window_id, width, height, &states);
			wl_array_release(&states);

			eclient->send_xdg_surface_configure(toplevel_data->xdg_surface_handle.get_local_id(), configure_serial_counter++);

			return MessageStatus::HANDLED;
		} else if (p_opcode == GODOT_EMBEDDED_CLIENT_SET_EMBEDDED_WINDOW_PARENT) {
			uint32_t main_client_parent_id = body[0];

			if (toplevel_data->parent_handle.get_local_id() == main_client_parent_id) {
				return MessageStatus::HANDLED;
			}

			if (main_client_parent_id == INVALID_ID && toplevel_data->wl_subsurface_id != INVALID_ID) {
				// Window hiding logic.

				send_wayland_method(compositor_socket, toplevel_data->wl_subsurface_id, wl_subsurface_interface, WL_SUBSURFACE_DESTROY, {});

				toplevel_data->parent_handle.invalidate();
				toplevel_data->wl_subsurface_id = INVALID_ID;

				return MessageStatus::HANDLED;
			}

			XdgToplevelData *parent_toplevel_data = (XdgToplevelData *)client->get_object(main_client_parent_id)->data;
			ERR_FAIL_NULL_V(parent_toplevel_data, MessageStatus::ERROR);
			XdgSurfaceData *parent_xdg_surf_data = (XdgSurfaceData *)parent_toplevel_data->xdg_surface_handle.get()->data;
			ERR_FAIL_NULL_V(parent_xdg_surf_data, MessageStatus::ERROR);

			XdgSurfaceData *xdg_surf_data = (XdgSurfaceData *)toplevel_data->xdg_surface_handle.get()->data;
			ERR_FAIL_NULL_V(xdg_surf_data, MessageStatus::ERROR);

			if (toplevel_data->wl_subsurface_id != INVALID_ID) {
				send_wayland_method(compositor_socket, toplevel_data->wl_subsurface_id, wl_subsurface_interface, WL_SUBSURFACE_DESTROY, {});
			}

			uint32_t new_sub_id = allocate_global_id();
			WaylandObject *new_sub_object = get_object(new_sub_id);
			new_sub_object->interface = &wl_subsurface_interface;
			new_sub_object->data = memnew(WaylandSubsurfaceData);
			new_sub_object->version = get_object(wl_subcompositor_id)->version;

			toplevel_data->wl_subsurface_id = new_sub_id;
			toplevel_data->parent_handle = LocalObjectHandle(main_client, main_client_parent_id);

			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Binding subsurface g0x%x.", new_sub_id));

			send_wayland_method(compositor_socket, wl_subcompositor_id, wl_subcompositor_interface, WL_SUBCOMPOSITOR_GET_SUBSURFACE, { wl_arg_new_id(new_sub_id), wl_arg_object(xdg_surf_data->wl_surface_id), wl_arg_object(parent_xdg_surf_data->wl_surface_id) });

			send_wayland_method(compositor_socket, new_sub_id, wl_subsurface_interface, WL_SUBSURFACE_SET_DESYNC, {});

			return MessageStatus::HANDLED;
		} else if (p_opcode == GODOT_EMBEDDED_CLIENT_FOCUS_WINDOW) {
			XdgSurfaceData *xdg_surf_data = (XdgSurfaceData *)toplevel_data->xdg_surface_handle.get()->data;
			ERR_FAIL_NULL_V(xdg_surf_data, MessageStatus::ERROR);

			for (uint32_t wl_seat_name : wl_seat_names) {
				RegistryGlobalInfo &global_seat_info = registry_globals[wl_seat_name];
				WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)global_seat_info.data;

				if (global_seat_data->focused_surface_id != INVALID_ID) {
					seat_name_leave_surface(wl_seat_name, global_seat_data->focused_surface_id);
				}
				global_seat_data->focused_surface_id = xdg_surf_data->wl_surface_id;

				seat_name_enter_surface(wl_seat_name, xdg_surf_data->wl_surface_id);
			}
		} else if (p_opcode == GODOT_EMBEDDED_CLIENT_EMBEDDED_WINDOW_REQUEST_CLOSE) {
			eclient->send_xdg_toplevel_close(eclient->embedded_window_id);
			return MessageStatus::HANDLED;
		}
	}

	// Server-allocated objects are a bit annoying to handle for us. Right now we
	// use a heuristic. See: https://ppaalanen.blogspot.com/2014/07/wayland-protocol-design-object-lifespan.html
	if (strcmp(message.name, "destroy") == 0 || strcmp(message.name, "release") == 0) {
		if (object->shared) {
			// We must not delete shared objects.
			client->delete_object(local_id);
			return MessageStatus::HANDLED;
		}

		if (global_id != INVALID_ID) {
			send_wayland_method(compositor_socket, global_id, *object->interface, p_opcode, {});
			object->destroyed = true;
		}

		if (local_id & 0xff000000) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("!!!!!! Deallocating server object l0x%x", local_id));
			client->delete_object(local_id);
		}

		return MessageStatus::HANDLED;
	}

	if (client->fake_objects.has(local_id)) {
		// Object is fake, we're done.
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Dropping unhandled request for fake object.");
		return MessageStatus::HANDLED;
	}

	if (global_id == INVALID_ID) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Dropping request with invalid global object id");
		return MessageStatus::HANDLED;
	}

	return MessageStatus::UNHANDLED;
}

WaylandEmbedder::MessageStatus WaylandEmbedder::handle_event(uint32_t p_global_id, LocalObjectHandle p_local_handle, uint32_t p_opcode, const uint32_t *msg_data, size_t msg_len) {
	WaylandObject *global_object = get_object(p_global_id);
	ERR_FAIL_NULL_V_MSG(global_object, MessageStatus::ERROR, "Compositor messages must always have a global object.");

	const uint32_t *body = msg_data + 2;
	size_t body_len = msg_len - (WL_WORD_SIZE * 2);

	if (debug_print_enabled) {
		ERR_FAIL_NULL_V(global_object->interface, MessageStatus::ERROR);
		const struct wl_interface *interface = global_object->interface;

		ERR_FAIL_COND_V((int)p_opcode >= interface->event_count, MessageStatus::ERROR);
		const struct wl_message message = interface->events[p_opcode];

		if (p_local_handle.is_valid()) {
			int socket = p_local_handle.get_client()->socket;
			DEBUG_LOG_WAYLAND_EMBED(vformat("Client #%d <- %s g0x%x", socket, stringify_message_body(ProxyDirection::CLIENT, interface, &message, body, body_len), p_global_id));
		} else {
			DEBUG_LOG_WAYLAND_EMBED(vformat("Client N/A <- %s g0x%x", stringify_message_body(ProxyDirection::CLIENT, interface, &message, body, body_len), p_global_id));
		}
	}

	// FIXME: Make sure that it makes sense to track this protocol. Not only is it
	// old and getting deprecated, but I can't even get this code branch to hit
	// probably because, at the time of writing, we only get the "main" display
	// through the proxy.
	if (global_object->interface == &wl_drm_interface) {
		// wl_drm can't ever be destroyed, so we need to track its state as it's going
		// to be instanced at least few times.
		uint32_t global_name = registry_globals_names[p_global_id];
		WaylandDrmGlobalData *global_data = (WaylandDrmGlobalData *)registry_globals[global_name].data;
		ERR_FAIL_NULL_V(global_data, MessageStatus::ERROR);

		if (p_opcode == WL_DRM_DEVICE) {
			// signature: s
			uint32_t name_len = body[0];
			uint8_t *name = (uint8_t *)(body + 1);
			global_data->device = String::utf8((const char *)name, name_len);

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == WL_DRM_FORMAT) {
			// signature: u
			uint32_t format = body[0];
			global_data->formats.push_back(format);

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == WL_DRM_AUTHENTICATED) {
			// signature: N/A
			global_data->authenticated = true;

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == WL_DRM_CAPABILITIES) {
			// signature: u
			uint32_t capabilities = body[0];
			global_data->capabilities = capabilities;
		}

		return MessageStatus::UNHANDLED;
	}

	if (global_object->interface == &wl_shm_interface) {
		uint32_t global_name = registry_globals_names[p_global_id];
		WaylandShmGlobalData *global_data = (WaylandShmGlobalData *)registry_globals[global_name].data;
		ERR_FAIL_NULL_V(global_data, MessageStatus::ERROR);

		if (p_opcode == WL_SHM_FORMAT) {
			// Signature: u
			uint32_t format = body[0];
			global_data->formats.push_back(format);
		}
	}

	if (!p_local_handle.is_valid()) {
		// Some requests might not have a valid local object handle for various
		// reasons, such as when certain events are directed to this proxy or when the
		// destination client of a message disconnected in the meantime.

		if (global_object->interface == &wl_display_interface) {
			if (p_opcode == WL_DISPLAY_DELETE_ID) {
				// [Event] wl_display::delete_id(u)
				uint32_t global_delete_id = body[0];
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Compositor requested deletion of g0x%x (no client)", global_delete_id));

				delete_object(global_delete_id);

				return MessageStatus::HANDLED;
			} else if (p_opcode == WL_DISPLAY_ERROR) {
				// [Event] wl_display::error(ous)
				uint32_t obj_id = body[0];
				uint32_t err_code = body[1];

				CRASH_NOW_MSG(vformat("Error obj g0x%x code %d: %s", obj_id, err_code, (const char *)(body + 3)));
			}
		}

		if (global_object->interface == &wl_callback_interface && p_opcode == WL_CALLBACK_DONE) {
			if (sync_callback_id != INVALID_ID && p_global_id == sync_callback_id) {
				sync_callback_id = 0;
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Sync response received");
				return MessageStatus::HANDLED;
			}
		}

		if (global_object->interface == &wl_registry_interface) {
			if (p_opcode == WL_REGISTRY_GLOBAL) {
				// [Event] wl_registry::global(usu).

				uint32_t global_name = body[0];
				uint32_t interface_name_len = body[1];
				const char *interface_name = (const char *)(body + 2);
				uint32_t global_version = body[2 + wl_array_word_offset(interface_name_len)];

				DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Global c#%d %s %d", global_name, interface_name, global_version);

				const struct wl_interface *global_interface = wl_interface_from_string(interface_name, interface_name_len);
				if (global_interface) {
					RegistryGlobalInfo global_info = {};
					global_info.interface = global_interface;
					global_info.version = MIN(global_version, (uint32_t)global_interface->version);
					DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Clamped global %s to version %d.", interface_name, global_info.version);
					global_info.compositor_name = global_name;

					int new_global_name = registry_globals_counter++;

					if (global_info.interface == &wl_shm_interface) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Allocating global wl_shm data.");
						global_info.data = memnew(WaylandShmGlobalData);
					}

					if (global_info.interface == &wl_seat_interface) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Allocating global wl_seat data.");
						global_info.data = memnew(WaylandSeatGlobalData);
						wl_seat_names.push_back(new_global_name);
					}

					if (global_info.interface == &wl_drm_interface) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Allocating global wl_drm data.");
						global_info.data = memnew(WaylandDrmGlobalData);
					}

					registry_globals[new_global_name] = global_info;

					// We need some interfaces directly. It's better to bind a "copy" ourselves
					// than to wait for the client to ask one.
					if (global_interface == &xdg_wm_base_interface && xdg_wm_base_id == 0) {
						xdg_wm_base_id = wl_registry_bind(p_global_id, new_global_name, global_info.version);
						ERR_FAIL_COND_V(xdg_wm_base_id == INVALID_ID, MessageStatus::ERROR);
					} else if (global_interface == &wl_compositor_interface && wl_compositor_id == 0) {
						wl_compositor_id = wl_registry_bind(p_global_id, new_global_name, global_info.version);
						ERR_FAIL_COND_V(wl_compositor_id == INVALID_ID, MessageStatus::ERROR);
					} else if (global_interface == &wl_subcompositor_interface && wl_subcompositor_id == 0) {
						wl_subcompositor_id = wl_registry_bind(p_global_id, new_global_name, global_info.version);
						ERR_FAIL_COND_V(wl_subcompositor_id == INVALID_ID, MessageStatus::ERROR);
					}

					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Local registry object name: l#%d", new_global_name));

					if (clients.is_empty()) {
						// Let's not waste time.
						return MessageStatus::HANDLED;
					}

					// Notify all clients.
					LocalVector<wl_argument> args;
					args.push_back(wl_arg_uint(new_global_name));
					args.push_back(wl_arg_string(interface_name));
					args.push_back(wl_arg_uint(global_info.version));
					for (KeyValue<int, Client> &pair : clients) {
						Client &client = pair.value;
						for (uint32_t local_registry_id : client.wl_registry_instances) {
							client.send_wl_registry_global(local_registry_id, new_global_name, interface_name, global_info.version);
						}
					}

					return MessageStatus::HANDLED;
				} else {
					DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Skipping unknown global %s version %d.", interface_name, global_version);

					return MessageStatus::HANDLED;
				}
			} else if (p_opcode == WL_REGISTRY_GLOBAL_REMOVE) {
				uint32_t compositor_name = body[0];
				uint32_t local_name = 0;
				RegistryGlobalInfo *global_info = nullptr;

				// FIXME: Use a map or something.
				for (KeyValue<uint32_t, RegistryGlobalInfo> &pair : registry_globals) {
					uint32_t name = pair.key;
					RegistryGlobalInfo &info = pair.value;

					if (info.compositor_name == compositor_name) {
						local_name = name;
						global_info = &info;
						break;
					}
				}

				ERR_FAIL_NULL_V(global_info, MessageStatus::ERROR);

				if (global_info->instance_counter == 0) {
					memdelete(global_info->data);
					registry_globals.erase(local_name);
				} else {
					global_info->destroyed = true;
				}

				// Notify all clients.
				LocalVector<wl_argument> args;
				args.push_back(wl_arg_uint(local_name));
				for (KeyValue<int, Client> &pair : clients) {
					Client &client = pair.value;
					for (uint32_t local_registry_id : client.wl_registry_instances) {
						client.send_wl_registry_global_remove(local_registry_id, local_name);
					}
				}

				return MessageStatus::HANDLED;
			}
		}

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("No valid local object handle, falling back to generic handler.");
		return MessageStatus::UNHANDLED;
	}

	Client *client = p_local_handle.get_client();

	ERR_FAIL_NULL_V(client, MessageStatus::ERROR);

	WaylandObject *object = p_local_handle.get();
	uint32_t local_id = p_local_handle.get_local_id();

	if (global_object->interface == &wl_display_interface) {
		if (p_opcode == WL_DISPLAY_DELETE_ID) {
			// [Event] wl_display::delete_id(u)
			uint32_t global_delete_id = body[0];
			uint32_t local_delete_id = client->get_local_id(global_delete_id);
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Compositor requested delete of g0x%x l0x%x", global_delete_id, local_delete_id));
			if (local_delete_id == INVALID_ID) {
				// No idea what this object is, might be of the other client. This
				// definitely does not make sense to us, so we're done.
				return MessageStatus::INVALID;
			}

			client->delete_object(local_delete_id);

			// DOC: Why not in delete_object?
			client->send_wl_display_delete_id(DISPLAY_ID, local_delete_id);

			return MessageStatus::HANDLED;
		}

		return MessageStatus::UNHANDLED;
	}

	if (object->interface == &wl_keyboard_interface) {
		WaylandKeyboardData *data = (WaylandKeyboardData *)object->data;
		ERR_FAIL_NULL_V(data, MessageStatus::ERROR);

		uint32_t global_seat_name = registry_globals_names[data->wl_seat_id];
		RegistryGlobalInfo &global_seat_info = registry_globals[global_seat_name];
		WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)global_seat_info.data;
		ERR_FAIL_NULL_V(global_seat_data, MessageStatus::ERROR);

		if (p_opcode == WL_KEYBOARD_ENTER) {
			// [Event] wl_keyboard::enter(uoa)
			uint32_t surface = body[1];

			if (global_seat_data->focused_surface_id != surface) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Focused g0x%x", surface));
				global_seat_data->focused_surface_id = surface;
			}
		} else if (p_opcode == WL_KEYBOARD_LEAVE) {
			// [Event] wl_keyboard::leave(uo)
			uint32_t surface = body[1];

			if (global_seat_data->focused_surface_id == surface) {
				global_seat_data->focused_surface_id = INVALID_ID;
			}

			global_seat_data->mods_depressed = 0;
			global_seat_data->mods_latched = 0;
			global_seat_data->mods_locked = 0;
			global_seat_data->group = 0;
		} else if (p_opcode == WL_KEYBOARD_KEY) {
			// NOTE: modifiers event can be sent even without focus, according to the
			// spec, so there's no need to skip it.
			if (global_seat_data->focused_surface_id != INVALID_ID && !client->local_ids.has(global_seat_data->focused_surface_id)) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Skipped wl_keyboard event due to unfocused surface 0x%x", global_seat_data->focused_surface_id));
				return MessageStatus::HANDLED;
			}
		} else if (p_opcode == WL_KEYBOARD_MODIFIERS) {
			// uint32_t serial = body[0];
			global_seat_data->mods_depressed = body[1];
			global_seat_data->mods_latched = body[2];
			global_seat_data->mods_locked = body[3];
			global_seat_data->group = body[4];
		}

		return MessageStatus::UNHANDLED;
	}

	if (object->interface == &wl_pointer_interface) {
		WaylandPointerData *data = (WaylandPointerData *)object->data;
		ERR_FAIL_NULL_V(data, MessageStatus::ERROR);

		uint32_t global_seat_name = registry_globals_names[data->wl_seat_id];
		RegistryGlobalInfo &global_seat_info = registry_globals[global_seat_name];
		WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)global_seat_info.data;
		ERR_FAIL_NULL_V(global_seat_data, MessageStatus::ERROR);

		WaylandSeatInstanceData *seat_data = (WaylandSeatInstanceData *)object->data;
		ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

		if (p_opcode == WL_POINTER_BUTTON && global_seat_data->pointed_surface_id != INVALID_ID) {
			// [Event] wl_pointer::button(uuuu);
			uint32_t button = body[2];
			uint32_t state = body[3];

			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Button %d state %d on surface g0x%x (focused g0x%x)", button, state, global_seat_data->pointed_surface_id, global_seat_data->focused_surface_id));

			bool client_pointed = client->local_ids.has(global_seat_data->pointed_surface_id);

			if (button != BTN_LEFT || state != WL_POINTER_BUTTON_STATE_RELEASED) {
				return client_pointed ? MessageStatus::UNHANDLED : MessageStatus::HANDLED;
			}

			if (global_seat_data->focused_surface_id == global_seat_data->pointed_surface_id) {
				return client_pointed ? MessageStatus::UNHANDLED : MessageStatus::HANDLED;
			}

			if (!global_surface_is_window(global_seat_data->pointed_surface_id)) {
				return client_pointed ? MessageStatus::UNHANDLED : MessageStatus::HANDLED;
			}

			if (global_seat_data->focused_surface_id != INVALID_ID) {
				seat_name_leave_surface(global_seat_name, global_seat_data->focused_surface_id);
			}

			global_seat_data->focused_surface_id = global_seat_data->pointed_surface_id;
			seat_name_enter_surface(global_seat_name, global_seat_data->focused_surface_id);
		} else if (p_opcode == WL_POINTER_ENTER) {
			// [Event] wl_pointer::enter(uoff).
			uint32_t surface = body[1];
			WaylandSurfaceData *surface_data = (WaylandSurfaceData *)get_object(surface)->data;
			ERR_FAIL_NULL_V(surface_data, MessageStatus::ERROR);

			if (global_seat_data->pointed_surface_id != surface) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Pointer (g0x%x seat g0x%x): pointed surface old g0x%x new g0x%x", p_global_id, data->wl_seat_id, global_seat_data->pointed_surface_id, surface));

				global_seat_data->pointed_surface_id = surface;
			}
		} else if (p_opcode == WL_POINTER_LEAVE) {
			// [Event] wl_pointer::leave(uo).
			uint32_t surface = body[1];
			WaylandSurfaceData *surface_data = (WaylandSurfaceData *)get_object(surface)->data;
			ERR_FAIL_NULL_V(surface_data, MessageStatus::ERROR);

			if (global_seat_data->pointed_surface_id == surface) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Pointer (g0x%x seat g0x%x): g0x%x -> g0x%x", p_global_id, data->wl_seat_id, global_seat_data->pointed_surface_id, INVALID_ID));
				global_seat_data->pointed_surface_id = INVALID_ID;
			}
		}

		return MessageStatus::UNHANDLED;
	}

	if (object->interface == &wl_touch_interface) {
		WaylandTouchData *data = (WaylandTouchData *)object->data;
		ERR_FAIL_NULL_V(data, MessageStatus::ERROR);

		uint32_t global_seat_name = registry_globals_names[data->wl_seat_id];
		RegistryGlobalInfo &global_seat_info = registry_globals[global_seat_name];
		WaylandSeatGlobalData *global_seat_data = (WaylandSeatGlobalData *)global_seat_info.data;
		ERR_FAIL_NULL_V(global_seat_data, MessageStatus::ERROR);

		WaylandSeatInstanceData *seat_data = (WaylandSeatInstanceData *)object->data;
		ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

		if (p_opcode == WL_TOUCH_DOWN) {
			// uint32_t serial = body[0];
			// uint32_t time = body[1];
			uint32_t surface = body[2];
			// uint32_t id = body[3];
			// uint32_t x = body[4];
			// uint32_t y = body[5];

			++data->touch_point_count;

			if (data->touch_point_count == 1) {
				if (global_seat_data->focused_surface_id == surface) {
					return MessageStatus::UNHANDLED;
				}

				if (!global_surface_is_window(surface)) {
					return MessageStatus::UNHANDLED;
				}

				if (global_seat_data->focused_surface_id != INVALID_ID) {
					seat_name_leave_surface(global_seat_name, global_seat_data->focused_surface_id);
				}

				global_seat_data->focused_surface_id = surface;
				seat_name_enter_surface(global_seat_name, surface);
			}

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == WL_TOUCH_UP) {
			--data->touch_point_count;

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == WL_TOUCH_CANCEL) {
			data->touch_point_count = 0;

			return MessageStatus::UNHANDLED;
		}

		return MessageStatus::UNHANDLED;
	}

	if (object->interface == &xdg_popup_interface) {
		if (p_opcode == XDG_POPUP_CONFIGURE) {
			// [Event] xdg_popup::configure(iiii);
			int32_t x = body[0];
			int32_t y = body[1];
			int32_t width = body[2];
			int32_t height = body[3];

			XdgPopupData *data = (XdgPopupData *)object->data;
			ERR_FAIL_NULL_V(data, MessageStatus::ERROR);

			XdgSurfaceData *parent_xdg_surf_data = (XdgSurfaceData *)data->parent_handle.get()->data;
			ERR_FAIL_NULL_V(parent_xdg_surf_data, MessageStatus::ERROR);

			WaylandSurfaceData *parent_surface_data = (WaylandSurfaceData *)get_object(parent_xdg_surf_data->wl_surface_id)->data;
			ERR_FAIL_NULL_V(parent_surface_data, MessageStatus::ERROR);

			WaylandObject *parent_role_obj = parent_surface_data->role_object_handle.get();
			ERR_FAIL_NULL_V(parent_role_obj, MessageStatus::ERROR);

			if (parent_role_obj->interface == &xdg_toplevel_interface) {
				XdgToplevelData *parent_toplevel_data = (XdgToplevelData *)parent_role_obj->data;
				ERR_FAIL_NULL_V(parent_toplevel_data, MessageStatus::ERROR);

				if (parent_toplevel_data->is_embedded()) {
					WaylandSubsurfaceData *subsurf_data = (WaylandSubsurfaceData *)get_object(parent_toplevel_data->wl_subsurface_id)->data;
					ERR_FAIL_NULL_V(subsurf_data, MessageStatus::ERROR);

					// The coordinates passed will be shifted by the embedded window position,
					// so we need to fix them back.
					Point2i fixed_position = Point2i(x, y) - subsurf_data->position;

					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Correcting popup configure position to %s", fixed_position));

					client->send_xdg_popup_configure(local_id, fixed_position.x, fixed_position.y, width, height);

					return MessageStatus::HANDLED;
				}
			}
		}
	}

	if (object->interface == &zwp_tablet_seat_v2_interface) {
		if (p_opcode == ZWP_TABLET_SEAT_V2_TABLET_ADDED) {
			uint32_t new_id = body[0];

			TabletSeatData *seat_data = (TabletSeatData *)object->data;
			ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

			seat_data->tablets.insert(new_id);

			TabletData *tablet_data = memnew(TabletData);
			ERR_FAIL_NULL_V(tablet_data, MessageStatus::ERROR);

			tablet_data->seat_id = p_global_id;

			uint32_t new_local_id = client->new_server_object(new_id, &zwp_tablet_v2_interface, object->version, tablet_data);

			client->send_zwp_tablet_seat_v2_tablet_added(local_id, new_local_id);

			return MessageStatus::HANDLED;
		}

		if (p_opcode == ZWP_TABLET_SEAT_V2_TOOL_ADDED) {
			uint32_t new_id = body[0];

			TabletSeatData *seat_data = (TabletSeatData *)object->data;
			ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

			TabletToolData *tool_data = memnew(TabletToolData);
			ERR_FAIL_NULL_V(tool_data, MessageStatus::ERROR);

			tool_data->seat_id = p_global_id;

			uint32_t new_local_id = client->new_server_object(new_id, &zwp_tablet_tool_v2_interface, object->version, tool_data);

			client->send_zwp_tablet_seat_v2_tool_added(local_id, new_local_id);

			return MessageStatus::HANDLED;
		}
	}

	if (object->interface == &zwp_tablet_v2_interface) {
		if (p_opcode == ZWP_TABLET_V2_REMOVED) {
			TabletData *tablet_data = (TabletData *)object->data;
			ERR_FAIL_NULL_V(tablet_data, MessageStatus::ERROR);

			WaylandObject *seat_object = get_object(tablet_data->seat_id);
			ERR_FAIL_NULL_V(seat_object, MessageStatus::ERROR);

			TabletSeatData *seat_data = (TabletSeatData *)seat_object->data;
			ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

			seat_data->tablets.erase(p_global_id);

			return MessageStatus::UNHANDLED;
		}
	}

	if (object->interface == &zwp_tablet_tool_v2_interface) {
		TabletToolData *tool_data = (TabletToolData *)object->data;
		ERR_FAIL_NULL_V(tool_data, MessageStatus::ERROR);

		WaylandObject *seat_object = get_object(tool_data->seat_id);
		ERR_FAIL_NULL_V(seat_object, MessageStatus::ERROR);

		TabletSeatData *seat_data = (TabletSeatData *)seat_object->data;
		ERR_FAIL_NULL_V(seat_data, MessageStatus::ERROR);

		if (p_opcode == ZWP_TABLET_TOOL_V2_PROXIMITY_IN) {
			uint32_t serial = body[0];
			uint32_t tablet = body[1];
			uint32_t surface = body[2];

			uint32_t local_tablet_id = client->get_local_id(tablet);

			if (local_tablet_id == INVALID_ID && seat_data->tablets.begin()) {
				// Client has no idea what this tablet is, likely because the compositor is
				// reporting an object from another tablet seat (a common implementation
				// mistake[1]). We can somewhat workaround by reporting a random tablet
				// instead. This isn't ideal as it might not represent the same thing, but
				// AFAICS there's no reliable way to 100% identify a tablet from its
				// description, and Godot does not do anything fancy in the first place.
				// [1]: https://oftc.catirclogs.org/wayland/2026-07-02#1782988779-1782991085
				tablet = *seat_data->tablets.begin();
			}

			local_tablet_id = client->get_local_id(tablet);
			uint32_t local_surface_id = client->get_local_id(surface);
			if (local_tablet_id == INVALID_ID || local_surface_id == INVALID_ID) {
				// We usually error, but in this case this can actually happen, if the
				// surface or tablet isn't known by the client (e.g. embedded).
				return MessageStatus::HANDLED;
			}

			seat_data->proximal_surface = LocalObjectHandle(client, local_surface_id);

			client->send_zwp_tablet_tool_v2_proximity_in(local_id, serial, local_tablet_id, local_surface_id);

			return MessageStatus::HANDLED;
		}

		if (p_opcode == ZWP_TABLET_TOOL_V2_PROXIMITY_OUT) {
			seat_data->proximal_surface.invalidate();

			return MessageStatus::UNHANDLED;
		}

		if (p_opcode == ZWP_TABLET_TOOL_V2_DOWN) {
			// FIXME: Gracefully handle deleted seats (here and in other seat logic).
			RegistryGlobalInfo &global_seat_info = registry_globals[seat_data->wl_seat_name];
			WaylandSeatGlobalData *global_wl_seat_data = (WaylandSeatGlobalData *)global_seat_info.data;
			ERR_FAIL_NULL_V(global_wl_seat_data, MessageStatus::ERROR);

			if (seat_data->proximal_surface.get_global_id() != INVALID_ID) {
				seat_name_leave_surface(seat_data->wl_seat_name, global_wl_seat_data->focused_surface_id);
				seat_name_enter_surface(seat_data->wl_seat_name, seat_data->proximal_surface.get_global_id());

				global_wl_seat_data->focused_surface_id = seat_data->proximal_surface.get_global_id();
			}

			return MessageStatus::UNHANDLED;
		}
	}

	return MessageStatus::UNHANDLED;
}

void WaylandEmbedder::shutdown() {
	thread_done.set();

	{
		// First making a list of all clients so that we can iteratively delete them.
		LocalVector<int> sockets;
		for (KeyValue<int, Client> &pair : clients) {
			sockets.push_back(pair.key);
		}

		for (int socket : sockets) {
			cleanup_socket(socket);
		}
	}

	close(compositor_socket);
	compositor_socket = -1;

	for (KeyValue<uint32_t, RegistryGlobalInfo> &pair : registry_globals) {
		RegistryGlobalInfo &info = pair.value;
		if (info.data) {
			memdelete(info.data);
			info.data = nullptr;
		}
	}
}

Error WaylandEmbedder::handle_msg_info(Client *client, const struct msg_info *info, uint32_t *buf, LocalVector<int> &r_sent_fds) {
	ERR_FAIL_NULL_V(info, ERR_BUG);
	ERR_FAIL_NULL_V_MSG(info->direction == ProxyDirection::COMPOSITOR && client, ERR_BUG, "Wait, where did this message come from?");

	WaylandObject *object = nullptr;

	uint32_t global_id = INVALID_ID;
	if (info->direction == ProxyDirection::CLIENT) {
		global_id = info->raw_id;
	} else if (info->direction == ProxyDirection::COMPOSITOR) {
		global_id = client->get_global_id(info->raw_id);
	}

	if (global_id != INVALID_ID) {
		object = get_object(global_id);
	} else if (client) {
		object = client->get_object(info->raw_id);
	}

	if (object == nullptr) {
		if (info->direction == ProxyDirection::COMPOSITOR) {
			uint32_t local_id = info->raw_id;
			ERR_PRINT(vformat("Couldn't find requested object l0x%x for client %d, disconnecting.", local_id, client->socket));

			socket_error(client->socket, local_id, WL_DISPLAY_ERROR_INVALID_OBJECT, vformat("Object l0x%x not found.", local_id));
			return OK;
		} else {
			CRASH_NOW_MSG(vformat("No object found for r0x%x", info->raw_id));
		}
	}

	const struct wl_interface *interface = nullptr;
	interface = object->interface;

	if (interface == nullptr && info->raw_id & 0xff000000) {
		// Regular clients have no confirmation about deleted server objects (why
		// should they?) but since we share connections there's the risk of receiving
		// messages about deleted server objects. The simplest solution is to ignore
		// unknown server-side objects. Not the safest thing, I know, but it should do
		// the job.
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Ignoring unknown server-side object r0x%x", info->raw_id));
		return OK;
	}

	ERR_FAIL_NULL_V_MSG(interface, ERR_BUG, vformat("Object r0x%x has no interface", info->raw_id));

	const struct wl_message *message = nullptr;
	if (info->direction == ProxyDirection::CLIENT) {
		ERR_FAIL_COND_V(info->opcode >= interface->event_count, ERR_BUG);
		message = &interface->events[info->opcode];
	} else {
		ERR_FAIL_COND_V(info->opcode >= interface->method_count, ERR_BUG);
		message = &interface->methods[info->opcode];
	}
	ERR_FAIL_NULL_V(message, ERR_BUG);

	int fds_requested = String(message->signature).count("h");
	if (fds_requested > 0) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Requested %d FDs.", fds_requested));

		List<int> &fd_queue = info->direction == ProxyDirection::COMPOSITOR ? client->fds : compositor_fds;
		for (int i = 0; i < fds_requested; ++i) {
			ERR_FAIL_COND_V_MSG(fd_queue.is_empty(), ERR_BUG, "Out of FDs.");
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Fetching FD %d.", fd_queue.front()->get()));
			r_sent_fds.push_back(fd_queue.front()->get());
			fd_queue.pop_front();
		}

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Remaining FDs: %d.", fd_queue.size()));
	}

	if (object->destroyed) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Ignoring message for inert object.");

		// Inert object.
		return OK;
	}

	if (info->direction == ProxyDirection::COMPOSITOR) {
		MessageStatus request_status = handle_request(LocalObjectHandle(client, info->raw_id), info->opcode, buf, info->size);
		if (request_status == MessageStatus::ERROR) {
			return ERR_BUG;
		}

		if (request_status == MessageStatus::HANDLED) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Custom handler success.");
			return OK;
		}

		if (global_id != INVALID_ID) {
			buf[0] = global_id;
		}

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Falling back to generic handler.");

		if (handle_generic_msg(client, object, message, info, buf)) {
			send_raw_message(compositor_socket, { { buf, info->size } }, r_sent_fds);
		}
	} else {
		uint32_t global_name = 0;

		bool is_global = false;
		if (registry_globals_names.has(global_id)) {
			global_name = registry_globals_names[global_id];
			is_global = true;
		}

		// FIXME: For compatibility, mirror events with instanced registry globals as
		// object arguments. For example, `wl_surface.enter` returns a `wl_output`. If
		// said `wl_output` has been instanced multiple times, we need to resend the
		// same event with each instance as the argument, or the client might miss the
		// event by looking for the "wrong" instance.
		//
		// Note that this missing behavior is exclusively a compatibility mechanism
		// for old compositors which only implement undestroyable globals. We
		// otherwise passthrough every bind request and then the compositor takes care
		// of everything.
		// See: https://lore.freedesktop.org/wayland-devel/20190326121421.06732fd2@eldfell.localdomain/
		if (object->shared) {
			bool handled = false;

			for (KeyValue<int, Client> &pair : clients) {
				Client &c = pair.value;
				if (c.socket < 0) {
					continue;
				}

				if (!c.local_ids.has(global_id)) {
					DEBUG_LOG_WAYLAND_EMBED_VERBOSE("!!!!!!!!!!! Instance missing?");
					continue;
				}

				if (is_global) {
					if (!c.registry_globals_instances.has(global_name)) {
						continue;
					}

					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Broadcasting to all global instances for client %d (socket %d)", c.pid, c.socket));
					for (uint32_t instance_id : c.registry_globals_instances[global_name]) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Global instance l0x%x", instance_id));

						LocalObjectHandle local_obj = LocalObjectHandle(&c, instance_id);
						if (!local_obj.is_valid()) {
							continue;
						}

						MessageStatus event_status = handle_event(global_id, local_obj, info->opcode, buf, info->size);

						if (event_status == MessageStatus::ERROR) {
							return ERR_BUG;
						}

						if (event_status == MessageStatus::HANDLED) {
							DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Custom handler success.");
							handled = true;
							continue;
						}

						if (event_status == MessageStatus::INVALID) {
							continue;
						}

						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Falling back to generic handler.");

						buf[0] = instance_id;

						if (handle_generic_msg(&c, local_obj.get(), message, info, buf, instance_id)) {
							send_raw_message(c.socket, { { buf, info->size } }, r_sent_fds);
						}

						handled = true;
					}
				} else if (interface == &wl_display_interface) {
					// NOTE: The only shared non-global objects are `wl_display` and
					// `wl_registry`, both of which require custom handlers. Additionally, of
					// those only `wl_display` has client-specific handlers, which is what this
					// branch manages.

					LocalObjectHandle local_obj = LocalObjectHandle(&c, c.get_local_id(global_id));
					if (!local_obj.is_valid()) {
						continue;
					}

					DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Shared non-global l0x%x g0x%x", c.get_local_id(global_id), global_id));

					MessageStatus event_status = handle_event(global_id, local_obj, info->opcode, buf, info->size);
					if (event_status == MessageStatus::ERROR) {
						return ERR_BUG;
					}

					if (event_status == MessageStatus::HANDLED) {
						DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Custom handler success.");
						handled = true;
						continue;
					}

					if (event_status == MessageStatus::INVALID) {
						continue;
					}

					DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Falling back to generic handler.");

					if (handle_generic_msg(&c, local_obj.get(), message, info, buf)) {
						send_raw_message(c.socket, { { buf, info->size } }, r_sent_fds);
					}

					handled = true;
				}
			}

			if (!handled) {
				// No client handled this, it's going to be handled as a client-less event.
				// We do this only at the end to avoid handling certain events (e.g.
				// deletion) twice.
				handle_event(global_id, LocalObjectHandle(nullptr, INVALID_ID), info->opcode, buf, info->size);
			}
		} else {
			LocalObjectHandle local_obj = LocalObjectHandle(client, client ? client->get_local_id(global_id) : INVALID_ID);

			MessageStatus event_status = handle_event(global_id, local_obj, info->opcode, buf, info->size);
			if (event_status == MessageStatus::ERROR) {
				return ERR_BUG;
			}

			if (event_status == MessageStatus::HANDLED || event_status == MessageStatus::INVALID) {
				// We're done.
				return OK;
			}

			// Generic passthrough.

			if (client) {
				uint32_t local_id = client->get_local_id(global_id);
				ERR_FAIL_COND_V(local_id == INVALID_ID, OK);

				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("%s::%s(%s) g0x%x -> l0x%x", interface->name, message->name, message->signature, global_id, local_id));
				buf[0] = local_id;

				if (handle_generic_msg(client, local_obj.get(), message, info, buf)) {
					send_raw_message(client->socket, { { buf, info->size } }, r_sent_fds);
				}
			} else {
				WARN_PRINT_ONCE(vformat("[Wayland Embedder] Unexpected client-less event from %s#g0x%x. Object has probably leaked.", object->interface->name, global_id));
				handle_generic_msg(nullptr, object, message, info, buf);
			}
		}
	}

	return OK;
}

Error WaylandEmbedder::handle_sock(int p_fd) {
	ERR_FAIL_COND_V(p_fd < 0, ERR_INVALID_PARAMETER);

	struct msg_info info = {};

	{
		struct msghdr head_msg = {};
		uint32_t header[2];
		struct iovec vec = { header, sizeof header };

		head_msg.msg_iov = &vec;
		head_msg.msg_iovlen = 1;

		ssize_t head_rec = recvmsg(p_fd, &head_msg, MSG_PEEK);

		if (head_rec == 0) {
			// Client disconnected.
			return ERR_CONNECTION_ERROR;
		}

		if (head_rec == -1) {
			if (errno == ECONNRESET) {
				// No need to print the error, the client forcefully disconnected, that's
				// fine.
				return ERR_CONNECTION_ERROR;
			}

			ERR_FAIL_V_MSG(FAILED, vformat("Can't read message header: %s", strerror(errno)));
		}

		ERR_FAIL_COND_V_MSG(((size_t)head_rec) != vec.iov_len, ERR_CONNECTION_ERROR, vformat("Should've received %d bytes, instead got %d bytes", vec.iov_len, head_rec));

		// Header is two 32-bit words: first is ID, second has size in most significant
		// half and opcode in the other half.
		info.raw_id = header[0];
		info.size = header[1] >> 16;
		info.opcode = header[1] & 0xFFFF;
		info.direction = p_fd != compositor_socket ? ProxyDirection::COMPOSITOR : ProxyDirection::CLIENT;
	}

	if (msg_buf.size() < info.words()) {
		msg_buf.resize(info.words());
	}

	ERR_FAIL_COND_V_MSG(info.size % WL_WORD_SIZE != 0, ERR_CONNECTION_ERROR, "Invalid message length.");

	struct msghdr full_msg = {};
	struct iovec vec = { msg_buf.ptr(), info.size };
	{
		full_msg.msg_iov = &vec;
		full_msg.msg_iovlen = 1;
		full_msg.msg_control = ancillary_buf.ptr();
		full_msg.msg_controllen = ancillary_buf.size();

		ssize_t full_rec = recvmsg(p_fd, &full_msg, 0);

		if (full_rec == -1) {
			if (errno == ECONNRESET) {
				// No need to print the error, the client forcefully disconnected, that's
				// fine.
				return ERR_CONNECTION_ERROR;
			}

			ERR_FAIL_V_MSG(FAILED, vformat("Can't read message: %s", strerror(errno)));
		}

		ERR_FAIL_COND_V_MSG(((size_t)full_rec) != info.size, ERR_CONNECTION_ERROR, "Invalid message length.");

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(" === START PACKET === ");

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
		printf("[PROXY] Received bytes: ");
		for (ssize_t i = 0; i < full_rec; ++i) {
			printf("%.2x", ((const uint8_t *)msg_buf.ptr())[i]);
		}
		printf("\n");
#endif
	}

	if (full_msg.msg_controllen > 0) {
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&full_msg);
		while (cmsg) {
			// TODO: Check for validity of message fields.
			size_t data_len = cmsg->cmsg_len - sizeof *cmsg;

			if (cmsg->cmsg_type == SCM_RIGHTS) {
				// NOTE: Linux docs say that we can't just cast data to pointer type because
				// of alignment concerns. So we have to memcpy into a new buffer.
				int *cmsg_fds = (int *)malloc(data_len);
				memcpy(cmsg_fds, CMSG_DATA(cmsg), data_len);

				size_t cmsg_fds_count = data_len / sizeof *cmsg_fds;
				for (size_t i = 0; i < cmsg_fds_count; ++i) {
					int fd = cmsg_fds[i];

					if (info.direction == ProxyDirection::COMPOSITOR) {
						clients[p_fd].fds.push_back(fd);
					} else {
						compositor_fds.push_back(fd);
					}
				}

#ifdef WAYLAND_EMBED_VERBOSE_LOGS_ENABLED
				printf("[PROXY] Received %ld file descriptors: ", cmsg_fds_count);
				for (size_t i = 0; i < cmsg_fds_count; ++i) {
					printf("%d ", cmsg_fds[i]);
				}
				printf("\n");
#endif

				free(cmsg_fds);
			}

			cmsg = CMSG_NXTHDR(&full_msg, cmsg);
		}
	}
	full_msg.msg_control = nullptr;
	full_msg.msg_controllen = 0;

	Client *client = nullptr;
	if (p_fd == compositor_socket) {
		// Let's figure out the recipient of the message.
		for (KeyValue<int, Client> &pair : clients) {
			Client &c = pair.value;

			if (c.local_ids.has(info.raw_id)) {
				client = &c;
			}
		}
	} else {
		CRASH_COND(!clients.has(p_fd));
		client = &clients[p_fd];
	}

	LocalVector<int> sent_fds;
	Error err = handle_msg_info(client, &info, msg_buf.ptr(), sent_fds);

	for (int fd : sent_fds) {
		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Closing fd %d.", fd));
		close(fd);
	}

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE(" === END PACKET === ");

	if (err != OK) {
		return ERR_BUG;
	}

	return OK;
}

void WaylandEmbedder::_thread_loop(void *p_data) {
	Thread::set_name("Wayland Embed");

	ERR_FAIL_NULL(p_data);
	WaylandEmbedder *proxy = (WaylandEmbedder *)p_data;

	DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Proxy thread started");

	while (!proxy->thread_done.is_set()) {
		proxy->poll_sockets();
	}
}

// TODO: Detect missing subcompositor.
Error WaylandEmbedder::init(bool debug) {
	debug_print_enabled = debug;

	ancillary_buf.resize(EMBED_ANCILLARY_BUF_SIZE);

	proxy_socket = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un addr = {};
	addr.sun_family = AF_UNIX;

	String runtime_dir_path = OS::get_environment("XDG_RUNTIME_DIR");
	ERR_FAIL_COND_V_MSG(runtime_dir_path.is_empty(), ERR_DOES_NOT_EXIST, "XDG_RUNTIME_DIR is not set or empty.");

	runtime_dir = DirAccess::create_for_path(runtime_dir_path);
	ERR_FAIL_COND_V(!runtime_dir.is_valid(), ERR_BUG);
	ERR_FAIL_COND_V_MSG(!runtime_dir->is_writable(runtime_dir_path), ERR_FILE_CANT_WRITE, "XDG_RUNTIME_DIR points to an invalid directory.");

	int socket_id = 0;
	while (socket_path.is_empty()) {
		String test_socket_path = runtime_dir_path + "/godot-wayland-" + itos(socket_id);
		String test_socket_lock_path = test_socket_path + ".lock";

		print_verbose(vformat("Trying to get socket %s", test_socket_path));
		print_verbose(vformat("Opening lock %s", test_socket_lock_path));
		int test_lock_fd = open(test_socket_lock_path.utf8().get_data(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

		if (flock(test_lock_fd, LOCK_EX | LOCK_NB) == -1) {
			print_verbose(vformat("Can't lock %s", test_socket_lock_path));
			close(test_lock_fd);
			++socket_id;
			continue;
		} else {
			lock_fd = test_lock_fd;
			socket_path = test_socket_path;
			socket_lock_path = test_socket_lock_path;

			break;
		}
	}

	DirAccess::remove_absolute(socket_path);
	strncpy(addr.sun_path, socket_path.utf8().get_data(), sizeof(addr.sun_path) - 1);

	if (bind(proxy_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "Can't bind embedding socket.");
	}

	if (listen(proxy_socket, 1) == -1) {
		ERR_FAIL_V_MSG(ERR_CANT_OPEN, "Can't listen embedding socket.");
	}

	struct wl_display *display = wl_display_connect(nullptr);
	ERR_FAIL_NULL_V(display, ERR_CANT_OPEN);
	compositor_socket = wl_display_get_fd(display);

	pollfds.push_back({ proxy_socket, POLLIN, 0 });
	pollfds.push_back({ compositor_socket, POLLIN, 0 });

	RegistryGlobalInfo control_global_info = {};
	control_global_info.interface = &godot_embedding_compositor_interface;
	control_global_info.version = godot_embedding_compositor_interface.version;

	godot_embedding_compositor_name = registry_globals_counter++;
	registry_globals[godot_embedding_compositor_name] = control_global_info;

	{
		uint32_t invalid_id = INVALID_ID;
		objects.request(invalid_id);

		CRASH_COND(invalid_id != INVALID_ID);
	}

	{
		uint32_t display_id = new_object(&wl_display_interface);
		CRASH_COND(display_id != DISPLAY_ID);

		get_object(DISPLAY_ID)->shared = true;
	}

	{
		uint32_t registry_id = new_object(&wl_registry_interface);
		CRASH_COND(registry_id != REGISTRY_ID);

		get_object(REGISTRY_ID)->shared = true;
	}

	send_wayland_method(compositor_socket, DISPLAY_ID, wl_display_interface, WL_DISPLAY_GET_REGISTRY, { wl_arg_new_id(REGISTRY_ID) });

	sync();

	proxy_thread.start(_thread_loop, this);

	return OK;
}

void WaylandEmbedder::handle_fd(int p_fd, int p_revents) {
	if (p_fd == proxy_socket && p_revents & POLLIN) {
		// Client init.
		int new_fd = accept(proxy_socket, nullptr, nullptr);
		ERR_FAIL_COND_MSG(new_fd == -1, "Failed to accept client.");

		struct ucred cred = {};
		socklen_t cred_size = sizeof cred;
		getsockopt(new_fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_size);

		Client &client = clients.insert_new(new_fd, {})->value;

		client.embedder = this;
		client.socket = new_fd;
		client.pid = cred.pid;

		client.global_ids[DISPLAY_ID] = Client::GlobalIdInfo(DISPLAY_ID, nullptr);
		client.local_ids[DISPLAY_ID] = DISPLAY_ID;

		pollfds.push_back({ new_fd, POLLIN, 0 });

		if (main_client == nullptr) {
			main_client = &client;
		}

		if (new_fd != main_client->socket && main_client->registry_globals_instances.has(godot_embedding_compositor_name)) {
			uint32_t new_local_id = main_client->allocate_server_id();

			client.embedded_client_id = new_local_id;

			for (uint32_t local_id : main_client->registry_globals_instances[godot_embedding_compositor_name]) {
				EmbeddedClientData *eclient_data = memnew(EmbeddedClientData);
				eclient_data->client = &client;

				main_client->new_fake_object(new_local_id, &godot_embedded_client_interface, 1, eclient_data);

				main_client->send_godot_embedding_compositor_client(local_id, new_local_id, cred.pid);
			}
		}

		DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("New client %d (pid %d) initialized.", client.socket, cred.pid));
		return;
	}

	if (p_fd == compositor_socket && p_revents & POLLIN) {
		Error err = handle_sock(p_fd);

		if (err == ERR_BUG) {
			ERR_PRINT("Unexpected error while handling socket, shutting down.");
			shutdown();
			return;
		}

		return;
	}

	const Client *client = clients.getptr(p_fd);
	if (client) {
		if (main_client && client == main_client && p_revents & (POLLHUP | POLLERR)) {
			DEBUG_LOG_WAYLAND_EMBED_VERBOSE("Main client disconnected, shutting down.");
			shutdown();
			return;
		}

		if (p_revents & POLLIN) {
			Error err = handle_sock(p_fd);
			if (err == ERR_BUG) {
				ERR_PRINT("Unexpected error while handling socket, shutting down.");
				shutdown();
				return;
			}

			if (err != OK) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE("disconnecting");
				cleanup_socket(p_fd);
				return;
			}

			return;
		} else if (p_revents & (POLLHUP | POLLERR | POLLNVAL)) {
			if (p_revents & POLLHUP) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Socket %d hangup.", p_fd));
			}
			if (p_revents & POLLERR) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Socket %d error.", p_fd));
			}
			if (p_revents & POLLNVAL) {
				DEBUG_LOG_WAYLAND_EMBED_VERBOSE(vformat("Socket %d invalid FD.", p_fd));
			}

			cleanup_socket(p_fd);

			return;
		}
	}
}

WaylandEmbedder::~WaylandEmbedder() {
	shutdown();
	if (proxy_thread.is_started()) {
		proxy_thread.wait_to_finish();
	}
}

#endif // TOOLS_ENABLED

#endif // WAYLAND_ENABLED

#ifdef WAYLAND_EMBED_ID_MAX
#undef WAYLAND_EMBED_ID_MAX
#endif
#ifdef DEBUG_LOG_WAYLAND_EMBED
#undef DEBUG_LOG_WAYLAND_EMBED
#endif
#ifdef DEBUG_LOG_WAYLAND_EMBED_VERBOSE
#undef DEBUG_LOG_WAYLAND_EMBED_VERBOSE
#endif
#ifdef WL_WORD_SIZE
#undef WL_WORD_SIZE
#endif
#ifdef WL_DISPLAY_ERROR
#undef WL_DISPLAY_ERROR
#endif
#ifdef WL_DISPLAY_DELETE_ID
#undef WL_DISPLAY_DELETE_ID
#endif
#ifdef WL_REGISTRY_GLOBAL
#undef WL_REGISTRY_GLOBAL
#endif
#ifdef WL_REGISTRY_GLOBAL_REMOVE
#undef WL_REGISTRY_GLOBAL_REMOVE
#endif
#ifdef WL_CALLBACK_DONE
#undef WL_CALLBACK_DONE
#endif
#ifdef WL_KEYBOARD_ENTER
#undef WL_KEYBOARD_ENTER
#endif
#ifdef WL_KEYBOARD_LEAVE
#undef WL_KEYBOARD_LEAVE
#endif
#ifdef WL_KEYBOARD_KEY
#undef WL_KEYBOARD_KEY
#endif
#ifdef WL_KEYBOARD_MODIFIERS
#undef WL_KEYBOARD_MODIFIERS
#endif
#ifdef WL_POINTER_ENTER
#undef WL_POINTER_ENTER
#endif
#ifdef WL_POINTER_LEAVE
#undef WL_POINTER_LEAVE
#endif
#ifdef WL_POINTER_BUTTON
#undef WL_POINTER_BUTTON
#endif
#ifdef WL_TOUCH_DOWN
#undef WL_TOUCH_DOWN
#endif
#ifdef WL_TOUCH_UP
#undef WL_TOUCH_UP
#endif
#ifdef WL_TOUCH_MOTION
#undef WL_TOUCH_MOTION
#endif
#ifdef WL_TOUCH_FRAME
#undef WL_TOUCH_FRAME
#endif
#ifdef WL_TOUCH_CANCEL
#undef WL_TOUCH_CANCEL
#endif
#ifdef WL_TOUCH_SHAPE
#undef WL_TOUCH_SHAPE
#endif
#ifdef WL_TOUCH_ORIENTATION
#undef WL_TOUCH_ORIENTATION
#endif
#ifdef WL_SHM_FORMAT
#undef WL_SHM_FORMAT
#endif
#ifdef WL_DRM_DEVICE
#undef WL_DRM_DEVICE
#endif
#ifdef WL_DRM_FORMAT
#undef WL_DRM_FORMAT
#endif
#ifdef WL_DRM_AUTHENTICATED
#undef WL_DRM_AUTHENTICATED
#endif
#ifdef WL_DRM_CAPABILITIES
#undef WL_DRM_CAPABILITIES
#endif
#ifdef XDG_POPUP_CONFIGURE
#undef XDG_POPUP_CONFIGURE
#endif
#ifdef XDG_SURFACE_CONFIGURE
#undef XDG_SURFACE_CONFIGURE
#endif
#ifdef XDG_TOPLEVEL_CONFIGURE
#undef XDG_TOPLEVEL_CONFIGURE
#endif
#ifdef XDG_TOPLEVEL_CLOSE
#undef XDG_TOPLEVEL_CLOSE
#endif
#ifdef ZWP_TABLET_SEAT_V2_TABLET_ADDED
#undef ZWP_TABLET_SEAT_V2_TABLET_ADDED
#endif
#ifdef ZWP_TABLET_SEAT_V2_TOOL_ADDED
#undef ZWP_TABLET_SEAT_V2_TOOL_ADDED
#endif
#ifdef ZWP_TABLET_SEAT_V2_PAD_ADDED
#undef ZWP_TABLET_SEAT_V2_PAD_ADDED
#endif
#ifdef ZWP_TABLET_V2_REMOVED
#undef ZWP_TABLET_V2_REMOVED
#endif
#ifdef ZWP_TABLET_TOOL_V2_PROXIMITY_IN
#undef ZWP_TABLET_TOOL_V2_PROXIMITY_IN
#endif
#ifdef ZWP_TABLET_TOOL_V2_PROXIMITY_OUT
#undef ZWP_TABLET_TOOL_V2_PROXIMITY_OUT
#endif
#ifdef ZWP_TABLET_TOOL_V2_DOWN
#undef ZWP_TABLET_TOOL_V2_DOWN
#endif
#ifdef GODOT_EMBEDDED_CLIENT_DISCONNECTED
#undef GODOT_EMBEDDED_CLIENT_DISCONNECTED
#endif
#ifdef GODOT_EMBEDDED_CLIENT_WINDOW_EMBEDDED
#undef GODOT_EMBEDDED_CLIENT_WINDOW_EMBEDDED
#endif
#ifdef GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_IN
#undef GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_IN
#endif
#ifdef GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_OUT
#undef GODOT_EMBEDDED_CLIENT_WINDOW_FOCUS_OUT
#endif
#ifdef GODOT_EMBEDDING_COMPOSITOR_CLIENT
#undef GODOT_EMBEDDING_COMPOSITOR_CLIENT
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/rendering_context_driver_vulkan_wayland.cpp
// ============================================================================

#ifdef VULKAN_ENABLED

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#include <drivers/vulkan/godot_vulkan.h>

const char *RenderingContextDriverVulkanWayland::_get_platform_surface_extension() const {
	return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
}

RenderingContextDriver::SurfaceID RenderingContextDriverVulkanWayland::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);

	VkWaylandSurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	create_info.display = wpd->display;
	create_info.surface = wpd->surface;

	VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
	VkResult err = vkCreateWaylandSurfaceKHR(instance_get(), &create_info, get_allocation_callbacks(VK_OBJECT_TYPE_SURFACE_KHR), &vk_surface);
	ERR_FAIL_COND_V(err != VK_SUCCESS, SurfaceID());

	Surface *surface = memnew(Surface);
	surface->vk_surface = vk_surface;
	return SurfaceID(surface);
}

RenderingContextDriverVulkanWayland::RenderingContextDriverVulkanWayland() {
	// Does nothing.
}

RenderingContextDriverVulkanWayland::~RenderingContextDriverVulkanWayland() {
	// Does nothing.
}

#endif // VULKAN_ENABLED


// --- Merged from platform/linuxbsd/wayland/egl_manager_wayland.cpp ---
#include "platform/linuxbsd/wayland/display_server_wayland.h"

#ifdef WAYLAND_ENABLED
#ifdef EGL_ENABLED
#ifdef GLES3_ENABLED

const char *EGLManagerWayland::_get_platform_extension_name() const {
	return "EGL_KHR_platform_wayland";
}

EGLenum EGLManagerWayland::_get_platform_extension_enum() const {
	return EGL_PLATFORM_WAYLAND_KHR;
}

EGLenum EGLManagerWayland::_get_platform_api_enum() const {
	return EGL_OPENGL_API;
}

Vector<EGLAttrib> EGLManagerWayland::_get_platform_display_attributes() const {
	return Vector<EGLAttrib>();
}

Vector<EGLint> EGLManagerWayland::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_MAJOR_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_CONTEXT_MINOR_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

#endif // GLES3_ENABLED
#endif // EGL_ENABLED
#endif // WAYLAND_ENABLED

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from platform/linuxbsd/wayland/egl_manager_wayland_gles.cpp
// ============================================================================

#include "platform/linuxbsd/wayland/display_server_wayland.h"

#ifdef WAYLAND_ENABLED
#ifdef EGL_ENABLED
#ifdef GLES3_ENABLED

const char *EGLManagerWaylandGLES::_get_platform_extension_name() const {
	return "EGL_KHR_platform_wayland";
}

EGLenum EGLManagerWaylandGLES::_get_platform_extension_enum() const {
	return EGL_PLATFORM_WAYLAND_KHR;
}

EGLenum EGLManagerWaylandGLES::_get_platform_api_enum() const {
	return EGL_OPENGL_ES_API;
}

Vector<EGLAttrib> EGLManagerWaylandGLES::_get_platform_display_attributes() const {
	return Vector<EGLAttrib>();
}

Vector<EGLint> EGLManagerWaylandGLES::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_MAJOR_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

#endif // GLES3_ENABLED
#endif // EGL_ENABLED
#endif // WAYLAND_ENABLED

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif
