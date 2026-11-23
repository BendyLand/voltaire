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

#include "scene/resources/image_texture.h"

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

#include "core/io/resource_loader.h"
#include "modules/modules_enabled.gen.h" // For svg.
#include "scene/resources/image_texture.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#ifdef MODULE_SVG_ENABLED
#include "core/io/resource_loader.h"
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

#include "servers/camera/camera_server.h"
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
#include "scene/resources/image_texture.h"
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

#include "scene/resources/gradient.h"

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

#include "scene/resources/image_texture.h"

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

#include "scene/main/node.h"
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

#include "scene/resources/image_texture.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "scene/resources/image_texture.h"
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

#include "scene/resources/image_texture.h"


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
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/types.h"
#include "font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/theme.h"
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

#include "scene/resources/font.h"

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

#include "scene/resources/font.h"

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

#include "scene/gui/dialogs.h"

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

#include "scene/gui/dialogs.h"

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

#include "scene/resources/theme.h"
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

#include "scene/resources/theme.h"

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


// --- Merged from scene/resources/material.cpp ---
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/version.h"
#include "scene/resources/material.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h"

void Material::set_next_pass(const Ref<Material>& p_pass)
{
	for (Ref<Material> pass_child = p_pass; pass_child.is_valid();
		 pass_child = pass_child->get_next_pass()) {
		ERR_FAIL_COND_MSG(pass_child == this,
			"Can't set as next_pass one of its parents to prevent crashes due to recursive loop.");
	}

	if (next_pass == p_pass) {
		return;
	}

	next_pass = p_pass;

	if (material.is_valid()) {
		RID next_pass_rid;
		if (next_pass.is_valid()) {
			next_pass_rid = next_pass->get_rid();
		}

		Renderer::material_set_next_pass(material, next_pass_rid);
	}
}

Ref<Material> Material::get_next_pass() const { return next_pass; }

void Material::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(p_priority > RENDER_PRIORITY_MAX);

	render_priority = p_priority;

	if (material.is_valid()) {
		Renderer::material_set_render_priority(material, p_priority);
	}
}

int Material::get_render_priority() const { return render_priority; }

RID Material::get_rid() const { return material; }

void Material::_mark_ready() { init_state = INIT_STATE_READY; }

Shader::Mode Material::get_shader_mode() const
{
	if (is_placeholder) {
		return Shader::MODE_CANVAS_ITEM;
	}
	return Shader::MODE_MAX;
}

Ref<Resource> Material::create_placeholder() const
{
	Ref<Material> placeholder;
	placeholder.instantiate();
	placeholder->is_placeholder = true;
	return placeholder;
}

Material::Material() { render_priority = 0; }

Material::~Material()
{
	if (material.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(material);
	}
}

bool ShaderMaterial::_property_can_revert(const String& p_name) const
{
	if (shader.is_valid()) {
		if (remap_cache.has(p_name)) {
			return true;
		}
		const String sname = p_name;
		return sname == "render_priority" || sname == "next_pass";
	}
	return false;
}

Ref<Shader> ShaderMaterial::get_shader() const { return shader; }

bool ShaderMaterial::_can_do_next_pass() const
{
	return shader.is_valid() && shader->get_mode() == Shader::MODE_SPATIAL;
}

bool ShaderMaterial::_can_use_render_priority() const
{
	return shader.is_valid() && shader->get_mode() == Shader::MODE_SPATIAL;
}

Shader::Mode ShaderMaterial::get_shader_mode() const
{
	if (shader.is_valid()) {
		return shader->get_mode();
	}
	else {
		return Shader::MODE_SPATIAL;
	}
}

RID ShaderMaterial::get_rid() const
{
	_check_material_rid();
	return Material::get_rid();
}

RID ShaderMaterial::get_shader_rid() const
{
	if (shader.is_valid()) {
		return shader->get_rid();
	}
	else {
		return RID();
	}
}

ShaderMaterial::ShaderMaterial()
{
	// Material RID will be empty until it is required.
}

ShaderMaterial::~ShaderMaterial() {}

HashMap<StandardMaterial3D::MaterialKey, StandardMaterial3D::ShaderData, StandardMaterial3D::MaterialKey>
	StandardMaterial3D::shader_map;
Mutex StandardMaterial3D::shader_map_mutex;
StandardMaterial3D::ShaderNames* StandardMaterial3D::shader_names = nullptr;
Mutex StandardMaterial3D::material_mutex;
SelfList<StandardMaterial3D>::List StandardMaterial3D::dirty_materials;

void StandardMaterial3D::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->albedo = "albedo";
	shader_names->specular = "specular";
	shader_names->roughness = "roughness";
	shader_names->metallic = "metallic";
	shader_names->emission = "emission";
	shader_names->emission_energy = "emission_energy";
	shader_names->normal_scale = "normal_scale";
	shader_names->rim = "rim";
	shader_names->rim_tint = "rim_tint";
	shader_names->clearcoat = "clearcoat";
	shader_names->clearcoat_roughness = "clearcoat_roughness";
	shader_names->anisotropy = "anisotropy_ratio";
	shader_names->heightmap_scale = "heightmap_scale";
	shader_names->subsurface_scattering_strength = "subsurface_scattering_strength";
	shader_names->backlight = "backlight";
	shader_names->refraction = "refraction";
	shader_names->point_size = "point_size";
	shader_names->uv1_scale = "uv1_scale";
	shader_names->uv1_offset = "uv1_offset";
	shader_names->uv2_scale = "uv2_scale";
	shader_names->uv2_offset = "uv2_offset";
	shader_names->uv1_blend_sharpness = "uv1_blend_sharpness";
	shader_names->uv2_blend_sharpness = "uv2_blend_sharpness";

	shader_names->particles_anim_h_frames = "particles_anim_h_frames";
	shader_names->particles_anim_v_frames = "particles_anim_v_frames";
	shader_names->particles_anim_loop = "particles_anim_loop";
	shader_names->heightmap_min_layers = "heightmap_min_layers";
	shader_names->heightmap_max_layers = "heightmap_max_layers";
	shader_names->heightmap_flip = "heightmap_flip";

	shader_names->grow = "grow";

	shader_names->ao_light_affect = "ao_light_affect";

	shader_names->proximity_fade_distance = "proximity_fade_distance";
	shader_names->distance_fade_min = "distance_fade_min";
	shader_names->distance_fade_max = "distance_fade_max";

	shader_names->msdf_pixel_range = "msdf_pixel_range";
	shader_names->msdf_outline_size = "msdf_outline_size";

	shader_names->metallic_texture_channel = "metallic_texture_channel";
	shader_names->ao_texture_channel = "ao_texture_channel";
	shader_names->clearcoat_texture_channel = "clearcoat_texture_channel";
	shader_names->rim_texture_channel = "rim_texture_channel";
	shader_names->heightmap_texture_channel = "heightmap_texture_channel";
	shader_names->refraction_texture_channel = "refraction_texture_channel";

	shader_names->transmittance_color = "transmittance_color";
	shader_names->transmittance_depth = "transmittance_depth";
	shader_names->transmittance_boost = "transmittance_boost";

	shader_names->texture_names[TEXTURE_ALBEDO] = "texture_albedo";
	shader_names->texture_names[TEXTURE_METALLIC] = "texture_metallic";
	shader_names->texture_names[TEXTURE_ROUGHNESS] = "texture_roughness";
	shader_names->texture_names[TEXTURE_EMISSION] = "texture_emission";
	shader_names->texture_names[TEXTURE_NORMAL] = "texture_normal";
	shader_names->texture_names[TEXTURE_BENT_NORMAL] = "texture_bent_normal";
	shader_names->texture_names[TEXTURE_RIM] = "texture_rim";
	shader_names->texture_names[TEXTURE_CLEARCOAT] = "texture_clearcoat";
	shader_names->texture_names[TEXTURE_FLOWMAP] = "texture_flowmap";
	shader_names->texture_names[TEXTURE_AMBIENT_OCCLUSION] = "texture_ambient_occlusion";
	shader_names->texture_names[TEXTURE_HEIGHTMAP] = "texture_heightmap";
	shader_names->texture_names[TEXTURE_SUBSURFACE_SCATTERING] = "texture_subsurface_scattering";
	shader_names->texture_names[TEXTURE_SUBSURFACE_TRANSMITTANCE] =
		"texture_subsurface_transmittance";
	shader_names->texture_names[TEXTURE_BACKLIGHT] = "texture_backlight";
	shader_names->texture_names[TEXTURE_REFRACTION] = "texture_refraction";
	shader_names->texture_names[TEXTURE_DETAIL_MASK] = "texture_detail_mask";
	shader_names->texture_names[TEXTURE_DETAIL_ALBEDO] = "texture_detail_albedo";
	shader_names->texture_names[TEXTURE_DETAIL_NORMAL] = "texture_detail_normal";
	shader_names->texture_names[TEXTURE_ORM] = "texture_orm";

	shader_names->alpha_scissor_threshold = "alpha_scissor_threshold";
	shader_names->alpha_hash_scale = "alpha_hash_scale";

	shader_names->alpha_antialiasing_edge = "alpha_antialiasing_edge";
	shader_names->albedo_texture_size = "albedo_texture_size";
	shader_names->z_clip_scale = "z_clip_scale";
	shader_names->fov_override = "fov_override";
}

HashMap<uint64_t, Ref<StandardMaterial3D>> StandardMaterial3D::materials_for_2d;

void StandardMaterial3D::finish_shaders()
{
	materials_for_2d.clear();

	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void StandardMaterial3D::_update_shader()
{
	if (!_is_initialized()) {
		_mark_ready();
	}

	MaterialKey mk = _compute_key();
	if (mk == current_key) {
		return; // no update required in the end
	}

	{
		MutexLock lock(shader_map_mutex);
		ShaderData* v = shader_map.getptr(current_key);
		if (v) {
			v->users--;
			if (v->users == 0) {
				// Deallocate shader which is no longer in use.
				shader_rid = RID();
				Renderer::free_rid(v->shader);
				shader_map.erase(current_key);
			}
		}

		current_key = mk;

		v = shader_map.getptr(mk);
		if (v) {
			shader_rid = v->shader;
			v->users++;

			if (_get_material().is_valid()) {
				Renderer::material_set_shader(_get_material(), shader_rid);
			}

			return;
		}
	}

	// From this point, it is possible that multiple threads requesting the same key will
	// race to create the shader. The winner, which is the one found in shader_map, will be
	// used. The losers will free their shader.

	String texfilter_str;
	// Force linear filtering for the heightmap texture, as the heightmap effect
	// looks broken with nearest-neighbor filtering (with and without Deep Parallax).
	String texfilter_height_str;
	switch (texture_filter) {
	case TEXTURE_FILTER_NEAREST:
		texfilter_str = "filter_nearest";
		texfilter_height_str = "filter_linear";
		break;
	case TEXTURE_FILTER_LINEAR:
		texfilter_str = "filter_linear";
		texfilter_height_str = "filter_linear";
		break;
	case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
		texfilter_str = "filter_nearest_mipmap";
		texfilter_height_str = "filter_linear_mipmap";
		break;
	case TEXTURE_FILTER_LINEAR_WITH_MIPMAPS:
		texfilter_str = "filter_linear_mipmap";
		texfilter_height_str = "filter_linear_mipmap";
		break;
	case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC:
		texfilter_str = "filter_nearest_mipmap_anisotropic";
		texfilter_height_str = "filter_linear_mipmap_anisotropic";
		break;
	case TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC:
		texfilter_str = "filter_linear_mipmap_anisotropic";
		texfilter_height_str = "filter_linear_mipmap_anisotropic";
		break;
	case TEXTURE_FILTER_MAX:
		break; // Internal value, skip.
	}

	if (flags[FLAG_USE_TEXTURE_REPEAT]) {
		texfilter_str += ", repeat_enable";
		texfilter_height_str += ", repeat_enable";
	}
	else {
		texfilter_str += ", repeat_disable";
		texfilter_height_str += ", repeat_disable";
	}

	// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
	String code = vformat("// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
						  " " VLTR_VERSION_FULL_CONFIG "'s %s.\n\n",
		orm ? "StandardMaterial3D (ORM)" : "StandardMaterial3D");

	// Define shader type and render mode based on property values.
	code += "shader_type spatial;\nrender_mode ";
	switch (blend_mode) {
	case BLEND_MODE_MIX:
		code += "blend_mix";
		break;
	case BLEND_MODE_ADD:
		code += "blend_add";
		break;
	case BLEND_MODE_SUB:
		code += "blend_sub";
		break;
	case BLEND_MODE_MUL:
		code += "blend_mul";
		break;
	case BLEND_MODE_PREMULT_ALPHA:
		code += "blend_premul_alpha";
		break;
	case BLEND_MODE_MAX:
		break; // Internal value, skip.
	}

	DepthDrawMode ddm = depth_draw_mode;
	if (features[FEATURE_REFRACTION]) {
		ddm = DEPTH_DRAW_ALWAYS;
	}

	switch (ddm) {
	case DEPTH_DRAW_OPAQUE_ONLY:
		code += ", depth_draw_opaque";
		break;
	case DEPTH_DRAW_ALWAYS:
		code += ", depth_draw_always";
		break;
	case DEPTH_DRAW_DISABLED:
		code += ", depth_draw_never";
		break;
	case DEPTH_DRAW_MAX:
		break; // Internal value, skip.
	}

	switch (cull_mode) {
	case CULL_BACK:
		code += ", cull_back";
		break;
	case CULL_FRONT:
		code += ", cull_front";
		break;
	case CULL_DISABLED:
		code += ", cull_disabled";
		break;
	case CULL_MAX:
		break; // Internal value, skip.
	}
	switch (diffuse_mode) {
	case DIFFUSE_BURLEY:
		code += ", diffuse_burley";
		break;
	case DIFFUSE_LAMBERT:
		code += ", diffuse_lambert";
		break;
	case DIFFUSE_LAMBERT_WRAP:
		code += ", diffuse_lambert_wrap";
		break;
	case DIFFUSE_TOON:
		code += ", diffuse_toon";
		break;
	case DIFFUSE_MAX:
		break; // Internal value, skip.
	}
	switch (specular_mode) {
	case SPECULAR_SCHLICK_GGX:
		code += ", specular_schlick_ggx";
		break;
	case SPECULAR_TOON:
		code += ", specular_toon";
		break;
	case SPECULAR_DISABLED:
		code += ", specular_disabled";
		break;
	case SPECULAR_MAX:
		break; // Internal value, skip.
	}
	if (features[FEATURE_SUBSURFACE_SCATTERING] && flags[FLAG_SUBSURFACE_MODE_SKIN]) {
		code += ", sss_mode_skin";
	}

	if (shading_mode == SHADING_MODE_UNSHADED) {
		code += ", unshaded";
	}
	if (flags[FLAG_DISABLE_DEPTH_TEST]) {
		code += ", depth_test_disabled";
	}
	else {
		switch (depth_test) {
		case DEPTH_TEST_DEFAULT:
			// depth_test_default is the default behavior, no need to emit it here.
			break;
		case DEPTH_TEST_INVERTED:
			code += ", depth_test_inverted";
			break;
		case DEPTH_TEST_MAX:
			break; // Internal value, skip.
		}
	}
	if (flags[FLAG_PARTICLE_TRAILS_MODE]) {
		code += ", particle_trails";
	}
	if (shading_mode == SHADING_MODE_PER_VERTEX) {
		code += ", vertex_lighting";
	}
	if (flags[FLAG_DONT_RECEIVE_SHADOWS]) {
		code += ", shadows_disabled";
	}
	if (flags[FLAG_DISABLE_AMBIENT_LIGHT]) {
		code += ", ambient_light_disabled";
	}
	if (flags[FLAG_USE_SHADOW_TO_OPACITY]) {
		code += ", shadow_to_opacity";
	}
	if (flags[FLAG_DISABLE_FOG]) {
		code += ", fog_disabled";
	}
	if (flags[FLAG_DISABLE_SPECULAR_OCCLUSION]) {
		code += ", specular_occlusion_disabled";
	}
	if (transparency == TRANSPARENCY_ALPHA_DEPTH_PRE_PASS) {
		code += ", depth_prepass_alpha";
	}

	// Although it's technically possible to do alpha antialiasing without using alpha hash or alpha
	// scissor, it is restricted in the base material because it has no use, and abusing it with
	// regular Alpha blending can saturate the MSAA mask.
	if (transparency == TRANSPARENCY_ALPHA_HASH || transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		// Alpha antialiasing is only useful with ALPHA_HASH or ALPHA_SCISSOR.
		if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE) {
			code += ", alpha_to_coverage";
		}
		else if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE) {
			code += ", alpha_to_coverage_and_one";
		}
	}

	code += ";\n";

	if (stencil_mode != STENCIL_MODE_DISABLED && stencil_flags != 0) {
		code += "stencil_mode ";

		if (stencil_flags & STENCIL_FLAG_READ) {
			code += "read";
		}

		if (stencil_flags & STENCIL_FLAG_WRITE) {
			if (stencil_flags & STENCIL_FLAG_READ) {
				code += ", ";
			}
			code += "write";
		}

		if (stencil_flags & STENCIL_FLAG_WRITE_DEPTH_FAIL) {
			if (stencil_flags & (STENCIL_FLAG_READ | STENCIL_FLAG_WRITE)) {
				code += ", ";
			}
			code += "write_depth_fail";
		}

		switch (stencil_compare) {
		case STENCIL_COMPARE_ALWAYS:
			code += ", compare_always";
			break;
		case STENCIL_COMPARE_LESS:
			code += ", compare_less";
			break;
		case STENCIL_COMPARE_EQUAL:
			code += ", compare_equal";
			break;
		case STENCIL_COMPARE_LESS_OR_EQUAL:
			code += ", compare_less_or_equal";
			break;
		case STENCIL_COMPARE_GREATER:
			code += ", compare_greater";
			break;
		case STENCIL_COMPARE_NOT_EQUAL:
			code += ", compare_not_equal";
			break;
		case STENCIL_COMPARE_GREATER_OR_EQUAL:
			code += ", compare_greater_or_equal";
			break;
		case STENCIL_COMPARE_MAX:
			break;
		}

		code += vformat(", %s;\n", stencil_reference);
	}

	// Generate list of uniforms.
	code += vformat(R"(
uniform vec4 albedo : source_color;
uniform sampler2D texture_albedo : source_color, %s;
)",
		texfilter_str);

	if (grow_enabled) {
		code += "uniform float grow : hint_range(-16.0, 16.0, 0.001);\n";
	}

	if (proximity_fade_enabled) {
		code += "uniform float proximity_fade_distance : hint_range(0.0, 4096.0, 0.01);\n";
	}
	if (distance_fade != DISTANCE_FADE_DISABLED) {
		code += R"(
uniform float distance_fade_min : hint_range(0.0, 4096.0, 0.01);
uniform float distance_fade_max : hint_range(0.0, 4096.0, 0.01);
)";
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && flags[FLAG_UV1_USE_TRIPLANAR]) {
		String msg = "MSDF is not supported on triplanar materials. Ignoring MSDF in favor of "
					 "triplanar mapping.";
		if (textures[TEXTURE_ALBEDO].is_valid()) {
			WARN_PRINT(vformat(
				"%s (albedo %s): " + msg, get_path(), textures[TEXTURE_ALBEDO]->get_path()));
		}
		else if (!get_path().is_empty()) {
			WARN_PRINT(vformat("%s: " + msg, get_path()));
		}
		else {
			WARN_PRINT(msg);
		}
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
uniform float msdf_pixel_range : hint_range(1.0, 100.0, 1.0);
uniform float msdf_outline_size : hint_range(0.0, 250.0, 1.0);
)";
	}

	// Alpha scissor is only valid if there is no antialiasing edge.
	// Alpha hash is valid whenever, but not with alpha scissor.
	if (transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		code += "uniform float alpha_scissor_threshold : hint_range(0.0, 1.0, 0.001);\n";
	}
	else if (transparency == TRANSPARENCY_ALPHA_HASH) {
		code += "uniform float alpha_hash_scale : hint_range(0.0, 2.0, 0.01);\n";
	}
	// If alpha antialiasing isn't off, add in the edge variable.
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF &&
		(transparency == TRANSPARENCY_ALPHA_SCISSOR || transparency == TRANSPARENCY_ALPHA_HASH)) {
		code += "uniform float alpha_antialiasing_edge : hint_range(0.0, 1.0, 0.01);\n";
	}

	code += "uniform ivec2 albedo_texture_size;\n";
	code += "uniform float point_size : hint_range(0.1, 128.0, 0.1);\n";

	if (!orm) {
		code += vformat(R"(
uniform float roughness : hint_range(0.0, 1.0);
uniform sampler2D texture_metallic : hint_default_white, %s;
uniform vec4 metallic_texture_channel;
)",
			texfilter_str);
		switch (roughness_texture_channel) {
		case TEXTURE_CHANNEL_RED: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_r, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_GREEN: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_g, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_BLUE: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_b, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_ALPHA: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_a, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_GRAYSCALE: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_gray, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_MAX:
			break; // Internal value, skip.
		}

		code += R"(
