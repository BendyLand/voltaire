/**************************************************************************/
/*  animation_bezier_editor.cpp                                           */
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

#include <climits>
#include "animation_bezier_editor.h"
#include "core/string/translation_server.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/option_button.h"
#include "scene/gui/view_panner.h"
#include "scene/resources/text_line.h"

float AnimationBezierTrackEdit::_bezier_h_to_pixel(float p_h)
{
	float h = p_h;
	h = (h - timeline_v_scroll) / timeline_v_zoom;
	h = (get_size().height / 2.0) - h;
	return h;
}

void AnimationBezierTrackEdit::_draw_track(int p_track, const Color& p_color)
{
	float scale = timeline->get_zoom_scale();

	int limit = timeline->get_name_limit();
	int right_limit = get_size().width;

	// Selection may have altered the order of keys.
	RBMap<real_t, int> key_order;

	for (int i = 0; i < animation->track_get_key_count(p_track); i++) {
		real_t ofs = animation->track_get_key_time(p_track, i);
		if (selection.has(IntPair(p_track, i))) {
			if (moving_selection) {
				ofs += moving_selection_offset.x;
			}
			else if (scaling_selection) {
				ofs += -scaling_selection_offset.x +
					   (ofs - scaling_selection_pivot.x) * (scaling_selection_scale.x - 1);
			}
		}

		key_order[ofs] = i;
	}

	for (RBMap<real_t, int>::Element* E = key_order.front(); E; E = E->next()) {
		int i = E->get();

		if (!E->next()) {
			break;
		}

		int i_n = E->next()->get();

		float offset = animation->track_get_key_time(p_track, i);
		float height = animation->bezier_track_get_key_value(p_track, i);
		Vector2 out_handle = animation->bezier_track_get_key_out_handle(p_track, i);
		if (p_track == moving_handle_track && (moving_handle == -1 || moving_handle == 1) &&
			moving_handle_key == i) {
			out_handle = moving_handle_right;
		}

		if (selection.has(IntPair(p_track, i))) {
			if (moving_selection) {
				offset += moving_selection_offset.x;
				height += moving_selection_offset.y;
			}
			else if (scaling_selection) {
				offset += -scaling_selection_offset.x +
						  (offset - scaling_selection_pivot.x) * (scaling_selection_scale.x - 1);
				height += -scaling_selection_offset.y +
						  (height - scaling_selection_pivot.y) * (scaling_selection_scale.y - 1);
			}
		}

		float offset_n = animation->track_get_key_time(p_track, i_n);
		float height_n = animation->bezier_track_get_key_value(p_track, i_n);
		Vector2 in_handle = animation->bezier_track_get_key_in_handle(p_track, i_n);
		if (p_track == moving_handle_track && (moving_handle == -1 || moving_handle == 1) &&
			moving_handle_key == i_n) {
			in_handle = moving_handle_left;
		}

		if (selection.has(IntPair(p_track, i_n))) {
			if (moving_selection) {
				offset_n += moving_selection_offset.x;
				height_n += moving_selection_offset.y;
			}
			else if (scaling_selection) {
				offset_n += -scaling_selection_offset.x + (offset_n - scaling_selection_pivot.x) *
															  (scaling_selection_scale.x - 1);
				height_n += -scaling_selection_offset.y + (height_n - scaling_selection_pivot.y) *
															  (scaling_selection_scale.y - 1);
			}
		}

		if (moving_inserted_key && moving_selection_from_track == p_track) {
			if (moving_selection_from_key == i) {
				Animation::HandleMode handle_mode =
					animation->bezier_track_get_key_handle_mode(p_track, i);
				if (handle_mode != Animation::HANDLE_MODE_FREE) {
					float offset_p = offset;
					float height_p = height;
					if (E->prev()) {
						int i_p = E->prev()->get();
						offset_p = animation->track_get_key_time(p_track, i_p);
						height_p = animation->bezier_track_get_key_value(p_track, i_p);
					}

					animation->bezier_track_calculate_handles(offset, offset_p, height_p, offset_n,
						height_n, handle_mode, Animation::HANDLE_SET_MODE_AUTO, nullptr,
						&out_handle);
				}
			}
			else if (moving_selection_from_key == i_n) {
				Animation::HandleMode handle_mode =
					animation->bezier_track_get_key_handle_mode(p_track, i_n);
				if (handle_mode != Animation::HANDLE_MODE_FREE) {
					float offset_nn = offset_n;
					float height_nn = height_n;
					if (E->next()->next()) {
						int i_nn = E->next()->next()->get();
						offset_nn = animation->track_get_key_time(p_track, i_nn);
						height_nn = animation->bezier_track_get_key_value(p_track, i_nn);
					}

					animation->bezier_track_calculate_handles(offset_n, offset, height, offset_nn,
						height_nn, handle_mode, Animation::HANDLE_SET_MODE_AUTO, &in_handle,
						nullptr);
				}
			}
		}

		out_handle += Vector2(offset, height);
		in_handle += Vector2(offset_n, height_n);

		Vector2 start(offset, height);
		Vector2 end(offset_n, height_n);

		int from_x = (offset - timeline->get_value()) * scale + limit;
		int point_start = from_x;
		int to_x = (offset_n - timeline->get_value()) * scale + limit;
		int point_end = to_x;

		if (from_x > right_limit) { // Not visible.
			continue;
		}

		if (to_x < limit) { // Not visible.
			continue;
		}

		from_x = MAX(from_x, limit);
		to_x = MIN(to_x, right_limit);

		Vector<Vector2> lines;

		Vector2 prev_pos;

		for (int j = from_x; j <= to_x; j++) {
			float t = (j - limit) / scale + timeline->get_value();

			float h;

			if (j == point_end) {
				h = end.y; // Make sure it always connects.
			}
			else if (j == point_start) {
				h = start.y; // Make sure it always connects.
			}
			else { // Custom interpolation, used because it needs to show paths affected by moving
					 // the selection or handles.
				int iterations = 10;
				float low = 0;
				float high = 1;

				// Narrow high and low as much as possible.
				for (int k = 0; k < iterations; k++) {
					float middle = (low + high) / 2.0;

					Vector2 interp = start.bezier_interpolate(out_handle, in_handle, end, middle);

					if (interp.x < t) {
						low = middle;
					}
					else {
						high = middle;
					}
				}

				// Interpolate the result.
				Vector2 low_pos = start.bezier_interpolate(out_handle, in_handle, end, low);
				Vector2 high_pos = start.bezier_interpolate(out_handle, in_handle, end, high);

				float c = (t - low_pos.x) / (high_pos.x - low_pos.x);

				h = low_pos.lerp(high_pos, c).y;
			}

			h = _bezier_h_to_pixel(h);

			Vector2 pos(j, h);

			if (j > from_x) {
				lines.push_back(prev_pos);
				lines.push_back(pos);
			}
			prev_pos = pos;
		}

		if (lines.size() >= 2) {
			draw_multiline(lines, p_color, Math::round(EDSCALE), true);
		}
	}
}

