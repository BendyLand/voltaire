/**************************************************************************/
/*  scroll_bar.cpp                                                        */
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

#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "scroll_bar.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

bool ScrollBar::focus_by_default = false;

void ScrollBar::set_can_focus_by_default(bool p_can_focus) { focus_by_default = p_can_focus; }

double ScrollBar::get_grabber_min_size() const
{
	Ref<StyleBox> grabber = theme_cache.grabber_style;
	Size2 gminsize = grabber->get_minimum_size();
	return (orientation == VERTICAL) ? gminsize.height : gminsize.width;
}

double ScrollBar::get_grabber_size() const
{
	float range = get_max() - get_min();
	if (range <= 0) {
		return 0;
	}

	float page = (get_page() > 0) ? get_page() : 0;
	double area_size = get_area_size();
	double grabber_size = page / range * area_size;
	return grabber_size + get_grabber_min_size();
}

double ScrollBar::get_area_size() const
{
	switch (orientation) {
	case VERTICAL: {
		double area = get_size().height;
		area -= theme_cache.scroll_style->get_minimum_size().height;
		area -= theme_cache.increment_icon->get_height();
		area -= theme_cache.decrement_icon->get_height();
		area -= get_grabber_min_size();
		return area;
	} break;
	case HORIZONTAL: {
		double area = get_size().width;
		area -= theme_cache.scroll_style->get_minimum_size().width;
		area -= theme_cache.increment_icon->get_width();
		area -= theme_cache.decrement_icon->get_width();
		area -= get_grabber_min_size();
		return area;
	} break;
	default: {
		return 0.0;
	}
	}
}

double ScrollBar::get_grabber_offset() const { return get_area_size() * get_as_ratio(); }

Size2 ScrollBar::get_minimum_size() const
{
	Ref<Texture2D> incr = theme_cache.increment_icon;
	Ref<Texture2D> decr = theme_cache.decrement_icon;
	Ref<StyleBox> bg = theme_cache.scroll_style;
	Size2 minsize;

	if (orientation == VERTICAL) {
		int padding_left = MAX(theme_cache.padding_left, 0);
		int padding_right = MAX(theme_cache.padding_right, 0);
		minsize.width = MAX(incr->get_size().width, bg->get_minimum_size().width);
		minsize.width += padding_left + padding_right;
		minsize.height += incr->get_size().height;
		minsize.height += decr->get_size().height;
		minsize.height += bg->get_minimum_size().height;
		minsize.height += get_grabber_min_size();
	}

	if (orientation == HORIZONTAL) {
		int padding_top = MAX(theme_cache.padding_top, 0);
		int padding_bottom = MAX(theme_cache.padding_bottom, 0);
		minsize.height = MAX(incr->get_size().height, bg->get_minimum_size().height);
		minsize.height += padding_top + padding_bottom;
		minsize.width += incr->get_size().width;
		minsize.width += decr->get_size().width;
		minsize.width += bg->get_minimum_size().width;
		minsize.width += get_grabber_min_size();
	}

	return minsize;
}

void ScrollBar::scroll(double p_amount) { scroll_to(get_value() + p_amount); }

void ScrollBar::set_custom_step(float p_custom_step) { custom_step = p_custom_step; }

float ScrollBar::get_custom_step() const { return custom_step; }

void ScrollBar::_drag_node_input(const Ref<InputEvent>& p_input)
{
	if (!drag_node_enabled) {
		return;
	}

	Ref<InputEventMouseButton> mb = p_input;

	if (mb.is_valid()) {
		if (mb->get_button_index() != MouseButton::LEFT) {
			return;
		}

		if (mb->is_pressed()) {
			drag_node_speed = Vector2();
			drag_node_accum = Vector2();
			last_drag_node_accum = Vector2();
			drag_node_from = Vector2(orientation == HORIZONTAL ? get_value() : 0,
				orientation == VERTICAL ? get_value() : 0);
			drag_node_touching = DisplayServer::get_singleton()->is_touchscreen_available();
			drag_node_touching_deaccel = false;
			time_since_motion = 0;

			if (drag_node_touching) {
				set_process_internal(true);
				time_since_motion = 0;
			}

		}
		else {
			if (drag_node_touching) {
				if (drag_node_speed == Vector2()) {
					drag_node_touching_deaccel = false;
					drag_node_touching = false;
					set_process_internal(false);
				}
				else {
					drag_node_touching_deaccel = true;
				}
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_input;

	if (mm.is_valid()) {
		if (drag_node_touching && !drag_node_touching_deaccel) {
			Vector2 motion = mm->get_relative();

			drag_node_accum -= motion;
			Vector2 diff = drag_node_from + drag_node_accum;

			if (orientation == HORIZONTAL) {
				scroll_to(diff.x);
			}

			if (orientation == VERTICAL) {
				scroll_to(diff.y);
			}

			time_since_motion = 0;
		}
	}
}

NodePath ScrollBar::get_drag_node() const { return drag_node_path; }

void ScrollBar::set_drag_node_enabled(bool p_enable) { drag_node_enabled = p_enable; }

void ScrollBar::set_smooth_scroll_enabled(bool p_enable) { smooth_scroll_enabled = p_enable; }

bool ScrollBar::is_smooth_scroll_enabled() const { return smooth_scroll_enabled; }

ScrollBar::ScrollBar(Orientation p_orientation)
{
	orientation = p_orientation;

	if (focus_by_default) {
		set_focus_mode(FOCUS_ALL);
	}
	else {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
	set_step(0);
}

ScrollBar::~ScrollBar() {}


