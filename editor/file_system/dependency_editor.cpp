/**************************************************************************/
/*  dependency_editor.cpp                                                 */
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
#include "core/os/os.h"
#include "dependency_editor.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tree.h"

static void _setup_search_file_dialog(
	EditorFileDialog* p_dialog, const String& p_file, const String& p_type)
{
	p_dialog->set_title(vformat(TTR("Search Replacement For: %s"), p_file.get_file()));

	// Set directory to closest existing directory.
	p_dialog->set_current_dir(p_file.get_base_dir());

	p_dialog->clear_filters();
	List<String> ext;
	ResourceLoader::get_recognized_extensions_for_type(p_type, &ext);
	for (const String& E : ext) {
		p_dialog->add_filter("*." + E);
	}
}

struct DependencyEditorSortByType
{
	bool operator()(const String& p_a, const String& p_b) const
	{
		const String a_type = p_a.contains("::") ? p_a.get_slice("::", 1) : "Resource";
		const String b_type = p_b.contains("::") ? p_b.get_slice("::", 1) : "Resource";
		const String a_path = p_a.contains("::") ? p_a.get_slice("::", 2) : p_a;
		const String b_path = p_b.contains("::") ? p_b.get_slice("::", 2) : p_b;
		return a_type == b_type ? a_path < b_path : a_type < b_type;
	}
};

struct DependencyEditorSortByPath
{
	bool operator()(const String& p_a, const String& p_b) const
	{
		const String a_path = p_a.contains("::") ? p_a.get_slice("::", 2) : p_a;
		const String b_path = p_b.contains("::") ? p_b.get_slice("::", 2) : p_b;
		return a_path < b_path;
	}
};

struct DependencyEditorSortByFile
{
	bool operator()(const String& p_a, const String& p_b) const
	{
		const String a_path = p_a.contains("::") ? p_a.get_slice("::", 2) : p_a;
		const String b_path = p_b.contains("::") ? p_b.get_slice("::", 2) : p_b;
		const String a_file = a_path.get_file();
		const String b_file = b_path.get_file();
		return a_file == b_file ? a_path < b_path : a_file < b_file;
	}
};

void DependencyEditor::_fix_and_find(
	EditorFileSystemDirectory* efsd, HashMap<String, HashMap<String, String>>& candidates)
{
	for (int i = 0; i < efsd->get_subdir_count(); i++) {
		_fix_and_find(efsd->get_subdir(i), candidates);
	}

	for (int i = 0; i < efsd->get_file_count(); i++) {
		String file = efsd->get_file(i);
		if (!candidates.has(file)) {
			continue;
		}

		String path = efsd->get_file_path(i);

		for (KeyValue<String, String>& E : candidates[file]) {
			if (E.value.is_empty()) {
				E.value = path;
				continue;
			}

			// must match the best, using subdirs
			String existing = E.value.replace_first("res://", "");
			String current = path.replace_first("res://", "");
			String lost = E.key.replace_first("res://", "");

			Vector<String> existingv = existing.split("/");
			existingv.reverse();
			Vector<String> currentv = current.split("/");
			currentv.reverse();
			Vector<String> lostv = lost.split("/");
			lostv.reverse();

			int existing_score = 0;
			int current_score = 0;

			for (int j = 0; j < lostv.size(); j++) {
				if (j < existingv.size() && lostv[j] == existingv[j]) {
					existing_score++;
				}
				if (j < currentv.size() && lostv[j] == currentv[j]) {
					current_score++;
				}
			}

			if (current_score > existing_score) {
				// if it was the same, could track distance to new path but..

				E.value = path; // replace by more accurate
			}
		}
	}
}

static String _get_resolved_dep_path(const String& p_dep)
{
	if (p_dep.get_slice_count("::") < 3) {
		return p_dep.get_slice("::", 0); // No UID, just return the path.
	}

	const String uid_text = p_dep.get_slice("::", 0);
	ResourceUID::ID uid = ResourceUID::get_singleton()->text_to_id(uid_text);

	// Dependency is in UID format, obtain proper path.
	if (uid != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(uid)) {
		return ResourceUID::get_singleton()->get_id_path(uid);
	}

	// UID fallback path.
	return p_dep.get_slice("::", 2);
}

