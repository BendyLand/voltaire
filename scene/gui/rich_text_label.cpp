/**************************************************************************/
/*  rich_text_label.cpp                                                   */
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

#include "core/input/input_map.h"
#include "core/io/resource_loader.h"
#include "core/math/math_defs.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "modules/modules_enabled.gen.h" // For regex.
#include "rich_text_label.compat.inc"
#include "rich_text_label.h"
#include "scene/gui/label.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/rich_text_effect.h"
#include "scene/gui/scroll_bar.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/texture.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"
#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif

RichTextLabel::ItemCustomFX::ItemCustomFX()
{
	type = ITEM_CUSTOMFX;
	char_fx_transform.instantiate();
}

RichTextLabel::ItemCustomFX::~ItemCustomFX()
{
	_clear_children();

	char_fx_transform.unref();
	custom_effect.unref();
}

Rect2i _merge_or_copy_rect(const Rect2i& p_a, const Rect2i& p_b)
{
	if (!p_a.has_area()) {
		return p_b;
	}
	else {
		return p_a.merge(p_b);
	}
}

RichTextLabel::Line::~Line()
{
	if (accessibility_line_element.is_valid()) {
		AccessibilityServer::get_singleton()->free_element(accessibility_line_element);
		accessibility_line_element = RID();
		accessibility_text_element = RID();
	}
}

RichTextLabel::Item* RichTextLabel::_get_next_item(Item* p_item, bool p_free) const
{
	if (!p_item) {
		return nullptr;
	}
	if (p_free) {
		if (p_item->subitems.size()) {
			return p_item->subitems.front()->get();
		}
		else if (!p_item->parent) {
			return nullptr;
		}
		else if (p_item->E->next()) {
			return p_item->E->next()->get();
		}
		else {
			// Go up until something with a next is found.
			while (p_item->parent && !p_item->E->next()) {
				p_item = p_item->parent;
			}

			if (p_item->parent) {
				return p_item->E->next()->get();
			}
			else {
				return nullptr;
			}
		}

	}
	else {
		if (p_item->subitems.size() && p_item->type != ITEM_TABLE) {
			return p_item->subitems.front()->get();
		}
		else if (p_item->type == ITEM_FRAME) {
			return nullptr;
		}
		else if (p_item->E->next()) {
			return p_item->E->next()->get();
		}
		else {
			// Go up until something with a next is found.
			while (p_item->parent && p_item->type != ITEM_FRAME && !p_item->E->next()) {
				p_item = p_item->parent;
			}

			if (p_item->type != ITEM_FRAME) {
				return p_item->E->next()->get();
			}
			else {
				return nullptr;
			}
		}
	}
}

RichTextLabel::Item* RichTextLabel::_get_prev_item(Item* p_item, bool p_free) const
{
	if (!p_item) {
		return nullptr;
	}
	if (p_free) {
		if (!p_item->parent) {
			return nullptr;
		}
		else if (p_item->E->prev()) {
			p_item = p_item->E->prev()->get();
			while (p_item->subitems.size()) {
				p_item = p_item->subitems.back()->get();
			}
			return p_item;
		}
		else {
			if (p_item->parent) {
				return p_item->parent;
			}
			else {
				return nullptr;
			}
		}

	}
	else {
		if (p_item->type == ITEM_FRAME) {
			return nullptr;
		}
		else if (p_item->E->prev()) {
			p_item = p_item->E->prev()->get();
			while (p_item->subitems.size() && p_item->type != ITEM_TABLE) {
				p_item = p_item->subitems.back()->get();
			}
			return p_item;
		}
		else {
			if (p_item->parent && p_item->type != ITEM_FRAME) {
				return p_item->parent;
			}
			else {
				return nullptr;
			}
		}
	}
}

Rect2 RichTextLabel::_get_text_rect()
{
	return Rect2(theme_cache.normal_style->get_offset(),
		get_size() - theme_cache.normal_style->get_minimum_size());
}

int RichTextLabel::_get_wrap_width(const Rect2& p_text_rect) const
{
	int wrap_width = p_text_rect.get_size().width;
	float combined_maximum_width = get_combined_maximum_size().x;
	if (autowrap_mode != TextServer::AUTOWRAP_OFF && combined_maximum_width > 0.0) {
		int maximum_width =
			int(combined_maximum_width - theme_cache.normal_style->get_minimum_size().width);
		if (maximum_width <= 0) {
			maximum_width = 1;
		}
		wrap_width = MIN(wrap_width, maximum_width);
		wrap_width = MAX(wrap_width, 1);
	}
	return wrap_width;
}

RichTextLabel::Item* RichTextLabel::_get_item_at_pos(
	RichTextLabel::Item* p_item_from, RichTextLabel::Item* p_item_to, int p_position)
{
	int offset = 0;
	for (Item* it = p_item_from; it && it != p_item_to; it = _get_next_item(it)) {
		switch (it->type) {
		case ITEM_TEXT: {
			ItemText* t = static_cast<ItemText*>(it);
			offset += t->text.length();
			if (offset > p_position) {
				return it;
			}
		} break;
		case ITEM_NEWLINE: {
			offset += 1;
			if (offset == p_position) {
				return it;
			}
		} break;
		case ITEM_IMAGE: {
			offset += 1;
			if (offset > p_position) {
				return it;
			}
		} break;
		case ITEM_TABLE: {
			ItemTable* table = static_cast<ItemTable*>(it);
			offset += table->char_count;
		} break;
		default:
			break;
		}
	}
	return p_item_from;
}

