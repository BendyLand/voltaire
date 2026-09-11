/**************************************************************************/
/*  animation_track_editor.cpp                                            */
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

#include "animation_track_editor.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "editor/animation/animation_bezier_editor.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/animation/animation_track_editor_plugins.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/animation/tween.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/view_panner.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/audio/audio_stream.h"

constexpr double FPS_DECIMAL = 1.0;
constexpr double SECOND_DECIMAL = 0.0001;

void AnimationTrackKeyEdit::_bind_methods() {}

void AnimationTrackKeyEdit::_update_obj(const Ref<Animation>& p_anim)
{
	if (setting || animation != p_anim) {
		return;
	}

	notify_change();
}

void AnimationTrackKeyEdit::_key_ofs_changed(const Ref<Animation>& p_anim, float from, float to)
{
	if (animation != p_anim || from != key_ofs) {
		return;
	}

	key_ofs = to;

	if (setting) {
		return;
	}

	notify_change();
}

Node* AnimationTrackKeyEdit::get_root_path() { return root_path; }

void AnimationMultiTrackKeyEdit::_bind_methods() {}

void AnimationMultiTrackKeyEdit::_update_obj(const Ref<Animation>& p_anim)
{
	if (setting || animation != p_anim) {
		return;
	}

	notify_change();
}

void AnimationMultiTrackKeyEdit::_key_ofs_changed(
	const Ref<Animation>& p_anim, float from, float to)
{
	if (animation != p_anim) {
		return;
	}

	for (const KeyValue<int, List<float>>& E : key_ofs_map) {
		int key = 0;
		for (const float& key_ofs : E.value) {
			if (from != key_ofs) {
				key++;
				continue;
			}

			int track = E.key;
			key_ofs_map[track].get(key) = to;

			if (setting) {
				return;
			}

			notify_change();

			return;
		}
	}
}

Node* AnimationMultiTrackKeyEdit::get_root_path() { return root_path; }

float AnimationTimelineEdit::get_zoom_scale() const { return _get_zoom_scale(zoom->get_value()); }

float AnimationTimelineEdit::_get_zoom_scale(double p_zoom_value) const
{
	float zv = zoom->get_max() - p_zoom_value;
	if (zv < 1) {
		zv = 1.0 - zv;
		return Math::pow(1.0f + zv, 8.0f) * 100;
	}
	else {
		return 1.0 / Math::pow(zv, 8.0f) * 100;
	}
}

int AnimationTimelineEdit::get_buttons_width() const
{
	const Ref<Texture2D> interp_mode = get_editor_theme_icon(SNAME("TrackContinuous"));
	const Ref<Texture2D> interp_type = get_editor_theme_icon(SNAME("InterpRaw"));
	const Ref<Texture2D> loop_type = get_editor_theme_icon(SNAME("InterpWrapClamp"));
	const Ref<Texture2D> remove_icon = get_editor_theme_icon(SNAME("Remove"));
	const Ref<Texture2D> down_icon = get_theme_icon(SNAME("select_arrow"), SNAME("Tree"));

	const int h_separation = get_theme_constant(SNAME("h_separation"), SNAME("AnimationTrackEdit"));
	const int outer_margin = get_theme_constant(SNAME("outer_margin"), SNAME("AnimationTrackEdit"));

	int total_w = interp_mode->get_width() + interp_type->get_width() + loop_type->get_width() +
				  remove_icon->get_width() + outer_margin;
	total_w += (down_icon->get_width() + h_separation) * 4;

	return total_w;
}

int AnimationTimelineEdit::get_name_limit() const
{
	Ref<Texture2D> hsize_icon = get_editor_theme_icon(SNAME("Hsize"));

	int filter_track_width =
		filter_track->is_visible() ? filter_track->get_custom_minimum_size().width : 0;
	int limit = MAX(name_limit, add_track->get_minimum_size().width + hsize_icon->get_width() +
									filter_track_width + 16 * EDSCALE);

	limit = MIN(limit, get_size().width - get_buttons_width() - 1);

	return limit;
}

void AnimationTimelineEdit::set_animation(const Ref<Animation>& p_animation, bool p_read_only)
{
	animation = p_animation;
	read_only = p_read_only;

	length->set_read_only(read_only);

	if (animation.is_valid()) {
		len_hb->show();
		filter_track->show();
		if (read_only) {
			add_track->hide();
		}
		else {
			add_track->show();
		}
		play_position->show();
	}
	else {
		len_hb->hide();
		filter_track->hide();
		add_track->hide();
		play_position->hide();
	}
	queue_redraw();
}

Size2 AnimationTimelineEdit::get_minimum_size() const
{
	Size2 ms = filter_track->get_minimum_size();
	const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	ms.height = MAX(ms.height, font->get_height(font_size));
	ms.width = get_buttons_width() + add_track->get_minimum_size().width +
			   get_editor_theme_icon(SNAME("Hsize"))->get_width() + 2 + 8 * EDSCALE;
	return ms;
}

void AnimationTimelineEdit::auto_fit()
{
	if (animation.is_null()) {
		return;
	}

	float anim_end = animation->get_length();
	float anim_start = 0;

	// Search for keyframe outside animation boundaries to include keyframes before animation start
	// and after animation length.
	int track_count = animation->get_track_count();
	for (int track = 0; track < track_count; ++track) {
		for (int i = 0; i < animation->track_get_key_count(track); i++) {
			float key_time = animation->track_get_key_time(track, i);
			if (key_time > anim_end) {
				anim_end = key_time;
			}
			if (key_time < anim_start) {
				anim_start = key_time;
			}
		}
	}

	float anim_length = anim_end - anim_start;
	int timeline_width_pixels = get_size().width - get_buttons_width() - get_name_limit();

	// I want a little buffer at the end... (5% looks nice and we should keep some space for the
	// bezier handles)
	timeline_width_pixels *= 0.95;

	// The technique is to reuse the _get_zoom_scale function directly to be sure that the auto_fit
	// is always calculated the same way as the zoom slider. It's a little bit more calculation then
	// doing the inverse of get_zoom_scale but it's really easier to understand and should always be
	// accurate.
	float new_zoom = zoom->get_max();
	while (true) {
		double test_zoom_scale = _get_zoom_scale(new_zoom);

		if (anim_length * test_zoom_scale <= timeline_width_pixels) {
			// It fits...
			break;
		}

		new_zoom -= zoom->get_step();

		if (new_zoom <= zoom->get_min()) {
			new_zoom = zoom->get_min();
			break;
		}
	}

	// Horizontal scroll to get_min which should include keyframes that are before the animation
	// start.
	hscroll->set_value(hscroll->get_min());
	// Set the zoom value... the signal value_changed will be emitted and the timeline will be

	// refreshed correctly!
	zoom->set_value(new_zoom);
}

