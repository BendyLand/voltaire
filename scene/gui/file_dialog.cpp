/**************************************************************************/
/*  file_dialog.cpp                                                       */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "file_dialog.compat.inc"
#include "file_dialog.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

void FileDialog::popup_file_dialog()
{
	popup_centered_clamped(Vector2(1050, 700) * get_theme_default_base_scale(), 0.8f);
	_focus_file_text();
}

void FileDialog::_focus_file_text()
{
	int lp = filename_edit->get_text().rfind_char('.');
	if (lp != -1) {
		filename_edit->select(0, lp);
		if (filename_edit->is_inside_tree() && !is_part_of_edited_scene()) {
			filename_edit->grab_focus();
		}
	}
}

bool FileDialog::_can_use_native_popup() const
{
	if (access == ACCESS_RESOURCES || access == ACCESS_USERDATA || options.size() > 0) {
		return DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA);
	}
	return DisplayServer::get_singleton()->has_feature(
		DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE);
}

Vector2i FileDialog::_get_list_mode_icon_size() const { return theme_cache.file->get_size(); }

void FileDialog::_popup_base(const Rect2i& p_screen_rect)
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		ConfirmationDialog::_popup_base(p_screen_rect);
		return;
	}
#endif

	if (_should_use_native_popup()) {
		_native_popup();
	}
	else {
		ConfirmationDialog::_popup_base(p_screen_rect);
	}
}

void FileDialog::_clear_changed_status()
{
	favorites_changed = false;
	recents_changed = false;
}

void FileDialog::set_visible(bool p_visible)
{
	if (p_visible) {
		_update_option_controls();
	}

#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		ConfirmationDialog::set_visible(p_visible);
		return;
	}
#endif

	if (_should_use_native_popup()) {
		if (p_visible) {
			_native_popup();
		}
	}
	else {
		ConfirmationDialog::set_visible(p_visible);
	}
}

bool FileDialog::_should_use_native_popup() const
{
	return _can_use_native_popup() && (use_native_dialog || OS::get_singleton()->is_sandboxed());
}

Vector<String> FileDialog::get_selected_files() const
{
	const String current_dir = dir_access->get_current_dir();
	Vector<String> list;
	for (int idx : file_list->get_selected_items()) {
		list.push_back(current_dir.path_join(file_list->get_item_text(idx)));
	}
	return list;
}

void FileDialog::_dir_submitted(String p_dir)
{
	String new_dir = OS::get_singleton()->expand_path(p_dir);
#ifdef WINDOWS_ENABLED
	if (root_prefix.is_empty() && drives->is_visible() && !new_dir.is_network_share_path() &&
		new_dir.is_absolute_path() && new_dir.find(":/") == -1 && new_dir.find(":\\") == -1) {
		// Non network path without X:/ prefix on Windows, add drive letter.
		new_dir = drives->get_popup()
					  ->get_item_metadata(selected_drive)
					  .
					  operator Dictionary()["path"]
					  .
					  operator String()
					  .path_join(new_dir);
	}
#endif
	if (!root_prefix.is_empty()) {
		new_dir = root_prefix.path_join(new_dir);
	}
	_change_dir(new_dir);
	if (mode != FILE_MODE_SAVE_FILE) {
		filename_edit->set_text("");
	}
	_push_history();
}

void FileDialog::_post_popup()
{
	ConfirmationDialog::_post_popup();
	if (mode == FILE_MODE_SAVE_FILE) {
		filename_edit->grab_focus(true);
	}
	else {
		file_list->grab_focus(true);
	}

	// For open dir mode, deselect all items on file dialog open.
	if (mode == FILE_MODE_OPEN_DIR) {
		deselect_all();
		file_box->set_visible(false);
	}
	else {
		file_box->set_visible(true);
	}

	local_history.clear();
	local_history_pos = -1;
	_push_history();
}

void FileDialog::_cancel_pressed()
{
	filename_edit->set_text("");
	hide();
}

void FileDialog::_go_up()
{
	_change_dir(get_current_dir().trim_suffix("/").get_base_dir());
	_push_history();
}

int FileDialog::_get_selected_file_idx()
{
	const PackedInt32Array selected = file_list->get_selected_items();
	return selected.is_empty() ? -1 : selected[0];
}

