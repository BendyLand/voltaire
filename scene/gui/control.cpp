/**************************************************************************/
/*  control.cpp                                                           */
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

#include "control.compat.inc"
#include "control.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input_map.h"
#include "core/math/transform_2d.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"
#include "scene/gui/container.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "scene/theme/theme_owner.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/text/text_server.h"

#ifdef TOOLS_ENABLED
#include "editor/scene/gui/control_editor_plugin.h"
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
void Control::_edit_set_position(const Point2& p_position)
{
	ERR_FAIL_COND_MSG(!Engine::get_singleton()->is_editor_hint(),
		"This function can only be used from editor plugins.");
	set_position(p_position,
		ControlEditorToolbar::get_singleton()->is_anchors_mode_enabled() && get_parent_control());
}

Point2 Control::_edit_get_position() const { return get_position(); }

void Control::_edit_set_scale(const Size2& p_scale) { set_scale(p_scale); }

Size2 Control::_edit_get_scale() const { return data.scale; }

void Control::_edit_set_rect(const Rect2& p_edit_rect)
{
	ERR_FAIL_COND_MSG(!Engine::get_singleton()->is_editor_hint(),
		"This function can only be used from editor plugins.");
	// Changing the size might change the internal transform (in case of non-zero
	// `pivot_offset_ratio`), hence `position` (which is in the parent space, and is not always
	// equivalent to the Control's rect top-left corner) needs to be changed after `size` (which is
	// local) and needs to account for the possibly changed internal transform.
	Vector2 rect_new_pos_in_parent_space = get_transform().xform(p_edit_rect.position);
	set_size(p_edit_rect.size, ControlEditorToolbar::get_singleton()->is_anchors_mode_enabled());
	set_position(rect_new_pos_in_parent_space - _get_internal_transform().get_origin(),
		ControlEditorToolbar::get_singleton()->is_anchors_mode_enabled());
}

void Control::_edit_set_rotation(real_t p_rotation) { set_rotation(p_rotation); }

real_t Control::_edit_get_rotation() const { return get_rotation(); }

bool Control::_edit_use_rotation() const { return true; }

void Control::_edit_set_pivot(const Point2& p_pivot)
{
	Vector2 delta_pivot = p_pivot - get_pivot_offset();
	Vector2 move = Vector2(
		(std::cos(data.rotation) - 1.0) * delta_pivot.x - std::sin(data.rotation) * delta_pivot.y,
		std::sin(data.rotation) * delta_pivot.x + (std::cos(data.rotation) - 1.0) * delta_pivot.y);
	set_position(get_position() + move);
	set_pivot_offset(p_pivot);
	set_pivot_offset_ratio(Vector2());
}

Point2 Control::_edit_get_pivot() const { return get_combined_pivot_offset(); }

bool Control::_edit_use_pivot() const { return true; }

Size2 Control::_edit_get_minimum_size() const { return get_combined_minimum_size(); }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 Control::_edit_get_rect() const { return Rect2(Point2(), get_size()); }

bool Control::_edit_use_rect() const { return true; }
#endif // DEBUG_ENABLED

void Control::reparent(Node* p_parent, bool p_keep_global_transform)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_keep_global_transform) {
		Transform2D temp = get_global_transform();
		Node::reparent(p_parent);
		set_global_position(temp.get_origin());
	}
	else {
		Node::reparent(p_parent);
	}
}

int Control::root_layout_direction = 0;

void Control::set_root_layout_direction(int p_root_dir) { root_layout_direction = p_root_dir; }

PackedStringArray Control::get_configuration_warnings() const
{
	ERR_READ_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray warnings = CanvasItem::get_configuration_warnings();

	if (data.mouse_filter == MOUSE_FILTER_IGNORE && !data.tooltip.is_empty()) {
		warnings.push_back(
			RTR("The Hint Tooltip won't be displayed as the control's Mouse Filter is set to "
				"\"Ignore\". To solve this, set the Mouse Filter to \"Stop\" or \"Pass\"."));
	}

	return warnings;
}

String Control::_get_accessibility_name() const
{
	if (get_parent_control()) {
		String container_info = get_parent_control()->get_accessibility_container_name(this);
		return container_info.is_empty() ? get_accessibility_name()
										 : get_accessibility_name() + " " + container_info;
	}
	else {
		return get_accessibility_name();
	}
}

bool Control::is_text_field() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return false;
}

String Control::properties_managed_by_container[] = {"offset_left", "offset_top", "offset_right",
	"offset_bottom", "anchor_left", "anchor_top", "anchor_right", "anchor_bottom", "position",
	"rotation", "scale", "size"};

bool Control::_property_can_revert(const StringName& p_name) const
{
	if (p_name == "layout_mode" || p_name == "anchors_preset") {
		return true;
	}

	return false;
}

Control* Control::get_parent_control() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return data.parent_control;
}

Window* Control::get_parent_window() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return data.parent_window;
}

Rect2 Control::get_parent_anchorable_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	if (!is_inside_tree()) {
		return Rect2();
	}

	Rect2 parent_rect;
	if (data.parent_canvas_item) {
		parent_rect = data.parent_canvas_item->get_anchorable_rect();
	}
	else {
#ifdef TOOLS_ENABLED
		Node* edited_scene_root = get_tree()->get_edited_scene_root();
		Node* scene_root_parent = edited_scene_root ? edited_scene_root->get_parent() : nullptr;

		if (scene_root_parent && get_viewport() == scene_root_parent->get_viewport()) {
			parent_rect.size =
				Size2(GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_width"),
					GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_height"));
		}
		else {
			parent_rect = get_viewport()->get_visible_rect();
		}

#else
		parent_rect = get_viewport()->get_visible_rect();
#endif // TOOLS_ENABLED
	}

	return parent_rect;
}

Size2 Control::get_parent_area_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return get_parent_anchorable_rect().size;
}

Transform2D Control::_get_internal_transform() const
{
	// T(pivot_offset) * R(rotation) * S(scale) * T(-pivot_offset)
	Transform2D xform(data.rotation, data.scale, 0.0f, get_combined_pivot_offset());
	xform.translate_local(-get_combined_pivot_offset());

	if (is_offset_transform_enabled() && !data.offset_transform->visual_only) {
		xform *= get_offset_transform();
	}

	return xform;
}

void Control::_update_canvas_item_transform()
{
	Transform2D xform = _get_internal_transform();
	xform[2] += get_position();

	// We use a little workaround to avoid flickering when moving the pivot with _edit_set_pivot()
	if (is_inside_tree() && Math::abs(Math::sin(data.rotation * 4.0f)) < 0.00001f &&
		get_viewport()->is_snap_controls_to_pixels_enabled()) {
		xform[2] = (xform[2] + Vector2(0.5, 0.5)).floor();
	}

	if (is_offset_transform_enabled() && data.offset_transform->visual_only) {
		xform *= get_offset_transform();
	}

	RenderingServer::get_singleton()->canvas_item_set_transform(get_canvas_item(), xform);
}

