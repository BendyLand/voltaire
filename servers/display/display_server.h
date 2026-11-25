/**************************************************************************/
/*  display_server.h                                                      */
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

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/io/ip.h"
#include "core/os/os.h"
#include "core/io/ip.h"
// --- Folded from servers/display/display_server_enums.h ---
namespace DisplayServerEnums
{

/* MAIN */

enum Context
{
	CONTEXT_EDITOR,
	CONTEXT_PROJECTMAN,
	CONTEXT_ENGINE,
};

enum Feature
{
#ifndef DISABLE_DEPRECATED
	FEATURE_GLOBAL_MENU,
#endif
	FEATURE_SUBWINDOWS,
	FEATURE_TOUCHSCREEN,
	FEATURE_MOUSE,
	FEATURE_MOUSE_WARP,
	FEATURE_CLIPBOARD,
	FEATURE_VIRTUAL_KEYBOARD,
	FEATURE_CURSOR_SHAPE,
	FEATURE_CUSTOM_CURSOR_SHAPE,
	FEATURE_NATIVE_DIALOG,
	FEATURE_IME,
	FEATURE_WINDOW_TRANSPARENCY,
	FEATURE_HIDPI,
	FEATURE_ICON,
	FEATURE_NATIVE_ICON,
	FEATURE_ORIENTATION,
	FEATURE_SWAP_BUFFERS,
	FEATURE_KEEP_SCREEN_ON,
	FEATURE_CLIPBOARD_PRIMARY,
	FEATURE_TEXT_TO_SPEECH,
	FEATURE_EXTEND_TO_TITLE,
	FEATURE_SCREEN_CAPTURE,
	FEATURE_STATUS_INDICATOR,
	FEATURE_NATIVE_HELP,
	FEATURE_NATIVE_DIALOG_INPUT,
	FEATURE_NATIVE_DIALOG_FILE,
	FEATURE_NATIVE_DIALOG_FILE_EXTRA,
	FEATURE_WINDOW_DRAG,
	FEATURE_SCREEN_EXCLUDE_FROM_CAPTURE,
	FEATURE_WINDOW_EMBEDDING,
	FEATURE_NATIVE_DIALOG_FILE_MIME,
	FEATURE_EMOJI_AND_SYMBOL_PICKER,
	FEATURE_NATIVE_COLOR_PICKER,
	FEATURE_SELF_FITTING_WINDOWS,
	FEATURE_ACCESSIBILITY_SCREEN_READER,
	FEATURE_HDR_OUTPUT,
	FEATURE_PIP_MODE,
};

/* RENDERING DEVICE */

enum RenderingDeviceCreationStatus
{
	UNKNOWN,
	SUCCESS,
	FAILURE,
};

/* TTS */

enum TTSUtteranceEvent
{
	TTS_UTTERANCE_STARTED,
	TTS_UTTERANCE_ENDED,
	TTS_UTTERANCE_CANCELED,
	TTS_UTTERANCE_BOUNDARY,
	TTS_UTTERANCE_MAX,
};

/* MOUSE */

// Keep synced with InputClassEnums::MouseMode enum.
enum MouseMode
{
	MOUSE_MODE_VISIBLE,			// Input::MouseMode::MOUSE_MODE_VISIBLE
	MOUSE_MODE_HIDDEN,			// Input::MouseMode::MOUSE_MODE_HIDDEN
	MOUSE_MODE_CAPTURED,		// Input::MouseMode::MOUSE_MODE_CAPTURED
	MOUSE_MODE_CONFINED,		// Input::MouseMode::MOUSE_MODE_CONFINED
	MOUSE_MODE_CONFINED_HIDDEN, // Input::MouseMode::MOUSE_MODE_CONFINED_HIDDEN
	MOUSE_MODE_MAX,				// Input::MouseMode::MOUSE_MODE_MAX
};

// Keep synced with InputClassEnums and Control enums.
#undef CursorShape

enum CursorShape
{
	CURSOR_ARROW,
	CURSOR_IBEAM,
	CURSOR_POINTING_HAND,
	CURSOR_CROSS,
	CURSOR_WAIT,
	CURSOR_BUSY,
	CURSOR_DRAG,
	CURSOR_CAN_DROP,
	CURSOR_FORBIDDEN,
	CURSOR_VSIZE,
	CURSOR_HSIZE,
	CURSOR_BDIAGSIZE,
	CURSOR_FDIAGSIZE,
	CURSOR_MOVE,
	CURSOR_VSPLIT,
	CURSOR_HSPLIT,
	CURSOR_HELP,
	CURSOR_MAX
};

/* KEYBOARD */

// Keep synced with LineEdit::VirtualKeyboardType enum.
enum VirtualKeyboardType
{
	KEYBOARD_TYPE_DEFAULT,
	KEYBOARD_TYPE_MULTILINE,
	KEYBOARD_TYPE_NUMBER,
	KEYBOARD_TYPE_NUMBER_DECIMAL,
	KEYBOARD_TYPE_PHONE,
	KEYBOARD_TYPE_EMAIL_ADDRESS,
	KEYBOARD_TYPE_PASSWORD,
	KEYBOARD_TYPE_URL
};

/* SCREEN */

// Keep synced with EditorSettings::InitialScreen enum.
enum
{
	INVALID_SCREEN = -1,
	SCREEN_WITH_MOUSE_FOCUS = -4,
	SCREEN_WITH_KEYBOARD_FOCUS = -3,
	SCREEN_PRIMARY = -2,
	SCREEN_OF_MAIN_WINDOW = -1, // Note: for the main window, determine screen from position.
};

// Keep the ScreenOrientation enum values in sync with the `display/window/handheld/orientation`
// project setting hint.
enum ScreenOrientation
{
	SCREEN_LANDSCAPE,
	SCREEN_PORTRAIT,
	SCREEN_REVERSE_LANDSCAPE,
	SCREEN_REVERSE_PORTRAIT,
	SCREEN_SENSOR_LANDSCAPE,
	SCREEN_SENSOR_PORTRAIT,
	SCREEN_SENSOR,
};

/* WINDOW */

typedef int WindowID;

enum
{
	MAIN_WINDOW_ID = 0,
	INVALID_WINDOW_ID = -1,
	INVALID_INDICATOR_ID = -1
};

enum HandleType
{
	DISPLAY_HANDLE,
	WINDOW_HANDLE,
	WINDOW_VIEW,
	OPENGL_CONTEXT,
	EGL_DISPLAY,
	EGL_CONFIG,
	GLX_VISUALID,
	GLX_FBCONFIG,
};

enum WindowMode
{
	WINDOW_MODE_WINDOWED,
	WINDOW_MODE_MINIMIZED,
	WINDOW_MODE_MAXIMIZED,
	WINDOW_MODE_FULLSCREEN,
	WINDOW_MODE_EXCLUSIVE_FULLSCREEN,
};

enum WindowFlags
{
	WINDOW_FLAG_RESIZE_DISABLED,
	WINDOW_FLAG_BORDERLESS,
	WINDOW_FLAG_ALWAYS_ON_TOP,
	WINDOW_FLAG_TRANSPARENT,
	WINDOW_FLAG_NO_FOCUS,
	WINDOW_FLAG_POPUP,
	WINDOW_FLAG_EXTEND_TO_TITLE,
	WINDOW_FLAG_MOUSE_PASSTHROUGH,
	WINDOW_FLAG_SHARP_CORNERS,
	WINDOW_FLAG_EXCLUDE_FROM_CAPTURE,
	WINDOW_FLAG_POPUP_WM_HINT,
	WINDOW_FLAG_MINIMIZE_DISABLED,
	WINDOW_FLAG_MAXIMIZE_DISABLED,
	WINDOW_FLAG_MAX,
};

// Separate enum otherwise we get warnings in switches not handling all values.
enum WindowFlagsBit
{
	WINDOW_FLAG_RESIZE_DISABLED_BIT = (1 << WINDOW_FLAG_RESIZE_DISABLED),
	WINDOW_FLAG_BORDERLESS_BIT = (1 << WINDOW_FLAG_BORDERLESS),
	WINDOW_FLAG_ALWAYS_ON_TOP_BIT = (1 << WINDOW_FLAG_ALWAYS_ON_TOP),
	WINDOW_FLAG_TRANSPARENT_BIT = (1 << WINDOW_FLAG_TRANSPARENT),
	WINDOW_FLAG_NO_FOCUS_BIT = (1 << WINDOW_FLAG_NO_FOCUS),
	WINDOW_FLAG_POPUP_BIT = (1 << WINDOW_FLAG_POPUP),
	WINDOW_FLAG_EXTEND_TO_TITLE_BIT = (1 << WINDOW_FLAG_EXTEND_TO_TITLE),
	WINDOW_FLAG_MOUSE_PASSTHROUGH_BIT = (1 << WINDOW_FLAG_MOUSE_PASSTHROUGH),
	WINDOW_FLAG_SHARP_CORNERS_BIT = (1 << WINDOW_FLAG_SHARP_CORNERS),
	WINDOW_FLAG_EXCLUDE_FROM_CAPTURE_BIT = (1 << WINDOW_FLAG_EXCLUDE_FROM_CAPTURE),
	WINDOW_FLAG_POPUP_WM_HINT_BIT = (1 << WINDOW_FLAG_POPUP_WM_HINT),
	WINDOW_FLAG_MINIMIZE_DISABLED_BIT = (1 << WINDOW_FLAG_MINIMIZE_DISABLED),
	WINDOW_FLAG_MAXIMIZE_DISABLED_BIT = (1 << WINDOW_FLAG_MAXIMIZE_DISABLED),
};

enum WindowEvent
{
	WINDOW_EVENT_MOUSE_ENTER,
	WINDOW_EVENT_MOUSE_EXIT,
	WINDOW_EVENT_FOCUS_IN,
	WINDOW_EVENT_FOCUS_OUT,
	WINDOW_EVENT_CLOSE_REQUEST,
	WINDOW_EVENT_GO_BACK_REQUEST,
	WINDOW_EVENT_DPI_CHANGE,
	WINDOW_EVENT_TITLEBAR_CHANGE,
	WINDOW_EVENT_FORCE_CLOSE,
	WINDOW_EVENT_OUTPUT_MAX_LINEAR_VALUE_CHANGED,
};

enum WindowResizeEdge
{
	WINDOW_EDGE_TOP_LEFT,
	WINDOW_EDGE_TOP,
	WINDOW_EDGE_TOP_RIGHT,
	WINDOW_EDGE_LEFT,
	WINDOW_EDGE_RIGHT,
	WINDOW_EDGE_BOTTOM_LEFT,
	WINDOW_EDGE_BOTTOM,
	WINDOW_EDGE_BOTTOM_RIGHT,
	WINDOW_EDGE_MAX,
};

// Keep the VSyncMode enum values in sync with the `display/window/vsync/vsync_mode`
// project setting hint.
enum VSyncMode
{
	VSYNC_DISABLED,
	VSYNC_ENABLED,
	VSYNC_ADAPTIVE,
	VSYNC_MAILBOX
};

enum ProgressState
{
	PROGRESS_STATE_NOPROGRESS,
	PROGRESS_STATE_INDETERMINATE,
	PROGRESS_STATE_NORMAL,
	PROGRESS_STATE_ERROR,
	PROGRESS_STATE_PAUSED,
};

/* DIALOGS */

enum FileDialogMode
{
	FILE_DIALOG_MODE_OPEN_FILE,
	FILE_DIALOG_MODE_OPEN_FILES,
	FILE_DIALOG_MODE_OPEN_DIR,
	FILE_DIALOG_MODE_OPEN_ANY,
	FILE_DIALOG_MODE_SAVE_FILE,
	FILE_DIALOG_MODE_SAVE_MAX
};

/* STATUS INDICATOR */

typedef int IndicatorID;

#ifndef DISABLE_DEPRECATED

/* ACCESSIBILITY */

enum AccessibilityRole
{
	ROLE_UNKNOWN,
	ROLE_DEFAULT_BUTTON,
	ROLE_AUDIO,
	ROLE_VIDEO,
	ROLE_STATIC_TEXT,
	ROLE_CONTAINER,
	ROLE_PANEL,
	ROLE_BUTTON,
	ROLE_LINK,
	ROLE_CHECK_BOX,
	ROLE_RADIO_BUTTON,
	ROLE_CHECK_BUTTON,
	ROLE_SCROLL_BAR,
	ROLE_SCROLL_VIEW,
	ROLE_SPLITTER,
	ROLE_SLIDER,
	ROLE_SPIN_BUTTON,
	ROLE_PROGRESS_INDICATOR,
	ROLE_TEXT_FIELD,
	ROLE_MULTILINE_TEXT_FIELD,
	ROLE_COLOR_PICKER,
	ROLE_TABLE,
	ROLE_CELL,
	ROLE_ROW,
	ROLE_ROW_GROUP,
	ROLE_ROW_HEADER,
	ROLE_COLUMN_HEADER,
	ROLE_TREE,
	ROLE_TREE_ITEM,
	ROLE_LIST,
	ROLE_LIST_ITEM,
	ROLE_LIST_BOX,
	ROLE_LIST_BOX_OPTION,
	ROLE_TAB_BAR,
	ROLE_TAB,
	ROLE_TAB_PANEL,
	ROLE_MENU_BAR,
	ROLE_MENU,
	ROLE_MENU_ITEM,
	ROLE_MENU_ITEM_CHECK_BOX,
	ROLE_MENU_ITEM_RADIO,
	ROLE_IMAGE,
	ROLE_WINDOW,
	ROLE_TITLE_BAR,
	ROLE_DIALOG,
	ROLE_TOOLTIP,
	ROLE_REGION,
	ROLE_TEXT_RUN,
};

enum AccessibilityPopupType
{
	POPUP_MENU,
	POPUP_LIST,
	POPUP_TREE,
	POPUP_DIALOG,
};

enum AccessibilityFlags
{
	FLAG_HIDDEN,
	FLAG_MULTISELECTABLE,
	FLAG_REQUIRED,
	FLAG_VISITED,
	FLAG_BUSY,
	FLAG_MODAL,
	FLAG_TOUCH_PASSTHROUGH,
	FLAG_READONLY,
	FLAG_DISABLED,
	FLAG_CLIPS_CHILDREN,
};

enum AccessibilityAction
{
	ACTION_CLICK,
	ACTION_FOCUS,
	ACTION_BLUR,
	ACTION_COLLAPSE,
	ACTION_EXPAND,
	ACTION_DECREMENT,
	ACTION_INCREMENT,
	ACTION_HIDE_TOOLTIP,
	ACTION_SHOW_TOOLTIP,
	ACTION_SET_TEXT_SELECTION,
	ACTION_REPLACE_SELECTED_TEXT,
	ACTION_SCROLL_BACKWARD,
	ACTION_SCROLL_DOWN,
	ACTION_SCROLL_FORWARD,
	ACTION_SCROLL_LEFT,
	ACTION_SCROLL_RIGHT,
	ACTION_SCROLL_UP,
	ACTION_SCROLL_INTO_VIEW,
	ACTION_SCROLL_TO_POINT,
	ACTION_SET_SCROLL_OFFSET,
	ACTION_SET_VALUE,
	ACTION_SHOW_CONTEXT_MENU,
	ACTION_CUSTOM,
};

enum AccessibilityLiveMode
{
	LIVE_OFF,
	LIVE_POLITE,
	LIVE_ASSERTIVE,
};

enum AccessibilityScrollUnit
{
	SCROLL_UNIT_ITEM,
	SCROLL_UNIT_PAGE,
};

enum AccessibilityScrollHint
{
	SCROLL_HINT_TOP_LEFT,
	SCROLL_HINT_BOTTOM_RIGHT,
	SCROLL_HINT_TOP_EDGE,
	SCROLL_HINT_BOTTOM_EDGE,
	SCROLL_HINT_LEFT_EDGE,
	SCROLL_HINT_RIGHT_EDGE,
};

#endif // DISABLE_DEPRECATED

} // namespace DisplayServerEnums
// --- End servers/display/display_server_enums.h ---

class NativeMenu;
class Texture2D;

#undef CursorShape

namespace InputClassEnums
{
enum MouseMode : int;
enum CursorShape : int;
} // namespace InputClassEnums

// Defined here so it can be forward-declared.
struct TTSUtterance
{
	String text;
	String voice;
	int volume = 50;
	float pitch = 1.f;
	float rate = 1.f;
	int64_t id = 0;
};

class DisplayServer;

class DisplayServerDriver
{
public:
	DisplayServerDriver();
	virtual ~DisplayServerDriver();

	virtual String get_name() const = 0;

	virtual void set_context(DisplayServerEnums::Context p_context);
	virtual void set_native_icon(const String& p_filename);
	virtual void set_icon(const Ref<Image>& p_icon);

	virtual bool has_feature(DisplayServerEnums::Feature p_feature) const = 0;

	virtual void process_events() = 0;
	virtual void force_process_and_drop_events();

	virtual void release_rendering_thread();
	virtual void swap_buffers();

	virtual void beep() const;


	/* GLOBAL MENU */

public:
	/* TTS */
public:
	virtual bool tts_is_speaking() const;
	virtual bool tts_is_paused() const;
	virtual PackedStringArray tts_get_voices_for_language(const String& p_language) const;

	virtual void tts_speak(const String& p_text, const String& p_voice, int p_volume = 50,
		float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0,
		bool p_interrupt = false);
	virtual void tts_pause();
	virtual void tts_resume();
	virtual void tts_stop();

	virtual void tts_post_utterance_event(
		DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos = 0);

	/* THEME */

	virtual bool is_dark_mode_supported() const { return false; }

	virtual bool is_dark_mode() const { return false; }

	virtual Color get_accent_color() const { return Color(0, 0, 0, 0); }

	virtual Color get_base_color() const { return Color(0, 0, 0, 0); }

