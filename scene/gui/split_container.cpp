/**************************************************************************/
/*  split_container.cpp                                                   */
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

#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/viewport.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "split_container.compat.inc"
#include "split_container.h"

void SplitContainerMultiDragger::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_MOUSE_ENTER: {
		if (!dragging && !Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
			split_container->show_grabber_icon(dragger_index);
		}
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		if (!dragging && !Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT)) {
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
	if (is_fixed) {
		return theme_cache.grabber_icon;
	}
	else {
		if (vertical) {
			return theme_cache.grabber_icon_v;
		}
		else {
			return theme_cache.grabber_icon_h;
		}
	}
}

Ref<Texture2D> SplitContainer::_get_touch_dragger_icon() const
{
	if (is_fixed) {
		return theme_cache.touch_dragger_icon;
	}
	else {
		if (vertical) {
			return theme_cache.touch_dragger_icon_v;
		}
		else {
			return theme_cache.touch_dragger_icon_h;
		}
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

void SplitContainer::move_child_notify(Node* p_child)
{
	Container::move_child_notify(p_child);

	Control* moved_child = as_sortable_control(p_child, SortableVisibilityMode::IGNORE);
	const int prev_index = valid_children.find(moved_child);
	if (prev_index == -1) {
		return;
	}

	PackedInt32Array desired_sizes;
	if (initialized && !split_offset_pending && valid_children.size() > 2u &&
		split_offsets.size() == (int)default_dragger_positions.size()) {
		desired_sizes = _get_desired_sizes();
	}

	valid_children.remove_at(prev_index);

	// Get new index.
	int index = 0;
	for (int i = 0; i < get_child_count(false); i++) {
		Control* child = as_sortable_control(get_child(i, false), SortableVisibilityMode::IGNORE);
		if (!child) {
			continue;
		}
		if (child == moved_child) {
			break;
		}
		if (valid_children.has(child)) {
			index++;
		}
	}

	valid_children.insert(index, moved_child);

	if (desired_sizes.is_empty()) {
		return;
	}

	const int prev_desired_size = desired_sizes[prev_index];
	desired_sizes.remove_at(prev_index);
	desired_sizes.insert(index, prev_desired_size);
	_set_desired_sizes(desired_sizes, index);
}

void SplitContainer::_on_child_visibility_changed(Control* p_control)
{
	if (p_control->is_visible()) {
		_add_valid_child(p_control);
	}
	else {
		_remove_valid_child(p_control);
	}
}

void SplitContainer::set_split_offset(int p_offset, int p_index)
{
	ERR_FAIL_INDEX(p_index, split_offsets.size());
	if (split_offsets[p_index] == p_offset) {
		return;
	}

	split_offsets.write[p_index] = p_offset;
	queue_sort();
}

int SplitContainer::get_split_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, split_offsets.size(), 0);
	return split_offsets[p_index];
}

void SplitContainer::set_split_offsets(const PackedInt32Array& p_offsets)
{
	if (split_offsets == p_offsets) {
		return;
	}
	split_offsets = p_offsets;
	split_offset_pending =
		split_offsets.size() > 1 && (int)valid_children.size() - 1 != split_offsets.size();
	queue_sort();
}

PackedInt32Array SplitContainer::get_split_offsets() const { return split_offsets; }

void SplitContainer::clamp_split_offset(int p_priority_index)
{
	ERR_FAIL_INDEX(p_priority_index, split_offsets.size());
	if (valid_children.size() < 2u) {
		// Needs at least two children.
		return;
	}

	_update_dragger_positions(p_priority_index);
	queue_sort();
}

void SplitContainer::set_collapsed(bool p_collapsed)
{
	if (collapsed == p_collapsed) {
		return;
	}
	collapsed = p_collapsed;
	queue_sort();
}

void SplitContainer::set_dragger_visibility(DraggerVisibility p_visibility)
{
	if (dragger_visibility == p_visibility) {
		return;
	}
	dragger_visibility = p_visibility;
	queue_sort();
}

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

void SplitContainer::set_drag_area_margin_begin(int p_margin)
{
	if (drag_area_margin_begin == p_margin) {
		return;
	}
	drag_area_margin_begin = p_margin;
	queue_sort();
}

int SplitContainer::get_drag_area_margin_begin() const { return drag_area_margin_begin; }

void SplitContainer::set_drag_area_margin_end(int p_margin)
{
	if (drag_area_margin_end == p_margin) {
		return;
	}
	drag_area_margin_end = p_margin;
	queue_sort();
}

int SplitContainer::get_drag_area_margin_end() const { return drag_area_margin_end; }

void SplitContainer::set_drag_area_offset(int p_offset)
{
	if (drag_area_offset == p_offset) {
		return;
	}
	drag_area_offset = p_offset;
	queue_sort();
}

int SplitContainer::get_drag_area_offset() const { return drag_area_offset; }

void SplitContainer::set_show_drag_area_enabled(bool p_enabled)
{
	show_drag_area = p_enabled;
	for (SplitContainerDragger* dragger : dragging_area_controls) {
		dragger->queue_redraw();
	}
}

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

void SplitContainer::show_grabber_icon(int p_index)
{
	if (force_show_grabber_icon == p_index) {
		return;
	}
	force_show_grabber_icon = p_index;
	for (SplitContainerDragger* dragger : dragging_area_controls) {
		dragger->queue_redraw();
	}
}

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

void SplitContainer::_bind_methods() {}

SplitContainer::SplitContainer(bool p_vertical)
{
	vertical = p_vertical;
	split_offsets.push_back(0);

	SplitContainerDragger* dragger = memnew(SplitContainerDragger);
	dragging_area_controls.push_back(dragger);
	add_child(dragger, false, Node::INTERNAL_MODE_BACK);
}