void AnimationTimelineEdit::_scroll_to_start()
{
	// Horizontal scroll to get_min which should include keyframes that are before the animation
	// start.
	hscroll->set_value(hscroll->get_min());
}

void AnimationTimelineEdit::set_track_edit(AnimationTrackEdit* p_track_edit)
{
	track_edit = p_track_edit;
}

void AnimationTimelineEdit::set_editor(AnimationTrackEditor* p_editor) { editor = p_editor; }

void AnimationTimelineEdit::set_play_position(float p_pos)
{
	play_position_pos = p_pos;
	play_position->queue_redraw();
}

float AnimationTimelineEdit::get_play_position() const { return play_position_pos; }

void AnimationTimelineEdit::update_play_position() { play_position->queue_redraw(); }

void AnimationTimelineEdit::_play_position_draw()
{
	if (animation.is_null() || play_position_pos < 0) {
		return;
	}

	float scale = get_zoom_scale();
	int px = (-get_value() + play_position_pos) * scale + get_name_limit();

	if (px >= get_name_limit() && px < (play_position->get_size().width - get_buttons_width())) {
		int h = editor->box_selection_container->get_global_position().y - get_global_position().y;
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));

		play_position->draw_line(Point2(px, 0), Point2(px, h), color, Math::round(2 * EDSCALE));
		play_position->draw_texture(get_editor_theme_icon(SNAME("TimelineIndicator")).ptr(),
			Point2(px - get_editor_theme_icon(SNAME("TimelineIndicator"))->get_width() * 0.5, 0),
			color);
	}
}

void AnimationTimelineEdit::_stop_dragging()
{
	dragging_hsize = false;
	dragging_timeline = false;
}

Control::CursorShape AnimationTimelineEdit::get_cursor_shape(const Point2& p_pos) const
{
	if (dragging_hsize || resizing_timeline || hsize_rect.has_point(p_pos) ||
		timeline_resize_rect.has_point(p_pos)) {
		// Indicate that the track name column's width and the timeline length can be adjusted.
		return Control::CURSOR_HSIZE;
	}
	else {
		return get_default_cursor_shape();
	}
}

void AnimationTimelineEdit::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	set_value(get_value() - p_scroll_vec.x / get_zoom_scale());
}

void AnimationTimelineEdit::_zoom_callback(
	float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	double current_zoom_value = get_zoom()->get_value();
	zoom_scroll_origin = p_origin;
	zoom_callback_occurred = true;
	get_zoom()->set_value(MAX(0.01, current_zoom_value - (1.0 - p_zoom_factor)));
}

void AnimationTimelineEdit::set_use_fps(bool p_use_fps)
{
	use_fps = p_use_fps;
	queue_redraw();
}

bool AnimationTimelineEdit::is_using_fps() const { return use_fps; }

void AnimationTimelineEdit::set_hscroll(HScrollBar* p_hscroll) { hscroll = p_hscroll; }

void AnimationTimelineEdit::_bind_methods() {}

////////////////////////////////////

int AnimationTrackEdit::get_key_height() const
{
	if (animation.is_null()) {
		return 0;
	}

	return type_icon->get_height();
}

Rect2 AnimationTrackEdit::get_key_rect(int p_index, float p_pixels_sec)
{
	if (animation.is_null()) {
		return Rect2();
	}
	Rect2 rect = Rect2(-type_icon->get_width() / 2, 0, type_icon->get_width(), get_size().height);

	// Make it a big easier to click.
	rect.position.x -= rect.size.x * 0.5;
	rect.size.x *= 2;
	return rect;
}

bool AnimationTrackEdit::is_key_selectable_by_distance() const { return true; }

// Helper.
void AnimationTrackEdit::draw_rect_clipped(const Rect2& p_rect, const Color& p_color, bool p_filled)
{
	int clip_left = timeline->get_name_limit();
	int clip_right = get_size().width - timeline->get_buttons_width();

	if (p_rect.position.x > clip_right) {
		return;
	}
	if (p_rect.position.x + p_rect.size.x < clip_left) {
		return;
	}
	Rect2 clip = Rect2(clip_left, 0, clip_right - clip_left, get_size().height);
	draw_rect(clip.intersection(p_rect), p_color, p_filled);
}

void AnimationTrackEdit::draw_bg(int p_clip_left, int p_clip_right) {}

void AnimationTrackEdit::draw_fg(int p_clip_left, int p_clip_right) {}

void AnimationTrackEdit::draw_texture_region_clipped(
	const Ref<Texture2D>& p_texture, const Rect2& p_rect, const Rect2& p_region)
{
	int clip_left = timeline->get_name_limit();
	int clip_right = get_size().width - timeline->get_buttons_width();

	// Clip left and right.
	if (clip_left > p_rect.position.x + p_rect.size.x) {
		return;
	}
	if (clip_right < p_rect.position.x) {
		return;
	}

	Rect2 rect = p_rect;
	Rect2 region = p_region;

	if (clip_left > rect.position.x) {
		int rect_pixels = (clip_left - rect.position.x);
		int region_pixels = rect_pixels * region.size.x / rect.size.x;

		rect.position.x += rect_pixels;
		rect.size.x -= rect_pixels;

		region.position.x += region_pixels;
		region.size.x -= region_pixels;
	}

	if (clip_right < rect.position.x + rect.size.x) {
		int rect_pixels = rect.position.x + rect.size.x - clip_right;
		int region_pixels = rect_pixels * region.size.x / rect.size.x;

		rect.size.x -= rect_pixels;
		region.size.x -= region_pixels;
	}

	draw_texture_rect_region(p_texture.ptr(), rect, region);
}

int AnimationTrackEdit::get_track() const { return track; }

Ref<Animation> AnimationTrackEdit::get_animation() const { return animation; }

void AnimationTrackEdit::set_animation_and_track(
	const Ref<Animation>& p_animation, int p_track, bool p_read_only)
{
	animation = p_animation;
	read_only = p_read_only;

	track = p_track;
	queue_redraw();

	ERR_FAIL_INDEX(track, animation->get_track_count());

	node_path = animation->track_get_path(p_track);
	type_icon = _get_key_type_icon();
	selected_icon = get_editor_theme_icon(SNAME("KeySelected"));
}

NodePath AnimationTrackEdit::get_path() const { return node_path; }

Size2 AnimationTrackEdit::get_minimum_size() const
{
	Ref<Texture2D> texture = get_editor_theme_icon(SNAME("Object"));
	const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	const int separation = get_theme_constant(SNAME("v_separation"), SNAME("ItemList"));

	int max_h = MAX(texture->get_height(), font->get_height(font_size));
	max_h = MAX(max_h, get_key_height());

	return Vector2(1, max_h + separation);
}

void AnimationTrackEdit::set_editor(AnimationTrackEditor* p_editor) { editor = p_editor; }