	/* MOUSE */

protected:
	static Ref<Image> _get_cursor_image_from_resource(
		const Ref<Resource>& p_cursor, const Vector2& p_hotspot);

public:
	virtual void mouse_set_mode(DisplayServerEnums::MouseMode p_mode);
	virtual DisplayServerEnums::MouseMode mouse_get_mode() const;
	virtual void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode);
	virtual DisplayServerEnums::MouseMode mouse_get_mode_override() const;
	virtual void mouse_set_mode_override_enabled(bool p_override_enabled);
	virtual bool mouse_is_mode_override_enabled() const;

	virtual void warp_mouse(const Point2i& p_position);
	virtual Point2i mouse_get_position() const;
	virtual uint32_t mouse_get_button_state() const;

	virtual void cursor_set_shape(DisplayServerEnums::CursorShape p_shape);
	virtual DisplayServerEnums::CursorShape cursor_get_shape() const;
	virtual void cursor_set_custom_image(const Ref<Resource>& p_cursor,
		DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW,
		const Vector2& p_hotspot = Vector2());

	/* KEYBOARD */

	virtual Point2i ime_get_selection() const;
	virtual String ime_get_text() const;

	virtual void virtual_keyboard_show(const String& p_existing_text,
		const Rect2& p_screen_rect = Rect2(),
		DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT,
		int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1);
	virtual void virtual_keyboard_hide();

	// Returns height of the currently shown virtual keyboard (0 if keyboard is hidden).
	virtual int virtual_keyboard_get_height() const;

	virtual bool has_hardware_keyboard() const;

	virtual int keyboard_get_layout_count() const;
	virtual int keyboard_get_current_layout() const;
	virtual void keyboard_set_current_layout(int p_index);
	virtual String keyboard_get_layout_language(int p_index) const;
	virtual String keyboard_get_layout_name(int p_index) const;
	virtual Key keyboard_get_keycode_from_physical(Key p_keycode) const;
	virtual Key keyboard_get_label_from_physical(Key p_keycode) const;

	/* TABLET */

	virtual int tablet_get_driver_count() const { return 1; }

	virtual String tablet_get_driver_name(int p_driver) const { return "default"; }

	virtual String tablet_get_current_driver() const { return "default"; }

	virtual void tablet_set_current_driver(const String& p_driver) {}

	/* CLIPBOARD */

	virtual void clipboard_set(const String& p_text);
	virtual String clipboard_get() const;
	virtual Ref<Image> clipboard_get_image() const;
	virtual bool clipboard_has() const;
	virtual bool clipboard_has_image() const;
	virtual void clipboard_set_primary(const String& p_text);
	virtual String clipboard_get_primary() const;

	/* SCREEN */

	static constexpr float SCREEN_REFRESH_RATE_FALLBACK = -1.0f;

	virtual Rect2i get_display_safe_area(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const
	{
		return screen_get_usable_rect(p_screen);
	}

	int _get_screen_index(int p_screen) const
	{
		switch (p_screen) {
		case DisplayServerEnums::SCREEN_WITH_MOUSE_FOCUS: {
			const Rect2i rect = Rect2i(mouse_get_position(), Vector2i(1, 1));
			return get_screen_from_rect(rect);
		} break;
		case DisplayServerEnums::SCREEN_WITH_KEYBOARD_FOCUS: {
			return get_keyboard_focus_screen();
		} break;
		case DisplayServerEnums::SCREEN_PRIMARY: {
			return get_primary_screen();
		} break;
		case DisplayServerEnums::SCREEN_OF_MAIN_WINDOW: {
			return window_get_current_screen(DisplayServerEnums::MAIN_WINDOW_ID);
		} break;
		default: {
			return p_screen;
		} break;
		}
	}

	virtual int get_screen_count() const = 0;
	virtual int get_primary_screen() const = 0;

	virtual int get_keyboard_focus_screen() const { return get_primary_screen(); }

	virtual int get_screen_from_rect(const Rect2& p_rect) const;
	virtual Point2i screen_get_position(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const = 0;
	virtual Size2i screen_get_size(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const = 0;
	virtual Rect2i screen_get_usable_rect(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const = 0;
	virtual int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const = 0;
	virtual float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const;

	virtual float screen_get_max_scale() const
	{
		float max_scale = 1.f;
		int screen_count = get_screen_count();
		for (int i = 0; i < screen_count; i++) {
			max_scale = std::fmax(max_scale, screen_get_scale(i));
		}
		return max_scale;
	}

	virtual float screen_get_refresh_rate(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const = 0;

	virtual Color screen_get_pixel(const Point2i& p_position) const { return Color(); }

	virtual Ref<Image> screen_get_image(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const
	{
		return Ref<Image>();
	}

	virtual Ref<Image> screen_get_image_rect(const Rect2i& p_rect) const { return Ref<Image>(); }

	virtual bool is_touchscreen_available() const;

	virtual void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation,
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	virtual DisplayServerEnums::ScreenOrientation screen_get_orientation(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const;

	virtual void screen_set_keep_on(bool p_enable); // disable screensaver
	virtual bool screen_is_kept_on() const;

	/* WINDOW */

protected:
	static bool _get_window_early_clear_override(Color& r_color);

public:
	virtual Vector<DisplayServerEnums::WindowID> get_window_list() const = 0;

	virtual DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode,
		DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags,
		const Rect2i& p_rect = Rect2i(), bool p_exclusive = false,
		DisplayServerEnums::WindowID p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID);
	virtual void show_window(DisplayServerEnums::WindowID p_id);
	virtual void delete_sub_window(DisplayServerEnums::WindowID p_id);

	virtual DisplayServerEnums::WindowID window_get_active_popup() const
	{
		return DisplayServerEnums::INVALID_WINDOW_ID;
	}

	virtual void window_set_popup_safe_rect(
		DisplayServerEnums::WindowID p_window, const Rect2i& p_rect)
	{
	}

	virtual Rect2i window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window) const
	{
		return Rect2i();
	}

	virtual int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual DisplayServerEnums::WindowID get_window_at_screen_position(
		const Point2i& p_position) const = 0;

	virtual void window_set_title(const String& p_title,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;

	virtual Size2i window_get_title_size(const String& p_title,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const
	{
		return Size2i();
	}

	virtual void window_set_mouse_passthrough(const Vector<Vector2>& p_region,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);

	virtual int window_get_current_screen(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;
	virtual void window_set_current_screen(int p_screen,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;

	virtual Point2i window_get_position(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;
	virtual Point2i window_get_position_with_decorations(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;
	virtual void window_set_position(const Point2i& p_position,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;

	virtual void window_set_transient(
		DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) = 0;
	virtual void window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive);

	virtual void window_set_max_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual Size2i window_get_max_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void window_set_min_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual Size2i window_get_min_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void window_set_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual Size2i window_get_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;
	virtual Size2i window_get_size_with_decorations(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual float window_get_scale(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const
	{
		int screen = window_get_current_screen(p_window);
		return screen_get_scale(screen);
	}

	virtual void window_set_mode(DisplayServerEnums::WindowMode p_mode,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual DisplayServerEnums::WindowMode window_get_mode(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	virtual DisplayServerEnums::VSyncMode window_get_vsync_mode(
		DisplayServerEnums::WindowID p_window) const;

	virtual bool window_is_hdr_output_supported(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual void window_request_hdr_output(const bool p_enable,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	virtual bool window_is_hdr_output_requested(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;
	virtual bool window_is_hdr_output_enabled(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual void window_set_hdr_output_reference_luminance(const float p_reference_luminance,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	virtual float window_get_hdr_output_reference_luminance(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;
	virtual float window_get_hdr_output_current_reference_luminance(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual void window_set_hdr_output_max_luminance(const float p_max_luminance,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	virtual float window_get_hdr_output_max_luminance(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;
	virtual float window_get_hdr_output_current_max_luminance(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual void window_set_icon(const Ref<Image>& p_icon,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual float window_get_output_max_linear_value(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const;

	virtual bool window_is_maximize_allowed(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual bool window_get_flag(DisplayServerEnums::WindowFlags p_flag,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual void window_request_attention(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;

	virtual void window_set_taskbar_progress_value(
		float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual void window_move_to_foreground(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) = 0;
	virtual bool window_is_focused(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual DisplayServerEnums::WindowID get_focused_window() const;

	virtual void window_set_window_buttons_offset(const Vector2i& p_offset,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual Vector3i window_get_safe_title_margins(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const
	{
		return Vector3i();
	}

	virtual bool window_can_draw(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const = 0;

	virtual bool can_any_window_draw() const = 0;

	virtual void window_set_ime_active(const bool p_active,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	virtual void window_set_ime_position(const Point2i& p_pos,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);

	virtual bool window_maximize_on_title_dbl_click() const { return false; }

	virtual bool window_minimize_on_title_dbl_click() const { return false; }

	virtual void window_start_drag(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual void window_set_color(const Color& p_color) {}

	virtual void window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	// necessary for GL focus, may be able to use one of the existing functions for this, not sure
	// yet
	virtual void gl_window_make_current(DisplayServerEnums::WindowID p_window_id);

	virtual bool is_window_transparency_available() const { return false; }

	/* PROCESS */

	virtual void enable_for_stealing_focus(ProcessID pid);

	virtual Error embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid,
		const Rect2i& p_rect, bool p_visible, bool p_grab_focus);
	virtual Error request_close_embedded_process(ProcessID p_pid);
	virtual Error remove_embedded_process(ProcessID p_pid);
	virtual ProcessID get_focused_process_id();

	/* DIALOGS */

	virtual bool get_swap_cancel_ok();

	virtual void show_emoji_and_symbol_picker() const;

	/* STATUS INDICATOR */

	virtual void status_indicator_set_icon(
		DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon);
	virtual void status_indicator_set_tooltip(
		DisplayServerEnums::IndicatorID p_id, const String& p_tooltip);
	virtual void status_indicator_set_menu(
		DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid);
	virtual Rect2 status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const;
	virtual void delete_status_indicator(DisplayServerEnums::IndicatorID p_id);

	/* OUTPUT */


public:
	/* PICTURE_IN_PICTURE */
	virtual bool is_in_pip_mode(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
		return false;
	}

	virtual void pip_mode_enter(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual void pip_mode_set_aspect_ratio(int p_numerator, int p_denominator,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	virtual void pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID)
	{
	}

	/* ACCESSIBILITY */

public:
	virtual int accessibility_should_increase_contrast() const { return -1; }

	virtual int accessibility_should_reduce_animation() const { return -1; }

	virtual int accessibility_should_reduce_transparency() const { return -1; }

	virtual int accessibility_screen_reader_active() const { return -1; }

#ifndef DISABLE_DEPRECATED
	virtual RID accessibility_create_element(
		DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::AccessibilityRole p_role);
	virtual RID accessibility_create_sub_element(const RID& p_parent_rid,
		DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos = -1);
	virtual RID accessibility_create_sub_text_edit_elements(const RID& p_parent_rid,
		const RID& p_shaped_text, float p_min_height, int p_insert_pos = -1,
		bool p_is_last_line = false);
	virtual bool accessibility_has_element(const RID& p_id) const;
	virtual void accessibility_free_element(const RID& p_id);

	virtual void accessibility_update_set_focus(const RID& p_id);
	virtual RID accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id) const;

	virtual void accessibility_set_window_rect(
		DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in);
	virtual void accessibility_set_window_focused(
		DisplayServerEnums::WindowID p_window_id, bool p_focused);
	virtual void accessibility_window_activation_completed(DisplayServerEnums::WindowID
			p_window_id); // Note: internal method used by Window, do not expose.
	virtual void accessibility_window_deactivation_completed(DisplayServerEnums::WindowID
			p_window_id); // Note: internal method used by Window, do not expose.

	virtual void accessibility_update_set_role(
		const RID& p_id, DisplayServerEnums::AccessibilityRole p_role);
	virtual void accessibility_update_set_name(const RID& p_id, const String& p_name);
	virtual void accessibility_update_set_extra_info(
		const RID& p_id, const String& p_name_extra_info);
	virtual void accessibility_update_set_description(const RID& p_id, const String& p_description);
	virtual void accessibility_update_set_value(const RID& p_id, const String& p_value);
	virtual void accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip);
	virtual void accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect);
	virtual void accessibility_update_set_transform(
		const RID& p_id, const Transform2D& p_transform);
	virtual void accessibility_update_add_child(const RID& p_id, const RID& p_child_id);
	virtual void accessibility_update_add_related_controls(
		const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_add_related_details(const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_add_related_described_by(
		const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_add_related_flow_to(const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_add_related_labeled_by(
		const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_add_related_radio_group(
		const RID& p_id, const RID& p_related_id);
	virtual void accessibility_update_set_active_descendant(const RID& p_id, const RID& p_other_id);
	virtual void accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id);
	virtual void accessibility_update_set_previous_on_line(const RID& p_id, const RID& p_other_id);
	virtual void accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id);
	virtual void accessibility_update_set_in_page_link_target(
		const RID& p_id, const RID& p_other_id);
	virtual void accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id);
	virtual void accessibility_update_set_live(
		const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live);
	virtual void accessibility_update_add_custom_action(
		const RID& p_id, int p_action_id, const String& p_action_description);
	virtual void accessibility_update_set_table_row_count(const RID& p_id, int p_count);
	virtual void accessibility_update_set_table_column_count(const RID& p_id, int p_count);
	virtual void accessibility_update_set_table_row_index(const RID& p_id, int p_index);
	virtual void accessibility_update_set_table_column_index(const RID& p_id, int p_index);
	virtual void accessibility_update_set_table_cell_position(
		const RID& p_id, int p_row_index, int p_column_index);
	virtual void accessibility_update_set_table_cell_span(
		const RID& p_id, int p_row_span, int p_column_span);
	virtual void accessibility_update_set_list_item_count(const RID& p_id, int p_size);
	virtual void accessibility_update_set_list_item_index(const RID& p_id, int p_index);
	virtual void accessibility_update_set_list_item_level(const RID& p_id, int p_level);
	virtual void accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected);
	virtual void accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded);
	virtual void accessibility_update_set_popup_type(
		const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup);
	virtual void accessibility_update_set_checked(const RID& p_id, bool p_checekd);
	virtual void accessibility_update_set_num_value(const RID& p_id, double p_position);
	virtual void accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max);
	virtual void accessibility_update_set_num_step(const RID& p_id, double p_step);
	virtual void accessibility_update_set_num_jump(const RID& p_id, double p_jump);
	virtual void accessibility_update_set_scroll_x(const RID& p_id, double p_position);
	virtual void accessibility_update_set_scroll_x_range(
		const RID& p_id, double p_min, double p_max);
	virtual void accessibility_update_set_scroll_y(const RID& p_id, double p_position);
	virtual void accessibility_update_set_scroll_y_range(
		const RID& p_id, double p_min, double p_max);
	virtual void accessibility_update_set_text_decorations(
		const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline);
	virtual void accessibility_update_set_text_align(const RID& p_id, HorizontalAlignment p_align);
	virtual void accessibility_update_set_text_selection(const RID& p_id,
		const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char);
	virtual void accessibility_update_set_flag(
		const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value);
	virtual void accessibility_update_set_classname(const RID& p_id, const String& p_classname);
	virtual void accessibility_update_set_placeholder(const RID& p_id, const String& p_placeholder);
	virtual void accessibility_update_set_language(const RID& p_id, const String& p_language);
	virtual void accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical);
	virtual void accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical);
	virtual void accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut);
	virtual void accessibility_update_set_url(const RID& p_id, const String& p_url);
	virtual void accessibility_update_set_role_description(
		const RID& p_id, const String& p_description);
	virtual void accessibility_update_set_state_description(
		const RID& p_id, const String& p_description);
	virtual void accessibility_update_set_color_value(const RID& p_id, const Color& p_color);
	virtual void accessibility_update_set_background_color(const RID& p_id, const Color& p_color);
	virtual void accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color);
#endif // DISABLE_DEPRECATED
};

class DisplayServer final
{
	friend class DisplayServerDriver;

	enum
	{
		MAX_SERVERS = 64
	};

public:
	typedef DisplayServerDriver* (*CreateFunction)(const String&, DisplayServerEnums::WindowMode,
		DisplayServerEnums::VSyncMode, uint32_t, const Point2i*, const Size2i&, int p_screen,
		DisplayServerEnums::Context, int64_t p_parent_window, Error& r_error);
	typedef Vector<String> (*GetRenderingDriversFunction)();

private:
	struct DisplayServerCreate
	{
		const char* name;
		CreateFunction create_function;
		GetRenderingDriversFunction get_rendering_drivers_function;
	};

	static DisplayServerCreate server_create_functions[MAX_SERVERS];
	static int server_create_count;

	static bool window_early_clear_override_enabled;
	static Color window_early_clear_override_color;

	static void _bind_driver(DisplayServerDriver* p_driver);
	static void _unbind_driver(DisplayServerDriver* p_driver);

	static void _input_set_mouse_mode(InputClassEnums::MouseMode p_mode);
	static InputClassEnums::MouseMode _input_get_mouse_mode();
	static void _input_set_mouse_mode_override(InputClassEnums::MouseMode p_mode);
	static InputClassEnums::MouseMode _input_get_mouse_mode_override();
	static void _input_set_mouse_mode_override_enabled(bool p_enabled);
	static bool _input_is_mouse_mode_override_enabled();
	static void _input_warp(const Vector2& p_to_pos);
	static InputClassEnums::CursorShape _input_get_current_cursor_shape();
	static void _input_set_custom_mouse_cursor_func(
		const Ref<Resource>& p_image, InputClassEnums::CursorShape p_shape, const Vector2& p_hotspot);

#ifndef DISABLE_DEPRECATED
	static RID _accessibility_create_sub_text_edit_elements_bind_compat_113459(
		const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height,
		int p_insert_pos = -1);
#endif

public:
	DisplayServer() = delete;

	static constexpr float SCREEN_REFRESH_RATE_FALLBACK =
		DisplayServerDriver::SCREEN_REFRESH_RATE_FALLBACK;

	struct Data
	{
		DisplayServerDriver* driver = nullptr;
	};

	static inline Data* data = nullptr;

	static void register_create_function(
		const char* p_name, CreateFunction p_function, GetRenderingDriversFunction p_get_drivers);
	static int get_create_function_count();
	static const char* get_create_function_name(int p_index);
	static Vector<String> get_create_function_rendering_drivers(int p_index);
	static DisplayServerDriver* create(int p_index, const String& p_rendering_driver,
		DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
		uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
		DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error);
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->driver != nullptr;
	}
	_FORCE_INLINE_ static DisplayServerDriver* get_driver() { return data ? data->driver : nullptr; }
	template <typename T> _FORCE_INLINE_ static T* driver_as()
	{
		return data ? dynamic_cast<T*>(data->driver) : nullptr;
	}

	static Error init_rendering_device();

	static inline DisplayServerEnums::RenderingDeviceCreationStatus created_rendering_device =
		DisplayServerEnums::RenderingDeviceCreationStatus::UNKNOWN;
	static bool can_create_rendering_device();

	static inline DisplayServerEnums::RenderingDeviceCreationStatus supported_rendering_device =
		DisplayServerEnums::RenderingDeviceCreationStatus::UNKNOWN;
	static bool is_rendering_device_supported();

	static void set_early_window_clear_color_override(
		bool p_enabled, Color p_color = Color(0, 0, 0, 0));

	static String get_name();
	static void set_context(DisplayServerEnums::Context p_context);
	static void set_native_icon(const String& p_filename);
	static void set_icon(const Ref<Image>& p_icon);
	static bool has_feature(DisplayServerEnums::Feature p_feature);
	static void process_events();
	static void force_process_and_drop_events();
	static void release_rendering_thread();
	static void swap_buffers();
	static void beep();
	static bool tts_is_speaking();
	static bool tts_is_paused();
	static PackedStringArray tts_get_voices_for_language(const String& p_language);
	static void tts_speak(const String& p_text, const String& p_voice, int p_volume = 50, float p_pitch = 1.f, float p_rate = 1.f, int64_t p_utterance_id = 0, bool p_interrupt = false);
	static void tts_pause();
	static void tts_resume();
	static void tts_stop();
	static void tts_post_utterance_event(DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos = 0);
	static bool is_dark_mode_supported();
	static bool is_dark_mode();
	static Color get_accent_color();
	static Color get_base_color();
	static void mouse_set_mode(DisplayServerEnums::MouseMode p_mode);
	static DisplayServerEnums::MouseMode mouse_get_mode();
	static void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode);
	static DisplayServerEnums::MouseMode mouse_get_mode_override();
	static void mouse_set_mode_override_enabled(bool p_override_enabled);
	static bool mouse_is_mode_override_enabled();
	static void warp_mouse(const Point2i& p_position);
	static Point2i mouse_get_position();
	static uint32_t mouse_get_button_state();
	static void cursor_set_shape(DisplayServerEnums::CursorShape p_shape);
	static DisplayServerEnums::CursorShape cursor_get_shape();
	static void cursor_set_custom_image(const Ref<Resource>& p_cursor, DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW, const Vector2& p_hotspot = Vector2());
	static Point2i ime_get_selection();
	static String ime_get_text();
	static void virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect = Rect2(), DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT, int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1);
	static void virtual_keyboard_hide();
	static int virtual_keyboard_get_height();
	static bool has_hardware_keyboard();
	static int keyboard_get_layout_count();
	static int keyboard_get_current_layout();
	static void keyboard_set_current_layout(int p_index);
	static String keyboard_get_layout_language(int p_index);
	static String keyboard_get_layout_name(int p_index);
	static Key keyboard_get_keycode_from_physical(Key p_keycode);
	static Key keyboard_get_label_from_physical(Key p_keycode);
	static int tablet_get_driver_count();
	static String tablet_get_driver_name(int p_driver);
	static String tablet_get_current_driver();
	static void tablet_set_current_driver(const String& p_driver);
	static void clipboard_set(const String& p_text);
	static String clipboard_get();
	static Ref<Image> clipboard_get_image();
	static bool clipboard_has();
	static bool clipboard_has_image();
	static void clipboard_set_primary(const String& p_text);
	static String clipboard_get_primary();
	static Rect2i get_display_safe_area(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static int get_screen_count();
	static int get_primary_screen();
	static int get_keyboard_focus_screen();
	static int get_screen_from_rect(const Rect2& p_rect);
	static Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static float screen_get_max_scale();
	static float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static Color screen_get_pixel(const Point2i& p_position);
	static Ref<Image> screen_get_image(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static Ref<Image> screen_get_image_rect(const Rect2i& p_rect);
	static bool is_touchscreen_available();
	static void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static DisplayServerEnums::ScreenOrientation screen_get_orientation(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);
	static void screen_set_keep_on(bool p_enable);
	static bool screen_is_kept_on();
	static Vector<DisplayServerEnums::WindowID> get_window_list();
	static DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect = Rect2i(), bool p_exclusive = false, DisplayServerEnums::WindowID p_transient_parent = DisplayServerEnums::INVALID_WINDOW_ID);
	static void show_window(DisplayServerEnums::WindowID p_id);
	static void delete_sub_window(DisplayServerEnums::WindowID p_id);
	static DisplayServerEnums::WindowID window_get_active_popup();
	static void window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i& p_rect);
	static Rect2i window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window);
	static int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static DisplayServerEnums::WindowID get_window_at_screen_position(const Point2i& p_position);
	static void window_set_title(const String& p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Size2i window_get_title_size(const String& p_title, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_mouse_passthrough(const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Point2i window_get_position(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Point2i window_get_position_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_position(const Point2i& p_position, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent);
	static void window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive);
	static void window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Size2i window_get_max_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Size2i window_get_min_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_scale(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static DisplayServerEnums::WindowMode window_get_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_window);
	static bool window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_icon(const Ref<Image>& p_icon, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_is_maximize_allowed(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_request_attention(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_move_to_foreground(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_is_focused(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static DisplayServerEnums::WindowID get_focused_window();
	static void window_set_window_buttons_offset(const Vector2i& p_offset, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static Vector3i window_get_safe_title_margins(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool can_any_window_draw();
	static void window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_ime_position(const Point2i& p_pos, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool window_maximize_on_title_dbl_click();
	static bool window_minimize_on_title_dbl_click();
	static void window_start_drag(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void window_set_color(const Color& p_color);
	static void window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void gl_window_make_current(DisplayServerEnums::WindowID p_window_id);
	static bool is_window_transparency_available();
	static void enable_for_stealing_focus(ProcessID pid);
	static Error embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid, const Rect2i& p_rect, bool p_visible, bool p_grab_focus);
	static Error request_close_embedded_process(ProcessID p_pid);
	static Error remove_embedded_process(ProcessID p_pid);
	static ProcessID get_focused_process_id();
	static bool get_swap_cancel_ok();
	static void show_emoji_and_symbol_picker();
	static void status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon);
	static void status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String& p_tooltip);
	static void status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid);
	static Rect2 status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id);
	static void delete_status_indicator(DisplayServerEnums::IndicatorID p_id);
	static bool is_in_pip_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void pip_mode_enter(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static void pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID);
	static int accessibility_should_increase_contrast();
	static int accessibility_should_reduce_animation();
	static int accessibility_should_reduce_transparency();
	static int accessibility_screen_reader_active();

#ifndef DISABLE_DEPRECATED
	static RID accessibility_create_element(DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::AccessibilityRole p_role);
	static RID accessibility_create_sub_element(const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos = -1);
	static RID accessibility_create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height, int p_insert_pos = -1, bool p_is_last_line = false);
	static bool accessibility_has_element(const RID& p_id);
	static void accessibility_free_element(const RID& p_id);
	static void accessibility_update_set_focus(const RID& p_id);
	static RID accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id);
	static void accessibility_set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in);
	static void accessibility_set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused);
	static void accessibility_window_activation_completed(DisplayServerEnums::WindowID p_window_id);
	static void accessibility_window_deactivation_completed(DisplayServerEnums::WindowID p_window_id);
	static void accessibility_update_set_role(const RID& p_id, DisplayServerEnums::AccessibilityRole p_role);
	static void accessibility_update_set_name(const RID& p_id, const String& p_name);
	static void accessibility_update_set_extra_info(const RID& p_id, const String& p_name_extra_info);
	static void accessibility_update_set_description(const RID& p_id, const String& p_description);
	static void accessibility_update_set_value(const RID& p_id, const String& p_value);
	static void accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip);
	static void accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect);
	static void accessibility_update_set_transform(const RID& p_id, const Transform2D& p_transform);
	static void accessibility_update_add_child(const RID& p_id, const RID& p_child_id);
	static void accessibility_update_add_related_controls(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_add_related_details(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_add_related_described_by(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_add_related_flow_to(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_add_related_labeled_by(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_add_related_radio_group(const RID& p_id, const RID& p_related_id);
	static void accessibility_update_set_active_descendant(const RID& p_id, const RID& p_other_id);
	static void accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id);
	static void accessibility_update_set_previous_on_line(const RID& p_id, const RID& p_other_id);
	static void accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id);
	static void accessibility_update_set_in_page_link_target(const RID& p_id, const RID& p_other_id);
	static void accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id);
	static void accessibility_update_set_live(const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live);
	static void accessibility_update_add_custom_action(const RID& p_id, int p_action_id, const String& p_action_description);
	static void accessibility_update_set_table_row_count(const RID& p_id, int p_count);
	static void accessibility_update_set_table_column_count(const RID& p_id, int p_count);
	static void accessibility_update_set_table_row_index(const RID& p_id, int p_index);
	static void accessibility_update_set_table_column_index(const RID& p_id, int p_index);
	static void accessibility_update_set_table_cell_position(const RID& p_id, int p_row_index, int p_column_index);
	static void accessibility_update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span);
	static void accessibility_update_set_list_item_count(const RID& p_id, int p_size);
	static void accessibility_update_set_list_item_index(const RID& p_id, int p_index);
	static void accessibility_update_set_list_item_level(const RID& p_id, int p_level);
	static void accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected);
	static void accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded);
	static void accessibility_update_set_popup_type(const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup);
	static void accessibility_update_set_checked(const RID& p_id, bool p_checekd);
	static void accessibility_update_set_num_value(const RID& p_id, double p_position);
	static void accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max);
	static void accessibility_update_set_num_step(const RID& p_id, double p_step);
	static void accessibility_update_set_num_jump(const RID& p_id, double p_jump);
	static void accessibility_update_set_scroll_x(const RID& p_id, double p_position);
	static void accessibility_update_set_scroll_x_range(const RID& p_id, double p_min, double p_max);
	static void accessibility_update_set_scroll_y(const RID& p_id, double p_position);
	static void accessibility_update_set_scroll_y_range(const RID& p_id, double p_min, double p_max);
	static void accessibility_update_set_text_decorations(const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline);
	static void accessibility_update_set_text_align(const RID& p_id, HorizontalAlignment p_align);
	static void accessibility_update_set_text_selection(const RID& p_id, const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char);
	static void accessibility_update_set_flag(const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value);
	static void accessibility_update_set_classname(const RID& p_id, const String& p_classname);
	static void accessibility_update_set_placeholder(const RID& p_id, const String& p_placeholder);
	static void accessibility_update_set_language(const RID& p_id, const String& p_language);
	static void accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical);
	static void accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical);
	static void accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut);
	static void accessibility_update_set_url(const RID& p_id, const String& p_url);
	static void accessibility_update_set_role_description(const RID& p_id, const String& p_description);
	static void accessibility_update_set_state_description(const RID& p_id, const String& p_description);
	static void accessibility_update_set_color_value(const RID& p_id, const Color& p_color);
	static void accessibility_update_set_background_color(const RID& p_id, const Color& p_color);
	static void accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color);
#endif // DISABLE_DEPRECATED
};

// --- Folded from servers/display/display_server_headless.h ---


struct InputEvent;

class DisplayServerHeadless : public DisplayServerDriver
{
private:
	friend class DisplayServer;

	static Vector<String> get_rendering_drivers_func()
	{
		Vector<String> drivers;
		drivers.push_back("dummy");
		return drivers;
	}

	static DisplayServerDriver* create_func(const String& p_rendering_driver,
		DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
		uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
		DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error);

	static void _dispatch_input_events(const InputEvent& p_event);
	void _dispatch_input_event(const InputEvent& p_event);

public:
	bool has_feature(DisplayServerEnums::Feature p_feature) const override { return false; }

	String get_name() const override { return "headless"; }

	// Stub implementations to prevent warnings from being printed for methods
	// that don't affect the project's behavior in headless mode.

	int get_screen_count() const override { return 0; }

	int get_primary_screen() const override { return 0; }

	Point2i screen_get_position(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return Point2i();
	}

	Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return Size2i();
	}

	Rect2i screen_get_usable_rect(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return Rect2i();
	}

	int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return 96; /* 0 might cause issues */
	}

	float screen_get_scale(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return 1;
	}

	float screen_get_max_scale() const override { return 1; }

	float screen_get_refresh_rate(
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override
	{
		return SCREEN_REFRESH_RATE_FALLBACK;
	}

	void screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation,
		int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) override
	{
	}

	void screen_set_keep_on(bool p_enable) override {}

	Vector<DisplayServerEnums::WindowID> get_window_list() const override
	{
		return Vector<DisplayServerEnums::WindowID>();
	}

	DisplayServerEnums::WindowID create_sub_window(DisplayServerEnums::WindowMode p_mode,
		DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags,
		const Rect2i& p_rect = Rect2i(), bool p_exclusive = false,
		DisplayServerEnums::WindowID p_transient_parent =
			DisplayServerEnums::INVALID_WINDOW_ID) override
	{
		return 0;
	}

	void show_window(DisplayServerEnums::WindowID p_id) override {}

	void delete_sub_window(DisplayServerEnums::WindowID p_id) override {}

	DisplayServerEnums::WindowID get_window_at_screen_position(
		const Point2i& p_position) const override
	{
		return 0;
	}

	void window_set_title(const String& p_title,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_set_mouse_passthrough(const Vector<Vector2>& p_region,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	int window_get_current_screen(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return DisplayServerEnums::INVALID_SCREEN;
	}

	void window_set_current_screen(int p_screen,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	Point2i window_get_position(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Point2i();
	}

	Point2i window_get_position_with_decorations(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Point2i();
	}

	void window_set_position(const Point2i& p_position,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_set_transient(
		DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent) override
	{
	}

	void window_set_max_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	Size2i window_get_max_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Size2i();
	}

	void window_set_min_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	Size2i window_get_min_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Size2i();
	}

	void window_set_size(const Size2i p_size,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	Size2i window_get_size(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Size2i();
	}

	Size2i window_get_size_with_decorations(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return Size2i();
	}

	void window_set_mode(DisplayServerEnums::WindowMode p_mode,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	DisplayServerEnums::WindowMode window_get_mode(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return DisplayServerEnums::WINDOW_MODE_MINIMIZED;
	}

	void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	DisplayServerEnums::VSyncMode window_get_vsync_mode(
		DisplayServerEnums::WindowID p_window) const override
	{
		return DisplayServerEnums::VSyncMode::VSYNC_ENABLED;
	}

	bool window_is_maximize_allowed(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return false;
	}

	void window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	bool window_get_flag(DisplayServerEnums::WindowFlags p_flag,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return false;
	}

	void window_request_attention(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_set_taskbar_progress_value(float p_value,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_move_to_foreground(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	bool window_is_focused(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return true;
	}

	bool window_can_draw(
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return false;
	}

	bool can_any_window_draw() const override { return false; }

	void window_set_ime_active(const bool p_active,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	void window_set_ime_position(const Point2i& p_pos,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override
	{
	}

	int64_t window_get_native_handle(DisplayServerEnums::HandleType p_handle_type,
		DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override
	{
		return 0;
	}

	void process_events() override;

	void set_native_icon(const String& p_filename) override {}

	void set_icon(const Ref<Image>& p_icon) override {}

	bool tts_is_speaking() const override { return false; }

	bool tts_is_paused() const override { return false; }

	void tts_speak(const String& p_text, const String& p_voice, int p_volume = 50,
		float p_pitch = 1.0f, float p_rate = 1.0f, int64_t p_utterance_id = 0,
		bool p_interrupt = false) override
	{
	}

	void tts_pause() override {}

	void tts_resume() override {}

	void tts_stop() override {}

	void mouse_set_mode(DisplayServerEnums::MouseMode p_mode) override {}

	void mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode) override {}

	void mouse_set_mode_override_enabled(bool p_override_enabled) override {}

	Point2i mouse_get_position() const override { return Point2i(); }

	void clipboard_set(const String& p_text) override {}

	void clipboard_set_primary(const String& p_text) override {}

	void virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect = Rect2(),
		DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT,
		int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1) override
	{
	}

	void virtual_keyboard_hide() override {}

	void cursor_set_shape(DisplayServerEnums::CursorShape p_shape) override {}

	void cursor_set_custom_image(const Ref<Resource>& p_cursor,
		DisplayServerEnums::CursorShape p_shape = DisplayServerEnums::CURSOR_ARROW,
		const Vector2& p_hotspot = Vector2()) override
	{
	}

	void release_rendering_thread() override {}

	void swap_buffers() override {}

	void status_indicator_set_icon(
		DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon) override
	{
	}

	void status_indicator_set_tooltip(
		DisplayServerEnums::IndicatorID p_id, const String& p_tooltip) override
	{
	}

	void delete_status_indicator(DisplayServerEnums::IndicatorID p_id) override {}

	DisplayServerHeadless();
	~DisplayServerHeadless();
};



// --- End servers/display/display_server_headless.h ---

// --- Folded from servers/display/native_menu.h ---
#include "core/os/os.h"
#include "core/io/ip.h"

class Texture2D;

namespace NativeMenuEnums
{
enum Feature
{
	FEATURE_GLOBAL_MENU,
	FEATURE_POPUP_MENU,
	FEATURE_OPEN_CLOSE_CALLBACK,
	FEATURE_HOVER_CALLBACK,
	FEATURE_KEY_CALLBACK,
};

enum SystemMenus
{
	INVALID_MENU_ID,
	MAIN_MENU_ID,
	APPLICATION_MENU_ID,
	WINDOW_MENU_ID,
	HELP_MENU_ID,
	DOCK_MENU_ID,
};
} // namespace NativeMenuEnums

class NativeMenuDriver
{
public:
	using Feature = NativeMenuEnums::Feature;
	using SystemMenus = NativeMenuEnums::SystemMenus;

	static constexpr Feature FEATURE_GLOBAL_MENU = NativeMenuEnums::FEATURE_GLOBAL_MENU;
	static constexpr Feature FEATURE_POPUP_MENU = NativeMenuEnums::FEATURE_POPUP_MENU;
	static constexpr Feature FEATURE_OPEN_CLOSE_CALLBACK =
		NativeMenuEnums::FEATURE_OPEN_CLOSE_CALLBACK;
	static constexpr Feature FEATURE_HOVER_CALLBACK = NativeMenuEnums::FEATURE_HOVER_CALLBACK;
	static constexpr Feature FEATURE_KEY_CALLBACK = NativeMenuEnums::FEATURE_KEY_CALLBACK;

	static constexpr SystemMenus INVALID_MENU_ID = NativeMenuEnums::INVALID_MENU_ID;
	static constexpr SystemMenus MAIN_MENU_ID = NativeMenuEnums::MAIN_MENU_ID;
	static constexpr SystemMenus APPLICATION_MENU_ID = NativeMenuEnums::APPLICATION_MENU_ID;
	static constexpr SystemMenus WINDOW_MENU_ID = NativeMenuEnums::WINDOW_MENU_ID;
	static constexpr SystemMenus HELP_MENU_ID = NativeMenuEnums::HELP_MENU_ID;
	static constexpr SystemMenus DOCK_MENU_ID = NativeMenuEnums::DOCK_MENU_ID;

	virtual bool has_feature(Feature p_feature) const;

	virtual bool has_system_menu(SystemMenus p_menu_id) const;
	virtual RID get_system_menu(SystemMenus p_menu_id) const;
	virtual String get_system_menu_name(SystemMenus p_menu_id) const;

	virtual String get_system_menu_text(SystemMenus p_menu_id) const;
	virtual void set_system_menu_text(SystemMenus p_menu_id, const String& p_name);

	virtual RID create_menu();
	virtual bool has_menu(const RID& p_rid) const;
	virtual void free_menu(const RID& p_rid);

	virtual Size2 get_size(const RID& p_rid) const;
	virtual void popup(const RID& p_rid, const Vector2i& p_position);

	virtual void set_interface_direction(const RID& p_rid, bool p_is_rtl);

	virtual void set_minimum_width(const RID& p_rid, float p_width);
	virtual float get_minimum_width(const RID& p_rid) const;

	virtual bool is_opened(const RID& p_rid) const;

	virtual int add_separator(const RID& p_rid, int p_index = -1);

	virtual int find_item_index_with_text(const RID& p_rid, const String& p_text) const;
	virtual int find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid) const;

	virtual bool is_item_checked(const RID& p_rid, int p_idx) const;
	virtual bool is_item_indeterminate(const RID& p_rid, int p_idx) const;
	virtual bool is_item_checkable(const RID& p_rid, int p_idx) const;
	virtual bool is_item_radio_checkable(const RID& p_rid, int p_idx) const;
	virtual String get_item_text(const RID& p_rid, int p_idx) const;
	virtual RID get_item_submenu(const RID& p_rid, int p_idx) const;
	virtual Key get_item_accelerator(const RID& p_rid, int p_idx) const;
	virtual bool is_item_disabled(const RID& p_rid, int p_idx) const;
	virtual bool is_item_hidden(const RID& p_rid, int p_idx) const;
	virtual String get_item_tooltip(const RID& p_rid, int p_idx) const;
	virtual int get_item_state(const RID& p_rid, int p_idx) const;
	virtual int get_item_max_states(const RID& p_rid, int p_idx) const;
	virtual Ref<Texture2D> get_item_icon(const RID& p_rid, int p_idx) const;
	virtual int get_item_indentation_level(const RID& p_rid, int p_idx) const;

	virtual void set_item_checked(const RID& p_rid, int p_idx, bool p_checked);
	virtual void set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate);
	virtual void set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	virtual void set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	virtual void set_item_text(const RID& p_rid, int p_idx, const String& p_text);
	virtual void set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid);
	virtual void set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode);
	virtual void set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled);
	virtual void set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden);
	virtual void set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip);
	virtual void set_item_state(const RID& p_rid, int p_idx, int p_state);
	virtual void set_item_max_states(const RID& p_rid, int p_idx, int p_max_states);
	virtual void set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon);
	virtual void set_item_indentation_level(const RID& p_rid, int p_idx, int p_level);
	virtual int set_item_index(const RID& p_rid, int p_idx, int p_target_idx);

	virtual int get_item_count(const RID& p_rid) const;
	virtual bool is_system_menu(const RID& p_rid) const;

	virtual void remove_item(const RID& p_rid, int p_idx);
	virtual void clear(const RID& p_rid);

	NativeMenuDriver();
	virtual ~NativeMenuDriver();
};

class NativeMenu final
{
	friend class NativeMenuDriver;

	static void _bind_driver(NativeMenuDriver* p_driver);
	static void _unbind_driver(NativeMenuDriver* p_driver);

public:
	NativeMenu() = delete;

	using Feature = NativeMenuEnums::Feature;
	using SystemMenus = NativeMenuEnums::SystemMenus;

	static constexpr Feature FEATURE_GLOBAL_MENU = NativeMenuEnums::FEATURE_GLOBAL_MENU;
	static constexpr Feature FEATURE_POPUP_MENU = NativeMenuEnums::FEATURE_POPUP_MENU;
	static constexpr Feature FEATURE_OPEN_CLOSE_CALLBACK =
		NativeMenuEnums::FEATURE_OPEN_CLOSE_CALLBACK;
	static constexpr Feature FEATURE_HOVER_CALLBACK = NativeMenuEnums::FEATURE_HOVER_CALLBACK;
	static constexpr Feature FEATURE_KEY_CALLBACK = NativeMenuEnums::FEATURE_KEY_CALLBACK;

	static constexpr SystemMenus INVALID_MENU_ID = NativeMenuEnums::INVALID_MENU_ID;
	static constexpr SystemMenus MAIN_MENU_ID = NativeMenuEnums::MAIN_MENU_ID;
	static constexpr SystemMenus APPLICATION_MENU_ID = NativeMenuEnums::APPLICATION_MENU_ID;
	static constexpr SystemMenus WINDOW_MENU_ID = NativeMenuEnums::WINDOW_MENU_ID;
	static constexpr SystemMenus HELP_MENU_ID = NativeMenuEnums::HELP_MENU_ID;
	static constexpr SystemMenus DOCK_MENU_ID = NativeMenuEnums::DOCK_MENU_ID;

	struct Data
	{
		NativeMenuDriver* driver = nullptr;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->driver != nullptr;
	}
	_FORCE_INLINE_ static NativeMenuDriver* get_driver() { return data ? data->driver : nullptr; }
	template <typename T> _FORCE_INLINE_ static T* driver_as()
	{
		return data ? dynamic_cast<T*>(data->driver) : nullptr;
	}

	static bool has_feature(Feature p_feature);

	static bool has_system_menu(SystemMenus p_menu_id);
	static RID get_system_menu(SystemMenus p_menu_id);
	static String get_system_menu_name(SystemMenus p_menu_id);

	static String get_system_menu_text(SystemMenus p_menu_id);
	static void set_system_menu_text(SystemMenus p_menu_id, const String& p_name);

	static RID create_menu();
	static bool has_menu(const RID& p_rid);
	static void free_menu(const RID& p_rid);

	static Size2 get_size(const RID& p_rid);
	static void popup(const RID& p_rid, const Vector2i& p_position);

	static void set_interface_direction(const RID& p_rid, bool p_is_rtl);

	static void set_minimum_width(const RID& p_rid, float p_width);
	static float get_minimum_width(const RID& p_rid);

	static bool is_opened(const RID& p_rid);

	static int add_separator(const RID& p_rid, int p_index = -1);

	static int find_item_index_with_text(const RID& p_rid, const String& p_text);
	static int find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid);

	static bool is_item_checked(const RID& p_rid, int p_idx);
	static bool is_item_indeterminate(const RID& p_rid, int p_idx);
	static bool is_item_checkable(const RID& p_rid, int p_idx);
	static bool is_item_radio_checkable(const RID& p_rid, int p_idx);
	static String get_item_text(const RID& p_rid, int p_idx);
	static RID get_item_submenu(const RID& p_rid, int p_idx);
	static Key get_item_accelerator(const RID& p_rid, int p_idx);
	static bool is_item_disabled(const RID& p_rid, int p_idx);
	static bool is_item_hidden(const RID& p_rid, int p_idx);
	static String get_item_tooltip(const RID& p_rid, int p_idx);
	static int get_item_state(const RID& p_rid, int p_idx);
	static int get_item_max_states(const RID& p_rid, int p_idx);
	static Ref<Texture2D> get_item_icon(const RID& p_rid, int p_idx);
	static int get_item_indentation_level(const RID& p_rid, int p_idx);

	static void set_item_checked(const RID& p_rid, int p_idx, bool p_checked);
	static void set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate);
	static void set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	static void set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable);
	static void set_item_text(const RID& p_rid, int p_idx, const String& p_text);
	static void set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid);
	static void set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode);
	static void set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled);
	static void set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden);
	static void set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip);
	static void set_item_state(const RID& p_rid, int p_idx, int p_state);
	static void set_item_max_states(const RID& p_rid, int p_idx, int p_max_states);
	static void set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon);
	static void set_item_indentation_level(const RID& p_rid, int p_idx, int p_level);
	static int set_item_index(const RID& p_rid, int p_idx, int p_target_idx);

	static int get_item_count(const RID& p_rid);
	static bool is_system_menu(const RID& p_rid);

	static void remove_item(const RID& p_rid, int p_idx);
	static void clear(const RID& p_rid);
};
// --- End servers/display/native_menu.h ---

// --- Folded from servers/display/accessibility_server.h ---
#include "core/io/ip.h"
// --- Folded from servers/display/accessibility_server_enums.h ---
namespace AccessibilityServerEnums {

enum AccessibilityMode {
	ACCESSIBILITY_AUTO,
	ACCESSIBILITY_ALWAYS,
	ACCESSIBILITY_DISABLED,
};

enum AccessibilityRole {
	ROLE_UNKNOWN,
	ROLE_DEFAULT_BUTTON,
	ROLE_AUDIO,
	ROLE_VIDEO,
	ROLE_STATIC_TEXT,
	ROLE_CONTAINER,
	ROLE_PANEL,
	ROLE_BUTTON,
	ROLE_LINK,
	ROLE_CHECK_BOX,
	ROLE_RADIO_BUTTON,
	ROLE_CHECK_BUTTON,
	ROLE_SCROLL_BAR,
	ROLE_SCROLL_VIEW,
	ROLE_SPLITTER,
	ROLE_SLIDER,
	ROLE_SPIN_BUTTON,
	ROLE_PROGRESS_INDICATOR,
	ROLE_TEXT_FIELD,
	ROLE_MULTILINE_TEXT_FIELD,
	ROLE_COLOR_PICKER,
	ROLE_TABLE,
	ROLE_CELL,
	ROLE_ROW,
	ROLE_ROW_GROUP,
	ROLE_ROW_HEADER,
	ROLE_COLUMN_HEADER,
	ROLE_TREE,
	ROLE_TREE_ITEM,
	ROLE_LIST,
	ROLE_LIST_ITEM,
	ROLE_LIST_BOX,
	ROLE_LIST_BOX_OPTION,
	ROLE_TAB_BAR,
	ROLE_TAB,
	ROLE_TAB_PANEL,
	ROLE_MENU_BAR,
	ROLE_MENU,
	ROLE_MENU_ITEM,
	ROLE_MENU_ITEM_CHECK_BOX,
	ROLE_MENU_ITEM_RADIO,
	ROLE_IMAGE,
	ROLE_WINDOW,
	ROLE_TITLE_BAR,
	ROLE_DIALOG,
	ROLE_TOOLTIP,
	ROLE_REGION,
	ROLE_TEXT_RUN,
};

enum AccessibilityPopupType {
	POPUP_MENU,
	POPUP_LIST,
	POPUP_TREE,
	POPUP_DIALOG,
};

enum AccessibilityFlags {
	FLAG_HIDDEN,
	FLAG_MULTISELECTABLE,
	FLAG_REQUIRED,
	FLAG_VISITED,
	FLAG_BUSY,
	FLAG_MODAL,
	FLAG_TOUCH_PASSTHROUGH,
	FLAG_READONLY,
	FLAG_DISABLED,
	FLAG_CLIPS_CHILDREN,
};

enum AccessibilityAction {
	ACTION_CLICK,
	ACTION_FOCUS,
	ACTION_BLUR,
	ACTION_COLLAPSE,
	ACTION_EXPAND,
	ACTION_DECREMENT,
	ACTION_INCREMENT,
	ACTION_HIDE_TOOLTIP,
	ACTION_SHOW_TOOLTIP,
	ACTION_SET_TEXT_SELECTION,
	ACTION_REPLACE_SELECTED_TEXT,
	ACTION_SCROLL_BACKWARD,
	ACTION_SCROLL_DOWN,
	ACTION_SCROLL_FORWARD,
	ACTION_SCROLL_LEFT,
	ACTION_SCROLL_RIGHT,
	ACTION_SCROLL_UP,
	ACTION_SCROLL_INTO_VIEW,
	ACTION_SCROLL_TO_POINT,
	ACTION_SET_SCROLL_OFFSET,
	ACTION_SET_VALUE,
	ACTION_SHOW_CONTEXT_MENU,
	ACTION_CUSTOM,
};

enum AccessibilityLiveMode {
	LIVE_OFF,
	LIVE_POLITE,
	LIVE_ASSERTIVE,
};

enum AccessibilityScrollUnit {
	SCROLL_UNIT_ITEM,
	SCROLL_UNIT_PAGE,
};

enum AccessibilityScrollHint {
	SCROLL_HINT_TOP_LEFT,
	SCROLL_HINT_BOTTOM_RIGHT,
	SCROLL_HINT_TOP_EDGE,
	SCROLL_HINT_BOTTOM_EDGE,
	SCROLL_HINT_LEFT_EDGE,
	SCROLL_HINT_RIGHT_EDGE,
};

}; // namespace AccessibilityServerEnums

// --- End servers/display/accessibility_server_enums.h ---


class AccessibilityServerDriver
{
public:
	virtual bool is_supported() const { return false; }

	virtual bool window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle) = 0;
	virtual void window_destroy(DisplayServerEnums::WindowID p_window_id) = 0;

	virtual RID create_element(DisplayServerEnums::WindowID p_window_id,
		AccessibilityServerEnums::AccessibilityRole p_role) = 0;
	virtual RID create_sub_element(const RID& p_parent_rid,
		AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos = -1) = 0;
	virtual RID create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text,
		float p_min_height, int p_insert_pos = -1, bool p_is_last_line = false) = 0;
	virtual bool has_element(const RID& p_id) const = 0;
	virtual void free_element(const RID& p_id) = 0;

	virtual RID get_window_root(DisplayServerEnums::WindowID p_window_id) const = 0;
	virtual void update_set_focus(const RID& p_id) = 0;

	virtual void set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out,
		const Rect2& p_rect_in) = 0;
	virtual void set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused) = 0;
	virtual void window_activation_completed(DisplayServerEnums::WindowID p_window_id) = 0;
	virtual void window_deactivation_completed(DisplayServerEnums::WindowID p_window_id) = 0;

	virtual void update_set_role(
		const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role) = 0;
	virtual void update_set_name(const RID& p_id, const String& p_name) = 0;
	virtual void update_set_braille_label(const RID& p_id, const String& p_name) = 0;
	virtual void update_set_braille_role_description(
		const RID& p_id, const String& p_description) = 0;
	virtual void update_set_extra_info(const RID& p_id, const String& p_name_extra_info) = 0;
	virtual void update_set_description(const RID& p_id, const String& p_description) = 0;
	virtual void update_set_value(const RID& p_id, const String& p_value) = 0;
	virtual void update_set_tooltip(const RID& p_id, const String& p_tooltip) = 0;
	virtual void update_set_bounds(const RID& p_id, const Rect2& p_rect) = 0;
	virtual void update_set_transform(const RID& p_id, const Transform2D& p_transform) = 0;
	virtual void update_add_child(const RID& p_id, const RID& p_child_id) = 0;
	virtual void update_add_related_controls(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_add_related_details(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_add_related_described_by(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_add_related_flow_to(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_add_related_labeled_by(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_add_related_radio_group(const RID& p_id, const RID& p_related_id) = 0;
	virtual void update_set_active_descendant(const RID& p_id, const RID& p_other_id) = 0;
	virtual void update_set_next_on_line(const RID& p_id, const RID& p_other_id) = 0;
	virtual void update_set_previous_on_line(const RID& p_id, const RID& p_other_id) = 0;
	virtual void update_set_member_of(const RID& p_id, const RID& p_group_id) = 0;
	virtual void update_set_in_page_link_target(const RID& p_id, const RID& p_other_id) = 0;
	virtual void update_set_error_message(const RID& p_id, const RID& p_other_id) = 0;
	virtual void update_set_live(
		const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live) = 0;
	virtual void update_add_custom_action(
		const RID& p_id, int p_action_id, const String& p_action_description) = 0;
	virtual void update_set_table_row_count(const RID& p_id, int p_count) = 0;
	virtual void update_set_table_column_count(const RID& p_id, int p_count) = 0;
	virtual void update_set_table_row_index(const RID& p_id, int p_index) = 0;
	virtual void update_set_table_column_index(const RID& p_id, int p_index) = 0;
	virtual void update_set_table_cell_position(
		const RID& p_id, int p_row_index, int p_column_index) = 0;
	virtual void update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span) = 0;
	virtual void update_set_list_item_count(const RID& p_id, int p_size) = 0;
	virtual void update_set_list_item_index(const RID& p_id, int p_index) = 0;
	virtual void update_set_list_item_level(const RID& p_id, int p_level) = 0;
	virtual void update_set_list_item_selected(const RID& p_id, bool p_selected) = 0;
	virtual void update_set_list_item_expanded(const RID& p_id, bool p_expanded) = 0;
	virtual void update_set_popup_type(
		const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup) = 0;
	virtual void update_set_checked(const RID& p_id, bool p_checekd) = 0;
	virtual void update_set_num_value(const RID& p_id, double p_position) = 0;
	virtual void update_set_num_range(const RID& p_id, double p_min, double p_max) = 0;
	virtual void update_set_num_step(const RID& p_id, double p_step) = 0;
	virtual void update_set_num_jump(const RID& p_id, double p_jump) = 0;
	virtual void update_set_scroll_x(const RID& p_id, double p_position) = 0;
	virtual void update_set_scroll_x_range(const RID& p_id, double p_min, double p_max) = 0;
	virtual void update_set_scroll_y(const RID& p_id, double p_position) = 0;
	virtual void update_set_scroll_y_range(const RID& p_id, double p_min, double p_max) = 0;
	virtual void update_set_text_decorations(const RID& p_id, bool p_underline,
		bool p_strikethrough, bool p_overline, const Color& p_color) = 0;
	virtual void update_set_text_align(const RID& p_id, HorizontalAlignment p_align) = 0;
	virtual void update_set_text_selection(const RID& p_id, const RID& p_text_start_id,
		int p_start_char, const RID& p_text_end_id, int p_end_char) = 0;
	virtual void update_set_flag(
		const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value) = 0;
	virtual void update_set_classname(const RID& p_id, const String& p_classname) = 0;
	virtual void update_set_placeholder(const RID& p_id, const String& p_placeholder) = 0;
	virtual void update_set_language(const RID& p_id, const String& p_language) = 0;
	virtual void update_set_text_orientation(const RID& p_id, bool p_vertical) = 0;
	virtual void update_set_list_orientation(const RID& p_id, bool p_vertical) = 0;
	virtual void update_set_shortcut(const RID& p_id, const String& p_shortcut) = 0;
	virtual void update_set_url(const RID& p_id, const String& p_url) = 0;
	virtual void update_set_role_description(const RID& p_id, const String& p_description) = 0;
	virtual void update_set_state_description(const RID& p_id, const String& p_description) = 0;
	virtual void update_set_color_value(const RID& p_id, const Color& p_color) = 0;
	virtual void update_set_background_color(const RID& p_id, const Color& p_color) = 0;
	virtual void update_set_foreground_color(const RID& p_id, const Color& p_color) = 0;

	AccessibilityServerDriver();
	virtual ~AccessibilityServerDriver();
};

class AccessibilityServer final
{
	friend class AccessibilityServerDriver;

	static void _bind_driver(AccessibilityServerDriver* p_driver);
	static void _unbind_driver(AccessibilityServerDriver* p_driver);

public:
	AccessibilityServer() = delete;

	typedef AccessibilityServerDriver* (*CreateFunction)(Error& r_error);

	struct Data
	{
		AccessibilityServerDriver* driver = nullptr;
	};

	static inline Data* data = nullptr;

private:
	static inline AccessibilityServerEnums::AccessibilityMode accessibility_mode =
		AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_AUTO;

	enum
	{
		MAX_SERVERS = 64
	};

	struct AccessibilityServerCreate
	{
		const char* name;
		CreateFunction create_function;
	};

	static AccessibilityServerCreate server_create_functions[MAX_SERVERS];
	static int server_create_count;

public:
	static void register_create_function(const char* p_name, CreateFunction p_function);
	static int get_create_function_count();
	static const char* get_create_function_name(int p_index);
	static AccessibilityServerDriver* create(int p_index, Error& r_error);
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->driver != nullptr;
	}
	_FORCE_INLINE_ static AccessibilityServerDriver* get_driver()
	{
		return data ? data->driver : nullptr;
	}
	template <typename T> _FORCE_INLINE_ static T* driver_as()
	{
		return data ? dynamic_cast<T*>(data->driver) : nullptr;
	}

	_FORCE_INLINE_ static AccessibilityServerEnums::AccessibilityMode get_mode()
	{
		return accessibility_mode;
	}

	static void set_mode(AccessibilityServerEnums::AccessibilityMode p_mode)
	{
		accessibility_mode = p_mode;
	}

	static bool is_supported();

	static bool window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle);
	static void window_destroy(DisplayServerEnums::WindowID p_window_id);

	static RID create_element(
		DisplayServerEnums::WindowID p_window_id, AccessibilityServerEnums::AccessibilityRole p_role);
	static RID create_sub_element(const RID& p_parent_rid,
		AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos = -1);
	static RID create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text,
		float p_min_height, int p_insert_pos = -1, bool p_is_last_line = false);
	static bool has_element(const RID& p_id);
	static void free_element(const RID& p_id);

	static RID get_window_root(DisplayServerEnums::WindowID p_window_id);
	static void update_set_focus(const RID& p_id);

	static void set_window_rect(
		DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in);
	static void set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused);
	static void window_activation_completed(DisplayServerEnums::WindowID p_window_id);
	static void window_deactivation_completed(DisplayServerEnums::WindowID p_window_id);

	static void update_set_role(
		const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role);
	static void update_set_name(const RID& p_id, const String& p_name);
	static void update_set_braille_label(const RID& p_id, const String& p_name);
	static void update_set_braille_role_description(const RID& p_id, const String& p_description);
	static void update_set_extra_info(const RID& p_id, const String& p_name_extra_info);
	static void update_set_description(const RID& p_id, const String& p_description);
	static void update_set_value(const RID& p_id, const String& p_value);
	static void update_set_tooltip(const RID& p_id, const String& p_tooltip);
	static void update_set_bounds(const RID& p_id, const Rect2& p_rect);
	static void update_set_transform(const RID& p_id, const Transform2D& p_transform);
	static void update_add_child(const RID& p_id, const RID& p_child_id);
	static void update_add_related_controls(const RID& p_id, const RID& p_related_id);
	static void update_add_related_details(const RID& p_id, const RID& p_related_id);
	static void update_add_related_described_by(const RID& p_id, const RID& p_related_id);
	static void update_add_related_flow_to(const RID& p_id, const RID& p_related_id);
	static void update_add_related_labeled_by(const RID& p_id, const RID& p_related_id);
	static void update_add_related_radio_group(const RID& p_id, const RID& p_related_id);
	static void update_set_active_descendant(const RID& p_id, const RID& p_other_id);
	static void update_set_next_on_line(const RID& p_id, const RID& p_other_id);
	static void update_set_previous_on_line(const RID& p_id, const RID& p_other_id);
	static void update_set_member_of(const RID& p_id, const RID& p_group_id);
	static void update_set_in_page_link_target(const RID& p_id, const RID& p_other_id);
	static void update_set_error_message(const RID& p_id, const RID& p_other_id);
	static void update_set_live(
		const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live);
	static void update_add_custom_action(
		const RID& p_id, int p_action_id, const String& p_action_description);
	static void update_set_table_row_count(const RID& p_id, int p_count);
	static void update_set_table_column_count(const RID& p_id, int p_count);
	static void update_set_table_row_index(const RID& p_id, int p_index);
	static void update_set_table_column_index(const RID& p_id, int p_index);
	static void update_set_table_cell_position(const RID& p_id, int p_row_index, int p_column_index);
	static void update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span);
	static void update_set_list_item_count(const RID& p_id, int p_size);
	static void update_set_list_item_index(const RID& p_id, int p_index);
	static void update_set_list_item_level(const RID& p_id, int p_level);
	static void update_set_list_item_selected(const RID& p_id, bool p_selected);
	static void update_set_list_item_expanded(const RID& p_id, bool p_expanded);
	static void update_set_popup_type(
		const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup);
	static void update_set_checked(const RID& p_id, bool p_checekd);
	static void update_set_num_value(const RID& p_id, double p_position);
	static void update_set_num_range(const RID& p_id, double p_min, double p_max);
	static void update_set_num_step(const RID& p_id, double p_step);
	static void update_set_num_jump(const RID& p_id, double p_jump);
	static void update_set_scroll_x(const RID& p_id, double p_position);
	static void update_set_scroll_x_range(const RID& p_id, double p_min, double p_max);
	static void update_set_scroll_y(const RID& p_id, double p_position);
	static void update_set_scroll_y_range(const RID& p_id, double p_min, double p_max);
	static void update_set_text_decorations(const RID& p_id, bool p_underline, bool p_strikethrough,
		bool p_overline, const Color& p_color);
	static void update_set_text_align(const RID& p_id, HorizontalAlignment p_align);
	static void update_set_text_selection(const RID& p_id, const RID& p_text_start_id,
		int p_start_char, const RID& p_text_end_id, int p_end_char);
	static void update_set_flag(
		const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value);
	static void update_set_classname(const RID& p_id, const String& p_classname);
	static void update_set_placeholder(const RID& p_id, const String& p_placeholder);
	static void update_set_language(const RID& p_id, const String& p_language);
	static void update_set_text_orientation(const RID& p_id, bool p_vertical);
	static void update_set_list_orientation(const RID& p_id, bool p_vertical);
	static void update_set_shortcut(const RID& p_id, const String& p_shortcut);
	static void update_set_url(const RID& p_id, const String& p_url);
	static void update_set_role_description(const RID& p_id, const String& p_description);
	static void update_set_state_description(const RID& p_id, const String& p_description);
	static void update_set_color_value(const RID& p_id, const Color& p_color);
	static void update_set_background_color(const RID& p_id, const Color& p_color);
	static void update_set_foreground_color(const RID& p_id, const Color& p_color);
};
// --- End servers/display/accessibility_server.h ---

// --- Folded from servers/audio/audio_server.h ---
#include <atomic>
// --- Folded from core/math/audio_frame.h ---
#include "core/math/vector2.h"
#include "core/typedefs.h"

static _FORCE_INLINE_ float undenormalize(float p_value) {
	union {
		uint32_t i;
		float f;
	} v;

	v.f = p_value;

	// original: return (v.i & 0x7f800000) == 0 ? 0.0f : f;
	// version from Tim Blechmann:
	return (v.i & 0x7f800000) < 0x08000000 ? 0.0f : p_value;
}

static const float AUDIO_PEAK_OFFSET = 0.0000000001f;
static const float AUDIO_MIN_PEAK_DB = -200.0f; // linear_to_db(AUDIO_PEAK_OFFSET)

struct AudioFrame {
	// Left and right samples.
	float left = 0.0f;
	float right = 0.0f;

	constexpr float &operator[](int p_idx) {
		// The pointer math below assumes that the elements are placed back-to-back, like an array.
		// This is always true in practice, but technically not guaranteed; we safety-check it here.
		static_assert(offsetof(AudioFrame, left) == 0 * sizeof(float));
		static_assert(offsetof(AudioFrame, right) == 1 * sizeof(float));
		static_assert(sizeof(AudioFrame) == 2 * sizeof(float));

		DEV_ASSERT((unsigned int)p_idx < 2);
		return (&left)[p_idx];
	}
	constexpr const float &operator[](int p_idx) const {
		DEV_ASSERT((unsigned int)p_idx < 2);
		return (&left)[p_idx];
	}

	constexpr AudioFrame operator+(const AudioFrame &p_frame) const { return AudioFrame(left + p_frame.left, right + p_frame.right); }
	constexpr AudioFrame operator-(const AudioFrame &p_frame) const { return AudioFrame(left - p_frame.left, right - p_frame.right); }
	constexpr AudioFrame operator*(const AudioFrame &p_frame) const { return AudioFrame(left * p_frame.left, right * p_frame.right); }
	constexpr AudioFrame operator/(const AudioFrame &p_frame) const { return AudioFrame(left / p_frame.left, right / p_frame.right); }

	constexpr AudioFrame operator+(float p_sample) const { return AudioFrame(left + p_sample, right + p_sample); }
	constexpr AudioFrame operator-(float p_sample) const { return AudioFrame(left - p_sample, right - p_sample); }
	constexpr AudioFrame operator*(float p_sample) const { return AudioFrame(left * p_sample, right * p_sample); }
	constexpr AudioFrame operator/(float p_sample) const { return AudioFrame(left / p_sample, right / p_sample); }

	constexpr void operator+=(const AudioFrame &p_frame) {
		left += p_frame.left;
		right += p_frame.right;
	}
	constexpr void operator-=(const AudioFrame &p_frame) {
		left -= p_frame.left;
		right -= p_frame.right;
	}
	constexpr void operator*=(const AudioFrame &p_frame) {
		left *= p_frame.left;
		right *= p_frame.right;
	}
	constexpr void operator/=(const AudioFrame &p_frame) {
		left /= p_frame.left;
		right /= p_frame.right;
	}

	constexpr void operator+=(float p_sample) {
		left += p_sample;
		right += p_sample;
	}
	constexpr void operator-=(float p_sample) {
		left -= p_sample;
		right -= p_sample;
	}
	constexpr void operator*=(float p_sample) {
		left *= p_sample;
		right *= p_sample;
	}
	constexpr void operator/=(float p_sample) {
		left /= p_sample;
		right /= p_sample;
	}

	_ALWAYS_INLINE_ void undenormalize() {
		left = ::undenormalize(left);
		right = ::undenormalize(right);
	}

	_FORCE_INLINE_ AudioFrame lerp(const AudioFrame &p_b, float p_t) const {
		AudioFrame res = *this;

		res.left += (p_t * (p_b.left - left));
		res.right += (p_t * (p_b.right - right));

		return res;
	}

	constexpr explicit operator Vector2() const {
		return Vector2(left, right);
	}

	constexpr AudioFrame() = default;

	constexpr AudioFrame(float p_left, float p_right) :
			left(p_left), right(p_right) {}
	constexpr explicit AudioFrame(const Vector2 &p_v2) :
			left(p_v2.x), right(p_v2.y) {}
};

constexpr AudioFrame operator*(float p_scalar, const AudioFrame &p_frame) {
	return AudioFrame(p_frame.left * p_scalar, p_frame.right * p_scalar);
}

constexpr AudioFrame operator*(int32_t p_scalar, const AudioFrame &p_frame) {
	return AudioFrame(p_frame.left * p_scalar, p_frame.right * p_scalar);
}

constexpr AudioFrame operator*(int64_t p_scalar, const AudioFrame &p_frame) {
	return AudioFrame(p_frame.left * p_scalar, p_frame.right * p_scalar);
}

template <>
struct is_zero_constructible<AudioFrame> : std::true_type {};

// --- End core/math/audio_frame.h ---
// --- Folded from core/templates/safe_list.h ---
#include "core/os/memory.h"
#include "core/typedefs.h"

#include <atomic>
#include <functional>
#include <initializer_list>

// Design goals for these classes:
// - Accessing this list with an iterator will never result in a use-after free,
//   even if the element being accessed has been logically removed from the list on
//   another thread.
// - Logical deletion from the list will not result in deallocation at that time,
//   instead the node will be deallocated at a later time when it is safe to do so.
// - No blocking synchronization primitives will be used.

// This is used in very specific areas of the engine where it's critical that these guarantees are held.

template <typename T, typename A = DefaultAllocator>
class SafeList {
	struct SafeListNode {
		std::atomic<SafeListNode *> next = nullptr;

		// If the node is logically deleted, this pointer will typically point
		// to the previous list item in time that was also logically deleted.
		std::atomic<SafeListNode *> graveyard_next = nullptr;

		std::function<void(T)> deletion_fn = [](T p_t) { return; };

		T val;
	};

	static_assert(std::atomic<T>::is_always_lock_free);

	std::atomic<SafeListNode *> head = nullptr;
	std::atomic<SafeListNode *> graveyard_head = nullptr;

	std::atomic_uint active_iterator_count = 0;

public:
	class Iterator {
		friend class SafeList;

		SafeListNode *cursor = nullptr;
		SafeList *list = nullptr;

		Iterator(SafeListNode *p_cursor, SafeList *p_list) :
				cursor(p_cursor), list(p_list) {
			list->active_iterator_count++;
		}

	public:
		Iterator(const Iterator &p_other) :
				cursor(p_other.cursor), list(p_other.list) {
			list->active_iterator_count++;
		}

		~Iterator() {
			list->active_iterator_count--;
		}

	public:
		T &operator*() {
			return cursor->val;
		}

		Iterator &operator++() {
			cursor = cursor->next;
			return *this;
		}

		// These two operators are mostly useful for comparisons to nullptr.
		bool operator==(const void *p_other) const {
			return cursor == p_other;
		}

		bool operator!=(const void *p_other) const {
			return cursor != p_other;
		}

		// These two allow easy range-based for loops.
		bool operator==(const Iterator &p_other) const {
			return cursor == p_other.cursor;
		}

		bool operator!=(const Iterator &p_other) const {
			return cursor != p_other.cursor;
		}
	};

public:
	// Calling this will cause an allocation.
	void insert(T p_value) {
		SafeListNode *new_node = memnew_allocator(SafeListNode, A);
		new_node->val = p_value;
		SafeListNode *expected_head = nullptr;
		do {
			expected_head = head.load();
			new_node->next.store(expected_head);
		} while (!head.compare_exchange_strong(/* expected= */ expected_head, /* new= */ new_node));
	}

	Iterator find(T p_value) {
		for (Iterator it = begin(); it != end(); ++it) {
			if (*it == p_value) {
				return it;
			}
		}
		return end();
	}

	void erase(T p_value, std::function<void(T)> p_deletion_fn) {
		Iterator tmp = find(p_value);
		erase(tmp, p_deletion_fn);
	}

	void erase(T p_value) {
		Iterator tmp = find(p_value);
		erase(tmp, [](T p_t) { return; });
	}

	void erase(Iterator &p_iterator, std::function<void(T)> p_deletion_fn) {
		p_iterator.cursor->deletion_fn = p_deletion_fn;
		erase(p_iterator);
	}

	void erase(Iterator &p_iterator) {
		if (find(p_iterator.cursor->val) == nullptr) {
			// Not in the list, nothing to do.
			return;
		}
		// First, remove the node from the list.
		while (true) {
			Iterator prev = begin();
			SafeListNode *expected_head = prev.cursor;
			for (; prev != end(); ++prev) {
				if (prev.cursor && prev.cursor->next == p_iterator.cursor) {
					break;
				}
			}
			if (prev != end()) {
				// There exists a node before this.
				prev.cursor->next.store(p_iterator.cursor->next.load());
				// Done.
				break;
			} else {
				if (head.compare_exchange_strong(/* expected= */ expected_head, /* new= */ p_iterator.cursor->next.load())) {
					// Successfully reassigned the head pointer before another thread changed it to something else.
					break;
				}
				// Fall through upon failure, try again.
			}
		}
		// Then queue it for deletion by putting it in the node graveyard.
		// Don't touch `next` because an iterator might still be pointing at this node.
		SafeListNode *expected_head = nullptr;
		do {
			expected_head = graveyard_head.load();
			p_iterator.cursor->graveyard_next.store(expected_head);
		} while (!graveyard_head.compare_exchange_strong(/* expected= */ expected_head, /* new= */ p_iterator.cursor));
	}

	Iterator begin() {
		return Iterator(head.load(), this);
	}

	Iterator end() {
		return Iterator(nullptr, this);
	}

	// Calling this will cause zero to many deallocations.
	bool maybe_cleanup() {
		SafeListNode *cursor = nullptr;
		SafeListNode *new_graveyard_head = nullptr;
		do {
			// The access order here is theoretically important.
			cursor = graveyard_head.load();
			if (active_iterator_count.load() != 0) {
				// It's not safe to clean up with an active iterator, because that iterator
				// could be pointing to an element that we want to delete.
				return false;
			}
			// Any iterator created after this point will never point to a deleted node.
			// Swap it out with the current graveyard head.
		} while (!graveyard_head.compare_exchange_strong(/* expected= */ cursor, /* new= */ new_graveyard_head));
		// Our graveyard list is now unreachable by any active iterators,
		// detached from the main graveyard head and ready for deletion.
		while (cursor) {
			SafeListNode *tmp = cursor;
			cursor = cursor->graveyard_next;
			tmp->deletion_fn(tmp->val);
			memdelete_allocator<SafeListNode, A>(tmp);
		}
		return true;
	}

	_FORCE_INLINE_ SafeList() {}
	_FORCE_INLINE_ SafeList(std::initializer_list<T> p_init) {
		for (const T &E : p_init) {
			insert(E);
		}
	}

	~SafeList() {
#ifdef DEBUG_ENABLED
		if (!maybe_cleanup()) {
			ERR_PRINT("There are still iterators around when destructing a SafeList. Memory will be leaked. This is a bug.");
		}
#else
		maybe_cleanup();
#endif
	}
};

// --- End core/templates/safe_list.h ---
// --- Folded from servers/audio/audio_effect.h ---
#include "core/io/ip.h"


#include <type_traits>
#include <utility>

class AudioEffectInstance final : public RefCounted
{
public:
	using ProcessFn = void (*)(void* p_state, const AudioFrame* p_src, AudioFrame* p_dst, int p_frames);
	using ProcessSilenceFn = bool (*)(const void* p_state);
	using FreeFn = void (*)(void* p_state);

	void* state = nullptr;
	ProcessFn process_fn = nullptr;
	ProcessSilenceFn process_silence_fn = nullptr;
	FreeFn free_fn = nullptr;

private:
	const void* state_type_tag = nullptr;

	template <typename T, typename = void>
	struct HasProcessSilence : std::false_type {};

	template <typename T>
	struct HasProcessSilence<T, std::void_t<decltype(std::declval<const T&>().process_silence())>> : std::true_type {};

	template <typename T>
	static const void* _state_type_id()
	{
		static const char tag = 0;
		return &tag;
	}

	template <typename StateT>
	static void _default_process(void* p_state, const AudioFrame* p_src, AudioFrame* p_dst, int p_frames)
	{
		static_cast<StateT*>(p_state)->process(p_src, p_dst, p_frames);
	}

	template <typename StateT>
	static bool _default_process_silence(const void* p_state)
	{
		return static_cast<const StateT*>(p_state)->process_silence();
	}

	template <typename StateT>
	static void _default_free(void* p_state)
	{
		memdelete(static_cast<StateT*>(p_state));
	}

public:
	_ALWAYS_INLINE_ void process(
		const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
	{
		if (process_fn && state) {
			process_fn(state, p_src_frames, p_dst_frames, p_frame_count);
		}
	}

	_ALWAYS_INLINE_ bool process_silence() const
	{
		return (process_silence_fn && state) ? process_silence_fn(state) : false;
	}

	template <typename T>
	_ALWAYS_INLINE_ T* get_state() const
	{
		return (state_type_tag == _state_type_id<T>()) ? static_cast<T*>(state) : nullptr;
	}

	template <typename StateT>
	static Ref<AudioEffectInstance> create(
		StateT* p_state,
		ProcessFn p_process_fn = nullptr,
		ProcessSilenceFn p_process_silence_fn = nullptr,
		FreeFn p_free_fn = nullptr)
	{
		Ref<AudioEffectInstance> ins;
		ins.instantiate();
		ins->state = p_state;
		ins->state_type_tag = _state_type_id<StateT>();
		ins->process_fn = p_process_fn ? p_process_fn : &_default_process<StateT>;
		if (p_process_silence_fn) {
			ins->process_silence_fn = p_process_silence_fn;
		}
		else if constexpr (HasProcessSilence<StateT>::value) {
			ins->process_silence_fn = &_default_process_silence<StateT>;
		}
		ins->free_fn = p_free_fn ? p_free_fn : &_default_free<StateT>;
		return ins;
	}

	~AudioEffectInstance()
	{
		if (free_fn && state) {
			free_fn(state);
		}
	}
};

struct AudioEffectAmplifyConfig;
struct AudioEffectCaptureConfig;
struct AudioEffectChorusConfig;
struct AudioEffectCompressorConfig;
struct AudioEffectDelayConfig;
struct AudioEffectDistortionConfig;
struct AudioEffectEQConfig;
struct AudioEffectFilterConfig;
struct AudioEffectHardLimiterConfig;
struct AudioEffectLimiterConfig;
struct AudioEffectPannerConfig;
struct AudioEffectPhaserConfig;
struct AudioEffectPitchShiftConfig;
struct AudioEffectRecordConfig;
struct AudioEffectReverbConfig;
struct AudioEffectSpectrumAnalyzerConfig;
struct AudioEffectStereoEnhanceConfig;

class AudioEffect final : public Resource
{
public:
	enum Type
	{
		EFFECT_AMPLIFY,
		EFFECT_CAPTURE,
		EFFECT_CHORUS,
		EFFECT_COMPRESSOR,
		EFFECT_DELAY,
		EFFECT_DISTORTION,
		EFFECT_EQ,
		EFFECT_FILTER,
		EFFECT_HARD_LIMITER,
		EFFECT_LIMITER,
		EFFECT_PANNER,
		EFFECT_PHASER,
		EFFECT_PITCH_SHIFT,
		EFFECT_RECORD,
		EFFECT_REVERB,
		EFFECT_SPECTRUM_ANALYZER,
		EFFECT_STEREO_ENHANCE,
	};

private:
	Type effect_type = EFFECT_AMPLIFY;
	void* config = nullptr;

	void _init_config(Type p_type);
	void _free_config();

public:
	Type get_effect_type() const { return effect_type; }
	void set_effect_type(Type p_type);

	void* get_config_ptr() const { return config; }

	AudioEffectAmplifyConfig* get_amplify_config() const
	{
		return effect_type == EFFECT_AMPLIFY ? static_cast<AudioEffectAmplifyConfig*>(config) : nullptr;
	}
	AudioEffectCaptureConfig* get_capture_config() const
	{
		return effect_type == EFFECT_CAPTURE ? static_cast<AudioEffectCaptureConfig*>(config) : nullptr;
	}
	AudioEffectChorusConfig* get_chorus_config() const
	{
		return effect_type == EFFECT_CHORUS ? static_cast<AudioEffectChorusConfig*>(config) : nullptr;
	}
	AudioEffectCompressorConfig* get_compressor_config() const
	{
		return effect_type == EFFECT_COMPRESSOR ? static_cast<AudioEffectCompressorConfig*>(config) : nullptr;
	}
	AudioEffectDelayConfig* get_delay_config() const
	{
		return effect_type == EFFECT_DELAY ? static_cast<AudioEffectDelayConfig*>(config) : nullptr;
	}
	AudioEffectDistortionConfig* get_distortion_config() const
	{
		return effect_type == EFFECT_DISTORTION ? static_cast<AudioEffectDistortionConfig*>(config) : nullptr;
	}
	AudioEffectEQConfig* get_eq_config() const
	{
		return effect_type == EFFECT_EQ ? static_cast<AudioEffectEQConfig*>(config) : nullptr;
	}
	AudioEffectFilterConfig* get_filter_config() const
	{
		return effect_type == EFFECT_FILTER ? static_cast<AudioEffectFilterConfig*>(config) : nullptr;
	}
	AudioEffectHardLimiterConfig* get_hard_limiter_config() const
	{
		return effect_type == EFFECT_HARD_LIMITER ? static_cast<AudioEffectHardLimiterConfig*>(config) : nullptr;
	}
	AudioEffectLimiterConfig* get_limiter_config() const
	{
		return effect_type == EFFECT_LIMITER ? static_cast<AudioEffectLimiterConfig*>(config) : nullptr;
	}
	AudioEffectPannerConfig* get_panner_config() const
	{
		return effect_type == EFFECT_PANNER ? static_cast<AudioEffectPannerConfig*>(config) : nullptr;
	}
	AudioEffectPhaserConfig* get_phaser_config() const
	{
		return effect_type == EFFECT_PHASER ? static_cast<AudioEffectPhaserConfig*>(config) : nullptr;
	}
	AudioEffectPitchShiftConfig* get_pitch_shift_config() const
	{
		return effect_type == EFFECT_PITCH_SHIFT ? static_cast<AudioEffectPitchShiftConfig*>(config) : nullptr;
	}
	AudioEffectRecordConfig* get_record_config() const
	{
		return effect_type == EFFECT_RECORD ? static_cast<AudioEffectRecordConfig*>(config) : nullptr;
	}
	AudioEffectReverbConfig* get_reverb_config() const
	{
		return effect_type == EFFECT_REVERB ? static_cast<AudioEffectReverbConfig*>(config) : nullptr;
	}
	AudioEffectSpectrumAnalyzerConfig* get_spectrum_analyzer_config() const
	{
		return effect_type == EFFECT_SPECTRUM_ANALYZER ? static_cast<AudioEffectSpectrumAnalyzerConfig*>(config) : nullptr;
	}
	AudioEffectStereoEnhanceConfig* get_stereo_enhance_config() const
	{
		return effect_type == EFFECT_STEREO_ENHANCE ? static_cast<AudioEffectStereoEnhanceConfig*>(config) : nullptr;
	}

	Ref<AudioEffectInstance> instantiate();

	AudioEffect(Type p_type = EFFECT_AMPLIFY);
	AudioEffect(const AudioEffect&) = delete;
	AudioEffect& operator=(const AudioEffect&) = delete;
	~AudioEffect();
};

// --- End servers/audio/audio_effect.h ---
// --- Folded from servers/audio/audio_filter_sw.h ---
#include "core/typedefs.h"

class AudioFilterSW {
public:
	struct Coeffs {
		double a1 = 0.0;
		double a2 = 0.0;
		double b0 = 0.0;
		double b1 = 0.0;
		double b2 = 0.0;
	};

	enum Mode {
		BANDPASS,
		HIGHPASS,
		LOWPASS,
		NOTCH,
		PEAK,
		BANDLIMIT,
		LOWSHELF,
		HIGHSHELF
	};

	class Processor { // Simple filter processor.
		AudioFilterSW *filter = nullptr;
		Coeffs coeffs;
		// History.
		float ha1 = 0.0f;
		float ha2 = 0.0f;
		float hb1 = 0.0f;
		float hb2 = 0.0f;
		Coeffs incr_coeffs;

	public:
		void set_filter(AudioFilterSW *p_filter, bool p_clear_history = true);
		void process(float *p_samples, int p_amount, int p_stride = 1, bool p_interpolate = false);
		void update_coeffs(int p_interp_buffer_len = 0);
		_ALWAYS_INLINE_ void process_one(float &p_sample);
		_ALWAYS_INLINE_ void process_one_interp(float &p_sample);

		Processor();
	};

private:
	float cutoff = 5000.0f;
	float resonance = 0.5f;
	float gain = 1.0f;
	float sampling_rate = 44100.0f;
	int stages = 1;
	Mode mode = LOWPASS;

public:
	float get_response(float p_freq, Coeffs *p_coeffs);

	void set_mode(Mode p_mode);
	void set_cutoff(float p_cutoff);
	void set_resonance(float p_resonance);
	void set_gain(float p_gain);
	void set_sampling_rate(float p_srate);
	void set_stages(int p_stages); //adjust for multiple stages

	void prepare_coefficients(Coeffs *p_coeffs);

	AudioFilterSW() {}
};

/* inline methods */

void AudioFilterSW::Processor::process_one(float &p_sample) {
	float pre = p_sample;
	p_sample = (p_sample * coeffs.b0 + hb1 * coeffs.b1 + hb2 * coeffs.b2 + ha1 * coeffs.a1 + ha2 * coeffs.a2);
	ha2 = ha1;
	hb2 = hb1;
	hb1 = pre;
	ha1 = p_sample;
}

void AudioFilterSW::Processor::process_one_interp(float &p_sample) {
	float pre = p_sample;
	p_sample = (p_sample * coeffs.b0 + hb1 * coeffs.b1 + hb2 * coeffs.b2 + ha1 * coeffs.a1 + ha2 * coeffs.a2);
	ha2 = ha1;
	hb2 = hb1;
	hb1 = pre;
	ha1 = p_sample;

	coeffs.b0 += incr_coeffs.b0;
	coeffs.b1 += incr_coeffs.b1;
	coeffs.b2 += incr_coeffs.b2;
	coeffs.a1 += incr_coeffs.a1;
	coeffs.a2 += incr_coeffs.a2;
}

// --- End servers/audio/audio_filter_sw.h ---

class AudioDriverDummy;
class AudioSample;
class AudioStream;
class AudioStreamWAV;
class AudioStreamPlayback;
class AudioSamplePlayback;

class AudioDriver
{
	uint64_t _last_mix_time = 0;
	uint64_t _last_mix_frames = 0;

#ifdef DEBUG_ENABLED
	SafeNumeric<uint64_t> prof_ticks;
	SafeNumeric<uint64_t> prof_time;
#endif

protected:
	Vector<int32_t> input_buffer;
	unsigned int input_position = 0;
	unsigned int input_size = 0;

	void audio_server_process(int p_frames, int32_t* p_buffer, bool p_update_mix_time = true);
	void update_mix_time(int p_frames);
	void input_buffer_init(int driver_buffer_frames);
	void input_buffer_write(int32_t sample);

	int _get_configured_mix_rate();

#ifdef DEBUG_ENABLED
	void start_counting_ticks();
	void stop_counting_ticks();
#else
	_FORCE_INLINE_ void start_counting_ticks() {}

	_FORCE_INLINE_ void stop_counting_ticks() {}
#endif

public:
	double get_time_since_last_mix(); // useful for video -> audio sync
	double get_time_to_next_mix();

	enum SpeakerMode
	{
		SPEAKER_MODE_STEREO,
		SPEAKER_SURROUND_31,
		SPEAKER_SURROUND_51,
		SPEAKER_SURROUND_71,
	};

	// Virtual API to implement.

	virtual const char* get_name() const = 0;

	virtual Error init() = 0;
	virtual void start() = 0;
	virtual int get_mix_rate() const = 0;

	virtual int get_input_mix_rate() const { return get_mix_rate(); }

	virtual SpeakerMode get_speaker_mode() const = 0;

	virtual float get_latency() { return 0; }

	virtual void lock() = 0;
	virtual void unlock() = 0;
	virtual void finish() = 0;

	virtual PackedStringArray get_output_device_list();
	virtual String get_output_device();

	virtual void set_output_device(const String& p_name) {}

	virtual Error input_start() { return FAILED; }

	virtual Error input_stop() { return FAILED; }

	virtual PackedStringArray get_input_device_list();

	virtual String get_input_device() { return "Default"; }

	virtual void set_input_device(const String& p_name) {}

	//

	SpeakerMode get_speaker_mode_by_total_channels(int p_channels) const;
	int get_total_channels_by_speaker_mode(SpeakerMode) const;

	Vector<int32_t> get_input_buffer() { return input_buffer; }

	unsigned int get_input_position() { return input_position; }

	unsigned int get_input_size() { return input_size; }

#ifdef DEBUG_ENABLED
	uint64_t get_profiling_time() const { return prof_time.get(); }

	void reset_profiling_time() { prof_time.set(0); }
#endif

	// Samples handling.
	virtual bool is_stream_registered_as_sample(const Ref<AudioStream>& p_stream) const
	{
		return false;
	}

	virtual void register_sample(const Ref<AudioSample>& p_sample) {}

	virtual void unregister_sample(const Ref<AudioSample>& p_sample) {}

	virtual void start_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	virtual void stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	virtual void set_sample_playback_pause(
		const Ref<AudioSamplePlayback>& p_playback, bool p_paused)
	{
	}

	virtual bool is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback)
	{
		return false;
	}

	virtual double get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback)
	{
		return false;
	}

	virtual void update_sample_playback_pitch_scale(
		const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale = 0.0f)
	{
	}

	virtual void set_sample_playback_bus_volumes_linear(const Ref<AudioSamplePlayback>& p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes)
	{
	}

	virtual void set_sample_bus_count(int p_count) {}

	virtual void remove_sample_bus(int p_bus) {}

	virtual void add_sample_bus(int p_at_pos = -1) {}

	virtual void move_sample_bus(int p_bus, int p_to_pos) {}

	virtual void set_sample_bus_send(int p_bus, const String& p_send) {}

	virtual void set_sample_bus_volume_db(int p_bus, float p_volume_db) {}

	virtual void set_sample_bus_solo(int p_bus, bool p_enable) {}

	virtual void set_sample_bus_mute(int p_bus, bool p_enable) {}

	AudioDriver() {}

	virtual ~AudioDriver() {}
};

class AudioDriverManager final
{
	enum
	{
		MAX_DRIVERS = 10
	};

	static AudioDriver* drivers[MAX_DRIVERS];
	static int driver_count;

	static AudioDriverDummy dummy_driver;
	static AudioDriver* active_driver;

public:
	static const int DEFAULT_MIX_RATE = 44100;

	static void add_driver(AudioDriver* p_driver);
	static void initialize(int p_driver);
	static int get_driver_count();
	static AudioDriver* get_driver(int p_driver);

	_FORCE_INLINE_ static AudioDriver* get_active_driver() { return active_driver; }
	static void set_active_driver(AudioDriver* p_driver);
	_FORCE_INLINE_ static AudioDriverDummy* get_dummy_driver() { return &dummy_driver; }

	AudioDriverManager() = delete;
};

class AudioBusLayout;

class AudioServer final
{
public:
	// re-expose this here, as AudioDriver is not exposed to script
	enum SpeakerMode
	{
		SPEAKER_MODE_STEREO,
		SPEAKER_SURROUND_31,
		SPEAKER_SURROUND_51,
		SPEAKER_SURROUND_71,
	};

	enum PlaybackType
	{
		PLAYBACK_TYPE_DEFAULT,
		PLAYBACK_TYPE_STREAM,
		PLAYBACK_TYPE_SAMPLE,
		PLAYBACK_TYPE_MAX
	};

	enum
	{
		AUDIO_DATA_INVALID_ID = -1,
		MAX_CHANNELS_PER_BUS = 4,
		MAX_BUSES_PER_PLAYBACK = 6,
		LOOKAHEAD_BUFFER_SIZE = 64,
	};

	typedef void (*AudioCallback)(void* p_userdata);

private:

	struct Bus
	{
		String name;
		bool solo = false;
		bool mute = false;
		bool bypass = false;

		bool soloed = false;

		// Each channel is a stereo pair.
		struct Channel
		{
			bool used = false;
			bool active = false;
			AudioFrame peak_volume = AudioFrame(AUDIO_MIN_PEAK_DB, AUDIO_MIN_PEAK_DB);
			Vector<AudioFrame> buffer;
			Vector<Ref<AudioEffectInstance>> effect_instances;
			uint64_t last_mix_with_audio = 0;

			Channel() {}
		};

		Vector<Channel> channels;

		struct Effect
		{
			Ref<AudioEffect> effect;
			bool enabled = false;
#ifdef DEBUG_ENABLED
			uint64_t prof_time = 0;
#endif
		};

		Vector<Effect> effects;
		float volume_db = 0.0f;
		String send;
		int index_cache = 0;
	};

	struct AudioStreamPlaybackBusDetails
	{
		bool bus_active[MAX_BUSES_PER_PLAYBACK] = {};
		String bus[MAX_BUSES_PER_PLAYBACK];
		AudioFrame volume[MAX_BUSES_PER_PLAYBACK][MAX_CHANNELS_PER_BUS];
	};

	struct AudioStreamPlaybackListNode
	{
		// The state machine for audio stream playbacks is as follows:
		// 1. The playback is created and added to the playback list in the playing state.
		// 2. The playback is (maybe) paused, and the state is set to FADE_OUT_TO_PAUSE.
		// 2.1. The playback is mixed after being paused, and the audio server thread atomically
		// sets the state to PAUSED after performing a brief fade-out.
		// 3. The playback is (maybe) deleted, and the state is set to FADE_OUT_TO_DELETION.
		// 3.1. The playback is mixed after being deleted, and the audio server thread atomically
		// sets the state to AWAITING_DELETION after performing a brief fade-out. 		NOTE: The playback
		// is not deallocated at this time because allocation and deallocation are not
		// realtime-safe.
		// 4. The playback is removed and deallocated on the main thread using the SafeList
		// maybe_cleanup method.
		enum PlaybackState
		{
			PAUSED = 0,	 // Paused. Keep this stream playback around though so it can be restarted.
			PLAYING = 1, // Playing. Fading may still be necessary if volume changes!
			FADE_OUT_TO_PAUSE = 2,	  // About to pause.
			FADE_OUT_TO_DELETION = 3, // About to stop.
			AWAITING_DELETION = 4,
		};

		// If zero or positive, a place in the stream to seek to during the next mix.
		SafeNumeric<float> setseek;
		SafeNumeric<float> pitch_scale;
		SafeNumeric<float> highshelf_gain;
		SafeNumeric<float>
			attenuation_filter_cutoff_hz; // This isn't used unless highshelf_gain is nonzero.
		AudioFilterSW::Processor filter_process[8];
		// Updating this ref after the list node is created breaks consistency guarantees, don't do
		// it!
		Ref<AudioStreamPlayback> stream_playback;
		// Playback state determines the fate of a particular AudioStreamListNode during the mix
		// step. Must be atomically replaced.
		std::atomic<PlaybackState> state = AWAITING_DELETION;
		// This data should only ever be modified by an atomic replacement of the pointer.
		std::atomic<AudioStreamPlaybackBusDetails*> bus_details = nullptr;
		// Previous bus details should only be accessed on the audio thread.
		AudioStreamPlaybackBusDetails* prev_bus_details = nullptr;
		// The next few samples are stored here so we have some time to fade audio out if it ends
		// abruptly at the beginning of the next mix.
		AudioFrame lookahead[LOOKAHEAD_BUFFER_SIZE];
	};

	struct CallbackItem
	{
		AudioCallback callback;
		void* userdata = nullptr;
	};

	struct Data
	{
		uint64_t mix_time = 0;
		int mix_size = 0;

		uint32_t buffer_size = 0;
		uint64_t mix_count = 0;
		uint64_t mix_frames = 0;
#ifdef DEBUG_ENABLED
		SafeNumeric<uint64_t> prof_time;
#endif

		float channel_disable_threshold_db = 0.0f;
		uint32_t channel_disable_frames = 0;

		int channel_count = 0;
		int to_mix = 0;

		float playback_speed_scale = 1.0f;

		bool tag_used_audio_streams = false;

#ifdef DEBUG_ENABLED
		bool debug_mute = false;
#endif // DEBUG_ENABLED

		bool input_device_active = false;
		int input_buffer_ofs = 0;

		SafeList<AudioStreamPlaybackListNode*> playback_list;
		SafeList<AudioStreamPlaybackBusDetails*> bus_details_graveyard;
		SafeList<AudioStreamPlaybackBusDetails*> bus_details_graveyard_frame_old;

		Vector<Vector<AudioFrame>> temp_buffer; // temp_buffer for each level
		Vector<AudioFrame> mix_buffer;
		Vector<Bus*> buses;
		HashMap<String, Bus*> bus_map;

		SafeList<CallbackItem*> update_callback_list;
		SafeList<CallbackItem*> mix_callback_list;
		SafeList<CallbackItem*> listener_changed_callback_list;

		LocalVector<Ref<AudioSamplePlayback>> sample_playback_list;
	};

	static inline Data* data = nullptr;

	static void _delete_stream_playback(Ref<AudioStreamPlayback> p_playback);
	static void _delete_stream_playback_list_node(AudioStreamPlaybackListNode* p_node);

	static void _cleanup_lists();
	static void _update_bus_effects(int p_bus);
	static void init_channels_and_buffers();

	static void _mix_step();
	static void _mix_step_for_channel(AudioFrame* p_out_buf, AudioFrame* p_source_buf,
		AudioFrame p_vol_start, AudioFrame p_vol_final, float p_attenuation_filter_cutoff_hz,
		float p_highshelf_gain, AudioFilterSW::Processor* p_processor_l,
		AudioFilterSW::Processor* p_processor_r);

	// Should only be called on the main thread.
	static AudioStreamPlaybackListNode* _find_playback_list_node(Ref<AudioStreamPlayback> p_playback);

	friend class AudioDriver;
	static void _driver_process(int p_frames, int32_t* p_buffer);

protected:

public:
	static _FORCE_INLINE_ int get_channel_count()
	{
		switch (get_speaker_mode()) {
		case SPEAKER_MODE_STEREO:
			return 1;
		case SPEAKER_SURROUND_31:
			return 2;
		case SPEAKER_SURROUND_51:
			return 3;
		case SPEAKER_SURROUND_71:
			return 4;
		}
		ERR_FAIL_V(1);
	}

	// Do not use from outside audio thread.
	static bool thread_has_channel_mix_buffer(int p_bus, int p_buffer);
	static AudioFrame* thread_get_channel_mix_buffer(int p_bus, int p_buffer);
	static int thread_get_mix_buffer_size();
	static int thread_find_bus_index(const String& p_name);

#ifdef DEBUG_ENABLED
	static void set_debug_mute(bool p_mute);
	static bool get_debug_mute();
#endif // DEBUG_ENABLED

	static void set_bus_count(int p_count);
	static int get_bus_count();

	static void remove_bus(int p_index);
	static void add_bus(int p_at_pos = -1);

	static void move_bus(int p_bus, int p_to_pos);

	static void set_bus_name(int p_bus, const String& p_name);
	static String get_bus_name(int p_bus);
	static int get_bus_index(const String& p_bus_name);

	static int get_bus_channels(int p_bus);

	static void set_bus_volume_db(int p_bus, float p_volume_db);
	static float get_bus_volume_db(int p_bus);

	static void set_bus_volume_linear(int p_bus, float p_volume_linear);
	static float get_bus_volume_linear(int p_bus);

	static void set_bus_send(int p_bus, const String& p_send);
	static String get_bus_send(int p_bus);

	static void set_bus_solo(int p_bus, bool p_enable);
	static bool is_bus_solo(int p_bus);

	static void set_bus_mute(int p_bus, bool p_enable);
	static bool is_bus_mute(int p_bus);

	static void set_bus_bypass_effects(int p_bus, bool p_enable);
	static bool is_bus_bypassing_effects(int p_bus);

	static void add_bus_effect(int p_bus, const Ref<AudioEffect>& p_effect, int p_at_pos = -1);
	static void remove_bus_effect(int p_bus, int p_effect);

	static int get_bus_effect_count(int p_bus);
	static Ref<AudioEffect> get_bus_effect(int p_bus, int p_effect);
	static Ref<AudioEffectInstance> get_bus_effect_instance(int p_bus, int p_effect, int p_channel = 0);

	static void swap_bus_effects(int p_bus, int p_effect, int p_by_effect);

	static void set_bus_effect_enabled(int p_bus, int p_effect, bool p_enabled);
	static bool is_bus_effect_enabled(int p_bus, int p_effect);

	static float get_bus_peak_volume_left_db(int p_bus, int p_channel);
	static float get_bus_peak_volume_right_db(int p_bus, int p_channel);

	static bool is_bus_channel_active(int p_bus, int p_channel);

	static void set_playback_speed_scale(float p_scale);
	static float get_playback_speed_scale();

	// Convenience method.
	static void start_playback_stream(Ref<AudioStreamPlayback> p_playback, const String& p_bus,
		Vector<AudioFrame> p_volume_db_vector, float p_start_time = 0, float p_pitch_scale = 1);
	// Expose all parameters.
	static void start_playback_stream(Ref<AudioStreamPlayback> p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes, float p_start_time = 0,
		float p_pitch_scale = 1, float p_highshelf_gain = 0, float p_attenuation_cutoff_hz = 0);
	static void stop_playback_stream(Ref<AudioStreamPlayback> p_playback);

	static void set_playback_bus_exclusive(
		Ref<AudioStreamPlayback> p_playback, const String& p_bus, Vector<AudioFrame> p_volumes);
	static void set_playback_bus_volumes_linear(Ref<AudioStreamPlayback> p_playback,
		const HashMap<String, Vector<AudioFrame>>& p_bus_volumes);
	static void set_playback_all_bus_volumes_linear(
		Ref<AudioStreamPlayback> p_playback, Vector<AudioFrame> p_volumes);
	static void set_playback_pitch_scale(Ref<AudioStreamPlayback> p_playback, float p_pitch_scale);
	static void set_playback_paused(Ref<AudioStreamPlayback> p_playback, bool p_paused);
	static void set_playback_highshelf_params(
		Ref<AudioStreamPlayback> p_playback, float p_gain, float p_attenuation_cutoff_hz);

	static bool is_playback_active(Ref<AudioStreamPlayback> p_playback);
	static float get_playback_position(Ref<AudioStreamPlayback> p_playback);
	static bool is_playback_paused(Ref<AudioStreamPlayback> p_playback);

	static uint64_t get_mix_count();
	static uint64_t get_mixed_frames();

	static String get_driver_name();

	static void notify_listener_changed();

	static void init();
	static void finish();
	static void update();
	static void load_default_bus_layout();

	/* MISC config */

	static void lock();
	static void unlock();

	static SpeakerMode get_speaker_mode();
	static float get_mix_rate();
	static float get_input_mix_rate();

	static float read_output_peak_db();

	static double get_output_latency();
	static double get_time_to_next_mix();
	static double get_time_since_last_mix();

	static void add_listener_changed_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_listener_changed_callback(AudioCallback p_callback, void* p_userdata);

	static void add_update_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_update_callback(AudioCallback p_callback, void* p_userdata);

	static void add_mix_callback(AudioCallback p_callback, void* p_userdata);
	static void remove_mix_callback(AudioCallback p_callback, void* p_userdata);

	static void set_bus_layout(const Ref<AudioBusLayout>& p_bus_layout);
	static Ref<AudioBusLayout> generate_bus_layout();

	static PackedStringArray get_output_device_list();
	static String get_output_device();
	static void set_output_device(const String& p_name);

	static PackedStringArray get_input_device_list();
	static String get_input_device();
	static void set_input_device(const String& p_name);
	static Error set_input_device_active(bool p_is_active);
	static int get_input_frames_available();
	static int get_input_buffer_length_frames();
	static PackedVector2Array get_input_frames(int p_frames);

	static void set_enable_tagging_used_audio_streams(bool p_enable);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	static PlaybackType get_default_playback_type();

	static bool is_stream_registered_as_sample(const Ref<AudioStream>& p_stream);
	static void register_stream_as_sample(const Ref<AudioStream>& p_stream);
	static void unregister_stream_as_sample(const Ref<AudioStream>& p_stream);
	static void register_sample(const Ref<AudioSample>& p_sample);
	static void unregister_sample(const Ref<AudioSample>& p_sample);
	static void start_sample_playback(const Ref<AudioSamplePlayback>& p_playback);
	static void stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback);
	static void set_sample_playback_pause(const Ref<AudioSamplePlayback>& p_playback, bool p_paused);
	static bool is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback);
	static double get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback);
	static void update_sample_playback_pitch_scale(
		const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale = 0.0f);

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	AudioServer() = delete;
	AudioServer(const AudioServer&) = delete;
	AudioServer& operator=(const AudioServer&) = delete;
	~AudioServer() = delete;
};

