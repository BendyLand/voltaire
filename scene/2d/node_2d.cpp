/**************************************************************************/
/*  node_2d.cpp                                                           */
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

#include "node_2d.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"

#ifdef TOOLS_ENABLED

#endif

void Node2D::_set_xform_dirty(bool p_dirty) const
{
	if (is_group_processing()) {
		if (p_dirty) {
			xform_dirty.mt.set();
		}
		else {
			xform_dirty.mt.clear();
		}
	}
	else {
		xform_dirty.st = p_dirty;
	}
}

void Node2D::_update_xform_values() const
{
	rotation = transform.get_rotation();
	skew = transform.get_skew();
	position = transform.columns[2];
	scale = transform.get_scale();
	_set_xform_dirty(false);
}

void Node2D::reparent(Node* p_parent, bool p_keep_global_transform)
{
	ERR_THREAD_GUARD;
	if (p_keep_global_transform) {
		Transform2D temp = get_global_transform();
		Node::reparent(p_parent);
		set_global_transform(temp);
	}
	else {
		Node::reparent(p_parent);
	}
}

void Node2D::set_position(const Point2& p_pos)
{
	ERR_THREAD_GUARD;
	if (_is_xform_dirty()) {
		_update_xform_values();
	}
	position = p_pos;
	_update_transform();
}

void Node2D::set_rotation(real_t p_radians)
{
	ERR_THREAD_GUARD;
	if (_is_xform_dirty()) {
		_update_xform_values();
	}
	rotation = p_radians;
	_update_transform();
}

void Node2D::set_rotation_degrees(real_t p_degrees)
{
	ERR_THREAD_GUARD;
	set_rotation(Math::deg_to_rad(p_degrees));
}

void Node2D::set_skew(real_t p_radians)
{
	ERR_THREAD_GUARD;
	if (_is_xform_dirty()) {
		_update_xform_values();
	}
	skew = p_radians;
	_update_transform();
}

void Node2D::set_scale(const Size2& p_scale)
{
	ERR_THREAD_GUARD;
	if (_is_xform_dirty()) {
		_update_xform_values();
	}
	scale = p_scale;
	// Avoid having 0 scale values, can lead to errors in physics and rendering.
	if (Math::is_zero_approx(scale.x)) {
		scale.x = CMP_EPSILON;
	}
	if (Math::is_zero_approx(scale.y)) {
		scale.y = CMP_EPSILON;
	}
	_update_transform();
}

Point2 Node2D::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	if (_is_xform_dirty()) {
		_update_xform_values();
	}

	return position;
}

real_t Node2D::get_rotation() const
{
	ERR_READ_THREAD_GUARD_V(0);
	if (_is_xform_dirty()) {
		_update_xform_values();
	}

	return rotation;
}

real_t Node2D::get_rotation_degrees() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return Math::rad_to_deg(get_rotation());
}

real_t Node2D::get_skew() const
{
	ERR_READ_THREAD_GUARD_V(0);
	if (_is_xform_dirty()) {
		_update_xform_values();
	}

	return skew;
}

Size2 Node2D::get_scale() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (_is_xform_dirty()) {
		_update_xform_values();
	}

	return scale;
}

Transform2D Node2D::get_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return transform;
}

void Node2D::rotate(real_t p_radians)
{
	ERR_THREAD_GUARD;
	set_rotation(get_rotation() + p_radians);
}

void Node2D::translate(const Vector2& p_amount)
{
	ERR_THREAD_GUARD;
	set_position(get_position() + p_amount);
}

void Node2D::global_translate(const Vector2& p_amount)
{
	ERR_THREAD_GUARD;
	set_global_position(get_global_position() + p_amount);
}

void Node2D::apply_scale(const Size2& p_amount)
{
	ERR_THREAD_GUARD;
	set_scale(get_scale() * p_amount);
}

void Node2D::move_x(real_t p_delta, bool p_scaled)
{
	ERR_THREAD_GUARD;
	Transform2D t = get_transform();
	Vector2 m = t[0];
	if (!p_scaled) {
		m.normalize();
	}
	set_position(t[2] + m * p_delta);
}

void Node2D::move_y(real_t p_delta, bool p_scaled)
{
	ERR_THREAD_GUARD;
	Transform2D t = get_transform();
	Vector2 m = t[1];
	if (!p_scaled) {
		m.normalize();
	}
	set_position(t[2] + m * p_delta);
}

