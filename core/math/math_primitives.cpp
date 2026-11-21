/**************************************************************************/
/*  math_primitives.cpp                                                   */
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

#include "core/error/error_macros.h"
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/color_names.inc"
#include "core/math/face3.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/random_pcg.h"
#include "core/math/static_raycaster.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "modules/modules_enabled.gen.h"

#include <thirdparty/misc/ok_color.h>

// --- plane.cpp ---

void Plane::set_normal(const Vector3& p_normal) { normal = p_normal; }

void Plane::normalize()
{
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Plane cannot be normalized, the distance and the normal should be finite.");
	}
#endif // MATH_CHECKS

	real_t l = normal.length();
	if (l == 0) {
		zero();
		return;
	}
	normal /= l;
	d /= l;
}

Plane Plane::normalized() const
{
	Plane p = *this;
	p.normalize();
	return p;
}

Vector3 Plane::get_any_perpendicular_normal() const
{
	static const Vector3 p1 = Vector3(1, 0, 0);
	static const Vector3 p2 = Vector3(0, 1, 0);
	Vector3 p;

	if (Math::abs(normal.dot(p1)) > 0.99f) { // if too similar to p1
		p = p2;								 // use p2
	}
	else {
		p = p1; // use p1
	}

	p -= normal * normal.dot(p);
	p.normalize();

	return p;
}

/* intersections */

bool Plane::intersect_3(const Plane& p_plane1, const Plane& p_plane2, Vector3* r_result) const
{
	const Plane& p_plane0 = *this;
	Vector3 normal0 = p_plane0.normal;
	Vector3 normal1 = p_plane1.normal;
	Vector3 normal2 = p_plane2.normal;

	real_t denom = vec3_cross(normal0, normal1).dot(normal2);

	if (Math::is_zero_approx(denom)) {
		return false;
	}

	if (r_result) {
		*r_result = ((vec3_cross(normal1, normal2) * p_plane0.d) +
						(vec3_cross(normal2, normal0) * p_plane1.d) +
						(vec3_cross(normal0, normal1) * p_plane2.d)) /
					denom;
	}

	return true;
}

bool Plane::intersects_ray(
	const Vector3& p_from, const Vector3& p_dir, Vector3* p_intersection) const
{
	Vector3 segment = p_dir;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_from) - d) / den;

	if (dist >
		(real_t)CMP_EPSILON) { // this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

bool Plane::intersects_segment(
	const Vector3& p_begin, const Vector3& p_end, Vector3* p_intersection) const
{
	Vector3 segment = p_begin - p_end;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_begin) - d) / den;

	if (dist < (real_t)-CMP_EPSILON || dist > (1.0f + (real_t)CMP_EPSILON)) {
		return false;
	}

	dist = -dist;
	*p_intersection = p_begin + segment * dist;

	return true;
}

/* misc */

bool Plane::is_equal_approx_any_side(const Plane& p_plane) const
{
	return (normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d)) ||
		   (normal.is_equal_approx(-p_plane.normal) && Math::is_equal_approx(d, -p_plane.d));
}

bool Plane::is_equal_approx(const Plane& p_plane) const
{
	return normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d);
}

bool Plane::is_same(const Plane& p_plane) const
{
	return normal.is_same(p_plane.normal) && Math::is_same(d, p_plane.d);
}

bool Plane::is_finite() const { return normal.is_finite() && Math::is_finite(d); }

Plane::operator String() const
{
	return "[N: " + normal.operator String() + ", D: " + String::num_real(d, false) + "]";
}

// --- face3.cpp ---

