/**************************************************************************/
/*  tab_bar.cpp                                                           */
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

#include <cfloat> // FLT_MAX
#include "core/input/input.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/timer.h"
#include "scene/main/viewport.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "tab_bar.h"

static inline Color _select_color(const Color& p_override_color, const Color& p_default_color)
{
	return p_override_color.a > 0 ? p_override_color : p_default_color;
}

Size2 TabBar::get_minimum_size() const
{
	Size2 ms;
	Size2 combined_max = get_combined_maximum_size();

	int buttons_size = get_tab_count() > 1 ? theme_cache.decrement_icon->get_width() +
												 theme_cache.increment_icon->get_width()
										   : 0;

	if (tabs.is_empty()) {
		return ms;
	}

	int y_margin = MAX(MAX(MAX(theme_cache.tab_unselected_style->get_minimum_size().height,
							   theme_cache.tab_hovered_style->get_minimum_size().height),
						   theme_cache.tab_selected_style->get_minimum_size().height),
		theme_cache.tab_disabled_style->get_minimum_size().height);
	int max_tab_width = 0;

	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].hidden) {
			continue;
		}

		int ofs = ms.width;

		Ref<StyleBox> style;
		if (tabs[i].disabled) {
			style = theme_cache.tab_disabled_style;
		}
		else if (current == i) {
			style = theme_cache.tab_selected_style;
		}
		else if (hover == i) {
			style = theme_cache.tab_hovered_style;
		}
		else {
			style = theme_cache.tab_unselected_style;
		}
		ms.width += style->get_minimum_size().width;

		if (tabs[i].icon.is_valid()) {
			const Size2 icon_size = _get_tab_icon_size(i);
			ms.height = MAX(ms.height, icon_size.height + y_margin);
			ms.width += icon_size.width + theme_cache.h_separation;
		}

		if (!tabs[i].text.is_empty()) {
			ms.width += tabs[i].size_text + theme_cache.h_separation;
		}
		ms.height = MAX(ms.height, tabs[i].text_buf->get_size().y + y_margin);

		bool close_visible = cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
							 (cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && i == current);

		if (tabs[i].right_button.is_valid()) {
			Ref<Texture2D> rb = tabs[i].right_button;

			if (close_visible) {
				ms.width += theme_cache.button_hl_style->get_minimum_size().width + rb->get_width();
			}
			else {
				ms.width += theme_cache.button_hl_style->get_margin(SIDE_LEFT) + rb->get_width() +
							theme_cache.h_separation;
			}

			ms.height = MAX(ms.height, rb->get_height() + y_margin);
		}

		if (close_visible) {
			ms.width += theme_cache.button_hl_style->get_margin(SIDE_LEFT) +
						theme_cache.close_icon->get_width() + theme_cache.h_separation;

			ms.height = MAX(ms.height, theme_cache.close_icon->get_height() + y_margin);
		}

		if (ms.width - ofs > style->get_minimum_size().width) {
			ms.width -= theme_cache.h_separation;
		}

		if (i < tabs.size() - 1) {
			ms.width += theme_cache.tab_separation;
		}

		if (ms.width - ofs > max_tab_width) {
			max_tab_width = ms.width - ofs;
		}
	}

	if (clip_tabs) {
		ms.width = max_tab_width + buttons_size;
		if (combined_max.width >= 0) {
			ms.width = MIN(ms.width, int(combined_max.width));
		}
	}
	else if (combined_max.width >= 0) {
		ms.width = MIN(ms.width, int(combined_max.width));
	}

	return ms;
}

RID TabBar::get_tab_accessibility_element(int p_tab) const
{
	RID ae = get_accessibility_element();
	ERR_FAIL_COND_V(ae.is_null(), RID());

	const Tab& item = tabs[p_tab];
	if (item.accessibility_item_element.is_null()) {
		item.accessibility_item_element = AccessibilityServer::get_singleton()->create_sub_element(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_TAB);
		item.accessibility_item_dirty = true;
	}
	return item.accessibility_item_element;
}

RID TabBar::get_focused_accessibility_element() const
{
	if (current == -1) {
		return get_accessibility_element();
	}
	else {
		const Tab& item = tabs[current];
		return item.accessibility_item_element;
	}
}

