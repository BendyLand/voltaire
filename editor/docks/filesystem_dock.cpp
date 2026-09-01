/**************************************************************************/
/*  filesystem_dock.cpp                                                   */
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
#include "core/input/input.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/templates/list.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/directory_create_dialog.h"
#include "editor/gui/editor_dir_dialog.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/editor_resource_tooltip_plugins.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/scene/scene_create_dialog.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/shader/shader_create_dialog.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "filesystem_dock.h"
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/progress_bar.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

void FileSystemList::_line_editor_submit(const String& p_text)
{
	if (popup_edit_committed) {
		return; // Already processed by _text_editor_popup_modal_close
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	popup_edit_committed = true; // End edit popup processing.
	popup_editor->hide();

	queue_redraw();
}

bool FileSystemList::edit_selected()
{
	ERR_FAIL_COND_V_MSG(!is_anything_selected(), false, "No item selected.");
	int s = get_current();
	ERR_FAIL_COND_V_MSG(s < 0, false, "No current item selected.");
	ensure_current_is_visible();

	Rect2 rect;
	Rect2 popup_rect;
	Vector2 ofs;

	Vector2 icon_size = get_fixed_icon_size() * get_icon_scale();

	// Handles the different icon modes (TOP/LEFT).
	switch (get_icon_mode()) {
	case ItemList::ICON_MODE_LEFT:
		rect = get_item_rect(s, true);
		if (get_v_scroll_bar()->is_visible()) {
			rect.position.y -= get_v_scroll_bar()->get_value();
		}
		if (get_h_scroll_bar()->is_visible()) {
			rect.position.x -= get_h_scroll_bar()->get_value();
		}
		ofs = Vector2(0,
			Math::floor(
				(MAX(line_editor->get_minimum_size().height, rect.size.height) - rect.size.height) /
				2));
		popup_rect.position = rect.position - ofs;
		popup_rect.size = rect.size;

		// Adjust for icon position and size.
		popup_rect.size.x -= MAX(theme_cache.h_separation, 0) / 2 + icon_size.x;
		popup_rect.position.x += MAX(theme_cache.h_separation, 0) / 2 + icon_size.x;
		break;
	case ItemList::ICON_MODE_TOP:
		rect = get_item_rect(s, false);
		if (get_v_scroll_bar()->is_visible()) {
			rect.position.y -= get_v_scroll_bar()->get_value();
		}
		if (get_h_scroll_bar()->is_visible()) {
			rect.position.x -= get_h_scroll_bar()->get_value();
		}
		popup_rect.position = rect.position;
		popup_rect.size = rect.size;

		// Adjust for icon position and size.
		popup_rect.size.y -=
			MAX(theme_cache.v_separation, 0) / 2 + theme_cache.icon_margin + icon_size.y;
		popup_rect.position.y +=
			MAX(theme_cache.v_separation, 0) / 2 + theme_cache.icon_margin + icon_size.y;
		break;
	}
	if (is_layout_rtl()) {
		popup_rect.position.x = get_size().width - popup_rect.position.x - popup_rect.size.x;
	}
	popup_rect.position += get_screen_position();

	popup_editor->set_position(popup_rect.position);
	popup_editor->set_size(popup_rect.size);

	String name = get_item_text(s);
	line_editor->set_text(name);
	line_editor->select(0, name.rfind_char('.'));

	popup_edit_committed = false; // Start edit popup processing.
	popup_editor->popup();
	popup_editor->child_controls_changed();
	line_editor->grab_focus();
	return true;
}

String FileSystemList::get_edit_text() { return line_editor->get_text(); }

void FileSystemList::_text_editor_popup_modal_close()
{
	if (popup_edit_committed) {
		return; // Already processed by _text_editor_popup_modal_close
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	_line_editor_submit(line_editor->get_text());
}

void FileSystemList::_bind_methods() {}

FileSystemList::FileSystemList()
{
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);

	popup_editor = memnew(Popup);
	add_child(popup_editor);

	popup_editor_vb = memnew(VBoxContainer);
	popup_editor_vb->add_theme_constant_override("separation", 0);
	popup_editor_vb->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	popup_editor->add_child(popup_editor_vb);

	line_editor = memnew(LineEdit);
	line_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	popup_editor_vb->add_child(line_editor);
}

Ref<Texture2D> FileSystemDock::_get_tree_item_icon(
	bool p_is_valid, const String& p_file_type, const String& p_icon_path)
{
	if (!p_icon_path.is_empty()) {
		Ref<Texture2D> icon = ResourceLoader::load(p_icon_path);
		if (icon.is_valid()) {
			return icon;
		}
	}

	if (!p_is_valid) {
		return get_editor_theme_icon(SNAME("ImportFail"));
	}
	else if (has_theme_icon(p_file_type, EditorStringName(EditorIcons))) {
		return get_editor_theme_icon(p_file_type);
	}
	else {
		return get_editor_theme_icon(SNAME("File"));
	}
}

void FileSystemDock::_create_tree(TreeItem* p_parent, EditorFileSystemDirectory* p_dir,
	const Vector<String>& p_uncollapsed_paths, const Vector<String>& p_selected_paths)
{
	// Create a tree item for the subdirectory.
	TreeItem* subdirectory_item = tree->create_item(p_parent);
	String dname = p_dir->get_name();
	String lpath = p_dir->get_path();

	if (dname.is_empty()) {
		dname = "res://";
		resources_item = subdirectory_item;
	}

	// Set custom folder color (if applicable).
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);

	TreeItem* parent = subdirectory_item->get_parent();
	if (parent) {
		Color parent_bg_color = parent->get_custom_bg_color(0);
		if (parent_bg_color != Color()) {
			subdirectory_item->set_icon_modulate(0, parent->get_icon_modulate(0));
		}
		else {
			subdirectory_item->set_icon_modulate(
				0, get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog")));
		}
	}

	subdirectory_item->set_text(0, dname);
	subdirectory_item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);
	subdirectory_item->set_icon(0, get_editor_theme_icon(SNAME("Folder")));
	if (da->is_link(lpath)) {
		subdirectory_item->set_icon_overlay(0, get_editor_theme_icon(SNAME("LinkOverlay")));
		subdirectory_item->set_tooltip_text(0, vformat(TTR("Link to: %s"), da->read_link(lpath)));
	}
	subdirectory_item->set_selectable(0, true);
	folder_map[lpath] = subdirectory_item;

	if (current_path == lpath || p_selected_paths.has(lpath) ||
		((display_mode != DISPLAY_MODE_TREE_ONLY) && (current_path.get_base_dir() == lpath))) {
		subdirectory_item->select(0, current_path == lpath);
	}

	subdirectory_item->set_collapsed(!p_uncollapsed_paths.has(lpath));

	// Create items for all subdirectories.
	bool reversed = file_sort == FileSortOption::FILE_SORT_NAME_REVERSE;
	for (int i = reversed ? p_dir->get_subdir_count() - 1 : 0;
		 reversed ? i >= 0 : i < p_dir->get_subdir_count(); reversed ? i-- : i++) {
		_create_tree(
			subdirectory_item, p_dir->get_subdir(i), p_uncollapsed_paths, p_selected_paths);
	}

	// Create all items for the files in the subdirectory.
	if (display_mode == DISPLAY_MODE_TREE_ONLY) {
		// Build the list of the files to display.
		List<FileInfo> file_list;
		for (int i = 0; i < p_dir->get_file_count(); i++) {
			String file_type = p_dir->get_file_type(i);
			if (_is_file_type_disabled_by_feature_profile(file_type)) {
				// If type is disabled, file won't be displayed.
				continue;
			}

			FileInfo file_info;
			file_info.name = p_dir->get_file(i);
			file_info.type = p_dir->get_file_type(i);
			file_info.icon_path = p_dir->get_file_icon_path(i);
			file_info.import_broken = !p_dir->get_file_import_is_valid(i);
			file_info.modified_time = p_dir->get_file_modified_time(i);

			file_list.push_back(file_info);
		}

		// Sort the file list if needed.
		sort_file_info_list(file_list, file_sort);

		// Build the tree.
		const int icon_size =
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

		for (const FileInfo& file_info : file_list) {
			TreeItem* file_item = tree->create_item(subdirectory_item);
			const String file_metadata = lpath.path_join(file_info.name);
			file_item->set_text(0, file_info.name);
			file_item->set_structured_text_bidi_override(0, TextServer::STRUCTURED_TEXT_FILE);
			file_item->set_icon(0,
				_get_tree_item_icon(!file_info.import_broken, file_info.type, file_info.icon_path));
			if (da->is_link(file_metadata)) {
				file_item->set_icon_overlay(0, get_editor_theme_icon(SNAME("LinkOverlay")));
				// TRANSLATORS: This is a tooltip for a file that is a symbolic link to another
				// file.
				file_item->set_tooltip_text(
					0, vformat(TTR("Link to: %s"), da->read_link(file_metadata)));
			}
			file_item->set_icon_max_width(0, icon_size);
			Color parent_bg_color = subdirectory_item->get_custom_bg_color(0);
			if (parent_bg_color != Color()) {
				file_item->set_custom_bg_color(0, parent_bg_color);
			}
			file_item->set_accept_children(false);
			if (current_path == file_metadata || p_selected_paths.has(file_metadata)) {
				file_item->select(0, current_path == file_metadata);
			}
			if (main_scene_path == file_metadata) {
				file_item->set_custom_color(
					0, get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
			}
		}
	}
	else if (lpath.get_base_dir() == current_path.get_base_dir()) {
		subdirectory_item->select(0);
	}
}

void FileSystemDock::_update_tree(const Vector<String>& p_uncollapsed_paths, bool p_uncollapse_root,
	bool p_scroll_to_selected, const Vector<String>& p_override_selection)
{
	const Vector<String> previous_selection =
		p_override_selection.is_empty() ? _tree_get_selected(false) : p_override_selection;

	// Recreate the tree.
	tree->clear();
	tree_update_id++;
	updating_tree = true;
	TreeItem* root = tree->create_item();
	root->set_accept_children(false);
	folder_map.clear();

	// Handles the favorites.
	favorites_item = tree->create_item(root);
	favorites_item->set_icon(0, get_editor_theme_icon(SNAME("Favorites")));
	favorites_item->set_text(0, TTRC("Favorites"));
	favorites_item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_ALWAYS);
	favorites_item->set_collapsed(!p_uncollapsed_paths.has("Favorites"));

	Vector<String> favorite_paths = EditorSettings::get_singleton()->get_favorites();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	bool fav_changed = false;
	for (int i = favorite_paths.size() - 1; i >= 0; i--) {
		if (da->dir_exists(favorite_paths[i]) || da->file_exists(favorite_paths[i])) {
			continue;
		}
		favorite_paths.remove_at(i);
		fav_changed = true;
	}
	if (fav_changed) {
		EditorSettings::get_singleton()->set_favorites(favorite_paths);
		// Setting favorites causes the tree to update, so continuing is redundant.
		return;
	}

	Ref<Texture2D> folder_icon = get_editor_theme_icon(SNAME("Folder"));
	const Color default_folder_color =
		get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog"));

	const int icon_size = get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));
	for (const String& favorite : favorite_paths) {
		if (!favorite.begins_with("res://")) {
			continue;
		}

		String text;
		Ref<Texture2D> icon;
		Color color;
		if (favorite == "res://") {
			text = "/";
			icon = folder_icon;
			color = default_folder_color;
		}
		else if (favorite.ends_with("/")) {
			text = favorite.substr(0, favorite.length() - 1).get_file();
			icon = folder_icon;
			color = FileSystemDock::get_dir_icon_color(favorite, default_folder_color);
		}
		else {
			text = favorite.get_file();
			int index;
			EditorFileSystemDirectory* dir =
				EditorFileSystem::get_singleton()->find_file(favorite, &index);
			if (dir) {
				icon = _get_tree_item_icon(dir->get_file_import_is_valid(index),
					dir->get_file_type(index), dir->get_file_icon_path(index));
			}
			else {
				icon = get_editor_theme_icon(SNAME("File"));
			}
			color = Color(1, 1, 1);
		}

		TreeItem* ti = tree->create_item(favorites_item);
		ti->set_text(0, text);
		ti->set_icon(0, icon);
		ti->set_icon_modulate(0, color);
		ti->set_icon_max_width(0, icon_size);
		ti->set_tooltip_text(0, favorite);
		ti->set_selectable(0, true);
		ti->set_accept_children(false);

		if (favorite == main_scene_path) {
			ti->set_custom_color(
				0, get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		}
	}

	Vector<String> uncollapsed_paths = p_uncollapsed_paths;
	if (p_uncollapse_root && !uncollapsed_paths.has("res://")) {
		uncollapsed_paths.push_back("res://");
	}

	// Create the remaining of the tree.
	_create_tree(root, EditorFileSystem::get_singleton()->get_filesystem(), uncollapsed_paths,
		previous_selection);
	if (!searched_tokens.is_empty()) {
		_update_filtered_items();
	}

	if (p_scroll_to_selected) {
		tree->ensure_cursor_is_visible();
	}

	updating_tree = false;
}

void FileSystemDock::set_display_mode(DisplayMode p_display_mode)
{
	display_mode = p_display_mode;
	_update_display_mode(false);
}