void AnimationBezierTrackEdit::_draw_line_clipped(const Vector2& p_from, const Vector2& p_to,
	const Color& p_color, int p_clip_left, int p_clip_right)
{
	Vector2 from = p_from;
	Vector2 to = p_to;

	if (from.x == to.x && from.y == to.y) {
		return;
	}
	if (to.x < from.x) {
		SWAP(to, from);
	}

	if (to.x < p_clip_left) {
		return;
	}

	if (from.x > p_clip_right) {
		return;
	}

	if (to.x > p_clip_right) {
		float c = (p_clip_right - from.x) / (to.x - from.x);
		to = from.lerp(to, c);
	}

	if (from.x < p_clip_left) {
		float c = (p_clip_left - from.x) / (to.x - from.x);
		from = from.lerp(to, c);
	}

	draw_line(from, to, p_color, Math::round(EDSCALE), true);
}

// Check if a track is displayed in the bezier editor (track type = bezier and track not filtered).
bool AnimationBezierTrackEdit::_is_track_displayed(int p_track_index)
{
	if (animation->track_get_type(p_track_index) != Animation::TrackType::TYPE_BEZIER) {
		return false;
	}

	if (is_filtered) {
		String path = String(animation->track_get_path(p_track_index));
		if (root && root->has_node(path)) {
			Node* node = root->get_node(path);
			if (!node) {
				return false; // No node, no filter.
			}
			if (!EditorNode::get_singleton()->get_editor_selection()->is_selected(node)) {
				return false; // Skip track due to not selected.
			}
		}
	}

	return true;
}

