/**************************************************************************/
/*  dialogs.cpp                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "dialogs.h"
#include "core/config/engine.h"
#include "scene/gui/line_edit.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/button.h"
#include "scene/gui/scroll_container.h"
#include "servers/display/display_server.h"
#include "popup.h"
#include "scene/gui/panel.h"
#include "scene/resources/style_box.h"
#include "core/input/input.h"
#include "core/string/fuzzy_search.h"
#include "popup_menu.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/button.h"
#include "scene/main/timer.h"

#ifdef TOOLS_ENABLED
#include "core/config/project_settings.h"
#endif

void DialogChrome::init_controls(Window* p_owner)
{
	p_owner->set_wrap_controls(true);
	p_owner->set_visible(false);
	p_owner->set_transient(true);
	p_owner->set_exclusive(true);
	p_owner->set_clamp_to_embedder(true);
	p_owner->set_keep_title_visible(true);

	bg_panel = memnew(Panel);
	p_owner->add_child(bg_panel, false, Node::INTERNAL_MODE_FRONT);

	message_label = memnew(Label);
	message_label->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	p_owner->add_child(message_label, false, Node::INTERNAL_MODE_FRONT);

	buttons_hbox = Control::create_hbox();
	p_owner->add_child(buttons_hbox, false, Node::INTERNAL_MODE_FRONT);

	ok_button = memnew(Button);
	ok_button->set_text(default_ok_text);
	buttons_hbox->add_child(ok_button);

	if (has_cancel_button) {
		cancel = memnew(Button);
		cancel->set_text("Cancel");
		buttons_hbox->add_child(cancel);
		if (swap_cancel_ok) {
			buttons_hbox->move_child(cancel, 0);
		}
	}
}

bool DialogChrome::handle_escape_input(const InputEvent& p_event) const
{
	return close_on_escape && p_event.is_action_pressed(SNAME("ui_close_dialog"), false, true);
}

bool DialogChrome::should_cancel_on_parent_focus(const Window* p_owner) const
{
	return popped_up && !p_owner->is_exclusive() && p_owner->get_flag(Window::FLAG_POPUP);
}

void DialogChrome::update_child_rects(Window*) {}

void DialogChrome::update_ok_text(Window* p_owner)
{
	if (!ok_button) {
		return;
	}
	String prev_text = ok_button->get_text();
	String new_text = ok_text.is_empty() ? default_ok_text : ok_text;

	if (new_text == prev_text) {
		return;
	}
	ok_button->set_text(new_text);

	p_owner->child_controls_changed();
	if (p_owner->is_visible()) {
		update_child_rects(p_owner);
	}
}

void DialogChrome::set_default_ok_text(Window* p_owner, const String& p_text)
{
	default_ok_text = p_text;
	update_ok_text(p_owner);
}

void DialogChrome::set_cancel_button_text(const String& p_cancel_button_text)
{
	if (cancel) {
		cancel->set_text(p_cancel_button_text);
	}
}

String DialogChrome::get_cancel_button_text() const
{
	return cancel ? cancel->get_text() : String();
}

void DialogChrome::set_text(Window* p_owner, const String& p_text)
{
	if (!message_label || message_label->get_text() == p_text) {
		return;
	}

	message_label->set_text(p_text);

	p_owner->child_controls_changed();
	if (p_owner->is_visible()) {
		update_child_rects(p_owner);
	}
}

String DialogChrome::get_text() const
{
	return message_label ? message_label->get_text() : String();
}

void DialogChrome::set_autowrap(bool p_autowrap)
{
	if (message_label) {
		message_label->set_autowrap_mode(
			p_autowrap ? TextServer::AUTOWRAP_WORD : TextServer::AUTOWRAP_OFF);
	}
}

bool DialogChrome::has_autowrap() const
{
	return message_label && message_label->get_autowrap_mode() != TextServer::AUTOWRAP_OFF;
}

void DialogChrome::set_ok_button_text(Window* p_owner, const String& p_ok_button_text)
{
	ok_text = p_ok_button_text;
	update_ok_text(p_owner);
}

Size2 Dialog::_get_contents_minimum_size() const { return Size2(); }

void Dialog::_input_from_window(const InputEvent& p_event)
{
	if (chrome.handle_escape_input(p_event)) {
		_cancel_pressed();
	}
	Window::_input_from_window(p_event);
}

void Dialog::_parent_focused()
{
	if (chrome.should_cancel_on_parent_focus(this)) {
		_cancel_pressed();
	}
}

void Dialog::_text_submitted(const String& p_text)
{
	if (get_ok_button() && get_ok_button()->is_disabled()) {
		return;
	}
	_ok_pressed();
}

void Dialog::_post_popup()
{
	Window::_post_popup();
	chrome.post_popup();
}

void Dialog::_notification(int p_what)
{
	if (p_what == NOTIFICATION_VISIBILITY_CHANGED && !is_visible()) {
		chrome.popped_up = false;
	}
}

void Dialog::set_swap_cancel_ok(bool p_swap) { DialogChrome::swap_cancel_ok = p_swap; }

void Dialog::_cancel_pressed()
{
	hide();
}

void Dialog::_ok_pressed()
{
	if (chrome.get_hide_on_ok()) {
		hide();
	}
}

Dialog::Dialog(bool p_with_cancel) : chrome(p_with_cancel)
{
	chrome.init_controls(this);
}

// --- file_dialog.cpp ---

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
		return DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE_EXTRA);
	}
	return DisplayServer::has_feature(
		DisplayServerEnums::FEATURE_NATIVE_DIALOG_FILE);
}

Vector2i FileDialog::_get_list_mode_icon_size() const { return theme_cache.file->get_size(); }

void FileDialog::_input_from_window(const InputEvent& p_event)
{
	if (chrome.handle_escape_input(p_event)) {
		_cancel_pressed();
	}
	Window::_input_from_window(p_event);
}

void FileDialog::_post_popup()
{
	Window::_post_popup();
	chrome.post_popup();
}

void FileDialog::_notification(int p_what)
{
	if (p_what == NOTIFICATION_VISIBILITY_CHANGED && !is_visible()) {
		chrome.popped_up = false;
	}
}

void FileDialog::_popup_base(const Rect2i& p_screen_rect)
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		Window::_popup_base(p_screen_rect);
		return;
	}
#endif

	if (_should_use_native_popup()) {
		_native_popup();
	}
	else {
		Window::_popup_base(p_screen_rect);
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
		Window::set_visible(p_visible);
		return;
	}
#endif

	if (_should_use_native_popup()) {
		if (p_visible) {
			_native_popup();
		}
	}
	else {
		Window::set_visible(p_visible);
	}
}

bool FileDialog::_should_use_native_popup() const
{
	return _can_use_native_popup() && (use_native_dialog || OS::is_sandboxed());
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
	String new_dir = OS::expand_path(p_dir);
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
	Error err = OS::move_to_trash(_get_item_path(_get_selected_file_idx()));
	if (err == OK) {
		invalidate();
		_dir_contents_changed();
	}
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
		dir_access->change_dir(OS::get_resource_dir());
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

	String path = OS::expand_path(p_path);

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

void FileDialog::set_file_mode(FileMode p_mode) {}

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
		set_current_dir(OS::get_system_dir(OS::SYSTEM_DIR_DESKTOP));
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
	new_dir_name->set_text(""); // reset label
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
		Window::set_visible(false);
		_native_popup();
	}
}

bool FileDialog::get_use_native_dialog() const { return use_native_dialog; }

FileDialog::FileDialog()
{
	for (int i = 0; i < CUSTOMIZATION_MAX; i++) {
		customization_flags[i] = true;
	}
	chrome.init_controls(this);
}

FileDialog::~FileDialog()
{
	if (unregister_func) {
		unregister_func(this);
	}
}

void FileDialog::invalidate() {}

void FileDialog::_update_option_controls() {}

void FileDialog::update_filters() {}

void FileDialog::update_dir() {}

void FileDialog::_push_history() {}

void FileDialog::_native_popup() {}

void FileDialog::_update_recent_list() {}

void FileDialog::update_file_list() {}

void FileDialog::_update_favorite_list() {}

void FileDialog::_update_drives(bool) {}

void FileDialog::_popup_menu(Vector2 const&, int) {}

void FileDialog::set_filename_filter(String const&) {}

String FileDialog::_get_item_path(int) const { return String(); }

void FileDialog::_file_list_selected(int) {}

void FileDialog::deselect_all() {}

void FileDialog::_item_menu_id_pressed(int p_id) {}

// --- file_dialog.compat.inc ---

#ifndef DISABLE_DEPRECATED


void FileDialog::_add_filter_bind_compat_111439(const String& p_filter, const String& p_description)
{
	add_filter(p_filter, p_description, "");
}

void FileDialog::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- popup.cpp ---

void PopupTracker::configure_window(Window* p_window)
{
	p_window->set_wrap_controls(true);
	p_window->set_visible(false);
	p_window->set_transient(true);
	p_window->set_flag(Window::FLAG_BORDERLESS, true);
	p_window->set_flag(Window::FLAG_RESIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_MINIMIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_MAXIMIZE_DISABLED, true);
	p_window->set_flag(Window::FLAG_POPUP, true);
	p_window->set_flag(Window::FLAG_POPUP_WM_HINT, true);
}

void PopupTracker::initialize_visible_parents(Window* p_window)
{
	if (p_window->is_embedded()) {
		visible_parents.clear();

		Window* parent_window = p_window;
		while (parent_window) {
			parent_window = parent_window->get_parent_visible_window();
			if (parent_window) {
				visible_parents.push_back(parent_window);
			}
		}
	}
}

void PopupTracker::deinitialize_visible_parents(Window* p_window)
{
	visible_parents.clear();
}

void PopupTracker::post_popup()
{
	popped_up = true;
}

bool PopupTracker::handle_cancel_input(const Window* p_window, const InputEvent& p_event)
{
	if (p_window->get_flag(Window::FLAG_POPUP) &&
		p_event.is_action_pressed(SNAME("ui_cancel"), false, true)) {
		hide_reason = HIDE_REASON_CANCELED; // ESC pressed, mark as canceled unconditionally.
		return true;
	}
	return false;
}

bool PopupTracker::handle_parent_focused(const Window* p_window)
{
	if (popped_up && p_window->get_flag(Window::FLAG_POPUP)) {
		if (hide_reason == HIDE_REASON_NONE) {
			hide_reason = HIDE_REASON_UNFOCUSED;
		}
		return true;
	}
	return false;
}

void PopupTracker::close_pressed(Window* p_window)
{
	popped_up = false;
	deinitialize_visible_parents(p_window);
	p_window->hide();
}

Rect2i PopupTracker::adjust_popup_rect(const Window* p_window)
{
	ERR_FAIL_COND_V(!p_window->is_inside_tree(), Rect2());
	Rect2i parent_rect = p_window->get_usable_parent_rect();

	if (parent_rect == Rect2i()) {
		return Rect2i();
	}

	Rect2i current(p_window->get_position(), p_window->get_size());

	if (!p_window->is_embedded() &&
		DisplayServer::has_feature(DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		// We're fine as is, the Display Server will take care of that for us.
		return current;
	}

	if (current.position.x + current.size.x > parent_rect.position.x + parent_rect.size.x) {
		current.position.x = parent_rect.position.x + parent_rect.size.x - current.size.x;
	}

	if (current.position.x < parent_rect.position.x) {
		current.position.x = parent_rect.position.x;
	}

	if (current.position.y + current.size.y > parent_rect.position.y + parent_rect.size.y) {
		current.position.y = parent_rect.position.y + parent_rect.size.y - current.size.y;
	}

	if (current.position.y < parent_rect.position.y) {
		current.position.y = parent_rect.position.y;
	}

	if (current.size.y > parent_rect.size.y) {
		current.size.y = parent_rect.size.y;
	}

	if (current.size.x > parent_rect.size.x) {
		current.size.x = parent_rect.size.x;
	}

	// Early out if max size not set.
	Size2i popup_max_size = p_window->get_max_size();
	if (popup_max_size <= Size2()) {
		return current;
	}

	if (current.size.x > popup_max_size.x) {
		current.size.x = popup_max_size.x;
	}

	if (current.size.y > popup_max_size.y) {
		current.size.y = popup_max_size.y;
	}

	return current;
}

void Popup::set_panel_enabled(bool p_enabled)
{
	if (is_panel == p_enabled) {
		return;
	}
	is_panel = p_enabled;
	if (is_panel) {
		if (!panel) {
			panel = memnew(Panel);
			panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			add_child(panel, false, INTERNAL_MODE_FRONT);
		}
	}
	else if (panel) {
		remove_child(panel);
		memdelete(panel);
		panel = nullptr;
	}
}

void Popup::_input_from_window(const InputEvent& p_event)
{
	if (is_panel) {
		if (close_on_accept && p_event.is_action_pressed(SNAME("ui_accept"), false, true)) {
			_close_pressed();
		}
		if (p_event.is_valid()) {
			if (!get_flag(FLAG_POPUP)) {
				return;
			}

			// Hide it if the shadows have been clicked.
			if (panel && p_event.type == InputEventType::MOUSE_BUTTON && p_event.is_pressed() &&
				p_event.mouse_button.button_index == MouseButton::LEFT) {
				Rect2 panel_area = panel->get_global_rect();
				float win_scale = get_content_scale_factor();
				panel_area.position *= win_scale;
				panel_area.size *= win_scale;
				if (!panel_area.has_point(p_event.mouse_button.position)) {
					_close_pressed();
				}
			}
		}
		else {
			WARN_PRINT_ONCE("Popup has received an invalid InputEvent. Consider filtering out "
							"invalid events.");
		}
	}

	if (tracker.handle_cancel_input(this, p_event)) {
		_close_pressed();
	}
	Window::_input_from_window(p_event);
}

void Popup::_parent_focused()
{
	if (tracker.handle_parent_focused(this)) {
		_close_pressed();
	}
}

void Popup::_post_popup()
{
	Window::_post_popup();
	tracker.post_popup();
}

Rect2i Popup::_popup_adjust_rect() const
{
	Rect2i current = PopupTracker::adjust_popup_rect(this);
	if (!is_panel || !panel || current == Rect2i()) {
		return current;
	}

	pre_popup_rect = current;

	_update_shadow_offsets();
	_update_child_rects();

	if (is_layout_rtl()) {
		current.position -= Vector2(-panel->get_offset(SIDE_RIGHT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	else {
		current.position -= Vector2(panel->get_offset(SIDE_LEFT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	current.size += Vector2(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
						panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM)) *
					get_content_scale_factor();

	return current;
}

void Popup::_update_shadow_offsets() const
{
	if (!panel) {
		return;
	}

	if (!DisplayServer::is_window_transparency_available() && !is_embedded()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const Ref<StyleBox> sb = theme_cache.panel_style;
	if (sb.is_null() || sb->get_style_type() != StyleBox::STYLE_FLAT) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const int shadow_size = sb->get_shadow_size();
	if (shadow_size == 0) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	// Offset the background panel so it leaves space inside the window for the shadows to be drawn.
	const Point2 shadow_offset = sb->get_shadow_offset();
	if (is_layout_rtl()) {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size + shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size + shadow_offset.x));
	}
	else {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size - shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size - shadow_offset.x));
	}
	panel->set_offset(SIDE_TOP, MAX(0, shadow_size - shadow_offset.y));
	panel->set_offset(SIDE_BOTTOM, MIN(0, -shadow_size - shadow_offset.y));
}

void Popup::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
			tracker.hide_reason = HIDE_REASON_NONE;
			tracker.initialize_visible_parents(this);
		}
		else {
			tracker.deinitialize_visible_parents(this);
			tracker.popped_up = false;
		}

		if (is_panel && panel) {
			if (!is_visible()) {
				// Remove the extra space used by the shadows, so they can be ignored when the popup
				// is hidden.
				panel->set_offsets_preset(
					Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
				_update_child_rects();

				if (pre_popup_rect != Rect2i()) {
					set_position(pre_popup_rect.position);
					set_size(pre_popup_rect.size);

					pre_popup_rect = Rect2i();
				}
			}
			else if (pre_popup_rect == Rect2i()) {
				// The popup was made visible directly (without `popup_*()`), so just update the
				// offsets without touching the rect.
				_update_shadow_offsets();
				_update_child_rects();
			}
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		if (is_panel && panel) {
			panel->add_theme_style_override(SceneString(panel), theme_cache.panel_style.ptr());

			if (is_visible()) {
				_update_shadow_offsets();
			}

			_update_child_rects();
		}
	} break;

	case Control::NOTIFICATION_TRANSLATION_CHANGED:
	case Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		if (is_panel && panel && is_visible()) {
			_update_shadow_offsets();
		}
	} break;

	case NOTIFICATION_WM_SIZE_CHANGED: {
		if (is_panel && panel) {
			_update_child_rects();

			if (is_visible()) {
				const Vector2i offsets =
					Vector2i(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
						panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM));
				// Check if the size actually changed.
				if (pre_popup_rect.size + offsets != get_size()) {
					// Play safe, and stick with the new size.
					pre_popup_rect = Rect2i();
				}
			}
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		tracker.deinitialize_visible_parents(this);
	} break;
	}
}

void Popup::_update_child_rects() const {}

void Popup::_close_pressed()
{
	tracker.close_pressed(this);
}

Size2 Popup::_get_contents_minimum_size() const { return Size2(); }

#ifdef TOOLS_ENABLED
PackedStringArray Popup::get_configuration_warnings() const
{
	PackedStringArray warnings = Window::get_configuration_warnings();

	if (is_panel && !DisplayServer::is_window_transparency_available() &&
		!GLOBAL_GET_CACHED(bool, "display/window/subwindows/embed_subwindows")) {
		Ref<StyleBox> sb = theme_cache.panel_style;
		if (sb.is_valid() && sb->get_style_type() == StyleBox::STYLE_FLAT &&
			(sb->get_shadow_size() > 0 || sb->get_corner_radius(CORNER_TOP_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_TOP_RIGHT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_RIGHT) > 0)) {
			warnings.push_back(RTR(
				"The current theme style has shadows and/or rounded corners for popups, but those "
				"won't display correctly if \"display/window/per_pixel_transparency/allowed\" "
				"isn't enabled in the Project Settings, nor if it isn't supported."));
		}
	}

	return warnings;
}
#endif

Popup::Popup(bool p_panel)
{
	tracker.configure_window(this);
	if (p_panel) {
		set_panel_enabled(true);
	}
}

Popup::~Popup() {}

// --- popup_menu.cpp ---

HashMap<NativeMenu::SystemMenus, PopupMenu*> PopupMenu::system_menus;

bool PopupMenu::_set_item_accelerator(int p_index, const InputEvent& p_ie)
{
	if (p_ie.type != InputEventType::KEY) {
		return false;
	}
	if (p_ie.key.physical_keycode == Key::NONE && p_ie.key.keycode == Key::NONE &&
		p_ie.key.key_label != Key::NONE) {
		NativeMenu::set_item_accelerator(global_menu, p_index, p_ie.get_key_label_with_modifiers());
		return true;
	}
	else if (p_ie.key.keycode != Key::NONE) {
		NativeMenu::set_item_accelerator(global_menu, p_index, p_ie.get_keycode_with_modifiers());
		return true;
	}
	else if (p_ie.key.physical_keycode != Key::NONE) {
		NativeMenu::set_item_accelerator(global_menu, p_index,
			DisplayServer::keyboard_get_keycode_from_physical(
				p_ie.get_physical_keycode_with_modifiers()));
		return true;
	}
	return false;
}

void PopupMenu::_set_item_checkable_type(int p_index, int p_checkable_type) {}

int PopupMenu::_get_item_checkable_type(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, items.size(), Item::CHECKABLE_TYPE_NONE);
	return items[p_index].checkable_type;
}

void PopupMenu::unbind_global_menu()
{
	if (global_menu.is_null()) {
		return;
	}

	if (global_menu == system_menu && system_menus[system_menu_id] == this) {
		system_menus.erase(system_menu_id);
	}

	for (int i = 0; i < items.size(); i++) {
		Item& item = items.write[i];
		if (item.submenu) {
			item.submenu->unbind_global_menu();
			item.submenu_bound = false;
		}
	}
	if (system_menu != global_menu) {
		NativeMenu::free_menu(global_menu);
	}
	else {
		NativeMenu::clear(global_menu);
	}

	system_menu = RID();
	global_menu = RID();
}

bool PopupMenu::is_system_menu() const
{
	return (global_menu == system_menu) && (system_menu_id != NativeMenu::INVALID_MENU_ID);
}

void PopupMenu::add_icon_item(
	const Ref<Texture2D>& p_icon, const String& p_label, int p_id, Key p_accel)
{
}

void PopupMenu::add_item(const String& p_label, int p_id, Key p_accel) {}

void PopupMenu::set_system_menu(NativeMenu::SystemMenus p_system_menu_id)
{
	if (is_inside_tree() && system_menu_id != NativeMenu::INVALID_MENU_ID) {
		unbind_global_menu();
	}
	system_menu_id = p_system_menu_id;
}

void PopupMenu::add_shortcut(
	const Ref<Shortcut>& p_shortcut, int p_id, bool p_global, bool p_allow_echo)
{
}

NativeMenu::SystemMenus PopupMenu::get_system_menu() const { return system_menu_id; }

String PopupMenu::_get_accel_text(const Item& p_item) const
{
	if (p_item.shortcut.is_valid()) {
		return p_item.shortcut->get_as_text();
	}
	else if (p_item.accel != Key::NONE) {
		return keycode_get_string(p_item.accel);
	}
	return String();
}

Size2 PopupMenu::_get_item_icon_size(int p_idx) const
{
	const PopupMenu::Item& item = items[p_idx];
	Size2 icon_size = item.get_icon_size();

	int max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		max_width = theme_cache.icon_max_width;
	}
	if (item.icon_max_width > 0 && (max_width == 0 || item.icon_max_width < max_width)) {
		max_width = item.icon_max_width;
	}

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

Size2 PopupMenu::_get_contents_minimum_size() const
{
	Size2 minsize = theme_cache.panel_style->get_minimum_size();
	minsize.width += scroll_container->get_v_scroll_bar()->get_size().width;
	// Take shadows into account.
	minsize.width += panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT);
	minsize.height += panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM);

	real_t body_max_w = 0.0; // Indentation, text, and submenu arrow.
	real_t icon_max_w = 0.0;
	real_t accel_max_w = 0.0;
	bool has_check_gutter = false;
	bool gutter_compact = theme_cache.gutter_compact;

	for (int i = 0; i < items.size(); i++) {
		_shape_item(i);

		icon_max_w = MAX(_get_item_icon_size(i).width, icon_max_w);

		if (items[i].checkable_type && !items[i].separator) {
			has_check_gutter = true;
			if (items[i].icon.is_valid()) {
				gutter_compact = false;
			}
		}

		if (items[i].accel != Key::NONE ||
			(items[i].shortcut.is_valid() && items[i].shortcut->has_valid_event())) {
			real_t accel_w = theme_cache.h_separation * 2 + items[i].accel_text_buf->get_size().x;
			accel_max_w = MAX(accel_w, accel_max_w);
		}

		real_t body_w = items[i].indent * theme_cache.indent + items[i].text_buf->get_size().x;
		if (items[i].submenu) {
			body_w += theme_cache.submenu->get_width();
		}
		body_max_w = MAX(body_max_w, body_w);

		minsize.height += _get_item_height(i) + theme_cache.v_separation;
	}

	body_max_w =
		theme_cache.item_start_padding + body_max_w + accel_max_w + theme_cache.item_end_padding;

	const int check_w =
		MAX(theme_cache.checked->get_width(), theme_cache.radio_checked->get_width());
	if (gutter_compact) {
		body_max_w += MAX(icon_max_w, check_w) + theme_cache.h_separation;
	}
	else {
		if (icon_max_w > 0) {
			body_max_w += icon_max_w + theme_cache.h_separation;
		}
		if (has_check_gutter) {
			body_max_w += check_w + theme_cache.h_separation;
		}
	}

	if (search_bar->is_visible()) {
		Size2 sb_min_size = search_bar->get_minimum_size();
		minsize.width += MAX(body_max_w, sb_min_size.width);
		minsize.height += sb_min_size.height + theme_cache.search_bar_separation;
	}
	else {
		minsize.width += body_max_w;
	}

	if (is_inside_tree()) {
		int height_limit = get_usable_parent_rect().size.height;
		if (minsize.height > height_limit) {
			minsize.height = height_limit;
		}
	}

	minsize.height = Math::ceil(minsize.height); // Ensures enough height at fractional content
												 // scales to prevent the v_scroll_bar from showing.
	return minsize;
}

int PopupMenu::_get_item_height(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);

	Size2 icon_size = _get_item_icon_size(p_idx);
	int icon_height = icon_size.height;
	if (items[p_idx].checkable_type && !items[p_idx].separator) {
		icon_height = MAX(icon_height,
			MAX(theme_cache.checked->get_height(), theme_cache.radio_checked->get_height()));
	}

	int text_height = items[p_idx].text_buf->get_size().height;
	if (text_height == 0 && !items[p_idx].separator) {
		text_height = theme_cache.font->get_height(theme_cache.font_size);
	}

	int separator_height = 0;
	if (items[p_idx].separator) {
		separator_height = MAX(theme_cache.separator_style->get_minimum_size().height,
			MAX(theme_cache.labeled_separator_left->get_minimum_size().height,
				theme_cache.labeled_separator_right->get_minimum_size().height));
	}

	return MAX(separator_height, MAX(text_height, icon_height));
}

int PopupMenu::_get_items_total_height() const
{
	// Get total height of all items by taking max of icon height and font height
	int items_total_height = 0;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}
		items_total_height += _get_item_height(i) + theme_cache.v_separation;
	}

	return items_total_height;
}

int PopupMenu::_get_mouse_over(const Point2& p_over) const
{
	// Transform to scroll_container local coordinates.
	const Point2 scaled_pos = p_over / get_content_scale_factor();
	const Point2 over_scroll_container =
		scroll_container->get_global_transform_with_canvas().xform_inv(scaled_pos);

	// Check if point is inside the item control as clipped by scroll_container.
	const Rect2 scroll_container_rect = Rect2(Point2(), scroll_container->get_size());
	const Rect2 bounding_rect = scroll_container_rect.intersection(control->get_rect());
	if (!bounding_rect.has_point(over_scroll_container)) {
		return -1;
	}

	// Perform item hit check in control node local space,
	// so we don't need to worry about any of the container theming.
	const float over_control_y = control->get_transform().xform_inv(over_scroll_container).y;
	float bottom_edge = 0;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}
		bottom_edge += theme_cache.v_separation;
		bottom_edge += _get_item_height(i);
		if (bottom_edge > over_control_y) {
			return i;
		}
	}

	return -1;
}

void PopupMenu::_submenu_timeout()
{
	if (mouse_over == submenu_over) {
		_activate_submenu(mouse_over);
	}
}

void PopupMenu::_input_from_window(const InputEvent& p_event)
{
	if (p_event.is_valid()) {
		_input_from_window_internal(p_event);
	}
	else {
		WARN_PRINT_ONCE(
			"PopupMenu has received an invalid InputEvent. Consider filtering out invalid events.");
	}
	if (tracker.handle_cancel_input(this, p_event)) {
		_close_pressed();
	}
	Window::_input_from_window(p_event);
}

bool PopupMenu::_is_mouse_moving_toward_submenu(const Vector2& p_relative, bool p_is_submenu_left,
	const Vector2& p_mouse_position, const Vector<Point2>& p_active_submenu_target_line) const
{
	Vector2 top_target = (p_active_submenu_target_line[0] - p_mouse_position)
							 .rotated(p_is_submenu_left ? -Math::PI * 0.5 : Math::PI * 0.5);
	Vector2 bottom_target = (p_active_submenu_target_line[1] - p_mouse_position)
								.rotated(p_is_submenu_left ? Math::PI * 0.5 : -Math::PI * 0.5);
	// The top_target vector is perpendicular to the vector between the mouse position and the top
	// point of the submenu target line. The dot product is > 0 if the relative vector is +/- 90
	// degrees of the top_vector. The bottom_target vector is perpendicular to the vector between
	// the mouse position and the bottom point of the submenu target line, but pointing in the
	// opposite direction of the top_target vector. Thus, the intersection of top and bottom test
	// semicircles is the area in which the relative vector is deemed to be moving toward the
	// submenu. Without normalization, the comparison is correct by testing only the sign, but would
	// not work against a range of +/- 1.
	return bottom_target.dot(p_relative) > 0 && top_target.dot(p_relative) > 0;
}

void PopupMenu::_draw_items()
{
	control->set_custom_minimum_size(Size2(0, _get_items_total_height()));
	RID ci = control->get_canvas_item();

	// Space between the item content and the sides of popup menu.
	bool rtl = control->is_layout_rtl();
	// In Item::checkable_type enum order (less the non-checkable member), with disabled repeated at
	// the end.
	Ref<Texture2D> check[] = {theme_cache.checked, theme_cache.radio_checked,
		theme_cache.checked_disabled, theme_cache.radio_checked_disabled};
	Ref<Texture2D> uncheck[] = {theme_cache.unchecked, theme_cache.radio_unchecked,
		theme_cache.unchecked_disabled, theme_cache.radio_unchecked_disabled};
	Ref<Texture2D> indeterminate[] = {
		theme_cache.indeterminate, theme_cache.indeterminate_disabled};
	Ref<Texture2D> submenu;
	if (rtl) {
		submenu = theme_cache.submenu_mirrored;
	}
	else {
		submenu = theme_cache.submenu;
	}

	float display_width = control->get_size().width;

	// Find the widest icon and whether any items have a checkbox, and store the offsets for each.
	real_t icon_max_w = 0.0;
	real_t check_max_w = 0.0;
	bool has_check_gutter = false;
	bool gutter_compact = theme_cache.gutter_compact;
	for (int i = 0; i < items.size(); i++) {
		if (items[i].separator || !items[i].visible) {
			continue;
		}

		icon_max_w = MAX(_get_item_icon_size(i).width, icon_max_w);

		if (items[i].checkable_type) {
			has_check_gutter = true;
			if (items[i].icon.is_valid()) {
				gutter_compact = false;
			}
		}
	}
	if (has_check_gutter) {
		for (int i = 0; i < 4; i++) {
			check_max_w = MAX(check_max_w, check[i]->get_width());
			check_max_w = MAX(check_max_w, uncheck[i]->get_width());
		}
	}

	Point2 ofs;

	// Loop through all items and draw each.
	bool first_visible = true;
	for (int i = 0; i < items.size(); i++) {
		if (!items[i].visible) {
			continue;
		}

		// For the first visible item only add half a separation. For all other items, add a whole
		// separation to the offset.
		ofs.y += first_visible ? theme_cache.v_separation / 2 : theme_cache.v_separation;
		first_visible = false;

		_shape_item(i);

		Point2 item_ofs = ofs;
		Size2 icon_size = _get_item_icon_size(i);
		float h = _get_item_height(i);
		if ((active_submenu_index == -1 && i == mouse_over) || i == active_submenu_index) {
			theme_cache.hover_style->draw(
				ci, Rect2(item_ofs + Point2(0, -theme_cache.v_separation / 2),
						Size2(display_width, h + theme_cache.v_separation)));
		}

		String text = items[i].xl_text;

		// Separator
		item_ofs.x += items[i].indent * theme_cache.indent;
		if (items[i].separator) {
			if (!text.is_empty() || items[i].icon.is_valid()) {
				int content_size =
					items[i].text_buf->get_size().width + theme_cache.h_separation * 2;
				if (items[i].icon.is_valid()) {
					content_size += icon_size.width + theme_cache.h_separation;
				}

				int content_center = display_width / 2;
				int content_left = content_center - content_size / 2;
				int content_right = content_center + content_size / 2;
				if (content_left > item_ofs.x) {
					int sep_h = theme_cache.labeled_separator_left->get_minimum_size().height;
					int sep_ofs = Math::floor((h - sep_h) / 2.0);
					theme_cache.labeled_separator_left->draw(
						ci, Rect2(item_ofs + Point2(0, sep_ofs),
								Size2(MAX(0, content_left - item_ofs.x), sep_h)));
				}
				if (content_right < display_width) {
					int sep_h = theme_cache.labeled_separator_right->get_minimum_size().height;
					int sep_ofs = Math::floor((h - sep_h) / 2.0);
					theme_cache.labeled_separator_right->draw(
						ci, Rect2(Point2(content_right, item_ofs.y + sep_ofs),
								Size2(MAX(0, display_width - content_right), sep_h)));
				}
			}
			else {
				int sep_h = theme_cache.separator_style->get_minimum_size().height;
				int sep_ofs = Math::floor((h - sep_h) / 2.0);
				theme_cache.separator_style->draw(
					ci, Rect2(item_ofs + Point2(0, sep_ofs), Size2(display_width, sep_h)));
			}
		}

		Color icon_color = items[i].icon_modulate;

		// For non-separator items, add some padding for the content.
		if (!items[i].separator) {
			item_ofs.x += theme_cache.item_start_padding;
		}

		// Checkboxes
		if (items[i].checkable_type && !items[i].separator) {
			int disabled = int(items[i].disabled) * 2;
			Texture2D* icon;
			if (items[i].indeterminate) {
				icon = indeterminate[disabled].ptr();
			}
			else {
				icon = (items[i].checked ? check[items[i].checkable_type - 1 + disabled]
										 : uncheck[items[i].checkable_type - 1 + disabled])
						   .ptr();
			}

			if (rtl) {
				icon->draw(ci,
					Size2(control->get_size().width - item_ofs.x - icon->get_width(), item_ofs.y) +
						Point2(0, Math::floor((h - icon->get_height()) / 2.0)),
					icon_color);
			}
			else {
				icon->draw(ci, item_ofs + Point2(0, Math::floor((h - icon->get_height()) / 2.0)),
					icon_color);
			}
		}

		int separator_ofs = (display_width - items[i].text_buf->get_size().width) / 2;

		// Icon
		if (items[i].icon.is_valid()) {
			const Point2 icon_offset = Point2(0, Math::floor((h - icon_size.height) / 2.0));
			Point2 icon_pos;

			if (items[i].separator) {
				separator_ofs -= (icon_size.width + theme_cache.h_separation) / 2;

				if (rtl) {
					icon_pos = Size2(
						control->get_size().width - item_ofs.x - separator_ofs - icon_size.width,
						item_ofs.y);
				}
				else {
					icon_pos = item_ofs + Size2(separator_ofs, 0);
					separator_ofs += icon_size.width + theme_cache.h_separation;
				}
			}
			else {
				const real_t check_w = (gutter_compact || !has_check_gutter)
										   ? 0
										   : (check_max_w + theme_cache.h_separation);
				if (rtl) {
					icon_pos =
						Size2(control->get_size().width - item_ofs.x - check_w - icon_size.width,
							item_ofs.y);
				}
				else {
					icon_pos = item_ofs + Size2(check_w, 0);
				}
			}

			items[i].icon->draw_rect(
				ci, Rect2(icon_pos + icon_offset, icon_size), false, icon_color);
		}

		// Submenu arrow on right hand side.
		if (items[i].submenu) {
			if (rtl) {
				submenu->draw(ci,
					Point2(theme_cache.item_end_padding,
						item_ofs.y + Math::floor(h - submenu->get_height()) / 2),
					icon_color);
			}
			else {
				submenu->draw(ci,
					Point2(display_width - submenu->get_width() - theme_cache.item_end_padding,
						item_ofs.y + Math::floor(h - submenu->get_height()) / 2),
					icon_color);
			}
		}

		// Text
		if (items[i].separator) {
			if (!text.is_empty()) {
				Vector2 text_pos = Point2(separator_ofs,
					item_ofs.y + Math::floor((h - items[i].text_buf->get_size().y) / 2.0));

				if (theme_cache.font_separator_outline_size > 0 &&
					theme_cache.font_separator_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos,
						theme_cache.font_separator_outline_size,
						theme_cache.font_separator_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos, theme_cache.font_separator_color);
			}
		}
		else {
			if (gutter_compact) {
				item_ofs.x += MAX(icon_max_w, check_max_w) + theme_cache.h_separation;
			}
			else {
				if (icon_max_w > 0) {
					item_ofs.x += icon_max_w + theme_cache.h_separation;
				}
				if (has_check_gutter) {
					item_ofs.x += check_max_w + theme_cache.h_separation;
				}
			}
			if (rtl) {
				Vector2 text_pos =
					Size2(control->get_size().width - items[i].text_buf->get_size().width -
							  item_ofs.x,
						item_ofs.y) +
					Point2(0, Math::floor((h - items[i].text_buf->get_size().y) / 2.0));
				if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos, theme_cache.font_outline_size,
						theme_cache.font_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos,
					items[i].disabled ? theme_cache.font_disabled_color
									  : (((active_submenu_index == -1 && i == mouse_over) ||
											 i == active_submenu_index)
												? theme_cache.font_hover_color
												: theme_cache.font_color));
			}
			else {
				Vector2 text_pos =
					item_ofs + Point2(0, Math::floor((h - items[i].text_buf->get_size().y) / 2.0));
				if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
					items[i].text_buf->draw_outline(ci, text_pos, theme_cache.font_outline_size,
						theme_cache.font_outline_color);
				}
				items[i].text_buf->draw(ci, text_pos,
					items[i].disabled ? theme_cache.font_disabled_color
									  : (((active_submenu_index == -1 && i == mouse_over) ||
											 i == active_submenu_index)
												? theme_cache.font_hover_color
												: theme_cache.font_color));
			}
		}

		// Accelerator / Shortcut
		if (items[i].accel != Key::NONE ||
			(items[i].shortcut.is_valid() && items[i].shortcut->has_valid_event())) {
			if (rtl) {
				item_ofs.x = theme_cache.item_end_padding;
			}
			else {
				item_ofs.x = display_width - items[i].accel_text_buf->get_size().x -
							 theme_cache.item_end_padding;
			}
			Vector2 text_pos =
				item_ofs +
				Point2(0, Math::floor((h - items[i].accel_text_buf->get_size().y) / 2.0));
			if (theme_cache.font_outline_size > 0 && theme_cache.font_outline_color.a > 0) {
				items[i].accel_text_buf->draw_outline(
					ci, text_pos, theme_cache.font_outline_size, theme_cache.font_outline_color);
			}
			items[i].accel_text_buf->draw(ci, text_pos,
				((active_submenu_index == -1 && i == mouse_over) || i == active_submenu_index)
					? theme_cache.font_hover_color
					: theme_cache.font_accelerator_color);
		}

		// Cache the item vertical offset from the first item and the height.
		items.write[i]._ofs_cache = ofs.y;
		items.write[i]._height_cache = h;

		ofs.y += h;
	}
}

void PopupMenu::_update_search_bar_visibility()
{
	if (search_bar) {
		if (search_bar_enabled) {
			int item_count = 0;
			for (const Item& item : items) {
				if (!item.separator) {
					item_count++;
				}
			}
			search_bar->set_visible(item_count >= search_bar_min_item_count);
		}
		else {
			search_bar->hide();
		}
	}
}

void PopupMenu::_filter_items(const String& p_query)
{
	for (PopupMenu::Item& item : items) {
		if (item.submenu) {
			item.submenu->_filter_items(p_query);
		}
	}

	for (PopupMenu::Item& item : items) {
		item.visible = true;
	}

	if (p_query.is_empty()) {
		return;
	}

	PackedStringArray search_candidates;
	search_candidates.reserve(items.size());

	Vector<int> search_candidate_to_item;
	search_candidate_to_item.reserve(items.size());

	for (int i = 0; i < items.size(); i++) {
		Item& item = items.write[i];
		item.visible = false;

		if (item.submenu) {
			for (const PopupMenu::Item& submenu_item : item.submenu->items) {
				if (submenu_item.visible) {
					item.visible = true;
					break;
				}
			}
		}

		if (!item.separator) {
			search_candidates.append(item.text);
			search_candidate_to_item.append(i);
		}
	}

	FuzzySearch fuzzy;
	fuzzy.set_max_results(search_candidates.size());
	fuzzy.set_max_misses(search_bar_fuzzy_search_max_misses);
	fuzzy.set_use_exact_tokens(!search_bar_fuzzy_search_enabled);

	for (const Ref<FuzzySearchMatch>& result : fuzzy.search_all(p_query, search_candidates)) {
		PopupMenu::Item& item = items.write[search_candidate_to_item[result->get_original_index()]];
		item.visible = true;
		if (item.submenu) {
			for (PopupMenu::Item& submenu_item : item.submenu->items) {
				submenu_item.visible = true;
			}
		}
	}
}

void PopupMenu::_close_pressed()
{
	if (this_submenu_index != -1 &&
		active_submenu_index != -1) { // Close secondary submenus on first submenu close.
		ERR_FAIL_INDEX_MSG(active_submenu_index, items.size(),
			vformat(
				"Invalid active_submenu_index index %d in _close_pressed.", active_submenu_index));
		items[active_submenu_index].submenu->_close_pressed();
	}
	tracker.close_pressed(this);
}

void PopupMenu::_update_shadow_offsets() const
{
	if (!DisplayServer::is_window_transparency_available() && !is_embedded()) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	Ref<StyleBox> sb = theme_cache.panel_style;
	if (sb.is_null() || sb->get_style_type() != StyleBox::STYLE_FLAT) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	const int shadow_size = sb->get_shadow_size();
	if (shadow_size == 0) {
		panel->set_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, 0);
		return;
	}

	// Offset the background panel so it leaves space inside the window for the shadows to be drawn.
	const Point2 shadow_offset = sb->get_shadow_offset();
	if (is_layout_rtl()) {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size + shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size + shadow_offset.x));
	}
	else {
		panel->set_offset(SIDE_LEFT, MAX(0, shadow_size - shadow_offset.x));
		panel->set_offset(SIDE_RIGHT, MIN(0, -shadow_size - shadow_offset.x));
	}
	panel->set_offset(SIDE_TOP, MAX(0, shadow_size - shadow_offset.y));
	panel->set_offset(SIDE_BOTTOM, MIN(0, -shadow_size - shadow_offset.y));
}

Rect2i PopupMenu::_popup_adjust_rect() const
{
	Rect2i current = PopupTracker::adjust_popup_rect(this);
	if (current == Rect2i()) {
		return current;
	}

	pre_popup_rect = current;

	_update_shadow_offsets();

	if (is_layout_rtl()) {
		current.position -= Vector2(-panel->get_offset(SIDE_RIGHT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	else {
		current.position -= Vector2(panel->get_offset(SIDE_LEFT), panel->get_offset(SIDE_TOP)) *
							get_content_scale_factor();
	}
	current.size += Vector2(panel->get_offset(SIDE_LEFT) - panel->get_offset(SIDE_RIGHT),
						panel->get_offset(SIDE_TOP) - panel->get_offset(SIDE_BOTTOM)) *
					get_content_scale_factor();

	return current;
}

RID PopupMenu::get_focused_accessibility_element() const
{
	if (mouse_over == -1) {
		return get_accessibility_element();
	}
	else {
		const Item& item = items[mouse_over];
		return item.accessibility_item_element;
	}
}

#define ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel)                                              \
	item.text = p_label;                                                                           \
	item.xl_text = atr(p_label);                                                                   \
	item.id = p_id == -1 ? items.size() : p_id;                                                    \
	item.accel = p_accel;

#define ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global, p_allow_echo)                         \
	ERR_FAIL_COND_MSG(p_shortcut.is_null(), "Cannot add item with invalid Shortcut.");             \
	_ref_shortcut(p_shortcut);                                                                     \
	item.text = p_shortcut->get_name();                                                            \
	item.xl_text = atr(item.text);                                                                 \
	item.id = p_id == -1 ? items.size() : p_id;                                                    \
	item.shortcut = p_shortcut;                                                                    \
	item.shortcut_is_global = p_global;                                                            \
	item.allow_echo = p_allow_echo;

#undef ITEM_SETUP_WITH_ACCEL
#undef ITEM_SETUP_WITH_SHORTCUT

void PopupMenu::_close_suspended_timeout()
{
	if (submenu_over != -1 && is_embedded()) {
		ERR_FAIL_INDEX_MSG(submenu_over, items.size(),
			vformat("Invalid submenu index %d in _close_suspended_timeout.", submenu_over));
		if (items[submenu_over].submenu->is_visible()) {
			items[submenu_over].submenu->_close_or_suspend();
		}
	}
	if (active_submenu_index != -1 && items[active_submenu_index].submenu->is_visible()) {
		// Closes the submenu if the mouse is moved off the parent item toward the submenu,
		// but comes to a stop before reaching the submenu and the timeout is reached.
		items[active_submenu_index].submenu->_close_pressed();
	}
}

void PopupMenu::set_item_checked(int p_idx, bool p_checked) {}

void PopupMenu::set_item_disabled(int p_idx, bool p_disabled) {}

String PopupMenu::get_item_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].text;
}

String PopupMenu::get_item_xl_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].xl_text;
}

Control::TextDirection PopupMenu::get_item_text_direction(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Control::TEXT_DIRECTION_INHERITED);
	return items[p_idx].text_direction;
}

String PopupMenu::get_item_language(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].language;
}

Node::AutoTranslateMode PopupMenu::get_item_auto_translate_mode(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), AUTO_TRANSLATE_MODE_INHERIT);
	return items[p_idx].auto_translate_mode;
}

int PopupMenu::get_item_idx_from_text(const String& text) const
{
	for (int idx = 0; idx < items.size(); idx++) {
		if (items[idx].text == text) {
			return idx;
		}
	}

	return -1;
}

Ref<Texture2D> PopupMenu::get_item_icon(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());
	return items[p_idx].icon;
}

int PopupMenu::get_item_icon_max_width(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].icon_max_width;
}

Color PopupMenu::get_item_icon_modulate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());
	return items[p_idx].icon_modulate;
}

Key PopupMenu::get_item_accelerator(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Key::NONE);
	return items[p_idx].accel;
}

bool PopupMenu::is_item_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

bool PopupMenu::is_item_checked(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checked;
}

bool PopupMenu::is_item_indeterminate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].indeterminate;
}

int PopupMenu::get_item_id(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].id;
}

int PopupMenu::get_item_index(int p_id) const
{
	for (int i = 0; i < items.size(); i++) {
		if (items[i].id == p_id) {
			return i;
		}
	}

	return -1;
}

String PopupMenu::get_item_submenu(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].submenu_name;
}

PopupMenu* PopupMenu::get_item_submenu_node(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), nullptr);
	return items[p_idx].submenu;
}

String PopupMenu::get_item_tooltip(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].tooltip;
}

Ref<Shortcut> PopupMenu::get_item_shortcut(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Shortcut>());
	return items[p_idx].shortcut;
}

int PopupMenu::get_item_indent(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].indent;
}

int PopupMenu::get_item_max_states(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), -1);
	return items[p_idx].max_states;
}

int PopupMenu::get_item_state(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), -1);
	return items[p_idx].state;
}

bool PopupMenu::is_item_separator(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].separator;
}

bool PopupMenu::is_item_checkable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type;
}

bool PopupMenu::is_item_radio_checkable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type == Item::CHECKABLE_TYPE_RADIO_BUTTON;
}

bool PopupMenu::is_item_shortcut_global(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].shortcut_is_global;
}

bool PopupMenu::is_item_shortcut_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].shortcut_is_disabled;
}

int PopupMenu::get_focused_item() const { return mouse_over; }

int PopupMenu::get_item_count() const { return items.size(); }

void PopupMenu::scroll_to_item(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());

	// Calculate the position of the item relative to the visible area.
	int item_y = items[p_idx]._ofs_cache;
	int visible_height = scroll_container->get_size().height;
	int relative_y = item_y - scroll_container->get_v_scroll();

	// If item is not fully visible, adjust scroll.
	if (relative_y < 0) {
		scroll_container->set_v_scroll(item_y);
	}
	else if (relative_y + items[p_idx]._height_cache > visible_height) {
		scroll_container->set_v_scroll(item_y + items[p_idx]._height_cache - visible_height);
	}
}

void PopupMenu::set_prefer_native_menu(bool p_enabled)
{
	if (prefer_native != p_enabled) {
		prefer_native = p_enabled;
		unbind_global_menu();
	}
}

bool PopupMenu::is_prefer_native_menu() const { return prefer_native; }

bool PopupMenu::is_native_menu() const
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return false;
	}
#endif

	return global_menu.is_valid();
}

void PopupMenu::add_separator(const String& p_text, int p_id) {}

bool PopupMenu::activate_item_by_event(const InputEvent& p_event, bool p_for_global_only)
{
	ERR_FAIL_COND_V(p_event.is_null(), false);
	Key code = Key::NONE;

	if (p_event.type == InputEventType::KEY) {
		code = p_event.key.keycode;
		if (code == Key::NONE) {
			code = (Key)p_event.key.unicode;
		}
		if (p_event.is_ctrl_pressed()) {
			code |= KeyModifierMask::CTRL;
		}
		if (p_event.is_alt_pressed()) {
			code |= KeyModifierMask::ALT;
		}
		if (p_event.is_meta_pressed()) {
			code |= KeyModifierMask::META;
		}
		if (p_event.is_shift_pressed()) {
			code |= KeyModifierMask::SHIFT;
		}
	}

	for (int i = 0; i < items.size(); i++) {
		if (is_item_disabled(i) || items[i].shortcut_is_disabled ||
			(!items[i].allow_echo && p_event.is_echo())) {
			continue;
		}

		if (items[i].shortcut.is_valid() && items[i].shortcut->matches_event(p_event) &&
			(items[i].shortcut_is_global || !p_for_global_only)) {
			return true;
		}

		if (code != Key::NONE && items[i].accel == code) {
			return true;
		}

		if (items[i].submenu) {
			if (items[i].submenu->activate_item_by_event(p_event, p_for_global_only)) {
				return true;
			}
		}
	}
	return false;
}

// Hide on item selection determines whether or not the popup will close after item selection
void PopupMenu::set_hide_on_item_selection(bool p_enabled) { hide_on_item_selection = p_enabled; }

bool PopupMenu::is_hide_on_item_selection() const { return hide_on_item_selection; }

void PopupMenu::set_hide_on_checkable_item_selection(bool p_enabled)
{
	hide_on_checkable_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_checkable_item_selection() const
{
	return hide_on_checkable_item_selection;
}

void PopupMenu::set_hide_on_multistate_item_selection(bool p_enabled)
{
	hide_on_multistate_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_multistate_item_selection() const
{
	return hide_on_multistate_item_selection;
}

float PopupMenu::get_submenu_popup_delay() const { return submenu_timer->get_wait_time(); }

void PopupMenu::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool PopupMenu::get_allow_search() const { return allow_search; }

bool PopupMenu::is_search_bar_enabled() const { return search_bar_enabled; }

int PopupMenu::get_search_bar_min_item_count() const { return search_bar_min_item_count; }

void PopupMenu::set_search_bar_fuzzy_search_enabled(bool p_enabled)
{
	search_bar_fuzzy_search_enabled = p_enabled;
}

bool PopupMenu::is_search_bar_fuzzy_search_enabled() const
{
	return search_bar_fuzzy_search_enabled;
}

void PopupMenu::set_search_bar_fuzzy_search_max_misses(int p_max_misses)
{
	ERR_FAIL_COND(p_max_misses < 0);
	search_bar_fuzzy_search_max_misses = p_max_misses;
}

int PopupMenu::get_search_bar_fuzzy_search_max_misses() const
{
	return search_bar_fuzzy_search_max_misses;
}

void PopupMenu::clear(bool p_free_submenus) {}

#ifdef TOOLS_ENABLED
PackedStringArray PopupMenu::get_configuration_warnings() const
{
	PackedStringArray warnings = Window::get_configuration_warnings();

	if (!DisplayServer::is_window_transparency_available() &&
		!GLOBAL_GET_CACHED(bool, "display/window/subwindows/embed_subwindows")) {
		Ref<StyleBox> sb = theme_cache.panel_style;
		if (sb.is_valid() && sb->get_style_type() == StyleBox::STYLE_FLAT &&
			(sb->get_shadow_size() > 0 || sb->get_corner_radius(CORNER_TOP_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_TOP_RIGHT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_LEFT) > 0 ||
				sb->get_corner_radius(CORNER_BOTTOM_RIGHT) > 0)) {
			warnings.push_back(RTR(
				"The current theme style has shadows and/or rounded corners for popups, but those "
				"won't display correctly if \"display/window/per_pixel_transparency/allowed\" "
				"isn't enabled in the Project Settings, nor if it isn't supported."));
		}
	}

	return warnings;
}
#endif

void PopupMenu::add_autohide_area(const Rect2& p_area) { autohide_areas.push_back(p_area); }

void PopupMenu::clear_autohide_areas() { autohide_areas.clear(); }

void PopupMenu::_native_popup(const Rect2i& p_rect)
{
	Point2i popup_pos = p_rect.position;
	if (is_embedded()) {
		popup_pos = get_embedder()->get_screen_transform().xform(
			popup_pos); // Note: for embedded windows "screen transform" is transform relative to
						// embedder not the actual screen.
		DisplayServerEnums::WindowID wid = get_window_id();
		if (wid == DisplayServerEnums::INVALID_WINDOW_ID) {
			wid = DisplayServerEnums::MAIN_WINDOW_ID;
		}
		popup_pos += DisplayServer::window_get_position(wid);
	}
	float win_scale = get_parent_visible_window()->get_content_scale_factor();
	NativeMenu::set_minimum_width(global_menu, p_rect.size.x * win_scale);
	NativeMenu::popup(global_menu, popup_pos);
}

void PopupMenu::_popup_base(const Rect2i& p_bounds)
{
	bool native = global_menu.is_valid();
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		native = false;
	}
#endif

	if (native) {
		_native_popup(p_bounds != Rect2i() ? p_bounds : Rect2i(get_position(), Size2i()));
	}
	else {
		if (is_inside_tree()) {
			set_flag(FLAG_POPUP, true);
			set_flag(FLAG_NO_FOCUS, !is_embedded());
		}

		moved = Vector2();
		popup_time_msec = OS::get_ticks_msec();

		Window::_popup_base(p_bounds);
	}
}

void PopupMenu::set_shrink_height(bool p_shrink) { shrink_height = p_shrink; }

bool PopupMenu::get_shrink_height() const { return shrink_height; }

void PopupMenu::set_shrink_width(bool p_shrink) { shrink_width = p_shrink; }

bool PopupMenu::get_shrink_width() const { return shrink_width; }

RID PopupMenuItems::get_focused_accessibility_element() const
{
	return popup->get_focused_accessibility_element();
}

void PopupMenu::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon) {}

PopupMenu::PopupMenu()
{
	tracker.configure_window(this);
}

PopupMenu::~PopupMenu() { unbind_global_menu(); }

void PopupMenu::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
			tracker.hide_reason = HIDE_REASON_NONE;
			tracker.initialize_visible_parents(this);
		}
		else {
			tracker.deinitialize_visible_parents(this);
			tracker.popped_up = false;
		}
	} break;
	case NOTIFICATION_EXIT_TREE: {
		tracker.deinitialize_visible_parents(this);
	} break;
	}
}

void PopupMenu::_post_popup()
{
	Window::_post_popup();
	tracker.post_popup();
}

void PopupMenu::_shape_item(int) const {}

void PopupMenu::_input_from_window_internal(const InputEvent&) {}

void PopupMenu::_close_or_suspend() {}

void PopupMenu::_activate_submenu(int, bool) {}

void PopupMenu::set_visible(bool p_visible) { Window::set_visible(p_visible); }

void PopupMenu::remove_child_notify(Node* p_child) {}

void PopupMenu::_pre_popup(const Size2i& p_size) {}

void PopupMenu::_parent_focused()
{
	if (tracker.handle_parent_focused(this)) {
		_close_pressed();
	}
}

String PopupMenu::_get_accessibility_name() const { return String(); }

void PopupMenu::add_child_notify(Node* p_child) {}

// --- popup_menu.compat.inc ---

#ifndef DISABLE_DEPRECATED


void PopupMenu::_add_shortcut_bind_compat_36493(
	const Ref<Shortcut>& p_shortcut, int p_id, bool p_global)
{
}

void PopupMenu::_add_icon_shortcut_bind_compat_36493(
	const Ref<Texture2D>& p_icon, const Ref<Shortcut>& p_shortcut, int p_id, bool p_global)
{
}

void PopupMenu::_clear_bind_compat_79965() {}

void PopupMenu::_set_system_menu_root_bind_compat_87452(const String& p_special)
{
	if (p_special == "_dock") {
		set_system_menu(NativeMenu::DOCK_MENU_ID);
	}
	else if (p_special == "_apple") {
		set_system_menu(NativeMenu::APPLICATION_MENU_ID);
	}
	else if (p_special == "_window") {
		set_system_menu(NativeMenu::WINDOW_MENU_ID);
	}
	else if (p_special == "_help") {
		set_system_menu(NativeMenu::HELP_MENU_ID);
	}
}

String PopupMenu::_get_system_menu_root_bind_compat_87452() const
{
	switch (get_system_menu()) {
	case NativeMenu::APPLICATION_MENU_ID:
		return "_apple";
	case NativeMenu::WINDOW_MENU_ID:
		return "_window";
	case NativeMenu::HELP_MENU_ID:
		return "_help";
	case NativeMenu::DOCK_MENU_ID:
		return "_dock";
	default:
		return "";
	}
}

void PopupMenu::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from scene/gui/color_picker.cpp
// ============================================================================

#include "color_picker.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/expression.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/gui/color_picker.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "core/math/color.h"
#include "scene/resources/gradient_texture.h"
#include <thirdparty/misc/ok_color_shader.h>
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"
#include "scene/gui/color_picker.h"

#ifdef MACOS_ENABLED
#include "core/os/os.h"
#endif

// --- color_mode (flattened internal helper) ---

class ColorMode final
{
public:
	ColorPicker* color_picker = nullptr;
	ColorPicker::ColorModeType mode_type = ColorPicker::MODE_RGB;
	float cached_hue = 0.0f;
	float cached_saturation = 0.0f;
	Ref<GradientTexture2D> rgb_texture[3];
	Ref<GradientTexture2D> hue_texture;

	ColorMode(ColorPicker* p_color_picker, ColorPicker::ColorModeType p_type = ColorPicker::MODE_RGB)
		: color_picker(p_color_picker), mode_type(p_type)
	{
	}

	String get_name() const
	{
		switch (mode_type) {
		case ColorPicker::MODE_RGB:
			return "RGB";
		case ColorPicker::MODE_HSV:
			return "HSV";
		case ColorPicker::MODE_LINEAR:
			return ETR("Linear");
		case ColorPicker::MODE_OKHSL:
			return "OKHSL";
		default:
			return String();
		}
	}

	int get_slider_count() const { return 3; }

	float get_slider_step() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 0.001f : 1.0f;
	}

	float get_spinbox_arrow_step() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 0.01f : get_slider_step();
	}

	String get_slider_label(int idx) const
	{
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), String(), "Couldn't get slider label.");
		switch (mode_type) {
		case ColorPicker::MODE_HSV: {
			static const char* const labels[3] = {"H", "S", "V"};
			return labels[idx];
		}
		case ColorPicker::MODE_OKHSL: {
			static const char* const labels[3] = {"H", "S", "L"};
			return labels[idx];
		}
		case ColorPicker::MODE_RGB:
		case ColorPicker::MODE_LINEAR:
		default: {
			static const char* const labels[3] = {"R", "G", "B"};
			return labels[idx];
		}
		}
	}

	float get_slider_max(int idx) const
	{
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider max value.");
		switch (mode_type) {
		case ColorPicker::MODE_RGB:
			return 255.0f;
		case ColorPicker::MODE_LINEAR:
			return 1.0f;
		case ColorPicker::MODE_HSV:
		case ColorPicker::MODE_OKHSL:
		default: {
			static constexpr float slider_max[3] = {359.0f, 100.0f, 100.0f};
			return slider_max[idx];
		}
		}
	}

	bool get_allow_greater() const
	{
		return mode_type == ColorPicker::MODE_RGB || mode_type == ColorPicker::MODE_LINEAR;
	}

	float get_alpha_slider_max() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 1.0f : 255.0f;
	}

	float get_alpha_slider_value() const
	{
		return mode_type == ColorPicker::MODE_LINEAR
				   ? color_picker->get_pick_color().a
				   : color_picker->get_pick_color().a * 255.0f;
	}

	float get_slider_value(int idx) const;
	Color get_color() const;
	void _value_changed();
	void _greater_value_inputted();
	void slider_draw(int p_which);
};

float ColorMode::get_slider_value(int idx) const
{
	switch (mode_type) {
	case ColorPicker::MODE_RGB: {
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider value.");
		return color_picker->color_normalized[idx] * 255;
	}
	case ColorPicker::MODE_HSV: {
		switch (idx) {
		case 0:
			return (color_picker->color_normalized.get_s() > 0)
					   ? color_picker->color_normalized.get_h() * 360.0
					   : cached_hue;
		case 1:
			return (color_picker->color_normalized.get_v() > 0)
					   ? color_picker->color_normalized.get_s() * 100.0
					   : cached_saturation;
		case 2:
			return color_picker->color_normalized.get_v() * 100.0;
		default:
			ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
		}
	}
	case ColorPicker::MODE_LINEAR: {
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider value.");
		Color color = color_picker->color_normalized.srgb_to_linear();
		return color[idx];
	}
	case ColorPicker::MODE_OKHSL: {
		switch (idx) {
		case 0:
			return (color_picker->color_normalized.get_ok_hsl_s() > 0)
					   ? color_picker->color_normalized.get_ok_hsl_h() * 360.0
					   : cached_hue;
		case 1:
			return (color_picker->color_normalized.get_ok_hsl_l() > 0)
					   ? color_picker->color_normalized.get_ok_hsl_s() * 100.0
					   : cached_saturation;
		case 2:
			return color_picker->color_normalized.get_ok_hsl_l() * 100.0;
		default:
			ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
		}
	}
	default:
		ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
	}
}

Color ColorMode::get_color() const
{
	Vector<float> values = color_picker->get_active_slider_values();
	switch (mode_type) {
	case ColorPicker::MODE_RGB: {
		Color color;
		for (int i = 0; i < 4; i++) {
			color[i] = values[i] / 255.0;
		}
		return color;
	}
	case ColorPicker::MODE_HSV:
		return Color::from_hsv(
			values[0] / 360.0, values[1] / 100.0, values[2] / 100.0, values[3] / 255.0);
	case ColorPicker::MODE_LINEAR: {
		Color color;
		for (int i = 0; i < 4; i++) {
			color[i] = values[i];
		}
		return color.linear_to_srgb();
	}
	case ColorPicker::MODE_OKHSL:
		return Color::from_ok_hsl(
			values[0] / 360.0, values[1] / 100.0, values[2] / 100.0, values[3] / 255.0);
	default:
		return Color();
	}
}

void ColorMode::_value_changed()
{
	if (mode_type == ColorPicker::MODE_HSV) {
		Vector<float> values = color_picker->get_active_slider_values();

		if (values[1] > 0 || values[0] != cached_hue) {
			cached_hue = values[0];
		}
		if (values[2] > 0 || values[1] != cached_saturation) {
			cached_saturation = values[1];
		}

		color_picker->h = color_picker->sliders[0]->get_value() / 360.0;
		color_picker->s = color_picker->sliders[1]->get_value() / 100.0;
		color_picker->v = color_picker->sliders[2]->get_value() / 100.0;

		color_picker->hsv_cached = true;
	}
	else if (mode_type == ColorPicker::MODE_OKHSL) {
		Vector<float> values = color_picker->get_active_slider_values();

		if (values[1] > 0 || values[0] != cached_hue) {
			cached_hue = values[0];
		}
		if (values[2] > 0 || values[1] != cached_saturation) {
			cached_saturation = values[1];
		}

		color_picker->ok_hsl_h = color_picker->sliders[0]->get_value() / 360.0;
		color_picker->ok_hsl_s = color_picker->sliders[1]->get_value() / 100.0;
		color_picker->ok_hsl_l = color_picker->sliders[2]->get_value() / 100.0;

		color_picker->okhsl_cached = true;
	}
}

void ColorMode::_greater_value_inputted()
{
	if (mode_type == ColorPicker::MODE_RGB) {
		Slider** sliders = color_picker->sliders;
		Color color_prev = color_picker->color;
		for (int i = 0; i < 3; i++) {
			if (sliders[i]->get_value() > 255) {
				color_prev[i] = sliders[i]->get_value() / 255.0;
			}
		}
		Color linear_color = color_prev.srgb_to_linear();
		float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));
		Color srgb = Color(linear_color.r / multiplier, linear_color.g / multiplier,
			linear_color.b / multiplier, linear_color.a)
						 .linear_to_srgb();
		sliders[0]->set_value_no_signal(srgb.r * 255);
		sliders[1]->set_value_no_signal(srgb.g * 255);
		sliders[2]->set_value_no_signal(srgb.b * 255);

		color_picker->intensity = Math::log2(multiplier);
		color_picker->intensity_slider->set_value_no_signal(color_picker->intensity);
	}
	else if (mode_type == ColorPicker::MODE_LINEAR) {
		Slider** sliders = color_picker->sliders;
		Color color_prev = color_picker->color;
		Color linear_color = color_prev.srgb_to_linear();
		for (int i = 0; i < 3; i++) {
			if (sliders[i]->get_value() > 1 + CMP_EPSILON) {
				linear_color[i] = sliders[i]->get_value();
			}
		}

		float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));

		sliders[0]->set_value_no_signal(linear_color.r / multiplier);
		sliders[1]->set_value_no_signal(linear_color.g / multiplier);
		sliders[2]->set_value_no_signal(linear_color.b / multiplier);

		color_picker->intensity = Math::log2(multiplier);
		color_picker->intensity_slider->set_value_no_signal(color_picker->intensity);
	}
}

void ColorMode::slider_draw(int p_which)
{
	Slider* slider = color_picker->get_slider(p_which);
	Size2 size = slider->get_size();
	Color color = color_picker->color_normalized;
	const real_t margin = 16 * color_picker->theme_cache.base_scale;

	switch (mode_type) {
	case ColorPicker::MODE_RGB:
	case ColorPicker::MODE_LINEAR: {
		Color left_color = Color(
			p_which == 0 ? 0 : color.r, p_which == 1 ? 0 : color.g, p_which == 2 ? 0 : color.b);
		Color right_color = Color(
			p_which == 0 ? 1 : color.r, p_which == 1 ? 1 : color.g, p_which == 2 ? 1 : color.b);

		if (rgb_texture[p_which].is_null()) {
			rgb_texture[p_which].instantiate();
			rgb_texture[p_which]->set_width(400);
			rgb_texture[p_which]->set_height(6);
		}

		Ref<GradientTexture2D> gradient_texture = rgb_texture[p_which];
		Ref<Gradient> gradient = gradient_texture->get_gradient();
		if (gradient.is_null()) {
			gradient.instantiate();
			PackedFloat32Array offsets = {0, 1};
			gradient->set_offsets(offsets);
			gradient->set_interpolation_color_space(mode_type == ColorPicker::MODE_LINEAR
														? Gradient::ColorSpace::GRADIENT_COLOR_SPACE_LINEAR_SRGB
														: Gradient::ColorSpace::GRADIENT_COLOR_SPACE_SRGB);
			gradient_texture->set_gradient(gradient);
		}

		PackedColorArray colors = {left_color, right_color};
		gradient->set_colors(colors);

		slider->draw_texture_rect(
			gradient_texture.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false);
	} break;

	case ColorPicker::MODE_HSV: {
		Vector<Vector2> pos;
		pos.resize(4);
		Vector<Color> col;
		col.resize(4);
		Color left_color;
		Color right_color;

		if (p_which == 0) {
			float v = color.get_v();
			left_color = Color(v, v, v);
			right_color = left_color;
		}
		else {
			Color s_col;
			Color v_col;
			s_col.set_hsv(color.get_h(), 0, color.get_v());
			left_color = (p_which == 1) ? s_col : Color(0, 0, 0);

			float s_col_hue =
				(Math::is_zero_approx(color.get_s())) ? cached_hue / 360.0 : color.get_h();
			s_col.set_hsv(s_col_hue, 1, color.get_v());
			v_col.set_hsv(color.get_h(), color.get_s(), 1);
			right_color = (p_which == 1) ? s_col : v_col;
		}
		col.set(0, left_color);
		col.set(1, right_color);
		col.set(2, right_color);
		col.set(3, left_color);
		pos.set(0, Vector2(0, 0));
		pos.set(1, Vector2(size.x, 0));
		pos.set(2, Vector2(size.x, margin));
		pos.set(3, Vector2(0, margin));

		slider->draw_polygon(pos, col);

		if (p_which == 0) {
			Ref<Texture2D> hue = color_picker->theme_cache.color_hue;
			slider->draw_texture_rect(hue.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false,
				Color::from_hsv(0, 0, color.get_v(), color.get_s()));
		}
	} break;

	case ColorPicker::MODE_OKHSL: {
		Vector<Vector2> pos;
		Vector<Color> col;
		Color left_color;
		Color right_color;
		float okhsl_l = color.get_ok_hsl_l();
		float slider_hue =
			(Math::is_zero_approx(color.get_ok_hsl_s())) ? cached_hue / 360.0 : color.get_ok_hsl_h();
		float slider_sat = (Math::is_zero_approx(okhsl_l) || Math::is_equal_approx(okhsl_l, 1))
							   ? cached_saturation / 100.0
							   : color.get_ok_hsl_s();

		if (p_which == 2) {
			pos.resize(6);
			col.resize(6);
			left_color = Color(0, 0, 0);
			Color middle_color = Color::from_ok_hsl(slider_hue, slider_sat, 0.5);
			right_color.set_ok_hsl(slider_hue, slider_sat, 1);

			col.set(0, left_color);
			col.set(1, middle_color);
			col.set(2, right_color);
			col.set(3, right_color);
			col.set(4, middle_color);
			col.set(5, left_color);
			pos.set(0, Vector2(0, 0));
			pos.set(1, Vector2(size.x * 0.5, 0));
			pos.set(2, Vector2(size.x, 0));
			pos.set(3, Vector2(size.x, margin));
			pos.set(4, Vector2(size.x * 0.5, margin));
			pos.set(5, Vector2(0, margin));
			slider->draw_polygon(pos, col);
		}
		else if (p_which == 1) {
			pos.resize(4);
			col.resize(4);

			left_color.set_ok_hsl(slider_hue, 0, okhsl_l);
			right_color.set_ok_hsl(slider_hue, 1, okhsl_l);

			col.set(0, left_color);
			col.set(1, right_color);
			col.set(2, right_color);
			col.set(3, left_color);
			pos.set(0, Vector2(0, 0));
			pos.set(1, Vector2(size.x, 0));
			pos.set(2, Vector2(size.x, margin));
			pos.set(3, Vector2(0, margin));
			slider->draw_polygon(pos, col);
		}
		else if (p_which == 0) {
			const int precision = 7;
			if (hue_texture.is_null()) {
				hue_texture.instantiate();
				hue_texture->set_width(400);
				hue_texture->set_height(6);
			}
			Ref<Gradient> hue_gradient = hue_texture->get_gradient();
			if (hue_gradient.is_null()) {
				hue_gradient.instantiate();
				PackedFloat32Array offsets;
				offsets.resize(precision);
				for (int i = 0; i < precision; i++) {
					float h = i / float(precision - 1);
					offsets.write[i] = h;
				}
				hue_gradient->set_offsets(offsets);
				hue_gradient->set_interpolation_color_space(
					Gradient::ColorSpace::GRADIENT_COLOR_SPACE_OKLAB);
				hue_texture->set_gradient(hue_gradient);
			}

			PackedColorArray colors;
			colors.resize(precision);

			for (int i = 0; i < precision; i++) {
				float h = i / float(precision - 1);
				colors.write[i] = Color::from_ok_hsl(h, slider_sat, okhsl_l);
			}
			hue_gradient->set_colors(colors);
			slider->draw_texture_rect(
				hue_texture.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false);
		}
	} break;

	default:
		break;
	}
}

// --- color_picker_shape (internal helper) ---

class ColorPickerShape final
{
	static constexpr float WHEEL_RADIUS = 0.42f;

	static inline Ref<Shader> wheel_shader;
	static inline Ref<Shader> circle_shader;
	static inline Ref<Shader> circle_ok_color_shader;
	static inline Ref<Shader> rectangle_ok_color_hs_shader;
	static inline Ref<Shader> rectangle_ok_color_hl_shader;

	ColorPicker* color_picker = nullptr;
	ColorPicker::PickerShapeType shape_type = ColorPicker::SHAPE_NONE;
	bool is_dragging = false;

	// HSV Rectangle controls.
	Control* sv_square = nullptr;
	Control* hue_slider = nullptr;

	// OK HS / HL Rectangle controls.
	Control* rectangle_margin = nullptr;
	Control* square = nullptr;
	Control* square_overlay = nullptr;

	// HSV Wheel controls & state.
	Control* wheel = nullptr;
	Control* wheel_uv = nullptr;
	bool wheel_focused = true;
	bool spinning = false;
	bool rotate_next_echo_event = false;

	// VHS / OKHSL Circle controls & state (value_slider shared with OK rectangles).
	Control* circle = nullptr;
	Control* circle_overlay = nullptr;
	Control* value_slider = nullptr;
	Vector2i circle_keyboard_joypad_picker_cursor_position;

	void _emit_color_changed();

	Ref<Shader> _get_shader() const;
	void _initialize_controls();
	void _update_cursor(const Vector2& p_color_change_vector, bool p_is_echo);

	bool can_handle(
		const InputEvent& p_event, Vector2& r_position, bool* r_is_click = nullptr);
	void cancel_event();

	void draw_focus_rect(Control* p_control, const Rect2& p_rect = Rect2());
	void draw_focus_circle(Control* p_control);
	void draw_sv_square(Control* p_control, const Rect2& p_square, bool p_draw_focus = true);
	void draw_cursor(Control* p_control, const Vector2& p_center, bool p_draw_bg = true);
	void draw_circle_cursor(Control* p_control, float p_hue, float p_saturation);

	void connect_shape_focus(Control* p_shape);
	void shape_focus_entered();
	void shape_focus_exited();

	void handle_cursor_editing(const InputEvent& p_event, Control* p_control);
	int get_edge_h_change(const Vector2& p_color_change_vector);
	float get_h_on_circle_edge(const Vector2& p_color_change_vector);
	float _get_h_on_wheel(const Vector2& p_color_change_vector);
	void _reset_wheel_focus();
	void update_circle_cursor(
		const Vector2& p_color_change_vector, const Vector2& p_center, const Vector2& p_hue_offset);

	// Draw & input callbacks dispatched by shape_type.
	void _sv_square_draw();
	void _hue_slider_draw();
	void _square_draw();
	void _square_overlay_draw();
	void _value_slider_draw();
	void _wheel_input(const InputEvent& p_event);
	void _wheel_draw();
	void _wheel_uv_draw();
	void _circle_input(const InputEvent& p_event);
	void _value_slider_input(const InputEvent& p_event);
	void _circle_draw();
	void _circle_overlay_draw();

public:
	Vector<Control*> controls;
	bool is_initialized = false;
	bool cursor_editing = false;
	float echo_multiplier = 1.0f;

	static void init_shaders();
	static void finish_shaders();

	ColorPicker::PickerShapeType get_shape_type() const { return shape_type; }
	String get_name() const;
	Ref<Texture2D> get_icon() const;
	bool is_ok_hsl() const;

	void initialize_controls();
	void update_theme();
	void grab_focus();

	ColorPickerShape(ColorPicker* p_color_picker, ColorPicker::PickerShapeType p_type);
};

void ColorPickerShape::init_shaders()
{
	wheel_shader.instantiate();
	wheel_shader->set_code(R"(
// ColorPicker wheel shader.

shader_type canvas_item;

uniform float wheel_radius = 0.42;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);

	COLOR = vec4(clamp((abs(fract(((a - TAU) / TAU) + vec3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), (b + b2 + b3 + b4) / 4.00);
}
)");

	circle_shader.instantiate();
	circle_shader->set_code(R"(
// ColorPicker circle shader.

shader_type canvas_item;

uniform float v = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);

	COLOR = vec4(mix(vec3(1.0), clamp(abs(fract(vec3((a - TAU) / TAU) + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - vec3(3.0)) - vec3(1.0), 0.0, 1.0), ((float(sqrt(x * x + y * y)) * 2.0)) / 1.0) * vec3(v), (b + b2 + b3 + b4) / 4.00);
})");

	circle_ok_color_shader.instantiate();
	circle_ok_color_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hsl circle shader.

uniform float ok_hsl_l = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float h = atan(y, x) / (2.0 * M_PI);
	float s = sqrt(x * x + y * y) * 2.0;
	vec3 col = okhsl_to_srgb(vec3(h, s, ok_hsl_l));
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);
	COLOR = vec4(col, (b + b2 + b3 + b4) / 4.00);
})");

	rectangle_ok_color_hs_shader.instantiate();
	rectangle_ok_color_hs_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hs rectangle shader.

uniform float ok_hsl_l = 0.0;

void fragment() {
	float h = UV.x;
	float s = 1.0 - UV.y;
	vec3 col = okhsl_to_srgb(vec3(h, s, ok_hsl_l));
	COLOR = vec4(col, 1.0);
})");

	rectangle_ok_color_hl_shader.instantiate();
	rectangle_ok_color_hl_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hl rectangle shader.

uniform float ok_hsl_s = 0.0;

void fragment() {
	float h = UV.x;
	float l = 1.0 - UV.y;
	vec3 col = okhsl_to_srgb(vec3(h, ok_hsl_s, l));
	COLOR = vec4(col, 1.0);
})");
}

void ColorPickerShape::finish_shaders()
{
	wheel_shader.unref();
	circle_shader.unref();
	circle_ok_color_shader.unref();
	rectangle_ok_color_hs_shader.unref();
	rectangle_ok_color_hl_shader.unref();
}

ColorPickerShape::ColorPickerShape(
	ColorPicker* p_color_picker, ColorPicker::PickerShapeType p_type)
	: color_picker(p_color_picker), shape_type(p_type)
{
}

String ColorPickerShape::get_name() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
		return ETR("HSV Rectangle");
	case ColorPicker::SHAPE_HSV_WHEEL:
		return ETR("HSV Wheel");
	case ColorPicker::SHAPE_VHS_CIRCLE:
		return ETR("VHS Circle");
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return ETR("OKHSL Circle");
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
		return ETR("OK HS Rectangle");
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return ETR("OK HL Rectangle");
	default:
		return String();
	}
}

Ref<Texture2D> ColorPickerShape::get_icon() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return color_picker->theme_cache.shape_rect;
	case ColorPicker::SHAPE_HSV_WHEEL:
		return color_picker->theme_cache.shape_rect_wheel;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return color_picker->theme_cache.shape_circle;
	default:
		return Ref<Texture2D>();
	}
}

bool ColorPickerShape::is_ok_hsl() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return true;
	default:
		return false;
	}
}

Ref<Shader> ColorPickerShape::_get_shader() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_WHEEL:
		return wheel_shader;
	case ColorPicker::SHAPE_VHS_CIRCLE:
		return circle_shader;
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return circle_ok_color_shader;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
		return rectangle_ok_color_hs_shader;
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return rectangle_ok_color_hl_shader;
	default:
		return Ref<Shader>();
	}
}

bool ColorPickerShape::can_handle(
	const InputEvent& p_event, Vector2& r_position, bool* r_is_click)
{
	if (p_event.type == InputEventType::MOUSE_BUTTON) {
		if (p_event.mouse_button.button_index != MouseButton::LEFT) {
			return false;
		}

		if (r_is_click) {
			*r_is_click = true;
		}

		if (p_event.is_pressed()) {
			is_dragging = true;
			r_position = p_event.mouse_button.position;
			return true;
		}
	}

	if (is_dragging && p_event.type == InputEventType::MOUSE_MOTION) {
		r_position = p_event.mouse_motion.position;
		return true;
	}
	return false;
}

void ColorPickerShape::cancel_event() { is_dragging = false; }

void ColorPickerShape::_emit_color_changed() {}

void ColorPickerShape::connect_shape_focus(Control*) {}

void ColorPickerShape::handle_cursor_editing(const InputEvent&, Control*) {}

void ColorPickerShape::draw_focus_rect(Control* p_control, const Rect2& p_rect)
{
	if (!p_control->has_focus(true)) {
		return;
	}

	Rect2 focus_rect;
	if (p_rect.has_area()) {
		focus_rect = p_rect;
	}
	else {
		focus_rect = Rect2(Vector2(), p_control->get_size());
	}

	const RID ci = p_control->get_canvas_item();
	if (!cursor_editing) {
		Renderer::canvas_item_add_rect(
			ci, focus_rect, color_picker->theme_cache.focused_not_editing_cursor_color);
	}
	color_picker->theme_cache.picker_focus_rectangle->draw(ci, focus_rect);
}

void ColorPickerShape::draw_focus_circle(Control* p_control)
{
	if (!p_control->has_focus(true)) {
		return;
	}

	const Rect2 focus_rect(Vector2(), p_control->get_size());
	const RID ci = p_control->get_canvas_item();
	if (!cursor_editing) {
		Renderer::canvas_item_add_circle(ci, focus_rect.get_center(),
			focus_rect.get_size().y * 0.5,
			color_picker->theme_cache.focused_not_editing_cursor_color);
	}
	color_picker->theme_cache.picker_focus_circle->draw(ci, focus_rect);
}

void ColorPickerShape::draw_sv_square(Control* p_control, const Rect2& p_square, bool p_draw_focus)
{
	const Vector2 end = p_square.get_end();
	PackedVector2Array points = {
		p_square.position,
		Vector2(end.x, p_square.position.y),
		end,
		Vector2(p_square.position.x, end.y),
	};

	Color color1 = Color::from_hsv(color_picker->h, 1, 1);
	Color color2 = Color::from_hsv(color_picker->h, 1, 0);

	PackedColorArray colors = {
		Color(1, 1, 1, 1),
		Color(1, 1, 1, 1),
		Color(0, 0, 0, 1),
		Color(0, 0, 0, 1),
	};
	p_control->draw_polygon(points, colors);

	colors = {
		Color(color1, 0),
		Color(color1, 1),
		Color(color2, 1),
		Color(color2, 0),
	};
	p_control->draw_polygon(points, colors);

	Vector2 cursor_pos;
	cursor_pos.x =
		CLAMP(p_square.position.x + p_square.size.x * color_picker->s, p_square.position.x, end.x);
	cursor_pos.y = CLAMP(p_square.position.y + p_square.size.y * (1.0 - color_picker->v),
		p_square.position.y, end.y);

	if (p_draw_focus) {
		draw_focus_rect(p_control, p_square);
	}
	draw_cursor(p_control, cursor_pos);
}

void ColorPickerShape::draw_cursor(Control* p_control, const Vector2& p_center, bool p_draw_bg)
{
	const Vector2 position = p_center - color_picker->theme_cache.picker_cursor->get_size() * 0.5;
	if (p_draw_bg) {
		p_control->draw_texture(color_picker->theme_cache.picker_cursor_bg.ptr(), position,
			Color(color_picker->color, 1.0));
	}
	p_control->draw_texture(color_picker->theme_cache.picker_cursor.ptr(), position);
}

void ColorPickerShape::draw_circle_cursor(Control* p_control, float p_hue, float p_saturation)
{
	const Vector2 center = p_control->get_size() * 0.5;
	const Vector2 cursor_pos(center.x + (center.x * Math::cos(p_hue * Math::TAU) * p_saturation),
		center.y + (center.y * Math::sin(p_hue * Math::TAU) * p_saturation));

	draw_cursor(p_control, cursor_pos);
}

void ColorPickerShape::shape_focus_entered()
{
	if (!(Input::is_action_pressed("ui_up") || Input::is_action_pressed("ui_down") ||
			Input::is_action_pressed("ui_left") || Input::is_action_pressed("ui_right"))) {
		cursor_editing = true;
	}
}

void ColorPickerShape::shape_focus_exited() { cursor_editing = false; }

int ColorPickerShape::get_edge_h_change(const Vector2& p_color_change_vector)
{
	int h_change = 0;

	if (color_picker->h > 0 && color_picker->h < 0.5) {
		h_change -= p_color_change_vector.x;
	}
	else if (color_picker->h > 0.5 && color_picker->h < 1) {
		h_change += p_color_change_vector.x;
	}

	if (color_picker->h > 0.25 && color_picker->h < 0.75) {
		h_change -= p_color_change_vector.y;
	}
	else if (color_picker->h < 0.25 || color_picker->h > 0.75) {
		h_change += p_color_change_vector.y;
	}
	return h_change;
}

float ColorPickerShape::get_h_on_circle_edge(const Vector2& p_color_change_vector)
{
	int h_change = get_edge_h_change(p_color_change_vector);

	float target_h = Math::wrapf(color_picker->h + h_change / 360.0, 0, 1);
	int current_quarter = color_picker->h * 4;
	int future_quarter = target_h * 4;
	if (p_color_change_vector.y > 0 && ((future_quarter == 0 && current_quarter == 1) ||
										   (future_quarter == 1 && current_quarter == 0))) {
		target_h = 0.25f;
	}
	else if (p_color_change_vector.y < 0 && ((future_quarter == 2 && current_quarter == 3) ||
												  (future_quarter == 3 && current_quarter == 2))) {
		target_h = 0.75f;
	}
	else if (p_color_change_vector.x < 0 && ((future_quarter == 1 && current_quarter == 2) ||
												  (future_quarter == 2 && current_quarter == 1))) {
		target_h = 0.5f;
	}
	else if (p_color_change_vector.x > 0 && ((future_quarter == 3 && current_quarter == 0) ||
												  (future_quarter == 0 && current_quarter == 3))) {
		target_h = 0;
	}
	return target_h;
}

float ColorPickerShape::_get_h_on_wheel(const Vector2& p_color_change_vector)
{
	int h_change = get_edge_h_change(p_color_change_vector);

	float target_h = Math::wrapf(color_picker->h + h_change / 360.0, 0, 1);
	int current_quarter = color_picker->h * 4;
	int future_quarter = target_h * 4;

	if (p_color_change_vector.y > 0 && ((future_quarter == 0 && current_quarter == 1) ||
										   (future_quarter == 1 && current_quarter == 0))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.y < 0 && ((future_quarter == 2 && current_quarter == 3) ||
												  (future_quarter == 3 && current_quarter == 2))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.x < 0 && ((future_quarter == 1 && current_quarter == 2) ||
												  (future_quarter == 2 && current_quarter == 1))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.x > 0 && ((future_quarter == 3 && current_quarter == 0) ||
												  (future_quarter == 0 && current_quarter == 3))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}

	return target_h;
}

void ColorPickerShape::_reset_wheel_focus() { wheel_focused = true; }

void ColorPickerShape::update_circle_cursor(
	const Vector2& p_color_change_vector, const Vector2& p_center, const Vector2& p_hue_offset)
{
	if (circle_keyboard_joypad_picker_cursor_position == Vector2i()) {
		circle_keyboard_joypad_picker_cursor_position = p_center + p_hue_offset;
	}

	Vector2i potential_cursor_position =
		circle_keyboard_joypad_picker_cursor_position + p_color_change_vector;
	real_t potential_new_cursor_distance = p_center.distance_to(potential_cursor_position);
	real_t dist_pre = p_center.distance_to(circle_keyboard_joypad_picker_cursor_position);
	if (color_picker->s < 1 || potential_new_cursor_distance < dist_pre) {
		circle_keyboard_joypad_picker_cursor_position += p_color_change_vector;
		real_t dist = p_center.distance_to(circle_keyboard_joypad_picker_cursor_position);
		real_t rad = p_center.angle_to_point(circle_keyboard_joypad_picker_cursor_position);
		color_picker->h = ((rad >= 0) ? rad : (Math::TAU + rad)) / Math::TAU;
		color_picker->s = CLAMP(dist / p_center.x, 0, 1);
	}
	else {
		color_picker->h = get_h_on_circle_edge(p_color_change_vector);
		circle_keyboard_joypad_picker_cursor_position = Vector2i();
	}
}

void ColorPickerShape::initialize_controls()
{
	_initialize_controls();
	update_theme();
	is_initialized = true;
}

void ColorPickerShape::_initialize_controls()
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		sv_square = memnew(Control);
		hue_slider = memnew(Control);
		controls.push_back(sv_square);
		controls.push_back(hue_slider);
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		rectangle_margin = memnew(Control);
		square = memnew(Control);
		square_overlay = memnew(Control);
		value_slider = memnew(Control);
		rectangle_margin->add_child(square);
		square->add_child(square_overlay);
		controls.push_back(rectangle_margin);
		controls.push_back(value_slider);
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		wheel = memnew(Control);
		wheel_uv = memnew(Control);
		wheel->add_child(wheel_uv);
		controls.push_back(wheel);
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		circle = memnew(Control);
		circle_overlay = memnew(Control);
		value_slider = memnew(Control);
		circle->add_child(circle_overlay);
		controls.push_back(circle);
		controls.push_back(value_slider);
	} break;
	default:
		break;
	}
}

void ColorPickerShape::update_theme()
{
	const ColorPicker::ThemeCache& theme_cache = color_picker->theme_cache;
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		if (sv_square) {
			sv_square->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (hue_slider) {
			hue_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		if (rectangle_margin) {
			rectangle_margin->set_custom_minimum_size(
				Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (value_slider) {
			value_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		if (wheel) {
			wheel->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		if (circle) {
			circle->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (value_slider) {
			value_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	default:
		break;
	}
}

void ColorPickerShape::grab_focus()
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
		if (hue_slider) {
			hue_slider->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		if (square_overlay) {
			square_overlay->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_HSV_WHEEL:
		if (wheel_uv) {
			wheel_uv->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		if (circle_overlay) {
			circle_overlay->grab_focus();
		}
		break;
	default:
		break;
	}
}

void ColorPickerShape::_update_cursor(const Vector2& p_color_change_vector, bool p_is_echo)
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		if (sv_square && sv_square->has_focus()) {
			color_picker->s = CLAMP(color_picker->s + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->v = CLAMP(color_picker->v - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (hue_slider && hue_slider->has_focus()) {
			color_picker->h =
				CLAMP(color_picker->h + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE: {
		if (square_overlay && square_overlay->has_focus()) {
			color_picker->ok_hsl_h =
				CLAMP(color_picker->ok_hsl_h + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->ok_hsl_s =
				CLAMP(color_picker->ok_hsl_s - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_l = CLAMP(
				color_picker->ok_hsl_l + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		if (square_overlay && square_overlay->has_focus()) {
			color_picker->ok_hsl_h =
				CLAMP(color_picker->ok_hsl_h + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->ok_hsl_l =
				CLAMP(color_picker->ok_hsl_l - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_s = CLAMP(
				color_picker->ok_hsl_s + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		if (wheel_focused) {
			if (p_is_echo && rotate_next_echo_event) {
				color_picker->h = _get_h_on_wheel(-p_color_change_vector);
			}
			else {
				rotate_next_echo_event = false;
				color_picker->h = _get_h_on_wheel(p_color_change_vector);
			}
		}
		else {
			color_picker->s = CLAMP(color_picker->s + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->v = CLAMP(color_picker->v - p_color_change_vector.y / 100.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE: {
		if (circle_overlay && circle_overlay->has_focus()) {
			const Vector2 center = circle_overlay->get_size() / 2.0;
			const Vector2 hue_offset = center *
									   Vector2(Math::cos(color_picker->h * Math::TAU),
										   Math::sin(color_picker->h * Math::TAU)) *
									   color_picker->s;
			update_circle_cursor(p_color_change_vector, center, hue_offset);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->v =
				CLAMP(color_picker->v - p_color_change_vector.y * echo_multiplier / 100.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		if (circle_overlay && circle_overlay->has_focus()) {
			const Vector2 center = circle_overlay->get_size() / 2.0;
			const Vector2 hue_offset = center *
									   Vector2(Math::cos(color_picker->ok_hsl_h * Math::TAU),
										   Math::sin(color_picker->ok_hsl_h * Math::TAU)) *
									   color_picker->ok_hsl_s;
			update_circle_cursor(p_color_change_vector, center, hue_offset);
			color_picker->ok_hsl_h = color_picker->h;
			color_picker->ok_hsl_s = color_picker->s;
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_l = CLAMP(
				color_picker->ok_hsl_l - p_color_change_vector.y * echo_multiplier / 100.0, 0, 1);
		}
	} break;
	default:
		break;
	}
}

void ColorPickerShape::_sv_square_draw()
{
	draw_sv_square(sv_square, Rect2(Vector2(), sv_square->get_size()));
}

void ColorPickerShape::_hue_slider_draw()
{
	const Vector2 size = hue_slider->get_size();
	hue_slider->draw_texture_rect(color_picker->theme_cache.color_hue.ptr(),
		Rect2(0, 0, -size.y, size.x), false, Color(1, 1, 1), true);

	draw_focus_rect(hue_slider);

	int y = size.y * color_picker->h;
	const Color color = Color::from_hsv(color_picker->h, 1, 1);
	hue_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
}

void ColorPickerShape::_square_draw() {}

void ColorPickerShape::_square_overlay_draw()
{
	if (!square_overlay) {
		return;
	}
	const Rect2 rect = Rect2(Vector2(), square_overlay->get_size());
	const Vector2 end = rect.get_end();
	const float y_channel =
		(shape_type == ColorPicker::SHAPE_OK_HL_RECTANGLE) ? color_picker->ok_hsl_l
														   : color_picker->ok_hsl_s;

	Vector2 cursor_pos;
	cursor_pos.x =
		CLAMP(rect.position.x + rect.size.x * color_picker->ok_hsl_h, rect.position.x, end.x);
	cursor_pos.y = CLAMP(rect.position.y + rect.size.y * (1.0 - y_channel), rect.position.y, end.y);

	draw_focus_rect(square_overlay);
	draw_cursor(square_overlay, cursor_pos);
}

void ColorPickerShape::_value_slider_draw()
{
	if (!value_slider) {
		return;
	}

	const Vector2 size = value_slider->get_size();

	switch (shape_type) {
	case ColorPicker::SHAPE_VHS_CIRCLE: {
		PackedVector2Array points{
			Vector2(),
			Vector2(size.x, 0),
			size,
			Vector2(0, size.y),
		};

		Color color = Color::from_hsv(color_picker->h, color_picker->s, 1);
		PackedColorArray colors = {
			color,
			color,
			Color(),
			Color(),
		};

		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(color_picker->v, 0, 1));
		color.set_hsv(color_picker->h, 1, color_picker->v);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		const float ok_hsl_h = color_picker->ok_hsl_h;
		const float ok_hsl_l = color_picker->ok_hsl_l;

		PackedVector2Array points{
			Vector2(size.x, 0),
			Vector2(size.x, size.y * 0.5),
			size,
			Vector2(0, size.y),
			Vector2(0, size.y * 0.5),
			Vector2(),
		};

		Color color1 = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		Color color2 = Color::from_ok_hsl(ok_hsl_h, 0.5, ok_hsl_l);
		Color color3 = Color::from_ok_hsl(ok_hsl_h, 0, ok_hsl_l);
		PackedColorArray colors = {
			color1,
			color2,
			color3,
			color3,
			color2,
			color1,
		};
		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(color_picker->ok_hsl_s, 0, 1));
		const Color color = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		const float ok_hsl_h = color_picker->ok_hsl_h;
		const float ok_hsl_s = color_picker->ok_hsl_s;
		const float ok_hsl_l = color_picker->ok_hsl_l;

		PackedVector2Array points{
			Vector2(size.x, 0),
			Vector2(size.x, size.y * 0.5),
			size,
			Vector2(0, size.y),
			Vector2(0, size.y * 0.5),
			Vector2(),
		};

		Color color1 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 1);
		Color color2 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 0.5);
		Color color3 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 0);
		PackedColorArray colors = {
			color1,
			color2,
			color3,
			color3,
			color2,
			color1,
		};
		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(ok_hsl_l, 0, 1));
		const Color color = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	default:
		break;
	}
}

void ColorPickerShape::_wheel_input(const InputEvent&) {}

void ColorPickerShape::_wheel_draw()
{
	wheel->draw_rect(Rect2(Point2(), wheel->get_size()), Color(1, 1, 1));
}

void ColorPickerShape::_wheel_uv_draw()
{
	const Vector2 uv_size = wheel_uv->get_size();
	const Vector2 ring_radius = uv_size * Math::SQRT12 * WHEEL_RADIUS;
	const Vector2 center = uv_size * 0.5;

	const Rect2 uv_rect(center - ring_radius, ring_radius * 2.0);
	draw_sv_square(wheel_uv, uv_rect, !wheel_focused);
	if (wheel_focused) {
		draw_focus_circle(wheel_uv);
	}

	float radius = WHEEL_RADIUS * 2.0;
	radius += (1.0 - radius) * 0.5;
	const Vector2 cursor_pos =
		center + Vector2(center.x * Math::cos(color_picker->h * Math::TAU) * radius,
					 center.y * Math::sin(color_picker->h * Math::TAU) * radius);
	draw_cursor(wheel_uv, cursor_pos, false);
}

void ColorPickerShape::_circle_input(const InputEvent&) {}

void ColorPickerShape::_value_slider_input(const InputEvent&) {}

void ColorPickerShape::_circle_draw() {}

void ColorPickerShape::_circle_overlay_draw()
{
	if (!circle_overlay) {
		return;
	}
	draw_focus_circle(circle_overlay);
	if (shape_type == ColorPicker::SHAPE_OKHSL_CIRCLE) {
		draw_circle_cursor(circle_overlay, color_picker->ok_hsl_h, color_picker->ok_hsl_s);
	}
	else {
		draw_circle_cursor(circle_overlay, color_picker->h, color_picker->s);
	}
}

// --- color_picker.cpp ---

static inline bool is_color_overbright(const Color& color)
{
	return (color.r > 1.0) || (color.g > 1.0) || (color.b > 1.0);
}

static inline bool is_color_valid_hex(const Color& color)
{
	return !is_color_overbright(color) && color.r >= 0 && color.g >= 0 && color.b >= 0;
}

static inline String color_to_string(
	const Color& color, bool show_alpha = true, bool force_value_format = false)
{
	if (!force_value_format && !is_color_overbright(color)) {
		return "#" + color.to_html(show_alpha);
	}
	String t = "(" + String::num(color.r, 3) + ", " + String::num(color.g, 3) + ", " +
			   String::num(color.b, 3);
	if (show_alpha) {
		t += ", " + String::num(color.a, 3) + ")";
	}
	else {
		t += ")";
	}
	return t;
}

void ColorPicker::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void ColorPicker::set_focus_on_picker_shape()
{
	int idx = get_current_shape_index();
	if (idx >= 0 && idx < (int)shapes.size()) {
		shapes[idx]->grab_focus();
	}
}

void ColorPicker::set_old_color(const Color& p_color) { old_color = p_color; }

void ColorPicker::set_display_old_color(bool p_enabled) { display_old_color = p_enabled; }

bool ColorPicker::is_displaying_old_color() const { return display_old_color; }

bool ColorPicker::is_editing_alpha() const { return edit_alpha; }

bool ColorPicker::is_editing_intensity() const { return edit_intensity; }

void ColorPicker::_slider_drag_started() { currently_dragging = true; }

void ColorPicker::add_mode(ColorMode* p_mode) { modes.push_back(p_mode); }

void ColorPicker::add_shape(ColorPickerShape* p_shape) { shapes.push_back(p_shape); }

Slider* ColorPicker::get_slider(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, MODE_MAX, nullptr);
	return sliders[p_idx];
}

Vector<float> ColorPicker::get_active_slider_values()
{
	Vector<float> cur_values;
	for (int i = 0; i < current_slider_count; i++) {
		cur_values.push_back(sliders[i]->get_value());
	}
	cur_values.push_back(alpha_slider->get_value());
	return cur_values;
}

void ColorPicker::_copy_normalized_to_hsv_okhsl()
{
	if (!okhsl_cached) {
		ok_hsl_h = color_normalized.get_ok_hsl_h();
		ok_hsl_s = color_normalized.get_ok_hsl_s();
		ok_hsl_l = color_normalized.get_ok_hsl_l();
	}
	if (!hsv_cached) {
		h = color_normalized.get_h();
		s = color_normalized.get_s();
		v = color_normalized.get_v();
	}
	hsv_cached = false;
	okhsl_cached = false;
}

void ColorPicker::_copy_hsv_okhsl_to_normalized()
{
	int idx = get_current_shape_index();
	if (current_shape != SHAPE_NONE && idx >= 0 && idx < (int)shapes.size() &&
		shapes[idx]->is_ok_hsl()) {
		color_normalized.set_ok_hsl(ok_hsl_h, ok_hsl_s, ok_hsl_l, color_normalized.a);
	}
	else {
		color_normalized.set_hsv(h, s, v, color_normalized.a);
	}
}

Color ColorPicker::_color_apply_intensity(const Color& col) const
{
	if (intensity == 0.0f) {
		return col;
	}
	Color linear_color = col.srgb_to_linear();
	Color result;
	float multiplier = Math::pow(2, intensity);
	for (int i = 0; i < 3; i++) {
		result[i] = linear_color[i] * multiplier;
	}
	result.a = col.a;
	return result.linear_to_srgb();
}

void ColorPicker::_normalized_apply_intensity_to_color()
{
	color = _color_apply_intensity(color_normalized);
}

void ColorPicker::_copy_color_to_normalized_and_intensity()
{
	Color linear_color = color.srgb_to_linear();
	float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));
	for (int i = 0; i < 3; i++) {
		color_normalized[i] = linear_color[i] / multiplier;
	}
	color_normalized.a = linear_color.a;
	color_normalized = color_normalized.linear_to_srgb();
	intensity = Math::log2(multiplier);
}

void ColorPicker::_reset_sliders_theme()
{
	Ref<StyleBox> style_box_flat(memnew(StyleBox(StyleBox::STYLE_FLAT)));
	style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
	style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

	for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
		sliders[i]->begin_bulk_theme_override();
		sliders[i]->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_icon_override(
			SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_constant_override(
			SNAME("grabber_offset"), 8 * theme_cache.base_scale);
		if (!colorize_sliders) {
			sliders[i]->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
		}
		sliders[i]->end_bulk_theme_override();
	}

	alpha_slider->begin_bulk_theme_override();
	alpha_slider->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_icon_override(SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_constant_override(SNAME("grabber_offset"), 8 * theme_cache.base_scale);
	if (!colorize_sliders) {
		alpha_slider->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
	}
	alpha_slider->end_bulk_theme_override();
}

void ColorPicker::_text_copy_pressed()
{
	DisplayServer::clipboard_set(c_text->get_text());
}

Color ColorPicker::get_pick_color() const { return color; }

Color ColorPicker::get_old_color() const { return old_color; }

ColorPicker::PickerShapeType ColorPicker::get_picker_shape() const { return current_shape; }

inline int ColorPicker::_get_preset_size()
{
	int sep = preset_container ? preset_container->layout.h_separation : 0;
	return (int(get_minimum_size().width) - (sep * (PRESET_COLUMN_COUNT - 1))) /
		   PRESET_COLUMN_COUNT;
}

void ColorPicker::_load_palette()
{
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("ColorPalette", &extensions);

	file_dialog->set_title(ETR("Load Color Palette"));
	file_dialog->clear_filters();
	for (const String& K : extensions) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_current_file("");
	file_dialog->popup_centered_ratio();
}

Control* ColorPicker::get_slider_container() { return slider_gc; }

PackedColorArray ColorPicker::get_presets() const
{
	PackedColorArray arr;
	arr.resize(presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = presets.begin(); itr != presets.end(); ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

PackedColorArray ColorPicker::get_recent_presets() const
{
	PackedColorArray arr;
	arr.resize(recent_presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = recent_presets.begin(); itr != recent_presets.end();
		 ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

ColorPicker::ColorModeType ColorPicker::get_color_mode() const { return current_mode; }

void ColorPicker::set_colorize_sliders(bool p_colorize_sliders)
{
	if (colorize_sliders == p_colorize_sliders) {
		return;
	}

	colorize_sliders = p_colorize_sliders;

	if (colorize_sliders) {
		Ref<StyleBox> style_box_empty(memnew(StyleBox(StyleBox::STYLE_EMPTY)));

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_empty.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_empty.ptr());
	}
	else {
		Ref<StyleBox> style_box_flat(memnew(StyleBox(StyleBox::STYLE_FLAT)));
		style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
		style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_flat.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_flat.ptr());
	}
}

bool ColorPicker::is_colorizing_sliders() const { return colorize_sliders; }

void ColorPicker::set_deferred_mode(bool p_enabled) { deferred_mode_enabled = p_enabled; }

bool ColorPicker::is_deferred_mode() const { return deferred_mode_enabled; }

void ColorPicker::_sample_draw()
{
	Rect2 rect_new;
	Rect2 rect_old;

	if (display_old_color) {
		rect_new = Rect2(Point2(sample->get_size().width * 0.5, 0),
			Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		rect_old = Rect2(
			Point2(), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		if (old_color.a < 1.0) {
			sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_old, true);
		}

		sample->draw_rect(rect_old, old_color);

		if (!old_color.is_equal_approx(color)) {
			sample->draw_texture(theme_cache.sample_revert.ptr(),
				rect_old.size * 0.5 - theme_cache.sample_revert->get_size() * 0.5,
				Math::lerp(0.75f, old_color.get_luminance(), old_color.a) < 0.455
					? Color(1, 1, 1)
					: (Color(0.01, 0.01, 0.01)));

			sample->set_focus_mode(FOCUS_ALL);
		}
		else {
			sample->set_focus_mode(FOCUS_NONE);
		}

		if (is_color_overbright(color)) {
			sample->draw_texture(theme_cache.overbright_indicator.ptr(), Point2());
		}
	}
	else {
		rect_new =
			Rect2(Point2(), Size2(sample->get_size().width, sample->get_size().height * 0.95));
	}

	if (color.a < 1.0) {
		sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_new, true);
	}

	sample->draw_rect(rect_new, color);

	if (display_old_color && !old_color.is_equal_approx(color) && sample->has_focus(true)) {
		RID ci = sample->get_canvas_item();
		theme_cache.sample_focus->draw(ci, rect_old);
	}

	if (is_color_overbright(color)) {
		sample->draw_texture(
			theme_cache.overbright_indicator.ptr(), Point2(sample->get_size().width * 0.5, 0));
	}
}

void ColorPicker::_slider_draw(int p_which)
{
	if (colorize_sliders) {
		modes[current_mode]->slider_draw(p_which);
	}
}

void ColorPicker::_alpha_slider_draw()
{
	if (!colorize_sliders) {
		return;
	}
	Vector<Vector2> pos;
	pos.resize(4);
	Vector<Color> col;
	col.resize(4);
	Size2 size = alpha_slider->get_size();
	Color left_color;
	Color right_color;
	const real_t margin = 16 * theme_cache.base_scale;
	alpha_slider->draw_texture_rect(
		theme_cache.sample_bg.ptr(), Rect2(Point2(0, 0), Size2(size.x, margin)), true);

	left_color = color_normalized;
	left_color.a = 0;
	right_color = color_normalized;
	right_color.a = 1;

	col.set(0, left_color);
	col.set(1, right_color);
	col.set(2, right_color);
	col.set(3, left_color);
	pos.set(0, Vector2(0, 0));
	pos.set(1, Vector2(size.x, 0));
	pos.set(2, Vector2(size.x, margin));
	pos.set(3, Vector2(0, margin));

	alpha_slider->draw_polygon(pos, col);
}

void ColorPicker::_line_edit_input(const InputEvent& p_event)
{
	if (p_event.type == InputEventType::MOUSE_BUTTON && !p_event.is_pressed() &&
		p_event.mouse_button.button_index == MouseButton::LEFT) {
		line_edit_mouse_release = true;
	}
}

void ColorPicker::_text_changed(const String&) { text_changed = true; }

void ColorPicker::_block_input_on_popup_show()
{
	if (!SceneTree::get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(true);
	}
}

void ColorPicker::_enable_input_on_popup_hide()
{
	if (!SceneTree::get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(false);
	}
}

bool ColorPicker::are_swatches_enabled() const { return can_add_swatches; }

bool ColorPicker::are_presets_visible() const { return presets_visible; }

bool ColorPicker::are_modes_visible() const { return color_modes_visible; }

bool ColorPicker::is_sampler_visible() const { return sampler_visible; }

bool ColorPicker::are_sliders_visible() const { return sliders_visible; }

bool ColorPicker::is_hex_visible() const { return hex_visible; }

void ColorPicker::_req_permission()
{
#ifdef MACOS_ENABLED
	OS::request_permission("macos.permission.RECORD_SCREEN");
#endif
}

ColorPicker::ColorPicker()
{
	layout.type = LayoutConfig::LAYOUT_BOX;
	layout.orientation = VERTICAL;
}

ColorPicker::~ColorPicker()
{
	for (ColorMode* mode : modes) {
		memdelete(mode);
	}
	for (ColorPickerShape* shape : shapes) {
		memdelete(shape);
	}
}

void ColorPickerButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
		AccessibilityServer::update_set_popup_type(
			ae, AccessibilityServerEnums::AccessibilityPopupType::POPUP_DIALOG);
		AccessibilityServer::update_set_color_value(ae, color);
	} break;

	case NOTIFICATION_DRAW: {
		if (theme_cache.normal_style.is_valid()) {
			const Rect2 r = Rect2(theme_cache.normal_style->get_offset(),
				get_size() - theme_cache.normal_style->get_minimum_size());
			if (theme_cache.background_icon.is_valid()) {
				draw_texture_rect(theme_cache.background_icon.ptr(), r, true);
			}
			draw_rect(r, color);

			if ((color.r > 1 || color.g > 1 || color.b > 1) &&
				theme_cache.overbright_indicator.is_valid()) {
				draw_texture(theme_cache.overbright_indicator.ptr(),
					theme_cache.normal_style->get_offset());
			}
		}
	} break;

	case NOTIFICATION_WM_CLOSE_REQUEST: {
		if (popup) {
			popup->hide();
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (popup && !is_visible_in_tree()) {
			popup->hide();
		}
	} break;
	}
}

void ColorPickerButton::_update_picker()
{
	if (!popup) {
		popup = memnew(Popup(true));
		picker = memnew(ColorPicker);
		popup->add_child(picker);
		add_child(popup, false, INTERNAL_MODE_FRONT);
	}
}

Popup* ColorPickerButton::get_popup()
{
	_update_picker();
	return popup;
}

ColorPicker* ColorPickerButton::get_picker()
{
	_update_picker();
	return picker;
}

Color ColorPickerButton::get_pick_color() const { return color; }

bool ColorPickerButton::is_editing_alpha() const { return edit_alpha; }

bool ColorPickerButton::is_editing_intensity() const { return edit_intensity; }

ColorPickerButton::ColorPickerButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);
}

// --- color_rect.cpp ---

Color ColorRect::get_color() const { return color; }

void ColorRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_color_value(ae, color);
	} break;

	case NOTIFICATION_DRAW: {
		drawing = true;
		printf(">>> ColorRect NOTIFICATION_DRAW running! size = (%f, %f)\n", get_size().x,
			get_size().y);
		draw_rect(Rect2(Point2(), get_size()), color);
		drawing = false;
	} break;
	}
}

void ColorRect::notification(int p_what) { _notification(p_what); }

void ColorRect::queue_redraw()
{
	if (!canvas_item.is_valid()) {
		return;
	}
	Renderer::canvas_item_clear(canvas_item);
	drawing = true;
	_notification(NOTIFICATION_DRAW);
	drawing = false;
}

void ColorRect::set_color(const Color& p_color)
{
	color = p_color;
	queue_redraw(); // Statically calls ColorRect::queue_redraw()
}

// ============================================================================
// Merged from scene/gui/graph_edit.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/os/keyboard.h"
#include "graph_edit.h"
#include "scene/2d/line_2d.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/material.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include <cfloat>
#include "core/input/shortcut.h"
#include "scene/main/viewport.h"
#include "scene/gui/graph_edit.h"
#include "scene/main/scene_tree.h"
#include "servers/display/accessibility_server.h"

constexpr int MINIMAP_OFFSET = 12;
constexpr int MINIMAP_PADDING = 5;
constexpr int MIN_DRAG_DISTANCE_FOR_VALID_CONNECTION = 20;
constexpr int MAX_CONNECTION_LINE_CURVE_TESSELATION_STAGES = 5;
constexpr int GRID_MINOR_STEPS_PER_MAJOR_LINE = 10;
constexpr int GRID_MINOR_STEPS_PER_MAJOR_DOT = 5;
constexpr int GRID_MIN_SNAPPING_DISTANCE = 2;
constexpr int GRID_MAX_SNAPPING_DISTANCE = 100;

bool GraphEditFilter::has_point(const Point2& p_point) const { return ge->_filter_input(p_point); }

GraphEditFilter::GraphEditFilter(GraphEdit* p_edit) { ge = p_edit; }

Control::CursorShape GraphEditMinimap::get_cursor_shape(const Point2& p_pos) const
{
	if (is_resizing || (p_pos.x < theme_cache.resizer->get_width() &&
						   p_pos.y < theme_cache.resizer->get_height())) {
		return CURSOR_FDIAGSIZE;
	}

	return Control::get_cursor_shape(p_pos);
}

void GraphEditMinimap::update_minimap()
{
	Vector2 graph_offset = _get_graph_offset();
	Vector2 graph_size = _get_graph_size();

	camera_position = ge->get_scroll_offset() - graph_offset;
	camera_size = ge->get_size();

	Vector2 render_size = _get_render_size();
	float target_ratio = render_size.width / render_size.height;
	float graph_ratio = graph_size.width / graph_size.height;

	graph_proportions = graph_size;
	graph_padding = Vector2(0, 0);
	if (graph_ratio > target_ratio) {
		graph_proportions.width = graph_size.width;
		graph_proportions.height = graph_size.width / target_ratio;
		graph_padding.y = Math::abs(graph_size.height - graph_proportions.y) / 2;
	}
	else {
		graph_proportions.width = graph_size.height * target_ratio;
		graph_proportions.height = graph_size.height;
		graph_padding.x = Math::abs(graph_size.width - graph_proportions.x) / 2;
	}

	// This centers minimap inside the minimap rectangle.
	minimap_offset = minimap_padding + _convert_from_graph_position(graph_padding);
}

Rect2 GraphEditMinimap::get_camera_rect()
{
	Vector2 camera_center =
		_convert_from_graph_position(camera_position + camera_size / 2) + minimap_offset;
	Vector2 camera_viewport = _convert_from_graph_position(camera_size);
	Vector2 camera_pos = (camera_center - camera_viewport / 2);
	return Rect2(camera_pos, camera_viewport);
}

Vector2 GraphEditMinimap::_get_render_size()
{
	if (!is_inside_tree()) {
		return Vector2(0, 0);
	}

	return get_size() - 2 * minimap_padding;
}

Vector2 GraphEditMinimap::_get_graph_offset() { return ge->min_scroll_offset; }

Vector2 GraphEditMinimap::_get_graph_size()
{
	Vector2 graph_size = ge->max_scroll_offset - ge->min_scroll_offset;

	if (graph_size.width == 0) {
		graph_size.width = 1;
	}
	if (graph_size.height == 0) {
		graph_size.height = 1;
	}

	return graph_size;
}

Vector2 GraphEditMinimap::_convert_from_graph_position(const Vector2& p_position)
{
	Vector2 map_position = Vector2(0, 0);
	Vector2 render_size = _get_render_size();

	map_position.x = p_position.x * render_size.width / graph_proportions.x;
	map_position.y = p_position.y * render_size.height / graph_proportions.y;

	return map_position;
}

Vector2 GraphEditMinimap::_convert_to_graph_position(const Vector2& p_position)
{
	Vector2 graph_position = Vector2(0, 0);
	Vector2 render_size = _get_render_size();

	graph_position.x = p_position.x * graph_proportions.x / render_size.width;
	graph_position.y = p_position.y * graph_proportions.y / render_size.height;

	return graph_position;
}

void GraphEditMinimap::_adjust_graph_scroll(const Vector2& p_offset)
{
	Vector2 graph_offset = _get_graph_offset();
	ge->set_scroll_offset(p_offset + graph_offset - camera_size / 2);
}

GraphEditMinimap::GraphEditMinimap(GraphEdit* p_edit)
{
	ge = p_edit;

	minimap_padding = Vector2(MINIMAP_PADDING, MINIMAP_PADDING);
	minimap_offset = minimap_padding + _convert_from_graph_position(graph_padding);
}

Ref<Shader> GraphEdit::default_connections_shader;

void GraphEdit::init_shaders()
{
	default_connections_shader.instantiate();
	default_connections_shader->set_code(R"(
// Connection lines shader.
shader_type canvas_item;
render_mode blend_mix;

uniform vec4 rim_color : source_color;
uniform int from_type;
uniform int to_type;
uniform float line_width;

void fragment(){
	float fake_aa_width = 1.5/line_width;
	float rim_width = 1.5/line_width;

	float dist = abs(UV.y - 0.5);
	float alpha = smoothstep(0.5, 0.5-fake_aa_width, dist);
	vec4 final_color = mix(rim_color, COLOR, smoothstep(0.5-rim_width, 0.5-fake_aa_width-rim_width, dist));
	COLOR = vec4(final_color.rgb, final_color.a*alpha);
}
)");
}

void GraphEdit::finish_shaders() { default_connections_shader.unref(); }

Control::CursorShape GraphEdit::get_cursor_shape(const Point2& p_pos) const
{
	if (moving_selection) {
		return CURSOR_MOVE;
	}

	return Control::get_cursor_shape(p_pos);
}

bool GraphEdit::is_node_connected(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	for (const Ref<Connection>& conn : connection_map[p_from]) {
		if (conn->from_node == p_from && conn->from_port == p_from_port && conn->to_node == p_to &&
			conn->to_port == p_to_port) {
			return true;
		}
	}

	return false;
}

const Vector<Ref<GraphEdit::Connection>>& GraphEdit::get_connections() const { return connections; }

int GraphEdit::get_connection_count(const String& p_node, int p_port)
{
	int count = 0;
	for (const Ref<Connection>& conn : connections) {
		if ((conn->from_node == p_node && conn->from_port == p_port) ||
			(conn->to_node == p_node && conn->to_port == p_port)) {
			count += 1;
		}
	}
	return count;
}

Vector2 GraphEdit::get_scroll_offset() const { return scroll_offset; }

void GraphEdit::_graph_element_visibility_changed(GraphNode* p_graph_element)
{
	if (p_graph_element->is_selected() && !p_graph_element->is_visible()) {
		p_graph_element->set_selected(false);
	}
}

void GraphEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

bool GraphEdit::is_in_input_hotzone(GraphNode* p_graph_node, int p_port_idx,
	const Vector2& p_mouse_pos, const Vector2i& p_port_size)
{
	Vector2 pos =
		p_graph_node->get_input_port_position(p_port_idx) * zoom + p_graph_node->get_position();
	return is_in_port_hotzone(pos / zoom, p_mouse_pos, p_port_size, true);
}

bool GraphEdit::is_in_output_hotzone(GraphNode* p_graph_node, int p_port_idx,
	const Vector2& p_mouse_pos, const Vector2i& p_port_size)
{
	if (p_graph_node->is_resizable()) {
		Ref<Texture2D> resizer = p_graph_node->theme_cache.resizer;
		Rect2 resizer_rect = Rect2(
			p_graph_node->get_position() / zoom + p_graph_node->get_size() - resizer->get_size(),
			resizer->get_size());
		if (resizer_rect.has_point(p_mouse_pos)) {
			return false;
		}
	}
	Vector2 pos =
		p_graph_node->get_output_port_position(p_port_idx) * zoom + p_graph_node->get_position();
	return is_in_port_hotzone(pos / zoom, p_mouse_pos, p_port_size, false);
}

PackedVector2Array GraphEdit::get_connection_line(const Vector2& p_from, const Vector2& p_to) const
{
	float x_diff = (p_to.x - p_from.x);
	float cp_offset = x_diff * lines_curvature;
	if (x_diff < 0) {
		cp_offset *= -1;
	}

	Curve2D curve;
	curve.add_point(p_from);
	curve.set_point_out(0, Vector2(cp_offset, 0));
	curve.add_point(p_to);
	curve.set_point_in(1, Vector2(-cp_offset, 0));

	if (lines_curvature > 0) {
		return curve.tessellate(MAX_CONNECTION_LINE_CURVE_TESSELATION_STAGES, 2.0);
	}
	else {
		return curve.tessellate(1);
	}
}

Ref<GraphEdit::Connection> GraphEdit::get_closest_connection_at_point(
	const Vector2& p_point, float p_max_distance) const
{
	Vector2 transformed_point = p_point + get_scroll_offset();

	Ref<GraphEdit::Connection> closest_connection;
	float closest_distance = p_max_distance;
	for (const Ref<Connection>& conn : connections) {
		if (conn->_cache.aabb.distance_to(transformed_point) > p_max_distance) {
			continue;
		}

		Vector<Vector2> points =
			get_connection_line(conn->_cache.from_pos * zoom, conn->_cache.to_pos * zoom);
		for (int i = 0; i < points.size() - 1; i++) {
			const real_t distance =
				Geometry2D::get_distance_to_segment(transformed_point, points[i], points[i + 1]);
			if (distance <= lines_thickness * 0.5 + p_max_distance && distance < closest_distance) {
				closest_connection = conn;
				closest_distance = distance;
			}
		}
	}

	return closest_connection;
}

List<Ref<GraphEdit::Connection>> GraphEdit::get_connections_intersecting_with_rect(
	const Rect2& p_rect) const
{
	Rect2 transformed_rect = p_rect;
	transformed_rect.position += get_scroll_offset();

	List<Ref<Connection>> intersecting_connections;
	for (const Ref<Connection>& conn : connections) {
		if (!conn->_cache.aabb.intersects(transformed_rect)) {
			continue;
		}

		Vector<Vector2> points =
			get_connection_line(conn->_cache.from_pos * zoom, conn->_cache.to_pos * zoom);
		for (int i = 0; i < points.size() - 1; i++) {
			if (Geometry2D::segment_intersects_rect(points[i], points[i + 1], transformed_rect)) {
				intersecting_connections.push_back(conn);
				break;
			}
		}
	}
	return intersecting_connections;
}

void GraphEdit::_draw_minimap_connection_line(const Vector2& p_from_graph_position,
	const Vector2& p_to_graph_position, const Color& p_from_color, const Color& p_to_color)
{
	Vector<Vector2> points = get_connection_line(p_from_graph_position, p_to_graph_position);
	ERR_FAIL_COND_MSG(points.size() < 2, "\"_get_connection_line()\" returned an invalid line.");
	// Convert to minimap points.
	for (Vector2& point : points) {
		point = minimap->_convert_from_graph_position(point) + minimap->minimap_offset;
	}

	// Setup polyline colors.
	LocalVector<Color> colors;
	colors.reserve(points.size());
	const Vector2& from = points[0];
	const Vector2& to = points[points.size() - 1];
	float length_inv = 1.0 / (from).distance_to(to);
	for (const Vector2& point : points) {
		float normalized_curve_position = from.distance_to(point) * length_inv;
		colors.push_back(p_from_color.lerp(p_to_color, normalized_curve_position));
	}

	minimap->draw_polyline_colors(points, Vector<Color>(colors), 0.5, lines_antialiased);
}

void GraphEdit::_top_layer_draw()
{
	if (!box_selecting) {
		return;
	}

	top_layer->draw_rect(box_selecting_rect, theme_cache.selection_fill);
	top_layer->draw_rect(box_selecting_rect, theme_cache.selection_stroke, false);
}

void GraphEdit::_draw_grid()
{
	Vector2 offset = get_scroll_offset() / zoom;
	Size2 size = get_size() / zoom;

	Point2i from_pos = (offset / float(snapping_distance)).floor();
	Point2i len = (size / float(snapping_distance)).floor() + Vector2(1, 1);

	switch (grid_pattern) {
	case GRID_PATTERN_LINES: {
		for (int i = from_pos.x; i < from_pos.x + len.x; i++) {
			Color color;

			if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_LINE == 0) {
				color = theme_cache.grid_major;
			}
			else {
				color = theme_cache.grid_minor;
			}

			float base_offset = i * snapping_distance * zoom - offset.x * zoom;
			draw_line(Vector2(base_offset, 0), Vector2(base_offset, get_size().height), color);
		}

		for (int i = from_pos.y; i < from_pos.y + len.y; i++) {
			Color color;

			if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_LINE == 0) {
				color = theme_cache.grid_major;
			}
			else {
				color = theme_cache.grid_minor;
			}

			float base_offset = i * snapping_distance * zoom - offset.y * zoom;
			draw_line(Vector2(0, base_offset), Vector2(get_size().width, base_offset), color);
		}
	} break;
	case GRID_PATTERN_DOTS: {
		Color transparent_grid_minor = theme_cache.grid_minor;
		transparent_grid_minor.a *= CLAMP(1.0 * (zoom - 0.4), 0, 1);

		// Minor dots.
		if (transparent_grid_minor.a != 0) {
			for (int i = from_pos.x; i < from_pos.x + len.x; i++) {
				for (int j = from_pos.y; j < from_pos.y + len.y; j++) {
					if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_DOT == 0 &&
						Math::abs(j) % GRID_MINOR_STEPS_PER_MAJOR_DOT == 0) {
						continue;
					}

					float base_offset_x = i * snapping_distance * zoom - offset.x * zoom;
					float base_offset_y = j * snapping_distance * zoom - offset.y * zoom;

					draw_rect(
						Rect2(base_offset_x - 1, base_offset_y - 1, 3, 3), transparent_grid_minor);
				}
			}
		}

		// Major dots.
		if (theme_cache.grid_major.a != 0) {
			for (int i = from_pos.x - from_pos.x % GRID_MINOR_STEPS_PER_MAJOR_DOT;
				 i < from_pos.x + len.x; i += GRID_MINOR_STEPS_PER_MAJOR_DOT) {
				for (int j = from_pos.y - from_pos.y % GRID_MINOR_STEPS_PER_MAJOR_DOT;
					 j < from_pos.y + len.y; j += GRID_MINOR_STEPS_PER_MAJOR_DOT) {
					float base_offset_x = i * snapping_distance * zoom - offset.x * zoom;
					float base_offset_y = j * snapping_distance * zoom - offset.y * zoom;

					draw_rect(
						Rect2(base_offset_x - 1, base_offset_y - 1, 3, 3), theme_cache.grid_major);
				}
			}
		}

	} break;
	}
}

void GraphEdit::_zoom_callback(float p_zoom_factor, Vector2 p_origin, const InputEvent& p_event)
{
	// We need to invalidate all connections since we don't know whether
	// the user is zooming/panning at the same time.
	_invalidate_connection_line_cache();

	set_zoom_custom(zoom * p_zoom_factor, p_origin);
}

void GraphEdit::set_panning_scheme(PanningScheme p_scheme)
{
	panning_scheme = p_scheme;
	panner->set_control_scheme((ViewPanner::ControlScheme)p_scheme);
}

GraphEdit::PanningScheme GraphEdit::get_panning_scheme() const { return panning_scheme; }

void GraphEdit::set_zoom(float p_zoom) { set_zoom_custom(p_zoom, get_size() / 2); }

float GraphEdit::get_zoom() const { return zoom; }

void GraphEdit::set_zoom_step(float p_zoom_step)
{
	p_zoom_step = std::abs(p_zoom_step);
	ERR_FAIL_COND(!std::isfinite(p_zoom_step));
	if (zoom_step == p_zoom_step) {
		return;
	}

	zoom_step = p_zoom_step;
	panner->set_scroll_zoom_factor(zoom_step);
}

float GraphEdit::get_zoom_step() const { return zoom_step; }

void GraphEdit::set_zoom_min(float p_zoom_min)
{
	ERR_FAIL_COND_MSG(
		p_zoom_min > zoom_max, "Cannot set min zoom level greater than max zoom level.");

	if (zoom_min == p_zoom_min) {
		return;
	}

	zoom_min = p_zoom_min;
	set_zoom(zoom);
}

float GraphEdit::get_zoom_min() const { return zoom_min; }

void GraphEdit::set_zoom_max(float p_zoom_max)
{
	ERR_FAIL_COND_MSG(
		p_zoom_max < zoom_min, "Cannot set max zoom level lesser than min zoom level.");

	if (zoom_max == p_zoom_max) {
		return;
	}

	zoom_max = p_zoom_max;
	set_zoom(zoom);
}

float GraphEdit::get_zoom_max() const { return zoom_max; }

void GraphEdit::set_right_disconnects(bool p_enable) { right_disconnects = p_enable; }

bool GraphEdit::is_right_disconnects_enabled() const { return right_disconnects; }

void GraphEdit::add_valid_right_disconnect_type(int p_type)
{
	valid_right_disconnect_types.insert(p_type);
}

void GraphEdit::remove_valid_right_disconnect_type(int p_type)
{
	valid_right_disconnect_types.erase(p_type);
}

void GraphEdit::add_valid_left_disconnect_type(int p_type)
{
	valid_left_disconnect_types.insert(p_type);
}

void GraphEdit::remove_valid_left_disconnect_type(int p_type)
{
	valid_left_disconnect_types.erase(p_type);
}

void GraphEdit::_zoom_minus() { set_zoom(zoom / zoom_step); }

void GraphEdit::_zoom_reset() { set_zoom(1); }

void GraphEdit::_zoom_plus() { set_zoom(zoom * zoom_step); }

void GraphEdit::_update_zoom_label()
{
	int zoom_percent = static_cast<int>(Math::round(zoom * 100));
	String zoom_text = itos(zoom_percent) + "%";
	zoom_label->set_text(zoom_text);
}

void GraphEdit::_invalidate_connection_line_cache()
{
	for (Ref<Connection>& conn : connections) {
		conn->_cache.dirty = true;
	}
}

float GraphEdit::_get_shader_line_width() { return lines_thickness * theme_cache.base_scale + 4.0; }

void GraphEdit::add_valid_connection_type(int p_type, int p_with_type)
{
	ConnectionType ct(p_type, p_with_type);
	valid_connection_types.insert(ct);
}

void GraphEdit::remove_valid_connection_type(int p_type, int p_with_type)
{
	ConnectionType ct(p_type, p_with_type);
	valid_connection_types.erase(ct);
}

bool GraphEdit::is_valid_connection_type(int p_type, int p_with_type) const
{
	ConnectionType ct(p_type, p_with_type);
	return valid_connection_types.has(ct);
}

bool GraphEdit::is_snapping_enabled() const { return snapping_enabled; }

int GraphEdit::get_snapping_distance() const { return snapping_distance; }

bool GraphEdit::is_showing_grid() const { return show_grid; }

GraphEdit::GridPattern GraphEdit::get_grid_pattern() const { return grid_pattern; }

void GraphEdit::_snapping_toggled() { snapping_enabled = toggle_snapping_button->is_pressed(); }

Vector2 GraphEdit::get_minimap_size() const { return minimap->get_size(); }

float GraphEdit::get_minimap_opacity() const
{
	Color minimap_modulate = minimap->get_modulate();
	return minimap_modulate.a;
}

bool GraphEdit::is_minimap_enabled() const { return minimap_button->is_pressed(); }

bool GraphEdit::is_showing_menu() const { return show_menu; }

bool GraphEdit::is_showing_zoom_label() const { return show_zoom_label; }

bool GraphEdit::is_showing_zoom_buttons() const { return show_zoom_buttons; }

bool GraphEdit::is_showing_grid_buttons() const { return show_grid_buttons; }

bool GraphEdit::is_showing_minimap_button() const { return show_minimap_button; }

bool GraphEdit::is_showing_arrange_button() const { return show_arrange_button; }

float GraphEdit::get_connection_lines_curvature() const { return lines_curvature; }

float GraphEdit::get_connection_lines_thickness() const { return lines_thickness; }

bool GraphEdit::is_connection_lines_antialiased() const { return lines_antialiased; }

Control* GraphEdit::get_menu_hbox() { return menu_hbox; }

Ref<ViewPanner> GraphEdit::get_panner() { return panner; }

void GraphEdit::set_warped_panning(bool p_warped)
{
	warped_panning = p_warped;

	update_warped_panning();
}

void GraphEdit::update_warped_panning() { panner->setup_warped_panning(this, warped_panning); }

void GraphEdit::arrange_nodes() { arranger->arrange_nodes(); }

bool GraphEdit::is_node_hover_valid(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	return true;
}

void GraphEdit::set_zoom_custom(float p_zoom, const Vector2& p_center) {}

bool GraphEdit::is_in_port_hotzone(Vector2 const&, Vector2 const&, Vector2i const&, bool) { return true; }

void GraphEdit::set_scroll_offset(Vector2 const&) {}

bool GraphEdit::_filter_input(Vector2 const&) { return false; }

Error GraphEdit::connect_node(String const&, int, String const&, int, bool) { return OK; }

void GraphEdit::add_child_notify(Node* p_child) {}

void GraphEdit::remove_child_notify(Node* p_child) {}

GraphNode* GraphEdit::get_element_frame(const String& p_attached_graph_element)
{
	if (!linked_parent_map.has(p_attached_graph_element)) {
		return nullptr;
	}
	Node* parent = get_node_or_null(NodePath(linked_parent_map[p_attached_graph_element]));
	GraphNode* frame = dynamic_cast<GraphNode*>(parent);
	return (frame && frame->is_frame()) ? frame : nullptr;
}

Vector<String> GraphEdit::get_attached_nodes_of_frame(const String& p_graph_frame) const
{
	Vector<String> attached_nodes;
	if (!frame_attached_nodes.has(p_graph_frame)) {
		return attached_nodes;
	}
	for (const String& elem : frame_attached_nodes[p_graph_frame]) {
		attached_nodes.push_back(elem);
	}
	return attached_nodes;
}

// --- graph_edit.compat.inc ---

#ifndef DISABLE_DEPRECATED


bool GraphEdit::_is_arrange_nodes_button_hidden_bind_compat_81582() const
{
	return !is_showing_arrange_button();
}

PackedVector2Array GraphEdit::_get_connection_line_bind_compat_86158(
	const Vector2& p_from, const Vector2& p_to)
{
	return get_connection_line(p_from, p_to);
}

Error GraphEdit::_connect_node_bind_compat_97449(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	return connect_node(p_from, p_from_port, p_to, p_to_port);
}

void GraphEdit::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- graph_edit_arranger.cpp ---

int GraphEditArranger::_set_operations(
	SET_OPERATIONS p_operation, HashSet<String>& r_u, const HashSet<String>& r_v)
{
	switch (p_operation) {
	case GraphEditArranger::IS_EQUAL: {
		for (const String& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return r_u.size() == r_v.size();
	} break;
	case GraphEditArranger::IS_SUBSET: {
		if (r_u.size() == r_v.size() && !r_u.size()) {
			return 1;
		}
		for (const String& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return 1;
	} break;
	case GraphEditArranger::DIFFERENCE: {
		Vector<String> common;
		for (const String& E : r_u) {
			if (r_v.has(E)) {
				common.append(E);
			}
		}
		for (const String& E : common) {
			r_u.erase(E);
		}
		return r_u.size();
	} break;
	case GraphEditArranger::UNION: {
		for (const String& E : r_v) {
			if (!r_u.has(E)) {
				r_u.insert(E);
			}
		}
		return r_u.size();
	} break;
	default:
		break;
	}
	return -1;
}

HashMap<int, Vector<String>> GraphEditArranger::_layering(
	const HashSet<String>& r_selected_nodes,
	const HashMap<String, HashSet<String>>& r_upper_neighbours)
{
	HashMap<int, Vector<String>> l;

	HashSet<String> p(r_selected_nodes);
	HashSet<String> q(r_selected_nodes);
	HashSet<String> u;
	HashSet<String> z;
	int current_layer = 0;
	bool selected = false;

	while (!_set_operations(GraphEditArranger::IS_EQUAL, q, u)) {
		_set_operations(GraphEditArranger::DIFFERENCE, p, u);
		for (const String& E : p) {
			HashSet<String> n(r_upper_neighbours[E]);
			if (_set_operations(GraphEditArranger::IS_SUBSET, n, z)) {
				Vector<String> t;
				t.push_back(E);
				if (!l.has(current_layer)) {
					l.insert(current_layer, Vector<String>{});
				}
				selected = true;
				t.append_array(l[current_layer]);
				l.insert(current_layer, t);
				u.insert(E);
			}
		}
		if (!selected) {
			current_layer++;
			uint32_t previous_size_z = z.size();
			_set_operations(GraphEditArranger::UNION, z, u);
			if (z.size() == previous_size_z) {
				WARN_PRINT(
					"Graph contains cycle(s). The cycle(s) will not be rearranged accurately.");
				Vector<String> t;
				if (l.has(0)) {
					t.append_array(l[0]);
				}
				for (const String& E : p) {
					t.push_back(E);
				}
				l.insert(0, t);
				break;
			}
		}
		selected = false;
	}

	return l;
}



void GraphEditArranger::arrange_nodes() {}

// --- view_panner.cpp ---

void ViewPanner::release_pan_key()
{
	pan_key_pressed = false;
	if (drag_type == DragType::DRAG_TYPE_PAN) {
		drag_type = DragType::DRAG_TYPE_NONE;
	}
}

void ViewPanner::set_control_scheme(ControlScheme p_scheme) { control_scheme = p_scheme; }

void ViewPanner::set_enable_rmb(bool p_enable) { enable_rmb = p_enable; }

void ViewPanner::set_pan_shortcut(Ref<Shortcut> p_shortcut)
{
	pan_view_shortcut = p_shortcut;
	pan_key_pressed = false;
}

void ViewPanner::set_simple_panning_enabled(bool p_enabled) { simple_panning_enabled = p_enabled; }

void ViewPanner::set_scroll_speed(int p_scroll_speed)
{
	ERR_FAIL_COND(p_scroll_speed <= 0);
	scroll_speed = p_scroll_speed;
}

void ViewPanner::set_scroll_zoom_factor(float p_scroll_zoom_factor)
{
	ERR_FAIL_COND(p_scroll_zoom_factor <= 1.0);
	scroll_zoom_factor = p_scroll_zoom_factor;
}

void ViewPanner::set_pan_axis(PanAxis p_pan_axis) { pan_axis = p_pan_axis; }

void ViewPanner::set_zoom_style(ZoomStyle p_zoom_style) { zoom_style = p_zoom_style; }

void ViewPanner::setup(ControlScheme p_scheme, Ref<Shortcut> p_shortcut, bool p_simple_panning)
{
	set_control_scheme(p_scheme);
	set_pan_shortcut(p_shortcut);
	set_simple_panning_enabled(p_simple_panning);
}

void ViewPanner::setup_warped_panning(Node* p_owner, bool p_allowed)
{
	warped_panning_owner = p_allowed ? p_owner : nullptr;
}

bool ViewPanner::is_panning() const
{
	return (drag_type == DragType::DRAG_TYPE_PAN) || pan_key_pressed;
}

void ViewPanner::set_force_drag(bool p_force) { force_drag = p_force; }



bool ViewPanner::gui_input(const InputEvent&, Rect2) { return false; }

// --- graph_node.cpp ---

void GraphNode::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;

	if (mode == MODE_ELEMENT) {
		if (titlebar_hbox) {
			remove_child(titlebar_hbox);
			memdelete(titlebar_hbox);
			titlebar_hbox = nullptr;
			title_label = nullptr;
		}
		set_focus_mode(FOCUS_NONE);
	}
	else {
		if (!titlebar_hbox) {
			titlebar_hbox = Control::create_hbox();
			titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->set_use_parent_material(true);
			add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

			title_label = memnew(Label);
			title_label->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->add_child(title_label);
		}
		if (title_label) {
			title_label->set_theme_type_variation(
				mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
			title_label->set_horizontal_alignment(
				mode == MODE_FRAME ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
			title_label->set_use_parent_material(mode == MODE_FRAME);
			title_label->set_text(title);
		}
		set_focus_mode(mode == MODE_NODE ? FOCUS_ACCESSIBILITY : FOCUS_NONE);
	}
	queue_redraw();
}

#ifdef TOOLS_ENABLED
void GraphNode::_edit_set_position(const Point2& p_position) {}
#endif

void GraphNode::set_position_offset(const Vector2& p_offset)
{
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	queue_redraw();
}

void GraphNode::set_selected(bool p_selected)
{
	if (!is_selectable() && p_selected) {
		return;
	}
	if (selected == p_selected) {
		return;
	}
	selected = p_selected;
	queue_redraw();
}

void GraphNode::set_drag(bool p_drag)
{
	if (p_drag) {
		drag_from = get_position_offset();
	}
}

void GraphNode::set_resizable(bool p_enable)
{
	if (resizable == p_enable) {
		return;
	}
	resizable = p_enable;
	queue_redraw();
}

void GraphNode::set_selectable(bool p_selectable)
{
	if (!p_selectable) {
		set_selected(false);
	}
	selectable = p_selectable;
}

void GraphNode::set_title(const String& p_title)
{
	if (title == p_title) {
		return;
	}
	title = p_title;
	if (title_label) {
		title_label->set_text(title);
	}
	update_minimum_size();
}

Size2 GraphNode::get_titlebar_size() const
{
	if (!titlebar_hbox) {
		return Size2();
	}
	Size2 min_size = titlebar_hbox->get_bound_minimum_size();
	if (theme_cache.titlebar.is_valid()) {
		min_size += theme_cache.titlebar->get_minimum_size();
	}
	return min_size;
}

void GraphNode::set_autoshrink_enabled(bool p_enable)
{
	if (autoshrink_enabled == p_enable) {
		return;
	}
	autoshrink_enabled = p_enable;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_autoshrink_margin(const int& p_margin)
{
	if (autoshrink_margin == p_margin) {
		return;
	}
	autoshrink_margin = p_margin;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_tint_color_enabled(bool p_enable)
{
	if (tint_color_enabled == p_enable) {
		return;
	}
	tint_color_enabled = p_enable;
	queue_redraw();
}

void GraphNode::set_tint_color(const Color& p_tint_color)
{
	if (tint_color == p_tint_color) {
		return;
	}
	tint_color = p_tint_color;
	queue_redraw();
}

void GraphNode::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
	} break;

	case NOTIFICATION_DRAW: {
		if (mode == MODE_FRAME) {
			if (theme_cache.panel.is_null() || theme_cache.titlebar.is_null() || !titlebar_hbox) {
				break;
			}
			// Used for layout calculations.
			Ref<StyleBox> sb_panel = theme_cache.panel;
			Ref<StyleBox> sb_titlebar = theme_cache.titlebar;

			// Used for drawing.
			Ref<StyleBox> sb_to_draw_panel =
				(selected && theme_cache.panel_selected.is_valid()) ? theme_cache.panel_selected
																	: sb_panel;
			Ref<StyleBox> sb_to_draw_titlebar =
				(selected && theme_cache.titlebar_selected.is_valid())
					? theme_cache.titlebar_selected
					: sb_titlebar;

			Rect2 titlebar_rect(
				Point2(), titlebar_hbox->get_bound_minimum_size() + sb_titlebar->get_minimum_size());
			Size2 body_size = get_size();
			titlebar_rect.size.width = body_size.width;
			body_size.y -= titlebar_rect.size.height;
			Rect2 body_rect(Point2(0, titlebar_rect.size.height), body_size);

			// Draw body stylebox.
			draw_style_box(sb_to_draw_panel.ptr(), body_rect);

			// Draw title bar stylebox above.
			draw_style_box(sb_to_draw_titlebar.ptr(), titlebar_rect);

			// Only draw the resize handle if the frame is not auto-resizing.
			if (resizable && !autoshrink_enabled && theme_cache.resizer.is_valid()) {
				Ref<Texture2D> resizer = theme_cache.resizer;
				draw_texture(
					resizer.ptr(), get_size() - resizer->get_size(), theme_cache.resizer_color);
			}
		}
	} break;
	}
}

void GraphNode::draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color)
{
	if (mode != MODE_NODE) {
		return;
	}
	Slot slot = slot_table[p_slot_index];
	Ref<Texture2D> port_icon = p_left ? slot.custom_port_icon_left : slot.custom_port_icon_right;

	if (port_icon.is_null()) {
		port_icon = theme_cache.port;
	}
	if (port_icon.is_null()) {
		return;
	}

	Point2 icon_offset = -port_icon->get_size() * 0.5;
	port_icon->draw(get_canvas_item(), p_pos + icon_offset, p_color);
}

void GraphNode::set_slot(int p_slot_index, bool p_enable_left, int p_type_left,
	const Color& p_color_left, bool p_enable_right, int p_type_right, const Color& p_color_right,
	const Ref<Texture2D>& p_custom_left, const Ref<Texture2D>& p_custom_right, bool p_draw_stylebox)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set slot with index (%d) lesser than zero.", p_slot_index));

	if (!p_enable_left && p_type_left == 0 && p_color_left == Color(1, 1, 1, 1) &&
		!p_enable_right && p_type_right == 0 && p_color_right == Color(1, 1, 1, 1) &&
		p_custom_left.is_null() && p_custom_right.is_null()) {
		slot_table.erase(p_slot_index);
		return;
	}

	Slot s;
	s.enable_left = p_enable_left;
	s.type_left = p_type_left;
	s.color_left = p_color_left;
	s.custom_port_icon_left = p_custom_left;
	s.enable_right = p_enable_right;
	s.type_right = p_type_right;
	s.color_right = p_color_right;
	s.custom_port_icon_right = p_custom_right;
	s.draw_stylebox = p_draw_stylebox;
	slot_table[p_slot_index] = s;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_slot(int p_slot_index)
{
	slot_table.erase(p_slot_index);
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_all_slots()
{
	slot_table.clear();
	port_pos_dirty = true;
	queue_redraw();
}

bool GraphNode::is_slot_enabled_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_left;
}

void GraphNode::set_slot_enabled_left(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_left for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_left = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_left(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_left = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_left;
}

void GraphNode::set_slot_color_left(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_left = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_left;
}

void GraphNode::set_slot_custom_icon_left(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_left = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_left;
}

bool GraphNode::is_slot_enabled_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_right;
}

void GraphNode::set_slot_enabled_right(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_right for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_right = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_right(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_right = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_right;
}

void GraphNode::set_slot_color_right(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_right = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_right;
}

void GraphNode::set_slot_custom_icon_right(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_right = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_right;
}

bool GraphNode::is_slot_draw_stylebox(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].draw_stylebox;
}

void GraphNode::set_slot_draw_stylebox(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].draw_stylebox = p_enable;
	queue_redraw();
}

void GraphNode::set_ignore_invalid_connection_type(bool p_ignore)
{
	ignore_invalid_connection_type = p_ignore;
}

bool GraphNode::is_ignoring_valid_connection_type() const { return ignore_invalid_connection_type; }

bool GraphNode::has_point(const Point2& p_point) const
{
	if (mode == MODE_FRAME) {
		if (theme_cache.resizer.is_valid()) {
			Ref<Texture2D> resizer = theme_cache.resizer;
			if (Rect2(get_size() - resizer->get_size(), resizer->get_size()).has_point(p_point)) {
				return true;
			}
		}

		// For grabbing on the titlebar.
		int titlebar_height = get_titlebar_size().height;
		if (Rect2(0, 0, get_size().width, titlebar_height).has_point(p_point)) {
			return true;
		}

		// Allow grabbing on all sides of the frame.
		Rect2 frame_rect = Rect2(0, 0, get_size().width, get_size().height);
		Rect2 no_drag_rect = frame_rect.grow(-drag_margin);

		if (frame_rect.has_point(p_point) && !no_drag_rect.has_point(p_point)) {
			return true;
		}

		return false;
	}

	return Control::has_point(p_point);
}

Size2 GraphNode::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GraphNode::get_desired_size() const { return _get_minimum_size(true); }

int GraphNode::get_input_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return left_port_cache.size();
}

int GraphNode::get_output_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return right_port_cache.size();
}

Vector2 GraphNode::get_input_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Vector2());
	return left_port_cache[p_port_idx].pos;
}

int GraphNode::get_input_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), 0);
	return left_port_cache[p_port_idx].type;
}

Color GraphNode::get_input_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Color());
	return left_port_cache[p_port_idx].color;
}

int GraphNode::get_input_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), -1);
	return left_port_cache[p_port_idx].slot_index;
}

Vector2 GraphNode::get_output_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Vector2());
	return right_port_cache[p_port_idx].pos;
}

int GraphNode::get_output_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), 0);
	return right_port_cache[p_port_idx].type;
}

Color GraphNode::get_output_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Color());
	return right_port_cache[p_port_idx].color;
}

int GraphNode::get_output_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), -1);
	return right_port_cache[p_port_idx].slot_index;
}

Control::CursorShape GraphNode::get_cursor_shape(const Point2& p_pos) const
{
	const bool can_resize = (mode == MODE_FRAME) ? (resizable && !autoshrink_enabled) : resizable;
	if (can_resize && theme_cache.resizer.is_valid()) {
		if (resizing || (p_pos.x > get_size().x - theme_cache.resizer->get_width() &&
							p_pos.y > get_size().y - theme_cache.resizer->get_height())) {
			return CURSOR_FDIAGSIZE;
		}
	}

	return Control::get_cursor_shape(p_pos);
}

Vector<int> GraphNode::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> GraphNode::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_EXPAND);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

void GraphNode::set_slots_focus_mode(Control::FocusMode p_focus_mode)
{
	slots_focus_mode = p_focus_mode;
}

Control::FocusMode GraphNode::get_slots_focus_mode() const { return slots_focus_mode; }

GraphNode::GraphNode(Mode p_mode) : mode(p_mode)
{
	if (mode != MODE_ELEMENT) {
		titlebar_hbox = Control::create_hbox();
		titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
		titlebar_hbox->set_use_parent_material(true);
		add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

		title_label = memnew(Label);
		title_label->set_theme_type_variation(
			mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
		title_label->set_h_size_flags(SIZE_EXPAND_FILL);
		if (mode == MODE_FRAME) {
			title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			title_label->set_use_parent_material(true);
		}
		titlebar_hbox->add_child(title_label);
	}

	set_mouse_filter(MOUSE_FILTER_STOP);
	if (mode == MODE_NODE) {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
}

void GraphNode::_port_pos_update()
{
	port_pos_dirty = false;
}

Size2 GraphNode::_get_minimum_size(bool) const { return Size2(); }

String GraphNode::get_accessibility_container_name(const Node* p_child) const { return String(); }

String GraphNode::_get_accessibility_name() const { return String(); }

void GraphNode::_resort()
{
	if (mode == MODE_FRAME && titlebar_hbox && theme_cache.titlebar.is_valid()) {
		Ref<StyleBox> sb_titlebar = theme_cache.titlebar;
		Size2 titlebar_size = Size2(get_size().width, titlebar_hbox->get_size().height);
		titlebar_size -= sb_titlebar->get_minimum_size();
		Rect2 titlebar_rect = Rect2(sb_titlebar->get_offset(), titlebar_size);
		fit_child_in_rect(titlebar_hbox, titlebar_rect);
	}
}
