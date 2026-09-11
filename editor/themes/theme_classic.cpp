/**************************************************************************/
/*  theme_classic.cpp                                                     */
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

#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/graph_edit.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h" // IWYU pragma: keep. Used by `EditorThemeManager::make_stylebox`.
#include "theme_classic.h"

void ThemeClassic::populate_shared_styles(
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

		// Ensure base colors are in the 0..1 luminance range to avoid 8-bit integer overflow or
		// text rendering issues. Some places in the editor use 8-bit integer colors.
		p_config.dark_color_1 =
			p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast).clamp();
		p_config.dark_color_2 =
			p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast * 1.5).clamp();
		p_config.dark_color_3 =
			p_config.base_color.lerp(Color(0, 0, 0, 1), p_config.contrast * 2).clamp();

		p_config.contrast_color_1 = p_config.base_color.lerp(
			p_config.mono_color, MAX(p_config.contrast, p_config.default_contrast));
		p_config.contrast_color_2 = p_config.base_color.lerp(
			p_config.mono_color, MAX(p_config.contrast * 1.5, p_config.default_contrast * 1.5));

		p_config.highlight_color =
			Color(p_config.accent_color.r, p_config.accent_color.g, p_config.accent_color.b, 0.275);
		p_config.highlight_disabled_color = p_config.highlight_color.lerp(
			p_config.dark_icon_and_font ? Color(0, 0, 0) : Color(1, 1, 1), 0.5);

		p_config.success_color = Color(0.45, 0.95, 0.5);
		p_config.warning_color = Color(1, 0.87, 0.4);
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
			p_config.warning_color = Color(0.82, 0.56, 0.1);
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
		p_theme->set_color("ruler_color", EditorStringName(Editor), p_config.dark_color_2);
#ifndef DISABLE_DEPRECATED // Used before 4.3.
		p_theme->set_color("disabled_highlight_color", EditorStringName(Editor),
			p_config.highlight_disabled_color);