// Check if the curves for a track are displayed in the editor (not hidden). Includes the check on
// the track visibility.
bool AnimationBezierTrackEdit::_is_track_curves_displayed(int p_track_index)
{
	// Is the track is visible in the editor?
	if (!_is_track_displayed(p_track_index)) {
		return false;
	}

	// And curves visible?
	if (hidden_tracks.has(p_track_index)) {
		return false;
	}

	return true;
}

Ref<Animation> AnimationBezierTrackEdit::get_animation() const { return animation; }

void AnimationBezierTrackEdit::set_animation_and_track(
	const Ref<Animation>& p_animation, int p_track, bool p_read_only)
{
	if (p_animation != animation) {
		selection.clear();
	}
	animation = p_animation;
	read_only = p_read_only;
	selected_track = p_track;
	queue_redraw();
}

Size2 AnimationBezierTrackEdit::get_minimum_size() const { return Vector2(1, 1); }

Control::CursorShape AnimationBezierTrackEdit::get_cursor_shape(const Point2& p_pos) const
{
	// Box selecting or moving a handle
	if (box_selecting || Math::abs(moving_handle) == 1) {
		return get_default_cursor_shape();
	}
	// Hovering a handle
	if (!read_only) {
		for (const EditPoint& edit_point : edit_points) {
			if (edit_point.in_rect.has_point(p_pos) || edit_point.out_rect.has_point(p_pos)) {
				return get_default_cursor_shape();
			}
		}
	}
	// Currently box scaling
	if (scaling_selection) {
		if (scaling_selection_handles == Vector2i(1, 1) ||
			scaling_selection_handles == Vector2i(-1, -1)) {
			return CURSOR_FDIAGSIZE;
		}
		else if (scaling_selection_handles == Vector2i(1, -1) ||
				   scaling_selection_handles == Vector2i(-1, 1)) {
			return CURSOR_BDIAGSIZE;
		}
		else if (abs(scaling_selection_handles.x) == 1) {
			return CURSOR_HSIZE;
		}
		else if (abs(scaling_selection_handles.y) == 1) {
			return CURSOR_VSIZE;
		}
	}
	// Hovering the scaling box
	const Vector2i rel_pos = p_pos - selection_rect.position;
	if (selection_handles_rect.has_point(p_pos)) {
		if ((rel_pos.x < 0 && rel_pos.y < 0) ||
			(rel_pos.x > selection_rect.size.width && rel_pos.y > selection_rect.size.height)) {
			return CURSOR_FDIAGSIZE;
		}
		else if ((rel_pos.x < 0 && rel_pos.y > selection_rect.size.height) ||
				   (rel_pos.x > selection_rect.size.width && rel_pos.y < 0)) {
			return CURSOR_BDIAGSIZE;
		}
		else if (rel_pos.x < 0 || rel_pos.x > selection_rect.size.width) {
			return CURSOR_HSIZE;
		}
		else if (rel_pos.y < 0 || rel_pos.y > selection_rect.size.height) {
			return CURSOR_VSIZE;
		}
		return CURSOR_MOVE;
	}
	return get_default_cursor_shape();
}

void AnimationBezierTrackEdit::_play_position_draw()
{
	if (animation.is_null() || play_position_pos < 0) {
		return;
	}

	float scale = timeline->get_zoom_scale();
	int h = get_size().height;

	int limit = timeline->get_name_limit();

	int px = (-timeline->get_value() + play_position_pos) * scale + limit;

	if (px >= limit && px < (get_size().width)) {
		const Color color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		play_position->draw_line(Point2(px, 0), Point2(px, h), color, Math::round(2 * EDSCALE));
	}
}

