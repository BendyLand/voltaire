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
	if (svg_data) {
		return HashMap<Color, Color>(svg_data->cmap);
	}
	return HashMap<Color, Color>();
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
// --- Inlined from scene/resources/external_texture.h ---
#include "scene/resources/texture.h"
#include "servers/camera/camera_server.h"

// External textures as defined by OES_EGL_image_external (GLES),
// VK_ANDROID_external_memory_android_hardware_buffer (Vulkan), or live CameraServer feeds.
class ExternalTexture final : public Texture2D
{
public:
	enum SourceMode
	{
		SOURCE_EXTERNAL_BUFFER,
		SOURCE_CAMERA_FEED,
	};

private:
	SourceMode source_mode = SOURCE_EXTERNAL_BUFFER;
	mutable RID texture;
	mutable bool using_placeholder = false;
	Size2 size = Size2(256, 256);
	uint64_t external_buffer = 0;

	int camera_feed_id = 0;
	CameraServer::FeedImage which_feed = CameraServer::FEED_RGBA_IMAGE;

	void _ensure_created() const;

public:
	void set_source_mode(SourceMode p_mode);
	SourceMode get_source_mode() const { return source_mode; }

	uint64_t get_external_texture_id() const;

	virtual Size2 get_size() const override;
	void set_size(const Size2& p_size);

	void set_external_buffer_id(uint64_t p_external_buffer);

	void set_camera_feed_id(int p_new_id);
	int get_camera_feed_id() const;

	void set_which_feed(CameraServer::FeedImage p_which);
	CameraServer::FeedImage get_which_feed() const;

	void set_camera_active(bool p_active);
	bool get_camera_active() const;

	virtual int get_width() const override;
	virtual int get_height() const override;

	virtual RID get_rid() const override;
	virtual bool has_alpha() const override;

	virtual Ref<Image> get_image() const override;

	explicit ExternalTexture(SourceMode p_mode = SOURCE_EXTERNAL_BUFFER);
	~ExternalTexture();
};

// --- End scene/resources/external_texture.h ---

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
// --- Inlined from scene/resources/drawable_texture_2d.h ---
#include "scene/resources/texture.h"

class Material;

class DrawableTexture2D final : public Texture2D
{
public:
	enum DrawableFormat
	{
		DRAWABLE_FORMAT_RGBA8,
		DRAWABLE_FORMAT_RGBA8_SRGB,
		DRAWABLE_FORMAT_RGBAH,
		DRAWABLE_FORMAT_RGBAF,
	};

private:
	mutable RID texture;
	int width = 64;
	int height = 64;
	bool mipmaps = false;
	DrawableFormat format = DRAWABLE_FORMAT_RGBA8;

	Color base_color = Color(1, 1, 1, 1);

	RID default_material;

	void _initialize();

protected:

public:
	int get_width() const override;
	int get_height() const override;

	void set_drawable_format(DrawableFormat p_format);
	DrawableFormat get_drawable_format() const;
	void set_use_mipmaps(bool p_mipmaps);
	bool get_use_mipmaps() const;

	virtual Image::Format get_format() const override;
	virtual RID get_rid() const override;

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;

	void setup(int p_width, int p_height, DrawableFormat p_format,
		const Color& p_modulate = Color(1, 1, 1, 1), bool p_use_mipmaps = false);

	void blit_rect(const Rect2i p_rect, const Ref<Texture2D>& p_source,
		const Color& p_modulate = Color(1, 1, 1, 1), int p_mipmap = 0,
		const Ref<Material>& p_material = Ref<Material>());

	virtual Ref<Image> get_image() const override;

	void generate_mipmaps();

	DrawableTexture2D();
	~DrawableTexture2D();
};



// --- End scene/resources/drawable_texture_2d.h ---
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

#include "scene/resources/material.h"

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
#include "scene/3d/visual_instance_3d.h"

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

// ============================================================================
// Merged from scene/resources/compressed_texture.cpp
// ============================================================================

#include "compressed_texture.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "scene/resources/bit_map.h"
#include "servers/rendering/renderer.h"

static const char* compression_mode_names[7] = {
	"Lossless", "Lossy", "Basis Universal", "S3TC", "ETC2", "BPTC", "ASTC"};

static CompressedTexture2D::CompressionMode get_expected_compression_mode(Image::Format format)
{
	if ((format >= Image::FORMAT_DXT1 && format <= Image::FORMAT_RGTC_RG) ||
		format == Image::FORMAT_DXT5_RA_AS_RG) {
		return CompressedTexture2D::COMPRESSION_MODE_S3TC;
	}
	else if (format >= Image::FORMAT_ETC && format <= Image::FORMAT_ETC2_RA_AS_RG) {
		return CompressedTexture2D::COMPRESSION_MODE_ETC2;
	}
	else if (format >= Image::FORMAT_BPTC_RGBA && format <= Image::FORMAT_BPTC_RGBFU) {
		return CompressedTexture2D::COMPRESSION_MODE_BPTC;
	}
	else if ((format >= Image::FORMAT_ASTC_4x4 && format <= Image::FORMAT_ASTC_8x8_HDR) ||
			 (format >= Image::FORMAT_ASTC_6x6 && format <= Image::FORMAT_ASTC_6x6_HDR)) {
		return CompressedTexture2D::COMPRESSION_MODE_ASTC;
	}
	ERR_FAIL_V(CompressedTexture2D::COMPRESSION_MODE_LOSSLESS);
}

Error CompressedTexture2D::_load_data(const String& p_path, int& r_width, int& r_height,
	Ref<Image>& image, bool& r_request_3d, bool& r_request_normal, bool& r_request_roughness,
	int& mipmap_limit, int p_size_limit)
{
	alpha_cache.unref();

	ERR_FAIL_COND_V(image.is_null(), ERR_INVALID_PARAMETER);

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != '2') {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is corrupt (Bad header).");
	}

	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}
	r_width = f->get_32();
	r_height = f->get_32();
	uint32_t df = f->get_32(); // data format

	// skip reserved
	mipmap_limit = int(f->get_32());
	// reserved
	f->get_32();
	f->get_32();
	f->get_32();

#ifdef TOOLS_ENABLED
	r_request_3d = request_3d_callback && df & FORMAT_BIT_DETECT_3D;
	r_request_roughness = request_roughness_callback && df & FORMAT_BIT_DETECT_ROUGNESS;
	r_request_normal = request_normal_callback && df & FORMAT_BIT_DETECT_NORMAL;
#else
	r_request_3d = false;
	r_request_roughness = false;
	r_request_normal = false;
#endif
	if (!(df & FORMAT_BIT_STREAM)) {
		p_size_limit = 0;
	}

	image = load_image_from_file(f, p_size_limit);

	if (image.is_null() || image->is_empty()) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

void CompressedTexture2D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

void CompressedTexture2D::_requested_3d(void* p_ud)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_3d_callback);
	request_3d_callback(ctex);
}

void CompressedTexture2D::_requested_roughness(
	void* p_ud, const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_roughness_channel)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_roughness_callback);
	request_roughness_callback(ctex, p_normal_path, p_roughness_channel);
}

void CompressedTexture2D::_requested_normal(void* p_ud)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_normal_callback);
	request_normal_callback(ctex);
}

CompressedTexture2D::TextureFormatRequestCallback CompressedTexture2D::request_3d_callback =
	nullptr;
CompressedTexture2D::TextureFormatRoughnessRequestCallback
	CompressedTexture2D::request_roughness_callback = nullptr;
CompressedTexture2D::TextureFormatRequestCallback CompressedTexture2D::request_normal_callback =
	nullptr;

bool CompressedTexture2D::keep_all_compressed_buffers = false;

void CompressedTexture2D::_set_data(const Vector<uint8_t>& p_data)
{
	if (p_data.is_empty()) {
		return; // nothing to do
	}

	const uint8_t* data = p_data.ptr();
	uint32_t data_size = p_data.size();
	ERR_FAIL_COND(data_size < 20);
	is_portable = true;
	compression_mode = CompressionMode(decode_uint16(data));
	PortableDataFormat data_format = PortableDataFormat(decode_uint16(data + 2));
	format = Image::Format(decode_uint32(data + 4));
	uint32_t mipmap_count = decode_uint32(data + 8);
	w = int(decode_uint32(data + 12));
	h = int(decode_uint32(data + 16));
	mipmaps = mipmap_count > 1;

	data += 20;
	data_size -= 20;

	Ref<Image> image;

	switch (compression_mode) {
	case COMPRESSION_MODE_LOSSLESS:
	case COMPRESSION_MODE_LOSSY: {
		ImageMemLoadFunc loader_func;
		if (data_format == PORTABLE_DATA_FORMAT_UNDEFINED) {
			loader_func = nullptr;
		}
		else if (data_format == PORTABLE_DATA_FORMAT_PNG) {
			loader_func = Image::_png_mem_unpacker_func;
		}
		else if (data_format == PORTABLE_DATA_FORMAT_WEBP) {
			loader_func = Image::_webp_mem_loader_func;
		}
		else {
			ERR_FAIL();
		}
		Vector<uint8_t> image_data;

		ERR_FAIL_COND(data_size < 4);
		for (uint32_t i = 0; i < mipmap_count; i++) {
			uint32_t mipsize = decode_uint32(data);
			data += 4;
			data_size -= 4;
			ERR_FAIL_COND(mipsize > data_size);
			Ref<Image> img = loader_func == nullptr ? memnew(Image(data, data_size))
													: Ref<Image>(loader_func(data, data_size));
			ERR_FAIL_COND(img->is_empty());
			if (img->get_format() != format) { // May happen due to webp/png in the tiny mipmaps.
				img->convert(format);
			}
			image_data.append_array(img->get_data());

			data += mipsize;
			data_size -= mipsize;
		}

		image.instantiate(w, h, mipmaps, format, image_data);
	} break;
	case COMPRESSION_MODE_BASIS_UNIVERSAL: {
		ERR_FAIL_NULL(Image::basis_universal_unpacker_ptr);
		image = Image::basis_universal_unpacker_ptr(data, data_size);
		format = image->get_format();
	} break;
	case COMPRESSION_MODE_S3TC:
	case COMPRESSION_MODE_ETC2:
	case COMPRESSION_MODE_BPTC:
	case COMPRESSION_MODE_ASTC: {
		image.instantiate(w, h, mipmaps, format, p_data.slice(20));
	} break;
	}
	ERR_FAIL_COND(image.is_null());

	if (texture.is_null()) {
		texture = Renderer::texture_2d_create(image);
	}
	else {
		RID new_texture = Renderer::texture_2d_create(image);
		Renderer::texture_replace(texture, new_texture);
	}

	image_stored = true;
	size_override = Size2(w, h);
	Renderer::texture_set_size_override(texture, size_override.width, size_override.height);
	alpha_cache.unref();

	if (keep_all_compressed_buffers || keep_compressed_buffer) {
		compressed_buffer = p_data;
	}
	else {
		compressed_buffer.clear();
	}
}

Vector<uint8_t> CompressedTexture2D::_get_data() const { return compressed_buffer; }

CompressedTexture2D::CompressionMode CompressedTexture2D::get_compression_mode() const
{
	return compression_mode;
}

void CompressedTexture2D::create_from_image(const Ref<Image>& p_image,
	CompressionMode p_compression_mode, bool p_normal_map, float p_lossy_quality)
{
	ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());

	Vector<uint8_t> buffer;

	buffer.resize(20);
	encode_uint16(p_compression_mode, buffer.ptrw());
	encode_uint16(PORTABLE_DATA_FORMAT_UNDEFINED, buffer.ptrw() + 2);
	encode_uint32(p_image->get_format(), buffer.ptrw() + 4);
	encode_uint32(p_image->get_mipmap_count() + 1, buffer.ptrw() + 8);
	encode_uint32(p_image->get_width(), buffer.ptrw() + 12);
	encode_uint32(p_image->get_height(), buffer.ptrw() + 16);

	switch (p_compression_mode) {
	case COMPRESSION_MODE_LOSSLESS:
	case COMPRESSION_MODE_LOSSY: {
		bool lossless_force_png =
			GLOBAL_GET_CACHED(bool, "rendering/textures/lossless_compression/force_png") ||
			!Image::_webp_mem_loader_func; // WebP module disabled.
		bool use_webp = !lossless_force_png && p_image->get_width() <= 16383 &&
						p_image->get_height() <= 16383; // WebP has a size limit.
		for (int i = 0; i < p_image->get_mipmap_count() + 1; i++) {
			Vector<uint8_t> data;
			if (p_compression_mode == COMPRESSION_MODE_LOSSY) {
				data = Image::webp_lossy_packer(
					i ? p_image->get_image_from_mipmap(i) : p_image, p_lossy_quality);
				encode_uint16(PORTABLE_DATA_FORMAT_WEBP, buffer.ptrw() + 2);
			}
			else {
				if (use_webp) {
					data = Image::webp_lossless_packer(
						i ? p_image->get_image_from_mipmap(i) : p_image);
					encode_uint16(PORTABLE_DATA_FORMAT_WEBP, buffer.ptrw() + 2);
				}
				else {
					data = Image::png_packer(i ? p_image->get_image_from_mipmap(i) : p_image);
					encode_uint16(PORTABLE_DATA_FORMAT_PNG, buffer.ptrw() + 2);
				}
			}
			int data_len = data.size();
			buffer.resize(buffer.size() + 4);
			encode_uint32(data_len, buffer.ptrw() + buffer.size() - 4);
			buffer.append_array(data);
		}
	} break;
	case COMPRESSION_MODE_BASIS_UNIVERSAL: {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND(p_image->is_compressed());
		encode_uint16(PORTABLE_DATA_FORMAT_BASIS_UNIVERSAL, buffer.ptrw() + 2);
		Image::UsedChannels uc = p_image->detect_used_channels(
			p_normal_map ? Image::COMPRESS_SOURCE_NORMAL : Image::COMPRESS_SOURCE_GENERIC);
		Vector<uint8_t> budata = Image::basis_universal_packer(p_image, uc, basisu_params);
		buffer.append_array(budata);
#else
		ERR_FAIL_MSG("Basis Universal compression can only run in editor build.");
#endif
	} break;
	case COMPRESSION_MODE_S3TC:
	case COMPRESSION_MODE_ETC2:
	case COMPRESSION_MODE_BPTC:
	case COMPRESSION_MODE_ASTC: {
		encode_uint16(PORTABLE_DATA_FORMAT_IMAGE, buffer.ptrw() + 2);
		Ref<Image> copy = p_image;
		if (p_image->is_compressed()) {
			CompressionMode expected_compression_mode =
				get_expected_compression_mode(p_image->get_format());
			ERR_FAIL_COND_MSG(expected_compression_mode != p_compression_mode,
				vformat("Mismatched compression mode for image format %s, expected %s, got %s.",
					Image::get_format_name(p_image->get_format()),
					compression_mode_names[expected_compression_mode],
					compression_mode_names[p_compression_mode]));
		}
		encode_uint32(copy->get_format(), buffer.ptrw() + 4);
		buffer.append_array(copy->get_data());
	} break;
	}

	_set_data(buffer);
}

void CompressedTexture2D::update(const Ref<Image>& p_image)
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
	alpha_cache.unref();
	image_stored = true;
	emit_changed();
}

void CompressedTexture2D::set_size_override(const Size2& p_size)
{
	size_override = p_size;
	if (p_size.width > 0) {
		w = int(p_size.width);
	}
	if (p_size.height > 0) {
		h = int(p_size.height);
	}
	Renderer::texture_set_size_override(texture, size_override.width, size_override.height);
}

Size2 CompressedTexture2D::get_size_override() const { return size_override; }

void CompressedTexture2D::set_keep_all_compressed_buffers(bool p_keep)
{
	keep_all_compressed_buffers = p_keep;
}

bool CompressedTexture2D::is_keeping_all_compressed_buffers()
{
	return keep_all_compressed_buffers;
}

void CompressedTexture2D::set_keep_compressed_buffer(bool p_keep)
{
	keep_compressed_buffer = p_keep;
	if (!p_keep) {
		compressed_buffer.clear();
	}
}

bool CompressedTexture2D::is_keeping_compressed_buffer() const
{
	return keep_compressed_buffer;
}

void CompressedTexture2D::set_basisu_compressor_params(
	int p_uastc_level, float p_rdo_quality_loss)
{
	basisu_params.uastc_level = p_uastc_level;
	basisu_params.rdo_quality_loss = p_rdo_quality_loss;
}

Image::Format CompressedTexture2D::get_format() const { return format; }

String CompressedTexture2D::get_load_path() const { return path_to_file; }

int CompressedTexture2D::get_width() const { return w; }

int CompressedTexture2D::get_height() const { return h; }

RID CompressedTexture2D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

void CompressedTexture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(w, h)), texture, false, p_modulate, p_transpose);
}

void CompressedTexture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void CompressedTexture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool CompressedTexture2D::is_pixel_opaque(int p_x, int p_y) const
{
	if (alpha_cache.is_valid()) {
		int aw = int(alpha_cache->get_size().width);
		int ah = int(alpha_cache->get_size().height);
		if (aw == 0 || ah == 0 || w == 0 || h == 0) {
			return true;
		}

		int x = p_x * aw / w;
		int y = p_y * ah / h;

		x = CLAMP(x, 0, aw - 1);
		y = CLAMP(y, 0, ah - 1);

		return alpha_cache->get_bit(x, y);
	}

	return true;
}

bool CompressedTexture2D::has_alpha() const
{
	if (is_portable) {
		return (format == Image::FORMAT_LA8 || format == Image::FORMAT_RGBA8);
	}
	return false;
}

Ref<Image> CompressedTexture2D::get_image() const
{
	if (is_portable && !image_stored) {
		return Ref<Image>();
	}
	if (texture.is_valid()) {
		return Renderer::texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

void CompressedTexture2D::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}

	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}

	load(path);
}

Error CompressedTexture2D::load(const String& p_path)
{
	int lw, lh;
	Ref<Image> image;
	image.instantiate();

	bool request_3d = false;
	bool request_normal = false;
	bool request_roughness = false;
	int mipmap_limit = 0;

	Error err = _load_data(p_path, lw, lh, image, request_3d, request_normal, request_roughness,
		mipmap_limit);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_2d_create(image);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_2d_create(image);
	}
	if (lw || lh) {
		Renderer::texture_set_size_override(texture, lw, lh);
	}

	w = lw;
	h = lh;
	format = image->get_format();
	mipmaps = image->has_mipmaps();
	is_portable = false;
	image_stored = true;
	alpha_cache.unref();

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

Ref<Image> CompressedTexture2D::load_image_from_file(Ref<FileAccess> f, int p_size_limit)
{
	uint32_t data_format = f->get_32();
	uint32_t w = f->get_16();
	uint32_t h = f->get_16();
	uint32_t mipmaps = f->get_32();
	Image::Format format = Image::Format(f->get_32());

	if (data_format == DATA_FORMAT_PNG || data_format == DATA_FORMAT_WEBP) {
		// look for a PNG or WebP file inside

		int sw = w;
		int sh = h;

		// mipmaps need to be read independently, they will be later combined
		Vector<Ref<Image>> mipmap_images;
		uint64_t total_size = 0;

		for (uint32_t i = 0; i < mipmaps + 1; i++) {
			uint32_t size = f->get_32();

			if (p_size_limit > 0 && i < (mipmaps - 1) && (sw > p_size_limit || sh > p_size_limit)) {
				// can't load this due to size limit
				sw = MAX(sw >> 1, 1);
				sh = MAX(sh >> 1, 1);
				f->seek(f->get_position() + size);
				continue;
			}

			Vector<uint8_t> pv;
			pv.resize(size);
			{
				uint8_t* wr = pv.ptrw();
				f->get_buffer(wr, size);
			}

			Ref<Image> img;
			if (data_format == DATA_FORMAT_PNG && Image::png_unpacker) {
				img = Image::png_unpacker(pv);
			}
			else if (data_format == DATA_FORMAT_WEBP && Image::webp_unpacker) {
				img = Image::webp_unpacker(pv);
			}

			if (img.is_null() || img->is_empty()) {
				ERR_FAIL_COND_V(img.is_null() || img->is_empty(), Ref<Image>());
			}
			// If the image is compressed and its format doesn't match the desired format, return an
			// empty reference. This is done to avoid recompressing the image on load.
			ERR_FAIL_COND_V(img->is_compressed() && format != img->get_format(), Ref<Image>());

			// The format will actually be the format of the header,
			// as it may have changed on compression.
			if (format != img->get_format()) {
				// Convert the image to the desired format.
				// Note: We are not decompressing the image here, just changing its format.
				// It's important that all images in the texture array share the same format for
				// correct rendering.
				img->convert(format);
			}

			total_size += img->get_data().size();

			mipmap_images.push_back(img);

			sw = MAX(sw >> 1, 1);
			sh = MAX(sh >> 1, 1);
		}

		Ref<Image> image;
		image.instantiate();

		if (mipmap_images.size() == 1) {
			// only one image (which will most likely be the case anyway for this format)
			image = mipmap_images[0];
			return image;
		}
		else {
			// rarer use case, but needs to be supported
			Vector<uint8_t> img_data;
			img_data.resize(total_size);

			{
				uint8_t* wr = img_data.ptrw();

				int ofs = 0;
				for (int i = 0; i < mipmap_images.size(); i++) {
					Vector<uint8_t> id = mipmap_images[i]->get_data();
					int len = id.size();
					const uint8_t* r = id.ptr();
					memcpy(&wr[ofs], r, len);
					ofs += len;
				}
			}

			image->set_data(w, h, true, mipmap_images[0]->get_format(), img_data);
			return image;
		}
	}
	else if (data_format == DATA_FORMAT_BASIS_UNIVERSAL) {
		int sw = w;
		int sh = h;
		uint32_t size = f->get_32();
		if (p_size_limit > 0 && (sw > p_size_limit || sh > p_size_limit)) {
			// can't load this due to size limit
			sw = MAX(sw >> 1, 1);
			sh = MAX(sh >> 1, 1);
			f->seek(f->get_position() + size);
			return Ref<Image>();
		}
		Vector<uint8_t> pv;
		pv.resize(size);
		{
			uint8_t* wr = pv.ptrw();
			f->get_buffer(wr, size);
		}
		Ref<Image> img;
		img = Image::basis_universal_unpacker(pv);
		if (img.is_null() || img->is_empty()) {
			ERR_FAIL_COND_V(img.is_null() || img->is_empty(), Ref<Image>());
		}
		format = img->get_format();
		sw = MAX(sw >> 1, 1);
		sh = MAX(sh >> 1, 1);
		return img;
	}
	else if (data_format == DATA_FORMAT_IMAGE) {
		int size = Image::get_image_data_size(w, h, format, mipmaps ? true : false);

		for (uint32_t i = 0; i < mipmaps + 1; i++) {
			int tw, th;
			int ofs = Image::get_image_mipmap_offset_and_dimensions(w, h, format, i, tw, th);

			if (p_size_limit > 0 && i < mipmaps && (p_size_limit > tw || p_size_limit > th)) {
				if (ofs) {
					f->seek(f->get_position() + ofs);
				}
				continue; // oops, size limit enforced, go to next
			}

			Vector<uint8_t> data;
			data.resize(size - ofs);

			{
				uint8_t* wr = data.ptrw();
				f->get_buffer(wr, data.size());
			}

			Ref<Image> image =
				Image::create_from_data(tw, th, mipmaps - i ? true : false, format, data);

			return image;
		}
	}

	return Ref<Image>();
}

CompressedTexture2D::~CompressedTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void CompressedTexture3D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

Image::Format CompressedTexture3D::get_format() const { return format; }

Error CompressedTexture3D::_load_data(const String& p_path, Vector<Ref<Image>>& r_data,
	Image::Format& r_format, int& r_width, int& r_height, int& r_depth, bool& r_mipmaps)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	ERR_FAIL_COND_V(header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != 'L',
		ERR_FILE_UNRECOGNIZED);

	// stored as compressed textures (used for lossless and lossy compression)
	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}

	r_depth = f->get_32(); // depth
	f->get_32();		   // ignored (mode)
	f->get_32();		   // ignored (data format)

	f->get_32(); // ignored
	int mipmap_count = f->get_32();
	f->get_32(); // ignored
	f->get_32(); // ignored

	r_mipmaps = mipmap_count != 0;

	r_data.clear();

	for (int i = 0; i < (r_depth + mipmap_count); i++) {
		Ref<Image> image = CompressedTexture2D::load_image_from_file(f, 0);
		ERR_FAIL_COND_V(image.is_null() || image->is_empty(), ERR_CANT_OPEN);
		if (i == 0) {
			r_format = image->get_format();
			r_width = image->get_width();
			r_height = image->get_height();
		}
		r_data.push_back(image);
	}

	return OK;
}

Error CompressedTexture3D::load(const String& p_path)
{
	Vector<Ref<Image>> data;
	Image::Format r_format;
	int r_width, r_height, r_depth;
	bool r_mipmaps;

	Error err = _load_data(p_path, data, r_format, r_width, r_height, r_depth, r_mipmaps);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	RID new_tex = Renderer::texture_3d_create(r_format, r_width, r_height, r_depth, r_mipmaps, data);
	ERR_FAIL_COND_V(new_tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, new_tex);
	}
	else {
		texture = new_tex;
	}

	format = r_format;
	w = r_width;
	h = r_height;
	d = r_depth;
	mipmaps = r_mipmaps;

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

String CompressedTexture3D::get_load_path() const { return path_to_file; }

int CompressedTexture3D::get_width() const { return w; }

int CompressedTexture3D::get_height() const { return h; }

int CompressedTexture3D::get_depth() const { return d; }

bool CompressedTexture3D::has_mipmaps() const { return mipmaps; }

RID CompressedTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}
	return texture;
}

Vector<Ref<Image>> CompressedTexture3D::get_data() const
{
	if (texture.is_valid()) {
		return Renderer::texture_3d_get(texture);
	}
	else {
		return Vector<Ref<Image>>();
	}
}

void CompressedTexture3D::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}

	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}

	load(path);
}

CompressedTexture3D::~CompressedTexture3D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void CompressedTextureLayered::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

Image::Format CompressedTextureLayered::get_format() const { return format; }

Error CompressedTextureLayered::_load_data(
	const String& p_path, Vector<Ref<Image>>& images, int& mipmap_limit, int p_size_limit)
{
	ERR_FAIL_COND_V(images.size() != 0, ERR_INVALID_PARAMETER);

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != 'L') {
		ERR_FAIL_V_MSG(
			ERR_FILE_CORRUPT, "Compressed texture layered file is corrupt (Bad header).");
	}

	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}

	uint32_t layer_count = f->get_32(); // layer count
	uint32_t type = f->get_32();		// layer type
	ERR_FAIL_COND_V((int)type != layered_type, ERR_INVALID_DATA);

	uint32_t df = f->get_32(); // data format
	mipmap_limit = int(f->get_32());
	// reserved
	f->get_32();
	f->get_32();
	f->get_32();

	if (!(df & FORMAT_BIT_STREAM)) {
		p_size_limit = 0;
	}

	images.resize(layer_count);

	for (uint32_t i = 0; i < layer_count; i++) {
		Ref<Image> image = CompressedTexture2D::load_image_from_file(f, p_size_limit);
		ERR_FAIL_COND_V(image.is_null() || image->is_empty(), ERR_CANT_OPEN);
		images.write[i] = image;
	}

	return OK;
}

Error CompressedTextureLayered::load(const String& p_path)
{
	Vector<Ref<Image>> images;
	int mipmap_limit = 0;

	Error err = _load_data(p_path, images, mipmap_limit);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	RID new_tex =
		Renderer::texture_2d_layered_create(images, RSE::TextureLayeredType(layered_type));
	ERR_FAIL_COND_V(new_tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, new_tex);
	}
	else {
		texture = new_tex;
	}

	w = images[0]->get_width();
	h = images[0]->get_height();
	mipmaps = images[0]->has_mipmaps();
	format = images[0]->get_format();
	layers = images.size();

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

String CompressedTextureLayered::get_load_path() const { return path_to_file; }

