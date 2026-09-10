/**************************************************************************/
/*  text_edit.cpp                                                         */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/alt_codes.h"
#include "core/string/string_builder.h"
#include "scene/gui/label.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_enums.h"
#include "text_edit.compat.inc"
#include "text_edit.h"

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
			RID rid = AccessibilityServer::get_singleton()->create_sub_text_edit_elements(
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
	RS::get_singleton()->canvas_item_add_line(
		text_ci, p_pos, p_pos + Vector2(0, line_height), handle_color, handle_line_width);

	Vector2 circle_center = p_pos + Vector2(0, line_height + selection_handle_radius);
	RS::get_singleton()->canvas_item_add_circle(
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

void TextEdit::unhandled_key_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		if (!k->is_pressed()) {
			return;
		}
		// Handle Unicode (with modifiers active, process after shortcuts).
		if (has_focus() && editable && (k->get_unicode() >= 32)) {
			handle_unicode_input(k->get_unicode());
			accept_event();
		}
	}
}

bool TextEdit::alt_input(const Ref<InputEvent>& p_gui_input)
{
	if (!editable) {
		return false;
	}
	Ref<InputEventKey> k = p_gui_input;
	if (k.is_valid()) {
		// Start Unicode Alt input (hold).
		if (k->is_alt_pressed() && k->get_keycode() == Key::KP_ADD && !alt_start &&
			!alt_start_no_hold) {
			if (has_selection()) {
				delete_selection();
			}
			alt_start = true;
			alt_code = 0;
			alt_mode = ALT_INPUT_UNICODE;
			ime_text = "u";
			ime_selection = Vector2i(0, -1);
			_update_ime_text();
			return true;
		}

		// Start Unicode input (press).
		if (k->is_action("ui_unicode_start", true) && !alt_start && !alt_start_no_hold) {
			if (has_selection()) {
				delete_selection();
			}
			alt_start_no_hold = true;
			alt_code = 0;
			alt_mode = ALT_INPUT_UNICODE;
			ime_text = "u";
			ime_selection = Vector2i(0, -1);
			_update_ime_text();
			return true;
		}

		// Start OEM Alt input (hold).
		if (k->is_alt_pressed() && k->get_keycode() >= Key::KP_1 && k->get_keycode() <= Key::KP_9 &&
			!alt_start && !alt_start_no_hold) {
			if (has_selection()) {
				delete_selection();
			}
			alt_start = true;
			alt_code = (uint32_t)(k->get_keycode() - Key::KP_0);
			alt_mode = ALT_INPUT_OEM;
			ime_text = vformat("o%s", String::num_int64(alt_code, 10));
			ime_selection = Vector2i(0, -1);
			_update_ime_text();
			return true;
		}

		// Start Windows Alt input (hold).
		if (k->is_alt_pressed() && k->get_keycode() == Key::KP_0 && !alt_start &&
			!alt_start_no_hold) {
			if (has_selection()) {
				delete_selection();
			}
			alt_start = true;
			alt_mode = ALT_INPUT_WIN;
			alt_code = 0;
			ime_text = "w";
			ime_selection = Vector2i(0, -1);
			_update_ime_text();
			return true;
		}

		// Update Unicode input.
		if (k->is_pressed() && ((k->is_alt_pressed() && alt_start) || alt_start_no_hold)) {
			if (k->get_keycode() >= Key::KEY_0 && k->get_keycode() <= Key::KEY_9) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					alt_code = alt_code << 4;
				}
				else {
					alt_code = alt_code * 10;
				}
				alt_code += (uint32_t)(k->get_keycode() - Key::KEY_0);
			}
			else if (k->get_keycode() >= Key::KP_0 && k->get_keycode() <= Key::KP_9) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					alt_code = alt_code << 4;
				}
				else {
					alt_code = alt_code * 10;
				}
				alt_code += (uint32_t)(k->get_keycode() - Key::KP_0);
			}
			else if (alt_mode == ALT_INPUT_UNICODE && k->get_keycode() >= Key::A &&
					   k->get_keycode() <= Key::F) {
				alt_code = alt_code << 4;
				alt_code += (uint32_t)(k->get_keycode() - Key::A) + 10;
			}
			else if ((Key)k->get_unicode() >= Key::KEY_0 && (Key)k->get_unicode() <= Key::KEY_9) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					alt_code = alt_code << 4;
				}
				else {
					alt_code = alt_code * 10;
				}
				alt_code += (uint32_t)((Key)k->get_unicode() - Key::KEY_0);
			}
			else if (alt_mode == ALT_INPUT_UNICODE && (Key)k->get_unicode() >= Key::A &&
					   (Key)k->get_unicode() <= Key::F) {
				alt_code = alt_code << 4;
				alt_code += (uint32_t)((Key)k->get_unicode() - Key::A) + 10;
			}
			else if (k->get_physical_keycode() >= Key::KEY_0 &&
					   k->get_physical_keycode() <= Key::KEY_9) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					alt_code = alt_code << 4;
				}
				else {
					alt_code = alt_code * 10;
				}
				alt_code += (uint32_t)(k->get_physical_keycode() - Key::KEY_0);
			}
			if (k->get_keycode() == Key::BACKSPACE) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					alt_code = alt_code >> 4;
				}
				else {
					alt_code = alt_code / 10;
				}
			}
			if (alt_code > 0x10ffff) {
				alt_code = 0x10ffff;
			}
			if (alt_code > 0) {
				if (alt_mode == ALT_INPUT_UNICODE) {
					ime_text = vformat("u%s", String::num_int64(alt_code, 16, true));
				}
				else if (alt_mode == ALT_INPUT_OEM) {
					ime_text = vformat("o%s", String::num_int64(alt_code, 10));
				}
				else if (alt_mode == ALT_INPUT_WIN) {
					ime_text = vformat("w%s", String::num_int64(alt_code, 10));
				}
			}
			else {
				if (alt_mode == ALT_INPUT_UNICODE) {
					ime_text = "u";
				}
				else if (alt_mode == ALT_INPUT_OEM) {
					ime_text = "o";
				}
				else if (alt_mode == ALT_INPUT_WIN) {
					ime_text = "w";
				}
			}
			ime_selection = Vector2i(0, -1);
			_update_ime_text();
			return true;
		}

		// Submit Unicode input.
		if ((!k->is_pressed() && alt_start && k->get_keycode() == Key::ALT) ||
			(alt_start_no_hold &&
				(k->is_action("ui_text_submit", true) || k->is_action("ui_accept", true)))) {
			alt_start = false;
			alt_start_no_hold = false;
			if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff)) {
				ime_text = String();
				ime_selection = Vector2i();
				if (alt_mode == ALT_INPUT_UNICODE) {
					if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff)) {
						handle_unicode_input(alt_code);
					}
				}
				else if (alt_mode == ALT_INPUT_OEM) {
					if (alt_code > 0x00 && alt_code <= 0xff) {
						handle_unicode_input(alt_code_oem437[alt_code]);
					}
					else if ((alt_code > 0xff && alt_code < 0xd800) || (alt_code > 0xdfff)) {
						handle_unicode_input(alt_code);
					}
				}
				else if (alt_mode == ALT_INPUT_WIN) {
					if (alt_code > 0x00 && alt_code <= 0xff) {
						handle_unicode_input(alt_code_cp1252[alt_code]);
					}
					else if ((alt_code > 0xff && alt_code < 0xd800) || (alt_code > 0xdfff)) {
						handle_unicode_input(alt_code);
					}
				}
				alt_mode = ALT_INPUT_NONE;
			}
			else {
				ime_text = String();
				ime_selection = Vector2i();
			}
			_update_ime_text();
			return true;
		}

		// Cancel Unicode input.
		if (alt_start_no_hold && k->is_action("ui_cancel", true)) {
			alt_start = false;
			alt_start_no_hold = false;
			alt_mode = ALT_INPUT_NONE;
			ime_text = String();
			ime_selection = Vector2i();
			_update_ime_text();
			return true;
		}
	}
	return false;
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