Point2 Node2D::get_global_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	return get_global_transform().get_origin();
}

void Node2D::set_global_position(const Point2& p_pos)
{
	ERR_THREAD_GUARD;
	CanvasItem* parent = get_parent_item();
	if (parent) {
		Transform2D inv = parent->get_global_transform().affine_inverse();
		set_position(inv.xform(p_pos));
	}
	else {
		set_position(p_pos);
	}
}

real_t Node2D::get_global_rotation() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return get_global_transform().get_rotation();
}

real_t Node2D::get_global_rotation_degrees() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return Math::rad_to_deg(get_global_rotation());
}

real_t Node2D::get_global_skew() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return get_global_transform().get_skew();
}

void Node2D::set_global_rotation(const real_t p_radians)
{
	ERR_THREAD_GUARD;
	CanvasItem* parent = get_parent_item();
	if (parent) {
		Transform2D parent_global_transform = parent->get_global_transform();
		Transform2D new_transform = parent_global_transform * get_transform();
		new_transform.set_rotation(p_radians);
		new_transform = parent_global_transform.affine_inverse() * new_transform;
		set_rotation(new_transform.get_rotation());
	}
	else {
		set_rotation(p_radians);
	}
}

void Node2D::set_global_rotation_degrees(const real_t p_degrees)
{
	ERR_THREAD_GUARD;
	set_global_rotation(Math::deg_to_rad(p_degrees));
}

void Node2D::set_global_skew(const real_t p_radians)
{
	ERR_THREAD_GUARD;
	CanvasItem* parent = get_parent_item();
	if (parent) {
		Transform2D parent_global_transform = parent->get_global_transform();
		Transform2D new_transform = parent_global_transform * get_transform();
		new_transform.set_skew(p_radians);
		new_transform = parent_global_transform.affine_inverse() * new_transform;
		set_skew(new_transform.get_skew());
	}
	else {
		set_skew(p_radians);
	}
}

Size2 Node2D::get_global_scale() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return get_global_transform().get_scale();
}

void Node2D::set_global_scale(const Size2& p_scale)
{
	ERR_THREAD_GUARD;
	CanvasItem* parent = get_parent_item();
	if (parent) {
		Transform2D parent_global_transform = parent->get_global_transform();
		Transform2D new_transform = parent_global_transform * get_transform();
		new_transform.set_scale(p_scale);
		new_transform = parent_global_transform.affine_inverse() * new_transform;
		set_scale(new_transform.get_scale());
	}
	else {
		set_scale(p_scale);
	}
}

void Node2D::set_global_transform(const Transform2D& p_transform)
{
	ERR_THREAD_GUARD;
	CanvasItem* parent = get_parent_item();
	if (parent) {
		set_transform(parent->get_global_transform().affine_inverse() * p_transform);
	}
	else {
		set_transform(p_transform);
	}
}

void Node2D::look_at(const Vector2& p_pos)
{
	ERR_THREAD_GUARD;
	rotate(get_angle_to(p_pos));
}

real_t Node2D::get_angle_to(const Vector2& p_pos) const
{
	ERR_READ_THREAD_GUARD_V(0);
	return (to_local(p_pos) * get_scale()).angle();
}

Point2 Node2D::to_local(const Point2& p_global) const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	return get_global_transform().affine_inverse().xform(p_global);
}

Point2 Node2D::to_global(const Point2& p_local) const
{
	ERR_READ_THREAD_GUARD_V(Point2());
	return get_global_transform().xform(p_local);
}



void Node2D::_update_transform() {}

void Node2D::set_transform(Transform2D const&) {}

// ==========================================================================
// Merged from scene/2d/camera_2d.cpp
// ==========================================================================

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
	uint64_t tick = Engine::get_physics_frames();
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

void Camera2D::_update_scroll() {}

bool Camera2D::is_current() const {}

void Camera2D::make_current() {}

void Camera2D::clear_current() {}

Transform2D Camera2D::get_camera_transform() { return Transform2D(); }

// ==========================================================================
// Merged from scene/2d/canvas_group.cpp
// ==========================================================================

#include "canvas_group.h"
#include "servers/rendering/renderer.h"

real_t CanvasGroup::get_fit_margin() const { return fit_margin; }