String RichTextLabel::_roman(int p_num, bool p_capitalize) const
{
	if (p_num > 3999) {
		return "ERR";
	};
	String s;
	if (p_capitalize) {
		const String roman_M[] = {"", "M", "MM", "MMM"};
		const String roman_C[] = {"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM"};
		const String roman_X[] = {"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC"};
		const String roman_I[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
		s = roman_M[p_num / 1000] + roman_C[(p_num % 1000) / 100] + roman_X[(p_num % 100) / 10] +
			roman_I[p_num % 10];
	}
	else {
		const String roman_M[] = {"", "m", "mm", "mmm"};
		const String roman_C[] = {"", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};
		const String roman_X[] = {"", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
		const String roman_I[] = {"", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
		s = roman_M[p_num / 1000] + roman_C[(p_num % 1000) / 100] + roman_X[(p_num % 100) / 10] +
			roman_I[p_num % 10];
	}
	return s;
}

String RichTextLabel::_letters(int p_num, bool p_capitalize) const
{
	uint64_t n = p_num;

	int chars = 0;
	do {
		n = (n - 1) / 26;
		chars++;
	} while (n);

	String s;
	s.resize_uninitialized(chars + 1);
	char32_t* c = s.ptrw();
	c[chars] = 0;
	n = p_num;
	do {
		const char a = (p_capitalize ? 'A' : 'a');
		n = n - 1;
		c[--chars] = a + n % 26;
		n /= 26;
	} while (n);

	return s;
}

String RichTextLabel::_get_prefix(
	Item* p_item, const Vector<int>& p_list_index, const Vector<ItemList*>& p_list_items)
{
	String prefix;
	int segments = 0;
	for (int i = 0; i < p_list_index.size(); i++) {
		String segment;
		if (p_list_items[i]->list_type == LIST_DOTS) {
			if (segments == 0) {
				prefix = p_list_items[i]->bullet;
			}
			break;
		}
		prefix = "." + prefix;
		if (p_list_items[i]->list_type == LIST_NUMBERS) {
			segment = itos(p_list_index[i]);
			if (is_localizing_numeral_system()) {
				segment = TranslationServer::get_singleton()->format_number(
					segment, _find_language(p_item));
			}
			segments++;
		}
		else if (p_list_items[i]->list_type == LIST_LETTERS) {
			segment = _letters(p_list_index[i], p_list_items[i]->capitalize);
			segments++;
		}
		else if (p_list_items[i]->list_type == LIST_ROMAN) {
			segment = _roman(p_list_index[i], p_list_items[i]->capitalize);
			segments++;
		}
		prefix = segment + prefix;
	}
	return prefix + " ";
}

int RichTextLabel::_get_line_max_width(ItemFrame* p_frame, int p_line) const
{
	ERR_FAIL_NULL_V(p_frame, 0);
	ERR_FAIL_INDEX_V(p_line, (int)p_frame->lines.size(), 0);

	Line& l = p_frame->lines[p_line];
	MutexLock lock(l.text_buf->get_mutex());

	int max_width = Math::ceil(l.text_buf->get_non_wrapped_size().x);

	Item* it_to =
		(p_line + 1 < (int)p_frame->lines.size()) ? p_frame->lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; it && it != it_to; it = _get_next_item(it)) {
		if (it->type != ITEM_TABLE) {
			continue;
		}

		ItemTable* table = static_cast<ItemTable*>(it);

		// Subtract the table's width recorded in text_buf.
		max_width -= table->total_width;

		// Recalculate the maximum width of the table.
		const int col_count = table->columns.size();
		max_width += theme_cache.table_h_separation * col_count;

		// The columns in the nested table have already been calculated in _shape_line().
		for (ItemTable::Column& C : table->columns) {
			max_width += C.max_width;
		}
	}

	max_width += Math::ceil(p_frame->padding.position.x + p_frame->padding.size.x + l.indent);

	return max_width;
}

Size2 RichTextLabel::_get_item_image_final_size(
	ItemImage* p_img, float p_orig_width, float p_base_font_size)
{
	Size2 new_size(p_img->rq_size);
	ItemFontSize* font_size_it = _find_font_size(p_img);

	if (p_img->width_unit == IMAGE_UNIT_PERCENT) {
		new_size.width = p_orig_width * p_img->rq_size.width / 100.f;
	}
	else if (p_img->width_unit == IMAGE_UNIT_EM) {
		new_size.width =
			(font_size_it ? font_size_it->font_size : p_base_font_size) * p_img->rq_size.width;
	}

	if (p_img->height_unit == IMAGE_UNIT_PERCENT) {
		new_size.height = p_orig_width * p_img->rq_size.height / 100.f;
	}
	else if (p_img->height_unit == IMAGE_UNIT_EM) {
		new_size.height =
			(font_size_it ? font_size_it->font_size : p_base_font_size) * p_img->rq_size.height;
	}
	return new_size;
}

void RichTextLabel::_update_table_column_width(ItemTable* p_table, int p_available_width)
{
	const int col_count = p_table->columns.size();

	LocalVector<bool> columns_will_stretch;
	columns_will_stretch.resize(col_count);

	int total_ratio = 0; // The total ratio of the columns that will stretch.
	p_table->total_width = 0;
	float remaining_width = 0; // The available width of the columns that will stretch.

	for (int i = 0; i < col_count; i++) {
		ItemTable::Column& C = p_table->columns[i];
		p_table->total_width += C.min_width;
		C.width = C.min_width;
		if (C.max_width > C.min_width) {
			C.expand = true; // Like a hack.
		}
		columns_will_stretch[i] = C.expand;
		if (columns_will_stretch[i]) {
			remaining_width += C.min_width;
			total_ratio += C.expand_ratio;
		}
	}

	int diff = p_available_width - p_table->total_width;
	if (diff < 0) {
		diff = 0; // Avoid negative stretch space.
	}
	remaining_width += diff;

	// Resize to max_width if needed and distribute the remaining space.
	bool table_need_fit = true;
	while (table_need_fit) {
		table_need_fit = false;
		float error = 0.0; // Keep track of accumulated error in pixels.

		for (int i = 0; i < col_count; i++) {
			if (!columns_will_stretch[i]) {
				continue;
			}
			ItemTable::Column& C = p_table->columns[i];

			float final_pixel_size = remaining_width * C.expand_ratio / total_ratio;
			error += Math::fract(final_pixel_size);

			if (C.shrink && final_pixel_size > C.max_width) {
				columns_will_stretch[i] = false;
				total_ratio -= C.expand_ratio;
				table_need_fit = true;
				remaining_width -= C.max_width;
				C.width = C.max_width;
				break;
			}

			if (final_pixel_size < C.min_width) {
				// If available stretching area is too small for widget,
				// then remove it from stretching area.
				columns_will_stretch[i] = false;
				total_ratio -= C.expand_ratio;
				table_need_fit = true;
				remaining_width -= C.min_width;
				C.width = C.min_width;
				break;
			}

			C.width = final_pixel_size;
			// Dump accumulated error if one pixel or more.
			if (error >= 1) {
				C.width += 1;
				error -= 1;
			}
		}
	}

	// Recalculate total width.
	p_table->total_width = theme_cache.table_h_separation * col_count;
	for (ItemTable::Column& C : p_table->columns) {
		p_table->total_width += C.width;
	}
}

void RichTextLabel::_update_table_size(ItemTable* p_table)
{
	const int col_count = p_table->columns.size();

	// Update line width and get total height.
	int idx = 0;
	p_table->total_height = 0;
	p_table->rows.clear();
	p_table->rows_baseline.clear();

	Vector2 offset =
		Vector2(theme_cache.table_h_separation * 0.5, theme_cache.table_v_separation * 0.5).floor();
	float row_height = 0.0;
	const List<Item*>::Element* prev = p_table->subitems.front();

	for (const List<Item*>::Element* E = prev; E; E = E->next()) {
		ERR_CONTINUE(E->get()->type != ITEM_FRAME); // Children should all be frames.
		ItemFrame* frame = static_cast<ItemFrame*>(E->get());

		int column = idx % col_count;
		const real_t frame_padding_space = frame->padding.position.x + frame->padding.size.x;

		offset += frame->padding.position;
		float yofs = 0.0;
		float prev_h = 0.0;
		float row_baseline = 0.0;
		for (int i = 0; i < (int)frame->lines.size(); i++) {
			Line& line = frame->lines[i];
			MutexLock sub_lock(line.text_buf->get_mutex());
			line.text_buf->set_width(
				p_table->columns[column].width - Math::ceil(frame_padding_space + line.indent));
			line.offset.y = prev_h;

			float h = line.text_buf->get_size().y +
					  (line.text_buf->get_line_count() - 1) * theme_cache.line_separation;
			if (i > 0) {
				h += theme_cache.paragraph_separation + theme_cache.line_separation;
			}
			if (frame->min_size_over.y > 0) {
				h = MAX(h, frame->min_size_over.y);
			}
			if (frame->max_size_over.y > 0) {
				h = MIN(h, frame->max_size_over.y);
			}
			yofs += h;
			prev_h = line.offset.y + line.text_buf->get_size().y +
					 line.text_buf->get_line_count() * theme_cache.line_separation +
					 theme_cache.paragraph_separation;

			line.offset += offset;
			row_baseline = MAX(
				row_baseline, line.text_buf->get_line_ascent(line.text_buf->get_line_count() - 1));
		}

		offset -= frame->padding.position;
		offset.x += p_table->columns[column].width + theme_cache.table_h_separation;

		row_height = MAX(yofs + frame->padding.position.y + frame->padding.size.y, row_height);
		// Add row height after last column of the row or last cell of the table.
		if (column == col_count - 1 || E->next() == nullptr) {
			offset.x = Math::floor(theme_cache.table_h_separation * 0.5);
			row_height += theme_cache.table_v_separation;
			p_table->rows.push_back(row_height);
			p_table->rows_baseline.push_back(p_table->total_height + row_baseline +
											 Math::floor(theme_cache.table_v_separation * 0.5));
			p_table->total_height += row_height;
			offset.y += row_height;
			row_height = 0.0;
			prev = E->next();
		}
		idx++;
	}
}

void RichTextLabel::_find_click(ItemFrame* p_frame, const Point2i& p_click,
	ItemFrame** r_click_frame, int* r_click_line, Item** r_click_item, int* r_click_char,
	bool* r_outside, bool p_meta)
{
	if (r_click_item) {
		*r_click_item = nullptr;
	}
	if (r_click_char != nullptr) {
		*r_click_char = 0;
	}
	if (r_outside != nullptr) {
		*r_outside = true;
	}

	Size2 size = get_size();
	Rect2 text_rect = _get_text_rect();

	int vofs = vscroll->get_value();

	// Search for the first line.
	int to_line = main->first_invalid_line.load();
	int from_line = _find_first_line(0, to_line, vofs);

	int total_height = INT32_MAX;
	if (to_line && vertical_alignment != VERTICAL_ALIGNMENT_TOP) {
		MutexLock lock(main->lines[to_line - 1].text_buf->get_mutex());
		if (theme_cache.line_separation < 0) {
			// Do not apply to the last line to avoid cutting text.
			total_height = main->lines[to_line - 1].offset.y +
						   main->lines[to_line - 1].text_buf->get_size().y +
						   (main->lines[to_line - 1].text_buf->get_line_count() - 1) *
							   theme_cache.line_separation;
		}
		else {
			total_height =
				main->lines[to_line - 1].offset.y +
				main->lines[to_line - 1].text_buf->get_size().y +
				main->lines[to_line - 1].text_buf->get_line_count() * theme_cache.line_separation +
				theme_cache.paragraph_separation;
		}
	}
	float vbegin = 0, vsep = 0;
	if (text_rect.size.y > total_height) {
		switch (vertical_alignment) {
		case VERTICAL_ALIGNMENT_TOP: {
			// Nothing.
		} break;
		case VERTICAL_ALIGNMENT_CENTER: {
			vbegin = (text_rect.size.y - total_height) / 2;
		} break;
		case VERTICAL_ALIGNMENT_BOTTOM: {
			vbegin = text_rect.size.y - total_height;
		} break;
		case VERTICAL_ALIGNMENT_FILL: {
			int lines = 0;
			for (int l = from_line; l < to_line; l++) {
				MutexLock lock(main->lines[l].text_buf->get_mutex());
				lines += main->lines[l].text_buf->get_line_count();
			}
			if (lines > 1) {
				vsep = (text_rect.size.y - total_height) / (lines - 1);
			}
		} break;
		}
	}

	Point2 ofs =
		text_rect.get_position() + Vector2(0, vbegin + main->lines[from_line].offset.y - vofs);
	while (ofs.y < size.height && from_line < to_line) {
		MutexLock lock(main->lines[from_line].text_buf->get_mutex());
		_find_click_in_line(p_frame, from_line, ofs, text_rect.size.x, vsep, p_click, r_click_frame,
			r_click_line, r_click_item, r_click_char, false, p_meta);
		ofs.y += main->lines[from_line].text_buf->get_size().y +
				 main->lines[from_line].text_buf->get_line_count() *
					 (theme_cache.line_separation + vsep) +
				 (theme_cache.paragraph_separation);
		if (((r_click_item != nullptr) && ((*r_click_item) != nullptr)) ||
			((r_click_frame != nullptr) && ((*r_click_frame) != nullptr))) {
			if (r_outside != nullptr) {
				*r_outside = false;
			}
			return;
		}
		from_line++;
	}
}

void RichTextLabel::_scroll_changed(double)
{
	if (updating_scroll) {
		return;
	}

	if (scroll_follow && vscroll->get_value() > (vscroll->get_max() - vscroll->get_page() - 1)) {
		scroll_following = true;
	}
	else {
		scroll_following = false;
	}

	scroll_updated = true;

	queue_redraw();
}

void RichTextLabel::_update_fx(RichTextLabel::ItemFrame* p_frame, double p_delta_time)
{
	Item* it = p_frame;
	while (it) {
		ItemFX* ifx = nullptr;

		if (it->type == ITEM_CUSTOMFX || it->type == ITEM_SHAKE || it->type == ITEM_WAVE ||
			it->type == ITEM_TORNADO || it->type == ITEM_RAINBOW || it->type == ITEM_PULSE) {
			ifx = static_cast<ItemFX*>(it);
		}

		if (!ifx) {
			it = _get_next_item(it, true);
			continue;
		}

		ifx->elapsed_time += p_delta_time;

		ItemShake* shake = nullptr;

		if (it->type == ITEM_SHAKE) {
			shake = static_cast<ItemShake*>(it);
		}

		if (shake) {
			bool cycle = (shake->elapsed_time > (1.0f / shake->rate));
			if (cycle) {
				shake->elapsed_time -= (1.0f / shake->rate);
				shake->reroll_random();
			}
		}

		it = _get_next_item(it, true);
	}
}

int RichTextLabel::_find_first_line(int p_from, int p_to, int p_vofs) const
{
	int l = p_from;
	int r = p_to;
	while (l < r) {
		int m = Math::floor(double(l + r) / 2.0);
		MutexLock lock(main->lines[m].text_buf->get_mutex());
		int ofs = _calculate_line_vertical_offset(main->lines[m]);
		if (ofs < p_vofs) {
			l = m + 1;
		}
		else {
			r = m;
		}
	}
	return MIN(l, (int)main->lines.size() - 1);
}

_FORCE_INLINE_ float RichTextLabel::_calculate_line_vertical_offset(
	const RichTextLabel::Line& line) const
{
	return line.get_height(theme_cache.line_separation, theme_cache.paragraph_separation);
}

void RichTextLabel::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
	use_selected_font_color = theme_cache.font_selected_color != Color(0, 0, 0, 0);
}

PackedStringArray RichTextLabel::get_accessibility_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_accessibility_configuration_warnings();

	Item* it = main;
	while (it) {
		if (it->type == ITEM_IMAGE) {
			ItemImage* img = static_cast<ItemImage*>(it);
			if (img && img->alt_text.strip_edges().is_empty()) {
				warnings.push_back(RTR("Image alternative text must not be empty."));
			}
		}
		it = _get_next_item(it, true);
	}

	return warnings;
}

void RichTextLabel::_invalidate_accessibility()
{
	if (accessibility_scroll_element.is_null()) {
		return;
	}

	Item* it = main;
	while (it) {
		if (it->type == ITEM_FRAME) {
			ItemFrame* fr = static_cast<ItemFrame*>(it);
			for (size_t i = 0; i < fr->lines.size(); i++) {
				if (fr->lines[i].accessibility_line_element.is_valid()) {
					AccessibilityServer::get_singleton()->free_element(
						fr->lines[i].accessibility_line_element);
				}
				fr->lines[i].accessibility_line_element = RID();
				fr->lines[i].accessibility_text_element = RID();
			}
		}
		it->accessibility_item_element = RID();
		it = _get_next_item(it, true);
	}
}

RID RichTextLabel::get_focused_accessibility_element() const
{
	if (keyboard_focus_frame && keyboard_focus_item) {
		if (keyboard_focus_on_text) {
			return keyboard_focus_frame->lines[keyboard_focus_line].accessibility_text_element;
		}
		else {
			if (keyboard_focus_item->accessibility_item_element.is_valid()) {
				return keyboard_focus_item->accessibility_item_element;
			}
		}
	}
	else {
		if (!main->lines.is_empty()) {
			return main->lines[0].accessibility_text_element;
		}
	}
	return get_accessibility_element();
}

void RichTextLabel::_prepare_scroll_anchor()
{
	scroll_w = vscroll->get_bound_minimum_size().width;
	vscroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -scroll_w);
}

void RichTextLabel::_update_selection()
{
	ItemFrame* c_frame = nullptr;
	int c_line = 0;
	Item* c_item = nullptr;
	int c_index = 0;
	bool outside;

	// Handle auto scrolling.
	const Size2 size = get_size();
	if (!(local_mouse_pos.x >= 0.0 && local_mouse_pos.y >= 0.0 && local_mouse_pos.x < size.x &&
			local_mouse_pos.y < size.y)) {
		real_t scroll_delta = 0.0;
		if (local_mouse_pos.y < 0) {
			scroll_delta = -auto_scroll_speed * (1 - (local_mouse_pos.y / 15.0));
		}
		else if (local_mouse_pos.y > size.y) {
			scroll_delta = auto_scroll_speed * (1 + (local_mouse_pos.y - size.y) / 15.0);
		}

		if (scroll_delta != 0.0) {
			vscroll->scroll(scroll_delta);
			queue_redraw();
		}
	}

	// Update selection area.
	_find_click(
		main, last_clamped_mouse_pos, &c_frame, &c_line, &c_item, &c_index, &outside, false);
	if (selection.click_item && c_item) {
		selection.from_frame = selection.click_frame;
		selection.from_line = selection.click_line;
		selection.from_item = selection.click_item;
		selection.from_char = selection.click_char;

		selection.to_frame = c_frame;
		selection.to_line = c_line;
		selection.to_item = c_item;
		selection.to_char = c_index;

		bool swap = false;
		if (selection.click_frame && c_frame) {
			const Line& l1 = c_frame->lines[c_line];
			const Line& l2 = selection.click_frame->lines[selection.click_line];
			if (l1.char_offset + c_index < l2.char_offset + selection.click_char) {
				swap = true;
			}
			else if (l1.char_offset + c_index == l2.char_offset + selection.click_char &&
					   selection.selection_mode == Selection::SINGLE_CLICK) {
				deselect();
				return;
			}
		}

		if (swap) {
			SWAP(selection.from_frame, selection.to_frame);
			SWAP(selection.from_line, selection.to_line);
			SWAP(selection.from_item, selection.to_item);
			SWAP(selection.from_char, selection.to_char);
		}

		if (selection.selection_mode == Selection::TRIPLE_CLICK && c_frame) {
			// Expand the selection to paragraph edges.
			selection.from_char = 0;
			selection.to_char = selection.to_frame->lines[selection.to_line].char_count;
		}
		else if (selection.selection_mode == Selection::DOUBLE_CLICK && c_frame) {
			// Expand the selection to word edges.

			Line* l = &selection.from_frame->lines[selection.from_line];
			MutexLock lock(l->text_buf->get_mutex());
			PackedInt32Array words = TS->shaped_text_get_word_breaks(l->text_buf->get_rid());
			for (int i = 0; i < words.size(); i = i + 2) {
				if (selection.from_char > words[i] && selection.from_char < words[i + 1]) {
					selection.from_char = words[i];
					break;
				}
			}
			l = &selection.to_frame->lines[selection.to_line];
			lock = MutexLock(l->text_buf->get_mutex());
			words = TS->shaped_text_get_word_breaks(l->text_buf->get_rid());
			for (int i = 0; i < words.size(); i = i + 2) {
				if (selection.to_char > words[i] && selection.to_char < words[i + 1]) {
					selection.to_char = words[i + 1];
					break;
				}
			}
		}

		selection.active = true;
		queue_accessibility_update();
		queue_redraw();
	}
}

