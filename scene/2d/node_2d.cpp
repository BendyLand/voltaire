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

#include "scene/2d/camera_2d.h"
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
// --- Inlined from scene/2d/canvas_modulate.h ---
#include "scene/2d/node_2d.h"

class CanvasModulate : public Node2D
{
	Color color = Color(1, 1, 1, 1);

	// CanvasModulate is in canvas-specific modulate group when both in canvas and visible in tree.
	// Exactly one CanvasModulate in each such non-empty group is active.
	bool is_in_canvas = false;
	bool was_visible_in_tree = false; // Relevant only when in canvas.
	bool is_active = false;

	void _on_in_canvas_visibility_changed(bool p_new_visibility);

protected:
	void _notification(int p_what);

public:
	void set_color(const Color& p_color);
	Color get_color() const;

	PackedStringArray get_configuration_warnings() const override;

	CanvasModulate();
	~CanvasModulate();
};



// --- End scene/2d/canvas_modulate.h ---
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
// --- Inlined from scene/2d/back_buffer_copy.h ---
#include "scene/2d/node_2d.h"

class BackBufferCopy : public Node2D
{
public:
	enum CopyMode
	{
		COPY_MODE_DISABLED,
		COPY_MODE_RECT,
		COPY_MODE_VIEWPORT
	};

private:
	Rect2 rect = Rect2(-100, -100, 200, 200);
	CopyMode copy_mode = COPY_MODE_RECT;

	void _update_copy_mode();

public:
#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_rect(const Rect2& p_rect);
	Rect2 get_rect() const;
	Rect2 get_anchorable_rect() const override;

	void set_copy_mode(CopyMode p_mode);
	CopyMode get_copy_mode() const;

	BackBufferCopy();
	~BackBufferCopy();
};



// --- End scene/2d/back_buffer_copy.h ---
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
// --- Inlined from scene/2d/marker_2d.h ---
#include "scene/2d/node_2d.h"

class Marker2D : public Node2D
{
	real_t gizmo_extents = 10.0;

	void _draw_cross();

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_gizmo_extents(real_t p_extents);
	real_t get_gizmo_extents() const;

	Marker2D();
};



// --- End scene/2d/marker_2d.h ---

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
// --- Inlined from scene/2d/remote_transform_2d.h ---
#include "scene/2d/node_2d.h"

class RemoteTransform2D : public Node2D
{
	NodePath remote_node;

	bool use_global_coordinates = true;
	bool update_remote_position = true;
	bool update_remote_rotation = true;
	bool update_remote_scale = true;

	void _update_remote();
	void _update_cache();
	// void _node_exited_scene();
protected:
	void _notification(int p_what);

public:
	NodePath get_remote_node() const;

	void set_use_global_coordinates(const bool p_enable);
	bool get_use_global_coordinates() const;

	void set_update_position(const bool p_update);
	bool get_update_position() const;

	void set_update_rotation(const bool p_update);
	bool get_update_rotation() const;

	void set_update_scale(const bool p_update);
	bool get_update_scale() const;

	void force_update_cache();

	PackedStringArray get_configuration_warnings() const override;

	RemoteTransform2D();
};



// --- End scene/2d/remote_transform_2d.h ---

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

#include "scene/2d/camera_2d.h"

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

#include "scene/2d/camera_2d.h"
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

#include "scene/2d/camera_2d.h"

bool AudioStreamPlayer2D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/2d/camera_2d.h"
#include "core/config/project_settings.h"
#include "scene/2d/camera_2d.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/physics_2d/physics_server_2d.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/physics/collision_object_2d.h"
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

// ============================================================================
// Merged from scene/2d/cpu_particles_2d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/math/random_number_generator.h"
#include "core/math/transform_interpolator.h"
// --- cpu_particles_2d.compat.inc ---
#ifndef DISABLE_DEPRECATED
// --- Inlined from scene/2d/cpu_particles_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/curve.h"
#include "scene/resources/gradient.h"

class RandomNumberGenerator;

class CPUParticles2D : public Node2D
{
public:
	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
	};

	enum Parameter
	{
		PARAM_INITIAL_LINEAR_VELOCITY,
		PARAM_ANGULAR_VELOCITY,
		PARAM_ORBIT_VELOCITY,
		PARAM_LINEAR_ACCEL,
		PARAM_RADIAL_ACCEL,
		PARAM_TANGENTIAL_ACCEL,
		PARAM_DAMPING,
		PARAM_ANGLE,
		PARAM_SCALE,
		PARAM_HUE_VARIATION,
		PARAM_ANIM_SPEED,
		PARAM_ANIM_OFFSET,
		PARAM_MAX
	};

	enum ParticleFlags
	{
		PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY,
		PARTICLE_FLAG_ROTATE_Y,	 // Unused, but exposed for consistency with 3D.
		PARTICLE_FLAG_DISABLE_Z, // Unused, but exposed for consistency with 3D.
		PARTICLE_FLAG_MAX
	};

	enum EmissionShape
	{
		EMISSION_SHAPE_POINT,
		EMISSION_SHAPE_SPHERE,
		EMISSION_SHAPE_SPHERE_SURFACE,
		EMISSION_SHAPE_RECTANGLE,
		EMISSION_SHAPE_POINTS,
		EMISSION_SHAPE_DIRECTED_POINTS,
		EMISSION_SHAPE_RING,
		EMISSION_SHAPE_MAX,
	};

private:
	bool emitting = false;
	bool active = false;

	struct Particle
	{
		Transform2D transform;
		Color color;
		real_t custom[4] = {};
		real_t rotation = 0.0;
		Vector2 velocity;
		bool active = false;
		real_t angle_rand = 0.0;
		real_t scale_rand = 0.0;
		real_t hue_rot_rand = 0.0;
		real_t anim_offset_rand = 0.0;
		Color start_color_rand;
		double time = 0.0;
		double lifetime = 0.0;
		Color base_color;

		uint32_t seed = 0;
	};

	double time = 0.0;
	double frame_remainder = 0.0;
	int cycle = 0;
	bool do_redraw = false;

	RID mesh;
	RID multimesh;

	Vector<Particle> particles;
	Vector<float> particle_data;
	Vector<int> particle_order;

	struct SortLifetime
	{
		const Particle* particles = nullptr;

		bool operator()(int p_a, int p_b) const
		{
			return particles[p_a].time > particles[p_b].time;
		}
	};

	//

	bool one_shot = false;

	double lifetime = 1.0;
	double pre_process_time = 0.0;
	real_t _request_process_time = 0.0;
	real_t _request_process_time_residual = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double lifetime_randomness = 0.0;
	double speed_scale = 1.0;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = true;
	uint32_t seed = 0;
	bool use_fixed_seed = false;

	Transform2D inv_emission_transform;

#ifdef TOOLS_ENABLED
	bool show_gizmos = false;
#endif

	DrawOrder draw_order = DRAW_ORDER_INDEX;

	Ref<Texture2D> texture;

	////////

	Vector2 direction = Vector2(1, 0);
	real_t spread = 45.0;

	real_t parameters_min[PARAM_MAX] = {};
	real_t parameters_max[PARAM_MAX] = {};

	Ref<Curve> curve_parameters[PARAM_MAX];
	Color color;
	Ref<Gradient> color_ramp;
	Ref<Gradient> color_initial_ramp;

	bool particle_flags[PARTICLE_FLAG_MAX];

	EmissionShape emission_shape = EMISSION_SHAPE_POINT;
	real_t emission_sphere_radius = 1.0;
	Vector2 emission_rect_extents = Vector2(1, 1);
	Vector<Vector2> emission_points;
	Vector<Vector2> emission_normals;
	Vector<Color> emission_colors;
	real_t emission_ring_inner_radius = 0.8;
	real_t emission_ring_radius = 1.0;

	Ref<Curve> scale_curve_x;
	Ref<Curve> scale_curve_y;
	bool split_scale = false;

	Vector2 gravity = Vector2(0, 980);

	Ref<RandomNumberGenerator> rng;

	void _update_internal();
	void _particles_process(double p_delta);
	void _update_particle_data_buffer();
	void _set_emitting();

	Mutex update_mutex;

	struct InterpolationData
	{
		// Whether this particle is non-interpolated, but following an interpolated parent.
		bool interpolated_follow = false;

		// If doing interpolated follow, we need to keep these updated per tick.
		Transform2D global_xform_curr;
		Transform2D global_xform_prev;
	} _interpolation_data;

	void _update_render_thread();

	void _update_mesh_texture();

	void _set_do_redraw(bool p_do_redraw);

	void _texture_changed();

	void _refresh_interpolation_state();

protected:
	void _notification(int p_what);
#ifdef TOOLS_ENABLED
	void _draw_emission_gizmo();
#endif

#ifndef DISABLE_DEPRECATED
	void _restart_bind_compat_92089();
	void _request_particles_process_bind_compat_109142(real_t p_time);
	static void _bind_compatibility_methods();
#endif

public:
	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_one_shot);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_lifetime_randomness(double p_random);
	void set_use_local_coordinates(bool p_enable);
	void set_speed_scale(double p_scale);

	bool is_emitting() const;
	int get_amount() const;
	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	double get_lifetime_randomness() const;
	bool get_use_local_coordinates() const;
	double get_speed_scale() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_use_fixed_seed(bool p_use_fixed_seed);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
#ifdef TOOLS_ENABLED
	void set_show_gizmos(bool p_show_gizmos);
#endif
	uint32_t get_seed() const;

	void request_particles_process(
		real_t p_requested_process_time, real_t p_requested_process_time_residual = 0.);

	///////////////////

	void set_direction(Vector2 p_direction);
	Vector2 get_direction() const;

	void set_spread(real_t p_spread);
	real_t get_spread() const;

	real_t get_param_min(Parameter p_param) const;

	real_t get_param_max(Parameter p_param) const;

	Ref<Curve> get_param_curve(Parameter p_param) const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_color_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_ramp() const;

	void set_color_initial_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_initial_ramp() const;

	void set_particle_flag(ParticleFlags p_particle_flag, bool p_enable);
	bool get_particle_flag(ParticleFlags p_particle_flag) const;

	void set_emission_shape(EmissionShape p_shape);
	void set_emission_sphere_radius(real_t p_radius);
	void set_emission_rect_extents(Vector2 p_extents);
	void set_emission_points(const Vector<Vector2>& p_points);
	void set_emission_normals(const Vector<Vector2>& p_normals);
	void set_emission_colors(const Vector<Color>& p_colors);
	void set_emission_ring_inner_radius(real_t p_inner_radius);
	void set_emission_ring_radius(real_t p_ring_radius);
	void set_scale_curve_x(Ref<Curve> p_scale_curve);
	void set_scale_curve_y(Ref<Curve> p_scale_curve);
	void set_split_scale(bool p_split_scale);

	EmissionShape get_emission_shape() const;
	real_t get_emission_sphere_radius() const;
	Vector2 get_emission_rect_extents() const;
	Vector<Vector2> get_emission_points() const;
	Vector<Vector2> get_emission_normals() const;
	Vector<Color> get_emission_colors() const;
	real_t get_emission_ring_inner_radius() const;
	real_t get_emission_ring_radius() const;
	Ref<Curve> get_scale_curve_x() const;
	Ref<Curve> get_scale_curve_y() const;
	bool get_split_scale();

	void set_gravity(const Vector2& p_gravity);
	Vector2 get_gravity() const;

	PackedStringArray get_configuration_warnings() const override;

	void restart(bool p_keep_seed = false);

	void convert_from_particles(Node* p_particles);

	CPUParticles2D() = default;
	~CPUParticles2D();
};



// --- End scene/2d/cpu_particles_2d.h ---

void CPUParticles2D::_restart_bind_compat_92089() { restart(false); }

void CPUParticles2D::_request_particles_process_bind_compat_109142(real_t p_time)
{
	request_particles_process(p_time, 0.);
}

void CPUParticles2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- Inlined from scene/2d/gpu_particles_2d.h ---
#include "scene/2d/node_2d.h"

class GPUParticles2D : public Node2D
{
public:
	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
		DRAW_ORDER_REVERSE_LIFETIME,
	};

private:
	RID particles;

	bool emitting = false;
	bool active = false;
	bool signal_canceled = false;
	bool one_shot = false;
	int amount = 0;
	float amount_ratio = 1.0;
	double lifetime = 0.0;
	double pre_process_time = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double speed_scale = 0.0;
	Rect2 visibility_rect;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = false;
	bool interpolate = true;
	float interp_to_end_factor = 0;
	Vector3 previous_velocity;
	Vector2 previous_position;
	uint32_t seed = 0;
	bool use_fixed_seed = false;
#ifdef TOOLS_ENABLED
	bool show_gizmos = false;
#endif
	Ref<Material> process_material;

	DrawOrder draw_order = DRAW_ORDER_LIFETIME;

	Ref<Texture2D> texture;

	void _update_particle_emission_transform();

	NodePath sub_emitter;
	real_t collision_base_size = 1.0;

	bool trail_enabled = false;
	double trail_lifetime = 0.3;
	int trail_sections = 8;
	int trail_section_subdivisions = 4;

	double time = 0.0;
	double emission_time = 0.0;
	double active_time = 0.0;

	RID mesh;

	void _attach_sub_emitter();

	void _texture_changed();

protected:
	void _notification(int p_what);
#ifdef TOOLS_ENABLED
	void _draw_emission_gizmo();
#endif
	void _update_collision_size();

#ifndef DISABLE_DEPRECATED
	void _restart_bind_compat_92089();
	void _request_particles_process_bind_compat_109142(real_t p_time);
	static void _bind_compatibility_methods();
#endif

public:
	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_enable);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_visibility_rect(const Rect2& p_visibility_rect);
	void set_use_local_coordinates(bool p_enable);
	void set_process_material(const Ref<Material>& p_material);
	void set_speed_scale(double p_scale);
	void set_collision_base_size(real_t p_ratio);
	void set_trail_enabled(bool p_enabled);
	void set_trail_lifetime(double p_seconds);
	void set_trail_sections(int p_sections);
	void set_trail_section_subdivisions(int p_subdivisions);
	void set_interp_to_end(float p_interp);
	void request_particles_process(
		real_t p_requested_process_time, real_t p_requested_process_time_residual);

#ifdef TOOLS_ENABLED
	void set_show_gizmos(bool p_show_gizmos);
#endif

	bool is_emitting() const;
	int get_amount() const;
	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	Rect2 get_visibility_rect() const;
	bool get_use_local_coordinates() const;
	Ref<Material> get_process_material() const;
	double get_speed_scale() const;

	real_t get_collision_base_size() const;
	bool is_trail_enabled() const;
	double get_trail_lifetime() const;
	int get_trail_sections() const;
	int get_trail_section_subdivisions() const;
	float get_interp_to_end() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_interpolate(bool p_enable);
	bool get_interpolate() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_amount_ratio(float p_ratio);
	float get_amount_ratio() const;

	PackedStringArray get_configuration_warnings() const override;

	NodePath get_sub_emitter() const;

	void set_use_fixed_seed(bool p_use_fixed_seed);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
	uint32_t get_seed() const;

	enum EmitFlags
	{
		EMIT_FLAG_POSITION = RSE::PARTICLES_EMIT_FLAG_POSITION,
		EMIT_FLAG_ROTATION_SCALE = RSE::PARTICLES_EMIT_FLAG_ROTATION_SCALE,
		EMIT_FLAG_VELOCITY = RSE::PARTICLES_EMIT_FLAG_VELOCITY,
		EMIT_FLAG_COLOR = RSE::PARTICLES_EMIT_FLAG_COLOR,
		EMIT_FLAG_CUSTOM = RSE::PARTICLES_EMIT_FLAG_CUSTOM
	};

	void emit_particle(const Transform2D& p_transform, const Vector2& p_velocity,
		const Color& p_color, const Color& p_custom, uint32_t p_emit_flags);

	void restart(bool p_keep_seed = false);
	Rect2 capture_rect() const;
	void convert_from_particles(Node* p_particles);

	GPUParticles2D();
	~GPUParticles2D();
};



// --- End scene/2d/gpu_particles_2d.h ---
#include "scene/main/scene_tree.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

void CPUParticles2D::set_emitting(bool p_emitting)
{
	if (emitting == p_emitting) {
		return;
	}

	if (p_emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}

	emitting = p_emitting;
	if (emitting) {
		_set_emitting();
	}
}

void CPUParticles2D::_set_emitting()
{
	active = true;
	set_process_internal(true);
	// first update before rendering to avoid one frame delay after emitting starts
	if (time == 0) {
		_update_internal();
	}
}

void CPUParticles2D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles must be greater than 0.");

	particles.resize(p_amount);
	{
		Particle* w = particles.ptrw();

		for (int i = 0; i < p_amount; i++) {
			w[i].active = false;
		}
	}

	particle_data.resize((8 + 4 + 4) * p_amount);
	Renderer::multimesh_allocate_data(
		multimesh, p_amount, RSE::MULTIMESH_TRANSFORM_2D, true, true);

	particle_order.resize(p_amount);
}

void CPUParticles2D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
}

void CPUParticles2D::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

void CPUParticles2D::set_pre_process_time(double p_time) { pre_process_time = p_time; }

void CPUParticles2D::set_explosiveness_ratio(real_t p_ratio) { explosiveness_ratio = p_ratio; }

void CPUParticles2D::set_randomness_ratio(real_t p_ratio) { randomness_ratio = p_ratio; }

void CPUParticles2D::set_lifetime_randomness(double p_random) { lifetime_randomness = p_random; }