int CompressedTextureLayered::get_width() const { return w; }

int CompressedTextureLayered::get_height() const { return h; }

int CompressedTextureLayered::get_layers() const { return layers; }

bool CompressedTextureLayered::has_mipmaps() const { return mipmaps; }

TextureLayered::LayeredType CompressedTextureLayered::get_layered_type() const
{
	return layered_type;
}

RID CompressedTextureLayered::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_layered_placeholder_create(
			RSE::TextureLayeredType(layered_type));
	}
	return texture;
}

Ref<Image> CompressedTextureLayered::get_layer_data(int p_layer) const
{
	if (texture.is_valid()) {
		ERR_FAIL_INDEX_V(p_layer, get_layers(), Ref<Image>());
		return Renderer::texture_2d_layer_get(texture, p_layer);
	}
	else {
		return Ref<Image>();
	}
}

void CompressedTextureLayered::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}
	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}
	load(path);
}

CompressedTextureLayered::CompressedTextureLayered(LayeredType p_type) { layered_type = p_type; }

CompressedTextureLayered::~CompressedTextureLayered()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

// --- compressed_texture_resource_format.cpp ---

#include "scene/resources/compressed_texture.h"

#include "scene/resources/compressed_texture.h"

Ref<Resource> ResourceFormatLoaderCompressedTexture::load(const String& p_path,
	const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	CacheMode p_cache_mode)
{
	if (p_path.has_extension("ctex")) {
		Ref<CompressedTexture2D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		Ref<CompressedTextureLayered> ct;
		if (p_path.has_extension("ctexarray")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_2D_ARRAY));
		}
		else if (p_path.has_extension("ccube")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP));
		}
		else {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY));
		}
		Error err = ct->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return ct;
	}

	if (p_path.has_extension("ctex3d")) {
		Ref<CompressedTexture3D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	return Ref<Resource>();
}

void ResourceFormatLoaderCompressedTexture::get_recognized_extensions(
	List<String>* p_extensions) const
{
	p_extensions->push_back("ctex");
	p_extensions->push_back("ctexarray");
	p_extensions->push_back("ccube");
	p_extensions->push_back("ccubearray");
	p_extensions->push_back("ctex3d");
}

bool ResourceFormatLoaderCompressedTexture::handles_type(const String& p_type) const
{
	return p_type == "CompressedTexture2D" || p_type == "CompressedTextureLayered" ||
		   p_type == "CompressedTexture3D";
}

String ResourceFormatLoaderCompressedTexture::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ctex")) {
		return "CompressedTexture2D";
	}
	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		return "CompressedTextureLayered";
	}
	if (p_path.has_extension("ctex3d")) {
		return "CompressedTexture3D";
	}
	return "";
}

// ============================================================================
// Merged from scene/resources/curve.cpp
// ============================================================================

#include <cfloat> // FLT_EPSILON
#include "core/math/math_funcs.h"
#include "curve.h"

const char* Curve::SIGNAL_RANGE_CHANGED = "range_changed";
const char* Curve::SIGNAL_DOMAIN_CHANGED = "domain_changed";

int Curve::_add_point(Vector2 p_position, real_t p_left_tangent, real_t p_right_tangent,
	TangentMode p_left_mode, TangentMode p_right_mode, bool p_mark_dirty)
{
	// Add a point and preserve order.

	// Points must remain within the given value and domain ranges.
	p_position.x = CLAMP(p_position.x, _min_domain, _max_domain);
	p_position.y = CLAMP(p_position.y, _min_value, _max_value);

	int ret = -1;

	if (_points.is_empty()) {
		_points.push_back(
			Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
		ret = 0;

	}
	else if (_points.size() == 1) {
		// TODO Is the `else` able to handle this block already?

		real_t diff = p_position.x - _points[0].position.x;

		if (diff > 0) {
			_points.push_back(
				Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 1;
		}
		else {
			_points.insert(
				0, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 0;
		}

	}
	else {
		int i = get_index(p_position.x);

		if (i == 0 && p_position.x < _points[0].position.x) {
			// Insert before anything else.
			_points.insert(
				0, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 0;
		}
		else {
			// Insert between i and i+1.
			++i;
			_points.insert(
				i, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = i;
		}
	}

	update_auto_tangents(ret);

	if (p_mark_dirty) {
		mark_dirty();
	}

	return ret;
}

// TODO: Needed to make the curve editor function properly until
// https://github.com/godotengine/godot/issues/76985 is fixed.
int Curve::add_point_no_update(Vector2 p_position, real_t p_left_tangent, real_t p_right_tangent,
	TangentMode p_left_mode, TangentMode p_right_mode)
{
	int ret = _add_point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode);

	return ret;
}

int Curve::get_index(real_t p_offset) const
{
	// Lower-bound float binary search.

	int imin = 0;
	int imax = _points.size() - 1;

	while (imax - imin > 1) {
		int m = (imin + imax) / 2;

		real_t a = _points[m].position.x;
		real_t b = _points[m + 1].position.x;

		if (a < p_offset && b < p_offset) {
			imin = m;
		}
		else if (a > p_offset) {
			imax = m;
		}
		else {
			return m;
		}
	}

	// Will happen if the offset is out of bounds.
	if (p_offset > _points[imax].position.x) {
		return imax;
	}
	return imin;
}

void Curve::clean_dupes()
{
	bool dirty = false;

	for (uint32_t i = 1; i < _points.size(); ++i) {
		real_t diff = _points[i - 1].position.x - _points[i].position.x;
		if (diff <= CMP_EPSILON) {
			_points.remove_at(i);
			--i;
			dirty = true;
		}
	}

	if (dirty) {
		mark_dirty();
	}
}

void Curve::set_point_left_tangent(int p_index, real_t p_tangent)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points[p_index].left_tangent = p_tangent;
	_points[p_index].left_mode = TANGENT_FREE;
	mark_dirty();
}

void Curve::set_point_right_tangent(int p_index, real_t p_tangent)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points[p_index].right_tangent = p_tangent;
	_points[p_index].right_mode = TANGENT_FREE;
	mark_dirty();
}

void Curve::set_point_left_mode(int p_index, TangentMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points[p_index].left_mode = p_mode;
	if (p_index > 0) {
		if (p_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - _points[p_index].position).normalized();
			_points[p_index].left_tangent = v.y / v.x;
		}
	}
	mark_dirty();
}

void Curve::set_point_right_mode(int p_index, TangentMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points[p_index].right_mode = p_mode;
	if ((uint32_t)p_index + 1 < _points.size()) {
		if (p_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - _points[p_index].position).normalized();
			_points[p_index].right_tangent = v.y / v.x;
		}
	}
	mark_dirty();
}

real_t Curve::get_point_left_tangent(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), 0);
	return _points[p_index].left_tangent;
}

real_t Curve::get_point_right_tangent(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), 0);
	return _points[p_index].right_tangent;
}

Curve::TangentMode Curve::get_point_left_mode(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), TANGENT_FREE);
	return _points[p_index].left_mode;
}

Curve::TangentMode Curve::get_point_right_mode(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), TANGENT_FREE);
	return _points[p_index].right_mode;
}

void Curve::_remove_point(int p_index, bool p_mark_dirty)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points.remove_at(p_index);
	if (p_mark_dirty) {
		mark_dirty();
	}
}

void Curve::_set_point_position(int p_index, const Vector2& p_position)
{
	// Set the value first, setting the offset might change the index.
	set_point_value(p_index, p_position.y);
	set_point_offset(p_index, p_position.x);
}

int Curve::add_point(Vector2 p_position, real_t p_left_tangent, real_t p_right_tangent,
	TangentMode p_left_mode, TangentMode p_right_mode)
{
	return 0;
}

void Curve::set_point_value(int p_index, real_t p_position)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, _points.size());
	_points[p_index].position.y = p_position;
	update_auto_tangents(p_index);
	mark_dirty();
}

int Curve::set_point_offset(int p_index, real_t p_offset)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), -1);
	Point p = _points[p_index];
	_remove_point(p_index, false);
	int i = _add_point(Vector2(p_offset, p.position.y), p.left_tangent, p.right_tangent,
		p.left_mode, p.right_mode, false);
	if (p_index != i) {
		update_auto_tangents(p_index);
	}
	update_auto_tangents(i);
	mark_dirty();
	return i;
}

Vector2 Curve::get_point_position(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), Vector2(0, 0));
	return _points[p_index].position;
}

Curve::Point Curve::get_point(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, _points.size(), Point());
	return _points[p_index];
}

void Curve::update_auto_tangents(int p_index)
{
	Point& p = _points[p_index];

	if (p_index > 0) {
		if (p.left_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - p.position).normalized();
			p.left_tangent = v.y / v.x;
		}
		if (_points[p_index - 1].right_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - p.position).normalized();
			_points[p_index - 1].right_tangent = v.y / v.x;
		}
	}

	if ((uint32_t)p_index + 1 < _points.size()) {
		if (p.right_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - p.position).normalized();
			p.right_tangent = v.y / v.x;
		}
		if (_points[p_index + 1].left_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - p.position).normalized();
			_points[p_index + 1].left_tangent = v.y / v.x;
		}
	}
}

#define MIN_X_RANGE 0.01
#define MIN_Y_RANGE 0.01

real_t Curve::sample(real_t p_offset) const
{
	if (_points.is_empty()) {
		return 0;
	}
	if (_points.size() == 1) {
		return _points[0].position.y;
	}

	uint32_t i = get_index(p_offset);

	if (i == _points.size() - 1) {
		return _points[i].position.y;
	}

	real_t local = p_offset - _points[i].position.x;

	if (i == 0 && local <= 0) {
		return _points[0].position.y;
	}

	return sample_local_nocheck(i, local);
}

real_t Curve::sample_local_nocheck(int p_index, real_t p_local_offset) const
{
	const Point a = _points[p_index];
	const Point b = _points[p_index + 1];

	/* Cubic bézier
	 *
	 *       ac-----bc
	 *      /         \
	 *     /           \     Here with a.right_tangent > 0
	 *    /             \    and b.left_tangent < 0
	 *   /               \
	 *  a                 b
	 *
	 *  |-d1--|-d2--|-d3--|
	 *
	 * d1 == d2 == d3 == d / 3
	 */

	// Control points are chosen at equal distances.
	real_t d = b.position.x - a.position.x;
	if (Math::is_zero_approx(d)) {
		return b.position.y;
	}
	p_local_offset /= d;
	d /= 3.0;
	real_t yac = a.position.y + d * a.right_tangent;
	real_t ybc = b.position.y - d * b.left_tangent;

	real_t y = Math::bezier_interpolate(a.position.y, yac, ybc, b.position.y, p_local_offset);

	return y;
}

bool Curve::_filter_property(const String& p_name, int p_index) const
{
	if (p_index == 0) {
		return p_name != "left_tangent" && p_name != "left_mode";
	}
	if (p_index == get_point_count() - 1) {
		return p_name != "right_tangent" && p_name != "right_mode";
	}
	return true;
}

void Curve::mark_dirty()
{
	_baked_cache_dirty = true;
	emit_changed();
}

void Curve::bake() { _bake(); }

void Curve::_bake() const
{
	_baked_cache.clear();

	_baked_cache.resize(_bake_resolution);

	for (int i = 1; i < _bake_resolution - 1; ++i) {
		real_t x = get_domain_range() * i / static_cast<real_t>(_bake_resolution - 1) + _min_domain;
		real_t y = sample(x);
		_baked_cache.write[i] = y;
	}

	if (_points.size() != 0) {
		_baked_cache.write[0] = _points[0].position.y;
		_baked_cache.write[_baked_cache.size() - 1] = _points[_points.size() - 1].position.y;
	}

	_baked_cache_dirty = false;
}

void Curve::set_bake_resolution(int p_resolution)
{
	ERR_FAIL_COND(p_resolution < 1);
	ERR_FAIL_COND(p_resolution > 1000);
	_bake_resolution = p_resolution;
	_baked_cache_dirty = true;
}

real_t Curve::sample_baked(real_t p_offset) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), 0, "Offset is non-finite");

	if (_baked_cache_dirty) {
		// Last-second bake if not done already.
		_bake();
	}

	// Special cases if the cache is too small.
	if (_baked_cache.is_empty()) {
		if (_points.is_empty()) {
			return 0;
		}
		return _points[0].position.y;
	}
	else if (_baked_cache.size() == 1) {
		return _baked_cache[0];
	}

	// Get interpolation index.
	real_t fi = (p_offset - _min_domain) / get_domain_range() * (_baked_cache.size() - 1);
	int i = Math::floor(fi);
	if (i < 0) {
		i = 0;
		fi = 0;
	}
	else if (i >= _baked_cache.size()) {
		i = _baked_cache.size() - 1;
		fi = 0;
	}

	// Sample.
	if (i + 1 < _baked_cache.size()) {
		real_t t = fi - i;
		return Math::lerp(_baked_cache[i], _baked_cache[i + 1], t);
	}
	else {
		return _baked_cache[_baked_cache.size() - 1];
	}
}

void Curve::ensure_default_setup(real_t p_min, real_t p_max)
{
	if (_points.is_empty() && _min_value == 0 && _max_value == 1) {
		add_point(Vector2(0, 1));
		add_point(Vector2(1, 1));
		set_min_value(p_min);
		set_max_value(p_max);
	}
}

int Curve2D::get_point_count() const { return points.size(); }

void Curve2D::_add_point(
	const Vector2& p_position, const Vector2& p_in, const Vector2& p_out, int p_atpos)
{
	Point n;
	n.position = p_position;
	n.in = p_in;
	n.out = p_out;
	if ((uint32_t)p_atpos < points.size()) {
		points.insert(p_atpos, n);
	}
	else {
		points.push_back(n);
	}

	mark_dirty();
}

void Curve2D::set_point_position(int p_index, const Vector2& p_position)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].position = p_position;
	mark_dirty();
}

Vector2 Curve2D::get_point_position(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector2());
	return points[p_index].position;
}

void Curve2D::set_point_in(int p_index, const Vector2& p_in)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].in = p_in;
	mark_dirty();
}

Vector2 Curve2D::get_point_in(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector2());
	return points[p_index].in;
}

void Curve2D::set_point_out(int p_index, const Vector2& p_out)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].out = p_out;
	mark_dirty();
}

Vector2 Curve2D::get_point_out(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector2());
	return points[p_index].out;
}

void Curve2D::_remove_point(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());
	points.remove_at(p_index);
	mark_dirty();
}

Vector2 Curve2D::sample(int p_index, const real_t p_offset) const
{
	int pc = points.size();
	ERR_FAIL_COND_V(pc == 0, Vector2());

	if (p_index >= pc - 1) {
		return points[pc - 1].position;
	}
	else if (p_index < 0) {
		return points[0].position;
	}

	Vector2 p0 = points[p_index].position;
	Vector2 p1 = p0 + points[p_index].out;
	Vector2 p3 = points[p_index + 1].position;
	Vector2 p2 = p3 + points[p_index + 1].in;

	return p0.bezier_interpolate(p1, p2, p3, p_offset);
}

Vector2 Curve2D::samplef(real_t p_findex) const
{
	if (p_findex < 0) {
		p_findex = 0;
	}
	else if (p_findex >= points.size()) {
		p_findex = points.size();
	}

	return sample((int)p_findex, Math::fmod(p_findex, (real_t)1.0));
}

void Curve2D::mark_dirty()
{
	baked_cache_dirty = true;
	emit_changed();
}

void Curve2D::_bake_segment2d(RBMap<real_t, Vector2>& r_bake, real_t p_begin, real_t p_end,
	const Vector2& p_a, const Vector2& p_out, const Vector2& p_b, const Vector2& p_in, int p_depth,
	int p_max_depth, real_t p_tol) const
{
	real_t mp = p_begin + (p_end - p_begin) * 0.5;
	Vector2 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector2 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
	Vector2 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	Vector2 na = (mid - beg).normalized();
	Vector2 nb = (end - mid).normalized();
	real_t dp = na.dot(nb);

	if (dp < Math::cos(Math::deg_to_rad(p_tol))) {
		r_bake[mp] = mid;
	}

	if (p_depth < p_max_depth) {
		_bake_segment2d(
			r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
		_bake_segment2d(r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
	}
}

void Curve2D::_bake_segment2d_even_length(RBMap<real_t, Vector2>& r_bake, real_t p_begin,
	real_t p_end, const Vector2& p_a, const Vector2& p_out, const Vector2& p_b, const Vector2& p_in,
	int p_depth, int p_max_depth, real_t p_length) const
{
	Vector2 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector2 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	real_t length = beg.distance_to(end);

	if (length > p_length && p_depth < p_max_depth) {
		real_t mp = (p_begin + p_end) * 0.5;
		Vector2 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
		r_bake[mp] = mid;

		_bake_segment2d_even_length(
			r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_length);
		_bake_segment2d_even_length(
			r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_length);
	}
}

Vector2 Curve2D::_calculate_tangent(const Vector2& p_begin, const Vector2& p_control_1,
	const Vector2& p_control_2, const Vector2& p_end, const real_t p_t)
{
	// Handle corner cases.
	if (Math::is_zero_approx(p_t - 0.0f)) {
		if (p_control_1.is_equal_approx(p_begin)) {
			if (p_control_1.is_equal_approx(p_control_2)) {
				return (p_end - p_begin).normalized();
			}
			else {
				return (p_control_2 - p_begin).normalized();
			}
		}
	}
	else if (Math::is_zero_approx(p_t - 1.0f)) {
		if (p_control_2.is_equal_approx(p_end)) {
			if (p_control_2.is_equal_approx(p_control_1)) {
				return (p_end - p_begin).normalized();
			}
			else {
				return (p_end - p_control_1).normalized();
			}
		}
	}

	if (p_control_1.is_equal_approx(p_end) && p_control_2.is_equal_approx(p_begin)) {
		return (p_end - p_begin).normalized();
	}

	return p_begin.bezier_derivative(p_control_1, p_control_2, p_end, p_t).normalized();
}

void Curve2D::_bake() const
{
	if (!baked_cache_dirty) {
		return;
	}

	baked_max_ofs = 0;
	baked_cache_dirty = false;

	if (points.is_empty()) {
		baked_point_cache.clear();
		baked_dist_cache.clear();
		baked_forward_vector_cache.clear();
		return;
	}

	if (points.size() == 1) {
		baked_point_cache.resize(1);
		baked_point_cache.set(0, points[0].position);
		baked_dist_cache.resize(1);
		baked_dist_cache.set(0, 0.0);
		baked_forward_vector_cache.resize(1);
		baked_forward_vector_cache.set(0, Vector2(0.0, 0.1));

		return;
	}

	// Tessellate curve to (almost) even length segments.
	{
		Vector<RBMap<real_t, Vector2>> midpoints = _tessellate_even_length(10, bake_interval);

		int pc = 1;
		for (uint32_t i = 0; i < points.size() - 1; i++) {
			pc++;
			pc += midpoints[i].size();
		}

		baked_point_cache.resize(pc);
		baked_dist_cache.resize(pc);
		baked_forward_vector_cache.resize(pc);

		Vector2* bpw = baked_point_cache.ptrw();
		Vector2* bfw = baked_forward_vector_cache.ptrw();

		// Collect positions and sample tilts and tangents for each baked points.
		bpw[0] = points[0].position;
		bfw[0] = _calculate_tangent(points[0].position, points[0].position + points[0].out,
			points[1].position + points[1].in, points[1].position, 0.0);
		int pidx = 0;

		for (uint32_t i = 0; i < points.size() - 1; i++) {
			for (const KeyValue<real_t, Vector2>& E : midpoints[i]) {
				pidx++;
				bpw[pidx] = E.value;
				bfw[pidx] =
					_calculate_tangent(points[i].position, points[i].position + points[i].out,
						points[i + 1].position + points[i + 1].in, points[i + 1].position, E.key);
			}

			pidx++;
			bpw[pidx] = points[i + 1].position;
			bfw[pidx] = _calculate_tangent(points[i].position, points[i].position + points[i].out,
				points[i + 1].position + points[i + 1].in, points[i + 1].position, 1.0);
		}

		// Recalculate the baked distances.
		real_t* bdw = baked_dist_cache.ptrw();
		bdw[0] = 0.0;
		for (int i = 0; i < pc - 1; i++) {
			bdw[i + 1] = bdw[i] + bpw[i].distance_to(bpw[i + 1]);
		}
		baked_max_ofs = bdw[pc - 1];
	}
}

real_t Curve2D::get_baked_length() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_max_ofs;
}

Curve2D::Interval Curve2D::_find_interval(real_t p_offset) const
{
	Interval interval = {-1, 0.0};
	ERR_FAIL_COND_V_MSG(baked_cache_dirty, interval, "Backed cache is dirty");

	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc < 2, interval, "Less than two points in cache");

	int start = 0;
	int end = pc;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		}
		else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(
		p_offset < offset_begin || p_offset > offset_end, interval, "Offset out of range.");

	interval.idx = idx;
	if (idx_interval < FLT_EPSILON) {
		interval.frac = 0.5; // For a very short interval, 0.5 is a reasonable choice.
		ERR_FAIL_V_MSG(interval, "Zero length interval.");
	}

	interval.frac = (p_offset - offset_begin) / idx_interval;
	return interval;
}

Vector2 Curve2D::_sample_baked(Interval p_interval, bool p_cubic) const
{
	// Assuming p_interval is valid.
	ERR_FAIL_INDEX_V_MSG(p_interval.idx, baked_point_cache.size(), Vector2(), "Invalid interval");

	int idx = p_interval.idx;
	real_t frac = p_interval.frac;

	const Vector2* r = baked_point_cache.ptr();
	int pc = baked_point_cache.size();

	if (p_cubic) {
		Vector2 pre = idx > 0 ? r[idx - 1] : r[idx];
		Vector2 post = (idx < (pc - 2)) ? r[idx + 2] : r[idx + 1];
		return r[idx].cubic_interpolate(r[idx + 1], pre, post, frac);
	}
	else {
		return r[idx].lerp(r[idx + 1], frac);
	}
}

Transform2D Curve2D::_sample_posture(Interval p_interval) const
{
	// Assuming that p_interval is valid.
	ERR_FAIL_INDEX_V_MSG(
		p_interval.idx, baked_point_cache.size(), Transform2D(), "Invalid interval");

	int idx = p_interval.idx;
	real_t frac = p_interval.frac;

	Vector2 forward_begin = baked_forward_vector_cache[idx];
	Vector2 forward_end = baked_forward_vector_cache[idx + 1];

	// Build frames at both ends of the interval, then interpolate.
	const Vector2 forward = forward_begin.slerp(forward_end, frac).normalized();
	const Vector2 side = Vector2(-forward.y, forward.x);

	return Transform2D(forward, side, Vector2(0.0, 0.0));
}

Vector2 Curve2D::sample_baked(real_t p_offset, bool p_cubic) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), Vector2(), "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector2(), "No points in Curve2D.");

	if (pc == 1) {
		return baked_point_cache[0];
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	Curve2D::Interval interval = _find_interval(p_offset);
	return _sample_baked(interval, p_cubic);
}

Transform2D Curve2D::sample_baked_with_rotation(real_t p_offset, bool p_cubic) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), Transform2D(), "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	const int point_count = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(point_count == 0, Transform2D(), "No points in Curve3D.");

	if (point_count == 1) {
		Transform2D t;
		t.set_origin(baked_point_cache.get(0));
		ERR_FAIL_V_MSG(t, "Only 1 point in Curve2D.");
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	// 0. Find interval for all sampling steps.
	Curve2D::Interval interval = _find_interval(p_offset);

	// 1. Sample position.
	Vector2 pos = _sample_baked(interval, p_cubic);

	// 2. Sample rotation frame.
	Transform2D frame = _sample_posture(interval);
	frame.set_origin(pos);

	return frame;
}

PackedVector2Array Curve2D::get_baked_points() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_point_cache;
}

void Curve2D::set_bake_interval(real_t p_tolerance)
{
	bake_interval = p_tolerance;
	mark_dirty();
}

real_t Curve2D::get_bake_interval() const { return bake_interval; }

Vector2 Curve2D::get_closest_point(const Vector2& p_to_point) const
{
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector2(), "No points in Curve2D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector2* r = baked_point_cache.ptr();

	Vector2 nearest;
	real_t nearest_dist = -1.0f;

	for (int i = 0; i < pc - 1; i++) {
		const real_t interval = baked_dist_cache[i + 1] - baked_dist_cache[i];
		Vector2 origin = r[i];
		Vector2 direction = (r[i + 1] - origin) / interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, interval);
		Vector2 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = proj;
			nearest_dist = dist;
		}
	}

	return nearest;
}

real_t Curve2D::get_closest_offset(const Vector2& p_to_point) const
{
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0.0f, "No points in Curve2D.");

	if (pc == 1) {
		return 0.0f;
	}

	const Vector2* r = baked_point_cache.ptr();

	real_t nearest = 0.0f;
	real_t nearest_dist = -1.0f;
	real_t offset = 0.0f;

	for (int i = 0; i < pc - 1; i++) {
		offset = baked_dist_cache[i];

		const real_t interval = baked_dist_cache[i + 1] - baked_dist_cache[i];
		Vector2 origin = r[i];
		Vector2 direction = (r[i + 1] - origin) / interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, interval);
		Vector2 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = offset + d;
			nearest_dist = dist;
		}
	}

	return nearest;
}

PackedVector2Array Curve2D::tessellate(int p_max_stages, real_t p_tolerance) const
{
	PackedVector2Array tess;

	if (points.is_empty()) {
		return tess;
	}

	// The current implementation requires a sorted map.
	Vector<RBMap<real_t, Vector2>> midpoints;

	midpoints.resize(points.size() - 1);

	int pc = 1;
	for (uint32_t i = 0; i < points.size() - 1; i++) {
		_bake_segment2d(midpoints.write[i], 0, 1, points[i].position, points[i].out,
			points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_tolerance);
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector2* bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (uint32_t i = 0; i < points.size() - 1; i++) {
		for (const KeyValue<real_t, Vector2>& E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		bpw[pidx] = points[i + 1].position;
	}

	return tess;
}

Vector<RBMap<real_t, Vector2>> Curve2D::_tessellate_even_length(
	int p_max_stages, real_t p_length) const
{
	Vector<RBMap<real_t, Vector2>> midpoints;
	ERR_FAIL_COND_V_MSG(points.size() < 2, midpoints, "Curve must have at least 2 control point");

	midpoints.resize(points.size() - 1);

	for (uint32_t i = 0; i < points.size() - 1; i++) {
		_bake_segment2d_even_length(midpoints.write[i], 0, 1, points[i].position, points[i].out,
			points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_length);
	}
	return midpoints;
}

bool Curve2D::_filter_property(const String& p_name, int p_index) const
{
	if (p_index == 0) {
		return p_name != "in";
	}
	if (p_index == get_point_count() - 1) {
		return p_name != "out";
	}
	return true;
}

PackedVector2Array Curve2D::tessellate_even_length(int p_max_stages, real_t p_length) const
{
	PackedVector2Array tess;

	Vector<RBMap<real_t, Vector2>> midpoints = _tessellate_even_length(p_max_stages, p_length);
	if (midpoints.is_empty()) {
		return tess;
	}

	int pc = 1;
	for (uint32_t i = 0; i < points.size() - 1; i++) {
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector2* bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (uint32_t i = 0; i < points.size() - 1; i++) {
		for (const KeyValue<real_t, Vector2>& E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		bpw[pidx] = points[i + 1].position;
	}

	return tess;
}

int Curve3D::get_point_count() const { return points.size(); }

void Curve3D::_add_point(
	const Vector3& p_position, const Vector3& p_in, const Vector3& p_out, int p_atpos)
{
	Point n;
	n.position = p_position;
	n.in = p_in;
	n.out = p_out;
	if ((uint32_t)p_atpos < points.size()) {
		points.insert(p_atpos, n);
	}
	else {
		points.push_back(n);
	}

	mark_dirty();
}

void Curve3D::set_point_position(int p_index, const Vector3& p_position)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].position = p_position;
	mark_dirty();
}

Vector3 Curve3D::get_point_position(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector3());
	return points[p_index].position;
}

void Curve3D::set_point_tilt(int p_index, real_t p_tilt)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].tilt = p_tilt;
	mark_dirty();
}

