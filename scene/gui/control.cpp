/**************************************************************************/
/*  control.cpp                                                           */
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

#include "servers/display/display_server.h"
#include "control.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input_map.h"
#include "core/math/transform_2d.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"

#ifdef TOOLS_ENABLED
void Control::_edit_set_position(const Point2& p_position)
{
	ERR_FAIL_COND_MSG(
		!Engine::is_editor_hint(), "This function can only be used from editor plugins.");
	set_position(p_position, false);
}

Point2 Control::_edit_get_position() const { return get_position(); }

void Control::_edit_set_scale(const Size2& p_scale) { set_scale(p_scale); }

Size2 Control::_edit_get_scale() const { return data.scale; }

void Control::_edit_set_rect(const Rect2& p_edit_rect)
{
	ERR_FAIL_COND_MSG(
		!Engine::is_editor_hint(), "This function can only be used from editor plugins.");
	Vector2 rect_new_pos_in_parent_space = get_transform().xform(p_edit_rect.position);
	set_size(p_edit_rect.size, false);
	set_position(rect_new_pos_in_parent_space - _get_internal_transform().get_origin(), false);
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

bool Control::_property_can_revert(const String& p_name) const
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
		Node* edited_scene_root = SceneTree::get_edited_scene_root();
		Node* scene_root_parent = edited_scene_root ? edited_scene_root->get_parent() : nullptr;
		if (scene_root_parent && get_viewport() == scene_root_parent->get_viewport()) {
			parent_rect.size =
				Size2(GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_width"),
					GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_height"));
		}
		else {
			Viewport* vp = get_viewport();
			if (!vp && SceneTree::is_initialized()) {
				vp = SceneTree::get_root();
			}

			if (vp) {
				parent_rect = vp->get_visible_rect();
			}
			else {
				return Rect2();
			}
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

	Renderer::canvas_item_set_transform(get_canvas_item(), xform);
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

void Control::set_anchor(
	Side p_side, real_t p_anchor, bool p_keep_offset, bool p_push_opposite_anchor)
{
}

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
		ERR_FAIL_COND(parent_rect_size.x == 0.0f);
		ERR_FAIL_COND(parent_rect_size.y == 0.0f);
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

	data.size_cache = new_size;
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

void Control::set_custom_minimum_size(const Size2& p_custom) {}

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

void Control::set_h_size_flags(uint32_t p_flags) {}

uint32_t Control::get_h_size_flags() const
{
	ERR_READ_THREAD_GUARD_V(SIZE_EXPAND_FILL);
	return data.h_size_flags;
}

void Control::set_v_size_flags(uint32_t p_flags) {}

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

void Control::set_mouse_filter(MouseFilter p_filter) {}

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

void Control::add_child_notify(Node* p_child) {}

void Control::remove_child_notify(Node* p_child) {}

bool Control::_is_focusable() const
{
	bool ac_enabled = is_inside_tree() && SceneTree::is_accessibility_enabled();
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
		if (!SceneTree::is_accessibility_enabled()) {
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
	ThemeDB::update_class_instance_items(this);
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

void Control::set_theme_type_variation(const String& p_theme_type) {}

String Control::get_theme_type_variation() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return data.theme_type_variation;
}

#ifdef TOOLS_ENABLED
Ref<Texture2D> Control::get_editor_theme_icon(const String& p_name) const
{
	return get_theme_icon(p_name, SNAME("EditorIcons"));
}
#endif // TOOLS_ENABLED

void Control::add_theme_style_override(const String& p_name, StyleBox* rp_style) {}

void Control::add_theme_font_override(const String& p_name, Font* rp_font) {}

void Control::add_theme_font_size_override(const String& p_name, int p_font_size)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_font_size_override[p_name] = p_font_size;
	_notify_theme_override_changed();
}

void Control::add_theme_color_override(const String& p_name, const Color& p_color)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_color_override[p_name] = p_color;
	_notify_theme_override_changed();
}

Color Control::get_theme_color(const String& p_name, const String& p_theme_type) const
{
	return Color();
}

void Control::add_theme_constant_override(const String& p_name, int p_constant)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_constant_override[p_name] = p_constant;
	_notify_theme_override_changed();
}

void Control::remove_theme_font_size_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_font_size_override.erase(p_name);
	_notify_theme_override_changed();
}

int Control::get_theme_font_size(const String& p_name, const String& p_theme_type) const
{
	return 0;
}

Ref<StyleBox> Control::get_theme_stylebox(
	const String& p_name, const String& p_theme_type) const
{
	Ref<StyleBox> sb = memnew(StyleBox);
	return sb;
}

Ref<Font> Control::get_theme_font(const String& p_name, const String& p_theme_type) const
{
	Ref<Font> font = memnew(Font);
	return font;
}

void Control::remove_theme_color_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_color_override.erase(p_name);
	_notify_theme_override_changed();
}

int Control::get_theme_constant(const String& p_name, const String& p_theme_type) const
{
	return 0;
}

void Control::remove_theme_constant_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	data.theme_constant_override.erase(p_name);
	_notify_theme_override_changed();
}

bool Control::has_theme_icon_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Texture2D>* tex = data.theme_icon_override.getptr(p_name);
	return tex != nullptr;
}

bool Control::has_theme_stylebox_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<StyleBox>* style = data.theme_style_override.getptr(p_name);
	return style != nullptr;
}

bool Control::has_theme_font_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Font>* font = data.theme_font_override.getptr(p_name);
	return font != nullptr;
}

bool Control::has_theme_font_size_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* font_size = data.theme_font_size_override.getptr(p_name);
	return font_size != nullptr;
}

bool Control::has_theme_color_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Color* color = data.theme_color_override.getptr(p_name);
	return color != nullptr;
}

bool Control::has_theme_constant_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* constant = data.theme_constant_override.getptr(p_name);
	return constant != nullptr;
}

bool Control::has_theme_stylebox(const String& p_name, const String& p_theme_type) const
{
	return true;
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

bool Control::is_layout_rtl() const { return false; }

void Control::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
}

Control* Control::make_custom_tooltip(const String& p_text) const
{
	Control* c = memnew(Control);
	return c;
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

String Control::get_tooltip_text() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return data.tooltip;
}

void Control::set_translation_context(const String& p_context)
{
	ERR_MAIN_THREAD_GUARD;
	data.translation_context = p_context;
}

String Control::get_translation_context() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return data.translation_context;
}

String Control::_get_translation_context_with_override(const String& p_context) const
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

void Control::_size_changed() {}

void Control::_notify_theme_override_changed() {}

Ref<Texture2D> Control::get_theme_icon(
	const String& p_name, const String& p_theme_type) const
{
	Ref<Texture2D> t = memnew(Texture2D);
	return t;
}

void Control::set_stretch_ratio(real_t p_ratio) {}

bool Control::has_theme_icon(String const&, String const&) const {}

void Control::add_theme_icon_override(String const&, Texture2D*) {}

void Control::set_shortcut_context(Node const*) {}

void Control::_propagate_mouse_behavior_recursive_recursively(bool, bool) {}

void Control::_propagate_focus_behavior_recursive_recursively(bool, bool) {}

String Control::get_accessibility_name() const { return String(); }

void Control::update_maximum_size() {}

void Control::set_rotation(float) {}

void Control::set_clip_contents(bool) {}

void Control::remove_theme_style_override(String const&) {}

void Control::_notification(int) {}

void Control::set_theme(Ref<Theme> const&) {}

void Control::set_scale(Vector2 const&) {}

void Control::set_pivot_offset(Vector2 const&) {}

void Control::set_pivot_offset_ratio(Vector2 const&) {}

void Control::set_offset_transform_visual_only(bool) {}

void Control::set_offset_transform_scale(Vector2 const&) {}

void Control::set_offset_transform_pivot_ratio(Vector2 const&) {}

void Control::set_offset_transform_enabled(bool) {}

bool Control::has_theme_color(String const&, String const&) const { return false; }

PackedStringArray Control::get_accessibility_configuration_warnings() const
{
	return PackedStringArray();
}

Control::LayoutMode Control::_get_layout_mode() const
{
	if (data.stored_layout_mode != LAYOUT_MODE_UNCONTROLLED) {
		return data.stored_layout_mode;
	}
	if (!is_inside_tree()) {
		return LAYOUT_MODE_POSITION;
	}
	if (data.anchor[SIDE_LEFT] != 0.0 || data.anchor[SIDE_TOP] != 0.0 ||
		data.anchor[SIDE_RIGHT] != 0.0 || data.anchor[SIDE_BOTTOM] != 0.0) {
		return LAYOUT_MODE_ANCHORS;
	}
	return LAYOUT_MODE_POSITION;
}

Control* Control::_get_focus_neighbor(Side, int) { return nullptr; }

void Control::update_minimum_size()
{
	data.minimum_size_valid = false;
}

void Control::set_layout_type(LayoutConfig::Type p_type)
{
	if (layout.type == p_type) {
		return;
	}
	layout.type = p_type;
	if (p_type != LayoutConfig::LAYOUT_NONE) {
		set_mouse_filter(MOUSE_FILTER_PASS);
	}
	queue_sort();
	update_minimum_size();
}

void Control::queue_sort()
{
	if (!is_inside_tree() || layout.pending_sort) {
		return;
	}
	layout.pending_sort = true;
}

void Control::fit_child_in_rect(Control* p_child, const Rect2& p_rect)
{
	ERR_FAIL_NULL(p_child);
	ERR_FAIL_COND(p_child->get_parent() != this);
	p_child->set_rect(p_rect);
}

Control* Control::as_sortable_control(
	Node* p_node, LayoutConfig::SortableVisibilityMode p_visibility_mode) const
{
	Control* c = dynamic_cast<Control*>(p_node);
	if (!c || c->is_set_as_top_level()) {
		return nullptr;
	}
	if (p_visibility_mode == LayoutConfig::SORTABLE_VISIBILITY_VISIBLE && !c->is_visible()) {
		return nullptr;
	}
	if (p_visibility_mode == LayoutConfig::SORTABLE_VISIBILITY_VISIBLE_IN_TREE &&
		!c->is_visible_in_tree()) {
		return nullptr;
	}
	return c;
}

Control* Control::add_spacer(bool p_begin)
{
	Control* c = memnew(Control);
	c->set_mouse_filter(MOUSE_FILTER_PASS);
	if (layout.orientation == VERTICAL) {
		c->set_v_size_flags(SIZE_EXPAND_FILL);
	}
	else {
		c->set_h_size_flags(SIZE_EXPAND_FILL);
	}
	add_child(c);
	if (p_begin) {
		move_child(c, 0);
	}
	return c;
}

Control* Control::create_box(Orientation p_orientation, LayoutConfig::AlignmentMode p_alignment)
{
	Control* c = memnew(Control);
	c->layout.type = LayoutConfig::LAYOUT_BOX;
	c->layout.orientation = p_orientation;
	c->layout.alignment = p_alignment;
	c->set_mouse_filter(MOUSE_FILTER_PASS);
	return c;
}

Control* Control::create_grid(int p_columns)
{
	Control* c = memnew(Control);
	c->layout.type = LayoutConfig::LAYOUT_GRID;
	c->layout.columns = MAX(1, p_columns);
	c->set_mouse_filter(MOUSE_FILTER_PASS);
	return c;
}

Control* Control::create_flow(Orientation p_orientation)
{
	Control* c = memnew(Control);
	c->layout.type = LayoutConfig::LAYOUT_FLOW;
	c->layout.orientation = p_orientation;
	c->set_mouse_filter(MOUSE_FILTER_PASS);
	return c;
}

Control* Control::create_margin(int p_left, int p_top, int p_right, int p_bottom)
{
	Control* c = memnew(Control);
	c->layout.type = LayoutConfig::LAYOUT_MARGIN;
	c->layout.margins[SIDE_LEFT] = p_left;
	c->layout.margins[SIDE_TOP] = p_top;
	c->layout.margins[SIDE_RIGHT] = p_right;
	c->layout.margins[SIDE_BOTTOM] = p_bottom;
	c->set_mouse_filter(MOUSE_FILTER_PASS);
	return c;
}

Vector<int> Control::get_allowed_size_flags_horizontal() const
{
	return {SIZE_FILL, SIZE_EXPAND, SIZE_SHRINK_BEGIN, SIZE_SHRINK_CENTER, SIZE_SHRINK_END};
}

Vector<int> Control::get_allowed_size_flags_vertical() const
{
	return {SIZE_FILL, SIZE_EXPAND, SIZE_SHRINK_BEGIN, SIZE_SHRINK_CENTER, SIZE_SHRINK_END};
}

// --- control.compat.inc ---

#ifndef DISABLE_DEPRECATED


bool Control::_has_focus_bind_compat_110250() const { return has_focus(false); }

void Control::_grab_focus_bind_compat_110250() { return grab_focus(false); }

void Control::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from scene/gui/button.cpp
// ============================================================================

#include "button.h"
#include "core/os/os.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "scene/gui/button.h"
#include "scene/main/window.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "scene/gui/button.h"

ButtonBehavior::DrawMode ButtonBehavior::get_draw_mode() const
{
	if (disabled) {
		return DRAW_DISABLED;
	}

	if (in_shortcut_feedback) {
		return DRAW_HOVER_PRESSED;
	}

	if (!press_attempt && hovering) {
		if (pressed) {
			return DRAW_HOVER_PRESSED;
		}
		return DRAW_HOVER;
	}
	else {
		bool pressing;
		if (press_attempt) {
			pressing = (pressing_inside || keep_pressed_outside);
			if (pressed) {
				pressing = !pressing;
			}
		}
		else {
			pressing = pressed;
		}

		return pressing ? DRAW_PRESSED : DRAW_NORMAL;
	}
}

void Button::set_style(Style p_style)
{
	if (style == p_style) {
		return;
	}
	style = p_style;
	if (style == STYLE_CHECK_BOX || style == STYLE_CHECK_BUTTON) {
		set_toggle_mode(true);
	}
	else if (style == STYLE_LINK) {
		if (link_text_buf.is_null()) {
			link_text_buf.instantiate();
		}
		set_focus_mode(FOCUS_ACCESSIBILITY);
		set_default_cursor_shape(CURSOR_POINTING_HAND);
	}
	update_minimum_size();
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_pressed(bool p_pressed)
{
	if (!behavior.toggle_mode) {
		return;
	}
	if (behavior.pressed == p_pressed) {
		return;
	}
	behavior.pressed = p_pressed;
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_pressed_no_signal(bool p_pressed)
{
	if (behavior.pressed == p_pressed) {
		return;
	}
	behavior.pressed = p_pressed;
	queue_redraw();
}

void Button::set_toggle_mode(bool p_on)
{
	if (!p_on) {
		set_pressed(false);
	}
	behavior.toggle_mode = p_on;
	queue_accessibility_update();
	update_configuration_warnings();
}

void Button::set_disabled(bool p_disabled)
{
	if (behavior.disabled == p_disabled) {
		return;
	}
	behavior.disabled = p_disabled;
	if (p_disabled) {
		if (!behavior.toggle_mode) {
			behavior.pressed = false;
		}
		behavior.press_attempt = false;
		behavior.pressing_inside = false;
	}
	queue_redraw();
	queue_accessibility_update();
}

void Button::set_shortcut_in_tooltip(bool p_on)
{
	if (behavior.shortcut_in_tooltip != p_on) {
		behavior.shortcut_in_tooltip = p_on;
		queue_accessibility_update();
	}
}

void Button::set_shortcut(const Ref<Shortcut>& p_shortcut)
{
	if (behavior.shortcut != p_shortcut) {
		behavior.shortcut = p_shortcut;
		set_process_shortcut_input(behavior.shortcut.is_valid());
		queue_accessibility_update();
	}
}

void Button::set_button_group(const Ref<ButtonGroup>& p_group)
{
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.erase(this);
	}
	behavior.button_group = p_group;
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.insert(this);
	}
	queue_redraw();
	update_configuration_warnings();
}

void Button::pressed()
{
	if (style == STYLE_LINK && !uri.is_empty()) {
		OS::shell_open(uri);
	}
}

void Button::toggled(bool p_pressed) {}

void Button::set_uri(const String& p_uri)
{
	if (uri != p_uri) {
		uri = p_uri;
		queue_accessibility_update();
	}
}

void Button::set_swatch_color(const Color& p_color)
{
	swatch_color = p_color;
	queue_redraw();
	queue_accessibility_update();
}

bool Button::has_point(const Point2& p_point) const
{
	if (style == STYLE_TEXTURE && textures.click_mask.is_valid()) {
		Point2 point = p_point;
		Rect2 rect;
		Size2 mask_size = textures.click_mask->get_size();

		if (!textures.position_rect.has_area()) {
			rect.size = mask_size;
		}
		else if (textures.tile) {
			rect.size = mask_size;
			if (textures.position_rect.has_point(point)) {
				int cols = (int)Math::ceil(textures.position_rect.size.x / mask_size.x);
				int rows = (int)Math::ceil(textures.position_rect.size.y / mask_size.y);
				int col = (int)(point.x / mask_size.x) % cols;
				int row = (int)(point.y / mask_size.y) % rows;
				point.x -= mask_size.x * col;
				point.y -= mask_size.y * row;
			}
		}
		else {
			Point2 ofs = textures.position_rect.position;
			Size2 scale = mask_size / textures.position_rect.size;

			if (textures.stretch_mode == STRETCH_KEEP_ASPECT_COVERED) {
				float min = MIN(scale.x, scale.y);
				scale.x = min;
				scale.y = min;
				ofs -= textures.texture_region.position / min;
			}

			point -= ofs;
			point *= scale;

			rect.position = textures.texture_region.position.maxf(0);
			rect.size = mask_size.min(textures.texture_region.size);
		}

		if (!rect.has_point(point)) {
			return false;
		}

		Point2i p = point;
		return textures.click_mask->get_bitv(p);
	}

	ERR_READ_THREAD_GUARD_V(false);
	Rect2 rect = Rect2(Point2(), get_size()).grow(behavior.click_margin);
	return rect.has_area() && rect.has_point(p_point);
}

Control::CursorShape Button::get_cursor_shape(const Point2& p_pos) const
{
	if (style == STYLE_LINK) {
		return is_disabled() ? CURSOR_ARROW : get_default_cursor_shape();
	}
	return Control::get_cursor_shape(p_pos);
}

PackedStringArray Button::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (behavior.button_group.is_valid() && !behavior.toggle_mode) {
		warnings.push_back(RTR("ButtonGroup is intended to be used only with buttons that have "
							   "toggle_mode set to true."));
	}

	return warnings;
}

Size2 Button::_get_check_icon_size() const
{
	Size2 tex_size;
	if (style == STYLE_CHECK_BOX) {
		const Ref<Texture2D> candidates[] = {theme_cache.checked, theme_cache.unchecked,
			theme_cache.radio_checked, theme_cache.radio_unchecked, theme_cache.checked_disabled,
			theme_cache.unchecked_disabled, theme_cache.radio_checked_disabled,
			theme_cache.radio_unchecked_disabled};
		for (const Ref<Texture2D>& tex : candidates) {
			if (tex.is_valid()) {
				tex_size = tex_size.max(tex->get_size());
			}
		}
	}
	else if (style == STYLE_CHECK_BUTTON) {
		Ref<Texture2D> on_tex = is_layout_rtl()
									? (is_disabled() ? theme_cache.checked_disabled_mirrored
													 : theme_cache.checked_mirrored)
									: (is_disabled() ? theme_cache.checked_disabled
													 : theme_cache.checked);
		Ref<Texture2D> off_tex = is_layout_rtl()
									 ? (is_disabled() ? theme_cache.unchecked_disabled_mirrored
													  : theme_cache.unchecked_mirrored)
									 : (is_disabled() ? theme_cache.unchecked_disabled
													  : theme_cache.unchecked);
		if (on_tex.is_valid()) {
			tex_size = on_tex->get_size();
		}
		if (off_tex.is_valid()) {
			tex_size = tex_size.max(off_tex->get_size());
		}
	}
	return _fit_icon_size(tex_size);
}

