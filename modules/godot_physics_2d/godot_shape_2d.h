/**************************************************************************/
/*  godot_shape_2d.h                                                      */
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

#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/physics_2d/physics_server_2d.h"

class GodotCollisionObject2D;

class GodotShape2D final
{
public:
	// Returns true to stop the query.
	typedef bool (*QueryCallback)(void* p_userdata, GodotShape2D* p_convex);

	struct ConvexPoint
	{
		Vector2 pos;
		Vector2 normal; // normal to next segment
	};

	struct ConcaveSegment
	{
		int points[2] = {};
	};

	struct BVH
	{
		Rect2 aabb;
		int left = 0, right = 0;
	};

private:
	RID self;
	PS2DE::ShapeType type = PS2DE::SHAPE_CUSTOM;
	Rect2 aabb;
	bool configured = false;
	real_t custom_bias = 0.0;

	HashMap<GodotCollisionObject2D*, int> owners;

	// World boundary data
	Vector2 wb_normal;
	real_t wb_d = 0.0;

	// Separation ray data
	real_t ray_length = 0.0;
	bool slide_on_slope = false;

	// Segment data
	Vector2 seg_a;
	Vector2 seg_b;
	Vector2 seg_n;

	// Circle / Capsule data
	real_t radius = 0.0;
	real_t height = 0.0;

	// Rectangle data
	Vector2 half_extents;

	// Convex polygon data
	ConvexPoint* convex_points = nullptr;
	int convex_point_count = 0;

	// Concave polygon data
	Vector<ConcaveSegment> concave_segments;
	Vector<Point2> concave_points;
	Vector<BVH> concave_bvh;
	int concave_bvh_depth = 0;

	struct BVH_CompareX
	{
		_FORCE_INLINE_ bool operator()(const BVH& a, const BVH& b) const
		{
			return (a.aabb.position.x + a.aabb.size.x * 0.5) <
				   (b.aabb.position.x + b.aabb.size.x * 0.5);
		}
	};

	struct BVH_CompareY
	{
		_FORCE_INLINE_ bool operator()(const BVH& a, const BVH& b) const
		{
			return (a.aabb.position.y + a.aabb.size.y * 0.5) <
				   (b.aabb.position.y + b.aabb.size.y * 0.5);
		}
	};

	static constexpr double segment_is_valid_support_threshold = 0.99998;
	static inline const double segment_is_valid_support_threshold_lower =
		Math::sqrt(1.0 - segment_is_valid_support_threshold * segment_is_valid_support_threshold);

	void configure(const Rect2& p_aabb);
	int _generate_bvh(BVH* p_bvh, int p_len, int p_depth);

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	_FORCE_INLINE_ PS2DE::ShapeType get_type() const { return type; }

	_FORCE_INLINE_ Rect2 get_aabb() const { return aabb; }

	_FORCE_INLINE_ bool is_configured() const { return configured; }

	_FORCE_INLINE_ bool allows_one_way_collision() const
	{
		return type != PS2DE::SHAPE_SEPARATION_RAY;
	}

	_FORCE_INLINE_ bool is_concave() const { return type == PS2DE::SHAPE_CONCAVE_POLYGON; }

	_FORCE_INLINE_ void set_custom_bias(real_t p_bias) { custom_bias = p_bias; }

	_FORCE_INLINE_ real_t get_custom_bias() const { return custom_bias; }

	void add_owner(GodotCollisionObject2D* p_owner);
	void remove_owner(GodotCollisionObject2D* p_owner);
	bool is_owner(GodotCollisionObject2D* p_owner) const;
	const HashMap<GodotCollisionObject2D*, int>& get_owners() const;

	// Typed shape configuration setters
	void set_world_boundary(const Vector2& p_normal, real_t p_d);
	void set_separation_ray(real_t p_length, bool p_slide_on_slope);
	void set_segment(const Vector2& p_a, const Vector2& p_b);
	void set_circle(real_t p_radius);
	void set_rectangle(const Vector2& p_half_extents);
	void set_capsule(real_t p_height, real_t p_radius);
	void set_convex_polygon(const Vector<Vector2>& p_points);
	void set_concave_polygon(const Vector<Vector2>& p_segments);

	// Shape-specific inline accessors
	_FORCE_INLINE_ const Vector2& get_world_boundary_normal() const { return wb_normal; }
	_FORCE_INLINE_ real_t get_world_boundary_d() const { return wb_d; }

	_FORCE_INLINE_ real_t get_length() const { return ray_length; }
	_FORCE_INLINE_ bool get_slide_on_slope() const { return slide_on_slope; }

	_FORCE_INLINE_ const Vector2& get_a() const { return seg_a; }
	_FORCE_INLINE_ const Vector2& get_b() const { return seg_b; }
	_FORCE_INLINE_ const Vector2& get_segment_normal() const { return seg_n; }
	_FORCE_INLINE_ Vector2 get_xformed_normal(const Transform2D& p_xform) const
	{
		return (p_xform.xform(seg_b) - p_xform.xform(seg_a)).normalized().orthogonal();
	}

