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
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
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

// ============================================================================
// Merged from modules/godot_physics_2d/godot_collision_solver_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_shape_2d.h"

#include "core/math/geometry_2d.h"
#include "core/templates/sort_array.h"

struct _CollectorCallback2D {
	GodotCollisionSolver2D::CallbackResult callback = nullptr;
	void *userdata = nullptr;
	bool swap = false;
	bool collided = false;
	Vector2 normal;
	Vector2 *sep_axis = nullptr;

	_FORCE_INLINE_ void call(const Vector2 &p_point_A, const Vector2 &p_point_B) {
		if (swap) {
			callback(p_point_B, p_point_A, userdata);
		} else {
			callback(p_point_A, p_point_B, userdata);
		}
	}
};

typedef void (*GenerateContactsFunc)(const Vector2 *, int, const Vector2 *, int, _CollectorCallback2D *);

_FORCE_INLINE_ static void _generate_contacts_point_point(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 1);
#endif

	p_collector->call(*p_points_A, *p_points_B);
}

_FORCE_INLINE_ static void _generate_contacts_point_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 1);
	ERR_FAIL_COND(p_point_count_B != 2);
#endif

	Vector2 closest_B = Geometry2D::get_closest_point_to_segment_uncapped(*p_points_A, p_points_B[0], p_points_B[1]);
	p_collector->call(*p_points_A, closest_B);
}

struct _generate_contacts_Pair {
	bool a = false;
	int idx = 0;
	real_t d = 0.0;
	_FORCE_INLINE_ bool operator<(const _generate_contacts_Pair &l) const { return d < l.d; }
};

_FORCE_INLINE_ static void _generate_contacts_edge_edge(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A != 2);
	ERR_FAIL_COND(p_point_count_B != 2); // circle is actually a 4x3 matrix
#endif

	Vector2 n = p_collector->normal;
	Vector2 t = n.orthogonal();
	real_t dA = n.dot(p_points_A[0]);
	real_t dB = n.dot(p_points_B[0]);

	_generate_contacts_Pair dvec[4];

	dvec[0].d = t.dot(p_points_A[0]);
	dvec[0].a = true;
	dvec[0].idx = 0;
	dvec[1].d = t.dot(p_points_A[1]);
	dvec[1].a = true;
	dvec[1].idx = 1;
	dvec[2].d = t.dot(p_points_B[0]);
	dvec[2].a = false;
	dvec[2].idx = 0;
	dvec[3].d = t.dot(p_points_B[1]);
	dvec[3].a = false;
	dvec[3].idx = 1;

	SortArray<_generate_contacts_Pair> sa;
	sa.sort(dvec, 4);

	for (int i = 1; i <= 2; i++) {
		if (dvec[i].a) {
			Vector2 a = p_points_A[dvec[i].idx];
			Vector2 b = n.plane_project(dB, a);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		} else {
			Vector2 b = p_points_B[dvec[i].idx];
			Vector2 a = n.plane_project(dA, b);
			if (n.dot(a) > n.dot(b) - CMP_EPSILON) {
				continue;
			}
			p_collector->call(a, b);
		}
	}
}

static void _generate_contacts_from_supports(const Vector2 *p_points_A, int p_point_count_A, const Vector2 *p_points_B, int p_point_count_B, _CollectorCallback2D *p_collector) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(p_point_count_A < 1);
	ERR_FAIL_COND(p_point_count_B < 1);
#endif

	static const GenerateContactsFunc generate_contacts_func_table[2][2] = {
		{
				_generate_contacts_point_point,
				_generate_contacts_point_edge,
		},
		{
				nullptr,
				_generate_contacts_edge_edge,
		}
	};

	int pointcount_B = 0;
	int pointcount_A = 0;
	const Vector2 *points_A = nullptr;
	const Vector2 *points_B = nullptr;

	if (p_point_count_A > p_point_count_B) {
		//swap
		p_collector->swap = !p_collector->swap;
		p_collector->normal = -p_collector->normal;

		pointcount_B = p_point_count_A;
		pointcount_A = p_point_count_B;
		points_A = p_points_B;
		points_B = p_points_A;
	} else {
		pointcount_B = p_point_count_B;
		pointcount_A = p_point_count_A;
		points_A = p_points_A;
		points_B = p_points_B;
	}

	int version_A = (pointcount_A > 2 ? 2 : pointcount_A) - 1;
	int version_B = (pointcount_B > 2 ? 2 : pointcount_B) - 1;

	GenerateContactsFunc contacts_func = generate_contacts_func_table[version_A][version_B];
	ERR_FAIL_NULL(contacts_func);
	contacts_func(points_A, pointcount_A, points_B, pointcount_B, p_collector);
}

template <PS2DE::ShapeType typeA, PS2DE::ShapeType typeB, bool castA = false, bool castB = false, bool withMargin = false>
class SeparatorAxisTest2D {
	const GodotShape2D *shape_A = nullptr;
	const GodotShape2D *shape_B = nullptr;
	const Transform2D *transform_A = nullptr;
	const Transform2D *transform_B = nullptr;
	real_t best_depth = 1e15;
	Vector2 best_axis;
#ifdef DEBUG_ENABLED
	int best_axis_count = 0;
	int best_axis_index = -1;
#endif
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	_CollectorCallback2D *callback;

public:
	_FORCE_INLINE_ bool test_previous_axis() {
		if (callback && callback->sep_axis && *callback->sep_axis != Vector2()) {
			return test_axis(*callback->sep_axis);
		} else {
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif
		}
		return true;
	}

	_FORCE_INLINE_ bool test_cast() {
		if (castA) {
			Vector2 na = motion_A.normalized();
			if (!test_axis(na)) {
				return false;
			}
			if (!test_axis(na.orthogonal())) {
				return false;
			}
		}

		if (castB) {
			Vector2 nb = motion_B.normalized();
			if (!test_axis(nb)) {
				return false;
			}
			if (!test_axis(nb.orthogonal())) {
				return false;
			}
		}

		return true;
	}

	_FORCE_INLINE_ bool test_axis(const Vector2 &p_axis) {
		Vector2 axis = p_axis;

		if (Math::is_zero_approx(axis.x) &&
				Math::is_zero_approx(axis.y)) {
			// strange case, try an upwards separator
			axis = Vector2(0.0, 1.0);
		}

		real_t min_A = 0.0, max_A = 0.0, min_B = 0.0, max_B = 0.0;

		if constexpr (castA) {
			shape_A->project_range_cast_t<typeA>(motion_A, axis, *transform_A, min_A, max_A);
		} else {
			shape_A->project_range_t<typeA>(axis, *transform_A, min_A, max_A);
		}

		if constexpr (castB) {
			shape_B->project_range_cast_t<typeB>(motion_B, axis, *transform_B, min_B, max_B);
		} else {
			shape_B->project_range_t<typeB>(axis, *transform_B, min_B, max_B);
		}

		if (withMargin) {
			min_A -= margin_A;
			max_A += margin_A;
			min_B -= margin_B;
			max_B += margin_B;
		}

		min_B -= (max_A - min_A) * 0.5;
		max_B += (max_A - min_A) * 0.5;

		real_t dmin = min_B - (min_A + max_A) * 0.5;
		real_t dmax = max_B - (min_A + max_A) * 0.5;

		if (dmin > 0.0 || dmax < 0.0) {
			if (callback && callback->sep_axis) {
				*callback->sep_axis = axis;
			}
#ifdef DEBUG_ENABLED
			best_axis_count++;
#endif

			return false; // doesn't contain 0
		}

		//use the smallest depth

		dmin = Math::abs(dmin);

		if (dmax < dmin) {
			if (dmax < best_depth) {
				best_depth = dmax;
				best_axis = axis;
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		} else {
			if (dmin < best_depth) {
				best_depth = dmin;
				best_axis = -axis; // keep it as A axis
#ifdef DEBUG_ENABLED
				best_axis_index = best_axis_count;
#endif
			}
		}

#ifdef DEBUG_ENABLED
		best_axis_count++;
#endif

		return true;
	}

	_FORCE_INLINE_ void generate_contacts() {
		// nothing to do, don't generate
		if (best_axis == Vector2(0.0, 0.0)) {
			return;
		}

		if (callback) {
			callback->collided = true;

			if (!callback->callback) {
				return; //only collide, no callback
			}
		}
		static const int max_supports = 2;

		Vector2 supports_A[max_supports];
		int support_count_A;
		if (castA) {
			shape_A->get_supports_transformed_cast(motion_A, -best_axis, *transform_A, supports_A, support_count_A);
		} else {
			shape_A->get_supports(transform_A->basis_xform_inv(-best_axis).normalized(), supports_A, support_count_A);
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] = transform_A->xform(supports_A[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_A; i++) {
				supports_A[i] += -best_axis * margin_A;
			}
		}

		Vector2 supports_B[max_supports];
		int support_count_B;
		if (castB) {
			shape_B->get_supports_transformed_cast(motion_B, best_axis, *transform_B, supports_B, support_count_B);
		} else {
			shape_B->get_supports(transform_B->basis_xform_inv(best_axis).normalized(), supports_B, support_count_B);
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] = transform_B->xform(supports_B[i]);
			}
		}

