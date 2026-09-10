/**************************************************************************/
/*  graph_node.cpp                                                        */
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

#include "graph_node.h"
#include "scene/gui/box_container.h"
#include "scene/gui/graph_edit.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

void GraphNode::draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color)
{
	Slot slot = slot_table[p_slot_index];
	Ref<Texture2D> port_icon = p_left ? slot.custom_port_icon_left : slot.custom_port_icon_right;

	Point2 icon_offset;
	if (port_icon.is_null()) {
		port_icon = theme_cache.port;
	}

	icon_offset = -port_icon->get_size() * 0.5;
	port_icon->draw(get_canvas_item(), p_pos + icon_offset, p_color);
}

bool GraphNode::is_slot_enabled_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_left;
}

int GraphNode::get_slot_type_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_left;
}

Color GraphNode::get_slot_color_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_left;
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

int GraphNode::get_slot_type_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_right;
}

Color GraphNode::get_slot_color_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_right;
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

void GraphNode::set_ignore_invalid_connection_type(bool p_ignore)
{
	ignore_invalid_connection_type = p_ignore;
}

bool GraphNode::is_ignoring_valid_connection_type() const { return ignore_invalid_connection_type; }

Size2 GraphNode::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GraphNode::get_desired_size() const { return _get_minimum_size(true); }

int GraphNode::get_input_port_count()
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return left_port_cache.size();
}

int GraphNode::get_output_port_count()
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return right_port_cache.size();
}

Vector2 GraphNode::get_input_port_position(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Vector2());
	Vector2 pos = left_port_cache[p_port_idx].pos;
	return pos;
}

int GraphNode::get_input_port_type(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), 0);
	return left_port_cache[p_port_idx].type;
}

Color GraphNode::get_input_port_color(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Color());
	return left_port_cache[p_port_idx].color;
}

int GraphNode::get_input_port_slot(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), -1);
	return left_port_cache[p_port_idx].slot_index;
}

Vector2 GraphNode::get_output_port_position(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Vector2());
	Vector2 pos = right_port_cache[p_port_idx].pos;
	return pos;
}

int GraphNode::get_output_port_type(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), 0);
	return right_port_cache[p_port_idx].type;
}

Color GraphNode::get_output_port_color(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Color());
	return right_port_cache[p_port_idx].color;
}

int GraphNode::get_output_port_slot(int p_port_idx)
{
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), -1);
	return right_port_cache[p_port_idx].slot_index;
}

String GraphNode::get_title() const { return title; }

HBoxContainer* GraphNode::get_titlebar_hbox() { return titlebar_hbox; }

Control::CursorShape GraphNode::get_cursor_shape(const Point2& p_pos) const
{
	if (resizable) {
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

Control::FocusMode GraphNode::get_slots_focus_mode() const { return slots_focus_mode; }

GraphNode::GraphNode()
{
	titlebar_hbox = memnew(HBoxContainer);
	titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	titlebar_hbox->set_use_parent_material(true);
	add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

	title_label = memnew(Label);
	title_label->set_theme_type_variation("GraphNodeTitleLabel");
	title_label->set_h_size_flags(SIZE_EXPAND_FILL);
	titlebar_hbox->set_use_parent_material(true);
	titlebar_hbox->add_child(title_label);

	set_mouse_filter(MOUSE_FILTER_STOP);
	set_focus_mode(FOCUS_ACCESSIBILITY);
}


