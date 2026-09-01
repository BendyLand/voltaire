/**************************************************************************/
/*  editor_quick_open_dialog.cpp                                          */
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
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/string/fuzzy_search.h"
#include "core/templates/fixed_vector.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_quick_open_dialog.h"
#include "scene/gui/center_container.h"
#include "scene/gui/check_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

void HighlightedLabel::draw_substr_rects(
	const Vector2i& p_substr, Vector2 p_offset, int p_line_limit, int line_spacing)
{
	for (int i = get_lines_skipped(); i < p_line_limit; i++) {
		RID line = get_line_rid(i);
		Vector<Vector2> ranges =
			TS->shaped_text_get_selection(line, p_substr.x, p_substr.x + p_substr.y);
		Rect2 line_rect = get_line_rect(i);
		for (const Vector2& range : ranges) {
			Rect2 rect = Rect2(Point2(range.x, 0) + line_rect.position,
				Size2(range.y - range.x, line_rect.size.y));
			rect.position = p_offset + line_rect.position;
			rect.position.x += range.x;
			rect.size = Size2(range.y - range.x, line_rect.size.y);
			rect.size.x = MIN(rect.size.x, line_rect.size.x - range.x);
			if (rect.size.x > 0) {
				draw_rect(rect, Color(1, 1, 1, 0.07), true);
				draw_rect(rect, Color(0.5, 0.7, 1.0, 0.4), false, 1);
			}
		}
		p_offset.y +=
			line_spacing + TS->shaped_text_get_ascent(line) + TS->shaped_text_get_descent(line);
	}
}

void HighlightedLabel::add_highlight(const Vector2i& p_interval)
{
	if (p_interval.y > 0) {
		highlights.append(p_interval);
		queue_redraw();
	}
}

void HighlightedLabel::reset_highlights()
{
	highlights.clear();
	queue_redraw();
}

void HighlightedLabel::_notification(int p_notification)
{
	if (p_notification == NOTIFICATION_DRAW) {
		if (highlights.is_empty()) {
			return;
		}

		Vector2 offset;
		int line_limit;
		int line_spacing;
		get_layout_data(offset, line_limit, line_spacing);

		for (const Vector2i& substr : highlights) {
			draw_substr_rects(substr, offset, line_limit, line_spacing);
		}
	}
}

String EditorQuickOpenDialog::get_dialog_title(const Vector<StringName>& p_base_types)
{
	if (p_base_types.size() > 1) {
		return TTR("Select Resource");
	}

	if (p_base_types[0] == SNAME("PackedScene")) {
		return TTR("Select Scene");
	}

	return vformat(TTR("Select %s"), p_base_types[0]);
}

void EditorQuickOpenDialog::_finish_dialog_setup(const Vector<StringName>& p_base_types)
{
	set_process_shortcut_input(allow_type_switching);
	get_ok_button()->set_disabled(container->has_nothing_selected());
	set_title(get_dialog_title(p_base_types));
	popup_centered_clamped(Size2(780, 650) * EDSCALE, 0.8f);
	search_box->grab_focus();
}

void EditorQuickOpenDialog::ok_pressed()
{
	container->save_selected_item();

	update_property();
	container->cleanup();
	search_box->clear();
	hide();
}

void EditorQuickOpenDialog::selection_changed()
{
	// This prevents the property from being changed the first time the Quick Open
	// window is opened.
	if (!initial_selection_performed) {
		initial_selection_performed = true;
		return;
	}

	if (_is_instant_preview_active()) {
		preview_property();
	}
}

void EditorQuickOpenDialog::item_pressed(bool p_double_click)
{
	// A double-click should always be taken as a "confirm" action.
	if (p_double_click) {
		ok_pressed();
		return;
	}

	// Single-clicks should be taken as a "confirm" action only if Instant Preview
	// isn't currently enabled, or the property object is null for some reason.
	if (!_is_instant_preview_active()) {
		ok_pressed();
	}
}

void EditorQuickOpenDialog::_search_box_text_changed(const String& p_query)
{
	container->set_query_and_update(p_query);
	get_ok_button()->set_disabled(container->has_nothing_selected());
}