real_t Curve3D::get_point_tilt(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), 0);
	return points[p_index].tilt;
}

void Curve3D::set_point_in(int p_index, const Vector3& p_in)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].in = p_in;
	mark_dirty();
}

Vector3 Curve3D::get_point_in(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector3());
	return points[p_index].in;
}

void Curve3D::set_point_out(int p_index, const Vector3& p_out)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());

	points[p_index].out = p_out;
	mark_dirty();
}

Vector3 Curve3D::get_point_out(int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, points.size(), Vector3());
	return points[p_index].out;
}

void Curve3D::_remove_point(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());
	points.remove_at(p_index);
	mark_dirty();
}

Vector3 Curve3D::sample(int p_index, real_t p_offset) const
{
	int pc = points.size();
	ERR_FAIL_COND_V(pc == 0, Vector3());

	if (p_index >= pc - 1) {
		if (!closed) {
			return points[pc - 1].position;
		}
		else {
			p_index = pc - 1;
		}
	}
	else if (p_index < 0) {
		return points[0].position;
	}

	Vector3 p0 = points[p_index].position;
	Vector3 p1 = p0 + points[p_index].out;
	Vector3 p3, p2;
	if (!closed || p_index < pc - 1) {
		p3 = points[p_index + 1].position;
		p2 = p3 + points[p_index + 1].in;
	}
	else {
		p3 = points[0].position;
		p2 = p3 + points[0].in;
	}

	return p0.bezier_interpolate(p1, p2, p3, p_offset);
}

Vector3 Curve3D::samplef(real_t p_findex) const
{
	if (p_findex < 0) {
		p_findex = 0;
	}
	else if (p_findex >= points.size()) {
		p_findex = points.size();
	}

	return sample((int)p_findex, Math::fmod(p_findex, (real_t)1.0));
}

void Curve3D::mark_dirty()
{
	baked_cache_dirty = true;
	emit_changed();
}

void Curve3D::_bake_segment3d(RBMap<real_t, Vector3>& r_bake, real_t p_begin, real_t p_end,
	const Vector3& p_a, const Vector3& p_out, const Vector3& p_b, const Vector3& p_in, int p_depth,
	int p_max_depth, real_t p_tol) const
{
	real_t mp = p_begin + (p_end - p_begin) * 0.5;
	Vector3 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector3 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
	Vector3 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	Vector3 na = (mid - beg).normalized();
	Vector3 nb = (end - mid).normalized();
	real_t dp = na.dot(nb);

	if (dp < Math::cos(Math::deg_to_rad(p_tol))) {
		r_bake[mp] = mid;
	}
	if (p_depth < p_max_depth) {
		_bake_segment3d(
			r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
		_bake_segment3d(r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
	}
}

void Curve3D::_bake_segment3d_even_length(RBMap<real_t, Vector3>& r_bake, real_t p_begin,
	real_t p_end, const Vector3& p_a, const Vector3& p_out, const Vector3& p_b, const Vector3& p_in,
	int p_depth, int p_max_depth, real_t p_length) const
{
	Vector3 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector3 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	real_t length = beg.distance_to(end);

	if (length > p_length && p_depth < p_max_depth) {
		real_t mp = (p_begin + p_end) * 0.5;
		Vector3 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
		r_bake[mp] = mid;

		_bake_segment3d_even_length(
			r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_length);
		_bake_segment3d_even_length(
			r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_length);
	}
}

Vector3 Curve3D::_calculate_tangent(const Vector3& p_begin, const Vector3& p_control_1,
	const Vector3& p_control_2, const Vector3& p_end, const real_t p_t)
{
	// Handle corner cases.
	if (Math::is_zero_approx(p_t - 0.0f)) {
		if (p_control_1.is_equal_approx(p_begin)) {
			if (p_control_1.is_equal_approx(p_control_2)) {
				return (p_end - p_begin).normalized();
			}
			else {
				return (p_control_2 - p_begin).normalized();
			}
		}
	}
	else if (Math::is_zero_approx(p_t - 1.0f)) {
		if (p_control_2.is_equal_approx(p_end)) {
			if (p_control_2.is_equal_approx(p_control_1)) {
				return (p_end - p_begin).normalized();
			}
			else {
				return (p_end - p_control_1).normalized();
			}
		}
	}

	if (p_control_1.is_equal_approx(p_end) && p_control_2.is_equal_approx(p_begin)) {
		return (p_end - p_begin).normalized();
	}

	return p_begin.bezier_derivative(p_control_1, p_control_2, p_end, p_t).normalized();
}

void Curve3D::_bake() const
{
	if (!baked_cache_dirty) {
		return;
	}

	baked_max_ofs = 0;
	baked_cache_dirty = false;

	if (points.is_empty()) {
#ifdef TOOLS_ENABLED
		points_in_cache.clear();
#endif
		baked_point_cache.clear();
		baked_tilt_cache.clear();
		baked_dist_cache.clear();

		baked_forward_vector_cache.clear();
		baked_up_vector_cache.clear();
		return;
	}

	if (points.size() == 1) {
#ifdef TOOLS_ENABLED
		points_in_cache.resize(1);
		points_in_cache.set(0, 0);
#endif

		baked_point_cache.resize(1);
		baked_point_cache.set(0, points[0].position);
		baked_tilt_cache.resize(1);
		baked_tilt_cache.set(0, points[0].tilt);
		baked_dist_cache.resize(1);
		baked_dist_cache.set(0, 0.0);
		baked_forward_vector_cache.resize(1);
		baked_forward_vector_cache.set(0, Vector3(0.0, 0.0, 1.0));

		if (up_vector_enabled) {
			baked_up_vector_cache.resize(1);
			baked_up_vector_cache.set(0, Vector3(0.0, 1.0, 0.0));
		}
		else {
			baked_up_vector_cache.clear();
		}

		return;
	}

	// Step 1: Tessellate curve to (almost) even length segments.
	{
		Vector<RBMap<real_t, Vector3>> midpoints = _tessellate_even_length(10, bake_interval);

		const int num_intervals = closed ? points.size() : points.size() - 1;

#ifdef TOOLS_ENABLED
		points_in_cache.resize(closed ? (points.size() + 1) : points.size());
		points_in_cache.set(0, 0);
#endif

		// Point Count: Begins at 1 to account for the last point.
		int pc = 1;
		for (int i = 0; i < num_intervals; i++) {
			pc++;
			pc += midpoints[i].size();
#ifdef TOOLS_ENABLED
			points_in_cache.set(i + 1, pc - 1);
#endif
		}

		baked_point_cache.resize(pc);
		baked_tilt_cache.resize(pc);
		baked_dist_cache.resize(pc);
		baked_forward_vector_cache.resize(pc);

		Vector3* bpw = baked_point_cache.ptrw();
		real_t* btw = baked_tilt_cache.ptrw();
		Vector3* bfw = baked_forward_vector_cache.ptrw();

		// Collect positions and sample tilts and tangents for each baked points.
		bpw[0] = points[0].position;
		bfw[0] = _calculate_tangent(points[0].position, points[0].position + points[0].out,
			points[1].position + points[1].in, points[1].position, 0.0);
		btw[0] = points[0].tilt;
		int pidx = 0;

		for (int i = 0; i < num_intervals; i++) {
			for (const KeyValue<real_t, Vector3>& E : midpoints[i]) {
				pidx++;
				bpw[pidx] = E.value;
				if (!closed || i < num_intervals - 1) {
					bfw[pidx] = _calculate_tangent(points[i].position,
						points[i].position + points[i].out,
						points[i + 1].position + points[i + 1].in, points[i + 1].position, E.key);
					btw[pidx] = Math::lerp(points[i].tilt, points[i + 1].tilt, E.key);
				}
				else {
					bfw[pidx] =
						_calculate_tangent(points[i].position, points[i].position + points[i].out,
							points[0].position + points[0].in, points[0].position, E.key);
					btw[pidx] = Math::lerp(points[i].tilt, points[0].tilt, E.key);
				}
			}

			pidx++;
			if (!closed || i < num_intervals - 1) {
				bpw[pidx] = points[i + 1].position;
				bfw[pidx] =
					_calculate_tangent(points[i].position, points[i].position + points[i].out,
						points[i + 1].position + points[i + 1].in, points[i + 1].position, 1.0);
				btw[pidx] = points[i + 1].tilt;
			}
			else {
				bpw[pidx] = points[0].position;
				bfw[pidx] =
					_calculate_tangent(points[i].position, points[i].position + points[i].out,
						points[0].position + points[0].in, points[0].position, 1.0);
				btw[pidx] = points[0].tilt;
			}
		}

		// Recalculate the baked distances.
		real_t* bdw = baked_dist_cache.ptrw();
		bdw[0] = 0.0;
		for (int i = 0; i < pc - 1; i++) {
			bdw[i + 1] = bdw[i] + bpw[i].distance_to(bpw[i + 1]);
		}
		baked_max_ofs = bdw[pc - 1];
	}

	if (!up_vector_enabled) {
		baked_up_vector_cache.resize(0);
		return;
	}

	// Step 2: Calculate the up vectors and the whole local reference frame.
	//
	// See Dougan, Carl. "The parallel transport frame." Game Programming Gems 2 (2001): 215-219.
	// for an example discussing about why not the Frenet frame.
	{
		int point_count = baked_point_cache.size();

		baked_up_vector_cache.resize(point_count);
		Vector3* up_write = baked_up_vector_cache.ptrw();

		const Vector3* forward_ptr = baked_forward_vector_cache.ptr();
		const Vector3* points_ptr = baked_point_cache.ptr();

		Basis frame; // X-right, Y-up, -Z-forward.
		Basis frame_prev;

		// Set the initial frame based on Y-up rule.
		{
			Vector3 forward = forward_ptr[0];

			if (std::abs(forward.dot(Vector3(0, 1, 0))) > 1.0 - UNIT_EPSILON) {
				frame_prev = Basis::looking_at(forward, Vector3(1, 0, 0));
			}
			else {
				frame_prev = Basis::looking_at(forward, Vector3(0, 1, 0));
			}

			up_write[0] = frame_prev.get_column(1);
		}

		// Calculate the Parallel Transport Frame.
		for (int idx = 1; idx < point_count; idx++) {
			Vector3 forward = forward_ptr[idx];

			Basis rotate;
			rotate.rotate_to_align(-frame_prev.get_column(2), forward);
			frame = rotate * frame_prev;
			frame.orthonormalize(); // Guard against float error accumulation.

			up_write[idx] = frame.get_column(1);
			frame_prev = frame;
		}

		bool is_loop = true;
		// Loop smoothing only applies when the curve is a loop, which means two ends meet, and
		// share forward directions.
		{
			if (!points_ptr[0].is_equal_approx(points_ptr[point_count - 1])) {
				is_loop = false;
			}

			real_t dot = forward_ptr[0].dot(forward_ptr[point_count - 1]);
			if (dot < 1.0 - UNIT_EPSILON) { // Alignment should not be too tight, or it doesn't work
											// for coarse bake interval.
				is_loop = false;
			}
		}

		// Twist up vectors, so that they align at two ends of the curve.
		if (is_loop) {
			const Vector3 up_start = up_write[0];
			const Vector3 up_end = up_write[point_count - 1];

			real_t sign = SIGN(up_end.cross(up_start).dot(forward_ptr[0]));
			real_t full_angle = Quaternion(up_end, up_start).get_angle();

			if (std::abs(full_angle) < CMP_EPSILON) {
				return;
			}
			else {
				const real_t* dists = baked_dist_cache.ptr();
				for (int idx = 1; idx < point_count; idx++) {
					const real_t frac = dists[idx] / baked_max_ofs;
					const real_t angle = Math::lerp((real_t)0.0, full_angle, frac);
					Basis twist(forward_ptr[idx] * sign, angle);

					up_write[idx] = twist.xform(up_write[idx]);
				}
			}
		}
	}
}

real_t Curve3D::get_baked_length() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_max_ofs;
}

Curve3D::Interval Curve3D::_find_interval(real_t p_offset) const
{
	Interval interval = {-1, 0.0};
	ERR_FAIL_COND_V_MSG(baked_cache_dirty, interval, "Backed cache is dirty");

	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc < 2, interval, "Less than two points in cache");

	int start = 0;
	int end = pc;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		}
		else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(
		p_offset < offset_begin || p_offset > offset_end, interval, "Offset out of range.");

	interval.idx = idx;
	if (idx_interval < FLT_EPSILON) {
		interval.frac = 0.5; // For a very short interval, 0.5 is a reasonable choice.
		ERR_FAIL_V_MSG(interval, "Zero length interval.");
	}

	interval.frac = (p_offset - offset_begin) / idx_interval;
	return interval;
}

Vector3 Curve3D::_sample_baked(Interval p_interval, bool p_cubic) const
{
	// Assuming p_interval is valid.
	ERR_FAIL_INDEX_V_MSG(p_interval.idx, baked_point_cache.size(), Vector3(), "Invalid interval");

	int idx = p_interval.idx;
	real_t frac = p_interval.frac;

	const Vector3* r = baked_point_cache.ptr();
	int pc = baked_point_cache.size();

	if (p_cubic) {
		Vector3 pre = idx > 0 ? r[idx - 1] : r[idx];
		Vector3 post = (idx < (pc - 2)) ? r[idx + 2] : r[idx + 1];
		return r[idx].cubic_interpolate(r[idx + 1], pre, post, frac);
	}
	else {
		return r[idx].lerp(r[idx + 1], frac);
	}
}

real_t Curve3D::_sample_baked_tilt(Interval p_interval) const
{
	// Assuming that p_interval is valid.
	ERR_FAIL_INDEX_V_MSG(p_interval.idx, baked_tilt_cache.size(), 0.0, "Invalid interval");

	int idx = p_interval.idx;
	real_t frac = p_interval.frac;

	const real_t* r = baked_tilt_cache.ptr();

	return Math::lerp(r[idx], r[idx + 1], frac);
}

// Internal method for getting posture at a baked point. Assuming caller
// make all safety checks.
Basis Curve3D::_compose_posture(int p_index) const
{
	Vector3 forward = baked_forward_vector_cache[p_index];

	Vector3 up;
	if (up_vector_enabled) {
		up = baked_up_vector_cache[p_index];
	}
	else {
		up = Vector3(0.0, 1.0, 0.0);
	}

	const Basis frame = Basis::looking_at(forward, up);
	return frame;
}

Basis Curve3D::_sample_posture(Interval p_interval, bool p_apply_tilt) const
{
	// Assuming that p_interval is valid.
	ERR_FAIL_INDEX_V_MSG(p_interval.idx, baked_point_cache.size(), Basis(), "Invalid interval");
	if (up_vector_enabled) {
		ERR_FAIL_INDEX_V_MSG(
			p_interval.idx, baked_up_vector_cache.size(), Basis(), "Invalid interval");
	}

	int idx = p_interval.idx;
	real_t frac = p_interval.frac;

	// Get frames at both ends of the interval, then interpolate.
	const Basis frame_begin = _compose_posture(idx);
	const Basis frame_end = _compose_posture(idx + 1);
	const Basis frame = frame_begin.slerp(frame_end, frac).orthonormalized();

	if (!p_apply_tilt) {
		return frame;
	}

	// Applying tilt.
	const real_t tilt = _sample_baked_tilt(p_interval);
	Vector3 tangent = -frame.get_column(2);

	const Basis twist(tangent, tilt);
	return twist * frame;
}

#ifdef TOOLS_ENABLED
// Get posture at a control point. Needed for Gizmo implementation.
Basis Curve3D::get_point_baked_posture(int p_index, bool p_apply_tilt) const
{
	if (baked_cache_dirty) {
		_bake();
	}

	// Assuming that p_idx is valid.
	ERR_FAIL_INDEX_V_MSG(p_index, points_in_cache.size(), Basis(), "Invalid control point index");

	int baked_idx = points_in_cache[p_index];
	Basis frame = _compose_posture(baked_idx);

	if (!p_apply_tilt) {
		return frame;
	}

	// Applying tilt.
	const real_t tilt = points[p_index].tilt;
	Vector3 tangent = -frame.get_column(2);
	const Basis twist(tangent, tilt);

	return twist * frame;
}
#endif

Vector3 Curve3D::sample_baked(real_t p_offset, bool p_cubic) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), Vector3(), "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector3(), "No points in Curve3D.");

	if (pc == 1) {
		return baked_point_cache[0];
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	Curve3D::Interval interval = _find_interval(p_offset);
	return _sample_baked(interval, p_cubic);
}

Transform3D Curve3D::sample_baked_with_rotation(
	real_t p_offset, bool p_cubic, bool p_apply_tilt) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), Transform3D(), "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	const int point_count = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(point_count == 0, Transform3D(), "No points in Curve3D.");

	if (point_count == 1) {
		Transform3D t;
		t.origin = baked_point_cache.get(0);
		ERR_FAIL_V_MSG(t, "Only 1 point in Curve3D.");
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	// 0. Find interval for all sampling steps.
	Curve3D::Interval interval = _find_interval(p_offset);

	// 1. Sample position.
	Vector3 pos = _sample_baked(interval, p_cubic);

	// 2. Sample rotation frame.
	Basis frame = _sample_posture(interval, p_apply_tilt);

	return Transform3D(frame, pos);
}

real_t Curve3D::sample_baked_tilt(real_t p_offset) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), 0, "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked tilts.
	int pc = baked_tilt_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0, "No tilts in Curve3D.");

	if (pc == 1) {
		return baked_tilt_cache.get(0);
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	Curve3D::Interval interval = _find_interval(p_offset);
	return _sample_baked_tilt(interval);
}

Vector3 Curve3D::sample_baked_up_vector(real_t p_offset, bool p_apply_tilt) const
{
	// Make sure that p_offset is finite.
	ERR_FAIL_COND_V_MSG(!Math::is_finite(p_offset), Vector3(0, 1, 0), "Offset is non-finite");

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked up vectors.
	ERR_FAIL_COND_V_MSG(!up_vector_enabled, Vector3(0, 1, 0), "No up vectors in Curve3D.");

	int count = baked_up_vector_cache.size();
	if (count == 1) {
		return baked_up_vector_cache.get(0);
	}

	p_offset = CLAMP(p_offset, 0.0, get_baked_length()); // PathFollower implement wrapping logic.

	Curve3D::Interval interval = _find_interval(p_offset);
	return _sample_posture(interval, p_apply_tilt).get_column(1);
}

PackedVector3Array Curve3D::get_baked_points() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_point_cache;
}

Vector<real_t> Curve3D::get_baked_tilts() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_tilt_cache;
}

PackedVector3Array Curve3D::get_baked_up_vectors() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_up_vector_cache;
}

Vector<real_t> Curve3D::get_baked_dist_cache() const
{
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_dist_cache;
}

Vector3 Curve3D::get_closest_point(const Vector3& p_to_point) const
{
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector3(), "No points in Curve3D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector3* r = baked_point_cache.ptr();

	Vector3 nearest;
	real_t nearest_dist = -1.0f;

	for (int i = 0; i < pc - 1; i++) {
		const real_t interval = baked_dist_cache[i + 1] - baked_dist_cache[i];
		Vector3 origin = r[i];
		Vector3 direction = (r[i + 1] - origin) / interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, interval);
		Vector3 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = proj;
			nearest_dist = dist;
		}
	}

	return nearest;
}

real_t Curve3D::get_closest_offset(const Vector3& p_to_point) const
{
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0.0f, "No points in Curve3D.");

	if (pc == 1) {
		return 0.0f;
	}

	const Vector3* r = baked_point_cache.ptr();

	real_t nearest = 0.0f;
	real_t nearest_dist = -1.0f;
	real_t offset;

	for (int i = 0; i < pc - 1; i++) {
		offset = baked_dist_cache[i];

		const real_t interval = baked_dist_cache[i + 1] - baked_dist_cache[i];
		Vector3 origin = r[i];
		Vector3 direction = (r[i + 1] - origin) / interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, interval);
		Vector3 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = offset + d;
			nearest_dist = dist;
		}
	}

	return nearest;
}

bool Curve3D::is_closed() const { return closed; }

void Curve3D::set_bake_interval(real_t p_tolerance)
{
	bake_interval = p_tolerance;
	mark_dirty();
}

real_t Curve3D::get_bake_interval() const { return bake_interval; }

void Curve3D::set_up_vector_enabled(bool p_enable)
{
	up_vector_enabled = p_enable;
	mark_dirty();
}

bool Curve3D::is_up_vector_enabled() const { return up_vector_enabled; }

PackedVector3Array Curve3D::tessellate(int p_max_stages, real_t p_tolerance) const
{
	PackedVector3Array tess;

	if (points.is_empty()) {
		return tess;
	}
	Vector<RBMap<real_t, Vector3>> midpoints;

	const int num_intervals = closed ? points.size() : points.size() - 1;
	midpoints.resize(num_intervals);

	// Point Count: Begins at 1 to account for the last point.
	int pc = 1;
	for (int i = 0; i < num_intervals; i++) {
		if (!closed || i < num_intervals - 1) {
			_bake_segment3d(midpoints.write[i], 0, 1, points[i].position, points[i].out,
				points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_tolerance);
		}
		else {
			_bake_segment3d(midpoints.write[i], 0, 1, points[i].position, points[i].out,
				points[0].position, points[0].in, 0, p_max_stages, p_tolerance);
		}
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector3* bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (int i = 0; i < num_intervals; i++) {
		for (const KeyValue<real_t, Vector3>& E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		if (!closed || i < num_intervals - 1) {
			bpw[pidx] = points[i + 1].position;
		}
		else {
			bpw[pidx] = points[0].position;
		}
	}

	return tess;
}

Vector<RBMap<real_t, Vector3>> Curve3D::_tessellate_even_length(
	int p_max_stages, real_t p_length) const
{
	Vector<RBMap<real_t, Vector3>> midpoints;
	ERR_FAIL_COND_V_MSG(points.size() < 2, midpoints, "Curve must have at least 2 control point");

	const int num_intervals = closed ? points.size() : points.size() - 1;
	midpoints.resize(num_intervals);

	for (int i = 0; i < num_intervals; i++) {
		if (!closed || i < num_intervals - 1) {
			_bake_segment3d_even_length(midpoints.write[i], 0, 1, points[i].position, points[i].out,
				points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_length);
		}
		else {
			_bake_segment3d_even_length(midpoints.write[i], 0, 1, points[i].position, points[i].out,
				points[0].position, points[0].in, 0, p_max_stages, p_length);
		}
	}
	return midpoints;
}

bool Curve3D::_filter_property(const String& p_name, int p_index) const
{
	if (!closed && p_index == 0) {
		return p_name != "in";
	}
	if (!closed && p_index == get_point_count() - 1) {
		return p_name != "out";
	}
	return true;
}

PackedVector3Array Curve3D::tessellate_even_length(int p_max_stages, real_t p_length) const
{
	PackedVector3Array tess;

	Vector<RBMap<real_t, Vector3>> midpoints = _tessellate_even_length(p_max_stages, p_length);
	if (midpoints.is_empty()) {
		return tess;
	}

	const int num_intervals = closed ? points.size() : points.size() - 1;
	// Point Count: Begins at 1 to account for the last point.
	int pc = 1;
	for (int i = 0; i < num_intervals; i++) {
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector3* bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (int i = 0; i < num_intervals; i++) {
		for (const KeyValue<real_t, Vector3>& E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		if (!closed || i < num_intervals - 1) {
			bpw[pidx] = points[i + 1].position;
		}
		else {
			bpw[pidx] = points[0].position;
		}
	}

	return tess;
}

void Curve2D::add_point(Vector2 const&, Vector2 const&, Vector2 const&, int) {}

void Curve::remove_point(int) {}

void Curve3D::clear_points() {}

void Curve2D::remove_point(int) {}

void Curve2D::clear_points() {}

void Curve3D::set_closed(bool) {}

void Curve3D::add_point(Vector3 const&, Vector3 const&, Vector3 const&, int) {}

Curve2D::Curve2D() {}

#ifdef MIN_X_RANGE
#undef MIN_X_RANGE
#endif
#ifdef MIN_Y_RANGE
#undef MIN_Y_RANGE
#endif

// --- gradient.cpp ---

#include "core/config/engine.h"
#include "gradient.h"

Gradient::Gradient()
{
	// Set initial gradient transition from black to white
	points.resize(2);
	points[0].color = Color(0, 0, 0, 1);
	points[0].offset = 0;
	points[1].color = Color(1, 1, 1, 1);
	points[1].offset = 1;
}

Gradient::~Gradient() {}

Vector<float> Gradient::get_offsets() const
{
	Vector<float> offsets;
	offsets.resize(points.size());
	for (uint32_t i = 0; i < points.size(); i++) {
		offsets.write[i] = points[i].offset;
	}
	return offsets;
}

Vector<Color> Gradient::get_colors() const
{
	Vector<Color> colors;
	colors.resize(points.size());
	for (uint32_t i = 0; i < points.size(); i++) {
		colors.write[i] = points[i].color;
	}
	return colors;
}

Gradient::InterpolationMode Gradient::get_interpolation_mode() { return interpolation_mode; }

void Gradient::set_interpolation_color_space(Gradient::ColorSpace p_color_space)
{
	if (p_color_space == interpolation_color_space) {
		return;
	}

	interpolation_color_space = p_color_space;
	emit_changed();
}

Gradient::ColorSpace Gradient::get_interpolation_color_space() { return interpolation_color_space; }

void Gradient::set_offsets(const Vector<float>& p_offsets)
{
	points.resize(p_offsets.size());
	for (uint32_t i = 0; i < points.size(); i++) {
		points[i].offset = p_offsets[i];
	}
	is_sorted = false;
	emit_changed();
}

void Gradient::set_colors(const Vector<Color>& p_colors)
{
	if (points.size() < p_colors.size()) {
		is_sorted = false;
	}
	points.resize(p_colors.size());
	for (uint32_t i = 0; i < points.size(); i++) {
		points[i].color = p_colors[i];
	}
	emit_changed();
}

void Gradient::add_point(float p_offset, const Color& p_color)
{
	Point p;
	p.offset = p_offset;
	p.color = p_color;
	is_sorted = false;
	points.push_back(p);

	emit_changed();
}

void Gradient::remove_point(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, points.size());
	ERR_FAIL_COND(points.size() <= 1);
	points.remove_at(p_index);
	emit_changed();
}

void Gradient::reverse()
{
	for (uint32_t i = 0; i < points.size(); i++) {
		points[i].offset = 1.0 - points[i].offset;
	}

	is_sorted = false;
	_update_sorting();
	emit_changed();
}

void Gradient::set_offset(int pos, const float offset)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)pos, points.size());
	_update_sorting();
	points[pos].offset = offset;
	is_sorted = false;
	emit_changed();
}

float Gradient::get_offset(int pos)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)pos, points.size(), 0.0);
	_update_sorting();
	return points[pos].offset;
}

void Gradient::set_color(int pos, const Color& color)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)pos, points.size());
	_update_sorting();
	points[pos].color = color;
	emit_changed();
}

Color Gradient::get_color(int pos)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)pos, points.size(), Color());
	_update_sorting();
	return points[pos].color;
}

int Gradient::get_point_count() const { return points.size(); }

// --- color_palette.cpp ---

#include "scene/gui/dialogs.h"

void ColorPalette::set_colors(const PackedColorArray& p_colors) { colors = p_colors; }

PackedColorArray ColorPalette::get_colors() const { return colors; }

// ============================================================================
// Merged from scene/resources/font.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/io/image_loader.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/types.h"
#include "font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/text_line.h"
#include "scene/resources/text_paragraph.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/renderer.h"

/*************************************************************************/
/*  Font                                                                 */
/*************************************************************************/

