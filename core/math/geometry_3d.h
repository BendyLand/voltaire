/**************************************************************************/
/*  geometry_3d.h                                                         */
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

#include "core/math/color.h"
// --- Folded from core/math/delaunay_3d.h ---
#include "core/math/aabb.h"
#include "core/math/projection.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"

#include <thirdparty/misc/r128.h>

class Delaunay3D {
	struct Simplex;

	enum {
		ACCEL_GRID_SIZE = 16,
		QUANTIZATION_MAX = 1 << 16 // A power of two smaller than the 23 bit significand of a float.
	};
	struct GridPos {
		Vector3i pos;
		List<Simplex *>::Element *E = nullptr;
	};

	struct Simplex {
		uint32_t points[4];
		R128 circum_center_x;
		R128 circum_center_y;
		R128 circum_center_z;
		R128 circum_r2;
		LocalVector<GridPos> grid_positions;
		List<Simplex *>::Element *SE = nullptr;

		_FORCE_INLINE_ Simplex() {}
		_FORCE_INLINE_ Simplex(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d) {
			points[0] = p_a;
			points[1] = p_b;
			points[2] = p_c;
			points[3] = p_d;
		}
	};

	struct Triangle {
		uint32_t triangle[3];
		bool bad = false;
		_FORCE_INLINE_ bool operator==(const Triangle &p_triangle) const {
			return triangle[0] == p_triangle.triangle[0] && triangle[1] == p_triangle.triangle[1] && triangle[2] == p_triangle.triangle[2];
		}

		_FORCE_INLINE_ Triangle() {}
		_FORCE_INLINE_ Triangle(uint32_t p_a, uint32_t p_b, uint32_t p_c) {
			if (p_a > p_b) {
				SWAP(p_a, p_b);
			}
			if (p_b > p_c) {
				SWAP(p_b, p_c);
			}
			if (p_a > p_b) {
				SWAP(p_a, p_b);
			}

			triangle[0] = p_a;
			triangle[1] = p_b;
			triangle[2] = p_c;
		}
	};

	struct TriangleHasher {
		_FORCE_INLINE_ static uint32_t hash(const Triangle &p_triangle) {
			uint32_t h = hash_djb2_one_32(p_triangle.triangle[0]);
			h = hash_djb2_one_32(p_triangle.triangle[1], h);
			return hash_fmix32(hash_djb2_one_32(p_triangle.triangle[2], h));
		}
	};

	_FORCE_INLINE_ static void circum_sphere_compute(const Vector3 *p_points, Simplex *p_simplex) {
		// The only part in the algorithm where there may be precision errors is this one,
		// so ensure that we do it with the maximum precision possible.

		R128 v0_x = p_points[p_simplex->points[0]].x;
		R128 v0_y = p_points[p_simplex->points[0]].y;
		R128 v0_z = p_points[p_simplex->points[0]].z;
		R128 v1_x = p_points[p_simplex->points[1]].x;
		R128 v1_y = p_points[p_simplex->points[1]].y;
		R128 v1_z = p_points[p_simplex->points[1]].z;
		R128 v2_x = p_points[p_simplex->points[2]].x;
		R128 v2_y = p_points[p_simplex->points[2]].y;
		R128 v2_z = p_points[p_simplex->points[2]].z;
		R128 v3_x = p_points[p_simplex->points[3]].x;
		R128 v3_y = p_points[p_simplex->points[3]].y;
		R128 v3_z = p_points[p_simplex->points[3]].z;

		// Create the rows of our "unrolled" 3x3 matrix.
		R128 row1_x = v1_x - v0_x;
		R128 row1_y = v1_y - v0_y;
		R128 row1_z = v1_z - v0_z;

		R128 row2_x = v2_x - v0_x;
		R128 row2_y = v2_y - v0_y;
		R128 row2_z = v2_z - v0_z;

		R128 row3_x = v3_x - v0_x;
		R128 row3_y = v3_y - v0_y;
		R128 row3_z = v3_z - v0_z;

		R128 sq_length1 = row1_x * row1_x + row1_y * row1_y + row1_z * row1_z;
		R128 sq_length2 = row2_x * row2_x + row2_y * row2_y + row2_z * row2_z;
		R128 sq_length3 = row3_x * row3_x + row3_y * row3_y + row3_z * row3_z;

		// Compute the determinant of said matrix.
		R128 determinant = row1_x * (row2_y * row3_z - row3_y * row2_z) - row2_x * (row1_y * row3_z - row3_y * row1_z) + row3_x * (row1_y * row2_z - row2_y * row1_z);

		// Compute the volume of the tetrahedron, and precompute a scalar quantity for reuse in the formula.
		R128 volume = determinant / R128(6.f);
		R128 i12volume = R128(1.f) / (volume * R128(12.f));

		R128 center_x = v0_x + i12volume * ((row2_y * row3_z - row3_y * row2_z) * sq_length1 - (row1_y * row3_z - row3_y * row1_z) * sq_length2 + (row1_y * row2_z - row2_y * row1_z) * sq_length3);
		R128 center_y = v0_y + i12volume * (-(row2_x * row3_z - row3_x * row2_z) * sq_length1 + (row1_x * row3_z - row3_x * row1_z) * sq_length2 - (row1_x * row2_z - row2_x * row1_z) * sq_length3);
		R128 center_z = v0_z + i12volume * ((row2_x * row3_y - row3_x * row2_y) * sq_length1 - (row1_x * row3_y - row3_x * row1_y) * sq_length2 + (row1_x * row2_y - row2_x * row1_y) * sq_length3);

		// Once we know the center, the radius is clearly the distance to any vertex.
		R128 rel1_x = center_x - v0_x;
		R128 rel1_y = center_y - v0_y;
		R128 rel1_z = center_z - v0_z;

		R128 radius1 = rel1_x * rel1_x + rel1_y * rel1_y + rel1_z * rel1_z;

		p_simplex->circum_center_x = center_x;
		p_simplex->circum_center_y = center_y;
		p_simplex->circum_center_z = center_z;
		p_simplex->circum_r2 = radius1;
	}

