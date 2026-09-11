/**************************************************************************/
/*  scroll_container.cpp                                                  */
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
#include "scene/gui/panel_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "scroll_container.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

Size2 ScrollContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	// Calculated in this function, as it needs to traverse all child controls once to calculate;
	// and needs to be calculated before being used by `_update_scrollbars()`.
	largest_child_min_size = Size2();

	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c || c == h_scroll || c == v_scroll || c == focus_panel || c == scroll_hint_top_left ||
			c == scroll_hint_bottom_right) {
			continue;
		}

		Size2 child_min_size =
			p_use_desired_sizes ? c->get_bound_desired_size() : c->get_bound_minimum_size();
		largest_child_min_size = largest_child_min_size.max(child_min_size);
	}

	Size2 min_size;
	const Size2 size = get_size();

	bool v_scroll_show = vertical_scroll_mode == SCROLL_MODE_SHOW_ALWAYS ||
						 vertical_scroll_mode == SCROLL_MODE_RESERVE ||
						 ((vertical_scroll_mode == SCROLL_MODE_AUTO ||
							  vertical_scroll_mode == SCROLL_MODE_MAXIMIZE_FIRST) &&
							 (largest_child_min_size.y > size.y));
	bool h_scroll_show = horizontal_scroll_mode == SCROLL_MODE_SHOW_ALWAYS ||
						 horizontal_scroll_mode == SCROLL_MODE_RESERVE ||
						 ((horizontal_scroll_mode == SCROLL_MODE_AUTO ||
							  horizontal_scroll_mode == SCROLL_MODE_MAXIMIZE_FIRST) &&
							 (largest_child_min_size.x > size.x));

	if (horizontal_scroll_mode == SCROLL_MODE_DISABLED) {
		min_size.x = largest_child_min_size.x;
		if (v_scroll_show && v_scroll->get_parent() == this) {
			min_size.x += v_scroll->get_minimum_size().x + theme_cache.scrollbar_h_separation;
		}
	}
	else if (horizontal_scroll_mode == SCROLL_MODE_MAXIMIZE_FIRST) {
		float h_max_size = get_combined_maximum_size().x;
		min_size.x =
			h_max_size >= 0 ? MIN(largest_child_min_size.x, h_max_size) : largest_child_min_size.x;
		if (v_scroll_show && v_scroll->get_parent() == this) {
			min_size.x += v_scroll->get_minimum_size().x + theme_cache.scrollbar_h_separation;
		}
	}

	if (vertical_scroll_mode == SCROLL_MODE_DISABLED) {
		min_size.y = largest_child_min_size.y;
		if (h_scroll_show && h_scroll->get_parent() == this) {
			min_size.y += h_scroll->get_minimum_size().y + theme_cache.scrollbar_v_separation;
		}
	}
	else if (vertical_scroll_mode == SCROLL_MODE_MAXIMIZE_FIRST) {
		float v_max_size = get_combined_maximum_size().y;
		min_size.y =
			v_max_size >= 0 ? MIN(largest_child_min_size.y, v_max_size) : largest_child_min_size.y;
		if (h_scroll_show && h_scroll->get_parent() == this) {
			min_size.y += h_scroll->get_minimum_size().y + theme_cache.scrollbar_v_separation;
		}
	}

	Rect2 margins = _get_margins();
	min_size += margins.position + margins.size;

	return min_size;
}

Size2 ScrollContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 ScrollContainer::get_desired_size() const { return _get_minimum_size(true); }

Size2 ScrollContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Container::get_inner_combined_maximum_size();

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	if (h_scroll && (_is_h_scroll_visible() || horizontal_scroll_mode == SCROLL_MODE_RESERVE)) {
		ms.y -= h_scroll->get_minimum_size().y + theme_cache.scrollbar_v_separation;
	}
	if (v_scroll && (_is_v_scroll_visible() || vertical_scroll_mode == SCROLL_MODE_RESERVE)) {
		ms.x -= v_scroll->get_minimum_size().x + theme_cache.scrollbar_h_separation;
	}

	return ms;
}

