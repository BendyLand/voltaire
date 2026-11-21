/**************************************************************************/
/*  geometry_2d.cpp                                                       */
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

#include "geometry_2d.h"

#include "core/math/math_funcs_binary.h"

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Walloc-zero")
#include <thirdparty/clipper2/include/clipper2/clipper.h>
VLTR_GCC_WARNING_POP
#include <thirdparty/misc/polypartition.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <thirdparty/misc/stb_rect_pack.h>

const int clipper_precision = 5; // Based on CMP_EPSILON.

void Geometry2D::merge_many_polygons(const Vector<Vector<Vector2>> &p_polygons, Vector<Vector<Vector2>> &r_out_polygons, Vector<Vector<Vector2>> &r_out_holes) {
	using namespace Clipper2Lib;

	PathsD subjects;
	for (const Vector<Vector2> &polygon : p_polygons) {
		PathD path(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			const Vector2 &point = polygon[i];
			path[i] = PointD(point.x, point.y);
		}
		subjects.push_back(path);
	}

	PathsD solution = Union(subjects, FillRule::NonZero);
	solution = SimplifyPaths(solution, 0.01);

	r_out_polygons.clear();
	r_out_holes.clear();
	for (PathsD::size_type i = 0; i < solution.size(); ++i) {
		PathD &path = solution[i];

		Vector<Point2> output_polygon;
		output_polygon.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			output_polygon.set(j, Vector2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		if (IsPositive(path)) {
			r_out_polygons.push_back(output_polygon);
		} else {
			r_out_holes.push_back(output_polygon);
		}
	}
}

Vector<Vector<Vector2>> Geometry2D::decompose_many_polygons_in_convex(const Vector<Vector<Point2>> &p_polygons, const Vector<Vector<Point2>> &p_holes) {
	Vector<Vector<Vector2>> decomp;
	List<TPPLPoly> in_poly, out_poly;

	for (const Vector<Vector2> &polygon : p_polygons) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CCW);
		in_poly.push_back(inp);
	}
	for (const Vector<Vector2> &polygon : p_holes) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CW);
		inp.SetHole(true);
		in_poly.push_back(inp);
	}
	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // Failed.
		ERR_PRINT("Convex decomposing failed!");
		return decomp;
	}

	decomp.resize(out_poly.size());
	int idx = 0;
	for (TPPLPoly &tp : out_poly) {
		decomp.write[idx].resize(tp.GetNumPoints());

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			decomp.write[idx].write[i] = tp.GetPoint(i);
		}

		idx++;
	}

	return decomp;
}

Vector<Vector<Vector2>> Geometry2D::decompose_polygon_in_convex(const Vector<Point2> &p_polygon) {
	return Geometry2D::decompose_many_polygons_in_convex({ p_polygon }, {});
}

struct _AtlasWorkRect {
	Size2i s;
	Point2i p;
	int idx = 0;
	_FORCE_INLINE_ bool operator<(const _AtlasWorkRect &p_r) const { return s.width > p_r.s.width; }
};

struct _AtlasWorkRectResult {
	Vector<_AtlasWorkRect> result;
	int max_w = 0;
	int max_h = 0;
};

