/**************************************************************************/
/*  shape_2d.cpp                                                          */
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

#include "shape_2d.h"

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
