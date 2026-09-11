/**************************************************************************/
/*  input_event_configuration_dialog.cpp                                  */
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

#include "core/input/input_map.h"
#include "editor/editor_string_names.h"
#include "editor/settings/event_listener_line_edit.h"
#include "editor/themes/editor_scale.h"
#include "input_event_configuration_dialog.h"
#include "scene/gui/check_box.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/tree.h"

void InputEventConfigurationDialog::_on_listen_input_changed(const Ref<InputEvent>& p_event)
{
	// Ignore if invalid, echo or not pressed
	if (p_event.is_null() || p_event->is_echo() || !p_event->is_pressed()) {
		return;
	}

	// Create an editable reference and a copy of full event.
	Ref<InputEvent> received_event = p_event;
	Ref<InputEvent> received_original_event = received_event->duplicate();

	// Check what the type is and if it is allowed.
	Ref<InputEventKey> k = received_event;
	Ref<InputEventJoypadButton> joyb = received_event;
	Ref<InputEventJoypadMotion> joym = received_event;
	Ref<InputEventMouseButton> mb = received_event;

	int type = 0;
	if (k.is_valid()) {
		type = INPUT_KEY;
	}
	else if (joyb.is_valid()) {
		type = INPUT_JOY_BUTTON;
	}
	else if (joym.is_valid()) {
		type = INPUT_JOY_MOTION;
	}
	else if (mb.is_valid()) {
		type = INPUT_MOUSE_BUTTON;
	}

	if (!(allowed_input_types & type)) {
		return;
	}

	if (joym.is_valid()) {
		joym->set_axis_value(SIGN(joym->get_axis_value()));
	}

	if (k.is_valid()) {
		k->set_pressed(false); // To avoid serialization of 'pressed' property - doesn't matter for
							   // actions anyway.
		if (key_mode->get_selected_id() == KEYMODE_KEYCODE) {
			k->set_physical_keycode(Key::NONE);
			k->set_key_label(Key::NONE);
		}
		else if (key_mode->get_selected_id() == KEYMODE_PHY_KEYCODE) {
			k->set_keycode(Key::NONE);
			k->set_key_label(Key::NONE);
		}
		else if (key_mode->get_selected_id() == KEYMODE_UNICODE) {
			k->set_physical_keycode(Key::NONE);
			k->set_keycode(Key::NONE);
		}
		if (key_location->get_selected_id() == (int)KeyLocation::UNSPECIFIED) {
			k->set_location(KeyLocation::UNSPECIFIED);
		}
	}

	Ref<InputEventWithModifiers> mod = received_event;
	if (mod.is_valid()) {
		mod->set_window_id(0);
	}

	// Maintain device selection.
	received_event->set_device(_get_current_device());

	_set_event(received_event, received_original_event);
}

void InputEventConfigurationDialog::_search_term_updated(const String&) { _update_input_list(); }

void InputEventConfigurationDialog::_mod_toggled(bool p_checked, int p_index)
{
	Ref<InputEventWithModifiers> ie = event;

	// Not event with modifiers
	if (ie.is_null()) {
		return;
	}

	if (p_index == 0) {
		ie->set_alt_pressed(p_checked);
	}
	else if (p_index == 1) {
		ie->set_shift_pressed(p_checked);
	}
	else if (p_index == 2) {
		if (!autoremap_command_or_control_checkbox->is_pressed()) {
			ie->set_ctrl_pressed(p_checked);
		}
	}
	else if (p_index == 3) {
		if (!autoremap_command_or_control_checkbox->is_pressed()) {
			ie->set_meta_pressed(p_checked);
		}
	}

	_set_event(ie, original_event);
}

void InputEventConfigurationDialog::_autoremap_command_or_control_toggled(bool p_checked)
{
	Ref<InputEventWithModifiers> ie = event;
	if (ie.is_valid()) {
		ie->set_command_or_control_autoremap(p_checked);
		_set_event(ie, original_event);
	}

	if (p_checked) {
		mod_checkboxes[MOD_META]->hide();
		mod_checkboxes[MOD_CTRL]->hide();
	}
	else {
		mod_checkboxes[MOD_META]->show();
		mod_checkboxes[MOD_CTRL]->show();
	}
}

