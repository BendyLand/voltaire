/**************************************************************************/
/*  popup.cpp                                                             */
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

#include "core/config/engine.h"
#include "popup.h"
#include "scene/gui/panel.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

#ifdef TOOLS_ENABLED
#include "core/config/project_settings.h"
#endif

void Popup::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (get_flag(FLAG_POPUP) && p_event->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		hide_reason = HIDE_REASON_CANCELED; // ESC pressed, mark as canceled unconditionally.
		_close_pressed();
	}
	Window::_input_from_window(p_event);
}

void Popup::_parent_focused()
{
	if (popped_up && get_flag(FLAG_POPUP)) {
		if (hide_reason == HIDE_REASON_NONE) {
			hide_reason = HIDE_REASON_UNFOCUSED;
		}
		_close_pressed();
	}
}

void Popup::_post_popup()
{
	Window::_post_popup();
	popped_up = true;
}

Rect2i Popup::_popup_adjust_rect() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	Rect2i parent_rect = get_usable_parent_rect();

	if (parent_rect == Rect2i()) {
		return Rect2i();
	}

	Rect2i current(get_position(), get_size());

	if (!is_embedded() && DisplayServer::get_singleton()->has_feature(
							  DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		// We're fine as is, the Display Server will take care of that for us.
		return current;
	}

	if (current.position.x + current.size.x > parent_rect.position.x + parent_rect.size.x) {
		current.position.x = parent_rect.position.x + parent_rect.size.x - current.size.x;
	}

	if (current.position.x < parent_rect.position.x) {
		current.position.x = parent_rect.position.x;
	}

	if (current.position.y + current.size.y > parent_rect.position.y + parent_rect.size.y) {
		current.position.y = parent_rect.position.y + parent_rect.size.y - current.size.y;
	}

	if (current.position.y < parent_rect.position.y) {
		current.position.y = parent_rect.position.y;
	}

	if (current.size.y > parent_rect.size.y) {
		current.size.y = parent_rect.size.y;
	}

	if (current.size.x > parent_rect.size.x) {
		current.size.x = parent_rect.size.x;
	}

	// Early out if max size not set.
	Size2i popup_max_size = get_max_size();
	if (popup_max_size <= Size2()) {
		return current;
	}

	if (current.size.x > popup_max_size.x) {
		current.size.x = popup_max_size.x;
	}

	if (current.size.y > popup_max_size.y) {
		current.size.y = popup_max_size.y;
	}

	return current;
}


Popup::Popup()
{
	set_wrap_controls(true);
	set_visible(false);
	set_transient(true);
	set_flag(FLAG_BORDERLESS, true);
	set_flag(FLAG_RESIZE_DISABLED, true);
	set_flag(FLAG_MINIMIZE_DISABLED, true);
	set_flag(FLAG_MAXIMIZE_DISABLED, true);
	set_flag(FLAG_POPUP, true);
	set_flag(FLAG_POPUP_WM_HINT, true);
}

Popup::~Popup() {}

#ifdef TOOLS_ENABLED
PackedStringArray PopupPanel::get_configuration_warnings() const
{
	PackedStringArray warnings = Popup::get_configuration_warnings();

	if (!DisplayServer::get_singleton()->is_window_transparency_available() &&
		!GLOBAL_GET_CACHED(bool, "display/window/subwindows/embed_subwindows")) {
		Ref<StyleBoxFlat> sb = theme_cache.panel_style;
		if (sb.is_valid() &&
			(sb->get_shadow_size() > 0 || sb->get_corner_radius(CORNER_TOP_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_TOP_RIGHT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_RIGHT) > 0)) {
			warnings.push_back(RTR(
				"The current theme style has shadows and/or rounded corners for popups, but those "
				"won't display correctly if \"display/window/per_pixel_transparency/allowed\" "
				"isn't enabled in the Project Settings, nor if it isn't supported."));
		}
	}

	return warnings;
}
#endif

void PopupPanel::_input_from_window(const Ref<InputEvent>& p_event)
{
	if (p_event.is_valid()) {
		if (!get_flag(FLAG_POPUP)) {
			return;
		}

		Ref<InputEventMouseButton> b = p_event;
		// Hide it if the shadows have been clicked.
		if (b.is_valid() && b->is_pressed() && b->get_button_index() == MouseButton::LEFT) {
			Rect2 panel_area = panel->get_global_rect();
			float win_scale = get_content_scale_factor();
			panel_area.position *= win_scale;
			panel_area.size *= win_scale;
			if (!panel_area.has_point(b->get_position())) {
				_close_pressed();
			}
		}
	}
	else {
		WARN_PRINT_ONCE("PopupPanel has received an invalid InputEvent. Consider filtering out "
						"invalid events.");
	}

	Popup::_input_from_window(p_event);
}

Rect2i PopupPanel::_popup_adjust_rect() const
{
	Rect2i current = Popup::_popup_adjust_rect();
	if (current == Rect2i()) {
		return current;
	}

	pre_popup_rect = current;

	_update_shadow_offsets();
	_update_child_rects();

	if (is_layout_rtl()) {
		current.position -= Vector2(-panel->get_offset(SIDE_RIGHT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	else {
		current.position -= Vector2(panel->get_offset(SIDE_LEFT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	current.size += Vector2(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
						panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM)) *
					get_content_scale_factor();

	return current;
}

void PopupPanel::_update_shadow_offsets() const
{
	if (!DisplayServer::get_singleton()->is_window_transparency_available() && !is_embedded()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const Ref<StyleBoxFlat> sb = theme_cache.panel_style;
	if (sb.is_null()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const int shadow_size = sb->get_shadow_size();
	if (shadow_size == 0) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	// Offset the background panel so it leaves space inside the window for the shadows to be drawn.
	const Point2 shadow_offset = sb->get_shadow_offset();
	if (is_layout_rtl()) {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size + shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size + shadow_offset.x));
	}
	else {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size - shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size - shadow_offset.x));
	}
	panel->set_offset(SIDE_TOP, MAX(0, shadow_size - shadow_offset.y));
	panel->set_offset(SIDE_BOTTOM, MIN(0, -shadow_size - shadow_offset.y));
}

void PopupPanel::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		panel->add_theme_style_override(SceneStringName(panel), theme_cache.panel_style.ptr());

		if (is_visible()) {
			_update_shadow_offsets();
		}

		_update_child_rects();

#ifdef TOOLS_ENABLED
		update_configuration_warnings();
#endif
	} break;

	case Control::NOTIFICATION_TRANSLATION_CHANGED:
	case Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		if (is_visible()) {
			_update_shadow_offsets();
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			// Remove the extra space used by the shadows, so they can be ignored when the popup is
			// hidden.
			panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
			_update_child_rects();

			if (pre_popup_rect != Rect2i()) {
				set_position(pre_popup_rect.position);
				set_size(pre_popup_rect.size);

				pre_popup_rect = Rect2i();
			}
		}
		else if (pre_popup_rect == Rect2i()) {
			// The popup was made visible directly (without `popup_*()`), so just update the offsets
			// without touching the rect.
			_update_shadow_offsets();
			_update_child_rects();
		}
	} break;

	case NOTIFICATION_WM_SIZE_CHANGED: {
		_update_child_rects();

		if (is_visible()) {
			const Vector2i offsets =
				Vector2i(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
					panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM));
			// Check if the size actually changed.
			if (pre_popup_rect.size + offsets != get_size()) {
				// Play safe, and stick with the new size.
				pre_popup_rect = Rect2i();
			}
		}
	} break;
	}
}


