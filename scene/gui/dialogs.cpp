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
#include "scene/gui/dialogs.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/scroll_container.h"
#include "servers/display/display_server.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/resources/style_box.h"
#include "core/input/input.h"
#include "core/string/translation_server.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"

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

#include "scene/gui/dialogs.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/expression.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/image_texture.h"
#include "scene/gui/dialogs.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "core/math/color.h"
#include "scene/resources/gradient.h"
#include <thirdparty/misc/ok_color_shader.h>
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"
#include "scene/gui/dialogs.h"

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
#include "scene/gui/dialogs.h"
#include "scene/2d/line_2d.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/dialogs.h"
#include "scene/resources/material.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include <cfloat>
#include "core/input/input_event.h"
#include "scene/main/viewport.h"
#include "scene/gui/dialogs.h"
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

// ============================================================================
// Merged from scene/gui/tree.cpp
// ============================================================================

#include <climits>
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/math/math_funcs.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "scene/gui/dialogs.h"

Size2 TreeItem::Cell::get_icon_size() const
{
	if (icon.is_null()) {
		return Size2();
	}
	if (icon_region == Rect2i()) {
		return icon->get_size();
	}
	else {
		return icon_region.size;
	}
}

void TreeItem::Cell::draw_icon(const RID& p_where, const Point2& p_pos, const Size2& p_size,
	const Rect2i& p_region, const Color& p_color) const
{
	if (icon.is_null()) {
		return;
	}

	Size2i dsize = (p_size == Size2()) ? icon->get_size() : p_size;

	if (p_region == Rect2i()) {
		icon->draw_rect_region(
			p_where, Rect2(p_pos, dsize), Rect2(Point2(), icon->get_size()), p_color);
		if (icon_overlay.is_valid()) {
			Vector2 offset = icon->get_size() - icon_overlay->get_size();
			icon_overlay->draw_rect_region(p_where, Rect2(p_pos + offset, dsize),
				Rect2(Point2(), icon_overlay->get_size()), p_color);
		}
	}
	else {
		icon->draw_rect_region(p_where, Rect2(p_pos, dsize), p_region, p_color);
		if (icon_overlay.is_valid()) {
			icon_overlay->draw_rect_region(p_where, Rect2(p_pos, dsize), p_region, p_color);
		}
	}
}

void TreeItem::_changed_notify(int p_cell)
{
	if (tree) {
		tree->item_changed(p_cell, this);
	}
}

void TreeItem::_changed_notify()
{
	if (tree) {
		tree->item_changed(-1, this);
	}
}

void TreeItem::_cell_selected(int p_cell, bool p_set_as_cursor)
{
	if (tree) {
		tree->item_selected(p_cell, this, p_set_as_cursor);
	}
}

void TreeItem::_cell_deselected(int p_cell)
{
	if (tree) {
		tree->item_deselected(p_cell, this);
	}
}

void TreeItem::set_cell_mode(int p_column, TreeCellMode p_mode)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].mode == p_mode) {
		return;
	}

	Cell& c = cells.write[p_column];
	c.mode = p_mode;
	c.min = 0;
	c.max = 100;
	c.step = 1;
	c.val = 0;
	c.checked = false;
	c.icon = Ref<Texture2D>();
	c.text = "";
	c.dirty = true;
	c.icon_max_w = 0;
	c.cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

TreeItem::TreeCellMode TreeItem::get_cell_mode(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), TreeItem::CELL_MODE_STRING);
	return cells[p_column].mode;
}

void TreeItem::set_auto_translate_mode(int p_column, Node::AutoTranslateMode p_mode)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].auto_translate_mode == p_mode) {
		return;
	}

	cells.write[p_column].auto_translate_mode = p_mode;
	cells.write[p_column].dirty = true;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Node::AutoTranslateMode TreeItem::get_auto_translate_mode(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Node::AUTO_TRANSLATE_MODE_INHERIT);
	return cells[p_column].auto_translate_mode;
}

void TreeItem::set_edit_multiline(int p_column, bool p_multiline)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].edit_multiline = p_multiline;
	_changed_notify(p_column);
}

bool TreeItem::is_edit_multiline(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].edit_multiline;
}

void TreeItem::set_checked(int p_column, bool p_checked)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].checked == p_checked) {
		return;
	}

	cells.write[p_column].checked = p_checked;
	cells.write[p_column].indeterminate = false;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

void TreeItem::set_indeterminate(int p_column, bool p_indeterminate)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	// Prevent uncheck if indeterminate set to false twice.
	if (p_indeterminate == cells[p_column].indeterminate) {
		return;
	}

	cells.write[p_column].indeterminate = p_indeterminate;
	cells.write[p_column].checked = false;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool TreeItem::is_checked(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].checked;
}

bool TreeItem::is_indeterminate(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].indeterminate;
}

String TreeItem::get_text(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].text;
}

String TreeItem::get_description(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].description;
}

void TreeItem::set_text_direction(int p_column, Control::TextDirection p_text_direction)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);

	if (cells[p_column].text_direction == p_text_direction) {
		return;
	}

	cells.write[p_column].text_direction = p_text_direction;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
	cells.write[p_column].cached_minimum_size_dirty = true;
}

Control::TextDirection TreeItem::get_text_direction(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Control::TEXT_DIRECTION_INHERITED);
	return cells[p_column].text_direction;
}

void TreeItem::set_autowrap_mode(int p_column, TextServer::AutowrapMode p_mode)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	ERR_FAIL_COND(p_mode < TextServer::AUTOWRAP_OFF || p_mode > TextServer::AUTOWRAP_WORD_SMART);

	if (cells[p_column].autowrap_mode == p_mode) {
		return;
	}

	cells.write[p_column].autowrap_mode = p_mode;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
	cells.write[p_column].cached_minimum_size_dirty = true;
}

TextServer::AutowrapMode TreeItem::get_autowrap_mode(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::AUTOWRAP_OFF);
	return cells[p_column].autowrap_mode;
}

void TreeItem::set_autowrap_trim_flags(int p_column, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	// Only trim-related flags are valid for this property.
	uint32_t masked_flags = p_flags & TextServer::BREAK_TRIM_MASK;
	if (cells[p_column].autowrap_trim_flags == masked_flags) {
		return;
	}

	cells.write[p_column].autowrap_trim_flags = masked_flags;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
	cells.write[p_column].cached_minimum_size_dirty = true;
}

uint32_t TreeItem::get_autowrap_trim_flags(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(),
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES);
	return cells[p_column].autowrap_trim_flags;
}

void TreeItem::set_text_overrun_behavior(int p_column, TextServer::OverrunBehavior p_behavior)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].text_buf->get_text_overrun_behavior() == p_behavior) {
		return;
	}

	cells.write[p_column].text_buf->set_text_overrun_behavior(p_behavior);
	cells.write[p_column].dirty = true;
	cells.write[p_column].cached_minimum_size_dirty = true;
	_changed_notify(p_column);
}

TextServer::OverrunBehavior TreeItem::get_text_overrun_behavior(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::OVERRUN_TRIM_ELLIPSIS);
	return cells[p_column].text_buf->get_text_overrun_behavior();
}

void TreeItem::set_structured_text_bidi_override(
	int p_column, TextServer::StructuredTextParser p_parser)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].st_parser != p_parser) {
		cells.write[p_column].st_parser = p_parser;
		cells.write[p_column].dirty = true;
		cells.write[p_column].cached_minimum_size_dirty = true;

		_changed_notify(p_column);
	}
}

TextServer::StructuredTextParser TreeItem::get_structured_text_bidi_override(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), TextServer::STRUCTURED_TEXT_DEFAULT);
	return cells[p_column].st_parser;
}

void TreeItem::set_language(int p_column, const String& p_language)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].language != p_language) {
		cells.write[p_column].language = p_language;
		cells.write[p_column].dirty = true;
		cells.write[p_column].cached_minimum_size_dirty = true;

		_changed_notify(p_column);
	}
}

String TreeItem::get_language(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].language;
}

void TreeItem::set_suffix(int p_column, String p_suffix)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].suffix == p_suffix) {
		return;
	}

	cells.write[p_column].suffix = p_suffix;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

String TreeItem::get_suffix(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].suffix;
}

void TreeItem::set_icon(int p_column, const Ref<Texture2D>& p_icon)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon == p_icon) {
		return;
	}

	cells.write[p_column].icon = p_icon;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Texture2D> TreeItem::get_icon(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Texture2D>());
	return cells[p_column].icon;
}

void TreeItem::set_icon_overlay(int p_column, const Ref<Texture2D>& p_icon_overlay)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_overlay == p_icon_overlay) {
		return;
	}

	cells.write[p_column].icon_overlay = p_icon_overlay;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Texture2D> TreeItem::get_icon_overlay(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Texture2D>());
	return cells[p_column].icon_overlay;
}

void TreeItem::set_icon_region(int p_column, const Rect2& p_icon_region)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_region == p_icon_region) {
		return;
	}

	cells.write[p_column].icon_region = p_icon_region;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Rect2 TreeItem::get_icon_region(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Rect2());
	return cells[p_column].icon_region;
}

void TreeItem::set_icon_modulate(int p_column, const Color& p_modulate)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_color == p_modulate) {
		return;
	}

	cells.write[p_column].icon_color = p_modulate;
	_changed_notify(p_column);
}

Color TreeItem::get_icon_modulate(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	return cells[p_column].icon_color;
}

void TreeItem::set_icon_max_width(int p_column, int p_max)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].icon_max_w == p_max) {
		return;
	}

	cells.write[p_column].icon_max_w = p_max;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

int TreeItem::get_icon_max_width(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), 0);
	return cells[p_column].icon_max_w;
}

void TreeItem::set_range(int p_column, double p_value)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	if (cells[p_column].step > 0) {
		p_value = Math::snapped(p_value, cells[p_column].step);
	}
	if (p_value < cells[p_column].min) {
		p_value = cells[p_column].min;
	}
	if (p_value > cells[p_column].max) {
		p_value = cells[p_column].max;
	}

	if (cells[p_column].val == p_value) {
		return;
	}

	cells.write[p_column].val = p_value;
	cells.write[p_column].dirty = true;
	_changed_notify(p_column);
}

double TreeItem::get_range(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), 0);
	return cells[p_column].val;
}

bool TreeItem::is_range_exponential(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].expr;
}

void TreeItem::set_range_config(int p_column, double p_min, double p_max, double p_step, bool p_exp)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].min == p_min && cells[p_column].max == p_max &&
		cells[p_column].step == p_step && cells[p_column].expr == p_exp) {
		return;
	}

	cells.write[p_column].min = p_min;
	cells.write[p_column].max = p_max;
	cells.write[p_column].step = p_step;
	cells.write[p_column].expr = p_exp;
	_changed_notify(p_column);
}

void TreeItem::get_range_config(int p_column, double& r_min, double& r_max, double& r_step) const
{
	ERR_FAIL_INDEX(p_column, cells.size());
	r_min = cells[p_column].min;
	r_max = cells[p_column].max;
	r_step = cells[p_column].step;
}

void TreeItem::set_collapsed(bool p_collapsed) {}

bool TreeItem::is_collapsed() { return collapsed; }

void TreeItem::set_collapsed_recursive(bool p_collapsed)
{
	if (!tree) {
		return;
	}

	set_collapsed(p_collapsed);

	TreeItem* child = get_first_child();
	while (child) {
		child->set_collapsed_recursive(p_collapsed);
		child = child->get_next();
	}
}

bool TreeItem::_is_any_collapsed(bool p_only_visible)
{
	TreeItem* child = get_first_child();

	// Check on children directly first (avoid recursing if possible).
	while (child) {
		if (child->get_first_child() && child->is_collapsed() &&
			(!p_only_visible || (child->is_visible() && child->get_visible_child_count()))) {
			return true;
		}
		child = child->get_next();
	}

	child = get_first_child();

	// Otherwise recurse on children.
	while (child) {
		if (child->get_first_child() &&
			(!p_only_visible || (child->is_visible() && child->get_visible_child_count())) &&
			child->_is_any_collapsed(p_only_visible)) {
			return true;
		}
		child = child->get_next();
	}

	return false;
}

bool TreeItem::is_any_collapsed(bool p_only_visible)
{
	if (p_only_visible && !is_visible_in_tree()) {
		return false;
	}

	// Collapsed if this is collapsed and it has children (only considers visible if only visible is
	// set).
	if (is_collapsed() && get_first_child() && (!p_only_visible || get_visible_child_count())) {
		return true;
	}

	return _is_any_collapsed(p_only_visible);
}

void TreeItem::set_visible(bool p_visible) {}

bool TreeItem::is_visible() { return visible; }

bool TreeItem::is_visible_in_tree() const { return visible && parent_visible_in_tree; }

void TreeItem::_handle_visibility_changed(bool p_visible)
{
	TreeItem* child = get_first_child();
	while (child) {
		child->_propagate_visibility_changed(p_visible);
		child = child->get_next();
	}
}

void TreeItem::_propagate_visibility_changed(bool p_parent_visible_in_tree)
{
	parent_visible_in_tree = p_parent_visible_in_tree;
	_handle_visibility_changed(p_parent_visible_in_tree);
}

void TreeItem::uncollapse_tree()
{
	TreeItem* t = this;
	while (t) {
		t->set_collapsed(false);
		t = t->parent;
	}
}

void TreeItem::set_custom_minimum_height(int p_height)
{
	if (custom_min_height == p_height) {
		return;
	}

	custom_min_height = p_height;

	for (Cell& c : cells) {
		c.cached_minimum_size_dirty = true;
	}

	_changed_notify();
}

int TreeItem::get_custom_minimum_height() const { return custom_min_height; }

void TreeItem::set_custom_stylebox(int p_column, const Ref<StyleBox>& p_stylebox)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_stylebox = p_stylebox;
	_changed_notify(p_column);
}

Ref<StyleBox> TreeItem::get_custom_stylebox(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<StyleBox>());
	return cells[p_column].custom_stylebox;
}

void TreeItem::add_child(TreeItem* p_item)
{
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(p_item->tree);
	ERR_FAIL_COND(p_item->parent);

	p_item->_change_tree(tree);
	p_item->parent = this;
	p_item->parent_visible_in_tree = is_visible_in_tree();
	p_item->_handle_visibility_changed(p_item->parent_visible_in_tree);

	if (last_child) {
		last_child->next = p_item;
		p_item->prev = last_child;
	}
	else {
		first_child = p_item;
	}
	last_child = p_item;

	if (!children_cache.is_empty()) {
		children_cache.push_back(p_item);
	}

	validate_cache();
}

void TreeItem::remove_child(TreeItem* p_item)
{
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND(p_item->parent != this);

	p_item->_unlink_from_tree();
	p_item->_change_tree(nullptr);
	p_item->prev = nullptr;
	p_item->next = nullptr;
	p_item->parent = nullptr;

	validate_cache();
}

Tree* TreeItem::get_tree() const { return tree; }

TreeItem* TreeItem::get_next() const { return next; }

TreeItem* TreeItem::get_prev()
{
	if (prev) {
		return prev;
	}

	if (!parent || parent->first_child == this) {
		return nullptr;
	}
	// This is an edge case.
	TreeItem* l_prev = parent->first_child;
	while (l_prev && l_prev->next != this) {
		l_prev = l_prev->next;
	}

	prev = l_prev;

	return prev;
}

TreeItem* TreeItem::get_parent() const { return parent; }

TreeItem* TreeItem::get_first_child() const { return first_child; }

TreeItem* TreeItem::get_last_child() const { return last_child; }

TreeItem* TreeItem::_get_prev_in_tree(bool p_wrap, bool p_include_invisible)
{
	TreeItem* current = this;

	TreeItem* prev_item = current->get_prev();

	if (!prev_item) {
		current = current->parent;
		if (!current || (current == tree->root && tree->hide_root)) {
			if (!p_wrap) {
				return nullptr;
			}
			// Wrap around to the last visible item.
			current = this;
			TreeItem* temp = get_next_visible();
			while (temp) {
				current = temp;
				temp = temp->get_next_visible();
			}
		}
	}
	else {
		current = prev_item;
		while ((!current->collapsed || p_include_invisible) && current->last_child) {
			current = current->last_child;
		}
	}

	return current;
}

TreeItem* TreeItem::get_prev_visible(bool p_wrap)
{
	TreeItem* loop = this;
	TreeItem* prev_item = _get_prev_in_tree(p_wrap);
	while (prev_item && !prev_item->is_visible_in_tree()) {
		prev_item = prev_item->_get_prev_in_tree(p_wrap);
		if (prev_item == loop) {
			// Check that we haven't looped all the way around to the start.
			prev_item = nullptr;
			break;
		}
	}
	return prev_item;
}

TreeItem* TreeItem::_get_next_in_tree(bool p_wrap, bool p_include_invisible)
{
	TreeItem* current = this;

	if ((!current->collapsed || p_include_invisible) && current->first_child) {
		current = current->first_child;

	}
	else if (current->next) {
		current = current->next;
	}
	else {
		while (current && !current->next) {
			current = current->parent;
		}

		if (!current) {
			if (p_wrap) {
				return tree->root;
			}
			else {
				return nullptr;
			}
		}
		else {
			current = current->next;
		}
	}

	return current;
}

TreeItem* TreeItem::get_next_visible(bool p_wrap)
{
	TreeItem* loop = this;
	TreeItem* next_item = _get_next_in_tree(p_wrap);
	while (next_item && !next_item->is_visible_in_tree()) {
		next_item = next_item->_get_next_in_tree(p_wrap);
		if (next_item == loop) {
			// Check that we haven't looped all the way around to the start.
			next_item = nullptr;
			break;
		}
	}
	return next_item;
}

TreeItem* TreeItem::get_prev_in_tree(bool p_wrap)
{
	TreeItem* prev_item = _get_prev_in_tree(p_wrap, true);
	return prev_item;
}

TreeItem* TreeItem::get_next_in_tree(bool p_wrap)
{
	TreeItem* next_item = _get_next_in_tree(p_wrap, true);
	return next_item;
}

TreeItem* TreeItem::get_child(int p_index)
{
	_create_children_cache();

	if (p_index < 0) {
		p_index += children_cache.size();
	}
	ERR_FAIL_INDEX_V(p_index, (int)children_cache.size(), nullptr);

	return children_cache[p_index];
}

int TreeItem::get_visible_child_count()
{
	_create_children_cache();
	int visible_count = 0;
	for (uint32_t i = 0; i < children_cache.size(); i++) {
		if (children_cache[i]->is_visible()) {
			visible_count += 1;
		}
	}
	return visible_count;
}

int TreeItem::get_child_count()
{
	_create_children_cache();
	return children_cache.size();
}

Vector<TreeItem*> TreeItem::get_children()
{
	// Don't need to explicitly create children cache, because get_child_count creates it.
	int size = get_child_count();
	Vector<TreeItem*> vec;
	vec.resize(size);
	for (int i = 0; i < size; i++) {
		vec.write[i] = children_cache[i];
	}
	return vec;
}

void TreeItem::clear_children()
{
	TreeItem* c = first_child;
	while (c) {
		TreeItem* aux = c;
		c = c->get_next();
		aux->parent = nullptr; // So it won't try to recursively auto-remove from me in here.
		memdelete(aux);
	}

	first_child = nullptr;
	last_child = nullptr;
	children_cache.clear();
}

int TreeItem::get_index()
{
	int idx = 0;
	TreeItem* c = this;

	while (c) {
		c = c->get_prev();
		idx++;
	}
	return idx - 1;
}

#ifdef DEV_ENABLED
void TreeItem::validate_cache() const
{
	if (!parent || parent->children_cache.is_empty()) {
		return;
	}
	TreeItem* scan = parent->first_child;
	uint32_t index = 0;
	while (scan) {
		DEV_ASSERT(parent->children_cache[index] == scan);
		++index;
		scan = scan->get_next();
	}
	DEV_ASSERT(index == parent->children_cache.size());
}
#endif

void TreeItem::set_selectable(int p_column, bool p_selectable)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].selectable = p_selectable;
}

bool TreeItem::is_selectable(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].selectable;
}

bool TreeItem::is_selected(int p_column)
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].selectable && cells[p_column].selected;
}

bool TreeItem::is_any_column_selected() const
{
	for (const Cell& cell : cells) {
		if (cell.selectable && cell.selected) {
			return true;
		}
	}
	return false;
}

void TreeItem::select(int p_column, bool p_set_as_cursor)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	_cell_selected(p_column, p_set_as_cursor);
}

void TreeItem::deselect(int p_column)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	_cell_deselected(p_column);
}

void TreeItem::clear_buttons()
{
	int i = 0;
	for (Cell& cell : cells) {
		if (!cell.buttons.is_empty()) {
			for (Cell::Button& btn : cell.buttons) {
				if (btn.accessibility_button_element.is_valid()) {
					AccessibilityServer::free_element(
						btn.accessibility_button_element);
				}
			}
			cell.buttons.clear();
			cell.cached_minimum_size_dirty = true;
			_changed_notify(i);
		}
		++i;
	}
}

int TreeItem::get_button_count(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), -1);
	return cells[p_column].buttons.size();
}

Ref<Texture2D> TreeItem::get_button(int p_column, int p_index) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Texture2D>());
	ERR_FAIL_INDEX_V(p_index, cells[p_column].buttons.size(), Ref<Texture2D>());
	return cells[p_column].buttons[p_index].texture;
}

String TreeItem::get_button_tooltip_text(int p_column, int p_index) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), String());
	ERR_FAIL_INDEX_V(p_index, cells[p_column].buttons.size(), String());
	return cells[p_column].buttons[p_index].tooltip;
}

int TreeItem::get_button_id(int p_column, int p_index) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), -1);
	ERR_FAIL_INDEX_V(p_index, cells[p_column].buttons.size(), -1);
	return cells[p_column].buttons[p_index].id;
}

int TreeItem::get_button_by_id(int p_column, int p_id) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), -1);
	for (int i = 0; i < cells[p_column].buttons.size(); i++) {
		if (cells[p_column].buttons[i].id == p_id) {
			return i;
		}
	}

	return -1;
}

Color TreeItem::get_button_color(int p_column, int p_index) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	ERR_FAIL_INDEX_V(p_index, cells[p_column].buttons.size(), Color());
	return cells[p_column].buttons[p_index].color;
}

void TreeItem::set_button_tooltip_text(int p_column, int p_index, const String& p_tooltip)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	if (p_index < 0) {
		p_index += cells[p_column].buttons.size();
	}

	ERR_FAIL_INDEX(p_index, cells[p_column].buttons.size());
	cells.write[p_column].buttons.write[p_index].tooltip = p_tooltip;

	_changed_notify(p_column);
}

void TreeItem::set_button(int p_column, int p_index, const Ref<Texture2D>& p_button)
{
	ERR_FAIL_COND(p_button.is_null());
	ERR_FAIL_INDEX(p_column, cells.size());
	if (p_index < 0) {
		p_index += cells[p_column].buttons.size();
	}
	ERR_FAIL_INDEX(p_index, cells[p_column].buttons.size());

	if (cells[p_column].buttons[p_index].texture == p_button) {
		return;
	}

	cells.write[p_column].buttons.write[p_index].texture = p_button;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

void TreeItem::set_button_color(int p_column, int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	if (p_index < 0) {
		p_index += cells[p_column].buttons.size();
	}
	ERR_FAIL_INDEX(p_index, cells[p_column].buttons.size());

	if (cells[p_column].buttons[p_index].color == p_color) {
		return;
	}

	cells.write[p_column].buttons.write[p_index].color = p_color;
	_changed_notify(p_column);
}

void TreeItem::set_button_disabled(int p_column, int p_index, bool p_disabled)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	if (p_index < 0) {
		p_index += cells[p_column].buttons.size();
	}
	ERR_FAIL_INDEX(p_index, cells[p_column].buttons.size());

	if (cells[p_column].buttons[p_index].disabled == p_disabled) {
		return;
	}

	cells.write[p_column].buttons.write[p_index].disabled = p_disabled;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool TreeItem::is_button_disabled(int p_column, int p_index) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	ERR_FAIL_INDEX_V(p_index, cells[p_column].buttons.size(), false);

	return cells[p_column].buttons[p_index].disabled;
}

void TreeItem::set_editable(int p_column, bool p_editable)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].editable == p_editable) {
		return;
	}

	cells.write[p_column].editable = p_editable;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool TreeItem::is_editable(int p_column)
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].editable;
}

void TreeItem::set_custom_color(int p_column, const Color& p_color)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_color && cells[p_column].color == p_color) {
		return;
	}

	cells.write[p_column].custom_color = true;
	cells.write[p_column].color = p_color;
	_changed_notify(p_column);
}

Color TreeItem::get_custom_color(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	if (!cells[p_column].custom_color) {
		return Color();
	}
	return cells[p_column].color;
}

void TreeItem::clear_custom_color(int p_column)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_color = false;
	cells.write[p_column].color = Color();
	_changed_notify(p_column);
}

void TreeItem::set_custom_font(int p_column, const Ref<Font>& p_font)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_font == p_font) {
		return;
	}

	cells.write[p_column].custom_font = p_font;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

Ref<Font> TreeItem::get_custom_font(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Ref<Font>());
	return cells[p_column].custom_font;
}

void TreeItem::set_custom_font_size(int p_column, int p_font_size)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_font_size == p_font_size) {
		return;
	}

	cells.write[p_column].custom_font_size = p_font_size;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

int TreeItem::get_custom_font_size(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), -1);
	return cells[p_column].custom_font_size;
}

void TreeItem::set_tooltip_text(int p_column, const String& p_tooltip)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].tooltip = p_tooltip;
}

String TreeItem::get_tooltip_text(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), "");
	return cells[p_column].tooltip;
}

void TreeItem::set_custom_bg_color(int p_column, const Color& p_color, bool p_bg_outline)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_bg_color && cells[p_column].custom_bg_outline == p_bg_outline &&
		cells[p_column].bg_color == p_color) {
		return;
	}

	cells.write[p_column].custom_bg_color = true;
	cells.write[p_column].custom_bg_outline = p_bg_outline;
	cells.write[p_column].bg_color = p_color;
	_changed_notify(p_column);
}

void TreeItem::clear_custom_bg_color(int p_column)
{
	ERR_FAIL_INDEX(p_column, cells.size());
	cells.write[p_column].custom_bg_color = false;
	cells.write[p_column].bg_color = Color();
	_changed_notify(p_column);
}

Color TreeItem::get_custom_bg_color(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Color());
	if (!cells[p_column].custom_bg_color) {
		return Color();
	}
	return cells[p_column].bg_color;
}

void TreeItem::set_custom_as_button(int p_column, bool p_button)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].custom_button == p_button) {
		return;
	}

	cells.write[p_column].custom_button = p_button;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool TreeItem::is_custom_set_as_button(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].custom_button;
}

void TreeItem::set_text_alignment(int p_column, HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].text_alignment == p_alignment) {
		return;
	}

	cells.write[p_column].text_alignment = p_alignment;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

HorizontalAlignment TreeItem::get_text_alignment(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), HORIZONTAL_ALIGNMENT_LEFT);
	return cells[p_column].text_alignment;
}

void TreeItem::set_expand_right(int p_column, bool p_enable)
{
	ERR_FAIL_INDEX(p_column, cells.size());

	if (cells[p_column].expand_right == p_enable) {
		return;
	}

	cells.write[p_column].expand_right = p_enable;
	cells.write[p_column].cached_minimum_size_dirty = true;

	_changed_notify(p_column);
}

bool TreeItem::get_expand_right(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), false);
	return cells[p_column].expand_right;
}

void TreeItem::set_disable_folding(bool p_disable)
{
	if (disable_folding == p_disable) {
		return;
	}

	disable_folding = p_disable;

	for (Cell& c : cells) {
		c.cached_minimum_size_dirty = true;
	}

	_changed_notify(0);
}

bool TreeItem::is_folding_disabled() const { return disable_folding; }

void TreeItem::set_accept_children(bool p_allowed) { accepts_children = p_allowed; }

bool TreeItem::is_accepting_children() const { return accepts_children; }

Size2 TreeItem::get_minimum_size(int p_column)
{
	ERR_FAIL_INDEX_V(p_column, cells.size(), Size2());
	Tree* parent_tree = get_tree();
	ERR_FAIL_NULL_V(parent_tree, Size2());

	const TreeItem::Cell& cell = cells[p_column];

	if (cell.cached_minimum_size_dirty || cell.text_buf->is_dirty() || cell.dirty) {
		Size2 size = Size2(parent_tree->theme_cache.inner_item_margin_left +
							   parent_tree->theme_cache.inner_item_margin_right,
			parent_tree->theme_cache.inner_item_margin_top +
				parent_tree->theme_cache.inner_item_margin_bottom);
		int content_height = size.height;

		// Text.
		if (!cell.text.is_empty()) {
			if (cell.dirty) {
				parent_tree->update_item_cell(this, p_column);
			}
			Size2 text_size = cell.text_buf->get_size();
			if (get_text_overrun_behavior(p_column) == TextServer::OVERRUN_NO_TRIMMING) {
				size.width += text_size.width;
			}
			content_height = MAX(content_height, text_size.height);
		}

		// Icon.
		if (cell.mode == CELL_MODE_CHECK) {
			Size2i check_size = parent_tree->theme_cache.checked->get_size();
			size.width += check_size.width + parent_tree->theme_cache.h_separation;
			content_height = MAX(content_height, check_size.height);
		}
		else if (cell.mode == CELL_MODE_RANGE) {
			Ref<Texture2D> icon = cell.text.is_empty() ? parent_tree->theme_cache.updown
													   : parent_tree->theme_cache.select_arrow;
			Size2i icon_size = icon->get_size();
			size.width += icon_size.width + parent_tree->theme_cache.h_separation;
			content_height = MAX(content_height, icon_size.height);
		}
		if (cell.icon.is_valid()) {
			Size2i icon_size = parent_tree->_get_cell_icon_size(cell);
			size.width += icon_size.width + parent_tree->theme_cache.icon_h_separation;
			content_height = MAX(content_height, icon_size.height);
		}

		// Buttons.
		for (int i = 0; i < cell.buttons.size(); i++) {
			Ref<Texture2D> texture = cell.buttons[i].texture;
			if (texture.is_valid()) {
				Size2 button_size = texture->get_size();
				button_size.width +=
					parent_tree->theme_cache.button_pressed->get_minimum_size().width;
				size.width += button_size.width + parent_tree->theme_cache.button_margin;
				content_height = MAX(content_height, button_size.height);
			}
		}

		size.height += content_height;
		cells.write[p_column].cached_minimum_size = size;
		cells.write[p_column].cached_minimum_size_dirty = false;
	}

	return cell.cached_minimum_size;
}

TreeItem::TreeItem(Tree* p_tree) { tree = p_tree; }

TreeItem::~TreeItem()
{
	_unlink_from_tree();
	_change_tree(nullptr);

	validate_cache();
	prev = nullptr;
	clear_children();
}

/*************************** END of TreeItem ***************************/

void Tree::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

Size2 Tree::_get_cell_icon_size(const TreeItem::Cell& p_cell) const
{
	Size2i icon_size = p_cell.get_icon_size();

	int max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		max_width = theme_cache.icon_max_width;
	}
	if (p_cell.icon_max_w > 0 && (max_width == 0 || p_cell.icon_max_w < max_width)) {
		max_width = p_cell.icon_max_w;
	}

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

int Tree::compute_item_height(TreeItem* p_item) const
{
	if ((p_item == root && hide_root) || !p_item->is_visible_in_tree()) {
		return 0;
	}

	ERR_FAIL_COND_V(theme_cache.font.is_null(), 0);
	int height = 0;

	for (int i = 0; i < columns.size(); i++) {
		height = MAX(height, p_item->get_minimum_size(i).y);
	}
	int font_height = cache.font_height != -1 ? cache.font_height
											  : theme_cache.font->get_height(theme_cache.font_size);
	int item_min_height = MAX(font_height, p_item->get_custom_minimum_height());
	if (height < item_min_height) {
		height = item_min_height;
	}

	height += theme_cache.v_separation;

	return height;
}

int Tree::get_item_height(TreeItem* p_item) const
{
	if (!p_item->is_visible_in_tree()) {
		return 0;
	}
	int height = compute_item_height(p_item);
	height += theme_cache.v_separation;

	if (!p_item->collapsed) { // If not collapsed, check the children.

		TreeItem* c = p_item->first_child;

		while (c) {
			height += get_item_height(c);

			c = c->next;
		}
	}

	return height;
}

Point2i Tree::convert_rtl_position(const Point2i& p_pos, int p_width) const
{
	if (cache.rtl) {
		return Point2i(get_size().width - p_pos.x - p_width, p_pos.y);
	}
	return p_pos;
}

Point2 Tree::convert_rtl_position(const Point2& p_pos, int p_width) const
{
	if (cache.rtl) {
		return Point2(get_size().width - p_pos.x - p_width, p_pos.y);
	}
	return p_pos;
}

Rect2i Tree::convert_rtl_rect(const Rect2i& p_rect) const
{
	if (cache.rtl) {
		return Rect2i(
			Point2i(get_size().width - p_rect.position.x - p_rect.size.x, p_rect.position.y),
			p_rect.size);
	}
	return p_rect;
}

void Tree::draw_item_rect(const TreeItem::Cell& p_cell, const Rect2i& p_rect, const Color& p_color,
	const Color& p_icon_color, int p_ol_size, const Color& p_ol_color, const RID& p_ci) const
{
	ERR_FAIL_COND(theme_cache.font.is_null());

	Rect2i rect = p_rect;
	Size2 ts = p_cell.text_buf->get_size();
	bool rtl = is_layout_rtl();

	Size2i icon_size;
	if (p_cell.icon.is_valid()) {
		icon_size = _get_cell_icon_size(p_cell);
	}

	int displayed_width = 0;
	if (p_cell.icon.is_valid()) {
		displayed_width =
			MIN(displayed_width + icon_size.width + theme_cache.icon_h_separation, rect.size.width);
	}
	if (displayed_width + ts.width > rect.size.width) {
		ts.width = rect.size.width - displayed_width;
	}
	displayed_width += ts.width;

	int empty_width = rect.size.width - displayed_width;

	switch (p_cell.text_alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			rect.position.x += empty_width;
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER:
		rect.position.x += empty_width / 2;
		break;
	case HORIZONTAL_ALIGNMENT_RIGHT:
		if (!rtl) {
			rect.position.x += empty_width;
		}
		break;
	}

	RID ci = p_ci;

	if (rtl) {
		if (ts.width > 0) {
			Point2 draw_pos = rect.position;
			draw_pos.y += Math::floor((rect.size.y - p_cell.text_buf->get_size().y) * 0.5);
			if (p_ol_size > 0 && p_ol_color.a > 0) {
				p_cell.text_buf->draw_outline(ci, draw_pos, p_ol_size, p_ol_color);
			}
			p_cell.text_buf->draw(ci, draw_pos, p_color);
		}
		rect.position.x += ts.width + theme_cache.icon_h_separation;
	}

	if (p_cell.icon.is_valid()) {
		Point2 icon_pos =
			rect.position + Size2i(0, Math::floor((real_t)(rect.size.y - icon_size.y) / 2));
		Rect2i icon_region = (p_cell.icon_region == Rect2i())
								 ? Rect2i(Point2(), p_cell.icon->get_size())
								 : p_cell.icon_region;
		if (icon_size.width > rect.size.width) {
			icon_region.size.width = icon_region.size.width * rect.size.width / icon_size.width;
			icon_size.width = rect.size.width;
		}
		p_cell.draw_icon(ci, icon_pos, icon_size, icon_region, p_icon_color);
		rect.position.x += icon_size.x + theme_cache.icon_h_separation;
	}

	if (!rtl && ts.width > 0) {
		Point2 draw_pos = rect.position;
		draw_pos.y += Math::floor((rect.size.y - p_cell.text_buf->get_size().y) * 0.5);
		if (p_ol_size > 0 && p_ol_color.a > 0) {
			p_cell.text_buf->draw_outline(ci, draw_pos, p_ol_size, p_ol_color);
		}
		p_cell.text_buf->draw(ci, draw_pos, p_color);
	}
}

void Tree::update_item_cache(TreeItem* p_item) const
{
	for (int i = 0; i < p_item->cells.size(); i++) {
		update_item_cell(p_item, i);
	}

	TreeItem* c = p_item->first_child;
	while (c) {
		update_item_cache(c);
		c = c->next;
	}
}

int Tree::_count_selected_items(TreeItem* p_from) const
{
	int count = 0;
	for (int i = 0; i < columns.size(); i++) {
		if (p_from->is_selected(i)) {
			count++;
		}
	}

	for (TreeItem* c = p_from->get_first_child(); c; c = c->get_next()) {
		count += _count_selected_items(c);
	}

	return count;
}

bool Tree::_is_branch_selected(TreeItem* p_from) const
{
	if (p_from->is_any_column_selected()) {
		return true;
	}

	TreeItem* child_item = p_from->get_first_child();
	while (child_item) {
		if (_is_branch_selected(child_item)) {
			return true;
		}
		child_item = child_item->get_next();
	}

	return false;
}

bool Tree::_is_sibling_branch_selected(TreeItem* p_from) const
{
	TreeItem* sibling_item = p_from->get_next();
	while (sibling_item) {
		if (_is_branch_selected(sibling_item)) {
			return true;
		}
		sibling_item = sibling_item->get_next();
	}

	return false;
}

Rect2 Tree::search_item_rect(TreeItem* p_from, TreeItem* p_item) { return Rect2(); }

void Tree::_text_editor_popup_modal_close()
{
	if (popup_edit_committed) {
		return; // Already processed by `LineEdit` / `TextEdit` commit.
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	if (value_editor && value_editor->has_point(value_editor->get_local_mouse_position())) {
		return;
	}

	if (!popup_edited_item) {
		return;
	}

	if (popup_edited_item->is_edit_multiline(popup_edited_item_col) &&
		popup_edited_item->get_cell_mode(popup_edited_item_col) == TreeItem::CELL_MODE_STRING) {
		_apply_multiline_edit();
	}
	else {
		_line_editor_submit(line_editor->get_text());
	}
}

void Tree::_text_editor_gui_input(const InputEvent& p_event)
{
	if (popup_edit_committed) {
		return; // Already processed by `_text_editor_popup_modal_close`.
	}

	if (popup_editor->get_hide_reason() == Popup::HIDE_REASON_CANCELED) {
		return; // ESC pressed, app focus lost, or forced close from code.
	}

	if (p_event.is_action_pressed("ui_text_newline_blank", true)) {
		accept_event();
	}
	else if (p_event.is_action_pressed("ui_text_newline")) {
		bool hide_focus = !text_editor->has_focus(true);
		popup_edit_committed = true; // End edit popup processing.
		popup_editor->hide();
		_apply_multiline_edit(hide_focus);
		accept_event();
	}
}

void Tree::_update_popup_menu(const TreeItem::Cell& p_cell)
{
	if (popup_menu == nullptr) {
		popup_menu = memnew(PopupMenu);
		popup_menu->set_shrink_width(false);
		popup_menu->hide();
		add_child(popup_menu, false, INTERNAL_MODE_FRONT);
	}
}

bool Tree::_scroll(bool p_horizontal, float p_pages)
{
	ScrollBar* scroll = p_horizontal ? (ScrollBar*)h_scroll : (ScrollBar*)v_scroll;

	double prev_value = scroll->get_value();
	scroll->set_value(scroll->get_value() + scroll->get_page() * p_pages);

	bool scroll_happened = scroll->get_value() != prev_value;
	if (scroll_happened) {
		_determine_hovered_item();
	}
	return scroll_happened;
}

Rect2 Tree::_get_scrollbar_layout_rect() const
{
	const Size2 control_size = get_size();
	const Ref<StyleBox> background = theme_cache.panel_style;

	// This is the background stylebox's content rect.
	const real_t width =
		control_size.x - background->get_margin(SIDE_LEFT) - background->get_margin(SIDE_RIGHT);
	const real_t height =
		control_size.y - background->get_margin(SIDE_TOP) - background->get_margin(SIDE_BOTTOM);
	const Rect2 content_rect = Rect2(background->get_offset(), Size2(width, height));

	// Use the stylebox's margins by default. Can be overridden by `scrollbar_margin_*`.
	const real_t top = theme_cache.scrollbar_margin_top < 0 ? content_rect.get_position().y
															: theme_cache.scrollbar_margin_top;
	const real_t right = theme_cache.scrollbar_margin_right < 0
							 ? content_rect.get_end().x
							 : (control_size.x - theme_cache.scrollbar_margin_right);
	const real_t bottom = theme_cache.scrollbar_margin_bottom < 0
							  ? content_rect.get_end().y
							  : (control_size.y - theme_cache.scrollbar_margin_bottom);
	const real_t left = theme_cache.scrollbar_margin_left < 0 ? content_rect.get_position().x
															  : theme_cache.scrollbar_margin_left;

	return Rect2(left, top, right - left, bottom - top);
}

Rect2 Tree::_get_content_rect() const
{
	const Size2 control_size = get_size();
	const Ref<StyleBox> background = theme_cache.panel_style;

	// This is the background stylebox's content rect.
	const real_t width =
		control_size.x - background->get_margin(SIDE_LEFT) - background->get_margin(SIDE_RIGHT);
	const real_t height =
		control_size.y - background->get_margin(SIDE_TOP) - background->get_margin(SIDE_BOTTOM);
	const Rect2 content_rect = Rect2(background->get_offset(), Size2(width, height));

	// Scrollbars won't affect Tree's content rect if they're not visible or placed inside the
	// stylebox margin area.
	const real_t v_size =
		v_scroll->is_visible()
			? (v_scroll->get_bound_minimum_size().x + theme_cache.scrollbar_h_separation)
			: 0;
	const real_t h_size =
		h_scroll->is_visible()
			? (h_scroll->get_bound_minimum_size().y + theme_cache.scrollbar_v_separation)
			: 0;
	const Point2 scroll_begin = _get_scrollbar_layout_rect().get_end() - Vector2(v_size, h_size);
	const Size2 offset = (content_rect.get_end() - scroll_begin).maxf(0);

	return content_rect.grow_individual(0, 0, -offset.x, -offset.y);
}

void Tree::_incr_search_as_needed(const InputEvent& p_event_key)
{
	if (p_event_key.type == InputEventType::KEY && !get_viewport()->is_input_handled()) {
		_do_incr_search(String::chr(p_event_key.key.unicode));
	}
}

void Tree::_on_dropping_unfold_timer_timeout()
{
	if (enable_drag_unfolding && drop_mode_over &&
		(drop_mode_section == 0 || drop_mode_section == 2)) {
		drop_mode_over->set_collapsed(false);
	}
}

void Tree::_reset_drop_mode_over()
{
	drop_mode_over = nullptr;
	if (!dropping_unfold_timer->is_stopped()) {
		dropping_unfold_timer->stop();
	}
}

void Tree::_queue_update_hovered_item()
{
	if (hovered_update_queued) {
		return;
	}
	hovered_update_queued = true;
}

Rect2 Tree::_get_item_focus_rect(const TreeItem* p_item) const
{
	Rect2 rect;
	if (select_mode == SELECT_ROW) {
		rect = p_item->cells[selected_col].focus_rect;
	}
	else {
		rect = p_item->focus_rect;
	}
	return rect;
}

bool Tree::is_editing() { return popup_editor->is_visible(); }

void Tree::set_editor_selection(
	int p_from_line, int p_to_line, int p_from_column, int p_to_column, int p_caret)
{
	if (p_from_column == -1 || p_to_column == -1) {
		line_editor->select(p_from_line, p_to_line);
	}
	else {
		text_editor->select(p_from_line, p_from_column, p_to_line, p_to_column, p_caret);
	}
}

Size2 Tree::get_internal_min_size() const
{
	Size2i size;
	if (root) {
		size.height += get_item_height(root);
	}
	for (int i = 0; i < columns.size(); i++) {
		size.width += get_column_minimum_width(i);
	}

	return size;
}

int Tree::_get_title_button_height() const
{
	ERR_FAIL_COND_V(theme_cache.tb_font.is_null() || theme_cache.title_button.is_null(), 0);
	int h = 0;
	if (show_column_titles) {
		for (int i = 0; i < columns.size(); i++) {
			h = MAX(h, columns[i].text_buf->get_size().y +
						   theme_cache.title_button->get_minimum_size().height);
		}
	}
	return h;
}

void Tree::_check_item_accessibility(
	TreeItem* p_item, PackedStringArray& r_warnings, int& r_row) const
{
	for (int i = 0; i < p_item->cells.size(); i++) {
		const TreeItem::Cell& cell = p_item->cells[i];
		if (cell.description.strip_edges().is_empty() && cell.text.strip_edges().is_empty()) {
			r_warnings.push_back(vformat(
				RTR("Cell %d x %d: either text or alternative text must not be empty."), r_row, i));
		}
		for (int j = 0; j < cell.buttons.size(); j++) {
			if (cell.buttons[j].description.strip_edges().is_empty()) {
				r_warnings.push_back(vformat(
					RTR("Button %d in %d x %d: alternative text must not be empty."), j, r_row, i));
			}
		}
	}
	r_row++;

	// Children.
	if (!p_item->collapsed) {
		TreeItem* c = p_item->first_child;
		while (c) {
			_check_item_accessibility(c, r_warnings, r_row);
			c = c->next;
		}
	}
}

PackedStringArray Tree::get_accessibility_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_accessibility_configuration_warnings();

	if (root) {
		int row = 1;
		_check_item_accessibility(root, warnings, row);
	}

	return warnings;
}

RID Tree::get_focused_accessibility_element() const
{
	if (selected_item) {
		if (selected_col >= 0) {
			if (selected_button >= 0) {
				return selected_item->cells[selected_col]
					.buttons[selected_button]
					.accessibility_button_element;
			}
			else {
				return selected_item->cells[selected_col].accessibility_cell_element;
			}
		}
		else {
			return selected_item->accessibility_row_element;
		}
	}
	else {
		return get_accessibility_element();
	}
}

void Tree::_accessibility_clean_info(TreeItem* p_item)
{
	p_item->accessibility_row_element = RID();
	for (TreeItem::Cell& cell : p_item->cells) {
		cell.accessibility_cell_element = RID();
		for (TreeItem::Cell::Button& btn : cell.buttons) {
			btn.accessibility_button_element = RID();
		}
	}

	// Children.
	TreeItem* c = p_item->first_child;
	while (c) {
		_accessibility_clean_info(c);
		c = c->next;
	}
}

void Tree::_accessibility_update_item(Point2& r_ofs, TreeItem* p_item, int& r_row, int p_level)
{
	// Row.
	if ((p_item != root || !hide_root) && p_item->is_visible()) {
		if (p_item->accessibility_row_element.is_null()) {
			p_item->accessibility_row_element =
				AccessibilityServer::create_sub_element(
					accessibility_scroll_element,
					AccessibilityServerEnums::AccessibilityRole::ROLE_TREE_ITEM);
			p_item->accessibility_row_dirty = true;
		}

		AccessibilityServer::update_set_table_row_index(
			p_item->accessibility_row_element, r_row);
		AccessibilityServer::update_set_list_item_level(
			p_item->accessibility_row_element, p_level);
		AccessibilityServer::update_set_list_item_expanded(
			p_item->accessibility_row_element, !p_item->collapsed);
		AccessibilityServer::update_set_flag(p_item->accessibility_row_element,
			AccessibilityServerEnums::AccessibilityFlags::FLAG_HIDDEN,
			!(p_item->visible && !p_item->parent_visible_in_tree));

		AccessibilityServer::update_set_list_item_selected(
			p_item->accessibility_row_element, selected_item == p_item);
		if (p_item == root && is_root_hidden()) {
			AccessibilityServer::update_set_flag(p_item->accessibility_row_element,
				AccessibilityServerEnums::AccessibilityFlags::FLAG_HIDDEN, true);
		}
		Transform2D row_xform;
		row_xform.set_origin(r_ofs);
		AccessibilityServer::update_set_transform(
			p_item->accessibility_row_element, row_xform);

		Size2 item_size = Size2(get_size().width, compute_item_height(p_item));
		AccessibilityServer::update_set_bounds(
			p_item->accessibility_row_element, Rect2(Vector2(), item_size));

		if (p_item->accessibility_row_dirty) {
			// Cells.
			int col_offset = 0;
			for (int i = 0; i < p_item->cells.size(); i++) {
				TreeItem::Cell& cell = p_item->cells.write[i];

				if (cell.accessibility_cell_element.is_null()) {
					cell.accessibility_cell_element =
						AccessibilityServer::create_sub_element(
							p_item->accessibility_row_element,
							AccessibilityServerEnums::AccessibilityRole::ROLE_CELL);
				}

				float cw = get_column_width(i);

				AccessibilityServer::update_set_table_cell_position(
					cell.accessibility_cell_element, r_row, i);
				AccessibilityServer::update_set_list_item_selected(
					cell.accessibility_cell_element, cell.selected);
				if (cell.description.is_empty()) {
					AccessibilityServer::update_set_name(
						cell.accessibility_cell_element, cell.xl_text);
				}
				else {
					AccessibilityServer::update_set_name(
						cell.accessibility_cell_element, cell.description);
				}

				AccessibilityServer::update_set_text_align(
					cell.accessibility_cell_element, cell.text_alignment);
				AccessibilityServer::update_set_flag(
					cell.accessibility_cell_element,
					AccessibilityServerEnums::AccessibilityFlags::FLAG_HIDDEN,
					!(p_item->visible && !p_item->parent_visible_in_tree));
				AccessibilityServer::update_set_flag(
					cell.accessibility_cell_element,
					AccessibilityServerEnums::AccessibilityFlags::FLAG_READONLY, !cell.editable);
				AccessibilityServer::update_set_tooltip(
					cell.accessibility_cell_element, cell.tooltip);
				switch (cell.mode) {
				case TreeItem::CELL_MODE_STRING: {
					AccessibilityServer::update_set_value(
						cell.accessibility_cell_element, cell.xl_text);
				} break;
				case TreeItem::CELL_MODE_CHECK: {
					AccessibilityServer::update_set_checked(
						cell.accessibility_cell_element, cell.checked);
				} break;
				case TreeItem::CELL_MODE_RANGE: {
					AccessibilityServer::update_set_num_value(
						cell.accessibility_cell_element, cell.val);
					AccessibilityServer::update_set_num_range(
						cell.accessibility_cell_element, cell.min, cell.max);
					if (cell.step > 0) {
						AccessibilityServer::update_set_num_step(
							cell.accessibility_cell_element, cell.step);
					}
					else {
						AccessibilityServer::update_set_num_step(
							cell.accessibility_cell_element, 1);
					}
				} break;
				case TreeItem::CELL_MODE_ICON: {
					// NOP
				} break;
				case TreeItem::CELL_MODE_CUSTOM: {
				} break;
				}
				AccessibilityServer::update_set_background_color(
					cell.accessibility_cell_element, cell.color);
				AccessibilityServer::update_set_foreground_color(
					cell.accessibility_cell_element, cell.bg_color);

				AccessibilityServer::update_set_bounds(
					cell.accessibility_cell_element,
					Rect2(Point2(col_offset, 0), Size2(cw, item_size.y)));

				Vector2 ofst = Vector2(col_offset + cw, 0);
				for (int j = cell.buttons.size() - 1; j >= 0; j--) {
					if (cell.buttons[j].accessibility_button_element.is_null()) {
						cell.buttons[j].accessibility_button_element =
							AccessibilityServer::create_sub_element(
								cell.accessibility_cell_element,
								AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
					}

					AccessibilityServer::update_set_flag(
						cell.buttons[j].accessibility_button_element,
						AccessibilityServerEnums::AccessibilityFlags::FLAG_DISABLED,
						cell.buttons[j].disabled);
					AccessibilityServer::update_set_tooltip(
						cell.buttons[j].accessibility_button_element, cell.buttons[j].tooltip);
					if (cell.buttons[j].description.is_empty()) {
						AccessibilityServer::update_set_name(
							cell.buttons[j].accessibility_button_element, cell.buttons[j].tooltip);
					}
					else {
						AccessibilityServer::update_set_name(
							cell.buttons[j].accessibility_button_element,
							cell.buttons[j].description);
					}

					Ref<Texture2D> b = cell.buttons[j].texture;
					Size2 b_size = b->get_size() + theme_cache.button_pressed->get_minimum_size();
					ofst.x -= b_size.x;

					AccessibilityServer::update_set_bounds(
						cell.buttons[j].accessibility_button_element, Rect2(ofst, b_size));
				}
				col_offset += cw;
			}
		}

		r_ofs.y += item_size.y;
		r_ofs.y += theme_cache.v_separation;

		p_item->accessibility_row_dirty = false;
		r_row++;
	}

	// Children.
	if (!p_item->collapsed) {
		TreeItem* c = p_item->first_child;
		while (c) {
			_accessibility_update_item(r_ofs, c, r_row, p_level + 1);
			c = c->next;
		}
	}
}

void Tree::set_self_modulate(const Color& p_self_modulate)
{
	if (get_self_modulate() == p_self_modulate) {
		return;
	}

	CanvasItem::set_self_modulate(p_self_modulate);
	Renderer::canvas_item_set_self_modulate(header_ci, p_self_modulate);
	Renderer::canvas_item_set_self_modulate(last_sticky_ci, p_self_modulate);
	Renderer::canvas_item_set_self_modulate(content_ci, p_self_modulate);
	Renderer::canvas_item_set_self_modulate(custom_ci, p_self_modulate);
	Renderer::canvas_item_set_self_modulate(stylebox_ci, p_self_modulate);
}

void Tree::_update_all()
{
	cache.font_height = theme_cache.font->get_height(theme_cache.font_size);
	for (int i = 0; i < columns.size(); i++) {
		update_column(i);
	}
	if (root) {
		update_item_cache(root);
	}
}

Size2 Tree::get_minimum_size() const
{
	Vector2 min_size = Vector2(0, _get_title_button_height());

	if (theme_cache.panel_style.is_valid()) {
		min_size += theme_cache.panel_style->get_minimum_size();
	}

	Vector2 content_min_size = get_internal_min_size();
	if (h_scroll_enabled) {
		content_min_size.x = 0;
		min_size.y += h_scroll->get_bound_minimum_size().height;
	}
	if (v_scroll_enabled) {
		min_size.x += v_scroll->get_bound_minimum_size().width;
		content_min_size.y = 0;
	}

	return min_size + content_min_size;
}

TreeItem* Tree::create_item(TreeItem* p_parent, int p_index)
{
	ERR_FAIL_COND_V_MSG(
		blocked > 0, nullptr, "The tree cannot create items during mouse selection events.");

	TreeItem* ti = nullptr;

	if (p_parent) {
		ERR_FAIL_COND_V_MSG(
			p_parent->tree != this, nullptr, "A different tree owns the given parent");
		ti = p_parent->create_child(p_index);
	}
	else {
		if (!root) {
			// No root exists, make the given item the new root.
			ti = memnew(TreeItem(this));
			ERR_FAIL_NULL_V(ti, nullptr);
			ti->cells.resize(columns.size());
			ti->is_root = true;
			root = ti;
		}
		else {
			// Root exists, append or insert to root.
			ti = create_item(root, p_index);
		}
	}
	_queue_update_hovered_item();
	queue_accessibility_update();
	return ti;
}

TreeItem* Tree::get_root() const { return root; }

TreeItem* Tree::get_last_item() const
{
	TreeItem* last = root;
	while (last && last->last_child && !last->collapsed) {
		last = last->last_child;
	}

	return last;
}

void Tree::set_select_mode(SelectMode p_mode) { select_mode = p_mode; }

Tree::SelectMode Tree::get_select_mode() const { return select_mode; }

bool Tree::is_anything_selected() { return (selected_item != nullptr); }

bool Tree::is_root_hidden() const { return hide_root; }

bool Tree::is_column_expanding(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), false);

	return columns[p_column].expand;
}

int Tree::get_column_expand_ratio(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), 1);

	return columns[p_column].expand_ratio;
}

bool Tree::is_column_clipping_content(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), false);

	return columns[p_column].clip_content;
}

TreeItem* Tree::get_selected() const { return selected_item; }

void Tree::set_selected(TreeItem* p_item, int p_column)
{
	ERR_FAIL_INDEX(p_column, columns.size());
	ERR_FAIL_NULL(p_item);
	ERR_FAIL_COND_MSG(p_item->get_tree() != this,
		"The provided TreeItem does not belong to this Tree. Ensure that the TreeItem is a part of "
		"the Tree before setting it as selected.");

	select_single_item(p_item, get_root(), p_column);
}

int Tree::get_selected_column() const { return selected_col; }

TreeItem* Tree::get_edited() const { return edited_item; }

int Tree::get_edited_column() const { return edited_col; }

TreeItem* Tree::get_next_selected(TreeItem* p_item)
{
	if (!root) {
		return nullptr;
	}

	while (true) {
		if (!p_item) {
			p_item = root;
		}
		else {
			if (p_item->first_child) {
				p_item = p_item->first_child;

			}
			else if (p_item->next) {
				p_item = p_item->next;
			}
			else {
				while (!p_item->next) {
					p_item = p_item->parent;
					if (p_item == nullptr) {
						return nullptr;
					}
				}

				p_item = p_item->next;
			}
		}

		for (int i = 0; i < columns.size(); i++) {
			if (p_item->cells[i].selected) {
				return p_item;
			}
		}
	}

	return nullptr;
}

int Tree::get_column_minimum_width(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), -1);

	if (columns[p_column].cached_minimum_width_dirty) {
		// Use the custom minimum width.
		int min_width = columns[p_column].custom_min_width;

		// Check if the visible title of the column is wider.
		if (show_column_titles) {
			const float padding = theme_cache.title_button->get_margin(SIDE_LEFT) +
								  theme_cache.title_button->get_margin(SIDE_RIGHT);
			min_width = MAX(theme_cache.font
									->get_string_size(columns[p_column].xl_title,
										HORIZONTAL_ALIGNMENT_LEFT, -1, theme_cache.font_size)
									.width +
								padding,
				min_width);
		}

		if (root && !columns[p_column].clip_content) {
			int depth = 1;

			TreeItem* last = nullptr;
			TreeItem* first = hide_root ? root->get_next_visible() : root;
			for (TreeItem* item = first; item; last = item, item = item->get_next_visible()) {
				// Get column indentation.
				int indent;
				if (p_column == 0) {
					if (last) {
						if (item->parent == last) {
							depth += 1;
						}
						else if (item->parent != last->parent) {
							depth = hide_root ? 0 : 1;
							for (TreeItem* iter = item->parent; iter; iter = iter->parent) {
								depth += 1;
							}
						}
					}
					indent = theme_cache.item_margin * depth;
				}
				else {
					indent = theme_cache.h_separation;
				}

				// Get the item minimum size.
				Size2 item_size = item->get_minimum_size(p_column);
				item_size.width += indent;

				// Check if the item is wider.
				min_width = MAX(min_width, item_size.width);
			}
		}

		columns.get(p_column).cached_minimum_width = min_width;
		columns.get(p_column).cached_minimum_width_dirty = false;
	}

	return columns[p_column].cached_minimum_width;
}

int Tree::get_column_width(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), -1);

	int column_width = get_column_minimum_width(p_column);

	if (columns[p_column].expand) {
		int expand_area = _get_content_rect().size.width;
		int expanding_total = 0;

		for (int i = 0; i < columns.size(); i++) {
			expand_area -= get_column_minimum_width(i);
			if (columns[i].expand) {
				expanding_total += columns[i].expand_ratio;
			}
		}

		if (expand_area >= expanding_total && expanding_total > 0) {
			column_width += expand_area * columns[p_column].expand_ratio / expanding_total;
		}
	}

	return column_width;
}

void Tree::propagate_set_columns(TreeItem* p_item)
{
	p_item->cells.resize(columns.size());
	p_item->accessibility_row_dirty = true;
	for (TreeItem::Cell& cell : p_item->cells) {
		if (cell.accessibility_cell_element.is_valid()) {
			AccessibilityServer::free_element(cell.accessibility_cell_element);
			cell.accessibility_cell_element = RID();
		}
		for (TreeItem::Cell::Button& btn : cell.buttons) {
			if (btn.accessibility_button_element.is_valid()) {
				AccessibilityServer::free_element(
					btn.accessibility_button_element);
				btn.accessibility_button_element = RID();
			}
		}
	}

	TreeItem* c = p_item->get_first_child();
	while (c) {
		propagate_set_columns(c);
		c = c->next;
	}
}

int Tree::get_columns() const { return columns.size(); }

Rect2 Tree::get_custom_popup_rect() const { return custom_popup_rect; }

int Tree::get_item_offset(TreeItem* p_item) const
{
	TreeItem* it = root;
	int ofs = _get_title_button_height();
	if (!it) {
		return 0;
	}

	while (true) {
		if (it == p_item) {
			return ofs;
		}

		if ((it != root || !hide_root) && it->is_visible_in_tree()) {
			ofs += compute_item_height(it);
			ofs += theme_cache.v_separation;
		}

		if (it->first_child && !it->collapsed) {
			it = it->first_child;

		}
		else if (it->next) {
			it = it->next;
		}
		else {
			while (!it->next) {
				it = it->parent;
				if (it == nullptr) {
					return 0;
				}
			}

			it = it->next;
		}
	}

	return -1; // Not found.
}

void Tree::ensure_cursor_is_visible()
{
	if (!is_inside_tree()) {
		return;
	}
	if (!selected_item || (selected_col == -1)) {
		return; // Nothing under cursor.
	}

	// Note: Code below similar to `Tree::scroll_to_item()`, in case of bug fix both.
	const Size2 area_size = _get_content_rect().size;

	double delta_v = 0;
	int y_offset = get_item_offset(selected_item);
	if (y_offset != -1) {
		const int tbh = _get_title_button_height();
		y_offset -= tbh;

		bool selected_is_sticky = false;
		for (TreeItem* item : sticky_list) {
			if (selected_item == item) {
				y_offset -= item->sticky_offset.y;
				selected_is_sticky = true;
				break;
			}
		}

		if (!selected_is_sticky) {
			y_offset -= sticky_stack_end;
		}

		const int cell_h = compute_item_height(selected_item) + theme_cache.v_separation;
		int screen_h = area_size.height - tbh - sticky_stack_end;

		if (cell_h > screen_h) { // Screen size is too small, maybe it was not resized yet.
			delta_v = y_offset - v_scroll->get_value();
			v_scroll->set_value(y_offset);
		}
		else if (y_offset + cell_h > v_scroll->get_value() + screen_h) {
			delta_v = y_offset - screen_h + cell_h - v_scroll->get_value();
			scroll_pending++;
		}
		else if (y_offset < v_scroll->get_value()) {
			delta_v = y_offset - v_scroll->get_value();
			v_scroll->set_value(y_offset);
		}
	}

	double delta_h = 0;
	if (select_mode != SELECT_ROW) { // Cursor always at column 0 in this mode.
		int x_offset = 0;
		for (int i = 0; i < selected_col; i++) {
			x_offset += get_column_width(i);
		}

		const int cell_w = get_column_width(selected_col);
		const int screen_w = area_size.width;

		if (cell_w > screen_w) {
			delta_h = x_offset - h_scroll->get_value();
			scroll_pending++;
			h_scroll->set_value(x_offset);
		}
		else if (x_offset + cell_w > h_scroll->get_value() + screen_w) {
			delta_h = x_offset - screen_w + cell_w - h_scroll->get_value();
			scroll_pending++;
		}
		else if (x_offset < h_scroll->get_value()) {
			delta_h = x_offset - h_scroll->get_value();
			h_scroll->set_value(x_offset);
		}
	}

	// Move focus_rect.
	if (select_mode == SELECT_ROW) {
		Rect2 r = selected_item->cells[selected_col].focus_rect;
		r.position -= Vector2(delta_h, delta_v);
		selected_item->cells.write[selected_col].focus_rect = r;
	}
	else {
		Rect2 r = selected_item->focus_rect;
		r.position -= Vector2(delta_h, delta_v);
		selected_item->focus_rect = r;
	}

	queue_accessibility_update();
}

int Tree::get_pressed_button() const { return pressed_button; }

Rect2 Tree::get_item_rect(TreeItem* p_item, int p_column, int p_button) const
{
	ERR_FAIL_NULL_V(p_item, Rect2());
	ERR_FAIL_COND_V(p_item->tree != this, Rect2());
	if (p_column != -1) {
		ERR_FAIL_INDEX_V(p_column, columns.size(), Rect2());
	}
	if (p_button != -1) {
		ERR_FAIL_COND_V(p_column == -1, Rect2()); // Pass a column if you want to pass a button.
		ERR_FAIL_INDEX_V(p_button, p_item->cells[p_column].buttons.size(), Rect2());
	}

	int ofs = get_item_offset(p_item);
	int height = compute_item_height(p_item) + theme_cache.v_separation;
	Rect2 r;
	r.position.y = ofs - theme_cache.offset.y + theme_cache.panel_style->get_offset().y;
	r.size.height = height;
	bool rtl = is_layout_rtl();
	const Rect2 content_rect = _get_content_rect();

	if (p_column == -1) {
		r.position.x = 0;
		r.size.x = get_size().width;
	}
	else {
		int accum = 0;
		for (int i = 0; i < p_column; i++) {
			accum += get_column_width(i);
		}
		r.position.x =
			(rtl) ? get_size().x - (accum - theme_cache.offset.x) - get_column_width(p_column) -
						theme_cache.panel_style->get_margin(SIDE_LEFT)
				  : accum - theme_cache.offset.x + theme_cache.panel_style->get_margin(SIDE_LEFT);
		r.size.x = get_column_width(p_column);
		if (p_button != -1) {
			// RTL direction support for button rect is different due to buttons not
			// having same behavior as they do in LTR when tree is scrolled.
			const TreeItem::Cell& c = p_item->cells[p_column];
			Vector2 ofst =
				Vector2(r.position.x + r.size.x + theme_cache.button_margin, r.position.y);

			// Compute total width of buttons block including spacings.
			int buttons_width = 0;
			for (int j = c.buttons.size() - 1; j >= 0; j--) {
				Ref<Texture2D> b = c.buttons[j].texture;
				Size2 size = b->get_size() + theme_cache.button_pressed->get_minimum_size();
				buttons_width += size.width + theme_cache.button_margin;
			}
			for (int j = c.buttons.size() - 1; j >= 0; j--) {
				Ref<Texture2D> b = c.buttons[j].texture;
				Size2 size = b->get_size() + theme_cache.button_pressed->get_minimum_size();
				ofst.x -= size.x + theme_cache.button_margin;

				if (rtl) {
					if (j == p_button) {
						return Rect2(r.position, Size2(size.x, r.size.y));
					}
					r.position.x += size.x + theme_cache.button_margin;
					continue;
				}

				if (j == p_button) {
					float content_rect_end_x = content_rect.position.x + content_rect.size.width;
					if (r.position.x + r.size.x < content_rect_end_x) {
						return Rect2(ofst, Size2(size.x, r.size.y));
					}
					// Button block can be outside of `content_rect`.
					if (content_rect_end_x - (r.position.x + theme_cache.h_separation) <
						buttons_width) {
						return Rect2(
							r.position +
								Point2(theme_cache.h_separation +
										   (buttons_width - ((r.position.x + r.size.x) - ofst.x)),
									0),
							Size2(size.x, r.size.y));
					}

					return Rect2(ofst - Vector2((r.position.x + r.size.x) - content_rect_end_x, 0),
						Size2(size.x, r.size.y));
				}
			}
		}
	}

	return r;
}

bool Tree::are_column_titles_visible() const { return show_column_titles; }

String Tree::get_column_title(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].title;
}

void Tree::set_column_title_tooltip_text(int p_column, const String& p_tooltip)
{
	ERR_FAIL_INDEX(p_column, columns.size());
	columns.write[p_column].title_tooltip = p_tooltip;
}

String Tree::get_column_title_tooltip_text(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].title_tooltip;
}

HorizontalAlignment Tree::get_column_title_alignment(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	return columns[p_column].title_alignment;
}

Control::TextDirection Tree::get_column_title_direction(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), TEXT_DIRECTION_INHERITED);
	return columns[p_column].text_direction;
}

String Tree::get_column_title_language(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, columns.size(), "");
	return columns[p_column].language;
}

Point2 Tree::get_scroll() const
{
	Point2 ofs;
	if (h_scroll->is_visible_in_tree()) {
		ofs.x = h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		ofs.y = v_scroll->get_value();
	}
	return ofs;
}

bool Tree::is_h_scroll_enabled() const { return h_scroll_enabled; }

bool Tree::is_v_scroll_enabled() const { return v_scroll_enabled; }

Tree::ScrollHintMode Tree::get_scroll_hint_mode() const { return scroll_hint_mode; }

bool Tree::is_scroll_hint_tiled() { return tile_scroll_hint; }

TreeItem* Tree::_search_item_text(
	TreeItem* p_at, const String& p_find, int* r_col, bool p_selectable, bool p_backwards)
{
	TreeItem* from = p_at;
	TreeItem* loop = nullptr; // Safe-guard against infinite loop.

	while (p_at) {
		for (int i = 0; i < columns.size(); i++) {
			if (p_at->get_text(i).findn(p_find) == 0 && (!p_selectable || p_at->is_selectable(i))) {
				if (r_col) {
					*r_col = i;
				}
				return p_at;
			}
		}

		if (p_backwards) {
			p_at = p_at->get_prev_visible(true);
		}
		else {
			p_at = p_at->get_next_visible(true);
		}

		if ((p_at) == from) {
			break;
		}

		if (!loop) {
			loop = p_at;
		}
		else if (loop == p_at) {
			break;
		}
	}

	return nullptr;
}

TreeItem* Tree::search_item_text(const String& p_find, int* r_col, bool p_selectable)
{
	TreeItem* from = get_selected();

	if (!from) {
		from = root;
	}
	if (!from) {
		return nullptr;
	}

	return _search_item_text(from->get_next_visible(true), p_find, r_col, p_selectable);
}

TreeItem* Tree::get_item_with_text(const String& p_find) const
{
	for (TreeItem* current = root; current; current = current->get_next_visible()) {
		for (int i = 0; i < columns.size(); i++) {
			if (current->get_text(i) == p_find) {
				return current;
			}
		}
	}
	return nullptr;
}

void Tree::_do_incr_search(const String& p_add)
{
	uint64_t time = OS::get_ticks_usec() / 1000; // Convert to msec.
	uint64_t diff = time - last_keypress;
	if (diff >
		uint64_t(GLOBAL_GET_CACHED(uint64_t, "gui/timers/incremental_search_max_interval_msec"))) {
		incr_search = p_add;
	}
	else if (incr_search != p_add) {
		incr_search += p_add;
	}

	last_keypress = time;
	int col;
	TreeItem* item = search_item_text(incr_search, &col, true);
	if (!item) {
		return;
	}

	if (select_mode == SELECT_MULTI) {
		item->set_as_cursor(col);
	}
	else {
		item->select(col);
	}
	ensure_cursor_is_visible();
}

TreeItem* Tree::_find_item_at_pos(TreeItem* p_item, const Point2& p_pos, int& r_column,
	int& r_height, int& r_section, bool p_sticky_check) const
{
	r_column = -1;
	r_height = 0;
	r_section = COLUMN_NOT_FOUND;

	if (!root) {
		return nullptr;
	}

	if (p_sticky_check) {
		Point2 relative_pos = p_pos - theme_cache.offset;
		for (TreeItem* item : sticky_list) {
			if (item && relative_pos.y >= item->sticky_offset.y &&
				relative_pos.y < item->sticky_offset.y + item->cached_label_height) {
				item = _find_item_at_pos(item, relative_pos - Point2(0, item->cached_start),
					r_column, r_height, r_section, false);
				return item;
			}
		}
	}

	Point2 pos = p_pos;

	// Calculate target parent based on horizontal mouse position.
	int indent_width = theme_cache.item_margin;
	int target_depth =
		MAX(0, (p_pos.x / indent_width) + 1); // +1 to overlap with relationship lines.
	int current_item_depth = 0;
	for (TreeItem* depth_parent = p_item->get_parent(); depth_parent;
		 depth_parent = depth_parent->get_parent()) {
		if (depth_parent == root && hide_root) {
			break;
		}
		current_item_depth++;
	}

	if (drop_mode_flags && drop_mode_flags != DROP_MODE_ON_ITEM) {
		// Clamp maximum depth to current item's depth.
		target_depth = MIN(target_depth, current_item_depth);
	}
	else if (drop_mode_flags == DROP_MODE_ON_ITEM) {
		// Ignore indent space and always target items directly.
		target_depth = current_item_depth;
	}

	bool is_at_target_depth = (drop_mode_flags && current_item_depth == target_depth);

	if ((root != p_item || !hide_root) && p_item->is_visible_in_tree()) {
		r_height = compute_item_height(p_item) + theme_cache.v_separation;

		bool within_vertical_bounds = (pos.y < r_height);

		if (within_vertical_bounds) {
			bool skip_item = false;
			if (drop_mode_flags && current_item_depth != target_depth) {
				// Skip items not at target depth.
				skip_item = true;
			}

			if (!skip_item) {
				int item_content_start_x = current_item_depth * theme_cache.item_margin;
				if (!p_item->first_child) {
					item_content_start_x +=
						((p_item->disable_folding || hide_folding) ? theme_cache.h_separation
																   : theme_cache.item_margin);
				}
				bool over_content_area = (p_pos.x >= item_content_start_x);

				bool item_allows_drop = p_item->is_accepting_children();
				bool parent_allows_drop =
					p_item->get_parent() && p_item->get_parent()->is_accepting_children();

				if (drop_mode_flags == DROP_MODE_ON_ITEM) {
					// When only on item drops are allowed.
					r_section = item_allows_drop ? 0 : COLUMN_NOT_FOUND;
				}
				else if (drop_mode_flags == DROP_MODE_INBETWEEN) {
					// When only sibling drops are allowed.
					if (parent_allows_drop) {					   // Parent must accept children.
						r_section = pos.y < r_height / 2 ? -1 : 1; // Above or below.
					}
					else {
						r_section = COLUMN_NOT_FOUND;
					}
				}
				else if (drop_mode_flags) {
					// When either are allowed, section is determined by cursor position.
					if (!over_content_area) {
						// In indent space.
						if (pos.y < r_height / 2 && current_item_depth > 0) {
							r_section =
								parent_allows_drop ? -1 : COLUMN_NOT_FOUND; // Sibling above.
						}
						else {
							r_section = parent_allows_drop ? 1 : COLUMN_NOT_FOUND; // Sibling below.
						}
					}
					else if (pos.y < r_height / 4) {
						// Near top edge.
						bool item_can_have_siblings =
							(p_item != root) &&
							(current_item_depth > 0 || hide_root); // Disqualify root.
						if (item_can_have_siblings) {
							r_section =
								parent_allows_drop ? -1 : COLUMN_NOT_FOUND; // Sibling above.
						}
						else {
							r_section =
								item_allows_drop
									? 0
									: COLUMN_NOT_FOUND; // On root, or as last child of root.
						}
					}
					else if (pos.y >= r_height * 3 / 4) {
						// Near bottom edge.
						if (p_item->first_child && item_allows_drop) {
							r_section = 2; // As first child.
						}
						else {
							r_section = parent_allows_drop ? 1 : COLUMN_NOT_FOUND; // Sibling below.
						}
					}
					else {
						// Middle zone.
						if (item_allows_drop) {
							r_section = 0; // On item, or as last child.
						}
						else if (parent_allows_drop) {
							r_section = pos.y < r_height / 2 ? -1 : 1; // Above or below as sibling.
						}
						else {
							r_section = COLUMN_NOT_FOUND;
						}
					}
				}
				else {
					r_section = 0;
				}

				for (int i = 0; i < columns.size(); i++) {
					int col_width = get_column_width(i);

					if (p_item->cells[i].expand_right) {
						int plus = 1;
						while (i + plus < columns.size() && !p_item->cells[i + plus].editable &&
							   p_item->cells[i + plus].mode == TreeItem::CELL_MODE_STRING &&
							   p_item->cells[i + plus].text.is_empty() &&
							   p_item->cells[i + plus].icon.is_null() &&
							   p_item->cells[i + plus].buttons.is_empty()) {
							col_width += theme_cache.h_separation;
							col_width += get_column_width(i + plus);
							plus++;
						}
					}

					if (pos.x < col_width) {
						r_column = i;
						return p_item;
					}
					pos.x -= col_width;
				}
				return nullptr;
			}
		}
		else {
			pos.y -= r_height;
		}
	}
	else {
		r_height = 0;
	}

	if (p_item->is_collapsed() || !p_item->is_visible_in_tree()) {
		return nullptr; // Do not try children, it's collapsed.
	}

	TreeItem* n = p_item->get_first_child();
	while (n) {
		int ch;
		TreeItem* r = _find_item_at_pos(n, pos, r_column, ch, r_section, false);
		pos.y -= ch;
		r_height += ch;
		if (r) {
			return r;
		}
		n = n->get_next();
	}

	// Fallback: if in drop mode, at target depth, and mouse is below all nodes (pos.y is now
	// negative after subtracting all children).
	if (drop_mode_flags && is_at_target_depth && (root != p_item || !hide_root) &&
		p_item->is_visible_in_tree() && pos.y < 0) {
		// Dropping below the last item at specified depth level.
		if (!p_item->get_parent() || !p_item->get_parent()->is_accepting_children()) {
			return nullptr;
		}
		r_section = 1;

		Point2 col_pos = p_pos;
		for (int i = 0; i < columns.size(); i++) {
			int col_width = get_column_width(i);

			if (p_item->cells[i].expand_right) {
				int plus = 1;
				while (i + plus < columns.size() && !p_item->cells[i + plus].editable &&
					   p_item->cells[i + plus].mode == TreeItem::CELL_MODE_STRING &&
					   p_item->cells[i + plus].text.is_empty() &&
					   p_item->cells[i + plus].icon.is_null() &&
					   p_item->cells[i + plus].buttons.is_empty()) {
					col_width += theme_cache.h_separation;
					col_width += get_column_width(i + plus);
					plus++;
				}
			}

			if (col_pos.x < col_width) {
				r_column = i;
				return p_item;
			}
			col_pos.x -= col_width;
		}
	}

	return nullptr;
}

void Tree::_find_button_at_pos(
	const Point2& p_pos, TreeItem*& r_item, int& r_column, int& r_index, int& r_section) const
{
	// When on a button, `r_index` is valid.
	// When on an item, both `r_item` and `r_column` are valid.
	// Otherwise, all output arguments are invalid.
	r_item = nullptr;
	r_column = -1;
	r_index = -1;
	r_section = -1;

	if (!root) {
		return;
	}

	Point2 pos = p_pos;
	if (cache.rtl) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return;
	}
	pos += theme_cache.offset; // Scrolling.

	int x_limit = _get_content_rect().size.x + theme_cache.offset.width;

	if (pos.x > x_limit) {
		// Inside the scroll area.
		return;
	}

	int col, h, section;
	TreeItem* it = _find_item_at_pos(root, pos, col, h, section);
	if (!it) {
		return;
	}

	r_item = it;
	r_column = col;
	r_section = section;

	const TreeItem::Cell& c = it->cells[col];
	if (c.buttons.is_empty()) {
		return;
	}

	// Search the button index.
	FindColumnButtonResult result = _find_column_and_button_at_pos(pos.x, it, 0, x_limit);
	r_index = result.button_index;
}

Tree::FindColumnButtonResult Tree::_find_column_and_button_at_pos(
	int p_x, const TreeItem* p_item, int p_x_ofs, int p_x_limit) const
{
	int x = p_x;

	int col = -1;
	int col_ofs = 0;
	int col_width = 0;

	int limit_w = p_x_limit;

	FindColumnButtonResult result;

	for (int i = 0; i < columns.size(); i++) {
		col_width = get_column_width(i);

		if (p_item->cells[i].expand_right) {
			int plus = 1;
			while (i + plus < columns.size() && !p_item->cells[i + plus].editable &&
				   p_item->cells[i + plus].mode == TreeItem::CELL_MODE_STRING &&
				   p_item->cells[i + plus].text.is_empty() &&
				   p_item->cells[i + plus].icon.is_null()) {
				col_width += theme_cache.h_separation;
				col_width += get_column_width(i + plus);
				plus++;
			}
		}

		if (x < col_width) {
			col = i;
			break;
		}

		col_ofs += col_width;
		x -= col_width;
		limit_w -= col_width;
	}

	if (col >= 0) {
		if (col == 0) {
			int margin = p_x_ofs + theme_cache.item_margin;
			col_width -= margin;
			limit_w -= margin;
			col_ofs += margin;
			x -= margin;
		}
		else {
			col_width -= theme_cache.h_separation;
			limit_w -= theme_cache.h_separation;
			x -= theme_cache.h_separation;
		}

		if (!cache.rtl && !p_item->cells[col].buttons.is_empty()) {
			int button_w = 0;
			for (int j = p_item->cells[col].buttons.size() - 1; j >= 0; j--) {
				const Ref<Texture2D>& b = p_item->cells[col].buttons[j].texture;
				button_w += b->get_size().width +
							theme_cache.button_pressed->get_minimum_size().width +
							theme_cache.button_margin;
			}

			col_width = MAX(button_w, MIN(limit_w, col_width));
		}

		// Cell button detection code.
		// The first half of the button margin will be applied to the left button,
		// and the other half to the right button.
		const int offset_button_width = theme_cache.button_margin / 2;
		const TreeItem::Cell& c = p_item->cells[col];
		for (int j = c.buttons.size() - 1; j >= 0; j--) {
			const Ref<Texture2D>& b = c.buttons[j].texture;
			int w = b->get_size().width + theme_cache.button_pressed->get_minimum_size().width;

			if (x >= col_width - w - offset_button_width - 1) {
				result.button_index = j;
				break;
			}

			col_width -= w + theme_cache.button_margin;
		}
	}

	result.column_index = col;
	result.column_offset = col_ofs;
	result.column_width = col_width;
	result.pos_x = x;

	return result;
}

int Tree::get_column_at_position(const Point2& p_pos) const
{
	if (!root || !Rect2(Vector2(), get_size()).has_point(p_pos)) {
		return -1;
	}

	Point2 pos = p_pos;
	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return -1;
	}

	if (h_scroll->is_visible_in_tree()) {
		pos.x += h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		pos.y += v_scroll->get_value();
	}

	int col, h, section;
	TreeItem* it = _find_item_at_pos(root, pos, col, h, section);

	if (it) {
		return col;
	}
	return -1;
}

int Tree::get_drop_section_at_position(const Point2& p_pos) const
{
	if (!root || !Rect2(Vector2(), get_size()).has_point(p_pos) || !drop_mode_flags) {
		return COLUMN_NOT_FOUND;
	}

	Point2 pos = p_pos;
	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return COLUMN_NOT_FOUND;
	}

	if (h_scroll->is_visible_in_tree()) {
		pos.x += h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		pos.y += v_scroll->get_value();
	}

	int col, h, section;
	TreeItem* it = _find_item_at_pos(root, pos, col, h, section);

	if (it) {
		if (dragging_within_self) {
			TreeItem* check = it->get_parent();
			while (check) {
				if (check->is_any_column_selected()) {
					return COLUMN_NOT_FOUND;
				}
				check = check->get_parent();
			}
		}
		return section;
	}
	return COLUMN_NOT_FOUND;
}

TreeItem* Tree::get_item_at_position(const Point2& p_pos) const
{
	if (!root || !Rect2(Vector2(), get_size()).has_point(p_pos)) {
		return nullptr;
	}

	Point2 pos = p_pos;
	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - 1;
	}
	Point2 margin_offset = theme_cache.panel_style->get_offset();
	pos -= margin_offset;
	pos.y -= _get_title_button_height();
	if (pos.y + margin_offset.y < 0) {
		return nullptr;
	}

	if (h_scroll->is_visible_in_tree()) {
		pos.x += h_scroll->get_value();
	}
	if (v_scroll->is_visible_in_tree()) {
		pos.y += v_scroll->get_value();
	}

	int col, h, section;
	TreeItem* it = _find_item_at_pos(root, pos, col, h, section);

	if (it) {
		return it;
	}
	return nullptr;
}

int Tree::get_button_id_at_position(const Point2& p_pos) const
{
	if (!root) {
		return -1;
	}

	if (p_pos == Vector2(Math::INF, Math::INF)) {
		if (selected_item && selected_button >= 0) {
			return selected_item->cells[selected_col].buttons[selected_button].id;
		}
	}
	else {
		if (!Rect2(Vector2(), get_size()).has_point(p_pos)) {
			return -1;
		}

		TreeItem* it;
		int col, index, section;
		_find_button_at_pos(p_pos, it, col, index, section);

		if (index == -1) {
			return -1;
		}
		return it->cells[col].buttons[index].id;
	}
	return -1;
}

Node::AutoTranslateMode Tree::get_tooltip_auto_translate_mode_at(const Point2& p_at) const
{
	Point2 pos = p_at - theme_cache.panel_style->get_offset();
	pos.y -= _get_title_button_height();

	// `pos.y` less than 0 indicates we're in the header.
	if (pos.y < 0) {
		// Columns don't have auto-translate mode, so return the default.
		return Control::get_tooltip_auto_translate_mode_at(p_at);
	}

	TreeItem* it;
	int col, index, section;
	_find_button_at_pos(p_at, it, col, index, section);

	if (it) {
		return it->get_auto_translate_mode(col);
	}
	return Control::get_tooltip_auto_translate_mode_at(p_at);
}

void Tree::set_cursor_can_exit_tree(bool p_enable) { cursor_can_exit_tree = p_enable; }

bool Tree::is_folding_hidden() const { return hide_folding; }

void Tree::set_enable_recursive_folding(bool p_enable) { enable_recursive_folding = p_enable; }

bool Tree::is_recursive_folding_enabled() const { return enable_recursive_folding; }

void Tree::set_enable_drag_unfolding(bool p_enable)
{
	if (enable_drag_unfolding == p_enable) {
		return;
	}

	enable_drag_unfolding = p_enable;
	if (!enable_drag_unfolding && !dropping_unfold_timer->is_stopped()) {
		dropping_unfold_timer->stop();
	}
}

bool Tree::is_drag_unfolding_enabled() const { return enable_drag_unfolding; }

int Tree::get_drop_mode_flags() const { return drop_mode_flags; }

void Tree::set_edit_checkbox_cell_only_when_checkbox_is_pressed(bool p_enable)
{
	force_edit_checkbox_only_on_checkbox = p_enable;
}

bool Tree::get_edit_checkbox_cell_only_when_checkbox_is_pressed() const
{
	return force_edit_checkbox_only_on_checkbox;
}

void Tree::set_allow_rmb_select(bool p_allow) { allow_rmb_select = p_allow; }

bool Tree::get_allow_rmb_select() const { return allow_rmb_select; }

void Tree::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool Tree::get_allow_reselect() const { return allow_reselect; }

void Tree::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool Tree::get_allow_search() const { return allow_search; }

void Tree::set_auto_tooltip(bool p_enable) { enable_auto_tooltip = p_enable; }

bool Tree::is_auto_tooltip_enabled() const { return enable_auto_tooltip; }

void Tree::item_changed(int p_column, TreeItem* p_item) {}

Tree::~Tree()
{
	memdelete(root);
	Renderer::free_rid(drop_indicator_ci);
	Renderer::free_rid(content_ci);
	Renderer::free_rid(custom_ci);
	Renderer::free_rid(header_ci);
	Renderer::free_rid(stylebox_ci);
	Renderer::free_rid(last_sticky_ci);
}

void TreeItem::propagate_check(int p_column, bool p_emit_signal) {}

void Tree::set_scroll_hint_mode(ScrollHintMode p_mode) {}

void Tree::update_item_cell(TreeItem* p_item, int p_col) const {}

void Tree::clear() {}



void Tree::set_column_title(int, String const&) {}

void Tree::item_selected(int, TreeItem*, bool) {}

void TreeItem::_change_tree(Tree*) {}

void Tree::deselect_all() {}

void Tree::item_deselected(int, TreeItem*) {}

void Tree::_apply_multiline_edit(bool) {}

void Tree::update_column(int) {}

void Tree::set_hide_root(bool) {}

void Tree::set_hide_folding(bool) {}

void Tree::set_column_titles_visible(bool) {}

void Tree::set_columns(int) {}

void Tree::select_single_item(TreeItem*, TreeItem*, int, TreeItem*, bool*, bool) {}

void Tree::_line_editor_submit(String) {}

void TreeItem::set_as_cursor(int) {}

void TreeItem::move_before(TreeItem*) {}

void TreeItem::move_after(TreeItem*) {}

TreeItem* TreeItem::create_child(int) {}

void Tree::_determine_hovered_item() {}

// --- tree.compat.inc ---

#ifndef DISABLE_DEPRECATED


void TreeItem::_select_bind_compat_119367(int p_column) { select(p_column, true); }

void TreeItem::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from scene/gui/label.cpp
// ============================================================================

#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "scene/gui/dialogs.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "scene/gui/dialogs.h"
#include "scene/resources/image_texture.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "servers/audio/audio_server.h"
// --- Inlined from scene/gui/video_stream_player.h ---
#include "scene/gui/control.h"
#include "scene/resources/packed_scene.h"
#include "servers/audio/audio_server.h"

class VideoStreamPlayer : public Control
{
	Ref<VideoStreamPlayback> playback;
	Ref<VideoStream> stream;

	int sp_get_channel_count() const;
	bool mix(AudioFrame* p_buffer, int p_frames);

	Ref<Texture2D> texture;
	Size2 texture_size;
	void texture_changed(const Ref<Texture2D>& p_texture);

	AudioRBResampler resampler;
	Vector<AudioFrame> mix_buffer;
	int wait_resampler = 0;
	int wait_resampler_limit = 2;

	bool paused = false;
	bool paused_from_tree = false;
	bool autoplay = false;
	float volume = 1.0;
	float speed_scale = 1.0;
	bool expand = false;
	bool loop = false;
	bool first_frame = false;
	int buffering_ms = 500;
	int audio_track = 0;
	int bus_index = 0;

	String bus;

	void _mix_audio();
	static int _audio_mix_callback(void* p_udata, const float* p_data, int p_frames);
	static void _mix_audios(void* p_self);

protected:
	void _notification(int p_notification);

public:
	Size2 get_minimum_size() const override;
	void set_expand(bool p_expand);
	bool has_expand() const;

	Ref<Texture2D> get_video_texture() const;

	void set_stream(const Ref<VideoStream>& p_stream);
	Ref<VideoStream> get_stream() const;

	void play();
	void stop();
	bool is_playing() const;

	void set_loop(bool p_loop);
	bool has_loop() const;

	void set_paused(bool p_paused);
	bool is_paused() const;

	void set_volume(float p_vol);
	float get_volume() const;

	void set_volume_db(float p_db);
	float get_volume_db() const;

	void set_speed_scale(float p_speed_scale);
	float get_speed_scale() const;

	String get_stream_name() const;
	double get_stream_length() const;
	double get_stream_position() const;
	void set_stream_position(double p_position);

	void set_autoplay(bool p_enable);
	bool has_autoplay() const;

	void set_audio_track(int p_track);
	int get_audio_track() const;

	void set_buffering_msec(int p_msec);
	int get_buffering_msec() const;

	void set_bus(const String& p_bus);
	String get_bus() const;

	~VideoStreamPlayer();
};



// --- End scene/gui/video_stream_player.h ---
// --- Inlined from scene/gui/virtual_joystick.h ---
#include "scene/gui/control.h"

class VirtualJoystick : public Control
{
public:
	enum JoystickMode
	{
		JOYSTICK_FIXED,
		JOYSTICK_DYNAMIC,
		JOYSTICK_FOLLOWING,
	};

	enum VisibilityMode
	{
		VISIBILITY_ALWAYS,
		VISIBILITY_WHEN_TOUCHED,
	};

private:
	JoystickMode joystick_mode = JOYSTICK_FIXED;
	float joystick_size = 100.0f;
	float tip_size = 50.0f;
	float deadzone_ratio = 0.0f;
	float clampzone_ratio = 1.0f;
	Vector2 initial_offset_ratio = Vector2(0.5, 0.5);
	String action_left = "ui_left";
	String action_right = "ui_right";
	String action_up = "ui_up";
	String action_down = "ui_down";
	VisibilityMode visibility = VISIBILITY_ALWAYS;

	struct ThemeCache
	{
		Ref<StyleBox> normal_joystick;
		Ref<StyleBox> normal_tip;
		Ref<StyleBox> pressed_joystick;
		Ref<StyleBox> pressed_tip;
	} theme_cache;

	bool is_pressed = false;
	bool has_input = false;
	bool has_moved = false;
	Vector2 raw_input_vector;
	Vector2 input_vector;
	bool is_flick_canceled = false;
	int touch_index = -1;

	Vector2 joystick_pos;
	Vector2 tip_pos;

	void _update_joystick(const Vector2& p_pos);
	void _handle_input_actions();
	void _reset();

protected:
	virtual void gui_input(const InputEvent& p_event);
	void _notification(int p_what);

public:
	Vector2 get_joystick_position() const;

	void set_joystick_mode(JoystickMode p_mode);
	JoystickMode get_joystick_mode() const;

	void set_joystick_size(float p_size);
	float get_joystick_size() const;

	void set_tip_size(float p_size);
	float get_tip_size() const;

	void set_deadzone_ratio(float p_ratio);
	float get_deadzone_ratio() const;

	void set_clampzone_ratio(float p_ratio);
	float get_clampzone_ratio() const;

	void set_initial_offset_ratio(const Vector2& p_ratio);
	Vector2 get_initial_offset_ratio() const;

	void set_action_left(const String& p_action);
	String get_action_left() const;
	void set_action_right(const String& p_action);
	String get_action_right() const;
	void set_action_up(const String& p_action);
	String get_action_up() const;
	void set_action_down(const String& p_action);
	String get_action_down() const;

	void set_visibility_mode(VisibilityMode p_mode);
	VisibilityMode get_visibility_mode() const;
};



// --- End scene/gui/virtual_joystick.h ---

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

TextServer::AutowrapMode Label::get_autowrap_mode() const { return autowrap_mode; }

uint32_t Label::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

uint32_t Label::get_justification_flags() const { return jst_flags; }

bool Label::is_uppercase() const { return uppercase; }

int Label::get_line_height(int p_line) const
{
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	if (p_line >= 0 && p_line < total_line_count) {
		RID rid = get_line_rid(p_line);
		double asc = TS->shaped_text_get_ascent(rid);
		double dsc = TS->shaped_text_get_descent(rid);
		if (asc + dsc < font_h) {
			double diff = font_h - (asc + dsc);
			asc += diff / 2;
			dsc += diff - (diff / 2);
		}
		return asc + dsc;
	}
	else if (total_line_count > 0) {
		int h = font_h;
		for (const Paragraph& para : paragraphs) {
			for (const RID& line_rid : para.lines_rid) {
				h = MAX(h, TS->shaped_text_get_size(line_rid).y);
			}
		}
		return h;
	}
	else {
		return font->get_height(font_size);
	}
}

void Label::_update_visible() const
{
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int lines_visible = total_line_count;

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	minsize.height = 0;
	int last_line = MIN(total_line_count, lines_visible + lines_skipped);

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size()
																	   : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				minsize.height += asc + dsc + line_spacing;
			}
			minsize.height += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (minsize.height > 0) {
		minsize.height -= (line_spacing + paragraph_spacing);
	}
}

inline void draw_glyph(
	const Glyph& p_gl, const RID& p_canvas, const Color& p_font_color, const Vector2& p_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_color);
	}
	else if (((p_gl.flags & TextServer::GRAPHEME_IS_VIRTUAL) !=
				   TextServer::GRAPHEME_IS_VIRTUAL) &&
			   ((p_gl.flags & TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) !=
				   TextServer::GRAPHEME_IS_EMBEDDED_OBJECT)) {
		TS->draw_hex_code_box(p_canvas, p_gl.font_size, p_ofs + Vector2(p_gl.x_off, p_gl.y_off),
			p_gl.index, p_font_color);
	}
}

inline void draw_glyph_shadow(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_shadow_color, const Vector2& p_ofs, const Vector2& shadow_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph(p_gl.font_rid, p_canvas, p_gl.font_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + shadow_ofs, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_shadow_outline(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_shadow_color, const Vector2& p_ofs, int p_shadow_outline_size,
	const Vector2& shadow_ofs)
{
	if (p_gl.font_rid != RID()) {
		TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_shadow_outline_size,
			p_ofs + Vector2(p_gl.x_off, p_gl.y_off) + shadow_ofs, p_gl.index, p_font_shadow_color);
	}
}

inline void draw_glyph_outline(const Glyph& p_gl, const RID& p_canvas,
	const Color& p_font_outline_color, const Vector2& p_ofs, int p_outline_size)
{
	if (p_gl.font_rid != RID()) {
		if (p_font_outline_color.a != 0.0 && p_outline_size > 0) {
			TS->font_draw_glyph_outline(p_gl.font_rid, p_canvas, p_gl.font_size, p_outline_size,
				p_ofs + Vector2(p_gl.x_off, p_gl.y_off), p_gl.index, p_font_outline_color);
		}
	}
}

void Label::_ensure_shaped() const
{
	if (dirty || font_dirty || text_dirty) {
		_shape();
	}
	else {
		for (const Paragraph& para : paragraphs) {
			if (para.lines_dirty || para.dirty) {
				_shape();
				return;
			}
		}
	}
}

RID Label::get_line_rid(int p_line) const
{
	if (p_line < 0 || p_line >= total_line_count) {
		return RID();
	}

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		}
		else {
			return para.lines_rid[p_line - line_index];
		}
	}

	return RID();
}

Rect2 Label::get_line_rect(int p_line) const
{
	if (p_line < 0 || p_line >= total_line_count) {
		return Rect2();
	}

	int line_index = 0;
	for (int p = 0; p < paragraphs.size(); p++) {
		const Paragraph& para = paragraphs[p];
		if (line_index + para.lines_rid.size() <= p_line) {
			line_index += para.lines_rid.size();
		}
		else {
			return _get_line_rect(p, p_line - line_index);
		}
	}

	return Rect2();
}

Rect2 Label::_get_line_rect(int p_para, int p_line) const
{
	// Returns a rect providing the line's horizontal offset and total size. To determine the
	// vertical offset, use r_offset and r_line_spacing from get_layout_data.
	bool rtl = TS->shaped_text_get_inferred_direction(paragraphs[p_para].text_rid) ==
			   TextServer::DIRECTION_RTL;
	bool rtl_layout = is_layout_rtl();
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	Size2 size = get_size();
	RID rid = paragraphs[p_para].lines_rid[p_line];
	Size2 line_size = TS->shaped_text_get_size(rid);
	double asc = TS->shaped_text_get_ascent(rid);
	double dsc = TS->shaped_text_get_descent(rid);
	if (asc + dsc < font_h) {
		double diff = font_h - (asc + dsc);
		asc += diff / 2;
		dsc += diff - (diff / 2);
	}
	line_size.y = asc + dsc;
	Vector2 offset;
	switch (horizontal_alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
		if (rtl && autowrap_mode != TextServer::AUTOWRAP_OFF) {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
		else {
			offset.x = style->get_offset().x;
		}
		break;
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl_layout) {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
		else {
			offset.x = style->get_offset().x;
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		offset.x = int(size.width - line_size.width) / 2;
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl_layout) {
			offset.x = style->get_offset().x;
		}
		else {
			offset.x = int(size.width - style->get_margin(SIDE_RIGHT) - line_size.width);
		}
	} break;
	}
	return Rect2(offset, line_size);
}

int Label::get_layout_data(Vector2& r_offset, int& r_last_line, int& r_line_spacing) const
{
	// Computes several common parameters involved in laying out and rendering text set to this
	// label. Only vertical margin is considered in r_offset: use get_line_rect to get the
	// horizontal offset for a given line of text.
	Size2 size = get_size();
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	float total_h = 0.0;
	int lines_visible = 0;

	// Get number of lines to fit to the height.
	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines_rid.size(); i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				if (total_h > Math::ceil(get_size().height - style->get_minimum_size().height +
										 line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	r_last_line = MIN(total_line_count, lines_visible + lines_skipped);

	// Get real total height.
	int total_glyphs = 0;
	total_h = 0;
	line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < r_last_line) ? para.lines_rid.size()
																		 : r_last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				total_glyphs += TS->shaped_text_get_glyph_count(para.lines_rid[i]) +
								TS->shaped_text_get_ellipsis_glyph_count(para.lines_rid[i]);
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	total_h += style->get_margin(SIDE_TOP) + style->get_margin(SIDE_BOTTOM);
	int vbegin = 0, vsep = 0;
	if (lines_visible > 0) {
		switch (vertical_alignment) {
		case VERTICAL_ALIGNMENT_TOP: {
			// Nothing.
		} break;
		case VERTICAL_ALIGNMENT_CENTER: {
			vbegin = (size.y - (total_h - line_spacing - paragraph_spacing)) / 2;
			vsep = 0;
		} break;
		case VERTICAL_ALIGNMENT_BOTTOM: {
			vbegin = size.y - (total_h - line_spacing - paragraph_spacing);
			vsep = 0;
		} break;
		case VERTICAL_ALIGNMENT_FILL: {
			vbegin = 0;
			if (lines_visible > 1) {
				vsep =
					(size.y - (total_h - line_spacing - paragraph_spacing)) / (lines_visible - 1);
			}
			else {
				vsep = 0;
			}
		} break;
		}
	}
	r_offset = {0, style->get_offset().y + vbegin};
	r_line_spacing = line_spacing + vsep;

	return total_glyphs;
}

PackedStringArray Label::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	// FIXME: This is not ideal and the sizing model should be fixed,
	// but for now we have to warn about this impossible to resolve combination.
	// See GH-83546.
	if (is_inside_tree() && SceneTree::get_edited_scene_root() != this) {
		if (autowrap_mode != TextServer::AUTOWRAP_OFF && get_combined_maximum_size().width <= 0 &&
			get_custom_minimum_size().width <= 0) {
			warnings.push_back(RTR("Labels with autowrapping enabled must have a positive custom "
								   "minimum or maximum width configured to work correctly."));
		}
	}

	// Ensure that the font can render all of the required glyphs.
	Ref<Font> font;
	if (settings.is_valid()) {
		font = settings->get_font();
	}
	if (font.is_null()) {
		font = theme_cache.font;
	}

	if (font.is_valid()) {
		_ensure_shaped();

		for (const Paragraph& para : paragraphs) {
			const Glyph* glyph = TS->shaped_text_get_glyphs(para.text_rid);
			int64_t glyph_count = TS->shaped_text_get_glyph_count(para.text_rid);
			for (int64_t i = 0; i < glyph_count; i++) {
				if (glyph[i].font_rid == RID() && glyph[i].index != 0) {
					warnings.push_back(RTR("The current font does not support rendering one or "
										   "more characters used in this Label's text."));
					break;
				}
			}
		}
	}

	Ref<FontFile> ff = font;
	if (ff.is_valid() && ff->is_multichannel_signed_distance_field()) {
		bool has_settings = settings.is_valid();
		int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
		int outline_size =
			has_settings ? settings->get_outline_size() : theme_cache.font_outline_size;
		Vector<LabelSettings::StackedOutlineData> stacked_outline_datas =
			has_settings ? settings->get_stacked_outline_data()
						 : Vector<LabelSettings::StackedOutlineData>();
		Vector<LabelSettings::StackedShadowData> stacked_shadow_datas =
			has_settings ? settings->get_stacked_shadow_data()
						 : Vector<LabelSettings::StackedShadowData>();
		int max_outline_draw_size = outline_size;
		if (stacked_outline_datas.size() != 0) {
			int draw_iterations = stacked_outline_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				int stacked_outline_size = stacked_outline_datas[j].size;
				if (stacked_outline_size <= 0) {
					continue;
				}
				max_outline_draw_size += stacked_outline_size;
			}
		}
		if (stacked_shadow_datas.size() != 0) {
			int draw_iterations = stacked_shadow_datas.size();
			for (int j = 0; j < draw_iterations; j++) {
				LabelSettings::StackedShadowData stacked_shadow_data = stacked_shadow_datas[j];
				if (stacked_shadow_data.outline_size > 0) {
					max_outline_draw_size =
						MAX(max_outline_draw_size, stacked_shadow_data.outline_size);
				}
			}
		}
		float scale = (float)font_size / (float)ff->get_msdf_size();
		float ol = (float)max_outline_draw_size / scale / 4.0;
		float pxr = (float)ff->get_msdf_pixel_range() / 2.0 - 1.0;
		float r_pxr = (ol + 1.0) * 2.0;

		if (ol > pxr) {
			warnings.push_back(vformat(
				RTR("MSDF font pixel range is too small, some outlines/shadows will not render. "
					"Set MSDF pixel range to be at least %d to render all outlines/shadows."),
				Math::ceil(r_pxr)));
		}
	}

	return warnings;
}

Rect2 Label::get_character_bounds(int p_pos) const
{
	_ensure_shaped();

	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);

	Vector2 ofs;
	int line_spacing;
	int last_line;
	get_layout_data(ofs, last_line, line_spacing);

	int line_index = 0;
	for (int p = 0; p < paragraphs.size(); p++) {
		const Paragraph& para = paragraphs[p];
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines_rid.size() < last_line) ? para.lines_rid.size()
																	   : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				RID line_rid = para.lines_rid[i];
				Rect2 line_rect = _get_line_rect(p, i);
				ofs.x = line_rect.position.x;

				int v_size = TS->shaped_text_get_glyph_count(line_rid);
				const Glyph* glyphs = TS->shaped_text_get_glyphs(line_rid);

				float gl_off = 0.0f;
				for (int j = 0; j < v_size; j++) {
					if ((glyphs[j].count > 0) &&
						((glyphs[j].index != 0) ||
							((glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) ==
								TextServer::GRAPHEME_IS_SPACE))) {
						if (p_pos >= glyphs[j].start + para.start &&
							p_pos < glyphs[j].end + para.start) {
							float advance = 0.f;
							for (int k = 0; k < glyphs[j].count; k++) {
								advance += glyphs[j + k].advance;
							}
							Rect2 rect;
							rect.position = ofs + Vector2(gl_off, 0);
							rect.size = Vector2(advance, line_rect.size.y);
							return rect;
						}
					}
					gl_off += glyphs[j].advance * glyphs[j].repeat;
				}
				double asc = TS->shaped_text_get_ascent(line_rid);
				double dsc = TS->shaped_text_get_descent(line_rid);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				ofs.y += asc + dsc + line_spacing;
			}
			ofs.y += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}
	return Rect2();
}

Size2 Label::get_minimum_size() const
{
	_ensure_shaped();

	Size2 min_size = minsize;

	const Ref<Font>& font = (settings.is_valid() && settings->get_font().is_valid())
								? settings->get_font()
								: theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;

	min_size.height = MAX(min_size.height, font->get_height(font_size));

	Size2 min_style = theme_cache.normal_style->get_minimum_size();
	if (autowrap_mode != TextServer::AUTOWRAP_OFF) {
		if (!clip && overrun_behavior != TextServer::OVERRUN_NO_TRIMMING && max_lines_visible > 0) {
			int line_spacing =
				settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
			min_size.height = MIN(
				min_size.height, (font->get_height(font_size) + line_spacing) * max_lines_visible);

		}
		else if (clip || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			min_size.height = 1;
		}
		return Size2(1, min_size.height) + min_style;
	}
	else {
		if (clip || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			min_size.width = 1;
		}
		return min_size + min_style;
	}
}

Size2 Label::get_desired_size() const
{
	Size2 combined_max = get_combined_maximum_size();
	if (combined_max.width < 0) {
		return Size2();
	}

	_ensure_shaped();
	Size2 min_style = theme_cache.normal_style->get_minimum_size();
	Size2 content_size = minsize + min_style;
	content_size.width = MIN(content_size.width, int(combined_max.width));

	return content_size;
}

int Label::get_line_count() const
{
	if (!is_inside_tree()) {
		return 1;
	}
	_ensure_shaped();

	return total_line_count;
}

int Label::get_visible_line_count() const
{
	Ref<StyleBox> style = theme_cache.normal_style;
	Ref<Font> font = (settings.is_valid() && settings->get_font().is_valid()) ? settings->get_font()
																			  : theme_cache.font;
	int font_size = settings.is_valid() ? settings->get_font_size() : theme_cache.font_size;
	int font_h = font->get_height(font_size);
	int line_spacing =
		settings.is_valid() ? settings->get_line_spacing() : theme_cache.line_spacing;
	int paragraph_spacing =
		settings.is_valid() ? settings->get_paragraph_spacing() : theme_cache.paragraph_spacing;
	int lines_visible = 0;
	float total_h = 0.0;

	int line_index = 0;
	for (const Paragraph& para : paragraphs) {
		if (line_index + para.lines_rid.size() <= lines_skipped) {
			line_index += para.lines_rid.size();
		}
		else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines_rid.size(); i++) {
				double asc = TS->shaped_text_get_ascent(para.lines_rid[i]);
				double dsc = TS->shaped_text_get_descent(para.lines_rid[i]);
				if (asc + dsc < font_h) {
					double diff = font_h - (asc + dsc);
					asc += diff / 2;
					dsc += diff - (diff / 2);
				}
				total_h += asc + dsc + line_spacing;
				if (total_h > Math::ceil(get_size().height - style->get_minimum_size().height +
										 line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines_rid.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	return lines_visible;
}

HorizontalAlignment Label::get_horizontal_alignment() const { return horizontal_alignment; }

VerticalAlignment Label::get_vertical_alignment() const { return vertical_alignment; }

Ref<LabelSettings> Label::get_label_settings() const { return settings; }

TextServer::StructuredTextParser Label::get_structured_text_bidi_override() const
{
	return st_parser;
}

Control::TextDirection Label::get_text_direction() const { return text_direction; }

String Label::get_language() const { return language; }

String Label::get_paragraph_separator() const { return paragraph_separator; }

bool Label::is_clipping_text() const { return clip; }

PackedFloat32Array Label::get_tab_stops() const { return tab_stops; }

void Label::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior) {}

TextServer::OverrunBehavior Label::get_text_overrun_behavior() const { return overrun_behavior; }

String Label::get_ellipsis_char() const { return el_char; }

String Label::get_text() const { return text; }

void Label::set_text(const String& p_string) { text = p_string; }

int Label::get_visible_characters() const { return visible_chars; }

float Label::get_visible_ratio() const { return visible_ratio; }

TextServer::VisibleCharactersBehavior Label::get_visible_characters_behavior() const
{
	return visible_chars_behavior;
}

int Label::get_lines_skipped() const { return lines_skipped; }

int Label::get_max_lines_visible() const { return max_lines_visible; }

int Label::get_total_character_count() const { return xl_text.length(); }

Label::~Label()
{
	for (Paragraph& para : paragraphs) {
		for (const RID& line_rid : para.lines_rid) {
			TS->free_rid(line_rid);
		}
		para.lines_rid.clear();
		TS->free_rid(para.text_rid);
	}
	paragraphs.clear();
}

void Label::set_horizontal_alignment(HorizontalAlignment p_alignment) {}

void Label::set_autowrap_mode(TextServer::AutowrapMode) {}

void Label::_shape() const {}

void Label::set_vertical_alignment(VerticalAlignment) {}

// --- line_edit.cpp ---

void LineEdit::edit(bool p_hide_focus) { _edit(true, p_hide_focus); }

void LineEdit::unedit()
{
	if (!editing) {
		return;
	}

	editing = false;
	_validate_caret_can_draw();

	apply_ime();
	set_caret_column(caret_column); // Update scroll_offset.

	if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		DisplayServer::virtual_keyboard_hide();
	}

	if (deselect_on_focus_loss_enabled && !selection.drag_attempt) {
		deselect();
	}
}

bool LineEdit::is_editing() const { return editing; }

void LineEdit::set_keep_editing_on_text_submit(bool p_enabled)
{
	keep_editing_on_text_submit = p_enabled;
}

bool LineEdit::is_editing_kept_on_text_submit() const { return keep_editing_on_text_submit; }

void LineEdit::_close_ime_window()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::window_set_ime_position(Point2(), wid);
	DisplayServer::window_set_ime_active(false, wid);
}

void LineEdit::_update_ime_window_position()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::window_set_ime_active(true, wid);
	Point2 pos = Point2(get_caret_pixel_pos().x,
					 (get_size().y + theme_cache.font->get_height(theme_cache.font_size)) / 2) +
				 get_global_position();
	if (get_window()->get_embedder()) {
		pos += get_viewport()->get_popup_base_transform().get_origin();
	}
	// Take into account the window's transform.
	pos = get_window()->get_screen_transform().xform(pos);
	// The window will move to the updated position the next time the IME is updated, not
	// immediately.
	DisplayServer::window_set_ime_position(pos, wid);
}

bool LineEdit::has_ime_text() const { return !ime_text.is_empty(); }

void LineEdit::cancel_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}
	ime_text = String();
	ime_selection = Vector2i();
	alt_start = false;
	alt_start_no_hold = false;
	_close_ime_window();
	_shape();
}

void LineEdit::apply_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}

	// Force apply the current IME text.
	if (alt_start || alt_start_no_hold) {
		cancel_ime();
		if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff && alt_code <= 0x10ffff)) {
			char32_t ucodestr[2] = {(char32_t)alt_code, 0};
			insert_text_at_caret(ucodestr);
		}
	}
	else {
		String insert_ime_text = ime_text;
		cancel_ime();
		insert_text_at_caret(insert_ime_text);
	}
}

void LineEdit::_swap_current_input_direction()
{
	if (input_direction == TEXT_DIRECTION_LTR) {
		input_direction = TEXT_DIRECTION_RTL;
	}
	else {
		input_direction = TEXT_DIRECTION_LTR;
	}
	set_caret_column(get_caret_column());
}

void LineEdit::_move_caret_left(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.begin);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Move to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() - 1);
		}
		else {
			set_caret_column(TS->shaped_text_prev_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_right(bool p_select, bool p_move_by_word)
{
	if (selection.enabled && !p_select) {
		set_caret_column(selection.end);
		deselect();
		return;
	}

	shift_selection_check_pre(p_select);

	if (p_move_by_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Move to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		set_caret_column(cc);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(get_caret_column() + 1);
		}
		else {
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, get_caret_column()));
		}
	}

	shift_selection_check_post(p_select);
	_reset_caret_blink_timer();
}

void LineEdit::_move_caret_start(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(0);
	shift_selection_check_post(p_select);
}

void LineEdit::_move_caret_end(bool p_select)
{
	shift_selection_check_pre(p_select);
	set_caret_column(text.length());
	shift_selection_check_post(p_select);
}

void LineEdit::_backspace(bool p_word, bool p_all_to_left)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == 0) {
		return; // Nothing to do.
	}

	if (p_all_to_left) {
		text = text.substr(caret_column);
		_shape();
		set_caret_column(0);
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;

		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc <= words[0]) {
			// Delete to the start when there are no more words.
			cc = 0;
		}
		else {
			for (int i = words.size() - 2; i >= 0; i = i - 2) {
				if (words[i] < cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(cc, caret_column);

		set_caret_column(cc);
	}
	else {
		delete_char();
	}
}

void LineEdit::_delete(bool p_word, bool p_all_to_right)
{
	if (!editable) {
		return;
	}

	if (selection.enabled) {
		selection_delete();
		return;
	}

	if (caret_column == text.length()) {
		return; // Nothing to do.
	}

	if (p_all_to_right) {
		text = text.substr(0, caret_column);
		_shape();
		_text_changed();
		return;
	}

	if (p_word) {
		int cc = caret_column;
		PackedInt32Array words = TS->shaped_text_get_word_breaks(text_rid);
		if (words.is_empty() || cc >= words[words.size() - 1]) {
			// Delete to the end when there are no more words.
			cc = text.length();
		}
		else {
			for (int i = 1; i < words.size(); i = i + 2) {
				if (words[i] > cc) {
					cc = words[i];
					break;
				}
			}
		}

		delete_text(caret_column, cc);
		set_caret_column(caret_column);
	}
	else {
		if (caret_mid_grapheme_enabled) {
			set_caret_column(caret_column + 1);
			delete_char();
		}
		else {
			int cc = caret_column;
			set_caret_column(TS->shaped_text_next_character_pos(text_rid, caret_column));
			delete_text(cc, caret_column);
		}
	}
}

Point2 LineEdit::_get_right_icon_size(Ref<Texture2D> p_right_icon) const
{
	Size2 icon_size;

	if (p_right_icon.is_null()) {
		return icon_size;
	}

	switch (icon_expand_mode) {
	default:
	case LineEdit::EXPAND_MODE_ORIGINAL_SIZE:
		icon_size = p_right_icon->get_size();
		break;
	case LineEdit::EXPAND_MODE_FIT_TO_TEXT: {
		real_t height = theme_cache.font->get_height(theme_cache.font_size);
		icon_size = Size2(height, height);
	} break;
	case LineEdit::EXPAND_MODE_FIT_TO_LINE_EDIT: {
		icon_size = p_right_icon->get_size();
		Point2 size = get_size();
		float icon_width = icon_size.width * size.height / icon_size.height;
		float icon_height = size.height;

		if (icon_width > size.width) {
			icon_width = size.width;
			icon_height = icon_size.height * icon_width / icon_size.width;
		}

		icon_size = Size2(icon_width, icon_height) * right_icon_scale;
	} break;
	}

	return icon_size;
}

void LineEdit::unhandled_key_input(const InputEvent& p_event)
{
	// Return to prevent editing if just focused.
	if (!editing) {
		return;
	}

	if (p_event.type == InputEventType::KEY) {
		if (!p_event.is_pressed()) {
			return;
		}
		// Handle Unicode (with modifiers active, process after shortcuts).
		if (has_focus() && editable && (p_event.key.unicode >= 32)) {
			selection_delete();
			char32_t ucodestr[2] = {p_event.key.unicode, 0};
			int prev_len = text.length();
			insert_text_at_caret(ucodestr);
			if (text.length() != prev_len) {
				_text_changed();
			}
			accept_event();
		}
	}
}

HorizontalAlignment LineEdit::get_horizontal_alignment() const { return alignment; }

Control::CursorShape LineEdit::get_cursor_shape(const Point2& p_pos) const
{
	if ((!text.is_empty() && is_editable() && _is_over_clear_button(p_pos)) ||
		(!is_editable() && (!is_selecting_enabled() || text.is_empty()))) {
		return CURSOR_ARROW;
	}
	return Control::get_cursor_shape(p_pos);
}

bool LineEdit::_is_over_clear_button(const Point2& p_pos) const
{
	if (!clear_button_enabled || !has_point(p_pos)) {
		return false;
	}
	Ref<Texture2D> icon = theme_cache.clear_icon;
	return is_layout_rtl() ? p_pos.x < theme_cache.normal->get_margin(SIDE_LEFT) + icon->get_width()
						   : p_pos.x > get_size().width - icon->get_width() -
										   theme_cache.normal->get_margin(SIDE_RIGHT);
}

void LineEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void LineEdit::copy_text()
{
	if (selection.enabled && !pass) {
		DisplayServer::clipboard_set(get_selected_text());
	}
}

void LineEdit::cut_text()
{
	if (editable && selection.enabled && !pass) {
		DisplayServer::clipboard_set(get_selected_text());
		selection_delete();
	}
}

bool LineEdit::has_undo() const
{
	if (undo_stack_pos == nullptr) {
		return undo_stack.size() > 1;
	}
	return undo_stack_pos != undo_stack.front();
}

bool LineEdit::has_redo() const
{
	return undo_stack_pos != nullptr && undo_stack_pos != undo_stack.back();
}

void LineEdit::deselect() {}

void LineEdit::undo()
{
	if (!editable) {
		return;
	}

	if (!has_undo()) {
		return;
	}

	if (undo_stack_pos == nullptr) {
		undo_stack_pos = undo_stack.back();
	}

	deselect();

	undo_stack_pos = undo_stack_pos->prev();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::redo()
{
	if (!editable) {
		return;
	}

	if (!has_redo()) {
		return;
	}

	deselect();

	undo_stack_pos = undo_stack_pos->next();
	TextOperation op = undo_stack_pos->get();
	text = op.text;
	scroll_offset = op.scroll_offset;

	_shape();
	set_caret_column(op.caret_column);

	_emit_text_change();
}

void LineEdit::shift_selection_check_pre(bool p_shift)
{
	if (!selection.enabled && p_shift) {
		selection.start_column = caret_column;
	}
	if (!p_shift) {
		deselect();
	}
}

void LineEdit::shift_selection_check_post(bool p_shift)
{
	if (p_shift) {
		selection_fill_at_caret();
	}
}

void LineEdit::set_caret_at_pixel_pos(int p_x)
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	int ofs = std::ceil(TS->shaped_text_hit_test_position(text_rid, p_x - x_ofs - scroll_offset));
	if (ofs == -1) {
		return;
	}
	if (!caret_mid_grapheme_enabled) {
		ofs = TS->shaped_text_closest_character_pos(text_rid, ofs);
	}
	set_caret_column(ofs);
}

Vector2 LineEdit::get_caret_pixel_pos()
{
	Ref<StyleBox> style = theme_cache.normal;
	bool rtl = is_layout_rtl();

	int x_ofs = 0;
	float text_width = TS->shaped_text_get_size(text_rid).x;
	switch (alignment) {
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_LEFT: {
		if (rtl) {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
		else {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		if (!Math::is_zero_approx(scroll_offset)) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
			int centered = int((get_size().width - total_margin - text_width)) / 2;
			x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, centered);
		}
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		if (rtl) {
			x_ofs = style->get_margin(SIDE_LEFT);
		}
		else {
			x_ofs = MAX(style->get_margin(SIDE_LEFT),
				int(get_size().width - style->get_margin(SIDE_RIGHT) - (text_width)));
		}
	} break;
	}

	bool using_placeholder = text.is_empty() && ime_text.is_empty();
	bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
	if (right_icon.is_valid() || display_clear_icon) {
		Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
		Point2 right_icon_size = _get_right_icon_size(r_icon);
		if (alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			if (Math::is_zero_approx(scroll_offset)) {
				int total_margin = style->get_margin(SIDE_LEFT) + style->get_margin(SIDE_RIGHT);
				int center =
					int(get_size().width - total_margin - text_width - right_icon_size.width) / 2;
				x_ofs = style->get_margin(SIDE_LEFT) + MAX(0, center);
			}
			if (rtl) {
				x_ofs += right_icon_size.width;
			}
		}
		else {
			if (rtl) {
				x_ofs = MAX(style->get_margin(SIDE_LEFT) + right_icon_size.width, x_ofs);
			}
			else {
				x_ofs = MAX(style->get_margin(SIDE_LEFT),
					x_ofs - right_icon_size.width - style->get_margin(SIDE_RIGHT));
			}
		}
	}

	Vector2 ret;
	CaretInfo caret;
	// Get position of the start of caret.
	if (!ime_text.is_empty() && ime_selection.x != 0) {
		caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_selection.x);
	}
	else {
		caret = TS->shaped_text_get_carets(text_rid, caret_column);
	}

	if ((caret.l_caret != Rect2() && (caret.l_dir == TextServer::DIRECTION_AUTO ||
										 caret.l_dir == (TextServer::Direction)input_direction)) ||
		(caret.t_caret == Rect2())) {
		ret.x = x_ofs + caret.l_caret.position.x + scroll_offset;
	}
	else {
		ret.x = x_ofs + caret.t_caret.position.x + scroll_offset;
	}

	// Get position of the end of caret.
	if (!ime_text.is_empty()) {
		if (ime_selection.y != 0) {
			caret = TS->shaped_text_get_carets(
				text_rid, caret_column + ime_selection.x + ime_selection.y);
		}
		else {
			caret = TS->shaped_text_get_carets(text_rid, caret_column + ime_text.size());
		}
		if ((caret.l_caret != Rect2() &&
				(caret.l_dir == TextServer::DIRECTION_AUTO ||
					caret.l_dir == (TextServer::Direction)input_direction)) ||
			(caret.t_caret == Rect2())) {
			ret.y = x_ofs + caret.l_caret.position.x + scroll_offset;
		}
		else {
			ret.y = x_ofs + caret.t_caret.position.x + scroll_offset;
		}
	}
	else {
		ret.y = ret.x;
	}

	return ret;
}

void LineEdit::set_caret_mid_grapheme_enabled(const bool p_enabled)
{
	caret_mid_grapheme_enabled = p_enabled;
}

bool LineEdit::is_caret_mid_grapheme_enabled() const { return caret_mid_grapheme_enabled; }

bool LineEdit::is_caret_blink_enabled() const { return caret_blink_enabled; }

bool LineEdit::is_caret_force_displayed() const { return caret_force_displayed; }

float LineEdit::get_caret_blink_interval() const { return caret_blink_interval; }

void LineEdit::set_caret_blink_interval(const float p_interval)
{
	ERR_FAIL_COND(p_interval <= 0);
	caret_blink_interval = p_interval;
}

void LineEdit::_validate_caret_can_draw()
{
	if (caret_blink_enabled) {
		draw_caret = true;
		caret_blink_timer = 0.0;
	}
	caret_can_draw = (caret_force_displayed && !is_part_of_edited_scene()) ||
					 (editing && (window_has_focus || (menu && menu->has_focus())) && has_focus());
}

void LineEdit::delete_char()
{
	if (text.is_empty() || caret_column == 0) {
		return;
	}
	int delete_char_offset = 1;
	if (!caret_mid_grapheme_enabled && backspace_deletes_composite_character_enabled) {
		delete_char_offset = caret_column - get_previous_composite_character_column(caret_column);
	}
	text = text.left(caret_column - delete_char_offset) + text.substr(caret_column);
	_shape();

	set_caret_column(get_caret_column() - delete_char_offset);

	_text_changed();
}

void LineEdit::set_text(String p_text) { _set_text(p_text); }

Control::TextDirection LineEdit::get_text_direction() const { return text_direction; }

String LineEdit::get_language() const { return language; }

bool LineEdit::get_draw_control_chars() const { return draw_control_chars; }

TextServer::StructuredTextParser LineEdit::get_structured_text_bidi_override() const
{
	return st_parser;
}

void LineEdit::clear()
{
	bool was_empty = text.is_empty();
	clear_internal();
	_clear_redo();
	if (!was_empty) {
		_emit_text_change();
	}

	// This should reset virtual keyboard state if needed.
	if (editing) {
		show_virtual_keyboard();
	}
}

void LineEdit::show_virtual_keyboard()
{
	_update_ime_window_position();

	if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD) &&
		virtual_keyboard_enabled) {
		if (selection.enabled) {
			DisplayServer::virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				selection.begin, selection.end);
		}
		else {
			DisplayServer::virtual_keyboard_show(text, get_global_rect(),
				DisplayServerEnums::VirtualKeyboardType(virtual_keyboard_type), max_length,
				caret_column);
		}
	}
}

String LineEdit::get_text() const { return text; }

String LineEdit::get_placeholder() const { return placeholder; }

int LineEdit::get_caret_column() const { return caret_column; }

int LineEdit::get_next_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == text.length()) {
		return p_column;
	}
	else {
		return TS->shaped_text_next_character_pos(text_rid, p_column);
	}
}

int LineEdit::get_previous_composite_character_column(int p_column) const
{
	ERR_FAIL_INDEX_V(p_column, text.length() + 1, -1);
	if (p_column == 0) {
		return 0;
	}
	else {
		return TS->shaped_text_prev_character_pos(text_rid, p_column);
	}
}

void LineEdit::set_scroll_offset(float p_pos)
{
	scroll_offset = p_pos;
	if (scroll_offset < 0.0) {
		scroll_offset = 0.0;
	}
}

float LineEdit::get_scroll_offset() const { return scroll_offset; }

Size2 LineEdit::get_minimum_size() const
{
	Ref<Font> font = theme_cache.font;
	int font_size = theme_cache.font_size;

	Size2 min_size;

	// Minimum size of text.
	// W is wider than M in most fonts, Using M may result in hiding the last digit when using float
	// values in SpinBox, ie. ColorPicker RAW values.
	float em_space_size = font->get_char_size('W', font_size).x;
	min_size.width = theme_cache.minimum_character_width * em_space_size;

	if (expand_to_text_length) {
		// Ensure some space for the caret when placed at the end.
		min_size.width = MAX(min_size.width, full_width + theme_cache.caret_width);
	}

	min_size.height = MAX(TS->shaped_text_get_size(text_rid).y, font->get_height(font_size));

	// Take icons into account.
	int icon_max_width = 0;
	if (right_icon.is_valid()) {
		Point2 right_icon_size = _get_right_icon_size(right_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = right_icon_size.width;
	}
	if (clear_button_enabled) {
		Point2 right_icon_size = _get_right_icon_size(theme_cache.clear_icon);
		min_size.height = MAX(min_size.height, right_icon_size.height);
		icon_max_width = MAX(icon_max_width, right_icon_size.width);
	}
	min_size.width += icon_max_width;

	Size2 style_min_size =
		theme_cache.normal->get_minimum_size().max(theme_cache.read_only->get_minimum_size());
	return style_min_size + min_size;
}

bool LineEdit::has_selection() const { return selection.enabled; }

String LineEdit::get_selected_text()
{
	if (selection.enabled) {
		return text.substr(selection.begin, selection.end - selection.begin);
	}
	else {
		return String();
	}
}

int LineEdit::get_selection_from_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.begin;
}

int LineEdit::get_selection_to_column() const
{
	ERR_FAIL_COND_V(!selection.enabled, -1);
	return selection.end;
}

void LineEdit::selection_delete()
{
	if (selection.enabled) {
		delete_text(selection.begin, selection.end);
	}

	deselect();
}

void LineEdit::set_max_length(int p_max_length)
{
	ERR_FAIL_COND(p_max_length < 0);
	max_length = p_max_length;
	set_text(text);
}

int LineEdit::get_max_length() const { return max_length; }

void LineEdit::selection_fill_at_caret()
{
	if (!selecting_enabled) {
		return;
	}

	selection.begin = caret_column;
	selection.end = selection.start_column;

	if (selection.end < selection.begin) {
		int aux = selection.end;
		selection.end = selection.begin;
		selection.begin = aux;
	}

	selection.enabled = (selection.begin != selection.end);
	queue_accessibility_update();
}

bool LineEdit::is_editable() const { return editable; }

bool LineEdit::is_secret() const { return pass; }

String LineEdit::get_secret_character() const { return secret_character; }

bool LineEdit::is_text_field() const { return true; }

void LineEdit::set_context_menu_enabled(bool p_enable) { context_menu_enabled = p_enable; }

bool LineEdit::is_context_menu_enabled() { return context_menu_enabled; }

void LineEdit::show_emoji_and_symbol_picker()
{
	_update_ime_window_position();
	DisplayServer::show_emoji_and_symbol_picker();
}

void LineEdit::set_emoji_menu_enabled(bool p_enabled)
{
	if (emoji_menu_enabled != p_enabled) {
		emoji_menu_enabled = p_enabled;
	}
}

bool LineEdit::is_emoji_menu_enabled() const { return emoji_menu_enabled; }

void LineEdit::set_backspace_deletes_composite_character_enabled(bool p_enabled)
{
	backspace_deletes_composite_character_enabled = p_enabled;
}

bool LineEdit::is_backspace_deletes_composite_character_enabled() const
{
	return backspace_deletes_composite_character_enabled;
}

bool LineEdit::is_menu_visible() const { return menu && menu->is_visible(); }

PopupMenu* LineEdit::get_menu() const
{
	if (!menu) {
		const_cast<LineEdit*>(this)->_generate_context_menu();
	}
	return menu;
}

bool LineEdit::is_expand_to_text_length_enabled() const { return expand_to_text_length; }

bool LineEdit::is_clear_button_enabled() const { return clear_button_enabled; }

void LineEdit::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool LineEdit::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

void LineEdit::set_virtual_keyboard_enabled(bool p_enable) { virtual_keyboard_enabled = p_enable; }

bool LineEdit::is_virtual_keyboard_enabled() const { return virtual_keyboard_enabled; }

void LineEdit::set_virtual_keyboard_show_on_focus(bool p_show_on_focus)
{
	virtual_keyboard_show_on_focus = p_show_on_focus;
}

bool LineEdit::get_virtual_keyboard_show_on_focus() const { return virtual_keyboard_show_on_focus; }

void LineEdit::set_virtual_keyboard_type(VirtualKeyboardType p_type)
{
	virtual_keyboard_type = p_type;
}

LineEdit::VirtualKeyboardType LineEdit::get_virtual_keyboard_type() const
{
	return virtual_keyboard_type;
}

void LineEdit::set_middle_mouse_paste_enabled(bool p_enabled)
{
	middle_mouse_paste_enabled = p_enabled;
}

bool LineEdit::is_middle_mouse_paste_enabled() const { return middle_mouse_paste_enabled; }

void LineEdit::set_selecting_enabled(bool p_enabled)
{
	if (selecting_enabled == p_enabled) {
		return;
	}

	selecting_enabled = p_enabled;

	if (!selecting_enabled) {
		deselect();
	}
}

bool LineEdit::is_selecting_enabled() const { return selecting_enabled; }

void LineEdit::set_deselect_on_focus_loss_enabled(const bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && selection.enabled && !has_focus()) {
		deselect();
	}
}

bool LineEdit::is_deselect_on_focus_loss_enabled() const { return deselect_on_focus_loss_enabled; }

void LineEdit::set_drag_and_drop_selection_enabled(const bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool LineEdit::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

Ref<Texture2D> LineEdit::get_right_icon() { return right_icon; }

LineEdit::ExpandMode LineEdit::get_icon_expand_mode() const { return icon_expand_mode; }

float LineEdit::get_right_icon_scale() const { return right_icon_scale; }

bool LineEdit::is_flat() const { return flat; }

void LineEdit::set_select_all_on_focus(bool p_enabled) { select_all_on_focus = p_enabled; }

bool LineEdit::is_select_all_on_focus() const { return select_all_on_focus; }

void LineEdit::clear_pending_select_all_on_focus() { pending_select_all_on_focus = false; }

void LineEdit::_text_changed()
{
	_emit_text_change();
	_clear_redo();
}

PackedStringArray LineEdit::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (secret_character.length() > 1) {
		warnings.push_back("Secret Character property supports only one character. Extra "
						   "characters will be ignored.");
	}
	return warnings;
}

void LineEdit::_fit_to_width()
{
	if (alignment == HORIZONTAL_ALIGNMENT_FILL) {
		Ref<StyleBox> style = theme_cache.normal;
		int t_width =
			get_size().width - style->get_margin(SIDE_RIGHT) - style->get_margin(SIDE_LEFT);
		bool using_placeholder = text.is_empty() && ime_text.is_empty();
		bool display_clear_icon = !using_placeholder && is_editable() && clear_button_enabled;
		if (right_icon.is_valid() || display_clear_icon) {
			Ref<Texture2D> r_icon = display_clear_icon ? theme_cache.clear_icon : right_icon;
			Point2 right_icon_size = _get_right_icon_size(r_icon);
			t_width -= right_icon_size.width;
		}
		TS->shaped_text_fit_to_width(text_rid, MAX(t_width, full_width));
	}
}

void LineEdit::_clear_redo()
{
	_create_undo_state();
	if (undo_stack_pos == nullptr) {
		return;
	}

	undo_stack_pos = undo_stack_pos->next();
	while (undo_stack_pos) {
		List<TextOperation>::Element* elem = undo_stack_pos;
		undo_stack_pos = undo_stack_pos->next();
		undo_stack.erase(elem);
	}
	_create_undo_state();
}

void LineEdit::_clear_undo_stack()
{
	undo_stack.clear();
	undo_stack_pos = nullptr;
	_create_undo_state();
}

void LineEdit::_create_undo_state()
{
	TextOperation op;
	op.text = text;
	op.caret_column = caret_column;
	op.scroll_offset = scroll_offset;
	undo_stack.push_back(op);
}

Key LineEdit::_get_menu_action_accelerator(const String& p_action)
{
	const List<InputEvent>* events = InputMap::action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<InputEvent>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const InputEvent& event = first_event->get();
	if (event.type != InputEventType::KEY) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero.
	if (event.key.physical_keycode != Key::NONE) {
		return event.get_physical_keycode_with_modifiers();
	}
	else {
		return event.get_keycode_with_modifiers();
	}
}

void LineEdit::_update_context_menu()
{
	if (!menu) {
		_generate_context_menu();
	}

	int idx = -1;

#define MENU_ITEM_ACTION_DISABLED(m_menu, m_id, m_action, m_disabled)                              \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_ACTION(m_menu, m_id, m_action)                                                   \
	idx = m_menu->get_item_index(m_id);                                                            \
                                                                                                   \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_accelerator(                                                              \
			idx, shortcut_keys_enabled ? _get_menu_action_accelerator(m_action) : Key::NONE);      \
	}

#define MENU_ITEM_DISABLED(m_menu, m_id, m_disabled)                                               \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_disabled(idx, m_disabled);                                                \
	}

#define MENU_ITEM_CHECKED(m_menu, m_id, m_checked)                                                 \
	idx = m_menu->get_item_index(m_id);                                                            \
	if (idx >= 0) {                                                                                \
		m_menu->set_item_checked(idx, m_checked);                                                  \
	}

	if (DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		MENU_ITEM_DISABLED(menu, MENU_EMOJI_AND_SYMBOL, !editable || !emoji_menu_enabled)
	}

#undef MENU_ITEM_ACTION_DISABLED
#undef MENU_ITEM_ACTION
#undef MENU_ITEM_DISABLED
#undef MENU_ITEM_CHECKED
}

void LineEdit::set_caret_column(int p_column) {}

LineEdit::LineEdit(const String& p_placeholder)
{
	text_rid = TS->create_shaped_text();
	_create_undo_state();
	deselect();
	set_focus_mode(FOCUS_ALL);
	set_default_cursor_shape(CURSOR_IBEAM);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_process_unhandled_key_input(true);
	set_caret_blink_enabled(false);
	set_placeholder(p_placeholder);
	set_editable(true); // Initialize to opposite first, so we get past the early-out in set_editable.
}

LineEdit::~LineEdit() { TS->free_rid(text_rid); }

void LineEdit::select_all() {}

void LineEdit::_emit_text_change() {}

void LineEdit::select(int p_from, int p_to) {}

void LineEdit::delete_text(int p_from_column, int p_to_column) {}

void LineEdit::_shape() {}


void LineEdit::set_right_icon(Ref<Texture2D> const&) {}

void LineEdit::set_placeholder(String) {}

void LineEdit::insert_text_at_caret(String) {}

void LineEdit::_set_text(String, bool) {}

void LineEdit::set_editable(bool) {}

void LineEdit::set_caret_blink_enabled(bool) {}

void LineEdit::_reset_caret_blink_timer() {}

void LineEdit::_generate_context_menu() {}

void LineEdit::_edit(bool, bool) {}

void LineEdit::set_secret(bool) {}

void LineEdit::set_horizontal_alignment(HorizontalAlignment) {}

void LineEdit::set_clear_button_enabled(bool) {}

void LineEdit::clear_internal() {}

String LineEdit::_get_accessibility_name() const { return String(); }

// --- line_edit.compat.inc ---

#ifndef DISABLE_DEPRECATED


void LineEdit::_edit_bind_compat_111117() { edit(false); }

void LineEdit::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- item_list.cpp ---

String ItemList::get_item_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].text;
}

Control::TextDirection ItemList::get_item_text_direction(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), TEXT_DIRECTION_INHERITED);
	return items[p_idx].text_direction;
}

String ItemList::get_item_language(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].language;
}

Node::AutoTranslateMode ItemList::get_item_auto_translate_mode(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), AUTO_TRANSLATE_MODE_INHERIT);
	return items[p_idx].auto_translate_mode;
}

void ItemList::set_item_tooltip_enabled(int p_idx, const bool p_enabled)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].tooltip_enabled != p_enabled) {
		items.write[p_idx].tooltip_enabled = p_enabled;
		items.write[p_idx].accessibility_item_dirty = true;
		queue_accessibility_update();
	}
}

bool ItemList::is_item_tooltip_enabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].tooltip_enabled;
}

String ItemList::get_item_tooltip(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].tooltip;
}

Ref<Texture2D> ItemList::get_item_icon(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());

	return items[p_idx].icon;
}

bool ItemList::is_item_icon_transposed(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].icon_transposed;
}

Rect2 ItemList::get_item_icon_region(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	return items[p_idx].icon_region;
}

Color ItemList::get_item_icon_modulate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].icon_modulate;
}

Color ItemList::get_item_custom_bg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_bg;
}

Color ItemList::get_item_custom_fg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_fg;
}

Rect2 ItemList::get_item_rect(int p_idx, bool p_expand) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	Rect2 ret = items[p_idx].rect_cache;
	if (p_expand && p_idx % current_columns == current_columns - 1) {
		int width = get_size().width - theme_cache.panel_style->get_minimum_size().width;
		if (scroll_bar_v->is_visible()) {
			width -=
				scroll_bar_v->get_bound_minimum_size().width + theme_cache.scroll_bar_h_separation;
		}
		ret.size.width = width - ret.position.x;
	}
	ret.position += theme_cache.panel_style->get_offset();
	return ret;
}

void ItemList::set_item_selectable(int p_idx, bool p_selectable)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	items.write[p_idx].selectable = p_selectable;
	items.write[p_idx].accessibility_item_dirty = true;
	queue_accessibility_update();
}

bool ItemList::is_item_selectable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].selectable;
}

bool ItemList::is_item_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

bool ItemList::is_selected(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].selected;
}

int ItemList::get_current() const { return current; }

int ItemList::get_item_count() const { return items.size(); }

int ItemList::get_fixed_column_width() const { return fixed_column_width; }

bool ItemList::is_same_column_width() const { return same_column_width; }

int ItemList::get_max_text_lines() const { return max_text_lines; }

int ItemList::get_max_columns() const { return max_columns; }

ItemList::SelectMode ItemList::get_select_mode() const { return select_mode; }

ItemList::IconMode ItemList::get_icon_mode() const { return icon_mode; }

Size2i ItemList::get_fixed_icon_size() const { return fixed_icon_size; }

Size2 ItemList::Item::get_icon_size() const
{
	if (icon.is_null()) {
		return Size2();
	}

	Size2 size_result = Size2(icon_region.size).abs();
	if (icon_region.size.x == 0 || icon_region.size.y == 0) {
		size_result = icon->get_size();
	}

	if (icon_transposed) {
		Size2 size_tmp = size_result;
		size_result.x = size_tmp.y;
		size_result.y = size_tmp.x;
	}

	return size_result;
}

void ItemList::center_on_current(bool p_center_verically, bool p_center_horizontally)
{
	if (current < 0 || current >= items.size()) {
		return;
	}

	ERR_FAIL_COND_MSG(!p_center_verically && !p_center_horizontally,
		"At least one of the parameters must be true.");

	Rect2 r = items[current].rect_cache;

	if (p_center_verically) {
		int from_v = scroll_bar_v->get_value();
		int to_v = from_v + scroll_bar_v->get_page();
		int item_center_y = r.position.y + r.size.y / 2;
		int viewport_center_y = (from_v + to_v) / 2;

		int offset_y = item_center_y - viewport_center_y;
		scroll_bar_v->set_value(scroll_bar_v->get_value() + offset_y);
	}

	if (p_center_horizontally) {
		int from_h = scroll_bar_h->get_value();
		int to_h = from_h + scroll_bar_h->get_page();
		int item_center_x = r.position.x + r.size.x / 2;
		int viewport_center_x = (from_h + to_h) / 2;

		int offset_x = item_center_x - viewport_center_x;
		scroll_bar_h->set_value(scroll_bar_h->get_value() + offset_x);
	}
}

static Rect2 _adjust_to_max_size(Size2 p_size, Size2 p_max_size)
{
	Size2 size = p_max_size;
	int tex_width = p_size.width * size.height / p_size.height;
	int tex_height = size.height;

	if (tex_width > size.width) {
		tex_width = size.width;
		tex_height = p_size.height * tex_width / p_size.width;
	}

	int ofs_x = (size.width - tex_width) / 2;
	int ofs_y = (size.height - tex_height) / 2;

	return Rect2(ofs_x, ofs_y, tex_width, tex_height);
}

RID ItemList::get_focused_accessibility_element() const
{
	if (current == -1) {
		return get_accessibility_element();
	}
	else {
		const Item& item = items[current];
		return item.accessibility_item_element;
	}
}

int ItemList::get_item_at_position(const Point2& p_pos, bool p_exact) const
{
	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();
	pos.x += scroll_bar_h->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - scroll_bar_h->get_value() -
				theme_cache.panel_style->get_margin(SIDE_LEFT) -
				theme_cache.panel_style->get_margin(SIDE_RIGHT);
	}

	int closest = -1;
	int closest_dist = 0x7FFFFFFF;

	for (int i = 0; i < items.size(); i++) {
		Rect2 rc = items[i].rect_cache;

		if (i % current_columns ==
			current_columns -
				1) { // Make sure you can still select the last item when clicking past the column.
			if (is_layout_rtl()) {
				rc.size.width = get_size().width - scroll_bar_h->get_value() + rc.position.x;
			}
			else {
				rc.size.width = get_size().width + scroll_bar_h->get_value() - rc.position.x;
			}
		}

		if (rc.size.x < 0) {
			continue; // Skip negative item sizes, because they are off screen.
		}

		if (rc.has_point(pos)) {
			closest = i;

			break;
		}

		float dist = rc.distance_to(pos);
		if (!p_exact && dist < closest_dist) {
			closest = i;
			closest_dist = dist;
		}
	}

	return closest;
}

bool ItemList::is_pos_at_end_of_items(const Point2& p_pos) const
{
	if (items.is_empty()) {
		return true;
	}

	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x;
	}

	Rect2 endrect = items[items.size() - 1].rect_cache;
	return (pos.y > endrect.position.y + endrect.size.y);
}

Node::AutoTranslateMode ItemList::get_tooltip_auto_translate_mode_at(const Point2& p_at) const
{
	int closest = get_item_at_position(p_at, true);
	if (closest != -1) {
		return items[closest].auto_translate_mode;
	}
	return Control::get_tooltip_auto_translate_mode_at(p_at);
}

void ItemList::set_allow_rmb_select(bool p_allow) { allow_rmb_select = p_allow; }

bool ItemList::get_allow_rmb_select() const { return allow_rmb_select; }

void ItemList::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool ItemList::get_allow_reselect() const { return allow_reselect; }

void ItemList::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool ItemList::get_allow_search() const { return allow_search; }

real_t ItemList::get_icon_scale() const { return icon_scale; }

Vector<int> ItemList::get_selected_items()
{
	Vector<int> selected;
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			selected.push_back(i);
			if (select_mode == SELECT_SINGLE) {
				break;
			}
		}
	}
	return selected;
}

bool ItemList::is_anything_selected()
{
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			return true;
		}
	}

	return false;
}

Size2 ItemList::get_minimum_size() const
{
	Size2 min_size;
	if (auto_width) {
		min_size.x = auto_width_value;
	}

	if (auto_height) {
		min_size.y = auto_height_value;
	}
	return min_size;
}

void ItemList::set_autoscroll_to_bottom(const bool p_enable) { do_autoscroll_to_bottom = p_enable; }

bool ItemList::has_auto_width() const { return auto_width; }

bool ItemList::has_auto_height() const { return auto_height; }

TextServer::OverrunBehavior ItemList::get_text_overrun_behavior() const
{
	return text_overrun_behavior;
}

bool ItemList::has_wraparound_items() const { return wraparound_items; }

ItemList::ScrollHintMode ItemList::get_scroll_hint_mode() const { return scroll_hint_mode; }

bool ItemList::is_scroll_hint_tiled() { return tile_scroll_hint; }

ItemList::~ItemList() {}

void ItemList::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon) {}

void ItemList::deselect_all() {}


void ItemList::set_scroll_hint_mode(ItemList::ScrollHintMode) {}

void ItemList::set_fixed_column_width(int) {}

void ItemList::set_fixed_icon_size(Vector2i const&) {}

void ItemList::select(int, bool) {}

void ItemList::set_current(int) {}

void ItemList::ensure_current_is_visible() {}

void ItemList::clear() {}

int ItemList::add_item(String const&, Ref<Texture2D> const&, bool) {}

void ItemList::set_icon_scale(float) {}

ItemList::ItemList() {}

// --- texture_rect.cpp ---

Size2 TextureRect::get_minimum_size() const
{
	if (texture.is_valid()) {
		switch (expand_mode) {
		case EXPAND_KEEP_SIZE: {
			return texture->get_size();
		} break;
		case EXPAND_IGNORE_SIZE: {
			return Size2();
		} break;
		case EXPAND_FIT_WIDTH: {
			return Size2(get_size().y, 0);
		} break;
		case EXPAND_FIT_WIDTH_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_width()) / texture->get_height();
			return Size2(get_size().y * ratio, 0);
		} break;
		case EXPAND_FIT_HEIGHT: {
			return Size2(0, get_size().x);
		} break;
		case EXPAND_FIT_HEIGHT_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_height()) / texture->get_width();
			return Size2(0, get_size().x * ratio);
		} break;
		}
	}
	return Size2();
}

PackedStringArray TextureRect::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (stretch_mode == STRETCH_TILE) {
		Ref<AtlasTexture> at = texture;
		while (at.is_valid() && !at->is_animated()) {
			if (at->get_margin() != Rect2()) {
				warnings.push_back(vformat(RTR("STRETCH_TILE mode is not supported for an "
											   "AtlasTexture with non-zero margin.")));
				break;
			}
			at = at->get_atlas();
		}
	}

	return warnings;
}

Ref<Texture2D> TextureRect::get_texture() const { return texture; }

TextureRect::ExpandMode TextureRect::get_expand_mode() const { return expand_mode; }

void TextureRect::set_expand_mode(ExpandMode p_mode) {}

TextureRect::StretchMode TextureRect::get_stretch_mode() const { return stretch_mode; }

bool TextureRect::is_flipped_h() const { return hflip; }

bool TextureRect::is_flipped_v() const { return vflip; }

TextureRect::TextureRect() { set_mouse_filter(MOUSE_FILTER_PASS); }

TextureRect::~TextureRect() {}

void TextureRect::set_texture(const Ref<Texture2D>& p_tex) {}

void TextureRect::set_stretch_mode(StretchMode p_mode) {}

// --- nine_patch_rect.cpp ---

void NinePatchRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (texture.is_null()) {
			return;
		}

		Rect2 rect = Rect2(Point2(), get_size());
		Rect2 src_rect = region_rect;

		texture->get_rect_region(rect, src_rect, rect, src_rect);

		RID ci = get_canvas_item();
		Renderer::canvas_item_add_nine_patch(ci, rect, src_rect,
			texture->get_scaled_rid(), Vector2(margin[SIDE_LEFT], margin[SIDE_TOP]),
			Vector2(margin[SIDE_RIGHT], margin[SIDE_BOTTOM]), RSE::NinePatchAxisMode(axis_h),
			RSE::NinePatchAxisMode(axis_v), draw_center);
	} break;
	}
}

Size2 NinePatchRect::get_minimum_size() const
{
	return Size2(margin[SIDE_LEFT] + margin[SIDE_RIGHT], margin[SIDE_TOP] + margin[SIDE_BOTTOM]);
}

Ref<Texture2D> NinePatchRect::get_texture() const { return texture; }

int NinePatchRect::get_patch_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return margin[p_side];
}

void NinePatchRect::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;

	item_rect_changed();
}

Rect2 NinePatchRect::get_region_rect() const { return region_rect; }

bool NinePatchRect::is_draw_center_enabled() const { return draw_center; }

NinePatchRect::AxisStretchMode NinePatchRect::get_h_axis_stretch_mode() const { return axis_h; }

NinePatchRect::AxisStretchMode NinePatchRect::get_v_axis_stretch_mode() const { return axis_v; }

NinePatchRect::NinePatchRect() { set_mouse_filter(MOUSE_FILTER_IGNORE); }

NinePatchRect::~NinePatchRect() {}

// --- reference_rect.cpp ---

void ReferenceRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!is_inside_tree()) {
			return;
		}
		if (Engine::is_editor_hint() || !editor_only) {
			draw_rect(Rect2(Point2(), get_size()), border_color, false, border_width);
		}
	} break;
	}
}

Color ReferenceRect::get_border_color() const { return border_color; }

float ReferenceRect::get_border_width() const { return border_width; }

bool ReferenceRect::get_editor_only() const { return editor_only; }

// --- video_stream_player.cpp ---

int VideoStreamPlayer::sp_get_channel_count() const
{
	if (playback.is_null()) {
		return 0;
	}

	return playback->get_channels();
}

bool VideoStreamPlayer::mix(AudioFrame* p_buffer, int p_frames)
{
	// Check the amount resampler can really handle.
	// If it cannot, wait "wait_resampler_phase_limit" times.
	// This mechanism contributes to smoother pause/unpause operation.
	if (p_frames <= resampler.get_num_of_ready_frames() || wait_resampler_limit <= wait_resampler) {
		wait_resampler = 0;
		return resampler.mix(p_buffer, p_frames);
	}
	wait_resampler++;
	return false;
}

// Called from main thread (e.g. VideoStreamPlayback::update).
int VideoStreamPlayer::_audio_mix_callback(void* p_udata, const float* p_data, int p_frames)
{
	ERR_FAIL_NULL_V(p_udata, 0);
	ERR_FAIL_NULL_V(p_data, 0);

	VideoStreamPlayer* vp = static_cast<VideoStreamPlayer*>(p_udata);

	int todo = MIN(vp->resampler.get_writer_space(), p_frames);

	float* wb = vp->resampler.get_write_buffer();
	int c = vp->resampler.get_channel_count();

	for (int i = 0; i < todo * c; i++) {
		wb[i] = p_data[i];
	}
	vp->resampler.write(todo);

	return todo;
}

void VideoStreamPlayer::_mix_audios(void* p_self)
{
	ERR_FAIL_NULL(p_self);
	static_cast<VideoStreamPlayer*>(p_self)->_mix_audio();
}

// Called from audio thread
void VideoStreamPlayer::_mix_audio()
{
	if (stream.is_null()) {
		return;
	}
	if (playback.is_null() || !playback->is_playing() || playback->is_paused()) {
		return;
	}

	AudioFrame* buffer = mix_buffer.ptrw();
	int buffer_size = mix_buffer.size();

	// Resample
	if (!mix(buffer, buffer_size)) {
		return;
	}

	AudioFrame vol = AudioFrame(volume, volume);

	int cc = AudioServer::get_channel_count();

	if (cc == 1) {
		AudioFrame* target =
			AudioServer::thread_get_channel_mix_buffer(bus_index, 0);
		ERR_FAIL_NULL(target);

		for (int j = 0; j < buffer_size; j++) {
			target[j] += buffer[j] * vol;
		}

	}
	else {
		AudioFrame* targets[4];

		for (int k = 0; k < cc; k++) {
			targets[k] = AudioServer::thread_get_channel_mix_buffer(bus_index, k);
			ERR_FAIL_NULL(targets[k]);
		}

		for (int j = 0; j < buffer_size; j++) {
			AudioFrame frame = buffer[j] * vol;
			for (int k = 0; k < cc; k++) {
				targets[k][j] += frame;
			}
		}
	}
}

Size2 VideoStreamPlayer::get_minimum_size() const
{
	if (!expand && texture.is_valid()) {
		return texture_size;
	}
	else {
		return Size2();
	}
}

bool VideoStreamPlayer::has_expand() const { return expand; }

void VideoStreamPlayer::set_loop(bool p_loop) { loop = p_loop; }

bool VideoStreamPlayer::has_loop() const { return loop; }

Ref<VideoStream> VideoStreamPlayer::get_stream() const { return stream; }

void VideoStreamPlayer::play()
{
	ERR_FAIL_COND(!is_inside_tree());
	if (playback.is_null()) {
		return;
	}
	playback->play();
	set_process_internal(true);
	first_frame = true;

	if (!can_process()) {
		_notification(NOTIFICATION_PAUSED);
	}
}

void VideoStreamPlayer::stop()
{
	if (!is_inside_tree()) {
		return;
	}
	if (playback.is_null()) {
		return;
	}

	playback->stop();
	resampler.flush();
	set_process_internal(false);
}

bool VideoStreamPlayer::is_playing() const
{
	if (playback.is_null()) {
		return false;
	}

	return playback->is_playing();
}

void VideoStreamPlayer::set_paused(bool p_paused)
{
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;
	if (!p_paused && !can_process()) {
		paused_from_tree = true;
		return;
	}
	else if (p_paused && paused_from_tree) {
		paused_from_tree = false;
		return;
	}

	if (playback.is_valid()) {
		playback->set_paused(p_paused);
		set_process_internal(!p_paused);
	}
}

bool VideoStreamPlayer::is_paused() const { return paused; }

void VideoStreamPlayer::set_buffering_msec(int p_msec) { buffering_ms = p_msec; }

int VideoStreamPlayer::get_buffering_msec() const { return buffering_ms; }

void VideoStreamPlayer::set_audio_track(int p_track)
{
	audio_track = p_track;
	if (stream.is_valid()) {
		stream->set_audio_track(audio_track);
	}
	if (playback.is_valid()) {
		playback->set_audio_track(audio_track);
	}
}

int VideoStreamPlayer::get_audio_track() const { return audio_track; }

void VideoStreamPlayer::set_volume(float p_vol) { volume = p_vol; }

float VideoStreamPlayer::get_volume() const { return volume; }

void VideoStreamPlayer::set_volume_db(float p_db)
{
	if (p_db < -79) {
		set_volume(0);
	}
	else {
		set_volume(Math::db_to_linear(p_db));
	}
}

float VideoStreamPlayer::get_volume_db() const
{
	if (volume == 0) {
		return -80;
	}
	else {
		return Math::linear_to_db(volume);
	}
}

void VideoStreamPlayer::set_speed_scale(float p_speed_scale)
{
	ERR_FAIL_COND(p_speed_scale < 0.0);
	speed_scale = p_speed_scale;
}

float VideoStreamPlayer::get_speed_scale() const { return speed_scale; }

String VideoStreamPlayer::get_stream_name() const
{
	if (stream.is_null()) {
		return "<No Stream>";
	}
	return stream->get_name();
}

double VideoStreamPlayer::get_stream_length() const
{
	if (playback.is_null()) {
		return 0;
	}
	return playback->get_length();
}

double VideoStreamPlayer::get_stream_position() const
{
	if (playback.is_null()) {
		return 0;
	}
	return playback->get_playback_position();
}

void VideoStreamPlayer::set_stream_position(double p_position)
{
	if (playback.is_valid()) {
		resampler.flush();
		playback->seek(p_position);
		first_frame = true;
	}
}

Ref<Texture2D> VideoStreamPlayer::get_video_texture() const
{
	if (playback.is_valid()) {
		return playback->get_texture();
	}

	return Ref<Texture2D>();
}

void VideoStreamPlayer::set_autoplay(bool p_enable) { autoplay = p_enable; }

bool VideoStreamPlayer::has_autoplay() const { return autoplay; }

void VideoStreamPlayer::set_bus(const String& p_bus)
{
	// If audio is active, must lock this.
	AudioServer::lock();
	bus = p_bus;
	AudioServer::unlock();
}

String VideoStreamPlayer::get_bus() const
{
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == bus) {
			return bus;
		}
	}
	return SceneString(Master);
}

VideoStreamPlayer::~VideoStreamPlayer()
{
	// Not necessary here, but make in consistent with other "stream_player" classes.
	resampler.clear();
}



void VideoStreamPlayer::_notification(int) {}

// --- virtual_joystick.cpp ---

void VirtualJoystick::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!Engine::is_editor_hint() && visibility == VISIBILITY_WHEN_TOUCHED &&
			!is_pressed) {
			return;
		}

		Rect2 rect_joystick = Rect2(joystick_pos - Vector2(0.5, 0.5) * joystick_size,
			Vector2(joystick_size, joystick_size));
		draw_style_box(
			is_pressed ? theme_cache.pressed_joystick.ptr() : theme_cache.normal_joystick.ptr(),
			rect_joystick);

		Rect2 rect_tip = Rect2(tip_pos - Vector2(0.5, 0.5) * tip_size, Vector2(tip_size, tip_size));
		draw_style_box(
			is_pressed ? theme_cache.pressed_tip.ptr() : theme_cache.normal_tip.ptr(), rect_tip);
	} break;

	case NOTIFICATION_ENTER_TREE: {
		joystick_pos = get_size() * initial_offset_ratio;
		tip_pos = joystick_pos;
	} break;

	case NOTIFICATION_RESIZED: {
		_reset();
	} break;
	}
}

void VirtualJoystick::_reset() {}

void VirtualJoystick::_handle_input_actions()
{
	if (input_vector.x >= 0.0f && Input::is_action_pressed(action_left)) {
		Input::action_release(action_left);
	}
	if (input_vector.x <= 0.0f && Input::is_action_pressed(action_right)) {
		Input::action_release(action_right);
	}
	if (input_vector.y >= 0.0f && Input::is_action_pressed(action_up)) {
		Input::action_release(action_up);
	}
	if (input_vector.y <= 0.0f && Input::is_action_pressed(action_down)) {
		Input::action_release(action_down);
	}

	if (input_vector.x < 0.0f) {
		Input::action_press(action_left, -input_vector.x);
	}
	else if (input_vector.x > 0.0f) {
		Input::action_press(action_right, input_vector.x);
	}
	if (input_vector.y < 0.0f) {
		Input::action_press(action_up, -input_vector.y);
	}
	else if (input_vector.y > 0.0f) {
		Input::action_press(action_down, input_vector.y);
	}
}

Vector2 VirtualJoystick::get_joystick_position() const { return joystick_pos; }

void VirtualJoystick::set_joystick_size(float p_size)
{
	if (joystick_size == p_size) {
		return;
	}
	joystick_size = p_size;
	_reset();
}

float VirtualJoystick::get_joystick_size() const { return joystick_size; }

void VirtualJoystick::set_tip_size(float p_size)
{
	if (tip_size == p_size) {
		return;
	}
	tip_size = p_size;
	_reset();
}

float VirtualJoystick::get_tip_size() const { return tip_size; }

float VirtualJoystick::get_deadzone_ratio() const { return deadzone_ratio; }

float VirtualJoystick::get_clampzone_ratio() const { return clampzone_ratio; }

void VirtualJoystick::set_initial_offset_ratio(const Vector2& p_ratio)
{
	if (initial_offset_ratio == p_ratio) {
		return;
	}
	initial_offset_ratio = p_ratio;
	_reset();
}

Vector2 VirtualJoystick::get_initial_offset_ratio() const { return initial_offset_ratio; }

void VirtualJoystick::set_joystick_mode(JoystickMode p_mode) { joystick_mode = p_mode; }

VirtualJoystick::JoystickMode VirtualJoystick::get_joystick_mode() const { return joystick_mode; }

void VirtualJoystick::set_action_left(const String& p_action) { action_left = p_action; }

String VirtualJoystick::get_action_left() const { return action_left; }

void VirtualJoystick::set_action_right(const String& p_action) { action_right = p_action; }

String VirtualJoystick::get_action_right() const { return action_right; }

void VirtualJoystick::set_action_up(const String& p_action) { action_up = p_action; }

String VirtualJoystick::get_action_up() const { return action_up; }

void VirtualJoystick::set_action_down(const String& p_action) { action_down = p_action; }

String VirtualJoystick::get_action_down() const { return action_down; }

void VirtualJoystick::set_visibility_mode(VisibilityMode p_mode) { visibility = p_mode; }

VirtualJoystick::VisibilityMode VirtualJoystick::get_visibility_mode() const { return visibility; }

// ============================================================================
// Merged from scene/gui/rich_text_label.cpp
// ============================================================================

#include "core/input/input_map.h"
#include "core/io/resource_loader.h"
#include "core/math/math_defs.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "modules/modules_enabled.gen.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/dialogs.h"
// --- Inlined from scene/gui/rich_text_effect.h ---
#include "core/io/resource.h"

class CharFXTransform : public RefCounted
{
public:
	Transform2D transform;
	Vector2i range;
	bool visibility = true;
	bool outline = false;
	Point2 offset;
	Color color;
	double elapsed_time = 0.0f;
	uint32_t glyph_index = 0;
	uint16_t glyph_flags = 0;
	uint8_t glyph_count = 0;
	int32_t relative_index = 0;
	RID font;

	CharFXTransform();
	~CharFXTransform() = default;

	void set_transform(const Transform2D& p_transform) { transform = p_transform; }

	const Transform2D& get_transform() { return transform; }

	Vector2i get_range() { return range; }

	void set_range(const Vector2i& p_range) { range = p_range; }

	double get_elapsed_time() { return elapsed_time; }

	void set_elapsed_time(double p_elapsed_time) { elapsed_time = p_elapsed_time; }

	bool is_visible() { return visibility; }

	void set_visibility(bool p_visibility) { visibility = p_visibility; }

	bool is_outline() { return outline; }

	void set_outline(bool p_outline) { outline = p_outline; }

	Point2 get_offset() { return offset; }

	void set_offset(Point2 p_offset) { offset = p_offset; }

	Color get_color() { return color; }

	void set_color(Color p_color) { color = p_color; }

	uint32_t get_glyph_index() const { return glyph_index; }

	void set_glyph_index(uint32_t p_glyph_index) { glyph_index = p_glyph_index; }

	uint16_t get_glyph_flags() const { return glyph_flags; }

	void set_glyph_flags(uint16_t p_glyph_flags) { glyph_flags = p_glyph_flags; }

	uint8_t get_glyph_count() const { return glyph_count; }

	void set_glyph_count(uint8_t p_glyph_count) { glyph_count = p_glyph_count; }

	int32_t get_relative_index() const { return relative_index; }

	void set_relative_index(int32_t p_relative_index) { relative_index = p_relative_index; }

	RID get_font() const { return font; }

	void set_font(RID p_font) { font = p_font; }
};

class RichTextEffect : public Resource
{
public:
	bool _process_effect_impl(Ref<class CharFXTransform> p_cfx);

	RichTextEffect();
};



// --- End scene/gui/rich_text_effect.h ---
#include "scene/gui/scroll_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/text_line.h"
#include "scene/resources/texture.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif

RichTextLabel::ImageData::~ImageData() = default;

RichTextLabel::FXData::FXData(ItemType p_type)
{
	if (p_type == ITEM_CUSTOMFX) {
		char_fx_transform.instantiate();
	}
}

RichTextLabel::FXData::~FXData()
{
	char_fx_transform.unref();
	custom_effect.unref();
}

void RichTextLabel::Item::_clear_children()
{ // Only ever called on main or a paragraph (Line).
	while (subitems.size()) {
		Item* subitem = subitems.front()->get();
		memdelete(subitem);
		subitems.pop_front();
	}
}

RichTextLabel::Item::Item(ItemType p_type) : type(p_type)
{
	switch (p_type) {
	case ITEM_FRAME: {
		frame = memnew(FrameData);
	} break;
	case ITEM_DROPCAP: {
		dropcap = memnew(DropcapData);
	} break;
	case ITEM_IMAGE: {
		image = memnew(ImageData);
	} break;
	case ITEM_PARAGRAPH: {
		paragraph = memnew(ParagraphData);
	} break;
	case ITEM_TABLE: {
		table = memnew(TableData);
	} break;
	case ITEM_SHAKE:
	case ITEM_WAVE:
	case ITEM_TORNADO:
	case ITEM_RAINBOW:
	case ITEM_PULSE:
	case ITEM_CUSTOMFX: {
		fx = memnew(FXData(p_type));
	} break;
	case ITEM_FONT_SIZE: {
		font_size = 16;
	} break;
	case ITEM_LIST: {
		text = U"•";
	} break;
	default:
		break;
	}
}

RichTextLabel::Item::~Item()
{
	_clear_children();
	if (rid.is_valid()) {
		items.free(rid);
	}
	switch (type) {
	case ITEM_FRAME: {
		if (frame) {
			memdelete(frame);
		}
	} break;
	case ITEM_DROPCAP: {
		if (dropcap) {
			memdelete(dropcap);
		}
	} break;
	case ITEM_IMAGE: {
		if (image) {
			memdelete(image);
		}
	} break;
	case ITEM_PARAGRAPH: {
		if (paragraph) {
			memdelete(paragraph);
		}
	} break;
	case ITEM_TABLE: {
		if (table) {
			memdelete(table);
		}
	} break;
	case ITEM_SHAKE:
	case ITEM_WAVE:
	case ITEM_TORNADO:
	case ITEM_RAINBOW:
	case ITEM_PULSE:
	case ITEM_CUSTOMFX: {
		if (fx) {
			memdelete(fx);
		}
	} break;
	default:
		break;
	}
}

Rect2i _merge_or_copy_rect(const Rect2i& p_a, const Rect2i& p_b)
{
	if (!p_a.has_area()) {
		return p_b;
	}
	else {
		return p_a.merge(p_b);
	}
}

RichTextLabel::Line::~Line()
{
	if (accessibility_line_element.is_valid()) {
		AccessibilityServer::free_element(accessibility_line_element);
		accessibility_line_element = RID();
		accessibility_text_element = RID();
	}
}

RichTextLabel::Item* RichTextLabel::_get_next_item(Item* p_item, bool p_free) const
{
	if (!p_item) {
		return nullptr;
	}
	if (p_free) {
		if (p_item->subitems.size()) {
			return p_item->subitems.front()->get();
		}
		else if (!p_item->parent) {
			return nullptr;
		}
		else if (p_item->E->next()) {
			return p_item->E->next()->get();
		}
		else {
			// Go up until something with a next is found.
			while (p_item->parent && !p_item->E->next()) {
				p_item = p_item->parent;
			}

			if (p_item->parent) {
				return p_item->E->next()->get();
			}
			else {
				return nullptr;
			}
		}

	}
	else {
		if (p_item->subitems.size() && p_item->type != ITEM_TABLE) {
			return p_item->subitems.front()->get();
		}
		else if (p_item->type == ITEM_FRAME) {
			return nullptr;
		}
		else if (p_item->E->next()) {
			return p_item->E->next()->get();
		}
		else {
			// Go up until something with a next is found.
			while (p_item->parent && p_item->type != ITEM_FRAME && !p_item->E->next()) {
				p_item = p_item->parent;
			}

			if (p_item->type != ITEM_FRAME) {
				return p_item->E->next()->get();
			}
			else {
				return nullptr;
			}
		}
	}
}

RichTextLabel::Item* RichTextLabel::_get_prev_item(Item* p_item, bool p_free) const
{
	if (!p_item) {
		return nullptr;
	}
	if (p_free) {
		if (!p_item->parent) {
			return nullptr;
		}
		else if (p_item->E->prev()) {
			p_item = p_item->E->prev()->get();
			while (p_item->subitems.size()) {
				p_item = p_item->subitems.back()->get();
			}
			return p_item;
		}
		else {
			if (p_item->parent) {
				return p_item->parent;
			}
			else {
				return nullptr;
			}
		}

	}
	else {
		if (p_item->type == ITEM_FRAME) {
			return nullptr;
		}
		else if (p_item->E->prev()) {
			p_item = p_item->E->prev()->get();
			while (p_item->subitems.size() && p_item->type != ITEM_TABLE) {
				p_item = p_item->subitems.back()->get();
			}
			return p_item;
		}
		else {
			if (p_item->parent && p_item->type != ITEM_FRAME) {
				return p_item->parent;
			}
			else {
				return nullptr;
			}
		}
	}
}

Rect2 RichTextLabel::_get_text_rect()
{
	return Rect2(theme_cache.normal_style->get_offset(),
		get_size() - theme_cache.normal_style->get_minimum_size());
}

int RichTextLabel::_get_wrap_width(const Rect2& p_text_rect) const
{
	int wrap_width = p_text_rect.get_size().width;
	float combined_maximum_width = get_combined_maximum_size().x;
	if (autowrap_mode != TextServer::AUTOWRAP_OFF && combined_maximum_width > 0.0) {
		int maximum_width =
			int(combined_maximum_width - theme_cache.normal_style->get_minimum_size().width);
		if (maximum_width <= 0) {
			maximum_width = 1;
		}
		wrap_width = MIN(wrap_width, maximum_width);
		wrap_width = MAX(wrap_width, 1);
	}
	return wrap_width;
}

RichTextLabel::Item* RichTextLabel::_get_item_at_pos(
	RichTextLabel::Item* p_item_from, RichTextLabel::Item* p_item_to, int p_position)
{
	int offset = 0;
	for (Item* it = p_item_from; it && it != p_item_to; it = _get_next_item(it)) {
		switch (it->type) {
		case ITEM_TEXT: {
			offset += it->text.length();
			if (offset > p_position) {
				return it;
			}
		} break;
		case ITEM_NEWLINE: {
			offset += 1;
			if (offset == p_position) {
				return it;
			}
		} break;
		case ITEM_IMAGE: {
			offset += 1;
			if (offset > p_position) {
				return it;
			}
		} break;
		case ITEM_TABLE: {
			offset += it->table->char_count;
		} break;
		default:
			break;
		}
	}
	return p_item_from;
}

String RichTextLabel::_roman(int p_num, bool p_capitalize) const
{
	if (p_num > 3999) {
		return "ERR";
	};
	String s;
	if (p_capitalize) {
		const String roman_M[] = {"", "M", "MM", "MMM"};
		const String roman_C[] = {"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM"};
		const String roman_X[] = {"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC"};
		const String roman_I[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
		s = roman_M[p_num / 1000] + roman_C[(p_num % 1000) / 100] + roman_X[(p_num % 100) / 10] +
			roman_I[p_num % 10];
	}
	else {
		const String roman_M[] = {"", "m", "mm", "mmm"};
		const String roman_C[] = {"", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};
		const String roman_X[] = {"", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
		const String roman_I[] = {"", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
		s = roman_M[p_num / 1000] + roman_C[(p_num % 1000) / 100] + roman_X[(p_num % 100) / 10] +
			roman_I[p_num % 10];
	}
	return s;
}

String RichTextLabel::_letters(int p_num, bool p_capitalize) const
{
	uint64_t n = p_num;

	int chars = 0;
	do {
		n = (n - 1) / 26;
		chars++;
	} while (n);

	String s;
	s.resize_uninitialized(chars + 1);
	char32_t* c = s.ptrw();
	c[chars] = 0;
	n = p_num;
	do {
		const char a = (p_capitalize ? 'A' : 'a');
		n = n - 1;
		c[--chars] = a + n % 26;
		n /= 26;
	} while (n);

	return s;
}

String RichTextLabel::_get_prefix(
	Item* p_item, const Vector<int>& p_list_index, const Vector<Item*>& p_list_items)
{
	String prefix;
	int segments = 0;
	for (int i = 0; i < p_list_index.size(); i++) {
		String segment;
		if (p_list_items[i]->list_type == LIST_DOTS) {
			if (segments == 0) {
				prefix = p_list_items[i]->text;
			}
			break;
		}
		prefix = "." + prefix;
		if (p_list_items[i]->list_type == LIST_NUMBERS) {
			segment = itos(p_list_index[i]);
			if (is_localizing_numeral_system()) {
				segment = TranslationServer::format_number(
					segment, _find_language(p_item));
			}
			segments++;
		}
		else if (p_list_items[i]->list_type == LIST_LETTERS) {
			segment = _letters(p_list_index[i], p_list_items[i]->capitalize);
			segments++;
		}
		else if (p_list_items[i]->list_type == LIST_ROMAN) {
			segment = _roman(p_list_index[i], p_list_items[i]->capitalize);
			segments++;
		}
		prefix = segment + prefix;
	}
	return prefix + " ";
}

int RichTextLabel::_get_line_max_width(Item* p_frame, int p_line) const
{
	ERR_FAIL_NULL_V(p_frame, 0);
	FrameData& fr = *p_frame->frame;
	ERR_FAIL_INDEX_V(p_line, (int)fr.lines.size(), 0);

	Line& l = fr.lines[p_line];
	MutexLock lock(l.text_buf->get_mutex());

	int max_width = Math::ceil(l.text_buf->get_non_wrapped_size().x);

	Item* it_to =
		(p_line + 1 < (int)fr.lines.size()) ? fr.lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; it && it != it_to; it = _get_next_item(it)) {
		if (it->type != ITEM_TABLE) {
			continue;
		}

		TableData& table = *it->table;

		// Subtract the table's width recorded in text_buf.
		max_width -= table.total_width;

		// Recalculate the maximum width of the table.
		const int col_count = table.columns.size();
		max_width += theme_cache.table_h_separation * col_count;

		// The columns in the nested table have already been calculated in _shape_line().
		for (TableData::Column& C : table.columns) {
			max_width += C.max_width;
		}
	}

	max_width += Math::ceil(fr.padding.position.x + fr.padding.size.x + l.indent);

	return max_width;
}

Size2 RichTextLabel::_get_item_image_final_size(
	Item* p_img, float p_orig_width, float p_base_font_size)
{
	ImageData& img = *p_img->image;
	Size2 new_size(img.rq_size);
	Item* font_size_it = _find_font_size(p_img);

	if (img.width_unit == IMAGE_UNIT_PERCENT) {
		new_size.width = p_orig_width * img.rq_size.width / 100.f;
	}
	else if (img.width_unit == IMAGE_UNIT_EM) {
		new_size.width =
			(font_size_it ? font_size_it->font_size : p_base_font_size) * img.rq_size.width;
	}

	if (img.height_unit == IMAGE_UNIT_PERCENT) {
		new_size.height = p_orig_width * img.rq_size.height / 100.f;
	}
	else if (img.height_unit == IMAGE_UNIT_EM) {
		new_size.height =
			(font_size_it ? font_size_it->font_size : p_base_font_size) * img.rq_size.height;
	}
	return new_size;
}

void RichTextLabel::_update_table_column_width(Item* p_table, int p_available_width)
{
	TableData& tbl = *p_table->table;
	const int col_count = tbl.columns.size();

	LocalVector<bool> columns_will_stretch;
	columns_will_stretch.resize(col_count);

	int total_ratio = 0; // The total ratio of the columns that will stretch.
	tbl.total_width = 0;
	float remaining_width = 0; // The available width of the columns that will stretch.

	for (int i = 0; i < col_count; i++) {
		TableData::Column& C = tbl.columns[i];
		tbl.total_width += C.min_width;
		C.width = C.min_width;
		if (C.max_width > C.min_width) {
			C.expand = true; // Like a hack.
		}
		columns_will_stretch[i] = C.expand;
		if (columns_will_stretch[i]) {
			remaining_width += C.min_width;
			total_ratio += C.expand_ratio;
		}
	}

	int diff = p_available_width - tbl.total_width;
	if (diff < 0) {
		diff = 0; // Avoid negative stretch space.
	}
	remaining_width += diff;

	// Resize to max_width if needed and distribute the remaining space.
	bool table_need_fit = true;
	while (table_need_fit) {
		table_need_fit = false;
		float error = 0.0; // Keep track of accumulated error in pixels.

		for (int i = 0; i < col_count; i++) {
			if (!columns_will_stretch[i]) {
				continue;
			}
			TableData::Column& C = tbl.columns[i];

			float final_pixel_size = remaining_width * C.expand_ratio / total_ratio;
			error += Math::fract(final_pixel_size);

			if (C.shrink && final_pixel_size > C.max_width) {
				columns_will_stretch[i] = false;
				total_ratio -= C.expand_ratio;
				table_need_fit = true;
				remaining_width -= C.max_width;
				C.width = C.max_width;
				break;
			}

			if (final_pixel_size < C.min_width) {
				// If available stretching area is too small for widget,
				// then remove it from stretching area.
				columns_will_stretch[i] = false;
				total_ratio -= C.expand_ratio;
				table_need_fit = true;
				remaining_width -= C.min_width;
				C.width = C.min_width;
				break;
			}

			C.width = final_pixel_size;
			// Dump accumulated error if one pixel or more.
			if (error >= 1) {
				C.width += 1;
				error -= 1;
			}
		}
	}

	// Recalculate total width.
	tbl.total_width = theme_cache.table_h_separation * col_count;
	for (TableData::Column& C : tbl.columns) {
		tbl.total_width += C.width;
	}
}

void RichTextLabel::_update_table_size(Item* p_table)
{
	TableData& tbl = *p_table->table;
	const int col_count = tbl.columns.size();

	// Update line width and get total height.
	int idx = 0;
	tbl.total_height = 0;
	tbl.rows.clear();
	tbl.rows_baseline.clear();

	Vector2 offset =
		Vector2(theme_cache.table_h_separation * 0.5, theme_cache.table_v_separation * 0.5).floor();
	float row_height = 0.0;
	const List<Item*>::Element* prev = p_table->subitems.front();

	for (const List<Item*>::Element* E = prev; E; E = E->next()) {
		ERR_CONTINUE(E->get()->type != ITEM_FRAME); // Children should all be frames.
		FrameData& frame = *E->get()->frame;

		int column = idx % col_count;
		const real_t frame_padding_space = frame.padding.position.x + frame.padding.size.x;

		offset += frame.padding.position;
		float yofs = 0.0;
		float prev_h = 0.0;
		float row_baseline = 0.0;
		for (int i = 0; i < (int)frame.lines.size(); i++) {
			Line& line = frame.lines[i];
			MutexLock sub_lock(line.text_buf->get_mutex());
			line.text_buf->set_width(
				tbl.columns[column].width - Math::ceil(frame_padding_space + line.indent));
			line.offset.y = prev_h;

			float h = line.text_buf->get_size().y +
					  (line.text_buf->get_line_count() - 1) * theme_cache.line_separation;
			if (i > 0) {
				h += theme_cache.paragraph_separation + theme_cache.line_separation;
			}
			if (frame.min_size_over.y > 0) {
				h = MAX(h, frame.min_size_over.y);
			}
			if (frame.max_size_over.y > 0) {
				h = MIN(h, frame.max_size_over.y);
			}
			yofs += h;
			prev_h = line.offset.y + line.text_buf->get_size().y +
					 line.text_buf->get_line_count() * theme_cache.line_separation +
					 theme_cache.paragraph_separation;

			line.offset += offset;
			row_baseline = MAX(
				row_baseline, line.text_buf->get_line_ascent(line.text_buf->get_line_count() - 1));
		}

		offset -= frame.padding.position;
		offset.x += tbl.columns[column].width + theme_cache.table_h_separation;

		row_height = MAX(yofs + frame.padding.position.y + frame.padding.size.y, row_height);
		// Add row height after last column of the row or last cell of the table.
		if (column == col_count - 1 || E->next() == nullptr) {
			offset.x = Math::floor(theme_cache.table_h_separation * 0.5);
			row_height += theme_cache.table_v_separation;
			tbl.rows.push_back(row_height);
			tbl.rows_baseline.push_back(tbl.total_height + row_baseline +
										Math::floor(theme_cache.table_v_separation * 0.5));
			tbl.total_height += row_height;
			offset.y += row_height;
			row_height = 0.0;
			prev = E->next();
		}
		idx++;
	}
}

void RichTextLabel::_find_click(Item* p_frame, const Point2i& p_click,
	Item** r_click_frame, int* r_click_line, Item** r_click_item, int* r_click_char,
	bool* r_outside, bool p_meta)
{
	if (r_click_item) {
		*r_click_item = nullptr;
	}
	if (r_click_char != nullptr) {
		*r_click_char = 0;
	}
	if (r_outside != nullptr) {
		*r_outside = true;
	}

	Size2 size = get_size();
	Rect2 text_rect = _get_text_rect();

	int vofs = vscroll->get_value();

	// Search for the first line.
	FrameData& main_fr = *main->frame;
	int to_line = main_fr.first_invalid_line.load();
	int from_line = _find_first_line(0, to_line, vofs);

	int total_height = INT32_MAX;
	if (to_line && vertical_alignment != VERTICAL_ALIGNMENT_TOP) {
		MutexLock lock(main_fr.lines[to_line - 1].text_buf->get_mutex());
		if (theme_cache.line_separation < 0) {
			// Do not apply to the last line to avoid cutting text.
			total_height = main_fr.lines[to_line - 1].offset.y +
						   main_fr.lines[to_line - 1].text_buf->get_size().y +
						   (main_fr.lines[to_line - 1].text_buf->get_line_count() - 1) *
							   theme_cache.line_separation;
		}
		else {
			total_height =
				main_fr.lines[to_line - 1].offset.y +
				main_fr.lines[to_line - 1].text_buf->get_size().y +
				main_fr.lines[to_line - 1].text_buf->get_line_count() * theme_cache.line_separation +
				theme_cache.paragraph_separation;
		}
	}
	float vbegin = 0, vsep = 0;
	if (text_rect.size.y > total_height) {
		switch (vertical_alignment) {
		case VERTICAL_ALIGNMENT_TOP: {
			// Nothing.
		} break;
		case VERTICAL_ALIGNMENT_CENTER: {
			vbegin = (text_rect.size.y - total_height) / 2;
		} break;
		case VERTICAL_ALIGNMENT_BOTTOM: {
			vbegin = text_rect.size.y - total_height;
		} break;
		case VERTICAL_ALIGNMENT_FILL: {
			int lines = 0;
			for (int l = from_line; l < to_line; l++) {
				MutexLock lock(main_fr.lines[l].text_buf->get_mutex());
				lines += main_fr.lines[l].text_buf->get_line_count();
			}
			if (lines > 1) {
				vsep = (text_rect.size.y - total_height) / (lines - 1);
			}
		} break;
		}
	}

	Point2 ofs =
		text_rect.get_position() + Vector2(0, vbegin + main_fr.lines[from_line].offset.y - vofs);
	while (ofs.y < size.height && from_line < to_line) {
		MutexLock lock(main_fr.lines[from_line].text_buf->get_mutex());
		_find_click_in_line(p_frame, from_line, ofs, text_rect.size.x, vsep, p_click, r_click_frame,
			r_click_line, r_click_item, r_click_char, false, p_meta);
		ofs.y += main_fr.lines[from_line].text_buf->get_size().y +
				 main_fr.lines[from_line].text_buf->get_line_count() *
					 (theme_cache.line_separation + vsep) +
				 (theme_cache.paragraph_separation);
		if (((r_click_item != nullptr) && ((*r_click_item) != nullptr)) ||
			((r_click_frame != nullptr) && ((*r_click_frame) != nullptr))) {
			if (r_outside != nullptr) {
				*r_outside = false;
			}
			return;
		}
		from_line++;
	}
}

void RichTextLabel::_update_fx(RichTextLabel::Item* p_frame, double p_delta_time)
{
	Item* it = p_frame;
	while (it) {
		if (!it->is_fx()) {
			it = _get_next_item(it, true);
			continue;
		}

		FXData& ifx = *it->fx;
		ifx.elapsed_time += p_delta_time;

		if (it->type == ITEM_SHAKE) {
			bool cycle = (ifx.elapsed_time > (1.0f / ifx.rate));
			if (cycle) {
				ifx.elapsed_time -= (1.0f / ifx.rate);
				ifx.reroll_random();
			}
		}

		it = _get_next_item(it, true);
	}
}

int RichTextLabel::_find_first_line(int p_from, int p_to, int p_vofs) const
{
	FrameData& main_fr = *main->frame;
	int l = p_from;
	int r = p_to;
	while (l < r) {
		int m = Math::floor(double(l + r) / 2.0);
		MutexLock lock(main_fr.lines[m].text_buf->get_mutex());
		int ofs = _calculate_line_vertical_offset(main_fr.lines[m]);
		if (ofs < p_vofs) {
			l = m + 1;
		}
		else {
			r = m;
		}
	}
	return MIN(l, (int)main_fr.lines.size() - 1);
}

_FORCE_INLINE_ float RichTextLabel::_calculate_line_vertical_offset(
	const RichTextLabel::Line& line) const
{
	return line.get_height(theme_cache.line_separation, theme_cache.paragraph_separation);
}

void RichTextLabel::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
	use_selected_font_color = theme_cache.font_selected_color != Color(0, 0, 0, 0);
}

PackedStringArray RichTextLabel::get_accessibility_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_accessibility_configuration_warnings();

	Item* it = main;
	while (it) {
		if (it->type == ITEM_IMAGE) {
			if (it->image && it->image->alt_text.strip_edges().is_empty()) {
				warnings.push_back(RTR("Image alternative text must not be empty."));
			}
		}
		it = _get_next_item(it, true);
	}

	return warnings;
}

void RichTextLabel::_invalidate_accessibility()
{
	if (accessibility_scroll_element.is_null()) {
		return;
	}

	Item* it = main;
	while (it) {
		if (it->type == ITEM_FRAME) {
			FrameData& fr = *it->frame;
			for (size_t i = 0; i < fr.lines.size(); i++) {
				if (fr.lines[i].accessibility_line_element.is_valid()) {
					AccessibilityServer::free_element(
						fr.lines[i].accessibility_line_element);
				}
				fr.lines[i].accessibility_line_element = RID();
				fr.lines[i].accessibility_text_element = RID();
			}
		}
		it->accessibility_item_element = RID();
		it = _get_next_item(it, true);
	}
}

RID RichTextLabel::get_focused_accessibility_element() const
{
	if (keyboard_focus_frame && keyboard_focus_item) {
		if (keyboard_focus_on_text) {
			return keyboard_focus_frame->frame->lines[keyboard_focus_line]
				.accessibility_text_element;
		}
		else {
			if (keyboard_focus_item->accessibility_item_element.is_valid()) {
				return keyboard_focus_item->accessibility_item_element;
			}
		}
	}
	else {
		if (!main->frame->lines.is_empty()) {
			return main->frame->lines[0].accessibility_text_element;
		}
	}
	return get_accessibility_element();
}

void RichTextLabel::_prepare_scroll_anchor()
{
	scroll_w = vscroll->get_bound_minimum_size().width;
	vscroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -scroll_w);
}

void RichTextLabel::_find_frame(Item* p_item, Item** r_frame, int* r_line)
{
	if (r_frame != nullptr) {
		*r_frame = nullptr;
	}
	if (r_line != nullptr) {
		*r_line = 0;
	}

	Item* item = p_item;

	while (item) {
		if (item->parent != nullptr && item->parent->type == ITEM_FRAME) {
			if (r_frame != nullptr) {
				*r_frame = item->parent;
			}
			if (r_line != nullptr) {
				*r_line = item->line;
			}
			return;
		}

		item = item->parent;
	}
}

RichTextLabel::Item* RichTextLabel::_find_indentable(Item* p_item)
{
	Item* indentable = p_item;

	while (indentable) {
		if (indentable->type == ITEM_INDENT || indentable->type == ITEM_LIST) {
			return indentable;
		}
		indentable = indentable->parent;
	}

	return indentable;
}

RichTextLabel::Item* RichTextLabel::_find_font(Item* p_item)
{
	Item* fontitem = p_item;

	while (fontitem) {
		if (fontitem->type == ITEM_FONT) {
			switch (fontitem->def_font) {
			case RTL_NORMAL_FONT: {
				if (fontitem->variation) {
					Ref<FontVariation> fc = fontitem->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.normal_font);
					}
				}
				else {
					fontitem->font = theme_cache.normal_font;
				}
				if (fontitem->def_size) {
					fontitem->font_size = theme_cache.normal_font_size;
				}
			} break;
			case RTL_BOLD_FONT: {
				if (fontitem->variation) {
					Ref<FontVariation> fc = fontitem->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.bold_font);
					}
				}
				else {
					fontitem->font = theme_cache.bold_font;
				}
				if (fontitem->def_size) {
					fontitem->font_size = theme_cache.bold_font_size;
				}
			} break;
			case RTL_ITALICS_FONT: {
				if (fontitem->variation) {
					Ref<FontVariation> fc = fontitem->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.italics_font);
					}
				}
				else {
					fontitem->font = theme_cache.italics_font;
				}
				if (fontitem->def_size) {
					fontitem->font_size = theme_cache.italics_font_size;
				}
			} break;
			case RTL_BOLD_ITALICS_FONT: {
				if (fontitem->variation) {
					Ref<FontVariation> fc = fontitem->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.bold_italics_font);
					}
				}
				else {
					fontitem->font = theme_cache.bold_italics_font;
				}
				if (fontitem->def_size) {
					fontitem->font_size = theme_cache.bold_italics_font_size;
				}
			} break;
			case RTL_MONO_FONT: {
				if (fontitem->variation) {
					Ref<FontVariation> fc = fontitem->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.mono_font);
					}
				}
				else {
					fontitem->font = theme_cache.mono_font;
				}
				if (fontitem->def_size) {
					fontitem->font_size = theme_cache.mono_font_size;
				}
			} break;
			default: {
			} break;
			}
			return fontitem;
		}

		fontitem = fontitem->parent;
	}

	return nullptr;
}

RichTextLabel::Item* RichTextLabel::_find_font_size(Item* p_item)
{
	Item* sizeitem = p_item;

	while (sizeitem) {
		if (sizeitem->type == ITEM_FONT_SIZE) {
			return sizeitem;
		}

		sizeitem = sizeitem->parent;
	}

	return nullptr;
}

int RichTextLabel::_find_outline_size(Item* p_item, int p_default)
{
	Item* sizeitem = p_item;

	while (sizeitem) {
		if (sizeitem->type == ITEM_OUTLINE_SIZE) {
			return sizeitem->outline_size;
		}

		sizeitem = sizeitem->parent;
	}

	return p_default;
}

RichTextLabel::Item* RichTextLabel::_find_dc_item(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_DROPCAP) {
			return item;
		}
		item = item->parent;
	}

	return nullptr;
}

RichTextLabel::Item* RichTextLabel::_find_list_item(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_LIST) {
			return item;
		}
		item = item->parent;
	}

	return nullptr;
}

int RichTextLabel::_find_list(
	Item* p_item, Vector<int>& r_index, Vector<int>& r_count, Vector<Item*>& r_list)
{
	Item* item = p_item;
	Item* prev_item = p_item;

	int level = 0;

	while (item) {
		if (item->type == ITEM_LIST) {
			Item* frame = nullptr;
			int line = -1;
			_find_frame(item, &frame, &line);

			int index = 1;
			int count = 1;
			if (frame != nullptr) {
				FrameData& fr = *frame->frame;
				for (int i = item->line + 1; i < (int)fr.lines.size(); i++) {
					if (_find_list_item(fr.lines[i].from) == item) {
						if (i <= prev_item->line) {
							index++;
						}
						count++;
					}
				}
			}

			r_index.push_back(index);
			r_count.push_back(count);
			r_list.push_back(item);

			prev_item = item;
		}
		level++;
		item = item->parent;
	}

	return level;
}

int RichTextLabel::_find_margin(Item* p_item, const Ref<Font>& p_base_font, int p_base_font_size)
{
	Item* item = p_item;
	while (item && item->subitems.size()) {
		Item* si = item->subitems.front()->get();
		if (si && (si->type == ITEM_INDENT || si->type == ITEM_LIST)) {
			item = si;
		}
		else {
			break;
		}
	}

	float margin = 0.0;

	while (item) {
		if (item->type == ITEM_FRAME) {
			break;
		}

		if (item->type == ITEM_INDENT || item->type == ITEM_LIST) {
			int lvl = item->level;
			Ref<Font> font = p_base_font;
			int font_size = p_base_font_size;

			Item* font_it = _find_font(item);
			if (font_it) {
				if (font_it->font.is_valid()) {
					font = font_it->font;
				}
				if (font_it->font_size > 0) {
					font_size = font_it->font_size;
				}
			}
			Item* font_size_it = _find_font_size(item);
			if (font_size_it && font_size_it->font_size > 0) {
				font_size = font_size_it->font_size;
			}
			if (tab_size > 0) {
				margin += MAX(1, lvl * tab_size *
									 (font->get_char_size(' ', font_size).width +
										 font->get_spacing(TextServer::SPACING_SPACE)));
			}
		}

		item = item->parent;
	}

	return margin;
}

uint32_t RichTextLabel::_find_jst_flags(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			return item->paragraph->jst_flags;
		}

		item = item->parent;
	}

	return default_jst_flags;
}

PackedFloat32Array RichTextLabel::_find_tab_stops(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			return item->paragraph->tab_stops;
		}

		item = item->parent;
	}

	return default_tab_stops;
}

HorizontalAlignment RichTextLabel::_find_alignment(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			return item->paragraph->alignment;
		}

		item = item->parent;
	}

	return default_alignment;
}

TextServer::Direction RichTextLabel::_find_direction(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			if (item->paragraph->direction != Control::TEXT_DIRECTION_INHERITED) {
				return (TextServer::Direction)item->paragraph->direction;
			}
		}

		item = item->parent;
	}

	if (text_direction == Control::TEXT_DIRECTION_INHERITED) {
		return is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR;
	}
	else {
		return (TextServer::Direction)text_direction;
	}
}

TextServer::StructuredTextParser RichTextLabel::_find_stt(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			return item->paragraph->st_parser;
		}

		item = item->parent;
	}

	return st_parser;
}

Color RichTextLabel::_find_color(Item* p_item, const Color& p_default_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_COLOR) {
			return item->color;
		}

		item = item->parent;
	}

	return p_default_color;
}

Color RichTextLabel::_find_outline_color(Item* p_item, const Color& p_default_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_OUTLINE_COLOR) {
			return item->color;
		}

		item = item->parent;
	}

	return p_default_color;
}

bool RichTextLabel::_find_underline(Item* p_item, Color* r_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_UNDERLINE) {
			if (r_color) {
				*r_color = item->color;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

bool RichTextLabel::_find_strikethrough(Item* p_item, Color* r_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_STRIKETHROUGH) {
			if (r_color) {
				*r_color = item->color;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

void RichTextLabel::_fetch_item_fx_stack(Item* p_item, Vector<Item*>& r_stack)
{
	Item* item = p_item;
	while (item) {
		if (item->is_fx()) {
			r_stack.push_back(item);
		}

		item = item->parent;
	}
}

void RichTextLabel::_normalize_subtags(Vector<String>& subtags)
{
	for (String& subtag : subtags) {
		subtag = subtag.unquote();
	}
}

bool RichTextLabel::_find_hint(Item* p_item, String* r_description)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_HINT) {
			if (r_description) {
				*r_description = item->text;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

Color RichTextLabel::_find_bgcolor(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_BGCOLOR) {
			return item->color;
		}

		item = item->parent;
	}

	return Color(0, 0, 0, 0);
}

Color RichTextLabel::_find_fgcolor(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_FGCOLOR) {
			return item->color;
		}

		item = item->parent;
	}

	return Color(0, 0, 0, 0);
}

bool RichTextLabel::_find_layout_subitem(Item* from, Item* to)
{
	if (from && from != to) {
		if (from->type != ITEM_FONT && from->type != ITEM_COLOR && from->type != ITEM_UNDERLINE &&
			from->type != ITEM_STRIKETHROUGH && from->type != ITEM_INDENT) {
			return true;
		}

		for (Item* E : from->subitems) {
			bool layout = _find_layout_subitem(E, to);

			if (layout) {
				return true;
			}
		}
	}

	return false;
}

int RichTextLabel::get_pending_paragraphs() const
{
	int to_line = main->frame->first_invalid_line.load();
	int lines = main->frame->lines.size();

	return lines - to_line;
}

bool RichTextLabel::is_finished() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	if (updating.load()) {
		return false;
	}
	FrameData& main_fr = *main->frame;
	return (main_fr.first_invalid_line.load() == (int)main_fr.lines.size() &&
			main_fr.first_resized_line.load() == (int)main_fr.lines.size() &&
			main_fr.first_invalid_font_line.load() == (int)main_fr.lines.size());
}

bool RichTextLabel::is_updating() const { return updating.load() || validating.load(); }

bool RichTextLabel::is_threaded() const { return threaded; }

void RichTextLabel::set_progress_bar_delay(int p_delay_ms) { progress_delay = p_delay_ms; }

int RichTextLabel::get_progress_bar_delay() const { return progress_delay; }

void RichTextLabel::_invalidate_current_line(Item* p_frame)
{
	FrameData& fr = *p_frame->frame;
	if ((int)fr.lines.size() - 1 <= fr.first_invalid_line) {
		fr.first_invalid_line = (int)fr.lines.size() - 1;
		queue_accessibility_update();
	}
}

Size2 RichTextLabel::_get_image_size(
	const Ref<Texture2D>& p_image, float p_width, float p_height, const Rect2& p_region)
{
	Size2 ret;
	if (p_width > 0) {
		// custom width
		ret.width = p_width;
		if (p_height > 0) {
			// custom height
			ret.height = p_height;
		}
		else {
			// calculate height to keep aspect ratio
			if (p_region.has_area()) {
				ret.height = p_region.get_size().height * p_width / p_region.get_size().width;
			}
			else {
				ret.height = p_image->get_height() * p_width / p_image->get_width();
			}
		}
	}
	else {
		if (p_height > 0) {
			// custom height
			ret.height = p_height;
			// calculate width to keep aspect ratio
			if (p_region.has_area()) {
				ret.width = p_region.get_size().width * p_height / p_region.get_size().height;
			}
			else {
				ret.width = p_image->get_width() * p_height / p_image->get_height();
			}
		}
		else {
			if (p_region.has_area()) {
				// if the image has a region, keep the region size
				ret = p_region.get_size();
			}
			else {
				// keep original width and height
				ret = p_image->get_size();
			}
		}
	}
	return ret;
}

void RichTextLabel::_remove_frame(HashSet<Item*>& r_erase_list, Item* p_frame, int p_line,
	bool p_erase, int p_char_offset, int p_line_offset)
{
	FrameData& fr = *p_frame->frame;
	Line& l = fr.lines[p_line];
	Item* it_to =
		(p_line + 1 < (int)fr.lines.size()) ? fr.lines[p_line + 1].from : nullptr;
	if (!p_erase) {
		l.char_offset -= p_char_offset;
	}

	for (Item* it = l.from; it && it != it_to;) {
		Item* next_it = _get_next_item(it);
		it->line -= p_line_offset;
		if (!p_erase) {
			while (r_erase_list.has(it->parent)) {
				it->E->erase();
				it->parent = it->parent->parent;
				it->E = it->parent->subitems.push_back(it);
			}
		}
		if (it->type == ITEM_TABLE) {
			for (List<Item*>::Element* sub_it = it->subitems.front(); sub_it;
				 sub_it = sub_it->next()) {
				ERR_CONTINUE(sub_it->get()->type != ITEM_FRAME); // Children should all be frames.
				Item* frame = sub_it->get();
				for (int i = 0; i < (int)frame->frame->lines.size(); i++) {
					_remove_frame(r_erase_list, frame, i, p_erase, p_char_offset, 0);
				}
				if (p_erase) {
					r_erase_list.insert(frame);
				}
				else {
					frame->char_ofs -= p_char_offset;
				}
			}
		}
		if (p_erase) {
			r_erase_list.insert(it);
		}
		else {
			it->char_ofs -= p_char_offset;
		}
		it = next_it;
	}
}

void RichTextLabel::push_normal()
{
	ERR_FAIL_COND(theme_cache.normal_font.is_null());

	_push_def_font(RTL_NORMAL_FONT);
}

void RichTextLabel::push_bold()
{
	ERR_FAIL_COND(theme_cache.bold_font.is_null());

	Item* item_font = _find_font(current);
	_push_def_font((item_font && item_font->def_font == RTL_ITALICS_FONT) ? RTL_BOLD_ITALICS_FONT
																		  : RTL_BOLD_FONT);
}

void RichTextLabel::push_bold_italics()
{
	ERR_FAIL_COND(theme_cache.bold_italics_font.is_null());

	_push_def_font(RTL_BOLD_ITALICS_FONT);
}

void RichTextLabel::push_italics()
{
	ERR_FAIL_COND(theme_cache.italics_font.is_null());

	Item* item_font = _find_font(current);
	_push_def_font((item_font && item_font->def_font == RTL_BOLD_FONT) ? RTL_BOLD_ITALICS_FONT
																	   : RTL_ITALICS_FONT);
}

void RichTextLabel::push_mono()
{
	ERR_FAIL_COND(theme_cache.mono_font.is_null());

	_push_def_font(RTL_MONO_FONT);
}

int RichTextLabel::get_current_table_column() const
{
	ERR_FAIL_COND_V(current->type != ITEM_TABLE, -1);

	return current->subitems.size() % current->table->columns.size();
}

int RichTextLabel::get_tab_size() const { return tab_size; }

bool RichTextLabel::is_fit_content_enabled() const { return fit_content; }

bool RichTextLabel::is_meta_underlined() const { return underline_meta; }

bool RichTextLabel::is_hint_underlined() const { return underline_hint; }

void RichTextLabel::set_offset(int p_pixel)
{
	vscroll->set_value(p_pixel);
	queue_accessibility_update();
}

bool RichTextLabel::is_scroll_active() const { return scroll_active; }

void RichTextLabel::set_scroll_follow(bool p_follow)
{
	scroll_follow = p_follow;
	if (!vscroll->is_visible_in_tree() ||
		vscroll->get_value() > (vscroll->get_max() - vscroll->get_page() - 1)) {
		scroll_following = true;
	}
}

bool RichTextLabel::is_scroll_following() const { return scroll_follow; }

void RichTextLabel::_update_follow_vc()
{
	if (!scroll_follow_visible_characters) {
		return;
	}
	int vc = (visible_characters < 0 ? get_total_character_count()
									 : MIN(visible_characters, get_total_character_count())) -
			 1;
	int voff = get_character_line(vc) + 1;
	if (voff <= get_line_count() - 1) {
		follow_vc_pos = get_line_offset(voff) - _get_text_rect().size.y;
	}
	else {
		follow_vc_pos = vscroll->get_max();
	}
	vscroll->scroll_to(follow_vc_pos);
}

void RichTextLabel::set_scroll_follow_visible_characters(bool p_follow)
{
	if (scroll_follow_visible_characters != p_follow) {
		scroll_follow_visible_characters = p_follow;
		_update_follow_vc();
	}
}

bool RichTextLabel::is_scroll_following_visible_characters() const
{
	return scroll_follow_visible_characters;
}

void RichTextLabel::parse_bbcode(const String& p_bbcode)
{
	clear();
	append_text(p_bbcode);
}

String RichTextLabel::_get_tag_value(const String& p_tag)
{
	return p_tag.substr(p_tag.find_char('=') + 1);
}

int RichTextLabel::_find_unquoted(const String& p_src, char32_t p_chr, int p_from)
{
	if (p_from < 0) {
		return -1;
	}

	const int len = p_src.length();
	if (len == 0) {
		return -1;
	}

	const char32_t* src = p_src.get_data();
	bool in_single_quote = false;
	bool in_double_quote = false;
	for (int i = p_from; i < len; i++) {
		if (in_double_quote) {
			if (src[i] == '"') {
				in_double_quote = false;
			}
		}
		else if (in_single_quote) {
			if (src[i] == '\'') {
				in_single_quote = false;
			}
		}
		else {
			if (src[i] == '"') {
				in_double_quote = true;
			}
			else if (src[i] == '\'') {
				in_single_quote = true;
			}
			else if (src[i] == p_chr) {
				return i;
			}
		}
	}

	return -1;
}

Vector<String> RichTextLabel::_split_unquoted(const String& p_src, char32_t p_splitter)
{
	Vector<String> ret;

	if (p_src.is_empty()) {
		return ret;
	}

	int from = 0;
	int len = p_src.length();

	while (true) {
		int end = _find_unquoted(p_src, p_splitter, from);
		if (end < 0) {
			end = len;
		}
		if (end > from) {
			ret.push_back(p_src.substr(from, end - from));
		}
		if (end == len) {
			break;
		}

		from = end + 1;
	}

	return ret;
}

void RichTextLabel::scroll_to_selection()
{
	float line_offset = get_selection_line_offset();
	if (line_offset != -1.0) {
		vscroll->set_value(line_offset);
		queue_accessibility_update();
	}
}

void RichTextLabel::scroll_to_paragraph(int p_paragraph)
{
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	if (p_paragraph <= 0) {
		vscroll->set_value(0);
	}
	else if (p_paragraph >= main_fr.first_invalid_line.load()) {
		vscroll->set_value(vscroll->get_max());
	}
	else {
		vscroll->set_value(main_fr.lines[p_paragraph].offset.y);
	}
	queue_accessibility_update();
}

int RichTextLabel::get_paragraph_count() const { return main->frame->lines.size(); }

int RichTextLabel::get_visible_paragraph_count() const
{
	if (!is_visible()) {
		return 0;
	}

	const_cast<RichTextLabel*>(this)->_validate_line_caches();
	return visible_paragraph_count;
}

void RichTextLabel::scroll_to_line(int p_line)
{
	if (p_line <= 0) {
		vscroll->set_value(0);
		queue_accessibility_update();
		return;
	}
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		if ((line_count <= p_line) &&
			(line_count + main_fr.lines[i].text_buf->get_line_count() >= p_line)) {
			float line_offset = 0.f;
			for (int j = 0; j < p_line - line_count; j++) {
				line_offset += main_fr.lines[i].text_buf->get_line_ascent(j) +
							   main_fr.lines[i].text_buf->get_line_descent(j) +
							   theme_cache.line_separation;
			}
			vscroll->set_value(main_fr.lines[i].offset.y + line_offset);
			queue_accessibility_update();
			return;
		}
		line_count += main_fr.lines[i].text_buf->get_line_count();
	}
	vscroll->set_value(vscroll->get_max());
	queue_accessibility_update();
}

float RichTextLabel::get_line_offset(int p_line)
{
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		if ((line_count <= p_line) &&
			(p_line <= line_count + main_fr.lines[i].text_buf->get_line_count())) {
			float line_offset = 0.f;
			for (int j = 0; j < p_line - line_count; j++) {
				line_offset += main_fr.lines[i].text_buf->get_line_ascent(j) +
							   main_fr.lines[i].text_buf->get_line_descent(j) +
							   theme_cache.line_separation;
			}
			return main_fr.lines[i].offset.y + line_offset;
		}
		line_count += main_fr.lines[i].text_buf->get_line_count();
	}
	return 0;
}

float RichTextLabel::get_paragraph_offset(int p_paragraph)
{
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int to_line = main_fr.first_invalid_line.load();
	if (0 <= p_paragraph && p_paragraph < to_line) {
		return main_fr.lines[p_paragraph].offset.y;
	}
	return 0;
}

int RichTextLabel::get_line_count() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		line_count += main_fr.lines[i].text_buf->get_line_count();
	}
	return line_count;
}

Vector2i RichTextLabel::get_line_range(int p_line)
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		int lc = main_fr.lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			Vector2i char_offset =
				Vector2i(main_fr.lines[i].char_offset, main_fr.lines[i].char_offset);
			Vector2i line_range = main_fr.lines[i].text_buf->get_line_range(p_line - line_count);
			return char_offset + line_range;
		}

		line_count += lc;
	}
	return Vector2i();
}

int RichTextLabel::get_visible_line_count() const
{
	if (!is_visible()) {
		return 0;
	}
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	return visible_line_count;
}

void RichTextLabel::set_selection_enabled(bool p_enabled)
{
	if (selection.enabled == p_enabled) {
		return;
	}

	selection.enabled = p_enabled;
	if (!p_enabled) {
		if (selection.active) {
			deselect();
		}
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
	else {
		set_focus_mode(FOCUS_ALL);
	}
	queue_accessibility_update();
}

void RichTextLabel::set_deselect_on_focus_loss_enabled(const bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && selection.active && !has_focus()) {
		deselect();
	}
}

bool RichTextLabel::_is_click_inside_selection() const
{
	if (selection.active && selection.enabled && selection.click_frame && selection.from_frame &&
		selection.to_frame) {
		const Line& l_click = selection.click_frame->frame->lines[selection.click_line];
		const Line& l_from = selection.from_frame->frame->lines[selection.from_line];
		const Line& l_to = selection.to_frame->frame->lines[selection.to_line];
		return (l_click.char_offset + selection.click_char >=
				   l_from.char_offset + selection.from_char) &&
			   (l_click.char_offset + selection.click_char <= l_to.char_offset + selection.to_char);
	}
	else {
		return false;
	}
}

bool RichTextLabel::_search_table_cell(Item* p_table, List<Item*>::Element* p_cell,
	const String& p_string, bool p_reverse_search, int p_from_line)
{
	ERR_FAIL_COND_V(p_cell->get()->type != ITEM_FRAME, false); // Children should all be frames.
	Item* frame = p_cell->get();
	if (p_from_line < 0) {
		p_from_line = (int)frame->frame->lines.size() - 1;
	}

	if (p_reverse_search) {
		for (int i = p_from_line; i >= 0; i--) {
			if (_search_line(frame, i, p_string, -1, p_reverse_search)) {
				return true;
			}
		}
	}
	else {
		for (int i = p_from_line; i < (int)frame->frame->lines.size(); i++) {
			if (_search_line(frame, i, p_string, 0, p_reverse_search)) {
				return true;
			}
		}
	}

	return false;
}

bool RichTextLabel::_search_table(
	Item* p_table, List<Item*>::Element* p_from, const String& p_string, bool p_reverse_search)
{
	List<Item*>::Element* E = p_from;
	while (E != nullptr) {
		int from_line = p_reverse_search ? -1 : 0;
		if (_search_table_cell(p_table, E, p_string, p_reverse_search, from_line)) {
			return true;
		}
		E = p_reverse_search ? E->prev() : E->next();
	}
	return false;
}

bool RichTextLabel::_search_line(
	Item* p_frame, int p_line, const String& p_string, int p_char_idx, bool p_reverse_search)
{
	ERR_FAIL_NULL_V(p_frame, false);
	FrameData& fr = *p_frame->frame;
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)fr.lines.size(), false);

	Line& l = fr.lines[p_line];

	String txt;
	Item* it_to =
		(p_line + 1 < (int)fr.lines.size()) ? fr.lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; it && it != it_to; it = _get_next_item(it)) {
		switch (it->type) {
		case ITEM_NEWLINE: {
			txt += "\n";
		} break;
		case ITEM_TEXT: {
			txt += it->text;
		} break;
		case ITEM_IMAGE: {
			txt += " ";
		} break;
		case ITEM_TABLE: {
			List<Item*>::Element* E =
				p_reverse_search ? it->subitems.back() : it->subitems.front();
			if (_search_table(it, E, p_string, p_reverse_search)) {
				return true;
			}
		} break;
		default:
			break;
		}
	}

	int sp = -1;
	if (p_reverse_search) {
		sp = txt.rfindn(p_string, p_char_idx);
	}
	else {
		sp = txt.findn(p_string, p_char_idx);
	}

	if (sp != -1) {
		selection.from_frame = p_frame;
		selection.from_line = p_line;
		selection.from_item = _get_item_at_pos(l.from, it_to, sp);
		selection.from_char = sp;
		selection.to_frame = p_frame;
		selection.to_line = p_line;
		selection.to_item = _get_item_at_pos(l.from, it_to, sp + p_string.length());
		selection.to_char = sp + p_string.length();
		selection.active = true;
		queue_accessibility_update();
		return true;
	}

	return false;
}

String RichTextLabel::_get_line_text(Item* p_frame, int p_line) const
{
	String txt;

	ERR_FAIL_NULL_V(p_frame, txt);
	FrameData& fr = *p_frame->frame;
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)fr.lines.size(), txt);

	int start = 0; // Unless the current line is the `from` line.
	if (!selection.from_line_found && p_frame == selection.from_frame) {
		if (p_line < selection.from_line) {
			return txt; // Skip the lines with smaller line numbers in the same `from` frame.
		}
		selection.from_line_found = true;
		start = selection.from_char;
	}
	const bool from_this =
		selection.from_line_found; // Used to detect cases starting from a sub-frame.

	Line& l = fr.lines[p_line];

	Item* it_to =
		(p_line + 1 < (int)fr.lines.size()) ? fr.lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; !selection.to_line_found && it && it != it_to;
		 it = _get_next_item(it)) {
		if (it->type == ITEM_TABLE) {
			for (Item* E : it->subitems) {
				ERR_CONTINUE(E->type != ITEM_FRAME); // Children should all be frames.
				for (int i = 0; !selection.to_line_found && i < (int)E->frame->lines.size(); i++) {
					txt += _get_line_text(E, i);
				}
				if (selection.to_line_found) {
					break;
				}
			}
			continue;
		}
		if (!selection.from_line_found) {
			continue;
		}
		if (it->type == ITEM_DROPCAP) {
			txt += it->dropcap->text;
		}
		else if (it->type == ITEM_TEXT) {
			txt += it->text;
		}
		else if (it->type == ITEM_NEWLINE) {
			txt += "\n";
		}
		else if (it->type == ITEM_IMAGE) {
			txt += " ";
		}
	}

	if (!selection.from_line_found) {
		return txt; // Empty.
	}

	int chars = -1; // Unless the current line is the `to` line.
	if (p_frame == selection.to_frame && p_line == selection.to_line) {
		selection.to_line_found = true;

		if (from_this ^ selection.from_line_found) {
			// For cases text from child frame to parent frame is considered to be on the same line,
			// even if the line numbers are different. `start` has been reset to 0.
			chars = selection.to_char - selection.from_char;
		}
		else {
			chars = selection.to_char - start;
		}
	}

	txt = txt.substr(start, chars);

	return txt;
}

void RichTextLabel::set_context_menu_enabled(bool p_enabled) { context_menu_enabled = p_enabled; }

bool RichTextLabel::is_context_menu_enabled() const { return context_menu_enabled; }

void RichTextLabel::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool RichTextLabel::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

PopupMenu* RichTextLabel::get_menu() const
{
	if (!menu) {
		const_cast<RichTextLabel*>(this)->_generate_context_menu();
	}
	return menu;
}

bool RichTextLabel::is_menu_visible() const { return menu && menu->is_visible(); }

bool RichTextLabel::is_selection_enabled() const { return selection.enabled; }

bool RichTextLabel::is_deselect_on_focus_loss_enabled() const
{
	return deselect_on_focus_loss_enabled;
}

void RichTextLabel::set_drag_and_drop_selection_enabled(const bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool RichTextLabel::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

int RichTextLabel::get_selection_from() const
{
	if (!selection.active || !selection.enabled) {
		return -1;
	}

	return selection.from_frame->frame->lines[selection.from_line].char_offset +
		   selection.from_char;
}

int RichTextLabel::get_selection_to() const
{
	if (!selection.active || !selection.enabled) {
		return -1;
	}

	return selection.to_frame->frame->lines[selection.to_line].char_offset + selection.to_char - 1;
}

float RichTextLabel::get_selection_line_offset() const
{
	if (selection.active && selection.from_frame && selection.from_line >= 0 &&
		selection.from_line < (int)selection.from_frame->frame->lines.size()) {
		const Line& from_l = selection.from_frame->frame->lines[selection.from_line];
		// Selected frame paragraph offset.
		float line_offset = from_l.offset.y;

		// Add wrapped line offset.
		for (int i = 0; i < from_l.text_buf->get_line_count(); i++) {
			Vector2i range = from_l.text_buf->get_line_range(i);
			if (range.x <= selection.from_char && range.y >= selection.from_char) {
				break;
			}
			line_offset += from_l.text_buf->get_line_ascent(i) +
						   from_l.text_buf->get_line_descent(i) + theme_cache.line_separation;
		}

		// Add nested frame (e.g. table cell) offset.
		Item* it = selection.from_frame;
		while (it->frame->parent_frame != nullptr) {
			line_offset += it->frame->parent_frame->frame->lines[it->line].offset.y;
			it = it->frame->parent_frame;
		}
		return line_offset;
	}

	return -1.0;
}

void RichTextLabel::set_text(const String& p_bbcode)
{
	// Allow clearing the tag stack.
	if (!p_bbcode.is_empty() && text == p_bbcode) {
		return;
	}

	stack_externally_modified = false;

	text = p_bbcode;
	if (text.is_empty()) {
		clear();
	}
	else {
		_apply_translation();
	}
}

String RichTextLabel::get_text() const { return text; }

bool RichTextLabel::is_using_bbcode() const { return use_bbcode; }

String RichTextLabel::get_parsed_text() const
{
	String txt;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_DROPCAP) {
			txt += it->dropcap->text;
		}
		else if (it->type == ITEM_TEXT) {
			txt += it->text;
		}
		else if (it->type == ITEM_NEWLINE) {
			txt += "\n";
		}
		else if (it->type == ITEM_IMAGE) {
			txt += " ";
		}
		else if (it->type == ITEM_INDENT || it->type == ITEM_LIST) {
			txt += "\t";
		}
		it = _get_next_item(it, true);
	}
	return txt;
}

Control::TextDirection RichTextLabel::get_text_direction() const { return text_direction; }

HorizontalAlignment RichTextLabel::get_horizontal_alignment() const { return default_alignment; }

VerticalAlignment RichTextLabel::get_vertical_alignment() const { return vertical_alignment; }

uint32_t RichTextLabel::get_justification_flags() const { return default_jst_flags; }

PackedFloat32Array RichTextLabel::get_tab_stops() const { return default_tab_stops; }

TextServer::StructuredTextParser RichTextLabel::get_structured_text_bidi_override() const
{
	return st_parser;
}

String RichTextLabel::get_language() const { return language; }

TextServer::AutowrapMode RichTextLabel::get_autowrap_mode() const { return autowrap_mode; }

uint32_t RichTextLabel::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

float RichTextLabel::get_visible_ratio() const { return visible_ratio; }

int RichTextLabel::get_content_height() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int total_height = 0;
	int to_line = main_fr.first_invalid_line.load();
	if (to_line) {
		MutexLock lock(main_fr.lines[to_line - 1].text_buf->get_mutex());
		if (theme_cache.line_separation < 0) {
			// Do not apply to the last line to avoid cutting text.
			total_height = main_fr.lines[to_line - 1].offset.y +
						   main_fr.lines[to_line - 1].text_buf->get_size().y +
						   (main_fr.lines[to_line - 1].text_buf->get_line_count() - 1) *
							   theme_cache.line_separation;
		}
		else {
			total_height =
				main_fr.lines[to_line - 1].offset.y +
				main_fr.lines[to_line - 1].text_buf->get_size().y +
				main_fr.lines[to_line - 1].text_buf->get_line_count() * theme_cache.line_separation +
				theme_cache.paragraph_separation;
		}
	}
	return total_height;
}

Rect2i RichTextLabel::get_visible_content_rect() const { return visible_rect; }

int RichTextLabel::get_content_width() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int total_width = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		total_width =
			MAX(total_width, main_fr.lines[i].offset.x + main_fr.lines[i].text_buf->get_size().x);
	}
	return total_width;
}

int RichTextLabel::get_line_height(int p_line) const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		int lc = main_fr.lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			const Ref<TextParagraph> text_buf = main_fr.lines[i].text_buf;
			return text_buf->get_line_ascent(p_line - line_count) +
				   text_buf->get_line_descent(p_line - line_count) + theme_cache.line_separation;
		}
		line_count += lc;
	}
	return 0;
}

int RichTextLabel::get_line_width(int p_line) const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		int lc = main_fr.lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			return main_fr.lines[i].text_buf->get_line_width(p_line - line_count);
		}
		line_count += lc;
	}
	return 0;
}

TextServer::VisibleCharactersBehavior RichTextLabel::get_visible_characters_behavior() const
{
	return visible_chars_behavior;
}

int RichTextLabel::get_visible_characters() const { return visible_characters; }

int RichTextLabel::get_character_line(int p_char)
{
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int line_count = 0;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
		int char_offset = main_fr.lines[i].char_offset;
		int char_count = main_fr.lines[i].char_count;
		if (char_offset <= p_char && p_char < char_offset + char_count) {
			int lc = main_fr.lines[i].text_buf->get_line_count();
			for (int j = 0; j < lc; j++) {
				Vector2i range = main_fr.lines[i].text_buf->get_line_range(j);
				if (char_offset + range.x <= p_char && p_char < char_offset + range.y) {
					break;
				}
				if (char_offset + range.x > p_char && line_count > 0) {
					line_count--; // Character is not rendered and is between the lines (e.g., edge
								  // space).
					break;
				}
				if (j != lc - 1) {
					line_count++;
				}
			}
			return line_count;
		}
		else {
			line_count += main_fr.lines[i].text_buf->get_line_count();
		}
	}
	return -1;
}

int RichTextLabel::get_character_paragraph(int p_char)
{
	_validate_line_caches();

	FrameData& main_fr = *main->frame;
	int to_line = main_fr.first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		int char_offset = main_fr.lines[i].char_offset;
		if (char_offset <= p_char && p_char < char_offset + main_fr.lines[i].char_count) {
			return i;
		}
	}
	return -1;
}

int RichTextLabel::get_total_character_count() const
{
	// Note: Do not use line buffer "char_count", it includes only visible characters.
	int tc = 0;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_TEXT) {
			tc += it->text.length();
		}
		else if (it->type == ITEM_NEWLINE) {
			tc++;
		}
		else if (it->type == ITEM_IMAGE) {
			tc++;
		}
		it = _get_next_item(it, true);
	}

	return tc;
}

int RichTextLabel::get_total_glyph_count() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int tg = 0;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_FRAME) {
			FrameData& f = *it->frame;
			for (int i = 0; i < (int)f.lines.size(); i++) {
				MutexLock lock(f.lines[i].text_buf->get_mutex());
				tg += TS->shaped_text_get_glyph_count(f.lines[i].text_buf->get_rid());
			}
		}
		it = _get_next_item(it, true);
	}

	return tg;
}

Size2 RichTextLabel::get_minimum_size() const
{
	Size2 sb_min_size = theme_cache.normal_style->get_minimum_size();
	Size2 min_size;
	bool wrap_with_max_width =
		autowrap_mode != TextServer::AUTOWRAP_OFF && get_combined_maximum_size().x > 0.0;

	if (fit_content) {
		if (!wrap_with_max_width) {
			min_size.x = get_content_width();
		}
		min_size.y = get_content_height();
	}

	if (wrap_with_max_width) {
		const_cast<RichTextLabel*>(this)->_validate_line_caches();

		FrameData& main_fr = *main->frame;
		int natural_width = 0;
		int to_line = main_fr.first_invalid_line.load();
		for (int i = 0; i < to_line; i++) {
			MutexLock lock(main_fr.lines[i].text_buf->get_mutex());
			natural_width = MAX(natural_width,
				int(Math::ceil(main_fr.lines[i].offset.x +
							   main_fr.lines[i].text_buf->get_non_wrapped_size().x)));
		}

		int maximum_width = int(get_combined_maximum_size().x - sb_min_size.width);
		if (maximum_width <= 0) {
			maximum_width = 1;
		}
		min_size.x = MIN(natural_width, maximum_width);
	}

	return sb_min_size +
		   ((autowrap_mode != TextServer::AUTOWRAP_OFF)
				   ? Size2(wrap_with_max_width ? MAX(1, min_size.x) : 1, min_size.height)
				   : min_size);
}

Key RichTextLabel::_get_menu_action_accelerator(const String& p_action)
{
	const List<InputEvent>* events = InputMap::action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<InputEvent>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const InputEvent& event = first_event->get();
	if (event.type != InputEventType::KEY) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero
	if (event.key.physical_keycode != Key::NONE) {
		return event.get_physical_keycode_with_modifiers();
	}
	else {
		return event.get_keycode_with_modifiers();
	}
}

void RichTextLabel::menu_option(int p_option)
{
	switch (p_option) {
	case MENU_COPY: {
		String txt = get_selected_text();
		if (txt.is_empty()) {
			txt = get_parsed_text();
		}

		if (!txt.is_empty()) {
			DisplayServer::clipboard_set(txt);
		}
	} break;
	case MENU_SELECT_ALL: {
		select_all();
	} break;
	}
}

void RichTextLabel::add_text(const String& p_text) {}

void RichTextLabel::wait_until_finished() {}

bool RichTextLabel::_validate_line_caches() { return true; }

void RichTextLabel::clear() {}

void RichTextLabel::_push_def_font(DefaultFont p_def_font) {}

void RichTextLabel::deselect() {}

void RichTextLabel::set_use_bbcode(bool) {}

void RichTextLabel::select_all() {}

bool RichTextLabel::search(String const&, bool, bool) { return true; }

RichTextLabel::RichTextLabel(String const&) {}

void RichTextLabel::push_font(Ref<Font> const&, int) {}

String RichTextLabel::get_selected_text() const { return String(); }

void RichTextLabel::_generate_context_menu() {}

String RichTextLabel::_find_language(RichTextLabel::Item*) { return String(); }

float RichTextLabel::_find_click_in_line(RichTextLabel::Item*, int, Vector2 const&, int, float,
	Vector2i const&, RichTextLabel::Item**, int*, RichTextLabel::Item**, int*, bool, bool)
{
	return 0.0;
}

void RichTextLabel::_apply_translation() {}

void RichTextLabel::append_text(String const&) {}

Control::CursorShape RichTextLabel::get_cursor_shape(const Vector2& p_pos) const
{
	return Control::CURSOR_ARROW;
}

// --- rich_text_label.compat.inc ---

#ifndef DISABLE_DEPRECATED


void RichTextLabel::_push_font_bind_compat_79053(const Ref<Font>& p_font, int p_size)
{
	push_font(p_font, p_size);
}

#endif // DISABLE_DEPRECATED

// --- rich_text_effect.cpp ---

CharFXTransform::CharFXTransform() {}

RichTextEffect::RichTextEffect() {}

bool RichTextEffect::_process_effect_impl(Ref<CharFXTransform> p_cfx) { return false; }


// --- Merged from scene/gui/text_edit.cpp ---
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/string/ustring.h"
#include "scene/gui/dialogs.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"
#include "scene/gui/dialogs.h"

void TextEdit::Text::set_font(const Ref<Font>& p_font)
{
	if (font == p_font) {
		return;
	}
	font = p_font;
	is_dirty = true;
}

void TextEdit::Text::set_font_size(int p_font_size)
{
	if (font_size == p_font_size) {
		return;
	}
	font_size = p_font_size;
	is_dirty = true;
}

void TextEdit::Text::set_tab_size(int p_tab_size)
{
	if (tab_size == p_tab_size) {
		return;
	}
	tab_size = p_tab_size;
	tab_size_dirty = true;
}

int TextEdit::Text::get_tab_size() const { return tab_size; }

void TextEdit::Text::set_indent_wrapped_lines(bool p_enabled)
{
	if (indent_wrapped_lines == p_enabled) {
		return;
	}
	indent_wrapped_lines = p_enabled;
	tab_size_dirty = true;
}

bool TextEdit::Text::is_indent_wrapped_lines() const { return indent_wrapped_lines; }

void TextEdit::Text::set_direction_and_language(
	TextServer::Direction p_direction, const String& p_language)
{
	if (direction == p_direction && language == p_language) {
		return;
	}
	direction = p_direction;
	language = p_language;
	is_dirty = true;
}

void TextEdit::Text::set_draw_control_chars(bool p_enabled)
{
	if (draw_control_chars == p_enabled) {
		return;
	}
	draw_control_chars = p_enabled;
	is_dirty = true;
}

int TextEdit::Text::get_line_width(int p_line, int p_wrap_index) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	if (p_wrap_index != -1) {
		return text[p_line].data_buf->get_line_width(p_wrap_index);
	}
	return text[p_line].data_buf->get_size().x;
}

int TextEdit::Text::get_max_width() const
{
	if (max_line_width_dirty) {
		int new_max_line_width = 0;
		for (const Line& l : text) {
			if (l.hidden) {
				continue;
			}
			new_max_line_width = MAX(new_max_line_width, l.width);
		}
		max_line_width = new_max_line_width;
	}

	return max_line_width;
}

int TextEdit::Text::get_line_height() const
{
	if (max_line_height_dirty) {
		int new_max_line_height = 0;
		for (const Line& l : text) {
			if (l.hidden) {
				continue;
			}
			new_max_line_height = MAX(new_max_line_height, l.height);
		}
		max_line_height = new_max_line_height;
	}

	return max_line_height;
}

void TextEdit::Text::set_width(float p_width) { width = p_width; }

float TextEdit::Text::get_width() const { return width; }

void TextEdit::Text::set_brk_flags(uint32_t p_flags) { brk_flags = p_flags; }

uint32_t TextEdit::Text::get_brk_flags() const { return brk_flags; }

int TextEdit::Text::get_line_wrap_amount(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	return text[p_line].line_count - 1;
}

Vector<Vector2i> TextEdit::Text::get_line_wrap_ranges(int p_line) const
{
	Vector<Vector2i> ret;
	ERR_FAIL_INDEX_V(p_line, text.size(), ret);

	Ref<TextParagraph> data_buf = text[p_line].data_buf;
	int line_count = data_buf->get_line_count();
	for (int i = 0; i < line_count; i++) {
		ret.push_back(data_buf->get_line_range(i));
	}
	return ret;
}

const Ref<TextParagraph> TextEdit::Text::get_line_data(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Ref<TextParagraph>());
	return text[p_line].data_buf;
}

float TextEdit::Text::get_indent_offset(int p_line, bool p_rtl) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	Line& text_line = text.write[p_line];
	if (text_line.indent_ofs < 0.0) {
		int char_count = 0;
		int line_length = text_line.data.size();
		for (int i = 0; i < line_length - 1; i++) {
			if (text_line.data[i] == '\t') {
				char_count++;
			}
			else if (text_line.data[i] == ' ') {
				char_count++;
			}
			else {
				break;
			}
		}
		RID text_rid = text_line.data_buf->get_line_rid(0);
		float offset = (p_rtl) ? TS->shaped_text_get_size(text_rid).x : 0;
		Vector<Vector2> sel = TS->shaped_text_get_selection(text_rid, 0, char_count);
		for (const Vector2 v : sel) {
			if (p_rtl) {
				offset = MIN(v.x, MIN(v.y, offset));
			}
			else {
				offset = MAX(v.x, MAX(v.y, offset));
			}
		}
		text_line.indent_ofs = (p_rtl) ? TS->shaped_text_get_size(text_rid).x - offset : offset;
	}
	return text_line.indent_ofs;
}

_FORCE_INLINE_ const String& TextEdit::Text::operator[](int p_line) const
{
	static const String empty;
	ERR_FAIL_INDEX_V(p_line, text.size(), empty);
	return text[p_line].data;
}

_FORCE_INLINE_ const String& TextEdit::Text::get_text_with_ime(int p_line) const
{
	if (!text[p_line].ime_data.is_empty()) {
		return text[p_line].ime_data;
	}
	else {
		return text[p_line].data;
	}
}

const Vector<RID> TextEdit::Text::get_accessibility_elements(int p_line)
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Vector<RID>());

	return text[p_line].accessibility_text_root_element;
}

void TextEdit::Text::update_accessibility(int p_line, RID p_root)
{
	ERR_FAIL_INDEX(p_line, text.size());

	Line& l = text.write[p_line];
	if (l.accessibility_text_root_element.is_empty()) {
		for (int i = 0; i < l.data_buf->get_line_count(); i++) {
			bool is_last_line =
				(p_line == text.size() - 1) && (i == l.data_buf->get_line_count() - 1);
			RID rid = AccessibilityServer::create_sub_text_edit_elements(
				p_root, l.data_buf->get_line_rid(i), max_line_height, p_line, is_last_line);
			l.accessibility_text_root_element.push_back(rid);
		}
	}
}

void TextEdit::Text::invalidate_all_lines()
{
	for (int i = 0; i < text.size(); i++) {
		if (tab_size_dirty) {
			if (tab_size > 0) {
				Vector<float> tabs;
				tabs.push_back(MAX(1, (font->get_char_size(' ', font_size).width +
										  font->get_spacing(TextServer::SPACING_SPACE)) *
										  tab_size));
				text[i].data_buf->tab_align(tabs);
			}
		}
		invalidate_cache(i, false);
	}
	tab_size_dirty = false;
}

void TextEdit::_selection_changed(int p_caret) {}

void TextEdit::set_text_direction(TextDirection p_text_direction) {}

void TextEdit::Text::invalidate_cache(int p_line, bool p_text_changed) {}

void TextEdit::Text::invalidate_font()
{
	if (!is_dirty) {
		return;
	}

	max_line_width_dirty = true;
	max_line_height_dirty = true;

	if (font.is_valid() && font_size > 0) {
		font_height = font->get_height(font_size);
	}

	for (int i = 0; i < text.size(); i++) {
		invalidate_cache(i, false);
	}
	is_dirty = false;
}

void TextEdit::Text::invalidate_all()
{
	if (!is_dirty) {
		return;
	}

	max_line_width_dirty = true;
	max_line_height_dirty = true;

	if (font.is_valid() && font_size > 0) {
		font_height = font->get_height(font_size);
	}

	for (int i = 0; i < text.size(); i++) {
		invalidate_cache(i, true);
	}
	is_dirty = false;
}

void TextEdit::Text::clear()
{
	text.clear();

	max_line_width_dirty = true;
	max_line_height_dirty = true;
	total_visible_line_count = 0;

	Line line;
	line.gutters.resize(gutter_count);
	text.insert(0, line);
	invalidate_cache(0, true);
}

int TextEdit::Text::get_total_visible_line_count() const { return total_visible_line_count; }

void TextEdit::Text::set_hidden(int p_line, bool p_hidden)
{
	ERR_FAIL_INDEX(p_line, text.size());

	Line& text_line = text.write[p_line];
	if (text_line.hidden == p_hidden) {
		return;
	}
	text_line.hidden = p_hidden;
	if (p_hidden) {
		total_visible_line_count -= text_line.line_count;
		if (text_line.width == max_line_width) {
			max_line_width_dirty = true;
		}
		if (text_line.height == max_line_height) {
			max_line_height_dirty = true;
		}
	}
	else {
		total_visible_line_count += text_line.line_count;
		max_line_width = MAX(text_line.width, max_line_width);
		max_line_height = MAX(text_line.height, max_line_height);
	}
}

bool TextEdit::Text::is_hidden(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), true);
	return text[p_line].hidden;
}

void TextEdit::Text::remove_range(int p_from_line, int p_to_line)
{
	if (p_from_line == p_to_line) {
		return;
	}

	for (int i = p_from_line + 1; i <= p_to_line; i++) {
		const Line& text_line = text[i];
		if (text_line.hidden) {
			continue;
		}

		if (text_line.height == max_line_height) {
			max_line_height_dirty = true;
		}
		if (text_line.width == max_line_width) {
			max_line_width_dirty = true;
		}
		total_visible_line_count -= text_line.line_count;
	}

	int diff = p_to_line - p_from_line;
	for (int i = p_to_line + 1; i < text.size(); i++) {
		text.write[i - diff] = text[i];
	}
	text.resize(text.size() - diff);

	ERR_FAIL_COND(total_visible_line_count < 0); // BUG
}

void TextEdit::Text::add_gutter(int p_at)
{
	for (int i = 0; i < text.size(); i++) {
		if (p_at < 0 || p_at > gutter_count) {
			text.write[i].gutters.push_back(Gutter());
		}
		else {
			text.write[i].gutters.insert(p_at, Gutter());
		}
	}
	gutter_count++;
}

void TextEdit::Text::remove_gutter(int p_gutter)
{
	ERR_FAIL_INDEX(p_gutter, text.size());

	for (int i = 0; i < text.size(); i++) {
		text.write[i].gutters.remove_at(p_gutter);
	}
	gutter_count--;
}

void TextEdit::Text::move_gutters(int p_from_line, int p_to_line)
{
	ERR_FAIL_INDEX(p_from_line, text.size());
	ERR_FAIL_INDEX(p_to_line, text.size());

	text.write[p_to_line].gutters = text[p_from_line].gutters;
	text.write[p_from_line].gutters.clear();
	text.write[p_from_line].gutters.resize(gutter_count);
}

void TextEdit::Text::set_use_default_word_separators(bool p_enabled)
{
	if (use_default_word_separators == p_enabled) {
		return;
	}
	use_default_word_separators = p_enabled;
	invalidate_all_lines();
}

void TextEdit::Text::set_use_custom_word_separators(bool p_enabled)
{
	if (use_custom_word_separators == p_enabled) {
		return;
	}
	use_custom_word_separators = p_enabled;
	invalidate_all_lines();
}

bool TextEdit::Text::is_default_word_separators_enabled() const
{
	return use_default_word_separators;
}

bool TextEdit::Text::is_custom_word_separators_enabled() const
{
	return use_custom_word_separators;
}

String TextEdit::Text::get_custom_word_separators() const { return custom_word_separators; }

String TextEdit::Text::get_default_word_separators() const
{
	String concat_separators = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
	for (char32_t ch = 0x2000; ch <= 0x206F; ++ch) { // General punctuation block.
		concat_separators += ch;
	}
	for (char32_t ch = 0x3000; ch <= 0x303F; ++ch) { // CJK punctuation block.
		concat_separators += ch;
	}
	return concat_separators;
}

String TextEdit::Text::get_enabled_word_separators() const
{
	String all_separators;
	if (use_default_word_separators) {
		all_separators += get_default_word_separators();
	}
	if (use_custom_word_separators) {
		all_separators += get_custom_word_separators();
	}
	return all_separators;
}

Ref<StyleBox> TextEdit::_get_current_stylebox() const
{
	return editable ? theme_cache.style_normal : theme_cache.style_readonly;
}

void TextEdit::_draw_selection_handle(Vector2 p_pos) const
{
	Color handle_color = theme_cache.caret_color;
	int line_height = get_line_height();

	int handle_line_width = theme_cache.caret_width * MAX(1, theme_cache.base_scale);
	Renderer::canvas_item_add_line(
		text_ci, p_pos, p_pos + Vector2(0, line_height), handle_color, handle_line_width);

	Vector2 circle_center = p_pos + Vector2(0, line_height + selection_handle_radius);
	Renderer::canvas_item_add_circle(
		text_ci, circle_center, selection_handle_radius, handle_color);
}

bool TextEdit::_is_first_column(int p_line, int p_column) const
{
	bool is_first_column = p_column == 0;
	if (!is_first_column && is_line_wrapped(p_line)) {
		int wrap_index = get_line_wrap_index_at_column(p_line, p_column);
		int wrap_index_last_column = get_line_wrap_index_at_column(p_line, p_column - 1);
		is_first_column = wrap_index != wrap_index_last_column;
	}
	return is_first_column;
}

Vector<Point2i> TextEdit::_get_selection_handles_pos(int p_caret) const
{
	int selection_from_line = get_selection_from_line(p_caret);
	int selection_from_column = get_selection_from_column(p_caret);
	int selection_to_line = get_selection_to_line(p_caret);
	int selection_to_column = get_selection_to_column(p_caret);
	Rect2i start_rect = get_rect_at_line_column(selection_from_line, selection_from_column);
	Rect2i end_rect = get_rect_at_line_column(selection_to_line, selection_to_column);

	Point2i start_pos = start_rect.position;
	if (start_pos.x != -1 && !_is_first_column(selection_from_line, selection_from_column)) {
		start_pos.x += start_rect.size.x;
	}

	Point2i end_pos = end_rect.position;
	if (end_pos.x != -1 && !_is_first_column(selection_to_line, selection_to_column)) {
		end_pos.x += end_rect.size.x;
	}

	Vector<Point2i> result;
	result.push_back(start_pos);
	result.push_back(end_pos);
	return result;
}

void TextEdit::_cancel_inertial_scroll()
{
	set_process_internal(false);
	touch_dragging_deaccel = false;
	drag_speed = Vector2();
	drag_accum = Vector2();
	last_drag_accum = Vector2();
	drag_from = Vector2();
}

void TextEdit::_move_caret_left(bool p_select, bool p_move_by_word)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		// Handle selection.
		if (p_select) {
			_pre_shift_selection(i);
		}
		else if (has_selection(i) && !p_move_by_word) {
			// If a selection is active, move caret to start of selection.
			set_caret_line(get_selection_from_line(i), false, true, -1, i);
			set_caret_column(get_selection_from_column(i), i == 0, i);
			deselect(i);
			continue;
		}
		else {
			deselect(i);
		}

		if (get_caret_column(i) == 0) {
			if (get_caret_line(i) == 0) {
				continue;
			}
			// If the caret is at the start of the line, and not on the first line, move it up to
			// the end of the previous line.
			int new_caret_line =
				get_caret_line(i) - get_next_visible_line_offset_from(get_caret_line(i) - 1, -1);
			set_caret_line(new_caret_line, false, true, -1, i);
			set_caret_column(text[get_caret_line(i)].length(), i == 0, i);
		}
		else if (p_move_by_word) {
			int caret_column = get_caret_column(i);
			const PackedInt32Array words =
				TS->shaped_text_get_word_breaks(text.get_line_data(get_caret_line(i))->get_rid());
			if (words.is_empty() || caret_column <= words[0]) {
				// Move to the start when there are no more words.
				caret_column = 0;
			}
			else {
				for (int j = words.size() - 2; j >= 0; j = j - 2) {
					if (words[j] < caret_column) {
						caret_column = words[j];
						break;
					}
				}
			}
			set_caret_column(caret_column, i == 0, i);
		}
		else {
			if (caret_mid_grapheme_enabled) {
				set_caret_column(get_caret_column(i) - 1, i == 0, i);
			}
			else {
				set_caret_column(
					TS->shaped_text_prev_character_pos(
						text.get_line_data(get_caret_line(i))->get_rid(), get_caret_column(i)),
					i == 0, i);
			}
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_right(bool p_select, bool p_move_by_word)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		// Handle selection.
		if (p_select) {
			_pre_shift_selection(i);
		}
		else if (has_selection(i) && !p_move_by_word) {
			// If a selection is active, move caret to end of selection.
			set_caret_line(get_selection_to_line(i), false, true, -1, i);
			set_caret_column(get_selection_to_column(i), i == 0, i);
			deselect(i);
			continue;
		}
		else {
			deselect(i);
		}

		if (get_caret_column(i) == text[get_caret_line(i)].length()) {
			if (get_caret_line(i) >= text.size() - 1 ||
				get_caret_line(i) == get_last_unhidden_line()) {
				continue;
			}
			// If the caret is at the end of the line, and not on the last line, move it down to the
			// beginning of the next line.
			int new_caret_line =
				get_caret_line(i) + get_next_visible_line_offset_from(get_caret_line(i) + 1, 1);
			set_caret_line(new_caret_line, false, true, -1, i);
			set_caret_column(0, i == 0, i);
		}
		else if (p_move_by_word) {
			int caret_column = get_caret_column(i);
			const PackedInt32Array words =
				TS->shaped_text_get_word_breaks(text.get_line_data(get_caret_line(i))->get_rid());
			if (words.is_empty() || caret_column >= words[words.size() - 1]) {
				// Move to the end when there are no more words.
				caret_column = text[get_caret_line(i)].length();
			}
			else {
				for (int j = 1; j < words.size(); j = j + 2) {
					if (words[j] > caret_column) {
						caret_column = words[j];
						break;
					}
				}
			}
			set_caret_column(caret_column, i == 0, i);
		}
		else {
			if (caret_mid_grapheme_enabled) {
				set_caret_column(get_caret_column(i) + 1, i == 0, i);
			}
			else {
				set_caret_column(
					TS->shaped_text_next_character_pos(
						text.get_line_data(get_caret_line(i))->get_rid(), get_caret_column(i)),
					i == 0, i);
			}
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_up(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		int cur_wrap_index = get_caret_wrap_index(i);
		if (cur_wrap_index > 0) {
			set_caret_line(get_caret_line(i), true, false, cur_wrap_index - 1, i);
		}
		else if (get_caret_line(i) == 0) {
			set_caret_column(0, i == 0, i);
		}
		else {
			int new_line =
				get_caret_line(i) - get_next_visible_line_offset_from(get_caret_line(i) - 1, -1);
			if (is_line_wrapped(new_line)) {
				set_caret_line(new_line, i == 0, false, get_line_wrap_count(new_line), i);
			}
			else {
				set_caret_line(new_line, i == 0, false, 0, i);
			}
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_down(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		int cur_wrap_index = get_caret_wrap_index(i);
		if (cur_wrap_index < get_line_wrap_count(get_caret_line(i))) {
			set_caret_line(get_caret_line(i), i == 0, false, cur_wrap_index + 1, i);
		}
		else if (get_caret_line(i) == get_last_unhidden_line()) {
			set_caret_column(text[get_caret_line(i)].length());
		}
		else {
			int new_line =
				get_caret_line(i) + get_next_visible_line_offset_from(
										CLAMP(get_caret_line(i) + 1, 0, text.size() - 1), 1);
			set_caret_line(new_line, i == 0, false, 0, i);
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_to_line_start(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		// Move caret column to start of wrapped row and then to start of text.
		Vector<String> rows = get_line_wrapped_text(get_caret_line(i));
		int wi = get_caret_wrap_index(i);
		int row_start_col = 0;
		for (int j = 0; j < wi; j++) {
			row_start_col += rows[j].length();
		}
		if (get_caret_column(i) == row_start_col || wi == 0) {
			// Compute whitespace symbols sequence length.
			int current_line_whitespace_len = get_first_non_whitespace_column(get_caret_line(i));
			if (get_caret_column(i) == current_line_whitespace_len) {
				set_caret_column(0, i == 0, i);
			}
			else {
				set_caret_column(current_line_whitespace_len, i == 0, i);
			}
		}
		else {
			set_caret_column(row_start_col, i == 0, i);
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_to_line_end(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		// Move caret column to end of wrapped row and then to end of text.
		Vector<String> rows = get_line_wrapped_text(get_caret_line(i));
		int wi = get_caret_wrap_index(i);
		int row_end_col = -1;
		for (int j = 0; j < wi + 1; j++) {
			row_end_col += rows[j].length();
		}
		if (wi == rows.size() - 1 || get_caret_column(i) == row_end_col) {
			set_caret_column(text[get_caret_line(i)].length(), i == 0, i);
		}
		else {
			set_caret_column(row_end_col, i == 0, i);
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_page_up(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		Point2i next_line = get_next_visible_line_index_offset_from(
			get_caret_line(i), get_caret_wrap_index(i), -get_visible_line_count());
		int n_line = get_caret_line(i) - next_line.x + 1;
		set_caret_line(n_line, i == 0, false, next_line.y, i);
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_page_down(bool p_select)
{
	_push_current_op();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_select) {
			_pre_shift_selection(i);
		}
		else {
			deselect(i);
		}

		Point2i next_line = get_next_visible_line_index_offset_from(
			get_caret_line(i), get_caret_wrap_index(i), get_visible_line_count());
		int n_line = get_caret_line(i) + next_line.x - 1;
		set_caret_line(n_line, i == 0, false, next_line.y, i);
	}
	merge_overlapping_carets();
}

void TextEdit::_move_caret_document_start(bool p_select)
{
	remove_secondary_carets();
	if (p_select) {
		_pre_shift_selection(0);
	}
	else {
		deselect();
	}

	set_caret_line(0, false, true, -1);
	set_caret_column(0);
}

void TextEdit::_move_caret_document_end(bool p_select)
{
	remove_secondary_carets();
	if (p_select) {
		_pre_shift_selection(0);
	}
	else {
		deselect();
	}

	set_caret_line(get_last_unhidden_line(), true, false, -1);
	set_caret_column(text[get_caret_line()].length());
}

bool TextEdit::_clear_carets_and_selection()
{
	_push_current_op();
	if (get_caret_count() > 1) {
		remove_secondary_carets();
		return true;
	}

	if (has_selection()) {
		deselect();
		return true;
	}

	return false;
}

bool TextEdit::_using_placeholder() const
{
	return text.size() == 1 && text[0].is_empty() && ime_text.is_empty();
}

void TextEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
	use_selected_font_color = theme_cache.font_selected_color != Color(0, 0, 0, 0);

	if (text.get_line_height() + theme_cache.line_spacing < 1) {
		WARN_PRINT("Line height is too small, please increase font_size and/or line_spacing");
	}

	// The value was chosen after trying a few different radii.
	// 10.0 provided the best balance between being easy to grab without making the touch area feel
	// too large.
	selection_handle_radius = 10.0 * theme_cache.base_scale;
}

void TextEdit::_close_ime_window()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::window_set_ime_position(Point2(), wid);
	DisplayServer::window_set_ime_active(false, wid);
}

void TextEdit::_update_ime_window_position()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	if (!editable) {
		DisplayServer::window_set_ime_active(false, wid);
		return;
	}
	DisplayServer::window_set_ime_active(true, wid);
	Point2 pos = get_global_position() + get_caret_draw_pos();
	if (get_window()->get_embedder()) {
		pos += get_viewport()->get_popup_base_transform().get_origin();
	}
	// Take into account the window's transform.
	pos = get_window()->get_screen_transform().xform(pos);
	// The window will move to the updated position the next time the IME is updated, not
	// immediately.
	DisplayServer::window_set_ime_position(pos, wid);
}

void TextEdit::_caret_changed(int p_caret) {}

void TextEdit::_show_virtual_keyboard()
{
	_update_ime_window_position();

	if (virtual_keyboard_enabled &&
		DisplayServer::has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD)) {
		int caret_start = -1;
		int caret_end = -1;

		if (!has_selection(0)) {
			String full_text = _base_get_text(0, 0, get_caret_line(), get_caret_column());

			caret_start = full_text.length();
		}
		else {
			String pre_text =
				_base_get_text(0, 0, get_selection_from_line(), get_selection_from_column());
			String post_text = get_selected_text(0);

			caret_start = pre_text.length();
			caret_end = caret_start + post_text.length();
		}

		DisplayServer::virtual_keyboard_show(get_text(), get_global_rect(),
			DisplayServerEnums::KEYBOARD_TYPE_MULTILINE, -1, caret_start, caret_end);
	}
}

Size2 TextEdit::get_minimum_size() const
{
	Size2 ms = _get_current_stylebox()->get_minimum_size();
	if (fit_content_height) {
		ms.height += content_size_cache.height;
	}
	if (fit_content_width) {
		ms.width += content_size_cache.width;
	}
	return ms;
}

bool TextEdit::is_text_field() const { return true; }

bool TextEdit::has_ime_text() const { return !ime_text.is_empty(); }

void TextEdit::cancel_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}
	ime_text = String();
	ime_selection = Vector2i();
	alt_start = false;
	alt_start_no_hold = false;
	_close_ime_window();
	_update_ime_text();
}

bool TextEdit::is_editable() const { return editable; }

Control::TextDirection TextEdit::get_text_direction() const { return text_direction; }

String TextEdit::get_language() const { return language; }

TextServer::StructuredTextParser TextEdit::get_structured_text_bidi_override() const
{
	return st_parser;
}

int TextEdit::get_tab_size() const { return text.get_tab_size(); }

bool TextEdit::is_indent_wrapped_lines() const { return text.is_indent_wrapped_lines(); }

void TextEdit::set_tab_input_mode(bool p_enabled) { tab_input_mode = p_enabled; }

bool TextEdit::get_tab_input_mode() const { return tab_input_mode; }

bool TextEdit::is_overtype_mode_enabled() const { return overtype_mode; }

void TextEdit::set_context_menu_enabled(bool p_enabled) { context_menu_enabled = p_enabled; }

bool TextEdit::is_context_menu_enabled() const { return context_menu_enabled; }

void TextEdit::show_emoji_and_symbol_picker()
{
	_update_ime_window_position();
	DisplayServer::show_emoji_and_symbol_picker();
}

void TextEdit::set_emoji_menu_enabled(bool p_enabled)
{
	if (emoji_menu_enabled != p_enabled) {
		emoji_menu_enabled = p_enabled;
	}
}

bool TextEdit::is_emoji_menu_enabled() const { return emoji_menu_enabled; }

void TextEdit::set_backspace_deletes_composite_character_enabled(bool p_enabled)
{
	backspace_deletes_composite_character_enabled = p_enabled;
}

bool TextEdit::is_backspace_deletes_composite_character_enabled() const
{
	return backspace_deletes_composite_character_enabled;
}

void TextEdit::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool TextEdit::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

void TextEdit::set_virtual_keyboard_enabled(bool p_enabled)
{
	virtual_keyboard_enabled = p_enabled;
}

bool TextEdit::is_virtual_keyboard_enabled() const { return virtual_keyboard_enabled; }

void TextEdit::set_virtual_keyboard_show_on_focus(bool p_show_on_focus)
{
	virtual_keyboard_show_on_focus = p_show_on_focus;
}

bool TextEdit::get_virtual_keyboard_show_on_focus() const { return virtual_keyboard_show_on_focus; }

void TextEdit::set_middle_mouse_paste_enabled(bool p_enabled)
{
	middle_mouse_paste_enabled = p_enabled;
}

bool TextEdit::is_middle_mouse_paste_enabled() const { return middle_mouse_paste_enabled; }

void TextEdit::set_empty_selection_clipboard_enabled(bool p_enabled)
{
	empty_selection_clipboard_enabled = p_enabled;
}

bool TextEdit::is_empty_selection_clipboard_enabled() const
{
	return empty_selection_clipboard_enabled;
}

void TextEdit::set_text(const String& p_text) { _set_text(p_text, false); }

String TextEdit::get_text() const
{
	StringBuilder ret_text;
	const int text_size = text.size();
	for (int i = 0; i < text_size; i++) {
		ret_text += text[i];
		if (i != text_size - 1) {
			ret_text += "\n";
		}
	}
	return ret_text.as_string();
}

int TextEdit::get_line_count() const { return text.size(); }

String TextEdit::get_placeholder() const { return placeholder_text; }

String TextEdit::get_line(int p_line) const
{
	if (p_line < 0 || p_line >= text.size()) {
		return String();
	}
	return text[p_line];
}

String TextEdit::get_line_with_ime(int p_line) const
{
	if (p_line < 0 || p_line >= text.size()) {
		return String();
	}
	return text.get_text_with_ime(p_line);
}

int TextEdit::get_line_width(int p_line, int p_wrap_index) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	ERR_FAIL_COND_V(p_wrap_index > get_line_wrap_count(p_line), 0);

	return text.get_line_width(p_line, p_wrap_index);
}

int TextEdit::get_line_height() const
{
	return MAX(text.get_line_height() + theme_cache.line_spacing, 1);
}

int TextEdit::_get_wrapped_indent_level(int p_line, int& r_first_wrap) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	const Vector<Vector2i> wr = text.get_line_wrap_ranges(p_line);
	r_first_wrap = 0;

	int tab_count = 0;
	int whitespace_count = 0;
	int line_length = text[p_line].size();
	for (int i = 0; i < line_length - 1; i++) {
		if (r_first_wrap < wr.size() && i >= wr[r_first_wrap].y) {
			tab_count = 0;
			whitespace_count = 0;
			r_first_wrap++;
		}
		if (text[p_line][i] == '\t') {
			tab_count++;
		}
		else if (text[p_line][i] == ' ') {
			whitespace_count++;
		}
		else {
			break;
		}
	}
	return tab_count * text.get_tab_size() + whitespace_count;
}

float TextEdit::_get_wrap_indent_offset(int p_line, int p_wrap_index, bool p_rtl) const
{
	if (!text.is_indent_wrapped_lines()) {
		return 0;
	}
	int first_indent_line = 0;
	_get_wrapped_indent_level(p_line, first_indent_line);
	if (p_wrap_index > first_indent_line) {
		return MIN(text.get_indent_offset(p_line, p_rtl), wrap_at_column * 0.6);
	}
	return 0;
}

int TextEdit::get_indent_level(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	int tab_count = 0;
	int whitespace_count = 0;
	int line_length = text[p_line].size();
	for (int i = 0; i < line_length - 1; i++) {
		if (text[p_line][i] == '\t') {
			tab_count++;
		}
		else if (text[p_line][i] == ' ') {
			whitespace_count++;
		}
		else {
			break;
		}
	}
	return tab_count * text.get_tab_size() + whitespace_count;
}

int TextEdit::get_first_non_whitespace_column(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	int col = 0;
	while (col < text[p_line].length() && is_whitespace(text[p_line][col])) {
		col++;
	}
	return col;
}

int TextEdit::get_last_unhidden_line() const
{
	// Returns the last line in the text that is not hidden.
	if (!_is_hiding_enabled()) {
		return text.size() - 1;
	}

	int last_line;
	for (last_line = text.size() - 1; last_line > 0; last_line--) {
		if (!_is_line_hidden(last_line)) {
			break;
		}
	}
	return last_line;
}

int TextEdit::get_next_visible_line_offset_from(int p_line_from, int p_visible_amount) const
{
	// Returns the number of lines (hidden and unhidden) from p_line_from to (p_line_from +
	// visible_amount of unhidden lines).
	ERR_FAIL_INDEX_V(p_line_from, text.size(), Math::abs(p_visible_amount));

	if (!_is_hiding_enabled()) {
		return Math::abs(p_visible_amount);
	}

	int num_visible = 0;
	int num_total = 0;
	if (p_visible_amount >= 0) {
		for (int i = p_line_from; i < text.size(); i++) {
			num_total++;
			if (!_is_line_hidden(i)) {
				num_visible++;
			}
			if (num_visible >= p_visible_amount) {
				break;
			}
		}
	}
	else {
		p_visible_amount = Math::abs(p_visible_amount);
		for (int i = p_line_from; i >= 0; i--) {
			num_total++;
			if (!_is_line_hidden(i)) {
				num_visible++;
			}
			if (num_visible >= p_visible_amount) {
				break;
			}
		}
	}
	return num_total;
}

Point2i TextEdit::get_next_visible_line_index_offset_from(
	int p_line_from, int p_wrap_index_from, int p_visible_amount) const
{
	// Returns the number of lines (hidden and unhidden) from (p_line_from + p_wrap_index_from) row
	// to (p_line_from + visible_amount of unhidden and wrapped rows). Wrap index is set to the wrap
	// index of the last line.
	int wrap_index = 0;
	ERR_FAIL_INDEX_V(p_line_from, text.size(), Point2i(Math::abs(p_visible_amount), 0));

	if (!_is_hiding_enabled() && get_line_wrapping_mode() == LineWrappingMode::LINE_WRAPPING_NONE) {
		return Point2i(Math::abs(p_visible_amount), 0);
	}

	int num_visible = 0;
	int num_total = 0;
	if (p_visible_amount == 0) {
		num_total = 0;
		wrap_index = 0;
	}
	else if (p_visible_amount > 0) {
		int i;
		num_visible -= p_wrap_index_from;
		for (i = p_line_from; i < text.size(); i++) {
			num_total++;
			if (!_is_line_hidden(i)) {
				num_visible++;
				num_visible += get_line_wrap_count(i);
			}
			if (num_visible >= p_visible_amount) {
				break;
			}
		}
		wrap_index =
			get_line_wrap_count(MIN(i, text.size() - 1)) - MAX(0, num_visible - p_visible_amount);

		// If we are a hidden line, then we are the last line as we cannot reach "p_visible_amount".
		// This means we need to backtrack to get last visible line.
		// Currently, line 0 cannot be hidden so this should always be valid.
		int line = (p_line_from + num_total) - 1;
		if (_is_line_hidden(line)) {
			Point2i backtrack = get_next_visible_line_index_offset_from(line, 0, -1);
			num_total = num_total - (backtrack.x - 1);
			wrap_index = backtrack.y;
		}
	}
	else {
		p_visible_amount = Math::abs(p_visible_amount);
		int i;
		num_visible -= get_line_wrap_count(p_line_from) - p_wrap_index_from;
		for (i = p_line_from; i >= 0; i--) {
			num_total++;
			if (!_is_line_hidden(i)) {
				num_visible++;
				num_visible += get_line_wrap_count(i);
			}
			if (num_visible >= p_visible_amount) {
				break;
			}
		}
		wrap_index = MAX(0, num_visible - p_visible_amount);
	}
	wrap_index = MAX(wrap_index, 0);
	return Point2i(num_total, wrap_index);
}

void TextEdit::copy(int p_caret) { _copy_internal(p_caret); }

PopupMenu* TextEdit::get_menu() const
{
	if (!menu) {
		const_cast<TextEdit*>(this)->_generate_context_menu();
	}
	return menu;
}

bool TextEdit::is_menu_visible() const { return menu && menu->is_visible(); }

void TextEdit::menu_option(int p_option)
{
	switch (p_option) {
	case MENU_COPY: {
		copy();
	} break;
	case MENU_CLEAR: {
		if (editable) {
			clear();
		}
	} break;
	case MENU_SELECT_ALL: {
		select_all();
	} break;
	case MENU_UNDO: {
		undo();
	} break;
	case MENU_REDO: {
		redo();
	} break;
	case MENU_DIR_INHERITED: {
		set_text_direction(TEXT_DIRECTION_INHERITED);
	} break;
	case MENU_DIR_AUTO: {
		set_text_direction(TEXT_DIRECTION_AUTO);
	} break;
	case MENU_DIR_LTR: {
		set_text_direction(TEXT_DIRECTION_LTR);
	} break;
	case MENU_DIR_RTL: {
		set_text_direction(TEXT_DIRECTION_RTL);
	} break;
	case MENU_DISPLAY_UCC: {
		set_draw_control_chars(!get_draw_control_chars());
	} break;
	case MENU_EMOJI_AND_SYMBOL: {
		show_emoji_and_symbol_picker();
	} break;
	}
}

void TextEdit::start_action(EditAction p_action)
{
	if (current_action != p_action) {
		if (current_action != EditAction::ACTION_NONE) {
			in_action = false;
			pending_action_end = false;
			end_complex_operation();
		}

		if (p_action != EditAction::ACTION_NONE) {
			in_action = true;
			begin_complex_operation();
		}
	}
	else if (current_action != EditAction::ACTION_NONE) {
		pending_action_end = false;
	}
	current_action = p_action;
}

void TextEdit::end_action()
{
	if (current_action != EditAction::ACTION_NONE) {
		pending_action_end = true;
		queue_accessibility_update();
	}
}

TextEdit::EditAction TextEdit::get_current_action() const { return current_action; }

void TextEdit::begin_complex_operation()
{
	_push_current_op();
	if (complex_operation_count == 0) {
		next_operation_is_complex = true;
		current_op.start_carets = carets;
	}
	complex_operation_count++;
}

void TextEdit::end_complex_operation()
{
	_push_current_op();

	queue_accessibility_update();

	complex_operation_count = MAX(complex_operation_count - 1, 0);
	if (complex_operation_count > 0) {
		return;
	}
	if (undo_stack.is_empty()) {
		return;
	}

	undo_stack.back()->get().end_carets = carets;
	if (undo_stack.back()->get().chain_forward) {
		undo_stack.back()->get().chain_forward = false;
		return;
	}

	undo_stack.back()->get().chain_backward = true;
}

bool TextEdit::has_undo() const
{
	if (undo_stack_pos == nullptr) {
		int pending = current_op.type == TextOperation::TYPE_NONE ? 0 : 1;
		return undo_stack.size() + pending > 0;
	}
	return undo_stack_pos != undo_stack.front();
}

bool TextEdit::has_redo() const { return undo_stack_pos != nullptr; }

void TextEdit::deselect(int p_caret) {}

void TextEdit::undo()
{
	if (!editable) {
		return;
	}

	if (in_action) {
		pending_action_end = true;
	}
	_push_current_op();

	if (undo_stack_pos == nullptr) {
		if (undo_stack.is_empty()) {
			return; // Nothing to undo.
		}

		undo_stack_pos = undo_stack.back();

	}
	else if (undo_stack_pos == undo_stack.front()) {
		return; // At the bottom of the undo stack.
	}
	else {
		undo_stack_pos = undo_stack_pos->prev();
	}

	deselect();

	TextOperation op = undo_stack_pos->get();
	_do_text_op(op, true);

	current_op.version = op.prev_version;
	if (undo_stack_pos->get().chain_backward) {
		// This was part of a complex operation, undo until the chain forward at the start of the
		// complex operation.
		while (true) {
			ERR_BREAK(!undo_stack_pos->prev());
			undo_stack_pos = undo_stack_pos->prev();
			op = undo_stack_pos->get();
			_do_text_op(op, true);
			current_op.version = op.prev_version;
			if (undo_stack_pos->get().chain_forward) {
				break;
			}
		}
	}

	_update_scrollbars();
	bool dirty_carets = get_caret_count() != undo_stack_pos->get().start_carets.size();
	if (!dirty_carets) {
		for (int i = 0; i < get_caret_count(); i++) {
			if (carets[i].line != undo_stack_pos->get().start_carets[i].line ||
				carets[i].column != undo_stack_pos->get().start_carets[i].column) {
				dirty_carets = true;
				break;
			}
		}
	}

	carets = undo_stack_pos->get().start_carets;

	_unhide_carets();

	if (dirty_carets) {
		_caret_changed();
		_selection_changed();
	}
	adjust_viewport_to_caret();
	queue_accessibility_update();
}

void TextEdit::redo()
{
	if (!editable) {
		return;
	}

	if (in_action) {
		pending_action_end = true;
	}
	_push_current_op();

	if (!has_redo()) {
		return; // Nothing to do.
	}

	deselect();

	TextOperation op = undo_stack_pos->get();
	_do_text_op(op, false);
	current_op.version = op.version;
	if (undo_stack_pos->get().chain_forward) {
		// This was part of a complex operation, redo until the chain backward at the end of the
		// complex operation.
		while (true) {
			ERR_BREAK(!undo_stack_pos->next());
			undo_stack_pos = undo_stack_pos->next();
			op = undo_stack_pos->get();
			_do_text_op(op, false);
			current_op.version = op.version;
			if (undo_stack_pos->get().chain_backward) {
				break;
			}
		}
	}

	_update_scrollbars();
	bool dirty_carets = get_caret_count() != undo_stack_pos->get().end_carets.size();
	if (!dirty_carets) {
		for (int i = 0; i < get_caret_count(); i++) {
			if (carets[i].line != undo_stack_pos->get().end_carets[i].line ||
				carets[i].column != undo_stack_pos->get().end_carets[i].column) {
				dirty_carets = true;
				break;
			}
		}
	}

	carets = undo_stack_pos->get().end_carets;
	undo_stack_pos = undo_stack_pos->next();

	_unhide_carets();

	if (dirty_carets) {
		_caret_changed();
		_selection_changed();
	}
	adjust_viewport_to_caret();
	queue_accessibility_update();
}

void TextEdit::clear_undo_history()
{
	saved_version = 0;
	current_op.type = TextOperation::TYPE_NONE;
	undo_stack_pos = nullptr;
	undo_stack.clear();
}

bool TextEdit::is_insert_text_operation() const
{
	return (current_op.type == TextOperation::TYPE_INSERT ||
			current_action == EditAction::ACTION_TYPING);
}

void TextEdit::tag_saved_version() { saved_version = get_version(); }

uint32_t TextEdit::get_version() const { return current_op.version; }

uint32_t TextEdit::get_saved_version() const { return saved_version; }

void TextEdit::set_search_text(const String& p_search_text) { search_text = p_search_text; }

void TextEdit::set_search_flags(uint32_t p_flags) { search_flags = p_flags; }

Point2i TextEdit::search(
	const String& p_key, uint32_t p_search_flags, int p_from_line, int p_from_column) const
{
	if (p_key.is_empty()) {
		return Point2(-1, -1);
	}
	ERR_FAIL_INDEX_V(p_from_line, text.size(), Point2i(-1, -1));
	ERR_FAIL_INDEX_V(p_from_column, text[p_from_line].length() + 1, Point2i(-1, -1));

	const bool key_start_is_symbol = is_symbol(p_key[0]);
	const bool key_end_is_symbol = is_symbol(p_key[p_key.length() - 1]);

	// Search the whole document, starting from the current line.
	// We'll auto-wrap through the start / end to search every line.
	int current_line = p_from_line;
	int current_column = p_from_column;

	if (p_search_flags & SEARCH_BACKWARDS) {
		// `rfind` requires the from index to be within the bounds of the last possible match
		// position.
		current_column = MIN(current_column, text[p_from_line].length() - p_key.length());
	}

	// + 1 because we'll search p_from_line twice - starting from p_from_column, and then again at
	// the very end.
	for (int i = 0; i < text.size() + 1; i++) {
		const String& text_line = text[current_line];

		// Search the current line as often as necessary.
		while (true) {
			if (p_search_flags & SEARCH_BACKWARDS) {
				current_column = (p_search_flags & SEARCH_MATCH_CASE)
									 ? text_line.rfind(p_key, current_column)
									 : text_line.rfindn(p_key, current_column);
			}
			else {
				current_column = (p_search_flags & SEARCH_MATCH_CASE)
									 ? text_line.find(p_key, current_column)
									 : text_line.findn(p_key, current_column);
			}

			if (current_column == -1) {
				break; // Nothing else found on the current line.
			}

			bool is_match = true;

			if (p_search_flags & SEARCH_WHOLE_WORDS) {
				// Validate for whole words.
				if (!key_start_is_symbol && current_column > 0 &&
					!is_symbol(text_line[current_column - 1])) {
					is_match = false;
				}
				else if (!key_end_is_symbol &&
						   current_column + p_key.length() < text_line.length() &&
						   !is_symbol(text_line[current_column + p_key.length()])) {
					is_match = false;
				}
			}

			if (is_match) {
				// Found the string!
				return Point2i(current_column, current_line);
			}

			// Advance past the current occurrence.
			current_column += p_search_flags & SEARCH_BACKWARDS ? -1 : 1;
		}

		// Prepare for next iteration.
		if (p_search_flags & SEARCH_BACKWARDS) {
			current_column = -1;
			current_line--;
			if (current_line < 0) {
				// Searched the whole document backwards; wrap to end.
				current_line = text.size() - 1;
			}
		}
		else {
			current_line++;
			current_column = 0;
			if (current_line == text.size()) {
				// Searched the whole document forwards; wrap to start.
				current_line = 0;
			}
		}
	}

	// Nothing found!
	return Point2i(-1, -1);
}

Point2 TextEdit::get_local_mouse_pos() const
{
	Point2 mp = get_local_mouse_position();
	if (is_layout_rtl()) {
		mp.x = get_size().width - mp.x;
	}
	return mp;
}

String TextEdit::get_word_at_pos(const Vector2& p_pos) const
{
	Point2i pos = get_line_column_at_pos(p_pos, false, false);
	int line = pos.y;
	int col = pos.x;
	return get_word(line, col);
}

String TextEdit::get_word(int p_line, int p_column) const
{
	if (p_line < 0 || p_column < 0) {
		return String();
	}
	ERR_FAIL_INDEX_V(p_line, text.size(), String());

	const String& text_line = text.get_text_with_ime(p_line);
	if (text_line.is_empty()) {
		return String();
	}
	ERR_FAIL_INDEX_V(p_column, text_line.size() + 1, String());

	const PackedInt32Array words =
		TS->shaped_text_get_word_breaks(text.get_line_data(p_line)->get_rid());
	for (int i = 0; i < words.size(); i = i + 2) {
		if (words[i] <= p_column && words[i + 1] >= p_column) {
			return text_line.substr(words[i], words[i + 1] - words[i]);
		}
	}
	return String();
}

Point2i TextEdit::get_line_column_at_pos(
	const Point2i& p_pos, bool p_clamp_line, bool p_clamp_column) const
{
	Ref<StyleBox> style = _get_current_stylebox();
	float rows = p_pos.y - style->get_margin(SIDE_TOP);
	rows /= get_line_height();
	rows += _get_v_scroll_offset();
	int first_vis_line = get_first_visible_line();
	int row = first_vis_line + Math::floor(rows);
	int wrap_index = 0;

	if (get_line_wrapping_mode() != LineWrappingMode::LINE_WRAPPING_NONE || _is_hiding_enabled()) {
		Point2i f_ofs = get_next_visible_line_index_offset_from(
			first_vis_line, first_visible_line_wrap_ofs, rows + (1 * SIGN(rows)));
		wrap_index = f_ofs.y;

		if (rows < 0) {
			row = first_vis_line - (f_ofs.x - 1);
		}
		else {
			row = first_vis_line + (f_ofs.x - 1);
		}
	}

	row = CLAMP(row, 0, text.size() - 1);

	int visible_lines = get_visible_line_count_in_range(first_vis_line, row);
	if (rows > visible_lines) {
		if (p_clamp_line) {
			return Point2i(text[row].length(), row);
		}
		return Point2i(-1, -1);
	}
	int colx =
		p_pos.x - (Math::ceil(style->get_margin(SIDE_LEFT)) + gutters_width + gutter_padding);
	colx += first_visible_col;

	RID text_rid = text.get_line_data(row)->get_line_rid(wrap_index);

	bool rtl = is_layout_rtl();
	const float wrap_indent = _get_wrap_indent_offset(row, wrap_index, rtl);

	if (rtl) {
		colx = TS->shaped_text_get_size(text_rid).x - colx + wrap_indent;
	}
	else {
		colx -= wrap_indent;
	}

	if (!p_clamp_column && (colx < 0 || colx > TS->shaped_text_get_size(text_rid).x)) {
		return Point2i(-1, -1);
	}

	int col = TS->shaped_text_hit_test_position(text_rid, colx);
	if (col == -1) {
		return Point2i(-1, -1);
	}
	if (!caret_mid_grapheme_enabled) {
		col = TS->shaped_text_closest_character_pos(text_rid, col);
	}

	return Point2i(col, row);
}

Point2i TextEdit::get_pos_at_line_column(int p_line, int p_column) const
{
	Rect2i rect = get_rect_at_line_column(p_line, p_column);
	return rect.position.x == -1 ? rect.position : rect.position + Vector2i(0, get_line_height());
}

Rect2i TextEdit::get_rect_at_line_column(int p_line, int p_column) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Rect2i(-1, -1, 0, 0));
	ERR_FAIL_COND_V(p_column < 0, Rect2i(-1, -1, 0, 0));
	ERR_FAIL_COND_V(p_column > text[p_line].length(), Rect2i(-1, -1, 0, 0));

	if (text.size() == 1 && text[0].is_empty()) {
		// The TextEdit is empty.
		return Rect2i();
	}

	if (line_drawing_cache.is_empty() || !line_drawing_cache.has(p_line)) {
		// Line is not in the cache, which means it's outside of the viewing area.
		return Rect2i(-1, -1, 0, 0);
	}
	LineDrawingCache cache_entry = line_drawing_cache[p_line];

	int wrap_index = get_line_wrap_index_at_column(p_line, p_column);
	if (wrap_index >= cache_entry.first_visible_chars.size()) {
		// Line seems to be wrapped beyond the viewable area.
		return Rect2i(-1, -1, 0, 0);
	}

	int first_visible_char = cache_entry.first_visible_chars[wrap_index];
	int last_visible_char = cache_entry.last_visible_chars[wrap_index];
	if (p_column < first_visible_char || p_column > last_visible_char) {
		// Character is outside of the viewing area, no point calculating its position.
		return Rect2i(-1, -1, 0, 0);
	}

	const float wrap_indent = _get_wrap_indent_offset(p_line, wrap_index, is_layout_rtl());

	Point2i pos, size;
	pos.y = cache_entry.y_offset + get_line_height() * wrap_index;
	pos.x = get_total_gutter_width() + get_line_start_margin() + wrap_indent - get_h_scroll();

	RID text_rid = text.get_line_data(p_line)->get_line_rid(wrap_index);
	Vector2 col_bounds = TS->shaped_text_get_grapheme_bounds(text_rid, p_column);
	pos.x += col_bounds.x;
	size.x = col_bounds.y - col_bounds.x;

	size.y = get_line_height();

	return Rect2i(pos, size);
}

int TextEdit::get_line_start_margin() const
{
	return Math::ceil(_get_current_stylebox()->get_margin(SIDE_LEFT));
}

int TextEdit::get_minimap_line_at_pos(const Point2i& p_pos) const
{
	float rows = p_pos.y - _get_current_stylebox()->get_margin(SIDE_TOP);
	rows /= (minimap_char_size.y + minimap_line_spacing);
	rows += _get_v_scroll_offset();

	// Calculate visible lines.
	int minimap_visible_lines = get_minimap_visible_lines();
	int visible_rows = get_visible_line_count() + 1;
	int first_vis_line = get_first_visible_line() - 1;
	int draw_amount = visible_rows + 1;
	draw_amount += get_line_wrap_count(first_vis_line + 1);
	int minimap_line_height = (minimap_char_size.y + minimap_line_spacing);

	// Calculate viewport size and y offset.
	int viewport_height = (draw_amount - 1) * minimap_line_height;
	int control_height = _get_control_height() - viewport_height;
	int viewport_offset_y =
		std::round(get_scroll_pos_for_line(first_vis_line + 1) * control_height) /
		((v_scroll->get_max() <= minimap_visible_lines) ? (minimap_visible_lines - draw_amount)
														: (v_scroll->get_max() - draw_amount));

	// Calculate the first line.
	int num_lines_before = std::round((viewport_offset_y) / minimap_line_height);
	int minimap_line = (v_scroll->get_max() <= minimap_visible_lines) ? -1 : first_vis_line;
	if (first_vis_line > 0 && minimap_line >= 0) {
		minimap_line -=
			get_next_visible_line_index_offset_from(first_vis_line, 0, -num_lines_before).x;
		minimap_line -= (minimap_line > 0 && smooth_scroll_enabled ? 1 : 0);
	}

	if (minimap_line < 0) {
		minimap_line = 0;
	}

	int row = minimap_line + Math::floor(rows);
	if (get_line_wrapping_mode() != LineWrappingMode::LINE_WRAPPING_NONE || _is_hiding_enabled()) {
		int f_ofs = get_next_visible_line_index_offset_from(
						minimap_line, first_visible_line_wrap_ofs, rows + (1 * SIGN(rows)))
						.x -
					1;
		if (rows < 0) {
			row = minimap_line - f_ofs;
		}
		else {
			row = minimap_line + f_ofs;
		}
	}

	row = CLAMP(row, 0, text.size() - 1);

	return row;
}

bool TextEdit::is_dragging_cursor() const { return dragging_selection || dragging_minimap; }

bool TextEdit::is_mouse_over_selection(bool p_edges, int p_caret) const
{
	Point2i pos = get_line_column_at_pos(get_local_mouse_pos());
	int line = pos.y;
	int column = pos.x;

	if ((p_caret == -1 && get_selection_at_line_column(line, column, p_edges) != -1) ||
		(p_caret != -1 && _selection_contains(p_caret, line, column, p_edges))) {
		return true;
	}
	return false;
}

TextEdit::CaretType TextEdit::get_caret_type() const { return caret_type; }

bool TextEdit::is_caret_blink_enabled() const { return caret_blink_enabled; }

float TextEdit::get_caret_blink_interval() const { return caret_blink_timer->get_wait_time(); }

bool TextEdit::is_drawing_caret_when_editable_disabled() const
{
	return draw_caret_when_editable_disabled;
}

void TextEdit::set_move_caret_on_right_click_enabled(bool p_enabled)
{
	move_caret_on_right_click = p_enabled;
}

bool TextEdit::is_move_caret_on_right_click_enabled() const { return move_caret_on_right_click; }

void TextEdit::set_caret_mid_grapheme_enabled(bool p_enabled)
{
	caret_mid_grapheme_enabled = p_enabled;
}

bool TextEdit::is_caret_mid_grapheme_enabled() const { return caret_mid_grapheme_enabled; }

void TextEdit::set_multiple_carets_enabled(bool p_enabled)
{
	multi_carets_enabled = p_enabled;
	if (!multi_carets_enabled) {
		remove_secondary_carets();
		multicaret_edit_count = 0;
		multicaret_edit_ignore_carets.clear();
		multicaret_edit_merge_queued = false;
	}
}

bool TextEdit::is_multiple_carets_enabled() const { return multi_carets_enabled; }

int TextEdit::add_caret(int p_line, int p_column)
{
	if (!multi_carets_enabled) {
		return -1;
	}
	_cancel_drag_and_drop_text();

	p_line = CLAMP(p_line, 0, text.size() - 1);
	p_column = CLAMP(p_column, 0, get_line(p_line).length());

	if (!is_in_mulitcaret_edit()) {
		// Carets cannot overlap.
		if (get_selection_at_line_column(p_line, p_column, true, false) != -1) {
			return -1;
		}
	}

	carets.push_back(Caret());
	int new_index = carets.size() - 1;
	set_caret_line(p_line, false, false, -1, new_index);
	set_caret_column(p_column, false, new_index);
	_caret_changed(new_index);

	if (is_in_mulitcaret_edit()) {
		multicaret_edit_ignore_carets.insert(new_index);
		merge_overlapping_carets();
	}
	return new_index;
}

void TextEdit::remove_caret(int p_caret)
{
	ERR_FAIL_COND_MSG(carets.size() <= 1, "The main caret should not be removed.");
	ERR_FAIL_INDEX(p_caret, carets.size());

	_caret_changed(p_caret);
	carets.remove_at(p_caret);

	if (drag_caret_index >= 0) {
		if (p_caret == drag_caret_index) {
			drag_caret_index = -1;
		}
		else if (p_caret < drag_caret_index) {
			drag_caret_index -= 1;
		}
	}
}

void TextEdit::remove_drag_caret()
{
	if (drag_caret_index >= 0) {
		if (drag_caret_index < carets.size()) {
			remove_caret(drag_caret_index);
		}
		drag_caret_index = -1;
	}
}

void TextEdit::remove_secondary_carets()
{
	if (carets.size() == 1) {
		return;
	}

	_caret_changed();
	carets.resize(1);

	if (drag_caret_index >= 0) {
		drag_caret_index = -1;
	}
	queue_accessibility_update();
}

int TextEdit::get_caret_count() const
{
	// Don't include drag caret.
	if (drag_caret_index >= 0) {
		return carets.size() - 1;
	}
	return carets.size();
}

void TextEdit::add_caret_at_carets(bool p_below)
{
	if (!multi_carets_enabled) {
		return;
	}
	const int last_line_max_wrap = get_line_wrap_count(text.size() - 1);

	set_selection_mode(SELECTION_MODE_NONE);

	begin_multicaret_edit();
	int view_target_caret = -1;
	int view_line = p_below ? -1 : INT_MAX;
	int num_carets = get_caret_count();
	for (int i = 0; i < num_carets; i++) {
		const int caret_line = get_caret_line(i);
		const int caret_column = get_caret_column(i);
		bool is_selected =
			has_selection(i) || carets[i].last_fit_x != carets[i].selection.origin_last_fit_x;
		const int selection_origin_line = get_selection_origin_line(i);
		const int selection_origin_column = get_selection_origin_column(i);
		const int caret_wrap_index = get_caret_wrap_index(i);
		const int selection_origin_wrap_index =
			!is_selected
				? -1
				: get_line_wrap_index_at_column(selection_origin_line, selection_origin_column);

		if (caret_line == 0 && !p_below &&
			(caret_wrap_index == 0 || selection_origin_wrap_index == 0)) {
			// Can't add above the first line.
			continue;
		}
		if (caret_line == text.size() - 1 && p_below &&
			(caret_wrap_index == last_line_max_wrap ||
				selection_origin_wrap_index == last_line_max_wrap)) {
			// Can't add below the last line.
			continue;
		}

		// Add a new caret.
		int new_caret_index = add_caret(caret_line, caret_column);
		ERR_FAIL_COND_MSG(new_caret_index < 0, "Failed to add a caret.");

		// Copy the selection origin and last fit.
		set_selection_origin_line(selection_origin_line, true, -1, new_caret_index);
		set_selection_origin_column(selection_origin_column, new_caret_index);
		carets.write[new_caret_index].last_fit_x = carets[i].last_fit_x;
		carets.write[new_caret_index].selection.origin_last_fit_x =
			carets[i].selection.origin_last_fit_x;

		// Move the caret up or down one visible line.
		if (!p_below) {
			// Move caret up.
			if (caret_wrap_index > 0) {
				set_caret_line(caret_line, false, false, caret_wrap_index - 1, new_caret_index);
			}
			else {
				int new_line = caret_line - get_next_visible_line_offset_from(caret_line - 1, -1);
				if (is_line_wrapped(new_line)) {
					set_caret_line(
						new_line, false, false, get_line_wrap_count(new_line), new_caret_index);
				}
				else {
					set_caret_line(new_line, false, false, 0, new_caret_index);
				}
			}
			// Move selection origin up.
			if (is_selected) {
				if (selection_origin_wrap_index > 0) {
					set_selection_origin_line(
						caret_line, false, selection_origin_wrap_index - 1, new_caret_index);
				}
				else {
					int new_line = selection_origin_line -
								   get_next_visible_line_offset_from(selection_origin_line - 1, -1);
					if (is_line_wrapped(new_line)) {
						set_selection_origin_line(
							new_line, false, get_line_wrap_count(new_line), new_caret_index);
					}
					else {
						set_selection_origin_line(new_line, false, 0, new_caret_index);
					}
				}
			}
			if (get_caret_line(new_caret_index) < view_line) {
				view_line = get_caret_line(new_caret_index);
				view_target_caret = new_caret_index;
			}
		}
		else {
			// Move caret down.
			if (caret_wrap_index < get_line_wrap_count(caret_line)) {
				set_caret_line(caret_line, false, false, caret_wrap_index + 1, new_caret_index);
			}
			else {
				int new_line = caret_line + get_next_visible_line_offset_from(
												CLAMP(caret_line + 1, 0, text.size() - 1), 1);
				set_caret_line(new_line, false, false, 0, new_caret_index);
			}
			// Move selection origin down.
			if (is_selected) {
				if (selection_origin_wrap_index < get_line_wrap_count(selection_origin_line)) {
					set_selection_origin_line(selection_origin_line, false,
						selection_origin_wrap_index + 1, new_caret_index);
				}
				else {
					int new_line = selection_origin_line +
								   get_next_visible_line_offset_from(
									   CLAMP(selection_origin_line + 1, 0, text.size() - 1), 1);
					set_selection_origin_line(new_line, false, 0, new_caret_index);
				}
			}
			if (get_caret_line(new_caret_index) > view_line) {
				view_line = get_caret_line(new_caret_index);
				view_target_caret = new_caret_index;
			}
		}
		if (is_selected) {
			// Make sure selection is active.
			select(get_selection_origin_line(new_caret_index),
				get_selection_origin_column(new_caret_index), get_caret_line(new_caret_index),
				get_caret_column(new_caret_index), new_caret_index);
			carets.write[new_caret_index].last_fit_x = carets[i].last_fit_x;
			carets.write[new_caret_index].selection.origin_last_fit_x =
				carets[i].selection.origin_last_fit_x;
		}

		bool check_edges = !has_selection(0) || !has_selection(new_caret_index);
		bool will_merge_with_main_caret =
			_selection_contains(0, get_caret_line(new_caret_index),
				get_caret_column(new_caret_index), check_edges, false) ||
			_selection_contains(
				new_caret_index, get_caret_line(0), get_caret_column(0), check_edges, false);
		if (will_merge_with_main_caret) {
			// Move next to the main caret so it stays the main caret after merging.
			Caret new_caret = carets[new_caret_index];
			carets.remove_at(new_caret_index);
			carets.insert(0, new_caret);
			i++;
			num_carets += 1;
		}
	}

	// Show the topmost caret if added above or bottommost caret if added below.
	if (view_target_caret >= 0 && view_target_caret < get_caret_count()) {
		adjust_viewport_to_caret(view_target_caret);
	}

	merge_overlapping_carets();
	end_multicaret_edit();
}

struct _CaretSortComparator
{
	_FORCE_INLINE_ bool operator()(const Vector3i& a, const Vector3i& b) const
	{
		// x is column, y is line, z is caret index.
		if (a.y == b.y) {
			return a.x < b.x;
		}
		return a.y < b.y;
	}
};

Vector<int> TextEdit::get_sorted_carets(bool p_include_ignored_carets) const
{
	// Returns caret indexes sorted by selection start or caret position from top to bottom of text.
	Vector<Vector3i> caret_line_col_indexes;
	for (int i = 0; i < get_caret_count(); i++) {
		if (!p_include_ignored_carets && multicaret_edit_ignore_caret(i)) {
			continue;
		}
		caret_line_col_indexes.push_back(
			Vector3i(get_selection_from_column(i), get_selection_from_line(i), i));
	}
	caret_line_col_indexes.sort_custom<_CaretSortComparator>();
	Vector<int> sorted;
	sorted.resize(caret_line_col_indexes.size());
	for (int i = 0; i < caret_line_col_indexes.size(); i++) {
		sorted.set(i, caret_line_col_indexes[i].z);
	}
	return sorted;
}

void TextEdit::collapse_carets(
	int p_from_line, int p_from_column, int p_to_line, int p_to_column, bool p_inclusive)
{
	// Collapse carets in the selected range to the from position.

	// Clamp the collapse target position.

	int collapse_line = CLAMP(p_from_line, 0, text.size() - 1);
	int collapse_column = CLAMP(p_from_column, 0, text[collapse_line].length());

	// Swap the lines if they are in the wrong order.
	if (p_from_line > p_to_line) {
		SWAP(p_from_line, p_to_line);
		SWAP(p_from_column, p_to_column);
	}
	if (p_from_line == p_to_line && p_from_column > p_to_column) {
		SWAP(p_from_column, p_to_column);
	}
	bool any_collapsed = false;

	// Intentionally includes carets in the multicaret_edit_ignore list so that they are moved
	// together.
	for (int i = 0; i < get_caret_count(); i++) {
		bool is_caret_in = _is_line_col_in_range(get_caret_line(i), get_caret_column(i),
			p_from_line, p_from_column, p_to_line, p_to_column, p_inclusive);
		if (!has_selection(i)) {
			if (is_caret_in) {
				// Caret was in the collapsed area.
				set_caret_line(collapse_line, false, true, -1, i);
				set_caret_column(collapse_column, false, i);
				if (is_in_mulitcaret_edit() && get_caret_count() > 1) {
					multicaret_edit_ignore_carets.insert(i);
				}
				any_collapsed = true;
			}
		}
		else {
			bool is_origin_in =
				_is_line_col_in_range(get_selection_origin_line(i), get_selection_origin_column(i),
					p_from_line, p_from_column, p_to_line, p_to_column, p_inclusive);

			if (is_caret_in && is_origin_in) {
				// Selection was completely encapsulated.
				deselect(i);
				set_caret_line(collapse_line, false, true, -1, i);
				set_caret_column(collapse_column, false, i);
				if (is_in_mulitcaret_edit() && get_caret_count() > 1) {
					multicaret_edit_ignore_carets.insert(i);
				}
				any_collapsed = true;
			}
			else if (is_caret_in) {
				// Only caret was inside.
				set_caret_line(collapse_line, false, true, -1, i);
				set_caret_column(collapse_column, false, i);
				any_collapsed = true;
			}
			else if (is_origin_in) {
				// Only selection origin was inside.
				set_selection_origin_line(collapse_line, true, -1, i);
				set_selection_origin_column(collapse_column, i);
				any_collapsed = true;
			}
		}
		if (!p_inclusive && !any_collapsed) {
			if ((get_caret_line(i) == collapse_line && get_caret_column(i) == collapse_column) ||
				(get_selection_origin_line(i) == collapse_line &&
					get_selection_origin_column(i) == collapse_column)) {
				// Make sure to queue a merge, even if we didn't include it.
				any_collapsed = true;
			}
		}
	}
	if (any_collapsed) {
		merge_overlapping_carets();
	}
}

void TextEdit::merge_overlapping_carets()
{
	if (is_in_mulitcaret_edit()) {
		// Queue merge to be performed the end of the multicaret edit.
		multicaret_edit_merge_queued = true;
		return;
	}

	multicaret_edit_merge_queued = false;
	multicaret_edit_ignore_carets.clear();

	if (get_caret_count() == 1) {
		return;
	}

	Vector<int> sorted_carets = get_sorted_carets(true);
	for (int i = 0; i < sorted_carets.size() - 1; i++) {
		int first_caret = sorted_carets[i];
		int second_caret = sorted_carets[i + 1];

		bool merge_carets;
		if (!has_selection(first_caret) || !has_selection(second_caret)) {
			// Merge if touching.
			merge_carets =
				get_selection_from_line(second_caret) < get_selection_to_line(first_caret) ||
				(get_selection_from_line(second_caret) == get_selection_to_line(first_caret) &&
					get_selection_from_column(second_caret) <=
						get_selection_to_column(first_caret));
		}
		else {
			// Merge two selections if overlapping.
			merge_carets =
				get_selection_from_line(second_caret) < get_selection_to_line(first_caret) ||
				(get_selection_from_line(second_caret) == get_selection_to_line(first_caret) &&
					get_selection_from_column(second_caret) < get_selection_to_column(first_caret));
		}

		if (!merge_carets) {
			continue;
		}

		// Save the newest one for Click + Drag.
		int caret_to_save = first_caret;
		int caret_to_remove = second_caret;
		if (first_caret < second_caret) {
			caret_to_save = second_caret;
			caret_to_remove = first_caret;
		}

		if (get_selection_from_line(caret_to_save) != get_selection_from_line(caret_to_remove) ||
			get_selection_to_line(caret_to_save) != get_selection_to_line(caret_to_remove) ||
			get_selection_from_column(caret_to_save) !=
				get_selection_from_column(caret_to_remove) ||
			get_selection_to_column(caret_to_save) != get_selection_to_column(caret_to_remove)) {
			// Selections are not the same, merge them into one bigger selection.
			int new_from_line = MIN(
				get_selection_from_line(caret_to_remove), get_selection_from_line(caret_to_save));
			int new_to_line =
				MAX(get_selection_to_line(caret_to_remove), get_selection_to_line(caret_to_save));
			int new_from_col;
			int new_to_col;
			if (get_selection_from_line(caret_to_remove) < get_selection_from_line(caret_to_save)) {
				new_from_col = get_selection_from_column(caret_to_remove);
			}
			else if (get_selection_from_line(caret_to_remove) >
					   get_selection_from_line(caret_to_save)) {
				new_from_col = get_selection_from_column(caret_to_save);
			}
			else {
				new_from_col = MIN(get_selection_from_column(caret_to_remove),
					get_selection_from_column(caret_to_save));
			}
			if (get_selection_to_line(caret_to_remove) < get_selection_to_line(caret_to_save)) {
				new_to_col = get_selection_to_column(caret_to_save);
			}
			else if (get_selection_to_line(caret_to_remove) >
					   get_selection_to_line(caret_to_save)) {
				new_to_col = get_selection_to_column(caret_to_remove);
			}
			else {
				new_to_col = MAX(get_selection_to_column(caret_to_remove),
					get_selection_to_column(caret_to_save));
			}

			// Use the direction from the last caret or the saved one.
			int caret_dir_to_copy;
			if (has_selection(caret_to_remove) && has_selection(caret_to_save)) {
				caret_dir_to_copy =
					caret_to_remove == get_caret_count() - 1 ? caret_to_remove : caret_to_save;
			}
			else {
				caret_dir_to_copy =
					!has_selection(caret_to_remove) ? caret_to_save : caret_to_remove;
			}

			if (is_caret_after_selection_origin(caret_dir_to_copy)) {
				select(new_from_line, new_from_col, new_to_line, new_to_col, caret_to_save);
			}
			else {
				select(new_to_line, new_to_col, new_from_line, new_from_col, caret_to_save);
			}
		}

		if (caret_to_save == 0) {
			adjust_viewport_to_caret(caret_to_save);
		}
		remove_caret(caret_to_remove);

		// Update the rest of the sorted list.
		for (int j = i; j < sorted_carets.size(); j++) {
			if (sorted_carets[j] > caret_to_remove) {
				// Shift the index since a caret before it was removed.
				sorted_carets.write[j] -= 1;
			}
		}
		// Remove the caret from the sorted array.
		sorted_carets.remove_at(caret_to_remove == first_caret ? i : i + 1);

		// Process the caret again, since it and the next caret might also overlap.
		i--;
	}
}

void TextEdit::select(
	int p_origin_line, int p_origin_column, int p_caret_line, int p_caret_column, int p_caret)
{
}

// Starts a multicaret edit operation. Call this before iterating over the carets and call
// [end_multicaret_edit] afterwards.
void TextEdit::begin_multicaret_edit()
{
	if (!multi_carets_enabled) {
		return;
	}
	multicaret_edit_count++;
}

void TextEdit::end_multicaret_edit()
{
	if (!multi_carets_enabled) {
		return;
	}
	if (multicaret_edit_count > 0) {
		multicaret_edit_count--;
	}
	if (multicaret_edit_count != 0) {
		return;
	}

	// This was the last multicaret edit operation.
	if (multicaret_edit_merge_queued) {
		merge_overlapping_carets();
	}
	multicaret_edit_ignore_carets.clear();
}

bool TextEdit::is_in_mulitcaret_edit() const { return multicaret_edit_count > 0; }

bool TextEdit::multicaret_edit_ignore_caret(int p_caret) const
{
	return multicaret_edit_ignore_carets.has(p_caret);
}

bool TextEdit::is_caret_visible(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), false);
	return carets[p_caret].visible;
}

Point2 TextEdit::get_caret_draw_pos(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), Point2(0, 0));
	return carets[p_caret].draw_pos;
}

void TextEdit::set_caret_line(
	int p_line, bool p_adjust_viewport, bool p_can_be_hidden, int p_wrap_index, int p_caret)
{
	ERR_FAIL_INDEX(p_caret, carets.size());
	if (setting_caret_line) {
		return;
	}

	setting_caret_line = true;
	p_line = CLAMP(p_line, 0, text.size() - 1);

	if (!p_can_be_hidden) {
		if (_is_line_hidden(p_line)) {
			int move_down = get_next_visible_line_offset_from(p_line, 1) - 1;
			if (p_line + move_down <= text.size() - 1 && !_is_line_hidden(p_line + move_down)) {
				p_line += move_down;
			}
			else {
				int move_up = get_next_visible_line_offset_from(p_line, -1) - 1;
				if (p_line - move_up > 0 && !_is_line_hidden(p_line - move_up)) {
					p_line -= move_up;
				}
				else {
					WARN_PRINT("Caret set to hidden line " + itos(p_line) +
							   " and there are no nonhidden lines.");
				}
			}
		}
	}
	bool caret_moved = get_caret_line(p_caret) != p_line;
	carets.write[p_caret].line = p_line;

	int n_col;
	if (p_wrap_index >= 0) {
		// Keep caret in same visual x position it was at previously.
		n_col = _get_char_pos_for_line(carets[p_caret].last_fit_x, p_line, p_wrap_index);
		if (n_col != 0 && get_line_wrapping_mode() != LineWrappingMode::LINE_WRAPPING_NONE &&
			p_wrap_index < get_line_wrap_count(p_line)) {
			// Offset by one to not go past the end of the wrapped line.
			if (n_col >= text.get_line_wrap_ranges(p_line)[p_wrap_index].y) {
				n_col -= 1;
			}
		}
	}
	else {
		// Clamp the column.
		n_col = MIN(get_caret_column(p_caret), get_line(p_line).length());
	}
	caret_moved = (caret_moved || get_caret_column(p_caret) != n_col);
	carets.write[p_caret].column = n_col;

	// Unselect if the caret moved to the selection origin.
	if (p_wrap_index >= 0 && has_selection(p_caret) &&
		get_caret_line(p_caret) == get_selection_origin_line(p_caret) &&
		get_caret_column(p_caret) == get_selection_origin_column(p_caret)) {
		deselect(p_caret);
	}

	if (is_inside_tree() && p_adjust_viewport) {
		adjust_viewport_to_caret(p_caret);
	}

	setting_caret_line = false;

	if (caret_moved) {
		_caret_changed(p_caret);
	}
	queue_accessibility_update();
}

int TextEdit::get_caret_line(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), 0);
	return carets[p_caret].line;
}

void TextEdit::set_caret_column(int p_column, bool p_adjust_viewport, int p_caret)
{
	ERR_FAIL_INDEX(p_caret, carets.size());

	p_column = CLAMP(p_column, 0, get_line(get_caret_line(p_caret)).length());

	bool caret_moved = get_caret_column(p_caret) != p_column;
	carets.write[p_caret].column = p_column;

	carets.write[p_caret].last_fit_x = _get_column_x_offset_for_line(
		get_caret_column(p_caret), get_caret_line(p_caret), get_caret_column(p_caret));

	if (!has_selection(p_caret)) {
		// Set the selection origin last fit x to be the same, so we can tell if there was a
		// selection.
		carets.write[p_caret].selection.origin_last_fit_x = carets[p_caret].last_fit_x;
	}

	// Unselect if the caret moved to the selection origin.
	if (has_selection(p_caret) && get_caret_line(p_caret) == get_selection_origin_line(p_caret) &&
		get_caret_column(p_caret) == get_selection_origin_column(p_caret)) {
		deselect(p_caret);
	}

	if (is_inside_tree() && p_adjust_viewport) {
		adjust_viewport_to_caret(p_caret);
	}

	if (caret_moved) {
		_caret_changed(p_caret);
	}
	queue_accessibility_update();
}

int TextEdit::get_caret_column(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), 0);
	return carets[p_caret].column;
}

int TextEdit::get_caret_wrap_index(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), 0);
	return get_line_wrap_index_at_column(get_caret_line(p_caret), get_caret_column(p_caret));
}

String TextEdit::get_word_under_caret(int p_caret) const
{
	ERR_FAIL_COND_V(p_caret >= carets.size() || p_caret < -1, "");

	StringBuilder selected_text;
	for (int c = 0; c < carets.size(); c++) {
		if (p_caret != -1 && p_caret != c) {
			continue;
		}

		PackedInt32Array words =
			TS->shaped_text_get_word_breaks(text.get_line_data(get_caret_line(c))->get_rid());
		for (int i = 0; i < words.size(); i = i + 2) {
			if (words[i] <= get_caret_column(c) && words[i + 1] >= get_caret_column(c)) {
				selected_text += text[get_caret_line(c)].substr(words[i], words[i + 1] - words[i]);
				if (p_caret == -1 && c != carets.size() - 1) {
					selected_text += "\n";
				}
			}
		}
	}
	return selected_text.as_string();
}

void TextEdit::set_selecting_enabled(bool p_enabled)
{
	if (selecting_enabled == p_enabled) {
		return;
	}

	selecting_enabled = p_enabled;

	if (!selecting_enabled) {
		deselect();
	}
}

bool TextEdit::is_selecting_enabled() const { return selecting_enabled; }

void TextEdit::set_deselect_on_focus_loss_enabled(bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && has_selection() && !has_focus()) {
		deselect();
	}
}

bool TextEdit::is_deselect_on_focus_loss_enabled() const { return deselect_on_focus_loss_enabled; }

void TextEdit::set_drag_and_drop_selection_enabled(bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool TextEdit::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

void TextEdit::set_selection_mode(SelectionMode p_mode) { selecting_mode = p_mode; }

TextEdit::SelectionMode TextEdit::get_selection_mode() const { return selecting_mode; }

void TextEdit::select_all()
{
	_push_current_op();
	if (!selecting_enabled) {
		return;
	}

	if (text.size() == 1 && text[0].is_empty()) {
		return;
	}

	remove_secondary_carets();
	set_selection_mode(SelectionMode::SELECTION_MODE_SHIFT);
	select(0, 0, text.size() - 1, text[text.size() - 1].length());
}

void TextEdit::select_word_under_caret(int p_caret)
{
	ERR_FAIL_COND(p_caret >= carets.size() || p_caret < -1);

	_push_current_op();
	if (!selecting_enabled) {
		return;
	}

	if (text.size() == 1 && text[0].is_empty()) {
		return;
	}

	set_selection_mode(SELECTION_MODE_NONE);

	for (int c = 0; c < carets.size(); c++) {
		if (p_caret != -1 && p_caret != c) {
			continue;
		}

		if (has_selection(c)) {
			// Allow toggling selection by pressing the shortcut a second time.
			// This is also usable as a general-purpose "deselect" shortcut after
			// selecting anything.
			deselect(c);
			continue;
		}

		int begin = 0;
		int end = 0;
		const PackedInt32Array words =
			TS->shaped_text_get_word_breaks(text.get_line_data(get_caret_line(c))->get_rid());
		for (int i = 0; i < words.size(); i = i + 2) {
			if ((words[i] <= get_caret_column(c) && words[i + 1] >= get_caret_column(c)) ||
				(i == words.size() - 2 && get_caret_column(c) == words[i + 1])) {
				begin = words[i];
				end = words[i + 1];
				break;
			}
		}

		// No word found.
		if (begin == 0 && end == 0) {
			continue;
		}

		select(get_caret_line(c), begin, get_caret_line(c), end, c);
	}
	merge_overlapping_carets();
}

void TextEdit::add_selection_for_next_occurrence()
{
	if (!selecting_enabled || !is_multiple_carets_enabled()) {
		return;
	}

	if (text.size() == 1 && text[0].is_empty()) {
		return;
	}

	_push_current_op();
	// Always use the last caret, to correctly search for
	// the next occurrence that comes after this caret.
	int caret = get_caret_count() - 1;

	if (!has_selection(caret)) {
		select_word_under_caret(caret);
		return;
	}

	set_selection_mode(SELECTION_MODE_NONE);

	const String& highlighted_text = get_selected_text(caret);
	int column = get_selection_from_column(caret) + 1;
	int line = get_selection_from_line(caret);

	const Point2i next_occurrence = search(highlighted_text, SEARCH_MATCH_CASE, line, column);

	if (next_occurrence.x == -1 || next_occurrence.y == -1) {
		return;
	}

	int to_column = get_selection_to_column(caret) + 1;
	int end = next_occurrence.x + (to_column - column);
	int new_caret = add_caret(next_occurrence.y, end);

	if (new_caret != -1) {
		select(next_occurrence.y, next_occurrence.x, next_occurrence.y, end, new_caret);
		_unhide_carets();
		adjust_viewport_to_caret(new_caret);
		merge_overlapping_carets();
	}
}

void TextEdit::skip_selection_for_next_occurrence()
{
	if (!selecting_enabled) {
		return;
	}

	if (text.size() == 1 && text[0].is_empty()) {
		return;
	}

	set_selection_mode(SELECTION_MODE_NONE);

	// Always use the last caret, to correctly search for
	// the next occurrence that comes after this caret.
	int caret = get_caret_count() - 1;

	// Supports getting the text under caret without selecting it.
	// It allows to use this shortcut to simply jump to the next (under caret) word.
	// Due to const and &(reference) presence, ternary operator is a way to avoid errors and
	// warnings.
	const String& searched_text =
		has_selection(caret) ? get_selected_text(caret) : get_word_under_caret(caret);

	int column = get_selection_from_column(caret) + 1;
	int line = get_selection_from_line(caret);

	const Point2i next_occurrence = search(searched_text, SEARCH_MATCH_CASE, line, column);

	if (next_occurrence.x == -1 || next_occurrence.y == -1) {
		return;
	}

	int to_column = get_selection_to_column(caret) + 1;
	int end = next_occurrence.x + (to_column - column);
	int new_caret = add_caret(next_occurrence.y, end);

	if (new_caret != -1) {
		select(next_occurrence.y, next_occurrence.x, next_occurrence.y, end, new_caret);
		_unhide_carets();
		adjust_viewport_to_caret(new_caret);
		merge_overlapping_carets();
	}

	// Deselect word under previous caret.
	if (has_selection(caret)) {
		select_word_under_caret(caret);
	}

	// Remove previous caret.
	if (get_caret_count() > 1) {
		remove_caret(caret);
	}
}

bool TextEdit::has_selection(int p_caret) const
{
	ERR_FAIL_COND_V(p_caret >= carets.size() || p_caret < -1, false);
	if (p_caret >= 0) {
		return carets[p_caret].selection.active;
	}
	for (int i = 0; i < carets.size(); i++) {
		if (carets[i].selection.active) {
			return true;
		}
	}
	return false;
}

String TextEdit::get_selected_text(int p_caret)
{
	ERR_FAIL_COND_V(p_caret >= carets.size() || p_caret < -1, "");

	if (p_caret >= 0) {
		if (!has_selection(p_caret)) {
			return "";
		}
		return _base_get_text(get_selection_from_line(p_caret), get_selection_from_column(p_caret),
			get_selection_to_line(p_caret), get_selection_to_column(p_caret));
	}

	StringBuilder selected_text;
	Vector<int> sorted_carets = get_sorted_carets();
	for (int i = 0; i < sorted_carets.size(); i++) {
		int caret_index = sorted_carets[i];

		if (!has_selection(caret_index)) {
			continue;
		}
		if (selected_text.get_string_length() != 0) {
			selected_text += "\n";
		}
		selected_text += _base_get_text(get_selection_from_line(caret_index),
			get_selection_from_column(caret_index), get_selection_to_line(caret_index),
			get_selection_to_column(caret_index));
	}

	return selected_text.as_string();
}

int TextEdit::get_selection_at_line_column(
	int p_line, int p_column, bool p_include_edges, bool p_only_selections) const
{
	// Return the caret index of the found selection, or -1.
	for (int i = 0; i < get_caret_count(); i++) {
		if (_selection_contains(i, p_line, p_column, p_include_edges, p_only_selections)) {
			return i;
		}
	}
	return -1;
}

Vector<Point2i> TextEdit::get_line_ranges_from_carets(
	bool p_only_selections, bool p_merge_adjacent) const
{
	// Get a series of line ranges that cover all lines that have a caret or selection.
	// For each Point2i range, x is the first line and y is the last line.
	Vector<Point2i> ret;
	int last_to_line = INT_MIN;

	Vector<int> sorted_carets = get_sorted_carets();
	for (int i = 0; i < sorted_carets.size(); i++) {
		int caret_index = sorted_carets[i];
		if (p_only_selections && !has_selection(caret_index)) {
			continue;
		}
		Point2i range =
			Point2i(get_selection_from_line(caret_index), get_selection_to_line(caret_index));
		if (has_selection(caret_index) && get_selection_to_column(caret_index) == 0) {
			// Dont include selection end line if it ends at column 0.
			range.y--;
		}
		if (range.x == last_to_line || (p_merge_adjacent && range.x - 1 == last_to_line)) {
			// Merge if starts on the same line or adjacent line.
			ret.write[ret.size() - 1].y = range.y;
		}
		else {
			ret.append(range);
		}
		last_to_line = range.y;
	}
	return ret;
}

void TextEdit::set_selection_origin_line(
	int p_line, bool p_can_be_hidden, int p_wrap_index, int p_caret)
{
	if (!selecting_enabled) {
		return;
	}
	ERR_FAIL_INDEX(p_caret, carets.size());
	p_line = CLAMP(p_line, 0, text.size() - 1);

	if (!p_can_be_hidden) {
		if (_is_line_hidden(p_line)) {
			int move_down = get_next_visible_line_offset_from(p_line, 1) - 1;
			if (p_line + move_down <= text.size() - 1 && !_is_line_hidden(p_line + move_down)) {
				p_line += move_down;
			}
			else {
				int move_up = get_next_visible_line_offset_from(p_line, -1) - 1;
				if (p_line - move_up > 0 && !_is_line_hidden(p_line - move_up)) {
					p_line -= move_up;
				}
				else {
					WARN_PRINT("Selection origin set to hidden line " + itos(p_line) +
							   " and there are no nonhidden lines.");
				}
			}
		}
	}

	bool selection_moved = get_selection_origin_line(p_caret) != p_line;
	carets.write[p_caret].selection.origin_line = p_line;

	int n_col;
	if (p_wrap_index >= 0) {
		// Keep selection origin in same visual x position it was at previously.
		n_col = _get_char_pos_for_line(
			carets[p_caret].selection.origin_last_fit_x, p_line, p_wrap_index);
		if (n_col != 0 && get_line_wrapping_mode() != LineWrappingMode::LINE_WRAPPING_NONE &&
			p_wrap_index < get_line_wrap_count(p_line)) {
			// Offset by one to not go past the end of the wrapped line.
			if (n_col >= text.get_line_wrap_ranges(p_line)[p_wrap_index].y) {
				n_col -= 1;
			}
		}
	}
	else {
		// Clamp the column.
		n_col = MIN(get_selection_origin_column(p_caret), get_line(p_line).length());
	}
	selection_moved = (selection_moved || get_selection_origin_column(p_caret) != n_col);
	carets.write[p_caret].selection.origin_column = n_col;

	// Unselect if the selection origin moved to the caret.
	if (p_wrap_index >= 0 && has_selection(p_caret) &&
		get_caret_line(p_caret) == get_selection_origin_line(p_caret) &&
		get_caret_column(p_caret) == get_selection_origin_column(p_caret)) {
		deselect(p_caret);
	}

	if (selection_moved && has_selection(p_caret)) {
		_selection_changed(p_caret);
	}
}

void TextEdit::set_selection_origin_column(int p_column, int p_caret)
{
	if (!selecting_enabled) {
		return;
	}
	ERR_FAIL_INDEX(p_caret, carets.size());

	p_column = CLAMP(p_column, 0, get_line(get_selection_origin_line(p_caret)).length());

	bool selection_moved = get_selection_origin_column(p_caret) != p_column;

	carets.write[p_caret].selection.origin_column = p_column;

	carets.write[p_caret].selection.origin_last_fit_x =
		_get_column_x_offset_for_line(get_selection_origin_column(p_caret),
			get_selection_origin_line(p_caret), get_selection_origin_column(p_caret));

	// Unselect if the selection origin moved to the caret.
	if (has_selection(p_caret) && get_caret_line(p_caret) == get_selection_origin_line(p_caret) &&
		get_caret_column(p_caret) == get_selection_origin_column(p_caret)) {
		deselect(p_caret);
	}

	if (get_selection_mode() == SELECTION_MODE_NONE ||
		get_selection_mode() == SELECTION_MODE_SHIFT) {
		carets.write[p_caret].selection.word_begin_column = p_column;
		carets.write[p_caret].selection.word_end_column = p_column;
	}

	if (selection_moved && has_selection(p_caret)) {
		_selection_changed(p_caret);
	}
}

int TextEdit::get_selection_origin_line(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	return carets[p_caret].selection.origin_line;
}

int TextEdit::get_selection_origin_column(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	return carets[p_caret].selection.origin_column;
}

int TextEdit::get_next_composite_character_column(int p_line, int p_column) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), -1);
	ERR_FAIL_INDEX_V(p_column, text[p_line].length() + 1, -1);
	if (p_column == text[p_line].length()) {
		return p_column;
	}
	else {
		return TS->shaped_text_next_character_pos(
			text.get_line_data(p_line)->get_rid(), (p_column));
	}
}

int TextEdit::get_previous_composite_character_column(int p_line, int p_column) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), -1);
	ERR_FAIL_INDEX_V(p_column, text[p_line].length() + 1, -1);
	if (p_column == 0) {
		return 0;
	}
	else {
		return TS->shaped_text_prev_character_pos(text.get_line_data(p_line)->get_rid(), p_column);
	}
}

int TextEdit::get_selection_from_line(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	if (!has_selection(p_caret)) {
		return carets[p_caret].line;
	}
	return MIN(carets[p_caret].selection.origin_line, carets[p_caret].line);
}

int TextEdit::get_selection_from_column(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	if (!has_selection(p_caret)) {
		return carets[p_caret].column;
	}
	if (carets[p_caret].selection.origin_line < carets[p_caret].line) {
		return carets[p_caret].selection.origin_column;
	}
	else if (carets[p_caret].selection.origin_line > carets[p_caret].line) {
		return carets[p_caret].column;
	}
	else {
		return MIN(carets[p_caret].selection.origin_column, carets[p_caret].column);
	}
}

int TextEdit::get_selection_to_line(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	if (!has_selection(p_caret)) {
		return carets[p_caret].line;
	}
	return MAX(carets[p_caret].selection.origin_line, carets[p_caret].line);
}

int TextEdit::get_selection_to_column(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), -1);
	if (!has_selection(p_caret)) {
		return carets[p_caret].column;
	}
	if (carets[p_caret].selection.origin_line < carets[p_caret].line) {
		return carets[p_caret].column;
	}
	else if (carets[p_caret].selection.origin_line > carets[p_caret].line) {
		return carets[p_caret].selection.origin_column;
	}
	else {
		return MAX(carets[p_caret].selection.origin_column, carets[p_caret].column);
	}
}

bool TextEdit::is_caret_after_selection_origin(int p_caret) const
{
	ERR_FAIL_INDEX_V(p_caret, carets.size(), false);
	if (!has_selection(p_caret)) {
		return true;
	}
	return carets[p_caret].line > carets[p_caret].selection.origin_line ||
		   (carets[p_caret].line == carets[p_caret].selection.origin_line &&
			   carets[p_caret].column >= carets[p_caret].selection.origin_column);
}

void TextEdit::set_selection_handle_enabled(bool p_enabled)
{
	selection_handle_enabled = p_enabled;
}

bool TextEdit::is_selection_handle_enabled() const { return selection_handle_enabled; }

TextEdit::LineWrappingMode TextEdit::get_line_wrapping_mode() const { return line_wrapping_mode; }

TextServer::AutowrapMode TextEdit::get_autowrap_mode() const { return autowrap_mode; }

bool TextEdit::is_line_wrapped(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), false);
	if (get_line_wrapping_mode() == LineWrappingMode::LINE_WRAPPING_NONE) {
		return false;
	}
	return text.get_line_wrap_amount(p_line) > 0;
}

int TextEdit::get_line_wrap_count(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	if (!is_line_wrapped(p_line)) {
		return 0;
	}

	return text.get_line_wrap_amount(p_line);
}

int TextEdit::get_line_wrap_index_at_column(int p_line, int p_column) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	ERR_FAIL_COND_V(p_column < 0, 0);
	ERR_FAIL_COND_V(p_column > text.get_text_with_ime(p_line).length(), 0);

	if (!is_line_wrapped(p_line)) {
		return 0;
	}

	// Loop through wraps in the line text until we get to the column.
	const Vector<Vector2i> line_ranges = text.get_line_wrap_ranges(p_line);
	for (int i = 0; i < line_ranges.size(); i++) {
		if (line_ranges[i].y > p_column) {
			return i;
		}
	}
	return line_ranges.is_empty() ? 0 : line_ranges.size() - 1;
}

Vector<String> TextEdit::get_line_wrapped_text(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Vector<String>());

	Vector<String> lines;
	if (!is_line_wrapped(p_line)) {
		lines.push_back(text.get_text_with_ime(p_line));
		return lines;
	}

	const String& line_text = text.get_text_with_ime(p_line);
	const Vector<Vector2i> line_ranges = text.get_line_wrap_ranges(p_line);
	lines.reserve(line_ranges.size());
	for (int i = 0; i < line_ranges.size(); i++) {
		lines.push_back(line_text.substr(line_ranges[i].x, line_ranges[i].y - line_ranges[i].x));
	}

	return lines;
}

void TextEdit::set_smooth_scroll_enabled(bool p_enabled)
{
	v_scroll->set_smooth_scroll_enabled(p_enabled);
	smooth_scroll_enabled = p_enabled;
}

bool TextEdit::is_smooth_scroll_enabled() const { return smooth_scroll_enabled; }

bool TextEdit::is_scroll_past_end_of_file_enabled() const
{
	return scroll_past_end_of_file_enabled;
}

RID TextEdit::get_text_canvas_item() const { return text_ci; }

ScrollBar* TextEdit::get_v_scroll_bar() const { return v_scroll; }

ScrollBar* TextEdit::get_h_scroll_bar() const { return h_scroll; }

void TextEdit::set_v_scroll(double p_scroll)
{
	v_scroll->set_value(p_scroll);
	int max_v_scroll = v_scroll->get_max() - v_scroll->get_page();
	if (p_scroll >= max_v_scroll - 1.0) {
		_scroll_moved(v_scroll->get_value());
	}
	queue_accessibility_update();
}

double TextEdit::get_v_scroll() const { return v_scroll->get_value(); }

void TextEdit::set_h_scroll(int p_scroll)
{
	if (p_scroll < 0) {
		p_scroll = 0;
	}
	h_scroll->set_value(p_scroll);
	queue_accessibility_update();
}

int TextEdit::get_h_scroll() const { return h_scroll->get_value(); }

void TextEdit::set_v_scroll_speed(float p_speed)
{
	// Prevent setting a vertical scroll speed value under 1.
	ERR_FAIL_COND(p_speed < 1.0);
	v_scroll_speed = p_speed;
}

float TextEdit::get_v_scroll_speed() const { return v_scroll_speed; }

bool TextEdit::is_fit_content_height_enabled() const { return fit_content_height; }

bool TextEdit::is_fit_content_width_enabled() const { return fit_content_width; }

double TextEdit::get_scroll_pos_for_line(int p_line, int p_wrap_index) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	ERR_FAIL_COND_V(p_wrap_index < 0, 0);
	ERR_FAIL_COND_V(p_wrap_index > get_line_wrap_count(p_line), 0);

	if (get_line_wrapping_mode() == LineWrappingMode::LINE_WRAPPING_NONE && !_is_hiding_enabled()) {
		return p_line;
	}

	double new_line_scroll_pos = 0.0;
	if (p_line > 0) {
		new_line_scroll_pos = get_visible_line_count_in_range(0, MIN(p_line - 1, text.size() - 1));
	}
	new_line_scroll_pos += p_wrap_index;
	return new_line_scroll_pos;
}

void TextEdit::set_line_as_first_visible(int p_line, int p_wrap_index)
{
	ERR_FAIL_INDEX(p_line, text.size());
	ERR_FAIL_COND(p_wrap_index < 0);
	ERR_FAIL_COND(p_wrap_index > get_line_wrap_count(p_line));
	set_v_scroll(get_scroll_pos_for_line(p_line, p_wrap_index));

	scrolling = false;
	minimap_clicked = false;
}

int TextEdit::get_first_visible_line() const
{
	return CLAMP(first_visible_line, 0, text.size() - 1);
}

void TextEdit::set_line_as_center_visible(int p_line, int p_wrap_index)
{
	ERR_FAIL_INDEX(p_line, text.size());
	ERR_FAIL_COND(p_wrap_index < 0);
	ERR_FAIL_COND(p_wrap_index > get_line_wrap_count(p_line));

	scrolling = false;
	minimap_clicked = false;

	int visible_rows = get_visible_line_count();
	Point2i next_line =
		get_next_visible_line_index_offset_from(p_line, p_wrap_index, (-visible_rows / 2) - 1);
	int first_line = p_line - next_line.x + 1;

	if (first_line < 0) {
		set_v_scroll(0);
		return;
	}
	set_v_scroll(get_scroll_pos_for_line(first_line, next_line.y));
}

void TextEdit::set_line_as_last_visible(int p_line, int p_wrap_index)
{
	ERR_FAIL_INDEX(p_line, text.size());
	ERR_FAIL_COND(p_wrap_index < 0);
	ERR_FAIL_COND(p_wrap_index > get_line_wrap_count(p_line));

	scrolling = false;
	minimap_clicked = false;

	Point2i next_line = get_next_visible_line_index_offset_from(
		p_line, p_wrap_index, -get_visible_line_count() - 1);
	int first_line = p_line - next_line.x + 1;

	// Adding _get_visible_lines_offset is not 100% correct as we end up showing almost p_line + 1,
	// however, it provides a better user experience. Therefore we need to special case < visible
	// line count, else showing line 0 is impossible.
	if (get_visible_line_count_in_range(0, p_line) < get_visible_line_count() + 1) {
		set_v_scroll(0);
		return;
	}
	set_v_scroll(get_scroll_pos_for_line(first_line, next_line.y) + _get_visible_lines_offset());
}

int TextEdit::get_last_full_visible_line() const
{
	int first_vis_line = get_first_visible_line();
	int last_vis_line = 0;
	last_vis_line = first_vis_line +
					get_next_visible_line_index_offset_from(
						first_vis_line, first_visible_line_wrap_ofs, get_visible_line_count())
						.x -
					1;
	last_vis_line = CLAMP(last_vis_line, 0, text.size() - 1);
	return last_vis_line;
}

int TextEdit::get_last_full_visible_line_wrap_index() const
{
	int first_vis_line = get_first_visible_line();
	return get_next_visible_line_index_offset_from(
		first_vis_line, first_visible_line_wrap_ofs, get_visible_line_count())
		.y;
}

int TextEdit::get_visible_line_count() const { return _get_control_height() / get_line_height(); }

int TextEdit::get_visible_line_count_in_range(int p_from_line, int p_to_line) const
{
	ERR_FAIL_INDEX_V(p_from_line, text.size(), 0);
	ERR_FAIL_INDEX_V(p_to_line, text.size(), 0);

	// So we can handle inputs in whatever order.
	if (p_from_line > p_to_line) {
		SWAP(p_from_line, p_to_line);
	}

	// Returns the total number of (lines + wrapped - hidden).
	if (!_is_hiding_enabled() && get_line_wrapping_mode() == LineWrappingMode::LINE_WRAPPING_NONE) {
		return (p_to_line - p_from_line) + 1;
	}

	int total_rows = 0;
	for (int i = p_from_line; i <= p_to_line; i++) {
		if (!text.is_hidden(i)) {
			total_rows++;
			total_rows += get_line_wrap_count(i);
		}
	}
	return total_rows;
}

int TextEdit::get_total_visible_line_count() const { return text.get_total_visible_line_count(); }

bool TextEdit::is_line_in_viewport(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), false);

	int line_wrap = get_line_wrap_index_at_column(p_line, 0);

	int first_vis_line = get_first_visible_line();
	int first_vis_wrap = first_visible_line_wrap_ofs;
	int last_vis_line = get_last_full_visible_line();
	int last_vis_wrap = get_last_full_visible_line_wrap_index();

	if (p_line < first_vis_line || (p_line == first_vis_line && p_line < first_vis_wrap)) {
		// Caret is above screen.
		return false;
	}
	else if (p_line > last_vis_line || (p_line == last_vis_line && line_wrap > last_vis_wrap)) {
		// Caret is below screen.
		return false;
	}
	return true;
}

void TextEdit::adjust_viewport_to_caret(int p_caret)
{
	ERR_FAIL_INDEX(p_caret, carets.size());

	// Move viewport so the caret is visible on the screen vertically.

	int cur_line = get_caret_line(p_caret);
	int cur_wrap = get_caret_wrap_index(p_caret);

	int first_vis_line = get_first_visible_line();
	int first_vis_wrap = first_visible_line_wrap_ofs;
	int last_vis_line = get_last_full_visible_line();
	int last_vis_wrap = get_last_full_visible_line_wrap_index();

	if (cur_line < first_vis_line || (cur_line == first_vis_line && cur_wrap < first_vis_wrap)) {
		// Caret is above screen.
		set_line_as_first_visible(cur_line, cur_wrap);
	}
	else if (cur_line > last_vis_line ||
			   (cur_line == last_vis_line && cur_wrap > last_vis_wrap)) {
		// Caret is below screen.
		set_line_as_last_visible(cur_line, cur_wrap);
	}

	_adjust_viewport_to_caret_horizontally(p_caret, false);
}

void TextEdit::center_viewport_to_caret(int p_caret)
{
	ERR_FAIL_INDEX(p_caret, carets.size());

	// Move viewport so the caret is in the center of the screen vertically.
	scrolling = false;
	minimap_clicked = false;

	set_line_as_center_visible(get_caret_line(p_caret), get_caret_wrap_index(p_caret));

	_adjust_viewport_to_caret_horizontally(p_caret);
}

bool TextEdit::is_drawing_minimap() const { return draw_minimap; }

int TextEdit::get_minimap_width() const { return minimap_width; }

int TextEdit::get_minimap_visible_lines() const
{
	return _get_control_height() / (minimap_char_size.y + minimap_line_spacing);
}

int TextEdit::get_gutter_count() const { return gutters.size(); }

void TextEdit::set_gutter_name(int p_gutter, const String& p_name)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	gutters.write[p_gutter].name = p_name;
}

void TextEdit::add_gutter(int p_at)
{
	ERR_FAIL_COND(p_at < -1 || p_at > gutters.size());
	if (p_at < 0 || p_at > gutters.size()) {
		gutters.push_back(GutterInfo());
	}
	else {
		gutters.insert(p_at, GutterInfo());
	}
	text.add_gutter(p_at);
	_update_gutter_width();
	queue_redraw();
}

String TextEdit::get_gutter_name(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), "");
	return gutters[p_gutter].name;
}

TextEdit::GutterType TextEdit::get_gutter_type(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), GUTTER_TYPE_STRING);
	return gutters[p_gutter].type;
}

void TextEdit::set_gutter_width(int p_gutter, int p_width)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	if (gutters[p_gutter].width == p_width) {
		return;
	}
	gutters.write[p_gutter].width = p_width;
	_update_gutter_width();
}

int TextEdit::get_gutter_width(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), -1);
	return gutters[p_gutter].width;
}

int TextEdit::get_total_gutter_width() const { return gutters_width + gutter_padding; }

void TextEdit::set_gutter_draw(int p_gutter, bool p_draw)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	if (gutters[p_gutter].draw == p_draw) {
		return;
	}
	gutters.write[p_gutter].draw = p_draw;
	_update_gutter_width();
}

bool TextEdit::is_gutter_drawn(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), false);
	return gutters[p_gutter].draw;
}

bool TextEdit::is_gutter_clickable(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), false);
	return gutters[p_gutter].clickable;
}

void TextEdit::set_gutter_overwritable(int p_gutter, bool p_overwritable)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	gutters.write[p_gutter].overwritable = p_overwritable;
}

bool TextEdit::is_gutter_overwritable(int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), false);
	return gutters[p_gutter].overwritable;
}

String TextEdit::get_line_gutter_text(int p_line, int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), "");
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), "");
	return text.get_line_gutter_text(p_line, p_gutter);
}

Ref<Texture2D> TextEdit::get_line_gutter_icon(int p_line, int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Ref<Texture2D>());
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), Ref<Texture2D>());
	return text.get_line_gutter_icon(p_line, p_gutter);
}

Color TextEdit::get_line_gutter_item_color(int p_line, int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Color());
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), Color());
	return text.get_line_gutter_item_color(p_line, p_gutter);
}

void TextEdit::set_line_gutter_clickable(int p_line, int p_gutter, bool p_clickable)
{
	ERR_FAIL_INDEX(p_line, text.size());
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	text.set_line_gutter_clickable(p_line, p_gutter, p_clickable);
}

bool TextEdit::is_line_gutter_clickable(int p_line, int p_gutter) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), false);
	ERR_FAIL_INDEX_V(p_gutter, gutters.size(), false);
	return text.is_line_gutter_clickable(p_line, p_gutter);
}

Color TextEdit::get_line_background_color(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), Color());
	return text.get_line_background_color(p_line);
}

Ref<CodeHighlighter> TextEdit::get_syntax_highlighter() const { return syntax_highlighter; }

bool TextEdit::is_highlight_current_line_enabled() const { return highlight_current_line; }

bool TextEdit::is_highlight_all_occurrences_enabled() const { return highlight_all_occurrences; }

void TextEdit::set_use_default_word_separators(bool p_enabled)
{
	text.set_use_default_word_separators(p_enabled);
}

bool TextEdit::is_default_word_separators_enabled() const
{
	return text.is_default_word_separators_enabled();
}

// Set word separators. Combine default separators with custom separators if those options are
// enabled.
void TextEdit::set_custom_word_separators(const String& p_separators)
{
	text.set_custom_word_separators(p_separators);
}

void TextEdit::Text::set_custom_word_separators(const String& p_separators)
{
	if (custom_word_separators == p_separators) {
		return;
	}
	custom_word_separators = p_separators;
	invalidate_all_lines();
}

bool TextEdit::is_custom_word_separators_enabled() const
{
	return text.is_custom_word_separators_enabled();
}

String TextEdit::get_custom_word_separators() const { return text.get_custom_word_separators(); }

// Enable or disable custom word separators.
void TextEdit::set_use_custom_word_separators(bool p_enabled)
{
	text.set_use_custom_word_separators(p_enabled);
}

String TextEdit::get_default_word_separators() const { return text.get_default_word_separators(); }

bool TextEdit::get_draw_control_chars() const { return draw_control_chars; }

bool TextEdit::is_drawing_tabs() const { return draw_tabs; }

bool TextEdit::is_drawing_spaces() const { return draw_spaces; }

Color TextEdit::get_font_color() const { return theme_cache.font_color; }

bool TextEdit::_is_hiding_enabled() const { return hiding_enabled; }

bool TextEdit::_is_line_hidden(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), false);
	return text.is_hidden(p_line);
}

void TextEdit::_unhide_carets()
{
	if (!code_mode && !hiding_enabled) {
		return;
	}
	for (int i = 0; i < get_caret_count(); i++) {
		if (_is_line_hidden(get_caret_line(i))) {
			unfold_line(get_caret_line(i));
		}
		if (has_selection(i) && _is_line_hidden(get_selection_origin_line(i))) {
			unfold_line(get_selection_origin_line(i));
		}
	}
}

void TextEdit::_copy_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (has_selection(p_caret)) {
		DisplayServer::clipboard_set(get_selected_text(p_caret));
		cut_copy_line = "";
		return;
	}

	if (!empty_selection_clipboard_enabled) {
		return;
	}

	// Copy full lines.
	StringBuilder clipboard;
	Vector<Point2i> line_ranges;
	if (p_caret == -1) {
		// When there are multiple carets on a line, only copy it once.
		line_ranges = get_line_ranges_from_carets(false, true);
	}
	else {
		line_ranges.push_back(Point2i(get_caret_line(p_caret), get_caret_line(p_caret)));
	}
	for (Point2i line_range : line_ranges) {
		for (int i = line_range.x; i <= line_range.y; i++) {
			if (text[i].length() != 0) {
				clipboard += _base_get_text(i, 0, i, text[i].length());
			}
			clipboard += "\n";
		}
	}

	String clipboard_string = clipboard.as_string();
	DisplayServer::clipboard_set(clipboard_string);
	// Set the cut copy line so we know to paste as a line.
	if (get_caret_count() == 1) {
		cut_copy_line = clipboard_string;
	}
	else {
		cut_copy_line = "";
	}
}

Key TextEdit::_get_menu_action_accelerator(const String& p_action)
{
	const List<InputEvent>* events = InputMap::action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<InputEvent>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const InputEvent& event = first_event->get();
	if (event.type != InputEventType::KEY) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero.
	if (event.key.physical_keycode != Key::NONE) {
		return event.get_physical_keycode_with_modifiers();
	}
	else {
		return event.get_keycode_with_modifiers();
	}
}

void TextEdit::_push_current_op()
{
	if (pending_action_end) {
		start_action(EditAction::ACTION_NONE);
		return;
	}
	if (current_op.type == TextOperation::TYPE_NONE) {
		return; // Nothing to do.
	}

	if (next_operation_is_complex) {
		current_op.chain_forward = true;
		next_operation_is_complex = false;
	}

	undo_stack.push_back(current_op);
	current_op.type = TextOperation::TYPE_NONE;
	current_op.text = "";
	current_op.chain_forward = false;

	if (undo_stack.size() > undo_stack_max_size) {
		undo_stack.pop_front();
	}
}

void TextEdit::_do_text_op(const TextOperation& p_op, bool p_reverse)
{
	ERR_FAIL_COND(p_op.type == TextOperation::TYPE_NONE);

	bool insert = p_op.type == TextOperation::TYPE_INSERT;
	if (p_reverse) {
		insert = !insert;
	}

	if (insert) {
		int check_line;
		int check_column;
		_base_insert_text(p_op.from_line, p_op.from_column, p_op.text, check_line, check_column);
		ERR_FAIL_COND(check_line != p_op.to_line);	   // BUG.
		ERR_FAIL_COND(check_column != p_op.to_column); // BUG.
	}
	else {
		_base_remove_text(p_op.from_line, p_op.from_column, p_op.to_line, p_op.to_column);
	}
}

void TextEdit::_clear_redo()
{
	if (undo_stack_pos == nullptr) {
		return; // Nothing to clear.
	}

	_push_current_op();

	while (undo_stack_pos) {
		List<TextOperation>::Element* elem = undo_stack_pos;
		undo_stack_pos = undo_stack_pos->next();
		undo_stack.erase(elem);
	}
}

int TextEdit::_get_column_pos_of_word(
	const String& p_key, const String& p_search, uint32_t p_search_flags, int p_from_column) const
{
	int col = -1;

	if (p_key.length() > 0 && p_search.length() > 0) {
		if (p_from_column < 0 || p_from_column > p_search.length()) {
			p_from_column = 0;
		}

		bool key_start_is_symbol = is_symbol(p_key[0]);
		bool key_end_is_symbol = is_symbol(p_key[p_key.length() - 1]);

		while (col == -1 && p_from_column <= p_search.length()) {
			if (p_search_flags & SEARCH_MATCH_CASE) {
				col = p_search.find(p_key, p_from_column);
			}
			else {
				col = p_search.findn(p_key, p_from_column);
			}

			// If not found, just break early to improve performance.
			if (col == -1) {
				break;
			}

			// Whole words only.
			if (col != -1 && p_search_flags & SEARCH_WHOLE_WORDS) {
				p_from_column = col;

				if (!key_start_is_symbol && col > 0 && !is_symbol(p_search[col - 1])) {
					col = -1;
				}
				else if (!key_end_is_symbol && (col + p_key.length()) < p_search.length() &&
						   !is_symbol(p_search[col + p_key.length()])) {
					col = -1;
				}
			}

			p_from_column += 1;
		}
	}
	return col;
}

int TextEdit::_get_char_pos_for_line(int p_px, int p_line, int p_wrap_index) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);
	p_wrap_index = MIN(p_wrap_index, text.get_line_data(p_line)->get_line_count() - 1);

	RID text_rid = text.get_line_data(p_line)->get_line_rid(p_wrap_index);
	const float wrap_indent = _get_wrap_indent_offset(p_line, p_wrap_index, is_layout_rtl());

	if (is_layout_rtl()) {
		p_px = TS->shaped_text_get_size(text_rid).x - p_px + wrap_indent;
	}
	else {
		p_px -= wrap_indent;
	}
	int ofs = TS->shaped_text_hit_test_position(text_rid, p_px);
	if (ofs == -1) {
		return 0;
	}
	if (!caret_mid_grapheme_enabled) {
		ofs = TS->shaped_text_closest_character_pos(text_rid, ofs);
	}
	return ofs;
}

void TextEdit::_set_caret_pos_dirty(bool p_dirty) { caret_pos_dirty = p_dirty; }

int TextEdit::_get_column_x_offset_for_line(int p_char, int p_line, int p_column) const
{
	ERR_FAIL_INDEX_V(p_line, text.size(), 0);

	int wrap_index = 0;
	Vector<Vector2i> wrap_ranges = text.get_line_wrap_ranges(p_line);
	for (int i = 0; i < wrap_ranges.size(); i++) {
		if ((p_char >= wrap_ranges[i].x) &&
			(p_char < wrap_ranges[i].y ||
				(i == wrap_ranges.size() - 1 && p_char == wrap_ranges[i].y))) {
			wrap_index = i;
			break;
		}
	}

	RID text_rid = text.get_line_data(p_line)->get_line_rid(wrap_index);
	bool rtl = is_layout_rtl();
	const float wrap_indent = _get_wrap_indent_offset(p_line, wrap_index, rtl);

	CaretInfo ts_caret = TS->shaped_text_get_carets(text_rid, p_column);
	if ((ts_caret.l_caret != Rect2() &&
			(ts_caret.l_dir == TextServer::DIRECTION_AUTO ||
				ts_caret.l_dir == (TextServer::Direction)input_direction)) ||
		(ts_caret.t_caret == Rect2())) {
		return ts_caret.l_caret.position.x + (rtl ? -wrap_indent : wrap_indent);
	}
	else {
		return ts_caret.t_caret.position.x + (rtl ? -wrap_indent : wrap_indent);
	}
}

bool TextEdit::_is_line_col_in_range(int p_line, int p_column, int p_from_line, int p_from_column,
	int p_to_line, int p_to_column, bool p_include_edges) const
{
	if (p_line >= p_from_line && p_line <= p_to_line &&
		(p_line > p_from_line || p_column > p_from_column) &&
		(p_line < p_to_line || p_column < p_to_column)) {
		return true;
	}
	if (p_include_edges) {
		if ((p_line == p_from_line && p_column == p_from_column) ||
			(p_line == p_to_line && p_column == p_to_column)) {
			return true;
		}
	}
	return false;
}

void TextEdit::_offset_carets_after(int p_old_line, int p_old_column, int p_new_line,
	int p_new_column, bool p_include_selection_begin, bool p_include_selection_end)
{
	// Moves all carets at or after old_line and old_column.
	// Called after deleting or inserting text so that the carets stay with the text they are at.

	int edit_height = p_new_line - p_old_line;
	int edit_size = p_new_column - p_old_column;
	if (edit_height == 0 && edit_size == 0) {
		return;
	}

	// Intentionally includes carets in the multicaret_edit_ignore list so that they are moved
	// together.
	for (int i = 0; i < get_caret_count(); i++) {
		bool selected = has_selection(i);
		bool caret_at_end = selected && is_caret_after_selection_origin(i);
		bool include_caret_at = caret_at_end ? p_include_selection_end : p_include_selection_begin;

		// Move caret.
		int caret_line = get_caret_line(i);
		int caret_column = get_caret_column(i);
		bool caret_after =
			caret_line > p_old_line || (caret_line == p_old_line && caret_column > p_old_column);
		bool caret_at = caret_line == p_old_line && caret_column == p_old_column;
		if (caret_after || (caret_at && include_caret_at)) {
			caret_line += edit_height;
			if (caret_line == p_new_line) {
				caret_column += edit_size;
			}

			if (edit_height != 0) {
				set_caret_line(caret_line, false, true, -1, i);
			}
			set_caret_column(caret_column, false, i);
		}

		// Move selection origin.
		if (!selected) {
			continue;
		}
		bool include_selection_origin_at =
			!caret_at_end ? p_include_selection_end : p_include_selection_begin;

		int selection_origin_line = get_selection_origin_line(i);
		int selection_origin_column = get_selection_origin_column(i);
		bool selection_origin_after =
			selection_origin_line > p_old_line ||
			(selection_origin_line == p_old_line && selection_origin_column > p_old_column);
		bool selection_origin_at =
			selection_origin_line == p_old_line && selection_origin_column == p_old_column;
		if (selection_origin_after || (selection_origin_at && include_selection_origin_at)) {
			selection_origin_line += edit_height;
			if (selection_origin_line == p_new_line) {
				selection_origin_column += edit_size;
			}
			select(selection_origin_line, selection_origin_column, caret_line, caret_column, i);
		}
	}
	if (!p_include_selection_begin && p_include_selection_end && has_selection()) {
		// It is possible that two adjacent selections now overlap.
		merge_overlapping_carets();
	}
}

void TextEdit::_cancel_drag_and_drop_text()
{
	// Cancel the drag operation if drag originated from here.
	if (selection_drag_attempt && get_viewport()) {
		get_viewport()->gui_cancel_drag();
	}
}

void TextEdit::_click_selection_held()
{
	// Update the selection mode on a timer so it is updated when the view scrolls even if the mouse
	// isn't moving.
	if (!Input::is_mouse_button_pressed(MouseButton::LEFT)) {
		click_select_held->stop();
		return;
	}
	click_select_held->stop();
}

void TextEdit::_pre_shift_selection(int p_caret)
{
	if (!selecting_enabled) {
		return;
	}

	set_selection_mode(SelectionMode::SELECTION_MODE_SHIFT);
	if (has_selection(p_caret)) {
		return;
	}
	// Prepare selection to start at current caret position.
	set_selection_origin_line(get_caret_line(p_caret), true, -1, p_caret);
	set_selection_origin_column(get_caret_column(p_caret), p_caret);
	carets.write[p_caret].selection.active = true;
}

bool TextEdit::_selection_contains(
	int p_caret, int p_line, int p_column, bool p_include_edges, bool p_only_selections) const
{
	if (!has_selection(p_caret)) {
		return !p_only_selections && p_line == get_caret_line(p_caret) &&
			   p_column == get_caret_column(p_caret);
	}
	return _is_line_col_in_range(p_line, p_column, get_selection_from_line(p_caret),
		get_selection_from_column(p_caret), get_selection_to_line(p_caret),
		get_selection_to_column(p_caret), p_include_edges);
}

void TextEdit::_update_wrap_at_column(bool p_force)
{
	int new_wrap_at = get_size().width - _get_current_stylebox()->get_minimum_size().width -
					  gutters_width - gutter_padding;
	if (draw_minimap) {
		new_wrap_at -= minimap_width;
	}
	if (v_scroll->is_visible_in_tree()) {
		new_wrap_at -= v_scroll->get_bound_minimum_size().width;
	}
	/* Give it a little more space. */
	new_wrap_at -= theme_cache.wrap_offset;

	if ((wrap_at_column != new_wrap_at) || p_force) {
		wrap_at_column = new_wrap_at;
		if (line_wrapping_mode) {
			uint32_t autowrap_flags = TextServer::BREAK_MANDATORY;
			switch (autowrap_mode) {
			case TextServer::AUTOWRAP_WORD_SMART:
				autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE |
								 TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_WORD:
				autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_ARBITRARY:
				autowrap_flags = TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_OFF:
				break;
			}
			text.set_brk_flags(autowrap_flags);
			text.set_width(wrap_at_column);
			text.invalidate_all_lines();
			_update_placeholder();
		}
		else if (text.get_width() != -1) {
			text.set_width(-1);
			text.invalidate_all_lines();
			_update_placeholder();
		}
	}

	// Update viewport.
	int first_vis_line = get_first_visible_line();
	if (is_line_wrapped(first_vis_line)) {
		first_visible_line_wrap_ofs =
			MIN(first_visible_line_wrap_ofs, get_line_wrap_count(first_vis_line));
	}
	else {
		first_visible_line_wrap_ofs = 0;
	}
	set_line_as_first_visible(first_visible_line, first_visible_line_wrap_ofs);
	queue_accessibility_update();
}

int TextEdit::_get_control_height() const
{
	int control_height = get_size().height - _get_current_stylebox()->get_minimum_size().height;
	if (h_scroll->is_visible_in_tree()) {
		control_height -= h_scroll->get_size().height;
	}
	return control_height;
}

void TextEdit::_v_scroll_input()
{
	scrolling = false;
	minimap_clicked = false;
}

double TextEdit::_get_visible_lines_offset() const
{
	double total = _get_control_height();
	total /= (double)get_line_height();
	total = total - std::floor(total);
	total = -CLAMP(total, 0.001, 1) + 1;
	return total;
}

double TextEdit::_get_v_scroll_offset() const
{
	double val = get_v_scroll() - std::floor(get_v_scroll());
	return CLAMP(val, 0, 1);
}

void TextEdit::_scroll_up(real_t p_delta, bool p_animate)
{
	if (scrolling && smooth_scroll_enabled &&
		SIGN(target_v_scroll - v_scroll->get_value()) != SIGN(-p_delta)) {
		scrolling = false;
		minimap_clicked = false;
	}

	if (scrolling) {
		target_v_scroll = (target_v_scroll - p_delta);
	}
	else {
		target_v_scroll = (get_v_scroll() - p_delta);
	}

	if (smooth_scroll_enabled) {
		if (target_v_scroll <= 0) {
			target_v_scroll = 0;
		}
		if (!p_animate || Math::abs(target_v_scroll - v_scroll->get_value()) < 1.0) {
			v_scroll->set_value(target_v_scroll);
			queue_accessibility_update();
		}
		else {
			scrolling = true;
			set_process_internal(true);
		}
	}
	else {
		set_v_scroll(target_v_scroll);
	}
}

void TextEdit::_scroll_down(real_t p_delta, bool p_animate)
{
	if (scrolling && smooth_scroll_enabled &&
		SIGN(target_v_scroll - v_scroll->get_value()) != SIGN(p_delta)) {
		scrolling = false;
		minimap_clicked = false;
	}

	if (scrolling) {
		target_v_scroll = (target_v_scroll + p_delta);
	}
	else {
		target_v_scroll = (get_v_scroll() + p_delta);
	}

	if (smooth_scroll_enabled) {
		double max_v_scroll = v_scroll->get_max() - v_scroll->get_page();
		if (target_v_scroll > max_v_scroll) {
			target_v_scroll = max_v_scroll;
		}
		if (!p_animate || Math::abs(target_v_scroll - v_scroll->get_value()) < 1.0) {
			v_scroll->set_value(target_v_scroll);
			queue_accessibility_update();
		}
		else {
			scrolling = true;
			set_process_internal(true);
		}
	}
	else {
		set_v_scroll(target_v_scroll);
	}
}

void TextEdit::_scroll_lines_up()
{
	scrolling = false;
	minimap_clicked = false;

	// Adjust the vertical scroll.
	set_v_scroll(get_v_scroll() - 1);

	// Adjust the caret to viewport.
	for (int i = 0; i < carets.size(); i++) {
		if (has_selection(i)) {
			continue;
		}

		int last_vis_line = get_last_full_visible_line();
		int last_vis_wrap = get_last_full_visible_line_wrap_index();
		if (get_caret_line(i) > last_vis_line ||
			(get_caret_line(i) == last_vis_line && get_caret_wrap_index(i) > last_vis_wrap)) {
			set_caret_line(last_vis_line, false, false, last_vis_wrap, i);
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_scroll_lines_down()
{
	scrolling = false;
	minimap_clicked = false;

	// Adjust the vertical scroll.
	set_v_scroll(get_v_scroll() + 1);

	// Adjust the caret to viewport.
	for (int i = 0; i < carets.size(); i++) {
		if (has_selection(i)) {
			continue;
		}

		int first_vis_line = get_first_visible_line();
		if (get_caret_line(i) < first_vis_line ||
			(get_caret_line(i) == first_vis_line &&
				get_caret_wrap_index(i) < first_visible_line_wrap_ofs)) {
			set_caret_line(first_vis_line, false, false, first_visible_line_wrap_ofs, i);
		}
	}
	merge_overlapping_carets();
}

void TextEdit::_update_minimap_click()
{
	Point2 mp = get_local_mouse_pos();

	int xmargin_end =
		get_size().width - Math::floor(_get_current_stylebox()->get_margin(SIDE_RIGHT));
	if (!dragging_minimap && (mp.x < xmargin_end - minimap_width || mp.x > xmargin_end)) {
		minimap_clicked = false;
		return;
	}
	minimap_clicked = true;
	dragging_minimap = true;

	int row = get_minimap_line_at_pos(mp);

	if (row >= get_first_visible_line() &&
		(row < get_last_full_visible_line() || row >= (text.size() - 1))) {
		minimap_scroll_ratio = v_scroll->get_as_ratio();
		minimap_scroll_click_pos = mp.y;
		can_drag_minimap = true;
		return;
	}

	Point2i next_line =
		get_next_visible_line_index_offset_from(row, 0, -get_visible_line_count() / 2);
	int first_line = MAX(0, row - next_line.x + 1);
	double delta = get_scroll_pos_for_line(first_line, next_line.y) - get_v_scroll();
	if (delta < 0) {
		_scroll_up(-delta, true);
	}
	else {
		_scroll_down(delta, true);
	}
}

void TextEdit::_update_minimap_drag()
{
	if (!can_drag_minimap) {
		return;
	}

	int control_height = _get_control_height();
	int scroll_height = v_scroll->get_max() * (minimap_char_size.y + minimap_line_spacing);
	if (control_height > scroll_height) {
		control_height = scroll_height;
	}

	Point2 mp = get_local_mouse_pos();

	double diff = (mp.y - minimap_scroll_click_pos) / control_height;
	v_scroll->set_as_ratio(minimap_scroll_ratio + diff);
}

Vector2i TextEdit::_get_hovered_gutter(const Point2& p_mouse_pos) const
{
	int left_margin = get_line_start_margin();
	if (p_mouse_pos.x > left_margin + gutters_width + gutter_padding) {
		return Vector2i(-1, -1);
	}
	int hovered_row = get_line_column_at_pos(p_mouse_pos, false).y;
	if (hovered_row == -1) {
		return Vector2i(-1, -1);
	}
	for (int i = 0; i < gutters.size(); i++) {
		if (!gutters[i].draw || gutters[i].width <= 0) {
			continue;
		}

		if (p_mouse_pos.x >= left_margin && p_mouse_pos.x < left_margin + gutters[i].width) {
			return Vector2i(i, hovered_row);
		}

		left_margin += gutters[i].width;
	}
	return Vector2i(-1, -1);
}

void TextEdit::_clear_syntax_highlighting_cache() { syntax_highlighting_cache.clear(); }

#ifndef DISABLE_DEPRECATED
Vector<int> TextEdit::get_caret_index_edit_order()
{
	Vector<int> carets_order = get_sorted_carets();
	carets_order.reverse();
	return carets_order;
}

void TextEdit::adjust_carets_after_edit(
	int p_caret, int p_from_line, int p_from_col, int p_to_line, int p_to_col)
{
}

int TextEdit::get_selection_line(int p_caret) const { return get_selection_origin_line(p_caret); }

int TextEdit::get_selection_column(int p_caret) const
{
	return get_selection_origin_column(p_caret);
}
#endif

String TextEdit::_base_get_text(
	int p_from_line, int p_from_column, int p_to_line, int p_to_column) const
{
	ERR_FAIL_INDEX_V(p_from_line, text.size(), String());
	ERR_FAIL_INDEX_V(p_from_column, text[p_from_line].length() + 1, String());
	ERR_FAIL_INDEX_V(p_to_line, text.size(), String());
	ERR_FAIL_INDEX_V(p_to_column, text[p_to_line].length() + 1, String());
	ERR_FAIL_COND_V(p_to_line < p_from_line, String()); // 'from > to'.
	ERR_FAIL_COND_V(
		p_to_line == p_from_line && p_to_column < p_from_column, String()); // 'from > to'.

	StringBuilder ret;

	for (int i = p_from_line; i <= p_to_line; i++) {
		int begin = (i == p_from_line) ? p_from_column : 0;
		int end = (i == p_to_line) ? p_to_column : text[i].length();

		if (i > p_from_line) {
			ret += "\n";
		}
		ret += text[i].substr(begin, end - begin);
	}

	return ret.as_string();
}

void TextEdit::_draw_rect_unfilled(RID p_canvas_item, const Rect2& p_rect, const Color& p_color,
	real_t p_width, bool p_antialiased) const
{
	Rect2 rect = p_rect.abs();

	if (p_width >= rect.size.width || p_width >= rect.size.height) {
		Renderer::canvas_item_add_rect(
			p_canvas_item, rect.grow(0.5f * p_width), p_color, p_antialiased);
	}
	else {
		Vector<Vector2> points;
		points.resize(5);
		points.write[0] = rect.position;
		points.write[1] = rect.position + Vector2(rect.size.x, 0);
		points.write[2] = rect.position + rect.size;
		points.write[3] = rect.position + Vector2(0, rect.size.y);
		points.write[4] = rect.position;

		Vector<Color> colors = {p_color};

		Renderer::canvas_item_add_polyline(
			p_canvas_item, points, colors, p_width, p_antialiased);
	}
}

TextEdit::TextEdit(const String& p_placeholder)
{
	v_scroll = memnew(ScrollBar(VERTICAL));
	add_child(v_scroll, false, INTERNAL_MODE_FRONT);
	h_scroll = memnew(ScrollBar(HORIZONTAL));
	add_child(h_scroll, false, INTERNAL_MODE_FRONT);

	caret_blink_timer = memnew(Timer);
	caret_blink_timer->set_wait_time(0.65);
	add_child(caret_blink_timer, false, INTERNAL_MODE_FRONT);

	idle_detect = memnew(Timer);
	idle_detect->set_wait_time(3);
	idle_detect->set_one_shot(true);
	add_child(idle_detect, false, INTERNAL_MODE_FRONT);

	click_select_held = memnew(Timer);
	click_select_held->set_wait_time(0.05);
	add_child(click_select_held, false, INTERNAL_MODE_FRONT);

	carets.resize(1);
	text.clear();
	placeholder_data_buf.instantiate();
	placeholder_text = p_placeholder;
	editable = true;
	set_focus_mode(FOCUS_ALL);
}

TextEdit::~TextEdit()
{
	_clear_line_number_text_cache();
	if (text_ci.is_valid()) {
		Renderer::free_rid(text_ci);
	}
}

void TextEdit::set_code_mode(bool p_enabled)
{
	if (code_mode == p_enabled) {
		return;
	}
	code_mode = p_enabled;
	if (code_mode) {
		_ensure_code_gutters();
		if (!code.symbol_tooltip_timer) {
			code.symbol_tooltip_timer = memnew(Timer);
			code.symbol_tooltip_timer->set_wait_time(0.5);
			code.symbol_tooltip_timer->set_one_shot(true);
			add_child(code.symbol_tooltip_timer, false, INTERNAL_MODE_FRONT);
		}
		if (code.delimiters.is_empty()) {
			add_string_delimiter("\"", "\"", false);
			add_string_delimiter("'", "'", false);
		}
		if (code.auto_brace_completion_pairs.is_empty()) {
			add_auto_brace_completion_pair("(", ")");
			add_auto_brace_completion_pair("{", "}");
			add_auto_brace_completion_pair("[", "]");
			add_auto_brace_completion_pair("\"", "\"");
			add_auto_brace_completion_pair("'", "'");
		}
	}
}

void TextEdit::_ensure_code_gutters()
{
	if (code.main_gutter != -1) {
		return;
	}
	add_gutter(0);
	set_gutter_name(0, "main_gutter");
	set_gutter_draw(0, false);
	set_gutter_type(0, GUTTER_TYPE_CUSTOM);

	add_gutter(1);
	set_gutter_name(1, "line_numbers");
	set_gutter_draw(1, false);
	set_gutter_type(1, GUTTER_TYPE_CUSTOM);
	set_gutter_clickable(1, true);

	add_gutter(2);
	set_gutter_name(2, "fold_gutter");
	set_gutter_draw(2, false);
	set_gutter_type(2, GUTTER_TYPE_CUSTOM);

	_update_gutter_indexes();
}

void TextEdit::_update_scrollbars() {}

void TextEdit::_update_placeholder() {}

void TextEdit::_update_gutter_width() {}

void TextEdit::set_line_background_color(int p_line, Color const& p_color)
{
	ERR_FAIL_INDEX(p_line, text.size());
	text.set_line_background_color(p_line, p_color);
}

void TextEdit::_set_line_as_hidden(int p_line, bool p_hidden)
{
	ERR_FAIL_INDEX(p_line, text.size());
	text.set_hidden(p_line, p_hidden);
}

void TextEdit::_adjust_viewport_to_caret_horizontally(int, bool) {}

void TextEdit::_update_ime_text() {}

void TextEdit::_set_text(String const&, bool) {}

void TextEdit::set_tab_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size <= 0, "Tab size must be greater than 0.");
	text.set_tab_size(p_size);
}

void TextEdit::set_syntax_highlighter(Ref<CodeHighlighter> p_syntax_highlighter)
{
	if (syntax_highlighter == p_syntax_highlighter) {
		return;
	}
	if (syntax_highlighter.is_valid()) {
		syntax_highlighter->set_text_edit(nullptr);
	}
	syntax_highlighter = p_syntax_highlighter;
	if (syntax_highlighter.is_valid()) {
		syntax_highlighter->set_text_edit(this);
	}
	_clear_syntax_highlighting_cache();
}

void TextEdit::_set_symbol_lookup_word(String const& p_symbol) { lookup_symbol_word = p_symbol; }

void TextEdit::_set_hiding_enabled(bool p_enabled) { hiding_enabled = p_enabled; }

void TextEdit::set_gutter_type(int p_gutter, GutterType p_type)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	gutters.write[p_gutter].type = p_type;
}

void TextEdit::set_gutter_clickable(int p_gutter, bool p_clickable)
{
	ERR_FAIL_INDEX(p_gutter, gutters.size());
	gutters.write[p_gutter].clickable = p_clickable;
}

void TextEdit::set_draw_control_chars(bool p_enabled)
{
	draw_control_chars = p_enabled;
	text.set_draw_control_chars(p_enabled);
}

void TextEdit::_scroll_moved(double) {}

Control::CursorShape TextEdit::get_cursor_shape(Vector2 const& p_pos) const
{
	if (code_mode) {
		if (!code.symbol_lookup_word.is_empty()) {
			return CURSOR_POINTING_HAND;
		}

		if (!is_dragging_cursor()) {
			if ((code.code_completion_active && code.code_completion_rect.has_point(p_pos)) ||
				(!is_editable() && (!is_selecting_enabled() || get_line_count() == 0))) {
				return CURSOR_ARROW;
			}

			if (code.code_completion_active && code.code_completion_scroll_rect.has_point(p_pos)) {
				return CURSOR_ARROW;
			}

			Point2i pos = get_line_column_at_pos(p_pos, false);
			int line = pos.y;
			int col = pos.x;

			if (line != -1 && is_line_folded(line)) {
				int wrap_index = get_line_wrap_index_at_column(line, col);
				if (wrap_index == get_line_wrap_count(line) &&
					theme_cache.folded_eol_icon.is_valid()) {
					int eol_icon_width = theme_cache.folded_eol_icon->get_width();
					int left_margin = get_total_gutter_width() + eol_icon_width +
									  get_line_width(line, wrap_index) - get_h_scroll();
					if (p_pos.x > left_margin && p_pos.x <= left_margin + eol_icon_width + 3) {
						return CURSOR_POINTING_HAND;
					}
				}
			}
		}
	}
	return Control::get_cursor_shape(p_pos);
}

void TextEdit::_generate_context_menu() {}

void TextEdit::clear() {}

void TextEdit::_base_remove_text(int, int, int, int) {}

void TextEdit::_base_insert_text(int, int, String const&, int&, int&) {}

String TextEdit::_get_accessibility_name() const { return String(); }

void TextEdit::_draw_guidelines() {}

/* Code Mode / CodeFeatures implementation */

void TextEdit::set_indent_size(const int p_size)
{
	ERR_FAIL_COND_MSG(p_size <= 0, "Indend size must be greater than 0.");
	if (code.indent_size == p_size) {
		return;
	}

	code.indent_size = p_size;
	if (code.indent_using_spaces) {
		code.indent_text = String(" ").repeat(p_size);
	}
	else {
		code.indent_text = "\t";
	}
	set_tab_size(p_size);
}

int TextEdit::get_indent_size() const { return code.indent_size; }

void TextEdit::set_indent_using_spaces(const bool p_use_spaces)
{
	code.indent_using_spaces = p_use_spaces;
	if (code.indent_using_spaces) {
		code.indent_text = String(" ").repeat(code.indent_size);
	}
	else {
		code.indent_text = "\t";
	}
}

bool TextEdit::is_indent_using_spaces() const { return code.indent_using_spaces; }

void TextEdit::set_auto_indent_enabled(bool p_enabled) { code.auto_indent = p_enabled; }

bool TextEdit::is_auto_indent_enabled() const { return code.auto_indent; }

void TextEdit::set_auto_indent_prefixes(const Vector<String>& p_prefixes)
{
	code.auto_indent_prefixes.clear();
	for (int i = 0; i < p_prefixes.size(); i++) {
		const String prefix = p_prefixes[i];
		if (!prefix.is_empty()) {
			code.auto_indent_prefixes.insert(prefix[0]);
		}
	}
}

Vector<String> TextEdit::get_auto_indent_prefixes() const
{
	Vector<String> prefixes;
	for (const char32_t& E : code.auto_indent_prefixes) {
		prefixes.push_back(String::chr(E));
	}
	return prefixes;
}

int TextEdit::_calculate_spaces_till_next_left_indent(int p_column) const
{
	int spaces_till_indent = p_column % code.indent_size;
	if (spaces_till_indent == 0) {
		spaces_till_indent = code.indent_size;
	}
	return spaces_till_indent;
}

int TextEdit::_calculate_spaces_till_next_right_indent(int p_column) const
{
	return code.indent_size - p_column % code.indent_size;
}

void TextEdit::set_auto_brace_completion_enabled(bool p_enabled)
{
	code.auto_brace_completion_enabled = p_enabled;
}

bool TextEdit::is_auto_brace_completion_enabled() const
{
	return code.auto_brace_completion_enabled;
}

void TextEdit::set_highlight_matching_braces_enabled(bool p_enabled)
{
	highlight_matching_braces_enabled = p_enabled;
	queue_redraw();
}

bool TextEdit::is_highlight_matching_braces_enabled() const
{
	return highlight_matching_braces_enabled;
}

void TextEdit::add_auto_brace_completion_pair(const String& p_open_key, const String& p_close_key)
{
	ERR_FAIL_COND_MSG(p_open_key.is_empty(), "auto brace completion open key cannot be empty");
	ERR_FAIL_COND_MSG(p_close_key.is_empty(), "auto brace completion close key cannot be empty");

	for (int i = 0; i < p_open_key.length(); i++) {
		ERR_FAIL_COND_MSG(
			!is_symbol(p_open_key[i]), "auto brace completion open key must be a symbol");
	}
	for (int i = 0; i < p_close_key.length(); i++) {
		ERR_FAIL_COND_MSG(
			!is_symbol(p_close_key[i]), "auto brace completion close key must be a symbol");
	}

	int at = 0;
	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		ERR_FAIL_COND_MSG(code.auto_brace_completion_pairs[i].open_key == p_open_key,
			"auto brace completion open key '" + p_open_key + "' already exists.");
		if (p_open_key.length() < code.auto_brace_completion_pairs[i].open_key.length()) {
			at++;
		}
	}

	BracePair brace_pair;
	brace_pair.open_key = p_open_key;
	brace_pair.close_key = p_close_key;
	code.auto_brace_completion_pairs.insert(at, brace_pair);
}

bool TextEdit::has_auto_brace_completion_open_key(const String& p_open_key) const
{
	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		if (code.auto_brace_completion_pairs[i].open_key == p_open_key) {
			return true;
		}
	}
	return false;
}

bool TextEdit::has_auto_brace_completion_close_key(const String& p_close_key) const
{
	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		if (code.auto_brace_completion_pairs[i].close_key == p_close_key) {
			return true;
		}
	}
	return false;
}

String TextEdit::get_auto_brace_completion_close_key(const String& p_open_key) const
{
	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		if (code.auto_brace_completion_pairs[i].open_key == p_open_key) {
			return code.auto_brace_completion_pairs[i].close_key;
		}
	}
	return String();
}

void TextEdit::_update_draw_main_gutter()
{
	_ensure_code_gutters();
	set_gutter_draw(
		code.main_gutter, code.draw_breakpoints || code.draw_bookmarks || code.draw_executing_lines);
	int main_gutter_width = get_line_height() / (_is_bookmark_only() ? 2 : 1);
	set_gutter_width(code.main_gutter, main_gutter_width);
}

void TextEdit::set_draw_breakpoints_gutter(bool p_draw)
{
	_ensure_code_gutters();
	code.draw_breakpoints = p_draw;
	set_gutter_clickable(code.main_gutter, p_draw);
	_update_draw_main_gutter();
}

bool TextEdit::is_drawing_breakpoints_gutter() const { return code.draw_breakpoints; }

void TextEdit::set_draw_bookmarks_gutter(bool p_draw)
{
	_ensure_code_gutters();
	code.draw_bookmarks = p_draw;
	_update_draw_main_gutter();
}

bool TextEdit::is_drawing_bookmarks_gutter() const { return code.draw_bookmarks; }

void TextEdit::set_draw_executing_lines_gutter(bool p_draw)
{
	_ensure_code_gutters();
	code.draw_executing_lines = p_draw;
	_update_draw_main_gutter();
}

bool TextEdit::is_drawing_executing_lines_gutter() const { return code.draw_executing_lines; }

void TextEdit::_main_gutter_draw_callback(int p_line, int p_gutter, const Rect2& p_region)
{
	bool hovering = get_hovered_gutter() == Vector2i(code.main_gutter, p_line);
	RID ci = get_text_canvas_item();
	if (code.draw_breakpoints && theme_cache.breakpoint_icon.is_valid()) {
		bool breakpointed = is_line_breakpointed(p_line);
		bool shift_pressed = Input::is_key_pressed(Key::SHIFT);

		if (breakpointed || (hovering && !is_dragging_cursor() && !shift_pressed)) {
			int padding = p_region.size.x / 6;

			Color use_color = theme_cache.breakpoint_color;
			if (hovering && !shift_pressed) {
				use_color = breakpointed ? use_color.lightened(0.3) : use_color.darkened(0.5);
			}
			Rect2 icon_region = p_region;
			icon_region.position += Point2(padding, padding);
			icon_region.size -= Point2(padding, padding) * 2;
			theme_cache.breakpoint_icon->draw_rect(ci, icon_region, false, use_color);
		}
	}

	if (code.draw_bookmarks && theme_cache.bookmark_icon.is_valid()) {
		bool bookmarked = is_line_bookmarked(p_line);
		bool shift_pressed = Input::is_key_pressed(Key::SHIFT);

		if (bookmarked || (hovering && !is_dragging_cursor() && shift_pressed)) {
			int horizontal_padding = p_region.size.x / (_is_bookmark_only() ? 8 : 2);
			int vertical_padding = p_region.size.y / 4;

			Color use_color = theme_cache.bookmark_color;
			if (hovering && shift_pressed) {
				use_color = bookmarked ? use_color.lightened(0.3) : use_color.darkened(0.5);
			}
			Rect2 icon_region = p_region;
			icon_region.position += Point2(horizontal_padding, 0);
			icon_region.size -= Point2(horizontal_padding * 1.1, vertical_padding);
			theme_cache.bookmark_icon->draw_rect(ci, icon_region, false, use_color);
		}
	}

	if (code.draw_executing_lines && is_line_executing(p_line) &&
		theme_cache.executing_line_icon.is_valid()) {
		int horizontal_padding = p_region.size.x / 10;
		int vertical_padding = p_region.size.y / 4;

		Rect2 icon_region = p_region;
		icon_region.position += Point2(horizontal_padding, vertical_padding);
		icon_region.size -= Point2(horizontal_padding, vertical_padding) * 2;
		theme_cache.executing_line_icon->draw_rect(
			ci, icon_region, false, theme_cache.executing_line_color);
	}
}

void TextEdit::set_line_as_breakpoint(int p_line, bool p_breakpointed)
{
	ERR_FAIL_INDEX(p_line, get_line_count());
	if (p_breakpointed) {
		code.breakpointed_lines[p_line] = true;
	}
	else {
		code.breakpointed_lines.erase(p_line);
	}
	queue_redraw();
}

bool TextEdit::is_line_breakpointed(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	return code.breakpointed_lines.has(p_line) && code.breakpointed_lines[p_line];
}

void TextEdit::clear_breakpointed_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_breakpointed(i)) {
			set_line_as_breakpoint(i, false);
		}
	}
}

PackedInt32Array TextEdit::get_breakpointed_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_breakpointed(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void TextEdit::set_line_as_bookmarked(int p_line, bool p_bookmarked)
{
	ERR_FAIL_INDEX(p_line, get_line_count());
	if (p_bookmarked) {
		code.bookmarked_lines[p_line] = true;
	}
	else {
		code.bookmarked_lines.erase(p_line);
	}
	queue_redraw();
}

bool TextEdit::is_line_bookmarked(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	return code.bookmarked_lines.has(p_line) && code.bookmarked_lines[p_line];
}

void TextEdit::clear_bookmarked_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_bookmarked(i)) {
			set_line_as_bookmarked(i, false);
		}
	}
}

PackedInt32Array TextEdit::get_bookmarked_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_bookmarked(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void TextEdit::set_line_as_executing(int p_line, bool p_executing)
{
	ERR_FAIL_INDEX(p_line, get_line_count());
	if (p_executing) {
		code.executing_lines[p_line] = true;
	}
	else {
		code.executing_lines.erase(p_line);
	}
	queue_redraw();
}

bool TextEdit::is_line_executing(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	return code.executing_lines.has(p_line) && code.executing_lines[p_line];
}

void TextEdit::clear_executing_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_executing(i)) {
			set_line_as_executing(i, false);
		}
	}
}

PackedInt32Array TextEdit::get_executing_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_executing(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void TextEdit::set_draw_line_numbers(bool p_draw)
{
	_ensure_code_gutters();
	set_gutter_draw(code.line_number_gutter, p_draw);
}

bool TextEdit::is_draw_line_numbers_enabled() const
{
	return code.line_number_gutter != -1 && is_gutter_drawn(code.line_number_gutter);
}

void TextEdit::set_line_numbers_zero_padded(bool p_zero_padded)
{
	String new_line_number_padding = p_zero_padded ? "0" : " ";
	if (code.line_number_padding == new_line_number_padding) {
		return;
	}
	code.line_number_padding = new_line_number_padding;
	_clear_line_number_text_cache();
	queue_redraw();
}

bool TextEdit::is_line_numbers_zero_padded() const { return code.line_number_padding == "0"; }

void TextEdit::set_line_numbers_min_digits(int p_count)
{
	int new_min = MAX(1, p_count);
	if (code.line_numbers_min_digits == new_min) {
		return;
	}
	code.line_numbers_min_digits = new_min;
	code.line_number_digits = MAX(code.line_number_digits, code.line_numbers_min_digits);
	_clear_line_number_text_cache();
	_update_line_number_gutter_width();
}

int TextEdit::get_line_numbers_min_digits() const { return code.line_numbers_min_digits; }

void TextEdit::_clear_line_number_text_cache()
{
	for (const KeyValue<int, RID>& KV : code.line_number_text_cache) {
		TS->free_rid(KV.value);
	}
	code.line_number_text_cache.clear();
}

void TextEdit::_update_line_number_gutter_width()
{
	if (code.line_number_gutter == -1 || theme_cache.font.is_null()) {
		return;
	}
	int width_in_chars =
		code.line_number_digits +
		(!is_drawing_fold_gutter() ? 1 : 0); // Extra padding if there is no fold gutter.
	set_gutter_width(code.line_number_gutter,
		width_in_chars * theme_cache.font->get_char_size('0', theme_cache.font_size).width);
}

void TextEdit::set_draw_fold_gutter(bool p_draw)
{
	_ensure_code_gutters();
	set_gutter_draw(code.fold_gutter, p_draw);
	_update_line_number_gutter_width();
}

bool TextEdit::is_drawing_fold_gutter() const
{
	return code.fold_gutter != -1 && is_gutter_drawn(code.fold_gutter);
}

void TextEdit::_fold_gutter_draw_callback(int p_line, int p_gutter, Rect2 p_region)
{
	if (!can_fold_line(p_line) && !is_line_folded(p_line)) {
		set_line_gutter_clickable(p_line, code.fold_gutter, false);
		return;
	}
	set_line_gutter_clickable(p_line, code.fold_gutter, true);
	RID ci = get_text_canvas_item();

	int horizontal_padding = p_region.size.x / 10;
	int vertical_padding = p_region.size.y / 6;

	p_region.position += Point2(horizontal_padding, vertical_padding);
	p_region.size -= Point2(horizontal_padding, vertical_padding) * 2;

	bool can_fold = can_fold_line(p_line);

	if (is_line_code_region_start(p_line)) {
		Color region_icon_color = theme_cache.folded_code_region_color;
		region_icon_color.a = MAX(region_icon_color.a, 0.4f);
		if (can_fold) {
			theme_cache.can_fold_code_region_icon->draw_rect(
				ci, p_region, false, region_icon_color);
		}
		else {
			theme_cache.folded_code_region_icon->draw_rect(ci, p_region, false, region_icon_color);
		}
		return;
	}
	if (can_fold) {
		theme_cache.can_fold_icon->draw_rect(ci, p_region, false, theme_cache.code_folding_color);
		return;
	}
	theme_cache.folded_icon->draw_rect(ci, p_region, false, theme_cache.code_folding_color);
}

void TextEdit::set_line_folding_enabled(bool p_enabled)
{
	code.line_folding_enabled = p_enabled;
	_set_hiding_enabled(p_enabled);
}

bool TextEdit::is_line_folding_enabled() const { return code.line_folding_enabled; }

bool TextEdit::can_fold_line(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (!code.line_folding_enabled) {
		return false;
	}

	if (p_line + 1 >= get_line_count() || get_line(p_line).strip_edges().is_empty()) {
		return false;
	}

	if (_is_line_hidden(p_line) || is_line_folded(p_line)) {
		return false;
	}

	// Check for code region.
	if (is_line_code_region_end(p_line)) {
		return false;
	}
	if (is_line_code_region_start(p_line)) {
		int region_level = 0;
		// Check if there is a valid end region tag.
		for (int next_line = p_line + 1; next_line < get_line_count(); next_line++) {
			if (is_line_code_region_end(next_line)) {
				region_level -= 1;
				if (region_level == -1) {
					return true;
				}
			}
			if (is_line_code_region_start(next_line)) {
				region_level += 1;
			}
		}
		return false;
	}

	/* Check for full multiline line or block strings / comments. */
	int in_comment = is_in_comment(p_line);
	int in_string = (in_comment == -1) ? is_in_string(p_line) : -1;
	if (in_string != -1 || in_comment != -1) {
		if (get_delimiter_start_position(p_line, get_line(p_line).size() - 1).y != p_line) {
			return false;
		}

		int delimiter_end_line = get_delimiter_end_position(p_line, get_line(p_line).size() - 1).y;
		/* No end line, therefore we have a multiline region over the rest of the file. */
		if (delimiter_end_line == -1) {
			return true;
		}
		/* End line is the same therefore we have a block. */
		if (delimiter_end_line == p_line) {
			/* Check we are the start of the block. */
			if (p_line - 1 >= 0) {
				if ((in_string != -1 && is_in_string(p_line - 1) != -1) ||
					(in_comment != -1 && is_in_comment(p_line - 1) != -1 &&
						!is_line_code_region_start(p_line - 1) &&
						!is_line_code_region_end(p_line - 1))) {
					return false;
				}
			}
			/* Check it continues for at least one line. */
			return ((in_string != -1 && is_in_string(p_line + 1) != -1) ||
					(in_comment != -1 && is_in_comment(p_line + 1) != -1 &&
						!is_line_code_region_start(p_line + 1) &&
						!is_line_code_region_end(p_line + 1)));
		}
		return ((in_string != -1 && is_in_string(delimiter_end_line) != -1) ||
				(in_comment != -1 && is_in_comment(delimiter_end_line) != -1));
	}

	/* Otherwise check indent levels. */
	int start_indent = get_indent_level(p_line);
	for (int i = p_line + 1; i < get_line_count(); i++) {
		if (is_in_string(i) != -1 || is_in_comment(i) != -1 ||
			get_line(i).strip_edges().is_empty()) {
			continue;
		}
		return (get_indent_level(i) > start_indent);
	}
	return false;
}

bool TextEdit::_fold_line(int p_line)
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (!is_line_folding_enabled() || !can_fold_line(p_line)) {
		return false;
	}

	/* Find the last line to be hidden. */
	const int line_count = get_line_count() - 1;
	int end_line = line_count;

	// Fold code region.
	if (is_line_code_region_start(p_line)) {
		int region_level = 0;
		for (int endregion_line = p_line + 1; endregion_line < get_line_count(); endregion_line++) {
			if (is_line_code_region_start(endregion_line)) {
				region_level += 1;
			}
			if (is_line_code_region_end(endregion_line)) {
				region_level -= 1;
				if (region_level == -1) {
					end_line = endregion_line;
					break;
				}
			}
		}
		set_line_background_color(p_line, theme_cache.folded_code_region_color);
	}

	int in_comment = is_in_comment(p_line);
	int in_string = (in_comment == -1) ? is_in_string(p_line) : -1;
	if (!is_line_code_region_start(p_line)) {
		if (in_string != -1 || in_comment != -1) {
			end_line = get_delimiter_end_position(p_line, get_line(p_line).size() - 1).y;
			// End line is the same therefore we have a block of single line delimiters.
			if (end_line == p_line) {
				for (int i = p_line + 1; i <= line_count; i++) {
					if ((in_string != -1 && is_in_string(i) == -1) ||
						(in_comment != -1 && is_in_comment(i) == -1)) {
						break;
					}
					if (in_comment != -1 &&
						(is_line_code_region_start(i) || is_line_code_region_end(i))) {
						// A code region tag should split a comment block, ending it early.
						break;
					}
					end_line = i;
				}
			}
		}
		else {
			int start_indent = get_indent_level(p_line);
			for (int i = p_line + 1; i <= line_count; i++) {
				if (get_line(i).strip_edges().is_empty()) {
					continue;
				}
				if (get_indent_level(i) > start_indent) {
					end_line = i;
					continue;
				}
				if (is_in_string(i) == -1 && is_in_comment(i) == -1) {
					break;
				}
			}
		}
	}

	for (int i = p_line + 1; i <= end_line; i++) {
		_set_line_as_hidden(i, true);
	}

	// Collapse any carets in the hidden area.
	collapse_carets(p_line, get_line(p_line).length(), end_line, get_line(end_line).length(), true);

	return true;
}

bool TextEdit::_unfold_line(int p_line)
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (!is_line_folded(p_line) && !_is_line_hidden(p_line)) {
		return false;
	}

	int fold_start = p_line;
	for (; fold_start > 0; fold_start--) {
		if (is_line_folded(fold_start)) {
			break;
		}
	}
	fold_start = is_line_folded(fold_start) ? fold_start : p_line;

	for (int i = fold_start + 1; i < get_line_count(); i++) {
		if (!_is_line_hidden(i)) {
			break;
		}
		_set_line_as_hidden(i, false);
		if (is_line_code_region_start(i - 1)) {
			set_line_background_color(i - 1, Color(0.0, 0.0, 0.0, 0.0));
		}
	}
	return true;
}

void TextEdit::fold_line(int p_line)
{
	if (_fold_line(p_line)) {
		queue_redraw();
	}
}

void TextEdit::unfold_line(int p_line)
{
	if (_unfold_line(p_line)) {
		queue_redraw();
	}
}

void TextEdit::fold_all_lines()
{
	bool any_folded = false;
	for (int i = 0; i < get_line_count(); i++) {
		any_folded |= _fold_line(i);
	}
	if (any_folded) {
		queue_redraw();
	}
}

void TextEdit::unfold_all_lines()
{
	bool any_unfolded = false;
	for (int i = 0; i < get_line_count(); i++) {
		any_unfolded |= _unfold_line(i);
	}
	if (any_unfolded) {
		queue_redraw();
	}
}

void TextEdit::toggle_foldable_line(int p_line)
{
	ERR_FAIL_INDEX(p_line, get_line_count());
	if (is_line_folded(p_line)) {
		unfold_line(p_line);
		return;
	}
	fold_line(p_line);
}

void TextEdit::toggle_foldable_lines_at_carets()
{
	begin_multicaret_edit();
	int previous_line = -1;
	Vector<int> sorted = get_sorted_carets();
	for (int caret_idx : sorted) {
		if (multicaret_edit_ignore_caret(caret_idx)) {
			continue;
		}
		int line_idx = get_caret_line(caret_idx);
		if (line_idx != previous_line) {
			toggle_foldable_line(line_idx);
			previous_line = line_idx;
		}
	}
	end_multicaret_edit();
}

int TextEdit::get_folded_line_header(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), 0);
	// Search for the first non hidden line.
	while (p_line > 0) {
		if (!_is_line_hidden(p_line)) {
			break;
		}
		p_line--;
	}
	return p_line;
}

bool TextEdit::is_line_folded(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	return p_line + 1 < get_line_count() && !_is_line_hidden(p_line) && _is_line_hidden(p_line + 1);
}

Vector<int> TextEdit::get_folded_lines_bind() const
{
	Vector<int> folded_lines;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_folded(i)) {
			folded_lines.push_back(i);
		}
	}
	return folded_lines;
}

PackedInt32Array TextEdit::get_folded_lines() const
{
	PackedInt32Array folded_lines;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_folded(i)) {
			folded_lines.push_back(i);
		}
	}
	return folded_lines;
}

void TextEdit::create_code_region() {}

String TextEdit::get_code_region_start_tag() const { return code.code_region_start_tag; }

String TextEdit::get_code_region_end_tag() const { return code.code_region_end_tag; }

void TextEdit::set_code_region_tags(const String& p_start, const String& p_end)
{
	ERR_FAIL_COND_MSG(p_start == p_end, "Starting and ending region tags cannot be identical.");
	ERR_FAIL_COND_MSG(p_start.is_empty(), "Starting region tag cannot be empty.");
	ERR_FAIL_COND_MSG(p_end.is_empty(), "Ending region tag cannot be empty.");
	code.code_region_start_tag = p_start;
	code.code_region_end_tag = p_end;
	_update_code_region_tags();
}

bool TextEdit::is_line_code_region_start(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (code.code_region_start_string.is_empty()) {
		return false;
	}
	if (is_in_string(p_line) != -1) {
		return false;
	}
	Vector<String> split = get_line(p_line).strip_edges().split_spaces(1);
	return split.size() > 0 && split[0] == code.code_region_start_string;
}

bool TextEdit::is_line_code_region_end(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (code.code_region_start_string.is_empty()) {
		return false;
	}
	if (is_in_string(p_line) != -1) {
		return false;
	}
	Vector<String> split = get_line(p_line).strip_edges().split_spaces(1);
	return split.size() > 0 && split[0] == code.code_region_end_string;
}

void TextEdit::add_string_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only)
{
	_add_delimiter(p_start_key, p_end_key, p_line_only, TYPE_STRING);
}

void TextEdit::remove_string_delimiter(const String& p_start_key)
{
	_remove_delimiter(p_start_key, TYPE_STRING);
}

bool TextEdit::has_string_delimiter(const String& p_start_key) const
{
	return _has_delimiter(p_start_key, TYPE_STRING);
}

void TextEdit::set_string_delimiters(const Vector<String>& p_string_delimiters)
{
	_set_delimiters(p_string_delimiters, TYPE_STRING);
}

void TextEdit::clear_string_delimiters() { _clear_delimiters(TYPE_STRING); }

Vector<String> TextEdit::get_string_delimiters() const
{
	return _get_delimiters(TYPE_STRING);
}

int TextEdit::is_in_string(int p_line, int p_column) const
{
	return _is_in_delimiter(p_line, p_column, TYPE_STRING);
}

void TextEdit::add_comment_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only)
{
	_add_delimiter(p_start_key, p_end_key, p_line_only, TYPE_COMMENT);
}

void TextEdit::remove_comment_delimiter(const String& p_start_key)
{
	_remove_delimiter(p_start_key, TYPE_COMMENT);
}

bool TextEdit::has_comment_delimiter(const String& p_start_key) const
{
	return _has_delimiter(p_start_key, TYPE_COMMENT);
}

void TextEdit::set_comment_delimiters(const Vector<String>& p_comment_delimiters)
{
	_set_delimiters(p_comment_delimiters, TYPE_COMMENT);
}

void TextEdit::clear_comment_delimiters() { _clear_delimiters(TYPE_COMMENT); }

Vector<String> TextEdit::get_comment_delimiters() const
{
	return _get_delimiters(TYPE_COMMENT);
}

int TextEdit::is_in_comment(int p_line, int p_column) const
{
	return _is_in_delimiter(p_line, p_column, TYPE_COMMENT);
}

String TextEdit::get_delimiter_start_key(int p_delimiter_idx) const
{
	ERR_FAIL_INDEX_V(p_delimiter_idx, code.delimiters.size(), "");
	return code.delimiters[p_delimiter_idx].start_key;
}

String TextEdit::get_delimiter_end_key(int p_delimiter_idx) const
{
	ERR_FAIL_INDEX_V(p_delimiter_idx, code.delimiters.size(), "");
	return code.delimiters[p_delimiter_idx].end_key;
}

Point2 TextEdit::get_delimiter_start_position(int p_line, int p_column) const
{
	if (code.delimiters.is_empty()) {
		return Point2(-1, -1);
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), Point2(-1, -1));
	ERR_FAIL_COND_V(p_column - 1 > get_line(p_line).size(), Point2(-1, -1));

	Point2 start_position;
	start_position.y = -1;
	start_position.x = -1;

	bool in_region = ((p_line <= 0 || code.delimiter_cache[p_line - 1].size() < 1)
							 ? -1
							 : code.delimiter_cache[p_line - 1].back()->get()) != -1;

	/* Check the keys for this line. */
	for (const KeyValue<int, int>& E : code.delimiter_cache[p_line]) {
		if (E.key > p_column) {
			break;
		}
		in_region = E.value != -1;
		start_position.x = in_region ? E.key : -1;
	}

	/* Region was found on this line and is not a multiline continuation. */
	int line_length = get_line(p_line).length();
	if (start_position.x != -1 && line_length > 0 && start_position.x != line_length + 1) {
		start_position.y = p_line;
		return start_position;
	}

	/* Not in a region */
	if (!in_region) {
		return start_position;
	}

	/* Region starts on a previous line */
	for (int i = p_line - 1; i >= 0; i--) {
		if (code.delimiter_cache[i].size() < 1) {
			continue;
		}
		start_position.y = i;
		start_position.x = code.delimiter_cache[i].back()->key();

		/* Make sure it's not a multiline continuation. */
		line_length = get_line(i).length();
		if (line_length > 0 && start_position.x != line_length + 1) {
			break;
		}
	}
	return start_position;
}

Point2 TextEdit::get_delimiter_end_position(int p_line, int p_column) const
{
	if (code.delimiters.is_empty()) {
		return Point2(-1, -1);
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), Point2(-1, -1));
	ERR_FAIL_COND_V(p_column - 1 > get_line(p_line).size(), Point2(-1, -1));

	Point2 end_position;
	end_position.y = -1;
	end_position.x = -1;

	int region = (p_line <= 0 || code.delimiter_cache[p_line - 1].size() < 1)
					 ? -1
					 : code.delimiter_cache[p_line - 1].back()->value();

	/* Check the keys for this line. */
	for (const KeyValue<int, int>& E : code.delimiter_cache[p_line]) {
		end_position.x = (E.value == -1) ? E.key : -1;
		if (E.key > p_column) {
			break;
		}
		region = E.value;
	}

	/* Region was found on this line and is not a multiline continuation. */
	if (region != -1 && end_position.x != -1 &&
		(code.delimiters[region].line_only || end_position.x != get_line(p_line).length() + 1)) {
		end_position.y = p_line;
		return end_position;
	}

	/* Not in a region */
	if (region == -1) {
		end_position.x = -1;
		return end_position;
	}

	/* Region ends on a later line */
	for (int i = p_line + 1; i < get_line_count(); i++) {
		if (code.delimiter_cache[i].size() < 1 || code.delimiter_cache[i].front()->value() != -1) {
			continue;
		}
		end_position.x = code.delimiter_cache[i].front()->key();

		/* Make sure it's not a multiline continuation. */
		if (get_line(i).length() > 0 && end_position.x != get_line(i).length() + 1) {
			end_position.y = i;
			break;
		}
		end_position.x = -1;
	}
	return end_position;
}

void TextEdit::set_code_hint(const String& p_hint)
{
	if (code.code_hint == p_hint) {
		return;
	}
	code.code_hint = p_hint;
	code.code_hint_xpos = -0xFFFF;
	queue_redraw();
}

void TextEdit::set_code_hint_draw_below(bool p_below)
{
	if (code.code_hint_draw_below == p_below) {
		return;
	}
	code.code_hint_draw_below = p_below;
	queue_redraw();
}

void TextEdit::set_code_completion_enabled(bool p_enable)
{
	code.code_completion_enabled = p_enable;
}

bool TextEdit::is_code_completion_enabled() const { return code.code_completion_enabled; }

void TextEdit::set_code_completion_prefixes(const Vector<String>& p_prefixes)
{
	code.code_completion_prefixes.clear();
	for (int i = 0; i < p_prefixes.size(); i++) {
		const String prefix = p_prefixes[i];
		if (!prefix.is_empty()) {
			code.code_completion_prefixes.insert(prefix[0]);
		}
	}
}

Vector<String> TextEdit::get_code_completion_prefixes() const
{
	Vector<String> prefixes;
	for (const char32_t& E : code.code_completion_prefixes) {
		prefixes.push_back(String::chr(E));
	}
	return prefixes;
}

String TextEdit::get_text_for_code_completion() const
{
	StringBuilder completion_text;
	const int text_size = get_line_count();
	for (int i = 0; i < text_size; i++) {
		String line = get_line(i);

		if (i == get_caret_line()) {
			completion_text += line.substr(0, get_caret_column());
			/* Not unicode, represents the caret. */
			completion_text += String::chr(0xFFFF);
			completion_text += line.substr(get_caret_column());
		}
		else {
			completion_text += line;
		}

		if (i != text_size - 1) {
			completion_text += "\n";
		}
	}
	return completion_text.as_string();
}

void TextEdit::request_code_completion(bool) {}

void TextEdit::update_code_completion_options(bool) {}

int TextEdit::get_code_completion_selected_index() const
{
	return (code.code_completion_active) ? code.code_completion_current_selected : -1;
}

void TextEdit::set_code_completion_selected_index(int p_index)
{
	if (!code.code_completion_active) {
		return;
	}
	code.code_completion_current_selected = p_index;
}

void TextEdit::confirm_code_completion(bool) {}

void TextEdit::cancel_code_completion() { code.code_completion_active = false; }

void TextEdit::set_line_length_guidelines(Vector<int> p_guideline_columns)
{
	code.line_length_guideline_columns = p_guideline_columns;
	queue_redraw();
}

Vector<int> TextEdit::get_line_length_guidelines() const
{
	return code.line_length_guideline_columns;
}

void TextEdit::set_symbol_lookup_on_click_enabled(bool p_enabled)
{
	code.symbol_lookup_on_click_enabled = p_enabled;
	set_symbol_lookup_word_as_valid(false);
}

bool TextEdit::is_symbol_lookup_on_click_enabled() const
{
	return code.symbol_lookup_on_click_enabled;
}

String TextEdit::get_text_for_symbol_lookup() const
{
	Point2i mp = get_local_mouse_pos();
	Point2i pos = get_line_column_at_pos(mp, false, false);
	int line = pos.y;
	int col = pos.x;

	if (line == -1) {
		return String();
	}

	return get_text_with_cursor_char(line, col);
}

String TextEdit::get_text_with_cursor_char(int p_line, int p_column) const
{
	const int text_size = get_line_count();
	StringBuilder result;
	for (int i = 0; i < text_size; i++) {
		String line_text = get_line(i);
		if (i == p_line && p_column >= 0 && p_column <= line_text.size()) {
			result += line_text.substr(0, p_column);
			/* Not unicode, represents the cursor. */
			result += String::chr(0xFFFF);
			result += line_text.substr(p_column);
		}
		else {
			result += line_text;
		}

		if (i != text_size - 1) {
			result += "\n";
		}
	}

	return result.as_string();
}

String TextEdit::get_lookup_word(int p_line, int p_column) const
{
	if (p_line < 0 || p_column < 0) {
		return String();
	}
	if (is_in_string(p_line, p_column) != -1) {
		// Return the string in case it is a path.
		Point2 start_pos = get_delimiter_start_position(p_line, p_column);
		Point2 end_pos = get_delimiter_end_position(p_line, p_column);
		int start_line = start_pos.y;
		int start_column = start_pos.x;
		int end_line = end_pos.y;
		int end_column = end_pos.x;
		if (start_line == end_line && start_column >= 0 && end_column >= 0) {
			return get_line(start_line).substr(start_column, end_column - start_column - 1);
		}
	}
	return get_word(p_line, p_column);
}

void TextEdit::set_symbol_lookup_word_as_valid(bool p_valid)
{
	code.symbol_lookup_word = p_valid ? code.symbol_lookup_new_word : "";
	code.symbol_lookup_new_word = "";
	if (lookup_symbol_word != code.symbol_lookup_word) {
		_set_symbol_lookup_word(code.symbol_lookup_word);
	}
}

void TextEdit::set_symbol_tooltip_on_hover_enabled(bool p_enabled)
{
	code.symbol_tooltip_on_hover_enabled = p_enabled;
	if (!p_enabled && code.symbol_tooltip_timer) {
		code.symbol_tooltip_timer->stop();
	}
}

bool TextEdit::is_symbol_tooltip_on_hover_enabled() const
{
	return code.symbol_tooltip_on_hover_enabled;
}

int TextEdit::_get_auto_brace_pair_open_at_pos(int p_line, int p_col)
{
	const String& line = get_line(p_line);
	int caret_col = MIN(p_col, line.length());

	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		const String& open_key = code.auto_brace_completion_pairs[i].open_key;
		if (caret_col < open_key.length()) {
			continue;
		}

		bool is_match = true;
		for (int j = 0; j < open_key.length(); j++) {
			if (line[(caret_col - 1) - j] != open_key[(open_key.length() - 1) - j]) {
				is_match = false;
				break;
			}
		}

		if (is_match) {
			return i;
		}
	}
	return -1;
}

int TextEdit::_get_auto_brace_pair_close_at_pos(int p_line, int p_col)
{
	const String& line = get_line(p_line);

	for (int i = 0; i < code.auto_brace_completion_pairs.size(); i++) {
		if (p_col + code.auto_brace_completion_pairs[i].close_key.length() > line.length()) {
			continue;
		}

		bool is_match = true;
		for (int j = 0; j < code.auto_brace_completion_pairs[i].close_key.length(); j++) {
			if (line[p_col + j] != code.auto_brace_completion_pairs[i].close_key[j]) {
				is_match = false;
				break;
			}
		}

		if (is_match) {
			return i;
		}
	}
	return -1;
}

void TextEdit::_gutter_clicked(int p_line, int p_gutter)
{
	if (!code_mode) {
		return;
	}
	bool shift_pressed = Input::is_key_pressed(Key::SHIFT);

	if (p_gutter == code.main_gutter) {
		if (code.draw_breakpoints && !shift_pressed) {
			set_line_as_breakpoint(p_line, !is_line_breakpointed(p_line));
		}
		else if (code.draw_bookmarks && shift_pressed) {
			set_line_as_bookmarked(p_line, !is_line_bookmarked(p_line));
		}
		return;
	}

	if (p_gutter == code.line_number_gutter) {
		remove_secondary_carets();
		set_selection_mode(TextEdit::SelectionMode::SELECTION_MODE_LINE);
		if (p_line == get_line_count() - 1) {
			select(p_line, 0, p_line, INT_MAX);
		}
		else {
			select(p_line, 0, p_line + 1, 0);
		}
		return;
	}

	if (p_gutter == code.fold_gutter) {
		if (is_line_folded(p_line)) {
			unfold_line(p_line);
		}
		else if (can_fold_line(p_line)) {
			fold_line(p_line);
		}
		return;
	}
}

void TextEdit::_update_gutter_indexes()
{
	for (int i = 0; i < get_gutter_count(); i++) {
		if (get_gutter_name(i) == "main_gutter") {
			code.main_gutter = i;
			continue;
		}

		if (get_gutter_name(i) == "line_numbers") {
			code.line_number_gutter = i;
			continue;
		}

		if (get_gutter_name(i) == "fold_gutter") {
			code.fold_gutter = i;
			continue;
		}
	}
}

void TextEdit::_update_code_region_tags()
{
	code.code_region_start_string = "";
	code.code_region_end_string = "";

	if (code.code_region_start_tag.is_empty() || code.code_region_end_tag.is_empty()) {
		return;
	}

	// A shorter delimiter has higher priority.
	for (int i = code.delimiters.size() - 1; i >= 0; i--) {
		if (code.delimiters[i].type != DelimiterType::TYPE_COMMENT) {
			continue;
		}
		if (code.delimiters[i].end_key.is_empty() && code.delimiters[i].line_only == true) {
			code.code_region_start_string =
				code.delimiters[i].start_key + code.code_region_start_tag;
			code.code_region_end_string = code.delimiters[i].start_key + code.code_region_end_tag;
			return;
		}
	}
}

void TextEdit::_update_delimiter_cache(int p_from_line, int p_to_line)
{
	if (code.delimiters.is_empty()) {
		return;
	}

	int line_count = get_line_count();
	if (p_to_line == -1) {
		p_to_line = line_count;
	}

	int start_line = MIN(p_from_line, p_to_line);
	int end_line = MAX(p_from_line, p_to_line);

	/* Make sure delimiter_cache has all the lines. */
	if (start_line != end_line) {
		if (p_to_line < p_from_line) {
			for (int i = end_line; i > start_line; i--) {
				code.delimiter_cache.remove_at(i);
			}
		}
		else {
			for (int i = start_line; i < end_line; i++) {
				code.delimiter_cache.insert(i, RBMap<int, int>());
			}
		}
	}
	if (code.delimiter_cache.size() < line_count) {
		code.delimiter_cache.resize(line_count);
	}

	int in_region = -1;
	for (int i = start_line; i < MIN(end_line + 1, line_count); i++) {
		int current_end_region = (i < 0 || code.delimiter_cache[i].size() < 1)
									 ? -1
									 : code.delimiter_cache[i].back()->value();
		in_region = (i <= 0 || code.delimiter_cache[i - 1].size() < 1)
						? -1
						: code.delimiter_cache[i - 1].back()->value();

		const String& str = get_line(i);
		const int line_length = str.length();
		code.delimiter_cache.write[i].clear();

		if (str.length() == 0) {
			if (in_region != -1) {
				code.delimiter_cache.write[i][0] = in_region;
			}
			if (i == end_line && current_end_region != in_region) {
				end_line++;
				end_line = MIN(end_line, line_count);
			}
			continue;
		}

		int end_region = -1;
		for (int j = 0; j < line_length; j++) {
			int from = j;
			for (; from < line_length; from++) {
				if (str[from] == '\\') {
					from++;
					continue;
				}
				break;
			}

			/* check if we are in entering a region */
			bool same_line = false;
			if (in_region == -1) {
				for (int d = 0; d < code.delimiters.size(); d++) {
					/* check there is enough room */
					int chars_left = line_length - from;
					int start_key_length = code.delimiters[d].start_key.length();
					int end_key_length = code.delimiters[d].end_key.length();
					if (chars_left < start_key_length) {
						continue;
					}

					/* search the line */
					bool match = true;
					const char32_t* start_key = code.delimiters[d].start_key.get_data();
					for (int k = 0; k < start_key_length; k++) {
						if (start_key[k] != str[from + k]) {
							match = false;
							break;
						}
					}
					if (!match) {
						continue;
					}
					same_line = true;
					in_region = d;
					code.delimiter_cache.write[i][from + 1] = d;
					from += start_key_length;

					/* check if it's the whole line */
					if (end_key_length == 0 || code.delimiters[d].line_only ||
						from + end_key_length > line_length) {
						j = line_length;
						if (code.delimiters[d].line_only) {
							code.delimiter_cache.write[i][line_length + 1] = -1;
						}
						else {
							end_region = in_region;
						}
					}
					break;
				}

				if (j == line_length || in_region == -1) {
					continue;
				}
			}

			/* if we are in one find the end key */
			/* search the line */
			int region_end_index = -1;
			int end_key_length = code.delimiters[in_region].end_key.length();
			const char32_t* end_key = code.delimiters[in_region].end_key.get_data();
			for (; from < line_length; from++) {
				if (line_length - from < end_key_length) {
					break;
				}
				if (!is_symbol(str[from])) {
					continue;
				}
				if (str[from] == '\\') {
					from++;
					continue;
				}
				region_end_index = from;
				for (int k = 0; k < end_key_length; k++) {
					if (end_key[k] != str[from + k]) {
						region_end_index = -1;
						break;
					}
				}

				if (region_end_index != -1) {
					break;
				}
			}

			j = from + (end_key_length - 1);
			end_region = (region_end_index == -1) ? in_region : -1;
			if (!same_line || region_end_index != -1) {
				code.delimiter_cache.write[i][j + 1] = end_region;
			}
			in_region = -1;
		}

		if (i == end_line && current_end_region != end_region) {
			end_line++;
			end_line = MIN(end_line, line_count);
		}
	}
}

int TextEdit::_is_in_delimiter(int p_line, int p_column, DelimiterType p_type) const
{
	if (code.delimiters.is_empty() || p_line >= code.delimiter_cache.size()) {
		return -1;
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), 0);

	int region = (p_line <= 0 || code.delimiter_cache[p_line - 1].size() < 1)
					 ? -1
					 : code.delimiter_cache[p_line - 1].back()->value();
	bool in_region = region != -1 && code.delimiters[region].type == p_type;
	for (RBMap<int, int>::Element* E = code.delimiter_cache[p_line].front(); E; E = E->next()) {
		/* If column is specified, loop until the key is larger then the column. */
		if (p_column != -1) {
			if (E->key() > p_column) {
				break;
			}
			in_region = E->value() != -1 && code.delimiters[E->value()].type == p_type;
			region = in_region ? E->value() : -1;
			continue;
		}

		/* If no column, calculate if the entire line is a region       */
		/* excluding whitespace.                                       */
		const String line = get_line(p_line);
		if (!in_region) {
			if (E->value() == -1 || code.delimiters[E->value()].type != p_type) {
				break;
			}

			region = E->value();
			in_region = true;
			for (int i = E->key() - 2; i >= 0; i--) {
				if (!is_whitespace(line[i])) {
					return -1;
				}
			}
		}

		if (code.delimiters[region].line_only) {
			return region;
		}

		int end_col = E->key();
		if (E->value() != -1) {
			if (!E->next()) {
				return region;
			}
			end_col = E->next()->key();
		}

		for (int i = end_col; i < line.length(); i++) {
			if (!is_whitespace(line[i])) {
				return -1;
			}
		}
		return region;
	}
	return in_region ? region : -1;
}

void TextEdit::_add_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only, DelimiterType p_type)
{
	if (!(Engine::is_editor_hint() && p_start_key == "null")) {
		ERR_FAIL_COND_MSG(p_start_key.is_empty(), "delimiter start key cannot be empty");

		for (int i = 0; i < p_start_key.length(); i++) {
			ERR_FAIL_COND_MSG(!is_symbol(p_start_key[i]), "delimiter must start with a symbol");
		}
	}

	if (p_end_key.length() > 0) {
		for (int i = 0; i < p_end_key.length(); i++) {
			ERR_FAIL_COND_MSG(!is_symbol(p_end_key[i]), "delimiter must end with a symbol");
		}
	}

	int at = 0;
	for (int i = 0; i < code.delimiters.size(); i++) {
		ERR_FAIL_COND_MSG(code.delimiters[i].start_key == p_start_key,
			"delimiter with start key '" + p_start_key + "' already exists.");
		if (p_start_key.length() < code.delimiters[i].start_key.length()) {
			at++;
		}
		else {
			break;
		}
	}

	Delimiter delimiter;
	delimiter.type = p_type;
	delimiter.start_key = p_start_key;
	delimiter.end_key = p_end_key;
	delimiter.line_only = p_line_only || p_end_key.is_empty();
	code.delimiters.insert(at, delimiter);
	if (!code.setting_delimiters) {
		code.delimiter_cache.clear();
		_update_delimiter_cache();
	}
	if (p_type == DelimiterType::TYPE_COMMENT) {
		_update_code_region_tags();
	}
}

void TextEdit::_remove_delimiter(const String& p_start_key, DelimiterType p_type)
{
	for (int i = 0; i < code.delimiters.size(); i++) {
		if (code.delimiters[i].start_key != p_start_key) {
			continue;
		}

		if (code.delimiters[i].type != p_type) {
			break;
		}

		code.delimiters.remove_at(i);
		if (!code.setting_delimiters) {
			code.delimiter_cache.clear();
			_update_delimiter_cache();
		}
		if (p_type == DelimiterType::TYPE_COMMENT) {
			_update_code_region_tags();
		}

		break;
	}
}

bool TextEdit::_has_delimiter(const String& p_start_key, DelimiterType p_type) const
{
	for (int i = 0; i < code.delimiters.size(); i++) {
		if (code.delimiters[i].start_key == p_start_key) {
			return code.delimiters[i].type == p_type;
		}
	}
	return false;
}

void TextEdit::_set_delimiters(const Vector<String>& p_delimiters, DelimiterType p_type)
{
	code.setting_delimiters = true;
	_clear_delimiters(p_type);
	for (int i = 0; i < p_delimiters.size(); i++) {
		String key = p_delimiters[i];
		if (key.is_empty()) {
			continue;
		}
		const String start_key = key.get_slicec(' ', 0);
		const String end_key = key.get_slice_count(" ") > 1 ? key.get_slicec(' ', 1) : String();
		_add_delimiter(start_key, end_key, end_key.is_empty(), p_type);
	}
	code.setting_delimiters = false;
	_update_delimiter_cache();
}

Vector<String> TextEdit::_get_delimiters(DelimiterType p_type) const
{
	Vector<String> ret;
	for (int i = 0; i < code.delimiters.size(); i++) {
		if (code.delimiters[i].type != p_type) {
			continue;
		}
		ret.push_back(code.delimiters[i].start_key +
					  (code.delimiters[i].end_key.is_empty() ? "" : " " + code.delimiters[i].end_key));
	}
	return ret;
}

void TextEdit::_clear_delimiters(DelimiterType p_type)
{
	for (int i = code.delimiters.size() - 1; i >= 0; i--) {
		if (code.delimiters[i].type == p_type) {
			code.delimiters.remove_at(i);
		}
	}
	code.delimiter_cache.clear();
	if (!code.setting_delimiters) {
		_update_delimiter_cache();
	}
	if (p_type == DelimiterType::TYPE_COMMENT) {
		_update_code_region_tags();
	}
}

void TextEdit::_code_lines_edited_from(int p_from_line, int p_to_line)
{
	_update_delimiter_cache(p_from_line, p_to_line);

	if (p_from_line == p_to_line) {
		return;
	}

	code.lines_edited_changed += p_to_line - p_from_line;
	code.lines_edited_from = (code.lines_edited_from == -1)
								 ? MIN(p_from_line, p_to_line)
								 : MIN(code.lines_edited_from, MIN(p_from_line, p_to_line));
	code.lines_edited_to = (code.lines_edited_to == -1)
							   ? MAX(p_from_line, p_to_line)
							   : MAX(code.lines_edited_from, MAX(p_from_line, p_to_line));
}

void TextEdit::_code_text_set()
{
	code.lines_edited_from = 0;
	code.lines_edited_to = 9999;
	_code_text_changed();
}

void TextEdit::_code_line_col_changed()
{
	if (!code.code_completion_active) {
		return;
	}

	if (get_caret_line() != code.code_completion_caret_line ||
		get_caret_column() != code.code_completion_caret_column) {
		cancel_code_completion();
	}
}

void TextEdit::_code_text_changed() {}

// --- text_edit.compat.inc ---

#ifndef DISABLE_DEPRECATED


void TextEdit::_set_selection_mode_bind_compat_86978(
	SelectionMode p_mode, int p_line, int p_column, int p_caret)
{
	set_selection_mode(p_mode);
}

Point2i TextEdit::_get_line_column_at_pos_bind_compat_100913(
	const Point2i& p_pos, bool p_allow_out_of_bounds) const
{
	return get_line_column_at_pos(p_pos, p_allow_out_of_bounds, true);
}

void TextEdit::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