void TextEdit::_new_line(bool p_split_current_line, bool p_above)
{
	if (!editable) {
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();

	for (int i = 0; i < get_caret_count(); i++) {
		if (multicaret_edit_ignore_caret(i)) {
			continue;
		}
		if (p_split_current_line) {
			insert_text_at_caret("\n", i);
		}
		else {
			int line = get_caret_line(i);
			insert_text("\n", line, p_above ? 0 : text[line].length(), p_above, p_above);
			deselect(i);
			set_caret_line(p_above ? line : line + 1, false, true, -1, i);
			set_caret_column(0, i == 0, i);
		}
	}

	end_multicaret_edit();
	end_complex_operation();
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

void TextEdit::_do_backspace(bool p_word, bool p_all_to_left)
{
	if (!editable) {
		return;
	}

	start_action(EditAction::ACTION_BACKSPACE);
	begin_multicaret_edit();

	Vector<int> sorted_carets = get_sorted_carets();
	sorted_carets.reverse();
	for (int i = 0; i < sorted_carets.size(); i++) {
		int caret_index = sorted_carets[i];
		if (multicaret_edit_ignore_caret(caret_index)) {
			continue;
		}

		if (get_caret_column(caret_index) == 0 && get_caret_line(caret_index) == 0 &&
			!has_selection(caret_index)) {
			continue;
		}

		if (has_selection(caret_index) || (!p_all_to_left && !p_word) ||
			get_caret_column(caret_index) == 0) {
			backspace(caret_index);
			continue;
		}

		if (p_all_to_left) {
			// Remove everything to left of caret to the start of the line.
			int caret_current_column = get_caret_column(caret_index);
			_remove_text(
				get_caret_line(caret_index), 0, get_caret_line(caret_index), caret_current_column);
			collapse_carets(
				get_caret_line(caret_index), 0, get_caret_line(caret_index), caret_current_column);
			set_caret_column(0, caret_index == 0, caret_index);
			_offset_carets_after(
				get_caret_line(caret_index), caret_current_column, get_caret_line(caret_index), 0);
			continue;
		}

		if (p_word) {
			// Remove text to the start of the word left of the caret.
			int from_column = get_caret_column(caret_index);
			int column = get_caret_column(caret_index);
			// Check for the case "<word><space><caret>" and ignore the space.
			// No need to check for column being 0 since it is checked above.
			if (is_whitespace(
					text[get_caret_line(caret_index)][get_caret_column(caret_index) - 1])) {
				column -= 1;
			}

			// Get a list with the indices of the word bounds of the given text line.
			const PackedInt32Array words = TS->shaped_text_get_word_breaks(
				text.get_line_data(get_caret_line(caret_index))->get_rid());
			if (words.is_empty() || column <= words[0]) {
				// Delete to the start when there are no more words.
				column = 0;
			}
			else {
				// Otherwise search for the first word break that is smaller than the index from
				// we're currently deleting.
				for (int c = words.size() - 2; c >= 0; c = c - 2) {
					if (words[c] < column) {
						column = words[c];
						break;
					}
				}
			}

			_remove_text(
				get_caret_line(caret_index), column, get_caret_line(caret_index), from_column);
			collapse_carets(
				get_caret_line(caret_index), column, get_caret_line(caret_index), from_column);
			set_caret_column(column, caret_index == 0, caret_index);
			_offset_carets_after(
				get_caret_line(caret_index), from_column, get_caret_line(caret_index), column);
		}
	}

	end_multicaret_edit();
	end_action();
}

void TextEdit::_delete(bool p_word, bool p_all_to_right)
{
	if (!editable) {
		return;
	}

	start_action(EditAction::ACTION_DELETE);
	begin_multicaret_edit();

	Vector<int> sorted_carets = get_sorted_carets();
	for (int i = 0; i < sorted_carets.size(); i++) {
		int caret_index = sorted_carets[i];
		if (multicaret_edit_ignore_caret(caret_index)) {
			continue;
		}

		if (has_selection(caret_index)) {
			delete_selection(caret_index);
			continue;
		}

		int curline_len = text[get_caret_line(caret_index)].length();
		if (get_caret_line(caret_index) == text.size() - 1 &&
			get_caret_column(caret_index) == curline_len) {
			continue; // Last line, last column: Nothing to do.
		}

		int next_line = get_caret_column(caret_index) < curline_len
							? get_caret_line(caret_index)
							: get_caret_line(caret_index) + 1;
		int next_column;

		if (p_all_to_right) {
			if (get_caret_column(caret_index) == curline_len) {
				continue;
			}

			// Delete everything to right of caret.
			next_column = curline_len;
			next_line = get_caret_line(caret_index);
		}
		else if (p_word && get_caret_column(caret_index) < curline_len - 1) {
			// Delete next word to right of caret.
			int line = get_caret_line(caret_index);
			int column = get_caret_column(caret_index);

			PackedInt32Array words =
				TS->shaped_text_get_word_breaks(text.get_line_data(line)->get_rid());
			if (words.is_empty() || column >= words[words.size() - 1]) {
				// Delete to the end when there are no more words.
				column = text[get_caret_line(i)].length();
			}
			else {
				for (int j = 1; j < words.size(); j = j + 2) {
					if (words[j] > column) {
						column = words[j];
						break;
					}
				}
			}

			next_line = line;
			next_column = column;
		}
		else {
			// Delete one character.
			if (caret_mid_grapheme_enabled) {
				next_column = get_caret_column(caret_index) < curline_len
								  ? (get_caret_column(caret_index) + 1)
								  : 0;
			}
			else {
				next_column = get_caret_column(caret_index) < curline_len
								  ? TS->shaped_text_next_character_pos(
										text.get_line_data(get_caret_line(caret_index))->get_rid(),
										(get_caret_column(caret_index)))
								  : 0;
			}
		}

		_remove_text(
			get_caret_line(caret_index), get_caret_column(caret_index), next_line, next_column);
		collapse_carets(
			get_caret_line(caret_index), get_caret_column(caret_index), next_line, next_column);
		_offset_carets_after(
			next_line, next_column, get_caret_line(caret_index), get_caret_column(caret_index));
	}

	end_multicaret_edit();
	end_action();
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
		!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	DisplayServer::get_singleton()->window_set_ime_position(Point2(), wid);
	DisplayServer::get_singleton()->window_set_ime_active(false, wid);
}

void TextEdit::_update_ime_window_position()
{
	DisplayServerEnums::WindowID wid =
		get_window() ? get_window()->get_window_id() : DisplayServerEnums::INVALID_WINDOW_ID;
	if (wid == DisplayServerEnums::INVALID_WINDOW_ID ||
		!DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_IME)) {
		return;
	}
	if (!editable) {
		DisplayServer::get_singleton()->window_set_ime_active(false, wid);
		return;
	}
	DisplayServer::get_singleton()->window_set_ime_active(true, wid);
	Point2 pos = get_global_position() + get_caret_draw_pos();
	if (get_window()->get_embedder()) {
		pos += get_viewport()->get_popup_base_transform().get_origin();
	}
	// Take into account the window's transform.
	pos = get_window()->get_screen_transform().xform(pos);
	// The window will move to the updated position the next time the IME is updated, not
	// immediately.
	DisplayServer::get_singleton()->window_set_ime_position(pos, wid);
}

void TextEdit::_show_virtual_keyboard()
{
	_update_ime_window_position();

	if (virtual_keyboard_enabled &&
		DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_VIRTUAL_KEYBOARD)) {
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

		DisplayServer::get_singleton()->virtual_keyboard_show(get_text(), get_global_rect(),
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

void TextEdit::apply_ime()
{
	if (!has_ime_text()) {
		_close_ime_window();
		return;
	}

	// Force apply the current IME text.
	if (alt_start || alt_start_no_hold) {
		cancel_ime();
		if ((alt_code > 0x31 && alt_code < 0xd800) || (alt_code > 0xdfff && alt_code <= 0x10ffff)) {
			handle_unicode_input(alt_code);
		}
	}
	else {
		String insert_ime_text = ime_text;
		cancel_ime();
		insert_text_at_caret(insert_ime_text);
	}
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
	DisplayServer::get_singleton()->show_emoji_and_symbol_picker();
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

void TextEdit::set_line(int p_line, const String& p_new_text)
{
	if (p_line < 0 || p_line >= text.size()) {
		return;
	}
	begin_complex_operation();

	int old_column = text[p_line].length();

	// Set the affected carets column to update their last offset x.
	for (int i = 0; i < get_caret_count(); i++) {
		if (_is_line_col_in_range(
				get_caret_line(i), get_caret_column(i), p_line, 0, p_line, old_column)) {
			set_caret_column(get_caret_column(i), false, i);
		}
		if (has_selection(i) &&
			_is_line_col_in_range(get_selection_origin_line(i), get_selection_origin_column(i),
				p_line, 0, p_line, old_column)) {
			set_selection_origin_column(get_selection_origin_column(i), i);
		}
	}

	_remove_text(p_line, 0, p_line, old_column);
	int new_line, new_column;
	_insert_text(p_line, 0, p_new_text, &new_line, &new_column);

	// Don't offset carets that were on the old line.
	_offset_carets_after(p_line, old_column, new_line, new_column, false, false);

	// Set the caret lines to update the column to match visually.
	for (int i = 0; i < get_caret_count(); i++) {
		if (_is_line_col_in_range(
				get_caret_line(i), get_caret_column(i), p_line, 0, p_line, old_column)) {
			set_caret_line(get_caret_line(i), false, true, 0, i);
		}
		if (has_selection(i) &&
			_is_line_col_in_range(get_selection_origin_line(i), get_selection_origin_column(i),
				p_line, 0, p_line, old_column)) {
			set_selection_origin_line(get_selection_origin_line(i), true, 0, i);
		}
	}
	merge_overlapping_carets();
	end_complex_operation();
}

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

void TextEdit::swap_lines(int p_from_line, int p_to_line)
{
	ERR_FAIL_INDEX(p_from_line, text.size());
	ERR_FAIL_INDEX(p_to_line, text.size());

	if (p_from_line == p_to_line) {
		return;
	}

	String from_line_text = get_line(p_from_line);
	String to_line_text = get_line(p_to_line);
	begin_complex_operation();
	begin_multicaret_edit();
	// Don't use set_line to avoid clamping and updating carets.
	_remove_text(p_to_line, 0, p_to_line, text[p_to_line].length());
	_insert_text(p_to_line, 0, from_line_text);
	_remove_text(p_from_line, 0, p_from_line, text[p_from_line].length());
	_insert_text(p_from_line, 0, to_line_text);

	// Swap carets.
	for (int i = 0; i < get_caret_count(); i++) {
		bool selected = has_selection(i);
		if (get_caret_line(i) == p_from_line || get_caret_line(i) == p_to_line) {
			int caret_new_line = get_caret_line(i) == p_from_line ? p_to_line : p_from_line;
			int caret_column = get_caret_column(i);
			set_caret_line(caret_new_line, false, true, -1, i);
			set_caret_column(caret_column, false, i);
		}
		if (selected && (get_selection_origin_line(i) == p_from_line ||
							get_selection_origin_line(i) == p_to_line)) {
			int origin_new_line =
				get_selection_origin_line(i) == p_from_line ? p_to_line : p_from_line;
			int origin_column = get_selection_origin_column(i);
			select(origin_new_line, origin_column, get_caret_line(i), get_caret_column(i), i);
		}
	}
	// If only part of a selection was changed, it may now overlap.
	merge_overlapping_carets();

	end_multicaret_edit();
	end_complex_operation();
}

void TextEdit::insert_line_at(int p_line, const String& p_text)
{
	ERR_FAIL_INDEX(p_line, text.size());

	// Use a complex operation so subsequent calls aren't merged together.
	begin_complex_operation();

	int new_line, new_column;
	_insert_text(p_line, 0, p_text + "\n", &new_line, &new_column);
	_offset_carets_after(p_line, 0, new_line, new_column);

	end_complex_operation();
}

void TextEdit::insert_text_at_caret(const String& p_text, int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);

	begin_complex_operation();
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret != -1 && p_caret != i) {
			continue;
		}
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}

		delete_selection(i);

		int from_line = get_caret_line(i);
		int from_col = get_caret_column(i);

		int new_line, new_column;
		_insert_text(from_line, from_col, p_text, &new_line, &new_column);
		_update_scrollbars();
		_offset_carets_after(from_line, from_col, new_line, new_column);

		set_caret_line(new_line, false, true, -1, i);
		set_caret_column(new_column, i == 0, i);
	}

	if (has_ime_text()) {
		_update_ime_text();
	}

	end_multicaret_edit();
	end_complex_operation();
}