Transform2D Control::get_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	Transform2D xform = _get_internal_transform();
	xform[2] += get_position();
	return xform;
}

void Control::_top_level_changed_on_parent()
{
	// Update root control status.
	_notification(NOTIFICATION_EXIT_CANVAS);
	_notification(NOTIFICATION_ENTER_CANVAS);
}

void Control::_set_anchor(Side p_side, real_t p_anchor) { set_anchor(p_side, p_anchor); }

real_t Control::get_anchor(Side p_side) const
{
	ERR_READ_THREAD_GUARD_V(0);
	ERR_FAIL_INDEX_V(int(p_side), 4, 0.0);

	return data.anchor[p_side];
}

void Control::set_offset(Side p_side, real_t p_value)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_side, 4);
	if (data.offset[p_side] == p_value) {
		return;
	}

	data.offset[p_side] = p_value;
	_size_changed();
}

real_t Control::get_offset(Side p_side) const
{
	ERR_READ_THREAD_GUARD_V(0);
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);

	return data.offset[p_side];
}

void Control::set_anchor_and_offset(
	Side p_side, real_t p_anchor, real_t p_pos, bool p_push_opposite_anchor)
{
	ERR_MAIN_THREAD_GUARD;
	set_anchor(p_side, p_anchor, false, p_push_opposite_anchor);
	set_offset(p_side, p_pos);
}

void Control::set_begin(const Point2& p_point)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!std::isfinite(p_point.x) || !std::isfinite(p_point.y));
	if (data.offset[0] == p_point.x && data.offset[1] == p_point.y) {
		return;
	}

	data.offset[0] = p_point.x;
	data.offset[1] = p_point.y;
	_size_changed();
}

Point2 Control::get_begin() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return Point2(data.offset[0], data.offset[1]);
}

void Control::set_end(const Point2& p_point)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.offset[2] == p_point.x && data.offset[3] == p_point.y) {
		return;
	}

	data.offset[2] = p_point.x;
	data.offset[3] = p_point.y;
	_size_changed();
}

Point2 Control::get_end() const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	return Point2(data.offset[2], data.offset[3]);
}

void Control::set_h_grow_direction(GrowDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.h_grow == p_direction) {
		return;
	}

	ERR_FAIL_INDEX((int)p_direction, 3);

	data.h_grow = p_direction;
	_size_changed();
}

Control::GrowDirection Control::get_h_grow_direction() const
{
	ERR_READ_THREAD_GUARD_V(GROW_DIRECTION_BEGIN);
	return data.h_grow;
}

void Control::set_v_grow_direction(GrowDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.v_grow == p_direction) {
		return;
	}

	ERR_FAIL_INDEX((int)p_direction, 3);

	data.v_grow = p_direction;
	_size_changed();
}

Control::GrowDirection Control::get_v_grow_direction() const
{
	ERR_READ_THREAD_GUARD_V(GROW_DIRECTION_BEGIN);
	return data.v_grow;
}

void Control::_compute_layout_rect(Rect2 p_rect, bool p_keep_offsets)
{
	Size2 parent_rect_size = get_parent_anchorable_rect().size;

	if (p_keep_offsets) {
		// If computing anchors, we need to ensure the parent rect size is valid to avoid division
		// by zero.
		ERR_FAIL_COND(parent_rect_size.x == 0.0);
		ERR_FAIL_COND(parent_rect_size.y == 0.0);
	}

	real_t x = p_rect.position.x;
	real_t y = p_rect.position.y;

	if (_get_layout_mode() != LayoutMode::LAYOUT_MODE_CONTAINER) {
		float left_grow_factor = (data.h_grow == GROW_DIRECTION_BEGIN) ? 1.0f
								 : (data.h_grow == GROW_DIRECTION_END) ? 0.0f
																	   : 0.5f;
		float top_grow_factor = (data.v_grow == GROW_DIRECTION_BEGIN) ? 1.0f
								: (data.v_grow == GROW_DIRECTION_END) ? 0.0f
																	  : 0.5f;

		Size2 size_diff = p_rect.size - data.size_cache;

		x -= size_diff.x * (is_layout_rtl() ? (1.0f - left_grow_factor) : left_grow_factor);
		y -= size_diff.y * top_grow_factor;
	}

	if (is_layout_rtl()) {
		x = parent_rect_size.x - x - p_rect.size.x;
	}

	if (p_keep_offsets) {
		data.anchor[0] = (x - data.offset[0]) / parent_rect_size.x;
		data.anchor[1] = (y - data.offset[1]) / parent_rect_size.y;
		data.anchor[2] = (x + p_rect.size.x - data.offset[2]) / parent_rect_size.x;
		data.anchor[3] = (y + p_rect.size.y - data.offset[3]) / parent_rect_size.y;
	}
	else {
		data.offset[0] = x - (data.anchor[0] * parent_rect_size.x);
		data.offset[1] = y - (data.anchor[1] * parent_rect_size.y);
		data.offset[2] = x + p_rect.size.x - (data.anchor[2] * parent_rect_size.x);
		data.offset[3] = y + p_rect.size.y - (data.anchor[3] * parent_rect_size.y);
	}
}

int Control::_get_anchors_layout_preset() const
{
	// If this is a layout mode that doesn't rely on anchors, avoid excessive checks.
	if (data.stored_layout_mode != LayoutMode::LAYOUT_MODE_UNCONTROLLED &&
		data.stored_layout_mode != LayoutMode::LAYOUT_MODE_ANCHORS) {
		return LayoutPreset::PRESET_TOP_LEFT;
	}

	// If the custom preset was selected by user, use it.
	if (data.stored_use_custom_anchors) {
		return -1;
	}

	// Check anchors to determine if the current state matches a preset, or not.

	float left = get_anchor(SIDE_LEFT);
	float right = get_anchor(SIDE_RIGHT);
	float top = get_anchor(SIDE_TOP);
	float bottom = get_anchor(SIDE_BOTTOM);

	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_BEGIN && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_BEGIN) {
		return (int)LayoutPreset::PRESET_TOP_LEFT;
	}
	if (left == (float)ANCHOR_END && right == (float)ANCHOR_END && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_BEGIN) {
		return (int)LayoutPreset::PRESET_TOP_RIGHT;
	}
	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_BEGIN && top == (float)ANCHOR_END &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_BOTTOM_LEFT;
	}
	if (left == (float)ANCHOR_END && right == (float)ANCHOR_END && top == (float)ANCHOR_END &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_BOTTOM_RIGHT;
	}

	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_BEGIN && top == 0.5 &&
		bottom == 0.5) {
		return (int)LayoutPreset::PRESET_CENTER_LEFT;
	}
	if (left == (float)ANCHOR_END && right == (float)ANCHOR_END && top == 0.5 && bottom == 0.5) {
		return (int)LayoutPreset::PRESET_CENTER_RIGHT;
	}
	if (left == 0.5 && right == 0.5 && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_BEGIN) {
		return (int)LayoutPreset::PRESET_CENTER_TOP;
	}
	if (left == 0.5 && right == 0.5 && top == (float)ANCHOR_END && bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_CENTER_BOTTOM;
	}
	if (left == 0.5 && right == 0.5 && top == 0.5 && bottom == 0.5) {
		return (int)LayoutPreset::PRESET_CENTER;
	}

	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_BEGIN && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_LEFT_WIDE;
	}
	if (left == (float)ANCHOR_END && right == (float)ANCHOR_END && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_RIGHT_WIDE;
	}
	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_END && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_BEGIN) {
		return (int)LayoutPreset::PRESET_TOP_WIDE;
	}
	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_END && top == (float)ANCHOR_END &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_BOTTOM_WIDE;
	}

	if (left == 0.5 && right == 0.5 && top == (float)ANCHOR_BEGIN && bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_VCENTER_WIDE;
	}
	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_END && top == 0.5 && bottom == 0.5) {
		return (int)LayoutPreset::PRESET_HCENTER_WIDE;
	}

	if (left == (float)ANCHOR_BEGIN && right == (float)ANCHOR_END && top == (float)ANCHOR_BEGIN &&
		bottom == (float)ANCHOR_END) {
		return (int)LayoutPreset::PRESET_FULL_RECT;
	}

	// Does not match any preset, return "Custom".
	return -1;
}

