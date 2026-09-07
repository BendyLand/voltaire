/**************************************************************************/
/*  item_list.cpp                                                         */
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
#include "core/os/os.h"
#include "item_list.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"

void ItemList::set_item_text(int p_idx, const String& p_text)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].text == p_text) {
		return;
	}

	items.write[p_idx].text = p_text;
	items.write[p_idx].xl_text = _atr(p_idx, p_text);
	_shape_text(p_idx);
	queue_accessibility_update();
	queue_redraw();
	shape_changed = true;
}

String ItemList::get_item_text(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].text;
}

void ItemList::set_item_text_direction(int p_idx, Control::TextDirection p_text_direction)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (items[p_idx].text_direction != p_text_direction) {
		items.write[p_idx].text_direction = p_text_direction;
		_shape_text(p_idx);
		queue_accessibility_update();
		queue_redraw();
	}
}

Control::TextDirection ItemList::get_item_text_direction(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), TEXT_DIRECTION_INHERITED);
	return items[p_idx].text_direction;
}

void ItemList::set_item_language(int p_idx, const String& p_language)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].language != p_language) {
		items.write[p_idx].language = p_language;
		_shape_text(p_idx);
		queue_accessibility_update();
		queue_redraw();
	}
}

String ItemList::get_item_language(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].language;
}

void ItemList::set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].auto_translate_mode != p_mode) {
		items.write[p_idx].auto_translate_mode = p_mode;
		items.write[p_idx].xl_text = _atr(p_idx, items[p_idx].text);
		_shape_text(p_idx);
		queue_accessibility_update();
		queue_redraw();
	}
}

Node::AutoTranslateMode ItemList::get_item_auto_translate_mode(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), AUTO_TRANSLATE_MODE_INHERIT);
	return items[p_idx].auto_translate_mode;
}

void ItemList::set_item_tooltip_enabled(int p_idx, const bool p_enabled)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].tooltip_enabled != p_enabled) {
		items.write[p_idx].tooltip_enabled = p_enabled;
		items.write[p_idx].accessibility_item_dirty = true;
		queue_accessibility_update();
	}
}

bool ItemList::is_item_tooltip_enabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].tooltip_enabled;
}

void ItemList::set_item_tooltip(int p_idx, const String& p_tooltip)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].tooltip == p_tooltip) {
		return;
	}

	items.write[p_idx].tooltip = p_tooltip;
	queue_accessibility_update();
	queue_redraw();
	shape_changed = true;
}

String ItemList::get_item_tooltip(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), String());
	return items[p_idx].tooltip;
}

Ref<Texture2D> ItemList::get_item_icon(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());

	return items[p_idx].icon;
}

void ItemList::set_item_icon_transposed(int p_idx, const bool p_transposed)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon_transposed == p_transposed) {
		return;
	}

	items.write[p_idx].icon_transposed = p_transposed;
	queue_redraw();
	shape_changed = true;
}

bool ItemList::is_item_icon_transposed(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].icon_transposed;
}

void ItemList::set_item_icon_region(int p_idx, const Rect2& p_region)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon_region == p_region) {
		return;
	}

	items.write[p_idx].icon_region = p_region;
	queue_redraw();
	shape_changed = true;
}

Rect2 ItemList::get_item_icon_region(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	return items[p_idx].icon_region;
}

void ItemList::set_item_icon_modulate(int p_idx, const Color& p_modulate)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].icon_modulate == p_modulate) {
		return;
	}

	items.write[p_idx].icon_modulate = p_modulate;
	queue_redraw();
}

Color ItemList::get_item_icon_modulate(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].icon_modulate;
}

void ItemList::set_item_custom_bg_color(int p_idx, const Color& p_custom_bg_color)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].custom_bg == p_custom_bg_color) {
		return;
	}

	items.write[p_idx].custom_bg = p_custom_bg_color;
	queue_redraw();
}

Color ItemList::get_item_custom_bg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_bg;
}

void ItemList::set_item_custom_fg_color(int p_idx, const Color& p_custom_fg_color)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].custom_fg == p_custom_fg_color) {
		return;
	}

	items.write[p_idx].custom_fg = p_custom_fg_color;
	queue_redraw();
}

Color ItemList::get_item_custom_fg_color(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Color());

	return items[p_idx].custom_fg;
}

