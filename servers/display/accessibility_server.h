/**************************************************************************/
/*  accessibility_server.h                                                */
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
#include "servers/display/display_server_enums.h"

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