void Control::set_anchors_preset(LayoutPreset p_preset, bool p_keep_offsets)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_preset, 16);

	// Left
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_BOTTOM_LEFT:
	case PRESET_CENTER_LEFT:
	case PRESET_TOP_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_LEFT_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_anchor(SIDE_LEFT, ANCHOR_BEGIN, p_keep_offsets);
		break;

	case PRESET_CENTER_TOP:
	case PRESET_CENTER_BOTTOM:
	case PRESET_CENTER:
	case PRESET_VCENTER_WIDE:
		set_anchor(SIDE_LEFT, 0.5, p_keep_offsets);
		break;

	case PRESET_TOP_RIGHT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_RIGHT:
	case PRESET_RIGHT_WIDE:
		set_anchor(SIDE_LEFT, ANCHOR_END, p_keep_offsets);
		break;
	}

	// Top
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_TOP_RIGHT:
	case PRESET_CENTER_TOP:
	case PRESET_LEFT_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_TOP_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_anchor(SIDE_TOP, ANCHOR_BEGIN, p_keep_offsets);
		break;

	case PRESET_CENTER_LEFT:
	case PRESET_CENTER_RIGHT:
	case PRESET_CENTER:
	case PRESET_HCENTER_WIDE:
		set_anchor(SIDE_TOP, 0.5, p_keep_offsets);
		break;

	case PRESET_BOTTOM_LEFT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_BOTTOM:
	case PRESET_BOTTOM_WIDE:
		set_anchor(SIDE_TOP, ANCHOR_END, p_keep_offsets);
		break;
	}

	// Right
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_BOTTOM_LEFT:
	case PRESET_CENTER_LEFT:
	case PRESET_LEFT_WIDE:
		set_anchor(SIDE_RIGHT, ANCHOR_BEGIN, p_keep_offsets);
		break;

	case PRESET_CENTER_TOP:
	case PRESET_CENTER_BOTTOM:
	case PRESET_CENTER:
	case PRESET_VCENTER_WIDE:
		set_anchor(SIDE_RIGHT, 0.5, p_keep_offsets);
		break;

	case PRESET_TOP_RIGHT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_RIGHT:
	case PRESET_TOP_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_anchor(SIDE_RIGHT, ANCHOR_END, p_keep_offsets);
		break;
	}

	// Bottom
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_TOP_RIGHT:
	case PRESET_CENTER_TOP:
	case PRESET_TOP_WIDE:
		set_anchor(SIDE_BOTTOM, ANCHOR_BEGIN, p_keep_offsets);
		break;

	case PRESET_CENTER_LEFT:
	case PRESET_CENTER_RIGHT:
	case PRESET_CENTER:
	case PRESET_HCENTER_WIDE:
		set_anchor(SIDE_BOTTOM, 0.5, p_keep_offsets);
		break;

	case PRESET_BOTTOM_LEFT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_BOTTOM:
	case PRESET_LEFT_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_anchor(SIDE_BOTTOM, ANCHOR_END, p_keep_offsets);
		break;
	}
}

void Control::set_offsets_preset(
	LayoutPreset p_preset, LayoutPresetMode p_resize_mode, int p_margin)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_preset, 16);
	ERR_FAIL_INDEX((int)p_resize_mode, 4);

	// Calculate the size if the node is not resized
	Size2 min_size = get_minimum_size();
	Size2 new_size = get_size();
	if (p_resize_mode == PRESET_MODE_MINSIZE || p_resize_mode == PRESET_MODE_KEEP_HEIGHT) {
		new_size.x = min_size.x;
	}
	if (p_resize_mode == PRESET_MODE_MINSIZE || p_resize_mode == PRESET_MODE_KEEP_WIDTH) {
		new_size.y = min_size.y;
	}

	Rect2 parent_rect = get_parent_anchorable_rect();

	real_t x = parent_rect.size.x;
	if (is_layout_rtl()) {
		x = parent_rect.size.x - x - new_size.x;
	}
	// Left
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_BOTTOM_LEFT:
	case PRESET_CENTER_LEFT:
	case PRESET_TOP_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_LEFT_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		data.offset[0] = x * (0.0 - data.anchor[0]) + p_margin + parent_rect.position.x;
		break;

	case PRESET_CENTER_TOP:
	case PRESET_CENTER_BOTTOM:
	case PRESET_CENTER:
	case PRESET_VCENTER_WIDE:
		data.offset[0] = x * (0.5 - data.anchor[0]) - new_size.x / 2 + parent_rect.position.x;
		break;

	case PRESET_TOP_RIGHT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_RIGHT:
	case PRESET_RIGHT_WIDE:
		data.offset[0] =
			x * (1.0 - data.anchor[0]) - new_size.x - p_margin + parent_rect.position.x;
		break;
	}

	// Top
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_TOP_RIGHT:
	case PRESET_CENTER_TOP:
	case PRESET_LEFT_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_TOP_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_FULL_RECT:
		data.offset[1] =
			parent_rect.size.y * (0.0 - data.anchor[1]) + p_margin + parent_rect.position.y;
		break;

	case PRESET_CENTER_LEFT:
	case PRESET_CENTER_RIGHT:
	case PRESET_CENTER:
	case PRESET_HCENTER_WIDE:
		data.offset[1] =
			parent_rect.size.y * (0.5 - data.anchor[1]) - new_size.y / 2 + parent_rect.position.y;
		break;

	case PRESET_BOTTOM_LEFT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_BOTTOM:
	case PRESET_BOTTOM_WIDE:
		data.offset[1] = parent_rect.size.y * (1.0 - data.anchor[1]) - new_size.y - p_margin +
						 parent_rect.position.y;
		break;
	}

	// Right
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_BOTTOM_LEFT:
	case PRESET_CENTER_LEFT:
	case PRESET_LEFT_WIDE:
		data.offset[2] =
			x * (0.0 - data.anchor[2]) + new_size.x + p_margin + parent_rect.position.x;
		break;

	case PRESET_CENTER_TOP:
	case PRESET_CENTER_BOTTOM:
	case PRESET_CENTER:
	case PRESET_VCENTER_WIDE:
		data.offset[2] = x * (0.5 - data.anchor[2]) + new_size.x / 2 + parent_rect.position.x;
		break;

	case PRESET_TOP_RIGHT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_RIGHT:
	case PRESET_TOP_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		data.offset[2] = x * (1.0 - data.anchor[2]) - p_margin + parent_rect.position.x;
		break;
	}

	// Bottom
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_TOP_RIGHT:
	case PRESET_CENTER_TOP:
	case PRESET_TOP_WIDE:
		data.offset[3] = parent_rect.size.y * (0.0 - data.anchor[3]) + new_size.y + p_margin +
						 parent_rect.position.y;
		break;

	case PRESET_CENTER_LEFT:
	case PRESET_CENTER_RIGHT:
	case PRESET_CENTER:
	case PRESET_HCENTER_WIDE:
		data.offset[3] =
			parent_rect.size.y * (0.5 - data.anchor[3]) + new_size.y / 2 + parent_rect.position.y;
		break;

	case PRESET_BOTTOM_LEFT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_BOTTOM:
	case PRESET_LEFT_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_FULL_RECT:
		data.offset[3] =
			parent_rect.size.y * (1.0 - data.anchor[3]) - p_margin + parent_rect.position.y;
		break;
	}

	_size_changed();
}

