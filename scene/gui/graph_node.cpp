/**************************************************************************/
/*  graph_node.cpp                                                        */
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

#include "graph_node.h"

#include "scene/gui/graph_edit.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void GraphNode::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;

	if (mode == MODE_ELEMENT) {
		if (titlebar_hbox) {
			remove_child(titlebar_hbox);
			memdelete(titlebar_hbox);
			titlebar_hbox = nullptr;
			title_label = nullptr;
		}
		set_focus_mode(FOCUS_NONE);
	}
	else {
		if (!titlebar_hbox) {
			titlebar_hbox = Control::create_hbox();
			titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->set_use_parent_material(true);
			add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

			title_label = memnew(Label);
			title_label->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->add_child(title_label);
		}
		if (title_label) {
			title_label->set_theme_type_variation(
				mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
			title_label->set_horizontal_alignment(
				mode == MODE_FRAME ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
			title_label->set_use_parent_material(mode == MODE_FRAME);
			title_label->set_text(title);
		}
		set_focus_mode(mode == MODE_NODE ? FOCUS_ACCESSIBILITY : FOCUS_NONE);
	}
	queue_redraw();
}

#ifdef TOOLS_ENABLED
void GraphNode::_edit_set_position(const Point2& p_position) {}
#endif

void GraphNode::set_position_offset(const Vector2& p_offset)
{
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	queue_redraw();
}

void GraphNode::set_selected(bool p_selected)
{
	if (!is_selectable() && p_selected) {
		return;
	}
	if (selected == p_selected) {
		return;
	}
	selected = p_selected;
	queue_redraw();
}

void GraphNode::set_drag(bool p_drag)
{
	if (p_drag) {
		drag_from = get_position_offset();
	}
}

void GraphNode::set_resizable(bool p_enable)
{
	if (resizable == p_enable) {
		return;
	}
	resizable = p_enable;
	queue_redraw();
}

void GraphNode::set_selectable(bool p_selectable)
{
	if (!p_selectable) {
		set_selected(false);
	}
	selectable = p_selectable;
}

void GraphNode::set_title(const String& p_title)
{
	if (title == p_title) {
		return;
	}
	title = p_title;
	if (title_label) {
		title_label->set_text(title);
	}
	update_minimum_size();
}

Size2 GraphNode::get_titlebar_size() const
{
	if (!titlebar_hbox) {
		return Size2();
	}
	Size2 min_size = titlebar_hbox->get_bound_minimum_size();
	if (theme_cache.titlebar.is_valid()) {
		min_size += theme_cache.titlebar->get_minimum_size();
	}
	return min_size;
}

void GraphNode::set_autoshrink_enabled(bool p_enable)
{
	if (autoshrink_enabled == p_enable) {
		return;
	}
	autoshrink_enabled = p_enable;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_autoshrink_margin(const int& p_margin)
{
	if (autoshrink_margin == p_margin) {
		return;
	}
	autoshrink_margin = p_margin;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_tint_color_enabled(bool p_enable)
{
	if (tint_color_enabled == p_enable) {
		return;
	}
	tint_color_enabled = p_enable;
	queue_redraw();
}

void GraphNode::set_tint_color(const Color& p_tint_color)
{
	if (tint_color == p_tint_color) {
		return;
	}
	tint_color = p_tint_color;
	queue_redraw();
}

void GraphNode::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
	} break;

	case NOTIFICATION_DRAW: {
		if (mode == MODE_FRAME) {
			if (theme_cache.panel.is_null() || theme_cache.titlebar.is_null() || !titlebar_hbox) {
				break;
			}
			// Used for layout calculations.
			Ref<StyleBox> sb_panel = theme_cache.panel;
			Ref<StyleBox> sb_titlebar = theme_cache.titlebar;

			// Used for drawing.
			Ref<StyleBox> sb_to_draw_panel =
				(selected && theme_cache.panel_selected.is_valid()) ? theme_cache.panel_selected
																	: sb_panel;
			Ref<StyleBox> sb_to_draw_titlebar =
				(selected && theme_cache.titlebar_selected.is_valid())
					? theme_cache.titlebar_selected
					: sb_titlebar;

			Rect2 titlebar_rect(
				Point2(), titlebar_hbox->get_bound_minimum_size() + sb_titlebar->get_minimum_size());
			Size2 body_size = get_size();
			titlebar_rect.size.width = body_size.width;
			body_size.y -= titlebar_rect.size.height;
			Rect2 body_rect(Point2(0, titlebar_rect.size.height), body_size);

			// Draw body stylebox.
			draw_style_box(sb_to_draw_panel.ptr(), body_rect);

			// Draw title bar stylebox above.
			draw_style_box(sb_to_draw_titlebar.ptr(), titlebar_rect);

			// Only draw the resize handle if the frame is not auto-resizing.
			if (resizable && !autoshrink_enabled && theme_cache.resizer.is_valid()) {
				Ref<Texture2D> resizer = theme_cache.resizer;
				draw_texture(
					resizer.ptr(), get_size() - resizer->get_size(), theme_cache.resizer_color);
			}
		}
	} break;
	}
}

