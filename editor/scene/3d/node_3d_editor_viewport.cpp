/**************************************************************************/
/*  node_3d_editor_viewport.cpp                                           */
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

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/projection.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/3d/node_3d_editor_constants.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/translations/editor_translation_preview_button.h"
#include "node_3d_editor_viewport.h"
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/decal.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/gradient.h"
#include "scene/resources/immediate_mesh.h"
#include "scene/resources/packed_scene.h"
#include "servers/physics_3d/physics_server_3d_types.h"
#include "servers/rendering/rendering_server.h"

using namespace Node3DEditorConstants;

Node3DEditorSelectedItem::~Node3DEditorSelectedItem()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	if (sbox_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance);
	}
	if (sbox_instance_offset.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_offset);
	}
	if (sbox_instance_xray.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_xray);
	}
	if (sbox_instance_xray_offset.is_valid()) {
		RenderingServer::get_singleton()->free_rid(sbox_instance_xray_offset);
	}
}

void ViewportNavigationControl::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (viewport != nullptr) {
			_draw();
			_update_navigation();
		}
	} break;

	case NOTIFICATION_MOUSE_ENTER: {
		hovered = true;
		queue_redraw();
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		hovered = false;
		queue_redraw();
	} break;
	}
}

void ViewportNavigationControl::_process_click(int p_index, Vector2 p_position, bool p_pressed)
{
	hovered = false;
	queue_redraw();

	if (focused_index != -1 && focused_index != p_index) {
		return;
	}
	if (p_pressed) {
		if (p_position.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
			focused_pos = p_position;
			focused_index = p_index;
			queue_redraw();
		}
	}
	else {
		focused_index = -1;
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(focused_mouse_start);
		}
	}
}

void ViewportNavigationControl::_process_drag(
	int p_index, Vector2 p_position, Vector2 p_relative_position)
{
	if (focused_index == p_index) {
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_VISIBLE) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
			focused_mouse_start = p_position;
		}
		focused_pos += p_relative_position;
		queue_redraw();
	}
}

void ViewportNavigationControl::set_viewport(Node3DEditorViewport* p_viewport)
{
	viewport = p_viewport;
}

void ViewportRotationControl::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		axis_menu_options.clear();
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_RIGHT);
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_TOP);
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_FRONT);
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_LEFT);
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_BOTTOM);
		axis_menu_options.push_back(Node3DEditorViewport::VIEW_REAR);

		axis_colors.clear();
		axis_colors.push_back(get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)));
		axis_colors.push_back(get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)));
		axis_colors.push_back(get_theme_color(SNAME("axis_z_color"), EditorStringName(Editor)));
		queue_redraw();
	} break;

	case NOTIFICATION_DRAW: {
		if (viewport != nullptr) {
			_draw();
		}
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		focused_axis = -2;
		queue_redraw();
	} break;

	case NOTIFICATION_WM_WINDOW_FOCUS_OUT: {
		gizmo_activated = false;
	} break;
	}
}

void ViewportRotationControl::_draw()
{
	const Vector2 center = get_size() / 2.0;
	const real_t radius = get_size().x / 2.0;

	if (focused_axis > -2 || orbiting_index != -1) {
		draw_circle(center, radius, Color(0.5, 0.5, 0.5, 0.25), true, -1.0, true);
	}

	Vector<Axis2D> axis_to_draw;
	_get_sorted_axis(axis_to_draw);
	for (int i = 0; i < axis_to_draw.size(); ++i) {
		_draw_axis(axis_to_draw[i]);
	}
}

void ViewportRotationControl::_draw_axis(const Axis2D& p_axis)
{
	const bool focused = focused_axis == p_axis.axis;
	const bool positive = p_axis.is_positive;
	const int direction = p_axis.axis % 3;

	const Color axis_color = axis_colors[direction];
	const double min_alpha = 0.35;
	const double alpha =
		focused ? 1.0 : Math::remap((p_axis.z_axis + 1.0) / 2.0, 0, 0.5, min_alpha, 1.0);
	const Color c = focused ? Color(axis_color.lightened(0.25), 1.0) : Color(axis_color, alpha);

	// Highlight positive axis text when hovered.
	const Color c_positive_axis =
		focused ? Color(1.0, 1.0, 1.0, alpha) : Color(0.0, 0.0, 0.0, alpha * 0.6);

	// Highlight negative axis text when hovered, but hide when not focused.
	const Color c_negative_axis = focused ? Color(1.0, 1.0, 1.0, alpha) : Color(axis_color, 0);

	if (positive) {
		// Draw axis lines for the positive axes.
		const Vector2 center = get_size() / 2.0;
		const Vector2 diff = p_axis.screen_point - center;
		const float line_length = MAX(diff.length() - AXIS_CIRCLE_RADIUS - 0.5 * EDSCALE, 0);

		draw_line(center + diff.limit_length(0.5 * EDSCALE),
			center + diff.limit_length(line_length), c, 1.5 * EDSCALE, true);

		draw_circle(p_axis.screen_point, AXIS_CIRCLE_RADIUS, c, true, -1.0, true);

		// Draw the axis letter for the positive axes.
		const String axis_name = direction == 0 ? "X" : (direction == 1 ? "Y" : "Z");
		const Ref<Font>& font =
			get_theme_font(SNAME("rotation_control"), EditorStringName(EditorFonts));
		const int font_size =
			get_theme_font_size(SNAME("rotation_control_size"), EditorStringName(EditorFonts));
		const Size2 char_size = font->get_char_size(axis_name[0], font_size);
		const Vector2 char_offset = Vector2(-char_size.width / 2.0, char_size.height * 0.25);
		draw_char(
			font.ptr(), p_axis.screen_point + char_offset, axis_name, font_size, c_positive_axis);
	}
	else {
		// Draw an outline around the negative axes.
		draw_circle(p_axis.screen_point, AXIS_CIRCLE_RADIUS, c, true, -1.0, true);
		draw_circle(
			p_axis.screen_point, AXIS_CIRCLE_RADIUS * 0.8, c.darkened(0.4), true, -1.0, true);

		// Draw the text for the negative axes.
		const String axis_name = direction == 0 ? "-X" : (direction == 1 ? "-Y" : "-Z");
		const Ref<Font>& font =
			get_theme_font(SNAME("rotation_control"), EditorStringName(EditorFonts));
		const int font_size =
			get_theme_font_size(SNAME("rotation_control_size"), EditorStringName(EditorFonts));
		const Size2 string_size =
			font->get_string_size(axis_name, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size);
		const float font_ascent = font->get_ascent(font_size);
		const float font_descent = font->get_descent(font_size);
		const float string_height = font_ascent + font_descent;
		const Vector2 offset(-string_size.width / 2.0, string_height * 0.25);
		draw_string(font.ptr(), p_axis.screen_point + offset, axis_name, HORIZONTAL_ALIGNMENT_LEFT,
			-1.0f, font_size, c_negative_axis);
	}
}

