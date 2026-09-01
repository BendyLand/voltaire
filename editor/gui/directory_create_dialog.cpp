/**************************************************************************/
/*  directory_create_dialog.cpp                                           */
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

#include "core/io/dir_access.h"
#include "directory_create_dialog.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"

String DirectoryCreateDialog::_sanitize_input(const String& p_path) const
{
	String path = p_path.strip_edges();
	if (mode == MODE_DIRECTORY) {
		path = path.trim_suffix("/");
	}
	return path;
}

String DirectoryCreateDialog::_validate_path(const String& p_path) const
{
	if (p_path.is_empty()) {
		return TTR("Name cannot be empty.");
	}
	if (mode == MODE_FILE && p_path.ends_with("/")) {
		return TTR("File name can't end with /.");
	}

	const PackedStringArray splits = p_path.split("/");
	for (int i = 0; i < splits.size(); i++) {
		const String& part = splits[i];
		bool is_file = mode == MODE_FILE && i == splits.size() - 1;

		if (part.is_empty()) {
			if (is_file) {
				return TTR("File name cannot be empty.");
			}
			else {
				return TTR("Folder name cannot be empty.");
			}
		}
		if (part.contains_char('\\') || part.contains_char(':') || part.contains_char('*') ||
			part.contains_char('|') || part.contains_char('>') || part.ends_with(".") ||
			part.ends_with(" ")) {
			if (is_file) {
				return TTR("File name contains invalid characters.");
			}
			else {
				return TTR("Folder name contains invalid characters.");
			}
		}
		if (part[0] == '.') {
			if (is_file) {
				return TTR("File name begins with a dot.");
			}
			else {
				return TTR("Folder name begins with a dot.");
			}
		}
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->change_dir(base_dir);
	if (da->file_exists(p_path)) {
		return TTR("File with that name already exists.");
	}
	if (da->dir_exists(p_path)) {
		return TTR("Folder with that name already exists.");
	}

	return String();
}

void DirectoryCreateDialog::_on_dir_path_changed()
{
	const String path = _sanitize_input(dir_path->get_text());
	const String error = _validate_path(path);

	if (error.is_empty()) {
		if (path.contains_char('/')) {
			if (mode == MODE_DIRECTORY) {
				validation_panel->set_message(EditorValidationPanel::MSG_ID_DEFAULT,
					TTRC("Using slashes in folder names will create subfolders recursively."),
					EditorValidationPanel::MSG_OK);
			}
			else {
				validation_panel->set_message(EditorValidationPanel::MSG_ID_DEFAULT,
					TTRC("Using slashes in path will create the file in subfolder, creating new "
						 "subfolders if necessary."),
					EditorValidationPanel::MSG_OK);
			}
		}
		else if (mode == MODE_FILE) {
			validation_panel->set_message(EditorValidationPanel::MSG_ID_DEFAULT,
				TTRC("File name is valid."), EditorValidationPanel::MSG_OK);
		}
	}
	else {
		validation_panel->set_message(
			EditorValidationPanel::MSG_ID_DEFAULT, error, EditorValidationPanel::MSG_ERROR);
	}
}

void DirectoryCreateDialog::_post_popup()
{
	ConfirmationDialog::_post_popup();
	dir_path->grab_focus();
}