void AnimationTrackEdit::_play_position_draw()
{
	if (animation.is_null() || play_position_pos < 0) {
		return;
	}

	float scale = timeline->get_zoom_scale();
	int h = get_size().height;

	int px = (-timeline->get_value() + play_position_pos) * scale + timeline->get_name_limit();

	if (px >= timeline->get_name_limit() &&
		px < (get_size().width - timeline->get_buttons_width())) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		play_position->draw_line(Point2(px, 0), Point2(px, h), color, Math::round(2 * EDSCALE));
	}
}

void AnimationTrackEdit::set_play_position(float p_pos)
{
	play_position_pos = p_pos;
	play_position->queue_redraw();
}

void AnimationTrackEdit::update_play_position() { play_position->queue_redraw(); }

void AnimationTrackEdit::set_root(Node* p_root) { root = p_root; }

void AnimationTrackEdit::_zoom_changed()
{
	queue_redraw();
	play_position->queue_redraw();
}

Ref<Texture2D> AnimationTrackEdit::_get_key_type_icon() const
{
	const Ref<Texture2D> type_icons[9] = {get_editor_theme_icon(SNAME("KeyValue")),
		get_editor_theme_icon(SNAME("KeyTrackPosition")),
		get_editor_theme_icon(SNAME("KeyTrackRotation")),
		get_editor_theme_icon(SNAME("KeyTrackScale")),
		get_editor_theme_icon(SNAME("KeyTrackBlendShape")), get_editor_theme_icon(SNAME("KeyCall")),
		get_editor_theme_icon(SNAME("KeyBezier")), get_editor_theme_icon(SNAME("KeyAudio")),
		get_editor_theme_icon(SNAME("KeyAnimation"))};
	return type_icons[animation->track_get_type(track)];
}

Control::CursorShape AnimationTrackEdit::get_cursor_shape(const Point2& p_pos) const
{
	if (command_or_control_pressed && animation->track_get_type(track) == Animation::TYPE_METHOD &&
		hovering_key_idx != -1) {
		return Control::CURSOR_POINTING_HAND;
	}
	return get_default_cursor_shape();
}

void AnimationTrackEdit::cancel_drop()
{
	if (dropping_at != 0) {
		dropping_at = 0;
		queue_redraw();
	}
}

void AnimationTrackEdit::set_in_group(bool p_enable)
{
	in_group = p_enable;
	queue_redraw();
}

///////////////////////////////////////

void AnimationTrackEditGroup::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		icon_size =
			Vector2(1, 1) * get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));
	} break;

	case NOTIFICATION_DRAW: {
		const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
		Color color = get_theme_color(SceneStringName(font_color), SNAME("Label"));

		const Ref<StyleBox>& stylebox_header =
			get_theme_stylebox(SNAME("header"), SNAME("AnimationTrackEditGroup"));
		float v_margin_offset = stylebox_header->get_content_margin(SIDE_TOP) -
								stylebox_header->get_content_margin(SIDE_BOTTOM);

		const Color h_line_color =
			get_theme_color(SNAME("h_line_color"), SNAME("AnimationTrackEditGroup"));
		const Color v_line_color =
			get_theme_color(SNAME("v_line_color"), SNAME("AnimationTrackEditGroup"));
		const int h_separation =
			get_theme_constant(SNAME("h_separation"), SNAME("AnimationTrackEditGroup"));

		const Ref<StyleBox>& stylebox_hover =
			get_theme_stylebox(SceneStringName(hover), SNAME("AnimationTrackEditGroup"));

		if (root) {
			Node* n = root->get_node_or_null(node);
			if (n && EditorNode::get_singleton()->get_editor_selection()->is_selected(n)) {
				color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			}
		}

		draw_style_box(stylebox_header.ptr(), Rect2(Point2(), get_size()));

		if (hovered) {
			// Draw hover feedback for AnimationTrackEditGroup.
			// Add a limit to just show hover over portion with text.
			int limit = timeline->get_name_limit();
			draw_style_box(stylebox_hover.ptr(),
				Rect2(Point2(1 * EDSCALE, 0), Size2(limit - 1 * EDSCALE, get_size().height)));
		}

		int limit = timeline->get_name_limit();
		int limit_end = get_size().width - timeline->get_buttons_width();

		// Unavailable timeline.

		{
			int px = (editor->get_current_animation()->get_length() - timeline->get_value()) *
						 timeline->get_zoom_scale() +
					 timeline->get_name_limit();
			px = MAX(px, timeline->get_name_limit());
			Rect2 rect = Rect2(px, 0, limit_end - px, get_size().height);
			if (rect.size.width > 0) {
				draw_rect(rect, Color(0, 0, 0, 0.2));
			}
		}

		// Section preview.

		{
			float scale = timeline->get_zoom_scale();

			PackedStringArray section = editor->get_selected_section();
			if (section.size() == 2) {
				StringName start_marker = section[0];
				StringName end_marker = section[1];
				double start_time = editor->get_current_animation()->get_marker_time(start_marker);
				double end_time = editor->get_current_animation()->get_marker_time(end_marker);

				AnimationPlayer* player = AnimationPlayerEditor::get_singleton()->get_player();
				// When AnimationPlayer is playing, don't move the preview rect, so it still
				// indicates the playback section.
				if (editor->is_marker_moving_selection() && !(player && player->is_playing())) {
					start_time += editor->get_marker_moving_selection_offset();
					end_time += editor->get_marker_moving_selection_offset();
				}

				if (start_time < editor->get_current_animation()->get_length() && end_time >= 0) {
					float start_ofs = MAX(0, start_time) - timeline->get_value();
					float end_ofs = MIN(editor->get_current_animation()->get_length(), end_time) -
									timeline->get_value();
					start_ofs = start_ofs * scale + limit;
					end_ofs = end_ofs * scale + limit;
					start_ofs = MAX(start_ofs, limit);
					end_ofs = MIN(end_ofs, limit_end);
					Rect2 rect;
					rect.set_position(Vector2(start_ofs, 0));
					rect.set_size(Vector2(end_ofs - start_ofs, get_size().height));

					draw_rect(rect, Color(1, 0.1, 0.1, 0.2));
				}
			}
		}

		// Marker overlays.

		{
			float scale = timeline->get_zoom_scale();
			PackedStringArray markers = editor->get_current_animation()->get_marker_names();
			for (const StringName marker : markers) {
				double time = editor->get_current_animation()->get_marker_time(marker);
				if (editor->is_marker_selected(marker) && editor->is_marker_moving_selection()) {
					time += editor->get_marker_moving_selection_offset();
				}
				if (time >= 0) {
					float offset = time - timeline->get_value();
					offset = offset * scale + limit;
					if (offset >= timeline->get_name_limit() && offset < limit_end) {
						Color marker_color =
							editor->get_current_animation()->get_marker_color(marker);
						marker_color.a = 0.2;
						draw_line(Point2(offset, 0), Point2(offset, get_size().height),
							marker_color, Math::round(EDSCALE));
					}
				}
			}
		}

		draw_line(Point2(), Point2(get_size().width, 0), h_line_color, Math::round(EDSCALE));
		draw_line(Point2(timeline->get_name_limit(), 0),
			Point2(timeline->get_name_limit(), get_size().height), v_line_color,
			Math::round(EDSCALE));
		draw_line(Point2(get_size().width - timeline->get_buttons_width(), 0),
			Point2(get_size().width - timeline->get_buttons_width(), get_size().height),
			v_line_color, Math::round(EDSCALE));

		int ofs = stylebox_header->get_margin(SIDE_LEFT);
		bool is_group_folded = editor->get_current_animation()->editor_is_group_folded(node_name);
		Ref<Texture2D> fold_icon = get_theme_icon(
			is_group_folded ? SNAME("arrow_collapsed") : SNAME("arrow"), SNAME("Tree"));
		Size2 fold_icon_size = fold_icon->get_size();
		draw_texture_rect(fold_icon.ptr(),
			Rect2(Point2(ofs, (get_size().height - fold_icon_size.y) / 2 + v_margin_offset).round(),
				fold_icon_size));

		ofs += h_separation + fold_icon_size.x;
		draw_texture_rect(icon.ptr(),
			Rect2(Point2(ofs, (get_size().height - icon_size.y) / 2 + v_margin_offset).round(),
				icon_size));

		ofs += h_separation + icon_size.x;
		draw_string(font.ptr(),
			Point2(ofs, (get_size().height - font->get_height(font_size)) / 2 +
							font->get_ascent(font_size) + v_margin_offset)
				.round(),
			node_name, HORIZONTAL_ALIGNMENT_LEFT, timeline->get_name_limit() - ofs, font_size,
			color);

		int px =
			(-timeline->get_value() + timeline->get_play_position()) * timeline->get_zoom_scale() +
			timeline->get_name_limit();
		if (px >= timeline->get_name_limit() && px < limit_end) {
			const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			draw_line(
				Point2(px, 0), Point2(px, get_size().height), accent, Math::round(2 * EDSCALE));
		}

		if (is_group_folded) {
			for (const AnimationTrackEdit* track_edit : track_edits) {
				const Ref<Texture2D>& key_type_icon = track_edit->get_key_type_icon();
				int track = track_edit->get_track();
				for (int i = 0; i < editor->get_current_animation()->track_get_key_count(track);
					 ++i) {
					float key_time_offset =
						editor->get_current_animation()->track_get_key_time(track, i) -
						timeline->get_value();
					int key_screen_pos = int(key_time_offset * timeline->get_zoom_scale() + limit);
					int key_limit_left = timeline->get_name_limit();
					int key_limit_right = get_size().width - timeline->get_buttons_width();
					if (key_screen_pos >= key_limit_left && key_screen_pos <= key_limit_right) {
						draw_texture(key_type_icon.ptr(),
							Vector2(key_screen_pos - key_type_icon->get_width() / 2,
								(get_size().height - key_type_icon->get_height()) / 2),
							Color(1, 1, 1, 0.3));
					}
				}
			}
		}
	} break;

	case NOTIFICATION_MOUSE_EXIT: {
		if (hovered) {
			hovered = false;
			// When the mouse cursor exits the AnimationTrackEditGroup, we're no longer hovering
			// the group.
			queue_redraw();
		}
	} break;
	}
}