void ViewportRotationControl::_process_click(int p_index, Vector2 p_position, bool p_pressed)
{
	if (orbiting_index != -1 && orbiting_index != p_index) {
		return;
	}
	if (p_pressed) {
		if (p_position.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
			orbiting_index = p_index;
		}
	}
	else {
		if (focused_axis > -1 && gizmo_activated) {
			viewport->_menu_option(axis_menu_options[focused_axis]);
			_update_focus();
		}
		orbiting_index = -1;
		if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_VISIBLE);
			Input::get_singleton()->warp_mouse(orbiting_mouse_start);
		}
	}
}

void ViewportRotationControl::_update_focus()
{
	int original_focus = focused_axis;
	focused_axis = -2;
	Vector2 mouse_pos = get_local_mouse_position();

	if (mouse_pos.distance_to(get_size() / 2.0) < get_size().x / 2.0) {
		focused_axis = -1;
	}

	Vector<Axis2D> axes;
	_get_sorted_axis(axes);

	for (int i = 0; i < axes.size(); i++) {
		const Axis2D& axis = axes[i];
		if (mouse_pos.distance_to(axis.screen_point) < AXIS_CIRCLE_RADIUS) {
			focused_axis = axis.axis;
		}
	}

	if (focused_axis != original_focus) {
		queue_redraw();
	}
}

void ViewportRotationControl::set_viewport(Node3DEditorViewport* p_viewport)
{
	viewport = p_viewport;
}

bool Node3DEditorViewport::_is_rotation_arc_visible() const
{
	return _edit.mode == TRANSFORM_ROTATE &&
		   !Math::is_zero_approx(_edit.accumulated_rotation_angle) && _edit.gizmo_initiated;
}

float Node3DEditorViewport::get_znear() const
{
	return CLAMP(spatial_editor->get_znear(), MIN_Z, MAX_Z);
}

float Node3DEditorViewport::get_zfar() const
{
	return CLAMP(spatial_editor->get_zfar(), MIN_Z, MAX_Z);
}

Transform3D Node3DEditorViewport::_get_camera_transform() const
{
	return camera->get_global_transform();
}

Vector3 Node3DEditorViewport::_get_camera_position() const
{
	return _get_camera_transform().origin;
}

Point2 Node3DEditorViewport::point_to_screen(const Vector3& p_point)
{
	return camera->unproject_position(p_point);
}

Vector3 Node3DEditorViewport::get_ray_pos(const Vector2& p_pos) const
{
	return camera->project_ray_origin(p_pos);
}

Vector3 Node3DEditorViewport::_get_camera_normal() const
{
	return -_get_camera_transform().basis.get_column(2);
}

Vector3 Node3DEditorViewport::get_ray(const Vector2& p_pos) const
{
	return camera->project_ray_normal(p_pos);
}

float Node3DEditorViewport::_min_screen_dist_to_aabb(
	const AABB& p_aabb, const Transform3D& p_transform, const Point2& p_cursor) const
{
	Vector3 first_corner = p_transform.xform(p_aabb.get_endpoint(0));
	if (camera->is_position_behind(first_corner)) {
		return 0.0f;
	}
	Point2 screen_min = camera->unproject_position(first_corner);
	Point2 screen_max = screen_min;

	for (int i = 1; i < 8; i++) {
		Vector3 world_corner = p_transform.xform(p_aabb.get_endpoint(i));
		if (camera->is_position_behind(world_corner)) {
			return 0.0f;
		}
		Point2 s = camera->unproject_position(world_corner);
		screen_min = screen_min.min(s);
		screen_max = screen_max.max(s);
	}

	float dx = MAX(screen_min.x - p_cursor.x, MAX(0.0f, p_cursor.x - screen_max.x));
	float dy = MAX(screen_min.y - p_cursor.y, MAX(0.0f, p_cursor.y - screen_max.y));
	return Math::sqrt(dx * dx + dy * dy);
}

bool Node3DEditorViewport::_is_vertex_occluded(
	const Vector3& p_world_pos, const Vector2& p_screen_pos) const
{
	Vector3 ray_pos = get_ray_pos(p_screen_pos);
	float vertex_dist = ray_pos.distance_to(p_world_pos);
	Vector<Node3D*> hits = Node3DEditor::get_singleton()->gizmo_bvh_ray_query(
		ray_pos, ray_pos + get_ray(p_screen_pos) * camera->get_far());
	for (Node3D* spat : hits) {
		if (!spat) {
			continue;
		}
		Vector<Ref<Node3DGizmo>> gizmos = spat->get_gizmos();
		for (int i = 0; i < gizmos.size(); i++) {
			Ref<EditorNode3DGizmo> seg = gizmos[i];
			if (seg.is_null()) {
				continue;
			}
			Vector3 point, normal;
			if (seg->intersect_ray(camera, p_screen_pos, point, normal)) {
				if (ray_pos.distance_to(point) < vertex_dist - 0.01f) {
					return true;
				}
			}
		}
	}
	return false;
}