bool ScrollContainer::_is_h_scroll_visible() const
{
	// Scrolls may have been moved out for reasons.
	return h_scroll && h_scroll->is_visible() && h_scroll->get_parent() == this;
}

bool ScrollContainer::_is_v_scroll_visible() const
{
	return v_scroll && v_scroll->is_visible() && v_scroll->get_parent() == this;
}

Rect2 ScrollContainer::_get_margins() const
{
	float right_margin = theme_cache.panel_style->get_margin(SIDE_RIGHT);
	float left_margin = theme_cache.panel_style->get_margin(SIDE_LEFT);
	float top_margin = theme_cache.panel_style->get_margin(SIDE_TOP);
	float bottom_margin = theme_cache.panel_style->get_margin(SIDE_BOTTOM);
	if (draw_focus_border) {
		// Only update margins if the focus style's margins don't fit into the panel style's
		// margins.
		float focus_margin = theme_cache.focus_style->get_margin(SIDE_RIGHT);
		if (focus_margin > right_margin) {
			right_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_LEFT);
		if (focus_margin > left_margin) {
			left_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_TOP);
		if (focus_margin > top_margin) {
			top_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_BOTTOM);
		if (focus_margin > bottom_margin) {
			bottom_margin = focus_margin;
		}
	}

	return Rect2(left_margin, top_margin, right_margin, bottom_margin);
}

Rect2 ScrollContainer::_get_local_visible_rect() const
{
	const float side_margin = v_scroll->is_visible() ? v_scroll->get_size().x : 0.0f;
	const float bottom_margin = h_scroll->is_visible() ? h_scroll->get_size().y : 0.0f;

	Point2 origin = Point2(is_layout_rtl() ? side_margin : 0.0f, 0.0f);
	Size2 size = Size2(get_size().x - side_margin, get_size().y - bottom_margin);

	return Rect2(origin, size);
}

void ScrollContainer::_update_scrollbar_position()
{
	if (!_updating_scrollbars) {
		return;
	}

	Rect2 margins = _get_margins();

	Size2 hmin = h_scroll->is_visible() ? h_scroll->get_bound_minimum_size() : Size2();
	Size2 vmin = v_scroll->is_visible() ? v_scroll->get_bound_minimum_size() : Size2();

	int lmar = is_layout_rtl() ? margins.size.x : margins.position.x;
	int rmar = is_layout_rtl() ? margins.position.x : margins.size.x;

	h_scroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN, lmar);
	h_scroll->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -rmar - vmin.width);
	h_scroll->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -hmin.height - margins.size.y);
	h_scroll->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -margins.size.y);

	v_scroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -vmin.width - rmar);
	v_scroll->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -rmar);
	v_scroll->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, margins.position.y);
	v_scroll->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -hmin.height - margins.size.y);

	_updating_scrollbars = false;
}

void ScrollContainer::_gui_focus_changed(Control* p_control)
{
	if (follow_focus && is_ancestor_of(p_control)) {
		following = true;
		ensure_control_visible(p_control);
		following = false;
	}
	if (draw_focus_border) {
		const bool _should_draw_focus_border = has_focus(true) || child_has_focus();
		if (focus_border_is_drawn != _should_draw_focus_border) {
			queue_redraw();
		}
	}
}

