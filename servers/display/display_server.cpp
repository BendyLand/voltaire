/**************************************************************************/
/*  display_server.cpp                                                    */
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
// Inlined servers/display/display_server.compat.inc
#ifndef DISABLE_DEPRECATED

#include "display_server.h"

RID DisplayServer::_accessibility_create_sub_text_edit_elements_bind_compat_113459(
	const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height, int p_insert_pos)
{
	return accessibility_create_sub_text_edit_elements(
		p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, false);
}

#endif // DISABLE_DEPRECATED



#include "display_server.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Input);
STATIC_ASSERT_INCOMPLETE_TYPE(class, NativeMenu);
STATIC_ASSERT_INCOMPLETE_TYPE(class, Texture2D);
STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/input/input.h"
#include "core/os/os.h"
#include "scene/resources/texture.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server_headless.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/renderer.h"

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"
#endif

#if defined(VULKAN_ENABLED)
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif
#if defined(D3D12_ENABLED)
#include "drivers/d3d12/rendering_context_driver_d3d12.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif


bool DisplayServer::window_early_clear_override_enabled = false;
Color DisplayServer::window_early_clear_override_color = Color(0, 0, 0, 0);

DisplayServer::DisplayServerCreate
	DisplayServer::server_create_functions[DisplayServer::MAX_SERVERS] = {{"headless",
		&DisplayServerHeadless::create_func, &DisplayServerHeadless::get_rendering_drivers_func}};

int DisplayServer::server_create_count = 1;

DisplayServerDriver::DisplayServerDriver()
{
	DisplayServer::_bind_driver(this);
}

DisplayServerDriver::~DisplayServerDriver()
{
	DisplayServer::_unbind_driver(this);
}

bool DisplayServerDriver::tts_is_speaking() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

bool DisplayServerDriver::tts_is_paused() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

void DisplayServerDriver::tts_pause() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServerDriver::tts_resume() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServerDriver::tts_speak(const String& p_text, const String& p_voice, int p_volume,
	float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	WARN_PRINT("TTS is not supported by this display server.");
}

PackedStringArray DisplayServerDriver::tts_get_voices_for_language(const String& p_language) const
{
	return PackedStringArray();
}

void DisplayServerDriver::tts_post_utterance_event(
	DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos)
{
}

void DisplayServerDriver::tts_stop() { WARN_PRINT("TTS is not supported by this display server."); }

bool DisplayServerDriver::_get_window_early_clear_override(Color& r_color)
{
	if (DisplayServer::window_early_clear_override_enabled) {
		r_color = DisplayServer::window_early_clear_override_color;
		return true;
	}
	else if (Renderer::data) {
		r_color = Renderer::get_default_clear_color();
		return true;
	}
	else {
		return false;
	}
}

void DisplayServer::set_early_window_clear_color_override(bool p_enabled, Color p_color)
{
	window_early_clear_override_enabled = p_enabled;
	window_early_clear_override_color = p_color;
}

void DisplayServerDriver::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServerDriver::mouse_get_mode() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServerDriver::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServerDriver::mouse_get_mode_override() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServerDriver::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

bool DisplayServerDriver::mouse_is_mode_override_enabled() const { return false; }

void DisplayServerDriver::warp_mouse(const Point2i& p_position) {}

Point2i DisplayServerDriver::mouse_get_position() const
{
	ERR_FAIL_V_MSG(Point2i(), "Mouse is not supported by this display server.");
}

void DisplayServerDriver::clipboard_set(const String& p_text)
{
	WARN_PRINT("Clipboard is not supported by this display server.");
}

String DisplayServerDriver::clipboard_get() const
{
	ERR_FAIL_V_MSG(String(), "Clipboard is not supported by this display server.");
}

Ref<Image> DisplayServerDriver::clipboard_get_image() const
{
	ERR_FAIL_V_MSG(Ref<Image>(), "Clipboard is not supported by this display server.");
}

bool DisplayServerDriver::clipboard_has() const { return !clipboard_get().is_empty(); }

bool DisplayServerDriver::clipboard_has_image() const { return clipboard_get_image().is_valid(); }

void DisplayServerDriver::clipboard_set_primary(const String& p_text)
{
	WARN_PRINT("Primary clipboard is not supported by this display server.");
}

String DisplayServerDriver::clipboard_get_primary() const
{
	ERR_FAIL_V_MSG(String(), "Primary clipboard is not supported by this display server.");
}

void DisplayServerDriver::screen_set_orientation(
	DisplayServerEnums::ScreenOrientation p_orientation, int p_screen)
{
	WARN_PRINT("Orientation not supported by this display server.");
}

DisplayServerEnums::ScreenOrientation DisplayServerDriver::screen_get_orientation(int p_screen) const
{
	return DisplayServerEnums::SCREEN_LANDSCAPE;
}

float DisplayServerDriver::screen_get_scale(int p_screen) const { return 1.0f; }

bool DisplayServerDriver::is_touchscreen_available() const
{
	return Input::is_initialized() && Input::is_emulating_touch_from_mouse();
}

void DisplayServerDriver::screen_set_keep_on(bool p_enable)
{
	WARN_PRINT("Keeping screen on not supported by this display server.");
}

bool DisplayServerDriver::screen_is_kept_on() const { return false; }

int DisplayServerDriver::get_screen_from_rect(const Rect2& p_rect) const
{
	int nearest_area = 0;
	int pos_screen = DisplayServerEnums::INVALID_SCREEN;
	for (int i = 0; i < get_screen_count(); i++) {
		Rect2i r;
		r.position = screen_get_position(i);
		r.size = screen_get_size(i);
		Rect2 inters = r.intersection(p_rect);
		int area = inters.size.width * inters.size.height;
		if (area > nearest_area) {
			pos_screen = i;
			nearest_area = area;
		}
	}
	return pos_screen;
}

DisplayServerEnums::WindowID DisplayServerDriver::create_sub_window(DisplayServerEnums::WindowMode p_mode,
	DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect,
	bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent)
{
	ERR_FAIL_V_MSG(
		DisplayServerEnums::INVALID_WINDOW_ID, "Sub-windows not supported by this display server.");
}

void DisplayServerDriver::show_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServerDriver::delete_sub_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServerDriver::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive)
{
	// Do nothing, if not supported.
}

void DisplayServerDriver::window_set_mouse_passthrough(
	const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_MSG("Mouse passthrough not supported by this display server.");
}

void DisplayServerDriver::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	// noop except in gles
}

void DisplayServerDriver::window_set_ime_active(
	const bool p_active, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

void DisplayServerDriver::window_set_ime_position(
	const Point2i& p_pos, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

#ifndef DISABLE_DEPRECATED

RID DisplayServerDriver::accessibility_create_element(
	DisplayServerEnums::WindowID p_window, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_element(
			p_window, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
	else {
		return RID();
	}
}

RID DisplayServerDriver::accessibility_create_sub_element(
	const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_sub_element(
			p_parent_rid, (AccessibilityServerEnums::AccessibilityRole)p_role, p_insert_pos);
	}
	else {
		return RID();
	}
}

RID DisplayServerDriver::accessibility_create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_sub_text_edit_elements(
			p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, p_is_last_line);
	}
	else {
		return RID();
	}
}