void AnimationBezierTrackEdit::set_play_position(real_t p_pos)
{
	play_position_pos = p_pos;
	play_position->queue_redraw();
}

void AnimationBezierTrackEdit::update_play_position() { play_position->queue_redraw(); }

void AnimationBezierTrackEdit::set_root(Node* p_root) { root = p_root; }

void AnimationBezierTrackEdit::set_filtered(bool p_filtered)
{
	is_filtered = p_filtered;
	if (animation.is_null()) {
		return;
	}
	String base_path = String(animation->track_get_path(selected_track));
	if (is_filtered) {
		if (root && root->has_node(base_path)) {
			Node* node = root->get_node(base_path);
			if (!node || !EditorNode::get_singleton()->get_editor_selection()->is_selected(node)) {
				for (int i = 0; i < animation->get_track_count(); ++i) {
					if (animation->track_get_type(i) != Animation::TrackType::TYPE_BEZIER) {
						continue;
					}

					base_path = String(animation->track_get_path(i));
					if (root && root->has_node(base_path)) {
						node = root->get_node(base_path);
						if (!node) {
							continue; // No node, no filter.
						}
						if (!EditorNode::get_singleton()->get_editor_selection()->is_selected(
								node)) {
							continue; // Skip track due to not selected.
						}

						set_animation_and_track(animation, i, read_only);
						break;
					}
				}
			}
		}
	}
	queue_redraw();
}

void AnimationBezierTrackEdit::auto_fit_vertically()
{
	int track_count = animation->get_track_count();
	real_t minimum_value = Math::INF;
	real_t maximum_value = -Math::INF;

	int nb_track_visible = 0;
	for (int i = 0; i < track_count; ++i) {
		if (!_is_track_curves_displayed(i) || locked_tracks.has(i)) {
			continue;
		}

		int key_count = animation->track_get_key_count(i);

		for (int j = 0; j < key_count; ++j) {
			real_t value = animation->bezier_track_get_key_value(i, j);

			minimum_value = MIN(value, minimum_value);
			maximum_value = MAX(value, maximum_value);

			// We also want to includes the handles...
			Vector2 in_vec = animation->bezier_track_get_key_in_handle(i, j);
			Vector2 out_vec = animation->bezier_track_get_key_out_handle(i, j);

			minimum_value = MIN(value + in_vec.y, minimum_value);
			maximum_value = MAX(value + in_vec.y, maximum_value);
			minimum_value = MIN(value + out_vec.y, minimum_value);
			maximum_value = MAX(value + out_vec.y, maximum_value);
		}

		nb_track_visible++;
	}

	if (nb_track_visible == 0) {
		// No visible track... we will not adjust the vertical zoom
		return;
	}

	if (Math::is_finite(minimum_value) && Math::is_finite(maximum_value)) {
		_zoom_vertically(minimum_value, maximum_value);
		queue_redraw();
	}
}

void AnimationBezierTrackEdit::_zoom_vertically(real_t p_minimum_value, real_t p_maximum_value)
{
	real_t target_height = p_maximum_value - p_minimum_value;
	if (target_height <= CMP_EPSILON) {
		timeline_v_scroll = p_maximum_value;
		return;
	}

	timeline_v_scroll = (p_maximum_value + p_minimum_value) / 2.0;
	timeline_v_zoom = target_height / ((get_size().height - timeline->get_size().height) * 0.9);
}

void AnimationBezierTrackEdit::_zoom_changed()
{
	queue_redraw();
	play_position->queue_redraw();
}

void AnimationBezierTrackEdit::_update_locked_tracks_after(int p_track)
{
	_unlock_track(p_track);

	Vector<int> updated_locked_tracks;
	for (const int& E : locked_tracks) {
		updated_locked_tracks.push_back(E);
	}
	locked_tracks.clear();
	for (int i = 0; i < updated_locked_tracks.size(); ++i) {
		if (updated_locked_tracks[i] > p_track) {
			locked_tracks.insert(updated_locked_tracks[i] - 1);
		}
		else {
			locked_tracks.insert(updated_locked_tracks[i]);
		}
	}
}