void TextEdit::insert_text(const String& p_text, int p_line, int p_column,
	bool p_before_selection_begin, bool p_before_selection_end)
{
	ERR_FAIL_INDEX(p_line, text.size());
	ERR_FAIL_INDEX(p_column, text[p_line].length() + 1);

	begin_complex_operation();

	int new_line, new_column;
	_insert_text(p_line, p_column, p_text, &new_line, &new_column);

	_offset_carets_after(
		p_line, p_column, new_line, new_column, p_before_selection_begin, p_before_selection_end);

	end_complex_operation();
}

void TextEdit::remove_text(int p_from_line, int p_from_column, int p_to_line, int p_to_column)
{
	ERR_FAIL_INDEX(p_from_line, text.size());
	ERR_FAIL_INDEX(p_from_column, text[p_from_line].length() + 1);
	ERR_FAIL_INDEX(p_to_line, text.size());
	ERR_FAIL_INDEX(p_to_column, text[p_to_line].length() + 1);
	ERR_FAIL_COND(p_to_line < p_from_line);
	ERR_FAIL_COND(p_to_line == p_from_line && p_to_column < p_from_column);

	begin_complex_operation();

	_remove_text(p_from_line, p_from_column, p_to_line, p_to_column);
	collapse_carets(p_from_line, p_from_column, p_to_line, p_to_column);
	_offset_carets_after(p_to_line, p_to_column, p_from_line, p_from_column);

	end_complex_operation();
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

void TextEdit::handle_unicode_input(const uint32_t p_unicode, int p_caret)
{
	_handle_unicode_input_internal(p_unicode, p_caret);
}

void TextEdit::backspace(int p_caret) { _backspace_internal(p_caret); }

void TextEdit::cut(int p_caret) { _cut_internal(p_caret); }

void TextEdit::copy(int p_caret) { _copy_internal(p_caret); }

void TextEdit::paste(int p_caret) { _paste_internal(p_caret); }

void TextEdit::paste_primary_clipboard(int p_caret) { _paste_primary_clipboard_internal(p_caret); }

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
	case MENU_CUT: {
		cut();
	} break;
	case MENU_COPY: {
		copy();
	} break;
	case MENU_PASTE: {
		paste();
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
	case MENU_INSERT_LRM: {
		if (editable) {
			insert_text_at_caret(String::chr(0x200E));
		}
	} break;
	case MENU_INSERT_RLM: {
		if (editable) {
			insert_text_at_caret(String::chr(0x200F));
		}
	} break;
	case MENU_INSERT_LRE: {
		if (editable) {
			insert_text_at_caret(String::chr(0x202A));
		}
	} break;
	case MENU_INSERT_RLE: {
		if (editable) {
			insert_text_at_caret(String::chr(0x202B));
		}
	} break;
	case MENU_INSERT_LRO: {
		if (editable) {
			insert_text_at_caret(String::chr(0x202D));
		}
	} break;
	case MENU_INSERT_RLO: {
		if (editable) {
			insert_text_at_caret(String::chr(0x202E));
		}
	} break;
	case MENU_INSERT_PDF: {
		if (editable) {
			insert_text_at_caret(String::chr(0x202C));
		}
	} break;
	case MENU_INSERT_ALM: {
		if (editable) {
			insert_text_at_caret(String::chr(0x061C));
		}
	} break;
	case MENU_INSERT_LRI: {
		if (editable) {
			insert_text_at_caret(String::chr(0x2066));
		}
	} break;
	case MENU_INSERT_RLI: {
		if (editable) {
			insert_text_at_caret(String::chr(0x2067));
		}
	} break;
	case MENU_INSERT_FSI: {
		if (editable) {
			insert_text_at_caret(String::chr(0x2068));
		}
	} break;
	case MENU_INSERT_PDI: {
		if (editable) {
			insert_text_at_caret(String::chr(0x2069));
		}
	} break;
	case MENU_INSERT_ZWJ: {
		if (editable) {
			insert_text_at_caret(String::chr(0x200D));
		}
	} break;
	case MENU_INSERT_ZWNJ: {
		if (editable) {
			insert_text_at_caret(String::chr(0x200C));
		}
	} break;
	case MENU_INSERT_WJ: {
		if (editable) {
			insert_text_at_caret(String::chr(0x2060));
		}
	} break;
	case MENU_INSERT_SHY: {
		if (editable) {
			insert_text_at_caret(String::chr(0x00AD));
		}
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
	ERR_FAIL_INDEX_V(p_line, text.size(),
String());

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

void TextEdit::set_caret_blink_enabled(bool p_enabled)
{
	if (caret_blink_enabled == p_enabled) {
		return;
	}

	caret_blink_enabled = p_enabled;

	if (has_focus()) {
		if (p_enabled) {
			caret_blink_timer->start();
		}
		else {
			caret_blink_timer->stop();
		}
	}
	draw_caret = true;
}

bool TextEdit::is_caret_blink_enabled() const { return caret_blink_enabled; }

float TextEdit::get_caret_blink_interval() const { return caret_blink_timer->get_wait_time(); }

void TextEdit::set_caret_blink_interval(const float p_interval)
{
	ERR_FAIL_COND(p_interval <= 0);
	caret_blink_timer->set_wait_time(p_interval);
}

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

void TextEdit::delete_selection(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);

	begin_complex_operation();
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret != -1 && p_caret != i) {
			continue;
		}
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}

		if (!has_selection(i)) {
			continue;
		}

		int selection_from_line = get_selection_from_line(i);
		int selection_from_column = get_selection_from_column(i);
		int selection_to_line = get_selection_to_line(i);
		int selection_to_column = get_selection_to_column(i);

		_remove_text(
			selection_from_line, selection_from_column, selection_to_line, selection_to_column);
		_offset_carets_after(
			selection_to_line, selection_to_column, selection_from_line, selection_from_column);
		merge_overlapping_carets();

		deselect(i);
		set_caret_line(selection_from_line, false, false, -1, i);
		set_caret_column(selection_from_column, i == 0, i);
	}
	end_multicaret_edit();
	end_complex_operation();
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

