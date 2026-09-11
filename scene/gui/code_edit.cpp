/**************************************************************************/
/*  code_edit.cpp                                                         */
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

#include "code_edit.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"
#include "core/string/translation_server.h"
#include "core/string/ustring.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"

void CodeEdit::_apply_project_settings()
{
	symbol_tooltip_timer->set_wait_time(GLOBAL_GET_CACHED(double, "gui/timers/tooltip_delay_sec"));
}

Control::CursorShape CodeEdit::get_cursor_shape(const Point2& p_pos) const
{
	if (!symbol_lookup_word.is_empty()) {
		return CURSOR_POINTING_HAND;
	}

	if (is_dragging_cursor()) {
		return TextEdit::get_cursor_shape(p_pos);
	}

	if ((code_completion_active && code_completion_rect.has_point(p_pos)) ||
		(!is_editable() && (!is_selecting_enabled() || get_line_count() == 0))) {
		return CURSOR_ARROW;
	}

	if (code_completion_active && code_completion_scroll_rect.has_point(p_pos)) {
		return CURSOR_ARROW;
	}

	Point2i pos = get_line_column_at_pos(p_pos, false);
	int line = pos.y;
	int col = pos.x;

	if (line != -1 && is_line_folded(line)) {
		int wrap_index = get_line_wrap_index_at_column(line, col);
		if (wrap_index == get_line_wrap_count(line)) {
			int eol_icon_width = theme_cache.folded_eol_icon->get_width();
			int left_margin = get_total_gutter_width() + eol_icon_width +
							  get_line_width(line, wrap_index) - get_h_scroll();
			if (p_pos.x > left_margin && p_pos.x <= left_margin + eol_icon_width + 3) {
				return CURSOR_POINTING_HAND;
			}
		}
	}

	return TextEdit::get_cursor_shape(p_pos);
}

void CodeEdit::_unhide_carets()
{
	// Unfold caret and selection origin.
	for (int i = 0; i < get_caret_count(); i++) {
		if (_is_line_hidden(get_caret_line(i))) {
			unfold_line(get_caret_line(i));
		}
		if (has_selection(i) && _is_line_hidden(get_selection_origin_line(i))) {
			unfold_line(get_selection_origin_line(i));
		}
	}
}

void CodeEdit::_handle_unicode_input_internal(const uint32_t p_unicode, int p_caret)
{
	start_action(EditAction::ACTION_TYPING);
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret != -1 && p_caret != i) {
			continue;
		}
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}

		bool had_selection = has_selection(i);
		String selection_text = (had_selection ? get_selected_text(i) : "");

		if (had_selection) {
			delete_selection(i);
		}

		// Remove the old character if in overtype mode and no selection.
		if (is_overtype_mode_enabled() && !had_selection) {
			// Make sure we don't try and remove empty space.
			if (get_caret_column(i) < get_line(get_caret_line(i)).length()) {
				remove_text(get_caret_line(i), get_caret_column(i), get_caret_line(i),
					get_caret_column(i) + 1);
			}
		}

		const char32_t chr[2] = {(char32_t)p_unicode, 0};

		if (auto_brace_completion_enabled) {
			int cl = get_caret_line(i);
			int cc = get_caret_column(i);

			if (had_selection) {
				insert_text_at_caret(chr, i);

				String close_key = get_auto_brace_completion_close_key(chr);
				if (!close_key.is_empty()) {
					insert_text_at_caret(selection_text + close_key, i);
					set_caret_column(get_caret_column(i) - 1, i == 0, i);
				}
			}
			else {
				int caret_move_offset = 1;

				int post_brace_pair =
					cc < get_line(cl).length() ? _get_auto_brace_pair_close_at_pos(cl, cc) : -1;

				if (has_string_delimiter(chr) && cc > 0 && !is_symbol(get_line(cl)[cc - 1]) &&
					post_brace_pair == -1) {
					insert_text_at_caret(chr, i);
				}
				else if (cc < get_line(cl).length() && !is_symbol(get_line(cl)[cc])) {
					insert_text_at_caret(chr, i);
				}
				else if (post_brace_pair != -1 &&
						   auto_brace_completion_pairs[post_brace_pair].close_key[0] == chr[0]) {
					caret_move_offset =
						auto_brace_completion_pairs[post_brace_pair].close_key.length();
				}
				else if (is_in_comment(cl, cc) != -1 ||
						   (is_in_string(cl, cc) != -1 && has_string_delimiter(chr))) {
					insert_text_at_caret(chr, i);
				}
				else {
					insert_text_at_caret(chr, i);

					int pre_brace_pair = _get_auto_brace_pair_open_at_pos(cl, cc + 1);
					if (pre_brace_pair != -1) {
						insert_text_at_caret(
							auto_brace_completion_pairs[pre_brace_pair].close_key, i);
					}
				}
				set_caret_column(cc + caret_move_offset, i == 0, i);
			}
		}
		else {
			insert_text_at_caret(chr, i);
		}
	}
	end_multicaret_edit();
	end_action();
}

