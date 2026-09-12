/**************************************************************************/
/*  rename_dialog.cpp                                                     */
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
#include "editor/editor_undo_redo_manager.h"
#include "editor/script/script_editor_plugin.h"
#include "modules/regex/regex.h"
#include "rename_dialog.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/control.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"
#include "scene/main/scene_tree.h"

String RenameDialog::_apply_rename(const Node* node, int count)
{
	String search = lne_search->get_text();
	String replace = lne_replace->get_text();
	String prefix = lne_prefix->get_text();
	String suffix = lne_suffix->get_text();
	String new_name = node->get_name();

	if (cbut_substitute->is_pressed()) {
		search = _substitute(search, node, count);
		replace = _substitute(replace, node, count);
		prefix = _substitute(prefix, node, count);
		suffix = _substitute(suffix, node, count);
	}

	if (cbut_regex->is_pressed()) {
		new_name = _regex(search, new_name, replace);
	}
	else {
		new_name = new_name.replace(search, replace);
	}

	new_name = prefix + new_name + suffix;

	if (cbut_process->is_pressed()) {
		new_name = _postprocess(new_name);
	}

	return new_name;
}

String RenameDialog::_regex(const String& pattern, const String& subject, const String& replacement)
{
	RegEx regex(pattern);

	return regex.sub(subject, replacement, true);
}

void RenameDialog::_iterate_scene(const Node* node, List<Node*>& selection, int* counter)
{
	if (!node) {
		return;
	}
	if (selection.find(node) != nullptr) {
		String new_name = _apply_rename(node, *counter);
		if (node->get_name() != new_name) {
			Pair<NodePath, String> rename_item;
			rename_item.first = node->get_path();
			rename_item.second = new_name;
			to_rename.push_back(rename_item);
		}

		*counter += spn_count_step->get_value();
	}

	int* cur_counter = counter;
	int level_counter = spn_count_start->get_value();

	if (chk_per_level_counter->is_pressed()) {
		cur_counter = &level_counter;
	}

	for (int i = 0; i < node->get_child_count(); ++i) {
		_iterate_scene(node->get_child(i), selection, cur_counter);
	}
}

bool RenameDialog::_is_main_field(LineEdit* line_edit)
{
	return line_edit && (line_edit == lne_search || line_edit == lne_replace ||
							line_edit == lne_prefix || line_edit == lne_suffix);
}