real_t CanvasGroup::get_clear_margin() const { return clear_margin; }

void CanvasGroup::set_use_mipmaps(bool p_use_mipmaps)
{
	use_mipmaps = p_use_mipmaps;
	Renderer::canvas_item_set_canvas_group_mode(get_canvas_item(),
		RSE::CANVAS_GROUP_MODE_TRANSPARENT, clear_margin, true, fit_margin, use_mipmaps);
}

bool CanvasGroup::is_using_mipmaps() const { return use_mipmaps; }

CanvasGroup::CanvasGroup()
{
	set_fit_margin(10.0); // sets things
}

CanvasGroup::~CanvasGroup()
{
	Renderer::canvas_item_set_canvas_group_mode(
		get_canvas_item(), RSE::CANVAS_GROUP_MODE_DISABLED);
}

PackedStringArray CanvasGroup::get_configuration_warnings() const { return PackedStringArray(); }

void CanvasGroup::set_fit_margin(float) {}

// ==========================================================================
// Merged from scene/2d/canvas_modulate.cpp
// ==========================================================================

#include "canvas_modulate.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void CanvasModulate::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_CANVAS: {
		is_in_canvas = true;
		bool visible_in_tree = is_visible_in_tree();
		if (visible_in_tree) {
			_on_in_canvas_visibility_changed(true);
		}
		was_visible_in_tree = visible_in_tree;
	} break;

	case NOTIFICATION_EXIT_CANVAS: {
		is_in_canvas = false;
		if (was_visible_in_tree) {
			_on_in_canvas_visibility_changed(false);
		}
	} break;


	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (!is_in_canvas) {
			return;
		}

		bool visible_in_tree = is_visible_in_tree();
		if (visible_in_tree == was_visible_in_tree) {
			return;
		}

		_on_in_canvas_visibility_changed(visible_in_tree);

		was_visible_in_tree = visible_in_tree;
	} break;
	}
}

void CanvasModulate::set_color(const Color& p_color)
{
	color = p_color;
	if (is_active) {
		Renderer::canvas_set_modulate(get_canvas(), color);
	}
}

Color CanvasModulate::get_color() const { return color; }

PackedStringArray CanvasModulate::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (is_in_canvas && is_visible_in_tree()) {
		Vector<Node*> nodes =
			SceneTree::get_nodes_in_group("_canvas_modulate_" + itos(get_canvas().get_id()));

		if (nodes.size() > 1) {
			warnings.push_back(
				RTR("Only one visible CanvasModulate is allowed per canvas.\nWhen there are more "
					"than one, only one of them will be active. Which one is undefined."));
		}
	}

	return warnings;
}

CanvasModulate::CanvasModulate() {}

CanvasModulate::~CanvasModulate() {}



void CanvasModulate::_on_in_canvas_visibility_changed(bool) {}

// ==========================================================================
// Merged from scene/2d/back_buffer_copy.cpp
// ==========================================================================

#include "back_buffer_copy.h"
#include "core/config/engine.h"
#include "servers/rendering/renderer.h"

void BackBufferCopy::_update_copy_mode()
{
	switch (copy_mode) {
	case COPY_MODE_DISABLED: {
		Renderer::canvas_item_set_copy_to_backbuffer(get_canvas_item(), false, Rect2());
	} break;
	case COPY_MODE_RECT: {
		Renderer::canvas_item_set_copy_to_backbuffer(get_canvas_item(), true, rect);
	} break;
	case COPY_MODE_VIEWPORT: {
		Renderer::canvas_item_set_copy_to_backbuffer(get_canvas_item(), true, Rect2());

	} break;
	}
}

#ifdef DEBUG_ENABLED
Rect2 BackBufferCopy::_edit_get_rect() const { return rect; }

bool BackBufferCopy::_edit_use_rect() const { return true; }
#endif // DEBUG_ENABLED

Rect2 BackBufferCopy::get_anchorable_rect() const { return rect; }

void BackBufferCopy::set_rect(const Rect2& p_rect)
{
	rect = p_rect;
	_update_copy_mode();
	item_rect_changed();
}

Rect2 BackBufferCopy::get_rect() const { return rect; }

void BackBufferCopy::set_copy_mode(CopyMode p_mode)
{
	copy_mode = p_mode;
	_update_copy_mode();
}

