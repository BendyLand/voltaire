/**************************************************************************/
/*  godot_shape_2d.cpp                                                    */
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
#include "core/templates/sort_array.h"
#include "godot_objects_2d.h"
#include "godot_shape_2d.h"

void GodotShape2D::configure(const Rect2& p_aabb)
{
	aabb = p_aabb;
	configured = true;
	for (const KeyValue<GodotCollisionObject2D*, int>& E : owners) {
		E.key->_shape_changed();
	}
}

Vector2 GodotShape2D::get_support(const Vector2& p_normal) const
{
	Vector2 res[2];
	int amnt;
	get_supports(p_normal, res, amnt);
	return res[0];
}

void GodotShape2D::add_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	if (E) {
		E->value++;
	}
	else {
		owners[p_owner] = 1;
	}
}

void GodotShape2D::remove_owner(GodotCollisionObject2D* p_owner)
{
	HashMap<GodotCollisionObject2D*, int>::Iterator E = owners.find(p_owner);
	ERR_FAIL_COND(!E);
	E->value--;
	if (E->value == 0) {
		owners.remove(E);
	}
}

bool GodotShape2D::is_owner(GodotCollisionObject2D* p_owner) const { return owners.has(p_owner); }

const HashMap<GodotCollisionObject2D*, int>& GodotShape2D::get_owners() const { return owners; }

GodotShape2D::~GodotShape2D()
{
	ERR_FAIL_COND(owners.size());
	if (convex_points) {
		memdelete_arr(convex_points);
	}
}

void GodotShape2D::set_world_boundary(const Vector2& p_normal, real_t p_d)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_WORLD_BOUNDARY);
	wb_normal = p_normal;
	wb_d = p_d;
	configure(Rect2(Vector2(-1e15, -1e15), Vector2(1e15 * 2.0, 1e15 * 2.0)));
}

void GodotShape2D::set_separation_ray(real_t p_length, bool p_slide_on_slope)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEPARATION_RAY);
	ray_length = p_length;
	slide_on_slope = p_slide_on_slope;
	configure(Rect2(0, 0, 0.001, MAX(ray_length, (real_t)0.1)));
}

void GodotShape2D::set_segment(const Vector2& p_a, const Vector2& p_b)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_SEGMENT);
	seg_a = p_a;
	seg_b = p_b;
	seg_n = (seg_b - seg_a).orthogonal().normalized();

	Rect2 r(seg_a, Size2());
	r.expand_to(seg_b);
	if (r.size.x == 0) {
		r.size.x = 0.001;
	}
	if (r.size.y == 0) {
		r.size.y = 0.001;
	}
	configure(r);
}

void GodotShape2D::set_circle(real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CIRCLE);
	radius = p_radius;
	configure(Rect2(-radius, -radius, radius * 2.0, radius * 2.0));
}

void GodotShape2D::set_rectangle(const Vector2& p_half_extents)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_RECTANGLE);
	half_extents = p_half_extents;
	configure(Rect2(-half_extents, half_extents * 2.0));
}

void GodotShape2D::set_capsule(real_t p_height, real_t p_radius)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CAPSULE);
	height = p_height;
	radius = p_radius;
	if (height < radius * 2.0) {
		height = radius * 2.0;
	}
	configure(Rect2(Point2(-radius, -height * 0.5), Size2(radius * 2.0, height)));
}