bool DisplayServerDriver::accessibility_has_element(const RID& p_id) const
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::has_element(p_id);
	}
	else {
		return false;
	}
}

void DisplayServerDriver::accessibility_free_element(const RID& p_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::free_element(p_id);
	}
}

void DisplayServerDriver::accessibility_update_set_focus(const RID& p_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_focus(p_id);
	}
}

RID DisplayServerDriver::accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id) const
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::get_window_root(p_window_id);
	}
	else {
		return RID();
	}
}

void DisplayServerDriver::accessibility_set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::set_window_rect(p_window_id, p_rect_out, p_rect_in);
	}
}

void DisplayServerDriver::accessibility_set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::set_window_focused(p_window_id, p_focused);
	}
}

void DisplayServerDriver::accessibility_window_activation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::window_activation_completed(p_window_id);
	}
}

void DisplayServerDriver::accessibility_window_deactivation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::window_deactivation_completed(p_window_id);
	}
}

void DisplayServerDriver::accessibility_update_set_role(
	const RID& p_id, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_role(
			p_id, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
}

void DisplayServerDriver::accessibility_update_set_name(const RID& p_id, const String& p_name)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_name(p_id, p_name);
	}
}

void DisplayServerDriver::accessibility_update_set_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_extra_info(p_id, p_name_extra_info);
	}
}

void DisplayServerDriver::accessibility_update_set_value(const RID& p_id, const String& p_value)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_value(p_id, p_value);
	}
}

void DisplayServerDriver::accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_tooltip(p_id, p_tooltip);
	}
}

void DisplayServerDriver::accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_bounds(p_id, p_rect);
	}
}

void DisplayServerDriver::accessibility_update_set_transform(
	const RID& p_id, const Transform2D& p_transform)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_transform(p_id, p_transform);
	}
}

void DisplayServerDriver::accessibility_update_add_child(const RID& p_id, const RID& p_child_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_child(p_id, p_child_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_controls(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_controls(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_details(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_details(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_described_by(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_flow_to(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_flow_to(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_labeled_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_labeled_by(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_radio_group(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_set_active_descendant(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_active_descendant(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_next_on_line(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_previous_on_line(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_previous_on_line(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_member_of(p_id, p_group_id);
	}
}

void DisplayServerDriver::accessibility_update_set_in_page_link_target(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_in_page_link_target(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_error_message(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_live(
	const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_live(
			p_id, (AccessibilityServerEnums::AccessibilityLiveMode)p_live);
	}
}

void DisplayServerDriver::accessibility_update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_custom_action(
			p_id, p_action_id, p_action_description);
	}
}

void DisplayServerDriver::accessibility_update_set_table_row_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_row_count(p_id, p_count);
	}
}

void DisplayServerDriver::accessibility_update_set_table_column_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_column_count(p_id, p_count);
	}
}

void DisplayServerDriver::accessibility_update_set_table_row_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_row_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_column_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_column_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_cell_position(
			p_id, p_row_index, p_column_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_cell_span(
			p_id, p_row_span, p_column_span);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_count(const RID& p_id, int p_size)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_count(p_id, p_size);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_level(const RID& p_id, int p_level)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_level(p_id, p_level);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_selected(p_id, p_selected);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_expanded(p_id, p_expanded);
	}
}

void DisplayServerDriver::accessibility_update_set_popup_type(
	const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_popup_type(
			p_id, (AccessibilityServerEnums::AccessibilityPopupType)p_popup);
	}
}

void DisplayServerDriver::accessibility_update_set_checked(const RID& p_id, bool p_checekd)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_checked(p_id, p_checekd);
	}
}

void DisplayServerDriver::accessibility_update_set_num_value(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_value(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_num_step(const RID& p_id, double p_step)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_step(p_id, p_step);
	}
}

void DisplayServerDriver::accessibility_update_set_num_jump(const RID& p_id, double p_jump)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_jump(p_id, p_jump);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_x(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_x(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_x_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_x_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_y(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_y(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_y_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_y_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_text_decorations(
	const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_decorations(
			p_id, p_underline, p_strikethrough, p_overline, Color(0, 0, 0, 1));
	}
}

void DisplayServerDriver::accessibility_update_set_text_align(
	const RID& p_id, HorizontalAlignment p_align)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_align(p_id, p_align);
	}
}

void DisplayServerDriver::accessibility_update_set_text_selection(const RID& p_id,
	const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_selection(
			p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
	}
}

void DisplayServerDriver::accessibility_update_set_flag(
	const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_flag(
			p_id, (AccessibilityServerEnums::AccessibilityFlags)p_flag, p_value);
	}
}

void DisplayServerDriver::accessibility_update_set_classname(const RID& p_id, const String& p_classname)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_classname(p_id, p_classname);
	}
}

void DisplayServerDriver::accessibility_update_set_placeholder(
	const RID& p_id, const String& p_placeholder)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_placeholder(p_id, p_placeholder);
	}
}

void DisplayServerDriver::accessibility_update_set_language(const RID& p_id, const String& p_language)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_language(p_id, p_language);
	}
}

void DisplayServerDriver::accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_orientation(p_id, p_vertical);
	}
}

void DisplayServerDriver::accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_orientation(p_id, p_vertical);
	}
}

void DisplayServerDriver::accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_shortcut(p_id, p_shortcut);
	}
}

void DisplayServerDriver::accessibility_update_set_url(const RID& p_id, const String& p_url)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_url(p_id, p_url);
	}
}

void DisplayServerDriver::accessibility_update_set_role_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_role_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_state_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_state_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_color_value(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_color_value(p_id, p_color);
	}
}

void DisplayServerDriver::accessibility_update_set_background_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_background_color(p_id, p_color);
	}
}

void DisplayServerDriver::accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_foreground_color(p_id, p_color);
	}
}

#endif // DISABLE_DEPRECATED

