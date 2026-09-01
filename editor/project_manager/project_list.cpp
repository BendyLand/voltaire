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

void ProjectListItemControl::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (icon_needs_reload) {
			// The project icon may not be loaded by the time the control is displayed,
			// so use a loading placeholder.
			project_icon->set_texture(get_editor_theme_icon(SNAME("ProjectIconLoading")));
		}

		project_title->begin_bulk_theme_override();
		project_title->add_theme_font_override(SceneStringName(font),
			get_theme_font(SNAME("title"), EditorStringName(EditorFonts)).ptr());
		project_title->add_theme_font_size_override(SceneStringName(font_size),
			get_theme_font_size(SNAME("title_size"), EditorStringName(EditorFonts)));
		project_title->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SceneStringName(font_color), SNAME("ProjectList")));
		project_title->end_bulk_theme_override();

		project_path->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SceneStringName(font_color), SNAME("ProjectList")));

		switch (version_match_type) {
		case VersionMatchType::PROJECT_USES_OLDER_MAJOR:
			project_different_version->set_texture(
				get_editor_theme_icon(SNAME("ProjectUpgradeMajor")));
			break;
		case VersionMatchType::PROJECT_USES_OLDER_MINOR:
			project_different_version->set_texture(get_editor_theme_icon(SNAME("ProjectUpgrade")));
			break;
		case VersionMatchType::PROJECT_USES_NEWER_MAJOR:
			project_different_version->set_texture(
				get_editor_theme_icon(SNAME("ProjectDowngradeMajor")));
			break;
		case VersionMatchType::PROJECT_USES_NEWER_MINOR:
			project_different_version->set_texture(
				get_editor_theme_icon(SNAME("ProjectDowngrade")));
			break;
		default:
			break;
		}

		project_unsupported_features->set_texture(get_editor_theme_icon(SNAME("NodeWarning")));

		favorite_focus_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		_update_favorite_button_focus_color();
		if (is_favorite) {
			favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Favorites")));
		}
		else {
			favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Unfavorite")));
		}

		if (project_is_missing) {
			explore_button->set_button_icon(get_editor_theme_icon(SNAME("FileBroken")));
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		}
		else {
			explore_button->set_button_icon(get_editor_theme_icon(SNAME("Load")));
#endif
		}
		if (touch_menu_button) {
			touch_menu_button->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		}
	} break;

	case NOTIFICATION_MOUSE_ENTER: {
		is_hovering = true;
		queue_redraw();
		queue_accessibility_update();
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		is_hovering = false;
		queue_redraw();
		queue_accessibility_update();
	} break;

	case NOTIFICATION_DRAW: {
		if (is_selected && is_hovering) {
			draw_style_box(get_theme_stylebox(SNAME("hover_pressed"), SNAME("ProjectList")).ptr(),
				Rect2(Point2(), get_size()));
		}
		else if (is_selected) {
			draw_style_box(get_theme_stylebox(SNAME("selected"), SNAME("ProjectList")).ptr(),
				Rect2(Point2(), get_size()));
		}
		else if (is_hovering) {
			draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("ProjectList")).ptr(),
				Rect2(Point2(), get_size()));
		}
		// Due to how this control works, we can't rely on the built-in way of checking for focus
		// visibility.
		if (has_focus() && !is_focus_hidden) {
			draw_style_box(get_theme_stylebox(SNAME("focus"), SNAME("ProjectList")).ptr(),
				Rect2(Point2(), get_size()));
		}

		draw_line(Point2(0, get_size().y + 1), Point2(get_size().x, get_size().y + 1),
			get_theme_color(SNAME("guide_color"), SNAME("ProjectList")));
	} break;

	case NOTIFICATION_READY: {
		set_project_title_autowrap();
	} break;
	}
}

void ProjectListItemControl::_update_favorite_button_focus_color()
{
	if (favorite_button->has_focus()) {
		favorite_button->set_self_modulate(favorite_focus_color);
	}
	else {
		favorite_button->set_self_modulate(Color(1.0, 1.0, 1.0, 1.0));
	}
}

void ProjectListItemControl::set_project_title(const String& p_title)
{
	project_title->set_text(p_title);
	project_title->set_accessibility_name(TTRC("Project Name"));
	queue_accessibility_update();
}