int Face3::split_by_plane(const Plane &p_plane, Face3 p_res[3], bool p_is_point_over[3]) const {
	ERR_FAIL_COND_V(is_degenerate(), 0);

	Vector3 above[4];
	int above_count = 0;

	Vector3 below[4];
	int below_count = 0;

	for (int i = 0; i < 3; i++) {
		if (p_plane.has_point(vertex[i], (real_t)CMP_EPSILON)) { // point is in plane

			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = vertex[i];
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = vertex[i];

		} else {
			if (p_plane.is_point_over(vertex[i])) {
				//Point is over
				ERR_FAIL_COND_V(above_count >= 4, 0);
				above[above_count++] = vertex[i];

			} else {
				//Point is under
				ERR_FAIL_COND_V(below_count >= 4, 0);
				below[below_count++] = vertex[i];
			}

			/* Check for Intersection between this and the next vertex*/

			Vector3 inters;
			if (!p_plane.intersects_segment(vertex[i], vertex[(i + 1) % 3], &inters)) {
				continue;
			}

			/* Intersection goes to both */
			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = inters;
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = inters;
		}
	}

	int polygons_created = 0;

	ERR_FAIL_COND_V(above_count >= 4 && below_count >= 4, 0); //bug in the algo

	if (above_count >= 3) {
		p_res[polygons_created] = Face3(above[0], above[1], above[2]);
		p_is_point_over[polygons_created] = true;
		polygons_created++;

		if (above_count == 4) {
			p_res[polygons_created] = Face3(above[2], above[3], above[0]);
			p_is_point_over[polygons_created] = true;
			polygons_created++;
		}
	}

	if (below_count >= 3) {
		p_res[polygons_created] = Face3(below[0], below[1], below[2]);
		p_is_point_over[polygons_created] = false;
		polygons_created++;

		if (below_count == 4) {
			p_res[polygons_created] = Face3(below[2], below[3], below[0]);
			p_is_point_over[polygons_created] = false;
			polygons_created++;
		}
	}

	return polygons_created;
}

bool Face3::intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::ray_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::intersects_segment(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::segment_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::is_degenerate() const {
	Vector3 normal = vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]);
	return (normal.length_squared() < (real_t)CMP_EPSILON2);
}

Vector3 Face3::get_random_point_inside() const {
	real_t a = Math::random(0.0, 1.0);
	real_t b = Math::random(0.0, 1.0);
	if (a > b) {
		SWAP(a, b);
	}

	return vertex[0] * a + vertex[1] * (b - a) + vertex[2] * (1.0f - b);
}

Plane Face3::get_plane(ClockDirection p_dir) const {
	return Plane(vertex[0], vertex[1], vertex[2], p_dir);
}

real_t Face3::get_area() const {
	return vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]).length() * 0.5f;
}

bool Face3::intersects_aabb(const AABB &p_aabb) const {
	/** TEST PLANE **/
	if (!p_aabb.intersects_plane(get_plane())) {
		return false;
	}

#define TEST_AXIS(m_ax) \
	/** TEST FACE AXIS */ \
	{ \
		real_t aabb_min = p_aabb.position.m_ax; \
		real_t aabb_max = p_aabb.position.m_ax + p_aabb.size.m_ax; \
		real_t tri_min = vertex[0].m_ax; \
		real_t tri_max = vertex[0].m_ax; \
		for (int i = 1; i < 3; i++) { \
			if (vertex[i].m_ax > tri_max) \
				tri_max = vertex[i].m_ax; \
			if (vertex[i].m_ax < tri_min) \
				tri_min = vertex[i].m_ax; \
		} \
\
		if (tri_max < aabb_min || aabb_max < tri_min) \
			return false; \
	}

	TEST_AXIS(x);
	TEST_AXIS(y);
	TEST_AXIS(z);

#undef TEST_AXIS

	/** TEST ALL EDGES **/

	const Vector3 edge_norms[3] = {
		vertex[0] - vertex[1],
		vertex[1] - vertex[2],
		vertex[2] - vertex[0],
	};

	for (int i = 0; i < 12; i++) {
		Vector3 from, to;
		p_aabb.get_edge(i, from, to);
		Vector3 e1 = from - to;
		for (int j = 0; j < 3; j++) {
			Vector3 e2 = edge_norms[j];

			Vector3 axis = vec3_cross(e1, e2);

			if (axis.length_squared() < 0.0001f) {
				continue; // coplanar
			}
			axis.normalize();

			real_t minA, maxA, minB, maxB;
			p_aabb.project_range_in_plane(Plane(axis), minA, maxA);
			project_range(axis, Transform3D(), minB, maxB);

			if (maxA < minB || maxB < minA) {
				return false;
			}
		}
	}
	return true;
}

Face3::operator String() const {
	return String() + String(vertex[0]) + ", " + String(vertex[1]) + ", " + String(vertex[2]);
}

void Face3::project_range(const Vector3 &p_normal, const Transform3D &p_transform, real_t &r_min, real_t &r_max) const {
	for (int i = 0; i < 3; i++) {
		Vector3 v = p_transform.xform(vertex[i]);
		real_t d = p_normal.dot(v);

		if (i == 0 || d > r_max) {
			r_max = d;
		}

		if (i == 0 || d < r_min) {
			r_min = d;
		}
	}
}

