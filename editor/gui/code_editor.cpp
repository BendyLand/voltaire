/**************************************************************************/
/*  code_editor.cpp                                                       */
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

#include "code_editor.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/string/string_builder.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/main/timer.h"
#include "scene/resources/font.h"
#include "scene/resources/syntax_highlighter.h"

void GotoLinePopup::_goto_line()
{
	if (line_input->get_text().is_empty()) {
		return;
	}

	PackedStringArray line_col_strings = line_input->get_text().split(":");
	// Subtract 1 because the editor user interface starts from 1, but the TextEdit starts from 0.
	const int line_number = line_col_strings[0].to_int() - 1;
	if (line_number < 0 || line_number >= text_editor->get_text_editor()->get_line_count()) {
		return;
	}

	int column_number = 0;
	if (line_col_strings.size() >= 2) {
		column_number = line_col_strings[1].to_int() - 1;
	}
	text_editor->goto_line_centered(line_number, column_number);
}

void GotoLinePopup::_submit()
{
	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	_goto_line();
	ScriptEditorNavigationMarker::get_singleton()->locate_end();
	hide();
}

void GotoLinePopup::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			text_editor->set_preview_navigation_change(false);
			text_editor->get_text_editor()->grab_focus();
		}
	} break;
	}
}

// Implemented in input(..) as the LineEdit consumes the Escape pressed key.
void FindReplaceBar::input(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_action_pressed(SNAME("ui_cancel"), false, true)) {
		Control* focus_owner = get_viewport()->gui_get_focus_owner();

		if (text_editor->has_focus() || (focus_owner && is_ancestor_of(focus_owner))) {
			_hide_bar();
			accept_event();
		}
	}
}

void FindReplaceBar::_update_flags(bool p_direction_backwards)
{
	flags = 0;

	if (is_whole_words()) {
		flags |= TextEdit::SEARCH_WHOLE_WORDS;
	}
	if (is_case_sensitive()) {
		flags |= TextEdit::SEARCH_MATCH_CASE;
	}
	if (p_direction_backwards) {
		flags |= TextEdit::SEARCH_BACKWARDS;
	}
}

bool FindReplaceBar::_search(uint32_t p_flags, int p_from_line, int p_from_col)
{
	if (!preserve_cursor) {
		text_editor->remove_secondary_carets();
	}
	String text = get_search_text();
	Point2i pos = text_editor->search(text, p_flags, p_from_line, p_from_col);

	if (pos.x != -1) {
		if (!preserve_cursor && !is_selection_only()) {
			text_editor->unfold_line(pos.y);
			text_editor->select(pos.y, pos.x, pos.y, pos.x + text.length());
			text_editor->center_viewport_to_caret(0);
			text_editor->set_code_hint("");

			line_col_changed_for_result = true;
		}

		text_editor->set_search_text(text);
		text_editor->set_search_flags(p_flags);

		result_line = pos.y;
		result_col = pos.x;

		_update_results_count();
	}
	else {
		results_count = 0;
		result_line = -1;
		result_col = -1;
		text_editor->set_search_text("");
		text_editor->set_search_flags(p_flags);
	}

	_update_matches_display();

	return pos.x != -1;
}