void RichTextLabel::_find_frame(Item* p_item, ItemFrame** r_frame, int* r_line)
{
	if (r_frame != nullptr) {
		*r_frame = nullptr;
	}
	if (r_line != nullptr) {
		*r_line = 0;
	}

	Item* item = p_item;

	while (item) {
		if (item->parent != nullptr && item->parent->type == ITEM_FRAME) {
			if (r_frame != nullptr) {
				*r_frame = static_cast<ItemFrame*>(item->parent);
			}
			if (r_line != nullptr) {
				*r_line = item->line;
			}
			return;
		}

		item = item->parent;
	}
}

RichTextLabel::Item* RichTextLabel::_find_indentable(Item* p_item)
{
	Item* indentable = p_item;

	while (indentable) {
		if (indentable->type == ITEM_INDENT || indentable->type == ITEM_LIST) {
			return indentable;
		}
		indentable = indentable->parent;
	}

	return indentable;
}

RichTextLabel::ItemFont* RichTextLabel::_find_font(Item* p_item)
{
	Item* fontitem = p_item;

	while (fontitem) {
		if (fontitem->type == ITEM_FONT) {
			ItemFont* fi = static_cast<ItemFont*>(fontitem);
			switch (fi->def_font) {
			case RTL_NORMAL_FONT: {
				if (fi->variation) {
					Ref<FontVariation> fc = fi->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.normal_font);
					}
				}
				else {
					fi->font = theme_cache.normal_font;
				}
				if (fi->def_size) {
					fi->font_size = theme_cache.normal_font_size;
				}
			} break;
			case RTL_BOLD_FONT: {
				if (fi->variation) {
					Ref<FontVariation> fc = fi->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.bold_font);
					}
				}
				else {
					fi->font = theme_cache.bold_font;
				}
				if (fi->def_size) {
					fi->font_size = theme_cache.bold_font_size;
				}
			} break;
			case RTL_ITALICS_FONT: {
				if (fi->variation) {
					Ref<FontVariation> fc = fi->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.italics_font);
					}
				}
				else {
					fi->font = theme_cache.italics_font;
				}
				if (fi->def_size) {
					fi->font_size = theme_cache.italics_font_size;
				}
			} break;
			case RTL_BOLD_ITALICS_FONT: {
				if (fi->variation) {
					Ref<FontVariation> fc = fi->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.bold_italics_font);
					}
				}
				else {
					fi->font = theme_cache.bold_italics_font;
				}
				if (fi->def_size) {
					fi->font_size = theme_cache.bold_italics_font_size;
				}
			} break;
			case RTL_MONO_FONT: {
				if (fi->variation) {
					Ref<FontVariation> fc = fi->font;
					if (fc.is_valid()) {
						fc->set_base_font(theme_cache.mono_font);
					}
				}
				else {
					fi->font = theme_cache.mono_font;
				}
				if (fi->def_size) {
					fi->font_size = theme_cache.mono_font_size;
				}
			} break;
			default: {
			} break;
			}
			return fi;
		}

		fontitem = fontitem->parent;
	}

	return nullptr;
}

RichTextLabel::ItemFontSize* RichTextLabel::_find_font_size(Item* p_item)
{
	Item* sizeitem = p_item;

	while (sizeitem) {
		if (sizeitem->type == ITEM_FONT_SIZE) {
			ItemFontSize* fi = static_cast<ItemFontSize*>(sizeitem);
			return fi;
		}

		sizeitem = sizeitem->parent;
	}

	return nullptr;
}

int RichTextLabel::_find_outline_size(Item* p_item, int p_default)
{
	Item* sizeitem = p_item;

	while (sizeitem) {
		if (sizeitem->type == ITEM_OUTLINE_SIZE) {
			ItemOutlineSize* fi = static_cast<ItemOutlineSize*>(sizeitem);
			return fi->outline_size;
		}

		sizeitem = sizeitem->parent;
	}

	return p_default;
}

RichTextLabel::ItemDropcap* RichTextLabel::_find_dc_item(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_DROPCAP) {
			return static_cast<ItemDropcap*>(item);
		}
		item = item->parent;
	}

	return nullptr;
}

RichTextLabel::ItemList* RichTextLabel::_find_list_item(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_LIST) {
			return static_cast<ItemList*>(item);
		}
		item = item->parent;
	}

	return nullptr;
}

int RichTextLabel::_find_list(
	Item* p_item, Vector<int>& r_index, Vector<int>& r_count, Vector<ItemList*>& r_list)
{
	Item* item = p_item;
	Item* prev_item = p_item;

	int level = 0;

	while (item) {
		if (item->type == ITEM_LIST) {
			ItemList* list = static_cast<ItemList*>(item);

			ItemFrame* frame = nullptr;
			int line = -1;
			_find_frame(list, &frame, &line);

			int index = 1;
			int count = 1;
			if (frame != nullptr) {
				for (int i = list->line + 1; i < (int)frame->lines.size(); i++) {
					if (_find_list_item(frame->lines[i].from) == list) {
						if (i <= prev_item->line) {
							index++;
						}
						count++;
					}
				}
			}

			r_index.push_back(index);
			r_count.push_back(count);
			r_list.push_back(list);

			prev_item = item;
		}
		level++;
		item = item->parent;
	}

	return level;
}

int RichTextLabel::_find_margin(Item* p_item, const Ref<Font>& p_base_font, int p_base_font_size)
{
	Item* item = p_item;
	while (item && item->subitems.size()) {
		Item* si = item->subitems.front()->get();
		if (si && (si->type == ITEM_INDENT || si->type == ITEM_LIST)) {
			item = si;
		}
		else {
			break;
		}
	}

	float margin = 0.0;

	while (item) {
		if (item->type == ITEM_FRAME) {
			break;
		}

		if (item->type == ITEM_INDENT) {
			int lvl = 1;
			ItemIndent* ind = static_cast<ItemIndent*>(item);
			if (ind) {
				lvl = ind->level;
			}
			Ref<Font> font = p_base_font;
			int font_size = p_base_font_size;

			ItemFont* font_it = _find_font(item);
			if (font_it) {
				if (font_it->font.is_valid()) {
					font = font_it->font;
				}
				if (font_it->font_size > 0) {
					font_size = font_it->font_size;
				}
			}
			ItemFontSize* font_size_it = _find_font_size(item);
			if (font_size_it && font_size_it->font_size > 0) {
				font_size = font_size_it->font_size;
			}
			if (tab_size > 0) {
				margin += MAX(1, lvl * tab_size *
									 (font->get_char_size(' ', font_size).width +
										 font->get_spacing(TextServer::SPACING_SPACE)));
			}
		}
		else if (item->type == ITEM_LIST) {
			int lvl = 1;
			ItemList* lst = static_cast<ItemList*>(item);
			if (lst) {
				lvl = lst->level;
			}
			Ref<Font> font = p_base_font;
			int font_size = p_base_font_size;

			ItemFont* font_it = _find_font(item);
			if (font_it) {
				if (font_it->font.is_valid()) {
					font = font_it->font;
				}
				if (font_it->font_size > 0) {
					font_size = font_it->font_size;
				}
			}
			ItemFontSize* font_size_it = _find_font_size(item);
			if (font_size_it && font_size_it->font_size > 0) {
				font_size = font_size_it->font_size;
			}
			if (tab_size > 0) {
				margin += MAX(1, lvl * tab_size *
									 (font->get_char_size(' ', font_size).width +
										 font->get_spacing(TextServer::SPACING_SPACE)));
			}
		}

		item = item->parent;
	}

	return margin;
}

uint32_t RichTextLabel::_find_jst_flags(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			ItemParagraph* p = static_cast<ItemParagraph*>(item);
			return p->jst_flags;
		}

		item = item->parent;
	}

	return default_jst_flags;
}

PackedFloat32Array RichTextLabel::_find_tab_stops(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			ItemParagraph* p = static_cast<ItemParagraph*>(item);
			return p->tab_stops;
		}

		item = item->parent;
	}

	return default_tab_stops;
}

HorizontalAlignment RichTextLabel::_find_alignment(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			ItemParagraph* p = static_cast<ItemParagraph*>(item);
			return p->alignment;
		}

		item = item->parent;
	}

	return default_alignment;
}

TextServer::Direction RichTextLabel::_find_direction(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			ItemParagraph* p = static_cast<ItemParagraph*>(item);
			if (p->direction != Control::TEXT_DIRECTION_INHERITED) {
				return (TextServer::Direction)p->direction;
			}
		}

		item = item->parent;
	}

	if (text_direction == Control::TEXT_DIRECTION_INHERITED) {
		return is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR;
	}
	else {
		return (TextServer::Direction)text_direction;
	}
}

TextServer::StructuredTextParser RichTextLabel::_find_stt(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_PARAGRAPH) {
			ItemParagraph* p = static_cast<ItemParagraph*>(item);
			return p->st_parser;
		}

		item = item->parent;
	}

	return st_parser;
}

Color RichTextLabel::_find_color(Item* p_item, const Color& p_default_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_COLOR) {
			ItemColor* color = static_cast<ItemColor*>(item);
			return color->color;
		}

		item = item->parent;
	}

	return p_default_color;
}

Color RichTextLabel::_find_outline_color(Item* p_item, const Color& p_default_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_OUTLINE_COLOR) {
			ItemOutlineColor* color = static_cast<ItemOutlineColor*>(item);
			return color->color;
		}

		item = item->parent;
	}

	return p_default_color;
}

bool RichTextLabel::_find_underline(Item* p_item, Color* r_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_UNDERLINE) {
			if (r_color) {
				ItemUnderline* ul = static_cast<ItemUnderline*>(item);
				*r_color = ul->color;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

bool RichTextLabel::_find_strikethrough(Item* p_item, Color* r_color)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_STRIKETHROUGH) {
			if (r_color) {
				ItemStrikethrough* st = static_cast<ItemStrikethrough*>(item);
				*r_color = st->color;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

void RichTextLabel::_fetch_item_fx_stack(Item* p_item, Vector<ItemFX*>& r_stack)
{
	Item* item = p_item;
	while (item) {
		if (item->type == ITEM_CUSTOMFX || item->type == ITEM_SHAKE || item->type == ITEM_WAVE ||
			item->type == ITEM_TORNADO || item->type == ITEM_RAINBOW || item->type == ITEM_PULSE) {
			r_stack.push_back(static_cast<ItemFX*>(item));
		}

		item = item->parent;
	}
}

void RichTextLabel::_normalize_subtags(Vector<String>& subtags)
{
	for (String& subtag : subtags) {
		subtag = subtag.unquote();
	}
}

bool RichTextLabel::_find_hint(Item* p_item, String* r_description)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_HINT) {
			ItemHint* hint = static_cast<ItemHint*>(item);
			if (r_description) {
				*r_description = hint->description;
			}
			return true;
		}

		item = item->parent;
	}

	return false;
}

Color RichTextLabel::_find_bgcolor(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_BGCOLOR) {
			ItemBGColor* color = static_cast<ItemBGColor*>(item);
			return color->color;
		}

		item = item->parent;
	}

	return Color(0, 0, 0, 0);
}

Color RichTextLabel::_find_fgcolor(Item* p_item)
{
	Item* item = p_item;

	while (item) {
		if (item->type == ITEM_FGCOLOR) {
			ItemFGColor* color = static_cast<ItemFGColor*>(item);
			return color->color;
		}

		item = item->parent;
	}

	return Color(0, 0, 0, 0);
}

bool RichTextLabel::_find_layout_subitem(Item* from, Item* to)
{
	if (from && from != to) {
		if (from->type != ITEM_FONT && from->type != ITEM_COLOR && from->type != ITEM_UNDERLINE &&
			from->type != ITEM_STRIKETHROUGH && from->type != ITEM_INDENT) {
			return true;
		}

		for (Item* E : from->subitems) {
			bool layout = _find_layout_subitem(E, to);

			if (layout) {
				return true;
			}
		}
	}

	return false;
}

void RichTextLabel::_thread_end()
{
	set_physics_process_internal(false);
	if (!scroll_visible) {
		vscroll->hide();
	}
	if (is_visible_in_tree()) {
		queue_accessibility_update();
		queue_redraw();
	}
}

