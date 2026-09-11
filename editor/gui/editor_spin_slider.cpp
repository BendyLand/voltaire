/**************************************************************************/
/*  editor_spin_slider.cpp                                                */
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
#include "core/math/expression.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_spin_slider.h"
#include "scene/theme/theme_db.h"

void EditorSpinSlider::_value_input_gui_input(const Ref<InputEvent>& p_event)
{
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !read_only) {
		Key code = k->get_keycode();

		switch (code) {
		case Key::UP:
		case Key::DOWN: {
			double step = get_step();
			if (step < 1) {
				double divisor = 1.0 / step;

				if (std::trunc(divisor) == divisor) {
					step = 1.0;
				}
			}

			if (k->is_command_or_control_pressed()) {
				step *= 100.0;
			}
			else if (k->is_shift_pressed()) {
				step *= 10.0;
			}
			else if (k->is_alt_pressed()) {
				step *= 0.1;
			}

			_evaluate_input_text();

			double last_value = get_value();
			if (code == Key::DOWN) {
				step *= -1;
			}
			set_value(last_value + step);

			value_input_dirty = true;
			set_process_internal(true);
		} break;
		case Key::ESCAPE: {
			value_input_closed_frame = Engine::get_singleton()->get_frames_drawn();
			if (value_input) {
				value_input_focus_visible = value_input->has_focus(true);
				value_input->hide();
			}
		} break;
		default:
			break;
		}
	}
}

LineEdit* EditorSpinSlider::get_line_edit()
{
	_ensure_value_input();
	return value_input;
}

void EditorSpinSlider::set_control_state(ControlState p_state)
{
	control_state = p_state;
	queue_redraw();
}

EditorSpinSlider::ControlState EditorSpinSlider::get_control_state() const { return control_state; }

#ifndef DISABLE_DEPRECATED
void EditorSpinSlider::set_hide_slider(bool p_hide)
{
	set_control_state(p_hide ? CONTROL_STATE_HIDE : CONTROL_STATE_DEFAULT);
}

bool EditorSpinSlider::is_hiding_slider() const { return control_state == CONTROL_STATE_HIDE; }
#endif

bool EditorSpinSlider::is_editing_integer() const { return editing_integer; }

void EditorSpinSlider::set_label(const String& p_label)
{
	label = p_label;
	queue_redraw();
}

String EditorSpinSlider::get_label() const { return label; }

void EditorSpinSlider::set_suffix(const String& p_suffix)
{
	suffix = p_suffix;
	queue_redraw();
}

String EditorSpinSlider::get_suffix() const { return suffix; }

void EditorSpinSlider::_value_input_submitted(const String& p_text)
{
	value_input_closed_frame = Engine::get_singleton()->get_frames_drawn();
	if (value_input) {
		value_input_focus_visible = value_input->has_focus(true);
		value_input->hide();
	}
}

void EditorSpinSlider::_value_input_hidden()
{
	_evaluate_input_text();
	value_input_closed_frame = Engine::get_singleton()->get_frames_drawn();
}

void EditorSpinSlider::_grabber_mouse_entered()
{
	mouse_over_grabber = true;
	queue_redraw();
}

void EditorSpinSlider::_grabber_mouse_exited()
{
	mouse_over_grabber = false;
	queue_redraw();
}

void EditorSpinSlider::set_read_only(bool p_enable)
{
	read_only = p_enable;
	if (read_only && value_input && value_input->is_inside_tree()) {
		value_input->release_focus();
	}

	queue_redraw();
}

bool EditorSpinSlider::is_read_only() const { return read_only; }

void EditorSpinSlider::set_flat(bool p_enable)
{
	flat = p_enable;
	queue_redraw();
}

bool EditorSpinSlider::is_flat() const { return flat; }

bool EditorSpinSlider::is_grabbing() const { return grabbing_grabber || grabbing_spinner; }

void EditorSpinSlider::set_deferred_drag_mode_enabled(bool p_enabled)
{
	deferred_drag_mode = p_enabled;
}

bool EditorSpinSlider::is_deferred_drag_mode_enabled() const { return deferred_drag_mode; }


