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

#include "core/input/input_enums.h"
#include "core/io/image.h"
#include "core/io/resource.h"
#include "core/os/keyboard.h"
#include "core/os/process_id.h"
#include "core/types.h"
#include "servers/display/display_server_enums.h"

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