class AudioBusLayout : public Resource
{
	friend class AudioServer;

	struct Bus
	{
		String name;
		bool solo = false;
		bool mute = false;
		bool bypass = false;

		struct Effect
		{
			Ref<AudioEffect> effect;
			bool enabled = false;
		};

		Vector<Effect> effects;

		float volume_db = 0.0f;
		String send;

		Bus() {}
	};
	Vector<Bus> buses;

public:
	AudioBusLayout();
};

// --- Folded from servers/audio/audio_driver_dummy.h ---
#include "core/os/mutex.h"
#include "core/os/memory.h"


class AudioDriverDummy : public AudioDriver {
	Thread thread;
	Mutex mutex;

	int32_t *samples_in = nullptr;

	static void thread_func(void *p_udata);

	uint32_t buffer_frames = 4096;
	int32_t mix_rate = -1;
	SpeakerMode speaker_mode = SPEAKER_MODE_STEREO;

	int channels;

	SafeFlag active;
	SafeFlag exit_thread;

	bool use_threads = true;

public:
	virtual const char *get_name() const override {
		return "Dummy";
	}

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override;
	virtual SpeakerMode get_speaker_mode() const override;

	virtual void lock() override;
	virtual void unlock() override;
	virtual void finish() override;

	void set_use_threads(bool p_use_threads);
	void set_speaker_mode(SpeakerMode p_mode);
	void set_mix_rate(int p_rate);

