/**************************************************************************/
/*  create_dialog.cpp                                                     */
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

#include "core/io/resource_loader.h"
#include "create_dialog.h"
#include "editor/doc/editor_help.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/tree.h"

void CreateDialog::for_inherit() { allow_abstract_scripts = true; }

bool CreateDialog::_is_type_preferred(const String& p_type) const
{
	return EditorNode::get_editor_data().script_class_is_parent(
		p_type, preferred_search_result_type);
}

bool CreateDialog::_is_class_disabled_by_feature_profile(const StringName& p_class) const
{
	Ref<EditorFeatureProfile> profile =
		EditorFeatureProfileManager::get_singleton()->get_current_profile();

	return profile.is_valid() && profile->is_class_disabled(p_class);
}

void CreateDialog::_update_search()
{
	search_options->clear();
	search_options_types.clear();

	TreeItem* root = search_options->create_item();
	root->set_text(0, base_type);
	root->set_icon(0, search_options->get_editor_theme_icon(icon_fallback));
	search_options_types[base_type] = root;
	_configure_search_option_item(root, base_type, TypeCategory::OTHER_TYPE, "");

	const String search_text = search_box->get_text();
	bool type_filter_enabled = search_text.is_empty();

	selectable_types.clear();
	if (type_filter_enabled) {
		for (const TypeInfo& candidate : type_info_list) {
			bool is_editor = false;
			bool valid = false;
			// Native type.
			if (is_editor) {
				valid = types_enabled[TYPE_EDITOR];
			}
			else {
				valid = types_enabled[TYPE_BUILT_IN];
			}
		}
	}

	float highest_score = 0.0f;
	StringName best_match;

	for (const TypeInfo& candidate : type_info_list) {
		if (type_filter_enabled && !selectable_types.has(candidate.type_name)) {
			continue;
		}
		String match_keyword;

		// First check if the name matches. If it does not, try the search keywords.
		float score = _score_type(candidate.type_name, search_text);
		if (score < 0.0f) {
			for (const String& keyword : candidate.search_keywords) {
				score = _score_type(keyword, search_text);

				// Reduce the score of keywords, since they are an indirect match.
				score *= 0.1f;

				if (score >= 0.0f) {
					match_keyword = keyword;
					break;
				}
			}
		}

		// Search did not match.
		if (score < 0.0f) {
			continue;
		}

		_add_type(candidate.type_name, TypeCategory::OTHER_TYPE, match_keyword);

		if (score > highest_score) {
			highest_score = score;
			best_match = candidate.type_name;
		}
	}

	// Select the best result.
	if (search_text.is_empty()) {
		select_type(base_type);
	}
	else if (best_match != StringName()) {
		select_type(best_match);
	}
	else {
		favorite->set_disabled(true);
		help_bit->set_custom_text(String(), String(),
			vformat(TTR("No results for \"%s\"."), search_text.replace("[", "[lb]")));
		get_ok_button()->set_disabled(true);
		search_options->deselect_all();
	}
}

float CreateDialog::_score_type(const String& p_type, const String& p_search) const
{
	if (p_search.is_empty()) {
		return 0.0f;
	}

	// Determine the best match for a non-empty search.
	if (!p_search.is_subsequence_ofn(p_type)) {
		return -1.0f;
	}

	if (p_type == p_search) {
		// Always favor an exact match (case-sensitive), since clicking a favorite will set the
		// search text to the type.
		return 1.0f;
	}

	float inverse_length = 1.f / float(p_type.length());

	// Favor types where search term is a substring close to the start of the type.
	float w = 0.5f;
	int pos = p_type.findn(p_search);
	float score = (pos > -1) ? 1.0f - w * MIN(1, 3 * pos * inverse_length) : MAX(0.f, .9f - w);

	// Favor shorter items: they resemble the search term more.
	w = 0.9f;
	score *= (1 - w) + w * MIN(1.0f, p_search.length() * inverse_length);

	score *= _is_type_preferred(p_type) ? 1.0f : 0.9f;

	// Add score for being a favorite type.
	score *= favorite_list.has(p_type) ? 1.0f : 0.8f;

	// Look through at most 5 recent items
	bool in_recent = false;
	constexpr int RECENT_COMPLETION_SIZE = 5;
	for (int i = 0; i < MIN(RECENT_COMPLETION_SIZE - 1, recent->get_item_count()); i++) {
		if (recent->get_item_text(i) == p_type) {
			in_recent = true;
			break;
		}
	}
	score *= in_recent ? 1.0f : 0.9f;

	return score;
}

