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

// --- atlas_texture.cpp ---

#include "atlas_texture.h"

#include "core/os/os.h"
#include "servers/rendering/renderer.h"

AtlasTexture::AnimData* AtlasTexture::_ensure_anim()
{
	if (!anim) {
		anim = memnew(AnimData);
		anim->frames.resize(MAX_FRAMES);
		if (Renderer::is_initialized()) {
			anim->proxy_ph = Renderer::texture_2d_placeholder_create();
			anim->proxy = Renderer::texture_proxy_create(anim->proxy_ph);
		}
	}
	return anim;
}

void AtlasTexture::_update_proxy() const
{
	if (!anim) {
		return;
	}

	RWLockWrite w(anim->rw_lock);

	if (anim->proxy.is_null() && Renderer::is_initialized()) {
		anim->proxy_ph = Renderer::texture_2d_placeholder_create();
		anim->proxy = Renderer::texture_proxy_create(anim->proxy_ph);
	}

	float delta;
	if (anim->prev_ticks == 0) {
		delta = 0;
		anim->prev_ticks = OS::get_ticks_usec();
	}
	else {
		uint64_t ticks = OS::get_ticks_usec();
		delta = float(double(ticks - anim->prev_ticks) / 1000000.0);
		anim->prev_ticks = ticks;
	}

	anim->time += delta;

	float speed = anim->speed_scale == 0 ? 0 : std::abs(1.0f / anim->speed_scale);

	int iter_max = anim->frame_count;
	while (iter_max && !anim->pause) {
		float frame_limit = anim->frames[anim->current_frame].duration * speed;

		if (anim->time > frame_limit) {
			if (anim->speed_scale > 0.0f) {
				anim->current_frame++;
			}
			else {
				anim->current_frame--;
			}
			if (anim->current_frame >= anim->frame_count) {
				if (anim->one_shot) {
					anim->current_frame = anim->frame_count - 1;
				}
				else {
					anim->current_frame = 0;
				}
			}
			else if (anim->current_frame < 0) {
				if (anim->one_shot) {
					anim->current_frame = 0;
				}
				else {
					anim->current_frame = anim->frame_count - 1;
				}
			}
			anim->time -= frame_limit;
		}
		else {
			break;
		}
		iter_max--;
	}

	if (anim->proxy.is_valid() && anim->frames[anim->current_frame].texture.is_valid()) {
		Renderer::texture_proxy_update(
			anim->proxy, anim->frames[anim->current_frame].texture->get_rid());
	}
}

void AtlasTexture::set_frames(int p_frames)
{
	ERR_FAIL_COND(p_frames < 1 || p_frames > MAX_FRAMES);
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->frame_count = p_frames;
	if (ad->current_frame >= ad->frame_count) {
		ad->current_frame = ad->frame_count - 1;
	}
}

int AtlasTexture::get_frames() const { return anim ? anim->frame_count : 0; }

void AtlasTexture::set_current_frame(int p_frame)
{
	AnimData* ad = _ensure_anim();
	ERR_FAIL_COND(p_frame < 0 || p_frame >= ad->frame_count);
	RWLockWrite w(ad->rw_lock);
	ad->current_frame = p_frame;
	ad->time = 0;
}

int AtlasTexture::get_current_frame() const { return anim ? anim->current_frame : 0; }

void AtlasTexture::set_pause(bool p_pause)
{
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->pause = p_pause;
}

bool AtlasTexture::get_pause() const { return anim ? anim->pause : false; }

void AtlasTexture::set_one_shot(bool p_one_shot)
{
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->one_shot = p_one_shot;
}

bool AtlasTexture::get_one_shot() const { return anim ? anim->one_shot : false; }

void AtlasTexture::set_frame_texture(int p_frame, const Ref<Texture2D>& p_texture)
{
	ERR_FAIL_COND(p_texture == this);
	ERR_FAIL_INDEX(p_frame, MAX_FRAMES);
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->frames[p_frame].texture = p_texture;
}

Ref<Texture2D> AtlasTexture::get_frame_texture(int p_frame) const
{
	ERR_FAIL_INDEX_V(p_frame, MAX_FRAMES, Ref<Texture2D>());
	if (!anim) {
		return Ref<Texture2D>();
	}
	RWLockRead r(anim->rw_lock);
	return anim->frames[p_frame].texture;
}

void AtlasTexture::set_frame_duration(int p_frame, float p_duration)
{
	ERR_FAIL_INDEX(p_frame, MAX_FRAMES);
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->frames[p_frame].duration = p_duration;
}

float AtlasTexture::get_frame_duration(int p_frame) const
{
	ERR_FAIL_INDEX_V(p_frame, MAX_FRAMES, 0);
	if (!anim) {
		return 0;
	}
	RWLockRead r(anim->rw_lock);
	return anim->frames[p_frame].duration;
}

void AtlasTexture::set_speed_scale(float p_scale)
{
	ERR_FAIL_COND(p_scale < -1000 || p_scale >= 1000);
	AnimData* ad = _ensure_anim();
	RWLockWrite w(ad->rw_lock);
	ad->speed_scale = p_scale;
}

float AtlasTexture::get_speed_scale() const { return anim ? anim->speed_scale : 1.0f; }

int AtlasTexture::get_width() const
{
	if (anim) {
		RWLockRead r(anim->rw_lock);
		if (anim->frames[anim->current_frame].texture.is_null()) {
			return 1;
		}
		return anim->frames[anim->current_frame].texture->get_width();
	}

	if (rounded_region.size.width == 0) {
		if (atlas.is_valid()) {
			return atlas->get_width();
		}
		return 1;
	}
	else {
		return rounded_region.size.width + margin.size.width;
	}
}

int AtlasTexture::get_height() const
{
	if (anim) {
		RWLockRead r(anim->rw_lock);
		if (anim->frames[anim->current_frame].texture.is_null()) {
			return 1;
		}
		return anim->frames[anim->current_frame].texture->get_height();
	}

	if (rounded_region.size.height == 0) {
		if (atlas.is_valid()) {
			return atlas->get_height();
		}
		return 1;
	}
	else {
		return rounded_region.size.height + margin.size.height;
	}
}

RID AtlasTexture::get_rid() const
{
	if (anim) {
		_update_proxy();
		return anim->proxy;
	}

	if (atlas.is_valid()) {
		return atlas->get_rid();
	}

	return RID();
}

bool AtlasTexture::has_alpha() const
{
	if (anim) {
		RWLockRead r(anim->rw_lock);
		if (anim->frames[anim->current_frame].texture.is_null()) {
			return false;
		}
		return anim->frames[anim->current_frame].texture->has_alpha();
	}

	if (atlas.is_valid()) {
		return atlas->has_alpha();
	}

	return false;
}

void AtlasTexture::set_atlas(const Ref<Texture2D>& p_atlas)
{
	ERR_FAIL_COND(p_atlas == this);
	if (atlas == p_atlas) {
		return;
	}
	atlas = p_atlas;
	emit_changed();
}

Ref<Texture2D> AtlasTexture::get_atlas() const { return atlas; }

void AtlasTexture::set_region(const Rect2& p_region)
{
	if (region == p_region) {
		return;
	}
	region = p_region;
	rounded_region = Rect2(p_region.position, p_region.size.floor());
	emit_changed();
}

Rect2 AtlasTexture::get_region() const { return region; }

void AtlasTexture::set_margin(const Rect2& p_margin)
{
	if (margin == p_margin) {
		return;
	}
	margin = p_margin;
	emit_changed();
}

Rect2 AtlasTexture::get_margin() const { return margin; }

void AtlasTexture::set_filter_clip(const bool p_enable)
{
	filter_clip = p_enable;
	emit_changed();
}

bool AtlasTexture::has_filter_clip() const { return filter_clip; }

Rect2 AtlasTexture::_get_region_rect() const
{
	Rect2 rc = rounded_region;
	if (atlas.is_valid()) {
		if (rc.size.width == 0) {
			rc.size.width = atlas->get_width();
		}
		if (rc.size.height == 0) {
			rc.size.height = atlas->get_height();
		}
	}
	return rc;
}

void AtlasTexture::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if (anim) {
		Texture2D::draw(p_canvas_item, p_pos, p_modulate, p_transpose);
		return;
	}

	if (atlas.is_null()) {
		return;
	}
	const Rect2 rc = _get_region_rect();
	atlas->draw_rect_region(p_canvas_item, Rect2(p_pos + margin.position, rc.size), rc, p_modulate,
		p_transpose, filter_clip);
}

void AtlasTexture::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if (anim) {
		Texture2D::draw_rect(p_canvas_item, p_rect, p_tile, p_modulate, p_transpose);
		return;
	}

	if (atlas.is_null()) {
		return;
	}

	Rect2 src_rect = Rect2(0, 0, get_width(), get_height());

	Rect2 dr;
	Rect2 src_c;
	if (get_rect_region(p_rect, src_rect, dr, src_c)) {
		atlas->draw_rect_region(p_canvas_item, dr, src_c, p_modulate, p_transpose, filter_clip);
	}
}

void AtlasTexture::draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if (anim) {
		Texture2D::draw_rect_region(
			p_canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
		return;
	}

	if (atlas.is_null()) {
		return;
	}

	Rect2 dr;
	Rect2 src_c;
	if (get_rect_region(p_rect, p_src_rect, dr, src_c)) {
		atlas->draw_rect_region(p_canvas_item, dr, src_c, p_modulate, p_transpose, filter_clip);
	}
}

