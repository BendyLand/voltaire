/**************************************************************************/
/*  ui_widgets.h                                                          */
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

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "scene/resources/font.h"

// Composable, zero-inheritance Canvas UI state structs and drawing primitives
// shared by Voltaire's EditorUI and ProjectManager.

struct UIContext
{
	RID viewport_rid;
	RID canvas_rid;
	RID canvas_item_rid;

	Size2i window_size;
	Point2 mouse_pos;
	Ref<Font> font;

	Input::EventDispatchFunc prev_input_dispatch = nullptr;
	bool owns_viewport = false;
	bool needs_redraw = true;

	void initialize(RID p_viewport_rid, Size2i p_size, Size2i p_default_size,
		Input::EventDispatchFunc p_handler);
	void teardown(Input::EventDispatchFunc p_handler);
	void forward_input(const Ref<InputEvent>& p_event, Input::EventDispatchFunc p_handler) const;
	bool sync_window_size();
	RID begin_draw() const;
};

struct UIButton
{
	String label;
	Rect2 rect;
	void (*on_click)() = nullptr;
	bool (*is_enabled)() = nullptr;
	bool hovered = false;
	bool pressed = false;
	bool primary = false;
	bool active_tab = false;
};

struct UIButtonGroup
{
	LocalVector<UIButton> items;

	uint32_t size() const { return items.size(); }
	bool is_empty() const { return items.is_empty(); }
	void clear() { items.clear(); }
	void clear_rects();

	UIButton& operator[](uint32_t p_idx) { return items[p_idx]; }
	const UIButton& operator[](uint32_t p_idx) const { return items[p_idx]; }

	void add(const String& p_label, void (*p_on_click)(), bool (*p_is_enabled)() = nullptr,
		bool p_primary = false, bool p_active_tab = false);

	bool update_hover(Point2 p_mouse);
	bool press(Point2 p_mouse);
	void (*release(Point2 p_mouse))();
};

struct UITextField
{
	String text;
	Rect2 rect;
	String placeholder;

	bool has_point(Point2 p_pt) const { return rect.has_point(p_pt); }
	bool handle_key(const Ref<InputEventKey>& p_key, char32_t p_disallow_char = 0);
	void draw(RID p_ci, const Ref<Font>& p_font, bool p_focused, int p_font_size = 14,
		const Color& p_bg = Color(0.06f, 0.07f, 0.09f, 1.0f),
		const Color& p_accent = Color(0.0f, 0.75f, 0.95f, 1.0f),
		const Color& p_border = Color(0.19f, 0.23f, 0.28f, 1.0f),
		float p_right_reserve = 0.0f) const;
};

struct UIScrollView
{
	Rect2 bounds;
	float offset = 0.0f;
	float max_scroll = 0.0f;

	void update_metrics(float p_content_h, float p_view_h);
	bool handle_wheel(MouseButton p_btn, float p_step);
	void draw_scrollbar(RID p_ci, float p_track_x, float p_top_y, float p_view_h,
		float p_content_h, const Color& p_accent = Color(0.0f, 0.75f, 0.95f, 1.0f)) const;
};

struct UISplitLayout
{
	// Two-column main/sidebar split
	Rect2 primary_rect;
	Rect2 secondary_rect;
	bool show_secondary = false;

	// Three-column + optional bottom output workspace split
	Rect2 left_rect;
	Rect2 center_rect;
	Rect2 right_rect;
	Rect2 bottom_rect;
	bool show_left = false;
	bool show_right = false;
	bool show_bottom = false;

	void compute_sidebar_split(const Rect2& p_area, float p_min_width_for_sidebar,
		float p_max_sidebar_w, float p_sidebar_ratio, float p_gap);

	void compute_editor_workspace(const Rect2& p_area, float p_gap);
};

namespace UIDraw
{
float centered_baseline_y(const Ref<Font>& p_font, const Rect2& p_rect, int p_font_size);

void draw_bordered_rect(RID p_ci, const Rect2& p_rect, const Color& p_fill, const Color& p_border,
	float p_border_w = 1.0f);

void draw_header_banner(RID p_ci, const Rect2& p_rect, const Color& p_bg, const Color& p_accent,
	float p_strip_w = 4.0f);

void draw_dock_panel(RID p_ci, const Ref<Font>& p_font, const Rect2& p_rect, const String& p_title,
	const Color& p_dock_bg, const Color& p_hdr_bg, const Color& p_title_col,
	float p_hdr_h = 30.0f, float p_right_reserve = 12.0f);

void draw_button(
	RID p_ci, const Ref<Font>& p_font, const UIButton& p_btn, int p_font_size = 13);

bool clip_segment(const Rect2& p_bounds, Point2& r_a, Point2& r_b);

void draw_clipped_line(RID p_ci, const Rect2& p_clip, Point2 p_a, Point2 p_b, const Color& p_col,
	float p_width = 1.0f);

String read_project_name(const String& p_dir);
} // namespace UIDraw