	_FORCE_INLINE_ static bool simplex_contains(const Vector3 *p_points, const Simplex &p_simplex, uint32_t p_vertex) {
		R128 v_x = p_points[p_vertex].x;
		R128 v_y = p_points[p_vertex].y;
		R128 v_z = p_points[p_vertex].z;

		R128 rel2_x = p_simplex.circum_center_x - v_x;
		R128 rel2_y = p_simplex.circum_center_y - v_y;
		R128 rel2_z = p_simplex.circum_center_z - v_z;

		R128 radius2 = rel2_x * rel2_x + rel2_y * rel2_y + rel2_z * rel2_z;

		return radius2 < (p_simplex.circum_r2 - R128(0.0000000001));
		// When this tolerance is too big, it can result in overlapping simplices.
		// When it's too small, large amounts of planar simplices are created.
	}

	static bool simplex_is_coplanar(const Vector3 *p_points, const Simplex &p_simplex) {
		// Checking every possible distance like this is overkill, but only checking
		// one is not enough. If the simplex is almost planar then the vectors p1-p2
		// and p1-p3 can be practically collinear, which makes Plane unreliable.
		for (uint32_t i = 0; i < 4; i++) {
			Plane p(p_points[p_simplex.points[i]], p_points[p_simplex.points[(i + 1) % 4]], p_points[p_simplex.points[(i + 2) % 4]]);
			// This tolerance should not be smaller than the one used with
			// Plane::has_point() when creating the LightmapGI probe BSP tree.
			if (Math::abs(p.distance_to(p_points[p_simplex.points[(i + 3) % 4]])) < 0.001) {
				return true;
			}
		}

		return false;
	}

public:
	struct OutputSimplex {
		uint32_t points[4];
	};