void RichTextLabel::_stop_thread()
{
	if (threaded) {
		stop_thread.store(true);
		wait_until_finished();
	}
}

int RichTextLabel::get_pending_paragraphs() const
{
	int to_line = main->first_invalid_line.load();
	int lines = main->lines.size();

	return lines - to_line;
}

bool RichTextLabel::is_finished() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	if (updating.load()) {
		return false;
	}
	return (main->first_invalid_line.load() == (int)main->lines.size() &&
			main->first_resized_line.load() == (int)main->lines.size() &&
			main->first_invalid_font_line.load() == (int)main->lines.size());
}

bool RichTextLabel::is_updating() const { return updating.load() || validating.load(); }

void RichTextLabel::set_threaded(bool p_threaded)
{
	if (threaded != p_threaded) {
		_stop_thread();
		threaded = p_threaded;
		queue_redraw();
	}
}

bool RichTextLabel::is_threaded() const { return threaded; }

void RichTextLabel::set_progress_bar_delay(int p_delay_ms) { progress_delay = p_delay_ms; }

int RichTextLabel::get_progress_bar_delay() const { return progress_delay; }

_FORCE_INLINE_ float RichTextLabel::_update_scroll_exceeds(float p_total_height,
	float p_ctrl_height, float p_width, int p_idx, float p_old_scroll, float p_text_rect_height)
{
	updating_scroll = true;

	float total_height = p_total_height;
	bool exceeds = scroll_active && p_total_height > p_ctrl_height &&
				   p_width > vscroll->get_bound_minimum_size().width;
	if (exceeds != scroll_visible) {
		if (exceeds) {
			scroll_visible = true;
			_prepare_scroll_anchor();
			vscroll->show();
		}
		else {
			scroll_visible = false;
			scroll_w = 0;
		}

		main->first_resized_line.store(0);

		total_height = 0;
		for (int j = 0; j <= p_idx; j++) {
			total_height = _resize_line(main, j, theme_cache.normal_font,
				theme_cache.normal_font_size, p_width - scroll_w, total_height);

			main->first_resized_line.store(j);
		}
	}
	vscroll->set_max(total_height);
	vscroll->set_page(p_text_rect_height);
	if (scroll_follow && scroll_following) {
		vscroll->set_value(total_height);
	}
	else {
		vscroll->set_value(p_old_scroll);
	}
	updating_scroll = false;

	return total_height;
}

void RichTextLabel::_invalidate_current_line(ItemFrame* p_frame)
{
	if ((int)p_frame->lines.size() - 1 <= p_frame->first_invalid_line) {
		p_frame->first_invalid_line = (int)p_frame->lines.size() - 1;
		queue_accessibility_update();
	}
}

void RichTextLabel::_texture_changed(RID p_item)
{
	Item* it = items.get_or_null(p_item);
	if (it && it->type == ITEM_IMAGE) {
		ItemImage* img = reinterpret_cast<ItemImage*>(it);
		Size2 new_size =
			_get_image_size(img->image, img->rq_size.width, img->rq_size.height, img->region);
		if (img->size != new_size) {
			main->first_invalid_line.store(0);
			img->size = new_size;
		}
	}
	queue_redraw();
}

void RichTextLabel::add_text(const String& p_text)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	if (current->type == ITEM_TABLE) {
		return; // can't add anything here
	}

	int pos = 0;
	String t = p_text.replace("\r\n", "\n");

	while (pos < t.length()) {
		int end = t.find_char('\n', pos);
		String line;
		bool eol = false;
		if (end == -1) {
			end = t.length();
		}
		else {
			eol = true;
		}

		if (pos == 0 && end == t.length()) {
			line = t;
		}
		else {
			line = t.substr(pos, end - pos);
		}

		if (line.length() > 0) {
			if (current->subitems.size() && current->subitems.back()->get()->type == ITEM_TEXT) {
				// append text condition!
				ItemText* ti = static_cast<ItemText*>(current->subitems.back()->get());
				ti->text += line;
				current_char_ofs += line.length();
				_invalidate_current_line(main);

			}
			else {
				// append item condition
				ItemText* item = memnew(ItemText);
				item->rid = items.make_rid(item);
				item->text = line;
				_add_item(item, false);
			}
		}

		if (eol) {
			ItemNewline* item = memnew(ItemNewline); // Sets item->type to ITEM_NEWLINE.
			item->rid = items.make_rid(item);
			item->line = current_frame->lines.size();
			_add_item(item, false);
			current_frame->lines.resize(current_frame->lines.size() + 1);
			if (item->type !=
				ITEM_NEWLINE) { // item IS an ITEM_NEWLINE so this will never get called?
				current_frame->lines[current_frame->lines.size() - 1].from = item;
			}
			_invalidate_current_line(current_frame);
		}

		pos = end + 1;
	}
	queue_redraw();
}

void RichTextLabel::_add_item(Item* p_item, bool p_enter, bool p_ensure_newline)
{
	if (!internal_stack_editing) {
		stack_externally_modified = true;
	}

	if (p_enter && !parsing_bbcode.load() && !tag_stack.is_empty()) {
		tag_stack.push_back(U"?");
	}

	p_item->parent = current;
	p_item->E = current->subitems.push_back(p_item);
	p_item->index = current_idx++;
	p_item->char_ofs = current_char_ofs;
	if (p_item->type == ITEM_TEXT) {
		ItemText* t = static_cast<ItemText*>(p_item);
		current_char_ofs += t->text.length();
	}
	else if (p_item->type == ITEM_IMAGE) {
		current_char_ofs++;
	}
	else if (p_item->type == ITEM_NEWLINE) {
		current_char_ofs++;
	}

	if (p_enter) {
		current = p_item;
	}

	if (p_ensure_newline) {
		Item* from = current_frame->lines[current_frame->lines.size() - 1].from;
		// only create a new line for Item types that generate content/layout, ignore those that
		// represent formatting/styling
		if (_find_layout_subitem(from, p_item)) {
			_invalidate_current_line(current_frame);
			current_frame->lines.resize(current_frame->lines.size() + 1);
		}
	}

	if (current_frame->lines[current_frame->lines.size() - 1].from == nullptr) {
		current_frame->lines[current_frame->lines.size() - 1].from = p_item;
	}
	p_item->line = current_frame->lines.size() - 1;

	_invalidate_current_line(current_frame);

	if (fit_content) {
		update_minimum_size();
	}
	queue_accessibility_update();
	queue_redraw();
}

Size2 RichTextLabel::_get_image_size(
	const Ref<Texture2D>& p_image, float p_width, float p_height, const Rect2& p_region)
{
	Size2 ret;
	if (p_width > 0) {
		// custom width
		ret.width = p_width;
		if (p_height > 0) {
			// custom height
			ret.height = p_height;
		}
		else {
			// calculate height to keep aspect ratio
			if (p_region.has_area()) {
				ret.height = p_region.get_size().height * p_width / p_region.get_size().width;
			}
			else {
				ret.height = p_image->get_height() * p_width / p_image->get_width();
			}
		}
	}
	else {
		if (p_height > 0) {
			// custom height
			ret.height = p_height;
			// calculate width to keep aspect ratio
			if (p_region.has_area()) {
				ret.width = p_region.get_size().width * p_height / p_region.get_size().height;
			}
			else {
				ret.width = p_image->get_width() * p_height / p_image->get_height();
			}
		}
		else {
			if (p_region.has_area()) {
				// if the image has a region, keep the region size
				ret = p_region.get_size();
			}
			else {
				// keep original width and height
				ret = p_image->get_size();
			}
		}
	}
	return ret;
}

void RichTextLabel::add_newline()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	if (current->type == ITEM_TABLE) {
		return;
	}
	ItemNewline* item = memnew(ItemNewline);
	item->rid = items.make_rid(item);
	item->line = current_frame->lines.size();
	_add_item(item, false);
	current_frame->lines.resize(current_frame->lines.size() + 1);
	_invalidate_current_line(current_frame);
	queue_redraw();
}

void RichTextLabel::_remove_frame(HashSet<Item*>& r_erase_list, ItemFrame* p_frame, int p_line,
	bool p_erase, int p_char_offset, int p_line_offset)
{
	Line& l = p_frame->lines[p_line];
	Item* it_to =
		(p_line + 1 < (int)p_frame->lines.size()) ? p_frame->lines[p_line + 1].from : nullptr;
	if (!p_erase) {
		l.char_offset -= p_char_offset;
	}

	for (Item* it = l.from; it && it != it_to;) {
		Item* next_it = _get_next_item(it);
		it->line -= p_line_offset;
		if (!p_erase) {
			while (r_erase_list.has(it->parent)) {
				it->E->erase();
				it->parent = it->parent->parent;
				it->E = it->parent->subitems.push_back(it);
			}
		}
		if (it->type == ITEM_TABLE) {
			ItemTable* table = static_cast<ItemTable*>(it);
			for (List<Item*>::Element* sub_it = table->subitems.front(); sub_it;
				 sub_it = sub_it->next()) {
				ERR_CONTINUE(sub_it->get()->type != ITEM_FRAME); // Children should all be frames.
				ItemFrame* frame = static_cast<ItemFrame*>(sub_it->get());
				for (int i = 0; i < (int)frame->lines.size(); i++) {
					_remove_frame(r_erase_list, frame, i, p_erase, p_char_offset, 0);
				}
				if (p_erase) {
					r_erase_list.insert(frame);
				}
				else {
					frame->char_ofs -= p_char_offset;
				}
			}
		}
		if (p_erase) {
			r_erase_list.insert(it);
		}
		else {
			it->char_ofs -= p_char_offset;
		}
		it = next_it;
	}
}

bool RichTextLabel::remove_paragraph(int p_paragraph, bool p_no_invalidate)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	if (p_paragraph >= (int)main->lines.size() || p_paragraph < 0) {
		return false;
	}

	stack_externally_modified = true;

	if (main->lines.size() == 1) {
		// Clear all.
		main->_clear_children();
		current = main;
		current_frame = main;
		main->lines.clear();
		main->lines.resize(1);

		current_char_ofs = 0;
	}
	else {
		HashSet<Item*> erase_list;
		Line& l = main->lines[p_paragraph];
		int off = l.char_count;
		for (int i = p_paragraph; i < (int)main->lines.size(); i++) {
			if (i == p_paragraph) {
				_remove_frame(erase_list, main, i, true, off, 0);
			}
			else {
				_remove_frame(erase_list, main, i, false, off, 1);

				Item* it_to = (i + 1 < (int)main->lines.size()) ? main->lines[i + 1].from : nullptr;
				Line& nl = main->lines[i];
				while (erase_list.has(nl.from)) {
					nl.from = _get_next_item(nl.from);
					if (nl.from == it_to) {
						nl.from = nullptr;
						break;
					}
				}
			}
		}
		for (HashSet<Item*>::Iterator E = erase_list.begin(); E; ++E) {
			Item* it = *E;
			if (current_frame == it) {
				current_frame = main;
			}
			if (current == it) {
				current = main;
			}
			if (!erase_list.has(it->parent)) {
				it->E->erase();
			}
			it->subitems.clear();
			memdelete(it);
		}
		main->lines.remove_at(p_paragraph);
		current_char_ofs -= off;
	}

	selection.click_frame = nullptr;
	selection.click_item = nullptr;
	selection.active = false;

	if (is_processing_internal()) {
		bool process_enabled = false;
		Item* it = main;
		while (it) {
			Vector<ItemFX*> fx_stack;
			_fetch_item_fx_stack(it, fx_stack);
			if (fx_stack.size()) {
				process_enabled = true;
				break;
			}
			it = _get_next_item(it, true);
		}
		set_process_internal(process_enabled);
	}

	if (p_no_invalidate) {
		// Do not invalidate cache, only update vertical offsets of the paragraphs after deleted one
		// and scrollbar.
		int to_line = main->first_invalid_line.load() - 1;
		float total_height =
			(p_paragraph == 0) ? 0 : _calculate_line_vertical_offset(main->lines[p_paragraph - 1]);
		for (int i = p_paragraph; i < to_line; i++) {
			MutexLock lock(main->lines[to_line - 1].text_buf->get_mutex());
			main->lines[i].offset.y = total_height;
			total_height = _calculate_line_vertical_offset(main->lines[i]);
		}
		updating_scroll = true;
		vscroll->set_max(total_height);
		updating_scroll = false;

		main->first_invalid_line.store(MAX(main->first_invalid_line.load() - 1, 0));
		main->first_resized_line.store(MAX(main->first_resized_line.load() - 1, 0));
		main->first_invalid_font_line.store(MAX(main->first_invalid_font_line.load() - 1, 0));
	}
	else {
		// Invalidate cache after the deleted paragraph.
		main->first_invalid_line.store(MIN(main->first_invalid_line.load(), p_paragraph));
		main->first_resized_line.store(MIN(main->first_resized_line.load(), p_paragraph));
		main->first_invalid_font_line.store(MIN(main->first_invalid_font_line.load(), p_paragraph));
	}
	queue_redraw();

	return true;
}

