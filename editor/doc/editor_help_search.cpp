/**************************************************************************/
/*  editor_help_search.cpp                                                */
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

#include "core/os/os.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_help_search.h"
#include "scene/gui/margin_container.h"
#include "servers/display/display_server.h"

bool EditorHelpSearch::_all_terms_in_name(const Vector<String>& p_terms, const String& p_name) const
{
	for (int i = 0; i < p_terms.size(); i++) {
		if (!p_name.containsn(p_terms[i])) {
			return false;
		}
	}
	return true;
}

void EditorHelpSearch::_update_results()
{
	const String term = search_box->get_text().strip_edges();

	int search_flags = filter_combo->get_selected_id();

	// Process separately if term is not short, or is "@" for annotations.
	if (term.length() > 1 || term == "@") {
		case_sensitive_button->set_disabled(false);
		hierarchy_button->set_disabled(false);

		if (case_sensitive_button->is_pressed()) {
			search_flags |= SEARCH_CASE_SENSITIVE;
		}
		if (hierarchy_button->is_pressed()) {
			search_flags |= SEARCH_SHOW_HIERARCHY;
		}

		search.instantiate(results_tree, results_tree, &tree_cache, term, search_flags);

		// Clear old search flags to force rebuild on short term.
		old_search_flags = 0;
		set_process(true);
	}
	else {
		// Disable hierarchy and case sensitive options, not used for short searches.
		case_sensitive_button->set_disabled(true);
		hierarchy_button->set_disabled(true);

		// Always show hierarchy for short searches.
		search.instantiate(
			results_tree, results_tree, &tree_cache, term, search_flags | SEARCH_SHOW_HIERARCHY);

		old_search_flags = search_flags;
		set_process(true);
	}
}

void EditorHelpSearch::_search_box_text_changed(const String& p_text) { _update_results(); }

void EditorHelpSearch::_filter_combo_item_selected(int p_option) { _update_results(); }

void EditorHelpSearch::_confirmed()
{
	TreeItem* item = results_tree->get_selected();
	if (!item) {
		return;
	}

	// Activate the script editor and emit the signal with the documentation link to display.
	EditorNode::get_singleton()->get_editor_main_screen()->select(EditorMainScreen::EDITOR_SCRIPT);

	hide();
}

void EditorHelpSearch::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			tree_cache.clear();
			results_tree->get_vscroll_bar()->set_value(0);
			search = Ref<Runner>();
			get_ok_button()->set_disabled(true);
		}
	} break;
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorThemeManager::is_generated_theme_outdated()) {
			break;
		}
		[[fallthrough]];
	}
	case NOTIFICATION_THEME_CHANGED: {
		const int icon_width =
			get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));
		results_tree->add_theme_constant_override("icon_max_width", icon_width);

		case_sensitive_button->set_button_icon(get_editor_theme_icon(SNAME("MatchCase")));
		hierarchy_button->set_button_icon(get_editor_theme_icon(SNAME("ClassList")));

		if (is_visible()) {
			_update_results();
		}
	} break;

	case NOTIFICATION_PROCESS: {
		// Update background search.
		if (search.is_valid()) {
			if (search->work()) {
				// Search done.

				// Only point to the match if it's a new search, and not just reopening a old one.
				if (!old_search) {
					results_tree->ensure_cursor_is_visible();
				}
				else {
					old_search = false;
				}

				get_ok_button()->set_disabled(!results_tree->get_selected());

				search = Ref<Runner>();
				set_process(false);
			}
		}
		else {
			set_process(false);
		}
	} break;
	}
}

void EditorHelpSearch::_bind_methods() {}

void EditorHelpSearch::popup_dialog() { popup_dialog(search_box->get_text()); }

