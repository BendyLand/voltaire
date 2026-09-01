/**************************************************************************/
/*  particle_process_material_editor_plugin.cpp                           */
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

#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_theme_manager.h"
#include "particle_process_material_editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/resources/particle_process_material.h"

void ParticleProcessMaterialMinMaxPropertyEditor::_update_sizing()
{
	edit_size = range_edit_widget->get_size();
	margin = Vector2(range_slider_left_icon->get_width(),
		(edit_size.y - range_slider_left_icon->get_height()) * 0.5);
	usable_area = edit_size - margin * 2;
}

void ParticleProcessMaterialMinMaxPropertyEditor::_range_edit_draw()
{
	ERR_FAIL_COND(range_slider_left_icon.is_null());
	ERR_FAIL_COND(range_slider_right_icon.is_null());
	_update_sizing();

	bool widget_active = mouse_inside || drag != Drag::NONE;

	// FIXME: Need to offset by 1 due to some outline bug.
	range_edit_widget->draw_rect(Rect2(margin + Vector2(1, 1), usable_area - Vector2(1, 1)),
		widget_active ? background_color.lerp(normal_color, 0.3) : background_color, false, 1.0);

	Color draw_color;

	if (widget_active) {
		float icon_offset = _get_left_offset() - range_slider_left_icon->get_width() - 1;

		if (drag == Drag::LEFT || drag == Drag::SCALE) {
			draw_color = drag_color;
		}
		else if (hover == Hover::LEFT) {
			draw_color = hovered_color;
		}
		else {
			draw_color = normal_color;
		}
		range_edit_widget->draw_texture(
			range_slider_left_icon.ptr(), Vector2(icon_offset, margin.y), draw_color);

		icon_offset = _get_right_offset();

		if (drag == Drag::RIGHT || drag == Drag::SCALE) {
			draw_color = drag_color;
		}
		else if (hover == Hover::RIGHT) {
			draw_color = hovered_color;
		}
		else {
			draw_color = normal_color;
		}
		range_edit_widget->draw_texture(
			range_slider_right_icon.ptr(), Vector2(icon_offset, margin.y), draw_color);
	}

	if (drag == Drag::MIDDLE || drag == Drag::SCALE) {
		draw_color = drag_color;
	}
	else if (hover == Hover::MIDDLE) {
		draw_color = hovered_color;
	}
	else {
		draw_color = normal_color;
	}
	range_edit_widget->draw_rect(_get_middle_rect(), draw_color);

	Rect2 midpoint_rect(
		Vector2(margin.x + usable_area.x * (_get_min_ratio() + _get_max_ratio()) * 0.5 - 1,
			margin.y + 2),
		Vector2(2, usable_area.y - 4));

	range_edit_widget->draw_rect(midpoint_rect, midpoint_color);
}

void ParticleProcessMaterialMinMaxPropertyEditor::_range_edit_gui_input(
	const Ref<InputEvent>& p_event)
{
	Ref<InputEventMouseButton> mb = p_event;
	Ref<InputEventMouseMotion> mm = p_event;

	// Prevent unnecessary computations.
	if ((mb.is_null() || mb->get_button_index() != MouseButton::LEFT) && (mm.is_null())) {
		return;
	}

	ERR_FAIL_COND(range_slider_left_icon.is_null());
	ERR_FAIL_COND(range_slider_right_icon.is_null());
	_update_sizing();

	if (mb.is_valid()) {
		const Drag prev_drag = drag;

		if (mb->is_pressed()) {
			if (mb->is_shift_pressed()) {
				drag = Drag::SCALE;
				drag_from_value = (max_range->get_value() - min_range->get_value()) * 0.5;
				drag_midpoint = (max_range->get_value() + min_range->get_value()) * 0.5;
			}
			else if (hover == Hover::LEFT) {
				drag = Drag::LEFT;
				drag_from_value = min_range->get_value();
			}
			else if (hover == Hover::RIGHT) {
				drag = Drag::RIGHT;
				drag_from_value = max_range->get_value();
			}
			else {
				drag = Drag::MIDDLE;
				drag_from_value = min_range->get_value();
			}
			drag_origin = mb->get_position().x;
		}
		else {
			drag = Drag::NONE;
		}

		if (drag != prev_drag) {
			range_edit_widget->queue_redraw();
		}
	}

	float property_length = property_range.y - property_range.x;
	if (mm.is_valid()) {
		switch (drag) {
		case Drag::NONE: {
			const Hover prev_hover = hover;
			float left_icon_offset = _get_left_offset() - range_slider_left_icon->get_width() - 1;

			if (Rect2(Vector2(left_icon_offset, 0), range_slider_left_icon->get_size())
					.has_point(mm->get_position())) {
				hover = Hover::LEFT;
			}
			else if (Rect2(Vector2(_get_right_offset(), 0), range_slider_right_icon->get_size())
						   .has_point(mm->get_position())) {
				hover = Hover::RIGHT;
			}
			else if (_get_middle_rect().has_point(mm->get_position())) {
				hover = Hover::MIDDLE;
			}
			else {
				hover = Hover::NONE;
			}

			if (hover != prev_hover) {
				range_edit_widget->queue_redraw();
			}
		} break;

		case Drag::LEFT:
		case Drag::RIGHT: {
			float new_value = drag_from_value + (mm->get_position().x - drag_origin) /
													usable_area.x * property_length;
			if (drag == Drag::LEFT) {
				new_value = MIN(new_value, max_range->get_value());
				_set_clamped_values(new_value, max_range->get_value());
			}
			else {
				new_value = MAX(new_value, min_range->get_value());
				_set_clamped_values(min_range->get_value(), new_value);
			}
		} break;

		case Drag::MIDDLE: {
			float delta = (mm->get_position().x - drag_origin) / usable_area.x * property_length;
			float diff = max_range->get_value() - min_range->get_value();
			delta = CLAMP(drag_from_value + delta, property_range.x, property_range.y - diff) -
					drag_from_value;
			_set_clamped_values(drag_from_value + delta, drag_from_value + delta + diff);
		} break;

		case Drag::SCALE: {
			float delta = (mm->get_position().x - drag_origin) / usable_area.x * property_length +
						  drag_from_value;
			_set_clamped_values(MIN(drag_midpoint, drag_midpoint - delta),
				MAX(drag_midpoint, drag_midpoint + delta));
		} break;
		}
	}
}