void AnimationBezierTrackEdit::_update_hidden_tracks_after(int p_track)
{
	_show_track(p_track);

	Vector<int> updated_hidden_tracks;
	for (const int& E : hidden_tracks) {
		updated_hidden_tracks.push_back(E);
	}
	hidden_tracks.clear();
	for (int i = 0; i < updated_hidden_tracks.size(); ++i) {
		if (updated_hidden_tracks[i] > p_track) {
			hidden_tracks.insert(updated_hidden_tracks[i] - 1);
		}
		else {
			hidden_tracks.insert(updated_hidden_tracks[i]);
		}
	}
}

bool AnimationBezierTrackEdit::_lock_track(int p_track)
{
	locked_tracks.insert(p_track);
	if (selected_track == p_track) {
		for (int i = 0; i < animation->get_track_count(); ++i) {
			if (!locked_tracks.has(i) &&
				animation->track_get_type(i) == Animation::TrackType::TYPE_BEZIER) {
				set_animation_and_track(animation, i, read_only);
				return true;
			}
		}
	}

	return false;
}

bool AnimationBezierTrackEdit::_unlock_track(int p_track) { return locked_tracks.erase(p_track); }

bool AnimationBezierTrackEdit::_hide_track(int p_track)
{
	hidden_tracks.insert(p_track);
	if (selected_track == p_track) {
		for (int i = 0; i < animation->get_track_count(); ++i) {
			if (!hidden_tracks.has(i) &&
				animation->track_get_type(i) == Animation::TrackType::TYPE_BEZIER) {
				set_animation_and_track(animation, i, read_only);
				return true;
			}
		}
	}

	return false;
}

bool AnimationBezierTrackEdit::_show_track(int p_track) { return hidden_tracks.erase(p_track); }

void AnimationBezierTrackEdit::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (mm->get_position().x > timeline->get_name_limit()) {
			timeline_v_scroll += p_scroll_vec.y * timeline_v_zoom;
			timeline_v_scroll = CLAMP(timeline_v_scroll, -100000, 100000);
			timeline->set_value(
				timeline->get_value() - p_scroll_vec.x / timeline->get_zoom_scale());
		}
		else {
			track_v_scroll += p_scroll_vec.y;
			if (track_v_scroll < -track_v_scroll_max) {
				track_v_scroll = -track_v_scroll_max;
			}
			else if (track_v_scroll > 0) {
				track_v_scroll = 0;
			}
		}
		queue_redraw();
	}
}

void AnimationBezierTrackEdit::_zoom_callback(
	float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	const float v_zoom_orig = timeline_v_zoom;
	Ref<InputEventWithModifiers> iewm = p_event;
	if (iewm.is_valid() && iewm->is_alt_pressed()) {
		// Alternate zoom (doesn't affect timeline).
		timeline_v_zoom = CLAMP(timeline_v_zoom / p_zoom_factor, 0.000001, 100000);
	}
	else {
		float zoom_factor = p_zoom_factor > 1.0 ? AnimationTimelineEdit::SCROLL_ZOOM_FACTOR_IN
												: AnimationTimelineEdit::SCROLL_ZOOM_FACTOR_OUT;
		timeline->_zoom_callback(zoom_factor, p_origin, p_event);
	}
	timeline_v_scroll =
		timeline_v_scroll + (p_origin.y - get_size().y / 2.0) * (timeline_v_zoom - v_zoom_orig);
	queue_redraw();
}

void AnimationBezierTrackEdit::_bezier_track_insert_key_at_anim(const Ref<Animation>& p_anim,
	int p_track, double p_time, real_t p_value, const Vector2& p_in_handle,
	const Vector2& p_out_handle, const Animation::HandleMode p_handle_mode,
	Animation::HandleSetMode p_handle_set_mode)
{
	int idx = p_anim->bezier_track_insert_key(p_track, p_time, p_value, p_in_handle, p_out_handle);
	p_anim->bezier_track_set_key_handle_mode(p_track, idx, p_handle_mode, p_handle_set_mode);
}