void Geometry2D::make_atlas(const Vector<Size2i> &p_rects, Vector<Point2i> &r_result, Size2i &r_size) {
	// Super simple, almost brute force scanline stacking fitter.
	// It's pretty basic for now, but it tries to make sure that the aspect ratio of the
	// resulting atlas is somehow square. This is necessary because video cards have limits
	// on texture size (usually 2048 or 4096), so the squarer a texture, the more the chances
	// that it will work in every hardware.
	// For example, it will prioritize a 1024x1024 atlas (works everywhere) instead of a
	// 256x8192 atlas (won't work anywhere).

	ERR_FAIL_COND(p_rects.is_empty());
	for (int i = 0; i < p_rects.size(); i++) {
		ERR_FAIL_COND(p_rects[i].width <= 0);
		ERR_FAIL_COND(p_rects[i].height <= 0);
	}

	Vector<_AtlasWorkRect> wrects;
	wrects.resize(p_rects.size());
	for (int i = 0; i < p_rects.size(); i++) {
		wrects.write[i].s = p_rects[i];
		wrects.write[i].idx = i;
	}
	wrects.sort();
	int widest = wrects[0].s.width;

	Vector<_AtlasWorkRectResult> results;

	for (int i = 0; i <= 12; i++) {
		int w = 1 << i;
		int max_h = 0;
		int max_w = 0;
		if (w < widest) {
			continue;
		}

		Vector<int> hmax;
		hmax.resize(w);
		for (int j = 0; j < w; j++) {
			hmax.write[j] = 0;
		}

		// Place them.
		int ofs = 0;
		int limit_h = 0;
		for (int j = 0; j < wrects.size(); j++) {
			if (ofs + wrects[j].s.width > w) {
				ofs = 0;
			}

			int from_y = 0;
			for (int k = 0; k < wrects[j].s.width; k++) {
				if (hmax[ofs + k] > from_y) {
					from_y = hmax[ofs + k];
				}
			}

			wrects.write[j].p.x = ofs;
			wrects.write[j].p.y = from_y;
			int end_h = from_y + wrects[j].s.height;
			int end_w = ofs + wrects[j].s.width;
			if (ofs == 0) {
				limit_h = end_h;
			}

			for (int k = 0; k < wrects[j].s.width; k++) {
				hmax.write[ofs + k] = end_h;
			}

			if (end_h > max_h) {
				max_h = end_h;
			}

			if (end_w > max_w) {
				max_w = end_w;
			}

			if (ofs == 0 || end_h > limit_h) { // While h limit not reached, keep stacking.
				ofs += wrects[j].s.width;
			}
		}

		_AtlasWorkRectResult result;
		result.result = wrects;
		result.max_h = max_h;
		result.max_w = max_w;
		results.push_back(result);
	}

	// Find the result with the best aspect ratio.

	int best = -1;
	real_t best_aspect = 1e20;

	for (int i = 0; i < results.size(); i++) {
		real_t h = Math::next_power_of_2((uint32_t)results[i].max_h);
		real_t w = Math::next_power_of_2((uint32_t)results[i].max_w);
		real_t aspect = h > w ? h / w : w / h;
		if (aspect < best_aspect) {
			best = i;
			best_aspect = aspect;
		}
	}

	r_result.resize(p_rects.size());

	for (int i = 0; i < p_rects.size(); i++) {
		r_result.write[results[best].result[i].idx] = results[best].result[i].p;
	}

	r_size = Size2(results[best].max_w, results[best].max_h);
}

Vector<Vector<Point2>> Geometry2D::_polypaths_do_operation(PolyBooleanOperation p_op, const Vector<Point2> &p_polypath_a, const Vector<Point2> &p_polypath_b, bool p_is_a_open) {
	using namespace Clipper2Lib;

	ClipType op = ClipType::Union;

	switch (p_op) {
		case OPERATION_UNION:
			op = ClipType::Union;
			break;
		case OPERATION_DIFFERENCE:
			op = ClipType::Difference;
			break;
		case OPERATION_INTERSECTION:
			op = ClipType::Intersection;
			break;
		case OPERATION_XOR:
			op = ClipType::Xor;
			break;
	}

	PathD path_a(p_polypath_a.size());
	for (int i = 0; i != p_polypath_a.size(); ++i) {
		path_a[i] = PointD(p_polypath_a[i].x, p_polypath_a[i].y);
	}
	PathD path_b(p_polypath_b.size());
	for (int i = 0; i != p_polypath_b.size(); ++i) {
		path_b[i] = PointD(p_polypath_b[i].x, p_polypath_b[i].y);
	}

	ClipperD clp(clipper_precision); // Scale points up internally to attain the desired precision.
	clp.PreserveCollinear(false); // Remove redundant vertices.
	if (p_is_a_open) {
		clp.AddOpenSubject({ path_a });
	} else {
		clp.AddSubject({ path_a });
	}
	clp.AddClip({ path_b });

	PathsD paths;

	if (p_is_a_open) {
		PolyTreeD tree; // Needed to populate polylines.
		clp.Execute(op, FillRule::EvenOdd, tree, paths);
	} else {
		clp.Execute(op, FillRule::EvenOdd, paths); // Works on closed polygons only.
	}

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); i++) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath;
		polypath.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath);
	}
	return polypaths;
}