void ParticleProcessMaterialMinMaxPropertyEditor::_set_mouse_inside(bool p_inside)
{
	mouse_inside = p_inside;
	if (!p_inside) {
		hover = Hover::NONE;
	}
	range_edit_widget->queue_redraw();
}

float ParticleProcessMaterialMinMaxPropertyEditor::_get_min_ratio() const
{
	return (min_range->get_value() - property_range.x) / (property_range.y - property_range.x);
}

float ParticleProcessMaterialMinMaxPropertyEditor::_get_max_ratio() const
{
	return (max_range->get_value() - property_range.x) / (property_range.y - property_range.x);
}

float ParticleProcessMaterialMinMaxPropertyEditor::_get_left_offset() const
{
	return margin.x + usable_area.x * _get_min_ratio();
}

float ParticleProcessMaterialMinMaxPropertyEditor::_get_right_offset() const
{
	return margin.x + usable_area.x * _get_max_ratio();
}

Rect2 ParticleProcessMaterialMinMaxPropertyEditor::_get_middle_rect() const
{
	if (Math::is_equal_approx(min_range->get_value(), max_range->get_value())) {
		return Rect2();
	}

	return Rect2(Vector2(_get_left_offset() - 1, margin.y),
		Vector2(usable_area.x * (_get_max_ratio() - _get_min_ratio()) + 1, usable_area.y));
}

void ParticleProcessMaterialMinMaxPropertyEditor::_set_clamped_values(float p_min, float p_max)
{
	// This is required for editing widget in case the properties have or_less or or_greater hint.
	min_range->set_value(MAX(p_min, property_range.x));
	max_range->set_value(MIN(p_max, property_range.y));
	_update_slider_values();
	_sync_property();
}

void ParticleProcessMaterialMinMaxPropertyEditor::_sync_property()
{
	const Vector2 value = Vector2(min_range->get_value(), max_range->get_value());
	range_edit_widget->queue_redraw();
}

float ParticleProcessMaterialMinMaxPropertyEditor::_get_max_spread() const
{
	float max_spread = max_range->get_max() - min_range->get_min();

	if (max_edit->is_greater_allowed()) {
		return max_spread;
	}

	if (!min_edit->is_lesser_allowed()) {
		max_spread = MIN(max_spread, min_edit->get_value() - min_edit->get_min());
	}

	if (!min_edit->is_greater_allowed()) {
		max_spread = MIN(max_spread, min_edit->get_max() - min_edit->get_value());
	}

	return max_spread;
}

void ParticleProcessMaterialMinMaxPropertyEditor::setup(
	float p_min, float p_max, float p_step, bool p_allow_less, bool p_allow_greater, bool p_degrees)
{
	property_range = Vector2(p_min, p_max);

	// Initially all Ranges share properties.
	for (Range* range : Vector<Range*>{min_range, min_edit, max_range, max_edit}) {
		range->set_min(p_min);
		range->set_max(p_max);
		range->set_step(p_step);
		range->set_allow_lesser(p_allow_less);
		range->set_allow_greater(p_allow_greater);
	}

	if (p_degrees) {
		min_edit->set_suffix(U" \u00B0");
		max_edit->set_suffix(U" \u00B0");
	}
	_update_mode();
}