//------------------------- Result Container

void style_button(Button* p_button)
{
	p_button->set_flat(true);
	p_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
}

void QuickOpenResultContainer::_menu_option(int p_option)
{
	ERR_FAIL_COND(get_selected() == ResourceUID::INVALID_ID);
	String selected_path = get_selected_path();

	switch (p_option) {
	case FILE_SHOW_IN_FILESYSTEM: {
		FileSystemDock::get_singleton()->navigate_to_path(selected_path);
	} break;
	case FILE_SHOW_IN_FILE_MANAGER: {
		String dir = ProjectSettings::get_singleton()->globalize_path(selected_path);
		OS::get_singleton()->shell_show_in_file_manager(dir, true);
	} break;
	}
}

void QuickOpenResultContainer::_sort_uids(int p_max_results)
{
	struct FilepathComparator
	{
		bool operator()(const ResourceUID::ID& p_lhs, const ResourceUID::ID& p_rhs) const
		{
			String lhs_path = ResourceUID::get_singleton()->get_id_path(p_lhs);
			String rhs_path = ResourceUID::get_singleton()->get_id_path(p_rhs);

			// Sort on (length, alphanumeric) to prioritize shorter filepaths
			return lhs_path.length() == rhs_path.length() ? lhs_path < rhs_path
														  : lhs_path.length() < rhs_path.length();
		}
	};

	SortArray<ResourceUID::ID, FilepathComparator> sorter{};

	if ((int)uids.size() > p_max_results) {
		sorter.partial_sort(0, uids.size(), p_max_results, uids.ptr());
	}
	else {
		sorter.sort(uids.ptr(), uids.size());
	}
}

void QuickOpenResultContainer::_create_initial_results()
{
	file_type_icons.clear();
	file_type_icons.insert(SNAME("__default_icon"), get_editor_theme_icon(SNAME("Object")));
	uids.clear();
	filetypes.clear();
	history_set.clear();

	Vector<ResourceUID::ID>* history = _get_history();
	if (history) {
		for (const ResourceUID::ID& uid : *history) {
			history_set.insert(uid);
		}
	}

	_find_uids_in_folder(
		EditorFileSystem::get_singleton()->get_filesystem(), include_addons_toggle->is_pressed());
	_sort_uids(result_items.size());
	max_total_results = MIN(uids.size(), result_items.size());
	update_results();
}

void QuickOpenResultContainer::_find_uids_in_folder(
	EditorFileSystemDirectory* p_directory, bool p_include_addons)
{
	for (int i = 0; i < p_directory->get_subdir_count(); i++) {
		if (p_include_addons || p_directory->get_name() != "addons") {
			_find_uids_in_folder(p_directory->get_subdir(i), p_include_addons);
		}
	}

	for (int i = 0; i < p_directory->get_file_count(); i++) {
		ResourceUID::ID uid = p_directory->get_file_uid(i);
		if (uid == ResourceUID::INVALID_ID) {
			continue;
		}

		const StringName engine_type = p_directory->get_file_type(i);
		const StringName script_type = p_directory->get_file_resource_script_class(i);

		const bool is_engine_type = script_type == StringName();
		const StringName& actual_type = is_engine_type ? engine_type : script_type;

		for (const StringName& parent_type : base_types) {
			bool is_valid = !is_engine_type && EditorNode::get_editor_data().script_class_is_parent(
												   script_type, parent_type);
			if (is_valid) {
				uids.push_back(uid);
				filetypes.insert(uid, actual_type);
				break; // Stop testing base types as soon as we get a match.
			}
		}
	}
}

void QuickOpenResultContainer::set_query_and_update(const String& p_query)
{
	query = p_query;
	update_results();
}

Vector<ResourceUID::ID>* QuickOpenResultContainer::_get_history()
{
	if (base_types.size() == 1) {
		return selected_history.getptr(base_types[0]);
	}
	return nullptr;
}