void Node3DEditorViewport::_find_items_at_pos(
	const Point2& p_pos, Vector<_RayResult>& r_results, bool p_include_locked_nodes)
{
	Vector3 ray = get_ray(p_pos);
	Vector3 pos = get_ray_pos(p_pos);

	Vector<Node3D*> nodes_with_gizmos =
		Node3DEditor::get_singleton()->gizmo_bvh_ray_query(pos, pos + ray * camera->get_far());

	HashSet<Node3D*> found_nodes;

	for (Node3D* spat : nodes_with_gizmos) {
		if (!spat) {
			continue;
		}

		if (found_nodes.has(spat)) {
			continue;
		}

		if (!p_include_locked_nodes && _is_node_locked(spat)) {
			continue;
		}

		Vector<Ref<Node3DGizmo>> gizmos = spat->get_gizmos();
		for (int j = 0; j < gizmos.size(); j++) {
			Ref<EditorNode3DGizmo> seg = gizmos[j];

			if (seg.is_null()) {
				continue;
			}

			Vector3 point;
			Vector3 normal;

			bool inters = seg->intersect_ray(camera, p_pos, point, normal);

			if (!inters) {
				continue;
			}

			const real_t dist = pos.distance_to(point);

			if (dist < 0) {
				continue;
			}

			found_nodes.insert(spat);

			_RayResult res;
			res.item = spat;
			res.depth = dist;
			r_results.push_back(res);
			break;
		}
	}

	r_results.sort();
}

static Key _get_key_modifier(Ref<InputEventWithModifiers> e)
{
	if (e->is_shift_pressed()) {
		return Key::SHIFT;
	}
	if (e->is_alt_pressed()) {
		return Key::ALT;
	}
	if (e->is_ctrl_pressed()) {
		return Key::CTRL;
	}
	if (e->is_meta_pressed()) {
		return Key::META;
	}
	return Key::NONE;
}

