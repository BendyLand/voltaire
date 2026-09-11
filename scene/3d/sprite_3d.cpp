/**************************************************************************/
/*  sprite_3d.cpp                                                         */
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

#include "core/config/engine.h"
#include "core/math/triangle_mesh.h"
#include "scene/resources/atlas_texture.h"
#include "servers/rendering/rendering_server.h"
#include "sprite_3d.h"

Color SpriteBase3D::_get_color_accum()
{
	if (!color_dirty) {
		return color_accum;
	}

	if (parent_sprite) {
		color_accum = parent_sprite->_get_color_accum();
	}
	else {
		color_accum = Color(1, 1, 1, 1);
	}

	color_accum.r *= modulate.r;
	color_accum.g *= modulate.g;
	color_accum.b *= modulate.b;
	color_accum.a *= modulate.a;
	color_dirty = false;
	return color_accum;
}

void SpriteBase3D::_propagate_color_changed()
{
	if (color_dirty) {
		return;
	}

	color_dirty = true;
	_queue_redraw();

	for (SpriteBase3D*& E : children) {
		E->_propagate_color_changed();
	}
}

void SpriteBase3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_im_update();
	} break;

	case NOTIFICATION_UNPARENTED: {
		if (parent_sprite) {
			parent_sprite->children.erase(pI);
			pI = nullptr;
			parent_sprite = nullptr;

			_propagate_color_changed();
		}
	} break;
	}
}

void SpriteBase3D::set_centered(bool p_center)
{
	if (centered == p_center) {
		return;
	}

	centered = p_center;
	_queue_redraw();
}

bool SpriteBase3D::is_centered() const { return centered; }

void SpriteBase3D::set_offset(const Point2& p_offset)
{
	if (offset == p_offset) {
		return;
	}

	offset = p_offset;
	_queue_redraw();
}

Point2 SpriteBase3D::get_offset() const { return offset; }

void SpriteBase3D::set_flip_h(bool p_flip)
{
	if (hflip == p_flip) {
		return;
	}

	hflip = p_flip;
	_queue_redraw();
}

bool SpriteBase3D::is_flipped_h() const { return hflip; }

void SpriteBase3D::set_flip_v(bool p_flip)
{
	if (vflip == p_flip) {
		return;
	}

	vflip = p_flip;
	_queue_redraw();
}

bool SpriteBase3D::is_flipped_v() const { return vflip; }

void SpriteBase3D::set_modulate(const Color& p_color)
{
	if (modulate == p_color) {
		return;
	}

	modulate = p_color;
	_propagate_color_changed();
	_queue_redraw();
}

Color SpriteBase3D::get_modulate() const { return modulate; }

void SpriteBase3D::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);

	if (render_priority == p_priority) {
		return;
	}

	render_priority = p_priority;
	_queue_redraw();
}

int SpriteBase3D::get_render_priority() const { return render_priority; }

void SpriteBase3D::set_pixel_size(real_t p_amount)
{
	if (pixel_size == p_amount) {
		return;
	}

	pixel_size = p_amount;
	_queue_redraw();
}

real_t SpriteBase3D::get_pixel_size() const { return pixel_size; }

void SpriteBase3D::set_axis(Vector3::Axis p_axis)
{
	ERR_FAIL_INDEX(p_axis, 3);

	if (axis == p_axis) {
		return;
	}

	axis = p_axis;
	_queue_redraw();
}

Vector3::Axis SpriteBase3D::get_axis() const { return axis; }

void SpriteBase3D::_im_update()
{
	if (!redraw_needed) {
		return;
	}
	_draw();
	redraw_needed = false;

	pending_update = false;
}

AABB SpriteBase3D::get_aabb() const { return aabb; }

