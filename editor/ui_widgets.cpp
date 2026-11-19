/**************************************************************************/
/*  ui_widgets.cpp                                                        */
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

#include "core/config/project_format.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "editor/ui_widgets.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

static inline bool _is_word_char(char32_t c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static inline int _prev_word_boundary(const String& s, int pos)
{
	pos = CLAMP(pos, 0, s.length());
	while (pos > 0 && !_is_word_char(s[pos - 1])) {
		pos--;
	}
	while (pos > 0 && _is_word_char(s[pos - 1])) {
		pos--;
	}
	return pos;
}

static inline int _next_word_boundary(const String& s, int pos)
{
	int len = s.length();
	pos = CLAMP(pos, 0, len);
	while (pos < len && _is_word_char(s[pos])) {
		pos++;
	}
	while (pos < len && !_is_word_char(s[pos])) {
		pos++;
	}
	return pos;
}

int UITextField::hit_test_cursor(const Ref<Font>& p_font, int p_font_size, float p_mouse_x) const
{
	if (p_font.is_null() || text.is_empty()) {
		return 0;
	}
	float local_x = p_mouse_x - (rect.position.x + 10.0f) + scroll_x;
	if (local_x <= 0.0f) {
		return 0;
	}
	int len = text.length();
	float prev_w = 0.0f;
	for (int i = 1; i <= len; i++) {
		float w =
			p_font->get_string_size(text.substr(0, i), HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size)
				.x;
		if (local_x < (prev_w + w) * 0.5f) {
			return i - 1;
		}
		prev_w = w;
	}
	return len;
}

bool UITextField::handle_mouse_button(
	const InputEvent& p_event, const Ref<Font>& p_font, int p_font_size)
{
	if (p_event.type != InputEventType::MOUSE_BUTTON ||
		p_event.mouse_button.button_index != MouseButton::LEFT) {
		return false;
	}
	if (!p_event.is_pressed()) {
		if (selecting) {
			selecting = false;
			return true;
		}
		return false;
	}
	if (!rect.has_point(p_event.mouse_button.position)) {
		selecting = false;
		return false;
	}

	int len = text.length();
	cursor = CLAMP(cursor, 0, len);
	anchor = CLAMP(anchor, 0, len);

	if (p_event.mouse_button.double_click) {
		select_all();
		selecting = false;
		return true;
	}

	int idx = hit_test_cursor(p_font, p_font_size, p_event.mouse_button.position.x);
	cursor = idx;
	if (!p_event.is_shift_pressed()) {
		anchor = idx;
	}
	selecting = true;
	return true;
}

bool UITextField::handle_mouse_motion(
	const InputEvent& p_event, const Ref<Font>& p_font, int p_font_size)
{
	if (!selecting || p_event.type != InputEventType::MOUSE_MOTION) {
		return false;
	}
	int next = hit_test_cursor(p_font, p_font_size, p_event.mouse_motion.position.x);
	if (next != cursor) {
		cursor = next;
		return true;
	}
	return false;
}

bool UITextField::handle_key(const InputEvent& p_key_event, char32_t p_disallow_char)
{
	if (p_key_event.type != InputEventType::KEY || !p_key_event.is_pressed()) {
		return false;
	}

	int len = text.length();
	cursor = CLAMP(cursor, 0, len);
	anchor = CLAMP(anchor, 0, len);

	const Key code = p_key_event.key.keycode;
	const bool ctrl = p_key_event.is_ctrl_pressed() || p_key_event.is_meta_pressed();
	const bool shift = p_key_event.is_shift_pressed();

	auto erase_selection = [&]() -> bool {
		int sel_a = MIN(cursor, anchor);
		int sel_b = MAX(cursor, anchor);
		if (sel_a == sel_b) {
			return false;
		}
		text = text.substr(0, sel_a) + text.substr(sel_b);
		cursor = anchor = sel_a;
		return true;
	};

	// Select All (Ctrl+A)
	if (ctrl && code == Key::A) {
		select_all();
		return true;
	}

	// Copy / Cut (Ctrl+C / Ctrl+X)
	if (ctrl && (code == Key::C || code == Key::X)) {
		int sel_a = MIN(cursor, anchor);
		int sel_b = MAX(cursor, anchor);
		if (sel_a < sel_b && DisplayServer::is_initialized()) {
			DisplayServer::clipboard_set(text.substr(sel_a, sel_b - sel_a));
			if (code == Key::X) {
				erase_selection();
			}
			return true;
		}
		return false;
	}

	// Paste (Ctrl+V)
	if (ctrl && code == Key::V && DisplayServer::is_initialized()) {
		String clip = DisplayServer::clipboard_get().replace("\r", "").replace("\n", "");
		if (p_disallow_char != 0) {
			clip = clip.replace(String::chr(p_disallow_char), "_");
		}
		bool changed = erase_selection();
		if (!clip.is_empty()) {
			text = text.substr(0, cursor) + clip + text.substr(cursor);
			cursor += clip.length();
			anchor = cursor;
			changed = true;
		}
		return changed;
	}

	// Caret navigation
	if (code == Key::LEFT || code == Key::RIGHT || code == Key::HOME || code == Key::END) {
		int next = cursor;
		if (code == Key::LEFT) {
			if (!shift && cursor != anchor) {
				next = MIN(cursor, anchor);
			}
			else {
				next = ctrl ? _prev_word_boundary(text, cursor) : MAX(0, cursor - 1);
			}
		}
		else if (code == Key::RIGHT) {
			if (!shift && cursor != anchor) {
				next = MAX(cursor, anchor);
			}
			else {
				next = ctrl ? _next_word_boundary(text, cursor) : MIN(len, cursor + 1);
			}
		}
		else if (code == Key::HOME) {
			next = 0;
		}
		else if (code == Key::END) {
			next = len;
		}
		cursor = next;
		if (!shift) {
			anchor = cursor;
		}
		return true;
	}

	// Backspace
	if (code == Key::BACKSPACE) {
		if (erase_selection()) {
			return true;
		}
		if (cursor > 0) {
			int del_from = ctrl ? _prev_word_boundary(text, cursor) : (cursor - 1);
			text = text.substr(0, del_from) + text.substr(cursor);
			cursor = anchor = del_from;
			return true;
		}
		return false;
	}

	// Delete
	if (code == Key::KEY_DELETE) {
		if (erase_selection()) {
			return true;
		}
		if (cursor < len) {
			int del_to = ctrl ? _next_word_boundary(text, cursor) : (cursor + 1);
			text = text.substr(0, cursor) + text.substr(del_to);
			anchor = cursor;
			return true;
		}
		return false;
	}

	// Printable character insertion (replaces selection if active)
	char32_t ch = p_key_event.key.unicode;
	if (ch >= 32 && ch != 127 && ch != p_disallow_char && !ctrl) {
		erase_selection();
		text = text.substr(0, cursor) + String::chr(ch) + text.substr(cursor);
		cursor += 1;
		anchor = cursor;
		return true;
	}

	return false;
}

void UITextField::draw(RID p_ci, const Ref<Font>& p_font, bool p_focused, int p_font_size,
	const Color& p_bg, const Color& p_accent, const Color& p_border, float p_right_reserve) const
{
	UIDraw::draw_bordered_rect(
		p_ci, rect, p_bg, p_focused ? p_accent : p_border, p_focused ? 2.0f : 1.0f);

	const float max_w = rect.size.x - 20.0f - p_right_reserve;
	if (p_font.is_null() || max_w <= 12.0f) {
		return;
	}

	const float text_x = rect.position.x + 10.0f;
	const float base_y = UIDraw::centered_baseline_y(p_font, rect, p_font_size);

	if (text.is_empty()) {
		if (!placeholder.is_empty()) {
			p_font->draw_string(p_ci, Point2(text_x, base_y), placeholder,
				HORIZONTAL_ALIGNMENT_LEFT, max_w, p_font_size, Color(0.58f, 0.64f, 0.72f, 1.0f));
		}
		if (p_focused) {
			float caret_h = MAX(12.0f, rect.size.y - 14.0f);
			float caret_y = rect.position.y + (rect.size.y - caret_h) * 0.5f;
			Renderer::canvas_item_add_rect(p_ci, Rect2(text_x, caret_y, 2.0f, caret_h), p_accent);
		}
		return;
	}

	int len = text.length();
	int cur = CLAMP(cursor, 0, len);
	int anc = CLAMP(anchor, 0, len);

	auto prefix_w = [&](int idx) -> float {
		if (idx <= 0) {
			return 0.0f;
		}
		return p_font
			->get_string_size(text.substr(0, idx), HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size)
			.x;
	};

	float cur_px = prefix_w(cur);
	float sx = scroll_x;
	if (cur_px - sx > max_w - 4.0f) {
		sx = cur_px - max_w + 4.0f;
	}
	else if (cur_px - sx < 0.0f) {
		sx = cur_px;
	}
	const_cast<UITextField*>(this)->scroll_x = MAX(0.0f, sx);

	// Find visible character slice [vis_start, vis_end) that fits inside max_w
	int vis_start = 0;
	while (vis_start < len && prefix_w(vis_start + 1) <= sx) {
		vis_start++;
	}
	float start_px = prefix_w(vis_start);
	int vis_end = vis_start;
	while (vis_end < len && (prefix_w(vis_end + 1) - start_px) <= max_w) {
		vis_end++;
	}

	const float caret_h = MAX(12.0f, rect.size.y - 12.0f);
	const float caret_y = rect.position.y + (rect.size.y - caret_h) * 0.5f;

	// Draw selection highlight rectangle
	if (p_focused && cur != anc) {
		int sel_a = CLAMP(MIN(cur, anc), vis_start, vis_end);
		int sel_b = CLAMP(MAX(cur, anc), vis_start, vis_end);
		if (sel_b > sel_a) {
			float x0 = text_x + (prefix_w(sel_a) - start_px);
			float x1 = text_x + (prefix_w(sel_b) - start_px);
			Renderer::canvas_item_add_rect(p_ci, Rect2(x0, caret_y, x1 - x0, caret_h),
				Color(p_accent.r, p_accent.g, p_accent.b, 0.32f));
		}
	}

	// Draw visible text slice
	String vis_str = text.substr(vis_start, vis_end - vis_start);
	p_font->draw_string(p_ci, Point2(text_x, base_y), vis_str, HORIZONTAL_ALIGNMENT_LEFT, max_w,
		p_font_size, Color(0.92f, 0.94f, 0.96f, 1.0f));

	// Draw vertical caret bar
	if (p_focused) {
		float caret_x = text_x + CLAMP(cur_px - start_px, 0.0f, max_w);
		Renderer::canvas_item_add_rect(p_ci, Rect2(caret_x, caret_y, 2.0f, caret_h), p_accent);
	}
}

void UIContext::initialize(
	RID p_viewport_rid, Size2i p_size, Size2i p_default_size, Input::EventDispatchFunc p_handler)
{
	window_size = (p_size.x > 0 && p_size.y > 0) ? p_size : p_default_size;

	if (p_viewport_rid.is_valid()) {
		viewport_rid = p_viewport_rid;
		owns_viewport = false;
	}
	else {
		viewport_rid = Renderer::viewport_create();
		owns_viewport = true;
	}

	canvas_rid = Renderer::canvas_create();
	Renderer::viewport_attach_canvas(viewport_rid, canvas_rid);
	Renderer::viewport_set_size(viewport_rid, window_size.x, window_size.y);
	Renderer::viewport_set_active(viewport_rid, true);
	Renderer::viewport_set_update_mode(viewport_rid, RSE::VIEWPORT_UPDATE_ALWAYS);
	Renderer::viewport_attach_to_screen(viewport_rid, Rect2(0, 0, window_size.x, window_size.y),
		DisplayServerEnums::MAIN_WINDOW_ID);

	canvas_item_rid = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(canvas_item_rid, canvas_rid);
	Renderer::canvas_item_set_visible(canvas_item_rid, true);

	if (ThemeDB::is_initialized()) {
		font = ThemeDB::get_fallback_font();
	}

	if (p_handler && Input::is_initialized() && Input::data) {
		prev_input_dispatch = Input::data->event_dispatch_function;
		Input::set_event_dispatch_function(p_handler);
	}

	needs_redraw = true;
}

void UIContext::teardown(Input::EventDispatchFunc p_handler)
{
	if (p_handler && Input::is_initialized() && Input::data &&
		Input::data->event_dispatch_function == p_handler) {
		Input::set_event_dispatch_function(prev_input_dispatch);
	}
	prev_input_dispatch = nullptr;

	if (canvas_item_rid.is_valid()) {
		Renderer::free_rid(canvas_item_rid);
		canvas_item_rid = RID();
	}
	if (canvas_rid.is_valid()) {
		Renderer::free_rid(canvas_rid);
		canvas_rid = RID();
	}
	if (owns_viewport && viewport_rid.is_valid()) {
		Renderer::free_rid(viewport_rid);
		viewport_rid = RID();
	}
	owns_viewport = false;
}

void UIContext::forward_input(const InputEvent& p_event, Input::EventDispatchFunc p_handler) const
{
	if (prev_input_dispatch && prev_input_dispatch != p_handler) {
		prev_input_dispatch(p_event);
	}
}

bool UIContext::sync_window_size()
{
	if (!DisplayServer::is_initialized()) {
		return false;
	}
	Size2i ds_size = DisplayServer::window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
	if (ds_size.x > 0 && ds_size.y > 0 && ds_size != window_size) {
		window_size = ds_size;
		Renderer::viewport_set_size(viewport_rid, window_size.x, window_size.y);
		Renderer::viewport_attach_to_screen(viewport_rid, Rect2(0, 0, window_size.x, window_size.y),
			DisplayServerEnums::MAIN_WINDOW_ID);
		needs_redraw = true;
		return true;
	}
	return false;
}

RID UIContext::begin_draw() const
{
	if (!canvas_item_rid.is_valid()) {
		return RID();
	}
	Renderer::canvas_item_clear(canvas_item_rid);
	return canvas_item_rid;
}

void UIButtonGroup::clear_rects()
{
	for (uint32_t i = 0; i < items.size(); i++) {
		items[i].rect = Rect2();
	}
}

void UIButtonGroup::add(const String& p_label, void (*p_on_click)(), bool (*p_is_enabled)(),
	bool p_primary, bool p_active_tab)
{
	items.push_back(
		{p_label, Rect2(), p_on_click, p_is_enabled, false, false, p_primary, p_active_tab});
}

bool UIButtonGroup::update_hover(Point2 p_mouse)
{
	bool changed = false;
	for (uint32_t i = 0; i < items.size(); i++) {
		bool h = items[i].rect.has_point(p_mouse);
		if (items[i].hovered != h) {
			items[i].hovered = h;
			changed = true;
		}
	}
	return changed;
}

bool UIButtonGroup::press(Point2 p_mouse)
{
	bool any = false;
	for (uint32_t i = 0; i < items.size(); i++) {
		UIButton& b = items[i];
		b.hovered = b.rect.has_point(p_mouse);
		const bool enabled = (b.is_enabled == nullptr) || b.is_enabled();
		b.pressed = b.hovered && enabled;
		any |= b.pressed;
	}
	return any;
}

void (*UIButtonGroup::release(Point2 p_mouse))()
{
	void (*to_invoke)() = nullptr;
	for (uint32_t i = 0; i < items.size(); i++) {
		UIButton& b = items[i];
		const bool was_pressed = b.pressed;
		b.pressed = false;
		b.hovered = b.rect.has_point(p_mouse);
		const bool enabled = (b.is_enabled == nullptr) || b.is_enabled();
		if (was_pressed && b.hovered && enabled && b.on_click) {
			to_invoke = b.on_click;
		}
	}
	return to_invoke;
}

void UIScrollView::update_metrics(float p_content_h, float p_view_h)
{
	max_scroll = MAX(0.0f, p_content_h - p_view_h);
	offset = CLAMP(offset, 0.0f, max_scroll);
}

bool UIScrollView::handle_wheel(MouseButton p_btn, float p_step)
{
	if (p_btn == MouseButton::WHEEL_UP) {
		float next = MAX(0.0f, offset - p_step);
		if (next != offset) {
			offset = next;
			return true;
		}
	}
	else if (p_btn == MouseButton::WHEEL_DOWN) {
		float next = MIN(max_scroll, offset + p_step);
		if (next != offset) {
			offset = next;
			return true;
		}
	}
	return false;
}

void UIScrollView::draw_scrollbar(RID p_ci, float p_track_x, float p_top_y, float p_view_h,
	float p_content_h, const Color& p_accent) const
{
	if (max_scroll <= 0.0f || p_view_h <= 12.0f || p_content_h <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(
		p_ci, Rect2(p_track_x, p_top_y, 4.0f, p_view_h), Color(0.08f, 0.09f, 0.11f, 1.0f));
	float thumb_h = MAX(20.0f, p_view_h * (p_view_h / p_content_h));
	float thumb_y = p_top_y + (p_view_h - thumb_h) * (offset / max_scroll);
	Renderer::canvas_item_add_rect(p_ci, Rect2(p_track_x, thumb_y, 4.0f, thumb_h), p_accent);
}

void UISplitLayout::compute_sidebar_split(const Rect2& p_area, float p_min_width_for_sidebar,
	float p_max_sidebar_w, float p_sidebar_ratio, float p_gap)
{
	show_secondary = p_area.size.x >= p_min_width_for_sidebar;
	const float gap = show_secondary ? p_gap : 0.0f;
	const float side_w =
		show_secondary ? MIN(p_max_sidebar_w, p_area.size.x * p_sidebar_ratio) : 0.0f;
	const float prim_w = MAX(0.0f, p_area.size.x - side_w - gap);

	primary_rect = Rect2(p_area.position.x, p_area.position.y, prim_w, p_area.size.y);
	secondary_rect = show_secondary ? Rect2(p_area.position.x + prim_w + gap, p_area.position.y,
										  side_w, p_area.size.y)
									: Rect2();
}

void UISplitLayout::compute_editor_workspace(const Rect2& p_area, float p_gap)
{
	show_bottom = p_area.size.y >= 240.0f;
	const float bot_gap = show_bottom ? p_gap : 0.0f;
	const float bot_h = show_bottom ? MIN(124.0f, p_area.size.y * 0.24f) : 0.0f;
	const float main_h = MAX(0.0f, p_area.size.y - bot_h - bot_gap);

	show_left = p_area.size.x >= 620.0f;
	show_right = p_area.size.x >= 820.0f;

	const float left_w = show_left ? MIN(230.0f, p_area.size.x * 0.23f) : 0.0f;
	const float right_w = show_right ? MIN(250.0f, p_area.size.x * 0.25f) : 0.0f;
	const float center_x = p_area.position.x + (show_left ? left_w + p_gap : 0.0f);
	const float center_w = MAX(0.0f, p_area.size.x - (show_left ? left_w + p_gap : 0.0f) -
										 (show_right ? right_w + p_gap : 0.0f));

	left_rect = show_left ? Rect2(p_area.position.x, p_area.position.y, left_w, main_h) : Rect2();
	center_rect = Rect2(center_x, p_area.position.y, center_w, main_h);
	right_rect = show_right ? Rect2(center_x + center_w + p_gap, p_area.position.y, right_w, main_h)
							: Rect2();
	bottom_rect = show_bottom ? Rect2(p_area.position.x, p_area.position.y + main_h + bot_gap,
									p_area.size.x, bot_h)
							  : Rect2();
}

namespace UIDraw
{
float centered_baseline_y(const Ref<Font>& p_font, const Rect2& p_rect, int p_font_size)
{
	if (p_font.is_valid()) {
		float ascent = p_font->get_ascent(p_font_size);
		float descent = p_font->get_descent(p_font_size);
		if (ascent > 0.0f) {
			return Math::round(p_rect.position.y + (p_rect.size.y + ascent - descent) * 0.5f);
		}
	}
	return Math::round(p_rect.position.y + p_rect.size.y * 0.65f);
}

void draw_bordered_rect(
	RID p_ci, const Rect2& p_rect, const Color& p_fill, const Color& p_border, float p_border_w)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_fill);
	const float bw = MIN(p_border_w, MIN(p_rect.size.x * 0.5f, p_rect.size.y * 0.5f));
	if (bw > 0.0f) {
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, p_rect.size.x, bw), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x, p_rect.position.y + p_rect.size.y - bw, p_rect.size.x, bw),
			p_border);
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, bw, p_rect.size.y), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x + p_rect.size.x - bw, p_rect.position.y, bw, p_rect.size.y),
			p_border);
	}
}

