/**************************************************************************/
/*  gradient_editor_plugin.cpp                                            */
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
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "gradient_editor_plugin.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/popup.h"
#include "scene/gui/separator.h"
#include "scene/resources/gradient_texture.h"

int GradientEdit::_get_point_at(int p_xpos) const
{
	int result = -1;
	int total_w = _get_gradient_rect_width();
	float min_distance =
		handle_width *
		0.8; // Allow the cursor to be more than half a handle width away for ease of use.
	for (int i = 0; i < gradient->get_point_count(); i++) {
		// Ignore points outside of [0, 1].
		if (gradient->get_offset(i) < 0) {
			continue;
		}
		else if (gradient->get_offset(i) > 1) {
			break;
		}
		// Check if we clicked at point.
		float distance = Math::abs(p_xpos - gradient->get_offset(i) * total_w);
		if (distance < min_distance) {
			result = i;
			min_distance = distance;
		}
	}
	return result;
}

int GradientEdit::_predict_insertion_index(float p_offset)
{
	int result = 0;
	while (result < gradient->get_point_count() && gradient->get_offset(result) < p_offset) {
		result++;
	}
	return result;
}

int GradientEdit::_get_gradient_rect_width() const
{
	return get_size().width - get_size().height - draw_spacing - handle_width;
}

void GradientEdit::_show_color_picker()
{
	if (selected_index == -1) {
		return;
	}

	picker->set_pick_color(gradient->get_color(selected_index));
	Size2 minsize = popup->get_contents_minimum_size();
	float viewport_height = get_viewport_rect().size.y;

	// Determine in which direction to show the popup. By default popup below.
	// But if the popup doesn't fit below and the Gradient Editor is in the bottom half of the
	// viewport, show above.
	bool show_above = get_global_position().y + get_size().y + minsize.y > viewport_height &&
					  get_global_position().y * 2 + get_size().y > viewport_height;

	float v_offset = show_above ? -minsize.y : get_size().y;
	popup->set_position(get_screen_position() + Vector2(0, v_offset));
	popup->popup();
}

void GradientEdit::_color_changed(const Color& p_color) { set_color(selected_index, p_color); }

const Ref<Gradient>& GradientEdit::get_gradient() const { return gradient; }

ColorPicker* GradientEdit::get_picker() const { return picker; }

PopupPanel* GradientEdit::get_popup() const { return popup; }

