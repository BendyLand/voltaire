/**************************************************************************/
/*  editor_asset_installer.cpp                                            */
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
#include "core/io/file_access.h"
#include "core/io/zip_io.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/progress_dialog.h"
#include "editor/themes/editor_scale.h"
#include "editor_asset_installer.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"

static bool _is_zip_entry_symlink(const unz_file_info& p_info)
{
	static constexpr uint32_t UNIX_FILE_TYPE_MASK = 0170000;
	static constexpr uint32_t UNIX_FILE_TYPE_SYMLINK = 0120000;

	uint32_t unix_mode = p_info.external_fa >> 16;
	return (unix_mode & UNIX_FILE_TYPE_MASK) == UNIX_FILE_TYPE_SYMLINK;
}

// Determine parent state based on non-conflict children, to avoid indeterminate state, and allow
// toggle dir with conflicts.
bool EditorAssetInstaller::_fix_conflicted_indeterminate_state(TreeItem* p_item, int p_column)
{
	if (p_item->get_child_count() == 0) {
		return false;
	}
	bool all_non_conflict_checked = true;
	bool all_non_conflict_unchecked = true;
	bool has_conflict_child = false;
	bool has_indeterminate_child = false;
	TreeItem* child_item = p_item->get_first_child();
	while (child_item) {
		has_conflict_child |= _fix_conflicted_indeterminate_state(child_item, p_column);
		bool child_checked = child_item->is_checked(p_column);
		bool child_indeterminate = child_item->is_indeterminate(p_column);
		all_non_conflict_checked &= (child_checked || child_indeterminate);
		all_non_conflict_unchecked &= !child_checked;
		has_indeterminate_child |= child_indeterminate;
		child_item = child_item->get_next();
	}
	if (has_indeterminate_child) {
		p_item->set_indeterminate(p_column, true);
	}
	else if (all_non_conflict_checked) {
		p_item->set_checked(p_column, true);
	}
	else if (all_non_conflict_unchecked) {
		p_item->set_checked(p_column, false);
	}
	if (has_conflict_child) {
		p_item->set_custom_color(
			p_column, get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
	}
	else {
		p_item->clear_custom_color(p_column);
	}
	return has_conflict_child;
}

bool EditorAssetInstaller::_is_item_checked(const String& p_source_path) const
{
	return file_item_map.has(p_source_path) &&
		   (file_item_map[p_source_path]->is_checked(0) ||
			   file_item_map[p_source_path]->is_indeterminate(0));
}

void EditorAssetInstaller::_update_file_mappings()
{
	mapped_files.clear();

	bool first = true;
	for (const String& E : asset_files) {
		if (first) {
			first = false;

			if (!toplevel_prefix.is_empty() && skip_toplevel) {
				continue;
			}
		}

		String path = E; // We're going to mutate it.
		if (!toplevel_prefix.is_empty() && skip_toplevel) {
			path = path.trim_prefix(toplevel_prefix);
		}

		mapped_files[E] = path;
	}
}

bool EditorAssetInstaller::_update_source_item_status(TreeItem* p_item, const String& p_path)
{
	ERR_FAIL_COND_V(!mapped_files.has(p_path), false);
	String target_path = target_dir_path.path_join(mapped_files[p_path]);

	bool target_exists = FileAccess::exists(target_path);
	if (target_exists) {
		p_item->set_custom_color(
			0, get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		p_item->set_tooltip_text(0, vformat(TTR("%s (already exists)"), target_path));
		p_item->set_checked(0, false);
	}
	else {
		p_item->clear_custom_color(0);
		p_item->set_tooltip_text(0, target_path);
		p_item->set_checked(0, true);
	}

	p_item->propagate_check(0);
	_fix_conflicted_indeterminate_state(p_item->get_tree()->get_root(), 0);
	return target_exists;
}

void EditorAssetInstaller::_open_target_dir_dialog()
{
	if (!target_dir_dialog) {
		target_dir_dialog = memnew(EditorFileDialog);
		target_dir_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
		target_dir_dialog->set_title(TTRC("Select Install Folder"));
		target_dir_dialog->set_current_dir(target_dir_path);
		add_child(target_dir_dialog);
	}

	target_dir_dialog->popup_file_dialog();
}

void EditorAssetInstaller::set_asset_name(const String& p_asset_name) { asset_name = p_asset_name; }

String EditorAssetInstaller::get_asset_name() const { return asset_name; }