Point2i DisplayServerDriver::ime_get_selection() const
{
	ERR_FAIL_V_MSG(
		Point2i(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

String DisplayServerDriver::ime_get_text() const
{
	ERR_FAIL_V_MSG(
		String(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

void DisplayServerDriver::virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect,
	DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start,
	int p_cursor_end)
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

void DisplayServerDriver::virtual_keyboard_hide()
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

// returns height of the currently shown keyboard (0 if keyboard is hidden)
int DisplayServerDriver::virtual_keyboard_get_height() const
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
	return 0;
}

bool DisplayServerDriver::has_hardware_keyboard() const { return true; }

void DisplayServerDriver::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	WARN_PRINT("Cursor shape not supported by this display server.");
}

DisplayServerEnums::CursorShape DisplayServerDriver::cursor_get_shape() const
{
	return DisplayServerEnums::CURSOR_ARROW;
}

void DisplayServerDriver::cursor_set_custom_image(const Ref<Resource>& p_cursor,
	DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	WARN_PRINT("Custom cursor shape not supported by this display server.");
}

bool DisplayServerDriver::get_swap_cancel_ok() { return false; }

void DisplayServerDriver::enable_for_stealing_focus(ProcessID pid) {}

Error DisplayServerDriver::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid,
	const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServerDriver::request_close_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServerDriver::remove_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

ProcessID DisplayServerDriver::get_focused_process_id()
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return 0;
}

void DisplayServerDriver::beep() const {}

int DisplayServerDriver::keyboard_get_layout_count() const { return 0; }

int DisplayServerDriver::keyboard_get_current_layout() const { return -1; }

void DisplayServerDriver::keyboard_set_current_layout(int p_index) {}

String DisplayServerDriver::keyboard_get_layout_language(int p_index) const { return ""; }

String DisplayServerDriver::keyboard_get_layout_name(int p_index) const { return "Not supported"; }

Key DisplayServerDriver::keyboard_get_keycode_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

Key DisplayServerDriver::keyboard_get_label_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

void DisplayServerDriver::show_emoji_and_symbol_picker() const {}

void DisplayServerDriver::force_process_and_drop_events() {}

void DisplayServerDriver::release_rendering_thread()
{
	WARN_PRINT("Rendering thread not supported by this display server.");
}

void DisplayServerDriver::swap_buffers()
{
	WARN_PRINT("Swap buffers not supported by this display server.");
}

void DisplayServerDriver::set_native_icon(const String& p_filename)
{
	WARN_PRINT("Native icon not supported by this display server.");
}

void DisplayServerDriver::set_icon(const Ref<Image>& p_icon)
{
	WARN_PRINT("Icon not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_icon(
	DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_tooltip(
	DisplayServerEnums::IndicatorID p_id, const String& p_tooltip)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_menu(
	DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

Rect2 DisplayServerDriver::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const
{
	WARN_PRINT("Status indicator not supported by this display server.");
	return Rect2();
}

void DisplayServerDriver::delete_status_indicator(DisplayServerEnums::IndicatorID p_id)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

int64_t DisplayServerDriver::window_get_native_handle(
	DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Native handle not supported by this display server.");
	return 0;
}

void DisplayServerDriver::window_set_vsync_mode(
	DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
}

DisplayServerEnums::VSyncMode DisplayServerDriver::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
	return DisplayServerEnums::VSyncMode::VSYNC_ENABLED;
}

bool DisplayServerDriver::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServerDriver::window_request_hdr_output(
	const bool p_enable, DisplayServerEnums::WindowID p_window)
{
	if (p_enable) {
		WARN_PRINT("HDR output requested, but it is not supported by this display server.");
	}
}

bool DisplayServerDriver::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

bool DisplayServerDriver::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServerDriver::window_set_hdr_output_reference_luminance(
	const float p_reference_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Attempting to set reference luminance, but HDR output is not supported by this "
			   "display server.");
}

float DisplayServerDriver::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServerDriver::window_get_hdr_output_current_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

void DisplayServerDriver::window_set_hdr_output_max_luminance(
	const float p_max_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT(
		"Attempting to set max luminance, but HDR output is not supported by this display server.");
}

float DisplayServerDriver::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServerDriver::window_get_hdr_output_current_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

float DisplayServerDriver::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const
{
	return 1.0f;
}

DisplayServerEnums::WindowID DisplayServerDriver::get_focused_window() const
{
	return DisplayServerEnums::MAIN_WINDOW_ID; // Proper value for single windows.
}

void DisplayServerDriver::set_context(DisplayServerEnums::Context p_context) {}

Ref<Image> DisplayServerDriver::_get_cursor_image_from_resource(
	const Ref<Resource>& p_cursor, const Vector2& p_hotspot)
{
	Ref<Image> image;
	ERR_FAIL_COND_V_MSG(p_hotspot.x < 0 || p_hotspot.y < 0, image, "Hotspot outside cursor image.");

	Ref<Texture2D> texture = p_cursor;
	if (texture.is_valid()) {
		image = texture->get_image();
	}
	else {
		image = p_cursor;
	}
	ERR_FAIL_COND_V(image.is_null(), image);

	Size2 image_size = image->get_size();
	ERR_FAIL_COND_V_MSG(p_hotspot.x > image_size.width || p_hotspot.y > image_size.height, image,
		"Hotspot outside cursor image.");
	ERR_FAIL_COND_V_MSG(image_size.width > 256 || image_size.height > 256, image,
		"Cursor image too big. Max supported size is 256x256.");
	return image;
}


uint32_t DisplayServerDriver::mouse_get_button_state() const { return 0; }

void DisplayServer::_bind_driver(DisplayServerDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
	Input::set_mouse_mode_func = _input_set_mouse_mode;
	Input::get_mouse_mode_func = _input_get_mouse_mode;
	Input::set_mouse_mode_override_func = _input_set_mouse_mode_override;
	Input::get_mouse_mode_override_func = _input_get_mouse_mode_override;
	Input::set_mouse_mode_override_enabled_func = _input_set_mouse_mode_override_enabled;
	Input::is_mouse_mode_override_enabled_func = _input_is_mouse_mode_override_enabled;
	Input::warp_mouse_func = _input_warp;
	Input::get_current_cursor_shape_func = _input_get_current_cursor_shape;
	Input::set_custom_mouse_cursor_func = _input_set_custom_mouse_cursor_func;
}

void DisplayServer::_unbind_driver(DisplayServerDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

void DisplayServer::register_create_function(
	const char* p_name, CreateFunction p_function, GetRenderingDriversFunction p_get_drivers)
{
	ERR_FAIL_COND(server_create_count == MAX_SERVERS);
	// Headless display server is always last
	server_create_functions[server_create_count] = server_create_functions[server_create_count - 1];
	server_create_functions[server_create_count - 1].name = p_name;
	server_create_functions[server_create_count - 1].create_function = p_function;
	server_create_functions[server_create_count - 1].get_rendering_drivers_function = p_get_drivers;
	server_create_count++;
}

int DisplayServer::get_create_function_count() { return server_create_count; }

const char* DisplayServer::get_create_function_name(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].name;
}

Vector<String> DisplayServer::get_create_function_rendering_drivers(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, Vector<String>());
	return server_create_functions[p_index].get_rendering_drivers_function();
}

DisplayServerDriver* DisplayServer::create(int p_index, const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	if (data && data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	DisplayServerDriver* driver = server_create_functions[p_index].create_function(
		p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen,
		p_context, p_parent_window, r_error);
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

void DisplayServer::finalize()
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

void DisplayServer::_input_set_mouse_mode(InputClassEnums::MouseMode p_mode)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode(DisplayServerEnums::MouseMode(p_mode));
	}
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode()
{
	if (data && data->driver) {
		return InputClassEnums::MouseMode(data->driver->mouse_get_mode());
	}
	return InputClassEnums::MouseMode(DisplayServerEnums::MOUSE_MODE_VISIBLE);
}

void DisplayServer::_input_set_mouse_mode_override(InputClassEnums::MouseMode p_mode)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode_override(DisplayServerEnums::MouseMode(p_mode));
	}
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode_override()
{
	if (data && data->driver) {
		return InputClassEnums::MouseMode(data->driver->mouse_get_mode_override());
	}
	return InputClassEnums::MouseMode(DisplayServerEnums::MOUSE_MODE_VISIBLE);
}

void DisplayServer::_input_set_mouse_mode_override_enabled(bool p_enabled)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode_override_enabled(p_enabled);
	}
}