BackBufferCopy::CopyMode BackBufferCopy::get_copy_mode() const { return copy_mode; }

BackBufferCopy::BackBufferCopy()
{
	_update_copy_mode();
	set_hide_clip_children(true);
}

BackBufferCopy::~BackBufferCopy() {}

// ==========================================================================
// Merged from scene/2d/marker_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "marker_2d.h"

void Marker2D::_draw_cross()
{
	const real_t extents = get_gizmo_extents();

	PackedVector2Array points = {
		Point2(+extents, 0),
		Point2(),
		Point2(),
		Point2(-extents, 0),
		Point2(0, +extents),
		Point2(),
		Point2(),
		Point2(0, -extents),
	};

	// Use the axis color which is brighter for the positive axis.
	// Use a darkened axis color for the negative axis.
	// This makes it possible to see in which direction the Marker3D node is rotated
	// (which can be important depending on how it's used).
	// Axis colors are taken from `axis_x_color` and `axis_y_color` (defined in
	// `editor/themes/editor_theme_manager.cpp`).
	const Color color_x = Color(0.96, 0.20, 0.32);
	const Color color_y = Color(0.53, 0.84, 0.01);
	PackedColorArray colors = {
		color_x,
		color_x.darkened(0.5),
		color_y,
		color_y.darkened(0.5),
	};

	draw_multiline_colors(points, colors);
}

#ifdef DEBUG_ENABLED
Rect2 Marker2D::_edit_get_rect() const
{
	real_t extents = get_gizmo_extents();
	return Rect2(Point2(-extents, -extents), Size2(extents * 2, extents * 2));
}

bool Marker2D::_edit_use_rect() const { return false; }
#endif // DEBUG_ENABLED

void Marker2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (!is_inside_tree()) {
			break;
		}
		if (Engine::is_editor_hint()) {
			_draw_cross();
		}
	} break;
	}
}

real_t Marker2D::get_gizmo_extents() const { return gizmo_extents; }

Marker2D::Marker2D() { set_hide_clip_children(true); }

// ==========================================================================
// Merged from scene/2d/remote_transform_2d.cpp
// ==========================================================================

#include "remote_transform_2d.h"

NodePath RemoteTransform2D::get_remote_node() const { return remote_node; }

void RemoteTransform2D::set_use_global_coordinates(const bool p_enable)
{
	if (use_global_coordinates == p_enable) {
		return;
	}

	use_global_coordinates = p_enable;
	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
	_update_remote();
}

bool RemoteTransform2D::get_use_global_coordinates() const { return use_global_coordinates; }

