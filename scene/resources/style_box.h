/**************************************************************************/
/*  style_box.h                                                           */
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

#include "core/io/resource.h"
#include "scene/resources/texture.h"

class CanvasItem;

class StyleBox : public Resource
{
public:
	enum StyleType
	{
		STYLE_EMPTY,
		STYLE_FLAT,
		STYLE_TEXTURE,
		STYLE_LINE,
	};

	enum AxisStretchMode
	{
		AXIS_STRETCH_MODE_STRETCH,
		AXIS_STRETCH_MODE_TILE,
		AXIS_STRETCH_MODE_TILE_FIT,
	};

private:
	StyleType style_type = STYLE_EMPTY;
	float content_margin[4];

	// Shared by STYLE_FLAT and STYLE_TEXTURE.
	real_t expand_margin[4] = {};
	bool draw_center = true;

	// STYLE_FLAT fields.
	Color bg_color = Color(0.6, 0.6, 0.6);
	Color shadow_color = Color(0, 0, 0, 0.6);
	Color border_color = Color(0.8, 0.8, 0.8);
	real_t border_width[4] = {};
	real_t corner_radius[4] = {};
	bool blend_border = false;
	Vector2 skew;
	bool anti_aliased = true;
	int corner_detail = 8;
	int shadow_size = 0;
	Point2 shadow_offset;
	real_t aa_size = 1;

	// STYLE_TEXTURE fields.
	float texture_margin[4] = {};
	Rect2 region_rect;
	Ref<Texture2D> texture;
	Color modulate = Color(1, 1, 1, 1);
	AxisStretchMode axis_h = AXIS_STRETCH_MODE_STRETCH;
	AxisStretchMode axis_v = AXIS_STRETCH_MODE_STRETCH;

	// STYLE_LINE fields.
	Color color;
	int thickness = 1;
	bool vertical = false;
	float grow_begin = 1.0;
	float grow_end = 1.0;

	void _draw_flat(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_texture(RID p_canvas_item, const Rect2& p_rect) const;
	void _draw_line(RID p_canvas_item, const Rect2& p_rect) const;

protected:
	float get_style_margin(Side p_side) const;

public:
	void set_style_type(StyleType p_type);
	StyleType get_style_type() const { return style_type; }

	Size2 get_minimum_size() const;

	void set_content_margin(Side p_side, float p_value);
	void set_content_margin_all(float p_value);
	void set_content_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_content_margin(Side p_side) const;

	float get_margin(Side p_side) const;
	Point2 get_offset() const;

	void draw(RID p_canvas_item, const Rect2& p_rect) const;
	Rect2 get_draw_rect(const Rect2& p_rect) const;

	CanvasItem* get_current_item_drawn() const;

	bool test_mask(const Point2& p_point, const Rect2& p_rect) const;

	// Shared STYLE_FLAT / STYLE_TEXTURE properties.
	void set_expand_margin(Side p_expand_side, float p_size);
	void set_expand_margin_all(float p_expand_margin_size);
	void set_expand_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_expand_margin(Side p_expand_side) const;

	void set_draw_center(bool p_enabled);
	bool is_draw_center_enabled() const;

	// STYLE_FLAT properties.
	void set_bg_color(const Color& p_color);
	Color get_bg_color() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_border_width_all(int p_size);
	int get_border_width_min() const;

	void set_border_width(Side p_side, int p_width);
	int get_border_width(Side p_side) const;

	void set_border_blend(bool p_blend);
	bool get_border_blend() const;

	void set_corner_radius_all(int radius);
	void set_corner_radius_individual(const int radius_top_left, const int radius_top_right,
		const int radius_bottom_right, const int radius_bottom_left);
	void set_corner_radius(Corner p_corner, const int radius);
	int get_corner_radius(Corner p_corner) const;

	void set_corner_detail(const int& p_corner_detail);
	int get_corner_detail() const;

	void set_skew(Vector2 p_skew);
	Vector2 get_skew() const;

	void set_shadow_color(const Color& p_color);
	Color get_shadow_color() const;

	void set_shadow_size(const int& p_size);
	int get_shadow_size() const;

	void set_shadow_offset(const Point2& p_offset);
	Point2 get_shadow_offset() const;

	void set_anti_aliased(const bool& p_anti_aliased);
	bool is_anti_aliased() const;
	void set_aa_size(const real_t p_aa_size);
	real_t get_aa_size() const;

	// STYLE_TEXTURE properties.
	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_margin(Side p_side, float p_size);
	void set_texture_margin_all(float p_size);
	void set_texture_margin_individual(float p_left, float p_top, float p_right, float p_bottom);
	float get_texture_margin(Side p_side) const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_h_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_h_axis_stretch_mode() const;

	void set_v_axis_stretch_mode(AxisStretchMode p_mode);
	AxisStretchMode get_v_axis_stretch_mode() const;

	void set_modulate(const Color& p_modulate);
	Color get_modulate() const;

	// STYLE_LINE properties.
	void set_color(const Color& p_color);
	Color get_color() const;

	void set_thickness(int p_thickness);
	int get_thickness() const;

	void set_vertical(bool p_vertical);
	bool is_vertical() const;

	void set_grow_begin(float p_grow);
	float get_grow_begin() const;

	void set_grow_end(float p_grow);
	float get_grow_end() const;

	StyleBox(StyleType p_type = STYLE_EMPTY);
};
