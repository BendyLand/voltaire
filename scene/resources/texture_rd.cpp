/**************************************************************************/
/*  texture_rd.cpp                                                        */
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

#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server.h"
#include "texture_rd.h"

int Texture2DRD::get_width() const { return size.width; }

int Texture2DRD::get_height() const { return size.height; }

RID Texture2DRD::get_rid() const
{
	if (texture_rid.is_null()) {
		// We are in trouble, create something temporary.
		texture_rid = RenderingServer::get_singleton()->texture_2d_placeholder_create();
	}

	return texture_rid;
}

bool Texture2DRD::has_alpha() const { return false; }

Ref<Image> Texture2DRD::get_image() const
{
	ERR_FAIL_NULL_V(RS::get_singleton(), Ref<Image>());
	if (texture_rid.is_valid()) {
		return RS::get_singleton()->texture_2d_get(texture_rid);
	}
	else {
		return Ref<Image>();
	}
}

RID Texture2DRD::get_texture_rd_rid() const { return texture_rd_rid; }

Texture2DRD::Texture2DRD() { size = Size2i(); }

Texture2DRD::~Texture2DRD()
{
	if (texture_rid.is_valid()) {
		ERR_FAIL_NULL(RS::get_singleton());
		RS::get_singleton()->free_rid(texture_rid);
		texture_rid = RID();
	}
}

TextureLayered::LayeredType TextureLayeredRD::get_layered_type() const { return layer_type; }

Image::Format TextureLayeredRD::get_format() const { return image_format; }

int TextureLayeredRD::get_width() const { return size.width; }

int TextureLayeredRD::get_height() const { return size.height; }

int TextureLayeredRD::get_layers() const { return (int)layers; }

bool TextureLayeredRD::has_mipmaps() const { return mipmaps > 1; }

RID TextureLayeredRD::get_rid() const
{
	if (texture_rid.is_null()) {
		// We are in trouble, create something temporary.
		texture_rid = RenderingServer::get_singleton()->texture_2d_placeholder_create();
	}

	return texture_rid;
}

Ref<Image> TextureLayeredRD::get_layer_data(int p_layer) const
{
	ERR_FAIL_INDEX_V(p_layer, (int)layers, Ref<Image>());
	return RS::get_singleton()->texture_2d_layer_get(texture_rid, p_layer);
}

RID TextureLayeredRD::get_texture_rd_rid() const { return texture_rd_rid; }

TextureLayeredRD::TextureLayeredRD(LayeredType p_layer_type)
{
	layer_type = p_layer_type;
	size = Size2i();
	image_format = Image::FORMAT_MAX;
	layers = 0;
	mipmaps = 0;
}

TextureLayeredRD::~TextureLayeredRD()
{
	if (texture_rid.is_valid()) {
		ERR_FAIL_NULL(RS::get_singleton());
		RS::get_singleton()->free_rid(texture_rid);
		texture_rid = RID();
	}
}

Image::Format Texture3DRD::get_format() const { return image_format; }

int Texture3DRD::get_width() const { return size.x; }

int Texture3DRD::get_height() const { return size.y; }

int Texture3DRD::get_depth() const { return size.z; }

bool Texture3DRD::has_mipmaps() const { return mipmaps > 1; }

RID Texture3DRD::get_rid() const
{
	if (texture_rid.is_null()) {
		// We are in trouble, create something temporary.
		texture_rid = RenderingServer::get_singleton()->texture_2d_placeholder_create();
	}

	return texture_rid;
}

RID Texture3DRD::get_texture_rd_rid() const { return texture_rd_rid; }

Texture3DRD::Texture3DRD()
{
	image_format = Image::FORMAT_MAX;
	size = Vector3i();
	mipmaps = 0;
}

Texture3DRD::~Texture3DRD()
{
	if (texture_rid.is_valid()) {
		ERR_FAIL_NULL(RS::get_singleton());
		RS::get_singleton()->free_rid(texture_rid);
		texture_rid = RID();
	}
}