QuickOpenResultCandidate QuickOpenResultCandidate::from_uid(
	const ResourceUID::ID& p_uid, bool& r_success)
{
	if (p_uid == ResourceUID::INVALID_ID || !ResourceUID::get_singleton()->has_id(p_uid)) {
		r_success = false;
		return QuickOpenResultCandidate();
	}

	QuickOpenResultCandidate candidate;
	candidate.uid = p_uid;
	candidate.result = nullptr;
	r_success = true;
	return candidate;
}

QuickOpenResultCandidate QuickOpenResultCandidate::from_result(
	Ref<FuzzySearchMatch> p_result, bool& r_success)
{
	ResourceUID::ID uid = EditorFileSystem::get_singleton()->get_file_uid(p_result->get_target());

	QuickOpenResultCandidate candidate = from_uid(uid, r_success);
	if (!r_success) {
		return QuickOpenResultCandidate();
	}

	candidate.result = p_result;
	return candidate;
}

void QuickOpenResultContainer::_add_candidate(QuickOpenResultCandidate& p_candidate)
{
	ERR_FAIL_COND(!ResourceUID::get_singleton()->has_id(p_candidate.uid));

	StringName actual_type;
	{
		StringName* actual_type_ptr = filetypes.getptr(p_candidate.uid);
		if (actual_type_ptr) {
			actual_type = *actual_type_ptr;
		}
		else {
			ERR_PRINT(vformat("EditorQuickOpenDialog: No type for path %s.",
				ResourceUID::get_singleton()->get_id_path(p_candidate.uid)));
		}
	}

	String file_path = ResourceUID::get_singleton()->get_id_path(p_candidate.uid);

	// Verify that a PackedScene is actually a "real" Scene if in a Open Scene context.
	if (base_types[0] == SNAME("PackedScene")) {
		static FixedVector<String, 3> valid_extensions = {"tscn", "scn", "res"};
		bool is_valid_type = false;
		for (const String& ext : valid_extensions) {
			if (file_path.has_extension(ext)) {
				is_valid_type = true;
				break;
			}
		}
		if (!is_valid_type) {
			return;
		}
	}

	EditorResourcePreview::PreviewItem item =
		EditorResourcePreview::get_singleton()->get_resource_preview_if_available(file_path);
	if (item.preview.is_valid()) {
		p_candidate.thumbnail = item.preview;
	}
	else if (file_type_icons.has(actual_type)) {
		p_candidate.thumbnail = *file_type_icons.getptr(actual_type);
	}
	else if (has_theme_icon(actual_type, EditorStringName(EditorIcons))) {
		p_candidate.thumbnail = get_editor_theme_icon(actual_type);
		file_type_icons.insert(actual_type, p_candidate.thumbnail);
	}
	else {
		p_candidate.thumbnail = *file_type_icons.getptr(SNAME("__default_icon"));
	}

	candidates.push_back(p_candidate);
	candidates_uids.insert(p_candidate.uid);
}

void QuickOpenResultContainer::update_results()
{
	candidates.clear();
	candidates_uids.clear();

	if (query.is_empty()) {
		_use_default_candidates();
	}
	else {
		_score_and_sort_candidates();
	}

	_update_result_items(MIN(candidates.size(), max_total_results), 0);
}

void QuickOpenResultContainer::_use_default_candidates()
{
	Vector<ResourceUID::ID>* history = _get_history();
	if (history) {
		for (const ResourceUID::ID& uid : *history) {
			bool success;
			QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_uid(uid, success);
			if (!success) {
				continue;
			}
			_add_candidate(candidate);
		}
	}

	for (const ResourceUID::ID& uid : uids) {
		if (candidates.size() >= max_total_results) {
			break;
		}
		if (candidates_uids.has(uid)) {
			continue;
		}

		bool success;
		QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_uid(uid, success);
		if (!success) {
			continue;
		}

		_add_candidate(candidate);
	}
}

void QuickOpenResultContainer::_score_and_sort_candidates()
{
	for (const Ref<FuzzySearchMatch>& result : _get_fuzzy_search_results()) {
		bool success;
		QuickOpenResultCandidate candidate = QuickOpenResultCandidate::from_result(result, success);
		if (!success) {
			continue;
		}

		_add_candidate(candidate);
	}
}

