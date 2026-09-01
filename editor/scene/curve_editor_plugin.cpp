/**************************************************************************/
/*  curve_editor_plugin.cpp                                               */
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

#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"
#include "curve_editor_plugin.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "scene/resources/image_texture.h"

CurveEdit::CurveEdit()
{
	set_focus_mode(FOCUS_ALL);
	set_clip_contents(true);
}

Ref<Curve> CurveEdit::get_curve() { return curve; }

Size2 CurveEdit::get_minimum_size() const
{
	return Vector2(64, MAX(135, get_size().x * ASPECT_RATIO)) * EDSCALE;
}

void CurveEdit::gui_input(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());
	if (curve.is_null()) {
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		// Deleting points or making tangents linear.
		if (k->is_pressed() && k->get_keycode() == Key::KEY_DELETE) {
			if (selected_tangent_index != TANGENT_NONE) {
				toggle_linear(selected_index, selected_tangent_index);
			}
			else if (selected_index != -1) {
				if (grabbing == GRAB_ADD) {
					curve->remove_point(
						selected_index); // Point is temporary, so remove directly from curve.
					set_selected_index(-1);
				}
				else {
					remove_point(selected_index);
				}
				grabbing = GRAB_NONE;
				hovered_index = -1;
				hovered_tangent_index = TANGENT_NONE;
			}
			accept_event();
		}

		if (k->get_keycode() == Key::SHIFT || k->get_keycode() == Key::ALT) {
			queue_redraw(); // Redraw to show the axes or constraints.
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed()) {
		Vector2 mpos = mb->get_position();

		if (mb->get_button_index() == MouseButton::RIGHT ||
			mb->get_button_index() == MouseButton::MIDDLE) {
			if (mb->get_button_index() == MouseButton::RIGHT && grabbing == GRAB_MOVE) {
				// Move a point to its old position.
				curve->set_point_value(selected_index, initial_grab_pos.y);
				curve->set_point_offset(selected_index, initial_grab_pos.x);
				set_selected_index(initial_grab_index);
				hovered_index = get_point_at(mpos);
				grabbing = GRAB_NONE;
			}
			else {
				// Remove a point or make a tangent linear.
				selected_tangent_index = get_tangent_at(mpos);
				if (selected_tangent_index != TANGENT_NONE) {
					toggle_linear(selected_index, selected_tangent_index);
				}
				else {
					int point_to_remove = get_point_at(mpos);
					if (point_to_remove == -1) {
						set_selected_index(
							-1); // Nothing on the place of the click, just deselect the point.
					}
					else {
						if (grabbing == GRAB_ADD) {
							curve->remove_point(point_to_remove); // Point is temporary, so remove
																  // directly from curve.
							set_selected_index(-1);
						}
						else {
							remove_point(point_to_remove);
						}
						hovered_index = get_point_at(mpos);
						grabbing = GRAB_NONE;
					}
				}
			}
		}

		// Selecting or creating points.
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (grabbing == GRAB_NONE) {
				selected_tangent_index = get_tangent_at(mpos);
				if (selected_tangent_index == TANGENT_NONE) {
					set_selected_index(get_point_at(mpos));
				}
				queue_redraw();
			}

			if (selected_index != -1) {
				// If an existing point/tangent was grabbed, remember a few things about it.
				grabbing = GRAB_MOVE;
				initial_grab_pos = curve->get_point_position(selected_index);
				initial_grab_index = selected_index;
				if (selected_index > 0) {
					initial_grab_left_tangent = curve->get_point_left_tangent(selected_index);
				}
				if (selected_index < curve->get_point_count() - 1) {
					initial_grab_right_tangent = curve->get_point_right_tangent(selected_index);
				}
			}
			else if (grabbing == GRAB_NONE) {
				// Adding a new point. Insert a temporary point for the user to adjust, so it's not
				// in the undo/redo.
				Vector2 new_pos = get_world_pos(mpos).clamp(
					Vector2(curve->get_min_domain(), curve->get_min_value()),
					Vector2(curve->get_max_domain(), curve->get_max_value()));
				if (snap_enabled || mb->is_command_or_control_pressed()) {
					new_pos.x = Math::snapped(new_pos.x - curve->get_min_domain(),
									curve->get_domain_range() / snap_count) +
								curve->get_min_domain();
					new_pos.y = Math::snapped(new_pos.y - curve->get_min_value(),
									curve->get_value_range() / snap_count) +
								curve->get_min_value();
				}

				new_pos.x = get_offset_without_collision(
					selected_index, new_pos.x, mpos.x >= get_view_pos(new_pos).x);

				// Add a temporary point for the user to adjust before adding it permanently.
				int new_idx = curve->add_point_no_update(new_pos);
				set_selected_index(new_idx);
				grabbing = GRAB_ADD;
				initial_grab_pos = new_pos;
			}
		}
	}

	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT && !mb->is_pressed()) {
		if (selected_tangent_index != TANGENT_NONE) {
			// Finish moving a tangent control.
			if (selected_index == 0) {
				set_point_right_tangent(
					selected_index, curve->get_point_right_tangent(selected_index));
			}
			else if (selected_index == curve->get_point_count() - 1) {
				set_point_left_tangent(
					selected_index, curve->get_point_left_tangent(selected_index));
			}
			else {
				set_point_tangents(selected_index, curve->get_point_left_tangent(selected_index),
					curve->get_point_right_tangent(selected_index));
			}
			grabbing = GRAB_NONE;
		}
		else if (grabbing == GRAB_MOVE) {
			// Finish moving a point.
			set_point_position(selected_index, curve->get_point_position(selected_index));
			grabbing = GRAB_NONE;
		}
		else if (grabbing == GRAB_ADD) {
			// Finish inserting a new point. Remove the temporary point and insert a permanent one
			// in its place.
			Vector2 new_pos = curve->get_point_position(selected_index);
			curve->remove_point(selected_index);
			add_point(new_pos);
			grabbing = GRAB_NONE;
		}
		queue_redraw();
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Vector2 mpos = mm->get_position();

		if (grabbing != GRAB_NONE && curve.is_valid()) {
			if (selected_index != -1) {
				if (selected_tangent_index == TANGENT_NONE) {
					// Drag point.
					Vector2 new_pos = get_world_pos(mpos).clamp(
						Vector2(curve->get_min_domain(), curve->get_min_value()),
						Vector2(curve->get_max_domain(), curve->get_max_value()));

					if (snap_enabled || mm->is_command_or_control_pressed()) {
						new_pos.x = Math::snapped(new_pos.x - curve->get_min_domain(),
										curve->get_domain_range() / snap_count) +
									curve->get_min_domain();
						new_pos.y = Math::snapped(new_pos.y - curve->get_min_value(),
										curve->get_value_range() / snap_count) +
									curve->get_min_value();
					}

					// Allow to snap to axes with Shift.
					if (mm->is_shift_pressed()) {
						Vector2 initial_mpos = get_view_pos(initial_grab_pos);
						if (Math::abs(mpos.x - initial_mpos.x) >
							Math::abs(mpos.y - initial_mpos.y)) {
							new_pos.y = initial_grab_pos.y;
						}
						else {
							new_pos.x = initial_grab_pos.x;
						}
					}

					// Allow to constraint the point between the adjacent two with Alt.
					if (mm->is_alt_pressed()) {
						float prev_point_offset =
							(selected_index > 0)
								? (curve->get_point_position(selected_index - 1).x + 0.00001)
								: curve->get_min_domain();
						float next_point_offset =
							(selected_index < curve->get_point_count() - 1)
								? (curve->get_point_position(selected_index + 1).x - 0.00001)
								: curve->get_max_domain();
						new_pos.x = CLAMP(new_pos.x, prev_point_offset, next_point_offset);
					}

					new_pos.x = get_offset_without_collision(
						selected_index, new_pos.x, mpos.x >= get_view_pos(new_pos).x);

					// The index may change if the point is dragged across another one.
					int i = curve->set_point_offset(selected_index, new_pos.x);
					hovered_index = i;
					set_selected_index(i);

					new_pos.y = CLAMP(new_pos.y, curve->get_min_value(), curve->get_max_value());
					curve->set_point_value(selected_index, new_pos.y);

				}
				else {
					// Drag tangent.

					const Vector2 new_pos = curve->get_point_position(selected_index);
					const Vector2 control_pos = get_world_pos(mpos);

					Vector2 dir = (control_pos - new_pos).normalized();
					real_t tangent =
						dir.y / (dir.x > 0 ? MAX(dir.x, 0.00001) : MIN(dir.x, -0.00001));

					// Must keep track of the hovered index as the cursor might move outside of the
					// editor while dragging.
					hovered_tangent_index = selected_tangent_index;

					// Adjust the tangents.
					if (selected_tangent_index == TANGENT_LEFT) {
						curve->set_point_left_tangent(selected_index, tangent);

						// Align the other tangent if it isn't linear and Shift is not pressed.
						// If Shift is pressed at any point, restore the initial angle of the other
						// tangent.
						if (selected_index != (curve->get_point_count() - 1) &&
							curve->get_point_right_mode(selected_index) != Curve::TANGENT_LINEAR) {
							curve->set_point_right_tangent(selected_index,
								mm->is_shift_pressed() ? initial_grab_right_tangent : tangent);
						}

					}
					else {
						curve->set_point_right_tangent(selected_index, tangent);

						if (selected_index != 0 &&
							curve->get_point_left_mode(selected_index) != Curve::TANGENT_LINEAR) {
							curve->set_point_left_tangent(selected_index,
								mm->is_shift_pressed() ? initial_grab_left_tangent : tangent);
						}
					}
				}
			}
		}
		else {
			// Grab mode is GRAB_NONE, so do hovering logic.
			hovered_index = get_point_at(mpos);
			hovered_tangent_index = get_tangent_at(mpos);
			queue_redraw();
		}
	}
}