void CodeEdit::_backspace_internal(int p_caret)
{
	if (!is_editable()) {
		return;
	}

	if (has_selection(p_caret)) {
		delete_selection(p_caret);
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret != -1 && p_caret != i) {
			continue;
		}
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}

		int to_line = get_caret_line(i);
		int to_column = get_caret_column(i);

		if (to_column == 0 && to_line == 0) {
			continue;
		}

		if (to_line > 0 && to_column == 0 && _is_line_hidden(to_line - 1)) {
			unfold_line(to_line - 1);
		}

		int from_line = to_column > 0 ? to_line : to_line - 1;
		int from_column = 0;
		if (to_column == 0) {
			from_column = get_line(to_line - 1).length();
		}
		else if (TextEdit::is_caret_mid_grapheme_enabled() ||
				   !TextEdit::is_backspace_deletes_composite_character_enabled()) {
			from_column = to_column - 1;
		}
		else {
			from_column = TextEdit::get_previous_composite_character_column(to_line, to_column);
		}

		merge_gutters(from_line, to_line);

		if (auto_brace_completion_enabled && to_column > 0) {
			int idx = _get_auto_brace_pair_open_at_pos(to_line, to_column);
			if (idx != -1) {
				from_column = to_column - auto_brace_completion_pairs[idx].open_key.length();

				if (_get_auto_brace_pair_close_at_pos(to_line, to_column) == idx) {
					to_column += auto_brace_completion_pairs[idx].close_key.length();
				}
			}
		}

		// For space indentation we need to do a basic unindent if there are no chars to the left,
		// acting the same way as tabs.
		if (indent_using_spaces && to_column != 0) {
			if (get_first_non_whitespace_column(to_line) >= to_column) {
				from_column = to_column - _calculate_spaces_till_next_left_indent(to_column);
				from_line = to_line;
			}
		}

		remove_text(from_line, from_column, to_line, to_column);

		set_caret_line(from_line, false, true, -1, i);
		set_caret_column(from_column, i == 0, i);
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::_cut_internal(int p_caret)
{
	// Overridden to unfold lines.
	_copy_internal(p_caret);

	if (!is_editable()) {
		return;
	}

	if (has_selection(p_caret)) {
		delete_selection(p_caret);
		return;
	}
	if (!is_empty_selection_clipboard_enabled()) {
		return;
	}
	if (p_caret == -1) {
		delete_lines();
	}
	else {
		unfold_line(get_caret_line(p_caret));
		remove_line_at(get_caret_line(p_caret));
	}
}

void CodeEdit::set_indent_size(const int p_size)
{
	ERR_FAIL_COND_MSG(p_size <= 0, "Indend size must be greater than 0.");
	if (indent_size == p_size) {
		return;
	}

	indent_size = p_size;
	if (indent_using_spaces) {
		indent_text = String(" ").repeat(p_size);
	}
	else {
		indent_text = "\t";
	}
	set_tab_size(p_size);
}

int CodeEdit::get_indent_size() const { return indent_size; }

void CodeEdit::set_indent_using_spaces(const bool p_use_spaces)
{
	indent_using_spaces = p_use_spaces;
	if (indent_using_spaces) {
		indent_text = String(" ").repeat(indent_size);
	}
	else {
		indent_text = "\t";
	}
}

bool CodeEdit::is_indent_using_spaces() const { return indent_using_spaces; }

void CodeEdit::set_auto_indent_enabled(bool p_enabled) { auto_indent = p_enabled; }

bool CodeEdit::is_auto_indent_enabled() const { return auto_indent; }