bool AtlasTexture::get_rect_region(
	const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const
{
	if (anim) {
		return Texture2D::get_rect_region(p_rect, p_src_rect, r_rect, r_src_rect);
	}

	if (atlas.is_null()) {
		return false;
	}

	Rect2 src = p_src_rect;
	if (src.size == Size2()) {
		src.size = rounded_region.size;
	}
	if (src.size == Size2() && atlas.is_valid()) {
		src.size = atlas->get_size();
	}
	Vector2 scale = p_rect.size / src.size;

	src.position += (rounded_region.position - margin.position);
	Rect2 src_clipped = _get_region_rect().intersection(src);
	if (src_clipped.size == Size2()) {
		return false;
	}

	Vector2 ofs = (src_clipped.position - src.position);
	if (scale.x < 0) {
		ofs.x += (src_clipped.size.x - src.size.x);
	}
	if (scale.y < 0) {
		ofs.y += (src_clipped.size.y - src.size.y);
	}

	r_rect = Rect2(p_rect.position + ofs * scale, src_clipped.size * scale);
	r_src_rect = src_clipped;
	return true;
}

bool AtlasTexture::is_pixel_opaque(int p_x, int p_y) const
{
	if (anim) {
		RWLockRead r(anim->rw_lock);
		if (anim->frames[anim->current_frame].texture.is_valid()) {
			return anim->frames[anim->current_frame].texture->is_pixel_opaque(p_x, p_y);
		}
		return true;
	}

	if (atlas.is_null()) {
		return true;
	}

	int x = p_x + rounded_region.position.x - margin.position.x;
	int y = p_y + rounded_region.position.y - margin.position.y;

	// Margin edge may outside of atlas.
	if (x < 0 || x >= atlas->get_width()) {
		return false;
	}
	if (y < 0 || y >= atlas->get_height()) {
		return false;
	}

	return atlas->is_pixel_opaque(x, y);
}

Ref<Image> AtlasTexture::get_image() const
{
	if (anim) {
		RWLockRead r(anim->rw_lock);
		if (anim->frames[anim->current_frame].texture.is_null()) {
			return Ref<Image>();
		}
		return anim->frames[anim->current_frame].texture->get_image();
	}

	if (atlas.is_null()) {
		return Ref<Image>();
	}

	const Ref<Image>& atlas_image = atlas->get_image();
	if (atlas_image.is_null()) {
		return Ref<Image>();
	}

	return atlas_image->get_region(_get_region_rect());
}

AtlasTexture::~AtlasTexture()
{
	if (anim) {
		if (Renderer::data) {
			if (anim->proxy.is_valid()) {
				Renderer::free_rid(anim->proxy);
			}
			if (anim->proxy_ph.is_valid()) {
				Renderer::free_rid(anim->proxy_ph);
			}
		}
		memdelete(anim);
	}
}

// --- image_texture.cpp ---

#include "image_texture.h"

#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "modules/modules_enabled.gen.h" // For svg.
#include "scene/resources/bit_map.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#ifdef MODULE_SVG_ENABLED
#include "modules/svg/image_loader_svg.h"
#endif

Mutex ImageTexture::svg_mutex;
HashMap<double, ImageTexture::ScalingLevel> ImageTexture::scaling_levels;

void ImageTexture::reference_scaling_level(double p_scale)
{
	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	double scale = double(oversampling) / 64.0;

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	if (sl) {
		sl->refcount++;
	}
	else {
		ScalingLevel new_sl;
		scaling_levels.insert(scale, new_sl);
	}
}

void ImageTexture::unreference_scaling_level(double p_scale)
{
	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	double scale = double(oversampling) / 64.0;

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	if (sl) {
		sl->refcount--;
		if (sl->refcount == 0) {
			for (ImageTexture* tx : sl->textures) {
				tx->_svg_remove_scale(scale);
			}
			sl->textures.clear();
			scaling_levels.erase(scale);
		}
	}
}

ImageTexture::SVGData* ImageTexture::_ensure_svg_data()
{
	if (!svg_data) {
		svg_data = memnew(SVGData);
	}
	return svg_data;
}

void ImageTexture::_svg_remove_scale(double p_scale)
{
	if (!svg_data || Math::is_equal_approx(p_scale, 1.0)) {
		return;
	}

	RID* rid = svg_data->texture_cache.getptr(p_scale);
	if (rid) {
		if (rid->is_valid()) {
			Renderer::free_rid(*rid);
		}
		svg_data->texture_cache.erase(p_scale);
	}
}

RID ImageTexture::_svg_ensure_scale(double p_scale) const
{
	if (!svg_data) {
		return RID();
	}

	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		if (svg_data->base_texture.is_null()) {
			svg_data->base_texture = _svg_load_at_scale(p_scale, true);
		}
		return svg_data->base_texture;
	}
	double scale = double(oversampling) / 64.0;

	RID* rid = svg_data->texture_cache.getptr(scale);
	if (rid) {
		return *rid;
	}

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	ERR_FAIL_NULL_V_MSG(sl, RID(), "Invalid scaling level");
	sl->textures.insert(const_cast<ImageTexture*>(this));

	RID new_rid = _svg_load_at_scale(scale, false);
	svg_data->texture_cache[scale] = new_rid;
	return new_rid;
}

RID ImageTexture::_svg_load_at_scale(double p_scale, bool p_set_size) const
{
	if (!svg_data) {
		return RID();
	}

	Ref<Image> img;
	img.instantiate();
#ifdef MODULE_SVG_ENABLED
	const bool upsample = !Math::is_equal_approx(
		Math::round(p_scale * svg_data->base_scale), p_scale * svg_data->base_scale);

	Error err = ImageLoaderSVG::create_image_from_string(
		img, svg_data->source, p_scale * svg_data->base_scale, upsample, svg_data->cmap);
	if (err != OK) {
		return RID();
	}
#else
	img = Image::create_empty(Math::round(16 * p_scale * svg_data->base_scale),
		Math::round(16 * p_scale * svg_data->base_scale), false, Image::FORMAT_RGBA8);
#endif
	if (svg_data->saturation != 1.0f) {
		img->adjust_bcs(1.0, 1.0, svg_data->saturation);
	}

	if (svg_data->fix_alpha_border) {
		img->fix_alpha_edges();
	}

	if (svg_data->premult_alpha) {
		img->premultiply_alpha();
	}

	Size2 current_size = svg_data->size;
	if (p_set_size || svg_data->size.is_zero_approx()) {
		svg_data->size.x = img->get_width() / p_scale;
		svg_data->base_size.x = svg_data->size.x;
		if (size_override.x != 0) {
			svg_data->size.x = size_override.x;
		}
		svg_data->size.y = img->get_height() / p_scale;
		svg_data->base_size.y = svg_data->size.y;
		if (size_override.y != 0) {
			svg_data->size.y = size_override.y;
		}
		current_size = svg_data->size;
	}
	if (current_size.is_zero_approx()) {
		current_size.x = img->get_width();
		current_size.y = img->get_height();
	}

	RID rid = Renderer::texture_2d_create(img);
	Renderer::texture_set_size_override(rid, current_size.x, current_size.y);
	return rid;
}

void ImageTexture::_svg_clear()
{
	if (!svg_data) {
		return;
	}
	for (KeyValue<double, RID>& tx : svg_data->texture_cache) {
		if (tx.value.is_valid()) {
			Renderer::free_rid(tx.value);
		}
	}
	svg_data->texture_cache.clear();
	if (svg_data->base_texture.is_valid()) {
		Renderer::free_rid(svg_data->base_texture);
	}
	svg_data->base_texture = RID();
	alpha_cache.unref();
}

void ImageTexture::_svg_destroy()
{
	if (!svg_data) {
		return;
	}
	_svg_clear();
	{
		MutexLock lock(svg_mutex);
		for (KeyValue<double, ScalingLevel>& sl : scaling_levels) {
			sl.value.textures.erase(this);
		}
	}
	memdelete(svg_data);
	svg_data = nullptr;
}

void ImageTexture::_svg_update_texture()
{
	_svg_clear();
	emit_changed();
}

void ImageTexture::set_source(const String& p_source)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->source == p_source) {
		return;
	}
	sd->source = p_source;
	_svg_update_texture();
}

String ImageTexture::get_source() const { return svg_data ? svg_data->source : String(); }

void ImageTexture::set_fix_alpha_border(bool p_enabled)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->fix_alpha_border == p_enabled) {
		return;
	}
	sd->fix_alpha_border = p_enabled;
	_svg_update_texture();
}

bool ImageTexture::get_fix_alpha_border() const
{
	return svg_data ? svg_data->fix_alpha_border : false;
}

void ImageTexture::set_premult_alpha(bool p_enabled)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->premult_alpha == p_enabled) {
		return;
	}
	sd->premult_alpha = p_enabled;
	_svg_update_texture();
}

bool ImageTexture::get_premult_alpha() const { return svg_data ? svg_data->premult_alpha : false; }

