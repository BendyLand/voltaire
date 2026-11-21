/**************************************************************************/
/*  color_picker.cpp                                                      */
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

#include "color_picker.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/expression.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/color_palette.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "core/math/color.h"
#include "scene/resources/gradient_texture.h"
#include <thirdparty/misc/ok_color_shader.h>
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"
#include "color_rect.h"

#ifdef MACOS_ENABLED
#include "core/os/os.h"
#endif

// --- color_mode (flattened internal helper) ---

class ColorMode final
{
public:
	ColorPicker* color_picker = nullptr;
	ColorPicker::ColorModeType mode_type = ColorPicker::MODE_RGB;
	float cached_hue = 0.0f;
	float cached_saturation = 0.0f;
	Ref<GradientTexture2D> rgb_texture[3];
	Ref<GradientTexture2D> hue_texture;

	ColorMode(ColorPicker* p_color_picker, ColorPicker::ColorModeType p_type = ColorPicker::MODE_RGB)
		: color_picker(p_color_picker), mode_type(p_type)
	{
	}

	String get_name() const
	{
		switch (mode_type) {
		case ColorPicker::MODE_RGB:
			return "RGB";
		case ColorPicker::MODE_HSV:
			return "HSV";
		case ColorPicker::MODE_LINEAR:
			return ETR("Linear");
		case ColorPicker::MODE_OKHSL:
			return "OKHSL";
		default:
			return String();
		}
	}

	int get_slider_count() const { return 3; }

	float get_slider_step() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 0.001f : 1.0f;
	}

	float get_spinbox_arrow_step() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 0.01f : get_slider_step();
	}

	String get_slider_label(int idx) const
	{
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), String(), "Couldn't get slider label.");
		switch (mode_type) {
		case ColorPicker::MODE_HSV: {
			static const char* const labels[3] = {"H", "S", "V"};
			return labels[idx];
		}
		case ColorPicker::MODE_OKHSL: {
			static const char* const labels[3] = {"H", "S", "L"};
			return labels[idx];
		}
		case ColorPicker::MODE_RGB:
		case ColorPicker::MODE_LINEAR:
		default: {
			static const char* const labels[3] = {"R", "G", "B"};
			return labels[idx];
		}
		}
	}

	float get_slider_max(int idx) const
	{
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider max value.");
		switch (mode_type) {
		case ColorPicker::MODE_RGB:
			return 255.0f;
		case ColorPicker::MODE_LINEAR:
			return 1.0f;
		case ColorPicker::MODE_HSV:
		case ColorPicker::MODE_OKHSL:
		default: {
			static constexpr float slider_max[3] = {359.0f, 100.0f, 100.0f};
			return slider_max[idx];
		}
		}
	}

	bool get_allow_greater() const
	{
		return mode_type == ColorPicker::MODE_RGB || mode_type == ColorPicker::MODE_LINEAR;
	}

	float get_alpha_slider_max() const
	{
		return mode_type == ColorPicker::MODE_LINEAR ? 1.0f : 255.0f;
	}

	float get_alpha_slider_value() const
	{
		return mode_type == ColorPicker::MODE_LINEAR
				   ? color_picker->get_pick_color().a
				   : color_picker->get_pick_color().a * 255.0f;
	}

	float get_slider_value(int idx) const;
	Color get_color() const;
	void _value_changed();
	void _greater_value_inputted();
	void slider_draw(int p_which);
};

float ColorMode::get_slider_value(int idx) const
{
	switch (mode_type) {
	case ColorPicker::MODE_RGB: {
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider value.");
		return color_picker->color_normalized[idx] * 255;
	}
	case ColorPicker::MODE_HSV: {
		switch (idx) {
		case 0:
			return (color_picker->color_normalized.get_s() > 0)
					   ? color_picker->color_normalized.get_h() * 360.0
					   : cached_hue;
		case 1:
			return (color_picker->color_normalized.get_v() > 0)
					   ? color_picker->color_normalized.get_s() * 100.0
					   : cached_saturation;
		case 2:
			return color_picker->color_normalized.get_v() * 100.0;
		default:
			ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
		}
	}
	case ColorPicker::MODE_LINEAR: {
		ERR_FAIL_INDEX_V_MSG(idx, get_slider_count(), 0, "Couldn't get slider value.");
		Color color = color_picker->color_normalized.srgb_to_linear();
		return color[idx];
	}
	case ColorPicker::MODE_OKHSL: {
		switch (idx) {
		case 0:
			return (color_picker->color_normalized.get_ok_hsl_s() > 0)
					   ? color_picker->color_normalized.get_ok_hsl_h() * 360.0
					   : cached_hue;
		case 1:
			return (color_picker->color_normalized.get_ok_hsl_l() > 0)
					   ? color_picker->color_normalized.get_ok_hsl_s() * 100.0
					   : cached_saturation;
		case 2:
			return color_picker->color_normalized.get_ok_hsl_l() * 100.0;
		default:
			ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
		}
	}
	default:
		ERR_FAIL_V_MSG(0, "Couldn't get slider value.");
	}
}

Color ColorMode::get_color() const
{
	Vector<float> values = color_picker->get_active_slider_values();
	switch (mode_type) {
	case ColorPicker::MODE_RGB: {
		Color color;
		for (int i = 0; i < 4; i++) {
			color[i] = values[i] / 255.0;
		}
		return color;
	}
	case ColorPicker::MODE_HSV:
		return Color::from_hsv(
			values[0] / 360.0, values[1] / 100.0, values[2] / 100.0, values[3] / 255.0);
	case ColorPicker::MODE_LINEAR: {
		Color color;
		for (int i = 0; i < 4; i++) {
			color[i] = values[i];
		}
		return color.linear_to_srgb();
	}
	case ColorPicker::MODE_OKHSL:
		return Color::from_ok_hsl(
			values[0] / 360.0, values[1] / 100.0, values[2] / 100.0, values[3] / 255.0);
	default:
		return Color();
	}
}

void ColorMode::_value_changed()
{
	if (mode_type == ColorPicker::MODE_HSV) {
		Vector<float> values = color_picker->get_active_slider_values();

		if (values[1] > 0 || values[0] != cached_hue) {
			cached_hue = values[0];
		}
		if (values[2] > 0 || values[1] != cached_saturation) {
			cached_saturation = values[1];
		}

		color_picker->h = color_picker->sliders[0]->get_value() / 360.0;
		color_picker->s = color_picker->sliders[1]->get_value() / 100.0;
		color_picker->v = color_picker->sliders[2]->get_value() / 100.0;

		color_picker->hsv_cached = true;
	}
	else if (mode_type == ColorPicker::MODE_OKHSL) {
		Vector<float> values = color_picker->get_active_slider_values();

		if (values[1] > 0 || values[0] != cached_hue) {
			cached_hue = values[0];
		}
		if (values[2] > 0 || values[1] != cached_saturation) {
			cached_saturation = values[1];
		}

		color_picker->ok_hsl_h = color_picker->sliders[0]->get_value() / 360.0;
		color_picker->ok_hsl_s = color_picker->sliders[1]->get_value() / 100.0;
		color_picker->ok_hsl_l = color_picker->sliders[2]->get_value() / 100.0;

		color_picker->okhsl_cached = true;
	}
}

