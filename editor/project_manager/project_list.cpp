/**************************************************************************/
/*  project_list.cpp                                                      */
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
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/version.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/project_manager/project_manager.h"
#include "editor/project_manager/project_tag.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "project_list.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

void ProjectListItemControl::_update_favorite_button_focus_color()
{
	if (favorite_button->has_focus()) {
		favorite_button->set_self_modulate(favorite_focus_color);
	}
	else {
		favorite_button->set_self_modulate(Color(1.0, 1.0, 1.0, 1.0));
	}
}

void ProjectListItemControl::set_project_icon(const Ref<Texture2D>& p_icon)
{
	icon_needs_reload = false;

	// The default project icon is 128×128 to look crisp on hiDPI displays,
	// but we want the actual displayed size to be 64×64 on loDPI displays.
	project_icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	project_icon->set_custom_minimum_size(Size2(64, 64) * EDSCALE);
	project_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

	project_icon->set_texture(p_icon);
}

void ProjectListItemControl::set_last_edited_info(const String& p_info)
{
	last_edited_info->set_text(p_info);
}

void ProjectListItemControl::set_project_version(const String& p_info)
{
	project_version->set_text(p_info);
}

bool ProjectListItemControl::should_load_project_icon() const { return icon_needs_reload; }

void ProjectListItemControl::set_is_grayed(bool p_grayed)
{
	if (p_grayed) {
		main_vbox->set_modulate(Color(1, 1, 1, 0.5));
		// Don't make the icon less prominent if the parent is already grayed out.
		explore_button->set_modulate(Color(1, 1, 1, 1.0));
	}
	else {
		main_vbox->set_modulate(Color(1, 1, 1, 1.0));
		explore_button->set_modulate(Color(1, 1, 1, 0.5));
	}
}

void ProjectListItemControl::set_project_title_index(int p_title_index)
{
	project_title_index = p_title_index;
}

struct ProjectListComparator
{
	ProjectList::FilterOption order_option = ProjectList::FilterOption::EDIT_DATE;

	// operator<
	_FORCE_INLINE_ bool operator()(const ProjectList::Item& a, const ProjectList::Item& b) const
	{
		if (a.favorite && !b.favorite) {
			return true;
		}
		if (b.favorite && !a.favorite) {
			return false;
		}
		switch (order_option) {
		case ProjectList::PATH:
			return a.path < b.path;
		case ProjectList::EDIT_DATE:
			return a.last_edited > b.last_edited;
		case ProjectList::TAGS:
			return a.tag_sort_string < b.tag_sort_string;
		default:
			return a.project_name < b.project_name;
		}
	}
};

String ProjectList::Item::get_last_edited_string() const
{
	if (missing) {
		return TTR("Missing Date");
	}

	OS::TimeZoneInfo tz = OS::get_singleton()->get_time_zone_info();
	return Time::get_singleton()->get_datetime_string_from_unix_time(
		last_edited + tz.bias * 60, true);
}

bool ProjectList::project_feature_looks_like_version(const String& p_feature)
{
	return p_feature.contains_char('.') && p_feature.substr(0, 3).is_numeric();
}

void ProjectList::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (project_context_menu) {
			_update_menu_icons();
		}
	} break;

	case NOTIFICATION_PROCESS: {
		// Load icons as a coroutine to speed up launch when you have hundreds of projects.
		if (_icon_load_index < _projects.size()) {
			Item& item = _projects.write[_icon_load_index];
			if (item.control->should_load_project_icon()) {
				_load_project_icon(_icon_load_index);
			}
			_icon_load_index++;

			// Scan directories in thread to avoid blocking the window.
		}
		else if (scan_data && scan_data->scan_in_progress.is_set()) {
			// Wait for the thread.
		}
		else {
			set_process(false);
			if (scan_data) {
				_scan_finished();
			}
		}
	} break;

	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::get_singleton()->update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_BOX);
		AccessibilityServer::get_singleton()->update_set_list_item_count(ae, _projects.size());
		AccessibilityServer::get_singleton()->update_set_flag(
			ae, AccessibilityServerEnums::AccessibilityFlags::FLAG_MULTISELECTABLE, false);
	}
	}
}

void ProjectList::_scan_thread(void* p_scan_data)
{
	ScanData* scan_data = static_cast<ScanData*>(p_scan_data);

	for (const String& base_path : scan_data->paths_to_scan) {
		print_verbose(vformat("Scanning for projects in \"%s\".", base_path));
		_scan_folder_recursive(base_path, &scan_data->found_projects, scan_data->scan_in_progress);

		if (!scan_data->scan_in_progress.is_set()) {
			print_verbose("Scan aborted.");
			break;
		}
	}
	print_verbose(vformat("Found %d project(s).", scan_data->found_projects.size()));
	scan_data->scan_in_progress.clear();
}

