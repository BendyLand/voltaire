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

#include "scene/2d/node_2d.h"
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

#include "scene/2d/node_2d.h"
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

#include "scene/2d/node_2d.h"

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

#include "scene/2d/node_2d.h"
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

#include "scene/2d/node_2d.h"

bool AudioStreamPlayer2D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/2d/node_2d.h"
#include "core/config/project_settings.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/main/viewport.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/physics_2d/physics_server_2d.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/node_2d.h"
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
#include "core/math/random_pcg.h"
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
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/gradient.h"
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


#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/gradient.h"
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
#include "scene/main/scene_tree.h"
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
#include "scene/main/canvas_item.h"
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
#include "scene/2d/node_2d.h"

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
#include "scene/3d/visual_instance_3d.h"

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
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
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
#include "scene/3d/visual_instance_3d.h"

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
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
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
#include "scene/2d/node_2d.h"
#include "servers/rendering/renderer.h"

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
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
#include "scene/2d/node_2d.h"

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

// ============================================================================
// Merged from scene/2d/tile_map_layer.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/io/marshalls.h"
#include "core/math/geometry_2d.h"
#include "core/math/random_pcg.h"
#include "core/templates/a_hash_map.h"
// --- Inlined from scene/2d/tile_map.h ---
#include "scene/2d/line_2d.h"
#include "scene/property_list_helper.h"
#include "scene/2d/line_2d.h"

class Control;
#ifndef NAVIGATION_2D_DISABLED
class NavigationMeshSourceGeometryData2D;
#endif // NAVIGATION_2D_DISABLED
class TileMapLayer;
class TerrainConstraint;

enum TileMapDataFormat
{
	TILE_MAP_DATA_FORMAT_1 = 0,
	TILE_MAP_DATA_FORMAT_2,
	TILE_MAP_DATA_FORMAT_3,
	TILE_MAP_DATA_FORMAT_MAX,
};

class TileMap : public Node2D
{
public:
	// Kept for compatibility, but should use TileMapLayer::VisibilityMode instead.
	enum VisibilityMode
	{
		VISIBILITY_MODE_DEFAULT,
		VISIBILITY_MODE_FORCE_SHOW,
		VISIBILITY_MODE_FORCE_HIDE,
	};

private:
	// A compatibility enum to specify how is the data if formatted.
	mutable TileMapDataFormat format = TileMapDataFormat::TILE_MAP_DATA_FORMAT_3;

	// Properties.
	Ref<TileSet> tile_set;
	int rendering_quadrant_size = 16;
	bool collision_animatable = false;
	VisibilityMode collision_visibility_mode = VISIBILITY_MODE_DEFAULT;
#ifndef NAVIGATION_2D_DISABLED
	VisibilityMode navigation_visibility_mode = VISIBILITY_MODE_DEFAULT;
#endif // NAVIGATION_2D_DISABLED

	// Layers.
	LocalVector<TileMapLayer*> layers;

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	// Transforms for collision_animatable.
	Transform2D last_valid_transform;
	Transform2D new_transform;

	void _emit_changed();

	// Kept for compatibility with TileMap. With TileMapLayers as individual nodes, the format is
	// stored directly in the array.
	void _set_tile_map_data_using_compatibility_format(
		int p_layer, TileMapDataFormat p_format, const Vector<int>& p_data);
	Vector<int> _get_tile_map_data_using_compatibility_format(int p_layer) const;
	void _set_layer_tile_data(int p_layer, const PackedInt32Array& p_data);

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	Rect2i _get_used_rect_bind_compat_78328();
	void _set_quadrant_size_bind_compat_81070(int p_quadrant_size);
	int _get_quadrant_size_bind_compat_81070() const;
	VisibilityMode _get_collision_visibility_mode_bind_compat_87115();
#ifndef NAVIGATION_2D_DISABLED
	VisibilityMode _get_navigation_visibility_mode_bind_compat_87115();
#endif // NAVIGATION_2D_DISABLED

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
#endif // DEBUG_ENABLED

#ifndef DISABLE_DEPRECATED
	void force_update(int p_layer);
#endif // DISABLE_DEPRECATED

	void set_rendering_quadrant_size(int p_size);
	int get_rendering_quadrant_size() const;

	// Accessors.
	void set_tileset(const Ref<TileSet>& p_tileset);
	Ref<TileSet> get_tileset() const;

	// Layers management.
	int get_layers_count() const;
	void add_layer(int p_to_pos);
	void move_layer(int p_layer, int p_to_pos);
	void remove_layer(int p_layer);

	void set_layer_name(int p_layer, String p_name);
	String get_layer_name(int p_layer) const;
	void set_layer_enabled(int p_layer, bool p_visible);
	bool is_layer_enabled(int p_layer) const;
	void set_layer_modulate(int p_layer, Color p_modulate);
	Color get_layer_modulate(int p_layer) const;
	bool is_layer_y_sort_enabled(int p_layer) const;
	int get_layer_y_sort_origin(int p_layer) const;
	void set_layer_z_index(int p_layer, int p_z_index);
	int get_layer_z_index(int p_layer) const;
#ifndef NAVIGATION_2D_DISABLED
	void set_layer_navigation_enabled(int p_layer, bool p_enabled);
	bool is_layer_navigation_enabled(int p_layer) const;
	void set_layer_navigation_map(int p_layer, RID p_map);
	RID get_layer_navigation_map(int p_layer) const;
#endif // NAVIGATION_2D_DISABLED

	void set_collision_animatable(bool p_collision_animatable);
	bool is_collision_animatable() const;

	// Debug visibility modes.
	void set_collision_visibility_mode(VisibilityMode p_show_collision);
	VisibilityMode get_collision_visibility_mode() const;

#ifndef NAVIGATION_2D_DISABLED
	void set_navigation_visibility_mode(VisibilityMode p_show_navigation);
	VisibilityMode get_navigation_visibility_mode() const;
#endif // NAVIGATION_2D_DISABLED

	// Cells accessors.
	void set_cell(int p_layer, const Vector2i& p_coords, int p_source_id = TileSet::INVALID_SOURCE,
		const Vector2i p_atlas_coords = TileSetSource::INVALID_ATLAS_COORDS,
		int p_alternative_tile = 0);
	void erase_cell(int p_layer, const Vector2i& p_coords);
	int get_cell_source_id(int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	Vector2i get_cell_atlas_coords(
		int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	int get_cell_alternative_tile(
		int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	// Helper method to make accessing the data easier.
	TileData* get_cell_tile_data(
		int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;

	bool is_cell_flipped_h(int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	bool is_cell_flipped_v(int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	bool is_cell_transposed(
		int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;

	// Patterns.
	Ref<TileMapPattern> get_pattern(int p_layer, TypedArray<Vector2i> p_coords_array);
	Vector2i map_pattern(const Vector2i& p_position_in_tilemap, const Vector2i& p_coords_in_pattern,
		Ref<TileMapPattern> p_pattern);
	void set_pattern(int p_layer, const Vector2i& p_position, const Ref<TileMapPattern> p_pattern);

	// Terrains (Not exposed).
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_constraints(int p_layer,
		const Vector<Vector2i>& p_to_replace, int p_terrain_set,
		const RBSet<TerrainConstraint>& p_constraints);
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_connect(int p_layer,
		const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true);
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_path(int p_layer,
		const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true);
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_pattern(int p_layer,
		const Vector<Vector2i>& p_coords_array, int p_terrain_set,
		TileSet::TerrainsPattern p_terrains_pattern, bool p_ignore_empty_terrains = true);

	// Terrains (exposed).
	void set_cells_terrain_connect(int p_layer, TypedArray<Vector2i> p_cells, int p_terrain_set,
		int p_terrain, bool p_ignore_empty_terrains = true);
	void set_cells_terrain_path(int p_layer, TypedArray<Vector2i> p_path, int p_terrain_set,
		int p_terrain, bool p_ignore_empty_terrains = true);

	// Not exposed to users.
	TileMapCell get_cell(int p_layer, const Vector2i& p_coords, bool p_use_proxies = false) const;
	int get_effective_quadrant_size(int p_layer) const;

	Vector2 map_to_local(const Vector2i& p_pos) const;
	Vector2i local_to_map(const Vector2& p_pos) const;
	bool is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const;
	Vector2i get_neighbor_cell(
		const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const;

	TypedArray<Vector2i> get_used_cells(int p_layer) const;
	TypedArray<Vector2i> get_used_cells_by_id(int p_layer,
		int p_source_id = TileSet::INVALID_SOURCE,
		const Vector2i p_atlas_coords = TileSetSource::INVALID_ATLAS_COORDS,
		int p_alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE) const;
	Rect2i get_used_rect() const;

	// Override some methods of the CanvasItem class to pass the changes to the quadrants
	// CanvasItems.
	virtual void set_light_mask(int p_light_mask) override;
	virtual void set_self_modulate(const Color& p_self_modulate) override;
	virtual void set_texture_filter(CanvasItem::TextureFilter p_texture_filter) override;
	virtual void set_texture_repeat(CanvasItem::TextureRepeat p_texture_repeat) override;

#ifndef PHYSICS_2D_DISABLED
	// For finding tiles from collision.
	Vector2i get_coords_for_body_rid(RID p_physics_body);
	// For getting their layers as well.
	int get_layer_for_body_rid(RID p_physics_body);
#endif // PHYSICS_2D_DISABLED

	// Fixing and clearing methods.
	void fix_invalid_tiles();

#ifdef TOOLS_ENABLED
	// Moving layers outside of TileMap.
	TileMapLayer* duplicate_layer_from_internal(int p_layer);
#endif // TOOLS_ENABLED

	// Clears tiles from a given layer.
	void clear_layer(int p_layer);
	void clear();

	// Force a TileMap update.
	void update_internals();
	void notify_runtime_tile_data_update(int p_layer = -1);

	// Helpers?
	TypedArray<Vector2i> get_surrounding_cells(const Vector2i& p_coords);

	// Configuration warnings.
	PackedStringArray get_configuration_warnings() const override;

#ifndef NAVIGATION_2D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	TileMap();
};



// --- End scene/2d/tile_map.h ---
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"
#include "scene/2d/line_2d.h"

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
RID TileMapLayer::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

Vector2i TileMapLayer::_coords_to_quadrant_coords(
	const Vector2i& p_coords, const int p_quadrant_size) const
{
	return Vector2i(p_coords.x > 0 ? p_coords.x / p_quadrant_size
								   : (p_coords.x - (p_quadrant_size - 1)) / p_quadrant_size,
		p_coords.y > 0 ? p_coords.y / p_quadrant_size
					   : (p_coords.y - (p_quadrant_size - 1)) / p_quadrant_size);
}

#ifdef DEBUG_ENABLED
constexpr int TILE_MAP_DEBUG_QUADRANT_SIZE = 16;

void TileMapLayer::_debug_update(bool p_force_cleanup)
{
	// Check if we should cleanup everything.
	bool forced_cleanup =
		p_force_cleanup || !enabled || tile_set.is_null() || !is_visible_in_tree();
	if (forced_cleanup && _debug_was_cleaned_up) {
		return;
	}

	if (forced_cleanup) {
		for (KeyValue<Vector2i, Ref<DebugQuadrant>>& kv : debug_quadrant_map) {
			// Free the quadrant.
			Ref<DebugQuadrant>& debug_quadrant = kv.value;
			if (debug_quadrant->canvas_item.is_valid()) {
				Renderer::free_rid(debug_quadrant->canvas_item);
			}
			if (debug_quadrant->physics_mesh.is_valid()) {
				Renderer::free_rid(debug_quadrant->physics_mesh);
			}
		}
		debug_quadrant_map.clear();
		_debug_was_cleaned_up = true;
		return;
	}

	// Check if anything is dirty, in such a case, redraw debug.
	bool anything_changed = false;
	for (int i = 0; i < DIRTY_FLAGS_MAX; i++) {
		if (dirty.flags[i]) {
			anything_changed = true;
			break;
		}
	}

	// List all debug quadrants to update.
	HashSet<Vector2i> quadrants_to_updates;
	if (_debug_was_cleaned_up || anything_changed) {
		// Update all cells.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			CellData& cell_data = kv.value;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE));
#ifndef PHYSICS_2D_DISABLED
			// Physics quadrants are drawn from their origin.
			Vector2i physics_quadrant_origin =
				_coords_to_quadrant_coords(cell_data.coords, physics_quadrant_size) *
				physics_quadrant_size;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(physics_quadrant_origin, TILE_MAP_DEBUG_QUADRANT_SIZE));
#endif // PHYSICS_2D_DISABLED
		}
	}
	else {
		// Update dirty cells.
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE));
#ifndef PHYSICS_2D_DISABLED
			// Physics quadrants are drawn from their origin.
			Vector2i physics_quadrant_origin =
				_coords_to_quadrant_coords(cell_data.coords, physics_quadrant_size) *
				physics_quadrant_size;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(physics_quadrant_origin, TILE_MAP_DEBUG_QUADRANT_SIZE));
#endif // PHYSICS_2D_DISABLED
		}
	}

	// Create new quadrants if needed.
	for (const Vector2i& quadrant_coords : quadrants_to_updates) {
		if (!debug_quadrant_map.has(quadrant_coords)) {
			// Create a new quadrant and add it to the quadrant map.
			Ref<DebugQuadrant> new_quadrant;
			new_quadrant.instantiate();
			new_quadrant->quadrant_coords = quadrant_coords;
			debug_quadrant_map[quadrant_coords] = new_quadrant;
		}
	}

	// Second pass on modified cells to update the list of cells per quandrant.
	if (_debug_was_cleaned_up || anything_changed) {
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			CellData& cell_data = kv.value;
			Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[_coords_to_quadrant_coords(
				cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE)];
			if (!cell_data.debug_quadrant_list_element.in_list()) {
				debug_quadrant->cells.add(&cell_data.debug_quadrant_list_element);
			}
		}
	}
	else {
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[_coords_to_quadrant_coords(
				cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE)];
			if (!cell_data.debug_quadrant_list_element.in_list()) {
				debug_quadrant->cells.add(&cell_data.debug_quadrant_list_element);
			}
		}
	}

	// Update those quadrants.
	bool needs_set_not_interpolated = is_inside_tree() &&
									  SceneTree::is_physics_interpolation_enabled() &&
									  !is_physics_interpolated();
	for (const Vector2i& quadrant_coords : quadrants_to_updates) {
		Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[quadrant_coords];

		// Update the quadrant's canvas item.
		RID& ci = debug_quadrant->canvas_item;
		if (ci.is_valid()) {
			Renderer::canvas_item_clear(ci);
		}
		else {
			ci = Renderer::canvas_item_create();
			if (needs_set_not_interpolated) {
				Renderer::canvas_item_set_interpolated(ci, false);
			}
			Renderer::canvas_item_set_z_index(ci, RSE::CANVAS_ITEM_Z_MAX - 1);
			Renderer::canvas_item_set_parent(ci, get_canvas_item());
		}
		const Vector2 quadrant_pos =
			tile_set->map_to_local(debug_quadrant->quadrant_coords * TILE_MAP_DEBUG_QUADRANT_SIZE);
		Transform2D xform(0, quadrant_pos);
		Renderer::canvas_item_set_transform(ci, xform);

#ifndef PHYSICS_2D_DISABLED
		// Draw physics.
		_physics_draw_quadrant_debug(ci, *debug_quadrant.ptr());
#endif // PHYSICS_2D_DISABLED

		// Draw debug info.
		for (SelfList<CellData>* cell_data_list_element = debug_quadrant->cells.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			if (cell_data.cell.source_id != TileSet::INVALID_SOURCE) {
				_rendering_draw_cell_debug(ci, quadrant_pos, cell_data);
#ifndef NAVIGATION_2D_DISABLED
				_navigation_draw_cell_debug(ci, quadrant_pos, cell_data);
#endif // NAVIGATION_2D_DISABLED
				_scenes_draw_cell_debug(ci, quadrant_pos, cell_data);
				debug_quadrant->drawn_to = true;
			}
		}

		// Free the quadrants that were not drawn to.
		if (!debug_quadrant->drawn_to) {
			// Free the quadrant.
			if (ci.is_valid()) {
				Renderer::free_rid(ci);
			}
			debug_quadrant_map.erase(quadrant_coords);
		}
	}

	_debug_was_cleaned_up = false;
}

#endif // DEBUG_ENABLED

Color TileMapLayer::_highlight_color(const Color& p_modulate) const
{
	if (highlight_mode == HIGHLIGHT_MODE_BELOW) {
		return p_modulate.darkened(0.5);
	}
	if (highlight_mode == HIGHLIGHT_MODE_ABOVE) {
		Color c = p_modulate.darkened(0.5);
		c.a *= 0.3;
		return c;
	}
	return p_modulate;
}

void TileMapLayer::_rendering_notification(int p_what)
{
	if (p_what == NOTIFICATION_TRANSFORM_CHANGED || p_what == NOTIFICATION_ENTER_CANVAS ||
		p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (tile_set.is_valid()) {
			Transform2D tilemap_xform = get_global_transform();
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				const CellData& cell_data = kv.value;
				for (const LocalVector<RID>& polygons : cell_data.occluders) {
					for (const RID& rid : polygons) {
						if (rid.is_null()) {
							continue;
						}
						Transform2D xform(0, tile_set->map_to_local(kv.key));
						Renderer::canvas_light_occluder_attach_to_canvas(rid, get_canvas());
						Renderer::canvas_light_occluder_set_transform(rid, tilemap_xform * xform);
					}
				}
			}
		}
	}
	else if (p_what == NOTIFICATION_RESET_PHYSICS_INTERPOLATION) {
		if (is_physics_interpolated_and_enabled() && is_visible_in_tree()) {
			for (const KeyValue<Vector2i, Ref<RenderingQuadrant>>& kv : rendering_quadrant_map) {
				for (const RID& ci : kv.value->canvas_items) {
					if (ci.is_valid()) {
						Renderer::canvas_item_reset_physics_interpolation(ci);
					}
				}
			}
		}
	}
}

void TileMapLayer::_rendering_occluders_clear_cell(CellData& r_cell_data)
{
	// Free the occluders.
	for (const LocalVector<RID>& polygons : r_cell_data.occluders) {
		for (const RID& rid : polygons) {
			Renderer::free_rid(rid);
		}
	}
	r_cell_data.occluders.clear();
}

#ifndef NAVIGATION_2D_DISABLED

void TileMapLayer::_navigation_update(bool p_force_cleanup)
{
	ERR_FAIL_NULL(NavigationServer2D::data);

	// Check if we should cleanup everything.
	bool forced_cleanup = p_force_cleanup || !enabled || !navigation_enabled || !is_inside_tree() ||
						  tile_set.is_null();
	if (forced_cleanup && _navigation_was_cleaned_up) {
		return;
	}

	// ----------- Layer level processing -----------
	// All this processing is kept for compatibility with the TileMap node.
	// Otherwise, layers shall use the World2D navigation map or define a custom one with
	// set_navigation_map(...).
	if (tile_map_node) {
		if (forced_cleanup) {
			if (navigation_map_override.is_valid()) {
				NavigationServer2D::free_rid(navigation_map_override);
				navigation_map_override = RID();
			}
		}
		else {
			// Update navigation maps.
			if (!navigation_map_override.is_valid()) {
				if (layer_index_in_tile_map_node > 0) {
					// Create a dedicated map for each layer.
					RID new_layer_map = NavigationServer2D::map_create();
					// Set the default NavigationPolygon cell_size on the new map as a mismatch
					// causes an error.
					NavigationServer2D::map_set_cell_size(new_layer_map, NavigationDefaults2D::NAV_MESH_CELL_SIZE);
					NavigationServer2D::map_set_active(new_layer_map, true);
					navigation_map_override = new_layer_map;
				}
			}
		}
	}

	// ----------- Navigation regions processing -----------
	if (forced_cleanup) {
		// Clean everything.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			_navigation_clear_cell(kv.value);
		}
	}
	else {
		if (_navigation_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET] ||
			dirty.flags[DIRTY_FLAGS_LAYER_IN_TREE] ||
			dirty.flags[DIRTY_FLAGS_LAYER_NAVIGATION_MAP]) {
			// Update all cells.
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				_navigation_update_cell(kv.value);
			}
		}
		else {
			// Update dirty cells.
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_navigation_update_cell(cell_data);
			}
		}
	}

	// -----------
	// Mark the navigation state as up to date.
	_navigation_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_navigation_notification(int p_what)
{
	if (p_what == NOTIFICATION_TRANSFORM_CHANGED) {
		if (tile_set.is_valid()) {
			Transform2D tilemap_xform = get_global_transform();
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				const CellData& cell_data = kv.value;
				// Update navigation regions transform.
				for (const RID& region : cell_data.navigation_regions) {
					if (!region.is_valid()) {
						continue;
					}
					Transform2D tile_transform;
					tile_transform.set_origin(tile_set->map_to_local(kv.key));
					NavigationServer2D::region_set_transform(
						region, tilemap_xform * tile_transform);
				}
			}
		}
	}
}

void TileMapLayer::_navigation_clear_cell(CellData& r_cell_data)
{
	// Clear navigation shapes.
	for (uint32_t i = 0; i < r_cell_data.navigation_regions.size(); i++) {
		const RID& region = r_cell_data.navigation_regions[i];
		if (region.is_valid()) {
			NavigationServer2D::region_set_map(region, RID());
			NavigationServer2D::free_rid(region);
		}
	}
	r_cell_data.navigation_regions.clear();
}

#ifdef DEBUG_ENABLED

#endif // DEBUG_ENABLED
#endif // NAVIGATION_2D_DISABLED

void TileMapLayer::_scenes_update(bool p_force_cleanup)
{
	// Check if we should cleanup everything.
	bool forced_cleanup = p_force_cleanup || !enabled || !is_inside_tree() || tile_set.is_null();
	if (forced_cleanup && _scenes_was_cleaned_up) {
		return;
	}

	if (forced_cleanup) {
		// Clean everything.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			_scenes_clear_cell(kv.value);
		}
	}
	else {
		if (_scenes_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET] ||
			dirty.flags[DIRTY_FLAGS_LAYER_IN_TREE] ||
			dirty.flags[DIRTY_FLAGS_LAYER_HIGHLIGHT_MODE]) {
			// Update all cells.
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				_scenes_update_cell(kv.value);
			}
		}
		else {
			// Update dirty cells.
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_scenes_update_cell(cell_data);
			}
		}
	}

	// -----------
	// Mark the scenes state as up to date.
	_scenes_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_scenes_clear_cell(CellData& r_cell_data)
{
	// Cleanup existing scene.
	Node* node = nullptr;
	if (tile_map_node) {
		// Compatibility with TileMap.
		node = tile_map_node->get_node_or_null(r_cell_data.scene);
	}
	else {
		node = get_node_or_null(r_cell_data.scene);
	}
	if (node) {
		node->queue_free();
	}
	r_cell_data.scene = "";
}

#ifdef DEBUG_ENABLED

#endif // DEBUG_ENABLED

void TileMapLayer::_set_scene_transform_with_alternative(
	Node2D* p_scene, const Vector2& p_cell_position, const int p_alternative_id)
{
	// Determine the transformations based on the alternative ID.
	bool transform_flip_h = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_H;
	bool transform_flip_v = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_V;
	bool transform_transpose = p_alternative_id & TileSetAtlasSource::TRANSFORM_TRANSPOSE;

	int axis_h = transform_transpose ? 1 : 0;
	int axis_v = 1 - axis_h;

	Transform2D xform;
	xform[axis_h].x = transform_flip_h ? -1.0f : 1.0f;
	xform[axis_h].y = 0.0f;
	xform[axis_v].x = 0.0f;
	xform[axis_v].y = transform_flip_v ? -1.0f : 1.0f;
	xform.set_origin(p_cell_position);

	p_scene->set_transform(xform * p_scene->get_transform());
}

/////////////////////////////////////////////////////////////////////

void TileMapLayer::_build_runtime_update_tile_data(bool p_force_cleanup)
{
	// Check if we should cleanup everything.
	bool forced_cleanup =
		p_force_cleanup || !enabled || tile_set.is_null() || !is_visible_in_tree();
	if (!forced_cleanup) {
		if (_runtime_update_tile_data_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET]) {
			_runtime_update_needs_all_cells_cleaned_up = true;
			for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
				_build_runtime_update_tile_data_for_cell(E.value, false);
			}
		}
		else if (dirty.flags[DIRTY_FLAGS_LAYER_RUNTIME_UPDATE]) {
			for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
				_build_runtime_update_tile_data_for_cell(E.value, false, true);
			}
		}
		else {
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_build_runtime_update_tile_data_for_cell(cell_data, false);
			}
		}
	}

	// -----------
	// Mark the tile data state as up to date.
	_runtime_update_tile_data_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_clear_runtime_update_tile_data()
{
	if (_runtime_update_needs_all_cells_cleaned_up) {
		for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			_clear_runtime_update_tile_data_for_cell(E.value);
		}
		_runtime_update_needs_all_cells_cleaned_up = false;
	}
	else {
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& r_cell_data = *cell_data_list_element->self();
			_clear_runtime_update_tile_data_for_cell(r_cell_data);
		}
	}
}

void TileMapLayer::_clear_runtime_update_tile_data_for_cell(CellData& r_cell_data)
{
	// Clear the runtime tile data.
	if (r_cell_data.runtime_tile_data_cache) {
		memdelete(r_cell_data.runtime_tile_data_cache);
		r_cell_data.runtime_tile_data_cache = nullptr;
	}
}