void FileSystemDock::_update_display_mode(bool p_force)
{
	if (!p_force && old_display_mode == display_mode) {
		return;
	}

	// Preserve the selection when switching modes.
	Vector<String> selected_paths;
	if (old_display_mode != display_mode && old_display_mode != DISPLAY_MODE_VSPLIT) {
		selected_paths = get_selected_paths();
	}

	switch (display_mode) {
	case DISPLAY_MODE_TREE_ONLY: {
		button_toggle_display_mode->set_button_icon(get_editor_theme_icon(SNAME("Panels1")));
		tree->show();
		tree->set_v_size_flags(SIZE_EXPAND_FILL);
		tree->set_theme_type_variation("");
		if (horizontal) {
			toolbar2_hbc->hide();

			tree->set_scroll_hint_mode(
				touches_bottom ? Tree::SCROLL_HINT_MODE_TOP : Tree::SCROLL_HINT_MODE_BOTH);
			tree_mc->set_theme_type_variation("NoBorderHorizontal");
		}
		else {
			toolbar2_hbc->show();

			tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_TOP);
			tree_mc->set_theme_type_variation("NoBorderBottomPanel");
		}
		button_file_list_display_mode->hide();

		_update_tree(get_uncollapsed_paths(), false, true, selected_paths);
		file_list_vb->hide();
	} break;

	case DISPLAY_MODE_HSPLIT:
	case DISPLAY_MODE_VSPLIT: {
		const bool is_vertical = display_mode == DISPLAY_MODE_VSPLIT;
		split_box->set_vertical(is_vertical);

		const int actual_offset = is_vertical ? split_box_offset_v : split_box_offset_h;
		split_box->set_split_offset(actual_offset);
		const StringName icon = is_vertical ? SNAME("Panels2") : SNAME("Panels2Alt");
		button_toggle_display_mode->set_button_icon(get_editor_theme_icon(icon));

		tree->show();
		tree->set_v_size_flags(SIZE_EXPAND_FILL);
		tree->set_theme_type_variation("TreeSecondary");
		tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_DISABLED);
		tree_mc->set_theme_type_variation("");

		files->set_theme_type_variation("ItemListSecondary");
		files->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_DISABLED);
		files_mc->set_theme_type_variation("");

		toolbar2_hbc->hide();
		button_file_list_display_mode->show();
		file_list_vb->show();

		if (old_display_mode == DISPLAY_MODE_TREE_ONLY) {
			// Properly allocate the selections between the views.
			Vector<String> selected_files;
			for (int i = 0; i < selected_paths.size(); i++) {
				const String& path = selected_paths[i];
				if (!path.ends_with("/")) {
					selected_files.append(path);
					selected_paths.remove_at(i);
					i--;
				}
			}

			_update_tree(get_uncollapsed_paths(), false, true, selected_paths);
			_update_file_list(!selected_files.is_empty(), selected_files);
		}
		else {
			tree->ensure_cursor_is_visible();

			// Always update to avoid broken icons, as previous updates
			// could have happened before the dock was inside the tree.
			update_all();
		}
	} break;
	}

	old_display_mode = display_mode;
}

Vector<String> FileSystemDock::get_selected_paths() const
{
	Vector<String> selected_tree = _tree_get_selected(false);
	// Use the old mode to help preserve selection between modes.
	// That variable also gets updated shortly after, so it shouldn't cause issues.
	if (old_display_mode == DISPLAY_MODE_TREE_ONLY) {
		return _tree_get_selected(false);
	}

	Vector<String> selected_files = _file_list_get_selected();
	for (const String& file : selected_files) {
		if (!selected_tree.has(file)) {
			selected_tree.append(file);
		}
	}

	return selected_tree;
}

String FileSystemDock::get_current_path() const { return current_path; }

String FileSystemDock::get_current_directory() const
{
	if (current_path.ends_with("/")) {
		return current_path;
	}
	else {
		return current_path.get_base_dir();
	}
}

void FileSystemDock::_set_current_path_line_edit_text(const String& p_path)
{
	if (p_path == "Favorites") {
		current_path_line_edit->set_text(TTR("Favorites"));
	}
	else {
		current_path_line_edit->set_text(current_path);
	}
}

void FileSystemDock::_navigate_to_path(
	const String& p_path, bool p_select_in_favorites, bool p_grab_focus)
{
	String target_path = p_path;
	bool is_directory = false;

	if (p_path.is_empty()) {
		target_path = "res://";
		is_directory = true;
	}
	else if (p_path != "Favorites") {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (da->dir_exists(p_path)) {
			is_directory = true;
			if (!p_path.ends_with("/")) {
				target_path += "/";
			}
		}
		else if (!da->file_exists(p_path)) {
			ERR_FAIL_MSG(vformat(
				"Cannot navigate to '%s' as it has not been found in the file system!", p_path));
		}
	}

	current_path = target_path;
	_set_current_path_line_edit_text(current_path);
	_push_to_history();

	String base_dir_path = target_path.get_base_dir();
	if (base_dir_path != "res://") {
		base_dir_path += "/";
	}

	TreeItem** directory_ptr = folder_map.getptr(base_dir_path);
	if (!directory_ptr) {
		return;
	}

	// Unfold all folders along the path.
	TreeItem* ti = *directory_ptr;
	while (ti) {
		ti->set_collapsed(false);
		ti = ti->get_parent();
	}

	// Select the file or directory in the tree.
	tree->deselect_all();
	if (display_mode == DISPLAY_MODE_TREE_ONLY) {
		// Either search for 'folder/' or '/file.ext'.
		const String file_name = is_directory ? target_path.trim_suffix("/").get_file() + "/"
											  : "/" + target_path.get_file();
		TreeItem* item = is_directory ? *directory_ptr : (*directory_ptr)->get_first_child();
		if (p_grab_focus) {
			tree->grab_focus(true);
		}
	}
	else {
		(*directory_ptr)->select(0);
		_update_file_list(false);
		if (p_grab_focus) {
			files->grab_focus(true);
		}
	}
	tree->ensure_cursor_is_visible();
}

bool FileSystemDock::_update_filtered_items(TreeItem* p_tree_item)
{
	TreeItem* item = p_tree_item;
	if (!item) {
		item = tree->get_root();
	}
	ERR_FAIL_NULL_V(item, false);

	bool keep_visible = false;
	for (TreeItem* child = item->get_first_child(); child; child = child->get_next()) {
		keep_visible = _update_filtered_items(child) || keep_visible;
	}

	if (searched_tokens.is_empty()) {
		item->set_visible(true);
		// Always uncollapse root (the hidden item above res:// and favorites).
		item->set_collapsed(item != tree->get_root());
		return true;
	}

	if (keep_visible) {
		item->set_collapsed(false);
	}
	else {
		// res:// and favorites are always visible.
		keep_visible = item == resources_item || item == favorites_item;
		keep_visible = keep_visible || _matches_all_search_tokens(item->get_text(0));
	}
	item->set_visible(keep_visible);
	return keep_visible;
}

void FileSystemDock::navigate_to_path(const String& p_path)
{
	file_list_search_box->clear();
	// Try to set the FileSystem dock visible.
	EditorDockManager::get_singleton()->focus_dock(this);
	_navigate_to_path(p_path, false, is_visible_in_tree());

	import_dock_needs_update = true;
	_update_import_dock();
}

void FileSystemDock::_file_list_thumbnail_done(const String& p_path,
	const Ref<Texture2D>& p_preview, const Ref<Texture2D>& p_small_preview, int p_index,
	const String& p_filename)
{
	if (p_preview.is_valid()) {
		if (p_index < files->get_item_count() && files->get_item_text(p_index) == p_filename) {
			Ref<Texture2D> thumbnail;

			if (file_list_display_mode == FILE_LIST_DISPLAY_LIST) {
				thumbnail = p_small_preview;
			}
			else {
				thumbnail = p_preview;
			}

			if (thumbnail.is_valid()) {
				files->set_item_icon(p_index, _apply_thumbnail_filter(thumbnail, p_path));
			}
		}
	}
}

Ref<Texture2D> FileSystemDock::_apply_thumbnail_filter(
	const Ref<Texture2D>& p_thumbnail, const String& p_file_path) const
{
	if (!p_file_path.is_empty()) {
		int index;
		EditorFileSystemDirectory* dir =
			EditorFileSystem::get_singleton()->find_file(p_file_path, &index);

		if (dir) {
			if (dir->get_file_import_is_valid(index)) {
				const StringName& file_type = dir->get_file_type(index);

				if (file_type == SNAME("CompressedTexture2D") || file_type == SNAME("Image")) {
					const String extension = p_file_path.get_extension();

					if (extension != "svg" && extension != "svgz") {
						Ref<CanvasTexture> thumbnail_wrapped;
						thumbnail_wrapped.instantiate();
						thumbnail_wrapped->set_diffuse_texture(p_thumbnail);
						thumbnail_wrapped->set_texture_filter(
							CanvasItem::TextureFilter::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS);
						return thumbnail_wrapped;
					}
				}
			}
		}
	}

	return p_thumbnail;
}

void FileSystemDock::_set_file_display(bool p_active)
{
	if (p_active) {
		file_list_display_mode = FILE_LIST_DISPLAY_LIST;
		button_file_list_display_mode->set_button_icon(
			get_editor_theme_icon(SNAME("FileThumbnail")));
		button_file_list_display_mode->set_tooltip_text(
			TTRC("View items as a grid of thumbnails."));
	}
	else {
		file_list_display_mode = FILE_LIST_DISPLAY_THUMBNAILS;
		button_file_list_display_mode->set_button_icon(get_editor_theme_icon(SNAME("FileList")));
		button_file_list_display_mode->set_tooltip_text(TTRC("View items as a list."));
	}

	_update_file_list(true);
}

bool FileSystemDock::_is_file_type_disabled_by_feature_profile(const StringName& p_class)
{
	Ref<EditorFeatureProfile> profile =
		EditorFeatureProfileManager::get_singleton()->get_current_profile();
	if (profile.is_null()) {
		return false;
	}

	StringName class_name = p_class;

	while (class_name != StringName()) {
		if (profile->is_class_disabled(class_name)) {
			return true;
		}
	}
	return false;
}

void FileSystemDock::_search(
	EditorFileSystemDirectory* p_path, List<FileInfo>* matches, int p_max_items)
{
	if (matches->size() > p_max_items) {
		return;
	}

	for (int i = 0; i < p_path->get_subdir_count(); i++) {
		_search(p_path->get_subdir(i), matches, p_max_items);
	}

	for (int i = 0; i < p_path->get_file_count(); i++) {
		String file = p_path->get_file(i);

		if (_matches_all_search_tokens(file)) {
			FileInfo file_info;
			file_info.name = file;
			file_info.type = p_path->get_file_type(i);
			file_info.path = p_path->get_file_path(i);
			file_info.import_broken = !p_path->get_file_import_is_valid(i);
			file_info.modified_time = p_path->get_file_modified_time(i);

			if (_is_file_type_disabled_by_feature_profile(file_info.type)) {
				// This type is disabled, will not appear here.
				continue;
			}

			matches->push_back(file_info);
			if (matches->size() > p_max_items) {
				return;
			}
		}
	}
}