VScrollBar* TextEdit::get_v_scroll_bar() const { return v_scroll; }

HScrollBar* TextEdit::get_h_scroll_bar() const { return h_scroll; }

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

Ref<SyntaxHighlighter> TextEdit::get_syntax_highlighter() const { return syntax_highlighter; }

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
	// Override for functionality.
}

void TextEdit::_handle_unicode_input_internal(const uint32_t p_unicode, int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (!editable) {
		return;
	}

	start_action(EditAction::ACTION_TYPING);
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}
		if (p_caret != -1 && p_caret != i) {
			continue;
		}

		// Remove the old character if in insert mode and no selection.
		if (overtype_mode && !has_selection(i)) {
			// Make sure we don't try and remove empty space.
			int cl = get_caret_line(i);
			int cc = get_caret_column(i);
			if (cc < get_line(cl).length()) {
				_remove_text(cl, cc, cl, cc + 1);
			}
		}

		const char32_t chr[2] = {(char32_t)p_unicode, 0};
		insert_text_at_caret(chr, i);
	}
	end_multicaret_edit();
	end_action();
}

void TextEdit::_backspace_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (!editable) {
		return;
	}

	if (has_selection(p_caret)) {
		delete_selection(p_caret);
		return;
	}

	begin_complex_operation();
	begin_multicaret_edit();
	for (int i = 0; i < get_caret_count(); i++) {
		if (p_caret == -1 && multicaret_edit_ignore_caret(i)) {
			continue;
		}
		if (p_caret != -1 && p_caret != i) {
			continue;
		}

		int to_line = get_caret_line(i);
		int to_column = get_caret_column(i);

		if (to_column == 0 && to_line == 0) {
			continue;
		}

		int from_line = to_column > 0 ? to_line : to_line - 1;
		int from_column = 0;
		if (to_column == 0) {
			from_column = text[to_line - 1].length();
		}
		else if (caret_mid_grapheme_enabled || !backspace_deletes_composite_character_enabled) {
			from_column = to_column - 1;
		}
		else {
			from_column = get_previous_composite_character_column(to_line, to_column);
		}

		merge_gutters(from_line, to_line);

		_remove_text(from_line, from_column, to_line, to_column);
		collapse_carets(from_line, from_column, to_line, to_column);
		_offset_carets_after(to_line, to_column, from_line, from_column);

		set_caret_line(from_line, false, true, -1, i);
		set_caret_column(from_column, i == 0, i);
	}
	end_multicaret_edit();
	end_complex_operation();
}