void TabBar::_draw_tab_drop(RID p_canvas_item)
{
	Vector2 size = get_size();
	int x;
	bool rtl = is_layout_rtl();

	int closest_tab = get_closest_tab_idx_to_point(get_local_mouse_position());
	if (closest_tab != -1) {
		Rect2 tab_rect = get_tab_rect(closest_tab);
		x = tab_rect.position.x;

		// Only add the tab_separation if closest tab is not on the edge.
		bool not_leftmost_tab =
			-1 != (rtl ? get_next_available(closest_tab) : get_previous_available(closest_tab));
		bool not_rightmost_tab =
			-1 != (rtl ? get_previous_available(closest_tab) : get_next_available(closest_tab));

		// Calculate midpoint between tabs.
		if (get_local_mouse_position().x > tab_rect.get_center().x) {
			x += tab_rect.size.x;
			if (not_rightmost_tab) {
				x += Math::ceil(0.5f * theme_cache.tab_separation);
			}
		}
		else if (not_leftmost_tab) {
			x -= Math::floor(0.5f * theme_cache.tab_separation);
		}
	}
	else {
		if (rtl ^ (get_local_mouse_position().x < get_tab_rect(0).position.x)) {
			x = get_tab_rect(0).position.x;
			if (rtl) {
				x += get_tab_rect(0).size.width;
			}
		}
		else {
			Rect2 tab_rect = get_tab_rect(get_tab_count() - 1);

			x = tab_rect.position.x;
			if (!rtl) {
				x += tab_rect.size.width;
			}
		}
	}

	theme_cache.drop_mark_icon->draw(p_canvas_item,
		Point2(x - theme_cache.drop_mark_icon->get_width() / 2,
			(size.height - theme_cache.drop_mark_icon->get_height()) / 2),
		theme_cache.drop_mark_color);
}

void TabBar::_draw_tab(Ref<StyleBox>& p_tab_style, const Color& p_font_color,
	const Color& p_icon_color, int p_index, float p_x, bool p_focus)
{
	RID ci = get_canvas_item();
	bool rtl = is_layout_rtl();

	Rect2 sb_rect = Rect2(p_x, 0, tabs[p_index].size_cache, get_size().height);
	if (tab_style_v_flip) {
		draw_set_transform(
			Point2(0.0, p_tab_style->get_draw_rect(sb_rect).size.y), 0.0, Size2(1.0, -1.0));
	}
	p_tab_style->draw(ci, sb_rect);
	if (tab_style_v_flip) {
		draw_set_transform(Point2(), 0.0, Size2(1.0, 1.0));
	}
	if (p_focus) {
		Ref<StyleBox> focus_style = theme_cache.tab_focus_style;
		focus_style->draw(ci, sb_rect);
	}

	p_x += rtl ? tabs[p_index].size_cache - p_tab_style->get_margin(SIDE_LEFT)
			   : p_tab_style->get_margin(SIDE_LEFT);

	Size2i sb_ms = p_tab_style->get_minimum_size();

	// Draw the icon.
	Ref<Texture2D> icon = tabs[p_index].icon;
	if (icon.is_valid()) {
		const Size2 icon_size = _get_tab_icon_size(p_index);
		const Point2 icon_pos = Point2i(rtl ? p_x - icon_size.width : p_x,
			p_tab_style->get_margin(SIDE_TOP) +
				((sb_rect.size.y - sb_ms.y) - icon_size.height) / 2);
		icon->draw_rect(ci, Rect2(icon_pos, icon_size), false, p_icon_color);

		p_x = rtl ? p_x - icon_size.width - theme_cache.h_separation
				  : p_x + icon_size.width + theme_cache.h_separation;
	}

	// Draw the text.
	if (!tabs[p_index].text.is_empty()) {
		Point2i text_pos = Point2i(rtl ? p_x - tabs[p_index].size_text : p_x,
			p_tab_style->get_margin(SIDE_TOP) +
				((sb_rect.size.y - sb_ms.y) - tabs[p_index].text_buf->get_size().y) / 2);

		if (theme_cache.outline_size > 0 && theme_cache.font_outline_color.a > 0) {
			tabs[p_index].text_buf->draw_outline(
				ci, text_pos, theme_cache.outline_size, theme_cache.font_outline_color);
		}
		tabs[p_index].text_buf->draw(ci, text_pos, p_font_color);

		p_x = rtl ? p_x - tabs[p_index].size_text - theme_cache.h_separation
				  : p_x + tabs[p_index].size_text + theme_cache.h_separation;
	}

	// Draw and calculate rect of the right button.
	if (tabs[p_index].right_button.is_valid()) {
		Ref<StyleBox> style = theme_cache.button_hl_style;
		Ref<Texture2D> rb = tabs[p_index].right_button;

		Rect2 rb_rect;
		rb_rect.size = style->get_minimum_size() + rb->get_size();
		rb_rect.position.x = rtl ? p_x - rb_rect.size.width : p_x;
		rb_rect.position.y =
			p_tab_style->get_margin(SIDE_TOP) + ((sb_rect.size.y - sb_ms.y) - (rb_rect.size.y)) / 2;

		tabs.write[p_index].rb_rect = rb_rect;

		if (rb_hover == p_index) {
			if (rb_pressing) {
				theme_cache.button_pressed_style->draw(ci, rb_rect);
			}
			else {
				style->draw(ci, rb_rect);
			}
		}

		rb->draw(ci, Point2i(rb_rect.position.x + style->get_margin(SIDE_LEFT),
						 rb_rect.position.y + style->get_margin(SIDE_TOP)));

		p_x = rtl ? rb_rect.position.x : rb_rect.position.x + rb_rect.size.width;
	}
	else {
		tabs.write[p_index].rb_rect = Rect2();
	}

	// Draw and calculate rect of the close button.
	if (cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
		(cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && p_index == current)) {
		Ref<StyleBox> style = theme_cache.button_hl_style;
		Ref<Texture2D> cb = theme_cache.close_icon;

		Rect2 cb_rect;
		cb_rect.size = style->get_minimum_size() + cb->get_size();
		cb_rect.position.x = rtl ? p_x - cb_rect.size.width : p_x;
		cb_rect.position.y =
			p_tab_style->get_margin(SIDE_TOP) + ((sb_rect.size.y - sb_ms.y) - (cb_rect.size.y)) / 2;

		tabs.write[p_index].cb_rect = cb_rect;

		if (!tabs[p_index].disabled && cb_hover == p_index) {
			if (cb_pressing) {
				theme_cache.button_pressed_style->draw(ci, cb_rect);
			}
			else {
				style->draw(ci, cb_rect);
			}
		}

		cb->draw(ci, Point2i(cb_rect.position.x + style->get_margin(SIDE_LEFT),
						 cb_rect.position.y + style->get_margin(SIDE_TOP)));
	}
	else {
		tabs.write[p_index].cb_rect = Rect2();
	}
}