void FileSystemDock::_update_file_list(
	bool p_keep_selection, const Vector<String>& p_override_selection)
{
	// Register the previously selected items.
	Vector<String> previous_selection;
	if (p_keep_selection) {
		previous_selection =
			p_override_selection.is_empty() ? _file_list_get_selected() : p_override_selection;
	}

	HashSet<int> valid_selection;

	files->clear();

	_set_current_path_line_edit_text(current_path);

	String directory = current_path;
	String file = "";

	int thumbnail_size = thumbnail_size_setting * EDSCALE;
	Ref<Texture2D> folder_thumbnail;
	Ref<Texture2D> file_thumbnail;
	Ref<Texture2D> file_thumbnail_broken;

	bool use_thumbnails = (file_list_display_mode == FILE_LIST_DISPLAY_THUMBNAILS);

	if (use_thumbnails) {
		// Thumbnails mode.
		files->set_max_columns(0);
		files->set_icon_mode(ItemList::ICON_MODE_TOP);
		files->set_fixed_column_width(thumbnail_size * 3 / 2);
		files->set_max_text_lines(2);
		files->set_fixed_icon_size(Size2(thumbnail_size, thumbnail_size));

		const int icon_size =
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));
		files->set_fixed_tag_icon_size(Size2(icon_size, icon_size));

		if (thumbnail_size < 64) {
			folder_thumbnail = get_editor_theme_icon(SNAME("FolderMediumThumb"));
			file_thumbnail = get_editor_theme_icon(SNAME("FileMediumThumb"));
			file_thumbnail_broken = get_editor_theme_icon(SNAME("FileDeadMediumThumb"));
		}
		else {
			folder_thumbnail = get_editor_theme_icon(SNAME("FolderBigThumb"));
			file_thumbnail = get_editor_theme_icon(SNAME("FileBigThumb"));
			file_thumbnail_broken = get_editor_theme_icon(SNAME("FileDeadBigThumb"));
		}
	}
	else {
		// No thumbnails.
		files->set_icon_mode(ItemList::ICON_MODE_LEFT);
		files->set_max_columns(1);
		files->set_max_text_lines(1);
		files->set_fixed_column_width(0);
		const int icon_size =
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));
		files->set_fixed_icon_size(Size2(icon_size, icon_size));
	}

	Ref<Texture2D> folder_icon =
		(use_thumbnails) ? folder_thumbnail : get_theme_icon(SNAME("folder"), SNAME("FileDialog"));
	const Color default_folder_color =
		get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog"));

	// Build the FileInfo list.
	List<FileInfo> file_list;
	if (current_path == "Favorites") {
		// Display the favorites.
		Vector<String> favorites_list = EditorSettings::get_singleton()->get_favorites();
		for (const String& favorite : favorites_list) {
			if (!favorite.begins_with("res://")) {
				continue;
			}
			String text;
			Ref<Texture2D> icon;
			if (favorite == "res://") {
				text = "/";
				icon = folder_icon;
				if (searched_tokens.is_empty() || _matches_all_search_tokens(text)) {
					files->add_item(text, icon, true);
				}
			}
			else if (favorite.ends_with("/")) {
				text = favorite.substr(0, favorite.length() - 1).get_file();
				icon = folder_icon;
				if (searched_tokens.is_empty() || _matches_all_search_tokens(text)) {
					files->add_item(text, icon, true);

					const Color folder_color =
						FileSystemDock::get_dir_icon_color(favorite, default_folder_color);
					if (!editor_is_dark_icon_and_font && folder_color != default_folder_color) {
						files->set_item_icon_modulate(-1, folder_color * ITEM_COLOR_SCALE);
					}
					else {
						files->set_item_icon_modulate(-1, folder_color);
					}
				}
			}
			else {
				int index;
				EditorFileSystemDirectory* efd =
					EditorFileSystem::get_singleton()->find_file(favorite, &index);

				FileInfo file_info;
				file_info.name = favorite.get_file();
				file_info.path = favorite;
				if (efd) {
					file_info.type = efd->get_file_type(index);
					file_info.icon_path = efd->get_file_icon_path(index);
					file_info.import_broken = !efd->get_file_import_is_valid(index);
					file_info.modified_time = efd->get_file_modified_time(index);
				}
				else {
					file_info.type = "";
					file_info.import_broken = true;
					file_info.modified_time = 0;
				}

				if (searched_tokens.is_empty() || _matches_all_search_tokens(file_info.name)) {
					file_list.push_back(file_info);
				}
			}
		}
	}
	else {
		if (!directory.begins_with("res://")) {
			directory = "res://" + directory;
		}
		// Get infos on the directory + file.
		if (directory.ends_with("/") && directory != "res://") {
			directory = directory.substr(0, directory.length() - 1);
		}
		EditorFileSystemDirectory* efd =
			EditorFileSystem::get_singleton()->get_filesystem_path(directory);
		if (!efd) {
			directory = current_path.get_base_dir();
			file = current_path.get_file();
			efd = EditorFileSystem::get_singleton()->get_filesystem_path(directory);
		}
		if (!efd) {
			return;
		}

		if (!searched_tokens.is_empty()) {
			// Display the search results.
			// Limit the number of results displayed to avoid an infinite loop.
			_search(EditorFileSystem::get_singleton()->get_filesystem(), &file_list, 10000);
		}
		else {
			if (display_mode == DISPLAY_MODE_TREE_ONLY || always_show_folders) {
				// Check for a folder color to inherit (if one is assigned).
				const Color inherited_folder_color =
					FileSystemDock::get_dir_icon_color(directory, default_folder_color);

				// Display folders in the list.
				if (directory != "res://") {
					files->add_item("..", folder_icon, true);

					String bd = directory.get_base_dir();
					if (bd != "res://" && !bd.ends_with("/")) {
						bd += "/";
					}

					files->set_item_selectable(-1, false);
					if (!editor_is_dark_icon_and_font &&
						inherited_folder_color != default_folder_color) {
						files->set_item_icon_modulate(
							-1, inherited_folder_color * ITEM_COLOR_SCALE);
					}
					else {
						files->set_item_icon_modulate(-1, inherited_folder_color);
					}
				}

				bool reversed = file_sort == FileSortOption::FILE_SORT_NAME_REVERSE;
				for (int i = reversed ? efd->get_subdir_count() - 1 : 0;
					 reversed ? i >= 0 : i < efd->get_subdir_count(); reversed ? i-- : i++) {
					String dname = efd->get_subdir(i)->get_name();
					String dpath = directory.path_join(dname) + "/";

					files->add_item(dname, folder_icon, true);

					if (previous_selection.has(dpath)) {
						files->select(files->get_item_count() - 1, false);
						valid_selection.insert(files->get_item_count() - 1);
					}
				}
			}

			// Display the folder content.
			for (int i = 0; i < efd->get_file_count(); i++) {
				FileInfo file_info;
				file_info.name = efd->get_file(i);
				file_info.path = directory.path_join(file_info.name);
				file_info.type = efd->get_file_type(i);
				file_info.icon_path = efd->get_file_icon_path(i);
				file_info.import_broken = !efd->get_file_import_is_valid(i);
				file_info.modified_time = efd->get_file_modified_time(i);

				file_list.push_back(file_info);
			}
		}
	}

	// Sort the file list if needed.
	sort_file_info_list(file_list, file_sort);

	// Fills the ItemList control node from the FileInfos.
	for (FileInfo& E : file_list) {
		FileInfo* finfo = &(E);
		String fname = finfo->name;
		String fpath = finfo->path;

		Ref<Texture2D> type_icon;
		Ref<Texture2D> big_icon;

		String tooltip = fpath;

		// Select the icons.
		type_icon = _get_tree_item_icon(!finfo->import_broken, finfo->type, finfo->icon_path);
		if (!finfo->import_broken) {
			big_icon = file_thumbnail;
		}
		else {
			big_icon = file_thumbnail_broken;
			tooltip +=
				"\n" + TTR("Status: Import of file failed. Please fix file and reimport manually.");
		}

		// Add the item to the ItemList.
		int item_index;
		if (use_thumbnails) {
			files->add_item(fname, big_icon, true);
			item_index = files->get_item_count() - 1;
			files->set_item_tag_icon(item_index, type_icon);

		}
		else {
			files->add_item(fname, type_icon, true);
			item_index = files->get_item_count() - 1;
		}

		if (fpath == main_scene_path) {
			files->set_item_custom_fg_color(
				item_index, get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		}

		// Select the items.
		if (previous_selection.has(fpath)) {
			files->select(item_index, false);
			if (current_path == fpath) {
				files->set_current(item_index);
			}
			valid_selection.insert(item_index);
		}

		if (!p_keep_selection && !file.is_empty() && fname == file) {
			files->select(item_index, true);
			files->ensure_current_is_visible();
		}

		// Tooltip.
		if (finfo->sources.size()) {
			for (int j = 0; j < finfo->sources.size(); j++) {
				tooltip += "\nSource: " + finfo->sources[j];
			}
		}
		files->set_item_tooltip(item_index, tooltip);
	}

	// If we have any selected items retained, one must be set as the current one.
	if (files->get_current() == -1 && !valid_selection.is_empty()) {
		files->set_current(*valid_selection.begin());
	}
}

HashSet<String> FileSystemDock::_get_valid_conversions_for_file_paths(const Vector<String>& p_paths)
{
	HashSet<String> all_valid_conversion_to_targets;
	for (const String& fpath : p_paths) {
		if (fpath.is_empty() || fpath == "res://" || !FileAccess::exists(fpath) ||
			FileAccess::exists(fpath + ".import")) {
			return HashSet<String>();
		}

		Vector<Ref<EditorResourceConversionPlugin>> conversions =
			EditorNode::get_singleton()->find_resource_conversion_plugin_for_type_name(
				EditorFileSystem::get_singleton()->get_file_type(fpath));

		if (conversions.is_empty()) {
			// This resource can't convert to anything, so return an empty list.
			return HashSet<String>();
		}

		// Get a list of all potential conversion-to targets.
		HashSet<String> current_valid_conversion_to_targets;
		if (all_valid_conversion_to_targets.is_empty()) {
			// If we have no existing valid conversions, this is the first one, so copy them
			// directly.
			all_valid_conversion_to_targets = current_valid_conversion_to_targets;
		}
		else {
			// Check existing conversion targets and remove any which are not in the current list.
			for (const String& S : all_valid_conversion_to_targets) {
				if (!current_valid_conversion_to_targets.has(S)) {
					all_valid_conversion_to_targets.erase(S);
				}
			}
			// We have no more remaining valid conversions, so break the loop.
			if (all_valid_conversion_to_targets.is_empty()) {
				break;
			}
		}
	}

	return all_valid_conversion_to_targets;
}

void FileSystemDock::_select_file(const String& p_path, bool p_select_in_favorites, bool p_navigate)
{
	String fpath = p_path;
	if (fpath.ends_with("/")) {
		// Ignore a directory.
	}
	else if (fpath != "Favorites") {
		if (FileAccess::exists(fpath + ".import")) {
			Ref<ConfigFile> config;
			config.instantiate();
		}

		String resource_type = ResourceLoader::get_resource_type(fpath);
		if (resource_type == "PackedScene" || resource_type == "AnimationLibrary") {
			bool is_imported = false;
			{
				List<String> importer_exts;
				ResourceImporterScene::get_scene_importer_extensions(&importer_exts);
				String extension = fpath.get_extension();
				for (const String& E : importer_exts) {
					if (extension.nocasecmp_to(E) == 0) {
						is_imported = true;
						break;
					}
				}
			}

			if (is_imported) {
				SceneImportSettingsDialog::get_singleton()->open_settings(p_path, resource_type);
			}
			else {
				EditorNode::get_singleton()->load_scene_or_resource(fpath);
			}
		}
		else if (ResourceLoader::is_imported(fpath)) {
			// If the importer has advanced settings, show them.
			int order;
			bool can_threads;
			String name;
			Error err =
				ResourceFormatImporter::get_singleton()->get_import_order_threads_and_importer(
					fpath, order, can_threads, name);
			bool used_advanced_settings = false;
			if (err == OK) {
				Ref<ResourceImporter> importer =
					ResourceFormatImporter::get_singleton()->get_importer_by_name(name);
				if (importer.is_valid() && importer->has_advanced_options()) {
					importer->show_advanced_options(fpath);
					used_advanced_settings = true;
				}
			}

			if (!used_advanced_settings) {
				EditorNode::get_singleton()->load_resource(fpath);
			}
		}
		else {
			EditorNode::get_singleton()->load_resource(fpath);
		}
	}
	if (p_navigate) {
		_navigate_to_path(fpath, p_select_in_favorites);
	}
}

void FileSystemDock::_fs_changed()
{
	button_hist_prev->set_disabled(history_pos == 0);
	button_hist_next->set_disabled(history_pos == history.size() - 1);
	scanning_vb->hide();
	split_box->show();

	update_all();

	if (!select_after_scan.is_empty()) {
		_navigate_to_path(select_after_scan);
		select_after_scan.clear();
		import_dock_needs_update = true;
		_update_import_dock();
	}

	set_process(false);
	if (had_focus) {
		had_focus->grab_focus();
		had_focus = nullptr;
	}
}

void FileSystemDock::_set_scanning_mode()
{
	button_hist_prev->set_disabled(true);
	button_hist_next->set_disabled(true);
	split_box->hide();
	scanning_vb->show();
	set_process(true);
	if (EditorFileSystem::get_singleton()->is_scanning()) {
		scanning_progress->set_value(
			EditorFileSystem::get_singleton()->get_scanning_progress() * 100);
	}
	else {
		scanning_progress->set_value(0);
	}
}

void FileSystemDock::_fw_history()
{
	if (history_pos < history.size() - 1) {
		history_pos++;
	}

	_update_history();
}

void FileSystemDock::_bw_history()
{
	if (history_pos > 0) {
		history_pos--;
	}

	_update_history();
}

void FileSystemDock::_update_history()
{
	current_path = history[history_pos];
	_set_current_path_line_edit_text(current_path);

	if (tree->is_visible()) {
		_update_tree(get_uncollapsed_paths());
		tree->grab_focus(true);
	}

	if (file_list_vb->is_visible()) {
		_update_file_list(false);
	}

	button_hist_prev->set_disabled(history_pos == 0);
	button_hist_next->set_disabled(history_pos == history.size() - 1);
}

void FileSystemDock::_push_to_history()
{
	if (history[history_pos] != current_path) {
		history.resize(history_pos + 1);
		history.push_back(current_path);
		history_pos++;

		if (history.size() > history_max_size) {
			history.remove_at(0);
			history_pos = history_max_size - 1;
		}
	}

	button_hist_prev->set_disabled(history_pos == 0);
	button_hist_next->set_disabled(history_pos == history.size() - 1);
}

void FileSystemDock::_get_all_items_in_dir(
	EditorFileSystemDirectory* p_efsd, Vector<String>& r_files, Vector<String>& r_folders) const
{
	if (p_efsd == nullptr) {
		return;
	}

	for (int i = 0; i < p_efsd->get_subdir_count(); i++) {
		r_folders.push_back(p_efsd->get_subdir(i)->get_path());
		_get_all_items_in_dir(p_efsd->get_subdir(i), r_files, r_folders);
	}
	for (int i = 0; i < p_efsd->get_file_count(); i++) {
		r_files.push_back(p_efsd->get_file_path(i));
	}
}