void ColorMode::_greater_value_inputted()
{
	if (mode_type == ColorPicker::MODE_RGB) {
		Slider** sliders = color_picker->sliders;
		Color color_prev = color_picker->color;
		for (int i = 0; i < 3; i++) {
			if (sliders[i]->get_value() > 255) {
				color_prev[i] = sliders[i]->get_value() / 255.0;
			}
		}
		Color linear_color = color_prev.srgb_to_linear();
		float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));
		Color srgb = Color(linear_color.r / multiplier, linear_color.g / multiplier,
			linear_color.b / multiplier, linear_color.a)
						 .linear_to_srgb();
		sliders[0]->set_value_no_signal(srgb.r * 255);
		sliders[1]->set_value_no_signal(srgb.g * 255);
		sliders[2]->set_value_no_signal(srgb.b * 255);

		color_picker->intensity = Math::log2(multiplier);
		color_picker->intensity_slider->set_value_no_signal(color_picker->intensity);
	}
	else if (mode_type == ColorPicker::MODE_LINEAR) {
		Slider** sliders = color_picker->sliders;
		Color color_prev = color_picker->color;
		Color linear_color = color_prev.srgb_to_linear();
		for (int i = 0; i < 3; i++) {
			if (sliders[i]->get_value() > 1 + CMP_EPSILON) {
				linear_color[i] = sliders[i]->get_value();
			}
		}

		float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));

		sliders[0]->set_value_no_signal(linear_color.r / multiplier);
		sliders[1]->set_value_no_signal(linear_color.g / multiplier);
		sliders[2]->set_value_no_signal(linear_color.b / multiplier);

		color_picker->intensity = Math::log2(multiplier);
		color_picker->intensity_slider->set_value_no_signal(color_picker->intensity);
	}
}

void ColorMode::slider_draw(int p_which)
{
	Slider* slider = color_picker->get_slider(p_which);
	Size2 size = slider->get_size();
	Color color = color_picker->color_normalized;
	const real_t margin = 16 * color_picker->theme_cache.base_scale;

	switch (mode_type) {
	case ColorPicker::MODE_RGB:
	case ColorPicker::MODE_LINEAR: {
		Color left_color = Color(
			p_which == 0 ? 0 : color.r, p_which == 1 ? 0 : color.g, p_which == 2 ? 0 : color.b);
		Color right_color = Color(
			p_which == 0 ? 1 : color.r, p_which == 1 ? 1 : color.g, p_which == 2 ? 1 : color.b);

		if (rgb_texture[p_which].is_null()) {
			rgb_texture[p_which].instantiate();
			rgb_texture[p_which]->set_width(400);
			rgb_texture[p_which]->set_height(6);
		}

		Ref<GradientTexture2D> gradient_texture = rgb_texture[p_which];
		Ref<Gradient> gradient = gradient_texture->get_gradient();
		if (gradient.is_null()) {
			gradient.instantiate();
			PackedFloat32Array offsets = {0, 1};
			gradient->set_offsets(offsets);
			gradient->set_interpolation_color_space(mode_type == ColorPicker::MODE_LINEAR
														? Gradient::ColorSpace::GRADIENT_COLOR_SPACE_LINEAR_SRGB
														: Gradient::ColorSpace::GRADIENT_COLOR_SPACE_SRGB);
			gradient_texture->set_gradient(gradient);
		}

		PackedColorArray colors = {left_color, right_color};
		gradient->set_colors(colors);

		slider->draw_texture_rect(
			gradient_texture.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false);
	} break;

	case ColorPicker::MODE_HSV: {
		Vector<Vector2> pos;
		pos.resize(4);
		Vector<Color> col;
		col.resize(4);
		Color left_color;
		Color right_color;

		if (p_which == 0) {
			float v = color.get_v();
			left_color = Color(v, v, v);
			right_color = left_color;
		}
		else {
			Color s_col;
			Color v_col;
			s_col.set_hsv(color.get_h(), 0, color.get_v());
			left_color = (p_which == 1) ? s_col : Color(0, 0, 0);

			float s_col_hue =
				(Math::is_zero_approx(color.get_s())) ? cached_hue / 360.0 : color.get_h();
			s_col.set_hsv(s_col_hue, 1, color.get_v());
			v_col.set_hsv(color.get_h(), color.get_s(), 1);
			right_color = (p_which == 1) ? s_col : v_col;
		}
		col.set(0, left_color);
		col.set(1, right_color);
		col.set(2, right_color);
		col.set(3, left_color);
		pos.set(0, Vector2(0, 0));
		pos.set(1, Vector2(size.x, 0));
		pos.set(2, Vector2(size.x, margin));
		pos.set(3, Vector2(0, margin));

		slider->draw_polygon(pos, col);

		if (p_which == 0) {
			Ref<Texture2D> hue = color_picker->theme_cache.color_hue;
			slider->draw_texture_rect(hue.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false,
				Color::from_hsv(0, 0, color.get_v(), color.get_s()));
		}
	} break;

	case ColorPicker::MODE_OKHSL: {
		Vector<Vector2> pos;
		Vector<Color> col;
		Color left_color;
		Color right_color;
		float okhsl_l = color.get_ok_hsl_l();
		float slider_hue =
			(Math::is_zero_approx(color.get_ok_hsl_s())) ? cached_hue / 360.0 : color.get_ok_hsl_h();
		float slider_sat = (Math::is_zero_approx(okhsl_l) || Math::is_equal_approx(okhsl_l, 1))
							   ? cached_saturation / 100.0
							   : color.get_ok_hsl_s();

		if (p_which == 2) {
			pos.resize(6);
			col.resize(6);
			left_color = Color(0, 0, 0);
			Color middle_color = Color::from_ok_hsl(slider_hue, slider_sat, 0.5);
			right_color.set_ok_hsl(slider_hue, slider_sat, 1);

			col.set(0, left_color);
			col.set(1, middle_color);
			col.set(2, right_color);
			col.set(3, right_color);
			col.set(4, middle_color);
			col.set(5, left_color);
			pos.set(0, Vector2(0, 0));
			pos.set(1, Vector2(size.x * 0.5, 0));
			pos.set(2, Vector2(size.x, 0));
			pos.set(3, Vector2(size.x, margin));
			pos.set(4, Vector2(size.x * 0.5, margin));
			pos.set(5, Vector2(0, margin));
			slider->draw_polygon(pos, col);
		}
		else if (p_which == 1) {
			pos.resize(4);
			col.resize(4);

			left_color.set_ok_hsl(slider_hue, 0, okhsl_l);
			right_color.set_ok_hsl(slider_hue, 1, okhsl_l);

			col.set(0, left_color);
			col.set(1, right_color);
			col.set(2, right_color);
			col.set(3, left_color);
			pos.set(0, Vector2(0, 0));
			pos.set(1, Vector2(size.x, 0));
			pos.set(2, Vector2(size.x, margin));
			pos.set(3, Vector2(0, margin));
			slider->draw_polygon(pos, col);
		}
		else if (p_which == 0) {
			const int precision = 7;
			if (hue_texture.is_null()) {
				hue_texture.instantiate();
				hue_texture->set_width(400);
				hue_texture->set_height(6);
			}
			Ref<Gradient> hue_gradient = hue_texture->get_gradient();
			if (hue_gradient.is_null()) {
				hue_gradient.instantiate();
				PackedFloat32Array offsets;
				offsets.resize(precision);
				for (int i = 0; i < precision; i++) {
					float h = i / float(precision - 1);
					offsets.write[i] = h;
				}
				hue_gradient->set_offsets(offsets);
				hue_gradient->set_interpolation_color_space(
					Gradient::ColorSpace::GRADIENT_COLOR_SPACE_OKLAB);
				hue_texture->set_gradient(hue_gradient);
			}

			PackedColorArray colors;
			colors.resize(precision);

			for (int i = 0; i < precision; i++) {
				float h = i / float(precision - 1);
				colors.write[i] = Color::from_ok_hsl(h, slider_sat, okhsl_l);
			}
			hue_gradient->set_colors(colors);
			slider->draw_texture_rect(
				hue_texture.ptr(), Rect2(Vector2(), Vector2(size.x, margin)), false);
		}
	} break;

	default:
		break;
	}
}