void QuickOpenResultContainer::_update_result_items(
	int p_new_visible_results_count, int p_new_selection_index)
{
	// Only need to update items that were not hidden in previous update.
	int num_items_needing_updates = MAX(num_visible_results, p_new_visible_results_count);
	num_visible_results = p_new_visible_results_count;

	for (int i = 0; i < num_items_needing_updates; i++) {
		QuickOpenResultItem* item = result_items[i];

		if (i < num_visible_results) {
			item->set_content(candidates[i]);
		}
		else {
			item->reset();
		}
	};

	const bool any_results = num_visible_results > 0;
	_select_item(any_results ? p_new_selection_index : -1);

	scroll_container->set_visible(any_results);
	no_results_container->set_visible(!any_results);

	if (!any_results) {
		if (uids.is_empty()) {
			no_results_label->set_text(TTR("No files found for this type"));
		}
		else {
			no_results_label->set_text(TTR("No results found"));
		}
	}
}

void QuickOpenResultContainer::_move_selection_index(Key p_key)
{
	// Don't move selection if there are no results.
	if (num_visible_results <= 0) {
		return;
	}
	const int max_index = num_visible_results - 1;

	int idx = selection_index;
	if (content_display_mode == QuickOpenDisplayMode::LIST) {
		if (p_key == Key::UP) {
			idx = (idx == 0) ? max_index : (idx - 1);
		}
		else if (p_key == Key::DOWN) {
			idx = (idx == max_index) ? 0 : (idx + 1);
		}
		else if (p_key == Key::PAGEUP) {
			idx = (idx == 0) ? idx : MAX(idx - 10, 0);
		}
		else if (p_key == Key::PAGEDOWN) {
			idx = (idx == max_index) ? idx : MIN(idx + 10, max_index);
		}
	}
	else {
		int column_count = grid->get_line_max_child_count();

		if (p_key == Key::LEFT) {
			idx = (idx == 0) ? max_index : (idx - 1);
		}
		else if (p_key == Key::RIGHT) {
			idx = (idx == max_index) ? 0 : (idx + 1);
		}
		else if (p_key == Key::UP) {
			idx = (idx == 0) ? max_index : MAX(idx - column_count, 0);
		}
		else if (p_key == Key::DOWN) {
			idx = (idx == max_index) ? 0 : MIN(idx + column_count, max_index);
		}
		else if (p_key == Key::PAGEUP) {
			idx = (idx == 0) ? idx : MAX(idx - (3 * column_count), 0);
		}
		else if (p_key == Key::PAGEDOWN) {
			idx = (idx == max_index) ? idx : MIN(idx + (3 * column_count), max_index);
		}
	}

	_select_item(idx);
}

String QuickOpenResultContainer::_get_cache_file_path() const
{
	return EditorPaths::get_singleton()->get_project_settings_dir().path_join(
		"quick_open_dialog_cache.cfg");
}

void QuickOpenResultContainer::_toggle_display_mode()
{
	QuickOpenDisplayMode new_display_mode = (content_display_mode == QuickOpenDisplayMode::LIST)
												? QuickOpenDisplayMode::GRID
												: QuickOpenDisplayMode::LIST;
	_set_display_mode(new_display_mode);
}

CanvasItem* QuickOpenResultContainer::_get_result_root()
{
	if (content_display_mode == QuickOpenDisplayMode::LIST) {
		return list;
	}
	else {
		return grid;
	}
}

void QuickOpenResultContainer::_layout_result_item(QuickOpenResultItem* item)
{
	item->set_display_mode(content_display_mode);
	Node* parent = item->get_parent();
	if (parent) {
		parent->remove_child(item);
	}
	_get_result_root()->add_child(item);
}

bool QuickOpenResultContainer::has_nothing_selected() const { return selection_index < 0; }

ResourceUID::ID QuickOpenResultContainer::get_selected() const
{
	ERR_FAIL_COND_V_MSG(has_nothing_selected(), ResourceUID::INVALID_ID,
		"Tried to get selected file, but nothing was selected.");
	return candidates[selection_index].uid;
}