Rect2 ItemList::get_item_rect(int p_idx, bool p_expand) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), Rect2());

	Rect2 ret = items[p_idx].rect_cache;
	if (p_expand && p_idx % current_columns == current_columns - 1) {
		int width = get_size().width - theme_cache.panel_style->get_minimum_size().width;
		if (scroll_bar_v->is_visible()) {
			width -=
				scroll_bar_v->get_bound_minimum_size().width + theme_cache.scroll_bar_h_separation;
		}
		ret.size.width = width - ret.position.x;
	}
	ret.position += theme_cache.panel_style->get_offset();
	return ret;
}

void ItemList::set_item_tag_icon(int p_idx, const Ref<Texture2D>& p_tag_icon)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].tag_icon == p_tag_icon) {
		return;
	}

	items.write[p_idx].tag_icon = p_tag_icon;
	queue_redraw();
	shape_changed = true;
}

void ItemList::set_item_selectable(int p_idx, bool p_selectable)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	items.write[p_idx].selectable = p_selectable;
	items.write[p_idx].accessibility_item_dirty = true;
	queue_accessibility_update();
}

bool ItemList::is_item_selectable(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].selectable;
}

void ItemList::set_item_disabled(int p_idx, bool p_disabled)
{
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].disabled == p_disabled) {
		return;
	}

	items.write[p_idx].disabled = p_disabled;
	items.write[p_idx].accessibility_item_dirty = true;
	queue_accessibility_update();
	queue_redraw();
}

bool ItemList::is_item_disabled(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

void ItemList::select(int p_idx, bool p_single)
{
	ERR_FAIL_INDEX(p_idx, items.size());

	if (p_single || select_mode == SELECT_SINGLE) {
		if (!items[p_idx].selectable || items[p_idx].disabled) {
			return;
		}

		for (int i = 0; i < items.size(); i++) {
			if (items.write[i].selected != (p_idx == i)) {
				items.write[i].selected = (p_idx == i);
				items.write[i].accessibility_item_dirty = true;
			}
		}

		current = p_idx;
		ensure_selected_visible = false;
	}
	else {
		if (items[p_idx].selectable && !items[p_idx].disabled) {
			items.write[p_idx].selected = true;
			items.write[p_idx].accessibility_item_dirty = true;
		}
	}
	queue_accessibility_update();
	queue_redraw();
}

void ItemList::deselect(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, items.size());

	if (select_mode == SELECT_SINGLE) {
		items.write[p_idx].selected = false;
		current = -1;
	}
	else {
		items.write[p_idx].selected = false;
	}
	items.write[p_idx].accessibility_item_dirty = true;
	queue_accessibility_update();
	queue_redraw();
}

void ItemList::deselect_all()
{
	if (items.is_empty()) {
		return;
	}

	for (int i = 0; i < items.size(); i++) {
		if (items.write[i].selected) {
			items.write[i].selected = false;
			items.write[i].accessibility_item_dirty = true;
		}
	}
	current = -1;
	queue_accessibility_update();
	queue_redraw();
}

bool ItemList::is_selected(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);

	return items[p_idx].selected;
}

void ItemList::set_current(int p_current)
{
	ERR_FAIL_INDEX(p_current, items.size());

	if (current == p_current) {
		return;
	}

	if (select_mode == SELECT_SINGLE) {
		select(p_current, true);
	}
	else {
		current = p_current;
		queue_accessibility_update();
		queue_redraw();
	}
}

int ItemList::get_current() const { return current; }

int ItemList::get_item_count() const { return items.size(); }

void ItemList::set_fixed_column_width(int p_size)
{
	ERR_FAIL_COND(p_size < 0);

	if (fixed_column_width == p_size) {
		return;
	}

	fixed_column_width = p_size;
	queue_redraw();
	shape_changed = true;
}

int ItemList::get_fixed_column_width() const { return fixed_column_width; }

void ItemList::set_same_column_width(bool p_enable)
{
	if (same_column_width == p_enable) {
		return;
	}

	same_column_width = p_enable;
	queue_redraw();
	shape_changed = true;
}

bool ItemList::is_same_column_width() const { return same_column_width; }

