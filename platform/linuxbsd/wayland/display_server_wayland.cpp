/**************************************************************************/
/*  display_server_wayland.cpp                                            */
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
#include "servers/rendering/rendering_server.h"

#ifdef RD_ENABLED
#ifdef VULKAN_ENABLED
#include "wayland/rendering_context_driver_vulkan_wayland.h"
#endif

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#endif

#ifdef GLES3_ENABLED
#include "core/io/file_access.h"
#include "detect_prime_egl.h"
#include "drivers/egl/egl_manager.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "wayland/egl_manager_wayland.h"
#include "wayland/egl_manager_wayland_gles.h"
#endif

#ifdef DBUS_ENABLED
#include "freedesktop_at_spi_monitor.h"
#include "freedesktop_portal_desktop.h"
#include "freedesktop_screensaver.h"

#ifdef SOWRAP_ENABLED
#include "dbus-so_wrap.h"
#else
#include <dbus/dbus.h>
#endif
#endif

#ifdef SPEECHD_ENABLED
#include "tts_linux.h"
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
	}

	return app_id;
}

void DisplayServerWayland::dispatch_input_events(const Ref<InputEvent>& p_event)
{
	static_cast<DisplayServerWayland*>(get_singleton())->_dispatch_input_event(p_event);
}

void DisplayServerWayland::_delete_window(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];

	ERR_FAIL_COND(!windows.has(wd.root_id));
	WindowData& root_wd = windows[wd.root_id];

	// We need to clear the hovered window now. By the time we'll get a hover
	// change from the thread, it will be already gone and we won't have anywhere
	// to send the mouse exit event to.
	if (hovered_window_id == p_window_id) {
		_hover_window(DisplayServerEnums::INVALID_WINDOW_ID);
	}

	// The XDG shell specification requires us to clear all popups in reverse order.
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

	AccessibilityServer::get_singleton()->window_destroy(p_window_id);

	if (wd.visible) {
#ifdef VULKAN_ENABLED
		if (rendering_device) {
			rendering_device->screen_free(p_window_id);
		}

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

void DisplayServerWayland::_hover_window(DisplayServerEnums::WindowID p_window_id)
{
	if (hovered_window_id == p_window_id) {
		return;
	}

	// NOTE: _send_window_event invokes callbacks and might have side effects!
	// (e.g. a popup indirectly calling this method again). Make sure to do all of
	// your bookkeping BEFORE sending window events!

	DisplayServerEnums::WindowID old_hover = hovered_window_id;

	if (old_hover != DisplayServerEnums::INVALID_WINDOW_ID) {
		hovered_window_id = DisplayServerEnums::INVALID_WINDOW_ID;

		DEBUG_LOG_WAYLAND(vformat("[DEBUG] Notifying mouse exit to window %d.", old_hover));
		_send_window_event(DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT, old_hover);
	}

	// The window might have been destroyed in the meantime.
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
		return (native_menu && native_menu->has_feature(NativeMenu::FEATURE_GLOBAL_MENU));
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

	// case DisplayServerEnums::FEATURE_NATIVE_DIALOG:
	// case DisplayServerEnums::FEATURE_NATIVE_DIALOG_INPUT:
#ifdef DBUS_ENABLED
	case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE:
	case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA:
	case DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_MIME: {
		return (portal_desktop && portal_desktop->is_supported() &&
				portal_desktop->is_file_chooser_supported());
	} break;
	case DisplayServerEnums::FEATURE_NATIVE_COLOR_PICKER: {
		return (portal_desktop && portal_desktop->is_supported() &&
				portal_desktop->is_screenshot_supported());
	} break;
#endif

#ifdef SPEECHD_ENABLED
	case DisplayServerEnums::FEATURE_TEXT_TO_SPEECH: {
		return true;
	} break;
#endif

	case DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER: {
		return AccessibilityServer::get_singleton()->is_supported();
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
	const_cast<DisplayServerWayland*>(this)->tts = memnew(TTS_Linux);
}

bool DisplayServerWayland::tts_is_speaking() const
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return tts->is_speaking();
}

bool DisplayServerWayland::tts_is_paused() const
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL_V(tts, false);
	return tts->is_paused();
}

