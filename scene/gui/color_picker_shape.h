/**************************************************************************/
/*  color_picker_shape.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#pragma once

#include "scene/gui/color_picker.h"

class ColorPickerShape
{
	static constexpr float WHEEL_RADIUS = 0.42f;

	static inline Ref<Shader> wheel_shader;
	static inline Ref<Shader> circle_shader;
	static inline Ref<Shader> circle_ok_color_shader;
	static inline Ref<Shader> rectangle_ok_color_hs_shader;
	static inline Ref<Shader> rectangle_ok_color_hl_shader;

	ColorPicker* color_picker = nullptr;
	ColorPicker::PickerShapeType shape_type = ColorPicker::SHAPE_NONE;
	bool is_dragging = false;

	// HSV Rectangle controls.
	Control* sv_square = nullptr;
	Control* hue_slider = nullptr;

	// OK HS / HL Rectangle controls.
	Control* rectangle_margin = nullptr;
	Control* square = nullptr;
	Control* square_overlay = nullptr;

	// HSV Wheel controls & state.
	Control* wheel = nullptr;
	Control* wheel_uv = nullptr;
	bool wheel_focused = true;
	bool spinning = false;
	bool rotate_next_echo_event = false;

	// VHS / OKHSL Circle controls & state (value_slider shared with OK rectangles).
	Control* circle = nullptr;
	Control* circle_overlay = nullptr;
	Control* value_slider = nullptr;
	Vector2i circle_keyboard_joypad_picker_cursor_position;

	void _emit_color_changed();

	Ref<Shader> _get_shader() const;
	void _initialize_controls();
	void _update_cursor(const Vector2& p_color_change_vector, bool p_is_echo);

	bool can_handle(
		const InputEvent& p_event, Vector2& r_position, bool* r_is_click = nullptr);
	void cancel_event();

	void draw_focus_rect(Control* p_control, const Rect2& p_rect = Rect2());
	void draw_focus_circle(Control* p_control);
	void draw_sv_square(Control* p_control, const Rect2& p_square, bool p_draw_focus = true);
	void draw_cursor(Control* p_control, const Vector2& p_center, bool p_draw_bg = true);
	void draw_circle_cursor(Control* p_control, float p_hue, float p_saturation);

	void connect_shape_focus(Control* p_shape);
	void shape_focus_entered();
	void shape_focus_exited();

	void handle_cursor_editing(const InputEvent& p_event, Control* p_control);
	int get_edge_h_change(const Vector2& p_color_change_vector);
	float get_h_on_circle_edge(const Vector2& p_color_change_vector);
	float _get_h_on_wheel(const Vector2& p_color_change_vector);
	void _reset_wheel_focus();
	void update_circle_cursor(
		const Vector2& p_color_change_vector, const Vector2& p_center, const Vector2& p_hue_offset);

	// Draw & input callbacks dispatched by shape_type.
	void _sv_square_draw();
	void _hue_slider_draw();
	void _square_draw();
	void _square_overlay_draw();
	void _value_slider_draw();
	void _wheel_input(const InputEvent& p_event);
	void _wheel_draw();
	void _wheel_uv_draw();
	void _circle_input(const InputEvent& p_event);
	void _value_slider_input(const InputEvent& p_event);
	void _circle_draw();
	void _circle_overlay_draw();

public:
	Vector<Control*> controls;
	bool is_initialized = false;
	bool cursor_editing = false;
	float echo_multiplier = 1.0f;

	static void init_shaders();
	static void finish_shaders();

	ColorPicker::PickerShapeType get_shape_type() const { return shape_type; }
	String get_name() const;
	Ref<Texture2D> get_icon() const;
	bool is_ok_hsl() const;

	void initialize_controls();
	void update_theme();
	void grab_focus();

	ColorPickerShape(ColorPicker* p_color_picker, ColorPicker::PickerShapeType p_type);
};