void FindReplaceBar::_replace()
{
	text_editor->begin_complex_operation();
	text_editor->remove_secondary_carets();
	bool selection_enabled = text_editor->has_selection(0);
	Point2i selection_begin, selection_end;
	if (selection_enabled) {
		selection_begin = Point2i(
			text_editor->get_selection_from_line(0), text_editor->get_selection_from_column(0));
		selection_end =
			Point2i(text_editor->get_selection_to_line(0), text_editor->get_selection_to_column(0));
	}

	String repl_text = get_replace_text();
	int search_text_len = get_search_text().length();

	if (selection_enabled && is_selection_only()) {
		// Restrict search_current() to selected region.
		text_editor->set_caret_line(selection_begin.width, false, true, -1, 0);
		text_editor->set_caret_column(selection_begin.height, true, 0);
	}

	if (search_current()) {
		text_editor->unfold_line(result_line);
		text_editor->select(result_line, result_col, result_line, result_col + search_text_len, 0);

		if (selection_enabled && is_selection_only()) {
			Point2i match_from(result_line, result_col);
			Point2i match_to(result_line, result_col + search_text_len);
			if (!(match_from < selection_begin || match_to > selection_end)) {
				text_editor->insert_text_at_caret(repl_text, 0);
				if (match_to.x == selection_end.x) {
					// Adjust selection bounds if necessary.
					selection_end.y += repl_text.length() - search_text_len;
				}
			}
		}
		else {
			text_editor->insert_text_at_caret(repl_text, 0);
		}
	}
	text_editor->end_complex_operation();
	results_count = -1;
	results_count_to_current = -1;
	needs_to_count_results = true;

	if (selection_enabled && is_selection_only()) {
		// Reselect in order to keep 'Replace' restricted to selection.
		text_editor->select(
			selection_begin.x, selection_begin.y, selection_end.x, selection_end.y, 0);
	}
	else {
		text_editor->deselect(0);
	}
}

void FindReplaceBar::_get_search_from(int& r_line, int& r_col, SearchMode p_search_mode)
{
	if (!text_editor->has_selection(0) || is_selection_only()) {
		r_line = text_editor->get_caret_line(0);
		r_col = text_editor->get_caret_column(0);

		if (p_search_mode == SEARCH_PREV && r_line == result_line && r_col >= result_col &&
			r_col <= result_col + get_search_text().length()) {
			r_col = result_col;
		}
		return;
	}

	if (p_search_mode == SEARCH_NEXT) {
		r_line = text_editor->get_selection_to_line();
		r_col = text_editor->get_selection_to_column();
	}
	else {
		r_line = text_editor->get_selection_from_line();
		r_col = text_editor->get_selection_from_column();
	}
}

void FindReplaceBar::_update_results_count()
{
	int caret_line, caret_column;
	_get_search_from(caret_line, caret_column, SEARCH_CURRENT);
	bool match_selected = caret_line == result_line && caret_column == result_col &&
						  !is_selection_only() && text_editor->has_selection(0);

	if (match_selected && !needs_to_count_results && result_line != -1 &&
		results_count_to_current > 0) {
		results_count_to_current += (flags & TextEdit::SEARCH_BACKWARDS) ? -1 : 1;

		if (results_count_to_current > results_count) {
			results_count_to_current = results_count_to_current - results_count;
		}
		else if (results_count_to_current <= 0) {
			results_count_to_current = results_count;
		}

		return;
	}

	String searched = get_search_text();
	if (searched.is_empty()) {
		return;
	}

	needs_to_count_results = !match_selected;

	results_count = 0;
	results_count_to_current = 0;

	for (int i = 0; i < text_editor->get_line_count(); i++) {
		String line_text = text_editor->get_line(i);

		int col_pos = 0;

		bool searched_start_is_symbol = is_symbol(searched[0]);
		bool searched_end_is_symbol = is_symbol(searched[searched.length() - 1]);

		while (true) {
			col_pos = is_case_sensitive() ? line_text.find(searched, col_pos)
										  : line_text.findn(searched, col_pos);

			if (col_pos == -1) {
				break;
			}

			if (is_whole_words()) {
				if (!searched_start_is_symbol && col_pos > 0 &&
					!is_symbol(line_text[col_pos - 1])) {
					col_pos += searched.length();
					continue;
				}
				if (!searched_end_is_symbol && col_pos + searched.length() < line_text.length() &&
					!is_symbol(line_text[col_pos + searched.length()])) {
					col_pos += searched.length();
					continue;
				}
			}

			results_count++;

			if (i <= result_line && col_pos <= result_col) {
				results_count_to_current = results_count;
			}
			if (i == result_line && col_pos < result_col &&
				col_pos + searched.length() > result_col) {
				// Searching forwards and backwards with repeating text can lead to different
				// matches.
				col_pos = result_col;
			}
			col_pos += searched.length();
		}
	}
	if (!match_selected) {
		// Current result should refer to the match before the caret, if the caret is not on a
		// match.
		if (caret_line != result_line || caret_column != result_col) {
			results_count_to_current -= 1;
		}
		if (results_count_to_current == 0 &&
			(caret_line > result_line ||
				(caret_line == result_line && caret_column > result_col))) {
			// Caret is after all matches.
			results_count_to_current = results_count;
		}
	}
}