void Face3::get_support(const Vector3 &p_normal, const Transform3D &p_transform, Vector3 *p_vertices, int *p_count, int p_max) const {
	constexpr double face_support_threshold = 0.98;
	constexpr double edge_support_threshold = 0.05;

	if (p_max <= 0) {
		return;
	}

	Vector3 n = p_transform.basis.xform_inv(p_normal);

	/** TEST FACE AS SUPPORT **/
	if (get_plane().normal.dot(n) > face_support_threshold) {
		*p_count = MIN(3, p_max);

		for (int i = 0; i < *p_count; i++) {
			p_vertices[i] = p_transform.xform(vertex[i]);
		}

		return;
	}

	/** FIND SUPPORT VERTEX **/

	int vert_support_idx = -1;
	real_t support_max = 0;

	for (int i = 0; i < 3; i++) {
		real_t d = n.dot(vertex[i]);

		if (i == 0 || d > support_max) {
			support_max = d;
			vert_support_idx = i;
		}
	}

	/** TEST EDGES AS SUPPORT **/

	for (int i = 0; i < 3; i++) {
		if (i != vert_support_idx && i + 1 != vert_support_idx) {
			continue;
		}

		// check if edge is valid as a support
		real_t dot = (vertex[i] - vertex[(i + 1) % 3]).normalized().dot(n);
		dot = Math::abs(dot);
		if (dot < edge_support_threshold) {
			*p_count = MIN(2, p_max);

			for (int j = 0; j < *p_count; j++) {
				p_vertices[j] = p_transform.xform(vertex[(j + i) % 3]);
			}

			return;
		}
	}

	*p_count = 1;
	p_vertices[0] = p_transform.xform(vertex[vert_support_idx]);
}

Vector3 Face3::get_closest_point_to(const Vector3 &p_point) const {
	Vector3 edge0 = vertex[1] - vertex[0];
	Vector3 edge1 = vertex[2] - vertex[0];
	Vector3 v0 = vertex[0] - p_point;

	real_t a = edge0.dot(edge0);
	real_t b = edge0.dot(edge1);
	real_t c = edge1.dot(edge1);
	real_t d = edge0.dot(v0);
	real_t e = edge1.dot(v0);

	real_t det = a * c - b * b;
	real_t s = b * e - c * d;
	real_t t = b * d - a * e;

	if (s + t < det) {
		if (s < 0.f) {
			if (t < 0.f) {
				if (d < 0.f) {
					s = CLAMP(-d / a, 0.f, 1.f);
					t = 0.f;
				} else {
					s = 0.f;
					t = CLAMP(-e / c, 0.f, 1.f);
				}
			} else {
				s = 0.f;
				t = CLAMP(-e / c, 0.f, 1.f);
			}
		} else if (t < 0.f) {
			s = CLAMP(-d / a, 0.f, 1.f);
			t = 0.f;
		} else {
			real_t invDet = 1.f / det;
			s *= invDet;
			t *= invDet;
		}
	} else {
		if (s < 0.f) {
			real_t tmp0 = b + d;
			real_t tmp1 = c + e;
			if (tmp1 > tmp0) {
				real_t numer = tmp1 - tmp0;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				t = CLAMP(-e / c, 0.f, 1.f);
				s = 0.f;
			}
		} else if (t < 0.f) {
			if (a + d > b + e) {
				real_t numer = c + e - b - d;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				s = CLAMP(-d / a, 0.f, 1.f);
				t = 0.f;
			}
		} else {
			real_t numer = c + e - b - d;
			real_t denom = a - 2 * b + c;
			s = CLAMP(numer / denom, 0.f, 1.f);
			t = 1.f - s;
		}
	}

	return vertex[0] + s * edge0 + t * edge1;
}

// --- aabb.cpp ---

real_t AABB::get_volume() const { return size.x * size.y * size.z; }

void AABB::merge_with(const AABB& p_aabb)
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 beg_1, beg_2;
	Vector3 end_1, end_2;
	Vector3 min, max;

	beg_1 = position;
	beg_2 = p_aabb.position;
	end_1 = size + beg_1;
	end_2 = p_aabb.size + beg_2;

	min.x = (beg_1.x < beg_2.x) ? beg_1.x : beg_2.x;
	min.y = (beg_1.y < beg_2.y) ? beg_1.y : beg_2.y;
	min.z = (beg_1.z < beg_2.z) ? beg_1.z : beg_2.z;

	max.x = (end_1.x > end_2.x) ? end_1.x : end_2.x;
	max.y = (end_1.y > end_2.y) ? end_1.y : end_2.y;
	max.z = (end_1.z > end_2.z) ? end_1.z : end_2.z;

	position = min;
	size = max - min;
}

bool AABB::is_equal_approx(const AABB& p_aabb) const
{
	return position.is_equal_approx(p_aabb.position) && size.is_equal_approx(p_aabb.size);
}