Size2 Button::get_minimum_size() const
{
	if (style == STYLE_LINK) {
		Size2 minsize = link_text_buf.is_valid() ? link_text_buf->get_size() : Size2();
		if (overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
			minsize.width = 0;
		}
		return minsize;
	}

	if (style == STYLE_TEXTURE) {
		Size2 rscale = Control::get_minimum_size();
		if (!textures.ignore_texture_size) {
			if (textures.normal.is_valid()) {
				rscale = textures.normal->get_size();
			}
			else if (textures.pressed.is_valid()) {
				rscale = textures.pressed->get_size();
			}
			else if (textures.hover.is_valid()) {
				rscale = textures.hover->get_size();
			}
			else if (textures.click_mask.is_valid()) {
				rscale = textures.click_mask->get_size();
			}
			else {
				rscale = Size2();
			}
		}
		return rscale.abs();
	}

	if (style == STYLE_COLOR_SWATCH) {
		return Control::get_minimum_size();
	}

	Ref<Texture2D> _icon = icon;
	if (_icon.is_null() && has_theme_icon(SNAME("icon"))) {
		_icon = theme_cache.icon;
	}

	Size2 minsize = get_minimum_size_for_text_and_icon("", _icon);

	if (style == STYLE_CHECK_BOX || style == STYLE_CHECK_BUTTON) {
		const Size2 tex_size = _get_check_icon_size();
		if (tex_size.width > 0 || tex_size.height > 0) {
			const Size2 padding = _get_largest_stylebox_size();
			Size2 content_size = minsize - padding;
			if (content_size.width > 0 && tex_size.width > 0) {
				content_size.width += MAX(0, theme_cache.h_separation);
			}
			content_size.width += tex_size.width;
			content_size.height = MAX(content_size.height, tex_size.height);
			minsize = content_size + padding;
		}
	}

	return minsize;
}

void Button::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		if (style == STYLE_CHECK_BOX) {
			AccessibilityServer::update_set_role(ae,
				is_radio() ? AccessibilityServerEnums::AccessibilityRole::ROLE_RADIO_BUTTON
						   : AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BOX);
		}
		else if (style == STYLE_CHECK_BUTTON) {
			AccessibilityServer::update_set_role(
				ae, AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BUTTON);
		}
		else {
			AccessibilityServer::update_set_role(
				ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
			if (style == STYLE_COLOR_SWATCH) {
				AccessibilityServer::update_set_color_value(ae, swatch_color);
			}
		}
	} break;

	case NOTIFICATION_THEME_CHANGED:
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (style == STYLE_CHECK_BOX) {
			const float w = _get_check_icon_size().width;
			_set_internal_margin(SIDE_LEFT, is_layout_rtl() ? 0.f : w);
			_set_internal_margin(SIDE_RIGHT, is_layout_rtl() ? w : 0.f);
		}
		else if (style == STYLE_CHECK_BUTTON) {
			const float w = _get_check_icon_size().width;
			_set_internal_margin(SIDE_LEFT, is_layout_rtl() ? w : 0.f);
			_set_internal_margin(SIDE_RIGHT, is_layout_rtl() ? 0.f : w);
		}
	} break;

	case NOTIFICATION_DRAW: {
		if (style == STYLE_TEXTURE) {
			DrawMode draw_mode = get_draw_mode();
			Ref<Texture2D> texdraw;

			switch (draw_mode) {
			case DRAW_NORMAL:
				texdraw = textures.normal;
				break;
			case DRAW_HOVER_PRESSED:
			case DRAW_PRESSED:
				texdraw = textures.pressed.is_valid()
							  ? textures.pressed
							  : (textures.hover.is_valid() ? textures.hover : textures.normal);
				break;
			case DRAW_HOVER:
				texdraw = textures.hover.is_valid()
							  ? textures.hover
							  : ((textures.pressed.is_valid() && is_pressed()) ? textures.pressed
																			   : textures.normal);
				break;
			case DRAW_DISABLED:
				texdraw = textures.disabled.is_valid() ? textures.disabled : textures.normal;
				break;
			}

			Point2 ofs;
			Size2 size;
			bool draw_focus = (has_focus(true) && textures.focused.is_valid());
			bool draw_focus_only = draw_focus && texdraw.is_null();
			if (draw_focus_only) {
				texdraw = textures.focused;
			}

			if (texdraw.is_valid() || textures.click_mask.is_valid()) {
				const Size2 texdraw_size = texdraw.is_valid()
											   ? texdraw->get_size()
											   : Size2(textures.click_mask->get_size());
				size = texdraw_size;
				textures.texture_region = Rect2(Point2(), texdraw_size);
				textures.tile = false;

				switch (textures.stretch_mode) {
				case STRETCH_KEEP:
					break;
				case STRETCH_SCALE:
					size = get_size();
					break;
				case STRETCH_TILE:
					size = get_size();
					textures.tile = true;
					break;
				case STRETCH_KEEP_CENTERED:
					ofs = (get_size() - texdraw_size) / 2;
					break;
				case STRETCH_KEEP_ASPECT_CENTERED:
				case STRETCH_KEEP_ASPECT: {
					Size2 _size = get_size();
					float tex_width = texdraw_size.width * _size.height / texdraw_size.height;
					float tex_height = _size.height;
					if (tex_width > _size.width) {
						tex_width = _size.width;
						tex_height = texdraw_size.height * tex_width / texdraw_size.width;
					}
					if (textures.stretch_mode == STRETCH_KEEP_ASPECT_CENTERED) {
						ofs.x = (_size.width - tex_width) / 2;
						ofs.y = (_size.height - tex_height) / 2;
					}
					size.width = tex_width;
					size.height = tex_height;
				} break;
				case STRETCH_KEEP_ASPECT_COVERED: {
					size = get_size();
					Size2 scale_size = size / texdraw_size;
					float scale = MAX(scale_size.width, scale_size.height);
					Size2 scaled_tex_size = texdraw_size * scale;
					Point2 ofs2 = ((scaled_tex_size - size) / scale).abs() / 2.0f;
					textures.texture_region = Rect2(ofs2, size / scale);
				} break;
				}

				textures.position_rect = Rect2(ofs, size);
				size.width *= textures.hflip ? -1.0f : 1.0f;
				size.height *= textures.vflip ? -1.0f : 1.0f;

				if (!draw_focus_only && texdraw.is_valid()) {
					if (textures.tile) {
						draw_texture_rect(texdraw.ptr(), Rect2(ofs, size), true);
					}
					else {
						draw_texture_rect_region(
							texdraw.ptr(), Rect2(ofs, size), textures.texture_region);
					}
				}
			}
			else {
				textures.position_rect = Rect2();
			}

			if (draw_focus) {
				draw_texture_rect(textures.focused.ptr(), Rect2(ofs, size), false);
			}
		}
		else if (style == STYLE_COLOR_SWATCH) {
			draw_rect(Rect2(Point2(), get_size()), swatch_color);
		}
	} break;
	}
}

void Button::_set_internal_margin(Side p_side, float p_value)
{
	_internal_margin[p_side] = p_value;
}

String Button::_get_translated_text(const String& p_text) const { return String(); }

void Button::_queue_update_size_cache() {}

void Button::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.max_style_size = Vector2();
	theme_cache.style_margin_left = 0;
	theme_cache.style_margin_right = 0;
	theme_cache.style_margin_top = 0;
	theme_cache.style_margin_bottom = 0;

	const bool rtl = is_layout_rtl();
	if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
		_update_style_margins(theme_cache.normal_mirrored);
	}
	else if (theme_cache.normal.is_valid()) {
		_update_style_margins(theme_cache.normal);
	}
	if (has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			_update_style_margins(theme_cache.hover_pressed_mirrored);
		}
		else if (theme_cache.hover_pressed.is_valid()) {
			_update_style_margins(theme_cache.hover_pressed);
		}
	}
	if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
		_update_style_margins(theme_cache.pressed_mirrored);
	}
	else if (theme_cache.pressed.is_valid()) {
		_update_style_margins(theme_cache.pressed);
	}
	if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
		_update_style_margins(theme_cache.hover_mirrored);
	}
	else if (theme_cache.hover.is_valid()) {
		_update_style_margins(theme_cache.hover);
	}
	if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
		_update_style_margins(theme_cache.disabled_mirrored);
	}
	else if (theme_cache.disabled.is_valid()) {
		_update_style_margins(theme_cache.disabled);
	}
	theme_cache.max_style_size = theme_cache.max_style_size.max(
		Vector2(theme_cache.style_margin_left + theme_cache.style_margin_right,
			theme_cache.style_margin_top + theme_cache.style_margin_bottom));
}

Size2 Button::_get_largest_stylebox_size() const { return theme_cache.max_style_size; }

Ref<StyleBox> Button::_get_current_stylebox() const
{
	Ref<StyleBox> stylebox = theme_cache.normal;
	const bool rtl = is_layout_rtl();

	switch (get_draw_mode()) {
	case DRAW_NORMAL: {
		stylebox = (rtl && has_theme_stylebox(SNAME("normal_mirrored")))
					   ? theme_cache.normal_mirrored
					   : theme_cache.normal;
	} break;

	case DRAW_HOVER_PRESSED: {
		if (has_theme_stylebox("hover_pressed")) {
			stylebox = (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored")))
						   ? theme_cache.hover_pressed_mirrored
						   : theme_cache.hover_pressed;
			break;
		}
	}
		[[fallthrough]];
	case DRAW_PRESSED: {
		stylebox = (rtl && has_theme_stylebox(SNAME("pressed_mirrored")))
					   ? theme_cache.pressed_mirrored
					   : theme_cache.pressed;
	} break;

	case DRAW_HOVER: {
		stylebox = (rtl && has_theme_stylebox(SNAME("hover_mirrored")))
					   ? theme_cache.hover_mirrored
					   : theme_cache.hover;
	} break;

	case DRAW_DISABLED: {
		stylebox = (rtl && has_theme_stylebox(SNAME("disabled_mirrored")))
					   ? theme_cache.disabled_mirrored
					   : theme_cache.disabled;
	} break;
	}

	return stylebox;
}

String Button::_get_accessibility_name() const
{
	const String& ac_name = Control::_get_accessibility_name();
	if (!xl_text.is_empty() && ac_name.is_empty()) {
		return xl_text;
	}
	else if (!xl_text.is_empty() && !ac_name.is_empty() && ac_name != xl_text) {
		return ac_name + ": " + xl_text;
	}
	else if (xl_text.is_empty() && ac_name.is_empty() && !get_tooltip_text().is_empty()) {
		return get_tooltip_text();
	}
	else {
		return ac_name;
	}
}

Size2 Button::_fit_icon_size(const Size2& p_size) const
{
	int max_width = theme_cache.icon_max_width;
	Size2 icon_size = p_size;

	if (max_width > 0 && icon_size.width > max_width) {
		icon_size.height = icon_size.height * max_width / icon_size.width;
		icon_size.width = max_width;
	}

	return icon_size;
}

Size2 Button::get_minimum_size_for_text_and_icon(const String& p_text, Ref<Texture2D> p_icon) const
{
	Ref<TextParagraph> paragraph;
	if (p_text.is_empty()) {
		paragraph = text_buf;
	}
	else {
		paragraph.instantiate();
		_shape(paragraph, p_text);
	}

	Size2 minsize = paragraph.is_valid() ? paragraph->get_size() : Size2();
	if (clip_text || overrun_behavior != TextServer::OVERRUN_NO_TRIMMING ||
		autowrap_mode != TextServer::AUTOWRAP_OFF) {
		minsize.width = 0;
	}

	if (!expand_icon && p_icon.is_valid()) {
		Size2 icon_size = _fit_icon_size(p_icon->get_size());
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(minsize.height, icon_size.height);
		}
		else {
			minsize.height += icon_size.height;
		}

		if (horizontal_icon_alignment != HORIZONTAL_ALIGNMENT_CENTER) {
			minsize.width += icon_size.width;
			if (!xl_text.is_empty() || !p_text.is_empty()) {
				minsize.width += MAX(0, theme_cache.h_separation);
			}
		}
		else {
			minsize.width = MAX(minsize.width, icon_size.width);
		}
	}

	if ((!xl_text.is_empty() || !p_text.is_empty()) && theme_cache.font.is_valid()) {
		float font_height = theme_cache.font->get_height(theme_cache.font_size);
		if (vertical_icon_alignment == VERTICAL_ALIGNMENT_CENTER) {
			minsize.height = MAX(font_height, minsize.height);
		}
		else {
			minsize.height += font_height;
		}
	}

	Ref<StyleBox> sb = _get_current_stylebox();
	Size2 sb_size = theme_cache.align_to_largest_stylebox
						? _get_largest_stylebox_size()
						: (sb.is_valid() ? sb->get_minimum_size() : Size2());
	return sb_size + minsize;
}

TextServer::OverrunBehavior Button::get_text_overrun_behavior() const { return overrun_behavior; }

String Button::get_text() const { return text; }

void Button::set_text(const String& p_text)
{
	if (text == p_text) {
		return;
	}
	text = p_text;
	xl_text = p_text;
	update_minimum_size();
	queue_redraw();
}

TextServer::AutowrapMode Button::get_autowrap_mode() const { return autowrap_mode; }
void Button::set_autowrap_mode(TextServer::AutowrapMode p_mode) { autowrap_mode = p_mode; }
void Button::set_autowrap_trim_flags(uint32_t p_flags) { autowrap_flags_trim = p_flags; }
uint32_t Button::get_autowrap_trim_flags() const { return autowrap_flags_trim; }
void Button::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	overrun_behavior = p_behavior;
}
void Button::set_text_direction(TextDirection p_text_direction)
{
	text_direction = p_text_direction;
}
Control::TextDirection Button::get_text_direction() const { return text_direction; }
void Button::set_language(const String& p_language) { language = p_language; }
String Button::get_language() const { return language; }

void Button::_update_style_margins(const Ref<StyleBox>& p_stylebox)
{
	if (p_stylebox.is_null()) {
		return;
	}
	theme_cache.max_style_size = theme_cache.max_style_size.max(p_stylebox->get_minimum_size());
	theme_cache.style_margin_left =
		MAX(theme_cache.style_margin_left, p_stylebox->get_margin(SIDE_LEFT));
	theme_cache.style_margin_right =
		MAX(theme_cache.style_margin_right, p_stylebox->get_margin(SIDE_RIGHT));
	theme_cache.style_margin_top =
		MAX(theme_cache.style_margin_top, p_stylebox->get_margin(SIDE_TOP));
	theme_cache.style_margin_bottom =
		MAX(theme_cache.style_margin_bottom, p_stylebox->get_margin(SIDE_BOTTOM));
}

Ref<Texture2D> Button::get_button_icon() const { return icon; }
void Button::set_expand_icon(bool p_enabled) { expand_icon = p_enabled; }
bool Button::is_expand_icon() const { return expand_icon; }
void Button::set_flat(bool p_enabled) { flat = p_enabled; }
bool Button::is_flat() const { return flat; }
void Button::set_clip_text(bool p_enabled) { clip_text = p_enabled; }
bool Button::get_clip_text() const { return clip_text; }
void Button::set_text_alignment(HorizontalAlignment p_alignment) { alignment = p_alignment; }
HorizontalAlignment Button::get_text_alignment() const { return alignment; }
void Button::set_icon_alignment(HorizontalAlignment p_alignment)
{
	horizontal_icon_alignment = p_alignment;
}
void Button::set_vertical_icon_alignment(VerticalAlignment p_alignment)
{
	vertical_icon_alignment = p_alignment;
}
HorizontalAlignment Button::get_icon_alignment() const { return horizontal_icon_alignment; }
VerticalAlignment Button::get_vertical_icon_alignment() const { return vertical_icon_alignment; }

void Button::_shape(Ref<TextParagraph>, String) const {}

Button::Button(const String& p_text, Style p_style)
{
	set_focus_mode(FOCUS_ALL);
	text_buf.instantiate();
	text_buf->set_break_flags(TextServer::BREAK_MANDATORY | autowrap_flags_trim);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_style(p_style);
	set_text(p_text);
}

Button::~Button()
{
	if (behavior.button_group.is_valid()) {
		behavior.button_group->buttons.erase(this);
	}
}

void ButtonGroup::get_buttons(List<Button*>* r_buttons)
{
	for (Button* E : buttons) {
		r_buttons->push_back(E);
	}
}

Button* ButtonGroup::get_pressed_button()
{
	for (Button* E : buttons) {
		if (E->is_pressed()) {
			return E;
		}
	}
	return nullptr;
}

ButtonGroup::ButtonGroup() { set_local_to_scene(true); }

// --- menu_button.cpp ---

void MenuButton::pressed()
{
	if (popup && popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

PopupMenu* MenuButton::get_popup() const { return popup; }

void MenuButton::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuButton::is_switch_on_hover() { return switch_on_hover; }

int MenuButton::get_item_count() const { return popup ? popup->get_item_count() : 0; }

void MenuButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray MenuButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (popup) {
		warnings.append_array(popup->get_configuration_warnings());
	}
	return warnings;
}
#endif

MenuButton::MenuButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_flat(true);
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);

	popup = memnew(PopupMenu);
	popup->hide();
	add_child(popup, false, INTERNAL_MODE_FRONT);
}

MenuButton::~MenuButton() {}

void MenuButton::show_popup() {}

// --- option_button.cpp ---

static const int NONE_SELECTED = -1;

Size2 OptionButton::get_minimum_size() const
{
	Size2 minsize = fit_to_longest_item
						? _cached_size
						: (button ? button->get_minimum_size() : Size2());

	if (has_theme_icon(SNAME("arrow")) && theme_cache.arrow_icon.is_valid()) {
		const Size2 padding = theme_cache.normal.is_valid()
								  ? theme_cache.normal->get_minimum_size()
								  : Size2();
		const Size2 arrow_size = theme_cache.arrow_icon->get_size();

		Size2 content_size = minsize - padding;
		content_size.width += arrow_size.width + MAX(0, theme_cache.h_separation);
		content_size.height = MAX(content_size.height, arrow_size.height);

		minsize = content_size + padding;
	}

	if (fit_to_longest_item && popup) {
		minsize.width = MAX(minsize.width, popup->get_contents_minimum_size().width);
	}

	return minsize;
}

void OptionButton::_selected(int p_which) { _select(p_which, true); }

void OptionButton::pressed()
{
	if (popup && popup->is_visible()) {
		popup->hide();
		return;
	}

	show_popup();
}

