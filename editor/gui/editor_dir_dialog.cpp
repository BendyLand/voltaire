/**************************************************************************/
/*  editor_dir_dialog.cpp                                                 */
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

#include "editor/docks/filesystem_dock.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/directory_create_dialog.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_dir_dialog.h"
#include "scene/gui/box_container.h"
#include "scene/gui/tree.h"
#include "servers/display/display_server.h"

void EditorDirDialog::config(const Vector<String>& p_paths)
{
	ERR_FAIL_COND(p_paths.is_empty());

	if (p_paths.size() == 1) {
		String path = p_paths[0];
		if (path.ends_with("/")) {
			path = path.substr(0, path.length() - 1);
		}
		// TRANSLATORS: %s is the file name that will be moved or duplicated.
		set_title(vformat(TTR("Move/Duplicate: %s"), path.get_file()));
	}
	else {
		// TRANSLATORS: %d is the number of files that will be moved or duplicated.
		set_title(vformat(TTRN("Move/Duplicate %d Item", "Move/Duplicate %d Items", p_paths.size()),
			p_paths.size()));
	}
	base_directory_path = p_paths[0].get_base_dir();
}

void EditorDirDialog::_item_activated()
{
	TreeItem* ti = tree->get_selected();
	ERR_FAIL_NULL(ti);
	if (ti->get_child_count() > 0) {
		ti->set_collapsed(!ti->is_collapsed());
	}
}

void EditorDirDialog::_make_dir_confirm(const String& p_path, const String& p_base_dir)
{
	FileSystemDock::get_singleton()->create_directory(p_path, p_base_dir);

	// Multiple level of directories can be created at once.
	String base_dir = p_path.get_base_dir();
	while (true) {
		opened_paths.insert(base_dir + "/");
		if (base_dir == "res://") {
			break;
		}
		base_dir = base_dir.get_base_dir();
	}

	new_dir_path = p_path + "/";
}