bool AABB::is_same(const AABB& p_aabb) const
{
	return position.is_same(p_aabb.position) && size.is_same(p_aabb.size);
}

bool AABB::is_finite() const { return position.is_finite() && size.is_finite(); }

AABB AABB::intersection(const AABB& p_aabb) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 src_min = position;
	Vector3 src_max = position + size;
	Vector3 dst_min = p_aabb.position;
	Vector3 dst_max = p_aabb.position + p_aabb.size;

	Vector3 min, max;

	if (src_min.x > dst_max.x || src_max.x < dst_min.x) {
		return AABB();
	}
	else {
		min.x = (src_min.x > dst_min.x) ? src_min.x : dst_min.x;
		max.x = (src_max.x < dst_max.x) ? src_max.x : dst_max.x;
	}

	if (src_min.y > dst_max.y || src_max.y < dst_min.y) {
		return AABB();
	}
	else {
		min.y = (src_min.y > dst_min.y) ? src_min.y : dst_min.y;
		max.y = (src_max.y < dst_max.y) ? src_max.y : dst_max.y;
	}

	if (src_min.z > dst_max.z || src_max.z < dst_min.z) {
		return AABB();
	}
	else {
		min.z = (src_min.z > dst_min.z) ? src_min.z : dst_min.z;
		max.z = (src_max.z < dst_max.z) ? src_max.z : dst_max.z;
	}

	return AABB(min, max - min);
}

// Note that this routine returns the BACKTRACKED (i.e. behind the ray origin)
// intersection point + normal if INSIDE the AABB.
// The caller can therefore decide when INSIDE whether to use the
// backtracked intersection, or use p_from as the intersection, and
// carry on progressing without e.g. reflecting against the normal.
bool AABB::find_intersects_ray(const Vector3& p_from, const Vector3& p_dir, bool& r_inside,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 end = position + size;
	real_t tmin = -1e20;
	real_t tmax = 1e20;
	int axis = 0;

	// Make sure r_inside is always initialized,
	// to prevent reading uninitialized data in the client code.
	r_inside = false;

	for (int i = 0; i < 3; i++) {
		if (p_dir[i] == 0) {
			if ((p_from[i] < position[i]) || (p_from[i] > end[i])) {
				return false;
			}
		}
		else { // ray not parallel to planes in this direction
			real_t t1 = (position[i] - p_from[i]) / p_dir[i];
			real_t t2 = (end[i] - p_from[i]) / p_dir[i];

			if (t1 > t2) {
				SWAP(t1, t2);
			}
			if (t1 >= tmin) {
				tmin = t1;
				axis = i;
			}
			if (t2 < tmax) {
				if (t2 < 0) {
					return false;
				}
				tmax = t2;
			}
			if (tmin > tmax) {
				return false;
			}
		}
	}

	// Did the ray start from inside the box?
	// In which case the intersection returned is the point of entry
	// (behind the ray start) or the calling routine can use the ray origin as intersection point.
	r_inside = tmin < 0;

	if (r_intersection_point) {
		*r_intersection_point = p_from + p_dir * tmin;

		// Prevent float error by making sure the point is exactly
		// on the AABB border on the relevant axis.
		(*r_intersection_point)[axis] = (p_dir[axis] >= 0) ? position[axis] : end[axis];
	}
	if (r_normal) {
		*r_normal = Vector3();
		(*r_normal)[axis] = (p_dir[axis] >= 0) ? -1 : 1;
	}

	return true;
}

bool AABB::intersects_segment(const Vector3& p_from, const Vector3& p_to,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	real_t min = 0, max = 1;
	int axis = 0;
	real_t sign = 0;

	for (int i = 0; i < 3; i++) {
		real_t seg_from = p_from[i];
		real_t seg_to = p_to[i];
		real_t box_begin = position[i];
		real_t box_end = box_begin + size[i];
		real_t cmin, cmax;
		real_t csign;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;

		}
		else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	Vector3 rel = p_to - p_from;

	if (r_normal) {
		Vector3 normal;
		normal[axis] = sign;
		*r_normal = normal;
	}

	if (r_intersection_point) {
		*r_intersection_point = p_from + rel * min;
	}

	return true;
}

