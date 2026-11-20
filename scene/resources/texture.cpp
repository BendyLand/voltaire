/**************************************************************************/
/*  texture.cpp                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "texture.h"

#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

Size2 Texture2D::get_size() const { return Size2(get_width(), get_height()); }

RID Texture2D::get_rid() const
{
	if (is_placeholder) {
		if (placeholder_rid.is_null() && Renderer::is_initialized()) {
			placeholder_rid = Renderer::texture_2d_placeholder_create();
		}
		return placeholder_rid;
	}
	return RID();
}

void Texture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, get_size()), get_rid(), false, p_modulate, p_transpose);
}

void Texture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, get_rid(), p_tile, p_modulate, p_transpose);
}

void Texture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, get_rid(), p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool Texture2D::get_rect_region(
	const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const
{
	r_rect = p_rect;
	r_src_rect = p_src_rect;
	return true;
}

Ref<Texture2D> Texture2D::create_placeholder_2d(Size2 p_size)
{
	Ref<Texture2D> placeholder;
	placeholder.instantiate();
	placeholder->is_placeholder = true;
	placeholder->placeholder_size = p_size;
	return placeholder;
}

Ref<Resource> Texture2D::create_placeholder() const
{
	return create_placeholder_2d(get_size());
}

Texture2D::Texture2D() {}

Texture2D::~Texture2D()
{
	if (placeholder_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(placeholder_rid);
	}
}

Image::Format Texture2D::get_format() const { return Image::FORMAT_L8; }

int Texture2D::get_mipmap_count() const { return 0; }

bool Texture2D::has_mipmaps() const { return false; }

bool Texture2D::is_pixel_opaque(int p_x, int p_y) const { return true; }

bool Texture2D::has_alpha() const { return false; }

Ref<Image> Texture2D::get_image() const { return Ref<Image>(); }

int Texture2D::get_width() const { return is_placeholder ? int(placeholder_size.width) : 0; }

int Texture2D::get_height() const { return is_placeholder ? int(placeholder_size.height) : 0; }

///////////////////////////////////////////////

void CanvasTexture::set_diffuse_texture(const Ref<Texture2D>& p_diffuse)
{
	ERR_FAIL_COND(p_diffuse == this);
	if (diffuse_texture == p_diffuse) {
		return;
	}
	diffuse_texture = p_diffuse;
	RID tex_rid = diffuse_texture.is_valid() ? diffuse_texture->get_rid() : RID();
	Renderer::canvas_texture_set_channel(
		canvas_texture, RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE, tex_rid);
	emit_changed();
}

Ref<Texture2D> CanvasTexture::get_diffuse_texture() const { return diffuse_texture; }

void CanvasTexture::set_normal_texture(const Ref<Texture2D>& p_normal)
{
	ERR_FAIL_COND(p_normal == this);
	if (normal_texture == p_normal) {
		return;
	}
	normal_texture = p_normal;
	RID tex_rid = normal_texture.is_valid() ? normal_texture->get_rid() : RID();
	Renderer::canvas_texture_set_channel(
		canvas_texture, RSE::CANVAS_TEXTURE_CHANNEL_NORMAL, tex_rid);
	emit_changed();
}

Ref<Texture2D> CanvasTexture::get_normal_texture() const { return normal_texture; }

void CanvasTexture::set_specular_texture(const Ref<Texture2D>& p_specular)
{
	ERR_FAIL_COND(p_specular == this);
	if (specular_texture == p_specular) {
		return;
	}
	specular_texture = p_specular;
	RID tex_rid = specular_texture.is_valid() ? specular_texture->get_rid() : RID();
	Renderer::canvas_texture_set_channel(
		canvas_texture, RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR, tex_rid);
	emit_changed();
}

Ref<Texture2D> CanvasTexture::get_specular_texture() const { return specular_texture; }

void CanvasTexture::set_specular_color(const Color& p_color)
{
	if (specular == p_color) {
		return;
	}
	specular = p_color;
	Renderer::canvas_texture_set_shading_parameters(canvas_texture, specular, shininess);
	emit_changed();
}

Color CanvasTexture::get_specular_color() const { return specular; }

void CanvasTexture::set_specular_shininess(real_t p_shininess)
{
	if (shininess == p_shininess) {
		return;
	}
	shininess = p_shininess;
	Renderer::canvas_texture_set_shading_parameters(canvas_texture, specular, shininess);
	emit_changed();
}

real_t CanvasTexture::get_specular_shininess() const { return shininess; }

void CanvasTexture::set_texture_filter(TextureFilter p_filter)
{
	if (texture_filter == p_filter) {
		return;
	}
	texture_filter = p_filter;
	Renderer::canvas_texture_set_texture_filter(
		canvas_texture, RSE::CanvasItemTextureFilter(p_filter));
	emit_changed();
}

CanvasTexture::TextureFilter CanvasTexture::get_texture_filter() const { return texture_filter; }

void CanvasTexture::set_texture_repeat(TextureRepeat p_repeat)
{
	if (texture_repeat == p_repeat) {
		return;
	}
	texture_repeat = p_repeat;
	Renderer::canvas_texture_set_texture_repeat(
		canvas_texture, RSE::CanvasItemTextureRepeat(p_repeat));
	emit_changed();
}

CanvasTexture::TextureRepeat CanvasTexture::get_texture_repeat() const { return texture_repeat; }

void CanvasTexture::set_mesh(const Ref<Mesh>& p_mesh)
{
	if (mesh == p_mesh) {
		return;
	}
	mesh = p_mesh;
	emit_changed();
}

Ref<Mesh> CanvasTexture::get_mesh() const { return mesh; }

void CanvasTexture::set_image_size(const Size2& p_size)
{
	Size2i new_size = p_size;
	if (image_size == new_size) {
		return;
	}
	image_size = new_size;
	emit_changed();
}

Size2 CanvasTexture::get_image_size() const { return image_size; }

void CanvasTexture::set_base_texture(const Ref<Texture2D>& p_texture)
{
	ERR_FAIL_COND(p_texture == this);
	if (base_texture == p_texture) {
		return;
	}
	base_texture = p_texture;
	emit_changed();
}

Ref<Texture2D> CanvasTexture::get_base_texture() const { return base_texture; }

int CanvasTexture::get_width() const
{
	if (mesh.is_valid()) {
		return image_size.width;
	}
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_width();
	}
	return image_size.width > 0 ? image_size.width : 1;
}

int CanvasTexture::get_height() const
{
	if (mesh.is_valid()) {
		return image_size.height;
	}
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_height();
	}
	return image_size.height > 0 ? image_size.height : 1;
}

bool CanvasTexture::is_pixel_opaque(int p_x, int p_y) const
{
	if (mesh.is_valid()) {
		return true;
	}
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->is_pixel_opaque(p_x, p_y);
	}
	return false;
}

bool CanvasTexture::has_alpha() const
{
	if (mesh.is_valid()) {
		return false;
	}
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->has_alpha();
	}
	return false;
}

Ref<Image> CanvasTexture::get_image() const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_image();
	}
	if (base_texture.is_valid()) {
		return base_texture->get_image();
	}
	return Ref<Image>();
}

RID CanvasTexture::get_rid() const
{
	if (mesh.is_valid()) {
		return RID();
	}
	return canvas_texture;
}

void CanvasTexture::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if (mesh.is_valid()) {
		const Ref<Texture2D>& tex = base_texture.is_valid() ? base_texture : diffuse_texture;
		if (tex.is_null()) {
			return;
		}
		Transform2D xform;
		xform.set_origin(p_pos);
		if (p_transpose) {
			SWAP(xform.columns[0][1], xform.columns[1][0]);
			SWAP(xform.columns[0][0], xform.columns[1][1]);
		}
		Renderer::canvas_item_add_mesh(
			p_canvas_item, mesh->get_rid(), xform, p_modulate, tex->get_rid());
		return;
	}
	Texture2D::draw(p_canvas_item, p_pos, p_modulate, p_transpose);
}

void CanvasTexture::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if (mesh.is_valid()) {
		const Ref<Texture2D>& tex = base_texture.is_valid() ? base_texture : diffuse_texture;
		if (tex.is_null()) {
			return;
		}
		Transform2D xform;
		Vector2 origin = p_rect.position;
		if (p_rect.size.x < 0) {
			origin.x += image_size.x;
		}
		if (p_rect.size.y < 0) {
			origin.y += image_size.y;
		}
		xform.set_origin(origin);
		xform.set_scale(p_rect.size / Size2(image_size));

		if (p_transpose) {
			SWAP(xform.columns[0][1], xform.columns[1][0]);
			SWAP(xform.columns[0][0], xform.columns[1][1]);
		}
		Renderer::canvas_item_add_mesh(
			p_canvas_item, mesh->get_rid(), xform, p_modulate, tex->get_rid());
		return;
	}
	Texture2D::draw_rect(p_canvas_item, p_rect, p_tile, p_modulate, p_transpose);
}

void CanvasTexture::draw_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if (mesh.is_valid()) {
		const Ref<Texture2D>& tex = base_texture.is_valid() ? base_texture : diffuse_texture;
		if (tex.is_null()) {
			return;
		}
		Transform2D xform;
		Vector2 origin = p_rect.position;
		if (p_rect.size.x < 0) {
			origin.x += image_size.x;
		}
		if (p_rect.size.y < 0) {
			origin.y += image_size.y;
		}
		xform.set_origin(origin);
		xform.set_scale(p_rect.size / Size2(image_size));

		if (p_transpose) {
			SWAP(xform.columns[0][1], xform.columns[1][0]);
			SWAP(xform.columns[0][0], xform.columns[1][1]);
		}
		Renderer::canvas_item_add_mesh(
			p_canvas_item, mesh->get_rid(), xform, p_modulate, tex->get_rid());
		return;
	}
	Texture2D::draw_rect_region(p_canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool CanvasTexture::get_rect_region(
	const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const
{
	r_rect = p_rect;
	r_src_rect = p_src_rect;
	return true;
}

CanvasTexture::CanvasTexture()
{
	if (Renderer::is_initialized()) {
		canvas_texture = Renderer::canvas_texture_create();
	}
}

CanvasTexture::~CanvasTexture()
{
	if (canvas_texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(canvas_texture);
	}
}

///////////////////////////////////////////////

Image::Format TextureLayered::get_format() const
{
	return is_placeholder ? Image::FORMAT_RGB8 : Image::FORMAT_MAX;
}

TextureLayered::LayeredType TextureLayered::get_layered_type() const
{
	return is_placeholder ? placeholder_layered_type : LAYERED_TYPE_2D_ARRAY;
}

int TextureLayered::get_width() const { return is_placeholder ? placeholder_size.x : 0; }

int TextureLayered::get_height() const { return is_placeholder ? placeholder_size.y : 0; }

int TextureLayered::get_layers() const { return is_placeholder ? placeholder_layers : 0; }

bool TextureLayered::has_mipmaps() const { return false; }

Ref<Image> TextureLayered::get_layer_data(int p_layer) const { return Ref<Image>(); }

RID TextureLayered::get_rid() const
{
	if (is_placeholder) {
		if (placeholder_rid.is_null() && Renderer::is_initialized()) {
			placeholder_rid = Renderer::texture_2d_layered_placeholder_create(
				static_cast<RSE::TextureLayeredType>(placeholder_layered_type));
		}
		return placeholder_rid;
	}
	return RID();
}

Ref<TextureLayered> TextureLayered::create_placeholder_layered(
	LayeredType p_type, const Size2i& p_size, int p_layers)
{
	Ref<TextureLayered> placeholder;
	placeholder.instantiate();
	placeholder->is_placeholder = true;
	placeholder->placeholder_layered_type = p_type;
	placeholder->placeholder_size = p_size;
	placeholder->placeholder_layers = p_layers;
	return placeholder;
}

Ref<Resource> TextureLayered::create_placeholder() const
{
	return create_placeholder_layered(
		get_layered_type(), Size2i(get_width(), get_height()), get_layers());
}

TextureLayered::~TextureLayered()
{
	if (placeholder_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(placeholder_rid);
	}
}

///////////////////////////////////////////////

Image::Format Texture3D::get_format() const
{
	return is_placeholder ? Image::FORMAT_RGB8 : Image::FORMAT_MAX;
}

int Texture3D::get_width() const { return is_placeholder ? placeholder_size.x : 0; }

int Texture3D::get_height() const { return is_placeholder ? placeholder_size.y : 0; }

int Texture3D::get_depth() const { return is_placeholder ? placeholder_size.z : 0; }

bool Texture3D::has_mipmaps() const { return false; }

Vector<Ref<Image>> Texture3D::get_data() const { return Vector<Ref<Image>>(); }

RID Texture3D::get_rid() const
{
	if (is_placeholder) {
		if (placeholder_rid.is_null() && Renderer::is_initialized()) {
			placeholder_rid = Renderer::texture_3d_placeholder_create();
		}
		return placeholder_rid;
	}
	return RID();
}

Ref<Texture3D> Texture3D::create_placeholder_3d(const Vector3i& p_size)
{
	Ref<Texture3D> placeholder;
	placeholder.instantiate();
	placeholder->is_placeholder = true;
	placeholder->placeholder_size = p_size;
	return placeholder;
}

Ref<Resource> Texture3D::create_placeholder() const
{
	return create_placeholder_3d(Vector3i(get_width(), get_height(), get_depth()));
}

Texture3D::~Texture3D()
{
	if (placeholder_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(placeholder_rid);
	}
}