void FileDialog::update_file_name()
{
	int idx = filter->get_selected() - 1;
	if ((idx == -1 && filter->get_item_count() == 2) ||
		(filter->get_item_count() > 2 && idx >= 0 && idx < filter->get_item_count() - 2)) {
		if (idx == -1) {
			idx += 1;
		}
		String filter_str = filters[idx];
		String file_str = filename_edit->get_text();
		String base_name = file_str.get_basename();
		Vector<String> filter_substr = filter_str.split(";");
		if (filter_substr.size() >= 2) {
			file_str = base_name + "." + filter_substr[0].strip_edges().get_extension().to_lower();
		}
		else {
			file_str = base_name + "." + filter_str.strip_edges().get_extension().to_lower();
		}
		filename_edit->set_text(file_str);
	}
}

void FileDialog::_empty_clicked(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button == MouseButton::RIGHT) {
		_popup_menu(p_pos, -1);
	}
	else if (p_button == MouseButton::LEFT) {
		deselect_all();
	}
}

void FileDialog::_item_clicked(int p_item, const Vector2& p_pos, MouseButton p_button)
{
	if (p_button == MouseButton::RIGHT) {
		_popup_menu(p_pos, p_item);
	}
}

void FileDialog::_filter_selected(int)
{
	update_file_name();
	update_file_list();
}

void FileDialog::_file_list_select_first()
{
	if (file_list->get_item_count() > 0) {
		file_list->select(0);
		_file_list_selected(0);
	}
}

void FileDialog::_delete_confirm()
{
	Error err = OS::get_singleton()->move_to_trash(_get_item_path(_get_selected_file_idx()));
	if (err == OK) {
		invalidate();
		_dir_contents_changed();
	}
}

void FileDialog::update_customization()
{
	_update_make_dir_visible();
	show_hidden->set_visible(customization_flags[CUSTOMIZATION_HIDDEN_FILES]);
	layout_container->set_visible(customization_flags[CUSTOMIZATION_LAYOUT]);
	layout_separator->set_visible(customization_flags[CUSTOMIZATION_FILE_FILTER] ||
								  customization_flags[CUSTOMIZATION_FILE_SORT]);
	show_filename_filter_button->set_visible(customization_flags[CUSTOMIZATION_FILE_FILTER]);
	file_sort_button->set_visible(customization_flags[CUSTOMIZATION_FILE_SORT]);
	show_hidden_separator->set_visible(customization_flags[CUSTOMIZATION_HIDDEN_FILES] &&
									   (customization_flags[CUSTOMIZATION_LAYOUT] ||
										   customization_flags[CUSTOMIZATION_FILE_FILTER] ||
										   customization_flags[CUSTOMIZATION_FILE_SORT]));
	favorite_button->set_visible(customization_flags[CUSTOMIZATION_FAVORITES]);
	favorite_vbox->set_visible(customization_flags[CUSTOMIZATION_FAVORITES]);
	recent_vbox->set_visible(customization_flags[CUSTOMIZATION_RECENT]);
}

void FileDialog::clear_filename_filter()
{
	set_filename_filter("");
	update_filename_filter_gui();
	invalidate();
}

void FileDialog::update_filename_filter_gui()
{
	filename_filter_box->set_visible(show_filename_filter);
	if (!show_filename_filter) {
		file_name_filter.clear();
	}
	if (filename_filter->get_text() == file_name_filter) {
		return;
	}
	filename_filter->set_text(file_name_filter);
}

void FileDialog::update_filename_filter()
{
	if (filename_filter->get_text() == file_name_filter) {
		return;
	}
	set_filename_filter(filename_filter->get_text());
}

void FileDialog::clear_filters()
{
	filters.clear();
	update_filters();
	invalidate();
}

void FileDialog::add_filter(
	const String& p_filter, const String& p_description, const String& p_mime)
{
	ERR_FAIL_COND_MSG(
		p_filter.begins_with("."), "Filter must be \"filename.extension\", can't start with dot.");
	if (p_description.is_empty() && p_mime.is_empty()) {
		filters.push_back(p_filter);
	}
	else if (p_mime.is_empty()) {
		filters.push_back(vformat("%s ; %s", p_filter, p_description));
	}
	else {
		filters.push_back(vformat("%s ; %s ; %s", p_filter, p_description, p_mime));
	}
	update_filters();
	invalidate();
}

void FileDialog::set_filters(const Vector<String>& p_filters)
{
	if (filters == p_filters) {
		return;
	}
	filters = p_filters;
	update_filters();
	invalidate();
}

Vector<String> FileDialog::get_filters() const { return filters; }

String FileDialog::get_filename_filter() const { return file_name_filter; }

String FileDialog::get_current_dir() const { return full_dir; }