		if (withMargin) {
			for (int i = 0; i < support_count_B; i++) {
				supports_B[i] += best_axis * margin_B;
			}
		}
		if (callback) {
			callback->normal = best_axis;
			_generate_contacts_from_supports(supports_A, support_count_A, supports_B, support_count_B, callback);

			if (callback->sep_axis && *callback->sep_axis != Vector2()) {
				*callback->sep_axis = Vector2(); //invalidate previous axis (no test)
			}
		}
	}

	_FORCE_INLINE_ SeparatorAxisTest2D(const GodotShape2D *p_shape_A, const Transform2D &p_transform_a, const GodotShape2D *p_shape_B, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_A = Vector2(), const Vector2 &p_motion_B = Vector2(), real_t p_margin_A = 0, real_t p_margin_B = 0) {
		margin_A = p_margin_A;
		margin_B = p_margin_B;
		shape_A = p_shape_A;
		shape_B = p_shape_B;
		transform_A = &p_transform_a;
		transform_B = &p_transform_b;
		motion_A = p_motion_A;
		motion_B = p_motion_B;
		callback = p_collector;
	}
};

/****** SAT TESTS *******/

#define TEST_POINT(m_a, m_b) \
	((!separator.test_axis(((m_a) - (m_b)).normalized())) || \
			(castA && !separator.test_axis(((m_a) + p_motion_a - (m_b)).normalized())) || \
			(castB && !separator.test_axis(((m_a) - ((m_b) + p_motion_b)).normalized())) || \
			(castA && castB && !separator.test_axis(((m_a) + p_motion_a - ((m_b) + p_motion_b)).normalized())))

typedef void (*CollisionFunc)(const GodotShape2D *, const Transform2D &, const GodotShape2D *, const Transform2D &, _CollectorCallback2D *p_collector, const Vector2 &, const Vector2 &, real_t, real_t);

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_segment(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *segment_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_SEGMENT, castA, castB, withMargin> separator(segment_A, p_transform_a, segment_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}
	//this collision is kind of pointless

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}
	if (!separator.test_axis(segment_B->get_xformed_normal(p_transform_b))) {
		return;
	}

	if (withMargin) {
		//points grow to circles

		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_a()))) {
			return;
		}
		if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(segment_B->get_b()))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(segment_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//segment normal
	if (!separator.test_axis(
				(p_transform_a.xform(segment_A->get_b()) - p_transform_a.xform(segment_A->get_a())).normalized().orthogonal())) {
		return;
	}

	//endpoint a vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.get_origin())) {
		return;
	}
	//endpoint b vs circle
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(segment_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if (withMargin) {
		Transform2D inv = p_transform_b.affine_inverse();

		Vector2 a = p_transform_a.xform(segment_A->get_a());
		Vector2 b = p_transform_a.xform(segment_A->get_b());

		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a))) {
			return;
		}
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b))) {
			return;
		}

		if constexpr (castA) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b + p_motion_a))) {
				return;
			}
		}

		if constexpr (castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, a - p_motion_b + p_motion_a))) {
				return;
			}
			if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, inv, b - p_motion_b + p_motion_a))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(segment_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_segment_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *segment_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_SEGMENT, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(segment_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (!separator.test_axis(segment_A->get_xformed_normal(p_transform_a))) {
		return;
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if (withMargin) {
			if (TEST_POINT(p_transform_a.xform(segment_A->get_a()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
			if (TEST_POINT(p_transform_a.xform(segment_A->get_b()), p_transform_b.xform(convex_B->get_point(i)))) {
				return;
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_circle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *circle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CIRCLE, castA, castB, withMargin> separator(circle_A, p_transform_a, circle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.get_origin())) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(circle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	const Vector2 &sphere = p_transform_a.columns[2];
	const Vector2 *axis = &p_transform_b.columns[0];
	//const Vector2& half_extents = rectangle_B->get_half_extents();

	if (!separator.test_axis(axis[0].normalized())) {
		return;
	}

	if (!separator.test_axis(axis[1].normalized())) {
		return;
	}

	Transform2D binv = p_transform_b.affine_inverse();
	{
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphere))) {
			return;
		}
	}

	if constexpr (castA) {
		Vector2 sphereofs = sphere + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castB) {
		Vector2 sphereofs = sphere - p_motion_b;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	if constexpr (castA && castB) {
		Vector2 sphereofs = sphere - p_motion_b + p_motion_a;
		if (!separator.test_axis(rectangle_B->get_circle_axis(p_transform_b, binv, sphereofs))) {
			return;
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(circle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	//capsule endpoints
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir))) {
		return;
	}
	if (TEST_POINT(p_transform_a.get_origin(), (p_transform_b.get_origin() - p_transform_b.columns[1] * capsule_dir))) {
		return;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_circle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *circle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CIRCLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(circle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//poly faces and poly points vs circle
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (TEST_POINT(p_transform_a.get_origin(), p_transform_b.xform(convex_B->get_point(i)))) {
			return;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_rectangle(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *rectangle_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_RECTANGLE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, rectangle_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces A
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//box faces B
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_b.columns[1].normalized())) {
		return;
	}

	if constexpr (withMargin) {
		Transform2D invA = p_transform_a.affine_inverse();
		Transform2D invB = p_transform_b.affine_inverse();

		if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, p_transform_b, invB))) {
			return;
		}

		if constexpr (castA || castB) {
			Transform2D aofs = p_transform_a;
			aofs.columns[2] += p_motion_a;

			Transform2D bofs = p_transform_b;
			bofs.columns[2] += p_motion_b;

			[[maybe_unused]] Transform2D aofsinv = aofs.affine_inverse();
			[[maybe_unused]] Transform2D bofsinv = bofs.affine_inverse();

			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, p_transform_b, invB))) {
					return;
				}
			}

			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(p_transform_a, invA, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}

			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_box_axis(aofs, aofsinv, rectangle_B, bofs, bofsinv))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(rectangle_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//capsule axis
	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	//box endpoints to capsule circles

	Transform2D boxinv = p_transform_a.affine_inverse();

	real_t capsule_dir = capsule_B->get_height() * 0.5 - capsule_B->get_radius();

	for (int i = 0; i < 2; i++) {
		{
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		if constexpr (castA && castB) {
			Vector2 capsule_endpoint = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir;
			capsule_endpoint -= p_motion_a;
			capsule_endpoint += p_motion_b;

			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, capsule_endpoint))) {
				return;
			}
		}

		capsule_dir *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_rectangle_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *rectangle_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_RECTANGLE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(rectangle_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//box faces
	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[1].normalized())) {
		return;
	}

	//convex faces
	Transform2D boxinv;
	if constexpr (withMargin) {
		boxinv = p_transform_a.affine_inverse();
	}
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}

		if constexpr (withMargin) {
			//all points vs all points need to be tested if margin exist
			if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i))))) {
				return;
			}
			if constexpr (castA) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) - p_motion_a))) {
					return;
				}
			}
			if constexpr (castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b))) {
					return;
				}
			}
			if constexpr (castA && castB) {
				if (!separator.test_axis(rectangle_A->get_circle_axis(p_transform_a, boxinv, p_transform_b.xform(convex_B->get_point(i)) + p_motion_b - p_motion_a))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_capsule(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *capsule_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CAPSULE, castA, castB, withMargin> separator(capsule_A, p_transform_a, capsule_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_b.columns[0].normalized())) {
		return;
	}

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//capsule endpoints

	real_t capsule_dir_A = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
	for (int i = 0; i < 2; i++) {
		Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir_A;

		real_t capsule_dir_B = capsule_B->get_height() * 0.5 - capsule_B->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_B = p_transform_b.get_origin() + p_transform_b.columns[1] * capsule_dir_B;

			if (TEST_POINT(capsule_endpoint_A, capsule_endpoint_B)) {
				return;
			}

			capsule_dir_B *= -1.0;
		}

		capsule_dir_A *= -1.0;
	}

	separator.generate_contacts();
}