#endif

		// Only used when the Draw Extra Borders editor setting is enabled.
		p_config.extra_border_color_1 = Color(0.5, 0.5, 0.5);
		p_config.extra_border_color_2 =
			p_config.dark_theme ? Color(0.3, 0.3, 0.3) : Color(0.7, 0.7, 0.7);

		p_theme->set_color(
			"extra_border_color_1", EditorStringName(Editor), p_config.extra_border_color_1);
		p_theme->set_color(
			"extra_border_color_2", EditorStringName(Editor), p_config.extra_border_color_2);

		// Font colors.

		p_config.font_color = p_config.mono_color_font.lerp(p_config.base_color, 0.25);
		p_config.font_focus_color = p_config.mono_color_font.lerp(p_config.base_color, 0.125);
		p_config.font_hover_color = p_config.mono_color_font.lerp(p_config.base_color, 0.125);
		p_config.font_pressed_color = p_config.accent_color;
		p_config.font_hover_pressed_color =
			p_config.font_hover_color.lerp(p_config.accent_color, 0.74);
		p_config.font_disabled_color = Color(p_config.mono_color_font.r, p_config.mono_color_font.g,
			p_config.mono_color_font.b, 0.35);
		p_config.font_readonly_color = Color(p_config.mono_color_font.r, p_config.mono_color_font.g,
			p_config.mono_color_font.b, 0.65);
		p_config.font_placeholder_color = Color(p_config.mono_color_font.r,
			p_config.mono_color_font.g, p_config.mono_color_font.b, 0.5);
		p_config.font_outline_color = Color(0, 0, 0, 0);

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

		p_config.icon_normal_color = Color(1, 1, 1);
		p_config.icon_focus_color =
			p_config.icon_normal_color * (p_config.dark_icon_and_font ? 1.15 : 1.45);
		p_config.icon_focus_color.a = 1.0;
		p_config.icon_hover_color = p_config.icon_focus_color;
		// Make the pressed icon color overbright because icons are not completely white on a dark
		// theme. On a light theme, icons are dark, so we need to modulate them with an even
		// brighter color.
		p_config.icon_pressed_color =
			p_config.accent_color * (p_config.dark_icon_and_font ? 1.15 : 3.5);
		p_config.icon_pressed_color.a = 1.0;
		p_config.icon_disabled_color = Color(p_config.icon_normal_color, 0.4);

		p_theme->set_color(
			"icon_normal_color", EditorStringName(Editor), p_config.icon_normal_color);
		p_theme->set_color("icon_focus_color", EditorStringName(Editor), p_config.icon_focus_color);
		p_theme->set_color("icon_hover_color", EditorStringName(Editor), p_config.icon_hover_color);
		p_theme->set_color(
			"icon_pressed_color", EditorStringName(Editor), p_config.icon_pressed_color);
		p_theme->set_color(
			"icon_disabled_color", EditorStringName(Editor), p_config.icon_disabled_color);

		// Additional GUI colors.

		p_config.shadow_color = Color(0, 0, 0, p_config.dark_theme ? 0.3 : 0.1);
		p_config.selection_color = p_config.accent_color * Color(1, 1, 1, 0.4);
		p_config.disabled_border_color =
			p_config.mono_color.inverted().lerp(p_config.base_color, 0.7);
		p_config.disabled_bg_color = p_config.mono_color.inverted().lerp(p_config.base_color, 0.9);
		p_config.separator_color =
			Color(p_config.mono_color.r, p_config.mono_color.g, p_config.mono_color.b, 0.1);

		p_theme->set_color("selection_color", EditorStringName(Editor), p_config.selection_color);
		p_theme->set_color(
			"disabled_border_color", EditorStringName(Editor), p_config.disabled_border_color);
		p_theme->set_color(
			"disabled_bg_color", EditorStringName(Editor), p_config.disabled_bg_color);
		p_theme->set_color("separator_color", EditorStringName(Editor), p_config.separator_color);

		// Additional editor colors.

		p_theme->set_color("box_selection_fill_color", EditorStringName(Editor),
			p_config.accent_color * Color(1, 1, 1, 0.3));
		p_theme->set_color("box_selection_stroke_color", EditorStringName(Editor),
			p_config.accent_color * Color(1, 1, 1, 0.8));

		p_theme->set_color("axis_x_color", EditorStringName(Editor), Color(0.96, 0.20, 0.32));
		p_theme->set_color("axis_y_color", EditorStringName(Editor), Color(0.53, 0.84, 0.01));
		p_theme->set_color("axis_z_color", EditorStringName(Editor), Color(0.16, 0.55, 0.96));
		p_theme->set_color("axis_w_color", EditorStringName(Editor), Color(0.55, 0.55, 0.55));
		p_theme->set_color(
			"axis_view_plane_color", EditorStringName(Editor), Color(0.75, 0.75, 0.75, 0.33));

		const float prop_color_saturation = p_config.accent_color.get_s() * 0.75;
		const float prop_color_value = p_config.accent_color.get_v();

		p_theme->set_color("property_color_x", EditorStringName(Editor),
			Color::from_hsv(0.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_y", EditorStringName(Editor),
			Color::from_hsv(1.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_z", EditorStringName(Editor),
			Color::from_hsv(2.0 / 3.0 + 0.05, prop_color_saturation, prop_color_value));
		p_theme->set_color("property_color_w", EditorStringName(Editor),
			Color::from_hsv(1.5 / 3.0 + 0.05, prop_color_saturation, prop_color_value));

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
			p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin,
			p_config.corner_radius);
		p_config.base_style->set_border_width_all(p_config.border_width);
		p_config.base_style->set_border_color(p_config.base_color);

		p_config.base_empty_style = EditorThemeManager::make_empty_stylebox(
			p_config.base_margin, p_config.base_margin, p_config.base_margin, p_config.base_margin);

		// Button styles.
		{
			p_config.widget_margin =
				Vector2(p_config.increased_margin + 2, p_config.increased_margin + 1) * EDSCALE;

			p_config.button_style = p_config.base_style->duplicate();
			p_config.button_style->set_content_margin_individual(p_config.widget_margin.x,
				p_config.widget_margin.y, p_config.widget_margin.x, p_config.widget_margin.y);
			p_config.button_style->set_bg_color(p_config.dark_color_1);
			if (p_config.draw_extra_borders) {
				p_config.button_style->set_border_width_all(Math::round(EDSCALE));
				p_config.button_style->set_border_color(p_config.extra_border_color_1);
			}
			else {
				p_config.button_style->set_border_color(p_config.dark_color_2);
			}

			p_config.button_style_disabled = p_config.button_style->duplicate();
			p_config.button_style_disabled->set_bg_color(p_config.disabled_bg_color);
			if (p_config.draw_extra_borders) {
				p_config.button_style_disabled->set_border_color(p_config.extra_border_color_2);
			}
			else {
				p_config.button_style_disabled->set_border_color(p_config.disabled_border_color);
			}

			p_config.button_style_focus = p_config.button_style->duplicate();
			p_config.button_style_focus->set_draw_center(false);
			p_config.button_style_focus->set_border_width_all(Math::round(2 * MAX(1, EDSCALE)));
			p_config.button_style_focus->set_border_color(p_config.accent_color);

			p_config.button_style_pressed = p_config.button_style->duplicate();
			p_config.button_style_pressed->set_bg_color(p_config.dark_color_1.darkened(0.125));

			p_config.button_style_hover = p_config.button_style->duplicate();
			p_config.button_style_hover->set_bg_color(p_config.mono_color * Color(1, 1, 1, 0.11));
			if (p_config.draw_extra_borders) {
				p_config.button_style_hover->set_border_color(p_config.extra_border_color_1);
			}
			else {
				p_config.button_style_hover->set_border_color(
					p_config.mono_color * Color(1, 1, 1, 0.05));
			}
		}

		// Windows and popups.
		{
			p_config.popup_style = p_config.base_style->duplicate();
			p_config.popup_style->set_content_margin_all(p_config.popup_margin);
			p_config.popup_style->set_border_color(p_config.contrast_color_1);
			p_config.popup_style->set_shadow_color(p_config.shadow_color);
			p_config.popup_style->set_shadow_size(4 * EDSCALE);
			// Popups are separate windows by default in the editor. Windows currently don't support
			// per-pixel transparency in 4.0, and even if it was, it may not always work in practice
			// (e.g. running with compositing disabled).
			p_config.popup_style->set_corner_radius_all(0);

			p_config.popup_border_style = p_config.popup_style->duplicate();
			p_config.popup_border_style->set_content_margin_all(
				MAX(Math::round(EDSCALE), p_config.border_width) + 2 +
				(p_config.base_margin * 1.5) * EDSCALE);
			// Always display a border for popups like PopupMenus so they can be distinguished from
			// their background.
			p_config.popup_border_style->set_border_width_all(
				MAX(Math::round(EDSCALE), p_config.border_width));
			if (p_config.draw_extra_borders) {
				p_config.popup_border_style->set_border_color(p_config.extra_border_color_2);
			}
			else {
				p_config.popup_border_style->set_border_color(p_config.dark_color_2);
			}

			p_config.window_style = p_config.popup_style->duplicate();
			p_config.window_style->set_border_color(p_config.base_color);
			p_config.window_style->set_border_width(SIDE_TOP, 24 * EDSCALE);
			p_config.window_style->set_expand_margin(SIDE_TOP, 24 * EDSCALE);

			// Prevent corner artifacts between window title and body.
			p_config.dialog_style = p_config.base_style->duplicate();
			p_config.dialog_style->set_corner_radius(CORNER_TOP_LEFT, 0);
			p_config.dialog_style->set_corner_radius(CORNER_TOP_RIGHT, 0);
			p_config.dialog_style->set_content_margin_all(p_config.popup_margin);
			// Prevent visible line between window title and body.
			p_config.dialog_style->set_expand_margin(SIDE_BOTTOM, 2 * EDSCALE);
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

			p_config.tab_container_style = p_config.content_panel_style;

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