void CurveEdit::_curve_changed()
{
	queue_redraw();
	// Point count can change in case of undo.
	if (selected_index >= curve->get_point_count()) {
		set_selected_index(-1);
	}
}

int CurveEdit::get_point_at(const Vector2& p_pos) const
{
	if (curve.is_null()) {
		return -1;
	}

	// Use a square-shaped hover region. If hovering multiple points, pick the closer one.
	const Rect2 hover_rect = Rect2(p_pos, Vector2(0, 0)).grow(hover_radius);
	int closest_idx = -1;
	float closest_dist_squared = hover_radius * hover_radius * 2;

	for (int i = 0; i < curve->get_point_count(); ++i) {
		Vector2 p = get_view_pos(curve->get_point_position(i));
		if (hover_rect.has_point(p) && p.distance_squared_to(p_pos) < closest_dist_squared) {
			closest_dist_squared = p.distance_squared_to(p_pos);
			closest_idx = i;
		}
	}

	return closest_idx;
}

CurveEdit::TangentIndex CurveEdit::get_tangent_at(const Vector2& p_pos) const
{
	if (curve.is_null() || selected_index < 0) {
		return TANGENT_NONE;
	}

	const Rect2 hover_rect = Rect2(p_pos, Vector2(0, 0)).grow(tangent_hover_radius);

	if (selected_index != 0) {
		Vector2 control_pos = get_tangent_view_pos(selected_index, TANGENT_LEFT);
		if (hover_rect.has_point(control_pos)) {
			return TANGENT_LEFT;
		}
	}

	if (selected_index != curve->get_point_count() - 1) {
		Vector2 control_pos = get_tangent_view_pos(selected_index, TANGENT_RIGHT);
		if (hover_rect.has_point(control_pos)) {
			return TANGENT_RIGHT;
		}
	}

	return TANGENT_NONE;
}