bool DisplayServer::_input_is_mouse_mode_override_enabled()
{
	return (data && data->driver) ? data->driver->mouse_is_mode_override_enabled() : false;
}

void DisplayServer::_input_warp(const Vector2& p_to_pos)
{
	if (data && data->driver) {
		data->driver->warp_mouse(p_to_pos);
	}
}

InputClassEnums::CursorShape DisplayServer::_input_get_current_cursor_shape()
{
	if (data && data->driver) {
		return (InputClassEnums::CursorShape)data->driver->cursor_get_shape();
	}
	return (InputClassEnums::CursorShape)DisplayServerEnums::CURSOR_ARROW;
}

void DisplayServer::_input_set_custom_mouse_cursor_func(
	const Ref<Resource>& p_image, InputClassEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	if (data && data->driver) {
		data->driver->cursor_set_custom_image(
			p_image, (DisplayServerEnums::CursorShape)p_shape, p_hotspot);
	}
}

bool DisplayServer::is_rendering_device_supported()
{
#if defined(RD_ENABLED)
	if (RD::data) {
		return true;
	}

	if (supported_rendering_device == DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS) {
		return true;
	}
	else if (supported_rendering_device ==
			   DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE) {
		return false;
	}

	Error err;

#if defined(WINDOWS_ENABLED) || defined(LINUXBSD_ENABLED)
	// On some drivers combining OpenGL and RenderingDevice can result in crash, offload the check
	// to the subprocess.
	List<String> arguments;
	arguments.push_back("--test-rd-support");
	if (is_initialized()) {
		arguments.push_back("--display-driver");
		arguments.push_back(get_name().to_lower());
	}

	String pipe;
	int exitcode = 0;
	err = OS::execute(
		OS::get_executable_path(), arguments, &pipe, &exitcode);
	if (err == OK && exitcode == 0) {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
		return true;
	}
	else {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
	}
#else // WINDOWS_ENABLED

	RenderingContextDriver* rcd = nullptr;

#if defined(VULKAN_ENABLED)
	rcd = memnew(RenderingContextDriverVulkan);
#endif
#ifdef D3D12_ENABLED
	if (rcd == nullptr) {
		rcd = memnew(RenderingContextDriverD3D12);
	}
#endif
#ifdef METAL_ENABLED
	if (rcd == nullptr) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		// Eliminate "RenderingContextDriverMetal is only available on iOS 14.0 or newer".
		rcd = memnew(RenderingContextDriverMetal);
		GODOT_CLANG_WARNING_POP
	}
#endif

	if (rcd != nullptr) {
		err = rcd->initialize();
		if (err == OK) {
			err = RenderingDevice::initialize(rcd, rcd->device_get(0));
			RenderingDevice::finalize();
			if (err == OK) {
				// Creating a RenderingDevice is quite slow.
				// Cache the result for future usage, so that it's much faster on subsequent calls.
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
				memdelete(rcd);
				rcd = nullptr;
				return true;
			}
			else {
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
			}
		}

		memdelete(rcd);
		rcd = nullptr;
	}

#endif // WINDOWS_ENABLED
#endif // RD_ENABLED
	return false;
}

bool DisplayServer::can_create_rendering_device() { return true; }

String DisplayServer::get_name()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->get_name();
}

void DisplayServer::set_context(DisplayServerEnums::Context p_context)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_context(p_context);
}

void DisplayServer::set_native_icon(const String& p_filename)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_native_icon(p_filename);
}

void DisplayServer::set_icon(const Ref<Image>& p_icon)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_icon(p_icon);
}

bool DisplayServer::has_feature(DisplayServerEnums::Feature p_feature)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->has_feature(p_feature);
}

void DisplayServer::process_events()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->process_events();
}

void DisplayServer::force_process_and_drop_events()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->force_process_and_drop_events();
}

void DisplayServer::release_rendering_thread()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->release_rendering_thread();
}

void DisplayServer::swap_buffers()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->swap_buffers();
}

void DisplayServer::beep()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->beep();
}

bool DisplayServer::tts_is_speaking()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->tts_is_speaking();
}

bool DisplayServer::tts_is_paused()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->tts_is_paused();
}

PackedStringArray DisplayServer::tts_get_voices_for_language(const String& p_language)
{
	ERR_FAIL_COND_V(!is_initialized(), PackedStringArray());
	return data->driver->tts_get_voices_for_language(p_language);
}

void DisplayServer::tts_speak(const String& p_text, const String& p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServer::tts_pause()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_pause();
}

void DisplayServer::tts_resume()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_resume();
}

void DisplayServer::tts_stop()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_stop();
}

void DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_post_utterance_event(p_event, p_id, p_pos);
}

bool DisplayServer::is_dark_mode_supported()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_dark_mode_supported();
}

bool DisplayServer::is_dark_mode()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_dark_mode();
}

Color DisplayServer::get_accent_color()
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->get_accent_color();
}

Color DisplayServer::get_base_color()
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->get_base_color();
}

void DisplayServer::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode(p_mode);
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::MOUSE_MODE_VISIBLE);
	return data->driver->mouse_get_mode();
}

void DisplayServer::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode_override(p_mode);
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode_override()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::MOUSE_MODE_VISIBLE);
	return data->driver->mouse_get_mode_override();
}

void DisplayServer::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode_override_enabled(p_override_enabled);
}

bool DisplayServer::mouse_is_mode_override_enabled()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->mouse_is_mode_override_enabled();
}

void DisplayServer::warp_mouse(const Point2i& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->warp_mouse(p_position);
}

Point2i DisplayServer::mouse_get_position()
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->mouse_get_position();
}

uint32_t DisplayServer::mouse_get_button_state()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->mouse_get_button_state();
}

void DisplayServer::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->cursor_set_shape(p_shape);
}

DisplayServerEnums::CursorShape DisplayServer::cursor_get_shape()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::CURSOR_ARROW);
	return data->driver->cursor_get_shape();
}

void DisplayServer::cursor_set_custom_image(const Ref<Resource>& p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->cursor_set_custom_image(p_cursor, p_shape, p_hotspot);
}

Point2i DisplayServer::ime_get_selection()
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->ime_get_selection();
}

String DisplayServer::ime_get_text()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->ime_get_text();
}

void DisplayServer::virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->virtual_keyboard_show(p_existing_text, p_screen_rect, p_type, p_max_length, p_cursor_start, p_cursor_end);
}

void DisplayServer::virtual_keyboard_hide()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->virtual_keyboard_hide();
}

int DisplayServer::virtual_keyboard_get_height()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->virtual_keyboard_get_height();
}

bool DisplayServer::has_hardware_keyboard()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->has_hardware_keyboard();
}

int DisplayServer::keyboard_get_layout_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->keyboard_get_layout_count();
}

