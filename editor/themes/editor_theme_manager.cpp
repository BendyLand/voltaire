/**************************************************************************/
/*  editor_theme_manager.cpp                                              */
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

#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_icons.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme.h"
#include "editor/themes/theme_classic.h"
#include "editor/themes/theme_modern.h"
#include "editor_theme_manager.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h"
#include "scene/resources/texture.h"
#include "scene/scene_string_names.h"
#include "servers/display/display_server.h"

// Theme configuration.

uint32_t EditorThemeManager::ThemeConfiguration::hash()
{
	uint32_t hash = hash_murmur3_one_float(EDSCALE);

	// Basic properties.

	hash = hash_murmur3_one_32(style.hash(), hash);
	hash = hash_murmur3_one_32(preset.hash(), hash);
	hash = hash_murmur3_one_32(spacing_preset.hash(), hash);

	hash = hash_murmur3_one_32(base_color.to_rgba32(), hash);
	hash = hash_murmur3_one_32(accent_color.to_rgba32(), hash);
	hash = hash_murmur3_one_float(contrast, hash);
	hash = hash_murmur3_one_float(icon_saturation, hash);

	// Extra properties.

	hash = hash_murmur3_one_32(base_spacing, hash);
	hash = hash_murmur3_one_32(extra_spacing, hash);
	hash = hash_murmur3_one_32(border_width, hash);
	hash = hash_murmur3_one_32(corner_radius, hash);

	hash = hash_murmur3_one_32((int)draw_extra_borders, hash);
	hash = hash_murmur3_one_float(relationship_line_opacity, hash);
	hash = hash_murmur3_one_32(thumb_size, hash);
	hash = hash_murmur3_one_32(class_icon_size, hash);
	hash = hash_murmur3_one_32((int)enable_touch_optimizations, hash);
	hash = hash_murmur3_one_float(gizmo_handle_scale, hash);
	hash = hash_murmur3_one_32(inspector_property_height, hash);
	hash = hash_murmur3_one_float(subresource_hue_tint, hash);

	hash = hash_murmur3_one_float(default_contrast, hash);

	// Generated properties.

	hash = hash_murmur3_one_32((int)dark_theme, hash);
	hash = hash_murmur3_one_32((int)dark_icon_and_font, hash);

	return hash;
}

uint32_t EditorThemeManager::ThemeConfiguration::hash_fonts()
{
	uint32_t hash = hash_murmur3_one_float(EDSCALE);

	// TODO: Implement the hash based on what editor_register_fonts() uses.

	return hash;
}

uint32_t EditorThemeManager::ThemeConfiguration::hash_icons()
{
	uint32_t hash = hash_murmur3_one_float(EDSCALE);

	hash = hash_murmur3_one_32(accent_color.to_rgba32(), hash);
	hash = hash_murmur3_one_float(icon_saturation, hash);

	hash = hash_murmur3_one_32(thumb_size, hash);
	hash = hash_murmur3_one_float(gizmo_handle_scale, hash);

	hash = hash_murmur3_one_32((int)dark_icon_and_font, hash);

	return hash;
}

// Benchmarks.

int EditorThemeManager::benchmark_run = 0;

String EditorThemeManager::get_benchmark_key()
{
	if (benchmark_run == 0) {
		return "EditorTheme (Startup)";
	}

	return vformat("EditorTheme (Run %d)", benchmark_run);
}

// Generation helper methods.

Ref<StyleBoxTexture> EditorThemeManager::make_stylebox(Ref<Texture2D> p_texture, float p_left,
	float p_top, float p_right, float p_bottom, float p_margin_left, float p_margin_top,
	float p_margin_right, float p_margin_bottom, bool p_draw_center)
{
	Ref<StyleBoxTexture> style(memnew(StyleBoxTexture));
	style->set_texture(p_texture);
	style->set_texture_margin_individual(
		p_left * EDSCALE, p_top * EDSCALE, p_right * EDSCALE, p_bottom * EDSCALE);
	style->set_content_margin_individual((p_left + p_margin_left) * EDSCALE,
		(p_top + p_margin_top) * EDSCALE, (p_right + p_margin_right) * EDSCALE,
		(p_bottom + p_margin_bottom) * EDSCALE);
	style->set_draw_center(p_draw_center);
	return style;
}

Ref<StyleBoxEmpty> EditorThemeManager::make_empty_stylebox(
	float p_margin_left, float p_margin_top, float p_margin_right, float p_margin_bottom)
{
	Ref<StyleBoxEmpty> style(memnew(StyleBoxEmpty));
	style->set_content_margin_individual(p_margin_left * EDSCALE, p_margin_top * EDSCALE,
		p_margin_right * EDSCALE, p_margin_bottom * EDSCALE);
	return style;
}