int TabBar::get_tab_count() const { return tabs.size(); }

int TabBar::get_current_tab() const { return current; }

int TabBar::get_previous_tab() const { return previous; }

int TabBar::get_hovered_tab() const { return hover; }

int TabBar::get_previous_available(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < -1 || p_idx > get_tab_count(), -1);
	const int idx = p_idx == -1 ? get_current_tab() : p_idx;
	const int offset_end = idx + 1;
	for (int i = 1; i < offset_end; i++) {
		int target_tab = idx - i;
		if (target_tab < 0) {
			target_tab += get_tab_count();
		}
		if (!is_tab_disabled(target_tab) && !is_tab_hidden(target_tab)) {
			return target_tab;
		}
	}
	return -1;
}

int TabBar::get_next_available(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < -1 || p_idx > get_tab_count(), -1);
	const int idx = p_idx == -1 ? get_current_tab() : p_idx;
	const int offset_end = get_tab_count() - idx;
	for (int i = 1; i < offset_end; i++) {
		int target_tab = (idx + i) % get_tab_count();
		if (!is_tab_disabled(target_tab) && !is_tab_hidden(target_tab)) {
			return target_tab;
		}
	}
	return -1;
}

bool TabBar::select_previous_available()
{
	const int previous_available = get_previous_available();
	if (previous_available != -1) {
		set_current_tab(previous_available);
	}
	return previous_available != -1;
}

bool TabBar::select_next_available()
{
	const int next_available = get_next_available();
	if (next_available != -1) {
		set_current_tab(next_available);
	}
	return next_available != -1;
}

int TabBar::get_tab_offset() const { return offset; }

bool TabBar::get_offset_buttons_visible() const { return buttons_visible; }

String TabBar::get_tab_title(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].text;
}

void TabBar::set_tab_tooltip(int p_tab, const String& p_tooltip)
{
	ERR_FAIL_INDEX(p_tab, tabs.size());
	tabs.write[p_tab].tooltip = p_tooltip;
	queue_accessibility_update();
}

String TabBar::get_tab_tooltip(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].tooltip;
}