void CPUParticles2D::set_speed_scale(double p_scale) { speed_scale = p_scale; }

bool CPUParticles2D::is_emitting() const { return emitting; }

int CPUParticles2D::get_amount() const { return particles.size(); }

double CPUParticles2D::get_lifetime() const { return lifetime; }

bool CPUParticles2D::get_one_shot() const { return one_shot; }

double CPUParticles2D::get_pre_process_time() const { return pre_process_time; }

real_t CPUParticles2D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t CPUParticles2D::get_randomness_ratio() const { return randomness_ratio; }

double CPUParticles2D::get_lifetime_randomness() const { return lifetime_randomness; }

bool CPUParticles2D::get_use_local_coordinates() const { return local_coords; }

double CPUParticles2D::get_speed_scale() const { return speed_scale; }

void CPUParticles2D::set_draw_order(DrawOrder p_order) { draw_order = p_order; }

CPUParticles2D::DrawOrder CPUParticles2D::get_draw_order() const { return draw_order; }

void CPUParticles2D::_refresh_interpolation_state()
{
	if (!is_inside_tree()) {
		return;
	}

	// The logic for whether to do an interpolated follow.
	// This is rather complex, but basically:
	// If project setting interpolation is ON and this particle system is in global mode,
	// we will follow the INTERPOLATED position rather than the actual position.
	// This is so that particles aren't generated AHEAD of the interpolated parent.
	bool follow = !local_coords && SceneTree::is_physics_interpolation_enabled();

	if (follow == _interpolation_data.interpolated_follow) {
		return;
	}

	_interpolation_data.interpolated_follow = follow;

	set_physics_process_internal(_interpolation_data.interpolated_follow);
}

Ref<Texture2D> CPUParticles2D::get_texture() const { return texture; }

void CPUParticles2D::set_fixed_fps(int p_count) { fixed_fps = p_count; }

int CPUParticles2D::get_fixed_fps() const { return fixed_fps; }

void CPUParticles2D::set_fractional_delta(bool p_enable) { fractional_delta = p_enable; }

bool CPUParticles2D::get_fractional_delta() const { return fractional_delta; }

void CPUParticles2D::restart(bool p_keep_seed)
{
	time = 0;
	frame_remainder = 0;
	cycle = 0;
	emitting = false;

	{
		int pc = particles.size();
		Particle* w = particles.ptrw();

		for (int i = 0; i < pc; i++) {
			w[i].active = false;
		}
	}
	if (!p_keep_seed && !use_fixed_seed) {
		seed = Math::rand();
	}

	emitting = true;
	_set_emitting();
}

void CPUParticles2D::set_direction(Vector2 p_direction) { direction = p_direction; }

Vector2 CPUParticles2D::get_direction() const { return direction; }

void CPUParticles2D::set_spread(real_t p_spread) { spread = p_spread; }

real_t CPUParticles2D::get_spread() const { return spread; }

real_t CPUParticles2D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
}

real_t CPUParticles2D::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_max[p_param];
}

static void _adjust_curve_range(const Ref<Curve>& p_curve, real_t p_min, real_t p_max)
{
	Ref<Curve> curve = p_curve;
	if (curve.is_null()) {
		return;
	}

	curve->ensure_default_setup(p_min, p_max);
}

Ref<Curve> CPUParticles2D::get_param_curve(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Curve>());

	return curve_parameters[p_param];
}

void CPUParticles2D::set_color(const Color& p_color) { color = p_color; }

Color CPUParticles2D::get_color() const { return color; }

void CPUParticles2D::set_color_ramp(const Ref<Gradient>& p_ramp) { color_ramp = p_ramp; }

Ref<Gradient> CPUParticles2D::get_color_ramp() const { return color_ramp; }

void CPUParticles2D::set_color_initial_ramp(const Ref<Gradient>& p_ramp)
{
	color_initial_ramp = p_ramp;
}

Ref<Gradient> CPUParticles2D::get_color_initial_ramp() const { return color_initial_ramp; }

void CPUParticles2D::set_particle_flag(ParticleFlags p_particle_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_particle_flag, PARTICLE_FLAG_MAX);
	particle_flags[p_particle_flag] = p_enable;
}

bool CPUParticles2D::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

void CPUParticles2D::set_emission_points(const Vector<Vector2>& p_points)
{
	emission_points = p_points;
}

void CPUParticles2D::set_emission_normals(const Vector<Vector2>& p_normals)
{
	emission_normals = p_normals;
}

void CPUParticles2D::set_emission_colors(const Vector<Color>& p_colors)
{
	emission_colors = p_colors;
}

void CPUParticles2D::set_emission_ring_inner_radius(real_t p_inner_radius)
{
	emission_ring_inner_radius = p_inner_radius;
}

void CPUParticles2D::set_emission_ring_radius(real_t p_ring_radius)
{
	emission_ring_radius = p_ring_radius;
}

real_t CPUParticles2D::get_emission_sphere_radius() const { return emission_sphere_radius; }

Vector2 CPUParticles2D::get_emission_rect_extents() const { return emission_rect_extents; }

Vector<Vector2> CPUParticles2D::get_emission_points() const { return emission_points; }

Vector<Vector2> CPUParticles2D::get_emission_normals() const { return emission_normals; }

Vector<Color> CPUParticles2D::get_emission_colors() const { return emission_colors; }

real_t CPUParticles2D::get_emission_ring_inner_radius() const { return emission_ring_inner_radius; }

real_t CPUParticles2D::get_emission_ring_radius() const { return emission_ring_radius; }

CPUParticles2D::EmissionShape CPUParticles2D::get_emission_shape() const { return emission_shape; }

void CPUParticles2D::set_gravity(const Vector2& p_gravity) { gravity = p_gravity; }

Vector2 CPUParticles2D::get_gravity() const { return gravity; }

void CPUParticles2D::set_scale_curve_x(Ref<Curve> p_scale_curve) { scale_curve_x = p_scale_curve; }

void CPUParticles2D::set_scale_curve_y(Ref<Curve> p_scale_curve) { scale_curve_y = p_scale_curve; }

Ref<Curve> CPUParticles2D::get_scale_curve_x() const { return scale_curve_x; }

Ref<Curve> CPUParticles2D::get_scale_curve_y() const { return scale_curve_y; }

bool CPUParticles2D::get_split_scale() { return split_scale; }

void CPUParticles2D::set_use_fixed_seed(bool p_use_fixed_seed)
{
	if (p_use_fixed_seed == use_fixed_seed) {
		return;
	}
	use_fixed_seed = p_use_fixed_seed;
}

bool CPUParticles2D::get_use_fixed_seed() const { return use_fixed_seed; }

void CPUParticles2D::set_seed(uint32_t p_seed) { seed = p_seed; }

uint32_t CPUParticles2D::get_seed() const { return seed; }

void CPUParticles2D::request_particles_process(
	real_t p_request_process_time, real_t p_request_process_time_residual)
{
	_request_process_time = p_request_process_time;
	_request_process_time_residual = p_request_process_time_residual;
	_update_internal();
}

static uint32_t idhash(uint32_t x)
{
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = (x >> uint32_t(16)) ^ x;
	return x;
}

static real_t rand_from_seed(uint32_t& seed)
{
	int k;
	int s = int(seed);
	if (s == 0) {
		s = 305420679;
	}
	k = s / 127773;
	s = 16807 * (s - k * 127773) - 2836 * k;
	if (s < 0) {
		s += 2147483647;
	}
	seed = uint32_t(s);
	return (seed % uint32_t(65536)) / 65535.0;
}

void CPUParticles2D::_update_internal()
{
	if (particles.is_empty() || !is_visible_in_tree()) {
		_set_do_redraw(false);
		return;
	}

	// Change update mode?
	_refresh_interpolation_state();

	double delta = get_process_delta_time();
	if (!active && !emitting) {
		set_process_internal(false);
		_set_do_redraw(false);

		// reset variables
		time = 0;
		frame_remainder = 0;
		cycle = 0;
		return;
	}

	_set_do_redraw(true);
	{
		float todo = time == 0 ? pre_process_time : 0;
		todo = todo > _request_process_time ? todo : _request_process_time;
		todo = todo > _request_process_time_residual ? todo : _request_process_time_residual;

		if (todo > 0.0) {
			real_t frame_time;
			if (fixed_fps > 0) {
				frame_time = 1.0 / fixed_fps;
			}
			else {
				frame_time = 1.0 / 30.0;
			}

			float tmp_scale = speed_scale;
			// We need this otherwise the speed scale of the particle system influences the `todo`.
			speed_scale = 1.0;
			if (time == 0) {
				todo = pre_process_time;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time > 0.0) {
				todo = _request_process_time;
				emitting = true;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time_residual > 0.0) {
				emitting = false;
				todo = _request_process_time_residual;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			speed_scale = tmp_scale;
		}
		_request_process_time = 0;
		_request_process_time_residual = 0;
	}
	double frame_time;
	if (fixed_fps > 0) {
		frame_time = 1.0 / fixed_fps;
	}
	else {
		frame_time = 1.0 / 30.0;
	}
	if (fixed_fps > 0) {
		double decr = frame_time;

		double ldelta = delta;
		if (ldelta > 0.1) { // avoid recursive stalls if fps goes below 10
			ldelta = 0.1;
		}
		else if (ldelta < 0.0) {
			ldelta = 0.0;
		}
		double todo = frame_remainder + ldelta;

		while (todo >= frame_time) {
			_particles_process(frame_time);
			todo -= decr;
		}
		frame_remainder = todo;

	}
	else {
		_particles_process(delta);
	}

	_update_particle_data_buffer();
}

void CPUParticles2D::_particles_process(double p_delta)
{
	p_delta *= speed_scale;

	int pcount = particles.size();
	Particle* w = particles.ptrw();

	Particle* parray = w;

	double prev_time = time;
	time += p_delta;
	if (time > lifetime) {
		time = Math::fmod(time, lifetime);
		cycle++;
		if (one_shot && cycle > 0) {
			set_emitting(false);
		}
	}

	Transform2D emission_xform;
	Transform2D velocity_xform;
	if (!local_coords) {
		if (!_interpolation_data.interpolated_follow) {
			emission_xform = get_global_transform();
		}
		else {
			TransformInterpolator::interpolate_transform_2d(_interpolation_data.global_xform_prev,
				_interpolation_data.global_xform_curr, emission_xform,
				Engine::get_physics_interpolation_fraction());
		}
		velocity_xform = emission_xform;
		velocity_xform[2] = Vector2();
	}

	double system_phase = time / lifetime;

	bool should_be_active = false;
	for (int i = 0; i < pcount; i++) {
		Particle& p = parray[i];

		if (!emitting && !p.active) {
			continue;
		}

		double local_delta = p_delta;

		// The phase is a ratio between 0 (birth) and 1 (end of life) for each particle.
		// While we use time in tests later on, for randomness we use the phase as done in the
		// original shader code, and we later multiply by lifetime to get the time.
		double restart_phase = double(i) / double(pcount);

		if (randomness_ratio > 0.0) {
			uint32_t _seed = cycle;
			if (restart_phase >= system_phase) {
				_seed -= uint32_t(1);
			}
			_seed *= uint32_t(pcount);
			_seed += uint32_t(i);
			double random = double(idhash(_seed) % uint32_t(65536)) / 65536.0;
			restart_phase += randomness_ratio * random * 1.0 / double(pcount);
		}

		restart_phase *= (1.0 - explosiveness_ratio);
		double restart_time = restart_phase * lifetime;
		bool restart = false;

		if (time > prev_time) {
			// restart_time >= prev_time is used so particles emit in the first frame they are
			// processed

			if (restart_time >= prev_time && restart_time < time) {
				restart = true;
				if (fractional_delta) {
					local_delta = time - restart_time;
				}
			}

		}
		else if (local_delta > 0.0) {
			if (restart_time >= prev_time) {
				restart = true;
				if (fractional_delta) {
					local_delta = lifetime - restart_time + time;
				}

			}
			else if (restart_time < time) {
				restart = true;
				if (fractional_delta) {
					local_delta = time - restart_time;
				}
			}
		}

		if (p.time * (1.0 - explosiveness_ratio) > p.lifetime) {
			restart = true;
		}

		float tv = 0.0;

		if (restart) {
			if (!emitting) {
				p.active = false;
				continue;
			}
			p.active = true;

			/*real_t tex_linear_velocity = 0;
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				tex_linear_velocity = curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY]->sample(0);
			}*/

			real_t tex_angle = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_angle = curve_parameters[PARAM_ANGLE]->sample(tv);
			}

			real_t tex_anim_offset = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_anim_offset = curve_parameters[PARAM_ANGLE]->sample(tv);
			}

			p.seed = seed + uint32_t(i) + i + cycle;
			rng->set_seed(p.seed);

			p.angle_rand = rng->randf();
			p.scale_rand = rng->randf();
			p.hue_rot_rand = rng->randf();
			p.anim_offset_rand = rng->randf();

			if (color_initial_ramp.is_valid()) {
				p.start_color_rand = color_initial_ramp->get_color_at_offset(rng->randf());
			}
			else {
				p.start_color_rand = Color(1, 1, 1, 1);
			}

			real_t angle1_rad =
				direction.angle() + Math::deg_to_rad((rng->randf() * 2.0 - 1.0) * spread);
			Vector2 rot = Vector2(Math::cos(angle1_rad), Math::sin(angle1_rad));
			p.velocity = rot * Math::lerp(parameters_min[PARAM_INITIAL_LINEAR_VELOCITY],
								   parameters_max[PARAM_INITIAL_LINEAR_VELOCITY], rng->randf());

			real_t base_angle = tex_angle * Math::lerp(parameters_min[PARAM_ANGLE],
												parameters_max[PARAM_ANGLE], p.angle_rand);
			p.rotation = Math::deg_to_rad(base_angle);

			p.custom[0] = 0.0; // unused
			p.custom[1] = 0.0; // phase [0..1]
			p.custom[2] =
				tex_anim_offset * Math::lerp(parameters_min[PARAM_ANIM_OFFSET],
									  parameters_max[PARAM_ANIM_OFFSET], p.anim_offset_rand);
			p.custom[3] = (1.0 - rng->randf() * lifetime_randomness);
			p.transform = Transform2D();
			p.time = 0;
			p.lifetime = lifetime * p.custom[3];
			p.base_color = Color(1, 1, 1, 1);

			switch (emission_shape) {
			case EMISSION_SHAPE_POINT: {
				// do none
			} break;
			case EMISSION_SHAPE_SPHERE: {
				real_t t = Math::TAU * rng->randf();
				real_t radius = emission_sphere_radius * rng->randf();
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_SPHERE_SURFACE: {
				real_t s = rng->randf(), t = Math::TAU * rng->randf();
				real_t radius = emission_sphere_radius * Math::sqrt(1.0 - s * s);
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_RECTANGLE: {
				p.transform[2] = Vector2(rng->randf() * 2.0 - 1.0, rng->randf() * 2.0 - 1.0) *
								 emission_rect_extents;
			} break;
			case EMISSION_SHAPE_POINTS:
			case EMISSION_SHAPE_DIRECTED_POINTS: {
				int pc = emission_points.size();
				if (pc == 0) {
					break;
				}

				int random_idx = Math::rand() % pc;

				p.transform[2] = emission_points.get(random_idx);

				if (emission_shape == EMISSION_SHAPE_DIRECTED_POINTS &&
					emission_normals.size() == pc) {
					Vector2 normal = emission_normals.get(random_idx);
					Transform2D m2;
					m2.columns[0] = normal;
					m2.columns[1] = normal.orthogonal();
					p.velocity = m2.basis_xform(p.velocity);
				}

				if (emission_colors.size() == pc) {
					p.base_color = emission_colors.get(random_idx);
				}
			} break;
			case EMISSION_SHAPE_RING: {
				real_t t = Math::TAU * Math::randf();
				real_t outer_sq = emission_ring_radius * emission_ring_radius;
				real_t inner_sq = emission_ring_inner_radius * emission_ring_inner_radius;
				real_t radius = Math::sqrt(Math::randf() * (outer_sq - inner_sq) + inner_sq);
				p.transform[2] = Vector2(Math::cos(t), Math::sin(t)) * radius;
			} break;
			case EMISSION_SHAPE_MAX: { // Max value for validity check.
				break;
			}
			}

			if (!local_coords) {
				p.velocity = velocity_xform.xform(p.velocity);
				p.transform = emission_xform * p.transform;
			}

		}
		else if (!p.active) {
			continue;
		}
		else if (p.time >= p.lifetime) {
			p.active = false;
			tv = 1.0;
		}
		else {
			uint32_t _seed = p.seed;
			p.time += local_delta;
			p.custom[1] = p.time / lifetime;
			tv = p.time / p.lifetime;

			real_t tex_linear_velocity = 1.0;
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				tex_linear_velocity = curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY]->sample(tv);
			}

			real_t tex_orbit_velocity = 1.0;
			if (curve_parameters[PARAM_ORBIT_VELOCITY].is_valid()) {
				tex_orbit_velocity = curve_parameters[PARAM_ORBIT_VELOCITY]->sample(tv);
			}

			real_t tex_angular_velocity = 1.0;
			if (curve_parameters[PARAM_ANGULAR_VELOCITY].is_valid()) {
				tex_angular_velocity = curve_parameters[PARAM_ANGULAR_VELOCITY]->sample(tv);
			}

			real_t tex_linear_accel = 1.0;
			if (curve_parameters[PARAM_LINEAR_ACCEL].is_valid()) {
				tex_linear_accel = curve_parameters[PARAM_LINEAR_ACCEL]->sample(tv);
			}

			real_t tex_tangential_accel = 1.0;
			if (curve_parameters[PARAM_TANGENTIAL_ACCEL].is_valid()) {
				tex_tangential_accel = curve_parameters[PARAM_TANGENTIAL_ACCEL]->sample(tv);
			}

			real_t tex_radial_accel = 1.0;
			if (curve_parameters[PARAM_RADIAL_ACCEL].is_valid()) {
				tex_radial_accel = curve_parameters[PARAM_RADIAL_ACCEL]->sample(tv);
			}

			real_t tex_damping = 1.0;
			if (curve_parameters[PARAM_DAMPING].is_valid()) {
				tex_damping = curve_parameters[PARAM_DAMPING]->sample(tv);
			}

			real_t tex_angle = 1.0;
			if (curve_parameters[PARAM_ANGLE].is_valid()) {
				tex_angle = curve_parameters[PARAM_ANGLE]->sample(tv);
			}
			real_t tex_anim_speed = 1.0;
			if (curve_parameters[PARAM_ANIM_SPEED].is_valid()) {
				tex_anim_speed = curve_parameters[PARAM_ANIM_SPEED]->sample(tv);
			}

			real_t tex_anim_offset = 1.0;
			if (curve_parameters[PARAM_ANIM_OFFSET].is_valid()) {
				tex_anim_offset = curve_parameters[PARAM_ANIM_OFFSET]->sample(tv);
			}

			Vector2 force = gravity;
			Vector2 pos = p.transform[2];

			// apply linear acceleration
			force += p.velocity.length() > 0.0f
						 ? p.velocity.normalized() * tex_linear_accel *
							   Math::lerp(parameters_min[PARAM_LINEAR_ACCEL],
								   parameters_max[PARAM_LINEAR_ACCEL], rand_from_seed(_seed))
						 : Vector2();
			// apply radial acceleration
			Vector2 org = emission_xform[2];
			Vector2 diff = pos - org;
			force += diff.length() > 0.0f
						 ? diff.normalized() *
							   (tex_radial_accel)*Math::lerp(parameters_min[PARAM_RADIAL_ACCEL],
								   parameters_max[PARAM_RADIAL_ACCEL], rand_from_seed(_seed))
						 : Vector2();
			// apply tangential acceleration;
			Vector2 yx = Vector2(diff.y, diff.x);
			force +=
				yx.length() > 0.0f
					? (yx * Vector2(-1.0f, 1.0f)).normalized() *
						  (tex_tangential_accel * Math::lerp(parameters_min[PARAM_TANGENTIAL_ACCEL],
													  parameters_max[PARAM_TANGENTIAL_ACCEL],
													  rand_from_seed(_seed)))
					: Vector2();
			// apply attractor forces
			p.velocity += force * local_delta;
			// orbit velocity
			real_t orbit_amount = tex_orbit_velocity *
								  Math::lerp(parameters_min[PARAM_ORBIT_VELOCITY],
									  parameters_max[PARAM_ORBIT_VELOCITY], rand_from_seed(_seed));
			if (orbit_amount != 0.0) {
				real_t ang = orbit_amount * local_delta * Math::TAU;
				// Not sure why the ParticleProcessMaterial code uses a clockwise rotation matrix,
				// but we use -ang here to reproduce its behavior.
				Transform2D rot = Transform2D(-ang, Vector2());
				p.transform[2] -= diff;
				p.transform[2] += rot.basis_xform(diff);
			}
			if (curve_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
				p.velocity = p.velocity.normalized() * tex_linear_velocity;
			}

			if (parameters_max[PARAM_DAMPING] + tex_damping > 0.0) {
				real_t v = p.velocity.length();
				real_t damp =
					tex_damping * Math::lerp(parameters_min[PARAM_DAMPING],
									  parameters_max[PARAM_DAMPING], rand_from_seed(_seed));
				v -= damp * local_delta;
				if (v < 0.0) {
					p.velocity = Vector2();
				}
				else {
					p.velocity = p.velocity.normalized() * v;
				}
			}
			real_t base_angle = (tex_angle)*Math::lerp(
				parameters_min[PARAM_ANGLE], parameters_max[PARAM_ANGLE], p.angle_rand);
			base_angle += p.custom[1] * lifetime * tex_angular_velocity *
						  Math::lerp(parameters_min[PARAM_ANGULAR_VELOCITY],
							  parameters_max[PARAM_ANGULAR_VELOCITY], rand_from_seed(_seed));
			p.rotation = Math::deg_to_rad(base_angle); // angle
			p.custom[2] =
				tex_anim_offset * Math::lerp(parameters_min[PARAM_ANIM_OFFSET],
									  parameters_max[PARAM_ANIM_OFFSET], p.anim_offset_rand) +
				tv * tex_anim_speed *
					Math::lerp(parameters_min[PARAM_ANIM_SPEED], parameters_max[PARAM_ANIM_SPEED],
						rand_from_seed(_seed));
		}
		// apply color
		// apply hue rotation

		Vector2 tex_scale = Vector2(1.0, 1.0);
		if (split_scale) {
			if (scale_curve_x.is_valid()) {
				tex_scale.x = scale_curve_x->sample(tv);
			}
			else {
				tex_scale.x = 1.0;
			}
			if (scale_curve_y.is_valid()) {
				tex_scale.y = scale_curve_y->sample(tv);
			}
			else {
				tex_scale.y = 1.0;
			}
		}
		else {
			if (curve_parameters[PARAM_SCALE].is_valid()) {
				real_t tmp_scale = curve_parameters[PARAM_SCALE]->sample(tv);
				tex_scale.x = tmp_scale;
				tex_scale.y = tmp_scale;
			}
		}

		real_t tex_hue_variation = 0.0;
		if (curve_parameters[PARAM_HUE_VARIATION].is_valid()) {
			tex_hue_variation = curve_parameters[PARAM_HUE_VARIATION]->sample(tv);
		}

		real_t hue_rot_angle = (tex_hue_variation)*Math::TAU *
							   Math::lerp(parameters_min[PARAM_HUE_VARIATION],
								   parameters_max[PARAM_HUE_VARIATION], p.hue_rot_rand);
		real_t hue_rot_c = Math::cos(hue_rot_angle);
		real_t hue_rot_s = Math::sin(hue_rot_angle);

		Basis hue_rot_mat;
		{
			Basis mat1(0.299, 0.587, 0.114, 0.299, 0.587, 0.114, 0.299, 0.587, 0.114);
			Basis mat2(0.701, -0.587, -0.114, -0.299, 0.413, -0.114, -0.300, -0.588, 0.886);
			Basis mat3(0.168, 0.330, -0.497, -0.328, 0.035, 0.292, 1.250, -1.050, -0.203);

			for (int j = 0; j < 3; j++) {
				hue_rot_mat[j] = mat1[j] + mat2[j] * hue_rot_c + mat3[j] * hue_rot_s;
			}
		}

		if (color_ramp.is_valid()) {
			p.color = color_ramp->get_color_at_offset(tv) * color;
		}
		else {
			p.color = color;
		}

		Vector3 color_rgb = hue_rot_mat.xform_inv(Vector3(p.color.r, p.color.g, p.color.b));
		p.color.r = color_rgb.x;
		p.color.g = color_rgb.y;
		p.color.b = color_rgb.z;

		p.color *= p.base_color * p.start_color_rand;

		if (particle_flags[PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY]) {
			if (p.velocity.length() > 0.0f) {
				p.transform.columns[1] = p.velocity;
			}

			p.transform.columns[1] = p.transform.columns[1].normalized();
			p.transform.columns[0] = p.transform.columns[1].orthogonal();
		}
		else {
			p.transform.columns[0] = Vector2(Math::cos(p.rotation), -Math::sin(p.rotation));
			p.transform.columns[1] = Vector2(Math::sin(p.rotation), Math::cos(p.rotation));
		}

		// scale by scale
		Vector2 base_scale = tex_scale * Math::lerp(parameters_min[PARAM_SCALE],
											 parameters_max[PARAM_SCALE], p.scale_rand);
		if (base_scale.x < 0.00001) {
			base_scale.x = 0.00001;
		}
		if (base_scale.y < 0.00001) {
			base_scale.y = 0.00001;
		}
		p.transform.columns[0] *= base_scale.x;
		p.transform.columns[1] *= base_scale.y;

		p.transform[2] += p.velocity * local_delta;

		should_be_active = true;
	}
	if (!Math::is_equal_approx(time, 0.0) && active && !should_be_active) {
		active = false;
	}
}