	static Vector<OutputSimplex> tetrahedralize(const Vector<Vector3> &p_points) {
		uint32_t point_count = p_points.size();
		Vector3 *points = (Vector3 *)memalloc(sizeof(Vector3) * (point_count + 4));
		const Vector3 *src_points = p_points.ptr();
		Vector3 proportions;

		{
			AABB rect;
			for (uint32_t i = 0; i < point_count; i++) {
				Vector3 point = src_points[i];
				if (i == 0) {
					rect.position = point;
				} else {
					rect.expand_to(point);
				}
			}

			real_t longest_axis = rect.size[rect.get_longest_axis_index()];
			proportions = Vector3(longest_axis, longest_axis, longest_axis) / rect.size;

			for (uint32_t i = 0; i < point_count; i++) {
				// Scale points to the unit cube to better utilize R128 precision
				// and quantize to stabilize triangulation over a wide range of
				// distances.
				points[i] = Vector3(Vector3i((src_points[i] - rect.position) / longest_axis * QUANTIZATION_MAX)) / QUANTIZATION_MAX;
			}

			const real_t delta_max = Math::sqrt(2.0) * 100.0;
			Vector3 center = Vector3(0.5, 0.5, 0.5);

			// The larger the root simplex is, the more likely it is that the
			// triangulation is convex. If it's not absolutely huge, there can
			// be missing simplices that are not created for the outermost faces
			// of the point cloud if the point density is very low there.
			points[point_count + 0] = center + Vector3(0, 1, 0) * delta_max;
			points[point_count + 1] = center + Vector3(0, -1, 1) * delta_max;
			points[point_count + 2] = center + Vector3(1, -1, -1) * delta_max;
			points[point_count + 3] = center + Vector3(-1, -1, -1) * delta_max;
		}

		List<Simplex *> acceleration_grid[ACCEL_GRID_SIZE][ACCEL_GRID_SIZE][ACCEL_GRID_SIZE];

		List<Simplex *> simplex_list;
		{
			//create root simplex
			Simplex *root = memnew(Simplex(point_count + 0, point_count + 1, point_count + 2, point_count + 3));
			root->SE = simplex_list.push_back(root);

			for (uint32_t i = 0; i < ACCEL_GRID_SIZE; i++) {
				for (uint32_t j = 0; j < ACCEL_GRID_SIZE; j++) {
					for (uint32_t k = 0; k < ACCEL_GRID_SIZE; k++) {
						GridPos gp;
						gp.E = acceleration_grid[i][j][k].push_back(root);
						gp.pos = Vector3i(i, j, k);
						root->grid_positions.push_back(gp);
					}
				}
			}

			circum_sphere_compute(points, root);
		}

		AHashMap<Triangle, uint32_t, TriangleHasher> triangles_inserted;
		LocalVector<Triangle> triangles;

		for (uint32_t i = 0; i < point_count; i++) {
			bool unique = true;
			for (uint32_t j = i + 1; j < point_count; j++) {
				if (points[i] == points[j]) {
					unique = false;
					break;
				}
			}
			if (!unique) {
				continue;
			}

			Vector3i grid_pos = Vector3i(points[i] * proportions * ACCEL_GRID_SIZE);
			grid_pos = grid_pos.clampi(0, ACCEL_GRID_SIZE - 1);

			for (List<Simplex *>::Element *E = acceleration_grid[grid_pos.x][grid_pos.y][grid_pos.z].front(); E;) {
				List<Simplex *>::Element *N = E->next(); //may be deleted

				Simplex *simplex = E->get();

				if (simplex_contains(points, *simplex, i)) {
					static const uint32_t triangle_order[4][3] = {
						{ 0, 1, 2 },
						{ 0, 1, 3 },
						{ 0, 2, 3 },
						{ 1, 2, 3 },
					};

					for (uint32_t k = 0; k < 4; k++) {
						Triangle t = Triangle(simplex->points[triangle_order[k][0]], simplex->points[triangle_order[k][1]], simplex->points[triangle_order[k][2]]);
						uint32_t *p = triangles_inserted.getptr(t);
						if (p) {
							// This Delaunay implementation uses the Bowyer-Watson algorithm.
							// The rule is that you don't reuse any triangles that were
							// shared by any of the retriangulated simplices.
							triangles[*p].bad = true;
						} else {
							triangles_inserted.insert(t, triangles.size());
							triangles.push_back(t);
						}
					}

					simplex_list.erase(simplex->SE);

					for (const GridPos &gp : simplex->grid_positions) {
						Vector3i p = gp.pos;
						acceleration_grid[p.x][p.y][p.z].erase(gp.E);
					}
					memdelete(simplex);
				}
				E = N;
			}

			for (const Triangle &triangle : triangles) {
				if (triangle.bad) {
					continue;
				}
				Simplex *new_simplex = memnew(Simplex(triangle.triangle[0], triangle.triangle[1], triangle.triangle[2], i));
				circum_sphere_compute(points, new_simplex);
				new_simplex->SE = simplex_list.push_back(new_simplex);
				{
					Vector3 center;
					center.x = double(new_simplex->circum_center_x);
					center.y = double(new_simplex->circum_center_y);
					center.z = double(new_simplex->circum_center_z);

					const real_t radius2 = Math::sqrt(double(new_simplex->circum_r2)) + 0.0001;
					Vector3 extents = Vector3(radius2, radius2, radius2);
					Vector3i from = Vector3i((center - extents) * proportions * ACCEL_GRID_SIZE);
					Vector3i to = Vector3i((center + extents) * proportions * ACCEL_GRID_SIZE);
					from = from.clampi(0, ACCEL_GRID_SIZE - 1);
					to = to.clampi(0, ACCEL_GRID_SIZE - 1);

					for (int32_t x = from.x; x <= to.x; x++) {
						for (int32_t y = from.y; y <= to.y; y++) {
							for (int32_t z = from.z; z <= to.z; z++) {
								GridPos gp;
								gp.pos = Vector3(x, y, z);
								gp.E = acceleration_grid[x][y][z].push_back(new_simplex);
								new_simplex->grid_positions.push_back(gp);
							}
						}
					}
				}
			}

			triangles.clear();
			triangles_inserted.clear();
		}

		//print_line("end with simplices: " + itos(simplex_list.size()));
		Vector<OutputSimplex> ret_simplices;
		ret_simplices.resize(simplex_list.size());
		OutputSimplex *ret_simplicesw = ret_simplices.ptrw();
		uint32_t simplices_written = 0;

		for (Simplex *simplex : simplex_list) {
			bool invalid = false;
			for (int j = 0; j < 4; j++) {
				if (simplex->points[j] >= point_count) {
					invalid = true;
					break;
				}
			}
			if (invalid || simplex_is_coplanar(src_points, *simplex)) {
				memdelete(simplex);
				continue;
			}

			ret_simplicesw[simplices_written].points[0] = simplex->points[0];
			ret_simplicesw[simplices_written].points[1] = simplex->points[1];
			ret_simplicesw[simplices_written].points[2] = simplex->points[2];
			ret_simplicesw[simplices_written].points[3] = simplex->points[3];
			simplices_written++;
			memdelete(simplex);
		}

		ret_simplices.resize(simplices_written);

		memfree(points);

		return ret_simplices;
	}
};

// --- End core/math/delaunay_3d.h ---
#include "core/math/face3.h"
#include "core/math/vector2.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"

namespace Geometry3D {
void get_closest_points_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1, Vector3 &r_ps, Vector3 &r_qt);
real_t get_closest_distance_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1);