bool RichTextLabel::invalidate_paragraph(int p_paragraph)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	if (p_paragraph >= (int)main->lines.size() || p_paragraph < 0) {
		return false;
	}

	// Invalidate cache.
	main->first_invalid_line.store(MIN(main->first_invalid_line.load(), p_paragraph));
	main->first_resized_line.store(MIN(main->first_resized_line.load(), p_paragraph));
	main->first_invalid_font_line.store(MIN(main->first_invalid_font_line.load(), p_paragraph));

	_invalidate_accessibility();
	if (is_inside_tree()) {
		queue_accessibility_update();
	}
	queue_redraw();
	update_configuration_warnings();

	return true;
}

void RichTextLabel::_invalidate_fonts()
{
	_stop_thread();
	main->first_invalid_font_line.store(0); // Invalidate all lines.
	_invalidate_accessibility();
	queue_accessibility_update();
	queue_redraw();
}

void RichTextLabel::push_normal()
{
	ERR_FAIL_COND(theme_cache.normal_font.is_null());

	_push_def_font(RTL_NORMAL_FONT);
}

void RichTextLabel::push_bold()
{
	ERR_FAIL_COND(theme_cache.bold_font.is_null());

	ItemFont* item_font = _find_font(current);
	_push_def_font((item_font && item_font->def_font == RTL_ITALICS_FONT) ? RTL_BOLD_ITALICS_FONT
																		  : RTL_BOLD_FONT);
}

void RichTextLabel::push_bold_italics()
{
	ERR_FAIL_COND(theme_cache.bold_italics_font.is_null());

	_push_def_font(RTL_BOLD_ITALICS_FONT);
}

void RichTextLabel::push_italics()
{
	ERR_FAIL_COND(theme_cache.italics_font.is_null());

	ItemFont* item_font = _find_font(current);
	_push_def_font((item_font && item_font->def_font == RTL_BOLD_FONT) ? RTL_BOLD_ITALICS_FONT
																	   : RTL_ITALICS_FONT);
}

void RichTextLabel::push_mono()
{
	ERR_FAIL_COND(theme_cache.mono_font.is_null());

	_push_def_font(RTL_MONO_FONT);
}

void RichTextLabel::push_font_size(int p_font_size)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemFontSize* item = memnew(ItemFontSize);
	item->font_size = p_font_size;
	_add_item(item, true);
}

void RichTextLabel::push_outline_size(int p_ol_size)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemOutlineSize* item = memnew(ItemOutlineSize);
	item->outline_size = p_ol_size;
	_add_item(item, true);
}

void RichTextLabel::push_color(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemColor* item = memnew(ItemColor);
	item->color = p_color;
	_add_item(item, true);
}

void RichTextLabel::push_outline_color(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemOutlineColor* item = memnew(ItemOutlineColor);
	item->color = p_color;
	_add_item(item, true);
}

void RichTextLabel::push_underline(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemUnderline* item = memnew(ItemUnderline);
	item->color = p_color;

	_add_item(item, true);
}

void RichTextLabel::push_strikethrough(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemStrikethrough* item = memnew(ItemStrikethrough);
	item->color = p_color;

	_add_item(item, true);
}

void RichTextLabel::push_paragraph(HorizontalAlignment p_alignment,
	Control::TextDirection p_direction, const String& p_language,
	TextServer::StructuredTextParser p_st_parser, uint32_t p_jst_flags,
	const PackedFloat32Array& p_tab_stops)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);

	ItemParagraph* item = memnew(ItemParagraph);
	item->alignment = p_alignment;
	item->direction = p_direction;
	item->language = p_language;
	item->st_parser = p_st_parser;
	item->jst_flags = p_jst_flags;
	item->tab_stops = p_tab_stops;
	_add_item(item, true, true);
}

void RichTextLabel::push_indent(int p_level)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ERR_FAIL_COND(p_level < 0);

	ItemIndent* item = memnew(ItemIndent);
	item->level = p_level;
	_add_item(item, true, true);
}

void RichTextLabel::push_list(
	int p_level, ListType p_list, bool p_capitalize, const String& p_bullet)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ERR_FAIL_COND(p_level < 0);

	ItemList* item = memnew(ItemList);
	item->list_type = p_list;
	item->level = p_level;
	item->capitalize = p_capitalize;
	item->bullet = p_bullet;
	_add_item(item, true, true);
}

void RichTextLabel::push_language(const String& p_language)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemLanguage* item = memnew(ItemLanguage);
	item->language = p_language;
	_add_item(item, true);
}

void RichTextLabel::push_hint(const String& p_string)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemHint* item = memnew(ItemHint);
	item->description = p_string;
	_add_item(item, true);
}

void RichTextLabel::push_table(
	int p_columns, InlineAlignment p_alignment, int p_align_to_row, const String& p_alt_text)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ERR_FAIL_COND(p_columns < 1);
	ItemTable* item = memnew(ItemTable);
	item->rid = items.make_rid(item);
	item->name = p_alt_text;
	item->columns.resize(p_columns);
	item->total_width = 0;
	item->inline_align = p_alignment;
	item->align_to_row = p_align_to_row;
	for (int i = 0; i < (int)item->columns.size(); i++) {
		item->columns[i].expand = false;
		item->columns[i].shrink = true;
		item->columns[i].expand_ratio = 1;
	}
	_add_item(item, true, false);
}

void RichTextLabel::push_fade(int p_start_index, int p_length)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemFade* item = memnew(ItemFade);
	item->starting_index = p_start_index;
	item->length = p_length;
	_add_item(item, true);
}

void RichTextLabel::push_shake(int p_strength = 10, float p_rate = 24.0f, bool p_connected = true)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemShake* item = memnew(ItemShake);
	item->strength = p_strength;
	item->rate = p_rate;
	item->connected = p_connected;
	_add_item(item, true);
}

void RichTextLabel::push_wave(
	float p_frequency = 1.0f, float p_amplitude = 10.0f, bool p_connected = true)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemWave* item = memnew(ItemWave);
	item->frequency = p_frequency;
	item->amplitude = p_amplitude;
	item->connected = p_connected;
	_add_item(item, true);
}

void RichTextLabel::push_tornado(
	float p_frequency = 1.0f, float p_radius = 10.0f, bool p_connected = true)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemTornado* item = memnew(ItemTornado);
	item->frequency = p_frequency;
	item->radius = p_radius;
	item->connected = p_connected;
	_add_item(item, true);
}

void RichTextLabel::push_rainbow(
	float p_saturation, float p_value, float p_frequency, float p_speed)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemRainbow* item = memnew(ItemRainbow);
	item->speed = p_speed;
	item->frequency = p_frequency;
	item->saturation = p_saturation;
	item->value = p_value;
	_add_item(item, true);
}

void RichTextLabel::push_pulse(const Color& p_color, float p_frequency, float p_ease)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ItemPulse* item = memnew(ItemPulse);
	item->color = p_color;
	item->frequency = p_frequency;
	item->ease = p_ease;
	_add_item(item, true);
}

void RichTextLabel::push_bgcolor(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemBGColor* item = memnew(ItemBGColor);
	item->color = p_color;
	_add_item(item, true);
}

void RichTextLabel::push_fgcolor(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemFGColor* item = memnew(ItemFGColor);
	item->color = p_color;
	_add_item(item, true);
}

void RichTextLabel::push_context()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type == ITEM_TABLE);
	ItemContext* item = memnew(ItemContext);
	_add_item(item, true);
}

void RichTextLabel::set_table_column_expand(int p_column, bool p_expand, int p_ratio, bool p_shrink)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_TABLE);

	ItemTable* table = static_cast<ItemTable*>(current);
	ERR_FAIL_INDEX(p_column, (int)table->columns.size());
	table->columns[p_column].expand = p_expand;
	table->columns[p_column].shrink = p_shrink;
	table->columns[p_column].expand_ratio = p_ratio;
}

void RichTextLabel::set_table_column_name(int p_column, const String& p_name)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_TABLE);

	ItemTable* table = static_cast<ItemTable*>(current);
	ERR_FAIL_INDEX(p_column, (int)table->columns.size());
	table->columns[p_column].name = p_name;
}

void RichTextLabel::set_cell_row_background_color(
	const Color& p_odd_row_bg, const Color& p_even_row_bg)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_FRAME);

	ItemFrame* cell = static_cast<ItemFrame*>(current);
	ERR_FAIL_COND(!cell->cell);
	cell->odd_row_bg = p_odd_row_bg;
	cell->even_row_bg = p_even_row_bg;
}

void RichTextLabel::set_cell_border_color(const Color& p_color)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_FRAME);

	ItemFrame* cell = static_cast<ItemFrame*>(current);
	ERR_FAIL_COND(!cell->cell);
	cell->border = p_color;
}

void RichTextLabel::set_cell_size_override(const Size2& p_min_size, const Size2& p_max_size)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_FRAME);

	ItemFrame* cell = static_cast<ItemFrame*>(current);
	ERR_FAIL_COND(!cell->cell);
	cell->min_size_over = p_min_size;
	cell->max_size_over = p_max_size;
}

void RichTextLabel::set_cell_padding(const Rect2& p_padding)
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_FRAME);

	ItemFrame* cell = static_cast<ItemFrame*>(current);
	ERR_FAIL_COND(!cell->cell);
	cell->padding = p_padding;
}

void RichTextLabel::push_cell()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_COND(current->type != ITEM_TABLE);

	ItemFrame* item = memnew(ItemFrame);
	item->parent_frame = current_frame;
	_add_item(item, true);
	current_frame = item;
	item->cell = true;
	item->lines.resize(1);
	item->lines[0].from = nullptr;
	item->first_invalid_line.store(0); // parent frame last line ???
	queue_accessibility_update();
}

int RichTextLabel::get_current_table_column() const
{
	ERR_FAIL_COND_V(current->type != ITEM_TABLE, -1);

	ItemTable* table = static_cast<ItemTable*>(current);
	return table->subitems.size() % table->columns.size();
}

void RichTextLabel::pop()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_NULL(current->parent);

	if (current->type == ITEM_FRAME) {
		current_frame = static_cast<ItemFrame*>(current)->parent_frame;
	}
	current = current->parent;
	if (!parsing_bbcode.load() && !tag_stack.is_empty()) {
		tag_stack.pop_back();
	}
}

void RichTextLabel::pop_context()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	ERR_FAIL_NULL(current->parent);

	while (current->parent && current != main) {
		if (current->type == ITEM_FRAME) {
			current_frame = static_cast<ItemFrame*>(current)->parent_frame;
		}
		else if (current->type == ITEM_CONTEXT) {
			if (!parsing_bbcode.load() && !tag_stack.is_empty()) {
				tag_stack.pop_back();
			}
			current = current->parent;
			return;
		}
		if (!parsing_bbcode.load() && !tag_stack.is_empty()) {
			tag_stack.pop_back();
		}
		current = current->parent;
	}
}

void RichTextLabel::pop_all()
{
	_stop_thread();
	MutexLock data_lock(data_mutex);

	current = main;
	current_frame = main;
}

void RichTextLabel::clear()
{
	_stop_thread();
	set_process_internal(false);
	MutexLock data_lock(data_mutex);

	stack_externally_modified = false;

	tag_stack.clear();
	main->_clear_children();
	current = main;
	current_frame = main;
	main->lines.clear();
	main->lines.resize(1);
	main->first_invalid_line.store(0);
	_invalidate_accessibility();

	keyboard_focus_frame = nullptr;
	keyboard_focus_line = 0;
	keyboard_focus_item = nullptr;

	selection.click_frame = nullptr;
	selection.click_item = nullptr;
	deselect();

	current_idx = 1;
	current_char_ofs = 0;
	if (scroll_follow) {
		scroll_following = true;
	}

	if (fit_content) {
		update_minimum_size();
	}
	queue_accessibility_update();
	update_configuration_warnings();
}

void RichTextLabel::set_tab_size(int p_spaces)
{
	if (tab_size == p_spaces) {
		return;
	}

	_stop_thread();

	tab_size = p_spaces;
	main->first_resized_line.store(0);
	_invalidate_accessibility();
	queue_accessibility_update();
	queue_redraw();
}

int RichTextLabel::get_tab_size() const { return tab_size; }

void RichTextLabel::set_fit_content(bool p_enabled)
{
	if (p_enabled == fit_content) {
		return;
	}

	fit_content = p_enabled;
	update_minimum_size();
}

bool RichTextLabel::is_fit_content_enabled() const { return fit_content; }

void RichTextLabel::set_meta_underline(bool p_underline)
{
	if (underline_meta == p_underline) {
		return;
	}

	underline_meta = p_underline;
	queue_redraw();
}

bool RichTextLabel::is_meta_underlined() const { return underline_meta; }