TileSet::TerrainsPattern TileMapLayer::_get_best_terrain_pattern_for_constraints(int p_terrain_set,
	const Vector2i& p_position, const RBSet<TerrainConstraint>& p_constraints,
	TileSet::TerrainsPattern p_current_pattern) const
{
	if (tile_set.is_null()) {
		return TileSet::TerrainsPattern();
	}
	// Returns all tiles compatible with the given constraints.
	RBMap<TileSet::TerrainsPattern, int> terrain_pattern_score;
	RBSet<TileSet::TerrainsPattern> pattern_set = tile_set->get_terrains_pattern_set(p_terrain_set);
	ERR_FAIL_COND_V(pattern_set.is_empty(), TileSet::TerrainsPattern());
	for (TileSet::TerrainsPattern& terrain_pattern : pattern_set) {
		int score = 0;

		// Check the center bit constraint.
		TerrainConstraint terrain_constraint =
			TerrainConstraint(tile_set, p_position, terrain_pattern.get_terrain());
		const RBSet<TerrainConstraint>::Element* in_set_constraint_element =
			p_constraints.find(terrain_constraint);
		if (in_set_constraint_element) {
			if (in_set_constraint_element->get().get_terrain() !=
				terrain_constraint.get_terrain()) {
				score += in_set_constraint_element->get().get_priority();
			}
		}
		else if (p_current_pattern.get_terrain() != terrain_pattern.get_terrain()) {
			continue; // Ignore a pattern that cannot keep bits without constraints unmodified.
		}

		// Check the surrounding bits
		bool invalid_pattern = false;
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				// Check if the bit is compatible with the constraints.
				TerrainConstraint terrain_bit_constraint = TerrainConstraint(
					tile_set, p_position, bit, terrain_pattern.get_terrain_peering_bit(bit));
				in_set_constraint_element = p_constraints.find(terrain_bit_constraint);
				if (in_set_constraint_element) {
					if (in_set_constraint_element->get().get_terrain() !=
						terrain_bit_constraint.get_terrain()) {
						score += in_set_constraint_element->get().get_priority();
					}
				}
				else if (p_current_pattern.get_terrain_peering_bit(bit) !=
						   terrain_pattern.get_terrain_peering_bit(bit)) {
					invalid_pattern = true; // Ignore a pattern that cannot keep bits without
											// constraints unmodified.
					break;
				}
			}
		}
		if (invalid_pattern) {
			continue;
		}

		terrain_pattern_score[terrain_pattern] = score;
	}

	// Compute the minimum score.
	TileSet::TerrainsPattern min_score_pattern = p_current_pattern;
	int min_score = INT32_MAX;
	for (KeyValue<TileSet::TerrainsPattern, int> E : terrain_pattern_score) {
		if (E.value < min_score) {
			min_score_pattern = E.key;
			min_score = E.value;
		}
	}

	return min_score_pattern;
}

RBSet<TerrainConstraint> TileMapLayer::_get_terrain_constraints_from_added_pattern(
	const Vector2i& p_position, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern) const
{
	if (tile_set.is_null()) {
		return RBSet<TerrainConstraint>();
	}

	// Compute the constraints needed from the surrounding tiles.
	RBSet<TerrainConstraint> output;
	output.insert(TerrainConstraint(tile_set, p_position, p_terrains_pattern.get_terrain()));

	for (uint32_t i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		TileSet::CellNeighbor side = TileSet::CellNeighbor(i);
		if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, side)) {
			TerrainConstraint c = TerrainConstraint(
				tile_set, p_position, side, p_terrains_pattern.get_terrain_peering_bit(side));
			output.insert(c);
		}
	}

	return output;
}

RBSet<TerrainConstraint> TileMapLayer::_get_terrain_constraints_from_painted_cells_list(
	const RBSet<Vector2i>& p_painted, int p_terrain_set, bool p_ignore_empty_terrains) const
{
	if (tile_set.is_null()) {
		return RBSet<TerrainConstraint>();
	}

	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(), RBSet<TerrainConstraint>());

	// Build a set of dummy constraints to get the constrained points.
	RBSet<TerrainConstraint> dummy_constraints;
	for (const Vector2i& E : p_painted) {
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) { // Iterates over neighbor bits.
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				dummy_constraints.insert(TerrainConstraint(tile_set, E, bit, -1));
			}
		}
	}

	// For each constrained point, we get all overlapping tiles, and select the most adequate
	// terrain for it.
	RBSet<TerrainConstraint> constraints;
	for (const TerrainConstraint& E_constraint : dummy_constraints) {
		HashMap<int, int> terrain_count;

		// Count the number of occurrences per terrain.
		HashMap<Vector2i, TileSet::CellNeighbor> overlapping_terrain_bits =
			E_constraint.get_overlapping_coords_and_peering_bits();
		for (const KeyValue<Vector2i, TileSet::CellNeighbor>& E_overlapping :
			overlapping_terrain_bits) {
			TileData* neighbor_tile_data = nullptr;
			TileMapCell neighbor_cell = get_cell(E_overlapping.key);
			if (neighbor_cell.source_id != TileSet::INVALID_SOURCE) {
				Ref<TileSetSource> source = tile_set->get_source(neighbor_cell.source_id);
				Ref<TileSetAtlasSource> atlas_source = source;
				if (atlas_source.is_valid()) {
					TileData* tile_data = atlas_source->get_tile_data(
						neighbor_cell.get_atlas_coords(), neighbor_cell.alternative_tile);
					if (tile_data && tile_data->get_terrain_set() == p_terrain_set) {
						neighbor_tile_data = tile_data;
					}
				}
			}

			int terrain = neighbor_tile_data ? neighbor_tile_data->get_terrain_peering_bit(
												   TileSet::CellNeighbor(E_overlapping.value))
											 : -1;
			if (!p_ignore_empty_terrains || terrain >= 0) {
				if (!terrain_count.has(terrain)) {
					terrain_count[terrain] = 0;
				}
				terrain_count[terrain] += 1;
			}
		}

		// Get the terrain with the max number of occurrences.
		int max = 0;
		int max_terrain = -1;
		for (const KeyValue<int, int>& E_terrain_count : terrain_count) {
			if (E_terrain_count.value > max) {
				max = E_terrain_count.value;
				max_terrain = E_terrain_count.key;
			}
		}

		// Set the adequate terrain.
		if (max > 0) {
			TerrainConstraint c = E_constraint;
			c.set_terrain(max_terrain);
			constraints.insert(c);
		}
	}

	// Add the centers as constraints.
	for (Vector2i E_coords : p_painted) {
		TileData* tile_data = nullptr;
		TileMapCell cell = get_cell(E_coords);
		if (cell.source_id != TileSet::INVALID_SOURCE) {
			Ref<TileSetSource> source = tile_set->get_source(cell.source_id);
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				tile_data =
					atlas_source->get_tile_data(cell.get_atlas_coords(), cell.alternative_tile);
			}
		}

		int terrain = (tile_data && tile_data->get_terrain_set() == p_terrain_set)
						  ? tile_data->get_terrain()
						  : -1;
		if (!p_ignore_empty_terrains || terrain >= 0) {
			constraints.insert(TerrainConstraint(tile_set, E_coords, terrain));
		}
	}

	return constraints;
}

void TileMapLayer::_update_notify_local_transform()
{
	bool notify = is_using_kinematic_bodies() || is_y_sort_enabled();
	if (!notify) {
		if (is_y_sort_enabled()) {
			notify = true;
		}
	}
	set_notify_local_transform(notify);
}

void TileMapLayer::_deferred_internal_update()
{
	// Other updates.
	if (!pending_update) {
		return;
	}

	// Update dirty quadrants on layers.
	_internal_update(false);
}

void TileMapLayer::_internal_update(bool p_force_cleanup)
{
	// Find TileData that need a runtime modification.
	// This may add cells to the dirty list if a runtime modification has been notified.
	_build_runtime_update_tile_data(p_force_cleanup);

	// Callback for implementing custom subsystems.
	// This may add to the dirty list if some cells are changed inside _update_cells.
	_update_cells_callback(p_force_cleanup);

	// Update all subsystems.
	_rendering_update(p_force_cleanup);
#ifndef PHYSICS_2D_DISABLED
	_physics_update(p_force_cleanup);
#endif // PHYSICS_2D_DISABLED
#ifndef NAVIGATION_2D_DISABLED
	_navigation_update(p_force_cleanup);
#endif // NAVIGATION_2D_DISABLED
	_scenes_update(p_force_cleanup);
#ifdef DEBUG_ENABLED
	_debug_update(p_force_cleanup);
#endif // DEBUG_ENABLED

	_clear_runtime_update_tile_data();

	// Clear the "what is dirty" flags.
	for (int i = 0; i < DIRTY_FLAGS_MAX; i++) {
		dirty.flags[i] = false;
	}

	// List the cells to delete definitely.
	Vector<Vector2i> to_delete;
	for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
		 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
		CellData& cell_data = *cell_data_list_element->self();
		// Select the cell from tile_map if it is invalid.
		if (cell_data.cell.source_id == TileSet::INVALID_SOURCE) {
			to_delete.push_back(cell_data.coords);
		}
	}

	// Remove cells that are empty after the cleanup.
	for (const Vector2i& coords : to_delete) {
		tile_map_layer_data.erase(coords);
	}

	// Clear the dirty cells list.
	dirty.cell_list.clear();

	pending_update = false;
}

void TileMapLayer::_physics_interpolated_changed()
{
	bool interpolated = is_physics_interpolated();
	bool needs_reset = interpolated && is_visible_in_tree();

	for (const KeyValue<Vector2i, Ref<RenderingQuadrant>>& kv : rendering_quadrant_map) {
		for (const RID& ci : kv.value->canvas_items) {
			if (ci.is_valid()) {
				Renderer::canvas_item_set_interpolated(ci, interpolated);
				if (needs_reset) {
					Renderer::canvas_item_reset_physics_interpolation(ci);
				}
			}
		}
	}

	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		for (const LocalVector<RID>& polygons : E.value.occluders) {
			for (const RID& occluder_id : polygons) {
				if (occluder_id.is_valid()) {
					Renderer::canvas_light_occluder_set_interpolated(occluder_id, interpolated);
					if (needs_reset) {
						Renderer::canvas_light_occluder_reset_physics_interpolation(occluder_id);
					}
				}
			}
		}
	}
}

#ifdef TOOLS_ENABLED
bool TileMapLayer::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return tile_set.is_valid() &&
		   get_cell_source_id(local_to_map(p_point)) != TileSet::INVALID_SOURCE;
}
#endif

Rect2 TileMapLayer::get_rect(bool& r_changed) const
{
	if (tile_set.is_null()) {
		r_changed = rect_cache != Rect2();
		return Rect2();
	}

	// Compute the displayed area of the tilemap.
	r_changed = false;
#ifdef DEBUG_ENABLED

	if (rect_cache_dirty) {
		Rect2 r_total;
		bool first = true;
		for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			Rect2 r;
			r.position = tile_set->map_to_local(E.key);
			r.size = Size2();
			if (first) {
				r_total = r;
				first = false;
			}
			else {
				r_total = r_total.merge(r);
			}
		}

		r_changed = rect_cache != r_total;

		rect_cache = r_total;
		rect_cache_dirty = false;
	}
#endif
	return rect_cache;
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_constraints(
	const Vector<Vector2i>& p_to_replace, int p_terrain_set,
	const RBSet<TerrainConstraint>& p_constraints) const
{
	return HashMap<Vector2i, TileSet::TerrainsPattern>();
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_connect(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	// Build a set, out of the possibly modified tiles, of the one with a center bit that is set (or
	// will be) to the painted terrain.
	RBSet<Vector2i> cells_with_terrain_center_bit;
	for (Vector2i coords : can_modify_set) {
		bool connect = false;
		if (painted_set.has(coords)) {
			connect = true;
		}
		else {
			// Get the center bit of the cell.
			TileData* tile_data = nullptr;
			TileMapCell cell = get_cell(coords);
			if (cell.source_id != TileSet::INVALID_SOURCE) {
				Ref<TileSetSource> source = tile_set->get_source(cell.source_id);
				Ref<TileSetAtlasSource> atlas_source = source;
				if (atlas_source.is_valid()) {
					tile_data =
						atlas_source->get_tile_data(cell.get_atlas_coords(), cell.alternative_tile);
				}
			}

			if (tile_data && tile_data->get_terrain_set() == p_terrain_set &&
				tile_data->get_terrain() == p_terrain) {
				connect = true;
			}
		}
		if (connect) {
			cells_with_terrain_center_bit.insert(coords);
		}
	}

	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		TerrainConstraint c = TerrainConstraint(tile_set, coords, p_terrain);
		c.set_priority(10);
		constraints.insert(c);

		// Constraints on the connecting bits.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				c = TerrainConstraint(tile_set, coords, bit, p_terrain);
				c.set_priority(10);
				if ((int(bit) % 2) == 0) {
					// Side peering bits: add the constraint if the center is of the same terrain.
					Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
					if (cells_with_terrain_center_bit.has(neighbor)) {
						constraints.insert(c);
					}
				}
				else {
					// Corner peering bits: add the constraint if all tiles on the constraint has
					// the same center bit.
					HashMap<Vector2i, TileSet::CellNeighbor> overlapping_terrain_bits =
						c.get_overlapping_coords_and_peering_bits();
					bool valid = true;
					for (KeyValue<Vector2i, TileSet::CellNeighbor> kv : overlapping_terrain_bits) {
						if (!cells_with_terrain_center_bit.has(kv.key)) {
							valid = false;
							break;
						}
					}
					if (valid) {
						constraints.insert(c);
					}
				}
			}
		}
	}

	// Fills in the constraint list from existing tiles.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_path(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));
	HashMap<Vector2i, TileSet::TerrainsPattern> output;

	// Make sure the path is correct and build the peering bit list while doing it.
	Vector<TileSet::CellNeighbor> neighbor_list;
	for (int i = 0; i < p_coords_array.size() - 1; i++) {
		// Find the adequate neighbor.
		TileSet::CellNeighbor found_bit = TileSet::CELL_NEIGHBOR_MAX;
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				if (tile_set->get_neighbor_cell(p_coords_array[i], bit) == p_coords_array[i + 1]) {
					found_bit = bit;
					break;
				}
			}
		}
		ERR_FAIL_COND_V_MSG(found_bit == TileSet::CELL_NEIGHBOR_MAX, output,
			vformat("Invalid terrain path, %s is not a neighboring tile of %s",
				p_coords_array[i + 1], p_coords_array[i]));
		neighbor_list.push_back(found_bit);
	}

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		TerrainConstraint c = TerrainConstraint(tile_set, coords, p_terrain);
		c.set_priority(10);
		constraints.insert(c);
	}
	for (int i = 0; i < p_coords_array.size() - 1; i++) {
		// Constraints on the peering bits.
		TerrainConstraint c =
			TerrainConstraint(tile_set, p_coords_array[i], neighbor_list[i], p_terrain);
		c.set_priority(10);
		constraints.insert(c);
	}

	// Fills in the constraint list from existing tiles.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_pattern(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern, bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	// Add constraint by the new ones.
	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		RBSet<TerrainConstraint> added_constraints =
			_get_terrain_constraints_from_added_pattern(coords, p_terrain_set, p_terrains_pattern);
		for (TerrainConstraint c : added_constraints) {
			c.set_priority(10);
			constraints.insert(c);
		}
	}

	// Fills in the constraint list from modified tiles border.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

TileMapCell TileMapLayer::get_cell(const Vector2i& p_coords) const
{
	if (!tile_map_layer_data.has(p_coords)) {
		return TileMapCell();
	}
	else {
		return tile_map_layer_data.find(p_coords)->value.cell;
	}
}

void TileMapLayer::compute_transformed_tile_dest_rect(Rect2& r_dest_rect, bool& r_transpose,
	const Vector2& p_position, const Vector2& p_dest_rect_size, const TileData* p_tile_data,
	int p_alternative_tile)
{
	DEV_ASSERT(p_tile_data);
	// Conceptually the order of transformations is (starting from the tile centered at the origin):
	// - Per TileSet-tile transforms (transpose then flips).
	// - Translation so texture origin is at the origin.
	// - Per TileMapLayer-cell transforms (transpose then flips).
	// - Translation to target position.

	const bool tile_transpose = p_tile_data->get_transpose();
	const bool tile_flip_h = p_tile_data->get_flip_h();
	const bool tile_flip_v = p_tile_data->get_flip_v();

	const Vector2 texture_origin = p_tile_data->get_texture_origin();

	const bool cell_transpose = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_TRANSPOSE);
	const bool cell_flip_h = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_FLIP_H);
	const bool cell_flip_v = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_FLIP_V);

	const bool final_transpose = tile_transpose != cell_transpose;
	const bool final_flip_h = cell_flip_h != (cell_transpose ? tile_flip_v : tile_flip_h);
	const bool final_flip_v = cell_flip_v != (cell_transpose ? tile_flip_h : tile_flip_v);

	// Rect draw commands swap the size based on the passed transpose, so the size is left
	// non-tranposed here. Position calculations need to use transposed size though.
	Rect2 dest_rect;
	dest_rect.size = p_dest_rect_size;
	dest_rect.size.x += FP_ADJUST;
	dest_rect.size.y += FP_ADJUST;
	Vector2 transposed_size =
		final_transpose ? Vector2(dest_rect.size.y, dest_rect.size.x) : dest_rect.size;
	if (final_flip_h) {
		dest_rect.size.x = -dest_rect.size.x;
	}
	if (final_flip_v) {
		dest_rect.size.y = -dest_rect.size.y;
	}

	dest_rect.position = -0.5f * transposed_size;
	dest_rect.position -=
		cell_transpose ? Vector2(texture_origin.y, texture_origin.x) : texture_origin;
	if (cell_flip_h) {
		dest_rect.position.x = -(dest_rect.position.x + transposed_size.x);
	}
	if (cell_flip_v) {
		dest_rect.position.y = -(dest_rect.position.y + transposed_size.y);
	}
	dest_rect.position += p_position;

	r_dest_rect = dest_rect;
	r_transpose = final_transpose;
}

void TileMapLayer::set_cell(const Vector2i& p_coords, int p_source_id,
	const Vector2i& p_atlas_coords, int p_alternative_tile)
{
	// Set the current cell tile (using integer position).
	Vector2i pk(p_coords);
	HashMap<Vector2i, CellData>::Iterator E = tile_map_layer_data.find(pk);

	int source_id = p_source_id;
	Vector2i atlas_coords = p_atlas_coords;
	int alternative_tile = p_alternative_tile;

	if ((source_id == TileSet::INVALID_SOURCE ||
			atlas_coords == TileSetSource::INVALID_ATLAS_COORDS ||
			alternative_tile == TileSetSource::INVALID_TILE_ALTERNATIVE) &&
		(source_id != TileSet::INVALID_SOURCE ||
			atlas_coords != TileSetSource::INVALID_ATLAS_COORDS ||
			alternative_tile != TileSetSource::INVALID_TILE_ALTERNATIVE)) {
		source_id = TileSet::INVALID_SOURCE;
		atlas_coords = TileSetSource::INVALID_ATLAS_COORDS;
		alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	if (!E) {
		if (source_id == TileSet::INVALID_SOURCE) {
			return; // Nothing to do, the tile is already empty.
		}

		// Insert a new cell in the tile map.
		CellData new_cell_data;
		new_cell_data.coords = pk;
		E = tile_map_layer_data.insert(pk, new_cell_data);
	}
	else {
		if (E->value.cell.source_id == source_id &&
			E->value.cell.get_atlas_coords() == atlas_coords &&
			E->value.cell.alternative_tile == alternative_tile) {
			return; // Nothing changed.
		}
	}

	TileMapCell& c = E->value.cell;
	c.source_id = source_id;
	c.set_atlas_coords(atlas_coords);
	c.alternative_tile = alternative_tile;

	// Make the given cell dirty.
	if (!E->value.dirty_list_element.in_list()) {
		dirty.cell_list.add(&(E->value.dirty_list_element));
	}
	_queue_internal_update();

	used_rect_cache_dirty = true;
}

void TileMapLayer::erase_cell(const Vector2i& p_coords)
{
	set_cell(p_coords, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS,
		TileSetSource::INVALID_TILE_ALTERNATIVE);
}

void TileMapLayer::clear()
{
	// Remove all tiles.
	for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
		erase_cell(kv.key);
	}
	used_rect_cache_dirty = true;
}

int TileMapLayer::get_cell_source_id(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSet::INVALID_SOURCE;
	}

	return E->value.cell.source_id;
}

Vector2i TileMapLayer::get_cell_atlas_coords(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSetSource::INVALID_ATLAS_COORDS;
	}

	return E->value.cell.get_atlas_coords();
}

int TileMapLayer::get_cell_alternative_tile(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	return E->value.cell.alternative_tile;
}

TileData* TileMapLayer::get_cell_tile_data(const Vector2i& p_coords) const
{
	int source_id = get_cell_source_id(p_coords);
	if (source_id == TileSet::INVALID_SOURCE) {
		return nullptr;
	}

	Ref<TileSetAtlasSource> source = tile_set->get_source(source_id);
	if (source.is_valid()) {
		return source->get_tile_data(
			get_cell_atlas_coords(p_coords), get_cell_alternative_tile(p_coords));
	}

	return nullptr;
}

Rect2i TileMapLayer::get_used_rect() const
{
	// Return the rect of the currently used area.
	if (used_rect_cache_dirty) {
		used_rect_cache = Rect2i();

		bool first = true;
		for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			const TileMapCell& c = E.value.cell;
			if (c.source_id == TileSet::INVALID_SOURCE) {
				continue;
			}
			if (first) {
				used_rect_cache = Rect2i(E.key, Size2i());
				first = false;
			}
			else {
				used_rect_cache.expand_to(E.key);
			}
		}
		if (!first) {
			// Only if we have at least one cell.
			// The cache expands to top-left coordinate, so we add one full tile.
			used_rect_cache.size += Vector2i(1, 1);
		}
		used_rect_cache_dirty = false;
	}

	return used_rect_cache;
}

bool TileMapLayer::is_cell_flipped_h(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_FLIP_H;
}

bool TileMapLayer::is_cell_flipped_v(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_FLIP_V;
}

bool TileMapLayer::is_cell_transposed(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_TRANSPOSE;
}

#ifndef PHYSICS_2D_DISABLED
bool TileMapLayer::has_body_rid(RID p_physics_body) const
{
	return bodies_coords.has(p_physics_body);
}

Vector2i TileMapLayer::get_coords_for_body_rid(RID p_physics_body) const
{
	const Vector2i* found = bodies_coords.getptr(p_physics_body);
	ERR_FAIL_NULL_V(found, Vector2i());
	return *found;
}
#endif // PHYSICS_2D_DISABLED

void TileMapLayer::update_internals() { _internal_update(false); }

Vector2i TileMapLayer::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern)
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->map_pattern(p_position_in_tilemap, p_coords_in_pattern, p_pattern);
}

Vector2i TileMapLayer::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->get_neighbor_cell(p_coords, p_cell_neighbor);
}

Vector2 TileMapLayer::map_to_local(const Vector2i& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2());
	return tile_set->map_to_local(p_pos);
}

Vector2i TileMapLayer::local_to_map(const Vector2& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->local_to_map(p_pos);
}

bool TileMapLayer::is_enabled() const { return enabled; }

Ref<TileSet> TileMapLayer::get_tile_set() const { return tile_set; }

void TileMapLayer::set_highlight_mode(HighlightMode p_highlight_mode)
{
	if (p_highlight_mode == highlight_mode) {
		return;
	}
	highlight_mode = p_highlight_mode;
	dirty.flags[DIRTY_FLAGS_LAYER_HIGHLIGHT_MODE] = true;
	_queue_internal_update();
}

TileMapLayer::HighlightMode TileMapLayer::get_highlight_mode() const { return highlight_mode; }

void TileMapLayer::set_tile_map_data_from_array(const Vector<uint8_t>& p_data)
{
	if (p_data.is_empty()) {
		clear();
		return;
	}

	const int cell_data_struct_size = 12;

	int size = p_data.size();
	const uint8_t* ptr = p_data.ptr();

	// Index in the array.
	int index = 0;

	// First extract the data version.
	ERR_FAIL_COND_MSG(size < 2, "Corrupted tile map data: not enough bytes.");
	uint16_t format = decode_uint16(&ptr[index]);
	index += 2;
	ERR_FAIL_COND_MSG(format >= TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX,
		vformat("Unsupported tile map data format: %s. Expected format ID lower or equal to: %s",
			format, TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX - 1));

	// Clear the TileMap.
	clear();

	while (index < size) {
		ERR_FAIL_COND_MSG(index + cell_data_struct_size > size,
			vformat("Corrupted tile map data: tiles might be missing."));

		// Get a pointer at the start of the cell data.
		const uint8_t* cell_data_ptr = &ptr[index];

		// Extracts position in TileMap.
		int16_t x = decode_uint16(&cell_data_ptr[0]);
		int16_t y = decode_uint16(&cell_data_ptr[2]);

		// Extracts the tile identifiers.
		uint16_t source_id = decode_uint16(&cell_data_ptr[4]);
		uint16_t atlas_coords_x = decode_uint16(&cell_data_ptr[6]);
		uint16_t atlas_coords_y = decode_uint16(&cell_data_ptr[8]);
		uint16_t alternative_tile = decode_uint16(&cell_data_ptr[10]);

		set_cell(
			Vector2i(x, y), source_id, Vector2i(atlas_coords_x, atlas_coords_y), alternative_tile);
		index += cell_data_struct_size;
	}
}

Vector<uint8_t> TileMapLayer::get_tile_map_data_as_array() const
{
	const int cell_data_struct_size = 12;

	Vector<uint8_t> tile_map_data_array;
	if (tile_map_layer_data.is_empty()) {
		return tile_map_data_array;
	}

	tile_map_data_array.resize(2 + tile_map_layer_data.size() * cell_data_struct_size);
	uint8_t* ptr = tile_map_data_array.ptrw();

	// Index in the array.
	int index = 0;

	// Save the version.
	encode_uint16(TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX - 1, &ptr[index]);
	index += 2;

	// Save in highest format.
	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		// Get a pointer at the start of the cell data.
		uint8_t* cell_data_ptr = (uint8_t*)&ptr[index];

		// Store position in TileMap.
		encode_uint16((int16_t)(E.key.x), &cell_data_ptr[0]);
		encode_uint16((int16_t)(E.key.y), &cell_data_ptr[2]);

		// Store the tile identifiers.
		encode_uint16(E.value.cell.source_id, &cell_data_ptr[4]);
		encode_uint16(E.value.cell.coord_x, &cell_data_ptr[6]);
		encode_uint16(E.value.cell.coord_y, &cell_data_ptr[8]);
		encode_uint16(E.value.cell.alternative_tile, &cell_data_ptr[10]);

		index += cell_data_struct_size;
	}

	return tile_map_data_array;
}