void ImageTexture::set_base_scale(float p_scale)
{
	ERR_FAIL_COND(p_scale <= 0.0f);
	SVGData* sd = _ensure_svg_data();
	if (sd->base_scale == p_scale) {
		return;
	}
	sd->base_scale = p_scale;
	_svg_update_texture();
}

float ImageTexture::get_base_scale() const { return svg_data ? svg_data->base_scale : 1.0f; }

void ImageTexture::set_saturation(float p_saturation)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->saturation == p_saturation) {
		return;
	}
	sd->saturation = p_saturation;
	_svg_update_texture();
}

float ImageTexture::get_saturation() const { return svg_data ? svg_data->saturation : 1.0f; }

void ImageTexture::set_color_map(const HashMap<Color, Color>& p_color_map)
{
	SVGData* sd = _ensure_svg_data();
	sd->cmap = p_color_map;
	_svg_update_texture();
}

HashMap<Color, Color> ImageTexture::get_color_map() const
{
	return svg_data ? svg_data->cmap : HashMap<Color, Color>();
}

void ImageTexture::reload_from_file() {}

Ref<ImageTexture> ImageTexture::create_from_image(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_V_MSG(p_image.is_null(), Ref<ImageTexture>(), "Invalid image: null");
	ERR_FAIL_COND_V_MSG(p_image->is_empty(), Ref<ImageTexture>(), "Invalid image: image is empty");

	Ref<ImageTexture> image_texture;
	image_texture.instantiate();
	image_texture->set_image(p_image);
	return image_texture;
}

Ref<ImageTexture> ImageTexture::create_from_string(const String& p_source, float p_scale,
	float p_saturation, const HashMap<Color, Color>& p_color_map)
{
	ERR_FAIL_COND_V_MSG(p_source.is_empty(), Ref<ImageTexture>(), "SVG source is empty.");
	ERR_FAIL_COND_V_MSG(p_scale <= 0.0f, Ref<ImageTexture>(), "Scale must be positive.");

	Ref<ImageTexture> image_texture;
	image_texture.instantiate();
	SVGData* sd = image_texture->_ensure_svg_data();
	sd->source = p_source;
	sd->base_scale = p_scale;
	sd->saturation = p_saturation;
	sd->cmap = p_color_map;
	image_texture->_svg_update_texture();
	return image_texture;
}

Image::Format ImageTexture::get_format() const
{
	if (svg_data) {
		return Image::FORMAT_RGBA8;
	}
	return format;
}

void ImageTexture::update(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_MSG(p_image.is_null(), "Invalid image");
	ERR_FAIL_COND_MSG(texture.is_null(), "Texture is not initialized.");
	ERR_FAIL_COND_MSG(p_image->get_width() != w || p_image->get_height() != h,
		"The new image dimensions must match the texture size.");
	ERR_FAIL_COND_MSG(p_image->get_format() != format,
		"The new image format must match the texture's image format.");
	ERR_FAIL_COND_MSG(mipmaps != p_image->has_mipmaps(),
		"The new image mipmaps configuration must match the texture's image mipmaps configuration");

	Renderer::texture_2d_update(texture, p_image);

	emit_changed();

	alpha_cache.unref();
	image_stored = true;
}

Ref<Image> ImageTexture::get_image() const
{
	if (svg_data) {
		RID rid = _svg_ensure_scale(1.0);
		if (rid.is_valid()) {
			return Renderer::texture_2d_get(rid);
		}
		return Ref<Image>();
	}

	if (image_stored) {
		return Renderer::texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

int ImageTexture::get_width() const
{
	if (svg_data) {
		_svg_ensure_scale(1.0);
		return svg_data->size.x;
	}
	return w;
}

int ImageTexture::get_height() const
{
	if (svg_data) {
		_svg_ensure_scale(1.0);
		return svg_data->size.y;
	}
	return h;
}

RID ImageTexture::get_rid() const
{
	if (svg_data) {
		return _svg_ensure_scale(1.0);
	}

	if (texture.is_null()) {
		// We are in trouble, create something temporary.
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

RID ImageTexture::get_scaled_rid() const
{
	if (svg_data) {
		double scale = TextServer::get_current_drawn_item_oversampling();
		if (scale == 0.0) {
			scale = 1.0;
		}
		return _svg_ensure_scale(scale);
	}
	return get_rid();
}

bool ImageTexture::has_alpha() const
{
	if (svg_data) {
		return true;
	}
	return (format == Image::FORMAT_LA8 || format == Image::FORMAT_RGBA8);
}

void ImageTexture::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_texture_rect(
			p_canvas_item, Rect2(p_pos, svg_data->size), rid, false, p_modulate, p_transpose);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(w, h)), texture, false, p_modulate, p_transpose);
}

void ImageTexture::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if (svg_data) {
		Renderer::canvas_item_add_texture_rect(
			p_canvas_item, p_rect, get_scaled_rid(), p_tile, p_modulate, p_transpose);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void ImageTexture::draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if (svg_data) {
		Renderer::canvas_item_add_texture_rect_region(
			p_canvas_item, p_rect, get_scaled_rid(), p_src_rect, p_modulate, p_transpose, p_clip_uv);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void ImageTexture::draw_msdf_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, int p_outline_size, float p_px_range,
	float p_scale) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_msdf_texture_rect_region(
			p_canvas_item, p_rect, rid, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_msdf_texture_rect_region(p_canvas_item,
		p_rect, texture, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
}

void ImageTexture::draw_lcd_rect_region(
	RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect, const Color& p_modulate) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_lcd_texture_rect_region(
			p_canvas_item, p_rect, rid, p_src_rect, p_modulate);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_lcd_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate);
}

bool ImageTexture::is_pixel_opaque(int p_x, int p_y) const
{
	if (alpha_cache.is_valid()) {
		int aw = int(alpha_cache->get_size().width);
		int ah = int(alpha_cache->get_size().height);
		if (aw == 0 || ah == 0) {
			return true;
		}

		int tw = svg_data ? int(svg_data->size.x) : w;
		int th = svg_data ? int(svg_data->size.y) : h;
		if (tw == 0 || th == 0) {
			return true;
		}

		int x = p_x * aw / tw;
		int y = p_y * ah / th;

		x = CLAMP(x, 0, aw - 1);
		y = CLAMP(y, 0, ah - 1);

		return alpha_cache->get_bit(x, y);
	}

	return true;
}

void ImageTexture::set_size_override(const Size2i& p_size)
{
	if (svg_data) {
		if (size_override == p_size) {
			return;
		}
		size_override = p_size;
		if (size_override.x == 0 || size_override.y == 0) {
			_svg_ensure_scale(1.0);
			svg_data->size = svg_data->base_size;
		}
		if (size_override.x != 0) {
			svg_data->size.x = size_override.x;
		}
		if (size_override.y != 0) {
			svg_data->size.y = size_override.y;
		}
		for (KeyValue<double, RID>& tx : svg_data->texture_cache) {
			if (tx.value.is_valid()) {
				Renderer::texture_set_size_override(tx.value, svg_data->size.x, svg_data->size.y);
			}
		}
		if (svg_data->base_texture.is_valid()) {
			Renderer::texture_set_size_override(
				svg_data->base_texture, svg_data->size.x, svg_data->size.y);
		}
		emit_changed();
		return;
	}

	Size2i s = p_size;
	if (s.x != 0) {
		w = s.x;
	}
	if (s.y != 0) {
		h = s.y;
	}
	Renderer::texture_set_size_override(texture, w, h);
}

void ImageTexture::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTexture::~ImageTexture()
{
	if (svg_data) {
		_svg_destroy();
	}
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void ImageTexture::set_image(const Ref<Image>& p_image)
{
	if (p_image.is_null() || p_image->is_empty()) {
		if (image_stored) {
			ERR_PRINT("Invalid image");
		}
		return;
	}

	if (svg_data) {
		_svg_destroy();
	}

	w = p_image->get_width();
	h = p_image->get_height();
	format = p_image->get_format();
	mipmaps = p_image->has_mipmaps();

	if (texture.is_null()) {
		texture = Renderer::texture_2d_create(p_image);
	}
	else {
		RID new_texture = Renderer::texture_2d_create(p_image);
		Renderer::texture_replace(texture, new_texture);
	}
	emit_changed();

	alpha_cache.unref();
	image_stored = true;
}

Image::Format ImageTextureLayered::get_format() const { return format; }

int ImageTextureLayered::get_width() const { return width; }

int ImageTextureLayered::get_height() const { return height; }

int ImageTextureLayered::get_layers() const { return layers; }

bool ImageTextureLayered::has_mipmaps() const { return mipmaps; }

void ImageTextureLayered::set_layered_type(LayeredType p_type)
{
	layered_type = p_type;
}

ImageTextureLayered::LayeredType ImageTextureLayered::get_layered_type() const
{
	return layered_type;
}

Error ImageTextureLayered::create_from_images(Vector<Ref<Image>> p_images)
{
	int new_layers = p_images.size();
	ERR_FAIL_COND_V(new_layers == 0, ERR_INVALID_PARAMETER);
	if (layered_type == LAYERED_TYPE_CUBEMAP) {
		ERR_FAIL_COND_V_MSG(
			new_layers != 6, ERR_INVALID_PARAMETER, "Cubemaps require exactly 6 layers");
	}
	else if (layered_type == LAYERED_TYPE_CUBEMAP_ARRAY) {
		ERR_FAIL_COND_V_MSG((new_layers % 6) != 0, ERR_INVALID_PARAMETER,
			"Cubemap array layers must be a multiple of 6");
	}

	ERR_FAIL_COND_V(p_images[0].is_null() || p_images[0]->is_empty(), ERR_INVALID_PARAMETER);

	Image::Format new_format = p_images[0]->get_format();
	int new_width = p_images[0]->get_width();
	int new_height = p_images[0]->get_height();
	bool new_mipmaps = p_images[0]->has_mipmaps();

	for (int i = 1; i < p_images.size(); i++) {
		ERR_FAIL_COND_V_MSG(p_images[i]->get_format() != new_format, ERR_INVALID_PARAMETER,
			"All images must share the same format");
		ERR_FAIL_COND_V_MSG(
			p_images[i]->get_width() != new_width || p_images[i]->get_height() != new_height,
			ERR_INVALID_PARAMETER, "All images must share the same dimensions");
		ERR_FAIL_COND_V_MSG(p_images[i]->has_mipmaps() != new_mipmaps, ERR_INVALID_PARAMETER,
			"All images must share the usage of mipmaps");
	}

	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_2d_layered_create(
			p_images, RSE::TextureLayeredType(layered_type));
		ERR_FAIL_COND_V(!new_texture.is_valid(), ERR_CANT_CREATE);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_2d_layered_create(
			p_images, RSE::TextureLayeredType(layered_type));
		ERR_FAIL_COND_V(!texture.is_valid(), ERR_CANT_CREATE);
	}

	format = new_format;
	width = new_width;
	height = new_height;
	layers = new_layers;
	mipmaps = new_mipmaps;
	return OK;
}

void ImageTextureLayered::update_layer(const Ref<Image>& p_image, int p_layer)
{
	ERR_FAIL_COND_MSG(texture.is_null(), "Texture is not initialized.");
	ERR_FAIL_COND_MSG(p_image.is_null(), "Invalid image.");
	ERR_FAIL_COND_MSG(
		p_image->get_format() != format, "Image format must match texture's image format.");
	ERR_FAIL_COND_MSG(p_image->get_width() != width || p_image->get_height() != height,
		"Image size must match texture's image size.");
	ERR_FAIL_COND_MSG(p_image->has_mipmaps() != mipmaps,
		"Image mipmap configuration must match texture's image mipmap configuration.");
	ERR_FAIL_INDEX_MSG(p_layer, layers, "Layer index is out of bounds.");
	Renderer::texture_2d_update(texture, p_image, p_layer);
}

Ref<Image> ImageTextureLayered::get_layer_data(int p_layer) const
{
	ERR_FAIL_INDEX_V(p_layer, layers, Ref<Image>());
	return Renderer::texture_2d_layer_get(texture, p_layer);
}

RID ImageTextureLayered::get_rid() const
{
	if (texture.is_null()) {
		texture = Renderer::texture_2d_layered_placeholder_create(
			RSE::TextureLayeredType(layered_type));
	}
	return texture;
}

void ImageTextureLayered::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTextureLayered::ImageTextureLayered(LayeredType p_layered_type)
{
	layered_type = p_layered_type;
}

ImageTextureLayered::~ImageTextureLayered()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

Image::Format ImageTexture3D::get_format() const { return format; }

int ImageTexture3D::get_width() const { return width; }

int ImageTexture3D::get_height() const { return height; }

int ImageTexture3D::get_depth() const { return depth; }

bool ImageTexture3D::has_mipmaps() const { return mipmaps; }

Error ImageTexture3D::create(Image::Format p_format, int p_width, int p_height, int p_depth,
	bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	RID tex = Renderer::texture_3d_create(
		p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	ERR_FAIL_COND_V(tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, tex);
	}
	else {
		texture = tex;
	}

	format = p_format;
	width = p_width;
	height = p_height;
	depth = p_depth;
	mipmaps = p_mipmaps;

	return OK;
}

void ImageTexture3D::update(const Vector<Ref<Image>>& p_data)
{
	ERR_FAIL_COND(!texture.is_valid());
	Renderer::texture_3d_update(texture, p_data);
}

Vector<Ref<Image>> ImageTexture3D::get_data() const
{
	ERR_FAIL_COND_V(!texture.is_valid(), Vector<Ref<Image>>());
	return Renderer::texture_3d_get(texture);
}

RID ImageTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}
	return texture;
}