void Control::set_anchors_and_offsets_preset(
	LayoutPreset p_preset, LayoutPresetMode p_resize_mode, int p_margin)
{
	ERR_MAIN_THREAD_GUARD;
	set_anchors_preset(p_preset);
	set_offsets_preset(p_preset, p_resize_mode, p_margin);
}

void Control::set_grow_direction_preset(LayoutPreset p_preset)
{
	ERR_MAIN_THREAD_GUARD;
	// Select correct horizontal grow direction.
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_BOTTOM_LEFT:
	case PRESET_CENTER_LEFT:
	case PRESET_LEFT_WIDE:
		set_h_grow_direction(GrowDirection::GROW_DIRECTION_END);
		break;
	case PRESET_TOP_RIGHT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_RIGHT:
	case PRESET_RIGHT_WIDE:
		set_h_grow_direction(GrowDirection::GROW_DIRECTION_BEGIN);
		break;
	case PRESET_CENTER_TOP:
	case PRESET_CENTER_BOTTOM:
	case PRESET_CENTER:
	case PRESET_TOP_WIDE:
	case PRESET_BOTTOM_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_h_grow_direction(GrowDirection::GROW_DIRECTION_BOTH);
		break;
	}

	// Select correct vertical grow direction.
	switch (p_preset) {
	case PRESET_TOP_LEFT:
	case PRESET_TOP_RIGHT:
	case PRESET_CENTER_TOP:
	case PRESET_TOP_WIDE:
		set_v_grow_direction(GrowDirection::GROW_DIRECTION_END);
		break;

	case PRESET_BOTTOM_LEFT:
	case PRESET_BOTTOM_RIGHT:
	case PRESET_CENTER_BOTTOM:
	case PRESET_BOTTOM_WIDE:
		set_v_grow_direction(GrowDirection::GROW_DIRECTION_BEGIN);
		break;

	case PRESET_CENTER_LEFT:
	case PRESET_CENTER_RIGHT:
	case PRESET_CENTER:
	case PRESET_LEFT_WIDE:
	case PRESET_RIGHT_WIDE:
	case PRESET_VCENTER_WIDE:
	case PRESET_HCENTER_WIDE:
	case PRESET_FULL_RECT:
		set_v_grow_direction(GrowDirection::GROW_DIRECTION_BOTH);
		break;
	}
}

void Control::_set_position(const Point2& p_point) { set_position(p_point); }

void Control::set_position(const Point2& p_point, bool p_keep_offsets)
{
	ERR_MAIN_THREAD_GUARD;

#ifdef TOOLS_ENABLED
	// Can't compute anchors, set position directly and return immediately.
	if (saving && !is_inside_tree()) {
		data.pos_cache = p_point;
		return;
	}
#endif // TOOLS_ENABLED

	_compute_layout_rect(Rect2(p_point, data.size_cache), p_keep_offsets);
	_size_changed();
}

Size2 Control::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return data.pos_cache;
}

void Control::_set_global_position(const Point2& p_point) { set_global_position(p_point); }

void Control::set_global_position(const Point2& p_point, bool p_keep_offsets)
{
	ERR_MAIN_THREAD_GUARD;
	// (parent_global_transform * T(new_position) * internal_transform).origin ==
	// new_global_position (T(new_position) * internal_transform).origin ==
	// new_position_in_parent_space new_position == new_position_in_parent_space -
	// internal_transform.origin
	Point2 position_in_parent_space =
		data.parent_canvas_item
			? data.parent_canvas_item->get_global_transform().affine_inverse().xform(p_point)
			: p_point;
	set_position(position_in_parent_space - _get_internal_transform().get_origin(), p_keep_offsets);
}

Point2 Control::get_global_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	return get_global_transform().get_origin();
}

Point2 Control::get_screen_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	ERR_FAIL_COND_V(!is_inside_tree(), Point2());
	return get_screen_transform().get_origin();
}

void Control::_set_size(const Size2& p_size)
{
#ifdef DEBUG_ENABLED
	if (data.size_warning && (data.anchor[SIDE_LEFT] != data.anchor[SIDE_RIGHT] ||
								 data.anchor[SIDE_TOP] != data.anchor[SIDE_BOTTOM])) {
		WARN_PRINT(
			"Nodes with non-equal opposite anchors will have their size overridden after _ready(). "
			"\nIf you want to set size, change the anchors or consider using set_deferred().");
	}
#endif // DEBUG_ENABLED
	set_size(p_size);
}

void Control::set_size(const Size2& p_size, bool p_keep_offsets)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!std::isfinite(p_size.x) || !std::isfinite(p_size.y));
	Size2 new_size = p_size;
	Size2 min = get_combined_minimum_size();
	if (new_size.x < min.x) {
		new_size.x = min.x;
	}
	if (new_size.y < min.y) {
		new_size.y = min.y;
	}

	Size2 max = get_combined_maximum_size();
	if (max.x >= 0 && new_size.x > max.x) {
		new_size.x = max.x;
	}
	if (max.y >= 0 && new_size.y > max.y) {
		new_size.y = max.y;
	}