void FindReplaceBar::_update_matches_display()
{
	if (search_text->get_text().is_empty() || results_count == -1) {
		matches_label->hide();
	}
	else {
		matches_label->show();

		matches_label->add_theme_color_override(SceneStringName(font_color),
			results_count > 0 ? get_theme_color(SceneStringName(font_color), SNAME("Label"))
							  : get_theme_color(SNAME("error_color"), EditorStringName(Editor)));

		if (results_count == 0) {
			matches_label->set_text(TTR("No match"));
		}
		else if (results_count_to_current == -1) {
			matches_label->set_text(
				vformat(TTRN("%d match", "%d matches", results_count), results_count));
		}
		else {
			matches_label->set_text(
				vformat(TTRN("%d of %d match", "%d of %d matches", results_count),
					results_count_to_current, results_count));
		}
	}
	find_prev->set_disabled(results_count < 1);
	find_next->set_disabled(results_count < 1);
	replace->set_disabled(search_text->get_text().is_empty());
	replace_all->set_disabled(search_text->get_text().is_empty());
}

bool FindReplaceBar::search_current()
{
	_update_flags(false);

	int line, col;
	_get_search_from(line, col, SEARCH_CURRENT);

	return _search(flags, line, col);
}

bool FindReplaceBar::search_prev()
{
	if (is_selection_only() && !replace_all_mode) {
		return false;
	}

	if (!is_visible()) {
		popup_search(true);
	}

	String text = get_search_text();

	if ((flags & TextEdit::SEARCH_BACKWARDS) == 0) {
		needs_to_count_results = true;
	}

	_update_flags(true);

	int line, col;
	_get_search_from(line, col, SEARCH_PREV);

	col -= text.length();
	if (col < 0) {
		line -= 1;
		if (line < 0) {
			line = text_editor->get_line_count() - 1;
		}
		col = text_editor->get_line(line).length();
	}

	return _search(flags, line, col);
}

bool FindReplaceBar::search_next()
{
	if (is_selection_only() && !replace_all_mode) {
		return false;
	}

	if (!is_visible()) {
		popup_search(true);
	}

	if (flags & TextEdit::SEARCH_BACKWARDS) {
		needs_to_count_results = true;
	}

	_update_flags(false);

	int line, col;
	_get_search_from(line, col, SEARCH_NEXT);

	return _search(flags, line, col);
}

void FindReplaceBar::_hide_bar()
{
	text_editor->grab_focus();
	text_editor->set_search_text("");
	result_line = -1;
	result_col = -1;
	hide();
}

void FindReplaceBar::_update_toggle_replace_button(bool p_replace_visible)
{
	String tooltip = p_replace_visible ? TTRC("Hide Replace") : TTRC("Show Replace");
	String shortcut = ED_GET_SHORTCUT(
		p_replace_visible ? "script_text_editor/find" : "script_text_editor/replace")
						  ->get_as_text();
	toggle_replace_button->set_tooltip_text(vformat("%s (%s)", tooltip, shortcut));
	StringName rtl_compliant_arrow =
		is_layout_rtl() ? SNAME("GuiTreeArrowLeft") : SNAME("GuiTreeArrowRight");
	toggle_replace_button->set_button_icon(
		get_editor_theme_icon(p_replace_visible ? SNAME("GuiTreeArrowDown") : rtl_compliant_arrow));
}

void FindReplaceBar::popup_search(bool p_show_only)
{
	replace_text->hide();
	hbc_button_replace->hide();
	hbc_option_replace->hide();
	selection_only->set_pressed(false);
	_update_toggle_replace_button(false);

	_show_search(false, p_show_only);
}

void FindReplaceBar::popup_replace()
{
	if (!replace_text->is_visible_in_tree()) {
		replace_text->show();
		hbc_button_replace->show();
		hbc_option_replace->show();
		_update_toggle_replace_button(true);
	}

	selection_only->set_pressed(
		text_editor->has_selection(0) &&
		text_editor->get_selection_from_line(0) < text_editor->get_selection_to_line(0));

	_show_search(true, false);
}

