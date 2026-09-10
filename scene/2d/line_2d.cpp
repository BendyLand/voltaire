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


