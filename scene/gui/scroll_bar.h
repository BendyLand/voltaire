/**************************************************************************/
/*  scroll_bar.h                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#pragma once

#include "scene/gui/control.h"
#include "scene/gui/range.h"

class ScrollBar : public Control
{
	enum HighlightStatus
	{
		HIGHLIGHT_NONE,
		HIGHLIGHT_DECR,
		HIGHLIGHT_RANGE,
		HIGHLIGHT_INCR,
	};

	static bool focus_by_default;

	Orientation orientation;
	Size2 size;
	float custom_step = -1.0;

	HighlightStatus highlight = HIGHLIGHT_NONE;

	bool incr_active = false;
	bool decr_active = false;

	struct Drag
	{
		bool active = false;
		float pos_at_click = 0.0;
		float value_at_click = 0.0;
	} drag;

	double get_grabber_size() const;
	double get_grabber_min_size() const;
	double get_area_size() const;
	double get_grabber_offset() const;

	static void set_can_focus_by_default(bool p_can_focus);

	Node* drag_node = nullptr;
	NodePath drag_node_path;
	bool drag_node_enabled = true;

	Vector2 drag_node_speed;
	Vector2 drag_node_accum;
	Vector2 drag_node_from;
	Vector2 last_drag_node_accum;
	float time_since_motion = 0.0;
	bool drag_node_touching = false;
	bool drag_node_touching_deaccel = false;

	bool scrolling = false;
	double target_scroll = 0.0;
	bool smooth_scroll_enabled = false;

	void _drag_node_exit();
	void _drag_node_input(const InputEvent& p_input);

protected:
	struct ThemeCache
	{
		Ref<StyleBox> scroll_style;
		Ref<StyleBox> scroll_focus_style;
		Ref<StyleBox> scroll_offset_style;
		Ref<StyleBox> grabber_style;
		Ref<StyleBox> grabber_hl_style;
		Ref<StyleBox> grabber_pressed_style;

		Ref<Texture2D> increment_icon;
		Ref<Texture2D> increment_hl_icon;
		Ref<Texture2D> increment_pressed_icon;
		Ref<Texture2D> decrement_icon;
		Ref<Texture2D> decrement_hl_icon;
		Ref<Texture2D> decrement_pressed_icon;

		int padding_left = 0;
		int padding_top = 0;
		int padding_right = 0;
		int padding_bottom = 0;
	} theme_cache;

	void _notification(int p_what);

public:
	static inline const int PAGE_DIVISOR = 8;

	NumericRange range;

	Orientation get_orientation() const { return orientation; }

	double get_value() const { return range.value; }
	double get_min() const { return range.min; }
	double get_max() const { return range.max; }
	double get_step() const { return range.step; }
	double get_page() const { return range.page; }
	double get_as_ratio() const { return range.get_as_ratio(); }

	void set_value(double p_val)
	{
		if (range.set_value(p_val)) {
			queue_redraw();
		}
		queue_accessibility_update();
	}

	void set_value_no_signal(double p_val)
	{
		if (range.set_value(p_val)) {
			queue_redraw();
		}
	}

	void set_min(double p_min)
	{
		if (range.set_min(p_min)) {
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_max(double p_max)
	{
		if (range.set_max(p_max)) {
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_step(double p_step)
	{
		if (range.set_step(p_step)) {
			queue_accessibility_update();
		}
	}

	void set_page(double p_page)
	{
		if (range.set_page(p_page)) {
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_as_ratio(double p_value)
	{
		if (range.set_as_ratio(p_value)) {
			queue_redraw();
		}
		queue_accessibility_update();
	}

	void set_allow_greater(bool p_allow) { range.allow_greater = p_allow; }
	bool is_greater_allowed() const { return range.allow_greater; }
	void set_allow_lesser(bool p_allow) { range.allow_lesser = p_allow; }
	bool is_lesser_allowed() const { return range.allow_lesser; }
	void set_use_rounded_values(bool p_enable) { range.rounded_values = p_enable; }
	bool is_using_rounded_values() const { return range.rounded_values; }
	bool is_ratio_exp() const { return range.exp_ratio; }

	void scroll(double p_amount);
	void scroll_to(double p_position);

	void set_custom_step(float p_custom_step);
	float get_custom_step() const;

	void set_drag_node(const NodePath& p_path);
	NodePath get_drag_node() const;
	void set_drag_node_enabled(bool p_enable);

	void set_smooth_scroll_enabled(bool p_enable);
	bool is_smooth_scroll_enabled() const;

	virtual Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	ScrollBar(Orientation p_orientation = VERTICAL);
	~ScrollBar();
};