bool Node3DEditorViewport::_transform_gizmo_select(
	const Vector2& p_screenpos, bool p_highlight_only)
{
	if (!spatial_editor->is_gizmo_visible()) {
		return false;
	}
	if (get_selected_count() == 0) {
		if (p_highlight_only) {
			spatial_editor->select_gizmo_highlight_axis(-1);
		}
		return false;
	}

	Vector3 ray_pos = get_ray_pos(p_screenpos);
	Vector3 ray = get_ray(p_screenpos);

	Transform3D gt = spatial_editor->get_gizmo_transform();

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM ||
		spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_MOVE) {
		int col_axis = -1;
		real_t col_d = 1e20;

		for (int i = 0; i < 3; i++) {
			const Vector3 grabber_pos =
				gt.origin + gt.basis.get_column(i).normalized() * gizmo_scale *
								(GIZMO_ARROW_OFFSET + (GIZMO_ARROW_SIZE * 0.5));
			const real_t grabber_radius = gizmo_scale * GIZMO_ARROW_SIZE;

			Vector3 r;

			if (Geometry3D::segment_intersects_sphere(
					ray_pos, ray_pos + ray * MAX_Z, grabber_pos, grabber_radius, &r)) {
				const real_t d = r.distance_to(ray_pos);
				if (d < col_d) {
					col_d = d;
					col_axis = i;
				}
			}
		}

		bool is_plane_translate = false;
		// plane select
		if (col_axis == -1) {
			col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				Vector3 ivec2 = gt.basis.get_column((i + 1) % 3).normalized();
				Vector3 ivec3 = gt.basis.get_column((i + 2) % 3).normalized();

				// Allow some tolerance to make the plane easier to click,
				// even if the click is actually slightly outside the plane.
				const Vector3 grabber_pos =
					gt.origin +
					(ivec2 + ivec3) * gizmo_scale * (GIZMO_PLANE_SIZE + GIZMO_PLANE_DST * 0.6667);

				Vector3 r;
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);

				if (plane.intersects_ray(ray_pos, ray, &r)) {
					const real_t dist = r.distance_to(grabber_pos);
					// Allow some tolerance to make the plane easier to click,
					// even if the click is actually slightly outside the plane.
					if (dist < (gizmo_scale * GIZMO_PLANE_SIZE * 1.5)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;

							is_plane_translate = true;
						}
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(
					col_axis + (is_plane_translate ? 6 : 0));

			}
			else {
				// handle plane translate
				_edit.mode = TRANSFORM_TRANSLATE;
				_compute_edit(p_screenpos);
				_edit.plane =
					TransformPlane(TRANSFORM_X_AXIS + col_axis + (is_plane_translate ? 3 : 0));
			}
			return true;
		}
	}

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_TRANSFORM ||
		spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_ROTATE) {
		int col_axis = -1;
		bool view_rotation_selected = false;
		bool trackball_selected = false;

		Vector3 hit_position;
		Vector3 hit_normal;
		real_t ray_length =
			gt.origin.distance_to(ray_pos) + (GIZMO_CIRCLE_SIZE * gizmo_scale) * 4.0f;
		if (Geometry3D::segment_intersects_sphere(ray_pos, ray_pos + ray * ray_length, gt.origin,
				gizmo_scale * (GIZMO_CIRCLE_SIZE), &hit_position, &hit_normal)) {
			if (hit_normal.dot(_get_camera_normal()) < 0.05) {
				hit_position = gt.xform_inv(hit_position).abs();
				int min_axis = hit_position.min_axis_index();
				if (hit_position[min_axis] < gizmo_scale * GIZMO_RING_HALF_WIDTH) {
					col_axis = min_axis;
				}
			}
		}

		if (col_axis == -1) {
			float col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);
				Vector3 r;
				if (!plane.intersects_ray(ray_pos, ray, &r)) {
					continue;
				}

				const real_t dist = r.distance_to(gt.origin);
				const Vector3 r_dir = (r - gt.origin).normalized();

				if (_get_camera_normal().dot(r_dir) <= 0.005) {
					if (dist > gizmo_scale * (GIZMO_CIRCLE_SIZE - GIZMO_RING_HALF_WIDTH) &&
						dist < gizmo_scale * (GIZMO_CIRCLE_SIZE + GIZMO_RING_HALF_WIDTH)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;
						}
					}
				}
			}
		}

		if (col_axis == -1) {
			Vector3 ray_to_center = gt.origin - ray_pos;
			real_t ray_length_to_center = ray_to_center.dot(ray);
			Vector3 closest_point_on_ray = ray_pos + ray * ray_length_to_center;
			real_t distance_ray_to_center = closest_point_on_ray.distance_to(gt.origin);

			real_t view_rotation_radius = gizmo_scale * spatial_editor->gizmo_view_rotation_scale;
			real_t circumference_tolerance = gizmo_scale * GIZMO_RING_HALF_WIDTH;

			if (Math::abs(distance_ray_to_center - view_rotation_radius) <
					circumference_tolerance &&
				ray_length_to_center > 0) {
				view_rotation_selected = true;
			}
			else if (spatial_editor->is_trackball_enabled() &&
					   distance_ray_to_center <
						   gizmo_scale * (GIZMO_CIRCLE_SIZE - GIZMO_RING_HALF_WIDTH) &&
					   ray_length_to_center > 0) {
				trackball_selected = true;
			}
		}

		if (view_rotation_selected) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(GIZMO_HIGHLIGHT_AXIS_VIEW_ROTATION);
			}
			else {
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_VIEW;
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = _get_camera_normal();
				_edit.view_axis_local = spatial_editor->get_gizmo_transform()
											.basis.xform_inv(_get_camera_normal())
											.normalized();
				_edit.gizmo_initiated = true;
			}
			return true;
		}
		else if (trackball_selected) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(GIZMO_HIGHLIGHT_AXIS_TRACKBALL);
			}
			else {
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_VIEW;
				_edit.is_trackball = true;
				_edit.show_rotation_line = false;
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = _get_camera_normal();
				_edit.gizmo_initiated = true;
				spatial_editor->select_gizmo_highlight_axis(-1);
			}
			return true;
		}
		else if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(col_axis + 3);
			}
			else {
				// handle axis-specific rotate
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TransformPlane(TRANSFORM_X_AXIS + col_axis);
				_edit.accumulated_rotation_angle = 0.0;
				_edit.rotation_angle = 0.0;
				_edit.rotation_axis = gt.basis.get_column(col_axis).normalized();
				_edit.gizmo_initiated = true;
			}
			return true;
		}
	}

	if (spatial_editor->get_tool_mode() == Node3DEditor::TOOL_MODE_SCALE) {
		int col_axis = -1;
		float col_d = 1e20;

		for (int i = 0; i < 3; i++) {
			const Vector3 grabber_pos =
				gt.origin + gt.basis.get_column(i).normalized() * gizmo_scale * GIZMO_SCALE_OFFSET;
			const real_t grabber_radius = gizmo_scale * GIZMO_ARROW_SIZE;

			Vector3 r;

			if (Geometry3D::segment_intersects_sphere(
					ray_pos, ray_pos + ray * MAX_Z, grabber_pos, grabber_radius, &r)) {
				const real_t d = r.distance_to(ray_pos);
				if (d < col_d) {
					col_d = d;
					col_axis = i;
				}
			}
		}

		bool is_plane_scale = false;
		// plane select
		if (col_axis == -1) {
			col_d = 1e20;

			for (int i = 0; i < 3; i++) {
				const Vector3 ivec2 = gt.basis.get_column((i + 1) % 3).normalized();
				const Vector3 ivec3 = gt.basis.get_column((i + 2) % 3).normalized();

				// Allow some tolerance to make the plane easier to click,
				// even if the click is actually slightly outside the plane.
				const Vector3 grabber_pos =
					gt.origin +
					(ivec2 + ivec3) * gizmo_scale * (GIZMO_PLANE_SIZE + GIZMO_PLANE_DST * 0.6667);

				Vector3 r;
				Plane plane(gt.basis.get_column(i).normalized(), gt.origin);

				if (plane.intersects_ray(ray_pos, ray, &r)) {
					const real_t dist = r.distance_to(grabber_pos);
					// Allow some tolerance to make the plane easier to click,
					// even if the click is actually slightly outside the plane.
					if (dist < (gizmo_scale * GIZMO_PLANE_SIZE * 1.5)) {
						const real_t d = ray_pos.distance_to(r);
						if (d < col_d) {
							col_d = d;
							col_axis = i;

							is_plane_scale = true;
						}
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				spatial_editor->select_gizmo_highlight_axis(col_axis + (is_plane_scale ? 12 : 9));

			}
			else {
				// handle scale
				_edit.mode = TRANSFORM_SCALE;
				_compute_edit(p_screenpos);
				_edit.plane =
					TransformPlane(TRANSFORM_X_AXIS + col_axis + (is_plane_scale ? 3 : 0));
			}
			return true;
		}
	}

	if (p_highlight_only) {
		spatial_editor->select_gizmo_highlight_axis(-1);
	}

	return false;
}

Transform3D Node3DEditorViewport::_compute_transform(TransformMode p_mode,
	const Transform3D& p_original, const Transform3D& p_original_local, Vector3 p_motion,
	double p_extra, bool p_local, bool p_orthogonal, bool p_view_axis)
{
	switch (p_mode) {
	case TRANSFORM_SCALE: {
		if (spatial_editor->is_snap_enabled()) {
			p_motion.snapf(p_extra);
		}
		Transform3D s;
		if (p_local) {
			s.basis = p_original_local.basis.scaled_local(p_motion + Vector3(1, 1, 1));
			s.origin = p_original_local.origin;
		}
		else {
			s.basis.scale(p_motion + Vector3(1, 1, 1));
			Transform3D base = Transform3D(Basis(), _edit.center);
			s = base * (s * (base.inverse() * p_original));

			// Recalculate orthogonalized scale without moving origin.
			if (p_orthogonal) {
				s.basis = p_original.basis.scaled_orthogonal(p_motion + Vector3(1, 1, 1));
			}
		}

		return s;
	}
	case TRANSFORM_TRANSLATE: {
		if (spatial_editor->is_snap_enabled()) {
			p_motion.snapf(p_extra);
		}

		if (p_local) {
			return p_original_local.translated_local(p_motion);
		}

		return p_original.translated(p_motion);
	}
	case TRANSFORM_ROTATE: {
		Transform3D r;

		Basis parent_global_basis = p_original.basis * p_original_local.basis.inverse();

		Vector3 axis;
		if (p_local && !p_view_axis) {
			axis = p_original_local.basis.xform(p_motion);
		}
		else {
			axis = parent_global_basis.xform_inv(p_motion);
		}

		if (p_local) {
			r.basis = Basis(axis.normalized(), p_extra) * p_original_local.basis;
			r.origin = p_original_local.origin;
		}
		else {
			r.basis =
				parent_global_basis * Basis(axis.normalized(), p_extra) * p_original_local.basis;
			r.origin =
				Basis(p_motion, p_extra).xform(p_original.origin - _edit.center) + _edit.center;
		}

		return r;
	}
	default: {
		ERR_FAIL_V_MSG(Transform3D(), "Invalid mode in '_compute_transform'");
	}
	}
}

void Node3DEditorViewport::_surface_mouse_enter()
{
	if (Input::get_singleton()->get_mouse_mode() == Input::MouseMode::MOUSE_MODE_CAPTURED) {
		return;
	}

	if (!surface->has_focus() && (!get_viewport()->gui_get_focus_owner() ||
									 !get_viewport()->gui_get_focus_owner()->is_text_field())) {
		surface->grab_focus();
	}
}

void Node3DEditorViewport::_surface_mouse_exit()
{
	_remove_preview_node();
	_reset_preview_material();
	_remove_preview_material();
}

void Node3DEditorViewport::_surface_focus_enter()
{
	view_display_menu->set_disable_shortcuts(false);
}

void Node3DEditorViewport::_surface_focus_exit() { view_display_menu->set_disable_shortcuts(true); }

void Node3DEditorViewport::_cursor_distance_scaled()
{
	zoom_indicator_delay = ZOOM_FREELOOK_INDICATOR_DELAY_S;
	surface->queue_redraw();
}

void Node3DEditorViewport::_pilot_ensure_undo_session()
{
	if (pilot_undo_session_active || !previewing) {
		return;
	}
	pilot_undo_initial_transform = previewing->get_global_transform();
	pilot_undo_session_active = true;
	pilot_undo_idle_time = 0.0;
}

void Node3DEditorViewport::_pilot_tick_undo_session(real_t p_delta)
{
	if (!pilot_undo_session_active) {
		return;
	}
	pilot_undo_idle_time += p_delta;
	if (pilot_undo_idle_time > 0.15) {
		_pilot_commit_undo_session();
	}
}

void Node3DEditorViewport::_freelook_speed_scaled()
{
	zoom_indicator_delay = ZOOM_FREELOOK_INDICATOR_DELAY_S;
	surface->queue_redraw();
}

bool Node3DEditorViewport::_is_nav_modifier_pressed(const String& p_name)
{
	return _is_shortcut_empty(p_name) || Input::get_singleton()->is_action_pressed(p_name);
}

bool Node3DEditorViewport::_is_shortcut_empty(const String& p_name)
{
	Ref<Shortcut> check_shortcut = ED_GET_SHORTCUT(p_name);

	ERR_FAIL_COND_V_MSG(
		check_shortcut.is_null(), true, "The Shortcut was null, possible name mismatch.");

	return check_shortcut->get_events().is_empty();
}

void Node3DEditorViewport::set_message(const String& p_message, float p_time)
{
	message = p_message;
	message_time = p_time;
}

static void override_label_colors(Control* p_control)
{
	p_control->begin_bulk_theme_override();
	p_control->add_theme_color_override(SceneStringName(font_color),
		p_control->get_theme_color(SNAME("font_dark_background_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override(
		"font_hover_color", p_control->get_theme_color(SNAME("font_dark_background_hover_color"),
								EditorStringName(Editor)));
	p_control->add_theme_color_override(
		"font_focus_color", p_control->get_theme_color(SNAME("font_dark_background_focus_color"),
								EditorStringName(Editor)));
	p_control->add_theme_color_override("font_pressed_color",
		p_control->get_theme_color(
			SNAME("font_dark_background_pressed_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_hover_pressed_color",
		p_control->get_theme_color(
			SNAME("font_dark_background_hover_pressed_color"), EditorStringName(Editor)));
	p_control->end_bulk_theme_override();
}

static void draw_indicator_bar(Control& p_surface, real_t p_fill, const Ref<Texture2D> p_icon,
	const Ref<Font> p_font, int p_font_size, const String& p_text, const Color& p_color)
{
	// Adjust bar size from control height
	const Vector2 surface_size = p_surface.get_size();
	const real_t h = surface_size.y / 2.0;
	const real_t y = (surface_size.y - h) / 2.0;

	const Rect2 r(10 * EDSCALE, y, 6 * EDSCALE, h);
	const real_t sy = r.size.y * p_fill;

	// Note: because this bar appears over the viewport, it has to stay readable for any background
	// color Draw both neutral dark and bright colors to account this
	p_surface.draw_rect(r, p_color * Color(1, 1, 1, 0.2));
	p_surface.draw_rect(Rect2(r.position.x, r.position.y + r.size.y - sy, r.size.x, sy),
		p_color * Color(1, 1, 1, 0.6));
	p_surface.draw_rect(r.grow(1), Color(0, 0, 0, 0.7), false, Math::round(EDSCALE));

	const Vector2 icon_size = p_icon->get_size();
	const Vector2 icon_pos =
		Vector2(r.position.x - (icon_size.x - r.size.x) / 2, r.position.y + r.size.y + 2 * EDSCALE);
	p_surface.draw_texture(p_icon.ptr(), icon_pos, p_color);

	// Draw text below the bar (for speed/zoom information).
	p_surface.draw_string_outline(p_font.ptr(),
		Vector2(icon_pos.x, icon_pos.y + icon_size.y + 16 * EDSCALE), p_text,
		HORIZONTAL_ALIGNMENT_LEFT, -1.f, p_font_size, Math::round(4 * EDSCALE), Color(0, 0, 0));
	p_surface.draw_string(p_font.ptr(),
		Vector2(icon_pos.x, icon_pos.y + icon_size.y + 16 * EDSCALE), p_text,
		HORIZONTAL_ALIGNMENT_LEFT, -1.f, p_font_size, p_color);
}

bool Node3DEditorViewport::_camera_moved_externally()
{
	if (previewing_camera && previewing) {
		if (pilot_preview_enabled) {
			Transform3D t = previewing->get_global_transform();
			return !t.is_equal_approx(last_camera_transform);
		}
		return false;
	}
	Transform3D t = camera->get_global_transform();
	return !t.is_equal_approx(last_camera_transform);
}

void Node3DEditorViewport::_apply_camera_transform_to_cursor()
{
	if (previewing_camera && previewing) {
		if (pilot_preview_enabled) {
			_sync_cursor_from_transform(previewing->get_global_transform());
		}
		return;
	}
	_sync_cursor_from_transform(camera->get_camera_transform());
}

void Node3DEditorViewport::_preview_camera_property_changed()
{
	if (previewing) {
		surface->queue_redraw();
	}
}

void Node3DEditorViewport::_update_centered_labels()
{
	if (cinema_label->is_visible()) {
		cinema_label->reset_size();
		float cinema_half_width = cinema_label->get_size().width / 2.0f;
		cinema_label->set_anchor_and_offset(SIDE_LEFT, 0.5f, -cinema_half_width);
	}

	if (locked_label->is_visible()) {
		locked_label->reset_size();
		float locked_half_width = locked_label->get_size().width / 2.0f;
		locked_label->set_anchor_and_offset(SIDE_LEFT, 0.5f, -locked_half_width);
	}
}

void Node3DEditorViewport::_init_gizmo_instance(int p_idx)
{
	uint32_t layer = 1 << (GIZMO_BASE_LAYER + p_idx);

	for (int i = 0; i < 3; i++) {
		move_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(
			move_gizmo_instance[i], spatial_editor->get_move_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			move_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			move_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(move_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			move_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			move_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		move_plane_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(
			move_plane_gizmo_instance[i], spatial_editor->get_move_plane_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			move_plane_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(move_plane_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			move_plane_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(move_plane_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			move_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			move_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		scale_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(
			scale_gizmo_instance[i], spatial_editor->get_scale_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			scale_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(scale_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			scale_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(scale_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			scale_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			scale_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		scale_plane_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(
			scale_plane_gizmo_instance[i], spatial_editor->get_scale_plane_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			scale_plane_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(scale_plane_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			scale_plane_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(scale_plane_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			scale_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			scale_plane_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);

		axis_gizmo_instance[i] = RS::get_singleton()->instance_create();
	}

	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->instance_set_base(
			axis_gizmo_instance[i], spatial_editor->get_axis_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			axis_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(axis_gizmo_instance[i], true);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			axis_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(axis_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			axis_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			axis_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	}

	for (int i = 0; i < 4; i++) {
		rotate_gizmo_instance[i] = RS::get_singleton()->instance_create();
		RS::get_singleton()->instance_set_base(
			rotate_gizmo_instance[i], spatial_editor->get_rotate_gizmo(i)->get_rid());
		RS::get_singleton()->instance_set_scenario(
			rotate_gizmo_instance[i], get_tree()->get_root()->get_world_3d()->get_scenario());
		RS::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
		RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
			rotate_gizmo_instance[i], RSE::SHADOW_CASTING_SETTING_OFF);
		RS::get_singleton()->instance_set_layer_mask(rotate_gizmo_instance[i], layer);
		RS::get_singleton()->instance_geometry_set_flag(
			rotate_gizmo_instance[i], RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
		RS::get_singleton()->instance_geometry_set_flag(
			rotate_gizmo_instance[i], RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
	}

	// Create trackball sphere instance
	trackball_sphere_instance = RS::get_singleton()->instance_create();
	RS::get_singleton()->instance_set_base(
		trackball_sphere_instance, spatial_editor->get_trackball_sphere_gizmo()->get_rid());
	RS::get_singleton()->instance_set_scenario(
		trackball_sphere_instance, get_tree()->get_root()->get_world_3d()->get_scenario());
	RS::get_singleton()->instance_set_visible(trackball_sphere_instance, false);
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
		trackball_sphere_instance, RSE::SHADOW_CASTING_SETTING_OFF);
	RS::get_singleton()->instance_set_layer_mask(trackball_sphere_instance, layer);
	RS::get_singleton()->instance_geometry_set_flag(
		trackball_sphere_instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, true);
	RS::get_singleton()->instance_geometry_set_flag(
		trackball_sphere_instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
}

void Node3DEditorViewport::_finish_gizmo_instances()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	for (int i = 0; i < 3; i++) {
		RS::get_singleton()->free_rid(move_gizmo_instance[i]);
		RS::get_singleton()->free_rid(move_plane_gizmo_instance[i]);
		RS::get_singleton()->free_rid(rotate_gizmo_instance[i]);
		RS::get_singleton()->free_rid(scale_gizmo_instance[i]);
		RS::get_singleton()->free_rid(scale_plane_gizmo_instance[i]);
		RS::get_singleton()->free_rid(axis_gizmo_instance[i]);
	}
	// Rotation white outline
	RS::get_singleton()->free_rid(rotate_gizmo_instance[3]);

	RS::get_singleton()->free_rid(trackball_sphere_instance);
}

void Node3DEditorViewport::_disable_follow_mode()
{
	// Exit follow mode by resetting the number of times the follow shortcut was used consecutively.
	times_focused_consecutively = 0;
}

void Node3DEditorViewport::_reset_follow_mode_count()
{
	bool is_in_follow_mode =
		times_focused_consecutively >= 2 && times_focused_consecutively % 2 == 0;
	if (!is_in_follow_mode) {
		times_focused_consecutively = 0;
	}
}

void Node3DEditorViewport::_selection_menu_hide()
{
	selection_results.clear();
	selection_menu->clear();
	selection_menu->reset_size();
}

void Node3DEditorViewport::set_can_preview(Camera3D* p_preview)
{
	preview = p_preview;

	if (!preview_camera->is_pressed() && !previewing_cinema) {
		preview_camera->set_visible(p_preview);
	}
}

void Node3DEditorViewport::update_transform_gizmo_highlight()
{
	if (!is_visible_in_tree() ||
		!Rect2(Vector2(), surface->get_size()).has_point(surface->get_local_mouse_position())) {
		return;
	}
	_transform_gizmo_select(surface->get_local_mouse_position(), true);
}

void Node3DEditorViewport::assign_pending_data_pointers(
	Node3D* p_preview_node, AABB* p_preview_bounds, AcceptDialog* p_accept)
{
	preview_node = p_preview_node;
	preview_bounds = p_preview_bounds;
	accept = p_accept;
}

void Node3DEditorViewport::_remove_preview_node()
{
	tooltip_panel->hide();

	set_message("");
	if (preview_node->get_parent()) {
		for (int i = preview_node->get_child_count() - 1; i >= 0; i--) {
			Node* node = preview_node->get_child(i);
			node->queue_free();
			preview_node->remove_child(node);
		}
		EditorNode::get_singleton()->get_scene_root()->remove_child(preview_node);
	}
}

bool Node3DEditorViewport::_cyclical_dependency_exists(
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

void Node3DEditorViewport::_show_tooltip(const String& p_title, const String& p_description) const
{
	tooltip_panel->set_text(vformat("[font_size=%s][b][color=%s]%s[/color][/b][/font_size]\n%s",
		get_theme_default_font_size() + 2,
		get_theme_color(SNAME("accent_color"), EditorStringName(Editor)).to_html(false), p_title,
		p_description));
	tooltip_panel->show();
}

void Node3DEditorViewport::begin_transform(TransformMode p_mode, bool instant)
{
	if (previewing) {
		return;
	}

	if (get_selected_count() > 0) {
		if (!_has_unlocked_selection()) {
			return;
		}
		_edit.children_original_globals.clear();

		_edit.mode = p_mode;
		_compute_edit(_edit.mouse_pos);
		_edit.instant = instant;
		_edit.initial_click_vector = Vector3();
		_edit.previous_rotation_vector = Vector3();
		_edit.accumulated_rotation_angle = 0.0;
		_edit.rotation_angle = 0.0;
		_edit.gizmo_initiated = false;
		switch (p_mode) {
		case TRANSFORM_ROTATE:
			_edit.show_rotation_line = true;
			set_message(vformat(TTR("Rotating %s degrees."), String::num(0, 0)));
			break;
		case TRANSFORM_TRANSLATE:
			set_message(vformat(TTR("Translating: %s"), vformat("%.0v", Vector3())));
			break;
		case TRANSFORM_SCALE:
			set_message(vformat(TTR("Scaling: %s"), vformat("%.0v", Vector3())));
			break;
		default:
			break;
		}
		update_transform_gizmo_view();
		set_process_input(instant);
		surface->queue_redraw();
	}
}

void Node3DEditorViewport::update_transform_numeric()
{
	Vector3 motion;
	switch (_edit.plane) {
	case TRANSFORM_VIEW: {
		switch (_edit.mode) {
		case TRANSFORM_TRANSLATE:
			motion = Vector3(1, 0, 0);
			break;
		case TRANSFORM_ROTATE:
			motion = _edit.view_axis_local;
			break;
		case TRANSFORM_SCALE:
			motion = Vector3(1, 1, 1);
			break;
		case TRANSFORM_NONE:
			ERR_FAIL_MSG("_edit.mode cannot be TRANSFORM_NONE in update_transform_numeric.");
		}
		break;
	}
	case TRANSFORM_X_AXIS:
		motion = Vector3(1, 0, 0);
		break;
	case TRANSFORM_Y_AXIS:
		motion = Vector3(0, 1, 0);
		break;
	case TRANSFORM_Z_AXIS:
		motion = Vector3(0, 0, 1);
		break;
	case TRANSFORM_XY:
		motion = Vector3(1, 1, 0);
		break;
	case TRANSFORM_XZ:
		motion = Vector3(1, 0, 1);
		break;
	case TRANSFORM_YZ:
		motion = Vector3(0, 1, 1);
		break;
	}

	double value = _edit.numeric_input * (_edit.numeric_negate ? -1 : 1);
	double extra = 0.0;
	switch (_edit.mode) {
	case TRANSFORM_TRANSLATE:
		motion *= value;
		set_message(vformat(TTR("Translating %s."), motion));
		break;
	case TRANSFORM_ROTATE:
		extra = Math::deg_to_rad(value);
		set_message(vformat(TTR("Rotating %f degrees."), value));
		break;
	case TRANSFORM_SCALE:
		// To halve the size of an object in Blender, you scale it by 0.5.
		// Doing the same in Godot is considered scaling it by -0.5.
		motion *= (value - 1.0);
		set_message(vformat(TTR("Scaling %s."), motion));
		break;
	case TRANSFORM_NONE:
		ERR_FAIL_MSG("_edit.mode cannot be TRANSFORM_NONE in update_transform_numeric.");
	}

	apply_transform(motion, extra);
}

// Update the action in the InputMap to the provided shortcut events.

void Node3DEditorViewport::_load_viewport_inputs()
{
	// Registering with Key::NONE intentionally creates an empty Array.
	register_shortcut_action(
		"spatial_editor/viewport_orbit_modifier_1", TTRC("Viewport Orbit Modifier 1"), Key::NONE);
	register_shortcut_action(
		"spatial_editor/viewport_orbit_modifier_2", TTRC("Viewport Orbit Modifier 2"), Key::NONE);
	register_shortcut_action("spatial_editor/viewport_orbit_snap_modifier_1",
		TTRC("Viewport Orbit Snap Modifier 1"), Key::ALT);
	register_shortcut_action("spatial_editor/viewport_orbit_snap_modifier_2",
		TTRC("Viewport Orbit Snap Modifier 2"), Key::NONE);
	register_shortcut_action(
		"spatial_editor/viewport_pan_modifier_1", TTRC("Viewport Pan Modifier 1"), Key::SHIFT);
	register_shortcut_action(
		"spatial_editor/viewport_pan_modifier_2", TTRC("Viewport Pan Modifier 2"), Key::NONE);
	register_shortcut_action(
		"spatial_editor/viewport_zoom_modifier_1", TTRC("Viewport Zoom Modifier 1"), Key::CTRL);
	register_shortcut_action(
		"spatial_editor/viewport_zoom_modifier_2", TTRC("Viewport Zoom Modifier 2"), Key::NONE);

	register_shortcut_action("spatial_editor/freelook_left", TTRC("Freelook Left"), Key::A, true);
	register_shortcut_action("spatial_editor/freelook_right", TTRC("Freelook Right"), Key::D, true);
	register_shortcut_action(
		"spatial_editor/freelook_forward", TTRC("Freelook Forward"), Key::W, true);
	register_shortcut_action(
		"spatial_editor/freelook_backwards", TTRC("Freelook Backwards"), Key::S, true);
	register_shortcut_action("spatial_editor/freelook_up", TTRC("Freelook Up"), Key::E, true);
	register_shortcut_action("spatial_editor/freelook_down", TTRC("Freelook Down"), Key::Q, true);
	register_shortcut_action(
		"spatial_editor/freelook_speed_modifier", TTRC("Freelook Speed Modifier"), Key::SHIFT);
	register_shortcut_action(
		"spatial_editor/freelook_slow_modifier", TTRC("Freelook Slow Modifier"), Key::ALT);
}

Node3DEditorViewport::~Node3DEditorViewport() { memdelete(ruler); }

//////////////////////////////////////////////////////////////

void Node3DEditorViewportContainer::_update_split_drag_margin()
{
	if (view != VIEW_USE_4_VIEWPORTS && view != VIEW_USE_3_VIEWPORTS) {
		return;
	}
	// Also for 3 viewports view since the first split container is used to remember the offset.
	first_split->set_split_offset(second_split->get_split_offset());

	if (view == VIEW_USE_4_VIEWPORTS) {
		// Extend to cover the first split on top.
		second_split->set_drag_area_margin_begin(second_split->get_size().y - get_size().y);
	}
}

void Node3DEditorViewportContainer::set_view(View p_view)
{
	view = p_view;

	Node3DEditorViewport* viewports[4];
	for (uint32_t i = 0; i < 4; i++) {
		viewports[i] = Node3DEditor::get_singleton()->get_editor_viewport(i);
		ERR_FAIL_NULL(viewports[i]);
	}

	const bool previous_main_vertical = !first_split->is_vertical();
	const float horizontal_offset =
		previous_main_vertical ? first_split->get_split_offset() : main_split->get_split_offset();
	const float vertical_offset =
		previous_main_vertical ? main_split->get_split_offset() : first_split->get_split_offset();

	first_split->set_dragging_enabled(true);
	second_split->set_drag_area_margin_begin(0);
	viewports[0]->show();

	switch (view) {
	case VIEW_USE_1_VIEWPORT: {
		for (int i = 1; i < 4; i++) {
			viewports[i]->hide();
		}
		second_split->hide();
	} break;
	case VIEW_USE_2_VIEWPORTS:
	case VIEW_USE_2_VIEWPORTS_ALT: {
		viewports[1]->show();
		viewports[2]->hide();
		viewports[3]->hide();
		second_split->hide();
		const bool is_vertical = view == VIEW_USE_2_VIEWPORTS;
		if (first_split->is_vertical() != is_vertical) {
			first_split->set_vertical(is_vertical);
			first_split->set_split_offset(is_vertical ? vertical_offset : horizontal_offset);
			main_split->set_split_offset(
				is_vertical ? horizontal_offset
							: vertical_offset); // Store the other offset here for later.
		}
	} break;
	case VIEW_USE_3_VIEWPORTS:
	case VIEW_USE_3_VIEWPORTS_ALT: {
		// Default mode has two on bottom (second_split). Alt mode has two on the left
		// (first_split).
		const bool main_vertical = view == VIEW_USE_3_VIEWPORTS;
		viewports[1]->set_visible(!main_vertical);
		viewports[2]->show();
		viewports[3]->set_visible(main_vertical);
		second_split->show();
		main_split->set_vertical(main_vertical);
		main_split->set_split_offset(main_vertical ? vertical_offset : horizontal_offset);
		first_split->set_vertical(!main_vertical);
		first_split->set_split_offset(main_vertical ? horizontal_offset : vertical_offset);
		second_split->set_split_offset(main_vertical ? horizontal_offset : vertical_offset);
	} break;
	case VIEW_USE_4_VIEWPORTS: {
		for (int i = 1; i < 4; i++) {
			viewports[i]->show();
		}
		second_split->show();
		main_split->set_vertical(true);
		main_split->set_split_offset(vertical_offset);
		first_split->set_vertical(false);
		first_split->set_split_offset(horizontal_offset);
		second_split->set_split_offset(horizontal_offset);

		first_split->set_dragging_enabled(false);
		_update_split_drag_margin();
	} break;
	}
}

Node3DEditorViewportContainer::View Node3DEditorViewportContainer::get_view() { return view; }

Node3DEditorViewportContainer::Node3DEditorViewportContainer()
{
	set_clip_contents(true);

	main_split = memnew(SplitContainer);
	main_split->set_drag_nested_intersections(true);

	first_split = memnew(SplitContainer);
	first_split->set_h_size_flags(SIZE_EXPAND_FILL);
	first_split->set_v_size_flags(SIZE_EXPAND_FILL);
	first_split->set_drag_nested_intersections(true);

	second_split = memnew(SplitContainer);
	second_split->set_h_size_flags(SIZE_EXPAND_FILL);
	second_split->set_v_size_flags(SIZE_EXPAND_FILL);
	second_split->set_drag_nested_intersections(true);

	main_split->add_child(first_split);
	main_split->add_child(second_split);
	add_child(main_split);
}