void CreateDialog::_cleanup()
{
	type_info_list.clear();
	favorite_list.clear();
	favorites->clear();
	recent->clear();
	custom_type_parents.clear();
	custom_type_indices.clear();
}

void CreateDialog::_reset_filters()
{
	if (!types_enabled[TYPE_BUILT_IN]) {
		_type_filter_toggled(TYPE_BUILT_IN, false);
	}
	if (!types_enabled[TYPE_CUSTOM]) {
		_type_filter_toggled(TYPE_CUSTOM, false);
	}
	if (types_enabled[TYPE_EDITOR]) {
		_type_filter_toggled(TYPE_EDITOR, false);
	}
	reset_filters_button->hide();
	_update_search();
}

void CreateDialog::_update_filter_button_state()
{
	const bool is_searching = !search_box->get_text().is_empty();
	filters_button->set_disabled(is_searching);
	if (!is_searching) {
		filters_button->get_popup()->set_item_checked(TYPE_BUILT_IN, types_enabled[TYPE_BUILT_IN]);
		filters_button->get_popup()->set_item_checked(TYPE_CUSTOM, types_enabled[TYPE_CUSTOM]);
		filters_button->get_popup()->set_item_checked(TYPE_EDITOR, types_enabled[TYPE_EDITOR]);
	}
	reset_filters_button->set_visible(
		!is_searching && (!types_enabled[TYPE_BUILT_IN] || !types_enabled[TYPE_CUSTOM] ||
							 types_enabled[TYPE_EDITOR]));
}

void CreateDialog::_text_changed(const String& p_newtext)
{
	_update_filter_button_state();
	_update_search();
}

void CreateDialog::_sbox_input(const Ref<InputEvent>& p_event)
{
	// Redirect navigational key events to the tree.
	Ref<InputEventKey> key = p_event;
	if (key.is_valid()) {
		if (key->is_action_pressed("ui_select", true)) {
			TreeItem* ti = search_options->get_selected();
			if (ti) {
				ti->set_collapsed(!ti->is_collapsed());
			}
			search_box->accept_event();
		}
	}
}

void CreateDialog::select_base()
{
	if (search_options_types.is_empty()) {
		_update_search();
	}
	select_type(base_type, false);
}

String CreateDialog::get_selected_type_name()
{
	TreeItem* selected = search_options->get_selected();
	if (!selected) {
		return String();
	}
	return selected->get_text(0).get_slicec(' ', 0);
}

void CreateDialog::_item_selected() { select_type(get_selected_type_name(), false); }

void CreateDialog::_hide_requested()
{
	_cancel_pressed(); // From AcceptDialog.
}

void CreateDialog::cancel_pressed() { _cleanup(); }

void CreateDialog::_favorite_toggled()
{
	TreeItem* item = search_options->get_selected();
	if (!item) {
		return;
	}

	String name = get_selected_type_name();

	if (favorite_list.has(name)) {
		favorite_list.erase(name);
		favorite->set_pressed(false);
	}
	else {
		favorite_list.push_back(name);
		favorite->set_pressed(true);
	}

	_save_and_update_favorite_list();
}

void CreateDialog::_history_selected(int p_idx)
{
	search_box->set_text(recent->get_item_text(p_idx));
	favorites->deselect_all();
	_update_search();
}

void CreateDialog::_favorite_selected()
{
	TreeItem* item = favorites->get_selected();
	if (!item) {
		return;
	}

	search_box->set_text(item->get_text(0));
	recent->deselect_all();
	_update_search();
}

void CreateDialog::_history_activated(int p_idx)
{
	_history_selected(p_idx);
	_confirmed();
}

void CreateDialog::_favorite_activated()
{
	_favorite_selected();
	_confirmed();
}

void CreateDialog::_load_favorites_and_history()
{
	String dir = EditorPaths::get_singleton()->get_project_settings_dir();
	Ref<FileAccess> f =
		FileAccess::open(dir.path_join("create_recent." + base_type), FileAccess::READ);
	if (f.is_valid()) {
		while (!f->eof_reached()) {
			String name = f->get_line().strip_edges();

			if (EditorNode::get_editor_data().is_type_recognized(name) &&
				!_is_class_disabled_by_feature_profile(name)) {
				recent->add_item(name, EditorNode::get_singleton()->get_class_icon(name));
			}
		}
	}

	f = FileAccess::open(dir.path_join("favorites." + base_type), FileAccess::READ);
	if (f.is_valid()) {
		while (!f->eof_reached()) {
			String name = f->get_line().strip_edges();

			if (!name.is_empty()) {
				favorite_list.push_back(name);
			}
		}
	}
}


