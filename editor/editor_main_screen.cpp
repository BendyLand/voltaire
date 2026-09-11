/**************************************************************************/
/*  editor_main_screen.cpp                                                */
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

#include "core/io/config_file.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor_main_screen.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"

void EditorMainScreen::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		set_accessibility_region(true);
		if (EDITOR_3D < buttons.size() && buttons[EDITOR_3D]->is_visible()) {
			// If the 3D editor is enabled, use this as the default.
			select(EDITOR_3D);
			return;
		}

		// Switch to the first main screen plugin that is enabled. Usually this is
		// 2D, but may be subsequent ones if 2D is disabled in the feature profile.
		for (int i = 0; i < buttons.size(); i++) {
			Button* editor_button = buttons[i];
			if (editor_button->is_visible()) {
				select(i);
				return;
			}
		}

		select(-1);
	} break;
	case NOTIFICATION_THEME_CHANGED: {
		for (int i = 0; i < buttons.size(); i++) {
			Button* tb = buttons[i];
			EditorPlugin* p_editor = editor_table[i];
			Ref<Texture2D> icon = p_editor->get_plugin_icon();

			if (icon.is_valid()) {
				tb->set_button_icon(icon);
			}
			else if (has_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons))) {
				tb->set_button_icon(
					get_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons)));
			}
		}
	} break;
	}
}

void EditorMainScreen::set_button_container(HBoxContainer* p_button_hb) { button_hb = p_button_hb; }

void EditorMainScreen::set_button_enabled(int p_index, bool p_enabled)
{
	ERR_FAIL_INDEX(p_index, buttons.size());
	buttons[p_index]->set_visible(p_enabled);
	if (!p_enabled && buttons[p_index]->is_pressed()) {
		select(EDITOR_2D);
	}
}

bool EditorMainScreen::is_button_enabled(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);
	return buttons[p_index]->is_visible();
}

int EditorMainScreen::_get_current_main_editor() const
{
	for (int i = 0; i < editor_table.size(); i++) {
		if (editor_table[i] == selected_plugin) {
			return i;
		}
	}

	return 0;
}

void EditorMainScreen::select_next()
{
	int editor = _get_current_main_editor();

	do {
		if (editor == editor_table.size() - 1) {
			editor = 0;
		}
		else {
			editor++;
		}
	} while (!buttons[editor]->is_visible());

	select(editor);
}

void EditorMainScreen::select_prev()
{
	int editor = _get_current_main_editor();

	do {
		if (editor == 0) {
			editor = editor_table.size() - 1;
		}
		else {
			editor--;
		}
	} while (!buttons[editor]->is_visible());

	select(editor);
}

void EditorMainScreen::select_by_name(const String& p_name)
{
	ERR_FAIL_COND(p_name.is_empty());

	for (int i = 0; i < buttons.size(); i++) {
		if (buttons[i]->get_text() == p_name) {
			select(i);
			return;
		}
	}

	ERR_FAIL_MSG("The editor name '" + p_name + "' was not found.");
}

int EditorMainScreen::get_selected_index() const
{
	for (int i = 0; i < editor_table.size(); i++) {
		if (selected_plugin == editor_table[i]) {
			return i;
		}
	}
	return -1;
}

int EditorMainScreen::get_plugin_index(EditorPlugin* p_editor) const
{
	int screen = -1;
	for (int i = 0; i < editor_table.size(); i++) {
		if (p_editor == editor_table[i]) {
			screen = i;
			break;
		}
	}
	return screen;
}

EditorPlugin* EditorMainScreen::get_selected_plugin() const { return selected_plugin; }

EditorPlugin* EditorMainScreen::get_plugin_by_name(const String& p_plugin_name) const
{
	ERR_FAIL_COND_V(!main_editor_plugins.has(p_plugin_name), nullptr);
	return main_editor_plugins[p_plugin_name];
}

VBoxContainer* EditorMainScreen::get_control() const { return main_screen_vbox; }

EditorMainScreen::EditorMainScreen()
{
	main_screen_vbox = memnew(VBoxContainer);
	main_screen_vbox->set_name("MainScreen");
	main_screen_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->add_theme_constant_override("separation", 0);
	add_child(main_screen_vbox);
}