void ItemList::set_max_text_lines(int p_lines)
{
	ERR_FAIL_COND(p_lines < 1);
	if (max_text_lines != p_lines) {
		max_text_lines = p_lines;
		for (int i = 0; i < items.size(); i++) {
			if (icon_mode == ICON_MODE_TOP && max_text_lines > 0) {
				items.write[i].text_buf->set_break_flags(
					TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND |
					TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES |
					TextServer::BREAK_TRIM_END_EDGE_SPACES);
				items.write[i].text_buf->set_max_lines_visible(p_lines);
			}
			else {
				items.write[i].text_buf->set_break_flags(TextServer::BREAK_NONE);
			}
		}
		shape_changed = true;
		queue_accessibility_update();
		queue_redraw();
	}
}

int ItemList::get_max_text_lines() const { return max_text_lines; }

void ItemList::set_max_columns(int p_amount)
{
	ERR_FAIL_COND(p_amount < 0);

	if (max_columns == p_amount) {
		return;
	}

	max_columns = p_amount;
	queue_accessibility_update();
	queue_redraw();
	shape_changed = true;
}

int ItemList::get_max_columns() const { return max_columns; }

void ItemList::set_select_mode(SelectMode p_mode)
{
	if (select_mode == p_mode) {
		return;
	}

	select_mode = p_mode;
	queue_accessibility_update();
	queue_redraw();
}

ItemList::SelectMode ItemList::get_select_mode() const { return select_mode; }

void ItemList::set_icon_mode(IconMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 2);
	if (icon_mode != p_mode) {
		icon_mode = p_mode;
		for (int i = 0; i < items.size(); i++) {
			if (icon_mode == ICON_MODE_TOP && max_text_lines > 0) {
				items.write[i].text_buf->set_break_flags(
					TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND |
					TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES |
					TextServer::BREAK_TRIM_END_EDGE_SPACES);
			}
			else {
				items.write[i].text_buf->set_break_flags(TextServer::BREAK_NONE);
			}
		}
		shape_changed = true;
		queue_redraw();
	}
}

ItemList::IconMode ItemList::get_icon_mode() const { return icon_mode; }

void ItemList::set_fixed_icon_size(const Size2i& p_size)
{
	if (fixed_icon_size == p_size) {
		return;
	}

	fixed_icon_size = p_size;
	queue_redraw();
	shape_changed = true;
}

Size2i ItemList::get_fixed_icon_size() const { return fixed_icon_size; }

Size2 ItemList::Item::get_icon_size() const
{
	if (icon.is_null()) {
		return Size2();
	}

	Size2 size_result = Size2(icon_region.size).abs();
	if (icon_region.size.x == 0 || icon_region.size.y == 0) {
		size_result = icon->get_size();
	}

	if (icon_transposed) {
		Size2 size_tmp = size_result;
		size_result.x = size_tmp.y;
		size_result.y = size_tmp.x;
	}

	return size_result;
}

void ItemList::set_fixed_tag_icon_size(const Size2i& p_size)
{
	if (fixed_tag_icon_size == p_size) {
		return;
	}

	fixed_tag_icon_size = p_size;
	queue_redraw();
	shape_changed = true;
}

void ItemList::ensure_current_is_visible()
{
	ensure_selected_visible = true;
	queue_redraw();
}

void ItemList::center_on_current(bool p_center_verically, bool p_center_horizontally)
{
	if (current < 0 || current >= items.size()) {
		return;
	}

	ERR_FAIL_COND_MSG(!p_center_verically && !p_center_horizontally,
		"At least one of the parameters must be true.");

	Rect2 r = items[current].rect_cache;

	if (p_center_verically) {
		int from_v = scroll_bar_v->get_value();
		int to_v = from_v + scroll_bar_v->get_page();
		int item_center_y = r.position.y + r.size.y / 2;
		int viewport_center_y = (from_v + to_v) / 2;

		int offset_y = item_center_y - viewport_center_y;
		scroll_bar_v->set_value(scroll_bar_v->get_value() + offset_y);
	}

	if (p_center_horizontally) {
		int from_h = scroll_bar_h->get_value();
		int to_h = from_h + scroll_bar_h->get_page();
		int item_center_x = r.position.x + r.size.x / 2;
		int viewport_center_x = (from_h + to_h) / 2;

		int offset_x = item_center_x - viewport_center_x;
		scroll_bar_h->set_value(scroll_bar_h->get_value() + offset_x);
	}
}