void FindReplaceBar::_search_options_changed(bool p_pressed)
{
	results_count = -1;
	results_count_to_current = -1;
	needs_to_count_results = true;
	search_current();
}

void FindReplaceBar::_editor_text_changed()
{
	results_count = -1;
	results_count_to_current = -1;
	needs_to_count_results = true;
	if (is_visible_in_tree()) {
		preserve_cursor = true;
		search_current();
		preserve_cursor = false;
	}
}

void FindReplaceBar::_search_text_changed(const String& p_text)
{
	results_count = -1;
	results_count_to_current = -1;
	needs_to_count_results = true;
	search_current();
}

void FindReplaceBar::_search_text_submitted(const String& p_text)
{
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		search_prev();
	}
	else {
		search_next();
	}
}

void FindReplaceBar::_replace_text_submitted(const String& p_text)
{
	if (selection_only->is_pressed() && text_editor->has_selection(0)) {
		_replace_all();
		_hide_bar();
	}
	else if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		_replace();
		search_prev();
	}
	else {
		_replace();
		search_next();
	}
}

void FindReplaceBar::_replace_button_pressed()
{
	_replace();
	search_next();
}

void FindReplaceBar::_toggle_replace_pressed()
{
	bool replace_visible = replace_text->is_visible_in_tree();
	replace_visible ? popup_search(true) : popup_replace();
}

String FindReplaceBar::get_search_text() const { return search_text->get_text(); }

String FindReplaceBar::get_replace_text() const { return replace_text->get_text(); }

bool FindReplaceBar::is_case_sensitive() const { return case_sensitive->is_pressed(); }

bool FindReplaceBar::is_whole_words() const { return whole_words->is_pressed(); }

bool FindReplaceBar::is_selection_only() const { return selection_only->is_pressed(); }

/*** CODE EDITOR ****/

static constexpr float ZOOM_FACTOR_PRESETS[8] = {0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f};

// This function should be used to handle shortcuts that could otherwise
// be handled too late if they weren't handled here.
void CodeTextEditor::input(const Ref<InputEvent>& event)
{
	ERR_FAIL_COND(event.is_null());

	const Ref<InputEventKey> key_event = event;

	if (key_event.is_null()) {
		return;
	}
	if (!key_event->is_pressed()) {
		return;
	}

	if (!text_editor->has_focus()) {
		if ((find_replace_bar != nullptr && find_replace_bar->is_visible()) &&
			(find_replace_bar->has_focus() ||
				(get_viewport()->gui_get_focus_owner() &&
					find_replace_bar->is_ancestor_of(get_viewport()->gui_get_focus_owner())))) {
			if (ED_IS_SHORTCUT("script_text_editor/find_next", key_event)) {
				find_replace_bar->search_next();
				accept_event();
				return;
			}
			if (ED_IS_SHORTCUT("script_text_editor/find_previous", key_event)) {
				find_replace_bar->search_prev();
				accept_event();
				return;
			}
		}
		return;
	}

	if (ED_IS_SHORTCUT("script_text_editor/move_up", key_event)) {
		text_editor->move_lines_up();
		accept_event();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/move_down", key_event)) {
		text_editor->move_lines_down();
		accept_event();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/delete_line", key_event)) {
		text_editor->delete_lines();
		accept_event();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/join_lines", key_event)) {
		text_editor->join_lines();
		accept_event();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/duplicate_selection", key_event)) {
		text_editor->duplicate_selection();
		accept_event();
		return;
	}
	if (ED_IS_SHORTCUT("script_text_editor/duplicate_lines", key_event)) {
		text_editor->duplicate_lines();
		accept_event();
		return;
	}
}

void CodeTextEditor::_text_editor_gui_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid()) {
		if (mb->is_pressed() && mb->is_command_or_control_pressed()) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP) {
				_zoom_in();
				accept_event();
				return;
			}
			if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				_zoom_out();
				accept_event();
				return;
			}
		}
	}