void OptionButton::add_icon_item(const Ref<Texture2D>& p_icon, const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::add_item(const String& p_label, int p_id)
{
	bool first_selectable = !has_selectable_items();
	if (first_selectable) {
		select(get_item_count() - 1);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_text(int p_idx, const String& p_text)
{
	if (current == p_idx) {
		set_text(p_text);
	}
	_queue_update_size_cache();
}

void OptionButton::set_item_id(int p_idx, int p_id) {}

void OptionButton::set_item_tooltip(int p_idx, const String& p_tooltip) {}

void OptionButton::set_item_auto_translate_mode(int p_idx, AutoTranslateMode p_mode)
{
	if (!popup) {
		return;
	}
	if (p_idx < 0) {
		p_idx += get_item_count();
	}
	if (popup->get_item_auto_translate_mode(p_idx) == p_mode) {
		return;
	}
	if (current == p_idx) {
		set_text(popup->get_item_text(p_idx));
	}
	_queue_update_size_cache();
}

String OptionButton::get_item_text(int p_idx) const
{
	return popup ? popup->get_item_text(p_idx) : String();
}

Ref<Texture2D> OptionButton::get_item_icon(int p_idx) const
{
	return popup ? popup->get_item_icon(p_idx) : Ref<Texture2D>();
}

int OptionButton::get_item_id(int p_idx) const
{
	if (p_idx == NONE_SELECTED || !popup) {
		return NONE_SELECTED;
	}

	return popup->get_item_id(p_idx);
}

int OptionButton::get_item_index(int p_id) const
{
	return popup ? popup->get_item_index(p_id) : -1;
}

String OptionButton::get_item_tooltip(int p_idx) const
{
	return popup ? popup->get_item_tooltip(p_idx) : String();
}

Node::AutoTranslateMode OptionButton::get_item_auto_translate_mode(int p_idx) const
{
	return popup ? popup->get_item_auto_translate_mode(p_idx) : AUTO_TRANSLATE_MODE_INHERIT;
}

bool OptionButton::is_item_disabled(int p_idx) const
{
	return popup ? popup->is_item_disabled(p_idx) : false;
}

bool OptionButton::is_item_separator(int p_idx) const
{
	return popup ? popup->is_item_separator(p_idx) : false;
}

bool OptionButton::has_selectable_items() const
{
	for (int i = 0; i < get_item_count(); i++) {
		if (!is_item_disabled(i) && !is_item_separator(i)) {
			return true;
		}
	}
	return false;
}

int OptionButton::get_selectable_item(bool p_from_last) const
{
	if (!p_from_last) {
		for (int i = 0; i < get_item_count(); i++) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	else {
		for (int i = get_item_count() - 1; i >= 0; i--) {
			if (!is_item_disabled(i) && !is_item_separator(i)) {
				return i;
			}
		}
	}
	return -1;
}

int OptionButton::get_item_count() const { return popup ? popup->get_item_count() : 0; }

void OptionButton::set_fit_to_longest_item(bool p_fit)
{
	if (p_fit == fit_to_longest_item) {
		return;
	}
	fit_to_longest_item = p_fit;

	_refresh_size_cache();
}

bool OptionButton::is_fit_to_longest_item() const { return fit_to_longest_item; }

void OptionButton::set_allow_reselect(bool p_allow) { allow_reselect = p_allow; }

bool OptionButton::get_allow_reselect() const { return allow_reselect; }

bool OptionButton::is_search_bar_enabled() const
{
	return popup ? popup->is_search_bar_enabled() : false;
}

int OptionButton::get_search_bar_min_item_count() const
{
	return popup ? popup->get_search_bar_min_item_count() : 0;
}

void OptionButton::set_search_bar_fuzzy_search_enabled(bool p_enabled)
{
	if (popup) {
		popup->set_search_bar_fuzzy_search_enabled(p_enabled);
	}
}

bool OptionButton::is_search_bar_fuzzy_search_enabled() const
{
	return popup ? popup->is_search_bar_fuzzy_search_enabled() : false;
}

void OptionButton::set_search_bar_fuzzy_search_max_misses(int p_max_misses)
{
	if (popup) {
		popup->set_search_bar_fuzzy_search_max_misses(p_max_misses);
	}
}

int OptionButton::get_search_bar_fuzzy_search_max_misses() const
{
	return popup ? popup->get_search_bar_fuzzy_search_max_misses() : 0;
}

void OptionButton::add_separator(const String& p_text) {}

void OptionButton::_select_int(int p_which)
{
	if (p_which < NONE_SELECTED || !popup) {
		return;
	}
	if (p_which >= popup->get_item_count()) {
		if (!initialized) {
			queued_current = p_which;
		}
		return;
	}
	_select(p_which, false);
}

void OptionButton::select(int p_idx) { _select(p_idx, false); }

int OptionButton::get_selected() const { return current; }

int OptionButton::get_selected_id() const { return get_item_id(current); }

void OptionButton::remove_item(int p_idx)
{
	if (current == p_idx) {
		_select(NONE_SELECTED);
	}
	_queue_update_size_cache();
}

PopupMenu* OptionButton::get_popup() const { return popup; }

void OptionButton::show_popup()
{
	if (!get_viewport() || !popup) {
		return;
	}

	if (current != NONE_SELECTED && !popup->is_item_disabled(current)) {
		popup->scroll_to_item(current);
	}
	else {
		for (int i = 0; i < popup->get_item_count(); i++) {
			if (!popup->is_item_disabled(i)) {
				popup->scroll_to_item(i);
				break;
			}
		}
	}

	Rect2 rect = get_screen_rect();
	rect.position.y += rect.size.height;
	if (get_viewport()->is_embedding_subwindows() && popup->get_force_native()) {
		Transform2D xform = get_viewport()->get_popup_base_transform_native();
		rect = xform.xform(rect);
	}
	rect.size.height = 0;
	popup->set_min_size(Size2(0, 0));
	popup->popup(rect);
}

void OptionButton::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

#ifdef TOOLS_ENABLED
PackedStringArray OptionButton::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();
	if (popup) {
		warnings.append_array(popup->get_configuration_warnings());
	}
	return warnings;
}
#endif

OptionButton::OptionButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);

	popup = memnew(PopupMenu);
	popup->hide();
	add_child(popup, false, INTERNAL_MODE_FRONT);
}

OptionButton::~OptionButton() {}

void OptionButton::_select(int p_which, bool p_emit) {}

void OptionButton::set_item_icon(int p_idx, const Ref<Texture2D>& p_icon) {}

void OptionButton::clear() {}

void OptionButton::_refresh_size_cache() {}

void OptionButton::_queue_update_size_cache() {}

String OptionButton::_get_translated_text(const String& p_text) const { return p_text; }

// --- menu_bar.cpp ---

bool MenuBar::is_native_menu() const
{
#ifdef TOOLS_ENABLED
	if (is_part_of_edited_scene()) {
		return false;
	}
#endif

	return (
		NativeMenu::has_feature(NativeMenu::FEATURE_GLOBAL_MENU) && prefer_native);
}

void MenuBar::unbind_global_menu()
{
	if (global_menu_tag.is_empty()) {
		return;
	}

	RID main_menu = NativeMenu::get_system_menu(NativeMenu::MAIN_MENU_ID);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = menu_cache.size() - 1; i >= 0; i--) {
		if (!popups[i]->is_system_menu()) {
			if (menu_cache[i].submenu_rid.is_valid()) {
				int item_idx =
					NativeMenu::find_item_index_with_submenu(main_menu, menu_cache[i].submenu_rid);
				if (item_idx >= 0) {
					NativeMenu::remove_item(main_menu, item_idx);
				}
			}
			popups[i]->unbind_global_menu();
			menu_cache.write[i].submenu_rid = RID();
		}
		menu_cache.write[i].sysmenu_id = NativeMenu::INVALID_MENU_ID;
	}

	global_menu_tag = String();
}

int MenuBar::_get_index_at_point(const Point2& p_point) const
{
	Ref<StyleBox> style = theme_cache.normal;
	int offset = 0;
	Point2 point = p_point;
	if (is_layout_rtl()) {
		point.x = get_size().x - point.x;
	}

	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		if (point.x > offset && point.x < offset + size.x) {
			if (point.y > 0 && point.y < size.y) {
				return i;
			}
		}
		offset += size.x + theme_cache.h_separation;
	}
	return -1;
}

Rect2 MenuBar::_get_menu_item_rect(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, menu_cache.size(), Rect2());

	Ref<StyleBox> style = theme_cache.normal;

	int offset = 0;
	for (int i = 0; i < p_index; i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 size = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		offset += size.x + theme_cache.h_separation;
	}

	Size2 size = menu_cache[p_index].text_buf->get_size() + style->get_minimum_size();
	if (is_layout_rtl()) {
		return Rect2(Point2(get_size().x - offset - size.x, 0), size);
	}
	else {
		return Rect2(Point2(offset, 0), size);
	}
}

void MenuBar::_draw_menu_item(int p_index)
{
	ERR_FAIL_INDEX(p_index, menu_cache.size());

	RID ci = get_canvas_item();
	bool hovered = (focused_menu == p_index);
	bool pressed = (active_menu == p_index);
	bool rtl = is_layout_rtl();

	if (menu_cache[p_index].hidden) {
		return;
	}

	Color color;
	Ref<StyleBox> style;
	Rect2 item_rect = _get_menu_item_rect(p_index);

	if (menu_cache[p_index].disabled) {
		if (rtl && has_theme_stylebox(SNAME("disabled_mirrored"))) {
			style = theme_cache.disabled_mirrored;
		}
		else {
			style = theme_cache.disabled;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_disabled_color;
	}
	else if (hovered && pressed && has_theme_stylebox("hover_pressed")) {
		if (rtl && has_theme_stylebox(SNAME("hover_pressed_mirrored"))) {
			style = theme_cache.hover_pressed_mirrored;
		}
		else {
			style = theme_cache.hover_pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_hover_pressed_color"))) {
			color = theme_cache.font_hover_pressed_color;
		}
	}
	else if (pressed) {
		if (rtl && has_theme_stylebox(SNAME("pressed_mirrored"))) {
			style = theme_cache.pressed_mirrored;
		}
		else {
			style = theme_cache.pressed;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		if (has_theme_color(SNAME("font_pressed_color"))) {
			color = theme_cache.font_pressed_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}
	else if (hovered) {
		if (rtl && has_theme_stylebox(SNAME("hover_mirrored"))) {
			style = theme_cache.hover_mirrored;
		}
		else {
			style = theme_cache.hover;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		color = theme_cache.font_hover_color;
	}
	else {
		if (rtl && has_theme_stylebox(SNAME("normal_mirrored"))) {
			style = theme_cache.normal_mirrored;
		}
		else {
			style = theme_cache.normal;
		}
		if (!flat) {
			style->draw(ci, item_rect);
		}
		// Focus colors only take precedence over normal state.
		if (has_focus(true)) {
			color = theme_cache.font_focus_color;
		}
		else {
			color = theme_cache.font_color;
		}
	}

	Point2 text_ofs =
		item_rect.position + Point2(style->get_margin(SIDE_LEFT), style->get_margin(SIDE_TOP));

	Color font_outline_color = theme_cache.font_outline_color;
	int outline_size = theme_cache.outline_size;
	if (outline_size > 0 && font_outline_color.a > 0) {
		menu_cache[p_index].text_buf->draw_outline(ci, text_ofs, outline_size, font_outline_color);
	}
	menu_cache[p_index].text_buf->draw(ci, text_ofs, color);
}

int MenuBar::get_menu_idx_from_control(PopupMenu* p_child) const
{
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);

	Vector<PopupMenu*> popups = _get_popups();
	for (int i = 0; i < popups.size(); i++) {
		if (popups[i] == p_child) {
			return i;
		}
	}

	return -1;
}

void MenuBar::set_switch_on_hover(bool p_enabled) { switch_on_hover = p_enabled; }

bool MenuBar::is_switch_on_hover() { return switch_on_hover; }

void MenuBar::set_disable_shortcuts(bool p_disabled) { disable_shortcuts = p_disabled; }

Control::TextDirection MenuBar::get_text_direction() const { return text_direction; }

String MenuBar::get_language() const { return language; }

bool MenuBar::is_flat() const { return flat; }

void MenuBar::set_start_index(int p_index)
{
	if (start_index != p_index) {
		start_index = p_index;
		if (!global_menu_tag.is_empty()) {
			unbind_global_menu();
			bind_global_menu();
		}
	}
}

int MenuBar::get_start_index() const { return start_index; }

void MenuBar::set_prefer_global_menu(bool p_enabled)
{
	if (prefer_native != p_enabled) {
		prefer_native = p_enabled;
		if (prefer_native) {
			bind_global_menu
();
		}
		else {
			unbind_global_menu();
		}
	}
}

bool MenuBar::is_prefer_global_menu() const { return prefer_native; }

Size2 MenuBar::get_minimum_size() const
{
	if (is_native_menu()) {
		return Size2();
	}

	Ref<StyleBox> style = theme_cache.normal;

	Vector2 size;
	for (int i = 0; i < menu_cache.size(); i++) {
		if (menu_cache[i].hidden) {
			continue;
		}
		Size2 sz = menu_cache[i].text_buf->get_size() + style->get_minimum_size();
		size.y = MAX(size.y, sz.y);
		size.x += sz.x;
	}
	if (menu_cache.size() > 1) {
		size.x += theme_cache.h_separation * (menu_cache.size() - 1);
	}
	return size;
}

int MenuBar::get_menu_count() const { return menu_cache.size(); }

String MenuBar::get_menu_title(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].name;
}

String MenuBar::get_menu_tooltip(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), String());
	return menu_cache[p_menu].tooltip;
}

void MenuBar::set_menu_disabled(int p_menu, bool p_disabled)
{
	ERR_FAIL_INDEX(p_menu, menu_cache.size());
	menu_cache.write[p_menu].disabled = p_disabled;
	if (!global_menu_tag.is_empty() && menu_cache[p_menu].submenu_rid.is_valid()) {
		RID main_menu = NativeMenu::get_system_menu(NativeMenu::MAIN_MENU_ID);
		int item_idx =
			NativeMenu::find_item_index_with_submenu(main_menu, menu_cache[p_menu].submenu_rid);
		if (item_idx >= 0) {
			NativeMenu::set_item_disabled(main_menu, item_idx, p_disabled);
		}
	}
}

bool MenuBar::is_menu_disabled(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].disabled;
}

bool MenuBar::is_menu_hidden(int p_menu) const
{
	ERR_FAIL_INDEX_V(p_menu, menu_cache.size(), false);
	return menu_cache[p_menu].hidden;
}

PopupMenu* MenuBar::get_menu_popup(int p_idx) const
{
	Vector<PopupMenu*> controls = _get_popups();
	if (p_idx >= 0 && p_idx < controls.size()) {
		return controls[p_idx];
	}
	else {
		return nullptr;
	}
}

String MenuBar::get_tooltip(const Point2& p_pos) const
{
	int index = _get_index_at_point(p_pos);
	if (index >= 0 && index < menu_cache.size()) {
		return menu_cache[index].tooltip;
	}
	else {
		return String();
	}
}

MenuBar::MenuBar()
{
	set_focus_mode(FOCUS_ACCESSIBILITY);
	set_process_shortcut_input(true);
}

Vector<PopupMenu*> MenuBar::_get_popups() const {}

void MenuBar::bind_global_menu() {}

void MenuBar::add_child_notify(Node* p_child) {}

void MenuBar::remove_child_notify(Node* p_child) {}

void MenuBar::move_child_notify(Node* p_child) {}

// ============================================================================
// Merged from scene/gui/range.cpp
// ============================================================================

#include <cmath>
#include <thirdparty/misc/r128.h>
#include "core/string/translation_server.h"
#include "range.h"
#include "core/config/engine.h"
// --- Inlined from scene/gui/progress_bar.h ---
#include "scene/gui/control.h"
#include "scene/gui/range.h"

class ProgressBar : public Control
{
	bool show_percentage = true;
	bool indeterminate = false;
	bool editor_preview_indeterminate = false;

	struct ThemeCache
	{
		Ref<StyleBox> background_style;
		Ref<StyleBox> fill_style;

		Ref<Font> font;
		int font_size = 0;
		Color font_color;
		int font_outline_size = 0;
		Color font_outline_color;
	} theme_cache;

protected:
	void _notification(int p_what);

	double indeterminate_min_speed = 200.0;

public:
	enum FillMode
	{
		FILL_BEGIN_TO_END,
		FILL_END_TO_BEGIN,
		FILL_TOP_TO_BOTTOM,
		FILL_BOTTOM_TO_TOP,
		FILL_MODE_MAX
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

	void set_show_percentage(bool p_visible);
	bool is_percentage_shown() const;

	void set_indeterminate(bool p_indeterminate);
	bool is_indeterminate() const;

	void set_editor_preview_indeterminate(bool p_indeterminate_preview);
	bool is_editor_preview_indeterminate_enabled() const;

	Size2 get_minimum_size() const override;
	PackedStringArray get_configuration_warnings() const override
	{
		PackedStringArray warnings = Control::get_configuration_warnings();
		range.append_configuration_warnings(warnings);
		return warnings;
	}

	ProgressBar();

private:
	float _indeterminate_fill_progress = 0;