static Rect2 _adjust_to_max_size(Size2 p_size, Size2 p_max_size)
{
	Size2 size = p_max_size;
	int tex_width = p_size.width * size.height / p_size.height;
	int tex_height = size.height;

	if (tex_width > size.width) {
		tex_width = size.width;
		tex_height = p_size.height * tex_width / p_size.width;
	}

	int ofs_x = (size.width - tex_width) / 2;
	int ofs_y = (size.height - tex_height) / 2;

	return Rect2(ofs_x, ofs_y, tex_width, tex_height);
}

RID ItemList::get_focused_accessibility_element() const
{
	if (current == -1) {
		return get_accessibility_element();
	}
	else {
		const Item& item = items[current];
		return item.accessibility_item_element;
	}
}

void ItemList::force_update_list_size()
{
	if (!shape_changed) {
		return;
	}

	int scroll_bar_v_minwidth = scroll_bar_v->get_minimum_size().x;
	Size2 size = get_size();
	float max_column_width = 0.0;

	// 1- compute item minimum sizes
	for (int i = 0; i < items.size(); i++) {
		Size2 minsize;
		if (items[i].icon.is_valid()) {
			if (fixed_icon_size.x > 0 && fixed_icon_size.y > 0) {
				minsize = fixed_icon_size * icon_scale;
			}
			else {
				minsize = items[i].get_icon_size() * icon_scale;
			}

			if (!items[i].text.is_empty()) {
				if (icon_mode == ICON_MODE_TOP) {
					minsize.y += theme_cache.icon_margin;
				}
				else {
					minsize.x += theme_cache.icon_margin;
				}
			}
		}

		if (!items[i].text.is_empty()) {
			int max_width = -1;
			if (fixed_column_width) {
				max_width = fixed_column_width;
			}
			items.write[i].text_buf->set_width(max_width);
			Size2 s = items[i].text_buf->get_size();

			if (icon_mode == ICON_MODE_TOP) {
				minsize.x = MAX(minsize.x, s.width);
				if (max_text_lines > 0) {
					minsize.y += s.height + theme_cache.line_separation * max_text_lines;
				}
				else {
					minsize.y += s.height;
				}

			}
			else {
				minsize.y = MAX(minsize.y, s.height);
				minsize.x += s.width;
			}
		}

		if (fixed_column_width > 0) {
			minsize.x = fixed_column_width;
		}
		max_column_width = MAX(max_column_width, minsize.x);

		// Elements need to adapt to the selected size.
		minsize.y += MAX(theme_cache.v_separation, 0);
		minsize.x += MAX(theme_cache.h_separation, 0);

		items.write[i].rect_cache.size = minsize;
		items.write[i].min_rect_cache.size = minsize;

		items.write[i].accessibility_item_dirty = true;
	}

	int fit_size = size.x - theme_cache.panel_style->get_minimum_size().width;
	if (!wraparound_items) {
		fit_size += (scroll_bar_h->get_max() - scroll_bar_h->get_page());
	}

	// 2-attempt best fit
	current_columns = 0x7FFFFFFF;
	if (max_columns > 0) {
		current_columns = max_columns;
	}

	// Repeat until all items fit.
	while (true) {
		bool all_fit = true;
		Vector2 ofs;
		int col = 0;
		int max_w = 0;
		int max_h = 0;

		separators.clear();

		for (int i = 0; i < items.size(); i++) {
			if (current_columns > 1 && items[i].rect_cache.size.width + ofs.x > fit_size &&
				!auto_width && wraparound_items) {
				// Went past.
				current_columns = MAX(col, 1);
				all_fit = false;
				break;
			}

			if (same_column_width) {
				items.write[i].rect_cache.size.x =
					max_column_width + MAX(theme_cache.h_separation, 0);
			}
			items.write[i].rect_cache.position = ofs;

			max_h = MAX(max_h, items[i].rect_cache.size.y);
			ofs.x += items[i].rect_cache.size.x;
			max_w = MAX(max_w, ofs.x);

			items.write[i].column = col;
			col++;
			if (col == current_columns) {
				if (i < items.size() - 1) {
					separators.push_back(ofs.y + max_h);
				}

				for (int j = i; j >= 0 && col > 0; j--, col--) {
					items.write[j].rect_cache.size.y = max_h;
				}

				ofs.x = 0;
				ofs.y += max_h;
				col = 0;
				max_h = 0;
			}
		}

		float scroll_bar_v_page =
			MAX(0, size.height - theme_cache.panel_style->get_minimum_size().height);
		float scroll_bar_v_max = MAX(scroll_bar_v_page, ofs.y + max_h);
		float scroll_bar_h_page =
			MAX(0, size.width - theme_cache.panel_style->get_minimum_size().width);
		float scroll_bar_h_max = 0;
		if (!wraparound_items) {
			scroll_bar_h_max = MAX(scroll_bar_h_page, max_w);
		}

		if (scroll_bar_v_page >= scroll_bar_v_max || is_layout_rtl()) {
			fit_size -= scroll_bar_v_minwidth;
		}

		if (all_fit) {
			for (int j = items.size() - 1; j >= 0 && col > 0; j--, col--) {
				items.write[j].rect_cache.size.y = max_h;
			}

			if (auto_height) {
				auto_height_value =
					ofs.y + max_h + theme_cache.panel_style->get_minimum_size().height;
			}
			if (auto_width) {
				auto_width_value = max_w + theme_cache.panel_style->get_minimum_size().width;
			}
			scroll_bar_v->set_max(scroll_bar_v_max);
			scroll_bar_v->set_page(scroll_bar_v_page);
			if (scroll_bar_v_max <= scroll_bar_v_page) {
				scroll_bar_v->set_value(0);
				scroll_bar_v->hide();
			}
			else {
				auto_width_value += scroll_bar_v_minwidth;
				scroll_bar_v->show();

				if (do_autoscroll_to_bottom) {
					scroll_bar_v->set_value(scroll_bar_v_max);
				}
			}

			if (is_layout_rtl() && !wraparound_items) {
				scroll_bar_h->set_max(scroll_bar_h_page);
				scroll_bar_h->set_min(-(scroll_bar_h_max - scroll_bar_h_page));
			}
			else {
				scroll_bar_h->set_max(scroll_bar_h_max);
				scroll_bar_h->set_min(0);
			}
			scroll_bar_h->set_page(scroll_bar_h_page);
			if (scroll_bar_h_max <= scroll_bar_h_page) {
				scroll_bar_h->set_value(0);
				scroll_bar_h->hide();
			}
			else {
				auto_height_value += scroll_bar_h->get_minimum_size().y;
				scroll_bar_h->show();
			}
			break;
		}
	}

	update_minimum_size();
	shape_changed = false;
}