#ifndef ANDROID_ENABLED
	Ref<InputEventMagnifyGesture> magnify_gesture = p_event;
	if (magnify_gesture.is_valid()) {
		_zoom_to(zoom_factor * std::pow(magnify_gesture->get_factor(), 0.25f));
		accept_event();
		return;
	}
#endif

	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		if (k->is_pressed()) {
			if (ED_IS_SHORTCUT("script_editor/zoom_in", p_event)) {
				_zoom_in();
				accept_event();
				return;
			}
			if (ED_IS_SHORTCUT("script_editor/zoom_out", p_event)) {
				_zoom_out();
				accept_event();
				return;
			}
			if (ED_IS_SHORTCUT("script_editor/reset_zoom", p_event)) {
				_zoom_to(1);
				accept_event();
				return;
			}
		}
	}
}

void CodeTextEditor::_line_col_changed()
{
	if (!code_complete_timer->is_stopped() &&
		code_complete_timer_line != text_editor->get_caret_line()) {
		code_complete_timer->stop();
	}

	Point2i display_position = get_pos_for_display(
		Point2i(text_editor->get_caret_line(), text_editor->get_caret_column()));
	StringBuilder sb;
	sb.append(itos(display_position.x).lpad(4));
	sb.append(" : ");
	sb.append(itos(display_position.y).lpad(3));

	line_and_col_button->set_text(sb.as_string());

	if (find_replace_bar) {
		if (!find_replace_bar->line_col_changed_for_result) {
			find_replace_bar->needs_to_count_results = true;
		}

		find_replace_bar->line_col_changed_for_result = false;
	}
}

void CodeTextEditor::_text_changed()
{
	if (code_complete_enabled && text_editor->is_insert_text_operation()) {
		code_complete_timer_line = text_editor->get_caret_line();
		code_complete_timer->start();
	}

	idle->start();

	if (find_replace_bar) {
		find_replace_bar->needs_to_count_results = true;
	}
}

void CodeTextEditor::_code_complete_timer_timeout()
{
	if (!is_visible_in_tree()) {
		return;
	}
	text_editor->request_code_completion();
}

void CodeTextEditor::set_find_replace_bar(FindReplaceBar* p_bar)
{
	if (find_replace_bar) {
		return;
	}

	find_replace_bar = p_bar;
	find_replace_bar->set_text_edit(this);
}

void CodeTextEditor::remove_find_replace_bar()
{
	if (!find_replace_bar) {
		return;
	}

	find_replace_bar = nullptr;
}

void CodeTextEditor::trim_trailing_whitespace()
{
	bool trimmed_whitespace = false;
	for (int i = 0; i < text_editor->get_line_count(); i++) {
		String line = text_editor->get_line(i);
		if (line.ends_with(" ") || line.ends_with("\t")) {
			if (!trimmed_whitespace) {
				text_editor->begin_complex_operation();
				trimmed_whitespace = true;
			}

			int end = 0;
			for (int j = line.length() - 1; j > -1; j--) {
				if (line[j] != ' ' && line[j] != '\t') {
					end = j + 1;
					break;
				}
			}
			text_editor->remove_text(i, end, i, line.length());
		}
	}

	if (trimmed_whitespace) {
		text_editor->merge_overlapping_carets();
		text_editor->end_complex_operation();
	}
}

void CodeTextEditor::trim_final_newlines()
{
	int final_line = text_editor->get_line_count() - 1;
	int check_line = final_line;

	String line = text_editor->get_line(check_line);

	while (line.is_empty() && check_line > -1) {
		--check_line;

		line = text_editor->get_line(check_line);
	}

	++check_line;

	if (check_line < final_line) {
		text_editor->begin_complex_operation();

		text_editor->remove_text(check_line, 0, final_line, 0);

		text_editor->merge_overlapping_carets();
		text_editor->end_complex_operation();
		text_editor->queue_redraw();
	}
}

void CodeTextEditor::insert_final_newline()
{
	int final_line = text_editor->get_line_count() - 1;
	String line = text_editor->get_line(final_line);

	// Length 0 means it's already an empty line, no need to add a newline.
	if (line.length() > 0 && !line.ends_with("\n")) {
		text_editor->insert_text("\n", final_line, line.length(), false);
	}
}

