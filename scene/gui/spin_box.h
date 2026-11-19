/**************************************************************************/
/*  spin_box.h                                                            */
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
#include "scene/gui/line_edit.h"
#include "scene/gui/range.h"
#include "scene/main/timer.h"

class SpinBox : public Control
{
	LineEdit* line_edit = nullptr;
	bool update_on_text_changed = false;
	bool accepted = true;

	struct SizingCache
	{
		int buttons_block_width = 0;
		int buttons_width = 0;
		int buttons_vertical_separation = 0;
		int buttons_left = 0;
		int button_up_height = 0;
		int button_down_height = 0;
		int second_button_top = 0;
		int buttons_separator_top = 0;
		int field_and_buttons_separator_left = 0;
		int field_and_buttons_separator_width = 0;
	} sizing_cache;

	Timer* range_click_timer = nullptr;
	void _release_mouse_from_drag_mode();
	void _arrow_clicked(bool p_up);

	void _update_text(bool p_only_update_if_value_changed = false);
	void _text_submitted(const String& p_string);
	void _text_changed(const String& p_string);

	String prefix;
	String suffix;
	String last_text_value;
	double custom_arrow_step = 0.0;
	bool custom_arrow_round = false;

	void _line_edit_input(const InputEvent& p_event);

	struct Drag
	{
		double base_val = 0.0;
		bool allowed = false;
		bool enabled = false;
		Vector2 capture_pos;
		double diff_y = 0.0;
	} drag;

	struct StateCache
	{
		bool up_button_hovered = false;
		bool up_button_pressed = false;
		bool up_button_disabled = false;
		bool down_button_hovered = false;
		bool down_button_pressed = false;
		bool down_button_disabled = false;
	} state_cache;

	void _line_edit_editing_toggled(bool p_toggled_on);

	inline void _compute_sizes();
	inline int _get_widest_button_icon_width();

	struct ThemeCache
	{
		Ref<Texture2D> up_icon;
		Ref<Texture2D> up_hover_icon;
		Ref<Texture2D> up_pressed_icon;
		Ref<Texture2D> up_disabled_icon;
		Ref<Texture2D> down_icon;
		Ref<Texture2D> down_hover_icon;
		Ref<Texture2D> down_pressed_icon;
		Ref<Texture2D> down_disabled_icon;

		Ref<StyleBox> up_base_stylebox;
		Ref<StyleBox> up_hover_stylebox;
		Ref<StyleBox> up_pressed_stylebox;
		Ref<StyleBox> up_disabled_stylebox;
		Ref<StyleBox> down_base_stylebox;
		Ref<StyleBox> down_hover_stylebox;
		Ref<StyleBox> down_pressed_stylebox;
		Ref<StyleBox> down_disabled_stylebox;

		Color up_icon_modulate;
		Color up_hover_icon_modulate;
		Color up_pressed_icon_modulate;
		Color up_disabled_icon_modulate;
		Color down_icon_modulate;
		Color down_hover_icon_modulate;
		Color down_pressed_icon_modulate;
		Color down_disabled_icon_modulate;

		Ref<StyleBox> field_and_buttons_separator;
		Ref<StyleBox> up_down_buttons_separator;

		int buttons_vertical_separation = 0;
		int field_and_buttons_separation = 0;
		int buttons_width = 0;
#ifndef DISABLE_DEPRECATED
		Ref<Texture2D> updown_icon;
		bool is_updown_assigned = false;
		bool set_min_buttons_width_from_icons = false;
#endif
	} theme_cache;

	void _mouse_exited();
	void _update_buttons_state_for_current_value();

protected:
	void _value_changed(double p_value);

	void _notification(int p_what);

public:
	NumericRange range;

	double get_value() const { return range.value; }
	double get_min() const { return range.min; }
	double get_max() const { return range.max; }
	double get_step() const { return range.step; }
	double get_page() const { return range.page; }
	double get_as_ratio() const { return range.get_as_ratio(); }

	void set_value(double p_val)
	{
		if (range.set_value(p_val)) {
			_value_changed(range.value);
			queue_redraw();
		}
		queue_accessibility_update();
	}

	void set_value_no_signal(double p_val)
	{
		if (range.set_value(p_val)) {
			_value_changed(range.value);
			queue_redraw();
		}
	}

	void set_min(double p_min)
	{
		if (range.set_min(p_min)) {
			_value_changed(range.value);
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_max(double p_max)
	{
		if (range.set_max(p_max)) {
			_value_changed(range.value);
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
			_value_changed(range.value);
			queue_redraw();
			queue_accessibility_update();
		}
	}

	void set_as_ratio(double p_value)
	{
		if (range.set_as_ratio(p_value)) {
			_value_changed(range.value);
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

	LineEdit* get_line_edit();

	virtual Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_editable(bool p_enabled);
	bool is_editable() const;

	void set_suffix(const String& p_suffix);
	String get_suffix() const;

	void set_prefix(const String& p_prefix);
	String get_prefix() const;

	void set_update_on_text_changed(bool p_enabled);
	bool get_update_on_text_changed() const;

	void set_select_all_on_focus(bool p_enabled);
	bool is_select_all_on_focus() const;

	void apply();
	void set_custom_arrow_step(const double p_custom_arrow_step);
	double get_custom_arrow_step() const;

	void set_custom_arrow_round(bool p_round);
	bool is_custom_arrow_rounding() const;

	SpinBox();
};