Control::TextDirection TabBar::get_tab_text_direction(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Control::TEXT_DIRECTION_INHERITED);
	return tabs[p_tab].text_direction;
}

String TabBar::get_tab_language(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].language;
}

Ref<Texture2D> TabBar::get_tab_icon(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Ref<Texture2D>());
	return tabs[p_tab].icon;
}

int TabBar::get_tab_icon_max_width(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), 0);
	return tabs[p_tab].icon_max_width;
}

Color TabBar::get_font_color_override(int p_tab, DrawMode p_draw_mode) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Color());
	ERR_FAIL_INDEX_V(p_draw_mode, DrawMode::DRAW_MAX, Color());

	return tabs[p_tab].font_color_overrides[p_draw_mode];
}

bool TabBar::is_tab_disabled(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), false);
	return tabs[p_tab].disabled;
}

bool TabBar::is_tab_hidden(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), false);
	return tabs[p_tab].hidden;
}

Ref<Texture2D> TabBar::get_tab_button_icon(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Ref<Texture2D>());
	return tabs[p_tab].right_button;
}

void TabBar::_update_cache(bool p_update_hover)
{
	if (tabs.is_empty()) {
		buttons_visible = false;
		return;
	}

	Size2 combined_max = get_combined_maximum_size();
	int combined_max_width = combined_max.width >= 0 ? int(combined_max.width) -
														   theme_cache.increment_icon->get_width() -
														   theme_cache.decrement_icon->get_width()
													 : INT_MAX;
	int effective_max_width =
		max_width > 0 ? MIN(max_width, combined_max_width) : combined_max_width;

	int limit =
		combined_max.width > 0 ? MIN(combined_max.width, get_size().width) : get_size().width;
	int limit_minus_buttons =
		limit - theme_cache.increment_icon->get_width() - theme_cache.decrement_icon->get_width();

	int w = 0;

	max_drawn_tab = tabs.size() - 1;

	for (int i = 0; i < tabs.size(); i++) {
		tabs.write[i].text_buf->set_width(-1);
		tabs.write[i].size_text = Math::ceil(tabs[i].text_buf->get_size().x);
		tabs.write[i].size_cache = get_tab_width(i);
		tabs.write[i].accessibility_item_dirty = true;

		tabs.write[i].truncated = effective_max_width > 0 && effective_max_width < INT_MAX &&
								  tabs[i].size_cache > effective_max_width;
		if (tabs[i].truncated) {
			int size_textless = tabs[i].size_cache - tabs[i].size_text;
			int mw = MAX(size_textless, effective_max_width);

			tabs.write[i].size_text = MAX(mw - size_textless, 1);
			tabs.write[i].text_buf->set_width(tabs[i].size_text);
			tabs.write[i].size_cache = size_textless + tabs[i].size_text;
		}

		if (i < offset || i > max_drawn_tab) {
			tabs.write[i].ofs_cache = 0;
			continue;
		}

		tabs.write[i].ofs_cache = w;

		if (tabs[i].hidden) {
			continue;
		}

		w += tabs[i].size_cache;

		// Check if all tabs would fit inside the area.
		if (clip_tabs && i > offset && (w > limit || (offset > 0 && w > limit_minus_buttons))) {
			tabs.write[i].ofs_cache = 0;

			w -= tabs[i].size_cache;
			w -= theme_cache.tab_separation;

			max_drawn_tab = i - 1;

			while (w > limit_minus_buttons && max_drawn_tab > offset) {
				tabs.write[max_drawn_tab].ofs_cache = 0;

				if (!tabs[max_drawn_tab].hidden) {
					w -= tabs[max_drawn_tab].size_cache;
					w -= theme_cache.tab_separation;
				}

				max_drawn_tab--;
			}
		}
		else if (i < tabs.size() - 1) {
			// Only add the tab separation if this isn't the last tab drawn.
			w += theme_cache.tab_separation;
		}
	}

	missing_right = max_drawn_tab < tabs.size() - 1;
	buttons_visible = offset > 0 || missing_right;

	if (tab_alignment == ALIGNMENT_LEFT) {
		if (p_update_hover) {
			_update_hover();
		}
		return;
	}

	if (tab_alignment == ALIGNMENT_CENTER) {
		w = ((buttons_visible ? limit_minus_buttons : limit) - w) / 2;
	}
	else if (tab_alignment == ALIGNMENT_RIGHT) {
		w = (buttons_visible ? limit_minus_buttons : limit) - w;
	}

	for (int i = offset; i <= max_drawn_tab; i++) {
		if (!tabs[i].hidden) {
			tabs.write[i].ofs_cache = w;

			w += tabs[i].size_cache;
			w += theme_cache.tab_separation;
		}
	}

	if (p_update_hover) {
		_update_hover();
	}
}

