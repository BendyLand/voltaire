/**************************************************************************/
/*  scroll_container.cpp                                                  */
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
#include "split_container.h"
#include <cfloat>
#include "scene/gui/label.h"
#include "scene/main/timer.h"
#include "tab_bar.h"
#include "scene/gui/button.h"
#include "scene/gui/popup.h"
#include "tab_container.h"
#include "foldable_container.h"
#include "scene/resources/text_line.h"
#include "core/types.h"
#include "subviewport_container.h"
#include "separator.h"

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
