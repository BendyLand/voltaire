/**************************************************************************/
/*  test_primitives.cpp                                                   */
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

TEST_FORCE_LINK(test_primitives)

#ifndef _3D_DISABLED

#include "scene/resources/mesh.h"

namespace TestPrimitives {

TEST_CASE("[SceneTree][Primitive][Capsule] Capsule Primitive") {
	Ref<PrimitiveMesh> capsule = memnew(PrimitiveMesh(PrimitiveMesh::MESH_CAPSULE));
	CHECK(capsule->get_mesh_type() == PrimitiveMesh::MESH_CAPSULE);

	SUBCASE("[SceneTree][Primitive][Capsule] Default values should be valid") {
		CHECK_MESSAGE(capsule->get_radius() > 0,
				"Radius of default capsule positive.");
		CHECK_MESSAGE(capsule->get_height() > 0,
				"Height of default capsule positive.");
		CHECK_MESSAGE(capsule->get_radial_segments() >= 0,
				"Radius Segments of default capsule positive.");
		CHECK_MESSAGE(capsule->get_rings() >= 0,
				"Number of rings of default capsule positive.");
	}

	SUBCASE("[SceneTree][Primitive][Capsule] Set properties of the capsule and get them with accessor methods") {
		capsule->set_height(7.1f);
		capsule->set_radius(1.3f);
		capsule->set_radial_segments(16);
		capsule->set_rings(32);

		CHECK_MESSAGE(capsule->get_radius() == doctest::Approx(1.3f),
				"Get/Set radius work with one set.");
		CHECK_MESSAGE(capsule->get_height() == doctest::Approx(7.1f),
				"Get/Set radius work with one set.");
		CHECK_MESSAGE(capsule->get_radial_segments() == 16,
				"Get/Set radius work with one set.");
		CHECK_MESSAGE(capsule->get_rings() == 32,
				"Get/Set radius work with one set.");
	}

	SUBCASE("[SceneTree][Primitive][Capsule] If set segments negative, default to at least 0") {
		ERR_PRINT_OFF;
		capsule->set_radial_segments(-5);
		capsule->set_rings(-17);
		ERR_PRINT_ON;

		CHECK_MESSAGE(capsule->get_radial_segments() >= 0,
				"Ensure number of radial segments is >= 0.");
		CHECK_MESSAGE(capsule->get_rings() >= 0,
				"Ensure number of rings is >= 0.");
	}

	SUBCASE("[SceneTree][Primitive][Capsule] If set height < 2*radius, adjust radius and height to radius=height*0.5") {
		capsule->set_radius(1.f);
		capsule->set_height(0.5f);

		CHECK_MESSAGE(capsule->get_radius() >= capsule->get_height() * 0.5,
				"Ensure radius >= height * 0.5 (needed for capsule to exist).");
	}
} // End capsule tests

TEST_CASE("[SceneTree][Primitive][Box] Box Primitive") {
	Ref<PrimitiveMesh> box = memnew(PrimitiveMesh(PrimitiveMesh::MESH_BOX));
	CHECK(box->get_mesh_type() == PrimitiveMesh::MESH_BOX);

	SUBCASE("[SceneTree][Primitive][Box] Default values should be valid") {
		CHECK(box->get_size().x > 0);
		CHECK(box->get_size().y > 0);
		CHECK(box->get_size().z > 0);
		CHECK(box->get_subdivide_width() >= 0);
		CHECK(box->get_subdivide_height() >= 0);
		CHECK(box->get_subdivide_depth() >= 0);
	}

	SUBCASE("[SceneTree][Primitive][Box] Set properties and get them with accessor methods") {
		Vector3 size{ 2.1, 3.3, 1.7 };
		box->set_size(size);
		box->set_subdivide_width(3);
		box->set_subdivide_height(2);
		box->set_subdivide_depth(4);

		CHECK(box->get_size().is_equal_approx(size));
		CHECK(box->get_subdivide_width() == 3);
		CHECK(box->get_subdivide_height() == 2);
		CHECK(box->get_subdivide_depth() == 4);
	}

	SUBCASE("[SceneTree][Primitive][Box] Set subdivides to negative and ensure they are >= 0") {
		ERR_PRINT_OFF;
		box->set_subdivide_width(-2);
		box->set_subdivide_height(-2);
		box->set_subdivide_depth(-2);
		ERR_PRINT_ON;

		CHECK(box->get_subdivide_width() >= 0);
		CHECK(box->get_subdivide_height() >= 0);
		CHECK(box->get_subdivide_depth() >= 0);
	}
} // End box tests

TEST_CASE("[SceneTree][Primitive][Cylinder] Cylinder Primitive") {
	Ref<PrimitiveMesh> cylinder = memnew(PrimitiveMesh(PrimitiveMesh::MESH_CYLINDER));
	CHECK(cylinder->get_mesh_type() == PrimitiveMesh::MESH_CYLINDER);

	SUBCASE("[SceneTree][Primitive][Cylinder] Default values should be valid") {
		CHECK(cylinder->get_top_radius() > 0);
		CHECK(cylinder->get_bottom_radius() > 0);
		CHECK(cylinder->get_height() > 0);
		CHECK(cylinder->get_radial_segments() > 0);
		CHECK(cylinder->get_rings() >= 0);
	}

	SUBCASE("[SceneTree][Primitive][Cylinder] Set properties and get them") {
		cylinder->set_top_radius(4.3f);
		cylinder->set_bottom_radius(1.2f);
		cylinder->set_height(9.77f);
		cylinder->set_radial_segments(12);
		cylinder->set_rings(16);
		cylinder->set_cap_top(false);
		cylinder->set_cap_bottom(false);

		CHECK(cylinder->get_top_radius() == doctest::Approx(4.3f));
		CHECK(cylinder->get_bottom_radius() == doctest::Approx(1.2f));
		CHECK(cylinder->get_height() == doctest::Approx(9.77f));
		CHECK(cylinder->get_radial_segments() == 12);
		CHECK(cylinder->get_rings() == 16);
		CHECK(!cylinder->is_cap_top());
		CHECK(!cylinder->is_cap_bottom());
	}

	SUBCASE("[SceneTree][Primitive][Cylinder] Ensure num segments is >= 0") {
		ERR_PRINT_OFF;
		cylinder->set_radial_segments(-12);
		cylinder->set_rings(-16);
		ERR_PRINT_ON;

		CHECK(cylinder->get_radial_segments() >= 0);
		CHECK(cylinder->get_rings() >= 0);
	}
} // End cylinder tests

TEST_CASE("[SceneTree][Primitive][Plane] Plane Primitive") {
	Ref<PrimitiveMesh> plane = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
	CHECK(plane->get_mesh_type() == PrimitiveMesh::MESH_PLANE);

	SUBCASE("[SceneTree][Primitive][Plane] Default values should be valid") {
		CHECK(plane->get_plane_size().x > 0);
		CHECK(plane->get_plane_size().y > 0);
		CHECK(plane->get_subdivide_width() >= 0);
		CHECK(plane->get_subdivide_depth() >= 0);
		CHECK((plane->get_orientation() == PrimitiveMesh::FACE_X || plane->get_orientation() == PrimitiveMesh::FACE_Y || plane->get_orientation() == PrimitiveMesh::FACE_Z));
	}

	SUBCASE("[SceneTree][Primitive][Plane] Set properties and get them.") {
		Size2 size{ 3.2, 1.8 };
		Vector3 offset{ -7.3, 0.4, -1.7 };
		plane->set_plane_size(size);
		plane->set_subdivide_width(15);
		plane->set_subdivide_depth(29);
		plane->set_center_offset(offset);
		plane->set_orientation(PrimitiveMesh::FACE_X);

		CHECK(plane->get_plane_size().is_equal_approx(size));
		CHECK(plane->get_subdivide_width() == 15);
		CHECK(plane->get_subdivide_depth() == 29);
		CHECK(plane->get_center_offset().is_equal_approx(offset));
		CHECK(plane->get_orientation() == PrimitiveMesh::FACE_X);
	}

	SUBCASE("[SceneTree][Primitive][Plane] Ensure number of segments is >= 0.") {
		ERR_PRINT_OFF;
		plane->set_subdivide_width(-15);
		plane->set_subdivide_depth(-29);
		ERR_PRINT_ON;

		CHECK(plane->get_subdivide_width() >= 0);
		CHECK(plane->get_subdivide_depth() >= 0);
	}
}

TEST_CASE("[SceneTree][Primitive][Quad] Quad Primitive") {
	Ref<PrimitiveMesh> quad = memnew(PrimitiveMesh(PrimitiveMesh::MESH_QUAD));
	CHECK(quad->get_mesh_type() == PrimitiveMesh::MESH_QUAD);

	SUBCASE("[Primitive][Quad] Orientation on initialization is in z direction") {
		CHECK(quad->get_orientation() == PrimitiveMesh::FACE_Z);
		CHECK(quad->get_plane_size().is_equal_approx(Size2(1, 1)));
	}
}

TEST_CASE("[SceneTree][Primitive][Point] Point Primitive") {
	Ref<PrimitiveMesh> point = memnew(PrimitiveMesh(PrimitiveMesh::MESH_POINT));
	CHECK(point->get_mesh_type() == PrimitiveMesh::MESH_POINT);
	CHECK(point->surface_get_primitive_type(0) == Mesh::PRIMITIVE_POINTS);
}

TEST_CASE("[SceneTree][Primitive][Prism] Prism Primitive") {
	Ref<PrimitiveMesh> prism = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PRISM));
	CHECK(prism->get_mesh_type() == PrimitiveMesh::MESH_PRISM);

	SUBCASE("[Primitive][Prism] There are valid values of properties on initialization.") {
		CHECK(prism->get_left_to_right() >= 0);
		CHECK(prism->get_size().x >= 0);
		CHECK(prism->get_size().y >= 0);
		CHECK(prism->get_size().z >= 0);
		CHECK(prism->get_subdivide_width() >= 0);
		CHECK(prism->get_subdivide_height() >= 0);
		CHECK(prism->get_subdivide_depth() >= 0);
	}

	SUBCASE("[Primitive][Prism] Are able to change prism properties.") {
		Vector3 size{ 4.3, 9.1, 0.43 };
		prism->set_left_to_right(3.4f);
		prism->set_size(size);
		prism->set_subdivide_width(36);
		prism->set_subdivide_height(5);
		prism->set_subdivide_depth(64);

		CHECK(prism->get_left_to_right() == doctest::Approx(3.4f));
		CHECK(prism->get_size().is_equal_approx(size));
		CHECK(prism->get_subdivide_width() == 36);
		CHECK(prism->get_subdivide_height() == 5);
		CHECK(prism->get_subdivide_depth() == 64);
	}

	SUBCASE("[Primitive][Prism] Ensure number of segments always >= 0") {
		ERR_PRINT_OFF;
		prism->set_subdivide_width(-36);
		prism->set_subdivide_height(-5);
		prism->set_subdivide_depth(-64);
		ERR_PRINT_ON;

		CHECK(prism->get_subdivide_width() >= 0);
		CHECK(prism->get_subdivide_height() >= 0);
		CHECK(prism->get_subdivide_depth() >= 0);
	}
}