void CodeEdit::do_indent()
{
	if (!is_editable()) {
		return;
	}

	if (has_selection()) {
		indent_lines();
		return;
	}

	if (!indent_using_spaces) {
		insert_text_at_caret("\t");
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (multicaret_edit_ignore_caret(i)) {
			continue;
		}
		int spaces_to_add = _calculate_spaces_till_next_right_indent(get_caret_column(i));
		if (spaces_to_add > 0) {
			insert_text_at_caret(String(" ").repeat(spaces_to_add), i);
		}
	}
	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::indent_lines()
{
	if (!is_editable()) {
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();

	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	for (Point2i line_range : line_ranges) {
		for (int i = line_range.x; i <= line_range.y; i++) {
			const String line_text = get_line(i);
			if (line_text.is_empty()) {
				// Ignore empty lines.
				continue;
			}

			if (indent_using_spaces) {
				int spaces_to_add =
					_calculate_spaces_till_next_right_indent(get_first_non_whitespace_column(i));
				insert_text(String(" ").repeat(spaces_to_add), i, 0, false);
			}
			else {
				insert_text("\t", i, 0, false);
			}
		}
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::unindent_lines()
{
	if (!is_editable()) {
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();

	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	for (Point2i line_range : line_ranges) {
		for (int i = line_range.x; i <= line_range.y; i++) {
			const String line_text = get_line(i);

			if (line_text.begins_with("\t")) {
				remove_text(i, 0, i, 1);
			}
			else if (line_text.begins_with(" ")) {
				// Remove only enough spaces to align text to nearest full multiple of
				// indentation_size.
				int spaces_to_remove =
					_calculate_spaces_till_next_left_indent(get_first_non_whitespace_column(i));
				remove_text(i, 0, i, spaces_to_remove);
			}
		}
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::convert_indent(int p_from_line, int p_to_line)
{
	if (!is_editable()) {
		return;
	}

	// Check line range.
	p_from_line = (p_from_line < 0) ? 0 : p_from_line;
	p_to_line = (p_to_line < 0) ? get_line_count() - 1 : p_to_line;

	ERR_FAIL_COND(p_from_line >= get_line_count());
	ERR_FAIL_COND(p_to_line >= get_line_count());
	ERR_FAIL_COND(p_to_line < p_from_line);

	// Check lines within range.
	const char32_t from_indent_char = indent_using_spaces ? '\t' : ' ';
	int size_diff = indent_using_spaces ? indent_size - 1 : -(indent_size - 1);
	bool changed_indentation = false;
	for (int i = p_from_line; i <= p_to_line; i++) {
		String line = get_line(i);

		if (line.length() <= 0) {
			continue;
		}

		if (is_in_string(i) != -1) {
			continue;
		}

		// Check chars in the line.
		int j = 0;
		int space_count = 0;
		bool line_changed = false;
		while (j < line.length() && (line[j] == ' ' || line[j] == '\t')) {
			if (line[j] != from_indent_char) {
				space_count = 0;
				j++;
				continue;
			}
			space_count++;

			if (!indent_using_spaces && space_count != indent_size) {
				j++;
				continue;
			}

			line_changed = true;
			if (!changed_indentation) {
				begin_complex_operation();
				begin_multicaret_edit();
				changed_indentation = true;
			}

			// Calculate new line.
			line =
				line.left(j + ((size_diff < 0) ? size_diff : 0)) + indent_text + line.substr(j + 1);

			space_count = 0;
			j += size_diff;
		}

		if (line_changed) {
			// Use set line to preserve carets visual position.
			set_line(i, line);
		}
	}

	if (!changed_indentation) {
		return;
	}

	merge_overlapping_carets();
	end_multicaret_edit();
	end_complex_operation();
}

int CodeEdit::_calculate_spaces_till_next_left_indent(int p_column) const
{
	int spaces_till_indent = p_column % indent_size;
	if (spaces_till_indent == 0) {
		spaces_till_indent = indent_size;
	}
	return spaces_till_indent;
}

int CodeEdit::_calculate_spaces_till_next_right_indent(int p_column) const
{
	return indent_size - p_column % indent_size;
}

void CodeEdit::_new_line(bool p_split_current_line, bool p_above)
{
	if (!is_editable()) {
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();

	for (int i = 0; i < get_caret_count(); i++) {
		if (multicaret_edit_ignore_caret(i)) {
			continue;
		}
		// When not splitting the line, we need to factor in indentation from the end of the current
		// line.
		const int cc =
			p_split_current_line ? get_caret_column(i) : get_line(get_caret_line(i)).length();
		const int cl = get_caret_line(i);

		const String line = get_line(cl);

		String ins = "";
		if (!p_above) {
			ins = "\n";
		}

		// Append current indentation.
		int space_count = 0;
		int line_col = 0;
		for (; line_col < cc; line_col++) {
			if (line[line_col] == '\t') {
				ins += indent_text;
				space_count = 0;
				continue;
			}

			if (line[line_col] == ' ') {
				space_count++;

				if (space_count == indent_size) {
					ins += indent_text;
					space_count = 0;
				}
				continue;
			}
			break;
		}
		if (p_above) {
			ins += "\n";
		}

		if (is_line_folded(cl)) {
			unfold_line(cl);
		}

		// Indent once again if the previous line needs it, ie ':'.
		// Then add an addition new line for any closing pairs aka '()'.
		// Skip this in comments or if we are going above.
		bool brace_indent = false;
		if (auto_indent && !p_above && cc > 0 && is_in_comment(cl) == -1) {
			bool should_indent = false;
			char32_t indent_char = ' ';

			for (; line_col < cc; line_col++) {
				char32_t c = line[line_col];
				if (auto_indent_prefixes.has(c) && is_in_comment(cl, line_col) == -1) {
					should_indent = true;
					indent_char = c;
					continue;
				}

				// Make sure this is the last char, trailing whitespace or comments are okay.
				// Increment column for comments because the delimiter (#) should be ignored.
				if (should_indent && (!is_whitespace(c) && is_in_comment(cl, line_col + 1) == -1)) {
					should_indent = false;
				}
			}

			if (should_indent) {
				ins += indent_text;

				String closing_pair = get_auto_brace_completion_close_key(String::chr(indent_char));
				if (!closing_pair.is_empty() && line.find(closing_pair, cc) == cc) {
					// No need to move the brace below if we are not taking the text with us.
					if (p_split_current_line) {
						brace_indent = true;
						ins += "\n" + ins.substr(indent_text.size(), ins.length() - 2);
					}
					else {
						brace_indent = false;
						ins = "\n" + ins.substr(indent_text.size(), ins.length() - 2);
					}
				}
			}
		}

		if (p_split_current_line) {
			insert_text_at_caret(ins, i);
		}
		else {
			insert_text(ins, cl, p_above ? 0 : get_line(cl).length(), p_above, p_above);
			deselect(i);
			set_caret_line(p_above ? cl : cl + 1, false, true, -1, i);
			set_caret_column(get_line(get_caret_line(i)).length(), i == 0, i);
		}
		if (brace_indent) {
			// Move to inner indented line.
			set_caret_line(get_caret_line(i) - 1, false, true, 0, i);
			set_caret_column(get_line(get_caret_line(i)).length(), i == 0, i);
		}
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::set_auto_brace_completion_enabled(bool p_enabled)
{
	auto_brace_completion_enabled = p_enabled;
}

bool CodeEdit::is_auto_brace_completion_enabled() const { return auto_brace_completion_enabled; }

void CodeEdit::set_highlight_matching_braces_enabled(bool p_enabled)
{
	highlight_matching_braces_enabled = p_enabled;
	queue_redraw();
}

bool CodeEdit::is_highlight_matching_braces_enabled() const
{
	return highlight_matching_braces_enabled;
}

void CodeEdit::add_auto_brace_completion_pair(const String& p_open_key, const String& p_close_key)
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
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		ERR_FAIL_COND_MSG(auto_brace_completion_pairs[i].open_key == p_open_key,
			"auto brace completion open key '" + p_open_key + "' already exists.");
		if (p_open_key.length() < auto_brace_completion_pairs[i].open_key.length()) {
			at++;
		}
	}

	BracePair brace_pair;
	brace_pair.open_key = p_open_key;
	brace_pair.close_key = p_close_key;
	auto_brace_completion_pairs.insert(at, brace_pair);
}

bool CodeEdit::has_auto_brace_completion_open_key(const String& p_open_key) const
{
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		if (auto_brace_completion_pairs[i].open_key == p_open_key) {
			return true;
		}
	}
	return false;
}

bool CodeEdit::has_auto_brace_completion_close_key(const String& p_close_key) const
{
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		if (auto_brace_completion_pairs[i].close_key == p_close_key) {
			return true;
		}
	}
	return false;
}

String CodeEdit::get_auto_brace_completion_close_key(const String& p_open_key) const
{
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		if (auto_brace_completion_pairs[i].open_key == p_open_key) {
			return auto_brace_completion_pairs[i].close_key;
		}
	}
	return String();
}

void CodeEdit::_update_draw_main_gutter()
{
	set_gutter_draw(main_gutter, draw_breakpoints || draw_bookmarks || draw_executing_lines);
	int main_gutter_width = get_line_height() / (_is_bookmark_only() ? 2 : 1);
	set_gutter_width(main_gutter, main_gutter_width);
}

void CodeEdit::set_draw_breakpoints_gutter(bool p_draw)
{
	draw_breakpoints = p_draw;
	set_gutter_clickable(main_gutter, p_draw);
	_update_draw_main_gutter();
}

bool CodeEdit::is_drawing_breakpoints_gutter() const { return draw_breakpoints; }

void CodeEdit::set_draw_bookmarks_gutter(bool p_draw)
{
	draw_bookmarks = p_draw;
	_update_draw_main_gutter();
}

bool CodeEdit::is_drawing_bookmarks_gutter() const { return draw_bookmarks; }

void CodeEdit::set_draw_executing_lines_gutter(bool p_draw)
{
	draw_executing_lines = p_draw;
	_update_draw_main_gutter();
}

bool CodeEdit::is_drawing_executing_lines_gutter() const { return draw_executing_lines; }

void CodeEdit::_main_gutter_draw_callback(int p_line, int p_gutter, const Rect2& p_region)
{
	bool hovering = get_hovered_gutter() == Vector2i(main_gutter, p_line);
	RID ci = get_text_canvas_item();
	if (draw_breakpoints && theme_cache.breakpoint_icon.is_valid()) {
		bool breakpointed = is_line_breakpointed(p_line);
		bool shift_pressed = Input::get_singleton()->is_key_pressed(Key::SHIFT);

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

	if (draw_bookmarks && theme_cache.bookmark_icon.is_valid()) {
		bool bookmarked = is_line_bookmarked(p_line);
		bool shift_pressed = Input::get_singleton()->is_key_pressed(Key::SHIFT);

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

	if (draw_executing_lines && is_line_executing(p_line) &&
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

void CodeEdit::clear_breakpointed_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_breakpointed(i)) {
			set_line_as_breakpoint(i, false);
		}
	}
}

PackedInt32Array CodeEdit::get_breakpointed_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_breakpointed(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void CodeEdit::clear_bookmarked_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_bookmarked(i)) {
			set_line_as_bookmarked(i, false);
		}
	}
}

PackedInt32Array CodeEdit::get_bookmarked_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_bookmarked(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void CodeEdit::clear_executing_lines()
{
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_executing(i)) {
			set_line_as_executing(i, false);
		}
	}
}

PackedInt32Array CodeEdit::get_executing_lines() const
{
	PackedInt32Array ret;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_executing(i)) {
			ret.append(i);
		}
	}
	return ret;
}

void CodeEdit::set_draw_line_numbers(bool p_draw) { set_gutter_draw(line_number_gutter, p_draw); }

bool CodeEdit::is_draw_line_numbers_enabled() const { return is_gutter_drawn(line_number_gutter); }

void CodeEdit::set_line_numbers_zero_padded(bool p_zero_padded)
{
	String new_line_number_padding = p_zero_padded ? "0" : " ";
	if (line_number_padding == new_line_number_padding) {
		return;
	}

	line_number_padding = new_line_number_padding;
	_clear_line_number_text_cache();
	queue_redraw();
}

bool CodeEdit::is_line_numbers_zero_padded() const { return line_number_padding == "0"; }

void CodeEdit::set_line_numbers_min_digits(int p_count)
{
	if (line_numbers_min_digits == p_count) {
		return;
	}
	line_numbers_min_digits = p_count;

	int digits = MAX(line_numbers_min_digits, std::log10(get_line_count()) + 1);
	if (digits == line_number_digits) {
		return;
	}
	line_number_digits = digits;
	_clear_line_number_text_cache();
	_update_line_number_gutter_width();
	queue_redraw();
}

int CodeEdit::get_line_numbers_min_digits() const { return line_numbers_min_digits; }

void CodeEdit::_clear_line_number_text_cache()
{
	for (const KeyValue<int, RID>& KV : line_number_text_cache) {
		TS->free_rid(KV.value);
	}
	line_number_text_cache.clear();
}

void CodeEdit::_update_line_number_gutter_width()
{
	int width_in_chars =
		line_number_digits +
		(!is_drawing_fold_gutter() ? 1 : 0); // Extra padding if there is no fold gutter.
	set_gutter_width(line_number_gutter,
		width_in_chars * theme_cache.font->get_char_size('0', theme_cache.font_size).width);
}

void CodeEdit::set_draw_fold_gutter(bool p_draw)
{
	set_gutter_draw(fold_gutter, p_draw);
	_update_line_number_gutter_width();
}

bool CodeEdit::is_drawing_fold_gutter() const { return is_gutter_drawn(fold_gutter); }

void CodeEdit::_fold_gutter_draw_callback(int p_line, int p_gutter, Rect2 p_region)
{
	if (!can_fold_line(p_line) && !is_line_folded(p_line)) {
		set_line_gutter_clickable(p_line, fold_gutter, false);
		return;
	}
	set_line_gutter_clickable(p_line, fold_gutter, true);
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

void CodeEdit::set_line_folding_enabled(bool p_enabled)
{
	line_folding_enabled = p_enabled;
	_set_hiding_enabled(p_enabled);
}

bool CodeEdit::is_line_folding_enabled() const { return line_folding_enabled; }

bool CodeEdit::can_fold_line(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (!line_folding_enabled) {
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

bool CodeEdit::_fold_line(int p_line)
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

bool CodeEdit::_unfold_line(int p_line)
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

void CodeEdit::toggle_foldable_line(int p_line)
{
	ERR_FAIL_INDEX(p_line, get_line_count());
	if (is_line_folded(p_line)) {
		unfold_line(p_line);
		return;
	}
	fold_line(p_line);
}

void CodeEdit::toggle_foldable_lines_at_carets()
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

int CodeEdit::get_folded_line_header(int p_line) const
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

bool CodeEdit::is_line_folded(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	return p_line + 1 < get_line_count() && !_is_line_hidden(p_line) && _is_line_hidden(p_line + 1);
}

PackedInt32Array CodeEdit::get_folded_lines() const
{
	PackedInt32Array folded_lines;
	for (int i = 0; i < get_line_count(); i++) {
		if (is_line_folded(i)) {
			folded_lines.push_back(i);
		}
	}
	return folded_lines;
}

String CodeEdit::get_code_region_start_tag() const { return code_region_start_tag; }

String CodeEdit::get_code_region_end_tag() const { return code_region_end_tag; }

void CodeEdit::set_code_region_tags(const String& p_start, const String& p_end)
{
	ERR_FAIL_COND_MSG(p_start == p_end, "Starting and ending region tags cannot be identical.");
	ERR_FAIL_COND_MSG(p_start.is_empty(), "Starting region tag cannot be empty.");
	ERR_FAIL_COND_MSG(p_end.is_empty(), "Ending region tag cannot be empty.");
	code_region_start_tag = p_start;
	code_region_end_tag = p_end;
	_update_code_region_tags();
}

bool CodeEdit::is_line_code_region_start(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (code_region_start_string.is_empty()) {
		return false;
	}
	if (is_in_string(p_line) != -1) {
		return false;
	}
	Vector<String> split = get_line(p_line).strip_edges().split_spaces(1);
	return split.size() > 0 && split[0] == code_region_start_string;
}

bool CodeEdit::is_line_code_region_end(int p_line) const
{
	ERR_FAIL_INDEX_V(p_line, get_line_count(), false);
	if (code_region_start_string.is_empty()) {
		return false;
	}
	if (is_in_string(p_line) != -1) {
		return false;
	}
	Vector<String> split = get_line(p_line).strip_edges().split_spaces(1);
	return split.size() > 0 && split[0] == code_region_end_string;
}

void CodeEdit::add_string_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only)
{
	_add_delimiter(p_start_key, p_end_key, p_line_only, TYPE_STRING);
}

void CodeEdit::remove_string_delimiter(const String& p_start_key)
{
	_remove_delimiter(p_start_key, TYPE_STRING);
}

bool CodeEdit::has_string_delimiter(const String& p_start_key) const
{
	return _has_delimiter(p_start_key, TYPE_STRING);
}

void CodeEdit::set_string_delimiters(const TypedArray<String>& p_string_delimiters)
{
	_set_delimiters(p_string_delimiters, TYPE_STRING);
}

void CodeEdit::clear_string_delimiters() { _clear_delimiters(TYPE_STRING); }

int CodeEdit::is_in_string(int p_line, int p_column) const
{
	return _is_in_delimiter(p_line, p_column, TYPE_STRING);
}

void CodeEdit::add_comment_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only)
{
	_add_delimiter(p_start_key, p_end_key, p_line_only, TYPE_COMMENT);
}

void CodeEdit::remove_comment_delimiter(const String& p_start_key)
{
	_remove_delimiter(p_start_key, TYPE_COMMENT);
}

bool CodeEdit::has_comment_delimiter(const String& p_start_key) const
{
	return _has_delimiter(p_start_key, TYPE_COMMENT);
}

void CodeEdit::set_comment_delimiters(const TypedArray<String>& p_comment_delimiters)
{
	_set_delimiters(p_comment_delimiters, TYPE_COMMENT);
}

void CodeEdit::clear_comment_delimiters() { _clear_delimiters(TYPE_COMMENT); }

int CodeEdit::is_in_comment(int p_line, int p_column) const
{
	return _is_in_delimiter(p_line, p_column, TYPE_COMMENT);
}

String CodeEdit::get_delimiter_start_key(int p_delimiter_idx) const
{
	ERR_FAIL_INDEX_V(p_delimiter_idx, delimiters.size(), "");
	return delimiters[p_delimiter_idx].start_key;
}

String CodeEdit::get_delimiter_end_key(int p_delimiter_idx) const
{
	ERR_FAIL_INDEX_V(p_delimiter_idx, delimiters.size(), "");
	return delimiters[p_delimiter_idx].end_key;
}

Point2 CodeEdit::get_delimiter_start_position(int p_line, int p_column) const
{
	if (delimiters.is_empty()) {
		return Point2(-1, -1);
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), Point2(-1, -1));
	ERR_FAIL_COND_V(p_column - 1 > get_line(p_line).size(), Point2(-1, -1));

	Point2 start_position;
	start_position.y = -1;
	start_position.x = -1;

	bool in_region = ((p_line <= 0 || delimiter_cache[p_line - 1].size() < 1)
							 ? -1
							 : delimiter_cache[p_line - 1].back()->get()) != -1;

	/* Check the keys for this line. */
	for (const KeyValue<int, int>& E : delimiter_cache[p_line]) {
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
		if (delimiter_cache[i].size() < 1) {
			continue;
		}
		start_position.y = i;
		start_position.x = delimiter_cache[i].back()->key();

		/* Make sure it's not a multiline continuation. */
		line_length = get_line(i).length();
		if (line_length > 0 && start_position.x != line_length + 1) {
			break;
		}
	}
	return start_position;
}

Point2 CodeEdit::get_delimiter_end_position(int p_line, int p_column) const
{
	if (delimiters.is_empty()) {
		return Point2(-1, -1);
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), Point2(-1, -1));
	ERR_FAIL_COND_V(p_column - 1 > get_line(p_line).size(), Point2(-1, -1));

	Point2 end_position;
	end_position.y = -1;
	end_position.x = -1;

	int region = (p_line <= 0 || delimiter_cache[p_line - 1].size() < 1)
					 ? -1
					 : delimiter_cache[p_line - 1].back()->value();

	/* Check the keys for this line. */
	for (const KeyValue<int, int>& E : delimiter_cache[p_line]) {
		end_position.x = (E.value == -1) ? E.key : -1;
		if (E.key > p_column) {
			break;
		}
		region = E.value;
	}

	/* Region was found on this line and is not a multiline continuation. */
	if (region != -1 && end_position.x != -1 &&
		(delimiters[region].line_only || end_position.x != get_line(p_line).length() + 1)) {
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
		if (delimiter_cache[i].size() < 1 || delimiter_cache[i].front()->value() != -1) {
			continue;
		}
		end_position.x = delimiter_cache[i].front()->key();

		/* Make sure it's not a multiline continuation. */
		if (get_line(i).length() > 0 && end_position.x != get_line(i).length() + 1) {
			end_position.y = i;
			break;
		}
		end_position.x = -1;
	}
	return end_position;
}