bool AABB::intersects_plane(const Plane& p_plane) const
{
	Vector3 points[8] = {
		Vector3(position.x, position.y, position.z),
		Vector3(position.x, position.y, position.z + size.z),
		Vector3(position.x, position.y + size.y, position.z),
		Vector3(position.x, position.y + size.y, position.z + size.z),
		Vector3(position.x + size.x, position.y, position.z),
		Vector3(position.x + size.x, position.y, position.z + size.z),
		Vector3(position.x + size.x, position.y + size.y, position.z),
		Vector3(position.x + size.x, position.y + size.y, position.z + size.z),
	};

	bool over = false;
	bool under = false;

	for (int i = 0; i < 8; i++) {
		if (p_plane.distance_to(points[i]) > 0) {
			over = true;
		}
		else {
			under = true;
		}
	}

	return under && over;
}

Vector3 AABB::get_longest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = Vector3(0, 1, 0);
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_longest_axis_index() const
{
	int axis = 0;
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = 1;
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = 2;
	}

	return axis;
}

Vector3 AABB::get_shortest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = Vector3(0, 1, 0);
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_shortest_axis_index() const
{
	int axis = 0;
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = 1;
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = 2;
	}

	return axis;
}

AABB AABB::merge(const AABB& p_with) const
{
	AABB aabb = *this;
	aabb.merge_with(p_with);
	return aabb;
}

AABB AABB::expand(const Vector3& p_vector) const
{
	AABB aabb = *this;
	aabb.expand_to(p_vector);
	return aabb;
}

AABB AABB::grow(real_t p_by) const
{
	AABB aabb = *this;
	aabb.grow_by(p_by);
	return aabb;
}

void AABB::get_edge(int p_edge, Vector3& r_from, Vector3& r_to) const
{
	ERR_FAIL_INDEX(p_edge, 12);
	switch (p_edge) {
	case 0: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z);
	} break;
	case 1: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z);
	} break;
	case 2: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z + size.z);

	} break;
	case 3: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z + size.z);

	} break;
	case 4: {
		r_from = Vector3(position.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);
	} break;
	case 5: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
	} break;
	case 6: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 7: {
		r_from = Vector3(position.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 8: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 9: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 10: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);

	} break;
	case 11: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);


	} break;
	}
}

AABB::operator String() const { return "[P: " + String(position) + ", S: " + String(size) + "]"; }

// --- color.cpp ---

uint32_t Color::to_argb32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);

	return c;
}

uint32_t Color::to_abgr32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);

	return c;
}

uint32_t Color::to_rgba32() const {
	uint32_t c = (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(a * 255.0f);

	return c;
}

uint64_t Color::to_abgr64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);

	return c;
}

uint64_t Color::to_argb64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);

	return c;
}

uint64_t Color::to_rgba64() const {
	uint64_t c = (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(a * 65535.0f);

	return c;
}

void _append_hex(float p_val, char32_t *r_string) {
	int v = Math::round(p_val * 255.0f);
	v = CLAMP(v, 0, 255);

	r_string[0] = hex_char_table_lower[(v >> 4) & 0xF];
	r_string[1] = hex_char_table_lower[v & 0xF];
}

String Color::to_html(bool p_alpha) const {
	String txt;
	txt.resize_uninitialized(p_alpha ? 9 : 7);
	char32_t *ptr = txt.ptrw();

	_append_hex(r, ptr + 0);
	_append_hex(g, ptr + 2);
	_append_hex(b, ptr + 4);
	if (p_alpha) {
		_append_hex(a, ptr + 6);
	}
	ptr[txt.size() - 1] = '\0';

	return txt;
}

float Color::get_h() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	if (delta == 0.0f) {
		return 0.0f;
	}

	float h;
	if (r == max) {
		h = (g - b) / delta; // between yellow & magenta
	} else if (g == max) {
		h = 2 + (b - r) / delta; // between cyan & yellow
	} else {
		h = 4 + (r - g) / delta; // between magenta & cyan
	}

	h /= 6.0f;
	if (h < 0.0f) {
		h += 1.0f;
	}

	return h;
}

float Color::get_s() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	return (max != 0.0f) ? (delta / max) : 0.0f;
}

float Color::get_v() const {
	float max = MAX(r, g);
	max = MAX(max, b);
	return max;
}

void Color::set_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	int i;
	float f, p, q, t;
	a = p_alpha;

	if (p_s == 0.0f) {
		// Achromatic (gray)
		r = g = b = p_v;
		return;
	}

	p_h *= 6.0f;
	p_h = Math::fmod(p_h, 6);
	i = Math::floor(p_h);

	f = p_h - i;
	p = p_v * (1.0f - p_s);
	q = p_v * (1.0f - p_s * f);
	t = p_v * (1.0f - p_s * (1.0f - f));

	switch (i) {
		case 0: // Red is the dominant color
			r = p_v;
			g = t;
			b = p;
			break;
		case 1: // Green is the dominant color
			r = q;
			g = p_v;
			b = p;
			break;
		case 2:
			r = p;
			g = p_v;
			b = t;
			break;
		case 3: // Blue is the dominant color
			r = p;
			g = q;
			b = p_v;
			break;
		case 4:
			r = t;
			g = p;
			b = p_v;
			break;
		default: // (5) Red is the dominant color
			r = p_v;
			g = p;
			b = q;
			break;
	}
}

