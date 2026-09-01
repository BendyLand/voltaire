/**************************************************************************/
/*  find_in_files.cpp                                                     */
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
#include "core/os/os.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/shader/shader_editor_plugin.h"
#include "editor/shader/text_shader_editor.h"
#include "editor/themes/editor_scale.h"
#include "find_in_files.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

// TODO: Would be nice in Vector and Vectors.
template <typename T> inline void pop_back(T& r_container)
{
	r_container.resize(r_container.size() - 1);
}

static bool find_next(const String& p_line, const String& p_pattern, int p_from, bool p_match_case,
	bool p_whole_words, int& r_out_begin, int& r_out_end)
{
	int end = p_from;

	while (true) {
		int begin = p_match_case ? p_line.find(p_pattern, end) : p_line.findn(p_pattern, end);

		if (begin == -1) {
			return false;
		}

		end = begin + p_pattern.length();
		r_out_begin = begin;
		r_out_end = end;

		if (p_whole_words) {
			if (begin > 0 && is_ascii_identifier_char(p_line[begin - 1])) {
				continue;
			}
			if (end < p_line.size() && is_ascii_identifier_char(p_line[end])) {
				continue;
			}
		}

		return true;
	}
}

//--------------------------------------------------------------------------------

void FindInFilesSearch::set_search_text(const String& p_pattern) { pattern = p_pattern; }

void FindInFilesSearch::set_whole_words(bool p_whole_word) { whole_words = p_whole_word; }

void FindInFilesSearch::set_match_case(bool p_match_case) { match_case = p_match_case; }

void FindInFilesSearch::set_folder(const String& p_folder) { root_dir = p_folder; }

void FindInFilesSearch::set_filter(const HashSet<String>& p_exts) { extension_filter = p_exts; }

void FindInFilesSearch::set_includes(const HashSet<String>& p_include_wildcards)
{
	include_wildcards = p_include_wildcards;
}

void FindInFilesSearch::set_excludes(const HashSet<String>& p_exclude_wildcards)
{
	exclude_wildcards = p_exclude_wildcards;
}

void FindInFilesSearch::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_PROCESS: {
		_process();
	} break;
	}
}

void FindInFilesSearch::stop()
{
	searching = false;
	current_dir = "";
	set_process(false);
}

void FindInFilesSearch::_process()
{
	// This part can be moved to a thread if needed.

	OS& os = *OS::get_singleton();
	uint64_t time_before = os.get_ticks_msec();
	while (is_processing()) {
		_iterate();
		uint64_t elapsed = (os.get_ticks_msec() - time_before);
		if (elapsed > 8) { // Process again after waiting 8 ticks.
			break;
		}
	}
}

void FindInFilesSearch::_iterate()
{
	if (folders_stack.size() != 0) {
		// Scan folders first so we can build a list of files and have progress info later.

		PackedStringArray& folders_to_scan = folders_stack.write[folders_stack.size() - 1];

		if (folders_to_scan.size() != 0) {
			// Scan one folder below.

			String folder_name = folders_to_scan[folders_to_scan.size() - 1];
			pop_back(folders_to_scan);

			current_dir = current_dir.path_join(folder_name);

			PackedStringArray sub_dirs;
			PackedStringArray new_files_to_scan;
			_scan_dir("res://" + current_dir, sub_dirs, new_files_to_scan);

			folders_stack.push_back(sub_dirs);
			files_to_scan.append_array(new_files_to_scan);

		}
		else {
			// Go back one level.

			pop_back(folders_stack);
			current_dir = current_dir.get_base_dir();

			if (folders_stack.is_empty()) {
				// All folders scanned.
				initial_files_count = files_to_scan.size();
			}
		}

	}
	else if (files_to_scan.size() != 0) {
		// Then scan files.

		String fpath = files_to_scan[files_to_scan.size() - 1];
		pop_back(files_to_scan);
		_scan_file(fpath);

	}
	else {
		print_verbose("Search complete");
		set_process(false);
		current_dir = "";
		searching = false;
	}
}

