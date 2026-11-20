/**************************************************************************/
/*  atlas_texture.cpp                                                     */
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