void TextEdit::_cut_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);

	_copy_internal(p_caret);

	if (!editable) {
		return;
	}

	if (has_selection(p_caret)) {
		delete_selection(p_caret);
		return;
	}

	if (!empty_selection_clipboard_enabled) {
		return;
	}

	// Remove full lines.
	begin_complex_operation();
	begin_multicaret_edit();
	Vector<Point2i> line_ranges;
	if (p_caret == -1) {
		line_ranges = get_line_ranges_from_carets();
	}
	else {
		line_ranges.push_back(Point2i(get_caret_line(p_caret), get_caret_line(p_caret)));
	}
	int line_offset = 0;
	for (Point2i line_range : line_ranges) {
		// Preserve carets on the last line.
		remove_line_at(line_range.y + line_offset);
		if (line_range.x != line_range.y) {
			remove_text(line_range.x + line_offset, 0, line_range.y + line_offset, 0);
		}
		line_offset += line_range.x - line_range.y - 1;
	}
	end_multicaret_edit();
	end_complex_operation();
}

void TextEdit::_copy_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (has_selection(p_caret)) {
		DisplayServer::get_singleton()->clipboard_set(get_selected_text(p_caret));
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
	DisplayServer::get_singleton()->clipboard_set(clipboard_string);
	// Set the cut copy line so we know to paste as a line.
	if (get_caret_count() == 1) {
		cut_copy_line = clipboard_string;
	}
	else {
		cut_copy_line = "";
	}
}