Ref<TriangleMesh> SpriteBase3D::generate_triangle_mesh() const
{
	if (triangle_mesh.is_valid()) {
		return triangle_mesh;
	}

	Vector<Vector3> faces;
	faces.resize(6);
	Vector3* facesw = faces.ptrw();

	Rect2 final_rect = get_item_rect();

	if (final_rect.size.x == 0 || final_rect.size.y == 0) {
		return Ref<TriangleMesh>();
	}

	real_t px_size = get_pixel_size();

	Vector2 vertices[4] = {
		(final_rect.position + Vector2(0, final_rect.size.y)) * px_size,
		(final_rect.position + final_rect.size) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, 0)) * px_size,
		final_rect.position * px_size,
	};

	int x_axis = ((axis + 1) % 3);
	int y_axis = ((axis + 2) % 3);

	if (axis != Vector3::AXIS_Z) {
		SWAP(x_axis, y_axis);

		for (int i = 0; i < 4; i++) {
			if (axis == Vector3::AXIS_Y) {
				vertices[i].y = -vertices[i].y;
			}
			else if (axis == Vector3::AXIS_X) {
				vertices[i].x = -vertices[i].x;
			}
		}
	}

	static const int indices[6] = {0, 1, 2, 0, 2, 3};

	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vector3 vtx;
		vtx[x_axis] = vertices[i][0];
		vtx[y_axis] = vertices[i][1];
		facesw[j] = vtx;
	}

	triangle_mesh.instantiate();
	triangle_mesh->create(faces);

	return triangle_mesh;
}

void SpriteBase3D::set_draw_flag(DrawFlags p_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);

	if (flags[p_flag] == p_enable) {
		return;
	}

	flags[p_flag] = p_enable;
	_queue_redraw();
}

bool SpriteBase3D::get_draw_flag(DrawFlags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void SpriteBase3D::set_alpha_cut_mode(AlphaCutMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, ALPHA_CUT_MAX);

	if (alpha_cut == p_mode) {
		return;
	}

	alpha_cut = p_mode;
	_queue_redraw();
}

SpriteBase3D::AlphaCutMode SpriteBase3D::get_alpha_cut_mode() const { return alpha_cut; }

void SpriteBase3D::set_alpha_hash_scale(float p_hash_scale)
{
	if (alpha_hash_scale == p_hash_scale) {
		return;
	}

	alpha_hash_scale = p_hash_scale;
	_queue_redraw();
}

float SpriteBase3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

void SpriteBase3D::set_alpha_scissor_threshold(float p_threshold)
{
	if (alpha_scissor_threshold == p_threshold) {
		return;
	}

	alpha_scissor_threshold = p_threshold;
	_queue_redraw();
}

float SpriteBase3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void SpriteBase3D::set_alpha_antialiasing(BaseMaterial3D::AlphaAntiAliasing p_alpha_aa)
{
	if (alpha_antialiasing_mode == p_alpha_aa) {
		return;
	}

	alpha_antialiasing_mode = p_alpha_aa;
	_queue_redraw();
}

BaseMaterial3D::AlphaAntiAliasing SpriteBase3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void SpriteBase3D::set_alpha_antialiasing_edge(float p_edge)
{
	if (alpha_antialiasing_edge == p_edge) {
		return;
	}

	alpha_antialiasing_edge = p_edge;
	_queue_redraw();
}

float SpriteBase3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

void SpriteBase3D::set_billboard_mode(StandardMaterial3D::BillboardMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, 3); // Cannot use BILLBOARD_PARTICLES.

	if (billboard_mode == p_mode) {
		return;
	}

	billboard_mode = p_mode;
	_queue_redraw();
}

StandardMaterial3D::BillboardMode SpriteBase3D::get_billboard_mode() const
{
	return billboard_mode;
}

void SpriteBase3D::set_texture_filter(StandardMaterial3D::TextureFilter p_filter)
{
	if (texture_filter == p_filter) {
		return;
	}

	texture_filter = p_filter;
	_queue_redraw();
}

StandardMaterial3D::TextureFilter SpriteBase3D::get_texture_filter() const
{
	return texture_filter;
}

SpriteBase3D::~SpriteBase3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RenderingServer::get_singleton()->free_rid(mesh);
	RenderingServer::get_singleton()->free_rid(material);
}

///////////////////////////////////////////

void Sprite3D::_draw()
{
	if (get_base() != get_mesh()) {
		set_base(get_mesh());
	}
	if (texture.is_null()) {
		set_base(RID());
		return;
	}
	Vector2 tsize = texture->get_size();
	if (tsize.x == 0 || tsize.y == 0) {
		return;
	}

	Rect2 base_rect;
	if (region) {
		base_rect = region_rect;
	}
	else {
		base_rect = Rect2(0, 0, texture->get_width(), texture->get_height());
	}

	Size2 frame_size = base_rect.size / Size2(hframes, vframes);
	Point2 frame_offset = Point2(frame % hframes, frame / hframes) * frame_size;

	Point2 dst_offset = get_offset();
	if (is_centered()) {
		dst_offset -= frame_size / 2.0f;
	}

	Rect2 src_rect(base_rect.position + frame_offset, frame_size);
	Rect2 dst_rect(dst_offset, frame_size);

	draw_texture_rect(texture, dst_rect, src_rect);
}

