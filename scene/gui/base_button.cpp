/**************************************************************************/
/*  base_button.cpp                                                       */
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

#include "base_button.h"
#include "core/config/project_settings.h"
#include "scene/gui/label.h"
#include "scene/main/timer.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void BaseButton::_unpress_group()
{
	if (button_group.is_null()) {
		return;
	}

	if (toggle_mode && !button_group->is_allow_unpress()) {
		status.pressed = true;
		queue_accessibility_update();
	}

	for (BaseButton* E : button_group->buttons) {
		if (E == this) {
			continue;
		}

		E->set_pressed(false);
	}
}

bool BaseButton::is_disabled() const { return status.disabled; }

void BaseButton::set_pressed_no_signal(bool p_pressed)
{
	if (!toggle_mode) {
		return;
	}
	if (status.pressed == p_pressed) {
		return;
	}
	status.pressed = p_pressed;
	queue_accessibility_update();
	queue_redraw();
}

bool BaseButton::is_pressing() const { return status.press_attempt; }

bool BaseButton::is_pressed() const { return toggle_mode ? status.pressed : status.press_attempt; }

bool BaseButton::is_hovered() const { return status.hovering; }

BaseButton::DrawMode BaseButton::get_draw_mode() const
{
	if (status.disabled) {
		return DRAW_DISABLED;
	}

	if (in_shortcut_feedback) {
		return DRAW_HOVER_PRESSED;
	}

	if (!status.press_attempt && status.hovering) {
		if (status.pressed) {
			return DRAW_HOVER_PRESSED;
		}

		return DRAW_HOVER;
	}
	else {
		// Determine if pressed or not.
		bool pressing;
		if (status.press_attempt) {
			pressing = (status.pressing_inside || keep_pressed_outside);
			if (status.pressed) {
				pressing = !pressing;
			}
		}
		else {
			pressing = status.pressed;
		}

		if (pressing) {
			return DRAW_PRESSED;
		}
		else {
			return DRAW_NORMAL;
		}
	}
}

bool BaseButton::has_point(const Point2& p_point) const
{
	ERR_READ_THREAD_GUARD_V(false);
	Rect2 rect = Rect2(Point2(), get_size()).grow(theme_cache.click_margin);
	return rect.has_area() && rect.has_point(p_point);
}

void BaseButton::set_toggle_mode(bool p_on)
{
	// Make sure to set 'pressed' to false if we are not in toggle mode
	if (!p_on) {
		set_pressed(false);
	}
	queue_accessibility_update();

	toggle_mode = p_on;
	update_configuration_warnings();
}

bool BaseButton::is_toggle_mode() const { return toggle_mode; }

void BaseButton::set_shortcut_in_tooltip(bool p_on)
{
	if (shortcut_in_tooltip != p_on) {
		shortcut_in_tooltip = p_on;
		queue_accessibility_update();
	}
}

bool BaseButton::is_shortcut_in_tooltip_enabled() const { return shortcut_in_tooltip; }

void BaseButton::set_action_mode(ActionMode p_mode) { action_mode = p_mode; }

BaseButton::ActionMode BaseButton::get_action_mode() const { return action_mode; }

void BaseButton::set_button_mask(uint32_t p_mask) { button_mask = p_mask; }

uint32_t BaseButton::get_button_mask() const { return button_mask; }

void BaseButton::set_keep_pressed_outside(bool p_on) { keep_pressed_outside = p_on; }

bool BaseButton::is_keep_pressed_outside() const { return keep_pressed_outside; }

void BaseButton::set_shortcut_feedback(bool p_enable) { shortcut_feedback = p_enable; }

bool BaseButton::is_shortcut_feedback() const { return shortcut_feedback; }

void BaseButton::set_shortcut(const Ref<Shortcut>& p_shortcut)
{
	if (shortcut != p_shortcut) {
		shortcut = p_shortcut;
		set_process_shortcut_input(shortcut.is_valid());
		queue_accessibility_update();
	}
}

Ref<Shortcut> BaseButton::get_shortcut() const { return shortcut; }

void BaseButton::_shortcut_feedback_timeout()
{
	in_shortcut_feedback = false;
	queue_redraw();
}

void BaseButton::set_button_group(const Ref<ButtonGroup>& p_group)
{
	if (button_group.is_valid()) {
		button_group->buttons.erase(this);
	}

	button_group = p_group;

	if (button_group.is_valid()) {
		button_group->buttons.insert(this);
	}

	queue_accessibility_update();
	queue_redraw(); // checkbox changes to radio if set a buttongroup
	update_configuration_warnings();
}

Ref<ButtonGroup> BaseButton::get_button_group() const { return button_group; }

bool BaseButton::_was_pressed_by_mouse() const { return was_mouse_pressed; }

PackedStringArray BaseButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (get_button_group().is_valid() && !is_toggle_mode()) {
		warnings.push_back(RTR("ButtonGroup is intended to be used only with buttons that have "
							   "toggle_mode set to true."));
	}

	return warnings;
}

void BaseButton::_bind_methods() {}

BaseButton::BaseButton() { set_focus_mode(FOCUS_ALL); }

BaseButton::~BaseButton()
{
	if (button_group.is_valid()) {
		button_group->buttons.erase(this);
	}
}

void ButtonGroup::get_buttons(List<BaseButton*>* r_buttons)
{
	for (BaseButton* E : buttons) {
		r_buttons->push_back(E);
	}
}

BaseButton* ButtonGroup::get_pressed_button()
{
	for (BaseButton* E : buttons) {
		if (E->is_pressed()) {
			return E;
		}
	}

	return nullptr;
}

void ButtonGroup::set_allow_unpress(bool p_enabled) { allow_unpress = p_enabled; }

bool ButtonGroup::is_allow_unpress() { return allow_unpress; }

void ButtonGroup::_bind_methods() {}

ButtonGroup::ButtonGroup() { set_local_to_scene(true); }