void ScrollContainer::_update_scroll_hints()
{
	Size2 size = get_size();
	Rect2 margins = _get_margins();
	Size2 scroll_size = size - margins.position - margins.size;

	float v_scroll_value = v_scroll->get_value();
	bool v_scroll_below_max =
		v_scroll_value < (largest_child_min_size.height - scroll_size.height - 1);
	bool show_vertical_hints = v_scroll_value > 1 || v_scroll_below_max;

	float h_scroll_value = h_scroll->get_value();
	bool h_scroll_below_max =
		h_scroll_value < (largest_child_min_size.width - scroll_size.width - 1);
	bool show_horizontal_hints = h_scroll_value > 1 || h_scroll_below_max;

	bool rtl = is_layout_rtl();
	if (show_vertical_hints) {
		scroll_hint_top_left->set_texture(theme_cache.scroll_hint_vertical);
		scroll_hint_top_left->set_modulate(theme_cache.scroll_hint_vertical_color);
		scroll_hint_top_left->set_visible(!show_horizontal_hints &&
										  (scroll_hint_mode == SCROLL_HINT_MODE_ALL ||
											  scroll_hint_mode == SCROLL_HINT_MODE_TOP_AND_LEFT) &&
										  v_scroll_value > 1);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN, rtl ? -size.x : 0);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, rtl ? 0 : size.x);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, 0);
		scroll_hint_top_left->set_anchor_and_offset(
			SIDE_BOTTOM, ANCHOR_BEGIN, theme_cache.scroll_hint_vertical->get_height());

		scroll_hint_bottom_right->set_flip_h(false);
		scroll_hint_bottom_right->set_flip_v(true);
		scroll_hint_bottom_right->set_texture(theme_cache.scroll_hint_vertical);
		scroll_hint_bottom_right->set_modulate(theme_cache.scroll_hint_vertical_color);
		scroll_hint_bottom_right->set_visible(
			!show_horizontal_hints &&
			(scroll_hint_mode == SCROLL_HINT_MODE_ALL ||
				scroll_hint_mode == SCROLL_HINT_MODE_BOTTOM_AND_RIGHT) &&
			v_scroll_below_max);
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN, rtl ? -size.x : 0);
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, rtl ? 0 : size.x);
		scroll_hint_bottom_right->set_anchor_and_offset(
			SIDE_TOP, ANCHOR_END, -theme_cache.scroll_hint_vertical->get_height());
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, 0);
	}
	else {
		scroll_hint_top_left->set_texture(theme_cache.scroll_hint_horizontal);
		scroll_hint_top_left->set_modulate(theme_cache.scroll_hint_horizontal_color);
		scroll_hint_top_left->set_visible(
			!show_vertical_hints &&
			(scroll_hint_mode == SCROLL_HINT_MODE_ALL ||
				(rtl ? scroll_hint_mode == SCROLL_HINT_MODE_BOTTOM_AND_RIGHT
					 : scroll_hint_mode == SCROLL_HINT_MODE_TOP_AND_LEFT)) &&
			h_scroll_value > 1);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN,
			rtl ? (size.x - theme_cache.scroll_hint_horizontal->get_width()) : 0);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_BEGIN,
			rtl ? size.x : theme_cache.scroll_hint_horizontal->get_width());
		scroll_hint_top_left->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, 0);
		scroll_hint_top_left->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, 0);

		scroll_hint_bottom_right->set_flip_h(true);
		scroll_hint_bottom_right->set_flip_v(false);
		scroll_hint_bottom_right->set_texture(theme_cache.scroll_hint_horizontal);
		scroll_hint_bottom_right->set_modulate(theme_cache.scroll_hint_horizontal_color);
		scroll_hint_bottom_right->set_visible(
			!show_vertical_hints &&
			(scroll_hint_mode == SCROLL_HINT_MODE_ALL ||
				(rtl ? scroll_hint_mode == SCROLL_HINT_MODE_TOP_AND_LEFT
					 : scroll_hint_mode == SCROLL_HINT_MODE_BOTTOM_AND_RIGHT)) &&
			h_scroll_below_max);
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END,
			rtl ? -size.x : -theme_cache.scroll_hint_horizontal->get_width());
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END,
			rtl ? (-size.x + theme_cache.scroll_hint_horizontal->get_width()) : 0);
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, 0);
		scroll_hint_bottom_right->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, 0);
	}
}

void ScrollContainer::_scroll_moved(float) { queue_sort(); }

void ScrollContainer::set_h_scroll(int p_pos)
{
	h_scroll->set_value(p_pos);
	_cancel_drag();
}

int ScrollContainer::get_h_scroll() const { return h_scroll->get_value(); }

void ScrollContainer::set_v_scroll(int p_pos)
{
	v_scroll->set_value(p_pos);
	_cancel_drag();
}

int ScrollContainer::get_v_scroll() const { return v_scroll->get_value(); }

void ScrollContainer::set_horizontal_custom_step(float p_custom_step)
{
	h_scroll->set_custom_step(p_custom_step);
}