int DisplayServer::keyboard_get_current_layout()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->keyboard_get_current_layout();
}

void DisplayServer::keyboard_set_current_layout(int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->keyboard_set_current_layout(p_index);
}

String DisplayServer::keyboard_get_layout_language(int p_index)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->keyboard_get_layout_language(p_index);
}

String DisplayServer::keyboard_get_layout_name(int p_index)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->keyboard_get_layout_name(p_index);
}

Key DisplayServer::keyboard_get_keycode_from_physical(Key p_keycode)
{
	ERR_FAIL_COND_V(!is_initialized(), p_keycode);
	return data->driver->keyboard_get_keycode_from_physical(p_keycode);
}

Key DisplayServer::keyboard_get_label_from_physical(Key p_keycode)
{
	ERR_FAIL_COND_V(!is_initialized(), p_keycode);
	return data->driver->keyboard_get_label_from_physical(p_keycode);
}

int DisplayServer::tablet_get_driver_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->tablet_get_driver_count();
}

String DisplayServer::tablet_get_driver_name(int p_driver)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->tablet_get_driver_name(p_driver);
}

String DisplayServer::tablet_get_current_driver()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->tablet_get_current_driver();
}

void DisplayServer::tablet_set_current_driver(const String& p_driver)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tablet_set_current_driver(p_driver);
}

void DisplayServer::clipboard_set(const String& p_text)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->clipboard_set(p_text);
}

String DisplayServer::clipboard_get()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->clipboard_get();
}

Ref<Image> DisplayServer::clipboard_get_image()
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->clipboard_get_image();
}

bool DisplayServer::clipboard_has()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->clipboard_has();
}

bool DisplayServer::clipboard_has_image()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->clipboard_has_image();
}

void DisplayServer::clipboard_set_primary(const String& p_text)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->clipboard_set_primary(p_text);
}

String DisplayServer::clipboard_get_primary()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->clipboard_get_primary();
}

Rect2i DisplayServer::get_display_safe_area(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->get_display_safe_area(p_screen);
}

int DisplayServer::get_screen_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_screen_count();
}

int DisplayServer::get_primary_screen()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_primary_screen();
}

int DisplayServer::get_keyboard_focus_screen()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_keyboard_focus_screen();
}

int DisplayServer::get_screen_from_rect(const Rect2& p_rect)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_screen_from_rect(p_rect);
}

Point2i DisplayServer::screen_get_position(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->screen_get_position(p_screen);
}

Size2i DisplayServer::screen_get_size(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->screen_get_size(p_screen);
}

Rect2i DisplayServer::screen_get_usable_rect(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->screen_get_usable_rect(p_screen);
}

int DisplayServer::screen_get_dpi(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->screen_get_dpi(p_screen);
}

float DisplayServer::screen_get_scale(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->screen_get_scale(p_screen);
}

float DisplayServer::screen_get_max_scale()
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->screen_get_max_scale();
}

float DisplayServer::screen_get_refresh_rate(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), SCREEN_REFRESH_RATE_FALLBACK);
	return data->driver->screen_get_refresh_rate(p_screen);
}

Color DisplayServer::screen_get_pixel(const Point2i& p_position)
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->screen_get_pixel(p_position);
}

Ref<Image> DisplayServer::screen_get_image(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->screen_get_image(p_screen);
}

Ref<Image> DisplayServer::screen_get_image_rect(const Rect2i& p_rect)
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->screen_get_image_rect(p_rect);
}

bool DisplayServer::is_touchscreen_available()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_touchscreen_available();
}

void DisplayServer::screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->screen_set_orientation(p_orientation, p_screen);
}

DisplayServerEnums::ScreenOrientation DisplayServer::screen_get_orientation(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::SCREEN_LANDSCAPE);
	return data->driver->screen_get_orientation(p_screen);
}

void DisplayServer::screen_set_keep_on(bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->screen_set_keep_on(p_enable);
}

bool DisplayServer::screen_is_kept_on()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->screen_is_kept_on();
}

Vector<DisplayServerEnums::WindowID> DisplayServer::get_window_list()
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<DisplayServerEnums::WindowID>());
	return data->driver->get_window_list();
}

DisplayServerEnums::WindowID DisplayServer::create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->create_sub_window(p_mode, p_vsync_mode, p_flags, p_rect, p_exclusive, p_transient_parent);
}

void DisplayServer::show_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->show_window(p_id);
}

void DisplayServer::delete_sub_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->delete_sub_window(p_id);
}

DisplayServerEnums::WindowID DisplayServer::window_get_active_popup()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->window_get_active_popup();
}

void DisplayServer::window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i& p_rect)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_popup_safe_rect(p_window, p_rect);
}

Rect2i DisplayServer::window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->window_get_popup_safe_rect(p_window);
}

int64_t DisplayServer::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->window_get_native_handle(p_handle_type, p_window);
}

DisplayServerEnums::WindowID DisplayServer::get_window_at_screen_position(const Point2i& p_position)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->get_window_at_screen_position(p_position);
}

void DisplayServer::window_set_title(const String& p_title, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_title(p_title, p_window);
}

Size2i DisplayServer::window_get_title_size(const String& p_title, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_title_size(p_title, p_window);
}

void DisplayServer::window_set_mouse_passthrough(const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_mouse_passthrough(p_region, p_window);
}

int DisplayServer::window_get_current_screen(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->window_get_current_screen(p_window);
}

void DisplayServer::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_current_screen(p_screen, p_window);
}

Point2i DisplayServer::window_get_position(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->window_get_position(p_window);
}

Point2i DisplayServer::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->window_get_position_with_decorations(p_window);
}

void DisplayServer::window_set_position(const Point2i& p_position, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_position(p_position, p_window);
}

void DisplayServer::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_transient(p_window, p_parent);
}

void DisplayServer::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_exclusive(p_window, p_exclusive);
}

void DisplayServer::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_max_size(p_size, p_window);
}

Size2i DisplayServer::window_get_max_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_max_size(p_window);
}

void DisplayServer::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_min_size(p_size, p_window);
}

Size2i DisplayServer::window_get_min_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_min_size(p_window);
}

void DisplayServer::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_size(p_size, p_window);
}

Size2i DisplayServer::window_get_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_size(p_window);
}

Size2i DisplayServer::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_size_with_decorations(p_window);
}

float DisplayServer::window_get_scale(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->window_get_scale(p_window);
}

void DisplayServer::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_mode(p_mode, p_window);
}

DisplayServerEnums::WindowMode DisplayServer::window_get_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	return data->driver->window_get_mode(p_window);
}

void DisplayServer::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_vsync_mode(p_vsync_mode, p_window);
}

DisplayServerEnums::VSyncMode DisplayServer::window_get_vsync_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::VSyncMode::VSYNC_ENABLED);
	return data->driver->window_get_vsync_mode(p_window);
}

bool DisplayServer::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_supported(p_window);
}

void DisplayServer::window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_request_hdr_output(p_enable, p_window);
}

bool DisplayServer::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_requested(p_window);
}

bool DisplayServer::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_enabled(p_window);
}