void ImageTexture3D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTexture3D::ImageTexture3D() {}

ImageTexture3D::~ImageTexture3D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

// --- external_texture.cpp ---

#include "external_texture.h"

#include "servers/camera/camera_feed.h"
#include "servers/rendering/renderer.h"

void ExternalTexture::set_source_mode(SourceMode p_mode)
{
	if (source_mode == p_mode) {
		return;
	}
	source_mode = p_mode;
	if (source_mode == SOURCE_CAMERA_FEED) {
		CameraServer::set_monitoring_feeds(true);
	}
	emit_changed();
}

uint64_t ExternalTexture::get_external_texture_id() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		return Renderer::texture_get_native_handle(get_rid());
	}
	_ensure_created();
	return Renderer::texture_get_native_handle(texture);
}

void ExternalTexture::set_size(const Size2& p_size)
{
	if (p_size.width > 0 && p_size.height > 0 && p_size != size) {
		size = p_size;
		if (source_mode == SOURCE_EXTERNAL_BUFFER) {
			_ensure_created();
			Renderer::texture_external_update(texture, size.width, size.height, external_buffer);
		}
		emit_changed();
	}
}

Size2 ExternalTexture::get_size() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		return Size2(get_width(), get_height());
	}
	return size;
}

void ExternalTexture::set_external_buffer_id(uint64_t p_external_buffer)
{
	source_mode = SOURCE_EXTERNAL_BUFFER;
	if (p_external_buffer != external_buffer) {
		external_buffer = p_external_buffer;
		_ensure_created();
		Renderer::texture_external_update(texture, size.width, size.height, external_buffer);
	}
}

void ExternalTexture::set_camera_feed_id(int p_new_id)
{
	if (source_mode != SOURCE_CAMERA_FEED) {
		source_mode = SOURCE_CAMERA_FEED;
		CameraServer::set_monitoring_feeds(true);
	}
	if (camera_feed_id != p_new_id) {
		camera_feed_id = p_new_id;
		emit_changed();
	}
}

int ExternalTexture::get_camera_feed_id() const { return camera_feed_id; }

void ExternalTexture::set_which_feed(CameraServer::FeedImage p_which)
{
	if (source_mode != SOURCE_CAMERA_FEED) {
		source_mode = SOURCE_CAMERA_FEED;
		CameraServer::set_monitoring_feeds(true);
	}
	if (which_feed != p_which) {
		which_feed = p_which;
		emit_changed();
	}
}

CameraServer::FeedImage ExternalTexture::get_which_feed() const { return which_feed; }

void ExternalTexture::set_camera_active(bool p_active)
{
	if (source_mode != SOURCE_CAMERA_FEED) {
		source_mode = SOURCE_CAMERA_FEED;
		CameraServer::set_monitoring_feeds(true);
	}
	Ref<CameraFeed> feed = CameraServer::get_feed_by_id(camera_feed_id);
	if (feed.is_valid()) {
		feed->set_active(p_active);
		emit_changed();
	}
}

bool ExternalTexture::get_camera_active() const
{
	Ref<CameraFeed> feed = CameraServer::get_feed_by_id(camera_feed_id);
	if (feed.is_valid()) {
		return feed->is_active();
	}
	return false;
}

int ExternalTexture::get_width() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		Ref<CameraFeed> feed = CameraServer::get_feed_by_id(camera_feed_id);
		if (feed.is_valid()) {
			return feed->get_base_width();
		}
		return 0;
	}
	return size.width;
}

int ExternalTexture::get_height() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		Ref<CameraFeed> feed = CameraServer::get_feed_by_id(camera_feed_id);
		if (feed.is_valid()) {
			return feed->get_base_height();
		}
		return 0;
	}
	return size.height;
}

bool ExternalTexture::has_alpha() const { return false; }

RID ExternalTexture::get_rid() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		Ref<CameraFeed> feed = CameraServer::get_feed_by_id(camera_feed_id);
		if (feed.is_valid()) {
			return feed->get_texture(which_feed);
		}
		if (!texture.is_valid()) {
			texture = Renderer::texture_2d_placeholder_create();
			using_placeholder = true;
		}
		return texture;
	}

	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_placeholder_create();
		using_placeholder = true;
	}
	return texture;
}

Ref<Image> ExternalTexture::get_image() const
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		return Renderer::texture_2d_get(get_rid());
	}
	return Texture2D::get_image();
}

void ExternalTexture::_ensure_created() const
{
	if (texture.is_valid() && !using_placeholder) {
		return;
	}

	RID new_texture = Renderer::texture_external_create(size.width, size.height);
	if (using_placeholder) {
		DEV_ASSERT(texture.is_valid());
		Renderer::texture_replace(texture, new_texture);
		using_placeholder = false;
	}
	else {
		texture = new_texture;
	}
}

ExternalTexture::ExternalTexture(SourceMode p_mode) : source_mode(p_mode)
{
	if (source_mode == SOURCE_CAMERA_FEED) {
		CameraServer::set_monitoring_feeds(true);
	}
}

