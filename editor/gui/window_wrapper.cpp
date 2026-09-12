/**************************************************************************/
/*  window_wrapper.cpp                                                    */
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

#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/progress_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "window_wrapper.h"

Rect2 WindowWrapper::_get_default_window_rect() const
{
	// Assume that the control rect is the desired one for the window.
	return wrapped_control->get_screen_rect();
}

Node* WindowWrapper::_get_wrapped_control_parent() const
{
	if (margins) {
		return margins;
	}
	return window;
}

void WindowWrapper::_notification(int p_what)
{
	if (!is_window_available()) {
		return;
	}
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		// Grab the focus when WindowWrapper.set_visible(true) is called
		// and the window is showing.
		grab_window_focus();
	} break;
	case NOTIFICATION_READY: {
		set_process_shortcut_input(true);
	} break;
	case NOTIFICATION_THEME_CHANGED: {
		window_background->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox("PanelForeground", EditorStringName(EditorStyles)).ptr());
	} break;
	}
}

void WindowWrapper::set_wrapped_control(Control* p_control, const Ref<Shortcut>& p_enable_shortcut)
{
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(wrapped_control);

	wrapped_control = p_control;
	enable_shortcut = p_enable_shortcut;
	add_child(p_control);
}

Control* WindowWrapper::get_wrapped_control() const { return wrapped_control; }

bool WindowWrapper::is_window_available() const { return window != nullptr; }

bool WindowWrapper::get_window_enabled() const
{
	return is_window_available() ? window->is_visible() : false;
}

Rect2i WindowWrapper::get_window_rect() const
{
	ERR_FAIL_COND_V(!get_window_enabled(), Rect2i());
	return Rect2i(window->get_position(), window->get_size());
}

int WindowWrapper::get_window_screen() const
{
	ERR_FAIL_COND_V(!get_window_enabled(), -1);
	return window->get_current_screen();
}

void WindowWrapper::set_window_title(const String& p_title)
{
	if (!is_window_available()) {
		return;
	}
	window->set_title(p_title);
}

void WindowWrapper::set_margins_enabled(bool p_enabled)
{
	if (!is_window_available()) {
		return;
	}

	if (!p_enabled && margins) {
		margins->queue_free();
		margins = nullptr;
	}
	else if (p_enabled && !margins) {
		Size2 borders = Size2(4, 4) * EDSCALE;
		margins = memnew(MarginContainer);
		margins->add_theme_constant_override("margin_right", borders.width);
		margins->add_theme_constant_override("margin_top", borders.height);
		margins->add_theme_constant_override("margin_left", borders.width);
		margins->add_theme_constant_override("margin_bottom", borders.height);

		window->add_child(margins);
		margins->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	}
}

Size2 WindowWrapper::get_margins_size()
{
	if (!margins) {
		return Size2();
	}

	return Size2(margins->get_margin_size(SIDE_LEFT) + margins->get_margin_size(SIDE_RIGHT),
		margins->get_margin_size(SIDE_TOP) + margins->get_margin_size(SIDE_RIGHT));
}

Size2 WindowWrapper::get_margins_top_left()
{
	if (!margins) {
		return Size2();
	}

	return Size2(margins->get_margin_size(SIDE_LEFT), margins->get_margin_size(SIDE_TOP));
}

void WindowWrapper::grab_window_focus()
{
	if (get_window_enabled() && is_visible()) {
		window->grab_focus();
	}
}

void WindowWrapper::set_override_close_request(bool p_enabled)
{
	override_close_request = p_enabled;
}

void ScreenSelect::_show_popup()
{
	// Adapted from /scene/gui/menu_button.cpp::show_popup
	if (!get_viewport()) {
		return;
	}

	Size2 size = get_size() * get_viewport()->get_canvas_transform().get_scale();

	popup->set_size(Size2(size.width, 0));
	Point2 gp = get_screen_position();
	gp.y += size.y;
	if (is_layout_rtl()) {
		gp.x += size.width - popup->get_size().width;
	}
	popup->set_position(gp);
	popup->popup();
}