TEST_CASE("[SceneTree][Primitive][Sphere] Sphere Primitive") {
	Ref<PrimitiveMesh> sphere = memnew(PrimitiveMesh(PrimitiveMesh::MESH_SPHERE));
	CHECK(sphere->get_mesh_type() == PrimitiveMesh::MESH_SPHERE);

	SUBCASE("[Primitive][Sphere] There are valid values of properties on initialization.") {
		CHECK(sphere->get_radius() >= 0);
		CHECK(sphere->get_height() >= 0);
		CHECK(sphere->get_radial_segments() >= 0);
		CHECK(sphere->get_rings() >= 0);
	}

	SUBCASE("[Primitive][Sphere] Are able to change sphere properties.") {
		sphere->set_radius(3.4f);
		sphere->set_height(2.2f);
		sphere->set_radial_segments(36);
		sphere->set_rings(5);
		sphere->set_is_hemisphere(true);

		CHECK(sphere->get_radius() == doctest::Approx(3.4f));
		CHECK(sphere->get_height() == doctest::Approx(2.2f));
		CHECK(sphere->get_radial_segments() == 36);
		CHECK(sphere->get_rings() == 5);
		CHECK(sphere->get_is_hemisphere());
	}

	SUBCASE("[Primitive][Sphere] Ensure number of segments always >= 0") {
		ERR_PRINT_OFF;
		sphere->set_radial_segments(-36);
		sphere->set_rings(-5);
		ERR_PRINT_ON;

		CHECK(sphere->get_radial_segments() >= 0);
		CHECK(sphere->get_rings() >= 0);
	}
}