ExternalTexture::~ExternalTexture()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

// --- drawable_texture_2d.cpp ---

#include "drawable_texture_2d.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

DrawableTexture2D::DrawableTexture2D()
{
	default_material = Renderer::texture_drawable_get_default_material();
}

DrawableTexture2D::~DrawableTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

// Initialize Texture Resource with a call to rendering server. Overwrite existing.
void DrawableTexture2D::_initialize()
{
	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_drawable_create(
			width, height, (RSE::TextureDrawableFormat)format, base_color, mipmaps);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_drawable_create(
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
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

void DrawableTexture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if ((width | height) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(width, height)), texture, false, p_modulate, p_transpose);
}

void DrawableTexture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if ((width | height) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void DrawableTexture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if ((width | height) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

Ref<Image> DrawableTexture2D::get_image() const
{
	if (texture.is_valid()) {
		return Renderer::texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

void DrawableTexture2D::generate_mipmaps()
{
	if (texture.is_valid()) {
		Renderer::texture_drawable_generate_mipmaps(texture);
	}
}

// --- curve_texture.cpp ---

#include "curve_texture.h"

#include "servers/rendering/renderer.h"

CurveTexture::CurveTexture(TextureMode p_mode) :
	texture_mode(p_mode), _current_texture_mode(p_mode)
{
}

void CurveTexture::set_width(int p_width)
{
	ERR_FAIL_COND(p_width < 32 || p_width > 4096);

	if (_width == p_width) {
		return;
	}

	_width = p_width;
	_update();
}

int CurveTexture::get_width() const { return _width; }

void CurveTexture::ensure_default_setup(float p_min, float p_max)
{
	if (texture_mode == TEXTURE_MODE_XYZ) {
		if (_curve_x.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_x(curve);
		}
		if (_curve_y.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_y(curve);
		}
		if (_curve_z.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_z(curve);
		}
	}
	else if (_curve.is_null()) {
		Ref<Curve> curve = Ref<Curve>(memnew(Curve));
		curve->add_point(Vector2(0, 1));
		curve->add_point(Vector2(1, 1));
		curve->set_min_value(p_min);
		curve->set_max_value(p_max);
		set_curve(curve);
	}
}

void CurveTexture::set_curve(Ref<Curve> p_curve)
{
	if (_curve != p_curve) {
		_curve = p_curve;
		_update();
	}
}

Ref<Curve> CurveTexture::get_curve() const { return _curve; }

void CurveTexture::set_curve_x(Ref<Curve> p_curve)
{
	if (_curve_x == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_x = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_x() const { return _curve_x; }

void CurveTexture::set_curve_y(Ref<Curve> p_curve)
{
	if (_curve_y == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_y = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_y() const { return _curve_y; }

void CurveTexture::set_curve_z(Ref<Curve> p_curve)
{
	if (_curve_z == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_z = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_z() const { return _curve_z; }

void CurveTexture::set_texture_mode(TextureMode p_mode)
{
	ERR_FAIL_COND(p_mode < TEXTURE_MODE_RGB || p_mode > TEXTURE_MODE_XYZ);
	if (texture_mode == p_mode) {
		return;
	}
	texture_mode = p_mode;
	_update();
}

CurveTexture::TextureMode CurveTexture::get_texture_mode() const { return texture_mode; }

void CurveTexture::_update()
{
	if (!Renderer::is_initialized()) {
		emit_changed();
		return;
	}

	const int channels = (texture_mode == TEXTURE_MODE_RED) ? 1 : 3;
	Vector<uint8_t> data;
	data.resize(_width * sizeof(float) * channels);

	{
		uint8_t* wd8 = data.ptrw();
		float* wd = reinterpret_cast<float*>(wd8);

		if (texture_mode == TEXTURE_MODE_XYZ) {
			if (_curve_x.is_valid()) {
				Curve& curve_x = *_curve_x;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 0] = curve_x.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 0] = 0.0f;
				}
			}

			if (_curve_y.is_valid()) {
				Curve& curve_y = *_curve_y;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 1] = curve_y.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 1] = 0.0f;
				}
			}

			if (_curve_z.is_valid()) {
				Curve& curve_z = *_curve_z;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 2] = curve_z.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 2] = 0.0f;
				}
			}
		}
		else if (_curve.is_valid()) {
			Curve& curve = *_curve;
			for (int i = 0; i < _width; ++i) {
				float t = i / static_cast<float>(_width);
				float v = curve.sample_baked(t);
				if (texture_mode == TEXTURE_MODE_RGB) {
					wd[i * 3 + 0] = v;
					wd[i * 3 + 1] = v;
					wd[i * 3 + 2] = v;
				}
				else {
					wd[i] = v;
				}
			}
		}
		else {
			for (int i = 0; i < _width * channels; ++i) {
				wd[i] = 0.0f;
			}
		}
	}

	const Image::Format new_format =
		(texture_mode == TEXTURE_MODE_RED) ? Image::FORMAT_RF : Image::FORMAT_RGBF;
	const Image::Format old_format =
		(_current_texture_mode == TEXTURE_MODE_RED) ? Image::FORMAT_RF : Image::FORMAT_RGBF;

	Ref<Image> image = memnew(Image(_width, 1, false, new_format, data));

	if (_texture.is_valid()) {
		if (old_format != new_format || _current_width != _width) {
			RID new_texture = Renderer::texture_2d_create(image);
			Renderer::texture_replace(_texture, new_texture);
		}
		else {
			Renderer::texture_2d_update(_texture, image);
		}
	}
	else {
		_texture = Renderer::texture_2d_create(image);
	}
	_current_texture_mode = texture_mode;
	_current_width = _width;

	emit_changed();
}

RID CurveTexture::get_rid() const
{
	if (!_texture.is_valid() && Renderer::is_initialized()) {
		if (_curve.is_valid() || _curve_x.is_valid() || _curve_y.is_valid() || _curve_z.is_valid()) {
			const_cast<CurveTexture*>(this)->_update();
		}
		if (!_texture.is_valid()) {
			_texture = Renderer::texture_2d_placeholder_create();
		}
	}
	return _texture;
}

CurveTexture::~CurveTexture()
{
	if (_texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(_texture);
	}
}

// --- gradient_texture.cpp ---

#include "gradient_texture.h"

#include "core/math/geometry_2d.h"
#include "servers/rendering/renderer.h"

GradientTexture2D::GradientTexture2D(bool p_1d) : is_1d(p_1d)
{
	if (is_1d) {
		width = 256;
		height = 1;
	}
	_queue_update();
}

GradientTexture2D::~GradientTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void GradientTexture2D::set_gradient(Ref<Gradient> p_gradient)
{
	if (gradient == p_gradient) {
		return;
	}
	gradient = p_gradient;
	_queue_update();
	emit_changed();
}

Ref<Gradient> GradientTexture2D::get_gradient() const { return gradient; }

void GradientTexture2D::_queue_update() { update_pending = true; }

void GradientTexture2D::_update() const
{
	update_pending = false;

	if (gradient.is_null() || !Renderer::is_initialized()) {
		return;
	}

	Ref<Image> image;
	if (is_1d) {
		if (use_hdr) {
			image.instantiate(width, 1, false, Image::FORMAT_RGBAF);
			Gradient& g = *gradient;
			for (int i = 0; i < width; i++) {
				float ofs = (width > 1) ? float(i) / (width - 1) : 0.0f;
				image->set_pixel(i, 0, g.get_color_at_offset(ofs));
			}
		}
		else {
			Vector<uint8_t> data;
			data.resize(width * 4);
			{
				uint8_t* wd8 = data.ptrw();
				Gradient& g = *gradient;
				for (int i = 0; i < width; i++) {
					float ofs = (width > 1) ? float(i) / (width - 1) : 0.0f;
					Color c = g.get_color_at_offset(ofs);
					wd8[i * 4 + 0] = uint8_t(c.get_r8());
					wd8[i * 4 + 1] = uint8_t(c.get_g8());
					wd8[i * 4 + 2] = uint8_t(c.get_b8());
					wd8[i * 4 + 3] = uint8_t(c.get_a8());
				}
			}
			image.instantiate(width, 1, false, Image::FORMAT_RGBA8, data);
		}
	}
	else {
		image.instantiate();
		if (gradient->get_point_count() <= 1) {
			image->initialize_data(
				width, height, false, use_hdr ? Image::FORMAT_RGBAF : Image::FORMAT_RGBA8);
			image->fill(
				(gradient->get_point_count() == 1) ? gradient->get_color(0) : Color(0, 0, 0, 1));
		}
		else if (use_hdr) {
			image->initialize_data(width, height, false, Image::FORMAT_RGBAF);
			Gradient& g = *gradient;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					float ofs = _get_gradient_offset_at(x, y);
					image->set_pixel(x, y, g.get_color_at_offset(ofs));
				}
			}
		}
		else {
			Vector<uint8_t> data;
			data.resize(width * height * 4);
			{
				uint8_t* wd8 = data.ptrw();
				Gradient& g = *gradient;
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						float ofs = _get_gradient_offset_at(x, y);
						const Color& c = g.get_color_at_offset(ofs);

						wd8[(x + (y * width)) * 4 + 0] = uint8_t(c.get_r8());
						wd8[(x + (y * width)) * 4 + 1] = uint8_t(c.get_g8());
						wd8[(x + (y * width)) * 4 + 2] = uint8_t(c.get_b8());
						wd8[(x + (y * width)) * 4 + 3] = uint8_t(c.get_a8());
					}
				}
			}
			image->set_data(width, height, false, Image::FORMAT_RGBA8, data);
		}
	}

	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_2d_create(image);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_2d_create(image);
	}
	Renderer::texture_set_path(texture, get_path());
}