void CPUParticles2D::_update_particle_data_buffer()
{
	MutexLock lock(update_mutex);

	int pc = particles.size();

	int* ow;
	int* order = nullptr;

	float* w = particle_data.ptrw();
	const Particle* r = particles.ptr();
	float* ptr = w;

	if (draw_order != DRAW_ORDER_INDEX) {
		ow = particle_order.ptrw();
		order = ow;

		for (int i = 0; i < pc; i++) {
			order[i] = i;
		}
		if (draw_order == DRAW_ORDER_LIFETIME) {
			SortArray<int, SortLifetime> sorter;
			sorter.compare.particles = r;
			sorter.sort(order, pc);
		}
	}

	for (int i = 0; i < pc; i++) {
		int idx = order ? order[i] : i;

		Transform2D t = r[idx].transform;

		if (!local_coords) {
			t = inv_emission_transform * t;
		}

		if (r[idx].active) {
			ptr[0] = t.columns[0][0];
			ptr[1] = t.columns[1][0];
			ptr[2] = 0;
			ptr[3] = t.columns[2][0];
			ptr[4] = t.columns[0][1];
			ptr[5] = t.columns[1][1];
			ptr[6] = 0;
			ptr[7] = t.columns[2][1];

		}
		else {
			memset(ptr, 0, sizeof(float) * 8);
		}

		Color c = r[idx].color;

		ptr[8] = c.r;
		ptr[9] = c.g;
		ptr[10] = c.b;
		ptr[11] = c.a;

		ptr[12] = r[idx].custom[0];
		ptr[13] = r[idx].custom[1];
		ptr[14] = r[idx].custom[2];
		ptr[15] = r[idx].custom[3];

		ptr += 16;
	}
}

void CPUParticles2D::_update_render_thread()
{
	MutexLock lock(update_mutex);

	Renderer::multimesh_set_buffer(multimesh, particle_data);
}

#ifdef TOOLS_ENABLED
void CPUParticles2D::_draw_emission_gizmo()
{
	Color emission_ring_color = Color(0.8, 0.7, 0.4, 0.4);
	Transform2D gizmo_transform;
	if (!local_coords) {
		gizmo_transform = get_global_transform();
	}

	draw_set_transform_matrix(gizmo_transform);

	switch (emission_shape) {
	case CPUParticles2D::EMISSION_SHAPE_RECTANGLE:
		draw_rect(
			Rect2(-emission_rect_extents, emission_rect_extents * 2.0), emission_ring_color, false);
		break;
	case CPUParticles2D::EMISSION_SHAPE_SPHERE:
	case CPUParticles2D::EMISSION_SHAPE_SPHERE_SURFACE:
		draw_circle(Vector2(), emission_sphere_radius, emission_ring_color, false);
		break;
	default:

		break;
	}
}
#endif

CPUParticles2D::~CPUParticles2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(multimesh);
	Renderer::free_rid(mesh);
}

void CPUParticles2D::_set_do_redraw(bool p_do_redraw) {}

PackedStringArray CPUParticles2D::get_configuration_warnings() const { return PackedStringArray(); }

// ==========================================================================
// Merged from scene/2d/gpu_particles_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/os/os.h"
// --- gpu_particles_2d.compat.inc ---
#ifndef DISABLE_DEPRECATED


void GPUParticles2D::_restart_bind_compat_92089() { restart(false); }

void GPUParticles2D::_request_particles_process_bind_compat_109142(real_t p_time)
{
	request_particles_process(p_time, 0.);
}

void GPUParticles2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#include "scene/resources/atlas_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/curve_texture.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

void GPUParticles2D::set_emitting(bool p_emitting)
{
	// Do not return even if `p_emitting == emitting` because `emitting` is just an approximation.

	if (p_emitting && p_emitting != emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}
	if (p_emitting && one_shot) {
		if (!active && !emitting) {
			// Last cycle ended.
			active = true;
			time = 0;
			signal_canceled = false;
			emission_time = lifetime;
			active_time = lifetime * (2 - explosiveness_ratio);
		}
		else {
			signal_canceled = true;
		}
		set_process_internal(true);
	}
	else if (!p_emitting) {
		if (one_shot) {
			set_process_internal(true);
		}
		else {
			set_process_internal(false);
		}
	}

	emitting = p_emitting;
	Renderer::particles_set_emitting(particles, p_emitting);
}

void GPUParticles2D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	Renderer::particles_set_amount(particles, amount);
}

void GPUParticles2D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	Renderer::particles_set_lifetime(particles, lifetime);
}

void GPUParticles2D::set_one_shot(bool p_enable)
{
	one_shot = p_enable;
	Renderer::particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		set_process_internal(true);
		if (!one_shot) {
			Renderer::particles_restart(particles);
		}
	}

	if (!one_shot) {
		set_process_internal(false);
	}
}

void GPUParticles2D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	Renderer::particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles2D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	Renderer::particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles2D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	Renderer::particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles2D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	Renderer::particles_set_use_local_coordinates(particles, local_coords);
	set_notify_transform(!p_enable);
	if (!p_enable && is_inside_tree()) {
		_update_particle_emission_transform();
	}
}

void GPUParticles2D::_update_particle_emission_transform()
{
	Transform2D xf2d = get_global_transform();
	Transform3D xf;
	xf.basis.set_column(0, Vector3(xf2d.columns[0].x, xf2d.columns[0].y, 0));
	xf.basis.set_column(1, Vector3(xf2d.columns[1].x, xf2d.columns[1].y, 0));
	xf.set_origin(Vector3(xf2d.get_origin().x, xf2d.get_origin().y, 0));

	Renderer::particles_set_emission_transform(particles, xf);
}

void GPUParticles2D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	Renderer::particles_set_interp_to_end(particles, interp_to_end_factor);
}

bool GPUParticles2D::is_trail_enabled() const { return trail_enabled; }

double GPUParticles2D::get_trail_lifetime() const { return trail_lifetime; }

void GPUParticles2D::_update_collision_size()
{
	real_t csize = collision_base_size;

	if (texture.is_valid()) {
		csize *=
			(texture->get_width() + texture->get_height()) / 4.0; // half size since its a radius
	}

	Renderer::particles_set_collision_base_size(particles, csize);
}

void GPUParticles2D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	_update_collision_size();
}

real_t GPUParticles2D::get_collision_base_size() const { return collision_base_size; }

void GPUParticles2D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	Renderer::particles_set_speed_scale(particles, p_scale);
}

bool GPUParticles2D::is_emitting() const { return emitting; }

int GPUParticles2D::get_amount() const { return amount; }

double GPUParticles2D::get_lifetime() const { return lifetime; }

int GPUParticles2D::get_trail_sections() const { return trail_sections; }

int GPUParticles2D::get_trail_section_subdivisions() const { return trail_section_subdivisions; }

bool GPUParticles2D::get_one_shot() const { return one_shot; }

double GPUParticles2D::get_pre_process_time() const { return pre_process_time; }

real_t GPUParticles2D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t GPUParticles2D::get_randomness_ratio() const { return randomness_ratio; }

Rect2 GPUParticles2D::get_visibility_rect() const { return visibility_rect; }

bool GPUParticles2D::get_use_local_coordinates() const { return local_coords; }

Ref<Material> GPUParticles2D::get_process_material() const { return process_material; }

double GPUParticles2D::get_speed_scale() const { return speed_scale; }

void GPUParticles2D::set_draw_order(DrawOrder p_order)
{
	draw_order = p_order;
	Renderer::particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

GPUParticles2D::DrawOrder GPUParticles2D::get_draw_order() const { return draw_order; }

void GPUParticles2D::set_fixed_fps(int p_count)
{
	fixed_fps = p_count;
	Renderer::particles_set_fixed_fps(particles, p_count);
}

int GPUParticles2D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles2D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	Renderer::particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles2D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles2D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	Renderer::particles_set_interpolate(particles, p_enable);
}

bool GPUParticles2D::get_interpolate() const { return interpolate; }

float GPUParticles2D::get_interp_to_end() const { return interp_to_end_factor; }

void GPUParticles2D::set_use_fixed_seed(bool p_use_fixed_seed)
{
	if (p_use_fixed_seed == use_fixed_seed) {
		return;
	}
	use_fixed_seed = p_use_fixed_seed;
}

bool GPUParticles2D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles2D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	Renderer::particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles2D::get_seed() const { return seed; }