void ProjectListItemControl::set_project_path(const String& p_path)
{
	project_path->set_text(p_path);
	project_path->set_accessibility_name(TTRC("Project Path"));
	queue_accessibility_update();
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

void ProjectListItemControl::set_unsupported_features(PackedStringArray p_features)
{
	if (p_features.size() > 0) {
		String tooltip_text = "";
		bool unknown_version = false;
		for (int i = 0; i < p_features.size(); i++) {
			if (ProjectList::project_feature_looks_like_version(p_features[i])) {
				PackedStringArray project_version_split = p_features[i].split(".");
				int project_version_major = 0, project_version_minor = 0;
				if (project_version_split.size() >= 2) {
					project_version_major = project_version_split[0].to_int();
					project_version_minor = project_version_split[1].to_int();
				}

				version_match_type = VersionMatchType::PROJECT_USES_SAME;
				if (project_version_major > VLTR_VERSION_MAJOR) {
					version_match_type = VersionMatchType::PROJECT_USES_NEWER_MAJOR;
				}
				else if (project_version_major < VLTR_VERSION_MAJOR) {
					version_match_type = VersionMatchType::PROJECT_USES_OLDER_MAJOR;
				}
				else {
					// Project is same major version.
					// Is it the same minor version, or an upgrade or downgrade?
					if (project_version_minor > VLTR_VERSION_MINOR) {
						version_match_type = VersionMatchType::PROJECT_USES_NEWER_MINOR;
					}
					else if (project_version_minor < VLTR_VERSION_MINOR) {
						version_match_type = VersionMatchType::PROJECT_USES_OLDER_MINOR;
					}
				}

				if (version_match_type != VersionMatchType::PROJECT_USES_SAME) {
					String project_version_tooltip_text =
						TTR("This project was last edited in a different Godot version: ") +
						p_features[i] + "\n";
					if (version_match_type == VersionMatchType::PROJECT_USES_OLDER_MAJOR ||
						version_match_type == VersionMatchType::PROJECT_USES_OLDER_MINOR) {
						project_version_tooltip_text +=
							vformat(TTR("Opening it will upgrade it to Godot %s.%s."),
								VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR) +
							"\n";
					}
					else if (version_match_type == VersionMatchType::PROJECT_USES_NEWER_MAJOR ||
							   version_match_type == VersionMatchType::PROJECT_USES_NEWER_MINOR) {
						project_version_tooltip_text +=
							vformat(TTR("Opening it will downgrade it to Godot %s.%s."),
								VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR) +
							"\n";
						project_version_tooltip_text +=
							TTR("Downgrading projects is not recommended.") + "\n";
					}
					project_different_version->set_focus_mode(FOCUS_ACCESSIBILITY);
					project_different_version->set_tooltip_text(project_version_tooltip_text);
					project_different_version->show();
				}
				else {
					project_different_version->hide();
				}
			}
			else {
				if (p_features[i] == "3.x") {
					version_match_type = VersionMatchType::PROJECT_USES_OLDER_MAJOR;
					String project_version_tooltip_text =
						TTR("This project was last edited in a different Godot version: ") +
						p_features[i] + "\n";
					project_version_tooltip_text +=
						vformat(TTR("Opening it will upgrade it to Godot %s.%s."),
							VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR) +
						"\n";
					project_different_version->set_focus_mode(FOCUS_ACCESSIBILITY);
					project_different_version->set_tooltip_text(project_version_tooltip_text);
					project_different_version->show();
				}
				else if (p_features[i] == "u-ver") {
					unknown_version = true;
					project_different_version->hide();
				}
			}

			p_features.remove_at(i);
			i--;
		}

		// This is actually triggered when the project.godot file's config_version
		// is less than 4, so perhaps it'd be more accurate to say the engine configuration
		// file's version is not supported...? If the config/features array includes
		// a proper version number, it will be displayed alongside the "unknown version"
		// warning otherwise.
		if (unknown_version) {
			tooltip_text += TTR("This project uses an unknown version of Godot.") + "\n";
		}
		if (p_features.size() > 0) {
			String unsupported_features_str = String(", ").join(p_features);
			tooltip_text += TTR("This project uses features unsupported by the current build:") +
							"\n" + unsupported_features_str;
		}

		if (tooltip_text.is_empty()) {
			return;
		}
		project_version->set_tooltip_text(tooltip_text);
		project_unsupported_features->set_focus_mode(FOCUS_ACCESSIBILITY);
		project_unsupported_features->set_tooltip_text(tooltip_text);
		project_unsupported_features->show();
	}
	else {
		project_different_version->hide();
		project_unsupported_features->hide();
	}
}

bool ProjectListItemControl::should_load_project_icon() const { return icon_needs_reload; }

void ProjectListItemControl::set_selected(bool p_selected, bool p_hide_focus)
{
	is_selected = p_selected;
	is_focus_hidden = is_selected && p_hide_focus;
	queue_redraw();
	queue_accessibility_update();
}

void ProjectListItemControl::set_is_favorite(bool p_favorite)
{
	is_favorite = p_favorite;
	if (p_favorite) {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Favorites")));
		favorite_button->set_accessibility_name(TTRC("Remove from Favorites"));
	}
	else {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Unfavorite")));
		favorite_button->set_accessibility_name(TTRC("Add to Favorites"));
	}
}

void ProjectListItemControl::set_is_missing(bool p_missing)
{
	project_is_missing = p_missing;

	if (project_is_missing) {
		project_icon->set_modulate(Color(1, 1, 1, 0.5));

		explore_button->set_button_icon(get_editor_theme_icon(SNAME("FileBroken")));
		explore_button->set_tooltip_text(TTRC("Error: Project is missing on the filesystem."));
	}
	else {
#if defined(ANDROID_ENABLED) || defined(WEB_ENABLED)
		// Opening the system file manager is not supported on the Android and web editors.
		explore_button->hide();
#else  // !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		explore_button->set_button_icon(get_editor_theme_icon(SNAME("Load")));
		explore_button->set_tooltip_text(
			OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN));
#endif // defined(ANDROID_ENABLED) || defined(WEB_ENABLED)
	}
}

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