bool Font::_is_base_cyclic(const Ref<Font>& p_f, int p_depth) const
{
	ERR_FAIL_COND_V(p_depth > MAX_FALLBACK_DEPTH, true);
	if (p_f.is_null()) {
		return false;
	}
	if (p_f == this) {
		return true;
	}
	Ref<FontVariation> fv = p_f;
	if (fv.is_valid()) {
		return _is_base_cyclic(fv->get_base_font(), p_depth + 1);
	}
	Ref<SystemFont> fs = p_f;
	if (fs.is_valid()) {
		return _is_base_cyclic(fs->get_base_font(), p_depth + 1);
	}
	return false;
}

static float _measure_glyph_run(
	const RID& p_font_rid, const String& p_text, int p_font_size, int p_len)
{
	float total_w = 0.0f;
	int64_t prev_glyph = 0;
	for (int i = 0; i < p_len; i++) {
		char32_t c = p_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(p_font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			total_w += TS->font_get_kerning(
							 p_font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						   .x;
		}
		total_w += TS->font_get_glyph_advance(p_font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
	return total_w;
}

real_t Font::draw_char_outline(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
	int p_font_size, int p_size, const Color& p_modulate, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	TS->font_draw_glyph_outline(
		font_rid, p_canvas_item, p_font_size, p_size, p_pos, glyph, p_modulate, p_oversampling);
	return TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
}

real_t Font::draw_char(RID p_canvas_item, const Point2& p_pos, char32_t p_char, int p_font_size,
	const Color& p_modulate, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	TS->font_draw_glyph(
		font_rid, p_canvas_item, p_font_size, p_pos, glyph, p_modulate, p_oversampling);
	return TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
}

void Font::draw_multiline_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_max_lines,
	const Color& p_modulate, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	Vector<String> lines = p_text.split("\n");
	float line_h = MAX(1.0f, get_height(p_font_size));
	Point2 cur_pos = p_pos;
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	for (int i = 0; i < count; i++) {
		draw_string(p_canvas_item, cur_pos, lines[i], p_alignment, p_width, p_font_size, p_modulate,
			p_jst_flags, p_direction, p_orientation, p_oversampling);
		cur_pos.y += line_h;
	}
}

void Font::draw_multiline_string_outline(RID p_canvas_item, const Point2& p_pos,
	const String& p_text, HorizontalAlignment p_alignment, float p_width, int p_font_size,
	int p_max_lines, int p_size, const Color& p_modulate, uint32_t p_brk_flags,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	Vector<String> lines = p_text.split("\n");
	float line_h = MAX(1.0f, get_height(p_font_size));
	Point2 cur_pos = p_pos;
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	for (int i = 0; i < count; i++) {
		draw_string_outline(p_canvas_item, cur_pos, lines[i], p_alignment, p_width, p_font_size,
			p_size, p_modulate, p_jst_flags, p_direction, p_orientation, p_oversampling);
		cur_pos.y += line_h;
	}
}

void Font::draw_string_outline(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_size,
	const Color& p_modulate, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation, float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty() || p_font_size <= 0) {
		return;
	}
	if (p_width == 0.0f) {
		return;
	}

	String draw_text = p_text;
	float text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
	if (p_width > 0.0f && text_w > p_width) {
		const String ellipsis = "...";
		const float ellipsis_w = _measure_glyph_run(font_rid, ellipsis, p_font_size, 3);
		if (ellipsis_w <= p_width) {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) + ellipsis_w >
					   p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep) + ellipsis;
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
		else {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) > p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep);
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
	}

	Point2 ofs = p_pos;
	if (p_width > 0.0f && text_w < p_width) {
		if (p_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			ofs.x += Math::floor((p_width - text_w) * 0.5f);
		}
		else if (p_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			ofs.x += Math::floor(p_width - text_w);
		}
	}

	int64_t prev_glyph = 0;
	for (int i = 0; i < draw_text.length(); i++) {
		char32_t c = draw_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			ofs.x += TS->font_get_kerning(
						   font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						 .x;
		}
		if (glyph != 0) {
			TS->font_draw_glyph_outline(font_rid, p_canvas_item, p_font_size, p_size, ofs, glyph,
				p_modulate, p_oversampling);
		}
		ofs.x += TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
}

void Font::draw_string(RID p_canvas_item, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, const Color& p_modulate,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty() || p_font_size <= 0) {
		return;
	}
	if (p_width == 0.0f) {
		return;
	}

	String draw_text = p_text;
	float text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
	if (p_width > 0.0f && text_w > p_width) {
		const String ellipsis = "...";
		const float ellipsis_w = _measure_glyph_run(font_rid, ellipsis, p_font_size, 3);
		if (ellipsis_w <= p_width) {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) + ellipsis_w >
					   p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep) + ellipsis;
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
		else {
			int keep = draw_text.length();
			while (keep > 0 &&
				   _measure_glyph_run(font_rid, draw_text, p_font_size, keep) > p_width) {
				keep--;
			}
			draw_text = draw_text.substr(0, keep);
			text_w = _measure_glyph_run(font_rid, draw_text, p_font_size, draw_text.length());
		}
	}

	Point2 ofs = p_pos;
	if (p_width > 0.0f && text_w < p_width) {
		if (p_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
			ofs.x += Math::floor((p_width - text_w) * 0.5f);
		}
		else if (p_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			ofs.x += Math::floor(p_width - text_w);
		}
	}

	int64_t prev_glyph = 0;
	for (int i = 0; i < draw_text.length(); i++) {
		char32_t c = draw_text[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, c, 0);
		if (prev_glyph != 0 && glyph != 0) {
			ofs.x += TS->font_get_kerning(
						   font_rid, p_font_size, Vector2i((int32_t)prev_glyph, (int32_t)glyph))
						 .x;
		}
		if (glyph != 0) {
			TS->font_draw_glyph(
				font_rid, p_canvas_item, p_font_size, ofs, glyph, p_modulate, p_oversampling);
		}
		ofs.x += TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x;
		prev_glyph = glyph;
	}
}

real_t Font::get_ascent(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_ascent(font_rid, p_font_size) + get_spacing(TextServer::SPACING_TOP);
}

Size2 Font::get_char_size(char32_t p_char, int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return Size2();
	}
	int64_t glyph = TS->font_get_glyph_index(font_rid, p_font_size, p_char, 0);
	return Size2(
		TS->font_get_glyph_advance(font_rid, p_font_size, glyph).x, get_height(p_font_size));
}

real_t Font::get_descent(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_descent(font_rid, p_font_size) + get_spacing(TextServer::SPACING_BOTTOM);
}

real_t Font::get_height(int p_font_size) const
{
	return get_ascent(p_font_size) + get_descent(p_font_size);
}

Size2 Font::get_multiline_string_size(const String& p_text, HorizontalAlignment p_alignment,
	float p_width, int p_font_size, int p_max_lines, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null() || p_text.is_empty()) {
		return Size2();
	}
	Vector<String> lines = p_text.split("\n");
	int count = lines.size();
	if (p_max_lines > 0 && count > p_max_lines) {
		count = p_max_lines;
	}
	float max_w = 0.0f;
	for (int i = 0; i < count; i++) {
		max_w = MAX(max_w, _measure_glyph_run(font_rid, lines[i], p_font_size, lines[i].length()));
	}
	if (p_width > 0.0f) {
		max_w = MIN(max_w, p_width);
	}
	return Size2(max_w, get_height(p_font_size) * count);
}

Size2 Font::get_string_size(const String& p_text, HorizontalAlignment p_alignment, float p_width,
	int p_font_size, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return Size2();
	}
	float w = _measure_glyph_run(font_rid, p_text, p_font_size, p_text.length());
	if (p_width > 0.0f) {
		w = MIN(w, p_width);
	}
	return Size2(w, get_height(p_font_size));
}

String Font::get_supported_chars() const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return String();
	}
	return TS->font_get_supported_chars(font_rid);
}

real_t Font::get_underline_position(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_underline_position(font_rid, p_font_size) +
		   get_spacing(TextServer::SPACING_TOP);
}

real_t Font::get_underline_thickness(int p_font_size) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return 0.0;
	}
	return TS->font_get_underline_thickness(font_rid, p_font_size);
}

bool Font::has_char(char32_t p_char) const
{
	RID font_rid = _get_rid();
	if (!font_rid.is_valid() || TS.is_null()) {
		return false;
	}
	return TS->font_has_char(font_rid, p_char);
}

void Font::_invalidate_rids() {}

bool Font::_is_cyclic(const Ref<Font>& p_f, int p_depth) const { return true; }

void Font::_update_rids_fb(const Font* p_f, int p_depth) const {}

void Font::_update_rids() const {}

void Font::reset_state() { _invalidate_rids(); }

String Font::get_font_name() const { return TS->font_get_name(_get_rid()); }

int64_t Font::get_palette_count() const { return TS->font_get_palette_count(_get_rid()); }

String Font::get_palette_name(int64_t p_index) const
{
	return TS->font_get_palette_name(_get_rid(), p_index);
}

Vector<Color> Font::get_palette_colors(int64_t p_index) const
{
	return TS->font_get_palette_colors(_get_rid(), p_index);
}

String Font::get_font_style_name() const { return TS->font_get_style_name(_get_rid()); }

uint32_t Font::get_font_style() const { return TS->font_get_style(_get_rid()); }

int Font::get_font_weight() const { return TS->font_get_weight(_get_rid()); }

int Font::get_font_stretch() const { return TS->font_get_stretch(_get_rid()); }

// Drawing string.
void Font::set_cache_capacity(int p_single_line, int p_multi_line)
{
	cache.set_capacity(p_single_line);
	cache_wrap.set_capacity(p_multi_line);
}

bool Font::is_language_supported(const String& p_language) const
{
	return TS->font_is_language_supported(_get_rid(), p_language);
}

bool Font::is_script_supported(const String& p_script) const
{
	return TS->font_is_script_supported(_get_rid(), p_script);
}

int64_t Font::get_face_count() const { return TS->font_get_face_count(_get_rid()); }

Font::Font()
{
	cache.set_capacity(64);
	cache_wrap.set_capacity(16);
}

Font::~Font() {}

/*************************************************************************/
/*  FontFile                                                             */
/*************************************************************************/

_FORCE_INLINE_ void FontFile::_clear_cache()
{
	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			if (TS.is_valid()) {
				TS->free_rid(cache[i]);
			}
			cache.write[i] = RID();
		}
	}
}

void FontFile::_convert_packed_8bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_r;
	imgdata_r.resize(w * h * 2);
	uint8_t* wr = imgdata_r.ptrw();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_b;
	imgdata_b.resize(w * h * 2);
	uint8_t* wb = imgdata_b.ptrw();

	PackedByteArray imgdata_a;
	imgdata_a.resize(w * h * 2);
	uint8_t* wa = imgdata_a.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * 4;
			int ofs_dst = (i * w + j) * 2;
			wr[ofs_dst + 0] = 255;
			wr[ofs_dst + 1] = r[ofs_src + 0];
			wg[ofs_dst + 0] = 255;
			wg[ofs_dst + 1] = r[ofs_src + 1];
			wb[ofs_dst + 0] = 255;
			wb[ofs_dst + 1] = r[ofs_src + 2];
			wa[ofs_dst + 0] = 255;
			wa[ofs_dst + 1] = r[ofs_src + 3];
		}
	}
	Ref<Image> img_r = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_r));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 0, img_r);
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 1, img_g);
	Ref<Image> img_b = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_b));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 2, img_b);
	Ref<Image> img_a = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_a));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 3, img_a);
}

void FontFile::_convert_packed_4bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_r;
	imgdata_r.resize(w * h * 2);
	uint8_t* wr = imgdata_r.ptrw();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_b;
	imgdata_b.resize(w * h * 2);
	uint8_t* wb = imgdata_b.ptrw();

	PackedByteArray imgdata_a;
	imgdata_a.resize(w * h * 2);
	uint8_t* wa = imgdata_a.ptrw();

	PackedByteArray imgdata_ro;
	imgdata_ro.resize(w * h * 2);
	uint8_t* wro = imgdata_ro.ptrw();

	PackedByteArray imgdata_go;
	imgdata_go.resize(w * h * 2);
	uint8_t* wgo = imgdata_go.ptrw();

	PackedByteArray imgdata_bo;
	imgdata_bo.resize(w * h * 2);
	uint8_t* wbo = imgdata_bo.ptrw();

	PackedByteArray imgdata_ao;
	imgdata_ao.resize(w * h * 2);
	uint8_t* wao = imgdata_ao.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * 4;
			int ofs_dst = (i * w + j) * 2;
			wr[ofs_dst + 0] = 255;
			wro[ofs_dst + 0] = 255;
			if (r[ofs_src + 0] > 0x0F) {
				wr[ofs_dst + 1] = (r[ofs_src + 0] - 0x0F) * 2;
				wro[ofs_dst + 1] = 0;
			}
			else {
				wr[ofs_dst + 1] = 0;
				wro[ofs_dst + 1] = r[ofs_src + 0] * 2;
			}
			wg[ofs_dst + 0] = 255;
			wgo[ofs_dst + 0] = 255;
			if (r[ofs_src + 1] > 0x0F) {
				wg[ofs_dst + 1] = (r[ofs_src + 1] - 0x0F) * 2;
				wgo[ofs_dst + 1] = 0;
			}
			else {
				wg[ofs_dst + 1] = 0;
				wgo[ofs_dst + 1] = r[ofs_src + 1] * 2;
			}
			wb[ofs_dst + 0] = 255;
			wbo[ofs_dst + 0] = 255;
			if (r[ofs_src + 2] > 0x0F) {
				wb[ofs_dst + 1] = (r[ofs_src + 2] - 0x0F) * 2;
				wbo[ofs_dst + 1] = 0;
			}
			else {
				wb[ofs_dst + 1] = 0;
				wbo[ofs_dst + 1] = r[ofs_src + 2] * 2;
			}
			wa[ofs_dst + 0] = 255;
			wao[ofs_dst + 0] = 255;
			if (r[ofs_src + 3] > 0x0F) {
				wa[ofs_dst + 1] = (r[ofs_src + 3] - 0x0F) * 2;
				wao[ofs_dst + 1] = 0;
			}
			else {
				wa[ofs_dst + 1] = 0;
				wao[ofs_dst + 1] = r[ofs_src + 3] * 2;
			}
		}
	}
	Ref<Image> img_r = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_r));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 0, img_r);
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 1, img_g);
	Ref<Image> img_b = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_b));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 2, img_b);
	Ref<Image> img_a = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_a));
	set_texture_image(0, Vector2i(p_sz, 0), p_page * 4 + 3, img_a);

	Ref<Image> img_ro = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_ro));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 0, img_ro);
	Ref<Image> img_go = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_go));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 1, img_go);
	Ref<Image> img_bo = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_bo));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 2, img_bo);
	Ref<Image> img_ao = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_ao));
	set_texture_image(0, Vector2i(p_sz, 1), p_page * 4 + 3, img_ao);
}

void FontFile::_convert_rgba_4bit(Ref<Image>& p_source, int p_page, int p_sz)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 4);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_o;
	imgdata_o.resize(w * h * 4);
	uint8_t* wo = imgdata_o.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs = (i * w + j) * 4;

			if (r[ofs + 0] > 0x7F) {
				wg[ofs + 0] = r[ofs + 0];
				wo[ofs + 0] = 0;
			}
			else {
				wg[ofs + 0] = 0;
				wo[ofs + 0] = r[ofs + 0] * 2;
			}
			if (r[ofs + 1] > 0x7F) {
				wg[ofs + 1] = r[ofs + 1];
				wo[ofs + 1] = 0;
			}
			else {
				wg[ofs + 1] = 0;
				wo[ofs + 1] = r[ofs + 1] * 2;
			}
			if (r[ofs + 2] > 0x7F) {
				wg[ofs + 2] = r[ofs + 2];
				wo[ofs + 2] = 0;
			}
			else {
				wg[ofs + 2] = 0;
				wo[ofs + 2] = r[ofs + 2] * 2;
			}
			if (r[ofs + 3] > 0x7F) {
				wg[ofs + 3] = r[ofs + 3];
				wo[ofs + 3] = 0;
			}
			else {
				wg[ofs + 3] = 0;
				wo[ofs + 3] = r[ofs + 3] * 2;
			}
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_RGBA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page, img_g);

	Ref<Image> img_o = memnew(Image(w, h, false, Image::FORMAT_RGBA8, imgdata_o));
	set_texture_image(0, Vector2i(p_sz, 1), p_page, img_o);
}

void FontFile::_convert_mono_8bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	int size = 4;
	if (p_source->get_format() == Image::FORMAT_L8) {
		size = 1;
		p_ch = 0;
	}

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * size;
			int ofs_dst = (i * w + j) * 2;
			wg[ofs_dst + 0] = 255;
			wg[ofs_dst + 1] = r[ofs_src + p_ch];
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, p_ol), p_page, img_g);
}

void FontFile::_convert_mono_4bit(Ref<Image>& p_source, int p_page, int p_ch, int p_sz, int p_ol)
{
	int w = p_source->get_width();
	int h = p_source->get_height();

	PackedByteArray imgdata = p_source->get_data();
	const uint8_t* r = imgdata.ptr();

	int size = 4;
	if (p_source->get_format() == Image::FORMAT_L8) {
		size = 1;
		p_ch = 0;
	}

	PackedByteArray imgdata_g;
	imgdata_g.resize(w * h * 2);
	uint8_t* wg = imgdata_g.ptrw();

	PackedByteArray imgdata_o;
	imgdata_o.resize(w * h * 2);
	uint8_t* wo = imgdata_o.ptrw();

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int ofs_src = (i * w + j) * size;
			int ofs_dst = (i * w + j) * 2;
			wg[ofs_dst + 0] = 255;
			wo[ofs_dst + 0] = 255;
			if (r[ofs_src + p_ch] > 0x7F) {
				wg[ofs_dst + 1] = r[ofs_src + p_ch];
				wo[ofs_dst + 1] = 0;
			}
			else {
				wg[ofs_dst + 1] = 0;
				wo[ofs_dst + 1] = r[ofs_src + p_ch] * 2;
			}
		}
	}
	Ref<Image> img_g = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_g));
	set_texture_image(0, Vector2i(p_sz, 0), p_page, img_g);

	Ref<Image> img_o = memnew(Image(w, h, false, Image::FORMAT_LA8, imgdata_o));
	set_texture_image(0, Vector2i(p_sz, p_ol), p_page, img_o);
}

void FontFile::reset_state()
{
	_clear_cache();
	data.clear();
	data_ptr = nullptr;
	data_size = 0;
	cache.clear();

	antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
	mipmaps = false;
	disable_embedded_bitmaps = true;
	msdf = false;
	force_autohinter = false;
	modulate_color_glyphs = false;
	allow_system_fallback = true;
	hinting = TextServer::HINTING_LIGHT;
	subpixel_positioning = TextServer::SUBPIXEL_POSITIONING_DISABLED;
	keep_rounding_remainders = true;
	oversampling_override = 0.0;
	msdf_pixel_range = 14;
	msdf_size = 128;
	fixed_size = 0;
	fixed_size_scale_mode = TextServer::FIXED_SIZE_SCALE_DISABLE;

	Font::reset_state();
}

/*************************************************************************/

// OEM encoding mapping for 0x80..0xFF range.
static const char32_t _oem_to_unicode[][129] = {
	U"\u20ac\ufffe\u201a\ufffe\u201e\u2026\u2020\u2021\ufffe\u2030\u0160\u2039\u015a\u0164\u017d"
	U"\u0179\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\u0161\u203a\u015b\u0165"
	U"\u017e\u017a\xa0\u02c7\u02d8\u0141\xa4\u0104\xa6\xa7\xa8\xa9\u015e\xab\xac\xad\xae\u017b\xb0"
	U"\xb1\u02db\u0142\xb4\xb5\xb6\xb7\xb8\u0105\u015f\xbb\u013d\u02dd\u013e\u017c\u0154\xc1\xc2"
	U"\u0102\xc4\u0139\u0106\xc7\u010c\xc9\u0118\xcb\u011a\xcd\xce\u010e\u0110\u0143\u0147\xd3\xd4"
	U"\u0150\xd6\xd7\u0158\u016e\xda\u0170\xdc\xdd\u0162\xdf\u0155\xe1\xe2\u0103\xe4\u013a\u0107"
	U"\xe7\u010d\xe9\u0119\xeb\u011b\xed\xee\u010f\u0111\u0144\u0148\xf3\xf4\u0151\xf6\xf7\u0159"
	U"\u016f\xfa\u0171\xfc\xfd\u0163\u02d9", // 1250 - Latin 2
	U"\u0402\u0403\u201a\u0453\u201e\u2026\u2020\u2021\u20ac\u2030\u0409\u2039\u040a\u040c\u040b"
	U"\u040f\u0452\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\u0459\u203a\u045a\u045c"
	U"\u045b\u045f\xa0\u040e\u045e\u0408\xa4\u0490\xa6\xa7\u0401\xa9\u0404\xab\xac\xad\xae\u0407"
	U"\xb0\xb1\u0406\u0456\u0491\xb5\xb6\xb7\u0451\u2116\u0454\xbb\u0458\u0405\u0455\u0457\u0410"
	U"\u0411\u0412\u0413\u0414\u0415\u0416\u0417\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f"
	U"\u0420\u0421\u0422\u0423\u0424\u0425\u0426\u0427\u0428\u0429\u042a\u042b\u042c\u042d\u042e"
	U"\u042f\u0430\u0431\u0432\u0433\u0434\u0435\u0436\u0437\u0438\u0439\u043a\u043b\u043c\u043d"
	U"\u043e\u043f\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044a\u044b\u044c"
	U"\u044d\u044e\u044f", // 1251 - Cyrillic
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0160\u2039\u0152\ufffe\u017d"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\u0161\u203a\u0153\ufffe"
	U"\u017e\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca"
	U"\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1"
	U"\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8"
	U"\xf9\xfa\xfb\xfc\xfd\xfe\xff", // 1252 - Latin 1
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\ufffe\u2030\ufffe\u2039\ufffe\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\ufffe\u203a\ufffe\ufffe"
	U"\ufffe\ufffe\xa0\u0385\u0386\xa3\xa4\xa5\xa6\xa7\xa8\xa9\ufffe\xab\xac\xad\xae\u2015\xb0\xb1"
	U"\xb2\xb3\u0384\xb5\xb6\xb7\u0388\u0389\u038a\xbb\u038c\xbd\u038e\u038f\u0390\u0391\u0392"
	U"\u0393\u0394\u0395\u0396\u0397\u0398\u0399\u039a\u039b\u039c\u039d\u039e\u039f\u03a0\u03a1"
	U"\ufffe\u03a3\u03a4\u03a5\u03a6\u03a7\u03a8\u03a9\u03aa\u03ab\u03ac\u03ad\u03ae\u03af\u03b0"
	U"\u03b1\u03b2\u03b3\u03b4\u03b5\u03b6\u03b7\u03b8\u03b9\u03ba\u03bb\u03bc\u03bd\u03be\u03bf"
	U"\u03c0\u03c1\u03c2\u03c3\u03c4\u03c5\u03c6\u03c7\u03c8\u03c9\u03ca\u03cb\u03cc\u03cd\u03ce"
	U"\ufffe", // 1253 - Greek
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0160\u2039\u0152\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\u0161\u203a\u0153\ufffe"
	U"\ufffe\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca"
	U"\xcb\xcc\xcd\xce\xcf\u011e\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\u0130\u015e\xdf"
	U"\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\u011f\xf1\xf2\xf3\xf4\xf5"
	U"\xf6\xf7\xf8\xf9\xfa\xfb\xfc\u0131\u015f\xff", // 1254 - Turkish
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\ufffe\u2039\ufffe\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\ufffe\u203a\ufffe\ufffe"
	U"\ufffe\ufffe\xa0\xa1\xa2\xa3\u20aa\xa5\xa6\xa7\xa8\xa9\xd7\xab\xac\xad\xae\xaf\xb0\xb1\xb2"
	U"\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xf7\xbb\xbc\xbd\xbe\xbf\u05b0\u05b1\u05b2\u05b3\u05b4\u05b5"
	U"\u05b6\u05b7\u05b8\u05b9\ufffe\u05bb\u05bc\u05bd\u05be\u05bf\u05c0\u05c1\u05c2\u05c3\u05f0"
	U"\u05f1\u05f2\u05f3\u05f4\ufffe\ufffe\ufffe\ufffe\ufffe\ufffe\ufffe\u05d0\u05d1\u05d2\u05d3"
	U"\u05d4\u05d5\u05d6\u05d7\u05d8\u05d9\u05da\u05db\u05dc\u05dd\u05de\u05df\u05e0\u05e1\u05e2"
	U"\u05e3\u05e4\u05e5\u05e6\u05e7\u05e8\u05e9\u05ea\ufffe\ufffe\u200e\u200f\ufffe", // 1255 -
																					   // Hebrew
	U"\u20ac\u067e\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\u0679\u2039\u0152\u0686\u0698"
	U"\u0688\u06af\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u06a9\u2122\u0691\u203a\u0153\u200c"
	U"\u200d\u06ba\xa0\u060c\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\u06be\xab\xac\xad\xae\xaf\xb0\xb1\xb2"
	U"\xb3\xb4\xb5\xb6\xb7\xb8\xb9\u061b\xbb\xbc\xbd\xbe\u061f\u06c1\u0621\u0622\u0623\u0624\u0625"
	U"\u0626\u0627\u0628\u0629\u062a\u062b\u062c\u062d\u062e\u062f\u0630\u0631\u0632\u0633\u0634"
	U"\u0635\u0636\xd7\u0637\u0638\u0639\u063a\u0640\u0641\u0642\u0643\xe0\u0644\xe2\u0645\u0646"
	U"\u0647\u0648\xe7\xe8\xe9\xea\xeb\u0649\u064a\xee\xef\u064b\u064c\u064d\u064e\xf4\u064f\u0650"
	U"\xf7\u0651\xf9\u0652\xfb\xfc\u200e\u200f\u06d2", // 1256 - Arabic
	U"\u20ac\ufffe\u201a\ufffe\u201e\u2026\u2020\u2021\ufffe\u2030\ufffe\u2039\ufffe\xa8\u02c7\xb8"
	U"\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\ufffe\u2122\ufffe\u203a\ufffe\xaf\u02db"
	U"\ufffe\xa0\ufffe\xa2\xa3\xa4\ufffe\xa6\xa7\xd8\xa9\u0156\xab\xac\xad\xae\xc6\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xf8\xb9\u0157\xbb\xbc\xbd\xbe\xe6\u0104\u012e\u0100\u0106\xc4\xc5\u0118"
	U"\u0112\u010c\xc9\u0179\u0116\u0122\u0136\u012a\u013b\u0160\u0143\u0145\xd3\u014c\xd5\xd6\xd7"
	U"\u0172\u0141\u015a\u016a\xdc\u017b\u017d\xdf\u0105\u012f\u0101\u0107\xe4\xe5\u0119\u0113"
	U"\u010d\xe9\u017a\u0117\u0123\u0137\u012b\u013c\u0161\u0144\u0146\xf3\u014d\xf5\xf6\xf7\u0173"
	U"\u0142\u015b\u016b\xfc\u017c\u017e\u02d9", // 1257 - Baltic
	U"\u20ac\ufffe\u201a\u0192\u201e\u2026\u2020\u2021\u02c6\u2030\ufffe\u2039\u0152\ufffe\ufffe"
	U"\ufffe\ufffe\u2018\u2019\u201c\u201d\u2022\u2013\u2014\u02dc\u2122\ufffe\u203a\u0153\ufffe"
	U"\ufffe\u0178\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3"
	U"\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\u0102\xc4\xc5\xc6\xc7\xc8\xc9"
	U"\xca\xcb\u0300\xcd\xce\xcf\u0110\xd1\u0309\xd3\xd4\u01a0\xd6\xd7\xd8\xd9\xda\xdb\xdc\u01af"
	U"\u0303\xdf\xe0\xe1\xe2\u0103\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\u0301\xed\xee\xef\u0111\xf1"
	U"\u0323\xf3\xf4\u01a1\xf6\xf7\xf8\xf9\xfa\xfb\xfc\u01b0\u20ab\xff", // 1258 - Vietnamese
};

