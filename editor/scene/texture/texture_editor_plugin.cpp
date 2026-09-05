/**************************************************************************/
/*  texture_editor_plugin.cpp                                             */
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
#include "editor/scene/texture/color_channel_selector.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/animated_texture.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/resources/texture_rd.h"
#include "servers/rendering/rendering_device.h"
#include "texture_editor_plugin.h"

constexpr const char* texture_2d_shader_code = R"(
shader_type canvas_item;
render_mode blend_mix;

instance uniform vec4 u_channel_factors = vec4(1.0);
instance uniform float lod = 0.0;

vec4 filter_preview_colors(vec4 input_color, vec4 factors) {
	// Filter RGB.
	vec4 output_color = input_color * vec4(factors.rgb, input_color.a);

	// Remove transparency when alpha is not enabled.
	output_color.a = mix(1.0, output_color.a, factors.a);

	// Switch to opaque grayscale when visualizing only one channel.
	float csum = factors.r + factors.g + factors.b + factors.a;
	float single = clamp(2.0 - csum, 0.0, 1.0);
	for (int i = 0; i < 4; i++) {
		float c = input_color[i];
		output_color = mix(output_color, vec4(c, c, c, 1.0), factors[i] * single);
	}

	return output_color;
}

void fragment() {
	COLOR = filter_preview_colors(textureLod(TEXTURE, UV, lod), u_channel_factors);
}
)";

void TexturePreview::init_shaders()
{
	texture_material.instantiate();

	Ref<Shader> texture_shader;
	texture_shader.instantiate();
	texture_shader->set_code(texture_2d_shader_code);

	texture_material->set_shader(texture_shader);
}

void TexturePreview::finish_shaders() { texture_material.unref(); }

TextureRect* TexturePreview::get_texture_display() { return texture_display; }

void TexturePreview::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (!is_inside_tree()) {
			// TODO: This is a workaround because `NOTIFICATION_THEME_CHANGED`
			// is getting called for some reason when the `TexturePreview` is
			// getting destroyed, which causes `get_theme_font()` to return `nullptr`.
			// See https://github.com/godotengine/godot/issues/50743.
			break;
		}

		if (metadata_label) {
			Ref<Font> metadata_label_font =
				get_theme_font(SNAME("expression"), EditorStringName(EditorFonts));
			metadata_label->add_theme_font_override(
				SceneStringName(font), metadata_label_font.ptr());
		}

		bg_rect->set_color(get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor)));
		checkerboard->set_texture(get_editor_theme_icon(SNAME("Checkerboard")));
		theme_cache.outline_color =
			get_theme_color(SNAME("extra_border_color_1"), EditorStringName(Editor));
	} break;
	}
}

void TexturePreview::_draw_outline()
{
	const float outline_width = Math::round(EDSCALE);
	const Rect2 outline_rect =
		Rect2(Vector2(), outline_overlay->get_size()).grow(outline_width * 0.5);
	outline_overlay->draw_rect(outline_rect, theme_cache.outline_color, false, outline_width);
}

void TexturePreview::_update_texture_display_ratio()
{
	if (texture_display->get_texture().is_valid()) {
		centering_container->set_ratio(texture_display->get_texture()->get_size().aspect());
	}
}

static Image::Format get_texture_2d_format(const Ref<Texture2D>& p_texture)
{
	const Ref<Texture2DRD> rd_texture = p_texture;
	if (rd_texture.is_valid() && RD::get_singleton() &&
		RD::get_singleton()->texture_is_valid(rd_texture->get_texture_rd_rid())) {
		return rd_texture->get_image()->get_format();
	}

	return p_texture->get_format();
}

static int get_texture_mipmaps_count(const Ref<Texture2D>& p_texture)
{
	ERR_FAIL_COND_V(p_texture.is_null(), -1);

	// We are having to download the image only to get its mipmaps count. It would be nice if we
	// didn't have to.
	Ref<Image> image;
	Ref<AtlasTexture> at = p_texture;
	Ref<Texture2DRD> rd_texture = p_texture;

	if (at.is_valid()) {
		// The AtlasTexture tries to obtain the region from the atlas as an image,
		// which will fail if it is a compressed format.
		Ref<Texture2D> atlas = at->get_atlas();
		if (atlas.is_valid()) {
			image = atlas->get_image();
		}
	}
	else if (rd_texture.is_valid()) {
		if (RD::get_singleton() &&
			RD::get_singleton()->texture_is_valid(rd_texture->get_texture_rd_rid())) {
			return -1;
		}
		image = p_texture->get_image();
	}
	else {
		image = p_texture->get_image();
	}

	if (image.is_valid()) {
		return image->get_mipmap_count();
	}
	return -1;
}

TextureEditorPlugin::TextureEditorPlugin()
{
	Ref<EditorInspectorPluginTexture> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}