String FileDialog::get_current_file() const { return filename_edit->get_text(); }

String FileDialog::get_current_path() const
{
	return full_dir.path_join(filename_edit->get_text());
}

void FileDialog::set_current_dir(const String& p_dir)
{
	if (p_dir.is_relative_path()) {
		dir_access->change_dir(OS::get_singleton()->get_resource_dir());
	}
	_change_dir(p_dir);

	_push_history();
}

void FileDialog::set_current_file(const String& p_file)
{
	if (filename_edit->get_text() == p_file) {
		return;
	}
	filename_edit->set_text(p_file);
	update_dir();
	invalidate();
	_focus_file_text();
}

void FileDialog::set_current_path(const String& p_path)
{
	if (!p_path.size()) {
		return;
	}

	String path = OS::get_singleton()->expand_path(p_path);

	int pos = MAX(path.rfind_char('/'), path.rfind_char('\\'));
	if (pos == -1) {
		set_current_file(path);
	}
	else {
		String path_dir = path.substr(0, pos);
		String path_file = path.substr(pos + 1);
		set_current_dir(path_dir);
		set_current_file(path_file);
	}
}

void FileDialog::set_root_subfolder(const String& p_root)
{
	root_subfolder = p_root;
	ERR_FAIL_COND_MSG(
		!dir_access->dir_exists(p_root), "root_subfolder must be an existing sub-directory.");

	local_history.clear();
	local_history_pos = -1;

	dir_access->change_dir(root_subfolder);
	if (root_subfolder.is_empty()) {
		root_prefix = "";
	}
	else {
		root_prefix = dir_access->get_current_dir();
	}
	invalidate();
	_update_drives();
	update_dir();
}

String FileDialog::get_root_subfolder() const { return root_subfolder; }

void FileDialog::set_mode_overrides_title(bool p_override) { mode_overrides_title = p_override; }

bool FileDialog::is_mode_overriding_title() const { return mode_overrides_title; }

FileDialog::FileMode FileDialog::get_file_mode() const { return mode; }

void FileDialog::set_display_mode(DisplayMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, DISPLAY_MAX);
	if (display_mode == p_mode) {
		return;
	}
	display_mode = p_mode;

	if (p_mode == DISPLAY_THUMBNAILS) {
		thumbnail_mode_button->set_pressed(true);
		list_mode_button->set_pressed(false);
	}
	else {
		thumbnail_mode_button->set_pressed(false);
		list_mode_button->set_pressed(true);
	}
	invalidate();
}

FileDialog::DisplayMode FileDialog::get_display_mode() const { return display_mode; }

void FileDialog::set_favorite_list(const PackedStringArray& p_favorites)
{
	ERR_FAIL_COND_MSG(Thread::get_caller_id() != Thread::get_main_id(),
		"Setting favorite list can only be done on the main thread.");

	global_favorites.clear();
	global_favorites.reserve(p_favorites.size());
	for (const String& fav : p_favorites) {
		if (fav.ends_with("/")) {
			global_favorites.push_back(fav);
		}
		else {
			global_favorites.push_back(fav + "/");
		}
	}
}

PackedStringArray FileDialog::get_favorite_list()
{
	PackedStringArray ret;
	ERR_FAIL_COND_V_MSG(Thread::get_caller_id() != Thread::get_main_id(), ret,
		"Getting favorite list can only be done on the main thread.");

	ret.resize(global_favorites.size());

	String* fav_write = ret.ptrw();
	int i = 0;
	for (const String& fav : global_favorites) {
		fav_write[i] = fav;
		i++;
	}
	return ret;
}

void FileDialog::set_recent_list(const PackedStringArray& p_recents)
{
	ERR_FAIL_COND_MSG(Thread::get_caller_id() != Thread::get_main_id(),
		"Setting recent list can only be done on the main thread.");

	global_recents.clear();
	global_recents.reserve(p_recents.size());
	for (const String& recent : p_recents) {
		if (recent.ends_with("/")) {
			global_recents.push_back(recent);
		}
		else {
			global_recents.push_back(recent + "/");
		}
	}
}

PackedStringArray FileDialog::get_recent_list()
{
	PackedStringArray ret;
	ERR_FAIL_COND_V_MSG(Thread::get_caller_id() != Thread::get_main_id(), ret,
		"Getting recent list can only be done on the main thread.");

	ret.resize(global_recents.size());

	String* recent_write = ret.ptrw();
	int i = 0;
	for (const String& recent : global_recents) {
		recent_write[i] = recent;
		i++;
	}
	return ret;
}