int TileMapLayer::get_y_sort_origin() const { return y_sort_origin; }

bool TileMapLayer::is_x_draw_order_reversed() const { return x_draw_order_reversed; }

int TileMapLayer::get_rendering_quadrant_size() const { return rendering_quadrant_size; }

bool TileMapLayer::is_collision_enabled() const { return collision_enabled; }

bool TileMapLayer::is_using_kinematic_bodies() const { return use_kinematic_bodies; }

TileMapLayer::DebugVisibilityMode TileMapLayer::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

int TileMapLayer::get_physics_quadrant_size() const { return physics_quadrant_size; }

bool TileMapLayer::is_occlusion_enabled() const { return occlusion_enabled; }

#ifndef NAVIGATION_2D_DISABLED

bool TileMapLayer::is_navigation_enabled() const { return navigation_enabled; }

RID TileMapLayer::get_navigation_map() const
{
	if (navigation_map_override.is_valid()) {
		return navigation_map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

TileMapLayer::DebugVisibilityMode TileMapLayer::get_navigation_visibility_mode() const
{
	return navigation_visibility_mode;
}

#endif // NAVIGATION_2D_DISABLED

TileMapLayer::TileMapLayer() { set_notify_transform(true); }

TileMapLayer::~TileMapLayer()
{
	clear();
	_internal_update(true);
}

HashMap<Vector2i, TileSet::CellNeighbor>
TerrainConstraint::get_overlapping_coords_and_peering_bits() const
{
	ERR_FAIL_COND_V(is_center_bit(), (HashMap<Vector2i, TileSet::CellNeighbor>()));
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::CellNeighbor>()));
	HashMap<Vector2i, TileSet::CellNeighbor> output;

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (bit) {
		case 1:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
			break;
		case 2:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(
				base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER)] =
				TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
			break;
		case 3:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
			break;
		default:
			ERR_FAIL_V(output);
		}
	}
	else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (bit) {
		case 1:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
			break;
		case 2:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_CORNER)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
			break;
		case 3:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
			break;
		default:
			ERR_FAIL_V(output);
		}
	}
	else {
		// Half offset shapes.
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (bit) {
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
				break;
			case 4:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
				break;
			case 5:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
				break;
			default:
				ERR_FAIL_V(output);
			}
		}
		else {
			switch (bit) {
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				break;
			case 4:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
				break;
			case 5:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
				break;
			default:
				ERR_FAIL_V(output);
			}
		}
	}
	return output;
}

TerrainConstraint::TerrainConstraint(
	Ref<TileSet> p_tile_set, const Vector2i& p_position, int p_terrain)
{
	ERR_FAIL_COND(p_tile_set.is_null());
	tile_set = p_tile_set;
	bit = 0;
	base_cell_coords = p_position;
	terrain = p_terrain;
}

TerrainConstraint::TerrainConstraint(Ref<TileSet> p_tile_set, const Vector2i& p_position,
	const TileSet::CellNeighbor& p_bit, int p_terrain)
{
	// The way we build the constraint make it easy to detect conflicting constraints.
	ERR_FAIL_COND(p_tile_set.is_null());
	tile_set = p_tile_set;

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			bit = 1;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			bit = 2;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			bit = 3;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			bit = 1;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			bit = 3;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
			break;
		default:
			ERR_FAIL();
			break;
		}
	}
	else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			bit = 1;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			bit = 2;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			bit = 3;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			bit = 1;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_CORNER);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			bit = 3;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
			break;
		default:
			ERR_FAIL();
			break;
		}
	}
	else {
		// Half-offset shapes.
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (p_bit) {
			case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
				bit = 1;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
				bit = 2;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
				bit = 3;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
				bit = 4;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
				bit = 5;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
				bit = 1;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_CORNER:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
				bit = 5;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			default:
				ERR_FAIL();
				break;
			}
		}
		else {
			switch (p_bit) {
			case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
				bit = 1;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
				bit = 2;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
				bit = 3;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
				bit = 4;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
				bit = 1;
				base_cell_coords = tile_set->get_neighbor_cell(
					p_position, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
				bit = 5;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
				bit = 1;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_SIDE:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
				bit = 5;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			default:
				ERR_FAIL();
				break;
			}
		}
	}
	terrain = p_terrain;
}

void TileMapLayer::_build_runtime_update_tile_data_for_cell(CellData&, bool, bool) {}

void TileMapLayer::_scenes_update_cell(CellData&) {}

void TileMapLayer::_queue_internal_update() {}

void TileMapLayer::notify_runtime_tile_data_update() {}

void TileMapLayer::_navigation_update_cell(CellData&) {}

void TileMapLayer::draw_tile(
	RID, Vector2 const&, Ref<TileSet>, int, Vector2i const&, int, int, TileData const*, float)
{
}

void TileMapLayer::_update_cells_callback(bool) {}

void TileMapLayer::set_use_kinematic_bodies(bool) {}

void TileMapLayer::set_rendering_quadrant_size(int) {}

void TileMapLayer::set_pattern(Vector2i const&, Ref<TileMapPattern>) {}

void TileMapLayer::set_navigation_visibility_mode(TileMapLayer::DebugVisibilityMode) {}

void TileMapLayer::set_navigation_map(RID) {}

void TileMapLayer::set_navigation_enabled(bool) {}

void TileMapLayer::set_enabled(bool) {}

void TileMapLayer::set_collision_visibility_mode(TileMapLayer::DebugVisibilityMode) {}

void TileMapLayer::_scenes_draw_cell_debug(RID const&, Vector2 const&, CellData const&) {}

void TileMapLayer::_rendering_update(bool) {}

void TileMapLayer::_rendering_draw_cell_debug(RID const&, Vector2 const&, CellData const&) {}

void TileMapLayer::_physics_update(bool) {}

void TileMapLayer::_physics_draw_quadrant_debug(RID const&, DebugQuadrant&) {}

void TileMapLayer::_navigation_draw_cell_debug(RID const&, Vector2 const&, CellData const&) {}

void TileMapLayer::fix_invalid_tiles() {}

void TileMapLayer::_update_self_texture_repeat(RSE::CanvasItemTextureRepeat p_texture_repeat) {}

void TileMapLayer::_update_self_texture_filter(RSE::CanvasItemTextureFilter p_texture_filter) {}

void TileMapLayer::set_z_index(int p_z_index) {}

void TileMapLayer::set_y_sort_enabled(bool p_y_sort_enabled) {}

void TileMapLayer::set_self_modulate(const Color& p_self_modulate) {}

void TileMapLayer::set_light_mask(int p_light_mask) {}

// ==========================================================================
// Merged from scene/2d/tile_map.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/io/marshalls.h"
// --- tile_map.compat.inc ---
#ifndef DISABLE_DEPRECATED


Rect2i TileMap::_get_used_rect_bind_compat_78328() { return get_used_rect(); }

void TileMap::_set_quadrant_size_bind_compat_81070(int p_quadrant_size)
{
	set_rendering_quadrant_size(p_quadrant_size);
}

int TileMap::_get_quadrant_size_bind_compat_81070() const { return get_rendering_quadrant_size(); }

TileMap::VisibilityMode TileMap::_get_collision_visibility_mode_bind_compat_87115()
{
	return get_collision_visibility_mode();
}

#ifndef NAVIGATION_2D_DISABLED
TileMap::VisibilityMode TileMap::_get_navigation_visibility_mode_bind_compat_87115()
{
	return get_navigation_visibility_mode();
}
#endif // NAVIGATION_2D_DISABLED

void TileMap::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

#define TILEMAP_CALL_FOR_LAYER(layer, function, ...)                                               \
	if (layer < 0) {                                                                               \
		layer = layers.size() + layer;                                                             \
	};                                                                                             \
	ERR_FAIL_INDEX(layer, (int)layers.size());                                                     \
	layers[layer]->function(__VA_ARGS__);

#define TILEMAP_CALL_FOR_LAYER_V(layer, err_value, function, ...)                                  \
	if (layer < 0) {                                                                               \
		layer = layers.size() + layer;                                                             \
	};                                                                                             \
	ERR_FAIL_INDEX_V(layer, (int)layers.size(), err_value);                                        \
	return layers[layer]->function(__VA_ARGS__);

#ifndef NAVIGATION_2D_DISABLED
RID TileMap::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

Vector<int> TileMap::_get_tile_map_data_using_compatibility_format(int p_layer) const
{
	ERR_FAIL_INDEX_V(p_layer, (int)layers.size(), Vector<int>());

	// Export tile data to raw format.
	const HashMap<Vector2i, CellData> tile_map_layer_data(
		layers[p_layer]->get_tile_map_layer_data());
	Vector<int> tile_data;
	tile_data.resize(tile_map_layer_data.size() * 3);
	int* w = tile_data.ptrw();

	// Save in highest format.

	int idx = 0;
	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		uint8_t* ptr = (uint8_t*)&w[idx];
		encode_uint16((int16_t)(E.key.x), &ptr[0]);
		encode_uint16((int16_t)(E.key.y), &ptr[2]);
		encode_uint16(E.value.cell.source_id, &ptr[4]);
		encode_uint16(E.value.cell.coord_x, &ptr[6]);
		encode_uint16(E.value.cell.coord_y, &ptr[8]);
		encode_uint16(E.value.cell.alternative_tile, &ptr[10]);
		idx += 3;
	}

	return tile_data;
}

void TileMap::_set_layer_tile_data(int p_layer, const PackedInt32Array& p_data)
{
	_set_tile_map_data_using_compatibility_format(p_layer, format, p_data);
}

void TileMap::_notification(int p_what)
{
	switch (p_what) {
	case TileMap::NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		// This is only executed when collision_animatable is enabled.

		bool in_editor = false;
#ifdef TOOLS_ENABLED
		in_editor = Engine::is_editor_hint();
#endif // TOOLS_ENABLED
		if (is_inside_tree() && collision_animatable && !in_editor) {
			// Update transform on the physics tick when in animatable mode.
			last_valid_transform = new_transform;
			set_notify_local_transform(false);
			set_global_transform(new_transform);
			set_notify_local_transform(true);
		}
	} break;

	case TileMap::NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
		// This is only executed when collision_animatable is enabled.

		bool in_editor = false;
#ifdef TOOLS_ENABLED
		in_editor = Engine::is_editor_hint();
#endif // TOOLS_ENABLED

		if (is_inside_tree() && collision_animatable && !in_editor) {
			// Store last valid transform.
			new_transform = get_global_transform();

			// ... but then revert changes.
			set_notify_local_transform(false);
			set_global_transform(last_valid_transform);
			set_notify_local_transform(true);
		}
	} break;
	}
}

#ifndef DISABLE_DEPRECATED
// Deprecated methods.
void TileMap::force_update(int p_layer)
{
	notify_runtime_tile_data_update(p_layer);
	update_internals();
}
#endif // DISABLE_DEPRECATED

void TileMap::set_rendering_quadrant_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 1, "TileMapQuadrant size cannot be smaller than 1.");

	rendering_quadrant_size = p_size;
	for (TileMapLayer* layer : layers) {
		layer->set_rendering_quadrant_size(p_size);
	}
	_emit_changed();
}

int TileMap::get_rendering_quadrant_size() const { return rendering_quadrant_size; }

Ref<TileSet> TileMap::get_tileset() const { return tile_set; }

int TileMap::get_layers_count() const { return layers.size(); }

void TileMap::set_layer_name(int p_layer, String p_name)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_name, p_name);
}

String TileMap::get_layer_name(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, "", get_name);
}

void TileMap::set_layer_enabled(int p_layer, bool p_enabled)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_enabled, p_enabled);
}

bool TileMap::is_layer_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_enabled);
}

void TileMap::set_layer_modulate(int p_layer, Color p_modulate)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_modulate, p_modulate);
}

Color TileMap::get_layer_modulate(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, Color(), get_modulate);
}

bool TileMap::is_layer_y_sort_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_y_sort_enabled);
}

int TileMap::get_layer_y_sort_origin(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, 0, get_y_sort_origin);
}

void TileMap::set_layer_z_index(int p_layer, int p_z_index)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_z_index, p_z_index);
}

int TileMap::get_layer_z_index(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, 0, get_z_index);
}

#ifndef NAVIGATION_2D_DISABLED
void TileMap::set_layer_navigation_enabled(int p_layer, bool p_enabled)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_navigation_enabled, p_enabled);
}

bool TileMap::is_layer_navigation_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_navigation_enabled);
}

void TileMap::set_layer_navigation_map(int p_layer, RID p_map)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_navigation_map, p_map);
}

RID TileMap::get_layer_navigation_map(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, RID(), get_navigation_map);
}
#endif // NAVIGATION_2D_DISABLED

void TileMap::set_collision_animatable(bool p_collision_animatable)
{
	if (collision_animatable == p_collision_animatable) {
		return;
	}
	collision_animatable = p_collision_animatable;
	set_notify_local_transform(p_collision_animatable);
	set_physics_process_internal(p_collision_animatable);
	for (TileMapLayer* layer : layers) {
		layer->set_use_kinematic_bodies(layer);
	}
}

bool TileMap::is_collision_animatable() const { return collision_animatable; }

void TileMap::set_collision_visibility_mode(TileMap::VisibilityMode p_show_collision)
{
	if (collision_visibility_mode == p_show_collision) {
		return;
	}
	collision_visibility_mode = p_show_collision;
	for (TileMapLayer* layer : layers) {
		layer->set_collision_visibility_mode(TileMapLayer::DebugVisibilityMode(p_show_collision));
	}
	_emit_changed();
}

TileMap::VisibilityMode TileMap::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

#ifndef NAVIGATION_2D_DISABLED
void TileMap::set_navigation_visibility_mode(TileMap::VisibilityMode p_show_navigation)
{
	if (navigation_visibility_mode == p_show_navigation) {
		return;
	}
	navigation_visibility_mode = p_show_navigation;
	for (TileMapLayer* layer : layers) {
		layer->set_navigation_visibility_mode(TileMapLayer::DebugVisibilityMode(p_show_navigation));
	}
	_emit_changed();
}

TileMap::VisibilityMode TileMap::get_navigation_visibility_mode() const
{
	return navigation_visibility_mode;
}
#endif // NAVIGATION_2D_DISABLED

void TileMap::set_cell(int p_layer, const Vector2i& p_coords, int p_source_id,
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	TILEMAP_CALL_FOR_LAYER(
		p_layer, set_cell, p_coords, p_source_id, p_atlas_coords, p_alternative_tile);
}

void TileMap::erase_cell(int p_layer, const Vector2i& p_coords)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_cell, p_coords, TileSet::INVALID_SOURCE,
		TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
}

bool TileMap::is_cell_flipped_h(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_FLIP_H;
}

bool TileMap::is_cell_flipped_v(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_FLIP_V;
}

bool TileMap::is_cell_transposed(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_TRANSPOSE;
}

Vector2i TileMap::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern)
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->map_pattern(p_position_in_tilemap, p_coords_in_pattern, p_pattern);
}

void TileMap::set_pattern(
	int p_layer, const Vector2i& p_position, const Ref<TileMapPattern> p_pattern)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_pattern, p_position, p_pattern);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_constraints(int p_layer,
	const Vector<Vector2i>& p_to_replace, int p_terrain_set,
	const RBSet<TerrainConstraint>& p_constraints)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_constraints, p_to_replace, p_terrain_set, p_constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_connect(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_connect, p_coords_array, p_terrain_set, p_terrain, p_ignore_empty_terrains);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_path(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_path, p_coords_array, p_terrain_set, p_terrain, p_ignore_empty_terrains);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_pattern(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern, bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_pattern, p_coords_array, p_terrain_set, p_terrains_pattern,
		p_ignore_empty_terrains);
}

TileMapCell TileMap::get_cell(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	if (p_use_proxies) {
		WARN_DEPRECATED_MSG("use_proxies is deprecated.");
	}
	TILEMAP_CALL_FOR_LAYER_V(p_layer, TileMapCell(), get_cell, p_coords);
}

#ifndef PHYSICS_2D_DISABLED
Vector2i TileMap::get_coords_for_body_rid(RID p_physics_body)
{
	for (const TileMapLayer* layer : layers) {
		if (layer->has_body_rid(p_physics_body)) {
			return layer->get_coords_for_body_rid(p_physics_body);
		}
	}
	ERR_FAIL_V_MSG(
		Vector2i(), vformat("No tiles for the given body RID %d.", p_physics_body.get_id()));
}

int TileMap::get_layer_for_body_rid(RID p_physics_body)
{
	for (uint32_t i = 0; i < layers.size(); i++) {
		if (layers[i]->has_body_rid(p_physics_body)) {
			return i;
		}
	}
	ERR_FAIL_V_MSG(-1, vformat("No tiles for the given body RID %d.", p_physics_body.get_id()));
}
#endif // PHYSICS_2D_DISABLED

void TileMap::fix_invalid_tiles()
{
	for (TileMapLayer* layer : layers) {
		layer->fix_invalid_tiles();
	}
}

void TileMap::clear_layer(int p_layer) { TILEMAP_CALL_FOR_LAYER(p_layer, clear) }

void TileMap::clear()
{
	for (TileMapLayer* layer : layers) {
		layer->clear();
	}
}

void TileMap::update_internals()
{
	for (TileMapLayer* layer : layers) {
		layer->update_internals();
	}
}

void TileMap::notify_runtime_tile_data_update(int p_layer)
{
	if (p_layer >= 0) {
		TILEMAP_CALL_FOR_LAYER(p_layer, notify_runtime_tile_data_update);
	}
	else {
		for (TileMapLayer* layer : layers) {
			layer->notify_runtime_tile_data_update();
		}
	}
}

#ifdef DEBUG_ENABLED
Rect2 TileMap::_edit_get_rect() const
{
	// Return the visible rect of the tilemap.
	if (layers.is_empty()) {
		return Rect2();
	}

	bool any_changed = false;
	bool changed = false;
	Rect2 rect = layers[0]->get_rect(changed);
	any_changed |= changed;
	for (uint32_t i = 1; i < layers.size(); i++) {
		rect = rect.merge(layers[i]->get_rect(changed));
		any_changed |= changed;
	}
	const_cast<TileMap*>(this)->item_rect_changed(any_changed);
	return rect;
}
#endif // DEBUG_ENABLED

Vector2 TileMap::map_to_local(const Vector2i& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2());
	return tile_set->map_to_local(p_pos);
}

Vector2i TileMap::local_to_map(const Vector2& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->local_to_map(p_pos);
}

bool TileMap::is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), false);
	return tile_set->is_existing_neighbor(p_cell_neighbor);
}

Vector2i TileMap::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->get_neighbor_cell(p_coords, p_cell_neighbor);
}

Rect2i TileMap::get_used_rect() const
{
	// Return the visible rect of the tilemap.
	bool first = true;
	Rect2i rect = Rect2i();
	for (const TileMapLayer* layer : layers) {
		Rect2i layer_rect = layer->get_used_rect();
		if (layer_rect == Rect2i()) {
			continue;
		}
		if (first) {
			rect = layer_rect;
			first = false;
		}
		else {
			rect = rect.merge(layer_rect);
		}
	}
	return rect;
}

// --- Override some methods of the CanvasItem class to pass the changes to the quadrants
// CanvasItems ---

void TileMap::set_light_mask(int p_light_mask)
{
	// Set light mask for occlusion and applies it to all layers too.
	CanvasItem::set_light_mask(p_light_mask);
	for (TileMapLayer* layer : layers) {
		layer->set_light_mask(p_light_mask);
	}
}

void TileMap::set_self_modulate(const Color& p_self_modulate)
{
	// Set self_modulation and applies it to all layers too.
	CanvasItem::set_self_modulate(p_self_modulate);
	for (TileMapLayer* layer : layers) {
		layer->set_self_modulate(p_self_modulate);
	}
}

void TileMap::set_texture_filter(TextureFilter p_texture_filter)
{
	// Set a default texture filter and applies it to all layers too.
	CanvasItem::set_texture_filter(p_texture_filter);
	for (TileMapLayer* layer : layers) {
		layer->set_texture_filter(p_texture_filter);
	}
}

void TileMap::set_texture_repeat(CanvasItem::TextureRepeat p_texture_repeat)
{
	// Set a default texture repeat and applies it to all layers too.
	CanvasItem::set_texture_repeat(p_texture_repeat);
	for (TileMapLayer* layer : layers) {
		layer->set_texture_repeat(p_texture_repeat);
	}
}

PackedStringArray TileMap::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	warnings.push_back(
		RTR("The TileMap node is deprecated as it is superseded by the use of multiple "
			"TileMapLayer nodes.\nTo convert a TileMap to a set of TileMapLayer nodes, open the "
			"TileMap bottom panel with this node selected, click the toolbox icon in the top-right "
			"corner and choose \"Extract TileMap layers as individual TileMapLayer nodes\"."));

	// Retrieve the set of Z index values with a Y-sorted layer.
	RBSet<int> y_sorted_z_index;
	for (const TileMapLayer* layer : layers) {
		if (layer->is_y_sort_enabled()) {
			y_sorted_z_index.insert(layer->get_z_index());
		}
	}

	// Check if we have a non-sorted layer in a Z-index with a Y-sorted layer.
	for (const TileMapLayer* layer : layers) {
		if (!layer->is_y_sort_enabled() && y_sorted_z_index.has(layer->get_z_index())) {
			warnings.push_back(
				RTR("A Y-sorted layer has the same Z-index value as a not Y-sorted layer.\nThis "
					"may lead to unwanted behaviors, as a layer that is not Y-sorted will be "
					"Y-sorted as a whole with tiles from Y-sorted layers."));
			break;
		}
	}

	if (!is_y_sort_enabled()) {
		// Check if Y-sort is enabled on a layer but not on the node.
		for (const TileMapLayer* layer : layers) {
			if (layer->is_y_sort_enabled()) {
				warnings.push_back(RTR("A TileMap layer is set as Y-sorted, but Y-sort is not "
									   "enabled on the TileMap node itself."));
				break;
			}
		}
	}
	else {
		// Check if Y-sort is enabled on the node, but not on any of the layers.
		bool need_warning = true;
		for (const TileMapLayer* layer : layers) {
			if (layer->is_y_sort_enabled()) {
				need_warning = false;
				break;
			}
		}
		if (need_warning) {
			warnings.push_back(
				RTR("The TileMap node is set as Y-sorted, but Y-sort is not enabled on any of the "
					"TileMap's layers.\nThis may lead to unwanted behaviors, as a layer that is "
					"not Y-sorted will be Y-sorted as a whole."));
		}
	}

	// Check if we are in isometric mode without Y-sort enabled.
	if (tile_set.is_valid() && tile_set->get_tile_shape() == TileSet::TILE_SHAPE_ISOMETRIC) {
		bool warn = !is_y_sort_enabled();
		if (!warn) {
			for (const TileMapLayer* layer : layers) {
				if (!layer->is_y_sort_enabled()) {
					warn = true;
					break;
				}
			}
		}
		if (warn) {
			warnings.push_back(RTR("Isometric TileSet will likely not look as intended without "
								   "Y-sort enabled for the TileMap and all of its layers."));
		}
	}
	return warnings;
}

#undef TILEMAP_CALL_FOR_LAYER
#undef TILEMAP_CALL_FOR_LAYER_V



int TileMap::get_cell_alternative_tile(int, Vector2i const&, bool) const {}

void TileMap::_emit_changed() {}

void TileMap::_set_tile_map_data_using_compatibility_format(int, TileMapDataFormat, Vector<int> const&) {}


// --- Merged from scene/2d/physics/physics_body_2d.cpp ---
#include "scene/2d/node_2d.h"

#include "core/config/engine.h"
// --- Inlined from scene/2d/physics/joints/joint_2d.h ---
#include "scene/2d/node_2d.h"

class PhysicsBody2D;

class Joint2D : public Node2D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_GROOVE,
		JOINT_DAMPED_SPRING,
	};

private:
	JointType joint_type = JOINT_PIN;
	RID joint;
	RID ba, bb;

	NodePath a;
	NodePath b;
	real_t bias = 0.0f;
	bool exclude_from_collision = true;
	bool configured = false;
	String warning;

	// Pin parameters.
	real_t softness = 0.0f;
	real_t angular_limit_lower = 0.0f;
	real_t angular_limit_upper = 0.0f;
	real_t motor_target_velocity = 0.0f;
	bool motor_enabled = false;
	bool angular_limit_enabled = false;

	// Groove & Spring parameters.
	real_t length = 50.0f;
	real_t initial_offset = 25.0f;
	real_t rest_length = 0.0f;
	real_t stiffness = 20.0f;
	real_t damping = 1.0f;

	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b);

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint2D(JointType p_type = JOINT_PIN);
	~Joint2D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const;

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const;

	void set_bias(real_t p_bias);
	real_t get_bias() const;

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const;

	// Pin parameters.
	void set_softness(real_t p_softness);
	real_t get_softness() const { return softness; }
	void set_angular_limit_lower(real_t p_angular_limit_lower);
	real_t get_angular_limit_lower() const { return angular_limit_lower; }
	void set_angular_limit_upper(real_t p_angular_limit_upper);
	real_t get_angular_limit_upper() const { return angular_limit_upper; }
	void set_motor_target_velocity(real_t p_motor_target_velocity);
	real_t get_motor_target_velocity() const { return motor_target_velocity; }
	void set_motor_enabled(bool p_motor_enabled);
	bool is_motor_enabled() const { return motor_enabled; }
	void set_angular_limit_enabled(bool p_angular_limit_enabled);
	bool is_angular_limit_enabled() const { return angular_limit_enabled; }

	// Groove & DampedSpring parameters.
	void set_length(real_t p_length);
	real_t get_length() const { return length; }
	void set_initial_offset(real_t p_initial_offset);
	real_t get_initial_offset() const { return initial_offset; }
	void set_rest_length(real_t p_rest_length);
	real_t get_rest_length() const { return rest_length; }
	void set_stiffness(real_t p_stiffness);
	real_t get_stiffness() const { return stiffness; }
	void set_damping(real_t p_damping);
	real_t get_damping() const { return damping; }

	RID get_rid() const { return joint; }
};