void GodotShape2D::set_convex_polygon(const Vector<Vector2>& p_points)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONVEX_POLYGON);
	if (convex_points) {
		memdelete_arr(convex_points);
		convex_points = nullptr;
	}
	convex_point_count = 0;

	int count = p_points.size();
	ERR_FAIL_COND(count < 3);

	Vector<Vector2> hull = Geometry2D::convex_hull(p_points);
	// convex_hull duplicates the first point at the end in counter-clockwise order.
	if (hull.size() > 1 && hull[0] == hull[hull.size() - 1]) {
		hull.resize(hull.size() - 1);
	}
	ERR_FAIL_COND(hull.size() < 3);

	convex_point_count = hull.size();
	convex_points = memnew_arr(ConvexPoint, convex_point_count);

	Rect2 r(hull[0], Size2());
	for (int i = 0; i < convex_point_count; i++) {
		convex_points[i].pos = hull[i];
		r.expand_to(hull[i]);
	}
	for (int i = 0; i < convex_point_count; i++) {
		Vector2 next = convex_points[(i + 1) % convex_point_count].pos;
		convex_points[i].normal = (next - convex_points[i].pos).orthogonal().normalized();
	}
	configure(r);
}

void GodotShape2D::set_concave_polygon(const Vector<Vector2>& p_segments)
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);
	int len = p_segments.size();
	ERR_FAIL_COND(len % 2 != 0);

	concave_segments.clear();
	concave_points.clear();
	concave_bvh.clear();
	concave_bvh_depth = 0;

	if (len == 0) {
		configure(Rect2());
		return;
	}

	const Vector2* arr = p_segments.ptr();
	HashMap<Point2, int> pointmap;
	concave_segments.resize(len / 2);

	Rect2 global_aabb(arr[0], Size2());
	Vector<BVH> bvh_list;
	bvh_list.resize(len / 2);

	for (int i = 0; i < len / 2; i++) {
		Point2 a = arr[i * 2 + 0];
		Point2 b = arr[i * 2 + 1];

		int idx_a;
		HashMap<Point2, int>::Iterator E_a = pointmap.find(a);
		if (E_a) {
			idx_a = E_a->value;
		}
		else {
			idx_a = pointmap.size();
			pointmap[a] = idx_a;
		}

		int idx_b;
		HashMap<Point2, int>::Iterator E_b = pointmap.find(b);
		if (E_b) {
			idx_b = E_b->value;
		}
		else {
			idx_b = pointmap.size();
			pointmap[b] = idx_b;
		}

		concave_segments.write[i].points[0] = idx_a;
		concave_segments.write[i].points[1] = idx_b;

		Rect2 s_aabb(a, Size2());
		s_aabb.expand_to(b);
		global_aabb.expand_to(a);
		global_aabb.expand_to(b);

		bvh_list.write[i].aabb = s_aabb;
		bvh_list.write[i].left = -1;
		bvh_list.write[i].right = i;
	}

	concave_points.resize(pointmap.size());
	for (const KeyValue<Point2, int>& E : pointmap) {
		concave_points.write[E.value] = E.key;
	}

	concave_bvh_depth = 1;
	_generate_bvh(bvh_list.ptrw(), bvh_list.size(), 1);
	configure(global_aabb);
}

void GodotShape2D::project_range(
	const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_t<PS2DE::SHAPE_WORLD_BOUNDARY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_t<PS2DE::SHAPE_SEPARATION_RAY>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_t<PS2DE::SHAPE_SEGMENT>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_t<PS2DE::SHAPE_CIRCLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_t<PS2DE::SHAPE_RECTANGLE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_t<PS2DE::SHAPE_CAPSULE>(p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_t<PS2DE::SHAPE_CONVEX_POLYGON>(p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range in GodotShape2D");
	}
}

void GodotShape2D::project_range_cast(const Vector2& p_cast, const Vector2& p_normal,
	const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
		project_range_cast_t<PS2DE::SHAPE_WORLD_BOUNDARY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEPARATION_RAY:
		project_range_cast_t<PS2DE::SHAPE_SEPARATION_RAY>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_SEGMENT:
		project_range_cast_t<PS2DE::SHAPE_SEGMENT>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CIRCLE:
		project_range_cast_t<PS2DE::SHAPE_CIRCLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_RECTANGLE:
		project_range_cast_t<PS2DE::SHAPE_RECTANGLE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CAPSULE:
		project_range_cast_t<PS2DE::SHAPE_CAPSULE>(p_cast, p_normal, p_transform, r_min, r_max);
		break;
	case PS2DE::SHAPE_CONVEX_POLYGON:
		project_range_cast_t<PS2DE::SHAPE_CONVEX_POLYGON>(
			p_cast, p_normal, p_transform, r_min, r_max);
		break;
	default:
		r_min = 0;
		r_max = 0;
		ERR_FAIL_MSG("Unsupported call to project_range_cast in GodotShape2D");
	}
}