void TextEdit::_paste_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (!editable) {
		return;
	}

	String clipboard = DisplayServer::get_singleton()->clipboard_get();
	if (clipboard.is_empty()) {
		// Nothing to paste.
		return;
	}

	// Paste a full line. Ignore '\r' characters that may have been added to the clipboard by the
	// OS.
	if (get_caret_count() == 1 && !has_selection(0) && !cut_copy_line.is_empty() &&
		cut_copy_line == clipboard.remove_char('\r')) {
		insert_text(clipboard, get_caret_line(), 0);

		_update_scrollbars();
		adjust_viewport_to_caret(0);

		return;
	}

	// Paste text at each caret or one line per caret.
	Vector<String> clipboard_lines = clipboard.split("\n");
	bool insert_line_per_caret =
		p_caret == -1 && get_caret_count() > 1 && clipboard_lines.size() == get_caret_count();

	begin_complex_operation();
	begin_multicaret_edit();
	Vector<int> sorted_carets = get_sorted_carets();
	for (int i = 0; i < sorted_carets.size(); i++) {
		int caret_index = sorted_carets[i];
		if (p_caret != -1 && p_caret != caret_index) {
			continue;
		}

		if (has_selection(caret_index)) {
			delete_selection(caret_index);
		}

		if (insert_line_per_caret) {
			clipboard = clipboard_lines[i];
		}

		insert_text_at_caret(clipboard, caret_index);
	}
	end_multicaret_edit();
	end_complex_operation();
}