void FileSystemDock::_find_file_owners(EditorFileSystemDirectory* p_efsd,
	const HashSet<String>& p_renames, HashSet<String>& r_file_owners) const
{
	for (int i = 0; i < p_efsd->get_subdir_count(); i++) {
		_find_file_owners(p_efsd->get_subdir(i), p_renames, r_file_owners);
	}
	for (int i = 0; i < p_efsd->get_file_count(); i++) {
		Vector<String> deps = p_efsd->get_file_deps(i);
		for (int j = 0; j < deps.size(); j++) {
			if (p_renames.has(deps[j])) {
				r_file_owners.insert(p_efsd->get_file_path(i));
				break;
			}
		}
	}
}

void FileSystemDock::_try_move_item(const FileOrFolder& p_item, const String& p_new_path,
	HashMap<String, String>& p_file_renames, HashMap<String, String>& p_folder_renames)
{
	// Ensure folder paths end with "/".
	String old_path =
		(p_item.is_file || p_item.path.ends_with("/")) ? p_item.path : (p_item.path + "/");
	String new_path =
		(p_item.is_file || p_new_path.ends_with("/")) ? p_new_path : (p_new_path + "/");

	if (new_path == old_path) {
		return;
	}
	else if (old_path == "res://") {
		EditorNode::get_singleton()->add_io_error(TTR("Cannot move/rename resources root."));
		return;
	}
	else if (!p_item.is_file && new_path.begins_with(old_path)) {
		// This check doesn't erroneously catch renaming to a longer name as folder paths always end
		// with "/".
		EditorNode::get_singleton()->add_io_error(
			TTR("Cannot move a folder into itself.") + "\n" + old_path + "\n");
		return;
	}

	// Build a list of files which will have new paths as a result of this operation.
	Vector<String> file_changed_paths;
	Vector<String> folder_changed_paths;
	if (p_item.is_file) {
		file_changed_paths.push_back(old_path);
	}
	else {
		folder_changed_paths.push_back(old_path);
		_get_all_items_in_dir(EditorFileSystem::get_singleton()->get_filesystem_path(old_path),
			file_changed_paths, folder_changed_paths);
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	print_verbose("Moving " + old_path + " -> " + new_path);
	Error err = da->rename(old_path, new_path);
	if (err == OK) {
		// Move/Rename any corresponding import settings too.
		if (p_item.is_file && FileAccess::exists(old_path + ".import")) {
			err = da->rename(old_path + ".import", new_path + ".import");
			if (err != OK) {
				EditorNode::get_singleton()->add_io_error(
					TTR("Error moving:") + "\n" + old_path + ".import\n");
			}
		}

		if (p_item.is_file && FileAccess::exists(old_path + ".uid")) {
			err = da->rename(old_path + ".uid", new_path + ".uid");
			if (err != OK) {
				EditorNode::get_singleton()->add_io_error(
					TTR("Error moving:") + "\n" + old_path + ".uid\n");
			}
		}

		// Update scene if it is open.
		for (int i = 0; i < file_changed_paths.size(); ++i) {
			String new_item_path =
				p_item.is_file ? new_path : file_changed_paths[i].replace_first(old_path, new_path);
			if (ResourceLoader::get_resource_type(new_item_path) == "PackedScene" &&
				EditorNode::get_singleton()->is_scene_open(file_changed_paths[i])) {
				EditorData* ed = &EditorNode::get_editor_data();
				for (int j = 0; j < ed->get_edited_scene_count(); j++) {
					if (ed->get_scene_path(j) == file_changed_paths[i]) {
						ed->get_edited_scene_root(j)->set_scene_file_path(new_item_path);
						EditorNode::get_singleton()->save_editor_layout_delayed();
						break;
					}
				}
			}
		}

		// Only treat as a changed dependency if it was successfully moved.
		for (int i = 0; i < file_changed_paths.size(); ++i) {
			p_file_renames[file_changed_paths[i]] =
				file_changed_paths[i].replace_first(old_path, new_path);
			print_verbose("  Remap: " + file_changed_paths[i] + " -> " +
						  p_file_renames[file_changed_paths[i]]);
		}
		for (int i = 0; i < folder_changed_paths.size(); ++i) {
			p_folder_renames[folder_changed_paths[i]] =
				folder_changed_paths[i].replace_first(old_path, new_path);
		}
	}
	else {
		EditorNode::get_singleton()->add_io_error(TTR("Error moving:") + "\n" + old_path + "\n");
	}
}

void FileSystemDock::_try_duplicate_item(const FileOrFolder& p_item, const String& p_new_path) const
{
	// Ensure folder paths end with "/".
	String old_path =
		(p_item.is_file || p_item.path.ends_with("/")) ? p_item.path : (p_item.path + "/");
	String new_path =
		(p_item.is_file || p_new_path.ends_with("/")) ? p_new_path : (p_new_path + "/");

	if (new_path == old_path) {
		return;
	}
	else if (old_path == "res://") {
		EditorNode::get_singleton()->add_io_error(TTR("Cannot move/rename resources root."));
		return;
	}
	else if (!p_item.is_file && new_path.begins_with(old_path)) {
		// This check doesn't erroneously catch renaming to a longer name as folder paths always end
		// with "/".
		EditorNode::get_singleton()->add_io_error(
			TTR("Cannot move a folder into itself.") + "\n" + old_path + "\n");
		return;
	}

	if (p_item.is_file) {
		print_verbose("Duplicating " + old_path + " -> " + new_path);

		// Create the directory structure.
		EditorFileSystem::get_singleton()->make_dir_recursive(p_new_path.get_base_dir());

		Error err = EditorFileSystem::get_singleton()->copy_file(old_path, new_path);
		if (err != OK) {
			EditorNode::get_singleton()->add_io_error(TTR("Error duplicating:") + "\n" + old_path +
													  U" → " + new_path + ": " +
													  TTR(error_names[err]) + "\n");
		}
	}
	else {
		Error err = EditorFileSystem::get_singleton()->copy_directory(old_path, new_path);
		if (err != OK) {
			EditorNode::get_singleton()->add_io_error(TTR("Error duplicating directory:") + "\n" +
													  old_path + U" → " + new_path + ": " +
													  TTR(error_names[err]) + "\n");
		}
	}
}

void FileSystemDock::_update_resource_paths_after_move(
	const HashMap<String, String>& p_renames) const
{
	// Rename all resources loaded, be it subresources or actual resources.
	List<Ref<Resource>> cached;
	ResourceCache::get_cached_resources(&cached);

	for (Ref<Resource>& r : cached) {
		String base_path = r->get_path();
		String extra_path;
		int sep_pos = r->get_path().find("::");
		if (sep_pos >= 0) {
			extra_path = base_path.substr(sep_pos);
			base_path = base_path.substr(0, sep_pos);
		}

		if (p_renames.has(base_path)) {
			base_path = p_renames[base_path];
			r->set_path(base_path + extra_path);
		}
	}

	Vector<String> files_to_update;
	for (const KeyValue<String, String>& E : p_renames) {
		if (!files_to_update.has(E.key)) {
			files_to_update.push_back(E.key);
		}
		if (!files_to_update.has(E.value)) {
			files_to_update.push_back(E.value);
		}
	}
	print_verbose("FileSystem: updating file infos.");
	EditorFileSystem::get_singleton()->update_files(files_to_update);
}

void FileSystemDock::_update_dependencies_after_move(
	const HashMap<String, String>& p_renames, const HashSet<String>& p_file_owners) const
{
	// The following code assumes that the following holds:
	// 1) EditorFileSystem contains the old paths/folder structure from before the rename/move.
	// 2) ResourceLoader can use the new paths without needing to call rescan.

	// The currently edited scene should be reloaded first, so get it's path (GH-82652).
	const String& edited_scene_path = EditorNode::get_editor_data().get_scene_path(
		EditorNode::get_editor_data().get_edited_scene());
	List<String> scenes_to_reload;
	for (const String& E : p_file_owners) {
		// Because we haven't called a rescan yet the found remap might still be an old path itself.
		const HashMap<String, String>::ConstIterator I = p_renames.find(E);
		const String file = I ? I->value : E;
		print_verbose("Remapping dependencies for: " + file);
		const Error err = ResourceLoader::rename_dependencies(file, p_renames);
		if (err == OK) {
			if (ResourceLoader::get_resource_type(file) == "PackedScene" &&
				EditorNode::get_editor_data().get_edited_scene_from_path(file) != -1) {
				if (file == edited_scene_path) {
					scenes_to_reload.push_front(file);
				}
				else {
					scenes_to_reload.push_back(file);
				}
			}
		}
		else {
			EditorNode::get_singleton()->add_io_error(
				TTR("Unable to update dependencies for:") + "\n" + E + "\n");
		}
	}

	for (const String& E : scenes_to_reload) {
		EditorNode::get_singleton()->reload_scene(E);
	}
}

String FileSystemDock::_get_unique_name(const FileOrFolder& p_entry, const String& p_at_path)
{
	String new_path;
	String new_path_base;

	if (p_entry.is_file) {
		new_path = p_at_path.path_join(p_entry.path.get_file());
		new_path_base = new_path.get_basename() + " (%d)." + new_path.get_extension();
	}
	else {
		PackedStringArray path_split = p_entry.path.split("/");
		new_path = p_at_path.path_join(path_split[path_split.size() - 2]);
		new_path_base = new_path + " (%d)";
	}

	int exist_counter = 1;
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	while (da->file_exists(new_path) || da->dir_exists(new_path)) {
		exist_counter++;
		new_path = vformat(new_path_base, exist_counter);
	}

	return new_path;
}

void FileSystemDock::_update_favorites_after_move(const HashMap<String, String>& p_files_renames,
	const HashMap<String, String>& p_folders_renames) const
{
	Vector<String> favorite_files = EditorSettings::get_singleton()->get_favorites();
	Vector<String> new_favorite_files;
	for (const String& old_path : favorite_files) {
		if (p_folders_renames.has(old_path)) {
			new_favorite_files.push_back(p_folders_renames[old_path]);
		}
		else if (p_files_renames.has(old_path)) {
			new_favorite_files.push_back(p_files_renames[old_path]);
		}
		else {
			new_favorite_files.push_back(old_path);
		}
	}
	EditorSettings::get_singleton()->set_favorites(new_favorite_files);

	HashMap<String, PackedStringArray> favorite_properties =
		EditorSettings::get_singleton()->get_favorite_properties();
	for (const KeyValue<String, String>& KV : p_files_renames) {
		if (favorite_properties.has(KV.key)) {
			favorite_properties.replace_key(KV.key, KV.value);
		}
	}
	EditorSettings::get_singleton()->set_favorite_properties(favorite_properties);
}

void FileSystemDock::_make_scene_confirm()
{
	const String scene_path = make_scene_dialog->get_scene_path();

	int idx = EditorNode::get_singleton()->new_scene();
	EditorNode::get_editor_data().set_scene_path(idx, scene_path);
	EditorNode::get_singleton()->set_edited_scene(make_scene_dialog->create_scene_root());
	EditorNode::get_singleton()->save_scene_if_open(scene_path);
}

void FileSystemDock::_rename_operation_confirm()
{
	String new_name;
	TreeItem* ti = tree->get_edited();
	int col_index = tree->get_edited_column();

	if (ti) {
		new_name = ti->get_text(col_index).strip_edges();
	}
	else {
		new_name = files->get_edit_text().strip_edges();
	}
	String old_name =
		to_rename.is_file ? to_rename.path.get_file() : to_rename.path.left(-1).get_file();

	bool rename_error = false;
	if (new_name.length() == 0) {
		EditorNode::get_singleton()->show_warning(TTRC("No name provided."));
		rename_error = true;
	}
	else if (new_name.contains_char('/') || new_name.contains_char('\\') ||
			   new_name.contains_char(':')) {
		EditorNode::get_singleton()->show_warning(TTRC("Name contains invalid characters."));
		rename_error = true;
	}
	else if (new_name[0] == '.') {
		EditorNode::get_singleton()->show_warning(
			TTRC("This filename begins with a dot rendering the file invisible to the editor.\nIf "
				 "you want to rename it anyway, use your operating system's file manager."));
		rename_error = true;
	}
	else if (to_rename.is_file && to_rename.path.get_extension() != new_name.get_extension()) {
		if (!EditorFileSystem::get_singleton()->get_valid_extensions().find(
				new_name.get_extension())) {
			unrecognized_ext_dialog->popup_centered_clamped();
			rename_error = true;
		}
	}

	// Restore original name.
	if (rename_error) {
		if (ti) {
			ti->set_text(col_index, old_name);
		}
		return;
	}

	String old_path = to_rename.path.ends_with("/") ? to_rename.path.left(-1) : to_rename.path;
	String new_path = old_path.get_base_dir().path_join(new_name);
	if (old_path == new_path) {
		return;
	}

	if (EditorFileSystem::get_singleton()->is_group_file(old_path)) {
		EditorFileSystem::get_singleton()->move_group_file(old_path, new_path);
	}

	// Present a more user friendly warning for name conflict.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);

	bool new_exist = (da->file_exists(new_path) || da->dir_exists(new_path));
	if (!da->is_case_sensitive(new_path.get_base_dir())) {
		new_exist = new_exist && (new_path.to_lower() != old_path.to_lower());
	}
	if (new_exist) {
		EditorNode::get_singleton()->show_warning(
			TTRC("A file or folder with this name already exists."));
		if (ti) {
			ti->set_text(col_index, old_name);
		}
		return;
	}

	HashSet<String> file_owners; // The files that use these moved/renamed resource files.
	_before_move(file_owners);

	HashMap<String, String> file_renames;
	HashMap<String, String> folder_renames;
	_try_move_item(to_rename, new_path, file_renames, folder_renames);

	int current_tab = EditorSceneTabs::get_singleton()->get_current_tab();
	_update_resource_paths_after_move(file_renames);
	_update_dependencies_after_move(file_renames, file_owners);
	_update_project_settings_after_move(file_renames, folder_renames);
	_update_favorites_after_move(file_renames, folder_renames);

	EditorSceneTabs::get_singleton()->set_current_tab(current_tab);

	if (ti) {
		current_path = new_path;
		current_path_line_edit->set_text(current_path);
	}

	print_verbose("FileSystem: calling rescan.");
	_rescan();
}

void FileSystemDock::_duplicate_operation_confirm(const String& p_path)
{
	const String base_dir = p_path.trim_suffix("/").get_base_dir();
	if (!DirAccess::dir_exists_absolute(base_dir)) {
		Error err = EditorFileSystem::get_singleton()->make_dir_recursive(base_dir);
		if (err != OK) {
			EditorNode::get_singleton()->show_warning(
				vformat(TTR("Could not create base directory: %s"), TTR(error_names[err])));
			return;
		}
	}
	_try_duplicate_item(to_duplicate, p_path);
}

void FileSystemDock::_overwrite_dialog_action(bool p_overwrite)
{
	overwrite_dialog->hide();
	_move_operation_confirm(
		to_move_path, to_move_or_copy, p_overwrite ? OVERWRITE_REPLACE : OVERWRITE_RENAME);
}

void FileSystemDock::_convert_dialog_action()
{
	Vector<Ref<Resource>> selected_resources;
	for (const String& S : to_convert) {
		Ref<Resource> res = ResourceLoader::load(S);
		ERR_FAIL_COND(res.is_null());
		selected_resources.push_back(res);
	}

	Vector<Ref<Resource>> converted_resources;
	HashSet<Ref<Resource>> resources_to_erase_history_for;
	for (Ref<Resource> res : selected_resources) {
		Vector<Ref<EditorResourceConversionPlugin>> conversions =
			EditorNode::get_singleton()->find_resource_conversion_plugin_for_resource(res);
		for ([[maybe_unused]] const Ref<EditorResourceConversionPlugin>& conversion : conversions) {
			int conversion_id = 0;
			for ([[maybe_unused]] const String& target : cached_valid_conversion_targets) {
				if (conversion_id == selected_conversion_id) {
					ERR_FAIL_COND(res.is_null());
					resources_to_erase_history_for.insert(res);
					break;
				}
				conversion_id++;
			}
		}
	}

	// Updates all the resources existing as node properties.
	EditorNode::get_singleton()->replace_resources_in_scenes(
		selected_resources, converted_resources);

	// Overwrite the old resources.
	for (int i = 0; i < converted_resources.size(); i++) {
		Ref<Resource> original_resource = selected_resources.get(i);
		Ref<Resource> new_resource = converted_resources.get(i);

		// Overwrite the path.
		new_resource->set_path(original_resource->get_path(), true);

		ResourceSaver::save(new_resource.ptr());
	}
}

Vector<String> FileSystemDock::_check_existing()
{
	Vector<String> conflicting_items;
	for (const FileOrFolder& item : to_move) {
		String old_path = item.path.trim_suffix("/");
		String new_path = to_move_path.path_join(old_path.get_file());

		if ((item.is_file && FileAccess::exists(new_path)) ||
			(!item.is_file && DirAccess::exists(new_path))) {
			conflicting_items.push_back(old_path);
		}
	}
	return conflicting_items;
}

void FileSystemDock::_move_operation_confirm(
	const String& p_to_path, bool p_copy, Overwrite p_overwrite)
{
	if (p_overwrite == OVERWRITE_UNDECIDED) {
		to_move_path = p_to_path;
		to_move_or_copy = p_copy;

		Vector<String> conflicting_items = _check_existing();
		if (!conflicting_items.is_empty()) {
			// Ask to do something.
			overwrite_dialog_header->set_text(
				vformat(TTR("The following files or folders conflict with items in the target "
							"location '%s':"),
					to_move_path));
			overwrite_dialog_file_list->set_text(String("\n").join(conflicting_items));
			overwrite_dialog_footer->set_text(
				p_copy ? TTRC("Do you wish to overwrite them or rename the copied files?")
					   : TTRC("Do you wish to overwrite them or rename the moved files?"));
			overwrite_dialog->popup_centered_ratio(0.6);
			return;
		}
	}

	Vector<String> new_paths;
	new_paths.resize(to_move.size());
	for (int i = 0; i < to_move.size(); i++) {
		if (p_overwrite == OVERWRITE_RENAME) {
			new_paths.write[i] = _get_unique_name(to_move[i], p_to_path);
		}
		else {
			new_paths.write[i] = p_to_path.path_join(to_move[i].path.trim_suffix("/").get_file());
		}
	}

	if (p_copy) {
		for (int i = 0; i < to_move.size(); i++) {
			if (to_move[i].path != new_paths[i]) {
				_try_duplicate_item(to_move[i], new_paths[i]);
				select_after_scan = new_paths[i];
			}
		}
	}
	else {
		// Check groups.
		for (int i = 0; i < to_move.size(); i++) {
			if (to_move[i].is_file &&
				EditorFileSystem::get_singleton()->is_group_file(to_move[i].path)) {
				EditorFileSystem::get_singleton()->move_group_file(to_move[i].path, new_paths[i]);
			}
		}

		HashSet<String> file_owners; // The files that use these moved/renamed resource files.
		_before_move(file_owners);

		bool is_moved = false;
		HashMap<String, String> file_renames;
		HashMap<String, String> folder_renames;

		for (int i = 0; i < to_move.size(); i++) {
			if (to_move[i].path != new_paths[i]) {
				_try_move_item(to_move[i], new_paths[i], file_renames, folder_renames);
				is_moved = true;
			}
		}

		if (is_moved) {
			int current_tab = EditorSceneTabs::get_singleton()->get_current_tab();
			_update_resource_paths_after_move(file_renames);
			_update_dependencies_after_move(file_renames, file_owners);
			_update_project_settings_after_move(file_renames, folder_renames);
			_update_favorites_after_move(file_renames, folder_renames);

			EditorSceneTabs::get_singleton()->set_current_tab(current_tab);

			print_verbose("FileSystem: calling rescan.");
			_rescan();

			current_path = p_to_path;
			current_path_line_edit->set_text(current_path);
		}
	}
}

void FileSystemDock::_before_move(HashSet<String>& r_file_owners) const
{
	HashSet<String> renamed_files;
	for (int i = 0; i < to_move.size(); i++) {
		if (to_move[i].is_file) {
			renamed_files.insert(to_move[i].path);
		}
		else {
			EditorFileSystemDirectory* current_folder =
				EditorFileSystem::get_singleton()->get_filesystem_path(to_move[i].path);
			ERR_CONTINUE(current_folder == nullptr);
			List<EditorFileSystemDirectory*> folders;
			folders.push_back(current_folder);
			while (folders.front()) {
				current_folder = folders.front()->get();
				for (int j = 0; j < current_folder->get_file_count(); j++) {
					const String file_path = current_folder->get_file_path(j);
					renamed_files.insert(file_path);
				}
				for (int j = 0; j < current_folder->get_subdir_count(); j++) {
					folders.push_back(current_folder->get_subdir(j));
				}
				folders.pop_front();
			}
		}
	}

	// Look for files that use these moved/renamed resource files.
	_find_file_owners(
		EditorFileSystem::get_singleton()->get_filesystem(), renamed_files, r_file_owners);

	// Open scenes with dependencies on the ones about to be moved will be reloaded,
	// so save them first to prevent losing unsaved changes.
	EditorNode::get_singleton()->save_scene_list(r_file_owners);
}

Vector<String> FileSystemDock::_tree_get_selected(
	bool remove_self_inclusion, bool p_include_unselected_cursor) const
{
	// Build a list of selected items with the active one at the first position.
	Vector<String> selected_strings;

	TreeItem* cursor_item = tree->get_selected();

	TreeItem* selected = tree->get_root();
	selected = tree->get_next_selected(selected);
	if (remove_self_inclusion) {
		selected_strings = _remove_self_included_paths(selected_strings);
	}
	return selected_strings;
}

Vector<String> FileSystemDock::_remove_self_included_paths(Vector<String> selected_strings)
{
	// Remove paths or files that are included into another.
	if (selected_strings.size() > 1) {
		selected_strings.sort_custom<FileNoCaseComparator>();
		String last_path = "";
		for (int i = 0; i < selected_strings.size(); i++) {
			if (!last_path.is_empty() && selected_strings[i].begins_with(last_path)) {
				selected_strings.remove_at(i);
				i--;
			}
			if (selected_strings[i].ends_with("/")) {
				last_path = selected_strings[i];
			}
		}
	}
	return selected_strings;
}

void FileSystemDock::_tree_rmb_option(int p_option)
{
	if (p_option > FILE_MENU_MAX && p_option < CONVERT_BASE_ID) {
		// Extra options don't need paths.
		_file_option(p_option, {});
		return;
	}

	Vector<String> selected_strings = _tree_get_selected(false);

	// Execute the current option.
	switch (p_option) {
	case FILE_MENU_EXPAND_ALL:
	case FILE_MENU_COLLAPSE_ALL: {
		// Expand or collapse the folder
		if (selected_strings.size() == 1) {
			tree->get_selected()->set_collapsed_recursive(p_option == FILE_MENU_COLLAPSE_ALL);
		}
	} break;
	case FILE_MENU_RENAME: {
		selected_strings = _tree_get_selected(false, true);
		[[fallthrough]];
	}
	default: {
		_file_option(p_option, selected_strings);
	} break;
	}
}

void FileSystemDock::_file_list_rmb_option(int p_option)
{
	if (p_option > FILE_MENU_MAX && p_option < CONVERT_BASE_ID) {
		// Extra options don't need paths.
		_file_option(p_option, {});
		return;
	}
	_file_option(p_option, _file_list_get_selected());
}

void FileSystemDock::_generic_rmb_option_selected(int p_option)
{
	// Used for submenu commands where we don't know whether we're
	// calling from the file_list_rmb menu or the _tree_rmb option.
	if (files->has_focus()) {
		_file_list_rmb_option(p_option);
	}
	else {
		_tree_rmb_option(p_option);
	}
}

int FileSystemDock::_get_menu_option_from_key(const Ref<InputEventKey>& p_key)
{
	if (ED_IS_SHORTCUT("filesystem_dock/duplicate", p_key)) {
		return FILE_MENU_DUPLICATE;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/copy_path", p_key)) {
		return FILE_MENU_COPY_PATH;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/copy_absolute_path", p_key)) {
		return FILE_MENU_COPY_ABSOLUTE_PATH;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/copy_uid", p_key)) {
		return FILE_MENU_COPY_UID;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/delete", p_key)) {
		return FILE_MENU_REMOVE;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/new_folder", p_key)) {
		return FILE_MENU_NEW_FOLDER;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/new_scene", p_key)) {
		return FILE_MENU_NEW_SCENE;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/new_script", p_key)) {
		return FILE_MENU_NEW_SCRIPT;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/new_resource", p_key)) {
		return FILE_MENU_NEW_RESOURCE;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/new_textfile", p_key)) {
		return FILE_MENU_NEW_TEXTFILE;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/rename", p_key)) {
		return FILE_MENU_RENAME;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/show_in_explorer", p_key)) {
		return FILE_MENU_SHOW_IN_EXPLORER;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/open_in_external_program", p_key)) {
		return FILE_MENU_OPEN_EXTERNAL;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/open_in_terminal", p_key)) {
		return FILE_MENU_OPEN_IN_TERMINAL;
	}
	else if (ED_IS_SHORTCUT("filesystem_dock/focus_path", p_key)) {
		return EXTRA_FOCUS_PATH;
	}
	else if (ED_IS_SHORTCUT("editor/open_search", p_key)) {
		return EXTRA_FOCUS_FILTER;
	}
	return -1;
}

void FileSystemDock::_search_changed(const String& p_text, const Control* p_from)
{
	if (searched_tokens.is_empty()) {
		// Register the uncollapsed paths before they change.
		uncollapsed_paths_before_search = get_uncollapsed_paths();
	}

	const String searched_string = p_text.to_lower();
	if (searched_string.begins_with("uid://")) {
		ResourceUID::ID id = ResourceUID::get_singleton()->text_to_id(searched_string);
		if (id != ResourceUID::INVALID_ID && ResourceUID::get_singleton()->has_id(id)) {
			navigate_to_path(ResourceUID::get_singleton()->get_id_path(id));
			return;
		}
	}

	searched_tokens = searched_string.split(" ", false);

	if (p_from == tree_search_box) {
		file_list_search_box->set_text(searched_string);
	}
	else { // File_list_search_box.
		tree_search_box->set_text(searched_string);
	}

	_update_filtered_items();
	if (display_mode == DISPLAY_MODE_HSPLIT || display_mode == DISPLAY_MODE_VSPLIT) {
		_update_file_list(false);
	}
	if (searched_tokens.is_empty()) {
		_navigate_to_path(current_path);
	}
}

bool FileSystemDock::_matches_all_search_tokens(const String& p_text)
{
	if (searched_tokens.is_empty()) {
		return false;
	}
	const String s = p_text.to_lower();
	for (const String& t : searched_tokens) {
		if (!s.contains(t)) {
			return false;
		}
	}
	return true;
}

void FileSystemDock::_rescan()
{
	if (tree->has_focus()) {
		had_focus = tree;
	}
	else if (files->has_focus()) {
		had_focus = files;
	}

	_set_scanning_mode();
	EditorFileSystem::get_singleton()->scan();
}

void FileSystemDock::_change_split_mode()
{
	DisplayMode next_mode = DISPLAY_MODE_TREE_ONLY;
	if (display_mode == DISPLAY_MODE_VSPLIT) {
		next_mode = DISPLAY_MODE_HSPLIT;
	}
	else if (display_mode == DISPLAY_MODE_TREE_ONLY) {
		next_mode = DISPLAY_MODE_VSPLIT;
	}

	set_display_mode(next_mode);
}

void FileSystemDock::_split_dragged(int p_offset)
{
	if (split_box->is_vertical()) {
		split_box_offset_v = p_offset;
	}
	else {
		split_box_offset_h = p_offset;
	}
}

void FileSystemDock::fix_dependencies(const String& p_for_file) { deps_editor->edit(p_for_file); }

void FileSystemDock::update_all()
{
	if (tree->is_visible()) {
		_update_tree(get_uncollapsed_paths(), false, false);
	}

	if (file_list_vb->is_visible()) {
		_update_file_list(true);
	}
}

void FileSystemDock::focus_on_path()
{
	current_path_line_edit->grab_focus();
	current_path_line_edit->select_all();
}

void FileSystemDock::focus_on_filter()
{
	LineEdit* current_search_box = nullptr;
	if (display_mode == DISPLAY_MODE_TREE_ONLY) {
		current_search_box = tree_search_box;
	}
	else {
		current_search_box = file_list_search_box;
	}

	if (current_search_box) {
		current_search_box->grab_focus();
		current_search_box->select_all();
	}
}

void FileSystemDock::create_directory(const String& p_path, const String& p_base_dir)
{
	String trimmed_path = p_path;
	if (!p_base_dir.is_empty()) {
		// Trims off the joining '/' if the base didn't end with one. If the base did have it
		// and there's two slashes, the empty directory is safe to trim off anyways.
		trimmed_path = trimmed_path.trim_prefix(p_base_dir).trim_prefix("/");
	}
	Error err = EditorFileSystem::get_singleton()->make_dir_recursive(trimmed_path, p_base_dir);
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Could not create folder: %s"), TTR(error_names[err])));
	}
}