String QuickOpenResultContainer::get_selected_path() const
{
	ERR_FAIL_COND_V_MSG(
		has_nothing_selected(), "", "Tried to get selected file path, but nothing was selected.");
	String path = ResourceUID::get_singleton()->get_id_path(candidates[selection_index].uid);
	ERR_FAIL_COND_V_MSG(path.is_empty(), "", "Failed to get selected file path.");
	return path;
}

const Vector<StringName>& QuickOpenResultContainer::get_base_types() const { return base_types; }

QuickOpenDisplayMode QuickOpenResultContainer::get_adaptive_display_mode(
	const Vector<StringName>& p_base_types)
{
	static const Vector<StringName> grid_preferred_types = {
		StringName("Font", true),
		StringName("Texture2D", true),
		StringName("Material", true),
		StringName("Mesh", true),
	};

	for (const StringName& type : grid_preferred_types) {
		for (const StringName& base_type : p_base_types) {
			if (base_type == type) {
				return QuickOpenDisplayMode::GRID;
			}
		}
	}

	return QuickOpenDisplayMode::LIST;
}

String _get_uid_string(const String& p_filepath)
{
	ResourceUID::ID id = EditorFileSystem::get_singleton()->get_file_uid(p_filepath);
	return id == ResourceUID::INVALID_ID ? p_filepath
										 : ResourceUID::get_singleton()->id_to_text(id);
}

bool QuickOpenResultContainer::is_instant_preview_enabled() const
{
	return instant_preview_toggle && instant_preview_toggle->is_visible() &&
		   instant_preview_toggle->is_pressed();
}

void QuickOpenResultContainer::set_instant_preview_toggle_visible(bool p_visible)
{
	instant_preview_toggle->set_visible(p_visible);
}

void QuickOpenResultContainer::cleanup()
{
	num_visible_results = 0;
	candidates.clear();
	history_set.clear();
	_select_item(-1);

	for (QuickOpenResultItem* item : result_items) {
		item->reset();
	}
}

void QuickOpenResultContainer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		Color text_color = get_theme_color("font_readonly_color", EditorStringName(Editor));
		file_details_path->add_theme_color_override(SceneStringName(font_color), text_color);
		no_results_label->add_theme_color_override(SceneStringName(font_color), text_color);

		file_context_menu->set_item_icon(
			FILE_SHOW_IN_FILESYSTEM, get_editor_theme_icon(SNAME("ShowInFileSystem")));
		file_context_menu->set_item_icon(
			FILE_SHOW_IN_FILE_MANAGER, get_editor_theme_icon(SNAME("Filesystem")));

		panel_container->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());

		if (content_display_mode == QuickOpenDisplayMode::LIST) {
			display_mode_toggle->set_button_icon(get_editor_theme_icon(SNAME("FileThumbnail")));
		}
		else {
			display_mode_toggle->set_button_icon(get_editor_theme_icon(SNAME("FileList")));
		}
	} break;
	}
}

void QuickOpenResultContainer::_bind_methods() {}

//------------------------- Result Item

QuickOpenResultItem::QuickOpenResultItem()
{
	set_focus_mode(FocusMode::FOCUS_NONE);
	_set_enabled(false);

	list_item = memnew(QuickOpenResultListItem);
	list_item->hide();
	add_child(list_item);

	grid_item = memnew(QuickOpenResultGridItem);
	grid_item->hide();
	add_child(grid_item);
}

void QuickOpenResultItem::set_display_mode(QuickOpenDisplayMode p_display_mode)
{
	if (p_display_mode == QuickOpenDisplayMode::LIST) {
		grid_item->hide();
		grid_item->reset();
		list_item->show();
	}
	else {
		list_item->hide();
		list_item->reset();
		grid_item->show();
	}

	queue_redraw();
}

void QuickOpenResultItem::set_content(const QuickOpenResultCandidate& p_candidate)
{
	_set_enabled(true);

	if (list_item->is_visible()) {
		list_item->set_content(p_candidate, enable_highlights);
	}
	else {
		grid_item->set_content(p_candidate, enable_highlights);
	}

	queue_redraw();
}

