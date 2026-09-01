/**************************************************************************/
/*  canvas_item_editor_plugin.cpp                                         */
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

#include "canvas_item_editor_plugin.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "core/types.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/gui/control_editor_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_preview_button.h"
#include "editor/translations/editor_translation_preview_menu.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/physics/touch_screen_button.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/gui/base_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/view_panner.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_texture.h"
#include "servers/rendering/rendering_server.h"

#define DRAG_THRESHOLD (8 * EDSCALE)
constexpr real_t SCALE_HANDLE_DISTANCE = 25;
constexpr real_t MOVE_HANDLE_DISTANCE = 25;

class SnapDialog : public ConfirmationDialog
{
	friend class CanvasItemEditor;

	SpinBox* grid_offset_x;
	SpinBox* grid_offset_y;
	SpinBox* grid_step_x;
	SpinBox* grid_step_y;
	SpinBox* primary_grid_step_x;
	SpinBox* primary_grid_step_y;
	SpinBox* rotation_offset;
	SpinBox* rotation_step;
	SpinBox* scale_step;

public:
	SnapDialog()
	{
		const int SPIN_BOX_GRID_RANGE = 16384;
		const int SPIN_BOX_ROTATION_RANGE = 360;
		const real_t SPIN_BOX_SCALE_MIN = 0.01;
		const real_t SPIN_BOX_SCALE_MAX = 100;

		Label* label;
		VBoxContainer* container;
		GridContainer* child_container;

		set_title(TTRC("Configure Snap"));

		container = memnew(VBoxContainer);
		add_child(container);

		child_container = memnew(GridContainer);
		child_container->set_columns(3);
		container->add_child(child_container);

		label = memnew(Label);
		label->set_text(TTRC("Grid Offset:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		grid_offset_x = memnew(SpinBox);
		grid_offset_x->set_min(-SPIN_BOX_GRID_RANGE);
		grid_offset_x->set_max(SPIN_BOX_GRID_RANGE);
		grid_offset_x->set_allow_lesser(true);
		grid_offset_x->set_allow_greater(true);
		grid_offset_x->set_suffix("px");
		grid_offset_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_offset_x->set_select_all_on_focus(true);
		grid_offset_x->set_accessibility_name(TTRC("X Offset"));
		child_container->add_child(grid_offset_x);

		grid_offset_y = memnew(SpinBox);
		grid_offset_y->set_min(-SPIN_BOX_GRID_RANGE);
		grid_offset_y->set_max(SPIN_BOX_GRID_RANGE);
		grid_offset_y->set_allow_lesser(true);
		grid_offset_y->set_allow_greater(true);
		grid_offset_y->set_suffix("px");
		grid_offset_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_offset_y->set_select_all_on_focus(true);
		grid_offset_y->set_accessibility_name(TTRC("Y Offset"));
		child_container->add_child(grid_offset_y);

		label = memnew(Label);
		label->set_text(TTRC("Grid Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		grid_step_x = memnew(SpinBox);
		grid_step_x->set_min(1);
		grid_step_x->set_max(SPIN_BOX_GRID_RANGE);
		grid_step_x->set_allow_greater(true);
		grid_step_x->set_suffix("px");
		grid_step_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_step_x->set_select_all_on_focus(true);
		grid_step_x->set_accessibility_name(TTRC("X Step"));
		child_container->add_child(grid_step_x);

		grid_step_y = memnew(SpinBox);
		grid_step_y->set_min(1);
		grid_step_y->set_max(SPIN_BOX_GRID_RANGE);
		grid_step_y->set_allow_greater(true);
		grid_step_y->set_suffix("px");
		grid_step_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_step_y->set_select_all_on_focus(true);
		grid_step_y->set_accessibility_name(TTRC("X Step"));
		child_container->add_child(grid_step_y);

		label = memnew(Label);
		label->set_text(TTRC("Primary Line Every:"));
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_container->add_child(label);

		primary_grid_step_x = memnew(SpinBox);
		primary_grid_step_x->set_min(1);
		primary_grid_step_x->set_step(1);
		primary_grid_step_x->set_max(SPIN_BOX_GRID_RANGE);
		primary_grid_step_x->set_allow_greater(true);
		primary_grid_step_x->set_suffix("steps");
		primary_grid_step_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		primary_grid_step_x->set_select_all_on_focus(true);
		primary_grid_step_x->set_accessibility_name(TTRC("X Primary Step"));
		child_container->add_child(primary_grid_step_x);

		primary_grid_step_y = memnew(SpinBox);
		primary_grid_step_y->set_min(1);
		primary_grid_step_y->set_step(1);
		primary_grid_step_y->set_max(SPIN_BOX_GRID_RANGE);
		primary_grid_step_y->set_allow_greater(true);
		primary_grid_step_y->set_suffix(TTRC("steps")); // TODO: Add suffix auto-translation.
		primary_grid_step_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		primary_grid_step_y->set_select_all_on_focus(true);
		primary_grid_step_y->set_accessibility_name(TTRC("Y Primary Step"));
		child_container->add_child(primary_grid_step_y);

		container->add_child(memnew(HSeparator));

		// We need to create another GridContainer with the same column count,
		// so we can put an HSeparator above
		child_container = memnew(GridContainer);
		child_container->set_columns(2);
		container->add_child(child_container);

		label = memnew(Label);
		label->set_text(TTRC("Rotation Offset:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		rotation_offset = memnew(SpinBox);
		rotation_offset->set_min(-SPIN_BOX_ROTATION_RANGE);
		rotation_offset->set_max(SPIN_BOX_ROTATION_RANGE);
		rotation_offset->set_suffix(U"°");
		rotation_offset->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rotation_offset->set_select_all_on_focus(true);
		rotation_offset->set_accessibility_name(TTRC("Rotation Offset:"));
		child_container->add_child(rotation_offset);

		label = memnew(Label);
		label->set_text(TTRC("Rotation Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		rotation_step = memnew(SpinBox);
		rotation_step->set_min(-SPIN_BOX_ROTATION_RANGE);
		rotation_step->set_max(SPIN_BOX_ROTATION_RANGE);
		rotation_step->set_suffix(U"°");
		rotation_step->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rotation_step->set_select_all_on_focus(true);
		rotation_step->set_accessibility_name(TTRC("Rotation Step:"));
		child_container->add_child(rotation_step);

		container->add_child(memnew(HSeparator));

		child_container = memnew(GridContainer);
		child_container->set_columns(2);
		container->add_child(child_container);
		label = memnew(Label);
		label->set_text(TTRC("Scale Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		scale_step = memnew(SpinBox);
		scale_step->set_min(SPIN_BOX_SCALE_MIN);
		scale_step->set_max(SPIN_BOX_SCALE_MAX);
		scale_step->set_allow_greater(true);
		scale_step->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		scale_step->set_step(0.01f);
		scale_step->set_select_all_on_focus(true);
		scale_step->set_accessibility_name(TTRC("Scale Step:"));
		child_container->add_child(scale_step);
	}

	void set_fields(const Point2 p_grid_offset, const Point2 p_grid_step,
		const Vector2i p_primary_grid_step, const real_t p_rotation_offset,
		const real_t p_rotation_step, const real_t p_scale_step)
	{
		grid_offset_x->set_value(p_grid_offset.x);
		grid_offset_y->set_value(p_grid_offset.y);
		grid_step_x->set_value(p_grid_step.x);
		grid_step_y->set_value(p_grid_step.y);
		primary_grid_step_x->set_value(p_primary_grid_step.x);
		primary_grid_step_y->set_value(p_primary_grid_step.y);
		rotation_offset->set_value(Math::rad_to_deg(p_rotation_offset));
		rotation_step->set_value(Math::rad_to_deg(p_rotation_step));
		scale_step->set_value(p_scale_step);
	}

	void get_fields(Point2& p_grid_offset, Point2& p_grid_step, Vector2i& p_primary_grid_step,
		real_t& p_rotation_offset, real_t& p_rotation_step, real_t& p_scale_step)
	{
		p_grid_offset = Point2(grid_offset_x->get_value(), grid_offset_y->get_value());
		p_grid_step = Point2(grid_step_x->get_value(), grid_step_y->get_value());
		p_primary_grid_step =
			Vector2i(primary_grid_step_x->get_value(), primary_grid_step_y->get_value());
		p_rotation_offset = Math::deg_to_rad(rotation_offset->get_value());
		p_rotation_step = Math::deg_to_rad(rotation_step->get_value());
		p_scale_step = scale_step->get_value();
	}
};

void CanvasItemEditor::_snap_if_closer_float(const real_t p_value, real_t& r_current_snap,
	SnapTarget& r_current_snap_target, const real_t p_target_value, const SnapTarget p_snap_target,
	const real_t p_radius)
{
	const real_t radius = p_radius / zoom;
	const real_t dist = Math::abs(p_value - p_target_value);
	if ((p_radius < 0 || dist < radius) &&
		(r_current_snap_target == SNAP_TARGET_NONE || dist < Math::abs(r_current_snap - p_value))) {
		r_current_snap = p_target_value;
		r_current_snap_target = p_snap_target;
	}
}

void CanvasItemEditor::_snap_if_closer_point(Point2 p_value, Point2& r_current_snap,
	SnapTarget (&r_current_snap_target)[2], Point2 p_target_value, const SnapTarget p_snap_target,
	const real_t rotation, const real_t p_radius)
{
	Transform2D rot_trans = Transform2D(rotation, Point2());
	p_value = rot_trans.inverse().xform(p_value);
	p_target_value = rot_trans.inverse().xform(p_target_value);
	r_current_snap = rot_trans.inverse().xform(r_current_snap);

	_snap_if_closer_float(p_value.x, r_current_snap.x, r_current_snap_target[0], p_target_value.x,
		p_snap_target, p_radius);

	_snap_if_closer_float(p_value.y, r_current_snap.y, r_current_snap_target[1], p_target_value.y,
		p_snap_target, p_radius);

	r_current_snap = rot_trans.xform(r_current_snap);
}

real_t CanvasItemEditor::snap_angle(real_t p_target, real_t p_start) const
{
	if (((smart_snap_active || snap_rotation) ^
			Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL)) &&
		snap_rotation_step != 0) {
		if (snap_relative) {
			return Math::snapped(p_target - snap_rotation_offset, snap_rotation_step) +
				   snap_rotation_offset +
				   (p_start - (int)(p_start / snap_rotation_step) * snap_rotation_step);
		}
		else {
			return Math::snapped(p_target - snap_rotation_offset, snap_rotation_step) +
				   snap_rotation_offset;
		}
	}
	else {
		return p_target;
	}
}

void CanvasItemEditor::_keying_changed()
{
	AnimationTrackEditor* te = AnimationPlayerEditor::get_singleton()->get_track_editor();
	if (te && te->is_visible_in_tree() && te->get_current_animation().is_valid()) {
		animation_hb->show();
	}
	else {
		animation_hb->hide();
	}
}

Rect2 CanvasItemEditor::_get_encompassing_rect_from_list(const List<CanvasItem*>& p_list)
{
	ERR_FAIL_COND_V(p_list.is_empty(), Rect2());

	// Handles the first element
	CanvasItem* ci = p_list.front()->get();
	Rect2 rect = Rect2(
		ci->get_global_transform_with_canvas().xform(ci->_edit_get_rect().get_center()), Size2());

	// Expand with the other ones
	for (CanvasItem* ci2 : p_list) {
		Transform2D xform = ci2->get_global_transform_with_canvas();

		Rect2 current_rect = ci2->_edit_get_rect();
		rect.expand_to(xform.xform(current_rect.position));
		rect.expand_to(xform.xform(current_rect.position + Vector2(current_rect.size.x, 0)));
		rect.expand_to(xform.xform(current_rect.position + current_rect.size));
		rect.expand_to(xform.xform(current_rect.position + Vector2(0, current_rect.size.y)));
	}

	return rect;
}

Rect2 CanvasItemEditor::_get_encompassing_rect(const Node* p_node)
{
	Rect2 rect;
	bool first = true;
	_expand_encompassing_rect_using_children(rect, p_node, first);

	return rect;
}

Vector2 CanvasItemEditor::_anchor_to_position(const Control* p_control, Vector2 anchor)
{
	ERR_FAIL_NULL_V(p_control, Vector2());

	Transform2D parent_transform = p_control->get_transform().affine_inverse();
	Rect2 parent_rect = p_control->get_parent_anchorable_rect();

	if (p_control->is_layout_rtl()) {
		return parent_transform.xform(
			parent_rect.position + Vector2(parent_rect.size.x - parent_rect.size.x * anchor.x,
									   parent_rect.size.y * anchor.y));
	}
	else {
		return parent_transform.xform(parent_rect.position + Vector2(parent_rect.size.x * anchor.x,
																 parent_rect.size.y * anchor.y));
	}
}

Vector2 CanvasItemEditor::_position_to_anchor(const Control* p_control, Vector2 position)
{
	ERR_FAIL_NULL_V(p_control, Vector2());

	Rect2 parent_rect = p_control->get_parent_anchorable_rect();

	Vector2 output;
	if (p_control->is_layout_rtl()) {
		output.x = (parent_rect.size.x == 0)
					   ? 0.0
					   : (parent_rect.size.x - p_control->get_transform().xform(position).x -
							 parent_rect.position.x) /
							 parent_rect.size.x;
	}
	else {
		output.x = (parent_rect.size.x == 0)
					   ? 0.0
					   : (p_control->get_transform().xform(position).x - parent_rect.position.x) /
							 parent_rect.size.x;
	}
	output.y = (parent_rect.size.y == 0)
				   ? 0.0
				   : (p_control->get_transform().xform(position).y - parent_rect.position.y) /
						 parent_rect.size.y;
	return output;
}

void CanvasItemEditor::_selection_result_pressed(int p_result)
{
	if (selection_results_menu.size() <= p_result) {
		return;
	}

	CanvasItem* item = selection_results_menu[p_result].item;

	if (item) {
		_select_click_on_item(item, Point2(), selection_menu_additive_selection);
	}
	selection_results_menu.clear();
}

void CanvasItemEditor::_selection_menu_hide()
{
	selection_results.clear();
	selection_menu->clear();
	selection_menu->reset_size();
}

void CanvasItemEditor::_reset_create_position() { node_create_position = Point2(); }

bool CanvasItemEditor::is_grid_visible() const
{
	switch (grid_visibility) {
	case GRID_VISIBILITY_SHOW:
		return true;
	case GRID_VISIBILITY_SHOW_WHEN_SNAPPING:
		return grid_snap_active;
	case GRID_VISIBILITY_HIDE:
		return false;
	}
	ERR_FAIL_V_MSG(true, "Unexpected grid_visibility value");
}

void CanvasItemEditor::_prepare_grid_menu()
{
	for (int i = GRID_VISIBILITY_SHOW; i <= GRID_VISIBILITY_HIDE; i++) {
		grid_menu->set_item_checked(i, i == grid_visibility);
	}
}

bool CanvasItemEditor::_gui_input_zoom_or_pan(
	const Ref<InputEvent>& p_event, bool p_already_accepted)
{
	panner->set_force_drag(tool == TOOL_PAN);
	bool panner_active = panner->gui_input(p_event, viewport->get_global_rect());
	if (panner->is_panning() != pan_pressed) {
		pan_pressed = panner->is_panning();
		_update_cursor();
	}

	if (panner_active) {
		return true;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		if (k->is_pressed()) {
			if (ED_IS_SHORTCUT("canvas_item_editor/zoom_3.125_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 32.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_6.25_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 16.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_12.5_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 8.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_25_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 4.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_50_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 2.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_100_percent", p_event)) {
				_shortcut_zoom_set(1.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_200_percent", p_event)) {
				_shortcut_zoom_set(2.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_400_percent", p_event)) {
				_shortcut_zoom_set(4.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_800_percent", p_event)) {
				_shortcut_zoom_set(8.0);
			}
			else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_1600_percent", p_event)) {
				_shortcut_zoom_set(16.0);
			}
		}
	}

	return false;
}

void CanvasItemEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	view_offset.x -= p_scroll_vec.x / zoom;
	view_offset.y -= p_scroll_vec.y / zoom;
	update_viewport();
}

bool CanvasItemEditor::_gui_input_rotate(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> b = p_event;
	Ref<InputEventMouseMotion> m = p_event;

	// Start rotation
	if (drag_type == DRAG_NONE) {
		if (b.is_valid() && b->get_button_index() == MouseButton::LEFT && b->is_pressed()) {
			if ((b->is_command_or_control_pressed() && !b->is_alt_pressed() &&
					tool == TOOL_SELECT) ||
				tool == TOOL_ROTATE) {
				bool has_locked_items = false;
				List<CanvasItem*> selection =
					_get_edited_canvas_items(false, true, &has_locked_items);

				// Remove not movable nodes
				for (List<CanvasItem*>::Element* E = selection.front(); E;) {
					List<CanvasItem*>::Element* N = E->next();
					if (!_is_node_movable(E->get(), true)) {
						selection.erase(E);
					}
					E = N;
				}

				drag_selection = selection;
				if (drag_selection.size() > 0) {
					drag_type = DRAG_ROTATE;
					drag_from = transform.affine_inverse().xform(b->get_position());
					CanvasItem* ci = drag_selection.front()->get();
					if (!Math::is_inf(temp_pivot.x) || !Math::is_inf(temp_pivot.y)) {
						drag_rotation_center = temp_pivot;
					}
					else if (ci->_edit_use_pivot()) {
						drag_rotation_center =
							ci->get_screen_transform().xform(ci->_edit_get_pivot());
					}
					else {
						drag_rotation_center = ci->get_screen_transform().get_origin();
					}
					_save_canvas_item_state(drag_selection);
					return true;
				}
				else {
					if (has_locked_items) {
						EditorToaster::get_singleton()->popup_str(
							TTR(locked_transform_warning), EditorToaster::SEVERITY_WARNING);
					}
					return has_locked_items;
				}
			}
		}
	}

	if (drag_type == DRAG_ROTATE) {
		// Rotate the node
		if (m.is_valid()) {
			_restore_canvas_item_state(drag_selection);
			for (CanvasItem* ci : drag_selection) {
				drag_to = transform.affine_inverse().xform(m->get_position());
				// Rotate the opposite way if the canvas item's compounded scale has an uneven
				// number of negative elements
				bool opposite = (ci->get_global_transform().get_scale().sign().dot(
									 ci->get_transform().get_scale().sign()) == 0);
				real_t prev_rotation = ci->_edit_get_rotation();
				real_t new_rotation = snap_angle(
					ci->_edit_get_rotation() +
						(opposite ? -1 : 1) * (drag_from - drag_rotation_center)
												  .angle_to(drag_to - drag_rotation_center),
					prev_rotation);

				ci->_edit_set_rotation(new_rotation);
				if (!Math::is_inf(temp_pivot.x) || !Math::is_inf(temp_pivot.y)) {
					Transform2D xform =
						ci->get_screen_transform() * ci->get_transform().affine_inverse();
					Vector2 radius = xform.xform(ci->_edit_get_position()) - temp_pivot;
					radius = radius.rotated(new_rotation - prev_rotation);
					ci->_edit_set_position(xform.affine_inverse().xform(temp_pivot + radius));
				}
				viewport->queue_redraw();
			}
			return true;
		}

		// Confirms the node rotation
		if (b.is_valid() && b->get_button_index() == MouseButton::LEFT && !b->is_pressed()) {
			_commit_drag();
			return true;
		}

		// Cancel a drag
		if (ED_IS_SHORTCUT("canvas_item_editor/cancel_transform", p_event) ||
			(b.is_valid() && b->get_button_index() == MouseButton::RIGHT && b->is_pressed())) {
			_restore_canvas_item_state(drag_selection);
			_reset_drag();
			viewport->queue_redraw();
			return true;
		}
	}
	return false;
}

bool CanvasItemEditor::_gui_input_open_scene_on_double_click(const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> b = p_event;

	// Open a sub-scene on double-click
	if (b.is_valid() && b->get_button_index() == MouseButton::LEFT && b->is_pressed() &&
		b->is_double_click() && tool == TOOL_SELECT) {
		List<CanvasItem*> selection = _get_edited_canvas_items();
		if (selection.size() == 1) {
			CanvasItem* ci = selection.front()->get();
			if (ci->is_instance() && ci != EditorNode::get_singleton()->get_edited_scene()) {
				EditorNode::get_singleton()->open_scene(ci->get_scene_file_path());
				return true;
			}
		}
	}
	return false;
}

bool CanvasItemEditor::_gui_input_ruler_tool(const Ref<InputEvent>& p_event)
{
	if (tool != TOOL_RULER) {
		ruler_tool_active = false;
		return false;
	}

	Ref<InputEventMouseButton> b = p_event;
	Ref<InputEventMouseMotion> m = p_event;

	Point2 previous_origin = ruler_tool_origin;
	if (!ruler_tool_active) {
		ruler_tool_origin = snap_point(viewport->get_local_mouse_position() / zoom + view_offset);
	}

	if (ruler_tool_active && b.is_valid() && b->get_button_index() == MouseButton::RIGHT) {
		ruler_tool_active = false;
		viewport->queue_redraw();
		return true;
	}

	if (b.is_valid() && b->get_button_index() == MouseButton::LEFT) {
		if (b->is_pressed()) {
			ruler_tool_active = true;
		}
		else {
			ruler_tool_active = false;
		}

		viewport->queue_redraw();
		return true;
	}

	if (m.is_valid() &&
		(ruler_tool_active || (grid_snap_active && previous_origin != ruler_tool_origin))) {
		viewport->queue_redraw();
		return true;
	}

	return false;
}

void CanvasItemEditor::_update_cursor()
{
	if (cursor_shape_override != CURSOR_ARROW) {
		set_default_cursor_shape(cursor_shape_override);
		return;
	}

	// Choose the correct default cursor.
	CursorShape c = CURSOR_ARROW;
	switch (tool) {
	case TOOL_MOVE:
		c = CURSOR_MOVE;
		break;
	case TOOL_EDIT_PIVOT:
		c = CURSOR_CROSS;
		break;
	case TOOL_PAN:
		c = CURSOR_DRAG;
		break;
	case TOOL_RULER:
		c = CURSOR_CROSS;
		break;
	default:
		break;
	}
	if (pan_pressed) {
		c = CURSOR_DRAG;
	}
	set_default_cursor_shape(c);
}

void CanvasItemEditor::set_cursor_shape_override(CursorShape p_shape)
{
	if (cursor_shape_override == p_shape) {
		return;
	}
	cursor_shape_override = p_shape;
	_update_cursor();
}

Control::CursorShape CanvasItemEditor::get_cursor_shape(const Point2& p_pos) const
{
	// Compute an eventual rotation of the cursor
	const CursorShape rotation_array[4] = {
		CURSOR_HSIZE, CURSOR_BDIAGSIZE, CURSOR_VSIZE, CURSOR_FDIAGSIZE};
	int rotation_array_index = 0;

	List<CanvasItem*> selection = _get_edited_canvas_items();
	if (selection.size() == 1) {
		const double angle = Math::fposmod(
			(double)selection.front()->get()->get_global_transform_with_canvas().get_rotation(),
			Math::PI);
		if (angle > Math::PI * 7.0 / 8.0) {
			rotation_array_index = 0;
		}
		else if (angle > Math::PI * 5.0 / 8.0) {
			rotation_array_index = 1;
		}
		else if (angle > Math::PI * 3.0 / 8.0) {
			rotation_array_index = 2;
		}
		else if (angle > Math::PI * 1.0 / 8.0) {
			rotation_array_index = 3;
		}
		else {
			rotation_array_index = 0;
		}
	}

	// Choose the correct cursor
	CursorShape c = get_default_cursor_shape();
	switch (drag_type) {
	case DRAG_LEFT:
	case DRAG_RIGHT:
		c = rotation_array[rotation_array_index];
		break;
	case DRAG_V_GUIDE:
		c = CURSOR_HSIZE;
		break;
	case DRAG_TOP:
	case DRAG_BOTTOM:
		c = rotation_array[(rotation_array_index + 2) % 4];
		break;
	case DRAG_H_GUIDE:
		c = CURSOR_VSIZE;
		break;
	case DRAG_TOP_LEFT:
	case DRAG_BOTTOM_RIGHT:
		c = rotation_array[(rotation_array_index + 3) % 4];
		break;
	case DRAG_DOUBLE_GUIDE:
		c = CURSOR_FDIAGSIZE;
		break;
	case DRAG_TOP_RIGHT:
	case DRAG_BOTTOM_LEFT:
		c = rotation_array[(rotation_array_index + 1) % 4];
		break;
	case DRAG_MOVE:
		c = CURSOR_MOVE;
		break;
	default:
		break;
	}

	if (is_hovering_h_guide) {
		c = CURSOR_VSIZE;
	}
	else if (is_hovering_v_guide) {
		c = CURSOR_HSIZE;
	}

	if (pan_pressed) {
		c = CURSOR_DRAG;
	}
	return c;
}

void CanvasItemEditor::_draw_text_at_position(
	Point2 p_position, const String& p_string, Side p_side)
{
	Color color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	color.a = 0.8;
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	Size2 text_size = font->get_string_size(p_string, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
	switch (p_side) {
	case SIDE_LEFT:
		p_position += Vector2(-text_size.x - 5, text_size.y / 2);
		break;
	case SIDE_TOP:
		p_position += Vector2(-text_size.x / 2, -5);
		break;
	case SIDE_RIGHT:
		p_position += Vector2(5, text_size.y / 2);
		break;
	case SIDE_BOTTOM:
		p_position += Vector2(-text_size.x / 2, text_size.y + 5);
		break;
	}
	viewport->draw_string(
		font.ptr(), p_position, p_string, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color);
}

void CanvasItemEditor::_draw_focus()
{
	// Draw the focus around the base viewport
	if (viewport->has_focus()) {
		get_theme_stylebox(SNAME("FocusViewport"), EditorStringName(EditorStyles))
			->draw(viewport->get_canvas_item(), Rect2(Point2(), viewport->get_size()));
	}
}

void CanvasItemEditor::_draw_straight_line(Point2 p_from, Point2 p_to, Color p_color)
{
	// Draw a line going through the whole screen from a vector
	RID ci = viewport->get_canvas_item();
	Vector<Point2> points;
	Point2 from = transform.xform(p_from);
	Point2 to = transform.xform(p_to);
	Size2 viewport_size = viewport->get_size();

	if (to.x == from.x) {
		// Vertical line
		points.push_back(Point2(to.x, 0));
		points.push_back(Point2(to.x, viewport_size.y));
	}
	else if (to.y == from.y) {
		// Horizontal line
		points.push_back(Point2(0, to.y));
		points.push_back(Point2(viewport_size.x, to.y));
	}
	else {
		real_t y_for_zero_x = (to.y * from.x - from.y * to.x) / (from.x - to.x);
		real_t x_for_zero_y = (to.x * from.y - from.x * to.y) / (from.y - to.y);
		real_t y_for_viewport_x =
			((to.y - from.y) * (viewport_size.x - from.x)) / (to.x - from.x) + from.y;
		real_t x_for_viewport_y =
			((to.x - from.x) * (viewport_size.y - from.y)) / (to.y - from.y) + from.x; // faux

		// bool start_set = false;
		if (y_for_zero_x >= 0 && y_for_zero_x <= viewport_size.y) {
			points.push_back(Point2(0, y_for_zero_x));
		}
		if (x_for_zero_y >= 0 && x_for_zero_y <= viewport_size.x) {
			points.push_back(Point2(x_for_zero_y, 0));
		}
		if (y_for_viewport_x >= 0 && y_for_viewport_x <= viewport_size.y) {
			points.push_back(Point2(viewport_size.x, y_for_viewport_x));
		}
		if (x_for_viewport_y >= 0 && x_for_viewport_y <= viewport_size.x) {
			points.push_back(Point2(x_for_viewport_y, viewport_size.y));
		}
	}
	if (points.size() >= 2) {
		RenderingServer::get_singleton()->canvas_item_add_line(ci, points[0], points[1], p_color);
	}
}

void CanvasItemEditor::_draw_hover()
{
	List<Rect2> previous_rects;
	Vector2 icon_size =
		Vector2(1, 1) * get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

	for (int i = 0; i < hovering_results.size(); i++) {
		Ref<Texture2D> node_icon = hovering_results[i].icon;
		String node_name = hovering_results[i].name;

		Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
		Size2 node_name_size =
			font->get_string_size(node_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		Size2 item_size =
			Size2(icon_size.x + 4 + node_name_size.x, MAX(icon_size.y, node_name_size.y - 3));

		Point2 pos = transform.xform(hovering_results[i].position) - Point2(0, item_size.y) +
					 (Point2(icon_size.x, -icon_size.y) / 4);
		// Rectify the position to avoid overlapping items
		for (const Rect2& E : previous_rects) {
			if (E.intersects(Rect2(pos, item_size))) {
				pos.y = E.get_position().y - item_size.y;
			}
		}

		previous_rects.push_back(Rect2(pos, item_size));

		// Draw icon
		viewport->draw_texture_rect(
			node_icon.ptr(), Rect2(pos, icon_size), false, Color(1.0, 1.0, 1.0, 0.5));

		// Draw name
		viewport->draw_string(font.ptr(), pos + Point2(icon_size.x + 4, item_size.y - 3), node_name,
			HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1.0, 1.0, 1.0, 0.5));
	}
}

void CanvasItemEditor::_draw_viewport()
{
	// Update the transform
	transform = Transform2D();
	transform.scale_basis(Size2(zoom, zoom));
	transform.columns[2] = -view_offset * zoom;
	EditorNode::get_singleton()->get_scene_root()->set_global_canvas_transform(transform);

	_draw_grid();
	_draw_ruler_tool();
	_draw_axis();
	if (EditorNode::get_singleton()->get_edited_scene()) {
		_draw_locks_and_groups(EditorNode::get_singleton()->get_edited_scene());
		_draw_invisible_nodes_positions(EditorNode::get_singleton()->get_edited_scene());
	}
	_draw_selection();

	RID ci = viewport->get_canvas_item();
	RenderingServer::get_singleton()->canvas_item_add_set_transform(ci, Transform2D());

	EditorNode::get_singleton()->get_editor_plugins_over()->forward_canvas_draw_over_viewport(
		viewport);
	EditorNode::get_singleton()
		->get_editor_plugins_force_over()
		->forward_canvas_force_draw_over_viewport(viewport);

	if (show_rulers) {
		_draw_rulers();
	}
	if (show_guides) {
		_draw_guides();
	}
	_draw_smart_snapping();
	_draw_focus();
	_draw_hover();
	_draw_message();
}

void CanvasItemEditor::update_viewport()
{
	_update_scrollbars();
	viewport->queue_redraw();
}

void CanvasItemEditor::set_current_tool(Tool p_tool) { _button_tool_select(p_tool); }

void CanvasItemEditor::edit(CanvasItem* p_canvas_item)
{
	if (!p_canvas_item) {
		return;
	}

	List<Node*> selection = editor_selection->get_full_selected_node_list();
	if (selection.size() != 1) {
		_reset_drag();
	}
}

void CanvasItemEditor::_update_scroll(real_t)
{
	if (updating_scroll) {
		return;
	}

	view_offset.x = h_scroll->get_value();
	view_offset.y = v_scroll->get_value();
	viewport->queue_redraw();
}

void CanvasItemEditor::_zoom_on_position(real_t p_zoom, Point2 p_position)
{
	p_zoom = CLAMP(p_zoom, zoom_widget->get_min_zoom(), zoom_widget->get_max_zoom());

	if (p_zoom == zoom) {
		return;
	}

	real_t prev_zoom = zoom;
	zoom = p_zoom;

	view_offset += p_position / prev_zoom - p_position / zoom;

	// We want to align in-scene pixels to screen pixels, this prevents blurry rendering
	// of small details (texts, lines).
	// This correction adds a jitter movement when zooming, so we correct only when the
	// zoom factor is an integer. (in the other cases, all pixels won't be aligned anyway)
	const real_t closest_zoom_factor = Math::round(zoom);
	if (Math::is_zero_approx(zoom - closest_zoom_factor)) {
		// Make sure scene pixel at view_offset is aligned on a screen pixel.
		Vector2 view_offset_int = view_offset.floor();
		Vector2 view_offset_frac = view_offset - view_offset_int;
		view_offset = view_offset_int +
					  (view_offset_frac * closest_zoom_factor).round() / closest_zoom_factor;
	}

	zoom_widget->set_zoom(zoom);
	update_viewport();
	if (auto_resampling_enabled) {
		resample_timer->start();
	}
}

void CanvasItemEditor::_update_zoom(real_t p_zoom)
{
	_zoom_on_position(p_zoom, viewport_scrollable->get_size() / 2.0);
}

void CanvasItemEditor::_update_oversampling()
{
	EditorNode::get_singleton()->get_scene_root()->set_oversampling_override(
		auto_resampling_enabled ? zoom : 0.0);
}

void CanvasItemEditor::_shortcut_zoom_set(real_t p_zoom)
{
	_zoom_on_position(p_zoom * MAX(1, EDSCALE), viewport->get_local_mouse_position());
}

void CanvasItemEditor::_button_toggle_local_space(bool p_status)
{
	use_local_space = p_status;
	viewport->queue_redraw();
}

void CanvasItemEditor::_button_toggle_smart_snap(bool p_status)
{
	smart_snap_active = p_status;
	viewport->queue_redraw();
}

void CanvasItemEditor::_button_toggle_grid_snap(bool p_status)
{
	grid_snap_active = p_status;
	viewport->queue_redraw();
}

void CanvasItemEditor::_button_tool_select(int p_index)
{
	if (drag_type != DRAG_NONE) {
		_commit_drag();
	}

	Button* tb[TOOL_MAX] = {select_button, scene_paint_button, list_select_button, move_button,
		scale_button, rotate_button, pivot_button, pan_button, ruler_button};
	for (int i = 0; i < TOOL_MAX; i++) {
		tb[i]->set_pressed(i == p_index);
	}

	tool = (Tool)p_index;

	if (p_index == TOOL_EDIT_PIVOT && Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		// Special action that places temporary rotation pivot in the middle of the selection.
		List<CanvasItem*> selection = _get_edited_canvas_items();
		if (!selection.is_empty()) {
			Vector2 center;
			for (const CanvasItem* ci : selection) {
				center +=
					ci->get_viewport()->get_popup_base_transform().xform(ci->_edit_get_position());
			}
			temp_pivot = center / selection.size();
		}
	}

	viewport->queue_redraw();
	_update_cursor();
}

void CanvasItemEditor::_prepare_view_menu()
{
	PopupMenu* popup = view_menu->get_popup();

	Node* root = EditorNode::get_singleton()->get_edited_scene();
	bool has_guides = root && (root->has_meta("_edit_horizontal_guides_") ||
								  root->has_meta("_edit_vertical_guides_"));
	popup->set_item_disabled(popup->get_item_index(CLEAR_GUIDES), !has_guides);
}

void CanvasItemEditor::_set_owner_for_node_and_children(Node* p_node, Node* p_owner)
{
	p_node->set_owner(p_owner);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_set_owner_for_node_and_children(p_node->get_child(i), p_owner);
	}
}

void CanvasItemEditor::_reset_drag()
{
	message = "";
	drag_type = DRAG_NONE;
	drag_selection.clear();
}

void CanvasItemEditor::add_control_to_left_panel(Control* p_control)
{
	left_panel_split->add_child(p_control);
	left_panel_split->move_child(p_control, 0);
}

void CanvasItemEditor::add_control_to_right_panel(Control* p_control)
{
	right_panel_split->add_child(p_control);
	right_panel_split->move_child(p_control, 1);
}

void CanvasItemEditor::remove_control_from_left_panel(Control* p_control)
{
	left_panel_split->remove_child(p_control);
}

void CanvasItemEditor::remove_control_from_right_panel(Control* p_control)
{
	right_panel_split->remove_child(p_control);
}

VSplitContainer* CanvasItemEditor::get_bottom_split() { return bottom_split; }

void CanvasItemEditor::focus_selection() { _focus_selection(VIEW_CENTER_TO_SELECTION); }

void CanvasItemEditor::center_at(const Point2& p_pos)
{
	Vector2 offset =
		viewport->get_size() / 2 -
		EditorNode::get_singleton()->get_scene_root()->get_global_canvas_transform().xform(p_pos);
	view_offset = (view_offset - offset / zoom).round();
	update_viewport();
}

CanvasItemEditor* CanvasItemEditor::singleton = nullptr;

CanvasItemEditorPlugin::CanvasItemEditorPlugin()
{
	canvas_item_editor = memnew(CanvasItemEditor);
	canvas_item_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	EditorNode::get_singleton()->get_editor_main_screen()->get_control()->add_child(
		canvas_item_editor);
	canvas_item_editor->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_item_editor->hide();
}

void CanvasItemEditorViewport::_on_mouse_exit()
{
	if (!texture_node_type_selector->is_visible() && preview_node->get_parent()) {
		_remove_preview();
	}
}

void CanvasItemEditorViewport::_on_change_type_closed() { _remove_preview(); }

void CanvasItemEditorViewport::_remove_preview()
{
	if (!canvas_item_editor->message.is_empty()) {
		canvas_item_editor->message = "";
		canvas_item_editor->update_viewport();
	}
	tooltip_panel->hide();
	if (preview_node->get_parent()) {
		for (int i = preview_node->get_child_count() - 1; i >= 0; i--) {
			Node* node = preview_node->get_child(i);
			node->queue_free();
			preview_node->remove_child(node);
		}
		EditorNode::get_singleton()->get_scene_root()->remove_child(preview_node);
	}
}

bool CanvasItemEditorViewport::_cyclical_dependency_exists(
	const String& p_target_scene_path, Node* p_desired_node) const
{
	if (p_desired_node->get_scene_file_path() == p_target_scene_path) {
		return true;
	}

	int childCount = p_desired_node->get_child_count();
	for (int i = 0; i < childCount; i++) {
		Node* child = p_desired_node->get_child(i);
		if (_cyclical_dependency_exists(p_target_scene_path, child)) {
			return true;
		}
	}
	return false;
}

void CanvasItemEditorViewport::set_hint_label(
	const String& p_title, const String& p_description) const
{
	if (p_title.is_empty() && p_description.is_empty()) {
		tooltip_panel->hide();
		return;
	}

	tooltip_panel->set_text(vformat("[font_size=%s][b][color=%s]%s[/color][/b][/font_size]\n%s",
		get_theme_default_font_size() + 2,
		get_theme_color(SNAME("accent_color"), EditorStringName(Editor)).to_html(false), p_title,
		p_description));
	tooltip_panel->show();
}

CanvasItemEditorViewport::~CanvasItemEditorViewport() { memdelete(preview_node); }