void ItemList::_scroll_changed(double) { queue_redraw(); }

void ItemList::_mouse_exited()
{
	if (hovered > -1) {
		prev_hovered = hovered;
		hovered = -1;
		queue_accessibility_update();
		queue_redraw();
	}
}

int ItemList::get_item_at_position(const Point2& p_pos, bool p_exact) const
{
	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();
	pos.x += scroll_bar_h->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x - scroll_bar_h->get_value() -
				theme_cache.panel_style->get_margin(SIDE_LEFT) -
				theme_cache.panel_style->get_margin(SIDE_RIGHT);
	}

	int closest = -1;
	int closest_dist = 0x7FFFFFFF;

	for (int i = 0; i < items.size(); i++) {
		Rect2 rc = items[i].rect_cache;

		if (i % current_columns ==
			current_columns -
				1) { // Make sure you can still select the last item when clicking past the column.
			if (is_layout_rtl()) {
				rc.size.width = get_size().width - scroll_bar_h->get_value() + rc.position.x;
			}
			else {
				rc.size.width = get_size().width + scroll_bar_h->get_value() - rc.position.x;
			}
		}

		if (rc.size.x < 0) {
			continue; // Skip negative item sizes, because they are off screen.
		}

		if (rc.has_point(pos)) {
			closest = i;

			break;
		}

		float dist = rc.distance_to(pos);
		if (!p_exact && dist < closest_dist) {
			closest = i;
			closest_dist = dist;
		}
	}

	return closest;
}

bool ItemList::is_pos_at_end_of_items(const Point2& p_pos) const
{
	if (items.is_empty()) {
		return true;
	}

	Vector2 pos = p_pos;
	pos -= theme_cache.panel_style->get_offset();
	pos.y += scroll_bar_v->get_value();

	if (is_layout_rtl()) {
		pos.x = get_size().width - pos.x;
	}

	Rect2 endrect = items[items.size() - 1].rect_cache;
	return (pos.y > endrect.position.y + endrect.size.y);
}