void EditorHelpSearch::popup_dialog(const String& p_term)
{
	// Restore valid window bounds or pop up at default size.
	popup_centered_ratio(0.5F);

	old_search_flags = 0;
	if (p_term.is_empty()) {
		search_box->clear();
	}
	else {
		if (old_term == p_term) {
			old_search = true;
		}
		else {
			old_term = p_term;
		}

		search_box->set_text(p_term);
		search_box->select_all();
	}
	search_box->grab_focus();
	_update_results();
}

EditorHelpSearch::EditorHelpSearch()
{
	set_hide_on_ok(false);
	set_clamp_to_embedder(true);

	set_title(TTR("Search Help"));

	get_ok_button()->set_disabled(true);
	set_ok_button_text(TTR("Open"));

	// Split search and results area.
	VBoxContainer* vbox = memnew(VBoxContainer);
	add_child(vbox);

	// Create the search box and filter controls (at the top).
	HBoxContainer* hbox = memnew(HBoxContainer);
	vbox->add_child(hbox);

	search_box = memnew(FilterLineEdit);
	search_box->set_accessibility_name(TTRC("Search"));
	search_box->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	register_text_enter(search_box);
	hbox->add_child(search_box);

	case_sensitive_button = memnew(Button);
	case_sensitive_button->set_theme_type_variation(SceneStringName(FlatButton));
	case_sensitive_button->set_tooltip_text(TTR("Case Sensitive"));
	case_sensitive_button->set_toggle_mode(true);
	case_sensitive_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbox->add_child(case_sensitive_button);

	hierarchy_button = memnew(Button);
	hierarchy_button->set_theme_type_variation(SceneStringName(FlatButton));
	hierarchy_button->set_tooltip_text(TTR("Show Hierarchy"));
	hierarchy_button->set_toggle_mode(true);
	hierarchy_button->set_pressed(true);
	hierarchy_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	hbox->add_child(hierarchy_button);

	filter_combo = memnew(OptionButton);
	filter_combo->set_accessibility_name(TTRC("Filter"));
	filter_combo->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	filter_combo->set_stretch_ratio(0); // Fixed width.
	filter_combo->add_item(TTR("Display All"), SEARCH_ALL);
	filter_combo->add_separator();
	filter_combo->add_item(TTR("Classes Only"), SEARCH_CLASSES);
	filter_combo->add_item(TTR("Constructors Only"), SEARCH_CONSTRUCTORS);
	filter_combo->add_item(TTR("Methods Only"), SEARCH_METHODS);
	filter_combo->add_item(TTR("Operators Only"), SEARCH_OPERATORS);
	filter_combo->add_item(TTR("Signals Only"), SEARCH_SIGNALS);
	filter_combo->add_item(TTR("Annotations Only"), SEARCH_ANNOTATIONS);
	filter_combo->add_item(TTR("Constants Only"), SEARCH_CONSTANTS);
	filter_combo->add_item(TTR("Properties Only"), SEARCH_PROPERTIES);
	filter_combo->add_item(TTR("Theme Properties Only"), SEARCH_THEME_ITEMS);
	hbox->add_child(filter_combo);

	MarginContainer* mc = memnew(MarginContainer);
	mc->set_theme_type_variation("NoBorderHorizontalWindow");
	mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(mc);

	// Create the results tree.
	results_tree = memnew(Tree);
	search_box->set_forward_control(results_tree);
	results_tree->set_accessibility_name(TTRC("Search Results"));
	results_tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	results_tree->set_columns(2);
	results_tree->set_column_title(0, TTR("Name"));
	results_tree->set_column_clip_content(0, true);
	results_tree->set_column_title(1, TTR("Member Type"));
	results_tree->set_column_expand(1, false);
	results_tree->set_column_custom_minimum_width(1, 150 * EDSCALE);
	results_tree->set_column_clip_content(1, true);
	results_tree->set_custom_minimum_size(Size2(0, 100) * EDSCALE);
	results_tree->set_hide_root(true);
	results_tree->set_select_mode(Tree::SELECT_ROW);
	results_tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTH);
	mc->add_child(results_tree, true);
}