void CodeEdit::set_code_hint(const String& p_hint)
{
	if (code_hint == p_hint) {
		return;
	}
	code_hint = p_hint;
	code_hint_xpos = -0xFFFF;
	queue_redraw();
}

void CodeEdit::set_code_hint_draw_below(bool p_below)
{
	if (code_hint_draw_below == p_below) {
		return;
	}
	code_hint_draw_below = p_below;
	queue_redraw();
}

void CodeEdit::set_code_completion_enabled(bool p_enable) { code_completion_enabled = p_enable; }

bool CodeEdit::is_code_completion_enabled() const { return code_completion_enabled; }

String CodeEdit::get_text_for_code_completion() const
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

int CodeEdit::get_code_completion_selected_index() const
{
	return (code_completion_active) ? code_completion_current_selected : -1;
}

void CodeEdit::cancel_code_completion()
{
	if (!code_completion_active) {
		return;
	}
	code_completion_forced = false;
	code_completion_active = false;
	is_code_completion_drag_started = false;
	queue_accessibility_update();
	queue_redraw();
}

void CodeEdit::set_symbol_lookup_on_click_enabled(bool p_enabled)
{
	symbol_lookup_on_click_enabled = p_enabled;
	set_symbol_lookup_word_as_valid(false);
}

bool CodeEdit::is_symbol_lookup_on_click_enabled() const { return symbol_lookup_on_click_enabled; }

