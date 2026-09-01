/**************************************************************************/
/*  editor_command_palette.cpp                                            */
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

#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_command_palette.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/tree.h"

EditorCommandPalette* EditorCommandPalette::singleton = nullptr;

static Rect2i prev_rect = Rect2i();
static bool was_showed = false;

float EditorCommandPalette::_score_path(const String& p_search, const String& p_path)
{
	float score = 0.9f + .1f * (p_search.length() / (float)p_path.length());

	// Positive bias for matches close to the beginning of the file name.
	int pos = p_path.findn(p_search);
	if (pos != -1) {
		return score * (1.0f - 0.1f * (float(pos) / p_path.length()));
	}

	// Positive bias for matches close to the end of the path.
	pos = p_path.rfindn(p_search);
	if (pos != -1) {
		return score * (0.8f - 0.1f * (float(p_path.length() - pos) / p_path.length()));
	}

	// Remaining results belong to the same class of results.
	return score * 0.69f;
}

void EditorCommandPalette::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			prev_rect = Rect2i(get_position(), get_size());
			was_showed = true;
		}
	} break;

	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorSettings::get_singleton()->check_changed_settings_in_group("shortcuts")) {
			break;
		}

		for (KeyValue<String, Command>& kv : commands) {
			Command& c = kv.value;
			if (c.shortcut.is_valid()) {
				c.shortcut_text = c.shortcut->get_as_text();
			}
		}
	} break;
	}
}

void EditorCommandPalette::open_popup()
{
	if (was_showed) {
		popup(prev_rect);
	}
	else {
		_update_command_search(String());
		popup_centered_clamped(Size2(600, 440) * EDSCALE, 0.8f);
	}

	command_search_box->clear();
	command_search_box->grab_focus();

	search_options->scroll_to_item(search_options->get_root());
}

void EditorCommandPalette::get_actions_list(List<String>* p_list) const
{
	for (const KeyValue<String, Command>& E : commands) {
		p_list->push_back(E.key);
	}
}

void EditorCommandPalette::remove_command(String p_key_name)
{
	ERR_FAIL_COND_MSG(!commands.has(p_key_name),
		"The Command '" + String(p_key_name) + "' doesn't exists. Unable to remove it.");

	commands.erase(p_key_name);
}

EditorCommandPalette* EditorCommandPalette::get_singleton()
{
	if (singleton == nullptr) {
		singleton = memnew(EditorCommandPalette);
	}
	return singleton;
}

Ref<Shortcut> ED_SHORTCUT_AND_COMMAND(
	const String& p_path, const String& p_name, Key p_keycode, String p_command_name)
{
	if (p_command_name.is_empty()) {
		p_command_name = p_name;
	}

	Ref<Shortcut> shortcut = ED_SHORTCUT(p_path, p_name, p_keycode);
	EditorCommandPalette::get_singleton()->add_shortcut_command(p_command_name, p_path, shortcut);
	return shortcut;
}

Ref<Shortcut> ED_SHORTCUT_ARRAY_AND_COMMAND(const String& p_path, const String& p_name,
	const PackedInt32Array& p_keycodes, String p_command_name)
{
	if (p_command_name.is_empty()) {
		p_command_name = p_name;
	}

	Ref<Shortcut> shortcut = ED_SHORTCUT_ARRAY(p_path, p_name, p_keycodes);
	EditorCommandPalette::get_singleton()->add_shortcut_command(p_command_name, p_path, shortcut);
	return shortcut;
}


