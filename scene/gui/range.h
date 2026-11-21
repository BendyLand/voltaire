/**************************************************************************/
/*  range.h                                                               */
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

#include "core/types.h"

struct NumericRange
{
	double value = 0.0;
	double min = 0.0;
	double max = 100.0;
	double step = 1.0;
	double page = 0.0;
	bool exp_ratio = false;
	bool allow_greater = false;
	bool allow_lesser = false;
	bool rounded_values = false;

	static double snapped_r128(double p_value, double p_step);
	double calc_value(double p_val, double p_step) const;

	bool set_value(double p_val);
	bool set_min(double p_min);
	bool set_max(double p_max);
	bool set_step(double p_step);
	bool set_page(double p_page);
	bool set_as_ratio(double p_ratio);
	double get_as_ratio() const;

	void append_configuration_warnings(PackedStringArray& r_warnings) const;
};

// --- Folded from scene/gui/slider.h ---
#include "scene/gui/control.h"


class Slider : public Control
{
public:
	enum TickPosition
	{
		TICK_POSITION_BOTTOM_RIGHT,
		TICK_POSITION_TOP_LEFT,
		TICK_POSITION_BOTH,
		TICK_POSITION_CENTER,
	};

private:
	struct Grab
	{
		int pos = 0;
		double uvalue = 0.0;
		double value_before_dragging = 0.0;
		bool active = false;
	} grab;

	int ticks = 0;
	TickPosition ticks_position = TICK_POSITION_BOTTOM_RIGHT;

	bool mouse_inside = false;
	Orientation orientation;
	double custom_step = -1.0;
	bool editable = true;
	bool scrollable = true;

	const float DEFAULT_GAMEPAD_EVENT_DELAY_MS = 0.5;
	const float GAMEPAD_EVENT_REPEAT_RATE_MS = 1.0 / 20;
	float gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;

	struct ThemeCache
	{
		Ref<StyleBox> slider_style;
		Ref<StyleBox> grabber_area_style;
		Ref<StyleBox> grabber_area_hl_style;

		Ref<Texture2D> grabber_icon;
		Ref<Texture2D> grabber_hl_icon;
		Ref<Texture2D> grabber_disabled_icon;
		Ref<Texture2D> tick_icon;

		bool center_grabber = false;
		int grabber_offset = 0;
		int tick_offset = 0;
	} theme_cache;

protected:
	bool ticks_on_borders = false;

	void _notification(int p_what);

public:
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

	virtual Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	void set_custom_step(double p_custom_step);
	double get_custom_step() const;

	void set_ticks(int p_count);
	int get_ticks() const;

	void set_ticks_on_borders(bool);
	bool get_ticks_on_borders() const;

	void set_ticks_position(TickPosition p_ticks_position);
	TickPosition get_ticks_position() const;

	void set_editable(bool p_editable);
	bool is_editable() const;

	void set_scrollable(bool p_scrollable);
	bool is_scrollable() const;

	Slider(Orientation p_orientation = HORIZONTAL);
};

// --- Folded from scene/gui/texture_progress_bar.h ---
#include "scene/gui/control.h"


class TextureProgressBar : public Control
{
	Ref<Texture2D> under;
	Ref<Texture2D> progress;
	Ref<Texture2D> over;

protected:
	void _notification(int p_what);

public:
	enum FillMode
	{
		FILL_LEFT_TO_RIGHT = 0,
		FILL_RIGHT_TO_LEFT,
		FILL_TOP_TO_BOTTOM,
		FILL_BOTTOM_TO_TOP,
		FILL_CLOCKWISE,
		FILL_COUNTER_CLOCKWISE,
		FILL_BILINEAR_LEFT_AND_RIGHT,
		FILL_BILINEAR_TOP_AND_BOTTOM,
		FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE,
		FILL_MODE_MAX,
	};

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

	void set_fill_mode(int p_fill);
	int get_fill_mode();

	void set_progress_offset(Point2 p_offset);
	Point2 get_progress_offset() const;

	void set_radial_initial_angle(float p_angle);
	float get_radial_initial_angle();

	void set_fill_degrees(float p_angle);
	float get_fill_degrees();

	void set_radial_center_offset(const Point2& p_off);
	Point2 get_radial_center_offset();

	void set_under_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_under_texture() const;

	void set_progress_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_progress_texture() const;

	void set_over_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_over_texture() const;

	void set_stretch_margin(Side p_side, int p_size);
	int get_stretch_margin(Side p_side) const;

	void set_nine_patch_stretch(bool p_stretch);
	bool get_nine_patch_stretch() const;

	void set_tint_under(const Color& p_tint);
	Color get_tint_under() const;

	void set_tint_progress(const Color& p_tint);
	Color get_tint_progress() const;

	void set_tint_over(const Color& p_tint);
	Color get_tint_over() const;

	Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	TextureProgressBar();

private:
	FillMode mode = FILL_LEFT_TO_RIGHT;
	Point2 progress_offset;
	float rad_init_angle = 0.0;
	float rad_max_degrees = 360.0;
	Point2 rad_center_off;
	bool nine_patch_stretch = false;
	int stretch_margin[4] = {};
	Color tint_under = Color(1, 1, 1);
	Color tint_progress = Color(1, 1, 1);
	Color tint_over = Color(1, 1, 1);

	void _set_texture(Ref<Texture2D>* p_destination, const Ref<Texture2D>& p_texture);
	void _texture_changed();
	Point2 unit_val_to_uv(float val);
	Point2 get_relative_center();
	void draw_nine_patch_stretched(
		const Ref<Texture2D>& p_texture, FillMode p_mode, double p_ratio, const Color& p_modulate);
};

// --- End scene/gui/texture_progress_bar.h ---

// --- End scene/gui/slider.h ---
