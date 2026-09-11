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


