/**************************************************************************/
/*  theme_modern.cpp                                                      */
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

#include "core/math/math_defs.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/graph_edit.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "theme_modern.h"

// Helper.
static Color _get_base_color(EditorThemeManager::ThemeConfiguration& p_config,
	float p_dimness_ofs = 0.0, float p_saturation_mult = 1.0)
{
	Color color = p_config.base_color;
	const float final_contrast =
		(p_dimness_ofs < 0) ? CLAMP(p_config.contrast, -0.1, 0.5) : p_config.contrast;
	color.set_v(CLAMP(Math::lerp(color.get_v(), 0, final_contrast * p_dimness_ofs), 0, 1));
	color.set_s(color.get_s() * p_saturation_mult);
	return color;
}

void ThemeModern::populate_shared_styles(
	const Ref<EditorTheme>& p_theme, EditorThemeManager::ThemeConfiguration& p_config)
{
	// Colors.
	{
		// Base colors.

		p_theme->set_color("base_color", EditorStringName(Editor), p_config.base_color);
		p_theme->set_color("accent_color", EditorStringName(Editor), p_config.accent_color);

		// White (dark theme) or black (light theme), will be used to generate the rest of the
		// colors
		p_config.mono_color = p_config.dark_theme ? Color(1, 1, 1) : Color(0, 0, 0);
		p_config.mono_color_font = p_config.dark_icon_and_font ? Color(1, 1, 1) : Color(0, 0, 0);
		p_config.mono_color_inv = p_config.dark_theme ? Color(0, 0, 0) : Color(1, 1, 1);

		// Ensure base colors are in the 0..1 luminance range to avoid 8-bit integer overflow or
		// text rendering issues. Some places in the editor use 8-bit integer colors.
		p_config.dark_color_1 =
			p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast * 1.15).clamp();
		p_config.dark_color_2 = p_config.dark_theme ? Color(0, 0, 0, 0.3) : Color(1, 1, 1, 0.3);
		p_config.dark_color_3 = _get_base_color(p_config, 0.8, 0.9);

		p_config.contrast_color_1 = p_config.base_color.lerp(
			p_config.mono_color, MAX(p_config.contrast * 1.15, p_config.default_contrast * 1.15));
		p_config.contrast_color_2 = p_config.base_color.lerp(
			p_config.mono_color, MAX(p_config.contrast * 1.725, p_config.default_contrast * 1.725));

		p_config.highlight_color =
			Color(p_config.accent_color.r, p_config.accent_color.g, p_config.accent_color.b, 0.275);
		p_config.highlight_disabled_color = p_config.highlight_color.lerp(
			p_config.dark_theme ? Color(0, 0, 0) : Color(1, 1, 1), 0.5);

		p_config.success_color = Color(0.45, 0.95, 0.5);
		p_config.warning_color = Color(0.83, 0.78, 0.62);
		p_config.error_color = Color(1, 0.47, 0.42);

		// Keep dark theme colors accessible for use in the frame time gradient in the 3D editor.
		// This frame time gradient is used to colorize text for a dark background, so it should
		// keep using bright colors even when using a light theme.
		p_theme->set_color(
			"success_color_dark_background", EditorStringName(Editor), p_config.success_color);
		p_theme->set_color(
			"warning_color_dark_background", EditorStringName(Editor), p_config.warning_color);
		p_theme->set_color(
			"error_color_dark_background", EditorStringName(Editor), p_config.error_color);

		if (!p_config.dark_icon_and_font) {
			// Darken some colors to be readable on a light background.
			p_config.success_color = p_config.success_color.lerp(p_config.mono_color_font, 0.35);
			p_config.warning_color = Color(0.83, 0.49, 0.01);
			p_config.error_color = Color(0.8, 0.22, 0.22);
		}

		p_theme->set_color("mono_color", EditorStringName(Editor), p_config.mono_color);
		p_theme->set_color("dark_color_1", EditorStringName(Editor), p_config.dark_color_1);
		p_theme->set_color("dark_color_2", EditorStringName(Editor), p_config.dark_color_2);
		p_theme->set_color("dark_color_3", EditorStringName(Editor), p_config.dark_color_3);
		p_theme->set_color("contrast_color_1", EditorStringName(Editor), p_config.contrast_color_1);
		p_theme->set_color("contrast_color_2", EditorStringName(Editor), p_config.contrast_color_2);
		p_theme->set_color("highlight_color", EditorStringName(Editor), p_config.highlight_color);
		p_theme->set_color("highlight_disabled_color", EditorStringName(Editor),
			p_config.highlight_disabled_color);
		p_theme->set_color("success_color", EditorStringName(Editor), p_config.success_color);
		p_theme->set_color("warning_color", EditorStringName(Editor), p_config.warning_color);
		p_theme->set_color("error_color", EditorStringName(Editor), p_config.error_color);
		p_theme->set_color("ruler_color", EditorStringName(Editor),
			p_config.base_color.lerp(p_config.mono_color_inv, 0.3) * Color(1, 1, 1, 0.8));
#ifndef DISABLE_DEPRECATED // Used before 4.3.
		p_theme->set_color("disabled_highlight_color", EditorStringName(Editor),
			p_config.highlight_disabled_color);
#endif

		// Only used when the Draw Extra Borders editor setting is enabled.
		p_config.extra_border_color_1 =
			p_config.dark_theme ? Color(1, 1, 1, 0.4) : Color(0, 0, 0, 0.4);
		p_config.extra_border_color_2 =
			p_config.dark_theme ? Color(1, 1, 1, 0.2) : Color(0, 0, 0, 0.2);

		p_theme->set_color(
			"extra_border_color_1", EditorStringName(Editor), p_config.extra_border_color_1);
		p_theme->set_color(
			"extra_border_color_2", EditorStringName(Editor), p_config.extra_border_color_2);

		// Font colors.

		p_config.font_color = p_config.mono_color_font * Color(1, 1, 1, 0.75);
		p_config.font_secondary_color = p_config.mono_color_font * Color(1, 1, 1, 0.55);
		p_config.font_focus_color = p_config.mono_color_font;
		p_config.font_hover_color = p_config.mono_color_font * Color(1, 1, 1, 0.85);
		p_config.font_pressed_color = p_config.mono_color_font * Color(1, 1, 1, 0.85);
		p_config.font_hover_pressed_color = p_config.mono_color_font;
		p_config.font_disabled_color =
			p_config.mono_color_font * Color(1, 1, 1, p_config.dark_icon_and_font ? 0.35 : 0.5);
		p_config.font_readonly_color = Color(p_config.mono_color_font.r, p_config.mono_color_font.g,
			p_config.mono_color_font.b, 0.65);
		p_config.font_placeholder_color = p_config.font_disabled_color;
		p_config.font_outline_color = Color(1, 1, 1, 0);

		// Colors designed for dark backgrounds, even when using a light theme.
		// This is used for 3D editor overlay texts.
		if (p_config.dark_theme) {
			p_config.font_dark_background_color = p_config.font_color;
			p_config.font_dark_background_focus_color = p_config.font_focus_color;
			p_config.font_dark_background_hover_color = p_config.font_hover_color;
			p_config.font_dark_background_pressed_color = p_config.font_pressed_color;
			p_config.font_dark_background_hover_pressed_color = p_config.font_hover_pressed_color;
		}
		else {
			p_config.font_dark_background_color =
				p_config.mono_color.inverted().lerp(p_config.base_color, 0.75);
			p_config.font_dark_background_focus_color =
				p_config.mono_color.inverted().lerp(p_config.base_color, 0.25);
			p_config.font_dark_background_hover_color =
				p_config.mono_color.inverted().lerp(p_config.base_color, 0.25);
			p_config.font_dark_background_pressed_color =
				p_config.font_dark_background_color.lerp(p_config.accent_color, 0.74);
			p_config.font_dark_background_hover_pressed_color =
				p_config.font_dark_background_color.lerp(p_config.accent_color, 0.5);
		}

		p_theme->set_color(
			SceneStringName(font_color), EditorStringName(Editor), p_config.font_color);
		p_theme->set_color("font_focus_color", EditorStringName(Editor), p_config.font_focus_color);
		p_theme->set_color("font_hover_color", EditorStringName(Editor), p_config.font_hover_color);
		p_theme->set_color(
			"font_pressed_color", EditorStringName(Editor), p_config.font_pressed_color);
		p_theme->set_color("font_hover_pressed_color", EditorStringName(Editor),
			p_config.font_hover_pressed_color);
		p_theme->set_color(
			"font_disabled_color", EditorStringName(Editor), p_config.font_disabled_color);
		p_theme->set_color(
			"font_readonly_color", EditorStringName(Editor), p_config.font_readonly_color);
		p_theme->set_color(
			"font_placeholder_color", EditorStringName(Editor), p_config.font_placeholder_color);
		p_theme->set_color(
			"font_outline_color", EditorStringName(Editor), p_config.font_outline_color);

		p_theme->set_color("font_dark_background_color", EditorStringName(Editor),
			p_config.font_dark_background_color);
		p_theme->set_color("font_dark_background_focus_color", EditorStringName(Editor),
			p_config.font_dark_background_focus_color);
		p_theme->set_color("font_dark_background_hover_color", EditorStringName(Editor),
			p_config.font_dark_background_hover_color);
		p_theme->set_color("font_dark_background_pressed_color", EditorStringName(Editor),
			p_config.font_dark_background_pressed_color);
		p_theme->set_color("font_dark_background_hover_pressed_color", EditorStringName(Editor),
			p_config.font_dark_background_hover_pressed_color);

#ifndef DISABLE_DEPRECATED // Used before 4.3.
		p_theme->set_color(
			"readonly_font_color", EditorStringName(Editor), p_config.font_readonly_color);
		p_theme->set_color(
			"disabled_font_color", EditorStringName(Editor), p_config.font_disabled_color);
		p_theme->set_color(
			"readonly_color", EditorStringName(Editor), p_config.font_readonly_color);
		p_theme->set_color("highlighted_font_color", EditorStringName(Editor),
			p_config.font_hover_color); // Closest equivalent.
#endif

		// Icon colors.

		p_config.icon_normal_color = Color(1, 1, 1, p_config.dark_icon_and_font ? 0.85 : 0.95);
		p_config.icon_secondary_color = Color(1, 1, 1, p_config.dark_icon_and_font ? 0.6 : 0.75);
		p_config.icon_focus_color = Color(1, 1, 1);
		p_config.icon_hover_color = Color(1, 1, 1);
		p_config.icon_pressed_color =
			p_config.accent_color * (p_config.dark_icon_and_font ? 1.15 : 3.5);
		p_config.icon_pressed_color.a = 1.0;
		p_config.icon_disabled_color = Color(1, 1, 1, p_config.dark_icon_and_font ? 0.35 : 0.5);

		p_theme->set_color(
			"icon_normal_color", EditorStringName(Editor), p_config.icon_normal_color);
		p_theme->set_color("icon_focus_color", EditorStringName(Editor), p_config.icon_focus_color);
		p_theme->set_color("icon_hover_color", EditorStringName(Editor), p_config.icon_hover_color);
		p_theme->set_color(
			"icon_pressed_color", EditorStringName(Editor), p_config.icon_pressed_color);
		p_theme->set_color(
			"icon_disabled_color", EditorStringName(Editor), p_config.icon_disabled_color);

		// Additional GUI colors.

		p_config.surface_popup_color = _get_base_color(p_config, 1.9, 0.9);
		p_config.surface_lowest_color = _get_base_color(p_config, 1.7, 0.9);
		p_config.surface_lower_color = _get_base_color(p_config, 1.1, 0.9);
		p_config.surface_low_color = _get_base_color(p_config, 0.8);
		p_config.surface_base_color = _get_base_color(p_config);
		p_config.surface_high_color = _get_base_color(p_config, -1.3, 0.8);
		p_config.surface_higher_color = _get_base_color(p_config, -1.5, 0.8);
		p_config.surface_highest_color = _get_base_color(p_config, -2.2, 0.6);

		p_config.button_normal_color = _get_base_color(p_config, -2.0, 0.85);
		p_config.button_hover_color = _get_base_color(p_config, -2.9, 0.75);
		p_config.button_pressed_color = _get_base_color(p_config, -3.2, 0.75);
		p_config.button_disabled_color = _get_base_color(p_config, -1.4, 0.75);
		p_config.button_border_normal_color = _get_base_color(p_config, -2.5, 0.75);
		p_config.button_border_hover_color = _get_base_color(p_config, -3.4, 0.75);
		p_config.button_border_pressed_color = _get_base_color(p_config, -3.7, 0.75);

		p_config.flat_button_hover_color = _get_base_color(p_config, -1.2, 0.75);
		p_config.flat_button_pressed_color = _get_base_color(p_config, -2.0, 0.75);
		p_config.flat_button_hover_pressed_color = _get_base_color(p_config, -2.4, 0.75);

		p_config.shadow_color = Color(0, 0, 0, p_config.dark_theme ? 0.3 : 0.1);
		p_config.selection_color = p_config.accent_color * Color(1, 1, 1, 0.4);
		p_config.disabled_border_color =
			p_config.mono_color.inverted().lerp(p_config.base_color, 0.7);
		p_config.disabled_bg_color = p_config.mono_color.inverted().lerp(p_config.base_color, 0.9);
		p_config.separator_color = p_config.dark_theme ? Color(0, 0, 0, 0.4) : Color(0, 0, 0, 0.2);

		p_theme->set_color("selection_color", EditorStringName(Editor), p_config.selection_color);
		p_theme->set_color(
			"disabled_border_color", EditorStringName(Editor), p_config.disabled_border_color);
		p_theme->set_color(
			"disabled_bg_color", EditorStringName(Editor), p_config.disabled_bg_color);
		p_theme->set_color("separator_color", EditorStringName(Editor), p_config.separator_color);

		// Additional editor colors.

		p_theme->set_color(
			"box_selection_fill_color", EditorStringName(Editor), Color(0.65, 0.65, 0.65, 0.15));
		p_theme->set_color(
			"box_selection_stroke_color", EditorStringName(Editor), Color(0.55, 0.55, 0.55, 0.55));

		p_theme->set_color("axis_x_color", EditorStringName(Editor), Color(0.96, 0.20, 0.32));
		p_theme->set_color("axis_y_color", EditorStringName(Editor), Color(0.53, 0.84, 0.01));
		p_theme->set_color("axis_z_color", EditorStringName(Editor), Color(0.16, 0.55, 0.96));
		p_theme->set_color("axis_w_color", EditorStringName(Editor), Color(0.55, 0.55, 0.55));
		p_theme->set_color(
			"axis_view_plane_color", EditorStringName(Editor), Color(0.75, 0.75, 0.75, 0.33));

		p_theme->set_color("property_color_x", EditorStringName(Editor),
			p_config.dark_icon_and_font ? Color(0.88, 0.38, 0.47) : Color(0.40, 0.04, 0.09));
		p_theme->set_color("property_color_y", EditorStringName(Editor),
			p_config.dark_icon_and_font ? Color(0.76, 0.93, 0.40) : Color(0.27, 0.37, 0.06));
		p_theme->set_color("property_color_z", EditorStringName(Editor),
			p_config.dark_icon_and_font ? Color(0.42, 0.67, 0.96) : Color(0.08, 0.22, 0.38));
		p_theme->set_color("property_color_w", EditorStringName(Editor), p_config.font_color);

		// Special colors for rendering methods.

		p_theme->set_color("forward_plus_color", EditorStringName(Editor), Color::hex(0x5d8c3fff));
		p_theme->set_color("mobile_color", EditorStringName(Editor), Color::hex(0xa5557dff));
		p_theme->set_color(
			"gl_compatibility_color", EditorStringName(Editor), Color::hex(0x5586a4ff));
	}

	// Constants.
	{
		// Can't save single float in theme, so using Color.
		p_theme->set_color("icon_saturation", EditorStringName(Editor),
			Color(p_config.icon_saturation, p_config.icon_saturation, p_config.icon_saturation));

		// Controls may rely on the scale for their internal drawing logic.
		p_theme->set_default_base_scale(EDSCALE);
		p_theme->set_constant("scale", EditorStringName(Editor), EDSCALE);

		p_theme->set_constant("thumb_size", EditorStringName(Editor), p_config.thumb_size);
		p_theme->set_constant(
			"class_icon_size", EditorStringName(Editor), p_config.class_icon_size);
		p_theme->set_constant(
			"gizmo_handle_scale", EditorStringName(Editor), p_config.gizmo_handle_scale);

		p_theme->set_constant("base_margin", EditorStringName(Editor), p_config.base_margin);
		p_theme->set_constant(
			"increased_margin", EditorStringName(Editor), p_config.increased_margin);
		p_theme->set_constant(
			"window_border_margin", EditorStringName(Editor), p_config.window_border_margin);
		p_theme->set_constant(
			"top_bar_separation", EditorStringName(Editor), p_config.top_bar_separation);

		p_theme->set_constant("dark_theme", EditorStringName(Editor), p_config.dark_theme);
	}

	// Styleboxes.
	{
		// This is the basic stylebox, used as a base for most other styleboxes (through
		// `duplicate()`).
		p_config.base_style = EditorThemeManager::make_flat_stylebox(p_config.base_color,
			p_config.increased_margin * 1.5, p_config.increased_margin * 1.5,
			p_config.increased_margin * 1.5, p_config.increased_margin * 1.5,
			p_config.corner_radius);

		p_config.focus_style = p_config.base_style->duplicate();
		p_config.focus_style->set_draw_center(false);
		p_config.focus_style->set_border_color(p_config.accent_color * Color(1, 1, 1, 0.8));
		p_config.focus_style->set_border_width_all(2);

		p_config.base_empty_style = EditorThemeManager::make_empty_stylebox();

		p_config.base_empty_wide_style = EditorThemeManager::make_empty_stylebox();
		// Ensure minimum margin for wide flat buttons otherwise the topbar looks broken.
		float base_empty_wide_margin = MAX(p_config.base_margin, 3.0);
		p_config.base_empty_wide_style->set_content_margin_individual(
			base_empty_wide_margin * 1.5 * EDSCALE, base_empty_wide_margin * EDSCALE,
			base_empty_wide_margin * 1.5 * EDSCALE, base_empty_wide_margin * EDSCALE);

		// Button styles.
		{
			p_config.widget_margin =
				Vector2(p_config.increased_margin + 2, p_config.increased_margin + 1) * EDSCALE;

			p_config.button_style = p_config.base_style->duplicate();
			p_config.button_style->set_content_margin_individual(p_config.base_margin * 2 * EDSCALE,
				p_config.base_margin * 1.5 * EDSCALE, p_config.base_margin * 2 * EDSCALE,
				p_config.base_margin * 1.5 * EDSCALE);
			p_config.button_style->set_bg_color(p_config.button_normal_color);
			p_config.button_style->set_border_width_all(Math::round(EDSCALE));
			p_config.button_style->set_shadow_color(
				p_config.dark_theme ? Color(0, 0, 0, 0.005) : Color(1, 1, 1, 0.005));
			p_config.button_style->set_shadow_size(Math::ceil(8 * EDSCALE));
			p_config.button_style->set_shadow_offset(Vector2(0, 4) * EDSCALE);
			if (p_config.draw_extra_borders) {
				p_config.button_style->set_border_color(p_config.extra_border_color_1);
			}
			else {
				p_config.button_style->set_border_color(p_config.button_border_normal_color);
			}

			p_config.button_style_disabled = p_config.button_style->duplicate();
			p_config.button_style_disabled->set_bg_color(p_config.button_disabled_color);
			if (p_config.draw_extra_borders) {
				p_config.button_style_disabled->set_border_color(
					p_config.extra_border_color_2 * Color(1, 1, 1, 0.5));
			}
			else {
				p_config.button_style_disabled->set_border_width_all(0);
			}

			p_config.button_style_pressed = p_config.button_style->duplicate();
			p_config.button_style_pressed->set_bg_color(p_config.button_pressed_color);
			if (p_config.draw_extra_borders) {
				p_config.button_style_pressed->set_border_color(p_config.extra_border_color_1);
			}
			else {
				p_config.button_style_pressed->set_border_color(
					p_config.button_border_pressed_color);
			}

			p_config.button_style_hover = p_config.button_style->duplicate();
			p_config.button_style_hover->set_bg_color(p_config.button_hover_color);
			if (p_config.draw_extra_borders) {
				p_config.button_style_pressed->set_border_color(p_config.extra_border_color_1);
			}
			else {
				p_config.button_style_hover->set_border_color(p_config.button_border_hover_color);
			}

			p_config.flat_button_hover = p_config.base_style->duplicate();
			p_config.flat_button_hover->set_bg_color(p_config.flat_button_hover_color);
			// This affects buttons in Tree so top and bottom margins should be kept low.
			p_config.flat_button_hover->set_content_margin_individual(
				p_config.base_margin * 1.5 * EDSCALE, p_config.base_margin * 0.9 * EDSCALE,
				p_config.base_margin * 1.5 * EDSCALE, p_config.base_margin * 0.9 * EDSCALE);
			if (p_config.draw_extra_borders) {
				p_config.button_style_hover->set_border_color(p_config.extra_border_color_1);
			}

			p_config.flat_button_pressed = p_config.flat_button_hover->duplicate();
			p_config.flat_button_pressed->set_bg_color(p_config.flat_button_pressed_color);
			if (p_config.draw_extra_borders) {
				p_config.flat_button_pressed->set_border_color(p_config.extra_border_color_1);
			}

			p_config.flat_button_hover_pressed = p_config.flat_button_hover->duplicate();
			p_config.flat_button_hover_pressed->set_bg_color(
				p_config.flat_button_hover_pressed_color);
			if (p_config.draw_extra_borders) {
				p_config.flat_button_hover_pressed->set_border_color(p_config.extra_border_color_1);
			}

			p_config.flat_button = p_config.flat_button_hover->duplicate();
			p_config.flat_button->set_draw_center(false);
		}

		// Windows and popups.
		{
			p_config.popup_panel_style = p_config.base_style->duplicate();
			p_config.popup_panel_style->set_bg_color(p_config.surface_popup_color);
			p_config.popup_panel_style->set_shadow_color(Color(0, 0, 0, 0.3));
			p_config.popup_panel_style->set_shadow_size(p_config.base_margin * 0.75 * EDSCALE);
			p_config.popup_panel_style->set_content_margin_all(p_config.popup_margin * EDSCALE);
			p_config.popup_panel_style->set_corner_radius_all(0);
			if (p_config.draw_extra_borders) {
				p_config.popup_panel_style->set_border_width_all(Math::round(EDSCALE));
				p_config.popup_panel_style->set_border_color(p_config.extra_border_color_2);
			}

			p_config.window_style = p_config.base_style->duplicate();
			p_config.window_style->set_content_margin_all(p_config.popup_margin);
			p_config.window_style->set_shadow_color(p_config.shadow_color);
			p_config.window_style->set_shadow_size(4 * EDSCALE);
			p_config.window_style->set_border_color(p_config.base_color);
			p_config.window_style->set_border_width(SIDE_TOP, 24 * EDSCALE);
			p_config.window_style->set_expand_margin(SIDE_TOP, 24 * EDSCALE);
			p_config.window_style->set_corner_radius_all(0);

			p_config.window_complex_style = p_config.window_style->duplicate();
			p_config.window_complex_style->set_bg_color(p_config.surface_lowest_color);

			p_config.dialog_style = p_config.base_style->duplicate();
			p_config.dialog_style->set_content_margin_all(p_config.popup_margin);
			p_config.dialog_style->set_corner_radius_all(0);
		}

		// Panels.
		{
			p_config.panel_container_style = p_config.button_style->duplicate();
			p_config.panel_container_style->set_draw_center(false);
			p_config.panel_container_style->set_border_width_all(0);

			// Content panel for tabs and similar containers.

			// Compensate for the border.
			const int content_panel_margin = p_config.base_margin * EDSCALE + p_config.border_width;

			p_config.content_panel_style = p_config.base_style->duplicate();
			p_config.content_panel_style->set_border_color(p_config.dark_color_3);
			p_config.content_panel_style->set_border_width_all(p_config.border_width);
			p_config.content_panel_style->set_border_width(Side::SIDE_TOP, 0);
			p_config.content_panel_style->set_corner_radius(CORNER_TOP_LEFT, 0);
			p_config.content_panel_style->set_corner_radius(CORNER_TOP_RIGHT, 0);
			p_config.content_panel_style->set_content_margin_individual(content_panel_margin,
				2 * EDSCALE + content_panel_margin, content_panel_margin, content_panel_margin);

			p_config.tab_container_style = p_config.base_style->duplicate();
			p_config.tab_container_style->set_content_margin_all(
				p_config.increased_margin * 1.5 * EDSCALE);
			p_config.tab_container_style->set_corner_radius_individual(
				0, 0, p_config.corner_radius * EDSCALE, p_config.corner_radius * EDSCALE);

			p_config.foreground_panel = p_config.tab_container_style->duplicate();
			p_config.foreground_panel->set_corner_radius(CORNER_TOP_LEFT,
				p_config.tab_container_style->get_corner_radius(CORNER_BOTTOM_LEFT));
			p_config.foreground_panel->set_corner_radius(CORNER_TOP_RIGHT,
				p_config.tab_container_style->get_corner_radius(CORNER_BOTTOM_RIGHT));

			// Trees and similarly inset panels.

			p_config.tree_panel_style = p_config.base_style->duplicate();
			// Make Trees easier to distinguish from other controls by using a darker background
			// color.
			p_config.tree_panel_style->set_bg_color(
				p_config.dark_color_1.lerp(p_config.dark_color_2, 0.5));
			if (p_config.draw_extra_borders) {
				p_config.tree_panel_style->set_border_width_all(Math::round(EDSCALE));
				p_config.tree_panel_style->set_border_color(p_config.extra_border_color_2);
			}
			else {
				p_config.tree_panel_style->set_border_color(p_config.dark_color_3);
			}
		}
	}
}