float ScrollContainer::get_horizontal_custom_step() const { return h_scroll->get_custom_step(); }

void ScrollContainer::set_vertical_custom_step(float p_custom_step)
{
	v_scroll->set_custom_step(p_custom_step);
}

float ScrollContainer::get_vertical_custom_step() const { return v_scroll->get_custom_step(); }

void ScrollContainer::set_horizontal_scroll_mode(ScrollMode p_mode)
{
	if (horizontal_scroll_mode == p_mode) {
		return;
	}

	horizontal_scroll_mode = p_mode;
	update_minimum_size();
	queue_sort();
}

ScrollContainer::ScrollMode ScrollContainer::get_horizontal_scroll_mode() const
{
	return horizontal_scroll_mode;
}

void ScrollContainer::set_vertical_scroll_mode(ScrollMode p_mode)
{
	if (vertical_scroll_mode == p_mode) {
		return;
	}

	vertical_scroll_mode = p_mode;
	update_minimum_size();
	queue_sort();
}

ScrollContainer::ScrollMode ScrollContainer::get_vertical_scroll_mode() const
{
	return vertical_scroll_mode;
}

void ScrollContainer::set_scroll_horizontal_by_default(bool p_enable)
{
	scroll_horizontal_by_default = p_enable;
}

bool ScrollContainer::is_scroll_horizontal_by_default() const
{
	return scroll_horizontal_by_default;
}

int ScrollContainer::get_deadzone() const { return deadzone; }

void ScrollContainer::set_deadzone(int p_deadzone) { deadzone = p_deadzone; }

void ScrollContainer::set_scroll_hint_mode(ScrollHintMode p_mode)
{
	if (scroll_hint_mode == p_mode) {
		return;
	}

	scroll_hint_mode = p_mode;
	_update_scroll_hints();
}

ScrollContainer::ScrollHintMode ScrollContainer::get_scroll_hint_mode() const
{
	return scroll_hint_mode;
}

void ScrollContainer::set_tile_scroll_hint(bool p_enable)
{
	if (tile_scroll_hint == p_enable) {
		return;
	}

	scroll_hint_top_left->set_stretch_mode(
		p_enable ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE);
	scroll_hint_bottom_right->set_stretch_mode(
		p_enable ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE);

	tile_scroll_hint = p_enable;
}

bool ScrollContainer::is_scroll_hint_tiled() { return tile_scroll_hint; }

bool ScrollContainer::is_following_focus() const { return follow_focus; }

void ScrollContainer::set_follow_focus(bool p_follow) { follow_focus = p_follow; }

PackedStringArray ScrollContainer::get_configuration_warnings() const
{
	PackedStringArray warnings = Container::get_configuration_warnings();

	int found = 0;

	for (int i = 0; i < get_child_count(); i++) {
		Control* c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c || c == h_scroll || c == v_scroll || c == focus_panel || c == scroll_hint_top_left ||
			c == scroll_hint_bottom_right) {
			continue;
		}

		found++;
	}

	if (found != 1) {
		warnings.push_back(RTR(
			"ScrollContainer is intended to work with a single child control.\nUse a container as "
			"child (VBox, HBox, etc.), or a Control and set the custom minimum size manually."));
	}

	return warnings;
}

void ScrollContainer::set_scroll_on_drag_hover(bool p_scroll) { scroll_on_drag_hover = p_scroll; }

HScrollBar* ScrollContainer::get_h_scroll_bar() { return h_scroll; }

VScrollBar* ScrollContainer::get_v_scroll_bar() { return v_scroll; }

void ScrollContainer::set_draw_focus_border(bool p_draw)
{
	if (draw_focus_border == p_draw) {
		return;
	}
	draw_focus_border = p_draw;
	if (is_ready()) {
		_reposition_children();
	}
}

bool ScrollContainer::get_draw_focus_border() { return draw_focus_border; }

bool ScrollContainer::child_has_focus()
{
	const Control* focus_owner = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
	return focus_owner && focus_owner->has_focus(true) && is_ancestor_of(focus_owner);
}