Ref<StyleBoxFlat> EditorThemeManager::make_flat_stylebox(Color p_color, float p_margin_left,
	float p_margin_top, float p_margin_right, float p_margin_bottom, int p_corner_width)
{
	Ref<StyleBoxFlat> style(memnew(StyleBoxFlat));
	style->set_bg_color(p_color);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(Math::ceil(0.8 * p_corner_width * EDSCALE));
	style->set_corner_radius_all(p_corner_width * EDSCALE);
	style->set_content_margin_individual(p_margin_left * EDSCALE, p_margin_top * EDSCALE,
		p_margin_right * EDSCALE, p_margin_bottom * EDSCALE);
	return style;
}

Ref<StyleBoxLine> EditorThemeManager::make_line_stylebox(
	Color p_color, int p_thickness, float p_grow_begin, float p_grow_end, bool p_vertical)
{
	Ref<StyleBoxLine> style(memnew(StyleBoxLine));
	style->set_color(p_color);
	style->set_grow_begin(p_grow_begin);
	style->set_grow_end(p_grow_end);
	style->set_thickness(p_thickness);
	style->set_vertical(p_vertical);
	return style;
}

// Theme generation and population routines.

Ref<EditorTheme> EditorThemeManager::_create_base_theme(const Ref<EditorTheme>& p_old_theme)
{
	OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Create Base Theme");

	Ref<EditorTheme> theme = memnew(EditorTheme);
	ThemeConfiguration config = _create_theme_config();
	theme->set_generated_hash(config.hash());
	theme->set_generated_fonts_hash(config.hash_fonts());
	theme->set_generated_icons_hash(config.hash_icons());

	print_verbose(vformat(
		"EditorTheme: Generating new theme for the config '%d'.", theme->get_generated_hash()));

	bool is_default_style = config.style == "Modern";
	if (is_default_style) {
		ThemeModern::populate_shared_styles(theme, config);
	}
	else {
		ThemeClassic::populate_shared_styles(theme, config);
	}

	// Register icons.
	{
		OS::get_singleton()->benchmark_begin_measure(get_benchmark_key(), "Register Icons");

		// External functions, see editor_icons.cpp.
		editor_configure_icons(config.dark_icon_and_font);

		// If settings are comparable to the old theme, then just copy existing icons over.
		// Otherwise, regenerate them.
		bool keep_old_icons =
			(p_old_theme.is_valid() &&
				theme->get_generated_icons_hash() == p_old_theme->get_generated_icons_hash());
		if (keep_old_icons) {
			print_verbose("EditorTheme: Can keep old icons, copying.");
			editor_copy_icons(theme, p_old_theme);
		}
		else {
			print_verbose("EditorTheme: Generating new icons.");
			editor_register_icons(theme, config.dark_icon_and_font, config.icon_saturation,
				config.thumb_size, config.gizmo_handle_scale);
		}

		OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Register Icons");
	}

	// TODO: Check if existing style definitions from the old theme are usable and copy them.

	print_verbose("EditorTheme: Generating new styles.");

	if (is_default_style) {
		ThemeModern::populate_standard_styles(theme, config);
		ThemeModern::populate_editor_styles(theme, config);
	}
	else {
		ThemeClassic::populate_standard_styles(theme, config);
		ThemeClassic::populate_editor_styles(theme, config);
	}

	_populate_text_editor_styles(theme, config);
	_populate_visual_shader_styles(theme, config);

	OS::get_singleton()->benchmark_end_measure(get_benchmark_key(), "Create Base Theme");
	return theme;
}

void EditorThemeManager::_reset_dirty_flag() { outdated_cache_dirty = true; }

// Public interface for theme generation.

bool EditorThemeManager::is_generated_theme_outdated()
{
	// This list includes settings used by files in the editor/themes folder.
	// Note that the editor scale is purposefully omitted because it cannot be changed
	// without a restart, so there is no point regenerating the theme.

	if (outdated_cache_dirty) {
		// TODO: We can use this information more intelligently to do partial theme updates and
		// speed things up.
		outdated_cache =
			EditorSettings::get_singleton()->check_changed_settings_in_group("interface/theme") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"interface/editor/fonts") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"interface/editor/appearance/max_sticky_tree_items") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"interface/touchscreen/enable_touch_optimizations") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"editors/visual_editors") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor/theme") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"text_editor/help/help") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"docks/property_editor/subresource_hue_tint") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"filesystem/file_dialog/thumbnail_size") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"run/output/font_size");

		// The outdated flag is relevant at the moment of changing editor settings.
		outdated_cache_dirty = false;
	}

	return outdated_cache;
}

void EditorThemeManager::initialize()
{
	EditorColorMap::create();
	EditorTheme::initialize();
}

void EditorThemeManager::finalize()
{
	EditorColorMap::finish();
	EditorTheme::finalize();
}


