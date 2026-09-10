/**************************************************************************/
/*  spin_box.cpp                                                          */
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

#include "core/input/input.h"
#include "core/math/expression.h"
#include "core/string/translation_server.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "spin_box.h"

Size2 SpinBox::get_minimum_size() const
{
	Size2 ms = line_edit->get_bound_minimum_size();
	ms.width += sizing_cache.buttons_block_width;
	return ms;
}

LineEdit* SpinBox::get_line_edit() { return line_edit; }

void SpinBox::_line_edit_input(const Ref<InputEvent>& p_event)
{
	if (drag.enabled) {
		line_edit->accept_event();
	}
}

void SpinBox::_range_click_timeout()
{
	if (!drag.enabled && Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
		Rect2 up_button_rc = Rect2(sizing_cache.buttons_left, 0, sizing_cache.buttons_width,
			sizing_cache.button_up_height);
		Rect2 down_button_rc = Rect2(sizing_cache.buttons_left, sizing_cache.second_button_top,
			sizing_cache.buttons_width, sizing_cache.button_down_height);

		Vector2 mpos = get_local_mouse_position();

		bool mouse_on_up_button = up_button_rc.has_point(mpos);
		bool mouse_on_down_button = down_button_rc.has_point(mpos);

		if (mouse_on_up_button || mouse_on_down_button) {
			_arrow_clicked(mouse_on_up_button);
		}

		if (range_click_timer->is_one_shot()) {
			range_click_timer->set_wait_time(0.075);
			range_click_timer->set_one_shot(false);
			range_click_timer->start();
		}

	}
	else {
		range_click_timer->stop();
	}
}

void SpinBox::_release_mouse_from_drag_mode()
{
	if (drag.enabled) {
		drag.enabled = false;
		Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_HIDDEN);
		warp_mouse(drag.capture_pos);
		Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
	}
}

void SpinBox::_arrow_clicked(bool p_up)
{
	double arrow_step = get_custom_arrow_step() != 0.0 ? get_custom_arrow_step() : get_step();
	if (custom_arrow_round) {
		// Arrow button is being pressed, snap the value to next `arrow_step`.
		// `arrow_step` should be a multiple of `step`, otherwise it may not be able to
		// increase/decrease the value.
		arrow_step = Math::snapped(arrow_step, get_step());
		double new_value = _calc_value(get_value(), arrow_step);
		if ((p_up && new_value <= get_value()) || (!p_up && new_value >= get_value())) {
			new_value = _calc_value(get_value() + (p_up ? arrow_step : -arrow_step), arrow_step);
		}
		set_value(new_value);
	}
	else {
		set_value(get_value() + (p_up ? arrow_step : -arrow_step));
	}
}

void SpinBox::_line_edit_editing_toggled(bool p_toggled_on)
{
	if (p_toggled_on) {
		int col = line_edit->get_caret_column();
		_update_text();
		line_edit->set_caret_column(col);

		// LineEdit text might change and it clears any selection. Have to re-select here.
		if (line_edit->is_select_all_on_focus() &&
			!Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
			line_edit->select_all();
		}
	}
	else {
		accepted = true;

		if (Input::get_singleton()->is_action_pressed("ui_cancel") ||
			line_edit->get_text().is_empty()) {
			_update_text(); // Revert text if editing was canceled.
		}
		else {
			line_edit->set_text(line_edit->get_text().trim_suffix(".").trim_suffix(","));
			_update_text(
				true); // Update text in case value was changed this frame (e.g. on `focus_exited`).
			_text_submitted(line_edit->get_text());
		}
	}
}