void RemoteTransform2D::set_update_position(const bool p_update)
{
	if (update_remote_position == p_update) {
		return;
	}
	update_remote_position = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_position() const { return update_remote_position; }

void RemoteTransform2D::set_update_rotation(const bool p_update)
{
	if (update_remote_rotation == p_update) {
		return;
	}
	update_remote_rotation = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_rotation() const { return update_remote_rotation; }

void RemoteTransform2D::set_update_scale(const bool p_update)
{
	if (update_remote_scale == p_update) {
		return;
	}
	update_remote_scale = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_scale() const { return update_remote_scale; }

void RemoteTransform2D::force_update_cache() { _update_cache(); }

PackedStringArray RemoteTransform2D::get_configuration_warnings() const { return PackedStringArray(); }

RemoteTransform2D::RemoteTransform2D()
{
	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
	set_hide_clip_children(true);
}



void RemoteTransform2D::_update_remote() {}

void RemoteTransform2D::_update_cache() {}

// ==========================================================================
// Merged from scene/2d/visible_on_screen_notifier_2d.cpp
// ==========================================================================

#include "visible_on_screen_notifier_2d.h"

#include "core/config/engine.h"
#include "servers/rendering/renderer.h"

#ifdef TOOLS_ENABLED
void VisibleOnScreenNotifier2D::_edit_set_rect(const Rect2& p_edit_rect) { set_rect(p_edit_rect); }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 VisibleOnScreenNotifier2D::_edit_get_rect() const { return rect; }

bool VisibleOnScreenNotifier2D::_edit_use_rect() const { return show_rect; }
#endif // DEBUG_ENABLED

void VisibleOnScreenNotifier2D::set_rect(const Rect2& p_rect)
{
	rect = p_rect;
	queue_redraw();
}

Rect2 VisibleOnScreenNotifier2D::get_rect() const { return rect; }

void VisibleOnScreenNotifier2D::set_show_rect(bool p_show_rect)
{
	show_rect = p_show_rect;
	queue_redraw();
}

bool VisibleOnScreenNotifier2D::is_showing_rect() const { return show_rect; }

bool VisibleOnScreenNotifier2D::is_on_screen() const { return on_screen; }

void VisibleOnScreenNotifier2D::set_enabler_enabled(bool p_enabled)
{
	enabler_enabled = p_enabled;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier2D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier2D::set_enable_node_path(const NodePath& p_path)
{
	enable_node_path = p_path;
}

void VisibleOnScreenNotifier2D::_visibility_enter()
{
	on_screen = true;
	if (enabler_enabled) {
		_update_enable_mode(true);
	}
}

void VisibleOnScreenNotifier2D::_visibility_exit()
{
	on_screen = false;
	if (enabler_enabled) {
		_update_enable_mode(false);
	}
}

void VisibleOnScreenNotifier2D::_update_enable_mode(bool p_enable)
{
	if (Engine::is_editor_hint()) {
		return;
	}
	Node* node = get_node_or_null(enable_node_path);
	if (!node) {
		return;
	}
	if (p_enable) {
		switch (enable_mode) {
		case ENABLE_MODE_INHERIT:
			node->set_process_mode(PROCESS_MODE_INHERIT);
			break;
		case ENABLE_MODE_ALWAYS:
			node->set_process_mode(PROCESS_MODE_ALWAYS);
			break;
		case ENABLE_MODE_WHEN_PAUSED:
			node->set_process_mode(PROCESS_MODE_WHEN_PAUSED);
			break;
		}
	}
	else {
		node->set_process_mode(PROCESS_MODE_DISABLED);
	}
}

void VisibleOnScreenNotifier2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
	case NOTIFICATION_EXIT_TREE: {
		on_screen = false;
	} break;
	}
}

VisibleOnScreenNotifier2D::VisibleOnScreenNotifier2D(bool p_enabler) : enabler_enabled(p_enabler)
{
	rect = Rect2(-10, -10, 20, 20);
	set_hide_clip_children(true);
}

// ==========================================================================
// Merged from scene/2d/audio_listener_2d.cpp
// ==========================================================================

#include "audio_listener_2d.h"
#include "scene/main/viewport.h"

void AudioListener2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!is_part_of_edited_scene() && current) {
			make_current();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (!is_part_of_edited_scene()) {
			if (is_current()) {
				clear_current();
				current = true; // Keep it true.
			}
			else {
				current = false;
			}
		}
	} break;
	}
}

void AudioListener2D::make_current()
{
	current = true;
	if (!is_inside_tree()) {
		return;
	}
	get_viewport()->_audio_listener_2d_set(this);
}

void AudioListener2D::clear_current()
{
	current = false;
	if (!is_inside_tree()) {
		return;
	}
	get_viewport()->_audio_listener_2d_remove(this);
}

bool AudioListener2D::is_current() const
{
	if (is_inside_tree() && !is_part_of_edited_scene()) {
		return get_viewport()->get_audio_listener_2d() == this;
	}
	else {
		return current;
	}
}

AudioListener2D::AudioListener2D() { set_hide_clip_children(true); }

// ==========================================================================
// Merged from scene/2d/audio_stream_player_2d.cpp
// ==========================================================================

// --- audio_stream_player_2d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "audio_stream_player_2d.h"

bool AudioStreamPlayer2D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "audio_stream_player_2d.h"
#include "core/config/project_settings.h"
#include "scene/2d/audio_listener_2d.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/physics_2d/physics_server_2d.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/physics/area_2d.h"
#endif // PHYSICS_2D_DISABLED

void AudioStreamPlayer2D::_notification(int p_what)
{
	internal->notification(p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		AudioServer::add_listener_changed_callback(_listener_changed_cb, this);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		AudioServer::remove_listener_changed_callback(_listener_changed_cb, this);
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		// Update anything related to position first, if possible of course.
		if (setplay.get() > 0 ||
			(internal->active.is_set() &&
				last_mix_count != AudioServer::get_mix_count()) ||
			force_update_panning) {
			force_update_panning = false;
			_update_panning();
		}

		if (setplayback.is_valid() && setplay.get() >= 0) {
			internal->active.set();
			AudioServer::start_playback_stream(setplayback, _get_actual_bus(),
				volume_vector, setplay.get(), internal->pitch_scale);
			setplayback.unref();
			setplay.set(-1);
		}

		if (!internal->stream_playbacks.is_empty() && internal->active.is_set()) {
			internal->process();
		}
		internal->ensure_playback_limit();
	} break;
	}
}