inline bool ray_intersects_triangle(const Vector3 &p_from, const Vector3 &p_dir, const Vector3 &p_v0, const Vector3 &p_v1, const Vector3 &p_v2, Vector3 *r_res = nullptr) {
	Vector3 e1 = p_v1 - p_v0;
	Vector3 e2 = p_v2 - p_v0;
	Vector3 h = p_dir.cross(e2);
	real_t a = e1.dot(h);
	if (Math::is_zero_approx(a)) { // Parallel test.
		return false;
	}

	real_t f = 1.0f / a;

	Vector3 s = p_from - p_v0;
	real_t u = f * s.dot(h);

	if ((u < 0.0f) || (u > 1.0f)) {
		return false;
	}

	Vector3 q = s.cross(e1);

	real_t v = f * p_dir.dot(q);

	if ((v < 0.0f) || (u + v > 1.0f)) {
		return false;
	}

	// At this stage we can compute t to find out where
	// the intersection point is on the line.
	real_t t = f * e2.dot(q);

	if (t > 0.00001f) { // ray intersection
		if (r_res) {
			*r_res = p_from + p_dir * t;
		}
		return true;
	} else { // This means that there is a line intersection but not a ray intersection.
		return false;
	}
}

inline bool segment_intersects_triangle(const Vector3 &p_from, const Vector3 &p_to, const Vector3 &p_v0, const Vector3 &p_v1, const Vector3 &p_v2, Vector3 *r_res = nullptr) {
	Vector3 rel = p_to - p_from;
	Vector3 e1 = p_v1 - p_v0;
	Vector3 e2 = p_v2 - p_v0;
	Vector3 h = rel.cross(e2);
	real_t a = e1.dot(h);
	if (Math::is_zero_approx(a)) { // Parallel test.
		return false;
	}

	real_t f = 1.0f / a;

	Vector3 s = p_from - p_v0;
	real_t u = f * s.dot(h);

	if ((u < 0.0f) || (u > 1.0f)) {
		return false;
	}

	Vector3 q = s.cross(e1);

	real_t v = f * rel.dot(q);

	if ((v < 0.0f) || (u + v > 1.0f)) {
		return false;
	}

	// At this stage we can compute t to find out where
	// the intersection point is on the line.
	real_t t = f * e2.dot(q);

	if (t > (real_t)CMP_EPSILON && t <= 1.0f) { // Ray intersection.
		if (r_res) {
			*r_res = p_from + rel * t;
		}
		return true;
	} else { // This means that there is a line intersection but not a ray intersection.
		return false;
	}
}

inline bool segment_intersects_sphere(const Vector3 &p_from, const Vector3 &p_to, const Vector3 &p_sphere_pos, real_t p_sphere_radius, Vector3 *r_res = nullptr, Vector3 *r_norm = nullptr) {
	Vector3 sphere_pos = p_sphere_pos - p_from;
	Vector3 rel = (p_to - p_from);
	real_t rel_l = rel.length();
	if (rel_l < (real_t)CMP_EPSILON) {
		return false; // Both points are the same.
	}
	Vector3 normal = rel / rel_l;

	real_t sphere_d = normal.dot(sphere_pos);

	real_t ray_distance = sphere_pos.distance_to(normal * sphere_d);

	if (ray_distance >= p_sphere_radius) {
		return false;
	}

	real_t inters_d2 = p_sphere_radius * p_sphere_radius - ray_distance * ray_distance;
	real_t inters_d = sphere_d;

	if (inters_d2 >= (real_t)CMP_EPSILON) {
		inters_d -= Math::sqrt(inters_d2);
	}

	// Check in segment.
	if (inters_d < 0 || inters_d > rel_l) {
		return false;
	}

	Vector3 result = p_from + normal * inters_d;

	if (r_res) {
		*r_res = result;
	}
	if (r_norm) {
		*r_norm = (result - p_sphere_pos).normalized();
	}

	return true;
}

inline bool segment_intersects_cylinder(const Vector3 &p_from, const Vector3 &p_to, real_t p_height, real_t p_radius, Vector3 *r_res = nullptr, Vector3 *r_norm = nullptr, int p_cylinder_axis = 2) {
	Vector3 rel = (p_to - p_from);
	real_t rel_l = rel.length();
	if (rel_l < (real_t)CMP_EPSILON) {
		return false; // Both points are the same.
	}

	ERR_FAIL_COND_V(p_cylinder_axis < 0, false);
	ERR_FAIL_COND_V(p_cylinder_axis > 2, false);
	Vector3 cylinder_axis;
	cylinder_axis[p_cylinder_axis] = 1.0f;

	// First check if they are parallel.
	Vector3 normal = (rel / rel_l);
	Vector3 crs = normal.cross(cylinder_axis);
	real_t crs_l = crs.length();

	Vector3 axis_dir;

	if (crs_l < (real_t)CMP_EPSILON) {
		Vector3 side_axis;
		side_axis[(p_cylinder_axis + 1) % 3] = 1.0f; // Any side axis OK.
		axis_dir = side_axis;
	} else {
		axis_dir = crs / crs_l;
	}

	real_t dist = axis_dir.dot(p_from);

	if (dist >= p_radius) {
		return false; // Too far away.
	}

	// Convert to 2D.
	real_t w2 = p_radius * p_radius - dist * dist;
	if (w2 < (real_t)CMP_EPSILON) {
		return false; // Avoid numerical error.
	}
	Size2 size(Math::sqrt(w2), p_height * 0.5f);

	Vector3 side_dir = axis_dir.cross(cylinder_axis).normalized();

	Vector2 from2D(side_dir.dot(p_from), p_from[p_cylinder_axis]);
	Vector2 to2D(side_dir.dot(p_to), p_to[p_cylinder_axis]);

	real_t min = 0, max = 1;

	int axis = -1;

	for (int i = 0; i < 2; i++) {
		real_t seg_from = from2D[i];
		real_t seg_to = to2D[i];
		real_t box_begin = -size[i];
		real_t box_end = size[i];
		real_t cmin, cmax;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;

		} else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	// Convert to 3D again.
	Vector3 result = p_from + (rel * min);
	Vector3 res_normal = result;

	if (axis == 0) {
		res_normal[p_cylinder_axis] = 0;
	} else {
		int axis_side = (p_cylinder_axis + 1) % 3;
		res_normal[axis_side] = 0;
		axis_side = (axis_side + 1) % 3;
		res_normal[axis_side] = 0;
	}

	res_normal.normalize();

	if (r_res) {
		*r_res = result;
	}
	if (r_norm) {
		*r_norm = res_normal;
	}

	return true;
}