TEST_CASE("[SceneTree][Primitive][Torus] Torus Primitive") {
	Ref<PrimitiveMesh> torus = memnew(PrimitiveMesh(PrimitiveMesh::MESH_TORUS));
	Ref<PrimitiveMesh> prim = memnew(PrimitiveMesh);
	CHECK(torus->get_mesh_type() == PrimitiveMesh::MESH_TORUS);
	CHECK(prim->get_mesh_type() == PrimitiveMesh::MESH_BOX);

	SUBCASE("[Primitive][Torus] There are valid values of properties on initialization.") {
		CHECK(torus->get_inner_radius() > 0);
		CHECK(torus->get_outer_radius() > 0);
		CHECK(torus->get_rings() >= 0);
		CHECK(torus->get_ring_segments() >= 0);
	}

	SUBCASE("[Primitive][Torus] Are able to change properties.") {
		torus->set_inner_radius(3.2f);
		torus->set_outer_radius(9.5f);
		torus->set_rings(19);
		torus->set_ring_segments(43);

		CHECK(torus->get_inner_radius() == doctest::Approx(3.2f));
		CHECK(torus->get_outer_radius() == doctest::Approx(9.5f));
		CHECK(torus->get_rings() == 19);
		CHECK(torus->get_ring_segments() == 43);
	}
}