void AnimationTrackEditGroup::set_type_and_name(
	const Ref<Texture2D>& p_type, const String& p_name, const NodePath& p_node)
{
	icon = p_type;
	node_name = p_name;
	node = p_node;
	queue_redraw();
	update_minimum_size();
}

Size2 AnimationTrackEditGroup::get_minimum_size() const
{
	const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	const int separation = get_theme_constant(SNAME("v_separation"), SNAME("ItemList"));

	const Ref<StyleBox>& header_style =
		get_theme_stylebox(SNAME("header"), SNAME("AnimationTrackEditGroup"));
	const int content_margin =
		header_style->get_content_margin(SIDE_TOP) + header_style->get_content_margin(SIDE_BOTTOM);

	return Vector2(0, MAX(font->get_height(font_size), icon_size.y) + separation + content_margin);
}

String AnimationTrackEditGroup::get_node_name() const { return node_name; }

void AnimationTrackEditGroup::set_root(Node* p_root)
{
	root = p_root;
	queue_redraw();
}

void AnimationTrackEditGroup::set_editor(AnimationTrackEditor* p_editor) { editor = p_editor; }

void AnimationTrackEditGroup::_zoom_changed() { queue_redraw(); }

AnimationTrackEditGroup::AnimationTrackEditGroup() { set_mouse_filter(MOUSE_FILTER_PASS); }

//////////////////////////////////////

void AnimationTrackEditor::add_track_edit_plugin(const Ref<AnimationTrackEditPlugin>& p_plugin)
{
	if (track_edit_plugins.has(p_plugin)) {
		return;
	}
	track_edit_plugins.push_back(p_plugin);
}

void AnimationTrackEditor::remove_track_edit_plugin(const Ref<AnimationTrackEditPlugin>& p_plugin)
{
	track_edit_plugins.erase(p_plugin);
}

void AnimationTrackEditor::_check_bezier_exist()
{
	bool is_exist = false;
	if (animation.is_valid()) {
		for (int i = 0; i < animation->get_track_count(); i++) {
			if (animation->track_get_type(i) == Animation::TrackType::TYPE_BEZIER) {
				is_exist = true;
				break;
			}
		}
	}
	if (is_exist) {
		bezier_edit_icon->set_disabled(false);
	}
	else {
		if (bezier_mc->is_visible()) {
			_cancel_bezier_edit();
		}
		bezier_edit_icon->set_disabled(true);
	}
}

Ref<Animation> AnimationTrackEditor::get_current_animation() const { return animation; }

void AnimationTrackEditor::_root_removed() { root = nullptr; }

Node* AnimationTrackEditor::get_root() const { return root; }

bool AnimationTrackEditor::has_keying() const { return keying; }

void AnimationTrackEditor::_name_limit_changed() { _redraw_tracks(); }

static bool track_type_is_resettable(Animation::TrackType p_type)
{
	switch (p_type) {
	case Animation::TYPE_VALUE:
		[[fallthrough]];
	case Animation::TYPE_BLEND_SHAPE:
		[[fallthrough]];
	case Animation::TYPE_BEZIER:
		[[fallthrough]];
	case Animation::TYPE_POSITION_3D:
		[[fallthrough]];
	case Animation::TYPE_ROTATION_3D:
		[[fallthrough]];
	case Animation::TYPE_SCALE_3D:
		return true;
	default:
		return false;
	}
}