	uint32_t get_channels() const;

	void mix_audio(int p_frames, int32_t *p_buffer);

	AudioDriverDummy();
	~AudioDriverDummy() {}
};

// --- End servers/audio/audio_driver_dummy.h ---

// --- Folded from servers/audio/audio_rb_resampler.h ---

#include "core/os/memory.h"
#include "core/typedefs.h"

struct AudioRBResampler {
	uint32_t rb_bits;
	uint32_t rb_len;
	uint32_t rb_mask;
	uint32_t read_buff_len;
	uint32_t channels;
	uint32_t src_mix_rate;
	uint32_t target_mix_rate;
	double playback_speed = 1.0;

	SafeNumeric<int> rb_read_pos;
	SafeNumeric<int> rb_write_pos;

	int32_t offset; //contains the fractional remainder of the resampler
	enum {
		MIX_FRAC_BITS = 13,
		MIX_FRAC_LEN = (1 << MIX_FRAC_BITS),
		MIX_FRAC_MASK = MIX_FRAC_LEN - 1,
	};

	float *read_buf = nullptr;
	float *rb = nullptr;

	template <int C>
	uint32_t _resample(AudioFrame *p_dest, int p_todo, int32_t p_increment);

public:
	_FORCE_INLINE_ void flush() {
		rb_read_pos.set(0);
		rb_write_pos.set(0);
		offset = 0;
	}

	_FORCE_INLINE_ bool is_ready() const {
		return rb != nullptr;
	}

	_FORCE_INLINE_ int get_total() const {
		return rb_len - 1;
	}

	_FORCE_INLINE_ int get_writer_space() const {
		int space, r, w;

		r = rb_read_pos.get();
		w = rb_write_pos.get();

		if (r == w) {
			space = rb_len - 1;
		} else if (w < r) {
			space = r - w - 1;
		} else {
			space = (rb_len - w) + (r - 1);
		}

		return space;
	}

	_FORCE_INLINE_ int get_reader_space() const {
		int space, r, w;

		r = rb_read_pos.get();
		w = rb_write_pos.get();

		if (r == w) {
			space = 0;
		} else if (w < r) {
			space = rb_len - r + w;
		} else {
			space = w - r;
		}

		return space;
	}

	_FORCE_INLINE_ bool has_data() const {
		return rb && rb_read_pos.get() != rb_write_pos.get();
	}

	_FORCE_INLINE_ float *get_write_buffer() { return read_buf; }
	_FORCE_INLINE_ void write(uint32_t p_frames) {
		ERR_FAIL_COND(p_frames >= rb_len);

		int wp = rb_write_pos.get();

		switch (channels) {
			case 1: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[wp] = read_buf[i];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 2: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 1) + 0] = read_buf[(i << 1) + 0];
					rb[(wp << 1) + 1] = read_buf[(i << 1) + 1];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 4: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 2) + 0] = read_buf[(i << 2) + 0];
					rb[(wp << 2) + 1] = read_buf[(i << 2) + 1];
					rb[(wp << 2) + 2] = read_buf[(i << 2) + 2];
					rb[(wp << 2) + 3] = read_buf[(i << 2) + 3];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 6: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp * 6) + 0] = read_buf[(i * 6) + 0];
					rb[(wp * 6) + 1] = read_buf[(i * 6) + 1];
					rb[(wp * 6) + 2] = read_buf[(i * 6) + 2];
					rb[(wp * 6) + 3] = read_buf[(i * 6) + 3];
					rb[(wp * 6) + 4] = read_buf[(i * 6) + 4];
					rb[(wp * 6) + 5] = read_buf[(i * 6) + 5];
					wp = (wp + 1) & rb_mask;
				}
			} break;
			case 8: {
				for (uint32_t i = 0; i < p_frames; i++) {
					rb[(wp << 3) + 0] = read_buf[(i << 3) + 0];
					rb[(wp << 3) + 1] = read_buf[(i << 3) + 1];
					rb[(wp << 3) + 2] = read_buf[(i << 3) + 2];
					rb[(wp << 3) + 3] = read_buf[(i << 3) + 3];
					rb[(wp << 3) + 4] = read_buf[(i << 3) + 4];
					rb[(wp << 3) + 5] = read_buf[(i << 3) + 5];
					rb[(wp << 3) + 6] = read_buf[(i << 3) + 6];
					rb[(wp << 3) + 7] = read_buf[(i << 3) + 7];
					wp = (wp + 1) & rb_mask;
				}
			} break;
		}

		rb_write_pos.set(wp);
	}

	int get_channel_count() const;

	Error setup(int p_channels, int p_src_mix_rate, int p_target_mix_rate, int p_buffer_msec, int p_minbuff_needed = -1);
	void clear();
	bool mix(AudioFrame *p_dest, int p_frames);
	int get_num_of_ready_frames();
	void set_playback_speed(double p_playback_speed);
	double get_playback_speed() const;

	AudioRBResampler();
	~AudioRBResampler();
};

// --- End servers/audio/audio_rb_resampler.h ---

// --- Folded from servers/audio/audio_stream.h ---
#include "core/io/ip.h"
#include "core/string/ustring.h"
#include "core/io/ip.h"


class AudioStream;

class AudioSamplePlayback : public RefCounted
{
public:
	Ref<AudioStream> stream;
	Ref<AudioStreamPlayback> stream_playback;

	float offset = 0.0f;
	float pitch_scale = 1.0;
	Vector<AudioFrame> volume_vector;
	String bus;
};

class AudioSample : public RefCounted
{
public:
	enum LoopMode
	{
		LOOP_DISABLED,
		LOOP_FORWARD,
		LOOP_PINGPONG,
		LOOP_BACKWARD,
	};

	Ref<AudioStream> stream;
	Vector<AudioFrame> data;
	int num_channels = 1;
	int sample_rate = 44100;
	LoopMode loop_mode = LOOP_DISABLED;
	int loop_begin = 0;
	int loop_end = 0;
};

///////////

class AudioStreamPlayback : public RefCounted
{
protected:
	PackedVector2Array _mix_audio_bind(float p_rate_scale, int p_frames);

public:
	virtual void start(double p_from_pos = 0.0);
	virtual void stop();
	virtual bool is_playing() const;

	virtual int get_loop_count() const; // times it looped

	virtual double get_playback_position() const;
	virtual void seek(double p_time);

	virtual void tag_used_streams();

	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames);

	virtual void set_is_sample(bool p_is_sample) {}

	virtual bool get_is_sample() const { return false; }

	virtual Ref<AudioSamplePlayback> get_sample_playback() const;

	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	AudioStreamPlayback();
	~AudioStreamPlayback();

	Vector<AudioFrame> mix_audio(float p_rate_scale, int p_frames);
	void start_playback(double p_from_pos = 0.0);
	void stop_playback();
	void seek_playback(double p_time);
};

class AudioStreamPlaybackResampled : public AudioStreamPlayback
{
	enum
	{
		FP_BITS = 16, // fixed point used for resampling
		FP_LEN = (1 << FP_BITS),
		FP_MASK = FP_LEN - 1,
		INTERNAL_BUFFER_LEN = 128, // 128 warrants 3ms positional jitter at much at 44100hz
		CUBIC_INTERP_HISTORY = 4
	};

	AudioFrame internal_buffer[INTERNAL_BUFFER_LEN + CUBIC_INTERP_HISTORY];
	unsigned int internal_buffer_end = -1;
	uint64_t mix_offset = 0;

protected:
	void begin_resample();
	// Returns the number of frames that were mixed.
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames);
	virtual float get_stream_sampling_rate();

public:
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;
	virtual float get_stream_sampling_rate() const;

	AudioStreamPlaybackResampled() { mix_offset = 0; }
};

class AudioStream : public Resource
{
	enum
	{
		MAX_TAGGED_OFFSETS = 8
	};

	uint64_t tagged_frame = 0;
	uint64_t offset_count = 0;
	float tagged_offsets[MAX_TAGGED_OFFSETS];

public:
	virtual Ref<AudioStreamPlayback> instantiate_playback();

	virtual double get_bpm() const;
	virtual bool has_loop() const;
	virtual int get_bar_beats() const;
	virtual int get_beat_count() const;

	virtual double get_length() const;
	virtual bool is_monophonic() const;
	virtual String get_stream_name() const;

	void tag_used(float p_offset);
	uint64_t get_tagged_frame() const;
	uint32_t get_tagged_frame_count() const;
	float get_tagged_frame_offset(int p_index) const;

	virtual bool can_be_sampled() const { return false; }

	virtual Ref<AudioSample> generate_sample() const;

	virtual bool is_meta_stream() const { return false; }
};

// Generator / Microphone

class AudioStreamGeneratorPlayback;

class AudioStreamGenerator final : public AudioStream
{
public:
	enum SourceMode
	{
		SOURCE_BUFFER,
		SOURCE_MICROPHONE,
	};

	enum AudioStreamGeneratorMixRate
	{
		MIX_RATE_OUTPUT,
		MIX_RATE_INPUT,
		MIX_RATE_CUSTOM,
		MIX_RATE_MAX,
	};

private:
	friend class AudioStreamGeneratorPlayback;

	SourceMode source_mode = SOURCE_BUFFER;
	AudioStreamGeneratorMixRate mix_rate_mode = MIX_RATE_CUSTOM;
	float mix_rate = 44100;
	float buffer_len = 0.5;
	HashSet<AudioStreamGeneratorPlayback*> playbacks;

public:
	void set_source_mode(SourceMode p_mode);
	SourceMode get_source_mode() const;

	float _get_target_rate() const;

	void set_mix_rate(float p_mix_rate);
	float get_mix_rate() const;

	void set_mix_rate_mode(AudioStreamGeneratorMixRate p_mix_rate_mode);
	AudioStreamGeneratorMixRate get_mix_rate_mode() const;

	void set_buffer_length(float p_seconds);
	float get_buffer_length() const;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	virtual double get_length() const override;
	virtual bool is_monophonic() const override;

	AudioStreamGenerator() = default;
};

class AudioStreamGeneratorPlayback final : public AudioStreamPlaybackResampled
{
	friend class AudioStreamGenerator;

	Ref<AudioStreamGenerator> generator;
	RingBuffer<AudioFrame> buffer;
	int skips = 0;
	bool active = false;
	bool using_microphone = false;
	float mixed = 0;
	unsigned int input_ofs = 0;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	bool push_frame(const Vector2& p_frame);
	bool can_push_buffer(int p_frames) const;
	bool push_buffer(const PackedVector2Array& p_frames);
	int get_frames_available() const;
	int get_skips() const;

	virtual void tag_used_streams() override;

	void clear_buffer();

	AudioStreamGeneratorPlayback();
	~AudioStreamGeneratorPlayback();
};

// Polyphonic / Randomizer

class AudioStreamPlaybackPolyphonic;

class AudioStreamPolyphonic final : public AudioStream
{
public:
	enum Mode
	{
		MODE_POLYPHONIC,
		MODE_RANDOMIZER,
	};

	enum PlaybackMode
	{
		PLAYBACK_RANDOM_NO_REPEATS,
		PLAYBACK_RANDOM,
		PLAYBACK_SEQUENTIAL,
	};

private:
	friend class AudioStreamPlaybackPolyphonic;

	struct PoolEntry
	{
		Ref<AudioStream> stream;
		float weight = 1.0f;
	};

	Mode mode = MODE_POLYPHONIC;

	// Polyphonic state
	int polyphony = 32;
	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	// Randomizer state
	HashSet<AudioStreamPlaybackPolyphonic*> playbacks;
	Vector<PoolEntry> audio_stream_pool;
	float random_pitch_scale = 1.0f;
	float random_volume_offset_db = 0.0f;
	Ref<AudioStream> last_playback;
	PlaybackMode playback_mode = PLAYBACK_RANDOM_NO_REPEATS;

	Ref<AudioStreamPlayback> instance_playback_random();
	Ref<AudioStreamPlayback> instance_playback_no_repeats();
	Ref<AudioStreamPlayback> instance_playback_sequential();

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const;

	// Polyphonic API
	void set_polyphony(int p_voices);
	int get_polyphony() const;

	// Randomizer API
	void add_stream(int p_index, Ref<AudioStream> p_stream, float p_weight = 1.0);
	void move_stream(int p_index_from, int p_index_to);
	void remove_stream(int p_index);

	void set_stream(int p_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_stream(int p_index) const;
	void set_stream_probability_weight(int p_index, float p_weight);
	float get_stream_probability_weight(int p_index) const;

	void set_streams_count(int p_count);
	int get_streams_count() const;

	void set_random_pitch_semitones(float p_pitch_semitones);
	float get_random_pitch_semitones() const;

	void set_random_pitch(float p_pitch_scale);
	float get_random_pitch() const;

	void set_random_volume_offset_db(float p_volume_offset_db);
	float get_random_volume_offset_db() const;

	void set_playback_mode(PlaybackMode p_playback_mode);
	PlaybackMode get_playback_mode() const;

	// AudioStream overrides
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual double get_length() const override;
	virtual bool is_monophonic() const override;
	virtual bool is_meta_stream() const override { return true; }

	AudioStreamPolyphonic();
};

class AudioStreamPlaybackPolyphonic final : public AudioStreamPlayback
{
	friend class AudioStreamPolyphonic;

	constexpr static uint32_t INTERNAL_BUFFER_LEN = 128;

	struct Stream
	{
		SafeFlag active;
		SafeFlag pending_play;
		SafeFlag finish_request;
		float play_offset = 0;
		float pitch_scale = 1.0;
		Ref<AudioStream> stream;
		Ref<AudioStreamPlayback> stream_playback;
		float prev_volume_db = 0;
		float volume_db = 0;
		uint32_t id = 0;

		Stream() : active(false), pending_play(false), finish_request(false) {}
	};

	AudioStreamPolyphonic::Mode mode = AudioStreamPolyphonic::MODE_POLYPHONIC;
	Ref<AudioStreamPolyphonic> parent_stream;

	// Polyphonic state
	LocalVector<Stream> streams;
	AudioFrame internal_buffer[INTERNAL_BUFFER_LEN];
	bool active = false;
	uint32_t id_counter = 1;
	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

	// Randomizer state
	Ref<AudioStreamPlayback> randomizer_playback;
	Ref<AudioStreamPlayback> randomizer_playing;
	float pitch_scale = 1.0f;
	float volume_scale = 1.0f;

	_FORCE_INLINE_ Stream* _find_stream(int64_t p_id);
	_FORCE_INLINE_ const Stream* _find_stream(int64_t p_id) const;

public:
	typedef int64_t ID;

	enum
	{
		INVALID_ID = -1
	};

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	ID play_stream(const Ref<AudioStream>& p_stream, float p_from_offset = 0, float p_volume_db = 0,
		float p_pitch_scale = 1.0,
		AudioServer::PlaybackType p_playback_type =
			AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT,
		const String& p_bus = "Master");
	void set_stream_volume(ID p_stream_id, float p_volume_db);
	void set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale);
	bool is_stream_playing(ID p_stream_id) const;
	void stop_stream(ID p_stream_id);

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackPolyphonic();
	~AudioStreamPlaybackPolyphonic();
};

// --- Folded from scene/resources/audio_stream_wav.h ---
#include <thirdparty/misc/qoa.h>


class AudioStreamWAV;

class AudioStreamPlaybackWAV : public AudioStreamPlaybackResampled
{
	struct IMA_ADPCM_State
	{
		int16_t step_index = 0;
		int32_t predictor = 0;
		/* values at loop point */
		int16_t loop_step_index = 0;
		int32_t loop_predictor = 0;
		int32_t last_nibble = 0;
		int32_t loop_pos = 0;
		int32_t window_ofs = 0;
	} ima_adpcm[2];

	struct QOA_State
	{
		qoa_desc desc = {};
		uint64_t data_ofs = 0;
		uint32_t frame_len = 0;
		TightLocalVector<int16_t> dec;
	} qoa;

	int64_t offset = 0;
	int8_t sign = 1;
	bool active = false;
	friend class AudioStreamWAV;
	Ref<AudioStreamWAV> base;

	template <typename Depth, bool is_stereo, bool is_ima_adpcm, bool is_qoa>
	void decode_samples(const Depth* p_src, AudioFrame* p_dst, int64_t& p_offset,
		int8_t& p_increment, uint32_t p_amount, IMA_ADPCM_State* p_ima_adpcm, QOA_State* p_qoa);

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;
};

class AudioStreamWAV : public AudioStream
{
public:
	enum Format
	{
		FORMAT_8_BITS,
		FORMAT_16_BITS,
		FORMAT_IMA_ADPCM,
		FORMAT_QOA,
	};

	// Keep the ResourceImporterWAV `edit/loop_mode` enum hint in sync with these options.
	enum LoopMode
	{
		LOOP_DISABLED,
		LOOP_FORWARD,
		LOOP_PINGPONG,
		LOOP_BACKWARD
	};

private:
	friend class AudioStreamPlaybackWAV;

	Format format = FORMAT_8_BITS;
	LoopMode loop_mode = LOOP_DISABLED;
	bool stereo = false;
	int64_t loop_begin = 0;
	int64_t loop_end = 0;
	uint32_t mix_rate = 44100;
	TightLocalVector<uint8_t, uint64_t> data;

protected:

public:
	void set_format(Format p_format);
	Format get_format() const;

	void set_loop_mode(LoopMode p_loop_mode);
	LoopMode get_loop_mode() const;

	void set_loop_begin(int p_frame);
	int get_loop_begin() const;

	void set_loop_end(int p_frame);
	int get_loop_end() const;

	void set_mix_rate(int p_hz);
	int get_mix_rate() const;

	void set_stereo(bool p_enable);
	bool is_stereo() const;

	virtual double get_length() const override; // if supported, otherwise return 0

	virtual bool is_monophonic() const override;

	void set_data(const Vector<uint8_t>& p_data);
	Vector<uint8_t> get_data() const;

	Error save_to_wav(const String& p_path);

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;