void GodotShape2D::get_supports(const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		r_amount = 0;
	} break;
	case PS2DE::SHAPE_SEPARATION_RAY: {
		r_amount = 1;
		if (p_normal.y > 0) {
			*r_supports = Vector2(0, ray_length);
		}
		else {
			*r_supports = Vector2();
		}
	} break;
	case PS2DE::SHAPE_SEGMENT: {
		if (Math::abs(p_normal.dot(seg_n)) > segment_is_valid_support_threshold) {
			r_supports[0] = seg_a;
			r_supports[1] = seg_b;
			r_amount = 2;
			return;
		}

		real_t dp = p_normal.dot(seg_b - seg_a);
		if (dp > 0) {
			*r_supports = seg_b;
		}
		else {
			*r_supports = seg_a;
		}
		r_amount = 1;
	} break;
	case PS2DE::SHAPE_CIRCLE: {
		r_amount = 1;
		*r_supports = p_normal * radius;
	} break;
	case PS2DE::SHAPE_RECTANGLE: {
		for (int i = 0; i < 2; i++) {
			Vector2 ag;
			ag[i] = 1.0;
			real_t dp = ag.dot(p_normal);
			if (Math::abs(dp) <= segment_is_valid_support_threshold) {
				continue;
			}

			real_t sgn = dp > 0 ? 1.0 : -1.0;

			r_amount = 2;

			r_supports[0][i] = half_extents[i] * sgn;
			r_supports[0][i ^ 1] = half_extents[i ^ 1];

			r_supports[1][i] = half_extents[i] * sgn;
			r_supports[1][i ^ 1] = -half_extents[i ^ 1];

			return;
		}

		r_amount = 1;
		r_supports[0] = Vector2((p_normal.x < 0) ? -half_extents.x : half_extents.x,
			(p_normal.y < 0) ? -half_extents.y : half_extents.y);
	} break;
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 n = p_normal;

		real_t h = height * 0.5 - radius; // half-height of the rectangle part

		if (h > 0 && Math::abs(n.x) > segment_is_valid_support_threshold) {
			// make it flat
			n.y = 0.0;
			n.x = SIGN(n.x) * radius;

			r_amount = 2;
			r_supports[0] = n;
			r_supports[0].y += h;
			r_supports[1] = n;
			r_supports[1].y -= h;
		}
		else {
			n *= radius;
			n.y += (n.y > 0) ? h : -h;
			r_amount = 1;
			*r_supports = n;
		}
	} break;
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		int support_idx = -1;
		real_t d = -1e10;
		r_amount = 0;

		for (int i = 0; i < convex_point_count; i++) {
			// test point
			real_t ld = p_normal.dot(convex_points[i].pos);
			if (ld > d) {
				support_idx = i;
				d = ld;
			}

			// test segment
			if (convex_points[i].normal.dot(p_normal) > segment_is_valid_support_threshold) {
				r_amount = 2;
				r_supports[0] = convex_points[i].pos;
				r_supports[1] = convex_points[(i + 1) % convex_point_count].pos;
				return;
			}
		}

		ERR_FAIL_COND_MSG(support_idx == -1, "Convex polygon shape support not found.");

		r_amount = 1;
		r_supports[0] = convex_points[support_idx].pos;
	} break;
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		real_t d = -1e10;
		int idx = -1;
		for (int i = 0; i < concave_points.size(); i++) {
			real_t ld = p_normal.dot(concave_points[i]);
			if (ld > d) {
				d = ld;
				idx = i;
			}
		}

		r_amount = 1;
		ERR_FAIL_COND(idx == -1);
		*r_supports = concave_points[idx];
	} break;
	default: {
		r_amount = 0;
	} break;
	}
}