// --- End scene/2d/physics/joints/joint_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

PhysicsBody2D::PhysicsBody2D(BodyMode p_mode) :
		mode(p_mode) {
	collision_state.init(false, PS2DE::BODY_MODE_STATIC);
	set_notify_transform(true);
	_apply_server_body_mode();
	if (mode == MODE_PHYSICAL_BONE) {
		PhysicsServer2D::body_set_collision_layer(get_rid(), 0);
		PhysicsServer2D::body_set_collision_mask(get_rid(), 0);
		PhysicsServer2D::body_set_mode(get_rid(), PS2DE::BODY_MODE_STATIC);
	}
}

PhysicsBody2D::~PhysicsBody2D() {
	collision_state.free_rid();
}

void PhysicsBody2D::set_body_mode(BodyMode p_mode) {
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	_apply_server_body_mode();
	update_configuration_warnings();
}

void PhysicsBody2D::_apply_server_body_mode() {
	switch (mode) {
		case MODE_STATIC: {
			collision_state.set_body_mode(this, PS2DE::BODY_MODE_STATIC);
		} break;
		case MODE_ANIMATABLE:
		case MODE_CHARACTER: {
			collision_state.set_body_mode(this, PS2DE::BODY_MODE_KINEMATIC);
		} break;
		case MODE_RIGID:
		case MODE_PHYSICAL_BONE: {
			if (mode == MODE_PHYSICAL_BONE && !bone_state.internal_simulate_physics) {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_STATIC);
				break;
			}
			if (freeze) {
				collision_state.set_body_mode(this, freeze_mode == FREEZE_MODE_STATIC ? PS2DE::BODY_MODE_STATIC : PS2DE::BODY_MODE_KINEMATIC);
			} else if (lock_rotation) {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_RIGID_LINEAR);
			} else {
				collision_state.set_body_mode(this, PS2DE::BODY_MODE_RIGID);
			}
		} break;
	}
}

void PhysicsBody2D::_reload_physics_characteristics() {}

void PhysicsBody2D::_body_state_changed_callback(void *p_instance, PhysicsDirectBodyState2D *p_state) {
	PhysicsBody2D *body = static_cast<PhysicsBody2D *>(p_instance);
	body->_body_state_changed(p_state);
}

void PhysicsBody2D::_body_state_changed(PhysicsDirectBodyState2D *p_state) {
	if (mode == MODE_ANIMATABLE) {
		if (!sync_to_physics) {
			return;
		}
		last_valid_transform = p_state->get_transform();
		set_notify_local_transform(false);
		set_global_transform(last_valid_transform);
		set_notify_local_transform(true);
		return;
	}

	linear_velocity = p_state->get_linear_velocity();
	angular_velocity = p_state->get_angular_velocity();
	sleeping = p_state->is_sleeping();
	contact_count = p_state->get_contact_count();

	Transform2D new_xform = p_state->get_transform();
	if (new_xform != get_global_transform()) {
		set_block_transform_notify(true);
		set_global_transform(new_xform);
		set_block_transform_notify(false);
	}
}

void PhysicsBody2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint() && (mode == MODE_RIGID || mode == MODE_PHYSICAL_BONE)) {
				set_notify_local_transform(true);
			}
#endif
		} break;
		case NOTIFICATION_READY: {
			if (mode == MODE_PHYSICAL_BONE) {
				if (bone_state.simulate_physics) {
					_start_bone_physics_simulation();
				} else {
					_stop_bone_physics_simulation();
				}
				set_physics_process_internal(true);
			}
		} break;
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (mode == MODE_PHYSICAL_BONE) {
				if (bone_state.follow_bone_when_simulating) {
					_position_at_bone2d();
				}
				if (bone_state.child_joint && bone_state.auto_configure_joint) {
					bone_state.child_joint->set_global_position(get_global_position());
				}
			}
		} break;
	}
}

// Common motion & exceptions.

bool PhysicsBody2D::move_and_collide(const PS2DT::MotionParameters &p_parameters, PS2DT::MotionResult &r_result, bool p_test_only, bool p_cancel_sliding) {
	bool colliding = PhysicsServer2D::body_test_motion(get_rid(), p_parameters, &r_result);

	if (p_cancel_sliding) {
		real_t motion_length = p_parameters.motion.length();
		real_t precision = 0.001f;
		if (colliding) {
			precision += motion_length * (r_result.collision_unsafe_fraction - r_result.collision_safe_fraction);
			if (r_result.collision_depth > p_parameters.margin + precision) {
				p_cancel_sliding = false;
			}
		}
		if (p_cancel_sliding) {
			Vector2 motion_normal;
			if (motion_length > CMP_EPSILON) {
				motion_normal = p_parameters.motion / motion_length;
			}
			real_t projected_length = r_result.travel.dot(motion_normal);
			Vector2 recovery = r_result.travel - motion_normal * projected_length;
			if (recovery.length() < p_parameters.margin + precision) {
				r_result.travel = motion_normal * projected_length;
				r_result.remainder = p_parameters.motion - r_result.travel;
			}
		}
	}

	if (!p_test_only) {
		Transform2D gt = get_global_transform();
		gt.columns[2] += r_result.travel;
		set_global_transform(gt);
	}
	return colliding;
}

bool PhysicsBody2D::test_move(const Transform2D &p_from, const Vector2 &p_motion, const Ref<KinematicCollision2D> &r_collision, real_t p_margin, bool p_recovery_as_collision) {
	ERR_FAIL_COND_V(!is_inside_tree(), false);
	PS2DT::MotionResult temp_result;
	PS2DT::MotionResult *r = r_collision.is_valid() ? &r_collision->result : &temp_result;
	PS2DT::MotionParameters parameters;
	parameters.motion = p_motion;
	parameters.margin = p_margin;
	parameters.recovery_as_collision = p_recovery_as_collision;
	return PhysicsServer2D::body_test_motion(get_rid(), parameters, r);
}

Vector2 PhysicsBody2D::get_gravity() const {
	PhysicsDirectBodyState2D *state = PhysicsServer2D::body_get_direct_state(get_rid());
	ERR_FAIL_NULL_V(state, Vector2());
	return state->get_total_gravity();
}

void PhysicsBody2D::add_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody2D *other = dynamic_cast<PhysicsBody2D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody2D.");
	PhysicsServer2D::body_add_collision_exception(get_rid(), other->get_rid());
}

void PhysicsBody2D::remove_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody2D *other = dynamic_cast<PhysicsBody2D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody2D.");
	PhysicsServer2D::body_remove_collision_exception(get_rid(), other->get_rid());
}

// Static / Animatable API.

void PhysicsBody2D::set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override) {
	physics_material_override = p_physics_material_override;
	_reload_physics_characteristics();
}

void PhysicsBody2D::set_constant_linear_velocity(const Vector2 &p_vel) {
	constant_linear_velocity = p_vel;
}

void PhysicsBody2D::set_constant_angular_velocity(real_t p_vel) {
	constant_angular_velocity = p_vel;
}

void PhysicsBody2D::set_sync_to_physics(bool p_enable) {
	sync_to_physics = p_enable;
	collision_state.only_update_transform_changes = p_enable && (mode == MODE_ANIMATABLE);
}

// RigidBody API.

void PhysicsBody2D::set_lock_rotation_enabled(bool p_lock_rotation) {
	if (lock_rotation == p_lock_rotation) {
		return;
	}
	lock_rotation = p_lock_rotation;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_freeze_enabled(bool p_freeze) {
	if (freeze == p_freeze) {
		return;
	}
	freeze = p_freeze;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_freeze_mode(FreezeMode p_freeze_mode) {
	if (freeze_mode == p_freeze_mode) {
		return;
	}
	freeze_mode = p_freeze_mode;
	_apply_server_body_mode();
}

void PhysicsBody2D::set_mass(real_t p_mass) {
	ERR_FAIL_COND(p_mass <= 0);
	mass = p_mass;
}

void PhysicsBody2D::set_inertia(real_t p_inertia) {
	ERR_FAIL_COND(p_inertia < 0);
	inertia = p_inertia;
}

void PhysicsBody2D::set_center_of_mass_mode(CenterOfMassMode p_mode) {
	if (center_of_mass_mode == p_mode) {
		return;
	}
	center_of_mass_mode = p_mode;
	if (center_of_mass_mode == CENTER_OF_MASS_MODE_AUTO) {
		PhysicsServer2D::body_reset_mass_properties(get_rid());
	}
}

void PhysicsBody2D::set_center_of_mass(const Vector2 &p_center_of_mass) {
	center_of_mass = p_center_of_mass;
}

void PhysicsBody2D::set_gravity_scale(real_t p_gravity_scale) {
	gravity_scale = p_gravity_scale;
}

void PhysicsBody2D::set_linear_damp_mode(DampMode p_mode) {
	linear_damp_mode = p_mode;
}

void PhysicsBody2D::set_angular_damp_mode(DampMode p_mode) {
	angular_damp_mode = p_mode;
}

void PhysicsBody2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

void PhysicsBody2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

void PhysicsBody2D::set_linear_velocity(const Vector2 &p_velocity) {
	linear_velocity = p_velocity;
}

void PhysicsBody2D::set_axis_velocity(const Vector2 &p_axis) {
	PhysicsServer2D::body_set_axis_velocity(get_rid(), p_axis);
}

void PhysicsBody2D::set_angular_velocity(real_t p_velocity) {
	angular_velocity = p_velocity;
}

void PhysicsBody2D::set_use_custom_integrator(bool p_enable) {
	if (custom_integrator == p_enable) {
		return;
	}
	custom_integrator = p_enable;
	PhysicsServer2D::body_set_omit_force_integration(get_rid(), p_enable);
}

void PhysicsBody2D::set_sleeping(bool p_sleeping) {
	sleeping = p_sleeping;
}

void PhysicsBody2D::set_can_sleep(bool p_active) {
	can_sleep = p_active;
}

void PhysicsBody2D::set_max_contacts_reported(int p_amount) {
	ERR_FAIL_INDEX_MSG(p_amount, PS2DC::MAX_CONTACTS_REPORTED_2D_MAX, "Max contacts reported is too high.");
	max_contacts_reported = p_amount;
	PhysicsServer2D::body_set_max_contacts_reported(get_rid(), p_amount);
}

void PhysicsBody2D::set_continuous_collision_detection_mode(CCDMode p_mode) {
	ccd_mode = p_mode;
	PhysicsServer2D::body_set_continuous_collision_detection_mode(get_rid(), PS2DE::CCDMode(p_mode));
}

void PhysicsBody2D::apply_central_impulse(const Vector2 &p_impulse) {
	PhysicsServer2D::body_apply_central_impulse(get_rid(), p_impulse);
}

void PhysicsBody2D::apply_impulse(const Vector2 &p_impulse, const Vector2 &p_position) {
	PhysicsServer2D::body_apply_impulse(get_rid(), p_impulse, p_position);
}

void PhysicsBody2D::apply_torque_impulse(real_t p_torque) {
	PhysicsServer2D::body_apply_torque_impulse(get_rid(), p_torque);
}

void PhysicsBody2D::apply_central_force(const Vector2 &p_force) {
	PhysicsServer2D::body_apply_central_force(get_rid(), p_force);
}

void PhysicsBody2D::apply_force(const Vector2 &p_force, const Vector2 &p_position) {
	PhysicsServer2D::body_apply_force(get_rid(), p_force, p_position);
}

void PhysicsBody2D::apply_torque(real_t p_torque) {
	PhysicsServer2D::body_apply_torque(get_rid(), p_torque);
}

void PhysicsBody2D::add_constant_central_force(const Vector2 &p_force) {
	PhysicsServer2D::body_add_constant_central_force(get_rid(), p_force);
}

void PhysicsBody2D::add_constant_force(const Vector2 &p_force, const Vector2 &p_position) {
	PhysicsServer2D::body_add_constant_force(get_rid(), p_force, p_position);
}

void PhysicsBody2D::add_constant_torque(real_t p_torque) {
	PhysicsServer2D::body_add_constant_torque(get_rid(), p_torque);
}

void PhysicsBody2D::set_constant_force(const Vector2 &p_force) {
	PhysicsServer2D::body_set_constant_force(get_rid(), p_force);
}

Vector2 PhysicsBody2D::get_constant_force() const {
	return PhysicsServer2D::body_get_constant_force(get_rid());
}

void PhysicsBody2D::set_constant_torque(real_t p_torque) {
	PhysicsServer2D::body_set_constant_torque(get_rid(), p_torque);
}

real_t PhysicsBody2D::get_constant_torque() const {
	return PhysicsServer2D::body_get_constant_torque(get_rid());
}

// CharacterMover2D API.

bool PhysicsBody2D::move_and_slide() {
	double delta = Engine::is_in_physics_frame() ? get_physics_process_delta_time() : get_process_delta_time();
	if (Math::is_zero_approx(delta)) {
		return false;
	}
	character.previous_position = get_global_transform().columns[2];
	character.motion_results.clear();
	character.on_floor = false;
	character.on_wall = false;
	character.on_ceiling = false;
	character.last_motion = Vector2();

	Vector2 motion = character.velocity * delta;
	for (int slide = 0; slide < character.max_slides && !motion.is_zero_approx(); slide++) {
		PS2DT::MotionParameters params;
		params.motion = motion;
		params.margin = character.margin;
		PS2DT::MotionResult res;
		if (!move_and_collide(params, res, false, false)) {
			character.last_motion = motion;
			break;
		}
		character.motion_results.push_back(res);
		character.last_motion = res.travel;
		if (character.motion_mode == MOTION_MODE_GROUNDED && !character.up_direction.is_zero_approx()) {
			real_t angle = Math::acos(CLAMP(res.collision_normal.dot(character.up_direction), -1.0f, 1.0f));
			if (angle <= character.floor_max_angle + 0.01f) {
				character.on_floor = true;
				character.floor_normal = res.collision_normal;
			} else if (Math::acos(CLAMP(res.collision_normal.dot(-character.up_direction), -1.0f, 1.0f)) <= character.floor_max_angle + 0.01f) {
				character.on_ceiling = true;
			} else {
				character.on_wall = true;
				character.wall_normal = res.collision_normal;
			}
		}
		motion = res.remainder.slide(res.collision_normal);
		character.velocity = character.velocity.slide(res.collision_normal);
	}
	character.real_velocity = (get_global_transform().columns[2] - character.previous_position) / delta;
	return !character.motion_results.is_empty();
}

void PhysicsBody2D::apply_floor_snap() {
	if (character.on_floor || character.up_direction.is_zero_approx()) {
		return;
	}
	real_t len = MAX(character.floor_snap_length, character.margin);
	PS2DT::MotionParameters params;
	params.motion = -character.up_direction * len;
	params.margin = character.margin;
	params.recovery_as_collision = true;
	PS2DT::MotionResult res;
	if (move_and_collide(params, res, true, false)) {
		real_t angle = Math::acos(CLAMP(res.collision_normal.dot(character.up_direction), -1.0f, 1.0f));
		if (angle <= character.floor_max_angle + 0.01f) {
			character.on_floor = true;
			character.floor_normal = res.collision_normal;
			Transform2D gt = get_global_transform();
			gt.columns[2] += character.up_direction * character.up_direction.dot(res.travel);
			set_global_transform(gt);
		}
	}
}

void PhysicsBody2D::set_up_direction(const Vector2 &p_up_direction) {
	ERR_FAIL_COND_MSG(p_up_direction == Vector2(), "up_direction can't be equal to Vector2.ZERO.");
	character.up_direction = p_up_direction.normalized();
}

real_t PhysicsBody2D::get_floor_angle(const Vector2 &p_up_direction) const {
	ERR_FAIL_COND_V(p_up_direction == Vector2(), 0);
	return Math::acos(character.floor_normal.dot(p_up_direction));
}

PS2DT::MotionResult PhysicsBody2D::get_slide_collision(int p_bounce) const {
	ERR_FAIL_INDEX_V(p_bounce, character.motion_results.size(), PS2DT::MotionResult());
	return character.motion_results[p_bounce];
}

void PhysicsBody2D::set_max_slides(int p_max_slides) {
	ERR_FAIL_COND(p_max_slides < 1);
	character.max_slides = p_max_slides;
}

void PhysicsBody2D::set_floor_snap_length(real_t p_len) {
	ERR_FAIL_COND(p_len < 0);
	character.floor_snap_length = p_len;
}

// PhysicalBoneState2D API.

void PhysicsBody2D::_position_at_bone2d() {
	if (bone_state.parent_skeleton) {
		Bone2D *bone_to_use = bone_state.parent_skeleton->get_bone(bone_state.bone2d_index);
		ERR_FAIL_NULL(bone_to_use);
		set_global_transform(bone_to_use->get_global_transform());
	}
}

void PhysicsBody2D::_start_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		return;
	}
	_position_at_bone2d();
	PhysicsServer2D::body_set_collision_layer(get_rid(), get_collision_layer());
	PhysicsServer2D::body_set_collision_mask(get_rid(), get_collision_mask());
	PhysicsServer2D::body_set_collision_priority(get_rid(), get_collision_priority());
	bone_state.internal_simulate_physics = true;
	_apply_server_body_mode();
	set_physics_process_internal(true);
}

void PhysicsBody2D::_stop_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		bone_state.internal_simulate_physics = false;
		_position_at_bone2d();
		set_physics_process_internal(false);
		PhysicsServer2D::body_set_collision_layer(get_rid(), 0);
		PhysicsServer2D::body_set_collision_mask(get_rid(), 0);
		PhysicsServer2D::body_set_collision_priority(get_rid(), 1.0);
		PhysicsServer2D::body_set_mode(get_rid(), PS2DE::BODY_MODE_STATIC);
	}
}

void PhysicsBody2D::set_auto_configure_joint(bool p_auto_configure) {
	bone_state.auto_configure_joint = p_auto_configure;
}

void PhysicsBody2D::set_simulate_physics(bool p_simulate) {
	if (bone_state.simulate_physics == p_simulate) {
		return;
	}
	bone_state.simulate_physics = p_simulate;
	if (bone_state.simulate_physics) {
		_start_bone_physics_simulation();
	} else {
		_stop_bone_physics_simulation();
	}
}

void PhysicsBody2D::set_follow_bone_when_simulating(bool p_follow) {
	bone_state.follow_bone_when_simulating = p_follow;
	if (bone_state.internal_simulate_physics) {
		_stop_bone_physics_simulation();
		_start_bone_physics_simulation();
	}
}

PackedStringArray PhysicsBody2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	if (SceneTree::is_fti_enabled_in_project() && !is_physics_interpolated()) {
		warnings.push_back(RTR("PhysicsBody2D will not work correctly on a non-interpolated branch of the SceneTree."));
	}
	if (mode == MODE_RIGID) {
		Transform2D t = get_transform();
		if (Math::abs(t.columns[0].length() - 1.0) > 0.05 || Math::abs(t.columns[1].length() - 1.0) > 0.05) {
			warnings.push_back(RTR("Size changes to a rigid PhysicsBody2D will be overridden by the physics engine when running."));
		}
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/physics/kinematic_collision_2d.cpp
// ==========================================================================

#include "scene/2d/node_2d.h"

Vector2 KinematicCollision2D::get_position() const { return result.collision_point; }

Vector2 KinematicCollision2D::get_normal() const { return result.collision_normal; }

Vector2 KinematicCollision2D::get_travel() const { return result.travel; }

Vector2 KinematicCollision2D::get_remainder() const { return result.remainder; }

real_t KinematicCollision2D::get_angle(const Vector2& p_up_direction) const
{
	ERR_FAIL_COND_V(p_up_direction == Vector2(), 0);
	return result.get_angle(p_up_direction);
}

real_t KinematicCollision2D::get_depth() const { return result.collision_depth; }

RID KinematicCollision2D::get_collider_rid() const { return result.collider; }

int KinematicCollision2D::get_collider_shape_index() const { return result.collider_shape; }

Vector2 KinematicCollision2D::get_collider_velocity() const { return result.collider_velocity; }

// ============================================================================
// Merged from scene/2d/physics/collision_object_2d.cpp
// ============================================================================

#include "scene/2d/node_2d.h"

#include "scene/main/viewport.h"
#include "servers/physics_2d/physics_server_2d.h"

void CollisionState2D::init(bool p_area, PS2DE::BodyMode p_mode) {
	area = p_area;
	body_mode = p_mode;
	if (area) {
		rid = PhysicsServer2D::area_create();
	} else {
		rid = PhysicsServer2D::body_create();
		PhysicsServer2D::body_set_mode(rid, body_mode);
	}
}

void CollisionState2D::free_rid() {
	if (rid.is_valid() && PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::free_rid(rid);
	}
	rid = RID();
}

void CollisionState2D::handle_notification(Node2D *p_owner, int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			if (area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
			if (p_owner->is_enabled()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
			update_pickable(p_owner);
		} break;
		case CanvasItem::NOTIFICATION_TRANSFORM_CHANGED: {
			if (!only_update_transform_changes && area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
		} break;
		case CanvasItem::NOTIFICATION_VISIBILITY_CHANGED: {
			update_pickable(p_owner);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			if (callback_lock == 0) {
				if (area) {
					PhysicsServer2D::area_set_space(rid, RID());
				} else {
					PhysicsServer2D::body_set_space(rid, RID());
				}
			}
		} break;
		case Node::NOTIFICATION_DISABLED: {
			apply_disabled(p_owner);
		} break;
		case Node::NOTIFICATION_ENABLED: {
			apply_enabled(p_owner);
		} break;
	}
}

void CollisionState2D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	if (area) {
		PhysicsServer2D::area_set_collision_layer(rid, p_layer);
	} else {
		PhysicsServer2D::body_set_collision_layer(rid, p_layer);
	}
}

void CollisionState2D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	if (area) {
		PhysicsServer2D::area_set_collision_mask(rid, p_mask);
	} else {
		PhysicsServer2D::body_set_collision_mask(rid, p_mask);
	}
}

void CollisionState2D::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer_new = collision_layer;
	if (p_value) {
		layer_new |= 1 << (p_layer_number - 1);
	} else {
		layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer_new);
}

bool CollisionState2D::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_layer & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = collision_mask;
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CollisionState2D::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_mask & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer2D::body_set_collision_priority(rid, p_priority);
	}
}

void CollisionState2D::set_disable_mode(Node2D *p_owner, DisableMode p_mode) {
	if (disable_mode == p_mode) {
		return;
	}
	bool disabled = p_owner->is_inside_tree() && !p_owner->is_enabled();
	if (disabled) {
		apply_enabled(p_owner);
	}
	disable_mode = p_mode;
	if (disabled) {
		apply_disabled(p_owner);
	}
}

void CollisionState2D::apply_disabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				if (callback_lock > 0) {
					ERR_PRINT("Disabling a collision node during a physics callback is not allowed.");
				} else {
					if (area) {
						PhysicsServer2D::area_set_space(rid, RID());
					} else {
						PhysicsServer2D::body_set_space(rid, RID());
					}
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, PS2DE::BODY_MODE_STATIC);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::apply_enabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, body_mode);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode) {
	ERR_FAIL_COND(area);
	if (body_mode == p_mode) {
		return;
	}
	body_mode = p_mode;
	if (p_owner->is_inside_tree() && !p_owner->is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}
	PhysicsServer2D::body_set_mode(rid, p_mode);
}

void CollisionState2D::set_pickable(const Node2D *p_owner, bool p_enabled) {
	if (pickable == p_enabled) {
		return;
	}
	pickable = p_enabled;
	update_pickable(p_owner);
}

void CollisionState2D::update_pickable(const Node2D *p_owner) {
	if (!p_owner->is_inside_tree()) {
		return;
	}
	bool is_pick = pickable && p_owner->is_visible_in_tree();
	if (area) {
		PhysicsServer2D::area_set_pickable(rid, is_pick);
	} else {
		PhysicsServer2D::body_set_pickable(rid, is_pick);
	}
}

// Inline shape slots.

int CollisionState2D::add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform, bool p_disabled) {
	ShapeSlot slot;
	slot.local_xform = p_xform;
	slot.shape = p_shape;
	slot.disabled = p_disabled;
	slot.owner_id = create_shape_owner();
	shape_owner_set_transform(slot.owner_id, p_xform);
	shape_owner_set_disabled(slot.owner_id, p_disabled);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
	int idx = (int)inline_shapes.size();
	inline_shapes.push_back(slot);
	return idx;
}

void CollisionState2D::set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	ShapeSlot &slot = inline_shapes[p_index];
	slot.shape = p_shape;
	shape_owner_clear_shapes(slot.owner_id);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
}

Ref<Shape2D> CollisionState2D::get_inline_shape(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Ref<Shape2D>());
	return inline_shapes[p_index].shape;
}

void CollisionState2D::set_inline_shape_transform(int p_index, const Transform2D &p_xform) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].local_xform = p_xform;
	shape_owner_set_transform(inline_shapes[p_index].owner_id, p_xform);
}

Transform2D CollisionState2D::get_inline_shape_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Transform2D());
	return inline_shapes[p_index].local_xform;
}

void CollisionState2D::set_inline_shape_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].disabled = p_disabled;
	shape_owner_set_disabled(inline_shapes[p_index].owner_id, p_disabled);
}

bool CollisionState2D::is_inline_shape_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), false);
	return inline_shapes[p_index].disabled;
}