uniform float specular : hint_range(0.0, 1.0, 0.01);
uniform float metallic : hint_range(0.0, 1.0, 0.01);
)";
	}
	else {
		code += "uniform sampler2D texture_orm : hint_roughness_g, " + texfilter_str + ";\n";
	}

	if (billboard_mode == BILLBOARD_PARTICLES) {
		code += R"(
uniform int particles_anim_h_frames : hint_range(1, 128);
uniform int particles_anim_v_frames : hint_range(1, 128);
uniform bool particles_anim_loop;
)";
	}

	if (features[FEATURE_EMISSION]) {
		code += vformat(R"(
uniform sampler2D texture_emission : source_color, hint_default_black, %s;
uniform vec4 emission : source_color;
uniform float emission_energy : hint_range(0.0, 100.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_REFRACTION]) {
		code += vformat(R"(
uniform sampler2D texture_refraction : %s;
uniform float refraction : hint_range(-1.0, 1.0, 0.001);
uniform vec4 refraction_texture_channel;
)",
			texfilter_str);
	}

	if (features[FEATURE_REFRACTION]) {
		code += "uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, "
				"filter_linear_mipmap;\n";
	}

	if (features[FEATURE_REFRACTION] || proximity_fade_enabled) {
		code += "uniform sampler2D depth_texture : hint_depth_texture, repeat_disable, "
				"filter_nearest;\n";
	}

	if (features[FEATURE_NORMAL_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_normal : hint_roughness_normal, %s;
uniform float normal_scale : hint_range(-16.0, 16.0);
)",
			texfilter_str);
	}
	if (features[FEATURE_BENT_NORMAL_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_bent_normal : hint_roughness_normal, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_RIM]) {
		code += vformat(R"(
uniform float rim : hint_range(0.0, 1.0, 0.01);
uniform float rim_tint : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_rim : hint_default_white, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_CLEARCOAT]) {
		code += vformat(R"(
uniform float clearcoat : hint_range(0.0, 1.0, 0.01);
uniform float clearcoat_roughness : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_clearcoat : hint_default_white, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_ANISOTROPY]) {
		code += vformat(R"(
uniform float anisotropy_ratio : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_flowmap : hint_anisotropy, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_AMBIENT_OCCLUSION]) {
		code += vformat(R"(
uniform sampler2D texture_ambient_occlusion : hint_default_white, %s;
uniform vec4 ao_texture_channel;
uniform float ao_light_affect : hint_range(0.0, 1.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_DETAIL]) {
		code += vformat(R"(
uniform sampler2D texture_detail_albedo : source_color, %s;
uniform sampler2D texture_detail_normal : hint_normal, %s;
uniform sampler2D texture_detail_mask : hint_default_white, %s;
)",
			texfilter_str, texfilter_str, texfilter_str);
	}

	if (features[FEATURE_SUBSURFACE_SCATTERING]) {
		code += vformat(R"(
uniform float subsurface_scattering_strength : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_subsurface_scattering : hint_default_white, %s;
)",
			texfilter_str);
	}

	if (features[FEATURE_SUBSURFACE_TRANSMITTANCE]) {
		code += vformat(R"(
uniform vec4 transmittance_color : source_color;
uniform float transmittance_depth : hint_range(0.001, 8.0, 0.001);
uniform sampler2D texture_subsurface_transmittance : hint_default_white, %s;
uniform float transmittance_boost : hint_range(0.0, 1.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_BACKLIGHT]) {
		code += vformat(R"(
uniform vec4 backlight : source_color;
uniform sampler2D texture_backlight : hint_default_black, %s;
)",
			texfilter_str);
	}

	if (features[FEATURE_HEIGHT_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_heightmap : hint_default_black, %s;
uniform float heightmap_scale : hint_range(-16.0, 16.0, 0.001);
uniform int heightmap_min_layers : hint_range(1, 64);
uniform int heightmap_max_layers : hint_range(1, 64);
uniform vec2 heightmap_flip;
)",
			texfilter_height_str);
	}
	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += "varying vec3 uv1_triplanar_pos;\n";
	}
	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += "varying vec3 uv2_triplanar_pos;\n";
	}
	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
uniform float uv1_blend_sharpness : hint_range(0.0, 150.0, 0.001);
varying vec3 uv1_power_normal;
)";
	}

	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += R"(uniform float uv2_blend_sharpness : hint_range(0.0, 150.0, 0.001);
varying vec3 uv2_power_normal;
)";
	}

	code += R"(
uniform vec3 uv1_scale;
uniform vec3 uv1_offset;
uniform vec3 uv2_scale;
uniform vec3 uv2_offset;
)";

	if (flags[FLAG_USE_Z_CLIP_SCALE]) {
		code += "uniform float z_clip_scale : hint_range(0.01, 1.0, 0.01);\n";
	}

	if (flags[FLAG_USE_FOV_OVERRIDE]) {
		code += "uniform float fov_override : hint_range(1.0, 179.0, 0.1);\n";
	}

	// Generate vertex shader.
	code += R"(
void vertex() {)";

	if (flags[FLAG_SRGB_VERTEX_COLOR]) {
		code += R"(
	// Vertex Color is sRGB: Enabled
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(
				pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)),
				COLOR.rgb * (1.0 / 12.92),
				lessThan(COLOR.rgb, vec3(0.04045)));
	}
)";
	}
	if (flags[FLAG_USE_POINT_SIZE]) {
		code += R"(
	// Use Point Size: Enabled
	POINT_SIZE = point_size;
)";
	}

	if (shading_mode == SHADING_MODE_PER_VERTEX) {
		code += R"(
	// Shading Mode: Per Vertex
	ROUGHNESS = roughness;
)";
	}

	if (!flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	UV = UV * uv1_scale.xy + uv1_offset.xy;
)";
	}

	if (detail_uv == DETAIL_UV_2 && !flags[FLAG_UV2_USE_TRIPLANAR]) {
		// Don't add a newline if the UV assignment above is already performed,
		// so that UV1 and UV2 are closer to each other.
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "\n";
		}
		code += R"(	// Detail UV Layer: UV2
	UV2 = UV2 * uv2_scale.xy + uv2_offset.xy;
)";
	}

	switch (billboard_mode) {
	case BILLBOARD_DISABLED: {
	} break;
	case BILLBOARD_ENABLED: {
		// `MAIN_CAM_INV_VIEW_MATRIX` is inverse of the camera, even on shadow passes.
		// This ensures the billboard faces the camera when casting shadows.
		code += R"(
	// Billboard Mode: Enabled
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
			MAIN_CAM_INV_VIEW_MATRIX[0],
			MAIN_CAM_INV_VIEW_MATRIX[1],
			MAIN_CAM_INV_VIEW_MATRIX[2],
			MODEL_MATRIX[3]);
)";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		code += "	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);\n";
	} break;
	case BILLBOARD_FIXED_Y: {
		// `MAIN_CAM_INV_VIEW_MATRIX` is inverse of the camera, even on shadow passes.
		// This ensures the billboard faces the camera when casting shadows.
		code += R"(
	// Billboard Mode: Y-Billboard
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
			vec4(normalize(cross(vec3(0.0, 1.0, 0.0), MAIN_CAM_INV_VIEW_MATRIX[2].xyz)), 0.0),
			vec4(0.0, 1.0, 0.0, 0.0),
			vec4(normalize(cross(MAIN_CAM_INV_VIEW_MATRIX[0].xyz, vec3(0.0, 1.0, 0.0))), 0.0),
			MODEL_MATRIX[3]);
)";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		code += "	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);\n";
	} break;
	case BILLBOARD_PARTICLES: {
		// Make billboard and rotated by rotation.
		code += R"(
	// Billboard Mode: Particles
	mat4 mat_world = mat4(
			normalize(INV_VIEW_MATRIX[0]),
			normalize(INV_VIEW_MATRIX[1]),
			normalize(INV_VIEW_MATRIX[2]),
			MODEL_MATRIX[3]);
	mat_world = mat_world * mat4(
			vec4(cos(INSTANCE_CUSTOM.x), -sin(INSTANCE_CUSTOM.x), 0.0, 0.0),
			vec4(sin(INSTANCE_CUSTOM.x), cos(INSTANCE_CUSTOM.x), 0.0, 0.0),
			vec4(0.0, 0.0, 1.0, 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		// Set modelview.
		code += "	MODELVIEW_MATRIX = VIEW_MATRIX * mat_world;\n";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		// Set modelview normal and handle animation.
		code += R"(
	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);

	float h_frames = float(particles_anim_h_frames);
	float v_frames = float(particles_anim_v_frames);
	float particle_total_frames = float(particles_anim_h_frames * particles_anim_v_frames);
	float particle_frame = floor(INSTANCE_CUSTOM.z * float(particle_total_frames));
	if (!particles_anim_loop) {
		particle_frame = clamp(particle_frame, 0.0, particle_total_frames - 1.0);
	}
	else {
		particle_frame = mod(particle_frame, particle_total_frames);
	}
	UV /= vec2(h_frames, v_frames);
	UV += vec2(mod(particle_frame, h_frames) / h_frames, floor((particle_frame + 0.5) / h_frames) / v_frames);
)";
	} break;
	case BILLBOARD_MAX:
		break; // Internal value, skip.
	}

	if (flags[FLAG_FIXED_SIZE]) {
		code += R"(
	// Fixed Size: Enabled
	if (PROJECTION_MATRIX[3][3] != 0.0) {
		// Orthogonal matrix; try to do about the same with viewport size.
		float h = abs(1.0 / (2.0 * PROJECTION_MATRIX[1][1]));
		// Consistent with vertical FOV (Keep Height).
		float sc = (h * 2.0);
		MODELVIEW_MATRIX[0] *= sc;
		MODELVIEW_MATRIX[1] *= sc;
		MODELVIEW_MATRIX[2] *= sc;
	}
	else {
		// Scale by depth.
		float sc;
		if (IS_MULTIVIEW) {
			// Assuming stereo rendering.
			// Moving in the z-plane gives the illusion of the object growing/shrinking in size.
			// We need to take the full distance to camera to compensate.
			sc = length((MODELVIEW_MATRIX)[3].xyz);
		}
		else {
			sc = -(MODELVIEW_MATRIX)[3].z;
		}
		MODELVIEW_MATRIX[0] *= sc;
		MODELVIEW_MATRIX[1] *= sc;
		MODELVIEW_MATRIX[2] *= sc;
	}
)";
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR] || flags[FLAG_UV2_USE_TRIPLANAR]) {
		// Generate tangent and binormal in world space.
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += R"(
	vec3 normal = MODEL_NORMAL_MATRIX * NORMAL;
)";
		}
		else {
			code += R"(
	vec3 normal = NORMAL;
)";
		}
		code += R"(
	TANGENT = vec3(0.0, 0.0, -1.0) * abs(normal.x);
	TANGENT += vec3(1.0, 0.0, 0.0) * abs(normal.y);
	TANGENT += vec3(1.0, 0.0, 0.0) * abs(normal.z);
)";
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += "	TANGENT = inverse(MODEL_NORMAL_MATRIX) * normalize(TANGENT);\n";
		}
		else {
			code += "	TANGENT = normalize(TANGENT);\n";
		}

		code += R"(
	BINORMAL = vec3(0.0, 1.0, 0.0) * abs(normal.x);
	BINORMAL += vec3(0.0, 0.0, -1.0) * abs(normal.y);
	BINORMAL += vec3(0.0, 1.0, 0.0) * abs(normal.z);
)";
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += "	BINORMAL = inverse(MODEL_NORMAL_MATRIX) * normalize(BINORMAL);\n";
		}
		else {
			code += "	BINORMAL = normalize(BINORMAL);\n";
		}
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += R"(
	// UV1 Triplanar: Enabled (with World Triplanar)
	uv1_power_normal = pow(abs(normal), vec3(uv1_blend_sharpness));
	uv1_triplanar_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz * uv1_scale + uv1_offset;
)";
		}
		else {
			code += R"(
	// UV1 Triplanar: Enabled
	uv1_power_normal = pow(abs(NORMAL), vec3(uv1_blend_sharpness));
	uv1_triplanar_pos = VERTEX * uv1_scale + uv1_offset;
)";
		}
		code += R"(	uv1_power_normal /= dot(uv1_power_normal, vec3(1.0));
	uv1_triplanar_pos *= vec3(1.0, -1.0, 1.0);
)";
	}

	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		if (flags[FLAG_UV2_USE_WORLD_TRIPLANAR]) {
			code += R"(
	// UV2 Triplanar: Enabled (with World Triplanar)
	uv2_power_normal = pow(abs(mat3(MODEL_MATRIX) * NORMAL), vec3(uv2_blend_sharpness));
	uv2_triplanar_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz * uv2_scale + uv2_offset;
)";
		}
		else {
			code += R"(
	// UV2 Triplanar: Enabled
	uv2_power_normal = pow(abs(NORMAL), vec3(uv2_blend_sharpness));
	uv2_triplanar_pos = VERTEX * uv2_scale + uv2_offset;
)";
		}
		code += R"(	uv2_power_normal /= dot(uv2_power_normal, vec3(1.0));
	uv2_triplanar_pos *= vec3(1.0, -1.0, 1.0);
)";
	}

	if (grow_enabled) {
		code += R"(
	// Grow: Enabled
	VERTEX += NORMAL * grow;
)";
	}

	if (flags[FLAG_USE_Z_CLIP_SCALE]) {
		code += R"(
	Z_CLIP_SCALE = z_clip_scale;
)";
	}

	if (flags[FLAG_USE_FOV_OVERRIDE]) {
		code += R"(
	if (!IN_SHADOW_PASS) {
		float flip_y = sign(PROJECTION_MATRIX[1][1]);
		float aspect = PROJECTION_MATRIX[1][1] / PROJECTION_MATRIX[0][0];
		float f = flip_y / tan(fov_override * PI / 360.0);
		PROJECTION_MATRIX[0][0] = f / aspect;
		PROJECTION_MATRIX[1][1] = f;
	}
)";
	}

	// End of the vertex shader function.
	code += "}\n";

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
float msdf_median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}
)";
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR] || flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += R"(
vec4 triplanar_texture(sampler2D p_sampler, vec3 p_weights, vec3 p_triplanar_pos) {
	vec4 samp = vec4(0.0);
	samp += texture(p_sampler, p_triplanar_pos.xy) * p_weights.z;
	samp += texture(p_sampler, p_triplanar_pos.xz) * p_weights.y;
	samp += texture(p_sampler, p_triplanar_pos.zy * vec2(-1.0, 1.0)) * p_weights.x;
	return samp;
}
)";
	}

	// Generate fragment shader.
	code += R"(