// FIXME: This function should be bounded better.
float CurveEdit::get_offset_without_collision(
	int p_current_index, float p_offset, bool p_prioritize_right)
{
	float safe_offset = p_offset;
	bool prioritizing_right = p_prioritize_right;

	for (int i = 0; i < curve->get_point_count(); i++) {
		if (i == p_current_index) {
			continue;
		}

		if (curve->get_point_position(i).x > safe_offset) {
			break;
		}

		if (curve->get_point_position(i).x == safe_offset) {
			if (prioritizing_right) {
				safe_offset += 0.00001;
				if (safe_offset > 1.0) {
					safe_offset = 1.0;
					prioritizing_right = false;
				}
			}
			else {
				safe_offset -= 0.00001;
				if (safe_offset < 0.0) {
					safe_offset = 0.0;
					prioritizing_right = true;
				}
			}
			i = -1;
		}
	}

	return safe_offset;
}

void CurveEdit::set_selected_index(int p_index)
{
	if (p_index != selected_index) {
		selected_index = p_index;
		queue_redraw();
	}
}

void CurveEdit::update_view_transform()
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));

	const real_t margin = font->get_height(font_size) + 2 * EDSCALE;

	float min_x = curve.is_valid() ? curve->get_min_domain() : 0.0;
	float max_x = curve.is_valid() ? curve->get_max_domain() : 1.0;
	float min_y = curve.is_valid() ? curve->get_min_value() : 0.0;
	float max_y = curve.is_valid() ? curve->get_max_value() : 1.0;

	const Rect2 world_rect = Rect2(min_x, min_y, max_x - min_x, max_y - min_y);
	const Size2 view_margin(margin, margin);
	const Size2 view_size = get_size() - view_margin * 2;
	const Vector2 scale = view_size / world_rect.size;

	Transform2D world_trans;
	world_trans.translate_local(-world_rect.position - Vector2(0, world_rect.size.y));
	world_trans.scale(Vector2(scale.x, -scale.y));

	Transform2D view_trans;
	view_trans.translate_local(view_margin);

	_world_to_view = view_trans * world_trans;
}