void GraphNode::draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color)
{
	if (mode != MODE_NODE) {
		return;
	}
	Slot slot = slot_table[p_slot_index];
	Ref<Texture2D> port_icon = p_left ? slot.custom_port_icon_left : slot.custom_port_icon_right;

	if (port_icon.is_null()) {
		port_icon = theme_cache.port;
	}
	if (port_icon.is_null()) {
		return;
	}

	Point2 icon_offset = -port_icon->get_size() * 0.5;
	port_icon->draw(get_canvas_item(), p_pos + icon_offset, p_color);
}

void GraphNode::set_slot(int p_slot_index, bool p_enable_left, int p_type_left,
	const Color& p_color_left, bool p_enable_right, int p_type_right, const Color& p_color_right,
	const Ref<Texture2D>& p_custom_left, const Ref<Texture2D>& p_custom_right, bool p_draw_stylebox)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set slot with index (%d) lesser than zero.", p_slot_index));

	if (!p_enable_left && p_type_left == 0 && p_color_left == Color(1, 1, 1, 1) &&
		!p_enable_right && p_type_right == 0 && p_color_right == Color(1, 1, 1, 1) &&
		p_custom_left.is_null() && p_custom_right.is_null()) {
		slot_table.erase(p_slot_index);
		return;
	}

	Slot s;
	s.enable_left = p_enable_left;
	s.type_left = p_type_left;
	s.color_left = p_color_left;
	s.custom_port_icon_left = p_custom_left;
	s.enable_right = p_enable_right;
	s.type_right = p_type_right;
	s.color_right = p_color_right;
	s.custom_port_icon_right = p_custom_right;
	s.draw_stylebox = p_draw_stylebox;
	slot_table[p_slot_index] = s;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_slot(int p_slot_index)
{
	slot_table.erase(p_slot_index);
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_all_slots()
{
	slot_table.clear();
	port_pos_dirty = true;
	queue_redraw();
}

bool GraphNode::is_slot_enabled_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_left;
}

void GraphNode::set_slot_enabled_left(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_left for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_left = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_left(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_left = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_left;
}

void GraphNode::set_slot_color_left(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_left = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_left;
}

void GraphNode::set_slot_custom_icon_left(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_left = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_left;
}

bool GraphNode::is_slot_enabled_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_right;
}

void GraphNode::set_slot_enabled_right(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_right for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_right = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_right(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_right = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_right;
}

void GraphNode::set_slot_color_right(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_right = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_right;
}

void GraphNode::set_slot_custom_icon_right(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_right = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_right;
}

bool GraphNode::is_slot_draw_stylebox(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].draw_stylebox;
}

void GraphNode::set_slot_draw_stylebox(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].draw_stylebox = p_enable;
	queue_redraw();
}

void GraphNode::set_ignore_invalid_connection_type(bool p_ignore)
{
	ignore_invalid_connection_type = p_ignore;
}

bool GraphNode::is_ignoring_valid_connection_type() const { return ignore_invalid_connection_type; }