void GPUParticles2D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	Renderer::particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	if (p_requested_process_time > 0.0) {
		emitting = true;
		Renderer::particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

Rect2 GPUParticles2D::capture_rect() const
{
	AABB aabb = Renderer::particles_get_current_aabb(particles);
	Rect2 r;
	r.position.x = aabb.position.x;
	r.position.y = aabb.position.y;
	r.size.x = aabb.size.x;
	r.size.y = aabb.size.y;
	return r;
}

Ref<Texture2D> GPUParticles2D::get_texture() const { return texture; }

void GPUParticles2D::emit_particle(const Transform2D& p_transform2d, const Vector2& p_velocity2d,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Transform3D emit_transform;
	emit_transform.basis.set_column(
		0, Vector3(p_transform2d.columns[0].x, p_transform2d.columns[0].y, 0));
	emit_transform.basis.set_column(
		1, Vector3(p_transform2d.columns[1].x, p_transform2d.columns[1].y, 0));
	emit_transform.set_origin(
		Vector3(p_transform2d.get_origin().x, p_transform2d.get_origin().y, 0));
	Vector3 velocity = Vector3(p_velocity2d.x, p_velocity2d.y, 0);

	Renderer::particles_emit(
		particles, emit_transform, velocity, p_color, p_custom, p_emit_flags);
}

NodePath GPUParticles2D::get_sub_emitter() const { return sub_emitter; }

void GPUParticles2D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	Renderer::particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles2D::get_amount_ratio() const { return amount_ratio; }

void GPUParticles2D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	Renderer::particles_restart(particles);
	Renderer::particles_set_emitting(particles, true);

	emitting = true;
	active = true;
	signal_canceled = false;
	time = 0;
	emission_time = lifetime;
	active_time = lifetime * (2 - explosiveness_ratio);
	if (one_shot) {
		set_process_internal(true);
	}
}

#ifdef TOOLS_ENABLED
void GPUParticles2D::_draw_emission_gizmo()
{
	Ref<ParticleProcessMaterial> pm = process_material;
	Color emission_ring_color = Color(0.8, 0.7, 0.4, 0.4);
	if (pm.is_null()) {
		return;
	}
	draw_set_transform(
		Vector2(pm->get_emission_shape_offset().x, pm->get_emission_shape_offset().y), 0.0,
		Vector2(pm->get_emission_shape_scale().x, pm->get_emission_shape_scale().y));

	switch (pm->get_emission_shape()) {
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_BOX: {
		Vector2 extents2d =
			Vector2(pm->get_emission_box_extents().x, pm->get_emission_box_extents().y);
		draw_rect(Rect2(-extents2d, extents2d * 2.0), emission_ring_color, false);
		break;
	}
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_SPHERE:
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_SPHERE_SURFACE: {
		draw_circle(Vector2(), pm->get_emission_sphere_radius(), emission_ring_color, false);
		break;
	}
	case ParticleProcessMaterial::EmissionShape::EMISSION_SHAPE_RING: {
		Vector3 ring_axis = pm->get_emission_ring_axis();
		if (ring_axis.is_equal_approx(Vector3(0.0, 0.0, 1.0)) || ring_axis.is_zero_approx()) {
			draw_circle(
				Vector2(), pm->get_emission_ring_inner_radius(), emission_ring_color, false);
			draw_circle(Vector2(), pm->get_emission_ring_radius(), emission_ring_color, false);
		}
		else {
			Vector2 a = Vector2(
				pm->get_emission_ring_height() / -2.0, pm->get_emission_ring_radius() / -1.0);
			Vector2 b = Vector2(
				-a.x, MIN(a.y + std::tan((90.0 - pm->get_emission_ring_cone_angle()) * 0.01745329) *
									pm->get_emission_ring_height(),
						  0.0));
			Vector2 c = Vector2(b.x, -b.y);
			Vector2 d = Vector2(a.x, -a.y);
			if (ring_axis.is_equal_approx(Vector3(1.0, 0.0, 0.0))) {
				Vector<Vector2> pos = {a, b, b, c, c, d, d, a};
				draw_multiline(pos, emission_ring_color);
			}
			else if (ring_axis.is_equal_approx(Vector3(0.0, 1.0, 0.0))) {
				a = Vector2(a.y, a.x);
				b = Vector2(b.y, b.x);
				c = Vector2(c.y, c.x);
				d = Vector2(d.y, d.x);
				Vector<Vector2> pos = {a, b, b, c, c, d, d, a};
				draw_multiline(pos, emission_ring_color);
			}
		}
		break;
	}
	default: {
		break;
	}
	}
}
#endif

GPUParticles2D::GPUParticles2D()
{
	particles = Renderer::particles_create();
	Renderer::particles_set_mode(particles, RSE::PARTICLES_MODE_2D);

	mesh = Renderer::mesh_create();
	Renderer::particles_set_draw_passes(particles, 1);
	Renderer::particles_set_draw_pass_mesh(particles, 0, mesh);

	one_shot = false; // Needed so that set_emitting doesn't access uninitialized values
	set_emitting(true);
	set_one_shot(false);
	set_seed(Math::rand());
	set_use_fixed_seed(false);
	set_amount(8);
	set_amount_ratio(1.0);
	set_lifetime(1);
	set_fixed_fps(0);
	set_fractional_delta(true);
	set_interpolate(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_visibility_rect(Rect2(Vector2(-100, -100), Vector2(200, 200)));
	set_use_local_coordinates(false);
	set_draw_order(DRAW_ORDER_LIFETIME);
	set_speed_scale(1);
	set_fixed_fps(30);
	set_collision_base_size(collision_base_size);
}

GPUParticles2D::~GPUParticles2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(particles);
	Renderer::free_rid(mesh);
}

PackedStringArray GPUParticles2D::get_configuration_warnings() const { return PackedStringArray(); }

void GPUParticles2D::set_visibility_rect(const Rect2& p_visibility_rect) {}

// ============================================================================
// Merged from scene/2d/line_2d.cpp
// ============================================================================

#include "core/math/geometry_2d.h"
#include "line_2d.h"
// --- line_builder.h ---
#include "scene/2d/line_2d.h"

class LineBuilder {
public:
	// TODO Move in a struct and reference it
	// Input
	Vector<Vector2> points;
	Line2D::LineJointMode joint_mode = Line2D::LINE_JOINT_SHARP;
	Line2D::LineCapMode begin_cap_mode = Line2D::LINE_CAP_NONE;
	Line2D::LineCapMode end_cap_mode = Line2D::LINE_CAP_NONE;
	bool closed = false;
	float width = 10.0;
	Curve *curve = nullptr;
	Color default_color = Color(0.4, 0.5, 1);
	Gradient *gradient = nullptr;
	Line2D::LineTextureMode texture_mode = Line2D::LineTextureMode::LINE_TEXTURE_NONE;
	float sharp_limit = 2.f;
	int round_precision = 8;
	float tile_aspect = 1.f; // w/h
	// TODO offset_joints option (offers alternative implementation of round joints)

	// TODO Move in a struct and reference it
	// Output
	Vector<Vector2> vertices;
	Vector<Color> colors;
	Vector<Vector2> uvs;
	Vector<int> indices;

	LineBuilder();

	void build();

private:
	enum Orientation {
		UP = 0,
		DOWN = 1
	};

	// Triangle-strip methods
	void strip_begin(Vector2 up, Vector2 down, Color color, float uvx);
	void strip_new_quad(Vector2 up, Vector2 down, Color color, float uvx);
	void strip_add_quad(Vector2 up, Vector2 down, Color color, float uvx);
	void strip_add_tri(Vector2 up, Orientation orientation);
	void strip_add_arc(Vector2 center, float angle_delta, Orientation orientation);

	void new_arc(Vector2 center, Vector2 vbegin, float angle_delta, Color color, Rect2 uv_rect);

private:
	bool _interpolate_color = false;
	int _last_index[2] = {}; // Index of last up and down vertices of the strip
};
#include "servers/rendering/renderer.h"

Line2D::Line2D() {}

#ifdef DEBUG_ENABLED
Rect2 Line2D::_edit_get_rect() const
{
	if (_points.is_empty()) {
		return Rect2(0, 0, 0, 0);
	}
	Vector2 min = _points[0];
	Vector2 max = min;
	for (int i = 1; i < _points.size(); i++) {
		min = min.min(_points[i]);
		max = max.max(_points[i]);
	}
	return Rect2(min, max - min).grow(_width);
}

bool Line2D::_edit_use_rect() const { return true; }

bool Line2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	const real_t d = _width / 2 + p_tolerance;
	const Vector2* points = _points.ptr();
	for (int i = 0; i < _points.size() - 1; i++) {
		Vector2 p = Geometry2D::get_closest_point_to_segment(p_point, points[i], points[i + 1]);
		if (p_point.distance_to(p) <= d) {
			return true;
		}
	}
	// Closing segment between the first and last point.
	if (_closed && _points.size() > 2) {
		Vector2 p = Geometry2D::get_closest_point_to_segment(
			p_point, points[0], points[_points.size() - 1]);
		if (p_point.distance_to(p) <= d) {
			return true;
		}
	}

	return false;
}
#endif

bool Line2D::is_closed() const { return _closed; }

float Line2D::get_width() const { return _width; }

Ref<Curve> Line2D::get_curve() const { return _curve; }

Vector<Vector2> Line2D::get_points() const { return _points; }

Vector2 Line2D::get_point_position(int i) const
{
	ERR_FAIL_INDEX_V(i, _points.size(), Vector2());
	return _points.get(i);
}

int Line2D::get_point_count() const { return _points.size(); }

Color Line2D::get_default_color() const { return _default_color; }

Ref<Gradient> Line2D::get_gradient() const { return _gradient; }

Ref<Texture2D> Line2D::get_texture() const { return _texture; }

Line2D::LineTextureMode Line2D::get_texture_mode() const { return _texture_mode; }

Line2D::LineJointMode Line2D::get_joint_mode() const { return _joint_mode; }

Line2D::LineCapMode Line2D::get_begin_cap_mode() const { return _begin_cap_mode; }

Line2D::LineCapMode Line2D::get_end_cap_mode() const { return _end_cap_mode; }

void Line2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		_draw();
	} break;
	}
}

float Line2D::get_sharp_limit() const { return _sharp_limit; }

int Line2D::get_round_precision() const { return _round_precision; }

bool Line2D::get_antialiased() const { return _antialiased; }



void Line2D::_draw() {}

// ==========================================================================
// Merged from scene/2d/line_builder.cpp
// ==========================================================================

#include "core/math/geometry_2d.h"

// Utility method.
static inline Vector2 interpolate(const Rect2 &r, const Vector2 &v) {
	return Vector2(
			Math::lerp(r.position.x, r.position.x + r.get_size().x, v.x),
			Math::lerp(r.position.y, r.position.y + r.get_size().y, v.y));
}

LineBuilder::LineBuilder() {
}