void CodeTextEditor::convert_case(CaseStyle p_case)
{
	if (!text_editor->has_selection()) {
		return;
	}
	text_editor->begin_complex_operation();
	text_editor->begin_multicaret_edit();

	for (int c = 0; c < text_editor->get_caret_count(); c++) {
		if (text_editor->multicaret_edit_ignore_caret(c)) {
			continue;
		}
		if (!text_editor->has_selection(c)) {
			continue;
		}

		int begin = text_editor->get_selection_from_line(c);
		int end = text_editor->get_selection_to_line(c);
		int begin_col = text_editor->get_selection_from_column(c);
		int end_col = text_editor->get_selection_to_column(c);

		for (int i = begin; i <= end; i++) {
			int len = text_editor->get_line(i).length();
			if (i == end) {
				len = end_col;
			}
			if (i == begin) {
				len -= begin_col;
			}
			String new_line = text_editor->get_line(i).substr(i == begin ? begin_col : 0, len);

			switch (p_case) {
			case UPPER: {
				new_line = new_line.to_upper();
			} break;
			case LOWER: {
				new_line = new_line.to_lower();
			} break;
			case CAPITALIZE: {
				new_line = new_line.capitalize();
			} break;
			}

			if (i == begin) {
				new_line = text_editor->get_line(i).left(begin_col) + new_line;
			}
			if (i == end) {
				new_line = new_line + text_editor->get_line(i).substr(end_col);
			}
			text_editor->set_line(i, new_line);
		}
	}
	text_editor->end_multicaret_edit();
	text_editor->end_complex_operation();
}

void CodeTextEditor::set_indent_using_spaces(bool p_use_spaces)
{
	text_editor->set_indent_using_spaces(p_use_spaces);
	indentation_txt->set_text(
		p_use_spaces ? TTR("Spaces", "Indentation") : TTR("Tabs", "Indentation"));
}

void CodeTextEditor::toggle_inline_comment(const String& delimiter)
{
	text_editor->begin_complex_operation();
	text_editor->begin_multicaret_edit();

	Vector<Point2i> line_ranges = text_editor->get_line_ranges_from_carets();
	int folded_to = 0;
	for (Point2i line_range : line_ranges) {
		int from_line = line_range.x;
		int to_line = line_range.y;
		// If last line is folded, extends to the end of the folded section
		if (text_editor->is_line_folded(to_line)) {
			folded_to = text_editor->get_next_visible_line_offset_from(to_line + 1, 1) - 1;
			to_line += folded_to;
		}
		// Check first if there's any uncommented lines in selection.
		bool is_commented = true;
		bool is_all_empty = true;
		for (int line = from_line; line <= to_line; line++) {
			// `+ delimiter.length()` here because comment delimiter is not actually `in comment` so
			// we check first character after it
			int delimiter_idx = text_editor->is_in_comment(
				line, text_editor->get_first_non_whitespace_column(line) + delimiter.length());
			// Empty lines should not be counted.
			bool is_empty = text_editor->get_line(line).strip_edges().is_empty();
			is_all_empty = is_all_empty && is_empty;
			// get_delimiter_start_key will return `##` instead of `#` when there is multiple
			// comment delimiter in a line.
			if (!is_empty &&
				(delimiter_idx == -1 ||
					!text_editor->get_delimiter_start_key(delimiter_idx).begins_with(delimiter))) {
				is_commented = false;
				break;
			}
		}

		// Special case for commenting empty lines, treat it/them as uncommented lines.
		is_commented = is_commented && !is_all_empty;

		// Comment/uncomment.
		for (int line = from_line; line <= to_line; line++) {
			if (is_all_empty) {
				text_editor->insert_text(delimiter, line, 0);
				continue;
			}

			if (is_commented) {
				int delimiter_column = text_editor->get_line(line).find(delimiter);
				if (delimiter_column != -1) {
					text_editor->remove_text(
						line, delimiter_column, line, delimiter_column + delimiter.length());
				}
			}
			else {
				text_editor->insert_text(
					delimiter, line, text_editor->get_first_non_whitespace_column(line));
			}
		}
	}

	text_editor->end_multicaret_edit();
	text_editor->end_complex_operation();
}