float FindInFilesSearch::get_progress() const
{
	if (initial_files_count != 0) {
		return static_cast<float>(initial_files_count - files_to_scan.size()) /
			   static_cast<float>(initial_files_count);
	}
	return 0;
}

void FindInFilesSearch::_scan_dir(
	const String& p_path, PackedStringArray& r_out_folders, PackedStringArray& r_out_files_to_scan)
{
	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		print_verbose("Cannot open directory! " + p_path);
		return;
	}

	dir->list_dir_begin();

	// Limit to 100,000 iterations to avoid an infinite loop just in case
	// (this technically limits results to 100,000 files per folder).
	for (int i = 0; i < 100'000; ++i) {
		String file = dir->get_next();

		if (file.is_empty()) {
			break;
		}

		// If there is a .gdignore file in the directory, clear all the files/folders
		// to be searched on this path and skip searching the directory.
		if (file == ".gdignore") {
			r_out_folders.clear();
			r_out_files_to_scan.clear();
			break;
		}

		// Ignore special directories (such as those beginning with . and the project data
		// directory).
		String project_data_dir_name =
			ProjectSettings::get_singleton()->get_project_data_dir_name();
		if (file.begins_with(".") || file == project_data_dir_name) {
			continue;
		}
		if (dir->current_is_hidden()) {
			continue;
		}

		if (dir->current_is_dir()) {
			r_out_folders.push_back(file);

		}
		else {
			String file_ext = file.get_extension();
			if (extension_filter.has(file_ext)) {
				String file_path = p_path.path_join(file);
				bool case_sensitive = dir->is_case_sensitive(p_path);

				if (!exclude_wildcards.is_empty() &&
					_is_file_matched(exclude_wildcards, file_path, case_sensitive)) {
					continue;
				}

				if (include_wildcards.is_empty() ||
					_is_file_matched(include_wildcards, file_path, case_sensitive)) {
					r_out_files_to_scan.push_back(file_path);
				}
			}
		}
	}
}