Error FontFile::load_bitmap_font(const String& p_path)
{
	return _load_bitmap_font(p_path, nullptr);
}

Error FontFile::load_dynamic_font(const String& p_path)
{
	reset_state();

	Vector<uint8_t> font_data = FileAccess::get_file_as_bytes(p_path);
	set_data(font_data);

	return OK;
}

void FontFile::set_data_ptr(const uint8_t* p_data, size_t p_size)
{
	data.clear();
	data_ptr = p_data;
	data_size = p_size;

	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			TS->font_set_data_ptr(cache[i], data_ptr, data_size);
		}
	}
}

void FontFile::set_data(const PackedByteArray& p_data)
{
	data = p_data;
	data_ptr = data.ptr();
	data_size = data.size();

	for (int i = 0; i < cache.size(); i++) {
		if (cache[i].is_valid()) {
			TS->font_set_data_ptr(cache[i], data_ptr, data_size);
		}
	}
}

PackedByteArray FontFile::get_data() const
{
	if (unlikely((size_t)data.size() != data_size)) {
		data.resize(data_size);
		memcpy(data.ptrw(), data_ptr, data_size);
	}
	return data;
}

TextServer::FontAntialiasing FontFile::get_antialiasing() const { return antialiasing; }

bool FontFile::get_disable_embedded_bitmaps() const { return disable_embedded_bitmaps; }

bool FontFile::get_generate_mipmaps() const { return mipmaps; }

bool FontFile::is_multichannel_signed_distance_field() const { return msdf; }

int FontFile::get_msdf_pixel_range() const { return msdf_pixel_range; }

int FontFile::get_msdf_size() const { return msdf_size; }

int FontFile::get_fixed_size() const { return fixed_size; }

TextServer::FixedSizeScaleMode FontFile::get_fixed_size_scale_mode() const
{
	return fixed_size_scale_mode;
}

bool FontFile::is_allow_system_fallback() const { return allow_system_fallback; }

bool FontFile::is_force_autohinter() const { return force_autohinter; }

bool FontFile::is_modulate_color_glyphs() const { return modulate_color_glyphs; }

TextServer::Hinting FontFile::get_hinting() const { return hinting; }

TextServer::SubpixelPositioning FontFile::get_subpixel_positioning() const
{
	return subpixel_positioning;
}

bool FontFile::get_keep_rounding_remainders() const { return keep_rounding_remainders; }

real_t FontFile::get_oversampling() const { return oversampling_override; }

int FontFile::get_cache_count() const { return cache.size(); }

void FontFile::clear_cache()
{
	_clear_cache();
	cache.clear();
	emit_changed();
}

void FontFile::remove_cache(int p_cache_index)
{
	ERR_FAIL_INDEX(p_cache_index, cache.size());
	if (cache[p_cache_index].is_valid()) {
		TS->free_rid(cache.write[p_cache_index]);
	}
	cache.remove_at(p_cache_index);
	emit_changed();
}

bool FontFile::get_language_support_override(const String& p_language) const
{
	if (language_support_overrides.has(p_language)) {
		return language_support_overrides[p_language];
	}
	else {
		return false;
	}
}

Vector<String> FontFile::get_language_support_overrides() const
{
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : language_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

bool FontFile::get_script_support_override(const String& p_script) const
{
	if (script_support_overrides.has(p_script)) {
		return script_support_overrides[p_script];
	}
	else {
		return false;
	}
}

Vector<String> FontFile::get_script_support_overrides() const
{
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : script_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

FontFile::FontFile() {}

FontFile::~FontFile() { _clear_cache(); }

Ref<Font> FontVariation::get_base_font() const { return base_font; }

void FontVariation::set_variation_embolden(float p_strength)
{
	if (variation.embolden != p_strength) {
		variation.embolden = p_strength;
		_invalidate_rids();
	}
}

float FontVariation::get_variation_embolden() const { return variation.embolden; }

void FontVariation::set_variation_transform(Transform2D p_transform)
{
	if (variation.transform != p_transform) {
		variation.transform = p_transform;
		_invalidate_rids();
	}
}

Transform2D FontVariation::get_variation_transform() const { return variation.transform; }

void FontVariation::set_variation_face_index(int p_face_index)
{
	if (variation.face_index != p_face_index) {
		variation.face_index = p_face_index;
		_invalidate_rids();
	}
}

int FontVariation::get_variation_face_index() const { return variation.face_index; }

void FontVariation::set_spacing(TextServer::SpacingType p_spacing, int p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, TextServer::SPACING_MAX);
	if (extra_spacing[p_spacing] != p_value) {
		extra_spacing[p_spacing] = p_value;
		_invalidate_rids();
	}
}

int FontVariation::get_spacing(TextServer::SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, TextServer::SPACING_MAX, 0);
	return extra_spacing[p_spacing];
}

void FontVariation::set_baseline_offset(float p_baseline_offset)
{
	if (baseline_offset != p_baseline_offset) {
		baseline_offset = p_baseline_offset;
		_invalidate_rids();
	}
}

float FontVariation::get_baseline_offset() const { return baseline_offset; }

void FontVariation::set_palette_index(int64_t p_palette_index)
{
	if (palette_index != p_palette_index) {
		palette_index = p_palette_index;
		_invalidate_rids();
	}
}

int64_t FontVariation::get_palette_index() const { return palette_index; }

void FontVariation::set_palette_custom_colors(const Vector<Color>& p_colors)
{
	if (custom_colors != p_colors) {
		custom_colors = p_colors;
		_invalidate_rids();
	}
}

Vector<Color> FontVariation::get_palette_custom_colors() const { return custom_colors; }

FontVariation::FontVariation()
{
	for (int i = 0; i < TextServer::SPACING_MAX; i++) {
		extra_spacing[i] = 0;
	}
}

FontVariation::~FontVariation() {}

/*************************************************************************/
/*  SystemFont                                                           */
/*************************************************************************/

void SystemFont::set_antialiasing(TextServer::FontAntialiasing p_antialiasing)
{
	if (antialiasing != p_antialiasing) {
		antialiasing = p_antialiasing;
		if (base_font.is_valid()) {
			base_font->set_antialiasing(antialiasing);
		}
		emit_changed();
	}
}

TextServer::FontAntialiasing SystemFont::get_antialiasing() const { return antialiasing; }

void SystemFont::set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps)
{
	if (disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		disable_embedded_bitmaps = p_disable_embedded_bitmaps;
		if (base_font.is_valid()) {
			base_font->set_disable_embedded_bitmaps(disable_embedded_bitmaps);
		}
		emit_changed();
	}
}

bool SystemFont::get_disable_embedded_bitmaps() const { return disable_embedded_bitmaps; }

void SystemFont::set_generate_mipmaps(bool p_generate_mipmaps)
{
	if (mipmaps != p_generate_mipmaps) {
		mipmaps = p_generate_mipmaps;
		if (base_font.is_valid()) {
			base_font->set_generate_mipmaps(mipmaps);
		}
		emit_changed();
	}
}

bool SystemFont::get_generate_mipmaps() const { return mipmaps; }

void SystemFont::set_allow_system_fallback(bool p_allow_system_fallback)
{
	if (allow_system_fallback != p_allow_system_fallback) {
		allow_system_fallback = p_allow_system_fallback;
		if (base_font.is_valid()) {
			base_font->set_allow_system_fallback(allow_system_fallback);
		}
		emit_changed();
	}
}

bool SystemFont::is_allow_system_fallback() const { return allow_system_fallback; }

void SystemFont::set_force_autohinter(bool p_force_autohinter)
{
	if (force_autohinter != p_force_autohinter) {
		force_autohinter = p_force_autohinter;
		if (base_font.is_valid()) {
			base_font->set_force_autohinter(force_autohinter);
		}
		emit_changed();
	}
}

bool SystemFont::is_force_autohinter() const { return force_autohinter; }

void SystemFont::set_modulate_color_glyphs(bool p_modulate)
{
	if (modulate_color_glyphs != p_modulate) {
		modulate_color_glyphs = p_modulate;
		if (base_font.is_valid()) {
			base_font->set_modulate_color_glyphs(modulate_color_glyphs);
		}
		emit_changed();
	}
}

bool SystemFont::is_modulate_color_glyphs() const { return modulate_color_glyphs; }

void SystemFont::set_hinting(TextServer::Hinting p_hinting)
{
	if (hinting != p_hinting) {
		hinting = p_hinting;
		if (base_font.is_valid()) {
			base_font->set_hinting(hinting);
		}
		emit_changed();
	}
}

TextServer::Hinting SystemFont::get_hinting() const { return hinting; }

void SystemFont::set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel)
{
	if (subpixel_positioning != p_subpixel) {
		subpixel_positioning = p_subpixel;
		if (base_font.is_valid()) {
			base_font->set_subpixel_positioning(subpixel_positioning);
		}
		emit_changed();
	}
}

TextServer::SubpixelPositioning SystemFont::get_subpixel_positioning() const
{
	return subpixel_positioning;
}

void SystemFont::set_keep_rounding_remainders(bool p_keep_rounding_remainders)
{
	if (keep_rounding_remainders != p_keep_rounding_remainders) {
		keep_rounding_remainders = p_keep_rounding_remainders;
		if (base_font.is_valid()) {
			base_font->set_keep_rounding_remainders(keep_rounding_remainders);
		}
		emit_changed();
	}
}

bool SystemFont::get_keep_rounding_remainders() const { return keep_rounding_remainders; }

void SystemFont::set_oversampling(real_t p_oversampling)
{
	if (oversampling_override != p_oversampling) {
		oversampling_override = p_oversampling;
		if (base_font.is_valid()) {
			base_font->set_oversampling(oversampling_override);
		}
		emit_changed();
	}
}

real_t SystemFont::get_oversampling() const { return oversampling_override; }

void SystemFont::set_multichannel_signed_distance_field(bool p_msdf)
{
	if (msdf != p_msdf) {
		msdf = p_msdf;
		if (base_font.is_valid()) {
			base_font->set_multichannel_signed_distance_field(msdf);
		}
		emit_changed();
	}
}

bool SystemFont::is_multichannel_signed_distance_field() const { return msdf; }

void SystemFont::set_msdf_pixel_range(int p_msdf_pixel_range)
{
	if (msdf_pixel_range != p_msdf_pixel_range) {
		msdf_pixel_range = p_msdf_pixel_range;
		if (base_font.is_valid()) {
			base_font->set_msdf_pixel_range(msdf_pixel_range);
		}
		emit_changed();
	}
}

int SystemFont::get_msdf_pixel_range() const { return msdf_pixel_range; }

void SystemFont::set_msdf_size(int p_msdf_size)
{
	if (msdf_size != p_msdf_size) {
		msdf_size = p_msdf_size;
		if (base_font.is_valid()) {
			base_font->set_msdf_size(msdf_size);
		}
		emit_changed();
	}
}

int SystemFont::get_msdf_size() const { return msdf_size; }

void SystemFont::set_font_names(const PackedStringArray& p_names)
{
	if (names != p_names) {
		names = p_names;
		_update_base_font();
	}
}

PackedStringArray SystemFont::get_font_names() const { return names; }

void SystemFont::set_font_italic(bool p_italic)
{
	if (italic != p_italic) {
		italic = p_italic;
		_update_base_font();
	}
}

bool SystemFont::get_font_italic() const { return italic; }

void SystemFont::set_font_weight(int p_weight)
{
	if (weight != p_weight) {
		weight = CLAMP(p_weight, 100, 999);
		_update_base_font();
	}
}

int SystemFont::get_font_weight() const { return weight; }

void SystemFont::set_font_stretch(int p_stretch)
{
	if (stretch != p_stretch) {
		stretch = CLAMP(p_stretch, 50, 200);
		_update_base_font();
	}
}

int SystemFont::get_font_stretch() const { return stretch; }

int SystemFont::get_spacing(TextServer::SpacingType p_spacing) const
{
	if (base_font.is_valid()) {
		return base_font->get_spacing(p_spacing);
	}
	else {
		return 0;
	}
}

int64_t SystemFont::get_face_count() const { return face_indices.size(); }

void SystemFont::_update_base_font() {}

SystemFont::SystemFont()
{ /* NOP */
}

SystemFont::~SystemFont() {}

void FontVariation::_update_rids() const {}

void FontVariation::reset_state() {}

Error FontFile::_load_bitmap_font(String const&, List<String, DefaultAllocator>*) {}

void FontFile::set_transform(int p_cache_index, Transform2D p_transform) {}

void FontFile::set_texture_offsets(int p_cache_index, const Vector2i& p_size, int p_texture_index,
	const PackedInt32Array& p_offset)
{
}

void FontFile::set_texture_image(
	int p_cache_index, const Vector2i& p_size, int p_texture_index, const Ref<Image>& p_image)
{
}

void FontFile::set_subpixel_positioning(TextServer::SubpixelPositioning p_subpixel)
{
	if (subpixel_positioning != p_subpixel) {
		subpixel_positioning = p_subpixel;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_subpixel_positioning(cache[i], subpixel_positioning);
			}
		}
	}
}

void FontFile::set_script_support_override(const String& p_script, bool p_supported) {}

void FontFile::set_oversampling(real_t p_oversampling)
{
	if (oversampling_override != p_oversampling) {
		oversampling_override = p_oversampling;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_oversampling(cache[i], oversampling_override);
			}
		}
	}
}

void FontFile::set_multichannel_signed_distance_field(bool p_msdf)
{
	if (msdf != p_msdf) {
		msdf = p_msdf;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_multichannel_signed_distance_field(cache[i], msdf);
			}
		}
	}
}

void FontFile::set_msdf_size(int p_msdf_size)
{
	if (msdf_size != p_msdf_size) {
		msdf_size = p_msdf_size;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_msdf_size(cache[i], msdf_size);
			}
		}
	}
}

void FontFile::set_msdf_pixel_range(int p_msdf_pixel_range)
{
	if (msdf_pixel_range != p_msdf_pixel_range) {
		msdf_pixel_range = p_msdf_pixel_range;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_msdf_pixel_range(cache[i], msdf_pixel_range);
			}
		}
	}
}

void FontFile::set_modulate_color_glyphs(bool p_modulate)
{
	if (modulate_color_glyphs != p_modulate) {
		modulate_color_glyphs = p_modulate;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_modulate_color_glyphs(cache[i], modulate_color_glyphs);
			}
		}
	}
}

void FontFile::set_language_support_override(const String& p_language, bool p_supported) {}

void FontFile::set_kerning(
	int p_cache_index, int p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning)
{
}

void FontFile::set_keep_rounding_remainders(bool p_keep_rounding_remainders)
{
	if (keep_rounding_remainders != p_keep_rounding_remainders) {
		keep_rounding_remainders = p_keep_rounding_remainders;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_keep_rounding_remainders(cache[i], keep_rounding_remainders);
			}
		}
	}
}

void FontFile::set_hinting(TextServer::Hinting p_hinting)
{
	if (hinting != p_hinting) {
		hinting = p_hinting;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_hinting(cache[i], hinting);
			}
		}
	}
}

void FontFile::set_glyph_uv_rect(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Rect2& p_uv_rect)
{
}

void FontFile::set_glyph_texture_idx(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, int p_texture_idx)
{
}

void FontFile::set_glyph_size(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_gl_size)
{
}

void FontFile::set_glyph_offset(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph, const Vector2& p_offset)
{
}

void FontFile::set_glyph_advance(
	int p_cache_index, int p_size, int32_t p_glyph, const Vector2& p_advance)
{
}

void FontFile::set_generate_mipmaps(bool p_generate_mipmaps)
{
	if (mipmaps != p_generate_mipmaps) {
		mipmaps = p_generate_mipmaps;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_generate_mipmaps(cache[i], mipmaps);
			}
		}
	}
}

void FontFile::set_force_autohinter(bool p_force_autohinter)
{
	if (force_autohinter != p_force_autohinter) {
		force_autohinter = p_force_autohinter;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_force_autohinter(cache[i], force_autohinter);
			}
		}
	}
}

void FontFile::set_font_weight(int p_weight) {}

void FontFile::set_font_style(uint32_t p_style) {}

void FontFile::set_font_style_name(const String& p_name) {}

void FontFile::set_font_stretch(int p_stretch) {}

void FontFile::set_font_name(const String& p_name) {}

void FontFile::set_fixed_size_scale_mode(TextServer::FixedSizeScaleMode p_fixed_size_scale_mode)
{
	if (fixed_size_scale_mode != p_fixed_size_scale_mode) {
		fixed_size_scale_mode = p_fixed_size_scale_mode;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_fixed_size_scale_mode(cache[i], fixed_size_scale_mode);
			}
		}
	}
}

void FontFile::set_fixed_size(int p_fixed_size)
{
	if (fixed_size != p_fixed_size) {
		fixed_size = p_fixed_size;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_fixed_size(cache[i], fixed_size);
			}
		}
	}
}

void FontFile::set_face_index(int p_cache_index, int64_t p_index) {}

void FontFile::set_extra_spacing(
	int p_cache_index, TextServer::SpacingType p_spacing, int64_t p_value)
{
}

void FontFile::set_extra_baseline_offset(int p_cache_index, float p_baseline_offset) {}

void FontFile::set_embolden(int p_cache_index, float p_strength) {}

void FontFile::set_disable_embedded_bitmaps(bool p_disable_embedded_bitmaps)
{
	if (disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		disable_embedded_bitmaps = p_disable_embedded_bitmaps;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_disable_embedded_bitmaps(cache[i], disable_embedded_bitmaps);
			}
		}
	}
}

void FontFile::set_cache_underline_thickness(
	int p_cache_index, int p_size, real_t p_underline_thickness)
{
}

void FontFile::set_cache_underline_position(
	int p_cache_index, int p_size, real_t p_underline_position)
{
}

void FontFile::set_cache_scale(int p_cache_index, int p_size, real_t p_scale) {}

void FontFile::set_cache_descent(int p_cache_index, int p_size, real_t p_descent) {}

void FontFile::set_cache_ascent(int p_cache_index, int p_size, real_t p_ascent) {}

void FontFile::set_antialiasing(TextServer::FontAntialiasing p_antialiasing)
{
	if (antialiasing != p_antialiasing) {
		antialiasing = p_antialiasing;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_antialiasing(cache[i], antialiasing);
			}
		}
	}
}

void FontFile::set_allow_system_fallback(bool p_allow_system_fallback)
{
	if (allow_system_fallback != p_allow_system_fallback) {
		allow_system_fallback = p_allow_system_fallback;
		for (int i = 0; i < cache.size(); i++) {
			if (cache[i].is_valid() && TS.is_valid()) {
				TS->font_set_allow_system_fallback(cache[i], allow_system_fallback);
			}
		}
	}
}

void FontFile::render_range(
	int p_cache_index, const Vector2i& p_size, char32_t p_start, char32_t p_end)
{
}

void FontFile::render_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_index) {}

void FontFile::remove_texture(int p_cache_index, const Vector2i& p_size, int p_texture_index) {}

void FontFile::remove_size_cache(int p_cache_index, const Vector2i& p_size) {}

void FontFile::remove_script_support_override(const String& p_script) {}

void FontFile::remove_language_support_override(const String& p_language) {}

void FontFile::remove_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) {}

void FontFile::remove_glyph(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) {}

Transform2D FontFile::get_transform(int p_cache_index) const { return Transform2D(); }

PackedInt32Array FontFile::get_texture_offsets(
	int p_cache_index, const Vector2i& p_size, int p_texture_index) const
{
	return PackedInt32Array();
}

Ref<Image> FontFile::get_texture_image(
	int p_cache_index, const Vector2i& p_size, int p_texture_index) const
{
	Ref<Image> i = memnew(Image);
	return i;
}

int FontFile::get_texture_count(int p_cache_index, const Vector2i& p_size) const { return 0; }

Vector2 FontFile::get_kerning(int p_cache_index, int p_size, const Vector2i& p_glyph_pair) const
{
	return Vector2();
}

Rect2 FontFile::get_glyph_uv_rect(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Rect2();
}

int FontFile::get_glyph_texture_idx(
	int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return 0;
}

Vector2 FontFile::get_glyph_size(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Vector2();
}

Vector2 FontFile::get_glyph_offset(int p_cache_index, const Vector2i& p_size, int32_t p_glyph) const
{
	return Vector2();
}

PackedInt32Array FontFile::get_glyph_list(int p_cache_index, const Vector2i& p_size) const
{
	return PackedInt32Array();
}

int32_t FontFile::get_glyph_index(int p_size, char32_t p_char, char32_t p_variation_selector) const
{
	return 0;
}

Vector2 FontFile::get_glyph_advance(int p_cache_index, int p_size, int32_t p_glyph) const
{
	return Vector2();
}

int64_t FontFile::get_face_index(int p_cache_index) const { return 0; }

int64_t FontFile::get_extra_spacing(int p_cache_index, TextServer::SpacingType p_spacing) const
{
	return 0;
}

float FontFile::get_extra_baseline_offset(int p_cache_index) const { return 0.0; }

float FontFile::get_embolden(int p_cache_index) const { return 0.0; }

char32_t FontFile::get_char_from_glyph_index(int p_size, int32_t p_glyph_index) const
{
	return ' ';
}

real_t FontFile::get_cache_underline_thickness(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_underline_position(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_scale(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_descent(int p_cache_index, int p_size) const { return 0.0; }

real_t FontFile::get_cache_ascent(int p_cache_index, int p_size) const { return 0.0; }

void FontFile::clear_textures(int p_cache_index, const Vector2i& p_size) {}

void FontFile::clear_size_cache(int p_cache_index) {}

void FontFile::clear_kerning_map(int p_cache_index, int p_size) {}

void FontFile::clear_glyphs(int p_cache_index, const Vector2i& p_size) {}

RID FontFile::_get_rid() const
{
	if (TS.is_null() || data_ptr == nullptr || data_size == 0) {
		return RID();
	}
	if (unlikely(cache.is_empty())) {
		cache.resize(1);
	}
	if (unlikely(!cache[0].is_valid())) {
		RID rid = TS->create_font();
		if (rid.is_valid()) {
			TS->font_set_data_ptr(rid, data_ptr, data_size);
			TS->font_set_antialiasing(rid, antialiasing);
			TS->font_set_generate_mipmaps(rid, mipmaps);
			TS->font_set_disable_embedded_bitmaps(rid, disable_embedded_bitmaps);
			TS->font_set_multichannel_signed_distance_field(rid, msdf);
			TS->font_set_msdf_pixel_range(rid, msdf_pixel_range);
			TS->font_set_msdf_size(rid, msdf_size);
			TS->font_set_fixed_size(rid, fixed_size);
			TS->font_set_fixed_size_scale_mode(rid, fixed_size_scale_mode);
			TS->font_set_force_autohinter(rid, force_autohinter);
			TS->font_set_modulate_color_glyphs(rid, modulate_color_glyphs);
			TS->font_set_allow_system_fallback(rid, allow_system_fallback);
			TS->font_set_hinting(rid, hinting);
			TS->font_set_subpixel_positioning(rid, subpixel_positioning);
			TS->font_set_keep_rounding_remainders(rid, keep_rounding_remainders);
			TS->font_set_oversampling(rid, oversampling_override);
			cache.write[0] = rid;
		}
	}
	return cache[0];
}

void SystemFont::reset_state() {}

void SystemFont::_update_rids() const {}

RID SystemFont::_get_rid() const
{
	return base_font.is_valid() ? base_font->_get_rid() : RID();
}

Ref<Font> SystemFont::_get_base_font_or_default() const
{
	if (base_font.is_valid()) {
		return base_font;
	}
	return ThemeDB::get_fallback_font();
}

RID FontVariation::_get_rid() const
{
	Ref<Font> f = _get_base_font_or_default();
	return f.is_valid() ? f->_get_rid() : RID();
}

void FontVariation::set_base_font(const Ref<Font>& p_font)
{
	ERR_FAIL_COND_MSG(_is_base_cyclic(p_font, 0), "Cyclic font reference.");
	base_font = p_font;
	_invalidate_rids();
}

Ref<Font> FontVariation::_get_base_font_or_default() const
{
	if (base_font.is_valid()) {
		return base_font;
	}
	return ThemeDB::get_fallback_font();
}

// --- font.compat.inc ---

#ifndef DISABLE_DEPRECATED
real_t Font::_draw_char_bind_compat_104872(RID p_canvas_item, const Point2& p_pos, char32_t p_char,
	int p_font_size, const Color& p_modulate) const
{
	return draw_char(p_canvas_item, p_pos, p_char, p_font_size, p_modulate, 0.0);
}

real_t Font::_draw_char_outline_bind_compat_104872(RID p_canvas_item, const Point2& p_pos,
	char32_t p_char, int p_font_size, int p_size, const Color& p_modulate) const
{
	return draw_char_outline(p_canvas_item, p_pos, p_char, p_font_size, p_size, p_modulate, 0.0);
}

#endif // DISABLE_DEPRECATED

// --- text_line.cpp ---

#include "text_line.h"

RID TextLine::get_rid() const { return rid; }

void TextLine::clear() { TS->shaped_text_clear(rid); }

Ref<TextLine> TextLine::duplicate() const
{
	Ref<TextLine> copy;
	copy.instantiate();
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->dirty = true;
	copy->width = width;
	copy->flags = flags;
	copy->alignment = alignment;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextLine::set_preserve_invalid(bool p_enabled)
{
	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_invalid() const { return TS->shaped_text_get_preserve_invalid(rid); }

void TextLine::set_preserve_control(bool p_enabled)
{
	TS->shaped_text_set_preserve_control(rid, p_enabled);
	dirty = true;
}

bool TextLine::get_preserve_control() const { return TS->shaped_text_get_preserve_control(rid); }

void TextLine::set_direction(TextServer::Direction p_direction)
{
	TS->shaped_text_set_direction(rid, p_direction);
	dirty = true;
}

TextServer::Direction TextLine::get_direction() const { return TS->shaped_text_get_direction(rid); }

TextServer::Direction TextLine::get_inferred_direction() const
{
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextLine::set_orientation(TextServer::Orientation p_orientation)
{
	TS->shaped_text_set_orientation(rid, p_orientation);
	dirty = true;
}

TextServer::Orientation TextLine::get_orientation() const
{
	return TS->shaped_text_get_orientation(rid);
}

void TextLine::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextLine::get_horizontal_alignment() const { return alignment; }

void TextLine::tab_align(const Vector<float>& p_tab_stops)
{
	tab_stops = p_tab_stops;
	dirty = true;
}

void TextLine::set_flags(uint32_t p_flags)
{
	if (flags != p_flags) {
		flags = p_flags;
		dirty = true;
	}
}

uint32_t TextLine::get_flags() const { return flags; }

void TextLine::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		dirty = true;
	}
}

TextServer::OverrunBehavior TextLine::get_text_overrun_behavior() const { return overrun_behavior; }

void TextLine::set_ellipsis_char(const String& p_char)
{
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}
	if (el_char == c) {
		return;
	}
	el_char = c;
	dirty = true;
}

String TextLine::get_ellipsis_char() const { return el_char; }

void TextLine::set_width(float p_width)
{
	if (width == p_width) {
		return;
	}
	width = p_width;
	if (alignment == HORIZONTAL_ALIGNMENT_FILL ||
		overrun_behavior != TextServer::OVERRUN_NO_TRIMMING) {
		dirty = true;
	}
}

float TextLine::get_width() const { return width; }

Size2 TextLine::get_size() const
{
	_shape();
	return TS->shaped_text_get_size(rid);
}

float TextLine::get_line_ascent() const
{
	_shape();
	return TS->shaped_text_get_ascent(rid);
}

float TextLine::get_line_descent() const
{
	_shape();
	return TS->shaped_text_get_descent(rid);
}

float TextLine::get_line_width() const
{
	_shape();
	return TS->shaped_text_get_width(rid);
}

float TextLine::get_line_underline_position() const
{
	_shape();
	return TS->shaped_text_get_underline_position(rid);
}

float TextLine::get_line_underline_thickness() const
{
	_shape();
	return TS->shaped_text_get_underline_thickness(rid);
}

void TextLine::draw(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_color, p_oversampling);
}

void TextLine::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_shape();

	Vector2 ofs = p_pos;

	float length = TS->shaped_text_get_width(rid);
	if (width > 0) {
		switch (alignment) {
		case HORIZONTAL_ALIGNMENT_FILL:
		case HORIZONTAL_ALIGNMENT_LEFT:
			break;
		case HORIZONTAL_ALIGNMENT_CENTER: {
			if (length <= width) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += Math::floor((width - length) / 2.0);
				}
				else {
					ofs.y += Math::floor((width - length) / 2.0);
				}
			}
			else if (TS->shaped_text_get_inferred_direction(rid) == TextServer::DIRECTION_RTL) {
				if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += width - length;
				}
				else {
					ofs.y += width - length;
				}
			}
		} break;
		case HORIZONTAL_ALIGNMENT_RIGHT: {
			if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += width - length;
			}
			else {
				ofs.y += width - length;
			}
		} break;
		}
	}

	float clip_l;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.x - ofs.x);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(rid);
		clip_l = MAX(0, p_pos.y - ofs.y);
	}
	return TS->shaped_text_draw_outline(
		rid, p_canvas, ofs, clip_l, clip_l + width, p_outline_size, p_color, p_oversampling);
}