template <bool castA, bool castB, bool withMargin>
static void _collision_capsule_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *capsule_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CAPSULE, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(capsule_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	//capsule axis

	if (!separator.test_axis(p_transform_a.columns[0].normalized())) {
		return;
	}

	//poly vs capsule
	for (int i = 0; i < convex_B->get_point_count(); i++) {
		Vector2 cpoint = p_transform_b.xform(convex_B->get_point(i));

		real_t capsule_dir = capsule_A->get_height() * 0.5 - capsule_A->get_radius();
		for (int j = 0; j < 2; j++) {
			Vector2 capsule_endpoint_A = p_transform_a.get_origin() + p_transform_a.columns[1] * capsule_dir;

			if (TEST_POINT(capsule_endpoint_A, cpoint)) {
				return;
			}

			capsule_dir *= -1.0;
		}

		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	separator.generate_contacts();
}

/////////

template <bool castA, bool castB, bool withMargin>
static void _collision_convex_polygon_convex_polygon(const GodotShape2D *p_a, const Transform2D &p_transform_a, const GodotShape2D *p_b, const Transform2D &p_transform_b, _CollectorCallback2D *p_collector, const Vector2 &p_motion_a, const Vector2 &p_motion_b, real_t p_margin_A, real_t p_margin_B) {
	const GodotShape2D *convex_A = p_a;
	const GodotShape2D *convex_B = p_b;

	SeparatorAxisTest2D<PS2DE::SHAPE_CONVEX_POLYGON, PS2DE::SHAPE_CONVEX_POLYGON, castA, castB, withMargin> separator(convex_A, p_transform_a, convex_B, p_transform_b, p_collector, p_motion_a, p_motion_b, p_margin_A, p_margin_B);

	if (!separator.test_previous_axis()) {
		return;
	}

	if (!separator.test_cast()) {
		return;
	}

	for (int i = 0; i < convex_A->get_point_count(); i++) {
		if (!separator.test_axis(convex_A->get_xformed_segment_normal(p_transform_a, i))) {
			return;
		}
	}

	for (int i = 0; i < convex_B->get_point_count(); i++) {
		if (!separator.test_axis(convex_B->get_xformed_segment_normal(p_transform_b, i))) {
			return;
		}
	}

	if (withMargin) {
		for (int i = 0; i < convex_A->get_point_count(); i++) {
			for (int j = 0; j < convex_B->get_point_count(); j++) {
				if (TEST_POINT(p_transform_a.xform(convex_A->get_point(i)), p_transform_b.xform(convex_B->get_point(j)))) {
					return;
				}
			}
		}
	}

	separator.generate_contacts();
}

////////

bool sat_2d_calculate_penetration(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, GodotCollisionSolver2D::CallbackResult p_result_callback, void *p_userdata, bool p_swap, Vector2 *sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();

	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_A == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_A->is_concave(), false);

	PS2DE::ShapeType type_B = p_shape_B->get_type();

	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_WORLD_BOUNDARY, false);
	ERR_FAIL_COND_V(type_B == PS2DE::SHAPE_SEPARATION_RAY, false);
	ERR_FAIL_COND_V(p_shape_B->is_concave(), false);

	static const CollisionFunc collision_table[5][5] = {
		{ _collision_segment_segment<false, false, false>,
				_collision_segment_circle<false, false, false>,
				_collision_segment_rectangle<false, false, false>,
				_collision_segment_capsule<false, false, false>,
				_collision_segment_convex_polygon<false, false, false> },
		{ nullptr,
				_collision_circle_circle<false, false, false>,
				_collision_circle_rectangle<false, false, false>,
				_collision_circle_capsule<false, false, false>,
				_collision_circle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, false>,
				_collision_rectangle_capsule<false, false, false>,
				_collision_rectangle_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, false>,
				_collision_capsule_convex_polygon<false, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, false> }

	};

	static const CollisionFunc collision_table_castA[5][5] = {
		{ _collision_segment_segment<true, false, false>,
				_collision_segment_circle<true, false, false>,
				_collision_segment_rectangle<true, false, false>,
				_collision_segment_capsule<true, false, false>,
				_collision_segment_convex_polygon<true, false, false> },
		{ nullptr,
				_collision_circle_circle<true, false, false>,
				_collision_circle_rectangle<true, false, false>,
				_collision_circle_capsule<true, false, false>,
				_collision_circle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, false>,
				_collision_rectangle_capsule<true, false, false>,
				_collision_rectangle_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, false>,
				_collision_capsule_convex_polygon<true, false, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, false> }

	};

	static const CollisionFunc collision_table_castB[5][5] = {
		{ _collision_segment_segment<false, true, false>,
				_collision_segment_circle<false, true, false>,
				_collision_segment_rectangle<false, true, false>,
				_collision_segment_capsule<false, true, false>,
				_collision_segment_convex_polygon<false, true, false> },
		{ nullptr,
				_collision_circle_circle<false, true, false>,
				_collision_circle_rectangle<false, true, false>,
				_collision_circle_capsule<false, true, false>,
				_collision_circle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, false>,
				_collision_rectangle_capsule<false, true, false>,
				_collision_rectangle_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, false>,
				_collision_capsule_convex_polygon<false, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, false> }

	};

	static const CollisionFunc collision_table_castA_castB[5][5] = {
		{ _collision_segment_segment<true, true, false>,
				_collision_segment_circle<true, true, false>,
				_collision_segment_rectangle<true, true, false>,
				_collision_segment_capsule<true, true, false>,
				_collision_segment_convex_polygon<true, true, false> },
		{ nullptr,
				_collision_circle_circle<true, true, false>,
				_collision_circle_rectangle<true, true, false>,
				_collision_circle_capsule<true, true, false>,
				_collision_circle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, false>,
				_collision_rectangle_capsule<true, true, false>,
				_collision_rectangle_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, false>,
				_collision_capsule_convex_polygon<true, true, false> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, false> }

	};

	static const CollisionFunc collision_table_margin[5][5] = {
		{ _collision_segment_segment<false, false, true>,
				_collision_segment_circle<false, false, true>,
				_collision_segment_rectangle<false, false, true>,
				_collision_segment_capsule<false, false, true>,
				_collision_segment_convex_polygon<false, false, true> },
		{ nullptr,
				_collision_circle_circle<false, false, true>,
				_collision_circle_rectangle<false, false, true>,
				_collision_circle_capsule<false, false, true>,
				_collision_circle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, false, true>,
				_collision_rectangle_capsule<false, false, true>,
				_collision_rectangle_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, false, true>,
				_collision_capsule_convex_polygon<false, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, false, true> }

	};

	static const CollisionFunc collision_table_castA_margin[5][5] = {
		{ _collision_segment_segment<true, false, true>,
				_collision_segment_circle<true, false, true>,
				_collision_segment_rectangle<true, false, true>,
				_collision_segment_capsule<true, false, true>,
				_collision_segment_convex_polygon<true, false, true> },
		{ nullptr,
				_collision_circle_circle<true, false, true>,
				_collision_circle_rectangle<true, false, true>,
				_collision_circle_capsule<true, false, true>,
				_collision_circle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, false, true>,
				_collision_rectangle_capsule<true, false, true>,
				_collision_rectangle_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, false, true>,
				_collision_capsule_convex_polygon<true, false, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, false, true> }

	};

	static const CollisionFunc collision_table_castB_margin[5][5] = {
		{ _collision_segment_segment<false, true, true>,
				_collision_segment_circle<false, true, true>,
				_collision_segment_rectangle<false, true, true>,
				_collision_segment_capsule<false, true, true>,
				_collision_segment_convex_polygon<false, true, true> },
		{ nullptr,
				_collision_circle_circle<false, true, true>,
				_collision_circle_rectangle<false, true, true>,
				_collision_circle_capsule<false, true, true>,
				_collision_circle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<false, true, true>,
				_collision_rectangle_capsule<false, true, true>,
				_collision_rectangle_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<false, true, true>,
				_collision_capsule_convex_polygon<false, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<false, true, true> }

	};

	static const CollisionFunc collision_table_castA_castB_margin[5][5] = {
		{ _collision_segment_segment<true, true, true>,
				_collision_segment_circle<true, true, true>,
				_collision_segment_rectangle<true, true, true>,
				_collision_segment_capsule<true, true, true>,
				_collision_segment_convex_polygon<true, true, true> },
		{ nullptr,
				_collision_circle_circle<true, true, true>,
				_collision_circle_rectangle<true, true, true>,
				_collision_circle_capsule<true, true, true>,
				_collision_circle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				_collision_rectangle_rectangle<true, true, true>,
				_collision_rectangle_capsule<true, true, true>,
				_collision_rectangle_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				_collision_capsule_capsule<true, true, true>,
				_collision_capsule_convex_polygon<true, true, true> },
		{ nullptr,
				nullptr,
				nullptr,
				nullptr,
				_collision_convex_polygon_convex_polygon<true, true, true> }

	};

	_CollectorCallback2D callback;
	callback.callback = p_result_callback;
	callback.swap = p_swap;
	callback.userdata = p_userdata;
	callback.collided = false;
	callback.sep_axis = sep_axis;

	const GodotShape2D *A = p_shape_A;
	const GodotShape2D *B = p_shape_B;
	const Transform2D *transform_A = &p_transform_A;
	const Transform2D *transform_B = &p_transform_B;
	const Vector2 *motion_A = &p_motion_A;
	const Vector2 *motion_B = &p_motion_B;
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	if (type_A > type_B) {
		SWAP(A, B);
		SWAP(transform_A, transform_B);
		SWAP(type_A, type_B);
		SWAP(motion_A, motion_B);
		SWAP(margin_A, margin_B);
		callback.swap = !callback.swap;
	}

	CollisionFunc collision_func;

	if (p_margin_A || p_margin_B) {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_margin[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA_margin[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB_margin[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB_margin[type_A - 2][type_B - 2];
		}
	} else {
		if (*motion_A == Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table[type_A - 2][type_B - 2];
		} else if (*motion_A != Vector2() && *motion_B == Vector2()) {
			collision_func = collision_table_castA[type_A - 2][type_B - 2];
		} else if (*motion_A == Vector2() && *motion_B != Vector2()) {
			collision_func = collision_table_castB[type_A - 2][type_B - 2];
		} else {
			collision_func = collision_table_castA_castB[type_A - 2][type_B - 2];
		}
	}

	ERR_FAIL_NULL_V(collision_func, false);

	collision_func(A, *transform_A, B, *transform_B, &callback, *motion_A, *motion_B, margin_A, margin_B);

	return callback.collided;
}

#define collision_solver sat_2d_calculate_penetration
//#define collision_solver gjk_epa_calculate_penetration

bool GodotCollisionSolver2D::solve_static_world_boundary(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_WORLD_BOUNDARY) {
		return false;
	}

	Vector2 n = p_transform_A.basis_xform(p_shape_A->get_world_boundary_normal()).normalized();
	Vector2 p = p_transform_A.xform(p_shape_A->get_world_boundary_normal() * p_shape_A->get_world_boundary_d());
	real_t d = n.dot(p);

	Vector2 supports[2];
	int support_count;

	p_shape_B->get_supports(p_transform_B.affine_inverse().basis_xform(-n).normalized(), supports, support_count);

	bool found = false;

	for (int i = 0; i < support_count; i++) {
		supports[i] += p_margin * supports[i].normalized();
		supports[i] = p_transform_B.xform(supports[i]);
		supports[i] += p_motion_B;
		real_t pd = n.dot(supports[i]);
		if (pd >= d) {
			continue;
		}
		found = true;

		Vector2 support_A = supports[i] - n * (pd - d);

		if (p_result_callback) {
			if (p_swap_result) {
				p_result_callback(supports[i], support_A, p_userdata);
			} else {
				p_result_callback(support_A, supports[i], p_userdata);
			}
		}
	}

	return found;
}