void RichTextLabel::set_hint_underline(bool p_underline)
{
	underline_hint = p_underline;
	queue_redraw();
}

bool RichTextLabel::is_hint_underlined() const { return underline_hint; }

void RichTextLabel::set_offset(int p_pixel)
{
	vscroll->set_value(p_pixel);
	queue_accessibility_update();
}

void RichTextLabel::set_scroll_active(bool p_active)
{
	if (scroll_active == p_active) {
		return;
	}

	scroll_active = p_active;
	vscroll->set_drag_node_enabled(p_active);
	vscroll->set_visible(p_active);
	_apply_translation(); // without this, RichLabelText is not updated in the editor/game.
	queue_redraw();
}

bool RichTextLabel::is_scroll_active() const { return scroll_active; }

void RichTextLabel::set_scroll_follow(bool p_follow)
{
	scroll_follow = p_follow;
	if (!vscroll->is_visible_in_tree() ||
		vscroll->get_value() > (vscroll->get_max() - vscroll->get_page() - 1)) {
		scroll_following = true;
	}
}

bool RichTextLabel::is_scroll_following() const { return scroll_follow; }

void RichTextLabel::_update_follow_vc()
{
	if (!scroll_follow_visible_characters) {
		return;
	}
	int vc = (visible_characters < 0 ? get_total_character_count()
									 : MIN(visible_characters, get_total_character_count())) -
			 1;
	int voff = get_character_line(vc) + 1;
	if (voff <= get_line_count() - 1) {
		follow_vc_pos = get_line_offset(voff) - _get_text_rect().size.y;
	}
	else {
		follow_vc_pos = vscroll->get_max();
	}
	vscroll->scroll_to(follow_vc_pos);
}

void RichTextLabel::set_scroll_follow_visible_characters(bool p_follow)
{
	if (scroll_follow_visible_characters != p_follow) {
		scroll_follow_visible_characters = p_follow;
		_update_follow_vc();
	}
}

bool RichTextLabel::is_scroll_following_visible_characters() const
{
	return scroll_follow_visible_characters;
}

void RichTextLabel::parse_bbcode(const String& p_bbcode)
{
	clear();
	append_text(p_bbcode);
}

String RichTextLabel::_get_tag_value(const String& p_tag)
{
	return p_tag.substr(p_tag.find_char('=') + 1);
}

int RichTextLabel::_find_unquoted(const String& p_src, char32_t p_chr, int p_from)
{
	if (p_from < 0) {
		return -1;
	}

	const int len = p_src.length();
	if (len == 0) {
		return -1;
	}

	const char32_t* src = p_src.get_data();
	bool in_single_quote = false;
	bool in_double_quote = false;
	for (int i = p_from; i < len; i++) {
		if (in_double_quote) {
			if (src[i] == '"') {
				in_double_quote = false;
			}
		}
		else if (in_single_quote) {
			if (src[i] == '\'') {
				in_single_quote = false;
			}
		}
		else {
			if (src[i] == '"') {
				in_double_quote = true;
			}
			else if (src[i] == '\'') {
				in_single_quote = true;
			}
			else if (src[i] == p_chr) {
				return i;
			}
		}
	}

	return -1;
}

Vector<String> RichTextLabel::_split_unquoted(const String& p_src, char32_t p_splitter)
{
	Vector<String> ret;

	if (p_src.is_empty()) {
		return ret;
	}

	int from = 0;
	int len = p_src.length();

	while (true) {
		int end = _find_unquoted(p_src, p_splitter, from);
		if (end < 0) {
			end = len;
		}
		if (end > from) {
			ret.push_back(p_src.substr(from, end - from));
		}
		if (end == len) {
			break;
		}

		from = end + 1;
	}

	return ret;
}

void RichTextLabel::scroll_to_selection()
{
	float line_offset = get_selection_line_offset();
	if (line_offset != -1.0) {
		vscroll->set_value(line_offset);
		queue_accessibility_update();
	}
}

void RichTextLabel::scroll_to_paragraph(int p_paragraph)
{
	_validate_line_caches();

	if (p_paragraph <= 0) {
		vscroll->set_value(0);
	}
	else if (p_paragraph >= main->first_invalid_line.load()) {
		vscroll->set_value(vscroll->get_max());
	}
	else {
		vscroll->set_value(main->lines[p_paragraph].offset.y);
	}
	queue_accessibility_update();
}

int RichTextLabel::get_paragraph_count() const { return main->lines.size(); }

int RichTextLabel::get_visible_paragraph_count() const
{
	if (!is_visible()) {
		return 0;
	}

	const_cast<RichTextLabel*>(this)->_validate_line_caches();
	return visible_paragraph_count;
}

void RichTextLabel::scroll_to_line(int p_line)
{
	if (p_line <= 0) {
		vscroll->set_value(0);
		queue_accessibility_update();
		return;
	}
	_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		if ((line_count <= p_line) &&
			(line_count + main->lines[i].text_buf->get_line_count() >= p_line)) {
			float line_offset = 0.f;
			for (int j = 0; j < p_line - line_count; j++) {
				line_offset += main->lines[i].text_buf->get_line_ascent(j) +
							   main->lines[i].text_buf->get_line_descent(j) +
							   theme_cache.line_separation;
			}
			vscroll->set_value(main->lines[i].offset.y + line_offset);
			queue_accessibility_update();
			return;
		}
		line_count += main->lines[i].text_buf->get_line_count();
	}
	vscroll->set_value(vscroll->get_max());
	queue_accessibility_update();
}

float RichTextLabel::get_line_offset(int p_line)
{
	_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		if ((line_count <= p_line) &&
			(p_line <= line_count + main->lines[i].text_buf->get_line_count())) {
			float line_offset = 0.f;
			for (int j = 0; j < p_line - line_count; j++) {
				line_offset += main->lines[i].text_buf->get_line_ascent(j) +
							   main->lines[i].text_buf->get_line_descent(j) +
							   theme_cache.line_separation;
			}
			return main->lines[i].offset.y + line_offset;
		}
		line_count += main->lines[i].text_buf->get_line_count();
	}
	return 0;
}

float RichTextLabel::get_paragraph_offset(int p_paragraph)
{
	_validate_line_caches();

	int to_line = main->first_invalid_line.load();
	if (0 <= p_paragraph && p_paragraph < to_line) {
		return main->lines[p_paragraph].offset.y;
	}
	return 0;
}

int RichTextLabel::get_line_count() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		line_count += main->lines[i].text_buf->get_line_count();
	}
	return line_count;
}

Vector2i RichTextLabel::get_line_range(int p_line)
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		int lc = main->lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			Vector2i char_offset = Vector2i(main->lines[i].char_offset, main->lines[i].char_offset);
			Vector2i line_range = main->lines[i].text_buf->get_line_range(p_line - line_count);
			return char_offset + line_range;
		}

		line_count += lc;
	}
	return Vector2i();
}

int RichTextLabel::get_visible_line_count() const
{
	if (!is_visible()) {
		return 0;
	}
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	return visible_line_count;
}

void RichTextLabel::set_selection_enabled(bool p_enabled)
{
	if (selection.enabled == p_enabled) {
		return;
	}

	selection.enabled = p_enabled;
	if (!p_enabled) {
		if (selection.active) {
			deselect();
		}
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
	else {
		set_focus_mode(FOCUS_ALL);
	}
	queue_accessibility_update();
}

void RichTextLabel::set_deselect_on_focus_loss_enabled(const bool p_enabled)
{
	if (deselect_on_focus_loss_enabled == p_enabled) {
		return;
	}

	deselect_on_focus_loss_enabled = p_enabled;
	if (p_enabled && selection.active && !has_focus()) {
		deselect();
	}
}

bool RichTextLabel::_is_click_inside_selection() const
{
	if (selection.active && selection.enabled && selection.click_frame && selection.from_frame &&
		selection.to_frame) {
		const Line& l_click = selection.click_frame->lines[selection.click_line];
		const Line& l_from = selection.from_frame->lines[selection.from_line];
		const Line& l_to = selection.to_frame->lines[selection.to_line];
		return (l_click.char_offset + selection.click_char >=
				   l_from.char_offset + selection.from_char) &&
			   (l_click.char_offset + selection.click_char <= l_to.char_offset + selection.to_char);
	}
	else {
		return false;
	}
}

bool RichTextLabel::_search_table_cell(ItemTable* p_table, List<Item*>::Element* p_cell,
	const String& p_string, bool p_reverse_search, int p_from_line)
{
	ERR_FAIL_COND_V(p_cell->get()->type != ITEM_FRAME, false); // Children should all be frames.
	ItemFrame* frame = static_cast<ItemFrame*>(p_cell->get());
	if (p_from_line < 0) {
		p_from_line = (int)frame->lines.size() - 1;
	}

	if (p_reverse_search) {
		for (int i = p_from_line; i >= 0; i--) {
			if (_search_line(frame, i, p_string, -1, p_reverse_search)) {
				return true;
			}
		}
	}
	else {
		for (int i = p_from_line; i < (int)frame->lines.size(); i++) {
			if (_search_line(frame, i, p_string, 0, p_reverse_search)) {
				return true;
			}
		}
	}

	return false;
}

bool RichTextLabel::_search_table(
	ItemTable* p_table, List<Item*>::Element* p_from, const String& p_string, bool p_reverse_search)
{
	List<Item*>::Element* E = p_from;
	while (E != nullptr) {
		int from_line = p_reverse_search ? -1 : 0;
		if (_search_table_cell(p_table, E, p_string, p_reverse_search, from_line)) {
			return true;
		}
		E = p_reverse_search ? E->prev() : E->next();
	}
	return false;
}

bool RichTextLabel::_search_line(
	ItemFrame* p_frame, int p_line, const String& p_string, int p_char_idx, bool p_reverse_search)
{
	ERR_FAIL_NULL_V(p_frame, false);
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)p_frame->lines.size(), false);

	Line& l = p_frame->lines[p_line];

	String txt;
	Item* it_to =
		(p_line + 1 < (int)p_frame->lines.size()) ? p_frame->lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; it && it != it_to; it = _get_next_item(it)) {
		switch (it->type) {
		case ITEM_NEWLINE: {
			txt += "\n";
		} break;
		case ITEM_TEXT: {
			ItemText* t = static_cast<ItemText*>(it);
			txt += t->text;
		} break;
		case ITEM_IMAGE: {
			txt += " ";
		} break;
		case ITEM_TABLE: {
			ItemTable* table = static_cast<ItemTable*>(it);
			List<Item*>::Element* E =
				p_reverse_search ? table->subitems.back() : table->subitems.front();
			if (_search_table(table, E, p_string, p_reverse_search)) {
				return true;
			}
		} break;
		default:
			break;
		}
	}

	int sp = -1;
	if (p_reverse_search) {
		sp = txt.rfindn(p_string, p_char_idx);
	}
	else {
		sp = txt.findn(p_string, p_char_idx);
	}

	if (sp != -1) {
		selection.from_frame = p_frame;
		selection.from_line = p_line;
		selection.from_item = _get_item_at_pos(l.from, it_to, sp);
		selection.from_char = sp;
		selection.to_frame = p_frame;
		selection.to_line = p_line;
		selection.to_item = _get_item_at_pos(l.from, it_to, sp + p_string.length());
		selection.to_char = sp + p_string.length();
		selection.active = true;
		queue_accessibility_update();
		return true;
	}

	return false;
}

