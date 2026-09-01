/**************************************************************************/
/*  path_3d_editor_plugin.cpp                                             */
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

#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/settings/editor_settings.h"
#include "path_3d_editor_plugin.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/curve.h"

String Path3DGizmo::get_handle_name(int p_id, bool p_secondary) const
{
	Ref<Curve3D> c = path->get_curve();
	if (c.is_null()) {
		return "";
	}

	// Primary handles: position.
	if (!p_secondary) {
		return TTR("Curve Point #") + itos(p_id);
	}

	// Secondary handles: in, out, tilt.
	const HandleInfo info = _secondary_handles_info[p_id];
	switch (info.type) {
	case HandleType::HANDLE_TYPE_IN:
		return TTR("Handle In #") + itos(info.point_idx);
	case HandleType::HANDLE_TYPE_OUT:
		return TTR("Handle Out #") + itos(info.point_idx);
	case HandleType::HANDLE_TYPE_TILT:
		return TTR("Handle Tilt #") + itos(info.point_idx);
	}

	return "";
}

void Path3DGizmo::set_handle(int p_id, bool p_secondary, Camera3D* p_camera, const Point2& p_point)
{
	Ref<Curve3D> c = path->get_curve();
	if (c.is_null()) {
		return;
	}

	const Transform3D gt = path->get_global_transform();
	const Transform3D gi = gt.affine_inverse();
	const Vector3 ray_from = p_camera->project_ray_origin(p_point);
	const Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	const Plane p = Plane(p_camera->get_transform().basis.get_column(2), gt.xform(original));

	// Primary handles: position.
	if (!p_secondary) {
		Vector3 inters;
		// Special case for primary handle, the handle id equals control point id.
		const int idx = p_id;
		if (!Path3DEditorPlugin::singleton->_edit.waiting_handle_physics ||
			!Path3DEditorPlugin::singleton->_edit.in_physics_frame) {
			Path3DEditorPlugin::singleton->_edit.waiting_handle_physics = true;
			Path3DEditorPlugin::singleton->_edit.gizmo_handle = p_id;
			Path3DEditorPlugin::singleton->_edit.gizmo_handle_secondary = p_secondary;
			Path3DEditorPlugin::singleton->_edit.gizmo_camera = p_camera;
			Path3DEditorPlugin::singleton->_edit.mouse_pos = p_point;
			return;
			// Only continue if inside physics frame and waiting for physics.
		}
		if (Path3DEditorPlugin::singleton->snap_to_collider) {
			PhysicsDirectSpaceState3D* ss = p_camera->get_world_3d()->get_direct_space_state();

			PS3DT::RayParameters ray_params;
			ray_params.from = ray_from;
			ray_params.to = ray_from + ray_dir * p_camera->get_far();
			PS3DT::RayResult result;
			if (ss->intersect_ray(ray_params, result)) {
				Vector3 local = gi.xform(result.position);
				c->set_point_position(idx, local);
				return;
			}
			// Will continue and do the plane intersect_ray if doesn't hit anything.
		}
		if (p.intersects_ray(ray_from, ray_dir, &inters)) {
			if (Node3DEditor::get_singleton()->is_snap_enabled()) {
				float snap = Node3DEditor::get_singleton()->get_translate_snap();
				inters.snapf(snap);
			}

			Vector3 local = gi.xform(inters);
			c->set_point_position(idx, local);
		}

		return;
	}

	// Secondary handles: in, out, tilt.
	const HandleInfo info = _secondary_handles_info[p_id];
	switch (info.type) {
	case HandleType::HANDLE_TYPE_OUT:
	case HandleType::HANDLE_TYPE_IN: {
		const int idx = info.point_idx;
		const Vector3 base = c->get_point_position(idx);

		Vector3 inters;
		if (p.intersects_ray(ray_from, ray_dir, &inters)) {
			if (!Path3DEditorPlugin::singleton->is_handle_clicked()) {
				orig_in_length = c->get_point_in(idx).length();
				orig_out_length = c->get_point_out(idx).length();
				Path3DEditorPlugin::singleton->set_handle_clicked(true);
			}

			Vector3 local = gi.xform(inters) - base;
			if (Node3DEditor::get_singleton()->is_snap_enabled()) {
				float snap = Node3DEditor::get_singleton()->get_translate_snap();
				local.snapf(snap);
			}

			// Determine if control points should be swapped based on delta movement.
			// Only run on the next update after an overlap is detected, to get proper delta
			// movement.
			if (control_points_overlapped) {
				control_points_overlapped = false;
				Vector3 delta = local - (info.type == HANDLE_TYPE_IN ? c->get_point_in(idx)
																	 : c->get_point_out(idx));
				Vector3 p0 = c->get_point_position(idx - 1) - base;
				Vector3 p1 = c->get_point_position(idx + 1) - base;
				HandleType new_type = Math::abs(delta.angle_to(p0)) < Math::abs(delta.angle_to(p1))
										  ? HANDLE_TYPE_IN
										  : HANDLE_TYPE_OUT;
				if (info.type != new_type) {
					swapped_control_points_idx = idx;
				}
			}

			// Detect control points overlap.
			bool control_points_equal = c->get_point_in(idx).is_equal_approx(c->get_point_out(idx));
			if (idx > 0 && idx < (c->get_point_count() - 1) && control_points_equal) {
				control_points_overlapped = true;
			}

			HandleType control_type = info.type;
			if (swapped_control_points_idx == idx) {
				control_type = info.type == HANDLE_TYPE_IN ? HANDLE_TYPE_OUT : HANDLE_TYPE_IN;
			}

			if (control_type == HandleType::HANDLE_TYPE_IN) {
				c->set_point_in(idx, local);
				if (Path3DEditorPlugin::singleton->mirror_angle_enabled()) {
					c->set_point_out(idx, Path3DEditorPlugin::singleton->mirror_length_enabled()
											  ? -local
											  : (-local.normalized() * orig_out_length));
				}
			}
			else {
				c->set_point_out(idx, local);
				if (Path3DEditorPlugin::singleton->mirror_angle_enabled()) {
					c->set_point_in(idx, Path3DEditorPlugin::singleton->mirror_length_enabled()
											 ? -local
											 : (-local.normalized() * orig_in_length));
				}
			}
		}
		break;
	}
	case HandleType::HANDLE_TYPE_TILT: {
		const int idx = info.point_idx;
		const Vector3 position = c->get_point_position(idx);
		const Basis posture = c->get_point_baked_posture(idx);
		const Vector3 tangent = -posture.get_column(2);
		const Vector3 up = posture.get_column(1);
		const Plane tilt_plane_global = gt.xform(Plane(tangent, position));

		Vector3 intersection;

		if (tilt_plane_global.intersects_ray(ray_from, ray_dir, &intersection)) {
			Vector3 direction = gi.xform(intersection) - position;
			real_t tilt_angle = up.signed_angle_to(direction, tangent);

			if (Node3DEditor::get_singleton()->is_snap_enabled()) {
				real_t snap_degrees = Node3DEditor::get_singleton()->get_rotate_snap();
				tilt_angle =
					Math::deg_to_rad(Math::snapped(Math::rad_to_deg(tilt_angle), snap_degrees));
			}

			c->set_point_tilt(idx, tilt_angle);
		}
		break;
	}
	}
}