bool GodotCollisionSolver2D::solve_separation_ray(const GodotShape2D *p_shape_A, const Vector2 &p_motion_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin) {
	if (p_shape_B->get_type() == PS2DE::SHAPE_SEPARATION_RAY) {
		return false;
	}

	Vector2 from = p_transform_A.get_origin();
	Vector2 to = from + p_transform_A[1] * (p_shape_A->get_length() + p_margin);
	if (p_motion_A != Vector2()) {
		//not the best but should be enough
		Vector2 normal = (to - from).normalized();
		to += normal * MAX(0.0, normal.dot(p_motion_A));
	}
	Vector2 support_A = to;

	Transform2D invb = p_transform_B.affine_inverse();
	from = invb.xform(from);
	to = invb.xform(to);

	Vector2 p, n;
	if (!p_shape_B->intersect_segment(from, to, p, n)) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts when the ray is fully contained inside the shape.
	if (n == Vector2()) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	// Discard contacts in the wrong direction.
	if (n.dot(from - to) < CMP_EPSILON) {
		if (r_sep_axis) {
			*r_sep_axis = p_transform_A[1].normalized();
		}
		return false;
	}

	Vector2 support_B = p_transform_B.xform(p);
	if (p_shape_A->get_slide_on_slope()) {
		Vector2 global_n = invb.basis_xform_inv(n).normalized();
		support_B = support_A + (support_B - support_A).length() * global_n;
	}

	if (p_result_callback) {
		if (p_swap_result) {
			p_result_callback(support_B, support_A, p_userdata);
		} else {
			p_result_callback(support_A, support_B, p_userdata);
		}
	}
	return true;
}

struct _ConcaveCollisionInfo2D {
	const Transform2D *transform_A = nullptr;
	const GodotShape2D *shape_A = nullptr;
	const Transform2D *transform_B = nullptr;
	Vector2 motion_A;
	Vector2 motion_B;
	real_t margin_A = 0.0;
	real_t margin_B = 0.0;
	GodotCollisionSolver2D::CallbackResult result_callback = nullptr;
	void *userdata = nullptr;
	bool swap_result = false;
	bool collided = false;
	int aabb_tests = 0;
	int collisions = 0;
	Vector2 *sep_axis = nullptr;
};

bool GodotCollisionSolver2D::concave_callback(void *p_userdata, GodotShape2D *p_convex) {
	_ConcaveCollisionInfo2D &cinfo = *(static_cast<_ConcaveCollisionInfo2D *>(p_userdata));
	cinfo.aabb_tests++;

	bool collided = collision_solver(cinfo.shape_A, *cinfo.transform_A, cinfo.motion_A, p_convex, *cinfo.transform_B, cinfo.motion_B, cinfo.result_callback, cinfo.userdata, cinfo.swap_result, cinfo.sep_axis, cinfo.margin_A, cinfo.margin_B);
	if (!collided) {
		return false;
	}

	cinfo.collided = true;
	cinfo.collisions++;

	// Stop at first collision if contacts are not needed.
	return !cinfo.result_callback;
}

bool GodotCollisionSolver2D::solve_concave(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	_ConcaveCollisionInfo2D cinfo;
	cinfo.transform_A = &p_transform_A;
	cinfo.shape_A = p_shape_A;
	cinfo.transform_B = &p_transform_B;
	cinfo.motion_A = p_motion_A;
	cinfo.result_callback = p_result_callback;
	cinfo.userdata = p_userdata;
	cinfo.swap_result = p_swap_result;
	cinfo.collided = false;
	cinfo.collisions = 0;
	cinfo.sep_axis = r_sep_axis;
	cinfo.margin_A = p_margin_A;
	cinfo.margin_B = p_margin_B;

	cinfo.aabb_tests = 0;

	Transform2D rel_transform = p_transform_A;
	rel_transform.columns[2] -= p_transform_B.get_origin();

	// Quickly compute a local Rect2.
	Rect2 local_aabb;
	for (int i = 0; i < 2; i++) {
		Vector2 axis(p_transform_B.columns[i]);
		real_t axis_scale = 1.0 / axis.length();
		axis *= axis_scale;

		real_t smin = 0.0, smax = 0.0;
		p_shape_A->project_rangev(axis, rel_transform, smin, smax);
		smin *= axis_scale;
		smax *= axis_scale;

		local_aabb.position[i] = smin;
		local_aabb.size[i] = smax - smin;
	}
	// In case of motion, expand the Rect2 in the motion direction.
	if (p_motion_A != Vector2()) {
		Rect2 moved_aabb = local_aabb;
		moved_aabb.position += p_motion_A;
		local_aabb = local_aabb.merge(moved_aabb);
	}

	p_shape_B->cull(local_aabb, concave_callback, &cinfo);

	return cinfo.collided;
}