void LineBuilder::build() {
	// Need at least 2 points to draw a line, so clear the output and return.
	if (points.size() < 2) {
		vertices.clear();
		colors.clear();
		indices.clear();
		uvs.clear();
		return;
	}

	ERR_FAIL_COND(tile_aspect <= 0.f);

	const float hw = width / 2.f;
	const float hw_sq = hw * hw;
	const float sharp_limit_sq = sharp_limit * sharp_limit;
	const int point_count = points.size();
	const bool wrap_around = closed && point_count > 2;

	_interpolate_color = gradient != nullptr;
	const bool retrieve_curve = curve != nullptr;
	const bool distance_required = _interpolate_color || retrieve_curve ||
			texture_mode == Line2D::LINE_TEXTURE_TILE ||
			texture_mode == Line2D::LINE_TEXTURE_STRETCH;

	// Initial values

	Vector2 pos0 = points[0];
	Vector2 pos1 = points[1];
	Vector2 f0 = (pos1 - pos0).normalized();
	Vector2 u0 = f0.orthogonal();
	Vector2 pos_up0 = pos0;
	Vector2 pos_down0 = pos0;

	Color color0;
	Color color1;

	float current_distance0 = 0.f;
	float current_distance1 = 0.f;
	float total_distance = 0.f;

	float width_factor = 1.f;
	float modified_hw = hw;
	if (retrieve_curve) {
		width_factor = curve->sample_baked(0.f);
		modified_hw = hw * width_factor;
	}

	if (distance_required) {
		// Calculate the total distance.
		for (int i = 1; i < point_count; ++i) {
			total_distance += points[i].distance_to(points[i - 1]);
		}
		if (wrap_around) {
			total_distance += points[point_count - 1].distance_to(pos0);
		} else {
			// Adjust the total distance.
			// The line's outer length may be a little higher due to the end caps.
			if (begin_cap_mode == Line2D::LINE_CAP_BOX || begin_cap_mode == Line2D::LINE_CAP_ROUND) {
				total_distance += modified_hw;
			}
			if (end_cap_mode == Line2D::LINE_CAP_BOX || end_cap_mode == Line2D::LINE_CAP_ROUND) {
				if (retrieve_curve) {
					total_distance += hw * curve->sample_baked(1.f);
				} else {
					total_distance += hw;
				}
			}
		}
	}

	if (point_count < 2 || (distance_required && Math::is_zero_approx(total_distance))) {
		// Zero-length line, nothing to build.
		return;
	}

	if (_interpolate_color) {
		color0 = gradient->get_color(0);
	} else {
		colors.push_back(default_color);
	}

	float uvx0 = 0.f;
	float uvx1 = 0.f;

	pos_up0 += u0 * modified_hw;
	pos_down0 -= u0 * modified_hw;

	// Begin cap
	if (!wrap_around) {
		if (begin_cap_mode == Line2D::LINE_CAP_BOX) {
			// Push back first vertices a little bit.
			pos_up0 -= f0 * modified_hw;
			pos_down0 -= f0 * modified_hw;

			current_distance0 += modified_hw;
			current_distance1 = current_distance0;
		} else if (begin_cap_mode == Line2D::LINE_CAP_ROUND) {
			if (texture_mode == Line2D::LINE_TEXTURE_TILE) {
				uvx0 = width_factor * 0.5f / tile_aspect;
			} else if (texture_mode == Line2D::LINE_TEXTURE_STRETCH) {
				uvx0 = width * width_factor / total_distance;
			}
			new_arc(pos0, pos_up0 - pos0, -Math::PI, color0, Rect2(0.f, 0.f, uvx0 * 2, 1.f));
			current_distance0 += modified_hw;
			current_distance1 = current_distance0;
		}
		strip_begin(pos_up0, pos_down0, color0, uvx0);
	}

	/*
	 *  pos_up0 ------------- pos_up1 --------------------
	 *     |                     |
	 *   pos0 - - - - - - - - - pos1 - - - - - - - - - pos2
	 *     |                     |
	 * pos_down0 ------------ pos_down1 ------------------
	 *
	 *   i-1                     i                      i+1
	 */

	// http://labs.hyperandroid.com/tag/opengl-lines
	// (not the same implementation but visuals help a lot)

	// If the polyline wraps around, then draw two more segments with joints:
	// The last one, which should normally end with an end cap, and the one that matches the end and the beginning.
	int segments_count = wrap_around ? point_count : (point_count - 2);
	// The wraparound case starts with a "fake walk" from the end of the polyline
	// to its beginning, so that its first joint is correct, without drawing anything.
	int first_point = wrap_around ? -1 : 1;

	// If the line wraps around, these variables will be used for the final segment.
	Vector2 first_pos_up, first_pos_down;
	bool is_first_joint_sharp = false;

	// For each additional segment
	for (int i = first_point; i <= segments_count; ++i) {
		pos1 = points[(i == -1) ? point_count - 1 : i % point_count]; // First point.
		Vector2 pos2 = points[(i + 1) % point_count]; // Second point.

		Vector2 f1 = (pos2 - pos1).normalized();
		Vector2 u1 = f1.orthogonal();

		// Determine joint orientation.
		float dp = u0.dot(f1);
		const Orientation orientation = (dp > 0.f ? UP : DOWN);

		if (distance_required && i >= 1) {
			current_distance1 += pos0.distance_to(pos1);
		}
		if (_interpolate_color) {
			color1 = gradient->get_color_at_offset(current_distance1 / total_distance);
		}
		if (retrieve_curve) {
			width_factor = curve->sample_baked(current_distance1 / total_distance);
			modified_hw = hw * width_factor;
		}

		Vector2 inner_normal0 = u0 * modified_hw;
		Vector2 inner_normal1 = u1 * modified_hw;
		if (orientation == DOWN) {
			inner_normal0 = -inner_normal0;
			inner_normal1 = -inner_normal1;
		}

		/*
		 * ---------------------------
		 *                        /
		 * 0                     /    1
		 *                      /          /
		 * --------------------x------    /
		 *                    /          /    (here shown with orientation == DOWN)
		 *                   /          /
		 *                  /          /
		 *                 /          /
		 *                     2     /
		 *                          /
		 */

		// Find inner intersection at the joint.
		Vector2 corner_pos_in, corner_pos_out;
		bool is_intersecting = Geometry2D::segment_intersects_segment(
				pos0 + inner_normal0, pos1 + inner_normal0,
				pos1 + inner_normal1, pos2 + inner_normal1,
				&corner_pos_in);

		if (is_intersecting) {
			// Inner parts of the segments intersect.
			corner_pos_out = 2.f * pos1 - corner_pos_in;
		} else {
			// No intersection, segments are too sharp or they overlap.
			corner_pos_in = pos1 + inner_normal0;
			corner_pos_out = pos1 - inner_normal0;
		}

		Vector2 corner_pos_up, corner_pos_down;
		if (orientation == UP) {
			corner_pos_up = corner_pos_in;
			corner_pos_down = corner_pos_out;
		} else {
			corner_pos_up = corner_pos_out;
			corner_pos_down = corner_pos_in;
		}

		Line2D::LineJointMode current_joint_mode = joint_mode;

		Vector2 pos_up1, pos_down1;
		if (is_intersecting) {
			// Fallback on bevel if sharp angle is too high (because it would produce very long miters).
			float width_factor_sq = width_factor * width_factor;
			if (current_joint_mode == Line2D::LINE_JOINT_SHARP && corner_pos_out.distance_squared_to(pos1) / (hw_sq * width_factor_sq) > sharp_limit_sq) {
				current_joint_mode = Line2D::LINE_JOINT_BEVEL;
			}
			if (current_joint_mode == Line2D::LINE_JOINT_SHARP) {
				// In this case, we won't create joint geometry,
				// The previous and next line quads will directly share an edge.
				pos_up1 = corner_pos_up;
				pos_down1 = corner_pos_down;
			} else {
				// Bevel or round
				if (orientation == UP) {
					pos_up1 = corner_pos_up;
					pos_down1 = pos1 - u0 * modified_hw;
				} else {
					pos_up1 = pos1 + u0 * modified_hw;
					pos_down1 = corner_pos_down;
				}
			}
		} else {
			// No intersection: fallback
			if (current_joint_mode == Line2D::LINE_JOINT_SHARP) {
				// There is no fallback implementation for LINE_JOINT_SHARP so switch to the LINE_JOINT_BEVEL.
				current_joint_mode = Line2D::LINE_JOINT_BEVEL;
			}
			pos_up1 = corner_pos_up;
			pos_down1 = corner_pos_down;
		}

		// Triangles are clockwise.
		if (texture_mode == Line2D::LINE_TEXTURE_TILE) {
			uvx1 = current_distance1 / (width * tile_aspect);
		} else if (texture_mode == Line2D::LINE_TEXTURE_STRETCH) {
			uvx1 = current_distance1 / total_distance;
		}

		// Swap vars for use in the next line.
		color0 = color1;
		u0 = u1;
		f0 = f1;
		pos0 = pos1;
		if (is_intersecting) {
			if (current_joint_mode == Line2D::LINE_JOINT_SHARP) {
				pos_up0 = pos_up1;
				pos_down0 = pos_down1;
			} else {
				if (orientation == UP) {
					pos_up0 = corner_pos_up;
					pos_down0 = pos1 - u1 * modified_hw;
				} else {
					pos_up0 = pos1 + u1 * modified_hw;
					pos_down0 = corner_pos_down;
				}
			}
		} else {
			pos_up0 = pos1 + u1 * modified_hw;
			pos_down0 = pos1 - u1 * modified_hw;
		}

		// End the "fake pass" in the closed line case before the drawing subroutine.
		if (i == -1) {
			continue;
		}

		// For wrap-around polylines, store some kind of start positions of the first joint for the final connection.
		if (wrap_around && i == 0) {
			Vector2 first_pos_center = (pos_up1 + pos_down1) / 2;
			float lerp_factor = 1.0 / width_factor;
			first_pos_up = first_pos_center.lerp(pos_up1, lerp_factor);
			first_pos_down = first_pos_center.lerp(pos_down1, lerp_factor);
			is_first_joint_sharp = current_joint_mode == Line2D::LINE_JOINT_SHARP;
		}

		// Add current line body quad.
		if (wrap_around && retrieve_curve && !is_first_joint_sharp && i == segments_count) {
			// If the width curve is not seamless, we might need to fetch the line's start points to use them for the final connection.
			Vector2 first_pos_center = (first_pos_up + first_pos_down) / 2;
			strip_add_quad(first_pos_center.lerp(first_pos_up, width_factor), first_pos_center.lerp(first_pos_down, width_factor), color1, uvx1);
			return;
		} else {
			strip_add_quad(pos_up1, pos_down1, color1, uvx1);
		}

		// From this point, bu0 and bd0 concern the next segment.
		// Add joint geometry.
		if (current_joint_mode != Line2D::LINE_JOINT_SHARP) {
			/* ________________ cbegin
			 *               / \
			 *              /   \
			 * ____________/_ _ _\ cend
			 *             |     |
			 *             |     |
			 *             |     |
			 */

			Vector2 cbegin, cend;
			if (orientation == UP) {
				cbegin = pos_down1;
				cend = pos_down0;
			} else {
				cbegin = pos_up1;
				cend = pos_up0;
			}

			if (current_joint_mode == Line2D::LINE_JOINT_BEVEL && !(wrap_around && i == segments_count)) {
				strip_add_tri(cend, orientation);
			} else if (current_joint_mode == Line2D::LINE_JOINT_ROUND && !(wrap_around && i == segments_count)) {
				Vector2 vbegin = cbegin - pos1;
				Vector2 vend = cend - pos1;
				// We want to use vbegin.angle_to(vend) below, which evaluates to
				// Math::atan2(vbegin.cross(vend), vbegin.dot(vend)) but we need to
				// calculate this ourselves as we need to check if the cross product
				// in that calculation ends up being -0.f and flip it if so, effectively
				// flipping the resulting angle_delta to not return -PI but +PI instead
				float cross_product = vbegin.cross(vend);
				float dot_product = vbegin.dot(vend);
				// Note that we're comparing against -0.f for clarity but 0.f would
				// match as well, therefore we need the explicit signbit check too.
				if (cross_product == -0.f && std::signbit(cross_product)) {
					cross_product = 0.f;
				}
				float angle_delta = Math::atan2(cross_product, dot_product);
				strip_add_arc(pos1, angle_delta, orientation);
			}

			if (!is_intersecting) {
				// In this case the joint is too corrupted to be reused,
				// start again the strip with fallback points
				strip_begin(pos_up0, pos_down0, color1, uvx1);
			}
		}
	}

	// Draw the last (or only) segment, with its end cap logic.
	if (!wrap_around) {
		pos1 = points[point_count - 1];

		if (distance_required) {
			current_distance1 += pos0.distance_to(pos1);
		}
		if (_interpolate_color) {
			color1 = gradient->get_color(gradient->get_point_count() - 1);
		}
		if (retrieve_curve) {
			width_factor = curve->sample_baked(1.f);
			modified_hw = hw * width_factor;
		}

		Vector2 pos_up1 = pos1 + u0 * modified_hw;
		Vector2 pos_down1 = pos1 - u0 * modified_hw;

		// Add extra distance for a box end cap.
		if (end_cap_mode == Line2D::LINE_CAP_BOX) {
			pos_up1 += f0 * modified_hw;
			pos_down1 += f0 * modified_hw;

			current_distance1 += modified_hw;
		}

		if (texture_mode == Line2D::LINE_TEXTURE_TILE) {
			uvx1 = current_distance1 / (width * tile_aspect);
		} else if (texture_mode == Line2D::LINE_TEXTURE_STRETCH) {
			uvx1 = current_distance1 / total_distance;
		}

		strip_add_quad(pos_up1, pos_down1, color1, uvx1);

		// Custom drawing for a round end cap.
		if (end_cap_mode == Line2D::LINE_CAP_ROUND) {
			// Note: color is not used in case we don't interpolate.
			Color color = _interpolate_color ? gradient->get_color(gradient->get_point_count() - 1) : Color(0, 0, 0);
			float dist = 0;
			if (texture_mode == Line2D::LINE_TEXTURE_TILE) {
				dist = width_factor / tile_aspect;
			} else if (texture_mode == Line2D::LINE_TEXTURE_STRETCH) {
				dist = width * width_factor / total_distance;
			}
			new_arc(pos1, pos_up1 - pos1, Math::PI, color, Rect2(uvx1 - 0.5f * dist, 0.f, dist, 1.f));
		}
	}
}

void LineBuilder::strip_begin(Vector2 up, Vector2 down, Color color, float uvx) {
	int vi = vertices.size();

	vertices.push_back(up);
	vertices.push_back(down);

	if (_interpolate_color) {
		colors.push_back(color);
		colors.push_back(color);
	}

	if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
		uvs.push_back(Vector2(uvx, 0.f));
		uvs.push_back(Vector2(uvx, 1.f));
	}

	_last_index[UP] = vi;
	_last_index[DOWN] = vi + 1;
}

void LineBuilder::strip_add_quad(Vector2 up, Vector2 down, Color color, float uvx) {
	int vi = vertices.size();

	vertices.push_back(up);
	vertices.push_back(down);

	if (_interpolate_color) {
		colors.push_back(color);
		colors.push_back(color);
	}

	if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
		uvs.push_back(Vector2(uvx, 0.f));
		uvs.push_back(Vector2(uvx, 1.f));
	}

	indices.push_back(_last_index[UP]);
	indices.push_back(vi + 1);
	indices.push_back(_last_index[DOWN]);
	indices.push_back(_last_index[UP]);
	indices.push_back(vi);
	indices.push_back(vi + 1);

	_last_index[UP] = vi;
	_last_index[DOWN] = vi + 1;
}

void LineBuilder::strip_add_tri(Vector2 up, Orientation orientation) {
	int vi = vertices.size();

	vertices.push_back(up);

	if (_interpolate_color) {
		colors.push_back(colors[colors.size() - 1]);
	}

	Orientation opposite_orientation = orientation == UP ? DOWN : UP;

	if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
		// UVs are just one slice of the texture all along
		// (otherwise we can't share the bottom vertex)
		uvs.push_back(uvs[_last_index[opposite_orientation]]);
	}

	indices.push_back(_last_index[opposite_orientation]);
	indices.push_back(vi);
	indices.push_back(_last_index[orientation]);

	_last_index[opposite_orientation] = vi;
}

void LineBuilder::strip_add_arc(Vector2 center, float angle_delta, Orientation orientation) {
	// Take the two last vertices and extrude an arc made of triangles
	// that all share one of the initial vertices

	Orientation opposite_orientation = orientation == UP ? DOWN : UP;
	Vector2 vbegin = vertices[_last_index[opposite_orientation]] - center;
	float radius = vbegin.length();
	float angle_step = Math::PI / static_cast<float>(round_precision);
	float steps = Math::abs(angle_delta) / angle_step;

	if (angle_delta < 0.f) {
		angle_step = -angle_step;
	}

	float t = Vector2(1, 0).angle_to(vbegin);
	float end_angle = t + angle_delta;
	Vector2 rpos(0, 0);

	// Arc vertices
	for (int ti = 0; ti < steps; ++ti, t += angle_step) {
		rpos = center + Vector2(Math::cos(t), Math::sin(t)) * radius;
		strip_add_tri(rpos, orientation);
	}

	// Last arc vertex
	rpos = center + Vector2(Math::cos(end_angle), Math::sin(end_angle)) * radius;
	strip_add_tri(rpos, orientation);
}

void LineBuilder::new_arc(Vector2 center, Vector2 vbegin, float angle_delta, Color color, Rect2 uv_rect) {
	// Make a standalone arc that doesn't use existing vertices,
	// with undistorted UVs from within a square section

	float radius = vbegin.length();
	float angle_step = Math::PI / static_cast<float>(round_precision);
	float steps = Math::abs(angle_delta) / angle_step;

	if (angle_delta < 0.f) {
		angle_step = -angle_step;
	}

	float t = Vector2(1, 0).angle_to(vbegin);
	float end_angle = t + angle_delta;
	Vector2 rpos(0, 0);
	float tt_begin = -Math::PI / 2.0f;
	float tt = tt_begin;

	// Center vertice
	int vi = vertices.size();
	vertices.push_back(center);
	if (_interpolate_color) {
		colors.push_back(color);
	}
	if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
		uvs.push_back(interpolate(uv_rect, Vector2(0.5f, 0.5f)));
	}

	// Arc vertices
	for (int ti = 0; ti < steps; ++ti, t += angle_step) {
		Vector2 sc = Vector2(Math::cos(t), Math::sin(t));
		rpos = center + sc * radius;

		vertices.push_back(rpos);
		if (_interpolate_color) {
			colors.push_back(color);
		}
		if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
			Vector2 tsc = Vector2(Math::cos(tt), Math::sin(tt));
			uvs.push_back(interpolate(uv_rect, 0.5f * (tsc + Vector2(1.f, 1.f))));
			tt += angle_step;
		}
	}

	// Last arc vertex
	Vector2 sc = Vector2(Math::cos(end_angle), Math::sin(end_angle));
	rpos = center + sc * radius;
	vertices.push_back(rpos);
	if (_interpolate_color) {
		colors.push_back(color);
	}
	if (texture_mode != Line2D::LINE_TEXTURE_NONE) {
		tt = tt_begin + angle_delta;
		Vector2 tsc = Vector2(Math::cos(tt), Math::sin(tt));
		uvs.push_back(interpolate(uv_rect, 0.5f * (tsc + Vector2(1.f, 1.f))));
	}

	// Make up triangles
	int vi0 = vi;
	for (int ti = 0; ti < steps; ++ti) {
		indices.push_back(vi0);
		indices.push_back(++vi);
		indices.push_back(vi + 1);
	}
}

// ==========================================================================
// Merged from scene/2d/path_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/line_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

#ifdef DEBUG_ENABLED
Rect2 Path2D::_edit_get_rect() const
{
	if (curve.is_null() || curve->get_point_count() == 0) {
		return Rect2(0, 0, 0, 0);
	}

	Rect2 aabb = Rect2(curve->get_point_position(0), Vector2(0, 0));

	for (int i = 0; i < curve->get_point_count(); i++) {
		for (int j = 0; j <= 8; j++) {
			real_t frac = j / 8.0;
			Vector2 p = curve->sample(i, frac);
			aabb.expand_to(p);
		}
	}

	return aabb;
}

bool Path2D::_edit_use_rect() const { return curve.is_valid() && curve->get_point_count() != 0; }

bool Path2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	if (curve.is_null()) {
		return false;
	}

	for (int i = 0; i < curve->get_point_count(); i++) {
		Vector2 segment_a = curve->get_point_position(i);

		for (int j = 1; j <= 8; j++) {
			real_t frac = j / 8.0;
			const Vector2 segment_b = curve->sample(i, frac);

			Vector2 p = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
			if (p.distance_to(p_point) <= p_tolerance) {
				return true;
			}

			segment_a = segment_b;
		}
	}

	return false;
}
#endif

void Path2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
		_debug_create();
#endif
	} break;

	case NOTIFICATION_EXIT_TREE: {
#ifdef DEBUG_ENABLED
		_debug_free();
#endif
	} break;
	// Draw the curve if path debugging is enabled.
	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		_debug_update();
#endif
	} break;
	}
}

#ifdef DEBUG_ENABLED
void Path2D::_debug_create()
{
	ERR_FAIL_NULL(Renderer::data);

	if (debug_mesh_rid.is_null()) {
		debug_mesh_rid = Renderer::mesh_create();
	}

	if (debug_instance.is_null()) {
		debug_instance = Renderer::instance_create();
	}

	Renderer::instance_set_base(debug_instance, debug_mesh_rid);
	Renderer::instance_geometry_set_cast_shadows_setting(
		debug_instance, RSE::SHADOW_CASTING_SETTING_OFF);
}

void Path2D::_debug_free()
{
	ERR_FAIL_NULL(Renderer::data);

	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh_rid.is_valid()) {
		Renderer::free_rid(debug_mesh_rid);
		debug_mesh_rid = RID();
	}
}

#endif // DEBUG_ENABLED

Ref<Curve2D> Path2D::get_curve() const { return curve; }

void PathFollow2D::_update_transform()
{
	if (!path) {
		return;
	}

	Ref<Curve2D> c = path->get_curve();
	if (c.is_null()) {
		return;
	}

	real_t path_length = c->get_baked_length();
	if (path_length == 0) {
		return;
	}

	if (rotates) {
		Transform2D xform = c->sample_baked_with_rotation(progress, cubic);
		xform.translate_local(h_offset, v_offset);
		set_rotation(xform[0].angle());
		set_position(xform[2]);
	}
	else {
		Vector2 pos = c->sample_baked(progress, cubic);
		pos.x += h_offset;
		pos.y += v_offset;
		set_position(pos);
	}
}

void PathFollow2D::set_cubic_interpolation_enabled(bool p_enabled) { cubic = p_enabled; }

bool PathFollow2D::is_cubic_interpolation_enabled() const { return cubic; }

void PathFollow2D::set_progress(real_t p_progress)
{
	ERR_FAIL_COND(!std::isfinite(p_progress));
	progress = p_progress;
	if (path) {
		if (path->get_curve().is_valid()) {
			real_t path_length = path->get_curve()->get_baked_length();

			if (loop && path_length) {
				progress = Math::fposmod(progress, path_length);
				if (!Math::is_zero_approx(p_progress) && Math::is_zero_approx(progress)) {
					progress = path_length;
				}
			}
			else {
				progress = CLAMP(progress, 0, path_length);
			}
		}

		_update_transform();
	}
}

void PathFollow2D::set_h_offset(real_t p_h_offset)
{
	h_offset = p_h_offset;
	if (path) {
		_update_transform();
	}
}

real_t PathFollow2D::get_h_offset() const { return h_offset; }

void PathFollow2D::set_v_offset(real_t p_v_offset)
{
	v_offset = p_v_offset;
	if (path) {
		_update_transform();
	}
}

real_t PathFollow2D::get_v_offset() const { return v_offset; }

real_t PathFollow2D::get_progress() const { return progress; }

void PathFollow2D::set_progress_ratio(real_t p_ratio)
{
	ERR_FAIL_NULL_MSG(path, "Can only set progress ratio on a PathFollow2D that is the child of a "
							"Path2D which is itself part of the scene tree.");
	ERR_FAIL_COND_MSG(path->get_curve().is_null(),
		"Can't set progress ratio on a PathFollow2D that does not have a Curve.");
	ERR_FAIL_COND_MSG(!path->get_curve()->get_baked_length(),
		"Can't set progress ratio on a PathFollow2D that has a 0 length curve.");
	set_progress(p_ratio * path->get_curve()->get_baked_length());
}