void InputEventConfigurationDialog::_key_mode_selected(int p_mode)
{
	Ref<InputEventKey> k = event;
	Ref<InputEventKey> ko = original_event;
	if (k.is_null() || ko.is_null()) {
		return;
	}

	if (key_mode->get_selected_id() == KEYMODE_KEYCODE) {
		k->set_keycode(ko->get_keycode());
		k->set_physical_keycode(Key::NONE);
		k->set_key_label(Key::NONE);
	}
	else if (key_mode->get_selected_id() == KEYMODE_PHY_KEYCODE) {
		k->set_keycode(Key::NONE);
		k->set_physical_keycode(ko->get_physical_keycode());
		k->set_key_label(Key::NONE);
	}
	else if (key_mode->get_selected_id() == KEYMODE_UNICODE) {
		k->set_physical_keycode(Key::NONE);
		k->set_keycode(Key::NONE);
		k->set_key_label(ko->get_key_label());
	}

	_set_event(k, original_event);
}

void InputEventConfigurationDialog::_key_location_selected(int p_location)
{
	Ref<InputEventKey> k = event;
	if (k.is_null()) {
		return;
	}

	k->set_location((KeyLocation)p_location);

	_set_event(k, original_event);
}

void InputEventConfigurationDialog::_input_list_item_activated()
{
	TreeItem* selected = input_list_tree->get_selected();
	selected->set_collapsed(!selected->is_collapsed());
}

void InputEventConfigurationDialog::_device_selection_changed(int p_option_button_index)
{
	// Subtract 1 as option index 0 corresponds to "All Devices" (value of -1)
	// and option index 1 corresponds to device 0, etc...
	event->set_device(p_option_button_index - 1);
	event_as_text->set_text(EventListenerLineEdit::get_event_text(event, true));
}

void InputEventConfigurationDialog::_set_current_device(int p_device)
{
	device_id_option->select(p_device + 1);
}

int InputEventConfigurationDialog::_get_current_device() const
{
	return device_id_option->get_selected() - 1;
}

void InputEventConfigurationDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		event_listener->grab_focus();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		input_list_search->set_right_icon(get_editor_theme_icon(SNAME("Search")));

		key_mode->set_item_icon(KEYMODE_KEYCODE, get_editor_theme_icon(SNAME("Keyboard")));
		key_mode->set_item_icon(
			KEYMODE_PHY_KEYCODE, get_editor_theme_icon(SNAME("KeyboardPhysical")));
		key_mode->set_item_icon(KEYMODE_UNICODE, get_editor_theme_icon(SNAME("KeyboardLabel")));

		icon_cache.keyboard = get_editor_theme_icon(SNAME("Keyboard"));
		icon_cache.mouse = get_editor_theme_icon(SNAME("Mouse"));
		icon_cache.mouse_left_button = get_editor_theme_icon(SNAME("MouseButtonLeft"));

		icon_cache.mouse_right_button = get_editor_theme_icon(SNAME("MouseButtonRight"));
		icon_cache.mouse_middle_button = get_editor_theme_icon(SNAME("MouseButtonMiddle"));
		icon_cache.mouse_wheel_up = get_editor_theme_icon(SNAME("MouseButtonWheelUp"));
		icon_cache.mouse_wheel_down = get_editor_theme_icon(SNAME("MouseButtonWheelDown"));
		icon_cache.mouse_wheel_left = get_editor_theme_icon(SNAME("MouseButtonWheelLeft"));
		icon_cache.mouse_wheel_right = get_editor_theme_icon(SNAME("MouseButtonWheelRight"));
		icon_cache.mouse_xbutton1 = get_editor_theme_icon(SNAME("MouseButtonXButton1"));
		icon_cache.mouse_xbutton2 = get_editor_theme_icon(SNAME("MouseButtonXButton2"));
		icon_cache.joypad_button = get_editor_theme_icon(SNAME("JoyButton"));
		icon_cache.joypad_axis = get_editor_theme_icon(SNAME("JoyAxis"));

		event_as_text->add_theme_font_override(SceneStringName(font),
			get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)).ptr());
		event_exists->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));

		_update_input_list();
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		key_location->set_item_text(key_location->get_item_index((int)KeyLocation::UNSPECIFIED),
			TTR("Unspecified", "Key Location"));
		key_location->set_item_text(
			key_location->get_item_index((int)KeyLocation::LEFT), TTR("Left", "Key Location"));
		key_location->set_item_text(
			key_location->get_item_index((int)KeyLocation::RIGHT), TTR("Right", "Key Location"));
	} break;
	}
}

Ref<InputEvent> InputEventConfigurationDialog::get_event() const { return event; }

void InputEventConfigurationDialog::set_allowed_input_types(int p_type_masks)
{
	allowed_input_types = p_type_masks;
	event_listener->set_allowed_input_types(p_type_masks);
}