bool FindInFilesSearch::_is_file_matched(
	const HashSet<String>& p_wildcards, const String& p_file_path, bool p_case_sensitive) const
{
	const String file_path = "/" + p_file_path.replace_char('\\', '/') + "/";

	for (const String& wildcard : p_wildcards) {
		if (p_case_sensitive && file_path.match(wildcard)) {
			return true;
		}
		else if (!p_case_sensitive && file_path.matchn(wildcard)) {
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

void FindInFilesDialog::set_replace_text(const String& p_text)
{
	replace_text_line_edit->set_text(p_text);
}

void FindInFilesDialog::set_replace_mode(bool p_replace)
{
	if (replace_mode == p_replace) {
		return;
	}

	replace_mode = p_replace;

	if (replace_mode) {
		set_title(TTRC("Replace in Files"));
		replace_label->show();
		replace_text_line_edit->show();
	}
	else {
		set_title(TTRC("Find in Files"));
		replace_label->hide();
		replace_text_line_edit->hide();
	}

	// Recalculate the dialog size after hiding child controls.
	set_size(Size2(get_size().x, 0));
}

String FindInFilesDialog::get_search_text() const { return search_text_line_edit->get_text(); }

String FindInFilesDialog::get_replace_text() const { return replace_text_line_edit->get_text(); }

bool FindInFilesDialog::is_match_case() const { return match_case_checkbox->is_pressed(); }

bool FindInFilesDialog::is_whole_words() const { return whole_words_checkbox->is_pressed(); }

String FindInFilesDialog::get_folder() const
{
	String p_text = folder_line_edit->get_text();
	return p_text.strip_edges();
}

HashSet<String> FindInFilesDialog::get_filter() const
{
	// Could check the filters_preferences but it might not have been generated yet.
	HashSet<String> filters;
	for (int i = 0; i < filters_container->get_child_count(); ++i) {
		CheckBox* cb = static_cast<CheckBox*>(filters_container->get_child(i));
		if (cb->is_pressed()) {
			filters.insert(cb->get_text());
		}
	}
	return filters;
}

HashSet<String> FindInFilesDialog::get_includes() const
{
	HashSet<String> includes;
	String p_text = includes_line_edit->get_text();

	if (p_text.is_empty()) {
		return includes;
	}

	PackedStringArray wildcards = p_text.split(",", false);
	for (const String& wildcard : wildcards) {
		includes.insert(_validate_filter_wildcard(wildcard));
	}
	return includes;
}

HashSet<String> FindInFilesDialog::get_excludes() const
{
	HashSet<String> excludes;
	String p_text = excludes_line_edit->get_text();

	if (p_text.is_empty()) {
		return excludes;
	}

	PackedStringArray wildcards = p_text.split(",", false);
	for (const String& wildcard : wildcards) {
		excludes.insert(_validate_filter_wildcard(wildcard));
	}
	return excludes;
}

void FindInFilesDialog::_on_folder_button_pressed() { folder_dialog->popup_file_dialog(); }

void FindInFilesDialog::_on_search_text_modified(const String& p_text)
{
	ERR_FAIL_NULL(find_button);
	ERR_FAIL_NULL(replace_button);

	find_button->set_disabled(get_search_text().is_empty());
	replace_button->set_disabled(get_search_text().is_empty());
}

void FindInFilesDialog::_on_search_text_submitted(const String& p_text)
{
	// This allows to trigger a global search without leaving the keyboard.
	if (!replace_mode && !find_button->is_disabled()) {
		custom_action("find");
	}

	if (replace_mode && !replace_button->is_disabled()) {
		custom_action("replace");
	}
}

void FindInFilesDialog::_on_replace_text_submitted(const String& p_text)
{
	// This allows to trigger a global search without leaving the keyboard.
	if (replace_mode && !replace_button->is_disabled()) {
		custom_action("replace");
	}
}

void FindInFilesDialog::_on_folder_selected(String p_path)
{
	int i = p_path.find("://");
	if (i != -1) {
		p_path = p_path.substr(i + 3);
	}
	folder_line_edit->set_text(p_path);
}

String FindInFilesDialog::_validate_filter_wildcard(const String& p_expression) const
{
	String ret = p_expression.replace_char('\\', '/');
	if (ret.begins_with("./")) {
		// Relative to the project root.
		ret = "res://" + ret.trim_prefix("./");
	}

	if (ret.begins_with(".")) {
		// To match extension.
		ret = "*" + ret;
	}

	if (!ret.begins_with("*")) {
		ret = "*/" + ret.trim_prefix("/");
	}

	if (!ret.ends_with("*")) {
		ret = ret.trim_suffix("/") + "/*";
	}

	return ret;
}

void FindInFilesPanel::set_with_replace(bool p_with_replace)
{
	with_replace = p_with_replace;
	replace_container->set_visible(p_with_replace);

	if (with_replace) {
		// Results show checkboxes on their left so they can be opted out.
		results_display->set_columns(2);
		results_display->set_column_expand(0, false);
		results_display->set_column_custom_minimum_width(0, 48 * EDSCALE);
	}
	else {
		// Results are single-cell items.
		results_display->set_column_expand(0, true);
		results_display->set_columns(1);
	}
}

void FindInFilesPanel::set_replace_text(const String& p_text)
{
	replace_line_edit->set_text(p_text);
}

bool FindInFilesPanel::is_keep_results() const { return keep_results_button->is_pressed(); }

void FindInFilesPanel::set_search_labels_visibility(bool p_visible)
{
	find_label->set_visible(p_visible);
	search_text_label->set_visible(p_visible);
	close_button->set_visible(p_visible);
}

void FindInFilesPanel::_clear()
{
	file_items.clear();
	file_items_results_count.clear();
	result_items.clear();
	results_display->clear();
	results_display->create_item(); // Root
}

void FindInFilesPanel::start_search()
{
	_clear();

	status_label->set_text(TTRC("Searching..."));
	search_text_label->set_text(finder->get_search_text());
	search_text_label->set_tooltip_text(finder->get_search_text());

	int label_min_width =
		search_text_label->get_minimum_size().x + search_text_label->get_character_bounds(0).size.x;
	search_text_label->set_custom_minimum_size(Size2(label_min_width, 0));

	set_process(true);
	progress_bar->set_visible(true);

	finder->start();

	_update_replace_buttons();
	refresh_button->hide();
	cancel_button->show();
}

void FindInFilesPanel::stop_search()
{
	finder->stop();

	status_label->set_text("");
	_update_replace_buttons();
	progress_bar->set_visible(false);
	refresh_button->show();
	cancel_button->hide();
}

void FindInFilesPanel::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (p_slot != EditorDock::DOCK_SLOT_BOTTOM) {
		results_display->set_theme_type_variation("NoBorderHorizontal");
		results_display->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTH);
	}
	else {
		results_display->set_theme_type_variation("NoBorderHorizontalBottom");
		results_display->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_TOP);
	}
}