void draw_header_banner(
	RID p_ci, const Rect2& p_rect, const Color& p_bg, const Color& p_accent, float p_strip_w)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_rect.position.x, p_rect.position.y, MIN(p_strip_w, p_rect.size.x), p_rect.size.y),
		p_accent);
}

void draw_dock_panel(RID p_ci, const Ref<Font>& p_font, const Rect2& p_rect, const String& p_title,
	const Color& p_dock_bg, const Color& p_hdr_bg, const Color& p_title_col, float p_hdr_h,
	float p_right_reserve)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_dock_bg);
	Rect2 hdr(p_rect.position.x, p_rect.position.y, p_rect.size.x, MIN(p_hdr_h, p_rect.size.y));
	Renderer::canvas_item_add_rect(p_ci, hdr, p_hdr_bg);
	if (p_font.is_valid()) {
		float max_w = MAX(0.0f, p_rect.size.x - 10.0f - p_right_reserve);
		if (max_w > 8.0f) {
			p_font->draw_string(p_ci,
				Point2(p_rect.position.x + 10.0f, centered_baseline_y(p_font, hdr, 12)), p_title,
				HORIZONTAL_ALIGNMENT_LEFT, max_w, 12, p_title_col);
		}
	}
}

void draw_button(RID p_ci, const Ref<Font>& p_font, const UIButton& p_btn, int p_font_size)
{
	if (p_btn.rect.size.x <= 2.0f || p_btn.rect.size.y <= 2.0f) {
		return;
	}
	const bool enabled = (p_btn.is_enabled == nullptr) || p_btn.is_enabled();

	Color bg(0.19f, 0.23f, 0.29f, 1.0f);
	Color accent(0.0f, 0.75f, 0.95f, 1.0f);
	Color text_col(0.92f, 0.94f, 0.96f, 1.0f);

	if (!enabled) {
		bg = Color(0.13f, 0.15f, 0.18f, 1.0f);
		accent = Color(0.20f, 0.24f, 0.29f, 1.0f);
		text_col = Color(0.45f, 0.49f, 0.55f, 1.0f);
	}
	else if (p_btn.primary) {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.0f, 0.50f, 0.68f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.06f, 0.68f, 0.88f, 1.0f);
		}
		else {
			bg = Color(0.02f, 0.58f, 0.78f, 1.0f);
		}
		accent = Color(0.45f, 0.92f, 1.0f, 1.0f);
		text_col = Color(0.98f, 0.99f, 1.0f, 1.0f);
	}
	else if (p_btn.active_tab) {
		bg = p_btn.hovered ? Color(0.24f, 0.34f, 0.44f, 1.0f) : Color(0.20f, 0.30f, 0.40f, 1.0f);
		accent = Color(0.0f, 0.85f, 1.0f, 1.0f);
	}
	else {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.15f, 0.20f, 0.27f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.27f, 0.34f, 0.44f, 1.0f);
		}
	}

	const float strip_h = p_btn.active_tab ? 3.0f : 2.0f;
	Renderer::canvas_item_add_rect(p_ci, p_btn.rect, bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_btn.rect.position.x, p_btn.rect.position.y + p_btn.rect.size.y - strip_h,
			p_btn.rect.size.x, strip_h),
		accent);

	if (p_font.is_valid() && p_btn.rect.size.x > 14.0f) {
		const float pad_x = 6.0f;
		const float avail_w = p_btn.rect.size.x - pad_x * 2.0f;
		const float base_y = centered_baseline_y(p_font, p_btn.rect, p_font_size);
		p_font->draw_string(p_ci, Point2(p_btn.rect.position.x + pad_x, base_y), p_btn.label,
			HORIZONTAL_ALIGNMENT_CENTER, avail_w, p_font_size, text_col);
	}
}