Vector2 CurveEdit::get_tangent_view_pos(int p_index, TangentIndex p_tangent) const
{
	Vector2 dir;
	if (p_tangent == TANGENT_LEFT) {
		dir = -Vector2(1, curve->get_point_left_tangent(p_index));
	}
	else {
		dir = Vector2(1, curve->get_point_right_tangent(p_index));
	}

	Vector2 point_pos = curve->get_point_position(p_index);
	Vector2 point_view_pos = get_view_pos(point_pos);
	Vector2 control_view_pos = get_view_pos(point_pos + dir);

	Vector2 distance_from_point = tangent_length * (control_view_pos - point_view_pos).normalized();
	Vector2 tangent_view_pos = point_view_pos + distance_from_point;

	// Since the tangent is long, it might slip outside of the area of the editor for points close
	// to the domain/range boundaries. The code below shrinks the tangent control by up to 50% so it
	// always stays inside the editor for points within the bounds.
	float fraction_inside = 1.0;
	if (distance_from_point.x != 0.0) {
		fraction_inside = MIN(fraction_inside,
			((distance_from_point.x > 0 ? get_rect().size.x : 0) - point_view_pos.x) /
				distance_from_point.x);
	}
	if (distance_from_point.y != 0.0) {
		fraction_inside = MIN(fraction_inside,
			((distance_from_point.y > 0 ? get_rect().size.y : 0) - point_view_pos.y) /
				distance_from_point.y);
	}

	if (fraction_inside < 1.0 && fraction_inside > 0.5) {
		tangent_view_pos = point_view_pos + distance_from_point * fraction_inside;
	}

	return tangent_view_pos;
}

Vector2 CurveEdit::get_view_pos(const Vector2& p_world_pos) const
{
	return _world_to_view.xform(p_world_pos);
}

Vector2 CurveEdit::get_world_pos(const Vector2& p_view_pos) const
{
	return _world_to_view.affine_inverse().xform(p_view_pos);
}