void EditorHelpSearch::TreeCache::clear()
{
	for (const KeyValue<String, TreeItem*>& E : item_cache) {
		memdelete(E.value);
	}
	item_cache.clear();
}

bool EditorHelpSearch::Runner::_is_class_disabled_by_feature_profile(const StringName& p_class)
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

bool EditorHelpSearch::Runner::_fill()
{
	bool phase_done = false;
	switch (phase) {
	case PHASE_MATCH_CLASSES_INIT:
		phase_done = _phase_fill_classes_init();
		break;
	case PHASE_MATCH_CLASSES:
		phase_done = _phase_fill_classes();
		break;
	case PHASE_CLASS_ITEMS_INIT:
	case PHASE_CLASS_ITEMS:
		phase_done = true;
		break;
	case PHASE_MEMBER_ITEMS_INIT:
		phase_done = _phase_fill_member_items_init();
		break;
	case PHASE_MEMBER_ITEMS:
		phase_done = _phase_fill_member_items();
		break;
	case PHASE_SELECT_MATCH:
		phase_done = _phase_select_match();
		break;
	case PHASE_MAX:
		return true;
	default:
		WARN_PRINT("Invalid or unhandled phase in EditorHelpSearch::Runner, aborting search.");
		return true;
	}

	if (phase_done) {
		phase++;
	}
	return false;
}

bool EditorHelpSearch::Runner::_phase_fill_classes_init()
{
	// Initialize fill.
	iterator_stack.clear();
	matched_item = nullptr;
	match_highest_score = 0;

	// Initialize stack of iterators to fill, in reverse.
	iterator_stack.push_back(EditorHelp::get_doc_data()->inheriting[""].back());

	return true;
}

bool EditorHelpSearch::Runner::_phase_fill_classes()
{
	if (iterator_stack.is_empty()) {
		return true;
	}

	if (iterator_stack[iterator_stack.size() - 1]) {
		// Decrement stack.
		iterator_stack[iterator_stack.size() - 1] =
			iterator_stack[iterator_stack.size() - 1]->prev();

		// Drop last element of stack if empty.
		if (!iterator_stack[iterator_stack.size() - 1]) {
			iterator_stack.resize(iterator_stack.size() - 1);
		}
		return false;
	}

	// Drop last element of stack if empty.
	if (!iterator_stack[iterator_stack.size() - 1]) {
		iterator_stack.resize(iterator_stack.size() - 1);
	}

	return iterator_stack.is_empty();
}

bool EditorHelpSearch::Runner::_phase_fill_member_items_init()
{
	// Prepare tree.
	class_items.clear();
	_populate_cache();

	return true;
}

TreeItem* EditorHelpSearch::Runner::_create_category_item(TreeItem* p_parent, const String& p_class,
	const StringName& p_icon, const String& p_text, const String& p_metatype)
{
	const String item_meta = "class_" + p_metatype + ":" + p_class;

	TreeItem* item = nullptr;
	if (_find_or_create_item(p_parent, item_meta, item)) {
		item->set_icon(0, ui_service->get_editor_theme_icon(p_icon));
		item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_ALWAYS);
		item->set_text(0, p_text);
	}
	item->set_collapsed(true);

	return item;
}

bool EditorHelpSearch::Runner::_slice()
{
	bool phase_done = false;
	switch (phase) {
	case PHASE_MATCH_CLASSES_INIT:
		phase_done = _phase_match_classes_init();
		break;
	case PHASE_MATCH_CLASSES:
		phase_done = _phase_match_classes();
		break;
	case PHASE_CLASS_ITEMS_INIT:
		phase_done = _phase_class_items_init();
		break;
	case PHASE_CLASS_ITEMS:
		phase_done = _phase_class_items();
		break;
	case PHASE_MEMBER_ITEMS_INIT:
		phase_done = _phase_member_items_init();
		break;
	case PHASE_MEMBER_ITEMS:
		phase_done = _phase_member_items();
		break;
	case PHASE_SELECT_MATCH:
		phase_done = _phase_select_match();
		break;
	case PHASE_MAX:
		return true;
	default:
		WARN_PRINT("Invalid or unhandled phase in EditorHelpSearch::Runner, aborting search.");
		return true;
	}

	if (phase_done) {
		phase++;
	}
	return false;
}