Size2 TabBar::get_desired_size() const
{
	if (!clip_tabs || tabs.is_empty()) {
		return Size2();
	}
	Size2 combined_max = get_combined_maximum_size();
	if (combined_max.width < 0) {
		return Size2();
	}

	int buttons_size = tabs.size() > 1 ? theme_cache.decrement_icon->get_width() +
											 theme_cache.increment_icon->get_width()
									   : 0;
	int limit = int(combined_max.width);
	int limit_minus_buttons = limit - buttons_size;

	int w = 0;
	bool overflowed = false;

	// First pass: check if all tabs fit without buttons.
	for (int i = offset; i < tabs.size(); i++) {
		if (tabs[i].hidden) {
			continue;
		}

		int next_w = w + tabs[i].size_cache;
		if (i > offset) {
			next_w += theme_cache.tab_separation;
		}

		if (next_w > limit) {
			overflowed = true;
			break;
		}

		w = next_w;
	}

	// If buttons are needed, recompute against the reduced limit.
	if (offset > 0 || overflowed) {
		w = 0;
		for (int i = offset; i < tabs.size(); i++) {
			if (tabs[i].hidden) {
				continue;
			}

			int next_w = w + tabs[i].size_cache;
			if (i > offset) {
				next_w += theme_cache.tab_separation;
			}

			if (next_w > limit_minus_buttons) {
				break;
			}

			w = next_w;
		}
	}

	int desired = (offset > 0 || overflowed) ? w + buttons_size : w;
	desired = MIN(desired, limit);

	return Size2(desired, 0);
}

void TabBar::_hover_switch_timeout() { set_current_tab(hover); }

int TabBar::get_tab_idx_at_point(const Point2& p_point) const
{
	if (tabs.is_empty()) {
		return -1;
	}

	int hover_now = -1;

	for (int i = offset; i <= max_drawn_tab; i++) {
		if (!tabs[i].hidden) {
			Rect2 rect = get_tab_rect(i);
			if (rect.has_point(p_point)) {
				hover_now = i;
			}
		}
	}

	return hover_now;
}

int TabBar::get_closest_tab_idx_to_point(const Point2& p_point) const
{
	if (tabs.is_empty()) {
		return -1;
	}

	int closest_tab = get_tab_idx_at_point(p_point);
	float closest_distance = FLT_MAX;

	// Search along the x-axis since the TabBar is horizontal.
	if (closest_tab == -1) {
		for (int i = offset; i <= max_drawn_tab; i++) {
			if (!tabs[i].hidden) {
				float center = get_tab_rect(i).get_center().x;
				float distance = Math::abs(center - p_point.x);
				if (distance < closest_distance) {
					closest_distance = distance;
					closest_tab = i;
				}
			}
		}
	}

	return closest_tab;
}

TabBar::AlignmentMode TabBar::get_tab_alignment() const { return tab_alignment; }

bool TabBar::get_clip_tabs() const { return clip_tabs; }

void TabBar::set_tab_style_v_flip(bool p_tab_style_v_flip)
{
	tab_style_v_flip = p_tab_style_v_flip;
}

int TabBar::get_tab_width(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, tabs.size(), 0);

	Ref<StyleBox> style;

	if (tabs[p_idx].disabled) {
		style = theme_cache.tab_disabled_style;
	}
	else if (current == p_idx) {
		style = theme_cache.tab_selected_style;
		// Always pick the widest style between hovered and unselected, to avoid an infinite loop
		// when switching tabs with the mouse.
	}
	else if (theme_cache.tab_hovered_style->get_minimum_size().width >
			   theme_cache.tab_unselected_style->get_minimum_size().width) {
		style = theme_cache.tab_hovered_style;
	}
	else {
		style = theme_cache.tab_unselected_style;
	}
	int x = style->get_minimum_size().width;

	if (tabs[p_idx].icon.is_valid()) {
		const Size2 icon_size = _get_tab_icon_size(p_idx);
		x += icon_size.width + theme_cache.h_separation;
	}

	if (!tabs[p_idx].text.is_empty()) {
		x += tabs[p_idx].size_text + theme_cache.h_separation;
	}

	bool close_visible = cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
						 (cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && p_idx == current);

	if (tabs[p_idx].right_button.is_valid()) {
		Ref<StyleBox> btn_style = theme_cache.button_hl_style;
		Ref<Texture2D> rb = tabs[p_idx].right_button;

		if (close_visible) {
			x += btn_style->get_minimum_size().width + rb->get_width();
		}
		else {
			x += btn_style->get_margin(SIDE_LEFT) + rb->get_width() + theme_cache.h_separation;
		}
	}

	if (close_visible) {
		Ref<StyleBox> btn_style = theme_cache.button_hl_style;
		Ref<Texture2D> cb = theme_cache.close_icon;
		x += btn_style->get_margin(SIDE_LEFT) + cb->get_width() + theme_cache.h_separation;
	}

	if (x > style->get_minimum_size().width) {
		x -= theme_cache.h_separation;
	}

	return x;
}

