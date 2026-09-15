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

bool FindReplaceBar::search_current()
{
	_update_flags(false);

	int line, col;
	_get_search_from(line, col, SEARCH_CURRENT);

	return _search(flags, line, col);
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

String FindReplaceBar::get_search_text() const { return search_text->get_text(); }

String FindReplaceBar::get_replace_text() const { return replace_text->get_text(); }

bool FindReplaceBar::is_case_sensitive() const { return case_sensitive->is_pressed(); }

bool FindReplaceBar::is_whole_words() const { return whole_words->is_pressed(); }

bool FindReplaceBar::is_selection_only() const { return selection_only->is_pressed(); }

/*** CODE EDITOR ****/

static constexpr float ZOOM_FACTOR_PRESETS[8] = {0.5f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.5f, 2.0f};

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

void CodeTextEditor::set_indent_using_spaces(bool p_use_spaces)
{
	text_editor->set_indent_using_spaces(p_use_spaces);
	indentation_txt->set_text(
		p_use_spaces ? TTR("Spaces", "Indentation") : TTR("Tabs", "Indentation"));
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