void DisplayServer::window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_hdr_output_reference_luminance(p_reference_luminance, p_window);
}

float DisplayServer::window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), -1.0f);
	return data->driver->window_get_hdr_output_reference_luminance(p_window);
}

float DisplayServer::window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0.0f);
	return data->driver->window_get_hdr_output_current_reference_luminance(p_window);
}

void DisplayServer::window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_hdr_output_max_luminance(p_max_luminance, p_window);
}

float DisplayServer::window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), -1.0f);
	return data->driver->window_get_hdr_output_max_luminance(p_window);
}

float DisplayServer::window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0.0f);
	return data->driver->window_get_hdr_output_current_max_luminance(p_window);
}

void DisplayServer::window_set_icon(const Ref<Image>& p_icon, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_icon(p_icon, p_window);
}

float DisplayServer::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->window_get_output_max_linear_value(p_window);
}

bool DisplayServer::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_maximize_allowed(p_window);
}

void DisplayServer::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_flag(p_flag, p_enabled, p_window);
}

bool DisplayServer::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_get_flag(p_flag, p_window);
}

void DisplayServer::window_request_attention(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_request_attention(p_window);
}

void DisplayServer::window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_taskbar_progress_value(p_value, p_window);
}

void DisplayServer::window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_taskbar_progress_state(p_state, p_window);
}

void DisplayServer::window_move_to_foreground(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_move_to_foreground(p_window);
}

bool DisplayServer::window_is_focused(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_focused(p_window);
}

DisplayServerEnums::WindowID DisplayServer::get_focused_window()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->get_focused_window();
}

void DisplayServer::window_set_window_buttons_offset(const Vector2i& p_offset, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_window_buttons_offset(p_offset, p_window);
}

Vector3i DisplayServer::window_get_safe_title_margins(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3i());
	return data->driver->window_get_safe_title_margins(p_window);
}

bool DisplayServer::window_can_draw(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_can_draw(p_window);
}

bool DisplayServer::can_any_window_draw()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->can_any_window_draw();
}

void DisplayServer::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_ime_active(p_active, p_window);
}

void DisplayServer::window_set_ime_position(const Point2i& p_pos, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_ime_position(p_pos, p_window);
}

bool DisplayServer::window_maximize_on_title_dbl_click()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_maximize_on_title_dbl_click();
}

bool DisplayServer::window_minimize_on_title_dbl_click()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_minimize_on_title_dbl_click();
}

void DisplayServer::window_start_drag(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_start_drag(p_window);
}

void DisplayServer::window_set_color(const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_color(p_color);
}

void DisplayServer::window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_start_resize(p_edge, p_window);
}

void DisplayServer::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->gl_window_make_current(p_window_id);
}

bool DisplayServer::is_window_transparency_available()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_window_transparency_available();
}

void DisplayServer::enable_for_stealing_focus(ProcessID pid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->enable_for_stealing_focus(pid);
}

Error DisplayServer::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid, const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->embed_process(p_window, p_pid, p_rect, p_visible, p_grab_focus);
}

Error DisplayServer::request_close_embedded_process(ProcessID p_pid)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->request_close_embedded_process(p_pid);
}

Error DisplayServer::remove_embedded_process(ProcessID p_pid)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->remove_embedded_process(p_pid);
}

ProcessID DisplayServer::get_focused_process_id()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_focused_process_id();
}

bool DisplayServer::get_swap_cancel_ok()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->get_swap_cancel_ok();
}

void DisplayServer::show_emoji_and_symbol_picker()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->show_emoji_and_symbol_picker();
}

void DisplayServer::status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_icon(p_id, p_icon);
}

void DisplayServer::status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String& p_tooltip)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_tooltip(p_id, p_tooltip);
}

void DisplayServer::status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_menu(p_id, p_menu_rid);
}

Rect2 DisplayServer::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2());
	return data->driver->status_indicator_get_rect(p_id);
}

void DisplayServer::delete_status_indicator(DisplayServerEnums::IndicatorID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->delete_status_indicator(p_id);
}

bool DisplayServer::is_in_pip_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_in_pip_mode(p_window);
}

void DisplayServer::pip_mode_enter(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_enter(p_window);
}

void DisplayServer::pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_set_aspect_ratio(p_numerator, p_denominator, p_window);
}

void DisplayServer::pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_set_auto_enter_on_background(p_auto_enter_on_background, p_window);
}

int DisplayServer::accessibility_should_increase_contrast()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_increase_contrast();
}

int DisplayServer::accessibility_should_reduce_animation()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_reduce_animation();
}

int DisplayServer::accessibility_should_reduce_transparency()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_reduce_transparency();
}

int DisplayServer::accessibility_screen_reader_active()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_screen_reader_active();
}

#ifndef DISABLE_DEPRECATED
RID DisplayServer::accessibility_create_element(DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::AccessibilityRole p_role)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_element(p_window_id, p_role);
}

RID DisplayServer::accessibility_create_sub_element(const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_sub_element(p_parent_rid, p_role, p_insert_pos);
}

RID DisplayServer::accessibility_create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_sub_text_edit_elements(p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, p_is_last_line);
}

bool DisplayServer::accessibility_has_element(const RID& p_id)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->accessibility_has_element(p_id);
}

void DisplayServer::accessibility_free_element(const RID& p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_free_element(p_id);
}

void DisplayServer::accessibility_update_set_focus(const RID& p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_focus(p_id);
}

RID DisplayServer::accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_get_window_root(p_window_id);
}

void DisplayServer::accessibility_set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_set_window_rect(p_window_id, p_rect_out, p_rect_in);
}

void DisplayServer::accessibility_set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_set_window_focused(p_window_id, p_focused);
}

void DisplayServer::accessibility_window_activation_completed(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_window_activation_completed(p_window_id);
}

void DisplayServer::accessibility_window_deactivation_completed(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_window_deactivation_completed(p_window_id);
}

void DisplayServer::accessibility_update_set_role(const RID& p_id, DisplayServerEnums::AccessibilityRole p_role)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_role(p_id, p_role);
}

void DisplayServer::accessibility_update_set_name(const RID& p_id, const String& p_name)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_name(p_id, p_name);
}

void DisplayServer::accessibility_update_set_extra_info(const RID& p_id, const String& p_name_extra_info)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_extra_info(p_id, p_name_extra_info);
}

void DisplayServer::accessibility_update_set_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_value(const RID& p_id, const String& p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_value(p_id, p_value);
}

void DisplayServer::accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_tooltip(p_id, p_tooltip);
}

void DisplayServer::accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_bounds(p_id, p_rect);
}

void DisplayServer::accessibility_update_set_transform(const RID& p_id, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_transform(p_id, p_transform);
}

void DisplayServer::accessibility_update_add_child(const RID& p_id, const RID& p_child_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_child(p_id, p_child_id);
}