void FindInFilesPanel::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		_on_theme_changed();
	} break;
	case NOTIFICATION_TRANSLATION_CHANGED: {
		_update_matches_text();

		TreeItem* file_item = results_display->get_root()->get_first_child();
		while (file_item) {
			if (with_replace) {
				file_item->set_button_tooltip_text(0,
					file_item->get_button_by_id(0, FIND_BUTTON_REPLACE),
					TTR("Replace all matches in file"));
			}
			file_item->set_button_tooltip_text(
				0, file_item->get_button_by_id(0, FIND_BUTTON_REMOVE), TTR("Remove result"));

			TreeItem* result_item = file_item->get_first_child();
			while (result_item) {
				if (with_replace) {
					result_item->set_button_tooltip_text(
						1, file_item->get_button_by_id(0, FIND_BUTTON_REPLACE), TTR("Replace"));
					result_item->set_button_tooltip_text(1,
						file_item->get_button_by_id(0, FIND_BUTTON_REMOVE), TTR("Remove result"));
				}
				else {
					result_item->set_button_tooltip_text(0,
						file_item->get_button_by_id(0, FIND_BUTTON_REMOVE), TTR("Remove result"));
				}
				result_item = result_item->get_next();
			}

			file_item = file_item->get_next();
		}
	} break;
	case NOTIFICATION_PROCESS: {
		progress_bar->set_as_ratio(finder->get_progress());
	} break;
	}
}

void FindInFilesPanel::_on_theme_changed()
{
	results_display->add_theme_font_override(SceneStringName(font),
		get_theme_font(SNAME("source"), EditorStringName(EditorFonts)).ptr());
	results_display->add_theme_font_size_override(SceneStringName(font_size),
		get_theme_font_size(SNAME("source_size"), EditorStringName(EditorFonts)));

	Color file_item_color =
		results_display->get_theme_color(SceneStringName(font_color)) * Color(1, 1, 1, 0.67);
	Ref<Texture2D> remove_texture = get_editor_theme_icon(SNAME("Close"));
	Ref<Texture2D> replace_texture = get_editor_theme_icon(SNAME("ReplaceText"));

	TreeItem* file_item = results_display->get_root()->get_first_child();
	while (file_item) {
		file_item->set_custom_color(0, file_item_color);
		if (with_replace) {
			file_item->set_button(
				0, file_item->get_button_by_id(0, FIND_BUTTON_REPLACE), replace_texture);
		}
		file_item->set_button(
			0, file_item->get_button_by_id(0, FIND_BUTTON_REMOVE), remove_texture);

		TreeItem* result_item = file_item->get_first_child();
		while (result_item) {
			if (with_replace) {
				result_item->set_button(
					1, result_item->get_button_by_id(1, FIND_BUTTON_REPLACE), replace_texture);
				result_item->set_button(
					1, result_item->get_button_by_id(1, FIND_BUTTON_REMOVE), remove_texture);
			}
			else {
				result_item->set_button(
					0, result_item->get_button_by_id(0, FIND_BUTTON_REMOVE), remove_texture);
			}

			result_item = result_item->get_next();
		}

		file_item = file_item->get_next();
	}
}

void FindInFilesPanel::_on_item_edited()
{
	TreeItem* item = results_display->get_selected();

	// Change opacity to half if checkbox is checked, otherwise full.
	Color use_color = results_display->get_theme_color(SceneStringName(font_color));
	if (!item->is_checked(0)) {
		use_color.a *= 0.5;
	}
	item->set_custom_color(1, use_color);
}

