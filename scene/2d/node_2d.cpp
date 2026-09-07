/**************************************************************************/
/*  node_2d.cpp                                                           */
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

#include "node_2d.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"

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