	_FORCE_INLINE_ const real_t& get_radius() const { return radius; }
	_FORCE_INLINE_ const real_t& get_height() const { return height; }

	_FORCE_INLINE_ const Vector2& get_half_extents() const { return half_extents; }

	_FORCE_INLINE_ Vector2 get_circle_axis(
		const Transform2D& p_xform, const Transform2D& p_xform_inv, const Vector2& p_circle) const
	{
		Vector2 local_v = p_xform_inv.xform(p_circle);

		Vector2 he((local_v.x < 0) ? -half_extents.x : half_extents.x,
			(local_v.y < 0) ? -half_extents.y : half_extents.y);

		return (p_xform.xform(he) - p_circle).normalized();
	}

	_FORCE_INLINE_ Vector2 get_box_axis(const Transform2D& p_xform, const Transform2D& p_xform_inv,
		const GodotShape2D* p_B, const Transform2D& p_B_xform,
		const Transform2D& p_B_xform_inv) const
	{
		Vector2 a, b;

		{
			Vector2 local_v = p_xform_inv.xform(p_B_xform.get_origin());

			Vector2 he((local_v.x < 0) ? -half_extents.x : half_extents.x,
				(local_v.y < 0) ? -half_extents.y : half_extents.y);

			a = p_xform.xform(he);
		}
		{
			Vector2 local_v = p_B_xform_inv.xform(p_xform.get_origin());

			Vector2 he((local_v.x < 0) ? -p_B->half_extents.x : p_B->half_extents.x,
				(local_v.y < 0) ? -p_B->half_extents.y : p_B->half_extents.y);

			b = p_B_xform.xform(he);
		}

		return (a - b).normalized();
	}

	_FORCE_INLINE_ int get_point_count() const { return convex_point_count; }
	_FORCE_INLINE_ const Vector2& get_point(int p_idx) const { return convex_points[p_idx].pos; }
	_FORCE_INLINE_ const Vector2& get_convex_segment_normal(int p_idx) const
	{
		return convex_points[p_idx].normal;
	}
	_FORCE_INLINE_ Vector2 get_xformed_segment_normal(const Transform2D& p_xform, int p_idx) const
	{
		Vector2 a = convex_points[p_idx].pos;
		p_idx++;
		Vector2 b = convex_points[p_idx == convex_point_count ? 0 : p_idx].pos;
		return (p_xform.xform(b) - p_xform.xform(a)).normalized().orthogonal();
	}

	void cull(const Rect2& p_local_aabb, QueryCallback p_callback, void* p_userdata) const;

	// Non-virtual query methods
	bool contains_point(const Vector2& p_point) const;
	bool intersect_segment(
		const Vector2& p_begin, const Vector2& p_end, Vector2& r_point, Vector2& r_normal) const;
	real_t get_moment_of_inertia(real_t p_mass, const Size2& p_scale) const;

	Vector2 get_support(const Vector2& p_normal) const;
	void get_supports(const Vector2& p_normal, Vector2* r_supports, int& r_amount) const;

