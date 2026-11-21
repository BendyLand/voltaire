/**************************************************************************/
/*  line_2d.cpp                                                           */
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
#include "path_2d.h"
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
#include "parallax_2d.h"
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
#include "parallax_layer.h"
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

#include "light_2d.h"

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
#include "light_occluder_2d.h"
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
