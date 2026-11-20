/**************************************************************************/
/*  popup.cpp                                                             */
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

#include "popup.h"

#include "core/config/engine.h"
#include "scene/gui/panel.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

#ifdef TOOLS_ENABLED
#include "core/config/project_settings.h"
#endif

void PopupTracker::configure_window(Window* p_window)
{
	p_window->set_wrap_controls(true);
	p_window->set_visible(false);
	p_window->set_transient(true);
	p_window->set_flag(Window::FLAG_BORDERLESS, true);
	p_window->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_MINIMIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_MAXIMIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_POPUP, true);
	p_window->set_flag(Window::FLAG_POPUP_WM_HINT, true);
}

void PopupTracker::initialize_visible_parents(Window* p_window)
{
	if (p_window->is_embedded()) {
		visible_parents.clear();

		Window* parent_window = p_window;
		while (parent_window) {
			parent_window = parent_window->get_parent_visible_window();
			if (parent_window) {
				visible_parents.push_back(parent_window);
			}
		}
	}
}

void PopupTracker::deinitialize_visible_parents(Window* p_window)
{
	visible_parents.clear();
}

void PopupTracker::post_popup()
{
	popped_up = true;
}

bool PopupTracker::handle_cancel_input(const Window* p_window, const InputEvent& p_event)
{
	if (p_window->get_flag(Window::FLAG_POPUP) &&
		p_event.is_action_pressed(SNAME("ui_cancel"), false, true)) {
		hide_reason = HIDE_REASON_CANCELED; // ESC pressed, mark as canceled unconditionally.
		return true;
	}
	return false;
}

bool PopupTracker::handle_parent_focused(const Window* p_window)
{
	if (popped_up && p_window->get_flag(Window::FLAG_POPUP)) {
		if (hide_reason == HIDE_REASON_NONE) {
			hide_reason = HIDE_REASON_UNFOCUSED;
		}
		return true;
	}
	return false;
}

void PopupTracker::close_pressed(Window* p_window)
{
	popped_up = false;
	deinitialize_visible_parents(p_window);
	p_window->hide();
}

Rect2i PopupTracker::adjust_popup_rect(const Window* p_window)
{
	ERR_FAIL_COND_V(!p_window->is_inside_tree(), Rect2());
	Rect2i parent_rect = p_window->get_usable_parent_rect();

	if (parent_rect == Rect2i()) {
		return Rect2i();
	}

	Rect2i current(p_window->get_position(), p_window->get_size());

	if (!p_window->is_embedded() &&
		DisplayServer::has_feature(DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
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
	Size2i popup_max_size = p_window->get_max_size();
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

void Popup::set_panel_enabled(bool p_enabled)
{
	if (is_panel == p_enabled) {
		return;
	}
	is_panel = p_enabled;
	if (is_panel) {
		if (!panel) {
			panel = memnew(Panel);
			panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			add_child(panel, false, INTERNAL_MODE_FRONT);
		}
	}
	else if (panel) {
		remove_child(panel);
		memdelete(panel);
		panel = nullptr;
	}
}

void Popup::_input_from_window(const InputEvent& p_event)
{
	if (is_panel) {
		if (close_on_accept && p_event.is_action_pressed(SNAME("ui_accept"), false, true)) {
			_close_pressed();
		}
		if (p_event.is_valid()) {
			if (!get_flag(FLAG_POPUP)) {
				return;
			}

			// Hide it if the shadows have been clicked.
			if (panel && p_event.type == InputEventType::MOUSE_BUTTON && p_event.is_pressed() &&
				p_event.mouse_button.button_index == MouseButton::LEFT) {
				Rect2 panel_area = panel->get_global_rect();
				float win_scale = get_content_scale_factor();
				panel_area.position *= win_scale;
				panel_area.size *= win_scale;
				if (!panel_area.has_point(p_event.mouse_button.position)) {
					_close_pressed();
				}
			}
		}
		else {
			WARN_PRINT_ONCE("Popup has received an invalid InputEvent. Consider filtering out "
							"invalid events.");
		}
	}

	if (tracker.handle_cancel_input(this, p_event)) {
		_close_pressed();
	}
	Window::_input_from_window(p_event);
}

void Popup::_parent_focused()
{
	if (tracker.handle_parent_focused(this)) {
		_close_pressed();
	}
}

void Popup::_post_popup()
{
	Window::_post_popup();
	tracker.post_popup();
}

Rect2i Popup::_popup_adjust_rect() const
{
	Rect2i current = PopupTracker::adjust_popup_rect(this);
	if (!is_panel || !panel || current == Rect2i()) {
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

void Popup::_update_shadow_offsets() const
{
	if (!panel) {
		return;
	}

	if (!DisplayServer::is_window_transparency_available() && !is_embedded()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const Ref<StyleBox> sb = theme_cache.panel_style;
	if (sb.is_null() || sb->get_style_type() != StyleBox::STYLE_FLAT) {
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

void Popup::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
			tracker.hide_reason = HIDE_REASON_NONE;
			tracker.initialize_visible_parents(this);
		}
		else {
			tracker.deinitialize_visible_parents(this);
			tracker.popped_up = false;
		}

		if (is_panel && panel) {
			if (!is_visible()) {
				// Remove the extra space used by the shadows, so they can be ignored when the popup
				// is hidden.
				panel->set_offsets_preset(
					Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
				_update_child_rects();

				if (pre_popup_rect != Rect2i()) {
					set_position(pre_popup_rect.position);
					set_size(pre_popup_rect.size);

					pre_popup_rect = Rect2i();
				}
			}
			else if (pre_popup_rect == Rect2i()) {
				// The popup was made visible directly (without `popup_*()`), so just update the
				// offsets without touching the rect.
				_update_shadow_offsets();
				_update_child_rects();
			}
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		if (is_panel && panel) {
			panel->add_theme_style_override(SceneString(panel), theme_cache.panel_style.ptr());

			if (is_visible()) {
				_update_shadow_offsets();
			}

			_update_child_rects();
		}
	} break;

	case Control::NOTIFICATION_TRANSLATION_CHANGED:
	case Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		if (is_panel && panel && is_visible()) {
			_update_shadow_offsets();
		}
	} break;

	case NOTIFICATION_WM_SIZE_CHANGED: {
		if (is_panel && panel) {
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
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		tracker.deinitialize_visible_parents(this);
	} break;
	}
}

void Popup::_update_child_rects() const {}

void Popup::_close_pressed()
{
	tracker.close_pressed(this);
}

Size2 Popup::_get_contents_minimum_size() const { return Size2(); }

#ifdef TOOLS_ENABLED
PackedStringArray Popup::get_configuration_warnings() const
{
	PackedStringArray warnings = Window::get_configuration_warnings();

	if (is_panel && !DisplayServer::is_window_transparency_available() &&
		!GLOBAL_GET_CACHED(bool, "display/window/subwindows/embed_subwindows")) {
		Ref<StyleBox> sb = theme_cache.panel_style;
		if (sb.is_valid() && sb->get_style_type() == StyleBox::STYLE_FLAT &&
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

Popup::Popup(bool p_panel)
{
	tracker.configure_window(this);
	if (p_panel) {
		set_panel_enabled(true);
	}
}

Popup::~Popup() {}