void CodeTextEditor::goto_line_without_history(int p_line, int p_column)
{
	text_editor->remove_secondary_carets();
	text_editor->deselect();
	text_editor->unfold_line(CLAMP(p_line, 0, text_editor->get_line_count() - 1));
	text_editor->set_caret_line(p_line, false);
	text_editor->set_caret_column(p_column, false);
	text_editor->set_code_hint("");
	adjust_viewport_to_caret();
}

void CodeTextEditor::goto_line(int p_line, int p_column)
{
	goto_line_without_history(p_line, p_column);
	trigger_history_save_on_navigate();
}

void CodeTextEditor::goto_line_selection(int p_line, int p_begin, int p_end)
{
	text_editor->remove_secondary_carets();
	text_editor->unfold_line(CLAMP(p_line, 0, text_editor->get_line_count() - 1));
	text_editor->select(p_line, p_begin, p_line, p_end);
	text_editor->set_code_hint("");
	adjust_viewport_to_caret();
	trigger_history_save_on_navigate();
}

void CodeTextEditor::goto_line_centered(int p_line, int p_column)
{
	text_editor->remove_secondary_carets();
	text_editor->deselect();
	text_editor->unfold_line(CLAMP(p_line, 0, text_editor->get_line_count() - 1));
	text_editor->set_caret_line(p_line, false);
	text_editor->set_caret_column(p_column, false);
	text_editor->set_code_hint("");
	center_viewport_to_caret();
	trigger_history_save_on_navigate();
}

void CodeTextEditor::goto_line_and_center_if_necessary(int p_line, int p_column)
{
	if (!text_editor->is_line_in_viewport(CLAMP(p_line, 0, text_editor->get_line_count() - 1))) {
		goto_line_centered(p_line, p_column);
	}
	else {
		goto_line(p_line, p_column);
	}
}

void CodeTextEditor::set_executing_line(int p_line)
{
	text_editor->set_line_as_executing(p_line, true);
}

void CodeTextEditor::clear_executing_line() { text_editor->clear_executing_lines(); }

bool CodeTextEditor::is_previewing_navigation_change() const { return preview_navigation_change; }

void CodeTextEditor::set_error(const String& p_error)
{
	error->set_text(p_error);

	_update_error_content_height();

	if (p_error.is_empty()) {
		error->set_default_cursor_shape(CURSOR_ARROW);
	}
	else {
		error->set_default_cursor_shape(CURSOR_POINTING_HAND);
	}
}

void CodeTextEditor::set_error_pos(int p_line, int p_column)
{
	error_line = p_line;
	error_column = p_column;
}

Point2i CodeTextEditor::get_error_pos() const { return Point2i(error_line, error_column); }

Point2i CodeTextEditor::get_pos_for_display(Point2i p_internal_position) const
{
	const String line_text = text_editor->get_line(p_internal_position.x);
	const int indent_size = text_editor->get_indent_size();

	int corrected_column = 0;
	for (int i = 0; i < p_internal_position.y; i++) {
		if (line_text[i] == '\t') {
			corrected_column += indent_size - (corrected_column % indent_size);
		}
		else {
			corrected_column += 1;
		}
	}

	return Point2(p_internal_position.x + 1, corrected_column + 1);
}

void CodeTextEditor::goto_error()
{
	if (!error->get_text().is_empty()) {
		goto_line_centered(error_line, error_column);
	}
}

void CodeTextEditor::validate_script() { idle->start(); }

void CodeTextEditor::_error_button_pressed()
{
	_set_show_errors_panel(!is_errors_panel_opened);
	_set_show_warnings_panel(false);
}

void CodeTextEditor::_warning_button_pressed()
{
	_set_show_warnings_panel(!is_warnings_panel_opened);
	_set_show_errors_panel(false);
}

void CodeTextEditor::_error_pressed(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		goto_error();
	}
}