bool GodotShape2D::contains_point(const Vector2& p_point) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		return wb_normal.dot(p_point) < wb_d;
	}
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_SEGMENT:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return false;
	}
	case PS2DE::SHAPE_CIRCLE: {
		return p_point.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		real_t x = p_point.x;
		real_t y = p_point.y;
		real_t edge_x = half_extents.x;
		real_t edge_y = half_extents.y;
		return (x >= -edge_x) && (x < edge_x) && (y >= -edge_y) && (y < edge_y);
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 p = p_point;
		p.y = Math::abs(p.y);
		p.y -= height * 0.5 - radius;
		if (p.y < 0) {
			p.y = 0;
		}

		return p.length_squared() < radius * radius;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		bool out = false;
		bool in = false;

		for (int i = 0; i < convex_point_count; i++) {
			real_t d = convex_points[i].normal.dot(p_point) -
					   convex_points[i].normal.dot(convex_points[i].pos);
			if (d > 0) {
				out = true;
			}
			else {
				in = true;
			}
		}

		return in != out;
	}
	default:
		return false;
	}
}

bool GodotShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY: {
		Vector2 segment = p_begin - p_end;
		real_t den = wb_normal.dot(segment);

		if (Math::abs(den) <= CMP_EPSILON) {
			return false;
		}

		real_t dist = (wb_normal.dot(p_begin) - wb_d) / den;

		if (dist < -CMP_EPSILON || dist > (1.0 + CMP_EPSILON)) {
			return false;
		}

		r_point = p_begin + segment * -dist;
		r_normal = wb_normal;

		return true;
	}
	case PS2DE::SHAPE_SEPARATION_RAY: {
		return false; // rays can't be intersected
	}
	case PS2DE::SHAPE_SEGMENT: {
		if (!Geometry2D::segment_intersects_segment(p_begin, p_end, seg_a, seg_b, &r_point)) {
			return false;
		}

		if (seg_n.dot(p_begin) > seg_n.dot(seg_a)) {
			r_normal = seg_n;
		}
		else {
			r_normal = -seg_n;
		}

		return true;
	}
	case PS2DE::SHAPE_CIRCLE: {
		Vector2 line_vec = p_end - p_begin;

		real_t a = line_vec.dot(line_vec);
		real_t b = 2 * p_begin.dot(line_vec);
		real_t c = p_begin.dot(p_begin) - radius * radius;

		real_t sqrtterm = b * b - 4 * a * c;

		if (sqrtterm < 0) {
			return false;
		}
		sqrtterm = Math::sqrt(sqrtterm);
		real_t res = (-b - sqrtterm) / (2 * a);

		if (res < 0 || res > 1 + CMP_EPSILON) {
			return false;
		}

		r_point = p_begin + line_vec * res;
		r_normal = r_point.normalized();
		return true;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		return get_aabb().intersects_segment(p_begin, p_end, &r_point, &r_normal);
	}
	case PS2DE::SHAPE_CAPSULE: {
		real_t d = 1e10;
		Vector2 n = (p_end - p_begin).normalized();
		bool collided = false;

		// try spheres
		for (int i = 0; i < 2; i++) {
			Vector2 begin = p_begin;
			Vector2 end = p_end;
			real_t ofs = (i == 0) ? -height * 0.5 + radius : height * 0.5 - radius;
			begin.y += ofs;
			end.y += ofs;

			Vector2 line_vec = end - begin;

			real_t a = line_vec.dot(line_vec);
			real_t b = 2 * begin.dot(line_vec);
			real_t c = begin.dot(begin) - radius * radius;

			real_t sqrtterm = b * b - 4 * a * c;

			if (sqrtterm < 0) {
				continue;
			}

			sqrtterm = Math::sqrt(sqrtterm);
			real_t res = (-b - sqrtterm) / (2 * a);

			if (res < 0 || res > 1 + CMP_EPSILON) {
				continue;
			}

			Vector2 point = begin + line_vec * res;
			Vector2 pointf(point.x, point.y - ofs);
			real_t pd = n.dot(pointf);
			if (pd < d) {
				r_point = pointf;
				r_normal = point.normalized();
				d = pd;
				collided = true;
			}
		}

		Vector2 rpos, rnorm;
		if (Rect2(Point2(-radius, -height * 0.5 + radius), Size2(radius * 2.0, height - radius * 2))
				.intersects_segment(p_begin, p_end, &rpos, &rnorm)) {
			real_t pd = n.dot(rpos);
			if (pd < d) {
				r_point = rpos;
				r_normal = rnorm;
				d = pd;
				collided = true;
			}
		}

		return collided;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		for (int i = 0; i < convex_point_count; i++) {
			Vector2 res;

			if (!Geometry2D::segment_intersects_segment(p_begin, p_end, convex_points[i].pos,
					convex_points[(i + 1) % convex_point_count].pos, &res)) {
				continue;
			}

			real_t nd = n.dot(res);
			if (nd < d) {
				d = nd;
				r_point = res;
				r_normal = convex_points[i].normal;
				inters = true;
			}
		}

		return inters;
	}
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
			return false;
		}

		uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

		enum
		{
			TEST_AABB_BIT = 0,
			VISIT_LEFT_BIT = 1,
			VISIT_RIGHT_BIT = 2,
			VISIT_DONE_BIT = 3,
			VISITED_BIT_SHIFT = 29,
			NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
			VISITED_BIT_MASK = ~NODE_IDX_MASK,
		};

		Vector2 n = (p_end - p_begin).normalized();
		real_t d = 1e10;
		bool inters = false;

		int level = 0;

		const ConcaveSegment* segmentptr = &concave_segments[0];
		const Vector2* pointptr = &concave_points[0];
		const BVH* bvhptr = &concave_bvh[0];

		stack[0] = 0;
		while (true) {
			uint32_t node = stack[level] & NODE_IDX_MASK;
			const BVH& bvh2 = bvhptr[node];
			bool done = false;

			switch (stack[level] >> VISITED_BIT_SHIFT) {
			case TEST_AABB_BIT: {
				bool valid = bvh2.aabb.intersects_segment(p_begin, p_end);
				if (!valid) {
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					if (bvh2.left < 0) {
						const ConcaveSegment& s = segmentptr[bvh2.right];
						Vector2 a = pointptr[s.points[0]];
						Vector2 b = pointptr[s.points[1]];

						Vector2 res;

						if (Geometry2D::segment_intersects_segment(p_begin, p_end, a, b, &res)) {
							real_t nd = n.dot(res);
							if (nd < d) {
								d = nd;
								r_point = res;
								r_normal = (b - a).orthogonal().normalized();
								inters = true;
							}
						}

						stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
					}
					else {
						stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
					}
				}
			}
				continue;
			case VISIT_LEFT_BIT: {
				stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.left | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_RIGHT_BIT: {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				stack[level + 1] = bvh2.right | TEST_AABB_BIT;
				level++;
			}
				continue;
			case VISIT_DONE_BIT: {
				if (level == 0) {
					done = true;
					break;
				}
				else {
					level--;
				}
			}
				continue;
			}

			if (done) {
				break;
			}
		}

		if (inters) {
			if (n.dot(r_normal) > 0) {
				r_normal = -r_normal;
			}
		}

		return inters;
	}
	default:
		return false;
	}
}

