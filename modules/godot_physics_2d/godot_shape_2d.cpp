/**************************************************************************/
/*  godot_shape_2d.cpp                                                    */
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
#include "core/templates/sort_array.h"
#include "godot_shape_2d.h"

void GodotShape2D::configure(const Rect2& p_aabb)
{
	aabb = p_aabb;
	configured = true;
	for (const KeyValue<GodotShapeOwner2D*, int>& E : owners) {
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

void GodotShape2D::add_owner(GodotShapeOwner2D* p_owner)
{
	HashMap<GodotShapeOwner2D*, int>::Iterator E = owners.find(p_owner);
	if (E) {
		E->value++;
	}
	else {
		owners[p_owner] = 1;
	}
}

void GodotShape2D::remove_owner(GodotShapeOwner2D* p_owner)
{
	HashMap<GodotShapeOwner2D*, int>::Iterator E = owners.find(p_owner);
	ERR_FAIL_COND(!E);
	E->value--;
	if (E->value == 0) {
		owners.remove(E);
	}
}

bool GodotShape2D::is_owner(GodotShapeOwner2D* p_owner) const { return owners.has(p_owner); }

const HashMap<GodotShapeOwner2D*, int>& GodotShape2D::get_owners() const { return owners; }

GodotShape2D::~GodotShape2D() { ERR_FAIL_COND(owners.size()); }

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotWorldBoundaryShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	r_amount = 0;
}

bool GodotWorldBoundaryShape2D::contains_point(const Vector2& p_point) const
{
	return normal.dot(p_point) < d;
}

bool GodotWorldBoundaryShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	Vector2 segment = p_begin - p_end;
	real_t den = normal.dot(segment);

	// printf("den is %i\n",den);
	if (Math::abs(den) <= CMP_EPSILON) {
		return false;
	}

	real_t dist = (normal.dot(p_begin) - d) / den;
	// printf("dist is %i\n",dist);

	if (dist < -CMP_EPSILON || dist > (1.0 + CMP_EPSILON)) {
		return false;
	}

	r_point = p_begin + segment * -dist;
	r_normal = normal;

	return true;
}

real_t GodotWorldBoundaryShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	return 0;
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotSeparationRayShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	r_amount = 1;

	if (p_normal.y > 0) {
		*r_supports = Vector2(0, length);
	}
	else {
		*r_supports = Vector2();
	}
}

bool GodotSeparationRayShape2D::contains_point(const Vector2& p_point) const { return false; }

bool GodotSeparationRayShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	return false; // rays can't be intersected
}

real_t GodotSeparationRayShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	return 0; // rays are mass-less
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotSegmentShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	if (Math::abs(p_normal.dot(n)) > segment_is_valid_support_threshold) {
		r_supports[0] = a;
		r_supports[1] = b;
		r_amount = 2;
		return;
	}

	real_t dp = p_normal.dot(b - a);
	if (dp > 0) {
		*r_supports = b;
	}
	else {
		*r_supports = a;
	}
	r_amount = 1;
}

bool GodotSegmentShape2D::contains_point(const Vector2& p_point) const { return false; }

bool GodotSegmentShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	if (!Geometry2D::segment_intersects_segment(p_begin, p_end, a, b, &r_point)) {
		return false;
	}

	if (n.dot(p_begin) > n.dot(a)) {
		r_normal = n;
	}
	else {
		r_normal = -n;
	}

	return true;
}

real_t GodotSegmentShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	return p_mass * ((a * p_scale).distance_squared_to(b * p_scale)) / 12;
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotCircleShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	r_amount = 1;
	*r_supports = p_normal * radius;
}

bool GodotCircleShape2D::contains_point(const Vector2& p_point) const
{
	return p_point.length_squared() < radius * radius;
}

bool GodotCircleShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	Vector2 line_vec = p_end - p_begin;

	real_t a, b, c;

	a = line_vec.dot(line_vec);
	b = 2 * p_begin.dot(line_vec);
	c = p_begin.dot(p_begin) - radius * radius;

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

real_t GodotCircleShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	real_t a = radius * p_scale.x;
	real_t b = radius * p_scale.y;
	return p_mass * (a * a + b * b) / 4;
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotRectangleShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
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

	/* USE POINT */

	r_amount = 1;
	r_supports[0] = Vector2((p_normal.x < 0) ? -half_extents.x : half_extents.x,
		(p_normal.y < 0) ? -half_extents.y : half_extents.y);
}

bool GodotRectangleShape2D::contains_point(const Vector2& p_point) const
{
	real_t x = p_point.x;
	real_t y = p_point.y;
	real_t edge_x = half_extents.x;
	real_t edge_y = half_extents.y;
	return (x >= -edge_x) && (x < edge_x) && (y >= -edge_y) && (y < edge_y);
}

bool GodotRectangleShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	return get_aabb().intersects_segment(p_begin, p_end, &r_point, &r_normal);
}

real_t GodotRectangleShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	Vector2 he2 = half_extents * 2 * p_scale;
	return p_mass * he2.dot(he2) / 12.0;
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotCapsuleShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
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
}

bool GodotCapsuleShape2D::contains_point(const Vector2& p_point) const
{
	Vector2 p = p_point;
	p.y = Math::abs(p.y);
	p.y -= height * 0.5 - radius;
	if (p.y < 0) {
		p.y = 0;
	}

	return p.length_squared() < radius * radius;
}