// Uses non-baked points, but takes advantage of ordered iteration to be faster.
void CurveEdit::plot_curve_accurate(
	float p_step, const Color& p_line_color, const Color& p_edge_line_color)
{
	const real_t min_x = curve->get_min_domain();
	const real_t max_x = curve->get_max_domain();
	if (curve->get_point_count() <= 1) { // Draw single line through entire plot.
		real_t y = curve->sample(0);
		draw_line(get_view_pos(Vector2(min_x, y)) + Vector2(0.5, 0),
			get_view_pos(Vector2(max_x, y)) - Vector2(1.5, 0), p_line_color, LINE_WIDTH, true);
		return;
	}

	Vector2 first_point = curve->get_point_position(0);
	Vector2 last_point = curve->get_point_position(curve->get_point_count() - 1);

	// Transform pixels-per-step into curve domain. Only works for non-rotated transforms.
	const float world_step_size = p_step / _world_to_view.get_scale().x;

	// Edge lines.
	draw_line(get_view_pos(Vector2(min_x, first_point.y)) + Vector2(0.5, 0),
		get_view_pos(first_point), p_edge_line_color, LINE_WIDTH, true);
	draw_line(get_view_pos(last_point),
		get_view_pos(Vector2(max_x, last_point.y)) - Vector2(1.5, 0), p_edge_line_color, LINE_WIDTH,
		true);

	// Draw section by section, so that we get maximum precision near points.
	// It's an accurate representation, but slower than using the baked one.
	for (int i = 1; i < curve->get_point_count(); ++i) {
		Vector2 a = curve->get_point_position(i - 1);
		Vector2 b = curve->get_point_position(i);

		Vector2 pos = a;
		Vector2 prev_pos = a;

		float samples = (b.x - a.x) / world_step_size;

		for (int j = 1; j < samples; j++) {
			float x = j * world_step_size;
			pos.x = a.x + x;
			pos.y = curve->sample_local_nocheck(i - 1, x);
			draw_line(get_view_pos(prev_pos), get_view_pos(pos), p_line_color, LINE_WIDTH, true);
			prev_pos = pos;
		}

		draw_line(get_view_pos(prev_pos), get_view_pos(b), p_line_color, LINE_WIDTH, true);
	}
}