	FillMode mode = FILL_BEGIN_TO_END;
};

// --- End scene/gui/progress_bar.h ---
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"
#include "scene/gui/slider.h"
#include "core/input/input.h"
#include "slider.h"
#include "core/math/expression.h"
#include "spin_box.h"
#include "scene/main/window.h"
#include "scroll_bar.h"
#include "servers/display/display_server.h"

double NumericRange::snapped_r128(double p_value, double p_step)
{
	if (p_step == 0.0) {
		return p_value;
	}
	if (p_value > (1e18 * p_step) || p_value < -(1e18 * p_step)) {
		return Math::snapped(p_value, p_step);
	}
	const int decimals = MIN(18, 14 - Math::floor(std::log10(MAX(Math::abs(p_value), p_step))));
	const double scale = Math::pow(10.0, decimals);
	p_value *= scale;
	p_step *= scale;
	String step_str = String::num(p_step);
	String value_str = String::num(p_value);
	R128 step_r128;
	R128 value_r128;
	const R128 half_r128 = R128(0.5);
	r128FromString(&step_r128, step_str.ascii().get_data(), nullptr);
	r128FromString(&value_r128, value_str.ascii().get_data(), nullptr);
	r128Div(&value_r128, &value_r128, &step_r128);
	r128Add(&value_r128, &value_r128, &half_r128);
	r128Floor(&value_r128, &value_r128);
	r128Mul(&value_r128, &value_r128, &step_r128);
	if (scale != 1.0) {
		const R128 scale_r128 = R128(scale);
		r128Div(&value_r128, &value_r128, &scale_r128);
	}
	return (double)value_r128;
}

double NumericRange::calc_value(double p_val, double p_step) const
{
	if (Math::is_nan(p_val)) {
		return p_val;
	}

	if (p_step > 0) {
		if (Math::abs(min) > p_step * 1e14) {
			p_val = snapped_r128(p_val, p_step);
		}
		else {
			p_val = snapped_r128(p_val - min, p_step) + min;
		}
	}

	if (rounded_values) {
		p_val = Math::round(p_val);
	}

	if (!allow_greater && p_val > max - page) {
		p_val = max - page;
	}

	if (!allow_lesser && p_val < min) {
		p_val = min;
	}
	return p_val;
}

bool NumericRange::set_value(double p_val)
{
	double prev_val = value;
	value = calc_value(p_val, step);
	return value != prev_val && !(Math::is_nan(value) && Math::is_nan(prev_val));
}

bool NumericRange::set_min(double p_min)
{
	if (min == p_min) {
		return false;
	}
	min = p_min;
	max = MAX(max, min);
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_max(double p_max)
{
	double max_validated = MAX(p_max, min);
	if (max == max_validated) {
		return false;
	}
	max = max_validated;
	page = CLAMP(page, 0.0, max - min);
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_step(double p_step)
{
	if (step == p_step) {
		return false;
	}
	step = p_step;
	return true;
}

bool NumericRange::set_page(double p_page)
{
	double page_validated = CLAMP(p_page, 0.0, max - min);
	if (page == page_validated) {
		return false;
	}
	page = page_validated;
	value = calc_value(value, step);
	return true;
}

bool NumericRange::set_as_ratio(double p_ratio)
{
	double v;
	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		v = Math::pow(2.0, exp_min + (exp_max - exp_min) * p_ratio);
	}
	else {
		double percent = (max - min) * p_ratio;
		if (step > 0) {
			double steps = std::round(percent / step);
			v = steps * step + min;
		}
		else {
			v = percent + min;
		}
	}
	v = CLAMP(v, min, max);
	return set_value(v);
}

double NumericRange::get_as_ratio() const
{
	if (Math::is_equal_approx(max, min)) {
		return 1.0;
	}

	if (exp_ratio && min >= 0) {
		double exp_min = min == 0 ? 0.0 : Math::log(min) / Math::log(2.0);
		double exp_max = Math::log(max) / Math::log(2.0);
		double clamped = CLAMP(value, min, max);
		double v = Math::log(clamped) / Math::log(2.0);
		return CLAMP((v - exp_min) / (exp_max - exp_min), 0.0, 1.0);
	}
	else {
		double clamped = CLAMP(value, min, max);
		return CLAMP((clamped - min) / (max - min), 0.0, 1.0);
	}
}

void NumericRange::append_configuration_warnings(PackedStringArray& r_warnings) const
{
	if (exp_ratio && min < 0) {
		r_warnings.push_back(
			RTR("If \"Exp Edit\" is enabled, \"Min Value\" must be greater or equal to 0."));
	}
}

// --- progress_bar.cpp ---

Size2 ProgressBar::get_minimum_size() const
{
	Size2 minimum_size = theme_cache.background_style->get_minimum_size();
	minimum_size = minimum_size.max(theme_cache.fill_style->get_minimum_size());
	if (show_percentage) {
		String txt = "100%";
		TextLine tl = TextLine(txt, theme_cache.font, theme_cache.font_size);
		minimum_size.height = MAX(minimum_size.height,
			theme_cache.background_style->get_minimum_size().height + tl.get_size().y);
	}
	else { // this is needed, else the progressbar will collapse
		minimum_size = minimum_size.maxf(1);
	}
	return minimum_size;
}

int ProgressBar::get_fill_mode() { return mode; }

bool ProgressBar::is_percentage_shown() const { return show_percentage; }

bool ProgressBar::is_indeterminate() const { return indeterminate; }

bool ProgressBar::is_editor_preview_indeterminate_enabled() const
{
	return editor_preview_indeterminate;
}

ProgressBar::ProgressBar()
{
	set_v_size_flags(0);
	set_step(0.01);
}

// --- texture_progress_bar.cpp ---

void TextureProgressBar::set_under_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&under, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_under_texture() const { return under; }

void TextureProgressBar::set_over_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&over, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_over_texture() const { return over; }

int TextureProgressBar::get_stretch_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return stretch_margin[p_side];
}

bool TextureProgressBar::get_nine_patch_stretch() const { return nine_patch_stretch; }

Size2 TextureProgressBar::get_minimum_size() const
{
	if (nine_patch_stretch) {
		return Size2(stretch_margin[SIDE_LEFT] + stretch_margin[SIDE_RIGHT],
			stretch_margin[SIDE_TOP] + stretch_margin[SIDE_BOTTOM]);
	}

	Size2 size = Size2(1, 1);
	if (under.is_valid()) {
		size = size.max(under->get_size());
	}
	if (progress.is_valid()) {
		size = size.max(progress->get_size());
	}
	if (over.is_valid()) {
		size = size.max(over->get_size());
	}
	return size;
}

void TextureProgressBar::set_progress_texture(const Ref<Texture2D>& p_texture)
{
	_set_texture(&progress, p_texture);
}

Ref<Texture2D> TextureProgressBar::get_progress_texture() const { return progress; }

Point2 TextureProgressBar::get_progress_offset() const { return progress_offset; }

Color TextureProgressBar::get_tint_under() const { return tint_under; }

Color TextureProgressBar::get_tint_progress() const { return tint_progress; }

Color TextureProgressBar::get_tint_over() const { return tint_over; }

Point2 TextureProgressBar::unit_val_to_uv(float val)
{
	if (progress.is_null()) {
		return Point2();
	}

	if (val < 0) {
		val += 1;
	}
	if (val > 1) {
		val -= 1;
	}

	Point2 p = get_relative_center();

	// Minimal version of Liang-Barsky clipping algorithm
	float angle = (val * Math::TAU) - Math::PI * 0.5;
	Point2 dir = Vector2(Math::cos(angle), Math::sin(angle));
	float t1 = 1.0;
	float cp = 0.0;
	float cq = 0.0;
	float cr = 0.0;
	float edgeLeft = 0.0;
	float edgeRight = 1.0;
	float edgeBottom = 0.0;
	float edgeTop = 1.0;

	for (int edge = 0; edge < 4; edge++) {
		if (edge == 0) {
			if (dir.x > 0) {
				continue;
			}
			cq = -(edgeLeft - p.x);
			dir.x *= 2.0 * cq;
			cp = -dir.x;
		}
		else if (edge == 1) {
			if (dir.x < 0) {
				continue;
			}
			cq = (edgeRight - p.x);
			dir.x *= 2.0 * cq;
			cp = dir.x;
		}
		else if (edge == 2) {
			if (dir.y > 0) {
				continue;
			}
			cq = -(edgeBottom - p.y);
			dir.y *= 2.0 * cq;
			cp = -dir.y;
		}
		else if (edge == 3) {
			if (dir.y < 0) {
				continue;
			}
			cq = (edgeTop - p.y);
			dir.y *= 2.0 * cq;
			cp = dir.y;
		}
		cr = cq / cp;
		if (cr >= 0 && cr < t1) {
			t1 = cr;
		}
	}
	return (p + t1 * dir);
}

Point2 TextureProgressBar::get_relative_center()
{
	if (progress.is_null()) {
		return Point2();
	}
	Point2 p = progress->get_size() / 2;
	p += rad_center_off;
	p.x /= progress->get_width();
	p.y /= progress->get_height();
	p = p.clampf(0, 1);
	return p;
}

void TextureProgressBar::draw_nine_patch_stretched(
	const Ref<Texture2D>& p_texture, FillMode p_mode, double p_ratio, const Color& p_modulate)
{
	Vector2 texture_size = p_texture->get_size();
	Vector2 topleft = Vector2(stretch_margin[SIDE_LEFT], stretch_margin[SIDE_TOP]);
	Vector2 bottomright = Vector2(stretch_margin[SIDE_RIGHT], stretch_margin[SIDE_BOTTOM]);

	Rect2 src_rect = Rect2(Point2(), texture_size);
	Rect2 dst_rect = Rect2(Point2(), get_size());

	if (p_ratio < 1.0) {
		// Drawing a partially-filled 9-patch is a little tricky -
		// texture is divided by 3 sections toward fill direction,
		// then middle section is stretching while the other two aren't.

		double width_total = 0.0;
		double width_texture = 0.0;
		double first_section_size = 0.0;
		double last_section_size = 0.0;
		switch (p_mode) {
		case FILL_LEFT_TO_RIGHT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			first_section_size = topleft.x;
			last_section_size = bottomright.x;
		} break;
		case FILL_RIGHT_TO_LEFT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			// In contrast to `FILL_LEFT_TO_RIGHT`, `first_section_size` and `last_section_size`
			// should switch value.
			first_section_size = bottomright.x;
			last_section_size = topleft.x;
		} break;
		case FILL_TOP_TO_BOTTOM: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			first_section_size = topleft.y;
			last_section_size = bottomright.y;
		} break;
		case FILL_BOTTOM_TO_TOP: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			// Similar to `FILL_RIGHT_TO_LEFT`.
			first_section_size = bottomright.y;
			last_section_size = topleft.y;
		} break;
		case FILL_BILINEAR_LEFT_AND_RIGHT: {
			width_total = dst_rect.size.x;
			width_texture = texture_size.x;
			first_section_size = topleft.x;
			last_section_size = bottomright.x;
		} break;
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			width_total = dst_rect.size.y;
			width_texture = texture_size.y;
			first_section_size = topleft.y;
			last_section_size = bottomright.y;
		} break;
		case FILL_CLOCKWISE:
		case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE:
		case FILL_COUNTER_CLOCKWISE: {
			// Those modes are circular, not relevant for nine patch.
		} break;
		case FILL_MODE_MAX:
			break;
		}

		double width_filled = width_total * p_ratio;
		double middle_section_size =
			MAX(0.0, width_texture - first_section_size - last_section_size);

		// Maximum middle texture size.
		double max_middle_texture_size = middle_section_size;

		// Maximum real middle texture size.
		double max_middle_real_size =
			MAX(0.0, width_total - (first_section_size + last_section_size));

		switch (p_mode) {
		case FILL_BILINEAR_LEFT_AND_RIGHT:
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			last_section_size = MAX(0.0, last_section_size - (width_total - width_filled) * 0.5);
			first_section_size = MAX(0.0, first_section_size - (width_total - width_filled) * 0.5);

			// When `width_filled` increases, `middle_section_size` only increases when either of
			// `first_section_size` and `last_section_size` is zero. Also, it should always be
			// smaller than or equal to `(width_total - (first_section_size + last_section_size))`.
			double real_middle_size = width_filled - first_section_size - last_section_size;
			middle_section_size *=
				MIN(max_middle_real_size, real_middle_size) / max_middle_real_size;

			width_texture =
				MIN(width_texture, first_section_size + middle_section_size + last_section_size);
		} break;
		case FILL_MODE_MAX:
			break;
		default: {
			middle_section_size *=
				MIN(1.0, (MAX(0.0, width_filled - first_section_size) /
							 MAX(1.0, width_total - first_section_size - last_section_size)));
			last_section_size = MAX(0.0, last_section_size - (width_total - width_filled));
			first_section_size = MIN(first_section_size, width_filled);
			width_texture =
				MIN(width_texture, first_section_size + middle_section_size + last_section_size);
		}
		}

		switch (p_mode) {
		case FILL_LEFT_TO_RIGHT: {
			src_rect.size.x = width_texture;
			dst_rect.size.x = width_filled;
			topleft.x = first_section_size;
			bottomright.x = last_section_size;
		} break;
		case FILL_RIGHT_TO_LEFT: {
			src_rect.position.x += src_rect.size.x - width_texture;
			src_rect.size.x = width_texture;
			dst_rect.position.x += width_total - width_filled;
			dst_rect.size.x = width_filled;
			topleft.x = last_section_size;
			bottomright.x = first_section_size;
		} break;
		case FILL_TOP_TO_BOTTOM: {
			src_rect.size.y = width_texture;
			dst_rect.size.y = width_filled;
			bottomright.y = last_section_size;
			topleft.y = first_section_size;
		} break;
		case FILL_BOTTOM_TO_TOP: {
			src_rect.position.y += src_rect.size.y - width_texture;
			src_rect.size.y = width_texture;
			dst_rect.position.y += width_total - width_filled;
			dst_rect.size.y = width_filled;
			topleft.y = last_section_size;
			bottomright.y = first_section_size;
		} break;
		case FILL_BILINEAR_LEFT_AND_RIGHT: {
			double center_mapped_from_real_width =
				(width_total * 0.5 - topleft.x) / max_middle_real_size * max_middle_texture_size +
				topleft.x;
			double drift_from_unscaled_center = 0;
			if (bottomright.y != topleft.y) { // To avoid division by zero.
				drift_from_unscaled_center =
					(src_rect.size.x * 0.5 - center_mapped_from_real_width) *
					(last_section_size - first_section_size) / (bottomright.x - topleft.x);
			}

			src_rect.position.x +=
				center_mapped_from_real_width + drift_from_unscaled_center - width_texture * 0.5;
			src_rect.size.x = width_texture;
			dst_rect.position.x += (width_total - width_filled) * 0.5;
			dst_rect.size.x = width_filled;
			topleft.x = first_section_size;
			bottomright.x = last_section_size;
		} break;
		case FILL_BILINEAR_TOP_AND_BOTTOM: {
			double center_mapped_from_real_width =
				(width_total * 0.5 - topleft.y) / max_middle_real_size * max_middle_texture_size +
				topleft.y;
			double drift_from_unscaled_center = 0;
			if (bottomright.y != topleft.y) { // To avoid division by zero.
				drift_from_unscaled_center =
					(src_rect.size.y * 0.5 - center_mapped_from_real_width) *
					(last_section_size - first_section_size) / (bottomright.y - topleft.y);
			}

			src_rect.position.y +=
				center_mapped_from_real_width + drift_from_unscaled_center - width_texture * 0.5;
			src_rect.size.y = width_texture;
			dst_rect.position.y += (width_total - width_filled) * 0.5;
			dst_rect.size.y = width_filled;
			topleft.y = first_section_size;
			bottomright.y = last_section_size;
		} break;
		case FILL_CLOCKWISE:
		case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE:
		case FILL_COUNTER_CLOCKWISE: {
			// Those modes are circular, not relevant for nine patch.
		} break;
		case FILL_MODE_MAX:
			break;
		}
	}

	if (p_texture == progress) {
		dst_rect.position += progress_offset;
	}
	p_texture->get_rect_region(dst_rect, src_rect, dst_rect, src_rect);

	RID ci = get_canvas_item();
	Renderer::canvas_item_add_nine_patch(ci, dst_rect, src_rect,
		p_texture->get_scaled_rid(), topleft, bottomright, RSE::NINE_PATCH_STRETCH,
		RSE::NINE_PATCH_STRETCH, true, p_modulate);
}