// --- color_picker_shape (internal helper) ---

class ColorPickerShape final
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

void ColorPickerShape::init_shaders()
{
	wheel_shader.instantiate();
	wheel_shader->set_code(R"(
// ColorPicker wheel shader.

shader_type canvas_item;

uniform float wheel_radius = 0.42;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5) * float(sqrt(x * x + y * y) > wheel_radius);

	COLOR = vec4(clamp((abs(fract(((a - TAU) / TAU) + vec3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), (b + b2 + b3 + b4) / 4.00);
}
)");

	circle_shader.instantiate();
	circle_shader->set_code(R"(
// ColorPicker circle shader.

shader_type canvas_item;

uniform float v = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float a = atan(y, x);
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);

	COLOR = vec4(mix(vec3(1.0), clamp(abs(fract(vec3((a - TAU) / TAU) + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - vec3(3.0)) - vec3(1.0), 0.0, 1.0), ((float(sqrt(x * x + y * y)) * 2.0)) / 1.0) * vec3(v), (b + b2 + b3 + b4) / 4.00);
})");

	circle_ok_color_shader.instantiate();
	circle_ok_color_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hsl circle shader.

uniform float ok_hsl_l = 1.0;

void fragment() {
	float x = UV.x - 0.5;
	float y = UV.y - 0.5;
	float h = atan(y, x) / (2.0 * M_PI);
	float s = sqrt(x * x + y * y) * 2.0;
	vec3 col = okhsl_to_srgb(vec3(h, s, ok_hsl_l));
	x += 0.001;
	y += 0.001;
	float b = float(sqrt(x * x + y * y) < 0.5);
	x -= 0.002;
	float b2 = float(sqrt(x * x + y * y) < 0.5);
	y -= 0.002;
	float b3 = float(sqrt(x * x + y * y) < 0.5);
	x += 0.002;
	float b4 = float(sqrt(x * x + y * y) < 0.5);
	COLOR = vec4(col, (b + b2 + b3 + b4) / 4.00);
})");

	rectangle_ok_color_hs_shader.instantiate();
	rectangle_ok_color_hs_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hs rectangle shader.

uniform float ok_hsl_l = 0.0;

void fragment() {
	float h = UV.x;
	float s = 1.0 - UV.y;
	vec3 col = okhsl_to_srgb(vec3(h, s, ok_hsl_l));
	COLOR = vec4(col, 1.0);
})");

	rectangle_ok_color_hl_shader.instantiate();
	rectangle_ok_color_hl_shader->set_code(OK_COLOR_SHADER + R"(
// ColorPicker ok color hl rectangle shader.

uniform float ok_hsl_s = 0.0;

void fragment() {
	float h = UV.x;
	float l = 1.0 - UV.y;
	vec3 col = okhsl_to_srgb(vec3(h, ok_hsl_s, l));
	COLOR = vec4(col, 1.0);
})");
}

void ColorPickerShape::finish_shaders()
{
	wheel_shader.unref();
	circle_shader.unref();
	circle_ok_color_shader.unref();
	rectangle_ok_color_hs_shader.unref();
	rectangle_ok_color_hl_shader.unref();
}

ColorPickerShape::ColorPickerShape(
	ColorPicker* p_color_picker, ColorPicker::PickerShapeType p_type)
	: color_picker(p_color_picker), shape_type(p_type)
{
}

String ColorPickerShape::get_name() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
		return ETR("HSV Rectangle");
	case ColorPicker::SHAPE_HSV_WHEEL:
		return ETR("HSV Wheel");
	case ColorPicker::SHAPE_VHS_CIRCLE:
		return ETR("VHS Circle");
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return ETR("OKHSL Circle");
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
		return ETR("OK HS Rectangle");
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return ETR("OK HL Rectangle");
	default:
		return String();
	}
}

Ref<Texture2D> ColorPickerShape::get_icon() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return color_picker->theme_cache.shape_rect;
	case ColorPicker::SHAPE_HSV_WHEEL:
		return color_picker->theme_cache.shape_rect_wheel;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return color_picker->theme_cache.shape_circle;
	default:
		return Ref<Texture2D>();
	}
}

bool ColorPickerShape::is_ok_hsl() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return true;
	default:
		return false;
	}
}

Ref<Shader> ColorPickerShape::_get_shader() const
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_WHEEL:
		return wheel_shader;
	case ColorPicker::SHAPE_VHS_CIRCLE:
		return circle_shader;
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		return circle_ok_color_shader;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
		return rectangle_ok_color_hs_shader;
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		return rectangle_ok_color_hl_shader;
	default:
		return Ref<Shader>();
	}
}

bool ColorPickerShape::can_handle(
	const InputEvent& p_event, Vector2& r_position, bool* r_is_click)
{
	if (p_event.type == InputEventType::MOUSE_BUTTON) {
		if (p_event.mouse_button.button_index != MouseButton::LEFT) {
			return false;
		}

		if (r_is_click) {
			*r_is_click = true;
		}

		if (p_event.is_pressed()) {
			is_dragging = true;
			r_position = p_event.mouse_button.position;
			return true;
		}
	}

	if (is_dragging && p_event.type == InputEventType::MOUSE_MOTION) {
		r_position = p_event.mouse_motion.position;
		return true;
	}
	return false;
}

void ColorPickerShape::cancel_event() { is_dragging = false; }

void ColorPickerShape::_emit_color_changed() {}

void ColorPickerShape::connect_shape_focus(Control*) {}

void ColorPickerShape::handle_cursor_editing(const InputEvent&, Control*) {}

