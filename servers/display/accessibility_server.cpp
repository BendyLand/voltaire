/**************************************************************************/
/*  accessibility_server.cpp                                              */
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

#include "accessibility_server.h"
#include "servers/display/accessibility_server_dummy.h"

AccessibilityServerDriver::AccessibilityServerDriver()
{
	AccessibilityServer::_bind_driver(this);
}

AccessibilityServerDriver::~AccessibilityServerDriver()
{
	AccessibilityServer::_unbind_driver(this);
}

AccessibilityServer::AccessibilityServerCreate
	AccessibilityServer::server_create_functions[AccessibilityServer::MAX_SERVERS] = {
		{"dummy", &AccessibilityServerDummy::create_func}};

int AccessibilityServer::server_create_count = 1;

void AccessibilityServer::_bind_driver(AccessibilityServerDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void AccessibilityServer::_unbind_driver(AccessibilityServerDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

AccessibilityServerDriver* AccessibilityServer::create(int p_index, Error& r_error)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	if (data && data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	AccessibilityServerDriver* driver = server_create_functions[p_index].create_function(r_error);
	if (r_error != OK) {
		if (driver) {
			memdelete(driver);
		}
		return nullptr;
	}
	if (driver) {
		_bind_driver(driver);
	}
	return driver;
}

void AccessibilityServer::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void AccessibilityServer::register_create_function(const char* p_name, CreateFunction p_function)
{
	ERR_FAIL_COND(server_create_count == MAX_SERVERS);
	// Dummy server is always last
	server_create_functions[server_create_count] = server_create_functions[server_create_count - 1];
	server_create_functions[server_create_count - 1].name = p_name;
	server_create_functions[server_create_count - 1].create_function = p_function;
	server_create_count++;
}

int AccessibilityServer::get_create_function_count() { return server_create_count; }

const char* AccessibilityServer::get_create_function_name(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].name;
}

bool AccessibilityServer::is_supported()
{
	return is_initialized() ? data->driver->is_supported() : false;
}

bool AccessibilityServer::window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle)
{
	return is_initialized() ? data->driver->window_create(p_window_id, p_handle) : true;
}

void AccessibilityServer::window_destroy(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_destroy(p_window_id);
	}
}

RID AccessibilityServer::create_element(
	DisplayServerEnums::WindowID p_window_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	return is_initialized() ? data->driver->create_element(p_window_id, p_role) : RID();
}

RID AccessibilityServer::create_sub_element(const RID& p_parent_rid,
	AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	return is_initialized() ? data->driver->create_sub_element(p_parent_rid, p_role, p_insert_pos)
							: RID();
}

RID AccessibilityServer::create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	return is_initialized() ? data->driver->create_sub_text_edit_elements(
								  p_parent_rid, p_shaped_text, p_min_height, p_insert_pos,
								  p_is_last_line)
							: RID();
}

bool AccessibilityServer::has_element(const RID& p_id)
{
	return is_initialized() ? data->driver->has_element(p_id) : false;
}

void AccessibilityServer::free_element(const RID& p_id)
{
	if (is_initialized()) {
		data->driver->free_element(p_id);
	}
}

RID AccessibilityServer::get_window_root(DisplayServerEnums::WindowID p_window_id)
{
	return is_initialized() ? data->driver->get_window_root(p_window_id) : RID();
}

void AccessibilityServer::update_set_focus(const RID& p_id)
{
	if (is_initialized()) {
		data->driver->update_set_focus(p_id);
	}
}

void AccessibilityServer::set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	if (is_initialized()) {
		data->driver->set_window_rect(p_window_id, p_rect_out, p_rect_in);
	}
}

void AccessibilityServer::set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	if (is_initialized()) {
		data->driver->set_window_focused(p_window_id, p_focused);
	}
}

void AccessibilityServer::window_activation_completed(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_activation_completed(p_window_id);
	}
}

void AccessibilityServer::window_deactivation_completed(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_deactivation_completed(p_window_id);
	}
}

void AccessibilityServer::update_set_role(
	const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	if (is_initialized()) {
		data->driver->update_set_role(p_id, p_role);
	}
}

void AccessibilityServer::update_set_name(const RID& p_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->update_set_name(p_id, p_name);
	}
}

void AccessibilityServer::update_set_braille_label(const RID& p_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->update_set_braille_label(p_id, p_name);
	}
}

void AccessibilityServer::update_set_braille_role_description(
	const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_braille_role_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	if (is_initialized()) {
		data->driver->update_set_extra_info(p_id, p_name_extra_info);
	}
}

void AccessibilityServer::update_set_description(const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_value(const RID& p_id, const String& p_value)
{
	if (is_initialized()) {
		data->driver->update_set_value(p_id, p_value);
	}
}

void AccessibilityServer::update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	if (is_initialized()) {
		data->driver->update_set_tooltip(p_id, p_tooltip);
	}
}

void AccessibilityServer::update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	if (is_initialized()) {
		data->driver->update_set_bounds(p_id, p_rect);
	}
}

void AccessibilityServer::update_set_transform(const RID& p_id, const Transform2D& p_transform)
{
	if (is_initialized()) {
		data->driver->update_set_transform(p_id, p_transform);
	}
}

void AccessibilityServer::update_add_child(const RID& p_id, const RID& p_child_id)
{
	if (is_initialized()) {
		data->driver->update_add_child(p_id, p_child_id);
	}
}