void Path3DGizmo::_update_transform_gizmo()
{
	Node3DEditor::get_singleton()->update_transform_gizmo();
}

void Path3DEditorPlugin::_handle_option_pressed(int p_option)
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
	case HANDLE_OPTION_SNAP_COLLIDER: {
		bool is_checked = pm->is_item_checked(HANDLE_OPTION_SNAP_COLLIDER);
		snap_to_collider = !is_checked;
		pm->set_item_checked(HANDLE_OPTION_SNAP_COLLIDER, snap_to_collider);
	} break;
	}
}

void Path3DEditorPlugin::_confirm_clear_points()
{
	if (!path || path->get_curve().is_null() || path->get_curve()->get_point_count() == 0) {
		return;
	}
	clear_points_dialog->reset_size();
	clear_points_dialog->popup_centered();
}

void Path3DEditorPlugin::_clear_curve_points()
{
	if (!path || path->get_curve().is_null() || path->get_curve()->get_point_count() == 0) {
		return;
	}
	Ref<Curve3D> curve = path->get_curve();
	curve->set_closed(false);
	curve->clear_points();
}

void Path3DEditorPlugin::_restore_curve_points(const PackedVector3Array& p_points)
{
	if (!path || path->get_curve().is_null()) {
		return;
	}
	Ref<Curve3D> curve = path->get_curve();

	if (curve->get_point_count() > 0) {
		curve->clear_points();
	}

	for (int i = 0; i < p_points.size(); i += 3) {
		curve->add_point(p_points[i + 2], p_points[i], p_points[i + 1]);
	}
}

void Path3DEditorPlugin::_update_theme()
{
	curve_edit->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveEdit")));
	curve_edit_curve->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveCurve")));
	curve_edit_tilt->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveTilt")));
	curve_create->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveCreate")));
	curve_del->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveDelete")));
	curve_closed->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("CurveClose")));
	curve_clear_points->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("Clear")));
	create_curve_button->set_button_icon(topmenu_bar->get_editor_theme_icon(SNAME("Curve3D")));
}

void Path3DEditorPlugin::_update_toolbar()
{
	if (!path) {
		return;
	}
	bool has_curve = path->get_curve().is_valid();
	toolbar->set_visible(has_curve);
	create_curve_button->set_visible(!has_curve);
}

String Path3DGizmoPlugin::get_gizmo_name() const { return "Path3D"; }