real_t PathFollow2D::get_progress_ratio() const
{
	if (path && path->get_curve().is_valid() && path->get_curve()->get_baked_length()) {
		return get_progress() / path->get_curve()->get_baked_length();
	}
	else {
		return 0;
	}
}

void PathFollow2D::set_rotation_enabled(bool p_enabled)
{
	rotates = p_enabled;
	_update_transform();
}

bool PathFollow2D::is_rotation_enabled() const { return rotates; }

void PathFollow2D::set_loop(bool p_loop) { loop = p_loop; }

bool PathFollow2D::has_loop() const { return loop; }



void Path2D::_debug_update() {}

// ==========================================================================
// Merged from scene/2d/parallax_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/2d/line_2d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"

void Parallax2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		group_name = "__cameras_" + itos(get_viewport_rid().get_id());
		add_to_group(group_name);
		_update_repeat();
		_update_scroll();
	} break;

	case NOTIFICATION_READY: {
		_update_process();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		Point2 offset = scroll_offset;
		offset += autoscroll * get_process_delta_time();

		if (repeat_size.x) {
			offset.x = Math::fposmod(offset.x, repeat_size.x);
		}

		if (repeat_size.y) {
			offset.y = Math::fposmod(offset.y, repeat_size.y);
		}

		scroll_offset = offset;
		_update_scroll();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		remove_from_group(group_name);
	} break;
	}
}

#ifdef TOOLS_ENABLED
void Parallax2D::_edit_set_position(const Point2& p_position)
{
	// Avoids early return for grid snap compatibility
	scroll_offset = p_position;
	_update_scroll();
}
#endif // TOOLS_ENABLED

void Parallax2D::_camera_moved(
	const Transform2D& p_transform, const Point2& p_screen_offset, const Point2& p_adj_screen_pos)
{
	if (!ignore_camera_scroll) {
		if (get_viewport() && get_viewport()->is_snap_2d_transforms_to_pixel_enabled()) {
			Size2 vps = get_viewport_rect().size;
			Vector2 offset;
			offset.x = ((int)vps.width % 2) ? 0.0 : 0.5;
			offset.y = ((int)vps.height % 2) ? 0.0 : 0.5;
			set_screen_offset((p_adj_screen_pos + offset).floor());
		}
		else {
			set_screen_offset(p_adj_screen_pos);
		}
	}
}

void Parallax2D::_update_process()
{
	set_process_internal(!Engine::is_editor_hint() &&
						 (repeat_size.x || repeat_size.y) && (autoscroll.x || autoscroll.y));
}

void Parallax2D::_update_scroll()
{
	if (!is_inside_tree()) {
		return;
	}

	Point2 scroll_ofs = screen_offset;

	if (!Engine::is_editor_hint()) {
		Size2 vps = get_viewport_rect().size;

		if (limit_begin.x <= limit_end.x - vps.x) {
			scroll_ofs.x = CLAMP(scroll_ofs.x, limit_begin.x, limit_end.x - vps.x);
		}
		if (limit_begin.y <= limit_end.y - vps.y) {
			scroll_ofs.y = CLAMP(scroll_ofs.y, limit_begin.y, limit_end.y - vps.y);
		}
	}

	scroll_ofs *= scroll_scale;

	if (repeat_size.x) {
		real_t mod = Math::fposmod(scroll_ofs.x - scroll_offset.x, repeat_size.x * get_scale().x);
		scroll_ofs.x = screen_offset.x - mod;
	}
	else {
		scroll_ofs.x = screen_offset.x + scroll_offset.x - scroll_ofs.x;
	}

	if (repeat_size.y) {
		real_t mod = Math::fposmod(scroll_ofs.y - scroll_offset.y, repeat_size.y * get_scale().y);
		scroll_ofs.y = screen_offset.y - mod;
	}
	else {
		scroll_ofs.y = screen_offset.y + scroll_offset.y - scroll_ofs.y;
	}

	if (!follow_viewport) {
		scroll_ofs -= screen_offset;
	}

	set_position(scroll_ofs);
}

void Parallax2D::_update_repeat()
{
	if (!is_inside_tree()) {
		return;
	}

	Renderer::canvas_set_item_repeat(
		get_canvas_item(), repeat_size, repeat_times);
	Renderer::canvas_item_set_interpolated(get_canvas_item(), false);
}

void Parallax2D::set_scroll_scale(const Size2& p_scale) { scroll_scale = p_scale; }

Size2 Parallax2D::get_scroll_scale() const { return scroll_scale; }

void Parallax2D::set_repeat_size(const Size2& p_repeat_size)
{
	if (p_repeat_size == repeat_size) {
		return;
	}

	repeat_size = p_repeat_size.maxf(0);

	_update_process();
	_update_repeat();
	_update_scroll();
}

Size2 Parallax2D::get_repeat_size() const { return repeat_size; }

void Parallax2D::set_repeat_times(int p_repeat_times)
{
	if (p_repeat_times == repeat_times) {
		return;
	}

	repeat_times = MAX(p_repeat_times, 1);

	_update_repeat();
}

int Parallax2D::get_repeat_times() const { return repeat_times; }

void Parallax2D::set_scroll_offset(const Point2& p_offset)
{
	if (p_offset == scroll_offset) {
		return;
	}

	scroll_offset = p_offset;

	_update_scroll();
}

Point2 Parallax2D::get_scroll_offset() const { return scroll_offset; }

void Parallax2D::set_autoscroll(const Point2& p_autoscroll)
{
	if (p_autoscroll == autoscroll) {
		return;
	}

	autoscroll = p_autoscroll;

	_update_process();
	_update_scroll();
}

Point2 Parallax2D::get_autoscroll() const { return autoscroll; }

void Parallax2D::set_screen_offset(const Point2& p_offset)
{
	if (p_offset == screen_offset) {
		return;
	}

	screen_offset = p_offset;

	_update_scroll();
}

Point2 Parallax2D::get_screen_offset() const { return screen_offset; }

void Parallax2D::set_limit_begin(const Point2& p_offset) { limit_begin = p_offset; }

Point2 Parallax2D::get_limit_begin() const { return limit_begin; }

void Parallax2D::set_limit_end(const Point2& p_offset) { limit_end = p_offset; }

Point2 Parallax2D::get_limit_end() const { return limit_end; }

void Parallax2D::set_follow_viewport(bool p_follow) { follow_viewport = p_follow; }

bool Parallax2D::get_follow_viewport() { return follow_viewport; }

void Parallax2D::set_ignore_camera_scroll(bool p_ignore) { ignore_camera_scroll = p_ignore; }

bool Parallax2D::is_ignore_camera_scroll() { return ignore_camera_scroll; }

// ==========================================================================
// Merged from scene/2d/parallax_layer.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/2d/line_2d.h"
#include "scene/main/canvas_layer.h"
#include "servers/rendering/renderer.h"

void ParallaxLayer::set_motion_scale(const Size2& p_scale)
{
	motion_scale = p_scale;
	CanvasLayer* pb = dynamic_cast<CanvasLayer*>(get_parent());
	if (pb && pb->is_parallax_enabled() && is_inside_tree()) {
		Vector2 ofs = pb->get_final_offset();
		real_t scale = pb->get_scroll_scale();
		set_base_offset_and_scale(ofs, scale);
	}
}

Size2 ParallaxLayer::get_motion_scale() const { return motion_scale; }

void ParallaxLayer::set_motion_offset(const Size2& p_offset)
{
	motion_offset = p_offset;
	CanvasLayer* pb = dynamic_cast<CanvasLayer*>(get_parent());
	if (pb && pb->is_parallax_enabled() && is_inside_tree()) {
		Vector2 ofs = pb->get_final_offset();
		real_t scale = pb->get_scroll_scale();
		set_base_offset_and_scale(ofs, scale);
	}
}

Size2 ParallaxLayer::get_motion_offset() const { return motion_offset; }

void ParallaxLayer::set_mirroring(const Size2& p_mirroring)
{
	mirroring = p_mirroring.maxf(0);

	_update_mirroring();
}

Size2 ParallaxLayer::get_mirroring() const { return mirroring; }

void ParallaxLayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		orig_offset = get_position();
		orig_scale = get_scale();
		_update_mirroring();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (Engine::is_editor_hint()) {
			break;
		}

		set_position(orig_offset);
		set_scale(orig_scale);
	} break;
	}
}

void ParallaxLayer::set_base_offset_and_scale(const Point2& p_offset, real_t p_scale)
{
	if (!is_inside_tree()) {
		return;
	}
	if (Engine::is_editor_hint()) {
		return;
	}

	Point2 new_ofs = p_offset * motion_scale + motion_offset * p_scale + orig_offset * p_scale;

	if (mirroring.x) {
		real_t den = mirroring.x * p_scale;
		new_ofs.x -= den * std::ceil(new_ofs.x / den);
	}

	if (mirroring.y) {
		real_t den = mirroring.y * p_scale;
		new_ofs.y -= den * std::ceil(new_ofs.y / den);
	}

	set_position(new_ofs);
	set_scale(Vector2(1, 1) * p_scale * orig_scale);

	_update_mirroring();
}

PackedStringArray ParallaxLayer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	const CanvasLayer* pb = dynamic_cast<const CanvasLayer*>(get_parent());
	if (!pb || !pb->is_parallax_enabled()) {
		warnings.push_back(
			RTR("ParallaxLayer node only works when set as child of a parallax CanvasLayer node."));
	}

	return warnings;
}

void ParallaxLayer::_update_mirroring() {}

// ==========================================================================
// Merged from scene/2d/light_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/light_2d.h ---
#include "scene/2d/node_2d.h"

class Light2D : public Node2D
{
public:
	enum LightType
	{
		LIGHT_POINT,
		LIGHT_DIRECTIONAL,
	};

	enum ShadowFilter
	{
		SHADOW_FILTER_NONE,
		SHADOW_FILTER_PCF5,
		SHADOW_FILTER_PCF13,
		SHADOW_FILTER_MAX
	};

	enum BlendMode
	{
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MIX,
	};

private:
	LightType light_type = LIGHT_POINT;
	RID canvas_light;
	bool enabled = true;
	bool editor_only = false;
	bool shadow = false;
	Color color = Color(1, 1, 1);
	Color shadow_color = Color(0, 0, 0, 0);
	real_t height = 0.0;
	real_t energy = 1.0;
	int z_min = -1024;
	int z_max = 1024;
	int layer_min = 0;
	int layer_max = 0;
	int item_mask = 1;
	int item_shadow_mask = 1;
	real_t shadow_smooth = 0.0;
	ShadowFilter shadow_filter = SHADOW_FILTER_NONE;
	BlendMode blend_mode = BLEND_MODE_ADD;

	// Point light parameters
	real_t _scale = 1.0;
	Ref<Texture2D> texture;
	Vector2 texture_offset;

	// Directional light parameters
	real_t max_distance = 10000.0;

	void _update_light_visibility();

	virtual void owner_changed_notify() override;
	virtual void _physics_interpolated_changed() override;

protected:
	_FORCE_INLINE_ RID _get_light() const { return canvas_light; }

	void _notification(int p_what);

public:
#ifdef TOOLS_ENABLED
	virtual void _edit_set_pivot(const Point2& p_pivot) override;
	virtual Point2 _edit_get_pivot() const override;
	virtual bool _edit_use_pivot() const override;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	virtual Rect2 get_anchorable_rect() const override;

	void set_light_type(LightType p_type);
	LightType get_light_type() const { return light_type; }

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_editor_only(bool p_editor_only);
	bool is_editor_only() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	void set_energy(real_t p_energy);
	real_t get_energy() const;

	void set_z_range_min(int p_min_z);
	int get_z_range_min() const;

	void set_z_range_max(int p_max_z);
	int get_z_range_max() const;

	void set_layer_range_min(int p_min_layer);
	int get_layer_range_min() const;

	void set_layer_range_max(int p_max_layer);
	int get_layer_range_max() const;

	void set_item_cull_mask(int p_mask);
	int get_item_cull_mask() const;

	void set_item_shadow_cull_mask(int p_mask);
	int get_item_shadow_cull_mask() const;

	void set_shadow_enabled(bool p_enabled);
	bool is_shadow_enabled() const;

	void set_shadow_filter(ShadowFilter p_filter);
	ShadowFilter get_shadow_filter() const;

	void set_shadow_color(const Color& p_shadow_color);
	Color get_shadow_color() const;

	void set_shadow_smooth(real_t p_amount);
	real_t get_shadow_smooth() const;

	void set_blend_mode(BlendMode p_mode);
	BlendMode get_blend_mode() const;

	// Point parameters
	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_offset(const Vector2& p_offset);
	Vector2 get_texture_offset() const;

	void set_texture_scale(real_t p_scale);
	real_t get_texture_scale() const;

	// Directional parameters
	void set_max_distance(real_t p_distance);
	real_t get_max_distance() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	Light2D(LightType p_type = LIGHT_POINT);
	~Light2D();
};

// --- End scene/2d/light_2d.h ---

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light2D::owner_changed_notify()
{
	_update_light_visibility();
}

void Light2D::_update_light_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif // TOOLS_ENABLED

	Renderer::canvas_light_set_enabled(
		canvas_light, enabled && is_visible_in_tree() && editor_ok);
}

void Light2D::set_light_type(LightType p_type)
{
	light_type = p_type;
	if (!canvas_light.is_valid() || !Renderer::data) {
		return;
	}
	if (light_type == LIGHT_POINT) {
		Renderer::canvas_light_set_mode(canvas_light, RSE::CANVAS_LIGHT_MODE_POINT);
	}
	else {
		Renderer::canvas_light_set_mode(canvas_light, RSE::CANVAS_LIGHT_MODE_DIRECTIONAL);
		Renderer::canvas_light_set_directional_distance(canvas_light, max_distance);
	}
}

void Light2D::set_enabled(bool p_enabled)
{
	enabled = p_enabled;
	_update_light_visibility();
}

bool Light2D::is_enabled() const { return enabled; }

void Light2D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_light_visibility();
}

bool Light2D::is_editor_only() const { return editor_only; }

void Light2D::set_color(const Color& p_color)
{
	color = p_color;
	Renderer::canvas_light_set_color(canvas_light, color);
}

Color Light2D::get_color() const { return color; }

void Light2D::set_height(real_t p_height)
{
	height = p_height;
	Renderer::canvas_light_set_height(canvas_light, height);
}

real_t Light2D::get_height() const { return height; }

void Light2D::set_energy(real_t p_energy)
{
	energy = p_energy;
	Renderer::canvas_light_set_energy(canvas_light, energy);
}

real_t Light2D::get_energy() const { return energy; }

void Light2D::set_z_range_min(int p_min_z)
{
	z_min = p_min_z;
	Renderer::canvas_light_set_z_range(canvas_light, z_min, z_max);
}

int Light2D::get_z_range_min() const { return z_min; }

void Light2D::set_z_range_max(int p_max_z)
{
	z_max = p_max_z;
	Renderer::canvas_light_set_z_range(canvas_light, z_min, z_max);
}

int Light2D::get_z_range_max() const { return z_max; }

void Light2D::set_layer_range_min(int p_min_layer)
{
	layer_min = p_min_layer;
	Renderer::canvas_light_set_layer_range(canvas_light, layer_min, layer_max);
}

int Light2D::get_layer_range_min() const { return layer_min; }

void Light2D::set_layer_range_max(int p_max_layer)
{
	layer_max = p_max_layer;
	Renderer::canvas_light_set_layer_range(canvas_light, layer_min, layer_max);
}

int Light2D::get_layer_range_max() const { return layer_max; }

void Light2D::set_item_cull_mask(int p_mask)
{
	item_mask = p_mask;
	Renderer::canvas_light_set_item_cull_mask(canvas_light, item_mask);
}

int Light2D::get_item_cull_mask() const { return item_mask; }

void Light2D::set_item_shadow_cull_mask(int p_mask)
{
	item_shadow_mask = p_mask;
	Renderer::canvas_light_set_item_shadow_cull_mask(canvas_light, item_shadow_mask);
}

int Light2D::get_item_shadow_cull_mask() const { return item_shadow_mask; }

void Light2D::set_shadow_enabled(bool p_enabled)
{
	shadow = p_enabled;
	Renderer::canvas_light_set_shadow_enabled(canvas_light, shadow);
}

bool Light2D::is_shadow_enabled() const { return shadow; }

void Light2D::set_shadow_filter(ShadowFilter p_filter)
{
	ERR_FAIL_INDEX(p_filter, SHADOW_FILTER_MAX);
	shadow_filter = p_filter;
	Renderer::canvas_light_set_shadow_filter(
		canvas_light, RSE::CanvasLightShadowFilter(p_filter));
}

Light2D::ShadowFilter Light2D::get_shadow_filter() const { return shadow_filter; }

void Light2D::set_shadow_color(const Color& p_shadow_color)
{
	shadow_color = p_shadow_color;
	Renderer::canvas_light_set_shadow_color(canvas_light, shadow_color);
}

Color Light2D::get_shadow_color() const { return shadow_color; }

void Light2D::set_blend_mode(BlendMode p_mode)
{
	blend_mode = p_mode;
	Renderer::canvas_light_set_blend_mode(canvas_light, RSE::CanvasLightBlendMode(p_mode));
}

Light2D::BlendMode Light2D::get_blend_mode() const { return blend_mode; }

