/**************************************************************************/
/*  shape_2d.h                                                            */
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

#pragma once

#include "core/io/resource.h"

class Shape2D : public Resource
{
public:
	enum ShapeType
	{
		SHAPE_WORLD_BOUNDARY,
		SHAPE_SEPARATION_RAY,
		SHAPE_SEGMENT,
		SHAPE_CIRCLE,
		SHAPE_RECTANGLE,
		SHAPE_CAPSULE,
		SHAPE_CONVEX_POLYGON,
		SHAPE_CONCAVE_POLYGON,
	};

private:
	ShapeType shape_type = SHAPE_RECTANGLE;
	RID shape;
	real_t custom_bias = 0.0;

	// World boundary parameters
	Vector2 normal = Vector2(0, -1);
	real_t distance = 0.0;

	// Separation ray parameters
	real_t length = 20.0;
	bool slide_on_slope = false;

	// Segment parameters
	Vector2 a;
	Vector2 b = Vector2(0, 10);

	// Circle / Capsule parameters
	real_t radius = 10.0;
	real_t height = 30.0;

	// Rectangle parameters
	Size2 size = Size2(20, 20);

	// Convex polygon parameters
	Vector<Vector2> points;

	// Concave polygon parameters
	Vector<Vector2> segments;

	static RID _create_shape_rid(ShapeType p_type);
	void _update_shape();
	Vector<Vector2> _get_capsule_points() const;

public:
	ShapeType get_shape_type() const { return shape_type; }
	void set_shape_type(ShapeType p_type);

	// World boundary
	void set_normal(const Vector2& p_normal);
	Vector2 get_normal() const;
	void set_distance(real_t p_distance);
	real_t get_distance() const;

	// Separation ray
	void set_length(real_t p_length);
	real_t get_length() const;
	void set_slide_on_slope(bool p_active);
	bool get_slide_on_slope() const;

	// Segment
	void set_a(const Vector2& p_a);
	Vector2 get_a() const;
	void set_b(const Vector2& p_b);
	Vector2 get_b() const;

	// Circle / Capsule
	void set_radius(real_t p_radius);
	real_t get_radius() const;
	void set_height(real_t p_height);
	real_t get_height() const;
	void set_mid_height(real_t p_mid_height);
	real_t get_mid_height() const;

	// Rectangle
	void set_size(const Size2& p_size);
	Size2 get_size() const;

	// Convex polygon
	void set_point_cloud(const Vector<Vector2>& p_points);
	void set_points(const Vector<Vector2>& p_points);
	Vector<Vector2> get_points() const;

	// Concave polygon
	void set_segments(const Vector<Vector2>& p_segments);
	Vector<Vector2> get_segments() const;

	bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;

	void set_custom_solver_bias(real_t p_bias);
	real_t get_custom_solver_bias() const;

	bool collide_with_motion(const Transform2D& p_local_xform, const Vector2& p_local_motion,
		Shape2D* rp_shape, const Transform2D& p_shape_xform,
		const Vector2& p_shape_motion);
	bool collide(const Transform2D& p_local_xform, Shape2D* rp_shape,
		const Transform2D& p_shape_xform);

	PackedVector2Array collide_with_motion_and_get_contacts(const Transform2D& p_local_xform,
		const Vector2& p_local_motion, Shape2D* rp_shape,
		const Transform2D& p_shape_xform, const Vector2& p_shape_motion);
	PackedVector2Array collide_and_get_contacts(const Transform2D& p_local_xform,
		Shape2D* rp_shape, const Transform2D& p_shape_xform);

	void draw(const RID& p_to_rid, const Color& p_color);
	Rect2 get_rect() const;

	/// Returns the radius of a circle that fully encloses this shape.
	real_t get_enclosing_radius() const;
	virtual RID get_rid() const override;

	static bool is_collision_outline_enabled();

	Shape2D(ShapeType p_type = SHAPE_RECTANGLE);
	~Shape2D();
};