inline bool segment_intersects_convex(const Vector3 &p_from, const Vector3 &p_to, const Plane *p_planes, int p_plane_count, Vector3 *r_res, Vector3 *r_norm) {
	real_t min = -1e20, max = 1e20;

	Vector3 rel = p_to - p_from;
	real_t rel_l = rel.length();

	if (rel_l < (real_t)CMP_EPSILON) {
		return false;
	}

	Vector3 dir = rel / rel_l;

	int min_index = -1;

	for (int i = 0; i < p_plane_count; i++) {
		const Plane &p = p_planes[i];

		real_t den = p.normal.dot(dir);

		if (Math::abs(den) <= (real_t)CMP_EPSILON) {
			if (p.is_point_over(p_from)) {
				// Separating plane.
				return false;
			}
			continue; // Ignore parallel plane.
		}

		real_t dist = -p.distance_to(p_from) / den;

		if (den > 0) {
			// Backwards facing plane.
			if (dist < max) {
				max = dist;
			}
		} else {
			// Front facing plane.
			if (dist > min) {
				min = dist;
				min_index = i;
			}
		}
	}

	if (max <= min || min < 0 || min > rel_l || min_index == -1) { // Exit conditions.
		return false; // No intersection.
	}

	if (r_res) {
		*r_res = p_from + dir * min;
	}
	if (r_norm) {
		*r_norm = p_planes[min_index].normal;
	}

	return true;
}

inline Vector3 get_closest_point_to_segment(const Vector3 &p_point, const Vector3 &p_segment_a, const Vector3 &p_segment_b) {
	Vector3 p = p_point - p_segment_a;
	Vector3 n = p_segment_b - p_segment_a;
	real_t l2 = n.length_squared();
	if (l2 < 1e-20f) {
		return p_segment_a; // Both points are the same, just give any.
	}

	real_t d = n.dot(p) / l2;

	if (d <= 0.0f) {
		return p_segment_a; // Before first point.
	} else if (d >= 1.0f) {
		return p_segment_b; // After first point.
	} else {
		return p_segment_a + n * d; // Inside.
	}
}

#ifndef DISABLE_DEPRECATED
inline Vector3 get_closest_point_to_segment(const Vector3 &p_point, const Vector3 *p_segment) {
	return get_closest_point_to_segment(p_point, p_segment[0], p_segment[1]);
}
#endif // DISABLE_DEPRECATED

inline Vector3 get_closest_point_to_segment_uncapped(const Vector3 &p_point, const Vector3 &p_segment_a, const Vector3 &p_segment_b) {
	Vector3 p = p_point - p_segment_a;
	Vector3 n = p_segment_b - p_segment_a;
	real_t l2 = n.length_squared();
	if (l2 < 1e-20f) {
		return p_segment_a; // Both points are the same, just give any.
	}

	real_t d = n.dot(p) / l2;

	return p_segment_a + n * d; // Inside.
}

#ifndef DISABLE_DEPRECATED
inline Vector3 get_closest_point_to_segment_uncapped(const Vector3 &p_point, const Vector3 *p_segment) {
	return get_closest_point_to_segment_uncapped(p_point, p_segment[0], p_segment[1]);
}
#endif // DISABLE_DEPRECATED

inline bool point_in_projected_triangle(const Vector3 &p_point, const Vector3 &p_v1, const Vector3 &p_v2, const Vector3 &p_v3) {
	Vector3 face_n = (p_v1 - p_v3).cross(p_v1 - p_v2);

	Vector3 n1 = (p_point - p_v3).cross(p_point - p_v2);

	if (face_n.dot(n1) < 0) {
		return false;
	}

	Vector3 n2 = (p_v1 - p_v3).cross(p_v1 - p_point);

	if (face_n.dot(n2) < 0) {
		return false;
	}

	Vector3 n3 = (p_v1 - p_point).cross(p_v1 - p_v2);

	if (face_n.dot(n3) < 0) {
		return false;
	}

	return true;
}