float GradientTexture2D::_get_gradient_offset_at(int x, int y) const
{
	if (fill_to == fill_from) {
		return 0;
	}
	float ofs = 0;
	Vector2 pos;
	if (width > 1) {
		pos.x = static_cast<float>(x) / (width - 1);
	}
	if (height > 1) {
		pos.y = static_cast<float>(y) / (height - 1);
	}
	if (fill == Fill::FILL_LINEAR) {
		const Vector2 closest =
			Geometry2D::get_closest_point_to_segment_uncapped(pos, fill_from, fill_to);
		ofs = (closest - fill_from).length() / (fill_to - fill_from).length();
		if ((closest - fill_from).dot(fill_to - fill_from) < 0) {
			ofs *= -1;
		}
	}
	else if (fill == Fill::FILL_RADIAL) {
		ofs = (pos - fill_from).length() / (fill_to - fill_from).length();
	}
	else if (fill == Fill::FILL_SQUARE) {
		ofs = MAX(Math::abs(pos.x - fill_from.x), Math::abs(pos.y - fill_from.y)) /
			  MAX(Math::abs(fill_to.x - fill_from.x), Math::abs(fill_to.y - fill_from.y));
	}
	else if (fill == Fill::FILL_CONIC) {
		float rel_angle = (fill_to - fill_from).angle_to(pos - fill_from);
		ofs = Math::fposmod((double)rel_angle, Math::TAU) / Math::TAU;
	}
	if (repeat == Repeat::REPEAT_NONE) {
		ofs = CLAMP(ofs, 0.0, 1.0);
	}
	else if (repeat == Repeat::REPEAT) {
		ofs = Math::fmod(ofs, 1.0f);
		if (ofs < 0) {
			ofs = 1 + ofs;
		}
	}
	else if (repeat == Repeat::REPEAT_MIRROR) {
		ofs = Math::abs(ofs);
		ofs = Math::fmod(ofs, 2.0f);
		if (ofs > 1.0) {
			ofs = 2.0 - ofs;
		}
	}
	return ofs;
}

void GradientTexture2D::set_width(int p_width)
{
	ERR_FAIL_COND_MSG(
		p_width <= 0 || p_width > 16384, "Texture dimensions have to be within 1 to 16384 range.");
	width = p_width;
	_queue_update();
	emit_changed();
}

int GradientTexture2D::get_width() const { return width; }

void GradientTexture2D::set_height(int p_height)
{
	ERR_FAIL_COND_MSG(p_height <= 0 || p_height > 16384,
		"Texture dimensions have to be within 1 to 16384 range.");
	if (is_1d) {
		height = 1;
		return;
	}
	height = p_height;
	_queue_update();
	emit_changed();
}

int GradientTexture2D::get_height() const { return is_1d ? 1 : height; }

void GradientTexture2D::set_use_hdr(bool p_enabled)
{
	if (p_enabled == use_hdr) {
		return;
	}

	use_hdr = p_enabled;
	_queue_update();
	emit_changed();
}

bool GradientTexture2D::is_using_hdr() const { return use_hdr; }

void GradientTexture2D::set_fill_from(Vector2 p_fill_from)
{
	fill_from = p_fill_from;
	_queue_update();
	emit_changed();
}

Vector2 GradientTexture2D::get_fill_from() const { return fill_from; }

void GradientTexture2D::set_fill_to(Vector2 p_fill_to)
{
	fill_to = p_fill_to;
	_queue_update();
	emit_changed();
}

Vector2 GradientTexture2D::get_fill_to() const { return fill_to; }

void GradientTexture2D::set_fill(Fill p_fill)
{
	fill = p_fill;
	_queue_update();
	emit_changed();
}

GradientTexture2D::Fill GradientTexture2D::get_fill() const { return fill; }

void GradientTexture2D::set_repeat(Repeat p_repeat)
{
	repeat = p_repeat;
	_queue_update();
	emit_changed();
}

GradientTexture2D::Repeat GradientTexture2D::get_repeat() const { return repeat; }