bool GraphNode::has_point(const Point2& p_point) const
{
	if (mode == MODE_FRAME) {
		if (theme_cache.resizer.is_valid()) {
			Ref<Texture2D> resizer = theme_cache.resizer;
			if (Rect2(get_size() - resizer->get_size(), resizer->get_size()).has_point(p_point)) {
				return true;
			}
		}

		// For grabbing on the titlebar.
		int titlebar_height = get_titlebar_size().height;
		if (Rect2(0, 0, get_size().width, titlebar_height).has_point(p_point)) {
			return true;
		}

		// Allow grabbing on all sides of the frame.
		Rect2 frame_rect = Rect2(0, 0, get_size().width, get_size().height);
		Rect2 no_drag_rect = frame_rect.grow(-drag_margin);

		if (frame_rect.has_point(p_point) && !no_drag_rect.has_point(p_point)) {
			return true;
		}

		return false;
	}

	return Control::has_point(p_point);
}

Size2 GraphNode::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GraphNode::get_desired_size() const { return _get_minimum_size(true); }

int GraphNode::get_input_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return left_port_cache.size();
}

int GraphNode::get_output_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return right_port_cache.size();
}

Vector2 GraphNode::get_input_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Vector2());
	return left_port_cache[p_port_idx].pos;
}

int GraphNode::get_input_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), 0);
	return left_port_cache[p_port_idx].type;
}

Color GraphNode::get_input_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Color());
	return left_port_cache[p_port_idx].color;
}

int GraphNode::get_input_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), -1);
	return left_port_cache[p_port_idx].slot_index;
}

Vector2 GraphNode::get_output_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Vector2());
	return right_port_cache[p_port_idx].pos;
}

int GraphNode::get_output_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), 0);
	return right_port_cache[p_port_idx].type;
}

Color GraphNode::get_output_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Color());
	return right_port_cache[p_port_idx].color;
}

int GraphNode::get_output_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), -1);
	return right_port_cache[p_port_idx].slot_index;
}

Control::CursorShape GraphNode::get_cursor_shape(const Point2& p_pos) const
{
	const bool can_resize = (mode == MODE_FRAME) ? (resizable && !autoshrink_enabled) : resizable;
	if (can_resize && theme_cache.resizer.is_valid()) {
		if (resizing || (p_pos.x > get_size().x - theme_cache.resizer->get_width() &&
							p_pos.y > get_size().y - theme_cache.resizer->get_height())) {
			return CURSOR_FDIAGSIZE;
		}
	}

	return Control::get_cursor_shape(p_pos);
}

Vector<int> GraphNode::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> GraphNode::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_EXPAND);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

void GraphNode::set_slots_focus_mode(Control::FocusMode p_focus_mode)
{
	slots_focus_mode = p_focus_mode;
}

Control::FocusMode GraphNode::get_slots_focus_mode() const { return slots_focus_mode; }

GraphNode::GraphNode(Mode p_mode) : mode(p_mode)
{
	if (mode != MODE_ELEMENT) {
		titlebar_hbox = Control::create_hbox();
		titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
		titlebar_hbox->set_use_parent_material(true);
		add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

		title_label = memnew(Label);
		title_label->set_theme_type_variation(
			mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
		title_label->set_h_size_flags(SIZE_EXPAND_FILL);
		if (mode == MODE_FRAME) {
			title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			title_label->set_use_parent_material(true);
		}
		titlebar_hbox->add_child(title_label);
	}

	set_mouse_filter(MOUSE_FILTER_STOP);
	if (mode == MODE_NODE) {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
}

void GraphNode::_port_pos_update()
{
	port_pos_dirty = false;
}

Size2 GraphNode::_get_minimum_size(bool) const { return Size2(); }

String GraphNode::get_accessibility_container_name(const Node* p_child) const { return String(); }

String GraphNode::_get_accessibility_name() const { return String(); }

void GraphNode::_resort()
{
	if (mode == MODE_FRAME && titlebar_hbox && theme_cache.titlebar.is_valid()) {
		Ref<StyleBox> sb_titlebar = theme_cache.titlebar;
		Size2 titlebar_size = Size2(get_size().width, titlebar_hbox->get_size().height);
		titlebar_size -= sb_titlebar->get_minimum_size();
		Rect2 titlebar_rect = Rect2(sb_titlebar->get_offset(), titlebar_size);
		fit_child_in_rect(titlebar_hbox, titlebar_rect);
	}
}