void CollisionState2D::remove_inline_shape(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	remove_shape_owner(inline_shapes[p_index].owner_id);
	inline_shapes.remove_at(p_index);
}

void CollisionState2D::clear_inline_shapes() {
	for (uint32_t i = 0; i < inline_shapes.size(); i++) {
		remove_shape_owner(inline_shapes[i].owner_id);
	}
	inline_shapes.clear();
}

// Shape owners.

uint32_t CollisionState2D::create_shape_owner(Node *p_owner) {
	uint32_t id = next_owner_id++;
	ShapeData sd;
	sd.owner_node = p_owner;
	shapes[id] = sd;
	return id;
}

void CollisionState2D::remove_shape_owner(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	shape_owner_clear_shapes(p_owner);
	shapes.erase(p_owner);
}

void CollisionState2D::get_shape_owners(List<uint32_t> *r_owners) const {
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionState2D::get_shape_owners_array() const {
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		ret.push_back(E.key);
	}
	return ret;
}

void CollisionState2D::shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		} else {
			PhysicsServer2D::body_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		}
	}
}

Transform2D CollisionState2D::shape_owner_get_transform(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform2D());
	return shapes[p_owner].xform;
}

void CollisionState2D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.disabled = p_disabled;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		} else {
			PhysicsServer2D::body_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		}
	}
}

bool CollisionState2D::is_shape_owner_disabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].disabled;
}

void CollisionState2D::shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision = p_enable;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

bool CollisionState2D::is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].one_way_collision;
}

void CollisionState2D::shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_margin = p_margin;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

real_t CollisionState2D::get_shape_owner_one_way_collision_margin(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].one_way_collision_margin;
}

void CollisionState2D::shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_direction = p_direction.normalized();
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

Vector2 CollisionState2D::get_shape_owner_one_way_collision_direction(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Vector2());
	return shapes[p_owner].one_way_collision_direction;
}

void CollisionState2D::shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_NULL(rp_shape);
	ShapeData &sd = shapes[p_owner];
	ShapeData::Shape s;
	s.index = total_subshapes;
	s.shape = rp_shape;
	if (area) {
		PhysicsServer2D::area_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
	} else {
		PhysicsServer2D::body_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
		if (sd.one_way_collision) {
			PhysicsServer2D::body_set_shape_as_one_way_collision(rid, s.index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
		}
	}
	sd.shapes.push_back(s);
	total_subshapes++;
}

int CollisionState2D::shape_owner_get_shape_count(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].shapes.size();
}

Ref<Shape2D> CollisionState2D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape2D>());
	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionState2D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);
	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionState2D::shape_owner_remove_shape(uint32_t p_owner, int p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_INDEX(p_shape, shapes[p_owner].shapes.size());

	int index_to_remove = shapes[p_owner].shapes[p_shape].index;
	if (area) {
		PhysicsServer2D::area_remove_shape(rid, index_to_remove);
	} else {
		PhysicsServer2D::body_remove_shape(rid, index_to_remove);
	}

	shapes[p_owner].shapes.remove_at(p_shape);

	for (KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index > index_to_remove) {
				E.value.shapes.write[i].index -= 1;
			}
		}
	}
	total_subshapes--;
}

void CollisionState2D::shape_owner_clear_shapes(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}
}

uint32_t CollisionState2D::shape_find_owner(int p_shape_index) const {
	ERR_FAIL_INDEX_V(p_shape_index, total_subshapes, UINT32_MAX);
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index == p_shape_index) {
				return E.key;
			}
		}
	}
	ERR_FAIL_V_MSG(UINT32_MAX, "Can't find owner for shape index " + itos(p_shape_index) + ".");
}

// ==========================================================================
// Merged from scene/2d/physics/area_2d.cpp
// ==========================================================================

#include "scene/2d/node_2d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

Area2D::Area2D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area2D::~Area2D() {
	collision_state.free_rid();
}

void Area2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	if (p_what == NOTIFICATION_EXIT_TREE || p_what == NOTIFICATION_DISABLED) {
		_clear_monitoring();
	}
}

void Area2D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area2D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area2D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area2D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area2D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area2D::set_gravity_point_center(const Vector2 &p_center) {
	gravity_vec = p_center;
}

const Vector2 &Area2D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area2D::set_gravity_direction(const Vector2 &p_direction) {
	gravity_vec = p_direction;
}

const Vector2 &Area2D::get_gravity_direction() const {
	return gravity_vec;
}

void Area2D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area2D::get_gravity() const {
	return gravity;
}

void Area2D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area2D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area2D::get_linear_damp() const {
	return linear_damp;
}

void Area2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area2D::get_angular_damp() const {
	return angular_damp;
}

void Area2D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area2D::get_priority() const {
	return priority;
}

void Area2D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const {
	return monitoring;
}

void Area2D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer2D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer2D::area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const {
	return monitorable;
}

bool Area2D::has_overlapping_bodies() const {
	return false;
}

bool Area2D::has_overlapping_areas() const {
	return false;
}

bool Area2D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area2D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area2D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area2D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area2D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area2D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area2D::_clear_monitoring() {}

PackedStringArray Area2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/physics/collision_shape_2d.cpp
// ==========================================================================

#include "scene/2d/node_2d.h"

#include "core/config/engine.h"
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"

void CollisionShape2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionShape2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape.ptr());
				}
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

void CollisionShape2D::_shape_changed() {
	queue_redraw();
}

void CollisionShape2D::set_shape(const Ref<Shape2D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape.ptr());
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

Ref<Shape2D> CollisionShape2D::get_shape() const {
	return shape;
}

bool CollisionShape2D::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	if (shape.is_null()) {
		return false;
	}
	return shape->_edit_is_selected_on_click(p_point, p_tolerance);
}

void CollisionShape2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape2D::is_disabled() const {
	return disabled;
}

void CollisionShape2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionShape2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionShape2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionShape2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionShape2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionShape2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

Color CollisionShape2D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape2D::set_debug_color(const Color &p_color) {
	debug_color = p_color;
	queue_redraw();
}

Color CollisionShape2D::get_debug_color() const {
	return debug_color;
}

#ifdef DEBUG_ENABLED
bool CollisionShape2D::_property_can_revert(const String &p_name) const {
	return p_name == "debug_color";
}
#endif // DEBUG_ENABLED

CollisionShape2D::CollisionShape2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionShape2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/collision_polygon_2d.cpp
// ==========================================================================
// --- Inlined from scene/2d/physics/collision_polygon_2d.h ---
#include "scene/2d/node_2d.h"

class CollisionPolygon2D : public Node2D {
public:
	enum BuildMode {
		BUILD_SOLIDS,
		BUILD_SEGMENTS,
	};

protected:
	Rect2 aabb = Rect2(-10, -10, 20, 20);
	BuildMode build_mode = BUILD_SOLIDS;
	Vector<Point2> polygon;
	uint32_t owner_id = 0;
	CollisionState2D *collision_state = nullptr;
	bool disabled = false;
	bool one_way_collision = false;
	real_t one_way_collision_margin = 1.0f;
	Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);

	Vector<Vector<Vector2>> _decompose_in_convex();
	void _build_polygon();
	void _update_in_shape_owner(bool p_xform_only = false);
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const override;
#endif

	void set_build_mode(BuildMode p_mode);
	BuildMode get_build_mode() const;

	void set_polygon(const Vector<Point2> &p_polygon);
	Vector<Point2> get_polygon() const;

	PackedStringArray get_configuration_warnings() const override;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision_enabled() const;

	void set_one_way_collision_margin(real_t p_margin);
	real_t get_one_way_collision_margin() const;

	void set_one_way_collision_direction(const Vector2 &p_direction);
	Vector2 get_one_way_collision_direction() const;

	CollisionPolygon2D();
};

// --- End scene/2d/physics/collision_polygon_2d.h ---

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/2d/node_2d.h"

void CollisionPolygon2D::_build_polygon() {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_clear_shapes(owner_id);

	if (build_mode == BUILD_SOLIDS) {
		if (polygon.size() < 3) {
			return;
		}
		Vector<Vector<Vector2>> decomp = _decompose_in_convex();
		for (int i = 0; i < decomp.size(); i++) {
			Ref<Shape2D> convex = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			convex->set_points(decomp[i]);
			collision_state->shape_owner_add_shape(owner_id, convex.ptr());
		}
	} else {
		if (polygon.size() < 2) {
			return;
		}
		Ref<Shape2D> concave = memnew(Shape2D(Shape2D::SHAPE_CONCAVE_POLYGON));
		Vector<Vector2> segments;
		segments.resize(polygon.size() * 2);
		Vector2 *w = segments.ptrw();
		for (int i = 0; i < polygon.size(); i++) {
			w[(i << 1) + 0] = polygon[i];
			w[(i << 1) + 1] = polygon[(i + 1) % polygon.size()];
		}
		concave->set_segments(segments);
		collision_state->shape_owner_add_shape(owner_id, concave.ptr());
	}
}

Vector<Vector<Vector2>> CollisionPolygon2D::_decompose_in_convex() {
	return Geometry2D::decompose_polygon_in_convex(polygon);
}

void CollisionPolygon2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionPolygon2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				_build_polygon();
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

void CollisionPolygon2D::set_polygon(const Vector<Point2> &p_polygon) {
	polygon = p_polygon;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

Vector<Point2> CollisionPolygon2D::get_polygon() const {
	return polygon;
}

void CollisionPolygon2D::set_build_mode(BuildMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 2);
	build_mode = p_mode;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

CollisionPolygon2D::BuildMode CollisionPolygon2D::get_build_mode() const {
	return build_mode;
}

#ifdef DEBUG_ENABLED
Rect2 CollisionPolygon2D::_edit_get_rect() const {
	return aabb;
}

bool CollisionPolygon2D::_edit_use_rect() const {
	return true;
}

bool CollisionPolygon2D::_edit_is_selected_on_click(const Vector2 &p_point, double p_tolerance) const {
	return false;
}
#endif

void CollisionPolygon2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon2D::is_disabled() const {
	return disabled;
}

void CollisionPolygon2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionPolygon2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionPolygon2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionPolygon2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionPolygon2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionPolygon2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

CollisionPolygon2D::CollisionPolygon2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
}

PackedStringArray CollisionPolygon2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/ray_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/2d/physics/ray_cast_2d.h ---
#include "scene/2d/node_2d.h"

class RayCast2D : public Node2D
{
	bool enabled = true;
	bool collided = false;
	RID against_rid;
	int against_shape = 0;
	Vector2 collision_point;
	Vector2 collision_normal;
	HashSet<RID> exclude;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;

	Vector2 target_position = Vector2(0, 50);

	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	bool hit_from_inside = false;

	void _draw_debug_shape();

protected:
	void _notification(int p_what);
	void _update_raycast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	void set_hit_from_inside(bool p_enable);
	bool is_hit_from_inside_enabled() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_target_position(const Vector2& p_point);
	Vector2 get_target_position() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	void force_raycast_update();

	bool is_colliding() const;
	RID get_collider_rid() const;
	int get_collider_shape() const;
	Vector2 get_collision_point() const;
	Vector2 get_collision_normal() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node2D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node2D* rp_node);
	void clear_exceptions();

	RayCast2D();
};



// --- End scene/2d/physics/ray_cast_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/physics_2d/physics_server_2d.h"

Vector2 RayCast2D::get_target_position() const { return target_position; }

void RayCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast2D::get_collision_mask() const { return collision_mask; }

void RayCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool RayCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast2D::is_colliding() const { return collided; }

RID RayCast2D::get_collider_rid() const { return against_rid; }

int RayCast2D::get_collider_shape() const { return against_shape; }

Vector2 RayCast2D::get_collision_point() const { return collision_point; }

Vector2 RayCast2D::get_collision_normal() const { return collision_normal; }

bool RayCast2D::is_enabled() const { return enabled; }

bool RayCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast2D::_draw_debug_shape()
{
	Color draw_col = collided ? Color(1.0, 0.01, 0) : SceneTree::get_debug_collisions_color();
	if (!enabled) {
		const float g = draw_col.get_v();
		draw_col.r = g;
		draw_col.g = g;
		draw_col.b = g;
	}

	// Draw an arrow indicating where the RayCast is pointing to
	const real_t max_arrow_size = 6;
	const real_t line_width = 1.4;
	bool no_line = target_position.length() < line_width;
	real_t arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

	if (no_line) {
		arrow_size = target_position.length();
	}
	else {
		draw_line(Vector2(), target_position - target_position.normalized() * arrow_size, draw_col,
			line_width);
	}

	Transform2D xf;
	xf.rotate(target_position.angle());
	xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

	Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)), xf.xform(Vector2(0, 0.5 * arrow_size)),
		xf.xform(Vector2(0, -0.5 * arrow_size))};

	Vector<Color> cols = {draw_col, draw_col, draw_col};

	draw_primitive(pts, cols, Vector<Vector2>());
}

void RayCast2D::force_raycast_update() { _update_raycast_state(); }

void RayCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void RayCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void RayCast2D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast2D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast2D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast2D::is_hit_from_inside_enabled() const { return hit_from_inside; }

RayCast2D::RayCast2D() { set_hide_clip_children(true); }



void RayCast2D::_update_raycast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/shape_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/physics_2d/physics_server_2d.h"
// --- Inlined from scene/2d/physics/shape_cast_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/2d/node_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

class ShapeCast2D : public Node2D
{
	bool enabled = true;

	Ref<Shape2D> shape;
	RID shape_rid;
	Vector2 target_position = Vector2(0, 50);

	HashSet<RID> exclude;
	real_t margin = 0.0;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;
	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	// Result
	int max_results = 32;
	Vector<PS2DT::ShapeRestInfo> result;
	bool collided = false;
	real_t collision_safe_fraction = 1.0;
	real_t collision_unsafe_fraction = 1.0;

	void _shape_changed();

protected:
	void _notification(int p_what);
	void _update_shapecast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_shape(const Ref<Shape2D>& p_shape);
	Ref<Shape2D> get_shape() const;

	void set_target_position(const Vector2& p_point);
	Vector2 get_target_position() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;

	void set_max_results(int p_max_results);
	int get_max_results() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	void force_shapecast_update();
	bool is_colliding() const;

	int get_collision_count() const;
	RID get_collider_rid(int p_idx) const;
	int get_collider_shape(int p_idx) const;
	Vector2 get_collision_point(int p_idx) const;
	Vector2 get_collision_normal(int p_idx) const;

	real_t get_closest_collision_safe_fraction() const;
	real_t get_closest_collision_unsafe_fraction() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node2D* rp_node);
	void remove_exception(const Node2D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void clear_exceptions();

	PackedStringArray get_configuration_warnings() const override;

	ShapeCast2D();
};



// --- End scene/2d/physics/shape_cast_2d.h ---

Vector2 ShapeCast2D::get_target_position() const { return target_position; }

void ShapeCast2D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast2D::get_margin() const { return margin; }

void ShapeCast2D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast2D::get_max_results() const { return max_results; }

void ShapeCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast2D::get_collision_mask() const { return collision_mask; }

void ShapeCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool ShapeCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast2D::get_collision_count() const { return result.size(); }

bool ShapeCast2D::is_colliding() const { return collided; }

RID ShapeCast2D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast2D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector2 ShapeCast2D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision point found.");
	return result[p_idx].point;
}

Vector2 ShapeCast2D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast2D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast2D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

bool ShapeCast2D::is_enabled() const { return enabled; }

Ref<Shape2D> ShapeCast2D::get_shape() const { return shape; }

bool ShapeCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast2D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void ShapeCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void ShapeCast2D::clear_exceptions() { exclude.clear(); }

void ShapeCast2D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast2D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

PackedStringArray ShapeCast2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (shape.is_null()) {
		warnings.push_back(
			RTR("This node cannot interact with other objects unless a Shape2D is assigned."));
	}
	return warnings;
}

ShapeCast2D::ShapeCast2D() { set_hide_clip_children(true); }



void ShapeCast2D::_update_shapecast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/touch_screen_button.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
// --- Inlined from scene/2d/physics/touch_screen_button.h ---
#include "scene/2d/node_2d.h"
#include "scene/2d/node_2d.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/texture.h"

class TouchScreenButton : public Node2D
{
public:
	enum VisibilityMode
	{
		VISIBILITY_ALWAYS,
		VISIBILITY_TOUCHSCREEN_ONLY
	};

private:
	Ref<Texture2D> texture_normal;
	Ref<Texture2D> texture_pressed;
	Ref<BitMap> bitmask;
	Ref<Shape2D> shape;
	bool shape_centered = true;
	bool shape_visible = true;

	Ref<Shape2D> unit_rect;

	String action;
	bool passby_press = false;
	int finger_pressed = -1;

	VisibilityMode visibility = VISIBILITY_ALWAYS;

	virtual void input(const InputEvent& p_event) override;

	bool _is_point_inside(const Point2& p_point);

	void _press(int p_finger_pressed);
	void _release(bool p_exiting_tree = false);

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_texture_normal(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture_normal() const;

	void set_texture_pressed(const Ref<Texture2D>& p_texture_pressed);
	Ref<Texture2D> get_texture_pressed() const;

	void set_bitmask(const Ref<BitMap>& p_bitmask);
	Ref<BitMap> get_bitmask() const;

	void set_shape(const Ref<Shape2D>& p_shape);
	Ref<Shape2D> get_shape() const;

	void set_shape_centered(bool p_shape_centered);
	bool is_shape_centered() const;

	void set_shape_visible(bool p_shape_visible);
	bool is_shape_visible() const;

	void set_action(const String& p_action);
	String get_action() const;

	void set_passby_press(bool p_enable);
	bool is_passby_press_enabled() const;

	void set_visibility_mode(VisibilityMode p_mode);
	VisibilityMode get_visibility_mode() const;

	bool is_pressed() const;

	virtual Rect2 get_anchorable_rect() const override;

	TouchScreenButton();
};



// --- End scene/2d/physics/touch_screen_button.h ---

Ref<Texture2D> TouchScreenButton::get_texture_normal() const { return texture_normal; }

Ref<Texture2D> TouchScreenButton::get_texture_pressed() const { return texture_pressed; }

void TouchScreenButton::set_bitmask(const Ref<BitMap>& p_bitmask) { bitmask = p_bitmask; }

Ref<BitMap> TouchScreenButton::get_bitmask() const { return bitmask; }

Ref<Shape2D> TouchScreenButton::get_shape() const { return shape; }

bool TouchScreenButton::is_shape_visible() const { return shape_visible; }

bool TouchScreenButton::is_shape_centered() const { return shape_centered; }

bool TouchScreenButton::is_pressed() const { return finger_pressed != -1; }

void TouchScreenButton::set_action(const String& p_action) { action = p_action; }

String TouchScreenButton::get_action() const { return action; }

bool TouchScreenButton::_is_point_inside(const Point2& p_point)
{
	Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(p_point);

	bool touched = false;
	bool check_rect = true;

	if (shape.is_valid()) {
		check_rect = false;

		Vector2 pos;
		if (shape_centered && texture_normal.is_valid()) {
			pos = texture_normal->get_size() * 0.5;
		}

		touched = shape->collide(Transform2D().translated_local(pos), unit_rect.ptr(),
			Transform2D(0, coord + Vector2(0.5, 0.5)));
	}

	if (bitmask.is_valid()) {
		check_rect = false;
		if (!touched && Rect2(Point2(), bitmask->get_size()).has_point(coord)) {
			if (bitmask->get_bitv(coord)) {
				touched = true;
			}
		}
	}

	if (!touched && check_rect) {
		if (texture_normal.is_valid()) {
			touched = Rect2(Size2(), texture_normal->get_size()).has_point(coord);
		}
	}

	return touched;
}

#ifdef DEBUG_ENABLED
Rect2 TouchScreenButton::_edit_get_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::_edit_get_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

bool TouchScreenButton::_edit_use_rect() const { return texture_normal.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 TouchScreenButton::get_anchorable_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::get_anchorable_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

TouchScreenButton::VisibilityMode TouchScreenButton::get_visibility_mode() const
{
	return visibility;
}

void TouchScreenButton::set_passby_press(bool p_enable) { passby_press = p_enable; }

bool TouchScreenButton::is_passby_press_enabled() const { return passby_press; }

TouchScreenButton::TouchScreenButton()
{
	unit_rect = memnew(Shape2D(Shape2D::SHAPE_RECTANGLE));
	unit_rect->set_size(Vector2(1, 1));
}

void TouchScreenButton::input(const InputEvent& p_event) {}

// ============================================================================
// Merged from scene/2d/physics/joints/joint_2d.cpp
// ============================================================================


#include "core/config/engine.h"
#include "scene/2d/node_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"

Joint2D::Joint2D(JointType p_type) :
		joint_type(p_type) {
	joint = PhysicsServer2D::joint_create();
	set_hide_clip_children(true);
}

Joint2D::~Joint2D() {
	ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
	PhysicsServer2D::free_rid(joint);
}

void Joint2D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	joint_type = p_type;
	_update_joint();
	queue_redraw();
}

void Joint2D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

NodePath Joint2D::get_node_a() const {
	return a;
}

void Joint2D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

NodePath Joint2D::get_node_b() const {
	return b;
}

void Joint2D::_disconnect_signals() {}

void Joint2D::_update_joint(bool p_only_free) {
	warning = String();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_remove_collision_exception(ba, bb);
	}
	ba = RID();
	bb = RID();
	configured = false;

	if (p_only_free || !is_inside_tree()) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody2D *body_a = dynamic_cast<PhysicsBody2D *>(node_a);
	PhysicsBody2D *body_b = dynamic_cast<PhysicsBody2D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a || (joint_type != JOINT_PIN && !body_b)) {
		PhysicsServer2D::joint_clear(joint);
		update_configuration_warnings();
		return;
	}

	_configure_joint(joint, body_a, body_b);
	PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();
	if (ba.is_valid() && bb.is_valid() && exclude_from_collision) {
		PhysicsServer2D::body_add_collision_exception(ba, bb);
	}
	configured = true;
	update_configuration_warnings();
}

void Joint2D::_configure_joint(RID p_joint, PhysicsBody2D *body_a, PhysicsBody2D *body_b) {
	switch (joint_type) {
		case JOINT_PIN: {
			PhysicsServer2D::joint_make_pin(p_joint, get_global_position(), body_a->get_rid(), body_b ? body_b->get_rid() : RID());
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_SOFTNESS, softness);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_UPPER, angular_limit_upper);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_LIMIT_LOWER, angular_limit_lower);
			PhysicsServer2D::pin_joint_set_param(p_joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, motor_target_velocity);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, motor_enabled);
			PhysicsServer2D::pin_joint_set_flag(p_joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, angular_limit_enabled);
		} break;
		case JOINT_GROOVE: {
			Transform2D gt = get_global_transform();
			Vector2 groove_A1 = gt.get_origin();
			Vector2 groove_A2 = gt.xform(Vector2(0, length));
			Vector2 anchor_B = gt.xform(Vector2(0, initial_offset));
			PhysicsServer2D::joint_make_groove(p_joint, groove_A1, groove_A2, anchor_B, body_a->get_rid(), body_b->get_rid());
		} break;
		case JOINT_DAMPED_SPRING: {
			Transform2D gt = get_global_transform();
			Vector2 anchor_A = gt.get_origin();
			Vector2 anchor_B = gt.xform(Vector2(0, length));
			PhysicsServer2D::joint_make_damped_spring(p_joint, anchor_A, anchor_B, body_a->get_rid(), body_b->get_rid());
			if (rest_length) {
				PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_REST_LENGTH, rest_length);
			}
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_STIFFNESS, stiffness);
			PhysicsServer2D::damped_spring_joint_set_param(p_joint, PS2DE::DAMPED_SPRING_DAMPING, damping);
		} break;
	}
}

void Joint2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
		case NOTIFICATION_DRAW: {
			if (!is_inside_tree()) {
				break;
			}
			if (!Engine::is_editor_hint() && !SceneTree::is_debugging_collisions_hint()) {
				break;
			}
			if (joint_type == JOINT_PIN) {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, -10), Point2(0, +10), Color(0.7, 0.6, 0.0, 0.5), 3);
			} else {
				draw_line(Point2(-10, 0), Point2(+10, 0), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(-10, length), Point2(+10, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				draw_line(Point2(0, 0), Point2(0, length), Color(0.7, 0.6, 0.0, 0.5), 3);
				if (joint_type == JOINT_GROOVE) {
					draw_line(Point2(-10, initial_offset), Point2(+10, initial_offset), Color(0.8, 0.8, 0.9, 0.5), 5);
				}
			}
		} break;
	}
}

void Joint2D::set_bias(real_t p_bias) {
	bias = p_bias;
	if (joint.is_valid()) {
		PhysicsServer2D::joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);
	}
}

real_t Joint2D::get_bias() const {
	return bias;
}

void Joint2D::set_exclude_nodes_from_collision(bool p_enable) {
	if (exclude_from_collision == p_enable) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	exclude_from_collision = p_enable;
	_update_joint();
}

bool Joint2D::get_exclude_nodes_from_collision() const {
	return exclude_from_collision;
}

void Joint2D::set_softness(real_t p_softness) {
	softness = p_softness;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_SOFTNESS, p_softness);
	}
}

void Joint2D::set_angular_limit_lower(real_t p_angular_limit_lower) {
	angular_limit_lower = p_angular_limit_lower;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_LOWER, p_angular_limit_lower);
	}
}

void Joint2D::set_angular_limit_upper(real_t p_angular_limit_upper) {
	angular_limit_upper = p_angular_limit_upper;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_LIMIT_UPPER, p_angular_limit_upper);
	}
}

void Joint2D::set_motor_target_velocity(real_t p_motor_target_velocity) {
	motor_target_velocity = p_motor_target_velocity;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_param(joint, PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY, p_motor_target_velocity);
	}
}