void FileDialog::set_customization_flag_enabled(Customization p_flag, bool p_enabled)
{
	ERR_FAIL_INDEX(p_flag, CUSTOMIZATION_MAX);
	if (customization_flags[p_flag] == p_enabled) {
		return;
	}
	customization_flags[p_flag] = p_enabled;
	update_customization();
}

bool FileDialog::is_customization_flag_enabled(Customization p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, CUSTOMIZATION_MAX, false);
	return customization_flags[p_flag];
}

void FileDialog::set_access(Access p_access)
{
	ERR_FAIL_INDEX(p_access, 3);
	if (access == p_access) {
		return;
	}
	access = p_access;
	root_prefix = "";
	root_subfolder = "";

	switch (p_access) {
	case ACCESS_FILESYSTEM: {
		dir_access = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
#ifdef ANDROID_ENABLED
		set_current_dir(OS::get_singleton()->get_system_dir(OS::SYSTEM_DIR_DESKTOP));
#endif
	} break;
	case ACCESS_RESOURCES: {
		dir_access = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	} break;
	case ACCESS_USERDATA: {
		dir_access = DirAccess::create(DirAccess::ACCESS_USERDATA);
	} break;
	}
	_update_drives();
	invalidate();
	update_filters();
	update_dir();
	_update_favorite_list();
	_update_recent_list();
}

void FileDialog::_invalidate()
{
	if (!is_invalidating) {
		return;
	}

	update_file_list();

	if (ensure_visible_after_invalidating) {
		file_list->ensure_current_is_visible();
		ensure_visible_after_invalidating = false;
	}
	is_invalidating = false;
}

void FileDialog::_update_make_dir_visible()
{
	can_create_folders = customization_flags[CUSTOMIZATION_CREATE_FOLDER] &&
						 mode != FILE_MODE_OPEN_FILE && mode != FILE_MODE_OPEN_FILES;
	make_dir_container->set_visible(can_create_folders);
}

FileDialog::Access FileDialog::get_access() const { return access; }

void FileDialog::_make_dir_confirm()
{
	Error err = dir_access->make_dir(new_dir_name->get_text().strip_edges());
	if (err == OK) {
		_dir_contents_changed();
		_change_dir(new_dir_name->get_text().strip_edges());
		update_filters();
		_push_history();
	}
	else {
		mkdirerr->popup_centered(Size2(250, 50));
	}
	new_dir_name->set_text(""); // reset label
}

void FileDialog::_make_dir()
{
	make_dir_dialog->popup_centered(Size2(250, 80));
	new_dir_name->grab_focus();
}

void FileDialog::_change_dir(const String& p_new_dir)
{
	if (access == ACCESS_RESOURCES && p_new_dir.begins_with("user://")) {
		ERR_FAIL_MSG("Can't change to userdata folder when using ACCESS_RESOURCES.");
	}
	else if (access == ACCESS_USERDATA && p_new_dir.begins_with("res://")) {
		ERR_FAIL_MSG("Can't change to resources folder when using ACCESS_USERDATA.");
	}

	if (root_prefix.is_empty()) {
		dir_access->change_dir(p_new_dir);
	}
	else {
		String old_dir = dir_access->get_current_dir();
		dir_access->change_dir(p_new_dir);
		if (!dir_access->get_current_dir().begins_with(root_prefix)) {
			dir_access->change_dir(old_dir);
			return;
		}
	}

	invalidate();
	update_dir();
}

void FileDialog::_sort_option_selected(int p_option)
{
	for (int i = 0; i < int(FileSortOption::MAX); i++) {
		file_sort_button->get_popup()->set_item_checked(i, (i == p_option));
	}
	file_sort = FileSortOption(p_option);
	ensure_visible_after_invalidating = true;
	invalidate();
}

void FileDialog::_favorite_pressed()
{
	String directory = get_current_dir();
	if (!directory.ends_with("/")) {
		directory += "/";
	}

	bool found = false;
	for (const String& name : global_favorites) {
		if (!_path_matches_access(name)) {
			continue;
		}

		if (name == directory) {
			found = true;
			break;
		}
	}

	if (found) {
		global_favorites.erase(directory);
	}
	else {
		global_favorites.push_back(directory);
	}
	favorites_changed = true;
	_update_favorite_list();
}