static String _get_stored_dep_path(const String& p_dep)
{
	if (p_dep.get_slice_count("::") > 2) {
		return p_dep.get_slice("::", 2);
	}
	return p_dep.get_slice("::", 0);
}

List<String> DependencyEditor::_filter_deps(const List<String>& p_deps)
{
	const String filter_text = filter->get_text();

	if (filter_text.is_empty()) {
		return List<String>(p_deps);
	}

	List<String> filtered;

	for (const String& item : p_deps) {
		const String path = item.contains("::") ? item.get_slice("::", 2) : item;

		if (path.containsn(filter_text)) {
			filtered.push_back(item);
		}
	}

	return filtered;
}

void DependencyEditor::_sort_option_selected(int p_id)
{
	sort_by = (DependencyEditorSortBy)p_id;
	_update_menu_sort();
	_update_list();
}

void DependencyEditor::_update_menu_sort() {}

void DependencyEditorOwners::_empty_clicked(const Vector2& p_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index != MouseButton::LEFT) {
		return;
	}

	owners->deselect_all();
}

void DependencyEditorOwners::_file_option(int p_option)
{
	switch (p_option) {
	case FILE_MENU_OPEN: {
		PackedInt32Array selected_items = owners->get_selected_items();
		for (int i = 0; i < selected_items.size(); i++) {
			int item_idx = selected_items[i];
			if (item_idx < 0 || item_idx >= owners->get_item_count()) {
				break;
			}
			_select_file(item_idx);
		}
	} break;
	}
}

void DependencyRemoveDialog::_find_files_in_removed_folder(
	EditorFileSystemDirectory* efsd, const String& p_folder)
{
	if (!efsd) {
		return;
	}

	for (int i = 0; i < efsd->get_subdir_count(); ++i) {
		_find_files_in_removed_folder(efsd->get_subdir(i), p_folder);
	}
	for (int i = 0; i < efsd->get_file_count(); i++) {
		String file = efsd->get_file_path(i);
		ERR_FAIL_COND(all_remove_files.has(file)); // We are deleting a directory which is contained
												   // in a directory we are deleting...
		all_remove_files[file] =
			p_folder; // Point the file to the ancestor directory we are deleting so we know what to
					  // parent it under in the tree.
	}
}

void DependencyRemoveDialog::_find_all_removed_dependencies(
	EditorFileSystemDirectory* efsd, Vector<RemovedDependency>& p_removed)
{
	if (!efsd) {
		return;
	}

	for (int i = 0; i < efsd->get_subdir_count(); i++) {
		_find_all_removed_dependencies(efsd->get_subdir(i), p_removed);
	}

	for (int i = 0; i < efsd->get_file_count(); i++) {
		const String path = efsd->get_file_path(i);

		// It doesn't matter if a file we are about to delete will have some of its dependencies
		// removed too
		if (all_remove_files.has(path)) {
			continue;
		}

		Vector<String> all_deps = efsd->get_file_deps(i);
		for (int j = 0; j < all_deps.size(); ++j) {
			if (all_remove_files.has(all_deps[j])) {
				RemovedDependency dep;
				dep.file = path;
				dep.file_type = efsd->get_file_type(i);
				dep.dependency = all_deps[j];
				dep.dependency_folder = all_remove_files[all_deps[j]];
				p_removed.push_back(dep);
			}
		}
	}
}

void DependencyRemoveDialog::_show_files_to_delete_list()
{
	files_to_delete_list->clear();

	for (const String& s : dirs_to_delete) {
		String t = s.trim_prefix("res://");
		files_to_delete_list->add_item(t, Ref<Texture2D>(), false);
	}

	for (const String& s : files_to_delete) {
		String t = s.trim_prefix("res://");
		files_to_delete_list->add_item(t, Ref<Texture2D>(), false);
	}
}

enum
{
	BUTTON_ID_SEARCH,
	BUTTON_ID_OPEN_DEPS_EDITOR,
};

void OrphanResourcesDialog::refresh()
{
	HashMap<String, int> refs;
	_fill_owners(EditorFileSystem::get_singleton()->get_filesystem(), refs, nullptr);
	files->clear();
	TreeItem* root = files->create_item();
	_fill_owners(EditorFileSystem::get_singleton()->get_filesystem(), refs, root);
}

void OrphanResourcesDialog::show()
{
	refresh();
	popup_centered_ratio(0.4);
}