Node::AutoTranslateMode ItemList::get_tooltip_auto_translate_mode_at(const Point2& p_at) const
{
	int closest = get_item_at_position(p_at, true);
	if (closest != -1) {
		return items[closest].auto_translate_mode;
	}
	return Control::get_tooltip_auto_translate_mode_at(p_at);
}

void ItemList::sort_items_by_text()
{
	items.sort();
	queue_accessibility_update();
	queue_redraw();
	shape_changed = true;

	if (select_mode == SELECT_SINGLE) {
		for (int i = 0; i < items.size(); i++) {
			if (items[i].selected) {
				select(i);
				return;
			}
		}
	}
}

void ItemList::set_allow_rmb_select(bool p_allow) { allow_rmb_select = p_allow; }

bool ItemList::get_allow_rmb_select() const { return allow_rmb_select; }

void ItemList::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool ItemList::get_allow_reselect() const { return allow_reselect; }

void ItemList::set_allow_search(bool p_allow) { allow_search = p_allow; }

bool ItemList::get_allow_search() const { return allow_search; }

void ItemList::set_icon_scale(real_t p_scale)
{
	ERR_FAIL_COND(!Math::is_finite(p_scale));

	if (icon_scale == p_scale) {
		return;
	}

	icon_scale = p_scale;
	queue_redraw();
	shape_changed = true;
}

real_t ItemList::get_icon_scale() const { return icon_scale; }

Vector<int> ItemList::get_selected_items()
{
	Vector<int> selected;
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			selected.push_back(i);
			if (select_mode == SELECT_SINGLE) {
				break;
			}
		}
	}
	return selected;
}

bool ItemList::is_anything_selected()
{
	for (int i = 0; i < items.size(); i++) {
		if (items[i].selected) {
			return true;
		}
	}

	return false;
}

Size2 ItemList::get_minimum_size() const
{
	Size2 min_size;
	if (auto_width) {
		min_size.x = auto_width_value;
	}

	if (auto_height) {
		min_size.y = auto_height_value;
	}
	return min_size;
}

void ItemList::set_autoscroll_to_bottom(const bool p_enable) { do_autoscroll_to_bottom = p_enable; }

void ItemList::set_auto_width(bool p_enable)
{
	if (auto_width == p_enable) {
		return;
	}

	auto_width = p_enable;
	shape_changed = true;
	queue_accessibility_update();
	queue_redraw();
}

bool ItemList::has_auto_width() const { return auto_width; }

void ItemList::set_auto_height(bool p_enable)
{
	if (auto_height == p_enable) {
		return;
	}

	auto_height = p_enable;
	shape_changed = true;
	queue_accessibility_update();
	queue_redraw();
}

bool ItemList::has_auto_height() const { return auto_height; }

void ItemList::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	if (text_overrun_behavior != p_behavior) {
		text_overrun_behavior = p_behavior;
		for (int i = 0; i < items.size(); i++) {
			items.write[i].text_buf->set_text_overrun_behavior(p_behavior);
		}
		shape_changed = true;
		queue_redraw();
	}
}

TextServer::OverrunBehavior ItemList::get_text_overrun_behavior() const
{
	return text_overrun_behavior;
}

void ItemList::set_wraparound_items(bool p_enable)
{
	if (wraparound_items == p_enable) {
		return;
	}

	wraparound_items = p_enable;
	shape_changed = true;
	queue_redraw();
}

bool ItemList::has_wraparound_items() const { return wraparound_items; }

void ItemList::set_scroll_hint_mode(ScrollHintMode p_mode)
{
	if (scroll_hint_mode == p_mode) {
		return;
	}

	scroll_hint_mode = p_mode;
	queue_redraw();
}

ItemList::ScrollHintMode ItemList::get_scroll_hint_mode() const { return scroll_hint_mode; }

void ItemList::set_tile_scroll_hint(bool p_enable)
{
	if (tile_scroll_hint == p_enable) {
		return;
	}

	tile_scroll_hint = p_enable;
	queue_redraw();
}

bool ItemList::is_scroll_hint_tiled() { return tile_scroll_hint; }

ItemList::~ItemList() {}


