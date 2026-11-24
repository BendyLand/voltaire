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

#include "core/input/input_event.h"
#include "core/io/image.h"
#include "core/io/resource.h"
#include "core/input/input_event.h"
#include "core/os/os.h"
#include "core/types.h"
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
#include "core/input/input_event.h"
#include "core/types.h"

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
#include "core/types.h"
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
#include "core/io/resource.h"

#include "core/types.h"

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
#include "core/os/thread.h"
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
#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/types.h"


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