void QuickOpenResultItem::reset()
{
	_set_enabled(false);
	is_hovering = false;
	is_selected = false;
	list_item->reset();
	grid_item->reset();
}

void QuickOpenResultItem::highlight_item(bool p_enabled)
{
	is_selected = p_enabled;

	if (list_item->is_visible()) {
		if (p_enabled) {
			list_item->highlight_item(highlighted_font_color);
		}
		else {
			list_item->remove_highlight();
		}
	}
	else {
		if (p_enabled) {
			grid_item->highlight_item(highlighted_font_color);
		}
		else {
			grid_item->remove_highlight();
		}
	}

	queue_redraw();
}

void QuickOpenResultItem::_set_enabled(bool p_enabled)
{
	set_visible(p_enabled);
	set_process(p_enabled);
	set_process_input(p_enabled);
}

void QuickOpenResultItem::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_MOUSE_ENTER:
	case NOTIFICATION_MOUSE_EXIT: {
		is_hovering = is_visible() && p_what == NOTIFICATION_MOUSE_ENTER;
		queue_redraw();
	} break;
	case NOTIFICATION_THEME_CHANGED: {
		selected_stylebox = get_theme_stylebox("selected", "Tree");
		hovering_stylebox = get_theme_stylebox(SNAME("hovered"), "Tree");
		highlighted_font_color = get_theme_color("font_focus_color", EditorStringName(Editor));
	} break;
	case NOTIFICATION_DRAW: {
		if (is_selected) {
			draw_style_box(selected_stylebox.ptr(), Rect2(Point2(), get_size()));
		}
		else if (is_hovering) {
			draw_style_box(hovering_stylebox.ptr(), Rect2(Point2(), get_size()));
		}
	} break;
	}
}

//----------------- List item

static Vector2i _get_path_interval(const Vector2i& p_interval, int p_dir_index)
{
	if (p_interval.x >= p_dir_index || p_interval.y < 1) {
		return {-1, -1};
	}
	return {p_interval.x, MIN(p_interval.x + p_interval.y, p_dir_index) - p_interval.x};
}

static Vector2i _get_name_interval(const Vector2i& p_interval, int p_dir_index)
{
	if (p_interval.x + p_interval.y <= p_dir_index || p_interval.y < 1) {
		return {-1, -1};
	}
	int first_name_idx = p_dir_index + 1;
	int start = MAX(p_interval.x, first_name_idx);
	return {start - first_name_idx, p_interval.y - start + p_interval.x};
}

QuickOpenResultListItem::QuickOpenResultListItem()
{
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("margin_left", 6 * EDSCALE);
	add_theme_constant_override("margin_right", 6 * EDSCALE);

	hbc = memnew(HBoxContainer);
	hbc->add_theme_constant_override(SNAME("separation"), 4 * EDSCALE);
	add_child(hbc);

	const int max_size = 36 * EDSCALE;

	thumbnail = memnew(TextureRect);
	thumbnail->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	thumbnail->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	thumbnail->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	thumbnail->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	thumbnail->set_custom_minimum_size(Size2i(max_size, max_size));
	hbc->add_child(thumbnail);

	text_container = memnew(VBoxContainer);
	text_container->add_theme_constant_override(SNAME("separation"), -7 * EDSCALE);
	text_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	text_container->set_v_size_flags(Control::SIZE_FILL);
	hbc->add_child(text_container);

	name = memnew(HighlightedLabel);
	name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	name->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
	text_container->add_child(name);

	path = memnew(HighlightedLabel);
	path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	path->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	path->add_theme_font_size_override(SceneStringName(font_size), 12 * EDSCALE);
	text_container->add_child(path);
}