void DisplayServerWayland::tts_speak(const String& p_text, const String& p_voice, int p_volume,
	float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	tts->speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServerWayland::tts_pause()
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	tts->pause();
}

void DisplayServerWayland::tts_resume()
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	tts->resume();
}

void DisplayServerWayland::tts_stop()
{
	if (unlikely(!tts)) {
		initialize_tts();
	}
	ERR_FAIL_NULL(tts);
	tts->stop();
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
		// Prefers dark theme.
		return true;
	case 2:
		// Prefers light theme.
		return false;
	default:
		// Preference unknown.
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

	// Are we sure this is the only way? This seems sus.
	// TODO: Handle tablets properly.
	// mouse_button_mask.set_flag(MouseButtonMask((int64_t)wls.current_seat->tablet_tool_data.pressed_button_mask));

	return wayland_thread.pointer_get_button_mask();
}

// NOTE: According to the Wayland specification, this method will only do
// anything if the user has interacted with the application by sending a
// "recent enough" input event.
// TODO: Add this limitation to the documentation.
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

	// TODO: Fallback to next media type on missing module or parse error.
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

int DisplayServerWayland::get_primary_screen() const
{
	// AFAIK Wayland doesn't allow knowing (nor we care) about which screen is
	// primary.
	return 0;
}

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

	// Could not get DPI.
	return 96;
}

float DisplayServerWayland::screen_get_scale(int p_screen) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	if (p_screen == DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) {
		// Wayland does not expose fractional scale factors at the screen-level, but
		// some code relies on it. Since this special screen is the default and a lot
		// of code relies on it, we'll return the window's scale, which is what we
		// really care about. After all, we have very little use of the actual screen
		// enumeration APIs and we're (for now) in single-window mode anyways.
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
		   (Input::get_singleton() && Input::get_singleton()->is_emulating_touch_from_mouse());
}

