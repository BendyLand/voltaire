/**************************************************************************/
/*  action_map_editor.cpp                                                 */
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

#include "action_map_editor.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_event_search_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/event_listener_line_edit.h"
#include "editor/settings/input_event_configuration_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/check_button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/tree.h"

static bool _is_action_name_valid(const String& p_name)
{
	const char32_t* cstr = p_name.get_data();
	for (int i = 0; cstr[i]; i++) {
		if (cstr[i] == '/' || cstr[i] == ':' || cstr[i] == '"' || cstr[i] == '=' ||
			cstr[i] == '\\' || cstr[i] < 32) {
			return false;
		}
	}
	return true;
}

void ActionMapEditor::_add_action_pressed() { _add_action(add_edit->get_text()); }

String ActionMapEditor::_check_new_action_name(const String& p_name)
{
	if (p_name.is_empty() || !_is_action_name_valid(p_name)) {
		return TTR(
			"Invalid action name. It cannot be empty nor contain '/', ':', '=', '\\' or '\"'");
	}

	if (_has_action(p_name)) {
		return vformat(TTR("An action with the name '%s' already exists."), p_name);
	}

	return "";
}

void ActionMapEditor::_add_edit_text_changed(const String& p_name)
{
	const String error = _check_new_action_name(p_name);
	add_button->set_tooltip_text(error);
	add_button->set_disabled(!error.is_empty());
}

bool ActionMapEditor::_has_action(const String& p_name) const
{
	for (const ActionInfo& action_info : actions_cache) {
		if (p_name == action_info.name) {
			return true;
		}
	}
	return false;
}

void ActionMapEditor::_on_search_bar_value_changed()
{
	if (action_list_search_bar->is_searching()) {
		show_builtin_actions_checkbutton->set_pressed_no_signal(true);
		show_builtin_actions_checkbutton->set_disabled(true);
		show_builtin_actions_checkbutton->set_tooltip_text(
			TTRC("Built-in actions are always shown when searching."));
	}
	else {
		show_builtin_actions_checkbutton->set_pressed_no_signal(show_builtin_actions);
		show_builtin_actions_checkbutton->set_disabled(false);
		show_builtin_actions_checkbutton->set_tooltip_text(String());
	}
	update_action_list();
}

void ActionMapEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (!actions_cache.is_empty()) {
			update_action_list();
		}
		if (!add_button->get_tooltip_text().is_empty()) {
			_add_edit_text_changed(add_edit->get_text());
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		if (!actions_cache.is_empty()) {
			update_action_list();
		}
	} break;
	}
}

LineEdit* ActionMapEditor::get_search_box() const
{
	return action_list_search_bar->get_name_search_box();
}

LineEdit* ActionMapEditor::get_path_box() const { return add_edit; }

InputEventConfigurationDialog* ActionMapEditor::get_configuration_dialog()
{
	return event_config_dialog;
}

void ActionMapEditor::show_message(const String& p_message)
{
	message->set_text(p_message);
	message->popup_centered();
}