#ifdef TOOLS_ENABLED
	// Can't compute anchors, set size directly and return immediately.
	if (saving && !is_inside_tree()) {
		data.size_cache = new_size;
		return;
	}
#endif // TOOLS_ENABLED

	data.expanded_by_desired_size = false;

	_compute_layout_rect(Rect2(data.pos_cache, new_size), p_keep_offsets);
	_size_changed();
}

Size2 Control::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return data.size_cache;
}

void Control::reset_size()
{
	ERR_MAIN_THREAD_GUARD;
	set_size(Size2());
}

void Control::set_rect(const Rect2& p_rect)
{
	ERR_MAIN_THREAD_GUARD;
	for (int i = 0; i < 4; i++) {
		data.anchor[i] = ANCHOR_BEGIN;
	}

	_compute_layout_rect(p_rect);
	if (is_inside_tree()) {
		_size_changed();
	}
}

Rect2 Control::get_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	Transform2D xform = get_transform();
	return Rect2(xform.get_origin(), xform.get_scale() * get_size());
}

Rect2 Control::get_global_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	Transform2D xform = get_global_transform();
	return Rect2(xform.get_origin(), xform.get_scale() * get_size());
}

Rect2 Control::get_screen_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());

	Transform2D xform = get_screen_transform();
	return Rect2(xform.get_origin(), xform.get_scale() * get_size());
}

Rect2 Control::get_anchorable_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	return Rect2(Point2(), get_size());
}

Vector2 Control::get_scale() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return data.scale;
}

void Control::set_rotation_degrees(real_t p_degrees)
{
	ERR_MAIN_THREAD_GUARD;
	set_rotation(Math::deg_to_rad(p_degrees));
}

real_t Control::get_rotation() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return data.rotation;
}

real_t Control::get_rotation_degrees() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return Math::rad_to_deg(get_rotation());
}

Vector2 Control::get_pivot_offset_ratio() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return data.pivot_offset_ratio;
}

Vector2 Control::get_pivot_offset() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return data.pivot_offset;
}

Vector2 Control::get_combined_pivot_offset() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return data.pivot_offset + data.pivot_offset_ratio * get_size();
}

void Control::set_propagate_maximum_size(bool p_propagate)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.propagate_maximum_size == p_propagate) {
		return;
	}
	data.propagate_maximum_size = p_propagate;
	update_maximum_size();
}

bool Control::is_propagating_maximum_size()
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.propagate_maximum_size;
}

void Control::set_block_maximum_size_adjust(bool p_block)
{
	ERR_MAIN_THREAD_GUARD;
	data.block_maximum_size_adjust = p_block;
}

void Control::set_custom_maximum_size(const Size2& p_custom)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_custom == data.custom_maximum_size) {
		return;
	}

	if (!p_custom.is_finite()) {
		// Prevent infinite loop.
		return;
	}

	Size2 normalized = p_custom;
	if (normalized.x < 0) {
		normalized.x = -1;
	}
	if (normalized.y < 0) {
		normalized.y = -1;
	}

	data.custom_maximum_size = normalized;
	update_maximum_size();
	update_configuration_warnings();
}

Size2 Control::get_custom_maximum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return data.custom_maximum_size;
}

Size2 Control::get_maximum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	Vector2 ms = Vector2(-1, -1);
	return ms;
}

void Control::_update_maximum_size_cache() const
{
	Size2 maxsize = get_maximum_size();
	if (data.custom_maximum_size.x >= 0) {
		if (maxsize.x >= 0) {
			maxsize.x = MIN(maxsize.x, data.custom_maximum_size.x);
		}
		else {
			maxsize.x = data.custom_maximum_size.x;
		}
	}
	if (data.custom_maximum_size.y >= 0) {
		if (maxsize.y >= 0) {
			maxsize.y = MIN(maxsize.y, data.custom_maximum_size.y);
		}
		else {
			maxsize.y = data.custom_maximum_size.y;
		}
	}

	if (data.parent_maximum_size_cache.x >= 0) {
		if (maxsize.x >= 0) {
			maxsize.x = MIN(maxsize.x, data.parent_maximum_size_cache.x);
		}
		else {
			maxsize.x = data.parent_maximum_size_cache.x;
		}
	}
	if (data.parent_maximum_size_cache.y >= 0) {
		if (maxsize.y >= 0) {
			maxsize.y = MIN(maxsize.y, data.parent_maximum_size_cache.y);
		}
		else {
			maxsize.y = data.parent_maximum_size_cache.y;
		}
	}

	data.maximum_size_cache = maxsize;
	data.maximum_size_valid = true;
}

Size2 Control::get_combined_maximum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!data.maximum_size_valid) {
		_update_maximum_size_cache();
	}
	return data.maximum_size_cache;
}

Size2 Control::get_inner_combined_maximum_size() const { return get_combined_maximum_size(); }

void Control::set_parent_maximum_size_cache(const Size2& p_parent_max)
{
	const Size2 normalized = p_parent_max.maxf(-1.0f);
	if (data.parent_maximum_size_cache == normalized) {
		return;
	}
	data.parent_maximum_size_cache = normalized;
	update_maximum_size();
}

void Control::set_block_minimum_size_adjust(bool p_block)
{
	ERR_MAIN_THREAD_GUARD;
	data.block_minimum_size_adjust = p_block;
}

Size2 Control::get_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	Vector2 ms;
	return ms;
}

Size2 Control::get_custom_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return data.custom_minimum_size;
}

void Control::_update_desired_size_cache() const
{
	Size2 desired_size = get_desired_size();

	data.desired_size_cache = desired_size;
	data.desired_size_valid = true;
}

Size2 Control::get_bound_desired_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!data.desired_size_valid) {
		_update_desired_size_cache();
	}
	Size2 desired_size = data.desired_size_cache;
	desired_size = desired_size.max(get_combined_minimum_size());
	Size2 max_size = get_combined_maximum_size();
	if (max_size.x >= 0) {
		desired_size.x = MIN(desired_size.x, max_size.x);
	}
	if (max_size.y >= 0) {
		desired_size.y = MIN(desired_size.y, max_size.y);
	}
	return desired_size;
}

Size2 Control::get_desired_size() const { return Size2(); }

void Control::grow_to_desired_size()
{
	ERR_MAIN_THREAD_GUARD;
	if (!is_inside_tree()) {
		return;
	}

	Size2 desired_size = get_bound_desired_size();
	if (desired_size <= get_combined_minimum_size()) {
		return;
	}

	if (data.expanded_by_desired_size) {
		set_size(desired_size);
		data.expanded_by_desired_size = true;
	}
	else if (desired_size.x > get_size().x || desired_size.y > get_size().y) {
		set_size(desired_size);
		data.expanded_by_desired_size = true;
	}
}

bool Control::is_expanded_by_desired_size() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.expanded_by_desired_size;
}

bool Control::is_layout_pending() const
{
	ERR_MAIN_THREAD_GUARD_V(false);
	return data.layout_pending;
}