bool GodotCollisionSolver2D::solve(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, Vector2 *r_sep_axis, real_t p_margin_A, real_t p_margin_B) {
	PS2DE::ShapeType type_A = p_shape_A->get_type();
	PS2DE::ShapeType type_B = p_shape_B->get_type();
	bool concave_A = p_shape_A->is_concave();
	bool concave_B = p_shape_B->is_concave();
	real_t margin_A = p_margin_A, margin_B = p_margin_B;

	bool swap = false;

	if (type_A > type_B) {
		SWAP(type_A, type_B);
		SWAP(concave_A, concave_B);
		SWAP(margin_A, margin_B);
		swap = true;
	}

	if (type_A == PS2DE::SHAPE_WORLD_BOUNDARY) {
		if (type_B == PS2DE::SHAPE_WORLD_BOUNDARY) {
			WARN_PRINT_ONCE("Collisions between world boundaries are not supported.");
			return false;
		}

		if (swap) {
			return solve_static_world_boundary(p_shape_B, p_transform_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, p_margin_A);
		} else {
			return solve_static_world_boundary(p_shape_A, p_transform_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, p_margin_B);
		}

	} else if (type_A == PS2DE::SHAPE_SEPARATION_RAY) {
		if (type_B == PS2DE::SHAPE_SEPARATION_RAY) {
			WARN_PRINT_ONCE("Collisions between two rays are not supported.");
			return false; //no ray-ray
		}

		if (swap) {
			return solve_separation_ray(p_shape_B, p_motion_B, p_transform_B, p_shape_A, p_transform_A, p_result_callback, p_userdata, true, r_sep_axis, p_margin_B);
		} else {
			return solve_separation_ray(p_shape_A, p_motion_A, p_transform_A, p_shape_B, p_transform_B, p_result_callback, p_userdata, false, r_sep_axis, p_margin_A);
		}

	} else if (concave_B) {
		if (concave_A) {
			WARN_PRINT_ONCE("Collisions between two concave shapes are not supported.");
			return false;
		}

		if (!swap) {
			return solve_concave(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
		} else {
			return solve_concave(p_shape_B, p_transform_B, p_motion_B, p_shape_A, p_transform_A, p_motion_A, p_result_callback, p_userdata, true, r_sep_axis, margin_A, margin_B);
		}

	} else {
		return collision_solver(p_shape_A, p_transform_A, p_motion_A, p_shape_B, p_transform_B, p_motion_B, p_result_callback, p_userdata, false, r_sep_axis, margin_A, margin_B);
	}
}

// ============================================================================
// Merged from modules/godot_physics_2d/godot_constraints_2d.cpp
// ============================================================================

#include "modules/godot_physics_2d/godot_physics_server_2d.h"

#include "modules/godot_physics_2d/godot_shape_2d.h"
#include "modules/godot_physics_2d/godot_physics_server_2d.h"

// ============================================================================
// GodotAreaPair2D / GodotArea2Pair2D
// ============================================================================

void GodotAreaPair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotAreaPair2D::GodotAreaPair2D(GodotBody2D *p_body, int p_body_shape, GodotArea2D *p_area, int p_area_shape) {
	body = p_body;
	area = p_area;
	body_shape = p_body_shape;
	area_shape = p_area_shape;
	body->add_constraint(this, 0);
	area->add_constraint(this);
	if (p_body->get_mode() == PS2DE::BODY_MODE_KINEMATIC) { // need to be active to process pair
		p_body->set_active(true);
	}
}

bool GodotAreaPair2D::setup(real_t p_step) {
	return false;
}

bool GodotAreaPair2D::pre_solve(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::setup(real_t p_step) {
	return false;
}

bool GodotArea2Pair2D::pre_solve(real_t p_step) {
	if (process_collision_a) {
		if (colliding_a) {
			area_a->add_area_to_query(area_b, shape_b, shape_a);
		} else {
			area_a->remove_area_from_query(area_b, shape_b, shape_a);
		}
	}

	if (process_collision_b) {
		if (colliding_b) {
			area_b->add_area_to_query(area_a, shape_a, shape_b);
		} else {
			area_b->remove_area_from_query(area_a, shape_a, shape_b);
		}
	}

	return false; // Never do any post solving.
}

void GodotArea2Pair2D::solve(real_t p_step) {
	// Nothing to do.
}

GodotArea2Pair2D::GodotArea2Pair2D(GodotArea2D *p_area_a, int p_shape_a, GodotArea2D *p_area_b, int p_shape_b) {
	area_a = p_area_a;
	area_b = p_area_b;
	shape_a = p_shape_a;
	shape_b = p_shape_b;
	area_a_monitorable = area_a->is_monitorable();
	area_b_monitorable = area_b->is_monitorable();
	area_a->add_constraint(this);
	area_b->add_constraint(this);
}

// ============================================================================
// GodotBodyPair2D
// ============================================================================

#define ACCUMULATE_IMPULSES

#define MIN_VELOCITY 0.001
#define MAX_BIAS_ROTATION (Math::PI / 8)

void GodotBodyPair2D::_add_contact(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_self) {
	GodotBodyPair2D *self = static_cast<GodotBodyPair2D *>(p_self);

	self->_contact_added_callback(p_point_A, p_point_B);
}

void GodotBodyPair2D::_contact_added_callback(const Vector2 &p_point_A, const Vector2 &p_point_B) {
	Vector2 local_A = A->get_inv_transform().basis_xform(p_point_A);
	Vector2 local_B = B->get_inv_transform().basis_xform(p_point_B - offset_B);

	int new_index = contact_count;

	ERR_FAIL_COND(new_index >= (MAX_CONTACTS + 1));

	Contact contact;
	contact.local_A = local_A;
	contact.local_B = local_B;
	contact.normal = (p_point_A - p_point_B).normalized();
	contact.used = true;

	// Attempt to determine if the contact will be reused.
	real_t recycle_radius_2 = space->get_contact_recycle_radius() * space->get_contact_recycle_radius();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		if (c.local_A.distance_squared_to(local_A) < (recycle_radius_2) &&
				c.local_B.distance_squared_to(local_B) < (recycle_radius_2)) {
			contact.acc_normal_impulse = c.acc_normal_impulse;
			contact.acc_tangent_impulse = c.acc_tangent_impulse;
			contact.acc_bias_impulse = c.acc_bias_impulse;
			contact.acc_bias_impulse_center_of_mass = c.acc_bias_impulse_center_of_mass;
			c = contact;
			return;
		}
	}

	// Figure out if the contact amount must be reduced to fit the new contact.
	if (new_index == MAX_CONTACTS) {
		// Remove the contact with the minimum depth.
		const Transform2D &transform_A = A->get_transform();
		const Transform2D &transform_B = B->get_transform();

		int least_deep = -1;
		real_t min_depth;

		// Start with depth for new contact.
		{
			Vector2 global_A = transform_A.basis_xform(contact.local_A);
			Vector2 global_B = transform_B.basis_xform(contact.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			min_depth = axis.dot(contact.normal);
		}

		for (int i = 0; i < contact_count; i++) {
			const Contact &c = contacts[i];
			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < min_depth) {
				min_depth = depth;
				least_deep = i;
			}
		}

		if (least_deep > -1) {
			// Replace the least deep contact by the new one.
			contacts[least_deep] = contact;
		}

		return;
	}

	contacts[new_index] = contact;
	contact_count++;
}

void GodotBodyPair2D::_validate_contacts() {
	// Make sure to erase contacts that are no longer valid.
	real_t max_separation = space->get_contact_max_separation();
	real_t max_separation2 = max_separation * max_separation;

	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];

		bool erase = false;
		if (!c.used) {
			// Was left behind in previous frame.
			erase = true;
		} else {
			c.used = false;

			Vector2 global_A = transform_A.basis_xform(c.local_A);
			Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;
			Vector2 axis = global_A - global_B;
			real_t depth = axis.dot(c.normal);

			if (depth < -max_separation || (global_B + c.normal * depth - global_A).length_squared() > max_separation2) {
				erase = true;
			}
		}

		if (erase) {
			// Contact no longer needed, remove.
			if ((i + 1) < contact_count) {
				// Swap with the last one.
				SWAP(contacts[i], contacts[contact_count - 1]);
			}

			i--;
			contact_count--;
		}
	}
}

bool GodotBodyPair2D::_test_ccd(real_t p_step, GodotBody2D *p_A, int p_shape_A, const Transform2D &p_xform_A, GodotBody2D *p_B, int p_shape_B, const Transform2D &p_xform_B) {
	Vector2 motion = p_A->get_linear_velocity() * p_step;
	real_t mlen = motion.length();
	if (mlen < CMP_EPSILON) {
		return false;
	}

	Vector2 mnormal = motion / mlen;

	real_t min = 0.0, max = 0.0;
	p_A->get_shape(p_shape_A)->project_rangev(mnormal, p_xform_A, min, max);

	bool fast_object = mlen > (max - min) * 0.3;
	if (!fast_object) {
		return false;
	}

	Transform2D predicted_xform_B = p_xform_B.translated(p_B->get_linear_velocity() * p_step);

	int a;
	Vector2 s[2];
	p_A->get_shape(p_shape_A)->get_supports(p_xform_A.basis_xform_inv(mnormal).normalized(), s, a);
	Vector2 from = p_xform_A.xform(s[0]);
	Vector2 to = from + motion;

	Transform2D from_inv = predicted_xform_B.affine_inverse();

	Vector2 local_from = from_inv.xform(from - motion * 0.1);
	Vector2 local_to = from_inv.xform(to);

	Vector2 rpos, rnorm;
	if (!p_B->get_shape(p_shape_B)->intersect_segment(local_from, local_to, rpos, rnorm)) {
		return false;
	}

	// Check one-way collision based on motion direction.
	if (p_A->get_shape(p_shape_A)->allows_one_way_collision() && p_B->is_shape_set_as_one_way_collision(p_shape_B)) {
		Vector2 direction = predicted_xform_B.basis_xform(p_B->get_shape_one_way_collision_direction(p_shape_B)).normalized();

		if (direction.dot(mnormal) < CMP_EPSILON) {
			collided = false;
			oneway_disabled = true;
			return false;
		}
	}

	// Shorten the linear velocity so it does not hit, but gets close enough,
	// next frame will hit softly or soft enough.
	Vector2 hitpos = predicted_xform_B.xform(rpos);

	real_t newlen = hitpos.distance_to(from) + (max - min) * 0.01;
	p_A->set_linear_velocity(mnormal * (newlen / p_step));

	return true;
}

static real_t combine_bounce(GodotBody2D *A, GodotBody2D *B) {
	return CLAMP(A->get_bounce() + B->get_bounce(), 0, 1);
}

static real_t combine_friction(GodotBody2D *A, GodotBody2D *B) {
	return Math::abs(MIN(A->get_friction(), B->get_friction()));
}

bool GodotBodyPair2D::setup(real_t p_step) {
	check_ccd = false;

	if (!A->interacts_with(B) || A->has_exception(B->get_self()) || B->has_exception(A->get_self())) {
		collided = false;
		return false;
	}

	collide_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && A->collides_with(B);
	collide_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC) && B->collides_with(A);

	report_contacts_only = false;
	if (!collide_A && !collide_B) {
		if ((A->get_max_contacts_reported() > 0) || (B->get_max_contacts_reported() > 0)) {
			report_contacts_only = true;
		} else {
			collided = false;
			return false;
		}
	}

	// use local A coordinates to avoid numerical issues on collision detection
	offset_B = B->get_transform().get_origin() - A->get_transform().get_origin();

	_validate_contacts();

	const Vector2 &offset_A = A->get_transform().get_origin();
	Transform2D xform_Au = A->get_transform().untranslated();
	Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

	Transform2D xform_Bu = B->get_transform();
	xform_Bu.columns[2] -= offset_A;
	Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	Vector2 motion_A, motion_B;

	if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_A = A->get_motion();
	}
	if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_SHAPE) {
		motion_B = B->get_motion();
	}

	bool prev_collided = collided;

	collided = GodotCollisionSolver2D::solve(shape_A_ptr, xform_A, motion_A, shape_B_ptr, xform_B, motion_B, _add_contact, this, &sep_axis);
	if (!collided) {
		oneway_disabled = false;

		if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
			check_ccd = true;
			return true;
		}

		if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
			check_ccd = true;
			return true;
		}

		return false;
	}

	if (oneway_disabled) {
		return false;
	}

	if (!prev_collided) {
		if (shape_B_ptr->allows_one_way_collision() && A->is_shape_set_as_one_way_collision(shape_A)) {
			Vector2 direction = xform_A.basis_xform(A->get_shape_one_way_collision_direction(shape_A)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) > -CMP_EPSILON) { // Greater (normal inverted).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}

		if (shape_A_ptr->allows_one_way_collision() && B->is_shape_set_as_one_way_collision(shape_B)) {
			Vector2 direction = xform_B.basis_xform(B->get_shape_one_way_collision_direction(shape_B)).normalized();

			bool valid = false;
			for (int i = 0; i < contact_count; i++) {
				Contact &c = contacts[i];
				if (c.normal.dot(direction) < CMP_EPSILON) { // Less (normal ok).
					continue;
				}
				valid = true;
				break;
			}
			if (!valid) {
				collided = false;
				oneway_disabled = true;
				return false;
			}
		}
	}

	return true;
}