real_t GodotShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	switch (type) {
	case PS2DE::SHAPE_WORLD_BOUNDARY:
	case PS2DE::SHAPE_SEPARATION_RAY:
	case PS2DE::SHAPE_CONCAVE_POLYGON: {
		return 0;
	}
	case PS2DE::SHAPE_SEGMENT: {
		return p_mass * ((seg_a * p_scale).distance_squared_to(seg_b * p_scale)) / 12;
	}
	case PS2DE::SHAPE_CIRCLE: {
		real_t a = radius * p_scale.x;
		real_t b = radius * p_scale.y;
		return p_mass * (a * a + b * b) / 4;
	}
	case PS2DE::SHAPE_RECTANGLE: {
		Vector2 he2 = half_extents * 2 * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CAPSULE: {
		Vector2 he2 = Vector2(radius * 2, height) * p_scale;
		return p_mass * he2.dot(he2) / 12.0;
	}
	case PS2DE::SHAPE_CONVEX_POLYGON: {
		ERR_FAIL_COND_V_MSG(convex_point_count == 0, 0, "Convex polygon shape has no points.");
		Rect2 aabb_new;
		aabb_new.position = convex_points[0].pos * p_scale;
		for (int i = 0; i < convex_point_count; i++) {
			aabb_new.expand_to(convex_points[i].pos * p_scale);
		}

		return p_mass * aabb_new.size.dot(aabb_new.size) / 12.0;
	}
	default:
		return 0;
	}
}