void TextureProgressBar::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_PROGRESS_INDICATOR);
	} break;

	case NOTIFICATION_DRAW: {
		if (under.is_valid()) {
			if (nine_patch_stretch) {
				draw_nine_patch_stretched(under, mode, 1.0, tint_under);
			}
			else {
				draw_texture(under.ptr(), Point2(), tint_under);
			}
		}

		if (progress.is_valid()) {
			const bool is_radial_mode = (mode == FILL_CLOCKWISE || mode == FILL_COUNTER_CLOCKWISE ||
										 mode == FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE);

			if (nine_patch_stretch && !is_radial_mode) {
				draw_nine_patch_stretched(progress, mode, get_as_ratio(), tint_progress);
			}
			else {
				Size2 s = progress->get_size();
				switch (mode) {
				case FILL_LEFT_TO_RIGHT: {
					Rect2 region = Rect2(progress_offset, Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(Point2(), Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_RIGHT_TO_LEFT: {
					Rect2 region = Rect2(progress_offset + Point2(s.x - s.x * get_as_ratio(), 0),
						Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(
						Point2(s.x - s.x * get_as_ratio(), 0), Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_TOP_TO_BOTTOM: {
					Rect2 region =
						Rect2(progress_offset + Point2(), Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(Point2(), Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_BOTTOM_TO_TOP: {
					Rect2 region = Rect2(progress_offset + Point2(0, s.y - s.y * get_as_ratio()),
						Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(
						Point2(0, s.y - s.y * get_as_ratio()), Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_CLOCKWISE:
				case FILL_COUNTER_CLOCKWISE:
				case FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE: {
					if (nine_patch_stretch) {
						s = get_size();
					}

					float val = get_as_ratio() * rad_max_degrees / 360;
					if (val == 1) {
						Rect2 region = Rect2(progress_offset, s);
						Rect2 source = Rect2(Point2(), progress->get_size());
						draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
					}
					else if (val != 0) {
						LocalVector<float> pts;
						float direction = mode == FILL_COUNTER_CLOCKWISE ? -1 : 1;
						float start;

						if (mode == FILL_CLOCKWISE_AND_COUNTER_CLOCKWISE) {
							start = rad_init_angle / 360 - val / 2;
						}
						else {
							start = rad_init_angle / 360;
						}

						float end = start + direction * val;
						float from = MIN(start, end);
						float to = MAX(start, end);
						pts.push_back(from);
						for (float corner = Math::floor(from * 4 + 0.5) * 0.25 + 0.125; corner < to;
							 corner += 0.25) {
							pts.push_back(corner);
						}
						pts.push_back(to);

						Vector<Point2> uvs;
						Vector<Point2> points;
						for (const float& f : pts) {
							Point2 uv = unit_val_to_uv(f);
							if (uvs.has(uv)) {
								continue;
							}
							points.push_back(progress_offset + Point2(uv.x * s.x, uv.y * s.y));
							uvs.push_back(uv);
						}

						// Filter out an edge case where almost equal `from`, `to` were mapped to
						// the same UV.
						if (points.size() >= 2) {
							Point2 center_point = get_relative_center();
							points.push_back(progress_offset + s * center_point);
							uvs.push_back(center_point);

							Vector<Color> colors;
							colors.push_back(tint_progress);
							draw_polygon(points, colors, uvs, progress);
						}
					}
				} break;
				case FILL_BILINEAR_LEFT_AND_RIGHT: {
					Rect2 region =
						Rect2(progress_offset + Point2(s.x / 2 - s.x * get_as_ratio() / 2, 0),
							Size2(s.x * get_as_ratio(), s.y));
					Rect2 source = Rect2(Point2(s.x / 2 - s.x * get_as_ratio() / 2, 0),
						Size2(s.x * get_as_ratio(), s.y));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_BILINEAR_TOP_AND_BOTTOM: {
					Rect2 region =
						Rect2(progress_offset + Point2(0, s.y / 2 - s.y * get_as_ratio() / 2),
							Size2(s.x, s.y * get_as_ratio()));
					Rect2 source = Rect2(Point2(0, s.y / 2 - s.y * get_as_ratio() / 2),
						Size2(s.x, s.y * get_as_ratio()));
					draw_texture_rect_region(progress.ptr(), region, source, tint_progress);
				} break;
				case FILL_MODE_MAX:
					break;
				}
			}
#ifdef TOOLS_ENABLED
			// Draw a reference cross for radial modes.
			if (is_radial_mode && is_part_of_edited_scene()) {
				Point2 p;

				if (nine_patch_stretch) {
					p = get_size();
				}
				else {
					p = progress->get_size();
				}

				p *= get_relative_center();
				p += progress_offset;
				draw_line(p - Point2(8, 0), p + Point2(8, 0), Color(0.9, 0.5, 0.5), 2);
				draw_line(p - Point2(0, 8), p + Point2(0, 8), Color(0.9, 0.5, 0.5), 2);
			}
#endif
		}

		if (over.is_valid()) {
			if (nine_patch_stretch) {
				draw_nine_patch_stretched(over, mode, 1.0, tint_over);
			}
			else {
				draw_texture(over.ptr(), Point2(), tint_over);
			}
		}
	} break;
	}
}

int TextureProgressBar::get_fill_mode() { return mode; }

float TextureProgressBar::get_radial_initial_angle() { return rad_init_angle; }

float TextureProgressBar::get_fill_degrees() { return rad_max_degrees; }

Point2 TextureProgressBar::get_radial_center_offset() { return rad_center_off; }

TextureProgressBar::TextureProgressBar() { set_mouse_filter(MOUSE_FILTER_PASS); }



void TextureProgressBar::_set_texture(Ref<Texture2D>*, Ref<Texture2D> const&) {}

void TextureProgressBar::set_tint_under(Color const&) {}

void TextureProgressBar::set_fill_mode(int) {}

// --- slider.cpp ---

Size2 Slider::get_minimum_size() const
{
	Size2i ss = theme_cache.slider_style->get_minimum_size();
	Size2i rs = theme_cache.grabber_icon->get_size();

	if (orientation == HORIZONTAL) {
		return Size2i(ss.width, MAX(ss.height, rs.height));
	}
	else {
		return Size2i(MAX(ss.width, rs.width), ss.height);
	}
}

void Slider::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (Input::is_action_just_released("ui_left") ||
			Input::is_action_just_released("ui_right") || Input::is_action_just_released("ui_up") ||
			Input::is_action_just_released("ui_down")) {
			gamepad_event_delay_ms = DEFAULT_GAMEPAD_EVENT_DELAY_MS;
			set_process_internal(false);
			return;
		}

		gamepad_event_delay_ms -= get_process_delta_time();
		if (gamepad_event_delay_ms <= 0) {
			gamepad_event_delay_ms = GAMEPAD_EVENT_REPEAT_RATE_MS + gamepad_event_delay_ms;
			if (orientation == HORIZONTAL) {
				if (Input::is_action_pressed("ui_left")) {
					if (is_layout_rtl()) {
						set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
					}
					else {
						set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
					}
				}

				if (Input::is_action_pressed("ui_right")) {
					if (is_layout_rtl()) {
						set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
					}
					else {
						set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
					}
				}
			}
			else if (orientation == VERTICAL) {
				if (Input::is_action_pressed("ui_down")) {
					set_value(get_value() - (custom_step >= 0 ? custom_step : get_step()));
				}

				if (Input::is_action_pressed("ui_up")) {
					set_value(get_value() + (custom_step >= 0 ? custom_step : get_step()));
				}
			}
		}
	} break;

	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_SLIDER);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_EXIT_TREE: {
		mouse_inside = false;
		grab.active = false;
	} break;

	case NOTIFICATION_DRAW: {
		RID ci = get_canvas_item();
		Size2i size = get_size();
		double ratio = Math::is_nan(get_as_ratio()) ? 0 : get_as_ratio();

		Ref<StyleBox> style = theme_cache.slider_style;
		Ref<Texture2D> tick = theme_cache.tick_icon;

		bool highlighted = editable && (mouse_inside || has_focus(true));
		Ref<Texture2D> grabber;
		if (editable) {
			if (highlighted) {
				grabber = theme_cache.grabber_hl_icon;
			}
			else {
				grabber = theme_cache.grabber_icon;
			}
		}
		else {
			grabber = theme_cache.grabber_disabled_icon;
		}

		Ref<StyleBox> grabber_area;
		if (highlighted) {
			grabber_area = theme_cache.grabber_area_hl_style;
		}
		else {
			grabber_area = theme_cache.grabber_area_style;
		}

		if (orientation == VERTICAL) {
			int widget_width = style->get_minimum_size().width;
			double areasize =
				size.height - (theme_cache.center_grabber ? 0 : grabber->get_height());
			int grabber_shift = theme_cache.center_grabber ? grabber->get_height() / 2 : 0;
			style->draw(ci, Rect2i(Point2i(size.width / 2 - widget_width / 2, 0),
								Size2i(widget_width, size.height)));
			grabber_area->draw(ci,
				Rect2i(Point2i((size.width - widget_width) / 2,
						   Math::round(size.height - areasize * ratio - grabber->get_height() / 2 +
									   grabber_shift)),
					Size2i(widget_width, Math::round(areasize * ratio + grabber->get_height() / 2 -
													 grabber_shift))));

			if (ticks > 1) {
				int grabber_offset = (grabber->get_height() / 2 - tick->get_height() / 2);
				for (int i = 0; i < ticks; i++) {
					if (!ticks_on_borders && (i == 0 || i + 1 == ticks)) {
						continue;
					}
					int ofs = (i * areasize / (ticks - 1)) + grabber_offset - grabber_shift;

					if (ticks_position == TICK_POSITION_BOTTOM_RIGHT ||
						ticks_position == TICK_POSITION_BOTH) {
						tick->draw(ci, Point2i(widget_width + (size.width - widget_width) / 2 +
												   theme_cache.tick_offset,
										   ofs));
					}

					if (ticks_position == TICK_POSITION_TOP_LEFT ||
						ticks_position == TICK_POSITION_BOTH) {
						Point2i pos = Point2i((size.width - widget_width) / 2 - tick->get_width() -
												  theme_cache.tick_offset,
							ofs);
						tick->draw_rect(
							ci, Rect2i(pos, Size2i(-tick->get_width(), tick->get_height())));
					}

					if (ticks_position == TICK_POSITION_CENTER) {
						tick->draw(ci,
							Point2i((size.width - tick->get_width()) / 2 + theme_cache.tick_offset,
								ofs));
					}
				}
			}
			grabber->draw(
				ci, Point2i(size.width / 2 - grabber->get_width() / 2 + theme_cache.grabber_offset,
						size.height - ratio * areasize - grabber->get_height() + grabber_shift));
		}
		else {
			int widget_height = style->get_minimum_size().height;
			double areasize =
				size.width - (theme_cache.center_grabber ? 0 : grabber->get_size().width);
			int grabber_shift = theme_cache.center_grabber ? -grabber->get_width() / 2 : 0;
			bool rtl = is_layout_rtl();

			style->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2),
								Size2i(size.width, widget_height)));
			int p = areasize * (rtl ? 1 - ratio : ratio) + grabber->get_width() / 2 + grabber_shift;
			if (rtl) {
				grabber_area->draw(ci, Rect2i(Point2i(p, (size.height - widget_height) / 2),
										   Size2i(size.width - p, widget_height)));
			}
			else {
				grabber_area->draw(ci, Rect2i(Point2i(0, (size.height - widget_height) / 2),
										   Size2i(p, widget_height)));
			}

			if (ticks > 1) {
				int grabber_offset = (grabber->get_width() / 2 - tick->get_width() / 2);
				for (int i = 0; i < ticks; i++) {
					if ((!ticks_on_borders) && ((i == 0) || ((i + 1) == ticks))) {
						continue;
					}
					int ofs = (i * areasize / (ticks - 1)) + grabber_offset + grabber_shift;

					if (ticks_position == TICK_POSITION_BOTTOM_RIGHT ||
						ticks_position == TICK_POSITION_BOTH) {
						tick->draw(
							ci, Point2i(ofs, widget_height + (size.height - widget_height) / 2 +
												 theme_cache.tick_offset));
					}

					if (ticks_position == TICK_POSITION_TOP_LEFT ||
						ticks_position == TICK_POSITION_BOTH) {
						Point2i pos =
							Point2i(ofs, (size.height - widget_height) / 2 - tick->get_height() -
											 theme_cache.tick_offset);
						tick->draw_rect(
							ci, Rect2i(pos, Size2i(tick->get_width(), -tick->get_height())));
					}

					if (ticks_position == TICK_POSITION_CENTER) {
						tick->draw(ci, Point2i(ofs, (size.height - tick->get_height()) / 2 +
														theme_cache.tick_offset));
					}
				}
			}
			grabber->draw(
				ci, Point2i((rtl ? 1 - ratio : ratio) * areasize + grabber_shift,
						size.height / 2 - grabber->get_height() / 2 + theme_cache.grabber_offset));
		}
	} break;
	}
}

void Slider::set_custom_step(double p_custom_step) { custom_step = p_custom_step; }

double Slider::get_custom_step() const { return custom_step; }

int Slider::get_ticks() const { return ticks; }

bool Slider::get_ticks_on_borders() const { return ticks_on_borders; }

Slider::TickPosition Slider::get_ticks_position() const { return ticks_position; }

bool Slider::is_editable() const { return editable; }

void Slider::set_scrollable(bool p_scrollable) { scrollable = p_scrollable; }

bool Slider::is_scrollable() const { return scrollable; }

Slider::Slider(Orientation p_orientation)
{
	orientation = p_orientation;
	if (orientation == HORIZONTAL) {
		set_v_size_flags(0);
	}
	else {
		set_h_size_flags(0);
	}
	set_focus_mode(FOCUS_ALL);
}

// --- spin_box.cpp ---

Size2 SpinBox::get_minimum_size() const
{
	Size2 ms = line_edit->get_bound_minimum_size();
	ms.width += sizing_cache.buttons_block_width;
	return ms;
}

LineEdit* SpinBox::get_line_edit() { return line_edit; }

void SpinBox::_line_edit_input(const InputEvent& p_event)
{
	if (drag.enabled) {
		line_edit->accept_event();
	}
}

void SpinBox::_release_mouse_from_drag_mode()
{
	if (drag.enabled) {
		drag.enabled = false;
		Input::set_mouse_mode(Input::MouseMode::MOUSE_MODE_HIDDEN);
		warp_mouse(drag.capture_pos);
		Input::set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
	}
}

void SpinBox::_arrow_clicked(bool p_up)
{
	double arrow_step = get_custom_arrow_step() != 0.0 ? get_custom_arrow_step() : get_step();
	if (custom_arrow_round) {
		// Arrow button is being pressed, snap the value to next `arrow_step`.
		// `arrow_step` should be a multiple of `step`, otherwise it may not be able to
		// increase/decrease the value.
		arrow_step = Math::snapped(arrow_step, get_step());
		double new_value = range.calc_value(get_value(), arrow_step);
		if ((p_up && new_value <= get_value()) || (!p_up && new_value >= get_value())) {
			new_value = range.calc_value(get_value() + (p_up ? arrow_step : -arrow_step), arrow_step);
		}
		set_value(new_value);
	}
	else {
		set_value(get_value() + (p_up ? arrow_step : -arrow_step));
	}
}

void SpinBox::_line_edit_editing_toggled(bool p_toggled_on)
{
	if (p_toggled_on) {
		int col = line_edit->get_caret_column();
		_update_text();
		line_edit->set_caret_column(col);

		// LineEdit text might change and it clears any selection. Have to re-select here.
		if (line_edit->is_select_all_on_focus() &&
			!Input::is_mouse_button_pressed(MouseButton::LEFT)) {
			line_edit->select_all();
		}
	}
	else {
		accepted = true;

		if (Input::is_action_pressed("ui_cancel") ||
			line_edit->get_text().is_empty()) {
			_update_text(); // Revert text if editing was canceled.
		}
		else {
			line_edit->set_text(line_edit->get_text().trim_suffix(".").trim_suffix(","));
			_update_text(
				true); // Update text in case value was changed this frame (e.g. on `focus_exited`).
			_text_submitted(line_edit->get_text());
		}
	}
}

inline void SpinBox::_compute_sizes()
{
	int buttons_block_wanted_width =
		theme_cache.buttons_width + theme_cache.field_and_buttons_separation;
	int buttons_block_icon_enforced_width =
		_get_widest_button_icon_width() + theme_cache.field_and_buttons_separation;

#ifndef DISABLE_DEPRECATED
	const bool min_width_from_icons =
		theme_cache.set_min_buttons_width_from_icons || (theme_cache.buttons_width < 0);
#else
	const bool min_width_from_icons = theme_cache.buttons_width < 0;
#endif
	int w = min_width_from_icons != 0
				? MAX(buttons_block_icon_enforced_width, buttons_block_wanted_width)
				: buttons_block_wanted_width;

	if (w != sizing_cache.buttons_block_width) {
		line_edit->set_offset(SIDE_LEFT, 0);
		line_edit->set_offset(SIDE_RIGHT, -w);
		sizing_cache.buttons_block_width = w;
	}

	Size2i size = get_size();

	sizing_cache.buttons_width = w - theme_cache.field_and_buttons_separation;
	sizing_cache.buttons_vertical_separation =
		CLAMP(theme_cache.buttons_vertical_separation, 0, size.height);
	sizing_cache.buttons_left = is_layout_rtl() ? 0 : size.width - sizing_cache.buttons_width;
	sizing_cache.button_up_height = (size.height - sizing_cache.buttons_vertical_separation) / 2;
	sizing_cache.button_down_height =
		size.height - sizing_cache.button_up_height - sizing_cache.buttons_vertical_separation;
	sizing_cache.second_button_top = size.height - sizing_cache.button_down_height;

	sizing_cache.buttons_separator_top = sizing_cache.button_up_height;
	sizing_cache.field_and_buttons_separator_left =
		is_layout_rtl() ? sizing_cache.buttons_width
						: size.width - sizing_cache.buttons_block_width;
	sizing_cache.field_and_buttons_separator_width = theme_cache.field_and_buttons_separation;
}

inline int SpinBox::_get_widest_button_icon_width()
{
	int max = 0;
#ifndef DISABLE_DEPRECATED
	max = MAX(max, theme_cache.updown_icon->get_width());
#endif
	max = MAX(max, theme_cache.up_icon->get_width());
	max = MAX(max, theme_cache.up_hover_icon->get_width());
	max = MAX(max, theme_cache.up_pressed_icon->get_width());
	max = MAX(max, theme_cache.up_disabled_icon->get_width());
	max = MAX(max, theme_cache.down_icon->get_width());
	max = MAX(max, theme_cache.down_hover_icon->get_width());
	max = MAX(max, theme_cache.down_pressed_icon->get_width());
	max = MAX(max, theme_cache.down_disabled_icon->get_width());
	return max;
}

void SpinBox::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	line_edit->set_horizontal_alignment(p_alignment);
}

HorizontalAlignment SpinBox::get_horizontal_alignment() const
{
	return line_edit->get_horizontal_alignment();
}

void SpinBox::set_suffix(const String& p_suffix)
{
	if (suffix == p_suffix) {
		return;
	}

	suffix = p_suffix;
	_update_text();
}

String SpinBox::get_suffix() const { return suffix; }

void SpinBox::set_prefix(const String& p_prefix)
{
	if (prefix == p_prefix) {
		return;
	}

	prefix = p_prefix;
	_update_text();
}

String SpinBox::get_prefix() const { return prefix; }

bool SpinBox::get_update_on_text_changed() const { return update_on_text_changed; }

void SpinBox::set_select_all_on_focus(bool p_enabled)
{
	line_edit->set_select_all_on_focus(p_enabled);
}

bool SpinBox::is_select_all_on_focus() const { return line_edit->is_select_all_on_focus(); }

bool SpinBox::is_editable() const { return line_edit->is_editable(); }

void SpinBox::apply() { _text_submitted(line_edit->get_text()); }

void SpinBox::set_custom_arrow_step(double p_custom_arrow_step)
{
	custom_arrow_step = p_custom_arrow_step;
}

double SpinBox::get_custom_arrow_step() const { return custom_arrow_step; }

void SpinBox::set_custom_arrow_round(bool p_round) { custom_arrow_round = p_round; }

bool SpinBox::is_custom_arrow_rounding() const { return custom_arrow_round; }

void SpinBox::_value_changed(double p_value)
{
	_update_buttons_state_for_current_value();
}

void SpinBox::_update_text(bool p_only_update_if_value_changed) {}



void SpinBox::_text_submitted(String const&) {}

void SpinBox::set_editable(bool) {}

void SpinBox::_update_buttons_state_for_current_value() {}

// --- scroll_bar.cpp ---

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

void ScrollBar::scroll_to(double p_position) {}

void ScrollBar::set_custom_step(float p_custom_step) { custom_step = p_custom_step; }

float ScrollBar::get_custom_step() const { return custom_step; }

void ScrollBar::_drag_node_input(const InputEvent& p_input)
{
	if (!drag_node_enabled) {
		return;
	}

	if (p_input.type == InputEventType::MOUSE_BUTTON) {
		if (p_input.mouse_button.button_index != MouseButton::LEFT) {
			return;
		}

		if (p_input.is_pressed()) {
			drag_node_speed = Vector2();
			drag_node_accum = Vector2();
			last_drag_node_accum = Vector2();
			drag_node_from = Vector2(orientation == HORIZONTAL ? get_value() : 0,
				orientation == VERTICAL ? get_value() : 0);
			drag_node_touching = DisplayServer::is_touchscreen_available();
			drag_node_touching_deaccel = false;
			time_since_motion = 0;

			if (drag_node_touching) {
				set_process_internal(true);
				time_since_motion = 0;
			}
		}
		else if (drag_node_touching) {
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

	if (p_input.type == InputEventType::MOUSE_MOTION) {
		if (drag_node_touching && !drag_node_touching_deaccel) {
			Vector2 motion = p_input.mouse_motion.relative;

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
	if (orientation == HORIZONTAL) {
		set_v_size_flags(0);
	}
	else {
		set_h_size_flags(0);
	}

	if (focus_by_default) {
		set_focus_mode(FOCUS_ALL);
	}
	else {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
	set_step(0);
}

ScrollBar::~ScrollBar() {}

// ============================================================================
// Merged from scene/gui/scroll_container.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "scene/gui/panel.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "scroll_container.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/main/viewport.h"
#include "scene/gui/scroll_container.h"
#include <cfloat>
#include "scene/gui/label.h"
#include "scene/main/timer.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/button.h"
#include "scene/gui/popup.h"
#include "scene/gui/scroll_container.h"
#include "scene/resources/text_line.h"
#include "core/types.h"
#include "scene/gui/scroll_container.h"

Size2 ScrollContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 ScrollContainer::get_desired_size() const { return _get_minimum_size(true); }

Size2 ScrollContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Control::get_inner_combined_maximum_size();

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	if (h_scroll && (_is_h_scroll_visible() || horizontal_scroll_mode == SCROLL_MODE_RESERVE)) {
		ms.y -= h_scroll->get_minimum_size().y + theme_cache.scrollbar_v_separation;
	}
	if (v_scroll && (_is_v_scroll_visible() || vertical_scroll_mode == SCROLL_MODE_RESERVE)) {
		ms.x -= v_scroll->get_minimum_size().x + theme_cache.scrollbar_h_separation;
	}

	return ms;
}

bool ScrollContainer::_is_h_scroll_visible() const
{
	// Scrolls may have been moved out for reasons.
	return h_scroll && h_scroll->is_visible() && h_scroll->get_parent() == this;
}

bool ScrollContainer::_is_v_scroll_visible() const
{
	return v_scroll && v_scroll->is_visible() && v_scroll->get_parent() == this;
}

Rect2 ScrollContainer::_get_margins() const
{
	float right_margin = theme_cache.panel_style->get_margin(SIDE_RIGHT);
	float left_margin = theme_cache.panel_style->get_margin(SIDE_LEFT);
	float top_margin = theme_cache.panel_style->get_margin(SIDE_TOP);
	float bottom_margin = theme_cache.panel_style->get_margin(SIDE_BOTTOM);
	if (draw_focus_border) {
		// Only update margins if the focus style's margins don't fit into the panel style's
		// margins.
		float focus_margin = theme_cache.focus_style->get_margin(SIDE_RIGHT);
		if (focus_margin > right_margin) {
			right_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_LEFT);
		if (focus_margin > left_margin) {
			left_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_TOP);
		if (focus_margin > top_margin) {
			top_margin = focus_margin;
		}
		focus_margin = theme_cache.focus_style->get_margin(SIDE_BOTTOM);
		if (focus_margin > bottom_margin) {
			bottom_margin = focus_margin;
		}
	}

	return Rect2(left_margin, top_margin, right_margin, bottom_margin);
}

Rect2 ScrollContainer::_get_local_visible_rect() const
{
	const float side_margin = v_scroll->is_visible() ? v_scroll->get_size().x : 0.0f;
	const float bottom_margin = h_scroll->is_visible() ? h_scroll->get_size().y : 0.0f;

	Point2 origin = Point2(is_layout_rtl() ? side_margin : 0.0f, 0.0f);
	Size2 size = Size2(get_size().x - side_margin, get_size().y - bottom_margin);

	return Rect2(origin, size);
}

void ScrollContainer::_update_scrollbar_position()
{
	if (!_updating_scrollbars) {
		return;
	}

	Rect2 margins = _get_margins();

	Size2 hmin = h_scroll->is_visible() ? h_scroll->get_bound_minimum_size() : Size2();
	Size2 vmin = v_scroll->is_visible() ? v_scroll->get_bound_minimum_size() : Size2();

	int lmar = is_layout_rtl() ? margins.size.x : margins.position.x;
	int rmar = is_layout_rtl() ? margins.position.x : margins.size.x;

	h_scroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_BEGIN, lmar);
	h_scroll->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -rmar - vmin.width);
	h_scroll->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -hmin.height - margins.size.y);
	h_scroll->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -margins.size.y);

	v_scroll->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -vmin.width - rmar);
	v_scroll->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -rmar);
	v_scroll->set_anchor_and_offset(SIDE_TOP, ANCHOR_BEGIN, margins.position.y);
	v_scroll->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -hmin.height - margins.size.y);

	_updating_scrollbars = false;
}

void ScrollContainer::set_h_scroll(int p_pos)
{
	h_scroll->set_value(p_pos);
	_cancel_drag();
}

int ScrollContainer::get_h_scroll() const { return h_scroll->get_value(); }

void ScrollContainer::set_v_scroll(int p_pos)
{
	v_scroll->set_value(p_pos);
	_cancel_drag();
}

int ScrollContainer::get_v_scroll() const { return v_scroll->get_value(); }

void ScrollContainer::set_horizontal_custom_step(float p_custom_step)
{
	h_scroll->set_custom_step(p_custom_step);
}

float ScrollContainer::get_horizontal_custom_step() const { return h_scroll->get_custom_step(); }

void ScrollContainer::set_vertical_custom_step(float p_custom_step)
{
	v_scroll->set_custom_step(p_custom_step);
}

float ScrollContainer::get_vertical_custom_step() const { return v_scroll->get_custom_step(); }

ScrollContainer::ScrollMode ScrollContainer::get_horizontal_scroll_mode() const
{
	return horizontal_scroll_mode;
}

ScrollContainer::ScrollMode ScrollContainer::get_vertical_scroll_mode() const
{
	return vertical_scroll_mode;
}

void ScrollContainer::set_scroll_horizontal_by_default(bool p_enable)
{
	scroll_horizontal_by_default = p_enable;
}

bool ScrollContainer::is_scroll_horizontal_by_default() const
{
	return scroll_horizontal_by_default;
}

int ScrollContainer::get_deadzone() const { return deadzone; }

void ScrollContainer::set_deadzone(int p_deadzone) { deadzone = p_deadzone; }

ScrollContainer::ScrollHintMode ScrollContainer::get_scroll_hint_mode() const
{
	return scroll_hint_mode;
}

void ScrollContainer::set_tile_scroll_hint(bool p_enable)
{
	if (tile_scroll_hint == p_enable) {
		return;
	}

	scroll_hint_top_left->set_stretch_mode(
		p_enable ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE);
	scroll_hint_bottom_right->set_stretch_mode(
		p_enable ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE);

	tile_scroll_hint = p_enable;
}

bool ScrollContainer::is_scroll_hint_tiled() { return tile_scroll_hint; }

bool ScrollContainer::is_following_focus() const { return follow_focus; }

void ScrollContainer::set_follow_focus(bool p_follow) { follow_focus = p_follow; }

void ScrollContainer::set_scroll_on_drag_hover(bool p_scroll) { scroll_on_drag_hover = p_scroll; }

ScrollBar* ScrollContainer::get_h_scroll_bar() { return h_scroll; }

ScrollBar* ScrollContainer::get_v_scroll_bar() { return v_scroll; }

PackedStringArray ScrollContainer::get_configuration_warnings() const { return PackedStringArray(); }

void ScrollContainer::set_draw_focus_border(bool p_draw)
{
	if (draw_focus_border == p_draw) {
		return;
	}
	draw_focus_border = p_draw;
	if (is_ready()) {
		_reposition_children();
	}
}

bool ScrollContainer::get_draw_focus_border() { return draw_focus_border; }

bool ScrollContainer::child_has_focus()
{
	const Control* focus_owner = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
	return focus_owner && focus_owner->has_focus(true) && is_ancestor_of(focus_owner);
}



Size2 ScrollContainer::_get_minimum_size(bool) const { return Size2(); }

void ScrollContainer::_cancel_drag() {}

void ScrollContainer::_reposition_children() {}

void ScrollContainer::ensure_control_visible(Control*) {}

// --- split_container.cpp ---

void SplitContainerMultiDragger::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_MOUSE_ENTER: {
		if (!dragging && !Input::is_mouse_button_pressed(MouseButton::LEFT)) {
			split_container->show_grabber_icon(dragger_index);
		}
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		if (!dragging && !Input::is_mouse_button_pressed(MouseButton::LEFT)) {
			split_container->show_grabber_icon(-1);
		}
	} break;
	}
}

SplitContainerMultiDragger::SplitContainerMultiDragger() { set_mouse_filter(MOUSE_FILTER_PASS); }

String SplitContainerDragger::_get_accessibility_name() const { return RTR("Drag to resize"); }

SplitContainerDragger::SplitContainerDragger() { set_focus_mode(FOCUS_ACCESSIBILITY); }

Ref<Texture2D> SplitContainer::_get_grabber_icon() const
{
	if (vertical) {
		return theme_cache.grabber_icon_v.is_valid() ? theme_cache.grabber_icon_v
													 : theme_cache.grabber_icon;
	}
	else {
		return theme_cache.grabber_icon_h.is_valid() ? theme_cache.grabber_icon_h
													 : theme_cache.grabber_icon;
	}
}

Ref<Texture2D> SplitContainer::_get_touch_dragger_icon() const
{
	if (vertical) {
		return theme_cache.touch_dragger_icon_v.is_valid() ? theme_cache.touch_dragger_icon_v
														   : theme_cache.touch_dragger_icon;
	}
	else {
		return theme_cache.touch_dragger_icon_h.is_valid() ? theme_cache.touch_dragger_icon_h
														   : theme_cache.touch_dragger_icon;
	}
}