	static void _compress_ima_adpcm(const Vector<float>& p_data, Vector<uint8_t>& r_dst_data)
	{
		static const int16_t _ima_adpcm_step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19,
			21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
			157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
			796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
			3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
			12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

		static const int8_t _ima_adpcm_index_table[16] = {
			-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

		int datalen = p_data.size();
		int datamax = datalen;
		if (datalen & 1) {
			datalen++;
		}

		r_dst_data.resize(datalen / 2 + 4);
		uint8_t* w = r_dst_data.ptrw();

		int i, step_idx = 0, prev = 0;
		uint8_t* out = w;
		const float* in = p_data.ptr();

		// Initial value is zero.
		*(out++) = 0;
		*(out++) = 0;
		// Table index initial value.
		*(out++) = 0;
		// Unused.
		*(out++) = 0;

		for (i = 0; i < datalen; i++) {
			int step, diff, vpdiff, mask;
			uint8_t nibble;
			int16_t xm_sample;

			if (i >= datamax) {
				xm_sample = 0;
			}
			else {
				xm_sample = CLAMP(in[i] * 32767.0, -32768, 32767);
			}

			diff = (int)xm_sample - prev;

			nibble = 0;
			step = _ima_adpcm_step_table[step_idx];
			vpdiff = step >> 3;
			if (diff < 0) {
				nibble = 8;
				diff = -diff;
			}
			mask = 4;
			while (mask) {
				if (diff >= step) {
					nibble |= mask;
					diff -= step;
					vpdiff += step;
				}

				step >>= 1;
				mask >>= 1;
			}

			if (nibble & 8) {
				prev -= vpdiff;
			}
			else {
				prev += vpdiff;
			}

			prev = CLAMP(prev, -32768, 32767);

			step_idx += _ima_adpcm_index_table[nibble];
			step_idx = CLAMP(step_idx, 0, 88);

			if (i & 1) {
				*out |= nibble << 4;
				out++;
			}
			else {
				*out = nibble;
			}
		}
	}

	static void _compress_qoa(
		const Vector<float>& p_data, Vector<uint8_t>& dst_data, qoa_desc* p_desc)
	{
		uint32_t frames_len = (p_desc->samples + QOA_FRAME_LEN - 1) / QOA_FRAME_LEN *
							  (QOA_LMS_LEN * 4 * p_desc->channels + 8);
		uint32_t slices_len =
			(p_desc->samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN * 8 * p_desc->channels;
		dst_data.resize(8 + frames_len + slices_len);

		for (uint32_t c = 0; c < p_desc->channels; c++) {
			memset(p_desc->lms[c].history, 0, sizeof(p_desc->lms[c].history));
			memset(p_desc->lms[c].weights, 0, sizeof(p_desc->lms[c].weights));
			p_desc->lms[c].weights[2] = -(1 << 13);
			p_desc->lms[c].weights[3] = (1 << 14);
		}

		TightLocalVector<int16_t> data16;
		data16.resize(QOA_FRAME_LEN * p_desc->channels);

		uint8_t* dst_ptr = dst_data.ptrw();
		dst_ptr += qoa_encode_header(p_desc, dst_data.ptrw());

		uint32_t frame_len = QOA_FRAME_LEN;
		for (uint32_t s = 0; s < p_desc->samples; s += frame_len) {
			frame_len = MIN(frame_len, p_desc->samples - s);
			for (uint32_t i = 0; i < frame_len * p_desc->channels; i++) {
				data16[i] = CLAMP(p_data[s * p_desc->channels + i] * 32767.0, -32768, 32767);
			}
			dst_ptr += qoa_encode_frame(data16.ptr(), p_desc, frame_len, dst_ptr);
		}
	}
};
// --- End scene/resources/audio_stream_wav.h ---
// --- End servers/audio/audio_stream.h ---
// --- End servers/audio/audio_server.h ---

// --- Folded from main/main.h ---
#include "core/os/mutex.h"
#include "core/typedefs.h"

template <typename T>
class Vector;

class Main {
	enum CLIOptionAvailability {
		CLI_OPTION_AVAILABILITY_EDITOR,
		CLI_OPTION_AVAILABILITY_TEMPLATE_DEBUG,
		CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE,
		CLI_OPTION_AVAILABILITY_TEMPLATE_UNSAFE,
		CLI_OPTION_AVAILABILITY_HIDDEN,
	};

	static void print_header(bool p_rich);
	static void print_help_copyright(const char *p_notice);
	static void print_help_title(const char *p_title);
	static void print_help_option(const char *p_option, const char *p_description, CLIOptionAvailability p_availability = CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE);
	static String format_help_option(const char *p_option);
	static void print_help(const char *p_binary);
	static uint64_t last_ticks;
	static uint32_t hide_print_fps_attempts;
	static uint32_t frames;
	static uint32_t frame;
	static bool force_redraw_requested;
	static int iterating;

public:
	static bool is_cmdline_tool();
#ifdef TOOLS_ENABLED
	enum CLIScope {
		CLI_SCOPE_TOOL, // Editor and project manager.
		CLI_SCOPE_PROJECT,
	};
	static const Vector<String> &get_forwardable_cli_arguments(CLIScope p_scope);
#endif

	static int test_entrypoint(int argc, char *argv[], bool &tests_need_run);
	static Error setup(const char *execpath, int argc, char *argv[], bool p_second_phase = true);
	static Error setup2(bool p_show_boot_logo = true); // The thread calling setup2() will effectively become the main thread.
	static String get_locale_override();
	static void setup_boot_logo();
	static Error test_setup();
	static void test_cleanup();
	static int start();

	static bool iteration();
	static void force_redraw();

	static bool is_iterating();

	static void cleanup(bool p_force = false);
};

// Test main override is for the testing behavior.
#define TEST_MAIN_OVERRIDE \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

#define TEST_MAIN_PARAM_OVERRIDE(argc, argv) \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

// --- Folded from core/register_core_types.h ---
void register_core_types();
void register_core_settings();
void register_core_extensions();
void register_early_core_singletons();
void register_core_singletons();
void unregister_core_types();
void unregister_core_extensions();

// --- Folded from drivers/register_driver_types.h ---
void register_core_driver_types();
void unregister_core_driver_types();

void register_driver_types();
void unregister_driver_types();

// --- Folded from scene/register_scene_types.h ---
void register_scene_types();
void unregister_scene_types();
void register_scene_singletons();

// --- Folded from servers/register_server_types.h ---
void register_server_types();
void unregister_server_types();

// --- Folded from servers/movie_writer/movie_writer.h ---
#include "core/io/net_socket.h"
#include "core/string/ustring.h"


class MovieWriter
{
	uint64_t fps = 0;
	uint64_t mix_rate = 0;
	uint32_t audio_channels = 0;

	// The output resolution, which can differ from the window size.
	// Used as a base for resizing all subsequent frames if their resolution differs.
	Vector2i movie_size;

	float cpu_time = 0.0f;
	float gpu_time = 0.0f;
	uint64_t encoding_time_usec = 0;

	String project_name;

	LocalVector<int32_t> audio_mix_buffer;

	enum
	{
		MAX_WRITERS = 8
	};

	static MovieWriter* writers[];
	static uint32_t writer_count;

protected:
	virtual uint32_t get_audio_mix_rate() const;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data);
	virtual void write_end();


public:
	virtual bool handles_file(const String& p_path) const;
	virtual void get_supported_extensions(List<String>* r_extensions) const;

	static void add_writer(MovieWriter* p_writer);
	static MovieWriter* find_writer_for_file(const String& p_file);

	void begin(const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	void add_frame();

	static void set_extensions_hint();

	void end();
};

// --- Folded from servers/movie_writer/movie_writer_pngwav.h ---


class MovieWriterPNGWAV : public MovieWriter {

	enum {
		MAX_TRAILING_ZEROS = 8 // more than 10 days at 60fps, no hard drive can put up with this anyway :)
	};

	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;

	uint32_t audio_block_size = 0;

	Ref<FileAccess> f_wav;
	uint32_t wav_data_size_pos = 0;

	String zeros_str(uint32_t p_index);

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String> *r_extensions) const override;

	virtual Error write_begin(const Size2i &p_movie_size, uint32_t p_fps, const String &p_base_path) override;
	virtual Error write_frame(const Ref<Image> &p_image, const int32_t *p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String &p_path) const override;

public:
	MovieWriterPNGWAV();
};

// --- End servers/movie_writer/movie_writer_pngwav.h ---

// --- Folded from platform/register_platform_apis.h ---
void register_platform_apis();
void unregister_platform_apis();

// --- Folded from platform/android/api/api.h ---
void register_android_api();
void unregister_android_api();
// --- End platform/android/api/api.h ---

// --- Folded from platform/ios/api/api.h ---
#if defined(IOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_ios_api();
void unregister_ios_api();
// --- End platform/ios/api/api.h ---

// --- Folded from platform/visionos/api/api.h ---
#if defined(VISIONOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_visionos_api();
void unregister_visionos_api();
// --- End platform/visionos/api/api.h ---

// --- Folded from platform/web/api/api.h ---
void register_web_api();
void unregister_web_api();
// --- End platform/web/api/api.h ---
// --- End platform/register_platform_apis.h ---

// --- Folded from platform/android/api/java_class_wrapper.h ---
#include "core/io/ip.h"

#ifdef ANDROID_ENABLED
#include <android/log.h>
#include <jni.h>
#include "core/io/ip.h"
#endif

#ifdef ANDROID_ENABLED
class JavaObject;
#endif

class JavaClass : public RefCounted
{
#ifdef ANDROID_ENABLED
	enum ArgumentType
	{
		ARG_TYPE_VOID,
		ARG_TYPE_BOOLEAN,
		ARG_TYPE_BYTE,
		ARG_TYPE_CHAR,
		ARG_TYPE_SHORT,
		ARG_TYPE_INT,
		ARG_TYPE_LONG,
		ARG_TYPE_FLOAT,
		ARG_TYPE_DOUBLE,
		ARG_TYPE_STRING, // special case
		ARG_TYPE_CHARSEQUENCE,
		ARG_TYPE_CALLABLE,
		ARG_TYPE_CLASS,
		ARG_ARRAY_BIT = 1 << 16,
		ARG_NUMBER_CLASS_BIT = 1 << 17,
		ARG_TYPE_MASK = (1 << 16) - 1
	};

	RBMap<String, Variant> constant_map;

	struct MethodInfo
	{
		bool _public = false;
		bool _static = false;
		bool _constructor = false;
		Vector<uint32_t> param_types;
		Vector<String> param_sigs;
		uint32_t return_type = 0;
		jmethodID method;
	};

	_FORCE_INLINE_ static void _convert_to_variant_type(
		int p_sig, Variant::Type& r_type, float& likelihood)
	{
		likelihood = 1.0;
		r_type = Variant::NIL;

		switch (p_sig) {
		case ARG_TYPE_VOID:
			r_type = Variant::NIL;
			break;
		case ARG_TYPE_BOOLEAN | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_BOOLEAN:
			r_type = Variant::BOOL;
			break;
		case ARG_TYPE_BYTE | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_BYTE:
			r_type = Variant::INT;
			likelihood = 0.1;
			break;
		case ARG_TYPE_CHAR | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_CHAR:
			r_type = Variant::INT;
			likelihood = 0.2;
			break;
		case ARG_TYPE_SHORT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_SHORT:
			r_type = Variant::INT;
			likelihood = 0.3;
			break;
		case ARG_TYPE_INT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_INT:
			r_type = Variant::INT;
			likelihood = 1.0;
			break;
		case ARG_TYPE_LONG | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_LONG:
			r_type = Variant::INT;
			likelihood = 0.5;
			break;
		case ARG_TYPE_FLOAT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_FLOAT:
			r_type = Variant::FLOAT;
			likelihood = 1.0;
			break;
		case ARG_TYPE_DOUBLE | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_DOUBLE:
			r_type = Variant::FLOAT;
			likelihood = 0.5;
			break;
		case ARG_TYPE_STRING:
		case ARG_TYPE_CHARSEQUENCE:
			r_type = Variant::STRING;
			break;
		case ARG_TYPE_CALLABLE:
			r_type = Variant::CALLABLE;
			break;
		case ARG_TYPE_CLASS:
			r_type = Variant::OBJECT;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_VOID:
			r_type = Variant::NIL;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN:
			r_type = Variant::ARRAY;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_BYTE:
			r_type = Variant::PACKED_BYTE_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHAR:
			r_type = Variant::PACKED_BYTE_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_SHORT:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 0.3;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_INT:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_LONG:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_FLOAT:
			r_type = Variant::PACKED_FLOAT32_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE:
			r_type = Variant::PACKED_FLOAT32_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_STRING:
		case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE:
			r_type = Variant::PACKED_STRING_ARRAY;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_CLASS:
		case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE:
			r_type = Variant::ARRAY;
			break;
		}
	}

	_FORCE_INLINE_ static bool _convert_object_to_variant(
		JNIEnv* env, jobject obj, Variant& var, uint32_t p_sig);

	bool _call_method(JavaObject* p_instance, const String& p_method, const Variant** p_args,
		int p_argcount, Callable::CallError& r_error, Variant& ret);

	friend class JavaClassWrapper;
	friend class JavaObject;
	String java_class_name;
	String java_constructor_name;
	HashMap<String, List<MethodInfo>> methods;
	jclass _class;
	bool is_interface;
#endif

public:

	String get_java_class_name() const;
	Ref<JavaClass> get_java_parent_class() const;
	bool has_java_method(const String& p_method) const;

#ifdef ANDROID_ENABLED
	virtual String _to_string() override;
#endif

	JavaClass();
	~JavaClass();
};

class JavaObject : public RefCounted
{
#ifdef ANDROID_ENABLED
	Ref<JavaClass> base_class;
	friend class JavaClass;

	jobject instance = nullptr;
#endif

protected:

public:

	Ref<JavaClass> get_java_class() const;
	bool has_java_method(const String& p_method) const;

#ifdef ANDROID_ENABLED
	virtual String _to_string() override;

	jobject get_instance() { return instance; }

	JavaObject();
	JavaObject(const Ref<JavaClass>& p_base, jobject p_instance);
	~JavaObject();
#endif
};

class JavaClassWrapper final
{
	friend class JavaClass;

public:
	struct Data
	{
#ifdef ANDROID_ENABLED
		RBMap<String, Ref<JavaClass>> class_cache;
		jmethodID Class_getConstructors = nullptr;
		jmethodID Class_getDeclaredMethods = nullptr;
		jmethodID Class_getFields = nullptr;
		jmethodID Class_getInterfaces = nullptr;
		jmethodID Class_getName = nullptr;
		jmethodID Class_getSuperclass = nullptr;
		jmethodID Class_isInterface = nullptr;
		jmethodID Constructor_getParameterTypes = nullptr;
		jmethodID Constructor_getModifiers = nullptr;
		jmethodID Method_getParameterTypes = nullptr;
		jmethodID Method_getReturnType = nullptr;
		jmethodID Method_getModifiers = nullptr;
		jmethodID Method_getName = nullptr;
		jmethodID Field_getName = nullptr;
		jmethodID Field_getModifiers = nullptr;
		jmethodID Field_get = nullptr;
		jmethodID Boolean_booleanValue = nullptr;
		jmethodID Byte_byteValue = nullptr;
		jmethodID Character_characterValue = nullptr;
		jmethodID Short_shortValue = nullptr;
		jmethodID Integer_integerValue = nullptr;
		jmethodID Long_longValue = nullptr;
		jmethodID Float_floatValue = nullptr;
		jmethodID Double_doubleValue = nullptr;

		jclass proxy_class = nullptr;
		jmethodID Proxy_isProxyClass = nullptr;

		jclass android_runtime_class = nullptr;
		jmethodID ARP_create_proxy_from_godot_callable = nullptr;
		jmethodID ARP_create_proxy_from_godot_object_id = nullptr;
#endif
		Ref<JavaObject> exception;
	};

	static inline Data* data = nullptr;

private:
#ifdef ANDROID_ENABLED
	static bool _is_proxy_class(JNIEnv* env, jclass p_class);
	static bool _get_type_sig(JNIEnv* env, jobject obj, uint32_t& sig, String& strsig);
	static bool _wrap_class_components(JNIEnv* p_env, const Ref<JavaClass>& p_java_class,
		jclass p_class, bool p_allow_non_public_methods_access);
#endif

	static Ref<JavaClass> _wrap(
		const String& p_class, bool p_allow_non_public_methods_access = false);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static Ref<JavaClass> wrap(const String& p_class) { return _wrap(p_class, false); }

	static Ref<JavaObject> get_exception() { return data ? data->exception : Ref<JavaObject>(); }

#ifdef ANDROID_ENABLED
	static Ref<JavaClass> wrap_jclass(
		jclass p_class, bool p_allow_non_public_methods_access = false);
	static Ref<JavaObject> create_sam_callback(
		const String& p_sam_interface, const Callable& p_callable);
	static Ref<JavaObject> create_proxy(
		const Object* p_object, const PackedStringArray& p_interfaces);
#endif

	JavaClassWrapper() = delete;
	JavaClassWrapper(const JavaClassWrapper&) = delete;
	JavaClassWrapper& operator=(const JavaClassWrapper&) = delete;
	~JavaClassWrapper() = delete;
};
// --- End platform/android/api/java_class_wrapper.h ---

// --- Folded from modules/register_module_types.h ---
enum ModuleInitializationLevel {
	MODULE_INITIALIZATION_LEVEL_CORE = 0,
	MODULE_INITIALIZATION_LEVEL_SERVERS = 1,
	MODULE_INITIALIZATION_LEVEL_SCENE = 2,
	MODULE_INITIALIZATION_LEVEL_EDITOR = 3
};

void initialize_modules(ModuleInitializationLevel p_level);
void uninitialize_modules(ModuleInitializationLevel p_level);

#include "core/version_generated.gen.h"

#ifdef MODULE_OPENXR_ENABLED
#define MODULE_OPENXR_HAS_PREREGISTER
#endif
#ifdef MODULE_GLSLANG_ENABLED
#define MODULE_GLSLANG_HAS_PREREGISTER
#endif

inline void initialize_freetype_module(ModuleInitializationLevel p_level) {}
inline void uninitialize_freetype_module(ModuleInitializationLevel p_level) {}
inline void initialize_msdfgen_module(ModuleInitializationLevel p_level) {}
inline void uninitialize_msdfgen_module(ModuleInitializationLevel p_level) {}

void initialize_text_server_adv_module(ModuleInitializationLevel p_level);
void uninitialize_text_server_adv_module(ModuleInitializationLevel p_level);
// --- End modules/register_module_types.h ---

// --- Folded from tests/test_main.h ---
int test_main(int argc, char *argv[]);
// --- End tests/test_main.h ---
// --- End main/main.h ---

// --- Folded from scene/resources/texture.h ---
#include "core/io/net_socket.h"
#include "core/io/ip.h"
// --- Folded from servers/rendering/rendering_server_enums.h ---
#include <cstdint>

namespace RenderingServerEnums
{

enum
{
	NO_INDEX_ARRAY = -1,
	ARRAY_WEIGHTS_SIZE = 4,
	CANVAS_ITEM_Z_MIN = -4096,
	CANVAS_ITEM_Z_MAX = 4096,
	CANVAS_LAYER_MIN = -2147483648,
	CANVAS_LAYER_MAX = 2147483647,
	MAX_GLOW_LEVELS = 7,
	MAX_CURSORS = 8,
	MAX_2D_DIRECTIONAL_LIGHTS = 8,
	MAX_MESH_SURFACES = 256,
};

/* TEXTURE API */

enum TextureType
{
	TEXTURE_TYPE_2D,
	TEXTURE_TYPE_LAYERED,
	TEXTURE_TYPE_3D,
};

enum TextureLayeredType
{
	TEXTURE_LAYERED_2D_ARRAY,
	TEXTURE_LAYERED_CUBEMAP,
	TEXTURE_LAYERED_CUBEMAP_ARRAY,
};

enum CubeMapLayer
{
	CUBEMAP_LAYER_LEFT,
	CUBEMAP_LAYER_RIGHT,
	CUBEMAP_LAYER_BOTTOM,
	CUBEMAP_LAYER_TOP,
	CUBEMAP_LAYER_FRONT,
	CUBEMAP_LAYER_BACK,
};

enum TextureDrawableFormat
{
	TEXTURE_DRAWABLE_FORMAT_RGBA8,
	TEXTURE_DRAWABLE_FORMAT_RGBA8_SRGB, // Use this if you want to read the result from both 2D
										// (non-hdr) and 3D.
	TEXTURE_DRAWABLE_FORMAT_RGBAH,
	TEXTURE_DRAWABLE_FORMAT_RGBAF,
};

enum TextureDetectRoughnessChannel
{
	TEXTURE_DETECT_ROUGHNESS_R,
	TEXTURE_DETECT_ROUGHNESS_G,
	TEXTURE_DETECT_ROUGHNESS_B,
	TEXTURE_DETECT_ROUGHNESS_A,
	TEXTURE_DETECT_ROUGHNESS_GRAY,
};

/* PIPELINES API */

enum PipelineSource
{
	PIPELINE_SOURCE_CANVAS,
	PIPELINE_SOURCE_MESH,
	PIPELINE_SOURCE_SURFACE,
	PIPELINE_SOURCE_DRAW,
	PIPELINE_SOURCE_SPECIALIZATION,
	PIPELINE_SOURCE_MAX,
};

/* SHADER API */

enum ShaderMode
{
	SHADER_SPATIAL,
	SHADER_CANVAS_ITEM,
	SHADER_PARTICLES,
	SHADER_SKY,
	SHADER_FOG,
	SHADER_TEXTURE_BLIT,
	SHADER_MAX,
};

enum CullMode
{
	CULL_MODE_DISABLED,
	CULL_MODE_FRONT,
	CULL_MODE_BACK,
};

/* COMMON MATERIAL API */

enum
{
	MATERIAL_RENDER_PRIORITY_MIN = -128,
	MATERIAL_RENDER_PRIORITY_MAX = 127,
};

/* MESH API */

enum ArrayType
{
	ARRAY_VERTEX = 0,  // RG32F (2D), RGB32F, RGBA16 (compressed)
	ARRAY_NORMAL = 1,  // RG16
	ARRAY_TANGENT = 2, // BA16 (with normal) or A16 (with vertex, when compressed)
	ARRAY_COLOR = 3,   // RGBA8
	ARRAY_TEX_UV = 4,  // RG32F or RG16
	ARRAY_TEX_UV2 = 5, // RG32F or RG16
	ARRAY_CUSTOM0 = 6, // Depends on ArrayCustomFormat.
	ARRAY_CUSTOM1 = 7,
	ARRAY_CUSTOM2 = 8,
	ARRAY_CUSTOM3 = 9,
	ARRAY_BONES = 10,	// RGBA16UI (x2 if 8 weights)
	ARRAY_WEIGHTS = 11, // RGBA16UNORM (x2 if 8 weights)
	ARRAY_INDEX = 12,	// 16 or 32 bits depending on length > 0xFFFF.
	ARRAY_MAX = 13,
};

enum
{
	ARRAY_CUSTOM_COUNT = ARRAY_BONES - ARRAY_CUSTOM0
};

enum ArrayCustomFormat
{
	ARRAY_CUSTOM_RGBA8_UNORM,
	ARRAY_CUSTOM_RGBA8_SNORM,
	ARRAY_CUSTOM_RG_HALF,
	ARRAY_CUSTOM_RGBA_HALF,
	ARRAY_CUSTOM_R_FLOAT,
	ARRAY_CUSTOM_RG_FLOAT,
	ARRAY_CUSTOM_RGB_FLOAT,
	ARRAY_CUSTOM_RGBA_FLOAT,
	ARRAY_CUSTOM_MAX,
};

enum ArrayFormat : uint64_t
{
	/* ARRAY FORMAT FLAGS */
	ARRAY_FORMAT_VERTEX = 1 << ARRAY_VERTEX,
	ARRAY_FORMAT_NORMAL = 1 << ARRAY_NORMAL,
	ARRAY_FORMAT_TANGENT = 1 << ARRAY_TANGENT,
	ARRAY_FORMAT_COLOR = 1 << ARRAY_COLOR,
	ARRAY_FORMAT_TEX_UV = 1 << ARRAY_TEX_UV,
	ARRAY_FORMAT_TEX_UV2 = 1 << ARRAY_TEX_UV2,
	ARRAY_FORMAT_CUSTOM0 = 1 << ARRAY_CUSTOM0,
	ARRAY_FORMAT_CUSTOM1 = 1 << ARRAY_CUSTOM1,
	ARRAY_FORMAT_CUSTOM2 = 1 << ARRAY_CUSTOM2,
	ARRAY_FORMAT_CUSTOM3 = 1 << ARRAY_CUSTOM3,
	ARRAY_FORMAT_BONES = 1 << ARRAY_BONES,
	ARRAY_FORMAT_WEIGHTS = 1 << ARRAY_WEIGHTS,
	ARRAY_FORMAT_INDEX = 1 << ARRAY_INDEX,

	ARRAY_FORMAT_BLEND_SHAPE_MASK =
		ARRAY_FORMAT_VERTEX | ARRAY_FORMAT_NORMAL | ARRAY_FORMAT_TANGENT,

	ARRAY_FORMAT_CUSTOM_BASE = (ARRAY_INDEX + 1),
	ARRAY_FORMAT_CUSTOM_BITS = 3,
	ARRAY_FORMAT_CUSTOM_MASK = 0x7,
	ARRAY_FORMAT_CUSTOM0_SHIFT = (ARRAY_FORMAT_CUSTOM_BASE + 0),
	ARRAY_FORMAT_CUSTOM1_SHIFT = (ARRAY_FORMAT_CUSTOM_BASE + ARRAY_FORMAT_CUSTOM_BITS),
	ARRAY_FORMAT_CUSTOM2_SHIFT = (ARRAY_FORMAT_CUSTOM_BASE + ARRAY_FORMAT_CUSTOM_BITS * 2),
	ARRAY_FORMAT_CUSTOM3_SHIFT = (ARRAY_FORMAT_CUSTOM_BASE + ARRAY_FORMAT_CUSTOM_BITS * 3),

	ARRAY_COMPRESS_FLAGS_BASE = (ARRAY_INDEX + 1 + 12),

	ARRAY_FLAG_USE_2D_VERTICES = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 0),
	ARRAY_FLAG_USE_DYNAMIC_UPDATE = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 1),
	ARRAY_FLAG_USE_8_BONE_WEIGHTS = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 2),

	ARRAY_FLAG_USES_EMPTY_VERTEX_ARRAY = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 3),

	ARRAY_FLAG_COMPRESS_ATTRIBUTES = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 4),

	// When set, the surface's vertex, attribute, skin, and index buffers are created with the
	// `BUFFER_CREATION_AS_STORAGE_BIT` usage bit so they can be bound as storage buffers in
	// compute shaders. Required to write into them from a compute pipeline through the RIDs
	// returned by `mesh_surface_get_vertex_buffer_rd_rid` and friends.
	ARRAY_FLAG_USE_STORAGE_BUFFER = 1 << (ARRAY_COMPRESS_FLAGS_BASE + 5),
	// We leave enough room for up to 4 more compression flags.

	ARRAY_FLAG_FORMAT_VERSION_BASE = ARRAY_COMPRESS_FLAGS_BASE + 10,
	ARRAY_FLAG_FORMAT_VERSION_SHIFT = ARRAY_FLAG_FORMAT_VERSION_BASE,
	// When changes are made to the mesh format, add a new version and use it for the
	// CURRENT_VERSION.
	ARRAY_FLAG_FORMAT_VERSION_1 = 0,
	ARRAY_FLAG_FORMAT_VERSION_2 = 1ULL << ARRAY_FLAG_FORMAT_VERSION_SHIFT,
	ARRAY_FLAG_FORMAT_CURRENT_VERSION = ARRAY_FLAG_FORMAT_VERSION_2,
	ARRAY_FLAG_FORMAT_VERSION_MASK = 0xFF, // 8 bits version
};

static_assert(sizeof(ArrayFormat) == 8, "ArrayFormat should be 64 bits long.");

enum PrimitiveType
{
	PRIMITIVE_POINTS,
	PRIMITIVE_LINES,
	PRIMITIVE_LINE_STRIP,
	PRIMITIVE_TRIANGLES,
	PRIMITIVE_TRIANGLE_STRIP,
	PRIMITIVE_MAX,
};

enum BlendShapeMode
{
	BLEND_SHAPE_MODE_NORMALIZED,
	BLEND_SHAPE_MODE_RELATIVE,
};

/* MULTIMESH API */

enum MultimeshTransformFormat
{
	MULTIMESH_TRANSFORM_2D,
	MULTIMESH_TRANSFORM_3D,
};

enum MultimeshPhysicsInterpolationQuality
{
	MULTIMESH_INTERP_QUALITY_FAST,
	MULTIMESH_INTERP_QUALITY_HIGH,
};

/* LIGHT API */

enum LightType
{
	LIGHT_DIRECTIONAL,
	LIGHT_OMNI,
	LIGHT_SPOT,
	LIGHT_AREA,
};

enum LightParam
{
	LIGHT_PARAM_ENERGY,
	LIGHT_PARAM_INDIRECT_ENERGY,
	LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY,
	LIGHT_PARAM_SPECULAR,
	LIGHT_PARAM_RANGE,
	LIGHT_PARAM_SIZE,
	LIGHT_PARAM_ATTENUATION,
	LIGHT_PARAM_SPOT_ANGLE,
	LIGHT_PARAM_SPOT_ATTENUATION,
	LIGHT_PARAM_SHADOW_MAX_DISTANCE,
	LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
	LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
	LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
	LIGHT_PARAM_SHADOW_FADE_START,
	LIGHT_PARAM_SHADOW_NORMAL_BIAS,
	LIGHT_PARAM_SHADOW_BIAS,
	LIGHT_PARAM_SHADOW_PANCAKE_SIZE,
	LIGHT_PARAM_SHADOW_OPACITY,
	LIGHT_PARAM_SHADOW_BLUR,
	LIGHT_PARAM_TRANSMITTANCE_BIAS,
	LIGHT_PARAM_INTENSITY,
	LIGHT_PARAM_MAX,
};

enum LightBakeMode
{
	LIGHT_BAKE_DISABLED,
	LIGHT_BAKE_STATIC,
	LIGHT_BAKE_DYNAMIC,
};

enum LightOmniShadowMode
{
	LIGHT_OMNI_SHADOW_DUAL_PARABOLOID,
	LIGHT_OMNI_SHADOW_CUBE,
};

enum LightDirectionalShadowMode
{
	LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL,
	LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
	LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS,
};

enum LightDirectionalSkyMode
{
	LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY,
	LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_ONLY,
	LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY,
};

enum ShadowQuality
{
	SHADOW_QUALITY_HARD,
	SHADOW_QUALITY_SOFT_VERY_LOW,
	SHADOW_QUALITY_SOFT_LOW,
	SHADOW_QUALITY_SOFT_MEDIUM,
	SHADOW_QUALITY_SOFT_HIGH,
	SHADOW_QUALITY_SOFT_ULTRA,
	SHADOW_QUALITY_MAX,
};

enum LightProjectorFilter
{
	LIGHT_PROJECTOR_FILTER_NEAREST,
	LIGHT_PROJECTOR_FILTER_LINEAR,
	LIGHT_PROJECTOR_FILTER_NEAREST_MIPMAPS,
	LIGHT_PROJECTOR_FILTER_LINEAR_MIPMAPS,
	LIGHT_PROJECTOR_FILTER_NEAREST_MIPMAPS_ANISOTROPIC,
	LIGHT_PROJECTOR_FILTER_LINEAR_MIPMAPS_ANISOTROPIC,
};

/* REFLECTION PROBE API */

enum ReflectionProbeUpdateMode
{
	REFLECTION_PROBE_UPDATE_ONCE,
	REFLECTION_PROBE_UPDATE_ALWAYS,
};

enum ReflectionProbeAmbientMode
{
	REFLECTION_PROBE_AMBIENT_DISABLED,
	REFLECTION_PROBE_AMBIENT_ENVIRONMENT,
	REFLECTION_PROBE_AMBIENT_COLOR,
};

/* DECAL API */

enum DecalTexture
{
	DECAL_TEXTURE_ALBEDO,
	DECAL_TEXTURE_NORMAL,
	DECAL_TEXTURE_ORM,
	DECAL_TEXTURE_EMISSION,
	DECAL_TEXTURE_MAX,
};

enum DecalFilter
{
	DECAL_FILTER_NEAREST,
	DECAL_FILTER_LINEAR,
	DECAL_FILTER_NEAREST_MIPMAPS,
	DECAL_FILTER_LINEAR_MIPMAPS,
	DECAL_FILTER_NEAREST_MIPMAPS_ANISOTROPIC,
	DECAL_FILTER_LINEAR_MIPMAPS_ANISOTROPIC,
};

/* VOXEL GI API */

enum VoxelGIQuality
{
	VOXEL_GI_QUALITY_LOW,
	VOXEL_GI_QUALITY_HIGH,
};

/* LIGHTMAP API */

enum ShadowmaskMode
{
	SHADOWMASK_MODE_NONE,
	SHADOWMASK_MODE_REPLACE,
	SHADOWMASK_MODE_OVERLAY,
	SHADOWMASK_MODE_ONLY,
};

/* PARTICLES API */

enum ParticlesMode
{
	PARTICLES_MODE_2D,
	PARTICLES_MODE_3D,
};

enum ParticlesTransformAlign
{
	PARTICLES_TRANSFORM_ALIGN_DISABLED,
	PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD,
	PARTICLES_TRANSFORM_ALIGN_Y_TO_VELOCITY,
	PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY,
	PARTICLES_TRANSFORM_ALIGN_LOCAL_BILLBOARD,
};

enum ParticlesEmitFlags
{
	PARTICLES_EMIT_FLAG_POSITION = 1,
	PARTICLES_EMIT_FLAG_ROTATION_SCALE = 2,
	PARTICLES_EMIT_FLAG_VELOCITY = 4,
	PARTICLES_EMIT_FLAG_COLOR = 8,
	PARTICLES_EMIT_FLAG_CUSTOM = 16,
};

enum ParticlesDrawOrder
{
	PARTICLES_DRAW_ORDER_INDEX,
	PARTICLES_DRAW_ORDER_LIFETIME,
	PARTICLES_DRAW_ORDER_REVERSE_LIFETIME,
	PARTICLES_DRAW_ORDER_VIEW_DEPTH,
};

enum ParticlesTransformAlignCustomSrc
{
	PARTICLES_ALIGN_CHANNEL_FILTER_DISABLED,
	PARTICLES_ALIGN_CHANNEL_FILTER_X,
	PARTICLES_ALIGN_CHANNEL_FILTER_Y,
	PARTICLES_ALIGN_CHANNEL_FILTER_Z,
	PARTICLES_ALIGN_CHANNEL_FILTER_W,
	PARTICLES_ALIGN_CHANNEL_FILTER_MAX,
};

enum ParticlesTransformAlignAxis
{
	PARTICLES_ALIGN_AXIS_X,
	PARTICLES_ALIGN_AXIS_Y,
	PARTICLES_ALIGN_AXIS_MAX,
};

/* PARTICLES COLLISION API */

enum ParticlesCollisionType
{
	PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT,
	PARTICLES_COLLISION_TYPE_BOX_ATTRACT,
	PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT,
	PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE,
	PARTICLES_COLLISION_TYPE_BOX_COLLIDE,
	PARTICLES_COLLISION_TYPE_SDF_COLLIDE,
	PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE,
};

enum ParticlesCollisionHeightfieldResolution
{ // Longest axis resolution.
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_256,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_512,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_1024,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_2048,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_4096,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_8192,
	PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_MAX,
};

/* FOG VOLUME API */

enum FogVolumeShape
{
	FOG_VOLUME_SHAPE_ELLIPSOID,
	FOG_VOLUME_SHAPE_CONE,
	FOG_VOLUME_SHAPE_CYLINDER,
	FOG_VOLUME_SHAPE_BOX,
	FOG_VOLUME_SHAPE_WORLD,
	FOG_VOLUME_SHAPE_MAX,
};