int GodotShape2D::_generate_bvh(BVH* p_bvh, int p_len, int p_depth)
{
	if (p_len == 1) {
		concave_bvh_depth = MAX(p_depth, concave_bvh_depth);
		concave_bvh.push_back(*p_bvh);
		return concave_bvh.size() - 1;
	}

	Rect2 global_aabb = p_bvh[0].aabb;
	for (int i = 1; i < p_len; i++) {
		global_aabb = global_aabb.merge(p_bvh[i].aabb);
	}

	if (global_aabb.size.x > global_aabb.size.y) {
		SortArray<BVH, BVH_CompareX> sort;
		sort.sort(p_bvh, p_len);
	}
	else {
		SortArray<BVH, BVH_CompareY> sort;
		sort.sort(p_bvh, p_len);
	}

	int median = p_len / 2;

	BVH node;
	node.aabb = global_aabb;
	int node_idx = concave_bvh.size();
	concave_bvh.push_back(node);

	int l = _generate_bvh(p_bvh, median, p_depth + 1);
	int r = _generate_bvh(&p_bvh[median], p_len - median, p_depth + 1);
	concave_bvh.write[node_idx].left = l;
	concave_bvh.write[node_idx].right = r;

	return node_idx;
}

void GodotShape2D::cull(
	const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const
{
	ERR_FAIL_COND(type != PS2DE::SHAPE_CONCAVE_POLYGON);

	if (concave_segments.is_empty() || concave_points.is_empty() || concave_bvh.is_empty()) {
		return;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * concave_bvh_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,
	};

	int level = 0;

	const ConcaveSegment* segmentptr = &concave_segments[0];
	const Vector2* pointptr = &concave_points[0];
	const BVH* bvhptr = &concave_bvh[0];

	stack[0] = 0;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& bvh2 = bvhptr[node];

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			bool valid = p_local_aabb.intersects(bvh2.aabb);
			if (!valid) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (bvh2.left < 0) {
					const ConcaveSegment& s = segmentptr[bvh2.right];
					Vector2 a = pointptr[s.points[0]];
					Vector2 b = pointptr[s.points[1]];

					GodotShape2D ss(a, b, (b - a).orthogonal().normalized());

					if (p_callback(p_userdata, &ss)) {
						return;
					}
					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
		}
			continue;
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.left | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			stack[level + 1] = bvh2.right | TEST_AABB_BIT;
			level++;
		}
			continue;
		case VISIT_DONE_BIT: {
			if (level == 0) {
				return;
			}
			else {
				level--;
			}
		}
			continue;
		}
	}
}