inline void SpinBox::_compute_sizes()
{
	int buttons_block_wanted_width =
		theme_cache.buttons_width + theme_cache.field_and_buttons_separation;
	int buttons_block_icon_enforced_width =
		_get_widest_button_icon_width() + theme_cache.field_and_buttons_separation;

#ifndef DISABLE_DEPRECATED
	const bool min_width_from_icons =
		theme_cache.set_min_buttons_width_from_icons || (theme_cache.buttons_width < 0);
#else
	const bool min_width_from_icons = theme_cache.buttons_width < 0;
#endif
	int w = min_width_from_icons != 0
				? MAX(buttons_block_icon_enforced_width, buttons_block_wanted_width)
				: buttons_block_wanted_width;

	if (w != sizing_cache.buttons_block_width) {
		line_edit->set_offset(SIDE_LEFT, 0);
		line_edit->set_offset(SIDE_RIGHT, -w);
		sizing_cache.buttons_block_width = w;
	}

	Size2i size = get_size();

	sizing_cache.buttons_width = w - theme_cache.field_and_buttons_separation;
	sizing_cache.buttons_vertical_separation =
		CLAMP(theme_cache.buttons_vertical_separation, 0, size.height);
	sizing_cache.buttons_left = is_layout_rtl() ? 0 : size.width - sizing_cache.buttons_width;
	sizing_cache.button_up_height = (size.height - sizing_cache.buttons_vertical_separation) / 2;
	sizing_cache.button_down_height =
		size.height - sizing_cache.button_up_height - sizing_cache.buttons_vertical_separation;
	sizing_cache.second_button_top = size.height - sizing_cache.button_down_height;

	sizing_cache.buttons_separator_top = sizing_cache.button_up_height;
	sizing_cache.field_and_buttons_separator_left =
		is_layout_rtl() ? sizing_cache.buttons_width
						: size.width - sizing_cache.buttons_block_width;
	sizing_cache.field_and_buttons_separator_width = theme_cache.field_and_buttons_separation;
}

inline int SpinBox::_get_widest_button_icon_width()
{
	int max = 0;
#ifndef DISABLE_DEPRECATED
	max = MAX(max, theme_cache.updown_icon->get_width());
#endif
	max = MAX(max, theme_cache.up_icon->get_width());
	max = MAX(max, theme_cache.up_hover_icon->get_width());
	max = MAX(max, theme_cache.up_pressed_icon->get_width());
	max = MAX(max, theme_cache.up_disabled_icon->get_width());
	max = MAX(max, theme_cache.down_icon->get_width());
	max = MAX(max, theme_cache.down_hover_icon->get_width());
	max = MAX(max, theme_cache.down_pressed_icon->get_width());
	max = MAX(max, theme_cache.down_disabled_icon->get_width());
	return max;
}

void SpinBox::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	line_edit->set_horizontal_alignment(p_alignment);
}

HorizontalAlignment SpinBox::get_horizontal_alignment() const
{
	return line_edit->get_horizontal_alignment();
}

void SpinBox::set_suffix(const String& p_suffix)
{
	if (suffix == p_suffix) {
		return;
	}

	suffix = p_suffix;
	_update_text();
}

String SpinBox::get_suffix() const { return suffix; }

void SpinBox::set_prefix(const String& p_prefix)
{
	if (prefix == p_prefix) {
		return;
	}

	prefix = p_prefix;
	_update_text();
}

String SpinBox::get_prefix() const { return prefix; }

bool SpinBox::get_update_on_text_changed() const { return update_on_text_changed; }

void SpinBox::set_select_all_on_focus(bool p_enabled)
{
	line_edit->set_select_all_on_focus(p_enabled);
}

bool SpinBox::is_select_all_on_focus() const { return line_edit->is_select_all_on_focus(); }

bool SpinBox::is_editable() const { return line_edit->is_editable(); }

void SpinBox::apply() { _text_submitted(line_edit->get_text()); }

void SpinBox::set_custom_arrow_step(double p_custom_arrow_step)
{
	custom_arrow_step = p_custom_arrow_step;
}

double SpinBox::get_custom_arrow_step() const { return custom_arrow_step; }

void SpinBox::set_custom_arrow_round(bool p_round) { custom_arrow_round = p_round; }

bool SpinBox::is_custom_arrow_rounding() const { return custom_arrow_round; }

void SpinBox::_value_changed(double p_value)
{
	_update_buttons_state_for_current_value();
	Range::_value_changed(p_value);
}