void Joint2D::set_motor_enabled(bool p_motor_enabled) {
	motor_enabled = p_motor_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED, p_motor_enabled);
	}
}

void Joint2D::set_angular_limit_enabled(bool p_angular_limit_enabled) {
	angular_limit_enabled = p_angular_limit_enabled;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer2D::pin_joint_set_flag(joint, PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED, p_angular_limit_enabled);
	}
}

void Joint2D::set_length(real_t p_length) {
	length = p_length;
	queue_redraw();
	if (is_configured() && joint_type != JOINT_PIN) {
		_update_joint();
	}
}

void Joint2D::set_initial_offset(real_t p_initial_offset) {
	initial_offset = p_initial_offset;
	queue_redraw();
	if (is_configured() && joint_type == JOINT_GROOVE) {
		_update_joint();
	}
}

void Joint2D::set_rest_length(real_t p_rest_length) {
	rest_length = p_rest_length;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_REST_LENGTH, p_rest_length ? p_rest_length : length);
	}
}

void Joint2D::set_stiffness(real_t p_stiffness) {
	stiffness = p_stiffness;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_STIFFNESS, p_stiffness);
	}
}

void Joint2D::set_damping(real_t p_damping) {
	damping = p_damping;
	if (is_configured() && joint_type == JOINT_DAMPED_SPRING) {
		PhysicsServer2D::damped_spring_joint_set_param(joint, PS2DE::DAMPED_SPRING_DAMPING, p_damping);
	}
}

PackedStringArray Joint2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}

// ============================================================================
// Merged from scene/resources/2d/shape_2d.cpp
// ============================================================================

#include "scene/2d/node_2d.h"

#include "core/config/project_settings.h"
#include "core/math/geometry_2d.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "servers/rendering/renderer.h"

RID Shape2D::_create_shape_rid(ShapeType p_type)
{
	switch (p_type) {
	case SHAPE_WORLD_BOUNDARY:
		return PhysicsServer2D::world_boundary_shape_create();
	case SHAPE_SEPARATION_RAY:
		return PhysicsServer2D::separation_ray_shape_create();
	case SHAPE_SEGMENT:
		return PhysicsServer2D::segment_shape_create();
	case SHAPE_CIRCLE:
		return PhysicsServer2D::circle_shape_create();
	case SHAPE_RECTANGLE:
		return PhysicsServer2D::rectangle_shape_create();
	case SHAPE_CAPSULE:
		return PhysicsServer2D::capsule_shape_create();
	case SHAPE_CONVEX_POLYGON:
		return PhysicsServer2D::convex_polygon_shape_create();
	case SHAPE_CONCAVE_POLYGON:
		return PhysicsServer2D::concave_polygon_shape_create();
	}
	return RID();
}

void Shape2D::_update_shape()
{
	emit_changed();
}

void Shape2D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	if (shape.is_valid() && PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::free_rid(shape);
	}
	shape_type = p_type;
	shape = _create_shape_rid(shape_type);
	if (custom_bias != 0.0) {
		PhysicsServer2D::shape_set_custom_solver_bias(shape, custom_bias);
	}
	_update_shape();
}

RID Shape2D::get_rid() const { return shape; }

// World boundary

void Shape2D::set_normal(const Vector2& p_normal)
{
	ERR_FAIL_COND(p_normal.is_zero_approx());
	if (normal == p_normal) {
		return;
	}
	normal = p_normal;
	_update_shape();
}

Vector2 Shape2D::get_normal() const { return normal; }

void Shape2D::set_distance(real_t p_distance)
{
	if (distance == p_distance) {
		return;
	}
	distance = p_distance;
	_update_shape();
}

real_t Shape2D::get_distance() const { return distance; }

// Separation ray

void Shape2D::set_length(real_t p_length)
{
	if (length == p_length) {
		return;
	}
	length = p_length;
	_update_shape();
}

real_t Shape2D::get_length() const { return length; }

void Shape2D::set_slide_on_slope(bool p_active)
{
	if (slide_on_slope == p_active) {
		return;
	}
	slide_on_slope = p_active;
	_update_shape();
}

bool Shape2D::get_slide_on_slope() const { return slide_on_slope; }

// Segment

void Shape2D::set_a(const Vector2& p_a)
{
	if (a == p_a) {
		return;
	}
	a = p_a;
	_update_shape();
}

Vector2 Shape2D::get_a() const { return a; }

void Shape2D::set_b(const Vector2& p_b)
{
	if (b == p_b) {
		return;
	}
	b = p_b;
	_update_shape();
}

Vector2 Shape2D::get_b() const { return b; }

// Circle / Capsule

Vector<Vector2> Shape2D::_get_capsule_points() const
{
	Vector<Vector2> capsule_points;
	const real_t turn_step = Math::TAU / 24.0;
	for (int i = 0; i < 24; i++) {
		Vector2 ofs =
			Vector2(0, (i > 6 && i <= 18) ? -height * 0.5f + radius : height * 0.5f - radius);

		capsule_points.push_back(
			Vector2(Math::sin(i * turn_step), Math::cos(i * turn_step)) * radius + ofs);
		if (i == 6 || i == 18) {
			capsule_points.push_back(
				Vector2(Math::sin(i * turn_step), Math::cos(i * turn_step)) * radius - ofs);
		}
	}

	return capsule_points;
}

void Shape2D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0f, "Shape2D radius cannot be negative.");
	if (radius == p_radius) {
		return;
	}
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && height < radius * 2.0f) {
		height = radius * 2.0f;
	}
	_update_shape();
}

real_t Shape2D::get_radius() const { return radius; }

void Shape2D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0f, "Shape2D height cannot be negative.");
	if (height == p_height) {
		return;
	}
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
	_update_shape();
}

real_t Shape2D::get_height() const { return height; }

void Shape2D::set_mid_height(real_t p_mid_height)
{
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "Shape2D mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
	_update_shape();
}

real_t Shape2D::get_mid_height() const { return height - radius * 2.0f; }

// Rectangle

void Shape2D::set_size(const Size2& p_size)
{
	ERR_FAIL_COND_MSG(p_size.x < 0 || p_size.y < 0, "Shape2D size cannot be negative.");
	if (size == p_size) {
		return;
	}
	size = p_size;
	_update_shape();
}

Size2 Shape2D::get_size() const { return size; }

// Convex polygon

void Shape2D::set_point_cloud(const Vector<Vector2>& p_points)
{
	Vector<Point2> hull = Geometry2D::convex_hull(p_points);
	ERR_FAIL_COND(hull.size() < 3);
	set_points(hull);
}

void Shape2D::set_points(const Vector<Vector2>& p_points)
{
	points = p_points;
	_update_shape();
}

Vector<Vector2> Shape2D::get_points() const { return points; }

// Concave polygon

void Shape2D::set_segments(const Vector<Vector2>& p_segments)
{
	segments = p_segments;
	_update_shape();
}

Vector<Vector2> Shape2D::get_segments() const { return segments; }

// Solver bias & collision

void Shape2D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer2D::shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape2D::get_custom_solver_bias() const { return custom_bias; }

bool Shape2D::collide_with_motion(const Transform2D& p_local_xform, const Vector2& p_local_motion,
	Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	int r;
	return PhysicsServer2D::shape_collide(get_rid(), p_local_xform, p_local_motion,
		rp_shape->get_rid(), p_shape_xform, p_shape_motion, nullptr, 0, r);
}

bool Shape2D::collide(
	const Transform2D& p_local_xform, Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	int r;
	return PhysicsServer2D::shape_collide(get_rid(), p_local_xform, Vector2(),
		rp_shape->get_rid(), p_shape_xform, Vector2(), nullptr, 0, r);
}

PackedVector2Array Shape2D::collide_with_motion_and_get_contacts(const Transform2D& p_local_xform,
	const Vector2& p_local_motion, Shape2D* rp_shape, const Transform2D& p_shape_xform,
	const Vector2& p_shape_motion)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::shape_collide(get_rid(), p_local_xform, p_local_motion,
			rp_shape->get_rid(), p_shape_xform, p_shape_motion, result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

PackedVector2Array Shape2D::collide_and_get_contacts(const Transform2D& p_local_xform,
	Shape2D* rp_shape, const Transform2D& p_shape_xform)
{
	const int max_contacts = 16;
	Vector2 result[max_contacts * 2];
	int contacts = 0;

	if (!PhysicsServer2D::shape_collide(get_rid(), p_local_xform, Vector2(),
			rp_shape->get_rid(), p_shape_xform, Vector2(), result, max_contacts, contacts)) {
		return PackedVector2Array();
	}

	PackedVector2Array results;
	results.resize(contacts * 2);
	for (int i = 0; i < contacts * 2; i++) {
		results.write[i] = result[i];
	}

	return results;
}

// Selection, drawing, bounds

bool Shape2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		const Vector2 shape_center = distance * normal;
		const Vector2 ortho_segment_a = shape_center - normal.orthogonal() * 100;
		const Vector2 ortho_segment_b = shape_center + normal.orthogonal() * 100;
		const Vector2 ortho_closest =
			Geometry2D::get_closest_point_to_segment(p_point, ortho_segment_a, ortho_segment_b);
		if (p_point.distance_to(ortho_closest) < p_tolerance) {
			return true;
		}
		const Vector2 normal_segment_a = shape_center;
		const Vector2 normal_segment_b = shape_center + normal * 30;
		const Vector2 normal_closest =
			Geometry2D::get_closest_point_to_segment(p_point, normal_segment_a, normal_segment_b);
		if (p_point.distance_to(normal_closest) < p_tolerance) {
			return true;
		}
		return false;
	}
	case SHAPE_SEGMENT: {
		Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, a, b);
		return p_point.distance_to(closest) < p_tolerance;
	}
	case SHAPE_CIRCLE: {
		return p_point.length() < radius + p_tolerance;
	}
	case SHAPE_CAPSULE: {
		return Geometry2D::is_point_in_polygon(p_point, _get_capsule_points());
	}
	case SHAPE_CONVEX_POLYGON: {
		return Geometry2D::is_point_in_polygon(p_point, points);
	}
	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0 || (len % 2) == 1) {
			return false;
		}
		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i += 2) {
			Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, r[i], r[i + 1]);
			if (p_point.distance_to(closest) < p_tolerance) {
				return true;
			}
		}
		return false;
	}
	case SHAPE_SEPARATION_RAY:
	case SHAPE_RECTANGLE:
	default:
		return get_rect().has_point(p_point);
	}
}

void Shape2D::draw(const RID& p_to_rid, const Color& p_color)
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Vector2 point = distance * normal;
		real_t line_width = 3.0;

		PackedVector2Array line_points = {point - normal.orthogonal() * 100,
			point - normal.orthogonal() * 60, point + normal.orthogonal() * 60,
			point + normal.orthogonal() * 100};

		Color transparent_color = Color(p_color, 0);
		PackedColorArray line_colors = {transparent_color, p_color, p_color, transparent_color};

		Renderer::canvas_item_add_polyline(p_to_rid, line_points, line_colors, line_width);

		Color arrow_color = p_color.inverted();

		Transform2D xf;
		xf.rotate(normal.angle());

		Vector<Vector2> arrow_points = {
			xf.xform(Vector2(distance + line_width / 2, -2.5)),
			xf.xform(Vector2(distance + 20, -2.5)),
			xf.xform(Vector2(distance + 20, -10)),
			xf.xform(Vector2(distance + 40, 0)),
			xf.xform(Vector2(distance + 20, 10)),
			xf.xform(Vector2(distance + 20, 2.5)),
			xf.xform(Vector2(distance + line_width / 2, 2.5)),
		};

		Renderer::canvas_item_add_polyline(
			p_to_rid, arrow_points, {arrow_color}, line_width / 2);
	} break;

	case SHAPE_SEPARATION_RAY: {
		const Vector2 target_position = Vector2(0, length);

		const float max_arrow_size = 6;
		const float line_width = 1.4;
		bool no_line = target_position.length() < line_width;
		float arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

		if (no_line) {
			arrow_size = target_position.length();
		}
		else {
			Renderer::canvas_item_add_line(p_to_rid, Vector2(),
				target_position - target_position.normalized() * arrow_size, p_color, line_width);
		}

		Transform2D xf;
		xf.rotate(target_position.angle());
		xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

		Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)),
			xf.xform(Vector2(0, 0.5 * arrow_size)),
			xf.xform(Vector2(0, -0.5 * arrow_size))};

		Vector<Color> cols = {p_color, p_color, p_color};

		Renderer::canvas_item_add_primitive(p_to_rid, pts, cols, Vector<Point2>(), RID());
	} break;

	case SHAPE_SEGMENT: {
		Renderer::canvas_item_add_line(p_to_rid, a, b, p_color, 3);
	} break;

	case SHAPE_CIRCLE: {
		Vector<Vector2> circle_points;
		circle_points.resize(24);

		const real_t turn_step = Math::TAU / 24.0;
		for (int i = 0; i < 24; i++) {
			circle_points.write[i] =
				Vector2(Math::cos(i * turn_step), Math::sin(i * turn_step)) * radius;
		}

		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, circle_points, col);

		if (is_collision_outline_enabled()) {
			circle_points.push_back(circle_points[0]);
			col = {Color(p_color, 1.0)};
			Renderer::canvas_item_add_polyline(p_to_rid, circle_points, col);
		}
	} break;

	case SHAPE_RECTANGLE: {
		Renderer::canvas_item_add_rect(
			p_to_rid, Rect2(-size * 0.5, size), p_color);
		if (is_collision_outline_enabled()) {
			Vector<Vector2> stroke_points;
			stroke_points.resize(5);
			stroke_points.write[0] = -size * 0.5;
			stroke_points.write[1] = Vector2(size.x, -size.y) * 0.5;
			stroke_points.write[2] = size * 0.5;
			stroke_points.write[3] = Vector2(-size.x, size.y) * 0.5;
			stroke_points.write[4] = -size * 0.5;

			Vector<Color> stroke_colors = {Color(p_color, 1.0)};

			Renderer::canvas_item_add_polyline(
				p_to_rid, stroke_points, stroke_colors);
		}
	} break;

	case SHAPE_CAPSULE: {
		Vector<Vector2> capsule_points = _get_capsule_points();
		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, capsule_points, col);

		if (is_collision_outline_enabled()) {
			capsule_points.push_back(capsule_points[0]);
			col = {Color(p_color, 1.0f)};
			Renderer::canvas_item_add_polyline(p_to_rid, capsule_points, col);
		}
	} break;

	case SHAPE_CONVEX_POLYGON: {
		if (points.size() < 3) {
			return;
		}

		Vector<Color> col = {p_color};
		Renderer::canvas_item_add_polygon(p_to_rid, points, col);

		if (is_collision_outline_enabled()) {
			col = {Color(p_color, 1.0)};
			Renderer::canvas_item_add_polyline(p_to_rid, points, col);
			Renderer::canvas_item_add_line(
				p_to_rid, points[points.size() - 1], points[0], Color(p_color, 1.0));
		}
	} break;

	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0 || (len % 2) == 1) {
			return;
		}

		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i += 2) {
			Renderer::canvas_item_add_line(
				p_to_rid, r[i], r[i + 1], p_color, 2);
		}
	} break;
	}
}

Rect2 Shape2D::get_rect() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Vector2 point = distance * normal;
		Vector2 l1[2] = {point - normal.orthogonal() * 100, point + normal.orthogonal() * 100};
		Vector2 l2[2] = {point, point + normal * 30};
		Rect2 rect;
		rect.position = l1[0];
		rect.expand_to(l1[1]);
		rect.expand_to(l2[0]);
		rect.expand_to(l2[1]);
		return rect;
	}
	case SHAPE_SEPARATION_RAY: {
		Rect2 rect;
		rect.position = Vector2();
		rect.expand_to(Vector2(0, length));
		rect = rect.grow(Math::SQRT12 * 4);
		return rect;
	}
	case SHAPE_SEGMENT: {
		Rect2 rect;
		rect.position = a;
		rect.expand_to(b);
		return rect;
	}
	case SHAPE_CIRCLE: {
		Rect2 rect;
		rect.position = -Point2(radius, radius);
		rect.size = Point2(radius, radius) * 2.0;
		return rect;
	}
	case SHAPE_RECTANGLE: {
		return Rect2(-size * 0.5, size);
	}
	case SHAPE_CAPSULE: {
		const Vector2 half_size = Vector2(radius, height * 0.5f);
		return Rect2(-half_size, half_size * 2.0f);
	}
	case SHAPE_CONVEX_POLYGON: {
		Rect2 rect;
		for (int i = 0; i < points.size(); i++) {
			if (i == 0) {
				rect.position = points[i];
			}
			else {
				rect.expand_to(points[i]);
			}
		}
		return rect;
	}
	case SHAPE_CONCAVE_POLYGON: {
		int len = segments.size();
		if (len == 0) {
			return Rect2();
		}
		Rect2 rect;
		const Vector2* r = segments.ptr();
		for (int i = 0; i < len; i++) {
			if (i == 0) {
				rect.position = r[i];
			}
			else {
				rect.expand_to(r[i]);
			}
		}
		return rect;
	}
	}
	return Rect2();
}

real_t Shape2D::get_enclosing_radius() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
		return distance;
	case SHAPE_SEPARATION_RAY:
		return length;
	case SHAPE_SEGMENT:
		return (a + b).length();
	case SHAPE_CIRCLE:
		return radius;
	case SHAPE_RECTANGLE:
		return size.length() / 2;
	case SHAPE_CAPSULE:
		return height * 0.5f;
	case SHAPE_CONVEX_POLYGON: {
		real_t r = 0.0;
		for (int i = 0; i < points.size(); i++) {
			r = MAX(points[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_CONCAVE_POLYGON: {
		const Vector2* read = segments.ptr();
		real_t r = 0.0;
		for (int i = 0; i < segments.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	}
	return 0.0;
}

bool Shape2D::is_collision_outline_enabled()
{
	return GLOBAL_GET_CACHED(bool, "debug/shapes/collision/draw_2d_outlines");
}

Shape2D::Shape2D(ShapeType p_type) : shape_type(p_type)
{
	shape = _create_shape_rid(shape_type);
	_update_shape();
}

Shape2D::~Shape2D()
{
	ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
	PhysicsServer2D::free_rid(shape);
}


// --- Merged from scene/2d/navigation/navigation_2d.cpp ---
#if !defined(NAVIGATION_2D_DISABLED)
#include "scene/2d/line_2d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "core/math/random_pcg.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "scene/main/viewport.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/rendering/renderer.h"

void NavigationAgent2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are
		// ready. cannot use READY as ready does not get called if Node is re-added to SceneTree
		set_agent_parent(get_parent());
		set_physics_process_internal(true);

		if (agent_parent && avoidance_enabled) {
			NavigationServer2D::agent_set_position(
				agent, agent_parent->get_global_position());
		}

#ifdef DEBUG_ENABLED
		if (NavigationServer2D::get_debug_enabled()) {
			debug_path_dirty = true;
		}
#endif // DEBUG_ENABLED

	} break;

	case NOTIFICATION_PARENTED: {
		if (is_inside_tree() && (get_parent() != agent_parent)) {
			// only react to PARENTED notifications when already inside_tree and parent changed,
			// e.g. users switch nodes around PARENTED notification fires also when Node is added in
			// scripts to a parent this would spam transforms fails and world fails while Node is
			// outside SceneTree when node gets reparented when joining the tree POST_ENTER_TREE
			// takes care of this
			set_agent_parent(get_parent());
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_UNPARENTED: {
		// if agent has no parent no point in processing it until reparented
		set_agent_parent(nullptr);
		set_physics_process_internal(false);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_agent_parent(nullptr);
		set_physics_process_internal(false);

#ifdef DEBUG_ENABLED
		if (debug_path_instance.is_valid()) {
			Renderer::canvas_item_set_visible(debug_path_instance, false);
		}
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (agent_parent) {
			NavigationServer2D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (agent_parent) {
			NavigationServer2D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (agent_parent && avoidance_enabled) {
			NavigationServer2D::agent_set_position(
				agent, agent_parent->get_global_position());
		}
		if (agent_parent && target_position_submitted) {
			if (velocity_submitted) {
				velocity_submitted = false;
				if (avoidance_enabled) {
					NavigationServer2D::agent_set_velocity(agent, velocity);
				}
			}
			if (velocity_forced_submitted) {
				velocity_forced_submitted = false;
				if (avoidance_enabled) {
					NavigationServer2D::agent_set_velocity_forced(
						agent, velocity_forced);
				}
			}
		}
#ifdef DEBUG_ENABLED
		if (debug_path_dirty) {
			_update_debug_path();
		}
#endif // DEBUG_ENABLED
	} break;
	}
}

bool NavigationAgent2D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationAgent2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	if (target_position_submitted) {
		_request_repath();
	}
}

uint32_t NavigationAgent2D::get_navigation_layers() const { return navigation_layers; }

void NavigationAgent2D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");
	uint32_t _navigation_layers = get_navigation_layers();
	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}
	set_navigation_layers(_navigation_layers);
}

bool NavigationAgent2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");
	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent2D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	if (pathfinding_algorithm == p_pathfinding_algorithm) {
		return;
	}

	pathfinding_algorithm = p_pathfinding_algorithm;

	navigation_query->set_pathfinding_algorithm(pathfinding_algorithm);
}

void NavigationAgent2D::set_path_postprocessing(
	const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing)
{
	if (path_postprocessing == p_path_postprocessing) {
		return;
	}

	path_postprocessing = p_path_postprocessing;

	navigation_query->set_path_postprocessing(path_postprocessing);
}

void NavigationAgent2D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
	navigation_query->set_simplify_path(simplify_path);
}

bool NavigationAgent2D::get_simplify_path() const { return simplify_path; }

void NavigationAgent2D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
	navigation_query->set_simplify_epsilon(simplify_epsilon);
}

real_t NavigationAgent2D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationAgent2D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
	navigation_query->set_path_return_max_length(path_return_max_length);
}

float NavigationAgent2D::get_path_return_max_length() const { return path_return_max_length; }

void NavigationAgent2D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
	navigation_query->set_path_return_max_radius(path_return_max_radius);
}

float NavigationAgent2D::get_path_return_max_radius() const { return path_return_max_radius; }

void NavigationAgent2D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
	navigation_query->set_path_search_max_polygons(path_search_max_polygons);
}

int NavigationAgent2D::get_path_search_max_polygons() const { return path_search_max_polygons; }

void NavigationAgent2D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
	navigation_query->set_path_search_max_distance(path_search_max_distance);
}

float NavigationAgent2D::get_path_search_max_distance() const { return path_search_max_distance; }

float NavigationAgent2D::get_path_length() const { return navigation_result->get_path_length(); }

void NavigationAgent2D::set_path_metadata_flags(uint32_t p_path_metadata_flags)
{
	if (path_metadata_flags == p_path_metadata_flags) {
		return;
	}

	path_metadata_flags = p_path_metadata_flags;
}

void NavigationAgent2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::agent_set_map(agent, map_override);
	if (target_position_submitted) {
		_request_repath();
	}
}

RID NavigationAgent2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (agent_parent != nullptr) {
		return agent_parent->get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationAgent2D::set_path_desired_distance(real_t p_path_desired_distance)
{
	if (Math::is_equal_approx(path_desired_distance, p_path_desired_distance)) {
		return;
	}

	path_desired_distance = p_path_desired_distance;
}

void NavigationAgent2D::set_target_desired_distance(real_t p_target_desired_distance)
{
	if (Math::is_equal_approx(target_desired_distance, p_target_desired_distance)) {
		return;
	}

	target_desired_distance = p_target_desired_distance;
}

void NavigationAgent2D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;

	NavigationServer2D::agent_set_radius(agent, radius);
}

void NavigationAgent2D::set_neighbor_distance(real_t p_distance)
{
	if (Math::is_equal_approx(neighbor_distance, p_distance)) {
		return;
	}

	neighbor_distance = p_distance;

	NavigationServer2D::agent_set_neighbor_distance(agent, neighbor_distance);
}

void NavigationAgent2D::set_max_neighbors(int p_count)
{
	if (max_neighbors == p_count) {
		return;
	}

	max_neighbors = p_count;

	NavigationServer2D::agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent2D::set_time_horizon_agents(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_agents, p_time_horizon)) {
		return;
	}
	time_horizon_agents = p_time_horizon;
	NavigationServer2D::agent_set_time_horizon_agents(agent, time_horizon_agents);
}

void NavigationAgent2D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_obstacles, p_time_horizon)) {
		return;
	}
	time_horizon_obstacles = p_time_horizon;
	NavigationServer2D::agent_set_time_horizon_obstacles(
		agent, time_horizon_obstacles);
}

void NavigationAgent2D::set_max_speed(real_t p_max_speed)
{
	ERR_FAIL_COND_MSG(p_max_speed < 0.0, "Max speed must be positive.");
	if (Math::is_equal_approx(max_speed, p_max_speed)) {
		return;
	}
	max_speed = p_max_speed;

	NavigationServer2D::agent_set_max_speed(agent, max_speed);
}

void NavigationAgent2D::set_path_max_distance(real_t p_path_max_distance)
{
	if (Math::is_equal_approx(path_max_distance, p_path_max_distance)) {
		return;
	}

	path_max_distance = p_path_max_distance;
}

real_t NavigationAgent2D::get_path_max_distance() { return path_max_distance; }

void NavigationAgent2D::set_target_position(Vector2 p_position)
{
	// Intentionally not checking for equality of the parameter, as we want to update the path even
	// if the target position is the same in case the world changed. Revisit later when the
	// navigation server can update the path without requesting a new path.

	target_position = p_position;
	target_position_submitted = true;

	_request_repath();
}

Vector2 NavigationAgent2D::get_target_position() const { return target_position; }

Vector2 NavigationAgent2D::get_next_path_position()
{
	_update_navigation();

	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		ERR_FAIL_NULL_V_MSG(agent_parent, Vector2(), "The agent has no parent.");
		return agent_parent->get_global_position();
	}
	else {
		return navigation_path[navigation_path_index];
	}
}