void ThemeModern::populate_standard_styles(
	const Ref<EditorTheme>& p_theme, EditorThemeManager::ThemeConfiguration& p_config)
{
	// Panels.
	{
		// Panel.
		p_theme->set_stylebox(SceneStringName(panel), "Panel",
			EditorThemeManager::make_flat_stylebox(
				p_config.dark_color_1, 6, 4, 6, 4, p_config.corner_radius));

		// PanelContainer.
		p_theme->set_stylebox(
			SceneStringName(panel), "PanelContainer", p_config.base_empty_wide_style);

		// TooltipPanel & TooltipLabel.
		{
			// TooltipPanel is also used for custom tooltips, while TooltipLabel
			// is only relevant for default tooltips.

			p_theme->set_color(
				SceneStringName(font_color), "TooltipLabel", p_config.font_hover_color);
			p_theme->set_color("font_shadow_color", "TooltipLabel", Color(1, 1, 1, 0));

			Ref<StyleBoxFlat> tooltip_style = p_config.base_style->duplicate();
			tooltip_style->set_bg_color(p_config.surface_popup_color);
			tooltip_style->set_content_margin_all(0);
			tooltip_style->set_corner_radius_all(0);
			if (p_config.draw_extra_borders) {
				tooltip_style->set_border_width_all(Math::round(EDSCALE));
				tooltip_style->set_border_color(p_config.extra_border_color_2);
			}
			p_theme->set_stylebox(SceneStringName(panel), "TooltipPanel", tooltip_style);
		}

		// PopupPanel
		Ref<StyleBoxFlat> popup_panel_style = p_config.base_style->duplicate();
		popup_panel_style->set_bg_color(p_config.surface_popup_color);
		popup_panel_style->set_shadow_color(Color(0, 0, 0, 0.3));
		popup_panel_style->set_shadow_size(p_config.base_margin * 0.75 * EDSCALE);
		popup_panel_style->set_content_margin_all(p_config.popup_margin);
		popup_panel_style->set_corner_radius_all(0);
		if (p_config.draw_extra_borders) {
			popup_panel_style->set_border_width_all(Math::round(EDSCALE));
			popup_panel_style->set_border_color(p_config.extra_border_color_2);
		}
		p_theme->set_stylebox(SceneStringName(panel), "PopupPanel", p_config.popup_panel_style);
	}

	// Buttons.
	{
		// Button.

		p_theme->set_stylebox(SceneStringName(hover), "Button", p_config.button_style_hover);
		p_theme->set_stylebox(SceneStringName(pressed), "Button", p_config.button_style_pressed);
		p_theme->set_stylebox("hover_pressed", "Button", p_config.button_style_pressed);
		p_theme->set_stylebox("focus", "Button", p_config.focus_style);
		p_theme->set_stylebox("disabled", "Button", p_config.button_style_disabled);

		p_theme->set_stylebox("normal_mirrored", "Button", p_config.button_style);
		p_theme->set_stylebox("hover_mirrored", "Button", p_config.button_style_hover);
		p_theme->set_stylebox("pressed_mirrored", "Button", p_config.button_style_pressed);
		p_theme->set_stylebox("hover_pressed_mirrored", "Button", p_config.button_style_pressed);
		p_theme->set_stylebox("disabled_mirrored", "Button", p_config.button_style_disabled);

		p_theme->set_color(SceneStringName(font_color), "Button", p_config.font_color);
		p_theme->set_color("font_hover_color", "Button", p_config.font_hover_color);
		p_theme->set_color("font_hover_pressed_color", "Button", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "Button", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "Button", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "Button", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "Button", p_config.font_outline_color);

		p_theme->set_color("icon_normal_color", "Button", p_config.icon_normal_color);
		p_theme->set_color("icon_hover_color", "Button", p_config.icon_hover_color);
		p_theme->set_color("icon_focus_color", "Button", p_config.icon_focus_color);
		p_theme->set_color("icon_hover_pressed_color", "Button", p_config.icon_pressed_color);
		p_theme->set_color("icon_pressed_color", "Button", p_config.icon_pressed_color);
		p_theme->set_color("icon_disabled_color", "Button", p_config.icon_disabled_color);

		p_theme->set_constant("h_separation", "Button", 4 * EDSCALE);
		p_theme->set_constant("outline_size", "Button", 0);

		p_theme->set_constant("outline_size", "Button", 0);
		p_theme->set_constant("align_to_largest_stylebox", "Button", 1); // Enabled.

#ifdef ANDROID_ENABLED
		// Use a larger click margin on the Android editor to improve touchscreen usability.
		const int click_margin = Math::round(4 * EDSCALE);
#else
		const int click_margin = Math::round(2 * EDSCALE);
#endif
		p_theme->set_constant("click_margin", "BaseButton", click_margin);

		// MenuBar.

		p_theme->set_stylebox(SceneStringName(hover), "MenuBar", p_config.button_style_hover);
		p_theme->set_stylebox(SceneStringName(pressed), "MenuBar", p_config.button_style_pressed);
		p_theme->set_stylebox("disabled", "MenuBar", p_config.button_style_disabled);

		p_theme->set_color(SceneStringName(font_color), "MenuBar", p_config.font_color);
		p_theme->set_color("font_hover_color", "MenuBar", p_config.font_hover_color);
		p_theme->set_color(
			"font_hover_pressed_color", "MenuBar", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "MenuBar", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "MenuBar", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "MenuBar", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "MenuBar", p_config.font_outline_color);

		p_theme->set_constant("h_separation", "MenuBar", 4 * EDSCALE);
		p_theme->set_constant("outline_size", "MenuBar", 0);

		// OptionButton.

		p_theme->set_icon("arrow", "OptionButton",
			p_theme->get_icon(SNAME("GuiOptionArrow"), EditorStringName(EditorIcons)));
		p_theme->set_constant("arrow_margin", "OptionButton", p_config.base_margin * 2 * EDSCALE);
		p_theme->set_constant("modulate_arrow", "OptionButton", true);
		p_theme->set_constant("h_separation", "OptionButton", 4 * EDSCALE);

		// CheckButton.

		p_theme->set_stylebox(
			SceneStringName(pressed), "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("disabled", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox(
			SceneStringName(hover), "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("hover_pressed", "CheckButton", p_config.panel_container_style);

		p_theme->set_stylebox("normal_mirrored", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("pressed_mirrored", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("disabled_mirrored", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox("hover_mirrored", "CheckButton", p_config.panel_container_style);
		p_theme->set_stylebox(
			"hover_pressed_mirrored", "CheckButton", p_config.panel_container_style);

		p_theme->set_icon("checked", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOn"), EditorStringName(EditorIcons)));
		p_theme->set_icon("checked_disabled", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOnDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOff"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked_disabled", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOffDisabled"), EditorStringName(EditorIcons)));

		p_theme->set_icon("checked_mirrored", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOnMirrored"), EditorStringName(EditorIcons)));
		p_theme->set_icon("checked_disabled_mirrored", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOnDisabledMirrored"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked_mirrored", "CheckButton",
			p_theme->get_icon(SNAME("GuiToggleOffMirrored"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked_disabled_mirrored", "CheckButton",
			p_theme->get_icon(
				SNAME("GuiToggleOffDisabledMirrored"), EditorStringName(EditorIcons)));

		p_theme->set_constant("h_separation", "CheckButton", 8 * EDSCALE);
		p_theme->set_constant("check_v_offset", "CheckButton", 0);
		p_theme->set_constant("outline_size", "CheckButton", 0);

		// CheckBox.
		{
			Ref<StyleBoxFlat> checkbox_style = p_config.panel_container_style->duplicate();
			checkbox_style->set_content_margin_individual(p_config.base_margin * 1.5 * EDSCALE,
				p_config.base_margin * 0.75 * EDSCALE, p_config.base_margin * 1.5 * EDSCALE,
				p_config.base_margin * 0.75 * EDSCALE);
			Ref<StyleBoxFlat> checkbox_style_normal = checkbox_style->duplicate();
			checkbox_style_normal->set_draw_center(false);

			p_theme->set_stylebox(SceneStringName(pressed), "CheckBox", checkbox_style);
			p_theme->set_stylebox("disabled", "CheckBox", checkbox_style);
			p_theme->set_stylebox(SceneStringName(hover), "CheckBox", checkbox_style);
			p_theme->set_stylebox("hover_pressed", "CheckBox", checkbox_style);

			p_theme->set_stylebox("normal_mirrored", "CheckBox", checkbox_style_normal);
			p_theme->set_stylebox("pressed_mirrored", "CheckBox", checkbox_style);
			p_theme->set_stylebox("disabled_mirrored", "CheckBox", checkbox_style);
			p_theme->set_stylebox("hover_mirrored", "CheckBox", checkbox_style);
			p_theme->set_stylebox("hover_pressed_mirrored", "CheckBox", checkbox_style);

			p_theme->set_icon("checked", "CheckBox",
				p_theme->get_icon(SNAME("GuiChecked"), EditorStringName(EditorIcons)));

			p_theme->set_icon("unchecked", "CheckBox",
				p_theme->get_icon(SNAME("GuiUnchecked"), EditorStringName(EditorIcons)));
			p_theme->set_icon("radio_checked", "CheckBox",
				p_theme->get_icon(SNAME("GuiRadioChecked"), EditorStringName(EditorIcons)));
			p_theme->set_icon("radio_unchecked", "CheckBox",
				p_theme->get_icon(SNAME("GuiRadioUnchecked"), EditorStringName(EditorIcons)));
			p_theme->set_icon("checked_disabled", "CheckBox",
				p_theme->get_icon(SNAME("GuiCheckedDisabled"), EditorStringName(EditorIcons)));
			p_theme->set_icon("unchecked_disabled", "CheckBox",
				p_theme->get_icon(SNAME("GuiUncheckedDisabled"), EditorStringName(EditorIcons)));
			p_theme->set_icon("radio_checked_disabled", "CheckBox",
				p_theme->get_icon(SNAME("GuiRadioCheckedDisabled"), EditorStringName(EditorIcons)));
			p_theme->set_icon("radio_unchecked_disabled", "CheckBox",
				p_theme->get_icon(
					SNAME("GuiRadioUncheckedDisabled"), EditorStringName(EditorIcons)));

			p_theme->set_constant("h_separation", "CheckBox", 8 * EDSCALE);
			p_theme->set_constant("check_v_offset", "CheckBox", 0);
			p_theme->set_constant("outline_size", "CheckBox", 0);
		}

		// LinkButton.

		p_theme->set_stylebox("focus", "LinkButton", p_config.base_empty_style);

		p_theme->set_color(SceneStringName(font_color), "LinkButton", p_config.font_color);
		p_theme->set_color("font_hover_color", "LinkButton", p_config.font_hover_color);
		p_theme->set_color(
			"font_hover_pressed_color", "LinkButton", p_config.font_hover_pressed_color);
		p_theme->set_color("font_focus_color", "LinkButton", p_config.font_focus_color);
		p_theme->set_color("font_pressed_color", "LinkButton", p_config.font_pressed_color);
		p_theme->set_color("font_disabled_color", "LinkButton", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "LinkButton", p_config.font_outline_color);

		p_theme->set_constant("outline_size", "LinkButton", 0);
	}

	// Tree & ItemList.
	{	 // Tree.
		{// Use empty stylebox for trees to avoid drawing unnecessary borders in docks.
			Ref<StyleBoxEmpty> style_tree_panel = p_config.base_empty_style->duplicate();
	style_tree_panel->set_content_margin_individual(p_config.base_margin * 1.5 * EDSCALE,
		p_config.base_margin * 2.5 * EDSCALE, p_config.base_margin * 1.5 * EDSCALE,
		p_config.base_margin * 2.5 * EDSCALE);

	Ref<StyleBoxFlat> style_button_pressed = p_config.flat_button_pressed->duplicate();
	style_button_pressed->set_content_margin_individual(
		p_config.base_margin, 0, p_config.base_margin, 0);

	p_theme->set_icon(
		"checked", "Tree", p_theme->get_icon(SNAME("GuiChecked"), EditorStringName(EditorIcons)));
	p_theme->set_icon("checked_disabled", "Tree",
		p_theme->get_icon(SNAME("GuiCheckedDisabled"), EditorStringName(EditorIcons)));
	p_theme->set_icon("indeterminate", "Tree",
		p_theme->get_icon(SNAME("GuiIndeterminate"), EditorStringName(EditorIcons)));
	p_theme->set_icon("indeterminate_disabled", "Tree",
		p_theme->get_icon(SNAME("GuiIndeterminateDisabled"), EditorStringName(EditorIcons)));
	p_theme->set_icon("unchecked", "Tree",
		p_theme->get_icon(SNAME("GuiUnchecked"), EditorStringName(EditorIcons)));
	p_theme->set_icon("unchecked_disabled", "Tree",
		p_theme->get_icon(SNAME("GuiUncheckedDisabled"), EditorStringName(EditorIcons)));
	p_theme->set_icon("arrow", "Tree",
		p_theme->get_icon(SNAME("GuiTreeArrowDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("arrow_collapsed", "Tree",
		p_theme->get_icon(SNAME("GuiTreeArrowRight"), EditorStringName(EditorIcons)));
	p_theme->set_icon("arrow_collapsed_mirrored", "Tree",
		p_theme->get_icon(SNAME("GuiTreeArrowLeft"), EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"updown", "Tree", p_theme->get_icon(SNAME("GuiTreeUpdown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("select_arrow", "Tree",
		p_theme->get_icon(SNAME("GuiDropdown"), EditorStringName(EditorIcons)));

	p_theme->set_stylebox(SceneStringName(panel), "Tree", style_tree_panel);
	p_theme->set_stylebox("focus", "Tree", p_config.focus_style);
	p_theme->set_stylebox("button_pressed", "Tree", style_button_pressed);
	p_theme->set_stylebox("custom_button", "Tree", p_config.flat_button);
	p_theme->set_stylebox("custom_button_pressed", "Tree", style_button_pressed);

	p_theme->set_color("custom_button_font_highlight", "Tree", p_config.font_hover_color);
	p_theme->set_color(SceneStringName(font_color), "Tree", p_config.font_color);
	p_theme->set_color("font_hovered_color", "Tree", p_config.font_hover_color);
	p_theme->set_color("font_hovered_dimmed_color", "Tree", p_config.font_hover_color);
	p_theme->set_color("font_hovered_selected_color", "Tree", p_config.mono_color_font);
	p_theme->set_color("font_selected_color", "Tree", p_config.mono_color_font);
	p_theme->set_color("font_disabled_color", "Tree", p_config.font_disabled_color);
	p_theme->set_color("font_outline_color", "Tree", p_config.font_outline_color);
	p_theme->set_color("title_button_color", "Tree", p_config.font_color);

	int tree_v_sep = p_config.enable_touch_optimizations
						 ? (p_config.separation_margin * 0.9)
						 : Math::pow(p_config.base_margin * 0.175 * EDSCALE, 3);
	p_theme->set_constant("v_separation", "Tree", tree_v_sep);
	p_theme->set_constant("h_separation", "Tree", (p_config.increased_margin + 2) * EDSCALE);
	p_theme->set_constant("guide_width", "Tree", p_config.border_width);
	p_theme->set_constant(
		"item_margin", "Tree", MAX(3 * p_config.increased_margin * EDSCALE, 12 * EDSCALE));
	p_theme->set_constant("inner_item_margin_top", "Tree", p_config.base_margin * 0.75 * EDSCALE);
	p_theme->set_constant(
		"inner_item_margin_bottom", "Tree", p_config.base_margin * 0.75 * EDSCALE);
	p_theme->set_constant("inner_item_margin_left", "Tree", p_config.base_margin * EDSCALE);
	p_theme->set_constant("inner_item_margin_right", "Tree", p_config.base_margin * EDSCALE);
	p_theme->set_constant("check_h_separation", "Tree", p_config.base_margin * 1.5 * EDSCALE);
	p_theme->set_constant("icon_h_separation", "Tree", p_config.base_margin * 1.5 * EDSCALE);
	p_theme->set_constant("button_margin", "Tree", p_config.base_margin * EDSCALE);
	p_theme->set_constant("dragging_unfold_wait_msec", "Tree", p_config.dragging_hover_wait_msec);
	p_theme->set_constant("scroll_max_sticky_items", "Tree", p_config.max_sticky_tree_items);
	p_theme->set_constant("scroll_border", "Tree", 40 * EDSCALE);
	p_theme->set_constant("scroll_speed", "Tree", 12);
	p_theme->set_constant("outline_size", "Tree", 0);
	p_theme->set_constant("scrollbar_margin_left", "Tree", 0);
	p_theme->set_constant("scrollbar_margin_top", "Tree", 0);
	p_theme->set_constant("scrollbar_margin_right", "Tree", 0);
	p_theme->set_constant("scrollbar_margin_bottom", "Tree", 0);
	p_theme->set_constant("scrollbar_h_separation", "Tree", 1 * EDSCALE);
	p_theme->set_constant("scrollbar_v_separation", "Tree", 1 * EDSCALE);

	Color relationship_line_color =
		p_config.mono_color * Color(1, 1, 1, p_config.relationship_line_opacity);
	Color highlight_line_color =
		p_config.mono_color * Color(1, 1, 1, p_config.relationship_line_opacity * 2);

	int draw_relationship_lines = 0;
	int relationship_line_width = 0;
	int highlighted_line_width = Math::ceil(EDSCALE);
	if (p_config.draw_relationship_lines == EditorThemeManager::RELATIONSHIP_ALL) {
		draw_relationship_lines = 1;
		relationship_line_width = 1;
	}
	else if (p_config.draw_relationship_lines == EditorThemeManager::RELATIONSHIP_SELECTED_ONLY) {
		draw_relationship_lines = 1;
	}

	p_theme->set_constant("draw_guides", "Tree", 0);
	p_theme->set_constant("draw_relationship_lines", "Tree",
		draw_relationship_lines && p_config.relationship_line_opacity >= 0.01);
	p_theme->set_constant("relationship_line_width", "Tree", relationship_line_width);
	p_theme->set_constant("parent_hl_line_width", "Tree", highlighted_line_width);
	p_theme->set_constant("children_hl_line_width", "Tree", 1);
	p_theme->set_constant("parent_hl_line_margin", "Tree", 3);
	p_theme->set_color("relationship_line_color", "Tree", relationship_line_color);
	p_theme->set_color("parent_hl_line_color", "Tree", highlight_line_color);
	p_theme->set_color("children_hl_line_color", "Tree", relationship_line_color);
	p_theme->set_color("drop_on_item_color", "Tree", p_config.accent_color);
	p_theme->set_color("drop_position_color", "Tree", p_config.icon_normal_color);
	p_theme->set_color("guide_color", "Tree", Color(1, 1, 1, 0));
	p_theme->set_color(
		"scroll_hint_color", "Tree", Color(0, 0, 0, p_config.dark_theme ? 1.0 : 0.5));

	Ref<StyleBoxFlat> style_tree_hover = p_config.flat_button_hover->duplicate();
	style_tree_hover->set_content_margin_all(0);

	p_theme->set_stylebox("button_hover", "Tree", style_tree_hover);
	p_theme->set_stylebox("hovered", "Tree", style_tree_hover);
	p_theme->set_stylebox("hovered_dimmed", "Tree", style_tree_hover);
	p_theme->set_stylebox("custom_button_hover", "Tree", style_tree_hover);

	Ref<StyleBoxFlat> style_tree_selected = p_config.flat_button_pressed->duplicate();
	style_tree_selected->set_content_margin_all(0);

	p_theme->set_stylebox("selected", "Tree", style_tree_selected);
	p_theme->set_stylebox("selected_focus", "Tree", p_config.focus_style);

	Ref<StyleBoxFlat> style_tree_hovered_selected = p_config.flat_button_hover_pressed->duplicate();
	style_tree_hovered_selected->set_content_margin_all(0);

	p_theme->set_stylebox("hovered_selected", "Tree", style_tree_hovered_selected);
	p_theme->set_stylebox("hovered_selected_focus", "Tree", p_config.focus_style);

	// Cursor is drawn on top of the item so it needs to be transparent.
	Ref<StyleBoxFlat> style_tree_cursor = p_config.base_style->duplicate();
	style_tree_cursor->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.04));

	Ref<StyleBoxFlat> style_tree_title = p_config.base_style->duplicate();
	style_tree_title->set_bg_color(p_config.surface_lower_color);
	// Use a transparent border to separate rounded column titles.
	style_tree_title->set_border_color(Color(p_config.surface_lower_color, 0));
	style_tree_title->set_border_width(SIDE_LEFT, Math::ceil(EDSCALE));
	style_tree_title->set_border_width(SIDE_RIGHT, Math::ceil(EDSCALE));

	p_theme->set_stylebox("cursor", "Tree", style_tree_cursor);
	p_theme->set_stylebox("cursor_unfocused", "Tree", style_tree_cursor);
	p_theme->set_stylebox("title_button_normal", "Tree", style_tree_title);
	p_theme->set_stylebox("title_button_hover", "Tree", style_tree_title);
	p_theme->set_stylebox("title_button_pressed", "Tree", style_tree_title);
}

// ProjectList.
{
	Ref<StyleBoxFlat> style_project_list_hover = p_config.flat_button_hover->duplicate();
	style_project_list_hover->set_bg_color(_get_base_color(p_config, -0.2, 0.75));
	style_project_list_hover->set_content_margin_all(0);

	Ref<StyleBoxFlat> style_project_list_selected = style_project_list_hover->duplicate();
	style_project_list_selected->set_bg_color(_get_base_color(p_config, -1.0, 0.75));

	Ref<StyleBoxFlat> style_project_list_hover_pressed = style_project_list_selected->duplicate();
	style_project_list_hover_pressed->set_bg_color(_get_base_color(p_config, -1.2, 0.75));

	p_theme->set_stylebox("hovered", "ProjectList", style_project_list_hover);
	p_theme->set_stylebox("selected", "ProjectList", style_project_list_selected);
	p_theme->set_stylebox("hover_pressed", "ProjectList", style_project_list_hover_pressed);
	p_theme->set_stylebox("focus", "ProjectList", p_config.focus_style);

	p_theme->set_color(SceneStringName(font_color), "ProjectList", p_config.font_color);
	p_theme->set_color("guide_color", "ProjectList", Color(1, 1, 1, 0));
}

// ItemList.
{
	Ref<StyleBoxFlat> style_itemlist_bg = p_config.base_style->duplicate();
	style_itemlist_bg->set_content_margin_all(p_config.base_margin * 2 * EDSCALE);
	Ref<StyleBoxFlat> style_itemlist_cursor = p_config.base_style->duplicate();
	style_itemlist_cursor->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.04));

	p_theme->set_stylebox(SceneStringName(panel), "ItemList", style_itemlist_bg);
	p_theme->set_stylebox("focus", "ItemList", p_config.focus_style);
	p_theme->set_stylebox("cursor", "ItemList", style_itemlist_cursor);
	p_theme->set_stylebox("cursor_unfocused", "ItemList", style_itemlist_cursor);
	p_theme->set_stylebox("selected_focus", "ItemList", p_config.focus_style);
	p_theme->set_stylebox("selected", "ItemList", p_config.flat_button_pressed);
	p_theme->set_stylebox("hovered", "ItemList", p_config.flat_button_hover);
	p_theme->set_stylebox("hovered_selected", "ItemList", p_config.flat_button_hover_pressed);
	p_theme->set_stylebox("hovered_selected_focus", "ItemList", p_config.focus_style);
	p_theme->set_color(SceneStringName(font_color), "ItemList", p_config.font_color);
	p_theme->set_color("font_hovered_color", "ItemList", p_config.font_hover_color);
	p_theme->set_color(
		"font_hovered_selected_color", "ItemList", p_config.font_hover_pressed_color);
	p_theme->set_color("font_selected_color", "ItemList", p_config.font_pressed_color);
	p_theme->set_color("font_outline_color", "ItemList", p_config.font_outline_color);
	p_theme->set_color("guide_color", "ItemList", Color(1, 1, 1, 0));
	p_theme->set_color(
		"scroll_hint_color", "ItemList", Color(0, 0, 0, p_config.dark_theme ? 1.0 : 0.5));
	p_theme->set_constant("v_separation", "ItemList", p_config.base_margin * 1.5 * EDSCALE);
	p_theme->set_constant("h_separation", "ItemList", (p_config.increased_margin + 2) * EDSCALE);
	p_theme->set_constant(
		"scroll_bar_h_separation", "ItemList", style_itemlist_bg->get_margin(SIDE_RIGHT));
	p_theme->set_constant("icon_margin", "ItemList", (p_config.increased_margin + 2) * EDSCALE);
	p_theme->set_constant(SceneStringName(line_separation), "ItemList", p_config.separation_margin);
	p_theme->set_constant("outline_size", "ItemList", 0);
}
}

// TabBar & TabContainer.
{
	Ref<StyleBoxFlat> style_tab_selected = p_config.base_style->duplicate();
	style_tab_selected->set_content_margin_individual(p_config.base_margin * 4 * EDSCALE,
		p_config.base_margin * 2.1 * EDSCALE, p_config.base_margin * 4 * EDSCALE,
		p_config.base_margin * 2.1 * EDSCALE);
	style_tab_selected->set_corner_radius_individual(
		p_config.corner_radius * EDSCALE, p_config.corner_radius * EDSCALE, 0, 0);

	Ref<StyleBoxFlat> style_tab_focus = style_tab_selected->duplicate();
	style_tab_focus->set_bg_color(p_config.base_color);
	style_tab_focus->set_border_color(p_config.accent_color);

	Ref<StyleBoxFlat> style_tab_unselected = style_tab_selected->duplicate();
	style_tab_unselected->set_bg_color(p_config.surface_lowest_color);
	style_tab_unselected->set_border_width_all(0);

	Ref<StyleBoxFlat> style_tab_hovered = style_tab_unselected->duplicate();
	style_tab_hovered->set_bg_color(p_config.surface_base_color * Color(1, 1, 1, 0.6));

	Color drop_mark_color = p_config.dark_color_2.lerp(p_config.accent_color, 0.75);

	Ref<StyleBoxFlat> style_tabbar_background = p_config.base_style->duplicate();
	style_tabbar_background->set_bg_color(p_config.surface_lowest_color);
	style_tabbar_background->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	style_tabbar_background->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	style_tabbar_background->set_content_margin_individual(
		0, 0, p_config.base_margin * 0.25 * EDSCALE, 0);

	p_theme->set_stylebox("tabbar_background", "TabContainer", style_tabbar_background);
	p_theme->set_stylebox(SceneStringName(panel), "TabContainer", p_config.tab_container_style);

	p_theme->set_stylebox("tab_selected", "TabContainer", style_tab_selected);
	p_theme->set_stylebox("tab_hovered", "TabContainer", style_tab_hovered);
	p_theme->set_stylebox("tab_unselected", "TabContainer", style_tab_unselected);
	p_theme->set_stylebox("tab_disabled", "TabContainer", style_tab_unselected);
	p_theme->set_stylebox("tab_focus", "TabContainer", p_config.focus_style);
	p_theme->set_stylebox("tab_selected", "TabBar", style_tab_selected);
	p_theme->set_stylebox("tab_hovered", "TabBar", style_tab_hovered);
	p_theme->set_stylebox("tab_unselected", "TabBar", style_tab_unselected);
	p_theme->set_stylebox("tab_disabled", "TabBar", style_tab_unselected);
	p_theme->set_stylebox("tab_focus", "TabBar", p_config.focus_style);
	p_theme->set_stylebox("button_pressed", "TabBar", p_config.panel_container_style);
	p_theme->set_stylebox("button_highlight", "TabBar", p_config.panel_container_style);

	p_theme->set_color("font_selected_color", "TabContainer", p_config.font_color);
	p_theme->set_color("font_hovered_color", "TabContainer", p_config.font_hover_color);
	p_theme->set_color("font_unselected_color", "TabContainer", p_config.font_secondary_color);
	p_theme->set_color(
		"font_disabled_color", "TabContainer", p_config.font_disabled_color * Color(1, 1, 1, 0.55));
	p_theme->set_color("font_outline_color", "TabContainer", p_config.font_outline_color);
	p_theme->set_color("font_selected_color", "TabBar", p_config.font_color);
	p_theme->set_color("font_hovered_color", "TabBar", p_config.font_hover_color);
	p_theme->set_color("font_unselected_color", "TabBar", p_config.font_secondary_color);
	p_theme->set_color(
		"font_disabled_color", "TabBar", p_config.font_disabled_color * Color(1, 1, 1, 0.55));
	p_theme->set_color("font_outline_color", "TabBar", p_config.font_outline_color);
	p_theme->set_color("drop_mark_color", "TabContainer", drop_mark_color);
	p_theme->set_color("drop_mark_color", "TabBar", drop_mark_color);

	p_theme->set_color("icon_selected_color", "TabContainer", p_config.icon_normal_color);
	p_theme->set_color("icon_hovered_color", "TabContainer", p_config.icon_hover_color);
	p_theme->set_color("icon_unselected_color", "TabContainer", p_config.icon_secondary_color);
	p_theme->set_color(
		"icon_disabled_color", "TabContainer", p_config.icon_disabled_color * Color(1, 1, 1, 0.55));
	p_theme->set_color("icon_selected_color", "TabBar", p_config.icon_normal_color);
	p_theme->set_color("icon_hovered_color", "TabBar", p_config.icon_hover_color);
	p_theme->set_color("icon_unselected_color", "TabBar", p_config.icon_secondary_color);
	p_theme->set_color(
		"icon_disabled_color", "TabBar", p_config.icon_disabled_color * Color(1, 1, 1, 0.55));

	p_theme->set_icon("menu", "TabContainer",
		p_theme->get_icon(SNAME("GuiTabMenu"), EditorStringName(EditorIcons)));
	p_theme->set_icon("menu_highlight", "TabContainer",
		p_theme->get_icon(SNAME("GuiTabMenuHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"close", "TabBar", p_theme->get_icon(SNAME("GuiClose"), EditorStringName(EditorIcons)));
	p_theme->set_icon("increment", "TabContainer",
		p_theme->get_icon(SNAME("GuiScrollArrowRight"), EditorStringName(EditorIcons)));
	p_theme->set_icon("decrement", "TabContainer",
		p_theme->get_icon(SNAME("GuiScrollArrowLeft"), EditorStringName(EditorIcons)));
	p_theme->set_icon("increment", "TabBar",
		p_theme->get_icon(SNAME("GuiScrollArrowRight"), EditorStringName(EditorIcons)));
	p_theme->set_icon("decrement", "TabBar",
		p_theme->get_icon(SNAME("GuiScrollArrowLeft"), EditorStringName(EditorIcons)));
	p_theme->set_icon("increment_highlight", "TabBar",
		p_theme->get_icon(SNAME("GuiScrollArrowRightHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("decrement_highlight", "TabBar",
		p_theme->get_icon(SNAME("GuiScrollArrowLeftHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("increment_highlight", "TabContainer",
		p_theme->get_icon(SNAME("GuiScrollArrowRightHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("decrement_highlight", "TabContainer",
		p_theme->get_icon(SNAME("GuiScrollArrowLeftHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("drop_mark", "TabContainer",
		p_theme->get_icon(SNAME("GuiTabDropMark"), EditorStringName(EditorIcons)));
	p_theme->set_icon("drop_mark", "TabBar",
		p_theme->get_icon(SNAME("GuiTabDropMark"), EditorStringName(EditorIcons)));

	p_theme->set_constant("side_margin", "TabContainer", 0);
	p_theme->set_constant("outline_size", "TabContainer", 0);
	p_theme->set_constant("h_separation", "TabBar", 4 * EDSCALE);
	p_theme->set_constant("outline_size", "TabBar", 0);
	p_theme->set_constant("hover_switch_wait_msec", "TabBar", p_config.dragging_hover_wait_msec);
}

// Separators.
{
	Ref<StyleBoxLine> style_h_separator =
		EditorThemeManager::make_line_stylebox(p_config.separator_color, Math::round(2 * EDSCALE),
			p_config.base_margin * -1 * EDSCALE, p_config.base_margin * -1 * EDSCALE);
	p_theme->set_stylebox("separator", "HSeparator", style_h_separator);

	Ref<StyleBoxLine> style_v_separator = style_h_separator->duplicate();
	style_v_separator->set_vertical(true);
	p_theme->set_stylebox("separator", "VSeparator", style_v_separator);

	p_theme->set_constant("separation", "Separator", p_config.base_margin * 2 * EDSCALE);
}

// LineEdit & TextEdit.
{
	Ref<StyleBoxFlat> text_editor_style = p_config.base_style->duplicate();
	text_editor_style->set_bg_color(p_config.surface_lower_color);
	text_editor_style->set_content_margin_individual(p_config.base_margin * 2 * EDSCALE,
		((p_config.base_margin * 0.75) + 1) * EDSCALE, p_config.base_margin * 2 * EDSCALE,
		((p_config.base_margin * 0.75) + 1) * EDSCALE);
	if (p_config.draw_extra_borders) {
		text_editor_style->set_border_width_all(Math::round(EDSCALE));
		text_editor_style->set_border_color(p_config.extra_border_color_1);
	}

	Ref<StyleBoxFlat> text_editor_disabled_style = text_editor_style->duplicate();
	// Using transparent background for readonly otherwise it looks bad in the master audio bus.
	text_editor_disabled_style->set_bg_color(
		p_config.dark_theme ? Color(0, 0, 0, 0.2) : Color(1, 1, 1, 0.5));

	// LineEdit.

	p_theme->set_stylebox("focus", "LineEdit", p_config.focus_style);
	p_theme->set_stylebox("read_only", "LineEdit", text_editor_disabled_style);

	p_theme->set_icon(
		"clear", "LineEdit", p_theme->get_icon(SNAME("GuiClose"), EditorStringName(EditorIcons)));

	p_theme->set_color(SceneStringName(font_color), "LineEdit", p_config.font_color);
	p_theme->set_color("font_selected_color", "LineEdit", p_config.font_pressed_color);
	p_theme->set_color("font_uneditable_color", "LineEdit", p_config.font_readonly_color);
	p_theme->set_color("font_placeholder_color", "LineEdit", p_config.font_placeholder_color);
	p_theme->set_color("font_outline_color", "LineEdit", p_config.font_outline_color);
	p_theme->set_color("caret_color", "LineEdit", p_config.font_color);
	p_theme->set_color("selection_color", "LineEdit", p_config.selection_color);
	p_theme->set_color("clear_button_color", "LineEdit", p_config.font_color);
	p_theme->set_color("clear_button_color_pressed", "LineEdit", p_config.accent_color);

	p_theme->set_constant("minimum_character_width", "LineEdit", 4);
	p_theme->set_constant("outline_size", "LineEdit", 0);
	p_theme->set_constant("caret_width", "LineEdit", 1);

	// TextEdit.

	p_theme->set_stylebox("focus", "TextEdit", p_config.focus_style);
	p_theme->set_stylebox("read_only", "TextEdit", text_editor_disabled_style);

	p_theme->set_icon(
		"tab", "TextEdit", p_theme->get_icon(SNAME("GuiTab"), EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"space", "TextEdit", p_theme->get_icon(SNAME("GuiSpace"), EditorStringName(EditorIcons)));

	p_theme->set_color(SceneStringName(font_color), "TextEdit", p_config.font_color);
	p_theme->set_color("font_readonly_color", "TextEdit", p_config.font_readonly_color);
	p_theme->set_color("font_placeholder_color", "TextEdit", p_config.font_placeholder_color);
	p_theme->set_color("font_outline_color", "TextEdit", p_config.font_outline_color);
	p_theme->set_color("caret_color", "TextEdit", p_config.font_color);
	p_theme->set_color("selection_color", "TextEdit", p_config.selection_color);

	p_theme->set_constant("line_spacing", "TextEdit", 4 * EDSCALE);
	p_theme->set_constant("outline_size", "TextEdit", 0);
	p_theme->set_constant("caret_width", "TextEdit", 1);
}

// Containers.
{
	p_theme->set_constant("separation", "BoxContainer", p_config.separation_margin);
	p_theme->set_constant("separation", "HBoxContainer", p_config.separation_margin);
	p_theme->set_constant("separation", "VBoxContainer", p_config.separation_margin);
	p_theme->set_constant("margin_left", "MarginContainer", 0);
	p_theme->set_constant("margin_top", "MarginContainer", 0);
	p_theme->set_constant("margin_right", "MarginContainer", 0);
	p_theme->set_constant("margin_bottom", "MarginContainer", 0);
	p_theme->set_constant("h_separation", "GridContainer", p_config.separation_margin);
	p_theme->set_constant("v_separation", "GridContainer", p_config.separation_margin);
	p_theme->set_constant("h_separation", "FlowContainer", p_config.separation_margin);
	p_theme->set_constant("v_separation", "FlowContainer", p_config.separation_margin);
	p_theme->set_constant("h_separation", "HFlowContainer", p_config.separation_margin);
	p_theme->set_constant("v_separation", "HFlowContainer", p_config.separation_margin);
	p_theme->set_constant("h_separation", "VFlowContainer", p_config.separation_margin);
	p_theme->set_constant("v_separation", "VFlowContainer", p_config.separation_margin);

	// SplitContainer.

	p_theme->set_icon("h_grabber", "SplitContainer",
		p_theme->get_icon(SNAME("GuiHsplitter"), EditorStringName(EditorIcons)));
	p_theme->set_icon("v_grabber", "SplitContainer",
		p_theme->get_icon(SNAME("GuiVsplitter"), EditorStringName(EditorIcons)));
	p_theme->set_icon("grabber", "VSplitContainer",
		p_theme->get_icon(SNAME("GuiVsplitter"), EditorStringName(EditorIcons)));
	p_theme->set_icon("grabber", "HSplitContainer",
		p_theme->get_icon(SNAME("GuiHsplitter"), EditorStringName(EditorIcons)));

	p_theme->set_constant("separation", "SplitContainer", p_config.base_margin * 0.75 * EDSCALE);
	p_theme->set_constant("separation", "HSplitContainer", p_config.base_margin * 0.75 * EDSCALE);
	p_theme->set_constant("separation", "VSplitContainer", p_config.base_margin * 0.75 * EDSCALE);

	p_theme->set_constant("autohide", "SplitContainer", 1);
	p_theme->set_constant("autohide", "HSplitContainer", 1);
	p_theme->set_constant("autohide", "VSplitContainer", 1);

	p_theme->set_constant(
		"minimum_grab_thickness", "SplitContainer", p_config.base_margin * 2 * EDSCALE);
	p_theme->set_constant(
		"minimum_grab_thickness", "HSplitContainer", p_config.base_margin * 2 * EDSCALE);
	p_theme->set_constant(
		"minimum_grab_thickness", "VSplitContainer", p_config.base_margin * 2 * EDSCALE);

	// GridContainer.
	p_theme->set_constant(
		"v_separation", "GridContainer", Math::round(p_config.widget_margin.y - 2 * EDSCALE));

	// FoldableContainer

	Ref<StyleBoxFlat> foldable_container_title =
		EditorThemeManager::make_flat_stylebox(p_config.dark_color_1.darkened(0.125),
			p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin);
	foldable_container_title->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	foldable_container_title->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	p_theme->set_stylebox("title_panel", "FoldableContainer", foldable_container_title);
	Ref<StyleBoxFlat> foldable_container_hover =
		EditorThemeManager::make_flat_stylebox(p_config.dark_color_1.lerp(p_config.base_color, 0.4),
			p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin);
	foldable_container_hover->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	foldable_container_hover->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	p_theme->set_stylebox("title_hover_panel", "FoldableContainer", foldable_container_hover);
	p_theme->set_stylebox("title_collapsed_panel", "FoldableContainer",
		EditorThemeManager::make_flat_stylebox(p_config.dark_color_1.darkened(0.125),
			p_config.base_margin, p_config.base_margin, p_config.base_margin,
			p_config.base_margin));
	p_theme->set_stylebox("title_collapsed_hover_panel", "FoldableContainer",
		EditorThemeManager::make_flat_stylebox(p_config.dark_color_1.lerp(p_config.base_color, 0.4),
			p_config.base_margin, p_config.base_margin, p_config.base_margin,
			p_config.base_margin));
	Ref<StyleBoxFlat> foldable_container_panel =
		EditorThemeManager::make_flat_stylebox(p_config.dark_color_1, p_config.base_margin,
			p_config.base_margin, p_config.base_margin, p_config.base_margin);
	foldable_container_panel->set_corner_radius(CORNER_TOP_LEFT, 0);
	foldable_container_panel->set_corner_radius(CORNER_TOP_RIGHT, 0);
	p_theme->set_stylebox(SceneStringName(panel), "FoldableContainer", foldable_container_panel);
	p_theme->set_stylebox("focus", "FoldableContainer", p_config.focus_style);

	p_theme->set_font(SceneStringName(font), "FoldableContainer",
		p_theme->get_font(SceneStringName(font), SNAME("HeaderSmall")));
	p_theme->set_font_size(SceneStringName(font_size), "FoldableContainer",
		p_theme->get_font_size(SceneStringName(font_size), SNAME("HeaderSmall")));

	p_theme->set_color(SceneStringName(font_color), "FoldableContainer", p_config.font_color);
	p_theme->set_color("hover_font_color", "FoldableContainer", p_config.font_hover_color);
	p_theme->set_color("collapsed_font_color", "FoldableContainer", p_config.font_pressed_color);
	p_theme->set_color("font_outline_color", "FoldableContainer", p_config.font_outline_color);

	p_theme->set_icon("expanded_arrow", "FoldableContainer",
		p_theme->get_icon(SNAME("GuiTreeArrowDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("expanded_arrow_mirrored", "FoldableContainer",
		p_theme->get_icon(SNAME("GuiArrowUp"), EditorStringName(EditorIcons)));
	p_theme->set_icon("folded_arrow", "FoldableContainer",
		p_theme->get_icon(SNAME("GuiTreeArrowRight"), EditorStringName(EditorIcons)));
	p_theme->set_icon("folded_arrow_mirrored", "FoldableContainer",
		p_theme->get_icon(SNAME("GuiTreeArrowLeft"), EditorStringName(EditorIcons)));

	p_theme->set_constant("outline_size", "FoldableContainer", 0);
	p_theme->set_constant("h_separation", "FoldableContainer", p_config.separation_margin);
}

// Window and dialogs.
{
	// Window.

	p_theme->set_stylebox("embedded_border", "Window", p_config.window_style);
	p_theme->set_stylebox("embedded_unfocused_border", "Window", p_config.window_style);

	p_theme->set_color("title_color", "Window", p_config.font_color);
	p_theme->set_icon(
		"close", "Window", p_theme->get_icon(SNAME("GuiClose"), EditorStringName(EditorIcons)));
	p_theme->set_icon("close_pressed", "Window",
		p_theme->get_icon(SNAME("GuiClose"), EditorStringName(EditorIcons)));
	p_theme->set_constant("close_h_offset", "Window", 22 * EDSCALE);
	p_theme->set_constant("close_v_offset", "Window", 20 * EDSCALE);
	p_theme->set_constant("title_height", "Window", 24 * EDSCALE);
	p_theme->set_constant("resize_margin", "Window", 4 * EDSCALE);
	p_theme->set_font(
		"title_font", "Window", p_theme->get_font(SNAME("title"), EditorStringName(EditorFonts)));
	p_theme->set_font_size("title_font_size", "Window",
		p_theme->get_font_size(SNAME("title_size"), EditorStringName(EditorFonts)));

	// AcceptDialog.
	p_theme->set_stylebox(SceneStringName(panel), "AcceptDialog", p_config.dialog_style);
	p_theme->set_constant("buttons_separation", "AcceptDialog", 8 * EDSCALE);
	// Make buttons with short texts such as "OK" easier to click/tap.
	p_theme->set_constant(
		"buttons_min_width", "AcceptDialog", p_config.dialogs_buttons_min_size.x * EDSCALE);
	p_theme->set_constant(
		"buttons_min_height", "AcceptDialog", p_config.dialogs_buttons_min_size.y * EDSCALE);

	// FileDialog.
	p_theme->set_icon(
		"folder", "FileDialog", p_theme->get_icon("Folder", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"file", "FileDialog", p_theme->get_icon("File", EditorStringName(EditorIcons)));
	p_theme->set_icon("folder_thumbnail", "FileDialog",
		p_theme->get_icon("FolderBigThumb", EditorStringName(EditorIcons)));
	p_theme->set_icon("file_thumbnail", "FileDialog",
		p_theme->get_icon("FileBigThumb", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"parent_folder", "FileDialog", p_theme->get_icon("ArrowUp", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"back_folder", "FileDialog", p_theme->get_icon("Back", EditorStringName(EditorIcons)));
	p_theme->set_icon("forward_folder", "FileDialog",
		p_theme->get_icon("Forward", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"reload", "FileDialog", p_theme->get_icon("Reload", EditorStringName(EditorIcons)));
	p_theme->set_icon("toggle_hidden", "FileDialog",
		p_theme->get_icon("GuiVisibilityVisible", EditorStringName(EditorIcons)));
	p_theme->set_icon("toggle_filename_filter", "FileDialog",
		p_theme->get_icon("FilenameFilter", EditorStringName(EditorIcons)));
	p_theme->set_icon("thumbnail_mode", "FileDialog",
		p_theme->get_icon("FileThumbnail", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"list_mode", "FileDialog", p_theme->get_icon("FileList", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"sort", "FileDialog", p_theme->get_icon("Sort", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"favorite", "FileDialog", p_theme->get_icon("Favorites", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"favorite_up", "FileDialog", p_theme->get_icon("MoveUp", EditorStringName(EditorIcons)));
	p_theme->set_icon("favorite_down", "FileDialog",
		p_theme->get_icon("MoveDown", EditorStringName(EditorIcons)));
	p_theme->set_icon("create_folder", "FileDialog",
		p_theme->get_icon("FolderCreate", EditorStringName(EditorIcons)));

	p_theme->set_icon("menu_copy_path", "FileDialog",
		p_theme->get_icon("ActionCopy", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"menu_delete", "FileDialog", p_theme->get_icon("Remove", EditorStringName(EditorIcons)));
	p_theme->set_icon(
		"menu_refresh", "FileDialog", p_theme->get_icon("Reload", EditorStringName(EditorIcons)));
	p_theme->set_icon("menu_new_folder", "FileDialog",
		p_theme->get_icon("Folder", EditorStringName(EditorIcons)));
	p_theme->set_icon("menu_show_in_file_manager", "FileDialog",
		p_theme->get_icon("Filesystem", EditorStringName(EditorIcons)));
	p_theme->set_icon("menu_open_bundle", "FileDialog",
		p_theme->get_icon("FolderBrowse", EditorStringName(EditorIcons)));
	// Use a different color for folder icons to make them easier to distinguish from files.
	// On a light theme, the icon will be dark, so we need to lighten it before blending it with the
	// accent color.
	p_theme->set_color("folder_icon_color", "FileDialog",
		(p_config.dark_icon_and_font ? Color(1, 1, 1) : Color(4.25, 4.25, 4.25))
			.lerp(p_config.accent_color, 0.7));
	p_theme->set_color("file_disabled_color", "FileDialog", p_config.font_disabled_color);

	p_theme->set_constant("thumbnail_size", "EditorFileDialog", p_config.thumb_size);

	// PopupDialog.
	p_theme->set_stylebox(SceneStringName(panel), "PopupDialog", p_config.dialog_style);

	// PopupMenu.
	{
		Ref<StyleBoxFlat> style_popup_menu = p_config.base_style->duplicate();
		style_popup_menu->set_bg_color(p_config.surface_popup_color);
		style_popup_menu->set_content_margin_all(p_config.popup_margin);
		style_popup_menu->set_corner_radius_all(0);
		if (p_config.draw_extra_borders) {
			style_popup_menu->set_border_width_all(Math::round(EDSCALE));
			style_popup_menu->set_border_color(p_config.extra_border_color_2);
		}
		p_theme->set_stylebox(SceneStringName(panel), "PopupMenu", style_popup_menu);

		Ref<StyleBoxFlat> style_popup_hover = p_config.flat_button_hover->duplicate();
		style_popup_hover->set_bg_color(_get_base_color(p_config, -0.5, 0.75));

		p_theme->set_stylebox(SceneStringName(hover), "PopupMenu", style_popup_hover);

		Ref<StyleBoxLine> style_popup_separator = EditorThemeManager::make_line_stylebox(
			p_config.mono_color * Color(1, 1, 1, p_config.dark_theme ? 0.075 : 0.125),
			Math::round(2 * EDSCALE), p_config.base_margin * -2 * EDSCALE,
			p_config.base_margin * -2 * EDSCALE);

		p_theme->set_stylebox("separator", "PopupMenu", style_popup_separator);
		p_theme->set_stylebox("labeled_separator_left", "PopupMenu", style_popup_separator);
		p_theme->set_stylebox("labeled_separator_right", "PopupMenu", style_popup_separator);

		p_theme->set_color(SceneStringName(font_color), "PopupMenu", p_config.font_color);
		p_theme->set_color("font_hover_color", "PopupMenu", p_config.font_hover_color);
		p_theme->set_color("font_accelerator_color", "PopupMenu", p_config.font_disabled_color);
		p_theme->set_color("font_disabled_color", "PopupMenu", p_config.font_disabled_color);
		p_theme->set_color("font_separator_color", "PopupMenu", p_config.font_disabled_color);
		p_theme->set_color("font_outline_color", "PopupMenu", p_config.font_outline_color);

		p_theme->set_icon("checked", "PopupMenu",
			p_theme->get_icon(SNAME("GuiChecked"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked", "PopupMenu",
			p_theme->get_icon(SNAME("GuiUnchecked"), EditorStringName(EditorIcons)));
		p_theme->set_icon("indeterminate", "PopupMenu",
			p_theme->get_icon(SNAME("GuiIndeterminate"), EditorStringName(EditorIcons)));
		p_theme->set_icon("radio_checked", "PopupMenu",
			p_theme->get_icon(SNAME("GuiRadioChecked"), EditorStringName(EditorIcons)));
		p_theme->set_icon("radio_unchecked", "PopupMenu",
			p_theme->get_icon(SNAME("GuiRadioUnchecked"), EditorStringName(EditorIcons)));
		p_theme->set_icon("checked_disabled", "PopupMenu",
			p_theme->get_icon(SNAME("GuiCheckedDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("unchecked_disabled", "PopupMenu",
			p_theme->get_icon(SNAME("GuiUncheckedDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("indeterminate_disabled", "PopupMenu",
			p_theme->get_icon(SNAME("GuiIndeterminateDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("radio_checked_disabled", "PopupMenu",
			p_theme->get_icon(SNAME("GuiRadioCheckedDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("radio_unchecked_disabled", "PopupMenu",
			p_theme->get_icon(SNAME("GuiRadioUncheckedDisabled"), EditorStringName(EditorIcons)));
		p_theme->set_icon("submenu", "PopupMenu",
			p_theme->get_icon(SNAME("ArrowRight"), EditorStringName(EditorIcons)));
		p_theme->set_icon("submenu_mirrored", "PopupMenu",
			p_theme->get_icon(SNAME("ArrowLeft"), EditorStringName(EditorIcons)));
		p_theme->set_icon("search", "PopupMenu",
			p_theme->get_icon(SNAME("Search"), EditorStringName(EditorIcons)));

		p_theme->set_constant("h_separation", "PopupMenu", p_config.base_margin * 1.75 * EDSCALE);
		int v_sep =
			(p_config.enable_touch_optimizations ? 12 : p_config.base_margin * 1.75) * EDSCALE;
		p_theme->set_constant("v_separation", "PopupMenu", v_sep);
		p_theme->set_constant("search_bar_separation", "PopupMenu", v_sep);
		p_theme->set_constant("outline_size", "PopupMenu", 0);
		p_theme->set_constant("item_start_padding", "PopupMenu", p_config.popup_margin);
		p_theme->set_constant("item_end_padding", "PopupMenu", p_config.popup_margin);
	}
}

// Sliders and scrollbars.
{
	Ref<Texture2D> empty_icon = memnew(ImageTexture);

	Ref<StyleBoxFlat> grabber_style = p_config.base_style->duplicate();
	grabber_style->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.225));

	Ref<StyleBoxFlat> grabber_hl_style = p_config.base_style->duplicate();
	grabber_hl_style->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.5));

	int scroll_margin = (p_config.enable_touch_optimizations ? 10 : 3) * EDSCALE;

	// HScrollBar.

	Ref<StyleBoxEmpty> h_scroll_style = p_config.base_empty_style->duplicate();
	h_scroll_style->set_content_margin_individual(0, scroll_margin, 0, scroll_margin);

	p_theme->set_stylebox("scroll", "HScrollBar", h_scroll_style);
	p_theme->set_stylebox("scroll_focus", "HScrollBar", p_config.focus_style);
	p_theme->set_stylebox("grabber", "HScrollBar", grabber_style);
	p_theme->set_stylebox("grabber_highlight", "HScrollBar", grabber_hl_style);
	p_theme->set_stylebox("grabber_pressed", "HScrollBar", grabber_hl_style);

	p_theme->set_icon("increment", "HScrollBar", empty_icon);
	p_theme->set_icon("increment_highlight", "HScrollBar", empty_icon);
	p_theme->set_icon("increment_pressed", "HScrollBar", empty_icon);
	p_theme->set_icon("decrement", "HScrollBar", empty_icon);
	p_theme->set_icon("decrement_highlight", "HScrollBar", empty_icon);
	p_theme->set_icon("decrement_pressed", "HScrollBar", empty_icon);

	p_theme->set_constant("padding_top", "HScrollBar", p_config.base_margin * EDSCALE);
	p_theme->set_constant("padding_bottom", "HScrollBar", p_config.base_margin * EDSCALE);

	// VScrollBar.

	Ref<StyleBoxEmpty> v_scroll_style = p_config.base_empty_style->duplicate();
	v_scroll_style->set_content_margin_individual(scroll_margin, 0, scroll_margin, 0);

	p_theme->set_stylebox("scroll", "VScrollBar", v_scroll_style);
	p_theme->set_stylebox("scroll_focus", "VScrollBar", p_config.focus_style);
	p_theme->set_stylebox("grabber", "VScrollBar", grabber_style);
	p_theme->set_stylebox("grabber_highlight", "VScrollBar", grabber_hl_style);
	p_theme->set_stylebox("grabber_pressed", "VScrollBar", grabber_hl_style);

	p_theme->set_icon("increment", "VScrollBar", empty_icon);
	p_theme->set_icon("increment_highlight", "VScrollBar", empty_icon);
	p_theme->set_icon("increment_pressed", "VScrollBar", empty_icon);
	p_theme->set_icon("decrement", "VScrollBar", empty_icon);
	p_theme->set_icon("decrement_highlight", "VScrollBar", empty_icon);
	p_theme->set_icon("decrement_pressed", "VScrollBar", empty_icon);

	p_theme->set_constant("padding_left", "VScrollBar", p_config.base_margin * EDSCALE);
	p_theme->set_constant("padding_right", "VScrollBar", p_config.base_margin * EDSCALE);

	// Slider
	const int background_margin = MAX(2, p_config.base_margin / 2);

	Ref<StyleBoxFlat> style_h_slider = p_config.base_style->duplicate();
	style_h_slider->set_bg_color(p_config.mono_color_inv * Color(1, 1, 1, 0.35));
	style_h_slider->set_content_margin_individual(0, 2 * EDSCALE, 0, 2 * EDSCALE);

	// HSlider.
	p_theme->set_icon("grabber_highlight", "HSlider",
		p_theme->get_icon(SNAME("GuiSliderGrabberHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("grabber", "HSlider",
		p_theme->get_icon(SNAME("GuiSliderGrabber"), EditorStringName(EditorIcons)));
	p_theme->set_stylebox("slider", "HSlider", style_h_slider);
	p_theme->set_stylebox("grabber_area", "HSlider",
		EditorThemeManager::make_flat_stylebox(p_config.contrast_color_1, 0, background_margin, 0,
			background_margin, p_config.corner_radius));
	p_theme->set_stylebox("grabber_area_highlight", "HSlider",
		EditorThemeManager::make_flat_stylebox(
			p_config.contrast_color_1, 0, background_margin, 0, background_margin));
	p_theme->set_constant("center_grabber", "HSlider", 0);
	p_theme->set_constant("grabber_offset", "HSlider", 0);

	Ref<StyleBoxFlat> style_v_slider = style_h_slider->duplicate();
	style_v_slider->set_content_margin_individual(2 * EDSCALE, 0, 2 * EDSCALE, 0);

	// VSlider.
	p_theme->set_icon("grabber", "VSlider",
		p_theme->get_icon(SNAME("GuiSliderGrabber"), EditorStringName(EditorIcons)));
	p_theme->set_icon("grabber_highlight", "VSlider",
		p_theme->get_icon(SNAME("GuiSliderGrabberHl"), EditorStringName(EditorIcons)));
	p_theme->set_stylebox("slider", "VSlider", style_v_slider);
	p_theme->set_stylebox("grabber_area", "VSlider",
		EditorThemeManager::make_flat_stylebox(p_config.contrast_color_1, background_margin, 0,
			background_margin, 0, p_config.corner_radius));
	p_theme->set_stylebox("grabber_area_highlight", "VSlider",
		EditorThemeManager::make_flat_stylebox(
			p_config.contrast_color_1, background_margin, 0, background_margin, 0));
	p_theme->set_constant("center_grabber", "VSlider", 0);
	p_theme->set_constant("grabber_offset", "VSlider", 0);
}

// Labels.
{
	// RichTextLabel.

	Ref<StyleBoxFlat> rich_text_style = p_config.base_style->duplicate();
	rich_text_style->set_bg_color(p_config.surface_low_color);
	rich_text_style->set_content_margin_all(p_config.base_margin * 2 * EDSCALE);

	p_theme->set_stylebox("focus", "RichTextLabel", EditorThemeManager::make_empty_stylebox());

	p_theme->set_color("default_color", "RichTextLabel", p_config.font_color);
	p_theme->set_color("font_shadow_color", "RichTextLabel", Color(1, 1, 1, 0));
	p_theme->set_color("font_outline_color", "RichTextLabel", p_config.font_outline_color);
	p_theme->set_color("selection_color", "RichTextLabel", p_config.selection_color);

	p_theme->set_constant("shadow_offset_x", "RichTextLabel", 1 * EDSCALE);
	p_theme->set_constant("shadow_offset_y", "RichTextLabel", 1 * EDSCALE);
	p_theme->set_constant("shadow_outline_size", "RichTextLabel", 1 * EDSCALE);
	p_theme->set_constant("outline_size", "RichTextLabel", 0);

	// Label.

	Ref<StyleBoxEmpty> label_style = p_config.base_empty_style->duplicate();
	label_style->set_content_margin_individual(p_config.base_margin * 2 * EDSCALE,
		p_config.base_margin * EDSCALE, p_config.base_margin * 2 * EDSCALE,
		p_config.base_margin * EDSCALE);

	p_theme->set_stylebox("focus", "Label", p_config.focus_style);

	p_theme->set_color(SceneStringName(font_color), "Label", p_config.font_color);
	p_theme->set_color("font_shadow_color", "Label", Color(1, 1, 1, 0));
	p_theme->set_color("font_outline_color", "Label", p_config.font_outline_color);

	p_theme->set_constant("shadow_offset_x", "Label", 1 * EDSCALE);
	p_theme->set_constant("shadow_offset_y", "Label", 1 * EDSCALE);
	p_theme->set_constant("shadow_outline_size", "Label", 1 * EDSCALE);
	p_theme->set_constant("line_spacing", "Label", 3 * EDSCALE);
	p_theme->set_constant("outline_size", "Label", 0);

	// Label with different margins.

	p_theme->set_type_variation("LabelNoMargin", "Label");

	p_theme->set_type_variation("LabelNoMarginVertical", "Label");
	Ref<StyleBoxEmpty> no_v_margin_style = label_style->duplicate();
	no_v_margin_style->set_content_margin(SIDE_TOP, 0);
	no_v_margin_style->set_content_margin(SIDE_BOTTOM, 0);
}

// SpinBox.
{
	Ref<Texture2D> empty_icon = memnew(ImageTexture);
	p_theme->set_icon("updown", "SpinBox", empty_icon);
	p_theme->set_icon(
		"up", "SpinBox", p_theme->get_icon(SNAME("GuiSpinboxUp"), EditorStringName(EditorIcons)));
	p_theme->set_icon("up_hover", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxUp"), EditorStringName(EditorIcons)));
	p_theme->set_icon("up_pressed", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxUp"), EditorStringName(EditorIcons)));
	p_theme->set_icon("up_disabled", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxUp"), EditorStringName(EditorIcons)));
	p_theme->set_icon("down", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("down_hover", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("down_pressed", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("down_disabled", "SpinBox",
		p_theme->get_icon(SNAME("GuiSpinboxDown"), EditorStringName(EditorIcons)));

	p_theme->set_stylebox("up_background", "SpinBox", EditorThemeManager::make_empty_stylebox());
	p_theme->set_stylebox("up_background_hovered", "SpinBox", p_config.button_style_hover);
	p_theme->set_stylebox("up_background_pressed", "SpinBox", p_config.button_style_pressed);
	p_theme->set_stylebox(
		"up_background_disabled", "SpinBox", EditorThemeManager::make_empty_stylebox());
	p_theme->set_stylebox("down_background", "SpinBox", EditorThemeManager::make_empty_stylebox());
	p_theme->set_stylebox("down_background_hovered", "SpinBox", p_config.button_style_hover);
	p_theme->set_stylebox("down_background_pressed", "SpinBox", p_config.button_style_pressed);
	p_theme->set_stylebox(
		"down_background_disabled", "SpinBox", EditorThemeManager::make_empty_stylebox());

	p_theme->set_color("up_icon_modulate", "SpinBox", p_config.icon_normal_color);
	p_theme->set_color("up_hover_icon_modulate", "SpinBox", p_config.icon_hover_color);
	p_theme->set_color("up_pressed_icon_modulate", "SpinBox", p_config.icon_pressed_color);
	p_theme->set_color("up_disabled_icon_modulate", "SpinBox", p_config.icon_disabled_color);
	p_theme->set_color("down_icon_modulate", "SpinBox", p_config.icon_normal_color);
	p_theme->set_color("down_hover_icon_modulate", "SpinBox", p_config.icon_hover_color);
	p_theme->set_color("down_pressed_icon_modulate", "SpinBox", p_config.icon_pressed_color);
	p_theme->set_color("down_disabled_icon_modulate", "SpinBox", p_config.icon_disabled_color);

	p_theme->set_stylebox(
		"field_and_buttons_separator", "SpinBox", EditorThemeManager::make_empty_stylebox());
	p_theme->set_stylebox(
		"up_down_buttons_separator", "SpinBox", EditorThemeManager::make_empty_stylebox());

	p_theme->set_constant("buttons_vertical_separation", "SpinBox", 0);
	p_theme->set_constant("field_and_buttons_separation", "SpinBox", 2);
	p_theme->set_constant("buttons_width", "SpinBox", 16);
#ifndef DISABLE_DEPRECATED
	p_theme->set_constant("set_min_buttons_width_from_icons", "SpinBox", 1);
#endif
}

// ProgressBar.

Ref<StyleBoxFlat> progress_bar_style = p_config.base_style->duplicate();
progress_bar_style->set_bg_color(p_config.surface_lowest_color);
progress_bar_style->set_expand_margin(SIDE_TOP, p_config.base_margin * 0.5 * EDSCALE);
progress_bar_style->set_expand_margin(SIDE_BOTTOM, p_config.base_margin * 0.5 * EDSCALE);
progress_bar_style->set_content_margin_all(p_config.base_margin* EDSCALE);
if (p_config.draw_extra_borders) {
	progress_bar_style->set_border_width_all(Math::round(EDSCALE));
	progress_bar_style->set_border_color(p_config.extra_border_color_2);
}

Ref<StyleBoxFlat> progress_fill_style = progress_bar_style->duplicate();
progress_fill_style->set_bg_color(p_config.button_normal_color);
if (p_config.draw_extra_borders) {
	progress_fill_style->set_border_color(p_config.extra_border_color_1);
}

p_theme->set_stylebox("background", "ProgressBar", progress_bar_style);
p_theme->set_stylebox("fill", "ProgressBar", progress_fill_style);
p_theme->set_color(SceneStringName(font_color), "ProgressBar", p_config.font_color);
p_theme->set_color("font_outline_color", "ProgressBar", p_config.font_outline_color);
p_theme->set_constant("outline_size", "ProgressBar", 0);

// PopupProgressBar

p_theme->set_type_variation("PopupProgressBar", "ProgressBar");

Ref<StyleBoxFlat> popup_progress_bar_style = progress_bar_style->duplicate();
popup_progress_bar_style->set_bg_color(_get_base_color(p_config, 0.4, 0.9));

Ref<StyleBoxFlat> popup_progress_fill_style = progress_fill_style->duplicate();
popup_progress_fill_style->set_bg_color(_get_base_color(p_config, -1.6, 0.9));
if (p_config.draw_extra_borders) {
	popup_progress_fill_style->set_border_color(p_config.extra_border_color_1);
}

p_theme->set_stylebox("background", "PopupProgressBar", popup_progress_bar_style);
p_theme->set_stylebox("fill", "PopupProgressBar", popup_progress_fill_style);

// ColorPicker and related nodes.
{
	// ColorPicker.
	Ref<StyleBoxFlat> circle_style_focus = p_config.base_style->duplicate();
	circle_style_focus->set_border_color(p_config.mono_color * Color(1, 1, 1, 0.3));
	circle_style_focus->set_draw_center(false);
	circle_style_focus->set_corner_radius_all(256 * EDSCALE);
	circle_style_focus->set_corner_detail(32 * EDSCALE);

	p_theme->set_constant("margin", "ColorPicker", p_config.base_margin);
	p_theme->set_constant("sv_width", "ColorPicker", 256 * EDSCALE);
	p_theme->set_constant("sv_height", "ColorPicker", 256 * EDSCALE);
	p_theme->set_constant("h_width", "ColorPicker", 30 * EDSCALE);
	p_theme->set_constant("label_width", "ColorPicker", 10 * EDSCALE);
	p_theme->set_constant("center_slider_grabbers", "ColorPicker", 1);

	p_theme->set_stylebox("sample_focus", "ColorPicker", p_config.focus_style);
	p_theme->set_stylebox("picker_focus_rectangle", "ColorPicker", p_config.focus_style);
	p_theme->set_stylebox("picker_focus_circle", "ColorPicker", circle_style_focus);
	p_theme->set_color("focused_not_editing_cursor_color", "ColorPicker", p_config.highlight_color);

	p_theme->set_icon("menu_option", "ColorPicker",
		p_theme->get_icon(SNAME("GuiTabMenuHl"), EditorStringName(EditorIcons)));
	p_theme->set_icon("expanded_arrow", "ColorPicker",
		p_theme->get_icon(SNAME("GuiTreeArrowDown"), EditorStringName(EditorIcons)));
	p_theme->set_icon("folded_arrow", "ColorPicker",
		p_theme->get_icon(SNAME("GuiTreeArrowRight"), EditorStringName(EditorIcons)));
	p_theme->set_icon("screen_picker", "ColorPicker",
		p_theme->get_icon(SNAME("ColorPick"), EditorStringName(EditorIcons)));
	p_theme->set_icon("shape_circle", "ColorPicker",
		p_theme->get_icon(SNAME("PickerShapeCircle"), EditorStringName(EditorIcons)));
	p_theme->set_icon("shape_rect", "ColorPicker",
		p_theme->get_icon(SNAME("PickerShapeRectangle"), EditorStringName(EditorIcons)));
	p_theme->set_icon("shape_rect_wheel", "ColorPicker",
		p_theme->get_icon(SNAME("PickerShapeRectangleWheel"), EditorStringName(EditorIcons)));
	p_theme->set_icon("add_preset", "ColorPicker",
		p_theme->get_icon(SNAME("Add"), EditorStringName(EditorIcons)));
	p_theme->set_icon("sample_bg", "ColorPicker",
		p_theme->get_icon(SNAME("GuiMiniCheckerboard"), EditorStringName(EditorIcons)));
	p_theme->set_icon("sample_revert", "ColorPicker",
		p_theme->get_icon(SNAME("Reload"), EditorStringName(EditorIcons)));
	p_theme->set_icon("overbright_indicator", "ColorPicker",
		p_theme->get_icon(SNAME("OverbrightIndicator"), EditorStringName(EditorIcons)));
	p_theme->set_icon("bar_arrow", "ColorPicker",
		p_theme->get_icon(SNAME("ColorPickerBarArrow"), EditorStringName(EditorIcons)));
	p_theme->set_icon("picker_cursor", "ColorPicker",
		p_theme->get_icon(SNAME("PickerCursor"), EditorStringName(EditorIcons)));
	p_theme->set_icon("picker_cursor_bg", "ColorPicker",
		p_theme->get_icon(SNAME("PickerCursorBg"), EditorStringName(EditorIcons)));
	p_theme->set_icon("color_script", "ColorPicker",
		p_theme->get_icon(SNAME("Script"), EditorStringName(EditorIcons)));
	p_theme->set_icon("color_copy", "ColorPicker",
		p_theme->get_icon(SNAME("ActionCopy"), EditorStringName(EditorIcons)));

	// ColorPickerButton.
	p_theme->set_icon("bg", "ColorPickerButton",
		p_theme->get_icon(SNAME("GuiMiniCheckerboard"), EditorStringName(EditorIcons)));

	// ColorPresetButton.
	p_theme->set_stylebox("preset_fg", "ColorPresetButton",
		EditorThemeManager::make_flat_stylebox(Color(1, 1, 1), 2, 2, 2, 2, 2));
	p_theme->set_icon("preset_bg", "ColorPresetButton",
		p_theme->get_icon(SNAME("GuiMiniCheckerboard"), EditorStringName(EditorIcons)));
	p_theme->set_icon("overbright_indicator", "ColorPresetButton",
		p_theme->get_icon(SNAME("OverbrightIndicator"), EditorStringName(EditorIcons)));
}
}


