/**************************************************************************/
/*  godot_collision_solver_2d.cpp                                         */
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

#include "godot_collision_solver_2d.h"

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