TEST_CASE("[SceneTree][Primitive][TubeTrail] TubeTrail Primitive") {
	Ref<PrimitiveMesh> tube = memnew(PrimitiveMesh(PrimitiveMesh::MESH_TUBE_TRAIL));
	CHECK(tube->get_mesh_type() == PrimitiveMesh::MESH_TUBE_TRAIL);

	SUBCASE("[Primitive][TubeTrail] There are valid values of properties on initialization.") {
		CHECK(tube->get_radius() > 0);
		CHECK(tube->get_radial_steps() >= 0);
		CHECK(tube->get_sections() >= 0);
		CHECK(tube->get_section_length() > 0);
		CHECK(tube->get_section_rings() >= 0);
		CHECK(tube->get_curve().is_null());
		CHECK(tube->get_builtin_bind_pose_count() > 0);
	}

	SUBCASE("[Primitive][TubeTrail] Are able to change properties.") {
		tube->set_radius(7.2f);
		tube->set_radial_steps(9);
		tube->set_sections(33);
		tube->set_section_length(5.5f);
		tube->set_section_rings(12);
		Ref<Curve> curve = memnew(Curve);
		tube->set_curve(curve);

		CHECK(tube->get_radius() == doctest::Approx(7.2f));
		CHECK(tube->get_section_length() == doctest::Approx(5.5f));
		CHECK(tube->get_radial_steps() == 9);
		CHECK(tube->get_sections() == 33);
		CHECK(tube->get_section_rings() == 12);
		CHECK(tube->get_curve() == curve);
	}

	SUBCASE("[Primitive][TubeTrail] Setting same curve more than once, it remains the same.") {
		Ref<Curve> curve = memnew(Curve);
		tube->set_curve(curve);
		tube->set_curve(curve);
		tube->set_curve(curve);

		CHECK(tube->get_curve() == curve);
	}

	SUBCASE("[Primitive][TubeTrail] Setting curve, then changing to different curve.") {
		Ref<Curve> curve1 = memnew(Curve);
		Ref<Curve> curve2 = memnew(Curve);
		tube->set_curve(curve1);
		CHECK(tube->get_curve() == curve1);

		tube->set_curve(curve2);
		CHECK(tube->get_curve() == curve2);
	}

	SUBCASE("[Primitive][TubeTrail] Assign same curve to two different tube trails") {
		Ref<PrimitiveMesh> tube2 = memnew(PrimitiveMesh(PrimitiveMesh::MESH_TUBE_TRAIL));
		Ref<Curve> curve = memnew(Curve);
		tube->set_curve(curve);
		tube2->set_curve(curve);

		CHECK(tube->get_curve() == curve);
		CHECK(tube2->get_curve() == curve);
	}
}