void ProjectListItemControl::resize_project_title()
{
	if (get_window() == nullptr) {
		return;
	}

	int window_size = get_window()->get_size().x;
	int difference = window_size - window_size_cache;
	window_size_cache = window_size;

	int& title_size_cache = get_list()->title_size_cache[project_title_index];
	title_size_cache += difference;

	if (title_size_cache > title_fullsize_cache + tag_size_cache) {
		project_title->set_custom_maximum_size(Vector2(-1, -1));
		project_title->set_custom_minimum_size(Vector2(0, 0));
		project_title->set_autowrap_mode(TextServer::AUTOWRAP_OFF);

		return;
	}
	ProjectTag tag = ProjectTag("dummy");
	int tag_maxsize = tag.get_custom_maximum_size().x;
	int title_maxsize = title_size_cache - tag_size_cache;
	int title_minsize = title_size_cache - tag_maxsize;

	int abs_minsize = (200 * EDSCALE);
	if (title_fullsize_cache > abs_minsize) {
		if (title_minsize < abs_minsize) {
			title_minsize = abs_minsize + tag_maxsize - tag_size_cache;
		}
		if (title_maxsize < title_minsize) {
			project_title->set_custom_maximum_size(Vector2(title_minsize, -1));
		}
		else {
			project_title->set_custom_maximum_size(Vector2(title_maxsize, -1));
		}
		project_title->set_custom_minimum_size(Vector2(title_minsize, 0));
		project_title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	}
}

void ProjectListItemControl::_bind_methods() {}

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

// Helpers.

bool ProjectList::project_feature_looks_like_version(const String& p_feature)
{
	return p_feature.contains_char('.') && p_feature.substr(0, 3).is_numeric();
}

// Notifications.

void ProjectList::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (is_ready()) {
			for (const Item& item : _projects) {
				_update_project_control_translatable_fields(item);
			}
			update_dock_menu();
		}
	} break;

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

// Projects scan.

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

// Initialization & loading.

void ProjectList::save_config() { _config.save(_config_path); }

// Load project data from p_property_key and return it in a ProjectList::Item.
// p_favorite is passed directly into the Item.

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

void ProjectList::sort_projects()
{
	SortArray<Item, ProjectListComparator> sorter;
	sorter.compare.order_option = _order_option;
	sorter.sort(_projects.ptrw(), _projects.size());

	String search_term;
	PackedStringArray tags;

	if (!_search_term.is_empty()) {
		PackedStringArray search_parts = _search_term.split(" ");
		if (search_parts.size() > 1 || search_parts[0].begins_with("tag:")) {
			PackedStringArray remaining;
			for (const String& part : search_parts) {
				if (part.begins_with("tag:")) {
					tags.push_back(part.get_slicec(':', 1));
				}
				else {
					remaining.append(part);
				}
			}
			search_term = String(" ").join(remaining); // Search term without tags.
		}
		else {
			search_term = _search_term;
		}
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item& item = _projects.write[i];

		bool item_visible = true;
		if (!_search_term.is_empty()) {
			String search_path;
			if (search_term.contains_char('/')) {
				// Search path will match the whole path
				search_path = item.path;
			}
			else {
				// Search path will only match the last path component to make searching more strict
				search_path = item.path.get_file();
			}

			bool missing_tags = false;
			for (const String& tag : tags) {
				if (!item.tags.has(tag)) {
					missing_tags = true;
					break;
				}
			}

			// When searching, display projects whose name or path contain the search term and whose
			// tags match the searched tags.
			item_visible = !missing_tags &&
						   (search_term.is_empty() || item.project_name.containsn(search_term) ||
							   search_path.containsn(search_term));
		}

		item.control->set_visible(item_visible);
	}

	for (int i = 0; i < _projects.size(); ++i) {
		Item& item = _projects.write[i];
		item.control->get_parent()->move_child(item.control, i);
	}

	// Rewind the coroutine because order of projects changed
	_update_icons_async();
	update_dock_menu();
	queue_accessibility_update();
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

void ProjectList::_update_project_control_translatable_fields(const Item& item)
{
	ProjectListItemControl* control = item.control;

	control->set_project_title(!item.missing ? item.project_name : TTR("Missing Project"));
	control->set_last_edited_info(item.get_last_edited_string());
	control->set_unsupported_features(item.unsupported_features.duplicate());
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

// Project list selection.

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

// Resize project titles.

void ProjectList::resize_project_titles()
{
	for (Item& item : _projects) {
		item.control->resize_project_title();
	}
}

// Missing projects.

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

// Project list sorting and filtering.

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

// Object methods.

void ProjectList::_bind_methods() {}

ProjectList::ProjectList()
{
	set_follow_focus(true);

	project_list_vbox = memnew(VBoxContainer);
	project_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(project_list_vbox);

	_config_path = EditorPaths::get_singleton()->get_data_dir().path_join("projects.cfg");
	_migrate_config();
}