void TextEdit::_paste_primary_clipboard_internal(int p_caret)
{
	ERR_FAIL_COND(p_caret >= get_caret_count() || p_caret < -1);
	if (!is_editable() || !DisplayServer::get_singleton()->has_feature(
							  DisplayServerEnums::FEATURE_CLIPBOARD_PRIMARY)) {
		return;
	}

	String paste_buffer = DisplayServer::get_singleton()->clipboard_get_primary();

	if (get_caret_count() == 1) {
		Point2i pos = get_line_column_at_pos(get_local_mouse_pos());
		deselect();
		set_caret_line(pos.y, true, false, -1);
		set_caret_column(pos.x);
	}

	if (!paste_buffer.is_empty()) {
		insert_text_at_caret(paste_buffer);
	}

	grab_focus();
}

Key TextEdit::_get_menu_action_accelerator(const String& p_action)
{
	const List<Ref<InputEvent>>* events = InputMap::get_singleton()->action_get_events(p_action);
	if (!events) {
		return Key::NONE;
	}

	// Use first event in the list for the accelerator.
	const List<Ref<InputEvent>>::Element* first_event = events->front();
	if (!first_event) {
		return Key::NONE;
	}

	const Ref<InputEventKey> event = first_event->get();
	if (event.is_null()) {
		return Key::NONE;
	}

	// Use physical keycode if non-zero.
	if (event->get_physical_keycode() != Key::NONE) {
		return event->get_physical_keycode_with_modifiers();
	}
	else {
		return event->get_keycode_with_modifiers();
	}
}

void TextEdit::_update_context_menu()
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

	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		MENU_ITEM_DISABLED(menu, MENU_EMOJI_AND_SYMBOL, !editable || !emoji_menu_enabled)
	}
	MENU_ITEM_ACTION_DISABLED(menu, MENU_CUT, "ui_cut", !editable)
	MENU_ITEM_ACTION(menu, MENU_COPY, "ui_copy")
	MENU_ITEM_ACTION_DISABLED(menu, MENU_PASTE, "ui_paste", !editable)
	MENU_ITEM_ACTION_DISABLED(menu, MENU_SELECT_ALL, "ui_text_select_all", !selecting_enabled)
	MENU_ITEM_DISABLED(menu, MENU_CLEAR, !editable)
	MENU_ITEM_ACTION_DISABLED(menu, MENU_UNDO, "ui_undo", !editable || !has_undo())
	MENU_ITEM_ACTION_DISABLED(menu, MENU_REDO, "ui_redo", !editable || !has_redo())
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_INHERITED, text_direction == TEXT_DIRECTION_INHERITED)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_AUTO, text_direction == TEXT_DIRECTION_AUTO)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_LTR, text_direction == TEXT_DIRECTION_LTR)
	MENU_ITEM_CHECKED(menu_dir, MENU_DIR_RTL, text_direction == TEXT_DIRECTION_RTL)
	MENU_ITEM_CHECKED(menu, MENU_DISPLAY_UCC, draw_control_chars)
	MENU_ITEM_DISABLED(menu, MENU_SUBMENU_INSERT_UCC, !editable)

#undef MENU_ITEM_ACTION_DISABLED
#undef MENU_ITEM_ACTION
#undef MENU_ITEM_DISABLED
#undef MENU_ITEM_CHECKED
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
	if (!Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
		click_select_held->stop();
		return;
	}
	switch (get_selection_mode()) {
	case SelectionMode::SELECTION_MODE_POINTER: {
		_update_selection_mode_pointer();
	} break;
	case SelectionMode::SELECTION_MODE_WORD: {
		_update_selection_mode_word();
	} break;
	case SelectionMode::SELECTION_MODE_LINE: {
		_update_selection_mode_line();
	} break;
	default: {
		click_select_held->stop();
		break;
	}
	}
}

void TextEdit::_update_selection_mode_pointer(bool p_initial)
{
	Point2i pos = get_line_column_at_pos(get_local_mouse_pos());
	int line = pos.y;
	int column = pos.x;
	int caret_index = get_caret_count() - 1;

	if (p_initial && !has_selection(caret_index)) {
		set_selection_origin_line(line, true, -1, caret_index);
		set_selection_origin_column(column, caret_index);
		// Set the word begin and end to the column in case the mode changes later.
		carets.write[caret_index].selection.word_begin_column = column;
		carets.write[caret_index].selection.word_end_column = column;
	}
	else {
		int origin_line = get_selection_origin_line(caret_index);
		bool is_new_selection_dir_right =
			line > origin_line ||
			(line == origin_line && column >= carets[caret_index].selection.word_begin_column);
		int origin_col = is_new_selection_dir_right
							 ? carets[caret_index].selection.word_begin_column
							 : carets[caret_index].selection.word_end_column;
		select(origin_line, origin_col, line, column, caret_index);
	}
	adjust_viewport_to_caret(caret_index);

	if (has_selection(caret_index)) {
		// Only set to true if any selection has been made.
		dragging_selection = true;
	}

	click_select_held->start();
	merge_overlapping_carets();
}