real_t NavigationAgent2D::distance_to_target() const
{
	ERR_FAIL_NULL_V_MSG(agent_parent, 0.0, "The agent has no parent.");
	return agent_parent->get_global_position().distance_to(target_position);
}

bool NavigationAgent2D::is_target_reached() const { return target_reached; }

bool NavigationAgent2D::is_target_reachable()
{
	_update_navigation();
	return _is_target_reachable();
}

bool NavigationAgent2D::_is_target_reachable() const
{
	return target_desired_distance >= _get_final_position().distance_to(target_position);
}

bool NavigationAgent2D::is_navigation_finished()
{
	_update_navigation();
	return navigation_finished;
}

Vector2 NavigationAgent2D::get_final_position()
{
	_update_navigation();
	return _get_final_position();
}

Vector2 NavigationAgent2D::_get_final_position() const
{
	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		return Vector2();
	}
	return navigation_path[navigation_path.size() - 1];
}

void NavigationAgent2D::set_velocity_forced(Vector2 p_velocity)
{
	// Intentionally not checking for equality of the parameter.
	// We need to always submit the velocity to the navigation server, even when it is the same, in
	// order to run avoidance every frame. Revisit later when the navigation server can update
	// avoidance without users resubmitting the velocity.

	velocity_forced = p_velocity;
	velocity_forced_submitted = true;
}

void NavigationAgent2D::set_velocity(const Vector2 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationAgent2D::_advance_waypoints(const Vector2& p_origin)
{
	if (last_waypoint_reached) {
		return;
	}

	// Advance to the farthest possible waypoint.
	while (_is_within_waypoint_distance(p_origin)) {
		_trigger_waypoint_reached();

		if (_is_last_waypoint()) {
			last_waypoint_reached = true;
			break;
		}

		_move_to_next_waypoint();
	}
}

void NavigationAgent2D::_request_repath()
{
	navigation_result->reset();
	target_reached = false;
	navigation_finished = false;
	last_waypoint_reached = false;
}

bool NavigationAgent2D::_is_last_waypoint() const
{
	return navigation_path_index == navigation_result->get_path().size() - 1;
}

void NavigationAgent2D::_move_to_next_waypoint() { navigation_path_index += 1; }

bool NavigationAgent2D::_is_within_waypoint_distance(const Vector2& p_origin) const
{
	const Vector<Vector2>& navigation_path = navigation_result->get_path();
	return p_origin.distance_to(navigation_path[navigation_path_index]) < path_desired_distance;
}

bool NavigationAgent2D::_is_within_target_distance(const Vector2& p_origin) const
{
	return p_origin.distance_to(target_position) < target_desired_distance;
}

void NavigationAgent2D::_transition_to_navigation_finished()
{
	navigation_finished = true;
	target_position_submitted = false;

	if (avoidance_enabled) {
		NavigationServer2D::agent_set_position(
			agent, agent_parent->get_global_position());
		NavigationServer2D::agent_set_velocity(agent, Vector2(0.0, 0.0));
		NavigationServer2D::agent_set_velocity_forced(agent, Vector2(0.0, 0.0));
	}
}

void NavigationAgent2D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer2D::agent_set_avoidance_layers(get_rid(), avoidance_layers);
}

uint32_t NavigationAgent2D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationAgent2D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	NavigationServer2D::agent_set_avoidance_mask(get_rid(), p_mask);
}

uint32_t NavigationAgent2D::get_avoidance_mask() const { return avoidance_mask; }

void NavigationAgent2D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationAgent2D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent2D::set_avoidance_mask_value(int p_mask_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_mask_number < 1, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_mask_number > 32, "Avoidance mask number must be between 1 and 32 inclusive.");
	uint32_t mask = get_avoidance_mask();
	if (p_value) {
		mask |= 1 << (p_mask_number - 1);
	}
	else {
		mask &= ~(1 << (p_mask_number - 1));
	}
	set_avoidance_mask(mask);
}

bool NavigationAgent2D::get_avoidance_mask_value(int p_mask_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_mask_number < 1, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_mask_number > 32, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	return get_avoidance_mask() & (1 << (p_mask_number - 1));
}

void NavigationAgent2D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	NavigationServer2D::agent_set_avoidance_priority(get_rid(), p_priority);
}

real_t NavigationAgent2D::get_avoidance_priority() const { return avoidance_priority; }

////////DEBUG////////////////////////////////////////////////////////////

void NavigationAgent2D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled == p_enabled) {
		return;
	}

	debug_enabled = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent2D::get_debug_enabled() const { return debug_enabled; }

void NavigationAgent2D::set_debug_use_custom(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_use_custom == p_enabled) {
		return;
	}

	debug_use_custom = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent2D::get_debug_use_custom() const { return debug_use_custom; }

void NavigationAgent2D::set_debug_path_custom_color(Color p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_path_custom_color == p_color) {
		return;
	}

	debug_path_custom_color = p_color;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

Color NavigationAgent2D::get_debug_path_custom_color() const { return debug_path_custom_color; }

void NavigationAgent2D::set_debug_path_custom_point_size(float p_point_size)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_point_size, p_point_size)) {
		return;
	}

	debug_path_custom_point_size = MAX(0.0, p_point_size);
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent2D::get_debug_path_custom_point_size() const
{
	return debug_path_custom_point_size;
}

void NavigationAgent2D::set_debug_path_custom_line_width(float p_line_width)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_line_width, p_line_width)) {
		return;
	}

	debug_path_custom_line_width = p_line_width;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent2D::get_debug_path_custom_line_width() const
{
	return debug_path_custom_line_width;
}

#ifdef DEBUG_ENABLED
void NavigationAgent2D::_navigation_debug_changed() { debug_path_dirty = true; }

void NavigationAgent2D::_update_debug_path()
{
	if (!debug_path_dirty) {
		return;
	}
	debug_path_dirty = false;

	if (!debug_path_instance.is_valid()) {
		debug_path_instance = Renderer::canvas_item_create();
	}

	Renderer::canvas_item_clear(debug_path_instance);

	if (!(debug_enabled &&
			NavigationServer2D::get_debug_navigation_enable_agent_paths())) {
		return;
	}

	if (!(agent_parent && agent_parent->is_inside_tree())) {
		return;
	}

	Renderer::canvas_item_set_parent(
		debug_path_instance, agent_parent->get_canvas());
	Renderer::canvas_item_set_z_index(
		debug_path_instance, RSE::CANVAS_ITEM_Z_MAX - 1);
	Renderer::canvas_item_set_visible(
		debug_path_instance, agent_parent->is_visible_in_tree());

	const Vector<Vector2>& navigation_path = navigation_result->get_path();

	if (navigation_path.size() <= 1) {
		return;
	}

	Color debug_path_color =
		NavigationServer2D::get_debug_navigation_agent_path_color();
	if (debug_use_custom) {
		debug_path_color = debug_path_custom_color;
	}

	Vector<Color> debug_path_colors;
	debug_path_colors.resize(navigation_path.size());
	debug_path_colors.fill(debug_path_color);

	Renderer::canvas_item_add_polyline(debug_path_instance, navigation_path,
		debug_path_colors, debug_path_custom_line_width, false);

	if (debug_path_custom_point_size <= 0.0) {
		return;
	}

	float point_size =
		NavigationServer2D::get_debug_navigation_agent_path_point_size();
	float half_point_size = point_size * 0.5;

	if (debug_use_custom) {
		point_size = debug_path_custom_point_size;
		half_point_size = debug_path_custom_point_size * 0.5;
	}

	for (int i = 0; i < navigation_path.size(); i++) {
		const Vector2& vert = navigation_path[i];
		Rect2 path_point_rect =
			Rect2(vert.x - half_point_size, vert.y - half_point_size, point_size, point_size);
		Renderer::canvas_item_add_rect(
			debug_path_instance, path_point_rect, debug_path_color);
	}
}
#endif // DEBUG_ENABLED



void NavigationAgent2D::_update_navigation() {}

void NavigationAgent2D::set_agent_parent(Node*) {}

void NavigationAgent2D::_trigger_waypoint_reached() {}


void NavigationLink2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_link_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_link_update_transform();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_link_exit_navigation_map();
	} break;
	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		_update_debug_mesh();
#endif // DEBUG_ENABLED
	} break;
	}
}

#ifdef DEBUG_ENABLED
Rect2 NavigationLink2D::_edit_get_rect() const
{
	if (!is_inside_tree()) {
		return Rect2();
	}

	real_t radius = NavigationServer2D::map_get_link_connection_radius(
		get_world_2d()->get_navigation_map());

	Rect2 rect(get_start_position(), Size2());
	rect.expand_to(get_end_position());
	rect.grow_by(radius);
	return rect;
}

bool NavigationLink2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	Vector2 closest_point =
		Geometry2D::get_closest_point_to_segment(p_point, get_start_position(), get_end_position());
	return p_point.distance_to(closest_point) < p_tolerance;
}
#endif // DEBUG_ENABLED

RID NavigationLink2D::get_rid() const { return link; }

void NavigationLink2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::link_set_map(link, map_override);
}

RID NavigationLink2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationLink2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer2D::link_set_navigation_layers(link, navigation_layers);
}

void NavigationLink2D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationLink2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationLink2D::set_global_start_position(Vector2 p_position)
{
	if (is_inside_tree()) {
		set_start_position(to_local(p_position));
	}
	else {
		set_start_position(p_position);
	}
}

Vector2 NavigationLink2D::get_global_start_position() const
{
	if (is_inside_tree()) {
		return to_global(start_position);
	}
	else {
		return start_position;
	}
}

void NavigationLink2D::set_global_end_position(Vector2 p_position)
{
	if (is_inside_tree()) {
		set_end_position(to_local(p_position));
	}
	else {
		set_end_position(p_position);
	}
}

Vector2 NavigationLink2D::get_global_end_position() const
{
	if (is_inside_tree()) {
		return to_global(end_position);
	}
	else {
		return end_position;
	}
}

void NavigationLink2D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer2D::link_set_enter_cost(link, enter_cost);
}

void NavigationLink2D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer2D::link_set_travel_cost(link, travel_cost);
}

PackedStringArray NavigationLink2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (start_position.is_equal_approx(end_position)) {
		warnings.push_back(RTR("NavigationLink2D start position should be different than the end "
							   "position to be useful."));
	}

	return warnings;
}

void NavigationLink2D::_link_exit_navigation_map()
{
	NavigationServer2D::link_set_map(link, RID());
}

#ifdef DEBUG_ENABLED
void NavigationLink2D::_update_debug_mesh()
{
	if (!is_inside_tree()) {
		return;
	}

	if (!Engine::is_editor_hint() &&
		!NavigationServer2D::get_debug_enabled()) {
		return;
	}

	Color color;
	if (enabled) {
		color = NavigationServer2D::get_debug_navigation_link_connection_color();
	}
	else {
		color = NavigationServer2D::get_debug_navigation_link_connection_disabled_color();
	}

	real_t radius = NavigationServer2D::map_get_link_connection_radius(
		get_world_2d()->get_navigation_map());

	draw_line(get_start_position(), get_end_position(), color);
	draw_arc(get_start_position(), radius, 0, Math::TAU, 10, color);
	draw_arc(get_end_position(), radius, 0, Math::TAU, 10, color);

	const Vector2 link_segment = end_position - start_position;
	const float arror_len = 5.0;

	{
		Vector2 anchor = start_position + (link_segment * 0.75);
		Vector2 direction = start_position.direction_to(end_position);
		Vector2 arrow_dir = -direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);

		arrow_dir = direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);
	}

	if (is_bidirectional()) {
		Vector2 anchor = start_position + (link_segment * 0.25);
		Vector2 direction = end_position.direction_to(start_position);
		Vector2 arrow_dir = -direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);

		arrow_dir = direction.orthogonal();
		draw_line(anchor, anchor + (arrow_dir - direction) * arror_len, color);
	}
}
#endif // DEBUG_ENABLED

NavigationLink2D::NavigationLink2D()
{
	link = NavigationServer2D::link_create();

	NavigationServer2D::link_set_enter_cost(link, enter_cost);
	NavigationServer2D::link_set_travel_cost(link, travel_cost);
	NavigationServer2D::link_set_navigation_layers(link, navigation_layers);
	NavigationServer2D::link_set_bidirectional(link, bidirectional);
	NavigationServer2D::link_set_enabled(link, enabled);

	set_notify_transform(true);
	set_hide_clip_children(true);
}

NavigationLink2D::~NavigationLink2D()
{
	ERR_FAIL_NULL(NavigationServer2D::data);
	NavigationServer2D::free_rid(link);
	link = RID();
}

void NavigationLink2D::set_start_position(Vector2) {}

void NavigationLink2D::set_end_position(Vector2) {}

void NavigationLink2D::_link_update_transform() {}

void NavigationLink2D::_link_enter_navigation_map() {}


RID NavigationObstacle2D::_navmesh_source_geometry_parser;

void NavigationObstacle2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (map_override.is_valid()) {
			_update_map(map_override);
		}
		else if (is_inside_tree()) {
			_update_map(get_world_2d()->get_navigation_map());
		}
		else {
			_update_map(RID());
		}
		previous_transform = get_global_transform();
		// need to trigger map controlled agent assignment somehow for the fake_agent since
		// obstacles use no callback like regular agents
		NavigationServer2D::obstacle_set_avoidance_enabled(
			obstacle, avoidance_enabled);
		_update_transform();
		set_physics_process_internal(true);
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_parent(
			debug_canvas_item, get_world_2d()->get_canvas());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_physics_process_internal(false);
		_update_map(RID());
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_parent(debug_canvas_item, RID());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer2D::obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer2D::obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
#ifdef DEBUG_ENABLED
		Renderer::canvas_item_set_visible(debug_canvas_item, is_visible_in_tree());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_inside_tree()) {
			_update_transform();

			if (velocity_submitted) {
				velocity_submitted = false;
				// only update if there is a noticeable change, else the rvo agent preferred
				// velocity stays the same
				if (!previous_velocity.is_equal_approx(velocity)) {
					NavigationServer2D::obstacle_set_velocity(obstacle, velocity);
				}
				previous_velocity = velocity;
			}
		}
	} break;

	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		if (is_inside_tree()) {
			bool is_debug_enabled = false;
			if (Engine::is_editor_hint()) {
				is_debug_enabled = true;
			}
			else if (NavigationServer2D::get_debug_enabled() &&
					   NavigationServer2D::get_debug_avoidance_enabled()) {
				is_debug_enabled = true;
			}

			if (is_debug_enabled) {
				Renderer::canvas_item_clear(debug_canvas_item);
				Renderer::canvas_item_set_transform(debug_canvas_item, Transform2D());
				_update_fake_agent_radius_debug();
				_update_static_obstacle_debug();
			}
		}
#endif // DEBUG_ENABLED
	} break;
	}
}

NavigationObstacle2D::NavigationObstacle2D()
{
	obstacle = NavigationServer2D::obstacle_create();

	NavigationServer2D::obstacle_set_radius(obstacle, radius);
	NavigationServer2D::obstacle_set_vertices(obstacle, vertices);
	NavigationServer2D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
	NavigationServer2D::obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);

#ifdef DEBUG_ENABLED
	debug_canvas_item = Renderer::canvas_item_create();
	debug_mesh_rid = Renderer::mesh_create();
#endif // DEBUG_ENABLED
}

NavigationObstacle2D::~NavigationObstacle2D()
{
	ERR_FAIL_NULL(NavigationServer2D::data);

	NavigationServer2D::free_rid(obstacle);
	obstacle = RID();

#ifdef DEBUG_ENABLED
	if (debug_mesh_rid.is_valid()) {
		Renderer::free_rid(debug_mesh_rid);
		debug_mesh_rid = RID();
	}
	if (debug_canvas_item.is_valid()) {
		Renderer::free_rid(debug_canvas_item);
		debug_canvas_item = RID();
	}
#endif // DEBUG_ENABLED
}

void NavigationObstacle2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}
	map_override = p_navigation_map;
	_update_map(map_override);
}

RID NavigationObstacle2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationObstacle2D::set_avoidance_layers(uint32_t p_layers)
{
	if (avoidance_layers == p_layers) {
		return;
	}
	avoidance_layers = p_layers;
	NavigationServer2D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
}

uint32_t NavigationObstacle2D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationObstacle2D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationObstacle2D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

bool NavigationObstacle2D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationObstacle2D::set_velocity(const Vector2 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationObstacle2D::set_affect_navigation_mesh(bool p_enabled)
{
	affect_navigation_mesh = p_enabled;
}

bool NavigationObstacle2D::get_affect_navigation_mesh() const { return affect_navigation_mesh; }

void NavigationObstacle2D::set_carve_navigation_mesh(bool p_enabled)
{
	carve_navigation_mesh = p_enabled;
}

bool NavigationObstacle2D::get_carve_navigation_mesh() const { return carve_navigation_mesh; }

PackedStringArray NavigationObstacle2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	const Vector2 global_scale = get_global_scale();
	if (global_scale.x < 0.001 || global_scale.y < 0.001) {
		warnings.push_back(RTR("NavigationObstacle2D does not support negative or zero scaling."));
	}

	if (radius > 0.0 && !get_global_transform().is_conformal()) {
		warnings.push_back(
			RTR("The agent radius can only be scaled uniformly. The largest value along the two "
				"axes of the global scale will be used to scale the radius. This value may change "
				"in unexpected ways when the node is rotated."));
	}

	if (radius > 0.0 && get_global_skew() != 0.0) {
		warnings.push_back(RTR("Skew has no effect on the agent radius."));
	}

	return warnings;
}

void NavigationObstacle2D::_update_map(RID p_map)
{
	map_current = p_map;
	NavigationServer2D::obstacle_set_map(obstacle, p_map);
}

#ifdef DEBUG_ENABLED
void NavigationObstacle2D::_update_fake_agent_radius_debug()
{
	if (radius > 0.0 && NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_radius()) {
		Color debug_radius_color = NavigationServer2D::get_debug_navigation_avoidance_obstacles_radius_color();
		// Prevent non-positive scaling.
		const Vector2 safe_scale = get_global_scale().abs().maxf(0.001);
		// Agent radius is a scalar value and does not support non-uniform scaling, choose the
		// largest axis.
		const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
		Renderer::canvas_item_add_circle(debug_canvas_item, get_global_position(),
			scaling_max_value * radius, debug_radius_color);
	}
}
#endif // DEBUG_ENABLED



void NavigationObstacle2D::_update_transform() {}

#ifdef DEBUG_ENABLED
void NavigationObstacle2D::_update_static_obstacle_debug() {}
#endif // DEBUG_ENABLED


RID NavigationRegion2D::get_rid() const { return region; }

bool NavigationRegion2D::is_enabled() const { return enabled; }

void NavigationRegion2D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer2D::region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion2D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer2D::region_set_navigation_layers(region, navigation_layers);
}

uint32_t NavigationRegion2D::get_navigation_layers() const { return navigation_layers; }

void NavigationRegion2D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationRegion2D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationRegion2D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer2D::region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion2D::get_enter_cost() const { return enter_cost; }

void NavigationRegion2D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer2D::region_set_travel_cost(region, travel_cost);
}

real_t NavigationRegion2D::get_travel_cost() const { return travel_cost; }

RID NavigationRegion2D::get_region_rid() const { return get_rid(); }

#ifdef DEBUG_ENABLED
Rect2 NavigationRegion2D::_edit_get_rect() const
{
	return navigation_polygon.is_valid() ? navigation_polygon->_edit_get_rect() : Rect2();
}

bool NavigationRegion2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return navigation_polygon.is_valid()
			   ? navigation_polygon->_edit_is_selected_on_click(p_point, p_tolerance)
			   : false;
}
#endif // DEBUG_ENABLED

void NavigationRegion2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_region_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_region_update_transform();
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
#ifdef DEBUG_ENABLED
		_set_debug_visible(is_visible_in_tree());
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_region_exit_navigation_map();
#ifdef DEBUG_ENABLED
		_set_debug_visible(false);
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		if (is_inside_tree() &&
			(Engine::is_editor_hint() ||
				(NavigationServer2D::get_debug_enabled() &&
					NavigationServer2D::get_debug_navigation_enabled())) &&
			navigation_polygon.is_valid()) {
			_update_debug_mesh();
			_update_debug_edge_connections_mesh();
			_update_debug_baking_rect();
		}
#endif // DEBUG_ENABLED
	} break;
	}
}

Ref<NavigationPolygon> NavigationRegion2D::get_navigation_polygon() const
{
	return navigation_polygon;
}

void NavigationRegion2D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer2D::region_set_map(region, map_override);
}

RID NavigationRegion2D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

PackedStringArray NavigationRegion2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (is_visible_in_tree() && is_inside_tree()) {
		if (navigation_polygon.is_null()) {
			warnings.push_back(RTR("A NavigationPolygon resource must be set or created for this "
								   "node to work. Please set a property or draw a polygon."));
		}
	}

	return warnings;
}