void Light2D::_physics_interpolated_changed()
{
	Renderer::canvas_light_set_interpolated(canvas_light, is_physics_interpolated());
}

void Light2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_CANVAS: {
		Renderer::canvas_light_attach_to_canvas(canvas_light, get_canvas());
		_update_light_visibility();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Renderer::canvas_light_set_transform(canvas_light, get_global_transform());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_light_visibility();
	} break;

	case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		if (is_visible_in_tree() && is_physics_interpolated_and_enabled()) {
			Renderer::canvas_light_set_transform(canvas_light, get_global_transform());
			Renderer::canvas_light_reset_physics_interpolation(canvas_light);
		}
	} break;

	case NOTIFICATION_EXIT_CANVAS: {
		Renderer::canvas_light_attach_to_canvas(canvas_light, RID());
		_update_light_visibility();
	} break;
	}
}

void Light2D::set_shadow_smooth(real_t p_amount)
{
	shadow_smooth = p_amount;
	Renderer::canvas_light_set_shadow_smooth(canvas_light, shadow_smooth);
}

real_t Light2D::get_shadow_smooth() const { return shadow_smooth; }

#ifdef TOOLS_ENABLED
void Light2D::_edit_set_pivot(const Point2& p_pivot)
{
	if (light_type == LIGHT_POINT) {
		set_position(get_transform().xform(p_pivot));
		set_texture_offset(get_texture_offset() - p_pivot);
	}
}

Point2 Light2D::_edit_get_pivot() const { return Vector2(); }

bool Light2D::_edit_use_pivot() const { return light_type == LIGHT_POINT; }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 Light2D::_edit_get_rect() const
{
	if (light_type != LIGHT_POINT || texture.is_null()) {
		return Rect2();
	}
	Size2 s = texture->get_size() * _scale;
	return Rect2(texture_offset - s / 2.0, s);
}

bool Light2D::_edit_use_rect() const { return light_type == LIGHT_POINT && texture.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 Light2D::get_anchorable_rect() const
{
	if (light_type != LIGHT_POINT || texture.is_null()) {
		return Rect2();
	}
	Size2 s = texture->get_size() * _scale;
	return Rect2(texture_offset - s / 2.0, s);
}

void Light2D::set_texture(const Ref<Texture2D>& p_texture)
{
	texture = p_texture;
	if (canvas_light.is_valid() && Renderer::data) {
		Renderer::canvas_light_set_texture(
			canvas_light, texture.is_valid() ? texture->get_rid() : RID());
	}
	item_rect_changed();
}

Ref<Texture2D> Light2D::get_texture() const { return texture; }

void Light2D::set_texture_offset(const Vector2& p_offset)
{
	texture_offset = p_offset;
	Renderer::canvas_light_set_texture_offset(canvas_light, texture_offset);
	item_rect_changed();
}

Vector2 Light2D::get_texture_offset() const { return texture_offset; }

void Light2D::set_texture_scale(real_t p_scale)
{
	_scale = p_scale;
	if (_scale == 0) {
		_scale = CMP_EPSILON;
	}
	Renderer::canvas_light_set_texture_scale(canvas_light, _scale);
	item_rect_changed();
}

real_t Light2D::get_texture_scale() const { return _scale; }

void Light2D::set_max_distance(real_t p_distance)
{
	max_distance = p_distance;
	if (light_type == LIGHT_DIRECTIONAL) {
		Renderer::canvas_light_set_directional_distance(canvas_light, max_distance);
	}
}

real_t Light2D::get_max_distance() const { return max_distance; }

PackedStringArray Light2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (light_type == LIGHT_POINT && texture.is_null()) {
		warnings.push_back(RTR(
			"A texture with the shape of the light must be supplied to the \"Texture\" property."));
	}

	return warnings;
}

Light2D::Light2D(LightType p_type) : light_type(p_type)
{
	if (Renderer::data) {
		canvas_light = Renderer::canvas_light_create();
		set_light_type(p_type);
	}
	set_notify_transform(true);
	set_hide_clip_children(true);
}

Light2D::~Light2D()
{
	if (canvas_light.is_valid() && Renderer::data) {
		Renderer::free_rid(canvas_light);
	}
}

// ==========================================================================
// Merged from scene/2d/light_occluder_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/line_2d.h"
#include "servers/rendering/renderer.h"

#define LINE_GRAB_WIDTH 8

#ifdef DEBUG_ENABLED
Rect2 OccluderPolygon2D::_edit_get_rect() const
{
	if (rect_cache_dirty) {
		if (closed) {
			const Vector2* r = polygon.ptr();
			item_rect = Rect2();
			for (int i = 0; i < polygon.size(); i++) {
				Vector2 pos = r[i];
				if (i == 0) {
					item_rect.position = pos;
				}
				else {
					item_rect.expand_to(pos);
				}
			}
			rect_cache_dirty = false;
		}
		else {
			if (polygon.is_empty()) {
				item_rect = Rect2();
			}
			else {
				Vector2 d = Vector2(LINE_GRAB_WIDTH, LINE_GRAB_WIDTH);
				item_rect = Rect2(polygon[0] - d, 2 * d);
				for (int i = 1; i < polygon.size(); i++) {
					item_rect.expand_to(polygon[i] - d);
					item_rect.expand_to(polygon[i] + d);
				}
			}
		}
	}

	return item_rect;
}

#endif // DEBUG_ENABLED

void OccluderPolygon2D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	rect_cache_dirty = true;
	Renderer::canvas_occluder_polygon_set_shape(occ_polygon, p_polygon, closed);
	emit_changed();
	update_configuration_warning();
}

Vector<Vector2> OccluderPolygon2D::get_polygon() const { return polygon; }

void OccluderPolygon2D::set_closed(bool p_closed)
{
	if (closed == p_closed) {
		return;
	}
	closed = p_closed;
	if (polygon.size()) {
		Renderer::canvas_occluder_polygon_set_shape(occ_polygon, polygon, closed);
	}
	emit_changed();
}

bool OccluderPolygon2D::is_closed() const { return closed; }

void OccluderPolygon2D::set_cull_mode(CullMode p_mode)
{
	cull = p_mode;
	Renderer::canvas_occluder_polygon_set_cull_mode(
		occ_polygon, RSE::CanvasOccluderPolygonCullMode(p_mode));
}

OccluderPolygon2D::CullMode OccluderPolygon2D::get_cull_mode() const { return cull; }

RID OccluderPolygon2D::get_rid() const { return occ_polygon; }

OccluderPolygon2D::OccluderPolygon2D()
{
	occ_polygon = Renderer::canvas_occluder_polygon_create();
}

OccluderPolygon2D::~OccluderPolygon2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(occ_polygon);
}

void LightOccluder2D::_physics_interpolated_changed()
{
	Renderer::canvas_light_occluder_set_interpolated(
		occluder, is_physics_interpolated());
}

#ifdef DEBUG_ENABLED
Rect2 LightOccluder2D::_edit_get_rect() const
{
	return occluder_polygon.is_valid() ? occluder_polygon->_edit_get_rect() : Rect2();
}

bool LightOccluder2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return occluder_polygon.is_valid()
			   ? occluder_polygon->_edit_is_selected_on_click(p_point, p_tolerance)
			   : false;
}
#endif // DEBUG_ENABLED

Ref<OccluderPolygon2D> LightOccluder2D::get_occluder_polygon() const { return occluder_polygon; }

void LightOccluder2D::set_occluder_light_mask(int p_mask)
{
	mask = p_mask;
	Renderer::canvas_light_occluder_set_light_mask(occluder, p_mask);
}

int LightOccluder2D::get_occluder_light_mask() const { return mask; }

PackedStringArray LightOccluder2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (occluder_polygon.is_null()) {
		warnings.push_back(
			RTR("An occluder polygon must be set (or drawn) for this occluder to take effect."));
	}

	if (occluder_polygon.is_valid() && occluder_polygon->get_polygon().is_empty()) {
		warnings.push_back(
			RTR("The occluder polygon for this occluder is empty. Please draw a polygon."));
	}

	return warnings;
}

void LightOccluder2D::set_as_sdf_collision(bool p_enable)
{
	sdf_collision = p_enable;
	Renderer::canvas_light_occluder_set_as_sdf_collision(occluder, sdf_collision);
}

bool LightOccluder2D::is_set_as_sdf_collision() const { return sdf_collision; }

LightOccluder2D::LightOccluder2D()
{
	occluder = Renderer::canvas_light_occluder_create();

	set_notify_transform(true);
	set_as_sdf_collision(true);
}

LightOccluder2D::~LightOccluder2D()
{
	ERR_FAIL_NULL(Renderer::data);

	Renderer::free_rid(occluder);
}

bool OccluderPolygon2D::_edit_is_selected_on_click(const Vector2& p_point, double p_tolerance) const
{
	return false;
}

void LightOccluder2D::set_occluder_polygon(Ref<OccluderPolygon2D> const&) {}

#undef LINE_GRAB_WIDTH

// ============================================================================
// Merged from scene/2d/sprite_2d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "sprite_2d.h"

#ifdef DEBUG_ENABLED
bool Sprite2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return is_pixel_opaque(p_point);
}

Rect2 Sprite2D::_edit_get_rect() const { return get_rect(); }

bool Sprite2D::_edit_use_rect() const { return texture.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 Sprite2D::get_anchorable_rect() const { return get_rect(); }

void Sprite2D::_get_rects(Rect2& r_src_rect, Rect2& r_dst_rect, bool& r_filter_clip_enabled) const
{
	Rect2 base_rect;

	if (region_enabled) {
		r_filter_clip_enabled = region_filter_clip_enabled;
		base_rect = region_rect;
	}
	else {
		r_filter_clip_enabled = false;
		base_rect = Rect2(0, 0, texture->get_width(), texture->get_height());
	}

	Size2 frame_size = base_rect.size / Size2(hframes, vframes);
	Point2 frame_offset = Point2(frame % hframes, frame / hframes);
	frame_offset *= frame_size;

	r_src_rect.size = frame_size;
	r_src_rect.position = base_rect.position + frame_offset;

	Point2 dest_offset = offset;
	if (centered) {
		dest_offset -= frame_size / 2;
	}

	if (get_viewport() && get_viewport()->is_snap_2d_transforms_to_pixel_enabled()) {
		dest_offset = (dest_offset + Point2(0.5, 0.5)).floor();
	}

	r_dst_rect = Rect2(dest_offset, frame_size);

	if (hflip) {
		r_dst_rect.size.x = -r_dst_rect.size.x;
	}
	if (vflip) {
		r_dst_rect.size.y = -r_dst_rect.size.y;
	}
}

Point2 Sprite2D::_get_rect_offset(const Size2i& p_size) const
{
	Point2 ofs = offset;
	if (centered) {
		ofs -= Size2(p_size) / 2;
	}

	if (get_viewport() && get_viewport()->is_snap_2d_transforms_to_pixel_enabled()) {
		ofs = (ofs + Point2(0.5, 0.5)).floor();
	}

	return ofs;
}

void Sprite2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		Rect2 dst_rect = get_rect();

		AccessibilityServer::update_set_role(
			ae, AccessibilityServerEnums::AccessibilityRole::ROLE_IMAGE);
		AccessibilityServer::update_set_transform(ae, get_transform());
		AccessibilityServer::update_set_bounds(ae, dst_rect);
	} break;

	case NOTIFICATION_DRAW: {
		if (texture.is_null()) {
			return;
		}

		RID ci = get_canvas_item();

		Rect2 src_rect, dst_rect;
		bool filter_clip_enabled;
		_get_rects(src_rect, dst_rect, filter_clip_enabled);

		texture->draw_rect_region(
			ci, dst_rect, src_rect, Color(1, 1, 1), false, filter_clip_enabled);
	} break;
	}
}

Ref<Texture2D> Sprite2D::get_texture() const { return texture; }

bool Sprite2D::is_centered() const { return centered; }

Point2 Sprite2D::get_offset() const { return offset; }

bool Sprite2D::is_flipped_h() const { return hflip; }

bool Sprite2D::is_flipped_v() const { return vflip; }

bool Sprite2D::is_region_enabled() const { return region_enabled; }

void Sprite2D::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}

	region_rect = p_region_rect;

	if (region_enabled) {
		item_rect_changed();
	}
}

Rect2 Sprite2D::get_region_rect() const { return region_rect; }

bool Sprite2D::is_region_filter_clip_enabled() const { return region_filter_clip_enabled; }

int Sprite2D::get_frame() const { return frame; }

void Sprite2D::set_frame_coords(const Vector2i& p_coord)
{
	ERR_FAIL_INDEX(p_coord.x, hframes);
	ERR_FAIL_INDEX(p_coord.y, vframes);

	set_frame(p_coord.y * hframes + p_coord.x);
}

Vector2i Sprite2D::get_frame_coords() const { return Vector2i(frame % hframes, frame / hframes); }

int Sprite2D::get_vframes() const { return vframes; }

int Sprite2D::get_hframes() const { return hframes; }

bool Sprite2D::is_pixel_opaque(const Point2& p_point) const
{
	if (texture.is_null()) {
		return false;
	}

	if (texture->get_size().width == 0 || texture->get_size().height == 0) {
		return false;
	}

	Rect2 src_rect, dst_rect;
	bool filter_clip_enabled;
	_get_rects(src_rect, dst_rect, filter_clip_enabled);
	dst_rect.size = dst_rect.size.abs();

	if (!dst_rect.has_point(p_point)) {
		return false;
	}

	Vector2 q = (p_point - dst_rect.position) / dst_rect.size;
	if (hflip) {
		q.x = 1.0f - q.x;
	}
	if (vflip) {
		q.y = 1.0f - q.y;
	}
	q = q * src_rect.size + src_rect.position;
	TextureRepeat repeat_mode = get_texture_repeat_in_tree();
	bool is_repeat = repeat_mode == TEXTURE_REPEAT_ENABLED || repeat_mode == TEXTURE_REPEAT_MIRROR;
	bool is_mirrored_repeat = repeat_mode == TEXTURE_REPEAT_MIRROR;
	if (is_repeat) {
		int mirror_x = 0;
		int mirror_y = 0;
		if (is_mirrored_repeat) {
			mirror_x = (int)(q.x / texture->get_size().width);
			mirror_y = (int)(q.y / texture->get_size().height);
		}
		q.x = Math::fmod(q.x, texture->get_size().width);
		q.y = Math::fmod(q.y, texture->get_size().height);
		if (mirror_x % 2 == 1) {
			q.x = texture->get_size().width - q.x - 1;
		}
		if (mirror_y % 2 == 1) {
			q.y = texture->get_size().height - q.y - 1;
		}
	}
	else {
		q = q.min(texture->get_size() - Vector2(1, 1));
	}

	return texture->is_pixel_opaque((int)q.x, (int)q.y);
}

bool Sprite2D::is_editor_region_rect_draggable() const
{
	return hframes <= 1 && vframes <= 1 && region_enabled;
}

Rect2 Sprite2D::get_rect() const
{
	if (texture.is_null()) {
		return Rect2(0, 0, 1, 1);
	}

	Size2i s;

	if (region_enabled) {
		s = region_rect.size;
	}
	else {
		s = texture->get_size();
	}

	s = s / Point2(hframes, vframes);

	Point2 ofs = _get_rect_offset(s);

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}



void Sprite2D::set_frame(int) {}

// ==========================================================================
// Merged from scene/2d/animated_sprite_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/animated_sprite_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/sprite_frames.h"

class AnimatedSprite2D : public Node2D
{
	Ref<SpriteFrames> frames;
	String autoplay;

	bool playing = false;
	String animation = SceneString(default_);
	int frame = 0;
	float speed_scale = 1.0;
	float custom_speed_scale = 1.0;

	bool centered = true;
	Point2 offset;

	real_t frame_speed_scale = 1.0;
	real_t frame_progress = 0.0;

	bool hflip = false;
	bool vflip = false;

	double _get_frame_duration();
	void _calc_frame_speed_scale();
	void _stop_internal(bool p_reset);

	Rect2 _get_rect() const;

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	virtual Rect2 get_anchorable_rect() const override;

	void set_sprite_frames(const Ref<SpriteFrames>& p_frames);
	Ref<SpriteFrames> get_sprite_frames() const;

	void play_backwards(const String& p_name = String());
	void pause();
	void stop();

	bool is_playing() const;

	void set_animation(const String& p_name);
	String get_animation() const;

	void set_autoplay(const String& p_name);
	String get_autoplay() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_progress(real_t p_progress);
	real_t get_frame_progress() const;

	void set_speed_scale(float p_speed_scale);
	float get_speed_scale() const;
	float get_playing_speed() const;

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	PackedStringArray get_configuration_warnings() const override;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif // TOOLS_ENABLED

	AnimatedSprite2D();
};



// --- End scene/2d/animated_sprite_2d.h ---
#include "core/config/engine.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"

#ifdef DEBUG_ENABLED
Rect2 AnimatedSprite2D::_edit_get_rect() const { return _get_rect(); }

bool AnimatedSprite2D::_edit_use_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return false;
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return false;
	}

	Ref<Texture2D> t;
	if (!animation.is_empty()) {
		t = frames->get_frame_texture(animation, frame);
	}
	return t.is_valid();
}
#endif // DEBUG_ENABLED

Rect2 AnimatedSprite2D::get_anchorable_rect() const { return _get_rect(); }