inline bool triangle_sphere_intersection_test(const Vector3 &p_triangle_a, const Vector3 &p_triangle_b, const Vector3 &p_triangle_c, const Vector3 &p_normal, const Vector3 &p_sphere_pos, real_t p_sphere_radius, Vector3 &r_triangle_contact, Vector3 &r_sphere_contact) {
	real_t d = p_normal.dot(p_sphere_pos) - p_normal.dot(p_triangle_a);

	if (d > p_sphere_radius || d < -p_sphere_radius) {
		// Not touching the plane of the face, return.
		return false;
	}

	Vector3 contact = p_sphere_pos - (p_normal * d);

	/** 2nd) TEST INSIDE TRIANGLE **/

	if (Geometry3D::point_in_projected_triangle(contact, p_triangle_a, p_triangle_b, p_triangle_c)) {
		r_triangle_contact = contact;
		r_sphere_contact = p_sphere_pos - p_normal * p_sphere_radius;
		//printf("solved inside triangle\n");
		return true;
	}

	/** 3rd TEST INSIDE EDGE CYLINDERS **/

	const Vector3 verts[4] = { p_triangle_a, p_triangle_b, p_triangle_c, p_triangle_a }; // for() friendly

	for (int i = 0; i < 3; i++) {
		// Check edge cylinder.

		Vector3 n1 = verts[i] - verts[i + 1];
		Vector3 n2 = p_sphere_pos - verts[i + 1];

		///@TODO Maybe discard by range here to make the algorithm quicker.

		// Check point within cylinder radius.
		Vector3 axis = n1.cross(n2).cross(n1);
		axis.normalize();

		real_t ad = axis.dot(n2);

		if (Math::abs(ad) > p_sphere_radius) {
			// No chance with this edge, too far away.
			continue;
		}

		// Check point within edge capsule cylinder.
		/** 4th TEST INSIDE EDGE POINTS **/

		real_t sphere_at = n1.dot(n2);

		if (sphere_at >= 0 && sphere_at < n1.dot(n1)) {
			r_triangle_contact = p_sphere_pos - axis * (axis.dot(n2));
			r_sphere_contact = p_sphere_pos - axis * p_sphere_radius;
			// Point inside here.
			return true;
		}

		real_t r2 = p_sphere_radius * p_sphere_radius;

		if (n2.length_squared() < r2) {
			Vector3 n = (p_sphere_pos - verts[i + 1]).normalized();

			r_triangle_contact = verts[i + 1];
			r_sphere_contact = p_sphere_pos - n * p_sphere_radius;
			return true;
		}

		if (n2.distance_squared_to(n1) < r2) {
			Vector3 n = (p_sphere_pos - verts[i]).normalized();

			r_triangle_contact = verts[i];
			r_sphere_contact = p_sphere_pos - n * p_sphere_radius;
			return true;
		}

		break; // It's pointless to continue at this point, so save some CPU cycles.
	}

	return false;
}

#ifndef DISABLE_DEPRECATED
inline bool triangle_sphere_intersection_test(const Vector3 *p_triangle, const Vector3 &p_normal, const Vector3 &p_sphere_pos, real_t p_sphere_radius, Vector3 &r_triangle_contact, Vector3 &r_sphere_contact) {
	return triangle_sphere_intersection_test(p_triangle[0], p_triangle[1], p_triangle[2], p_normal, p_sphere_pos, p_sphere_radius, r_triangle_contact, r_sphere_contact);
}
#endif // DISABLE_DEPRECATED

inline Vector<Vector3> clip_polygon(const Vector<Vector3> &p_polygon, const Plane &p_plane) {
	enum LocationCache {
		LOC_INSIDE = 1,
		LOC_BOUNDARY = 0,
		LOC_OUTSIDE = -1
	};

	if (p_polygon.is_empty()) {
		return p_polygon;
	}

	int *location_cache = (int *)alloca(sizeof(int) * p_polygon.size());
	int inside_count = 0;
	int outside_count = 0;

	for (int a = 0; a < p_polygon.size(); a++) {
		real_t dist = p_plane.distance_to(p_polygon[a]);
		if (dist < (real_t)-CMP_POINT_IN_PLANE_EPSILON) {
			location_cache[a] = LOC_INSIDE;
			inside_count++;
		} else {
			if (dist > (real_t)CMP_POINT_IN_PLANE_EPSILON) {
				location_cache[a] = LOC_OUTSIDE;
				outside_count++;
			} else {
				location_cache[a] = LOC_BOUNDARY;
			}
		}
	}

	if (outside_count == 0) {
		return p_polygon; // No changes.
	} else if (inside_count == 0) {
		return Vector<Vector3>(); // Empty.
	}

	long previous = p_polygon.size() - 1;
	Vector<Vector3> clipped;

	for (int index = 0; index < p_polygon.size(); index++) {
		int loc = location_cache[index];
		if (loc == LOC_OUTSIDE) {
			if (location_cache[previous] == LOC_INSIDE) {
				const Vector3 &v1 = p_polygon[previous];
				const Vector3 &v2 = p_polygon[index];

				Vector3 segment = v1 - v2;
				real_t den = p_plane.normal.dot(segment);
				real_t dist = p_plane.distance_to(v1) / den;
				dist = -dist;
				clipped.push_back(v1 + segment * dist);
			}
		} else {
			const Vector3 &v1 = p_polygon[index];
			if ((loc == LOC_INSIDE) && (location_cache[previous] == LOC_OUTSIDE)) {
				const Vector3 &v2 = p_polygon[previous];
				Vector3 segment = v1 - v2;
				real_t den = p_plane.normal.dot(segment);
				real_t dist = p_plane.distance_to(v1) / den;
				dist = -dist;
				clipped.push_back(v1 + segment * dist);
			}

			clipped.push_back(v1);
		}

		previous = index;
	}

	return clipped;
}

