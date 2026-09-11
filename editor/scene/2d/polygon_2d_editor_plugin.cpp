/**************************************************************************/
/*  polygon_2d_editor_plugin.cpp                                          */
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

#include "core/input/input_event.h"
#include "core/math/geometry_2d.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "polygon_2d_editor_plugin.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/view_panner.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

Node2D* Polygon2DEditor::_get_node() const { return node; }

Vector2 Polygon2DEditor::_get_offset(int p_idx) const { return node->get_offset(); }

int Polygon2DEditor::_get_polygon_count() const
{
	if (node->get_internal_vertex_count() > 0) {
		return 0; // do not edit if internal vertices exist
	}
	else {
		return 1;
	}
}

void Polygon2DEditor::_bone_paint_selected(int p_index) { canvas->queue_redraw(); }

void Polygon2DEditor::_select_mode(int p_mode)
{
	current_mode = Mode(p_mode);
	mode_buttons[current_mode]->set_pressed(true);
	for (int i = 0; i < ACTION_MAX; i++) {
		action_buttons[i]->hide();
	}
	bone_scroll_main_vb->hide();
	bone_paint_strength->hide();
	bone_paint_radius->hide();
	bone_paint_radius_label->hide();
	switch (current_mode) {
	case MODE_POINTS: {
		action_buttons[ACTION_CREATE]->show();
		action_buttons[ACTION_CREATE_INTERNAL]->show();
		action_buttons[ACTION_REMOVE_INTERNAL]->show();
		action_buttons[ACTION_EDIT_POINT]->show();
		action_buttons[ACTION_MOVE]->show();
		action_buttons[ACTION_ROTATE]->show();
		action_buttons[ACTION_SCALE]->show();

		if (node->get_polygon().is_empty()) {
			_set_action(ACTION_CREATE);
		}
		else {
			_set_action(ACTION_EDIT_POINT);
		}
	} break;
	case MODE_POLYGONS: {
		action_buttons[ACTION_ADD_POLYGON]->show();
		action_buttons[ACTION_REMOVE_POLYGON]->show();
		_set_action(ACTION_ADD_POLYGON);
	} break;
	case MODE_UV: {
		if (node->get_uv().size() != node->get_polygon().size()) {
			_edit_menu_option(MENU_POLYGON_TO_UV);
		}
		action_buttons[ACTION_EDIT_POINT]->show();
		action_buttons[ACTION_MOVE]->show();
		action_buttons[ACTION_ROTATE]->show();
		action_buttons[ACTION_SCALE]->show();
		_set_action(ACTION_EDIT_POINT);
	} break;
	case MODE_BONES: {
		action_buttons[ACTION_PAINT_WEIGHT]->show();
		action_buttons[ACTION_CLEAR_WEIGHT]->show();
		_set_action(ACTION_PAINT_WEIGHT);

		bone_scroll_main_vb->show();
		bone_paint_strength->show();
		bone_paint_radius->show();
		bone_paint_radius_label->show();
		_update_bone_list(node);
		bone_paint_pos = Vector2(-100000, -100000); // Send brush away when switching.
	} break;
	default:
		break;
	}
	canvas->queue_redraw();
}

void Polygon2DEditor::_update_polygon_editing_state()
{
	if (!_get_node()) {
		return;
	}

	if (node->get_internal_vertex_count() > 0) {
		disable_polygon_editing(true, TTR("Polygon 2D has internal vertices, so it can no longer "
										  "be edited in the viewport."));
	}
	else {
		disable_polygon_editing(false, String());
	}
}

void Polygon2DEditor::_set_action(int p_action)
{
	polygon_create.clear();
	is_dragging = false;
	is_creating = false;

	selected_action = Action(p_action);
	for (int i = 0; i < ACTION_MAX; i++) {
		action_buttons[i]->set_pressed(p_action == i);
	}
	canvas->queue_redraw();
}

void Polygon2DEditor::_update_available_modes()
{
	// Force point editing mode if there's no polygon yet.
	if (node->get_polygon().is_empty()) {
		if (current_mode != MODE_POINTS) {
			_select_mode(MODE_POINTS);
		}
		mode_buttons[MODE_UV]->set_disabled(true);
		mode_buttons[MODE_POLYGONS]->set_disabled(true);
		mode_buttons[MODE_BONES]->set_disabled(true);
	}
	else {
		mode_buttons[MODE_UV]->set_disabled(false);
		mode_buttons[MODE_POLYGONS]->set_disabled(false);
		mode_buttons[MODE_BONES]->set_disabled(false);
	}
}

void Polygon2DEditor::_center_view()
{
	Size2 texture_size;
	if (node->get_texture().is_valid()) {
		texture_size = node->get_texture()->get_size();
		Vector2 zoom_factor = (canvas->get_size() - Vector2(1, 1) * 50 * EDSCALE) / texture_size;
		zoom_widget->set_zoom(MIN(zoom_factor.x, zoom_factor.y));
	}
	else {
		zoom_widget->set_zoom(EDSCALE);
	}
	// Recalculate scroll limits.
	_update_zoom_and_pan(false);

	Size2 offset = (texture_size - canvas->get_size() / draw_zoom) / 2;
	hscroll->set_value_no_signal(offset.x);
	vscroll->set_value_no_signal(offset.y);
	_update_zoom_and_pan(false);
}

void Polygon2DEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	hscroll->set_value_no_signal(hscroll->get_value() - p_scroll_vec.x / draw_zoom);
	vscroll->set_value_no_signal(vscroll->get_value() - p_scroll_vec.y / draw_zoom);
	_update_zoom_and_pan(false);
}

void Polygon2DEditor::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	zoom_widget->set_zoom(draw_zoom * p_zoom_factor);
	draw_offset += p_origin / draw_zoom - p_origin / zoom_widget->get_zoom();
	hscroll->set_value_no_signal(draw_offset.x);
	vscroll->set_value_no_signal(draw_offset.y);
	_update_zoom_and_pan(false);
}

Vector2 Polygon2DEditor::snap_point(Vector2 p_target) const
{
	if (use_snap) {
		p_target.x = Math::snap_scalar(
			(snap_offset.x - draw_offset.x) * draw_zoom, snap_step.x * draw_zoom, p_target.x);
		p_target.y = Math::snap_scalar(
			(snap_offset.y - draw_offset.y) * draw_zoom, snap_step.y * draw_zoom, p_target.y);
	}

	return p_target;
}

Polygon2DEditorPlugin::Polygon2DEditorPlugin()
	: AbstractPolygon2DEditorPlugin(memnew(Polygon2DEditor), "Polygon2D")
{
}


