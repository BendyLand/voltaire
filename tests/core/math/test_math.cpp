
// === Merged from tests/core/math/test_aabb.cpp ===
/*  test_aabb.cpp                                                         */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestAABB {

TEST_CASE("[AABB] Constructor methods") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	constexpr AABB aabb_copy = AABB(aabb);

	CHECK_MESSAGE(
			aabb == aabb_copy,
			"AABBs created with the same dimensions but by different methods should be equal.");
}

TEST_CASE("[AABB] String conversion") {
	CHECK_MESSAGE(
			String(AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6))) == "[P: (-1.5, 2.0, -2.5), S: (4.0, 5.0, 6.0)]",
			"The string representation should match the expected value.");
}

TEST_CASE("[AABB] Basic getters") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_position().is_equal_approx(Vector3(-1.5, 2, -2.5)),
			"get_position() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_size().is_equal_approx(Vector3(4, 5, 6)),
			"get_size() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_end().is_equal_approx(Vector3(2.5, 7, 3.5)),
			"get_end() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_center().is_equal_approx(Vector3(0.5, 4.5, 0.5)),
			"get_center() should return the expected value.");
}

TEST_CASE("[AABB] Basic setters") {
	AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	aabb.set_end(Vector3(100, 0, 100));
	CHECK_MESSAGE(
			aabb.is_equal_approx(AABB(Vector3(-1.5, 2, -2.5), Vector3(101.5, -2, 102.5))),
			"set_end() should result in the expected AABB.");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	aabb.set_position(Vector3(-1000, -2000, -3000));
	CHECK_MESSAGE(
			aabb.is_equal_approx(AABB(Vector3(-1000, -2000, -3000), Vector3(4, 5, 6))),
			"set_position() should result in the expected AABB.");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	aabb.set_size(Vector3(0, 0, -50));
	CHECK_MESSAGE(
			aabb.is_equal_approx(AABB(Vector3(-1.5, 2, -2.5), Vector3(0, 0, -50))),
			"set_size() should result in the expected AABB.");
}

TEST_CASE("[AABB] Volume getters") {
	AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_volume() == doctest::Approx(120),
			"get_volume() should return the expected value with positive size.");
	CHECK_MESSAGE(
			aabb.has_volume(),
			"Non-empty volumetric AABB should have a volume.");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(-4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_volume() == doctest::Approx(-120),
			"get_volume() should return the expected value with negative size (1 component).");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(-4, -5, 6));
	CHECK_MESSAGE(
			aabb.get_volume() == doctest::Approx(120),
			"get_volume() should return the expected value with negative size (2 components).");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(-4, -5, -6));
	CHECK_MESSAGE(
			aabb.get_volume() == doctest::Approx(-120),
			"get_volume() should return the expected value with negative size (3 components).");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 0, 6));
	CHECK_MESSAGE(
			!aabb.has_volume(),
			"Non-empty flat AABB should not have a volume.");

	CHECK_MESSAGE(
			!AABB().has_volume(),
			"Empty AABB should not have a volume.");
}

TEST_CASE("[AABB] Surface getters") {
	AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.has_surface(),
			"Non-empty volumetric AABB should have an surface.");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 0, 6));
	CHECK_MESSAGE(
			aabb.has_surface(),
			"Non-empty flat AABB should have a surface.");

	aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 0, 0));
	CHECK_MESSAGE(
			aabb.has_surface(),
			"Non-empty linear AABB should have a surface.");

	CHECK_MESSAGE(
			!AABB().has_surface(),
			"Empty AABB should not have an surface.");
}

TEST_CASE("[AABB] Intersection") {
	constexpr AABB aabb_big = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));

	AABB aabb_small = AABB(Vector3(-1.5, 2, -2.5), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.intersects(aabb_small),
			"intersects() with fully contained AABB (touching the edge) should return the expected result.");

	aabb_small = AABB(Vector3(0.5, 1.5, -2), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.intersects(aabb_small),
			"intersects() with partially contained AABB (overflowing on Y axis) should return the expected result.");

	aabb_small = AABB(Vector3(10, -10, -10), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			!aabb_big.intersects(aabb_small),
			"intersects() with non-contained AABB should return the expected result.");

	aabb_small = AABB(Vector3(-1.5, 2, -2.5), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.intersection(aabb_small).is_equal_approx(aabb_small),
			"intersection() with fully contained AABB (touching the edge) should return the expected result.");

	aabb_small = AABB(Vector3(0.5, 1.5, -2), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.intersection(aabb_small).is_equal_approx(AABB(Vector3(0.5, 2, -2), Vector3(1, 0.5, 1))),
			"intersection() with partially contained AABB (overflowing on Y axis) should return the expected result.");

	aabb_small = AABB(Vector3(10, -10, -10), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.intersection(aabb_small).is_equal_approx(AABB()),
			"intersection() with non-contained AABB should return the expected result.");

	CHECK_MESSAGE(
			aabb_big.intersects_plane(Plane(Vector3(0, 1, 0), 4)),
			"intersects_plane() should return the expected result.");
	CHECK_MESSAGE(
			aabb_big.intersects_plane(Plane(Vector3(0, -1, 0), -4)),
			"intersects_plane() should return the expected result.");
	CHECK_MESSAGE(
			!aabb_big.intersects_plane(Plane(Vector3(0, 1, 0), 200)),
			"intersects_plane() should return the expected result.");

	CHECK_MESSAGE(
			aabb_big.intersects_segment(Vector3(1, 3, 0), Vector3(0, 3, 0)),
			"intersects_segment() should return the expected result.");
	CHECK_MESSAGE(
			aabb_big.intersects_segment(Vector3(0, 3, 0), Vector3(0, -300, 0)),
			"intersects_segment() should return the expected result.");
	CHECK_MESSAGE(
			aabb_big.intersects_segment(Vector3(-50, 3, -50), Vector3(50, 3, 50)),
			"intersects_segment() should return the expected result.");
	CHECK_MESSAGE(
			!aabb_big.intersects_segment(Vector3(-50, 25, -50), Vector3(50, 25, 50)),
			"intersects_segment() should return the expected result.");
	CHECK_MESSAGE(
			aabb_big.intersects_segment(Vector3(0, 3, 0), Vector3(0, 3, 0)),
			"intersects_segment() should return the expected result with segment of length 0.");
	CHECK_MESSAGE(
			!aabb_big.intersects_segment(Vector3(0, 300, 0), Vector3(0, 300, 0)),
			"intersects_segment() should return the expected result with segment of length 0.");
	CHECK_MESSAGE( // Simple ray intersection test.
			aabb_big.intersects_ray(Vector3(-100, 3, 0), Vector3(1, 0, 0)),
			"intersects_ray() should return true when ray points directly to AABB from outside.");
	CHECK_MESSAGE( // Ray parallel to an edge.
			!aabb_big.intersects_ray(Vector3(10, 10, 0), Vector3(0, 1, 0)),
			"intersects_ray() should return false for ray parallel and outside of AABB.");
	CHECK_MESSAGE( // Ray origin inside aabb.
			aabb_big.intersects_ray(Vector3(1, 1, 1), Vector3(0, 1, 0)),
			"intersects_ray() should return true for rays originating inside the AABB.");
	CHECK_MESSAGE( // Ray pointing away from aabb.
			!aabb_big.intersects_ray(Vector3(-10, 0, 0), Vector3(-1, 0, 0)),
			"intersects_ray() should return false when ray points away from AABB.");
	CHECK_MESSAGE( // Ray along a diagonal of aabb.
			aabb_big.intersects_ray(Vector3(0, 0, 0), Vector3(1, 1, 1)),
			"intersects_ray() should return true for rays along the AABB diagonal.");
	CHECK_MESSAGE( // Ray originating at aabb edge.
			aabb_big.intersects_ray(aabb_big.position, Vector3(-1, 0, 0)),
			"intersects_ray() should return true for rays starting on AABB's edge.");
	CHECK_MESSAGE( // Ray with zero direction inside.
			aabb_big.intersects_ray(Vector3(-1, 3, -2), Vector3(0, 0, 0)),
			"intersects_ray() should return true because its inside.");
	CHECK_MESSAGE( // Ray with zero direction outside.
			!aabb_big.intersects_ray(Vector3(-1000, 3, -2), Vector3(0, 0, 0)),
			"intersects_ray() should return false for being outside.");

	// Finding ray intersections.
	constexpr AABB aabb_simple = AABB(Vector3(), Vector3(1, 1, 1));
	bool inside = false;
	Vector3 intersection_point;
	Vector3 intersection_normal;

	// Borders.
	aabb_simple.find_intersects_ray(Vector3(0.5, 0, 0.5), Vector3(0, 1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == false, "find_intersects_ray() should return outside on borders.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 0, 0.5)), "find_intersects_ray() border intersection point incorrect.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, -1, 0)), "find_intersects_ray() border intersection normal incorrect.");
	aabb_simple.find_intersects_ray(Vector3(0.5, 1, 0.5), Vector3(0, -1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == false, "find_intersects_ray() should return outside on borders.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 1, 0.5)), "find_intersects_ray() border intersection point incorrect.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, 1, 0)), "find_intersects_ray() border intersection normal incorrect.");

	// Inside.
	aabb_simple.find_intersects_ray(Vector3(0.5, 0.1, 0.5), Vector3(0, 1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == true, "find_intersects_ray() should return inside when inside.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 0, 0.5)), "find_intersects_ray() inside backtracking intersection point incorrect.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, -1, 0)), "find_intersects_ray() inside intersection normal incorrect.");

	// Zero sized AABB.
	constexpr AABB aabb_zero = AABB(Vector3(), Vector3(1, 0, 1));
	aabb_zero.find_intersects_ray(Vector3(0.5, 0, 0.5), Vector3(0, 1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == false, "find_intersects_ray() should return outside on borders of zero sized AABB.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 0, 0.5)), "find_intersects_ray() border intersection point incorrect for zero sized AABB.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, -1, 0)), "find_intersects_ray() border intersection normal incorrect for zero sized AABB.");
	aabb_zero.find_intersects_ray(Vector3(0.5, 0, 0.5), Vector3(0, -1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == false, "find_intersects_ray() should return outside on borders of zero sized AABB.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 0, 0.5)), "find_intersects_ray() border intersection point incorrect for zero sized AABB.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, 1, 0)), "find_intersects_ray() border intersection normal incorrect for zero sized AABB.");
	aabb_zero.find_intersects_ray(Vector3(0.5, -1, 0.5), Vector3(0, 1, 0), inside, &intersection_point, &intersection_normal);
	CHECK_MESSAGE(inside == false, "find_intersects_ray() should return outside on borders of zero sized AABB.");
	CHECK_MESSAGE(intersection_point.is_equal_approx(Vector3(0.5, 0, 0.5)), "find_intersects_ray() border intersection point incorrect for zero sized AABB.");
	CHECK_MESSAGE(intersection_normal.is_equal_approx(Vector3(0, -1, 0)), "find_intersects_ray() border intersection normal incorrect for zero sized AABB.");
}

TEST_CASE("[AABB] Merging") {
	constexpr AABB aabb_big = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));

	AABB aabb_small = AABB(Vector3(-1.5, 2, -2.5), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.merge(aabb_small).is_equal_approx(aabb_big),
			"merge() with fully contained AABB (touching the edge) should return the expected result.");

	aabb_small = AABB(Vector3(0.5, 1.5, -2), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.merge(aabb_small).is_equal_approx(AABB(Vector3(-1.5, 1.5, -2.5), Vector3(4, 5.5, 6))),
			"merge() with partially contained AABB (overflowing on Y axis) should return the expected result.");

	aabb_small = AABB(Vector3(10, -10, -10), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.merge(aabb_small).is_equal_approx(AABB(Vector3(-1.5, -10, -10), Vector3(12.5, 17, 13.5))),
			"merge() with non-contained AABB should return the expected result.");
}

TEST_CASE("[AABB] Encloses") {
	constexpr AABB aabb_big = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));

	CHECK_MESSAGE(
			aabb_big.encloses(aabb_big),
			"encloses() with itself should return the expected result.");

	AABB aabb_small = AABB(Vector3(-1.5, 2, -2.5), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.encloses(aabb_small),
			"encloses() with fully contained AABB (touching the edge) should return the expected result.");

	aabb_small = AABB(Vector3(1.5, 6, 2.5), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			aabb_big.encloses(aabb_small),
			"encloses() with fully contained AABB (touching the edge) should return the expected result.");

	aabb_small = AABB(Vector3(0.5, 1.5, -2), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			!aabb_big.encloses(aabb_small),
			"encloses() with partially contained AABB (overflowing on Y axis) should return the expected result.");

	aabb_small = AABB(Vector3(10, -10, -10), Vector3(1, 1, 1));
	CHECK_MESSAGE(
			!aabb_big.encloses(aabb_small),
			"encloses() with non-contained AABB should return the expected result.");
}

TEST_CASE("[AABB] Get endpoints") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_endpoint(0).is_equal_approx(Vector3(-1.5, 2, -2.5)),
			"The endpoint at index 0 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(1).is_equal_approx(Vector3(-1.5, 2, 3.5)),
			"The endpoint at index 1 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(2).is_equal_approx(Vector3(-1.5, 7, -2.5)),
			"The endpoint at index 2 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(3).is_equal_approx(Vector3(-1.5, 7, 3.5)),
			"The endpoint at index 3 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(4).is_equal_approx(Vector3(2.5, 2, -2.5)),
			"The endpoint at index 4 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(5).is_equal_approx(Vector3(2.5, 2, 3.5)),
			"The endpoint at index 5 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(6).is_equal_approx(Vector3(2.5, 7, -2.5)),
			"The endpoint at index 6 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(7).is_equal_approx(Vector3(2.5, 7, 3.5)),
			"The endpoint at index 7 should match the expected value.");

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			aabb.get_endpoint(8).is_equal_approx(Vector3()),
			"The endpoint at invalid index 8 should match the expected value.");
	CHECK_MESSAGE(
			aabb.get_endpoint(-1).is_equal_approx(Vector3()),
			"The endpoint at invalid index -1 should match the expected value.");
	ERR_PRINT_ON;
}

TEST_CASE("[AABB] Get longest/shortest axis") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_longest_axis() == Vector3(0, 0, 1),
			"get_longest_axis() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_longest_axis_index() == Vector3::AXIS_Z,
			"get_longest_axis_index() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_longest_axis_size() == 6,
			"get_longest_axis_size() should return the expected value.");

	CHECK_MESSAGE(
			aabb.get_shortest_axis() == Vector3(1, 0, 0),
			"get_shortest_axis() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_shortest_axis_index() == Vector3::AXIS_X,
			"get_shortest_axis_index() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_shortest_axis_size() == 4,
			"get_shortest_axis_size() should return the expected value.");
}

TEST_CASE("[AABB] Get support") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.get_support(Vector3(1, 0, 0)) == Vector3(2.5, 2, -2.5),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_support(Vector3(0.5, 1, 1)) == Vector3(2.5, 7, 3.5),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_support(Vector3(0.5, 1, -400)) == Vector3(2.5, 7, -2.5),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_support(Vector3(0, -1, 0)) == Vector3(-1.5, 2, -2.5),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_support(Vector3(0, -0.1, 0)) == Vector3(-1.5, 2, -2.5),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			aabb.get_support(Vector3()) == Vector3(-1.5, 2, -2.5),
			"get_support() should return the AABB position when given a zero vector.");
}

TEST_CASE("[AABB] Grow") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.grow(0.25).is_equal_approx(AABB(Vector3(-1.75, 1.75, -2.75), Vector3(4.5, 5.5, 6.5))),
			"grow() with positive value should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.grow(-0.25).is_equal_approx(AABB(Vector3(-1.25, 2.25, -2.25), Vector3(3.5, 4.5, 5.5))),
			"grow() with negative value should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.grow(-10).is_equal_approx(AABB(Vector3(8.5, 12, 7.5), Vector3(-16, -15, -14))),
			"grow() with large negative value should return the expected AABB.");
}

TEST_CASE("[AABB] Has point") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.has_point(Vector3(-1, 3, 0)),
			"has_point() with contained point should return the expected value.");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(2, 3, 0)),
			"has_point() with contained point should return the expected value.");
	CHECK_MESSAGE(
			!aabb.has_point(Vector3(-20, 0, 0)),
			"has_point() with non-contained point should return the expected value.");

	CHECK_MESSAGE(
			aabb.has_point(Vector3(-1.5, 3, 0)),
			"has_point() with positive size should include point on near face (X axis).");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(2.5, 3, 0)),
			"has_point() with positive size should include point on far face (X axis).");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(0, 2, 0)),
			"has_point() with positive size should include point on near face (Y axis).");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(0, 7, 0)),
			"has_point() with positive size should include point on far face (Y axis).");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(0, 3, -2.5)),
			"has_point() with positive size should include point on near face (Z axis).");
	CHECK_MESSAGE(
			aabb.has_point(Vector3(0, 3, 3.5)),
			"has_point() with positive size should include point on far face (Z axis).");
}

TEST_CASE("[AABB] Expanding") {
	constexpr AABB aabb = AABB(Vector3(-1.5, 2, -2.5), Vector3(4, 5, 6));
	CHECK_MESSAGE(
			aabb.expand(Vector3(-1, 3, 0)).is_equal_approx(aabb),
			"expand() with contained point should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.expand(Vector3(2, 3, 0)).is_equal_approx(aabb),
			"expand() with contained point should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.expand(Vector3(-1.5, 3, 0)).is_equal_approx(aabb),
			"expand() with contained point on negative edge should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.expand(Vector3(2.5, 3, 0)).is_equal_approx(aabb),
			"expand() with contained point on positive edge should return the expected AABB.");
	CHECK_MESSAGE(
			aabb.expand(Vector3(-20, 0, 0)).is_equal_approx(AABB(Vector3(-20, 0, -2.5), Vector3(22.5, 7, 6))),
			"expand() with non-contained point should return the expected AABB.");
}

TEST_CASE("[AABB] Finite number checks") {
	constexpr Vector3 x(0, 1, 2);
	constexpr Vector3 infinite(Math::NaN, Math::NaN, Math::NaN);

	CHECK_MESSAGE(
			AABB(x, x).is_finite(),
			"AABB with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			AABB(infinite, x).is_finite(),
			"AABB with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			AABB(x, infinite).is_finite(),
			"AABB with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			AABB(infinite, infinite).is_finite(),
			"AABB with two components infinite should not be finite.");
}

} // namespace TestAABB

// === Merged from tests/core/math/test_astar.cpp ===
/*  test_astar.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/math/geometry_3d.h"

namespace TestAStar {

class ABCX : public AStar3D {
public:
	enum {
		A,
		B,
		C,
		X,
	};

	ABCX() {
		add_point(A, Vector3(0, 0, 0));
		add_point(B, Vector3(1, 0, 0));
		add_point(C, Vector3(0, 1, 0));
		add_point(X, Vector3(0, 0, 1));
		connect_points(A, B);
		connect_points(A, C);
		connect_points(B, C);
		connect_points(X, A);
	}

	// Disable heuristic completely.
	real_t _compute_cost(int64_t p_from, int64_t p_to) {
		if (p_from == A && p_to == C) {
			return 1000;
		}
		return 100;
	}
};

TEST_CASE("[AStar3D] ABC path") {
	ABCX abcx;
	Vector<int64_t> path = abcx.get_id_path(ABCX::A, ABCX::C);
	REQUIRE(path.size() == 3);
	CHECK(path[0] == ABCX::A);
	CHECK(path[1] == ABCX::B);
	CHECK(path[2] == ABCX::C);
}

TEST_CASE("[AStar3D] ABCX path") {
	ABCX abcx;
	Vector<int64_t> path = abcx.get_id_path(ABCX::X, ABCX::C);
	REQUIRE(path.size() == 4);
	CHECK(path[0] == ABCX::X);
	CHECK(path[1] == ABCX::A);
	CHECK(path[2] == ABCX::B);
	CHECK(path[3] == ABCX::C);
}

TEST_CASE("[AStar3D] Add/Remove") {
	AStar3D a;

	// Manual tests.
	a.add_point(1, Vector3(0, 0, 0));
	a.add_point(2, Vector3(0, 1, 0));
	a.add_point(3, Vector3(1, 1, 0));
	a.add_point(4, Vector3(2, 0, 0));
	a.connect_points(1, 2, true);
	a.connect_points(1, 3, true);
	a.connect_points(1, 4, false);

	CHECK(a.are_points_connected(2, 1));
	CHECK(a.are_points_connected(4, 1));
	CHECK(a.are_points_connected(2, 1, false));
	CHECK_FALSE(a.are_points_connected(4, 1, false));

	a.disconnect_points(1, 2, true);
	CHECK(a.get_point_connections(1).size() == 2); // 3, 4
	CHECK(a.get_point_connections(2).size() == 0);

	a.disconnect_points(4, 1, false);
	CHECK(a.get_point_connections(1).size() == 2); // 3, 4
	CHECK(a.get_point_connections(4).size() == 0);

	a.disconnect_points(4, 1, true);
	CHECK(a.get_point_connections(1).size() == 1); // 3
	CHECK(a.get_point_connections(4).size() == 0);

	a.connect_points(2, 3, false);
	CHECK(a.get_point_connections(2).size() == 1); // 3
	CHECK(a.get_point_connections(3).size() == 1); // 1

	a.connect_points(2, 3, true);
	CHECK(a.get_point_connections(2).size() == 1); // 3
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2

	a.disconnect_points(2, 3, false);
	CHECK(a.get_point_connections(2).size() == 0);
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2

	a.connect_points(4, 3, true);
	CHECK(a.get_point_connections(3).size() == 3); // 1, 2, 4
	CHECK(a.get_point_connections(4).size() == 1); // 3

	a.disconnect_points(3, 4, false);
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2
	CHECK(a.get_point_connections(4).size() == 1); // 3

	a.remove_point(3);
	CHECK(a.get_point_connections(1).size() == 0);
	CHECK(a.get_point_connections(2).size() == 0);
	CHECK(a.get_point_connections(4).size() == 0);

	a.add_point(0, Vector3(0, -1, 0));
	a.add_point(3, Vector3(2, 1, 0));
	// 0: (0, -1)
	// 1: (0, 0)
	// 2: (0, 1)
	// 3: (2, 1)
	// 4: (2, 0)

	// Tests for get_closest_position_in_segment.
	a.connect_points(2, 3);
	CHECK(a.get_closest_position_in_segment(Vector3(0.5, 0.5, 0)) == Vector3(0.5, 1, 0));

	a.connect_points(3, 4);
	a.connect_points(0, 3);
	a.connect_points(1, 4);
	a.disconnect_points(1, 4, false);
	a.disconnect_points(4, 3, false);
	a.disconnect_points(3, 4, false);
	// Remaining edges: <2, 3>, <0, 3>, <1, 4> (directed).
	CHECK(a.get_closest_position_in_segment(Vector3(2, 0.5, 0)) == Vector3(1.75, 0.75, 0));
	CHECK(a.get_closest_position_in_segment(Vector3(-1, 0.2, 0)) == Vector3(0, 0, 0));
	CHECK(a.get_closest_position_in_segment(Vector3(3, 2, 0)) == Vector3(2, 1, 0));

	Math::seed(0);

	// Random tests for connectivity checks
	for (int i = 0; i < 20000; i++) {
		int u = Math::rand() % 5;
		int v = Math::rand() % 4;
		if (u == v) {
			v = 4;
		}
		if (Math::rand() % 2 == 1) {
			// Add a (possibly existing) directed edge and confirm connectivity.
			a.connect_points(u, v, false);
			CHECK(a.are_points_connected(u, v, false));
		} else {
			// Remove a (possibly nonexistent) directed edge and confirm disconnectivity.
			a.disconnect_points(u, v, false);
			CHECK_FALSE(a.are_points_connected(u, v, false));
		}
	}

	// Random tests for point removal.
	for (int i = 0; i < 20000; i++) {
		a.clear();
		for (int j = 0; j < 5; j++) {
			a.add_point(j, Vector3(0, 0, 0));
		}

		// Add or remove random edges.
		for (int j = 0; j < 10; j++) {
			int u = Math::rand() % 5;
			int v = Math::rand() % 4;
			if (u == v) {
				v = 4;
			}
			if (Math::rand() % 2 == 1) {
				a.connect_points(u, v, false);
			} else {
				a.disconnect_points(u, v, false);
			}
		}

		// Remove point 0.
		a.remove_point(0);
		// White box: this will check all edges remaining in the segments set.
		for (int j = 1; j < 5; j++) {
			CHECK_FALSE(a.are_points_connected(0, j, true));
		}
	}
}

TEST_CASE("[AStar3D] Path from disabled point is empty") {
	AStar3D a;
	Vector3 p1(0, 0, 0);
	Vector3 p2(0, 1, 0);
	a.add_point(1, p1);
	a.add_point(2, p2);
	a.connect_points(1, 2);

	CHECK_EQ(a.get_id_path(1, 1), Vector<int64_t>{ 1 });
	CHECK_EQ(a.get_id_path(1, 2), Vector<int64_t>{ 1, 2 });

	CHECK_EQ(a.get_point_path(1, 1), Vector<Vector3>{ p1 });
	CHECK_EQ(a.get_point_path(1, 2), Vector<Vector3>{ p1, p2 });

	a.set_point_disabled(1, true);

	CHECK(a.get_id_path(1, 1).is_empty());
	CHECK(a.get_id_path(1, 2).is_empty());

	CHECK(a.get_point_path(1, 1).is_empty());
	CHECK(a.get_point_path(1, 2).is_empty());
}

} // namespace TestAStar

// === Merged from tests/core/math/test_basis.cpp ===
/*  test_basis.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestBasis {

Vector3 deg_to_rad(const Vector3 &p_rotation) {
	return p_rotation / 180.0 * Math::PI;
}

Vector3 rad2deg(const Vector3 &p_rotation) {
	return p_rotation / Math::PI * 180.0;
}

String get_rot_order_name(EulerOrder ro) {
	switch (ro) {
		case EulerOrder::XYZ:
			return "XYZ";
		case EulerOrder::XZY:
			return "XZY";
		case EulerOrder::YZX:
			return "YZX";
		case EulerOrder::YXZ:
			return "YXZ";
		case EulerOrder::ZXY:
			return "ZXY";
		case EulerOrder::ZYX:
			return "ZYX";
		default:
			return "[Not supported]";
	}
}

void test_rotation(Vector3 deg_original_euler, EulerOrder rot_order) {
	// This test:
	// 1. Converts the rotation vector from deg to rad.
	// 2. Converts euler to basis.
	// 3. Converts the above basis back into euler.
	// 4. Converts the above euler into basis again.
	// 5. Compares the basis obtained in step 2 with the basis of step 4
	//
	// The conversion "basis to euler", done in the step 3, may be different from
	// the original euler, even if the final rotation are the same.
	// This happens because there are more ways to represents the same rotation,
	// both valid, using eulers.
	// For this reason is necessary to convert that euler back to basis and finally
	// compares it.
	//
	// In this way we can assert that both functions: basis to euler / euler to basis
	// are correct.

	// Euler to rotation
	const Vector3 original_euler = deg_to_rad(deg_original_euler);
	const Basis to_rotation = Basis::from_euler(original_euler, rot_order);

	// Euler from rotation
	const Vector3 euler_from_rotation = to_rotation.get_euler(rot_order);
	const Basis rotation_from_computed_euler = Basis::from_euler(euler_from_rotation, rot_order);

	Basis res = to_rotation.inverse() * rotation_from_computed_euler;

	CHECK_MESSAGE((res.get_column(0) - Vector3(1.0, 0.0, 0.0)).length() <= 0.001, vformat("Fail due to X %s\n", String(res.get_column(0))));
	CHECK_MESSAGE((res.get_column(1) - Vector3(0.0, 1.0, 0.0)).length() <= 0.001, vformat("Fail due to Y %s\n", String(res.get_column(1))));
	CHECK_MESSAGE((res.get_column(2) - Vector3(0.0, 0.0, 1.0)).length() <= 0.001, vformat("Fail due to Z %s\n", String(res.get_column(2))));

	// Double check `to_rotation` decomposing with XYZ rotation order.
	const Vector3 euler_xyz_from_rotation = to_rotation.get_euler(EulerOrder::XYZ);
	Basis rotation_from_xyz_computed_euler = Basis::from_euler(euler_xyz_from_rotation, EulerOrder::XYZ);

	res = to_rotation.inverse() * rotation_from_xyz_computed_euler;

	CHECK_MESSAGE((res.get_column(0) - Vector3(1.0, 0.0, 0.0)).length() <= 0.001, vformat("Double check with XYZ rot order failed, due to X %s\n", String(res.get_column(0))));
	CHECK_MESSAGE((res.get_column(1) - Vector3(0.0, 1.0, 0.0)).length() <= 0.001, vformat("Double check with XYZ rot order failed, due to Y %s\n", String(res.get_column(1))));
	CHECK_MESSAGE((res.get_column(2) - Vector3(0.0, 0.0, 1.0)).length() <= 0.001, vformat("Double check with XYZ rot order failed, due to Z %s\n", String(res.get_column(2))));

	INFO(vformat("Rotation order: %s\n.", get_rot_order_name(rot_order)));
	INFO(vformat("Original Rotation: %s\n", String(deg_original_euler)));
	INFO(vformat("Quaternion to rotation order: %s\n", String(rad2deg(euler_from_rotation))));
}

TEST_CASE("[Basis] Euler conversions") {
	Vector<EulerOrder> euler_order_to_test;
	euler_order_to_test.push_back(EulerOrder::XYZ);
	euler_order_to_test.push_back(EulerOrder::XZY);
	euler_order_to_test.push_back(EulerOrder::YZX);
	euler_order_to_test.push_back(EulerOrder::YXZ);
	euler_order_to_test.push_back(EulerOrder::ZXY);
	euler_order_to_test.push_back(EulerOrder::ZYX);

	Vector<Vector3> vectors_to_test;

	// Test the special cases.
	vectors_to_test.push_back(Vector3(0.0, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.5, 0.5, 0.5));
	vectors_to_test.push_back(Vector3(-0.5, -0.5, -0.5));
	vectors_to_test.push_back(Vector3(40.0, 40.0, 40.0));
	vectors_to_test.push_back(Vector3(-40.0, -40.0, -40.0));
	vectors_to_test.push_back(Vector3(0.0, 0.0, -90.0));
	vectors_to_test.push_back(Vector3(0.0, -90.0, 0.0));
	vectors_to_test.push_back(Vector3(-90.0, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.0, 0.0, 90.0));
	vectors_to_test.push_back(Vector3(0.0, 90.0, 0.0));
	vectors_to_test.push_back(Vector3(90.0, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.0, 0.0, -30.0));
	vectors_to_test.push_back(Vector3(0.0, -30.0, 0.0));
	vectors_to_test.push_back(Vector3(-30.0, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.0, 0.0, 30.0));
	vectors_to_test.push_back(Vector3(0.0, 30.0, 0.0));
	vectors_to_test.push_back(Vector3(30.0, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.5, 50.0, 20.0));
	vectors_to_test.push_back(Vector3(-0.5, -50.0, -20.0));
	vectors_to_test.push_back(Vector3(0.5, 0.0, 90.0));
	vectors_to_test.push_back(Vector3(0.5, 0.0, -90.0));
	vectors_to_test.push_back(Vector3(360.0, 360.0, 360.0));
	vectors_to_test.push_back(Vector3(-360.0, -360.0, -360.0));
	vectors_to_test.push_back(Vector3(-90.0, 60.0, -90.0));
	vectors_to_test.push_back(Vector3(90.0, 60.0, -90.0));
	vectors_to_test.push_back(Vector3(90.0, -60.0, -90.0));
	vectors_to_test.push_back(Vector3(-90.0, -60.0, -90.0));
	vectors_to_test.push_back(Vector3(-90.0, 60.0, 90.0));
	vectors_to_test.push_back(Vector3(90.0, 60.0, 90.0));
	vectors_to_test.push_back(Vector3(90.0, -60.0, 90.0));
	vectors_to_test.push_back(Vector3(-90.0, -60.0, 90.0));
	vectors_to_test.push_back(Vector3(60.0, 90.0, -40.0));
	vectors_to_test.push_back(Vector3(60.0, -90.0, -40.0));
	vectors_to_test.push_back(Vector3(-60.0, -90.0, -40.0));
	vectors_to_test.push_back(Vector3(-60.0, 90.0, 40.0));
	vectors_to_test.push_back(Vector3(60.0, 90.0, 40.0));
	vectors_to_test.push_back(Vector3(60.0, -90.0, 40.0));
	vectors_to_test.push_back(Vector3(-60.0, -90.0, 40.0));
	vectors_to_test.push_back(Vector3(-90.0, 90.0, -90.0));
	vectors_to_test.push_back(Vector3(90.0, 90.0, -90.0));
	vectors_to_test.push_back(Vector3(90.0, -90.0, -90.0));
	vectors_to_test.push_back(Vector3(-90.0, -90.0, -90.0));
	vectors_to_test.push_back(Vector3(-90.0, 90.0, 90.0));
	vectors_to_test.push_back(Vector3(90.0, 90.0, 90.0));
	vectors_to_test.push_back(Vector3(90.0, -90.0, 90.0));
	vectors_to_test.push_back(Vector3(20.0, 150.0, 30.0));
	vectors_to_test.push_back(Vector3(20.0, -150.0, 30.0));
	vectors_to_test.push_back(Vector3(-120.0, -150.0, 30.0));
	vectors_to_test.push_back(Vector3(-120.0, -150.0, -130.0));
	vectors_to_test.push_back(Vector3(120.0, -150.0, -130.0));
	vectors_to_test.push_back(Vector3(120.0, 150.0, -130.0));
	vectors_to_test.push_back(Vector3(120.0, 150.0, 130.0));
	vectors_to_test.push_back(Vector3(89.9, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(-89.9, 0.0, 0.0));
	vectors_to_test.push_back(Vector3(0.0, 89.9, 0.0));
	vectors_to_test.push_back(Vector3(0.0, -89.9, 0.0));
	vectors_to_test.push_back(Vector3(0.0, 0.0, 89.9));
	vectors_to_test.push_back(Vector3(0.0, 0.0, -89.9));

	for (int h = 0; h < euler_order_to_test.size(); h += 1) {
		for (int i = 0; i < vectors_to_test.size(); i += 1) {
			test_rotation(vectors_to_test[i], euler_order_to_test[h]);
		}
	}
}

TEST_CASE("[Basis] Set axis angle") {
	Vector3 axis;
	real_t angle;
	real_t pi = (real_t)Math::PI;

	// Testing the singularity when the angle is 0°.
	Basis identity(1, 0, 0, 0, 1, 0, 0, 0, 1);
	identity.get_axis_angle(axis, angle);
	CHECK(angle == 0);

	// Testing the singularity when the angle is 180°.
	Basis singularityPi(-1, 0, 0, 0, 1, 0, 0, 0, -1);
	singularityPi.get_axis_angle(axis, angle);
	CHECK(angle == doctest::Approx(pi));

	// Testing reversing the an axis (of an 30° angle).
	float cos30deg = Math::cos(Math::deg_to_rad((real_t)30.0));
	Basis z_positive(cos30deg, -0.5, 0, 0.5, cos30deg, 0, 0, 0, 1);
	Basis z_negative(cos30deg, 0.5, 0, -0.5, cos30deg, 0, 0, 0, 1);

	z_positive.get_axis_angle(axis, angle);
	CHECK(angle == doctest::Approx(Math::deg_to_rad((real_t)30.0)));
	CHECK(axis == Vector3(0, 0, 1));

	z_negative.get_axis_angle(axis, angle);
	CHECK(angle == doctest::Approx(Math::deg_to_rad((real_t)30.0)));
	CHECK(axis == Vector3(0, 0, -1));

	// Testing a rotation of 90° on x-y-z.
	Basis x90deg(1, 0, 0, 0, 0, -1, 0, 1, 0);
	x90deg.get_axis_angle(axis, angle);
	CHECK(angle == doctest::Approx(pi / (real_t)2));
	CHECK(axis == Vector3(1, 0, 0));

	Basis y90deg(0, 0, 1, 0, 1, 0, -1, 0, 0);
	y90deg.get_axis_angle(axis, angle);
	CHECK(axis == Vector3(0, 1, 0));

	Basis z90deg(0, -1, 0, 1, 0, 0, 0, 0, 1);
	z90deg.get_axis_angle(axis, angle);
	CHECK(axis == Vector3(0, 0, 1));

	// Regression test: checks that the method returns a small angle (not 0).
	Basis tiny(1, 0, 0, 0, 0.9999995, -0.001, 0, 001, 0.9999995); // The min angle possible with float is 0.001rad.
	tiny.get_axis_angle(axis, angle);
	CHECK(angle == doctest::Approx(0.001).epsilon(0.0001));

	// Regression test: checks that the method returns an angle which is a number (not NaN)
	Basis bugNan(1.00000024, 0, 0.000100001693, 0, 1, 0, -0.000100009143, 0, 1.00000024);
	bugNan.get_axis_angle(axis, angle);
	CHECK(!Math::is_nan(angle));
}

TEST_CASE("[Basis] Finite number checks") {
	constexpr Vector3 x(0, 1, 2);
	constexpr Vector3 infinite(Math::NaN, Math::NaN, Math::NaN);

	CHECK_MESSAGE(
			Basis(x, x, x).is_finite(),
			"Basis with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Basis(infinite, x, x).is_finite(),
			"Basis with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Basis(x, infinite, x).is_finite(),
			"Basis with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Basis(x, x, infinite).is_finite(),
			"Basis with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Basis(infinite, infinite, x).is_finite(),
			"Basis with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Basis(infinite, x, infinite).is_finite(),
			"Basis with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Basis(x, infinite, infinite).is_finite(),
			"Basis with two components infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Basis(infinite, infinite, infinite).is_finite(),
			"Basis with three components infinite should not be finite.");
}

TEST_CASE("[Basis] Is conformal checks") {
	CHECK_MESSAGE(
			Basis().is_conformal(),
			"Identity Basis should be conformal.");

	CHECK_MESSAGE(
			Basis::from_euler(Vector3(1.2, 3.4, 5.6)).is_conformal(),
			"Basis with only rotation should be conformal.");

	CHECK_MESSAGE(
			Basis::from_scale(Vector3(-1, -1, -1)).is_conformal(),
			"Basis with only a flip should be conformal.");

	CHECK_MESSAGE(
			Basis::from_scale(Vector3(1.2, 1.2, 1.2)).is_conformal(),
			"Basis with only uniform scale should be conformal.");

	CHECK_MESSAGE(
			Basis(Vector3(3, 4, 0), Vector3(4, -3, 0.0), Vector3(0, 0, 5)).is_conformal(),
			"Basis with a flip, rotation, and uniform scale should be conformal.");

	CHECK_FALSE_MESSAGE(
			Basis::from_scale(Vector3(1.2, 3.4, 5.6)).is_conformal(),
			"Basis with non-uniform scale should not be conformal.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(Math::SQRT12, Math::SQRT12, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)).is_conformal(),
			"Basis with the X axis skewed 45 degrees should not be conformal.");

	CHECK_MESSAGE(
			Basis(0, 0, 0, 0, 0, 0, 0, 0, 0).is_conformal(),
			"Edge case: Basis with all zeroes should return true for is_conformal (because a 0 scale is uniform).");
}

TEST_CASE("[Basis] Is orthogonal checks") {
	CHECK_MESSAGE(
			Basis().is_orthogonal(),
			"Identity Basis should be orthogonal.");

	CHECK_MESSAGE(
			Basis::from_euler(Vector3(1.2, 3.4, 5.6)).is_orthogonal(),
			"Basis with only rotation should be orthogonal.");

	CHECK_MESSAGE(
			Basis::from_scale(Vector3(-1, -1, -1)).is_orthogonal(),
			"Basis with only a flip should be orthogonal.");

	CHECK_MESSAGE(
			Basis::from_scale(Vector3(1.2, 3.4, 5.6)).is_orthogonal(),
			"Basis with only scale should be orthogonal.");

	CHECK_MESSAGE(
			Basis(Vector3(3, 4, 0), Vector3(4, -3, 0), Vector3(0, 0, 5)).is_orthogonal(),
			"Basis with a flip, rotation, and uniform scale should be orthogonal.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(Math::SQRT12, Math::SQRT12, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)).is_orthogonal(),
			"Basis with the X axis skewed 45 degrees should not be orthogonal.");

	CHECK_MESSAGE(
			Basis(0, 0, 0, 0, 0, 0, 0, 0, 0).is_orthogonal(),
			"Edge case: Basis with all zeroes should return true for is_orthogonal, since zero vectors are orthogonal to all vectors.");
}

TEST_CASE("[Basis] Is orthonormal checks") {
	CHECK_MESSAGE(
			Basis().is_orthonormal(),
			"Identity Basis should be orthonormal.");

	CHECK_MESSAGE(
			Basis::from_euler(Vector3(1.2, 3.4, 5.6)).is_orthonormal(),
			"Basis with only rotation should be orthonormal.");

	CHECK_MESSAGE(
			Basis::from_scale(Vector3(-1, -1, -1)).is_orthonormal(),
			"Basis with only a flip should be orthonormal.");

	CHECK_FALSE_MESSAGE(
			Basis::from_scale(Vector3(1.2, 3.4, 5.6)).is_orthonormal(),
			"Basis with only scale should not be orthonormal.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(3, 4, 0), Vector3(4, -3, 0), Vector3(0, 0, 5)).is_orthonormal(),
			"Basis with a flip, rotation, and uniform scale should not be orthonormal.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(Math::SQRT12, Math::SQRT12, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)).is_orthonormal(),
			"Basis with the X axis skewed 45 degrees should not be orthonormal.");

	CHECK_FALSE_MESSAGE(
			Basis(0, 0, 0, 0, 0, 0, 0, 0, 0).is_orthonormal(),
			"Edge case: Basis with all zeroes should return false for is_orthonormal, since the vectors do not have a length of 1.");
}

TEST_CASE("[Basis] Is rotation checks") {
	CHECK_MESSAGE(
			Basis().is_rotation(),
			"Identity Basis should be a rotation (a rotation of zero).");

	CHECK_MESSAGE(
			Basis::from_euler(Vector3(1.2, 3.4, 5.6)).is_rotation(),
			"Basis with only rotation should be a rotation.");

	CHECK_FALSE_MESSAGE(
			Basis::from_scale(Vector3(-1, -1, -1)).is_rotation(),
			"Basis with only a flip should not be a rotation.");

	CHECK_FALSE_MESSAGE(
			Basis::from_scale(Vector3(1.2, 3.4, 5.6)).is_rotation(),
			"Basis with only scale should not be a rotation.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(2, 0, 0), Vector3(0, 0.5, 0), Vector3(0, 0, 1)).is_rotation(),
			"Basis with a squeeze should not be a rotation.");

	CHECK_FALSE_MESSAGE(
			Basis(Vector3(Math::SQRT12, Math::SQRT12, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)).is_rotation(),
			"Basis with the X axis skewed 45 degrees should not be a rotation.");

	CHECK_FALSE_MESSAGE(
			Basis(0, 0, 0, 0, 0, 0, 0, 0, 0).is_rotation(),
			"Edge case: Basis with all zeroes should return false for is_rotation, because it is not just a rotation (has a scale of 0).");
}

} // namespace TestBasis

// === Merged from tests/core/math/test_color.cpp ===
/*  test_color.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/templates/hashfuncs.h"

namespace TestColor {

TEST_CASE("[Color] Constructor methods") {
	constexpr Color blue_rgba = Color(0.25098, 0.376471, 1, 0.501961);
	const Color blue_html = Color::html("#4060ff80");
	const Color blue_hex = Color::hex(0x4060ff80);
	const Color blue_hex64 = Color::hex64(0x4040'6060'ffff'8080);

	CHECK_MESSAGE(
			blue_rgba.is_equal_approx(blue_html),
			"Creation with HTML notation should result in components approximately equal to the default constructor.");
	CHECK_MESSAGE(
			blue_rgba.is_equal_approx(blue_hex),
			"Creation with a 32-bit hexadecimal number should result in components approximately equal to the default constructor.");
	CHECK_MESSAGE(
			blue_rgba.is_equal_approx(blue_hex64),
			"Creation with a 64-bit hexadecimal number should result in components approximately equal to the default constructor.");

	ERR_PRINT_OFF;
	const Color html_invalid = Color::html("invalid");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			html_invalid.is_equal_approx(Color()),
			"Creation with invalid HTML notation should result in a Color with the default values.");

	constexpr Color green_rgba = Color(0, 1, 0, 0.25);
	const Color green_hsva = Color::from_hsv(120 / 360.0, 1, 1, 0.25);

	CHECK_MESSAGE(
			green_rgba.is_equal_approx(green_hsva),
			"Creation with HSV notation should result in components approximately equal to the default constructor.");
}

TEST_CASE("[Color] Operators") {
	constexpr Color blue = Color(0.2, 0.2, 1);
	constexpr Color dark_red = Color(0.3, 0.1, 0.1);

	// Color components may be negative. Also, the alpha component may be greater than 1.0.
	CHECK_MESSAGE(
			(blue + dark_red).is_equal_approx(Color(0.5, 0.3, 1.1, 2)),
			"Color addition should behave as expected.");
	CHECK_MESSAGE(
			(blue - dark_red).is_equal_approx(Color(-0.1, 0.1, 0.9, 0)),
			"Color subtraction should behave as expected.");
	CHECK_MESSAGE(
			(blue * 2).is_equal_approx(Color(0.4, 0.4, 2, 2)),
			"Color multiplication with a scalar should behave as expected.");
	CHECK_MESSAGE(
			(blue / 2).is_equal_approx(Color(0.1, 0.1, 0.5, 0.5)),
			"Color division with a scalar should behave as expected.");
	CHECK_MESSAGE(
			(blue * dark_red).is_equal_approx(Color(0.06, 0.02, 0.1)),
			"Color multiplication with another Color should behave as expected.");
	CHECK_MESSAGE(
			(blue / dark_red).is_equal_approx(Color(0.666667, 2, 10)),
			"Color division with another Color should behave as expected.");
	CHECK_MESSAGE(
			(-blue).is_equal_approx(Color(0.8, 0.8, 0, 0)),
			"Color negation should behave as expected (affecting the alpha channel, unlike `invert()`).");
}

TEST_CASE("[Color] Hash") {
	const Color red = Color(1, 0, 0, 1);
	const Color yellow = Color(1, 1, 0, 1);

	CHECK(red.hash() != yellow.hash());
}

TEST_CASE("[Color] Reading methods") {
	constexpr Color dark_blue = Color(0, 0, 0.5, 0.4);

	CHECK_MESSAGE(
			dark_blue.get_h() == doctest::Approx(240.0f / 360.0f),
			"The returned HSV hue should match the expected value.");
	CHECK_MESSAGE(
			dark_blue.get_s() == doctest::Approx(1.0f),
			"The returned HSV saturation should match the expected value.");
	CHECK_MESSAGE(
			dark_blue.get_v() == doctest::Approx(0.5f),
			"The returned HSV value should match the expected value.");
}

TEST_CASE("[Color] Conversion methods") {
	constexpr Color cyan = Color(0, 1, 1);
	constexpr Color cyan_transparent = Color(0, 1, 1, 0);

	CHECK_MESSAGE(
			cyan.to_html() == "00ffffff",
			"The returned RGB HTML color code should match the expected value.");
	CHECK_MESSAGE(
			cyan_transparent.to_html() == "00ffff00",
			"The returned RGBA HTML color code should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_argb32() == 0xff00ffff,
			"The returned 32-bit RGB number should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_abgr32() == 0xffffff00,
			"The returned 32-bit BGR number should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_rgba32() == 0x00ffffff,
			"The returned 32-bit BGR number should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_argb64() == 0xffff'0000'ffff'ffff,
			"The returned 64-bit RGB number should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_abgr64() == 0xffff'ffff'ffff'0000,
			"The returned 64-bit BGR number should match the expected value.");
	CHECK_MESSAGE(
			cyan.to_rgba64() == 0x0000'ffff'ffff'ffff,
			"The returned 64-bit BGR number should match the expected value.");
	CHECK_MESSAGE(
			String(cyan) == "(0.0, 1.0, 1.0, 1.0)",
			"The string representation should match the expected value.");
}

TEST_CASE("[Color] Linear <-> sRGB conversion") {
	constexpr Color color = Color(0.35, 0.5, 0.6, 0.7);
	const Color color_linear = color.srgb_to_linear();
	const Color color_srgb = color.linear_to_srgb();
	CHECK_MESSAGE(
			color_linear.is_equal_approx(Color(0.100481, 0.214041, 0.318547, 0.7)),
			"The color converted to linear color space should match the expected value.");
	CHECK_MESSAGE(
			color_srgb.is_equal_approx(Color(0.62621, 0.735357, 0.797738, 0.7)),
			"The color converted to sRGB color space should match the expected value.");
	CHECK_MESSAGE(
			color_linear.linear_to_srgb().is_equal_approx(Color(0.35, 0.5, 0.6, 0.7)),
			"The linear color converted back to sRGB color space should match the expected value.");
	CHECK_MESSAGE(
			color_srgb.srgb_to_linear().is_equal_approx(Color(0.35, 0.5, 0.6, 0.7)),
			"The sRGB color converted back to linear color space should match the expected value.");
	CHECK_MESSAGE(
			Color(1.0, 1.0, 1.0, 1.0).srgb_to_linear() == (Color(1.0, 1.0, 1.0, 1.0)),
			"White converted from sRGB to linear should remain white.");
	CHECK_MESSAGE(
			Color(1.0, 1.0, 1.0, 1.0).linear_to_srgb() == (Color(1.0, 1.0, 1.0, 1.0)),
			"White converted from linear to sRGB should remain white.");
}

TEST_CASE("[Color] Named colors") {
	CHECK_MESSAGE(
			Color::named("red").is_equal_approx(Color::hex(0xFF0000FF)),
			"The named color \"red\" should match the expected value.");

	// Named colors have their names automatically normalized.
	CHECK_MESSAGE(
			Color::named("white_smoke").is_equal_approx(Color::hex(0xF5F5F5FF)),
			"The named color \"white_smoke\" should match the expected value.");
	CHECK_MESSAGE(
			Color::named("Slate Blue").is_equal_approx(Color::hex(0x6A5ACDFF)),
			"The named color \"Slate Blue\" should match the expected value.");

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			Color::named("doesn't exist").is_equal_approx(Color()),
			"The invalid named color \"doesn't exist\" should result in a Color with the default values.");
	ERR_PRINT_ON;
}

TEST_CASE("[Color] Validation methods") {
	CHECK_MESSAGE(
			Color::html_is_valid("#4080ff"),
			"Valid HTML color (with leading #) should be considered valid.");
	CHECK_MESSAGE(
			Color::html_is_valid("4080ff"),
			"Valid HTML color (without leading #) should be considered valid.");
	CHECK_MESSAGE(
			!Color::html_is_valid("12345"),
			"Invalid HTML color should be considered invalid.");
	CHECK_MESSAGE(
			!Color::html_is_valid("#fuf"),
			"Invalid HTML color should be considered invalid.");
}

TEST_CASE("[Color] Manipulation methods") {
	constexpr Color blue = Color(0, 0, 1, 0.4);

	CHECK_MESSAGE(
			blue.inverted().is_equal_approx(Color(1, 1, 0, 0.4)),
			"Inverted color should have its red, green and blue components inverted.");

	constexpr Color purple = Color(0.5, 0.2, 0.5, 0.25);

	CHECK_MESSAGE(
			purple.lightened(0.2).is_equal_approx(Color(0.6, 0.36, 0.6, 0.25)),
			"Color should be lightened by the expected amount.");
	CHECK_MESSAGE(
			purple.darkened(0.2).is_equal_approx(Color(0.4, 0.16, 0.4, 0.25)),
			"Color should be darkened by the expected amount.");

	constexpr Color red = Color(1, 0, 0, 0.2);
	constexpr Color yellow = Color(1, 1, 0, 0.8);

	CHECK_MESSAGE(
			red.lerp(yellow, 0.5).is_equal_approx(Color(1, 0.5, 0, 0.5)),
			"Red interpolated with yellow should be orange (with interpolated alpha).");
}

} // namespace TestColor

// === Merged from tests/core/math/test_expression.cpp ===
/*  test_expression.cpp                                                   */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestExpression {

TEST_CASE("[Expression] Integer arithmetic") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("-123456") == OK,
			"Integer identity should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == -123456,
			"Integer identity should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2 + 3") == OK,
			"Integer addition should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 5,
			"Integer addition should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("999999999999 + 999999999999") == OK,
			"Large integer addition should parse successfully.");
	CHECK_MESSAGE(
			int64_t(expression.execute()) == 1'999'999'999'998,
			"Large integer addition should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("25 / 10") == OK,
			"Integer / integer division should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 2,
			"Integer / integer division should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2 * (6 + 14) / 2 - 5") == OK,
			"Integer multiplication-addition-subtraction-division should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 15,
			"Integer multiplication-addition-subtraction-division should return the expected result.");
}

TEST_CASE("[Expression] Floating-point arithmetic") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("-123.456") == OK,
			"Float identity should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(-123.456),
			"Float identity should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2.0 + 3.0") == OK,
			"Float addition should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(5),
			"Float addition should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("3.0 / 10") == OK,
			"Float / integer division should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.3),
			"Float / integer division should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("3 / 10.0") == OK,
			"Basic integer / float division should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.3),
			"Basic integer / float division should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("3.0 / 10.0") == OK,
			"Float / float division should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.3),
			"Float / float division should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2.5 * (6.0 + 14.25) / 2.0 - 5.12345") == OK,
			"Float multiplication-addition-subtraction-division should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(20.18905),
			"Float multiplication-addition-subtraction-division should return the expected result.");
}

TEST_CASE("[Expression] Floating-point notation") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("2.") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(2.0),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("(2.)") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(2.0),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse(".3") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.3),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2.+5.") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(7.0),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse(".3-.8") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(-0.5),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2.+.2") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(2.2),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse(".0*0.") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.0),
			"The expression should return the expected result.");
}

TEST_CASE("[Expression] Scientific notation") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("2.e5") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("2.E5") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(200'000),
			"The expression should return the expected result.");

	// The middle "e" is ignored here.
	CHECK_MESSAGE(
			expression.parse("2e5") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(2e5),
			"The expression should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("2e.5") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(2),
			"The expression should return the expected result.");
}

TEST_CASE("[Expression] Underscored numeric literals") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("1_000_000") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("1_000.000") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("0xff_99_00") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("0Xff_99_00") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("0b10_11_00") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			expression.parse("0B10_11_00") == OK,
			"The expression should parse successfully.");
}

TEST_CASE("[Expression] Built-in functions") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("sqrt(pow(3, 2) + pow(4, 2))") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 5,
			"`sqrt(pow(3, 2) + pow(4, 2))` should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("snapped(sin(0.5), 0.01)") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			double(expression.execute()) == doctest::Approx(0.48),
			"`snapped(sin(0.5), 0.01)` should return the expected result.");

	CHECK_MESSAGE(
			expression.parse("pow(2.0, -2500)") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			Math::is_zero_approx(double(expression.execute())),
			"`pow(2.0, -2500)` should return the expected result (asymptotically zero).");
}

TEST_CASE("[Expression] Boolean expressions") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("24 >= 12") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			bool(expression.execute()),
			"The boolean expression should evaluate to `true`.");

	CHECK_MESSAGE(
			expression.parse("1.0 < 1.25 && 1.25 < 2.0") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			bool(expression.execute()),
			"The boolean expression should evaluate to `true`.");

	CHECK_MESSAGE(
			expression.parse("!2") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			!bool(expression.execute()),
			"The boolean expression should evaluate to `false`.");

	CHECK_MESSAGE(
			expression.parse("!!2") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			bool(expression.execute()),
			"The boolean expression should evaluate to `true`.");

	CHECK_MESSAGE(
			expression.parse("!0") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			bool(expression.execute()),
			"The boolean expression should evaluate to `true`.");

	CHECK_MESSAGE(
			expression.parse("!!0") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			!bool(expression.execute()),
			"The boolean expression should evaluate to `false`.");

	CHECK_MESSAGE(
			expression.parse("2 && 5") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			bool(expression.execute()),
			"The boolean expression should evaluate to `true`.");

	CHECK_MESSAGE(
			expression.parse("0 || 0") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			!bool(expression.execute()),
			"The boolean expression should evaluate to `false`.");

	CHECK_MESSAGE(
			expression.parse("(2 <= 4) && (2 > 5)") == OK,
			"The boolean expression should parse successfully.");
	CHECK_MESSAGE(
			!bool(expression.execute()),
			"The boolean expression should evaluate to `false`.");
}

TEST_CASE("[Expression] Expressions with variables") {
	Expression expression;

	PackedStringArray parameter_names = { "foo", "bar" };
	CHECK_MESSAGE(
			expression.parse("foo + bar + 50", parameter_names) == OK,
			"The expression should parse successfully.");
	Array values = { 60, 20 };
	CHECK_MESSAGE(
			int(expression.execute(values)) == 130,
			"The expression should return the expected value.");

	PackedStringArray parameter_names_invalid;
	parameter_names_invalid.push_back("foo");
	parameter_names_invalid.push_back("baz"); // Invalid parameter name.
	CHECK_MESSAGE(
			expression.parse("foo + bar + 50", parameter_names_invalid) == OK,
			"The expression should parse successfully.");
	Array values_invalid = { 60, 20 };
	// Invalid parameters will parse successfully but print an error message when executing.
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			int(expression.execute(values_invalid)) == 0,
			"The expression should return the expected value.");
	ERR_PRINT_ON;

	// Mismatched argument count (more values than parameters).
	PackedStringArray parameter_names_mismatch = { "foo", "bar" };
	CHECK_MESSAGE(
			expression.parse("foo + bar + 50", parameter_names_mismatch) == OK,
			"The expression should parse successfully.");
	Array values_mismatch = { 60, 20, 110 };
	CHECK_MESSAGE(
			int(expression.execute(values_mismatch)) == 130,
			"The expression should return the expected value.");

	// Mismatched argument count (more parameters than values).
	PackedStringArray parameter_names_mismatch2 = { "foo", "bar", "baz" };
	CHECK_MESSAGE(
			expression.parse("foo + bar + baz + 50", parameter_names_mismatch2) == OK,
			"The expression should parse successfully.");
	Array values_mismatch2 = { 60, 20 };
	// Having more parameters than values will parse successfully but print an
	// error message when executing.
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			int(expression.execute(values_mismatch2)) == 0,
			"The expression should return the expected value.");
	ERR_PRINT_ON;
}

TEST_CASE("[Expression] Invalid expressions") {
	Expression expression;

	CHECK_MESSAGE(
			expression.parse("\\") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("0++") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("()") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("()()") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("() - ()") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("() * 12345") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("() * 12345") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("123'456") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");

	CHECK_MESSAGE(
			expression.parse("123\"456") == ERR_INVALID_PARAMETER,
			"The expression shouldn't parse successfully.");
}

TEST_CASE("[Expression] Unusual expressions") {
	Expression expression;

	// Redundant parentheses don't cause a parse error as long as they're matched.
	CHECK_MESSAGE(
			expression.parse("(((((((((((((((666)))))))))))))))") == OK,
			"The expression should parse successfully.");

	// Using invalid identifiers doesn't cause a parse error.
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			expression.parse("hello + hello") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 0,
			"The expression should return the expected result.");
	ERR_PRINT_ON;

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			expression.parse("$1.00 + ???5") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 0,
			"The expression should return the expected result.");
	ERR_PRINT_ON;

	// Commas can't be used as a decimal parameter.
	CHECK_MESSAGE(
			expression.parse("123,456") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 123,
			"The expression should return the expected result.");

	// Spaces can't be used as a separator for large numbers.
	CHECK_MESSAGE(
			expression.parse("123 456") == OK,
			"The expression should parse successfully.");
	CHECK_MESSAGE(
			int(expression.execute()) == 123,
			"The expression should return the expected result.");

	// Division by zero is accepted, even though it prints an error message normally.
	CHECK_MESSAGE(
			expression.parse("-25.4 / 0") == OK,
			"The expression should parse successfully.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			Math::is_inf(double(expression.execute())),
			"`-25.4 / 0` should return inf.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			expression.parse("0 / 0") == OK,
			"The expression should parse successfully.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			int(expression.execute()) == 0,
			"`0 / 0` should return 0.");
	ERR_PRINT_ON;

	// The tests below currently crash the engine.
	//
	//CHECK_MESSAGE(
	//		expression.parse("(-9223372036854775807 - 1) % -1") == OK,
	//		"The expression should parse successfully.");
	//CHECK_MESSAGE(
	//		int64_t(expression.execute()) == 0,
	//		"`(-9223372036854775807 - 1) % -1` should return the expected result.");
	//
	//CHECK_MESSAGE(
	//		expression.parse("(-9223372036854775807 - 1) / -1") == OK,
	//		"The expression should parse successfully.");
	//CHECK_MESSAGE(
	//		int64_t(expression.execute()) == 0,
	//		"`(-9223372036854775807 - 1) / -1` should return the expected result.");
}

} // namespace TestExpression

// === Merged from tests/core/math/test_geometry_2d.cpp ===
/*  test_geometry_2d.cpp                                                  */
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

#include "tests/test_macros.h"



#include "core/math/geometry_3d.h"

namespace TestGeometry2D {

TEST_CASE("[Geometry2D] Point in circle") {
	CHECK(Geometry2D::is_point_in_circle(Vector2(0, 0), Vector2(0, 0), 1.0));

	CHECK(Geometry2D::is_point_in_circle(Vector2(0, 0), Vector2(11.99, 0), 12));
	CHECK(Geometry2D::is_point_in_circle(Vector2(-11.99, 0), Vector2(0, 0), 12));

	CHECK_FALSE(Geometry2D::is_point_in_circle(Vector2(0, 0), Vector2(12.01, 0), 12));
	CHECK_FALSE(Geometry2D::is_point_in_circle(Vector2(-12.01, 0), Vector2(0, 0), 12));

	CHECK(Geometry2D::is_point_in_circle(Vector2(7, -42), Vector2(4, -40), 3.7));
	CHECK_FALSE(Geometry2D::is_point_in_circle(Vector2(7, -42), Vector2(4, -40), 3.5));

	// This tests points on the edge of the circle. They are treated as being inside the circle.
	CHECK(Geometry2D::is_point_in_circle(Vector2(1.0, 0.0), Vector2(0, 0), 1.0));
	CHECK(Geometry2D::is_point_in_circle(Vector2(0.0, -1.0), Vector2(0, 0), 1.0));
}

TEST_CASE("[Geometry2D] Point in triangle") {
	CHECK(Geometry2D::is_point_in_triangle(Vector2(0, 0), Vector2(-1, 1), Vector2(0, -1), Vector2(1, 1)));
	CHECK_FALSE(Geometry2D::is_point_in_triangle(Vector2(-1.01, 1.0), Vector2(-1, 1), Vector2(0, -1), Vector2(1, 1)));

	CHECK(Geometry2D::is_point_in_triangle(Vector2(3, 2.5), Vector2(1, 4), Vector2(3, 2), Vector2(5, 4)));
	CHECK(Geometry2D::is_point_in_triangle(Vector2(-3, -2.5), Vector2(-1, -4), Vector2(-3, -2), Vector2(-5, -4)));
	CHECK_FALSE(Geometry2D::is_point_in_triangle(Vector2(0, 0), Vector2(1, 4), Vector2(3, 2), Vector2(5, 4)));

	// This tests points on the edge of the triangle. They are treated as being outside the triangle.
	// In `is_point_in_circle` and `is_point_in_polygon` they are treated as being inside, so in order the make
	// the behavior consistent this may change in the future (see issue #44717 and PR #44274).
	CHECK_FALSE(Geometry2D::is_point_in_triangle(Vector2(1, 1), Vector2(-1, 1), Vector2(0, -1), Vector2(1, 1)));
	CHECK_FALSE(Geometry2D::is_point_in_triangle(Vector2(0, 1), Vector2(-1, 1), Vector2(0, -1), Vector2(1, 1)));
}

TEST_CASE("[Geometry2D] Point in polygon") {
	Vector<Vector2> p;
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(0, 0), p));

	p.push_back(Vector2(-88, 120));
	p.push_back(Vector2(-74, -38));
	p.push_back(Vector2(135, -145));
	p.push_back(Vector2(425, 70));
	p.push_back(Vector2(68, 112));
	p.push_back(Vector2(-120, 370));
	p.push_back(Vector2(-323, -145));
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(-350, 0), p));
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(-110, 60), p));
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(412, 96), p));
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(83, 130), p));
	CHECK_FALSE(Geometry2D::is_point_in_polygon(Vector2(-320, -153), p));

	CHECK(Geometry2D::is_point_in_polygon(Vector2(0, 0), p));
	CHECK(Geometry2D::is_point_in_polygon(Vector2(-230, 0), p));
	CHECK(Geometry2D::is_point_in_polygon(Vector2(130, -110), p));
	CHECK(Geometry2D::is_point_in_polygon(Vector2(370, 55), p));
	CHECK(Geometry2D::is_point_in_polygon(Vector2(-160, 190), p));

	// This tests points on the edge of the polygon. They are treated as being inside the polygon.
	int c = p.size();
	for (int i = 0; i < c; i++) {
		const Vector2 &p1 = p[i];
		CHECK(Geometry2D::is_point_in_polygon(p1, p));

		const Vector2 &p2 = p[(i + 1) % c];
		Vector2 midpoint((p1 + p2) * 0.5);
		CHECK(Geometry2D::is_point_in_polygon(midpoint, p));
	}
}

TEST_CASE("[Geometry2D] Polygon clockwise") {
	Vector<Vector2> p;
	CHECK_FALSE(Geometry2D::is_polygon_clockwise(p));

	p.push_back(Vector2(5, -5));
	p.push_back(Vector2(-1, -5));
	p.push_back(Vector2(-5, -1));
	p.push_back(Vector2(-1, 3));
	p.push_back(Vector2(1, 5));
	CHECK(Geometry2D::is_polygon_clockwise(p));

	p.reverse();
	CHECK_FALSE(Geometry2D::is_polygon_clockwise(p));
}

TEST_CASE("[Geometry2D] Line intersection") {
	Vector2 r;
	CHECK(Geometry2D::line_intersects_line(Vector2(2, 0), Vector2(0, 1), Vector2(0, 2), Vector2(1, 0), r));
	CHECK(r.is_equal_approx(Vector2(2, 2)));

	CHECK(Geometry2D::line_intersects_line(Vector2(-1, 1), Vector2(1, -1), Vector2(4, 1), Vector2(-1, -1), r));
	CHECK(r.is_equal_approx(Vector2(1.5, -1.5)));

	CHECK(Geometry2D::line_intersects_line(Vector2(-1, 0), Vector2(-1, -1), Vector2(1, 0), Vector2(1, -1), r));
	CHECK(r.is_equal_approx(Vector2(0, 1)));

	CHECK_FALSE_MESSAGE(
			Geometry2D::line_intersects_line(Vector2(-1, 1), Vector2(1, -1), Vector2(0, 1), Vector2(1, -1), r),
			"Parallel lines should not intersect.");
}

TEST_CASE("[Geometry2D] Segment intersection") {
	Vector2 r;

	CHECK(Geometry2D::segment_intersects_segment(Vector2(-1, 1), Vector2(1, -1), Vector2(1, 1), Vector2(-1, -1), &r));
	CHECK(r.is_equal_approx(Vector2(0, 0)));

	CHECK_FALSE(Geometry2D::segment_intersects_segment(Vector2(-1, 1), Vector2(1, -1), Vector2(1, 1), Vector2(0.1, 0.1), &r));
	CHECK_FALSE(Geometry2D::segment_intersects_segment(Vector2(-1, 1), Vector2(1, -1), Vector2(0.1, 0.1), Vector2(1, 1), &r));

	CHECK_FALSE_MESSAGE(
			Geometry2D::segment_intersects_segment(Vector2(-1, 1), Vector2(1, -1), Vector2(0, 1), Vector2(2, -1), &r),
			"Parallel segments should not intersect.");

	CHECK_FALSE_MESSAGE(
			Geometry2D::segment_intersects_segment(Vector2(1, 2), Vector2(3, 2), Vector2(0, 2), Vector2(-2, 2), &r),
			"Non-overlapping collinear segments should not intersect.");

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_segment(Vector2(0, 0), Vector2(0, 1), Vector2(0, 0), Vector2(1, 0), &r),
			"Touching segments should intersect.");
	CHECK(r.is_equal_approx(Vector2(0, 0)));

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_segment(Vector2(0, 1), Vector2(0, 0), Vector2(0, 0), Vector2(1, 0), &r),
			"Touching segments should intersect.");
	CHECK(r.is_equal_approx(Vector2(0, 0)));
}

TEST_CASE("[Geometry2D] Segment intersection with circle") {
	constexpr real_t minus_one = -1.0;
	constexpr real_t zero = 0.0;
	constexpr real_t one_quarter = 0.25;
	constexpr real_t three_quarters = 0.75;
	constexpr real_t one = 1.0;

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(0, 0), Vector2(4, 0), Vector2(0, 0), 1.0) == doctest::Approx(one_quarter),
			"Segment from inside to outside of circle should intersect it.");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(4, 0), Vector2(0, 0), Vector2(0, 0), 1.0) == doctest::Approx(three_quarters),
			"Segment from outside to inside of circle should intersect it.");

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(-2, 0), Vector2(2, 0), Vector2(0, 0), 1.0) == doctest::Approx(one_quarter),
			"Segment running through circle should intersect it.");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(2, 0), Vector2(-2, 0), Vector2(0, 0), 1.0) == doctest::Approx(one_quarter),
			"Segment running through circle should intersect it.");

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(0, 0), Vector2(1, 0), Vector2(0, 0), 1.0) == doctest::Approx(one),
			"Segment starting inside the circle and ending on the circle should intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(1, 0), Vector2(0, 0), Vector2(0, 0), 1.0) == doctest::Approx(zero),
			"Segment starting on the circle and going inwards should intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(1, 0), Vector2(2, 0), Vector2(0, 0), 1.0) == doctest::Approx(zero),
			"Segment starting on the circle and going outwards should intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(2, 0), Vector2(1, 0), Vector2(0, 0), 1.0) == doctest::Approx(one),
			"Segment starting outside the circle and ending on the circle intersect it");

	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(-1, 0), Vector2(1, 0), Vector2(0, 0), 2.0) == doctest::Approx(minus_one),
			"Segment completely within the circle should not intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(1, 0), Vector2(-1, 0), Vector2(0, 0), 2.0) == doctest::Approx(minus_one),
			"Segment completely within the circle should not intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(2, 0), Vector2(3, 0), Vector2(0, 0), 1.0) == doctest::Approx(minus_one),
			"Segment completely outside the circle should not intersect it");
	CHECK_MESSAGE(
			Geometry2D::segment_intersects_circle(Vector2(3, 0), Vector2(2, 0), Vector2(0, 0), 1.0) == doctest::Approx(minus_one),
			"Segment completely outside the circle should not intersect it");
}

TEST_CASE("[Geometry2D] Segment intersection with polygon") {
	Vector<Point2> a;

	a.push_back(Point2(-2, 2));
	a.push_back(Point2(3, 4));
	a.push_back(Point2(1, 1));
	a.push_back(Point2(2, -2));
	a.push_back(Point2(-1, -1));

	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(0, 2), Vector2(2, 2), a),
			"Segment from inside to outside of polygon should intersect it.");
	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(2, 2), Vector2(0, 2), a),
			"Segment from outside to inside of polygon should intersect it.");

	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(2, 4), Vector2(3, 3), a),
			"Segment running through polygon should intersect it.");
	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(3, 3), Vector2(2, 4), a),
			"Segment running through polygon should intersect it.");

	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(0, 0), Vector2(1, 1), a),
			"Segment starting inside the polygon and ending on the polygon should intersect it");
	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(1, 1), Vector2(0, 0), a),
			"Segment starting on the polygon and going inwards should intersect it");
	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(-2, 2), Vector2(-2, -1), a),
			"Segment starting on the polygon and going outwards should intersect it");
	CHECK_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(-2, 1), Vector2(-2, 2), a),
			"Segment starting outside the polygon and ending on the polygon intersect it");

	CHECK_FALSE_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(-1, 2), Vector2(1, -1), a),
			"Segment completely within the polygon should not intersect it");
	CHECK_FALSE_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(1, -1), Vector2(-1, 2), a),
			"Segment completely within the polygon should not intersect it");
	CHECK_FALSE_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(2, 2), Vector2(2, -1), a),
			"Segment completely outside the polygon should not intersect it");
	CHECK_FALSE_MESSAGE(
			Geometry2D::is_segment_intersecting_polygon(Vector2(2, -1), Vector2(2, 2), a),
			"Segment completely outside the polygon should not intersect it");
}

TEST_CASE("[Geometry2D] Closest point to segment") {
	Vector2 a = Vector2(-4, -4);
	Vector2 b = Vector2(4, 4);
	CHECK(Geometry2D::get_closest_point_to_segment(Vector2(4.1, 4.1), a, b).is_equal_approx(Vector2(4, 4)));
	CHECK(Geometry2D::get_closest_point_to_segment(Vector2(-4.1, -4.1), a, b).is_equal_approx(Vector2(-4, -4)));
	CHECK(Geometry2D::get_closest_point_to_segment(Vector2(-1, 1), a, b).is_equal_approx(Vector2(0, 0)));

	a = Vector2(1, -2);
	b = Vector2(1, -2);
	CHECK_MESSAGE(
			Geometry2D::get_closest_point_to_segment(Vector2(-3, 4), a, b).is_equal_approx(Vector2(1, -2)),
			"Line segment is only a single point. This point should be the closest.");
}

TEST_CASE("[Geometry2D] Closest point to uncapped segment") {
	constexpr Vector2 a = Vector2(-4, -4);
	constexpr Vector2 b = Vector2(4, 4);
	CHECK(Geometry2D::get_closest_point_to_segment_uncapped(Vector2(-1, 1), a, b).is_equal_approx(Vector2(0, 0)));
	CHECK(Geometry2D::get_closest_point_to_segment_uncapped(Vector2(-4, -6), a, b).is_equal_approx(Vector2(-5, -5)));
	CHECK(Geometry2D::get_closest_point_to_segment_uncapped(Vector2(4, 6), a, b).is_equal_approx(Vector2(5, 5)));
}

TEST_CASE("[Geometry2D] Closest points between segments") {
	Vector2 c1, c2;
	// Basis Path Testing suite
	SUBCASE("[Geometry2D] Both segments degenerate to a point") {
		Geometry2D::get_closest_points_between_segments(Vector2(0, 0), Vector2(0, 0), Vector2(0, 0), Vector2(0, 0), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(0, 0)));
		CHECK(c2.is_equal_approx(Vector2(0, 0)));
	}

	SUBCASE("[Geometry2D] Closest point on second segment trajectory is above [0,1]") {
		Geometry2D::get_closest_points_between_segments(Vector2(50, -25), Vector2(50, -10), Vector2(-50, 10), Vector2(-40, 10), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(50, -10)));
		CHECK(c2.is_equal_approx(Vector2(-40, 10)));
	}

	SUBCASE("[Geometry2D] Parallel segments") {
		Geometry2D::get_closest_points_between_segments(Vector2(2, 1), Vector2(4, 3), Vector2(2, 3), Vector2(4, 5), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(3, 2)));
		CHECK(c2.is_equal_approx(Vector2(2, 3)));
	}

	SUBCASE("[Geometry2D] Closest point on second segment trajectory is within [0,1]") {
		Geometry2D::get_closest_points_between_segments(Vector2(2, 4), Vector2(2, 3), Vector2(1, 1), Vector2(4, 4), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(2, 3)));
		CHECK(c2.is_equal_approx(Vector2(2.5, 2.5)));
	}

	SUBCASE("[Geometry2D] Closest point on second segment trajectory is below [0,1]") {
		Geometry2D::get_closest_points_between_segments(Vector2(-20, -20), Vector2(-10, -40), Vector2(10, 25), Vector2(25, 40), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(-20, -20)));
		CHECK(c2.is_equal_approx(Vector2(10, 25)));
	}

	SUBCASE("[Geometry2D] Second segment degenerates to a point") {
		Geometry2D::get_closest_points_between_segments(Vector2(1, 2), Vector2(2, 1), Vector2(3, 3), Vector2(3, 3), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(1.5, 1.5)));
		CHECK(c2.is_equal_approx(Vector2(3, 3)));
	}

	SUBCASE("[Geometry2D] First segment degenerates to a point") {
		Geometry2D::get_closest_points_between_segments(Vector2(1, 1), Vector2(1, 1), Vector2(2, 2), Vector2(4, 4), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(1, 1)));
		CHECK(c2.is_equal_approx(Vector2(2, 2)));
	}
	// End Basis Path Testing suite

	SUBCASE("[Geometry2D] Segments are equal vectors") {
		Geometry2D::get_closest_points_between_segments(Vector2(2, 2), Vector2(3, 3), Vector2(4, 4), Vector2(4, 5), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(3, 3)));
		CHECK(c2.is_equal_approx(Vector2(4, 4)));
	}

	SUBCASE("[Geometry2D] Standard case") {
		Geometry2D::get_closest_points_between_segments(Vector2(0, 1), Vector2(-2, -1), Vector2(0, 0), Vector2(2, -2), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(-0.5, 0.5)));
		CHECK(c2.is_equal_approx(Vector2(0, 0)));
	}

	SUBCASE("[Geometry2D] Segments intersect") {
		Geometry2D::get_closest_points_between_segments(Vector2(-1, 1), Vector2(1, -1), Vector2(1, 1), Vector2(-1, -1), c1, c2);
		CHECK(c1.is_equal_approx(Vector2(0, 0)));
		CHECK(c2.is_equal_approx(Vector2(0, 0)));
	}
}

TEST_CASE("[Geometry2D] Make atlas") {
	Vector<Point2i> result;
	Size2i size;

	Vector<Size2i> r;
	r.push_back(Size2i(2, 2));
	Geometry2D::make_atlas(r, result, size);
	CHECK(size == Size2i(2, 2));
	CHECK(result.size() == r.size());

	r.clear();
	result.clear();
	r.push_back(Size2i(1, 2));
	r.push_back(Size2i(3, 4));
	r.push_back(Size2i(5, 6));
	r.push_back(Size2i(7, 8));
	Geometry2D::make_atlas(r, result, size);
	CHECK(result.size() == r.size());
}

TEST_CASE("[Geometry2D] Polygon intersection") {
	Vector<Point2> a;
	Vector<Point2> b;
	Vector<Vector<Point2>> r;

	a.push_back(Point2(30, 60));
	a.push_back(Point2(70, 5));
	a.push_back(Point2(200, 40));
	a.push_back(Point2(80, 200));

	SUBCASE("[Geometry2D] Both polygons are empty") {
		r = Geometry2D::intersect_polygons(Vector<Point2>(), Vector<Point2>());
		CHECK_MESSAGE(r.is_empty(), "Both polygons are empty. The intersection should also be empty.");
	}

	SUBCASE("[Geometry2D] One polygon is empty") {
		r = Geometry2D::intersect_polygons(a, b);
		REQUIRE_MESSAGE(r.is_empty(), "One polygon is empty. The intersection should also be empty.");
	}

	SUBCASE("[Geometry2D] Basic intersection") {
		b.push_back(Point2(200, 300));
		b.push_back(Point2(90, 200));
		b.push_back(Point2(50, 100));
		b.push_back(Point2(200, 90));
		r = Geometry2D::intersect_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "The polygons should intersect each other with 1 resulting intersection polygon.");
		REQUIRE_MESSAGE(r[0].size() == 3, "The resulting intersection polygon should have 3 vertices.");
		CHECK(r[0][0].is_equal_approx(Point2(86.52174, 191.30436)));
		CHECK(r[0][1].is_equal_approx(Point2(50, 100)));
		CHECK(r[0][2].is_equal_approx(Point2(160.52632, 92.63157)));
	}

	SUBCASE("[Geometry2D] Intersection with one polygon being completely inside the other polygon") {
		b.push_back(Point2(80, 100));
		b.push_back(Point2(50, 50));
		b.push_back(Point2(150, 50));
		r = Geometry2D::intersect_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "The polygons should intersect each other with 1 resulting intersection polygon.");
		REQUIRE_MESSAGE(r[0].size() == 3, "The resulting intersection polygon should have 3 vertices.");
		CHECK(r[0][0].is_equal_approx(b[0]));
		CHECK(r[0][1].is_equal_approx(b[1]));
		CHECK(r[0][2].is_equal_approx(b[2]));
	}

	SUBCASE("[Geometry2D] No intersection with 2 non-empty polygons") {
		b.push_back(Point2(150, 150));
		b.push_back(Point2(250, 100));
		b.push_back(Point2(300, 200));
		r = Geometry2D::intersect_polygons(a, b);
		REQUIRE_MESSAGE(r.is_empty(), "The polygons should not intersect each other.");
	}

	SUBCASE("[Geometry2D] Intersection with 2 resulting polygons") {
		a.clear();
		a.push_back(Point2(70, 5));
		a.push_back(Point2(140, 7));
		a.push_back(Point2(100, 52));
		a.push_back(Point2(170, 50));
		a.push_back(Point2(60, 125));
		b.push_back(Point2(70, 105));
		b.push_back(Point2(115, 55));
		b.push_back(Point2(90, 15));
		b.push_back(Point2(160, 50));
		r = Geometry2D::intersect_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 2, "The polygons should intersect each other with 2 resulting intersection polygons.");
		REQUIRE_MESSAGE(r[0].size() == 4, "The resulting intersection polygon should have 4 vertices.");
		CHECK(r[0][0].is_equal_approx(Point2(70, 105)));
		CHECK(r[0][1].is_equal_approx(Point2(115, 55)));
		CHECK(r[0][2].is_equal_approx(Point2(112.894737, 51.63158)));
		CHECK(r[0][3].is_equal_approx(Point2(159.509537, 50.299728)));

		REQUIRE_MESSAGE(r[1].size() == 3, "The intersection polygon should have 3 vertices.");
		CHECK(r[1][0].is_equal_approx(Point2(119.692307, 29.846149)));
		CHECK(r[1][1].is_equal_approx(Point2(107.706421, 43.33028)));
		CHECK(r[1][2].is_equal_approx(Point2(90, 15)));
	}
}

TEST_CASE("[Geometry2D] Merge polygons") {
	Vector<Point2> a;
	Vector<Point2> b;
	Vector<Vector<Point2>> r;

	a.push_back(Point2(225, 180));
	a.push_back(Point2(160, 230));
	a.push_back(Point2(20, 212));
	a.push_back(Point2(50, 115));

	SUBCASE("[Geometry2D] Both polygons are empty") {
		r = Geometry2D::merge_polygons(Vector<Point2>(), Vector<Point2>());
		REQUIRE_MESSAGE(r.is_empty(), "Both polygons are empty. The union should also be empty.");
	}

	SUBCASE("[Geometry2D] One polygon is empty") {
		r = Geometry2D::merge_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "One polygon is non-empty. There should be 1 resulting merged polygon.");
		REQUIRE_MESSAGE(r[0].size() == 4, "The resulting merged polygon should have 4 vertices.");
		CHECK(r[0][0].is_equal_approx(a[0]));
		CHECK(r[0][1].is_equal_approx(a[1]));
		CHECK(r[0][2].is_equal_approx(a[2]));
		CHECK(r[0][3].is_equal_approx(a[3]));
	}

	SUBCASE("[Geometry2D] Basic merge with 2 polygons") {
		b.push_back(Point2(180, 190));
		b.push_back(Point2(60, 140));
		b.push_back(Point2(160, 80));
		r = Geometry2D::merge_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "The merged polygons should result in 1 polygon.");
		REQUIRE_MESSAGE(r[0].size() == 7, "The resulting merged polygon should have 7 vertices.");
		CHECK(r[0][0].is_equal_approx(Point2(174.791077, 161.350967)));
		CHECK(r[0][1].is_equal_approx(Point2(225, 180)));
		CHECK(r[0][2].is_equal_approx(Point2(160, 230)));
		CHECK(r[0][3].is_equal_approx(Point2(20, 212)));
		CHECK(r[0][4].is_equal_approx(Point2(50, 115)));
		CHECK(r[0][5].is_equal_approx(Point2(81.911758, 126.852943)));
		CHECK(r[0][6].is_equal_approx(Point2(160, 80)));
	}

	SUBCASE("[Geometry2D] Merge with 2 resulting merged polygons (outline and hole)") {
		b.push_back(Point2(180, 190));
		b.push_back(Point2(140, 125));
		b.push_back(Point2(60, 140));
		b.push_back(Point2(160, 80));
		r = Geometry2D::merge_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 2, "The merged polygons should result in 2 polygons.");

		REQUIRE_MESSAGE(!Geometry2D::is_polygon_clockwise(r[0]), "The merged polygon (outline) should be counter-clockwise.");
		REQUIRE_MESSAGE(r[0].size() == 7, "The resulting merged polygon (outline) should have 7 vertices.");
		CHECK(r[0][0].is_equal_approx(Point2(174.791077, 161.350967)));
		CHECK(r[0][1].is_equal_approx(Point2(225, 180)));
		CHECK(r[0][2].is_equal_approx(Point2(160, 230)));
		CHECK(r[0][3].is_equal_approx(Point2(20, 212)));
		CHECK(r[0][4].is_equal_approx(Point2(50, 115)));
		CHECK(r[0][5].is_equal_approx(Point2(81.911758, 126.852943)));
		CHECK(r[0][6].is_equal_approx(Point2(160, 80)));

		REQUIRE_MESSAGE(Geometry2D::is_polygon_clockwise(r[1]), "The resulting merged polygon (hole) should be clockwise.");
		REQUIRE_MESSAGE(r[1].size() == 3, "The resulting merged polygon (hole) should have 3 vertices.");
		CHECK(r[1][0].is_equal_approx(Point2(98.083069, 132.859421)));
		CHECK(r[1][1].is_equal_approx(Point2(158.689453, 155.370377)));
		CHECK(r[1][2].is_equal_approx(Point2(140, 125)));
	}
}

TEST_CASE("[Geometry2D] Clip polygons") {
	Vector<Point2> a;
	Vector<Point2> b;
	Vector<Vector<Point2>> r;

	a.push_back(Point2(225, 180));
	a.push_back(Point2(160, 230));
	a.push_back(Point2(20, 212));
	a.push_back(Point2(50, 115));

	SUBCASE("[Geometry2D] Both polygons are empty") {
		r = Geometry2D::clip_polygons(Vector<Point2>(), Vector<Point2>());
		CHECK_MESSAGE(r.is_empty(), "Both polygons are empty. The clip should also be empty.");
	}

	SUBCASE("[Geometry2D] Basic clip with one result polygon") {
		b.push_back(Point2(250, 170));
		b.push_back(Point2(175, 270));
		b.push_back(Point2(120, 260));
		b.push_back(Point2(25, 80));
		r = Geometry2D::clip_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "The clipped polygons should result in 1 polygon.");
		REQUIRE_MESSAGE(r[0].size() == 3, "The resulting clipped polygon should have 3 vertices.");
		CHECK(r[0][0].is_equal_approx(Point2(100.102173, 222.298843)));
		CHECK(r[0][1].is_equal_approx(Point2(20, 212)));
		CHECK(r[0][2].is_equal_approx(Point2(47.588089, 122.798492)));
	}

	SUBCASE("[Geometry2D] Polygon b completely overlaps polygon a") {
		b.push_back(Point2(250, 170));
		b.push_back(Point2(175, 270));
		b.push_back(Point2(10, 210));
		b.push_back(Point2(55, 80));
		r = Geometry2D::clip_polygons(a, b);
		CHECK_MESSAGE(r.is_empty(), "Polygon 'b' completely overlaps polygon 'a'. This should result in no clipped polygons.");
	}

	SUBCASE("[Geometry2D] Polygon a completely overlaps polygon b") {
		b.push_back(Point2(150, 200));
		b.push_back(Point2(65, 190));
		b.push_back(Point2(80, 140));
		r = Geometry2D::clip_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 2, "Polygon 'a' completely overlaps polygon 'b'. This should result in 2 clipped polygons.");
		REQUIRE_MESSAGE(r[0].size() == 4, "The resulting clipped polygon should have 4 vertices.");
		REQUIRE_MESSAGE(!Geometry2D::is_polygon_clockwise(r[0]), "The resulting clipped polygon (outline) should be counter-clockwise.");
		CHECK(r[0][0].is_equal_approx(a[0]));
		CHECK(r[0][1].is_equal_approx(a[1]));
		CHECK(r[0][2].is_equal_approx(a[2]));
		CHECK(r[0][3].is_equal_approx(a[3]));
		REQUIRE_MESSAGE(r[1].size() == 3, "The resulting clipped polygon should have 3 vertices.");
		REQUIRE_MESSAGE(Geometry2D::is_polygon_clockwise(r[1]), "The resulting clipped polygon (hole) should be clockwise.");
		CHECK(r[1][0].is_equal_approx(b[1]));
		CHECK(r[1][1].is_equal_approx(b[0]));
		CHECK(r[1][2].is_equal_approx(b[2]));
	}
}

TEST_CASE("[Geometry2D] Exclude polygons") {
	Vector<Point2> a;
	Vector<Point2> b;
	Vector<Vector<Point2>> r;

	a.push_back(Point2(225, 180));
	a.push_back(Point2(160, 230));
	a.push_back(Point2(20, 212));
	a.push_back(Point2(50, 115));

	SUBCASE("[Geometry2D] Both polygons are empty") {
		r = Geometry2D::exclude_polygons(Vector<Point2>(), Vector<Point2>());
		CHECK_MESSAGE(r.is_empty(), "Both polygons are empty. The excluded polygon should also be empty.");
	}

	SUBCASE("[Geometry2D] One polygon is empty") {
		r = Geometry2D::exclude_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 1, "One polygon is non-empty. There should be 1 resulting excluded polygon.");
		REQUIRE_MESSAGE(r[0].size() == 4, "The resulting excluded polygon should have 4 vertices.");
		CHECK(r[0][0].is_equal_approx(a[0]));
		CHECK(r[0][1].is_equal_approx(a[1]));
		CHECK(r[0][2].is_equal_approx(a[2]));
		CHECK(r[0][3].is_equal_approx(a[3]));
	}

	SUBCASE("[Geometry2D] Exclude with 2 resulting polygons (outline and hole)") {
		b.push_back(Point2(140, 160));
		b.push_back(Point2(150, 220));
		b.push_back(Point2(40, 200));
		b.push_back(Point2(60, 140));
		r = Geometry2D::exclude_polygons(a, b);
		REQUIRE_MESSAGE(r.size() == 2, "There should be 2 resulting excluded polygons (outline and hole).");
		REQUIRE_MESSAGE(r[0].size() == 4, "The resulting excluded polygon should have 4 vertices.");
		REQUIRE_MESSAGE(!Geometry2D::is_polygon_clockwise(r[0]), "The resulting excluded polygon (outline) should be counter-clockwise.");
		CHECK(r[0][0].is_equal_approx(a[0]));
		CHECK(r[0][1].is_equal_approx(a[1]));
		CHECK(r[0][2].is_equal_approx(a[2]));
		CHECK(r[0][3].is_equal_approx(a[3]));
		REQUIRE_MESSAGE(r[1].size() == 4, "The resulting excluded polygon should have 4 vertices.");
		REQUIRE_MESSAGE(Geometry2D::is_polygon_clockwise(r[1]), "The resulting excluded polygon (hole) should be clockwise.");
		CHECK(r[1][0].is_equal_approx(Point2(40, 200)));
		CHECK(r[1][1].is_equal_approx(Point2(150, 220)));
		CHECK(r[1][2].is_equal_approx(Point2(140, 160)));
		CHECK(r[1][3].is_equal_approx(Point2(60, 140)));
	}
}

TEST_CASE("[Geometry2D] Intersect polyline with polygon") {
	Vector<Vector2> l;
	Vector<Vector2> p;
	Vector<Vector<Point2>> r;

	l.push_back(Vector2(100, 90));
	l.push_back(Vector2(120, 250));

	p.push_back(Vector2(225, 180));
	p.push_back(Vector2(160, 230));
	p.push_back(Vector2(20, 212));
	p.push_back(Vector2(50, 115));

	SUBCASE("[Geometry2D] Both line and polygon are empty") {
		r = Geometry2D::intersect_polyline_with_polygon(Vector<Vector2>(), Vector<Vector2>());
		CHECK_MESSAGE(r.is_empty(), "Both line and polygon are empty. The intersection line should also be empty.");
	}

	SUBCASE("[Geometry2D] Line is non-empty and polygon is empty") {
		r = Geometry2D::intersect_polyline_with_polygon(l, Vector<Vector2>());
		CHECK_MESSAGE(r.is_empty(), "The polygon is empty while the line is non-empty. The intersection line should be empty.");
	}

	SUBCASE("[Geometry2D] Basic intersection with 1 resulting intersection line") {
		r = Geometry2D::intersect_polyline_with_polygon(l, p);
		REQUIRE_MESSAGE(r.size() == 1, "There should be 1 resulting intersection line.");
		REQUIRE_MESSAGE(r[0].size() == 2, "The resulting intersection line should have 2 vertices.");
		CHECK(r[0][0].is_equal_approx(Vector2(105.711609, 135.692886)));
		CHECK(r[0][1].is_equal_approx(Vector2(116.805809, 224.446457)));
	}

	SUBCASE("[Geometry2D] Complex intersection with 2 resulting intersection lines") {
		l.clear();
		l.push_back(Vector2(100, 90));
		l.push_back(Vector2(190, 255));
		l.push_back(Vector2(135, 260));
		l.push_back(Vector2(57, 200));
		l.push_back(Vector2(50, 170));
		l.push_back(Vector2(15, 155));
		r = Geometry2D::intersect_polyline_with_polygon(l, p);
		REQUIRE_MESSAGE(r.size() == 2, "There should be 2 resulting intersection lines.");
		REQUIRE_MESSAGE(r[0].size() == 2, "The resulting intersection line should have 2 vertices.");
		CHECK(r[0][0].is_equal_approx(Vector2(129.804565, 144.641693)));
		CHECK(r[0][1].is_equal_approx(Vector2(171.527084, 221.132996)));
		REQUIRE_MESSAGE(r[1].size() == 4, "The resulting intersection line should have 4 vertices.");
		CHECK(r[1][0].is_equal_approx(Vector2(83.15609, 220.120087)));
		CHECK(r[1][1].is_equal_approx(Vector2(57, 200)));
		CHECK(r[1][2].is_equal_approx(Vector2(50, 170)));
		CHECK(r[1][3].is_equal_approx(Vector2(34.980492, 163.563065)));
	}
}

TEST_CASE("[Geometry2D] Clip polyline with polygon") {
	Vector<Vector2> l;
	Vector<Vector2> p;
	Vector<Vector<Point2>> r;

	l.push_back(Vector2(70, 140));
	l.push_back(Vector2(160, 320));

	p.push_back(Vector2(225, 180));
	p.push_back(Vector2(160, 230));
	p.push_back(Vector2(20, 212));
	p.push_back(Vector2(50, 115));

	SUBCASE("[Geometry2D] Both line and polygon are empty") {
		r = Geometry2D::clip_polyline_with_polygon(Vector<Vector2>(), Vector<Vector2>());
		CHECK_MESSAGE(r.is_empty(), "Both line and polygon are empty. The clipped line should also be empty.");
	}

	SUBCASE("[Geometry2D] Polygon is empty and line is non-empty") {
		r = Geometry2D::clip_polyline_with_polygon(l, Vector<Vector2>());
		REQUIRE_MESSAGE(r.size() == 1, "There should be 1 resulting clipped line.");
		REQUIRE_MESSAGE(r[0].size() == 2, "The resulting clipped line should have 2 vertices.");
		CHECK(r[0][0].is_equal_approx(l[0]));
		CHECK(r[0][1].is_equal_approx(l[1]));
	}

	SUBCASE("[Geometry2D] Basic clip with 1 resulting clipped line") {
		r = Geometry2D::clip_polyline_with_polygon(l, p);
		REQUIRE_MESSAGE(r.size() == 1, "There should be 1 resulting clipped line.");
		REQUIRE_MESSAGE(r[0].size() == 2, "The resulting clipped line should have 2 vertices.");
		CHECK(r[0][0].is_equal_approx(Vector2(111.908401, 223.816803)));
		CHECK(r[0][1].is_equal_approx(Vector2(160, 320)));
	}

	SUBCASE("[Geometry2D] Complex clip with 2 resulting clipped lines") {
		l.clear();
		l.push_back(Vector2(55, 70));
		l.push_back(Vector2(50, 190));
		l.push_back(Vector2(120, 165));
		l.push_back(Vector2(122, 250));
		l.push_back(Vector2(160, 320));
		r = Geometry2D::clip_polyline_with_polygon(l, p);
		REQUIRE_MESSAGE(r.size() == 2, "There should be 2 resulting clipped lines.");
		REQUIRE_MESSAGE(r[0].size() == 3, "The resulting clipped line should have 3 vertices.");
		CHECK(r[0][0].is_equal_approx(Vector2(121.412682, 225.038757)));
		CHECK(r[0][1].is_equal_approx(Vector2(122, 250)));
		CHECK(r[0][2].is_equal_approx(Vector2(160, 320)));
		REQUIRE_MESSAGE(r[1].size() == 2, "The resulting clipped line should have 2 vertices.");
		CHECK(r[1][0].is_equal_approx(Vector2(55, 70)));
		CHECK(r[1][1].is_equal_approx(Vector2(53.07737, 116.143021)));
	}
}

TEST_CASE("[Geometry2D] Convex hull") {
	Vector<Point2> a;
	Vector<Point2> r;

	a.push_back(Point2(-4, -8));
	a.push_back(Point2(-10, -4));
	a.push_back(Point2(8, 2));
	a.push_back(Point2(-6, 10));
	a.push_back(Point2(-12, 4));
	a.push_back(Point2(10, -8));
	a.push_back(Point2(4, 8));

	SUBCASE("[Geometry2D] No points") {
		r = Geometry2D::convex_hull(Vector<Vector2>());

		CHECK_MESSAGE(r.is_empty(), "The convex hull should be empty if there are no input points.");
	}

	SUBCASE("[Geometry2D] Single point") {
		Vector<Point2> b;
		b.push_back(Point2(4, -3));

		r = Geometry2D::convex_hull(b);
		REQUIRE_MESSAGE(r.size() == 1, "Convex hull should contain 1 point.");
		CHECK(r[0].is_equal_approx(b[0]));
	}

	SUBCASE("[Geometry2D] All points form the convex hull") {
		r = Geometry2D::convex_hull(a);
		REQUIRE_MESSAGE(r.size() == 8, "Convex hull should contain 8 points.");
		CHECK(r[0].is_equal_approx(Point2(-12, 4)));
		CHECK(r[1].is_equal_approx(Point2(-10, -4)));
		CHECK(r[2].is_equal_approx(Point2(-4, -8)));
		CHECK(r[3].is_equal_approx(Point2(10, -8)));
		CHECK(r[4].is_equal_approx(Point2(8, 2)));
		CHECK(r[5].is_equal_approx(Point2(4, 8)));
		CHECK(r[6].is_equal_approx(Point2(-6, 10)));
		CHECK(r[7].is_equal_approx(Point2(-12, 4)));
	}

	SUBCASE("[Geometry2D] Add extra points inside original convex hull") {
		a.push_back(Point2(-4, -8));
		a.push_back(Point2(0, 0));
		a.push_back(Point2(0, 8));
		a.push_back(Point2(-10, -3));
		a.push_back(Point2(9, -4));
		a.push_back(Point2(6, 4));

		r = Geometry2D::convex_hull(a);
		REQUIRE_MESSAGE(r.size() == 8, "Convex hull should contain 8 points.");
		CHECK(r[0].is_equal_approx(Point2(-12, 4)));
		CHECK(r[1].is_equal_approx(Point2(-10, -4)));
		CHECK(r[2].is_equal_approx(Point2(-4, -8)));
		CHECK(r[3].is_equal_approx(Point2(10, -8)));
		CHECK(r[4].is_equal_approx(Point2(8, 2)));
		CHECK(r[5].is_equal_approx(Point2(4, 8)));
		CHECK(r[6].is_equal_approx(Point2(-6, 10)));
		CHECK(r[7].is_equal_approx(Point2(-12, 4)));
	}

	SUBCASE("[Geometry2D] Add extra points on border of original convex hull") {
		a.push_back(Point2(9, -3));
		a.push_back(Point2(-2, -8));

		r = Geometry2D::convex_hull(a);
		REQUIRE_MESSAGE(r.size() == 8, "Convex hull should contain 8 points.");
		CHECK(r[0].is_equal_approx(Point2(-12, 4)));
		CHECK(r[1].is_equal_approx(Point2(-10, -4)));
		CHECK(r[2].is_equal_approx(Point2(-4, -8)));
		CHECK(r[3].is_equal_approx(Point2(10, -8)));
		CHECK(r[4].is_equal_approx(Point2(8, 2)));
		CHECK(r[5].is_equal_approx(Point2(4, 8)));
		CHECK(r[6].is_equal_approx(Point2(-6, 10)));
		CHECK(r[7].is_equal_approx(Point2(-12, 4)));
	}

	SUBCASE("[Geometry2D] Add extra points outside border of original convex hull") {
		a.push_back(Point2(-11, -1));
		a.push_back(Point2(7, 6));

		r = Geometry2D::convex_hull(a);
		REQUIRE_MESSAGE(r.size() == 10, "Convex hull should contain 10 points.");
		CHECK(r[0].is_equal_approx(Point2(-12, 4)));
		CHECK(r[1].is_equal_approx(Point2(-11, -1)));
		CHECK(r[2].is_equal_approx(Point2(-10, -4)));
		CHECK(r[3].is_equal_approx(Point2(-4, -8)));
		CHECK(r[4].is_equal_approx(Point2(10, -8)));
		CHECK(r[5].is_equal_approx(Point2(8, 2)));
		CHECK(r[6].is_equal_approx(Point2(7, 6)));
		CHECK(r[7].is_equal_approx(Point2(4, 8)));
		CHECK(r[8].is_equal_approx(Point2(-6, 10)));
		CHECK(r[9].is_equal_approx(Point2(-12, 4)));
	}
}

TEST_CASE("[Geometry2D] Bresenham line") {
	Vector<Vector2i> r;

	SUBCASE("[Geometry2D] Single point") {
		r = Geometry2D::bresenham_line(Point2i(0, 0), Point2i(0, 0));

		REQUIRE_MESSAGE(r.size() == 1, "The Bresenham line should contain exactly one point.");
		CHECK(r[0] == Vector2i(0, 0));
	}

	SUBCASE("[Geometry2D] Line parallel to x-axis") {
		r = Geometry2D::bresenham_line(Point2i(1, 2), Point2i(5, 2));

		REQUIRE_MESSAGE(r.size() == 5, "The Bresenham line should contain exactly five points.");
		CHECK(r[0] == Vector2i(1, 2));
		CHECK(r[1] == Vector2i(2, 2));
		CHECK(r[2] == Vector2i(3, 2));
		CHECK(r[3] == Vector2i(4, 2));
		CHECK(r[4] == Vector2i(5, 2));
	}

	SUBCASE("[Geometry2D] 45 degree line from the origin") {
		r = Geometry2D::bresenham_line(Point2i(0, 0), Point2i(4, 4));

		REQUIRE_MESSAGE(r.size() == 5, "The Bresenham line should contain exactly five points.");
		CHECK(r[0] == Vector2i(0, 0));
		CHECK(r[1] == Vector2i(1, 1));
		CHECK(r[2] == Vector2i(2, 2));
		CHECK(r[3] == Vector2i(3, 3));
		CHECK(r[4] == Vector2i(4, 4));
	}

	SUBCASE("[Geometry2D] Sloped line going up one unit") {
		r = Geometry2D::bresenham_line(Point2i(0, 0), Point2i(4, 1));

		REQUIRE_MESSAGE(r.size() == 5, "The Bresenham line should contain exactly five points.");
		CHECK(r[0] == Vector2i(0, 0));
		CHECK(r[1] == Vector2i(1, 0));
		CHECK(r[2] == Vector2i(2, 0));
		CHECK(r[3] == Vector2i(3, 1));
		CHECK(r[4] == Vector2i(4, 1));
	}

	SUBCASE("[Geometry2D] Sloped line going up two units") {
		r = Geometry2D::bresenham_line(Point2i(0, 0), Point2i(4, 2));

		REQUIRE_MESSAGE(r.size() == 5, "The Bresenham line should contain exactly five points.");
		CHECK(r[0] == Vector2i(0, 0));
		CHECK(r[1] == Vector2i(1, 0));
		CHECK(r[2] == Vector2i(2, 1));
		CHECK(r[3] == Vector2i(3, 1));
		CHECK(r[4] == Vector2i(4, 2));
	}

	SUBCASE("[Geometry2D] Long sloped line") {
		r = Geometry2D::bresenham_line(Point2i(0, 0), Point2i(11, 5));

		REQUIRE_MESSAGE(r.size() == 12, "The Bresenham line should contain exactly twelve points.");
		CHECK(r[0] == Vector2i(0, 0));
		CHECK(r[1] == Vector2i(1, 0));
		CHECK(r[2] == Vector2i(2, 1));
		CHECK(r[3] == Vector2i(3, 1));
		CHECK(r[4] == Vector2i(4, 2));
		CHECK(r[5] == Vector2i(5, 2));
		CHECK(r[6] == Vector2i(6, 3));
		CHECK(r[7] == Vector2i(7, 3));
		CHECK(r[8] == Vector2i(8, 4));
		CHECK(r[9] == Vector2i(9, 4));
		CHECK(r[10] == Vector2i(10, 5));
		CHECK(r[11] == Vector2i(11, 5));
	}
}

} // namespace TestGeometry2D

// === Merged from tests/core/math/test_geometry_3d.cpp ===
/*  test_geometry_3d.cpp                                                  */
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

#include "tests/test_macros.h"



#include "core/math/geometry_3d.h"

namespace TestGeometry3D {

TEST_CASE("[Geometry3D] Closest Points Between Segments") {
	Vector3 ps, qt;
	Geometry3D::get_closest_points_between_segments(Vector3(1, -1, 1), Vector3(1, 1, -1), Vector3(-1, -2, -1), Vector3(-1, 1, 1), ps, qt);
	CHECK(ps.is_equal_approx(Vector3(1, -0.2, 0.2)));
	CHECK(qt.is_equal_approx(Vector3(-1, -0.2, 0.2)));
}

TEST_CASE("[Geometry3D] Closest Distance Between Segments") {
	CHECK(Geometry3D::get_closest_distance_between_segments(Vector3(1, -2, 0), Vector3(1, 2, 0), Vector3(-1, 2, 0), Vector3(-1, -2, 0)) == 2.0f);
}

TEST_CASE("[Geometry3D] Build Box Planes") {
	constexpr Vector3 extents = Vector3(5, 5, 20);
	Vector<Plane> box = Geometry3D::build_box_planes(extents);
	CHECK(box.size() == 6);
	CHECK(extents.x == box[0].d);
	CHECK(box[0].normal == Vector3(1, 0, 0));
	CHECK(extents.x == box[1].d);
	CHECK(box[1].normal == Vector3(-1, 0, 0));
	CHECK(extents.y == box[2].d);
	CHECK(box[2].normal == Vector3(0, 1, 0));
	CHECK(extents.y == box[3].d);
	CHECK(box[3].normal == Vector3(0, -1, 0));
	CHECK(extents.z == box[4].d);
	CHECK(box[4].normal == Vector3(0, 0, 1));
	CHECK(extents.z == box[5].d);
	CHECK(box[5].normal == Vector3(0, 0, -1));
}

TEST_CASE("[Geometry3D] Build Capsule Planes") {
	Vector<Plane> capsule = Geometry3D::build_capsule_planes(10, 20, 6, 10);
	CHECK(capsule.size() == 126);
}

TEST_CASE("[Geometry3D] Build Cylinder Planes") {
	Vector<Plane> planes = Geometry3D::build_cylinder_planes(3.0f, 10.0f, 10);
	CHECK(planes.size() == 12);
}

TEST_CASE("[Geometry3D] Build Sphere Planes") {
	Vector<Plane> planes = Geometry3D::build_sphere_planes(10.0f, 10, 3);
	CHECK(planes.size() == 63);
}

#if false
// This test has been temporarily disabled because it's really fragile and
// breaks if calculations change very slightly. For example, it breaks when
// using doubles, and it breaks when making Plane calculations more accurate.
TEST_CASE("[Geometry3D] Build Convex Mesh") {
	struct Case {
		Vector<Plane> object;
		int want_faces, want_edges, want_vertices;
		Case(){};
		Case(Vector<Plane> p_object, int p_want_faces, int p_want_edges, int p_want_vertices) :
				object(p_object), want_faces(p_want_faces), want_edges(p_want_edges), want_vertices(p_want_vertices){};
	};
	Vector<Case> tt;
	tt.push_back(Case(Geometry3D::build_box_planes(Vector3(5, 10, 5)), 6, 12, 8));
	tt.push_back(Case(Geometry3D::build_capsule_planes(5, 5, 20, 20, Vector3::Axis()), 820, 7603, 6243));
	tt.push_back(Case(Geometry3D::build_cylinder_planes(5, 5, 20, Vector3::Axis()), 22, 100, 80));
	tt.push_back(Case(Geometry3D::build_sphere_planes(5, 5, 20), 220, 1011, 522));
	for (int i = 0; i < tt.size(); ++i) {
		Case current_case = tt[i];
		Geometry3D::MeshData mesh = Geometry3D::build_convex_mesh(current_case.object);
		CHECK(mesh.faces.size() == current_case.want_faces);
		CHECK(mesh.edges.size() == current_case.want_edges);
		CHECK(mesh.vertices.size() == current_case.want_vertices);
	}
}
#endif

TEST_CASE("[Geometry3D] Clip Polygon") {
	Vector<Plane> box_planes = Geometry3D::build_box_planes(Vector3(5, 10, 5));
	Vector<Vector3> box = Geometry3D::compute_convex_mesh_points(&box_planes[0], box_planes.size());
	Vector<Vector3> output = Geometry3D::clip_polygon(box, Plane());
	CHECK(output == box);
	output = Geometry3D::clip_polygon(box, Plane(Vector3(0, 1, 0), Vector3(0, 3, 0)));
	CHECK(output != box);
}

TEST_CASE("[Geometry3D] Compute Convex Mesh Points") {
	Vector<Vector3> cube;
	cube.push_back(Vector3(-5, -5, -5));
	cube.push_back(Vector3(5, -5, -5));
	cube.push_back(Vector3(-5, 5, -5));
	cube.push_back(Vector3(5, 5, -5));
	cube.push_back(Vector3(-5, -5, 5));
	cube.push_back(Vector3(5, -5, 5));
	cube.push_back(Vector3(-5, 5, 5));
	cube.push_back(Vector3(5, 5, 5));
	Vector<Plane> box_planes = Geometry3D::build_box_planes(Vector3(5, 5, 5));
	CHECK(Geometry3D::compute_convex_mesh_points(&box_planes[0], box_planes.size()) == cube);
}

TEST_CASE("[Geometry3D] Get Closest Point To Segment") {
	constexpr Vector3 a = Vector3(1, 1, 1);
	constexpr Vector3 b = Vector3(5, 5, 5);
	Vector3 output = Geometry3D::get_closest_point_to_segment(Vector3(2, 1, 4), a, b);
	CHECK(output.is_equal_approx(Vector3(2.33333, 2.33333, 2.33333)));
}

TEST_CASE("[Geometry3D] Plane and Box Overlap") {
	CHECK(Geometry3D::planeBoxOverlap(Vector3(3, 4, 2), 5.0f, Vector3(5, 5, 5)) == true);
	CHECK(Geometry3D::planeBoxOverlap(Vector3(0, 1, 0), -10.0f, Vector3(5, 5, 5)) == false);
	CHECK(Geometry3D::planeBoxOverlap(Vector3(1, 0, 0), -6.0f, Vector3(5, 5, 5)) == false);
}

TEST_CASE("[Geometry3D] Is Point in Projected Triangle") {
	CHECK(Geometry3D::point_in_projected_triangle(Vector3(1, 1, 0), Vector3(3, 0, 0), Vector3(0, 3, 0), Vector3(-3, 0, 0)) == true);
	CHECK(Geometry3D::point_in_projected_triangle(Vector3(5, 1, 0), Vector3(3, 0, 0), Vector3(0, 3, 0), Vector3(-3, 0, 0)) == false);
	CHECK(Geometry3D::point_in_projected_triangle(Vector3(3, 0, 0), Vector3(3, 0, 0), Vector3(0, 3, 0), Vector3(-3, 0, 0)) == true);
}

TEST_CASE("[Geometry3D] Does Ray Intersect Triangle") {
	Vector3 result;
	CHECK(Geometry3D::ray_intersects_triangle(Vector3(0, 1, 1), Vector3(0, 0, -10), Vector3(0, 3, 0), Vector3(-3, 0, 0), Vector3(3, 0, 0), &result) == true);
	CHECK(Geometry3D::ray_intersects_triangle(Vector3(5, 10, 1), Vector3(0, 0, -10), Vector3(0, 3, 0), Vector3(-3, 0, 0), Vector3(3, 0, 0), &result) == false);
	CHECK(Geometry3D::ray_intersects_triangle(Vector3(0, 1, 1), Vector3(0, 0, 10), Vector3(0, 3, 0), Vector3(-3, 0, 0), Vector3(3, 0, 0), &result) == false);
}

TEST_CASE("[Geometry3D] Does Segment Intersect Convex") {
	Vector<Plane> box_planes = Geometry3D::build_box_planes(Vector3(5, 5, 5));
	Vector3 result, normal;
	CHECK(Geometry3D::segment_intersects_convex(Vector3(10, 10, 10), Vector3(0, 0, 0), &box_planes[0], box_planes.size(), &result, &normal) == true);
	CHECK(Geometry3D::segment_intersects_convex(Vector3(10, 10, 10), Vector3(5, 5, 5), &box_planes[0], box_planes.size(), &result, &normal) == true);
	CHECK(Geometry3D::segment_intersects_convex(Vector3(10, 10, 10), Vector3(6, 5, 5), &box_planes[0], box_planes.size(), &result, &normal) == false);
	CHECK(Geometry3D::segment_intersects_convex(Vector3(10, 10, 0), Vector3(10, 0, 0), &box_planes[0], box_planes.size(), &result, &normal) == false);
}

TEST_CASE("[Geometry3D] Segment Intersects Cylinder") {
	Vector3 result, normal;
	CHECK(Geometry3D::segment_intersects_cylinder(Vector3(10, 10, 10), Vector3(0, 0, 0), 5, 5, &result, &normal) == true);
	CHECK(Geometry3D::segment_intersects_cylinder(Vector3(10, 10, 10), Vector3(6, 6, 6), 5, 5, &result, &normal) == false);
}

TEST_CASE("[Geometry3D] Segment Intersects Cylinder") {
	Vector3 result, normal;
	CHECK(Geometry3D::segment_intersects_sphere(Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3(0, 0, 0), 5, &result, &normal) == true);
	CHECK(Geometry3D::segment_intersects_sphere(Vector3(10, 10, 10), Vector3(0, 0, 2.5), Vector3(0, 0, 0), 5, &result, &normal) == true);
	CHECK(Geometry3D::segment_intersects_sphere(Vector3(10, 10, 10), Vector3(5, 5, 5), Vector3(0, 0, 0), 5, &result, &normal) == false);
}

TEST_CASE("[Geometry3D] Segment Intersects Triangle") {
	Vector3 result;
	CHECK(Geometry3D::segment_intersects_triangle(Vector3(1, 1, 1), Vector3(-1, -1, -1), Vector3(-3, 0, 0), Vector3(0, 3, 0), Vector3(3, 0, 0), &result) == true);
	CHECK(Geometry3D::segment_intersects_triangle(Vector3(1, 1, 1), Vector3(3, 0, 0), Vector3(-3, 0, 0), Vector3(0, 3, 0), Vector3(3, 0, 0), &result) == true);
	CHECK(Geometry3D::segment_intersects_triangle(Vector3(1, 1, 1), Vector3(10, -1, -1), Vector3(-3, 0, 0), Vector3(0, 3, 0), Vector3(3, 0, 0), &result) == false);
}

TEST_CASE("[Geometry3D] Triangle and Box Overlap") {
	constexpr Vector3 good_triangle[3] = { Vector3(3, 2, 3), Vector3(2, 2, 1), Vector3(2, 1, 1) };
	CHECK(Geometry3D::triangle_box_overlap(Vector3(0, 0, 0), Vector3(5, 5, 5), good_triangle) == true);
	constexpr Vector3 bad_triangle[3] = { Vector3(100, 100, 100), Vector3(-100, -100, -100), Vector3(10, 10, 10) };
	CHECK(Geometry3D::triangle_box_overlap(Vector3(1000, 1000, 1000), Vector3(1, 1, 1), bad_triangle) == false);
}

TEST_CASE("[Geometry3D] Triangle and Sphere Intersect") {
	constexpr Vector3 triangle_a = Vector3(3, 0, 0);
	constexpr Vector3 triangle_b = Vector3(-3, 0, 0);
	constexpr Vector3 triangle_c = Vector3(0, 3, 0);
	Vector3 triangle_contact, sphere_contact;
	CHECK(Geometry3D::triangle_sphere_intersection_test(triangle_a, triangle_b, triangle_c, Vector3(0, -1, 0), Vector3(0, 0, 0), 5, triangle_contact, sphere_contact) == true);
	CHECK(Geometry3D::triangle_sphere_intersection_test(triangle_a, triangle_b, triangle_c, Vector3(0, 1, 0), Vector3(0, 0, 0), 5, triangle_contact, sphere_contact) == true);
	CHECK(Geometry3D::triangle_sphere_intersection_test(triangle_a, triangle_b, triangle_c, Vector3(0, 1, 0), Vector3(20, 0, 0), 5, triangle_contact, sphere_contact) == false);
}

} // namespace TestGeometry3D

// === Merged from tests/core/math/test_math_funcs.cpp ===
/*  test_math_funcs.cpp                                                   */
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

#include "core/types.h"
#include "tests/test_macros.h"



namespace TestMathFuncs {

TEST_CASE("[Math] C++ macros") {
	static_assert(MIN(-2, 2) == -2);
	static_assert(MIN(600, 2) == 2);

	static_assert(MAX(-2, 2) == 2);
	static_assert(MAX(600, 2) == 600);

	static_assert(CLAMP(600, -2, 2) == 2);
	static_assert(CLAMP(620, 600, 650) == 620);
	// `max` is lower than `min`.
	static_assert(CLAMP(620, 600, 50) == 50);

	static_assert(Math::abs(-5) == 5);
	static_assert(Math::abs(0) == 0);
	static_assert(Math::abs(5) == 5);

	static_assert(SIGN(-5) == -1.0);
	static_assert(SIGN(0) == 0.0);
	static_assert(SIGN(5) == 1.0);
	// Check that SIGN(Math::NaN) returns 0.0.
	// FIXME: `static_assert` fails on some compilers, substitute with normal `CHECK` instead.
	CHECK(SIGN(Math::NaN) == 0.0);
}

TEST_CASE("[Math] Power of two functions") {
	static_assert(Math::next_power_of_2((uint32_t)0) == 0);
	static_assert(Math::next_power_of_2((uint32_t)1) == 1);
	static_assert(Math::next_power_of_2((uint32_t)16) == 16);
	static_assert(Math::next_power_of_2((uint32_t)17) == 32);
	static_assert(Math::next_power_of_2((uint32_t)65535) == 65536);

	static_assert(Math::previous_power_of_2((uint32_t)0) == 0);
	static_assert(Math::previous_power_of_2((uint32_t)1) == 1);
	static_assert(Math::previous_power_of_2((uint32_t)16) == 16);
	static_assert(Math::previous_power_of_2((uint32_t)17) == 16);
	static_assert(Math::previous_power_of_2((uint32_t)65535) == 32768);

	static_assert(Math::closest_power_of_2((uint32_t)0) == 0);
	static_assert(Math::closest_power_of_2((uint32_t)1) == 1);
	static_assert(Math::closest_power_of_2((uint32_t)16) == 16);
	static_assert(Math::closest_power_of_2((uint32_t)17) == 16);
	static_assert(Math::closest_power_of_2((uint32_t)65535) == 65536);

	static_assert(Math::get_shift_from_power_of_2((uint32_t)0) == -1);
	static_assert(Math::get_shift_from_power_of_2((uint32_t)1) == 0);
	static_assert(Math::get_shift_from_power_of_2((uint32_t)16) == 4);
	static_assert(Math::get_shift_from_power_of_2((uint32_t)17) == -1);
	static_assert(Math::get_shift_from_power_of_2((uint32_t)65535) == -1);

	static_assert(Math::nearest_shift((uint32_t)0) == 0);
	static_assert(Math::nearest_shift((uint32_t)1) == 1);
	static_assert(Math::nearest_shift((uint32_t)16) == 5);
	static_assert(Math::nearest_shift((uint32_t)17) == 5);
	static_assert(Math::nearest_shift((uint32_t)65535) == 16);
}

TEST_CASE_TEMPLATE("[Math] abs", T, int, float, double) {
	static_assert(Math::abs((T)-1) == (T)1);
	static_assert(Math::abs((T)0) == (T)0);
	static_assert(Math::abs((T)1) == (T)1);
	static_assert(Math::abs((T)0.1) == (T)0.1);
}

TEST_CASE_TEMPLATE("[Math] round/floor/ceil", T, float, double) {
	CHECK(Math::round((T)1.5) == (T)2.0);
	CHECK(Math::round((T)1.6) == (T)2.0);
	CHECK(Math::round((T)-1.5) == (T)-2.0);
	CHECK(Math::round((T)-1.1) == (T)-1.0);

	CHECK(Math::floor((T)1.5) == (T)1.0);
	CHECK(Math::floor((T)-1.5) == (T)-2.0);

	CHECK(Math::ceil((T)1.5) == (T)2.0);
	CHECK(Math::ceil((T)-1.9) == (T)-1.0);
}

TEST_CASE_TEMPLATE("[Math] integer division round up unsigned", T, uint32_t, uint64_t) {
	static_assert(Math::division_round_up((T)0, (T)64) == 0);
	static_assert(Math::division_round_up((T)1, (T)64) == 1);
	static_assert(Math::division_round_up((T)63, (T)64) == 1);
	static_assert(Math::division_round_up((T)64, (T)64) == 1);
	static_assert(Math::division_round_up((T)65, (T)64) == 2);
	static_assert(Math::division_round_up((T)65, (T)1) == 65);
}

TEST_CASE_TEMPLATE("[Math] integer division round up signed", T, int32_t, int64_t) {
	static_assert(Math::division_round_up((T)0, (T)64) == 0);
	static_assert(Math::division_round_up((T)1, (T)64) == 1);
	static_assert(Math::division_round_up((T)63, (T)64) == 1);
	static_assert(Math::division_round_up((T)64, (T)64) == 1);
	static_assert(Math::division_round_up((T)65, (T)64) == 2);
	static_assert(Math::division_round_up((T)65, (T)1) == 65);
	static_assert(Math::division_round_up((T)-1, (T)64) == 0);
	static_assert(Math::division_round_up((T)-1, (T)-1) == 1);
	static_assert(Math::division_round_up((T)-1, (T)1) == -1);
	static_assert(Math::division_round_up((T)-1, (T)-2) == 1);
	static_assert(Math::division_round_up((T)-4, (T)-2) == 2);
}

TEST_CASE_TEMPLATE("[Math] sin/cos/tan", T, float, double) {
	CHECK(Math::sin((T)-0.1) == doctest::Approx((T)-0.0998334166));
	CHECK(Math::sin((T)0.1) == doctest::Approx((T)0.0998334166));
	CHECK(Math::sin((T)0.5) == doctest::Approx((T)0.4794255386));
	CHECK(Math::sin((T)1.0) == doctest::Approx((T)0.8414709848));
	CHECK(Math::sin((T)1.5) == doctest::Approx((T)0.9974949866));
	CHECK(Math::sin((T)450.0) == doctest::Approx((T)-0.683283725));

	CHECK(Math::cos((T)-0.1) == doctest::Approx((T)0.99500416530));
	CHECK(Math::cos((T)0.1) == doctest::Approx((T)0.9950041653));
	CHECK(Math::cos((T)0.5) == doctest::Approx((T)0.8775825619));
	CHECK(Math::cos((T)1.0) == doctest::Approx((T)0.5403023059));
	CHECK(Math::cos((T)1.5) == doctest::Approx((T)0.0707372017));
	CHECK(Math::cos((T)450.0) == doctest::Approx((T)-0.7301529642));

	CHECK(Math::tan((T)-0.1) == doctest::Approx((T)-0.1003346721));
	CHECK(Math::tan((T)0.1) == doctest::Approx((T)0.1003346721));
	CHECK(Math::tan((T)0.5) == doctest::Approx((T)0.5463024898));
	CHECK(Math::tan((T)1.0) == doctest::Approx((T)1.5574077247));
	CHECK(Math::tan((T)1.5) == doctest::Approx((T)14.1014199472));
	CHECK(Math::tan((T)450.0) == doctest::Approx((T)0.9358090134));
}

TEST_CASE_TEMPLATE("[Math] sinh/cosh/tanh", T, float, double) {
	CHECK(Math::sinh((T)-0.1) == doctest::Approx((T)-0.10016675));
	CHECK(Math::sinh((T)0.1) == doctest::Approx((T)0.10016675));
	CHECK(Math::sinh((T)0.5) == doctest::Approx((T)0.5210953055));
	CHECK(Math::sinh((T)1.0) == doctest::Approx((T)1.1752011936));
	CHECK(Math::sinh((T)1.5) == doctest::Approx((T)2.1292794551));

	CHECK(Math::cosh((T)-0.1) == doctest::Approx((T)1.0050041681));
	CHECK(Math::cosh((T)0.1) == doctest::Approx((T)1.0050041681));
	CHECK(Math::cosh((T)0.5) == doctest::Approx((T)1.1276259652));
	CHECK(Math::cosh((T)1.0) == doctest::Approx((T)1.5430806348));
	CHECK(Math::cosh((T)1.5) == doctest::Approx((T)2.3524096152));

	CHECK(Math::tanh((T)-0.1) == doctest::Approx((T)-0.0996679946));
	CHECK(Math::tanh((T)0.1) == doctest::Approx((T)0.0996679946));
	CHECK(Math::tanh((T)0.5) == doctest::Approx((T)0.4621171573));
	CHECK(Math::tanh((T)1.0) == doctest::Approx((T)0.761594156));
	CHECK(Math::tanh((T)1.5) == doctest::Approx((T)0.9051482536));
	CHECK(Math::tanh((T)450.0) == doctest::Approx((T)1.0));
}

TEST_CASE_TEMPLATE("[Math] asin/acos/atan", T, float, double) {
	CHECK(Math::asin((T)-0.1) == doctest::Approx((T)-0.1001674212));
	CHECK(Math::asin((T)0.1) == doctest::Approx((T)0.1001674212));
	CHECK(Math::asin((T)0.5) == doctest::Approx((T)0.5235987756));
	CHECK(Math::asin((T)1.0) == doctest::Approx((T)1.5707963268));
	CHECK(Math::asin((T)2.0) == doctest::Approx((T)1.5707963268));
	CHECK(Math::asin((T)-2.0) == doctest::Approx((T)-1.5707963268));

	CHECK(Math::acos((T)-0.1) == doctest::Approx((T)1.670963748));
	CHECK(Math::acos((T)0.1) == doctest::Approx((T)1.4706289056));
	CHECK(Math::acos((T)0.5) == doctest::Approx((T)1.0471975512));
	CHECK(Math::acos((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::acos((T)2.0) == doctest::Approx((T)0.0));
	CHECK(Math::acos((T)-2.0) == doctest::Approx((T)Math::PI));

	CHECK(Math::atan((T)-0.1) == doctest::Approx((T)-0.0996686525));
	CHECK(Math::atan((T)0.1) == doctest::Approx((T)0.0996686525));
	CHECK(Math::atan((T)0.5) == doctest::Approx((T)0.463647609));
	CHECK(Math::atan((T)1.0) == doctest::Approx((T)0.7853981634));
	CHECK(Math::atan((T)1.5) == doctest::Approx((T)0.9827937232));
	CHECK(Math::atan((T)450.0) == doctest::Approx((T)1.5685741082));
}

TEST_CASE_TEMPLATE("[Math] asinh/acosh/atanh", T, float, double) {
	CHECK(Math::asinh((T)-2.0) == doctest::Approx((T)-1.4436354751));
	CHECK(Math::asinh((T)-0.1) == doctest::Approx((T)-0.0998340788));
	CHECK(Math::asinh((T)0.1) == doctest::Approx((T)0.0998340788));
	CHECK(Math::asinh((T)0.5) == doctest::Approx((T)0.4812118250));
	CHECK(Math::asinh((T)1.0) == doctest::Approx((T)0.8813735870));
	CHECK(Math::asinh((T)2.0) == doctest::Approx((T)1.4436354751));

	CHECK(Math::acosh((T)-2.0) == doctest::Approx((T)0.0));
	CHECK(Math::acosh((T)-0.1) == doctest::Approx((T)0.0));
	CHECK(Math::acosh((T)0.1) == doctest::Approx((T)0.0));
	CHECK(Math::acosh((T)0.5) == doctest::Approx((T)0.0));
	CHECK(Math::acosh((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::acosh((T)2.0) == doctest::Approx((T)1.3169578969));
	CHECK(Math::acosh((T)450.0) == doctest::Approx((T)6.8023935287));

	CHECK(Math::is_inf(Math::atanh((T)-2.0)));
	CHECK(Math::atanh((T)-2.0) < (T)0.0);
	CHECK(Math::is_inf(Math::atanh((T)-1.0)));
	CHECK(Math::atanh((T)-1.0) < (T)0.0);
	CHECK(Math::atanh((T)-0.1) == doctest::Approx((T)-0.1003353477));
	CHECK(Math::atanh((T)0.1) == doctest::Approx((T)0.1003353477));
	CHECK(Math::atanh((T)0.5) == doctest::Approx((T)0.5493061443));
	CHECK(Math::is_inf(Math::atanh((T)1.0)));
	CHECK(Math::atanh((T)1.0) > (T)0.0);
	CHECK(Math::is_inf(Math::atanh((T)1.5)));
	CHECK(Math::atanh((T)1.5) > (T)0.0);
	CHECK(Math::is_inf(Math::atanh((T)450.0)));
	CHECK(Math::atanh((T)450.0) > (T)0.0);
}

TEST_CASE_TEMPLATE("[Math] sinc/sincn/atan2", T, float, double) {
	CHECK(Math::sinc((T)-0.1) == doctest::Approx((T)0.9983341665));
	CHECK(Math::sinc((T)0.1) == doctest::Approx((T)0.9983341665));
	CHECK(Math::sinc((T)0.5) == doctest::Approx((T)0.9588510772));
	CHECK(Math::sinc((T)1.0) == doctest::Approx((T)0.8414709848));
	CHECK(Math::sinc((T)1.5) == doctest::Approx((T)0.6649966577));
	CHECK(Math::sinc((T)450.0) == doctest::Approx((T)-0.0015184083));

	CHECK(Math::sincn((T)-0.1) == doctest::Approx((T)0.9836316431));
	CHECK(Math::sincn((T)0.1) == doctest::Approx((T)0.9836316431));
	CHECK(Math::sincn((T)0.5) == doctest::Approx((T)0.6366197724));
	CHECK(Math::sincn((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::sincn((T)1.5) == doctest::Approx((T)-0.2122065908));
	CHECK(Math::sincn((T)450.0) == doctest::Approx((T)0.0));

	CHECK(Math::atan2((T)-0.1, (T)0.5) == doctest::Approx((T)-0.1973955598));
	CHECK(Math::atan2((T)0.1, (T)-0.5) == doctest::Approx((T)2.9441970937));
	CHECK(Math::atan2((T)0.5, (T)1.5) == doctest::Approx((T)0.3217505544));
	CHECK(Math::atan2((T)1.0, (T)2.5) == doctest::Approx((T)0.3805063771));
	CHECK(Math::atan2((T)1.5, (T)1.0) == doctest::Approx((T)0.9827937232));
	CHECK(Math::atan2((T)450.0, (T)1.0) == doctest::Approx((T)1.5685741082));
}

TEST_CASE_TEMPLATE("[Math] pow/log/log2/exp/sqrt", T, float, double) {
	CHECK(Math::pow((T)-0.1, (T)2.0) == doctest::Approx((T)0.01));
	CHECK(Math::pow((T)0.1, (T)2.5) == doctest::Approx((T)0.0031622777));
	CHECK(Math::pow((T)0.5, (T)0.5) == doctest::Approx((T)0.7071067812));
	CHECK(Math::pow((T)1.0, (T)1.0) == doctest::Approx((T)1.0));
	CHECK(Math::pow((T)1.5, (T)-1.0) == doctest::Approx((T)0.6666666667));
	CHECK(Math::pow((T)450.0, (T)-2.0) == doctest::Approx((T)0.0000049383));
	CHECK(Math::pow((T)450.0, (T)0.0) == doctest::Approx((T)1.0));

	CHECK(Math::is_nan(Math::log((T)-0.1)));
	CHECK(Math::log((T)0.1) == doctest::Approx((T)-2.302585093));
	CHECK(Math::log((T)0.5) == doctest::Approx((T)-0.6931471806));
	CHECK(Math::log((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::log((T)1.5) == doctest::Approx((T)0.4054651081));
	CHECK(Math::log((T)450.0) == doctest::Approx((T)6.1092475828));

	CHECK(Math::is_nan(Math::log2((T)-0.1)));
	CHECK(Math::log2((T)0.1) == doctest::Approx((T)-3.3219280949));
	CHECK(Math::log2((T)0.5) == doctest::Approx((T)-1.0));
	CHECK(Math::log2((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::log2((T)1.5) == doctest::Approx((T)0.5849625007));
	CHECK(Math::log2((T)450.0) == doctest::Approx((T)8.8137811912));

	CHECK(Math::exp((T)-0.1) == doctest::Approx((T)0.904837418));
	CHECK(Math::exp((T)0.1) == doctest::Approx((T)1.1051709181));
	CHECK(Math::exp((T)0.5) == doctest::Approx((T)1.6487212707));
	CHECK(Math::exp((T)1.0) == doctest::Approx((T)2.7182818285));
	CHECK(Math::exp((T)1.5) == doctest::Approx((T)4.4816890703));

	CHECK(Math::is_nan(Math::sqrt((T)-0.1)));
	CHECK(Math::sqrt((T)0.1) == doctest::Approx((T)0.316228));
	CHECK(Math::sqrt((T)0.5) == doctest::Approx((T)0.707107));
	CHECK(Math::sqrt((T)1.0) == doctest::Approx((T)1.0));
	CHECK(Math::sqrt((T)1.5) == doctest::Approx((T)1.224745));
}

TEST_CASE_TEMPLATE("[Math] is_nan/is_inf/is_finite", T, float, double) {
	static_assert(!Math::is_nan((T)0.0));
	static_assert(!Math::is_nan((T)CMP_EPSILON));
	static_assert(!Math::is_nan((T)Math::INF));
	static_assert(!Math::is_nan((T)-Math::INF));
	static_assert(Math::is_nan((T)Math::NaN));

	static_assert(!Math::is_inf((T)0.0));
	static_assert(!Math::is_inf((T)CMP_EPSILON));
	static_assert(Math::is_inf((T)Math::INF));
	static_assert(Math::is_inf((T)-Math::INF));
	static_assert(!Math::is_inf((T)Math::NaN));

	static_assert(Math::is_finite((T)0.0));
	static_assert(Math::is_finite((T)CMP_EPSILON));
	static_assert(!Math::is_finite((T)Math::INF));
	static_assert(!Math::is_finite((T)-Math::INF));
	static_assert(!Math::is_finite((T)Math::NaN));
}

TEST_CASE_TEMPLATE("[Math] linear_to_db", T, float, double) {
	CHECK(Math::linear_to_db((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::linear_to_db((T)20.0) == doctest::Approx((T)26.0206));
	CHECK(Math::is_inf(Math::linear_to_db((T)0.0)));
	CHECK(Math::is_nan(Math::linear_to_db((T)-20.0)));
}

TEST_CASE_TEMPLATE("[Math] db_to_linear", T, float, double) {
	CHECK(Math::db_to_linear((T)0.0) == doctest::Approx((T)1.0));
	CHECK(Math::db_to_linear((T)1.0) == doctest::Approx((T)1.122018));
	CHECK(Math::db_to_linear((T)20.0) == doctest::Approx((T)10.0));
	CHECK(Math::db_to_linear((T)-20.0) == doctest::Approx((T)0.1));
}

TEST_CASE_TEMPLATE("[Math] step_decimals", T, float, double) {
	CHECK(Math::step_decimals((T)-0.5) == 1);
	CHECK(Math::step_decimals((T)0) == 0);
	CHECK(Math::step_decimals((T)1) == 0);
	CHECK(Math::step_decimals((T)0.1) == 1);
	CHECK(Math::step_decimals((T)0.01) == 2);
	CHECK(Math::step_decimals((T)0.001) == 3);
	CHECK(Math::step_decimals((T)0.0001) == 4);
	CHECK(Math::step_decimals((T)0.00001) == 5);
	CHECK(Math::step_decimals((T)0.000001) == 6);
	CHECK(Math::step_decimals((T)0.0000001) == 7);
	CHECK(Math::step_decimals((T)0.00000001) == 8);
	CHECK(Math::step_decimals((T)0.000000001) == 9);
	// Too many decimals to handle.
	CHECK(Math::step_decimals((T)0.0000000001) == 0);
}

TEST_CASE_TEMPLATE("[Math] range_step_decimals", T, float, double) {
	CHECK(Math::range_step_decimals((T)0.000000001) == 9);
	// Too many decimals to handle.
	CHECK(Math::range_step_decimals((T)0.0000000001) == 0);
	// Should be treated as a step of 0 for use by the editor.
	CHECK(Math::range_step_decimals((T)0.0) == 16);
	CHECK(Math::range_step_decimals((T)-0.5) == 16);
}

TEST_CASE_TEMPLATE("[Math] is_equal_approx", T, float, double) {
	static_assert(Math::is_equal_approx((T)2.0, (T)2.0));
	static_assert(Math::is_equal_approx((T)2.0, (T)2.0 + (T)CMP_EPSILON));
	static_assert(!Math::is_equal_approx((T)2.0, (T)2.0 + (T)CMP_EPSILON + (T)CMP_EPSILON));

	static_assert(Math::is_equal_approx((T)-2.0, (T)-2.0));
	static_assert(Math::is_equal_approx((T)-2.0, (T)-2.0 - (T)CMP_EPSILON));
	static_assert(!Math::is_equal_approx((T)-2.0, (T)-2.0 - (T)CMP_EPSILON - (T)CMP_EPSILON));
}

TEST_CASE_TEMPLATE("[Math] is_zero_approx", T, float, double) {
	static_assert(Math::is_zero_approx((T)0.0));
	static_assert(Math::is_zero_approx((T)0.0 + ((T)CMP_EPSILON / 2)));
	static_assert(!Math::is_zero_approx((T)0.0 + (T)CMP_EPSILON));

	static_assert(Math::is_zero_approx((T)-0.0));
	static_assert(Math::is_zero_approx((T)-0.0 - ((T)CMP_EPSILON / 2)));
	static_assert(!Math::is_zero_approx((T)-0.0 - (T)-CMP_EPSILON));
}

TEST_CASE_TEMPLATE("[Math] lerp", T, float, double) {
	static_assert(Math::is_equal_approx(Math::lerp((T)2.0, (T)5.0, (T)-0.1), (T)1.7));
	static_assert(Math::is_equal_approx(Math::lerp((T)2.0, (T)5.0, (T)0.0), (T)2.0));
	static_assert(Math::is_equal_approx(Math::lerp((T)2.0, (T)5.0, (T)0.1), (T)2.3));
	static_assert(Math::is_equal_approx(Math::lerp((T)2.0, (T)5.0, (T)1.0), (T)5.0));
	static_assert(Math::is_equal_approx(Math::lerp((T)2.0, (T)5.0, (T)2.0), (T)8.0));

	static_assert(Math::is_equal_approx(Math::lerp((T)-2.0, (T)-5.0, (T)-0.1), (T)-1.7));
	static_assert(Math::is_equal_approx(Math::lerp((T)-2.0, (T)-5.0, (T)0.0), (T)-2.0));
	static_assert(Math::is_equal_approx(Math::lerp((T)-2.0, (T)-5.0, (T)0.1), (T)-2.3));
	static_assert(Math::is_equal_approx(Math::lerp((T)-2.0, (T)-5.0, (T)1.0), (T)-5.0));
	static_assert(Math::is_equal_approx(Math::lerp((T)-2.0, (T)-5.0, (T)2.0), (T)-8.0));
}

TEST_CASE_TEMPLATE("[Math] inverse_lerp", T, float, double) {
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)2.0, (T)5.0, (T)1.7), (T)-0.1));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)2.0, (T)5.0, (T)2.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)2.0, (T)5.0, (T)2.3), (T)0.1));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)2.0, (T)5.0, (T)5.0), (T)1.0));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)2.0, (T)5.0, (T)8.0), (T)2.0));

	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)-2.0, (T)-5.0, (T)-1.7), (T)-0.1));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)-2.0, (T)-5.0, (T)-2.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)-2.0, (T)-5.0, (T)-2.3), (T)0.1));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)-2.0, (T)-5.0, (T)-5.0), (T)1.0));
	static_assert(Math::is_equal_approx(Math::inverse_lerp((T)-2.0, (T)-5.0, (T)-8.0), (T)2.0));
}

TEST_CASE_TEMPLATE("[Math] remap", T, float, double) {
	static_assert(Math::is_equal_approx(Math::remap((T)50.0, (T)100.0, (T)200.0, (T)0.0, (T)1000.0), (T)-500.0));
	static_assert(Math::is_equal_approx(Math::remap((T)100.0, (T)100.0, (T)200.0, (T)0.0, (T)1000.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::remap((T)200.0, (T)100.0, (T)200.0, (T)0.0, (T)1000.0), (T)1000.0));
	static_assert(Math::is_equal_approx(Math::remap((T)250.0, (T)100.0, (T)200.0, (T)0.0, (T)1000.0), (T)1500.0));

	static_assert(Math::is_equal_approx(Math::remap((T)-50.0, (T)-100.0, (T)-200.0, (T)0.0, (T)1000.0), (T)-500.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-100.0, (T)-100.0, (T)-200.0, (T)0.0, (T)1000.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-200.0, (T)-100.0, (T)-200.0, (T)0.0, (T)1000.0), (T)1000.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-250.0, (T)-100.0, (T)-200.0, (T)0.0, (T)1000.0), (T)1500.0));

	static_assert(Math::is_equal_approx(Math::remap((T)-50.0, (T)-100.0, (T)-200.0, (T)0.0, (T)-1000.0), (T)500.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-100.0, (T)-100.0, (T)-200.0, (T)0.0, (T)-1000.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-200.0, (T)-100.0, (T)-200.0, (T)0.0, (T)-1000.0), (T)-1000.0));
	static_assert(Math::is_equal_approx(Math::remap((T)-250.0, (T)-100.0, (T)-200.0, (T)0.0, (T)-1000.0), (T)-1500.0));

	// Note: undefined behavior can happen when `p_istart == p_istop`. We don't bother testing this as it will
	// vary between hardware and compilers properly implementing IEEE 754.
}

TEST_CASE_TEMPLATE("[Math] angle_difference", T, float, double) {
	// Loops around, should return 0.0.
	CHECK(Math::angle_difference((T)0.0, (T)Math::TAU) == doctest::Approx((T)0.0));
	CHECK(Math::angle_difference((T)Math::PI, (T)-Math::PI) == doctest::Approx((T)0.0));
	CHECK(Math::angle_difference((T)0.0, (T)Math::TAU * (T)4.0) == doctest::Approx((T)0.0));

	// Rotation is clockwise, so it should return -PI.
	CHECK(Math::angle_difference((T)0.0, (T)Math::PI) == doctest::Approx((T)-Math::PI));
	CHECK(Math::angle_difference((T)0.0, (T)-Math::PI) == doctest::Approx((T)Math::PI));
	CHECK(Math::angle_difference((T)Math::PI, (T)0.0) == doctest::Approx((T)Math::PI));
	CHECK(Math::angle_difference((T)-Math::PI, (T)0.0) == doctest::Approx((T)-Math::PI));

	CHECK(Math::angle_difference((T)0.0, (T)3.0) == doctest::Approx((T)3.0));
	CHECK(Math::angle_difference((T)1.0, (T)-2.0) == doctest::Approx((T)-3.0));
	CHECK(Math::angle_difference((T)-1.0, (T)2.0) == doctest::Approx((T)3.0));
	CHECK(Math::angle_difference((T)-2.0, (T)-4.5) == doctest::Approx((T)-2.5));
	CHECK(Math::angle_difference((T)100.0, (T)102.5) == doctest::Approx((T)2.5));
}

TEST_CASE_TEMPLATE("[Math] lerp_angle", T, float, double) {
	// Counter-clockwise rotation.
	CHECK(Math::lerp_angle((T)0.24 * Math::TAU, 0.75 * Math::TAU, 0.5) == doctest::Approx((T)-0.005 * Math::TAU));
	// Counter-clockwise rotation.
	CHECK(Math::lerp_angle((T)0.25 * Math::TAU, 0.75 * Math::TAU, 0.5) == doctest::Approx((T)0.0));
	// Clockwise rotation.
	CHECK(Math::lerp_angle((T)0.26 * Math::TAU, 0.75 * Math::TAU, 0.5) == doctest::Approx((T)0.505 * Math::TAU));

	CHECK(Math::lerp_angle((T)-0.25 * Math::TAU, 1.25 * Math::TAU, 0.5) == doctest::Approx((T)-0.5 * Math::TAU));
	CHECK(Math::lerp_angle((T)0.72 * Math::TAU, 1.44 * Math::TAU, 0.96) == doctest::Approx((T)0.4512 * Math::TAU));
	CHECK(Math::lerp_angle((T)0.72 * Math::TAU, 1.44 * Math::TAU, 1.04) == doctest::Approx((T)0.4288 * Math::TAU));

	// Initial and final angles are effectively identical, so the value returned
	// should always be the same regardless of the `weight` parameter.
	CHECK(Math::lerp_angle((T)-4 * Math::TAU, 4 * Math::TAU, -1.0) == doctest::Approx((T)-4.0 * Math::TAU));
	CHECK(Math::lerp_angle((T)-4 * Math::TAU, 4 * Math::TAU, 0.0) == doctest::Approx((T)-4.0 * Math::TAU));
	CHECK(Math::lerp_angle((T)-4 * Math::TAU, 4 * Math::TAU, 0.5) == doctest::Approx((T)-4.0 * Math::TAU));
	CHECK(Math::lerp_angle((T)-4 * Math::TAU, 4 * Math::TAU, 1.0) == doctest::Approx((T)-4.0 * Math::TAU));
	CHECK(Math::lerp_angle((T)-4 * Math::TAU, 4 * Math::TAU, 500.0) == doctest::Approx((T)-4.0 * Math::TAU));
}

TEST_CASE_TEMPLATE("[Math] move_toward", T, float, double) {
	CHECK(Math::move_toward(2.0, 5.0, -1.0) == doctest::Approx((T)1.0));
	CHECK(Math::move_toward(2.0, 5.0, 2.5) == doctest::Approx((T)4.5));
	CHECK(Math::move_toward(2.0, 5.0, 4.0) == doctest::Approx((T)5.0));
	CHECK(Math::move_toward(-2.0, -5.0, -1.0) == doctest::Approx((T)-1.0));
	CHECK(Math::move_toward(-2.0, -5.0, 2.5) == doctest::Approx((T)-4.5));
	CHECK(Math::move_toward(-2.0, -5.0, 4.0) == doctest::Approx((T)-5.0));
}

TEST_CASE_TEMPLATE("[Math] rotate_toward", T, float, double) {
	// Rotate toward.
	CHECK(Math::rotate_toward((T)0.0, (T)Math::PI * (T)0.75, (T)1.5) == doctest::Approx((T)1.5));
	CHECK(Math::rotate_toward((T)-2.0, (T)1.0, (T)2.5) == doctest::Approx((T)0.5));
	CHECK(Math::rotate_toward((T)-2.0, (T)Math::PI, (T)Math::PI) == doctest::Approx((T)-Math::PI));
	CHECK(Math::rotate_toward((T)1.0, (T)Math::PI, (T)20.0) == doctest::Approx((T)Math::PI));

	// Rotate away.
	CHECK(Math::rotate_toward((T)0.0, (T)0.0, (T)-1.5) == doctest::Approx((T)-1.5));
	CHECK(Math::rotate_toward((T)0.0, (T)0.0, (T)-Math::PI) == doctest::Approx((T)-Math::PI));
	CHECK(Math::rotate_toward((T)3.0, (T)Math::PI, (T)-Math::PI) == doctest::Approx((T)0.0));
	CHECK(Math::rotate_toward((T)2.0, (T)Math::PI, (T)-1.5) == doctest::Approx((T)0.5));
	CHECK(Math::rotate_toward((T)1.0, (T)2.0, (T)-0.5) == doctest::Approx((T)0.5));
	CHECK(Math::rotate_toward((T)2.5, (T)2.0, (T)-0.5) == doctest::Approx((T)3.0));
	CHECK(Math::rotate_toward((T)-1.0, (T)1.0, (T)-1.0) == doctest::Approx((T)-2.0));
}

TEST_CASE_TEMPLATE("[Math] smoothstep", T, float, double) {
	CHECK(Math::smoothstep((T)0.0, (T)2.0, (T)-5.0) == doctest::Approx((T)0.0));
	CHECK(Math::smoothstep((T)0.0, (T)2.0, (T)0.5) == doctest::Approx((T)0.15625));
	CHECK(Math::smoothstep((T)0.0, (T)2.0, (T)1.0) == doctest::Approx((T)0.5));
	CHECK(Math::smoothstep((T)0.0, (T)2.0, (T)2.0) == doctest::Approx((T)1.0));
}

TEST_CASE("[Math] ease") {
	CHECK(Math::ease(0.1, 1.0) == doctest::Approx(0.1));
	CHECK(Math::ease(0.1, 2.0) == doctest::Approx(0.01));
	CHECK(Math::ease(0.1, 0.5) == doctest::Approx(0.19));
	CHECK(Math::ease(0.1, 0.0) == doctest::Approx(0));
	CHECK(Math::ease(0.1, -0.5) == doctest::Approx(0.2236067977));
	CHECK(Math::ease(0.1, -1.0) == doctest::Approx(0.1));
	CHECK(Math::ease(0.1, -2.0) == doctest::Approx(0.02));

	CHECK(Math::ease(-1.0, 1.0) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, 2.0) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, 0.5) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, 0.0) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, -0.5) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, -1.0) == doctest::Approx(0));
	CHECK(Math::ease(-1.0, -2.0) == doctest::Approx(0));
}

TEST_CASE("[Math] snapped") {
	CHECK(Math::snapped(0.5, 0.04) == doctest::Approx(0.52));
	CHECK(Math::snapped(-0.5, 0.04) == doctest::Approx(-0.48));
	CHECK(Math::snapped(0.0, 0.04) == doctest::Approx(0));
	CHECK(Math::snapped(128'000.025, 0.04) == doctest::Approx(128'000.04));

	CHECK(Math::snapped(0.5, 400) == doctest::Approx(0));
	CHECK(Math::snapped(-0.5, 400) == doctest::Approx(0));
	CHECK(Math::snapped(0.0, 400) == doctest::Approx(0));
	CHECK(Math::snapped(128'000.025, 400) == doctest::Approx(128'000.0));

	CHECK(Math::snapped(0.5, 0.0) == doctest::Approx(0.5));
	CHECK(Math::snapped(-0.5, 0.0) == doctest::Approx(-0.5));
	CHECK(Math::snapped(0.0, 0.0) == doctest::Approx(0.0));
	CHECK(Math::snapped(128'000.025, 0.0) == doctest::Approx(128'000.0));

	CHECK(Math::snapped(0.5, -1.0) == doctest::Approx(0));
	CHECK(Math::snapped(-0.5, -1.0) == doctest::Approx(-1.0));
	CHECK(Math::snapped(0.0, -1.0) == doctest::Approx(0));
	CHECK(Math::snapped(128'000.025, -1.0) == doctest::Approx(128'000.0));
}

TEST_CASE("[Math] larger_prime") {
	CHECK(Math::larger_prime(0) == 5);
	CHECK(Math::larger_prime(1) == 5);
	CHECK(Math::larger_prime(2) == 5);
	CHECK(Math::larger_prime(5) == 13);
	CHECK(Math::larger_prime(500) == 769);
	CHECK(Math::larger_prime(1'000'000) == 1'572'869);
	CHECK(Math::larger_prime(1'000'000'000) == 1'610'612'741);

	// The next prime is larger than `INT32_MAX` and is not present in the built-in prime table.
	ERR_PRINT_OFF;
	CHECK(Math::larger_prime(2'000'000'000) == 0);
	ERR_PRINT_ON;
}

TEST_CASE_TEMPLATE("[Math] fmod", T, float, double) {
	CHECK(Math::fmod((T)-2.0, (T)0.3) == doctest::Approx((T)-0.2));
	CHECK(Math::fmod((T)0.0, (T)0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fmod((T)2.0, (T)0.3) == doctest::Approx((T)0.2));

	CHECK(Math::fmod((T)-2.0, (T)-0.3) == doctest::Approx((T)-0.2));
	CHECK(Math::fmod((T)0.0, (T)-0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fmod((T)2.0, (T)-0.3) == doctest::Approx((T)0.2));
}

TEST_CASE_TEMPLATE("[Math] fposmod", T, float, double) {
	CHECK(Math::fposmod((T)-2.0, (T)0.3) == doctest::Approx((T)0.1));
	CHECK(Math::fposmod((T)0.0, (T)0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fposmod((T)2.0, (T)0.3) == doctest::Approx((T)0.2));

	CHECK(Math::fposmod((T)-2.0, (T)-0.3) == doctest::Approx((T)-0.2));
	CHECK(Math::fposmod((T)0.0, (T)-0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fposmod((T)2.0, (T)-0.3) == doctest::Approx((T)-0.1));
}

TEST_CASE_TEMPLATE("[Math] fposmodp", T, float, double) {
	CHECK(Math::fposmodp((T)-2.0, (T)0.3) == doctest::Approx((T)0.1));
	CHECK(Math::fposmodp((T)0.0, (T)0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fposmodp((T)2.0, (T)0.3) == doctest::Approx((T)0.2));

	CHECK(Math::fposmodp((T)-2.0, (T)-0.3) == doctest::Approx((T)-0.5));
	CHECK(Math::fposmodp((T)0.0, (T)-0.3) == doctest::Approx((T)0.0));
	CHECK(Math::fposmodp((T)2.0, (T)-0.3) == doctest::Approx((T)0.2));
}

TEST_CASE("[Math] posmod") {
	CHECK(Math::posmod(-20, 3) == 1);
	CHECK(Math::posmod(0, 3) == 0);
	CHECK(Math::posmod(20, 3) == 2);
	CHECK(Math::posmod(-20, -3) == -2);
	CHECK(Math::posmod(0, -3) == 0);
	CHECK(Math::posmod(20, -3) == -1);
}

TEST_CASE("[Math] wrapi") {
	static_assert(Math::wrapi(-30, -20, 160) == 150);
	static_assert(Math::wrapi(30, -20, 160) == 30);
	static_assert(Math::wrapi(300, -20, 160) == 120);
	static_assert(Math::wrapi(300'000'000'000, -20, 160) == 120);
}

TEST_CASE_TEMPLATE("[Math] wrapf", T, float, double) {
	CHECK(Math::wrapf((T)-30.0, (T)-20.0, (T)160.0) == doctest::Approx((T)150.0));
	CHECK(Math::wrapf((T)30.0, (T)-2.0, (T)160.0) == doctest::Approx((T)30.0));
	CHECK(Math::wrapf((T)300.0, (T)-20.0, (T)160.0) == doctest::Approx((T)120.0));

	CHECK(Math::wrapf(300'000'000'000.0, -20.0, 160.0) == doctest::Approx((T)120.0));
	// float's precision is too low for 300'000'000'000.0, so we reduce it by a factor of 1000.
	CHECK(Math::wrapf((float)15'000'000.0, (float)-20.0, (float)160.0) == doctest::Approx((T)60.0));
}

TEST_CASE_TEMPLATE("[Math] fract", T, float, double) {
	CHECK(Math::fract((T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::fract((T)77.8) == doctest::Approx((T)0.8));
	CHECK(Math::fract((T)-10.1) == doctest::Approx((T)0.9));
}

TEST_CASE_TEMPLATE("[Math] pingpong", T, float, double) {
	CHECK(Math::pingpong((T)0.0, (T)0.0) == doctest::Approx((T)0.0));
	CHECK(Math::pingpong((T)1.0, (T)1.0) == doctest::Approx((T)1.0));
	CHECK(Math::pingpong((T)0.5, (T)2.0) == doctest::Approx((T)0.5));
	CHECK(Math::pingpong((T)3.5, (T)2.0) == doctest::Approx((T)0.5));
	CHECK(Math::pingpong((T)11.5, (T)2.0) == doctest::Approx((T)0.5));
	CHECK(Math::pingpong((T)-2.5, (T)2.0) == doctest::Approx((T)1.5));
}

TEST_CASE_TEMPLATE("[Math] deg_to_rad/rad_to_deg", T, float, double) {
	static_assert(Math::is_equal_approx(Math::deg_to_rad((T)180.0), (T)Math::PI));
	static_assert(Math::is_equal_approx(Math::deg_to_rad((T)-27.0), (T)-0.471239));

	static_assert(Math::is_equal_approx(Math::rad_to_deg((T)Math::PI), (T)180.0));
	static_assert(Math::is_equal_approx(Math::rad_to_deg((T)-1.5), (T)-85.94366927));
}

TEST_CASE_TEMPLATE("[Math] cubic_interpolate", T, float, double) {
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.0), (T)0.2));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.25), (T)0.33125));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.5), (T)0.5));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.75), (T)0.66875));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)1.0), (T)0.8));

	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)20.2, (T)30.1, (T)-100.0, (T)32.0, (T)-50.0), (T)-6662732.3));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)20.2, (T)30.1, (T)-100.0, (T)32.0, (T)-5.0), (T)-9356.3));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)20.2, (T)30.1, (T)-100.0, (T)32.0, (T)0.0), (T)20.2));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)20.2, (T)30.1, (T)-100.0, (T)32.0, (T)1.0), (T)30.1));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate((T)20.2, (T)30.1, (T)-100.0, (T)32.0, (T)4.0), (T)1853.2));
}

TEST_CASE_TEMPLATE("[Math] cubic_interpolate_angle", T, float, double) {
	CHECK(Math::cubic_interpolate_angle((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.0) == doctest::Approx((T)Math::PI * (1.0 / 6.0)));
	CHECK(Math::cubic_interpolate_angle((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.25) == doctest::Approx((T)0.973566));
	CHECK(Math::cubic_interpolate_angle((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.5) == doctest::Approx((T)Math::PI / 2.0));
	CHECK(Math::cubic_interpolate_angle((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.75) == doctest::Approx((T)2.16803));
	CHECK(Math::cubic_interpolate_angle((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)1.0) == doctest::Approx((T)Math::PI * (5.0 / 6.0)));
}

TEST_CASE_TEMPLATE("[Math] cubic_interpolate_in_time", T, float, double) {
	static_assert(Math::is_equal_approx(Math::cubic_interpolate_in_time((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.0, (T)0.5, (T)0.0, (T)1.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate_in_time((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.25, (T)0.5, (T)0.0, (T)1.0), (T)0.1625));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate_in_time((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.5, (T)0.5, (T)0.0, (T)1.0), (T)0.4));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate_in_time((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)0.75, (T)0.5, (T)0.0, (T)1.0), (T)0.6375));
	static_assert(Math::is_equal_approx(Math::cubic_interpolate_in_time((T)0.2, (T)0.8, (T)0.0, (T)1.0, (T)1.0, (T)0.5, (T)0.0, (T)1.0), (T)0.8));
}

TEST_CASE_TEMPLATE("[Math] cubic_interpolate_angle_in_time", T, float, double) {
	CHECK(Math::cubic_interpolate_angle_in_time((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.0, (T)0.5, (T)0.0, (T)1.0) == doctest::Approx((T)0.0));
	CHECK(Math::cubic_interpolate_angle_in_time((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.25, (T)0.5, (T)0.0, (T)1.0) == doctest::Approx((T)0.494964));
	CHECK(Math::cubic_interpolate_angle_in_time((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.5, (T)0.5, (T)0.0, (T)1.0) == doctest::Approx((T)1.27627));
	CHECK(Math::cubic_interpolate_angle_in_time((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)0.75, (T)0.5, (T)0.0, (T)1.0) == doctest::Approx((T)2.07394));
	CHECK(Math::cubic_interpolate_angle_in_time((T)(Math::PI * (1.0 / 6.0)), (T)(Math::PI * (5.0 / 6.0)), (T)0.0, (T)Math::PI, (T)1.0, (T)0.5, (T)0.0, (T)1.0) == doctest::Approx((T)Math::PI * (5.0 / 6.0)));
}

TEST_CASE_TEMPLATE("[Math] bezier_interpolate", T, float, double) {
	static_assert(Math::is_equal_approx(Math::bezier_interpolate((T)0.0, (T)0.2, (T)0.8, (T)1.0, (T)0.0), (T)0.0));
	static_assert(Math::is_equal_approx(Math::bezier_interpolate((T)0.0, (T)0.2, (T)0.8, (T)1.0, (T)0.25), (T)0.2125));
	static_assert(Math::is_equal_approx(Math::bezier_interpolate((T)0.0, (T)0.2, (T)0.8, (T)1.0, (T)0.5), (T)0.5));
	static_assert(Math::is_equal_approx(Math::bezier_interpolate((T)0.0, (T)0.2, (T)0.8, (T)1.0, (T)0.75), (T)0.7875));
	static_assert(Math::is_equal_approx(Math::bezier_interpolate((T)0.0, (T)0.2, (T)0.8, (T)1.0, (T)1.0), (T)1.0));
}

} // namespace TestMathFuncs

// === Merged from tests/core/math/test_plane.cpp ===
/*  test_plane.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/math/plane.h"

namespace TestPlane {

TEST_CASE("[Plane] Constructor methods") {
	constexpr Plane plane = Plane(32, 22, 16, 3);
	constexpr Plane plane_vector = Plane(Vector3(32, 22, 16), 3);
	constexpr Plane plane_copy_plane = Plane(plane);

	static_assert(
			plane == plane_vector,
			"Planes created with same values but different methods should be equal.");

	static_assert(
			plane == plane_copy_plane,
			"Planes created with same values but different methods should be equal.");
}

TEST_CASE("[Plane] Basic getters") {
	constexpr Plane plane = Plane(32, 22, 16, 3);
	constexpr Plane plane_normalized = Plane(32.0 / 42, 22.0 / 42, 16.0 / 42, 3.0 / 42);

	CHECK_MESSAGE(
			plane.get_normal().is_equal_approx(Vector3(32, 22, 16)),
			"get_normal() should return the expected value.");

	CHECK_MESSAGE(
			plane.normalized().is_equal_approx(plane_normalized),
			"normalized() should return a copy of the normalized value.");
}

TEST_CASE("[Plane] Basic setters") {
	Plane plane = Plane(32, 22, 16, 3);
	plane.set_normal(Vector3(4, 2, 3));

	CHECK_MESSAGE(
			plane.is_equal_approx(Plane(4, 2, 3, 3)),
			"set_normal() should result in the expected plane.");

	plane = Plane(32, 22, 16, 3);
	plane.normalize();

	CHECK_MESSAGE(
			plane.is_equal_approx(Plane(32.0 / 42, 22.0 / 42, 16.0 / 42, 3.0 / 42)),
			"normalize() should result in the expected plane.");
}

TEST_CASE("[Plane] Plane-point operations") {
	constexpr Plane plane = Plane(32, 22, 16, 3);
	constexpr Plane y_facing_plane = Plane(0, 1, 0, 4);

	CHECK_MESSAGE(
			plane.get_center().is_equal_approx(Vector3(32 * 3, 22 * 3, 16 * 3)),
			"get_center() should return a vector pointing to the center of the plane.");

	CHECK_MESSAGE(
			y_facing_plane.is_point_over(Vector3(0, 5, 0)),
			"is_point_over() should return the expected result.");

	CHECK_MESSAGE(
			y_facing_plane.get_any_perpendicular_normal().is_equal_approx(Vector3(1, 0, 0)),
			"get_any_perpendicular_normal() should return the expected result.");

	// TODO distance_to()
}

TEST_CASE("[Plane] Has point") {
	constexpr Plane x_facing_plane = Plane(1, 0, 0, 0);
	constexpr Plane y_facing_plane = Plane(0, 1, 0, 0);
	constexpr Plane z_facing_plane = Plane(0, 0, 1, 0);

	constexpr Vector3 x_axis_point = Vector3(10, 0, 0);
	constexpr Vector3 y_axis_point = Vector3(0, 10, 0);
	constexpr Vector3 z_axis_point = Vector3(0, 0, 10);

	constexpr Plane x_facing_plane_with_d_offset = Plane(1, 0, 0, 1);
	constexpr Vector3 y_axis_point_with_d_offset = Vector3(1, 10, 0);

	CHECK_MESSAGE(
			x_facing_plane.has_point(y_axis_point),
			"has_point() with contained Vector3 should return the expected result.");
	CHECK_MESSAGE(
			x_facing_plane.has_point(z_axis_point),
			"has_point() with contained Vector3 should return the expected result.");

	CHECK_MESSAGE(
			y_facing_plane.has_point(x_axis_point),
			"has_point() with contained Vector3 should return the expected result.");
	CHECK_MESSAGE(
			y_facing_plane.has_point(z_axis_point),
			"has_point() with contained Vector3 should return the expected result.");

	CHECK_MESSAGE(
			z_facing_plane.has_point(y_axis_point),
			"has_point() with contained Vector3 should return the expected result.");
	CHECK_MESSAGE(
			z_facing_plane.has_point(x_axis_point),
			"has_point() with contained Vector3 should return the expected result.");

	CHECK_MESSAGE(
			x_facing_plane_with_d_offset.has_point(y_axis_point_with_d_offset),
			"has_point() with passed Vector3 should return the expected result.");
}

TEST_CASE("[Plane] Intersection") {
	constexpr Plane x_facing_plane = Plane(1, 0, 0, 1);
	constexpr Plane y_facing_plane = Plane(0, 1, 0, 2);
	constexpr Plane z_facing_plane = Plane(0, 0, 1, 3);

	Vector3 vec_out;

	CHECK_MESSAGE(
			x_facing_plane.intersect_3(y_facing_plane, z_facing_plane, &vec_out),
			"intersect_3() should return the expected result.");
	CHECK_MESSAGE(
			vec_out.is_equal_approx(Vector3(1, 2, 3)),
			"intersect_3() should modify vec_out to the expected result.");

	CHECK_MESSAGE(
			x_facing_plane.intersects_ray(Vector3(0, 1, 1), Vector3(2, 0, 0), &vec_out),
			"intersects_ray() should return the expected result.");
	CHECK_MESSAGE(
			vec_out.is_equal_approx(Vector3(1, 1, 1)),
			"intersects_ray() should modify vec_out to the expected result.");

	CHECK_MESSAGE(
			x_facing_plane.intersects_segment(Vector3(0, 1, 1), Vector3(2, 1, 1), &vec_out),
			"intersects_segment() should return the expected result.");
	CHECK_MESSAGE(
			vec_out.is_equal_approx(Vector3(1, 1, 1)),
			"intersects_segment() should modify vec_out to the expected result.");
}

TEST_CASE("[Plane] Finite number checks") {
	constexpr Vector3 x(0, 1, 2);
	constexpr Vector3 infinite_vec(Math::NaN, Math::NaN, Math::NaN);
	constexpr real_t y = 0;
	constexpr real_t infinite_y = Math::NaN;

	CHECK_MESSAGE(
			Plane(x, y).is_finite(),
			"Plane with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Plane(x, infinite_y).is_finite(),
			"Plane with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Plane(infinite_vec, y).is_finite(),
			"Plane with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Plane(infinite_vec, infinite_y).is_finite(),
			"Plane with two components infinite should not be finite.");
}

} // namespace TestPlane

// === Merged from tests/core/math/test_projection.cpp ===
/*  test_projection.cpp                                                   */
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

#include "tests/test_macros.h"



#include "core/types.h"
#include "core/math/plane.h"
#include "core/types.h"

namespace TestProjection {

TEST_CASE("[Projection] Construction") {
	Projection default_proj;

	CHECK(default_proj[0].is_equal_approx(Vector4(1, 0, 0, 0)));
	CHECK(default_proj[1].is_equal_approx(Vector4(0, 1, 0, 0)));
	CHECK(default_proj[2].is_equal_approx(Vector4(0, 0, 1, 0)));
	CHECK(default_proj[3].is_equal_approx(Vector4(0, 0, 0, 1)));

	Projection from_vec4(
			Vector4(1, 2, 3, 4),
			Vector4(5, 6, 7, 8),
			Vector4(9, 10, 11, 12),
			Vector4(13, 14, 15, 16));

	CHECK(from_vec4[0].is_equal_approx(Vector4(1, 2, 3, 4)));
	CHECK(from_vec4[1].is_equal_approx(Vector4(5, 6, 7, 8)));
	CHECK(from_vec4[2].is_equal_approx(Vector4(9, 10, 11, 12)));
	CHECK(from_vec4[3].is_equal_approx(Vector4(13, 14, 15, 16)));

	Transform3D transform(
			Basis(
					Vector3(1, 0, 0),
					Vector3(0, 2, 0),
					Vector3(0, 0, 3)),
			Vector3(4, 5, 6));

	Projection from_transform(transform);

	CHECK(from_transform[0].is_equal_approx(Vector4(1, 0, 0, 0)));
	CHECK(from_transform[1].is_equal_approx(Vector4(0, 2, 0, 0)));
	CHECK(from_transform[2].is_equal_approx(Vector4(0, 0, 3, 0)));
	CHECK(from_transform[3].is_equal_approx(Vector4(4, 5, 6, 1)));
}

TEST_CASE("[Projection] set_zero()") {
	Projection proj;
	proj.set_zero();

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			CHECK(proj.columns[i][j] == 0);
		}
	}
}

TEST_CASE("[Projection] set_identity()") {
	Projection proj;
	proj.set_identity();

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			CHECK(proj.columns[i][j] == (i == j ? 1 : 0));
		}
	}
}

TEST_CASE("[Projection] determinant()") {
	Projection proj(
			Vector4(1, 5, 9, 13),
			Vector4(2, 6, 11, 15),
			Vector4(4, 7, 11, 15),
			Vector4(4, 8, 12, 16));

	CHECK(proj.determinant() == -12);
}

TEST_CASE("[Projection] Inverse and invert") {
	SUBCASE("[Projection] Arbitrary projection matrix inversion") {
		Projection proj(
				Vector4(1, 5, 9, 13),
				Vector4(2, 6, 11, 15),
				Vector4(4, 7, 11, 15),
				Vector4(4, 8, 12, 16));

		Projection inverse_truth(
				Vector4(-4.0 / 12, 0, 1, -8.0 / 12),
				Vector4(8.0 / 12, -1, -1, 16.0 / 12),
				Vector4(-20.0 / 12, 2, -1, 5.0 / 12),
				Vector4(1, -1, 1, -0.75));

		Projection inverse = proj.inverse();
		CHECK(inverse[0].is_equal_approx(inverse_truth[0]));
		CHECK(inverse[1].is_equal_approx(inverse_truth[1]));
		CHECK(inverse[2].is_equal_approx(inverse_truth[2]));
		CHECK(inverse[3].is_equal_approx(inverse_truth[3]));

		proj.invert();
		CHECK(proj[0].is_equal_approx(inverse_truth[0]));
		CHECK(proj[1].is_equal_approx(inverse_truth[1]));
		CHECK(proj[2].is_equal_approx(inverse_truth[2]));
		CHECK(proj[3].is_equal_approx(inverse_truth[3]));
	}

	SUBCASE("[Projection] Orthogonal projection matrix inversion") {
		Projection p = Projection::create_orthogonal(-125.0f, 125.0f, -125.0f, 125.0f, 0.01f, 25.0f);
		p = p.inverse() * p;

		CHECK(p[0].is_equal_approx(Vector4(1, 0, 0, 0)));
		CHECK(p[1].is_equal_approx(Vector4(0, 1, 0, 0)));
		CHECK(p[2].is_equal_approx(Vector4(0, 0, 1, 0)));
		CHECK(p[3].is_equal_approx(Vector4(0, 0, 0, 1)));
	}

	SUBCASE("[Projection] Perspective projection matrix inversion") {
		Projection p = Projection::create_perspective(90.0f, 1.77777f, 0.05f, 4000.0f);
		p = p.inverse() * p;

		CHECK(p[0].is_equal_approx(Vector4(1, 0, 0, 0)));
		CHECK(p[1].is_equal_approx(Vector4(0, 1, 0, 0)));
		CHECK(p[2].is_equal_approx(Vector4(0, 0, 1, 0)));
		CHECK(p[3].is_equal_approx(Vector4(0, 0, 0, 1)));
	}
}

TEST_CASE("[Projection] Matrix product") {
	Projection proj1(
			Vector4(1, 5, 9, 13),
			Vector4(2, 6, 11, 15),
			Vector4(4, 7, 11, 15),
			Vector4(4, 8, 12, 16));

	Projection proj2(
			Vector4(0, 1, 2, 3),
			Vector4(10, 11, 12, 13),
			Vector4(20, 21, 22, 23),
			Vector4(30, 31, 32, 33));

	Projection prod = proj1 * proj2;

	CHECK(prod[0].is_equal_approx(Vector4(22, 44, 69, 93)));
	CHECK(prod[1].is_equal_approx(Vector4(132, 304, 499, 683)));
	CHECK(prod[2].is_equal_approx(Vector4(242, 564, 929, 1273)));
	CHECK(prod[3].is_equal_approx(Vector4(352, 824, 1359, 1863)));
}

TEST_CASE("[Projection] Vector transformation") {
	Projection proj(
			Vector4(1, 5, 9, 13),
			Vector4(2, 6, 11, 15),
			Vector4(4, 7, 11, 15),
			Vector4(4, 8, 12, 16));

	Vector4 vec4(1, 2, 3, 4);
	CHECK(proj.xform(vec4).is_equal_approx(Vector4(33, 70, 112, 152)));
	CHECK(proj.xform_inv(vec4).is_equal_approx(Vector4(90, 107, 111, 120)));

	Vector3 vec3(1, 2, 3);
	CHECK(proj.xform(vec3).is_equal_approx(Vector3(21, 46, 76) / 104));
}

TEST_CASE("[Projection] Plane transformation") {
	Projection proj(
			Vector4(1, 5, 9, 13),
			Vector4(2, 6, 11, 15),
			Vector4(4, 7, 11, 15),
			Vector4(4, 8, 12, 16));

	Plane plane(1, 2, 3, 4);
	CHECK(proj.xform4(plane).is_equal_approx(Plane(33, 70, 112, 152)));
}

TEST_CASE("[Projection] Values access") {
	Projection proj(
			Vector4(00, 01, 02, 03),
			Vector4(10, 11, 12, 13),
			Vector4(20, 21, 22, 23),
			Vector4(30, 31, 32, 33));

	CHECK(proj[0] == Vector4(00, 01, 02, 03));
	CHECK(proj[1] == Vector4(10, 11, 12, 13));
	CHECK(proj[2] == Vector4(20, 21, 22, 23));
	CHECK(proj[3] == Vector4(30, 31, 32, 33));
}

TEST_CASE("[Projection] flip_y() and flipped_y()") {
	Projection proj(
			Vector4(00, 01, 02, 03),
			Vector4(10, 11, 12, 13),
			Vector4(20, 21, 22, 23),
			Vector4(30, 31, 32, 33));

	Projection flipped = proj.flipped_y();

	CHECK(flipped[0] == proj[0]);
	CHECK(flipped[1] == -proj[1]);
	CHECK(flipped[2] == proj[2]);
	CHECK(flipped[3] == proj[3]);

	proj.flip_y();

	CHECK(proj[0] == flipped[0]);
	CHECK(proj[1] == flipped[1]);
	CHECK(proj[2] == flipped[2]);
	CHECK(proj[3] == flipped[3]);
}

TEST_CASE("[Projection] Jitter offset") {
	Projection proj(
			Vector4(00, 01, 02, 03),
			Vector4(10, 11, 12, 13),
			Vector4(20, 21, 22, 23),
			Vector4(30, 31, 32, 33));

	Projection offsetted = proj.jitter_offseted(Vector2(1, 2));

	CHECK(offsetted[0] == proj[0]);
	CHECK(offsetted[1] == proj[1]);
	CHECK(offsetted[2] == proj[2]);
	CHECK(offsetted[3] == proj[3] + Vector4(1, 2, 0, 0));

	proj.add_jitter_offset(Vector2(1, 2));

	CHECK(proj[0] == offsetted[0]);
	CHECK(proj[1] == offsetted[1]);
	CHECK(proj[2] == offsetted[2]);
	CHECK(proj[3] == offsetted[3]);
}

TEST_CASE("[Projection] Adjust znear") {
	Projection persp = Projection::create_perspective(90, 0.5, 1, 50, false);
	Projection adjusted = persp.perspective_znear_adjusted(2);

	CHECK(adjusted[0] == persp[0]);
	CHECK(adjusted[1] == persp[1]);
	CHECK(adjusted[2].is_equal_approx(Vector4(persp[2][0], persp[2][1], -1.083333, persp[2][3])));
	CHECK(adjusted[3].is_equal_approx(Vector4(persp[3][0], persp[3][1], -4.166666, persp[3][3])));

	persp.adjust_perspective_znear(2);

	CHECK(persp[0] == adjusted[0]);
	CHECK(persp[1] == adjusted[1]);
	CHECK(persp[2] == adjusted[2]);
	CHECK(persp[3] == adjusted[3]);
}

TEST_CASE("[Projection] Set light bias") {
	Projection proj;
	proj.set_light_bias();

	CHECK(proj[0] == Vector4(0.5, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, 0.5, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, 0.5, 0));
	CHECK(proj[3] == Vector4(0.5, 0.5, 0.5, 1));
}

TEST_CASE("[Projection] Depth correction") {
	Projection corrected = Projection::create_depth_correction(true);

	CHECK(corrected[0] == Vector4(1, 0, 0, 0));
	CHECK(corrected[1] == Vector4(0, -1, 0, 0));
	CHECK(corrected[2] == Vector4(0, 0, -0.5, 0));
	CHECK(corrected[3] == Vector4(0, 0, 0.5, 1));

	Projection proj;
	proj.set_depth_correction(true, true, true);

	CHECK(proj[0] == corrected[0]);
	CHECK(proj[1] == corrected[1]);
	CHECK(proj[2] == corrected[2]);
	CHECK(proj[3] == corrected[3]);

	proj.set_depth_correction(false, true, true);

	CHECK(proj[0] == Vector4(1, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, 1, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, -0.5, 0));
	CHECK(proj[3] == Vector4(0, 0, 0.5, 1));

	proj.set_depth_correction(false, false, true);

	CHECK(proj[0] == Vector4(1, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, 1, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, 0.5, 0));
	CHECK(proj[3] == Vector4(0, 0, 0.5, 1));

	proj.set_depth_correction(false, false, false);

	CHECK(proj[0] == Vector4(1, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, 1, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, 1, 0));
	CHECK(proj[3] == Vector4(0, 0, 0, 1));

	proj.set_depth_correction(true, true, false);

	CHECK(proj[0] == Vector4(1, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, -1, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, -1, 0));
	CHECK(proj[3] == Vector4(0, 0, 0, 1));
}

TEST_CASE("[Projection] Light atlas rect") {
	Projection rect = Projection::create_light_atlas_rect(Rect2(1, 2, 30, 40));

	CHECK(rect[0] == Vector4(30, 0, 0, 0));
	CHECK(rect[1] == Vector4(0, 40, 0, 0));
	CHECK(rect[2] == Vector4(0, 0, 1, 0));
	CHECK(rect[3] == Vector4(1, 2, 0, 1));

	Projection proj;
	proj.set_light_atlas_rect(Rect2(1, 2, 30, 40));

	CHECK(proj[0] == rect[0]);
	CHECK(proj[1] == rect[1]);
	CHECK(proj[2] == rect[2]);
	CHECK(proj[3] == rect[3]);
}

TEST_CASE("[Projection] Make scale") {
	Projection proj;
	proj.make_scale(Vector3(2, 3, 4));

	CHECK(proj[0] == Vector4(2, 0, 0, 0));
	CHECK(proj[1] == Vector4(0, 3, 0, 0));
	CHECK(proj[2] == Vector4(0, 0, 4, 0));
	CHECK(proj[3] == Vector4(0, 0, 0, 1));
}

TEST_CASE("[Projection] Scale translate to fit aabb") {
	Projection fit = Projection::create_fit_aabb(AABB(Vector3(), Vector3(0.1, 0.2, 0.4)));

	CHECK(fit[0] == Vector4(20, 0, 0, 0));
	CHECK(fit[1] == Vector4(0, 10, 0, 0));
	CHECK(fit[2] == Vector4(0, 0, 5, 0));
	CHECK(fit[3] == Vector4(-1, -1, -1, 1));

	Projection proj;
	proj.scale_translate_to_fit(AABB(Vector3(), Vector3(0.1, 0.2, 0.4)));

	CHECK(proj[0] == fit[0]);
	CHECK(proj[1] == fit[1]);
	CHECK(proj[2] == fit[2]);
	CHECK(proj[3] == fit[3]);
}

TEST_CASE("[Projection] Perspective") {
	Projection persp = Projection::create_perspective(90, 0.5, 5, 15, false);

	CHECK(persp[0].is_equal_approx(Vector4(2, 0, 0, 0)));
	CHECK(persp[1].is_equal_approx(Vector4(0, 1, 0, 0)));
	CHECK(persp[2].is_equal_approx(Vector4(0, 0, -2, -1)));
	CHECK(persp[3].is_equal_approx(Vector4(0, 0, -15, 0)));

	Projection proj;
	proj.set_perspective(90, 0.5, 5, 15, false);

	CHECK(proj[0] == persp[0]);
	CHECK(proj[1] == persp[1]);
	CHECK(proj[2] == persp[2]);
	CHECK(proj[3] == persp[3]);
}

TEST_CASE("[Projection] Frustum") {
	Projection frustum = Projection::create_frustum(15, 20, 10, 12, 5, 15);

	CHECK(frustum[0].is_equal_approx(Vector4(2, 0, 0, 0)));
	CHECK(frustum[1].is_equal_approx(Vector4(0, 5, 0, 0)));
	CHECK(frustum[2].is_equal_approx(Vector4(7, 11, -2, -1)));
	CHECK(frustum[3].is_equal_approx(Vector4(0, 0, -15, 0)));

	Projection proj;
	proj.set_frustum(15, 20, 10, 12, 5, 15);

	CHECK(proj[0] == frustum[0]);
	CHECK(proj[1] == frustum[1]);
	CHECK(proj[2] == frustum[2]);
	CHECK(proj[3] == frustum[3]);
}

TEST_CASE("[Projection] Ortho") {
	Projection ortho = Projection::create_orthogonal(15, 20, 10, 12, 5, 15);

	CHECK(ortho[0].is_equal_approx(Vector4(0.4, 0, 0, 0)));
	CHECK(ortho[1].is_equal_approx(Vector4(0, 1, 0, 0)));
	CHECK(ortho[2].is_equal_approx(Vector4(0, 0, -0.2, 0)));
	CHECK(ortho[3].is_equal_approx(Vector4(-7, -11, -2, 1)));

	Projection proj;
	proj.set_orthogonal(15, 20, 10, 12, 5, 15);

	CHECK(proj[0] == ortho[0]);
	CHECK(proj[1] == ortho[1]);
	CHECK(proj[2] == ortho[2]);
	CHECK(proj[3] == ortho[3]);
}

TEST_CASE("[Projection] get_fovy()") {
	double fov = Projection::get_fovy(90, 0.5);
	CHECK(fov == doctest::Approx(53.1301));
}

TEST_CASE("[Projection] Perspective values extraction") {
	Projection persp = Projection::create_perspective(90, 0.5, 1, 50, true);

	double znear = persp.get_z_near();
	double zfar = persp.get_z_far();
	double aspect = persp.get_aspect();
	double fov = persp.get_fov();

	CHECK(znear == doctest::Approx(1));
	CHECK(zfar == doctest::Approx(50));
	CHECK(aspect == doctest::Approx(0.5));
	CHECK(fov == doctest::Approx(90));

	persp.set_perspective(38, 1.3, 0.2, 8, false);

	znear = persp.get_z_near();
	zfar = persp.get_z_far();
	aspect = persp.get_aspect();
	fov = persp.get_fov();

	CHECK(znear == doctest::Approx(0.2));
	CHECK(zfar == doctest::Approx(8));
	CHECK(aspect == doctest::Approx(1.3));
	CHECK(fov == doctest::Approx(Projection::get_fovy(38, 1.3)));

	persp.set_perspective(47, 2.5, 0.9, 14, true);

	znear = persp.get_z_near();
	zfar = persp.get_z_far();
	aspect = persp.get_aspect();
	fov = persp.get_fov();

	CHECK(znear == doctest::Approx(0.9));
	CHECK(zfar == doctest::Approx(14));
	CHECK(aspect == doctest::Approx(2.5));
	CHECK(fov == doctest::Approx(47));
}

TEST_CASE("[Projection] Frustum values extraction") {
	Projection frustum = Projection::create_frustum_aspect(1.0, 4.0 / 3.0, Vector2(0.5, -0.25), 0.5, 50, true);

	double znear = frustum.get_z_near();
	double zfar = frustum.get_z_far();
	double aspect = frustum.get_aspect();
	double fov = frustum.get_fov();

	CHECK(znear == doctest::Approx(0.5));
	CHECK(zfar == doctest::Approx(50));
	CHECK(aspect == doctest::Approx(4.0 / 3.0));
	CHECK(fov == doctest::Approx(Math::rad_to_deg(Math::atan(2.0))));

	frustum.set_frustum(2.0, 1.5, Vector2(-0.5, 2), 2, 12, false);

	znear = frustum.get_z_near();
	zfar = frustum.get_z_far();
	aspect = frustum.get_aspect();
	fov = frustum.get_fov();

	CHECK(znear == doctest::Approx(2));
	CHECK(zfar == doctest::Approx(12));
	CHECK(aspect == doctest::Approx(1.5));
	CHECK(fov == doctest::Approx(Math::rad_to_deg(Math::atan(1.0) + Math::atan(0.5))));
}

TEST_CASE("[Projection] Orthographic values extraction") {
	Projection ortho = Projection::create_orthogonal(-2, 3, -0.5, 1.5, 1.2, 15);

	double znear = ortho.get_z_near();
	double zfar = ortho.get_z_far();
	double aspect = ortho.get_aspect();

	CHECK(znear == doctest::Approx(1.2));
	CHECK(zfar == doctest::Approx(15));
	CHECK(aspect == doctest::Approx(2.5));

	ortho.set_orthogonal(-7, 2, 2.5, 5.5, 0.5, 6);

	znear = ortho.get_z_near();
	zfar = ortho.get_z_far();
	aspect = ortho.get_aspect();

	CHECK(znear == doctest::Approx(0.5));
	CHECK(zfar == doctest::Approx(6));
	CHECK(aspect == doctest::Approx(3));
}

TEST_CASE("[Projection] Orthographic check") {
	Projection persp = Projection::create_perspective(90, 0.5, 1, 50, false);
	Projection ortho = Projection::create_orthogonal(15, 20, 10, 12, 5, 15);

	CHECK(!persp.is_orthogonal());
	CHECK(ortho.is_orthogonal());
}

TEST_CASE("[Projection] Planes extraction") {
	Projection persp = Projection::create_perspective(90, 1, 1, 40, false);
	Vector<Plane> planes = persp.get_projection_planes(Transform3D());

	CHECK(planes[Projection::PLANE_NEAR].normalized().is_equal_approx(Plane(0, 0, 1, -1)));
	CHECK(planes[Projection::PLANE_FAR].normalized().is_equal_approx(Plane(0, 0, -1, 40)));
	CHECK(planes[Projection::PLANE_LEFT].normalized().is_equal_approx(Plane(-0.707107, 0, 0.707107, 0)));
	CHECK(planes[Projection::PLANE_TOP].normalized().is_equal_approx(Plane(0, 0.707107, 0.707107, 0)));
	CHECK(planes[Projection::PLANE_RIGHT].normalized().is_equal_approx(Plane(0.707107, 0, 0.707107, 0)));
	CHECK(planes[Projection::PLANE_BOTTOM].normalized().is_equal_approx(Plane(0, -0.707107, 0.707107, 0)));

	Plane plane_array[6]{
		persp.get_projection_plane(Projection::PLANE_NEAR),
		persp.get_projection_plane(Projection::PLANE_FAR),
		persp.get_projection_plane(Projection::PLANE_LEFT),
		persp.get_projection_plane(Projection::PLANE_TOP),
		persp.get_projection_plane(Projection::PLANE_RIGHT),
		persp.get_projection_plane(Projection::PLANE_BOTTOM)
	};

	CHECK(plane_array[Projection::PLANE_NEAR].normalized().is_equal_approx(planes[Projection::PLANE_NEAR].normalized()));
	CHECK(plane_array[Projection::PLANE_FAR].normalized().is_equal_approx(planes[Projection::PLANE_FAR].normalized()));
	CHECK(plane_array[Projection::PLANE_LEFT].normalized().is_equal_approx(planes[Projection::PLANE_LEFT].normalized()));
	CHECK(plane_array[Projection::PLANE_TOP].normalized().is_equal_approx(planes[Projection::PLANE_TOP].normalized()));
	CHECK(plane_array[Projection::PLANE_RIGHT].normalized().is_equal_approx(planes[Projection::PLANE_RIGHT].normalized()));
	CHECK(plane_array[Projection::PLANE_BOTTOM].normalized().is_equal_approx(planes[Projection::PLANE_BOTTOM].normalized()));
}

TEST_CASE("[Projection] Perspective Half extents") {
	constexpr real_t sqrt3 = 1.7320508;
	Projection persp = Projection::create_perspective(90, 1, 1, 40, false);
	Vector2 ne = persp.get_viewport_half_extents();
	Vector2 fe = persp.get_far_plane_half_extents();

	CHECK(ne.is_equal_approx(Vector2(1, 1) * 1));
	CHECK(fe.is_equal_approx(Vector2(1, 1) * 40));

	persp.set_perspective(120, sqrt3, 0.8, 10, true);
	ne = persp.get_viewport_half_extents();
	fe = persp.get_far_plane_half_extents();

	CHECK(ne.is_equal_approx(Vector2(sqrt3, 1.0) * 0.8));
	CHECK(fe.is_equal_approx(Vector2(sqrt3, 1.0) * 10));

	persp.set_perspective(60, 1.2, 0.5, 15, false);
	ne = persp.get_viewport_half_extents();
	fe = persp.get_far_plane_half_extents();

	CHECK(ne.is_equal_approx(Vector2(sqrt3 / 3 * 1.2, sqrt3 / 3) * 0.5));
	CHECK(fe.is_equal_approx(Vector2(sqrt3 / 3 * 1.2, sqrt3 / 3) * 15));
}

TEST_CASE("[Projection] Orthographic Half extents") {
	Projection ortho = Projection::create_orthogonal(-3, 3, -1.5, 1.5, 1.2, 15);
	Vector2 ne = ortho.get_viewport_half_extents();
	Vector2 fe = ortho.get_far_plane_half_extents();

	CHECK(ne.is_equal_approx(Vector2(3, 1.5)));
	CHECK(fe.is_equal_approx(Vector2(3, 1.5)));

	ortho.set_orthogonal(-7, 7, -2.5, 2.5, 0.5, 6);
	ne = ortho.get_viewport_half_extents();
	fe = ortho.get_far_plane_half_extents();

	CHECK(ne.is_equal_approx(Vector2(7, 2.5)));
	CHECK(fe.is_equal_approx(Vector2(7, 2.5)));
}

TEST_CASE("[Projection] Endpoints") {
	constexpr real_t sqrt3 = 1.7320508;
	Projection persp = Projection::create_perspective(90, 1, 1, 40, false);
	Vector3 ep[8];
	persp.get_endpoints(Transform3D(), ep);

	CHECK(ep[0].is_equal_approx(Vector3(-1, 1, -1) * 40));
	CHECK(ep[1].is_equal_approx(Vector3(-1, -1, -1) * 40));
	CHECK(ep[2].is_equal_approx(Vector3(1, 1, -1) * 40));
	CHECK(ep[3].is_equal_approx(Vector3(1, -1, -1) * 40));
	CHECK(ep[4].is_equal_approx(Vector3(-1, 1, -1) * 1));
	CHECK(ep[5].is_equal_approx(Vector3(-1, -1, -1) * 1));
	CHECK(ep[6].is_equal_approx(Vector3(1, 1, -1) * 1));
	CHECK(ep[7].is_equal_approx(Vector3(1, -1, -1) * 1));

	persp.set_perspective(120, sqrt3, 0.8, 10, true);
	persp.get_endpoints(Transform3D(), ep);

	CHECK(ep[0].is_equal_approx(Vector3(-sqrt3, 1, -1) * 10));
	CHECK(ep[1].is_equal_approx(Vector3(-sqrt3, -1, -1) * 10));
	CHECK(ep[2].is_equal_approx(Vector3(sqrt3, 1, -1) * 10));
	CHECK(ep[3].is_equal_approx(Vector3(sqrt3, -1, -1) * 10));
	CHECK(ep[4].is_equal_approx(Vector3(-sqrt3, 1, -1) * 0.8));
	CHECK(ep[5].is_equal_approx(Vector3(-sqrt3, -1, -1) * 0.8));
	CHECK(ep[6].is_equal_approx(Vector3(sqrt3, 1, -1) * 0.8));
	CHECK(ep[7].is_equal_approx(Vector3(sqrt3, -1, -1) * 0.8));

	persp.set_perspective(60, 1.2, 0.5, 15, false);
	persp.get_endpoints(Transform3D(), ep);

	CHECK(ep[0].is_equal_approx(Vector3(-sqrt3 / 3 * 1.2, sqrt3 / 3, -1) * 15));
	CHECK(ep[1].is_equal_approx(Vector3(-sqrt3 / 3 * 1.2, -sqrt3 / 3, -1) * 15));
	CHECK(ep[2].is_equal_approx(Vector3(sqrt3 / 3 * 1.2, sqrt3 / 3, -1) * 15));
	CHECK(ep[3].is_equal_approx(Vector3(sqrt3 / 3 * 1.2, -sqrt3 / 3, -1) * 15));
	CHECK(ep[4].is_equal_approx(Vector3(-sqrt3 / 3 * 1.2, sqrt3 / 3, -1) * 0.5));
	CHECK(ep[5].is_equal_approx(Vector3(-sqrt3 / 3 * 1.2, -sqrt3 / 3, -1) * 0.5));
	CHECK(ep[6].is_equal_approx(Vector3(sqrt3 / 3 * 1.2, sqrt3 / 3, -1) * 0.5));
	CHECK(ep[7].is_equal_approx(Vector3(sqrt3 / 3 * 1.2, -sqrt3 / 3, -1) * 0.5));
}

TEST_CASE("[Projection] LOD multiplier") {
	constexpr real_t sqrt3 = 1.7320508;
	Projection proj;
	real_t multiplier;

	proj.set_perspective(60, 1, 1, 40, false);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(2 * sqrt3 / 3));

	proj.set_perspective(120, 1.5, 0.5, 20, false);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(3 * sqrt3));

	proj.set_orthogonal(15, 20, 10, 12, 5, 15);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(5));

	proj.set_orthogonal(-5, 15, -8, 10, 1.5, 10);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(20));

	proj.set_frustum(1.0, 4.0 / 3.0, Vector2(0.5, -0.25), 0.5, 50, false);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(8.0 / 3.0));

	proj.set_frustum(2.0, 1.2, Vector2(-0.1, 0.8), 1, 10, true);
	multiplier = proj.get_lod_multiplier();
	CHECK(multiplier == doctest::Approx(2));
}

TEST_CASE("[Projection] Pixels per meter") {
	constexpr real_t sqrt3 = 1.7320508;
	Projection proj;
	int ppm;

	proj.set_perspective(60, 1, 1, 40, false);
	ppm = proj.get_pixels_per_meter(1024);
	CHECK(ppm == int(1536.0f / sqrt3));

	proj.set_perspective(120, 1.5, 0.5, 20, false);
	ppm = proj.get_pixels_per_meter(1200);
	CHECK(ppm == int(800.0f / sqrt3));

	proj.set_orthogonal(15, 20, 10, 12, 5, 15);
	ppm = proj.get_pixels_per_meter(500);
	CHECK(ppm == 100);

	proj.set_orthogonal(-5, 15, -8, 10, 1.5, 10);
	ppm = proj.get_pixels_per_meter(640);
	CHECK(ppm == 32);

	proj.set_frustum(1.0, 4.0 / 3.0, Vector2(0.5, -0.25), 0.5, 50, false);
	ppm = proj.get_pixels_per_meter(2048);
	CHECK(ppm == 1536);

	proj.set_frustum(2.0, 1.2, Vector2(-0.1, 0.8), 1, 10, true);
	ppm = proj.get_pixels_per_meter(800);
	CHECK(ppm == 400);
}

} // namespace TestProjection

// === Merged from tests/core/math/test_quaternion.cpp ===
/*  test_quaternion.cpp                                                   */
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

#include "tests/test_macros.h"



#include "core/typedefs.h"
#include "core/types.h"
#include "core/math/plane.h"

namespace TestQuaternion {

Quaternion quat_euler_yxz_deg(Vector3 angle) {
	double yaw = Math::deg_to_rad(angle[1]);
	double pitch = Math::deg_to_rad(angle[0]);
	double roll = Math::deg_to_rad(angle[2]);

	// Generate YXZ (Z-then-X-then-Y) Quaternion using single-axis Euler
	// constructor and quaternion product, both tested separately.
	Quaternion q_y = Quaternion::from_euler(Vector3(0.0, yaw, 0.0));
	Quaternion q_p = Quaternion::from_euler(Vector3(pitch, 0.0, 0.0));
	Quaternion q_r = Quaternion::from_euler(Vector3(0.0, 0.0, roll));
	// Roll-Z is followed by Pitch-X, then Yaw-Y.
	Quaternion q_yxz = q_y * q_p * q_r;

	return q_yxz;
}

TEST_CASE("[Quaternion] Default Construct") {
	constexpr Quaternion q;

	CHECK(q[0] == 0.0);
	CHECK(q[1] == 0.0);
	CHECK(q[2] == 0.0);
	CHECK(q[3] == 1.0);
}

TEST_CASE("[Quaternion] Construct x,y,z,w") {
	// Values are taken from actual use in another project & are valid (except roundoff error).
	constexpr Quaternion q(0.2391, 0.099, 0.3696, 0.8924);

	CHECK(q[0] == doctest::Approx(0.2391));
	CHECK(q[1] == doctest::Approx(0.099));
	CHECK(q[2] == doctest::Approx(0.3696));
	CHECK(q[3] == doctest::Approx(0.8924));
}

TEST_CASE("[Quaternion] Construct AxisAngle 1") {
	// Easy to visualize: 120 deg about X-axis.
	Quaternion q(Vector3(1.0, 0.0, 0.0), Math::deg_to_rad(120.0));

	// 0.866 isn't close enough; doctest::Approx doesn't cut much slack!
	CHECK(q[0] == doctest::Approx(0.866025)); // Sine of half the angle.
	CHECK(q[1] == doctest::Approx(0.0));
	CHECK(q[2] == doctest::Approx(0.0));
	CHECK(q[3] == doctest::Approx(0.5)); // Cosine of half the angle.
}

TEST_CASE("[Quaternion] Construct AxisAngle 2") {
	// Easy to visualize: 30 deg about Y-axis.
	Quaternion q(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(30.0));

	CHECK(q[0] == doctest::Approx(0.0));
	CHECK(q[1] == doctest::Approx(0.258819)); // Sine of half the angle.
	CHECK(q[2] == doctest::Approx(0.0));
	CHECK(q[3] == doctest::Approx(0.965926)); // Cosine of half the angle.
}

TEST_CASE("[Quaternion] Construct AxisAngle 3") {
	// Easy to visualize: 60 deg about Z-axis.
	Quaternion q(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(60.0));

	CHECK(q[0] == doctest::Approx(0.0));
	CHECK(q[1] == doctest::Approx(0.0));
	CHECK(q[2] == doctest::Approx(0.5)); // Sine of half the angle.
	CHECK(q[3] == doctest::Approx(0.866025)); // Cosine of half the angle.
}

TEST_CASE("[Quaternion] Construct AxisAngle 4") {
	// More complex & hard to visualize, so test w/ data from online calculator.
	constexpr Vector3 axis(1.0, 2.0, 0.5);
	Quaternion q(axis.normalized(), Math::deg_to_rad(35.0));

	CHECK(q[0] == doctest::Approx(0.131239));
	CHECK(q[1] == doctest::Approx(0.262478));
	CHECK(q[2] == doctest::Approx(0.0656194));
	CHECK(q[3] == doctest::Approx(0.953717));
}

TEST_CASE("[Quaternion] Construct from Quaternion") {
	constexpr Vector3 axis(1.0, 2.0, 0.5);
	Quaternion q_src(axis.normalized(), Math::deg_to_rad(35.0));
	Quaternion q(q_src);

	CHECK(q[0] == doctest::Approx(0.131239));
	CHECK(q[1] == doctest::Approx(0.262478));
	CHECK(q[2] == doctest::Approx(0.0656194));
	CHECK(q[3] == doctest::Approx(0.953717));
}

TEST_CASE("[Quaternion] Construct Euler SingleAxis") {
	double yaw = Math::deg_to_rad(45.0);
	double pitch = Math::deg_to_rad(30.0);
	double roll = Math::deg_to_rad(10.0);

	Vector3 euler_y(0.0, yaw, 0.0);
	Quaternion q_y = Quaternion::from_euler(euler_y);
	CHECK(q_y[0] == doctest::Approx(0.0));
	CHECK(q_y[1] == doctest::Approx(0.382684));
	CHECK(q_y[2] == doctest::Approx(0.0));
	CHECK(q_y[3] == doctest::Approx(0.923879));

	Vector3 euler_p(pitch, 0.0, 0.0);
	Quaternion q_p = Quaternion::from_euler(euler_p);
	CHECK(q_p[0] == doctest::Approx(0.258819));
	CHECK(q_p[1] == doctest::Approx(0.0));
	CHECK(q_p[2] == doctest::Approx(0.0));
	CHECK(q_p[3] == doctest::Approx(0.965926));

	Vector3 euler_r(0.0, 0.0, roll);
	Quaternion q_r = Quaternion::from_euler(euler_r);
	CHECK(q_r[0] == doctest::Approx(0.0));
	CHECK(q_r[1] == doctest::Approx(0.0));
	CHECK(q_r[2] == doctest::Approx(0.0871558));
	CHECK(q_r[3] == doctest::Approx(0.996195));
}

TEST_CASE("[Quaternion] Construct Euler YXZ dynamic axes") {
	double yaw = Math::deg_to_rad(45.0);
	double pitch = Math::deg_to_rad(30.0);
	double roll = Math::deg_to_rad(10.0);

	// Generate YXZ comparison data (Z-then-X-then-Y) using single-axis Euler
	// constructor and quaternion product, both tested separately.
	Vector3 euler_y(0.0, yaw, 0.0);
	Quaternion q_y = Quaternion::from_euler(euler_y);
	Vector3 euler_p(pitch, 0.0, 0.0);
	Quaternion q_p = Quaternion::from_euler(euler_p);
	Vector3 euler_r(0.0, 0.0, roll);
	Quaternion q_r = Quaternion::from_euler(euler_r);

	// Instrinsically, Yaw-Y then Pitch-X then Roll-Z.
	// Extrinsically, Roll-Z is followed by Pitch-X, then Yaw-Y.
	Quaternion check_yxz = q_y * q_p * q_r;

	// Test construction from YXZ Euler angles.
	Vector3 euler_yxz(pitch, yaw, roll);
	Quaternion q = Quaternion::from_euler(euler_yxz);
	CHECK(q[0] == doctest::Approx(check_yxz[0]));
	CHECK(q[1] == doctest::Approx(check_yxz[1]));
	CHECK(q[2] == doctest::Approx(check_yxz[2]));
	CHECK(q[3] == doctest::Approx(check_yxz[3]));

	CHECK(q.is_equal_approx(check_yxz));
	CHECK(q.get_euler().is_equal_approx(euler_yxz));
	CHECK(check_yxz.get_euler().is_equal_approx(euler_yxz));
}

TEST_CASE("[Quaternion] Construct Basis Euler") {
	double yaw = Math::deg_to_rad(45.0);
	double pitch = Math::deg_to_rad(30.0);
	double roll = Math::deg_to_rad(10.0);
	Vector3 euler_yxz(pitch, yaw, roll);
	Quaternion q_yxz = Quaternion::from_euler(euler_yxz);
	Basis basis_axes = Basis::from_euler(euler_yxz);
	Quaternion q(basis_axes);
	CHECK(q.is_equal_approx(q_yxz));
}

TEST_CASE("[Quaternion] Construct Basis Axes") {
	// Arbitrary Euler angles.
	const Vector3 euler_yxz(Math::deg_to_rad(31.41), Math::deg_to_rad(-49.16), Math::deg_to_rad(12.34));
	// Basis vectors from online calculation of rotation matrix.
	constexpr Vector3 i_unit(0.5545787, 0.1823950, 0.8118957);
	constexpr Vector3 j_unit(-0.5249245, 0.8337420, 0.1712555);
	constexpr Vector3 k_unit(-0.6456754, -0.5211586, 0.5581192);
	// Quaternion from online calculation.
	constexpr Quaternion q_calc(0.2016913, -0.4245716, 0.206033, 0.8582598);
	// Quaternion from local calculation.
	const Quaternion q_local = quat_euler_yxz_deg(Vector3(31.41, -49.16, 12.34));
	// Quaternion from Euler angles constructor.
	const Quaternion q_euler = Quaternion::from_euler(euler_yxz);
	CHECK(q_calc.is_equal_approx(q_local));
	CHECK(q_local.is_equal_approx(q_euler));

	// Calculate Basis and construct Quaternion.
	// When this is written, C++ Basis class does not construct from basis vectors.
	// This is by design, but may be subject to change.
	// Workaround by constructing Basis from Euler angles.
	// basis_axes = Basis(i_unit, j_unit, k_unit);
	Basis basis_axes = Basis::from_euler(euler_yxz);
	Quaternion q(basis_axes);

	CHECK(basis_axes.get_column(0).is_equal_approx(i_unit));
	CHECK(basis_axes.get_column(1).is_equal_approx(j_unit));
	CHECK(basis_axes.get_column(2).is_equal_approx(k_unit));

	CHECK(q.is_equal_approx(q_calc));
	CHECK_FALSE(q.inverse().is_equal_approx(q_calc));
	CHECK(q.is_equal_approx(q_local));
	CHECK(q.is_equal_approx(q_euler));
	CHECK(q[0] == doctest::Approx(0.2016913));
	CHECK(q[1] == doctest::Approx(-0.4245716));
	CHECK(q[2] == doctest::Approx(0.206033));
	CHECK(q[3] == doctest::Approx(0.8582598));
}

TEST_CASE("[Quaternion] Construct Shortest Arc For 180 Degree Arc") {
	Vector3 up(0, 1, 0);
	Vector3 down(0, -1, 0);
	Vector3 left(-1, 0, 0);
	Vector3 right(1, 0, 0);
	Vector3 forward(0, 0, -1);
	Vector3 back(0, 0, 1);

	// When we have a 180 degree rotation quaternion which was defined as
	// A to B, logically when we transform A we expect to get B.
	Quaternion left_to_right(left, right);
	Quaternion right_to_left(right, left);
	CHECK(left_to_right.xform(left).is_equal_approx(right));
	CHECK(Quaternion(right, left).xform(right).is_equal_approx(left));
	CHECK(Quaternion(up, down).xform(up).is_equal_approx(down));
	CHECK(Quaternion(down, up).xform(down).is_equal_approx(up));
	CHECK(Quaternion(forward, back).xform(forward).is_equal_approx(back));
	CHECK(Quaternion(back, forward).xform(back).is_equal_approx(forward));

	// With (arbitrary) opposite vectors that are not axis-aligned as parameters.
	Vector3 diagonal_up = Vector3(1.2, 2.3, 4.5).normalized();
	Vector3 diagonal_down = -diagonal_up;
	Quaternion q1(diagonal_up, diagonal_down);
	CHECK(q1.xform(diagonal_down).is_equal_approx(diagonal_up));
	CHECK(q1.xform(diagonal_up).is_equal_approx(diagonal_down));

	// For the consistency of the rotation direction, they should be symmetrical to the plane.
	CHECK(left_to_right.is_equal_approx(right_to_left.inverse()));

	// If vectors are same, no rotation.
	CHECK(Quaternion(diagonal_up, diagonal_up).is_equal_approx(Quaternion()));
}

TEST_CASE("[Quaternion] Get Euler Orders") {
	double x = Math::deg_to_rad(30.0);
	double y = Math::deg_to_rad(45.0);
	double z = Math::deg_to_rad(10.0);
	Vector3 euler(x, y, z);
	for (int i = 0; i < 6; i++) {
		EulerOrder order = (EulerOrder)i;
		Basis basis = Basis::from_euler(euler, order);
		Quaternion q = Quaternion(basis);
		Vector3 check = q.get_euler(order);
		CHECK_MESSAGE(check.is_equal_approx(euler),
				"Quaternion get_euler method should return the original angles.");
		CHECK_MESSAGE(check.is_equal_approx(basis.get_euler(order)),
				"Quaternion get_euler method should behave the same as Basis get_euler.");
	}
}

TEST_CASE("[Quaternion] Product (book)") {
	// Example from "Quaternions and Rotation Sequences" by Jack Kuipers, p. 108.
	constexpr Quaternion p(1.0, -2.0, 1.0, 3.0);
	constexpr Quaternion q(-1.0, 2.0, 3.0, 2.0);

	constexpr Quaternion pq = p * q;
	CHECK(pq[0] == doctest::Approx(-9.0));
	CHECK(pq[1] == doctest::Approx(-2.0));
	CHECK(pq[2] == doctest::Approx(11.0));
	CHECK(pq[3] == doctest::Approx(8.0));
}

TEST_CASE("[Quaternion] Product") {
	double yaw = Math::deg_to_rad(45.0);
	double pitch = Math::deg_to_rad(30.0);
	double roll = Math::deg_to_rad(10.0);

	Vector3 euler_y(0.0, yaw, 0.0);
	Quaternion q_y = Quaternion::from_euler(euler_y);
	CHECK(q_y[0] == doctest::Approx(0.0));
	CHECK(q_y[1] == doctest::Approx(0.382684));
	CHECK(q_y[2] == doctest::Approx(0.0));
	CHECK(q_y[3] == doctest::Approx(0.923879));

	Vector3 euler_p(pitch, 0.0, 0.0);
	Quaternion q_p = Quaternion::from_euler(euler_p);
	CHECK(q_p[0] == doctest::Approx(0.258819));
	CHECK(q_p[1] == doctest::Approx(0.0));
	CHECK(q_p[2] == doctest::Approx(0.0));
	CHECK(q_p[3] == doctest::Approx(0.965926));

	Vector3 euler_r(0.0, 0.0, roll);
	Quaternion q_r = Quaternion::from_euler(euler_r);
	CHECK(q_r[0] == doctest::Approx(0.0));
	CHECK(q_r[1] == doctest::Approx(0.0));
	CHECK(q_r[2] == doctest::Approx(0.0871558));
	CHECK(q_r[3] == doctest::Approx(0.996195));

	// Test ZYX dynamic-axes since test data is available online.
	// Rotate first about X axis, then new Y axis, then new Z axis.
	// (Godot uses YXZ Yaw-Pitch-Roll order).
	Quaternion q_yp = q_y * q_p;
	CHECK(q_yp[0] == doctest::Approx(0.239118));
	CHECK(q_yp[1] == doctest::Approx(0.369644));
	CHECK(q_yp[2] == doctest::Approx(-0.099046));
	CHECK(q_yp[3] == doctest::Approx(0.892399));

	Quaternion q_ryp = q_r * q_yp;
	CHECK(q_ryp[0] == doctest::Approx(0.205991));
	CHECK(q_ryp[1] == doctest::Approx(0.389078));
	CHECK(q_ryp[2] == doctest::Approx(-0.0208912));
	CHECK(q_ryp[3] == doctest::Approx(0.897636));
}

TEST_CASE("[Quaternion] xform unit vectors") {
	// Easy to visualize: 120 deg about X-axis.
	// Transform the i, j, & k unit vectors.
	Quaternion q(Vector3(1.0, 0.0, 0.0), Math::deg_to_rad(120.0));
	Vector3 i_t = q.xform(Vector3(1.0, 0.0, 0.0));
	Vector3 j_t = q.xform(Vector3(0.0, 1.0, 0.0));
	Vector3 k_t = q.xform(Vector3(0.0, 0.0, 1.0));
	//
	CHECK(i_t.is_equal_approx(Vector3(1.0, 0.0, 0.0)));
	CHECK(j_t.is_equal_approx(Vector3(0.0, -0.5, 0.866025)));
	CHECK(k_t.is_equal_approx(Vector3(0.0, -0.866025, -0.5)));
	CHECK(i_t.length_squared() == doctest::Approx(1.0));
	CHECK(j_t.length_squared() == doctest::Approx(1.0));
	CHECK(k_t.length_squared() == doctest::Approx(1.0));

	// Easy to visualize: 30 deg about Y-axis.
	q = Quaternion(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(30.0));
	i_t = q.xform(Vector3(1.0, 0.0, 0.0));
	j_t = q.xform(Vector3(0.0, 1.0, 0.0));
	k_t = q.xform(Vector3(0.0, 0.0, 1.0));
	//
	CHECK(i_t.is_equal_approx(Vector3(0.866025, 0.0, -0.5)));
	CHECK(j_t.is_equal_approx(Vector3(0.0, 1.0, 0.0)));
	CHECK(k_t.is_equal_approx(Vector3(0.5, 0.0, 0.866025)));
	CHECK(i_t.length_squared() == doctest::Approx(1.0));
	CHECK(j_t.length_squared() == doctest::Approx(1.0));
	CHECK(k_t.length_squared() == doctest::Approx(1.0));

	// Easy to visualize: 60 deg about Z-axis.
	q = Quaternion(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(60.0));
	i_t = q.xform(Vector3(1.0, 0.0, 0.0));
	j_t = q.xform(Vector3(0.0, 1.0, 0.0));
	k_t = q.xform(Vector3(0.0, 0.0, 1.0));
	//
	CHECK(i_t.is_equal_approx(Vector3(0.5, 0.866025, 0.0)));
	CHECK(j_t.is_equal_approx(Vector3(-0.866025, 0.5, 0.0)));
	CHECK(k_t.is_equal_approx(Vector3(0.0, 0.0, 1.0)));
	CHECK(i_t.length_squared() == doctest::Approx(1.0));
	CHECK(j_t.length_squared() == doctest::Approx(1.0));
	CHECK(k_t.length_squared() == doctest::Approx(1.0));
}

TEST_CASE("[Quaternion] xform vector") {
	// Arbitrary quaternion rotates an arbitrary vector.
	const Vector3 euler_yzx(Math::deg_to_rad(31.41), Math::deg_to_rad(-49.16), Math::deg_to_rad(12.34));
	const Basis basis_axes = Basis::from_euler(euler_yzx);
	const Quaternion q(basis_axes);

	constexpr Vector3 v_arb(3.0, 4.0, 5.0);
	const Vector3 v_rot = q.xform(v_arb);
	const Vector3 v_compare = basis_axes.xform(v_arb);

	CHECK(v_rot.length_squared() == doctest::Approx(v_arb.length_squared()));
	CHECK(v_rot.is_equal_approx(v_compare));
}

// Test vector xform for a single combination of Quaternion and Vector.
void test_quat_vec_rotate(Vector3 euler_yzx, Vector3 v_in) {
	const Basis basis_axes = Basis::from_euler(euler_yzx);
	const Quaternion q(basis_axes);

	const Vector3 v_rot = q.xform(v_in);
	const Vector3 v_compare = basis_axes.xform(v_in);

	CHECK(v_rot.length_squared() == doctest::Approx(v_in.length_squared()));
	CHECK(v_rot.is_equal_approx(v_compare));
}

TEST_CASE("[Quaternion] Finite number checks") {
	constexpr real_t x = Math::NaN;

	CHECK_MESSAGE(
			Quaternion(0, 1, 2, 3).is_finite(),
			"Quaternion with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Quaternion(x, 1, 2, 3).is_finite(),
			"Quaternion with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, x, 2, 3).is_finite(),
			"Quaternion with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, 1, x, 3).is_finite(),
			"Quaternion with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, 1, 2, x).is_finite(),
			"Quaternion with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Quaternion(x, x, 2, 3).is_finite(),
			"Quaternion with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(x, 1, x, 3).is_finite(),
			"Quaternion with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(x, 1, 2, x).is_finite(),
			"Quaternion with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, x, x, 3).is_finite(),
			"Quaternion with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, x, 2, x).is_finite(),
			"Quaternion with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(0, 1, x, x).is_finite(),
			"Quaternion with two components infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Quaternion(0, x, x, x).is_finite(),
			"Quaternion with three components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(x, 1, x, x).is_finite(),
			"Quaternion with three components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(x, x, 2, x).is_finite(),
			"Quaternion with three components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Quaternion(x, x, x, 3).is_finite(),
			"Quaternion with three components infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Quaternion(x, x, x, x).is_finite(),
			"Quaternion with four components infinite should not be finite.");
}

} // namespace TestQuaternion

// === Merged from tests/core/math/test_random_number_generator.cpp ===
/*  test_random_number_generator.cpp                                      */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestRandomNumberGenerator {

TEST_CASE("[RandomNumberGenerator] Float") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0);

	INFO("Should give float between 0.0 and 1.0.");
	for (int i = 0; i < 1000; i++) {
		real_t n = rng->randf();
		CHECK(n >= 0.0);
		CHECK(n <= 1.0);
	}
}

TEST_CASE("[RandomNumberGenerator] Integer range via modulo") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0);

	INFO("Should give integer between 0 and 100.");
	for (int i = 0; i < 1000; i++) {
		uint32_t n = rng->randi() % 100;
		CHECK(n >= 0);
		CHECK(n <= 100);
	}
}

TEST_CASE_MAY_FAIL("[RandomNumberGenerator] Integer 32 bit") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0); // Change the seed if this fails.

	bool higher = false;
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t n = rng->randi();
		if (n > 0x0fff'ffff) {
			higher = true;
			break;
		}
	}
	INFO("Current seed: ", rng->get_seed());
	INFO("Current iteration: ", i);
	CHECK_MESSAGE(higher, "Given current seed, this should give an integer higher than 0x0fff'ffff at least once.");
}

TEST_CASE("[RandomNumberGenerator] Float and integer range") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0);
	uint64_t initial_state = rng->get_state();
	uint32_t initial_seed = rng->get_seed();

	INFO("Should give float between -100.0 and 100.0, base test.");
	for (int i = 0; i < 1000; i++) {
		real_t n0 = rng->randf_range(-100.0, 100.0);
		CHECK(n0 >= -100);
		CHECK(n0 <= 100);
	}

	rng->randomize();
	INFO("Should give float between -75.0 and 75.0.");
	INFO("Shouldn't be affected by randomize.");
	for (int i = 0; i < 1000; i++) {
		real_t n1 = rng->randf_range(-75.0, 75.0);
		CHECK(n1 >= -75);
		CHECK(n1 <= 75);
	}

	rng->set_state(initial_state);
	INFO("Should give integer between -50 and 50.");
	INFO("Shouldn't be affected by set_state.");
	for (int i = 0; i < 1000; i++) {
		real_t n2 = rng->randi_range(-50, 50);
		CHECK(n2 >= -50);
		CHECK(n2 <= 50);
	}

	rng->set_seed(initial_seed);
	INFO("Should give integer between -25 and 25.");
	INFO("Shouldn't be affected by set_seed.");
	for (int i = 0; i < 1000; i++) {
		int32_t n3 = rng->randi_range(-25, 25);
		CHECK(n3 >= -25);
		CHECK(n3 <= 25);
	}

	rng->randf();
	rng->randf();

	INFO("Should give float between -10.0 and 10.0.");
	INFO("Shouldn't be affected after generating new numbers.");
	for (int i = 0; i < 1000; i++) {
		real_t n4 = rng->randf_range(-10.0, 10.0);
		CHECK(n4 >= -10);
		CHECK(n4 <= 10);
	}

	rng->randi();
	rng->randi();

	INFO("Should give integer between -5 and 5.");
	INFO("Shouldn't be affected after generating new numbers.");
	for (int i = 0; i < 1000; i++) {
		real_t n5 = rng->randf_range(-5, 5);
		CHECK(n5 >= -5);
		CHECK(n5 <= 5);
	}
}

TEST_CASE_MAY_FAIL("[RandomNumberGenerator] Normal distribution") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(1); // Change the seed if this fails.
	INFO("Should give a number between -5 to 5 (5 std deviations away; above 99.7% chance it will be in this range).");
	INFO("Standard randfn function call.");
	for (int i = 0; i < 100; i++) {
		real_t n = rng->randfn();
		CHECK(n >= -5);
		CHECK(n <= 5);
	}

	INFO("Should give number between -5 to 5 after multiple randi/randf calls.");
	INFO("5 std deviations away; above 99.7% chance it will be in this range.");
	rng->randf();
	rng->randi();
	for (int i = 0; i < 100; i++) {
		real_t n = rng->randfn();
		CHECK(n >= -5);
		CHECK(n <= 5);
	}

	INFO("Checks if user defined mean and deviation work properly.");
	INFO("5 std deviations away; above 99.7% chance it will be in this range.");
	for (int i = 0; i < 100; i++) {
		real_t n = rng->randfn(5, 10);
		CHECK(n >= -45);
		CHECK(n <= 55);
	}

	INFO("Checks if randfn works with changed seeds.");
	INFO("5 std deviations away; above 99.7% chance it will be in this range.");
	rng->randomize();
	for (int i = 0; i < 100; i++) {
		real_t n = rng->randfn(3, 3);
		CHECK(n >= -12);
		CHECK(n <= 18);
	}
}

TEST_CASE("[RandomNumberGenerator] Zero for first number immediately after seeding") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0);
	uint32_t n1 = rng->randi();
	uint32_t n2 = rng->randi();
	INFO("Initial random values: ", n1, " ", n2);
	CHECK(n1 != 0);

	rng->set_seed(1);
	uint32_t n3 = rng->randi();
	uint32_t n4 = rng->randi();
	INFO("Values after changing the seed: ", n3, " ", n4);
	CHECK(n3 != 0);
}

TEST_CASE("[RandomNumberGenerator] Restore state") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->randomize();
	uint64_t last_seed = rng->get_seed();
	INFO("Current seed: ", last_seed);

	rng->randi();
	rng->randi();

	CHECK_MESSAGE(rng->get_seed() == last_seed,
			"The seed should remain the same after generating some numbers");

	uint64_t saved_state = rng->get_state();
	INFO("Current state: ", saved_state);

	real_t f1_before = rng->randf();
	real_t f2_before = rng->randf();
	INFO("This seed produces: ", f1_before, " ", f2_before);

	// Restore now.
	rng->set_state(saved_state);

	real_t f1_after = rng->randf();
	real_t f2_after = rng->randf();
	INFO("Resetting the state produces: ", f1_after, " ", f2_after);

	String msg = "Should restore the sequence of numbers after resetting the state";
	CHECK_MESSAGE(f1_before == f1_after, msg);
	CHECK_MESSAGE(f2_before == f2_after, msg);
}

TEST_CASE("[RandomNumberGenerator] Restore from seed") {
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	rng->set_seed(0);
	INFO("Current seed: ", rng->get_seed());
	uint32_t s0_1_before = rng->randi();
	uint32_t s0_2_before = rng->randi();
	INFO("This seed produces: ", s0_1_before, " ", s0_2_before);

	rng->set_seed(9000);
	INFO("Current seed: ", rng->get_seed());
	uint32_t s9000_1 = rng->randi();
	uint32_t s9000_2 = rng->randi();
	INFO("This seed produces: ", s9000_1, " ", s9000_2);

	rng->set_seed(0);
	INFO("Current seed: ", rng->get_seed());
	uint32_t s0_1_after = rng->randi();
	uint32_t s0_2_after = rng->randi();
	INFO("This seed produces: ", s0_1_after, " ", s0_2_after);

	String msg = "Should restore the sequence of numbers after resetting the seed";
	CHECK_MESSAGE(s0_1_before == s0_1_after, msg);
	CHECK_MESSAGE(s0_2_before == s0_2_after, msg);
}

TEST_CASE_MAY_FAIL("[RandomNumberGenerator] randi_range bias check") {
	int zeros = 0;
	int ones = 0;
	Ref<RandomNumberGenerator> rng = memnew(RandomNumberGenerator);
	for (int i = 0; i < 10000; i++) {
		int val = rng->randi_range(0, 1);
		val == 0 ? zeros++ : ones++;
	}
	CHECK_MESSAGE(std::abs(zeros * 1.0 / ones - 1.0) < 0.1, "The ratio of zeros to ones should be nearly 1");

	int vals[10] = { 0 };
	for (int i = 0; i < 1000000; i++) {
		vals[rng->randi_range(0, 9)]++;
	}

	for (int i = 0; i < 10; i++) {
		CHECK_MESSAGE(std::abs(vals[i] / 1000000.0 - 0.1) < 0.01, "Each element should appear roughly 10% of the time");
	}
}

} // namespace TestRandomNumberGenerator

// === Merged from tests/core/math/test_rect2.cpp ===
/*  test_rect2.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestRect2 {

TEST_CASE("[Rect2] Constructor methods") {
	constexpr Rect2 rect = Rect2(0, 100, 1280, 720);
	constexpr Rect2 rect_vector = Rect2(Vector2(0, 100), Vector2(1280, 720));
	constexpr Rect2 rect_copy_rect = Rect2(rect);
	const Rect2 rect_copy_recti = Rect2(Rect2i(0, 100, 1280, 720));

	static_assert(
			rect == rect_vector,
			"Rect2s created with the same dimensions but by different methods should be equal.");
	static_assert(
			rect == rect_copy_rect,
			"Rect2s created with the same dimensions but by different methods should be equal.");
	CHECK_MESSAGE(
			rect == rect_copy_recti,
			"Rect2s created with the same dimensions but by different methods should be equal.");
}

TEST_CASE("[Rect2] String conversion") {
	// Note: This also depends on the Vector2 string representation.
	CHECK_MESSAGE(
			String(Rect2(0, 100, 1280, 720)) == "[P: (0.0, 100.0), S: (1280.0, 720.0)]",
			"The string representation should match the expected value.");
}

TEST_CASE("[Rect2] Basic getters") {
	constexpr Rect2 rect = Rect2(0, 100, 1280, 720);
	CHECK_MESSAGE(
			rect.get_position().is_equal_approx(Vector2(0, 100)),
			"get_position() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_size().is_equal_approx(Vector2(1280, 720)),
			"get_size() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_end().is_equal_approx(Vector2(1280, 820)),
			"get_end() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_center().is_equal_approx(Vector2(640, 460)),
			"get_center() should return the expected value.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1281, 721).get_center().is_equal_approx(Vector2(640.5, 460.5)),
			"get_center() should return the expected value.");
}

TEST_CASE("[Rect2] Basic setters") {
	Rect2 rect = Rect2(0, 100, 1280, 720);
	rect.set_end(Vector2(4000, 4000));
	CHECK_MESSAGE(
			rect.is_equal_approx(Rect2(0, 100, 4000, 3900)),
			"set_end() should result in the expected Rect2.");

	rect = Rect2(0, 100, 1280, 720);
	rect.set_position(Vector2(4000, 4000));
	CHECK_MESSAGE(
			rect.is_equal_approx(Rect2(4000, 4000, 1280, 720)),
			"set_position() should result in the expected Rect2.");

	rect = Rect2(0, 100, 1280, 720);
	rect.set_size(Vector2(4000, 4000));
	CHECK_MESSAGE(
			rect.is_equal_approx(Rect2(0, 100, 4000, 4000)),
			"set_size() should result in the expected Rect2.");
}

TEST_CASE("[Rect2] Area getters") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).get_area() == doctest::Approx(921'600),
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2(0, 100, -1280, -720).get_area() == doctest::Approx(921'600),
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, -720).get_area() == doctest::Approx(-921'600),
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2(0, 100, -1280, 720).get_area() == doctest::Approx(-921'600),
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Math::is_zero_approx(Rect2(0, 100, 0, 720).get_area()),
			"get_area() should return the expected value.");

	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).has_area(),
			"has_area() should return the expected value on Rect2 with an area.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 0, 500).has_area(),
			"has_area() should return the expected value on Rect2 with no area.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 500, 0).has_area(),
			"has_area() should return the expected value on Rect2 with no area.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 0, 0).has_area(),
			"has_area() should return the expected value on Rect2 with no area.");
}

TEST_CASE("[Rect2] Absolute coordinates") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).abs().is_equal_approx(Rect2(0, 100, 1280, 720)),
			"abs() should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, -100, 1280, 720).abs().is_equal_approx(Rect2(0, -100, 1280, 720)),
			"abs() should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, -100, -1280, -720).abs().is_equal_approx(Rect2(-1280, -820, 1280, 720)),
			"abs() should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, 100, -1280, 720).abs().is_equal_approx(Rect2(-1280, 100, 1280, 720)),
			"abs() should return the expected Rect2.");
}

TEST_CASE("[Rect2] Intersection") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).intersection(Rect2(0, 300, 100, 100)).is_equal_approx(Rect2(0, 300, 100, 100)),
			"intersection() with fully enclosed Rect2 should return the expected result.");
	// The resulting Rect2 is 100 pixels high because the first Rect2 is vertically offset by 100 pixels.
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).intersection(Rect2(1200, 700, 100, 100)).is_equal_approx(Rect2(1200, 700, 80, 100)),
			"intersection() with partially enclosed Rect2 should return the expected result.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).intersection(Rect2(-4000, -4000, 100, 100)).is_equal_approx(Rect2()),
			"intersection() with non-enclosed Rect2 should return the expected result.");
}

TEST_CASE("[Rect2] Intersection with the transformed rect") {
	Rect2 rect = Rect2(0, 100, 400, 300);
	SUBCASE("Translated") {
		Transform2D xform = Transform2D(0.0f, Point2(100, 100));

		SUBCASE("intersection_transformed() with fully enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, 100, 100, 100)).is_equal_approx(Rect2(100, 200, 100, 100)));
		}
		SUBCASE("intersection_transformed() with partially enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(250, 150, 100, 100)).is_equal_approx(Rect2(350, 250, 50, 100)));
		}
		SUBCASE("intersection_transformed() with non-enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(350, 350, 100, 100)).is_equal_approx(Rect2()));
		}
		SUBCASE("intersection_transformed when fully enclosed") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-150, -50, 500, 400)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-150, -50, 500, 400)).is_equal_approx(rect.intersection(xform.xform(Rect2(-150, -50, 500, 400)))));
		}
		SUBCASE("intersection_transformed with no enclosed point") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -150, 100, 500)).is_equal_approx(Rect2(100, 100, 100, 300)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -150, 100, 500)).is_equal_approx(rect.intersection(xform.xform(Rect2(0, -150, 100, 500)))));
		}
	}
	SUBCASE("Scaled") {
		Transform2D xform = Transform2D(0.0f, Size2(2.0f, 2.0f), 0.0f, Point2(0, 0));

		SUBCASE("intersection_transformed() with fully enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(50, 75, 100, 100)).is_equal_approx(Rect2(100, 150, 200, 200)));
		}
		SUBCASE("intersection_transformed() with partially enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(175, 150, 100, 100)).is_equal_approx(Rect2(350, 300, 50, 100)));
		}
		SUBCASE("intersection_transformed() with non-enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(250, 220, 100, 100)).is_equal_approx(Rect2()));
		}
		SUBCASE("intersection_transformed when fully enclosed") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-100, -50, 500, 400)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-100, -50, 500, 400)).is_equal_approx(rect.intersection(xform.xform(Rect2(-100, -50, 500, 400)))));
		}
	}
	SUBCASE("Rotated") {
		Transform2D xform = Transform2D(Math::PI / 4, Point2());

		SUBCASE("intersection_transformed() with fully enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(100, 50, 20, 20)).is_equal_approx(Rect2(15 * Math::SQRT2, 75 * Math::SQRT2, 20 * Math::SQRT2, 20 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(100, 50, 20, 20)).is_equal_approx(rect.intersection(xform.xform(Rect2(100, 50, 20, 20)))));
		}
		SUBCASE("intersection_transformed() with partially enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(80, 90, 20, 20)).is_equal_approx(Rect2(0, 90 * Math::SQRT2, 5 * Math::SQRT2, 10 * Math::SQRT2)));
			CHECK_UNARY_FALSE(rect.intersection_transformed(xform, Rect2(80, 90, 20, 20)).is_equal_approx(rect.intersection(xform.xform(Rect2(80, 90, 20, 20)))));
		}
		SUBCASE("intersection_transformed() with non-enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(575, -50, 100, 100)).is_equal_approx(Rect2()));
			CHECK_UNARY_FALSE(rect.intersection_transformed(xform, Rect2(575, -50, 100, 100)).is_equal_approx(rect.intersection(xform.xform(Rect2(575, -50, 100, 100)))));
		}
		SUBCASE("intersection_transformed when fully enclosed") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -250, 600, 600)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -250, 600, 600)).is_equal_approx(rect.intersection(xform.xform(Rect2(0, -250, 600, 600)))));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -250, 1000, 1000)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, -250, 1000, 1000)).is_equal_approx(rect.intersection(xform.xform(Rect2(0, -250, 1000, 1000)))));
		}
		SUBCASE("intersection_transformed with no enclosed point") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(75, 25, 500, 10)).is_equal_approx(Rect2(100 - 35 * Math::SQRT2, 100, 300 + 10 * Math::SQRT2, 300)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-50, -100, 600, 10)).is_equal_approx(Rect2(100 + 90 * Math::SQRT2, 100, 300 - 90 * Math::SQRT2, 300 - 90 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-50, -100, 1000, 10)).is_equal_approx(Rect2(100 + 90 * Math::SQRT2, 100, 300 - 90 * Math::SQRT2, 300 - 90 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-95, -100, 600, 10)).is_equal_approx(Rect2(100 + 90 * Math::SQRT2, 100, 300 - 90 * Math::SQRT2, 300 - 90 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-95, -100, 1000, 10)).is_equal_approx(Rect2(100 + 90 * Math::SQRT2, 100, 300 - 90 * Math::SQRT2, 300 - 90 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-200, -100, 1000, 10)).is_equal_approx(Rect2(100 + 90 * Math::SQRT2, 100, 300 - 90 * Math::SQRT2, 300 - 90 * Math::SQRT2)));
		}
	}
	SUBCASE("Skewed") {
		Transform2D xform = Transform2D(0, Size2(1, 1), Math::PI / 4, Vector2(0, 0));

		SUBCASE("intersection_transformed() with fully enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(300, 200, 100, 50)).is_equal_approx(Rect2(300 - 125 * Math::SQRT2, 100 * Math::SQRT2, 100 + 25 * Math::SQRT2, 25 * Math::SQRT2)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(300, 200, 100, 50)).is_equal_approx(rect.intersection(xform.xform(Rect2(300, 200, 100, 50)))));
		}
		SUBCASE("intersection_transformed() with partially enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(550, 200, 100, 50)).is_equal_approx(Rect2(550 - 125 * Math::SQRT2, 150, -150 + 125 * Math::SQRT2, -150 + 125 * Math::SQRT2)));
			CHECK_UNARY_FALSE(rect.intersection_transformed(xform, Rect2(550, 200, 100, 50)).is_equal_approx(rect.intersection(xform.xform(Rect2(550, 200, 100, 50)))));
		}
		SUBCASE("intersection_transformed() with non-enclosed Rect2") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(810, 550, 100, 50)).is_equal_approx(Rect2()));
			CHECK_UNARY_FALSE(rect.intersection_transformed(xform, Rect2(810, 550, 100, 50)).is_equal_approx(rect.intersection(xform.xform(Rect2(810, 550, 100, 50)))));
		}
		SUBCASE("intersection_transformed when fully enclosed") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, 0, 2000, 1000)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(0, 0, 2000, 1000)).is_equal_approx(rect.intersection(xform.xform(Rect2(0, 0, 2000, 1000)))));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-3000, 0, 4000, 3000)).is_equal_approx(rect));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-3000, 0, 4000, 3000)).is_equal_approx(rect.intersection(xform.xform(Rect2(-3000, 0, 4000, 3000)))));
		}
		SUBCASE("intersection_transformed with no enclosed point") {
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(-150, 130, 600, 450)).is_equal_approx(Rect2(0, 100, 350, 300)));
			CHECK_UNARY(rect.intersection_transformed(xform, Rect2(450, 130, 600, 450)).is_equal_approx(Rect2(50, 100, 350, 300)));
		}
	}
}

TEST_CASE("[Rect2] Enclosing") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).encloses(Rect2(0, 300, 100, 100)),
			"encloses() with fully contained Rect2 should return the expected result.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 1280, 720).encloses(Rect2(1200, 700, 100, 100)),
			"encloses() with partially contained Rect2 should return the expected result.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 1280, 720).encloses(Rect2(-4000, -4000, 100, 100)),
			"encloses() with non-contained Rect2 should return the expected result.");
}

TEST_CASE("[Rect2] Expanding") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).expand(Vector2(500, 600)).is_equal_approx(Rect2(0, 100, 1280, 720)),
			"expand() with contained Vector2 should return the expected result.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).expand(Vector2(0, 0)).is_equal_approx(Rect2(0, 0, 1280, 820)),
			"expand() with non-contained Vector2 should return the expected result.");
}

TEST_CASE("[Rect2] Get support") {
	constexpr Rect2 rect = Rect2(Vector2(-1.5, 2), Vector2(4, 5));
	CHECK_MESSAGE(
			rect.get_support(Vector2(1, 0)) == Vector2(2.5, 2),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_support(Vector2(0.5, 1)) == Vector2(2.5, 7),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_support(Vector2(0.5, 1)) == Vector2(2.5, 7),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_support(Vector2(0, -1)) == Vector2(-1.5, 2),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_support(Vector2(0, -0.1)) == Vector2(-1.5, 2),
			"get_support() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_support(Vector2()) == Vector2(-1.5, 2),
			"get_support() should return the Rect2 position when given a zero vector.");
}

TEST_CASE("[Rect2] Growing") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow(100).is_equal_approx(Rect2(-100, 0, 1480, 920)),
			"grow() with positive value should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow(-100).is_equal_approx(Rect2(100, 200, 1080, 520)),
			"grow() with negative value should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow(-4000).is_equal_approx(Rect2(4000, 4100, -6720, -7280)),
			"grow() with large negative value should return the expected Rect2.");

	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow_individual(100, 200, 300, 400).is_equal_approx(Rect2(-100, -100, 1680, 1320)),
			"grow_individual() with positive values should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow_individual(-100, 200, 300, -400).is_equal_approx(Rect2(100, -100, 1480, 520)),
			"grow_individual() with positive and negative values should return the expected Rect2.");

	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow_side(SIDE_TOP, 500).is_equal_approx(Rect2(0, -400, 1280, 1220)),
			"grow_side() with positive value should return the expected Rect2.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).grow_side(SIDE_TOP, -500).is_equal_approx(Rect2(0, 600, 1280, 220)),
			"grow_side() with negative value should return the expected Rect2.");
}

TEST_CASE("[Rect2] Has point") {
	Rect2 rect = Rect2(0, 100, 1280, 720);
	CHECK_MESSAGE(
			rect.has_point(Vector2(500, 600)),
			"has_point() with contained Vector2 should return the expected result.");
	CHECK_MESSAGE(
			!rect.has_point(Vector2(0, 0)),
			"has_point() with non-contained Vector2 should return the expected result.");

	CHECK_MESSAGE(
			rect.has_point(rect.position),
			"has_point() with positive size should include `position`.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2(1, 1)),
			"has_point() with positive size should include `position + (1, 1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2(1, -1)),
			"has_point() with positive size should not include `position + (1, -1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size),
			"has_point() with positive size should not include `position + size`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size + Vector2(1, 1)),
			"has_point() with positive size should not include `position + size + (1, 1)`.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + rect.size + Vector2(-1, -1)),
			"has_point() with positive size should include `position + size + (-1, -1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size + Vector2(-1, 1)),
			"has_point() with positive size should not include `position + size + (-1, 1)`.");

	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2(0, 10)),
			"has_point() with point located on left edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2(rect.size.x, 10)),
			"has_point() with point located on right edge should return false.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2(10, 0)),
			"has_point() with point located on top edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2(10, rect.size.y)),
			"has_point() with point located on bottom edge should return false.");

	/*
	// FIXME: Disabled for now until GH-37617 is fixed one way or another.
	// More tests should then be written like for the positive size case.
	rect = Rect2(0, 100, -1280, -720);
	CHECK_MESSAGE(
			rect.has_point(rect.position),
			"has_point() with negative size should include `position`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size),
			"has_point() with negative size should not include `position + size`.");
	*/

	rect = Rect2(-4000, -200, 1280, 720);
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2(0, 10)),
			"has_point() with negative position and point located on left edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2(rect.size.x, 10)),
			"has_point() with negative position and point located on right edge should return false.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2(10, 0)),
			"has_point() with negative position and point located on top edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2(10, rect.size.y)),
			"has_point() with negative position and point located on bottom edge should return false.");
}

TEST_CASE("[Rect2] Intersection") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).intersects(Rect2(0, 300, 100, 100)),
			"intersects() with fully enclosed Rect2 should return the expected result.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).intersects(Rect2(1200, 700, 100, 100)),
			"intersects() with partially enclosed Rect2 should return the expected result.");
	CHECK_MESSAGE(
			!Rect2(0, 100, 1280, 720).intersects(Rect2(-4000, -4000, 100, 100)),
			"intersects() with non-enclosed Rect2 should return the expected result.");
}

TEST_CASE("[Rect2] Merging") {
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).merge(Rect2(0, 300, 100, 100)).is_equal_approx(Rect2(0, 100, 1280, 720)),
			"merge() with fully enclosed Rect2 should return the expected result.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).merge(Rect2(1200, 700, 100, 100)).is_equal_approx(Rect2(0, 100, 1300, 720)),
			"merge() with partially enclosed Rect2 should return the expected result.");
	CHECK_MESSAGE(
			Rect2(0, 100, 1280, 720).merge(Rect2(-4000, -4000, 100, 100)).is_equal_approx(Rect2(-4000, -4000, 5280, 4820)),
			"merge() with non-enclosed Rect2 should return the expected result.");
}

TEST_CASE("[Rect2] Finite number checks") {
	constexpr Vector2 x(0, 1);
	constexpr Vector2 infinite(Math::NaN, Math::NaN);

	CHECK_MESSAGE(
			Rect2(x, x).is_finite(),
			"Rect2 with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Rect2(infinite, x).is_finite(),
			"Rect2 with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Rect2(x, infinite).is_finite(),
			"Rect2 with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Rect2(infinite, infinite).is_finite(),
			"Rect2 with two components infinite should not be finite.");
}

} // namespace TestRect2

// === Merged from tests/core/math/test_rect2i.cpp ===
/*  test_rect2i.cpp                                                       */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestRect2i {

TEST_CASE("[Rect2i] Constructor methods") {
	constexpr Rect2i recti = Rect2i(0, 100, 1280, 720);
	constexpr Rect2i recti_vector = Rect2i(Vector2i(0, 100), Vector2i(1280, 720));
	constexpr Rect2i recti_copy_recti = Rect2i(recti);
	const Rect2i recti_copy_rect = Rect2i(Rect2(0, 100, 1280, 720));

	static_assert(
			recti == recti_vector,
			"Rect2is created with the same dimensions but by different methods should be equal.");
	static_assert(
			recti == recti_copy_recti,
			"Rect2is created with the same dimensions but by different methods should be equal.");
	CHECK_MESSAGE(
			recti == recti_copy_rect,
			"Rect2is created with the same dimensions but by different methods should be equal.");
}

TEST_CASE("[Rect2i] String conversion") {
	// Note: This also depends on the Vector2 string representation.
	CHECK_MESSAGE(
			String(Rect2i(0, 100, 1280, 720)) == "[P: (0, 100), S: (1280, 720)]",
			"The string representation should match the expected value.");
}

TEST_CASE("[Rect2i] Basic getters") {
	constexpr Rect2i rect = Rect2i(0, 100, 1280, 720);
	CHECK_MESSAGE(
			rect.get_position() == Vector2i(0, 100),
			"get_position() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_size() == Vector2i(1280, 720),
			"get_size() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_end() == Vector2i(1280, 820),
			"get_end() should return the expected value.");
	CHECK_MESSAGE(
			rect.get_center() == Vector2i(640, 460),
			"get_center() should return the expected value.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1281, 721).get_center() == Vector2i(640, 460),
			"get_center() should return the expected value.");
}

TEST_CASE("[Rect2i] Basic setters") {
	Rect2i rect = Rect2i(0, 100, 1280, 720);
	rect.set_end(Vector2i(4000, 4000));
	CHECK_MESSAGE(
			rect == Rect2i(0, 100, 4000, 3900),
			"set_end() should result in the expected Rect2i.");

	rect = Rect2i(0, 100, 1280, 720);
	rect.set_position(Vector2i(4000, 4000));
	CHECK_MESSAGE(
			rect == Rect2i(4000, 4000, 1280, 720),
			"set_position() should result in the expected Rect2i.");

	rect = Rect2i(0, 100, 1280, 720);
	rect.set_size(Vector2i(4000, 4000));
	CHECK_MESSAGE(
			rect == Rect2i(0, 100, 4000, 4000),
			"set_size() should result in the expected Rect2i.");
}

TEST_CASE("[Rect2i] Area getters") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).get_area() == 921'600,
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2i(0, 100, -1280, -720).get_area() == 921'600,
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, -720).get_area() == -921'600,
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2i(0, 100, -1280, 720).get_area() == -921'600,
			"get_area() should return the expected value.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 0, 720).get_area() == 0,
			"get_area() should return the expected value.");

	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).has_area(),
			"has_area() should return the expected value on Rect2i with an area.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 0, 500).has_area(),
			"has_area() should return the expected value on Rect2i with no area.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 500, 0).has_area(),
			"has_area() should return the expected value on Rect2i with no area.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 0, 0).has_area(),
			"has_area() should return the expected value on Rect2i with no area.");
}

TEST_CASE("[Rect2i] Absolute coordinates") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).abs() == Rect2i(0, 100, 1280, 720),
			"abs() should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, -100, 1280, 720).abs() == Rect2i(0, -100, 1280, 720),
			"abs() should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, -100, -1280, -720).abs() == Rect2i(-1280, -820, 1280, 720),
			"abs() should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, 100, -1280, 720).abs() == Rect2i(-1280, 100, 1280, 720),
			"abs() should return the expected Rect2i.");
}

TEST_CASE("[Rect2i] Intersection") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).intersection(Rect2i(0, 300, 100, 100)) == Rect2i(0, 300, 100, 100),
			"intersection() with fully enclosed Rect2i should return the expected result.");
	// The resulting Rect2i is 100 pixels high because the first Rect2i is vertically offset by 100 pixels.
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).intersection(Rect2i(1200, 700, 100, 100)) == Rect2i(1200, 700, 80, 100),
			"intersection() with partially enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).intersection(Rect2i(-4000, -4000, 100, 100)) == Rect2i(),
			"intersection() with non-enclosed Rect2i should return the expected result.");
}

TEST_CASE("[Rect2i] Enclosing") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).encloses(Rect2i(0, 300, 100, 100)),
			"encloses() with fully contained Rect2i should return the expected result.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 1280, 720).encloses(Rect2i(1200, 700, 100, 100)),
			"encloses() with partially contained Rect2i should return the expected result.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 1280, 720).encloses(Rect2i(-4000, -4000, 100, 100)),
			"encloses() with non-contained Rect2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).encloses(Rect2i(0, 100, 1280, 720)),
			"encloses() with identical Rect2i should return the expected result.");
}

TEST_CASE("[Rect2i] Expanding") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).expand(Vector2i(500, 600)) == Rect2i(0, 100, 1280, 720),
			"expand() with contained Vector2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).expand(Vector2i(0, 0)) == Rect2i(0, 0, 1280, 820),
			"expand() with non-contained Vector2i should return the expected result.");
}

TEST_CASE("[Rect2i] Growing") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow(100) == Rect2i(-100, 0, 1480, 920),
			"grow() with positive value should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow(-100) == Rect2i(100, 200, 1080, 520),
			"grow() with negative value should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow(-4000) == Rect2i(4000, 4100, -6720, -7280),
			"grow() with large negative value should return the expected Rect2i.");

	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow_individual(100, 200, 300, 400) == Rect2i(-100, -100, 1680, 1320),
			"grow_individual() with positive values should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow_individual(-100, 200, 300, -400) == Rect2i(100, -100, 1480, 520),
			"grow_individual() with positive and negative values should return the expected Rect2i.");

	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow_side(SIDE_TOP, 500) == Rect2i(0, -400, 1280, 1220),
			"grow_side() with positive value should return the expected Rect2i.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).grow_side(SIDE_TOP, -500) == Rect2i(0, 600, 1280, 220),
			"grow_side() with negative value should return the expected Rect2i.");
}

TEST_CASE("[Rect2i] Has point") {
	Rect2i rect = Rect2i(0, 100, 1280, 720);
	CHECK_MESSAGE(
			rect.has_point(Vector2i(500, 600)),
			"has_point() with contained Vector2i should return the expected result.");
	CHECK_MESSAGE(
			!rect.has_point(Vector2i(0, 0)),
			"has_point() with non-contained Vector2i should return the expected result.");

	CHECK_MESSAGE(
			rect.has_point(rect.position),
			"has_point() with positive size should include `position`.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2i(1, 1)),
			"has_point() with positive size should include `position + (1, 1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2i(1, -1)),
			"has_point() with positive size should not include `position + (1, -1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size),
			"has_point() with positive size should not include `position + size`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size + Vector2i(1, 1)),
			"has_point() with positive size should not include `position + size + (1, 1)`.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + rect.size + Vector2i(-1, -1)),
			"has_point() with positive size should include `position + size + (-1, -1)`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size + Vector2i(-1, 1)),
			"has_point() with positive size should not include `position + size + (-1, 1)`.");

	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2i(0, 10)),
			"has_point() with point located on left edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2i(rect.size.x, 10)),
			"has_point() with point located on right edge should return false.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2i(10, 0)),
			"has_point() with point located on top edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2i(10, rect.size.y)),
			"has_point() with point located on bottom edge should return false.");

	/*
	// FIXME: Disabled for now until GH-37617 is fixed one way or another.
	// More tests should then be written like for the positive size case.
	rect = Rect2i(0, 100, -1280, -720);
	CHECK_MESSAGE(
			rect.has_point(rect.position),
			"has_point() with negative size should include `position`.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + rect.size),
			"has_point() with negative size should not include `position + size`.");
	*/

	rect = Rect2i(-4000, -200, 1280, 720);
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2i(0, 10)),
			"has_point() with negative position and point located on left edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2i(rect.size.x, 10)),
			"has_point() with negative position and point located on right edge should return false.");
	CHECK_MESSAGE(
			rect.has_point(rect.position + Vector2i(10, 0)),
			"has_point() with negative position and point located on top edge should return true.");
	CHECK_MESSAGE(
			!rect.has_point(rect.position + Vector2i(10, rect.size.y)),
			"has_point() with negative position and point located on bottom edge should return false.");
}

TEST_CASE("[Rect2i] Intersection") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).intersects(Rect2i(0, 300, 100, 100)),
			"intersects() with fully enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).intersects(Rect2i(1200, 700, 100, 100)),
			"intersects() with partially enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			!Rect2i(0, 100, 1280, 720).intersects(Rect2i(-4000, -4000, 100, 100)),
			"intersects() with non-enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			!Rect2i(0, 0, 2, 2).intersects(Rect2i(2, 2, 2, 2)),
			"intersects() with adjacent Rect2i should return the expected result.");
}

TEST_CASE("[Rect2i] Merging") {
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).merge(Rect2i(0, 300, 100, 100)) == Rect2i(0, 100, 1280, 720),
			"merge() with fully enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).merge(Rect2i(1200, 700, 100, 100)) == Rect2i(0, 100, 1300, 720),
			"merge() with partially enclosed Rect2i should return the expected result.");
	CHECK_MESSAGE(
			Rect2i(0, 100, 1280, 720).merge(Rect2i(-4000, -4000, 100, 100)) == Rect2i(-4000, -4000, 5280, 4820),
			"merge() with non-enclosed Rect2i should return the expected result.");
}

} // namespace TestRect2i

// === Merged from tests/core/math/test_transform_2d.cpp ===
/*  test_transform_2d.cpp                                                 */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestTransform2D {

Transform2D create_dummy_transform() {
	return Transform2D(Vector2(1, 2), Vector2(3, 4), Vector2(5, 6));
}

Transform2D identity() {
	return Transform2D();
}

TEST_CASE("[Transform2D] Default constructor") {
	Transform2D default_constructor = Transform2D();
	CHECK(default_constructor == Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(0, 0)));
}

TEST_CASE("[Transform2D] Copy constructor") {
	Transform2D T = create_dummy_transform();
	Transform2D copy_constructor = Transform2D(T);
	CHECK(T == copy_constructor);
}

TEST_CASE("[Transform2D] Constructor from angle and position") {
	constexpr float ROTATION = Math::PI / 4;
	constexpr Vector2 TRANSLATION = Vector2(20, -20);

	const Transform2D test = Transform2D(ROTATION, TRANSLATION);
	const Transform2D expected = Transform2D().rotated(ROTATION).translated(TRANSLATION);
	CHECK(test == expected);
}

TEST_CASE("[Transform2D] Constructor from angle, scale, skew and position") {
	constexpr float ROTATION = Math::PI / 2;
	constexpr Vector2 SCALE = Vector2(2, 0.5);
	constexpr float SKEW = Math::PI / 4;
	constexpr Vector2 TRANSLATION = Vector2(30, 0);

	const Transform2D test = Transform2D(ROTATION, SCALE, SKEW, TRANSLATION);
	Transform2D expected = Transform2D().scaled(SCALE).rotated(ROTATION).translated(TRANSLATION);
	expected.set_skew(SKEW);

	CHECK(test.is_equal_approx(expected));
}

TEST_CASE("[Transform2D] Constructor from raw values") {
	constexpr Transform2D test = Transform2D(1, 2, 3, 4, 5, 6);
	constexpr Transform2D expected = Transform2D(Vector2(1, 2), Vector2(3, 4), Vector2(5, 6));
	static_assert(test == expected);
}

TEST_CASE("[Transform2D] xform") {
	constexpr Vector2 v = Vector2(2, 3);
	constexpr Transform2D T = Transform2D(Vector2(1, 2), Vector2(3, 4), Vector2(5, 6));
	constexpr Vector2 expected = Vector2(1 * 2 + 3 * 3 + 5 * 1, 2 * 2 + 4 * 3 + 6 * 1);
	CHECK(T.xform(v) == expected);
}

TEST_CASE("[Transform2D] Basis xform") {
	constexpr Vector2 v = Vector2(2, 2);
	constexpr Transform2D T1 = Transform2D(Vector2(1, 2), Vector2(3, 4), Vector2(0, 0));

	// Both versions should be the same when the origin is (0,0).
	CHECK(T1.basis_xform(v) == T1.xform(v));

	constexpr Transform2D T2 = Transform2D(Vector2(1, 2), Vector2(3, 4), Vector2(5, 6));

	// Each version should be different when the origin is not (0,0).
	CHECK_FALSE(T2.basis_xform(v) == T2.xform(v));
}

TEST_CASE("[Transform2D] Affine inverse") {
	const Transform2D orig = create_dummy_transform();
	const Transform2D affine_inverted = orig.affine_inverse();
	const Transform2D affine_inverted_again = affine_inverted.affine_inverse();
	CHECK(affine_inverted_again == orig);
}

TEST_CASE("[Transform2D] Orthonormalized") {
	const Transform2D T = create_dummy_transform();
	const Transform2D orthonormalized_T = T.orthonormalized();

	// Check each basis has length 1.
	CHECK(Math::is_equal_approx(orthonormalized_T[0].length_squared(), 1));
	CHECK(Math::is_equal_approx(orthonormalized_T[1].length_squared(), 1));

	const Vector2 vx = Vector2(orthonormalized_T[0].x, orthonormalized_T[1].x);
	const Vector2 vy = Vector2(orthonormalized_T[0].y, orthonormalized_T[1].y);

	// Check the basis are orthogonal.
	CHECK(Math::is_equal_approx(orthonormalized_T.tdotx(vx), 1));
	CHECK(Math::is_equal_approx(orthonormalized_T.tdotx(vy), 0));
	CHECK(Math::is_equal_approx(orthonormalized_T.tdoty(vx), 0));
	CHECK(Math::is_equal_approx(orthonormalized_T.tdoty(vy), 1));
}

TEST_CASE("[Transform2D] translation") {
	constexpr Vector2 offset = Vector2(1, 2);

	// Both versions should give the same result applied to identity.
	CHECK(identity().translated(offset) == identity().translated_local(offset));

	// Check both versions against left and right multiplications.
	const Transform2D orig = create_dummy_transform();
	const Transform2D T = identity().translated(offset);
	CHECK(orig.translated(offset) == T * orig);
	CHECK(orig.translated_local(offset) == orig * T);
}

TEST_CASE("[Transform2D] scaling") {
	constexpr Vector2 scaling = Vector2(1, 2);

	// Both versions should give the same result applied to identity.
	CHECK(identity().scaled(scaling) == identity().scaled_local(scaling));

	// Check both versions against left and right multiplications.
	const Transform2D orig = create_dummy_transform();
	const Transform2D S = identity().scaled(scaling);
	CHECK(orig.scaled(scaling) == S * orig);
	CHECK(orig.scaled_local(scaling) == orig * S);
}

TEST_CASE("[Transform2D] rotation") {
	constexpr real_t phi = 1.0;

	// Both versions should give the same result applied to identity.
	CHECK(identity().rotated(phi) == identity().rotated_local(phi));

	// Check both versions against left and right multiplications.
	const Transform2D orig = create_dummy_transform();
	const Transform2D R = identity().rotated(phi);
	CHECK(orig.rotated(phi) == R * orig);
	CHECK(orig.rotated_local(phi) == orig * R);
}

TEST_CASE("[Transform2D] Interpolation") {
	const Transform2D rotate_scale_skew_pos = Transform2D(Math::deg_to_rad(170.0), Vector2(3.6, 8.0), Math::deg_to_rad(20.0), Vector2(2.4, 6.8));
	const Transform2D rotate_scale_skew_pos_halfway = Transform2D(Math::deg_to_rad(85.0), Vector2(2.3, 4.5), Math::deg_to_rad(10.0), Vector2(1.2, 3.4));
	Transform2D interpolated = Transform2D().interpolate_with(rotate_scale_skew_pos, 0.5);
	CHECK(interpolated.get_origin().is_equal_approx(rotate_scale_skew_pos_halfway.get_origin()));
	CHECK(interpolated.get_rotation() == doctest::Approx(rotate_scale_skew_pos_halfway.get_rotation()));
	CHECK(interpolated.get_scale().is_equal_approx(rotate_scale_skew_pos_halfway.get_scale()));
	CHECK(interpolated.get_skew() == doctest::Approx(rotate_scale_skew_pos_halfway.get_skew()));
	CHECK(interpolated.is_equal_approx(rotate_scale_skew_pos_halfway));
	interpolated = rotate_scale_skew_pos.interpolate_with(Transform2D(), 0.5);
	CHECK(interpolated.is_equal_approx(rotate_scale_skew_pos_halfway));
}

TEST_CASE("[Transform2D] Finite number checks") {
	constexpr Vector2 x = Vector2(0, 1);
	constexpr Vector2 infinite = Vector2(Math::NaN, Math::NaN);

	CHECK_MESSAGE(
			Transform2D(x, x, x).is_finite(),
			"Transform2D with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Transform2D(infinite, x, x).is_finite(),
			"Transform2D with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Transform2D(x, infinite, x).is_finite(),
			"Transform2D with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Transform2D(x, x, infinite).is_finite(),
			"Transform2D with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Transform2D(infinite, infinite, x).is_finite(),
			"Transform2D with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Transform2D(infinite, x, infinite).is_finite(),
			"Transform2D with two components infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Transform2D(x, infinite, infinite).is_finite(),
			"Transform2D with two components infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Transform2D(infinite, infinite, infinite).is_finite(),
			"Transform2D with three components infinite should not be finite.");
}

TEST_CASE("[Transform2D] Is conformal checks") {
	CHECK_MESSAGE(
			Transform2D().is_conformal(),
			"Identity Transform2D should be conformal.");

	CHECK_MESSAGE(
			Transform2D(1.2, Vector2()).is_conformal(),
			"Transform2D with only rotation should be conformal.");

	CHECK_MESSAGE(
			Transform2D(Vector2(1, 0), Vector2(0, -1), Vector2()).is_conformal(),
			"Transform2D with only a flip should be conformal.");

	CHECK_MESSAGE(
			Transform2D(Vector2(1.2, 0), Vector2(0, 1.2), Vector2()).is_conformal(),
			"Transform2D with only uniform scale should be conformal.");

	CHECK_MESSAGE(
			Transform2D(Vector2(1.2, 3.4), Vector2(3.4, -1.2), Vector2()).is_conformal(),
			"Transform2D with a flip, rotation, and uniform scale should be conformal.");

	CHECK_FALSE_MESSAGE(
			Transform2D(Vector2(1.2, 0), Vector2(0, 3.4), Vector2()).is_conformal(),
			"Transform2D with non-uniform scale should not be conformal.");

	CHECK_FALSE_MESSAGE(
			Transform2D(Vector2(Math::SQRT12, Math::SQRT12), Vector2(0, 1), Vector2()).is_conformal(),
			"Transform2D with the X axis skewed 45 degrees should not be conformal.");
}

} // namespace TestTransform2D

// === Merged from tests/core/math/test_transform_3d.cpp ===
/*  test_transform_3d.cpp                                                 */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestTransform3D {

Transform3D create_dummy_transform() {
	return Transform3D(Basis(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9)), Vector3(10, 11, 12));
}

Transform3D identity() {
	return Transform3D();
}

TEST_CASE("[Transform3D] translation") {
	constexpr Vector3 offset = Vector3(1, 2, 3);

	// Both versions should give the same result applied to identity.
	CHECK(identity().translated(offset) == identity().translated_local(offset));

	// Check both versions against left and right multiplications.
	Transform3D orig = create_dummy_transform();
	Transform3D T = identity().translated(offset);
	CHECK(orig.translated(offset) == T * orig);
	CHECK(orig.translated_local(offset) == orig * T);
}

TEST_CASE("[Transform3D] scaling") {
	constexpr Vector3 scaling = Vector3(1, 2, 3);

	// Both versions should give the same result applied to identity.
	CHECK(identity().scaled(scaling) == identity().scaled_local(scaling));

	// Check both versions against left and right multiplications.
	Transform3D orig = create_dummy_transform();
	Transform3D S = identity().scaled(scaling);
	CHECK(orig.scaled(scaling) == S * orig);
	CHECK(orig.scaled_local(scaling) == orig * S);
}

TEST_CASE("[Transform3D] rotation") {
	const Vector3 axis = Vector3(1, 2, 3).normalized();
	constexpr real_t phi = 1.0;

	// Both versions should give the same result applied to identity.
	CHECK(identity().rotated(axis, phi) == identity().rotated_local(axis, phi));

	// Check both versions against left and right multiplications.
	Transform3D orig = create_dummy_transform();
	Transform3D R = identity().rotated(axis, phi);
	CHECK(orig.rotated(axis, phi) == R * orig);
	CHECK(orig.rotated_local(axis, phi) == orig * R);
}

TEST_CASE("[Transform3D] Finite number checks") {
	constexpr Vector3 y(0, 1, 2);
	constexpr Vector3 infinite_vec(Math::NaN, Math::NaN, Math::NaN);
	constexpr Basis x(y, y, y);
	constexpr Basis infinite_basis(infinite_vec, infinite_vec, infinite_vec);

	CHECK_MESSAGE(
			Transform3D(x, y).is_finite(),
			"Transform3D with all components finite should be finite");

	CHECK_FALSE_MESSAGE(
			Transform3D(x, infinite_vec).is_finite(),
			"Transform3D with one component infinite should not be finite.");
	CHECK_FALSE_MESSAGE(
			Transform3D(infinite_basis, y).is_finite(),
			"Transform3D with one component infinite should not be finite.");

	CHECK_FALSE_MESSAGE(
			Transform3D(infinite_basis, infinite_vec).is_finite(),
			"Transform3D with two components infinite should not be finite.");
}

TEST_CASE("[Transform3D] Rotate around global origin") {
	// Start with the default orientation, but not centered on the origin.
	// Rotating should rotate both our basis and the origin.
	Transform3D transform = Transform3D();
	transform.origin = Vector3(0, 0, 1);

	Transform3D expected = Transform3D();
	expected.origin = Vector3(0, 0, -1);
	expected.basis[0] = Vector3(-1, 0, 0);
	expected.basis[2] = Vector3(0, 0, -1);

	const Transform3D rotated_transform = transform.rotated(Vector3(0, 1, 0), Math::PI);
	CHECK_MESSAGE(rotated_transform.is_equal_approx(expected), "The rotated transform should have a new orientation and basis.");
}

TEST_CASE("[Transform3D] Rotate in-place (local rotation)") {
	// Start with the default orientation.
	// Local rotation should not change the origin, only the basis.
	Transform3D transform = Transform3D();
	transform.origin = Vector3(1, 2, 3);

	Transform3D expected = Transform3D();
	expected.origin = Vector3(1, 2, 3);
	expected.basis[0] = Vector3(-1, 0, 0);
	expected.basis[2] = Vector3(0, 0, -1);

	const Transform3D rotated_transform = Transform3D(transform.rotated_local(Vector3(0, 1, 0), Math::PI));
	CHECK_MESSAGE(rotated_transform.is_equal_approx(expected), "The rotated transform should have a new orientation but still be based on the same origin.");
}

} // namespace TestTransform3D

// === Merged from tests/core/math/test_triangle_mesh.cpp ===
/*  test_triangle_mesh.cpp                                                */
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

#include "tests/test_macros.h"



#ifndef _3D_DISABLED

#include "core/math/geometry_3d.h"
#include "scene/resources/mesh.h"

namespace TestTriangleMesh {

TEST_CASE("[SceneTree][TriangleMesh] BVH creation and intersection") {
	Ref<PrimitiveMesh> box_mesh;
	box_mesh.instantiate(PrimitiveMesh::MESH_BOX);

	const Vector<Face3> faces = box_mesh->get_faces();

	Ref<TriangleMesh> triangle_mesh;
	triangle_mesh.instantiate();
	CHECK(triangle_mesh->create_from_faces(Variant(faces)));

	const Vector3 begin = Vector3(0.0, 2.0, 0.0);
	const Vector3 end = Vector3(0.0, -2.0, 0.0);

	{
		Vector3 point;
		Vector3 normal;
		int32_t *surf_index = nullptr;
		int32_t face_index = -1;
		const bool has_result = triangle_mesh->intersect_segment(begin, end, point, normal, surf_index, &face_index);
		CHECK(has_result);
		CHECK(point.is_equal_approx(Vector3(0.0, 0.5, 0.0)));
		CHECK(normal.is_equal_approx(Vector3(0.0, 1.0, 0.0)));
		CHECK(surf_index == nullptr);
		REQUIRE(face_index != -1);
		CHECK(face_index == 8);
	}

	{
		Vector3 dir = begin.direction_to(end);
		Vector3 point;
		Vector3 normal;
		int32_t *surf_index = nullptr;
		int32_t face_index = -1;
		const bool has_result = triangle_mesh->intersect_ray(begin, dir, point, normal, surf_index, &face_index);
		CHECK(has_result);
		CHECK(point.is_equal_approx(Vector3(0.0, 0.5, 0.0)));
		CHECK(normal.is_equal_approx(Vector3(0.0, 1.0, 0.0)));
		CHECK(surf_index == nullptr);
		REQUIRE(face_index != -1);
		CHECK(face_index == 8);
	}
}

} // namespace TestTriangleMesh

#endif // _3D_DISABLED

// === Merged from tests/core/math/test_vector2.cpp ===
/*  test_vector2.cpp                                                      */
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

#include "tests/test_macros.h"



#include "core/math/vector2.h"
#include "core/templates/hashfuncs.h"

namespace TestVector2 {

TEST_CASE("[Vector2] Constructor methods") {
	constexpr Vector2 vector_empty = Vector2();
	constexpr Vector2 vector_zero = Vector2(0.0, 0.0);
	static_assert(
			vector_empty == vector_zero,
			"Vector2 Constructor with no inputs should return a zero Vector2.");
}

TEST_CASE("[Vector2] Angle methods") {
	constexpr Vector2 vector_x = Vector2(1, 0);
	constexpr Vector2 vector_y = Vector2(0, 1);
	CHECK_MESSAGE(
			vector_x.angle_to(vector_y) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector2 angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_y.angle_to(vector_x) == doctest::Approx((real_t)-Math::TAU / 4),
			"Vector2 angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_x.angle_to_point(vector_y) == doctest::Approx((real_t)Math::TAU * 3 / 8),
			"Vector2 angle_to_point should work as expected.");
	CHECK_MESSAGE(
			vector_y.angle_to_point(vector_x) == doctest::Approx((real_t)-Math::TAU / 8),
			"Vector2 angle_to_point should work as expected.");
}

TEST_CASE("[Vector2] Axis methods") {
	Vector2 vector = Vector2(1.2, 3.4);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector2::Axis::AXIS_Y,
			"Vector2 max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector2::Axis::AXIS_X,
			"Vector2 min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == (real_t)1.2,
			"Vector2 array operator should work as expected.");
	vector[Vector2::Axis::AXIS_Y] = 3.7;
	CHECK_MESSAGE(
			vector[Vector2::Axis::AXIS_Y] == (real_t)3.7,
			"Vector2 array operator setter should work as expected.");
}

TEST_CASE("[Vector2] Interpolation methods") {
	constexpr Vector2 vector1 = Vector2(1, 2);
	constexpr Vector2 vector2 = Vector2(4, 5);
	CHECK_MESSAGE(
			vector1.lerp(vector2, 0.5) == Vector2(2.5, 3.5),
			"Vector2 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.lerp(vector2, 1.0 / 3.0).is_equal_approx(Vector2(2, 3)),
			"Vector2 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.normalized().slerp(vector2.normalized(), 0.5).is_equal_approx(Vector2(0.538953602313995361, 0.84233558177947998)),
			"Vector2 slerp should work as expected.");
	CHECK_MESSAGE(
			vector1.normalized().slerp(vector2.normalized(), 1.0 / 3.0).is_equal_approx(Vector2(0.508990883827209473, 0.860771894454956055)),
			"Vector2 slerp should work as expected.");
	CHECK_MESSAGE(
			Vector2(5, 0).slerp(Vector2(0, 5), 0.5).is_equal_approx(Vector2(5, 5) * Math::SQRT12),
			"Vector2 slerp with non-normalized values should work as expected.");
	CHECK_MESSAGE(
			Vector2(1, 1).slerp(Vector2(2, 2), 0.5).is_equal_approx(Vector2(1.5, 1.5)),
			"Vector2 slerp with colinear inputs should behave as expected.");
	CHECK_MESSAGE(
			Vector2().slerp(Vector2(), 0.5) == Vector2(),
			"Vector2 slerp with both inputs as zero vectors should return a zero vector.");
	CHECK_MESSAGE(
			Vector2().slerp(Vector2(1, 1), 0.5) == Vector2(0.5, 0.5),
			"Vector2 slerp with one input as zero should behave like a regular lerp.");
	CHECK_MESSAGE(
			Vector2(1, 1).slerp(Vector2(), 0.5) == Vector2(0.5, 0.5),
			"Vector2 slerp with one input as zero should behave like a regular lerp.");
	CHECK_MESSAGE(
			Vector2(4, 6).slerp(Vector2(8, 10), 0.5).is_equal_approx(Vector2(5.9076470794008017626, 8.07918879020090480697)),
			"Vector2 slerp should work as expected.");
	CHECK_MESSAGE(
			vector1.slerp(vector2, 0.5).length() == doctest::Approx((real_t)4.31959610746631919),
			"Vector2 slerp with different length input should return a vector with an interpolated length.");
	CHECK_MESSAGE(
			vector1.angle_to(vector1.slerp(vector2, 0.5)) * 2 == doctest::Approx(vector1.angle_to(vector2)),
			"Vector2 slerp with different length input should return a vector with an interpolated angle.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector2(), Vector2(7, 7), 0.5) == Vector2(2.375, 3.5),
			"Vector2 cubic_interpolate should work as expected.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector2(), Vector2(7, 7), 1.0 / 3.0).is_equal_approx(Vector2(1.851851940155029297, 2.962963104248046875)),
			"Vector2 cubic_interpolate should work as expected.");
	CHECK_MESSAGE(
			Vector2(1, 0).move_toward(Vector2(10, 0), 3) == Vector2(4, 0),
			"Vector2 move_toward should work as expected.");
}

TEST_CASE("[Vector2] Length methods") {
	constexpr Vector2 vector1 = Vector2(10, 10);
	constexpr Vector2 vector2 = Vector2(20, 30);
	CHECK_MESSAGE(
			vector1.length_squared() == 200,
			"Vector2 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(10 * (real_t)Math::SQRT2),
			"Vector2 length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 1300,
			"Vector2 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx((real_t)36.05551275463989293119),
			"Vector2 length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == 500,
			"Vector2 distance_squared_to should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx((real_t)22.36067977499789696409),
			"Vector2 distance_to should work as expected.");
}

TEST_CASE("[Vector2] Limiting methods") {
	constexpr Vector2 vector = Vector2(10, 10);
	CHECK_MESSAGE(
			vector.limit_length().is_equal_approx(Vector2(Math::SQRT12, Math::SQRT12)),
			"Vector2 limit_length should work as expected.");
	CHECK_MESSAGE(
			vector.limit_length(5).is_equal_approx(5 * Vector2(Math::SQRT12, Math::SQRT12)),
			"Vector2 limit_length should work as expected.");

	CHECK_MESSAGE(
			Vector2(-5, 15).clamp(Vector2(), vector).is_equal_approx(Vector2(0, 10)),
			"Vector2 clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector2(0, 15), Vector2(5, 20)).is_equal_approx(Vector2(5, 15)),
			"Vector2 clamp should work as expected.");
}

TEST_CASE("[Vector2] Normalization methods") {
	CHECK_MESSAGE(
			Vector2(1, 0).is_normalized() == true,
			"Vector2 is_normalized should return true for a normalized vector.");
	CHECK_MESSAGE(
			Vector2(1, 1).is_normalized() == false,
			"Vector2 is_normalized should return false for a non-normalized vector.");
	CHECK_MESSAGE(
			Vector2(1, 0).normalized() == Vector2(1, 0),
			"Vector2 normalized should return the same vector for a normalized vector.");
	CHECK_MESSAGE(
			Vector2(1, 1).normalized().is_equal_approx(Vector2(Math::SQRT12, Math::SQRT12)),
			"Vector2 normalized should work as expected.");

	Vector2 vector = Vector2(3.2, -5.4);
	vector.normalize();
	CHECK_MESSAGE(
			vector == Vector2(3.2, -5.4).normalized(),
			"Vector2 normalize should convert same way as Vector2 normalized.");
	CHECK_MESSAGE(
			vector.is_equal_approx(Vector2(0.509802390301732898898, -0.860291533634174266891)),
			"Vector2 normalize should work as expected.");
}

TEST_CASE("[Vector2] Operators") {
	constexpr Vector2 decimal1 = Vector2(2.3, 4.9);
	constexpr Vector2 decimal2 = Vector2(1.2, 3.4);
	constexpr Vector2 power1 = Vector2(0.75, 1.5);
	constexpr Vector2 power2 = Vector2(0.5, 0.125);
	constexpr Vector2 int1 = Vector2(4, 5);
	constexpr Vector2 int2 = Vector2(1, 2);

	CHECK_MESSAGE(
			(decimal1 + decimal2).is_equal_approx(Vector2(3.5, 8.3)),
			"Vector2 addition should behave as expected.");
	static_assert(
			(power1 + power2) == Vector2(1.25, 1.625),
			"Vector2 addition with powers of two should give exact results.");
	static_assert(
			(int1 + int2) == Vector2(5, 7),
			"Vector2 addition with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 - decimal2).is_equal_approx(Vector2(1.1, 1.5)),
			"Vector2 subtraction should behave as expected.");
	static_assert(
			(power1 - power2) == Vector2(0.25, 1.375),
			"Vector2 subtraction with powers of two should give exact results.");
	static_assert(
			(int1 - int2) == Vector2(3, 3),
			"Vector2 subtraction with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * decimal2).is_equal_approx(Vector2(2.76, 16.66)),
			"Vector2 multiplication should behave as expected.");
	static_assert(
			(power1 * power2) == Vector2(0.375, 0.1875),
			"Vector2 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * int2) == Vector2(4, 10),
			"Vector2 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / decimal2).is_equal_approx(Vector2(1.91666666666666666, 1.44117647058823529)),
			"Vector2 division should behave as expected.");
	static_assert(
			(power1 / power2) == Vector2(1.5, 12.0),
			"Vector2 division with powers of two should give exact results.");
	static_assert(
			(int1 / int2) == Vector2(4, 2.5),
			"Vector2 division with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * 2).is_equal_approx(Vector2(4.6, 9.8)),
			"Vector2 multiplication should behave as expected.");
	static_assert(
			(power1 * 2) == Vector2(1.5, 3),
			"Vector2 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * 2) == Vector2(8, 10),
			"Vector2 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / 2).is_equal_approx(Vector2(1.15, 2.45)),
			"Vector2 division should behave as expected.");
	static_assert(
			(power1 / 2) == Vector2(0.375, 0.75),
			"Vector2 division with powers of two should give exact results.");
	static_assert(
			(int1 / 2) == Vector2(2, 2.5),
			"Vector2 division with integers should give exact results.");

	CHECK_MESSAGE(
			((Vector2i)decimal1) == Vector2i(2, 4),
			"Vector2 cast to Vector2i should work as expected.");
	CHECK_MESSAGE(
			((Vector2i)decimal2) == Vector2i(1, 3),
			"Vector2 cast to Vector2i should work as expected.");
	CHECK_MESSAGE(
			Vector2(Vector2i(1, 2)) == Vector2(1, 2),
			"Vector2 constructed from Vector2i should work as expected.");

	CHECK_MESSAGE(
			((String)decimal1) == "(2.3, 4.9)",
			"Vector2 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)decimal2) == "(1.2, 3.4)",
			"Vector2 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)Vector2(9.8, 9.9)) == "(9.8, 9.9)",
			"Vector2 cast to String should work as expected.");
#ifdef REAL_T_IS_DOUBLE
	CHECK_MESSAGE(
			((String)Vector2(Math::PI, Math::TAU)) == "(3.14159265358979, 6.28318530717959)",
			"Vector2 cast to String should print the correct amount of digits for real_t = double.");
#else
	CHECK_MESSAGE(
			((String)Vector2(Math::PI, Math::TAU)) == "(3.141593, 6.283185)",
			"Vector2 cast to String should print the correct amount of digits for real_t = float.");
#endif // REAL_T_IS_DOUBLE
}

TEST_CASE("[Vector2] Other methods") {
	constexpr Vector2 vector = Vector2(1.2, 3.4);
	CHECK_MESSAGE(
			vector.aspect() == doctest::Approx((real_t)1.2 / (real_t)3.4),
			"Vector2 aspect should work as expected.");

	CHECK_MESSAGE(
			vector.direction_to(Vector2()).is_equal_approx(-vector.normalized()),
			"Vector2 direction_to should work as expected.");
	CHECK_MESSAGE(
			Vector2(1, 1).direction_to(Vector2(2, 2)).is_equal_approx(Vector2(Math::SQRT12, Math::SQRT12)),
			"Vector2 direction_to should work as expected.");

	CHECK_MESSAGE(
			vector.posmod(2).is_equal_approx(Vector2(1.2, 1.4)),
			"Vector2 posmod should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmod(2).is_equal_approx(Vector2(0.8, 0.6)),
			"Vector2 posmod should work as expected.");
	CHECK_MESSAGE(
			vector.posmodv(Vector2(1, 2)).is_equal_approx(Vector2(0.2, 1.4)),
			"Vector2 posmodv should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmodv(Vector2(2, 3)).is_equal_approx(Vector2(0.8, 2.6)),
			"Vector2 posmodv should work as expected.");

	CHECK_MESSAGE(
			vector.rotated(Math::TAU).is_equal_approx(Vector2(1.2, 3.4)),
			"Vector2 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Math::TAU / 4).is_equal_approx(Vector2(-3.4, 1.2)),
			"Vector2 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Math::TAU / 3).is_equal_approx(Vector2(-3.544486372867091398996, -0.660769515458673623883)),
			"Vector2 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Math::TAU / 2).is_equal_approx(vector.rotated(Math::TAU / -2)),
			"Vector2 rotated should work as expected.");

	CHECK_MESSAGE(
			vector.snapped(Vector2(1, 1)) == Vector2(1, 3),
			"Vector2 snapped to integers should be the same as rounding.");
	CHECK_MESSAGE(
			Vector2(3.4, 5.6).snapped(Vector2(1, 1)) == Vector2(3, 6),
			"Vector2 snapped to integers should be the same as rounding.");
	CHECK_MESSAGE(
			vector.snapped(Vector2(0.25, 0.25)) == Vector2(1.25, 3.5),
			"Vector2 snapped to 0.25 should give exact results.");

	CHECK_MESSAGE(
			Vector2(1.2, 2.5).is_equal_approx(vector.min(Vector2(3.0, 2.5))),
			"Vector2 min should return expected value.");

	CHECK_MESSAGE(
			Vector2(5.3, 3.4).is_equal_approx(vector.max(Vector2(5.3, 2.0))),
			"Vector2 max should return expected value.");
}

TEST_CASE("[Vector2] Plane methods") {
	constexpr Vector2 vector = Vector2(1.2, 3.4);
	constexpr Vector2 vector_y = Vector2(0, 1);
	constexpr Vector2 vector_normal = Vector2(0.95879811270838721622267, 0.2840883296913739899919);
	constexpr real_t p_d = 99.1;
	CHECK_MESSAGE(
			vector.bounce(vector_y) == Vector2(1.2, -3.4),
			"Vector2 bounce on a plane with normal of the Y axis should.");
	CHECK_MESSAGE(
			vector.bounce(vector_normal).is_equal_approx(Vector2(-2.85851197982345523329, 2.197477931904161412358)),
			"Vector2 bounce with normal should return expected value.");
	CHECK_MESSAGE(
			vector.reflect(vector_y) == Vector2(-1.2, 3.4),
			"Vector2 reflect on a plane with normal of the Y axis should.");
	CHECK_MESSAGE(
			vector.reflect(vector_normal).is_equal_approx(Vector2(2.85851197982345523329, -2.197477931904161412358)),
			"Vector2 reflect with normal should return expected value.");
	CHECK_MESSAGE(
			vector.project(vector_y) == Vector2(0, 3.4),
			"Vector2 projected on the Y axis should only give the Y component.");
	CHECK_MESSAGE(
			vector.project(vector_normal).is_equal_approx(Vector2(2.0292559899117276166, 0.60126103404791929382)),
			"Vector2 projected on a normal should return expected value.");
	CHECK_MESSAGE(
			vector_normal.plane_project(p_d, vector).is_equal_approx(Vector2(94.187635516479631, 30.951892004882851)),
			"Vector2 plane_project should return expected value.");
	CHECK_MESSAGE(
			vector.slide(vector_y) == Vector2(1.2, 0),
			"Vector2 slide on a plane with normal of the Y axis should set the Y to zero.");
	CHECK_MESSAGE(
			vector.slide(vector_normal).is_equal_approx(Vector2(-0.8292559899117276166456, 2.798738965952080706179)),
			"Vector2 slide with normal should return expected value.");
	// There's probably a better way to test these ones?
#ifdef MATH_CHECKS
	constexpr Vector2 vector_non_normal = Vector2(5.4, 1.6);
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			vector.bounce(vector_non_normal).is_equal_approx(Vector2()),
			"Vector2 bounce should return empty Vector2 with non-normalized input.");
	CHECK_MESSAGE(
			vector.reflect(vector_non_normal).is_equal_approx(Vector2()),
			"Vector2 reflect should return empty Vector2 with non-normalized input.");
	CHECK_MESSAGE(
			vector.slide(vector_non_normal).is_equal_approx(Vector2()),
			"Vector2 slide should return empty Vector2 with non-normalized input.");
	ERR_PRINT_ON;
#endif // MATH_CHECKS
}

TEST_CASE("[Vector2] Rounding methods") {
	constexpr Vector2 vector1 = Vector2(1.2, 5.6);
	constexpr Vector2 vector2 = Vector2(1.2, -5.6);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector2 abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector2 abs should work as expected.");

	CHECK_MESSAGE(
			vector1.ceil() == Vector2(2, 6),
			"Vector2 ceil should work as expected.");
	CHECK_MESSAGE(
			vector2.ceil() == Vector2(2, -5),
			"Vector2 ceil should work as expected.");

	CHECK_MESSAGE(
			vector1.floor() == Vector2(1, 5),
			"Vector2 floor should work as expected.");
	CHECK_MESSAGE(
			vector2.floor() == Vector2(1, -6),
			"Vector2 floor should work as expected.");

	CHECK_MESSAGE(
			vector1.round() == Vector2(1, 6),
			"Vector2 round should work as expected.");
	CHECK_MESSAGE(
			vector2.round() == Vector2(1, -6),
			"Vector2 round should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector2(1, 1),
			"Vector2 sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector2(1, -1),
			"Vector2 sign should work as expected.");
}

TEST_CASE("[Vector2] Linear algebra methods") {
	constexpr Vector2 vector_x = Vector2(1, 0);
	constexpr Vector2 vector_y = Vector2(0, 1);
	constexpr Vector2 a = Vector2(3.5, 8.5);
	constexpr Vector2 b = Vector2(5.2, 4.6);
	CHECK_MESSAGE(
			vector_x.cross(vector_y) == 1,
			"Vector2 cross product of X and Y should give 1.");
	CHECK_MESSAGE(
			vector_y.cross(vector_x) == -1,
			"Vector2 cross product of Y and X should give negative 1.");
	CHECK_MESSAGE(
			a.cross(b) == doctest::Approx((real_t)-28.1),
			"Vector2 cross should return expected value.");
	CHECK_MESSAGE(
			Vector2(-a.x, a.y).cross(Vector2(b.x, -b.y)) == doctest::Approx((real_t)-28.1),
			"Vector2 cross should return expected value.");

	CHECK_MESSAGE(
			vector_x.dot(vector_y) == 0.0,
			"Vector2 dot product of perpendicular vectors should be zero.");
	CHECK_MESSAGE(
			vector_x.dot(vector_x) == 1.0,
			"Vector2 dot product of identical unit vectors should be one.");
	CHECK_MESSAGE(
			(vector_x * 10).dot(vector_x * 10) == 100.0,
			"Vector2 dot product of same direction vectors should behave as expected.");
	CHECK_MESSAGE(
			a.dot(b) == doctest::Approx((real_t)57.3),
			"Vector2 dot should return expected value.");
	CHECK_MESSAGE(
			Vector2(-a.x, a.y).dot(Vector2(b.x, -b.y)) == doctest::Approx((real_t)-57.3),
			"Vector2 dot should return expected value.");
}

TEST_CASE("[Vector2] Finite number checks") {
	constexpr double infinite[] = { Math::NaN, Math::INF, -Math::INF };

	CHECK_MESSAGE(
			Vector2(0, 1).is_finite(),
			"Vector2(0, 1) should be finite");

	for (double x : infinite) {
		CHECK_FALSE_MESSAGE(
				Vector2(x, 1).is_finite(),
				"Vector2 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector2(0, x).is_finite(),
				"Vector2 with one component infinite should not be finite.");
	}

	for (double x : infinite) {
		for (double y : infinite) {
			CHECK_FALSE_MESSAGE(
					Vector2(x, y).is_finite(),
					"Vector2 with two components infinite should not be finite.");
		}
	}
}

} // namespace TestVector2

// === Merged from tests/core/math/test_vector2i.cpp ===
/*  test_vector2i.cpp                                                     */
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

#include "tests/test_macros.h"



#include "core/math/vector2.h"
#include "core/templates/hashfuncs.h"

namespace TestVector2i {

TEST_CASE("[Vector2i] Constructor methods") {
	constexpr Vector2i vector_empty = Vector2i();
	constexpr Vector2i vector_zero = Vector2i(0, 0);
	static_assert(
			vector_empty == vector_zero,
			"Vector2i Constructor with no inputs should return a zero Vector2i.");
}

TEST_CASE("[Vector2i] Axis methods") {
	Vector2i vector = Vector2i(2, 3);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector2i::Axis::AXIS_Y,
			"Vector2i max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector2i::Axis::AXIS_X,
			"Vector2i min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == 2,
			"Vector2i array operator should work as expected.");
	vector[Vector2i::Axis::AXIS_Y] = 5;
	CHECK_MESSAGE(
			vector[Vector2i::Axis::AXIS_Y] == 5,
			"Vector2i array operator setter should work as expected.");
}

TEST_CASE("[Vector2i] Clamp method") {
	constexpr Vector2i vector = Vector2i(10, 10);
	CHECK_MESSAGE(
			Vector2i(-5, 15).clamp(Vector2i(), vector) == Vector2i(0, 10),
			"Vector2i clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector2i(0, 15), Vector2i(5, 20)) == Vector2i(5, 15),
			"Vector2i clamp should work as expected.");
}

TEST_CASE("[Vector2i] Length methods") {
	constexpr Vector2i vector1 = Vector2i(10, 10);
	constexpr Vector2i vector2 = Vector2i(20, 30);
	CHECK_MESSAGE(
			vector1.length_squared() == 200,
			"Vector2i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(10 * Math::SQRT2),
			"Vector2i length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 1300,
			"Vector2i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx(36.05551275463989293119),
			"Vector2i length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == 500,
			"Vector2i distance_squared_to should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx(22.36067977499789696409),
			"Vector2i distance_to should work as expected.");
}

TEST_CASE("[Vector2i] Operators") {
	constexpr Vector2i vector1 = Vector2i(5, 9);
	constexpr Vector2i vector2 = Vector2i(2, 3);

	static_assert(
			(vector1 + vector2) == Vector2i(7, 12),
			"Vector2i addition with integers should give exact results.");
	static_assert(
			(vector1 - vector2) == Vector2i(3, 6),
			"Vector2i subtraction with integers should give exact results.");
	static_assert(
			(vector1 * vector2) == Vector2i(10, 27),
			"Vector2i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / vector2) == Vector2i(2, 3),
			"Vector2i division with integers should give exact results.");

	static_assert(
			(vector1 * 2) == Vector2i(10, 18),
			"Vector2i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / 2) == Vector2i(2, 4),
			"Vector2i division with integers should give exact results.");

	CHECK_MESSAGE(
			((Vector2)vector1) == Vector2(5, 9),
			"Vector2i cast to Vector2 should work as expected.");
	CHECK_MESSAGE(
			((Vector2)vector2) == Vector2(2, 3),
			"Vector2i cast to Vector2 should work as expected.");
	CHECK_MESSAGE(
			Vector2i(Vector2(1.1, 2.9)) == Vector2i(1, 2),
			"Vector2i constructed from Vector2 should work as expected.");
}

TEST_CASE("[Vector2i] Other methods") {
	constexpr Vector2i vector = Vector2i(1, 3);
	CHECK_MESSAGE(
			vector.aspect() == doctest::Approx((real_t)1.0 / (real_t)3.0),
			"Vector2i aspect should work as expected.");

	CHECK_MESSAGE(
			vector.min(Vector2i(3, 2)) == Vector2i(1, 2),
			"Vector2i min should return expected value.");

	CHECK_MESSAGE(
			vector.max(Vector2i(5, 2)) == Vector2i(5, 3),
			"Vector2i max should return expected value.");

	CHECK_MESSAGE(
			vector.snapped(Vector2i(4, 2)) == Vector2i(0, 4),
			"Vector2i snapped should work as expected.");
}

TEST_CASE("[Vector2i] Abs and sign methods") {
	constexpr Vector2i vector1 = Vector2i(1, 3);
	constexpr Vector2i vector2 = Vector2i(1, -3);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector2i abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector2i abs should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector2i(1, 1),
			"Vector2i sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector2i(1, -1),
			"Vector2i sign should work as expected.");
}

} // namespace TestVector2i

// === Merged from tests/core/math/test_vector3.cpp ===
/*  test_vector3.cpp                                                      */
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

#include "tests/test_macros.h"



#include "core/math/plane.h"

namespace TestVector3 {

TEST_CASE("[Vector3] Constructor methods") {
	constexpr Vector3 vector_empty = Vector3();
	constexpr Vector3 vector_zero = Vector3(0.0, 0.0, 0.0);
	static_assert(
			vector_empty == vector_zero,
			"Vector3 Constructor with no inputs should return a zero Vector3.");
}

TEST_CASE("[Vector3] Angle methods") {
	constexpr Vector3 vector_x = Vector3(1, 0, 0);
	constexpr Vector3 vector_y = Vector3(0, 1, 0);
	constexpr Vector3 vector_yz = Vector3(0, 1, 1);
	CHECK_MESSAGE(
			vector_x.angle_to(vector_y) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector3 angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_x.angle_to(vector_yz) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector3 angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_yz.angle_to(vector_x) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector3 angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_y.angle_to(vector_yz) == doctest::Approx((real_t)Math::TAU / 8),
			"Vector3 angle_to should work as expected.");

	CHECK_MESSAGE(
			vector_x.signed_angle_to(vector_y, vector_y) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector3 signed_angle_to edge case should be positive.");
	CHECK_MESSAGE(
			vector_x.signed_angle_to(vector_yz, vector_y) == doctest::Approx((real_t)Math::TAU / -4),
			"Vector3 signed_angle_to should work as expected.");
	CHECK_MESSAGE(
			vector_yz.signed_angle_to(vector_x, vector_y) == doctest::Approx((real_t)Math::TAU / 4),
			"Vector3 signed_angle_to should work as expected.");
}

TEST_CASE("[Vector3] Axis methods") {
	Vector3 vector = Vector3(1.2, 3.4, 5.6);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector3::Axis::AXIS_Z,
			"Vector3 max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector3::Axis::AXIS_X,
			"Vector3 min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.max_axis_index()] == (real_t)5.6,
			"Vector3 array operator should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == (real_t)1.2,
			"Vector3 array operator should work as expected.");

	vector[Vector3::Axis::AXIS_Y] = 3.7;
	CHECK_MESSAGE(
			vector[Vector3::Axis::AXIS_Y] == (real_t)3.7,
			"Vector3 array operator setter should work as expected.");
}

TEST_CASE("[Vector3] Interpolation methods") {
	constexpr Vector3 vector1 = Vector3(1, 2, 3);
	constexpr Vector3 vector2 = Vector3(4, 5, 6);
	CHECK_MESSAGE(
			vector1.lerp(vector2, 0.5) == Vector3(2.5, 3.5, 4.5),
			"Vector3 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.lerp(vector2, 1.0 / 3.0).is_equal_approx(Vector3(2, 3, 4)),
			"Vector3 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.normalized().slerp(vector2.normalized(), 0.5).is_equal_approx(Vector3(0.363866806030273438, 0.555698215961456299, 0.747529566287994385)),
			"Vector3 slerp should work as expected.");
	CHECK_MESSAGE(
			vector1.normalized().slerp(vector2.normalized(), 1.0 / 3.0).is_equal_approx(Vector3(0.332119762897491455, 0.549413740634918213, 0.766707837581634521)),
			"Vector3 slerp should work as expected.");
	CHECK_MESSAGE(
			Vector3(5, 0, 0).slerp(Vector3(0, 3, 4), 0.5).is_equal_approx(Vector3(3.535533905029296875, 2.121320486068725586, 2.828427314758300781)),
			"Vector3 slerp with non-normalized values should work as expected.");
	CHECK_MESSAGE(
			Vector3(1, 1, 1).slerp(Vector3(2, 2, 2), 0.5).is_equal_approx(Vector3(1.5, 1.5, 1.5)),
			"Vector3 slerp with colinear inputs should behave as expected.");
	CHECK_MESSAGE(
			Vector3().slerp(Vector3(), 0.5) == Vector3(),
			"Vector3 slerp with both inputs as zero vectors should return a zero vector.");
	CHECK_MESSAGE(
			Vector3().slerp(Vector3(1, 1, 1), 0.5) == Vector3(0.5, 0.5, 0.5),
			"Vector3 slerp with one input as zero should behave like a regular lerp.");
	CHECK_MESSAGE(
			Vector3(1, 1, 1).slerp(Vector3(), 0.5) == Vector3(0.5, 0.5, 0.5),
			"Vector3 slerp with one input as zero should behave like a regular lerp.");
	CHECK_MESSAGE(
			Vector3(4, 6, 2).slerp(Vector3(8, 10, 3), 0.5).is_equal_approx(Vector3(5.90194219811429941053, 8.06758688849378394534, 2.558307894718317120038)),
			"Vector3 slerp should work as expected.");
	CHECK_MESSAGE(
			vector1.slerp(vector2, 0.5).length() == doctest::Approx((real_t)6.25831088708303172),
			"Vector3 slerp with different length input should return a vector with an interpolated length.");
	CHECK_MESSAGE(
			vector1.angle_to(vector1.slerp(vector2, 0.5)) * 2 == doctest::Approx(vector1.angle_to(vector2)),
			"Vector3 slerp with different length input should return a vector with an interpolated angle.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector3(), Vector3(7, 7, 7), 0.5) == Vector3(2.375, 3.5, 4.625),
			"Vector3 cubic_interpolate should work as expected.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector3(), Vector3(7, 7, 7), 1.0 / 3.0).is_equal_approx(Vector3(1.851851940155029297, 2.962963104248046875, 4.074074268341064453)),
			"Vector3 cubic_interpolate should work as expected.");
	CHECK_MESSAGE(
			Vector3(1, 0, 0).move_toward(Vector3(10, 0, 0), 3) == Vector3(4, 0, 0),
			"Vector3 move_toward should work as expected.");
}

TEST_CASE("[Vector3] Length methods") {
	constexpr Vector3 vector1 = Vector3(10, 10, 10);
	constexpr Vector3 vector2 = Vector3(20, 30, 40);
	CHECK_MESSAGE(
			vector1.length_squared() == 300,
			"Vector3 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(10 * (real_t)Math::SQRT3),
			"Vector3 length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 2900,
			"Vector3 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx((real_t)53.8516480713450403125),
			"Vector3 length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == 1400,
			"Vector3 distance_squared_to should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx((real_t)37.41657386773941385584),
			"Vector3 distance_to should work as expected.");
}

TEST_CASE("[Vector3] Limiting methods") {
	constexpr Vector3 vector = Vector3(10, 10, 10);
	CHECK_MESSAGE(
			vector.limit_length().is_equal_approx(Vector3(Math::SQRT13, Math::SQRT13, Math::SQRT13)),
			"Vector3 limit_length should work as expected.");
	CHECK_MESSAGE(
			vector.limit_length(5).is_equal_approx(5 * Vector3(Math::SQRT13, Math::SQRT13, Math::SQRT13)),
			"Vector3 limit_length should work as expected.");

	CHECK_MESSAGE(
			Vector3(-5, 5, 15).clamp(Vector3(), vector) == Vector3(0, 5, 10),
			"Vector3 clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector3(0, 10, 15), Vector3(5, 10, 20)) == Vector3(5, 10, 15),
			"Vector3 clamp should work as expected.");
}

TEST_CASE("[Vector3] Normalization methods") {
	CHECK_MESSAGE(
			Vector3(1, 0, 0).is_normalized() == true,
			"Vector3 is_normalized should return true for a normalized vector.");
	CHECK_MESSAGE(
			Vector3(1, 1, 1).is_normalized() == false,
			"Vector3 is_normalized should return false for a non-normalized vector.");
	CHECK_MESSAGE(
			Vector3(1, 0, 0).normalized() == Vector3(1, 0, 0),
			"Vector3 normalized should return the same vector for a normalized vector.");
	CHECK_MESSAGE(
			Vector3(1, 1, 0).normalized().is_equal_approx(Vector3(Math::SQRT12, Math::SQRT12, 0)),
			"Vector3 normalized should work as expected.");
	CHECK_MESSAGE(
			Vector3(1, 1, 1).normalized().is_equal_approx(Vector3(Math::SQRT13, Math::SQRT13, Math::SQRT13)),
			"Vector3 normalized should work as expected.");

	Vector3 vector = Vector3(3.2, -5.4, 6);
	vector.normalize();
	CHECK_MESSAGE(
			vector == Vector3(3.2, -5.4, 6).normalized(),
			"Vector3 normalize should convert same way as Vector3 normalized.");
	CHECK_MESSAGE(
			vector.is_equal_approx(Vector3(0.368522751763902980457, -0.621882143601586279522, 0.6909801595573180883585)),
			"Vector3 normalize should work as expected.");
}

TEST_CASE("[Vector3] Operators") {
	constexpr Vector3 decimal1 = Vector3(2.3, 4.9, 7.8);
	constexpr Vector3 decimal2 = Vector3(1.2, 3.4, 5.6);
	constexpr Vector3 power1 = Vector3(0.75, 1.5, 0.625);
	constexpr Vector3 power2 = Vector3(0.5, 0.125, 0.25);
	constexpr Vector3 int1 = Vector3(4, 5, 9);
	constexpr Vector3 int2 = Vector3(1, 2, 3);

	CHECK_MESSAGE(
			(decimal1 + decimal2).is_equal_approx(Vector3(3.5, 8.3, 13.4)),
			"Vector3 addition should behave as expected.");
	static_assert(
			(power1 + power2) == Vector3(1.25, 1.625, 0.875),
			"Vector3 addition with powers of two should give exact results.");
	static_assert(
			(int1 + int2) == Vector3(5, 7, 12),
			"Vector3 addition with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 - decimal2).is_equal_approx(Vector3(1.1, 1.5, 2.2)),
			"Vector3 subtraction should behave as expected.");
	static_assert(
			(power1 - power2) == Vector3(0.25, 1.375, 0.375),
			"Vector3 subtraction with powers of two should give exact results.");
	static_assert(
			(int1 - int2) == Vector3(3, 3, 6),
			"Vector3 subtraction with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * decimal2).is_equal_approx(Vector3(2.76, 16.66, 43.68)),
			"Vector3 multiplication should behave as expected.");
	static_assert(
			(power1 * power2) == Vector3(0.375, 0.1875, 0.15625),
			"Vector3 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * int2) == Vector3(4, 10, 27),
			"Vector3 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / decimal2).is_equal_approx(Vector3(1.91666666666666666, 1.44117647058823529, 1.39285714285714286)),
			"Vector3 division should behave as expected.");
	static_assert(
			(power1 / power2) == Vector3(1.5, 12.0, 2.5),
			"Vector3 division with powers of two should give exact results.");
	static_assert(
			(int1 / int2) == Vector3(4, 2.5, 3),
			"Vector3 division with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * 2).is_equal_approx(Vector3(4.6, 9.8, 15.6)),
			"Vector3 multiplication should behave as expected.");
	static_assert(
			(power1 * 2) == Vector3(1.5, 3, 1.25),
			"Vector3 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * 2) == Vector3(8, 10, 18),
			"Vector3 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / 2).is_equal_approx(Vector3(1.15, 2.45, 3.9)),
			"Vector3 division should behave as expected.");
	static_assert(
			(power1 / 2) == Vector3(0.375, 0.75, 0.3125),
			"Vector3 division with powers of two should give exact results.");
	static_assert(
			(int1 / 2) == Vector3(2, 2.5, 4.5),
			"Vector3 division with integers should give exact results.");

	CHECK_MESSAGE(
			((Vector3i)decimal1) == Vector3i(2, 4, 7),
			"Vector3 cast to Vector3i should work as expected.");
	CHECK_MESSAGE(
			((Vector3i)decimal2) == Vector3i(1, 3, 5),
			"Vector3 cast to Vector3i should work as expected.");
	CHECK_MESSAGE(
			Vector3(Vector3i(1, 2, 3)) == Vector3(1, 2, 3),
			"Vector3 constructed from Vector3i should work as expected.");

	CHECK_MESSAGE(
			((String)decimal1) == "(2.3, 4.9, 7.8)",
			"Vector3 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)decimal2) == "(1.2, 3.4, 5.6)",
			"Vector3 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)Vector3(9.7, 9.8, 9.9)) == "(9.7, 9.8, 9.9)",
			"Vector3 cast to String should work as expected.");
#ifdef REAL_T_IS_DOUBLE
	CHECK_MESSAGE(
			((String)Vector3(Math::E, Math::SQRT2, Math::SQRT3)) == "(2.71828182845905, 1.4142135623731, 1.73205080756888)",
			"Vector3 cast to String should print the correct amount of digits for real_t = double.");
#else
	CHECK_MESSAGE(
			((String)Vector3(Math::E, Math::SQRT2, Math::SQRT3)) == "(2.718282, 1.414214, 1.732051)",
			"Vector3 cast to String should print the correct amount of digits for real_t = float.");
#endif // REAL_T_IS_DOUBLE
}

TEST_CASE("[Vector3] Other methods") {
	constexpr Vector3 vector = Vector3(1.2, 3.4, 5.6);
	CHECK_MESSAGE(
			vector.direction_to(Vector3()).is_equal_approx(-vector.normalized()),
			"Vector3 direction_to should work as expected.");
	CHECK_MESSAGE(
			Vector3(1, 1, 1).direction_to(Vector3(2, 2, 2)).is_equal_approx(Vector3(Math::SQRT13, Math::SQRT13, Math::SQRT13)),
			"Vector3 direction_to should work as expected.");
	CHECK_MESSAGE(
			vector.inverse().is_equal_approx(Vector3(1 / 1.2, 1 / 3.4, 1 / 5.6)),
			"Vector3 inverse should work as expected.");
	CHECK_MESSAGE(
			vector.posmod(2).is_equal_approx(Vector3(1.2, 1.4, 1.6)),
			"Vector3 posmod should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmod(2).is_equal_approx(Vector3(0.8, 0.6, 0.4)),
			"Vector3 posmod should work as expected.");
	CHECK_MESSAGE(
			vector.posmodv(Vector3(1, 2, 3)).is_equal_approx(Vector3(0.2, 1.4, 2.6)),
			"Vector3 posmodv should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmodv(Vector3(2, 3, 4)).is_equal_approx(Vector3(0.8, 2.6, 2.4)),
			"Vector3 posmodv should work as expected.");

	CHECK_MESSAGE(
			vector.rotated(Vector3(0, 1, 0), Math::TAU).is_equal_approx(vector),
			"Vector3 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Vector3(0, 1, 0), Math::TAU / 4).is_equal_approx(Vector3(5.6, 3.4, -1.2)),
			"Vector3 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Vector3(1, 0, 0), Math::TAU / 3).is_equal_approx(Vector3(1.2, -6.54974226119285642, 0.1444863728670914)),
			"Vector3 rotated should work as expected.");
	CHECK_MESSAGE(
			vector.rotated(Vector3(0, 0, 1), Math::TAU / 2).is_equal_approx(vector.rotated(Vector3(0, 0, 1), Math::TAU / -2)),
			"Vector3 rotated should work as expected.");

	CHECK_MESSAGE(
			vector.snapped(Vector3(1, 1, 1)) == Vector3(1, 3, 6),
			"Vector3 snapped to integers should be the same as rounding.");
	CHECK_MESSAGE(
			vector.snapped(Vector3(0.25, 0.25, 0.25)) == Vector3(1.25, 3.5, 5.5),
			"Vector3 snapped to 0.25 should give exact results.");

	CHECK_MESSAGE(
			Vector3(1.2, 2.5, 2.0).is_equal_approx(vector.min(Vector3(3.0, 2.5, 2.0))),
			"Vector3 min should return expected value.");

	CHECK_MESSAGE(
			Vector3(5.3, 3.4, 5.6).is_equal_approx(vector.max(Vector3(5.3, 2.0, 3.0))),
			"Vector3 max should return expected value.");
}

TEST_CASE("[Vector3] Plane methods") {
	constexpr Vector3 vector = Vector3(1.2, 3.4, 5.6);
	constexpr Vector3 vector_y = Vector3(0, 1, 0);
	constexpr Vector3 vector_normal = Vector3(0.88763458893247992491, 0.26300284116517923701, 0.37806658417494515320);
	CHECK_MESSAGE(
			vector.bounce(vector_y) == Vector3(1.2, -3.4, 5.6),
			"Vector3 bounce on a plane with normal of the Y axis should.");
	CHECK_MESSAGE(
			vector.bounce(vector_normal).is_equal_approx(Vector3(-6.0369629829775736287, 1.25571467171034855444, 2.517589840583626047)),
			"Vector3 bounce with normal should return expected value.");
	CHECK_MESSAGE(
			vector.reflect(vector_y) == Vector3(-1.2, 3.4, -5.6),
			"Vector3 reflect on a plane with normal of the Y axis should.");
	CHECK_MESSAGE(
			vector.reflect(vector_normal).is_equal_approx(Vector3(6.0369629829775736287, -1.25571467171034855444, -2.517589840583626047)),
			"Vector3 reflect with normal should return expected value.");
	CHECK_MESSAGE(
			vector.project(vector_y) == Vector3(0, 3.4, 0),
			"Vector3 projected on the Y axis should only give the Y component.");
	CHECK_MESSAGE(
			vector.project(vector_normal).is_equal_approx(Vector3(3.61848149148878681437, 1.0721426641448257227776, 1.54120507970818697649)),
			"Vector3 projected on a normal should return expected value.");
	CHECK_MESSAGE(
			vector.slide(vector_y) == Vector3(1.2, 0, 5.6),
			"Vector3 slide on a plane with normal of the Y axis should set the Y to zero.");
	CHECK_MESSAGE(
			vector.slide(vector_normal).is_equal_approx(Vector3(-2.41848149148878681437, 2.32785733585517427722237, 4.0587949202918130235)),
			"Vector3 slide with normal should return expected value.");
	// There's probably a better way to test these ones?
#ifdef MATH_CHECKS
	constexpr Vector3 vector_non_normal = Vector3(5.4, 1.6, 2.3);
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			vector.bounce(vector_non_normal).is_equal_approx(Vector3()),
			"Vector3 bounce should return empty Vector3 with non-normalized input.");
	CHECK_MESSAGE(
			vector.reflect(vector_non_normal).is_equal_approx(Vector3()),
			"Vector3 reflect should return empty Vector3 with non-normalized input.");
	CHECK_MESSAGE(
			vector.slide(vector_non_normal).is_equal_approx(Vector3()),
			"Vector3 slide should return empty Vector3 with non-normalized input.");
	ERR_PRINT_ON;
#endif // MATH_CHECKS
}

TEST_CASE("[Vector3] Rounding methods") {
	constexpr Vector3 vector1 = Vector3(1.2, 3.4, 5.6);
	constexpr Vector3 vector2 = Vector3(1.2, -3.4, -5.6);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector3 abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector3 abs should work as expected.");

	CHECK_MESSAGE(
			vector1.ceil() == Vector3(2, 4, 6),
			"Vector3 ceil should work as expected.");
	CHECK_MESSAGE(
			vector2.ceil() == Vector3(2, -3, -5),
			"Vector3 ceil should work as expected.");

	CHECK_MESSAGE(
			vector1.floor() == Vector3(1, 3, 5),
			"Vector3 floor should work as expected.");
	CHECK_MESSAGE(
			vector2.floor() == Vector3(1, -4, -6),
			"Vector3 floor should work as expected.");

	CHECK_MESSAGE(
			vector1.round() == Vector3(1, 3, 6),
			"Vector3 round should work as expected.");
	CHECK_MESSAGE(
			vector2.round() == Vector3(1, -3, -6),
			"Vector3 round should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector3(1, 1, 1),
			"Vector3 sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector3(1, -1, -1),
			"Vector3 sign should work as expected.");
}

TEST_CASE("[Vector3] Linear algebra methods") {
	constexpr Vector3 vector_x = Vector3(1, 0, 0);
	constexpr Vector3 vector_y = Vector3(0, 1, 0);
	constexpr Vector3 vector_z = Vector3(0, 0, 1);
	constexpr Vector3 a = Vector3(3.5, 8.5, 2.3);
	constexpr Vector3 b = Vector3(5.2, 4.6, 7.8);
	CHECK_MESSAGE(
			vector_x.cross(vector_y) == vector_z,
			"Vector3 cross product of X and Y should give Z.");
	CHECK_MESSAGE(
			vector_y.cross(vector_x) == -vector_z,
			"Vector3 cross product of Y and X should give negative Z.");
	CHECK_MESSAGE(
			vector_y.cross(vector_z) == vector_x,
			"Vector3 cross product of Y and Z should give X.");
	CHECK_MESSAGE(
			vector_z.cross(vector_x) == vector_y,
			"Vector3 cross product of Z and X should give Y.");
	CHECK_MESSAGE(
			a.cross(b).is_equal_approx(Vector3(55.72, -15.34, -28.1)),
			"Vector3 cross should return expected value.");
	CHECK_MESSAGE(
			Vector3(-a.x, a.y, -a.z).cross(Vector3(b.x, -b.y, b.z)).is_equal_approx(Vector3(55.72, 15.34, -28.1)),
			"Vector2 cross should return expected value.");

	CHECK_MESSAGE(
			vector_x.dot(vector_y) == 0.0,
			"Vector3 dot product of perpendicular vectors should be zero.");
	CHECK_MESSAGE(
			vector_x.dot(vector_x) == 1.0,
			"Vector3 dot product of identical unit vectors should be one.");
	CHECK_MESSAGE(
			(vector_x * 10).dot(vector_x * 10) == 100.0,
			"Vector3 dot product of same direction vectors should behave as expected.");
	CHECK_MESSAGE(
			a.dot(b) == doctest::Approx((real_t)75.24),
			"Vector3 dot should return expected value.");
	CHECK_MESSAGE(
			Vector3(-a.x, a.y, -a.z).dot(Vector3(b.x, -b.y, b.z)) == doctest::Approx((real_t)-75.24),
			"Vector3 dot should return expected value.");
}

TEST_CASE("[Vector3] Finite number checks") {
	constexpr double infinite[] = { Math::NaN, Math::INF, -Math::INF };

	CHECK_MESSAGE(
			Vector3(0, 1, 2).is_finite(),
			"Vector3(0, 1, 2) should be finite");

	for (double x : infinite) {
		CHECK_FALSE_MESSAGE(
				Vector3(x, 1, 2).is_finite(),
				"Vector3 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector3(0, x, 2).is_finite(),
				"Vector3 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector3(0, 1, x).is_finite(),
				"Vector3 with one component infinite should not be finite.");
	}

	for (double x : infinite) {
		for (double y : infinite) {
			CHECK_FALSE_MESSAGE(
					Vector3(x, y, 2).is_finite(),
					"Vector3 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector3(x, 1, y).is_finite(),
					"Vector3 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector3(0, x, y).is_finite(),
					"Vector3 with two components infinite should not be finite.");
		}
	}

	for (double x : infinite) {
		for (double y : infinite) {
			for (double z : infinite) {
				CHECK_FALSE_MESSAGE(
						Vector3(x, y, z).is_finite(),
						"Vector3 with three components infinite should not be finite.");
			}
		}
	}
}

} // namespace TestVector3

// === Merged from tests/core/math/test_vector3i.cpp ===
/*  test_vector3i.cpp                                                     */
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

#include "tests/test_macros.h"



#include "core/math/plane.h"

namespace TestVector3i {

TEST_CASE("[Vector3i] Constructor methods") {
	constexpr Vector3i vector_empty = Vector3i();
	constexpr Vector3i vector_zero = Vector3i(0, 0, 0);
	static_assert(
			vector_empty == vector_zero,
			"Vector3i Constructor with no inputs should return a zero Vector3i.");
}

TEST_CASE("[Vector3i] Axis methods") {
	Vector3i vector = Vector3i(1, 2, 3);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector3i::Axis::AXIS_Z,
			"Vector3i max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector3i::Axis::AXIS_X,
			"Vector3i min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.max_axis_index()] == 3,
			"Vector3i array operator should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == 1,
			"Vector3i array operator should work as expected.");

	vector[Vector3i::Axis::AXIS_Y] = 5;
	CHECK_MESSAGE(
			vector[Vector3i::Axis::AXIS_Y] == 5,
			"Vector3i array operator setter should work as expected.");
}

TEST_CASE("[Vector3i] Clamp method") {
	constexpr Vector3i vector = Vector3i(10, 10, 10);
	CHECK_MESSAGE(
			Vector3i(-5, 5, 15).clamp(Vector3i(), vector) == Vector3i(0, 5, 10),
			"Vector3i clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector3i(0, 10, 15), Vector3i(5, 10, 20)) == Vector3i(5, 10, 15),
			"Vector3i clamp should work as expected.");
}

TEST_CASE("[Vector3i] Length methods") {
	constexpr Vector3i vector1 = Vector3i(10, 10, 10);
	constexpr Vector3i vector2 = Vector3i(20, 30, 40);
	CHECK_MESSAGE(
			vector1.length_squared() == 300,
			"Vector3i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(10 * Math::SQRT3),
			"Vector3i length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 2900,
			"Vector3i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx(53.8516480713450403125),
			"Vector3i length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == 1400,
			"Vector3i distance_squared_to should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx(37.41657386773941385584),
			"Vector3i distance_to should work as expected.");
}

TEST_CASE("[Vector3i] Operators") {
	constexpr Vector3i vector1 = Vector3i(4, 5, 9);
	constexpr Vector3i vector2 = Vector3i(1, 2, 3);

	static_assert(
			(vector1 + vector2) == Vector3i(5, 7, 12),
			"Vector3i addition with integers should give exact results.");
	static_assert(
			(vector1 - vector2) == Vector3i(3, 3, 6),
			"Vector3i subtraction with integers should give exact results.");
	static_assert(
			(vector1 * vector2) == Vector3i(4, 10, 27),
			"Vector3i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / vector2) == Vector3i(4, 2, 3),
			"Vector3i division with integers should give exact results.");

	static_assert(
			(vector1 * 2) == Vector3i(8, 10, 18),
			"Vector3i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / 2) == Vector3i(2, 2, 4),
			"Vector3i division with integers should give exact results.");

	CHECK_MESSAGE(
			((Vector3)vector1) == Vector3(4, 5, 9),
			"Vector3i cast to Vector3 should work as expected.");
	CHECK_MESSAGE(
			((Vector3)vector2) == Vector3(1, 2, 3),
			"Vector3i cast to Vector3 should work as expected.");
	CHECK_MESSAGE(
			Vector3i(Vector3(1.1, 2.9, 3.9)) == Vector3i(1, 2, 3),
			"Vector3i constructed from Vector3 should work as expected.");
}

TEST_CASE("[Vector3i] Other methods") {
	constexpr Vector3i vector = Vector3i(1, 3, -7);

	CHECK_MESSAGE(
			vector.min(Vector3i(3, 2, 5)) == Vector3i(1, 2, -7),
			"Vector3i min should return expected value.");
	CHECK_MESSAGE(
			vector.max(Vector3i(5, 2, 4)) == Vector3i(5, 3, 4),
			"Vector3i max should return expected value.");

	CHECK_MESSAGE(
			vector.snapped(Vector3i(4, 2, 5)) == Vector3i(0, 4, -5),
			"Vector3i snapped should work as expected.");
}

TEST_CASE("[Vector3i] Abs and sign methods") {
	constexpr Vector3i vector1 = Vector3i(1, 3, 5);
	constexpr Vector3i vector2 = Vector3i(1, -3, -5);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector3i abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector3i abs should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector3i(1, 1, 1),
			"Vector3i sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector3i(1, -1, -1),
			"Vector3i sign should work as expected.");
}

} // namespace TestVector3i

// === Merged from tests/core/math/test_vector4.cpp ===
/*  test_vector4.cpp                                                      */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestVector4 {

TEST_CASE("[Vector4] Constructor methods") {
	constexpr Vector4 vector_empty = Vector4();
	constexpr Vector4 vector_zero = Vector4(0.0, 0.0, 0.0, 0.0);
	static_assert(
			vector_empty == vector_zero,
			"Vector4 Constructor with no inputs should return a zero Vector4.");
}

TEST_CASE("[Vector4] Axis methods") {
	Vector4 vector = Vector4(1.2, 3.4, 5.6, -0.9);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector4::Axis::AXIS_Z,
			"Vector4 max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector4::Axis::AXIS_W,
			"Vector4 min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.max_axis_index()] == (real_t)5.6,
			"Vector4 array operator should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == (real_t)-0.9,
			"Vector4 array operator should work as expected.");

	vector[Vector4::Axis::AXIS_Y] = 3.7;
	CHECK_MESSAGE(
			vector[Vector4::Axis::AXIS_Y] == (real_t)3.7,
			"Vector4 array operator setter should work as expected.");
}

TEST_CASE("[Vector4] Interpolation methods") {
	constexpr Vector4 vector1 = Vector4(1, 2, 3, 4);
	constexpr Vector4 vector2 = Vector4(4, 5, 6, 7);
	CHECK_MESSAGE(
			vector1.lerp(vector2, 0.5) == Vector4(2.5, 3.5, 4.5, 5.5),
			"Vector4 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.lerp(vector2, 1.0 / 3.0).is_equal_approx(Vector4(2, 3, 4, 5)),
			"Vector4 lerp should work as expected.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector4(), Vector4(7, 7, 7, 7), 0.5) == Vector4(2.375, 3.5, 4.625, 5.75),
			"Vector4 cubic_interpolate should work as expected.");
	CHECK_MESSAGE(
			vector1.cubic_interpolate(vector2, Vector4(), Vector4(7, 7, 7, 7), 1.0 / 3.0).is_equal_approx(Vector4(1.851851940155029297, 2.962963104248046875, 4.074074268341064453, 5.185185185185)),
			"Vector4 cubic_interpolate should work as expected.");
}

TEST_CASE("[Vector4] Length methods") {
	constexpr Vector4 vector1 = Vector4(10, 10, 10, 10);
	constexpr Vector4 vector2 = Vector4(20, 30, 40, 50);
	CHECK_MESSAGE(
			vector1.length_squared() == 400,
			"Vector4 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(20),
			"Vector4 length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 5400,
			"Vector4 length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx((real_t)73.484692283495),
			"Vector4 length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx((real_t)54.772255750517),
			"Vector4 distance_to should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == doctest::Approx(3000),
			"Vector4 distance_squared_to should work as expected.");
}

TEST_CASE("[Vector4] Limiting methods") {
	constexpr Vector4 vector = Vector4(10, 10, 10, 10);
	CHECK_MESSAGE(
			Vector4(-5, 5, 15, -15).clamp(Vector4(), vector) == Vector4(0, 5, 10, 0),
			"Vector4 clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector4(0, 10, 15, 18), Vector4(5, 10, 20, 25)) == Vector4(5, 10, 15, 18),
			"Vector4 clamp should work as expected.");
}

TEST_CASE("[Vector4] Normalization methods") {
	CHECK_MESSAGE(
			Vector4(1, 0, 0, 0).is_normalized() == true,
			"Vector4 is_normalized should return true for a normalized vector.");
	CHECK_MESSAGE(
			Vector4(1, 1, 1, 1).is_normalized() == false,
			"Vector4 is_normalized should return false for a non-normalized vector.");
	CHECK_MESSAGE(
			Vector4(1, 0, 0, 0).normalized() == Vector4(1, 0, 0, 0),
			"Vector4 normalized should return the same vector for a normalized vector.");
	CHECK_MESSAGE(
			Vector4(1, 1, 0, 0).normalized().is_equal_approx(Vector4(Math::SQRT12, Math::SQRT12, 0, 0)),
			"Vector4 normalized should work as expected.");
	CHECK_MESSAGE(
			Vector4(1, 1, 1, 1).normalized().is_equal_approx(Vector4(0.5, 0.5, 0.5, 0.5)),
			"Vector4 normalized should work as expected.");
}

TEST_CASE("[Vector4] Operators") {
	constexpr Vector4 decimal1 = Vector4(2.3, 4.9, 7.8, 3.2);
	constexpr Vector4 decimal2 = Vector4(1.2, 3.4, 5.6, 1.7);
	constexpr Vector4 power1 = Vector4(0.75, 1.5, 0.625, 0.125);
	constexpr Vector4 power2 = Vector4(0.5, 0.125, 0.25, 0.75);
	constexpr Vector4 int1 = Vector4(4, 5, 9, 2);
	constexpr Vector4 int2 = Vector4(1, 2, 3, 1);

	static_assert(
			-decimal1 == Vector4(-2.3, -4.9, -7.8, -3.2),
			"Vector4 change of sign should work as expected.");
	CHECK_MESSAGE(
			(decimal1 + decimal2).is_equal_approx(Vector4(3.5, 8.3, 13.4, 4.9)),
			"Vector4 addition should behave as expected.");
	static_assert(
			(power1 + power2) == Vector4(1.25, 1.625, 0.875, 0.875),
			"Vector4 addition with powers of two should give exact results.");
	static_assert(
			(int1 + int2) == Vector4(5, 7, 12, 3),
			"Vector4 addition with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 - decimal2).is_equal_approx(Vector4(1.1, 1.5, 2.2, 1.5)),
			"Vector4 subtraction should behave as expected.");
	static_assert(
			(power1 - power2) == Vector4(0.25, 1.375, 0.375, -0.625),
			"Vector4 subtraction with powers of two should give exact results.");
	static_assert(
			(int1 - int2) == Vector4(3, 3, 6, 1),
			"Vector4 subtraction with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * decimal2).is_equal_approx(Vector4(2.76, 16.66, 43.68, 5.44)),
			"Vector4 multiplication should behave as expected.");
	static_assert(
			(power1 * power2) == Vector4(0.375, 0.1875, 0.15625, 0.09375),
			"Vector4 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * int2) == Vector4(4, 10, 27, 2),
			"Vector4 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / decimal2).is_equal_approx(Vector4(1.91666666666666666, 1.44117647058823529, 1.39285714285714286, 1.88235294118)),
			"Vector4 division should behave as expected.");
	static_assert(
			(power1 / power2) == Vector4(1.5, 12.0, 2.5, 1.0 / 6.0),
			"Vector4 division with powers of two should give exact results.");
	static_assert(
			(int1 / int2) == Vector4(4, 2.5, 3, 2),
			"Vector4 division with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 * 2).is_equal_approx(Vector4(4.6, 9.8, 15.6, 6.4)),
			"Vector4 multiplication should behave as expected.");
	static_assert(
			(power1 * 2) == Vector4(1.5, 3, 1.25, 0.25),
			"Vector4 multiplication with powers of two should give exact results.");
	static_assert(
			(int1 * 2) == Vector4(8, 10, 18, 4),
			"Vector4 multiplication with integers should give exact results.");

	CHECK_MESSAGE(
			(decimal1 / 2).is_equal_approx(Vector4(1.15, 2.45, 3.9, 1.6)),
			"Vector4 division should behave as expected.");
	static_assert(
			(power1 / 2) == Vector4(0.375, 0.75, 0.3125, 0.0625),
			"Vector4 division with powers of two should give exact results.");
	static_assert(
			(int1 / 2) == Vector4(2, 2.5, 4.5, 1),
			"Vector4 division with integers should give exact results.");

	CHECK_MESSAGE(
			((String)decimal1) == "(2.3, 4.9, 7.8, 3.2)",
			"Vector4 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)decimal2) == "(1.2, 3.4, 5.6, 1.7)",
			"Vector4 cast to String should work as expected.");
	CHECK_MESSAGE(
			((String)Vector4(9.7, 9.8, 9.9, -1.8)) == "(9.7, 9.8, 9.9, -1.8)",
			"Vector4 cast to String should work as expected.");
#ifdef REAL_T_IS_DOUBLE
	CHECK_MESSAGE(
			((String)Vector4(Math::E, Math::SQRT2, Math::SQRT3, Math::SQRT3)) == "(2.71828182845905, 1.4142135623731, 1.73205080756888, 1.73205080756888)",
			"Vector4 cast to String should print the correct amount of digits for real_t = double.");
#else
	CHECK_MESSAGE(
			((String)Vector4(Math::E, Math::SQRT2, Math::SQRT3, Math::SQRT3)) == "(2.718282, 1.414214, 1.732051, 1.732051)",
			"Vector4 cast to String should print the correct amount of digits for real_t = float.");
#endif // REAL_T_IS_DOUBLE
}

TEST_CASE("[Vector4] Other methods") {
	constexpr Vector4 vector = Vector4(1.2, 3.4, 5.6, 1.6);
	CHECK_MESSAGE(
			vector.direction_to(Vector4()).is_equal_approx(-vector.normalized()),
			"Vector4 direction_to should work as expected.");
	CHECK_MESSAGE(
			Vector4(1, 1, 1, 1).direction_to(Vector4(2, 2, 2, 2)).is_equal_approx(Vector4(0.5, 0.5, 0.5, 0.5)),
			"Vector4 direction_to should work as expected.");
	CHECK_MESSAGE(
			vector.inverse().is_equal_approx(Vector4(1 / 1.2, 1 / 3.4, 1 / 5.6, 1 / 1.6)),
			"Vector4 inverse should work as expected.");
	CHECK_MESSAGE(
			vector.posmod(2).is_equal_approx(Vector4(1.2, 1.4, 1.6, 1.6)),
			"Vector4 posmod should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmod(2).is_equal_approx(Vector4(0.8, 0.6, 0.4, 0.4)),
			"Vector4 posmod should work as expected.");
	CHECK_MESSAGE(
			vector.posmodv(Vector4(1, 2, 3, 4)).is_equal_approx(Vector4(0.2, 1.4, 2.6, 1.6)),
			"Vector4 posmodv should work as expected.");
	CHECK_MESSAGE(
			(-vector).posmodv(Vector4(2, 3, 4, 5)).is_equal_approx(Vector4(0.8, 2.6, 2.4, 3.4)),
			"Vector4 posmodv should work as expected.");
	CHECK_MESSAGE(
			vector.snapped(Vector4(1, 1, 1, 1)) == Vector4(1, 3, 6, 2),
			"Vector4 snapped to integers should be the same as rounding.");
	CHECK_MESSAGE(
			vector.snapped(Vector4(0.25, 0.25, 0.25, 0.25)) == Vector4(1.25, 3.5, 5.5, 1.5),
			"Vector4 snapped to 0.25 should give exact results.");

	CHECK_MESSAGE(
			Vector4(1.2, 2.5, 2.0, 1.6).is_equal_approx(vector.min(Vector4(3.0, 2.5, 2.0, 3.4))),
			"Vector4 min should return expected value.");

	CHECK_MESSAGE(
			Vector4(5.3, 3.4, 5.6, 4.2).is_equal_approx(vector.max(Vector4(5.3, 2.0, 3.0, 4.2))),
			"Vector4 max should return expected value.");
}

TEST_CASE("[Vector4] Rounding methods") {
	constexpr Vector4 vector1 = Vector4(1.2, 3.4, 5.6, 1.6);
	constexpr Vector4 vector2 = Vector4(1.2, -3.4, -5.6, -1.6);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector4 abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector4 abs should work as expected.");
	CHECK_MESSAGE(
			vector1.ceil() == Vector4(2, 4, 6, 2),
			"Vector4 ceil should work as expected.");
	CHECK_MESSAGE(
			vector2.ceil() == Vector4(2, -3, -5, -1),
			"Vector4 ceil should work as expected.");

	CHECK_MESSAGE(
			vector1.floor() == Vector4(1, 3, 5, 1),
			"Vector4 floor should work as expected.");
	CHECK_MESSAGE(
			vector2.floor() == Vector4(1, -4, -6, -2),
			"Vector4 floor should work as expected.");

	CHECK_MESSAGE(
			vector1.round() == Vector4(1, 3, 6, 2),
			"Vector4 round should work as expected.");
	CHECK_MESSAGE(
			vector2.round() == Vector4(1, -3, -6, -2),
			"Vector4 round should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector4(1, 1, 1, 1),
			"Vector4 sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector4(1, -1, -1, -1),
			"Vector4 sign should work as expected.");
}

TEST_CASE("[Vector4] Linear algebra methods") {
	constexpr Vector4 vector_x = Vector4(1, 0, 0, 0);
	constexpr Vector4 vector_y = Vector4(0, 1, 0, 0);
	constexpr Vector4 vector1 = Vector4(1.7, 2.3, 1, 9.1);
	constexpr Vector4 vector2 = Vector4(-8.2, -16, 3, 2.4);

	CHECK_MESSAGE(
			vector_x.dot(vector_y) == 0.0,
			"Vector4 dot product of perpendicular vectors should be zero.");
	CHECK_MESSAGE(
			vector_x.dot(vector_x) == 1.0,
			"Vector4 dot product of identical unit vectors should be one.");
	CHECK_MESSAGE(
			(vector_x * 10).dot(vector_x * 10) == 100.0,
			"Vector4 dot product of same direction vectors should behave as expected.");
	CHECK_MESSAGE(
			(vector1 * 2).dot(vector2 * 4) == doctest::Approx((real_t)-25.9 * 8),
			"Vector4 dot product should work as expected.");
}

TEST_CASE("[Vector4] Finite number checks") {
	constexpr double infinite[] = { Math::NaN, Math::INF, -Math::INF };

	CHECK_MESSAGE(
			Vector4(0, 1, 2, 3).is_finite(),
			"Vector4(0, 1, 2, 3) should be finite");

	for (double x : infinite) {
		CHECK_FALSE_MESSAGE(
				Vector4(x, 1, 2, 3).is_finite(),
				"Vector4 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector4(0, x, 2, 3).is_finite(),
				"Vector4 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector4(0, 1, x, 3).is_finite(),
				"Vector4 with one component infinite should not be finite.");
		CHECK_FALSE_MESSAGE(
				Vector4(0, 1, 2, x).is_finite(),
				"Vector4 with one component infinite should not be finite.");
	}

	for (double x : infinite) {
		for (double y : infinite) {
			CHECK_FALSE_MESSAGE(
					Vector4(x, y, 2, 3).is_finite(),
					"Vector4 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector4(x, 1, y, 3).is_finite(),
					"Vector4 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector4(x, 1, 2, y).is_finite(),
					"Vector4 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector4(0, x, y, 3).is_finite(),
					"Vector4 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector4(0, x, 2, y).is_finite(),
					"Vector4 with two components infinite should not be finite.");
			CHECK_FALSE_MESSAGE(
					Vector4(0, 1, x, y).is_finite(),
					"Vector4 with two components infinite should not be finite.");
		}
	}

	for (double x : infinite) {
		for (double y : infinite) {
			for (double z : infinite) {
				CHECK_FALSE_MESSAGE(
						Vector4(0, x, y, z).is_finite(),
						"Vector4 with three components infinite should not be finite.");
				CHECK_FALSE_MESSAGE(
						Vector4(x, 1, y, z).is_finite(),
						"Vector4 with three components infinite should not be finite.");
				CHECK_FALSE_MESSAGE(
						Vector4(x, y, 2, z).is_finite(),
						"Vector4 with three components infinite should not be finite.");
				CHECK_FALSE_MESSAGE(
						Vector4(x, y, z, 3).is_finite(),
						"Vector4 with three components infinite should not be finite.");
			}
		}
	}

	for (double x : infinite) {
		for (double y : infinite) {
			for (double z : infinite) {
				for (double w : infinite) {
					CHECK_FALSE_MESSAGE(
							Vector4(x, y, z, w).is_finite(),
							"Vector4 with four components infinite should not be finite.");
				}
			}
		}
	}
}

} // namespace TestVector4

// === Merged from tests/core/math/test_vector4i.cpp ===
/*  test_vector4i.cpp                                                     */
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

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestVector4i {

TEST_CASE("[Vector4i] Constructor methods") {
	constexpr Vector4i vector_empty = Vector4i();
	constexpr Vector4i vector_zero = Vector4i(0, 0, 0, 0);
	static_assert(
			vector_empty == vector_zero,
			"Vector4i Constructor with no inputs should return a zero Vector4i.");
}

TEST_CASE("[Vector4i] Axis methods") {
	Vector4i vector = Vector4i(1, 2, 3, 4);
	CHECK_MESSAGE(
			vector.max_axis_index() == Vector4i::Axis::AXIS_W,
			"Vector4i max_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector.min_axis_index() == Vector4i::Axis::AXIS_X,
			"Vector4i min_axis_index should work as expected.");
	CHECK_MESSAGE(
			vector[vector.max_axis_index()] == 4,
			"Vector4i array operator should work as expected.");
	CHECK_MESSAGE(
			vector[vector.min_axis_index()] == 1,
			"Vector4i array operator should work as expected.");

	vector[Vector4i::Axis::AXIS_Y] = 5;
	CHECK_MESSAGE(
			vector[Vector4i::Axis::AXIS_Y] == 5,
			"Vector4i array operator setter should work as expected.");
}

TEST_CASE("[Vector4i] Clamp method") {
	constexpr Vector4i vector = Vector4i(10, 10, 10, 10);
	CHECK_MESSAGE(
			Vector4i(-5, 5, 15, INT_MAX).clamp(Vector4i(), vector) == Vector4i(0, 5, 10, 10),
			"Vector4i clamp should work as expected.");
	CHECK_MESSAGE(
			vector.clamp(Vector4i(0, 10, 15, -10), Vector4i(5, 10, 20, -5)) == Vector4i(5, 10, 15, -5),
			"Vector4i clamp should work as expected.");
}

TEST_CASE("[Vector4i] Length methods") {
	constexpr Vector4i vector1 = Vector4i(10, 10, 10, 10);
	constexpr Vector4i vector2 = Vector4i(20, 30, 40, 50);
	CHECK_MESSAGE(
			vector1.length_squared() == 400,
			"Vector4i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector1.length() == doctest::Approx(20),
			"Vector4i length should work as expected.");
	CHECK_MESSAGE(
			vector2.length_squared() == 5400,
			"Vector4i length_squared should work as expected and return exact result.");
	CHECK_MESSAGE(
			vector2.length() == doctest::Approx(73.4846922835),
			"Vector4i length should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_squared_to(vector2) == 3000,
			"Vector4i distance_squared_to should work as expected.");
	CHECK_MESSAGE(
			vector1.distance_to(vector2) == doctest::Approx(54.772255750517),
			"Vector4i distance_to should work as expected.");
}

TEST_CASE("[Vector4i] Operators") {
	constexpr Vector4i vector1 = Vector4i(4, 5, 9, 2);
	constexpr Vector4i vector2 = Vector4i(1, 2, 3, 4);

	static_assert(
			-vector1 == Vector4i(-4, -5, -9, -2),
			"Vector4i change of sign should work as expected.");
	static_assert(
			(vector1 + vector2) == Vector4i(5, 7, 12, 6),
			"Vector4i addition with integers should give exact results.");
	static_assert(
			(vector1 - vector2) == Vector4i(3, 3, 6, -2),
			"Vector4i subtraction with integers should give exact results.");
	static_assert(
			(vector1 * vector2) == Vector4i(4, 10, 27, 8),
			"Vector4i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / vector2) == Vector4i(4, 2, 3, 0),
			"Vector4i division with integers should give exact results.");

	static_assert(
			(vector1 * 2) == Vector4i(8, 10, 18, 4),
			"Vector4i multiplication with integers should give exact results.");
	static_assert(
			(vector1 / 2) == Vector4i(2, 2, 4, 1),
			"Vector4i division with integers should give exact results.");

	CHECK_MESSAGE(
			((Vector4)vector1) == Vector4(4, 5, 9, 2),
			"Vector4i cast to Vector4 should work as expected.");
	CHECK_MESSAGE(
			((Vector4)vector2) == Vector4(1, 2, 3, 4),
			"Vector4i cast to Vector4 should work as expected.");
	CHECK_MESSAGE(
			Vector4i(Vector4(1.1, 2.9, 3.9, 100.5)) == Vector4i(1, 2, 3, 100),
			"Vector4i constructed from Vector4 should work as expected.");
}

TEST_CASE("[Vector3i] Other methods") {
	constexpr Vector4i vector = Vector4i(1, 3, -7, 13);

	CHECK_MESSAGE(
			vector.min(Vector4i(3, 2, 5, 8)) == Vector4i(1, 2, -7, 8),
			"Vector4i min should return expected value.");

	CHECK_MESSAGE(
			vector.max(Vector4i(5, 2, 4, 8)) == Vector4i(5, 3, 4, 13),
			"Vector4i max should return expected value.");

	CHECK_MESSAGE(
			vector.snapped(Vector4i(4, 2, 5, 8)) == Vector4i(0, 4, -5, 16),
			"Vector4i snapped should work as expected.");
}

TEST_CASE("[Vector4i] Abs and sign methods") {
	constexpr Vector4i vector1 = Vector4i(1, 3, 5, 7);
	constexpr Vector4i vector2 = Vector4i(1, -3, -5, 7);
	CHECK_MESSAGE(
			vector1.abs() == vector1,
			"Vector4i abs should work as expected.");
	CHECK_MESSAGE(
			vector2.abs() == vector1,
			"Vector4i abs should work as expected.");

	CHECK_MESSAGE(
			vector1.sign() == Vector4i(1, 1, 1, 1),
			"Vector4i sign should work as expected.");
	CHECK_MESSAGE(
			vector2.sign() == Vector4i(1, -1, -1, 1),
			"Vector4i sign should work as expected.");
}

} // namespace TestVector4i

TEST_FORCE_LINK(test_math)