void EditorHelpSearch::Runner::_populate_cache()
{
	// Deselect to prevent re-selection issues.
	results_tree->deselect_all();

	root_item = results_tree->get_root();

	if (root_item) {
		LocalVector<TreeItem*> stack;

		// Add children of root item to stack.
		for (TreeItem* child = root_item->get_first_child(); child; child = child->get_next()) {
			stack.push_back(child);
		}

		// Traverse stack and cache items.
		while (!stack.is_empty()) {
			TreeItem* cur_item = stack[stack.size() - 1];
			stack.resize(stack.size() - 1);

			// Add any children to the stack.
			for (TreeItem* child = cur_item->get_first_child(); child; child = child->get_next()) {
				stack.push_back(child);
			}

			// Remove from parent.
			cur_item->get_parent()->remove_child(cur_item);
		}
	}
	else {
		root_item = results_tree->create_item();
	}
}

bool EditorHelpSearch::Runner::_phase_select_match()
{
	if (matched_item) {
		matched_item->select(0);
	}
	return true;
}

bool EditorHelpSearch::Runner::_all_terms_in_name(const String& p_name) const
{
	for (int i = 0; i < terms.size(); i++) {
		if (!_match_string(terms[i], p_name)) {
			return false;
		}
	}
	return true;
}

String EditorHelpSearch::Runner::_match_keywords_in_all_terms(const String& p_keywords) const
{
	String matching_keyword;
	for (int i = 0; i < terms.size(); i++) {
		matching_keyword = _match_keywords(terms[i], p_keywords);
		if (matching_keyword.is_empty()) {
			return String();
		}
	}
	return matching_keyword;
}

bool EditorHelpSearch::Runner::_match_string(const String& p_term, const String& p_string) const
{
	if (search_flags & SEARCH_CASE_SENSITIVE) {
		return p_string.contains(p_term);
	}
	else {
		return p_string.containsn(p_term);
	}
}

String EditorHelpSearch::Runner::_match_keywords(
	const String& p_term, const String& p_keywords) const
{
	for (const String& k : p_keywords.split(",")) {
		const String keyword = k.strip_edges();
		if (_match_string(p_term, keyword)) {
			return keyword;
		}
	}
	return String();
}

void EditorHelpSearch::Runner::_match_item(
	TreeItem* p_item, const String& p_text, bool p_is_keywords)
{
	if (p_text.is_empty()) {
		return;
	}

	float inverse_length = 1.0f / float(p_text.length());

	// Favor types where search term is a substring close to the start of the type.
	float w = 0.5f;
	int pos = p_text.findn(term);
	float score = (pos > -1) ? 1.0f - w * MIN(1, 3 * pos * inverse_length) : MAX(0.0f, 0.9f - w);

	// Favor shorter items: they resemble the search term more.
	w = 0.1f;
	score *= (1 - w) + w * (term.length() * inverse_length);

	// Reduce the score of keywords, since they are an indirect match.
	if (p_is_keywords) {
		score *= 0.9f;
	}

	// Replace current match if term is short as we are searching in reverse.
	if (match_highest_score == 0 || score > match_highest_score ||
		(score == match_highest_score && term.length() == 1)) {
		matched_item = p_item;
		match_highest_score = score;
	}
}

String EditorHelpSearch::Runner::_build_keywords_tooltip(const String& p_keywords) const
{
	String tooltip;
	if (p_keywords.is_empty()) {
		return tooltip;
	}

	tooltip = "\n\n" + TTR("Keywords") + ": ";

	for (const String& keyword : p_keywords.split(",")) {
		tooltip += keyword.strip_edges().quote() + ", ";
	}

	// Remove trailing comma and space.
	return tooltip.left(-2);
}