/* VIEWPORT API */

enum CanvasItemTextureFilter
{
	CANVAS_ITEM_TEXTURE_FILTER_DEFAULT, // Uses canvas item setting for draw command, uses global
										// setting for canvas item.
	CANVAS_ITEM_TEXTURE_FILTER_NEAREST,
	CANVAS_ITEM_TEXTURE_FILTER_LINEAR,
	CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS,
	CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
	CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC,
	CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC,
	CANVAS_ITEM_TEXTURE_FILTER_MAX,
};

enum CanvasItemTextureRepeat
{
	CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT, // Uses canvas item setting for draw command, uses global
										// setting for canvas item.
	CANVAS_ITEM_TEXTURE_REPEAT_DISABLED,
	CANVAS_ITEM_TEXTURE_REPEAT_ENABLED,
	CANVAS_ITEM_TEXTURE_REPEAT_MIRROR,
	CANVAS_ITEM_TEXTURE_REPEAT_MAX,
};

enum ViewportScaling3DMode
{
	VIEWPORT_SCALING_3D_MODE_BILINEAR,
	VIEWPORT_SCALING_3D_MODE_FSR,
	VIEWPORT_SCALING_3D_MODE_FSR2,
	VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL,
	VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL,
	VIEWPORT_SCALING_3D_MODE_NEAREST,
	VIEWPORT_SCALING_3D_MODE_MAX,
	VIEWPORT_SCALING_3D_MODE_OFF = 255, // for internal use only
};

enum ViewportScaling3DType
{
	VIEWPORT_SCALING_3D_TYPE_NONE,
	VIEWPORT_SCALING_3D_TYPE_TEMPORAL,
	VIEWPORT_SCALING_3D_TYPE_SPATIAL,
	VIEWPORT_SCALING_3D_TYPE_MAX,
};

inline ViewportScaling3DType scaling_3d_mode_type(ViewportScaling3DMode p_mode)
{
	if (p_mode == VIEWPORT_SCALING_3D_MODE_NEAREST || p_mode == VIEWPORT_SCALING_3D_MODE_BILINEAR ||
		p_mode == VIEWPORT_SCALING_3D_MODE_FSR ||
		p_mode == VIEWPORT_SCALING_3D_MODE_METALFX_SPATIAL) {
		return VIEWPORT_SCALING_3D_TYPE_SPATIAL;
	}
	else if (p_mode == VIEWPORT_SCALING_3D_MODE_FSR2 ||
			   p_mode == VIEWPORT_SCALING_3D_MODE_METALFX_TEMPORAL) {
		return VIEWPORT_SCALING_3D_TYPE_TEMPORAL;
	}
	return VIEWPORT_SCALING_3D_TYPE_NONE;
}

enum ViewportAnisotropicFiltering
{
	VIEWPORT_ANISOTROPY_DISABLED,
	VIEWPORT_ANISOTROPY_2X,
	VIEWPORT_ANISOTROPY_4X,
	VIEWPORT_ANISOTROPY_8X,
	VIEWPORT_ANISOTROPY_16X,
	VIEWPORT_ANISOTROPY_MAX,
};

enum ViewportUpdateMode
{
	VIEWPORT_UPDATE_DISABLED,
	VIEWPORT_UPDATE_ONCE,		  // Then goes to disabled, must be manually updated.
	VIEWPORT_UPDATE_WHEN_VISIBLE, // Default
	VIEWPORT_UPDATE_WHEN_PARENT_VISIBLE,
	VIEWPORT_UPDATE_ALWAYS,
};

enum ViewportClearMode
{
	VIEWPORT_CLEAR_ALWAYS,
	VIEWPORT_CLEAR_NEVER,
	VIEWPORT_CLEAR_ONLY_NEXT_FRAME,
};

enum ViewportEnvironmentMode
{
	VIEWPORT_ENVIRONMENT_DISABLED,
	VIEWPORT_ENVIRONMENT_ENABLED,
	VIEWPORT_ENVIRONMENT_INHERIT,
	VIEWPORT_ENVIRONMENT_MAX,
};

enum ViewportSDFOversize
{
	VIEWPORT_SDF_OVERSIZE_100_PERCENT,
	VIEWPORT_SDF_OVERSIZE_120_PERCENT,
	VIEWPORT_SDF_OVERSIZE_150_PERCENT,
	VIEWPORT_SDF_OVERSIZE_200_PERCENT,
	VIEWPORT_SDF_OVERSIZE_MAX,
};

enum ViewportSDFScale
{
	VIEWPORT_SDF_SCALE_100_PERCENT,
	VIEWPORT_SDF_SCALE_50_PERCENT,
	VIEWPORT_SDF_SCALE_25_PERCENT,
	VIEWPORT_SDF_SCALE_MAX,
};

enum ViewportMSAA
{
	VIEWPORT_MSAA_DISABLED,
	VIEWPORT_MSAA_2X,
	VIEWPORT_MSAA_4X,
	VIEWPORT_MSAA_8X,
	VIEWPORT_MSAA_MAX,
};

enum ViewportScreenSpaceAA
{
	VIEWPORT_SCREEN_SPACE_AA_DISABLED,
	VIEWPORT_SCREEN_SPACE_AA_FXAA,
	VIEWPORT_SCREEN_SPACE_AA_SMAA,
	VIEWPORT_SCREEN_SPACE_AA_MAX,
};

enum ViewportOcclusionCullingBuildQuality
{
	VIEWPORT_OCCLUSION_BUILD_QUALITY_LOW = 0,
	VIEWPORT_OCCLUSION_BUILD_QUALITY_MEDIUM = 1,
	VIEWPORT_OCCLUSION_BUILD_QUALITY_HIGH = 2,
};

enum ViewportRenderInfo
{
	VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME,
	VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME,
	VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME,
	VIEWPORT_RENDER_INFO_MAX,
};

enum ViewportRenderInfoType
{
	VIEWPORT_RENDER_INFO_TYPE_VISIBLE,
	VIEWPORT_RENDER_INFO_TYPE_SHADOW,
	VIEWPORT_RENDER_INFO_TYPE_CANVAS,
	VIEWPORT_RENDER_INFO_TYPE_MAX,
};

enum ViewportDebugDraw
{
	VIEWPORT_DEBUG_DRAW_DISABLED,
	VIEWPORT_DEBUG_DRAW_UNSHADED,
	VIEWPORT_DEBUG_DRAW_LIGHTING,
	VIEWPORT_DEBUG_DRAW_OVERDRAW,
	VIEWPORT_DEBUG_DRAW_WIREFRAME,
	VIEWPORT_DEBUG_DRAW_NORMAL_BUFFER,
	VIEWPORT_DEBUG_DRAW_VOXEL_GI_ALBEDO,
	VIEWPORT_DEBUG_DRAW_VOXEL_GI_LIGHTING,
	VIEWPORT_DEBUG_DRAW_VOXEL_GI_EMISSION,
	VIEWPORT_DEBUG_DRAW_SHADOW_ATLAS,
	VIEWPORT_DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS,
	VIEWPORT_DEBUG_DRAW_SCENE_LUMINANCE,
	VIEWPORT_DEBUG_DRAW_SSAO,
	VIEWPORT_DEBUG_DRAW_SSIL,
	VIEWPORT_DEBUG_DRAW_PSSM_SPLITS,
	VIEWPORT_DEBUG_DRAW_DECAL_ATLAS,
	VIEWPORT_DEBUG_DRAW_SDFGI,
	VIEWPORT_DEBUG_DRAW_SDFGI_PROBES,
	VIEWPORT_DEBUG_DRAW_GI_BUFFER,
	VIEWPORT_DEBUG_DRAW_DISABLE_LOD,
	VIEWPORT_DEBUG_DRAW_CLUSTER_OMNI_LIGHTS,
	VIEWPORT_DEBUG_DRAW_CLUSTER_SPOT_LIGHTS,
	VIEWPORT_DEBUG_DRAW_CLUSTER_DECALS,
	VIEWPORT_DEBUG_DRAW_CLUSTER_REFLECTION_PROBES,
	VIEWPORT_DEBUG_DRAW_OCCLUDERS,
	VIEWPORT_DEBUG_DRAW_MOTION_VECTORS,
	VIEWPORT_DEBUG_DRAW_INTERNAL_BUFFER,
	VIEWPORT_DEBUG_DRAW_CLUSTER_AREA_LIGHTS,
	VIEWPORT_DEBUG_DRAW_AREA_LIGHT_ATLAS,
};

enum ViewportVRSMode
{
	VIEWPORT_VRS_DISABLED,
	VIEWPORT_VRS_TEXTURE,
	VIEWPORT_VRS_XR,
	VIEWPORT_VRS_MAX,
};

enum ViewportVRSUpdateMode
{
	VIEWPORT_VRS_UPDATE_DISABLED,
	VIEWPORT_VRS_UPDATE_ONCE,
	VIEWPORT_VRS_UPDATE_ALWAYS,
	VIEWPORT_VRS_UPDATE_MAX,
};

/* SKY API */

enum SkyMode
{
	SKY_MODE_AUTOMATIC,
	SKY_MODE_QUALITY,
	SKY_MODE_INCREMENTAL,
	SKY_MODE_REALTIME,
};

/* COMPOSITOR EFFECTS API */

enum CompositorEffectFlags
{
	COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_COLOR = 1,
	COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_DEPTH = 2,
	COMPOSITOR_EFFECT_FLAG_NEEDS_MOTION_VECTORS = 4,
	COMPOSITOR_EFFECT_FLAG_NEEDS_ROUGHNESS = 8,
	COMPOSITOR_EFFECT_FLAG_NEEDS_SEPARATE_SPECULAR = 16,
};

enum CompositorEffectCallbackType
{
	COMPOSITOR_EFFECT_CALLBACK_TYPE_PRE_OPAQUE,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_OPAQUE,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_SKY,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_TRANSPARENT,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_MAX,
	COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY = -1,
};

/* ENVIRONMENT API */

enum EnvironmentBG
{
	ENV_BG_CLEAR_COLOR,
	ENV_BG_COLOR,
	ENV_BG_SKY,
	ENV_BG_CANVAS,
	ENV_BG_KEEP,
	ENV_BG_CAMERA_FEED,
	ENV_BG_MAX,
};

enum EnvironmentAmbientSource
{
	ENV_AMBIENT_SOURCE_BG,
	ENV_AMBIENT_SOURCE_DISABLED,
	ENV_AMBIENT_SOURCE_COLOR,
	ENV_AMBIENT_SOURCE_SKY,
};

enum EnvironmentReflectionSource
{
	ENV_REFLECTION_SOURCE_BG,
	ENV_REFLECTION_SOURCE_DISABLED,
	ENV_REFLECTION_SOURCE_SKY,
};

enum EnvironmentGlowBlendMode
{
	ENV_GLOW_BLEND_MODE_ADDITIVE,
	ENV_GLOW_BLEND_MODE_SCREEN,
	ENV_GLOW_BLEND_MODE_SOFTLIGHT,
	ENV_GLOW_BLEND_MODE_REPLACE,
	ENV_GLOW_BLEND_MODE_MIX,
};

enum EnvironmentToneMapper
{
	ENV_TONE_MAPPER_LINEAR,
	ENV_TONE_MAPPER_REINHARD,
	ENV_TONE_MAPPER_FILMIC,
	ENV_TONE_MAPPER_ACES,
	ENV_TONE_MAPPER_AGX,
};

enum EnvironmentSSRRoughnessQuality
{
	ENV_SSR_ROUGHNESS_QUALITY_DISABLED,
	ENV_SSR_ROUGHNESS_QUALITY_LOW,
	ENV_SSR_ROUGHNESS_QUALITY_MEDIUM,
	ENV_SSR_ROUGHNESS_QUALITY_HIGH,
};

enum EnvironmentSSAOQuality
{
	ENV_SSAO_QUALITY_VERY_LOW,
	ENV_SSAO_QUALITY_LOW,
	ENV_SSAO_QUALITY_MEDIUM,
	ENV_SSAO_QUALITY_HIGH,
	ENV_SSAO_QUALITY_ULTRA,
};

enum EnvironmentSSILQuality
{
	ENV_SSIL_QUALITY_VERY_LOW,
	ENV_SSIL_QUALITY_LOW,
	ENV_SSIL_QUALITY_MEDIUM,
	ENV_SSIL_QUALITY_HIGH,
	ENV_SSIL_QUALITY_ULTRA,
};

enum EnvironmentSDFGIYScale
{
	ENV_SDFGI_Y_SCALE_50_PERCENT,
	ENV_SDFGI_Y_SCALE_75_PERCENT,
	ENV_SDFGI_Y_SCALE_100_PERCENT,
};

enum EnvironmentSDFGIRayCount
{
	ENV_SDFGI_RAY_COUNT_4,
	ENV_SDFGI_RAY_COUNT_8,
	ENV_SDFGI_RAY_COUNT_16,
	ENV_SDFGI_RAY_COUNT_32,
	ENV_SDFGI_RAY_COUNT_64,
	ENV_SDFGI_RAY_COUNT_96,
	ENV_SDFGI_RAY_COUNT_128,
	ENV_SDFGI_RAY_COUNT_MAX,
};

enum EnvironmentSDFGIFramesToConverge
{
	ENV_SDFGI_CONVERGE_IN_5_FRAMES,
	ENV_SDFGI_CONVERGE_IN_10_FRAMES,
	ENV_SDFGI_CONVERGE_IN_15_FRAMES,
	ENV_SDFGI_CONVERGE_IN_20_FRAMES,
	ENV_SDFGI_CONVERGE_IN_25_FRAMES,
	ENV_SDFGI_CONVERGE_IN_30_FRAMES,
	ENV_SDFGI_CONVERGE_MAX
};

enum EnvironmentSDFGIFramesToUpdateLight
{
	ENV_SDFGI_UPDATE_LIGHT_IN_1_FRAME,
	ENV_SDFGI_UPDATE_LIGHT_IN_2_FRAMES,
	ENV_SDFGI_UPDATE_LIGHT_IN_4_FRAMES,
	ENV_SDFGI_UPDATE_LIGHT_IN_8_FRAMES,
	ENV_SDFGI_UPDATE_LIGHT_IN_16_FRAMES,
	ENV_SDFGI_UPDATE_LIGHT_MAX,
};

enum EnvironmentFogMode
{
	ENV_FOG_MODE_EXPONENTIAL,
	ENV_FOG_MODE_DEPTH,
};

enum SubSurfaceScatteringQuality
{
	SUB_SURFACE_SCATTERING_QUALITY_DISABLED,
	SUB_SURFACE_SCATTERING_QUALITY_LOW,
	SUB_SURFACE_SCATTERING_QUALITY_MEDIUM,
	SUB_SURFACE_SCATTERING_QUALITY_HIGH,
};

/* CAMERA ATTRIBUTES API */

enum DOFBlurQuality
{
	DOF_BLUR_QUALITY_VERY_LOW,
	DOF_BLUR_QUALITY_LOW,
	DOF_BLUR_QUALITY_MEDIUM,
	DOF_BLUR_QUALITY_HIGH,
};

enum DOFBokehShape
{
	DOF_BOKEH_BOX,
	DOF_BOKEH_HEXAGON,
	DOF_BOKEH_CIRCLE,
};

/* INSTANCING API */

enum InstanceType
{
	INSTANCE_NONE,
	INSTANCE_MESH,
	INSTANCE_MULTIMESH,
	INSTANCE_PARTICLES,
	INSTANCE_PARTICLES_COLLISION,
	INSTANCE_LIGHT,
	INSTANCE_REFLECTION_PROBE,
	INSTANCE_DECAL,
	INSTANCE_VOXEL_GI,
	INSTANCE_LIGHTMAP,
	INSTANCE_OCCLUDER,
	INSTANCE_VISIBLITY_NOTIFIER, // TODO: Fix typo in "VISIBILITY" (in 5.0).
	INSTANCE_FOG_VOLUME,
	INSTANCE_MAX,

	INSTANCE_GEOMETRY_MASK =
		(1 << INSTANCE_MESH) | (1 << INSTANCE_MULTIMESH) | (1 << INSTANCE_PARTICLES)
};

enum InstanceFlags
{
	INSTANCE_FLAG_USE_BAKED_LIGHT,
	INSTANCE_FLAG_USE_DYNAMIC_GI,
	INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE,
	INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING,
	INSTANCE_FLAG_MAX,
};

enum ShadowCastingSetting
{
	SHADOW_CASTING_SETTING_OFF,
	SHADOW_CASTING_SETTING_ON,
	SHADOW_CASTING_SETTING_DOUBLE_SIDED,
	SHADOW_CASTING_SETTING_SHADOWS_ONLY,
};

enum VisibilityRangeFadeMode
{
	VISIBILITY_RANGE_FADE_DISABLED,
	VISIBILITY_RANGE_FADE_SELF,
	VISIBILITY_RANGE_FADE_DEPENDENCIES,
};

/* BAKE API */

enum BakeChannels
{
	BAKE_CHANNEL_ALBEDO_ALPHA,
	BAKE_CHANNEL_NORMAL,
	BAKE_CHANNEL_ORM,
	BAKE_CHANNEL_EMISSION,
};

/* CANVAS TEXTURE API */

enum CanvasTextureChannel
{
	CANVAS_TEXTURE_CHANNEL_DIFFUSE,
	CANVAS_TEXTURE_CHANNEL_NORMAL,
	CANVAS_TEXTURE_CHANNEL_SPECULAR,
};

/* CANVAS ITEM API */

enum NinePatchAxisMode
{
	NINE_PATCH_STRETCH,
	NINE_PATCH_TILE,
	NINE_PATCH_TILE_FIT,
};

enum CanvasGroupMode
{
	CANVAS_GROUP_MODE_DISABLED,
	CANVAS_GROUP_MODE_CLIP_ONLY,
	CANVAS_GROUP_MODE_CLIP_AND_DRAW,
	CANVAS_GROUP_MODE_TRANSPARENT,
};

/* CANVAS LIGHT API */

enum CanvasLightMode
{
	CANVAS_LIGHT_MODE_POINT,
	CANVAS_LIGHT_MODE_DIRECTIONAL,
};

enum CanvasLightBlendMode
{
	CANVAS_LIGHT_BLEND_MODE_ADD,
	CANVAS_LIGHT_BLEND_MODE_SUB,
	CANVAS_LIGHT_BLEND_MODE_MIX,
};

enum CanvasLightShadowFilter
{
	CANVAS_LIGHT_FILTER_NONE,
	CANVAS_LIGHT_FILTER_PCF5,
	CANVAS_LIGHT_FILTER_PCF13,
	CANVAS_LIGHT_FILTER_MAX,
};

/* CANVAS OCCLUDER POLYGON API */

enum CanvasOccluderPolygonCullMode
{
	CANVAS_OCCLUDER_POLYGON_CULL_DISABLED,
	CANVAS_OCCLUDER_POLYGON_CULL_CLOCKWISE,
	CANVAS_OCCLUDER_POLYGON_CULL_COUNTER_CLOCKWISE,
};

/* GLOBAL SHADER PARAMETERS API */

enum GlobalShaderParameterType
{
	GLOBAL_VAR_TYPE_BOOL,
	GLOBAL_VAR_TYPE_BVEC2,
	GLOBAL_VAR_TYPE_BVEC3,
	GLOBAL_VAR_TYPE_BVEC4,
	GLOBAL_VAR_TYPE_INT,
	GLOBAL_VAR_TYPE_IVEC2,
	GLOBAL_VAR_TYPE_IVEC3,
	GLOBAL_VAR_TYPE_IVEC4,
	GLOBAL_VAR_TYPE_RECT2I,
	GLOBAL_VAR_TYPE_UINT,
	GLOBAL_VAR_TYPE_UVEC2,
	GLOBAL_VAR_TYPE_UVEC3,
	GLOBAL_VAR_TYPE_UVEC4,
	GLOBAL_VAR_TYPE_FLOAT,
	GLOBAL_VAR_TYPE_VEC2,
	GLOBAL_VAR_TYPE_VEC3,
	GLOBAL_VAR_TYPE_VEC4,
	GLOBAL_VAR_TYPE_COLOR,
	GLOBAL_VAR_TYPE_RECT2,
	GLOBAL_VAR_TYPE_MAT2,
	GLOBAL_VAR_TYPE_MAT3,
	GLOBAL_VAR_TYPE_MAT4,
	GLOBAL_VAR_TYPE_TRANSFORM_2D,
	GLOBAL_VAR_TYPE_TRANSFORM,
	GLOBAL_VAR_TYPE_SAMPLER2D,
	GLOBAL_VAR_TYPE_SAMPLER2DARRAY,
	GLOBAL_VAR_TYPE_SAMPLER3D,
	GLOBAL_VAR_TYPE_SAMPLERCUBE,
	GLOBAL_VAR_TYPE_SAMPLEREXT,
	GLOBAL_VAR_TYPE_MAX,
};

/* STATUS INFORMATION */

enum RenderingInfo
{
	RENDERING_INFO_TOTAL_OBJECTS_IN_FRAME,
	RENDERING_INFO_TOTAL_PRIMITIVES_IN_FRAME,
	RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME,
	RENDERING_INFO_TEXTURE_MEM_USED,
	RENDERING_INFO_BUFFER_MEM_USED,
	RENDERING_INFO_VIDEO_MEM_USED,
	RENDERING_INFO_PIPELINE_COMPILATIONS_CANVAS,
	RENDERING_INFO_PIPELINE_COMPILATIONS_MESH,
	RENDERING_INFO_PIPELINE_COMPILATIONS_SURFACE,
	RENDERING_INFO_PIPELINE_COMPILATIONS_DRAW,
	RENDERING_INFO_PIPELINE_COMPILATIONS_SPECIALIZATION,
	RENDERING_INFO_MAX,
};

/* BACKGROUND */

// If this is modified, review the conversion code in `project_settings.cpp`
// which is hardcoded to avoid coupling `core` with `servers`.
enum SplashStretchMode
{
	SPLASH_STRETCH_MODE_DISABLED,
	SPLASH_STRETCH_MODE_KEEP,
	SPLASH_STRETCH_MODE_KEEP_WIDTH,
	SPLASH_STRETCH_MODE_KEEP_HEIGHT,
	SPLASH_STRETCH_MODE_COVER,
	SPLASH_STRETCH_MODE_IGNORE,
};

/* MISC */

#ifndef DISABLE_DEPRECATED
// Never actually used, should be removed when we can break compatibility.
enum Features
{
	FEATURE_SHADERS,
	FEATURE_MULTITHREADED,
};
#endif

} // namespace RenderingServerEnums

// Alias to make it easier to use.
#define RSE RenderingServerEnums
// --- End servers/rendering/rendering_server_enums.h ---

class Mesh;

using Texture = Resource;

class Texture2D : public Resource
{
	mutable RID placeholder_rid;
	Size2 placeholder_size = Size2(1, 1);
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual int get_mipmap_count() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual Size2 get_size() const;

	virtual bool is_pixel_opaque(int p_x, int p_y) const;

	virtual bool has_alpha() const;
	virtual bool has_mipmaps() const;

	virtual RID get_rid() const override;
	virtual RID get_scaled_rid() const { return get_rid(); }

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const;
	virtual bool get_rect_region(
		const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const;

	virtual Ref<Image> get_image() const;

	static Ref<Texture2D> create_placeholder_2d(Size2 p_size = Size2(1, 1));
	virtual Ref<Resource> create_placeholder() const;

	Texture2D();
	virtual ~Texture2D();
};

class CanvasTexture final : public Texture2D
{
public:
	enum TextureFilter
	{
		TEXTURE_FILTER_PARENT_NODE,
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_MAX
	};

	enum TextureRepeat
	{
		TEXTURE_REPEAT_PARENT_NODE,
		TEXTURE_REPEAT_DISABLED,
		TEXTURE_REPEAT_ENABLED,
		TEXTURE_REPEAT_MIRROR,
		TEXTURE_REPEAT_MAX,
	};

private:
	Ref<Texture2D> diffuse_texture;
	Ref<Texture2D> normal_texture;
	Ref<Texture2D> specular_texture;
	Color specular = Color(1, 1, 1, 1);
	real_t shininess = 1.0;

	RID canvas_texture;

	TextureFilter texture_filter = TEXTURE_FILTER_PARENT_NODE;
	TextureRepeat texture_repeat = TEXTURE_REPEAT_PARENT_NODE;

	// Optional 2D mesh drawing mode (folded from MeshTexture).
	Ref<Mesh> mesh;
	Ref<Texture2D> base_texture;
	Size2i image_size;

public:
	void set_diffuse_texture(const Ref<Texture2D>& p_diffuse);
	Ref<Texture2D> get_diffuse_texture() const;

	void set_normal_texture(const Ref<Texture2D>& p_normal);
	Ref<Texture2D> get_normal_texture() const;

	void set_specular_texture(const Ref<Texture2D>& p_specular);
	Ref<Texture2D> get_specular_texture() const;

	void set_specular_color(const Color& p_color);
	Color get_specular_color() const;

	void set_specular_shininess(real_t p_shininess);
	real_t get_specular_shininess() const;

	void set_texture_filter(TextureFilter p_filter);
	TextureFilter get_texture_filter() const;

	void set_texture_repeat(TextureRepeat p_repeat);
	TextureRepeat get_texture_repeat() const;

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_image_size(const Size2& p_size);
	Size2 get_image_size() const;

	void set_base_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_base_texture() const;

	virtual int get_width() const override;
	virtual int get_height() const override;

	virtual bool is_pixel_opaque(int p_x, int p_y) const override;
	virtual bool has_alpha() const override;

	virtual Ref<Image> get_image() const override;

	virtual RID get_rid() const override;

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect,
		Rect2& r_src_rect) const override;

	CanvasTexture();
	~CanvasTexture();
};

class TextureLayered : public Resource
{
public:
	enum LayeredType
	{
		LAYERED_TYPE_2D_ARRAY,
		LAYERED_TYPE_CUBEMAP,
		LAYERED_TYPE_CUBEMAP_ARRAY
	};

private:
	mutable RID placeholder_rid;
	Size2i placeholder_size = Size2i(1, 1);
	int placeholder_layers = 1;
	LayeredType placeholder_layered_type = LAYERED_TYPE_2D_ARRAY;
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual LayeredType get_layered_type() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual int get_layers() const;
	virtual bool has_mipmaps() const;
	virtual Ref<Image> get_layer_data(int p_layer) const;
	virtual RID get_rid() const override;

	static Ref<TextureLayered> create_placeholder_layered(
		LayeredType p_type = LAYERED_TYPE_2D_ARRAY, const Size2i& p_size = Size2i(1, 1),
		int p_layers = 1);
	virtual Ref<Resource> create_placeholder() const;

	TextureLayered() = default;
	virtual ~TextureLayered();
};

class Texture3D : public Resource
{
	mutable RID placeholder_rid;
	Vector3i placeholder_size = Vector3i(1, 1, 1);
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual int get_depth() const;
	virtual bool has_mipmaps() const;
	virtual Vector<Ref<Image>> get_data() const;
	virtual RID get_rid() const override;

	static Ref<Texture3D> create_placeholder_3d(const Vector3i& p_size = Vector3i(1, 1, 1));
	virtual Ref<Resource> create_placeholder() const;

	Texture3D() = default;
	virtual ~Texture3D();
};
// --- End scene/resources/texture.h ---

// --- Folded from scene/resources/theme.h ---
#include "core/io/ip.h"
// --- Folded from scene/resources/font.h ---
#include "core/io/ip.h"
// --- Folded from servers/text/text_server.h ---
#include "core/io/net_socket.h"
#include "core/math/vector2.h"
#include "core/io/ip.h"

template <typename T> class TypedArray;

struct Glyph;
struct CaretInfo;

#define OT_TAG(m_c1, m_c2, m_c3, m_c4)                                                             \
	((int32_t)((((uint32_t)(m_c1)&0xff) << 24) | (((uint32_t)(m_c2)&0xff) << 16) |                 \
			   (((uint32_t)(m_c3)&0xff) << 8) | ((uint32_t)(m_c4)&0xff)))

class TextServer : public RefCounted
{
public:
	enum FontAntialiasing
	{
		FONT_ANTIALIASING_NONE,
		FONT_ANTIALIASING_GRAY,
		FONT_ANTIALIASING_LCD,
	};

	enum FontLCDSubpixelLayout
	{
		FONT_LCD_SUBPIXEL_LAYOUT_NONE,
		FONT_LCD_SUBPIXEL_LAYOUT_HRGB,
		FONT_LCD_SUBPIXEL_LAYOUT_HBGR,
		FONT_LCD_SUBPIXEL_LAYOUT_VRGB,
		FONT_LCD_SUBPIXEL_LAYOUT_VBGR,
		FONT_LCD_SUBPIXEL_LAYOUT_MAX,
	};

	enum Direction
	{
		DIRECTION_AUTO,
		DIRECTION_LTR,
		DIRECTION_RTL,
		DIRECTION_INHERITED,
	};

	enum Orientation
	{
		ORIENTATION_HORIZONTAL,
		ORIENTATION_VERTICAL
	};

	enum JustificationFlag
	{
		JUSTIFICATION_NONE = 0,
		JUSTIFICATION_KASHIDA = 1 << 0,
		JUSTIFICATION_WORD_BOUND = 1 << 1,
		JUSTIFICATION_TRIM_EDGE_SPACES = 1 << 2,
		JUSTIFICATION_AFTER_LAST_TAB = 1 << 3,
		JUSTIFICATION_CONSTRAIN_ELLIPSIS = 1 << 4,
		JUSTIFICATION_SKIP_LAST_LINE = 1 << 5,
		JUSTIFICATION_SKIP_LAST_LINE_WITH_VISIBLE_CHARS = 1 << 6,
		JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE = 1 << 7,
	};

	enum VisibleCharactersBehavior
	{
		VC_CHARS_BEFORE_SHAPING,
		VC_CHARS_AFTER_SHAPING,
		VC_GLYPHS_AUTO,
		VC_GLYPHS_LTR,
		VC_GLYPHS_RTL,
	};

	enum AutowrapMode
	{
		AUTOWRAP_OFF,
		AUTOWRAP_ARBITRARY,
		AUTOWRAP_WORD,
		AUTOWRAP_WORD_SMART
	};

	enum LineBreakFlag
	{
		BREAK_NONE = 0,
		BREAK_MANDATORY = 1 << 0,
		BREAK_WORD_BOUND = 1 << 1,
		BREAK_GRAPHEME_BOUND = 1 << 2,
		BREAK_ADAPTIVE = 1 << 3,
#ifndef DISABLE_DEPRECATED
		BREAK_TRIM_EDGE_SPACES = 1 << 4,
#else
	// RESERVED = 1 << 4,
#endif
		BREAK_TRIM_INDENT = 1 << 5,
		BREAK_TRIM_START_EDGE_SPACES = 1 << 6,
		BREAK_TRIM_END_EDGE_SPACES = 1 << 7,

		BREAK_TRIM_MASK =
			BREAK_TRIM_INDENT | BREAK_TRIM_START_EDGE_SPACES | BREAK_TRIM_END_EDGE_SPACES,
	};

	enum OverrunBehavior
	{
		OVERRUN_NO_TRIMMING,
		OVERRUN_TRIM_CHAR,
		OVERRUN_TRIM_WORD,
		OVERRUN_TRIM_ELLIPSIS,
		OVERRUN_TRIM_WORD_ELLIPSIS,
		OVERRUN_TRIM_ELLIPSIS_FORCE,
		OVERRUN_TRIM_WORD_ELLIPSIS_FORCE,
	};

	enum TextOverrunFlag
	{
		OVERRUN_NO_TRIM = 0,
		OVERRUN_TRIM = 1 << 0,
		OVERRUN_TRIM_WORD_ONLY = 1 << 1,
		OVERRUN_ADD_ELLIPSIS = 1 << 2,
		OVERRUN_ENFORCE_ELLIPSIS = 1 << 3,
		OVERRUN_JUSTIFICATION_AWARE = 1 << 4,
		OVERRUN_SHORT_STRING_ELLIPSIS = 1 << 5,
	};

	enum GraphemeFlag
	{
		GRAPHEME_IS_VALID = 1 << 0,		 // Grapheme is valid.
		GRAPHEME_IS_RTL = 1 << 1,		 // Grapheme is right-to-left.
		GRAPHEME_IS_VIRTUAL = 1 << 2,	 // Grapheme is not part of source string (added by
										 // fit_to_width function, do not affect caret movement).
		GRAPHEME_IS_SPACE = 1 << 3,		 // Is whitespace (for justification and word breaks).
		GRAPHEME_IS_BREAK_HARD = 1 << 4, // Is line break (mandatory break, e.g. "\n").
		GRAPHEME_IS_BREAK_SOFT = 1 << 5, // Is line break (optional break, e.g. space).
		GRAPHEME_IS_TAB = 1 << 6,		 // Is tab or vertical tab.
		GRAPHEME_IS_ELONGATION = 1 << 7, // Elongation (e.g. kashida), grapheme can be duplicated or
										 // truncated to fit line to width.
		GRAPHEME_IS_PUNCTUATION = 1 << 8, // Punctuation, except underscore (can be used as word
										  // break, but not line break or justifiction).
		GRAPHEME_IS_UNDERSCORE = 1 << 9,  // Underscore (can be used as word break).
		GRAPHEME_IS_CONNECTED = 1 << 10,  // Connected to previous grapheme.
		GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL =
			1 << 11, // It is safe to insert a U+0640 before this grapheme for elongation.
		GRAPHEME_IS_EMBEDDED_OBJECT =
			1 << 12, // Grapheme is an object replacement character for the embedded object.
		GRAPHEME_IS_SOFT_HYPHEN = 1 << 13, // Grapheme is a soft hyphen.
	};

	enum Hinting
	{
		HINTING_NONE,
		HINTING_LIGHT,
		HINTING_NORMAL
	};

	enum SubpixelPositioning
	{
		SUBPIXEL_POSITIONING_DISABLED = 0,
		SUBPIXEL_POSITIONING_AUTO = 1,
		SUBPIXEL_POSITIONING_ONE_HALF = 2,
		SUBPIXEL_POSITIONING_ONE_QUARTER = 3,

		SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE = 20,
		SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE = 16,
	};

	enum Feature
	{
		FEATURE_SIMPLE_LAYOUT = 1 << 0,
		FEATURE_BIDI_LAYOUT = 1 << 1,
		FEATURE_VERTICAL_LAYOUT = 1 << 2,
		FEATURE_SHAPING = 1 << 3,
		FEATURE_KASHIDA_JUSTIFICATION = 1 << 4,
		FEATURE_BREAK_ITERATORS = 1 << 5,
		FEATURE_FONT_BITMAP = 1 << 6,
		FEATURE_FONT_DYNAMIC = 1 << 7,
		FEATURE_FONT_MSDF = 1 << 8,
		FEATURE_FONT_SYSTEM = 1 << 9,
		FEATURE_FONT_VARIABLE = 1 << 10,
		FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION = 1 << 11,
		FEATURE_USE_SUPPORT_DATA = 1 << 12,
		FEATURE_UNICODE_IDENTIFIERS = 1 << 13,
		FEATURE_UNICODE_SECURITY = 1 << 14,
	};

	enum ContourPointTag
	{
		CONTOUR_CURVE_TAG_ON = 0x01,
		CONTOUR_CURVE_TAG_OFF_CONIC = 0x00,
		CONTOUR_CURVE_TAG_OFF_CUBIC = 0x02
	};

	enum SpacingType
	{
		SPACING_GLYPH,
		SPACING_SPACE,
		SPACING_TOP,
		SPACING_BOTTOM,
		SPACING_MAX,
	};