Vector<Vector<Point2>> Geometry2D::_polypath_offset(const Vector<Point2> &p_polypath, real_t p_delta, PolyJoinType p_join_type, PolyEndType p_end_type) {
	using namespace Clipper2Lib;

	JoinType jt = JoinType::Square;

	switch (p_join_type) {
		case JOIN_SQUARE:
			jt = JoinType::Square;
			break;
		case JOIN_ROUND:
			jt = JoinType::Round;
			break;
		case JOIN_MITER:
			jt = JoinType::Miter;
			break;
	}

	EndType et = EndType::Polygon;

	switch (p_end_type) {
		case END_POLYGON:
			et = EndType::Polygon;
			break;
		case END_JOINED:
			et = EndType::Joined;
			break;
		case END_BUTT:
			et = EndType::Butt;
			break;
		case END_SQUARE:
			et = EndType::Square;
			break;
		case END_ROUND:
			et = EndType::Round;
			break;
	}

	PathD polypath(p_polypath.size());
	for (int i = 0; i != p_polypath.size(); ++i) {
		polypath[i] = PointD(p_polypath[i].x, p_polypath[i].y);
	}

	// Inflate/deflate.
	PathsD paths = InflatePaths({ polypath }, p_delta, jt, et, 2.0, clipper_precision, 0.25);

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); ++i) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath2;
		polypath2.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath2.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath2);
	}
	return polypaths;
}

Vector<Vector3i> Geometry2D::partial_pack_rects(const Vector<Vector2i> &p_sizes, const Size2i &p_atlas_size) {
	Vector<stbrp_node> nodes;
	nodes.resize(p_atlas_size.width);
	memset(nodes.ptrw(), 0, sizeof(stbrp_node) * nodes.size());

	stbrp_context context;
	stbrp_init_target(&context, p_atlas_size.width, p_atlas_size.height, nodes.ptrw(), p_atlas_size.width);

	Vector<stbrp_rect> rects;
	rects.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		rects.write[i].id = i;
		rects.write[i].w = p_sizes[i].width;
		rects.write[i].h = p_sizes[i].height;
		rects.write[i].x = 0;
		rects.write[i].y = 0;
		rects.write[i].was_packed = 0;
	}

	stbrp_pack_rects(&context, rects.ptrw(), rects.size());

	Vector<Vector3i> ret;
	ret.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		ret.write[rects[i].id] = Vector3i(rects[i].x, rects[i].y, rects[i].was_packed != 0 ? 1 : 0);
	}

	return ret;
}

// --- triangulate.cpp ---

real_t Triangulate::get_area(const Vector<Vector2> &p_contour) {
	int n = p_contour.size();
	const Vector2 *c = &p_contour[0];

	real_t A = 0.0;

	for (int p = n - 1, q = 0; q < n; p = q++) {
		A += c[p].cross(c[q]);
	}
	return A * 0.5f;
}

/* `is_inside_triangle` decides if a point P is inside the triangle
 * defined by A, B, C. */