RID GradientTexture2D::get_rid() const
{
	if (update_pending) {
		_update();
	}
	if (!texture.is_valid() && Renderer::is_initialized()) {
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

Ref<Image> GradientTexture2D::get_image() const
{
	update_now();
	if (!texture.is_valid()) {
		return Ref<Image>();
	}
	return Renderer::texture_2d_get(texture);
}

void GradientTexture2D::update_now() const
{
	if (update_pending) {
		_update();
	}
}

// --- bit_map.cpp ---

#include "bit_map.h"

void BitMap::create(const Size2i& p_size)
{
	ERR_FAIL_COND(p_size.width < 1);
	ERR_FAIL_COND(p_size.height < 1);

	ERR_FAIL_COND(
		static_cast<int64_t>(p_size.width) * static_cast<int64_t>(p_size.height) > INT32_MAX);

	Error err = bitmask.resize(Math::division_round_up(p_size.width * p_size.height, 8));
	ERR_FAIL_COND(err != OK);

	width = p_size.width;
	height = p_size.height;

	memset(bitmask.ptrw(), 0, bitmask.size());
}

void BitMap::set_bit_rect(const Rect2i& p_rect, bool p_value)
{
	Rect2i current = Rect2i(0, 0, width, height).intersection(p_rect);
	uint8_t* data = bitmask.ptrw();

	for (int i = current.position.x; i < current.position.x + current.size.x; i++) {
		for (int j = current.position.y; j < current.position.y + current.size.y; j++) {
			int ofs = width * j + i;
			int bbyte = ofs / 8;
			int bbit = ofs % 8;

			uint8_t b = data[bbyte];

			if (p_value) {
				b |= (1 << bbit);
			}
			else {
				b &= ~(1 << bbit);
			}

			data[bbyte] = b;
		}
	}
}

int BitMap::get_true_bit_count() const
{
	int ds = bitmask.size();
	const uint8_t* d = bitmask.ptr();
	int c = 0;

	// Fast, almost branchless version.

	for (int i = 0; i < ds; i++) {
		c += (d[i] & (1 << 7)) >> 7;
		c += (d[i] & (1 << 6)) >> 6;
		c += (d[i] & (1 << 5)) >> 5;
		c += (d[i] & (1 << 4)) >> 4;
		c += (d[i] & (1 << 3)) >> 3;
		c += (d[i] & (1 << 2)) >> 2;
		c += (d[i] & (1 << 1)) >> 1;
		c += d[i] & 1;
	}

	return c;
}

void BitMap::set_bitv(const Point2i& p_pos, bool p_value) { set_bit(p_pos.x, p_pos.y, p_value); }

void BitMap::set_bit(int p_x, int p_y, bool p_value)
{
	ERR_FAIL_INDEX(p_x, width);
	ERR_FAIL_INDEX(p_y, height);

	int ofs = width * p_y + p_x;
	int bbyte = ofs / 8;
	int bbit = ofs % 8;

	uint8_t b = bitmask[bbyte];

	if (p_value) {
		b |= (1 << bbit);
	}
	else {
		b &= ~(1 << bbit);
	}

	bitmask.write[bbyte] = b;
}

bool BitMap::get_bitv(const Point2i& p_pos) const { return get_bit(p_pos.x, p_pos.y); }

bool BitMap::get_bit(int p_x, int p_y) const
{
	ERR_FAIL_INDEX_V(p_x, width, false);
	ERR_FAIL_INDEX_V(p_y, height, false);

	int ofs = width * p_y + p_x;
	int bbyte = ofs / 8;
	int bbit = ofs % 8;

	return (bitmask[bbyte] & (1 << bbit)) != 0;
}

Size2i BitMap::get_size() const { return Size2i(width, height); }

Vector<Vector<Vector2>> BitMap::_march_square(const Rect2i& p_rect, const Point2i& p_start) const
{
	int stepx = 0;
	int stepy = 0;
	int prevx = 0;
	int prevy = 0;
	int startx = p_start.x;
	int starty = p_start.y;
	int curx = startx;
	int cury = starty;
	unsigned int count = 0;

	HashMap<Point2i, int> cross_map;

	Vector<Vector2> _points;
	int points_size = 0;

	Vector<Vector<Vector2>> ret;

	// Add starting entry at start of return.
	ret.resize(1);

	do {
		int sv = 0;
		{ // Square value

			/*
			checking the 2x2 pixel grid, assigning these values to each pixel, if not transparent
			+---+---+
			| 1 | 2 |
			+---+---+
			| 4 | 8 | <- current pixel (curx,cury)
			+---+---+
			*/
			Point2i tl = Point2i(curx - 1, cury - 1);
			sv += (p_rect.has_point(tl) && get_bitv(tl)) ? 1 : 0;
			Point2i tr = Point2i(curx, cury - 1);
			sv += (p_rect.has_point(tr) && get_bitv(tr)) ? 2 : 0;
			Point2i bl = Point2i(curx - 1, cury);
			sv += (p_rect.has_point(bl) && get_bitv(bl)) ? 4 : 0;
			Point2i br = Point2i(curx, cury);
			sv += (p_rect.has_point(br) && get_bitv(br)) ? 8 : 0;
			ERR_FAIL_COND_V(sv == 0 || sv == 15, Vector<Vector<Vector2>>());
		}

		switch (sv) {
		case 1:
		case 5:
		case 13:
			/* going UP with these cases:
			1          5           13
			+---+---+  +---+---+  +---+---+
			| 1 |   |  | 1 |   |  | 1 |   |
			+---+---+  +---+---+  +---+---+
			|   |   |  | 4 |   |  | 4 | 8 |
			+---+---+  +---+---+  +---+---+
			*/
			stepx = 0;
			stepy = -1;
			break;

		case 8:
		case 10:
		case 11:
			/* going DOWN with these cases:
			8          10         11
			+---+---+  +---+---+  +---+---+
			|   |   |  |   | 2 |  | 1 | 2 |
			+---+---+  +---+---+  +---+---+
			|   | 8 |  |   | 8 |  |   | 8 |
			+---+---+  +---+---+  +---+---+
			*/
			stepx = 0;
			stepy = 1;
			break;

		case 4:
		case 12:
		case 14:
			/* going LEFT with these cases:
			4          12         14
			+---+---+  +---+---+  +---+---+
			|   |   |  |   |   |  |   | 2 |
			+---+---+  +---+---+  +---+---+
			| 4 |   |  | 4 | 8 |  | 4 | 8 |
			+---+---+  +---+---+  +---+---+
			*/
			stepx = -1;
			stepy = 0;
			break;

		case 2:
		case 3:
		case 7:
			/* going RIGHT with these cases:
			2          3          7
			+---+---+  +---+---+  +---+---+
			|   | 2 |  | 1 | 2 |  | 1 | 2 |
			+---+---+  +---+---+  +---+---+
			|   |   |  |   |   |  | 4 |   |
			+---+---+  +---+---+  +---+---+
			*/
			stepx = 1;
			stepy = 0;
			break;
		case 9:
			/* Going DOWN if coming from the LEFT, otherwise go UP.
			9
			+---+---+
			| 1 |   |
			+---+---+
			|   | 8 |
			+---+---+
			*/

			if (prevx == 1) {
				stepx = 0;
				stepy = 1;
			}
			else {
				stepx = 0;
				stepy = -1;
			}
			break;
		case 6:
			/* Going RIGHT if coming from BELOW, otherwise go LEFT.
			6
			+---+---+
			|   | 2 |
			+---+---+
			| 4 |   |
			+---+---+
			*/

			if (prevy == -1) {
				stepx = 1;
				stepy = 0;
			}
			else {
				stepx = -1;
				stepy = 0;
			}
			break;
		default:
			ERR_PRINT("this shouldn't happen.");
		}

		// Handle crossing points.
		if (sv == 6 || sv == 9) {
			const Point2i cur_pos(curx, cury);

			// Find if this point has occurred before.
			if (HashMap<Point2i, int>::Iterator found = cross_map.find(cur_pos)) {
				// Add points after the previous crossing to the result.
				ret.push_back(_points.slice(found->value + 1, points_size));

				// Remove points after crossing point.
				points_size = found->value + 1;

				// Erase trailing map elements.
				while (cross_map.last() != found) {
					cross_map.remove(cross_map.last());
				}

				cross_map.erase(cur_pos);
			}
			else {
				// Add crossing point to map.
				cross_map.insert(cur_pos, points_size - 1);
			}
		}

		// Small optimization:
		// If the previous direction is same as the current direction,
		// then we should modify the last vector to current.
		curx += stepx;
		cury += stepy;
		if (stepx == prevx && stepy == prevy) {
			_points.set(points_size - 1, Vector2(curx, cury) - p_rect.position);
		}
		else {
			_points.resize(MAX(points_size + 1, _points.size()));
			_points.set(points_size, Vector2(curx, cury) - p_rect.position);
			points_size++;
		}

		count++;
		prevx = stepx;
		prevy = stepy;

		ERR_FAIL_COND_V((int)count > 2 * (width * height + 1), Vector<Vector<Vector2>>());
	} while (curx != startx || cury != starty);

	// Add remaining points to result.
	_points.resize(points_size);

	ret.set(0, _points);

	return ret;
}

static float perpendicular_distance(const Vector2& i, const Vector2& start, const Vector2& end)
{
	float res;
	float slope;
	float intercept;

	if (start.x == end.x) {
		res = Math::abs(i.x - end.x);
	}
	else if (start.y == end.y) {
		res = Math::abs(i.y - end.y);
	}
	else {
		slope = (end.y - start.y) / (end.x - start.x);
		intercept = start.y - (slope * start.x);
		res = Math::abs(slope * i.x - i.y + intercept) / Math::sqrt(Math::pow(slope, 2.0f) + 1.0);
	}
	return res;
}

static Vector<Vector2> rdp(const Vector<Vector2>& v, float optimization)
{
	if (v.size() < 3) {
		return v;
	}

	int index = -1;
	float dist = 0.0;
	// Not looping first and last point.
	for (size_t i = 1, size = v.size(); i < size - 1; ++i) {
		float cdist = perpendicular_distance(v[i], v[0], v[v.size() - 1]);
		if (cdist > dist) {
			dist = cdist;
			index = static_cast<int>(i);
		}
	}
	if (dist > optimization) {
		Vector<Vector2> left, right;
		left.resize(index);
		for (int i = 0; i < index; i++) {
			left.write[i] = v[i];
		}
		right.resize(v.size() - index);
		for (int i = 0; i < right.size(); i++) {
			right.write[i] = v[index + i];
		}
		Vector<Vector2> r1 = rdp(left, optimization);
		Vector<Vector2> r2 = rdp(right, optimization);

		int middle = r1.size();
		r1.resize(r1.size() + r2.size());
		for (int i = 0; i < r2.size(); i++) {
			r1.write[middle + i] = r2[i];
		}
		return r1;
	}
	else {
		Vector<Vector2> ret;
		ret.push_back(v[0]);
		ret.push_back(v[v.size() - 1]);
		return ret;
	}
}

static Vector<Vector2> reduce(const Vector<Vector2>& points, const Rect2i& rect, float epsilon)
{
	int size = points.size();
	// If there are less than 3 points, then we have nothing.
	ERR_FAIL_COND_V(size < 3, Vector<Vector2>());
	// If there are less than 9 points (but more than 3), then we don't need to reduce it.
	if (size < 9) {
		return points;
	}

	float maxEp = MIN(rect.size.width, rect.size.height);
	float ep = CLAMP(epsilon, 0.0, maxEp / 2);
	Vector<Vector2> result = rdp(points, ep);

	Vector2 last = result[result.size() - 1];

	if (last.y > result[0].y && last.distance_to(result[0]) < ep * 0.5f) {
		result.write[0].y = last.y;
		result.resize(result.size() - 1);
	}
	return result;
}

struct FillBitsStackEntry
{
	Point2i pos;
	int i = 0;
	int j = 0;
};

static void fill_bits(
	const BitMap* p_src, Ref<BitMap>& p_map, const Point2i& p_pos, const Rect2i& rect)
{
	// Using a custom stack to work iteratively to avoid stack overflow on big bitmaps.
	Vector<FillBitsStackEntry> stack;
	// Tracking size since we won't be shrinking the stack vector.
	int stack_size = 0;

	Point2i pos = p_pos;
	int next_i = 0;
	int next_j = 0;

	bool reenter = true;
	bool popped = false;
	do {
		if (reenter) {
			next_i = pos.x - 1;
			next_j = pos.y - 1;
			reenter = false;
		}

		for (int i = next_i; i <= pos.x + 1; i++) {
			for (int j = next_j; j <= pos.y + 1; j++) {
				if (popped) {
					// The next loop over j must start normally.
					next_j = pos.y - 1;
					popped = false;
					// Skip because an iteration was already executed with current counter values.
					continue;
				}

				if (i < rect.position.x || i >= rect.position.x + rect.size.x) {
					continue;
				}
				if (j < rect.position.y || j >= rect.position.y + rect.size.y) {
					continue;
				}

				if (p_map->get_bit(i, j)) {
					continue;

				}
				else if (p_src->get_bit(i, j)) {
					p_map->set_bit(i, j, true);

					FillBitsStackEntry se = {pos, i, j};
					stack.resize(MAX(stack_size + 1, stack.size()));
					stack.set(stack_size, se);
					stack_size++;

					pos = Point2i(i, j);
					reenter = true;
					break;
				}
			}
			if (reenter) {
				break;
			}
		}
		if (!reenter) {
			if (stack_size) {
				FillBitsStackEntry se = stack.get(stack_size - 1);
				stack_size--;
				pos = se.pos;
				next_i = se.i;
				next_j = se.j;
				popped = true;
			}
		}
	} while (reenter || popped);
}

Vector<Vector<Vector2>> BitMap::clip_opaque_to_polygons(const Rect2i& p_rect, float p_epsilon) const
{
	Rect2i r = Rect2i(0, 0, width, height).intersection(p_rect);

	Ref<BitMap> fill;
	fill.instantiate();
	fill->create(get_size());

	Vector<Vector<Vector2>> polygons;
	for (int i = r.position.y; i < r.position.y + r.size.height; i++) {
		for (int j = r.position.x; j < r.position.x + r.size.width; j++) {
			if (!fill->get_bit(j, i) && get_bit(j, i)) {
				fill_bits(this, fill, Point2i(j, i), r);

				for (Vector<Vector2> polygon : _march_square(r, Point2i(j, i))) {
					polygon = reduce(polygon, r, p_epsilon);

					if (polygon.size() < 3) {
						print_verbose("Invalid polygon, skipped");
						continue;
					}

					polygons.push_back(polygon);
				}
			}
		}
	}

	return polygons;
}

void BitMap::grow_mask(int p_pixels, const Rect2i& p_rect)
{
	if (p_pixels == 0) {
		return;
	}

	bool bit_value = p_pixels > 0;
	p_pixels = Math::abs(p_pixels);
	const int pixels2 = p_pixels * p_pixels;

	Rect2i r = Rect2i(0, 0, width, height).intersection(p_rect);

	Ref<BitMap> copy;
	copy.instantiate();
	copy->create(get_size());
	copy->bitmask = bitmask;

	for (int i = r.position.y; i < r.position.y + r.size.height; i++) {
		for (int j = r.position.x; j < r.position.x + r.size.width; j++) {
			if (bit_value == get_bit(j, i)) {
				continue;
			}

			bool found = false;

			for (int y = i - p_pixels; y <= i + p_pixels; y++) {
				for (int x = j - p_pixels; x <= j + p_pixels; x++) {
					bool outside = false;

					if ((x < p_rect.position.x) || (x >= p_rect.position.x + p_rect.size.x) ||
						(y < p_rect.position.y) || (y >= p_rect.position.y + p_rect.size.y)) {
						// Outside of rectangle counts as bit not set.
						if (!bit_value) {
							outside = true;
						}
						else {
							continue;
						}
					}

					float d = Point2(j, i).distance_squared_to(Point2(x, y)) - CMP_EPSILON2;
					if (d > pixels2) {
						continue;
					}

					if (outside || (bit_value == copy->get_bit(x, y))) {
						found = true;
						break;
					}
				}
				if (found) {
					break;
				}
			}

			if (found) {
				set_bit(j, i, bit_value);
			}
		}
	}
}

void BitMap::shrink_mask(int p_pixels, const Rect2i& p_rect) { grow_mask(-p_pixels, p_rect); }

void BitMap::resize(const Size2i& p_new_size)
{
	ERR_FAIL_COND(p_new_size.width < 0 || p_new_size.height < 0);
	if (p_new_size == get_size()) {
		return;
	}

	Ref<BitMap> new_bitmap;
	new_bitmap.instantiate();
	new_bitmap->create(p_new_size);
	// also allow for upscaling
	int lw = (width == 0) ? 0 : p_new_size.width;
	int lh = (height == 0) ? 0 : p_new_size.height;

	float scale_x = ((float)width / p_new_size.width);
	float scale_y = ((float)height / p_new_size.height);
	for (int x = 0; x < lw; x++) {
		for (int y = 0; y < lh; y++) {
			bool new_bit = get_bit(x * scale_x, y * scale_y);
			new_bitmap->set_bit(x, y, new_bit);
		}
	}

	width = new_bitmap->width;
	height = new_bitmap->height;
	bitmask = new_bitmap->bitmask;
}

Ref<Image> BitMap::convert_to_image() const
{
	Ref<Image> image = Image::create_empty(width, height, false, Image::FORMAT_L8);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			image->set_pixel(i, j, get_bit(i, j) ? Color(1, 1, 1) : Color(0, 0, 0));
		}
	}

	return image;
}