void ColorPickerShape::draw_focus_rect(Control* p_control, const Rect2& p_rect)
{
	if (!p_control->has_focus(true)) {
		return;
	}

	Rect2 focus_rect;
	if (p_rect.has_area()) {
		focus_rect = p_rect;
	}
	else {
		focus_rect = Rect2(Vector2(), p_control->get_size());
	}

	const RID ci = p_control->get_canvas_item();
	if (!cursor_editing) {
		Renderer::canvas_item_add_rect(
			ci, focus_rect, color_picker->theme_cache.focused_not_editing_cursor_color);
	}
	color_picker->theme_cache.picker_focus_rectangle->draw(ci, focus_rect);
}

void ColorPickerShape::draw_focus_circle(Control* p_control)
{
	if (!p_control->has_focus(true)) {
		return;
	}

	const Rect2 focus_rect(Vector2(), p_control->get_size());
	const RID ci = p_control->get_canvas_item();
	if (!cursor_editing) {
		Renderer::canvas_item_add_circle(ci, focus_rect.get_center(),
			focus_rect.get_size().y * 0.5,
			color_picker->theme_cache.focused_not_editing_cursor_color);
	}
	color_picker->theme_cache.picker_focus_circle->draw(ci, focus_rect);
}

void ColorPickerShape::draw_sv_square(Control* p_control, const Rect2& p_square, bool p_draw_focus)
{
	const Vector2 end = p_square.get_end();
	PackedVector2Array points = {
		p_square.position,
		Vector2(end.x, p_square.position.y),
		end,
		Vector2(p_square.position.x, end.y),
	};

	Color color1 = Color::from_hsv(color_picker->h, 1, 1);
	Color color2 = Color::from_hsv(color_picker->h, 1, 0);

	PackedColorArray colors = {
		Color(1, 1, 1, 1),
		Color(1, 1, 1, 1),
		Color(0, 0, 0, 1),
		Color(0, 0, 0, 1),
	};
	p_control->draw_polygon(points, colors);

	colors = {
		Color(color1, 0),
		Color(color1, 1),
		Color(color2, 1),
		Color(color2, 0),
	};
	p_control->draw_polygon(points, colors);

	Vector2 cursor_pos;
	cursor_pos.x =
		CLAMP(p_square.position.x + p_square.size.x * color_picker->s, p_square.position.x, end.x);
	cursor_pos.y = CLAMP(p_square.position.y + p_square.size.y * (1.0 - color_picker->v),
		p_square.position.y, end.y);

	if (p_draw_focus) {
		draw_focus_rect(p_control, p_square);
	}
	draw_cursor(p_control, cursor_pos);
}

void ColorPickerShape::draw_cursor(Control* p_control, const Vector2& p_center, bool p_draw_bg)
{
	const Vector2 position = p_center - color_picker->theme_cache.picker_cursor->get_size() * 0.5;
	if (p_draw_bg) {
		p_control->draw_texture(color_picker->theme_cache.picker_cursor_bg.ptr(), position,
			Color(color_picker->color, 1.0));
	}
	p_control->draw_texture(color_picker->theme_cache.picker_cursor.ptr(), position);
}

void ColorPickerShape::draw_circle_cursor(Control* p_control, float p_hue, float p_saturation)
{
	const Vector2 center = p_control->get_size() * 0.5;
	const Vector2 cursor_pos(center.x + (center.x * Math::cos(p_hue * Math::TAU) * p_saturation),
		center.y + (center.y * Math::sin(p_hue * Math::TAU) * p_saturation));

	draw_cursor(p_control, cursor_pos);
}

void ColorPickerShape::shape_focus_entered()
{
	if (!(Input::is_action_pressed("ui_up") || Input::is_action_pressed("ui_down") ||
			Input::is_action_pressed("ui_left") || Input::is_action_pressed("ui_right"))) {
		cursor_editing = true;
	}
}

void ColorPickerShape::shape_focus_exited() { cursor_editing = false; }

int ColorPickerShape::get_edge_h_change(const Vector2& p_color_change_vector)
{
	int h_change = 0;

	if (color_picker->h > 0 && color_picker->h < 0.5) {
		h_change -= p_color_change_vector.x;
	}
	else if (color_picker->h > 0.5 && color_picker->h < 1) {
		h_change += p_color_change_vector.x;
	}

	if (color_picker->h > 0.25 && color_picker->h < 0.75) {
		h_change -= p_color_change_vector.y;
	}
	else if (color_picker->h < 0.25 || color_picker->h > 0.75) {
		h_change += p_color_change_vector.y;
	}
	return h_change;
}

float ColorPickerShape::get_h_on_circle_edge(const Vector2& p_color_change_vector)
{
	int h_change = get_edge_h_change(p_color_change_vector);

	float target_h = Math::wrapf(color_picker->h + h_change / 360.0, 0, 1);
	int current_quarter = color_picker->h * 4;
	int future_quarter = target_h * 4;
	if (p_color_change_vector.y > 0 && ((future_quarter == 0 && current_quarter == 1) ||
										   (future_quarter == 1 && current_quarter == 0))) {
		target_h = 0.25f;
	}
	else if (p_color_change_vector.y < 0 && ((future_quarter == 2 && current_quarter == 3) ||
												  (future_quarter == 3 && current_quarter == 2))) {
		target_h = 0.75f;
	}
	else if (p_color_change_vector.x < 0 && ((future_quarter == 1 && current_quarter == 2) ||
												  (future_quarter == 2 && current_quarter == 1))) {
		target_h = 0.5f;
	}
	else if (p_color_change_vector.x > 0 && ((future_quarter == 3 && current_quarter == 0) ||
												  (future_quarter == 0 && current_quarter == 3))) {
		target_h = 0;
	}
	return target_h;
}

float ColorPickerShape::_get_h_on_wheel(const Vector2& p_color_change_vector)
{
	int h_change = get_edge_h_change(p_color_change_vector);

	float target_h = Math::wrapf(color_picker->h + h_change / 360.0, 0, 1);
	int current_quarter = color_picker->h * 4;
	int future_quarter = target_h * 4;

	if (p_color_change_vector.y > 0 && ((future_quarter == 0 && current_quarter == 1) ||
										   (future_quarter == 1 && current_quarter == 0))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.y < 0 && ((future_quarter == 2 && current_quarter == 3) ||
												  (future_quarter == 3 && current_quarter == 2))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.x < 0 && ((future_quarter == 1 && current_quarter == 2) ||
												  (future_quarter == 2 && current_quarter == 1))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}
	else if (p_color_change_vector.x > 0 && ((future_quarter == 3 && current_quarter == 0) ||
												  (future_quarter == 0 && current_quarter == 3))) {
		rotate_next_echo_event = !rotate_next_echo_event;
	}

	return target_h;
}

void ColorPickerShape::_reset_wheel_focus() { wheel_focused = true; }