void FindInFilesPanel::_on_finished()
{
	_update_matches_text();
	_update_replace_buttons();
	progress_bar->set_visible(false);
	refresh_button->show();
	cancel_button->hide();
}

void FindInFilesPanel::_on_refresh_button_clicked() { start_search(); }

void FindInFilesPanel::_on_cancel_button_clicked() { stop_search(); }

void FindInFilesPanel::_on_replace_text_changed(const String& p_text) { _update_replace_buttons(); }

String FindInFilesPanel::_get_replace_text() { return replace_line_edit->get_text(); }

void FindInFilesPanel::_update_replace_buttons()
{
	bool disabled = finder->is_searching();

	replace_all_button->set_disabled(disabled);
}

//-----------------------------------------------------------------------------

void FindInFilesContainer::_on_theme_changed()
{
	const Ref<StyleBox> bottom_panel_style =
		EditorNode::get_singleton()->get_editor_theme()->get_stylebox(
			SNAME("BottomPanel"), EditorStringName(EditorStyles));
	if (bottom_panel_style.is_valid()) {
		begin_bulk_theme_override();
		add_theme_constant_override("margin_top", -bottom_panel_style->get_margin(SIDE_TOP));
		add_theme_constant_override("margin_left", -bottom_panel_style->get_margin(SIDE_LEFT));
		add_theme_constant_override("margin_right", -bottom_panel_style->get_margin(SIDE_RIGHT));
		add_theme_constant_override("margin_bottom", -bottom_panel_style->get_margin(SIDE_BOTTOM));
		end_bulk_theme_override();
	}
}

void FindInFilesContainer::_close_panel(FindInFilesPanel* p_panel)
{
	ERR_FAIL_COND_MSG(p_panel->get_parent() != tabs, "This panel is not a child!");
	tabs->remove_child(p_panel);
	p_panel->queue_free();
	_update_bar_visibility();
	if (tabs->get_tab_count() == 0) {
		close();
	}
}

void FindInFilesContainer::_on_dock_closed()
{
	while (tabs->get_tab_count() > 0) {
		Control* tab = tabs->get_tab_control(0);
		tabs->remove_child(tab);
		tab->queue_free();
	}
	_update_bar_visibility();
}

void FindInFilesContainer::_bar_input(const Ref<InputEvent>& p_input)
{
	int tab_id = tabs->get_tab_bar()->get_hovered_tab();
	Ref<InputEventMouseButton> mb = p_input;

	if (tab_id >= 0 && mb.is_valid() && mb->is_pressed() &&
		mb->get_button_index() == MouseButton::RIGHT) {
		tabs_context_menu->set_item_disabled(tabs_context_menu->get_item_index(PANEL_CLOSE_RIGHT),
			tab_id == tabs->get_tab_count() - 1);
		tabs_context_menu->set_position(
			tabs->get_tab_bar()->get_screen_position() + mb->get_position());
		tabs_context_menu->reset_size();
		tabs_context_menu->popup();
	}
}

void FindInFiles::_start_search(bool p_with_replace)
{
	FindInFilesPanel* panel = container->get_panel_for_results(
		(p_with_replace ? TTR("Replace:") : TTR("Find:")) + " " + dialog->get_search_text());
	FindInFilesSearch* search = panel->get_finder();

	search->set_search_text(dialog->get_search_text());
	search->set_match_case(dialog->is_match_case());
	search->set_whole_words(dialog->is_whole_words());
	search->set_folder(dialog->get_folder());
	search->set_filter(dialog->get_filter());
	search->set_includes(dialog->get_includes());
	search->set_excludes(dialog->get_excludes());

	panel->set_with_replace(p_with_replace);
	panel->set_replace_text(dialog->get_replace_text());
	panel->start_search();

	container->make_visible();
}

void FindInFiles::open_dialog(const String& p_initial_text, bool p_replace)
{
	dialog->set_replace_mode(p_replace);
	dialog->set_search_text(p_initial_text);
	if (p_replace) {
		dialog->set_replace_text(String());
	}
	dialog->popup_centered();
}