void Color::set_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	ok_color::HSL hsl;
	hsl.h = p_h;
	hsl.s = p_s;
	hsl.l = p_l;
	ok_color::RGB rgb = ok_color::okhsl_to_srgb(hsl);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

void Color::set_ok_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	ok_color::HSV hsv;
	hsv.h = p_h;
	hsv.s = p_s;
	hsv.v = p_v;
	ok_color::RGB rgb = ok_color::okhsv_to_srgb(hsv);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

bool Color::is_equal_approx(const Color &p_color) const {
	return Math::is_equal_approx(r, p_color.r) && Math::is_equal_approx(g, p_color.g) && Math::is_equal_approx(b, p_color.b) && Math::is_equal_approx(a, p_color.a);
}

bool Color::is_same(const Color &p_color) const {
	return Math::is_same(r, p_color.r) && Math::is_same(g, p_color.g) && Math::is_same(b, p_color.b) && Math::is_same(a, p_color.a);
}

Color Color::clamp(const Color &p_min, const Color &p_max) const {
	return Color(
			CLAMP(r, p_min.r, p_max.r),
			CLAMP(g, p_min.g, p_max.g),
			CLAMP(b, p_min.b, p_max.b),
			CLAMP(a, p_min.a, p_max.a));
}

void Color::invert() {
	r = 1.0f - r;
	g = 1.0f - g;
	b = 1.0f - b;
}

Color Color::hex(uint32_t p_hex) {
	float a = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float b = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float g = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float r = (p_hex & 0xFF) / 255.0f;

	return Color(r, g, b, a);
}

Color Color::hex64(uint64_t p_hex) {
	float a = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float b = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float g = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float r = (p_hex & 0xFFFF) / 65535.0f;

	return Color(r, g, b, a);
}

static int _parse_col4(const String &p_str, int p_ofs) {
	char character = p_str[p_ofs];

	if (character >= '0' && character <= '9') {
		return character - '0';
	} else if (character >= 'a' && character <= 'f') {
		return character + (10 - 'a');
	} else if (character >= 'A' && character <= 'F') {
		return character + (10 - 'A');
	}
	return -1;
}

static int _parse_col8(const String &p_str, int p_ofs) {
	return _parse_col4(p_str, p_ofs) * 16 + _parse_col4(p_str, p_ofs + 1);
}

Color Color::inverted() const {
	Color c = *this;
	c.invert();
	return c;
}

Color Color::html(const String &p_rgba) {
	if (p_rgba.is_empty()) {
		return Color();
	}

	const int current_pos = (p_rgba[0] == '#') ? 1 : 0;
	const int num_of_digits = p_rgba.length() - current_pos;

	float r, g, b, a = 1.0f;

	if (num_of_digits == 3) {
		// #rgb
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
	} else if (num_of_digits == 4) {
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
		a = _parse_col4(p_rgba, current_pos + 3) / 15.0f;
	} else if (num_of_digits == 6) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
	} else if (num_of_digits == 8) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
		a = _parse_col8(p_rgba, current_pos + 6) / 255.0f;
	} else {
		ERR_FAIL_V_MSG(Color(), "Invalid color code: " + p_rgba + ".");
	}

	ERR_FAIL_COND_V_MSG(r < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(g < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(b < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(a < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");

	return Color(r, g, b, a);
}

bool Color::html_is_valid(const String &p_color) {
	String color = p_color;

	if (color.is_empty()) {
		return false;
	}

	const int current_pos = (color[0] == '#') ? 1 : 0;
	const int len = color.length();
	const int num_of_digits = len - current_pos;
	if (!(num_of_digits == 3 || num_of_digits == 4 || num_of_digits == 6 || num_of_digits == 8)) {
		return false;
	}

	// Check if each hex digit is valid.
	for (int i = current_pos; i < len; i++) {
		if (!is_hex_digit(p_color[i])) {
			return false;
		}
	}

	return true;
}

Color Color::named(const String &p_name) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		ERR_FAIL_V_MSG(Color(), "Invalid color name: " + p_name + ".");
	}
	return named_colors[idx].color;
}

Color Color::named(const String &p_name, const Color &p_default) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		return p_default;
	}
	return named_colors[idx].color;
}