	enum FontStyle
	{
		FONT_BOLD = 1 << 0,
		FONT_ITALIC = 1 << 1,
		FONT_FIXED_WIDTH = 1 << 2,
	};

	enum StructuredTextParser
	{
		STRUCTURED_TEXT_DEFAULT,
		STRUCTURED_TEXT_URI,
		STRUCTURED_TEXT_FILE,
		STRUCTURED_TEXT_EMAIL,
		STRUCTURED_TEXT_LIST,
		STRUCTURED_TEXT_GDSCRIPT,
		STRUCTURED_TEXT_CUSTOM
	};

	enum FixedSizeScaleMode
	{
		FIXED_SIZE_SCALE_DISABLE,
		FIXED_SIZE_SCALE_INTEGER_ONLY,
		FIXED_SIZE_SCALE_ENABLED,
	};

	void _draw_hex_code_box_number(const RID& p_canvas, int64_t p_size, const Vector2& p_pos,
		uint8_t p_index, const Color& p_color) const;

protected:
	static double vp_oversampling;
	HashMap<char32_t, char32_t> diacritics_map;
	void _diacritics_map_add(const String& p_from, char32_t p_to);
	void _init_diacritics_map();


#ifndef DISABLE_DEPRECATED
	void _font_draw_glyph_bind_compat_104872(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1)) const;
	void _font_draw_glyph_outline_bind_compat_104872(const RID& p_font, const RID& p_canvas,
		int64_t p_size, int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1)) const;
	void _shaped_text_draw_bind_compat_104872(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		const Color& p_color = Color(1, 1, 1)) const;
	void _shaped_text_draw_outline_bind_compat_104872(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		int64_t p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	PackedInt32Array _shaped_text_get_word_breaks_bind_compat_90732(const RID& p_shaped,
		uint32_t p_grapheme_flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_PUNCTUATION) const;
	static void _bind_compatibility_methods();
#endif

public:
	static uint32_t get_overrun_flags_from_behavior(OverrunBehavior p_behavior);

	virtual bool has_feature(Feature p_feature) const { return false; }

	virtual String get_name() const { return String(); }

	virtual String get_short_name() const { return String(); }

	virtual int64_t get_features() const { return 0; }

	virtual void free_rid(const RID& p_rid) {}

	virtual bool has(const RID& p_rid) { return false; }

	virtual bool load_support_data(const String& p_filename) { return false; }

	virtual String get_support_data_filename() const { return String(); }

	virtual String get_support_data_info() const { return String(); }

	virtual bool save_support_data(const String& p_filename) const { return false; }

	virtual PackedByteArray get_support_data() const { return PackedByteArray(); }

	virtual bool is_locale_using_support_data(const String& p_locale) const { return false; }

	virtual bool is_locale_right_to_left(const String& p_locale) const { return false; }

	virtual int64_t name_to_tag(const String& p_name) const;
	virtual String tag_to_name(int64_t p_tag) const;

	/* Font interface */

	virtual RID create_font() { return RID(); }

	virtual RID create_font_linked_variation(const RID& p_font_rid) { return RID(); }

	virtual void font_set_data(const RID& p_font_rid, const PackedByteArray& p_data) {}

	virtual void font_set_data_ptr(
		const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size)
	{
	}

	virtual void font_set_face_index(const RID& p_font_rid, int64_t p_index) {}

	virtual int64_t font_get_face_index(const RID& p_font_rid) const { return 0; }

	virtual int64_t font_get_face_count(const RID& p_font_rid) const { return 0; }

	virtual void font_set_style(const RID& p_font_rid, uint32_t p_style) {}

	virtual uint32_t font_get_style(const RID& p_font_rid) const { return 0; }

	virtual void font_set_name(const RID& p_font_rid, const String& p_name) {}

	virtual String font_get_name(const RID& p_font_rid) const { return String(); }

	virtual void font_set_style_name(const RID& p_font_rid, const String& p_name) {}

	virtual String font_get_style_name(const RID& p_font_rid) const { return String(); }

	virtual void font_set_weight(const RID& p_font_rid, int64_t p_weight) {}

	virtual int64_t font_get_weight(const RID& p_font_rid) const { return 0; }

	virtual void font_set_stretch(const RID& p_font_rid, int64_t p_stretch) {}

	virtual int64_t font_get_stretch(const RID& p_font_rid) const { return 0; }

	virtual void font_set_antialiasing(const RID& p_font_rid, FontAntialiasing p_antialiasing) {}

	virtual FontAntialiasing font_get_antialiasing(const RID& p_font_rid) const
	{
		return FONT_ANTIALIASING_NONE;
	}

	virtual void font_set_disable_embedded_bitmaps(
		const RID& p_font_rid, bool p_disable_embedded_bitmaps)
	{
	}

	virtual bool font_get_disable_embedded_bitmaps(const RID& p_font_rid) const { return false; }

	virtual void font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps) {}

	virtual bool font_get_generate_mipmaps(const RID& p_font_rid) const { return false; }

	virtual void font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf) {}

	virtual bool font_is_multichannel_signed_distance_field(const RID& p_font_rid) const
	{
		return false;
	}

	virtual void font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range) {}

	virtual int64_t font_get_msdf_pixel_range(const RID& p_font_rid) const { return 0; }

	virtual void font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size) {}

	virtual int64_t font_get_msdf_size(const RID& p_font_rid) const { return 0; }

	virtual void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) {}

	virtual int64_t font_get_fixed_size(const RID& p_font_rid) const { return 0; }

	virtual void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, FixedSizeScaleMode p_fixed_size_scale)
	{
	}

	virtual FixedSizeScaleMode font_get_fixed_size_scale_mode(const RID& p_font_rid) const
	{
		return FIXED_SIZE_SCALE_DISABLE;
	}

	virtual void font_set_allow_system_fallback(const RID& p_font_rid, bool p_allow_system_fallback)
	{
	}

	virtual bool font_is_allow_system_fallback(const RID& p_font_rid) const { return false; }

	virtual void font_clear_system_fallback_cache() {}

	virtual void font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter) {}

	virtual bool font_is_force_autohinter(const RID& p_font_rid) const { return false; }

	virtual void font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate) {}

	virtual bool font_is_modulate_color_glyphs(const RID& p_font_rid) const { return false; }

	virtual int64_t font_get_palette_count(const RID& p_font_rid) const { return 0; }

	virtual String font_get_palette_name(const RID& p_font_rid, int64_t p_index) const
	{
		return String();
	}

	virtual Vector<Color> font_get_palette_colors(const RID& p_font_rid, int64_t p_index) const
	{
		return Vector<Color>();
	}

	virtual void font_set_palette_custom_colors(
		const RID& p_font_rid, const Vector<Color>& p_colors)
	{
	}

	virtual Vector<Color> font_get_palette_custom_colors(const RID& p_font_rid) const
	{
		return Vector<Color>();
	}

	virtual int64_t font_get_used_palette(const RID& p_font_rid) const { return 0; }

	virtual void font_set_used_palette(const RID& p_font_rid, int64_t p_index) {}

	virtual void font_set_hinting(const RID& p_font_rid, Hinting p_hinting) {}

	virtual Hinting font_get_hinting(const RID& p_font_rid) const { return HINTING_NONE; }

	virtual void font_set_subpixel_positioning(
		const RID& p_font_rid, SubpixelPositioning p_subpixel)
	{
	}

	virtual SubpixelPositioning font_get_subpixel_positioning(const RID& p_font_rid) const
	{
		return SUBPIXEL_POSITIONING_DISABLED;
	}

	virtual void font_set_keep_rounding_remainders(
		const RID& p_font_rid, bool p_keep_rounding_remainders)
	{
	}

	virtual bool font_get_keep_rounding_remainders(const RID& p_font_rid) const { return false; }

	virtual void font_set_embolden(const RID& p_font_rid, double p_strength) {}

	virtual double font_get_embolden(const RID& p_font_rid) const { return 0.0; }

	virtual void font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value) {}

	virtual int64_t font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const
	{
		return 0;
	}

	virtual void font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset) {}

	virtual double font_get_baseline_offset(const RID& p_font_rid) const { return 0.0; }

	virtual void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) {}

	virtual Transform2D font_get_transform(const RID& p_font_rid) const { return Transform2D(); }

	virtual void font_set_oversampling(const RID& p_font_rid, double p_oversampling) {}

	virtual double font_get_oversampling(const RID& p_font_rid) const { return 0.0; }

	virtual Vector<Vector2i> font_get_size_cache_list(const RID& p_font_rid) const
	{
		return Vector<Vector2i>();
	}

	virtual void font_clear_size_cache(const RID& p_font_rid) {}

	virtual void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) {}

	virtual double font_get_ascent(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) {}

	virtual double font_get_descent(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position)
	{
	}

	virtual double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const
	{
		return 0.0;
	}

	virtual void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness)
	{
	}

	virtual double font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const
	{
		return 0.0;
	}

	virtual void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) {}

	virtual double font_get_scale(const RID& p_font_rid, int64_t p_size) const { return 0.0; }

	virtual int64_t font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const
	{
		return 0;
	}

	virtual void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index)
	{
	}

	virtual void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image)
	{
	}

	virtual Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
	{
		return Ref<Image>();
	}

	virtual void font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const PackedInt32Array& p_offset)
	{
	}

	virtual PackedInt32Array font_get_texture_offsets(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
	{
		return PackedInt32Array();
	}

	virtual PackedInt32Array font_get_glyph_list(
		const RID& p_font_rid, const Vector2i& p_size) const
	{
		return PackedInt32Array();
	}

	virtual void font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size) {}

	virtual void font_remove_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph)
	{
	}

	virtual Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance)
	{
	}

	virtual Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset)
	{
	}

	virtual Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Vector2();
	}

	virtual void font_set_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size)
	{
	}

	virtual Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Rect2();
	}

	virtual void font_set_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect)
	{
	}

	virtual int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return 0;
	}

	virtual void font_set_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx)
	{
	}

	virtual RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return RID();
	}

	virtual Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
	{
		return Size2();
	}

	virtual void font_clear_kerning_map(const RID& p_font_rid, int64_t p_size) {}

	virtual void font_remove_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair)
	{
	}

	virtual void font_set_kerning(const RID& p_font_rid, int64_t p_size,
		const Vector2i& p_glyph_pair, const Vector2& p_kerning)
	{
	}

	virtual Vector2 font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const
	{
		return Vector2();
	}

	virtual int64_t font_get_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const
	{
		return 0;
	}

	virtual int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const
	{
		return 0;
	}

	virtual bool font_has_char(const RID& p_font_rid, int64_t p_char) const { return false; }

	virtual String font_get_supported_chars(const RID& p_font_rid) const { return String(); }

	virtual PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const
	{
		return PackedInt32Array();
	}

	virtual void font_render_range(
		const RID& p_font, const Vector2i& p_size, int64_t p_start, int64_t p_end)
	{
	}

	virtual void font_render_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_index)
	{
	}

	virtual void font_draw_glyph(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const
	{
	}

	virtual void font_draw_glyph_outline(const RID& p_font, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const
	{
	}

	virtual bool font_is_language_supported(const RID& p_font_rid, const String& p_language) const
	{
		return false;
	}

	virtual void font_set_language_support_override(
		const RID& p_font_rid, const String& p_language, bool p_supported)
	{
	}

	virtual bool font_get_language_support_override(const RID& p_font_rid, const String& p_language)
	{
		return false;
	}

	virtual void font_remove_language_support_override(
		const RID& p_font_rid, const String& p_language)
	{
	}

	virtual PackedStringArray font_get_language_support_overrides(const RID& p_font_rid)
	{
		return PackedStringArray();
	}

	virtual bool font_is_script_supported(const RID& p_font_rid, const String& p_script) const
	{
		return false;
	}

	virtual void font_set_script_support_override(
		const RID& p_font_rid, const String& p_script, bool p_supported)
	{
	}

	virtual bool font_get_script_support_override(const RID& p_font_rid, const String& p_script)
	{
		return false;
	}

	virtual void font_remove_script_support_override(const RID& p_font_rid, const String& p_script)
	{
	}

	virtual PackedStringArray font_get_script_support_overrides(const RID& p_font_rid)
	{
		return PackedStringArray();
	}

#ifndef DISABLE_DEPRECATED
	virtual double font_get_global_oversampling() const { return 1.0; }

	virtual void font_set_global_oversampling(double p_oversampling) {}
#endif
	virtual void reference_oversampling_level(double p_oversampling) {}

	virtual void unreference_oversampling_level(double p_oversampling) {}

	virtual Vector2 get_hex_code_box_size(int64_t p_size, int64_t p_index) const;
	virtual void draw_hex_code_box(const RID& p_canvas, int64_t p_size, const Vector2& p_pos,
		int64_t p_index, const Color& p_color) const;

	/* Shaped text buffer interface */

	virtual RID create_shaped_text(
		Direction p_direction = DIRECTION_AUTO, Orientation p_orientation = ORIENTATION_HORIZONTAL)
	{
		return RID();
	}

	virtual void shaped_text_clear(const RID& p_shaped) {}

	virtual RID shaped_text_duplicate(const RID& p_shaped) { return RID(); }

	virtual void shaped_text_set_direction(
		const RID& p_shaped, Direction p_direction = DIRECTION_AUTO)
	{
	}

	virtual Direction shaped_text_get_direction(const RID& p_shaped) const
	{
		return DIRECTION_AUTO;
	}

	virtual Direction shaped_text_get_inferred_direction(const RID& p_shaped) const
	{
		return DIRECTION_AUTO;
	}

	virtual void shaped_text_set_custom_punctuation(const RID& p_shaped, const String& p_punct) {}

	virtual String shaped_text_get_custom_punctuation(const RID& p_shaped) const
	{
		return String();
	}

	virtual void shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char) {}

	virtual int64_t shaped_text_get_custom_ellipsis(const RID& p_shaped) const { return 0; }

	virtual void shaped_text_set_orientation(
		const RID& p_shaped, Orientation p_orientation = ORIENTATION_HORIZONTAL)
	{
	}

	virtual Orientation shaped_text_get_orientation(const RID& p_shaped) const
	{
		return ORIENTATION_HORIZONTAL;
	}

	virtual void shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled) {}

	virtual bool shaped_text_get_preserve_invalid(const RID& p_shaped) const { return false; }

	virtual void shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled) {}

	virtual bool shaped_text_get_preserve_control(const RID& p_shaped) const { return false; }

	virtual void shaped_text_set_spacing(
		const RID& p_shaped, SpacingType p_spacing, int64_t p_value)
	{
	}

	virtual int64_t shaped_text_get_spacing(const RID& p_shaped, SpacingType p_spacing) const
	{
		return 0;
	}

	virtual String shaped_get_text(const RID& p_shaped) const { return String(); }

	virtual int64_t shaped_get_span_count(const RID& p_shaped) const { return 0; }

	virtual String shaped_get_span_text(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual int64_t shaped_get_run_count(const RID& p_shaped) const { return 0; }

	virtual String shaped_get_run_text(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual Vector2i shaped_get_run_range(const RID& p_shaped, int64_t p_index) const
	{
		return Vector2i();
	}

	virtual Vector2i shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const
	{
		return Vector2i();
	}

	virtual RID shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const
	{
		return RID();
	}

	virtual int shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const { return 0; }

	virtual String shaped_get_run_language(const RID& p_shaped, int64_t p_index) const
	{
		return String();
	}

	virtual Direction shaped_get_run_direction(const RID& p_shaped, int64_t p_index) const
	{
		return DIRECTION_AUTO;
	}

	virtual RID shaped_text_substr(const RID& p_shaped, int64_t p_start, int64_t p_length) const
	{
		return RID();
	} // Copy shaped substring (e.g. line break) without reshaping, but correctly reordered,
	  // preservers range.

	virtual RID shaped_text_get_parent(const RID& p_shaped) const { return RID(); }

	virtual double shaped_text_fit_to_width(const RID& p_shaped, double p_width,
		uint32_t p_jst_flags = JUSTIFICATION_WORD_BOUND | JUSTIFICATION_KASHIDA)
	{
		return 0.0;
	}

	virtual double shaped_text_tab_align(const RID& p_shaped, const PackedFloat32Array& p_tab_stops)
	{
		return 0.0;
	}

	virtual bool shaped_text_shape(const RID& p_shaped) { return false; }

	virtual bool shaped_text_update_breaks(const RID& p_shaped) { return false; }

	virtual bool shaped_text_update_justification_ops(const RID& p_shaped) { return false; }

	virtual bool shaped_text_is_ready(const RID& p_shaped) const { return false; }

	bool shaped_text_has_visible_chars(const RID& p_shaped) const;

	virtual const Glyph* shaped_text_get_glyphs(const RID& p_shaped) const { return nullptr; }

	virtual const Glyph* shaped_text_sort_logical(const RID& p_shaped) { return nullptr; }

	virtual int64_t shaped_text_get_glyph_count(const RID& p_shaped) const { return 0; }

	virtual Vector2i shaped_text_get_range(const RID& p_shaped) const { return Vector2i(); }

	virtual PackedInt32Array shaped_text_get_line_breaks_adv(const RID& p_shaped,
		const PackedFloat32Array& p_width, int64_t p_start = 0, bool p_once = true,
		uint32_t p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const;
	virtual PackedInt32Array shaped_text_get_line_breaks(const RID& p_shaped, double p_width,
		int64_t p_start = 0, uint32_t p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const;
	virtual PackedInt32Array shaped_text_get_word_breaks(const RID& p_shaped,
		uint32_t p_grapheme_flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_PUNCTUATION,
		uint32_t p_skip_grapheme_flags = GRAPHEME_IS_VIRTUAL) const;

	virtual PackedInt32Array shaped_text_get_character_breaks(const RID& p_shaped) const
	{
		return PackedInt32Array();
	}

	virtual PackedInt32Array string_get_word_breaks(
		const String& p_string, const String& p_language = "", int64_t p_chars_per_line = 0) const
	{
		return PackedInt32Array();
	}

	virtual String string_to_upper(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual String string_to_lower(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual String string_to_title(const String& p_string, const String& p_language = "") const
	{
		return String();
	}

	virtual int64_t shaped_text_get_trim_pos(const RID& p_shaped) const { return -1; }

	virtual int64_t shaped_text_get_ellipsis_pos(const RID& p_shaped) const { return -1; }

	virtual const Glyph* shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const
	{
		return nullptr;
	}

	virtual int64_t shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const { return -1; }

	virtual void shaped_text_overrun_trim_to_width(
		const RID& p_shaped, double p_width, uint32_t p_trim_flags)
	{
	}

	virtual Size2 shaped_text_get_size(const RID& p_shaped) const { return Size2(); }

	virtual double shaped_text_get_ascent(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_descent(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_width(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_underline_position(const RID& p_shaped) const { return 0.0; }

	virtual double shaped_text_get_underline_thickness(const RID& p_shaped) const { return 0.0; }

	virtual Direction shaped_text_get_dominant_direction_in_range(
		const RID& p_shaped, int64_t p_start, int64_t p_end) const;

	virtual CaretInfo shaped_text_get_carets(const RID& p_shaped, int64_t p_position) const;

	virtual Vector<Vector2> shaped_text_get_selection(
		const RID& p_shaped, int64_t p_start, int64_t p_end) const;

	virtual int64_t shaped_text_hit_test_grapheme(
		const RID& p_shaped, double p_coords) const; // Return grapheme index.
	virtual int64_t shaped_text_hit_test_position(
		const RID& p_shaped, double p_coords) const; // Return caret/selection position.

	virtual Vector2 shaped_text_get_grapheme_bounds(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_next_grapheme_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_prev_grapheme_pos(const RID& p_shaped, int64_t p_pos) const;

	virtual int64_t shaped_text_next_character_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_prev_character_pos(const RID& p_shaped, int64_t p_pos) const;
	virtual int64_t shaped_text_closest_character_pos(const RID& p_shaped, int64_t p_pos) const;

	// The pen position is always placed on the baseline and moving left to right.
	virtual void shaped_text_draw(const RID& p_shaped, const RID& p_canvas, const Vector2& p_pos,
		double p_clip_l = -1.0, double p_clip_r = -1.0, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	virtual void shaped_text_draw_outline(const RID& p_shaped, const RID& p_canvas,
		const Vector2& p_pos, double p_clip_l = -1.0, double p_clip_r = -1.0,
		int64_t p_outline_size = 1, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;

#ifdef DEBUG_ENABLED
	void debug_print_glyph(int p_idx, const Glyph& p_glyph) const;
	void shaped_text_debug_print(const RID& p_shaped) const;
#endif

#ifndef DISABLE_DEPRECATED
	// Number conversion.
	virtual String format_number(const String& p_string, const String& p_language = "") const;
	virtual String parse_number(const String& p_string, const String& p_language = "") const;
	virtual String percent_sign(const String& p_language = "") const;
#endif // DISABLE_DEPRECATED

	// String functions.
	virtual PackedInt32Array string_get_character_breaks(
		const String& p_string, const String& p_language = "") const;


	virtual int64_t is_confusable(const String& p_string, const PackedStringArray& p_dict) const
	{
		return -1;
	}

	virtual bool spoof_check(const String& p_string) const { return false; }

	virtual String strip_diacritics(const String& p_string) const;
	virtual bool is_valid_identifier(const String& p_string) const;
	virtual bool is_valid_letter(uint64_t p_unicode) const;

	static void set_current_drawn_item_oversampling(double p_vp_oversampling)
	{
		vp_oversampling = p_vp_oversampling;
	}

	static double get_current_drawn_item_oversampling() { return vp_oversampling; }

	virtual void cleanup() {}

	TextServer() = default;
	~TextServer() = default;
};

struct Glyph
{
	int start = -1; // Start offset in the source string.
	int end = -1;	// End offset in the source string.

	uint8_t count = 0;	// Number of glyphs in the grapheme, set in the first glyph only.
	uint8_t repeat = 1; // Draw multiple times in the row.
	uint16_t flags = 0; // Grapheme flags (valid, rtl, virtual), set in the first glyph only.

	float x_off = 0.f; // Offset from the origin of the glyph on baseline.
	float y_off = 0.f;
	float advance =
		0.f; // Advance to the next glyph along baseline(x for horizontal layout, y for vertical).

	RID font_rid;	   // Font resource.
	int font_size = 0; // Font size.
	int32_t index = 0; // Glyph index (font specific) or UTF-32 codepoint (for the invalid glyphs).

	int span_index = -1;

	bool operator==(const Glyph& p_a) const;
	bool operator!=(const Glyph& p_a) const;

	bool operator<(const Glyph& p_a) const;
	bool operator>(const Glyph& p_a) const;
};

struct CaretInfo
{
	Rect2 l_caret;
	Rect2 t_caret;
	TextServer::Direction l_dir;
	TextServer::Direction t_dir;
};

class TextServerManager final
{
private:
	struct Data
	{
		Ref<TextServer> primary_interface;
		Vector<Ref<TextServer>> interfaces;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void add_interface(const Ref<TextServer>& p_interface);
	static void remove_interface(const Ref<TextServer>& p_interface);
	static int get_interface_count();
	static Ref<TextServer> get_interface(int p_index);
	static Ref<TextServer> find_interface(const String& p_name);

	_FORCE_INLINE_ static Ref<TextServer> get_primary_interface()
	{
		return data ? data->primary_interface : Ref<TextServer>();
	}

	static void set_primary_interface(const Ref<TextServer>& p_primary_interface);

	TextServerManager() = delete;
	TextServerManager(const TextServerManager&) = delete;
	TextServerManager& operator=(const TextServerManager&) = delete;
	~TextServerManager() = delete;
};

#define TS TextServerManager::get_primary_interface()

// --- Folded from servers/text/text_server_dummy.h ---
/*************************************************************************/

#include "core/os/os.h"


class TextServerDummy : public TextServer
{
	_THREAD_SAFE_CLASS_

public:
	virtual bool has_feature(Feature p_feature) const override { return false; }

	virtual String get_name() const override { return "Dummy"; }

	virtual String get_short_name() const override { return "dummy"; }

	virtual int64_t get_features() const override { return 0; }

	virtual void free_rid(const RID& p_rid) override {}

	virtual bool has(const RID& p_rid) override { return false; }

	virtual RID create_font() override { return RID(); }

	virtual void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) override {}

	virtual int64_t font_get_fixed_size(const RID& p_font_rid) const override { return 0; }

	virtual void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode) override
	{
	}

	virtual TextServer::FixedSizeScaleMode font_get_fixed_size_scale_mode(
		const RID& p_font_rid) const override
	{
		return FIXED_SIZE_SCALE_DISABLE;
	}

	virtual void font_clear_size_cache(const RID& p_font_rid) override {}

	virtual void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) override {}

	virtual double font_get_ascent(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) override
	{
	}

	virtual double font_get_descent(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position) override
	{
	}

	virtual double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness) override
	{
	}

	virtual double font_get_underline_thickness(
		const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) override {}

	virtual double font_get_scale(const RID& p_font_rid, int64_t p_size) const override
	{
		return 0;
	}

	virtual int64_t font_get_texture_count(
		const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return 0;
	}

	virtual void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) override
	{
	}

	virtual void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image) override
	{
	}

	virtual Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const override
	{
		return Ref<Image>();
	}

	virtual PackedInt32Array font_get_glyph_list(
		const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return PackedInt32Array();
	}

	virtual void font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size) override {}

	virtual void font_remove_glyph(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) override
	{
	}

	virtual Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance) override
	{
	}

	virtual Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_offset(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, const Vector2& p_offset) override
	{
	}

	virtual Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Vector2();
	}

	virtual void font_set_glyph_size(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_gl_size) override
	{
	}

	virtual Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Rect2();
	}

	virtual void font_set_glyph_uv_rect(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, const Rect2& p_uv_rect) override
	{
	}

	virtual int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return 0;
	}

	virtual void font_set_glyph_texture_idx(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_glyph, int64_t p_texture_idx) override
	{
	}

	virtual RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return RID();
	}

	virtual Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return Size2();
	}

	virtual int64_t font_get_glyph_index(const RID& p_font_rid, int64_t p_size, int64_t p_char,
		int64_t p_variation_selector) const override
	{
		return 0;
	}

	virtual int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const override
	{
		return 0;
	}

	virtual bool font_has_char(const RID& p_font_rid, int64_t p_char) const override
	{
		return false;
	}

	virtual String font_get_supported_chars(const RID& p_font_rid) const override
	{
		return String();
	}

	virtual PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const override
	{
		return PackedInt32Array();
	}

	virtual void font_draw_glyph(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const override
	{
	}

	virtual void font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const override
	{
	}

	virtual RID create_shaped_text(
		TextServer::Direction p_direction, TextServer::Orientation p_orientation) override
	{
		return RID();
	}

	virtual void shaped_text_clear(const RID& p_shaped) override {}

	virtual RID shaped_text_duplicate(const RID& p_shaped) override { return RID(); }

	virtual int64_t shaped_get_span_count(const RID& p_shaped) const override { return 0; }

	virtual RID shaped_text_substr(
		const RID& p_shaped, int64_t p_start, int64_t p_length) const override
	{
		return RID();
	}

	virtual RID shaped_text_get_parent(const RID& p_shaped) const override { return RID(); }

	virtual bool shaped_text_shape(const RID& p_shaped) override { return false; }

	virtual bool shaped_text_is_ready(const RID& p_shaped) const override { return false; }

	virtual const Glyph* shaped_text_get_glyphs(const RID& p_shaped) const override
	{
		return nullptr;
	}

	virtual const Glyph* shaped_text_sort_logical(const RID& p_shaped) override { return nullptr; }

	virtual int64_t shaped_text_get_glyph_count(const RID& p_shaped) const override { return 0; }

	virtual Vector2i shaped_text_get_range(const RID& p_shaped) const override
	{
		return Vector2i();
	}

	virtual int64_t shaped_text_get_trim_pos(const RID& p_shaped) const override { return -1; }

	virtual int64_t shaped_text_get_ellipsis_pos(const RID& p_shaped) const override { return -1; }

	virtual const Glyph* shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const override
	{
		return nullptr;
	}

	virtual int64_t shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const override
	{
		return -1;
	}

	virtual Size2 shaped_text_get_size(const RID& p_shaped) const override { return Size2(); }

	virtual double shaped_text_get_ascent(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_descent(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_width(const RID& p_shaped) const override { return 0; }

	virtual double shaped_text_get_underline_position(const RID& p_shaped) const override
	{
		return 0;
	}

	virtual double shaped_text_get_underline_thickness(const RID& p_shaped) const override
	{
		return 0;
	}

	virtual void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) override
	{
	}
};



// --- End servers/text/text_server_dummy.h ---
// --- End servers/text/text_server.h ---

class TextLine;
class TextParagraph;

class Font : public Resource
{
	struct ShapedTextKey
	{
		String text;
		int font_size = 14;
		float width = 0.f;
		uint32_t jst_flags = TextServer::JUSTIFICATION_NONE;
		uint32_t brk_flags = TextServer::BREAK_MANDATORY;
		TextServer::Direction direction = TextServer::DIRECTION_AUTO;
		TextServer::Orientation orientation = TextServer::ORIENTATION_HORIZONTAL;

		bool operator==(const ShapedTextKey& p_b) const
		{
			return (font_size == p_b.font_size) && (width == p_b.width) &&
				   (jst_flags == p_b.jst_flags) && (brk_flags == p_b.brk_flags) &&
				   (direction == p_b.direction) && (orientation == p_b.orientation) &&
				   (text == p_b.text);
		}

		ShapedTextKey() {}

		ShapedTextKey(const String& p_text, int p_font_size, float p_width, uint32_t p_jst_flags,
			uint32_t p_brk_flags, TextServer::Direction p_direction,
			TextServer::Orientation p_orientation)
		{
			text = p_text;
			font_size = p_font_size;
			width = p_width;
			jst_flags = p_jst_flags;
			brk_flags = p_brk_flags;
			direction = p_direction;
			orientation = p_orientation;
		}
	};

	struct ShapedTextKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const ShapedTextKey& p_a)
		{
			uint32_t hash = p_a.text.hash();
			hash = hash_murmur3_one_32(p_a.font_size, hash);
			hash = hash_murmur3_one_float(p_a.width, hash);
			hash = hash_murmur3_one_32(p_a.brk_flags | (p_a.jst_flags << 6) |
										   (p_a.direction << 12) | (p_a.orientation << 15),
				hash);
			return hash_fmix32(hash);
		}
	};

	// Shaped string cache.
	mutable LRUCache<ShapedTextKey, Ref<TextLine>, ShapedTextKeyHasher> cache;
	mutable LRUCache<ShapedTextKey, Ref<TextParagraph>, ShapedTextKeyHasher> cache_wrap;

protected:
	// Output.
	mutable bool dirty_rids = true;

	// Fallbacks.
	static constexpr int MAX_FALLBACK_DEPTH = 64;

	virtual void reset_state() override;
	virtual void _update_rids_fb(const Font* p_f, int p_depth) const;
	virtual void _update_rids() const;

#ifndef DISABLE_DEPRECATED
	void _draw_string_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_string_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	void _draw_multiline_string_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		int p_size = 1, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	real_t _draw_char_bind_compat_104872(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	real_t _draw_char_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
		char32_t p_char, int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0)) const;
	static void _bind_compatibility_methods();
#endif

public:
	virtual bool _is_cyclic(const Ref<Font>& p_f, int p_depth) const;
	virtual bool _is_base_cyclic(const Ref<Font>& p_f, int p_depth) const;
	virtual void _invalidate_rids();

	static constexpr int DEFAULT_FONT_SIZE = 16;

	virtual RID _get_rid() const { return RID(); }

	// Font metrics.
	virtual real_t get_height(int p_font_size) const;
	virtual real_t get_ascent(int p_font_size) const;
	virtual real_t get_descent(int p_font_size) const;
	virtual real_t get_underline_position(int p_font_size) const;
	virtual real_t get_underline_thickness(int p_font_size) const;

	virtual String get_font_name() const;
	virtual String get_font_style_name() const;
	virtual uint32_t get_font_style() const;
	virtual int get_font_weight() const;
	virtual int get_font_stretch() const;

	virtual int64_t get_palette_count() const;
	virtual String get_palette_name(int64_t p_index) const;
	virtual Vector<Color> get_palette_colors(int64_t p_index) const;

	virtual int get_spacing(TextServer::SpacingType p_spacing) const { return 0; }

	// Drawing string.
	virtual void set_cache_capacity(int p_single_line, int p_multi_line);

	virtual Size2 get_string_size(const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;
	virtual Size2 get_multiline_string_size(const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL) const;

	virtual void draw_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	virtual void draw_multiline_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	virtual void draw_string_outline(RID p_canvas_item, const Point2& p_pos, const String& p_text,
		HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT, float p_width = -1,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;
	virtual void draw_multiline_string_outline(RID p_canvas_item, const Point2& p_pos,
		const String& p_text, HorizontalAlignment p_alignment = HORIZONTAL_ALIGNMENT_LEFT,
		float p_width = -1, int p_font_size = DEFAULT_FONT_SIZE, int p_max_lines = -1,
		int p_size = 1, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		uint32_t p_brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND,
		uint32_t p_jst_flags = TextServer::JUSTIFICATION_KASHIDA |
							   TextServer::JUSTIFICATION_WORD_BOUND,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL,
		float p_oversampling = 0.0) const;

	// Drawing char.
	virtual Size2 get_char_size(char32_t p_char, int p_font_size = DEFAULT_FONT_SIZE) const;
	virtual real_t draw_char(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, const Color& p_modulate = Color(1.0, 1.0, 1.0),
		float p_oversampling = 0.0) const;
	virtual real_t draw_char_outline(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
		int p_font_size = DEFAULT_FONT_SIZE, int p_size = 1,
		const Color& p_modulate = Color(1.0, 1.0, 1.0), float p_oversampling = 0.0) const;

	// Helper functions.
	virtual bool has_char(char32_t p_char) const;
	virtual String get_supported_chars() const;

	virtual bool is_language_supported(const String& p_language) const;
	virtual bool is_script_supported(const String& p_script) const;

	virtual int64_t get_face_count() const;

	Font();
	~Font();
};

class FontFile : public Font
{
	// Font source data.
	const uint8_t* data_ptr = nullptr;
	size_t data_size = 0;
	mutable PackedByteArray data;

	TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	bool mipmaps = false;
	bool disable_embedded_bitmaps = true;
	bool msdf = false;
	int msdf_pixel_range = 16;
	int msdf_size = 48;
	int fixed_size = 0;
	TextServer::FixedSizeScaleMode fixed_size_scale_mode = TextServer::FIXED_SIZE_SCALE_DISABLE;
	bool force_autohinter = false;
	bool modulate_color_glyphs = false;
	bool allow_system_fallback = true;
	TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
	TextServer::SubpixelPositioning subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_AUTO;
	bool keep_rounding_remainders = true;
	double oversampling_override = 0.0;

	HashMap<String, bool> language_support_overrides;
	HashMap<String, bool> script_support_overrides;

#ifndef DISABLE_DEPRECATED
	real_t bmp_height = 0.0;
	real_t bmp_ascent = 0.0;
#endif

	// Cache.
	mutable Vector<RID> cache;

	_FORCE_INLINE_ void _clear_cache();

	void _convert_packed_8bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_packed_4bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_rgba_4bit(Ref<Image>& p_source, int p_page, int p_sz);
	void _convert_mono_8bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol);
	void _convert_mono_4bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol);

protected:
	virtual void reset_state() override;