bool clip_segment(const Rect2& p_bounds, Point2& r_a, Point2& r_b)
{
	const float xmin = p_bounds.position.x;
	const float ymin = p_bounds.position.y;
	const float xmax = p_bounds.position.x + p_bounds.size.x;
	const float ymax = p_bounds.position.y + p_bounds.size.y;

	auto outcode = [&](const Point2& p) -> int {
		int code = 0;
		if (p.x < xmin) {
			code |= 1;
		}
		else if (p.x > xmax) {
			code |= 2;
		}
		if (p.y < ymin) {
			code |= 4;
		}
		else if (p.y > ymax) {
			code |= 8;
		}
		return code;
	};

	int c0 = outcode(r_a);
	int c1 = outcode(r_b);
	for (int iter = 0; iter < 12; iter++) {
		if (!(c0 | c1)) {
			return true;
		}
		if (c0 & c1) {
			return false;
		}
		int cout = c0 ? c0 : c1;
		Point2 p;
		if (cout & 8) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymax - r_a.y) / (r_b.y - r_a.y);
			p.y = ymax;
		}
		else if (cout & 4) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymin - r_a.y) / (r_b.y - r_a.y);
			p.y = ymin;
		}
		else if (cout & 2) {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmax - r_a.x) / (r_b.x - r_a.x);
			p.x = xmax;
		}
		else {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmin - r_a.x) / (r_b.x - r_a.x);
			p.x = xmin;
		}
		if (cout == c0) {
			r_a = p;
			c0 = outcode(r_a);
		}
		else {
			r_b = p;
			c1 = outcode(r_b);
		}
	}
	return false;
}

void draw_clipped_line(
	RID p_ci, const Rect2& p_clip, Point2 p_a, Point2 p_b, const Color& p_col, float p_width)
{
	if (clip_segment(p_clip, p_a, p_b)) {
		Renderer::canvas_item_add_line(p_ci, p_a, p_b, p_col, p_width, true);
	}
}

bool read_voltaire_project(const String& p_dir, VoltaireProjectFile* r_out)
{
	if (!r_out) {
		return false;
	}
	String proj_file = p_dir.path_join("project.vltr");
	Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::READ);
	if (fa.is_null() || fa->get_length() < (int64_t)sizeof(VoltaireProjectFile)) {
		return false;
	}
	fa->get_buffer((uint8_t*)r_out, sizeof(VoltaireProjectFile));
	return vltr_project_valid(r_out);
}

String read_project_name(const String& p_dir)
{
	VoltaireProjectFile proj;
	if (read_voltaire_project(p_dir, &proj) && proj.config.project_name[0] != '\0') {
		return String::utf8(proj.config.project_name);
	}
	String fallback = p_dir.get_file();
	return fallback.is_empty() ? p_dir : fallback;
}
} // namespace UIDraw