void GradientEdit::_redraw()
{
	int w = get_size().x;
	int h = get_size().y - draw_spacing; // A bit of spacing below the gradient too.

	if (w == 0 || h == 0) {
		return; // Safety check as there is nothing to draw with such size.
	}

	int total_w = _get_gradient_rect_width();
	int half_handle_width = handle_width * 0.5;

	// Draw gradient.
	draw_texture_rect(get_editor_theme_icon(SNAME("GuiMiniCheckerboard")).ptr(),
		Rect2(half_handle_width, 0, total_w, h), true);
	preview_texture->set_gradient(gradient);
	draw_texture_rect(preview_texture.ptr(), Rect2(half_handle_width, 0, total_w, h));

	// Draw vertical snap lines.
	if (snap_enabled ||
		(Input::get_singleton()->is_key_pressed(Key::CTRL) && grabbing != GRAB_NONE)) {
		const Color line_color = Color(0.5, 0.5, 0.5, 0.5);
		for (int idx = 1; idx < snap_count; idx++) {
			float offset_x = idx * total_w / (float)snap_count + half_handle_width;
			draw_line(Point2(offset_x, 0), Point2(offset_x, h), line_color);
		}
	}

	// Draw handles.
	for (int i = 0; i < gradient->get_point_count(); i++) {
		// Only draw handles for points in [0, 1]. If there are points before or after, draw a
		// little indicator.
		if (gradient->get_offset(i) < 0.0) {
			continue;
		}
		else if (gradient->get_offset(i) > 1.0) {
			break;
		}
		// White or black handle color, to contrast with the selected color's brightness.
		// Also consider the fact that the color may be translucent.
		// The checkerboard pattern in the background has an average luminance of 0.75.
		Color inside_col = gradient->get_color(i);
		Color border_col = Math::lerp(0.75f, inside_col.get_luminance(), inside_col.a) > 0.455
							   ? Color(0, 0, 0)
							   : Color(1, 1, 1);

		int handle_thickness = MAX(1, Math::round(EDSCALE));
		float handle_x_pos = gradient->get_offset(i) * total_w + half_handle_width;
		float handle_start_x = handle_x_pos - half_handle_width;
		Rect2 rect = Rect2(handle_start_x, h / 2, handle_width, h / 2);

		if (inside_col.a < 1) {
			// If the color is translucent, draw a little opaque rectangle at the bottom to more
			// easily see it.
			draw_texture_rect(
				get_editor_theme_icon(SNAME("GuiMiniCheckerboard")).ptr(), rect, true);
			draw_rect(rect, inside_col, true);
			Color inside_col_opaque = inside_col;
			inside_col_opaque.a = 1.0;
			draw_rect(
				Rect2(handle_start_x + handle_thickness / 2.0, h * 0.9 - handle_thickness / 2.0,
					handle_width - handle_thickness, h * 0.1),
				inside_col_opaque, true);
		}
		else {
			draw_rect(rect, inside_col, true);
		}

		if (selected_index == i) {
			// Handle is selected.
			draw_rect(rect, border_col, false, handle_thickness);
			draw_line(Vector2(handle_x_pos, 0), Vector2(handle_x_pos, h / 2 - handle_thickness),
				border_col, handle_thickness);
			if (inside_col.a < 1) {
				draw_line(
					Vector2(handle_start_x + handle_thickness / 2.0, h * 0.9 - handle_thickness),
					Vector2(handle_start_x + handle_width - handle_thickness / 2.0,
						h * 0.9 - handle_thickness),
					border_col, handle_thickness);
			}
			rect = rect.grow(-handle_thickness);
			const Color focus_col =
				get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			draw_rect(
				rect, has_focus() ? focus_col : focus_col.darkened(0.4), false, handle_thickness);
			rect = rect.grow(-handle_thickness);
			draw_rect(rect, border_col, false, handle_thickness);
		}
		else {
			// Handle isn't selected.
			border_col.a = 0.9;
			draw_rect(rect, border_col, false, handle_thickness);
			draw_line(Vector2(handle_x_pos, 0), Vector2(handle_x_pos, h / 2 - handle_thickness),
				border_col, handle_thickness);
			if (inside_col.a < 1) {
				draw_line(
					Vector2(handle_start_x + handle_thickness / 2.0, h * 0.9 - handle_thickness),
					Vector2(handle_start_x + handle_width - handle_thickness / 2.0,
						h * 0.9 - handle_thickness),
					border_col, handle_thickness);
			}
			if (hovered_index == i) {
				// Draw a subtle translucent rect inside the handle if it's being hovered.
				rect = rect.grow(-handle_thickness);
				border_col.a = 0.54;
				draw_rect(rect, border_col, false, handle_thickness);
			}
		}
	}

	// Draw "button" for color selector.
	int button_offset = total_w + handle_width + draw_spacing;
	if (selected_index != -1) {
		Color grabbed_col = gradient->get_color(selected_index);
		if (grabbed_col.a < 1) {
			draw_texture_rect(get_editor_theme_icon(SNAME("GuiMiniCheckerboard")).ptr(),
				Rect2(button_offset, 0, h, h), true);
		}
		draw_rect(Rect2(button_offset, 0, h, h), grabbed_col);
		if (grabbed_col.r > 1 || grabbed_col.g > 1 || grabbed_col.b > 1) {
			// Draw an indicator to denote that the currently selected color is "overbright".
			draw_texture(get_theme_icon(SNAME("overbright_indicator"), SNAME("ColorPicker")).ptr(),
				Point2(button_offset, 0));
		}
	}
	else {
		// If no color is selected, draw gray color with 'X' on top.
		draw_rect(Rect2(button_offset, 0, h, h), Color(0.5, 0.5, 0.5, 1));
		draw_line(Vector2(button_offset, 0), Vector2(button_offset + h, h), Color(0.8, 0.8, 0.8));
		draw_line(Vector2(button_offset, h), Vector2(button_offset + h, 0), Color(0.8, 0.8, 0.8));
	}
}

void GradientEdit::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		draw_spacing = BASE_SPACING * get_theme_default_base_scale();
		handle_width = BASE_HANDLE_WIDTH * get_theme_default_base_scale();
	} break;
	case NOTIFICATION_DRAW: {
		_redraw();
	} break;
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			grabbing = GRAB_NONE;
		}
	} break;
	}
}

const int GradientEditor::DEFAULT_SNAP = 10;

void GradientEditor::_set_snap_enabled(bool p_enabled)
{
	gradient_editor_rect->set_snap_enabled(p_enabled);
	snap_count_edit->set_visible(p_enabled);
}

void GradientEditor::_set_snap_count(int p_count)
{
	gradient_editor_rect->set_snap_count(CLAMP(p_count, 2, 100));
}

void GradientEditor::set_gradient(const Ref<Gradient>& p_gradient)
{
	gradient_editor_rect->set_gradient(p_gradient);
}