int Color::find_named_color(const String &p_name) {
	String name = p_name;
	// Normalize name.
	name = name.remove_chars(" -_'.");
	name = name.to_upper();

	static HashMap<String, int> named_colors_hashmap;
	if (unlikely(named_colors_hashmap.is_empty())) {
		const int named_color_count = get_named_color_count();
		for (int i = 0; i < named_color_count; i++) {
			named_colors_hashmap[String(named_colors[i].name).remove_char('_')] = i;
		}
	}

	const HashMap<String, int>::ConstIterator E = named_colors_hashmap.find(name);
	if (E) {
		return E->value;
	}

	return -1;
}

int Color::get_named_color_count() {
	return std_size(named_colors);
}

String Color::get_named_color_name(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), "");
	return named_colors[p_idx].name;
}

Color Color::get_named_color(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), Color());
	return named_colors[p_idx].color;
}

// For a version that errors on invalid values instead of returning
// a default color, use the Color(String) constructor instead.
Color Color::from_string(const String &p_string, const Color &p_default) {
	if (html_is_valid(p_string)) {
		return html(p_string);
	} else {
		return named(p_string, p_default);
	}
}

Color Color::from_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	Color c;
	c.set_hsv(p_h, p_s, p_v, p_alpha);
	return c;
}

Color Color::from_rgbe9995(uint32_t p_rgbe) {
	float r = p_rgbe & 0x1ff;
	float g = (p_rgbe >> 9) & 0x1ff;
	float b = (p_rgbe >> 18) & 0x1ff;
	float e = (p_rgbe >> 27);
	float m = Math::pow(2.0f, e - 15.0f - 9.0f);

	float rd = r * m;
	float gd = g * m;
	float bd = b * m;

	return Color(rd, gd, bd, 1.0f);
}

Color Color::from_rgba8(int64_t p_r8, int64_t p_g8, int64_t p_b8, int64_t p_a8) {
	return Color(p_r8 / 255.0f, p_g8 / 255.0f, p_b8 / 255.0f, p_a8 / 255.0f);
}

Color::operator String() const {
	return "(" + String::num(r, 4) + ", " + String::num(g, 4) + ", " + String::num(b, 4) + ", " + String::num(a, 4) + ")";
}

Color Color::from_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsl(p_h, p_s, p_l, p_alpha);
	return c;
}

Color Color::from_ok_hsv(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsv(p_h, p_s, p_l, p_alpha);
	return c;
}

float Color::get_ok_hsl_h() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.h)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.h, 0.0f, 1.0f);
}

float Color::get_ok_hsl_s() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.s)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.s, 0.0f, 1.0f);
}

float Color::get_ok_hsl_l() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.l)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.l, 0.0f, 1.0f);
}

// --- math_funcs.cpp ---

static RandomPCG default_rand;

uint32_t Math::rand_from_seed(uint64_t *p_seed) {
	RandomPCG rng = RandomPCG(*p_seed);
	uint32_t r = rng.rand();
	*p_seed = rng.get_seed();
	return r;
}

void Math::seed(uint64_t p_value) {
	default_rand.seed(p_value);
}

void Math::randomize() {
	default_rand.randomize();
}

uint32_t Math::rand() {
	return default_rand.rand();
}

double Math::randfn(double p_mean, double p_deviation) {
	return default_rand.randfn(p_mean, p_deviation);
}

int Math::step_decimals(double p_step) {
	static const int maxn = 10;
	static const double sd[maxn] = {
		0.9999, // somehow compensate for floating point error
		0.09999,
		0.009999,
		0.0009999,
		0.00009999,
		0.000009999,
		0.0000009999,
		0.00000009999,
		0.000000009999,
		0.0000000009999
	};

	double abs = Math::abs(p_step);
	double decs = abs - (int)abs; // Strip away integer part
	for (int i = 0; i < maxn; i++) {
		if (decs >= sd[i]) {
			return i;
		}
	}

	return 0;
}

// Only meant for editor usage in float ranges, where a step of 0
// means that decimal digits should not be limited in String::num.
int Math::range_step_decimals(double p_step) {
	if (p_step < 0.0000000000001) {
		return 16; // Max value hardcoded in String::num
	}
	return step_decimals(p_step);
}