// Interacts with PhysicsServer2D, so can only be called during _physics_process
void AudioStreamPlayer2D::_update_panning()
{
	if (!internal->active.is_set() || internal->stream.is_null()) {
		return;
	}

	Ref<World2D> world_2d = get_world_2d();
	ERR_FAIL_COND(world_2d.is_null());

	Vector2 global_pos = get_global_position();

	HashSet<Viewport*> viewports(world_2d->get_viewports());

	volume_vector.resize(4);
	volume_vector.write[0] = AudioFrame(0, 0);
	volume_vector.write[1] = AudioFrame(0, 0);
	volume_vector.write[2] = AudioFrame(0, 0);
	volume_vector.write[3] = AudioFrame(0, 0);

	String actual_bus = _get_actual_bus();

	for (Viewport* vp : viewports) {
		if (!vp->is_audio_listener_2d()) {
			continue;
		}
		// compute matrix to convert to screen
		Vector2 screen_size = vp->get_visible_rect().size;
		Vector2 listener_in_global;
		Vector2 relative_to_listener;

		// screen in global is used for attenuation
		AudioListener2D* listener = vp->get_audio_listener_2d();
		Transform2D full_canvas_transform =
			vp->get_global_canvas_transform() * vp->get_canvas_transform();
		if (listener) {
			listener_in_global = listener->get_global_position();
			relative_to_listener =
				(global_pos - listener_in_global).rotated(-listener->get_global_rotation());
			relative_to_listener *=
				full_canvas_transform
					.get_scale(); // Default listener scales with canvas size, do the same here.
		}
		else {
			listener_in_global = full_canvas_transform.affine_inverse().xform(screen_size * 0.5);
			relative_to_listener = full_canvas_transform.xform(global_pos) - screen_size * 0.5;
		}

		float dist =
			global_pos.distance_to(listener_in_global); // Distance to listener, or screen if none.

		if (dist > max_distance) {
			continue; // Can't hear this sound in this viewport.
		}

		float multiplier = Math::pow(1.0f - dist / max_distance, attenuation);
		multiplier *= Math::db_to_linear(internal->volume_db); // Also apply player volume!

		float pan = relative_to_listener.x / screen_size.x;
		// Don't let the panning effect extend (too far) beyond the screen.
		pan = CLAMP(pan, -1, 1);

		// Bake in a constant factor here to allow the project setting defaults for 2d and 3d to be
		// normalized to 1.0.
		pan *= panning_strength * cached_global_panning_strength * 0.5f;

		pan = CLAMP(pan + 0.5, 0.0, 1.0);

		float l = 1.0 - pan;
		float r = pan;

		const AudioFrame& prev_sample = volume_vector[0];
		AudioFrame new_sample = AudioFrame(l, r) * multiplier;

		volume_vector.write[0] =
			AudioFrame(MAX(prev_sample[0], new_sample[0]), MAX(prev_sample[1], new_sample[1]));
	}

	for (const Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_bus_exclusive(
			playback, actual_bus, volume_vector);
	}

	for (const Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_pitch_scale(playback, internal->pitch_scale);
		if (playback->get_is_sample() && playback->get_sample_playback().is_valid()) {
			Ref<AudioSamplePlayback> sample_playback = playback->get_sample_playback();
			AudioServer::update_sample_playback_pitch_scale(
				sample_playback, internal->pitch_scale);
		}
	}

	last_mix_count = AudioServer::get_mix_count();
}

void AudioStreamPlayer2D::set_stream(Ref<AudioStream> p_stream) { internal->set_stream(p_stream); }

Ref<AudioStream> AudioStreamPlayer2D::get_stream() const { return internal->stream; }

void AudioStreamPlayer2D::set_volume_db(float p_volume)
{
	ERR_FAIL_COND_MSG(Math::is_nan(p_volume), "Volume can't be set to NaN.");
	internal->volume_db = p_volume;
}

