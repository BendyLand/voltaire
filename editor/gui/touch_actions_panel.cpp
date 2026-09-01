/**************************************************************************/
/*  touch_actions_panel.cpp                                               */
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
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"
#include "touch_actions_panel.h"

void TouchActionsPanel::_hardware_keyboard_connected(bool p_connected)
{
	set_visible(!p_connected);
}

void TouchActionsPanel::_simulate_editor_shortcut(const String& p_shortcut_name)
{
	Ref<Shortcut> shortcut = ED_GET_SHORTCUT(p_shortcut_name);

	if (shortcut.is_valid() && !shortcut->get_events().is_empty()) {
		Ref<InputEventKey> event = shortcut->get_events()[0];
		if (event.is_valid()) {
			event->set_pressed(true);
			Input::get_singleton()->parse_input_event(event.ptr());
		}
	}
}

void TouchActionsPanel::_simulate_key_press(Key p_keycode)
{
	Ref<InputEventKey> event;
	event.instantiate();
	event->set_keycode(p_keycode);
	event->set_pressed(true);
	Input::get_singleton()->parse_input_event(event.ptr());
}

void TouchActionsPanel::_on_modifier_button_toggled(bool p_pressed, int p_modifier)
{
	switch ((Modifier)p_modifier) {
	case MODIFIER_CTRL:
		ctrl_btn_pressed = p_pressed;
		break;
	case MODIFIER_SHIFT:
		shift_btn_pressed = p_pressed;
		break;
	case MODIFIER_ALT:
		alt_btn_pressed = p_pressed;
		break;
	}
}

void TouchActionsPanel::_lock_panel_toggled(bool p_pressed)
{
	locked_panel = p_pressed;
	layout_toggle_button->set_visible(!p_pressed);
	drag_handle->set_visible(!p_pressed);
	reset_size();
	queue_redraw();
}