bool GodotBodyPair2D::pre_solve(real_t p_step) {
	if (oneway_disabled) {
		return false;
	}

	if (!collided) {
		if (check_ccd) {
			const Vector2 &offset_A = A->get_transform().get_origin();
			Transform2D xform_Au = A->get_transform().untranslated();
			Transform2D xform_A = xform_Au * A->get_shape_transform(shape_A);

			Transform2D xform_Bu = B->get_transform();
			xform_Bu.columns[2] -= offset_A;
			Transform2D xform_B = xform_Bu * B->get_shape_transform(shape_B);

			if (A->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_A) {
				_test_ccd(p_step, A, shape_A, xform_A, B, shape_B, xform_B);
			}

			if (B->get_continuous_collision_detection_mode() == PS2DE::CCD_MODE_CAST_RAY && collide_B) {
				_test_ccd(p_step, B, shape_B, xform_B, A, shape_A, xform_A);
			}
		}

		return false;
	}

	real_t max_penetration = space->get_contact_max_allowed_penetration();
	real_t bias = space->get_contact_bias();

	GodotShape2D *shape_A_ptr = A->get_shape(shape_A);
	GodotShape2D *shape_B_ptr = B->get_shape(shape_B);

	if (shape_A_ptr->get_custom_bias() || shape_B_ptr->get_custom_bias()) {
		if (shape_A_ptr->get_custom_bias() == 0) {
			bias = shape_B_ptr->get_custom_bias();
		} else if (shape_B_ptr->get_custom_bias() == 0) {
			bias = shape_A_ptr->get_custom_bias();
		} else {
			bias = (shape_B_ptr->get_custom_bias() + shape_A_ptr->get_custom_bias()) * 0.5;
		}
	}

	real_t inv_dt = 1.0 / p_step;

	bool do_process = false;

	const Vector2 &offset_A = A->get_transform().get_origin();
	const Transform2D &transform_A = A->get_transform();
	const Transform2D &transform_B = B->get_transform();

	real_t inv_inertia_A = collide_A ? A->get_inv_inertia() : 0.0;
	real_t inv_inertia_B = collide_B ? B->get_inv_inertia() : 0.0;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; i++) {
		Contact &c = contacts[i];
		c.active = false;

		Vector2 global_A = transform_A.basis_xform(c.local_A);
		Vector2 global_B = transform_B.basis_xform(c.local_B) + offset_B;

		Vector2 axis = global_A - global_B;
		real_t depth = axis.dot(c.normal);

		if (depth <= 0.0) {
			continue;
		}

#ifdef DEBUG_ENABLED
		if (space->is_debugging_contacts()) {
			space->add_debug_contact(global_A + offset_A);
			space->add_debug_contact(global_B + offset_A);
		}
#endif

		c.rA = global_A - A->get_center_of_mass();
		c.rB = global_B - B->get_center_of_mass() - offset_B;

		// Precompute normal mass, tangent mass, and bias.
		real_t rnA = c.rA.dot(c.normal);
		real_t rnB = c.rB.dot(c.normal);
		real_t kNormal = inv_mass_A + inv_mass_B;
		kNormal += inv_inertia_A * (c.rA.dot(c.rA) - rnA * rnA) + inv_inertia_B * (c.rB.dot(c.rB) - rnB * rnB);
		c.mass_normal = 1.0f / kNormal;

		Vector2 tangent = c.normal.orthogonal();
		real_t rtA = c.rA.dot(tangent);
		real_t rtB = c.rB.dot(tangent);
		real_t kTangent = inv_mass_A + inv_mass_B;
		kTangent += inv_inertia_A * (c.rA.dot(c.rA) - rtA * rtA) + inv_inertia_B * (c.rB.dot(c.rB) - rtB * rtB);
		c.mass_tangent = 1.0f / kTangent;

		c.bias = -bias * inv_dt * MIN(0.0f, -depth + max_penetration);
		c.depth = depth;

		Vector2 P = c.acc_normal_impulse * c.normal + c.acc_tangent_impulse * tangent;

		c.acc_impulse -= P;

		if (report_contacts_only) {
			collided = false;
			continue;
		}

#ifdef ACCUMULATE_IMPULSES
		{
			// Apply normal + friction impulse
			if (collide_A) {
				A->apply_impulse(-P, c.rA + A->get_center_of_mass());
			}
			if (collide_B) {
				B->apply_impulse(P, c.rB + B->get_center_of_mass());
			}
		}
#endif

		c.bounce = combine_bounce(A, B);
		if (c.bounce) {
			Vector2 crA(-A->get_prev_angular_velocity() * c.rA.y, A->get_prev_angular_velocity() * c.rA.x);
			Vector2 crB(-B->get_prev_angular_velocity() * c.rB.y, B->get_prev_angular_velocity() * c.rB.x);
			Vector2 dv = B->get_prev_linear_velocity() + crB - A->get_prev_linear_velocity() - crA;
			c.bounce = c.bounce * dv.dot(c.normal);
		}

		c.active = true;
		do_process = true;
	}

	return do_process;
}