bool RichTextLabel::search(const String& p_string, bool p_from_selection, bool p_search_previous)
{
	ERR_FAIL_COND_V(!selection.enabled, false);

	if (p_string.is_empty()) {
		selection.active = false;
		queue_accessibility_update();
		return false;
	}

	int char_idx = p_search_previous ? -1 : 0;
	int current_line = 0;
	int to_line = main->first_invalid_line.load();
	int ending_line = to_line - 1;
	if (p_from_selection && selection.active) {
		// First check to see if other results exist in current line
		char_idx = p_search_previous ? selection.from_char - 1 : selection.to_char;
		if (!(p_search_previous && char_idx < 0) &&
			_search_line(
				selection.from_frame, selection.from_line, p_string, char_idx, p_search_previous)) {
			scroll_to_selection();
			queue_redraw();
			return true;
		}
		char_idx = p_search_previous ? -1 : 0;

		// Next, check to see if the current search result is in a table
		bool in_table = selection.from_frame->parent != nullptr &&
						selection.from_frame->parent->type == ITEM_TABLE;
		if (in_table) {
			// Find last search result in table
			ItemTable* parent_table = static_cast<ItemTable*>(selection.from_frame->parent);
			List<Item*>::Element* parent_element =
				p_search_previous ? parent_table->subitems.back() : parent_table->subitems.front();

			while (parent_element->get() != selection.from_frame) {
				parent_element =
					p_search_previous ? parent_element->prev() : parent_element->next();
				ERR_FAIL_NULL_V(parent_element, false);
			}

			// Search remainder of current cell
			int from_line = p_search_previous ? selection.from_line - 1 : selection.from_line + 1;
			if (from_line >= 0 && _search_table_cell(parent_table, parent_element, p_string,
									  p_search_previous, from_line)) {
				scroll_to_selection();
				queue_redraw();
				return true;
			}

			// Search remainder of table
			if (!(p_search_previous && parent_element == parent_table->subitems.front()) &&
				!(!p_search_previous && parent_element == parent_table->subitems.back())) {
				parent_element = p_search_previous
									 ? parent_element->prev()
									 : parent_element->next(); // Don't want to search current item
				ERR_FAIL_NULL_V(parent_element, false);

				// Search for next element
				if (_search_table(parent_table, parent_element, p_string, p_search_previous)) {
					scroll_to_selection();
					queue_redraw();
					return true;
				}
			}
		}

		ending_line = selection.from_frame->line;
		if (!in_table) {
			ending_line += selection.from_line;
		}
		current_line = p_search_previous ? ending_line - 1 : ending_line + 1;
	}
	else if (p_search_previous) {
		current_line = ending_line;
		ending_line = 0;
	}

	// Search remainder of the file
	while (current_line != ending_line) {
		// Wrap around
		if (current_line < 0) {
			current_line = to_line - 1;
		}
		else if (current_line >= to_line) {
			current_line = 0;
		}

		if (_search_line(main, current_line, p_string, char_idx, p_search_previous)) {
			scroll_to_selection();
			queue_redraw();
			return true;
		}

		if (current_line != ending_line) {
			p_search_previous ? current_line-- : current_line++;
		}
	}

	if (p_from_selection && selection.active) {
		// Check contents of selection
		return _search_line(main, current_line, p_string, char_idx, p_search_previous);
	}
	else {
		return false;
	}
}

String RichTextLabel::_get_line_text(ItemFrame* p_frame, int p_line) const
{
	String txt;

	ERR_FAIL_NULL_V(p_frame, txt);
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)p_frame->lines.size(), txt);

	int start = 0; // Unless the current line is the `from` line.
	if (!selection.from_line_found && p_frame == selection.from_frame) {
		if (p_line < selection.from_line) {
			return txt; // Skip the lines with smaller line numbers in the same `from` frame.
		}
		selection.from_line_found = true;
		start = selection.from_char;
	}
	const bool from_this =
		selection.from_line_found; // Used to detect cases starting from a sub-frame.

	Line& l = p_frame->lines[p_line];

	Item* it_to =
		(p_line + 1 < (int)p_frame->lines.size()) ? p_frame->lines[p_line + 1].from : nullptr;
	for (Item* it = l.from; !selection.to_line_found && it && it != it_to;
		 it = _get_next_item(it)) {
		if (it->type == ITEM_TABLE) {
			ItemTable* table = static_cast<ItemTable*>(it);
			for (Item* E : table->subitems) {
				ERR_CONTINUE(E->type != ITEM_FRAME); // Children should all be frames.
				ItemFrame* frame = static_cast<ItemFrame*>(E);
				for (int i = 0; !selection.to_line_found && i < (int)frame->lines.size(); i++) {
					txt += _get_line_text(frame, i);
				}
				if (selection.to_line_found) {
					break;
				}
			}
			continue;
		}
		if (!selection.from_line_found) {
			continue;
		}
		if (it->type == ITEM_DROPCAP) {
			const ItemDropcap* dc = static_cast<ItemDropcap*>(it);
			txt += dc->text;
		}
		else if (it->type == ITEM_TEXT) {
			const ItemText* t = static_cast<ItemText*>(it);
			txt += t->text;
		}
		else if (it->type == ITEM_NEWLINE) {
			txt += "\n";
		}
		else if (it->type == ITEM_IMAGE) {
			txt += " ";
		}
	}

	if (!selection.from_line_found) {
		return txt; // Empty.
	}

	int chars = -1; // Unless the current line is the `to` line.
	if (p_frame == selection.to_frame && p_line == selection.to_line) {
		selection.to_line_found = true;

		if (from_this ^ selection.from_line_found) {
			// For cases text from child frame to parent frame is considered to be on the same line,
			// even if the line numbers are different. `start` has been reset to 0.
			chars = selection.to_char - selection.from_char;
		}
		else {
			chars = selection.to_char - start;
		}
	}

	txt = txt.substr(start, chars);

	return txt;
}

void RichTextLabel::set_context_menu_enabled(bool p_enabled) { context_menu_enabled = p_enabled; }

bool RichTextLabel::is_context_menu_enabled() const { return context_menu_enabled; }

void RichTextLabel::set_shortcut_keys_enabled(bool p_enabled) { shortcut_keys_enabled = p_enabled; }

bool RichTextLabel::is_shortcut_keys_enabled() const { return shortcut_keys_enabled; }

PopupMenu* RichTextLabel::get_menu() const
{
	if (!menu) {
		const_cast<RichTextLabel*>(this)->_generate_context_menu();
	}
	return menu;
}

bool RichTextLabel::is_menu_visible() const { return menu && menu->is_visible(); }

void RichTextLabel::deselect()
{
	selection.active = false;
	queue_accessibility_update();
	queue_redraw();
}

void RichTextLabel::select_all()
{
	_validate_line_caches();

	if (!selection.enabled) {
		return;
	}

	Item* it = main;
	Item* from_item = nullptr;
	Item* to_item = nullptr;

	while (it) {
		if (it->type != ITEM_FRAME) {
			if (!from_item) {
				from_item = it;
			}
			to_item = it;
		}
		it = _get_next_item(it, true);
	}
	if (!from_item) {
		return;
	}

	ItemFrame* from_frame = nullptr;
	int from_line = 0;
	_find_frame(from_item, &from_frame, &from_line);
	if (!from_frame) {
		return;
	}
	ItemFrame* to_frame = nullptr;
	int to_line = 0;
	_find_frame(to_item, &to_frame, &to_line);
	if (!to_frame) {
		return;
	}
	selection.from_line = from_line;
	selection.from_frame = from_frame;
	selection.from_char = 0;
	selection.from_item = from_item;
	selection.to_line = to_line;
	selection.to_frame = to_frame;
	selection.to_char = to_frame->lines[to_line].char_count;
	selection.to_item = to_item;
	selection.active = true;
	queue_accessibility_update();
	queue_redraw();
}

bool RichTextLabel::is_selection_enabled() const { return selection.enabled; }

bool RichTextLabel::is_deselect_on_focus_loss_enabled() const
{
	return deselect_on_focus_loss_enabled;
}

void RichTextLabel::set_drag_and_drop_selection_enabled(const bool p_enabled)
{
	drag_and_drop_selection_enabled = p_enabled;
}

bool RichTextLabel::is_drag_and_drop_selection_enabled() const
{
	return drag_and_drop_selection_enabled;
}

int RichTextLabel::get_selection_from() const
{
	if (!selection.active || !selection.enabled) {
		return -1;
	}

	return selection.from_frame->lines[selection.from_line].char_offset + selection.from_char;
}

int RichTextLabel::get_selection_to() const
{
	if (!selection.active || !selection.enabled) {
		return -1;
	}

	return selection.to_frame->lines[selection.to_line].char_offset + selection.to_char - 1;
}

float RichTextLabel::get_selection_line_offset() const
{
	if (selection.active && selection.from_frame && selection.from_line >= 0 &&
		selection.from_line < (int)selection.from_frame->lines.size()) {
		// Selected frame paragraph offset.
		float line_offset = selection.from_frame->lines[selection.from_line].offset.y;

		// Add wrapped line offset.
		for (int i = 0;
			 i < selection.from_frame->lines[selection.from_line].text_buf->get_line_count(); i++) {
			Vector2i range =
				selection.from_frame->lines[selection.from_line].text_buf->get_line_range(i);
			if (range.x <= selection.from_char && range.y >= selection.from_char) {
				break;
			}
			line_offset +=
				selection.from_frame->lines[selection.from_line].text_buf->get_line_ascent(i) +
				selection.from_frame->lines[selection.from_line].text_buf->get_line_descent(i) +
				theme_cache.line_separation;
		}

		// Add nested frame (e.g. table cell) offset.
		ItemFrame* it = selection.from_frame;
		while (it->parent_frame != nullptr) {
			line_offset += it->parent_frame->lines[it->line].offset.y;
			it = it->parent_frame;
		}
		return line_offset;
	}

	return -1.0;
}

void RichTextLabel::set_text(const String& p_bbcode)
{
	// Allow clearing the tag stack.
	if (!p_bbcode.is_empty() && text == p_bbcode) {
		return;
	}

	stack_externally_modified = false;

	text = p_bbcode;
	if (text.is_empty()) {
		clear();
	}
	else {
		_apply_translation();
	}
}

String RichTextLabel::get_text() const { return text; }

bool RichTextLabel::is_using_bbcode() const { return use_bbcode; }

String RichTextLabel::get_parsed_text() const
{
	String txt;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_DROPCAP) {
			ItemDropcap* dc = static_cast<ItemDropcap*>(it);
			txt += dc->text;
		}
		else if (it->type == ITEM_TEXT) {
			ItemText* t = static_cast<ItemText*>(it);
			txt += t->text;
		}
		else if (it->type == ITEM_NEWLINE) {
			txt += "\n";
		}
		else if (it->type == ITEM_IMAGE) {
			txt += " ";
		}
		else if (it->type == ITEM_INDENT || it->type == ITEM_LIST) {
			txt += "\t";
		}
		it = _get_next_item(it, true);
	}
	return txt;
}

void RichTextLabel::set_text_direction(Control::TextDirection p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	_stop_thread();

	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_invalidate_accessibility();
			_validate_line_caches();
		}
		queue_redraw();
	}
}

Control::TextDirection RichTextLabel::get_text_direction() const { return text_direction; }

void RichTextLabel::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	_stop_thread();

	if (default_alignment != p_alignment) {
		default_alignment = p_alignment;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_validate_line_caches();
		}
		queue_redraw();
	}
}

HorizontalAlignment RichTextLabel::get_horizontal_alignment() const { return default_alignment; }

void RichTextLabel::set_vertical_alignment(VerticalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);

	if (vertical_alignment == p_alignment) {
		return;
	}

	vertical_alignment = p_alignment;
	queue_redraw();
}

VerticalAlignment RichTextLabel::get_vertical_alignment() const { return vertical_alignment; }

void RichTextLabel::set_justification_flags(uint32_t p_flags)
{
	_stop_thread();

	if (default_jst_flags != p_flags) {
		default_jst_flags = p_flags;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_validate_line_caches();
		}
		queue_redraw();
	}
}

uint32_t RichTextLabel::get_justification_flags() const { return default_jst_flags; }

void RichTextLabel::set_tab_stops(const PackedFloat32Array& p_tab_stops)
{
	_stop_thread();

	if (default_tab_stops != p_tab_stops) {
		default_tab_stops = p_tab_stops;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_validate_line_caches();
		}
		queue_redraw();
	}
}

PackedFloat32Array RichTextLabel::get_tab_stops() const { return default_tab_stops; }

void RichTextLabel::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		_stop_thread();

		st_parser = p_parser;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_invalidate_accessibility();
			_validate_line_caches();
		}
		queue_redraw();
	}
}

TextServer::StructuredTextParser RichTextLabel::get_structured_text_bidi_override() const
{
	return st_parser;
}

void RichTextLabel::set_language(const String& p_language)
{
	if (language != p_language) {
		_stop_thread();

		language = p_language;
		if (!stack_externally_modified) {
			_apply_translation();
		}
		else {
			main->first_invalid_line.store(0); // Invalidate all lines.
			_invalidate_accessibility();
			_validate_line_caches();
		}
		queue_redraw();
	}
}

String RichTextLabel::get_language() const { return language; }

void RichTextLabel::set_autowrap_mode(TextServer::AutowrapMode p_mode)
{
	if (autowrap_mode != p_mode) {
		_stop_thread();

		autowrap_mode = p_mode;
		main->first_invalid_line = 0; // Invalidate all lines.
		_invalidate_accessibility();
		_validate_line_caches();
		queue_redraw();
		update_minimum_size();
	}
}

TextServer::AutowrapMode RichTextLabel::get_autowrap_mode() const { return autowrap_mode; }

void RichTextLabel::set_autowrap_trim_flags(uint32_t p_flags)
{
	if (autowrap_flags_trim != (p_flags & TextServer::BREAK_TRIM_MASK)) {
		_stop_thread();

		autowrap_flags_trim = p_flags & TextServer::BREAK_TRIM_MASK;
		main->first_invalid_line = 0; // Invalidate all lines.
		_validate_line_caches();
		queue_redraw();
		update_minimum_size();
	}
}

uint32_t RichTextLabel::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