bool EditorHelpSearch::Runner::_find_or_create_item(
	TreeItem* p_parent, const String& p_item_meta, TreeItem*& r_item)
{
	// Attempt to find in cache.
	if (tree_cache->item_cache.has(p_item_meta)) {
		r_item = tree_cache->item_cache[p_item_meta];

		// Remove from cache.
		tree_cache->item_cache.erase(p_item_meta);

		// Add to tree.
		p_parent->add_child(r_item);

		return false;
	}
	else {
		// Otherwise create item.
		r_item = results_tree->create_item(p_parent);

		return true;
	}
}

TreeItem* EditorHelpSearch::Runner::_create_member_item(TreeItem* p_parent,
	const String& p_class_name, const StringName& p_icon, const String& p_name,
	const String& p_text, const String& p_type, const String& p_metatype, const String& p_tooltip,
	const String& p_keywords, bool p_is_deprecated, bool p_is_experimental,
	const String& p_matching_keyword)
{
	const String item_meta = "class_" + p_metatype + ":" + p_class_name + ":" + p_name;

	TreeItem* item = nullptr;
	if (_find_or_create_item(p_parent, item_meta, item)) {
		item->set_icon(0, ui_service->get_editor_theme_icon(p_icon));
		item->set_text(1, TTRGET(p_type));
		item->set_tooltip_text(0, p_tooltip);
		item->set_tooltip_text(1, p_tooltip);

		if (p_is_deprecated) {
			Ref<Texture2D> error_icon = ui_service->get_editor_theme_icon(SNAME("StatusError"));
			item->add_button(0, error_icon, 0, false, TTR("This member is marked as deprecated."));
		}
		else if (p_is_experimental) {
			Ref<Texture2D> warning_icon = ui_service->get_editor_theme_icon(SNAME("NodeWarning"));
			item->add_button(
				0, warning_icon, 0, false, TTR("This member is marked as experimental."));
		}
	}

	String text;
	if (search_flags & SEARCH_SHOW_HIERARCHY) {
		text = p_text;
	}
	else {
		text = p_class_name + "." + p_text;
	}
	if (!p_matching_keyword.is_empty()) {
		text += "      - " + vformat(TTR("Matches the \"%s\" keyword."), p_matching_keyword);
	}
	item->set_text(0, text);

	// Don't match member items for short searches.
	if (term.length() > 1 || term == "@") {
		_match_item(item, p_name);
	}
	for (const String& keyword : p_keywords.split(",")) {
		_match_item(item, keyword.strip_edges(), true);
	}

	return item;
}

bool EditorHelpSearch::Runner::work(uint64_t slot)
{
	// Return true when the search has been completed, otherwise false.
	const uint64_t until = OS::get_singleton()->get_ticks_usec() + slot;
	if (term.length() > 1 || term == "@") {
		while (!_slice()) {
			if (OS::get_singleton()->get_ticks_usec() > until) {
				return false;
			}
		}
	}
	else {
		while (!_fill()) {
			if (OS::get_singleton()->get_ticks_usec() > until) {
				return false;
			}
		}
	}
	return true;
}

EditorHelpSearch::Runner::Runner(Control* p_icon_service, Tree* p_results_tree,
	TreeCache* p_tree_cache, const String& p_term, int p_search_flags)
	: ui_service(p_icon_service), results_tree(p_results_tree), tree_cache(p_tree_cache),
	  term((p_search_flags & SEARCH_CASE_SENSITIVE) == 0 ? p_term.to_lower() : p_term),
	  search_flags(p_search_flags), disabled_color(ui_service->get_theme_color(
										SNAME("font_disabled_color"), EditorStringName(Editor)))
{
}