ScriptCreateDialog* FileSystemDock::get_script_create_dialog() const { return make_script_dialog; }

void FileSystemDock::set_file_list_display_mode(FileListDisplayMode p_mode)
{
	if (p_mode == file_list_display_mode) {
		return;
	}

	_toggle_file_display();
}

void FileSystemDock::add_resource_tooltip_plugin(const Ref<EditorResourceTooltipPlugin>& p_plugin)
{
	tooltip_plugins.push_back(p_plugin);
}

void FileSystemDock::remove_resource_tooltip_plugin(
	const Ref<EditorResourceTooltipPlugin>& p_plugin)
{
	int index = tooltip_plugins.find(p_plugin);
	ERR_FAIL_COND_MSG(index == -1, "Can't remove plugin that wasn't registered.");
	tooltip_plugins.remove_at(index);
}

Control* FileSystemDock::create_tooltip_for_path(const String& p_path) const
{
	if (p_path == "Favorites") {
		// No tooltip for the "Favorites" group.
		return nullptr;
	}
	if (DirAccess::exists(p_path)) {
		// No tooltip for directory.
		return nullptr;
	}
	ERR_FAIL_COND_V(!FileAccess::exists(p_path), nullptr);

	const String type = ResourceLoader::get_resource_type(p_path);
	Control* tooltip = EditorResourceTooltipPlugin::make_default_tooltip(p_path);
	return tooltip;
}