void RichTextLabel::set_visible_ratio(float p_ratio)
{
	if (visible_ratio != p_ratio) {
		_stop_thread();

		int prev_vc = visible_characters;
		if (p_ratio >= 1.0) {
			visible_characters = -1;
			visible_ratio = 1.0;
		}
		else if (p_ratio < 0.0) {
			visible_characters = 0;
			visible_ratio = 0.0;
		}
		else {
			visible_characters = get_total_character_count() * p_ratio;
			visible_ratio = p_ratio;
		}

		if (visible_chars_behavior == TextServer::VC_CHARS_BEFORE_SHAPING &&
			visible_characters != prev_vc) {
			int new_vc =
				(visible_characters < 0) ? get_total_character_count() : visible_characters;
			int old_vc = (prev_vc < 0) ? get_total_character_count() : prev_vc;
			int to_line = main->first_invalid_line.load();
			int old_from_l = to_line;
			int new_from_l = to_line;
			for (int i = 0; i < to_line; i++) {
				const Line& l = main->lines[i];
				if (l.char_offset <= old_vc && l.char_offset + l.char_count > old_vc) {
					old_from_l = i;
				}
				if (l.char_offset <= new_vc && l.char_offset + l.char_count > new_vc) {
					new_from_l = i;
				}
			}
			Rect2 text_rect = _get_text_rect();
			int first_invalid = MIN(new_from_l, old_from_l);
			int second_invalid = MAX(new_from_l, old_from_l);

			float total_height = (first_invalid == 0) ? 0
													  : _calculate_line_vertical_offset(
															main->lines[first_invalid - 1]);
			if (first_invalid < to_line) {
				int total_chars = main->lines[first_invalid].char_offset;
				total_height = _shape_line(main, first_invalid, theme_cache.normal_font,
					theme_cache.normal_font_size, text_rect.get_size().width - scroll_w,
					total_height, &total_chars);
			}
			if (first_invalid != second_invalid) {
				for (int i = first_invalid + 1; i < second_invalid; i++) {
					main->lines[i].offset.y = total_height;
					total_height = _calculate_line_vertical_offset(main->lines[i]);
				}
				if (second_invalid < to_line) {
					int total_chars = main->lines[second_invalid].char_offset;
					total_height = _shape_line(main, second_invalid, theme_cache.normal_font,
						theme_cache.normal_font_size, text_rect.get_size().width - scroll_w,
						total_height, &total_chars);
				}
			}
			for (int i = second_invalid + 1; i < to_line; i++) {
				main->lines[i].offset.y = total_height;
				total_height = _calculate_line_vertical_offset(main->lines[i]);
			}
		}
		_update_follow_vc();
		queue_redraw();
	}
}

float RichTextLabel::get_visible_ratio() const { return visible_ratio; }

int RichTextLabel::get_content_height() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int total_height = 0;
	int to_line = main->first_invalid_line.load();
	if (to_line) {
		MutexLock lock(main->lines[to_line - 1].text_buf->get_mutex());
		if (theme_cache.line_separation < 0) {
			// Do not apply to the last line to avoid cutting text.
			total_height = main->lines[to_line - 1].offset.y +
						   main->lines[to_line - 1].text_buf->get_size().y +
						   (main->lines[to_line - 1].text_buf->get_line_count() - 1) *
							   theme_cache.line_separation;
		}
		else {
			total_height =
				main->lines[to_line - 1].offset.y +
				main->lines[to_line - 1].text_buf->get_size().y +
				main->lines[to_line - 1].text_buf->get_line_count() * theme_cache.line_separation +
				theme_cache.paragraph_separation;
		}
	}
	return total_height;
}

Rect2i RichTextLabel::get_visible_content_rect() const { return visible_rect; }

int RichTextLabel::get_content_width() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int total_width = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		total_width =
			MAX(total_width, main->lines[i].offset.x + main->lines[i].text_buf->get_size().x);
	}
	return total_width;
}

int RichTextLabel::get_line_height(int p_line) const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		int lc = main->lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			const Ref<TextParagraph> text_buf = main->lines[i].text_buf;
			return text_buf->get_line_ascent(p_line - line_count) +
				   text_buf->get_line_descent(p_line - line_count) + theme_cache.line_separation;
		}
		line_count += lc;
	}
	return 0;
}

int RichTextLabel::get_line_width(int p_line) const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		int lc = main->lines[i].text_buf->get_line_count();

		if (p_line < line_count + lc) {
			return main->lines[i].text_buf->get_line_width(p_line - line_count);
		}
		line_count += lc;
	}
	return 0;
}

void RichTextLabel::_maximum_size_changed()
{
	if (!fit_content || autowrap_mode == TextServer::AUTOWRAP_OFF) {
		return;
	}

	_stop_thread();
	main->first_resized_line.store(0); // Invalidate all lines.
	_invalidate_accessibility();
	_validate_line_caches();
	queue_redraw();
	update_minimum_size();
}

void RichTextLabel::_bind_methods() {}

TextServer::VisibleCharactersBehavior RichTextLabel::get_visible_characters_behavior() const
{
	return visible_chars_behavior;
}

void RichTextLabel::set_visible_characters_behavior(
	TextServer::VisibleCharactersBehavior p_behavior)
{
	if (visible_chars_behavior != p_behavior) {
		_stop_thread();

		visible_chars_behavior = p_behavior;
		main->first_invalid_line.store(0); // Invalidate all lines.
		_invalidate_accessibility();
		_validate_line_caches();
		queue_redraw();
	}
}

void RichTextLabel::set_visible_characters(int p_visible)
{
	if (visible_characters != p_visible) {
		_stop_thread();

		int prev_vc = visible_characters;
		visible_characters = p_visible;
		if (p_visible == -1) {
			visible_ratio = 1;
		}
		else {
			int total_char_count = get_total_character_count();
			if (total_char_count > 0) {
				visible_ratio = (float)p_visible / (float)total_char_count;
			}
		}
		if (visible_chars_behavior == TextServer::VC_CHARS_BEFORE_SHAPING &&
			visible_characters != prev_vc) {
			int new_vc =
				(visible_characters < 0) ? get_total_character_count() : visible_characters;
			int old_vc = (prev_vc < 0) ? get_total_character_count() : prev_vc;
			int to_line = main->first_invalid_line.load();
			int old_from_l = to_line;
			int new_from_l = to_line;
			for (int i = 0; i < to_line; i++) {
				const Line& l = main->lines[i];
				if (l.char_offset <= old_vc && l.char_offset + l.char_count > old_vc) {
					old_from_l = i;
				}
				if (l.char_offset <= new_vc && l.char_offset + l.char_count > new_vc) {
					new_from_l = i;
				}
			}
			Rect2 text_rect = _get_text_rect();
			int first_invalid = MIN(new_from_l, old_from_l);
			int second_invalid = MAX(new_from_l, old_from_l);

			float total_height = (first_invalid == 0) ? 0
													  : _calculate_line_vertical_offset(
															main->lines[first_invalid - 1]);
			if (first_invalid < to_line) {
				int total_chars = main->lines[first_invalid].char_offset;
				total_height = _shape_line(main, first_invalid, theme_cache.normal_font,
					theme_cache.normal_font_size, text_rect.get_size().width - scroll_w,
					total_height, &total_chars);
			}
			if (first_invalid != second_invalid) {
				for (int i = first_invalid + 1; i < second_invalid; i++) {
					main->lines[i].offset.y = total_height;
					total_height = _calculate_line_vertical_offset(main->lines[i]);
				}
				if (second_invalid < to_line) {
					int total_chars = main->lines[second_invalid].char_offset;
					total_height = _shape_line(main, second_invalid, theme_cache.normal_font,
						theme_cache.normal_font_size, text_rect.get_size().width - scroll_w,
						total_height, &total_chars);
				}
			}
			for (int i = second_invalid + 1; i < to_line; i++) {
				main->lines[i].offset.y = total_height;
				total_height = _calculate_line_vertical_offset(main->lines[i]);
			}
		}
		_update_follow_vc();
		queue_redraw();
	}
}

int RichTextLabel::get_visible_characters() const { return visible_characters; }

int RichTextLabel::get_character_line(int p_char)
{
	_validate_line_caches();

	int line_count = 0;
	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		MutexLock lock(main->lines[i].text_buf->get_mutex());
		int char_offset = main->lines[i].char_offset;
		int char_count = main->lines[i].char_count;
		if (char_offset <= p_char && p_char < char_offset + char_count) {
			int lc = main->lines[i].text_buf->get_line_count();
			for (int j = 0; j < lc; j++) {
				Vector2i range = main->lines[i].text_buf->get_line_range(j);
				if (char_offset + range.x <= p_char && p_char < char_offset + range.y) {
					break;
				}
				if (char_offset + range.x > p_char && line_count > 0) {
					line_count--; // Character is not rendered and is between the lines (e.g., edge
								  // space).
					break;
				}
				if (j != lc - 1) {
					line_count++;
				}
			}
			return line_count;
		}
		else {
			line_count += main->lines[i].text_buf->get_line_count();
		}
	}
	return -1;
}

int RichTextLabel::get_character_paragraph(int p_char)
{
	_validate_line_caches();

	int to_line = main->first_invalid_line.load();
	for (int i = 0; i < to_line; i++) {
		int char_offset = main->lines[i].char_offset;
		if (char_offset <= p_char && p_char < char_offset + main->lines[i].char_count) {
			return i;
		}
	}
	return -1;
}

int RichTextLabel::get_total_character_count() const
{
	// Note: Do not use line buffer "char_count", it includes only visible characters.
	int tc = 0;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_TEXT) {
			ItemText* t = static_cast<ItemText*>(it);
			tc += t->text.length();
		}
		else if (it->type == ITEM_NEWLINE) {
			tc++;
		}
		else if (it->type == ITEM_IMAGE) {
			tc++;
		}
		it = _get_next_item(it, true);
	}

return tc;
}

int RichTextLabel::get_total_glyph_count() const
{
	const_cast<RichTextLabel*>(this)->_validate_line_caches();

	int tg = 0;
	Item* it = main;
	while (it) {
		if (it->type == ITEM_FRAME) {
			ItemFrame* f = static_cast<ItemFrame*>(it);
			for (int i = 0; i < (int)f->lines.size(); i++) {
				MutexLock lock(f->lines[i].text_buf->get_mutex());
				tg += TS->shaped_text_get_glyph_count(f->lines[i].text_buf->get_rid());
			}
		}
		it = _get_next_item(it, true);
	}

	return tg;
}

Size2 RichTextLabel::get_minimum_size() const
{
	Size2 sb_min_size = theme_cache.normal_style->get_minimum_size();
	Size2 min_size;
	bool wrap_with_max_width =
		autowrap_mode != TextServer::AUTOWRAP_OFF && get_combined_maximum_size().x > 0.0;

	if (fit_content) {
		if (!wrap_with_max_width) {
			min_size.x = get_content_width();
		}
		min_size.y = get_content_height();
	}

	if (wrap_with_max_width) {
		const_cast<RichTextLabel*>(this)->_validate_line_caches();

		int natural_width = 0;
		int to_line = main->first_invalid_line.load();
		for (int i = 0; i < to_line; i++) {
			MutexLock lock(main->lines[i].text_buf->get_mutex());
			natural_width = MAX(natural_width,
				int(Math::ceil(
					main->lines[i].offset.x + main->lines[i].text_buf->get_non_wrapped_size().x)));
		}

		int maximum_width = int(get_combined_maximum_size().x - sb_min_size.width);
		if (maximum_width <= 0) {
			maximum_width = 1;
		}
		min_size.x = MIN(natural_width, maximum_width);
	}

	return sb_min_size +
		   ((autowrap_mode != TextServer::AUTOWRAP_OFF)
				   ? Size2(wrap_with_max_width ? MAX(1, min_size.x) : 1, min_size.height)
				   : min_size);
}

void RichTextLabel::_update_context_menu()
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

	MENU_ITEM_ACTION_DISABLED(menu, MENU_COPY, "ui_copy", !selection.enabled)
	MENU_ITEM_ACTION_DISABLED(menu, MENU_SELECT_ALL, "ui_text_select_all", !selection.enabled)

#undef MENU_ITEM_ACTION_DISABLED
}

Key RichTextLabel::_get_menu_action_accelerator(const String& p_action)
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

	// Use physical keycode if non-zero
	if (event->get_physical_keycode() != Key::NONE) {
		return event->get_physical_keycode_with_modifiers();
	}
	else {
		return event->get_keycode_with_modifiers();
	}
}

void RichTextLabel::menu_option(int p_option)
{
	switch (p_option) {
	case MENU_COPY: {
		String txt = get_selected_text();
		if (txt.is_empty()) {
			txt = get_parsed_text();
		}

		if (!txt.is_empty()) {
			DisplayServer::get_singleton()->clipboard_set(txt);
		}
	} break;
	case MENU_SELECT_ALL: {
		select_all();
	} break;
	}
}

RichTextLabel::~RichTextLabel()
{
	_stop_thread();
	memdelete(main);
}