TEST_CASE("[SceneTree][Primitive][RibbonTrail] RibbonTrail Primitive") {
	Ref<PrimitiveMesh> ribbon = memnew(PrimitiveMesh(PrimitiveMesh::MESH_RIBBON_TRAIL));
	CHECK(ribbon->get_mesh_type() == PrimitiveMesh::MESH_RIBBON_TRAIL);

	SUBCASE("[Primitive][RibbonTrail] There are valid values of properties on initialization.") {
		CHECK(ribbon->get_ribbon_size() > 0);
		CHECK(ribbon->get_sections() >= 0);
		CHECK(ribbon->get_section_length() > 0);
		CHECK(ribbon->get_section_segments() >= 0);
		CHECK(ribbon->get_builtin_bind_pose_count() > 0);
		CHECK(ribbon->get_curve().is_null());
		CHECK((ribbon->get_ribbon_shape() == PrimitiveMesh::SHAPE_CROSS ||
				ribbon->get_ribbon_shape() == PrimitiveMesh::SHAPE_FLAT));
	}

	SUBCASE("[Primitive][RibbonTrail] Able to change properties.") {
		Ref<Curve> curve = memnew(Curve);
		ribbon->set_ribbon_size(4.3f);
		ribbon->set_sections(16);
		ribbon->set_section_length(1.3f);
		ribbon->set_section_segments(9);
		ribbon->set_curve(curve);

		CHECK(ribbon->get_ribbon_size() == doctest::Approx(4.3f));
		CHECK(ribbon->get_section_length() == doctest::Approx(1.3f));
		CHECK(ribbon->get_sections() == 16);
		CHECK(ribbon->get_section_segments() == 9);
		CHECK(ribbon->get_curve() == curve);
	}

	SUBCASE("[Primitive][RibbonTrail] Setting same curve more than once, it remains the same.") {
		Ref<Curve> curve = memnew(Curve);
		ribbon->set_curve(curve);
		ribbon->set_curve(curve);
		ribbon->set_curve(curve);

		CHECK(ribbon->get_curve() == curve);
	}

	SUBCASE("[Primitive][RibbonTrail] Setting curve, then changing to different curve.") {
		Ref<Curve> curve1 = memnew(Curve);
		Ref<Curve> curve2 = memnew(Curve);
		ribbon->set_curve(curve1);
		CHECK(ribbon->get_curve() == curve1);

		ribbon->set_curve(curve2);
		CHECK(ribbon->get_curve() == curve2);
	}

	SUBCASE("[Primitive][RibbonTrail] Assign same curve to two different ribbon trails") {
		Ref<PrimitiveMesh> ribbon2 = memnew(PrimitiveMesh(PrimitiveMesh::MESH_RIBBON_TRAIL));
		Ref<Curve> curve = memnew(Curve);
		ribbon->set_curve(curve);
		ribbon2->set_curve(curve);

		CHECK(ribbon->get_curve() == curve);
		CHECK(ribbon2->get_curve() == curve);
	}
}