void FileSystemDock::_get_drag_target_folder(
	String& target, bool& target_favorites, const Point2& p_point, Control* p_from) const
{
	target = String();
	target_favorites = false;

	// In the tree.
	if (p_from == tree) {
		TreeItem* ti = (p_point == Vector2(Math::INF, Math::INF))
						   ? tree->get_selected()
						   : tree->get_item_at_position(p_point);
		if (ti) {
			int section = (p_point == Vector2(Math::INF, Math::INF))
							  ? tree->get_drop_section_at_position(tree->get_item_rect(ti).position)
							  : tree->get_drop_section_at_position(p_point);

			// Check the favorites first.
			if (ti == tree->get_root()->get_first_child() && section >= 0) {
				target_favorites = true;
				return;
			}
			else if (ti->get_parent() == tree->get_root()->get_first_child()) {
				target_favorites = true;
				return;
			}
		}
	}
}

void FileSystemDock::_file_and_folders_fill_popup(
	PopupMenu* p_popup, const Vector<String>& p_paths, bool p_display_path_dependent_options)
{
	Vector<String> filenames;
	Vector<String> foldernames;

	Vector<String> favorites_list = EditorSettings::get_singleton()->get_favorites();

	bool no_paths = p_paths.is_empty();
	bool single_path = !no_paths && p_paths.size() == 1;

	bool all_files = !no_paths;
	bool all_files_scenes = true;
	bool all_folders = !no_paths;
	bool all_favorites = true;
	bool all_not_favorites = true;

	for (const String& fpath : p_paths) {
		if (fpath.ends_with("/")) {
			foldernames.push_back(fpath);
			all_files = false;
		}
		else {
			filenames.push_back(fpath);
			all_folders = false;
			all_files_scenes &=
				(EditorFileSystem::get_singleton()->get_file_type(fpath) == "PackedScene");
		}

		// Check if in favorites.
		bool found = false;
		for (const String& fav : favorites_list) {
			if (fav == fpath) {
				found = true;
				break;
			}
		}
		if (found) {
			all_not_favorites = false;
		}
		else {
			all_favorites = false;
		}
	}

	if (all_files) {
		if (all_files_scenes) {
			if (filenames.size() == 1) {
				p_popup->add_icon_item(
					get_editor_theme_icon(SNAME("Load")), TTRC("Open Scene"), FILE_MENU_OPEN);
				p_popup->add_icon_item(
					get_editor_theme_icon(SNAME("Play")), TTRC("Play Scene"), FILE_MENU_RUN_SCENE);
				p_popup->add_icon_item(get_editor_theme_icon(SNAME("CreateNewSceneFrom")),
					TTRC("New Inherited Scene"), FILE_MENU_INHERIT);
				if (main_scene_path != filenames[0]) {
					p_popup->add_icon_item(get_editor_theme_icon(SNAME("PlayScene")),
						TTRC("Set as Main Scene"), FILE_MENU_MAIN_SCENE);
				}
			}
			else {
				p_popup->add_icon_item(
					get_editor_theme_icon(SNAME("Load")), TTRC("Open Scenes"), FILE_MENU_OPEN);
			}
			p_popup->add_icon_item(get_editor_theme_icon(SNAME("Instance")), TTRC("Instantiate"),
				FILE_MENU_INSTANTIATE);
			p_popup->add_separator();
		}
		else if (filenames.size() == 1) {
			p_popup->add_icon_item(
				get_editor_theme_icon(SNAME("Load")), TTRC("Open"), FILE_MENU_OPEN);

			String type = EditorFileSystem::get_singleton()->get_file_type(filenames[0]);
			p_popup->add_separator();
		}

		if (filenames.size() == 1) {
			p_popup->add_item(TTRC("Edit Dependencies..."), FILE_MENU_DEPENDENCIES);
			p_popup->add_item(TTRC("View Owners..."), FILE_MENU_OWNERS);
			p_popup->add_separator();
		}
	}

	if (no_paths) {
		_add_create_options(p_popup, String());
	}

	// Check if the root path is selected, we must check p_paths[1] because the first string in
	// the list of paths obtained by _tree_get_selected(...) is not always the root path.
	bool root_path_not_selected =
		!no_paths && p_paths[0] != "res://" && (p_paths.size() <= 1 || p_paths[1] != "res://");

	if (all_folders && foldernames.size() > 0) {
		p_popup->add_icon_item(
			get_editor_theme_icon(SNAME("Load")), TTRC("Expand Folder"), FILE_MENU_OPEN);

		if (foldernames.size() == 1) {
			p_popup->add_icon_item(get_editor_theme_icon(SNAME("GuiTreeArrowDown")),
				TTRC("Expand Hierarchy"), FILE_MENU_EXPAND_ALL);
			p_popup->add_icon_item(get_editor_theme_icon(SNAME("GuiTreeArrowRight")),
				TTRC("Collapse Hierarchy"), FILE_MENU_COLLAPSE_ALL);
		}

		p_popup->add_separator();
	}

	// Add the options that are only available when a single item is selected.
	if (single_path) {
		p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("ActionCopy")),
			ED_GET_SHORTCUT("filesystem_dock/copy_path"), FILE_MENU_COPY_PATH);
		p_popup->add_shortcut(
			ED_GET_SHORTCUT("filesystem_dock/copy_absolute_path"), FILE_MENU_COPY_ABSOLUTE_PATH);
		if (ResourceLoader::get_resource_uid(p_paths[0]) != ResourceUID::INVALID_ID) {
			p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Instance")),
				ED_GET_SHORTCUT("filesystem_dock/copy_uid"), FILE_MENU_COPY_UID);
		}
		if (root_path_not_selected) {
			p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Rename")),
				ED_GET_SHORTCUT("filesystem_dock/rename"), FILE_MENU_RENAME);
			p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Duplicate")),
				ED_GET_SHORTCUT("filesystem_dock/duplicate"), FILE_MENU_DUPLICATE);
		}
	}

	// Add the options that are only available when the root path is not selected.
	if (root_path_not_selected) {
		p_popup->add_icon_item(
			get_editor_theme_icon(SNAME("MoveUp")), TTRC("Move/Duplicate To..."), FILE_MENU_MOVE);
		p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Remove")),
			ED_GET_SHORTCUT("filesystem_dock/delete"), FILE_MENU_REMOVE);
	}

	// Only add a separator if we have actually placed any options in the menu since the last
	// separator.
	if (single_path || root_path_not_selected) {
		p_popup->add_separator();
	}

	// Add the options that are available when one or more items are selected.
	if (p_paths.size() >= 1) {
		if (!all_favorites) {
			p_popup->add_icon_item(get_editor_theme_icon(SNAME("Favorites")),
				TTRC("Add to Favorites"), FILE_MENU_ADD_FAVORITE);
		}
		if (!all_not_favorites) {
			p_popup->add_icon_item(get_editor_theme_icon(SNAME("NonFavorite")),
				TTRC("Remove from Favorites"), FILE_MENU_REMOVE_FAVORITE);
		}

		if (root_path_not_selected) {
			cached_valid_conversion_targets = _get_valid_conversions_for_file_paths(p_paths);

			int relative_id = 0;
			if (!cached_valid_conversion_targets.is_empty()) {
				p_popup->add_separator();

				// If we have more than one type we can convert into, collapse it into a submenu.
				const int CONVERSION_SUBMENU_THRESHOLD = 1;

				PopupMenu* container_menu = p_popup;
				String conversion_string_template = "Convert to %s";

				for (const String& E : cached_valid_conversion_targets) {
					Ref<Texture2D> icon;
					if (has_theme_icon(E, SNAME("EditorIcons"))) {
						icon = get_editor_theme_icon(E);
					}
					else {
						icon = get_editor_theme_icon(SNAME("Object"));
					}

					container_menu->add_icon_item(icon, vformat(TTR(conversion_string_template), E),
						CONVERT_BASE_ID + relative_id);
					relative_id++;
				}
			}
		}

		{
			List<String> resource_extensions;
			ResourceFormatImporter::get_singleton()->get_recognized_extensions_for_type(
				"Resource", &resource_extensions);
			HashSet<String> extension_list;
			for (const String& extension : resource_extensions) {
				extension_list.insert(extension);
			}

			bool resource_valid = true;
			String main_extension;

			for (int i = 0; i != p_paths.size(); ++i) {
				String extension = p_paths[i].get_extension();
				if (extension_list.has(extension)) {
					if (main_extension.is_empty()) {
						main_extension = extension;
					}
					else if (extension != main_extension) {
						resource_valid = false;
						break;
					}
				}
				else {
					resource_valid = false;
					break;
				}
			}

			if (resource_valid) {
				p_popup->add_icon_item(
					get_editor_theme_icon(SNAME("Load")), TTRC("Reimport"), FILE_MENU_REIMPORT);
			}
		}
	}

	if (single_path) {
		const String& fpath = p_paths[0];

		[[maybe_unused]] bool added_separator = false;

		if (favorites_list.has(fpath)) {
			TreeItem* cursor_item = tree->get_selected();
			bool is_item_in_favorites = false;
			while (cursor_item != nullptr) {
				if (cursor_item == favorites_item) {
					is_item_in_favorites = true;
					break;
				}

				cursor_item = cursor_item->get_parent();
			}

			if (is_item_in_favorites) {
				p_popup->add_separator();
				added_separator = true;
				p_popup->add_icon_item(get_editor_theme_icon(SNAME("ShowInFileSystem")),
					TTRC("Show in FileSystem"), FILE_MENU_SHOW_IN_FILESYSTEM);
			}
		}

#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		if (!added_separator) {
			p_popup->add_separator();
			added_separator = true;
		}

		// Opening the system file manager is not supported on the Android and web editors.
		const bool is_directory = fpath.ends_with("/");

		p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Terminal")),
			ED_GET_SHORTCUT("filesystem_dock/open_in_terminal"), FILE_MENU_OPEN_IN_TERMINAL);
		p_popup->set_item_text(p_popup->get_item_index(FILE_MENU_OPEN_IN_TERMINAL),
			is_directory ? TTRC("Open in Terminal") : TTRC("Open Folder in Terminal"));

		if (!is_directory) {
			p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("ExternalLink")),
				ED_GET_SHORTCUT("filesystem_dock/open_in_external_program"),
				FILE_MENU_OPEN_EXTERNAL);
		}

		p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Filesystem")),
			ED_GET_SHORTCUT("filesystem_dock/show_in_explorer"), FILE_MENU_SHOW_IN_EXPLORER);
		p_popup->set_item_text(p_popup->get_item_index(FILE_MENU_SHOW_IN_EXPLORER),
			is_directory
				? OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN)
				: OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_SHOW));