void TextEdit::_update_selection_mode_word(bool p_initial)
{
	dragging_selection = true;

	Point2i pos = get_line_column_at_pos(get_local_mouse_pos());
	int line = pos.y;
	int column = pos.x;
	int caret_index = get_caret_count() - 1;

	int caret_pos = CLAMP(column, 0, text[line].length());
	int beg = caret_pos;
	int end = beg;
	PackedInt32Array words = TS->shaped_text_get_word_breaks(text.get_line_data(line)->get_rid());
	for (int i = 0; i < words.size(); i = i + 2) {
		if ((p_initial && words[i] <= caret_pos && words[i + 1] >= caret_pos) ||
			(!p_initial && words[i] < caret_pos && words[i + 1] > caret_pos)) {
			beg = words[i];
			end = words[i + 1];
			break;
		}
	}

	if (p_initial && !has_selection(caret_index)) {
		// Set the selection origin if there is no existing selection.
		select(line, beg, line, end, caret_index);
		carets.write[caret_index].selection.word_begin_column = beg;
		carets.write[caret_index].selection.word_end_column = end;
	}
	else {
		// Expand the word selection to the mouse.
		int origin_line = get_selection_origin_line(caret_index);
		bool is_new_selection_dir_right =
			line > origin_line ||
			(line == origin_line && column >= carets[caret_index].selection.word_begin_column);
		int origin_col = is_new_selection_dir_right
							 ? carets[caret_index].selection.word_begin_column
							 : carets[caret_index].selection.word_end_column;
		int caret_col = is_new_selection_dir_right ? end : beg;

		// Expand the word selection only if the caret is not at the start of the selection.
		if (column != carets[caret_index].selection.word_begin_column || line != origin_line ||
			carets[caret_index].selection.word_begin_column ==
				carets[caret_index].selection.word_end_column) {
			select(origin_line, origin_col, line, caret_col, caret_index);
		}
	}
	adjust_viewport_to_caret(caret_index);

	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_CLIPBOARD_PRIMARY)) {
		DisplayServer::get_singleton()->clipboard_set_primary(get_selected_text());
	}

	click_select_held->start();
	merge_overlapping_carets();
}

void TextEdit::_update_selection_mode_line(bool p_initial)
{
	dragging_selection = true;

	Point2i pos = get_line_column_at_pos(get_local_mouse_pos());
	int line = pos.y;
	int caret_index = get_caret_count() - 1;

	int origin_line =
		p_initial && !has_selection(caret_index) ? line : get_selection_origin_line(caret_index);
	bool line_below = line >= origin_line;
	int origin_col = line_below ? 0 : get_line(origin_line).length();
	int caret_line = line_below ? line + 1 : line;
	int caret_col = caret_line < text.size() ? 0 : get_line(text.size() - 1).length();

	select(origin_line, origin_col, caret_line, caret_col, caret_index);
	adjust_viewport_to_caret(caret_index);

	if (p_initial) {
		// Set the word begin and end to the start and end of the origin line in case the mode
		// changes later.
		carets.write[caret_index].selection.word_begin_column = 0;
		carets.write[caret_index].selection.word_end_column = get_line(origin_line).length();
	}

	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_CLIPBOARD_PRIMARY)) {
		DisplayServer::get_singleton()->clipboard_set_primary(get_selected_text());
	}

	click_select_held->start();
	merge_overlapping_carets();
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

void TextEdit::_insert_text(
	int p_line, int p_char, const String& p_text, int* r_end_line, int* r_end_char)
{
	if (!setting_text && idle_detect->is_inside_tree()) {
		idle_detect->start();
	}

	if (undo_enabled) {
		_clear_redo();
	}

	int retline, retchar;
	_base_insert_text(p_line, p_char, p_text, retline, retchar);
	if (r_end_line) {
		*r_end_line = retline;
	}
	if (r_end_char) {
		*r_end_char = retchar;
	}

	if (!undo_enabled) {
		return;
	}

	/* UNDO!! */
	TextOperation op;
	op.type = TextOperation::TYPE_INSERT;
	op.from_line = p_line;
	op.from_column = p_char;
	op.to_line = retline;
	op.to_column = retchar;
	op.text = p_text;
	op.version = ++version;
	op.chain_forward = false;
	op.chain_backward = false;
	if (next_operation_is_complex) {
		op.start_carets = current_op.start_carets;
	}
	else {
		op.start_carets = carets;
	}
	op.end_carets = carets;

	op.prev_version = get_version();
	_push_current_op();
	current_op = op;
}

void TextEdit::_remove_text(int p_from_line, int p_from_column, int p_to_line, int p_to_column)
{
	if (!setting_text && idle_detect->is_inside_tree()) {
		idle_detect->start();
	}

	String txt;
	if (undo_enabled) {
		_clear_redo();
		txt = _base_get_text(p_from_line, p_from_column, p_to_line, p_to_column);
	}

	_base_remove_text(p_from_line, p_from_column, p_to_line, p_to_column);

	if (!undo_enabled) {
		return;
	}

	/* UNDO! */
	TextOperation op;
	op.type = TextOperation::TYPE_REMOVE;
	op.from_line = p_from_line;
	op.from_column = p_from_column;
	op.to_line = p_to_line;
	op.to_column = p_to_column;
	op.text = txt;
	op.version = ++version;
	op.chain_forward = false;
	op.chain_backward = false;
	if (next_operation_is_complex) {
		op.start_carets = current_op.start_carets;
	}
	else {
		op.start_carets = carets;
	}
	op.end_carets = carets;

	op.prev_version = get_version();
	_push_current_op();
	current_op = op;
}

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
		RS::get_singleton()->canvas_item_add_rect(
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

		RS::get_singleton()->canvas_item_add_polyline(
			p_canvas_item, points, colors, p_width, p_antialiased);
	}
}

TextEdit::~TextEdit() { RS::get_singleton()->free_rid(text_ci); }