void NavigationRegion2D::_region_exit_navigation_map()
{
	NavigationServer2D::region_set_map(region, RID());
}

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_edge_connections_mesh()
{
	bool enable_edge_connections =
		use_edge_connections && NavigationServer2D::get_debug_navigation_enable_edge_connections() &&
		NavigationServer2D::map_get_use_edge_connections(get_world_2d()->get_navigation_map());

	if (enable_edge_connections) {
		Color debug_edge_connection_color = NavigationServer2D::get_debug_navigation_edge_connection_color();
		// Draw the region edge connections.
		Transform2D xform = get_global_transform();
		real_t radius =
			NavigationServer2D::map_get_edge_connection_margin(get_world_2d()->get_navigation_map()) / 2.0;
		for (int i = 0; i < NavigationServer2D::region_get_connections_count(region); i++) {
			// Two main points
			Vector2 a = NavigationServer2D::region_get_connection_pathway_start(region, i);
			a = xform.affine_inverse().xform(a);
			Vector2 b = NavigationServer2D::region_get_connection_pathway_end(region, i);
			b = xform.affine_inverse().xform(b);
			draw_line(a, b, debug_edge_connection_color);

			// Draw a circle to illustrate the margins.
			real_t angle = a.angle_to_point(b);
			draw_arc(a, radius, angle + Math::PI / 2.0, angle - Math::PI / 2.0 + Math::TAU, 10,
				debug_edge_connection_color);
			draw_arc(b, radius, angle - Math::PI / 2.0, angle + Math::PI / 2.0, 10,
				debug_edge_connection_color);
		}
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_baking_rect()
{
	Rect2 baking_rect = get_navigation_polygon()->get_baking_rect();
	if (baking_rect.has_area()) {
		Vector2 baking_rect_offset = get_navigation_polygon()->get_baking_rect_offset();
		Rect2 debug_baking_rect = Rect2(baking_rect.position.x + baking_rect_offset.x,
			baking_rect.position.y + baking_rect_offset.y, baking_rect.size.x, baking_rect.size.y);
		Color debug_baking_rect_color = Color(0.8, 0.5, 0.7, 0.1);
		draw_rect(debug_baking_rect, debug_baking_rect_color);
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion2D::_set_debug_visible(bool p_visible)
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance_rid.is_valid()) {
		Renderer::canvas_item_set_visible(debug_instance_rid, p_visible);
	}
}
#endif // DEBUG_ENABLED

void NavigationRegion2D::_update_bounds()
{
	if (navigation_polygon.is_null()) {
		bounds = Rect2();
		return;
	}

	const Vector<Vector2>& vertices = navigation_polygon->get_vertices();
	if (vertices.is_empty()) {
		bounds = Rect2();
		return;
	}

	const Transform2D gt = is_inside_tree() ? get_global_transform() : get_transform();

	Rect2 new_bounds;
	new_bounds.position = gt.xform(vertices[0]);

	for (const Vector2& vertex : vertices) {
		new_bounds.expand_to(gt.xform(vertex));
	}
	bounds = new_bounds;
}



#ifdef DEBUG_ENABLED
void NavigationRegion2D::_update_debug_mesh() {}
#endif // DEBUG_ENABLED

void NavigationRegion2D::set_navigation_polygon(Ref<NavigationPolygon> const&) {}

void NavigationRegion2D::_region_update_transform() {}

void NavigationRegion2D::_region_enter_navigation_map() {}

// ============================================================================
// Merged from scene/resources/2d/navigation_polygon.cpp
// ============================================================================

#include <thirdparty/misc/polypartition.h>
#include "core/math/geometry_2d.h"
#include "core/os/mutex.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"

NavigationPolygon::NavigationPolygon() :
		cell_size(NavigationDefaults2D::NAV_MESH_CELL_SIZE) {
}

#ifdef DEBUG_ENABLED
Rect2 NavigationPolygon::_edit_get_rect() const
{
	RWLockRead read_lock(rwlock);
	if (rect_cache_dirty) {
		item_rect = Rect2();
		bool first = true;

		for (int i = 0; i < outlines.size(); i++) {
			const Vector<Vector2>& outline = outlines[i];
			const int outline_size = outline.size();
			if (outline_size < 3) {
				continue;
			}
			const Vector2* p = outline.ptr();
			for (int j = 0; j < outline_size; j++) {
				if (first) {
					item_rect = Rect2(p[j], Vector2(0, 0));
					first = false;
				}
				else {
					item_rect.expand_to(p[j]);
				}
			}
		}

		rect_cache_dirty = false;
	}
	return item_rect;
}

#endif // DEBUG_ENABLED

void NavigationPolygon::set_vertices(const Vector<Vector2>& p_vertices)
{
	RWLockWrite write_lock(rwlock);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	vertices = p_vertices;
	rect_cache_dirty = true;
}

const Vector<Vector2>& NavigationPolygon::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

void NavigationPolygon::add_polygon(const Vector<int>& p_polygon)
{
	RWLockWrite write_lock(rwlock);
	polygons.push_back(p_polygon);
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::add_outline_at_index(const Vector<Vector2>& p_outline, int p_index)
{
	RWLockWrite write_lock(rwlock);
	outlines.insert(p_index, p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationPolygon::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationPolygon::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(
	const Vector<Vector2>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::set_data(const Vector<Vector2>& p_vertices,
	const Vector<Vector<int>>& p_polygons, const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
	outlines = p_outlines;
	rect_cache_dirty = true;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}

void NavigationPolygon::get_data(Vector<Vector2>& r_vertices, Vector<Vector<int>>& r_polygons,
	Vector<Vector<Vector2>>& r_outlines)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
	r_outlines = outlines;
}

Ref<NavigationMesh> NavigationPolygon::get_navigation_mesh()
{
	MutexLock lock(navigation_mesh_generation);

	if (navigation_mesh.is_null()) {
		navigation_mesh.instantiate();
		Vector<Vector3> verts;
		Vector<Vector<int>> polys;
		{
			verts.resize(get_vertices().size());
			Vector3* w = verts.ptrw();

			const Vector2* r = get_vertices().ptr();

			for (int i(0); i < get_vertices().size(); i++) {
				w[i] = Vector3(r[i].x, 0.0, r[i].y);
			}
		}

		for (int i(0); i < get_polygon_count(); i++) {
			polys.push_back(get_polygon(i));
		}

		navigation_mesh->set_data(verts, polys);
		navigation_mesh->set_cell_size(
			cell_size); // Needed to not fail the cell size check on the server
	}

	return navigation_mesh;
}

void NavigationPolygon::set_outlines(const Vector<Vector<Vector2>>& p_outlines)
{
	RWLockWrite write_lock(rwlock);
	outlines = p_outlines;
	rect_cache_dirty = true;
}

Vector<Vector<Vector2>> NavigationPolygon::get_outlines() const
{
	RWLockRead read_lock(rwlock);
	return outlines;
}

void NavigationPolygon::set_polygons(const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	polygons = p_polygons;
	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
}

Vector<Vector<int>> NavigationPolygon::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

void NavigationPolygon::add_outline(const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	outlines.push_back(p_outline);
	rect_cache_dirty = true;
}

int NavigationPolygon::get_outline_count() const
{
	RWLockRead read_lock(rwlock);
	return outlines.size();
}

void NavigationPolygon::set_outline(int p_idx, const Vector<Vector2>& p_outline)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.write[p_idx] = p_outline;
	rect_cache_dirty = true;
}

void NavigationPolygon::remove_outline(int p_idx)
{
	RWLockWrite write_lock(rwlock);
	ERR_FAIL_INDEX(p_idx, outlines.size());
	outlines.remove_at(p_idx);
	rect_cache_dirty = true;
}

Vector<Vector2> NavigationPolygon::get_outline(int p_idx) const
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, outlines.size(), Vector<Vector2>());
	return outlines[p_idx];
}

void NavigationPolygon::clear_outlines()
{
	RWLockWrite write_lock(rwlock);
	outlines.clear();
	rect_cache_dirty = true;
}

#ifndef DISABLE_DEPRECATED
void NavigationPolygon::make_polygons_from_outlines()
{
	RWLockWrite write_lock(rwlock);
	WARN_PRINT("Function make_polygons_from_outlines() is deprecated."
			   "\nUse NavigationServer2D.parse_source_geometry_data() and "
			   "NavigationServer2D.bake_from_source_geometry_data() instead.");

	{
		MutexLock lock(navigation_mesh_generation);
		navigation_mesh.unref();
	}
	List<TPPLPoly> in_poly, out_poly;

	Vector2 outside_point(-1e10, -1e10);

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();
		for (int j = 0; j < olsize; j++) {
			outside_point = outside_point.max(r[j]);
		}
	}

	outside_point += Vector2(0.7239784, 0.819238); // avoid precision issues

	for (int i = 0; i < outlines.size(); i++) {
		Vector<Vector2> ol = outlines[i];
		int olsize = ol.size();
		if (olsize < 3) {
			continue;
		}
		const Vector2* r = ol.ptr();

		int interscount = 0;
		// test if this is an outer outline
		for (int k = 0; k < outlines.size(); k++) {
			if (i == k) {
				continue; // no self intersect
			}

			Vector<Vector2> ol2 = outlines[k];
			int olsize2 = ol2.size();
			if (olsize2 < 3) {
				continue;
			}
			const Vector2* r2 = ol2.ptr();

			for (int l = 0; l < olsize2; l++) {
				if (Geometry2D::segment_intersects_segment(
						r[0], outside_point, r2[l], r2[(l + 1) % olsize2], nullptr)) {
					interscount++;
				}
			}
		}

		bool outer = (interscount % 2) == 0;

		TPPLPoly tp;
		tp.Init(olsize);
		for (int j = 0; j < olsize; j++) {
			tp[j] = r[j];
		}

		if (outer) {
			tp.SetOrientation(TPPL_ORIENTATION_CCW);
		}
		else {
			tp.SetOrientation(TPPL_ORIENTATION_CW);
			tp.SetHole(true);
		}

		in_poly.push_back(tp);
	}

	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // failed!
		ERR_PRINT("NavigationPolygon: Convex partition failed! Failed to convert outlines to a "
				  "valid NavigationPolygon."
				  "\nNavigationPolygon outlines can not overlap vertices or edges inside same "
				  "outline or with other outlines or have any intersections."
				  "\nAdd the outmost and largest outline first. To add holes inside this outline "
				  "add the smaller outlines with same winding order.");
		return;
	}

	polygons.clear();
	vertices.clear();

	HashMap<Vector2, int> points;
	for (const TPPLPoly& tp : out_poly) {
		Vector<int> p;

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			HashMap<Vector2, int>::Iterator E = points.find(tp[i]);
			if (!E) {
				E = points.insert(tp[i], vertices.size());
				vertices.push_back(tp[i]);
			}
			p.push_back(E->value);
		}

		polygons.push_back(p);
	}

	emit_changed();
}
#endif // DISABLE_DEPRECATED

void NavigationPolygon::set_cell_size(real_t p_cell_size)
{
	cell_size = p_cell_size;
	get_navigation_mesh()->set_cell_size(cell_size);
}

real_t NavigationPolygon::get_cell_size() const { return cell_size; }

void NavigationPolygon::set_border_size(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0.0);
	border_size = p_value;
}

real_t NavigationPolygon::get_border_size() const { return border_size; }

void NavigationPolygon::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationPolygon::SamplePartitionType NavigationPolygon::get_sample_partition_type() const
{
	return partition_type;
}

NavigationPolygon::ParsedGeometryType NavigationPolygon::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationPolygon::set_parsed_collision_mask(uint32_t p_mask)
{
	parsed_collision_mask = p_mask;
}

uint32_t NavigationPolygon::get_parsed_collision_mask() const { return parsed_collision_mask; }

void NavigationPolygon::set_parsed_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_parsed_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_parsed_collision_mask(mask);
}

bool NavigationPolygon::get_parsed_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_parsed_collision_mask() & (1 << (p_layer_number - 1));
}

NavigationPolygon::SourceGeometryMode NavigationPolygon::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationPolygon::set_source_geometry_group_name(const String& p_group_name)
{
	source_geometry_group_name = p_group_name;
}

String NavigationPolygon::get_source_geometry_group_name() const
{
	return source_geometry_group_name;
}

void NavigationPolygon::set_agent_radius(real_t p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

real_t NavigationPolygon::get_agent_radius() const { return agent_radius; }

void NavigationPolygon::set_baking_rect(const Rect2& p_rect)
{
	baking_rect = p_rect;
	emit_changed();
}

Rect2 NavigationPolygon::get_baking_rect() const { return baking_rect; }

void NavigationPolygon::set_baking_rect_offset(const Vector2& p_rect_offset)
{
	baking_rect_offset = p_rect_offset;
	emit_changed();
}

Vector2 NavigationPolygon::get_baking_rect_offset() const { return baking_rect_offset; }



bool NavigationPolygon::_edit_is_selected_on_click(Vector2 const&, double) const { return false; }

// --- navigation_mesh_source_geometry_data_2d.cpp ---

#include "servers/navigation_2d/navigation_server_2d.h"

void NavigationMeshSourceGeometryData2D::clear()
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.clear();
	obstruction_outlines.clear();
	_projected_obstructions.clear();
	bounds_dirty = true;
}

bool NavigationMeshSourceGeometryData2D::has_data()
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines.size();
}

void NavigationMeshSourceGeometryData2D::clear_projected_obstructions()
{
	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_traversable_outlines(
	const Vector<Vector<Vector2>>& p_traversable_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::_set_obstruction_outlines(
	const Vector<Vector<Vector2>>& p_obstruction_outlines)
{
	RWLockWrite write_lock(geometry_rwlock);
	obstruction_outlines = p_obstruction_outlines;
	bounds_dirty = true;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_traversable_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return traversable_outlines;
}

const Vector<Vector<Vector2>>& NavigationMeshSourceGeometryData2D::_get_obstruction_outlines() const
{
	RWLockRead read_lock(geometry_rwlock);
	return obstruction_outlines;
}

void NavigationMeshSourceGeometryData2D::_add_traversable_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		traversable_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::_add_obstruction_outline(
	const Vector<Vector2>& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		obstruction_outlines.push_back(p_shape_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_traversable_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> traversable_outline;
		traversable_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			traversable_outline.write[i] = p_shape_outline[i];
		}
		traversable_outlines.push_back(traversable_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::add_obstruction_outline(
	const PackedVector2Array& p_shape_outline)
{
	if (p_shape_outline.size() > 1) {
		RWLockWrite write_lock(geometry_rwlock);
		Vector<Vector2> obstruction_outline;
		obstruction_outline.resize(p_shape_outline.size());
		for (int i = 0; i < p_shape_outline.size(); i++) {
			obstruction_outline.write[i] = p_shape_outline[i];
		}
		obstruction_outlines.push_back(obstruction_outline);
		bounds_dirty = true;
	}
}

void NavigationMeshSourceGeometryData2D::merge(
	const Ref<NavigationMeshSourceGeometryData2D>& p_other_geometry)
{
	ERR_FAIL_COND(p_other_geometry.is_null());

	Vector<Vector<Vector2>> other_traversable_outlines;
	Vector<Vector<Vector2>> other_obstruction_outlines;
	Vector<ProjectedObstruction> other_projected_obstructions;

	p_other_geometry->get_data(
		other_traversable_outlines, other_obstruction_outlines, other_projected_obstructions);

	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines.append_array(other_traversable_outlines);
	obstruction_outlines.append_array(other_obstruction_outlines);
	_projected_obstructions.append_array(other_projected_obstructions);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::add_projected_obstruction(
	const Vector<Vector2>& p_vertices, bool p_carve)
{
	ERR_FAIL_COND(p_vertices.size() < 2);

	ProjectedObstruction projected_obstruction;
	projected_obstruction.vertices.resize(p_vertices.size() * 2);
	projected_obstruction.carve = p_carve;

	float* obstruction_vertices_ptrw = projected_obstruction.vertices.ptrw();

	int vertex_index = 0;
	for (const Vector2& vertex : p_vertices) {
		obstruction_vertices_ptrw[vertex_index++] = vertex.x;
		obstruction_vertices_ptrw[vertex_index++] = vertex.y;
	}

	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.push_back(projected_obstruction);
	bounds_dirty = true;
}

Vector<NavigationMeshSourceGeometryData2D::ProjectedObstruction>
NavigationMeshSourceGeometryData2D::_get_projected_obstructions() const
{
	RWLockRead read_lock(geometry_rwlock);
	return _projected_obstructions;
}

void NavigationMeshSourceGeometryData2D::set_data(
	const Vector<Vector<Vector2>>& p_traversable_outlines,
	const Vector<Vector<Vector2>>& p_obstruction_outlines,
	Vector<ProjectedObstruction>& p_projected_obstructions)
{
	RWLockWrite write_lock(geometry_rwlock);
	traversable_outlines = p_traversable_outlines;
	obstruction_outlines = p_obstruction_outlines;
	_projected_obstructions = p_projected_obstructions;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData2D::get_data(Vector<Vector<Vector2>>& r_traversable_outlines,
	Vector<Vector<Vector2>>& r_obstruction_outlines,
	Vector<ProjectedObstruction>& r_projected_obstructions)
{
	RWLockRead read_lock(geometry_rwlock);
	r_traversable_outlines = traversable_outlines;
	r_obstruction_outlines = obstruction_outlines;
	r_projected_obstructions = _projected_obstructions;
}

Rect2 NavigationMeshSourceGeometryData2D::get_bounds()
{
	geometry_rwlock.read_lock();

	if (bounds_dirty) {
		geometry_rwlock.read_unlock();
		RWLockWrite write_lock(geometry_rwlock);

		bounds_dirty = false;
		bounds = Rect2();
		bool first_vertex = true;

		for (const Vector<Vector2>& traversable_outline : traversable_outlines) {
			for (const Vector2& traversable_point : traversable_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = traversable_point;
				}
				else {
					bounds.expand_to(traversable_point);
				}
			}
		}

		for (const Vector<Vector2>& obstruction_outline : obstruction_outlines) {
			for (const Vector2& obstruction_point : obstruction_outline) {
				if (first_vertex) {
					first_vertex = false;
					bounds.position = obstruction_point;
				}
				else {
					bounds.expand_to(obstruction_point);
				}
			}
		}

		for (const ProjectedObstruction& projected_obstruction : _projected_obstructions) {
			for (int i = 0; i < projected_obstruction.vertices.size() / 2; i++) {
				const Vector2 vertex = Vector2(projected_obstruction.vertices[i * 2],
					projected_obstruction.vertices[i * 2 + 1]);
				if (first_vertex) {
					first_vertex = false;
					bounds.position = vertex;
				}
				else {
					bounds.expand_to(vertex);
				}
			}
		}
	}
	else {
		geometry_rwlock.read_unlock();
	}

	RWLockRead read_lock(geometry_rwlock);
	return bounds;
}

// --- polygon_path_finder.cpp ---

#include "core/math/geometry_2d.h"
// --- Inlined from scene/resources/2d/polygon_path_finder.h ---
#include "core/io/resource.h"

class PolygonPathFinder : public Resource {

	struct Point {
		Vector2 pos;
		HashSet<int> connections;
		float distance = 0.0;
		float penalty = 0.0;
		int prev = 0;
	};

	union Edge {
		struct {
			int32_t points[2];
		};
		uint64_t key = 0;

		_FORCE_INLINE_ bool operator==(const Edge &p_edge) const {
			return key == p_edge.key;
		}
		_FORCE_INLINE_ static uint32_t hash(const Edge &p_edge) {
			return hash_one_uint64(p_edge.key);
		}

		Edge(int a = 0, int b = 0) {
			if (a > b) {
				SWAP(a, b);
			}
			points[0] = a;
			points[1] = b;
		}
	};

	Vector2 outside_point;
	Rect2 bounds;

	Vector<Point> points;
	HashSet<Edge, Edge> edges;

	bool _is_point_inside(const Vector2 &p_point) const;

public:
	void setup(const Vector<Vector2> &p_points, const Vector<int> &p_connections);
	Vector<Vector2> find_path(const Vector2 &p_from, const Vector2 &p_to);

	void set_point_penalty(int p_point, float p_penalty);
	float get_point_penalty(int p_point) const;

	bool is_point_inside(const Vector2 &p_point) const;
	Vector2 get_closest_point(const Vector2 &p_point) const;
	Vector<Vector2> get_intersections(const Vector2 &p_from, const Vector2 &p_to) const;
	Rect2 get_bounds() const;

	PolygonPathFinder();
};

// --- End scene/resources/2d/polygon_path_finder.h ---

bool PolygonPathFinder::_is_point_inside(const Vector2& p_point) const
{
	int crosses = 0;

	for (const Edge& E : edges) {
		const Edge& e = E;

		Vector2 a = points[e.points[0]].pos;
		Vector2 b = points[e.points[1]].pos;

		if (Geometry2D::segment_intersects_segment(a, b, p_point, outside_point, nullptr)) {
			crosses++;
		}
	}

	return crosses & 1;
}

void PolygonPathFinder::setup(const Vector<Vector2>& p_points, const Vector<int>& p_connections)
{
	ERR_FAIL_COND(p_connections.size() & 1);

	points.clear();
	edges.clear();

	// insert points

	int point_count = p_points.size();
	points.resize(point_count + 2);
	bounds = Rect2();

	for (int i = 0; i < p_points.size(); i++) {
		points.write[i].pos = p_points[i];
		points.write[i].penalty = 0;

		outside_point = i == 0 ? p_points[0] : p_points[i].max(outside_point);

		if (i == 0) {
			bounds.position = points[i].pos;
		}
		else {
			bounds.expand_to(points[i].pos);
		}
	}

	outside_point.x += 20.451 + Math::randf() * 10.2039;
	outside_point.y += 21.193 + Math::randf() * 12.5412;

	// insert edges (which are also connections)

	for (int i = 0; i < p_connections.size(); i += 2) {
		Edge e(p_connections[i], p_connections[i + 1]);
		ERR_FAIL_INDEX(e.points[0], point_count);
		ERR_FAIL_INDEX(e.points[1], point_count);
		points.write[p_connections[i]].connections.insert(p_connections[i + 1]);
		points.write[p_connections[i + 1]].connections.insert(p_connections[i]);
		edges.insert(e);
	}

	// fill the remaining connections based on visibility

	for (int i = 0; i < point_count; i++) {
		for (int j = i + 1; j < point_count; j++) {
			if (edges.has(Edge(i, j))) {
				continue; // if in edge ignore
			}

			Vector2 from = points[i].pos;
			Vector2 to = points[j].pos;

			if (!_is_point_inside(
					from * 0.5 + to * 0.5)) { // connection between points in inside space
				continue;
			}

			bool valid = true;

			for (const Edge& E : edges) {
				const Edge& e = E;
				if (e.points[0] == i || e.points[1] == i || e.points[0] == j || e.points[1] == j) {
					continue;
				}

				Vector2 a = points[e.points[0]].pos;
				Vector2 b = points[e.points[1]].pos;

				if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
					valid = false;
					break;
				}
			}

			if (valid) {
				points.write[i].connections.insert(j);
				points.write[j].connections.insert(i);
			}
		}
	}
}

Vector<Vector2> PolygonPathFinder::find_path(const Vector2& p_from, const Vector2& p_to)
{
	Vector<Vector2> path;

	Vector2 from = p_from;
	Vector2 to = p_to;
	Edge ignore_from_edge(-1, -1);
	Edge ignore_to_edge(-1, -1);

	if (!_is_point_inside(from)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(from, segment_a, segment_b);
			float d = from.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_from_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		from = closest_point;
	};

	if (!_is_point_inside(to)) {
		float closest_dist = 1e20f;
		Vector2 closest_point;

		for (const Edge& E : edges) {
			const Edge& e = E;
			const Vector2 segment_a = points[e.points[0]].pos;
			const Vector2 segment_b = points[e.points[1]].pos;
			Vector2 closest = Geometry2D::get_closest_point_to_segment(to, segment_a, segment_b);
			float d = to.distance_squared_to(closest);

			if (d < closest_dist) {
				ignore_to_edge = E;
				closest_dist = d;
				closest_point = closest;
			}
		}

		to = closest_point;
	};

	// test direct connection
	{
		bool can_see_eachother = true;

		for (const Edge& E : edges) {
			const Edge& e = E;
			if (e.points[0] == ignore_from_edge.points[0] &&
				e.points[1] == ignore_from_edge.points[1]) {
				continue;
			}
			if (e.points[0] == ignore_to_edge.points[0] &&
				e.points[1] == ignore_to_edge.points[1]) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (Geometry2D::segment_intersects_segment(a, b, from, to, nullptr)) {
				can_see_eachother = false;
				break;
			}
		}

		if (can_see_eachother) {
			path.push_back(from);
			path.push_back(to);
			return path;
		}
	}

	// add to graph

	int aidx = points.size() - 2;
	int bidx = points.size() - 1;
	points.write[aidx].pos = from;
	points.write[bidx].pos = to;
	points.write[aidx].distance = 0;
	points.write[bidx].distance = 0;
	points.write[aidx].prev = -1;
	points.write[bidx].prev = -1;
	points.write[aidx].penalty = 0;
	points.write[bidx].penalty = 0;

	for (int i = 0; i < points.size() - 2; i++) {
		bool valid_a = true;
		bool valid_b = true;
		points.write[i].prev = -1;
		points.write[i].distance = 0;

		if (!_is_point_inside(from * 0.5 + points[i].pos * 0.5)) {
			valid_a = false;
		}

		if (!_is_point_inside(to * 0.5 + points[i].pos * 0.5)) {
			valid_b = false;
		}

		for (const Edge& E : edges) {
			const Edge& e = E;

			if (e.points[0] == i || e.points[1] == i) {
				continue;
			}

			Vector2 a = points[e.points[0]].pos;
			Vector2 b = points[e.points[1]].pos;

			if (valid_a) {
				if (e.points[0] != ignore_from_edge.points[1] &&
					e.points[1] != ignore_from_edge.points[1] &&
					e.points[0] != ignore_from_edge.points[0] &&
					e.points[1] != ignore_from_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(
							a, b, from, points[i].pos, nullptr)) {
						valid_a = false;
					}
				}
			}

			if (valid_b) {
				if (e.points[0] != ignore_to_edge.points[1] &&
					e.points[1] != ignore_to_edge.points[1] &&
					e.points[0] != ignore_to_edge.points[0] &&
					e.points[1] != ignore_to_edge.points[0]) {
					if (Geometry2D::segment_intersects_segment(a, b, to, points[i].pos, nullptr)) {
						valid_b = false;
					}
				}
			}

			if (!valid_a && !valid_b) {
				break;
			}
		}

		if (valid_a) {
			points.write[i].connections.insert(aidx);
			points.write[aidx].connections.insert(i);
		}

		if (valid_b) {
			points.write[i].connections.insert(bidx);
			points.write[bidx].connections.insert(i);
		}
	}
	// solve graph

	HashSet<int> open_list;

	points.write[aidx].distance = 0;
	points.write[aidx].prev = aidx;
	for (const int& E : points[aidx].connections) {
		open_list.insert(E);
		points.write[E].distance = from.distance_to(points[E].pos);
		points.write[E].prev = aidx;
	}

	bool found_route = false;

	while (true) {
		if (open_list.is_empty()) {
			print_verbose("Open list empty.");
			break;
		}
		// check open list

		int least_cost_point = -1;
		float least_cost = 1e30;

		// this could be faster (cache previous results)
		for (const int& E : open_list) {
			const Point& p = points[E];
			float cost = p.distance;
			cost += p.pos.distance_to(to);
			cost += p.penalty;

			if (cost < least_cost) {
				least_cost_point = E;
				least_cost = cost;
			}
		}

		const Point& np = points[least_cost_point];
		// open the neighbors for search

		for (const int& E : np.connections) {
			Point& p = points.write[E];
			float distance = np.pos.distance_to(p.pos) + np.distance;

			if (p.prev != -1) {
				// oh this was visited already, can we win the cost?

				if (p.distance > distance) {
					p.prev = least_cost_point; // reassign previous
					p.distance = distance;
				}
			}
			else {
				// add to open neighbors

				p.prev = least_cost_point;
				p.distance = distance;
				open_list.insert(E);

				if (E == bidx) {
					// oh my reached end! stop algorithm
					found_route = true;
					break;
				}
			}
		}

		if (found_route) {
			break;
		}

		open_list.erase(least_cost_point);
	}

	if (found_route) {
		int at = bidx;
		path.push_back(points[at].pos);
		do {
			at = points[at].prev;
			path.push_back(points[at].pos);
		} while (at != aidx);

		path.reverse();
	}

	for (int i = 0; i < points.size() - 2; i++) {
		points.write[i].connections.erase(aidx);
		points.write[i].connections.erase(bidx);
		points.write[i].prev = -1;
		points.write[i].distance = 0;
	}

	points.write[aidx].connections.clear();
	points.write[aidx].prev = -1;
	points.write[aidx].distance = 0;
	points.write[bidx].connections.clear();
	points.write[bidx].prev = -1;
	points.write[bidx].distance = 0;

	return path;
}

bool PolygonPathFinder::is_point_inside(const Vector2& p_point) const
{
	return _is_point_inside(p_point);
}

Vector2 PolygonPathFinder::get_closest_point(const Vector2& p_point) const
{
	float closest_dist = 1e20f;
	Vector2 closest_point;

	for (const Edge& E : edges) {
		const Edge& e = E;
		const Vector2 segment_a = points[e.points[0]].pos;
		const Vector2 segment_b = points[e.points[1]].pos;
		Vector2 closest = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
		float d = p_point.distance_squared_to(closest);

		if (d < closest_dist) {
			closest_dist = d;
			closest_point = closest;
		}
	}

	ERR_FAIL_COND_V(Math::is_equal_approx(closest_dist, 1e20f), Vector2());

	return closest_point;
}

Vector<Vector2> PolygonPathFinder::get_intersections(
	const Vector2& p_from, const Vector2& p_to) const
{
	Vector<Vector2> inters;

	for (const Edge& E : edges) {
		Vector2 a = points[E.points[0]].pos;
		Vector2 b = points[E.points[1]].pos;

		Vector2 res;
		if (Geometry2D::segment_intersects_segment(a, b, p_from, p_to, &res)) {
			inters.push_back(res);
		}
	}

	return inters;
}

Rect2 PolygonPathFinder::get_bounds() const { return bounds; }

void PolygonPathFinder::set_point_penalty(int p_point, float p_penalty)
{
	ERR_FAIL_INDEX(p_point, points.size() - 2);
	points.write[p_point].penalty = p_penalty;
}

float PolygonPathFinder::get_point_penalty(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, points.size() - 2, 0);
	return points[p_point].penalty;
}

PolygonPathFinder::PolygonPathFinder() {}
#endif // !defined(NAVIGATION_2D_DISABLED)