void ProjectList::_scan_finished()
{
	if (scan_data->scan_in_progress.is_set()) {
		// Abort scanning.
		scan_data->scan_in_progress.clear();
	}

	scan_data->thread->wait_to_finish();
	memdelete(scan_data->thread);
	if (scan_progress) {
		scan_progress->hide();
	}

	for (const String& E : scan_data->found_projects) {
		add_project(E, false);
	}
	memdelete(scan_data);
	scan_data = nullptr;

	save_config();

	if (ProjectManager::get_singleton()->is_initialized()) {
		update_project_list();
	}
}

void ProjectList::save_config() { _config.save(_config_path); }

void ProjectList::_update_icons_async()
{
	_icon_load_index = 0;
	set_process(true);
}

void ProjectList::_load_project_icon(int p_index)
{
	Item& item = _projects.write[p_index];

	Ref<Texture2D> default_icon = get_editor_theme_icon(SNAME("DefaultProjectIcon"));
	Ref<Texture2D> icon;
	if (!item.icon.is_empty()) {
		Ref<Image> img;
		img.instantiate();
		Error err = img->load(item.icon.replace_first("res://", item.path + "/"));
		if (err == OK) {
			img->resize(
				default_icon->get_width(), default_icon->get_height(), Image::INTERPOLATE_LANCZOS);
			icon = ImageTexture::create_from_image(img);
		}
	}
	if (icon.is_null()) {
		icon = default_icon;
	}

	item.control->set_project_icon(icon);
}

int ProjectList::get_project_count() const { return _projects.size(); }

void ProjectList::find_projects(const String& p_path)
{
	PackedStringArray paths = {p_path};
	find_projects_multiple(paths);
}

void ProjectList::_scan_folder_recursive(
	const String& p_path, List<String>* r_projects, const SafeFlag& p_scan_active)
{
	if (!p_scan_active.is_set()) {
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error error = da->change_dir(p_path);
	ERR_FAIL_COND_MSG(error != OK,
		vformat("Failed to open the path \"%s\" for scanning (code %d).", p_path, error));

	da->list_dir_begin();
	String n = da->get_next();
	while (!n.is_empty()) {
		if (!p_scan_active.is_set()) {
			return;
		}

		if (da->current_is_dir() && n[0] != '.') {
			_scan_folder_recursive(da->get_current_dir().path_join(n), r_projects, p_scan_active);
		}
		else if (n == "project.godot") {
			r_projects->push_back(da->get_current_dir());
		}
		n = da->get_next();
	}
	da->list_dir_end();
}

void ProjectList::set_project_version(const String& p_project_path, int p_version)
{
	for (ProjectList::Item& E : _projects) {
		if (E.path == p_project_path) {
			E.version = p_version;
			break;
		}
	}
}

int ProjectList::get_index(const ProjectListItemControl* p_control) const
{
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].control == p_control) {
			return i;
		}
	}
	return -1;
}

void ProjectList::ensure_project_visible(int p_index)
{
	const Item& item = _projects[p_index];
	// Since follow focus is enabled.
	item.control->grab_focus(true);
}

void ProjectList::_toggle_project(int p_index)
{
	// This methods adds to the selection or removes from the
	// selection.
	Item& item = _projects.write[p_index];

	if (_selected_project_paths.has(item.path)) {
		_deselect_project_nocheck(p_index);
	}
	else {
		_select_project_nocheck(p_index);
	}
}

void ProjectList::_remove_project(int p_index, bool p_update_config)
{
	const Item item = _projects[p_index]; // Take a copy

	_selected_project_paths.erase(item.path);

	if (_last_clicked == item.path) {
		_last_clicked = "";
	}

	memdelete(item.control);
	_projects.remove_at(p_index);

	if (p_update_config) {
		_config.erase_section(item.path);
		// Not actually saving the file, in case you are doing more changes to settings
	}

	queue_accessibility_update();
	update_dock_menu();
}

void ProjectList::_on_explore_pressed(const String& p_path)
{
	OS::get_singleton()->shell_show_in_file_manager(p_path, true);
}

void ProjectList::_update_menu_icons()
{
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_EDIT), get_editor_theme_icon("Edit"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_VERBOSE),
		get_editor_theme_icon("Notification"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_RECOVERY),
		get_editor_theme_icon("NodeWarning"));
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_RUN), get_editor_theme_icon("Play"));
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_SHOW_IN_FILE_MANAGER),
		get_editor_theme_icon("Load"));
#endif
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_COPY_PATH), get_editor_theme_icon("ActionCopy"));
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_RENAME), get_editor_theme_icon("Rename"));
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_MANAGE_TAGS), get_editor_theme_icon("Script"));
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_DUPLICATE), get_editor_theme_icon("Duplicate"));
	project_context_menu->set_item_icon(
		project_context_menu->get_item_index(MENU_REMOVE), get_editor_theme_icon("Remove"));
}

void ProjectList::_clear_project_selection()
{
	Vector<Item> previous_selected_items = get_selected_projects();
	_selected_project_paths.clear();

	for (int i = 0; i < previous_selected_items.size(); ++i) {
		previous_selected_items[i].control->set_selected(false);
	}
	queue_accessibility_update();
}