TEST_CASE("[SceneTree][Primitive][Text] Text Primitive") {
	Ref<PrimitiveMesh> text = memnew(PrimitiveMesh(PrimitiveMesh::MESH_TEXT));
	CHECK(text->get_mesh_type() == PrimitiveMesh::MESH_TEXT);

	SUBCASE("[Primitive][Text] There are valid values of properties on initialization.") {
		CHECK((text->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_CENTER ||
				text->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_LEFT ||
				text->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_RIGHT ||
				text->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_FILL));
		CHECK((text->get_vertical_alignment() == VERTICAL_ALIGNMENT_BOTTOM ||
				text->get_vertical_alignment() == VERTICAL_ALIGNMENT_TOP ||
				text->get_vertical_alignment() == VERTICAL_ALIGNMENT_CENTER ||
				text->get_vertical_alignment() == VERTICAL_ALIGNMENT_FILL));
		CHECK(text->get_font().is_null());
		CHECK(text->get_font_size() > 0);
		CHECK(text->get_line_spacing() >= 0);
		CHECK((text->get_autowrap_mode() == TextServer::AUTOWRAP_OFF ||
				text->get_autowrap_mode() == TextServer::AUTOWRAP_ARBITRARY ||
				text->get_autowrap_mode() == TextServer::AUTOWRAP_WORD ||
				text->get_autowrap_mode() == TextServer::AUTOWRAP_WORD_SMART));
		CHECK((text->get_text_direction() == TextServer::DIRECTION_AUTO ||
				text->get_text_direction() == TextServer::DIRECTION_LTR ||
				text->get_text_direction() == TextServer::DIRECTION_RTL));
		CHECK((text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_DEFAULT ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_URI ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_FILE ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_EMAIL ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_LIST ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_GDSCRIPT ||
				text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_CUSTOM));
		CHECK(text->get_width() > 0);
		CHECK(text->get_depth() > 0);
		CHECK(text->get_curve_step() > 0);
		CHECK(text->get_pixel_size() > 0);
	}

	SUBCASE("[Primitive][Text] Change the properties of the mesh.") {
		Ref<Font> font = memnew(Font);
		Point2 offset{ 30.8, 104.23 };
		text->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		text->set_vertical_alignment(VERTICAL_ALIGNMENT_BOTTOM);
		text->set_text("Hello");
		text->set_font(font);
		text->set_font_size(12);
		text->set_line_spacing(1.7f);
		text->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		text->set_text_direction(TextServer::DIRECTION_RTL);
		text->set_language("French");
		text->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_EMAIL);
		text->set_uppercase(true);
		real_t width{ 0.6 };
		real_t depth{ 1.7 };
		real_t pixel_size{ 2.8 };
		real_t curve_step{ 4.8 };
		text->set_width(width);
		text->set_depth(depth);
		text->set_curve_step(curve_step);
		text->set_pixel_size(pixel_size);
		text->set_offset(offset);

		CHECK(text->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_RIGHT);
		CHECK(text->get_vertical_alignment() == VERTICAL_ALIGNMENT_BOTTOM);
		CHECK(text->get_text_direction() == TextServer::DIRECTION_RTL);
		CHECK(text->get_text() == "Hello");
		CHECK(text->get_font() == font);
		CHECK(text->get_font_size() == 12);
		CHECK(text->get_autowrap_mode() == TextServer::AUTOWRAP_WORD_SMART);
		CHECK(text->get_language() == "French");
		CHECK(text->get_structured_text_bidi_override() == TextServer::STRUCTURED_TEXT_EMAIL);
		CHECK(text->is_uppercase() == true);
		CHECK(text->get_offset() == offset);
		CHECK(text->get_line_spacing() == doctest::Approx(1.7f));
		CHECK(text->get_width() == doctest::Approx(width));
		CHECK(text->get_depth() == doctest::Approx(depth));
		CHECK(text->get_curve_step() == doctest::Approx(curve_step));
		CHECK(text->get_pixel_size() == doctest::Approx(pixel_size));
	}

	SUBCASE("[Primitive][Text] Set objects multiple times.") {
		Ref<Font> font = memnew(Font);
		Point2 offset{ 30.8, 104.23 };

		text->set_font(font);
		text->set_font(font);
		text->set_font(font);
		text->set_offset(offset);
		text->set_offset(offset);
		text->set_offset(offset);

		CHECK(text->get_font() == font);
		CHECK(text->get_offset() == offset);
	}

	SUBCASE("[Primitive][Text] Set then change objects.") {
		Ref<Font> font1 = memnew(Font);
		Ref<Font> font2 = memnew(Font);
		Point2 offset1{ 30.8, 104.23 };
		Point2 offset2{ -30.8, -104.23 };

		text->set_font(font1);
		text->set_offset(offset1);

		CHECK(text->get_font() == font1);
		CHECK(text->get_offset() == offset1);

		text->set_font(font2);
		text->set_offset(offset2);

		CHECK(text->get_font() == font2);
		CHECK(text->get_offset() == offset2);
	}

	SUBCASE("[Primitive][Text] Assign same font to two Text meshes.") {
		Ref<PrimitiveMesh> text2 = memnew(PrimitiveMesh(PrimitiveMesh::MESH_TEXT));
		Ref<Font> font = memnew(Font);

		text->set_font(font);
		text2->set_font(font);

		CHECK(text->get_font() == font);
		CHECK(text2->get_font() == font);
	}
}

} // namespace TestPrimitives

#endif // _3D_DISABLED