inline Vector<int32_t> tetrahedralize_delaunay(const Vector<Vector3> &p_points) {
	Vector<Delaunay3D::OutputSimplex> tetr = Delaunay3D::tetrahedralize(p_points);
	Vector<int32_t> tetrahedrons;

	tetrahedrons.resize(4 * tetr.size());
	int32_t *ptr = tetrahedrons.ptrw();
	for (int i = 0; i < tetr.size(); i++) {
		*ptr++ = tetr[i].points[0];
		*ptr++ = tetr[i].points[1];
		*ptr++ = tetr[i].points[2];
		*ptr++ = tetr[i].points[3];
	}
	return tetrahedrons;
}

// Create a "wrap" that encloses the given geometry.
Vector<Face3> wrap_geometry(const Vector<Face3> &p_array, real_t *p_error = nullptr);

struct MeshData {
	struct Face {
		Plane plane;
		LocalVector<int> indices;
	};

	LocalVector<Face> faces;

	struct Edge {
		int vertex_a, vertex_b;
		int face_a, face_b;
	};

	LocalVector<Edge> edges;

	LocalVector<Vector3> vertices;

	void optimize_vertices();
};

MeshData build_convex_mesh(const Vector<Plane> &p_planes);
Vector<Plane> build_sphere_planes(real_t p_radius, int p_lats, int p_lons, Vector3::Axis p_axis = Vector3::AXIS_Z);
Vector<Plane> build_box_planes(const Vector3 &p_extents);
Vector<Plane> build_cylinder_planes(real_t p_radius, real_t p_height, int p_sides, Vector3::Axis p_axis = Vector3::AXIS_Z);
Vector<Plane> build_capsule_planes(real_t p_radius, real_t p_height, int p_sides, int p_lats, Vector3::Axis p_axis = Vector3::AXIS_Z);

Vector<Vector3> compute_convex_mesh_points(const Plane *p_planes, int p_plane_count);

#define FINDMINMAX(x0, x1, x2, min, max) \
	min = max = x0; \
	if (x1 < min) { \
		min = x1; \
	} \
	if (x1 > max) { \
		max = x1; \
	} \
	if (x2 < min) { \
		min = x2; \
	} \
	if (x2 > max) { \
		max = x2; \
	}

_FORCE_INLINE_ bool planeBoxOverlap(Vector3 p_normal, real_t p_d, Vector3 p_maxbox) {
	int q;
	Vector3 vmin, vmax;
	for (q = 0; q <= 2; q++) {
		if (p_normal[q] > 0.0f) {
			vmin[q] = -p_maxbox[q];
			vmax[q] = p_maxbox[q];
		} else {
			vmin[q] = p_maxbox[q];
			vmax[q] = -p_maxbox[q];
		}
	}
	if (p_normal.dot(vmin) + p_d > 0.0f) {
		return false;
	}
	if (p_normal.dot(vmax) + p_d >= 0.0f) {
		return true;
	}

	return false;
}

/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb) \
	p0 = a * v0.y - b * v0.z; \
	p2 = a * v2.y - b * v2.z; \
	if (p0 < p2) { \
		min = p0; \
		max = p2; \
	} else { \
		min = p2; \
		max = p0; \
	} \
	rad = fa * p_box_half_size.y + fb * p_box_half_size.z; \
	if (min > rad || max < -rad) { \
		return false; \
	}

#define AXISTEST_X2(a, b, fa, fb) \
	p0 = a * v0.y - b * v0.z; \
	p1 = a * v1.y - b * v1.z; \
	if (p0 < p1) { \
		min = p0; \
		max = p1; \
	} else { \
		min = p1; \
		max = p0; \
	} \
	rad = fa * p_box_half_size.y + fb * p_box_half_size.z; \
	if (min > rad || max < -rad) { \
		return false; \
	}

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb) \
	p0 = -a * v0.x + b * v0.z; \
	p2 = -a * v2.x + b * v2.z; \
	if (p0 < p2) { \
		min = p0; \
		max = p2; \
	} else { \
		min = p2; \
		max = p0; \
	} \
	rad = fa * p_box_half_size.x + fb * p_box_half_size.z; \
	if (min > rad || max < -rad) { \
		return false; \
	}

#define AXISTEST_Y1(a, b, fa, fb) \
	p0 = -a * v0.x + b * v0.z; \
	p1 = -a * v1.x + b * v1.z; \
	if (p0 < p1) { \
		min = p0; \
		max = p1; \
	} else { \
		min = p1; \
		max = p0; \
	} \
	rad = fa * p_box_half_size.x + fb * p_box_half_size.z; \
	if (min > rad || max < -rad) { \
		return false; \
	}

/*======================== Z-tests ========================*/
#define AXISTEST_Z12(a, b, fa, fb) \
	p1 = a * v1.x - b * v1.y; \
	p2 = a * v2.x - b * v2.y; \
	if (p2 < p1) { \
		min = p2; \
		max = p1; \
	} else { \
		min = p1; \
		max = p2; \
	} \
	rad = fa * p_box_half_size.x + fb * p_box_half_size.y; \
	if (min > rad || max < -rad) { \
		return false; \
	}

#define AXISTEST_Z0(a, b, fa, fb) \
	p0 = a * v0.x - b * v0.y; \
	p1 = a * v1.x - b * v1.y; \
	if (p0 < p1) { \
		min = p0; \
		max = p1; \
	} else { \
		min = p1; \
		max = p0; \
	} \
	rad = fa * p_box_half_size.x + fb * p_box_half_size.y; \
	if (min > rad || max < -rad) { \
		return false; \
	}