bool AnimationTrackEditor::is_read_only() const { return read_only; }

void AnimationTrackEditor::make_insert_queue()
{
	insert_data.clear();
	insert_queue = true;
}

bool AnimationTrackEditor::has_transform_3d_track(
	Node3D* p_node, const String& p_sub, const Animation::TrackType p_type)
{
	ERR_FAIL_NULL_V(root, false);
	if (!keying) {
		return false;
	}
	if (animation.is_null()) {
		return false;
	}

	// Let's build a node path.
	String path = String(root->get_path_to(p_node, true));
	if (!p_sub.is_empty()) {
		path += ":" + p_sub;
	}

	int track_id = animation->find_track(path, p_type);
	if (track_id >= 0) {
		return true;
	}
	return false;
}

PackedStringArray AnimationTrackEditor::get_selected_section() const
{
	return marker_edit->get_selected_section();
}

bool AnimationTrackEditor::is_marker_selected(const StringName& p_marker) const
{
	return marker_edit->is_marker_selected(p_marker);
}

bool AnimationTrackEditor::is_marker_moving_selection() const
{
	return marker_edit->is_moving_selection();
}

float AnimationTrackEditor::get_marker_moving_selection_offset() const
{
	return marker_edit->get_moving_selection_offset();
}

void AnimationTrackEditor::show_select_node_warning(bool p_show)
{
	info_message_vbox->set_visible(p_show);
}

void AnimationTrackEditor::show_dummy_player_warning(bool p_show)
{
	dummy_player_warning->set_visible(p_show);
}

void AnimationTrackEditor::show_inactive_player_warning(bool p_show)
{
	inactive_player_warning->set_visible(p_show);
}

bool AnimationTrackEditor::is_key_selected(int p_track, int p_key) const
{
	SelectedKey sk;
	sk.key = p_key;
	sk.track = p_track;

	return selection.has(sk);
}

bool AnimationTrackEditor::is_selection_active() const { return selection.size(); }

bool AnimationTrackEditor::is_key_clipboard_active() const { return key_clipboard.keys.size(); }

bool AnimationTrackEditor::is_snap_timeline_enabled() const
{
	return snap_timeline->is_pressed() ^ Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL);
}

bool AnimationTrackEditor::is_snap_keys_enabled() const
{
	return snap_keys->is_pressed() ^ Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL);
}

bool AnimationTrackEditor::is_insert_at_current_time_enabled() const
{
	return insert_at_current_time->is_pressed();
}

void AnimationTrackEditor::resolve_insertion_offset(float& r_offset) const
{
	if (is_insert_at_current_time_enabled()) {
		r_offset = timeline->get_play_position();
	}
}

bool AnimationTrackEditor::is_bezier_editor_active() const { return bezier_mc->is_visible(); }

void AnimationTrackEditor::_redraw_tracks()
{
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->queue_redraw();
	}
}

void AnimationTrackEditor::_redraw_groups()
{
	for (int i = 0; i < groups.size(); i++) {
		groups[i]->queue_redraw();
	}
}

void AnimationTrackEditor::_update_fps_compat_mode(bool p_enabled) { _update_snap_unit(); }

void AnimationTrackEditor::_update_nearest_fps_label()
{
	bool is_fps_invalid = nearest_fps == 0;
	if (is_fps_invalid) {
		nearest_fps_label->hide();
	}
	else {
		nearest_fps_label->show();
		nearest_fps_label->set_text(vformat(TTR("Nearest FPS: %d"), nearest_fps));
	}
}

MenuButton* AnimationTrackEditor::get_edit_menu() { return edit; }

void AnimationTrackEditor::_update_scroll(double)
{
	_redraw_tracks();
	_redraw_groups();
	marker_edit->queue_redraw();
}

void AnimationTrackEditor::_add_track(int p_type)
{
	AnimationPlayer* ap = AnimationPlayerEditor::get_singleton()->get_player();
	if (!ap) {
		ERR_FAIL_EDMSG("No AnimationPlayer is currently being edited.");
	}
	Node* root_node = ap->get_node_or_null(ap->get_root_node());
	if (!root_node) {
		EditorNode::get_singleton()->show_warning(
			TTR("Not possible to add a new track without a root"));
		return;
	}
	adding_track_type = p_type;

	String title_text = TTRC("Pick a node to animate:");
	Vector<StringName> valid_types;
	switch (adding_track_type) {
	case Animation::TYPE_BLEND_SHAPE: {
		// Blend Shape is a property of MeshInstance3D.
		valid_types.push_back(SNAME("MeshInstance3D"));
	} break;
	case Animation::TYPE_POSITION_3D:
	case Animation::TYPE_ROTATION_3D:
	case Animation::TYPE_SCALE_3D: {
		// 3D Properties come from nodes inheriting Node3D.
		valid_types.push_back(SNAME("Node3D"));
	} break;
	case Animation::TYPE_METHOD: {
		title_text = TTRC("Pick a node to select method:");
	} break;
	case Animation::TYPE_AUDIO: {
		valid_types.push_back(SNAME("AudioStreamPlayer"));
		valid_types.push_back(SNAME("AudioStreamPlayer2D"));
		valid_types.push_back(SNAME("AudioStreamPlayer3D"));
		title_text = TTRC("Pick a node to play audio:");
	} break;
	case Animation::TYPE_ANIMATION: {
		valid_types.push_back(SNAME("AnimationPlayer"));
		title_text = TTRC("Pick a node to play animation:");
	} break;
	}
	pick_track->set_valid_types(valid_types);
	pick_track->set_title(title_text);
	pick_track->popup_scenetree_dialog(nullptr, root_node);
	pick_track->get_filter_line_edit()->clear();
	pick_track->get_filter_line_edit()->grab_focus();
}

void AnimationTrackEditor::_timeline_value_changed(double)
{
	timeline->update_play_position();

	_redraw_tracks();
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update_play_position();
	}
	_redraw_groups();

	bezier_edit->queue_redraw();
	bezier_edit->update_play_position();

	marker_edit->update_play_position();
}

int AnimationTrackEditor::_get_track_selected()
{
	for (int i = 0; i < track_edits.size(); i++) {
		if (track_edits[i]->has_focus()) {
			return track_edits[i]->get_track();
		}
	}

	return -1;
}

void AnimationTrackEditor::_move_selection_begin()
{
	moving_selection = true;
	moving_selection_offset = 0;
}

void AnimationTrackEditor::_move_selection(float p_offset)
{
	moving_selection_offset = p_offset;
	_redraw_tracks();
}

struct _AnimMoveRestore
{
	int track = 0;
	float time = 0;
	float transition = 0;
};