Rect2 AnimatedSprite2D::_get_rect() const
{
	if (frames.is_null() || !frames->has_animation(animation)) {
		return Rect2();
	}
	if (frame < 0 || frame >= frames->get_frame_count(animation)) {
		return Rect2();
	}

	Ref<Texture2D> t;
	if (!animation.is_empty()) {
		t = frames->get_frame_texture(animation, frame);
	}
	if (t.is_null()) {
		return Rect2();
	}
	Size2 s = t->get_size();

	Point2 ofs = offset;
	if (centered) {
		ofs -= s / 2;
	}

	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}

	return Rect2(ofs, s);
}

Ref<SpriteFrames> AnimatedSprite2D::get_sprite_frames() const { return frames; }

int AnimatedSprite2D::get_frame() const { return frame; }

void AnimatedSprite2D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t AnimatedSprite2D::get_frame_progress() const { return frame_progress; }

void AnimatedSprite2D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float AnimatedSprite2D::get_speed_scale() const { return speed_scale; }

float AnimatedSprite2D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

bool AnimatedSprite2D::is_centered() const { return centered; }

Point2 AnimatedSprite2D::get_offset() const { return offset; }

bool AnimatedSprite2D::is_flipped_h() const { return hflip; }

bool AnimatedSprite2D::is_flipped_v() const { return vflip; }

bool AnimatedSprite2D::is_playing() const { return playing; }

void AnimatedSprite2D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimatedSprite2D::get_autoplay() const { return autoplay; }

void AnimatedSprite2D::pause() { _stop_internal(false); }

void AnimatedSprite2D::stop() { _stop_internal(true); }

double AnimatedSprite2D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void AnimatedSprite2D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

String AnimatedSprite2D::get_animation() const { return animation; }

PackedStringArray AnimatedSprite2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (frames.is_null()) {
		warnings.push_back(
			RTR("A SpriteFrames resource must be created or set in the \"Sprite Frames\" property "
				"in order for AnimatedSprite2D to display frames."));
	}
	return warnings;
}

#ifdef TOOLS_ENABLED
void AnimatedSprite2D::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && frames.is_valid()) {
		if (pf == "play" || pf == "play_backwards" || pf == "set_animation" ||
			pf == "set_autoplay") {
			List<String> al;
			frames->get_animation_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node2D::get_argument_options(p_function, p_idx, r_options);
}
#endif // TOOLS_ENABLED

AnimatedSprite2D::AnimatedSprite2D() {}



void AnimatedSprite2D::_stop_internal(bool) {}

// ==========================================================================
// Merged from scene/2d/mesh_instance_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/mesh_instance_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/mesh.h"

class NavigationPolygon;
class NavigationMeshSourceGeometryData2D;

class MeshInstance2D : public Node2D
{
	Ref<Mesh> mesh;

	Ref<Texture2D> texture;

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

private:
	static RID _navmesh_source_geometry_parser;

public:
#ifndef NAVIGATION_2D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	virtual PackedStringArray get_configuration_warnings() const override;
};



// --- End scene/2d/mesh_instance_2d.h ---

#ifndef NAVIGATION_2D_DISABLED
#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID MeshInstance2D::_navmesh_source_geometry_parser;

void MeshInstance2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		if (mesh.is_valid()) {
			draw_mesh(mesh.ptr(), texture);
		}
	} break;
	}
}





Ref<Mesh> MeshInstance2D::get_mesh() const { return mesh; }



Ref<Texture2D> MeshInstance2D::get_texture() const { return texture; }

#ifdef DEBUG_ENABLED
Rect2 MeshInstance2D::_edit_get_rect() const
{
	if (mesh.is_valid()) {
		AABB aabb = mesh->get_aabb();
		return Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
	}

	return Node2D::_edit_get_rect();
}

bool MeshInstance2D::_edit_use_rect() const { return mesh.is_valid(); }
#endif // DEBUG_ENABLED

PackedStringArray MeshInstance2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (mesh.is_null()) {
		warnings.push_back(RTR("MeshInstance2D requires a Mesh to render anything. Please add a "
							   "mesh resource for it!"));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/multimesh_instance_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/multimesh_instance_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/multimesh.h"

class NavigationPolygon;
class NavigationMeshSourceGeometryData2D;

class MultiMeshInstance2D : public Node2D
{
	Ref<MultiMesh> multimesh;

	Ref<Texture2D> texture;

	void _refresh_interpolated();

protected:
	virtual void _physics_interpolated_changed() override;
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
#endif // DEBUG_ENABLED

	void set_multimesh(const Ref<MultiMesh>& p_multimesh);
	Ref<MultiMesh> get_multimesh() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

private:
	static RID _navmesh_source_geometry_parser;

public:
#ifndef NAVIGATION_2D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	MultiMeshInstance2D() = default;
	~MultiMeshInstance2D() = default;
};



// --- End scene/2d/multimesh_instance_2d.h ---

#ifndef NAVIGATION_2D_DISABLED
#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID MultiMeshInstance2D::_navmesh_source_geometry_parser;

void MultiMeshInstance2D::_refresh_interpolated()
{
	if (is_inside_tree() && multimesh.is_valid()) {
		bool interpolated = is_physics_interpolated_and_enabled();
		multimesh->set_physics_interpolated(interpolated);
	}
}

void MultiMeshInstance2D::_physics_interpolated_changed()
{
	CanvasItem::_physics_interpolated_changed();
	_refresh_interpolated();
}

void MultiMeshInstance2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_refresh_interpolated();
		break;
	}
	case NOTIFICATION_DRAW: {
		if (multimesh.is_valid()) {
			draw_multimesh(multimesh.ptr(), texture);
		}
	} break;
	}
}

Ref<MultiMesh> MultiMeshInstance2D::get_multimesh() const { return multimesh; }

Ref<Texture2D> MultiMeshInstance2D::get_texture() const { return texture; }

#ifdef DEBUG_ENABLED
Rect2 MultiMeshInstance2D::_edit_get_rect() const
{
	if (multimesh.is_valid()) {
		AABB aabb = multimesh->get_aabb();
		return Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
	}

	return Node2D::_edit_get_rect();
}
#endif // DEBUG_ENABLED

// ==========================================================================
// Merged from scene/2d/polygon_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/sprite_2d.h"
#include "servers/rendering/renderer.h"

#ifndef NAVIGATION_2D_DISABLED
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/2d/navigation_polygon.h"
#include "servers/navigation_2d/navigation_server_2d.h"

RID Polygon2D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

#ifdef TOOLS_ENABLED

void Polygon2D::_edit_set_pivot(const Point2& p_pivot)
{
	set_position(get_transform().xform(p_pivot));
	set_offset(get_offset() - p_pivot);
}

Point2 Polygon2D::_edit_get_pivot() const { return Vector2(); }

bool Polygon2D::_edit_use_pivot() const { return true; }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 Polygon2D::_edit_get_rect() const
{
	if (rect_cache_dirty) {
		int l = polygon.size();
		const Vector2* r = polygon.ptr();
		item_rect = Rect2();
		for (int i = 0; i < l; i++) {
			Vector2 pos = r[i] + offset;
			if (i == 0) {
				item_rect.position = pos;
			}
			else {
				item_rect.expand_to(pos);
			}
		}
		rect_cache_dirty = false;
	}

	return item_rect;
}

bool Polygon2D::_edit_use_rect() const { return polygon.size() > 0; }

#endif // DEBUG_ENABLED

Vector<Vector2> Polygon2D::get_polygon() const { return polygon; }

void Polygon2D::set_internal_vertex_count(int p_count) { internal_vertices = p_count; }

int Polygon2D::get_internal_vertex_count() const { return internal_vertices; }

Vector<Vector2> Polygon2D::get_uv() const { return uv; }

Color Polygon2D::get_color() const { return color; }

Vector<Color> Polygon2D::get_vertex_colors() const { return vertex_colors; }

Ref<Texture2D> Polygon2D::get_texture() const { return texture; }

Vector2 Polygon2D::get_texture_offset() const { return tex_ofs; }

real_t Polygon2D::get_texture_rotation() const { return tex_rot; }

Size2 Polygon2D::get_texture_scale() const { return tex_scale; }

bool Polygon2D::get_invert() const { return invert; }

bool Polygon2D::get_antialiased() const { return antialiased; }

real_t Polygon2D::get_invert_border() const { return invert_border; }

Vector2 Polygon2D::get_offset() const { return offset; }

void Polygon2D::add_bone(const NodePath& p_path, const Vector<float>& p_weights)
{
	Bone bone;
	bone.path = p_path;
	bone.weights = p_weights;
	bone_weights.push_back(bone);
}

int Polygon2D::get_bone_count() const { return bone_weights.size(); }

NodePath Polygon2D::get_bone_path(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), NodePath());
	return bone_weights[p_index].path;
}

const Vector<float>& Polygon2D::get_bone_weights(int p_index) const
{
	static const Vector<float> EMPTY;
	ERR_FAIL_INDEX_V(p_index, bone_weights.size(), EMPTY);
	return bone_weights[p_index].weights;
}

void Polygon2D::erase_bone(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, bone_weights.size());
	bone_weights.remove_at(p_idx);
}

void Polygon2D::clear_bones() { bone_weights.clear(); }

NodePath Polygon2D::get_skeleton() const { return skeleton; }

Polygon2D::Polygon2D() { mesh = Renderer::mesh_create(); }

Polygon2D::~Polygon2D()
{
	// This will free the internally-allocated mesh instance, if allocated.
	ERR_FAIL_NULL(Renderer::data);
	Renderer::canvas_item_attach_skeleton(get_canvas_item(), RID());
	Renderer::free_rid(mesh);
}

void Polygon2D::set_offset(const Vector2& p_offset) {}

bool Polygon2D::_edit_is_selected_on_click(const Vector2& p_point, double p_tolerance) const
{
	return false;
}

// ==========================================================================
// Merged from scene/2d/skeleton_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/transform_interpolator.h"
#include "servers/rendering/renderer.h"
#include "scene/2d/sprite_2d.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
bool Bone2D::_editor_get_show_bone_gizmo() const { return _editor_show_bone_gizmo; }
#endif // TOOLS_ENABLED

Transform2D Bone2D::get_rest() const { return rest; }

Transform2D Bone2D::get_skeleton_rest() const
{
	if (parent_bone) {
		return parent_bone->get_skeleton_rest() * rest;
	}
	else {
		return rest;
	}
}

void Bone2D::apply_rest() { set_transform(rest); }

int Bone2D::get_index_in_skeleton() const
{
	ERR_FAIL_NULL_V(skeleton, -1);
	skeleton->_update_bone_setup();
	return skeleton_index;
}

PackedStringArray Bone2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!skeleton) {
		if (parent_bone) {
			warnings.push_back(RTR("This Bone2D chain should end at a Skeleton2D node."));
		}
		else {
			warnings.push_back(
				RTR("A Bone2D only works with a Skeleton2D or another Bone2D as parent node."));
		}
	}

	if (rest == Transform2D(0, 0, 0, 0, 0, 0)) {
		warnings.push_back(
			RTR("This bone lacks a proper REST pose. Go to the Skeleton2D node and set one."));
	}

	return warnings;
}

bool Bone2D::get_autocalculate_length_and_angle() const { return autocalculate_length_and_angle; }

real_t Bone2D::get_length() const { return length; }

real_t Bone2D::get_bone_angle() const { return bone_angle; }

Bone2D::Bone2D()
{
	skeleton = nullptr;
	parent_bone = nullptr;
	skeleton_index = -1;
	length = 16;
	bone_angle = 0;
	autocalculate_length_and_angle = true;
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	// this is a clever hack so the bone knows no rest has been set yet, allowing to show an error.
	for (int i = 0; i < 3; i++) {
		rest[i] = Vector2(0, 0);
	}
	copy_transform_to_cache = true;
}

Bone2D::~Bone2D()
{
#ifdef TOOLS_ENABLED
	if (!editor_gizmo_rid.is_null()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(editor_gizmo_rid);
	}
#endif // TOOLS_ENABLED
}

//////////////////////////////////////

void Skeleton2D::_update_transform()
{
	if (bone_setup_dirty) {
		_update_bone_setup();
		return; // above will update transform anyway
	}
	if (!transform_dirty) {
		return;
	}

	transform_dirty = false;

	for (uint32_t i = 0; i < bones.size(); i++) {
		ERR_CONTINUE(bones[i].parent_index >= (int)i);
		if (bones[i].parent_index >= 0) {
			bones[i].accum_transform =
				bones[bones[i].parent_index].accum_transform * bones[i].bone->get_transform();
		}
		else {
			bones[i].accum_transform = bones[i].bone->get_transform();
		}
	}

	for (uint32_t i = 0; i < bones.size(); i++) {
		Transform2D final_xform = bones[i].accum_transform * bones[i].rest_inverse;
		Renderer::skeleton_bone_set_transform_2d(skeleton, i, final_xform);
	}
}

int Skeleton2D::get_bone_count() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), 0);

	if (bone_setup_dirty) {
		// TODO: Is this necessary? It doesn't seem to change bones.size()
		const_cast<Skeleton2D*>(this)->_update_bone_setup();
	}

	return bones.size();
}

Bone2D* Skeleton2D::get_bone(int p_idx)
{
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
	ERR_FAIL_INDEX_V(p_idx, (int)bones.size(), nullptr);

	return bones[p_idx].bone;
}

void Skeleton2D::_update_process_mode()
{
	bool process = modification_stack.is_valid() && is_inside_tree();
	if (!process) {
		// We might have another reason to process.
		process = is_physics_interpolated_and_enabled() && is_visible_in_tree();
	}

	set_process_internal(process);
	set_physics_process_internal(process);
}

void Skeleton2D::_ensure_update_interpolation_data()
{
	uint64_t tick = Engine::get_physics_frames();

	if (_interpolation_data.last_update_physics_tick != tick) {
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		_interpolation_data.last_update_physics_tick = tick;
	}
}

void Skeleton2D::_physics_interpolated_changed() { _update_process_mode(); }

void Skeleton2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (bone_setup_dirty) {
			_update_bone_setup();
		}
		if (transform_dirty) {
			_update_transform();
		}
		request_ready();
	} break;

	case NOTIFICATION_ENTER_TREE: {
		_update_process_mode();

		if (is_physics_interpolated_and_enabled()) {
			_interpolation_data.xform_curr = get_global_transform();
			_interpolation_data.xform_prev = _interpolation_data.xform_curr;
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			if (Engine::is_in_physics_frame()) {
				_interpolation_data.xform_curr = get_global_transform();
			}
		}
		else {
			Renderer::skeleton_set_base_transform_2d(skeleton, get_global_transform());
		}
	} break;

	case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		_interpolation_data.xform_curr = get_global_transform();
		_interpolation_data.xform_prev = _interpolation_data.xform_curr;
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			Transform2D res;
			TransformInterpolator::interpolate_transform_2d(_interpolation_data.xform_prev,
				_interpolation_data.xform_curr, res,
				Engine::get_physics_interpolation_fraction());
			Renderer::skeleton_set_base_transform_2d(skeleton, res);
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_process);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_physics_interpolated_and_enabled()) {
			_ensure_update_interpolation_data();
			_interpolation_data.xform_curr = get_global_transform();
		}
		if (modification_stack.is_valid()) {
			execute_modifications(get_physics_process_delta_time(),
				SkeletonModificationStack2D::EXECUTION_MODE::execution_mode_physics_process);
		}
	} break;

	case NOTIFICATION_POST_ENTER_TREE: {
		set_modification_stack(modification_stack);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_process_mode();
	} break;

#ifdef TOOLS_ENABLED
	case NOTIFICATION_DRAW: {
		if (Engine::is_editor_hint()) {
			if (modification_stack.is_valid()) {
				modification_stack->draw_editor_gizmos();
			}
		}
	} break;
#endif // TOOLS_ENABLED
	}
}

RID Skeleton2D::get_skeleton() const { return skeleton; }

void Skeleton2D::set_bone_local_pose_override(
	int p_bone_idx, Transform2D p_override, real_t p_amount, bool p_persistent)
{
	ERR_FAIL_INDEX_MSG(p_bone_idx, (int)bones.size(), "Bone index is out of range!");
	bones[p_bone_idx].local_pose_override = p_override;
	bones[p_bone_idx].local_pose_override_amount = p_amount;
	bones[p_bone_idx].local_pose_override_persistent = p_persistent;
}

Transform2D Skeleton2D::get_bone_local_pose_override(int p_bone_idx)
{
	ERR_FAIL_INDEX_V_MSG(
		p_bone_idx, (int)bones.size(), Transform2D(), "Bone index is out of range!");
	return bones[p_bone_idx].local_pose_override;
}

void Skeleton2D::set_modification_stack(Ref<SkeletonModificationStack2D> p_stack)
{
	if (modification_stack.is_valid()) {
		modification_stack->is_setup = false;
		modification_stack->set_skeleton(nullptr);
	}
	modification_stack = p_stack;
	if (modification_stack.is_valid() && is_inside_tree()) {
		modification_stack->set_skeleton(this);
		modification_stack->setup();

#ifdef TOOLS_ENABLED
		modification_stack->set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	}
	_update_process_mode();
}

Ref<SkeletonModificationStack2D> Skeleton2D::get_modification_stack() const
{
	return modification_stack;
}

Skeleton2D::Skeleton2D()
{
	skeleton = Renderer::skeleton_create();
	set_notify_transform(true);
	set_hide_clip_children(true);
}

Skeleton2D::~Skeleton2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(skeleton);
}

void Skeleton2D::_update_bone_setup() {}