String CodeEdit::get_text_for_symbol_lookup() const
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

String CodeEdit::get_text_with_cursor_char(int p_line, int p_column) const
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

String CodeEdit::get_lookup_word(int p_line, int p_column) const
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

void CodeEdit::set_symbol_lookup_word_as_valid(bool p_valid)
{
	symbol_lookup_word = p_valid ? symbol_lookup_new_word : "";
	symbol_lookup_new_word = "";
	if (lookup_symbol_word != symbol_lookup_word) {
		_set_symbol_lookup_word(symbol_lookup_word);
	}
}

void CodeEdit::set_symbol_tooltip_on_hover_enabled(bool p_enabled)
{
	symbol_tooltip_on_hover_enabled = p_enabled;
	if (!p_enabled) {
		symbol_tooltip_timer->stop();
	}
}

bool CodeEdit::is_symbol_tooltip_on_hover_enabled() const
{
	return symbol_tooltip_on_hover_enabled;
}

void CodeEdit::move_lines_up()
{
	begin_complex_operation();
	begin_multicaret_edit();

	// Move lines up by swapping each line with the one above it.
	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	for (Point2i line_range : line_ranges) {
		if (line_range.x == 0) {
			continue;
		}
		unfold_line(line_range.x - 1);
		for (int line = line_range.x; line <= line_range.y; line++) {
			unfold_line(line);
			swap_lines(line - 1, line);
		}
		// Fix selection if the last one ends at column 0, since it wasn't moved.
		for (int i = 0; i < get_caret_count(); i++) {
			if (has_selection(i) && get_selection_to_column(i) == 0 &&
				get_selection_to_line(i) == line_range.y + 1) {
				if (is_caret_after_selection_origin(i)) {
					set_caret_line(get_caret_line(i) - 1, false, true, -1, i);
				}
				else {
					set_selection_origin_line(get_selection_origin_line(i) - 1, true, -1, i);
				}
				break;
			}
		}
	}
	adjust_viewport_to_caret();

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::move_lines_down()
{
	begin_complex_operation();
	begin_multicaret_edit();

	// Move lines down by swapping each line with the one below it.
	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	// Reverse in case line ranges are adjacent, if the first ends at column 0.
	line_ranges.reverse();
	for (Point2i line_range : line_ranges) {
		if (line_range.y == get_line_count() - 1) {
			continue;
		}
		// Fix selection if the last one ends at column 0, since it won't be moved.
		bool selection_to_line_at_end = false;
		for (int i = 0; i < get_caret_count(); i++) {
			if (has_selection(i) && get_selection_to_column(i) == 0 &&
				get_selection_to_line(i) == line_range.y + 1) {
				selection_to_line_at_end = get_selection_to_line(i) == get_line_count() - 1;
				if (selection_to_line_at_end) {
					break;
				}
				if (is_caret_after_selection_origin(i)) {
					set_caret_line(get_caret_line(i) + 1, false, true, -1, i);
				}
				else {
					set_selection_origin_line(get_selection_origin_line(i) + 1, true, -1, i);
				}
				break;
			}
		}
		if (selection_to_line_at_end) {
			continue;
		}

		unfold_line(line_range.y + 1);
		for (int line = line_range.y; line >= line_range.x; line--) {
			unfold_line(line);
			swap_lines(line + 1, line);
		}
	}
	adjust_viewport_to_caret();

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::delete_lines()
{
	begin_complex_operation();
	begin_multicaret_edit();

	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	int line_offset = 0;
	for (Point2i line_range : line_ranges) {
		// Remove last line of range separately to preserve carets.
		unfold_line(line_range.y + line_offset);
		remove_line_at(line_range.y + line_offset);
		if (line_range.x != line_range.y) {
			remove_text(line_range.x + line_offset, 0, line_range.y + line_offset, 0);
		}
		line_offset += line_range.x - line_range.y - 1;
	}

	// Deselect all.
	deselect();

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::join_lines(const String& p_line_ending)
{
	ERR_FAIL_COND_MSG(p_line_ending.contains_char('\n'), "Cannot join lines with a newline.");

	begin_complex_operation();
	begin_multicaret_edit();

	Vector<Point2i> line_ranges = get_line_ranges_from_carets();
	int line_offset = 0;
	for (const Point2i& line_range : line_ranges) {
		for (int32_t line_index = line_range.x; line_index <= line_range.y; line_index++) {
			int32_t real_line = line_index + line_offset;
			if (real_line + 1 >= get_line_count()) {
				break;
			}
			unfold_line(real_line);
			String line = get_line(real_line);
			int line_length = line.length();
			int next_line_leading_whitespace_length =
				get_first_non_whitespace_column(real_line + 1);
			int next_line_length = get_line(real_line + 1).length();
			int corrected_line_length = line_length - 1;
			for (; corrected_line_length >= 0; corrected_line_length--) {
				if (!is_whitespace(line[corrected_line_length])) {
					break;
				}
			}
			corrected_line_length++;
			remove_text(real_line, corrected_line_length, real_line + 1,
				next_line_leading_whitespace_length);
			if (next_line_leading_whitespace_length != next_line_length &&
				corrected_line_length != 0) {
				insert_text(p_line_ending, real_line, corrected_line_length);
			}
			line_offset--;
		}
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::duplicate_selection()
{
	begin_complex_operation();
	begin_multicaret_edit();

	// Duplicate lines from carets without selections first.
	for (int i = 0; i < get_caret_count(); i++) {
		if (multicaret_edit_ignore_caret(i)) {
			continue;
		}
		for (int l = get_selection_from_line(i); l <= get_selection_to_line(i); l++) {
			unfold_line(l);
		}
		if (has_selection(i)) {
			continue;
		}

		String text_to_insert = get_line(get_caret_line(i)) + "\n";
		// Insert new text before the line, so the caret is on the second one.
		insert_text(text_to_insert, get_caret_line(i), 0);
	}

	// Duplicate selections.
	for (int i = 0; i < get_caret_count(); i++) {
		if (multicaret_edit_ignore_caret(i)) {
			continue;
		}
		if (!has_selection(i)) {
			continue;
		}

		// Insert new text before the selection, so the caret is on the second one.
		insert_text(get_selected_text(i), get_selection_from_line(i), get_selection_from_column(i));
	}

	end_multicaret_edit();
	end_complex_operation();
}

void CodeEdit::duplicate_lines()
{
	begin_complex_operation();
	begin_multicaret_edit();

	Vector<Point2i> line_ranges = get_line_ranges_from_carets(false, false);
	int line_offset = 0;
	for (Point2i line_range : line_ranges) {
		// The text that will be inserted. All lines in one string.
		String text_to_insert;

		for (int i = line_range.x + line_offset; i <= line_range.y + line_offset; i++) {
			text_to_insert += get_line(i) + "\n";
			unfold_line(i);
		}

		// Insert new text before the line.
		insert_text(text_to_insert, line_range.x + line_offset, 0);
		line_offset += line_range.y - line_range.x + 1;
	}

	end_multicaret_edit();
	end_complex_operation();
}

Color CodeEdit::_get_brace_mismatch_color() const { return theme_cache.brace_mismatch_color; }

Color CodeEdit::_get_code_folding_color() const { return theme_cache.code_folding_color; }

Ref<Texture2D> CodeEdit::_get_folded_eol_icon() const { return theme_cache.folded_eol_icon; }

void CodeEdit::_bind_methods() {}

int CodeEdit::_get_auto_brace_pair_open_at_pos(int p_line, int p_col)
{
	const String& line = get_line(p_line);
	int caret_col = MIN(p_col, line.length());

	/* Should be fast enough, expecting low amount of pairs... */
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		const String& open_key = auto_brace_completion_pairs[i].open_key;
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

int CodeEdit::_get_auto_brace_pair_close_at_pos(int p_line, int p_col)
{
	const String& line = get_line(p_line);

	/* Should be fast enough, expecting low amount of pairs... */
	for (int i = 0; i < auto_brace_completion_pairs.size(); i++) {
		if (p_col + auto_brace_completion_pairs[i].close_key.length() > line.length()) {
			continue;
		}

		bool is_match = true;
		for (int j = 0; j < auto_brace_completion_pairs[i].close_key.length(); j++) {
			if (line[p_col + j] != auto_brace_completion_pairs[i].close_key[j]) {
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

void CodeEdit::_gutter_clicked(int p_line, int p_gutter)
{
	bool shift_pressed = Input::get_singleton()->is_key_pressed(Key::SHIFT);

	if (p_gutter == main_gutter) {
		if (draw_breakpoints && !shift_pressed) {
			set_line_as_breakpoint(p_line, !is_line_breakpointed(p_line));
		}
		else if (draw_bookmarks && shift_pressed) {
			set_line_as_bookmarked(p_line, !is_line_bookmarked(p_line));
		}
		return;
	}

	if (p_gutter == line_number_gutter) {
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

	if (p_gutter == fold_gutter) {
		if (is_line_folded(p_line)) {
			unfold_line(p_line);
		}
		else if (can_fold_line(p_line)) {
			fold_line(p_line);
		}
		return;
	}
}

void CodeEdit::_update_gutter_indexes()
{
	for (int i = 0; i < get_gutter_count(); i++) {
		if (get_gutter_name(i) == "main_gutter") {
			main_gutter = i;
			continue;
		}

		if (get_gutter_name(i) == "line_numbers") {
			line_number_gutter = i;
			continue;
		}

		if (get_gutter_name(i) == "fold_gutter") {
			fold_gutter = i;
			continue;
		}
	}
}

void CodeEdit::_update_code_region_tags()
{
	code_region_start_string = "";
	code_region_end_string = "";

	if (code_region_start_tag.is_empty() || code_region_end_tag.is_empty()) {
		return;
	}

	// A shorter delimiter has higher priority.
	for (int i = delimiters.size() - 1; i >= 0; i--) {
		if (delimiters[i].type != DelimiterType::TYPE_COMMENT) {
			continue;
		}
		if (delimiters[i].end_key.is_empty() && delimiters[i].line_only == true) {
			code_region_start_string = delimiters[i].start_key + code_region_start_tag;
			code_region_end_string = delimiters[i].start_key + code_region_end_tag;
			return;
		}
	}
}

void CodeEdit::_update_delimiter_cache(int p_from_line, int p_to_line)
{
	if (delimiters.is_empty()) {
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
				delimiter_cache.remove_at(i);
			}
		}
		else {
			for (int i = start_line; i < end_line; i++) {
				delimiter_cache.insert(i, RBMap<int, int>());
			}
		}
	}

	int in_region = -1;
	for (int i = start_line; i < MIN(end_line + 1, line_count); i++) {
		int current_end_region =
			(i < 0 || delimiter_cache[i].size() < 1) ? -1 : delimiter_cache[i].back()->value();
		in_region = (i <= 0 || delimiter_cache[i - 1].size() < 1)
						? -1
						: delimiter_cache[i - 1].back()->value();

		const String& str = get_line(i);
		const int line_length = str.length();
		delimiter_cache.write[i].clear();

		if (str.length() == 0) {
			if (in_region != -1) {
				delimiter_cache.write[i][0] = in_region;
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
				for (int d = 0; d < delimiters.size(); d++) {
					/* check there is enough room */
					int chars_left = line_length - from;
					int start_key_length = delimiters[d].start_key.length();
					int end_key_length = delimiters[d].end_key.length();
					if (chars_left < start_key_length) {
						continue;
					}

					/* search the line */
					bool match = true;
					const char32_t* start_key = delimiters[d].start_key.get_data();
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
					delimiter_cache.write[i][from + 1] = d;
					from += start_key_length;

					/* check if it's the whole line */
					if (end_key_length == 0 || delimiters[d].line_only ||
						from + end_key_length > line_length) {
						j = line_length;
						if (delimiters[d].line_only) {
							delimiter_cache.write[i][line_length + 1] = -1;
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
			int end_key_length = delimiters[in_region].end_key.length();
			const char32_t* end_key = delimiters[in_region].end_key.get_data();
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
				delimiter_cache.write[i][j + 1] = end_region;
			}
			in_region = -1;
		}

		if (i == end_line && current_end_region != end_region) {
			end_line++;
			end_line = MIN(end_line, line_count);
		}
	}
}

int CodeEdit::_is_in_delimiter(int p_line, int p_column, DelimiterType p_type) const
{
	if (delimiters.is_empty() || p_line >= delimiter_cache.size()) {
		return -1;
	}
	ERR_FAIL_INDEX_V(p_line, get_line_count(), 0);

	int region = (p_line <= 0 || delimiter_cache[p_line - 1].size() < 1)
					 ? -1
					 : delimiter_cache[p_line - 1].back()->value();
	bool in_region = region != -1 && delimiters[region].type == p_type;
	for (RBMap<int, int>::Element* E = delimiter_cache[p_line].front(); E; E = E->next()) {
		/* If column is specified, loop until the key is larger then the column. */
		if (p_column != -1) {
			if (E->key() > p_column) {
				break;
			}
			in_region = E->value() != -1 && delimiters[E->value()].type == p_type;
			region = in_region ? E->value() : -1;
			continue;
		}

		/* If no column, calculate if the entire line is a region       */
		/* excluding whitespace.                                       */
		const String line = get_line(p_line);
		if (!in_region) {
			if (E->value() == -1 || delimiters[E->value()].type != p_type) {
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

		if (delimiters[region].line_only) {
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

void CodeEdit::_add_delimiter(
	const String& p_start_key, const String& p_end_key, bool p_line_only, DelimiterType p_type)
{
	// If we are the editor allow "null" as a valid start key, otherwise users cannot add delimiters
	// via the inspector.
	if (!(Engine::get_singleton()->is_editor_hint() && p_start_key == "null")) {
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
	for (int i = 0; i < delimiters.size(); i++) {
		ERR_FAIL_COND_MSG(delimiters[i].start_key == p_start_key,
			"delimiter with start key '" + p_start_key + "' already exists.");
		if (p_start_key.length() < delimiters[i].start_key.length()) {
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
	delimiters.insert(at, delimiter);
	if (!setting_delimiters) {
		delimiter_cache.clear();
		_update_delimiter_cache();
	}
	if (p_type == DelimiterType::TYPE_COMMENT) {
		_update_code_region_tags();
	}
}

void CodeEdit::_remove_delimiter(const String& p_start_key, DelimiterType p_type)
{
	for (int i = 0; i < delimiters.size(); i++) {
		if (delimiters[i].start_key != p_start_key) {
			continue;
		}

		if (delimiters[i].type != p_type) {
			break;
		}

		delimiters.remove_at(i);
		if (!setting_delimiters) {
			delimiter_cache.clear();
			_update_delimiter_cache();
		}
		if (p_type == DelimiterType::TYPE_COMMENT) {
			_update_code_region_tags();
		}
		break;
	}
}

bool CodeEdit::_has_delimiter(const String& p_start_key, DelimiterType p_type) const
{
	for (int i = 0; i < delimiters.size(); i++) {
		if (delimiters[i].start_key == p_start_key) {
			return delimiters[i].type == p_type;
		}
	}
	return false;
}

void CodeEdit::_clear_delimiters(DelimiterType p_type)
{
	for (int i = delimiters.size() - 1; i >= 0; i--) {
		if (delimiters[i].type == p_type) {
			delimiters.remove_at(i);
		}
	}
	delimiter_cache.clear();
	if (!setting_delimiters) {
		_update_delimiter_cache();
	}
	if (p_type == DelimiterType::TYPE_COMMENT) {
		_update_code_region_tags();
	}
}

void CodeEdit::_lines_edited_from(int p_from_line, int p_to_line)
{
	_update_delimiter_cache(p_from_line, p_to_line);

	if (p_from_line == p_to_line) {
		return;
	}

	lines_edited_changed += p_to_line - p_from_line;
	lines_edited_from = (lines_edited_from == -1)
							? MIN(p_from_line, p_to_line)
							: MIN(lines_edited_from, MIN(p_from_line, p_to_line));
	lines_edited_to = (lines_edited_to == -1) ? MAX(p_from_line, p_to_line)
											  : MAX(lines_edited_from, MAX(p_from_line, p_to_line));
}

void CodeEdit::_text_set()
{
	lines_edited_from = 0;
	lines_edited_to = 9999;
	_text_changed();
}

void CodeEdit::_line_col_changed()
{
	if (!code_completion_active) {
		return;
	}

	if (get_caret_line() != code_completion_caret_line ||
		get_caret_column() != code_completion_caret_column) {
		cancel_code_completion();
	}
}

CodeEdit::~CodeEdit() { _clear_line_number_text_cache(); }


