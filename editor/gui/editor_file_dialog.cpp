/**************************************************************************/
/*  editor_file_dialog.cpp                                                */
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
#include "core/os/os.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/settings/editor_settings.h"
#include "editor_file_dialog.h"

bool EditorFileDialog::_should_hide_file(const String& p_file) const
{
	if (get_access() != FileDialog::ACCESS_RESOURCES) {
		return false;
	}
	const String full_path = dir_access->get_current_dir().path_join(p_file);
	return EditorFileSystem::_should_skip_directory(full_path);
}

Color EditorFileDialog::_get_folder_color(const String& p_path) const
{
	return FileSystemDock::get_dir_icon_color(p_path, FileDialog::_get_folder_color(p_path));
}

Vector2i EditorFileDialog::_get_list_mode_icon_size() const { return Vector2i(); }

void EditorFileDialog::_bind_methods() {}

void EditorFileDialog::_dir_contents_changed()
{
	if (!EditorFileSystem::get_singleton()) {
		return;
	}

	bool scan_required = false;
	switch (get_access()) {
	case FileDialog::ACCESS_RESOURCES: {
		scan_required = true;
	} break;
	case FileDialog::ACCESS_USERDATA: {
		// Directories within the project dir are unlikely to be accessed.
	} break;
	case FileDialog::ACCESS_FILESYSTEM: {
		// Directories within the project dir may still be accessed.
		const String localized_path =
			ProjectSettings::get_singleton()->localize_path(get_current_dir());
		scan_required = localized_path.is_resource_file();
	} break;
	}
	if (scan_required) {
		EditorFileSystem::get_singleton()->scan_changes();
	}
}