bool Control::is_layout_pending_in_tree() const
{
	ERR_MAIN_THREAD_GUARD_V(false);
	const Control* current_node = this;
	while (current_node != nullptr) {
		if (current_node->is_layout_pending()) {
			return true;
		}
		current_node = current_node->get_parent_control();
	}
	return false;
}

void Control::layout_pending_start()
{
	ERR_MAIN_THREAD_GUARD;
	data.layout_pending = true;
}

Control* Control::get_layout_pending_control_in_tree() const
{
	Control* current_node = const_cast<Control*>(this);
	while (current_node != nullptr) {
		if (current_node->is_layout_pending()) {
			return current_node;
		}
		current_node = current_node->get_parent_control();
	}
	return nullptr;
}

void Control::_update_minimum_size_cache() const
{
	Size2 minsize = get_minimum_size();
	minsize = minsize.max(data.custom_minimum_size);

	data.minimum_size_cache = minsize;
	data.minimum_size_valid = true;

	// Keep the desired size cache in sync so it will update if needed when the minimum size
	// changes. This is needed for get_bound_desired_size to work correctly.
	data.desired_size_valid = false;
}

Size2 Control::get_combined_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!data.minimum_size_valid) {
		_update_minimum_size_cache();
	}
	return data.minimum_size_cache;
}

Size2 Control::get_bound_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	Size2 min_size = get_combined_minimum_size();
	Size2 max_size = get_combined_maximum_size();

	if (max_size.x >= 0 && min_size.x > max_size.x) {
		min_size.x = max_size.x;
	}
	if (max_size.y >= 0 && min_size.y > max_size.y) {
		min_size.y = max_size.y;
	}

	return min_size;
}

void Control::_clear_size_warning() { data.size_warning = false; }

uint32_t Control::get_h_size_flags() const
{
	ERR_READ_THREAD_GUARD_V(SIZE_EXPAND_FILL);
	return data.h_size_flags;
}

uint32_t Control::get_v_size_flags() const
{
	ERR_READ_THREAD_GUARD_V(SIZE_EXPAND_FILL);
	return data.v_size_flags;
}

real_t Control::get_stretch_ratio() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return data.expand;
}

bool Control::is_offset_transform_enabled() const
{
	return data.offset_transform != nullptr && data.offset_transform->enabled;
}

Vector2 Control::get_offset_transform_position() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_TRANSLATION_ABSOLUTE;
	}

	return data.offset_transform->translation_absolute;
}

Vector2 Control::get_offset_transform_position_ratio() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_TRANSLATION_RELATIVE;
	}

	return data.offset_transform->translation_relative;
}

Vector2 Control::get_offset_transform_scale() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_SCALE;
	}

	return data.offset_transform->scale;
}

real_t Control::get_offset_transform_rotation() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_ROTATION;
	}

	return data.offset_transform->rotation;
}

Vector2 Control::get_offset_transform_pivot() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_PIVOT_ABSOLUTE;
	}

	return data.offset_transform->pivot_absolute;
}

Vector2 Control::get_offset_transform_pivot_ratio() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_PIVOT_RELATIVE;
	}

	return data.offset_transform->pivot_relative;
}

bool Control::is_offset_transform_visual_only() const
{
	if (data.offset_transform == nullptr) {
		return Data::OffsetTransform::DEFAULT_VISUAL_ONLY;
	}

	return data.offset_transform->visual_only;
}

Transform2D Control::get_offset_transform() const
{
	if (!is_offset_transform_enabled()) {
		return Transform2D();
	}

	Vector2 combined_translation = data.offset_transform->translation_absolute +
								   data.offset_transform->translation_relative * get_size();
	Vector2 combined_pivot =
		data.offset_transform->pivot_absolute + data.offset_transform->pivot_relative * get_size();

	Transform2D offset_xform(data.offset_transform->rotation, data.offset_transform->scale, 0.0f,
		combined_pivot + combined_translation);
	offset_xform.translate_local(-combined_pivot);
	return offset_xform;
}

void Control::accept_event()
{
	ERR_MAIN_THREAD_GUARD;
	if (is_inside_tree()) {
		get_viewport()->_gui_accept_event();
	}
}

bool Control::has_point(const Point2& p_point) const
{
	ERR_READ_THREAD_GUARD_V(false);
	return Rect2(Point2(), get_size()).has_point(p_point);
}

Control::MouseFilter Control::get_mouse_filter() const
{
	ERR_READ_THREAD_GUARD_V(MOUSE_FILTER_IGNORE);
	return data.mouse_filter;
}

Control::MouseFilter Control::get_mouse_filter_with_override() const
{
	ERR_READ_THREAD_GUARD_V(MOUSE_FILTER_IGNORE);
	if (!_is_mouse_filter_enabled()) {
		return MOUSE_FILTER_IGNORE;
	}
	return data.mouse_filter;
}

void Control::set_mouse_behavior_recursive(MouseBehaviorRecursive p_mouse_behavior_recursive)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_mouse_behavior_recursive, 3);
	if (data.mouse_behavior_recursive == p_mouse_behavior_recursive) {
		return;
	}
	data.mouse_behavior_recursive = p_mouse_behavior_recursive;
	_update_mouse_behavior_recursive();
}

Control::MouseBehaviorRecursive Control::get_mouse_behavior_recursive() const
{
	ERR_READ_THREAD_GUARD_V(MOUSE_BEHAVIOR_INHERITED);
	return data.mouse_behavior_recursive;
}

bool Control::_is_mouse_filter_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (data.mouse_behavior_recursive == MOUSE_BEHAVIOR_INHERITED) {
		if (data.parent_control) {
			return data.parent_mouse_behavior_recursive_enabled;
		}
		return true;
	}
	return data.mouse_behavior_recursive == MOUSE_BEHAVIOR_ENABLED;
}

void Control::_update_mouse_behavior_recursive()
{
	if (data.mouse_behavior_recursive == MOUSE_BEHAVIOR_INHERITED) {
		if (data.parent_control) {
			_propagate_mouse_behavior_recursive_recursively(
				data.parent_control->_is_mouse_filter_enabled(), false);
		}
		else {
			_propagate_mouse_behavior_recursive_recursively(true, false);
		}
	}
	else {
		_propagate_mouse_behavior_recursive_recursively(
			data.mouse_behavior_recursive == MOUSE_BEHAVIOR_ENABLED, false);
	}
	if (get_viewport()) {
		get_viewport()->_gui_update_mouse_over();
	}
}

void Control::set_force_pass_scroll_events(bool p_force_pass_scroll_events)
{
	ERR_MAIN_THREAD_GUARD;
	data.force_pass_scroll_events = p_force_pass_scroll_events;
}

bool Control::is_force_pass_scroll_events() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.force_pass_scroll_events;
}

void Control::warp_mouse(const Point2& p_position)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	get_viewport()->warp_mouse(get_global_transform_with_canvas().xform(p_position));
}