float AudioStreamPlayer2D::get_volume_db() const { return internal->volume_db; }

void AudioStreamPlayer2D::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioStreamPlayer2D::get_volume_linear() const { return Math::db_to_linear(get_volume_db()); }

void AudioStreamPlayer2D::set_pitch_scale(float p_pitch_scale)
{
	internal->set_pitch_scale(p_pitch_scale);
}

float AudioStreamPlayer2D::get_pitch_scale() const { return internal->pitch_scale; }

void AudioStreamPlayer2D::play(float p_from_pos)
{
	Ref<AudioStreamPlayback> stream_playback = internal->play_basic();
	if (stream_playback.is_null()) {
		return;
	}
	setplayback = stream_playback;
	setplay.set(p_from_pos);

	// Sample handling.
	if (stream_playback->get_is_sample() && stream_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = stream_playback->get_sample_playback();
		sample_playback->offset = p_from_pos;
		sample_playback->bus = _get_actual_bus();

		AudioServer::start_sample_playback(sample_playback);
	}
}

void AudioStreamPlayer2D::seek(float p_seconds) { internal->seek(p_seconds); }

void AudioStreamPlayer2D::stop()
{
	setplay.set(-1);
	internal->stop_basic();
}

bool AudioStreamPlayer2D::is_playing() const
{
	if (setplay.get() >= 0) {
		return true; // play() has been called this frame, but no playback exists just yet.
	}
	return internal->is_playing();
}

float AudioStreamPlayer2D::get_playback_position()
{
	if (setplay.get() >= 0) {
		return setplay.get(); // play() has been called this frame, but no playback exists just yet.
	}
	return internal->get_playback_position();
}

void AudioStreamPlayer2D::set_bus(const String& p_bus)
{
	internal->bus = p_bus; // This will be pushed to the audio server during the next physics
						   // timestep, which is fast enough.
}

String AudioStreamPlayer2D::get_bus() const { return internal->get_bus(); }

void AudioStreamPlayer2D::set_autoplay(bool p_enable) { internal->autoplay = p_enable; }

bool AudioStreamPlayer2D::is_autoplay_enabled() const { return internal->autoplay; }

void AudioStreamPlayer2D::_set_playing(bool p_enable) { internal->set_playing(p_enable); }

void AudioStreamPlayer2D::set_max_distance(float p_pixels)
{
	ERR_FAIL_COND(p_pixels <= 0.0);
	max_distance = p_pixels;
}

float AudioStreamPlayer2D::get_max_distance() const { return max_distance; }

void AudioStreamPlayer2D::set_attenuation(float p_curve) { attenuation = p_curve; }

float AudioStreamPlayer2D::get_attenuation() const { return attenuation; }

void AudioStreamPlayer2D::set_area_mask(uint32_t p_mask) { area_mask = p_mask; }

uint32_t AudioStreamPlayer2D::get_area_mask() const { return area_mask; }

void AudioStreamPlayer2D::set_stream_paused(bool p_pause) { internal->set_stream_paused(p_pause); }

bool AudioStreamPlayer2D::get_stream_paused() const { return internal->get_stream_paused(); }

bool AudioStreamPlayer2D::has_stream_playback() { return internal->has_stream_playback(); }

Ref<AudioStreamPlayback> AudioStreamPlayer2D::get_stream_playback()
{
	return internal->get_stream_playback();
}

void AudioStreamPlayer2D::set_max_polyphony(int p_max_polyphony)
{
	internal->set_max_polyphony(p_max_polyphony);
}

int AudioStreamPlayer2D::get_max_polyphony() const { return internal->max_polyphony; }

void AudioStreamPlayer2D::set_panning_strength(float p_panning_strength)
{
	ERR_FAIL_COND_MSG(p_panning_strength < 0, "Panning strength must be a positive number.");
	panning_strength = p_panning_strength;
}

float AudioStreamPlayer2D::get_panning_strength() const { return panning_strength; }

AudioServer::PlaybackType AudioStreamPlayer2D::get_playback_type() const
{
	return internal->get_playback_type();
}

void AudioStreamPlayer2D::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	internal->set_playback_type(p_playback_type);
}

AudioStreamPlayer2D::~AudioStreamPlayer2D() { memdelete(internal); }



String AudioStreamPlayer2D::_get_actual_bus() {}