void FileDialog::_save_to_recent()
{
	String directory = get_current_dir();
	if (!directory.ends_with("/")) {
		directory += "/";
	}

	int count = 0;
	for (uint32_t i = 0; i < global_recents.size(); i++) {
		const String& dir = global_recents[i];
		if (!_path_matches_access(dir)) {
			continue;
		}

		if (dir == directory || count > MAX_RECENTS) {
			global_recents.remove_at(i);
			i--;
		}
		else {
			count++;
		}
	}
	global_recents.insert(0, directory);
	recents_changed = true;

	_update_recent_list();
}

bool FileDialog::_path_matches_access(const String& p_path) const
{
	bool is_res = p_path.begins_with("res://");
	bool is_user = p_path.begins_with("user://");
	if (access == ACCESS_RESOURCES) {
		return is_res;
	}
	else if (access == ACCESS_USERDATA) {
		return is_user;
	}
	return !is_res && !is_user;
}

String FileDialog::get_option_name(int p_option) const
{
	ERR_FAIL_INDEX_V(p_option, options.size(), String());
	return options[p_option].name;
}

Vector<String> FileDialog::get_option_values(int p_option) const
{
	ERR_FAIL_INDEX_V(p_option, options.size(), Vector<String>());
	return options[p_option].values;
}

int FileDialog::get_option_default(int p_option) const
{
	ERR_FAIL_INDEX_V(p_option, options.size(), -1);
	return options[p_option].default_idx;
}

void FileDialog::set_option_name(int p_option, const String& p_name)
{
	if (p_option < 0) {
		p_option += get_option_count();
	}
	ERR_FAIL_INDEX(p_option, options.size());
	options.write[p_option].name = p_name;
	options_dirty = true;
	if (is_visible()) {
		_update_option_controls();
	}
}

void FileDialog::set_option_values(int p_option, const Vector<String>& p_values)
{
	if (p_option < 0) {
		p_option += get_option_count();
	}
	ERR_FAIL_INDEX(p_option, options.size());
	options.write[p_option].values = p_values;
	if (p_values.is_empty()) {
		options.write[p_option].default_idx = CLAMP(options[p_option].default_idx, 0, 1);
	}
	else {
		options.write[p_option].default_idx =
			CLAMP(options[p_option].default_idx, 0, options[p_option].values.size() - 1);
	}
	options_dirty = true;
	if (is_visible()) {
		_update_option_controls();
	}
}

void FileDialog::set_option_default(int p_option, int p_index)
{
	if (p_option < 0) {
		p_option += get_option_count();
	}
	ERR_FAIL_INDEX(p_option, options.size());
	if (options[p_option].values.is_empty()) {
		options.write[p_option].default_idx = CLAMP(p_index, 0, 1);
	}
	else {
		options.write[p_option].default_idx =
			CLAMP(p_index, 0, options[p_option].values.size() - 1);
	}
	options_dirty = true;
	if (is_visible()) {
		_update_option_controls();
	}
}

void FileDialog::add_option(const String& p_name, const Vector<String>& p_values, int p_index)
{
	Option opt;
	opt.name = p_name;
	opt.values = p_values;
	if (opt.values.is_empty()) {
		opt.default_idx = CLAMP(p_index, 0, 1);
	}
	else {
		opt.default_idx = CLAMP(p_index, 0, opt.values.size() - 1);
	}
	options.push_back(opt);
	options_dirty = true;
	if (is_visible()) {
		_update_option_controls();
	}
}

int FileDialog::get_option_count() const { return options.size(); }

void FileDialog::set_show_hidden_files(bool p_show)
{
	if (show_hidden_files == p_show) {
		return;
	}
	show_hidden->set_pressed_no_signal(p_show);
	show_hidden_files = p_show;
	invalidate();
}

bool FileDialog::get_show_filename_filter() const { return show_filename_filter; }

bool FileDialog::is_showing_hidden_files() const { return show_hidden_files; }

void FileDialog::set_default_show_hidden_files(bool p_show) { default_show_hidden_files = p_show; }

void FileDialog::set_default_display_mode(DisplayMode p_mode) { default_display_mode = p_mode; }

void FileDialog::set_use_native_dialog(bool p_native)
{
	use_native_dialog = p_native;

#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return;
	}
#endif

	// Replace the built-in dialog with the native one if it's currently visible.
	if (is_inside_tree() && is_visible() && _should_use_native_popup()) {
		ConfirmationDialog::set_visible(false);
		_native_popup();
	}
}

bool FileDialog::get_use_native_dialog() const { return use_native_dialog; }

FileDialog::~FileDialog()
{
	if (unregister_func) {
		unregister_func(this);
	}
}