void DisplayServer::accessibility_update_add_related_controls(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_controls(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_details(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_details(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_described_by(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_described_by(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_flow_to(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_flow_to(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_labeled_by(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_labeled_by(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_radio_group(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_radio_group(p_id, p_related_id);
}

void DisplayServer::accessibility_update_set_active_descendant(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_active_descendant(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_next_on_line(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_previous_on_line(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_previous_on_line(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_member_of(p_id, p_group_id);
}

void DisplayServer::accessibility_update_set_in_page_link_target(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_in_page_link_target(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_error_message(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_live(const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_live(p_id, p_live);
}

void DisplayServer::accessibility_update_add_custom_action(const RID& p_id, int p_action_id, const String& p_action_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_custom_action(p_id, p_action_id, p_action_description);
}

void DisplayServer::accessibility_update_set_table_row_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_row_count(p_id, p_count);
}

void DisplayServer::accessibility_update_set_table_column_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_column_count(p_id, p_count);
}

void DisplayServer::accessibility_update_set_table_row_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_row_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_table_column_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_column_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_table_cell_position(const RID& p_id, int p_row_index, int p_column_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_cell_position(p_id, p_row_index, p_column_index);
}

void DisplayServer::accessibility_update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_cell_span(p_id, p_row_span, p_column_span);
}

void DisplayServer::accessibility_update_set_list_item_count(const RID& p_id, int p_size)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_count(p_id, p_size);
}

void DisplayServer::accessibility_update_set_list_item_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_list_item_level(const RID& p_id, int p_level)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_level(p_id, p_level);
}

void DisplayServer::accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_selected(p_id, p_selected);
}

void DisplayServer::accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_expanded(p_id, p_expanded);
}

void DisplayServer::accessibility_update_set_popup_type(const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_popup_type(p_id, p_popup);
}

void DisplayServer::accessibility_update_set_checked(const RID& p_id, bool p_checekd)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_checked(p_id, p_checekd);
}

void DisplayServer::accessibility_update_set_num_value(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_value(p_id, p_position);
}

void DisplayServer::accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_num_step(const RID& p_id, double p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_step(p_id, p_step);
}

void DisplayServer::accessibility_update_set_num_jump(const RID& p_id, double p_jump)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_jump(p_id, p_jump);
}

void DisplayServer::accessibility_update_set_scroll_x(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_x(p_id, p_position);
}

void DisplayServer::accessibility_update_set_scroll_x_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_x_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_scroll_y(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_y(p_id, p_position);
}

void DisplayServer::accessibility_update_set_scroll_y_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_y_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_text_decorations(const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_decorations(p_id, p_underline, p_strikethrough, p_overline);
}

void DisplayServer::accessibility_update_set_text_align(const RID& p_id, HorizontalAlignment p_align)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_align(p_id, p_align);
}

void DisplayServer::accessibility_update_set_text_selection(const RID& p_id, const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_selection(p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
}

void DisplayServer::accessibility_update_set_flag(const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_flag(p_id, p_flag, p_value);
}

void DisplayServer::accessibility_update_set_classname(const RID& p_id, const String& p_classname)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_classname(p_id, p_classname);
}

void DisplayServer::accessibility_update_set_placeholder(const RID& p_id, const String& p_placeholder)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_placeholder(p_id, p_placeholder);
}

void DisplayServer::accessibility_update_set_language(const RID& p_id, const String& p_language)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_language(p_id, p_language);
}

void DisplayServer::accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_orientation(p_id, p_vertical);
}

void DisplayServer::accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_orientation(p_id, p_vertical);
}

void DisplayServer::accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_shortcut(p_id, p_shortcut);
}

void DisplayServer::accessibility_update_set_url(const RID& p_id, const String& p_url)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_url(p_id, p_url);
}

void DisplayServer::accessibility_update_set_role_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_role_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_state_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_state_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_color_value(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_color_value(p_id, p_color);
}

void DisplayServer::accessibility_update_set_background_color(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_background_color(p_id, p_color);
}

void DisplayServer::accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_foreground_color(p_id, p_color);
}
#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from servers/display/accessibility_server.cpp
// ============================================================================

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

// ============================================================================
// Merged from servers/display/display_server_headless.cpp
// ============================================================================

#include "display_server_headless.h"

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"

DisplayServerDriver* DisplayServerHeadless::create_func(const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	r_error = OK;
	RasterizerDummy::make_current();
	return memnew(DisplayServerHeadless());
}

void DisplayServerHeadless::_dispatch_input_events(const InputEvent& p_event)
{
	if (DisplayServerHeadless* ds = DisplayServer::driver_as<DisplayServerHeadless>()) {
		ds->_dispatch_input_event(p_event);
	}
}

void DisplayServerHeadless::process_events() { Input::flush_buffered_events(); }

DisplayServerHeadless::DisplayServerHeadless()
{
	NativeMenu::initialize();
	Input::set_event_dispatch_function(_dispatch_input_events);
}

DisplayServerHeadless::~DisplayServerHeadless()
{
	NativeMenu::finalize();
}

void DisplayServerHeadless::_dispatch_input_event(const InputEvent&) {}

// ============================================================================
// Merged from servers/display/native_menu.cpp
// ============================================================================

#include "native_menu.h"
#include "scene/resources/texture.h"

NativeMenuDriver::NativeMenuDriver() { NativeMenu::_bind_driver(this); }

NativeMenuDriver::~NativeMenuDriver() { NativeMenu::_unbind_driver(this); }

bool NativeMenuDriver::has_feature(Feature p_feature) const
{
	(void)p_feature;
	return false;
}

bool NativeMenuDriver::has_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	return false;
}

RID NativeMenuDriver::get_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

String NativeMenuDriver::get_system_menu_name(SystemMenus p_menu_id) const
{
	switch (p_menu_id) {
	case MAIN_MENU_ID:
		return "Main menu";
	case APPLICATION_MENU_ID:
		return "Application menu";
	case WINDOW_MENU_ID:
		return "Window menu";
	case HELP_MENU_ID:
		return "Help menu";
	case DOCK_MENU_ID:
		return "Dock menu";
	default:
		return "Invalid";
	}
}

String NativeMenuDriver::get_system_menu_text(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

void NativeMenuDriver::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	(void)p_menu_id;
	(void)p_name;
	WARN_PRINT("Global menus are not supported on this platform.");
}

RID NativeMenuDriver::create_menu()
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