#endif

		current_path = fpath;
	}
	else if (no_paths) {
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		tree_popup->add_separator();
		tree_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Terminal")),
			ED_GET_SHORTCUT("filesystem_dock/open_in_terminal"), FILE_MENU_OPEN_IN_TERMINAL);
		tree_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("Filesystem")),
			ED_GET_SHORTCUT("filesystem_dock/show_in_explorer"), FILE_MENU_SHOW_IN_EXPLORER);
#endif
	}

#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	if (all_files && p_paths.size() > 1) {
		p_popup->add_separator();
		p_popup->add_icon_shortcut(get_editor_theme_icon(SNAME("ExternalLink")),
			ED_GET_SHORTCUT("filesystem_dock/open_in_external_program"), FILE_MENU_OPEN_EXTERNAL);
	}
#endif
	EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(
		p_popup, EditorContextMenuPlugin::CONTEXT_SLOT_FILESYSTEM, p_paths);
}

void FileSystemDock::_add_create_options(PopupMenu* p_popup, const String& p_base_folder)
{
	bool prefix_new = p_base_folder.is_empty();
	p_popup->add_icon_item(get_editor_theme_icon(SNAME("Folder")),
		prefix_new ? TTRC("New Folder...") : TTRC("Folder..."), FILE_MENU_NEW_FOLDER);
	p_popup->set_item_shortcut(-1, ED_GET_SHORTCUT("filesystem_dock/new_folder"));
	p_popup->add_icon_item(get_editor_theme_icon(SNAME("PackedScene")),
		prefix_new ? TTRC("New Scene...") : TTRC("Scene..."), FILE_MENU_NEW_SCENE);
	p_popup->set_item_shortcut(-1, ED_GET_SHORTCUT("filesystem_dock/new_scene"));
	p_popup->add_icon_item(get_editor_theme_icon(SNAME("Script")),
		prefix_new ? TTRC("New Script...") : TTRC("Script..."), FILE_MENU_NEW_SCRIPT);
	p_popup->set_item_shortcut(-1, ED_GET_SHORTCUT("filesystem_dock/new_script"));
	p_popup->add_icon_item(get_editor_theme_icon(SNAME("Object")),
		prefix_new ? TTRC("New Resource...") : TTRC("Resource..."), FILE_MENU_NEW_RESOURCE);
	p_popup->set_item_shortcut(-1, ED_GET_SHORTCUT("filesystem_dock/new_resource"));
	p_popup->add_icon_item(get_editor_theme_icon(SNAME("TextFile")),
		prefix_new ? TTRC("New TextFile...") : TTRC("TextFile..."), FILE_MENU_NEW_TEXTFILE);
	p_popup->set_item_shortcut(-1, ED_GET_SHORTCUT("filesystem_dock/new_textfile"));
	// Options for CONTEXT_SLOT_FILESYSTEM_CREATE are added with an offset, to avoid conflicts in
	// case plugins add options for both FileSystem slots.
	EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(p_popup,
		EditorContextMenuPlugin::CONTEXT_SLOT_FILESYSTEM_CREATE,
		prefix_new ? PackedStringArray() : PackedStringArray{p_base_folder}, 500);
}

void FileSystemDock::_tree_rmb_select(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	tree->grab_focus(true);

	// Right click is pressed in the tree.
	Vector<String> paths = _tree_get_selected(false);

	tree_popup->clear();

	// Popup.
	if (!paths.is_empty()) {
		tree_popup->reset_size();
		_file_and_folders_fill_popup(tree_popup, paths);
		tree_popup->set_position(tree->get_screen_position() + p_pos);
		tree_popup->reset_size();
		tree_popup->popup();
	}
}

void FileSystemDock::_tree_empty_click(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	// Right click is pressed in the empty space of the tree.
	current_path = "res://";
	tree_popup->clear();
	_file_and_folders_fill_popup(tree_popup, PackedStringArray());
	tree_popup->set_position(tree->get_screen_position() + p_pos);
	tree_popup->reset_size();
	tree_popup->popup();
}

void FileSystemDock::_tree_empty_selected()
{
	tree->deselect_all();
	current_path = "";
	current_path_line_edit->set_text(current_path);
	if (file_list_vb->is_visible()) {
		_update_file_list(false);
	}
	_update_selection_changed();
}

void FileSystemDock::_file_list_item_clicked(
	int p_item, const Vector2& p_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index != MouseButton::RIGHT) {
		return;
	}
	files->grab_focus(true);

	// Right click is pressed in the file list.
	Vector<String> paths;
	for (int i = 0; i < files->get_item_count(); i++) {
		if (!files->is_selected(i)) {
			continue;
		}
		if (files->get_item_text(p_item) == "..") {
			files->deselect(i);
			continue;
		}
	}

	// Popup.
	if (!paths.is_empty()) {
		file_list_popup->clear();
		_file_and_folders_fill_popup(file_list_popup, paths, searched_tokens.is_empty());
		file_list_popup->set_position(files->get_screen_position() + p_pos);
		file_list_popup->reset_size();
		file_list_popup->popup();
	}
}

void FileSystemDock::_file_list_empty_clicked(
	const Vector2& p_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index != MouseButton::RIGHT) {
		return;
	}

	// Right click on empty space for file list.
	if (!searched_tokens.is_empty()) {
		return;
	}

	current_path = current_path_line_edit->get_text();

	// Favorites isn't a directory so don't show menu.
	if (current_path == "Favorites") {
		return;
	}

	file_list_popup->clear();
	_file_and_folders_fill_popup(file_list_popup, PackedStringArray());
	file_list_popup->set_position(files->get_screen_position() + p_pos);
	file_list_popup->reset_size();
	file_list_popup->popup();
}

void FileSystemDock::select_file(const String& p_file) { _navigate_to_path(p_file); }

void FileSystemDock::_tree_mouse_exited()
{
	if (holding_branch) {
		_reselect_items_selected_on_drag_begin();
	}
}

void FileSystemDock::_reselect_items_selected_on_drag_begin(bool reset)
{
	TreeItem* selected_item = tree->get_next_selected(tree->get_root());
	if (selected_item) {
		selected_item->deselect(0);
	}
	if (!tree_items_selected_on_drag_begin.is_empty()) {
		bool reselected = false;
		for (TreeItem* item : tree_items_selected_on_drag_begin) {
			if (item->get_tree()) {
				item->select(0);
				reselected = true;
			}
		}

		if (reset) {
			tree_items_selected_on_drag_begin.clear();
		}

		if (!reselected) {
			// If couldn't reselect the items selected on drag begin, select the "res://" item.
			tree->get_root()->get_child(1)->select(0);
		}
	}

	files->deselect_all();
	if (!list_items_selected_on_drag_begin.is_empty()) {
		for (const int idx : list_items_selected_on_drag_begin) {
			files->select(idx, false);
		}

		if (reset) {
			list_items_selected_on_drag_begin.clear();
		}
	}
}

bool FileSystemDock::_get_imported_files(
	const String& p_path, String& r_extension, Vector<String>& r_files) const
{
	if (!p_path.ends_with("/")) {
		if (FileAccess::exists(p_path + ".import")) {
			if (r_extension.is_empty()) {
				r_extension = p_path.get_extension();
			}
			else if (r_extension != p_path.get_extension()) {
				r_files.clear();
				return false; // File type mismatch, stop search.
			}

			r_files.push_back(p_path);
		}
		return true;
	}

	Ref<DirAccess> da = DirAccess::open(p_path);
	ERR_FAIL_COND_V(da.is_null(), false);

	da->list_dir_begin();
	String n = da->get_next();
	while (!n.is_empty()) {
		if (n != "." && n != ".." && !n.ends_with(".import")) {
			String npath = p_path + n + (da->current_is_dir() ? "/" : "");
			if (!_get_imported_files(npath, r_extension, r_files)) {
				return false;
			}
		}
		n = da->get_next();
	}
	da->list_dir_end();
	return true;
}

void FileSystemDock::_update_import_dock()
{
	if (!import_dock_needs_update) {
		return;
	}

	_update_selection_changed();

	// List selected.
	Vector<String> selected;
	if (display_mode == DISPLAY_MODE_TREE_ONLY) {
		// Use the tree
		selected = _tree_get_selected();
	}
	if (!selected.is_empty() && selected[0] == "res://") {
		// Scanning res:// is costly and unlikely to yield any useful results.
		return;
	}

	// Expand directory selection.
	Vector<String> efiles;
	String extension;
	for (const String& fpath : selected) {
		_get_imported_files(fpath, extension, efiles);
	}

	// Check import.
	Vector<String> imports;
	String import_type;
	for (int i = 0; i < efiles.size(); i++) {
		const String& fpath = efiles[i];
		Ref<ConfigFile> cf;
		cf.instantiate();
		Error err = cf->load(fpath + ".import");
		if (err != OK) {
			imports.clear();
			break;
		}
		imports.push_back(fpath);
	}

	if (imports.is_empty()) {
		ImportDock::get_singleton()->clear();
	}
	else if (imports.size() == 1) {
		ImportDock::get_singleton()->set_edit_path(imports[0]);
	}
	else {
		ImportDock::get_singleton()->set_edit_multiple_paths(imports);
	}

	import_dock_needs_update = false;
}

void FileSystemDock::_feature_profile_changed() { _update_display_mode(true); }

void FileSystemDock::set_file_sort(FileSortOption p_file_sort)
{
	for (int i = 0; i != (int)FileSortOption::FILE_SORT_MAX; i++) {
		tree_button_sort->get_popup()->set_item_checked(i, (i == (int)p_file_sort));
		file_list_button_sort->get_popup()->set_item_checked(i, (i == (int)p_file_sort));
	}
	file_sort = p_file_sort;

	// Update everything needed.
	update_all();
}

void FileSystemDock::_file_sort_popup(int p_id) { set_file_sort((FileSortOption)p_id); }

// TODO: Could use a unit test.
const HashMap<String, Color>& FileSystemDock::get_folder_colors() const { return folder_colors; }

void FileSystemDock::_on_open_editor_settings_file_exts()
{
	unrecognized_ext_dialog->hide();

	// The FileSystem settings are under "advanced settings", so we have to ensure
	// that setting is enabled before we attempt to open the menu to them.
	EditorSettingsDialog* ed_settings = EditorNode::get_singleton()->editor_settings_dialog;
	ed_settings->set_advanced_mode_enabled(true);
	ed_settings->popup_edit_settings();
	ed_settings->set_current_section("docks/filesystem");
}

void FileSystemDock::_bind_methods() {}