void AnimationTrackEditor::_clear_selection(bool p_update)
{
	selection.clear();

	if (p_update) {
		_redraw_tracks();
	}

	_clear_key_edit();
}

void AnimationTrackEditor::_clear_selection_for_anim(const Ref<Animation>& p_anim)
{
	if (animation != p_anim) {
		return;
	}

	_clear_selection();
}

void AnimationTrackEditor::_move_selection_cancel()
{
	moving_selection = false;
	_redraw_tracks();
}

bool AnimationTrackEditor::is_moving_selection() const { return moving_selection; }

float AnimationTrackEditor::get_moving_selection_offset() const { return moving_selection_offset; }

void AnimationTrackEditor::_box_selection_draw()
{
	const Rect2 selection_rect = Rect2(Point2(), box_selection->get_size());
	box_selection->draw_rect(selection_rect,
		get_theme_color(SNAME("box_selection_fill_color"), EditorStringName(Editor)));
	box_selection->draw_rect(selection_rect,
		get_theme_color(SNAME("box_selection_stroke_color"), EditorStringName(Editor)), false,
		Math::round(EDSCALE));
}

void AnimationTrackEditor::_toggle_bezier_edit()
{
	if (bezier_mc->is_visible()) {
		_cancel_bezier_edit();
	}
	else {
		int track_count = animation->get_track_count();
		for (int i = 0; i < track_count; ++i) {
			if (animation->track_get_type(i) == Animation::TrackType::TYPE_BEZIER) {
				_bezier_edit(i);
				return;
			}
		}
	}
}

void AnimationTrackEditor::_scroll_changed(const Vector2& p_val)
{
	if (box_selecting) {
		const Vector2 scroll_difference = p_val - prev_scroll_position;

		Vector2 from = box_selecting_from - scroll_difference;
		Vector2 to = box_selecting_to;

		box_selecting_from = from;

		if (from.x > to.x) {
			SWAP(from.x, to.x);
		}

		if (from.y > to.y) {
			SWAP(from.y, to.y);
		}

		Rect2 rect(from, to - from);
		box_selection->set_rect(Rect2(from - scroll->get_global_position(), rect.get_size()));
		box_select_rect = rect;
	}

	prev_scroll_position = p_val;
}

void AnimationTrackEditor::_v_scroll_changed(float p_val)
{
	_scroll_changed(Vector2(prev_scroll_position.x, p_val));
}

void AnimationTrackEditor::_h_scroll_changed(float p_val)
{
	_scroll_changed(Vector2(p_val, prev_scroll_position.y));
}

void AnimationTrackEditor::_zoom_callback(
	float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	timeline->_zoom_callback(p_zoom_factor, p_origin, p_event);
}

void AnimationTrackEditor::_cancel_bezier_edit()
{
	bezier_mc->hide();
	box_selection_container->show();
	bezier_edit_icon->set_pressed(false);
	auto_fit->show();
	auto_fit_bezier->hide();
}

void AnimationTrackEditor::_bezier_edit(int p_for_track)
{
	_clear_selection(); // Bezier probably wants to use a separate selection mode.
	bezier_edit->set_root(root);
	bezier_edit->set_animation_and_track(animation, p_for_track, read_only);
	box_selection_container->hide();
	bezier_mc->show();
	auto_fit->hide();
	auto_fit_bezier->show();
	// Search everything within the track and curve - edit it.
}

void AnimationTrackEditor::_bezier_track_set_key_handle_mode(Animation* p_anim, int p_track,
	int p_index, Animation::HandleMode p_mode, Animation::HandleSetMode p_set_mode)
{
	ERR_FAIL_NULL(p_anim);
	p_anim->bezier_track_set_key_handle_mode(p_track, p_index, p_mode, p_set_mode);
}

void AnimationTrackEditor::_bezier_track_set_key_handle_mode_at_time(Animation* p_anim, int p_track,
	float p_time, Animation::HandleMode p_mode, Animation::HandleSetMode p_set_mode)
{
	ERR_FAIL_NULL(p_anim);
	int index = p_anim->track_find_key(p_track, p_time, Animation::FIND_MODE_APPROX);
	ERR_FAIL_COND(index < 0);
	_bezier_track_set_key_handle_mode(p_anim, p_track, index, p_mode, p_set_mode);
}

void AnimationTrackEditor::_toggle_function_names() { _redraw_tracks(); }

bool AnimationTrackEditor::is_grouping_tracks()
{
	if (!view_group) {
		return false;
	}

	return !view_group->is_pressed();
}

bool AnimationTrackEditor::is_sorting_alphabetically() { return alphabetic_sorting->is_pressed(); }

bool AnimationTrackEditor::is_function_name_pressed()
{
	return function_name_toggler->is_pressed();
}

void AnimationTrackEditor::_auto_fit() { timeline->auto_fit(); }

void AnimationTrackEditor::_auto_fit_bezier()
{
	timeline->auto_fit();

	if (bezier_mc->is_visible()) {
		bezier_edit->auto_fit_vertically();
	}
}

void AnimationTrackEditor::_root_node_changed(Node* p_node, bool p_removed)
{
	add_animation_player->set_disabled(p_removed);
}

void AnimationTrackEditor::_scene_changed()
{
	add_animation_player->set_disabled(EditorNode::get_singleton()->get_edited_scene() == nullptr);
}

void AnimationTrackEditor::_update_snap_unit()
{
	nearest_fps = 0;

	if (step->get_value() <= 0) {
		snap_unit = 0;
		_update_nearest_fps_label();
		return; // Avoid zero div.
	}

	if (timeline->is_using_fps()) {
		snap_unit = 1.0 / step->get_value();
	}
	else {
		if (fps_compat->is_pressed()) {
			snap_unit = CLAMP(step->get_value(), 0.0, 1.0);
			if (!Math::is_zero_approx(snap_unit)) {
				real_t fps = Math::round(1.0 / snap_unit);
				nearest_fps = int(fps);
				snap_unit = 1.0 / fps;
			}
		}
		else {
			snap_unit = step->get_value();
		}
	}
	_update_nearest_fps_label();
}

float AnimationTrackEditor::snap_time(float p_value, bool p_relative)
{
	if (is_snap_keys_enabled()) {
		double current_snap = snap_unit;
		if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
			// Use more precise snapping when holding Shift.
			current_snap *= 0.25;
		}

		if (p_relative) {
			double rel = Math::fmod(timeline->get_value(), current_snap);
			p_value = Math::snapped(p_value + rel, current_snap) - rel;
		}
		else {
			p_value = Math::snapped(p_value, current_snap);
		}
	}

	return p_value;
}

float AnimationTrackEditor::get_snap_unit() { return snap_unit; }