void CodeTextEditor::set_error_count(int p_error_count)
{
	error_button->set_text(itos(p_error_count));
	error_button->set_visible(p_error_count > 0);
	if (p_error_count > 0) {
		idle->set_wait_time(idle_time_with_errors); // Parsing should happen sooner.
	}
	else {
		_set_show_errors_panel(false);
		idle->set_wait_time(idle_time);
	}
}

void CodeTextEditor::set_warning_count(int p_warning_count)
{
	warning_button->set_text(itos(p_warning_count));
	warning_button->set_visible(p_warning_count > 0);
	if (!p_warning_count) {
		_set_show_warnings_panel(false);
	}
}

void CodeTextEditor::toggle_bookmark()
{
	Vector<int> sorted_carets = text_editor->get_sorted_carets();
	int last_line = -1;
	for (const int& c : sorted_carets) {
		int from = text_editor->get_selection_from_line(c);
		from += from == last_line ? 1 : 0;
		int to = text_editor->get_selection_to_line(c);
		if (to < from) {
			continue;
		}
		// Check first if there's any bookmarked lines in the selection.
		bool selection_has_bookmarks = false;
		for (int line = from; line <= to; line++) {
			if (text_editor->is_line_bookmarked(line)) {
				selection_has_bookmarks = true;
				break;
			}
		}

		// Set bookmark on caret or remove all bookmarks from the selection.
		if (!selection_has_bookmarks) {
			if (text_editor->get_caret_line(c) != last_line) {
				text_editor->set_line_as_bookmarked(text_editor->get_caret_line(c), true);
			}
		}
		else {
			for (int line = from; line <= to; line++) {
				text_editor->set_line_as_bookmarked(line, false);
			}
		}
		last_line = to;
	}
}

void CodeTextEditor::goto_next_bookmark()
{
	PackedInt32Array bmarks = text_editor->get_bookmarked_lines();
	if (bmarks.is_empty()) {
		return;
	}

	int current_line = text_editor->get_caret_line();
	int bmark_idx = 0;
	if (current_line < (int)bmarks[bmarks.size() - 1]) {
		while (bmark_idx < bmarks.size() && bmarks[bmark_idx] <= current_line) {
			bmark_idx++;
		}
	}
	goto_line_centered(bmarks[bmark_idx]);
}

void CodeTextEditor::goto_prev_bookmark()
{
	PackedInt32Array bmarks = text_editor->get_bookmarked_lines();
	if (bmarks.is_empty()) {
		return;
	}

	int current_line = text_editor->get_caret_line();
	int bmark_idx = bmarks.size() - 1;
	if (current_line > (int)bmarks[0]) {
		while (bmark_idx >= 0 && bmarks[bmark_idx] >= current_line) {
			bmark_idx--;
		}
	}
	goto_line_centered(bmarks[bmark_idx]);
}

void CodeTextEditor::remove_all_bookmarks() { text_editor->clear_bookmarked_lines(); }

void CodeTextEditor::_zoom_in()
{
	int s = text_editor->get_theme_font_size(SceneStringName(font_size));
	_zoom_to(zoom_factor * (s + MAX(1.0f, EDSCALE)) / s);
}

void CodeTextEditor::_zoom_out()
{
	int s = text_editor->get_theme_font_size(SceneStringName(font_size));
	_zoom_to(zoom_factor * (s - MAX(1.0f, EDSCALE)) / s);
}

float CodeTextEditor::get_zoom_factor() { return zoom_factor; }

void CodeTextEditor::set_toggle_list_control(Control* p_toggle_list_control)
{
	toggle_files_list = p_toggle_list_control;
}

void CodeTextEditor::show_toggle_files_button() { toggle_files_button->show(); }

void CodeTextEditor::update_toggle_files_button()
{
	ERR_FAIL_NULL(toggle_files_list);
	bool forward = toggle_files_list->is_visible() == is_layout_rtl();
	toggle_files_button->set_button_icon(
		get_editor_theme_icon(forward ? SNAME("Forward") : SNAME("Back")));
	toggle_files_button->set_tooltip_text(vformat("%s (%s)", TTR("Toggle Files Panel"),
		ED_GET_SHORTCUT("script_editor/toggle_files_panel")->get_as_text()));
}