void BitMap::blit(const Vector2i& p_pos, const Ref<BitMap>& p_bitmap)
{
	ERR_FAIL_COND_MSG(p_bitmap.is_null(), "It's not a reference to a valid BitMap object.");

	int x = p_pos.x;
	int y = p_pos.y;
	int w = p_bitmap->get_size().width;
	int h = p_bitmap->get_size().height;

	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			int px = x + i;
			int py = y + j;
			if (px < 0 || px >= width) {
				continue;
			}
			if (py < 0 || py >= height) {
				continue;
			}
			if (p_bitmap->get_bit(i, j)) {
				set_bit(px, py, true);
			}
		}
	}
}

// --- sprite_frames.cpp ---

#include "scene/scene_string_names.h"
#include "sprite_frames.h"

void SpriteFrames::add_frame(
	const String& p_anim, const Ref<Texture2D>& p_texture, float p_duration, int p_at_pos)
{
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	p_duration = MAX(SPRITE_FRAME_MINIMUM_DURATION, p_duration);

	Frame frame = {p_texture, p_duration};

	if (p_at_pos >= 0 && p_at_pos < E->value.frames.size()) {
		E->value.frames.insert(p_at_pos, frame);
	}
	else {
		E->value.frames.push_back(frame);
	}

	emit_changed();
}

void SpriteFrames::set_frame(
	const String& p_anim, int p_idx, const Ref<Texture2D>& p_texture, float p_duration)
{
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	ERR_FAIL_COND(p_idx < 0);
	if (p_idx >= E->value.frames.size()) {
		return;
	}

	p_duration = MAX(SPRITE_FRAME_MINIMUM_DURATION, p_duration);

	Frame frame = {p_texture, p_duration};

	E->value.frames.write[p_idx] = frame;

	emit_changed();
}

int SpriteFrames::get_frame_count(const String& p_anim) const
{
	HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(!E, 0, "Animation '" + String(p_anim) + "' doesn't exist.");

	return E->value.frames.size();
}

void SpriteFrames::remove_frame(const String& p_anim, int p_idx)
{
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	E->value.frames.remove_at(p_idx);

	emit_changed();
}

void SpriteFrames::clear(const String& p_anim)
{
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");

	E->value.frames.clear();

	emit_changed();
}

void SpriteFrames::clear_all()
{
	animations.clear();
	add_animation(SceneString(default_));
}

void SpriteFrames::add_animation(const String& p_anim)
{
	ERR_FAIL_COND_MSG(
		animations.has(p_anim), "SpriteFrames already has animation '" + p_anim + "'.");

	animations[p_anim] = Anim();
}

bool SpriteFrames::has_animation(const String& p_anim) const { return animations.has(p_anim); }

void SpriteFrames::duplicate_animation(const String& p_from, const String& p_to)
{
	ERR_FAIL_COND_MSG(
		!animations.has(p_from), vformat("SpriteFrames doesn't have animation '%s'.", p_from));
	ERR_FAIL_COND_MSG(animations.has(p_to), vformat("Animation '%s' already exists.", p_to));
	animations[p_to] = animations[p_from];
}

void SpriteFrames::remove_animation(const String& p_anim) { animations.erase(p_anim); }

void SpriteFrames::rename_animation(const String& p_prev, const String& p_next)
{
	ERR_FAIL_COND_MSG(
		!animations.has(p_prev), "SpriteFrames doesn't have animation '" + String(p_prev) + "'.");
	ERR_FAIL_COND_MSG(animations.has(p_next), "Animation '" + String(p_next) + "' already exists.");

	Anim anim = animations[p_prev];
	animations.erase(p_prev);
	animations[p_next] = anim;
}

void SpriteFrames::get_animation_list(List<String>* r_animations) const
{
	for (const KeyValue<String, Anim>& E : animations) {
		r_animations->push_back(E.key);
	}
}

Vector<String> SpriteFrames::get_animation_names() const
{
	Vector<String> names;
	for (const KeyValue<String, Anim>& E : animations) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

void SpriteFrames::set_animation_speed(const String& p_anim, double p_fps)
{
	ERR_FAIL_COND_MSG(p_fps < 0, "Animation speed cannot be negative (" + itos(p_fps) + ").");
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	E->value.speed = p_fps;
}

double SpriteFrames::get_animation_speed(const String& p_anim) const
{
	HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(!E, 0, "Animation '" + String(p_anim) + "' doesn't exist.");
	return E->value.speed;
}

#ifndef DISABLE_DEPRECATED
void SpriteFrames::set_animation_loop(const String& p_anim, bool p_loop)
{
	set_animation_loop_mode(p_anim, p_loop ? LOOP_LINEAR : LOOP_NONE);
}

bool SpriteFrames::get_animation_loop(const String& p_anim) const
{
	return get_animation_loop_mode(p_anim) == LOOP_LINEAR;
}
#endif

void SpriteFrames::set_animation_loop_mode(const String& p_anim, LoopMode p_loop_mode)
{
	HashMap<String, Anim>::Iterator E = animations.find(p_anim);
	ERR_FAIL_COND_MSG(!E, "Animation '" + String(p_anim) + "' doesn't exist.");
	E->value.loop = p_loop_mode;
}

SpriteFrames::LoopMode SpriteFrames::get_animation_loop_mode(const String& p_anim) const
{
	HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
	ERR_FAIL_COND_V_MSG(
		!E, LoopMode::LOOP_NONE, "Animation '" + String(p_anim) + "' doesn't exist.");
	return E->value.loop;
}

SpriteFrames::SpriteFrames() { add_animation(SceneString(default_)); }

void SpriteFrames::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const {}