int TextLine::hit_test(float p_coords) const
{
	_shape();

	return TS->shaped_text_hit_test_position(rid, p_coords);
}

TextLine::TextLine() { rid = TS->create_shaped_text(); }

TextLine::~TextLine() { TS->free_rid(rid); }

void TextLine::_shape() const {}


TextLine::TextLine(String const&, Ref<Font> const&, int, String const&, TextServer::Direction, TextServer::Orientation) {}

// --- text_line.compat.inc ---

#ifndef DISABLE_DEPRECATED
void TextLine::_draw_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color) const
{
	draw(p_canvas, p_pos, p_color, 0.0);
}

void TextLine::_draw_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_outline_size, const Color& p_color) const
{
	draw_outline(p_canvas, p_pos, p_outline_size, p_color, 0.0);
}

void TextLine::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- text_paragraph.cpp ---

#include "text_paragraph.h"

RID TextParagraph::get_rid() const { return rid; }

RID TextParagraph::get_line_rid(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), RID());
	return lines_rid[p_line];
}

RID TextParagraph::get_dropcap_rid() const { return dropcap_rid; }

void TextParagraph::clear()
{
	_THREAD_SAFE_METHOD_

	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->shaped_text_clear(rid);
	TS->shaped_text_clear(dropcap_rid);
}

Ref<TextParagraph> TextParagraph::duplicate() const
{
	Ref<TextParagraph> copy;
	copy.instantiate();
	if (dropcap_rid.is_valid()) {
		TS->free_rid(copy->dropcap_rid);
		copy->dropcap_rid = TS->shaped_text_duplicate(dropcap_rid);
	}
	copy->dropcap_lines = dropcap_lines;
	copy->dropcap_margins = dropcap_margins;
	if (rid.is_valid()) {
		TS->free_rid(copy->rid);
		copy->rid = TS->shaped_text_duplicate(rid);
	}
	copy->lines_dirty = true;
	copy->line_spacing = line_spacing;
	copy->width = width;
	copy->max_lines_visible = max_lines_visible;
	copy->brk_flags = brk_flags;
	copy->jst_flags = jst_flags;
	copy->el_char = el_char;
	copy->overrun_behavior = overrun_behavior;
	copy->alignment = alignment;
	copy->tab_stops = tab_stops;

	return copy;
}

void TextParagraph::set_preserve_invalid(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_invalid(rid, p_enabled);
	TS->shaped_text_set_preserve_invalid(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_invalid() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_invalid(rid);
}

void TextParagraph::set_preserve_control(bool p_enabled)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_preserve_control(rid, p_enabled);
	TS->shaped_text_set_preserve_control(dropcap_rid, p_enabled);
	lines_dirty = true;
}

bool TextParagraph::get_preserve_control() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_preserve_control(rid);
}

void TextParagraph::set_direction(TextServer::Direction p_direction)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_direction(rid, p_direction);
	TS->shaped_text_set_direction(dropcap_rid, p_direction);
	lines_dirty = true;
}

TextServer::Direction TextParagraph::get_direction() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_direction(rid);
}

TextServer::Direction TextParagraph::get_inferred_direction() const
{
	_THREAD_SAFE_METHOD_

	const_cast<TextParagraph*>(this)->_shape_lines();
	return TS->shaped_text_get_inferred_direction(rid);
}

void TextParagraph::set_custom_punctuation(const String& p_punct)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_custom_punctuation(rid, p_punct);
	lines_dirty = true;
}

String TextParagraph::get_custom_punctuation() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_custom_punctuation(rid);
}

void TextParagraph::set_orientation(TextServer::Orientation p_orientation)
{
	_THREAD_SAFE_METHOD_

	TS->shaped_text_set_orientation(rid, p_orientation);
	TS->shaped_text_set_orientation(dropcap_rid, p_orientation);
	lines_dirty = true;
}

TextServer::Orientation TextParagraph::get_orientation() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_orientation(rid);
}

void TextParagraph::clear_dropcap()
{
	_THREAD_SAFE_METHOD_
	dropcap_margins = Rect2();
	TS->shaped_text_clear(dropcap_rid);
	lines_dirty = true;
}

void TextParagraph::set_alignment(HorizontalAlignment p_alignment)
{
	_THREAD_SAFE_METHOD_

	if (alignment != p_alignment) {
		if (alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			alignment = p_alignment;
			lines_dirty = true;
		}
		else {
			alignment = p_alignment;
		}
	}
}

HorizontalAlignment TextParagraph::get_alignment() const { return alignment; }

void TextParagraph::tab_align(const Vector<float>& p_tab_stops)
{
	_THREAD_SAFE_METHOD_

	tab_stops = p_tab_stops;
	lines_dirty = true;
}

void TextParagraph::set_justification_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_justification_flags() const { return jst_flags; }

void TextParagraph::set_break_flags(uint32_t p_flags)
{
	_THREAD_SAFE_METHOD_

	if (brk_flags != p_flags) {
		brk_flags = p_flags;
		lines_dirty = true;
	}
}

uint32_t TextParagraph::get_break_flags() const { return brk_flags; }

void TextParagraph::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior)
{
	_THREAD_SAFE_METHOD_

	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		lines_dirty = true;
	}
}

TextServer::OverrunBehavior TextParagraph::get_text_overrun_behavior() const
{
	return overrun_behavior;
}

void TextParagraph::set_ellipsis_char(const String& p_char)
{
	String c = p_char;
	if (c.length() > 1) {
		WARN_PRINT("Ellipsis must be exactly one character long (" + itos(c.length()) +
				   " characters given).");
		c = c.left(1);
	}
	if (el_char == c) {
		return;
	}
	el_char = c;
	lines_dirty = true;
}

String TextParagraph::get_ellipsis_char() const { return el_char; }

void TextParagraph::set_width(float p_width)
{
	_THREAD_SAFE_METHOD_

	if (width != p_width) {
		width = p_width;
		lines_dirty = true;
	}
}

float TextParagraph::get_width() const { return width; }

Size2 TextParagraph::get_non_wrapped_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return TS->shaped_text_get_size(rid);
}

Size2 TextParagraph::get_size() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();

	float h_offset = 0.f;
	float v_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
		v_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
		v_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}

	Size2 size;
	int visible_lines = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();
	for (int i = 0; i < visible_lines; i++) {
		Size2 lsize = TS->shaped_text_get_size(lines_rid[i]);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.x += h_offset;
			}
			size.x = MAX(size.x, lsize.x);
			size.y += lsize.y;
			if (i != visible_lines - 1) {
				size.y += line_spacing;
			}
		}
		else {
			if (h_offset > 0 && i <= dropcap_lines) {
				lsize.y += h_offset;
			}
			size.x += lsize.x;
			size.y = MAX(size.y, lsize.y);
			if (i != visible_lines - 1) {
				size.x += line_spacing;
			}
		}
	}
	if (h_offset > 0) {
		if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			size.y = MAX(size.y, v_offset);
		}
		else {
			size.x = MAX(size.x, v_offset);
		}
	}
	return size;
}

Vector2i TextParagraph::get_range() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_range(rid);
}

int TextParagraph::get_line_count() const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	return (int)lines_rid.size();
}

void TextParagraph::set_max_lines_visible(int p_lines)
{
	_THREAD_SAFE_METHOD_

	if (p_lines != max_lines_visible) {
		max_lines_visible = p_lines;
		lines_dirty = true;
	}
}

int TextParagraph::get_max_lines_visible() const { return max_lines_visible; }

void TextParagraph::set_line_spacing(float p_spacing)
{
	_THREAD_SAFE_METHOD_

	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		lines_dirty = true;
	}
}

float TextParagraph::get_line_spacing() const { return line_spacing; }

Size2 TextParagraph::get_line_size(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Size2());
	return TS->shaped_text_get_size(lines_rid[p_line]);
}

Vector2i TextParagraph::get_line_range(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), Vector2i());
	return TS->shaped_text_get_range(lines_rid[p_line]);
}

float TextParagraph::get_line_ascent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_ascent(lines_rid[p_line]);
}

float TextParagraph::get_line_descent(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_descent(lines_rid[p_line]);
}

float TextParagraph::get_line_width(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_width(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_position(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_position(lines_rid[p_line]);
}

float TextParagraph::get_line_underline_thickness(int p_line) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND_V(p_line < 0 || p_line >= (int)lines_rid.size(), 0.f);
	return TS->shaped_text_get_underline_thickness(lines_rid[p_line]);
}

Size2 TextParagraph::get_dropcap_size() const
{
	_THREAD_SAFE_METHOD_

	return TS->shaped_text_get_size(dropcap_rid) + dropcap_margins.size + dropcap_margins.position;
}

int TextParagraph::get_dropcap_lines() const { return dropcap_lines; }

void TextParagraph::draw(RID p_canvas, const Vector2& p_pos, const Color& p_color,
	const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			dc_off + Vector2(0, TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.size.y +
									dropcap_margins.position.y / 2),
			-1, -1, p_dc_color, p_oversampling);
	}

	int lines_visible = (max_lines_visible >= 0) ? MIN(max_lines_visible, (int)lines_rid.size())
												 : (int)lines_rid.size();

	for (int i = 0; i < lines_visible; i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float line_width = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (line_width <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - line_width) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - line_width) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - line_width;
					}
					else {
						ofs.y += l_width - line_width;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - line_width;
				}
				else {
					ofs.y += l_width - line_width;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw(
			lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

void TextParagraph::draw_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, const Color& p_dc_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs = p_pos;

	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		Vector2 dc_off = ofs;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				dc_off.x += l_width - h_offset;
			}
			else {
				dc_off.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			dc_off + Vector2(dropcap_margins.position.x,
						 TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_dc_color, p_oversampling);
	}

	for (int i = 0; i < (int)lines_rid.size(); i++) {
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.x -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_ascent(lines_rid[i]);
			if (i <= dropcap_lines) {
				if (TS->shaped_text_get_inferred_direction(dropcap_rid) ==
					TextServer::DIRECTION_LTR) {
					ofs.y -= h_offset;
				}
				l_width -= h_offset;
			}
		}
		float length = TS->shaped_text_get_width(lines_rid[i]);
		if (l_width > 0) {
			switch (alignment) {
			case HORIZONTAL_ALIGNMENT_FILL:
				if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
					TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
				break;
			case HORIZONTAL_ALIGNMENT_LEFT:
				break;
			case HORIZONTAL_ALIGNMENT_CENTER: {
				if (length <= l_width) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += Math::floor((l_width - length) / 2.0);
					}
					else {
						ofs.y += Math::floor((l_width - length) / 2.0);
					}
				}
				else if (TS->shaped_text_get_inferred_direction(lines_rid[i]) ==
						   TextServer::DIRECTION_RTL) {
					if (TS->shaped_text_get_orientation(lines_rid[i]) ==
						TextServer::ORIENTATION_HORIZONTAL) {
						ofs.x += l_width - length;
					}
					else {
						ofs.y += l_width - length;
					}
				}
			} break;
			case HORIZONTAL_ALIGNMENT_RIGHT: {
				if (TS->shaped_text_get_orientation(lines_rid[i]) ==
					TextServer::ORIENTATION_HORIZONTAL) {
					ofs.x += l_width - length;
				}
				else {
					ofs.y += l_width - length;
				}
			} break;
			}
		}
		float clip_l;
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			clip_l = MAX(0, p_pos.x - ofs.x);
		}
		else {
			clip_l = MAX(0, p_pos.y - ofs.y);
		}
		TS->shaped_text_draw_outline(lines_rid[i], p_canvas, ofs, clip_l, clip_l + l_width,
			p_outline_size, p_color, p_oversampling);
		if (TS->shaped_text_get_orientation(lines_rid[i]) == TextServer::ORIENTATION_HORIZONTAL) {
			ofs.x = p_pos.x;
			ofs.y += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
		else {
			ofs.y = p_pos.y;
			ofs.x += TS->shaped_text_get_descent(lines_rid[i]) + line_spacing;
		}
	}
}

int TextParagraph::hit_test(const Point2& p_coords) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	Vector2 ofs;
	if (TS->shaped_text_get_orientation(rid) == TextServer::ORIENTATION_HORIZONTAL) {
		if (ofs.y < 0) {
			return 0;
		}
	}
	else {
		if (ofs.x < 0) {
			return 0;
		}
	}
	for (const RID& line_rid : lines_rid) {
		if (TS->shaped_text_get_orientation(line_rid) == TextServer::ORIENTATION_HORIZONTAL) {
			if ((p_coords.y >= ofs.y) &&
				(p_coords.y <= ofs.y + TS->shaped_text_get_size(line_rid).y)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.x);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).y + line_spacing;
		}
		else {
			if ((p_coords.x >= ofs.x) &&
				(p_coords.x <= ofs.x + TS->shaped_text_get_size(line_rid).x)) {
				return TS->shaped_text_hit_test_position(line_rid, p_coords.y);
			}
			ofs.y += TS->shaped_text_get_size(line_rid).x + line_spacing;
		}
	}
	return TS->shaped_text_get_range(rid).y;
}

bool TextParagraph::is_dirty() { return lines_dirty; }

void TextParagraph::draw_dropcap(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_color, p_oversampling);
	}
}

void TextParagraph::draw_dropcap_outline(RID p_canvas, const Vector2& p_pos, int p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	Vector2 ofs = p_pos;
	float h_offset = 0.f;
	if (TS->shaped_text_get_orientation(dropcap_rid) == TextServer::ORIENTATION_HORIZONTAL) {
		h_offset = TS->shaped_text_get_size(dropcap_rid).x + dropcap_margins.size.x +
				   dropcap_margins.position.x;
	}
	else {
		h_offset = TS->shaped_text_get_size(dropcap_rid).y + dropcap_margins.size.y +
				   dropcap_margins.position.y;
	}

	if (h_offset > 0) {
		// Draw dropcap.
		float l_width = width > 0 ? width : get_size().x;
		if (TS->shaped_text_get_inferred_direction(dropcap_rid) == TextServer::DIRECTION_RTL) {
			if (TS->shaped_text_get_orientation(dropcap_rid) ==
				TextServer::ORIENTATION_HORIZONTAL) {
				ofs.x += l_width - h_offset;
			}
			else {
				ofs.y += l_width - h_offset;
			}
		}
		TS->shaped_text_draw_outline(dropcap_rid, p_canvas,
			ofs + Vector2(dropcap_margins.position.x,
					  TS->shaped_text_get_ascent(dropcap_rid) + dropcap_margins.position.y),
			-1, -1, p_outline_size, p_color, p_oversampling);
	}
}

void TextParagraph::draw_line(RID p_canvas, const Vector2& p_pos, int p_line, const Color& p_color,
	float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;

	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw(lines_rid[p_line], p_canvas, ofs, -1, -1, p_color, p_oversampling);
}

void TextParagraph::draw_line_outline(RID p_canvas, const Vector2& p_pos, int p_line,
	int p_outline_size, const Color& p_color, float p_oversampling) const
{
	_THREAD_SAFE_METHOD_

	_shape_lines();
	ERR_FAIL_COND(p_line < 0 || p_line >= (int)lines_rid.size());

	Vector2 ofs = p_pos;
	if (TS->shaped_text_get_orientation(lines_rid[p_line]) == TextServer::ORIENTATION_HORIZONTAL) {
		ofs.y += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	else {
		ofs.x += TS->shaped_text_get_ascent(lines_rid[p_line]);
	}
	return TS->shaped_text_draw_outline(
		lines_rid[p_line], p_canvas, ofs, -1, -1, p_outline_size, p_color, p_oversampling);
}

TextParagraph::TextParagraph()
{
	rid = TS->create_shaped_text();
	dropcap_rid = TS->create_shaped_text();
}

TextParagraph::~TextParagraph()
{
	for (const RID& line_rid : lines_rid) {
		TS->free_rid(line_rid);
	}
	lines_rid.clear();
	TS->free_rid(rid);
	TS->free_rid(dropcap_rid);
}

void TextParagraph::_shape_lines() const {}

// --- text_paragraph.compat.inc ---

#ifndef DISABLE_DEPRECATED
void TextParagraph::_draw_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color, const Color& p_dc_color) const
{
	draw(p_canvas, p_pos, p_color, p_dc_color, 0.0);
}

void TextParagraph::_draw_outline_bind_compat_104872(RID p_canvas, const Vector2& p_pos,
	int p_outline_size, const Color& p_color, const Color& p_dc_color) const
{
	draw_outline(p_canvas, p_pos, p_outline_size, p_color, p_dc_color, 0.0);
}

void TextParagraph::_draw_line_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_line, const Color& p_color) const
{
	draw_line(p_canvas, p_pos, p_line, p_color, 0.0);
}

void TextParagraph::_draw_line_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_line, int p_outline_size, const Color& p_color) const
{
	draw_line_outline(p_canvas, p_pos, p_line, p_outline_size, p_color, 0.0);
}

void TextParagraph::_draw_dropcap_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, const Color& p_color) const
{
	draw_dropcap(p_canvas, p_pos, p_color, 0.0);
}

void TextParagraph::_draw_dropcap_outline_bind_compat_104872(
	RID p_canvas, const Vector2& p_pos, int p_outline_size, const Color& p_color) const
{
	draw_dropcap_outline(p_canvas, p_pos, p_outline_size, p_color, 0.0);
}

void TextParagraph::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- label_settings.cpp ---

#include "scene/gui/label.h"

void LabelSettings::_font_changed() { emit_changed(); }

void LabelSettings::set_line_spacing(real_t p_spacing)
{
	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_line_spacing() const { return line_spacing; }

void LabelSettings::set_paragraph_spacing(real_t p_spacing)
{
	if (paragraph_spacing != p_spacing) {
		paragraph_spacing = p_spacing;
		emit_changed();
	}
}

real_t LabelSettings::get_paragraph_spacing() const { return paragraph_spacing; }

Ref<Font> LabelSettings::get_font() const { return font; }

void LabelSettings::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_font_size() const { return font_size; }

void LabelSettings::set_font_color(const Color& p_color)
{
	if (font_color != p_color) {
		font_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_font_color() const { return font_color; }

void LabelSettings::set_outline_size(int p_size)
{
	if (outline_size != p_size) {
		outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_outline_size() const { return outline_size; }

void LabelSettings::set_outline_color(const Color& p_color)
{
	if (outline_color != p_color) {
		outline_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_outline_color() const { return outline_color; }

void LabelSettings::set_shadow_size(int p_size)
{
	if (shadow_size != p_size) {
		shadow_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_shadow_size() const { return shadow_size; }

void LabelSettings::set_shadow_color(const Color& p_color)
{
	if (shadow_color != p_color) {
		shadow_color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_shadow_color() const { return shadow_color; }

void LabelSettings::set_shadow_offset(const Vector2& p_offset)
{
	if (shadow_offset != p_offset) {
		shadow_offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_shadow_offset() const { return shadow_offset; }

Vector<LabelSettings::StackedOutlineData> LabelSettings::get_stacked_outline_data() const
{
	return stacked_outline_data;
}

int LabelSettings::get_stacked_outline_count() const { return stacked_outline_data.size(); }

void LabelSettings::set_stacked_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].size != p_size) {
		stacked_outline_data.write[p_index].size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), 0);
	return stacked_outline_data[p_index].size;
}

void LabelSettings::set_stacked_outline_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_outline_data.size());
	if (stacked_outline_data[p_index].color != p_color) {
		stacked_outline_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_outline_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_outline_data.size(), Color());
	return stacked_outline_data[p_index].color;
}

Vector<LabelSettings::StackedShadowData> LabelSettings::get_stacked_shadow_data() const
{
	return stacked_shadow_data;
}

int LabelSettings::get_stacked_shadow_count() const { return stacked_shadow_data.size(); }

void LabelSettings::set_stacked_shadow_offset(int p_index, const Vector2& p_offset)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].offset != p_offset) {
		stacked_shadow_data.write[p_index].offset = p_offset;
		emit_changed();
	}
}

Vector2 LabelSettings::get_stacked_shadow_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Vector2());
	return stacked_shadow_data[p_index].offset;
}

void LabelSettings::set_stacked_shadow_color(int p_index, const Color& p_color)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].color != p_color) {
		stacked_shadow_data.write[p_index].color = p_color;
		emit_changed();
	}
}

Color LabelSettings::get_stacked_shadow_color(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), Color());
	return stacked_shadow_data[p_index].color;
}

void LabelSettings::set_stacked_shadow_outline_size(int p_index, int p_size)
{
	ERR_FAIL_INDEX(p_index, stacked_shadow_data.size());
	if (stacked_shadow_data[p_index].outline_size != p_size) {
		stacked_shadow_data.write[p_index].outline_size = p_size;
		emit_changed();
	}
}

int LabelSettings::get_stacked_shadow_outline_size(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, stacked_shadow_data.size(), 0);
	return stacked_shadow_data[p_index].outline_size;
}

// --- text_file.cpp ---

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
// --- Inlined from scene/resources/text_file.h ---
#include "core/io/resource.h"

class TextFile : public Resource
{
private:
	String text;
	String path;

protected:
	virtual bool editor_can_reload_from_file() override { return false; }

public:
	virtual bool has_text() const;
	virtual String get_text() const;
	virtual void set_text(const String& p_code);
	virtual void reload_from_file() override;

	void set_file_path(const String& p_path) { path = p_path; }

	Error load_text(const String& p_path);
};



// --- End scene/resources/text_file.h ---

bool TextFile::has_text() const { return !text.is_empty(); }

String TextFile::get_text() const { return text; }

void TextFile::set_text(const String& p_code) { text = p_code; }

void TextFile::reload_from_file() { load_text(path); }



Error TextFile::load_text(String const&) {}

// --- syntax_highlighter.cpp ---

#include "scene/gui/text_edit.h"

void CodeHighlighter::_clear_highlighting_cache() { color_region_cache.clear(); }

void CodeHighlighter::clear_highlighting_cache() { _clear_highlighting_cache(); }

void CodeHighlighter::_update_cache() { font_color = text_edit->get_font_color(); }

void CodeHighlighter::update_cache()
{
	clear_highlighting_cache();

	if (text_edit == nullptr) {
		return;
	}
	_update_cache();
}

void CodeHighlighter::set_text_edit(TextEdit* p_text_edit) { text_edit = p_text_edit; }

TextEdit* CodeHighlighter::get_text_edit() const { return text_edit; }

void CodeHighlighter::add_color_region(
	const String& p_start_key, const String& p_end_key, const Color& p_color, bool p_line_only)
{
	for (int i = 0; i < p_start_key.length(); i++) {
		ERR_FAIL_COND_MSG(!is_symbol(p_start_key[i]), "color regions must start with a symbol");
	}

	if (p_end_key.length() > 0) {
		for (int i = 0; i < p_end_key.length(); i++) {
			ERR_FAIL_COND_MSG(!is_symbol(p_end_key[i]), "color regions must end with a symbol");
		}
	}

	int at = 0;
	for (int i = 0; i < color_regions.size(); i++) {
		ERR_FAIL_COND_MSG(color_regions[i].start_key == p_start_key,
			"color region with start key '" + p_start_key + "' already exists.");
		if (p_start_key.length() < color_regions[i].start_key.length()) {
			at++;
		}
	}

	ColorRegion color_region;
	color_region.color = p_color;
	color_region.start_key = p_start_key;
	color_region.end_key = p_end_key;
	color_region.line_only = p_line_only || p_end_key.is_empty();
	color_regions.insert(at, color_region);
	clear_highlighting_cache();
}

void CodeHighlighter::remove_color_region(const String& p_start_key)
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			color_regions.remove_at(i);
			break;
		}
	}
	clear_highlighting_cache();
}

bool CodeHighlighter::has_color_region(const String& p_start_key) const
{
	for (int i = 0; i < color_regions.size(); i++) {
		if (color_regions[i].start_key == p_start_key) {
			return true;
		}
	}
	return false;
}

void CodeHighlighter::clear_color_regions()
{
	color_regions.clear();
	clear_highlighting_cache();
}

void CodeHighlighter::set_uint_suffix_enabled(bool p_enabled) { uint_suffix_enabled = p_enabled; }

void CodeHighlighter::set_number_color(Color p_color)
{
	number_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_number_color() const { return number_color; }

void CodeHighlighter::set_symbol_color(Color p_color)
{
	symbol_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_symbol_color() const { return symbol_color; }

void CodeHighlighter::set_function_color(Color p_color)
{
	function_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_function_color() const { return function_color; }

void CodeHighlighter::set_member_variable_color(Color p_color)
{
	member_color = p_color;
	clear_highlighting_cache();
}

Color CodeHighlighter::get_member_variable_color() const { return member_color; }

// ============================================================================
// Merged from scene/resources/theme.cpp
// ============================================================================

#include "scene/theme/theme_db.h"
#include "theme.h"

bool Theme::is_valid_type_name(const String& p_name)
{
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

bool Theme::is_valid_item_name(const String& p_name)
{
	if (p_name.is_empty()) {
		return false;
	}
	int len = p_name.length();
	const char32_t* str = p_name.ptr();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}
	return true;
}

String Theme::validate_type_name(const String& p_name)
{
	String type_name = p_name.strip_edges();
	int len = type_name.length();
	char32_t* buffer = type_name.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(buffer[i])) {
			buffer[i] = '_';
		}
	}
	return type_name;
}

// Fallback values for theme item types, configurable per theme.
void Theme::set_default_base_scale(float p_base_scale)
{
	if (default_base_scale == p_base_scale) {
		return;
	}

	default_base_scale = p_base_scale;

	_emit_theme_changed();
}

float Theme::get_default_base_scale() const { return default_base_scale; }

bool Theme::has_default_base_scale() const { return default_base_scale > 0.0; }

Ref<Font> Theme::get_default_font() const { return default_font; }

bool Theme::has_default_font() const { return default_font.is_valid(); }

void Theme::set_default_font_size(int p_font_size)
{
	if (default_font_size == p_font_size) {
		return;
	}

	default_font_size = p_font_size;

	_emit_theme_changed();
}

int Theme::get_default_font_size() const { return default_font_size; }

bool Theme::has_default_font_size() const { return default_font_size > 0; }

Ref<Texture2D> Theme::get_icon(const String& p_name, const String& p_theme_type) const
{
	if (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
		icon_map[p_theme_type][p_name].is_valid()) {
		return icon_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_icon();
	}
}

bool Theme::has_icon(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name) &&
			icon_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_icon_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (icon_map.has(p_theme_type) && icon_map[p_theme_type].has(p_name));
}

void Theme::rename_icon(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!icon_map.has(p_theme_type), "Cannot rename the icon '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(icon_map[p_theme_type].has(p_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!icon_map[p_theme_type].has(p_old_name),
		"Cannot rename the icon '" + String(p_old_name) + "' because it does not exist.");

	icon_map[p_theme_type][p_name] = icon_map[p_theme_type][p_old_name];
	icon_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_icon_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!icon_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Texture2D>>& E : icon_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_icon_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (icon_map.has(p_theme_type)) {
		return;
	}
	icon_map[p_theme_type] = ThemeIconMap();
}