bool Triangulate::is_inside_triangle(real_t p_ax, real_t p_ay, real_t p_bx, real_t p_by, real_t p_cx, real_t p_cy, real_t p_px, real_t p_py, bool p_include_edges) {
	real_t ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
	real_t cCROSSap, bCROSScp, aCROSSbp;

	ax = p_cx - p_bx;
	ay = p_cy - p_by;
	bx = p_ax - p_cx;
	by = p_ay - p_cy;
	cx = p_bx - p_ax;
	cy = p_by - p_ay;
	apx = p_px - p_ax;
	apy = p_py - p_ay;
	bpx = p_px - p_bx;
	bpy = p_py - p_by;
	cpx = p_px - p_cx;
	cpy = p_py - p_cy;

	aCROSSbp = ax * bpy - ay * bpx;
	cCROSSap = cx * apy - cy * apx;
	bCROSScp = bx * cpy - by * cpx;

	if (p_include_edges) {
		return ((aCROSSbp > 0.0f) && (bCROSScp > 0.0f) && (cCROSSap > 0.0f));
	} else {
		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
}

bool Triangulate::snip(const Vector<Vector2> &p_contour, int p_u, int p_v, int p_w, int p_n, const Vector<int> &p_values, bool p_relaxed) {
	int p;
	real_t Ax, Ay, Bx, By, Cx, Cy, Px, Py;
	const Vector2 *contour = &p_contour[0];

	Ax = contour[p_values[p_u]].x;
	Ay = contour[p_values[p_u]].y;

	Bx = contour[p_values[p_v]].x;
	By = contour[p_values[p_v]].y;

	Cx = contour[p_values[p_w]].x;
	Cy = contour[p_values[p_w]].y;

	// It can happen that the triangulation ends up with three aligned vertices to deal with.
	// In this scenario, making the check below strict may reject the possibility of
	// forming a last triangle with these aligned vertices, preventing the triangulation
	// from completing.
	// To avoid that we allow zero-area triangles if all else failed.
	float threshold = p_relaxed ? -CMP_EPSILON : CMP_EPSILON;

	if (threshold > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax)))) {
		return false;
	}

	for (p = 0; p < p_n; p++) {
		if ((p == p_u) || (p == p_v) || (p == p_w)) {
			continue;
		}
		Px = contour[p_values[p]].x;
		Py = contour[p_values[p]].y;
		if (is_inside_triangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py, p_relaxed)) {
			return false;
		}
	}

	return true;
}

bool Triangulate::triangulate(const Vector<Vector2> &p_contour, Vector<int> &r_result) {
	/* allocate and initialize list of Vertices in polygon */

	int n = p_contour.size();
	if (n < 3) {
		return false;
	}

	Vector<int> V;
	V.resize(n);

	/* we want a counter-clockwise polygon in V */

	if (0.0f < get_area(p_contour)) {
		for (int v = 0; v < n; v++) {
			V.write[v] = v;
		}
	} else {
		for (int v = 0; v < n; v++) {
			V.write[v] = (n - 1) - v;
		}
	}

	bool relaxed = false;

	int nv = n;

	/*  remove nv-2 Vertices, creating 1 triangle every time */
	int count = 2 * nv; /* error detection */

	for (int v = nv - 1; nv > 2;) {
		/* if we loop, it is probably a non-simple polygon */
		if (0 >= (count--)) {
			if (relaxed) {
				//** Triangulate: ERROR - probable bad polygon!
				return false;
			} else {
				// There may be aligned vertices that the strict
				// checks prevent from triangulating. In this situation
				// we are better off adding flat triangles than
				// failing, so we relax the checks and try one last
				// round.
				// Only relaxing the constraints as a last resort avoids
				// degenerate triangles when they aren't necessary.
				count = 2 * nv;
				relaxed = true;
			}
		}

		/* three consecutive vertices in current polygon, <u,v,w> */
		int u = v;
		if (nv <= u) {
			u = 0; /* previous */
		}
		v = u + 1;
		if (nv <= v) {
			v = 0; /* new v    */
		}
		int w = v + 1;
		if (nv <= w) {
			w = 0; /* next     */
		}

		if (snip(p_contour, u, v, w, nv, V, relaxed)) {
			int a, b, c, s, t;

			/* true names of the vertices */
			a = V[u];
			b = V[v];
			c = V[w];

			/* output Triangle */
			r_result.push_back(a);
			r_result.push_back(b);
			r_result.push_back(c);

			/* remove v from remaining polygon */
			for (s = v, t = v + 1; t < nv; s++, t++) {
				V.write[s] = V[t];
			}

			nv--;

			/* reset error detection counter */
			count = 2 * nv;
		}
	}

	return true;
}
