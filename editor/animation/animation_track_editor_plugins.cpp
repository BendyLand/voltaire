/**************************************************************************/
/*  animation_track_editor_plugins.cpp                                    */
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

#include "animation_track_editor_plugins.h"
#include "core/io/resource_loader.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/animated_sprite_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/3d/sprite_3d.h"
#include "scene/animation/animation_player.h"
#include "servers/audio/audio_stream.h"
#include "servers/rendering/rendering_server.h"

/// BOOL ///
int AnimationTrackEditBool::get_key_height() const
{
	Ref<Texture2D> checked = get_theme_icon(SNAME("checked"), SNAME("CheckBox"));
	return checked->get_height();
}

Rect2 AnimationTrackEditBool::get_key_rect(int p_index, float p_pixels_sec)
{
	Ref<Texture2D> checked = get_theme_icon(SNAME("checked"), SNAME("CheckBox"));
	return Rect2(-checked->get_width() / 2, 0, checked->get_width(), get_size().height);
}

bool AnimationTrackEditBool::is_key_selectable_by_distance() const { return false; }

/// COLOR ///

int AnimationTrackEditColor::get_key_height() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	return font->get_height(font_size) * 0.8;
}

Rect2 AnimationTrackEditColor::get_key_rect(int p_index, float p_pixels_sec)
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	int fh = font->get_height(font_size) * 0.8;
	return Rect2(-fh / 2, 0, fh, get_size().height);
}

bool AnimationTrackEditColor::is_key_selectable_by_distance() const { return false; }

void AnimationTrackEditColor::draw_key_link(int p_index_from, int p_index_to, float p_pixels_sec,
	int p_x, int p_next_x, int p_clip_left, int p_clip_right)
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	int fh = (font->get_height(font_size) * 0.8);

	fh /= 3;

	int x_from = p_x + fh / 2 - 1;
	int x_to = p_next_x - fh / 2 + 1;
	x_from = MAX(x_from, p_clip_left);
	x_to = MIN(x_to, p_clip_right);

	int y_from = (get_size().height - fh) / 2;

	if (x_from > p_clip_right || x_to < p_clip_left) {
		return;
	}

	Vector<Color> color_samples;

	if (get_animation()->track_get_type(get_track()) == Animation::TYPE_VALUE) {
		if (get_animation()->track_get_interpolation_type(get_track()) !=
				Animation::INTERPOLATION_NEAREST &&
			(get_animation()->value_track_get_update_mode(get_track()) ==
					Animation::UPDATE_CONTINUOUS ||
				get_animation()->value_track_get_update_mode(get_track()) ==
					Animation::UPDATE_CAPTURE) &&
			!Math::is_zero_approx(
				get_animation()->track_get_key_transition(get_track(), p_index_from))) {
			float start_time = get_animation()->track_get_key_time(get_track(), p_index_from);
			float end_time = get_animation()->track_get_key_time(get_track(), p_index_to);
		}
		else {
			color_samples.append(color_samples[0]);
		}
	}

	for (int i = 0; i < color_samples.size() - 1; i++) {
		Vector<Vector2> points = {
			Vector2(Math::lerp(x_from, x_to, float(i) / (color_samples.size() - 1)), y_from),
			Vector2(Math::lerp(x_from, x_to, float(i + 1) / (color_samples.size() - 1)), y_from),
			Vector2(
				Math::lerp(x_from, x_to, float(i + 1) / (color_samples.size() - 1)), y_from + fh),
			Vector2(Math::lerp(x_from, x_to, float(i) / (color_samples.size() - 1)), y_from + fh)};

		Vector<Color> colors = {
			color_samples[i], color_samples[i + 1], color_samples[i + 1], color_samples[i]};

		draw_primitive(points, colors, Vector<Vector2>());
	}
}

bool AnimationTrackEditAudio::is_key_selectable_by_distance() const { return false; }

bool AnimationTrackEditSpriteFrame::is_key_selectable_by_distance() const { return false; }

void AnimationTrackEditSpriteFrame::set_as_coords() { is_coords = true; }

/// SUB ANIMATION ///

bool AnimationTrackEditSubAnim::is_key_selectable_by_distance() const { return false; }

//// VOLUME DB ////

int AnimationTrackEditVolumeDB::get_key_height() const
{
	Ref<Texture2D> volume_texture = get_editor_theme_icon(SNAME("ColorTrackVu"));
	return volume_texture->get_height() * 1.2;
}

void AnimationTrackEditVolumeDB::draw_bg(int p_clip_left, int p_clip_right)
{
	Ref<Texture2D> volume_texture = get_editor_theme_icon(SNAME("ColorTrackVu"));
	int tex_h = volume_texture->get_height();

	int y_from = (get_size().height - tex_h) / 2;
	int y_size = tex_h;

	Color color(1, 1, 1, 0.3);
	draw_texture_rect(volume_texture.ptr(),
		Rect2(p_clip_left, y_from, p_clip_right - p_clip_left, y_from + y_size), false, color);
}

void AnimationTrackEditVolumeDB::draw_fg(int p_clip_left, int p_clip_right)
{
	Ref<Texture2D> volume_texture = get_editor_theme_icon(SNAME("ColorTrackVu"));
	int tex_h = volume_texture->get_height();
	int y_from = (get_size().height - tex_h) / 2;
	int db0 = y_from + (24 / 80.0) * tex_h;

	draw_line(Vector2(p_clip_left, db0), Vector2(p_clip_right, db0), Color(1, 1, 1, 0.3));
}

////////////////////////

/// AUDIO ///

int AnimationTrackEditTypeAudio::get_key_height() const
{
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	return int(font->get_height(font_size) * 1.5);
}

Rect2 AnimationTrackEditTypeAudio::get_key_rect(int p_index, float p_pixels_sec)
{
	Ref<AudioStream> stream = get_animation()->audio_track_get_key_stream(get_track(), p_index);

	if (stream.is_null()) {
		return AnimationTrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	float start_ofs = get_animation()->audio_track_get_key_start_offset(get_track(), p_index);
	float end_ofs = get_animation()->audio_track_get_key_end_offset(get_track(), p_index);

	float len = stream->get_length();

	if (len == 0) {
		Ref<AudioStreamPreview> preview =
			AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);
		len = preview->get_length();
	}

	len -= end_ofs;
	len -= start_ofs;
	if (len <= 0.0001) {
		len = 0.0001;
	}

	if (get_animation()->track_get_key_count(get_track()) > p_index + 1) {
		len = MIN(len, get_animation()->track_get_key_time(get_track(), p_index + 1) -
						   get_animation()->track_get_key_time(get_track(), p_index));
	}

	return Rect2(0, 0, len * p_pixels_sec, get_size().height);
}

bool AnimationTrackEditTypeAudio::is_key_selectable_by_distance() const { return false; }

Control::CursorShape AnimationTrackEditTypeAudio::get_cursor_shape(const Point2& p_pos) const
{
	if (over_drag_position || len_resizing) {
		return Control::CURSOR_HSIZE;
	}
	else {
		return get_default_cursor_shape();
	}
}

////////////////////
/// SUB ANIMATION ///

bool AnimationTrackEditTypeAnimation::is_key_selectable_by_distance() const { return false; }

AnimationTrackEdit* AnimationTrackEditDefaultPlugin::create_audio_track_edit()
{
	return memnew(AnimationTrackEditTypeAudio);
}