void Theme::rename_icon_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!icon_map.has(p_old_theme_type) || icon_map.has(p_theme_type)) {
		return;
	}

	icon_map[p_theme_type] = icon_map[p_old_theme_type];
	icon_map.erase(p_old_theme_type);
}

void Theme::get_icon_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		p_list->push_back(E.key);
	}
}

Ref<StyleBox> Theme::get_stylebox(const String& p_name, const String& p_theme_type) const
{
	if (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
		style_map[p_theme_type][p_name].is_valid()) {
		return style_map[p_theme_type][p_name];
	}
	else {
		return ThemeDB::get_fallback_stylebox();
	}
}

bool Theme::has_stylebox(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name) &&
			style_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_stylebox_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (style_map.has(p_theme_type) && style_map[p_theme_type].has(p_name));
}

void Theme::rename_stylebox(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!style_map.has(p_theme_type),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(style_map[p_theme_type].has(p_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!style_map[p_theme_type].has(p_old_name),
		"Cannot rename the stylebox '" + String(p_old_name) + "' because it does not exist.");

	style_map[p_theme_type][p_name] = style_map[p_theme_type][p_old_name];
	style_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_stylebox_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!style_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<StyleBox>>& E : style_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_stylebox_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (style_map.has(p_theme_type)) {
		return;
	}
	style_map[p_theme_type] = ThemeStyleMap();
}

void Theme::rename_stylebox_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!style_map.has(p_old_theme_type) || style_map.has(p_theme_type)) {
		return;
	}

	style_map[p_theme_type] = style_map[p_old_theme_type];
	style_map.erase(p_old_theme_type);
}

void Theme::get_stylebox_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		p_list->push_back(E.key);
	}
}

// Fonts.

Ref<Font> Theme::get_font(const String& p_name, const String& p_theme_type) const
{
	if (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
		font_map[p_theme_type][p_name].is_valid()) {
		return font_map[p_theme_type][p_name];
	}
	else if (has_default_font()) {
		return default_font;
	}
	else {
		return ThemeDB::get_fallback_font();
	}
}

bool Theme::has_font(const String& p_name, const String& p_theme_type) const
{
	return ((font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
				font_map[p_theme_type][p_name].is_valid()) ||
			has_default_font());
}

bool Theme::has_font_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name) &&
			font_map[p_theme_type][p_name].is_valid());
}

bool Theme::has_font_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_map.has(p_theme_type) && font_map[p_theme_type].has(p_name));
}

void Theme::rename_font(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_map.has(p_theme_type), "Cannot rename the font '" + String(p_old_name) +
													   "' because the node type '" +
													   String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_map[p_theme_type].has(p_name),
		"Cannot rename the font '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_map[p_theme_type].has(p_old_name),
		"Cannot rename the font '" + String(p_old_name) + "' because it does not exist.");

	font_map[p_theme_type][p_name] = font_map[p_theme_type][p_old_name];
	font_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::get_font_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Ref<Font>>& E : font_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_map.has(p_theme_type)) {
		return;
	}
	font_map[p_theme_type] = ThemeFontMap();
}

void Theme::rename_font_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_map.has(p_old_theme_type) || font_map.has(p_theme_type)) {
		return;
	}

	font_map[p_theme_type] = font_map[p_old_theme_type];
	font_map.erase(p_old_theme_type);
}

void Theme::get_font_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		p_list->push_back(E.key);
	}
}

// Font sizes.
void Theme::set_font_size(const String& p_name, const String& p_theme_type, int p_font_size)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_font_size_nocheck(p_name, p_theme_type);
	font_size_map[p_theme_type][p_name] = p_font_size;

	_emit_theme_changed(!existing);
}

int Theme::get_font_size(const String& p_name, const String& p_theme_type) const
{
	if (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
		(font_size_map[p_theme_type][p_name] > 0)) {
		return font_size_map[p_theme_type][p_name];
	}
	else if (has_default_font_size()) {
		return default_font_size;
	}
	else {
		return ThemeDB::get_fallback_font_size();
	}
}

bool Theme::has_font_size(const String& p_name, const String& p_theme_type) const
{
	return ((font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
				(font_size_map[p_theme_type][p_name] > 0)) ||
			has_default_font_size());
}

bool Theme::has_font_size_no_default(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name) &&
			(font_size_map[p_theme_type][p_name] > 0));
}

bool Theme::has_font_size_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (font_size_map.has(p_theme_type) && font_size_map[p_theme_type].has(p_name));
}

void Theme::rename_font_size(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot rename the font size '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(font_size_map[p_theme_type].has(p_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_old_name),
		"Cannot rename the font size '" + String(p_old_name) + "' because it does not exist.");

	font_size_map[p_theme_type][p_name] = font_size_map[p_theme_type][p_old_name];
	font_size_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_font_size(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!font_size_map.has(p_theme_type),
		"Cannot clear the font size '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!font_size_map[p_theme_type].has(p_name),
		"Cannot clear the font size '" + String(p_name) + "' because it does not exist.");

	font_size_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_font_size_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : font_size_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_font_size_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (font_size_map.has(p_theme_type)) {
		return;
	}
	font_size_map[p_theme_type] = ThemeFontSizeMap();
}

void Theme::remove_font_size_type(const String& p_theme_type)
{
	if (!font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map.erase(p_theme_type);
}

void Theme::rename_font_size_type(
	const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!font_size_map.has(p_old_theme_type) || font_size_map.has(p_theme_type)) {
		return;
	}

	font_size_map[p_theme_type] = font_size_map[p_old_theme_type];
	font_size_map.erase(p_old_theme_type);
}

void Theme::get_font_size_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		p_list->push_back(E.key);
	}
}

// Colors.
void Theme::set_color(
	const String& p_name, const String& p_theme_type, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_color_nocheck(p_name, p_theme_type);
	color_map[p_theme_type][p_name] = p_color;

	_emit_theme_changed(!existing);
}

Color Theme::get_color(const String& p_name, const String& p_theme_type) const
{
	if (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name)) {
		return color_map[p_theme_type][p_name];
	}
	else {
		return Color();
	}
}

bool Theme::has_color(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

bool Theme::has_color_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (color_map.has(p_theme_type) && color_map[p_theme_type].has(p_name));
}

void Theme::rename_color(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type),
		"Cannot rename the color '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(color_map[p_theme_type].has(p_name),
		"Cannot rename the color '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_old_name),
		"Cannot rename the color '" + String(p_old_name) + "' because it does not exist.");

	color_map[p_theme_type][p_name] = color_map[p_theme_type][p_old_name];
	color_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_color(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!color_map.has(p_theme_type), "Cannot clear the color '" + String(p_name) +
														"' because the node type '" +
														String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!color_map[p_theme_type].has(p_name),
		"Cannot clear the color '" + String(p_name) + "' because it does not exist.");

	color_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_color_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!color_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, Color>& E : color_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_color_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (color_map.has(p_theme_type)) {
		return;
	}
	color_map[p_theme_type] = ThemeColorMap();
}

void Theme::remove_color_type(const String& p_theme_type)
{
	if (!color_map.has(p_theme_type)) {
		return;
	}

	color_map.erase(p_theme_type);
}

void Theme::rename_color_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!color_map.has(p_old_theme_type) || color_map.has(p_theme_type)) {
		return;
	}

	color_map[p_theme_type] = color_map[p_old_theme_type];
	color_map.erase(p_old_theme_type);
}

void Theme::get_color_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		p_list->push_back(E.key);
	}
}

// Theme constants.
void Theme::set_constant(const String& p_name, const String& p_theme_type, int p_constant)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	bool existing = has_constant_nocheck(p_name, p_theme_type);
	constant_map[p_theme_type][p_name] = p_constant;

	_emit_theme_changed(!existing);
}

int Theme::get_constant(const String& p_name, const String& p_theme_type) const
{
	if (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name)) {
		return constant_map[p_theme_type][p_name];
	}
	else {
		return 0;
	}
}

bool Theme::has_constant(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

bool Theme::has_constant_nocheck(const String& p_name, const String& p_theme_type) const
{
	return (constant_map.has(p_theme_type) && constant_map[p_theme_type].has(p_name));
}

void Theme::rename_constant(
	const String& p_old_name, const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!is_valid_item_name(p_name), vformat("Invalid item name: '%s'", p_name));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot rename the constant '" + String(p_old_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(constant_map[p_theme_type].has(p_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because the new name '" +
			String(p_name) + "' already exists.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_old_name),
		"Cannot rename the constant '" + String(p_old_name) + "' because it does not exist.");

	constant_map[p_theme_type][p_name] = constant_map[p_theme_type][p_old_name];
	constant_map[p_theme_type].erase(p_old_name);

	_emit_theme_changed(true);
}

void Theme::clear_constant(const String& p_name, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!constant_map.has(p_theme_type),
		"Cannot clear the constant '" + String(p_name) + "' because the node type '" +
			String(p_theme_type) + "' does not exist.");
	ERR_FAIL_COND_MSG(!constant_map[p_theme_type].has(p_name),
		"Cannot clear the constant '" + String(p_name) + "' because it does not exist.");

	constant_map[p_theme_type].erase(p_name);

	_emit_theme_changed(true);
}

void Theme::get_constant_list(const String& p_theme_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!constant_map.has(p_theme_type)) {
		return;
	}

	for (const KeyValue<String, int>& E : constant_map[p_theme_type]) {
		p_list->push_back(E.key);
	}
}

void Theme::add_constant_type(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (constant_map.has(p_theme_type)) {
		return;
	}
	constant_map[p_theme_type] = ThemeConstantMap();
}

void Theme::remove_constant_type(const String& p_theme_type)
{
	if (!constant_map.has(p_theme_type)) {
		return;
	}

	constant_map.erase(p_theme_type);
}

void Theme::rename_constant_type(const String& p_old_theme_type, const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));

	if (!constant_map.has(p_old_theme_type) || constant_map.has(p_theme_type)) {
		return;
	}

	constant_map[p_theme_type] = constant_map[p_old_theme_type];
	constant_map.erase(p_old_theme_type);
}

void Theme::get_constant_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		p_list->push_back(E.key);
	}
}

bool Theme::has_theme_item(
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		if (!variation_map.has(p_theme_type)) {
			return has_font(p_name, p_theme_type);
		}
		else {
			return has_font_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_FONT_SIZE:
		if (!variation_map.has(p_theme_type)) {
			return has_font_size(p_name, p_theme_type);
		}
		else {
			return has_font_size_no_default(p_name, p_theme_type);
		}
	case DATA_TYPE_ICON:
		return has_icon(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

bool Theme::has_theme_item_nocheck(
	DataType p_data_type, const String& p_name, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return has_color_nocheck(p_name, p_theme_type);
	case DATA_TYPE_CONSTANT:
		return has_constant_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT:
		return has_font_nocheck(p_name, p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return has_font_size_nocheck(p_name, p_theme_type);
	case DATA_TYPE_ICON:
		return has_icon_nocheck(p_name, p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return has_stylebox_nocheck(p_name, p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return false;
}

void Theme::rename_theme_item(DataType p_data_type, const String& p_old_name,
	const String& p_name, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox(p_old_name, p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::clear_theme_item(
	DataType p_data_type, const String& p_name, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		clear_color(p_name, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		clear_constant(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		clear_font(p_name, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		clear_font_size(p_name, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		clear_icon(p_name, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		clear_stylebox(p_name, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_list(
	DataType p_data_type, const String& p_theme_type, List<String>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_list(p_theme_type, p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::add_theme_item_type(DataType p_data_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		add_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		add_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		add_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		add_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		add_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		add_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::remove_theme_item_type(DataType p_data_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		remove_color_type(p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		remove_constant_type(p_theme_type);
		break;
	case DATA_TYPE_FONT:
		remove_font_type(p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		remove_font_size_type(p_theme_type);
		break;
	case DATA_TYPE_ICON:
		remove_icon_type(p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		remove_stylebox_type(p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::rename_theme_item_type(
	DataType p_data_type, const String& p_old_theme_type, const String& p_theme_type)
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		rename_color_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_CONSTANT:
		rename_constant_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT:
		rename_font_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_FONT_SIZE:
		rename_font_size_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_ICON:
		rename_icon_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_STYLEBOX:
		rename_stylebox_type(p_old_theme_type, p_theme_type);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

void Theme::get_theme_item_type_list(DataType p_data_type, List<String>* p_list) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		get_color_type_list(p_list);
		break;
	case DATA_TYPE_CONSTANT:
		get_constant_type_list(p_list);
		break;
	case DATA_TYPE_FONT:
		get_font_type_list(p_list);
		break;
	case DATA_TYPE_FONT_SIZE:
		get_font_size_type_list(p_list);
		break;
	case DATA_TYPE_ICON:
		get_icon_type_list(p_list);
		break;
	case DATA_TYPE_STYLEBOX:
		get_stylebox_type_list(p_list);
		break;
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}
}

// Theme type variations.
void Theme::set_type_variation(const String& p_theme_type, const String& p_base_type)
{
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_theme_type), vformat("Invalid type name: '%s'", p_theme_type));
	ERR_FAIL_COND_MSG(
		!is_valid_type_name(p_base_type), vformat("Invalid type name: '%s'", p_base_type));
	ERR_FAIL_COND_MSG(p_theme_type == String(),
		vformat("An empty theme type cannot be marked as a variation of another type (\"%s\").",
			p_base_type));
	ERR_FAIL_COND_MSG(
		p_base_type == String(), "An empty theme type cannot be the base type of a variation. "
									 "Use clear_type_variation() instead if you want to unmark '" +
										 String(p_theme_type) + "' as a variation.");

	if (variation_map.has(p_theme_type)) {
		String old_base = variation_map[p_theme_type];
		variation_base_map[old_base].erase(p_theme_type);
	}

	variation_map[p_theme_type] = p_base_type;
	variation_base_map[p_base_type].push_back(p_theme_type);

	_emit_theme_changed(true);
}

bool Theme::is_type_variation(const String& p_theme_type, const String& p_base_type) const
{
	return (variation_map.has(p_theme_type) && variation_map[p_theme_type] == p_base_type);
}

void Theme::clear_type_variation(const String& p_theme_type)
{
	ERR_FAIL_COND_MSG(!variation_map.has(p_theme_type), "Cannot clear the type variation '" +
															String(p_theme_type) +
															"' because it does not exist.");

	String base_type = variation_map[p_theme_type];
	variation_base_map[base_type].erase(p_theme_type);
	variation_map.erase(p_theme_type);

	_emit_theme_changed(true);
}

void Theme::_emit_theme_changed(bool p_notify_list_changed) {}

String Theme::get_type_variation_base(const String& p_theme_type) const
{
	if (!variation_map.has(p_theme_type)) {
		return String();
	}

	return variation_map[p_theme_type];
}

void Theme::get_type_variation_list(const String& p_base_type, List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	if (!variation_base_map.has(p_base_type)) {
		return;
	}

	for (const String& E : variation_base_map[p_base_type]) {
		// Prevent infinite loops if variants were set to be cross-dependent (that's still invalid
		// usage, but handling for stability sake).
		if (p_list->find(E)) {
			continue;
		}

		p_list->push_back(E);
		// Continue looking for sub-variations.
		get_type_variation_list(E, p_list);
	}
}

// Theme types.
void Theme::add_type(const String& p_theme_type)
{
	// Add a record to every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		add_theme_item_type(dt, p_theme_type);
	}

	_emit_theme_changed(true);
}

void Theme::remove_type(const String& p_theme_type)
{
	// Gracefully remove the record from every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		remove_theme_item_type(dt, p_theme_type);
	}

	// If type is a variation, remove that connection.
	if (get_type_variation_base(p_theme_type) != String()) {
		clear_type_variation(p_theme_type);
	}

	// If type is a variation base, remove all those connections.
	List<String> names;
	get_type_variation_list(p_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
	}

	_emit_theme_changed(true);
}

void Theme::rename_type(const String& p_old_theme_type, const String& p_theme_type)
{
	// Gracefully rename the record in every data type map.
	for (int i = 0; i < Theme::DATA_TYPE_MAX; i++) {
		Theme::DataType dt = (Theme::DataType)i;
		rename_theme_item_type(dt, p_old_theme_type, p_theme_type);
	}

	// If type is a variation, replace that connection.
	const String base_type = get_type_variation_base(p_old_theme_type);
	if (base_type != String()) {
		clear_type_variation(p_old_theme_type);
		if (p_theme_type != String()) {
			set_type_variation(p_theme_type, base_type);
		}
	}

	// If type is a variation base, replace all those connections.
	List<String> names;
	get_type_variation_list(p_old_theme_type, &names);
	for (const String& E : names) {
		clear_type_variation(E);
		if (p_theme_type != String()) {
			set_type_variation(E, p_theme_type);
		}
	}

	_emit_theme_changed(true);
}

void Theme::get_type_list(List<String>* p_list) const
{
	ERR_FAIL_NULL(p_list);

	// This Set guarantees uniqueness.
	// Because each map can have the same type defined, but for this method
	// we only want one occurrence of each type.
	HashSet<String> types;

	// Icons.
	for (const KeyValue<String, ThemeIconMap>& E : icon_map) {
		types.insert(E.key);
	}

	// Styles.
	for (const KeyValue<String, ThemeStyleMap>& E : style_map) {
		types.insert(E.key);
	}

	// Fonts.
	for (const KeyValue<String, ThemeFontMap>& E : font_map) {
		types.insert(E.key);
	}

	// Font sizes.
	for (const KeyValue<String, ThemeFontSizeMap>& E : font_size_map) {
		types.insert(E.key);
	}

	// Colors.
	for (const KeyValue<String, ThemeColorMap>& E : color_map) {
		types.insert(E.key);
	}

	// Constants.
	for (const KeyValue<String, ThemeConstantMap>& E : constant_map) {
		types.insert(E.key);
	}

	// Variations.
	for (const KeyValue<String, String>& E : variation_map) {
		types.insert(E.key);
	}

	for (const String& E : types) {
		p_list->push_back(E);
	}
}

void Theme::get_type_dependencies(
	const String& p_base_type, const String& p_type_variation, Vector<String>& r_result)
{
	// Build the dependency chain for type variations.
	if (p_type_variation != String()) {
		String variation_name = p_type_variation;
		while (variation_name != String()) {
			r_result.push_back(variation_name);
			variation_name = get_type_variation_base(variation_name);

			// If we have reached the base type dependency, it's safe to stop (assuming no funny
			// business was done to the Theme).
			if (variation_name == p_base_type) {
				break;
			}
		}
	}

	// Continue building the chain using native class hierarchy.
	ThemeDB::get_native_type_dependencies(p_base_type, r_result);
}

// Internal methods for getting lists as a Vector of String (compatible with public API).
Vector<String> Theme::_get_icon_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_icon_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_icon_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_stylebox_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_stylebox_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_font_size_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_font_size_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_color_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_color_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_color_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_constant_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_constant_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_theme_item_list(DataType p_data_type, const String& p_theme_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_list(p_theme_type);
	case DATA_TYPE_CONSTANT:
		return _get_constant_list(p_theme_type);
	case DATA_TYPE_FONT:
		return _get_font_list(p_theme_type);
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_list(p_theme_type);
	case DATA_TYPE_ICON:
		return _get_icon_list(p_theme_type);
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_list(p_theme_type);
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_theme_item_type_list(DataType p_data_type) const
{
	switch (p_data_type) {
	case DATA_TYPE_COLOR:
		return _get_color_type_list();
	case DATA_TYPE_CONSTANT:
		return _get_constant_type_list();
	case DATA_TYPE_FONT:
		return _get_font_type_list();
	case DATA_TYPE_FONT_SIZE:
		return _get_font_size_type_list();
	case DATA_TYPE_ICON:
		return _get_icon_type_list();
	case DATA_TYPE_STYLEBOX:
		return _get_stylebox_type_list();
	case DATA_TYPE_MAX:
		break; // Can't happen, but silences warning.
	}

	return Vector<String>();
}

Vector<String> Theme::_get_type_variation_list(const String& p_theme_type) const
{
	Vector<String> ilret;
	List<String> il;

	get_type_variation_list(p_theme_type, &il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

Vector<String> Theme::_get_type_list() const
{
	Vector<String> ilret;
	List<String> il;

	get_type_list(&il);
	ilret.resize(il.size());

	int i = 0;
	String* w = ilret.ptrw();
	for (List<String>::Element* E = il.front(); E; E = E->next(), i++) {
		w[i] = E->get();
	}
	return ilret;
}

void Theme::_freeze_change_propagation() { no_change_propagation = true; }

void Theme::_unfreeze_and_propagate_changes()
{
	no_change_propagation = false;
	_emit_theme_changed(true);
}

void Theme::merge_with(const Ref<Theme>& p_other)
{
	if (p_other.is_null()) {
		return;
	}

	_freeze_change_propagation();

	// Colors.
	{
		for (const KeyValue<String, ThemeColorMap>& E : p_other->color_map) {
			for (const KeyValue<String, Color>& F : E.value) {
				set_color(F.key, E.key, F.value);
			}
		}
	}

	// Constants.
	{
		for (const KeyValue<String, ThemeConstantMap>& E : p_other->constant_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_constant(F.key, E.key, F.value);
			}
		}
	}

	// Fonts.
	{
		for (const KeyValue<String, ThemeFontMap>& E : p_other->font_map) {
			for (const KeyValue<String, Ref<Font>>& F : E.value) {
				set_font(F.key, E.key, F.value);
			}
		}
	}

	// Font sizes.
	{
		for (const KeyValue<String, ThemeFontSizeMap>& E : p_other->font_size_map) {
			for (const KeyValue<String, int>& F : E.value) {
				set_font_size(F.key, E.key, F.value);
			}
		}
	}

	// Icons.
	{
		for (const KeyValue<String, ThemeIconMap>& E : p_other->icon_map) {
			for (const KeyValue<String, Ref<Texture2D>>& F : E.value) {
				set_icon(F.key, E.key, F.value);
			}
		}
	}

	// Type variations.
	{
		for (const KeyValue<String, String>& E : p_other->variation_map) {
			set_type_variation(E.key, E.value);
		}
	}

	// Defaults.
	if (p_other->has_default_font()) {
		set_default_font(p_other->default_font);
	}
	if (p_other->has_default_font_size()) {
		set_default_font_size(p_other->default_font_size);
	}
	if (p_other->has_default_base_scale()) {
		set_default_base_scale(p_other->default_base_scale);
	}

	_unfreeze_and_propagate_changes();
}

void Theme::reset_state() { clear(); }

Theme::Theme() {}

Theme::~Theme() {}



void Theme::set_icon(String const&, String const&, Ref<Texture2D> const&) {}

void Theme::set_font(String const&, String const&, Ref<Font> const&) {}

void Theme::set_default_font(Ref<Font> const&) {}

void Theme::remove_stylebox_type(String const&) {}

void Theme::remove_icon_type(String const&) {}

void Theme::remove_font_type(String const&) {}

void Theme::clear_stylebox(String const&, String const&) {}

void Theme::clear_icon(String const&, String const&) {}

void Theme::clear_font(String const&, String const&) {}

void Theme::clear() {}

// --- style_box.cpp ---

#include "style_box.h"

#include <cfloat> // FLT_EPSILON
#include "core/config/engine.h"
#include "scene/main/canvas_item.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"

void StyleBox::set_style_type(StyleType p_type)
{
	if (style_type == p_type) {
		return;
	}
	style_type = p_type;
	emit_changed();
}

float StyleBox::get_style_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	switch (style_type) {
	case STYLE_EMPTY:
		return 0.0;
	case STYLE_FLAT:
		return border_width[p_side];
	case STYLE_TEXTURE:
		return texture_margin[p_side];
	case STYLE_LINE:
		if (vertical) {
			if (p_side == SIDE_LEFT || p_side == SIDE_RIGHT) {
				return thickness / 2.0;
			}
		}
		else if (p_side == SIDE_TOP || p_side == SIDE_BOTTOM) {
			return thickness / 2.0;
		}
		return 0.0;
	}
	return 0.0;
}

Size2 StyleBox::get_minimum_size() const
{
	Size2 min_size = Size2(get_margin(SIDE_LEFT) + get_margin(SIDE_RIGHT),
		get_margin(SIDE_TOP) + get_margin(SIDE_BOTTOM));
	Size2 custom_size;
	if (min_size.x < custom_size.x) {
		min_size.x = custom_size.x;
	}
	if (min_size.y < custom_size.y) {
		min_size.y = custom_size.y;
	}

	return min_size;
}

void StyleBox::set_content_margin(Side p_side, float p_value)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	content_margin[p_side] = p_value;
	emit_changed();
}

void StyleBox::set_content_margin_all(float p_value)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = p_value;
	}
	emit_changed();
}

void StyleBox::set_content_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	content_margin[SIDE_LEFT] = p_left;
	content_margin[SIDE_TOP] = p_top;
	content_margin[SIDE_RIGHT] = p_right;
	content_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_content_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return content_margin[p_side];
}

float StyleBox::get_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	if (content_margin[p_side] < 0) {
		return get_style_margin(p_side);
	}
	else {
		return content_margin[p_side];
	}
}

Point2 StyleBox::get_offset() const { return Point2(get_margin(SIDE_LEFT), get_margin(SIDE_TOP)); }

CanvasItem* StyleBox::get_current_item_drawn() const
{
	return CanvasItem::get_current_item_drawn();
}

StyleBox::StyleBox(StyleType p_type) : style_type(p_type)
{
	for (int i = 0; i < 4; i++) {
		content_margin[i] = -1;
	}
}

Rect2 StyleBox::get_draw_rect(const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_FLAT: {
		Rect2 draw_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);

		if (shadow_size > 0) {
			Rect2 shadow_rect = draw_rect.grow(shadow_size);
			shadow_rect.position += shadow_offset;
			draw_rect = draw_rect.merge(shadow_rect);
		}

		return draw_rect;
	}
	case STYLE_TEXTURE:
		return p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
			expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	case STYLE_EMPTY:
	case STYLE_LINE:
	default:
		return p_rect;
	}
}

void StyleBox::draw(RID p_canvas_item, const Rect2& p_rect) const
{
	switch (style_type) {
	case STYLE_EMPTY:
		return;
	case STYLE_FLAT:
		_draw_flat(p_canvas_item, p_rect);
		return;
	case STYLE_TEXTURE:
		_draw_texture(p_canvas_item, p_rect);
		return;
	case STYLE_LINE:
		_draw_line(p_canvas_item, p_rect);
		return;
	}
}

bool StyleBox::test_mask(const Vector2& p_point, const Rect2& p_rect) const { return true; }

// Shared STYLE_FLAT / STYLE_TEXTURE properties.

void StyleBox::set_expand_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	expand_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_expand_margin_all(float p_expand_margin_size)
{
	for (int i = 0; i < 4; i++) {
		expand_margin[i] = p_expand_margin_size;
	}
	emit_changed();
}

void StyleBox::set_expand_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	expand_margin[SIDE_LEFT] = p_left;
	expand_margin[SIDE_TOP] = p_top;
	expand_margin[SIDE_RIGHT] = p_right;
	expand_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_expand_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);
	return expand_margin[p_side];
}

void StyleBox::set_draw_center(bool p_enabled)
{
	draw_center = p_enabled;
	emit_changed();
}

bool StyleBox::is_draw_center_enabled() const { return draw_center; }

// STYLE_FLAT properties & drawing.

void StyleBox::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	emit_changed();
}

Color StyleBox::get_bg_color() const { return bg_color; }

void StyleBox::set_border_color(const Color& p_color)
{
	border_color = p_color;
	emit_changed();
}

Color StyleBox::get_border_color() const { return border_color; }

void StyleBox::set_border_width_all(int p_size)
{
	border_width[0] = p_size;
	border_width[1] = p_size;
	border_width[2] = p_size;
	border_width[3] = p_size;
	emit_changed();
}

int StyleBox::get_border_width_min() const
{
	return MIN(MIN(border_width[0], border_width[1]), MIN(border_width[2], border_width[3]));
}

void StyleBox::set_border_width(Side p_side, int p_width)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	border_width[p_side] = p_width;
	emit_changed();
}

int StyleBox::get_border_width(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return border_width[p_side];
}

void StyleBox::set_border_blend(bool p_blend)
{
	blend_border = p_blend;
	emit_changed();
}

bool StyleBox::get_border_blend() const { return blend_border; }

void StyleBox::set_corner_radius(const Corner p_corner, const int radius)
{
	ERR_FAIL_INDEX((int)p_corner, 4);
	corner_radius[p_corner] = radius;
	emit_changed();
}

void StyleBox::set_corner_radius_all(int radius)
{
	for (int i = 0; i < 4; i++) {
		corner_radius[i] = radius;
	}

	emit_changed();
}

void StyleBox::set_corner_radius_individual(const int radius_top_left,
	const int radius_top_right, const int radius_bottom_right, const int radius_bottom_left)
{
	corner_radius[0] = radius_top_left;
	corner_radius[1] = radius_top_right;
	corner_radius[2] = radius_bottom_right;
	corner_radius[3] = radius_bottom_left;

	emit_changed();
}

int StyleBox::get_corner_radius(const Corner p_corner) const
{
	ERR_FAIL_INDEX_V((int)p_corner, 4, 0);
	return corner_radius[p_corner];
}

void StyleBox::set_corner_detail(const int& p_corner_detail)
{
	corner_detail = CLAMP(p_corner_detail, 1, 20);
	emit_changed();
}

int StyleBox::get_corner_detail() const { return corner_detail; }

void StyleBox::set_skew(Vector2 p_skew)
{
	skew = p_skew;
	emit_changed();
}

Vector2 StyleBox::get_skew() const { return skew; }

void StyleBox::set_shadow_color(const Color& p_color)
{
	shadow_color = p_color;
	emit_changed();
}

Color StyleBox::get_shadow_color() const { return shadow_color; }

void StyleBox::set_shadow_size(const int& p_size)
{
	shadow_size = p_size;
	emit_changed();
}

int StyleBox::get_shadow_size() const { return shadow_size; }

void StyleBox::set_shadow_offset(const Point2& p_offset)
{
	shadow_offset = p_offset;
	emit_changed();
}

Point2 StyleBox::get_shadow_offset() const { return shadow_offset; }

void StyleBox::set_anti_aliased(const bool& p_anti_aliased)
{
	anti_aliased = p_anti_aliased;
	emit_changed();
}

bool StyleBox::is_anti_aliased() const { return anti_aliased; }

void StyleBox::set_aa_size(const real_t p_aa_size)
{
	aa_size = CLAMP(p_aa_size, 0.01, 10);
	emit_changed();
}

real_t StyleBox::get_aa_size() const { return aa_size; }

static inline void set_inner_corner_radius(const Rect2 style_rect, const Rect2 inner_rect,
	const real_t corner_radius[4], real_t* inner_corner_radius)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	inner_corner_radius[0] = MAX(corner_radius[0] - MIN(border_top, border_left), 0);  // Top left.
	inner_corner_radius[1] = MAX(corner_radius[1] - MIN(border_top, border_right), 0); // Top right.
	inner_corner_radius[2] =
		MAX(corner_radius[2] - MIN(border_bottom, border_right), 0); // Bottom right.
	inner_corner_radius[3] =
		MAX(corner_radius[3] - MIN(border_bottom, border_left), 0); // Bottom left.
}

static inline void set_corner_scale(const Rect2& style_rect, const Rect2& inner_rect,
	const real_t corner_radius[4], Point2* inner_scale)
{
	real_t border_left = inner_rect.position.x - style_rect.position.x;
	real_t border_top = inner_rect.position.y - style_rect.position.y;
	real_t border_right = style_rect.size.width - inner_rect.size.width - border_left;
	real_t border_bottom = style_rect.size.height - inner_rect.size.height - border_top;

	// Amount of overflow along an edge.
	// Ex. SIDE_LEFT edge is the overflow between top_left and bottom_left corners.
	// MIN(0,) is to ignore underflow, and negating is to make values positive.
	real_t edge_overflow[4] = {-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_LEFT] -
										   corner_radius[CORNER_BOTTOM_LEFT]),
		-MIN(0,
			inner_rect.size.x - corner_radius[CORNER_TOP_LEFT] - corner_radius[CORNER_TOP_RIGHT]),
		-MIN(0, inner_rect.size.y - corner_radius[CORNER_TOP_RIGHT] -
					corner_radius[CORNER_BOTTOM_RIGHT]),
		-MIN(0, inner_rect.size.x - corner_radius[CORNER_BOTTOM_LEFT] -
					corner_radius[CORNER_BOTTOM_RIGHT])};

	// Sums of borders.
	real_t hb_sum = border_left + border_right;
	real_t vb_sum = border_top + border_bottom;

	// Ratio of each side to the sum of itself and opposite side.
	// Since overflow only happens with opposite borders, you only need to get the ratio of each
	// border relative to the sum of involved borders.
	real_t ratios[4] = {// Prevent divide by 0 errors.
		hb_sum > 0 ? (border_left / hb_sum) : 0, vb_sum > 0 ? (border_top / vb_sum) : 0,
		hb_sum > 0 ? (border_right / hb_sum) : 0, vb_sum > 0 ? (border_bottom / vb_sum) : 0};

	// Raw amount each corner should shrink.
	Point2 corner_reduction[4] = {
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_TOP] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_TOP]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_RIGHT],
			edge_overflow[SIDE_RIGHT] * ratios[SIDE_BOTTOM]),
		Point2(edge_overflow[SIDE_BOTTOM] * ratios[SIDE_LEFT],
			edge_overflow[SIDE_LEFT] * ratios[SIDE_BOTTOM]),
	};

	// Corner Radii as Point2s.
	Point2 pcr[4] = {
		Point2(corner_radius[0], corner_radius[0]),
		Point2(corner_radius[1], corner_radius[1]),
		Point2(corner_radius[2], corner_radius[2]),
		Point2(corner_radius[3], corner_radius[3]),
	};

	// If corner radii are too small, they won't shrink the full amount.
	// Adjacent corners will have to shrink the leftovers if they can.
	// Minf(0) is to ignore non-leftovers, and negating is to make values positive.
	Point2 leftovers[4] = {
		-((pcr[0] - corner_reduction[0]).minf(0)),
		-((pcr[1] - corner_reduction[1]).minf(0)),
		-((pcr[2] - corner_reduction[2]).minf(0)),
		-((pcr[3] - corner_reduction[3]).minf(0)),
	};

	// New shrunken radii after distributing the leftovers.
	Point2 distributed[4] = {
		((pcr[0] - corner_reduction[0] - leftovers[3] - leftovers[1]).maxf(0)),
		((pcr[1] - corner_reduction[1] - leftovers[0] - leftovers[2]).maxf(0)),
		((pcr[2] - corner_reduction[2] - leftovers[1] - leftovers[3]).maxf(0)),
		((pcr[3] - corner_reduction[3] - leftovers[2] - leftovers[0]).maxf(0)),
	};

	// How much the curve should scale to achieve the shrunken radii.
	for (int i = 0; i < 4; i++) {
		// Unshrinkable is how much is still left over, even after distributing leftovers.
		// Exclude it from the final scale.
		Point2 unshrinkable =
			(leftovers[(i + 1) % 4] + leftovers[(i + 4 - 1) % 4] - distributed[i]).maxf(0);
		inner_scale[i] = distributed[i] / (pcr[i] - unshrinkable).maxf(FLT_EPSILON);
	}
}

