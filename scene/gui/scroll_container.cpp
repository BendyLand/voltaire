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

ScrollContainer::ScrollMode ScrollContainer::get_horizontal_scroll_mode() const
{
	return horizontal_scroll_mode;
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


