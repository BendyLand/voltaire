/**************************************************************************/
/*  drawable_texture_2d.cpp                                               */
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

#include "drawable_texture_2d.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_server.h"

DrawableTexture2D::DrawableTexture2D()
{
	default_material = RS::get_singleton()->texture_drawable_get_default_material();
}

DrawableTexture2D::~DrawableTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RenderingServer::get_singleton()->free_rid(texture);
	}
}

// Initialize Texture Resource with a call to rendering server. Overwrite existing.
void DrawableTexture2D::_initialize()
{
	if (texture.is_valid()) {
		RID new_texture = RS::get_singleton()->texture_drawable_create(
			width, height, (RSE::TextureDrawableFormat)format, base_color, mipmaps);
		RS::get_singleton()->texture_replace(texture, new_texture);
	}
	else {
		texture = RS::get_singleton()->texture_drawable_create(
			width, height, (RSE::TextureDrawableFormat)format, base_color, mipmaps);
	}
}



int DrawableTexture2D::get_width() const { return width; }

int DrawableTexture2D::get_height() const { return height; }



DrawableTexture2D::DrawableFormat DrawableTexture2D::get_drawable_format() const { return format; }

Image::Format DrawableTexture2D::get_format() const
{
	switch (format) {
	case DRAWABLE_FORMAT_RGBA8:
		return Image::FORMAT_RGBA8;
	case DRAWABLE_FORMAT_RGBA8_SRGB:
		return Image::FORMAT_RGBA8;
	case DRAWABLE_FORMAT_RGBAH:
		return Image::FORMAT_RGBAH;
	case DRAWABLE_FORMAT_RGBAF:
		return Image::FORMAT_RGBAF;
	default:
		return Image::FORMAT_RGBA8;
	}
}



bool DrawableTexture2D::get_use_mipmaps() const { return mipmaps; }

RID DrawableTexture2D::get_rid() const
{
	if (texture.is_null()) {
		// We are in trouble, create something temporary.
		// 4, 4, false, Image::FORMAT_RGBA8
		texture = RenderingServer::get_singleton()->texture_2d_placeholder_create();
	}
	return texture;
}

void DrawableTexture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if ((width | height) == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(width, height)), texture, false, p_modulate, p_transpose);
}

void DrawableTexture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if ((width | height) == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void DrawableTexture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if ((width | height) == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

Ref<Image> DrawableTexture2D::get_image() const
{
	if (texture.is_valid()) {
		return RS::get_singleton()->texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

void DrawableTexture2D::generate_mipmaps()
{
	if (texture.is_valid()) {
		RS::get_singleton()->texture_drawable_generate_mipmaps(texture);
	}
}