int SplitContainer::_get_separation() const
{
	if (dragger_visibility == DRAGGER_HIDDEN_COLLAPSED) {
		return 0;
	}

	if (touch_dragger_enabled) {
		return theme_cache.separation;
	}
	// DRAGGER_VISIBLE or DRAGGER_HIDDEN.
	Ref<Texture2D> g = _get_grabber_icon();
	return MAX(theme_cache.separation, vertical ? g->get_height() : g->get_width());
}

Point2i SplitContainer::_get_valid_range(int p_dragger_index) const
{
	ERR_FAIL_INDEX_V(p_dragger_index, (int)dragger_positions.size(), Point2i());
	const int axis = vertical ? 1 : 0;
	const int sep = _get_separation();

	// Sum the minimum sizes on the left and right sides of the dragger.
	Point2i position_range = Point2i(0, (int)get_size()[axis]);
	position_range.x += sep * p_dragger_index;
	position_range.y -= sep * ((int)dragger_positions.size() - p_dragger_index);

	int max_left = sep * p_dragger_index;
	int max_right = sep * ((int)dragger_positions.size() - p_dragger_index);
	bool has_left_max = true;
	bool has_right_max = true;

	for (int i = 0; i < (int)valid_children.size(); i++) {
		Control* child = valid_children[i];
		ERR_FAIL_NULL_V(child, Point2i());
		int max_size = (int)child->get_combined_maximum_size()[axis];
		if (i <= p_dragger_index) {
			position_range.x += (int)child->get_bound_minimum_size()[axis];
			if (has_left_max) {
				if (max_size >= 0) {
					max_left += max_size;
				}
				else {
					has_left_max = false;
				}
			}
		}
		else if (i > p_dragger_index) {
			position_range.y -= (int)child->get_bound_minimum_size()[axis];
			if (has_right_max) {
				if (max_size >= 0) {
					max_right += max_size;
				}
				else {
					has_right_max = false;
				}
			}
		}
	}

	if (has_left_max) {
		position_range.y = MIN(position_range.y, max_left);
	}
	if (has_right_max) {
		position_range.x = MAX(position_range.x, (int)get_size()[axis] - max_right);
	}

	return position_range;
}

PackedInt32Array SplitContainer::_get_desired_sizes() const
{
	ERR_FAIL_COND_V((int)default_dragger_positions.size() != split_offsets.size() ||
						(int)valid_children.size() - 1 != split_offsets.size(),
		PackedInt32Array());
	PackedInt32Array desired_sizes;
	desired_sizes.resize_uninitialized((int)valid_children.size());

	const int sep = _get_separation();
	const int axis = vertical ? 1 : 0;

	int desired_start_pos = 0;
	for (int i = 0; i < (int)valid_children.size() - 1; i++) {
		const int desired_end_pos = default_dragger_positions[i] + split_offsets[i];
		desired_sizes.write[i] = desired_end_pos - desired_start_pos;
		desired_start_pos = desired_end_pos + sep;
	}
	desired_sizes.write[(int)valid_children.size() - 1] = (int)get_size()[axis] - desired_start_pos;

	return desired_sizes;
}

void SplitContainer::_update_dragger_positions(int p_clamp_index)
{
	if (p_clamp_index != -1) {
		ERR_FAIL_INDEX(p_clamp_index, (int)dragger_positions.size());
	}

	const int sep = _get_separation();
	const int axis = vertical ? 1 : 0;
	const int size = (int)get_size()[axis];

	dragger_positions.resize(default_dragger_positions.size());

	if (split_offsets.size() < (int)default_dragger_positions.size() || split_offsets.is_empty()) {
		split_offsets.resize_initialized(MAX(1, (int)default_dragger_positions.size()));
	}

	if (collapsed) {
		for (int i = 0; i < (int)dragger_positions.size(); i++) {
			dragger_positions[i] = default_dragger_positions[i];
			const Point2i valid_range = _get_valid_range(i);
			dragger_positions[i] = CLAMP(dragger_positions[i], valid_range.x, valid_range.y);
			if (p_clamp_index != -1) {
				split_offsets.write[i] = dragger_positions[i] - default_dragger_positions[i];
			}
			if (!vertical && is_layout_rtl()) {
				dragger_positions[i] = size - dragger_positions[i] - sep;
			}
		}
		return;
	}

	// Use split_offsets to find the desired dragger positions.
	for (int i = 0; i < (int)dragger_positions.size(); i++) {
		// Clamp the desired position to acceptable values.
		const Point2i valid_range = _get_valid_range(i);
		dragger_positions[i] =
			CLAMP(default_dragger_positions[i] + split_offsets[i], valid_range.x, valid_range.y);
	}

	// Prevent overlaps.
	if (p_clamp_index == -1) {
		// Check each dragger with the one to the right of it.
		for (int i = 0; i < (int)dragger_positions.size() - 1; i++) {
			const int check_min_size = (int)valid_children[i + 1]->get_bound_minimum_size()[axis];
			const int push_pos = dragger_positions[i] + sep + check_min_size;
			if (dragger_positions[i + 1] < push_pos) {
				dragger_positions[i + 1] = push_pos;
				const Point2i valid_range = _get_valid_range(i);
				dragger_positions[i] = CLAMP(dragger_positions[i], valid_range.x, valid_range.y);
			}
		}
	}
	else {
		// Prioritize the active dragger.
		const int dragging_position = dragger_positions[p_clamp_index];

		// Propagate constraints to the left.
		for (int i = p_clamp_index - 1; i >= 0; i--) {
			const int right_dragger_position =
				i == p_clamp_index - 1 ? dragging_position : dragger_positions[i + 1];
			const int min_size = (int)valid_children[i + 1]->get_bound_minimum_size()[axis];
			const int max_position = right_dragger_position - sep - min_size;
			if (dragger_positions[i] > max_position) {
				dragger_positions[i] = max_position;
			}

			const int max_size = (int)valid_children[i + 1]->get_combined_maximum_size()[axis];
			if (max_size >= 0) {
				const int min_position = right_dragger_position - sep - max_size;
				if (dragger_positions[i] < min_position) {
					dragger_positions[i] = min_position;
				}
			}
		}

		// Propagate constraints to the right.
		for (int i = p_clamp_index + 1; i < (int)dragger_positions.size(); i++) {
			const int left_dragger_position =
				i == p_clamp_index + 1 ? dragging_position : dragger_positions[i - 1];
			const int min_size = (int)valid_children[i]->get_bound_minimum_size()[axis];
			const int min_position = left_dragger_position + sep + min_size;
			if (dragger_positions[i] < min_position) {
				dragger_positions[i] = min_position;
			}

			const int max_size = (int)valid_children[i]->get_combined_maximum_size()[axis];
			if (max_size >= 0) {
				const int max_position = left_dragger_position + sep + max_size;
				if (dragger_positions[i] > max_position) {
					dragger_positions[i] = max_position;
				}
			}
		}
	}

	// Clamp the split_offset if requested.
	if (p_clamp_index != -1) {
		for (int i = 0; i < (int)dragger_positions.size(); i++) {
			split_offsets.write[i] = dragger_positions[i] - default_dragger_positions[i];
		}
	}

	// Invert if rtl.
	if (!vertical && is_layout_rtl()) {
		for (int i = 0; i < (int)dragger_positions.size(); i++) {
			dragger_positions[i] = size - dragger_positions[i] - sep;
		}
	}
}

void SplitContainer::add_child_notify(Node* p_child) {}

Size2 SplitContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	const int sep = _get_separation();
	const int axis = vertical ? 1 : 0;
	const int other_axis = vertical ? 0 : 1;

	Size2i minimum;

	if (valid_children.size() >= 2u) {
		minimum[axis] += sep * ((int)valid_children.size() - 1);
	}

	for (const Control* child : valid_children) {
		const Size2 min_size =
			p_use_desired_sizes ? child->get_bound_desired_size() : child->get_bound_minimum_size();
		minimum[axis] += (int)min_size[axis];
		minimum[other_axis] = (int)MAX(minimum[other_axis], min_size[other_axis]);
	}

	return minimum;
}

Size2 SplitContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 SplitContainer::get_desired_size() const { return _get_minimum_size(true); }

void SplitContainer::_on_child_visibility_changed(Control* p_control)
{
	if (p_control->is_visible()) {
		_add_valid_child(p_control);
	}
	else {
		_remove_valid_child(p_control);
	}
}

int SplitContainer::get_split_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, split_offsets.size(), 0);
	return split_offsets[p_index];
}

PackedInt32Array SplitContainer::get_split_offsets() const { return split_offsets; }

SplitContainer::DraggerVisibility SplitContainer::get_dragger_visibility() const
{
	return dragger_visibility;
}

bool SplitContainer::is_collapsed() const { return collapsed; }

bool SplitContainer::is_vertical() const { return vertical; }

void SplitContainer::set_dragging_enabled(bool p_enabled)
{
	if (dragging_enabled == p_enabled) {
		return;
	}
	dragging_enabled = p_enabled;
	if (!dragging_enabled) {
		for (SplitContainerDragger* dragger : dragging_area_controls) {
			dragger->stop_dragging();
		}
	}
	if (touch_dragger_enabled) {
		for (SplitContainerDragger* dragger : dragging_area_controls) {
			dragger->update_touch_dragger();
		}
	}
	if (get_viewport()) {
		get_viewport()->update_mouse_cursor_state();
	}
	_resort();
}

bool SplitContainer::is_dragging_enabled() const { return dragging_enabled; }

Vector<int> SplitContainer::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (!vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> SplitContainer::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	if (vertical) {
		flags.append(SIZE_EXPAND);
	}
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

int SplitContainer::get_drag_area_margin_begin() const { return drag_area_margin_begin; }

int SplitContainer::get_drag_area_margin_end() const { return drag_area_margin_end; }

int SplitContainer::get_drag_area_offset() const { return drag_area_offset; }

bool SplitContainer::is_show_drag_area_enabled() const { return show_drag_area; }

void SplitContainer::set_touch_dragger_enabled(bool p_enabled)
{
	if (touch_dragger_enabled == p_enabled) {
		return;
	}
	touch_dragger_enabled = p_enabled;
	for (SplitContainerDragger* dragger : dragging_area_controls) {
		dragger->set_touch_dragger_enabled(p_enabled);
	}
}

bool SplitContainer::is_touch_dragger_enabled() const { return touch_dragger_enabled; }

void SplitContainer::set_drag_nested_intersections(bool p_enabled)
{
	if (drag_nested_intersections == p_enabled) {
		return;
	}
	drag_nested_intersections = p_enabled;
	if (!is_inside_tree()) {
		return;
	}
	_update_nested_ancestors(!drag_nested_intersections);
	if (drag_nested_intersections) {
		_update_all_nested_descendents(this);
	}
	else {
		_remove_nested_descendent(nullptr);
	}
}

bool SplitContainer::is_dragging_nested_intersections() const { return drag_nested_intersections; }

SplitContainer::SplitContainer(bool p_vertical)
{
	vertical = p_vertical;
	split_offsets.push_back(0);

	SplitContainerDragger* dragger = memnew(SplitContainerDragger);
	dragging_area_controls.push_back(dragger);
	add_child(dragger, false, Node::INTERNAL_MODE_BACK);
}

void SplitContainer::set_split_offset(int p_offset, int p_index) {}

void SplitContainer::show_grabber_icon(int) {}

void SplitContainer::_update_nested_ancestors(bool) {}

void SplitContainer::_update_all_nested_descendents(Control*, Control*) {}

void SplitContainer::set_drag_area_margin_begin(int) {}

void SplitContainer::_resort() {}

void SplitContainer::_remove_valid_child(Control*) {}

void SplitContainer::_remove_nested_descendent(SplitContainer*) {}

void SplitContainerDragger::update_touch_dragger() {}

void SplitContainerDragger::stop_dragging() {}

void SplitContainerDragger::set_touch_dragger_enabled(bool) {}

void SplitContainer::clamp_split_offset(int) {}

void SplitContainer::_add_valid_child(Control*) {}

Control::CursorShape SplitContainerMultiDragger::get_cursor_shape(const Point2& p_pos) const
{
	return Control::CursorShape();
}

void SplitContainer::remove_child_notify(Node* p_child) {}

void SplitContainer::move_child_notify(Node* p_child) {}

Control::CursorShape SplitContainerDragger::get_cursor_shape(const Vector2& p_pos) const
{
	return Control::CURSOR_ARROW;
}

// --- split_container.compat.inc ---

#ifndef DISABLE_DEPRECATED


void SplitContainer::_clamp_split_offset_bind_compat_90411() { clamp_split_offset(0); }

void SplitContainer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- tab_bar.cpp ---

static inline Color _select_color(const Color& p_override_color, const Color& p_default_color)
{
	return p_override_color.a > 0 ? p_override_color : p_default_color;
}

Size2 TabBar::get_minimum_size() const
{
	Size2 ms;
	Size2 combined_max = get_combined_maximum_size();

	int buttons_size = get_tab_count() > 1 ? theme_cache.decrement_icon->get_width() +
												 theme_cache.increment_icon->get_width()
										   : 0;

	if (tabs.is_empty()) {
		return ms;
	}

	int y_margin = MAX(MAX(MAX(theme_cache.tab_unselected_style->get_minimum_size().height,
							   theme_cache.tab_hovered_style->get_minimum_size().height),
						   theme_cache.tab_selected_style->get_minimum_size().height),
		theme_cache.tab_disabled_style->get_minimum_size().height);
	int max_tab_width = 0;

	for (int i = 0; i < tabs.size(); i++) {
		if (tabs[i].hidden) {
			continue;
		}

		int ofs = ms.width;

		Ref<StyleBox> style;
		if (tabs[i].disabled) {
			style = theme_cache.tab_disabled_style;
		}
		else if (current == i) {
			style = theme_cache.tab_selected_style;
		}
		else if (hover == i) {
			style = theme_cache.tab_hovered_style;
		}
		else {
			style = theme_cache.tab_unselected_style;
		}
		ms.width += style->get_minimum_size().width;

		if (tabs[i].icon.is_valid()) {
			const Size2 icon_size = _get_tab_icon_size(i);
			ms.height = MAX(ms.height, icon_size.height + y_margin);
			ms.width += icon_size.width + theme_cache.h_separation;
		}

		if (!tabs[i].text.is_empty()) {
			ms.width += tabs[i].size_text + theme_cache.h_separation;
		}
		ms.height = MAX(ms.height, tabs[i].text_buf->get_size().y + y_margin);

		bool close_visible = cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
							 (cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && i == current);

		if (tabs[i].right_button.is_valid()) {
			Ref<Texture2D> rb = tabs[i].right_button;

			if (close_visible) {
				ms.width += theme_cache.button_hl_style->get_minimum_size().width + rb->get_width();
			}
			else {
				ms.width += theme_cache.button_hl_style->get_margin(SIDE_LEFT) + rb->get_width() +
							theme_cache.h_separation;
			}

			ms.height = MAX(ms.height, rb->get_height() + y_margin);
		}

		if (close_visible) {
			ms.width += theme_cache.button_hl_style->get_margin(SIDE_LEFT) +
						theme_cache.close_icon->get_width() + theme_cache.h_separation;

			ms.height = MAX(ms.height, theme_cache.close_icon->get_height() + y_margin);
		}

		if (ms.width - ofs > style->get_minimum_size().width) {
			ms.width -= theme_cache.h_separation;
		}

		if (i < tabs.size() - 1) {
			ms.width += theme_cache.tab_separation;
		}

		if (ms.width - ofs > max_tab_width) {
			max_tab_width = ms.width - ofs;
		}
	}

	if (clip_tabs) {
		ms.width = max_tab_width + buttons_size;
		if (combined_max.width >= 0) {
			ms.width = MIN(ms.width, int(combined_max.width));
		}
	}
	else if (combined_max.width >= 0) {
		ms.width = MIN(ms.width, int(combined_max.width));
	}

	return ms;
}

RID TabBar::get_tab_accessibility_element(int p_tab) const
{
	RID ae = get_accessibility_element();
	ERR_FAIL_COND_V(ae.is_null(), RID());

	const Tab& item = tabs[p_tab];
	if (item.accessibility_item_element.is_null()) {
		item.accessibility_item_element = AccessibilityServer::create_sub_element(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_TAB);
		item.accessibility_item_dirty = true;
	}
	return item.accessibility_item_element;
}

RID TabBar::get_focused_accessibility_element() const
{
	if (current == -1) {
		return get_accessibility_element();
	}
	else {
		const Tab& item = tabs[current];
		return item.accessibility_item_element;
	}
}

void TabBar::_draw_tab_drop(RID p_canvas_item)
{
	Vector2 size = get_size();
	int x;
	bool rtl = is_layout_rtl();

	int closest_tab = get_closest_tab_idx_to_point(get_local_mouse_position());
	if (closest_tab != -1) {
		Rect2 tab_rect = get_tab_rect(closest_tab);
		x = tab_rect.position.x;

		// Only add the tab_separation if closest tab is not on the edge.
		bool not_leftmost_tab =
			-1 != (rtl ? get_next_available(closest_tab) : get_previous_available(closest_tab));
		bool not_rightmost_tab =
			-1 != (rtl ? get_previous_available(closest_tab) : get_next_available(closest_tab));

		// Calculate midpoint between tabs.
		if (get_local_mouse_position().x > tab_rect.get_center().x) {
			x += tab_rect.size.x;
			if (not_rightmost_tab) {
				x += Math::ceil(0.5f * theme_cache.tab_separation);
			}
		}
		else if (not_leftmost_tab) {
			x -= Math::floor(0.5f * theme_cache.tab_separation);
		}
	}
	else {
		if (rtl ^ (get_local_mouse_position().x < get_tab_rect(0).position.x)) {
			x = get_tab_rect(0).position.x;
			if (rtl) {
				x += get_tab_rect(0).size.width;
			}
		}
		else {
			Rect2 tab_rect = get_tab_rect(get_tab_count() - 1);

			x = tab_rect.position.x;
			if (!rtl) {
				x += tab_rect.size.width;
			}
		}
	}

	theme_cache.drop_mark_icon->draw(p_canvas_item,
		Point2(x - theme_cache.drop_mark_icon->get_width() / 2,
			(size.height - theme_cache.drop_mark_icon->get_height()) / 2),
		theme_cache.drop_mark_color);
}

void TabBar::_draw_tab(Ref<StyleBox>& p_tab_style, const Color& p_font_color,
	const Color& p_icon_color, int p_index, float p_x, bool p_focus)
{
	RID ci = get_canvas_item();
	bool rtl = is_layout_rtl();

	Rect2 sb_rect = Rect2(p_x, 0, tabs[p_index].size_cache, get_size().height);
	if (tab_style_v_flip) {
		draw_set_transform(
			Point2(0.0, p_tab_style->get_draw_rect(sb_rect).size.y), 0.0, Size2(1.0, -1.0));
	}
	p_tab_style->draw(ci, sb_rect);
	if (tab_style_v_flip) {
		draw_set_transform(Point2(), 0.0, Size2(1.0, 1.0));
	}
	if (p_focus) {
		Ref<StyleBox> focus_style = theme_cache.tab_focus_style;
		focus_style->draw(ci, sb_rect);
	}

	p_x += rtl ? tabs[p_index].size_cache - p_tab_style->get_margin(SIDE_LEFT)
			   : p_tab_style->get_margin(SIDE_LEFT);

	Size2i sb_ms = p_tab_style->get_minimum_size();

	// Draw the icon.
	Ref<Texture2D> icon = tabs[p_index].icon;
	if (icon.is_valid()) {
		const Size2 icon_size = _get_tab_icon_size(p_index);
		const Point2 icon_pos = Point2i(rtl ? p_x - icon_size.width : p_x,
			p_tab_style->get_margin(SIDE_TOP) +
				((sb_rect.size.y - sb_ms.y) - icon_size.height) / 2);
		icon->draw_rect(ci, Rect2(icon_pos, icon_size), false, p_icon_color);

		p_x = rtl ? p_x - icon_size.width - theme_cache.h_separation
				  : p_x + icon_size.width + theme_cache.h_separation;
	}

	// Draw the text.
	if (!tabs[p_index].text.is_empty()) {
		Point2i text_pos = Point2i(rtl ? p_x - tabs[p_index].size_text : p_x,
			p_tab_style->get_margin(SIDE_TOP) +
				((sb_rect.size.y - sb_ms.y) - tabs[p_index].text_buf->get_size().y) / 2);

		if (theme_cache.outline_size > 0 && theme_cache.font_outline_color.a > 0) {
			tabs[p_index].text_buf->draw_outline(
				ci, text_pos, theme_cache.outline_size, theme_cache.font_outline_color);
		}
		tabs[p_index].text_buf->draw(ci, text_pos, p_font_color);

		p_x = rtl ? p_x - tabs[p_index].size_text - theme_cache.h_separation
				  : p_x + tabs[p_index].size_text + theme_cache.h_separation;
	}

	// Draw and calculate rect of the right button.
	if (tabs[p_index].right_button.is_valid()) {
		Ref<StyleBox> style = theme_cache.button_hl_style;
		Ref<Texture2D> rb = tabs[p_index].right_button;

		Rect2 rb_rect;
		rb_rect.size = style->get_minimum_size() + rb->get_size();
		rb_rect.position.x = rtl ? p_x - rb_rect.size.width : p_x;
		rb_rect.position.y =
			p_tab_style->get_margin(SIDE_TOP) + ((sb_rect.size.y - sb_ms.y) - (rb_rect.size.y)) / 2;

		tabs.write[p_index].rb_rect = rb_rect;

		if (rb_hover == p_index) {
			if (rb_pressing) {
				theme_cache.button_pressed_style->draw(ci, rb_rect);
			}
			else {
				style->draw(ci, rb_rect);
			}
		}

		rb->draw(ci, Point2i(rb_rect.position.x + style->get_margin(SIDE_LEFT),
						 rb_rect.position.y + style->get_margin(SIDE_TOP)));

		p_x = rtl ? rb_rect.position.x : rb_rect.position.x + rb_rect.size.width;
	}
	else {
		tabs.write[p_index].rb_rect = Rect2();
	}

	// Draw and calculate rect of the close button.
	if (cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
		(cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && p_index == current)) {
		Ref<StyleBox> style = theme_cache.button_hl_style;
		Ref<Texture2D> cb = theme_cache.close_icon;

		Rect2 cb_rect;
		cb_rect.size = style->get_minimum_size() + cb->get_size();
		cb_rect.position.x = rtl ? p_x - cb_rect.size.width : p_x;
		cb_rect.position.y =
			p_tab_style->get_margin(SIDE_TOP) + ((sb_rect.size.y - sb_ms.y) - (cb_rect.size.y)) / 2;

		tabs.write[p_index].cb_rect = cb_rect;

		if (!tabs[p_index].disabled && cb_hover == p_index) {
			if (cb_pressing) {
				theme_cache.button_pressed_style->draw(ci, cb_rect);
			}
			else {
				style->draw(ci, cb_rect);
			}
		}

		cb->draw(ci, Point2i(cb_rect.position.x + style->get_margin(SIDE_LEFT),
						 cb_rect.position.y + style->get_margin(SIDE_TOP)));
	}
	else {
		tabs.write[p_index].cb_rect = Rect2();
	}
}

int TabBar::get_tab_count() const { return tabs.size(); }

int TabBar::get_current_tab() const { return current; }

int TabBar::get_previous_tab() const { return previous; }

int TabBar::get_hovered_tab() const { return hover; }

int TabBar::get_previous_available(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < -1 || p_idx > get_tab_count(), -1);
	const int idx = p_idx == -1 ? get_current_tab() : p_idx;
	const int offset_end = idx + 1;
	for (int i = 1; i < offset_end; i++) {
		int target_tab = idx - i;
		if (target_tab < 0) {
			target_tab += get_tab_count();
		}
		if (!is_tab_disabled(target_tab) && !is_tab_hidden(target_tab)) {
			return target_tab;
		}
	}
	return -1;
}