bool GodotCapsuleShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
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

		real_t a, b, c;

		a = line_vec.dot(line_vec);
		b = 2 * begin.dot(line_vec);
		c = begin.dot(begin) - radius * radius;

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

	// return get_aabb().intersects_segment(p_begin,p_end,&r_point,&r_normal);
	return collided; // todo
}

real_t GodotCapsuleShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	Vector2 he2 = Vector2(radius * 2, height) * p_scale;
	return p_mass * he2.dot(he2) / 12.0;
}

/*********************************************************/
/*********************************************************/
/*********************************************************/

void GodotConvexPolygonShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	int support_idx = -1;
	real_t d = -1e10;
	r_amount = 0;

	for (int i = 0; i < point_count; i++) {
		// test point
		real_t ld = p_normal.dot(points[i].pos);
		if (ld > d) {
			support_idx = i;
			d = ld;
		}

		// test segment
		if (points[i].normal.dot(p_normal) > segment_is_valid_support_threshold) {
			r_amount = 2;
			r_supports[0] = points[i].pos;
			r_supports[1] = points[(i + 1) % point_count].pos;
			return;
		}
	}

	ERR_FAIL_COND_MSG(support_idx == -1, "Convex polygon shape support not found.");

	r_amount = 1;
	r_supports[0] = points[support_idx].pos;
}

bool GodotConvexPolygonShape2D::contains_point(const Vector2& p_point) const
{
	bool out = false;
	bool in = false;

	for (int i = 0; i < point_count; i++) {
		real_t d = points[i].normal.dot(p_point) - points[i].normal.dot(points[i].pos);
		if (d > 0) {
			out = true;
		}
		else {
			in = true;
		}
	}

	return in != out;
}

bool GodotConvexPolygonShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	Vector2 n = (p_end - p_begin).normalized();
	real_t d = 1e10;
	bool inters = false;

	for (int i = 0; i < point_count; i++) {
		Vector2 res;

		if (!Geometry2D::segment_intersects_segment(
				p_begin, p_end, points[i].pos, points[(i + 1) % point_count].pos, &res)) {
			continue;
		}

		real_t nd = n.dot(res);
		if (nd < d) {
			d = nd;
			r_point = res;
			r_normal = points[i].normal;
			inters = true;
		}
	}

	return inters;
}

real_t GodotConvexPolygonShape2D::get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const
{
	ERR_FAIL_COND_V_MSG(point_count == 0, 0, "Convex polygon shape has no points.");
	Rect2 aabb_new;
	aabb_new.position = points[0].pos * p_scale;
	for (int i = 0; i < point_count; i++) {
		aabb_new.expand_to(points[i].pos * p_scale);
	}

	return p_mass * aabb_new.size.dot(aabb_new.size) / 12.0;
}

GodotConvexPolygonShape2D::~GodotConvexPolygonShape2D()
{
	if (points) {
		memdelete_arr(points);
	}
}

//////////////////////////////////////////////////

void GodotConcavePolygonShape2D::get_supports(
	const Vector2& p_normal, Vector2* r_supports, int& r_amount) const
{
	real_t d = -1e10;
	int idx = -1;
	for (int i = 0; i < points.size(); i++) {
		real_t ld = p_normal.dot(points[i]);
		if (ld > d) {
			d = ld;
			idx = i;
		}
	}

	r_amount = 1;
	ERR_FAIL_COND(idx == -1);
	*r_supports = points[idx];
}

bool GodotConcavePolygonShape2D::contains_point(const Vector2& p_point) const
{
	return false; // sorry
}

bool GodotConcavePolygonShape2D::intersect_segment(
	const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const
{
	if (segments.is_empty() || points.is_empty()) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * bvh_depth);

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

	/*
	for(int i=0;i<bvh_depth;i++)
		stack[i]=0;
	*/

	int level = 0;

	const Segment* segmentptr = &segments[0];
	const Vector2* pointptr = &points[0];
	const BVH* bvhptr = &bvh[0];

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
					const Segment& s = segmentptr[bvh2.right];
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

int GodotConcavePolygonShape2D::_generate_bvh(BVH* p_bvh, int p_len, int p_depth)
{
	if (p_len == 1) {
		bvh_depth = MAX(p_depth, bvh_depth);
		bvh.push_back(*p_bvh);
		return bvh.size() - 1;
	}

	// else sort best

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
	int node_idx = bvh.size();
	bvh.push_back(node);

	int l = _generate_bvh(p_bvh, median, p_depth + 1);
	int r = _generate_bvh(&p_bvh[median], p_len - median, p_depth + 1);
	bvh.write[node_idx].left = l;
	bvh.write[node_idx].right = r;

	return node_idx;
}

void GodotConcavePolygonShape2D::cull(
	const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const
{
	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * bvh_depth);

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

	/*
	for(int i=0;i<bvh_depth;i++)
		stack[i]=0;
	*/

	if (segments.is_empty() || points.is_empty() || bvh.is_empty()) {
		return;
	}

	int level = 0;

	const Segment* segmentptr = &segments[0];
	const Vector2* pointptr = &points[0];
	const BVH* bvhptr = &bvh[0];

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
					const Segment& s = segmentptr[bvh2.right];
					Vector2 a = pointptr[s.points[0]];
					Vector2 b = pointptr[s.points[1]];

					GodotSegmentShape2D ss(a, b, (b - a).orthogonal().normalized());

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