	template <PS2DE::ShapeType T>
	_FORCE_INLINE_ void project_range_t(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		if constexpr (T == PS2DE::SHAPE_WORLD_BOUNDARY) {
			r_min = -1e10;
			r_max = 1e10;
		}
		else if constexpr (T == PS2DE::SHAPE_SEPARATION_RAY) {
			r_max = p_normal.dot(p_transform.get_origin());
			r_min = p_normal.dot(p_transform.xform(Vector2(0, ray_length)));
			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_SEGMENT) {
			r_max = p_normal.dot(p_transform.xform(seg_a));
			r_min = p_normal.dot(p_transform.xform(seg_b));
			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CIRCLE) {
			real_t d = p_normal.dot(p_transform.get_origin());
			Vector2 local_normal = p_transform.basis_xform_inv(p_normal);
			real_t scale = local_normal.length();
			r_min = d - radius * scale;
			r_max = d + radius * scale;
		}
		else if constexpr (T == PS2DE::SHAPE_RECTANGLE) {
			r_max = -1e20;
			r_min = 1e20;
			for (int i = 0; i < 4; i++) {
				real_t d = p_normal.dot(p_transform.xform(Vector2(
					((i & 1) * 2 - 1) * half_extents.x, ((i >> 1) * 2 - 1) * half_extents.y)));

				if (d > r_max) {
					r_max = d;
				}
				if (d < r_min) {
					r_min = d;
				}
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CAPSULE) {
			Vector2 n = p_transform.basis_xform_inv(p_normal).normalized();
			real_t h = height * 0.5 - radius;

			n *= radius;
			n.y += (n.y > 0) ? h : -h;

			r_max = p_normal.dot(p_transform.xform(n));
			r_min = p_normal.dot(p_transform.xform(-n));

			if (r_max < r_min) {
				SWAP(r_max, r_min);
			}
		}
		else if constexpr (T == PS2DE::SHAPE_CONVEX_POLYGON) {
			if (!convex_points || convex_point_count <= 0) {
				r_min = r_max = 0;
				return;
			}

			r_min = r_max = p_normal.dot(p_transform.xform(convex_points[0].pos));
			for (int i = 1; i < convex_point_count; i++) {
				real_t d = p_normal.dot(p_transform.xform(convex_points[i].pos));
				if (d > r_max) {
					r_max = d;
				}
				if (d < r_min) {
					r_min = d;
				}
			}
		}
		else {
			r_min = 0;
			r_max = 0;
			ERR_FAIL_MSG("Unsupported call to project_range in GodotShape2D");
		}
	}

	template <PS2DE::ShapeType T>
	_FORCE_INLINE_ void project_range_cast_t(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		if constexpr (T == PS2DE::SHAPE_WORLD_BOUNDARY) {
			r_min = -1e10;
			r_max = 1e10;
		}
		else {
			real_t mina, maxa;
			real_t minb, maxb;
			Transform2D ofsb = p_transform;
			ofsb.columns[2] += p_cast;
			project_range_t<T>(p_normal, p_transform, mina, maxa);
			project_range_t<T>(p_normal, ofsb, minb, maxb);
			r_min = MIN(mina, minb);
			r_max = MAX(maxa, maxb);
		}
	}

	void project_range(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const;
	void project_range_cast(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const;

	_FORCE_INLINE_ void project_rangev(
		const Vector2& p_normal, const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		project_range(p_normal, p_transform, r_min, r_max);
	}

	_FORCE_INLINE_ void project_range_castv(const Vector2& p_cast, const Vector2& p_normal,
		const Transform2D& p_transform, real_t& r_min, real_t& r_max) const
	{
		project_range_cast(p_cast, p_normal, p_transform, r_min, r_max);
	}

	_FORCE_INLINE_ void get_supports_transformed_cast(const Vector2& p_cast,
		const Vector2& p_normal, const Transform2D& p_xform, Vector2* r_supports,
		int& r_amount) const
	{
		get_supports(p_xform.basis_xform_inv(p_normal).normalized(), r_supports, r_amount);
		for (int i = 0; i < r_amount; i++) {
			r_supports[i] = p_xform.xform(r_supports[i]);
		}

		if (r_amount == 1) {
			if (Math::abs(p_normal.dot(p_cast.normalized())) <
				segment_is_valid_support_threshold_lower) {
				// make line because they are parallel
				r_amount = 2;
				r_supports[1] = r_supports[0] + p_cast;
			}
			else if (p_cast.dot(p_normal) > 0) {
				// normal points towards cast, add cast
				r_supports[0] += p_cast;
			}
		}
		else {
			if (Math::abs(p_normal.dot(p_cast.normalized())) <
				segment_is_valid_support_threshold_lower) {
				// optimize line and make it larger because they are parallel
				if ((r_supports[1] - r_supports[0]).dot(p_cast) > 0) {
					// larger towards 1
					r_supports[1] += p_cast;
				}
				else {
					// larger towards 0
					r_supports[0] += p_cast;
				}
			}
			else if (p_cast.dot(p_normal) > 0) {
				// normal points towards cast, add cast
				r_supports[0] += p_cast;
				r_supports[1] += p_cast;
			}
		}
	}

	explicit GodotShape2D(PS2DE::ShapeType p_type) : type(p_type) {}

	_FORCE_INLINE_ GodotShape2D(const Vector2& p_a, const Vector2& p_b, const Vector2& p_n)
		: type(PS2DE::SHAPE_SEGMENT), seg_a(p_a), seg_b(p_b), seg_n(p_n)
	{
	}

	GodotShape2D(const GodotShape2D&) = delete;
	GodotShape2D& operator=(const GodotShape2D&) = delete;

	~GodotShape2D();
};

// --- Folded from modules/godot_physics_2d/godot_collision_solver_2d.h ---


class GodotCollisionSolver2D {
public:
	typedef void (*CallbackResult)(const Vector2 &p_point_A, const Vector2 &p_point_B, void *p_userdata);

private:
	static bool solve_static_world_boundary(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, real_t p_margin = 0);
	static bool concave_callback(void *p_userdata, GodotShape2D *p_convex);
	static bool solve_concave(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis = nullptr, real_t p_margin_A = 0, real_t p_margin_B = 0);
	static bool solve_separation_ray(const GodotShape2D *p_shape_A, const Vector2 &p_motion_A, const Transform2D &p_transform_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, CallbackResult p_result_callback, void *p_userdata, bool p_swap_result, Vector2 *r_sep_axis = nullptr, real_t p_margin = 0);

public:
	static bool solve(const GodotShape2D *p_shape_A, const Transform2D &p_transform_A, const Vector2 &p_motion_A, const GodotShape2D *p_shape_B, const Transform2D &p_transform_B, const Vector2 &p_motion_B, CallbackResult p_result_callback, void *p_userdata, Vector2 *r_sep_axis = nullptr, real_t p_margin_A = 0, real_t p_margin_B = 0);
};

// --- End modules/godot_physics_2d/godot_collision_solver_2d.h ---