void AnimationTrackEditor::_update_timeline_margins()
{
	int margin_left =
		timeline_mc->get_theme_constant(SNAME("margin_left"), SNAME("AnimationTrackMargins"));
	int margin_right =
		timeline_mc->get_theme_constant(SNAME("margin_right"), SNAME("AnimationTrackMargins"));

	// Prevent the timeline cursor from misaligning with the tracks on the right-to-left layout.
	if (scroll->get_v_scroll_bar()->is_visible() && is_layout_rtl()) {
		margin_left += scroll->get_v_scroll_bar()->get_minimum_size().width;
	}

	timeline_mc->add_theme_constant_override(SNAME("margin_left"), margin_left);
	timeline_mc->add_theme_constant_override(SNAME("margin_right"), margin_right);

	bezier_mc->add_theme_constant_override(SNAME("margin_left"), margin_left);
}

void AnimationTrackEditor::_show_imported_anim_warning()
{
	// It looks terrible on a single line but the TTR extractor doesn't support line breaks yet.
	EditorNode::get_singleton()->show_warning(
		TTR("This animation belongs to an imported scene, so changes to imported tracks will not "
			"be saved.\n\nTo modify this animation, navigate to the scene's Advanced Import "
			"settings and select the animation.\nSome options, including looping, are available "
			"here. To add custom tracks, enable \"Save To File\" and\n\"Keep Custom Tracks\"."));
}

void AnimationTrackEditor::_show_dummy_player_warning()
{
	EditorNode::get_singleton()->show_warning(
		TTR("Some AnimationPlayerEditor's options are disabled since this is the dummy "
			"AnimationPlayer "
			"for preview.\n\nThe dummy player is forced active, non-deterministic and doesn't "
			"have the "
			"root motion track. Furthermore, the original node is inactive temporary."));
}

void AnimationTrackEditor::_show_inactive_player_warning()
{
	EditorNode::get_singleton()->show_warning(
		TTR("AnimationPlayer is inactive. The playback will not be processed."));
}

void AnimationTrackEditor::_select_all_tracks_for_copy()
{
	TreeItem* track = track_copy_select->get_root()->get_first_child();
	if (!track) {
		return;
	}

	bool all_selected = true;
	while (track) {
		if (!track->is_checked(0)) {
			all_selected = false;
		}

		track = track->get_next();
	}

	track = track_copy_select->get_root()->get_first_child();
	while (track) {
		track->set_checked(0, !all_selected);
		track = track->get_next();
	}
}

void AnimationTrackEditor::_bind_methods() {}

void AnimationTrackEditor::_pick_track_filter_text_changed(const String& p_newtext)
{
	TreeItem* root_item = pick_track->get_scene_tree()->get_scene_tree()->get_root();

	Vector<Node*> select_candidates;
	Node* to_select = nullptr;

	String filter = pick_track->get_filter_line_edit()->get_text();

	_pick_track_select_recursive(root_item, filter, select_candidates);

	if (!select_candidates.is_empty()) {
		for (int i = 0; i < select_candidates.size(); ++i) {
			Node* candidate = select_candidates[i];

			if (((String)candidate->get_name()).to_lower().begins_with(filter.to_lower())) {
				to_select = candidate;
				break;
			}
		}

		if (!to_select) {
			to_select = select_candidates[0];
		}
	}

	pick_track->get_scene_tree()->set_selected(to_select);
}

void AnimationTrackEditor::popup_read_only_dialog()
{
	read_only_dialog->popup_centered(Size2(200, 100) * EDSCALE);
}

AnimationTrackEditor::~AnimationTrackEditor()
{
	memdelete(key_edit);
	memdelete(multi_key_edit);
}

void AnimationMarkerEdit::_zoom_changed()
{
	queue_redraw();
	play_position->queue_redraw();
}

void AnimationMarkerEdit::_play_position_draw()
{
	if (animation.is_null() || play_position_pos < 0) {
		return;
	}

	float scale = timeline->get_zoom_scale();
	int px = (play_position_pos - timeline->get_value()) * scale + timeline->get_name_limit();

	if (px >= timeline->get_name_limit() &&
		px < (get_size().width - timeline->get_buttons_width())) {
		Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		play_position->draw_line(
			Point2(px, 0), Point2(px, get_size().height), color, Math::round(2 * EDSCALE));
	}
}

bool AnimationMarkerEdit::_is_ui_pos_in_current_section(const Point2& p_pos)
{
	int limit = timeline->get_name_limit();
	int limit_end = get_size().width - timeline->get_buttons_width();

	if (p_pos.x >= limit && p_pos.x <= limit_end) {
		PackedStringArray section = get_selected_section();
		if (!section.is_empty()) {
			StringName start_marker = section[0];
			StringName end_marker = section[1];
			float start_offset =
				(animation->get_marker_time(start_marker) - timeline->get_value()) *
					timeline->get_zoom_scale() +
				limit;
			float end_offset = (animation->get_marker_time(end_marker) - timeline->get_value()) *
								   timeline->get_zoom_scale() +
							   limit;
			return p_pos.x >= start_offset && p_pos.x <= end_offset;
		}
	}

	return false;
}

HBoxContainer* AnimationMarkerEdit::_create_hbox_labeled_control(
	const String& p_text, Control* p_control) const
{
	HBoxContainer* hbox = memnew(HBoxContainer);
	Label* label = memnew(Label);
	label->set_text(p_text);
	hbox->add_child(label);
	hbox->add_child(p_control);
	hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	label->set_h_size_flags(SIZE_EXPAND_FILL);
	label->set_stretch_ratio(1.0);
	p_control->set_h_size_flags(SIZE_EXPAND_FILL);
	p_control->set_stretch_ratio(1.0);
	return hbox;
}

void AnimationMarkerEdit::_update_key_edit()
{
	_clear_key_edit();
	if (animation.is_null()) {
		return;
	}

	if (selection.size() == 1) {
		key_edit = memnew(AnimationMarkerKeyEdit);
		key_edit->animation = animation;
		key_edit->animation_read_only = read_only;
		key_edit->marker_name = *selection.begin();
		key_edit->use_fps = timeline->is_using_fps();
		key_edit->marker_edit = this;

		InspectorDock::get_singleton()->set_info(TTR("Marker name is read-only in the inspector."),
			TTR("A marker's name can only be changed by right-clicking it in the animation "
				"editor "
				"and selecting \"Rename Marker\", in order to make sure that marker names are "
				"all "
				"unique."),
			true);
	}
	else if (selection.size() > 1) {
		multi_key_edit = memnew(AnimationMultiMarkerKeyEdit);
		multi_key_edit->animation = animation;
		multi_key_edit->animation_read_only = read_only;
		multi_key_edit->marker_edit = this;
		for (const StringName& name : selection) {
			multi_key_edit->marker_names.push_back(name);
		}
	}
}

void AnimationMarkerEdit::_bind_methods() {}