void ProjectList::_select_project_nocheck(int p_index, bool p_hide_focus)
{
	Item& item = _projects.write[p_index];
	_selected_project_paths.insert(item.path);
	item.control->set_selected(true, p_hide_focus);
	queue_accessibility_update();
}

void ProjectList::_deselect_project_nocheck(int p_index)
{
	Item& item = _projects.write[p_index];
	_selected_project_paths.erase(item.path);
	item.control->set_selected(false);
	queue_accessibility_update();
}

inline void _sort_project_range(int& a, int& b)
{
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
}

void ProjectList::_select_project_range(int p_begin, int p_end)
{
	_clear_project_selection();

	_sort_project_range(p_begin, p_end);
	for (int i = p_begin; i <= p_end; ++i) {
		_select_project_nocheck(i);
	}
}

void ProjectList::select_project(int p_index, bool p_hide_focus)
{
	// This method keeps only one project selected.
	_clear_project_selection();
	_select_project_nocheck(p_index, p_hide_focus);
}

void ProjectList::deselect_project(int p_index) { _deselect_project_nocheck(p_index); }

void ProjectList::select_first_visible_project()
{
	_clear_project_selection();

	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_select_project_nocheck(i);
			break;
		}
	}
}

void ProjectList::deselect_all_visible_projects()
{
	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_deselect_project_nocheck(i);
		}
	}
}

void ProjectList::select_all_visible_projects()
{
	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].control->is_visible()) {
			_select_project_nocheck(i);
		}
	}
}

Vector<ProjectList::Item> ProjectList::get_selected_projects() const
{
	Vector<Item> items;
	if (_selected_project_paths.is_empty()) {
		return items;
	}
	items.resize(_selected_project_paths.size());
	int j = 0;
	for (int i = 0; i < _projects.size(); ++i) {
		const Item& item = _projects[i];
		if (_selected_project_paths.has(item.path)) {
			items.write[j++] = item;
		}
	}
	ERR_FAIL_COND_V(j != items.size(), items);
	return items;
}

const HashSet<String>& ProjectList::get_selected_project_keys() const
{
	// Faster if that's all you need
	return _selected_project_paths;
}

int ProjectList::get_single_selected_index() const
{
	if (_selected_project_paths.is_empty()) {
		// Default selection
		return 0;
	}
	String key;
	if (_selected_project_paths.size() == 1) {
		// Only one selected
		key = *_selected_project_paths.begin();
	}
	else {
		// Multiple selected, consider the last clicked one as "main"
		key = _last_clicked;
	}
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].path == key) {
			return i;
		}
	}
	return 0;
}

void ProjectList::erase_selected_projects(bool p_delete_project_contents)
{
	if (_selected_project_paths.is_empty()) {
		return;
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item& item = _projects.write[i];
		if (_selected_project_paths.has(item.path) && item.control->is_visible()) {
			_config.erase_section(item.path);

			// Comment out for now until we have a better warning system to
			// ensure users delete their project only.
			// if (p_delete_project_contents) {
			//	OS::get_singleton()->move_to_trash(item.path);
			//}

			memdelete(item.control);
			_projects.remove_at(i);
			--i;
		}
	}

	save_config();
	_selected_project_paths.clear();
	_last_clicked = "";

	update_dock_menu();
}

bool ProjectList::is_any_project_missing() const
{
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].missing) {
			return true;
		}
	}
	return false;
}

void ProjectList::erase_missing_projects()
{
	if (_projects.is_empty()) {
		return;
	}

	int deleted_count = 0;
	int remaining_count = 0;

	for (int i = 0; i < _projects.size(); ++i) {
		const Item& item = _projects[i];

		if (item.missing) {
			_remove_project(i, true);
			--i;
			++deleted_count;

		}
		else {
			++remaining_count;
		}
	}

	__print_line("Removed " + itos(deleted_count) + " projects from the list, remaining " +
				 itos(remaining_count) + " projects");
	save_config();
}

void ProjectList::set_search_term(String p_search_term) { _search_term = p_search_term; }

void ProjectList::add_search_tag(const String& p_tag)
{
	const String tag_string = "tag:" + p_tag;

	int exists = _search_term.find(tag_string);
	if (exists > -1) {
		_search_term = _search_term.erase(exists, tag_string.length() + 1);
	}
	else if (_search_term.is_empty() || _search_term.ends_with(" ")) {
		_search_term += tag_string;
	}
	else {
		_search_term += " " + tag_string;
	}
	ProjectManager::get_singleton()->get_search_box()->set_text(_search_term);

	sort_projects();
}

ProjectList::ProjectList()
{
	set_follow_focus(true);

	project_list_vbox = memnew(VBoxContainer);
	project_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(project_list_vbox);

	_config_path = EditorPaths::get_singleton()->get_data_dir().path_join("projects.cfg");
	_migrate_config();
}


