/**************************************************************************/
/*  texture_3d_editor_plugin.cpp                                          */
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
#include "scene/gui/label.h"
#include "texture_3d_editor_plugin.h"

// Shader sources.

constexpr const char* texture_3d_shader = R"(
	// Texture3DEditor preview shader.

	shader_type canvas_item;

	uniform sampler3D tex;
	uniform float layer;

	uniform vec4 u_channel_factors = vec4(1.0);

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
		COLOR = textureLod(tex, vec3(UV, layer), 0.0);
		COLOR = filter_preview_colors(COLOR, u_channel_factors);
	}
)";

void Texture3DEditor::_texture_rect_draw()
{
	texture_rect->draw_rect(Rect2(Point2(), texture_rect->get_size()), Color(1, 1, 1, 1));
}

void Texture3DEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_RESIZED: {
		_texture_rect_update_area();
	} break;

	case NOTIFICATION_DRAW: {
		Ref<Texture2D> checkerboard = get_editor_theme_icon(SNAME("Checkerboard"));
		draw_texture_rect(checkerboard.ptr(), texture_rect->get_rect(), true);
		_draw_outline();
	} break;
	}
}

void Texture3DEditor::_draw_outline()
{
	const float outline_width = Math::round(EDSCALE);
	const Rect2 outline_rect = texture_rect->get_rect().grow(outline_width * 0.5);
	draw_rect(outline_rect, theme_cache.outline_color, false, outline_width);
}

void Texture3DEditor::_texture_rect_update_area()
{
	Size2 size = get_size();
	int tex_width = texture->get_width() * size.height / texture->get_height();
	int tex_height = size.height;

	if (tex_width > size.width) {
		tex_width = size.width;
		tex_height = texture->get_height() * tex_width / texture->get_width();
	}

	// Prevent the texture from being unpreviewable after the rescale, so that we can still see
	// something
	if (tex_height <= 0) {
		tex_height = 1;
	}
	if (tex_width <= 0) {
		tex_width = 1;
	}

	int ofs_x = (size.width - tex_width) / 2;
	int ofs_y = (size.height - tex_height) / 2;

	texture_rect->set_position(Vector2(ofs_x, ofs_y - Math::round(EDSCALE)));
	texture_rect->set_size(Vector2(tex_width, tex_height));
}

void Texture3DEditor::on_selected_channels_changed() { _update_material(false); }

void Texture3DEditor::init_shaders()
{
	texture_shader.instantiate();
	texture_shader->set_code(texture_3d_shader);
}

void Texture3DEditor::finish_shaders() { texture_shader.unref(); }



void Texture3DEditor::_update_material(bool) {}