int AnimationMarkerEdit::get_key_height() const
{
	if (animation.is_null()) {
		return 0;
	}

	return type_icon->get_height();
}

Rect2 AnimationMarkerEdit::get_key_rect(float p_pixels_sec) const
{
	if (animation.is_null()) {
		return Rect2();
	}

	Rect2 rect =
		Rect2(-type_icon->get_width() / 2, get_size().height - type_icon->get_size().height,
			type_icon->get_width(), type_icon->get_size().height);

	// Make it a big easier to click.
	rect.position.x -= rect.size.x * 0.5;
	rect.size.x *= 2;
	return rect;
}

PackedStringArray AnimationMarkerEdit::get_selected_section() const
{
	if (selection.size() >= 2) {
		PackedStringArray arr;
		arr.push_back(""); // Marker with smallest time.
		arr.push_back(""); // Marker with largest time.
		double min_time = Math::INF;
		double max_time = -Math::INF;
		for (const StringName& marker_name : selection) {
			double time = animation->get_marker_time(marker_name);
			if (time < min_time) {
				arr.set(0, marker_name);
				min_time = time;
			}
			if (time > max_time) {
				arr.set(1, marker_name);
				max_time = time;
			}
		}
		return arr;
	}

	return PackedStringArray();
}

bool AnimationMarkerEdit::is_marker_selected(const StringName& p_marker) const
{
	return selection.has(p_marker);
}

bool AnimationMarkerEdit::is_key_selectable_by_distance() const { return true; }

void AnimationMarkerEdit::draw_bg(int p_clip_left, int p_clip_right) {}

void AnimationMarkerEdit::draw_fg(int p_clip_left, int p_clip_right) {}

Ref<Animation> AnimationMarkerEdit::get_animation() const { return animation; }

void AnimationMarkerEdit::set_animation(const Ref<Animation>& p_animation, bool p_read_only)
{
	if (animation.is_valid()) {
		_clear_selection_for_anim(animation);
	}
	animation = p_animation;
	read_only = p_read_only;
	type_icon = get_editor_theme_icon(SNAME("Marker"));
	selected_icon = get_editor_theme_icon(SNAME("MarkerSelected"));

	queue_redraw();
}

Size2 AnimationMarkerEdit::get_minimum_size() const
{
	Ref<Texture2D> texture = get_editor_theme_icon(SNAME("Object"));
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	int separation = get_theme_constant(SNAME("v_separation"), SNAME("ItemList"));

	int max_h = MAX(texture->get_height(), font->get_height(font_size));
	max_h = MAX(max_h, get_key_height());

	return Vector2(1, max_h + separation);
}

void AnimationMarkerEdit::set_editor(AnimationTrackEditor* p_editor) { editor = p_editor; }

void AnimationMarkerEdit::set_play_position(float p_pos)
{
	play_position_pos = p_pos;
	play_position->queue_redraw();
}

void AnimationMarkerEdit::update_play_position() { play_position->queue_redraw(); }

void AnimationMarkerEdit::_move_selection_begin()
{
	moving_selection = true;
	moving_selection_offset = 0;
}

void AnimationMarkerEdit::_move_selection(float p_offset)
{
	moving_selection_offset = p_offset;
	queue_redraw();
}

void AnimationMarkerEdit::_move_selection_cancel()
{
	moving_selection = false;
	queue_redraw();
}

void AnimationMarkerEdit::_clear_selection(bool p_update)
{
	AnimationPlayer* player = AnimationPlayerEditor::get_singleton()->get_player();
	if (player) {
		player->reset_section();
	}

	selection.clear();

	if (p_update) {
		queue_redraw();
	}

	_clear_key_edit();
}

void AnimationMarkerEdit::_clear_selection_for_anim(const Ref<Animation>& p_anim)
{
	if (animation != p_anim) {
		return;
	}

	_clear_selection(true);
}

void AnimationMarkerEdit::_select_key(const StringName& p_name, bool is_single)
{
	if (is_single) {
		_clear_selection(false);
	}

	selection.insert(p_name);

	AnimationPlayer* player = AnimationPlayerEditor::get_singleton()->get_player();
	if (player) {
		if (selection.size() >= 2) {
			PackedStringArray selected_section = get_selected_section();
			double start_time = animation->get_marker_time(selected_section[0]);
			double end_time = animation->get_marker_time(selected_section[1]);
			player->set_section(start_time, end_time);
		}
		else {
			player->reset_section();
		}
	}

	queue_redraw();
	_update_key_edit();

	editor->_clear_selection(editor->is_selection_active());
}

void AnimationMarkerEdit::_deselect_key(const StringName& p_name)
{
	selection.erase(p_name);

	AnimationPlayer* player = AnimationPlayerEditor::get_singleton()->get_player();
	if (player) {
		if (selection.size() >= 2) {
			PackedStringArray selected_section = get_selected_section();
			double start_time = animation->get_marker_time(selected_section[0]);
			double end_time = animation->get_marker_time(selected_section[1]);
			player->set_section(start_time, end_time);
		}
		else {
			player->reset_section();
		}
	}

	queue_redraw();
	_update_key_edit();
}

void AnimationMarkerEdit::_insert_marker(float p_ofs)
{
	if (editor->is_snap_timeline_enabled()) {
		p_ofs = editor->snap_time(p_ofs);
	}

	editor->resolve_insertion_offset(p_ofs);

	marker_insert_confirm->popup_centered(Size2(200, 100) * EDSCALE);
	marker_insert_color->set_pick_color(Color(1, 1, 1));

	String base = "new_marker";
	int count = 1;
	while (true) {
		String attempt = base;
		if (count > 1) {
			attempt += vformat("_%d", count);
		}
		if (animation->has_marker(attempt)) {
			count++;
			continue;
		}
		base = attempt;
		break;
	}

	marker_insert_new_name->set_text(base);
	_marker_insert_new_name_changed(base);
	marker_insert_ofs = p_ofs;
}

void AnimationMarkerEdit::_rename_marker(const StringName& p_name)
{
	marker_rename_confirm->popup_centered(Size2i(200, 0) * EDSCALE);
	marker_rename_prev_name = p_name;
	marker_rename_new_name->set_text(p_name);
}

void AnimationMarkerEdit::_marker_insert_new_name_changed(const String& p_text)
{
	marker_insert_confirm->get_ok_button()->set_disabled(p_text.is_empty());
}

void AnimationMarkerEdit::_marker_rename_new_name_changed(const String& p_text)
{
	marker_rename_confirm->get_ok_button()->set_disabled(p_text.is_empty());
}

float AnimationMarkerKeyEdit::get_time() const { return animation->get_marker_time(marker_name); }

void AnimationMarkerKeyEdit::_set_marker_name(const StringName& p_name) { marker_name = p_name; }