bool NativeMenuDriver::has_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::free_menu(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

Size2 NativeMenuDriver::get_size(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Size2();
}

void NativeMenuDriver::popup(const RID& p_rid, const Vector2i& p_position)
{
	(void)p_rid;
	(void)p_position;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	(void)p_rid;
	(void)p_is_rtl;
	WARN_PRINT("Global menus are not supported on this platform.");
}

bool NativeMenuDriver::is_opened(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::set_minimum_width(const RID& p_rid, float p_width)
{
	(void)p_rid;
	(void)p_width;
	WARN_PRINT("Global menus are not supported on this platform.");
}

float NativeMenuDriver::get_minimum_width(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0.f;
}

int NativeMenuDriver::add_separator(const RID& p_rid, int p_index)
{
	(void)p_rid;
	(void)p_index;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_text(const RID& p_rid, const String& p_text) const
{
	(void)p_rid;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_submenu(
	const RID& p_rid, const RID& p_submenu_rid) const
{
	if (!has_menu(p_rid) || !has_menu(p_submenu_rid)) {
		return -1;
	}
	int count = get_item_count(p_rid);
	for (int i = 0; i < count; i++) {
		if (p_submenu_rid == get_item_submenu(p_rid, i)) {
			return i;
		}
	}
	return -1;
}

bool NativeMenuDriver::is_item_checked(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_indeterminate(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_radio_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_text(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

RID NativeMenuDriver::get_item_submenu(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

Key NativeMenuDriver::get_item_accelerator(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Key::NONE;
}

bool NativeMenuDriver::is_item_disabled(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_hidden(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_tooltip(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

int NativeMenuDriver::get_item_state(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_max_states(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

Ref<Texture2D> NativeMenuDriver::get_item_icon(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Ref<Texture2D>();
}

int NativeMenuDriver::get_item_indentation_level(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

void NativeMenuDriver::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checked;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_indeterminate;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_submenu_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_keycode;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_disabled;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_hidden;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_tooltip;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_state;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_max_states;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_icon;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_level;
	WARN_PRINT("Global menus are not supported on this platform.");
}

int NativeMenuDriver::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_target_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_count(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

bool NativeMenuDriver::is_system_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::remove_item(const RID& p_rid, int p_idx)
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::clear(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

/* NativeMenu Static Dispatch */

void NativeMenu::_bind_driver(NativeMenuDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void NativeMenu::_unbind_driver(NativeMenuDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

void NativeMenu::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->driver) {
		memnew(NativeMenuDriver);
	}
}

void NativeMenu::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		NativeMenuDriver* drv = data->driver;
		data->driver = nullptr;
		memdelete(drv);
	}
	memdelete(data);
	data = nullptr;
}

bool NativeMenu::has_feature(Feature p_feature)
{
	return is_initialized() ? data->driver->has_feature(p_feature) : false;
}

bool NativeMenu::has_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->has_system_menu(p_menu_id) : false;
}

RID NativeMenu::get_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu(p_menu_id) : RID();
}

String NativeMenu::get_system_menu_name(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_name(p_menu_id) : String();
}

String NativeMenu::get_system_menu_text(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_text(p_menu_id) : String();
}

void NativeMenu::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->set_system_menu_text(p_menu_id, p_name);
	}
}

RID NativeMenu::create_menu() { return is_initialized() ? data->driver->create_menu() : RID(); }

bool NativeMenu::has_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->has_menu(p_rid) : false;
}

void NativeMenu::free_menu(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->free_menu(p_rid);
	}
}

Size2 NativeMenu::get_size(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_size(p_rid) : Size2();
}

void NativeMenu::popup(const RID& p_rid, const Vector2i& p_position)
{
	if (is_initialized()) {
		data->driver->popup(p_rid, p_position);
	}
}

void NativeMenu::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	if (is_initialized()) {
		data->driver->set_interface_direction(p_rid, p_is_rtl);
	}
}

void NativeMenu::set_minimum_width(const RID& p_rid, float p_width)
{
	if (is_initialized()) {
		data->driver->set_minimum_width(p_rid, p_width);
	}
}

float NativeMenu::get_minimum_width(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_minimum_width(p_rid) : 0.f;
}

bool NativeMenu::is_opened(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_opened(p_rid) : false;
}

int NativeMenu::add_separator(const RID& p_rid, int p_index)
{
	return is_initialized() ? data->driver->add_separator(p_rid, p_index) : -1;
}

int NativeMenu::find_item_index_with_text(const RID& p_rid, const String& p_text)
{
	return is_initialized() ? data->driver->find_item_index_with_text(p_rid, p_text) : -1;
}

int NativeMenu::find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid)
{
	return is_initialized() ? data->driver->find_item_index_with_submenu(p_rid, p_submenu_rid) : -1;
}

bool NativeMenu::is_item_checked(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checked(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_indeterminate(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_indeterminate(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checkable(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_radio_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_radio_checkable(p_rid, p_idx) : false;
}

String NativeMenu::get_item_text(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_text(p_rid, p_idx) : String();
}

RID NativeMenu::get_item_submenu(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_submenu(p_rid, p_idx) : RID();
}

Key NativeMenu::get_item_accelerator(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_accelerator(p_rid, p_idx) : Key::NONE;
}

bool NativeMenu::is_item_disabled(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_disabled(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_hidden(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_hidden(p_rid, p_idx) : false;
}

String NativeMenu::get_item_tooltip(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_tooltip(p_rid, p_idx) : String();
}

int NativeMenu::get_item_state(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_state(p_rid, p_idx) : -1;
}

int NativeMenu::get_item_max_states(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_max_states(p_rid, p_idx) : -1;
}

Ref<Texture2D> NativeMenu::get_item_icon(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_icon(p_rid, p_idx) : Ref<Texture2D>();
}

int NativeMenu::get_item_indentation_level(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_indentation_level(p_rid, p_idx) : 0;
}

void NativeMenu::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	if (is_initialized()) {
		data->driver->set_item_checked(p_rid, p_idx, p_checked);
	}
}

void NativeMenu::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	if (is_initialized()) {
		data->driver->set_item_indeterminate(p_rid, p_idx, p_indeterminate);
	}
}

void NativeMenu::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_radio_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	if (is_initialized()) {
		data->driver->set_item_text(p_rid, p_idx, p_text);
	}
}

void NativeMenu::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	if (is_initialized()) {
		data->driver->set_item_submenu(p_rid, p_idx, p_submenu_rid);
	}
}

void NativeMenu::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	if (is_initialized()) {
		data->driver->set_item_accelerator(p_rid, p_idx, p_keycode);
	}
}

void NativeMenu::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	if (is_initialized()) {
		data->driver->set_item_disabled(p_rid, p_idx, p_disabled);
	}
}

void NativeMenu::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	if (is_initialized()) {
		data->driver->set_item_hidden(p_rid, p_idx, p_hidden);
	}
}

void NativeMenu::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	if (is_initialized()) {
		data->driver->set_item_tooltip(p_rid, p_idx, p_tooltip);
	}
}

void NativeMenu::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	if (is_initialized()) {
		data->driver->set_item_state(p_rid, p_idx, p_state);
	}
}

void NativeMenu::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	if (is_initialized()) {
		data->driver->set_item_max_states(p_rid, p_idx, p_max_states);
	}
}

void NativeMenu::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	if (is_initialized()) {
		data->driver->set_item_icon(p_rid, p_idx, p_icon);
	}
}

void NativeMenu::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	if (is_initialized()) {
		data->driver->set_item_indentation_level(p_rid, p_idx, p_level);
	}
}

int NativeMenu::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	return is_initialized() ? data->driver->set_item_index(p_rid, p_idx, p_target_idx) : -1;
}

int NativeMenu::get_item_count(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_item_count(p_rid) : 0;
}

bool NativeMenu::is_system_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_system_menu(p_rid) : false;
}

void NativeMenu::remove_item(const RID& p_rid, int p_idx)
{
	if (is_initialized()) {
		data->driver->remove_item(p_rid, p_idx);
	}
}

void NativeMenu::clear(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->clear(p_rid);
	}
}
