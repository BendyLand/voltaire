/**************************************************************************/
/*  path_2d_editor_plugin.cpp                                             */
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

#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "path_2d_editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server.h"

void Path2DEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		curve_edit->set_button_icon(get_editor_theme_icon(SNAME("CurveEdit")));
		curve_edit_curve->set_button_icon(get_editor_theme_icon(SNAME("CurveCurve")));
		curve_create->set_button_icon(get_editor_theme_icon(SNAME("CurveCreate")));
		curve_del->set_button_icon(get_editor_theme_icon(SNAME("CurveDelete")));
		curve_close->set_button_icon(get_editor_theme_icon(SNAME("CurveClose")));
		curve_clear_points->set_button_icon(get_editor_theme_icon(SNAME("Clear")));

		create_curve_button->set_button_icon(get_editor_theme_icon(SNAME("Curve2D")));
	} break;
	}
}

void Path2DEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
		hide();
	}
}

void Path2DEditor::_node_visibility_changed()
{
	if (!node) {
		return;
	}

	canvas_item_editor->update_viewport();
	_update_toolbar();
}

void Path2DEditor::_update_toolbar()
{
	if (!node) {
		return;
	}
	bool has_curve = node->get_curve().is_valid();
	toolbar->set_visible(has_curve);
	create_curve_button->set_visible(!has_curve);
}

void Path2DEditor::_handle_option_pressed(int p_option)
{
	PopupMenu* pm;
	pm = handle_menu->get_popup();

	switch (p_option) {
	case HANDLE_OPTION_ANGLE: {
		bool is_checked = pm->is_item_checked(HANDLE_OPTION_ANGLE);
		mirror_handle_angle = !is_checked;
		pm->set_item_checked(HANDLE_OPTION_ANGLE, mirror_handle_angle);
		pm->set_item_disabled(HANDLE_OPTION_LENGTH, !mirror_handle_angle);
	} break;
	case HANDLE_OPTION_LENGTH: {
		bool is_checked = pm->is_item_checked(HANDLE_OPTION_LENGTH);
		mirror_handle_length = !is_checked;
		pm->set_item_checked(HANDLE_OPTION_LENGTH, mirror_handle_length);
	} break;
	}
}

void Path2DEditor::_cancel_current_action()
{
	ERR_FAIL_NULL(node);
	Ref<Curve2D> curve = node->get_curve();
	ERR_FAIL_COND(curve.is_null());

	switch (action) {
	case ACTION_MOVING_POINT: {
		curve->set_point_position(action_point, moving_from);
	} break;

	case ACTION_MOVING_NEW_POINT: {
		curve->remove_point(curve->get_point_count() - 1);
	} break;

	case ACTION_MOVING_NEW_POINT_FROM_SPLIT: {
		curve->remove_point(action_point);
	} break;

	case ACTION_MOVING_IN: {
		curve->set_point_in(action_point, moving_from);
		curve->set_point_out(action_point,
			mirror_handle_length ? -moving_from : (-moving_from.normalized() * orig_out_length));
	} break;

	case ACTION_MOVING_OUT: {
		curve->set_point_out(action_point, moving_from);
		curve->set_point_in(action_point,
			mirror_handle_length ? -moving_from : (-moving_from.normalized() * orig_in_length));
	} break;

	default: {
	}
	}

	canvas_item_editor->update_viewport();
	action = ACTION_NONE;
}

void Path2DEditor::_confirm_clear_points()
{
	if (!node || node->get_curve().is_null()) {
		return;
	}
	if (node->get_curve()->get_point_count() == 0) {
		return;
	}
	clear_points_dialog->reset_size();
	clear_points_dialog->popup_centered();
}

void Path2DEditor::_clear_curve_points(Path2D* p_path2d)
{
	if (!p_path2d || p_path2d->get_curve().is_null()) {
		return;
	}
	Ref<Curve2D> curve = p_path2d->get_curve();

	if (curve->get_point_count() == 0) {
		return;
	}
	curve->clear_points();

	if (node == p_path2d) {
		_mode_selected(MODE_CREATE);
	}
}

void Path2DEditor::_restore_curve_points(Path2D* p_path2d, const PackedVector2Array& p_points)
{
	if (!p_path2d || p_path2d->get_curve().is_null()) {
		return;
	}
	Ref<Curve2D> curve = p_path2d->get_curve();

	if (curve->get_point_count() > 0) {
		curve->clear_points();
	}

	for (int i = 0; i < p_points.size(); i += 3) {
		curve->add_point(p_points[i + 2], p_points[i],
			p_points[i +
					 1]); // The Curve2D::points pattern is [point_in, point_out, point_position].
	}

	if (node == p_path2d) {
		_mode_selected(MODE_EDIT);
	}
}

Path2DEditor::~Path2DEditor()
{
	ERR_FAIL_NULL(RS::get_singleton());
	RS::get_singleton()->free_rid(debug_mesh_rid);
	RS::get_singleton()->free_rid(debug_handle_curve_multimesh_rid);
	RS::get_singleton()->free_rid(debug_handle_sharp_multimesh_rid);
	RS::get_singleton()->free_rid(debug_handle_smooth_multimesh_rid);
	RS::get_singleton()->free_rid(debug_handle_mesh_rid);
}

Path2DEditorPlugin::Path2DEditorPlugin()
{
	path2d_editor = memnew(Path2DEditor);
	CanvasItemEditor::get_singleton()->add_control_to_menu_panel(path2d_editor);
	path2d_editor->hide();
}


