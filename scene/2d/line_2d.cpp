/**************************************************************************/
/*  line_2d.cpp                                                           */
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

#include "core/math/geometry_2d.h"
#include "line_2d.h"
#include "scene/2d/line_builder.h"
#include "servers/rendering/rendering_server.h"

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

void Line2D::set_points(const Vector<Vector2>& p_points)
{
	_points = p_points;
	queue_redraw();
}

void Line2D::set_closed(bool p_closed)
{
	_closed = p_closed;
	queue_redraw();
}

bool Line2D::is_closed() const { return _closed; }

void Line2D::set_width(float p_width)
{
	if (p_width < 0.0) {
		p_width = 0.0;
	}
	_width = p_width;
	queue_redraw();
}

float Line2D::get_width() const { return _width; }

void Line2D::set_curve(const Ref<Curve>& p_curve)
{
	if (_curve.is_valid()) {
		_curve->disconnect_changed(callable_mp(this, &Line2D::_curve_changed));
	}

	_curve = p_curve;

	if (_curve.is_valid()) {
		_curve->connect_changed(callable_mp(this, &Line2D::_curve_changed));
	}

	queue_redraw();
}

Ref<Curve> Line2D::get_curve() const { return _curve; }

Vector<Vector2> Line2D::get_points() const { return _points; }

void Line2D::set_point_position(int i, Vector2 p_pos)
{
	ERR_FAIL_INDEX(i, _points.size());
	_points.set(i, p_pos);
	queue_redraw();
}

Vector2 Line2D::get_point_position(int i) const
{
	ERR_FAIL_INDEX_V(i, _points.size(), Vector2());
	return _points.get(i);
}

int Line2D::get_point_count() const { return _points.size(); }

void Line2D::clear_points()
{
	int count = _points.size();
	if (count > 0) {
		_points.clear();
		queue_redraw();
	}
}

void Line2D::add_point(Vector2 p_pos, int p_atpos)
{
	if (p_atpos < 0 || _points.size() < p_atpos) {
		_points.push_back(p_pos);
	}
	else {
		_points.insert(p_atpos, p_pos);
	}
	queue_redraw();
}

void Line2D::remove_point(int i)
{
	_points.remove_at(i);
	queue_redraw();
}

void Line2D::set_default_color(Color p_color)
{
	_default_color = p_color;
	queue_redraw();
}

Color Line2D::get_default_color() const { return _default_color; }

void Line2D::set_gradient(const Ref<Gradient>& p_gradient)
{
	if (_gradient.is_valid()) {
		_gradient->disconnect_changed(callable_mp(this, &Line2D::_gradient_changed));
	}

	_gradient = p_gradient;

	if (_gradient.is_valid()) {
		_gradient->connect_changed(callable_mp(this, &Line2D::_gradient_changed));
	}

	queue_redraw();
}

Ref<Gradient> Line2D::get_gradient() const { return _gradient; }

void Line2D::set_texture(const Ref<Texture2D>& p_texture)
{
	_texture = p_texture;
	queue_redraw();
}

Ref<Texture2D> Line2D::get_texture() const { return _texture; }

void Line2D::set_texture_mode(const LineTextureMode p_mode)
{
	_texture_mode = p_mode;
	queue_redraw();
}

Line2D::LineTextureMode Line2D::get_texture_mode() const { return _texture_mode; }

void Line2D::set_joint_mode(LineJointMode p_mode)
{
	_joint_mode = p_mode;
	queue_redraw();
}

Line2D::LineJointMode Line2D::get_joint_mode() const { return _joint_mode; }

void Line2D::set_begin_cap_mode(LineCapMode p_mode)
{
	_begin_cap_mode = p_mode;
	queue_redraw();
}

Line2D::LineCapMode Line2D::get_begin_cap_mode() const { return _begin_cap_mode; }

void Line2D::set_end_cap_mode(LineCapMode p_mode)
{
	_end_cap_mode = p_mode;
	queue_redraw();
}

Line2D::LineCapMode Line2D::get_end_cap_mode() const { return _end_cap_mode; }

void Line2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_DRAW: {
		_draw();
	} break;
	}
}

void Line2D::set_sharp_limit(float p_limit)
{
	if (p_limit < 0.f) {
		p_limit = 0.f;
	}
	_sharp_limit = p_limit;
	queue_redraw();
}

float Line2D::get_sharp_limit() const { return _sharp_limit; }

void Line2D::set_round_precision(int p_precision)
{
	_round_precision = MAX(1, p_precision);
	queue_redraw();
}

int Line2D::get_round_precision() const { return _round_precision; }

void Line2D::set_antialiased(bool p_antialiased)
{
	_antialiased = p_antialiased;
	queue_redraw();
}

bool Line2D::get_antialiased() const { return _antialiased; }

void Line2D::_draw()
{
	int len = _points.size();
	if (len <= 1 || _width == 0.f) {
		return;
	}

	// TODO Maybe have it as member rather than copying parameters and allocating memory?
	LineBuilder lb;
	lb.points = _points;
	lb.closed = _closed;
	lb.default_color = _default_color;
	lb.gradient = *_gradient;
	lb.texture_mode = _texture_mode;
	lb.joint_mode = _joint_mode;
	lb.begin_cap_mode = _begin_cap_mode;
	lb.end_cap_mode = _end_cap_mode;
	lb.round_precision = _round_precision;
	lb.sharp_limit = _sharp_limit;
	lb.width = _width;
	lb.curve = *_curve;

	RID texture_rid;
	if (_texture.is_valid()) {
		texture_rid = _texture->get_scaled_rid();

		lb.tile_aspect = _texture->get_size().aspect();
	}

	lb.build();
	if (lb.indices.is_empty()) {
		return;
	}

	RS::get_singleton()->canvas_item_add_triangle_array(get_canvas_item(), lb.indices, lb.vertices,
		lb.colors, lb.uvs, Vector<int>(), Vector<float>(), texture_rid);

	// DEBUG: Draw wireframe
	//	if (lb.indices.size() % 3 == 0) {
	//		Color col(0, 0, 0);
	//		for (int i = 0; i < lb.indices.size(); i += 3) {
	//			Vector2 a = lb.vertices[lb.indices[i]];
	//			Vector2 b = lb.vertices[lb.indices[i+1]];
	//			Vector2 c = lb.vertices[lb.indices[i+2]];
	//			draw_line(a, b, col);
	//			draw_line(b, c, col);
	//			draw_line(c, a, col);
	//		}
	//		for (int i = 0; i < lb.vertices.size(); ++i) {
	//			Vector2 p = lb.vertices[i];
	//			draw_rect(Rect2(p.x - 1, p.y - 1, 2, 2), Color(0, 0, 0, 0.5));
	//		}
	//	}
}

void Line2D::_gradient_changed() { queue_redraw(); }

void Line2D::_curve_changed() { queue_redraw(); }

void Line2D::_bind_methods() {}