void QuickOpenResultListItem::set_content(
	const QuickOpenResultCandidate& p_candidate, bool p_highlight)
{
	thumbnail->set_texture(p_candidate.thumbnail);

	String file_path = ResourceUID::get_singleton()->get_id_path(p_candidate.uid);
	name->set_text(file_path.get_file());
	path->set_text(file_path.get_base_dir());
	name->reset_highlights();
	path->reset_highlights();

	if (p_highlight && p_candidate.result.is_valid()) {
		for (const FuzzyTokenMatch& match : p_candidate.result->get_token_matches()) {
			for (const Vector2i& interval : match.substrings) {
				path->add_highlight(
					_get_path_interval(interval, p_candidate.result->get_dir_index()));
				name->add_highlight(
					_get_name_interval(interval, p_candidate.result->get_dir_index()));
			}
		}
	}
}

void QuickOpenResultListItem::reset()
{
	thumbnail->set_texture(nullptr);
	name->set_text("");
	path->set_text("");
	name->reset_highlights();
	path->reset_highlights();
}

void QuickOpenResultListItem::highlight_item(const Color& p_color)
{
	name->add_theme_color_override(SceneStringName(font_color), p_color);
}

void QuickOpenResultListItem::remove_highlight()
{
	name->remove_theme_color_override(SceneStringName(font_color));
}

void QuickOpenResultListItem::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		path->add_theme_color_override(SceneStringName(font_color),
			get_theme_color("font_disabled_color", EditorStringName(Editor)));
	} break;
	}
}

//--------------- Grid Item

QuickOpenResultGridItem::QuickOpenResultGridItem()
{
	set_custom_minimum_size(Size2i(120 * EDSCALE, 0));
	add_theme_constant_override("margin_top", 6 * EDSCALE);
	add_theme_constant_override("margin_left", 2 * EDSCALE);
	add_theme_constant_override("margin_right", 2 * EDSCALE);

	vbc = memnew(VBoxContainer);
	vbc->set_h_size_flags(Control::SIZE_FILL);
	vbc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vbc->add_theme_constant_override(SNAME("separation"), 0);
	add_child(vbc);

	const int max_size = 64 * EDSCALE;

	thumbnail = memnew(TextureRect);
	thumbnail->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	thumbnail->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	thumbnail->set_custom_minimum_size(Size2i(max_size, max_size));
	vbc->add_child(thumbnail);

	name = memnew(HighlightedLabel);
	name->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	name->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	name->add_theme_font_size_override(SceneStringName(font_size), 13 * EDSCALE);
	vbc->add_child(name);
}

void QuickOpenResultGridItem::set_content(
	const QuickOpenResultCandidate& p_candidate, bool p_highlight)
{
	thumbnail->set_texture(p_candidate.thumbnail);

	String file_path = ResourceUID::get_singleton()->get_id_path(p_candidate.uid);
	name->set_text(file_path.get_file());
	name->set_tooltip_text(file_path);
	name->reset_highlights();

	if (p_highlight && p_candidate.result.is_valid()) {
		for (const FuzzyTokenMatch& match : p_candidate.result->get_token_matches()) {
			for (const Vector2i& interval : match.substrings) {
				name->add_highlight(
					_get_name_interval(interval, p_candidate.result->get_dir_index()));
			}
		}
	}

	bool uses_icon = p_candidate.thumbnail->get_width() < (32 * EDSCALE);

	if (uses_icon ||
		p_candidate.thumbnail->get_height() <= thumbnail->get_custom_minimum_size().y) {
		thumbnail->set_expand_mode(TextureRect::EXPAND_KEEP_SIZE);
		thumbnail->set_stretch_mode(TextureRect::StretchMode::STRETCH_KEEP_CENTERED);
	}
	else {
		thumbnail->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
		thumbnail->set_stretch_mode(TextureRect::StretchMode::STRETCH_SCALE);
	}
}

void QuickOpenResultGridItem::reset()
{
	thumbnail->set_texture(nullptr);
	name->set_text("");
	name->reset_highlights();
}

void QuickOpenResultGridItem::highlight_item(const Color& p_color)
{
	name->add_theme_color_override(SceneStringName(font_color), p_color);
}

void QuickOpenResultGridItem::remove_highlight()
{
	name->remove_theme_color_override(SceneStringName(font_color));
}