void AccessibilityServer::update_add_related_controls(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_controls(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_details(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_details(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_described_by(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_flow_to(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_flow_to(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_labeled_by(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_labeled_by(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_radio_group(p_id, p_related_id);
	}
}

void AccessibilityServer::update_set_active_descendant(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_active_descendant(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_next_on_line(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_previous_on_line(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_previous_on_line(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	if (is_initialized()) {
		data->driver->update_set_member_of(p_id, p_group_id);
	}
}

void AccessibilityServer::update_set_in_page_link_target(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_in_page_link_target(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_error_message(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_live(
	const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live)
{
	if (is_initialized()) {
		data->driver->update_set_live(p_id, p_live);
	}
}

void AccessibilityServer::update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	if (is_initialized()) {
		data->driver->update_add_custom_action(p_id, p_action_id, p_action_description);
	}
}

void AccessibilityServer::update_set_table_row_count(const RID& p_id, int p_count)
{
	if (is_initialized()) {
		data->driver->update_set_table_row_count(p_id, p_count);
	}
}

void AccessibilityServer::update_set_table_column_count(const RID& p_id, int p_count)
{
	if (is_initialized()) {
		data->driver->update_set_table_column_count(p_id, p_count);
	}
}

void AccessibilityServer::update_set_table_row_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_row_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_table_column_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_column_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_cell_position(p_id, p_row_index, p_column_index);
	}
}

void AccessibilityServer::update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	if (is_initialized()) {
		data->driver->update_set_table_cell_span(p_id, p_row_span, p_column_span);
	}
}

void AccessibilityServer::update_set_list_item_count(const RID& p_id, int p_size)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_count(p_id, p_size);
	}
}

void AccessibilityServer::update_set_list_item_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_list_item_level(const RID& p_id, int p_level)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_level(p_id, p_level);
	}
}

void AccessibilityServer::update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_selected(p_id, p_selected);
	}
}

void AccessibilityServer::update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_expanded(p_id, p_expanded);
	}
}

void AccessibilityServer::update_set_popup_type(
	const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup)
{
	if (is_initialized()) {
		data->driver->update_set_popup_type(p_id, p_popup);
	}
}

void AccessibilityServer::update_set_checked(const RID& p_id, bool p_checekd)
{
	if (is_initialized()) {
		data->driver->update_set_checked(p_id, p_checekd);
	}
}

void AccessibilityServer::update_set_num_value(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_num_value(p_id, p_position);
	}
}

void AccessibilityServer::update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_num_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_num_step(const RID& p_id, double p_step)
{
	if (is_initialized()) {
		data->driver->update_set_num_step(p_id, p_step);
	}
}

void AccessibilityServer::update_set_num_jump(const RID& p_id, double p_jump)
{
	if (is_initialized()) {
		data->driver->update_set_num_jump(p_id, p_jump);
	}
}

void AccessibilityServer::update_set_scroll_x(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_x(p_id, p_position);
	}
}

void AccessibilityServer::update_set_scroll_x_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_x_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_scroll_y(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_y(p_id, p_position);
	}
}

void AccessibilityServer::update_set_scroll_y_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_y_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_text_decorations(const RID& p_id, bool p_underline,
	bool p_strikethrough, bool p_overline, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_text_decorations(
			p_id, p_underline, p_strikethrough, p_overline, p_color);
	}
}

void AccessibilityServer::update_set_text_align(const RID& p_id, HorizontalAlignment p_align)
{
	if (is_initialized()) {
		data->driver->update_set_text_align(p_id, p_align);
	}
}

void AccessibilityServer::update_set_text_selection(const RID& p_id, const RID& p_text_start_id,
	int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	if (is_initialized()) {
		data->driver->update_set_text_selection(
			p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
	}
}

void AccessibilityServer::update_set_flag(
	const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	if (is_initialized()) {
		data->driver->update_set_flag(p_id, p_flag, p_value);
	}
}

void AccessibilityServer::update_set_classname(const RID& p_id, const String& p_classname)
{
	if (is_initialized()) {
		data->driver->update_set_classname(p_id, p_classname);
	}
}

void AccessibilityServer::update_set_placeholder(const RID& p_id, const String& p_placeholder)
{
	if (is_initialized()) {
		data->driver->update_set_placeholder(p_id, p_placeholder);
	}
}

void AccessibilityServer::update_set_language(const RID& p_id, const String& p_language)
{
	if (is_initialized()) {
		data->driver->update_set_language(p_id, p_language);
	}
}

void AccessibilityServer::update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	if (is_initialized()) {
		data->driver->update_set_text_orientation(p_id, p_vertical);
	}
}

void AccessibilityServer::update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	if (is_initialized()) {
		data->driver->update_set_list_orientation(p_id, p_vertical);
	}
}

void AccessibilityServer::update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	if (is_initialized()) {
		data->driver->update_set_shortcut(p_id, p_shortcut);
	}
}

void AccessibilityServer::update_set_url(const RID& p_id, const String& p_url)
{
	if (is_initialized()) {
		data->driver->update_set_url(p_id, p_url);
	}
}

void AccessibilityServer::update_set_role_description(const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_role_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_state_description(
	const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_state_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_color_value(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_color_value(p_id, p_color);
	}
}

void AccessibilityServer::update_set_background_color(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_background_color(p_id, p_color);
	}
}

void AccessibilityServer::update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_foreground_color(p_id, p_color);
	}
}
