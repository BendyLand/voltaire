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

void RenameDialog::_update_preview_int(int new_value) { _update_preview(); }

void RenameDialog::_update_preview(const String& new_text)
{
	if (lock_preview_update || preview_node == nullptr) {
		return;
	}

	has_errors = false;
	add_error_handler(&eh);

	String new_name = _apply_rename(preview_node, spn_count_start->get_value());

	if (!has_errors) {
		lbl_preview_title->set_text(TTR("Preview:"));
		lbl_preview->set_text(new_name);

		if (new_name == preview_node->get_name()) {
			// New name is identical to the old one. Don't color it as much to avoid distracting the
			// user.
			const Color accent_color = EditorNode::get_singleton()->get_editor_theme()->get_color(
				SNAME("accent_color"), EditorStringName(Editor));
			const Color text_color = EditorNode::get_singleton()->get_editor_theme()->get_color(
				SNAME("default_color"), SNAME("RichTextLabel"));
			lbl_preview->add_theme_color_override(
				SceneStringName(font_color), accent_color.lerp(text_color, 0.5));
		}
		else {
			lbl_preview->add_theme_color_override(SceneStringName(font_color),
				EditorNode::get_singleton()->get_editor_theme()->get_color(
					SNAME("success_color"), EditorStringName(Editor)));
		}
	}

	remove_error_handler(&eh);
}

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

void RenameDialog::_error_handler(void* p_self, const char* p_func, const char* p_file, int p_line,
	const char* p_error, const char* p_errorexp, bool p_editor_notify, ErrorHandlerType p_type)
{
	RenameDialog* self = (RenameDialog*)p_self;
	String source_file = String::utf8(p_file);

	// Only show first error that is related to "regex"
	if (self->has_errors || !source_file.contains("regex")) {
		return;
	}

	String err_str;
	if (p_errorexp && p_errorexp[0]) {
		err_str = String::utf8(p_errorexp);
	}
	else {
		err_str = String::utf8(p_error);
	}

	self->has_errors = true;
	self->lbl_preview_title->set_text(TTR("Regular Expression Error:"));
	self->lbl_preview->add_theme_color_override(
		SceneStringName(font_color), EditorNode::get_singleton()->get_editor_theme()->get_color(
										 SNAME("error_color"), EditorStringName(Editor)));
	self->lbl_preview->set_text(vformat(TTR("At character %s"), err_str));
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

void RenameDialog::reset()
{
	lock_preview_update = true;

	lne_prefix->clear();
	lne_suffix->clear();
	lne_search->clear();
	lne_replace->clear();

	cbut_substitute->set_pressed(false);
	cbut_regex->set_pressed(false);
	cbut_process->set_pressed(false);

	chk_per_level_counter->set_pressed(true);

	spn_count_start->set_value(1);
	spn_count_step->set_value(1);
	spn_count_padding->set_value(1);

	opt_style->select(0);
	opt_case->select(0);

	lock_preview_update = false;
	_update_preview();
}

bool RenameDialog::_is_main_field(LineEdit* line_edit)
{
	return line_edit && (line_edit == lne_search || line_edit == lne_replace ||
							line_edit == lne_prefix || line_edit == lne_suffix);
}

void RenameDialog::_features_toggled(bool pressed)
{
	if (pressed) {
		tabc_features->show();
	}
	else {
		tabc_features->hide();
	}

	// Adjust to minimum size in y
	Size2i new_size = get_size();
	new_size.y = 0;
	set_size(new_size);
}


