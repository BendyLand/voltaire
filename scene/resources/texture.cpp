/**************************************************************************/
/*  texture.cpp                                                           */
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

#include "core/object/class_db.h"
#include "scene/resources/placeholder_textures.h"
#include "servers/rendering/rendering_server.h"
#include "texture.h"

Size2 Texture2D::get_size() const { return Size2(get_width(), get_height()); }

void Texture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, get_size()), get_rid(), false, p_modulate, p_transpose);
}

void Texture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		p_canvas_item, p_rect, get_rid(), p_tile, p_modulate, p_transpose);
}

void Texture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, get_rid(), p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool Texture2D::get_rect_region(
	const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const
{
	r_rect = p_rect;
	r_src_rect = p_src_rect;
	return true;
}

Ref<Resource> Texture2D::create_placeholder() const
{
	Ref<PlaceholderTexture2D> placeholder;
	placeholder.instantiate();
	placeholder->set_size(get_size());
	return placeholder;
}

void Texture2D::_bind_methods() {}

Texture2D::Texture2D() {}

Array Texture3D::_get_datai() const
{
	Vector<Ref<Image>> data = get_data();

	Array ret;
	ret.resize(data.size());
	for (int i = 0; i < data.size(); i++) {
		ret[i] = data[i];
	}
	return ret;
}

Image::Format Texture3D::get_format() const
{
	Image::Format ret = Image::FORMAT_MAX;
	return ret;
}

Vector<Ref<Image>> Texture3D::get_data() const
{
	Array ret;
	Vector<Ref<Image>> data;
	data.resize(ret.size());
	for (int i = 0; i < data.size(); i++) {
		data.write[i] = ret[i];
	}
	return data;
}

void Texture3D::_bind_methods() {}

Ref<Resource> Texture3D::create_placeholder() const
{
	Ref<PlaceholderTexture3D> placeholder;
	placeholder.instantiate();
	placeholder->set_size(Vector3i(get_width(), get_height(), get_depth()));
	return placeholder;
}

Image::Format TextureLayered::get_format() const
{
	Image::Format ret = Image::FORMAT_MAX;
	return ret;
}

TextureLayered::LayeredType TextureLayered::get_layered_type() const
{
	uint32_t ret = LAYERED_TYPE_2D_ARRAY;
	return (LayeredType)ret;
}

void TextureLayered::_bind_methods() {}

Image::Format Texture2D::get_format() const { return Image::FORMAT_L8; }

int Texture2D::get_mipmap_count() const { return 0; }

bool Texture2D::has_mipmaps() const { return false; }

bool Texture2D::is_pixel_opaque(int p_x, int p_y) const { return true; }

bool Texture2D::has_alpha() const { return false; }

Ref<Image> Texture2D::get_image() const { return Ref<Image>(); }

int Texture2D::get_width() const { return 0; }

int Texture2D::get_height() const { return 0; }

int Texture3D::get_width() const { return 0; }

int Texture3D::get_height() const { return 0; }

int Texture3D::get_depth() const { return 0; }

bool Texture3D::has_mipmaps() const { return false; }

int TextureLayered::get_width() const { return 0; }

int TextureLayered::get_height() const { return 0; }

int TextureLayered::get_layers() const { return 0; }

bool TextureLayered::has_mipmaps() const { return false; }

Ref<Image> TextureLayered::get_layer_data(int p_layer) const { return Ref<Image>(); }