Size2 TabBar::_get_tab_icon_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, tabs.size(), Size2());
	const TabBar::Tab& tab = tabs[p_index];
	Size2 icon_size = tab.icon->get_size();

	int icon_max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		icon_max_width = theme_cache.icon_max_width;
	}
	if (tab.icon_max_width > 0 && (icon_max_width == 0 || tab.icon_max_width < icon_max_width)) {
		icon_max_width = tab.icon_max_width;
	}

	if (icon_max_width > 0 && icon_size.width > icon_max_width) {
		icon_size.height = icon_size.height * icon_max_width / icon_size.width;
		icon_size.width = icon_max_width;
	}

	return icon_size;
}

bool TabBar::_can_deselect() const
{
	if (deselect_enabled) {
		return true;
	}
	// All tabs must be disabled or hidden.
	for (const Tab& tab : tabs) {
		if (!tab.disabled && !tab.hidden) {
			return false;
		}
	}
	return true;
}

Rect2 TabBar::get_tab_rect(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Rect2());

	if (is_layout_rtl()) {
		return Rect2(get_size().width - tabs[p_tab].ofs_cache - tabs[p_tab].size_cache, 0,
			tabs[p_tab].size_cache, get_size().height);
	}
	else {
		return Rect2(tabs[p_tab].ofs_cache, 0, tabs[p_tab].size_cache, get_size().height);
	}
}

void TabBar::set_close_with_middle_mouse(bool p_scroll_close)
{
	close_with_middle_mouse = p_scroll_close;
}

bool TabBar::get_close_with_middle_mouse() const { return close_with_middle_mouse; }

TabBar::CloseButtonDisplayPolicy TabBar::get_tab_close_display_policy() const
{
	return cb_displaypolicy;
}

int TabBar::get_max_tab_width() const { return max_width; }

void TabBar::set_scrolling_enabled(bool p_enabled) { scrolling_enabled = p_enabled; }

bool TabBar::get_scrolling_enabled() const { return scrolling_enabled; }

void TabBar::set_drag_to_rearrange_enabled(bool p_enabled)
{
	drag_to_rearrange_enabled = p_enabled;
}

bool TabBar::get_drag_to_rearrange_enabled() const { return drag_to_rearrange_enabled; }

void TabBar::set_tabs_rearrange_group(int p_group_id) { tabs_rearrange_group = p_group_id; }

int TabBar::get_tabs_rearrange_group() const { return tabs_rearrange_group; }

void TabBar::set_scroll_to_selected(bool p_enabled)
{
	scroll_to_selected = p_enabled;
	if (p_enabled) {
		ensure_tab_visible(current);
	}
}

bool TabBar::get_scroll_to_selected() const { return scroll_to_selected; }

void TabBar::set_switch_on_drag_hover(bool p_enabled) { switch_on_drag_hover = p_enabled; }

bool TabBar::get_switch_on_drag_hover() const { return switch_on_drag_hover; }

void TabBar::set_select_with_rmb(bool p_enabled) { select_with_rmb = p_enabled; }

bool TabBar::get_select_with_rmb() const { return select_with_rmb; }

void TabBar::set_deselect_enabled(bool p_enabled)
{
	if (deselect_enabled == p_enabled) {
		return;
	}
	deselect_enabled = p_enabled;
	if (!deselect_enabled && current == -1 && !tabs.is_empty()) {
		select_next_available();
	}
}

bool TabBar::get_deselect_enabled() const { return deselect_enabled; }


