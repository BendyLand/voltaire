/**************************************************************************/
/*  default_theme.cpp                                                     */
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

#include "core/io/image.h"
#include "default_theme.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/theme.h"
#include "scene/scene_string_names.h"
#include "scene/theme/default_theme_icons.gen.h"
#include "scene/theme/theme_db.h"
#include "servers/text/text_server.h"

#ifdef BROTLI_ENABLED
#include "scene/theme/default_font.gen.h"
#endif

static const int default_font_size = 16;

static float scale = 1.0;

static const int default_margin = 4;
static const int default_corner_radius = 3;

static Ref<StyleBoxFlat> make_flat_stylebox(Color p_color, float p_margin_left = default_margin,
	float p_margin_top = default_margin, float p_margin_right = default_margin,
	float p_margin_bottom = default_margin, int p_corner_radius = default_corner_radius,
	bool p_draw_center = true, int p_border_width = 0)
{
	Ref<StyleBoxFlat> style(memnew(StyleBoxFlat));
	style->set_bg_color(p_color);
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));

	style->set_corner_radius_all(Math::round(p_corner_radius * scale));
	style->set_anti_aliased(true);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(MIN(Math::ceil(1.5 * p_corner_radius), 6) * scale);

	style->set_draw_center(p_draw_center);
	style->set_border_width_all(Math::round(p_border_width * scale));

	return style;
}

static Ref<StyleBoxFlat> sb_expand(
	Ref<StyleBoxFlat> p_sbox, float p_left, float p_top, float p_right, float p_bottom)
{
	p_sbox->set_expand_margin(SIDE_LEFT, Math::round(p_left * scale));
	p_sbox->set_expand_margin(SIDE_TOP, Math::round(p_top * scale));
	p_sbox->set_expand_margin(SIDE_RIGHT, Math::round(p_right * scale));
	p_sbox->set_expand_margin(SIDE_BOTTOM, Math::round(p_bottom * scale));
	return p_sbox;
}

static Ref<StyleBox> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1,
	float p_margin_right = -1, float p_margin_bottom = -1)
{
	Ref<StyleBox> style(memnew(StyleBoxEmpty));
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));
	return style;
}

void make_default_theme(float p_scale, Ref<Font> p_font,
	TextServer::SubpixelPositioning p_font_subpixel, TextServer::Hinting p_font_hinting,
	TextServer::FontAntialiasing p_font_antialiasing, bool p_font_msdf,
	bool p_font_generate_mipmaps)
{
	Ref<Theme> t;
	t.instantiate();
	Ref<StyleBox> default_style;
	Ref<Texture2D> default_icon;
	Ref<Font> default_font;
	Ref<FontVariation> bold_font;
	Ref<FontVariation> bold_italics_font;
	Ref<FontVariation> italics_font;
	float default_scale = CLAMP(p_scale, 0.5, 8.0);
	if (p_font.is_valid()) {
		// Use the custom font defined in the Project Settings.
		default_font = p_font;
	}
	else {
		// Use the default DynamicFont (separate from the editor font).
		// The default DynamicFont is chosen to have a small file size since it's
		// embedded in both editor and export template binaries.
		Ref<FontFile> dynamic_font;
		dynamic_font.instantiate();
#ifdef BROTLI_ENABLED
		dynamic_font->set_data_ptr(_font_OpenSans_SemiBold, _font_OpenSans_SemiBold_size);
		dynamic_font->set_subpixel_positioning(p_font_subpixel);
		dynamic_font->set_hinting(p_font_hinting);
		dynamic_font->set_antialiasing(p_font_antialiasing);
		dynamic_font->set_multichannel_signed_distance_field(p_font_msdf);
		dynamic_font->set_generate_mipmaps(p_font_generate_mipmaps);
#endif
		default_font = dynamic_font;
	}
	if (default_font.is_valid()) {
		bold_font.instantiate();
		bold_font->set_base_font(default_font);
		bold_font->set_variation_embolden(1.2);
		bold_italics_font.instantiate();
		bold_italics_font->set_base_font(default_font);
		bold_italics_font->set_variation_embolden(1.2);
		bold_italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
		italics_font.instantiate();
		italics_font->set_base_font(default_font);
		italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
	}
	fill_default_theme(t, default_font, bold_font, bold_italics_font, italics_font, default_icon,
		default_style, default_scale);
	ThemeDB::get_singleton()->set_default_theme(t);
	ThemeDB::get_singleton()->set_fallback_base_scale(default_scale);
	ThemeDB::get_singleton()->set_fallback_icon(default_icon);
	ThemeDB::get_singleton()->set_fallback_stylebox(default_style);
	ThemeDB::get_singleton()->set_fallback_font(default_font);
	ThemeDB::get_singleton()->set_fallback_font_size(default_font_size * default_scale);
}