void DisplayServerWayland::screen_set_keep_on(bool p_enable)
{
	MutexLock mutex_lock(wayland_thread.mutex);

	// FIXME: For some reason this does not also windows from the wayland thread.

	if (screen_is_kept_on() == p_enable) {
		return;
	}

	wayland_thread.window_set_idle_inhibition(DisplayServerEnums::MAIN_WINDOW_ID, p_enable);

#ifdef DBUS_ENABLED
	if (portal_desktop && portal_desktop->is_inhibit_supported()) {
		if (p_enable) {
			// Attach the inhibit request to the main window, not the last focused window,
			// on the basis that inhibiting the screensaver is global state for the application.
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
	// FIXME: Multiwindow support.
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

	if (!AccessibilityServer::get_singleton()->window_create(wd.id, nullptr)) {
		if (OS::get_singleton()->is_stdout_verbose()) {
			ERR_PRINT("Can't create an accessibility adapter for window, accessibility support "
					  "disabled!");
		}
	}

	// NOTE: Remember to clear its position if this window will be a toplevel. We
	// can only know once we show it.
	wd.rect = p_rect;

	wd.title = "Voltaire";
	wd.parent_id = p_transient_parent;
	return id;
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
		return 0; // Not supported.
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
#endif // GLES3_ENABLED

	default: {
		return 0;
	} break;
	}
}

DisplayServerEnums::WindowID DisplayServerWayland::get_window_at_screen_position(
	const Point2i& p_position) const
{
	// Standard Wayland APIs don't support this.
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
	// TODO
	DEBUG_LOG_WAYLAND(vformat("wayland stub window_set_mouse_passthrough region %s", p_region));
}

int DisplayServerWayland::window_get_current_screen(DisplayServerEnums::WindowID p_window_id) const
{
	ERR_FAIL_COND_V(!windows.has(p_window_id), DisplayServerEnums::INVALID_SCREEN);
	// Standard Wayland APIs don't support getting the screen of a window.
	return 0;
}

void DisplayServerWayland::window_set_current_screen(
	int p_screen, DisplayServerEnums::WindowID p_window_id)
{
	// Standard Wayland APIs don't support setting the screen of a window.
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
	// Unsupported with toplevels.
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

	// FIXME: Is `p_size.x < wd.min_size.x || p_size.y < wd.min_size.y` == `p_size < wd.min_size`?
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

		// NOTE: Looks like live unparenting is not really practical unfortunately.
		// See WaylandThread::window_set_parent for more info.
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

	// FIXME: Is `p_size.x > wd.max_size.x || p_size.y > wd.max_size.y` == `p_size > wd.max_size`?
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

	Size2i new_size = p_size;
	new_size = p_size.maxi(1);

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

	// I don't think there's a way of actually knowing the size of the window
	// decoration in Wayland, at least in the case of SSDs, nor that it would be
	// that useful in this case. We'll just return the main window's size.
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
	// Standard Wayland APIs don't support this.
}

bool DisplayServerWayland::window_is_focused(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	return wayland_thread.pointer_get_pointed_window_id() == p_window_id;
}

bool DisplayServerWayland::window_can_draw(DisplayServerEnums::WindowID p_window_id) const
{
	MutexLock mutex_lock(wayland_thread.mutex);

	uint64_t last_frame_time = wayland_thread.window_get_last_frame_time(p_window_id);
	uint64_t time_since_frame = OS::get_singleton()->get_ticks_usec() - last_frame_time;

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
	return suspend_state == SuspendState::NONE;
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

// NOTE: While Wayland is supposed to be tear-free, wayland-protocols version
// 1.30 added a protocol for allowing async flips which is supposed to be
// handled by drivers such as Vulkan. We can then just ask to disable v-sync and
// hope for the best. See:
// https://gitlab.freedesktop.org/wayland/wayland-protocols/-/commit/6394f0b4f3be151076f10a845a2fb131eeb56706
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
#endif // VULKAN_ENABLED

#ifdef GLES3_ENABLED
	if (egl_manager) {
		egl_manager->set_use_vsync(p_vsync_mode != DisplayServerEnums::VSYNC_DISABLED);

		// NOTE: Mesa's EGL implementation does not seem to make use of fifo_v1 so
		// we'll have to always emulate V-Sync.
		wd.emulate_vsync = egl_manager->is_using_vsync();

		if (wd.emulate_vsync) {
			print_verbose("VSYNC: manually throttling frames with swap delay 0.");
			egl_manager->set_use_vsync(false);
		}
	}
#endif // GLES3_ENABLED
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
#endif // VULKAN_ENABLED

#ifdef GLES3_ENABLED
	if (egl_manager) {
		return egl_manager->is_using_vsync() ? DisplayServerEnums::VSYNC_ENABLED
											 : DisplayServerEnums::VSYNC_DISABLED;
	}
#endif // GLES3_ENABLED

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
	DisplayServerEnums::WindowID window_id = p_window.id;

#if defined(RD_ENABLED)
	if (rendering_context) {
		// The `display/window/hdr/request_hdr_output` project setting makes the main window
		// "request" HDR. On Windows, this means enable HDR for the main window if it is on an HDR
		// screen. Since all screens support HDR on Wayland, we use whether the window "prefers" HDR
		// or not instead.
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
	ERR_FAIL_COND_V(!windows.has(p_window_id), false);
	bool renderer_supports_hdr_output = false;
	bool surface_supports_hdr_output = false;
#if defined(RD_ENABLED)
	if (rendering_device &&
		rendering_device->has_feature(RenderingDevice::Features::SUPPORTS_HDR_OUTPUT)) {
		renderer_supports_hdr_output = true;
		surface_supports_hdr_output =
			rendering_device->screen_get_hdr_output_supported(p_window_id);
	}
#endif
	if (!renderer_supports_hdr_output) {
		return false;
	}

	if (!surface_supports_hdr_output) {
		return false;
	}

	const WindowData& wd = windows[p_window_id];

	return wd.color_profile.target_max_luminance > wd.color_profile.reference_luminance;
}

void DisplayServerWayland::window_request_hdr_output(
	const bool p_enabled, DisplayServerEnums::WindowID p_window_id)
{
	if (p_enabled) {
		bool renderer_supports_hdr_output = false;
		bool surface_supports_hdr_output = false;
#if defined(RD_ENABLED)
		if (rendering_device &&
			rendering_device->has_feature(RenderingDevice::Features::SUPPORTS_HDR_OUTPUT)) {
			renderer_supports_hdr_output = true;
			surface_supports_hdr_output =
				rendering_device->screen_get_hdr_output_supported(p_window_id);
		}
#endif
		if (!renderer_supports_hdr_output) {
			WARN_PRINT("HDR output requested, but is not supported by the renderer or rendering "
					   "device driver.");
			return;
		}

		if (!surface_supports_hdr_output) {
			WARN_PRINT("HDR output requested, but the window does not support an HDR format.");
			return;
		}
	}

	ERR_FAIL_COND(!windows.has(p_window_id));
	WindowData& wd = windows[p_window_id];
	wd.hdr_requested = p_enabled;

	_window_update_hdr_state(wd);
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
		// Hidden.
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
				// We have a cached cursor. Nice.
				wayland_thread.cursor_set_shape(p_shape);
				return;
			}

			// We're changing this cursor; we'll have to rebuild it.
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
		// Clear cache and reset to default system cursor.
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
		return OS::get_singleton()->get_process_id();
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

	// xkbcommon exposes only the layout's name, which looks like it overlaps with
	// its language.
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

	// If not found, fallback to QWERTY.
	// This should match the behavior of the event pump.
	if (key == Key::NONE) {
		return p_keycode;
	}

	if (key >= Key::A + 32 && key <= Key::Z + 32) {
		key -= 'a' - 'A';
	}

	// Make it consistent with the keys returned by `Input`.
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
	// Due to various reasons, we manually handle display synchronization by
	// waiting for a frame event (request to draw) or, if available, the actual
	// window's suspend status. When a window is suspended, we can avoid drawing
	// altogether, either because the compositor told us that we don't need to or
	// because the pace of the frame events became unreliable.
	bool frame = wayland_thread.wait_frame_suspend_ms(WAYLAND_MAX_FRAME_TIME_US / 1000);
	if (!frame) {
		suspend_state = SuspendState::TIMEOUT;
	}
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
	return OS::get_singleton()->is_layered_allowed();
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

DisplayServer* DisplayServerWayland::create_func(const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Point2i* p_position, const Size2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	DisplayServer* ds = memnew(DisplayServerWayland(p_rendering_driver, p_mode, p_vsync_mode,
		p_flags, p_resolution, p_context, p_parent_window, r_error));
	if (r_error != OK) {
		ERR_PRINT("Can't create the Wayland display server.");
		memdelete(ds);

		return nullptr;
	}
	return ds;
}

DisplayServerWayland::~DisplayServerWayland()
{
	wayland_thread.mutex.lock();

	if (native_menu) {
		memdelete(native_menu);
		native_menu = nullptr;
	}

	// Iterating on the window map while we delete stuff from it is a bit
	// uncomfortable, plus we can't even delete /all/ windows in an arbitrary order
	// (due to popups).
	List<DisplayServerEnums::WindowID> toplevels;

	for (const KeyValue<DisplayServerEnums::WindowID, WindowData>& pair : windows) {
		DisplayServerEnums::WindowID id = pair.key;

		if (!window_get_flag(DisplayServerEnums::WINDOW_FLAG_POPUP_WM_HINT, id)) {
			toplevels.push_back(id);
		}
		else {
			AccessibilityServer::get_singleton()->window_destroy(id);
		}
	}

	for (DisplayServerEnums::WindowID& id : toplevels) {
		_delete_window(id);
	}
	windows.clear();

	// The thread needs the mutex to clean up. We're not going to touch Wayland
	// stuff anymore from now on anyways.
	wayland_thread.mutex.unlock();
	wayland_thread.destroy();

	// Destroy all drivers.
#ifdef RD_ENABLED
	memdelete(rendering_device);
	memdelete(rendering_context);
#endif

#ifdef SPEECHD_ENABLED
	memdelete(tts);
#endif

#ifdef DBUS_ENABLED
	memdelete(portal_desktop);
	memdelete(screensaver);
	memdelete(atspi_monitor);
#endif
}

void DisplayServerWayland::register_wayland_driver()
{
	register_create_function("wayland", create_func, get_rendering_drivers_func);
}

#endif // WAYLAND_ENABLED