int TabBar::get_next_available(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < -1 || p_idx > get_tab_count(), -1);
	const int idx = p_idx == -1 ? get_current_tab() : p_idx;
	const int offset_end = get_tab_count() - idx;
	for (int i = 1; i < offset_end; i++) {
		int target_tab = (idx + i) % get_tab_count();
		if (!is_tab_disabled(target_tab) && !is_tab_hidden(target_tab)) {
			return target_tab;
		}
	}
	return -1;
}

bool TabBar::select_previous_available()
{
	const int previous_available = get_previous_available();
	if (previous_available != -1) {
		set_current_tab(previous_available);
	}
	return previous_available != -1;
}

bool TabBar::select_next_available()
{
	const int next_available = get_next_available();
	if (next_available != -1) {
		set_current_tab(next_available);
	}
	return next_available != -1;
}

int TabBar::get_tab_offset() const { return offset; }

bool TabBar::get_offset_buttons_visible() const { return buttons_visible; }

String TabBar::get_tab_title(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].text;
}

void TabBar::set_tab_tooltip(int p_tab, const String& p_tooltip)
{
	ERR_FAIL_INDEX(p_tab, tabs.size());
	tabs.write[p_tab].tooltip = p_tooltip;
	queue_accessibility_update();
}

String TabBar::get_tab_tooltip(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].tooltip;
}

Control::TextDirection TabBar::get_tab_text_direction(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Control::TEXT_DIRECTION_INHERITED);
	return tabs[p_tab].text_direction;
}

String TabBar::get_tab_language(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), "");
	return tabs[p_tab].language;
}

Ref<Texture2D> TabBar::get_tab_icon(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Ref<Texture2D>());
	return tabs[p_tab].icon;
}

int TabBar::get_tab_icon_max_width(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), 0);
	return tabs[p_tab].icon_max_width;
}

Color TabBar::get_font_color_override(int p_tab, DrawMode p_draw_mode) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Color());
	ERR_FAIL_INDEX_V(p_draw_mode, DrawMode::DRAW_MAX, Color());

	return tabs[p_tab].font_color_overrides[p_draw_mode];
}

bool TabBar::is_tab_disabled(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), false);
	return tabs[p_tab].disabled;
}

bool TabBar::is_tab_hidden(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), false);
	return tabs[p_tab].hidden;
}

Ref<Texture2D> TabBar::get_tab_button_icon(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Ref<Texture2D>());
	return tabs[p_tab].right_button;
}

void TabBar::_update_cache(bool p_update_hover)
{
	if (tabs.is_empty()) {
		buttons_visible = false;
		return;
	}

	Size2 combined_max = get_combined_maximum_size();
	int combined_max_width = combined_max.width >= 0 ? int(combined_max.width) -
														   theme_cache.increment_icon->get_width() -
														   theme_cache.decrement_icon->get_width()
													 : INT_MAX;
	int effective_max_width =
		max_width > 0 ? MIN(max_width, combined_max_width) : combined_max_width;

	int limit =
		combined_max.width > 0 ? MIN(combined_max.width, get_size().width) : get_size().width;
	int limit_minus_buttons =
		limit - theme_cache.increment_icon->get_width() - theme_cache.decrement_icon->get_width();

	int w = 0;

	max_drawn_tab = tabs.size() - 1;

	for (int i = 0; i < tabs.size(); i++) {
		tabs.write[i].text_buf->set_width(-1);
		tabs.write[i].size_text = Math::ceil(tabs[i].text_buf->get_size().x);
		tabs.write[i].size_cache = get_tab_width(i);
		tabs.write[i].accessibility_item_dirty = true;

		tabs.write[i].truncated = effective_max_width > 0 && effective_max_width < INT_MAX &&
								  tabs[i].size_cache > effective_max_width;
		if (tabs[i].truncated) {
			int size_textless = tabs[i].size_cache - tabs[i].size_text;
			int mw = MAX(size_textless, effective_max_width);

			tabs.write[i].size_text = MAX(mw - size_textless, 1);
			tabs.write[i].text_buf->set_width(tabs[i].size_text);
			tabs.write[i].size_cache = size_textless + tabs[i].size_text;
		}

		if (i < offset || i > max_drawn_tab) {
			tabs.write[i].ofs_cache = 0;
			continue;
		}

		tabs.write[i].ofs_cache = w;

		if (tabs[i].hidden) {
			continue;
		}

		w += tabs[i].size_cache;

		// Check if all tabs would fit inside the area.
		if (clip_tabs && i > offset && (w > limit || (offset > 0 && w > limit_minus_buttons))) {
			tabs.write[i].ofs_cache = 0;

			w -= tabs[i].size_cache;
			w -= theme_cache.tab_separation;

			max_drawn_tab = i - 1;

			while (w > limit_minus_buttons && max_drawn_tab > offset) {
				tabs.write[max_drawn_tab].ofs_cache = 0;

				if (!tabs[max_drawn_tab].hidden) {
					w -= tabs[max_drawn_tab].size_cache;
					w -= theme_cache.tab_separation;
				}

				max_drawn_tab--;
			}
		}
		else if (i < tabs.size() - 1) {
			// Only add the tab separation if this isn't the last tab drawn.
			w += theme_cache.tab_separation;
		}
	}

	missing_right = max_drawn_tab < tabs.size() - 1;
	buttons_visible = offset > 0 || missing_right;

	if (tab_alignment == ALIGNMENT_LEFT) {
		if (p_update_hover) {
			_update_hover();
		}
		return;
	}

	if (tab_alignment == ALIGNMENT_CENTER) {
		w = ((buttons_visible ? limit_minus_buttons : limit) - w) / 2;
	}
	else if (tab_alignment == ALIGNMENT_RIGHT) {
		w = (buttons_visible ? limit_minus_buttons : limit) - w;
	}

	for (int i = offset; i <= max_drawn_tab; i++) {
		if (!tabs[i].hidden) {
			tabs.write[i].ofs_cache = w;

			w += tabs[i].size_cache;
			w += theme_cache.tab_separation;
		}
	}

	if (p_update_hover) {
		_update_hover();
	}
}

Size2 TabBar::get_desired_size() const
{
	if (!clip_tabs || tabs.is_empty()) {
		return Size2();
	}
	Size2 combined_max = get_combined_maximum_size();
	if (combined_max.width < 0) {
		return Size2();
	}

	int buttons_size = tabs.size() > 1 ? theme_cache.decrement_icon->get_width() +
											 theme_cache.increment_icon->get_width()
									   : 0;
	int limit = int(combined_max.width);
	int limit_minus_buttons = limit - buttons_size;

	int w = 0;
	bool overflowed = false;

	// First pass: check if all tabs fit without buttons.
	for (int i = offset; i < tabs.size(); i++) {
		if (tabs[i].hidden) {
			continue;
		}

		int next_w = w + tabs[i].size_cache;
		if (i > offset) {
			next_w += theme_cache.tab_separation;
		}

		if (next_w > limit) {
			overflowed = true;
			break;
		}

		w = next_w;
	}

	// If buttons are needed, recompute against the reduced limit.
	if (offset > 0 || overflowed) {
		w = 0;
		for (int i = offset; i < tabs.size(); i++) {
			if (tabs[i].hidden) {
				continue;
			}

			int next_w = w + tabs[i].size_cache;
			if (i > offset) {
				next_w += theme_cache.tab_separation;
			}

			if (next_w > limit_minus_buttons) {
				break;
			}

			w = next_w;
		}
	}

	int desired = (offset > 0 || overflowed) ? w + buttons_size : w;
	desired = MIN(desired, limit);

	return Size2(desired, 0);
}

void TabBar::_hover_switch_timeout() { set_current_tab(hover); }

int TabBar::get_tab_idx_at_point(const Point2& p_point) const
{
	if (tabs.is_empty()) {
		return -1;
	}

	int hover_now = -1;

	for (int i = offset; i <= max_drawn_tab; i++) {
		if (!tabs[i].hidden) {
			Rect2 rect = get_tab_rect(i);
			if (rect.has_point(p_point)) {
				hover_now = i;
			}
		}
	}

	return hover_now;
}

int TabBar::get_closest_tab_idx_to_point(const Point2& p_point) const
{
	if (tabs.is_empty()) {
		return -1;
	}

	int closest_tab = get_tab_idx_at_point(p_point);
	float closest_distance = FLT_MAX;

	// Search along the x-axis since the TabBar is horizontal.
	if (closest_tab == -1) {
		for (int i = offset; i <= max_drawn_tab; i++) {
			if (!tabs[i].hidden) {
				float center = get_tab_rect(i).get_center().x;
				float distance = Math::abs(center - p_point.x);
				if (distance < closest_distance) {
					closest_distance = distance;
					closest_tab = i;
				}
			}
		}
	}

	return closest_tab;
}

TabBar::AlignmentMode TabBar::get_tab_alignment() const { return tab_alignment; }

bool TabBar::get_clip_tabs() const { return clip_tabs; }

void TabBar::set_tab_style_v_flip(bool p_tab_style_v_flip)
{
	tab_style_v_flip = p_tab_style_v_flip;
}

int TabBar::get_tab_width(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, tabs.size(), 0);

	Ref<StyleBox> style;

	if (tabs[p_idx].disabled) {
		style = theme_cache.tab_disabled_style;
	}
	else if (current == p_idx) {
		style = theme_cache.tab_selected_style;
		// Always pick the widest style between hovered and unselected, to avoid an infinite loop
		// when switching tabs with the mouse.
	}
	else if (theme_cache.tab_hovered_style->get_minimum_size().width >
			   theme_cache.tab_unselected_style->get_minimum_size().width) {
		style = theme_cache.tab_hovered_style;
	}
	else {
		style = theme_cache.tab_unselected_style;
	}
	int x = style->get_minimum_size().width;

	if (tabs[p_idx].icon.is_valid()) {
		const Size2 icon_size = _get_tab_icon_size(p_idx);
		x += icon_size.width + theme_cache.h_separation;
	}

	if (!tabs[p_idx].text.is_empty()) {
		x += tabs[p_idx].size_text + theme_cache.h_separation;
	}

	bool close_visible = cb_displaypolicy == CLOSE_BUTTON_SHOW_ALWAYS ||
						 (cb_displaypolicy == CLOSE_BUTTON_SHOW_ACTIVE_ONLY && p_idx == current);

	if (tabs[p_idx].right_button.is_valid()) {
		Ref<StyleBox> btn_style = theme_cache.button_hl_style;
		Ref<Texture2D> rb = tabs[p_idx].right_button;

		if (close_visible) {
			x += btn_style->get_minimum_size().width + rb->get_width();
		}
		else {
			x += btn_style->get_margin(SIDE_LEFT) + rb->get_width() + theme_cache.h_separation;
		}
	}

	if (close_visible) {
		Ref<StyleBox> btn_style = theme_cache.button_hl_style;
		Ref<Texture2D> cb = theme_cache.close_icon;
		x += btn_style->get_margin(SIDE_LEFT) + cb->get_width() + theme_cache.h_separation;
	}

	if (x > style->get_minimum_size().width) {
		x -= theme_cache.h_separation;
	}

	return x;
}

Size2 TabBar::_get_tab_icon_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, tabs.size(), Size2());
	const TabBar::Tab& tab = tabs[p_index];
	Size2 icon_size = tab.icon->get_size();

	int icon_max_width = 0;
	if (theme_cache.icon_max_width > 0) {
		icon_max_width = theme_cache.icon_max_width;
	}
	if (tab.icon_max_width > 0 && (icon_max_width == 0 || tab.icon_max_width < icon_max_width)) {
		icon_max_width = tab.icon_max_width;
	}

	if (icon_max_width > 0 && icon_size.width > icon_max_width) {
		icon_size.height = icon_size.height * icon_max_width / icon_size.width;
		icon_size.width = icon_max_width;
	}

	return icon_size;
}

bool TabBar::_can_deselect() const
{
	if (deselect_enabled) {
		return true;
	}
	// All tabs must be disabled or hidden.
	for (const Tab& tab : tabs) {
		if (!tab.disabled && !tab.hidden) {
			return false;
		}
	}
	return true;
}

Rect2 TabBar::get_tab_rect(int p_tab) const
{
	ERR_FAIL_INDEX_V(p_tab, tabs.size(), Rect2());

	if (is_layout_rtl()) {
		return Rect2(get_size().width - tabs[p_tab].ofs_cache - tabs[p_tab].size_cache, 0,
			tabs[p_tab].size_cache, get_size().height);
	}
	else {
		return Rect2(tabs[p_tab].ofs_cache, 0, tabs[p_tab].size_cache, get_size().height);
	}
}

void TabBar::set_close_with_middle_mouse(bool p_scroll_close)
{
	close_with_middle_mouse = p_scroll_close;
}

bool TabBar::get_close_with_middle_mouse() const { return close_with_middle_mouse; }

TabBar::CloseButtonDisplayPolicy TabBar::get_tab_close_display_policy() const
{
	return cb_displaypolicy;
}

int TabBar::get_max_tab_width() const { return max_width; }

void TabBar::set_scrolling_enabled(bool p_enabled) { scrolling_enabled = p_enabled; }

bool TabBar::get_scrolling_enabled() const { return scrolling_enabled; }

void TabBar::set_drag_to_rearrange_enabled(bool p_enabled)
{
	drag_to_rearrange_enabled = p_enabled;
}

bool TabBar::get_drag_to_rearrange_enabled() const { return drag_to_rearrange_enabled; }

void TabBar::set_tabs_rearrange_group(int p_group_id) { tabs_rearrange_group = p_group_id; }

int TabBar::get_tabs_rearrange_group() const { return tabs_rearrange_group; }

void TabBar::set_scroll_to_selected(bool p_enabled)
{
	scroll_to_selected = p_enabled;
	if (p_enabled) {
		ensure_tab_visible(current);
	}
}

bool TabBar::get_scroll_to_selected() const { return scroll_to_selected; }

void TabBar::set_switch_on_drag_hover(bool p_enabled) { switch_on_drag_hover = p_enabled; }

bool TabBar::get_switch_on_drag_hover() const { return switch_on_drag_hover; }

void TabBar::set_select_with_rmb(bool p_enabled) { select_with_rmb = p_enabled; }

bool TabBar::get_select_with_rmb() const { return select_with_rmb; }

void TabBar::set_deselect_enabled(bool p_enabled)
{
	if (deselect_enabled == p_enabled) {
		return;
	}
	deselect_enabled = p_enabled;
	if (!deselect_enabled && current == -1 && !tabs.is_empty()) {
		select_next_available();
	}
}

bool TabBar::get_deselect_enabled() const { return deselect_enabled; }

void TabBar::set_current_tab(int p_current) {}



void TabBar::_update_hover() {}

void TabBar::set_tab_button_icon(int, Ref<Texture2D> const&) {}

void TabBar::set_tab_alignment(TabBar::AlignmentMode) {}

void TabBar::set_clip_tabs(bool) {}

void TabBar::ensure_tab_visible(int) {}

// --- tab_container.cpp ---

TabContainer::CachedTab* TabContainer::get_pending_tab(int p_idx) const
{
	if (p_idx >= pending_tabs.size()) {
		ERR_FAIL_COND_V(pending_tabs.resize(p_idx + 1) != OK, nullptr);
	}
	ERR_FAIL_INDEX_V(p_idx, pending_tabs.size(), nullptr);

	return pending_tabs.ptrw() + p_idx;
}

int TabContainer::_get_tab_height() const
{
	int height = 0;
	if (tabs_visible && get_tab_count() > 0) {
		height = tab_bar->get_minimum_size().height +
				 theme_cache.tabbar_style->get_margin(SIDE_TOP) +
				 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
	}

	return height;
}

void TabContainer::_repaint()
{
	layout_pending_start();
	_repaint_internal();
}

Vector<Control*> TabContainer::_get_tab_controls() const
{
	Vector<Control*> controls;
	ERR_THREAD_GUARD_V(controls);

	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (control) {
			controls.push_back(control);
		}
	}
	return controls;
}

void TabContainer::_drag_move_tab(int p_from_index, int p_to_index)
{
	move_child(get_tab_control(p_from_index), get_tab_control(p_to_index)->get_index(false));
}

TabBar* TabContainer::get_tab_bar() const { return tab_bar; }

int TabContainer::get_tab_count() const { return tab_bar->get_tab_count(); }

void TabContainer::set_current_tab(int p_current)
{
	if (!is_inside_tree()) {
		setup_current_tab = p_current;
		return;
	}

	tab_bar->set_current_tab(p_current);
}

int TabContainer::get_current_tab() const { return tab_bar->get_current_tab(); }

int TabContainer::get_previous_tab() const { return tab_bar->get_previous_tab(); }

bool TabContainer::select_previous_available() { return tab_bar->select_previous_available(); }

bool TabContainer::select_next_available() { return tab_bar->select_next_available(); }

void TabContainer::set_deselect_enabled(bool p_enabled)
{
	tab_bar->set_deselect_enabled(p_enabled);
}

bool TabContainer::get_deselect_enabled() const { return tab_bar->get_deselect_enabled(); }

Control* TabContainer::get_tab_control(int p_idx) const
{
	if (p_idx < 0) {
		return nullptr;
	}
	ERR_THREAD_GUARD_V(nullptr);

	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (!control) {
			continue;
		}

		if (p_idx > 0) {
			p_idx--;
		}
		else {
			return control;
		}
	}
	return nullptr;
}

Control* TabContainer::get_current_tab_control() const
{
	return get_tab_control(tab_bar->get_current_tab());
}

int TabContainer::get_tab_idx_at_point(const Point2& p_point) const
{
	return tab_bar->get_tab_idx_at_point(p_point);
}

