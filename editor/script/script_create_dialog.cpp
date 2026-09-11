/**************************************************************************/
/*  script_create_dialog.cpp                                              */
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

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/theme/theme_db.h"
#include "script_create_dialog.h"

void ScriptCreateDialog::_path_hbox_sorted()
{
	if (is_visible()) {
		int filename_start_pos = file_path->get_text().rfind_char('/') + 1;
		int filename_end_pos = file_path->get_text().get_basename().length();

		if (!is_built_in) {
			file_path->select(filename_start_pos, filename_end_pos);
		}

		// First set cursor to the end of line to scroll LineEdit view
		// to the right and then set the actual cursor position.
		file_path->set_caret_column(file_path->get_text().length());
		file_path->set_caret_column(filename_start_pos);

		file_path->grab_focus();
	}
}

bool ScriptCreateDialog::_can_be_built_in() { return (supports_built_in && built_in_enabled); }

void ScriptCreateDialog::set_inheritance_base_type(const String& p_base) { base_type = p_base; }

bool ScriptCreateDialog::_validate_parent(const String& p_string)
{
	if (p_string.length() == 0) {
		return false;
	}

	if (can_inherit_from_file && p_string.is_quoted()) {
		String p = p_string.substr(1, p_string.length() - 2);
		if (_validate_path(p, true).is_empty()) {
			return true;
		}
	}

	return EditorNode::get_editor_data().is_type_recognized(p_string);
}

void ScriptCreateDialog::_parent_name_changed(const String&
 p_parent)
{
	is_parent_name_valid = _validate_parent(parent_name->get_text());
	validation_panel->update();
}

void ScriptCreateDialog::_load_exist()
{
	String path = file_path->get_text();
	Ref<Resource> p_script = ResourceLoader::load(path, "Script");
	if (p_script.is_null()) {
		alert->set_text(vformat(TTR("Error loading script from %s"), path));
		alert->popup_centered();
		return;
	}

	hide();
}

void ScriptCreateDialog::_built_in_pressed()
{
	if (built_in->is_pressed()) {
		is_built_in = true;
		is_new_script_created = true;
	}
	else {
		is_built_in = false;
		_path_changed(file_path->get_text());
	}
	validation_panel->update();
}

void ScriptCreateDialog::_file_selected(const String& p_file)
{
	String path = ProjectSettings::get_singleton()->localize_path(p_file);
	if (is_browsing_parent) {
		parent_name->set_text("\"" + path + "\"");
		_parent_name_changed(parent_name->get_text());
	}
	else {
		file_path->set_text(path);
		_path_changed(path);

		String filename = path.get_file().get_basename();
		int select_start = path.rfind(filename);
		file_path->select(select_start, select_start + filename.length());
		file_path->set_caret_column(select_start + filename.length());
		file_path->grab_focus();
	}
}

void ScriptCreateDialog::_create()
{
	parent_name->set_text(select_class->get_selected_type_name());
	_parent_name_changed(parent_name->get_text());
}

void ScriptCreateDialog::_browse_class_in_tree()
{
	select_class->set_base_type(base_type);
	select_class->popup_create(true);
	select_class->set_title(vformat(TTR("Inherit %s"), base_type));
	select_class->set_ok_button_text(TTR("Inherit"));
}

void ScriptCreateDialog::_path_changed(const String& p_path)
{
	if (is_built_in) {
		return;
	}

	is_new_script_created = true;

	path_error = _validate_path(p_path, false, &is_path_valid);
	if (!path_error.is_empty()) {
		validation_panel->update();
		return;
	}

	// Check if file exists.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	String p = ProjectSettings::get_singleton()->localize_path(p_path.strip_edges());
	if (da->file_exists(p)) {
		is_new_script_created = false;
	}
	validation_panel->update();
}