_FORCE_INLINE_ bool triangle_box_overlap(const Vector3 &p_box_center, const Vector3 p_box_half_size, const Vector3 *p_triverts) {
	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	real_t min, max, p0, p1, p2, rad, fex, fey, fez;

	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */

	const Vector3 v0 = p_triverts[0] - p_box_center;
	const Vector3 v1 = p_triverts[1] - p_box_center;
	const Vector3 v2 = p_triverts[2] - p_box_center;

	/* compute triangle edges */
	const Vector3 e0 = v1 - v0; /* tri edge 0 */
	const Vector3 e1 = v2 - v1; /* tri edge 1 */
	const Vector3 e2 = v0 - v2; /* tri edge 2 */

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	fex = Math::abs(e0.x);
	fey = Math::abs(e0.y);
	fez = Math::abs(e0.z);
	AXISTEST_X01(e0.z, e0.y, fez, fey);
	AXISTEST_Y02(e0.z, e0.x, fez, fex);
	AXISTEST_Z12(e0.y, e0.x, fey, fex);

	fex = Math::abs(e1.x);
	fey = Math::abs(e1.y);
	fez = Math::abs(e1.z);
	AXISTEST_X01(e1.z, e1.y, fez, fey);
	AXISTEST_Y02(e1.z, e1.x, fez, fex);
	AXISTEST_Z0(e1.y, e1.x, fey, fex);

	fex = Math::abs(e2.x);
	fey = Math::abs(e2.y);
	fez = Math::abs(e2.z);
	AXISTEST_X2(e2.z, e2.y, fez, fey);
	AXISTEST_Y1(e2.z, e2.x, fez, fex);
	AXISTEST_Z12(e2.y, e2.x, fey, fex);

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	FINDMINMAX(v0.x, v1.x, v2.x, min, max);
	if (min > p_box_half_size.x || max < -p_box_half_size.x) {
		return false;
	}

	/* test in Y-direction */
	FINDMINMAX(v0.y, v1.y, v2.y, min, max);
	if (min > p_box_half_size.y || max < -p_box_half_size.y) {
		return false;
	}

	/* test in Z-direction */
	FINDMINMAX(v0.z, v1.z, v2.z, min, max);
	if (min > p_box_half_size.z || max < -p_box_half_size.z) {
		return false;
	}

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	const Vector3 normal = e0.cross(e1);
	const real_t d = -normal.dot(v0); /* plane eq: normal.x+d=0 */
	return planeBoxOverlap(normal, d, p_box_half_size); /* if true, box and triangle overlaps */
}

Vector<uint32_t> generate_edf(const Vector<bool> &p_voxels, const Vector3i &p_size, bool p_negative);
Vector<int8_t> generate_sdf8(const Vector<uint32_t> &p_positive, const Vector<uint32_t> &p_negative);

inline Vector3 triangle_get_barycentric_coords(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c, const Vector3 &p_pos) {
	const Vector3 v0 = p_b - p_a;
	const Vector3 v1 = p_c - p_a;
	const Vector3 v2 = p_pos - p_a;

	const real_t d00 = v0.dot(v0);
	const real_t d01 = v0.dot(v1);
	const real_t d11 = v1.dot(v1);
	const real_t d20 = v2.dot(v0);
	const real_t d21 = v2.dot(v1);
	const real_t denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		return Vector3(); //invalid triangle, return empty
	}
	const real_t v = (d11 * d20 - d01 * d21) / denom;
	const real_t w = (d00 * d21 - d01 * d20) / denom;
	const real_t u = 1.0f - v - w;
	return Vector3(u, v, w);
}

inline Color tetrahedron_get_barycentric_coords(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c, const Vector3 &p_d, const Vector3 &p_pos) {
	const Vector3 vap = p_pos - p_a;
	const Vector3 vbp = p_pos - p_b;

	const Vector3 vab = p_b - p_a;
	const Vector3 vac = p_c - p_a;
	const Vector3 vad = p_d - p_a;

	const Vector3 vbc = p_c - p_b;
	const Vector3 vbd = p_d - p_b;
	// ScTP computes the scalar triple product
#define STP(m_a, m_b, m_c) ((m_a).dot((m_b).cross((m_c))))
	const real_t va6 = STP(vbp, vbd, vbc);
	const real_t vb6 = STP(vap, vac, vad);
	const real_t vc6 = STP(vap, vad, vab);
	const real_t vd6 = STP(vap, vab, vac);
	const real_t v6 = 1 / STP(vab, vac, vad);
	return Color(va6 * v6, vb6 * v6, vc6 * v6, vd6 * v6);
#undef STP
}

_FORCE_INLINE_ Vector3 octahedron_map_decode(const Vector2 &p_uv) {
	// https://twitter.com/Stubbesaurus/status/937994790553227264
	const Vector2 f = p_uv * 2.0f - Vector2(1.0f, 1.0f);
	Vector3 n = Vector3(f.x, f.y, 1.0f - Math::abs(f.x) - Math::abs(f.y));
	const real_t t = CLAMP(-n.z, 0.0f, 1.0f);
	n.x += n.x >= 0 ? -t : t;
	n.y += n.y >= 0 ? -t : t;
	return n.normalized();
}
} //namespace Geometry3D