double Math::ease(double p_x, double p_c) {
	if (p_x < 0) {
		p_x = 0;
	} else if (p_x > 1.0) {
		p_x = 1.0;
	}
	if (p_c > 0) {
		if (p_c < 1.0) {
			return 1.0 - Math::pow(1.0 - p_x, 1.0 / p_c);
		} else {
			return Math::pow(p_x, p_c);
		}
	} else if (p_c < 0) {
		//inout ease

		if (p_x < 0.5) {
			return Math::pow(p_x * 2.0, -p_c) * 0.5;
		} else {
			return (1.0 - Math::pow(1.0 - (p_x - 0.5) * 2.0, -p_c)) * 0.5 + 0.5;
		}
	} else {
		return 0; // no ease (raw)
	}
}

double Math::snapped(double p_value, double p_step) {
	if (p_step != 0) {
		p_value = Math::floor(p_value / p_step + 0.5) * p_step;
	}
	return p_value;
}

uint32_t Math::larger_prime(uint32_t p_val) {
	static const uint32_t primes[] = {
		5,
		13,
		23,
		47,
		97,
		193,
		389,
		769,
		1543,
		3079,
		6151,
		12289,
		24593,
		49157,
		98317,
		196613,
		393241,
		786433,
		1572869,
		3145739,
		6291469,
		12582917,
		25165843,
		50331653,
		100663319,
		201326611,
		402653189,
		805306457,
		1610612741,
		0,
	};

	int idx = 0;
	while (true) {
		ERR_FAIL_COND_V(primes[idx] == 0, 0);
		if (primes[idx] > p_val) {
			return primes[idx];
		}
		idx++;
	}
}

double Math::random(double p_from, double p_to) {
	return default_rand.random(p_from, p_to);
}

float Math::random(float p_from, float p_to) {
	return default_rand.random(p_from, p_to);
}

int Math::random(int p_from, int p_to) {
	return default_rand.random(p_from, p_to);
}

// --- random_pcg.cpp ---

RandomPCG::RandomPCG(uint64_t p_seed, uint64_t p_inc) :
		pcg(),
		current_inc(p_inc) {
	seed(p_seed);
}

void RandomPCG::randomize() {
	seed(((uint64_t)OS::get_unix_time() + OS::get_ticks_usec()) * pcg.state + PCG_DEFAULT_INC_64);
}

int64_t RandomPCG::rand_weighted(const Vector<float> &p_weights) {
	ERR_FAIL_COND_V_MSG(p_weights.is_empty(), -1, "Weights array is empty.");
	int64_t weights_size = p_weights.size();
	const float *weights = p_weights.ptr();
	float weights_sum = 0.0;
	for (int64_t i = 0; i < weights_size; ++i) {
		ERR_FAIL_COND_V_MSG(weights[i] < 0.0, -1, vformat("Weight array contains the negative value %f at index %d.", weights[i], i));
		weights_sum += weights[i];
	}

	float remaining_distance = randf() * weights_sum;
	for (int64_t i = 0; i < weights_size; ++i) {
		remaining_distance -= weights[i];
		if (remaining_distance < 0) {
			return i;
		}
	}

	for (int64_t i = weights_size - 1; i >= 0; --i) {
		if (weights[i] > 0) {
			return i;
		}
	}
	return -1;
}

double RandomPCG::random(double p_from, double p_to) {
	return randd() * (p_to - p_from) + p_from;
}

float RandomPCG::random(float p_from, float p_to) {
	return randf() * (p_to - p_from) + p_from;
}

int RandomPCG::random(int p_from, int p_to) {
	if (p_from == p_to) {
		return p_from;
	}

	int64_t min = MIN(p_from, p_to);
	int64_t max = MAX(p_from, p_to);
	uint32_t diff = static_cast<uint32_t>(max - min);

	if (diff == UINT32_MAX) {
		// Can't add 1 to max uint32_t value for inclusive range, so call rand without passing bounds.
		return static_cast<int64_t>(rand()) + min;
	}

	return static_cast<int64_t>(rand(diff + 1U)) + min;
}

// --- static_raycaster.cpp ---

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<StaticRaycaster> StaticRaycaster::create() { return Ref<StaticRaycaster>(); }

void StaticRaycaster::free() {}

bool StaticRaycaster::intersect(Ray& p_ray) { return false; }

void StaticRaycaster::intersect(Vector<Ray>& r_rays) {}

void StaticRaycaster::add_mesh(
	const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices, unsigned int p_id)
{
}

void StaticRaycaster::commit() {}

void StaticRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void StaticRaycaster::clear_mesh_filter() {}

StaticRaycaster::StaticRaycaster() {}

StaticRaycaster::~StaticRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)
