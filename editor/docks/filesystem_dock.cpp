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

void FileSystemDock::_split_dragged(int p_offset)
{
	if (split_box->is_vertical()) {
		split_box_offset_v = p_offset;
	}
	else {
		split_box_offset_h = p_offset;
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

ScriptCreateDialog* FileSystemDock::get_script_create_dialog() const { return make_script_dialog; }

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

// TODO: Could use a unit test.
const HashMap<String, Color>& FileSystemDock::get_folder_colors() const { return folder_colors; }

FileSystemDock::~FileSystemDock() { singleton = nullptr; }