int TabContainer::get_tab_idx_from_control(Control* p_child) const
{
	ERR_FAIL_NULL_V(p_child, -1);
	ERR_FAIL_COND_V(p_child->get_parent() != this, -1);
	ERR_THREAD_GUARD_V(-1);

	int idx = 0;
	for (Node* child : iterate_children()) {
		Control* control = _as_tab_control(child);
		if (!control) {
			continue;
		}

		if (control == p_child) {
			return idx;
		}
		idx++;
	}
	return -1;
}

void TabContainer::set_tab_alignment(TabBar::AlignmentMode p_alignment)
{
	if (tab_bar->get_tab_alignment() == p_alignment) {
		return;
	}

	tab_bar->set_tab_alignment(p_alignment);
	_update_margins();
}

TabBar::AlignmentMode TabContainer::get_tab_alignment() const
{
	return tab_bar->get_tab_alignment();
}

TabContainer::TabPosition TabContainer::get_tabs_position() const { return tabs_position; }

void TabContainer::set_tab_focus_mode(Control::FocusMode p_focus_mode)
{
	tab_bar->set_focus_mode(p_focus_mode);
}

Control::FocusMode TabContainer::get_tab_focus_mode() const { return tab_bar->get_focus_mode(); }

void TabContainer::set_clip_tabs(bool p_clip_tabs) { tab_bar->set_clip_tabs(p_clip_tabs); }

bool TabContainer::get_clip_tabs() const { return tab_bar->get_clip_tabs(); }

bool TabContainer::are_tabs_visible() const { return tabs_visible; }

#ifndef DISABLE_DEPRECATED
void TabContainer::set_all_tabs_in_front(bool p_in_front)
{
	if (p_in_front) {
		WARN_PRINT_ONCE("Due to internal changes, `all_tabs_in_front` doesn't do anything anymore, "
						"as they're always in front.");
	}
}

bool TabContainer::is_all_tabs_in_front() const { return false; }
#endif

String TabContainer::get_tab_title(int p_tab) const { return tab_bar->get_tab_title(p_tab); }

void TabContainer::set_tab_tooltip(int p_tab, const String& p_tooltip)
{
	tab_bar->set_tab_tooltip(p_tab, p_tooltip);
}

String TabContainer::get_tab_tooltip(int p_tab) const { return tab_bar->get_tab_tooltip(p_tab); }

Ref<Texture2D> TabContainer::get_tab_icon(int p_tab) const { return tab_bar->get_tab_icon(p_tab); }

int TabContainer::get_tab_icon_max_width(int p_tab) const
{
	return tab_bar->get_tab_icon_max_width(p_tab);
}

bool TabContainer::is_tab_disabled(int p_tab) const { return tab_bar->is_tab_disabled(p_tab); }

bool TabContainer::is_tab_hidden(int p_tab) const { return tab_bar->is_tab_hidden(p_tab); }

void TabContainer::set_tab_button_icon(int p_tab, const Ref<Texture2D>& p_icon)
{
	tab_bar->set_tab_button_icon(p_tab, p_icon);

	_update_margins();
	_repaint();
}

Ref<Texture2D> TabContainer::get_tab_button_icon(int p_tab) const
{
	return tab_bar->get_tab_button_icon(p_tab);
}

void TabContainer::remove_child_notify(Node* p_child) {}

void TabContainer::move_child_notify(Node* p_child) {}

void TabContainer::add_child_notify(Node* p_child) {}

Size2 TabContainer::_get_minimum_size(bool p_use_desired_sizes) const
{
	Size2 ms;

	if (tabs_visible) {
		ms = p_use_desired_sizes ? tab_bar->get_bound_desired_size() : tab_bar->get_minimum_size();
		ms.width += theme_cache.tabbar_style->get_margin(SIDE_LEFT) +
					theme_cache.tabbar_style->get_margin(SIDE_RIGHT);
		ms.height += theme_cache.tabbar_style->get_margin(SIDE_TOP) +
					 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);

		if (get_popup()) {
			ms.width += p_use_desired_sizes ? popup_button->get_bound_desired_size().x
											: popup_button->get_minimum_size().x;
		}

		if (theme_cache.side_margin > 0 && get_tab_alignment() != TabBar::ALIGNMENT_CENTER &&
			(get_tab_alignment() != TabBar::ALIGNMENT_RIGHT || !get_popup())) {
			ms.width += theme_cache.side_margin;
		}
	}

	Vector<Control*> controls = _get_tab_controls();
	Size2 largest_child_min_size;
	for (int i = 0; i < controls.size(); i++) {
		Control* c = controls[i];

		if (!c->is_visible() && !use_hidden_tabs_for_min_size) {
			continue;
		}

		Size2 cms = p_use_desired_sizes ? c->get_bound_desired_size() : c->get_bound_minimum_size();
		largest_child_min_size = largest_child_min_size.max(cms);
	}
	ms.height += largest_child_min_size.height;

	Size2 panel_ms = theme_cache.panel_style->get_minimum_size();

	ms.width = MAX(ms.width, largest_child_min_size.width + panel_ms.width);
	ms.height += panel_ms.height;

	return ms;
}

Size2 TabContainer::get_minimum_size() const { return _get_minimum_size(false); }

Size2 TabContainer::get_desired_size() const { return _get_minimum_size(true); }

Size2 TabContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Control::get_inner_combined_maximum_size();

	if (tabs_visible && tab_bar) {
		Size2 tab_bar_ms = tab_bar->get_minimum_size();
		ms.height -= tab_bar_ms.height;

		if (theme_cache.tabbar_style.is_valid()) {
			ms.height -= theme_cache.tabbar_style->get_margin(SIDE_TOP) +
						 theme_cache.tabbar_style->get_margin(SIDE_BOTTOM);
		}
	}

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	return ms;
}

void TabContainer::set_switch_on_drag_hover(bool p_enabled)
{
	tab_bar->set_switch_on_drag_hover(p_enabled);
}

bool TabContainer::get_switch_on_drag_hover() const { return tab_bar->get_switch_on_drag_hover(); }

void TabContainer::set_drag_to_rearrange_enabled(bool p_enabled)
{
	drag_to_rearrange_enabled = p_enabled;
}

bool TabContainer::get_drag_to_rearrange_enabled() const { return drag_to_rearrange_enabled; }

void TabContainer::set_tabs_rearrange_group(int p_group_id)
{
	tab_bar->set_tabs_rearrange_group(p_group_id);
}

int TabContainer::get_tabs_rearrange_group() const { return tab_bar->get_tabs_rearrange_group(); }

bool TabContainer::get_use_hidden_tabs_for_min_size() const { return use_hidden_tabs_for_min_size; }

Vector<int> TabContainer::get_allowed_size_flags_horizontal() const { return Vector<int>(); }

Vector<int> TabContainer::get_allowed_size_flags_vertical() const { return Vector<int>(); }

Popup* TabContainer::get_popup() const
{
	Popup p = Popup();
	return &p;
}

Control* TabContainer::_as_tab_control(Node*) const
{
	Control c = Control();
	return &c;
}

void TabContainer::_update_margins() {}

void TabContainer::_repaint_internal() {}

void TabContainer::set_tab_icon(int, Ref<Texture2D> const&) {}

void TabContainer::set_popup(Node*) {}

// --- foldable_container.cpp ---

Size2 FoldableContainer::get_inner_combined_maximum_size() const
{
	Size2 ms = Control::get_inner_combined_maximum_size();

	if (theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}

	return ms;
}

bool FoldableContainer::is_folded() const { return folded; }

Ref<FoldableGroup> FoldableContainer::get_foldable_group() const { return foldable_group; }

String FoldableContainer::get_title() const { return title; }

HorizontalAlignment FoldableContainer::get_title_alignment() const { return title_alignment; }

String FoldableContainer::get_language() const { return language; }

Control::TextDirection FoldableContainer::get_title_text_direction() const
{
	return title_text_direction;
}

TextServer::OverrunBehavior FoldableContainer::get_title_text_overrun_behavior() const
{
	return overrun_behavior;
}

FoldableContainer::TitlePosition FoldableContainer::get_title_position() const
{
	return title_position;
}

void FoldableContainer::add_title_bar_control(Control* p_control)
{
	ERR_FAIL_NULL(p_control);
	if (p_control->get_parent()) {
		p_control->get_parent()->remove_child(p_control);
		ERR_FAIL_COND_MSG(
			p_control->get_parent() != nullptr, "Failed to remove control from parent.");
	}
	add_child(p_control, false, INTERNAL_MODE_FRONT);
	title_controls.push_back(p_control);
}

void FoldableContainer::remove_title_bar_control(Control* p_control)
{
	ERR_FAIL_NULL(p_control);

	int64_t index = title_controls.find(p_control);
	ERR_FAIL_COND_MSG(index == -1, "Can't remove control from title bar.");

	title_controls.remove_at(index);
	remove_child(p_control);
}

bool FoldableContainer::has_point(const Point2& p_point) const
{
	if (folded) {
		return _get_title_rect().has_point(p_point);
	}
	return Control::has_point(p_point);
}

void FoldableContainer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		RID ci = get_canvas_item();
		Size2 size = get_size();
		int h_separation = _get_h_separation();

		Ref<StyleBox> title_style = _get_title_style();
		Ref<Texture2D> icon = _get_title_icon();

		real_t title_controls_width = _get_title_controls_width();
		if (title_controls_width > 0) {
			title_controls_width += h_separation;
		}

		const Rect2 title_rect = _get_title_rect();
		_draw_flippable_stylebox(title_style, title_rect);

		Size2 title_ms = title_style->get_minimum_size();
		int title_text_width = size.width - title_ms.width;

		int title_style_ofs = (title_position == POSITION_TOP)
								  ? title_style->get_margin(SIDE_TOP)
								  : title_style->get_margin(SIDE_BOTTOM);
		Point2 title_text_pos(title_style->get_margin(SIDE_LEFT), title_style_ofs);
		title_text_pos.y += MAX(
			(title_minimum_size.height - title_ms.height - text_buf->get_size().height) * 0.5, 0);

		title_text_width -= icon->get_width() + h_separation + title_controls_width;
		Point2 icon_pos(
			0, MAX((title_minimum_size.height - title_ms.height - icon->get_height()) * 0.5, 0) +
				   title_style_ofs);

		bool rtl = is_layout_rtl();
		if (rtl) {
			icon_pos.x = size.width - title_style->get_margin(SIDE_RIGHT) - icon->get_width();
			title_text_pos.x += title_controls_width;
		}
		else {
			icon_pos.x = title_style->get_margin(SIDE_LEFT);
			title_text_pos.x += icon->get_width() + h_separation;
		}
		icon->draw(ci, title_rect.position + icon_pos);

		Color font_color =
			folded ? theme_cache.title_collapsed_font_color : theme_cache.title_font_color;
		if (is_hovering) {
			font_color = theme_cache.title_hovered_font_color;
		}
		text_buf->set_width(title_text_width);

		if (title_text_width > 0) {
			if (theme_cache.title_font_outline_size > 0 &&
				theme_cache.title_font_outline_color.a > 0) {
				text_buf->draw_outline(ci, title_rect.position + title_text_pos,
					theme_cache.title_font_outline_size, theme_cache.title_font_outline_color);
			}
			text_buf->draw(ci, title_rect.position + title_text_pos, font_color);
		}

		if (!folded) {
			Rect2 panel_rect(
				Point2(0, (title_position == POSITION_TOP) ? title_minimum_size.height : 0),
				Size2(size.width, size.height - title_minimum_size.height));
			_draw_flippable_stylebox(theme_cache.panel_style, panel_rect);
		}

		if (has_focus(true)) {
			Rect2 focus_rect = folded ? title_rect : Rect2(Point2(), size);
			_draw_flippable_stylebox(theme_cache.focus_style, focus_rect);
		}
	} break;
	}
}

real_t FoldableContainer::_get_title_controls_width() const
{
	real_t width = 0.0;
	int visible_controls = 0;
	for (const Control* control : title_controls) {
		if (control->is_visible()) {
			width += control->get_bound_minimum_size().x;
			visible_controls++;
		}
	}
	if (visible_controls > 1) {
		width += _get_h_separation() * (visible_controls - 1);
	}
	return width;
}

Ref<StyleBox> FoldableContainer::_get_title_style() const
{
	if (is_hovering) {
		return folded ? theme_cache.title_collapsed_hover_style : theme_cache.title_hover_style;
	}
	return folded ? theme_cache.title_collapsed_style : theme_cache.title_style;
}

Ref<Texture2D> FoldableContainer::_get_title_icon() const
{
	if (!folded) {
		return (title_position == POSITION_TOP) ? theme_cache.expanded_arrow
												: theme_cache.expanded_arrow_mirrored;
	}
	else if (is_layout_rtl()) {
		return theme_cache.folded_arrow_mirrored;
	}
	return theme_cache.folded_arrow;
}

Rect2 FoldableContainer::_get_title_rect() const
{
	return Rect2(0,
		(title_position == POSITION_TOP) ? 0 : (get_size().height - title_minimum_size.height),
		get_size().width, title_minimum_size.height);
}

void FoldableContainer::_update_title_min_size() const
{
	Ref<StyleBox> title_style =
		folded ? theme_cache.title_collapsed_style : theme_cache.title_style;
	Ref<Texture2D> icon = _get_title_icon();
	Size2 title_ms = title_style->get_minimum_size();
	int h_separation = _get_h_separation();

	title_minimum_size = title_ms;
	title_minimum_size.width += icon->get_width();

	if (!title.is_empty()) {
		title_minimum_size.width += h_separation;
		Size2 text_size = text_buf->get_size();
		title_minimum_size.height += MAX(text_size.height, icon->get_height());
		if (overrun_behavior == TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING) {
			title_minimum_size.width += text_size.width;
		}
	}
	else {
		title_minimum_size.height += icon->get_height();
	}

	if (!title_controls.is_empty()) {
		real_t controls_height = 0;
		int visible_controls = 0;

		for (const Control* control : title_controls) {
			if (!control->is_visible()) {
				continue;
			}
			Vector2 size = control->get_bound_minimum_size();
			title_minimum_size.width += size.width;
			controls_height = MAX(controls_height, size.height);
			visible_controls++;
		}
		if (visible_controls > 0) {
			title_minimum_size.width += h_separation * visible_controls;
		}
		title_minimum_size.height =
			MAX(title_minimum_size.height, title_ms.height + controls_height);
	}
}

HorizontalAlignment FoldableContainer::_get_actual_alignment() const
{
	if (is_layout_rtl()) {
		if (title_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			return HORIZONTAL_ALIGNMENT_LEFT;
		}
		else if (title_alignment == HORIZONTAL_ALIGNMENT_LEFT) {
			return HORIZONTAL_ALIGNMENT_RIGHT;
		}
	}
	return title_alignment;
}

void FoldableContainer::_update_group()
{
	foldable_group->updating_group = true;
	for (FoldableContainer* container : foldable_group->containers) {
		if (container != this) {
			container->set_folded(true);
		}
	}
	foldable_group->updating_group = false;
}

void FoldableContainer::_draw_flippable_stylebox(
	const Ref<StyleBox> p_stylebox, const Rect2& p_rect)
{
	if (title_position == POSITION_BOTTOM) {
		Rect2 rect(-p_rect.position, p_rect.size);
		draw_set_transform(
			Point2(0.0, p_stylebox->get_draw_rect(rect).size.height), 0.0, Size2(1.0, -1.0));
		p_stylebox->draw(get_canvas_item(), rect);
		draw_set_transform_matrix(Transform2D());
	}
	else {
		p_stylebox->draw(get_canvas_item(), p_rect);
	}
}

FoldableContainer::FoldableContainer(const String& p_text)
{
	text_buf.instantiate();
	set_title(p_text);
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
}

FoldableContainer::~FoldableContainer()
{
	if (foldable_group.is_valid()) {
		foldable_group->containers.erase(this);
	}
}

FoldableContainer* FoldableGroup::get_expanded_container() const
{
	for (FoldableContainer* container : containers) {
		if (!container->is_folded()) {
			return container;
		}
	}

	return nullptr;
}

void FoldableGroup::set_allow_folding_all(bool p_enabled)
{
	allow_folding_all = p_enabled;
	if (!allow_folding_all && !get_expanded_container() && containers.size() > 0) {
		updating_group = true;
		(*containers.begin())->set_folded(false);
		updating_group = false;
	}
}

bool FoldableGroup::is_allow_folding_all() const { return allow_folding_all; }

void FoldableGroup::get_containers(List<FoldableContainer*>* r_containers) const
{
	for (FoldableContainer* container : containers) {
		r_containers->push_back(container);
	}
}

FoldableGroup::FoldableGroup() { set_local_to_scene(true); }

Size2 FoldableContainer::get_minimum_size() const { return Size2(); }

Size2 FoldableContainer::get_desired_size() const { return Size2(); }

void FoldableContainer::gui_input(const InputEvent& p_event) {}

String FoldableContainer::get_tooltip(const Vector2& p_pos) const { return String(); }

void FoldableContainer::set_folded(bool) {}

void FoldableContainer::set_title(String const&) {}

// --- subviewport_container.cpp ---

bool SubViewportContainer::is_stretch_enabled() const { return stretch; }

int SubViewportContainer::get_stretch_shrink() const { return shrink; }

Vector<int> SubViewportContainer::get_allowed_size_flags_horizontal() const
{
	return Vector<int>();
}

Vector<int> SubViewportContainer::get_allowed_size_flags_vertical() const { return Vector<int>(); }

void SubViewportContainer::input(const InputEvent& p_event)
{
	_propagate_nonpositional_event(p_event);
}

void SubViewportContainer::unhandled_input(const InputEvent& p_event)
{
	_propagate_nonpositional_event(p_event);
}

void SubViewportContainer::add_child_notify(Node* p_child) {}

void SubViewportContainer::_propagate_nonpositional_event(const InputEvent& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	if (Engine::is_editor_hint()) {
		return;
	}

	if (_is_propagated_in_gui_input(p_event)) {
		return;
	}

	_send_event_to_viewports(p_event);
}

void SubViewportContainer::gui_input(const InputEvent& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	if (Engine::is_editor_hint()) {
		return;
	}

	if (!_is_propagated_in_gui_input(p_event)) {
		return;
	}

	if (stretch && shrink > 1) {
		Transform2D xform;
		xform.scale(Vector2(1, 1) / shrink);
		_send_event_to_viewports(p_event.xformed_by(xform));
	}
	else {
		_send_event_to_viewports(p_event);
	}
}

void SubViewportContainer::set_mouse_target(bool p_enable) { mouse_target = p_enable; }

bool SubViewportContainer::is_mouse_target_enabled() { return mouse_target; }

SubViewportContainer::SubViewportContainer()
{
	set_process_unhandled_input(true);
	set_focus_mode(FOCUS_CLICK);
}

void SubViewportContainer::_send_event_to_viewports(const InputEvent& p_event) {}

void SubViewportContainer::remove_child_notify(Node* p_child) {}

bool SubViewportContainer::_is_propagated_in_gui_input(const InputEvent&) { return false; }

Size2 SubViewportContainer::get_minimum_size() const { return Size2(); }

PackedStringArray SubViewportContainer::get_configuration_warnings() const { return PackedStringArray(); }

// --- panel.cpp ---

Size2 Panel::get_inner_combined_maximum_size() const
{
	Size2 ms = Control::get_inner_combined_maximum_size();
	if (fit_children && theme_cache.panel_style.is_valid()) {
		ms -= theme_cache.panel_style->get_minimum_size();
	}
	return ms;
}

void Panel::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_PANEL);
	} break;

	case NOTIFICATION_DRAW: {
		RID ci = get_canvas_item();
		if (theme_cache.panel_style.is_valid()) {
			theme_cache.panel_style->draw(ci, Rect2(Point2(), get_size()));
		}
	} break;
	}
}

Panel::Panel(bool p_fit_children) : fit_children(p_fit_children)
{
	set_mouse_filter(MOUSE_FILTER_STOP);
}

// --- separator.cpp ---

Size2 Separator::get_minimum_size() const
{
	Size2 ms(3, 3);
	if (orientation == VERTICAL) {
		ms.x = theme_cache.separation;
	}
	else { // HORIZONTAL
		ms.y = theme_cache.separation;
	}
	return ms;
}

void Separator::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		Size2i size = get_size();
		Size2i ssize = theme_cache.separator_style->get_minimum_size();

		if (orientation == VERTICAL) {
			theme_cache.separator_style->draw(
				get_canvas_item(), Rect2((size.x - ssize.x) / 2, 0, ssize.x, size.y));
		}
		else {
			theme_cache.separator_style->draw(
				get_canvas_item(), Rect2(0, (size.y - ssize.y) / 2, size.x, ssize.y));
		}
	} break;
	}
}

Separator::Separator(Orientation p_orientation) : orientation(p_orientation) {}

Separator::~Separator() {}