void Control::accessibility_drop()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND(!get_viewport()->gui_is_dragging());

	get_viewport()->gui_perform_drop_at(Vector2(Math::INF, Math::INF), this);

	queue_accessibility_update();
}

String Control::get_accessibility_container_name(const Node* p_node) const
{
	String ret;
	if (data.parent_control) {
		ret = data.parent_control->get_accessibility_container_name(this);
	}
	return ret;
}

void Control::set_accessibility_name(const String& p_name)
{
	ERR_THREAD_GUARD
	if (data.accessibility_name != p_name) {
		data.accessibility_name = p_name;
		queue_accessibility_update();
		update_configuration_warnings();
	}
}

void Control::set_accessibility_description(const String& p_description)
{
	ERR_THREAD_GUARD
	if (data.accessibility_description != p_description) {
		data.accessibility_description = p_description;
		queue_accessibility_update();
	}
}

void Control::set_accessibility_live(AccessibilityServerEnums::AccessibilityLiveMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.accessibility_live != p_mode) {
		data.accessibility_live = p_mode;
		queue_accessibility_update();
	}
}

AccessibilityServerEnums::AccessibilityLiveMode Control::get_accessibility_live() const
{
	return data.accessibility_live;
}

void Control::set_drag_preview(Control* p_control)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND(!get_viewport()->gui_is_dragging());
	get_viewport()->_gui_set_drag_preview(this, p_control);
}

bool Control::is_drag_successful() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_inside_tree() && get_viewport()->gui_is_drag_successful();
}

void Control::set_focus_mode(FocusMode p_focus_mode)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_focus_mode, 4);

	if (is_inside_tree() && p_focus_mode == FOCUS_NONE && data.focus_mode != FOCUS_NONE &&
		has_focus()) {
		release_focus();
	}

	if (data.focus_mode == p_focus_mode) {
		return;
	}
	data.focus_mode = p_focus_mode;
	queue_accessibility_update();
}

Control::FocusMode Control::get_focus_mode() const
{
	ERR_READ_THREAD_GUARD_V(FOCUS_NONE);
	return data.focus_mode;
}

Control::FocusMode Control::get_focus_mode_with_override() const
{
	ERR_READ_THREAD_GUARD_V(FOCUS_NONE);
	if (!_is_focus_mode_enabled()) {
		return FOCUS_NONE;
	}
	return data.focus_mode;
}

void Control::set_focus_behavior_recursive(FocusBehaviorRecursive p_focus_behavior_recursive)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_focus_behavior_recursive, 3);
	if (data.focus_behavior_recursive == p_focus_behavior_recursive) {
		return;
	}
	data.focus_behavior_recursive = p_focus_behavior_recursive;
	_update_focus_behavior_recursive();
	queue_accessibility_update();
}

Control::FocusBehaviorRecursive Control::get_focus_behavior_recursive() const
{
	ERR_READ_THREAD_GUARD_V(FOCUS_BEHAVIOR_INHERITED);
	return data.focus_behavior_recursive;
}

bool Control::_is_focusable() const
{
	bool ac_enabled = is_inside_tree() && get_tree()->is_accessibility_enabled();
	return (is_visible_in_tree() &&
			((get_focus_mode_with_override() == FOCUS_ALL) ||
				(get_focus_mode_with_override() == FOCUS_CLICK) ||
				(ac_enabled && get_focus_mode_with_override() == FOCUS_ACCESSIBILITY)));
}

bool Control::_is_focus_mode_enabled() const
{
	if (data.focus_behavior_recursive == FOCUS_BEHAVIOR_INHERITED) {
		if (data.parent_control) {
			return data.parent_focus_behavior_recursive_enabled;
		}
		return true;
	}
	return data.focus_behavior_recursive == FOCUS_BEHAVIOR_ENABLED;
}

void Control::_update_focus_behavior_recursive()
{
	if (data.focus_behavior_recursive == FOCUS_BEHAVIOR_INHERITED) {
		Control* parent = get_parent_control();
		if (parent) {
			_propagate_focus_behavior_recursive_recursively(
				parent->_is_focus_mode_enabled(), false);
		}
		else {
			_propagate_focus_behavior_recursive_recursively(true, false);
		}
	}
	else {
		_propagate_focus_behavior_recursive_recursively(
			data.focus_behavior_recursive == FOCUS_BEHAVIOR_ENABLED, false);
	}
}

bool Control::has_focus(bool p_ignore_hidden_focus) const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_inside_tree() && get_viewport()->_gui_control_has_focus(this, p_ignore_hidden_focus);
}

void Control::grab_focus(bool p_hide_focus)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	if (get_focus_mode_with_override() == FOCUS_ACCESSIBILITY) {
		if (!get_tree()->is_accessibility_enabled()) {
			WARN_PRINT(
				"This control can grab focus only when screen reader is active. Use "
				"set_focus_mode() and set_focus_behavior_recursive() to allow a control to get "
				"focus. Use get_tree().is_accessibility_enabled() to check screen-reader state.");
			return;
		}
	}

	if (get_focus_mode_with_override() == FOCUS_NONE) {
		WARN_PRINT("This control can't grab focus. Use set_focus_mode() and "
				   "set_focus_behavior_recursive() to allow a control to get focus.");
		return;
	}

	get_viewport()->_gui_control_grab_focus(this, p_hide_focus);
}

void Control::grab_click_focus()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	get_viewport()->_gui_grab_click_focus(this);
}

void Control::release_focus()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	if (!has_focus()) {
		return;
	}

	get_viewport()->gui_release_focus();
}

void Control::set_focus_neighbor(Side p_side, const NodePath& p_neighbor)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX((int)p_side, 4);
	data.focus_neighbor[p_side] = p_neighbor;
}

NodePath Control::get_focus_neighbor(Side p_side) const
{
	ERR_READ_THREAD_GUARD_V(NodePath());
	ERR_FAIL_INDEX_V((int)p_side, 4, NodePath());
	return data.focus_neighbor[p_side];
}

void Control::set_focus_next(const NodePath& p_next)
{
	ERR_MAIN_THREAD_GUARD;
	data.focus_next = p_next;
}

NodePath Control::get_focus_next() const
{
	ERR_READ_THREAD_GUARD_V(NodePath());
	return data.focus_next;
}

void Control::set_focus_previous(const NodePath& p_prev)
{
	ERR_MAIN_THREAD_GUARD;
	data.focus_prev = p_prev;
}

NodePath Control::get_focus_previous() const
{
	ERR_READ_THREAD_GUARD_V(NodePath());
	return data.focus_prev;
}

#define MAX_NEIGHBOR_SEARCH_COUNT 512

Control* Control::find_valid_focus_neighbor(Side p_side) const
{
	return const_cast<Control*>(this)->_get_focus_neighbor(p_side);
}