void ColorPickerShape::update_circle_cursor(
	const Vector2& p_color_change_vector, const Vector2& p_center, const Vector2& p_hue_offset)
{
	if (circle_keyboard_joypad_picker_cursor_position == Vector2i()) {
		circle_keyboard_joypad_picker_cursor_position = p_center + p_hue_offset;
	}

	Vector2i potential_cursor_position =
		circle_keyboard_joypad_picker_cursor_position + p_color_change_vector;
	real_t potential_new_cursor_distance = p_center.distance_to(potential_cursor_position);
	real_t dist_pre = p_center.distance_to(circle_keyboard_joypad_picker_cursor_position);
	if (color_picker->s < 1 || potential_new_cursor_distance < dist_pre) {
		circle_keyboard_joypad_picker_cursor_position += p_color_change_vector;
		real_t dist = p_center.distance_to(circle_keyboard_joypad_picker_cursor_position);
		real_t rad = p_center.angle_to_point(circle_keyboard_joypad_picker_cursor_position);
		color_picker->h = ((rad >= 0) ? rad : (Math::TAU + rad)) / Math::TAU;
		color_picker->s = CLAMP(dist / p_center.x, 0, 1);
	}
	else {
		color_picker->h = get_h_on_circle_edge(p_color_change_vector);
		circle_keyboard_joypad_picker_cursor_position = Vector2i();
	}
}

void ColorPickerShape::initialize_controls()
{
	_initialize_controls();
	update_theme();
	is_initialized = true;
}

void ColorPickerShape::_initialize_controls()
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		sv_square = memnew(Control);
		hue_slider = memnew(Control);
		controls.push_back(sv_square);
		controls.push_back(hue_slider);
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		rectangle_margin = memnew(Control);
		square = memnew(Control);
		square_overlay = memnew(Control);
		value_slider = memnew(Control);
		rectangle_margin->add_child(square);
		square->add_child(square_overlay);
		controls.push_back(rectangle_margin);
		controls.push_back(value_slider);
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		wheel = memnew(Control);
		wheel_uv = memnew(Control);
		wheel->add_child(wheel_uv);
		controls.push_back(wheel);
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		circle = memnew(Control);
		circle_overlay = memnew(Control);
		value_slider = memnew(Control);
		circle->add_child(circle_overlay);
		controls.push_back(circle);
		controls.push_back(value_slider);
	} break;
	default:
		break;
	}
}