public:
	Error _load_bitmap_font(const String& p_path, List<String>* r_image_files);

	Error load_bitmap_font(const String& p_path);
	Error load_dynamic_font(const String& p_path);

	// Font source data.
	virtual void set_data_ptr(const uint8_t* p_data, size_t p_size);
	virtual void set_data(const PackedByteArray& p_data);
	virtual PackedByteArray get_data() const;

	// Common properties.
	virtual void set_font_name(const String& p_name);
	virtual void set_font_style_name(const String& p_name);
	virtual void set_font_style(uint32_t p_style);
	virtual void set_font_weight(int p_weight);
	virtual void set_font_stretch(int p_stretch);

	virtual void set_antialiasing(TextServer::FontAntialiasing p_antialiasing);
	virtual TextServer::FontAntialiasing get_antialiasing() const;

	virtual void set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps);
	virtual bool get_disable_embedded_bitmaps() const;

	virtual void set_generate_mipmaps(bool p_generate_mipmaps);
	virtual bool get_generate_mipmaps() const;

	virtual void set_multichannel_signed_distance_field(bool p_msdf);
	virtual bool is_multichannel_signed_distance_field() const;

	virtual void set_msdf_pixel_range(int p_msdf_pixel_range);
	virtual int get_msdf_pixel_range() const;

	virtual void set_msdf_size(int p_msdf_size);
	virtual int get_msdf_size() const;

	virtual void set_fixed_size(int p_fixed_size);
	virtual int get_fixed_size() const;

	virtual void set_fixed_size_scale_mode(TextServer::FixedSizeScaleMode p_fixed_size_scale_mode);
	virtual TextServer::FixedSizeScaleMode get_fixed_size_scale_mode() const;

	virtual void set_allow_system_fallback(bool p_allow_system_fallback);
	virtual bool is_allow_system_fallback() const;

	virtual void set_force_autohinter(bool p_force_autohinter);
	virtual bool is_force_autohinter() const;

	virtual void set_modulate_color_glyphs(bool p_modulate);
	virtual bool is_modulate_color_glyphs() const;

	virtual void set_hinting(TextServer::Hinting p_hinting);
	virtual TextServer::Hinting get_hinting() const;

	virtual void set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel);
	virtual TextServer::SubpixelPositioning get_subpixel_positioning() const;

	virtual void set_keep_rounding_remainders(bool p_keep_rounding_remainders);
	virtual bool get_keep_rounding_remainders() const;

	virtual void set_oversampling(real_t p_oversampling);
	virtual real_t get_oversampling() const;

	virtual RID _get_rid() const override;

	virtual int get_cache_count() const;
	virtual void clear_cache();
	virtual void remove_cache(int p_cache_index);

	virtual void clear_size_cache(int p_cache_index);
	virtual void remove_size_cache(int p_cache_index, const Vector2i& p_size);

	virtual void set_embolden(int p_cache_index, float p_strength);
	virtual float get_embolden(int p_cache_index) const;

	virtual void set_transform(int p_cache_index, Transform2D p_transform);
	virtual Transform2D get_transform(int p_cache_index) const;

	virtual void set_extra_spacing(
		int p_cache_index, TextServer::SpacingType p_spacing, int64_t p_value);
	virtual int64_t get_extra_spacing(int p_cache_index, TextServer::SpacingType p_spacing) const;

	virtual float get_extra_baseline_offset(int p_cache_index) const;
	virtual void set_extra_baseline_offset(int p_cache_index, float p_baseline_offset);

	virtual void set_face_index(int p_cache_index, int64_t p_index);
	virtual int64_t get_face_index(int p_cache_index) const;

	virtual void set_cache_ascent(int p_cache_index, int p_size, real_t p_ascent);
	virtual real_t get_cache_ascent(int p_cache_index, int p_size) const;

	virtual void set_cache_descent(int p_cache_index, int p_size, real_t p_descent);
	virtual real_t get_cache_descent(int p_cache_index, int p_size) const;

	virtual void set_cache_underline_position(
		int p_cache_index, int p_size, real_t p_underline_position);
	virtual real_t get_cache_underline_position(int p_cache_index, int p_size) const;

	virtual void set_cache_underline_thickness(
		int p_cache_index, int p_size, real_t p_underline_thickness);
	virtual real_t get_cache_underline_thickness(int p_cache_index, int p_size) const;

	virtual void set_cache_scale(int p_cache_index, int p_size,
		real_t p_scale); // Rendering scale for bitmap fonts (e.g. emoji fonts).
	virtual real_t get_cache_scale(int p_cache_index, int p_size) const;

	virtual int get_texture_count(int p_cache_index, const Vector2i& p_size) const;
	virtual void clear_textures(int p_cache_index, const Vector2i& p_size);
	virtual void remove_texture(int p_cache_index, const Vector2i& p_size, int p_texture_index);

	virtual void set_texture_image(
		int p_cache_index, const Vector2i& p_size, int p_texture_index, const Ref<Image>& p_image);
	virtual Ref<Image> get_texture_image(
		int p_cache_index, const Vector2i& p_size, int p_texture_index) const;

	virtual void set_texture_offsets(int p_cache_index, const Vector2i& p_size, int p_texture_index,
		const PackedInt32Array& p_offset);
	virtual PackedInt32Array get_texture_offsets(
		int p_cache_index, const Vector2i& p_size, int p_texture_index) const;

	virtual PackedInt32Array get_glyph_list(int p_cache_index, const Vector2i& p_size) const;
	virtual void clear_glyphs(int p_cache_index, const Vector2i& p_size);
	virtual void remove_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_glyph);

	virtual void set_glyph_advance(
		int p_cache_index, int p_size, int32_t p_glyph, const Vector2& p_advance);
	virtual Vector2 get_glyph_advance(int p_cache_index, int p_size, int32_t p_glyph) const;

	virtual void set_glyph_offset(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_offset);
	virtual Vector2 get_glyph_offset(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_size(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_gl_size);
	virtual Vector2 get_glyph_size(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_uv_rect(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Rect2& p_uv_rect);
	virtual Rect2 get_glyph_uv_rect(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void set_glyph_texture_idx(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph, int p_texture_idx);
	virtual int get_glyph_texture_idx(
		int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const;

	virtual void clear_kerning_map(int p_cache_index, int p_size);
	virtual void remove_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair);

	virtual void set_kerning(
		int p_cache_index, int p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning);
	virtual Vector2 get_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) const;

	virtual void render_range(
		int p_cache_index, const Vector2i& p_size, char32_t p_start, char32_t p_end);
	virtual void render_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_index);

	// Language/script support override.
	virtual void set_language_support_override(const String& p_language, bool p_supported);
	virtual bool get_language_support_override(const String& p_language) const;
	virtual void remove_language_support_override(const String& p_language);
	virtual Vector<String> get_language_support_overrides() const;

	virtual void set_script_support_override(const String& p_script, bool p_supported);
	virtual bool get_script_support_override(const String& p_script) const;
	virtual void remove_script_support_override(const String& p_script);
	virtual Vector<String> get_script_support_overrides() const;

	// Base font properties.
	virtual int32_t get_glyph_index(
		int p_size, char32_t p_char, char32_t p_variation_selector = 0x0000) const;
	virtual char32_t get_char_from_glyph_index(int p_size, int32_t p_glyph_index) const;

	FontFile();
	~FontFile();
};

class FontVariation : public Font
{
	struct Variation
	{
		real_t embolden = 0.f;
		int face_index = 0;
		Transform2D transform;
	};

	mutable Ref<Font> theme_font;

	Ref<Font> base_font;

	Variation variation;
	int extra_spacing[TextServer::SPACING_MAX];
	float baseline_offset = 0.0;
	int64_t palette_index = 0;
	Vector<Color> custom_colors;

protected:
	virtual void _update_rids() const override;

	virtual void reset_state() override;

public:
	virtual Ref<Font> get_base_font() const;
	virtual void set_base_font(const Ref<Font>& p_font);
	virtual Ref<Font> _get_base_font_or_default() const;

	virtual void set_variation_embolden(float p_strength);
	virtual float get_variation_embolden() const;

	virtual void set_variation_transform(Transform2D p_transform);
	virtual Transform2D get_variation_transform() const;

	virtual void set_variation_face_index(int p_face_index);
	virtual int get_variation_face_index() const;

	virtual void set_spacing(TextServer::SpacingType p_spacing, int p_value);
	virtual int get_spacing(TextServer::SpacingType p_spacing) const override;

	virtual float get_baseline_offset() const;
	virtual void set_baseline_offset(float p_baseline_offset);

	virtual int64_t get_palette_index() const;
	virtual void set_palette_index(int64_t p_palette_index);

	virtual Vector<Color> get_palette_custom_colors() const;
	virtual void set_palette_custom_colors(const Vector<Color>& p_colors);

	// Output.
	virtual RID _get_rid() const override;

	FontVariation();
	~FontVariation();
};

class SystemFont : public Font
{
	PackedStringArray names;
	bool italic = false;
	int weight = 400;
	int stretch = 100;

	mutable Ref<Font> theme_font;

	Ref<FontFile> base_font;
	Vector<int> face_indices;
	int ftr_weight = 0;
	int ftr_stretch = 0;
	int ftr_italic = 0;

	TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	bool mipmaps = false;
	bool disable_embedded_bitmaps = true;
	bool force_autohinter = false;
	bool modulate_color_glyphs = false;
	bool allow_system_fallback = true;
	TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
	TextServer::SubpixelPositioning subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_AUTO;
	bool keep_rounding_remainders = true;
	double oversampling_override = 0.0;
	bool msdf = false;
	int msdf_pixel_range = 16;
	int msdf_size = 48;

protected:
	virtual void _update_base_font();
	virtual void _update_rids() const override;

	virtual void reset_state() override;

public:
	virtual Ref<Font> get_base_font() const { return base_font; }

	virtual Ref<Font> _get_base_font_or_default() const;

	virtual void set_antialiasing(TextServer::FontAntialiasing p_antialiasing);
	virtual TextServer::FontAntialiasing get_antialiasing() const;

	virtual void set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps);
	virtual bool get_disable_embedded_bitmaps() const;

	virtual void set_generate_mipmaps(bool p_generate_mipmaps);
	virtual bool get_generate_mipmaps() const;

	virtual void set_allow_system_fallback(bool p_allow_system_fallback);
	virtual bool is_allow_system_fallback() const;

	virtual void set_force_autohinter(bool p_force_autohinter);
	virtual bool is_force_autohinter() const;

	virtual void set_modulate_color_glyphs(bool p_modulate);
	virtual bool is_modulate_color_glyphs() const;

	virtual void set_hinting(TextServer::Hinting p_hinting);
	virtual TextServer::Hinting get_hinting() const;

	virtual void set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel);
	virtual TextServer::SubpixelPositioning get_subpixel_positioning() const;

	virtual void set_keep_rounding_remainders(bool p_keep_rounding_remainders);
	virtual bool get_keep_rounding_remainders() const;

	virtual void set_oversampling(real_t p_oversampling);
	virtual real_t get_oversampling() const;

	virtual void set_multichannel_signed_distance_field(bool p_msdf);
	virtual bool is_multichannel_signed_distance_field() const;

	virtual void set_msdf_pixel_range(int p_msdf_pixel_range);
	virtual int get_msdf_pixel_range() const;

	virtual void set_msdf_size(int p_msdf_size);
	virtual int get_msdf_size() const;

	virtual void set_font_names(const PackedStringArray& p_names);
	virtual PackedStringArray get_font_names() const;

	virtual void set_font_italic(bool p_italic);
	virtual bool get_font_italic() const;

	virtual void set_font_weight(int p_weight);
	virtual int get_font_weight() const override;

	virtual void set_font_stretch(int p_stretch);
	virtual int get_font_stretch() const override;

	virtual int get_spacing(TextServer::SpacingType p_spacing) const override;

	virtual RID _get_rid() const override;

	int64_t get_face_count() const override;

	SystemFont();
	~SystemFont();
};

// --- Folded from scene/resources/text_line.h ---



/*************************************************************************/

class TextLine : public RefCounted
{
private:
	RID rid;

	mutable bool dirty = true;

	float width = -1.0;
	uint32_t flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA;
	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
	String el_char = U"…";
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_TRIM_ELLIPSIS;

	Vector<float> tab_stops;

protected:
#ifndef DISABLE_DEPRECATED
	void _draw_bind_compat_104872(
		RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	static void _bind_compatibility_methods();
#endif


	void _shape() const;

public:
	RID get_rid() const;

	void clear();
	Ref<TextLine> duplicate() const;

	void set_direction(TextServer::Direction p_direction);
	TextServer::Direction get_direction() const;
	TextServer::Direction get_inferred_direction() const;

	void set_orientation(TextServer::Orientation p_orientation);
	TextServer::Orientation get_orientation() const;

	void set_preserve_invalid(bool p_enabled);
	bool get_preserve_invalid() const;

	void set_preserve_control(bool p_enabled);
	bool get_preserve_control() const;

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void tab_align(const Vector<float>& p_tab_stops);

	void set_flags(uint32_t p_flags);
	uint32_t get_flags() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String& p_char);
	String get_ellipsis_char() const;

	void set_width(float p_width);
	float get_width() const;

	Size2 get_size() const;

	float get_line_ascent() const;
	float get_line_descent() const;
	float get_line_width() const;
	float get_line_underline_position() const;
	float get_line_underline_thickness() const;

	void draw(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	void draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	int hit_test(float p_coords) const;

	TextLine(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const String& p_language = "",
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL);
	TextLine();
	~TextLine();
};

// --- Folded from scene/resources/text_paragraph.h ---
#include "core/os/os.h"
#include "core/string/ustring.h"



/*************************************************************************/

class TextParagraph : public RefCounted
{
	_THREAD_SAFE_CLASS_

private:
	mutable RID dropcap_rid;
	mutable int dropcap_lines = 0;
	Rect2 dropcap_margins;

	RID rid;
	mutable LocalVector<RID> lines_rid;

	mutable bool lines_dirty = true;

	float line_spacing = 0.0;
	float width = -1.0;
	int max_lines_visible = -1;

	uint32_t brk_flags = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND;
	uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
						 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
						 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	String el_char = U"…";
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;

	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;

	Vector<float> tab_stops;

protected:
#ifndef DISABLE_DEPRECATED
	void _draw_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		const Color& p_color = Color(1, 1, 1), const Color& p_dc_color = Color(1, 1, 1)) const;
	void _draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1),
		const Color& p_dc_color = Color(1, 1, 1)) const;
	void _draw_line_bind_compat_104872(RID p_canvas, const Vector2& p_pos, int p_line,
		const Color& p_color = Color(1, 1, 1)) const;
	void _draw_line_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos, int p_line,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_dropcap_bind_compat_104872(
		RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1)) const;
	void _draw_dropcap_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
		int p_outline_size = 1, const Color& p_color = Color(1, 1, 1)) const;

	static void _bind_compatibility_methods();
#endif

	void _shape_lines() const;

public:
	RID get_rid() const;
	RID get_line_rid(int p_line) const;
	RID get_dropcap_rid() const;

	void clear();

	Ref<TextParagraph> duplicate() const;

	void set_direction(TextServer::Direction p_direction);
	TextServer::Direction get_direction() const;
	TextServer::Direction get_inferred_direction() const;

	void set_orientation(TextServer::Orientation p_orientation);
	TextServer::Orientation get_orientation() const;

	void set_preserve_invalid(bool p_enabled);
	bool get_preserve_invalid() const;

	void set_preserve_control(bool p_enabled);
	bool get_preserve_control() const;

	void set_custom_punctuation(const String& p_punct);
	String get_custom_punctuation() const;

	bool set_dropcap(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const Rect2& p_dropcap_margins = Rect2(), const String& p_language = "");
	void clear_dropcap();

	void set_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_alignment() const;

	void tab_align(const Vector<float>& p_tab_stops);

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_break_flags(uint32_t p_flags);
	uint32_t get_break_flags() const;

	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	TextServer::OverrunBehavior get_text_overrun_behavior() const;

	void set_ellipsis_char(const String& p_char);
	String get_ellipsis_char() const;

	void set_width(float p_width);
	float get_width() const;

	void set_max_lines_visible(int p_lines);
	int get_max_lines_visible() const;

	void set_line_spacing(float p_spacing);
	float get_line_spacing() const;

	Size2 get_non_wrapped_size() const;

	Size2 get_size() const;

	Vector2i get_range() const;

	int get_line_count() const;

	Size2 get_line_size(int p_line) const;
	float get_line_ascent(int p_line) const;
	float get_line_descent(int p_line) const;
	float get_line_width(int p_line) const;
	Vector2i get_line_range(int p_line) const;
	float get_line_underline_position(int p_line) const;
	float get_line_underline_thickness(int p_line) const;

	Size2 get_dropcap_size() const;
	int get_dropcap_lines() const;

	void draw(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		const Color& p_dc_color = Color(1, 1, 1), float p_oversampling = 0.0) const;
	void draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), const Color& p_dc_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;

	void draw_line(RID p_canvas, const Vector2& p_pos, int p_line,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;
	void draw_line_outline(RID p_canvas, const Vector2& p_pos, int p_line, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	void draw_dropcap(RID p_canvas, const Vector2& p_pos, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const;
	void draw_dropcap_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size = 1,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const;

	int hit_test(const Point2& p_coords) const;

	bool is_dirty();

	Mutex& get_mutex() const { return _thread_safe_; }

	TextParagraph(const String& p_text, const Ref<Font>& p_font, int p_font_size,
		const String& p_language = "", float p_width = -1.f,
		TextServer::Direction p_direction = TextServer::DIRECTION_AUTO,
		TextServer::Orientation p_orientation = TextServer::ORIENTATION_HORIZONTAL);
	TextParagraph();
	~TextParagraph();
};
// --- End scene/resources/text_line.h ---
// --- End scene/resources/font.h ---
// --- Folded from scene/resources/style_box.h ---
#include "core/io/ip.h"


class CanvasItem;

class StyleBox : public Resource
{
public:
	enum StyleType
	{
		STYLE_EMPTY,
		STYLE_FLAT,
		STYLE_TEXTURE,
		STYLE_LINE,
	};

	enum AxisStretchMode
	{
		AXIS_STRETCH_MODE_STRETCH,
		AXIS_STRETCH_MODE_TILE,
		AXIS_STRETCH_MODE_TILE_FIT,
	};

private:
	StyleType style_type = STYLE_EMPTY;
	float content_margin[4];

	// Shared by STYLE_FLAT and STYLE_TEXTURE.
	real_t expand_margin[4] = {};
	bool draw_center = true;

	// STYLE_FLAT fields.
	Color bg_color = Color(0.6, 0.6, 0.6);
	Color shadow_color = Color(0, 0, 0, 0.6);
	Color border_color = Color(0.8, 0.8, 0.8);
	real_t border_width[4] = {};
	real_t corner_radius[4] = {};
	bool blend_border = false;
	Vector2 skew;
	bool anti_aliased = true;
	int corner_detail = 8;
	int shadow_size = 0;
	Point2 shadow_offset;
	real_t aa_size = 1;

	// STYLE_TEXTURE fields.
	float texture_margin[4] = {};
	Rect2 region_rect;
	Ref<Texture2D> texture;
	Color modulate = Color(1, 1, 1, 1);
	AxisStretchMode axis_h = AXIS_STRETCH_MODE_STRETCH;
	AxisStretchMode axis_v = AXIS_STRETCH_MODE_STRETCH;

	// STYLE_LINE fields.
	Color color;
	int thickness = 1;
	bool vertical = false;
	float grow_begin = 1.0;
	float grow_end = 1.0;

	void _draw_flat(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_texture(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_line(RID p_canvas_item, const Rect2& p_rect) const;

protected:
	float get_style_margin(Side p_side) const;

public:
	void set_style_type(StyleType p_type);
	StyleType get_style_type() const { return style_type; }

	Size2 get_minimum_size() const;

	void set_content_margin(Side p_side, float p_value);
	void set_content_margin_all(float p_value);
	void set_content_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_content_margin(Side p_side) const;

	float get_margin(Side p_side) const;
	Point2 get_offset() const;

	void draw(RID p_canvas_item, const Rect2& p_rect) const;
	Rect2 get_draw_rect(const Rect2& p_rect) const;

	CanvasItem* get_current_item_drawn() const;

	bool test_mask(const Point2& p_point, const Rect2& p_rect) const;

	// Shared STYLE_FLAT / STYLE_TEXTURE properties.
	void set_expand_margin(Side p_expand_side, float p_size);
	void set_expand_margin_all(float p_expand_margin_size);
	void set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_expand_margin(Side p_expand_side) const;

	void set_draw_center(bool p_enabled);
	bool is_draw_center_enabled() const;

	// STYLE_FLAT properties.
	void set_bg_color(const Color& p_color);
	Color get_bg_color() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_border_width_all(int p_size);
	int get_border_width_min() const;

	void set_border_width(Side p_side, int p_width);
	int get_border_width(Side p_side) const;

	void set_border_blend(bool p_blend);
	bool get_border_blend() const;

	void set_corner_radius_all(int radius);
	void set_corner_radius_individual(const int radius_top_left, const int radius_top_right,
		const int radius_bottom_right, const int radius_bottom_left);
	void set_corner_radius(Corner p_corner, const int radius);
	int get_corner_radius(Corner p_corner) const;

	void set_corner_detail(const int& p_corner_detail);
	int get_corner_detail() const;

	void set_skew(Vector2 p_skew);
	Vector2 get_skew() const;

	void set_shadow_color(const Color& p_color);
	Color get_shadow_color() const;

	void set_shadow_size(const int& p_size);
	int get_shadow_size() const;

	void set_shadow_offset(const Point2& p_offset);
	Point2 get_shadow_offset() const;

	void set_anti_aliased(const bool& p_anti_aliased);
	bool is_anti_aliased() const;
	void set_aa_size(const real_t p_aa_size);
	real_t get_aa_size() const;

	// STYLE_TEXTURE properties.
	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_margin(Side p_side, float p_size);
	void set_texture_margin_all(float p_size);
	void set_texture_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_texture_margin(Side p_side) const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;

	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	void set_modulate(const Color& p_modulate);
	Color get_modulate() const;

	// STYLE_LINE properties.
	void set_color(const Color& p_color);
	Color get_color() const;

	void set_thickness(int p_thickness);
	int get_thickness() const;

	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	void set_grow_begin(float p_grow);
	float get_grow_begin() const;

	void set_grow_end(float p_grow);
	float get_grow_end() const;

	StyleBox(StyleType p_type = STYLE_EMPTY);
};
// --- End scene/resources/style_box.h ---


class Theme : public Resource
{
#ifdef TOOLS_ENABLED
	friend class ThemeItemImportTree;
	friend class ThemeItemEditorDialog;
	friend class ThemeTypeEditor;
#endif

public:
	using ThemeIconMap = HashMap<String, Ref<Texture2D>>;
	using ThemeStyleMap = HashMap<String, Ref<StyleBox>>;
	using ThemeFontMap = HashMap<String, Ref<Font>>;
	using ThemeFontSizeMap = HashMap<String, int>;
	using ThemeColorMap = HashMap<String, Color>;
	using ThemeConstantMap = HashMap<String, int>;

	enum DataType
	{
		DATA_TYPE_COLOR,
		DATA_TYPE_CONSTANT,
		DATA_TYPE_FONT,
		DATA_TYPE_FONT_SIZE,
		DATA_TYPE_ICON,
		DATA_TYPE_STYLEBOX,
		DATA_TYPE_MAX
	};

private:
	bool no_change_propagation = false;

	void _emit_theme_changed(bool p_notify_list_changed = false);

	Vector<String> _get_icon_list(const String& p_theme_type) const;
	Vector<String> _get_icon_type_list() const;
	Vector<String> _get_stylebox_list(const String& p_theme_type) const;
	Vector<String> _get_stylebox_type_list() const;
	Vector<String> _get_font_list(const String& p_theme_type) const;
	Vector<String> _get_font_type_list() const;
	Vector<String> _get_font_size_list(const String& p_theme_type) const;
	Vector<String> _get_font_size_type_list() const;
	Vector<String> _get_color_list(const String& p_theme_type) const;
	Vector<String> _get_color_type_list() const;
	Vector<String> _get_constant_list(const String& p_theme_type) const;
	Vector<String> _get_constant_type_list() const;

	Vector<String> _get_theme_item_list(DataType p_data_type, const String& p_theme_type) const;
	Vector<String> _get_theme_item_type_list(DataType p_data_type) const;

	Vector<String> _get_type_variation_list(const String& p_theme_type) const;
	Vector<String> _get_type_list() const;

protected:
	// Default values configurable for each individual theme.
	float default_base_scale = 0.0;
	Ref<Font> default_font;
	int default_font_size = -1;

	HashMap<String, ThemeIconMap> icon_map;
	HashMap<String, ThemeStyleMap> style_map;
	HashMap<String, ThemeFontMap> font_map;
	HashMap<String, ThemeFontSizeMap> font_size_map;
	HashMap<String, ThemeColorMap> color_map;
	HashMap<String, ThemeConstantMap> constant_map;
	HashMap<String, String> variation_map;
	HashMap<String, List<String>> variation_base_map;


	void _freeze_change_propagation();
	void _unfreeze_and_propagate_changes();

	virtual void reset_state() override;

public:
	static bool is_valid_type_name(const String& p_name);
	static bool is_valid_item_name(const String& p_name);
	static String validate_type_name(const String& p_name);

	void set_default_base_scale(float p_base_scale);
	float get_default_base_scale() const;
	bool has_default_base_scale() const;

	void set_default_font(const Ref<Font>& p_default_font);
	Ref<Font> get_default_font() const;
	bool has_default_font() const;

	void set_default_font_size(int p_font_size);
	int get_default_font_size() const;
	bool has_default_font_size() const;

	void set_icon(
		const String& p_name, const String& p_theme_type, const Ref<Texture2D>& p_icon);
	virtual Ref<Texture2D> get_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon(const String& p_name, const String& p_theme_type) const;
	bool has_icon_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_icon(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_icon(const String& p_name, const String& p_theme_type);
	void get_icon_list(const String& p_theme_type, List<String>* p_list) const;
	void add_icon_type(const String& p_theme_type);
	void remove_icon_type(const String& p_theme_type);
	void rename_icon_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_icon_type_list(List<String>* p_list) const;

	virtual Ref<StyleBox> get_stylebox(
		const String& p_name, const String& p_theme_type) const;
	bool has_stylebox(const String& p_name, const String& p_theme_type) const;
	bool has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_stylebox(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_stylebox(const String& p_name, const String& p_theme_type);
	void get_stylebox_list(const String& p_theme_type, List<String>* p_list) const;
	void add_stylebox_type(const String& p_theme_type);
	void remove_stylebox_type(const String& p_theme_type);
	void rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_stylebox_type_list(List<String>* p_list) const;

	void set_font(
		const String& p_name, const String& p_theme_type, const Ref<Font>& p_font);
	virtual Ref<Font> get_font(const String& p_name, const String& p_theme_type) const;
	bool has_font(const String& p_name, const String& p_theme_type) const;
	bool has_font_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font(const String& p_name, const String& p_theme_type);
	void get_font_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_type(const String& p_theme_type);
	void remove_font_type(const String& p_theme_type);
	void rename_font_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_type_list(List<String>* p_list) const;

	void set_font_size(const String& p_name, const String& p_theme_type, int p_font_size);
	virtual int get_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_no_default(const String& p_name, const String& p_theme_type) const;
	bool has_font_size_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_font_size(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_font_size(const String& p_name, const String& p_theme_type);
	void get_font_size_list(const String& p_theme_type, List<String>* p_list) const;
	void add_font_size_type(const String& p_theme_type);
	void remove_font_size_type(const String& p_theme_type);
	void rename_font_size_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_font_size_type_list(List<String>* p_list) const;

	void set_color(const String& p_name, const String& p_theme_type, const Color& p_color);
	virtual Color get_color(const String& p_name, const String& p_theme_type) const;
	bool has_color(const String& p_name, const String& p_theme_type) const;
	bool has_color_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_color(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_color(const String& p_name, const String& p_theme_type);
	void get_color_list(const String& p_theme_type, List<String>* p_list) const;
	void add_color_type(const String& p_theme_type);
	void remove_color_type(const String& p_theme_type);
	void rename_color_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_color_type_list(List<String>* p_list) const;

	void set_constant(const String& p_name, const String& p_theme_type, int p_constant);
	virtual int get_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant(const String& p_name, const String& p_theme_type) const;
	bool has_constant_nocheck(const String& p_name, const String& p_theme_type) const;
	void rename_constant(
		const String& p_old_name, const String& p_name, const String& p_theme_type);
	void clear_constant(const String& p_name, const String& p_theme_type);
	void get_constant_list(const String& p_theme_type, List<String>* p_list) const;
	void add_constant_type(const String& p_theme_type);
	void remove_constant_type(const String& p_theme_type);
	void rename_constant_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_constant_type_list(List<String>* p_list) const;

	bool has_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	bool has_theme_item_nocheck(
		DataType p_data_type, const String& p_name, const String& p_theme_type) const;
	void rename_theme_item(DataType p_data_type, const String& p_old_name,
		const String& p_name, const String& p_theme_type);
	void clear_theme_item(
		DataType p_data_type, const String& p_name, const String& p_theme_type);
	void get_theme_item_list(
		DataType p_data_type, const String& p_theme_type, List<String>* p_list) const;
	void add_theme_item_type(DataType p_data_type, const String& p_theme_type);
	void remove_theme_item_type(DataType p_data_type,
const String& p_theme_type);
	void rename_theme_item_type(
		DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type);
	void get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const;

	void set_type_variation(const String& p_theme_type, const String& p_base_type);
	bool is_type_variation(const String& p_theme_type, const String& p_base_type) const;
	void clear_type_variation(const String& p_theme_type);
	String get_type_variation_base(const String& p_theme_type) const;
	void get_type_variation_list(const String& p_base_type, List<String>* p_list) const;

	void add_type(const String& p_theme_type);
	void remove_type(const String& p_theme_type);
	void rename_type(const String& p_old_theme_type, const String& p_theme_type);
	void get_type_list(List<String>* p_list) const;
	void get_type_dependencies(const String& p_base_type, const String& p_type_variant,
		Vector<String>& r_result);

	void merge_with(const Ref<Theme>& p_other);
	void clear();

	Theme();
	~Theme();
};

// --- Folded from scene/theme/theme_db.h ---
#include <functional>


class Font;
class Node;
class StyleBox;
class Texture2D;
class ThemeContext;

class ThemeDB final
{
public:
	typedef std::function<void(Node*, const String&, const String&)> ThemeItemSetter;

	struct ThemeItemBind
	{
		Theme::DataType data_type;
		String class_name;
		String item_name;
		String type_name;
		bool external = false;

		ThemeItemSetter setter;

		struct SortByType
		{
			_FORCE_INLINE_ bool operator()(const ThemeItemBind& l, const ThemeItemBind& r) const
			{
				return l.data_type < r.data_type;
			}
		};
	};

	struct Data
	{
		// Global Theme resources used by the default theme context.
		Ref<Theme> default_theme;
		Ref<Theme> project_theme;

		// Universal default values, final fallback for every theme.
		float fallback_base_scale = 1.0f;
		Ref<Font> fallback_font;
		int fallback_font_size = 16;
		Ref<Texture2D> fallback_icon;
		Ref<StyleBox> fallback_stylebox;

		// Global theme contexts used to scope global Theme resources.
		ThemeContext* default_theme_context = nullptr;
		HashMap<Node*, ThemeContext*> theme_contexts;

		HashMap<String, HashMap<String, ThemeItemBind>> theme_item_binds;
		HashMap<String, List<ThemeItemBind>> theme_item_binds_list;
	};

	static inline Data* data = nullptr;

private:
	static void _propagate_theme_context(Node* p_from_node, ThemeContext* p_context);
	static void _init_default_theme_context();
	static void _finalize_theme_contexts();
	static void _sort_theme_items();

public:
	ThemeDB() = delete;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void initialize_theme();
	static void initialize_theme_noproject();
	static void finalize_theme();

	// Global Theme resources.
	static void set_default_theme(const Ref<Theme>& p_default);
	static Ref<Theme> get_default_theme();

	static void set_project_theme(const Ref<Theme>& p_project_default);
	static Ref<Theme> get_project_theme();

	// Universal fallback values.
	static void set_fallback_base_scale(float p_base_scale);
	static float get_fallback_base_scale();

	static void set_fallback_font(const Ref<Font>& p_font);
	static Ref<Font> get_fallback_font();

	static void set_fallback_font_size(int p_font_size);
	static int get_fallback_font_size();

	static void set_fallback_icon(const Ref<Texture2D>& p_icon);
	static Ref<Texture2D> get_fallback_icon();

	static void set_fallback_stylebox(const Ref<StyleBox>& p_stylebox);
	static Ref<StyleBox> get_fallback_stylebox();

	static void get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result);

	// Global theme contexts.
	static ThemeContext* create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes);
	static void destroy_theme_context(Node* p_node);

	static ThemeContext* get_theme_context(Node* p_node);
	static ThemeContext* get_default_theme_context();
	static ThemeContext* get_nearest_theme_context(Node* p_for_node);

	// Theme item binding.
	static void bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter);
	static void bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
		const String& p_prop_name, const String& p_item_name, const String& p_type_name,
		ThemeItemSetter p_setter);
	static void update_class_instance_items(Node* p_instance);

	static void get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
		bool p_include_inherited = false, Theme::DataType p_filter_type = Theme::DATA_TYPE_MAX);
};

class ThemeContext
{
	friend class ThemeDB;

	Node* node = nullptr;
	ThemeContext* parent = nullptr;

	// Themes are stacked in the order of relevance, for easy iteration.
	// This means that the first theme is the one you should check first,
	// and the last theme is the fallback theme where every lookup ends.
	Vector<Ref<Theme>> themes;

	void _emit_changed();

public:
	void set_themes(Vector<Ref<Theme>>& p_themes);
	const Vector<Ref<Theme>> get_themes() const;
	Ref<Theme> get_fallback_theme() const;
};

// --- Folded from scene/theme/theme_owner.h ---


class Control;
class Node;
class ThemeContext;

class ThemeOwner
{
	Node* holder = nullptr;

	Node* owner_node = nullptr;
	ThemeContext* owner_context = nullptr;

	void _owner_context_changed();
	ThemeContext* _get_active_owner_context() const;

	Node* _get_next_owner_node(Node* p_from_node) const;
	Ref<Theme> _get_owner_node_theme(Node* p_owner_node) const;

public:
	// Theme owner node.
	void set_owner_node(Node* p_node);

	Node* get_owner_node() const { return owner_node; }

	bool has_owner_node() const { return owner_node != nullptr; }

	void set_owner_context(ThemeContext* p_context, bool p_propagate = true);

	// Theme propagation.

	void assign_theme_on_parented(Node* p_for_node);
	void clear_theme_on_unparented(Node* p_for_node);
	void propagate_theme_changed(Node* p_to_node, Node* p_owner_node, bool p_notify, bool p_assign);

	// Theme lookup.

	void get_theme_type_dependencies(
		const Node* p_for_node, const String& p_theme_type, Vector<String>& r_result) const;

	bool has_theme_item_in_types(Theme::DataType p_data_type, const String& p_name,
		const Vector<String>& p_theme_types);

	float get_theme_default_base_scale();
	Ref<Font> get_theme_default_font();
	int get_theme_default_font_size();

	ThemeOwner(Node* p_holder) { holder = p_holder; }
};



// --- End scene/theme/theme_owner.h ---
// --- End scene/theme/theme_db.h ---
// --- End scene/resources/theme.h ---