void Control::set_default_cursor_shape(CursorShape p_shape)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(int(p_shape), DisplayServerEnums::CURSOR_MAX);

	if (data.default_cursor == p_shape) {
		return;
	}
	data.default_cursor = p_shape;

	if (!is_inside_tree()) {
		return;
	}
	if (!get_global_rect().has_point(get_global_mouse_position())) {
		return;
	}

	// Display the new cursor shape instantly.
	get_viewport()->update_mouse_cursor_state();
}

Control::CursorShape Control::get_default_cursor_shape() const
{
	ERR_READ_THREAD_GUARD_V(CURSOR_ARROW);
	return data.default_cursor;
}

Control::CursorShape Control::get_cursor_shape(const Point2& p_pos) const
{
	ERR_READ_THREAD_GUARD_V(CURSOR_ARROW);
	return data.default_cursor;
}

bool Control::is_visibility_clip_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.disable_visibility_clip;
}

bool Control::is_clipping_contents()
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.clip_contents;
}

void Control::_theme_changed()
{
	if (is_inside_tree()) {
		data.theme_owner->propagate_theme_changed(this, this, true, false);
	}
}

void Control::_invalidate_theme_cache()
{
	data.theme_icon_cache.clear();
	data.theme_style_cache.clear();
	data.theme_font_cache.clear();
	data.theme_font_size_cache.clear();
	data.theme_color_cache.clear();
	data.theme_constant_cache.clear();
}

void Control::_update_theme_item_cache()
{
	ThemeDB::get_singleton()->update_class_instance_items(this);
}

void Control::set_theme_owner_node(Node* p_node)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_owner->set_owner_node(p_node);
}

Node* Control::get_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return data.theme_owner->get_owner_node();
}

bool Control::has_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.theme_owner->has_owner_node();
}

void Control::set_theme_context(ThemeContext* p_context, bool p_propagate)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_owner->set_owner_context(p_context, p_propagate);
}

Ref<Theme> Control::get_theme() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Theme>());
	return data.theme;
}

StringName Control::get_theme_type_variation() const
{
	ERR_READ_THREAD_GUARD_V(StringName());
	return data.theme_type_variation;
}

#ifdef TOOLS_ENABLED
Ref<Texture2D> Control::get_editor_theme_icon(const StringName& p_name) const
{
	return get_theme_icon(p_name, SNAME("EditorIcons"));
}
#endif // TOOLS_ENABLED

void Control::add_theme_font_size_override(const StringName& p_name, int p_font_size)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_font_size_override[p_name] = p_font_size;
	_notify_theme_override_changed();
}

void Control::add_theme_color_override(const StringName& p_name, const Color& p_color)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_color_override[p_name] = p_color;
	_notify_theme_override_changed();
}

void Control::add_theme_constant_override(const StringName& p_name, int p_constant)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_constant_override[p_name] = p_constant;
	_notify_theme_override_changed();
}

void Control::remove_theme_font_size_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_font_size_override.erase(p_name);
	_notify_theme_override_changed();
}

void Control::remove_theme_color_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_color_override.erase(p_name);
	_notify_theme_override_changed();
}

void Control::remove_theme_constant_override(const StringName& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_constant_override.erase(p_name);
	_notify_theme_override_changed();
}

bool Control::has_theme_icon_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Texture2D>* tex = data.theme_icon_override.getptr(p_name);
	return tex != nullptr;
}

bool Control::has_theme_stylebox_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<StyleBox>* style = data.theme_style_override.getptr(p_name);
	return style != nullptr;
}

bool Control::has_theme_font_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Font>* font = data.theme_font_override.getptr(p_name);
	return font != nullptr;
}

bool Control::has_theme_font_size_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* font_size = data.theme_font_size_override.getptr(p_name);
	return font_size != nullptr;
}

bool Control::has_theme_color_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Color* color = data.theme_color_override.getptr(p_name);
	return color != nullptr;
}

bool Control::has_theme_constant_override(const StringName& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* constant = data.theme_constant_override.getptr(p_name);
	return constant != nullptr;
}

float Control::get_theme_default_base_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return data.theme_owner->get_theme_default_base_scale();
}

Ref<Font> Control::get_theme_default_font() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Font>());
	return data.theme_owner->get_theme_default_font();
}

int Control::get_theme_default_font_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return data.theme_owner->get_theme_default_font_size();
}

void Control::begin_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	data.bulk_theme_override = true;
}

void Control::end_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!data.bulk_theme_override);

	data.bulk_theme_override = false;
	_notify_theme_override_changed();
}

void Control::set_layout_direction(Control::LayoutDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.layout_dir == p_direction) {
		return;
	}
	ERR_FAIL_INDEX(p_direction, LAYOUT_DIRECTION_MAX);

	data.layout_dir = p_direction;

	propagate_notification(NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
}

Control::LayoutDirection Control::get_layout_direction() const
{
	ERR_READ_THREAD_GUARD_V(LAYOUT_DIRECTION_INHERITED);
	return data.layout_dir;
}

bool Control::is_localizing_numeral_system() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.localize_numeral_system;
}

#ifndef DISABLE_DEPRECATED
void Control::set_auto_translate(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	set_auto_translate_mode(p_enable ? AUTO_TRANSLATE_MODE_ALWAYS : AUTO_TRANSLATE_MODE_DISABLED);
}

bool Control::is_auto_translating() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return can_auto_translate();
}
#endif // DISABLE_DEPRECATED

void Control::set_tooltip_auto_translate_mode(AutoTranslateMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	data.tooltip_auto_translate_mode = p_mode;
}

Node::AutoTranslateMode Control::get_tooltip_auto_translate_mode() const
{
	ERR_READ_THREAD_GUARD_V(AUTO_TRANSLATE_MODE_INHERIT);
	return data.tooltip_auto_translate_mode;
}

Node::AutoTranslateMode Control::get_tooltip_auto_translate_mode_at(const Vector2& p_at) const
{
	ERR_READ_THREAD_GUARD_V(AUTO_TRANSLATE_MODE_INHERIT);
	return get_tooltip_auto_translate_mode();
}

void Control::set_tooltip_text(const String& p_hint)
{
	ERR_MAIN_THREAD_GUARD;
	data.tooltip = p_hint;
	update_configuration_warnings();
}

String Control::get_tooltip_text() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return data.tooltip;
}

void Control::set_translation_context(const StringName& p_context)
{
	ERR_MAIN_THREAD_GUARD;
	data.translation_context = p_context;
}

StringName Control::get_translation_context() const
{
	ERR_READ_THREAD_GUARD_V(StringName());
	return data.translation_context;
}

StringName Control::_get_translation_context_with_override(const StringName& p_context) const
{
	if (p_context.is_empty()) {
		return data.translation_context;
	}
	return p_context;
}

String Control::accessibility_get_contextual_info() const
{
	ERR_READ_THREAD_GUARD_V(String());
	String ret;
	return ret;
}

void Control::_ensure_allocated_offset_transform()
{
	if (data.offset_transform != nullptr) {
		return;
	}

	data.offset_transform = memnew(Data::OffsetTransform);
}