void ColorPickerShape::update_theme()
{
	const ColorPicker::ThemeCache& theme_cache = color_picker->theme_cache;
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		if (sv_square) {
			sv_square->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (hue_slider) {
			hue_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		if (rectangle_margin) {
			rectangle_margin->set_custom_minimum_size(
				Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (value_slider) {
			value_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		if (wheel) {
			wheel->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		if (circle) {
			circle->set_custom_minimum_size(Size2(theme_cache.sv_width, theme_cache.sv_height));
		}
		if (value_slider) {
			value_slider->set_custom_minimum_size(Size2(theme_cache.h_width, 0));
		}
	} break;
	default:
		break;
	}
}

void ColorPickerShape::grab_focus()
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE:
		if (hue_slider) {
			hue_slider->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OK_HL_RECTANGLE:
		if (square_overlay) {
			square_overlay->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_HSV_WHEEL:
		if (wheel_uv) {
			wheel_uv->grab_focus();
		}
		break;
	case ColorPicker::SHAPE_VHS_CIRCLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE:
		if (circle_overlay) {
			circle_overlay->grab_focus();
		}
		break;
	default:
		break;
	}
}

void ColorPickerShape::_update_cursor(const Vector2& p_color_change_vector, bool p_is_echo)
{
	switch (shape_type) {
	case ColorPicker::SHAPE_HSV_RECTANGLE: {
		if (sv_square && sv_square->has_focus()) {
			color_picker->s = CLAMP(color_picker->s + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->v = CLAMP(color_picker->v - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (hue_slider && hue_slider->has_focus()) {
			color_picker->h =
				CLAMP(color_picker->h + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OK_HS_RECTANGLE: {
		if (square_overlay && square_overlay->has_focus()) {
			color_picker->ok_hsl_h =
				CLAMP(color_picker->ok_hsl_h + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->ok_hsl_s =
				CLAMP(color_picker->ok_hsl_s - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_l = CLAMP(
				color_picker->ok_hsl_l + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		if (square_overlay && square_overlay->has_focus()) {
			color_picker->ok_hsl_h =
				CLAMP(color_picker->ok_hsl_h + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->ok_hsl_l =
				CLAMP(color_picker->ok_hsl_l - p_color_change_vector.y / 100.0, 0, 1);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_s = CLAMP(
				color_picker->ok_hsl_s + p_color_change_vector.y * echo_multiplier / 360.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_HSV_WHEEL: {
		if (wheel_focused) {
			if (p_is_echo && rotate_next_echo_event) {
				color_picker->h = _get_h_on_wheel(-p_color_change_vector);
			}
			else {
				rotate_next_echo_event = false;
				color_picker->h = _get_h_on_wheel(p_color_change_vector);
			}
		}
		else {
			color_picker->s = CLAMP(color_picker->s + p_color_change_vector.x / 100.0, 0, 1);
			color_picker->v = CLAMP(color_picker->v - p_color_change_vector.y / 100.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_VHS_CIRCLE: {
		if (circle_overlay && circle_overlay->has_focus()) {
			const Vector2 center = circle_overlay->get_size() / 2.0;
			const Vector2 hue_offset = center *
									   Vector2(Math::cos(color_picker->h * Math::TAU),
										   Math::sin(color_picker->h * Math::TAU)) *
									   color_picker->s;
			update_circle_cursor(p_color_change_vector, center, hue_offset);
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->v =
				CLAMP(color_picker->v - p_color_change_vector.y * echo_multiplier / 100.0, 0, 1);
		}
	} break;
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		if (circle_overlay && circle_overlay->has_focus()) {
			const Vector2 center = circle_overlay->get_size() / 2.0;
			const Vector2 hue_offset = center *
									   Vector2(Math::cos(color_picker->ok_hsl_h * Math::TAU),
										   Math::sin(color_picker->ok_hsl_h * Math::TAU)) *
									   color_picker->ok_hsl_s;
			update_circle_cursor(p_color_change_vector, center, hue_offset);
			color_picker->ok_hsl_h = color_picker->h;
			color_picker->ok_hsl_s = color_picker->s;
		}
		else if (value_slider && value_slider->has_focus()) {
			color_picker->ok_hsl_l = CLAMP(
				color_picker->ok_hsl_l - p_color_change_vector.y * echo_multiplier / 100.0, 0, 1);
		}
	} break;
	default:
		break;
	}
}

void ColorPickerShape::_sv_square_draw()
{
	draw_sv_square(sv_square, Rect2(Vector2(), sv_square->get_size()));
}

void ColorPickerShape::_hue_slider_draw()
{
	const Vector2 size = hue_slider->get_size();
	hue_slider->draw_texture_rect(color_picker->theme_cache.color_hue.ptr(),
		Rect2(0, 0, -size.y, size.x), false, Color(1, 1, 1), true);

	draw_focus_rect(hue_slider);

	int y = size.y * color_picker->h;
	const Color color = Color::from_hsv(color_picker->h, 1, 1);
	hue_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
}

void ColorPickerShape::_square_draw() {}

void ColorPickerShape::_square_overlay_draw()
{
	if (!square_overlay) {
		return;
	}
	const Rect2 rect = Rect2(Vector2(), square_overlay->get_size());
	const Vector2 end = rect.get_end();
	const float y_channel =
		(shape_type == ColorPicker::SHAPE_OK_HL_RECTANGLE) ? color_picker->ok_hsl_l
														   : color_picker->ok_hsl_s;

	Vector2 cursor_pos;
	cursor_pos.x =
		CLAMP(rect.position.x + rect.size.x * color_picker->ok_hsl_h, rect.position.x, end.x);
	cursor_pos.y = CLAMP(rect.position.y + rect.size.y * (1.0 - y_channel), rect.position.y, end.y);

	draw_focus_rect(square_overlay);
	draw_cursor(square_overlay, cursor_pos);
}

void ColorPickerShape::_value_slider_draw()
{
	if (!value_slider) {
		return;
	}

	const Vector2 size = value_slider->get_size();

	switch (shape_type) {
	case ColorPicker::SHAPE_VHS_CIRCLE: {
		PackedVector2Array points{
			Vector2(),
			Vector2(size.x, 0),
			size,
			Vector2(0, size.y),
		};

		Color color = Color::from_hsv(color_picker->h, color_picker->s, 1);
		PackedColorArray colors = {
			color,
			color,
			Color(),
			Color(),
		};

		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(color_picker->v, 0, 1));
		color.set_hsv(color_picker->h, 1, color_picker->v);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	case ColorPicker::SHAPE_OK_HL_RECTANGLE: {
		const float ok_hsl_h = color_picker->ok_hsl_h;
		const float ok_hsl_l = color_picker->ok_hsl_l;

		PackedVector2Array points{
			Vector2(size.x, 0),
			Vector2(size.x, size.y * 0.5),
			size,
			Vector2(0, size.y),
			Vector2(0, size.y * 0.5),
			Vector2(),
		};

		Color color1 = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		Color color2 = Color::from_ok_hsl(ok_hsl_h, 0.5, ok_hsl_l);
		Color color3 = Color::from_ok_hsl(ok_hsl_h, 0, ok_hsl_l);
		PackedColorArray colors = {
			color1,
			color2,
			color3,
			color3,
			color2,
			color1,
		};
		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(color_picker->ok_hsl_s, 0, 1));
		const Color color = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	case ColorPicker::SHAPE_OK_HS_RECTANGLE:
	case ColorPicker::SHAPE_OKHSL_CIRCLE: {
		const float ok_hsl_h = color_picker->ok_hsl_h;
		const float ok_hsl_s = color_picker->ok_hsl_s;
		const float ok_hsl_l = color_picker->ok_hsl_l;

		PackedVector2Array points{
			Vector2(size.x, 0),
			Vector2(size.x, size.y * 0.5),
			size,
			Vector2(0, size.y),
			Vector2(0, size.y * 0.5),
			Vector2(),
		};

		Color color1 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 1);
		Color color2 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 0.5);
		Color color3 = Color::from_ok_hsl(ok_hsl_h, ok_hsl_s, 0);
		PackedColorArray colors = {
			color1,
			color2,
			color3,
			color3,
			color2,
			color1,
		};
		value_slider->draw_polygon(points, colors);
		draw_focus_rect(value_slider);

		int y = size.y * (1 - CLAMP(ok_hsl_l, 0, 1));
		const Color color = Color::from_ok_hsl(ok_hsl_h, 1, ok_hsl_l);
		value_slider->draw_line(Vector2(0, y), Vector2(size.x, y), color.inverted());
	} break;

	default:
		break;
	}
}

void ColorPickerShape::_wheel_input(const InputEvent&) {}

void ColorPickerShape::_wheel_draw()
{
	wheel->draw_rect(Rect2(Point2(), wheel->get_size()), Color(1, 1, 1));
}

void ColorPickerShape::_wheel_uv_draw()
{
	const Vector2 uv_size = wheel_uv->get_size();
	const Vector2 ring_radius = uv_size * Math::SQRT12 * WHEEL_RADIUS;
	const Vector2 center = uv_size * 0.5;

	const Rect2 uv_rect(center - ring_radius, ring_radius * 2.0);
	draw_sv_square(wheel_uv, uv_rect, !wheel_focused);
	if (wheel_focused) {
		draw_focus_circle(wheel_uv);
	}

	float radius = WHEEL_RADIUS * 2.0;
	radius += (1.0 - radius) * 0.5;
	const Vector2 cursor_pos =
		center + Vector2(center.x * Math::cos(color_picker->h * Math::TAU) * radius,
					 center.y * Math::sin(color_picker->h * Math::TAU) * radius);
	draw_cursor(wheel_uv, cursor_pos, false);
}

void ColorPickerShape::_circle_input(const InputEvent&) {}

void ColorPickerShape::_value_slider_input(const InputEvent&) {}

void ColorPickerShape::_circle_draw() {}

void ColorPickerShape::_circle_overlay_draw()
{
	if (!circle_overlay) {
		return;
	}
	draw_focus_circle(circle_overlay);
	if (shape_type == ColorPicker::SHAPE_OKHSL_CIRCLE) {
		draw_circle_cursor(circle_overlay, color_picker->ok_hsl_h, color_picker->ok_hsl_s);
	}
	else {
		draw_circle_cursor(circle_overlay, color_picker->h, color_picker->s);
	}
}

// --- color_picker.cpp ---

static inline bool is_color_overbright(const Color& color)
{
	return (color.r > 1.0) || (color.g > 1.0) || (color.b > 1.0);
}

static inline bool is_color_valid_hex(const Color& color)
{
	return !is_color_overbright(color) && color.r >= 0 && color.g >= 0 && color.b >= 0;
}

static inline String color_to_string(
	const Color& color, bool show_alpha = true, bool force_value_format = false)
{
	if (!force_value_format && !is_color_overbright(color)) {
		return "#" + color.to_html(show_alpha);
	}
	String t = "(" + String::num(color.r, 3) + ", " + String::num(color.g, 3) + ", " +
			   String::num(color.b, 3);
	if (show_alpha) {
		t += ", " + String::num(color.a, 3) + ")";
	}
	else {
		t += ")";
	}
	return t;
}

void ColorPicker::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

void ColorPicker::set_focus_on_picker_shape()
{
	int idx = get_current_shape_index();
	if (idx >= 0 && idx < (int)shapes.size()) {
		shapes[idx]->grab_focus();
	}
}

void ColorPicker::set_old_color(const Color& p_color) { old_color = p_color; }

void ColorPicker::set_display_old_color(bool p_enabled) { display_old_color = p_enabled; }

bool ColorPicker::is_displaying_old_color() const { return display_old_color; }

bool ColorPicker::is_editing_alpha() const { return edit_alpha; }

bool ColorPicker::is_editing_intensity() const { return edit_intensity; }

void ColorPicker::_slider_drag_started() { currently_dragging = true; }

void ColorPicker::add_mode(ColorMode* p_mode) { modes.push_back(p_mode); }

void ColorPicker::add_shape(ColorPickerShape* p_shape) { shapes.push_back(p_shape); }

Slider* ColorPicker::get_slider(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, MODE_MAX, nullptr);
	return sliders[p_idx];
}

Vector<float> ColorPicker::get_active_slider_values()
{
	Vector<float> cur_values;
	for (int i = 0; i < current_slider_count; i++) {
		cur_values.push_back(sliders[i]->get_value());
	}
	cur_values.push_back(alpha_slider->get_value());
	return cur_values;
}

void ColorPicker::_copy_normalized_to_hsv_okhsl()
{
	if (!okhsl_cached) {
		ok_hsl_h = color_normalized.get_ok_hsl_h();
		ok_hsl_s = color_normalized.get_ok_hsl_s();
		ok_hsl_l = color_normalized.get_ok_hsl_l();
	}
	if (!hsv_cached) {
		h = color_normalized.get_h();
		s = color_normalized.get_s();
		v = color_normalized.get_v();
	}
	hsv_cached = false;
	okhsl_cached = false;
}

void ColorPicker::_copy_hsv_okhsl_to_normalized()
{
	int idx = get_current_shape_index();
	if (current_shape != SHAPE_NONE && idx >= 0 && idx < (int)shapes.size() &&
		shapes[idx]->is_ok_hsl()) {
		color_normalized.set_ok_hsl(ok_hsl_h, ok_hsl_s, ok_hsl_l, color_normalized.a);
	}
	else {
		color_normalized.set_hsv(h, s, v, color_normalized.a);
	}
}

Color ColorPicker::_color_apply_intensity(const Color& col) const
{
	if (intensity == 0.0f) {
		return col;
	}
	Color linear_color = col.srgb_to_linear();
	Color result;
	float multiplier = Math::pow(2, intensity);
	for (int i = 0; i < 3; i++) {
		result[i] = linear_color[i] * multiplier;
	}
	result.a = col.a;
	return result.linear_to_srgb();
}

void ColorPicker::_normalized_apply_intensity_to_color()
{
	color = _color_apply_intensity(color_normalized);
}

void ColorPicker::_copy_color_to_normalized_and_intensity()
{
	Color linear_color = color.srgb_to_linear();
	float multiplier = MAX(1, MAX(MAX(linear_color.r, linear_color.g), linear_color.b));
	for (int i = 0; i < 3; i++) {
		color_normalized[i] = linear_color[i] / multiplier;
	}
	color_normalized.a = linear_color.a;
	color_normalized = color_normalized.linear_to_srgb();
	intensity = Math::log2(multiplier);
}

void ColorPicker::_reset_sliders_theme()
{
	Ref<StyleBox> style_box_flat(memnew(StyleBox(StyleBox::STYLE_FLAT)));
	style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
	style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

	for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
		sliders[i]->begin_bulk_theme_override();
		sliders[i]->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_icon_override(
			SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
		sliders[i]->add_theme_constant_override(
			SNAME("grabber_offset"), 8 * theme_cache.base_scale);
		if (!colorize_sliders) {
			sliders[i]->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
		}
		sliders[i]->end_bulk_theme_override();
	}

	alpha_slider->begin_bulk_theme_override();
	alpha_slider->add_theme_icon_override(SNAME("grabber"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_icon_override(SNAME("grabber_highlight"), theme_cache.bar_arrow.ptr());
	alpha_slider->add_theme_constant_override(SNAME("grabber_offset"), 8 * theme_cache.base_scale);
	if (!colorize_sliders) {
		alpha_slider->add_theme_style_override(SNAME("slider"), style_box_flat.ptr());
	}
	alpha_slider->end_bulk_theme_override();
}

void ColorPicker::_text_copy_pressed()
{
	DisplayServer::clipboard_set(c_text->get_text());
}

Color ColorPicker::get_pick_color() const { return color; }

Color ColorPicker::get_old_color() const { return old_color; }

ColorPicker::PickerShapeType ColorPicker::get_picker_shape() const { return current_shape; }

inline int ColorPicker::_get_preset_size()
{
	int sep = preset_container ? preset_container->layout.h_separation : 0;
	return (int(get_minimum_size().width) - (sep * (PRESET_COLUMN_COUNT - 1))) /
		   PRESET_COLUMN_COUNT;
}

void ColorPicker::_load_palette()
{
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("ColorPalette", &extensions);

	file_dialog->set_title(ETR("Load Color Palette"));
	file_dialog->clear_filters();
	for (const String& K : extensions) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_current_file("");
	file_dialog->popup_centered_ratio();
}

Control* ColorPicker::get_slider_container() { return slider_gc; }

PackedColorArray ColorPicker::get_presets() const
{
	PackedColorArray arr;
	arr.resize(presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = presets.begin(); itr != presets.end(); ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

PackedColorArray ColorPicker::get_recent_presets() const
{
	PackedColorArray arr;
	arr.resize(recent_presets.size());
	int i = 0;
	for (List<Color>::ConstIterator itr = recent_presets.begin(); itr != recent_presets.end();
		 ++itr, ++i) {
		arr.set(i, *itr);
	}
	return arr;
}

ColorPicker::ColorModeType ColorPicker::get_color_mode() const { return current_mode; }

void ColorPicker::set_colorize_sliders(bool p_colorize_sliders)
{
	if (colorize_sliders == p_colorize_sliders) {
		return;
	}

	colorize_sliders = p_colorize_sliders;

	if (colorize_sliders) {
		Ref<StyleBox> style_box_empty(memnew(StyleBox(StyleBox::STYLE_EMPTY)));

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_empty.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_empty.ptr());
	}
	else {
		Ref<StyleBox> style_box_flat(memnew(StyleBox(StyleBox::STYLE_FLAT)));
		style_box_flat->set_content_margin(SIDE_TOP, 16 * theme_cache.base_scale);
		style_box_flat->set_bg_color(Color(0.2, 0.23, 0.31).lerp(Color(0, 0, 0, 1), 0.3).clamp());

		for (int i = 0; i < MODE_SLIDER_COUNT; i++) {
			sliders[i]->add_theme_style_override("slider", style_box_flat.ptr());
		}

		alpha_slider->add_theme_style_override("slider", style_box_flat.ptr());
	}
}

bool ColorPicker::is_colorizing_sliders() const { return colorize_sliders; }

void ColorPicker::set_deferred_mode(bool p_enabled) { deferred_mode_enabled = p_enabled; }

bool ColorPicker::is_deferred_mode() const { return deferred_mode_enabled; }

void ColorPicker::_sample_draw()
{
	Rect2 rect_new;
	Rect2 rect_old;

	if (display_old_color) {
		rect_new = Rect2(Point2(sample->get_size().width * 0.5, 0),
			Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		rect_old = Rect2(
			Point2(), Size2(sample->get_size().width * 0.5, sample->get_size().height * 0.95));

		if (old_color.a < 1.0) {
			sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_old, true);
		}

		sample->draw_rect(rect_old, old_color);

		if (!old_color.is_equal_approx(color)) {
			sample->draw_texture(theme_cache.sample_revert.ptr(),
				rect_old.size * 0.5 - theme_cache.sample_revert->get_size() * 0.5,
				Math::lerp(0.75f, old_color.get_luminance(), old_color.a) < 0.455
					? Color(1, 1, 1)
					: (Color(0.01, 0.01, 0.01)));

			sample->set_focus_mode(FOCUS_ALL);
		}
		else {
			sample->set_focus_mode(FOCUS_NONE);
		}

		if (is_color_overbright(color)) {
			sample->draw_texture(theme_cache.overbright_indicator.ptr(), Point2());
		}
	}
	else {
		rect_new =
			Rect2(Point2(), Size2(sample->get_size().width, sample->get_size().height * 0.95));
	}

	if (color.a < 1.0) {
		sample->draw_texture_rect(theme_cache.sample_bg.ptr(), rect_new, true);
	}

	sample->draw_rect(rect_new, color);

	if (display_old_color && !old_color.is_equal_approx(color) && sample->has_focus(true)) {
		RID ci = sample->get_canvas_item();
		theme_cache.sample_focus->draw(ci, rect_old);
	}

	if (is_color_overbright(color)) {
		sample->draw_texture(
			theme_cache.overbright_indicator.ptr(), Point2(sample->get_size().width * 0.5, 0));
	}
}

void ColorPicker::_slider_draw(int p_which)
{
	if (colorize_sliders) {
		modes[current_mode]->slider_draw(p_which);
	}
}

void ColorPicker::_alpha_slider_draw()
{
	if (!colorize_sliders) {
		return;
	}
	Vector<Vector2> pos;
	pos.resize(4);
	Vector<Color> col;
	col.resize(4);
	Size2 size = alpha_slider->get_size();
	Color left_color;
	Color right_color;
	const real_t margin = 16 * theme_cache.base_scale;
	alpha_slider->draw_texture_rect(
		theme_cache.sample_bg.ptr(), Rect2(Point2(0, 0), Size2(size.x, margin)), true);

	left_color = color_normalized;
	left_color.a = 0;
	right_color = color_normalized;
	right_color.a = 1;

	col.set(0, left_color);
	col.set(1, right_color);
	col.set(2, right_color);
	col.set(3, left_color);
	pos.set(0, Vector2(0, 0));
	pos.set(1, Vector2(size.x, 0));
	pos.set(2, Vector2(size.x, margin));
	pos.set(3, Vector2(0, margin));

	alpha_slider->draw_polygon(pos, col);
}

void ColorPicker::_line_edit_input(const InputEvent& p_event)
{
	if (p_event.type == InputEventType::MOUSE_BUTTON && !p_event.is_pressed() &&
		p_event.mouse_button.button_index == MouseButton::LEFT) {
		line_edit_mouse_release = true;
	}
}

void ColorPicker::_text_changed(const String&) { text_changed = true; }

void ColorPicker::_block_input_on_popup_show()
{
	if (!SceneTree::get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(true);
	}
}

void ColorPicker::_enable_input_on_popup_hide()
{
	if (!SceneTree::get_root()->is_embedding_subwindows()) {
		get_viewport()->set_disable_input(false);
	}
}

bool ColorPicker::are_swatches_enabled() const { return can_add_swatches; }

bool ColorPicker::are_presets_visible() const { return presets_visible; }

bool ColorPicker::are_modes_visible() const { return color_modes_visible; }

bool ColorPicker::is_sampler_visible() const { return sampler_visible; }

bool ColorPicker::are_sliders_visible() const { return sliders_visible; }

bool ColorPicker::is_hex_visible() const { return hex_visible; }

void ColorPicker::_req_permission()
{
#ifdef MACOS_ENABLED
	OS::request_permission("macos.permission.RECORD_SCREEN");
#endif
}

ColorPicker::ColorPicker()
{
	layout.type = LayoutConfig::LAYOUT_BOX;
	layout.orientation = VERTICAL;
}

ColorPicker::~ColorPicker()
{
	for (ColorMode* mode : modes) {
		memdelete(mode);
	}
	for (ColorPickerShape* shape : shapes) {
		memdelete(shape);
	}
}

void ColorPickerButton::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON);
		AccessibilityServer::update_set_popup_type(
			ae, AccessibilityServerEnums::AccessibilityPopupType::POPUP_DIALOG);
		AccessibilityServer::update_set_color_value(ae, color);
	} break;

	case NOTIFICATION_DRAW: {
		if (theme_cache.normal_style.is_valid()) {
			const Rect2 r = Rect2(theme_cache.normal_style->get_offset(),
				get_size() - theme_cache.normal_style->get_minimum_size());
			if (theme_cache.background_icon.is_valid()) {
				draw_texture_rect(theme_cache.background_icon.ptr(), r, true);
			}
			draw_rect(r, color);

			if ((color.r > 1 || color.g > 1 || color.b > 1) &&
				theme_cache.overbright_indicator.is_valid()) {
				draw_texture(theme_cache.overbright_indicator.ptr(),
					theme_cache.normal_style->get_offset());
			}
		}
	} break;

	case NOTIFICATION_WM_CLOSE_REQUEST: {
		if (popup) {
			popup->hide();
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (popup && !is_visible_in_tree()) {
			popup->hide();
		}
	} break;
	}
}

void ColorPickerButton::_update_picker()
{
	if (!popup) {
		popup = memnew(Popup(true));
		picker = memnew(ColorPicker);
		popup->add_child(picker);
		add_child(popup, false, INTERNAL_MODE_FRONT);
	}
}

Popup* ColorPickerButton::get_popup()
{
	_update_picker();
	return popup;
}

ColorPicker* ColorPickerButton::get_picker()
{
	_update_picker();
	return picker;
}

Color ColorPickerButton::get_pick_color() const { return color; }

bool ColorPickerButton::is_editing_alpha() const { return edit_alpha; }

bool ColorPickerButton::is_editing_intensity() const { return edit_intensity; }

ColorPickerButton::ColorPickerButton(const String& p_text)
{
	button = memnew(Button(p_text));
	button->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	add_child(button, false, INTERNAL_MODE_FRONT);
}

// --- color_rect.cpp ---

Color ColorRect::get_color() const { return color; }

void ColorRect::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::update_set_color_value(ae, color);
	} break;

	case NOTIFICATION_DRAW: {
		drawing = true;
		printf(">>> ColorRect NOTIFICATION_DRAW running! size = (%f, %f)\n", get_size().x,
			get_size().y);
		draw_rect(Rect2(Point2(), get_size()), color);
		drawing = false;
	} break;
	}
}

void ColorRect::notification(int p_what) { _notification(p_what); }

void ColorRect::queue_redraw()
{
	if (!canvas_item.is_valid()) {
		return;
	}
	Renderer::canvas_item_clear(canvas_item);
	drawing = true;
	_notification(NOTIFICATION_DRAW);
	drawing = false;
}

void ColorRect::set_color(const Color& p_color)
{
	color = p_color;
	queue_redraw(); // Statically calls ColorRect::queue_redraw()
}