void GodotBodyPair2D::solve(real_t p_step) {
	if (!collided || oneway_disabled) {
		return;
	}

	const real_t max_bias_av = MAX_BIAS_ROTATION / p_step;

	real_t inv_mass_A = collide_A ? A->get_inv_mass() : 0.0;
	real_t inv_mass_B = collide_B ? B->get_inv_mass() : 0.0;

	for (int i = 0; i < contact_count; ++i) {
		Contact &c = contacts[i];

		if (!c.active) {
			continue;
		}

		// Relative velocity at contact
		Vector2 crA(-A->get_angular_velocity() * c.rA.y, A->get_angular_velocity() * c.rA.x);
		Vector2 crB(-B->get_angular_velocity() * c.rB.y, B->get_angular_velocity() * c.rB.x);
		Vector2 dv = B->get_linear_velocity() + crB - A->get_linear_velocity() - crA;

		Vector2 crbA(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		Vector2 crbB(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		Vector2 dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		real_t vn = dv.dot(c.normal);
		real_t vbn = dbv.dot(c.normal);

		Vector2 tangent = c.normal.orthogonal();
		real_t vt = dv.dot(tangent);

		real_t jbn = (c.bias - vbn) * c.mass_normal;
		real_t jbnOld = c.acc_bias_impulse;
		c.acc_bias_impulse = MAX(jbnOld + jbn, 0.0f);

		Vector2 jb = c.normal * (c.acc_bias_impulse - jbnOld);

		if (collide_A) {
			A->apply_bias_impulse(-jb, c.rA + A->get_center_of_mass(), max_bias_av);
		}
		if (collide_B) {
			B->apply_bias_impulse(jb, c.rB + B->get_center_of_mass(), max_bias_av);
		}

		crbA = Vector2(-A->get_biased_angular_velocity() * c.rA.y, A->get_biased_angular_velocity() * c.rA.x);
		crbB = Vector2(-B->get_biased_angular_velocity() * c.rB.y, B->get_biased_angular_velocity() * c.rB.x);
		dbv = B->get_biased_linear_velocity() + crbB - A->get_biased_linear_velocity() - crbA;

		vbn = dbv.dot(c.normal);

		if (Math::abs(-vbn + c.bias) > MIN_VELOCITY) {
			real_t jbn_com = (-vbn + c.bias) / (inv_mass_A + inv_mass_B);
			real_t jbnOld_com = c.acc_bias_impulse_center_of_mass;
			c.acc_bias_impulse_center_of_mass = MAX(jbnOld_com + jbn_com, 0.0f);

			Vector2 jb_com = c.normal * (c.acc_bias_impulse_center_of_mass - jbnOld_com);

			if (collide_A) {
				A->apply_bias_impulse(-jb_com, A->get_center_of_mass(), 0.0f);
			}
			if (collide_B) {
				B->apply_bias_impulse(jb_com, B->get_center_of_mass(), 0.0f);
			}
		}

		real_t jn = -(c.bounce + vn) * c.mass_normal;
		real_t jnOld = c.acc_normal_impulse;
		c.acc_normal_impulse = MAX(jnOld + jn, 0.0f);

		real_t friction = combine_friction(A, B);

		real_t jtMax = friction * c.acc_normal_impulse;
		real_t jt = -vt * c.mass_tangent;
		real_t jtOld = c.acc_tangent_impulse;
		c.acc_tangent_impulse = CLAMP(jtOld + jt, -jtMax, jtMax);

		Vector2 j = c.normal * (c.acc_normal_impulse - jnOld) + tangent * (c.acc_tangent_impulse - jtOld);

		if (collide_A) {
			A->apply_impulse(-j, c.rA + A->get_center_of_mass());
		}
		if (collide_B) {
			B->apply_impulse(j, c.rB + B->get_center_of_mass());
		}
		c.acc_impulse -= j;
	}
}

GodotBodyPair2D::GodotBodyPair2D(GodotBody2D *p_A, int p_shape_A, GodotBody2D *p_B, int p_shape_B) :
		GodotConstraint2D(_arr, 2) {
	A = p_A;
	B = p_B;
	shape_A = p_shape_A;
	shape_B = p_shape_B;
	space = A->get_space();
	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

GodotBodyPair2D::~GodotBodyPair2D() {
	A->remove_constraint(this, 0);
	B->remove_constraint(this, 1);
}

// ============================================================================
// GodotJoint2D (based on Chipmunk joint constraints)
// ============================================================================

/* Copyright (c) 2007 Scott Lembcke
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

void GodotJoint2D::copy_settings_from(GodotJoint2D *p_joint) {
	set_self(p_joint->get_self());
	set_max_force(p_joint->get_max_force());
	set_bias(p_joint->get_bias());
	set_max_bias(p_joint->get_max_bias());
	disable_collisions_between_bodies(p_joint->is_disabled_collisions_between_bodies());
}

static inline real_t k_scalar(GodotBody2D *a, GodotBody2D *b, const Vector2 &rA, const Vector2 &rB, const Vector2 &n) {
	real_t value = 0.0;

	{
		value += a->get_inv_mass();
		real_t rcn = (rA - a->get_center_of_mass()).cross(n);
		value += a->get_inv_inertia() * rcn * rcn;
	}

	if (b) {
		value += b->get_inv_mass();
		real_t rcn = (rB - b->get_center_of_mass()).cross(n);
		value += b->get_inv_inertia() * rcn * rcn;
	}

	return value;
}

static inline Vector2 relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB) {
	Vector2 sum = a->get_linear_velocity() - (rA - a->get_center_of_mass()).orthogonal() * a->get_angular_velocity();
	if (b) {
		return (b->get_linear_velocity() - (rB - b->get_center_of_mass()).orthogonal() * b->get_angular_velocity()) - sum;
	} else {
		return -sum;
	}
}

static inline real_t normal_relative_velocity(GodotBody2D *a, GodotBody2D *b, Vector2 rA, Vector2 rB, Vector2 n) {
	return relative_velocity(a, b, rA, rB).dot(n);
}

static inline Vector2 custom_cross(const Vector2 &p_vec, real_t p_other) {
	return Vector2(p_other * p_vec.y, -p_other * p_vec.x);
}

static inline void k_tensor(GodotBody2D *a, GodotBody2D *b, Vector2 r1, Vector2 r2, Vector2 *k1, Vector2 *k2) {
	real_t k11, k12, k21, k22;
	real_t m_sum = a->get_inv_mass() + b->get_inv_mass();

	// start with I*m_sum
	k11 = m_sum;
	k12 = 0.0f;
	k21 = 0.0f;
	k22 = m_sum;

	r1 -= a->get_center_of_mass();
	r2 -= b->get_center_of_mass();

	// add the influence from r1
	real_t a_i_inv = a->get_inv_inertia();
	real_t r1xsq = r1.x * r1.x * a_i_inv;
	real_t r1ysq = r1.y * r1.y * a_i_inv;
	real_t r1nxy = -r1.x * r1.y * a_i_inv;
	k11 += r1ysq;
	k12 += r1nxy;
	k21 += r1nxy;
	k22 += r1xsq;

	// add the influence from r2
	real_t b_i_inv = b->get_inv_inertia();
	real_t r2xsq = r2.x * r2.x * b_i_inv;
	real_t r2ysq = r2.y * r2.y * b_i_inv;
	real_t r2nxy = -r2.x * r2.y * b_i_inv;
	k11 += r2ysq;
	k12 += r2nxy;
	k21 += r2nxy;
	k22 += r2xsq;

	// invert
	real_t determinant = k11 * k22 - k12 * k21;
	ERR_FAIL_COND(determinant == 0.0);

	real_t det_inv = 1.0f / determinant;
	*k1 = Vector2(k22 * det_inv, -k12 * det_inv);
	*k2 = Vector2(-k21 * det_inv, k11 * det_inv);
}

static _FORCE_INLINE_ Vector2 mult_k(const Vector2 &vr, const Vector2 &k1, const Vector2 &k2) {
	return Vector2(vr.dot(k1), vr.dot(k2));
}

bool GodotJoint2D::setup(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _setup_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _setup_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _setup_damped_spring(p_step);
		default:
			return false;
	}
}

bool GodotJoint2D::pre_solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			return _pre_solve_pin(p_step);
		case PS2DE::JOINT_TYPE_GROOVE:
			return _pre_solve_groove(p_step);
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			return _pre_solve_damped_spring(p_step);
		default:
			return false;
	}
}

void GodotJoint2D::solve(real_t p_step) {
	switch (type) {
		case PS2DE::JOINT_TYPE_PIN:
			_solve_pin(p_step);
			break;
		case PS2DE::JOINT_TYPE_GROOVE:
			_solve_groove(p_step);
			break;
		case PS2DE::JOINT_TYPE_DAMPED_SPRING:
			_solve_damped_spring(p_step);
			break;
		default:
			break;
	}
}

// --- Pin Joint ---

bool GodotJoint2D::_setup_pin(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B && B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B ? B->get_transform().basis_xform(anchor_B) : anchor_B;

	real_t B_inv_mass = B ? B->get_inv_mass() : 0.0;

	Transform2D K1;
	K1[0].x = A->get_inv_mass() + B_inv_mass;
	K1[1].x = 0.0f;
	K1[0].y = 0.0f;
	K1[1].y = A->get_inv_mass() + B_inv_mass;

	Vector2 r1 = rA - A->get_center_of_mass();

	Transform2D K2;
	K2[0].x = A->get_inv_inertia() * r1.y * r1.y;
	K2[1].x = -A->get_inv_inertia() * r1.x * r1.y;
	K2[0].y = -A->get_inv_inertia() * r1.x * r1.y;
	K2[1].y = A->get_inv_inertia() * r1.x * r1.x;

	Transform2D K;
	K[0] = K1[0] + K2[0];
	K[1] = K1[1] + K2[1];

	if (B) {
		Vector2 r2 = rB - B->get_center_of_mass();

		Transform2D K3;
		K3[0].x = B->get_inv_inertia() * r2.y * r2.y;
		K3[1].x = -B->get_inv_inertia() * r2.x * r2.y;
		K3[0].y = -B->get_inv_inertia() * r2.x * r2.y;
		K3[1].y = B->get_inv_inertia() * r2.x * r2.x;

		K[0] += K3[0];
		K[1] += K3[1];
	}

	K[0].x += softness;
	K[1].y += softness;

	M = K.affine_inverse();

	Vector2 gA = rA + A->get_transform().get_origin();
	Vector2 gB = B ? rB + B->get_transform().get_origin() : rB;

	Vector2 delta = gB - gA;

	pin_bias = delta * -(get_bias() == 0 ? space->get_constraint_bias() : get_bias()) * (1.0 / p_step);

	// Compute max impulse.
	jn_max = get_max_force() * p_step;

	return true;
}

bool GodotJoint2D::_pre_solve_pin(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-P, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(P, rB);
	}
	real_t i_sum_local = A->get_inv_inertia();
	if (B) {
		i_sum_local += B->get_inv_inertia();
	}
	i_sum = 1.0 / (i_sum_local);
	if (angular_limit_enabled && B) {
		Vector2 diff_vector = B->get_transform().get_origin() - A->get_transform().get_origin();
		diff_vector = diff_vector.rotated(-initial_angle);
		real_t dist = diff_vector.angle();
		real_t pdist = 0.0;
		if (dist > angular_limit_upper) {
			pdist = dist - angular_limit_upper;
		} else if (dist < angular_limit_lower) {
			pdist = dist - angular_limit_lower;
		}
		real_t error_bias = Math::pow(1.0 - 0.15, 60.0);
		// Calculate bias velocity.
		bias_velocity = -CLAMP((-1.0 - Math::pow(error_bias, p_step)) * pdist / p_step, -get_max_bias(), get_max_bias());
		// If the bias velocity is 0, the joint is not at a limit.
		if (bias_velocity >= -CMP_EPSILON && bias_velocity <= CMP_EPSILON) {
			j_acc = 0;
			is_joint_at_limit = false;
		} else {
			is_joint_at_limit = true;
		}
	} else {
		bias_velocity = 0.0;
	}

	return true;
}

void GodotJoint2D::_solve_pin(real_t p_step) {
	// Compute relative velocity.
	Vector2 vA = A->get_linear_velocity() - custom_cross(rA - A->get_center_of_mass(), A->get_angular_velocity());

	Vector2 rel_vel;
	if (B) {
		rel_vel = B->get_linear_velocity() - custom_cross(rB - B->get_center_of_mass(), B->get_angular_velocity()) - vA;
	} else {
		rel_vel = -vA;
	}
	if ((angular_limit_enabled || motor_enabled) && B) {
		// Compute relative rotational velocity.
		real_t wr = B->get_angular_velocity() - A->get_angular_velocity();
		if (motor_enabled) {
			wr -= motor_target_velocity;
		}
		real_t j_max = jn_max;

		// Compute normal impulse.
		real_t j_pin = -(bias_velocity + wr) * i_sum;
		real_t j_old = j_acc;
		// Only enable the limits if we have to.
		if (angular_limit_enabled && is_joint_at_limit) {
			if (bias_velocity < 0.0) {
				j_acc = CLAMP(j_old + j_pin, 0.0, j_max);
			} else {
				j_acc = CLAMP(j_old + j_pin, -j_max, 0.0);
			}
		} else {
			j_acc = CLAMP(j_old + j_pin, -j_max, j_max);
		}
		j_pin = j_acc - j_old;
		A->apply_torque_impulse(-j_pin * A->get_inv_inertia());
		B->apply_torque_impulse(j_pin * B->get_inv_inertia());
	}

	Vector2 impulse = M.basis_xform(pin_bias - rel_vel - Vector2(softness, softness) * P);

	if (dynamic_A) {
		A->apply_impulse(-impulse, rA);
	}
	if (B && dynamic_B) {
		B->apply_impulse(impulse, rB);
	}

	P += impulse;
}

void GodotJoint2D::set_param(PS2DE::PinJointParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			softness = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			angular_limit_upper = p_value;
		} break;
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			angular_limit_lower = p_value;
		} break;
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			motor_target_velocity = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::PinJointParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, 0);
	switch (p_param) {
		case PS2DE::PIN_JOINT_SOFTNESS: {
			return softness;
		}
		case PS2DE::PIN_JOINT_LIMIT_UPPER: {
			return angular_limit_upper;
		}
		case PS2DE::PIN_JOINT_LIMIT_LOWER: {
			return angular_limit_lower;
		}
		case PS2DE::PIN_JOINT_MOTOR_TARGET_VELOCITY: {
			return motor_target_velocity;
		}
	}
	ERR_FAIL_V(0);
}

void GodotJoint2D::set_flag(PS2DE::PinJointFlag p_flag, bool p_enabled) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_PIN);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			angular_limit_enabled = p_enabled;
		} break;
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			motor_enabled = p_enabled;
		} break;
	}
}

bool GodotJoint2D::get_flag(PS2DE::PinJointFlag p_flag) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_PIN, false);
	switch (p_flag) {
		case PS2DE::PIN_JOINT_FLAG_ANGULAR_LIMIT_ENABLED: {
			return angular_limit_enabled;
		}
		case PS2DE::PIN_JOINT_FLAG_MOTOR_ENABLED: {
			return motor_enabled;
		}
	}
	ERR_FAIL_V(false);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_pos, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, p_body_b ? 2 : 1),
		type(PS2DE::JOINT_TYPE_PIN) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = p_body_a->get_inv_transform().xform(p_pos);
	anchor_B = p_body_b ? p_body_b->get_inv_transform().xform(p_pos) : p_pos;

	p_body_a->add_constraint(this, 0);
	if (p_body_b) {
		p_body_b->add_constraint(this, 1);
		initial_angle = A->get_transform().get_origin().angle_to_point(B->get_transform().get_origin());
	}
}

// --- Groove Joint ---

bool GodotJoint2D::_setup_groove(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	GodotSpace2D *space = A->get_space();
	ERR_FAIL_NULL_V(space, false);

	// calculate endpoints in worldspace
	Vector2 ta = A->get_transform().xform(A_groove_1);
	Vector2 tb = A->get_transform().xform(A_groove_2);

	// calculate axis
	Vector2 axis_n = -(tb - ta).orthogonal().normalized();
	real_t d = ta.dot(axis_n);

	xf_normal = axis_n;
	rB = B->get_transform().basis_xform(B_anchor);

	// calculate tangential distance along the axis of rB
	real_t td = (B->get_transform().get_origin() + rB).cross(axis_n);
	// calculate clamping factor and rB
	if (td <= ta.cross(axis_n)) {
		clamp = 1.0f;
		rA = ta - A->get_transform().get_origin();
	} else if (td >= tb.cross(axis_n)) {
		clamp = -1.0f;
		rA = tb - A->get_transform().get_origin();
	} else {
		clamp = 0.0f;
		rA = ((-axis_n.orthogonal() * -td) + axis_n * d) - A->get_transform().get_origin();
	}

	// Calculate mass tensor
	k_tensor(A, B, rA, rB, &k1, &k2);

	// compute max impulse
	jn_max = get_max_force() * p_step;

	// calculate bias velocity
	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);

	real_t _b = get_bias();
	gbias = (delta * -(_b == 0 ? space->get_constraint_bias() : _b) * (1.0 / p_step)).limit_length(get_max_bias());

	correct = true;
	return true;
}

bool GodotJoint2D::_pre_solve_groove(real_t p_step) {
	// Apply accumulated impulse.
	if (dynamic_A) {
		A->apply_impulse(-jn_acc, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(jn_acc, rB);
	}

	return true;
}

void GodotJoint2D::_solve_groove(real_t p_step) {
	// compute impulse
	Vector2 vr = relative_velocity(A, B, rA, rB);

	Vector2 j_groove = mult_k(gbias - vr, k1, k2);
	Vector2 jOld = jn_acc;
	j_groove += jOld;

	jn_acc = (((clamp * j_groove.cross(xf_normal)) > 0) ? j_groove : j_groove.project(xf_normal)).limit_length(jn_max);

	j_groove = jn_acc - jOld;

	if (dynamic_A) {
		A->apply_impulse(-j_groove, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_groove, rB);
	}
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_a_groove1, const Vector2 &p_a_groove2, const Vector2 &p_b_anchor, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_GROOVE) {
	A = p_body_a;
	B = p_body_b;

	A_groove_1 = A->get_inv_transform().xform(p_a_groove1);
	A_groove_2 = A->get_inv_transform().xform(p_a_groove2);
	B_anchor = B->get_inv_transform().xform(p_b_anchor);
	A_groove_normal = -(A_groove_2 - A_groove_1).normalized().orthogonal();

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}

// --- Damped Spring Joint ---

bool GodotJoint2D::_setup_damped_spring(real_t p_step) {
	dynamic_A = (A->get_mode() > PS2DE::BODY_MODE_KINEMATIC);
	dynamic_B = (B->get_mode() > PS2DE::BODY_MODE_KINEMATIC);

	if (!dynamic_A && !dynamic_B) {
		return false;
	}

	rA = A->get_transform().basis_xform(anchor_A);
	rB = B->get_transform().basis_xform(anchor_B);

	Vector2 delta = (B->get_transform().get_origin() + rB) - (A->get_transform().get_origin() + rA);
	real_t dist = delta.length();

	if (dist) {
		n = delta / dist;
	} else {
		n = Vector2();
	}

	real_t k = k_scalar(A, B, rA, rB, n);
	n_mass = 1.0f / k;

	target_vrn = 0.0f;
	v_coef = 1.0f - Math::exp(-damping * (p_step)*k);

	// Calculate spring force.
	real_t f_spring = (rest_length - dist) * stiffness;
	j = n * f_spring * (p_step);

	return true;
}

bool GodotJoint2D::_pre_solve_damped_spring(real_t p_step) {
	// Apply spring force.
	if (dynamic_A) {
		A->apply_impulse(-j, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j, rB);
	}

	return true;
}

void GodotJoint2D::_solve_damped_spring(real_t p_step) {
	// compute relative velocity
	real_t vrn = normal_relative_velocity(A, B, rA, rB, n) - target_vrn;

	// compute velocity loss from drag
	real_t v_damp = -vrn * v_coef;
	target_vrn = vrn + v_damp;
	Vector2 j_new = n * v_damp * n_mass;

	if (dynamic_A) {
		A->apply_impulse(-j_new, rA);
	}
	if (dynamic_B) {
		B->apply_impulse(j_new, rB);
	}
}

void GodotJoint2D::set_param(PS2DE::DampedSpringParam p_param, real_t p_value) {
	ERR_FAIL_COND(type != PS2DE::JOINT_TYPE_DAMPED_SPRING);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			rest_length = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			damping = p_value;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			stiffness = p_value;
		} break;
	}
}

real_t GodotJoint2D::get_param(PS2DE::DampedSpringParam p_param) const {
	ERR_FAIL_COND_V(type != PS2DE::JOINT_TYPE_DAMPED_SPRING, 0);
	switch (p_param) {
		case PS2DE::DAMPED_SPRING_REST_LENGTH: {
			return rest_length;
		} break;
		case PS2DE::DAMPED_SPRING_DAMPING: {
			return damping;
		} break;
		case PS2DE::DAMPED_SPRING_STIFFNESS: {
			return stiffness;
		} break;
	}

	ERR_FAIL_V(0);
}

GodotJoint2D::GodotJoint2D(const Vector2 &p_anchor_a, const Vector2 &p_anchor_b, GodotBody2D *p_body_a, GodotBody2D *p_body_b) :
		GodotConstraint2D(_arr, 2),
		type(PS2DE::JOINT_TYPE_DAMPED_SPRING) {
	A = p_body_a;
	B = p_body_b;
	anchor_A = A->get_inv_transform().xform(p_anchor_a);
	anchor_B = B->get_inv_transform().xform(p_anchor_b);

	rest_length = p_anchor_a.distance_to(p_anchor_b);

	A->add_constraint(this, 0);
	B->add_constraint(this, 1);
}