FileSystemDock::FileSystemDock()
{
	singleton = this;
	set_name(TTRC("FileSystem"));
	set_icon_name("Folder");
	set_dock_shortcut(ED_SHORTCUT_AND_COMMAND(
		"docks/open_filesystem", TTRC("Open FileSystem Dock"), KeyModifierMask::ALT | Key::F));
	set_default_slot(EditorDock::DOCK_SLOT_LEFT_BR);
	set_available_layouts(DOCK_LAYOUT_ALL);

	ProjectSettings::get_singleton()->add_hidden_prefix("file_customization/");

	// `KeyModifierMask::CMD_OR_CTRL | Key::C` conflicts with other editor shortcuts.
	ED_SHORTCUT("filesystem_dock/copy_path", TTRC("Copy Path"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::C);
	ED_SHORTCUT("filesystem_dock/copy_absolute_path", TTRC("Copy Absolute Path"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::C);
	ED_SHORTCUT("filesystem_dock/copy_uid", TTRC("Copy UID"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | KeyModifierMask::SHIFT | Key::C);
	ED_SHORTCUT(
		"filesystem_dock/duplicate", TTRC("Duplicate..."), KeyModifierMask::CMD_OR_CTRL | Key::D);
	ED_SHORTCUT("filesystem_dock/delete", TTRC("Delete"), Key::KEY_DELETE);
	ED_SHORTCUT("filesystem_dock/new_folder", TTRC("New Folder..."), Key::NONE);
	ED_SHORTCUT("filesystem_dock/new_scene", TTRC("New Scene..."), Key::NONE);
	ED_SHORTCUT("filesystem_dock/new_script", TTRC("New Script..."), Key::NONE);
	ED_SHORTCUT("filesystem_dock/new_resource", TTRC("New Resource..."), Key::NONE);
	ED_SHORTCUT("filesystem_dock/new_textfile", TTRC("New TextFile..."), Key::NONE);
	ED_SHORTCUT("filesystem_dock/rename", TTRC("Rename..."), Key::F2);
	ED_SHORTCUT_OVERRIDE("filesystem_dock/rename", "macos", Key::ENTER);
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	// Opening the system file manager or opening in an external program is not supported on the
	// Android and web editors.
	ED_SHORTCUT("filesystem_dock/show_in_explorer", TTRC("Open in File Manager"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::R);
	ED_SHORTCUT("filesystem_dock/open_in_external_program", TTRC("Open in External Program"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::E);
	ED_SHORTCUT("filesystem_dock/open_in_terminal", TTRC("Open in Terminal"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT | Key::T);
#endif

	ED_SHORTCUT(
		"filesystem_dock/focus_path", TTRC("Focus Path"), KeyModifierMask::CMD_OR_CTRL | Key::L);
	// Allow both Cmd + L and Cmd + Shift + G to match Safari's and Finder's shortcuts respectively.
	ED_SHORTCUT_OVERRIDE_ARRAY("filesystem_dock/focus_path", "macos",
		{int32_t(KeyModifierMask::META | Key::L),
			int32_t(KeyModifierMask::META | KeyModifierMask::SHIFT | Key::G)});

	// Properly translating color names would require a separate HashMap, so for simplicity they are
	// provided as comments.
	folder_colors["red"] = Color(1.0, 0.271, 0.271);	// TTR("Red")
	folder_colors["orange"] = Color(1.0, 0.561, 0.271); // TTR("Orange")
	folder_colors["yellow"] = Color(1.0, 0.890, 0.271); // TTR("Yellow")
	folder_colors["green"] = Color(0.502, 1.0, 0.271);	// TTR("Green")
	folder_colors["teal"] = Color(0.271, 1.0, 0.635);	// TTR("Teal")
	folder_colors["blue"] = Color(0.271, 0.843, 1.0);	// TTR("Blue")
	folder_colors["purple"] = Color(0.502, 0.271, 1.0); // TTR("Purple")
	folder_colors["pink"] = Color(1.0, 0.271, 0.588);	// TTR("Pink")
	folder_colors["gray"] = Color(0.616, 0.616, 0.616); // TTR("Gray")

	editor_is_dark_icon_and_font = EditorThemeManager::is_dark_icon_and_font();

	VBoxContainer* main_vb = memnew(VBoxContainer);
	add_child(main_vb);

	VBoxContainer* top_vbc = memnew(VBoxContainer);
	main_vb->add_child(top_vbc);

	toolbar_hbc = memnew(HBoxContainer);
	top_vbc->add_child(toolbar_hbc);

	HBoxContainer* nav_hbc = memnew(HBoxContainer);
	nav_hbc->add_theme_constant_override("separation", 0);
	toolbar_hbc->add_child(nav_hbc);

	button_hist_prev = memnew(Button);
	button_hist_prev->set_theme_type_variation(SceneStringName(FlatButton));
	button_hist_prev->set_disabled(true);
	button_hist_prev->set_focus_mode(FOCUS_ACCESSIBILITY);
	button_hist_prev->set_tooltip_text(TTRC("Go to previous selected folder/file."));
	nav_hbc->add_child(button_hist_prev);

	button_hist_next = memnew(Button);
	button_hist_next->set_theme_type_variation(SceneStringName(FlatButton));
	button_hist_next->set_disabled(true);
	button_hist_next->set_focus_mode(FOCUS_ACCESSIBILITY);
	button_hist_next->set_tooltip_text(TTRC("Go to next selected folder/file."));
	nav_hbc->add_child(button_hist_next);

	current_path_line_edit = memnew(LineEdit);
	current_path_line_edit->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
	current_path_line_edit->set_accessibility_name(TTRC("Path"));
	current_path_line_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	_set_current_path_line_edit_text(current_path);
	toolbar_hbc->add_child(current_path_line_edit);

	button_toggle_display_mode = memnew(Button);
	button_toggle_display_mode->set_focus_mode(FOCUS_ACCESSIBILITY);
	button_toggle_display_mode->set_tooltip_text(TTRC("Change Split Mode"));
	button_toggle_display_mode->set_theme_type_variation("FlatMenuButton");
	toolbar_hbc->add_child(button_toggle_display_mode);

	toolbar2_hbc = memnew(HBoxContainer);
	top_vbc->add_child(toolbar2_hbc);

	tree_search_box = memnew(LineEdit);
	tree_search_box->set_h_size_flags(SIZE_EXPAND_FILL);
	tree_search_box->set_placeholder(TTRC("Filter Files"));
	tree_search_box->set_clear_button_enabled(true);
	toolbar2_hbc->add_child(tree_search_box);

	tree_button_sort = _create_file_menu_button();
	toolbar2_hbc->add_child(tree_button_sort);

	file_list_popup = memnew(PopupMenu);

	add_child(file_list_popup);

	tree_popup = memnew(PopupMenu);

	add_child(tree_popup);

	split_box = memnew(SplitContainer);
	split_box->set_v_size_flags(SIZE_EXPAND_FILL);
	split_box_offset_h = 240 * EDSCALE;
	main_vb->add_child(split_box);

	tree_mc = memnew(MarginContainer);
	split_box->add_child(tree_mc);
	tree_mc->set_theme_type_variation("NoBorderHorizontalBottom");
	tree_mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	tree = memnew(FileSystemTree);
	tree->set_accessibility_name(TTRC("Directories"));
	tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tree->set_hide_root(true);
	tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_TOP);
	tree->set_allow_reselect(true);
	tree->set_allow_rmb_select(true);
	tree->set_select_mode(Tree::SELECT_MULTI);
	tree->set_custom_minimum_size(Size2(40 * EDSCALE, 15 * EDSCALE));
	tree->set_column_clip_content(0, true);
	tree_mc->add_child(tree);

	file_list_vb = memnew(VBoxContainer);
	file_list_vb->set_v_size_flags(SIZE_EXPAND_FILL);
	split_box->add_child(file_list_vb);

	path_hb = memnew(HBoxContainer);
	path_hb->set_h_size_flags(SIZE_EXPAND_FILL);
	file_list_vb->add_child(path_hb);

	file_list_search_box = memnew(LineEdit);
	file_list_search_box->set_h_size_flags(SIZE_EXPAND_FILL);
	file_list_search_box->set_placeholder(TTRC("Filter Files"));
	file_list_search_box->set_accessibility_name(TTRC("Filter Files"));
	file_list_search_box->set_clear_button_enabled(true);
	path_hb->add_child(file_list_search_box);

	file_list_button_sort = _create_file_menu_button();
	path_hb->add_child(file_list_button_sort);

	button_file_list_display_mode = memnew(Button);
	button_file_list_display_mode->set_accessibility_name(TTRC("Display Mode"));
	button_file_list_display_mode->set_theme_type_variation("FlatMenuButton");
	path_hb->add_child(button_file_list_display_mode);

	files_mc = memnew(MarginContainer);
	file_list_vb->add_child(files_mc);
	files_mc->set_theme_type_variation("NoBorderHorizontalBottom");
	files_mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	files = memnew(FileSystemList);
	files->set_accessibility_name(TTRC("Files"));
	files->set_select_mode(ItemList::SELECT_MULTI);
	files->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_TOP);
	files->set_custom_minimum_size(Size2(0, 15 * EDSCALE));
	files->set_allow_rmb_select(true);
	files_mc->add_child(files);

	scanning_vb = memnew(VBoxContainer);
	scanning_vb->hide();
	main_vb->add_child(scanning_vb);

	Label* slabel = memnew(Label);
	slabel->set_text(TTRC("Scanning Files,\nPlease Wait..."));
	slabel->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	scanning_vb->add_child(slabel);

	scanning_progress = memnew(ProgressBar);
	scanning_progress->set_accessibility_name(TTRC("Filesystem Scan"));
	scanning_vb->add_child(scanning_progress);

	deps_editor = memnew(DependencyEditor);
	add_child(deps_editor);

	owners_editor = memnew(DependencyEditorOwners());
	add_child(owners_editor);

	remove_dialog = memnew(DependencyRemoveDialog);
	add_child(remove_dialog);

	move_dialog = memnew(EditorDirDialog);
	add_child(move_dialog);

	overwrite_dialog = memnew(ConfirmationDialog);
	add_child(overwrite_dialog);
	overwrite_dialog->set_ok_button_text(TTRC("Overwrite"));
	VBoxContainer* overwrite_dialog_vb = memnew(VBoxContainer);
	overwrite_dialog->add_child(overwrite_dialog_vb);

	overwrite_dialog_header = memnew(Label);
	overwrite_dialog_vb->add_child(overwrite_dialog_header);

	overwrite_dialog_scroll = memnew(ScrollContainer);
	overwrite_dialog_vb->add_child(overwrite_dialog_scroll);
	overwrite_dialog_scroll->set_custom_minimum_size(Vector2(50, 50) * EDSCALE);
	overwrite_dialog_scroll->set_v_size_flags(SIZE_EXPAND_FILL);

	overwrite_dialog_file_list = memnew(Label);
	overwrite_dialog_scroll->add_child(overwrite_dialog_file_list);

	overwrite_dialog_footer = memnew(Label);
	overwrite_dialog_vb->add_child(overwrite_dialog_footer);

	make_dir_dialog = memnew(DirectoryCreateDialog);
	add_child(make_dir_dialog);

	make_scene_dialog = memnew(SceneCreateDialog);
	add_child(make_scene_dialog);
	make_script_dialog = memnew(ScriptCreateDialog);
	make_script_dialog->set_title(TTRC("Create Script"));
	add_child(make_script_dialog);
	make_shader_dialog = memnew(ShaderCreateDialog);
	add_child(make_shader_dialog);
	new_resource_dialog = memnew(CreateDialog);
	add_child(new_resource_dialog);
	new_resource_dialog->set_base_type("Resource");

	conversion_dialog = memnew(ConfirmationDialog);
	conversion_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	add_child(conversion_dialog);
	conversion_dialog->set_ok_button_text(TTRC("Convert"));
	move_confirm_dialog = memnew(ConfirmationDialog);
	add_child(move_confirm_dialog);
	VBoxContainer* vb = memnew(VBoxContainer);
	move_confirm_dialog->add_child(vb);
	move_confirm_dialog_label = memnew(Label);
	move_confirm_dialog_label->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	vb->add_child(move_confirm_dialog_label);
	confirm_before_move_checkbox = memnew(CheckBox(TTRC("Don't Ask Again")));
	confirm_before_move_checkbox->set_tooltip_text(
		TTRC("This dialog can be skipped by holding shift or enabled/disabled in the Editor "
			 "Settings: Docks > FileSystem > Ask Before Moving Files."));
	vb->add_child(confirm_before_move_checkbox);

	unrecognized_ext_dialog = memnew(AcceptDialog);
	unrecognized_ext_dialog->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	add_child(unrecognized_ext_dialog);
	unrecognized_ext_dialog->set_text(
		TTRC("This file extension is not recognized by the editor.\nIf you want to rename it "
			 "anyway, use your operating system's file manager.\nAfter renaming to an unknown "
			 "extension, the file won't be shown in the editor anymore.\nTo make the editor "
			 "recognize this file extension, add it to one of the lists of extensions in Editor "
			 "Settings > Docks > FileSystem."));
	Button* settings_button = unrecognized_ext_dialog->add_button(
		TTRC("Open Editor Settings"), false, "open_editor_settings_docks_filesystem");

	uncollapsed_paths_before_search = Vector<String>();

	tree_update_id = 0;

	history_pos = 0;
	history_max_size = 20;
	history.push_back("res://");

	display_mode = DISPLAY_MODE_TREE_ONLY;
	old_display_mode = DISPLAY_MODE_TREE_ONLY;
	file_list_display_mode = FILE_LIST_DISPLAY_THUMBNAILS;

	add_resource_tooltip_plugin(memnew(EditorTextureTooltipPlugin));
	add_resource_tooltip_plugin(memnew(EditorAudioStreamTooltipPlugin));
}

FileSystemDock::~FileSystemDock() { singleton = nullptr; }