static inline void draw_rounded_rectangle(Vector<Vector2>& verts, Vector<int>& indices,
	Vector<Color>& colors, const Rect2& style_rect, const real_t corner_radius[4],
	const Rect2& ring_rect, const Rect2& inner_rect, const Color& inner_color,
	const Color& outer_color, const int corner_detail, const Vector2& skew, bool is_filled = false)
{
	int vert_offset = verts.size();
	int adapted_corner_detail = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
										(corner_radius[2] > 0) || (corner_radius[3] > 0)
									? corner_detail
									: 1;

	bool draw_border = !is_filled;

	real_t ring_corner_radius[4];
	set_inner_corner_radius(style_rect, ring_rect, corner_radius, ring_corner_radius);

	Point2 ring_scale[4];
	set_corner_scale(style_rect, ring_rect, ring_corner_radius, ring_scale);

	// Corner radius center points.
	Vector<Point2> outer_points = {
		ring_rect.position +
			Vector2(ring_corner_radius[0], ring_corner_radius[0]) * ring_scale[0], // tl
		Point2(ring_rect.position.x + ring_rect.size.x - ring_corner_radius[1] * ring_scale[1].x,
			ring_rect.position.y + ring_corner_radius[1] * ring_scale[1].y), // tr
		ring_rect.position + ring_rect.size -
			Vector2(ring_corner_radius[2], ring_corner_radius[2]) * ring_scale[2], // br
		Point2(ring_rect.position.x + ring_corner_radius[3] * ring_scale[3].x,
			ring_rect.position.y + ring_rect.size.y - ring_corner_radius[3] * ring_scale[3].y) // bl
	};

	real_t inner_corner_radius[4];
	set_inner_corner_radius(style_rect, inner_rect, corner_radius, inner_corner_radius);

	Point2 inner_scale[4];
	set_corner_scale(style_rect, inner_rect, inner_corner_radius, inner_scale);

	Vector<Point2> inner_points = {
		inner_rect.position +
			Vector2(inner_corner_radius[0], inner_corner_radius[0]) * inner_scale[0], // tl
		Point2(
			inner_rect.position.x + inner_rect.size.x - inner_corner_radius[1] * inner_scale[1].x,
			inner_rect.position.y + inner_corner_radius[1] * inner_scale[1].y), // tr
		inner_rect.position + inner_rect.size -
			Vector2(inner_corner_radius[2], inner_corner_radius[2]) * inner_scale[2], // br
		Point2(inner_rect.position.x + inner_corner_radius[3] * inner_scale[3].x,
			inner_rect.position.y + inner_rect.size.y -
				inner_corner_radius[3] * inner_scale[3].y) // bl
	};

	// Calculate the vertices.

	// If the center is filled, we do not draw the border and directly use the inner ring as
	// reference. Because all calls to this method either draw a ring or a filled rounded rectangle,
	// but not both.
	const real_t quarter_arc_rad = Math::PI / 2.0;
	const Point2 style_rect_center = style_rect.get_center();

	const int colors_size = colors.size();
	const int verts_size = verts.size();
	const int new_verts_amount = (adapted_corner_detail + 1) * (draw_border ? 8 : 4);

	colors.resize(colors_size + new_verts_amount);
	verts.resize(verts_size + new_verts_amount);
	Color* colors_ptr = colors.ptrw();
	Vector2* verts_ptr = verts.ptrw();

	for (int corner_idx = 0; corner_idx < 4; corner_idx++) {
		for (int detail = 0; detail <= adapted_corner_detail; detail++) {
			int idx_ofs = (adapted_corner_detail + 1) * corner_idx + detail;
			if (draw_border) {
				idx_ofs *= 2;
			}

			const real_t pt_angle =
				(corner_idx + detail / (double)adapted_corner_detail) * quarter_arc_rad + Math::PI;
			const real_t angle_cosine = Math::cos(pt_angle);
			const real_t angle_sine = Math::sin(pt_angle);

			{
				const real_t x =
					inner_corner_radius[corner_idx] * angle_cosine * inner_scale[corner_idx].x +
					inner_points[corner_idx].x;
				const real_t y =
					inner_corner_radius[corner_idx] * angle_sine * inner_scale[corner_idx].y +
					inner_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs] = inner_color;
			}

			if (draw_border) {
				const real_t x =
					ring_corner_radius[corner_idx] * angle_cosine * ring_scale[corner_idx].x +
					outer_points[corner_idx].x;
				const real_t y =
					ring_corner_radius[corner_idx] * angle_sine * ring_scale[corner_idx].y +
					outer_points[corner_idx].y;
				const float x_skew = -skew.x * (y - style_rect_center.y);
				const float y_skew = -skew.y * (x - style_rect_center.x);
				verts_ptr[verts_size + idx_ofs + 1] = Vector2(x + x_skew, y + y_skew);
				colors_ptr[colors_size + idx_ofs + 1] = outer_color;
			}
		}
	}

	int ring_vert_count = verts.size() - vert_offset;

	// Fill the indices and the colors for the border.

	if (draw_border) {
		int indices_size = indices.size();
		indices.resize(indices_size + ring_vert_count * 3);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < ring_vert_count; i++) {
			int idx_ofs = indices_size + i * 3;
			indices_ptr[idx_ofs] = vert_offset + i % ring_vert_count;
			indices_ptr[idx_ofs + 1] = vert_offset + (i + 2) % ring_vert_count;
			indices_ptr[idx_ofs + 2] = vert_offset + (i + 1) % ring_vert_count;
		}
	}

	if (is_filled) {
		// Compute the triangles pattern to draw the rounded rectangle.
		// Consists of vertical stripes of two triangles each.

		int stripes_count = ring_vert_count / 2 - 1;
		int last_vert_id = ring_vert_count - 1;

		int indices_size = indices.size();
		indices.resize(indices_size + stripes_count * 6);
		int* indices_ptr = indices.ptrw();

		for (int i = 0; i < stripes_count; i++) {
			int idx_ofs = indices_size + i * 6;
			// Polygon 1.
			indices_ptr[idx_ofs] = vert_offset + i;
			indices_ptr[idx_ofs + 1] = vert_offset + last_vert_id - i - 1;
			indices_ptr[idx_ofs + 2] = vert_offset + i + 1;
			// Polygon 2.
			indices_ptr[idx_ofs + 3] = vert_offset + i;
			indices_ptr[idx_ofs + 4] = vert_offset + last_vert_id - i;
			indices_ptr[idx_ofs + 5] = vert_offset + last_vert_id - i - 1;
		}
	}
}

static inline void adapt_values(int p_index_a, int p_index_b, real_t* adapted_values,
	const real_t* p_values, const real_t p_width, const real_t p_max_a, const real_t p_max_b)
{
	real_t value_a = p_values[p_index_a];
	real_t value_b = p_values[p_index_b];
	real_t factor = MIN(1.0, p_width / (value_a + value_b));
	adapted_values[p_index_a] = MIN(MIN(value_a * factor, p_max_a), adapted_values[p_index_a]);
	adapted_values[p_index_b] = MIN(MIN(value_b * factor, p_max_b), adapted_values[p_index_b]);
}

void StyleBox::_draw_flat(RID p_canvas_item, const Rect2& p_rect) const
{
	bool draw_border = (border_width[0] > 0) || (border_width[1] > 0) || (border_width[2] > 0) ||
					   (border_width[3] > 0);
	bool draw_shadow = (shadow_size > 0);
	if (!draw_border && !draw_center && !draw_shadow) {
		return;
	}

	Rect2 style_rect = p_rect.grow_individual(expand_margin[SIDE_LEFT], expand_margin[SIDE_TOP],
		expand_margin[SIDE_RIGHT], expand_margin[SIDE_BOTTOM]);
	if (Math::is_zero_approx(style_rect.size.width) ||
		Math::is_zero_approx(style_rect.size.height)) {
		return;
	}

	const bool rounded_corners = (corner_radius[0] > 0) || (corner_radius[1] > 0) ||
								 (corner_radius[2] > 0) || (corner_radius[3] > 0);
	// Only enable antialiasing if it is actually needed. This improves performance
	// and maximizes sharpness for non-skewed StyleBoxes with sharp corners.
	const bool aa_on = (rounded_corners || !skew.is_zero_approx()) && anti_aliased;

	const bool blend_on = blend_border && draw_border;

	Color border_color_alpha = Color(border_color.r, border_color.g, border_color.b, 0);
	Color border_color_blend = (draw_center ? bg_color : border_color_alpha);
	Color border_color_inner = blend_on ? border_color_blend : border_color;

	// Adapt borders (prevent weird overlapping/glitchy drawings).
	real_t width = MAX(style_rect.size.width, 0);
	real_t height = MAX(style_rect.size.height, 0);
	real_t adapted_border[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(SIDE_TOP, SIDE_BOTTOM, adapted_border, border_width, height, height, height);
	adapt_values(SIDE_LEFT, SIDE_RIGHT, adapted_border, border_width, width, width, width);

	// Adapt corners (prevent weird overlapping/glitchy drawings).
	real_t adapted_corner[4] = {1000000.0, 1000000.0, 1000000.0, 1000000.0};
	adapt_values(CORNER_TOP_RIGHT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_BOTTOM_LEFT, adapted_corner, corner_radius, height,
		height - adapted_border[SIDE_BOTTOM], height - adapted_border[SIDE_TOP]);
	adapt_values(CORNER_TOP_LEFT, CORNER_TOP_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);
	adapt_values(CORNER_BOTTOM_LEFT, CORNER_BOTTOM_RIGHT, adapted_corner, corner_radius, width,
		width - adapted_border[SIDE_RIGHT], width - adapted_border[SIDE_LEFT]);

	Rect2 infill_rect = style_rect.grow_individual(-adapted_border[SIDE_LEFT],
		-adapted_border[SIDE_TOP], -adapted_border[SIDE_RIGHT], -adapted_border[SIDE_BOTTOM]);

	Rect2 border_style_rect = style_rect;

	real_t aa_size_scaled = 1.0f;
	if (aa_on) {
		real_t scale_factor = TextServer::get_current_drawn_item_oversampling();
		if (scale_factor == 0.0) {
			scale_factor = 1.0;
		}

		// Adjust AA feather size to account for the 2D scale factor, so that
		// antialiasing doesn't become blurry at viewport resolutions higher
		// than the default when using the `canvas_items` stretch mode
		// (or when using `oversampling` values different than `1.0`).
		aa_size_scaled = aa_size / scale_factor;
	}

	if (aa_on) {
		for (int i = 0; i < 4; i++) {
			if (border_width[i] > 0) {
				border_style_rect = border_style_rect.grow_side((Side)i, -aa_size_scaled);
			}
		}
	}

	Vector<Point2> verts;
	Vector<int> indices;
	Vector<Color> colors;
	Vector<Point2> uvs;

	// Create shadow.
	if (draw_shadow) {
		Rect2 shadow_inner_rect = style_rect;
		shadow_inner_rect.position += shadow_offset;

		Rect2 shadow_rect = style_rect.grow(shadow_size);
		shadow_rect.position += shadow_offset;

		Color shadow_color_transparent = Color(shadow_color.r, shadow_color.g, shadow_color.b, 0);

		draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
			shadow_rect, shadow_inner_rect, shadow_color, shadow_color_transparent, corner_detail,
			skew);

		if (draw_center) {
			draw_rounded_rectangle(verts, indices, colors, shadow_inner_rect, adapted_corner,
				shadow_inner_rect, shadow_inner_rect, shadow_color, shadow_color, corner_detail,
				skew, true);
		}
	}

	// Create border (no AA).
	if (draw_border && !aa_on) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			border_style_rect, infill_rect, border_color_inner, border_color, corner_detail, skew);
	}

	// Create infill (no AA).
	if (draw_center && (!aa_on || blend_on)) {
		draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
			infill_rect, infill_rect, bg_color, bg_color, corner_detail, skew, true);
	}

	if (aa_on) {
		real_t aa_border_width[4];
		real_t aa_border_width_half[4];
		real_t aa_fill_width[4];
		real_t aa_fill_width_half[4];

		if (draw_border) {
			for (int i = 0; i < 4; i++) {
				if (border_width[i] > 0) {
					aa_border_width[i] = aa_size_scaled;
					aa_border_width_half[i] = aa_size_scaled * 0.5;
					aa_fill_width[i] = 0;
					aa_fill_width_half[i] = 0;
				}
				else {
					aa_border_width[i] = 0;
					aa_border_width_half[i] = 0;
					aa_fill_width[i] = aa_size_scaled;
					aa_fill_width_half[i] = aa_size_scaled * 0.5;
				}
			}
		}
		else {
			for (int i = 0; i < 4; i++) {
				aa_border_width[i] = 0;
				aa_border_width_half[i] = 0;
				aa_fill_width[i] = aa_size_scaled;
				aa_fill_width_half[i] = aa_size_scaled * 0.5;
			}
		}

		if (draw_center) {
			// Infill rect, transparent side of antialiasing gradient (base infill rect enlarged by
			// AA size)
			Rect2 infill_rect_aa_transparent = infill_rect.grow_individual(
				aa_fill_width_half[SIDE_LEFT], aa_fill_width_half[SIDE_TOP],
				aa_fill_width_half[SIDE_RIGHT], aa_fill_width_half[SIDE_BOTTOM]);
			// Infill rect, colored side of antialiasing gradient (base infill rect shrunk by AA
			// size)
			Rect2 infill_rect_aa_colored = infill_rect_aa_transparent.grow_individual(
				-aa_fill_width[SIDE_LEFT], -aa_fill_width[SIDE_TOP], -aa_fill_width[SIDE_RIGHT],
				-aa_fill_width[SIDE_BOTTOM]);
			if (!blend_on) {
				// Create center fill, not antialiased yet
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_colored, infill_rect_aa_colored, bg_color, bg_color,
					corner_detail, skew, true);
			}
			if (!blend_on || !draw_border) {
				Color alpha_bg = Color(bg_color.r, bg_color.g, bg_color.b, 0);
				// Add antialiasing on the center fill
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					infill_rect_aa_transparent, infill_rect_aa_colored, bg_color, alpha_bg,
					corner_detail, skew);
			}
		}

		if (draw_border) {
			// Inner border rect, fully colored side of antialiasing gradient (base inner rect
			// enlarged by AA size)
			Rect2 inner_rect_aa_colored = infill_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Inner border rect, transparent side of antialiasing gradient (base inner rect shrunk
			// by AA size)
			Rect2 inner_rect_aa_transparent = inner_rect_aa_colored.grow_individual(
				-aa_border_width[SIDE_LEFT], -aa_border_width[SIDE_TOP],
				-aa_border_width[SIDE_RIGHT], -aa_border_width[SIDE_BOTTOM]);
			// Outer border rect, transparent side of antialiasing gradient (base outer rect
			// enlarged by AA size)
			Rect2 outer_rect_aa_transparent = style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);
			// Outer border rect, colored side of antialiasing gradient (base outer rect shrunk by
			// AA size)
			Rect2 outer_rect_aa_colored = border_style_rect.grow_individual(
				aa_border_width_half[SIDE_LEFT], aa_border_width_half[SIDE_TOP],
				aa_border_width_half[SIDE_RIGHT], aa_border_width_half[SIDE_BOTTOM]);

			// Create border ring, not antialiased yet
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_colored, ((blend_on) ? infill_rect : inner_rect_aa_colored),
				border_color_inner, border_color, corner_detail, skew);
			if (!blend_on) {
				// Add antialiasing on the ring inner border
				draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
					inner_rect_aa_colored, inner_rect_aa_transparent, border_color_blend,
					border_color, corner_detail, skew);
			}
			// Add antialiasing on the ring outer border
			draw_rounded_rectangle(verts, indices, colors, border_style_rect, adapted_corner,
				outer_rect_aa_transparent, outer_rect_aa_colored, border_color, border_color_alpha,
				corner_detail, skew);
		}
	}

	// Compute UV coordinates.
	Rect2 uv_rect = style_rect.grow(aa_on ? aa_size_scaled : 0);
	uvs.resize(verts.size());
	Point2* uvs_ptr = uvs.ptrw();
	for (int i = 0; i < verts.size(); i++) {
		uvs_ptr[i].x = (verts[i].x - uv_rect.position.x) / uv_rect.size.width;
		uvs_ptr[i].y = (verts[i].y - uv_rect.position.y) / uv_rect.size.height;
	}

	// Draw stylebox.
	Renderer::canvas_item_add_triangle_array(p_canvas_item, indices, verts, colors, uvs);
}

// STYLE_TEXTURE properties & drawing.

void StyleBox::set_texture(Ref<Texture2D> p_texture)
{
	if (texture == p_texture) {
		return;
	}
	texture = p_texture;
	emit_changed();
}

Ref<Texture2D> StyleBox::get_texture() const { return texture; }

void StyleBox::set_texture_margin(Side p_side, float p_size)
{
	ERR_FAIL_INDEX((int)p_side, 4);

	texture_margin[p_side] = p_size;
	emit_changed();
}

void StyleBox::set_texture_margin_all(float p_size)
{
	for (int i = 0; i < 4; i++) {
		texture_margin[i] = p_size;
	}
	emit_changed();
}

void StyleBox::set_texture_margin_individual(
	float p_left, float p_top, float p_right, float p_bottom)
{
	texture_margin[SIDE_LEFT] = p_left;
	texture_margin[SIDE_TOP] = p_top;
	texture_margin[SIDE_RIGHT] = p_right;
	texture_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_texture_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return texture_margin[p_side];
}

void StyleBox::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;
	emit_changed();
}

Rect2 StyleBox::get_region_rect() const { return region_rect; }

void StyleBox::set_h_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_h = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_h_axis_stretch_mode() const { return axis_h; }

void StyleBox::set_v_axis_stretch_mode(AxisStretchMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 3);
	axis_v = p_mode;
	emit_changed();
}

StyleBox::AxisStretchMode StyleBox::get_v_axis_stretch_mode() const { return axis_v; }

void StyleBox::set_modulate(const Color& p_modulate)
{
	if (modulate == p_modulate) {
		return;
	}
	modulate = p_modulate;
	emit_changed();
}

Color StyleBox::get_modulate() const { return modulate; }

void StyleBox::_draw_texture(RID p_canvas_item, const Rect2& p_rect) const
{
	if (texture.is_null()) {
		return;
	}

	Rect2 rect = p_rect;
	Rect2 src_rect = region_rect;

	texture->get_rect_region(rect, src_rect, rect, src_rect);

	rect.position.x -= expand_margin[SIDE_LEFT];
	rect.position.y -= expand_margin[SIDE_TOP];
	rect.size.x += expand_margin[SIDE_LEFT] + expand_margin[SIDE_RIGHT];
	rect.size.y += expand_margin[SIDE_TOP] + expand_margin[SIDE_BOTTOM];

	Vector2 start_offset = Vector2(texture_margin[SIDE_LEFT], texture_margin[SIDE_TOP]);
	Vector2 end_offset = Vector2(texture_margin[SIDE_RIGHT], texture_margin[SIDE_BOTTOM]);

	Renderer::canvas_item_add_nine_patch(p_canvas_item, rect, src_rect,
		texture->get_scaled_rid(), start_offset, end_offset, RSE::NinePatchAxisMode(axis_h),
		RSE::NinePatchAxisMode(axis_v), draw_center, modulate);
}

// STYLE_LINE properties & drawing.

void StyleBox::set_color(const Color& p_color)
{
	color = p_color;
	emit_changed();
}

Color StyleBox::get_color() const { return color; }

void StyleBox::set_thickness(int p_thickness)
{
	thickness = p_thickness;
	emit_changed();
}

int StyleBox::get_thickness() const { return thickness; }

void StyleBox::set_vertical(bool p_vertical)
{
	vertical = p_vertical;
	emit_changed();
}

bool StyleBox::is_vertical() const { return vertical; }

void StyleBox::set_grow_end(float p_grow_end)
{
	grow_end = p_grow_end;
	emit_changed();
}

float StyleBox::get_grow_end() const { return grow_end; }

void StyleBox::set_grow_begin(float p_grow_begin)
{
	grow_begin = p_grow_begin;
	emit_changed();
}

float StyleBox::get_grow_begin() const { return grow_begin; }

void StyleBox::_draw_line(RID p_canvas_item, const Rect2& p_rect) const
{
	Rect2i r = p_rect;

	if (vertical) {
		r.position.y -= grow_begin;
		r.size.y += (grow_begin + grow_end);
		r.size.x = thickness;
	}
	else {
		r.position.x -= grow_begin;
		r.size.x += (grow_begin + grow_end);
		r.size.y = thickness;
	}

	Renderer::canvas_item_add_rect(p_canvas_item, r, color);
}
