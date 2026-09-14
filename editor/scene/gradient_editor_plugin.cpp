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

void GradientEdit::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		draw_spacing = BASE_SPACING * get_theme_default_base_scale();
		handle_width = BASE_HANDLE_WIDTH * get_theme_default_base_scale();
	} break;
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_visible()) {
			grabbing = GRAB_NONE;
		}
	} break;
	}
}

const int GradientEditor::DEFAULT_SNAP = 10;

void GradientEditor::_set_snap_count(int p_count)
{
	gradient_editor_rect->set_snap_count(CLAMP(p_count, 2, 100));
}

void GradientEditor::set_gradient(const Ref<Gradient>& p_gradient)
{
	gradient_editor_rect->set_gradient(p_gradient);
}