void fragment() {)";

	if (!flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	vec2 base_uv = UV;
)";
	}

	if ((features[FEATURE_DETAIL] && detail_uv == DETAIL_UV_2) ||
		(features[FEATURE_AMBIENT_OCCLUSION] && flags[FLAG_AO_ON_UV2]) ||
		(features[FEATURE_EMISSION] && flags[FLAG_EMISSION_ON_UV2])) {
		// Don't add a newline if the UV assignment above is already performed,
		// so that UV1 and UV2 are closer to each other.
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "\n";
		}
		code += R"(	vec2 base_uv2 = UV2;
)";
	}

	if (features[FEATURE_HEIGHT_MAPPING] && flags[FLAG_UV1_USE_TRIPLANAR]) {
		// Display both resource name and albedo texture name.
		// Materials are often built-in to scenes, so displaying the resource name alone may not be
		// meaningful. On the other hand, albedo textures are almost always external to the scene.
		if (textures[TEXTURE_ALBEDO].is_valid()) {
			WARN_PRINT(vformat("%s (albedo %s): Height mapping is not supported on triplanar "
							   "materials. Ignoring height mapping in favor of triplanar mapping.",
				get_path(), textures[TEXTURE_ALBEDO]->get_path()));
		}
		else if (!get_path().is_empty()) {
			WARN_PRINT(vformat("%s: Height mapping is not supported on triplanar materials. "
							   "Ignoring height mapping in favor of triplanar mapping.",
				get_path()));
		}
		else {
			// Resource wasn't saved yet.
			WARN_PRINT("Height mapping is not supported on triplanar materials. Ignoring height "
					   "mapping in favor of triplanar mapping.");
		}
	}

	// Heightmapping isn't supported at the same time as triplanar mapping.
	if (features[FEATURE_HEIGHT_MAPPING] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		// Binormal is negative due to mikktspace. Flipping it "unflips" it.
		code += R"(
	{
		// Height: Enabled
		vec3 view_dir = normalize(normalize(-VERTEX + EYE_OFFSET) * mat3(TANGENT * heightmap_flip.x, -BINORMAL * heightmap_flip.y, NORMAL));
)";

		if (deep_parallax) {
			// Multiply the heightmap scale by 0.01 to improve heightmap scale usability.
			code += R"(
		// Height Deep Parallax: Enabled
		float num_layers = mix(float(heightmap_max_layers), float(heightmap_min_layers), abs(dot(vec3(0.0, 0.0, 1.0), view_dir)));
		float layer_depth = 1.0 / num_layers;
		float current_layer_depth = 0.0;
		vec2 p = view_dir.xy * heightmap_scale * 0.01;
		vec2 delta = p / num_layers;
		vec2 ofs = base_uv;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float depth = texture(texture_heightmap, ofs).r;\n";
			}
			else {
				code += "		float depth = 1.0 - texture(texture_heightmap, ofs).r;\n";
			}
			code += R"(
		float current_depth = 0.0;
		while (current_depth < depth) {
			ofs -= delta;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "			depth = texture(texture_heightmap, ofs).r;\n";
			}
			else {
				code += "			depth = 1.0 - texture(texture_heightmap, ofs).r;\n";
			}
			code += R"(
			current_depth += layer_depth;
		}

		vec2 prev_ofs = ofs + delta;
		float after_depth = depth - current_depth;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float before_depth = texture(texture_heightmap, prev_ofs).r - "
						"current_depth + layer_depth;\n";
			}
			else {
				code += "		float before_depth = (1.0 - texture(texture_heightmap, "
						"prev_ofs).r) - current_depth + layer_depth;\n";
			}
			code += R"(
		float weight = after_depth / (after_depth - before_depth);
		ofs = mix(ofs, prev_ofs, weight);
)";

		}
		else {
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float depth = texture(texture_heightmap, base_uv).r;\n";
			}
			else {
				code += "		float depth = 1.0 - texture(texture_heightmap, base_uv).r;\n";
			}
			// Use offset limiting to improve the appearance of non-deep parallax.
			// This reduces the impression of depth, but avoids visible warping in the distance.
			// Multiply the heightmap scale by 0.01 to improve heightmap scale usability.
			code += "		vec2 ofs = base_uv - view_dir.xy * depth * heightmap_scale * 0.01;\n";
		}

		code += "		base_uv = ofs;\n";
		if (features[FEATURE_DETAIL] && detail_uv == DETAIL_UV_2) {
			code += "		base_uv2 -= ofs;\n";
		}

		code += "	}\n";
	}

	if (flags[FLAG_USE_POINT_SIZE]) {
		code += R"(
	// Use Point Size: Enabled
	vec4 albedo_tex = texture(texture_albedo, POINT_COORD);
)";
	}
	else {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	vec4 albedo_tex = triplanar_texture(texture_albedo, uv1_power_normal, uv1_triplanar_pos);
)";
		}
		else {
			code += R"(
	vec4 albedo_tex = texture(texture_albedo, base_uv);
)";
		}
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	{
		// Albedo Texture MSDF: Enabled
		if (!OUTPUT_IS_SRGB) {
			albedo_tex.rgb = mix(
					vec3(1.0 + 0.055) * pow(albedo_tex.rgb, vec3(1.0 / 2.4)) - vec3(0.055),
					vec3(12.92) * albedo_tex.rgb,
					lessThan(albedo_tex.rgb, vec3(0.0031308)));
		}
		vec2 msdf_size = vec2(msdf_pixel_range) / vec2(albedo_texture_size);
)";
		if (flags[FLAG_USE_POINT_SIZE]) {
			code += "		vec2 dest_size = vec2(1.0) / fwidth(POINT_COORD);\n";
		}
		else {
			code += "		vec2 dest_size = vec2(1.0) / fwidth(base_uv);\n";
		}
		code += R"(
		float px_size = max(0.5 * dot(msdf_size, dest_size), 1.0);
		float d = msdf_median(albedo_tex.r, albedo_tex.g, albedo_tex.b);
		if (msdf_outline_size > 0.0) {
			float cr = clamp(msdf_outline_size, 0.0, (msdf_pixel_range / 2.0) - 1.0) / msdf_pixel_range;
			d = min(d, albedo_tex.a);
			albedo_tex.a = clamp((d - 0.5 + cr) * px_size, 0.0, 1.0);
		}
		else {
			albedo_tex.a = clamp((d - 0.5) * px_size + 0.5, 0.0, 1.0);
		}
		albedo_tex.rgb = vec3(1.0);
	}
)";
	}
	else if (flags[FLAG_ALBEDO_TEXTURE_FORCE_SRGB]) {
		code += R"(
	// Albedo Texture Force sRGB: Enabled
	albedo_tex.rgb = mix(
			pow((albedo_tex.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)),
			albedo_tex.rgb.rgb * (1.0 / 12.92),
			lessThan(albedo_tex.rgb, vec3(0.04045)));
)";
	}

	if (flags[FLAG_ALBEDO_FROM_VERTEX_COLOR]) {
		code += R"(
	// Vertex Color Use as Albedo: Enabled
	albedo_tex *= COLOR;

)";
	}
	code += "	ALBEDO = albedo.rgb * albedo_tex.rgb;\n";

	if (!orm) {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	float metallic_tex = dot(triplanar_texture(texture_metallic, uv1_power_normal, uv1_triplanar_pos), metallic_texture_channel);
)";
		}
		else {
			code += R"(
	float metallic_tex = dot(texture(texture_metallic, base_uv), metallic_texture_channel);
)";
		}

		code += R"(	METALLIC = metallic_tex * metallic;
	SPECULAR = specular;
)";
		switch (roughness_texture_channel) {
		case TEXTURE_CHANNEL_RED: {
			code += R"(
	vec4 roughness_texture_channel = vec4(1.0, 0.0, 0.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_GREEN: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 1.0, 0.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_BLUE: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 0.0, 1.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_ALPHA: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 0.0, 0.0, 1.0);
)";
		} break;
		case TEXTURE_CHANNEL_GRAYSCALE: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.333333, 0.333333, 0.333333, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_MAX:
			break; // Internal value, skip.
		}

		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	float roughness_tex = dot(triplanar_texture(texture_roughness, "
					"uv1_power_normal, uv1_triplanar_pos), roughness_texture_channel);\n";
		}
		else {
			code += "	float roughness_tex = dot(texture(texture_roughness, base_uv), "
					"roughness_texture_channel);\n";
		}
		code += R"(	ROUGHNESS = roughness_tex * roughness;
)";
	}
	else {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	vec4 orm_tex = triplanar_texture(texture_orm, uv1_power_normal, uv1_triplanar_pos);
)";
		}
		else {
			code += R"(
	vec4 orm_tex = texture(texture_orm, base_uv);
)";
		}

		code += R"(	ROUGHNESS = orm_tex.g;
 	METALLIC = orm_tex.b;
)";
	}

	if (features[FEATURE_NORMAL_MAPPING]) {
		code += R"(
	// Normal Map: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	NORMAL_MAP = triplanar_texture(texture_normal, uv1_power_normal, "
					"uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	NORMAL_MAP = texture(texture_normal, base_uv).rgb;\n";
		}
		code += "	NORMAL_MAP_DEPTH = normal_scale;\n";
	}

	if (features[FEATURE_BENT_NORMAL_MAPPING]) {
		code += R"(
	// Bent Normal Map: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	BENT_NORMAL_MAP = triplanar_texture(texture_bent_normal, uv1_power_normal, "
					"uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	BENT_NORMAL_MAP = texture(texture_bent_normal, base_uv).rgb;\n";
		}
	}

	if (features[FEATURE_EMISSION]) {
		code += R"(
	// Emission: Enabled
)";
		if (flags[FLAG_EMISSION_ON_UV2]) {
			if (flags[FLAG_UV2_USE_TRIPLANAR]) {
				code += "	vec3 emission_tex = triplanar_texture(texture_emission, "
						"uv2_power_normal, uv2_triplanar_pos).rgb;\n";
			}
			else {
				code += "	vec3 emission_tex = texture(texture_emission, base_uv2).rgb;\n";
			}
		}
		else {
			if (flags[FLAG_UV1_USE_TRIPLANAR]) {
				code += "	vec3 emission_tex = triplanar_texture(texture_emission, "
						"uv1_power_normal, uv1_triplanar_pos).rgb;\n";
			}
			else {
				code += "	vec3 emission_tex = texture(texture_emission, base_uv).rgb;\n";
			}
		}

		if (emission_op == EMISSION_OP_ADD) {
			code += R"(	// Emission Operator: Add
	EMISSION = (emission.rgb + emission_tex) * emission_energy;
)";
		}
		else {
			code += R"(	// Emission Operator: Multiply
	EMISSION = (emission.rgb * emission_tex) * emission_energy;
)";
		}
	}

	if (features[FEATURE_REFRACTION]) {
		if (features[FEATURE_NORMAL_MAPPING]) {
			code += R"(
	// Refraction: Enabled (with normal map texture)
	vec3 unpacked_normal = NORMAL_MAP;
	unpacked_normal.xy = unpacked_normal.xy * 2.0 - 1.0;
	unpacked_normal.z = sqrt(max(0.0, 1.0 - dot(unpacked_normal.xy, unpacked_normal.xy)));
	vec3 ref_normal = normalize(mix(
			NORMAL,
			TANGENT * unpacked_normal.x + BINORMAL * unpacked_normal.y + NORMAL * unpacked_normal.z,
			NORMAL_MAP_DEPTH));
)";
		}
		else {
			code += R"(
	// Refraction: Enabled
	vec3 ref_normal = NORMAL;
)";
		}
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 ref_ofs = SCREEN_UV - ref_normal.xy * "
					"dot(triplanar_texture(texture_refraction, uv1_power_normal, "
					"uv1_triplanar_pos), refraction_texture_channel) * refraction;\n";
		}
		else {
			code += "	vec2 ref_ofs = SCREEN_UV - ref_normal.xy * dot(texture(texture_refraction, "
					"base_uv), refraction_texture_channel) * refraction;\n";
		}
		code += R"(
	float ref_amount = 1.0 - albedo.a * albedo_tex.a;

	float refraction_depth_tex = textureLod(depth_texture, ref_ofs, 0.0).r;
	vec4 ndc = OUTPUT_IS_SRGB ? vec4(vec3(SCREEN_UV, refraction_depth_tex) * 2.0 - 1.0, 1.0) : vec4(SCREEN_UV * 2.0 - 1.0, refraction_depth_tex, 1.0);
	vec4 refraction_view_pos = INV_PROJECTION_MATRIX * ndc;
	refraction_view_pos.xyz /= refraction_view_pos.w;

	// If the depth buffer is lower then the model's Z position, use the refracted UV, otherwise use the normal screen UV.
	// At low depth differences, decrease refraction intensity to avoid sudden discontinuities.
	EMISSION += textureLod(screen_texture, mix(SCREEN_UV, ref_ofs, smoothstep(0.0, 1.0, VERTEX.z - refraction_view_pos.z)), ROUGHNESS * 8.0).rgb * ref_amount * EXPOSURE;
	ALBEDO *= 1.0 - ref_amount;
	// Force transparency on the material (required for refraction).
	ALPHA = 1.0;
)";

	}
	else if (transparency != TRANSPARENCY_DISABLED || flags[FLAG_USE_SHADOW_TO_OPACITY] ||
			   (distance_fade == DISTANCE_FADE_PIXEL_ALPHA) || proximity_fade_enabled) {
		code += "	ALPHA *= albedo.a * albedo_tex.a;\n";
	}
	if (transparency == TRANSPARENCY_ALPHA_HASH) {
		code += "	ALPHA_HASH_SCALE = alpha_hash_scale;\n";
	}
	else if (transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		code += "	ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;\n";
	}
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF &&
		(transparency == TRANSPARENCY_ALPHA_HASH || transparency == TRANSPARENCY_ALPHA_SCISSOR)) {
		code += "	ALPHA_ANTIALIASING_EDGE = alpha_antialiasing_edge;\n";
		code += "	ALPHA_TEXTURE_COORDINATE = UV * vec2(albedo_texture_size);\n";
	}

	if (proximity_fade_enabled) {
		code += R"(
	// Proximity Fade: Enabled
	float proximity_depth_tex = textureLod(depth_texture, SCREEN_UV, 0.0).r;
	vec4 ndc = OUTPUT_IS_SRGB ? vec4(vec3(SCREEN_UV, proximity_depth_tex) * 2.0 - 1.0, 1.0) : vec4(SCREEN_UV * 2.0 - 1.0, proximity_depth_tex, 1.0);
	vec4 proximity_view_pos = INV_PROJECTION_MATRIX * ndc;
	proximity_view_pos.xyz /= proximity_view_pos.w;
	ALPHA *= clamp(1.0 - smoothstep(proximity_view_pos.z + proximity_fade_distance, proximity_view_pos.z, VERTEX.z), 0.0, 1.0);
)";
	}

	if (distance_fade != DISTANCE_FADE_DISABLED) {
		// Use the slightly more expensive circular fade (distance to the object) instead of linear
		// (Z distance), so that the fade is always the same regardless of the camera angle.
		if ((distance_fade == DISTANCE_FADE_OBJECT_DITHER ||
				distance_fade == DISTANCE_FADE_PIXEL_DITHER)) {
			code += "\n	{";

			if (distance_fade == DISTANCE_FADE_OBJECT_DITHER) {
				code += R"(
		// Distance Fade: Object Dither
		float fade_distance = length((VIEW_MATRIX * MODEL_MATRIX[3]));
)";
			}
			else {
				code += R"(
		// Distance Fade: Pixel Dither
		float fade_distance = length(VERTEX);
)";
			}
			code += R"(
		// Use interleaved gradient noise, which is fast but still looks good.
		const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
		float fade = clamp(smoothstep(distance_fade_min, distance_fade_max, fade_distance), 0.0, 1.0);
		// Use a hard cap to prevent a few stray pixels from remaining when past the fade-out distance.
		if (fade < 0.001 || fade < fract(magic.z * fract(dot(FRAGCOORD.xy, magic.xy)))) {
			discard;
		}
	}
)";
		}
		else {
			code += R"(
	// Distance Fade: Pixel Alpha
	ALPHA *= clamp(smoothstep(distance_fade_min, distance_fade_max, length(VERTEX)), 0.0, 1.0);
)";
		}
	}

	if (features[FEATURE_RIM]) {
		code += R"(
	// Rim: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 rim_tex = triplanar_texture(texture_rim, uv1_power_normal, "
					"uv1_triplanar_pos).xy;\n";
		}
		else {
			code += "	vec2 rim_tex = texture(texture_rim, base_uv).xy;\n";
		}
		code += R"(	RIM = rim * rim_tex.x;
	RIM_TINT = rim_tint * rim_tex.y;
)";
	}

	if (features[FEATURE_CLEARCOAT]) {
		code += R"(
	// Clearcoat: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 clearcoat_tex = triplanar_texture(texture_clearcoat, "
					"uv1_power_normal, uv1_triplanar_pos).xy;\n";
		}
		else {
			code += "	vec2 clearcoat_tex = texture(texture_clearcoat, base_uv).xy;\n";
		}
		code += R"(	CLEARCOAT = clearcoat * clearcoat_tex.x;
	CLEARCOAT_ROUGHNESS = clearcoat_roughness * clearcoat_tex.y;
)";
	}

	if (features[FEATURE_ANISOTROPY]) {
		code += R"(
	// Anisotropy: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec3 anisotropy_tex = triplanar_texture(texture_flowmap, uv1_power_normal, "
					"uv1_triplanar_pos).rga;\n";
		}
		else {
			code += "	vec3 anisotropy_tex = texture(texture_flowmap, base_uv).rga;\n";
		}
		code += R"(	ANISOTROPY = anisotropy_ratio * anisotropy_tex.b;
	ANISOTROPY_FLOW = anisotropy_tex.rg * 2.0 - 1.0;
)";
	}

	if (features[FEATURE_AMBIENT_OCCLUSION]) {
		code += R"(
	// Ambient Occlusion: Enabled
)";
		if (!orm) {
			if (flags[FLAG_AO_ON_UV2]) {
				if (flags[FLAG_UV2_USE_TRIPLANAR]) {
					code += "	AO = dot(triplanar_texture(texture_ambient_occlusion, "
							"uv2_power_normal, uv2_triplanar_pos), ao_texture_channel);\n";
				}
				else {
					code += "	AO = dot(texture(texture_ambient_occlusion, base_uv2), "
							"ao_texture_channel);\n";
				}
			}
			else {
				if (flags[FLAG_UV1_USE_TRIPLANAR]) {
					code += "	AO = dot(triplanar_texture(texture_ambient_occlusion, "
							"uv1_power_normal, uv1_triplanar_pos), ao_texture_channel);\n";
				}
				else {
					code += "	AO = dot(texture(texture_ambient_occlusion, base_uv), "
							"ao_texture_channel);\n";
				}
			}
		}
		else {
			code += "	AO = orm_tex.r;\n";
		}

		code += "	AO_LIGHT_AFFECT = ao_light_affect;\n";
	}

	if (features[FEATURE_SUBSURFACE_SCATTERING]) {
		code += R"(
	// Subsurface Scattering: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	float sss_tex = triplanar_texture(texture_subsurface_scattering, "
					"uv1_power_normal, uv1_triplanar_pos).r;\n";
		}
		else {
			code += "	float sss_tex = texture(texture_subsurface_scattering, base_uv).r;\n";
		}
		code += "	SSS_STRENGTH = subsurface_scattering_strength * sss_tex;\n";
	}

	if (features[FEATURE_SUBSURFACE_TRANSMITTANCE]) {
		code += R"(
	// Subsurface Scattering Transmittance: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec4 trans_color_tex = triplanar_texture(texture_subsurface_transmittance, "
					"uv1_power_normal, uv1_triplanar_pos);\n";
		}
		else {
			code +=
				"	vec4 trans_color_tex = texture(texture_subsurface_transmittance, base_uv);\n";
		}
		code += "	SSS_TRANSMITTANCE_COLOR = transmittance_color * trans_color_tex;\n";

		code += R"(	SSS_TRANSMITTANCE_DEPTH = transmittance_depth;
	SSS_TRANSMITTANCE_BOOST = transmittance_boost;
)";
	}

	if (features[FEATURE_BACKLIGHT]) {
		code += R"(
	// Backlight: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec3 backlight_tex = triplanar_texture(texture_backlight, "
					"uv1_power_normal, uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	vec3 backlight_tex = texture(texture_backlight, base_uv).rgb;\n";
		}
		code += "	BACKLIGHT = (backlight.rgb + backlight_tex);\n";
	}

	if (features[FEATURE_DETAIL]) {
		code += R"(
	// Detail: Enabled
)";
		const bool triplanar = (flags[FLAG_UV1_USE_TRIPLANAR] && detail_uv == DETAIL_UV_1) ||
							   (flags[FLAG_UV2_USE_TRIPLANAR] && detail_uv == DETAIL_UV_2);
		if (triplanar) {
			const String tp_uv = detail_uv == DETAIL_UV_1 ? "uv1" : "uv2";
			code += vformat(
				R"(	vec4 detail_tex = triplanar_texture(texture_detail_albedo, %s_power_normal, %s_triplanar_pos);
	vec4 detail_norm_tex = triplanar_texture(texture_detail_normal, %s_power_normal, %s_triplanar_pos);
)",
				tp_uv, tp_uv, tp_uv, tp_uv);
		}
		else {
			const String det_uv = detail_uv == DETAIL_UV_1 ? "base_uv" : "base_uv2";
			code += vformat(R"(	vec4 detail_tex = texture(texture_detail_albedo, %s);
	vec4 detail_norm_tex = texture(texture_detail_normal, %s);
)",
				det_uv, det_uv);
		}

		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec4 detail_mask_tex = triplanar_texture(texture_detail_mask, "
					"uv1_power_normal, uv1_triplanar_pos);\n";
		}
		else {
			code += "	vec4 detail_mask_tex = texture(texture_detail_mask, base_uv);\n";
		}

		switch (detail_blend_mode) {
		case BLEND_MODE_MIX: {
			code += R"(
	// Detail Blend Mode: Mix
	vec3 detail = mix(ALBEDO.rgb, detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_ADD: {
			code += R"(
	// Detail Blend Mode: Add
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb + detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_SUB: {
			code += R"(
	// Detail Blend Mode: Subtract
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb - detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_MUL: {
			code += R"(
	// Detail Blend Mode: Multiply
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb * detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_PREMULT_ALPHA: {
			// This is unlikely to ever be used for detail textures, and in order for it to function
			// in the editor, another bit must be used in MaterialKey, but there are only 5 bits
			// left, so I'm going to leave this disabled unless it's actually requested.
			// code += "\tvec3 detail = (1.0-detail_tex.a)*ALBEDO.rgb+detail_tex.rgb;\n";
		} break;
		case BLEND_MODE_MAX:
			break; // Internal value, skip.
		}

		code += R"(	vec3 detail_norm = mix(NORMAL_MAP, detail_norm_tex.rgb, detail_tex.a);
	NORMAL_MAP = mix(NORMAL_MAP, detail_norm, detail_mask_tex.r);
	ALBEDO.rgb = mix(ALBEDO.rgb, detail, detail_mask_tex.r);)";
	}

	code += "}\n";

	// We must create the shader outside the shader_map_mutex to avoid potential deadlocks with
	// other tasks in the WorkerThreadPool simultaneously creating materials, which
	// may also hold the shared shader_map_mutex lock.
	RID new_shader = Renderer::shader_create_from_code(code);
	MutexLock lock(shader_map_mutex);
	ShaderData* v = shader_map.getptr(mk);
	if (unlikely(v)) {
		// We raced and managed to create the same key concurrently, so we'll free the shader we
		// just created, given we know it isn't used, and use the winner.
		Renderer::free_rid(new_shader);
	}
	else {
		ShaderData shader_data;
		shader_data.shader = new_shader;
		// ShaderData will be inserted with a users count of 0, but we
		// increment unconditionally outside this if block, whilst still under lock.
		v = &shader_map.insert(mk, shader_data)->value;
	}

	shader_rid = v->shader;
	v->users++;

	if (_get_material().is_valid()) {
		Renderer::material_set_shader(_get_material(), shader_rid);
	}
}

void StandardMaterial3D::flush_changes()
{
	SelfList<StandardMaterial3D>::List copy;
	{
		MutexLock lock(material_mutex);
		while (SelfList<StandardMaterial3D>* E = dirty_materials.first()) {
			dirty_materials.remove(E);
			copy.add(E);
		}
	}

	while (SelfList<StandardMaterial3D>* E = copy.first()) {
		E->self()->_update_shader();
		copy.remove(E);
	}
}

void StandardMaterial3D::_queue_shader_change()
{
	if (!_is_initialized()) {
		return;
	}

	MutexLock lock(material_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

void StandardMaterial3D::set_albedo(const Color& p_albedo) {}

Color StandardMaterial3D::get_albedo() const { return albedo; }

float StandardMaterial3D::get_specular() const { return specular; }

float StandardMaterial3D::get_roughness() const { return roughness; }

float StandardMaterial3D::get_metallic() const { return metallic; }

Color StandardMaterial3D::get_emission() const { return emission; }

float StandardMaterial3D::get_emission_energy_multiplier() const { return emission_energy_multiplier; }

float StandardMaterial3D::get_emission_intensity() const { return emission_intensity; }

float StandardMaterial3D::get_normal_scale() const { return normal_scale; }

float StandardMaterial3D::get_rim() const { return rim; }

float StandardMaterial3D::get_rim_tint() const { return rim_tint; }

float StandardMaterial3D::get_ao_light_affect() const { return ao_light_affect; }

float StandardMaterial3D::get_clearcoat() const { return clearcoat; }

float StandardMaterial3D::get_clearcoat_roughness() const { return clearcoat_roughness; }

float StandardMaterial3D::get_anisotropy() const { return anisotropy; }

float StandardMaterial3D::get_heightmap_scale() const { return heightmap_scale; }

float StandardMaterial3D::get_subsurface_scattering_strength() const
{
	return subsurface_scattering_strength;
}

Color StandardMaterial3D::get_transmittance_color() const { return transmittance_color; }

float StandardMaterial3D::get_transmittance_depth() const { return transmittance_depth; }

float StandardMaterial3D::get_transmittance_boost() const { return transmittance_boost; }

Color StandardMaterial3D::get_backlight() const { return backlight; }

float StandardMaterial3D::get_refraction() const { return refraction; }

void StandardMaterial3D::set_detail_uv(DetailUV p_detail_uv)
{
	if (detail_uv == p_detail_uv) {
		return;
	}

	detail_uv = p_detail_uv;
	_queue_shader_change();
}

StandardMaterial3D::DetailUV StandardMaterial3D::get_detail_uv() const { return detail_uv; }

void StandardMaterial3D::set_blend_mode(BlendMode p_mode)
{
	if (blend_mode == p_mode) {
		return;
	}

	blend_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::BlendMode StandardMaterial3D::get_blend_mode() const { return blend_mode; }

void StandardMaterial3D::set_detail_blend_mode(BlendMode p_mode)
{
	detail_blend_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::BlendMode StandardMaterial3D::get_detail_blend_mode() const
{
	return detail_blend_mode;
}

void StandardMaterial3D::set_transparency(Transparency p_transparency) {}

StandardMaterial3D::Transparency StandardMaterial3D::get_transparency() const { return transparency; }

StandardMaterial3D::AlphaAntiAliasing StandardMaterial3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void StandardMaterial3D::set_shading_mode(ShadingMode p_shading_mode) {}

StandardMaterial3D::ShadingMode StandardMaterial3D::get_shading_mode() const { return shading_mode; }

void StandardMaterial3D::set_depth_draw_mode(DepthDrawMode p_mode)
{
	if (depth_draw_mode == p_mode) {
		return;
	}

	depth_draw_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::DepthDrawMode StandardMaterial3D::get_depth_draw_mode() const
{
	return depth_draw_mode;
}

void StandardMaterial3D::set_depth_test(DepthTest p_func)
{
	if (depth_test == p_func) {
		return;
	}

	depth_test = p_func;
	_queue_shader_change();
}

StandardMaterial3D::DepthTest StandardMaterial3D::get_depth_test() const { return depth_test; }

void StandardMaterial3D::set_cull_mode(CullMode p_mode)
{
	if (cull_mode == p_mode) {
		return;
	}

	cull_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::CullMode StandardMaterial3D::get_cull_mode() const { return cull_mode; }

void StandardMaterial3D::set_diffuse_mode(DiffuseMode p_mode)
{
	if (diffuse_mode == p_mode) {
		return;
	}

	diffuse_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::DiffuseMode StandardMaterial3D::get_diffuse_mode() const { return diffuse_mode; }

void StandardMaterial3D::set_specular_mode(SpecularMode p_mode)
{
	if (specular_mode == p_mode) {
		return;
	}

	specular_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::SpecularMode StandardMaterial3D::get_specular_mode() const { return specular_mode; }

void StandardMaterial3D::set_flag(Flags p_flag, bool p_enabled) {}

bool StandardMaterial3D::get_flag(Flags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void StandardMaterial3D::set_feature(Feature p_feature, bool p_enabled)
{
	ERR_FAIL_INDEX(p_feature, FEATURE_MAX);
	if (features[p_feature] == p_enabled) {
		return;
	}

	features[p_feature] = p_enabled;
	_queue_shader_change();
}

bool StandardMaterial3D::get_feature(Feature p_feature) const
{
	ERR_FAIL_INDEX_V(p_feature, FEATURE_MAX, false);
	return features[p_feature];
}

Ref<Texture2D> StandardMaterial3D::get_texture(TextureParam p_param) const
{
	ERR_FAIL_INDEX_V(p_param, TEXTURE_MAX, Ref<Texture2D>());
	return textures[p_param];
}

Ref<Texture2D> StandardMaterial3D::get_texture_by_name(const String& p_name) const
{
	for (int i = 0; i < (int)StandardMaterial3D::TEXTURE_MAX; i++) {
		TextureParam param = TextureParam(i);
		if (p_name == shader_names->texture_names[param]) {
			return textures[param];
		}
	}
	return Ref<Texture2D>();
}

void StandardMaterial3D::set_texture_filter(TextureFilter p_filter)
{
	texture_filter = p_filter;
	_queue_shader_change();
}

StandardMaterial3D::TextureFilter StandardMaterial3D::get_texture_filter() const { return texture_filter; }

void StandardMaterial3D::set_point_size(float p_point_size) {}

float StandardMaterial3D::get_point_size() const { return point_size; }

Vector3 StandardMaterial3D::get_uv1_scale() const { return uv1_scale; }

Vector3 StandardMaterial3D::get_uv1_offset() const { return uv1_offset; }

float StandardMaterial3D::get_uv1_triplanar_blend_sharpness() const { return uv1_triplanar_sharpness; }

Vector3 StandardMaterial3D::get_uv2_scale() const { return uv2_scale; }

Vector3 StandardMaterial3D::get_uv2_offset() const { return uv2_offset; }

float StandardMaterial3D::get_uv2_triplanar_blend_sharpness() const { return uv2_triplanar_sharpness; }

StandardMaterial3D::BillboardMode StandardMaterial3D::get_billboard_mode() const { return billboard_mode; }

int StandardMaterial3D::get_particles_anim_h_frames() const { return particles_anim_h_frames; }

int StandardMaterial3D::get_particles_anim_v_frames() const { return particles_anim_v_frames; }

bool StandardMaterial3D::get_particles_anim_loop() const { return particles_anim_loop; }

bool StandardMaterial3D::is_heightmap_deep_parallax_enabled() const { return deep_parallax; }

int StandardMaterial3D::get_heightmap_deep_parallax_min_layers() const
{
	return deep_parallax_min_layers;
}

int StandardMaterial3D::get_heightmap_deep_parallax_max_layers() const
{
	return deep_parallax_max_layers;
}

bool StandardMaterial3D::get_heightmap_deep_parallax_flip_tangent() const
{
	return heightmap_parallax_flip_tangent;
}

bool StandardMaterial3D::get_heightmap_deep_parallax_flip_binormal() const
{
	return heightmap_parallax_flip_binormal;
}

bool StandardMaterial3D::is_grow_enabled() const { return grow_enabled; }

float StandardMaterial3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

float StandardMaterial3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

float StandardMaterial3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

float StandardMaterial3D::get_grow() const { return grow; }

static Vector4 _get_texture_mask(StandardMaterial3D::TextureChannel p_channel)
{
	static const Vector4 masks[5] = {
		Vector4(1, 0, 0, 0),
		Vector4(0, 1, 0, 0),
		Vector4(0, 0, 1, 0),
		Vector4(0, 0, 0, 1),
		Vector4(0.3333333, 0.3333333, 0.3333333, 0),
	};

	return masks[p_channel];
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_metallic_texture_channel() const
{
	return metallic_texture_channel;
}

void StandardMaterial3D::set_roughness_texture_channel(TextureChannel p_channel)
{
	ERR_FAIL_INDEX(p_channel, 5);
	roughness_texture_channel = p_channel;
	_queue_shader_change();
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_roughness_texture_channel() const
{
	return roughness_texture_channel;
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_ao_texture_channel() const
{
	return ao_texture_channel;
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_refraction_texture_channel() const
{
	return refraction_texture_channel;
}

float StandardMaterial3D::get_z_clip_scale() const { return z_clip_scale; }

float StandardMaterial3D::get_fov_override() const { return fov_override; }

Ref<Material> StandardMaterial3D::get_material_for_2d(bool p_shaded, Transparency p_transparency,
	bool p_double_sided, bool p_billboard, bool p_billboard_y, bool p_msdf, bool p_no_depth,
	bool p_fixed_size, TextureFilter p_filter, AlphaAntiAliasing p_alpha_antialiasing_mode,
	bool p_texture_repeat, RID* r_shader_rid)
{
	uint64_t key = 0;
	key |= ((int8_t)p_shaded & 0x01) << 0;
	key |= ((int8_t)p_transparency & 0x07) << 1; // Bits 1-3.
	key |= ((int8_t)p_double_sided & 0x01) << 4;
	key |= ((int8_t)p_billboard & 0x01) << 5;
	key |= ((int8_t)p_billboard_y & 0x01) << 6;
	key |= ((int8_t)p_msdf & 0x01) << 7;
	key |= ((int8_t)p_no_depth & 0x01) << 8;
	key |= ((int8_t)p_fixed_size & 0x01) << 9;
	key |= ((int8_t)p_filter & 0x07) << 10;					 // Bits 10-12.
	key |= ((int8_t)p_alpha_antialiasing_mode & 0x07) << 13; // Bits 13-15.
	key |= ((int8_t)p_texture_repeat & 0x01) << 16;

	if (materials_for_2d.has(key)) {
		if (r_shader_rid) {
			*r_shader_rid = materials_for_2d[key]->get_shader_rid();
		}
		return materials_for_2d[key];
	}

	Ref<StandardMaterial3D> material;
	material.instantiate();

	material->set_shading_mode(p_shaded ? SHADING_MODE_PER_PIXEL : SHADING_MODE_UNSHADED);
	material->set_transparency(p_transparency);
	material->set_cull_mode(p_double_sided ? CULL_DISABLED : CULL_BACK);
	material->set_alpha_antialiasing(p_alpha_antialiasing_mode);
	material->set_texture_filter(p_filter);
	if (p_billboard || p_billboard_y) {
		material->set_billboard_mode(p_billboard_y ? BILLBOARD_FIXED_Y : BILLBOARD_ENABLED);
	}

	materials_for_2d[key] = material;

	if (r_shader_rid) {
		*r_shader_rid = materials_for_2d[key]->get_shader_rid();
	}

	return materials_for_2d[key];
}

void StandardMaterial3D::set_on_top_of_alpha()
{
	set_transparency(TRANSPARENCY_DISABLED);
	set_render_priority(RENDER_PRIORITY_MAX);
}

bool StandardMaterial3D::is_proximity_fade_enabled() const { return proximity_fade_enabled; }

float StandardMaterial3D::get_proximity_fade_distance() const { return proximity_fade_distance; }

float StandardMaterial3D::get_msdf_pixel_range() const { return msdf_pixel_range; }

float StandardMaterial3D::get_msdf_outline_size() const { return msdf_outline_size; }

StandardMaterial3D::DistanceFadeMode StandardMaterial3D::get_distance_fade() const { return distance_fade; }

float StandardMaterial3D::get_distance_fade_max_distance() const { return distance_fade_max_distance; }

float StandardMaterial3D::get_distance_fade_min_distance() const { return distance_fade_min_distance; }

void StandardMaterial3D::set_emission_operator(EmissionOperator p_op)
{
	if (emission_op == p_op) {
		return;
	}
	emission_op = p_op;
	_queue_shader_change();
}

StandardMaterial3D::EmissionOperator StandardMaterial3D::get_emission_operator() const
{
	return emission_op;
}

RID StandardMaterial3D::get_rid() const
{
	const_cast<StandardMaterial3D*>(this)->_update_shader();
	const_cast<StandardMaterial3D*>(this)->_check_material_rid();
	return _get_material();
}

StandardMaterial3D::StencilMode StandardMaterial3D::get_stencil_mode() const { return stencil_mode; }

void StandardMaterial3D::set_stencil_flags(int p_stencil_flags)
{
	if (stencil_flags == p_stencil_flags) {
		return;
	}

	// If enabling read while already writing, switch to read only.
	if ((p_stencil_flags & STENCIL_FLAG_READ) &&
		(stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL))) {
		p_stencil_flags = p_stencil_flags & STENCIL_FLAG_READ;
	}

	// If enabling write while already reading, switch to write or write_depth_fail.
	if ((p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL)) &&
		(stencil_flags & STENCIL_FLAG_READ)) {
		p_stencil_flags = p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL);
	}

	// If enabling read+write while already doing neither, only allow read.
	if ((p_stencil_flags & STENCIL_FLAG_READ) &&
		(p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL))) {
		p_stencil_flags = p_stencil_flags & STENCIL_FLAG_READ;
	}

	stencil_flags = p_stencil_flags;
	_queue_shader_change();
}

int StandardMaterial3D::get_stencil_flags() const { return stencil_flags; }

void StandardMaterial3D::set_stencil_compare(StandardMaterial3D::StencilCompare p_op)
{
	if (stencil_compare == p_op) {
		return;
	}

	stencil_compare = p_op;
	_queue_shader_change();
}

StandardMaterial3D::StencilCompare StandardMaterial3D::get_stencil_compare() const
{
	return stencil_compare;
}

void StandardMaterial3D::set_stencil_reference(int p_reference)
{
	if (stencil_reference == p_reference) {
		return;
	}

	stencil_reference = p_reference;
	_queue_shader_change();

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
	if (stencil_next_pass.is_valid()) {
		stencil_next_pass->set_stencil_reference(p_reference);
	}
}

int StandardMaterial3D::get_stencil_reference() const { return stencil_reference; }

void StandardMaterial3D::set_stencil_effect_color(const Color& p_color)
{
	if (stencil_effect_color == p_color) {
		return;
	}

	stencil_effect_color = p_color;

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
}

Color StandardMaterial3D::get_stencil_effect_color() const { return stencil_effect_color; }

void StandardMaterial3D::set_stencil_effect_outline_thickness(float p_outline_thickness)
{
	if (stencil_effect_outline_thickness == p_outline_thickness) {
		return;
	}

	stencil_effect_outline_thickness = p_outline_thickness;

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
	if (stencil_next_pass.is_valid()) {
		stencil_next_pass->set_grow(p_outline_thickness);
	}
}

float StandardMaterial3D::get_stencil_effect_outline_thickness() const
{
	return stencil_effect_outline_thickness;
}

RID StandardMaterial3D::get_shader_rid() const
{
	const_cast<StandardMaterial3D*>(this)->_update_shader();
	return shader_rid;
}

void StandardMaterial3D::set_orm_mode(bool p_orm)
{
	if (orm == p_orm) {
		return;
	}
	orm = p_orm;
	_queue_shader_change();
}

Shader::Mode StandardMaterial3D::get_shader_mode() const { return Shader::MODE_SPATIAL; }

StandardMaterial3D::StandardMaterial3D(bool p_orm) : element(this)
{
	orm = p_orm;
	// Initialize to the same values as the shader
	set_specular(0.5);
	set_roughness(1.0);
	set_metallic(0.0);
	set_emission(Color(0, 0, 0));
	set_emission_energy_multiplier(1.0);
	set_normal_scale(1);
	set_rim(1.0);
	set_rim_tint(0.5);
	set_clearcoat(1);
	set_clearcoat_roughness(0.5);
	set_anisotropy(0);
	set_heightmap_scale(5.0);
	set_subsurface_scattering_strength(0);
	set_backlight(Color(0, 0, 0));
	set_transmittance_color(Color(1, 1, 1, 1));
	set_transmittance_depth(0.1);
	set_transmittance_boost(0.0);
	set_refraction(0.05);
	set_point_size(1);
	set_uv1_offset(Vector3(0, 0, 0));
	set_uv1_scale(Vector3(1, 1, 1));
	set_uv1_triplanar_blend_sharpness(1);
	set_uv2_offset(Vector3(0, 0, 0));
	set_uv2_scale(Vector3(1, 1, 1));
	set_uv2_triplanar_blend_sharpness(1);
	set_billboard_mode(BILLBOARD_DISABLED);
	set_particles_anim_h_frames(1);
	set_particles_anim_v_frames(1);
	set_particles_anim_loop(false);

	set_transparency(TRANSPARENCY_DISABLED);
	set_alpha_antialiasing(ALPHA_ANTIALIASING_OFF);
	// Alpha scissor threshold of 0.5 matches the glTF specification and Label3D default.
	// <https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_material_alphacutoff>
	set_alpha_scissor_threshold(0.5);
	set_alpha_hash_scale(1.0);
	set_alpha_antialiasing_edge(0.3);

	set_proximity_fade_distance(1);
	set_distance_fade_min_distance(0);
	set_distance_fade_max_distance(10);

	set_ao_light_affect(0.0);

	set_metallic_texture_channel(TEXTURE_CHANNEL_RED);
	set_roughness_texture_channel(TEXTURE_CHANNEL_RED);
	set_ao_texture_channel(TEXTURE_CHANNEL_RED);
	set_refraction_texture_channel(TEXTURE_CHANNEL_RED);

	set_grow(0.0);

	set_msdf_pixel_range(4.0);
	set_msdf_outline_size(0.0);

	set_heightmap_deep_parallax_min_layers(8);
	set_heightmap_deep_parallax_max_layers(32);
	set_heightmap_deep_parallax_flip_tangent(false); // also sets binormal

	set_z_clip_scale(1.0);
	set_fov_override(75.0);

	set_stencil_mode(STENCIL_MODE_DISABLED);

	flags[FLAG_ALBEDO_TEXTURE_MSDF] = false;
	flags[FLAG_USE_TEXTURE_REPEAT] = true;

	current_key.invalid_key = 1;
}

StandardMaterial3D::~StandardMaterial3D()
{
	ERR_FAIL_NULL(Renderer::data);

	{
		MutexLock lock(shader_map_mutex);
		if (shader_map.has(current_key)) {
			shader_map[current_key].users--;
			if (shader_map[current_key].users == 0) {
				// Deallocate shader which is no longer in use.
				Renderer::free_rid(shader_map[current_key].shader);
				shader_map.erase(current_key);
			}
		}
	}

	if (_get_material().is_valid()) {
		Renderer::material_set_shader(_get_material(), RID());
	}
}

#ifndef DISABLE_DEPRECATED

bool Material::_can_do_next_pass() const { return false; }

bool Material::_can_use_render_priority() const { return false; }

RID Material::get_shader_rid() const { return RID(); }

#endif // DISABLE_DEPRECATED

void ShaderMaterial::set_shader(const Ref<Shader>& p_shader) {}

void StandardMaterial3D::set_texture(TextureParam p_param, const Ref<Texture2D>& p_texture) {}

void StandardMaterial3D::set_roughness(float) {}

void StandardMaterial3D::set_metallic(float) {}

void StandardMaterial3D::set_emission(Color const&) {}

void StandardMaterial3D::set_billboard_mode(StandardMaterial3D::BillboardMode) {}

Ref<StandardMaterial3D> StandardMaterial3D::_get_stencil_next_pass() const
{
	Ref<StandardMaterial3D> bm = memnew(StandardMaterial3D);
	return bm;
}

void StandardMaterial3D::set_specular(float) {}

void StandardMaterial3D::set_metallic_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_grow(float) {}

void StandardMaterial3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing) {}

void ShaderMaterial::_check_material_rid() const {}

void StandardMaterial3D::set_z_clip_scale(float) {}

void StandardMaterial3D::set_uv2_triplanar_blend_sharpness(float) {}

void StandardMaterial3D::set_uv2_scale(Vector3 const&) {}

void StandardMaterial3D::set_uv2_offset(Vector3 const&) {}


void StandardMaterial3D::set_uv1_triplanar_blend_sharpness(float) {}

void StandardMaterial3D::set_uv1_scale(Vector3 const&) {}

void StandardMaterial3D::set_uv1_offset(Vector3 const&) {}

void StandardMaterial3D::set_transmittance_depth(float) {}

void StandardMaterial3D::set_transmittance_color(Color const&) {}

void StandardMaterial3D::set_transmittance_boost(float) {}

void StandardMaterial3D::set_subsurface_scattering_strength(float) {}

void StandardMaterial3D::set_stencil_mode(StandardMaterial3D::StencilMode) {}

void StandardMaterial3D::set_rim_tint(float) {}

void StandardMaterial3D::set_rim(float) {}

void StandardMaterial3D::set_refraction_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_refraction(float) {}

void StandardMaterial3D::set_proximity_fade_distance(float) {}

void StandardMaterial3D::set_particles_anim_v_frames(int) {}

void StandardMaterial3D::set_particles_anim_loop(bool) {}

void StandardMaterial3D::set_particles_anim_h_frames(int) {}

void StandardMaterial3D::set_normal_scale(float) {}

void StandardMaterial3D::set_msdf_pixel_range(float) {}

void StandardMaterial3D::set_msdf_outline_size(float) {}

void StandardMaterial3D::set_heightmap_scale(float) {}

void StandardMaterial3D::set_heightmap_deep_parallax_min_layers(int) {}

void StandardMaterial3D::set_heightmap_deep_parallax_max_layers(int) {}

void StandardMaterial3D::set_heightmap_deep_parallax_flip_tangent(bool) {}

void StandardMaterial3D::set_fov_override(float) {}

void StandardMaterial3D::set_emission_energy_multiplier(float) {}

void StandardMaterial3D::set_distance_fade_min_distance(float) {}

void StandardMaterial3D::set_distance_fade_max_distance(float) {}

void StandardMaterial3D::set_clearcoat_roughness(float) {}

void StandardMaterial3D::set_clearcoat(float) {}

void StandardMaterial3D::set_backlight(Color const&) {}

void StandardMaterial3D::set_ao_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_ao_light_affect(float) {}

void StandardMaterial3D::set_anisotropy(float) {}

void StandardMaterial3D::set_alpha_scissor_threshold(float) {}

void StandardMaterial3D::set_alpha_hash_scale(float) {}

void StandardMaterial3D::set_alpha_antialiasing_edge(float) {}

void StandardMaterial3D::_check_material_rid() {}

// --- canvas_item_material.cpp ---

#include "scene/resources/material.h"
#include "core/version.h"
#include "servers/rendering/renderer.h"

Mutex CanvasItemMaterial::material_mutex;
SelfList<CanvasItemMaterial>::List CanvasItemMaterial::dirty_materials;
HashMap<CanvasItemMaterial::MaterialKey, CanvasItemMaterial::ShaderData,
	CanvasItemMaterial::MaterialKey>
	CanvasItemMaterial::shader_map;
CanvasItemMaterial::ShaderNames* CanvasItemMaterial::shader_names = nullptr;
Mutex CanvasItemMaterial::blit_shader_mutex;
RID CanvasItemMaterial::blit_shader_cache[6];

void CanvasItemMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->particles_anim_h_frames = "particles_anim_h_frames";
	shader_names->particles_anim_v_frames = "particles_anim_v_frames";
	shader_names->particles_anim_loop = "particles_anim_loop";
}

void CanvasItemMaterial::cleanup_blit_shaders()
{
	for (int i = 0; i < 6; i++) {
		if (blit_shader_cache[i].is_valid()) {
			Renderer::free_rid(blit_shader_cache[i]);
			blit_shader_cache[i] = RID();
		}
	}
}

void CanvasItemMaterial::finish_shaders()
{
	cleanup_blit_shaders();

	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void CanvasItemMaterial::_update_blit_shader(BlendMode p_blend)
{
	ERR_FAIL_INDEX(int(p_blend), 6);
	MutexLock shader_lock(blit_shader_mutex);
	int index = int(p_blend);
	if (blit_shader_cache[index].is_null()) {
		blit_shader_cache[index] = Renderer::shader_create();
		String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
					  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial (TextureBlit).\n\n";

		code += "shader_type texture_blit;\nrender_mode ";
		switch (p_blend) {
		case BLEND_MODE_MIX:
			code += "blend_mix";
			break;
		case BLEND_MODE_ADD:
			code += "blend_add";
			break;
		case BLEND_MODE_SUB:
			code += "blend_sub";
			break;
		case BLEND_MODE_MUL:
			code += "blend_mul";
			break;
		case BLEND_MODE_PREMULT_ALPHA:
			// texture_blit shaders support mix, add, sub, mul, disabled.
			code += "blend_mix";
			break;
		case BLEND_MODE_DISABLED:
			code += "blend_disabled";
			break;
		default:
			code += "blend_mix";
			break;
		}
		code += ";\n\n";

		code += "uniform sampler2D source_texture0 : hint_blit_source0;\n";
		code += "uniform sampler2D source_texture1 : hint_blit_source1;\n";
		code += "uniform sampler2D source_texture2 : hint_blit_source2;\n";
		code += "uniform sampler2D source_texture3 : hint_blit_source3;\n\n";

		code += "void blit() {\n";
		code += "	// Copies from each whole source texture to a rect on each output texture.\n";
		code += "	COLOR0 = texture(source_texture0, UV) * MODULATE;\n";
		code += "	COLOR1 = texture(source_texture1, UV) * MODULATE;\n";
		code += "	COLOR2 = texture(source_texture2, UV) * MODULATE;\n";
		code += "	COLOR3 = texture(source_texture3, UV) * MODULATE;\n}";
		Renderer::shader_set_code(blit_shader_cache[index], code);
	}
}

void CanvasItemMaterial::_update_shader()
{
	if (texture_blit) {
		return;
	}

	MaterialKey mk = _compute_key();
	if (mk.key == current_key.key) {
		return; // no update required in the end
	}

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}
	}

	current_key = mk;

	if (shader_map.has(mk)) {
		Renderer::material_set_shader(_get_material(), shader_map[mk].shader);
		shader_map[mk].users++;
		return;
	}

	// must create a shader!

	// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
	String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
				  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial.\n\n";

	code += "shader_type canvas_item;\nrender_mode ";
	switch (blend_mode) {
	case BLEND_MODE_MIX:
		code += "blend_mix";
		break;
	case BLEND_MODE_ADD:
		code += "blend_add";
		break;
	case BLEND_MODE_SUB:
		code += "blend_sub";
		break;
	case BLEND_MODE_MUL:
		code += "blend_mul";
		break;
	case BLEND_MODE_PREMULT_ALPHA:
		code += "blend_premul_alpha";
		break;
	case BLEND_MODE_DISABLED:
		code += "blend_disabled";
		break;
	}

	switch (light_mode) {
	case LIGHT_MODE_NORMAL:
		break;
	case LIGHT_MODE_UNSHADED:
		code += ",unshaded";
		break;
	case LIGHT_MODE_LIGHT_ONLY:
		code += ",light_only";
		break;
	}

	code += ";\n";

	if (particles_animation) {
		code += "uniform int particles_anim_h_frames;\n";
		code += "uniform int particles_anim_v_frames;\n";
		code += "uniform bool particles_anim_loop;\n\n";

		code += "void vertex() {\n";
		code += "	float h_frames = float(particles_anim_h_frames);\n";
		code += "	float v_frames = float(particles_anim_v_frames);\n";
		code += "	VERTEX.xy /= vec2(h_frames, v_frames);\n";
		code += "	float particle_total_frames = float(particles_anim_h_frames * "
				"particles_anim_v_frames);\n";
		code +=
			"	float particle_frame = floor(INSTANCE_CUSTOM.z * float(particle_total_frames));\n";
		code += "	if (!particles_anim_loop) {\n";
		code +=
			"		particle_frame = clamp(particle_frame, 0.0, particle_total_frames - 1.0);\n";
		code += "	} else {\n";
		code += "		particle_frame = mod(particle_frame, particle_total_frames);\n";
		code += "	}";
		code += "	UV /= vec2(h_frames, v_frames);\n";
		code += "	UV += vec2(mod(particle_frame, h_frames) / h_frames, floor((particle_frame + "
				"0.5) / h_frames) / v_frames);\n";
		code += "}\n";
	}

	ShaderData shader_data;
	shader_data.shader = Renderer::shader_create();
	shader_data.users = 1;

	Renderer::shader_set_code(shader_data.shader, code);

	shader_map[mk] = shader_data;

	Renderer::material_set_shader(_get_material(), shader_data.shader);
}

void CanvasItemMaterial::flush_changes()
{
	MutexLock lock(material_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void CanvasItemMaterial::_queue_shader_change()
{
	if (texture_blit || !_is_initialized()) {
		return;
	}

	MutexLock lock(material_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

void CanvasItemMaterial::set_texture_blit(bool p_enable)
{
	if (texture_blit == p_enable) {
		return;
	}

	if (!texture_blit) {
		MutexLock lock(material_mutex);
		if (element.in_list()) {
			element.remove_from_list();
		}
		if (shader_map.has(current_key)) {
			shader_map[current_key].users--;
			if (shader_map[current_key].users == 0) {
				Renderer::free_rid(shader_map[current_key].shader);
				shader_map.erase(current_key);
			}
		}
		current_key.key = 0;
		current_key.invalid_key = 1;
	}

	texture_blit = p_enable;
	blit_shader_set = false;

	if (texture_blit) {
		_update_blit_shader(blend_mode);
		Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		blit_shader_set = true;
	}
	else {
		Renderer::material_set_shader(_get_material(), RID());
		_queue_shader_change();
	}
}

void CanvasItemMaterial::set_blend_mode(BlendMode p_blend_mode)
{
	ERR_FAIL_INDEX(int(p_blend_mode), 6);
	blend_mode = p_blend_mode;
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		}
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::BlendMode CanvasItemMaterial::get_blend_mode() const { return blend_mode; }

void CanvasItemMaterial::set_light_mode(LightMode p_light_mode)
{
	light_mode = p_light_mode;
	_queue_shader_change();
}

CanvasItemMaterial::LightMode CanvasItemMaterial::get_light_mode() const { return light_mode; }

void CanvasItemMaterial::set_particles_animation(bool p_particles_anim)
{
	particles_animation = p_particles_anim;
	_queue_shader_change();
}

bool CanvasItemMaterial::get_particles_animation() const { return particles_animation; }

void CanvasItemMaterial::set_particles_anim_h_frames(int p_frames)
{
	particles_anim_h_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_h_frames() const { return particles_anim_h_frames; }

void CanvasItemMaterial::set_particles_anim_v_frames(int p_frames)
{
	particles_anim_v_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_v_frames() const { return particles_anim_v_frames; }

void CanvasItemMaterial::set_particles_anim_loop(bool p_loop) { particles_anim_loop = p_loop; }

bool CanvasItemMaterial::get_particles_anim_loop() const { return particles_anim_loop; }

RID CanvasItemMaterial::get_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (!blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
			blit_shader_set = true;
		}
	}
	return _get_material();
}

RID CanvasItemMaterial::get_shader_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		return blit_shader_cache[int(blend_mode)];
	}
	ERR_FAIL_COND_V(!shader_map.has(current_key), RID());
	return shader_map[current_key].shader;
}

Shader::Mode CanvasItemMaterial::get_shader_mode() const
{
	return texture_blit ? Shader::MODE_TEXTURE_BLIT : Shader::MODE_CANVAS_ITEM;
}

CanvasItemMaterial::CanvasItemMaterial(bool p_texture_blit)
	: element(this), texture_blit(p_texture_blit)
{
	_set_material(Renderer::material_create());
	current_key.invalid_key = 1;
	set_particles_anim_h_frames(1);
	set_particles_anim_v_frames(1);
	set_particles_anim_loop(false);
	_mark_ready();
	if (texture_blit) {
		_update_blit_shader(blend_mode);
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::~CanvasItemMaterial()
{
	MutexLock lock(material_mutex);

	ERR_FAIL_NULL(Renderer::data);

	if (!texture_blit && shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		Renderer::material_set_shader(_get_material(), RID());
	}
}

// --- particle_process_material.cpp ---

#include "core/config/engine.h"
#include "core/version.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

Mutex ParticleProcessMaterial::dirty_materials_mutex;
SelfList<ParticleProcessMaterial>::List ParticleProcessMaterial::dirty_materials;
Mutex ParticleProcessMaterial::shader_map_mutex;
HashMap<ParticleProcessMaterial::MaterialKey, ParticleProcessMaterial::ShaderData,
	ParticleProcessMaterial::MaterialKey>
	ParticleProcessMaterial::shader_map;
RBSet<String> ParticleProcessMaterial::min_max_properties;
ParticleProcessMaterial::ShaderNames* ParticleProcessMaterial::shader_names = nullptr;

void ParticleProcessMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->direction = "direction";
	shader_names->spread = "spread";
	shader_names->flatness = "flatness";
	shader_names->initial_linear_velocity_min = "initial_linear_velocity_min";
	shader_names->initial_angle_min = "initial_angle_min";
	shader_names->angular_velocity_min = "angular_velocity_min";
	shader_names->orbit_velocity_min = "orbit_velocity_min";
	shader_names->radial_velocity_min = "radial_velocity_min";
	shader_names->linear_accel_min = "linear_accel_min";
	shader_names->radial_accel_min = "radial_accel_min";
	shader_names->tangent_accel_min = "tangent_accel_min";
	shader_names->damping_min = "damping_min";
	shader_names->scale_min = "scale_min";
	shader_names->hue_variation_min = "hue_variation_min";
	shader_names->anim_speed_min = "anim_speed_min";
	shader_names->anim_offset_min = "anim_offset_min";
	shader_names->directional_velocity_min = "directional_velocity_min";
	shader_names->scale_over_velocity_min = "scale_over_velocity_min";
	shader_names->scale_3d_min = "scale_3d_min";
	shader_names->rotation_3d_min = "rotation_3d_min";

	shader_names->initial_linear_velocity_max = "initial_linear_velocity_max";
	shader_names->initial_angle_max = "initial_angle_max";
	shader_names->angular_velocity_max = "angular_velocity_max";
	shader_names->orbit_velocity_max = "orbit_velocity_max";
	shader_names->radial_velocity_max = "radial_velocity_max";
	shader_names->linear_accel_max = "linear_accel_max";
	shader_names->radial_accel_max = "radial_accel_max";
	shader_names->tangent_accel_max = "tangent_accel_max";
	shader_names->damping_max = "damping_max";
	shader_names->scale_max = "scale_max";
	shader_names->hue_variation_max = "hue_variation_max";
	shader_names->anim_speed_max = "anim_speed_max";
	shader_names->anim_offset_max = "anim_offset_max";
	shader_names->directional_velocity_max = "directional_velocity_max";
	shader_names->scale_over_velocity_max = "scale_over_velocity_max";
	shader_names->scale_3d_max = "scale_3d_max";
	shader_names->rotation_3d_max = "rotation_3d_max";

	shader_names->angle_texture = "angle_texture";
	shader_names->angular_velocity_texture = "angular_velocity_texture";
	shader_names->orbit_velocity_texture = "orbit_velocity_curve";
	shader_names->radial_velocity_texture = "radial_velocity_curve";
	shader_names->linear_accel_texture = "linear_accel_texture";
	shader_names->radial_accel_texture = "radial_accel_texture";
	shader_names->tangent_accel_texture = "tangent_accel_texture";
	shader_names->damping_texture = "damping_texture";
	shader_names->scale_texture = "scale_curve";
	shader_names->hue_variation_texture = "hue_rot_curve";
	shader_names->anim_speed_texture = "animation_speed_curve";
	shader_names->anim_offset_texture = "animation_offset_curve";
	shader_names->directional_velocity_texture = "directional_velocity_curve";
	shader_names->scale_over_velocity_texture = "scale_over_velocity_curve";

	shader_names->color = "color_value";
	shader_names->color_ramp = "color_ramp";
	shader_names->alpha_ramp = "alpha_curve";
	shader_names->emission_ramp = "emission_curve";
	shader_names->color_initial_ramp = "color_initial_ramp";
	shader_names->velocity_limit_curve = "velocity_limit_curve";
	shader_names->inherit_emitter_velocity_ratio = "inherit_emitter_velocity_ratio";
	shader_names->velocity_pivot = "velocity_pivot";
	shader_names->rotation_velocity_3d_max = "rotation_velocity_3d_max";
	shader_names->rotation_velocity_3d_min = "rotation_velocity_3d_min";
	shader_names->rotation_velocity_3d_curve = "rotation_velocity_3d_curve";

	shader_names->emission_sphere_radius = "emission_sphere_radius";
	shader_names->emission_box_extents = "emission_box_extents";
	shader_names->emission_texture_point_count = "emission_texture_point_count";
	shader_names->emission_texture_points = "emission_texture_points";
	shader_names->emission_texture_normal = "emission_texture_normal";
	shader_names->emission_texture_color = "emission_texture_color";
	shader_names->emission_ring_axis = "emission_ring_axis";
	shader_names->emission_ring_height = "emission_ring_height";
	shader_names->emission_ring_radius = "emission_ring_radius";
	shader_names->emission_ring_inner_radius = "emission_ring_inner_radius";
	shader_names->emission_ring_cone_angle = "emission_ring_cone_angle";
	shader_names->emission_shape_offset = "emission_shape_offset";
	shader_names->emission_shape_scale = "emission_shape_scale";

	shader_names->turbulence_enabled = "turbulence_enabled";
	shader_names->turbulence_noise_strength = "turbulence_noise_strength";
	shader_names->turbulence_noise_scale = "turbulence_noise_scale";
	shader_names->turbulence_noise_speed = "turbulence_noise_speed";
	shader_names->turbulence_noise_speed_random = "turbulence_noise_speed_random";
	shader_names->turbulence_influence_over_life = "turbulence_influence_over_life";
	shader_names->turbulence_influence_min = "turbulence_influence_min";
	shader_names->turbulence_influence_max = "turbulence_influence_max";
	shader_names->turbulence_initial_displacement_min = "turbulence_initial_displacement_min";
	shader_names->turbulence_initial_displacement_max = "turbulence_initial_displacement_max";

	shader_names->gravity = "gravity";

	shader_names->lifetime_randomness = "lifetime_randomness";

	shader_names->sub_emitter_frequency = "sub_emitter_frequency";
	shader_names->sub_emitter_amount_at_end = "sub_emitter_amount_at_end";
	shader_names->sub_emitter_amount_at_collision = "sub_emitter_amount_at_collision";
	shader_names->sub_emitter_amount_at_start = "sub_emitter_amount_at_start";
	shader_names->sub_emitter_keep_velocity = "sub_emitter_keep_velocity";

	shader_names->collision_friction = "collision_friction";
	shader_names->collision_bounce = "collision_bounce";
}

void ParticleProcessMaterial::finish_shaders()
{
	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void ParticleProcessMaterial::flush_changes()
{
	MutexLock lock(dirty_materials_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void ParticleProcessMaterial::_queue_shader_change()
{
	if (!_is_initialized()) {
		return;
	}

	MutexLock lock(dirty_materials_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

bool ParticleProcessMaterial::has_min_max_property(const String& p_name)
{
	return min_max_properties.has(p_name);
}

Vector3 ParticleProcessMaterial::get_direction() const { return direction; }

float ParticleProcessMaterial::get_spread() const { return spread; }

float ParticleProcessMaterial::get_flatness() const { return flatness; }

Vector3 ParticleProcessMaterial::get_velocity_pivot() { return velocity_pivot; }

void ParticleProcessMaterial::set_param(Parameter p_param, const Vector2& p_value)
{
	set_param_min(p_param, p_value.x);
	set_param_max(p_param, p_value.y);
}

Vector2 ParticleProcessMaterial::get_param(Parameter p_param) const
{
	return Vector2(get_param_min(p_param), get_param_max(p_param));
}

float ParticleProcessMaterial::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_min[p_param];
}

float ParticleProcessMaterial::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_max[p_param];
}

static void _adjust_curve_range(const Ref<Texture2D>& p_texture, float p_min, float p_max)
{
	Ref<CurveTexture> curve_tex = p_texture;
	if (curve_tex.is_valid()) {
		curve_tex->ensure_default_setup(p_min, p_max);
	}
}

Ref<Texture2D> ParticleProcessMaterial::get_param_texture(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Texture2D>());

	return tex_parameters[p_param];
}

Color ParticleProcessMaterial::get_color() const { return color; }

Ref<Texture2D> ParticleProcessMaterial::get_color_ramp() const { return color_ramp; }

Ref<Texture2D> ParticleProcessMaterial::get_color_initial_ramp() const
{
	return color_initial_ramp;
}

Ref<Texture2D> ParticleProcessMaterial::get_alpha_curve() const { return alpha_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_curve() const { return emission_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_velocity_limit_curve() const
{
	return velocity_limit_curve;
}

bool ParticleProcessMaterial::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

ParticleProcessMaterial::EmissionShape ParticleProcessMaterial::get_emission_shape() const
{
	return emission_shape;
}

real_t ParticleProcessMaterial::get_emission_sphere_radius() const
{
	return emission_sphere_radius;
}

Vector3 ParticleProcessMaterial::get_emission_box_extents() const { return emission_box_extents; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_point_texture() const
{
	return emission_point_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_normal_texture() const
{
	return emission_normal_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_color_texture() const
{
	return emission_color_texture;
}

int ParticleProcessMaterial::get_emission_point_count() const { return emission_point_count; }

Vector3 ParticleProcessMaterial::get_emission_ring_axis() const { return emission_ring_axis; }

real_t ParticleProcessMaterial::get_emission_ring_height() const { return emission_ring_height; }

real_t ParticleProcessMaterial::get_emission_ring_radius() const { return emission_ring_radius; }

real_t ParticleProcessMaterial::get_emission_ring_inner_radius() const
{
	return emission_ring_inner_radius;
}

real_t ParticleProcessMaterial::get_emission_ring_cone_angle() const
{
	return emission_ring_cone_angle;
}

Vector3 ParticleProcessMaterial::get_emission_shape_offset() const { return emission_shape_offset; }

Vector3 ParticleProcessMaterial::get_emission_shape_scale() const { return emission_shape_scale; }

double ParticleProcessMaterial::get_inherit_velocity_ratio()
{
	return inherit_emitter_velocity_ratio;
}

bool ParticleProcessMaterial::is_using_scale_3d() const { return use_scale_3d; }

Vector3 ParticleProcessMaterial::get_scale_3d_min() const { return scale_3d_min; }

Vector3 ParticleProcessMaterial::get_scale_3d_max() const { return scale_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_3d() const { return use_rotation_3d; }

Vector3 ParticleProcessMaterial::get_rotation_3d_min() const { return rotation_3d_min; }

Vector3 ParticleProcessMaterial::get_rotation_3d_max() const { return rotation_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_velocity_3d() const
{
	return using_rotation_velocity_3d;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_min() const
{
	return rotation_velocity_3d_min;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_max() const
{
	return rotation_velocity_3d_max;
}

Ref<Texture2D> ParticleProcessMaterial::get_rotation_velocity_3d_curve() const
{
	return rotation_velocity_3d_curve;
}

bool ParticleProcessMaterial::get_turbulence_enabled() const { return turbulence_enabled; }

float ParticleProcessMaterial::get_turbulence_noise_strength() const
{
	return turbulence_noise_strength;
}

float ParticleProcessMaterial::get_turbulence_noise_scale() const { return turbulence_noise_scale; }

float ParticleProcessMaterial::get_turbulence_noise_speed_random() const
{
	return turbulence_noise_speed_random;
}

Vector3 ParticleProcessMaterial::get_turbulence_noise_speed() const
{
	return turbulence_noise_speed;
}

Vector3 ParticleProcessMaterial::get_gravity() const { return gravity; }

double ParticleProcessMaterial::get_lifetime_randomness() const { return lifetime_randomness; }

RID ParticleProcessMaterial::get_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return Material::get_rid();
}

RID ParticleProcessMaterial::get_shader_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return shader_rid;
}

ParticleProcessMaterial::SubEmitterMode ParticleProcessMaterial::get_sub_emitter_mode() const
{
	return sub_emitter_mode;
}

double ParticleProcessMaterial::get_sub_emitter_frequency() const { return sub_emitter_frequency; }

int ParticleProcessMaterial::get_sub_emitter_amount_at_end() const
{
	return sub_emitter_amount_at_end;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_collision() const
{
	return sub_emitter_amount_at_collision;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_start() const
{
	return sub_emitter_amount_at_start;
}

bool ParticleProcessMaterial::get_sub_emitter_keep_velocity() const
{
	return sub_emitter_keep_velocity;
}

void ParticleProcessMaterial::set_attractor_interaction_enabled(bool p_enable)
{
	attractor_interaction_enabled = p_enable;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_attractor_interaction_enabled() const
{
	return attractor_interaction_enabled;
}

ParticleProcessMaterial::CollisionMode ParticleProcessMaterial::get_collision_mode() const
{
	return collision_mode;
}

void ParticleProcessMaterial::set_collision_use_scale(bool p_scale)
{
	collision_scale = p_scale;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_collision_using_scale() const { return collision_scale; }

float ParticleProcessMaterial::get_collision_friction() const { return collision_friction; }

float ParticleProcessMaterial::get_collision_bounce() const { return collision_bounce; }

Shader::Mode ParticleProcessMaterial::get_shader_mode() const { return Shader::MODE_PARTICLES; }

ParticleProcessMaterial::ParticleProcessMaterial() : element(this)
{
	_set_material(Renderer::material_create());

	set_direction(Vector3(1, 0, 0));
	set_spread(45);
	set_flatness(0);
	set_param_min(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_min(PARAM_ANGULAR_VELOCITY, 0);
	set_param_min(PARAM_ORBIT_VELOCITY, 0);
	set_param_min(PARAM_LINEAR_ACCEL, 0);
	set_param_min(PARAM_RADIAL_ACCEL, 0);
	set_param_min(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_min(PARAM_DAMPING, 0);
	set_param_min(PARAM_ANGLE, 0);
	set_param_min(PARAM_SCALE, 1);
	set_param_min(PARAM_HUE_VARIATION, 0);
	set_param_min(PARAM_ANIM_SPEED, 0);
	set_param_min(PARAM_ANIM_OFFSET, 0);
	set_param_max(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_max(PARAM_ANGULAR_VELOCITY, 0);
	set_param_max(PARAM_ORBIT_VELOCITY, 0);
	set_param_max(PARAM_LINEAR_ACCEL, 0);
	set_param_max(PARAM_RADIAL_ACCEL, 0);
	set_param_max(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_max(PARAM_DAMPING, 0);
	set_param_max(PARAM_ANGLE, 0);
	set_param_max(PARAM_SCALE, 1);
	set_param_max(PARAM_HUE_VARIATION, 0);
	set_param_max(PARAM_ANIM_SPEED, 0);
	set_param_max(PARAM_ANIM_OFFSET, 0);
	set_param_min(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_param_max(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_use_scale_3d(false);
	set_scale_3d_min(Vector3(1.0, 1.0, 1.0));
	set_scale_3d_max(Vector3(1.0, 1.0, 1.0));
	set_use_rotation_3d(false);
	set_rotation_3d_min(Vector3(0.0, 0.0, 0.0));
	set_rotation_3d_max(Vector3(0.0, 0.0, 0.0));
	set_emission_shape(EMISSION_SHAPE_POINT);
	set_emission_sphere_radius(1);
	set_emission_box_extents(Vector3(1, 1, 1));
	set_emission_ring_axis(Vector3(0, 0, 1.0));
	set_emission_ring_height(1);
	set_emission_ring_radius(1);
	set_emission_ring_inner_radius(0);
	set_emission_ring_cone_angle(90);
	set_emission_shape_offset(Vector3(0.0, 0.0, 0.0));
	set_emission_shape_scale(Vector3(1.0, 1.0, 1.0));

	set_turbulence_enabled(false);
	set_turbulence_noise_speed(Vector3(0.0, 0.0, 0.0));
	set_turbulence_noise_strength(1);
	set_turbulence_noise_scale(9);
	set_turbulence_noise_speed_random(0.2);
	set_param_min(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_max(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_min(PARAM_TURB_INIT_DISPLACEMENT, 0.0);
	set_param_max(PARAM_TURB_INIT_DISPLACEMENT, 0.0);

	set_gravity(Vector3(0, -9.8, 0));
	set_lifetime_randomness(0);

	set_sub_emitter_mode(SUB_EMITTER_DISABLED);
	set_sub_emitter_frequency(4);
	set_sub_emitter_amount_at_end(1);
	set_sub_emitter_amount_at_collision(1);
	set_sub_emitter_amount_at_start(1);
	set_sub_emitter_keep_velocity(false);

	set_attractor_interaction_enabled(true);
	set_collision_mode(COLLISION_DISABLED);
	set_collision_bounce(0.0);
	set_collision_friction(0.0);
	set_collision_use_scale(false);

	for (int i = 0; i < PARTICLE_FLAG_MAX; i++) {
		particle_flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));

	current_key.invalid_key = 1;
}

ParticleProcessMaterial::~ParticleProcessMaterial()
{
	ERR_FAIL_NULL(Renderer::data);
	MutexLock lock(shader_map_mutex);

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		Renderer::material_set_shader(_get_material(), RID());
	}
}

void ParticleProcessMaterial::set_param_min(Parameter p_param, float p_value) {}

void ParticleProcessMaterial::set_param_max(Parameter p_param, float p_value) {}



void ParticleProcessMaterial::_update_shader() {}

void ParticleProcessMaterial::set_use_scale_3d(bool) {}

void ParticleProcessMaterial::set_use_rotation_3d(bool) {}

void ParticleProcessMaterial::set_turbulence_noise_strength(float) {}

void ParticleProcessMaterial::set_turbulence_noise_speed(Vector3 const&) {}

void ParticleProcessMaterial::set_turbulence_noise_speed_random(float) {}

void ParticleProcessMaterial::set_turbulence_noise_scale(float) {}

void ParticleProcessMaterial::set_turbulence_enabled(bool) {}

void ParticleProcessMaterial::set_sub_emitter_mode(ParticleProcessMaterial::SubEmitterMode) {}

void ParticleProcessMaterial::set_sub_emitter_keep_velocity(bool) {}

void ParticleProcessMaterial::set_sub_emitter_frequency(double) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_start(int) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_end(int) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_collision(int) {}

void ParticleProcessMaterial::set_spread(float) {}

void ParticleProcessMaterial::set_scale_3d_min(Vector3 const&) {}

void ParticleProcessMaterial::set_scale_3d_max(Vector3 const&) {}

void ParticleProcessMaterial::set_rotation_3d_min(Vector3 const&) {}

void ParticleProcessMaterial::set_rotation_3d_max(Vector3 const&) {}

void ParticleProcessMaterial::set_lifetime_randomness(double) {}

void ParticleProcessMaterial::set_gravity(Vector3 const&) {}

void ParticleProcessMaterial::set_flatness(float) {}

void ParticleProcessMaterial::set_emission_sphere_radius(float) {}

void ParticleProcessMaterial::set_emission_shape_scale(Vector3 const&) {}

void ParticleProcessMaterial::set_emission_shape(ParticleProcessMaterial::EmissionShape) {}

void ParticleProcessMaterial::set_emission_shape_offset(Vector3 const&) {}

void ParticleProcessMaterial::set_emission_ring_radius(float) {}

void ParticleProcessMaterial::set_emission_ring_inner_radius(float) {}

void ParticleProcessMaterial::set_emission_ring_height(float) {}

void ParticleProcessMaterial::set_emission_ring_cone_angle(float) {}

void ParticleProcessMaterial::set_emission_ring_axis(Vector3) {}

void ParticleProcessMaterial::set_emission_box_extents(Vector3) {}

void ParticleProcessMaterial::set_direction(Vector3) {}

void ParticleProcessMaterial::set_color(Color const&) {}

void ParticleProcessMaterial::set_collision_mode(ParticleProcessMaterial::CollisionMode) {}

void ParticleProcessMaterial::set_collision_friction(float) {}

void ParticleProcessMaterial::set_collision_bounce(float) {}

// --- physics_material.cpp ---

#include "scene/resources/material.h"

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)



void PhysicsMaterial::set_friction(real_t p_val)
{
	friction = p_val;
	emit_changed();
}

void PhysicsMaterial::set_rough(bool p_val)
{
	rough = p_val;
	emit_changed();
}

void PhysicsMaterial::set_bounce(real_t p_val)
{
	bounce = p_val;
	emit_changed();
}

void PhysicsMaterial::set_absorbent(bool p_val)
{
	absorbent = p_val;
	emit_changed();
}
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)

// ============================================================================
// Merged from scene/resources/shader.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_language.h"
#include "scene/resources/shader.h"

#ifdef TOOLS_ENABLED
#include "modules/modules_enabled.gen.h" // For regex.
#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif
#endif

Shader::Mode Shader::get_mode() const { return mode; }

void Shader::_check_shader_rid() const
{
	MutexLock lock(shader_rid_mutex);
	if (shader_rid.is_null() && !preprocessed_code.is_empty()) {
		shader_rid = Renderer::shader_create_from_code(
			preprocessed_code, get_path());
		preprocessed_code = String();
	}
}

void Shader::_dependency_changed()
{
	// Preprocess and compile the code again because a dependency has changed. It also calls
	// emit_changed() for us.
	_recompile();
}

void Shader::_recompile() { set_code(get_code()); }

void Shader::set_path(const String& p_path, bool p_take_over)
{
	Resource::set_path(p_path, p_take_over);

	if (shader_rid.is_valid()) {
		Renderer::shader_set_path_hint(shader_rid, p_path);
	}
}

void Shader::set_include_path(const String& p_path)
{
	// Used only if the shader does not have a resource path set,
	// for example during loading stage or when created by code.
	include_path = p_path;
}


void Shader::set_code(const String& p_code)
{
	code = p_code;
}

String Shader::get_code() const
{
	_update_shader();
	return code;
}

RID Shader::get_rid() const
{
	_update_shader();
	_check_shader_rid();

	return shader_rid;
}

void Shader::set_default_texture_parameter(
	const String& p_name, const Ref<Texture>& p_texture, int p_index)
{
	_check_shader_rid();

	if (p_texture.is_valid()) {
		if (!default_textures.has(p_name)) {
			default_textures[p_name] = HashMap<int, Ref<Texture>>();
		}
		default_textures[p_name][p_index] = p_texture;
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, p_texture->get_rid(), p_index);
	}
	else {
		if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
			default_textures[p_name].erase(p_index);

			if (default_textures[p_name].is_empty()) {
				default_textures.erase(p_name);
			}
		}
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, RID(), p_index);
	}

	emit_changed();
}

Ref<Texture> Shader::get_default_texture_parameter(const String& p_name, int p_index) const
{
	if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
		return default_textures[p_name][p_index];
	}
	return Ref<Texture>();
}

void Shader::get_default_texture_parameter_list(List<String>* r_textures) const
{
	for (const KeyValue<String, HashMap<int, Ref<Texture>>>& E : default_textures) {
		r_textures->push_back(E.key);
	}
}

bool Shader::is_text_shader() const { return true; }

void Shader::_update_shader() const
{
	// Base implementation does nothing.
}

Shader::Shader()
{
	// Shader RID will be empty until it is required.
}

Shader::~Shader()
{
	if (shader_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(shader_rid);
	}
}

// --- shader.compat.inc ---

#ifndef DISABLE_DEPRECATED
void Shader::_set_default_texture_parameter_bind_compat_95126(
	const String& p_name, const Ref<Texture2D>& p_texture, int p_index)
{
	set_default_texture_parameter(p_name, p_texture, p_index);
}

Ref<Texture2D> Shader::_get_default_texture_parameter_bind_compat_95126(
	const String& p_name, int p_index) const
{
	return get_default_texture_parameter(p_name, p_index);
}

void Shader::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- shader_include.cpp ---

#include "servers/rendering/shader_language.h"
#include "scene/resources/shader.h"

void ShaderInclude::_dependency_changed() { emit_changed(); }

String ShaderInclude::get_code() const { return code; }

void ShaderInclude::set_include_path(const String& p_path) { include_path = p_path; }

void set_code(const String& p_text) {}

void ShaderInclude::set_code(const String& p_code) {}

// --- shader_resource_format.cpp ---

#include "scene/resources/shader.h"

#include "core/io/file_access.h"
#include "scene/resources/shader.h"

Ref<Resource> ResourceFormatLoaderShader::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	(void)p_original_path;
	(void)p_use_sub_threads;
	(void)r_progress;
	(void)p_cache_mode;

	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	const bool is_include = p_path.has_extension("gdshaderinc");

	Error error = OK;
	Vector<uint8_t> buffer = FileAccess::get_file_as_bytes(p_path, &error);
	ERR_FAIL_COND_V_MSG(error, nullptr,
		(is_include ? "Cannot load shader include: " : "Cannot load shader: ") + p_path);

	String str;
	if (buffer.size() > 0) {
		error = str.append_utf8((const char*)buffer.ptr(), buffer.size());
		ERR_FAIL_COND_V_MSG(error, nullptr,
			(is_include ? "Cannot parse shader include: " : "Cannot parse shader: ") + p_path);
	}

	if (is_include) {
		Ref<ShaderInclude> shader_inc;
		shader_inc.instantiate();

		shader_inc->set_include_path(p_path);
		shader_inc->set_code(str);

		if (r_error) {
			*r_error = OK;
		}

		return shader_inc;
	}

	Ref<Shader> shader;
	shader.instantiate();

	shader->set_include_path(p_path);
	shader->set_code(str);

	if (r_error) {
		*r_error = OK;
	}

	return shader;
}

void ResourceFormatLoaderShader::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("gdshader");
	p_extensions->push_back("shader");
	p_extensions->push_back("gdshaderinc");
}

void ResourceFormatLoaderShader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}
	if (p_type == "Shader") {
		p_extensions->push_back("gdshader");
		p_extensions->push_back("shader");
	}
	else if (p_type == "ShaderInclude") {
		p_extensions->push_back("gdshaderinc");
	}
}

bool ResourceFormatLoaderShader::handles_type(const String& p_type) const
{
	return (p_type == "Shader" || p_type == "ShaderInclude");
}

String ResourceFormatLoaderShader::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("gdshader") || p_path.has_extension("shader")) {
		return "Shader";
	}
	if (p_path.has_extension("gdshaderinc")) {
		return "ShaderInclude";
	}
	return "";
}

Error ResourceFormatSaverShader::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	(void)p_flags;

	Ref<Shader> shader = p_resource;
	Ref<ShaderInclude> shader_inc = p_resource;
	ERR_FAIL_COND_V(shader.is_null() && shader_inc.is_null(), ERR_INVALID_PARAMETER);

	String source = shader.is_valid() ? shader->get_code() : shader_inc->get_code();

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err, err,
		(shader_inc.is_valid() ? "Cannot save shader include '" : "Cannot save shader '") + p_path +
			"'.");

	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

bool ResourceFormatSaverShader::recognize(const Ref<Resource>& p_resource) const
{
	if (p_resource.is_null()) {
		return false;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid()) {
		return shader->is_text_shader();
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	return shader_inc.is_valid();
}

void ResourceFormatSaverShader::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	if (p_resource.is_null()) {
		return;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid() && shader->is_text_shader()) {
		p_extensions->push_back("gdshader");
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	if (shader_inc.is_valid()) {
		p_extensions->push_back("gdshaderinc");
	}
}

// ============================================================================
// Merged from scene/resources/environment.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/resources/environment.h"
#include "scene/resources/gradient.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

RID Environment::get_rid() const { return environment; }

Environment::BGMode Environment::get_background() const { return bg_mode; }

void Environment::set_sky(const Ref<Sky>& p_sky)
{
	bg_sky = p_sky;
	RID sb_rid;
	if (bg_sky.is_valid()) {
		sb_rid = bg_sky->get_rid();
	}
	Renderer::environment_set_sky(environment, sb_rid);
}

Ref<Sky> Environment::get_sky() const { return bg_sky; }

void Environment::set_sky_custom_fov(float p_scale)
{
	bg_sky_custom_fov = p_scale;
	Renderer::environment_set_sky_custom_fov(environment, p_scale);
}

float Environment::get_sky_custom_fov() const { return bg_sky_custom_fov; }

void Environment::set_sky_rotation(const Vector3& p_rotation)
{
	bg_sky_rotation = p_rotation;
	Renderer::environment_set_sky_orientation(
		environment, Basis::from_euler(p_rotation));
}

Vector3 Environment::get_sky_rotation() const { return bg_sky_rotation; }

void Environment::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	Renderer::environment_set_bg_color(environment, p_color);
}

Color Environment::get_bg_color() const { return bg_color; }

void Environment::set_bg_energy_multiplier(float p_multiplier)
{
	bg_energy_multiplier = p_multiplier;
	_update_bg_energy();
}

float Environment::get_bg_energy_multiplier() const { return bg_energy_multiplier; }

void Environment::set_bg_intensity(float p_exposure_value)
{
	bg_intensity = p_exposure_value;
	_update_bg_energy();
}

float Environment::get_bg_intensity() const { return bg_intensity; }

void Environment::_update_bg_energy()
{
	if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
		Renderer::environment_set_bg_energy(
			environment, bg_energy_multiplier, bg_intensity);
	}
	else {
		Renderer::environment_set_bg_energy(environment, bg_energy_multiplier, 1.0);
	}
}

void Environment::set_canvas_max_layer(int p_max_layer)
{
	bg_canvas_max_layer = p_max_layer;
	Renderer::environment_set_canvas_max_layer(environment, p_max_layer);
}

int Environment::get_canvas_max_layer() const { return bg_canvas_max_layer; }

void Environment::set_camera_feed_id(int p_id)
{
	bg_camera_feed_id = p_id;
	Renderer::environment_set_camera_feed_id(environment, bg_camera_feed_id);
}

int Environment::get_camera_feed_id() const { return bg_camera_feed_id; }


void Environment::set_ambient_light_color(const Color& p_color)
{
	ambient_color = p_color;
	_update_ambient_light();
}

Color Environment::get_ambient_light_color() const { return ambient_color; }

Environment::AmbientSource Environment::get_ambient_source() const { return ambient_source; }

void Environment::set_ambient_light_energy(float p_energy)
{
	ambient_energy = p_energy;
	_update_ambient_light();
}

float Environment::get_ambient_light_energy() const { return ambient_energy; }

void Environment::set_ambient_light_sky_contribution(float p_ratio)
{
	// Sky contribution values outside the [0.0; 1.0] range don't make sense and
	// can result in negative colors.
	ambient_sky_contribution = CLAMP(p_ratio, 0.0, 1.0);
	_update_ambient_light();
}

float Environment::get_ambient_light_sky_contribution() const { return ambient_sky_contribution; }

Environment::ReflectionSource Environment::get_reflection_source() const
{
	return reflection_source;
}

void Environment::_update_ambient_light()
{
	Renderer::environment_set_ambient_light(environment, ambient_color,
		RSE::EnvironmentAmbientSource(ambient_source), ambient_energy, ambient_sky_contribution,
		RSE::EnvironmentReflectionSource(reflection_source));
}

Environment::ToneMapper Environment::get_tonemapper() const { return tone_mapper; }

void Environment::set_tonemap_exposure(float p_exposure)
{
	tonemap_exposure = p_exposure;
	_update_tonemap();
}

float Environment::get_tonemap_exposure() const { return tonemap_exposure; }

void Environment::set_tonemap_white(float p_white)
{
	tonemap_white = p_white;
	_update_tonemap();
}

float Environment::get_tonemap_white() const { return tonemap_white; }

void Environment::set_tonemap_agx_white(float p_white)
{
	tonemap_agx_white = p_white;
	_update_tonemap();
}

float Environment::get_tonemap_agx_white() const { return tonemap_agx_white; }

void Environment::set_tonemap_agx_contrast(float p_agx_contrast)
{
	tonemap_agx_contrast = p_agx_contrast;
	Renderer::environment_set_tonemap_agx_contrast(environment, p_agx_contrast);
}

float Environment::get_tonemap_agx_contrast() const { return tonemap_agx_contrast; }

void Environment::_update_tonemap()
{
	Renderer::environment_set_tonemap(environment,
		RSE::EnvironmentToneMapper(tone_mapper), tonemap_exposure,
		tone_mapper == TONE_MAPPER_AGX ? tonemap_agx_white : tonemap_white);
}


void Environment::set_ssr_enabled(bool p_enabled)
{
	ssr_enabled = p_enabled;
	_update_ssr();
}

bool Environment::is_ssr_enabled() const { return ssr_enabled; }

void Environment::set_ssr_max_steps(int p_steps)
{
	ssr_max_steps = p_steps;
	_update_ssr();
}

int Environment::get_ssr_max_steps() const { return ssr_max_steps; }

void Environment::set_ssr_fade_in(float p_fade_in)
{
	ssr_fade_in = MAX(p_fade_in, 0.0f);
	_update_ssr();
}

float Environment::get_ssr_fade_in() const { return ssr_fade_in; }

void Environment::set_ssr_fade_out(float p_fade_out)
{
	ssr_fade_out = MAX(p_fade_out, 0.0f);
	_update_ssr();
}

float Environment::get_ssr_fade_out() const { return ssr_fade_out; }

void Environment::set_ssr_depth_tolerance(float p_depth_tolerance)
{
	ssr_depth_tolerance = p_depth_tolerance;
	_update_ssr();
}

float Environment::get_ssr_depth_tolerance() const { return ssr_depth_tolerance; }

void Environment::_update_ssr()
{
	Renderer::environment_set_ssr(
		environment, ssr_enabled, ssr_max_steps, ssr_fade_in, ssr_fade_out, ssr_depth_tolerance);
}


void Environment::set_ssao_enabled(bool p_enabled)
{
	ssao_enabled = p_enabled;
	_update_ssao();
}

bool Environment::is_ssao_enabled() const { return ssao_enabled; }

void Environment::set_ssao_radius(float p_radius)
{
	ssao_radius = p_radius;
	_update_ssao();
}

float Environment::get_ssao_radius() const { return ssao_radius; }

void Environment::set_ssao_intensity(float p_intensity)
{
	ssao_intensity = p_intensity;
	_update_ssao();
}

float Environment::get_ssao_intensity() const { return ssao_intensity; }

void Environment::set_ssao_power(float p_power)
{
	ssao_power = p_power;
	_update_ssao();
}

float Environment::get_ssao_power() const { return ssao_power; }

void Environment::set_ssao_detail(float p_detail)
{
	ssao_detail = p_detail;
	_update_ssao();
}

float Environment::get_ssao_detail() const { return ssao_detail; }

void Environment::set_ssao_horizon(float p_horizon)
{
	ssao_horizon = p_horizon;
	_update_ssao();
}

float Environment::get_ssao_horizon() const { return ssao_horizon; }

void Environment::set_ssao_sharpness(float p_sharpness)
{
	ssao_sharpness = p_sharpness;
	_update_ssao();
}

float Environment::get_ssao_sharpness() const { return ssao_sharpness; }

void Environment::set_ssao_direct_light_affect(float p_direct_light_affect)
{
	ssao_direct_light_affect = p_direct_light_affect;
	_update_ssao();
}

float Environment::get_ssao_direct_light_affect() const { return ssao_direct_light_affect; }

void Environment::set_ssao_ao_channel_affect(float p_ao_channel_affect)
{
	ssao_ao_channel_affect = p_ao_channel_affect;
	_update_ssao();
}

float Environment::get_ssao_ao_channel_affect() const { return ssao_ao_channel_affect; }

void Environment::_update_ssao()
{
	Renderer::environment_set_ssao(environment, ssao_enabled, ssao_radius,
		ssao_intensity, ssao_power, ssao_detail, ssao_horizon, ssao_sharpness,
		ssao_direct_light_affect, ssao_ao_channel_affect);
}


void Environment::set_ssil_enabled(bool p_enabled)
{
	ssil_enabled = p_enabled;
	_update_ssil();
}

bool Environment::is_ssil_enabled() const { return ssil_enabled; }

void Environment::set_ssil_radius(float p_radius)
{
	ssil_radius = p_radius;
	_update_ssil();
}

float Environment::get_ssil_radius() const { return ssil_radius; }

void Environment::set_ssil_intensity(float p_intensity)
{
	ssil_intensity = p_intensity;
	_update_ssil();
}

float Environment::get_ssil_intensity() const { return ssil_intensity; }

void Environment::set_ssil_sharpness(float p_sharpness)
{
	ssil_sharpness = p_sharpness;
	_update_ssil();
}

float Environment::get_ssil_sharpness() const { return ssil_sharpness; }

void Environment::set_ssil_normal_rejection(float p_normal_rejection)
{
	ssil_normal_rejection = p_normal_rejection;
	_update_ssil();
}

float Environment::get_ssil_normal_rejection() const { return ssil_normal_rejection; }

void Environment::_update_ssil()
{
	Renderer::environment_set_ssil(environment, ssil_enabled, ssil_radius,
		ssil_intensity, ssil_sharpness, ssil_normal_rejection);
}


void Environment::set_sdfgi_enabled(bool p_enabled)
{
	sdfgi_enabled = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_enabled() const { return sdfgi_enabled; }

void Environment::set_sdfgi_cascades(int p_cascades)
{
	ERR_FAIL_COND_MSG(p_cascades < 1 || p_cascades > 8,
		"Invalid number of SDFGI cascades (must be between 1 and 8).");
	sdfgi_cascades = p_cascades;
	_update_sdfgi();
}

int Environment::get_sdfgi_cascades() const { return sdfgi_cascades; }

void Environment::set_sdfgi_min_cell_size(float p_size)
{
	sdfgi_min_cell_size = p_size;
	_update_sdfgi();
}

float Environment::get_sdfgi_min_cell_size() const { return sdfgi_min_cell_size; }

void Environment::set_sdfgi_max_distance(float p_distance)
{
	p_distance /= 64.0;
	for (int i = 0; i < sdfgi_cascades; i++) {
		p_distance *= 0.5; // halve for each cascade
	}
	sdfgi_min_cell_size = p_distance;
	_update_sdfgi();
}

float Environment::get_sdfgi_max_distance() const
{
	float md = sdfgi_min_cell_size;
	md *= 64.0;
	for (int i = 0; i < sdfgi_cascades; i++) {
		md *= 2.0;
	}
	return md;
}

void Environment::set_sdfgi_cascade0_distance(float p_distance)
{
	sdfgi_min_cell_size = p_distance / 64.0;
	_update_sdfgi();
}

float Environment::get_sdfgi_cascade0_distance() const { return sdfgi_min_cell_size * 64.0; }

void Environment::set_sdfgi_y_scale(SDFGIYScale p_y_scale)
{
	sdfgi_y_scale = p_y_scale;
	_update_sdfgi();
}

Environment::SDFGIYScale Environment::get_sdfgi_y_scale() const { return sdfgi_y_scale; }

void Environment::set_sdfgi_use_occlusion(bool p_enabled)
{
	sdfgi_use_occlusion = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_using_occlusion() const { return sdfgi_use_occlusion; }

void Environment::set_sdfgi_bounce_feedback(float p_amount)
{
	sdfgi_bounce_feedback = p_amount;
	_update_sdfgi();
}

float Environment::get_sdfgi_bounce_feedback() const { return sdfgi_bounce_feedback; }

void Environment::set_sdfgi_read_sky_light(bool p_enabled)
{
	sdfgi_read_sky_light = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_reading_sky_light() const { return sdfgi_read_sky_light; }

void Environment::set_sdfgi_energy(float p_energy)
{
	sdfgi_energy = p_energy;
	_update_sdfgi();
}

float Environment::get_sdfgi_energy() const { return sdfgi_energy; }

void Environment::set_sdfgi_normal_bias(float p_bias)
{
	sdfgi_normal_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_normal_bias() const { return sdfgi_normal_bias; }

void Environment::set_sdfgi_probe_bias(float p_bias)
{
	sdfgi_probe_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_probe_bias() const { return sdfgi_probe_bias; }

void Environment::_update_sdfgi()
{
	Renderer::environment_set_sdfgi(environment, sdfgi_enabled, sdfgi_cascades,
		sdfgi_min_cell_size, RSE::EnvironmentSDFGIYScale(sdfgi_y_scale), sdfgi_use_occlusion,
		sdfgi_bounce_feedback, sdfgi_read_sky_light, sdfgi_energy, sdfgi_normal_bias,
		sdfgi_probe_bias);
}


void Environment::set_glow_enabled(bool p_enabled)
{
	glow_enabled = p_enabled;
	_update_glow();
}

bool Environment::is_glow_enabled() const { return glow_enabled; }

void Environment::set_glow_level(int p_level, float p_intensity)
{
	ERR_FAIL_INDEX(p_level, RSE::MAX_GLOW_LEVELS);

	glow_levels.write[p_level] = p_intensity;

	_update_glow();
}

float Environment::get_glow_level(int p_level) const
{
	ERR_FAIL_INDEX_V(p_level, RSE::MAX_GLOW_LEVELS, 0.0);

	return glow_levels[p_level];
}

void Environment::set_glow_normalized(bool p_normalized)
{
	glow_normalize_levels = p_normalized;

	_update_glow();
}

bool Environment::is_glow_normalized() const { return glow_normalize_levels; }

void Environment::set_glow_intensity(float p_intensity)
{
	glow_intensity = p_intensity;
	_update_glow();
}

float Environment::get_glow_intensity() const { return glow_intensity; }

void Environment::set_glow_strength(float p_strength)
{
	glow_strength = p_strength;
	_update_glow();
}

float Environment::get_glow_strength() const { return glow_strength; }

void Environment::set_glow_mix(float p_mix)
{
	glow_mix = p_mix;
	_update_glow();
}

float Environment::get_glow_mix() const { return glow_mix; }

void Environment::set_glow_bloom(float p_threshold)
{
	glow_bloom = p_threshold;
	_update_glow();
}

float Environment::get_glow_bloom() const { return glow_bloom; }

Environment::GlowBlendMode Environment::get_glow_blend_mode() const { return glow_blend_mode; }

void Environment::set_glow_hdr_bleed_threshold(float p_threshold)
{
	glow_hdr_bleed_threshold = p_threshold;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_threshold() const { return glow_hdr_bleed_threshold; }

void Environment::set_glow_hdr_bleed_scale(float p_scale)
{
	glow_hdr_bleed_scale = p_scale;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_scale() const { return glow_hdr_bleed_scale; }

void Environment::set_glow_hdr_luminance_cap(float p_amount)
{
	glow_hdr_luminance_cap = p_amount;
	_update_glow();
}

float Environment::get_glow_hdr_luminance_cap() const { return glow_hdr_luminance_cap; }

void Environment::set_glow_map_strength(float p_strength)
{
	glow_map_strength = p_strength;
	_update_glow();
}

float Environment::get_glow_map_strength() const { return glow_map_strength; }

void Environment::set_glow_map(Ref<Texture> p_glow_map)
{
	glow_map = p_glow_map;
	_update_glow();
}

Ref<Texture> Environment::get_glow_map() const { return glow_map; }

void Environment::_update_glow()
{
	Vector<float> normalized_levels;
	if (glow_normalize_levels) {
		normalized_levels.resize(7);
		float size = 0.0;
		for (int i = 0; i < glow_levels.size(); i++) {
			size += glow_levels[i];
		}
		for (int i = 0; i < glow_levels.size(); i++) {
			normalized_levels.write[i] = glow_levels[i] / size;
		}
	}
	else {
		normalized_levels = glow_levels;
	}

	float _glow_map_strength = 0.0f;
	RID glow_map_rid;
	if (glow_map.is_valid()) {
		glow_map_rid = glow_map->get_rid();
		_glow_map_strength = glow_map_strength;
	}
	else {
		glow_map_rid = RID();
	}

	Renderer::environment_set_glow(environment, glow_enabled, normalized_levels,
		glow_intensity, glow_strength, glow_mix, glow_bloom,
		RSE::EnvironmentGlowBlendMode(glow_blend_mode), glow_hdr_bleed_threshold,
		glow_hdr_bleed_scale, glow_hdr_luminance_cap, _glow_map_strength, glow_map_rid);
}


void Environment::set_fog_enabled(bool p_enabled)
{
	fog_enabled = p_enabled;
	_update_fog();
}

bool Environment::is_fog_enabled() const { return fog_enabled; }

Environment::FogMode Environment::get_fog_mode() const { return fog_mode; }

void Environment::set_fog_light_color(const Color& p_light_color)
{
	fog_light_color = p_light_color;
	_update_fog();
}

Color Environment::get_fog_light_color() const { return fog_light_color; }

void Environment::set_fog_light_energy(float p_amount)
{
	fog_light_energy = p_amount;
	_update_fog();
}

float Environment::get_fog_light_energy() const { return fog_light_energy; }

void Environment::set_fog_sun_scatter(float p_amount)
{
	fog_sun_scatter = p_amount;
	_update_fog();
}

float Environment::get_fog_sun_scatter() const { return fog_sun_scatter; }

void Environment::set_fog_density(float p_amount)
{
	fog_density = p_amount;
	_update_fog();
}

float Environment::get_fog_density() const { return fog_density; }

void Environment::set_fog_height(float p_amount)
{
	fog_height = p_amount;
	_update_fog();
}

float Environment::get_fog_height() const { return fog_height; }

void Environment::set_fog_height_density(float p_amount)
{
	fog_height_density = p_amount;
	_update_fog();
}

float Environment::get_fog_height_density() const { return fog_height_density; }

void Environment::set_fog_aerial_perspective(float p_aerial_perspective)
{
	fog_aerial_perspective = p_aerial_perspective;
	_update_fog();
}

float Environment::get_fog_aerial_perspective() const { return fog_aerial_perspective; }

void Environment::set_fog_sky_affect(float p_sky_affect)
{
	fog_sky_affect = p_sky_affect;
	_update_fog();
}

float Environment::get_fog_sky_affect() const { return fog_sky_affect; }

void Environment::_update_fog()
{
	Renderer::environment_set_fog(environment, fog_enabled, fog_light_color,
		fog_light_energy, fog_sun_scatter, fog_density, fog_height, fog_height_density,
		fog_aerial_perspective, fog_sky_affect, RSE::EnvironmentFogMode(fog_mode));
}


void Environment::set_fog_depth_curve(float p_curve)
{
	fog_depth_curve = p_curve;
	_update_fog_depth();
}

float Environment::get_fog_depth_curve() const { return fog_depth_curve; }

void Environment::set_fog_depth_begin(float p_begin)
{
	fog_depth_begin = p_begin;
	if (fog_depth_begin > fog_depth_end) {
		set_fog_depth_end(fog_depth_begin);
	}
	_update_fog_depth();
}

float Environment::get_fog_depth_begin() const { return fog_depth_begin; }

void Environment::set_fog_depth_end(float p_end)
{
	fog_depth_end = p_end;
	if (fog_depth_end < fog_depth_begin) {
		set_fog_depth_begin(fog_depth_end);
	}
	_update_fog_depth();
}

float Environment::get_fog_depth_end() const { return fog_depth_end; }

void Environment::_update_fog_depth()
{
	Renderer::environment_set_fog_depth(
		environment, fog_depth_curve, fog_depth_begin, fog_depth_end);
}


void Environment::_update_volumetric_fog()
{
	Renderer::environment_set_volumetric_fog(environment, volumetric_fog_enabled,
		volumetric_fog_density, volumetric_fog_albedo, volumetric_fog_emission,
		volumetric_fog_emission_energy, volumetric_fog_anisotropy, volumetric_fog_length,
		volumetric_fog_detail_spread, volumetric_fog_gi_inject, volumetric_fog_temporal_reproject,
		volumetric_fog_temporal_reproject_amount, volumetric_fog_ambient_inject,
		volumetric_fog_sky_affect);
}

void Environment::set_volumetric_fog_enabled(bool p_enable)
{
	volumetric_fog_enabled = p_enable;
	_update_volumetric_fog();
}

bool Environment::is_volumetric_fog_enabled() const { return volumetric_fog_enabled; }

void Environment::set_volumetric_fog_density(float p_density)
{
	volumetric_fog_density = p_density;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_density() const { return volumetric_fog_density; }

void Environment::set_volumetric_fog_albedo(Color p_color)
{
	volumetric_fog_albedo = p_color;
	_update_volumetric_fog();
}

Color Environment::get_volumetric_fog_albedo() const { return volumetric_fog_albedo; }

void Environment::set_volumetric_fog_emission(Color p_color)
{
	volumetric_fog_emission = p_color;
	_update_volumetric_fog();
}

Color Environment::get_volumetric_fog_emission() const { return volumetric_fog_emission; }

void Environment::set_volumetric_fog_emission_energy(float p_begin)
{
	volumetric_fog_emission_energy = p_begin;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_emission_energy() const
{
	return volumetric_fog_emission_energy;
}

void Environment::set_volumetric_fog_anisotropy(float p_anisotropy)
{
	volumetric_fog_anisotropy = p_anisotropy;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_anisotropy() const { return volumetric_fog_anisotropy; }

void Environment::set_volumetric_fog_length(float p_length)
{
	volumetric_fog_length = p_length;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_length() const { return volumetric_fog_length; }

void Environment::set_volumetric_fog_detail_spread(float p_detail_spread)
{
	p_detail_spread = CLAMP(p_detail_spread, 0.5, 6.0);
	volumetric_fog_detail_spread = p_detail_spread;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_detail_spread() const { return volumetric_fog_detail_spread; }

void Environment::set_volumetric_fog_gi_inject(float p_gi_inject)
{
	volumetric_fog_gi_inject = p_gi_inject;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_gi_inject() const { return volumetric_fog_gi_inject; }

void Environment::set_volumetric_fog_ambient_inject(float p_ambient_inject)
{
	volumetric_fog_ambient_inject = p_ambient_inject;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_ambient_inject() const
{
	return volumetric_fog_ambient_inject;
}

void Environment::set_volumetric_fog_sky_affect(float p_sky_affect)
{
	volumetric_fog_sky_affect = p_sky_affect;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_sky_affect() const { return volumetric_fog_sky_affect; }

void Environment::set_volumetric_fog_temporal_reprojection_enabled(bool p_enable)
{
	volumetric_fog_temporal_reproject = p_enable;
	_update_volumetric_fog();
}

bool Environment::is_volumetric_fog_temporal_reprojection_enabled() const
{
	return volumetric_fog_temporal_reproject;
}

void Environment::set_volumetric_fog_temporal_reprojection_amount(float p_amount)
{
	volumetric_fog_temporal_reproject_amount = p_amount;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_temporal_reprojection_amount() const
{
	return volumetric_fog_temporal_reproject_amount;
}


void Environment::set_adjustment_enabled(bool p_enabled)
{
	adjustment_enabled = p_enabled;
	_update_adjustment();
}

bool Environment::is_adjustment_enabled() const { return adjustment_enabled; }

void Environment::set_adjustment_brightness(float p_brightness)
{
	adjustment_brightness = p_brightness;
	_update_adjustment();
}

float Environment::get_adjustment_brightness() const { return adjustment_brightness; }

void Environment::set_adjustment_contrast(float p_contrast)
{
	adjustment_contrast = p_contrast;
	_update_adjustment();
}

float Environment::get_adjustment_contrast() const { return adjustment_contrast; }

void Environment::set_adjustment_saturation(float p_saturation)
{
	adjustment_saturation = p_saturation;
	_update_adjustment();
}

float Environment::get_adjustment_saturation() const { return adjustment_saturation; }

Ref<Texture> Environment::get_adjustment_color_correction() const
{
	return adjustment_color_correction;
}

void Environment::_update_adjustment()
{
	RID color_correction =
		adjustment_color_correction.is_valid() ? adjustment_color_correction->get_rid() : RID();

	Renderer::environment_set_adjustment(environment, adjustment_enabled,
		adjustment_brightness, adjustment_contrast, adjustment_saturation, use_1d_color_correction,
		color_correction);
}

Environment::~Environment()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(environment);
}



void Environment::set_tonemapper(Environment::ToneMapper) {}

// --- camera_attributes.cpp ---

#include "scene/resources/material.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "servers/rendering/renderer.h"

void CameraAttributes::set_exposure_multiplier(float p_multiplier)
{
	exposure_multiplier = p_multiplier;
	_update_exposure();
	emit_changed();
}

float CameraAttributes::get_exposure_multiplier() const { return exposure_multiplier; }

void CameraAttributes::set_exposure_sensitivity(float p_sensitivity)
{
	exposure_sensitivity = p_sensitivity;
	_update_exposure();
	emit_changed();
}

float CameraAttributes::get_exposure_sensitivity() const { return exposure_sensitivity; }

void CameraAttributes::_update_exposure()
{
	float exposure_normalization = 1.0;
	// Ignore physical properties if not using physical light units.
	if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
		exposure_normalization = calculate_exposure_normalization();
	}

	Renderer::camera_attributes_set_exposure(
		camera_attributes, exposure_multiplier, exposure_normalization);
}

bool CameraAttributes::is_auto_exposure_enabled() const { return auto_exposure_enabled; }

void CameraAttributes::set_auto_exposure_speed(float p_auto_exposure_speed)
{
	auto_exposure_speed = p_auto_exposure_speed;
	_update_auto_exposure();
}

float CameraAttributes::get_auto_exposure_speed() const { return auto_exposure_speed; }

void CameraAttributes::set_auto_exposure_scale(float p_auto_exposure_scale)
{
	auto_exposure_scale = p_auto_exposure_scale;
	_update_auto_exposure();
}

float CameraAttributes::get_auto_exposure_scale() const { return auto_exposure_scale; }

RID CameraAttributes::get_rid() const { return camera_attributes; }


CameraAttributes::CameraAttributes()
{
	camera_attributes = Renderer::camera_attributes_create();
}

CameraAttributes::~CameraAttributes()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(camera_attributes);
}

//////////////////////////////////////////////////////
/* CameraAttributesPractical */

bool CameraAttributesPractical::is_dof_blur_far_enabled() const { return dof_blur_far_enabled; }

void CameraAttributesPractical::set_dof_blur_far_distance(float p_distance)
{
	dof_blur_far_distance = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_far_distance() const { return dof_blur_far_distance; }

void CameraAttributesPractical::set_dof_blur_far_transition(float p_distance)
{
	dof_blur_far_transition = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_far_transition() const
{
	return dof_blur_far_transition;
}

bool CameraAttributesPractical::is_dof_blur_near_enabled() const { return dof_blur_near_enabled; }

void CameraAttributesPractical::set_dof_blur_near_distance(float p_distance)
{
	dof_blur_near_distance = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_near_distance() const
{
	return dof_blur_near_distance;
}

void CameraAttributesPractical::set_dof_blur_near_transition(float p_distance)
{
	dof_blur_near_transition = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_near_transition() const
{
	return dof_blur_near_transition;
}

void CameraAttributesPractical::set_dof_blur_amount(float p_amount)
{
	dof_blur_amount = p_amount;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_amount() const { return dof_blur_amount; }

void CameraAttributesPractical::_update_dof_blur()
{
	Renderer::camera_attributes_set_dof_blur(get_rid(), dof_blur_far_enabled,
		dof_blur_far_distance, dof_blur_far_transition, dof_blur_near_enabled,
		dof_blur_near_distance, dof_blur_near_transition, dof_blur_amount);
}

float CameraAttributesPractical::calculate_exposure_normalization() const
{
	return exposure_sensitivity / 3072007.0; // Matches exposure normalization for default
											 // CameraAttributesPhysical at ISO 100.
}

void CameraAttributesPractical::set_auto_exposure_min_sensitivity(float p_min)
{
	auto_exposure_min = p_min;
	_update_auto_exposure();
}

float CameraAttributesPractical::get_auto_exposure_min_sensitivity() const
{
	return auto_exposure_min;
}

void CameraAttributesPractical::set_auto_exposure_max_sensitivity(float p_max)
{
	auto_exposure_max = p_max;
	_update_auto_exposure();
}

float CameraAttributesPractical::get_auto_exposure_max_sensitivity() const
{
	return auto_exposure_max;
}

void CameraAttributesPractical::_update_auto_exposure()
{
	Renderer::camera_attributes_set_auto_exposure(get_rid(), auto_exposure_enabled,
		auto_exposure_min *
			((12.5 / 100.0) / exposure_sensitivity), // Convert from Sensitivity to Luminance
		auto_exposure_max *
			((12.5 / 100.0) / exposure_sensitivity), // Convert from Sensitivity to Luminance
		auto_exposure_speed, auto_exposure_scale);
	emit_changed();
}

CameraAttributesPractical::~CameraAttributesPractical() {}

//////////////////////////////////////////////////////
/* CameraAttributesPhysical */

void CameraAttributesPhysical::set_aperture(float p_aperture)
{
	exposure_aperture = p_aperture;
	_update_exposure();
	_update_frustum();
}

float CameraAttributesPhysical::get_aperture() const { return exposure_aperture; }

void CameraAttributesPhysical::set_shutter_speed(float p_shutter_speed)
{
	exposure_shutter_speed = p_shutter_speed;
	_update_exposure();
}

float CameraAttributesPhysical::get_shutter_speed() const { return exposure_shutter_speed; }

void CameraAttributesPhysical::set_focal_length(float p_focal_length)
{
	frustum_focal_length = p_focal_length;
	_update_frustum();
	emit_changed();
}

float CameraAttributesPhysical::get_focal_length() const { return frustum_focal_length; }

void CameraAttributesPhysical::set_focus_distance(float p_focus_distance)
{
	frustum_focus_distance = p_focus_distance;
	_update_frustum();
}

float CameraAttributesPhysical::get_focus_distance() const { return frustum_focus_distance; }

void CameraAttributesPhysical::set_near(real_t p_near)
{
	frustum_near = p_near;
	_update_frustum();
	emit_changed();
}

real_t CameraAttributesPhysical::get_near() const { return frustum_near; }

void CameraAttributesPhysical::set_far(real_t p_far)
{
	frustum_far = p_far;
	_update_frustum();
	emit_changed();
}

real_t CameraAttributesPhysical::get_far() const { return frustum_far; }

real_t CameraAttributesPhysical::get_fov() const { return frustum_fov; }

void CameraAttributesPhysical::_update_frustum()
{
	// https://en.wikipedia.org/wiki/Circle_of_confusion#Circle_of_confusion_diameter_limit_based_on_d/1500
	Vector2i sensor_size =
		Vector2i(36, 24); // Matches high-end DSLR, could be made variable if there is demand.
	float CoC = sensor_size.length() / 1500.0;

	frustum_fov = Math::rad_to_deg(2 * std::atan(sensor_size.height / (2 * frustum_focal_length)));

	// Based on https://en.wikipedia.org/wiki/Depth_of_field.
	float u = MAX(frustum_focus_distance * 1000.0,
		frustum_focal_length +
			1.0); // Focus distance expressed in mm and clamped to at least 1 mm away from lens.
	float hyperfocal_length =
		frustum_focal_length +
		((frustum_focal_length * frustum_focal_length) / (exposure_aperture * CoC));

	// This computes the start and end of the depth of field. Anything between these two points has
	// a Circle of Confusino so small that it is not picked up by the camera sensors. To be properly
	// physically-based, we would run the DoF shader at all depths. To be efficient, we are only
	// running it where the CoC will be visible, this introduces some value shifts in the near field
	// that we have to compensate for below.
	float depth_near =
		((hyperfocal_length * u) / (hyperfocal_length + (u - frustum_focal_length))) /
		1000.0; // In meters.
	float depth_far = ((hyperfocal_length * u) / (hyperfocal_length - (u - frustum_focal_length))) /
					  1000.0; // In meters.
	float scale = (frustum_focal_length / (u - frustum_focal_length)) *
				  (frustum_focal_length / exposure_aperture);

	bool use_far = (depth_far < frustum_far) && (depth_far > 0.0);
	bool use_near = depth_near > frustum_near;
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "gl_compatibility") {
		// Force disable DoF in editor builds to suppress warnings.
		use_far = false;
		use_near = false;
	}
#endif
	Renderer::camera_attributes_set_dof_blur(get_rid(), use_far,
		u / 1000.0, // Focus distance clampd to focal length expressed in meters.
		-1.0,		// Negative to tell Bokeh effect to use physically-based scaling.
		use_near, u / 1000.0, -1.0,
		scale / 5.0); // Arbitrary scaling to get close to how much blur there should be.
}

float CameraAttributesPhysical::calculate_exposure_normalization() const
{
	const float e = (exposure_aperture * exposure_aperture) * exposure_shutter_speed *
					(100.0 / exposure_sensitivity);
	return 1.0 / (e * 1.2);
}

void CameraAttributesPhysical::set_auto_exposure_min_exposure_value(float p_min)
{
	auto_exposure_min = p_min;
	_update_auto_exposure();
}

float CameraAttributesPhysical::get_auto_exposure_min_exposure_value() const
{
	return auto_exposure_min;
}

void CameraAttributesPhysical::set_auto_exposure_max_exposure_value(float p_max)
{
	auto_exposure_max = p_max;
	_update_auto_exposure();
}

float CameraAttributesPhysical::get_auto_exposure_max_exposure_value() const
{
	return auto_exposure_max;
}

void CameraAttributesPhysical::_update_auto_exposure()
{
	Renderer::camera_attributes_set_auto_exposure(get_rid(), auto_exposure_enabled,
		std::pow(2.0, auto_exposure_min) *
			(12.5 / exposure_sensitivity), // Convert from EV100 to Luminance
		std::pow(2.0, auto_exposure_max) *
			(12.5 / exposure_sensitivity), // Convert from EV100 to Luminance
		auto_exposure_speed, auto_exposure_scale);
	emit_changed();
}

CameraAttributesPhysical::~CameraAttributesPhysical() {}

// --- compositor.cpp ---

#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

void CompositorEffect::set_enabled(bool p_enabled)
{
	enabled = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_enabled(rid, enabled);
	}
}

bool CompositorEffect::get_enabled() const { return enabled; }

CompositorEffect::EffectCallbackType CompositorEffect::get_effect_callback_type() const
{
	return effect_callback_type;
}

void CompositorEffect::set_access_resolved_color(bool p_enabled)
{
	access_resolved_color = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_COLOR,
			access_resolved_color);
	}
}

bool CompositorEffect::get_access_resolved_color() const { return access_resolved_color; }

void CompositorEffect::set_access_resolved_depth(bool p_enabled)
{
	access_resolved_depth = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_DEPTH,
			access_resolved_depth);
	}
}

bool CompositorEffect::get_access_resolved_depth() const { return access_resolved_depth; }

void CompositorEffect::set_needs_motion_vectors(bool p_enabled)
{
	needs_motion_vectors = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_MOTION_VECTORS,
			needs_motion_vectors);
	}
}

bool CompositorEffect::get_needs_motion_vectors() const { return needs_motion_vectors; }

void CompositorEffect::set_needs_normal_roughness(bool p_enabled)
{
	needs_normal_roughness = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_ROUGHNESS,
			needs_normal_roughness);
	}
}

bool CompositorEffect::get_needs_normal_roughness() const { return needs_normal_roughness; }

void CompositorEffect::set_needs_separate_specular(bool p_enabled)
{
	needs_separate_specular = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_SEPARATE_SPECULAR,
			needs_separate_specular);
	}
}

bool CompositorEffect::get_needs_separate_specular() const { return needs_separate_specular; }

CompositorEffect::~CompositorEffect()
{
	if (Renderer::data != nullptr && rid.is_valid()) {
		Renderer::free_rid(rid);
	}
}

/* Compositor */


Compositor::Compositor()
{
	if (Renderer::data != nullptr) {
		compositor = Renderer::compositor_create();
	}
}

Compositor::~Compositor()
{
	if (Renderer::data != nullptr && compositor.is_valid()) {
		Renderer::free_rid(compositor);
	}
}

void CompositorEffect::_call_render_callback(
	int p_effect_callback_type, const RenderDataRD* p_render_data)
{
}

// --- sky.cpp ---

#include "servers/rendering/renderer.h"
#include "scene/resources/environment.h"

void Sky::set_radiance_size(RadianceSize p_size)
{
	ERR_FAIL_INDEX(p_size, RADIANCE_SIZE_MAX);

	radiance_size = p_size;
	static const int size[RADIANCE_SIZE_MAX] = {32, 64, 128, 256, 512, 1024, 2048};
	Renderer::sky_set_radiance_size(sky, size[radiance_size]);
}

Sky::RadianceSize Sky::get_radiance_size() const { return radiance_size; }

void Sky::set_process_mode(ProcessMode p_mode)
{
	mode = p_mode;
	Renderer::sky_set_mode(sky, RSE::SkyMode(mode));
}

Sky::ProcessMode Sky::get_process_mode() const { return mode; }

void Sky::set_material(const Ref<Material>& p_material)
{
	sky_material = p_material;
	RID material_rid;
	if (sky_material.is_valid()) {
		material_rid = sky_material->get_rid();
	}
	Renderer::sky_set_material(sky, material_rid);
}

Ref<Material> Sky::get_material() const { return sky_material; }

RID Sky::get_rid() const { return sky; }


Sky::Sky() { sky = Renderer::sky_create(); }

Sky::~Sky()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(sky);
}

// --- world_2d.cpp ---

#include "core/config/project_settings.h"
#include "scene/2d/node_2d.h"
#include "servers/rendering/renderer.h"
#include "scene/main/viewport.h"

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID World2D::get_canvas() const { return canvas; }

#ifndef PHYSICS_2D_DISABLED
PhysicsDirectSpaceState2D* World2D::get_direct_space_state()
{
	return PhysicsServer2D::space_get_direct_state(get_space());
}
#endif // PHYSICS_2D_DISABLED

void World2D::register_viewport(Viewport* p_viewport) { viewports.insert(p_viewport); }

void World2D::remove_viewport(Viewport* p_viewport) { viewports.erase(p_viewport); }

World2D::World2D() { canvas = Renderer::canvas_create(); }

World2D::~World2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas);

#ifndef NAVIGATION_2D_DISABLED
	ERR_FAIL_NULL(NavigationServer2D::data);
	if (navigation_map.is_valid()) {
		NavigationServer2D::free_rid(navigation_map);
	}
#endif // NAVIGATION_2D_DISABLED

#ifndef PHYSICS_2D_DISABLED
	if (space.is_valid()) {
		ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
		PhysicsServer2D::free_rid(space);
	}
#endif // PHYSICS_2D_DISABLED
}

#ifndef NAVIGATION_2D_DISABLED
RID World2D::get_navigation_map() const
{
	return RID();
}
#endif // NAVIGATION_2D_DISABLED

RID World2D::get_space() const { return RID(); }

// --- navigation_mesh.cpp ---

#include "scene/resources/navigation_mesh.h"

#include "servers/navigation_3d/navigation_server_3d.h"

NavigationMesh::NavigationMesh() :
		cell_size(NavigationDefaults3D::NAV_MESH_CELL_SIZE),
		cell_height(NavigationDefaults3D::NAV_MESH_CELL_HEIGHT) {
}

void NavigationMesh::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationMesh::SamplePartitionType NavigationMesh::get_sample_partition_type() const
{
	return partition_type;
}

NavigationMesh::ParsedGeometryType NavigationMesh::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationMesh::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t NavigationMesh::get_collision_mask() const { return collision_mask; }

void NavigationMesh::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool NavigationMesh::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

NavigationMesh::SourceGeometryMode NavigationMesh::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationMesh::set_source_group_name(const String& p_group_name)
{
	source_group_name = p_group_name;
}

String NavigationMesh::get_source_group_name() const { return source_group_name; }

void NavigationMesh::set_cell_size(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_size = p_value;
}

float NavigationMesh::get_cell_size() const { return cell_size; }

void NavigationMesh::set_cell_height(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_height = p_value;
}

float NavigationMesh::get_cell_height() const { return cell_height; }

void NavigationMesh::set_border_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	border_size = p_value;
}

float NavigationMesh::get_border_size() const { return border_size; }

void NavigationMesh::set_agent_height(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_height = p_value;
}

float NavigationMesh::get_agent_height() const { return agent_height; }

void NavigationMesh::set_agent_radius(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

float NavigationMesh::get_agent_radius() { return agent_radius; }

void NavigationMesh::set_agent_max_climb(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_max_climb = p_value;
}

float NavigationMesh::get_agent_max_climb() const { return agent_max_climb; }

void NavigationMesh::set_agent_max_slope(float p_value)
{
	ERR_FAIL_COND(p_value < 0 || p_value > 90);
	agent_max_slope = p_value;
}

float NavigationMesh::get_agent_max_slope() const { return agent_max_slope; }

void NavigationMesh::set_region_min_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_min_size = p_value;
}

float NavigationMesh::get_region_min_size() const { return region_min_size; }

void NavigationMesh::set_region_merge_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_merge_size = p_value;
}

float NavigationMesh::get_region_merge_size() const { return region_merge_size; }

void NavigationMesh::set_edge_max_length(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_length = p_value;
}

float NavigationMesh::get_edge_max_length() const { return edge_max_length; }

void NavigationMesh::set_edge_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_error = p_value;
}

float NavigationMesh::get_edge_max_error() const { return edge_max_error; }

void NavigationMesh::set_vertices_per_polygon(float p_value)
{
	ERR_FAIL_COND(p_value < 3);
	vertices_per_polygon = p_value;
}

float NavigationMesh::get_vertices_per_polygon() const { return vertices_per_polygon; }

void NavigationMesh::set_detail_sample_distance(float p_value)
{
	ERR_FAIL_COND(p_value < 0.1);
	detail_sample_distance = p_value;
}

float NavigationMesh::get_detail_sample_distance() const { return detail_sample_distance; }

void NavigationMesh::set_detail_sample_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	detail_sample_max_error = p_value;
}

float NavigationMesh::get_detail_sample_max_error() const { return detail_sample_max_error; }

void NavigationMesh::set_filter_low_hanging_obstacles(bool p_value)
{
	filter_low_hanging_obstacles = p_value;
}

bool NavigationMesh::get_filter_low_hanging_obstacles() const
{
	return filter_low_hanging_obstacles;
}

void NavigationMesh::set_filter_ledge_spans(bool p_value) { filter_ledge_spans = p_value; }

bool NavigationMesh::get_filter_ledge_spans() const { return filter_ledge_spans; }

void NavigationMesh::set_filter_walkable_low_height_spans(bool p_value)
{
	filter_walkable_low_height_spans = p_value;
}

bool NavigationMesh::get_filter_walkable_low_height_spans() const
{
	return filter_walkable_low_height_spans;
}

void NavigationMesh::set_filter_baking_aabb(const AABB& p_aabb)
{
	filter_baking_aabb = p_aabb;
	emit_changed();
}

AABB NavigationMesh::get_filter_baking_aabb() const { return filter_baking_aabb; }

void NavigationMesh::set_filter_baking_aabb_offset(const Vector3& p_aabb_offset)
{
	filter_baking_aabb_offset = p_aabb_offset;
	emit_changed();
}

Vector3 NavigationMesh::get_filter_baking_aabb_offset() const { return filter_baking_aabb_offset; }

Vector<Vector3> NavigationMesh::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

Vector<Vector<int>> NavigationMesh::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

int NavigationMesh::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationMesh::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationMesh::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
}

void NavigationMesh::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
}

void NavigationMesh::set_data(
	const Vector<Vector3>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
}

void NavigationMesh::get_data(Vector<Vector3>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}

void NavigationMesh::set_vertices(const Vector<Vector3>& p_vertices) {}
