/**************************************************************************/
/*  camera_2d.cpp                                                         */
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

#include "camera_2d.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

void Camera2D::_update_process_callback()
{
	if (is_physics_interpolated_and_enabled()) {
		set_process_internal(is_current());
		set_physics_process_internal(is_current());

#ifdef TOOLS_ENABLED
		if (process_callback == CAMERA2D_PROCESS_IDLE) {
			WARN_PRINT_ONCE(
				"Camera2D overridden to physics process mode due to use of physics interpolation.");
		}
#endif
	}
	else if (is_part_of_edited_scene()) {
		set_process_internal(false);
		set_physics_process_internal(false);
	}
	else {
		if (process_callback == CAMERA2D_PROCESS_IDLE) {
			set_process_internal(true);
			set_physics_process_internal(false);
		}
		else {
			set_process_internal(false);
			set_physics_process_internal(true);
		}
	}
}

void Camera2D::set_zoom(const Vector2& p_zoom)
{
	// Setting zoom to zero causes 'affine_invert' issues.
	ERR_FAIL_COND_MSG(Math::is_zero_approx(p_zoom.x) || Math::is_zero_approx(p_zoom.y),
		"Zoom level must be different from 0 (can be negative).");

	zoom = p_zoom;
	zoom_scale = Vector2(1, 1) / zoom;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;
	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

Vector2 Camera2D::get_zoom() const { return zoom; }

void Camera2D::_ensure_update_interpolation_data()
{
	// The "curr -> previous" update can either occur
	// on NOTIFICATION_INTERNAL_PHYSICS_PROCESS, OR
	// on NOTIFICATION_TRANSFORM_CHANGED,
	// if NOTIFICATION_TRANSFORM_CHANGED takes place earlier than
	// NOTIFICATION_INTERNAL_PHYSICS_PROCESS on a tick.
	// This is to ensure that the data keeps flowing, but the new data
	// doesn't overwrite before prev has been set.

	// Keep the data flowing.
	uint64_t tick = Engine::get_singleton()->get_physics_frames();
	if (_interpolation_data.last_update_physics_tick != tick) {
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		_interpolation_data.last_update_physics_tick = tick;
	}
}

void Camera2D::set_offset(const Vector2& p_offset)
{
	if (offset == p_offset) {
		return;
	}
	offset = p_offset;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;
	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

Vector2 Camera2D::get_offset() const { return offset; }

void Camera2D::set_anchor_mode(AnchorMode p_anchor_mode)
{
	if (anchor_mode == p_anchor_mode) {
		return;
	}
	anchor_mode = p_anchor_mode;
	_update_scroll();
}

Camera2D::AnchorMode Camera2D::get_anchor_mode() const { return anchor_mode; }

void Camera2D::set_ignore_rotation(bool p_ignore)
{
	if (ignore_rotation == p_ignore) {
		return;
	}
	ignore_rotation = p_ignore;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;

	// Reset back to zero so it matches the camera rotation when ignore_rotation is enabled.
	if (ignore_rotation) {
		camera_angle = 0.0;
	}

	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

bool Camera2D::is_ignoring_rotation() const { return ignore_rotation; }

void Camera2D::set_limit_enabled(bool p_limit_enabled)
{
	if (limit_enabled == p_limit_enabled) {
		return;
	}
	limit_enabled = p_limit_enabled;
	_update_scroll();
}

bool Camera2D::is_limit_enabled() const { return limit_enabled; }

void Camera2D::set_process_callback(Camera2DProcessCallback p_mode)
{
	if (process_callback == p_mode) {
		return;
	}

	process_callback = p_mode;
	_update_process_callback();
}

void Camera2D::set_enabled(bool p_enabled)
{
	if (enabled == p_enabled) {
		return;
	}
	enabled = p_enabled;

	if (!is_inside_tree()) {
		return;
	}

	if (enabled && !viewport->get_camera_2d()) {
		make_current();
	}
	else if (!enabled && is_current()) {
		clear_current();
	}
}

bool Camera2D::is_enabled() const { return enabled; }

Camera2D::Camera2DProcessCallback Camera2D::get_process_callback() const
{
	return process_callback;
}

void Camera2D::set_limit_rect(const Rect2i& p_limit_rect)
{
	const Point2i limit_rect_end = p_limit_rect.get_end();
	set_limit(SIDE_LEFT, p_limit_rect.position.x);
	set_limit(SIDE_TOP, p_limit_rect.position.y);
	set_limit(SIDE_RIGHT, limit_rect_end.x);
	set_limit(SIDE_BOTTOM, limit_rect_end.y);
}

Rect2i Camera2D::get_limit_rect() const
{
	return Rect2i(limit[SIDE_LEFT], limit[SIDE_TOP], limit[SIDE_RIGHT] - limit[SIDE_LEFT],
		limit[SIDE_BOTTOM] - limit[SIDE_TOP]);
}

void Camera2D::set_limit(Side p_side, int p_limit)
{
	ERR_FAIL_INDEX((int)p_side, 4);
	if (limit[p_side] == p_limit) {
		return;
	}
	limit[p_side] = p_limit;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;
	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

int Camera2D::get_limit(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return limit[p_side];
}

void Camera2D::set_limit_smoothing_enabled(bool p_enabled)
{
	if (limit_smoothing_enabled == p_enabled) {
		return;
	}
	limit_smoothing_enabled = p_enabled;
	_update_scroll();
}

bool Camera2D::is_limit_smoothing_enabled() const { return limit_smoothing_enabled; }

real_t Camera2D::get_drag_margin(Side p_side) const
{
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return drag_margin[p_side];
}

Vector2 Camera2D::get_camera_position() const { return camera_pos; }

void Camera2D::force_update_scroll() { _update_scroll(); }

void Camera2D::reset_smoothing()
{
	_update_scroll();
	smoothed_camera_pos = camera_pos;
}

void Camera2D::set_position_smoothing_speed(real_t p_speed)
{
	if (position_smoothing_speed == p_speed) {
		return;
	}
	position_smoothing_speed = MAX(0, p_speed);
	_update_process_callback();
}

real_t Camera2D::get_position_smoothing_speed() const { return position_smoothing_speed; }

void Camera2D::set_rotation_smoothing_speed(real_t p_speed)
{
	if (rotation_smoothing_speed == p_speed) {
		return;
	}
	rotation_smoothing_speed = MAX(0, p_speed);
	_update_process_callback();
}

real_t Camera2D::get_rotation_smoothing_speed() const { return rotation_smoothing_speed; }

void Camera2D::set_rotation_smoothing_enabled(bool p_enabled)
{
	if (rotation_smoothing_enabled == p_enabled) {
		return;
	}
	rotation_smoothing_enabled = p_enabled;
}

bool Camera2D::is_rotation_smoothing_enabled() const { return rotation_smoothing_enabled; }

Point2 Camera2D::get_camera_screen_center() const { return camera_screen_center; }

real_t Camera2D::get_screen_rotation() const { return camera_angle; }

Size2 Camera2D::_get_camera_screen_size() const
{
	if (is_part_of_edited_scene()) {
		return Size2(GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_width"),
			GLOBAL_GET_CACHED(real_t, "display/window/size/viewport_height"));
	}
	ERR_FAIL_NULL_V(viewport, Size2());
	return viewport->get_visible_rect().size;
}

void Camera2D::set_drag_horizontal_enabled(bool p_enabled) { drag_horizontal_enabled = p_enabled; }

bool Camera2D::is_drag_horizontal_enabled() const { return drag_horizontal_enabled; }

void Camera2D::set_drag_vertical_enabled(bool p_enabled) { drag_vertical_enabled = p_enabled; }

bool Camera2D::is_drag_vertical_enabled() const { return drag_vertical_enabled; }

void Camera2D::set_drag_vertical_offset(real_t p_offset)
{
	if (drag_vertical_offset == p_offset) {
		return;
	}
	drag_vertical_offset = p_offset;
	drag_vertical_offset_changed = true;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;
	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

real_t Camera2D::get_drag_vertical_offset() const { return drag_vertical_offset; }

void Camera2D::set_drag_horizontal_offset(real_t p_offset)
{
	if (drag_horizontal_offset == p_offset) {
		return;
	}
	drag_horizontal_offset = p_offset;
	drag_horizontal_offset_changed = true;
	Point2 old_smoothed_camera_pos = smoothed_camera_pos;
	_update_scroll();
	smoothed_camera_pos = old_smoothed_camera_pos;
}

real_t Camera2D::get_drag_horizontal_offset() const { return drag_horizontal_offset; }

void Camera2D::set_position_smoothing_enabled(bool p_enabled)
{
	if (position_smoothing_enabled == p_enabled) {
		return;
	}
	position_smoothing_enabled = p_enabled;
}

bool Camera2D::is_position_smoothing_enabled() const { return position_smoothing_enabled; }

Node* Camera2D::get_custom_viewport() const { return custom_viewport; }

bool Camera2D::is_screen_drawing_enabled() const { return screen_drawing_enabled; }

bool Camera2D::is_limit_drawing_enabled() const { return limit_drawing_enabled; }

bool Camera2D::is_margin_drawing_enabled() const { return margin_drawing_enabled; }

Camera2D::Camera2D()
{
	set_notify_transform(true);
	set_hide_clip_children(true);
}