Ref<Texture2D> Sprite3D::get_texture() const { return texture; }

bool Sprite3D::is_region_enabled() const { return region; }

void Sprite3D::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;
	if (region) {
		_queue_redraw();
	}
}

Rect2 Sprite3D::get_region_rect() const { return region_rect; }

int Sprite3D::get_frame() const { return frame; }

void Sprite3D::set_frame_coords(const Vector2i& p_coord)
{
	ERR_FAIL_INDEX(p_coord.x, hframes);
	ERR_FAIL_INDEX(p_coord.y, vframes);

	set_frame(p_coord.y * hframes + p_coord.x);
}

Vector2i Sprite3D::get_frame_coords() const { return Vector2i(frame % hframes, frame / hframes); }

int Sprite3D::get_vframes() const { return vframes; }

int Sprite3D::get_hframes() const { return hframes; }

Rect2 Sprite3D::get_item_rect() const
{
	if (texture.is_null()) {
		return Rect2(0, 0, 1, 1);
	}

	Size2 s;

	if (region) {
		s = region_rect.size;
	}
	else {
		s = texture->get_size();
		s = s / Point2(hframes, vframes);
	}

	Point2 ofs = get_offset();
	if (is_centered()) {
		ofs -= s / 2;
	}

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}

Sprite3D::Sprite3D() {}

void AnimatedSprite3D::_draw()
{
	if (get_base() != get_mesh()) {
		set_base(get_mesh());
	}

	if (frames.is_null() || !frames->has_animation(animation)) {
		return;
	}

	Ref<Texture2D> texture = frames->get_frame_texture(animation, frame);
	if (texture.is_null()) {
		set_base(RID());
		return;
	}
	Size2 tsize = texture->get_size();
	if (tsize.x == 0 || tsize.y == 0) {
		return;
	}

	Rect2 src_rect;
	src_rect.size = tsize;

	Point2 ofs = get_offset();
	if (is_centered()) {
		ofs -= tsize / 2;
	}

	Rect2 dst_rect(ofs, tsize);

	draw_texture_rect(texture, dst_rect, src_rect);
}

Ref<SpriteFrames> AnimatedSprite3D::get_sprite_frames() const { return frames; }

int AnimatedSprite3D::get_frame() const { return frame; }

void AnimatedSprite3D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t AnimatedSprite3D::get_frame_progress() const { return frame_progress; }

void AnimatedSprite3D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float AnimatedSprite3D::get_speed_scale() const { return speed_scale; }

float AnimatedSprite3D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

Rect2 AnimatedSprite3D::get_item_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return Rect2(0, 0, 1, 1);
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return Rect2(0, 0, 1, 1);
	}

	Ref<Texture2D> t;
	if (animation) {
		t = frames->get_frame_texture(animation, frame);
	}
	if (t.is_null()) {
		return Rect2(0, 0, 1, 1);
	}
	Size2 s = t->get_size();

	Point2 ofs = get_offset();
	if (is_centered()) {
		ofs -= s / 2;
	}

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}

bool AnimatedSprite3D::is_playing() const { return playing; }

void AnimatedSprite3D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimatedSprite3D::get_autoplay() const { return autoplay; }

void AnimatedSprite3D::pause() { _stop_internal(false); }

void AnimatedSprite3D::stop() { _stop_internal(true); }

double AnimatedSprite3D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void AnimatedSprite3D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

StringName AnimatedSprite3D::get_animation() const { return animation; }

PackedStringArray AnimatedSprite3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SpriteBase3D::get_configuration_warnings();
	if (frames.is_null()) {
		warnings.push_back(
			RTR("A SpriteFrames resource must be created or set in the \"Sprite Frames\" property "
				"in order for AnimatedSprite3D to display frames."));
	}
	return warnings;
}

#ifdef TOOLS_ENABLED
void AnimatedSprite3D::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && frames.is_valid()) {
		if (pf == "play" || pf == "play_backwards" || pf == "set_animation" ||
			pf == "set_autoplay") {
			List<StringName> al;
			frames->get_animation_list(&al);
			for (const StringName& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	SpriteBase3D::get_argument_options(p_function, p_idx, r_options);
}
#endif

AnimatedSprite3D::AnimatedSprite3D() {}