void CurveEdit::_redraw()
{
	if (curve.is_null()) {
		return;
	}

	update_view_transform();

	// Draw background.

	Vector2 view_size = get_rect().size;
	draw_style_box(get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr(),
		Rect2(Point2(), view_size));

	// Draw primary grid.
	draw_set_transform_matrix(_world_to_view);

	Vector2 min_edge = get_world_pos(Vector2(0, view_size.y));
	Vector2 max_edge = get_world_pos(Vector2(view_size.x, 0));

	const Color grid_color_primary =
		get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.25);
	const Color grid_color =
		get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.1);

	const Vector2i grid_steps = Vector2i(4, 2);
	const Vector2 step_size =
		Vector2(curve->get_domain_range(), curve->get_value_range()) / grid_steps;

	draw_line(Vector2(min_edge.x, curve->get_min_value()),
		Vector2(max_edge.x, curve->get_min_value()), grid_color_primary);
	draw_line(Vector2(max_edge.x, curve->get_max_value()),
		Vector2(min_edge.x, curve->get_max_value()), grid_color_primary);
	draw_line(Vector2(curve->get_min_domain(), min_edge.y),
		Vector2(curve->get_min_domain(), max_edge.y), grid_color_primary);
	draw_line(Vector2(curve->get_max_domain(), max_edge.y),
		Vector2(curve->get_max_domain(), min_edge.y), grid_color_primary);

	for (int i = 1; i < grid_steps.x; i++) {
		real_t x = curve->get_min_domain() + i * step_size.x;
		draw_line(Vector2(x, min_edge.y), Vector2(x, max_edge.y), grid_color);
	}

	for (int i = 1; i < grid_steps.y; i++) {
		real_t y = curve->get_min_value() + i * step_size.y;
		draw_line(Vector2(min_edge.x, y), Vector2(max_edge.x, y), grid_color);
	}

	// Draw number markings.
	draw_set_transform_matrix(Transform2D());

	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	float font_height = font->get_height(font_size);
	Color text_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));

	int pad = Math::round(2 * EDSCALE);

	for (int i = 0; i <= grid_steps.x; ++i) {
		real_t x = curve->get_min_domain() + i * step_size.x;
		draw_string(font.ptr(),
			get_view_pos(Vector2(x, curve->get_min_value())) + Vector2(pad, font_height - pad),
			String::num(x, 2), HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, text_color);
	}

	for (int i = 0; i <= grid_steps.y; ++i) {
		real_t y = curve->get_min_value() + i * step_size.y;
		draw_string(font.ptr(),
			get_view_pos(Vector2(curve->get_min_domain(), y)) + Vector2(pad, -pad),
			String::num(y, 2), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_color);
	}

	// Draw curve in view coordinates. Curve world-to-view point conversion happens in
	// plot_curve_accurate().

	const Color line_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	const Color edge_line_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor)) *
		Color(1, 1, 1, 0.75);

	plot_curve_accurate(STEP_SIZE, line_color, edge_line_color);

	// Draw points, except for the selected one.

	bool shift_pressed = Input::get_singleton()->is_key_pressed(Key::SHIFT);

	const Color point_color =
		get_theme_color(SceneStringName(font_color), EditorStringName(Editor));

	for (int i = 0; i < curve->get_point_count(); ++i) {
		Vector2 pos = get_view_pos(curve->get_point_position(i));
		if (selected_index != i) {
			draw_rect(Rect2(pos, Vector2(0, 0)).grow(point_radius), point_color);
		}
		if (hovered_index == i && hovered_tangent_index == TANGENT_NONE) {
			draw_rect(Rect2(pos, Vector2(0, 0)).grow(hover_radius - Math::round(3 * EDSCALE)),
				line_color, false, Math::round(1 * EDSCALE));
		}
	}

	// Draw selected point and its tangents.

	if (selected_index >= 0) {
		const Vector2 point_pos = curve->get_point_position(selected_index);
		const Color selected_point_color =
			get_theme_color(SNAME("accent_color"), EditorStringName(Editor));

		// Draw tangents if not dragging a point, or if holding a point without having moved it yet.
		if (grabbing == GRAB_NONE || initial_grab_pos == point_pos ||
			selected_tangent_index != TANGENT_NONE) {
			const Color selected_tangent_color =
				get_theme_color(SNAME("accent_color"), EditorStringName(Editor)).darkened(0.25);
			const Color tangent_color =
				get_theme_color(SceneStringName(font_color), EditorStringName(Editor))
					.darkened(0.25);

			if (selected_index != 0) {
				Vector2 control_pos = get_tangent_view_pos(selected_index, TANGENT_LEFT);
				Color left_tangent_color = (selected_tangent_index == TANGENT_LEFT)
											   ? selected_tangent_color
											   : tangent_color;

				draw_line(
					get_view_pos(point_pos), control_pos, left_tangent_color, 0.5 * EDSCALE, true);
				// Square for linear mode, circle otherwise.
				if (curve->get_point_left_mode(selected_index) == Curve::TANGENT_FREE) {
					draw_circle(control_pos, tangent_radius, left_tangent_color);
				}
				else {
					draw_rect(
						Rect2(control_pos, Vector2(0, 0)).grow(tangent_radius), left_tangent_color);
				}
				// Hover indicator.
				if (hovered_tangent_index == TANGENT_LEFT ||
					(hovered_tangent_index == TANGENT_RIGHT && !shift_pressed &&
						curve->get_point_left_mode(selected_index) != Curve::TANGENT_LINEAR)) {
					draw_rect(Rect2(control_pos, Vector2(0, 0))
								  .grow(tangent_hover_radius - Math::round(3 * EDSCALE)),
						tangent_color, false, Math::round(1 * EDSCALE));
				}
			}

			if (selected_index != curve->get_point_count() - 1) {
				Vector2 control_pos = get_tangent_view_pos(selected_index, TANGENT_RIGHT);
				Color right_tangent_color = (selected_tangent_index == TANGENT_RIGHT)
												? selected_tangent_color
												: tangent_color;

				draw_line(
					get_view_pos(point_pos), control_pos, right_tangent_color, 0.5 * EDSCALE, true);
				// Square for linear mode, circle otherwise.
				if (curve->get_point_right_mode(selected_index) == Curve::TANGENT_FREE) {
					draw_circle(control_pos, tangent_radius, right_tangent_color);
				}
				else {
					draw_rect(Rect2(control_pos, Vector2(0, 0)).grow(tangent_radius),
						right_tangent_color);
				}
				// Hover indicator.
				if (hovered_tangent_index == TANGENT_RIGHT ||
					(hovered_tangent_index == TANGENT_LEFT && !shift_pressed &&
						curve->get_point_right_mode(selected_index) != Curve::TANGENT_LINEAR)) {
					draw_rect(Rect2(control_pos, Vector2(0, 0))
								  .grow(tangent_hover_radius - Math::round(3 * EDSCALE)),
						tangent_color, false, Math::round(1 * EDSCALE));
				}
			}
		}

		draw_rect(
			Rect2(get_view_pos(point_pos), Vector2(0, 0)).grow(point_radius), selected_point_color);
	}

	// Draw help text.

	if (selected_index > 0 && selected_index < curve->get_point_count() - 1 &&
		selected_tangent_index == TANGENT_NONE && hovered_tangent_index != TANGENT_NONE &&
		!shift_pressed) {
		float width = view_size.x - 50 * EDSCALE;
		text_color.a *= 0.4;

		draw_multiline_string(font.ptr(),
			Vector2(25 * EDSCALE, font_height - Math::round(2 * EDSCALE)),
			TTR("Hold Shift to edit tangents individually"), HORIZONTAL_ALIGNMENT_CENTER, width,
			font_size, -1, text_color);

	}
	else if (selected_index != -1 && selected_tangent_index == TANGENT_NONE) {
		const Vector2 point_pos = curve->get_point_position(selected_index);
		float width = view_size.x - 50 * EDSCALE;
		text_color.a *= 0.8;

		draw_string(font.ptr(), Vector2(25 * EDSCALE, font_height - Math::round(2 * EDSCALE)),
			vformat("(%.2f, %.2f)", point_pos.x, point_pos.y), HORIZONTAL_ALIGNMENT_CENTER, width,
			font_size, text_color);

	}
	else if (selected_index != -1 && selected_tangent_index != TANGENT_NONE) {
		float width = view_size.x - 50 * EDSCALE;
		text_color.a *= 0.8;
		real_t theta =
			Math::rad_to_deg(Math::atan(selected_tangent_index == TANGENT_LEFT
											? -1 * curve->get_point_left_tangent(selected_index)
											: curve->get_point_right_tangent(selected_index)));

		draw_string(font.ptr(), Vector2(25 * EDSCALE, font_height - Math::round(2 * EDSCALE)),
			String::num(theta, 1) + String::utf8(" °"), HORIZONTAL_ALIGNMENT_CENTER, width,
			font_size, text_color);
	}

	// Draw temporary constraints and snapping axes.
	draw_set_transform_matrix(_world_to_view);

	if (Input::get_singleton()->is_key_pressed(Key::ALT) && grabbing != GRAB_NONE &&
		selected_tangent_index == TANGENT_NONE) {
		float prev_point_offset = (selected_index > 0)
									  ? curve->get_point_position(selected_index - 1).x
									  : curve->get_min_domain();
		float next_point_offset = (selected_index < curve->get_point_count() - 1)
									  ? curve->get_point_position(selected_index + 1).x
									  : curve->get_max_domain();

		draw_line(Vector2(prev_point_offset, curve->get_min_value()),
			Vector2(prev_point_offset, curve->get_max_value()), Color(point_color, 0.6));
		draw_line(Vector2(next_point_offset, curve->get_min_value()),
			Vector2(next_point_offset, curve->get_max_value()), Color(point_color, 0.6));
	}

	if (shift_pressed && grabbing != GRAB_NONE && selected_tangent_index == TANGENT_NONE) {
		draw_line(Vector2(initial_grab_pos.x, curve->get_min_value()),
			Vector2(initial_grab_pos.x, curve->get_max_value()),
			get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)).darkened(0.4));
		draw_line(Vector2(curve->get_min_domain(), initial_grab_pos.y),
			Vector2(curve->get_max_domain(), initial_grab_pos.y),
			get_theme_color(SNAME("axis_y_color"), EditorStringName
(Editor)).darkened(0.4));
	}
}

///////////////////////

const int CurveEditor::DEFAULT_SNAP = 10;

void CurveEditor::_set_snap_enabled(bool p_enabled)
{
	curve_editor_rect->set_snap_enabled(p_enabled);
	snap_count_edit->set_visible(p_enabled);
}

void CurveEditor::_set_snap_count(int p_snap_count)
{
	curve_editor_rect->set_snap_count(CLAMP(p_snap_count, 2, 100));
}

void CurveEditor::_on_preset_item_selected(int p_preset_id)
{
	curve_editor_rect->use_preset(p_preset_id);
}

void CurveEditor::set_curve(const Ref<Curve>& p_curve) { curve_editor_rect->set_curve(p_curve); }

///////////////////////

///////////////////////

bool CurvePreviewGenerator::handles(const String& p_type) const { return p_type == "Curve"; }


