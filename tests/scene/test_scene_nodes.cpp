
// === Merged from tests/scene/test_arraymesh.cpp ===
/*  test_arraymesh.cpp                                                    */
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

#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

namespace TestArrayMesh {

TEST_CASE("[SceneTree][ArrayMesh] Adding and modifying blendshapes.") {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	String name_a{ "ShapeA" };
	String name_b{ "ShapeB" };

	SUBCASE("Adding a blend shape to the mesh before a surface is added.") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);

		CHECK(mesh->get_blend_shape_name(0) == name_a);
		CHECK(mesh->get_blend_shape_name(1) == name_b);
	}

	SUBCASE("Add same blend shape multiple times appends name with number.") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_a);

		CHECK(mesh->get_blend_shape_name(0) == "ShapeA");
		bool all_different = (static_cast<String>(mesh->get_blend_shape_name(0)) != static_cast<String>(mesh->get_blend_shape_name(1))) &&
				(static_cast<String>(mesh->get_blend_shape_name(1)) != static_cast<String>(mesh->get_blend_shape_name(2))) &&
				(static_cast<String>(mesh->get_blend_shape_name(0)) != static_cast<String>(mesh->get_blend_shape_name(2)));
		bool all_have_name = static_cast<String>(mesh->get_blend_shape_name(1)).contains("ShapeA") &&
				static_cast<String>(mesh->get_blend_shape_name(2)).contains("ShapeA");
		CHECK((all_different && all_have_name));
	}

	SUBCASE("ArrayMesh keeps correct count of number of blend shapes") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		mesh->add_blend_shape(name_b);
		mesh->add_blend_shape(name_b);

		REQUIRE(mesh->get_blend_shape_count() == 5);
	}

	SUBCASE("Adding blend shape after surface is added causes error") {
		Ref<PrimitiveMesh> cylinder;
		cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
		Array cylinder_array;
		cylinder_array.resize(Mesh::ARRAY_MAX);
		cylinder->create_mesh_array(cylinder_array, 3.f, 3.f, 5.f);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);

		ERR_PRINT_OFF
		mesh->add_blend_shape(name_a);
		ERR_PRINT_ON
		CHECK(mesh->get_blend_shape_count() == 0);
	}

	SUBCASE("Adding blend shapes once all surfaces have been removed is allowed") {
		Ref<PrimitiveMesh> cylinder;
		cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
		Array cylinder_array;
		cylinder_array.resize(Mesh::ARRAY_MAX);
		cylinder->create_mesh_array(cylinder_array, 3.f, 3.f, 5.f);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);

		mesh->surface_remove(0);
		ERR_PRINT_OFF
		mesh->add_blend_shape(name_a);
		ERR_PRINT_ON
		CHECK(mesh->get_blend_shape_count() == 0);

		mesh->surface_remove(0);
		mesh->add_blend_shape(name_a);
		CHECK(mesh->get_blend_shape_count() == 1);
	}

	SUBCASE("Change blend shape name after adding.") {
		mesh->add_blend_shape(name_a);
		mesh->set_blend_shape_name(0, name_b);

		CHECK(mesh->get_blend_shape_name(0) == name_b);
	}

	SUBCASE("Change blend shape name to the name of one already there, should append number to end") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		mesh->set_blend_shape_name(0, name_b);

		String name_string = mesh->get_blend_shape_name(0);
		CHECK(name_string.contains("ShapeB"));
		CHECK(name_string.length() > static_cast<String>(name_b).size());
	}

	SUBCASE("Clear all blend shapes before surface has been added.") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		CHECK(mesh->get_blend_shape_count() == 2);

		mesh->clear_blend_shapes();
		CHECK(mesh->get_blend_shape_count() == 0);
	}

	SUBCASE("Clearing all blend shapes once all surfaces have been removed is allowed") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		Ref<PrimitiveMesh> cylinder;
		cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
		Array cylinder_array;
		cylinder_array.resize(Mesh::ARRAY_MAX);
		cylinder->create_mesh_array(cylinder_array, 3.f, 3.f, 5.f);
		Array blend_shape;
		blend_shape.resize(Mesh::ARRAY_MAX);
		blend_shape[Mesh::ARRAY_VERTEX] = cylinder_array[Mesh::ARRAY_VERTEX];
		blend_shape[Mesh::ARRAY_NORMAL] = cylinder_array[Mesh::ARRAY_NORMAL];
		blend_shape[Mesh::ARRAY_TANGENT] = cylinder_array[Mesh::ARRAY_TANGENT];
		Array blend_shapes = { blend_shape, blend_shape };
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array, blend_shapes);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array, blend_shapes);

		mesh->surface_remove(0);
		ERR_PRINT_OFF
		mesh->clear_blend_shapes();
		ERR_PRINT_ON
		CHECK(mesh->get_blend_shape_count() == 2);

		mesh->surface_remove(0);
		mesh->clear_blend_shapes();
		CHECK(mesh->get_blend_shape_count() == 0);
	}

	SUBCASE("Can't add surface with incorrect number of blend shapes.") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		Array cylinder_array;
		ERR_PRINT_OFF
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);
		ERR_PRINT_ON
		CHECK(mesh->get_surface_count() == 0);
	}

	SUBCASE("Can't clear blend shapes after surface had been added.") {
		mesh->add_blend_shape(name_a);
		mesh->add_blend_shape(name_b);
		Ref<PrimitiveMesh> cylinder;
		cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
		Array cylinder_array;
		cylinder_array.resize(Mesh::ARRAY_MAX);
		cylinder->create_mesh_array(cylinder_array, 3.f, 3.f, 5.f);
		Array blend_shape;
		blend_shape.resize(Mesh::ARRAY_MAX);
		blend_shape[Mesh::ARRAY_VERTEX] = cylinder_array[Mesh::ARRAY_VERTEX];
		blend_shape[Mesh::ARRAY_NORMAL] = cylinder_array[Mesh::ARRAY_NORMAL];
		blend_shape[Mesh::ARRAY_TANGENT] = cylinder_array[Mesh::ARRAY_TANGENT];
		Array blend_shapes = { blend_shape, blend_shape };
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array, blend_shapes);

		ERR_PRINT_OFF
		mesh->clear_blend_shapes();
		ERR_PRINT_ON
		CHECK(mesh->get_blend_shape_count() == 2);
	}

	SUBCASE("Set the blend shape mode of ArrayMesh and underlying mesh RID.") {
		mesh->set_blend_shape_mode(Mesh::BLEND_SHAPE_MODE_RELATIVE);
		CHECK(mesh->get_blend_shape_mode() == Mesh::BLEND_SHAPE_MODE_RELATIVE);
	}
}

TEST_CASE("[SceneTree][ArrayMesh] Surface metadata tests.") {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	Ref<PrimitiveMesh> cylinder;
	cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
	Array cylinder_array;
	cylinder_array.resize(Mesh::ARRAY_MAX);
	cylinder->create_mesh_array(cylinder_array, 3.f, 3.f, 5.f);
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);

	Ref<PrimitiveMesh> box;
	box.instantiate(PrimitiveMesh::MESH_BOX);
	Array box_array;
	box_array.resize(Mesh::ARRAY_MAX);
	box->create_mesh_array(box_array, Vector3(2.f, 1.2f, 1.6f));
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, box_array);

	SUBCASE("Add 2 surfaces and count the number of surfaces in the mesh.") {
		REQUIRE(mesh->get_surface_count() == 2);
	}

	SUBCASE("Get the surface array from mesh.") {
		REQUIRE(mesh->surface_get_arrays(0)[0] == cylinder_array[0]);
		REQUIRE(mesh->surface_get_arrays(1)[0] == box_array[0]);
	}

	SUBCASE("Get the array length of a particular surface.") {
		CHECK(mesh->surface_get_array_len(0) == static_cast<Vector<Vector3>>(cylinder_array[RSE::ARRAY_VERTEX]).size());
		CHECK(mesh->surface_get_array_len(1) == static_cast<Vector<Vector3>>(box_array[RSE::ARRAY_VERTEX]).size());
	}

	SUBCASE("Get the index array length of a particular surface.") {
		CHECK(mesh->surface_get_array_index_len(0) == static_cast<Vector<Vector3>>(cylinder_array[RSE::ARRAY_INDEX]).size());
		CHECK(mesh->surface_get_array_index_len(1) == static_cast<Vector<Vector3>>(box_array[RSE::ARRAY_INDEX]).size());
	}

	SUBCASE("Get correct primitive type") {
		CHECK(mesh->surface_get_primitive_type(0) == Mesh::PRIMITIVE_TRIANGLES);
		CHECK(mesh->surface_get_primitive_type(1) == Mesh::PRIMITIVE_TRIANGLES);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLE_STRIP, box_array);
		CHECK(mesh->surface_get_primitive_type(2) == Mesh::PRIMITIVE_TRIANGLE_STRIP);
	}

	SUBCASE("Returns correct format for the mesh") {
		int format = RSE::ARRAY_FORMAT_BLEND_SHAPE_MASK | RSE::ARRAY_FORMAT_TEX_UV | RSE::ARRAY_FORMAT_INDEX;
		CHECK((mesh->surface_get_format(0) & format) != 0);
		CHECK((mesh->surface_get_format(1) & format) != 0);
	}

	SUBCASE("Set a surface name and retrieve it by name.") {
		mesh->surface_set_name(0, "surf1");
		CHECK(mesh->surface_find_by_name("surf1") == 0);
		CHECK(mesh->surface_get_name(0) == "surf1");
	}

	SUBCASE("Set material to two different surfaces.") {
		Ref<Material> mat;
		mat.instantiate();
		mesh->surface_set_material(0, mat);
		CHECK(mesh->surface_get_material(0) == mat);
		mesh->surface_set_material(1, mat);
		CHECK(mesh->surface_get_material(1) == mat);
	}

	SUBCASE("Set same material multiple times doesn't change material of surface.") {
		Ref<Material> mat;
		mat.instantiate();
		mesh->surface_set_material(0, mat);
		mesh->surface_set_material(0, mat);
		mesh->surface_set_material(0, mat);
		CHECK(mesh->surface_get_material(0) == mat);
	}

	SUBCASE("Set material of surface then change to different material.") {
		Ref<Material> mat1;
		mat1.instantiate();
		Ref<Material> mat2;
		mat2.instantiate();
		mesh->surface_set_material(1, mat1);
		CHECK(mesh->surface_get_material(1) == mat1);
		mesh->surface_set_material(1, mat2);
		CHECK(mesh->surface_get_material(1) == mat2);
	}

	SUBCASE("Get the LOD of the mesh.") {
		Dictionary lod;
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array, TypedArray<Array>(), lod);
		CHECK(mesh->surface_get_lods(2) == lod);
	}

	SUBCASE("Get the blend shape arrays from the mesh.") {
		TypedArray<Array> blend;
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array, blend);
		CHECK(mesh->surface_get_blend_shape_arrays(2) == blend);
	}
}

TEST_CASE("[SceneTree][ArrayMesh] Get/Set mesh metadata and actions") {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	Ref<PrimitiveMesh> cylinder;
	cylinder.instantiate(PrimitiveMesh::MESH_CYLINDER);
	Array cylinder_array;
	cylinder_array.resize(Mesh::ARRAY_MAX);
	constexpr float cylinder_radius = 3.f;
	constexpr float cylinder_height = 5.f;
	cylinder->create_mesh_array(cylinder_array, cylinder_radius, cylinder_radius, cylinder_height);
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, cylinder_array);

	Ref<PrimitiveMesh> box;
	box.instantiate(PrimitiveMesh::MESH_BOX);
	Array box_array;
	box_array.resize(Mesh::ARRAY_MAX);
	const Vector3 box_size = Vector3(2.f, 1.2f, 1.6f);
	box->create_mesh_array(box_array, box_size);
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, box_array);

	SUBCASE("Set the shadow mesh.") {
		Ref<ArrayMesh> shadow;
		shadow.instantiate();
		mesh->set_shadow_mesh(shadow);
		CHECK(mesh->get_shadow_mesh() == shadow);
	}

	SUBCASE("Set the shadow mesh multiple times.") {
		Ref<ArrayMesh> shadow;
		shadow.instantiate();
		mesh->set_shadow_mesh(shadow);
		mesh->set_shadow_mesh(shadow);
		mesh->set_shadow_mesh(shadow);
		mesh->set_shadow_mesh(shadow);
		CHECK(mesh->get_shadow_mesh() == shadow);
	}

	SUBCASE("Set the same shadow mesh on multiple meshes.") {
		Ref<ArrayMesh> shadow;
		shadow.instantiate();
		Ref<ArrayMesh> mesh2;
		mesh2.instantiate();
		mesh->set_shadow_mesh(shadow);
		mesh2->set_shadow_mesh(shadow);

		CHECK(mesh->get_shadow_mesh() == shadow);
		CHECK(mesh2->get_shadow_mesh() == shadow);
	}

	SUBCASE("Set the shadow mesh and then change it.") {
		Ref<ArrayMesh> shadow;
		shadow.instantiate();
		mesh->set_shadow_mesh(shadow);
		CHECK(mesh->get_shadow_mesh() == shadow);
		Ref<ArrayMesh> shadow2;
		shadow2.instantiate();
		mesh->set_shadow_mesh(shadow2);
		CHECK(mesh->get_shadow_mesh() == shadow2);
	}

	SUBCASE("Set custom AABB.") {
		AABB bound;
		mesh->set_custom_aabb(bound);
		CHECK(mesh->get_custom_aabb() == bound);
	}

	SUBCASE("Set custom AABB multiple times.") {
		AABB bound;
		mesh->set_custom_aabb(bound);
		mesh->set_custom_aabb(bound);
		mesh->set_custom_aabb(bound);
		mesh->set_custom_aabb(bound);
		CHECK(mesh->get_custom_aabb() == bound);
	}

	SUBCASE("Set custom AABB then change to another AABB.") {
		AABB bound;
		AABB bound2;
		mesh->set_custom_aabb(bound);
		CHECK(mesh->get_custom_aabb() == bound);
		mesh->set_custom_aabb(bound2);
		CHECK(mesh->get_custom_aabb() == bound2);
	}

	SUBCASE("Clear all surfaces should leave zero count.") {
		mesh->clear_surfaces();
		CHECK(mesh->get_surface_count() == 0);
	}

	SUBCASE("Able to get correct mesh RID.") {
		RID rid = mesh->get_rid();
		CHECK(Renderer::mesh_get_surface_count(rid) == 2);
	}

	SUBCASE("Create surface from raw SurfaceData data.") {
		RID mesh_rid = mesh->get_rid();
		RenderingServerTypes::SurfaceData surface_data = Renderer::mesh_get_surface(mesh_rid, 0);
		Ref<ArrayMesh> mesh2;
		mesh2.instantiate();
		mesh2->add_surface(surface_data.format, Mesh::PRIMITIVE_TRIANGLES, surface_data.vertex_data, surface_data.attribute_data,
				surface_data.skin_data, surface_data.vertex_count, surface_data.index_data, surface_data.index_count, surface_data.aabb);
		CHECK(mesh2->get_surface_count() == 1);
		CHECK(mesh2->surface_get_primitive_type(0) == Mesh::PRIMITIVE_TRIANGLES);
		CHECK((mesh2->surface_get_format(0) & surface_data.format) != 0);
		CHECK(mesh2->get_aabb().is_equal_approx(surface_data.aabb));
	}

	SUBCASE("Removing a surface decreases surface count.") {
		REQUIRE(mesh->get_surface_count() == 2);
		mesh->surface_remove(0);
		CHECK(mesh->get_surface_count() == 1);
		mesh->surface_remove(0);
		CHECK(mesh->get_surface_count() == 0);
	}

	SUBCASE("Remove the first surface and check the mesh's AABB.") {
		REQUIRE(mesh->get_surface_count() >= 1);
		mesh->surface_remove(0);
		const AABB box_aabb = AABB(-box_size / 2, box_size);
		CHECK(mesh->get_aabb().is_equal_approx(box_aabb));
	}

	SUBCASE("Remove the last surface and check the mesh's AABB.") {
		REQUIRE(mesh->get_surface_count() >= 1);
		mesh->surface_remove(mesh->get_surface_count() - 1);
		const AABB cylinder_aabb = AABB(Vector3(-cylinder_radius, -cylinder_height / 2, -cylinder_radius),
				Vector3(2 * cylinder_radius, cylinder_height, 2 * cylinder_radius));
		CHECK(mesh->get_aabb().is_equal_approx(cylinder_aabb));
	}

	SUBCASE("Remove all surfaces and check the mesh's AABB.") {
		while (mesh->get_surface_count()) {
			mesh->surface_remove(0);
		}
		CHECK(mesh->get_aabb() == AABB());
	}

	SUBCASE("Removing a non-existent surface causes error.") {
		ERR_PRINT_OFF
		mesh->surface_remove(42);
		ERR_PRINT_ON
		CHECK(mesh->get_surface_count() == 2);
	}
}

} // namespace TestArrayMesh

#endif // _3D_DISABLED

// === Merged from tests/scene/test_camera_2d.cpp ===
/*  test_camera_2d.cpp                                                    */
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



#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestCamera2D {

TEST_CASE("[SceneTree][Camera2D] Getters and setters") {
	Camera2D *test_camera = memnew(Camera2D);

	SUBCASE("AnchorMode") {
		test_camera->set_anchor_mode(Camera2D::AnchorMode::ANCHOR_MODE_FIXED_TOP_LEFT);
		CHECK(test_camera->get_anchor_mode() == Camera2D::AnchorMode::ANCHOR_MODE_FIXED_TOP_LEFT);
		test_camera->set_anchor_mode(Camera2D::AnchorMode::ANCHOR_MODE_DRAG_CENTER);
		CHECK(test_camera->get_anchor_mode() == Camera2D::AnchorMode::ANCHOR_MODE_DRAG_CENTER);
	}

	SUBCASE("ProcessCallback") {
		test_camera->set_process_callback(Camera2D::Camera2DProcessCallback::CAMERA2D_PROCESS_PHYSICS);
		CHECK(test_camera->get_process_callback() == Camera2D::Camera2DProcessCallback::CAMERA2D_PROCESS_PHYSICS);
		test_camera->set_process_callback(Camera2D::Camera2DProcessCallback::CAMERA2D_PROCESS_IDLE);
		CHECK(test_camera->get_process_callback() == Camera2D::Camera2DProcessCallback::CAMERA2D_PROCESS_IDLE);
	}

	SUBCASE("Drag") {
		constexpr float drag_left_margin = 0.8f;
		constexpr float drag_top_margin = 0.8f;
		constexpr float drag_right_margin = 0.8f;
		constexpr float drag_bottom_margin = 0.8f;
		constexpr float drag_horizontal_offset1 = 0.5f;
		constexpr float drag_horizontal_offset2 = -0.5f;
		constexpr float drag_vertical_offset1 = 0.5f;
		constexpr float drag_vertical_offset2 = -0.5f;
		test_camera->set_drag_margin(SIDE_LEFT, drag_left_margin);
		CHECK(test_camera->get_drag_margin(SIDE_LEFT) == drag_left_margin);
		test_camera->set_drag_margin(SIDE_TOP, drag_top_margin);
		CHECK(test_camera->get_drag_margin(SIDE_TOP) == drag_top_margin);
		test_camera->set_drag_margin(SIDE_RIGHT, drag_right_margin);
		CHECK(test_camera->get_drag_margin(SIDE_RIGHT) == drag_right_margin);
		test_camera->set_drag_margin(SIDE_BOTTOM, drag_bottom_margin);
		CHECK(test_camera->get_drag_margin(SIDE_BOTTOM) == drag_bottom_margin);
		test_camera->set_drag_horizontal_enabled(true);
		CHECK(test_camera->is_drag_horizontal_enabled());
		test_camera->set_drag_horizontal_enabled(false);
		CHECK_FALSE(test_camera->is_drag_horizontal_enabled());
		test_camera->set_drag_horizontal_offset(drag_horizontal_offset1);
		CHECK(test_camera->get_drag_horizontal_offset() == drag_horizontal_offset1);
		test_camera->set_drag_horizontal_offset(drag_horizontal_offset2);
		CHECK(test_camera->get_drag_horizontal_offset() == drag_horizontal_offset2);
		test_camera->set_drag_vertical_enabled(true);
		CHECK(test_camera->is_drag_vertical_enabled());
		test_camera->set_drag_vertical_enabled(false);
		CHECK_FALSE(test_camera->is_drag_vertical_enabled());
		test_camera->set_drag_vertical_offset(drag_vertical_offset1);
		CHECK(test_camera->get_drag_vertical_offset() == drag_vertical_offset1);
		test_camera->set_drag_vertical_offset(drag_vertical_offset2);
		CHECK(test_camera->get_drag_vertical_offset() == drag_vertical_offset2);
	}

	SUBCASE("Drawing") {
		test_camera->set_margin_drawing_enabled(true);
		CHECK(test_camera->is_margin_drawing_enabled());
		test_camera->set_margin_drawing_enabled(false);
		CHECK_FALSE(test_camera->is_margin_drawing_enabled());
		test_camera->set_limit_drawing_enabled(true);
		CHECK(test_camera->is_limit_drawing_enabled());
		test_camera->set_limit_drawing_enabled(false);
		CHECK_FALSE(test_camera->is_limit_drawing_enabled());
		test_camera->set_screen_drawing_enabled(true);
		CHECK(test_camera->is_screen_drawing_enabled());
		test_camera->set_screen_drawing_enabled(false);
		CHECK_FALSE(test_camera->is_screen_drawing_enabled());
	}

	SUBCASE("Enabled") {
		test_camera->set_enabled(true);
		CHECK(test_camera->is_enabled());
		test_camera->set_enabled(false);
		CHECK_FALSE(test_camera->is_enabled());
	}

	SUBCASE("Rotation") {
		constexpr float rotation_smoothing_speed = 20.0f;
		test_camera->set_ignore_rotation(true);
		CHECK(test_camera->is_ignoring_rotation());
		test_camera->set_ignore_rotation(false);
		CHECK_FALSE(test_camera->is_ignoring_rotation());
		test_camera->set_rotation_smoothing_enabled(true);
		CHECK(test_camera->is_rotation_smoothing_enabled());
		test_camera->set_rotation_smoothing_speed(rotation_smoothing_speed);
		CHECK(test_camera->get_rotation_smoothing_speed() == rotation_smoothing_speed);
	}

	SUBCASE("Zoom") {
		const Vector2 zoom = Vector2(4, 4);
		test_camera->set_zoom(zoom);
		CHECK(test_camera->get_zoom() == zoom);
	}

	SUBCASE("Offset") {
		const Vector2 offset = Vector2(100, 100);
		test_camera->set_offset(offset);
		CHECK(test_camera->get_offset() == offset);
	}

	SUBCASE("Limit") {
		constexpr int limit_left = 100;
		constexpr int limit_top = 100;
		constexpr int limit_right = 100;
		constexpr int limit_bottom = 100;
		test_camera->set_limit_smoothing_enabled(true);
		CHECK(test_camera->is_limit_smoothing_enabled());
		test_camera->set_limit_smoothing_enabled(false);
		CHECK_FALSE(test_camera->is_limit_smoothing_enabled());
		test_camera->set_limit(SIDE_LEFT, limit_left);
		CHECK(test_camera->get_limit(SIDE_LEFT) == limit_left);
		test_camera->set_limit(SIDE_TOP, limit_top);
		CHECK(test_camera->get_limit(SIDE_TOP) == limit_top);
		test_camera->set_limit(SIDE_RIGHT, limit_right);
		CHECK(test_camera->get_limit(SIDE_RIGHT) == limit_right);
		test_camera->set_limit(SIDE_BOTTOM, limit_bottom);
		CHECK(test_camera->get_limit(SIDE_BOTTOM) == limit_bottom);
	}

	SUBCASE("Position") {
		constexpr float smoothing_speed = 20.0f;
		test_camera->set_position_smoothing_enabled(true);
		CHECK(test_camera->is_position_smoothing_enabled());
		test_camera->set_position_smoothing_speed(smoothing_speed);
		CHECK(test_camera->get_position_smoothing_speed() == smoothing_speed);
	}

	memdelete(test_camera);
}

TEST_CASE("[SceneTree][Camera2D] Camera positioning") {
	SubViewport *mock_viewport = memnew(SubViewport);
	Camera2D *test_camera = memnew(Camera2D);

	mock_viewport->set_size(Vector2(400, 200));
	SceneTree::get_root()->add_child(mock_viewport);
	mock_viewport->add_child(test_camera);

	SUBCASE("Anchor mode") {
		test_camera->set_anchor_mode(Camera2D::ANCHOR_MODE_DRAG_CENTER);
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(0, 0)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));

		test_camera->set_anchor_mode(Camera2D::ANCHOR_MODE_FIXED_TOP_LEFT);
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(200, 100)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));
	}

	SUBCASE("Offset") {
		test_camera->set_offset(Vector2(100, 100));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(100, 100)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));

		test_camera->set_offset(Vector2(-100, 300));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(-100, 300)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));

		test_camera->set_offset(Vector2(0, 0));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(0, 0)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));
	}

	SUBCASE("Limits") {
		test_camera->set_limit(SIDE_LEFT, 100);
		test_camera->set_limit(SIDE_TOP, 50);

		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(300, 150)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));

		test_camera->set_limit(SIDE_LEFT, 0);
		test_camera->set_limit(SIDE_TOP, 0);

		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(200, 100)));
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));
	}

	SUBCASE("Drag") {
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(0, 0)));

		// horizontal
		test_camera->set_drag_horizontal_enabled(true);
		test_camera->set_drag_margin(SIDE_RIGHT, 0.5);

		test_camera->set_position(Vector2(100, 100));
		test_camera->force_update_scroll();
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 100)));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(0, 100)));
		test_camera->set_position(Vector2(101, 101));
		test_camera->force_update_scroll();
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(1, 101)));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(1, 101)));

		// test align
		test_camera->set_position(Vector2(0, 0));
		test_camera->align();
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(0, 0)));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(0, 0)));

		// vertical
		test_camera->set_drag_vertical_enabled(true);
		test_camera->set_drag_horizontal_enabled(false);
		test_camera->set_drag_margin(SIDE_TOP, 0.3);

		test_camera->set_position(Vector2(200, -20));
		test_camera->force_update_scroll();
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(200, 0)));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(200, 0)));
		test_camera->set_position(Vector2(250, -55));
		test_camera->force_update_scroll();
		CHECK(test_camera->get_camera_position().is_equal_approx(Vector2(250, -25)));
		CHECK(test_camera->get_camera_screen_center().is_equal_approx(Vector2(250, -25)));
	}

	memdelete(test_camera);
	memdelete(mock_viewport);
}

TEST_CASE("[SceneTree][Camera2D] Transforms") {
	SubViewport *mock_viewport = memnew(SubViewport);
	Camera2D *test_camera = memnew(Camera2D);

	mock_viewport->set_size(Vector2(400, 200));
	SceneTree::get_root()->add_child(mock_viewport);
	mock_viewport->add_child(test_camera);

	SUBCASE("Default camera") {
		Transform2D xform = mock_viewport->get_canvas_transform();
		// x,y are basis vectors, origin = screen center
		Transform2D test_xform = Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));
	}

	SUBCASE("Zoom") {
		test_camera->set_zoom(Vector2(0.5, 2));
		Transform2D xform = mock_viewport->get_canvas_transform();
		Transform2D test_xform = Transform2D(Vector2(0.5, 0), Vector2(0, 2), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));

		test_camera->set_zoom(Vector2(10, 10));
		xform = mock_viewport->get_canvas_transform();
		test_xform = Transform2D(Vector2(10, 0), Vector2(0, 10), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));

		test_camera->set_zoom(Vector2(1, 1));
		xform = mock_viewport->get_canvas_transform();
		test_xform = Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));
	}

	SUBCASE("Rotation") {
		test_camera->set_rotation(Math::PI / 2);
		Transform2D xform = mock_viewport->get_canvas_transform();
		Transform2D test_xform = Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));

		test_camera->set_ignore_rotation(false);
		xform = mock_viewport->get_canvas_transform();
		test_xform = Transform2D(Vector2(0, -1), Vector2(1, 0), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));

		test_camera->set_rotation(-1 * Math::PI);
		test_camera->force_update_scroll();
		xform = mock_viewport->get_canvas_transform();
		test_xform = Transform2D(Vector2(-1, 0), Vector2(0, -1), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));

		test_camera->set_rotation(0);
		test_camera->force_update_scroll();
		xform = mock_viewport->get_canvas_transform();
		test_xform = Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(200, 100));
		CHECK(xform.is_equal_approx(test_xform));
	}

	memdelete(test_camera);
	memdelete(mock_viewport);
}

} // namespace TestCamera2D

// === Merged from tests/scene/test_camera_3d.cpp ===
/*  test_camera_3d.cpp                                                    */
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

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestCamera3D {

TEST_CASE("[SceneTree][Camera3D] Getters and setters") {
	Camera3D *test_camera = memnew(Camera3D);

	SUBCASE("Cull mask") {
		constexpr int cull_mask = (1 << 5) | (1 << 7) | (1 << 9);
		constexpr int set_enable_layer = 3;
		constexpr int set_disable_layer = 5;
		test_camera->set_cull_mask(cull_mask);
		CHECK(test_camera->get_cull_mask() == cull_mask);
		test_camera->set_cull_mask_value(set_enable_layer, true);
		CHECK(test_camera->get_cull_mask_value(set_enable_layer));
		test_camera->set_cull_mask_value(set_disable_layer, false);
		CHECK_FALSE(test_camera->get_cull_mask_value(set_disable_layer));
	}

	SUBCASE("Attributes") {
		Ref<CameraAttributes> attributes = memnew(CameraAttributes);
		test_camera->set_attributes(attributes);
		CHECK(test_camera->get_attributes() == attributes);
		Ref<CameraAttributesPhysical> physical_attributes = memnew(CameraAttributesPhysical);
		test_camera->set_attributes(physical_attributes);
		CHECK(test_camera->get_attributes() == physical_attributes);
	}

	SUBCASE("Camera frustum properties") {
		constexpr float depth_near = 0.2f;
		constexpr float depth_far = 995.0f;
		constexpr float fov = 120.0f;
		constexpr float size = 7.0f;
		constexpr float h_offset = 1.1f;
		constexpr float v_offset = -1.6f;
		const Vector2 frustum_offset(5, 7);
		test_camera->set_near(depth_near);
		CHECK(test_camera->get_near() == depth_near);
		test_camera->set_far(depth_far);
		CHECK(test_camera->get_far() == depth_far);
		test_camera->set_fov(fov);
		CHECK(test_camera->get_fov() == fov);
		test_camera->set_size(size);
		CHECK(test_camera->get_size() == size);
		test_camera->set_h_offset(h_offset);
		CHECK(test_camera->get_h_offset() == h_offset);
		test_camera->set_v_offset(v_offset);
		CHECK(test_camera->get_v_offset() == v_offset);
		test_camera->set_frustum_offset(frustum_offset);
		CHECK(test_camera->get_frustum_offset() == frustum_offset);
		test_camera->set_keep_aspect_mode(Camera3D::KeepAspect::KEEP_HEIGHT);
		CHECK(test_camera->get_keep_aspect_mode() == Camera3D::KeepAspect::KEEP_HEIGHT);
		test_camera->set_keep_aspect_mode(Camera3D::KeepAspect::KEEP_WIDTH);
		CHECK(test_camera->get_keep_aspect_mode() == Camera3D::KeepAspect::KEEP_WIDTH);
	}

	SUBCASE("Projection mode") {
		test_camera->set_projection(Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
		CHECK(test_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
		test_camera->set_projection(Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
		CHECK(test_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
	}

	SUBCASE("Helper setters") {
		constexpr float fov = 90.0f, size = 6.0f;
		constexpr float near1 = 0.1f, near2 = 0.5f;
		constexpr float far1 = 1001.0f, far2 = 1005.0f;
		test_camera->set_perspective(fov, near1, far1);
		CHECK(test_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
		CHECK(test_camera->get_near() == near1);
		CHECK(test_camera->get_far() == far1);
		CHECK(test_camera->get_fov() == fov);
		test_camera->set_orthogonal(size, near2, far2);
		CHECK(test_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
		CHECK(test_camera->get_near() == near2);
		CHECK(test_camera->get_far() == far2);
		CHECK(test_camera->get_size() == size);
	}

	SUBCASE("Doppler tracking") {
		test_camera->set_doppler_tracking(Camera3D::DopplerTracking::DOPPLER_TRACKING_IDLE_STEP);
		CHECK(test_camera->get_doppler_tracking() == Camera3D::DopplerTracking::DOPPLER_TRACKING_IDLE_STEP);
		test_camera->set_doppler_tracking(Camera3D::DopplerTracking::DOPPLER_TRACKING_PHYSICS_STEP);
		CHECK(test_camera->get_doppler_tracking() == Camera3D::DopplerTracking::DOPPLER_TRACKING_PHYSICS_STEP);
		test_camera->set_doppler_tracking(Camera3D::DopplerTracking::DOPPLER_TRACKING_DISABLED);
		CHECK(test_camera->get_doppler_tracking() == Camera3D::DopplerTracking::DOPPLER_TRACKING_DISABLED);
	}

	memdelete(test_camera);
}

TEST_CASE("[SceneTree][Camera3D] Position queries") {
	// Cameras need a viewport to know how to compute their frustums, so we make a fake one here.
	Camera3D *test_camera = memnew(Camera3D);
	SubViewport *mock_viewport = memnew(SubViewport);
	// 4:2.
	mock_viewport->set_size(Vector2(400, 200));
	SceneTree::get_root()->add_child(mock_viewport);
	mock_viewport->add_child(test_camera);
	test_camera->set_keep_aspect_mode(Camera3D::KeepAspect::KEEP_WIDTH);
	REQUIRE_MESSAGE(test_camera->is_current(), "Camera3D should be made current upon entering tree.");

	SUBCASE("Orthogonal projection") {
		test_camera->set_projection(Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
		// The orthogonal case is simpler, so we test a more random position + rotation combination here.
		// For the other cases we'll use zero translation and rotation instead.
		test_camera->set_global_position(Vector3(1, 2, 3));
		test_camera->look_at(Vector3(-4, 5, 1));
		// Width = 5, Aspect Ratio = 400 / 200 = 2, so Height is 2.5.
		test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
		const Basis basis = test_camera->get_global_basis();
		// Subtract near so offset starts from the near plane.
		const Vector3 offset1 = basis.xform(Vector3(-1.5f, 3.5f, 0.2f - test_camera->get_near()));
		const Vector3 offset2 = basis.xform(Vector3(2.0f, -0.5f, -0.6f - test_camera->get_near()));
		const Vector3 offset3 = basis.xform(Vector3(-3.0f, 1.0f, -0.6f - test_camera->get_near()));
		const Vector3 offset4 = basis.xform(Vector3(-2.0f, 1.5f, -0.6f - test_camera->get_near()));
		const Vector3 offset5 = basis.xform(Vector3(0, 0, 10000.0f - test_camera->get_near()));

		SUBCASE("is_position_behind") {
			CHECK(test_camera->is_position_behind(test_camera->get_global_position() + offset1));
			CHECK_FALSE(test_camera->is_position_behind(test_camera->get_global_position() + offset2));

			SUBCASE("h/v offset should have no effect on the result of is_position_behind") {
				test_camera->set_h_offset(-11.0f);
				test_camera->set_v_offset(22.1f);
				CHECK(test_camera->is_position_behind(test_camera->get_global_position() + offset1));
				test_camera->set_h_offset(4.7f);
				test_camera->set_v_offset(-3.0f);
				CHECK_FALSE(test_camera->is_position_behind(test_camera->get_global_position() + offset2));
			}
			// Reset h/v offsets.
			test_camera->set_h_offset(0);
			test_camera->set_v_offset(0);
		}

		SUBCASE("is_position_in_frustum") {
			// If the point is behind the near plane, it is outside the camera frustum.
			// So offset1 is not in frustum.
			CHECK_FALSE(test_camera->is_position_in_frustum(test_camera->get_global_position() + offset1));
			// If |right| > 5 / 2 or |up| > 2.5 / 2, the point is outside the camera frustum.
			// So offset2 is in frustum and offset3 and offset4 are not.
			CHECK(test_camera->is_position_in_frustum(test_camera->get_global_position() + offset2));
			CHECK_FALSE(test_camera->is_position_in_frustum(test_camera->get_global_position() + offset3));
			CHECK_FALSE(test_camera->is_position_in_frustum(test_camera->get_global_position() + offset4));
			// offset5 is beyond the far plane, so it is not in frustum.
			CHECK_FALSE(test_camera->is_position_in_frustum(test_camera->get_global_position() + offset5));
		}
	}

	SUBCASE("Perspective projection") {
		test_camera->set_projection(Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
		// Camera at origin, looking at +Z.
		test_camera->set_global_position(Vector3(0, 0, 0));
		test_camera->set_global_rotation(Vector3(0, 0, 0));
		// Keep width, so horizontal fov = 120.
		// Since the near plane distance is 1,
		// with trig we know the near plane's width is 2 * sqrt(3), so its height is sqrt(3).
		test_camera->set_perspective(120.0f, 1.0f, 1000.0f);

		SUBCASE("is_position_behind") {
			CHECK_FALSE(test_camera->is_position_behind(Vector3(0, 0, -1.5f)));
			CHECK(test_camera->is_position_behind(Vector3(2, 0, -0.2f)));
		}

		SUBCASE("is_position_in_frustum") {
			CHECK(test_camera->is_position_in_frustum(Vector3(-1.3f, 0, -1.1f)));
			CHECK_FALSE(test_camera->is_position_in_frustum(Vector3(2, 0, -1.1f)));
			CHECK(test_camera->is_position_in_frustum(Vector3(1, 0.5f, -1.1f)));
			CHECK_FALSE(test_camera->is_position_in_frustum(Vector3(1, 1, -1.1f)));
			CHECK(test_camera->is_position_in_frustum(Vector3(0, 0, -1.5f)));
			CHECK_FALSE(test_camera->is_position_in_frustum(Vector3(0, 0, -0.5f)));
		}
	}

	memdelete(test_camera);
	memdelete(mock_viewport);
}

TEST_CASE("[SceneTree][Camera3D] Project/Unproject position") {
	// Cameras need a viewport to know how to compute their frustums, so we make a fake one here.
	Camera3D *test_camera = memnew(Camera3D);
	SubViewport *mock_viewport = memnew(SubViewport);
	// 4:2.
	mock_viewport->set_size(Vector2(400, 200));
	SceneTree::get_root()->add_child(mock_viewport);
	mock_viewport->add_child(test_camera);
	test_camera->set_global_position(Vector3(0, 0, 0));
	test_camera->set_global_rotation(Vector3(0, 0, 0));
	test_camera->set_keep_aspect_mode(Camera3D::KeepAspect::KEEP_HEIGHT);

	SUBCASE("project_position") {
		SUBCASE("Orthogonal projection") {
			test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_position(Vector2(200, 100), 0.5f).is_equal_approx(Vector3(0, 0, -0.5f)));
			CHECK(test_camera->project_position(Vector2(200, 100), test_camera->get_far()).is_equal_approx(Vector3(0, 0, -test_camera->get_far())));
			// Top left.
			CHECK(test_camera->project_position(Vector2(0, 0), 1.5f).is_equal_approx(Vector3(-5.0f, 2.5f, -1.5f)));
			CHECK(test_camera->project_position(Vector2(0, 0), test_camera->get_near()).is_equal_approx(Vector3(-5.0f, 2.5f, -test_camera->get_near())));
			// Bottom right.
			CHECK(test_camera->project_position(Vector2(400, 200), 5.0f).is_equal_approx(Vector3(5.0f, -2.5f, -5.0f)));
			CHECK(test_camera->project_position(Vector2(400, 200), test_camera->get_far()).is_equal_approx(Vector3(5.0f, -2.5f, -test_camera->get_far())));
		}

		SUBCASE("Perspective projection") {
			test_camera->set_perspective(120.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_position(Vector2(200, 100), 0.5f).is_equal_approx(Vector3(0, 0, -0.5f)));
			CHECK(test_camera->project_position(Vector2(200, 100), 100.0f).is_equal_approx(Vector3(0, 0, -100.0f)));
			CHECK(test_camera->project_position(Vector2(200, 100), test_camera->get_far()).is_equal_approx(Vector3(0, 0, -1.0f) * test_camera->get_far()));
			// 3/4th way to Top left.
			CHECK(test_camera->project_position(Vector2(100, 50), 0.5f).is_equal_approx(Vector3(-Math::SQRT3 * 0.5f, Math::SQRT3 * 0.25f, -0.5f)));
			CHECK(test_camera->project_position(Vector2(100, 50), 1.0f).is_equal_approx(Vector3(-Math::SQRT3, Math::SQRT3 * 0.5f, -1.0f)));
			CHECK(test_camera->project_position(Vector2(100, 50), test_camera->get_near()).is_equal_approx(Vector3(-Math::SQRT3, Math::SQRT3 * 0.5f, -1.0f) * test_camera->get_near()));
			// 3/4th way to Bottom right.
			CHECK(test_camera->project_position(Vector2(300, 150), 0.5f).is_equal_approx(Vector3(Math::SQRT3 * 0.5f, -Math::SQRT3 * 0.25f, -0.5f)));
			CHECK(test_camera->project_position(Vector2(300, 150), 1.0f).is_equal_approx(Vector3(Math::SQRT3, -Math::SQRT3 * 0.5f, -1.0f)));
			CHECK(test_camera->project_position(Vector2(300, 150), test_camera->get_far()).is_equal_approx(Vector3(Math::SQRT3, -Math::SQRT3 * 0.5f, -1.0f) * test_camera->get_far()));
		}
	}

	// Uses cases that are the inverse of the above sub-case.
	SUBCASE("unproject_position") {
		SUBCASE("Orthogonal projection") {
			test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
			// Center
			CHECK(test_camera->unproject_position(Vector3(0, 0, -0.5f)).is_equal_approx(Vector2(200, 100)));
			// Top left
			CHECK(test_camera->unproject_position(Vector3(-5.0f, 2.5f, -1.5f)).is_equal_approx(Vector2(0, 0)));
			// Bottom right
			CHECK(test_camera->unproject_position(Vector3(5.0f, -2.5f, -5.0f)).is_equal_approx(Vector2(400, 200)));
		}

		SUBCASE("Perspective projection") {
			test_camera->set_perspective(120.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->unproject_position(Vector3(0, 0, -0.5f)).is_equal_approx(Vector2(200, 100)));
			CHECK(test_camera->unproject_position(Vector3(0, 0, -100.0f)).is_equal_approx(Vector2(200, 100)));
			// 3/4th way to Top left.
			WARN(test_camera->unproject_position(Vector3(-Math::SQRT3 * 0.5f, Math::SQRT3 * 0.25f, -0.5f)).is_equal_approx(Vector2(100, 50)));
			WARN(test_camera->unproject_position(Vector3(-Math::SQRT3, Math::SQRT3 * 0.5f, -1.0f)).is_equal_approx(Vector2(100, 50)));
			// 3/4th way to Bottom right.
			CHECK(test_camera->unproject_position(Vector3(Math::SQRT3 * 0.5f, -Math::SQRT3 * 0.25f, -0.5f)).is_equal_approx(Vector2(300, 150)));
			CHECK(test_camera->unproject_position(Vector3(Math::SQRT3, -Math::SQRT3 * 0.5f, -1.0f)).is_equal_approx(Vector2(300, 150)));
		}
	}

	memdelete(test_camera);
	memdelete(mock_viewport);
}

TEST_CASE("[SceneTree][Camera3D] Project ray") {
	// Cameras need a viewport to know how to compute their frustums, so we make a fake one here.
	Camera3D *test_camera = memnew(Camera3D);
	SubViewport *mock_viewport = memnew(SubViewport);
	// 4:2.
	mock_viewport->set_size(Vector2(400, 200));
	SceneTree::get_root()->add_child(mock_viewport);
	mock_viewport->add_child(test_camera);
	test_camera->set_global_position(Vector3(0, 0, 0));
	test_camera->set_global_rotation(Vector3(0, 0, 0));
	test_camera->set_keep_aspect_mode(Camera3D::KeepAspect::KEEP_HEIGHT);

	SUBCASE("project_ray_origin") {
		SUBCASE("Orthogonal projection") {
			test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_ray_origin(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, -0.5f)));
			// Top left.
			CHECK(test_camera->project_ray_origin(Vector2(0, 0)).is_equal_approx(Vector3(-5.0f, 2.5f, -0.5f)));
			// Bottom right.
			CHECK(test_camera->project_ray_origin(Vector2(400, 200)).is_equal_approx(Vector3(5.0f, -2.5f, -0.5f)));
		}

		SUBCASE("Perspective projection") {
			test_camera->set_perspective(120.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_ray_origin(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, 0)));
			// Top left.
			CHECK(test_camera->project_ray_origin(Vector2(0, 0)).is_equal_approx(Vector3(0, 0, 0)));
			// Bottom right.
			CHECK(test_camera->project_ray_origin(Vector2(400, 200)).is_equal_approx(Vector3(0, 0, 0)));
		}
	}

	SUBCASE("project_ray_normal") {
		SUBCASE("Orthogonal projection") {
			test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_ray_normal(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, -1)));
			// Top left.
			CHECK(test_camera->project_ray_normal(Vector2(0, 0)).is_equal_approx(Vector3(0, 0, -1)));
			// Bottom right.
			CHECK(test_camera->project_ray_normal(Vector2(400, 200)).is_equal_approx(Vector3(0, 0, -1)));
		}

		SUBCASE("Perspective projection") {
			test_camera->set_perspective(120.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_ray_normal(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, -1)));
			// Top left.
			CHECK(test_camera->project_ray_normal(Vector2(0, 0)).is_equal_approx(Vector3(-Math::SQRT3, Math::SQRT3 / 2, -0.5f).normalized()));
			// Bottom right.
			CHECK(test_camera->project_ray_normal(Vector2(400, 200)).is_equal_approx(Vector3(Math::SQRT3, -Math::SQRT3 / 2, -0.5f).normalized()));
		}
	}

	SUBCASE("project_local_ray_normal") {
		test_camera->set_rotation_degrees(Vector3(60, 60, 60));

		SUBCASE("Orthogonal projection") {
			test_camera->set_orthogonal(5.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_local_ray_normal(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, -1)));
			// Top left.
			CHECK(test_camera->project_local_ray_normal(Vector2(0, 0)).is_equal_approx(Vector3(0, 0, -1)));
			// Bottom right.
			CHECK(test_camera->project_local_ray_normal(Vector2(400, 200)).is_equal_approx(Vector3(0, 0, -1)));
		}

		SUBCASE("Perspective projection") {
			test_camera->set_perspective(120.0f, 0.5f, 1000.0f);
			// Center.
			CHECK(test_camera->project_local_ray_normal(Vector2(200, 100)).is_equal_approx(Vector3(0, 0, -1)));
			// Top left.
			CHECK(test_camera->project_local_ray_normal(Vector2(0, 0)).is_equal_approx(Vector3(-Math::SQRT3, Math::SQRT3 / 2, -0.5f).normalized()));
			// Bottom right.
			CHECK(test_camera->project_local_ray_normal(Vector2(400, 200)).is_equal_approx(Vector3(Math::SQRT3, -Math::SQRT3 / 2, -0.5f).normalized()));
		}
	}

	memdelete(test_camera);
	memdelete(mock_viewport);
}

} // namespace TestCamera3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_convert_transform_modifier_3d.cpp ===
/*  test_convert_transform_modifier_3d.cpp                                */
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

#include "core/types.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestConvertTransformModifier3D {

Transform3D make_random_transform_3d(int p_seed) {
	RandomNumberGenerator rng;
	rng.set_seed(p_seed);

	Vector3 pos;
	pos.x = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);
	pos.y = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);
	pos.z = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);

	Quaternion rot;
	rot.x = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.y = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.z = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.w = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot = rot.normalized();

	Vector3 scl;
	scl.x = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);
	scl.y = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);
	scl.z = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);

	return Transform3D(Basis(rot).scaled(scl), pos);
}

TEST_CASE("[SceneTree][ConvertTransformModifier3D]") {
	int seed = 12345;
	Skeleton3D *skeleton = memnew(Skeleton3D);
	BoneConstraint3D *mod = memnew(BoneConstraint3D(BoneConstraint3D::CONSTRAINT_CONVERT_TRANSFORM));

	// Instead of awaiting the process to wait to finish deferred process and watch "skeleton_updated" signal,
	// force notify NOTIFICATION_UPDATE_SKELETON and get the modified pose from the BoneAttachment's transform.
	BoneAttachment3D *modified = memnew(BoneAttachment3D);

	SceneTree::get_root()->add_child(skeleton);

	int root = skeleton->add_bone("root");
	skeleton->set_bone_rest(root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(root, make_random_transform_3d(++seed));

	int apl_root = skeleton->add_bone("apl_root");
	skeleton->set_bone_parent(apl_root, root);
	skeleton->set_bone_rest(apl_root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(apl_root, make_random_transform_3d(++seed));

	int apl_bone = skeleton->add_bone("apl_bone");
	skeleton->set_bone_parent(apl_bone, apl_root);
	skeleton->set_bone_rest(apl_bone, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(apl_bone, make_random_transform_3d(++seed));

	int tgt_root = skeleton->add_bone("tgt_root");
	skeleton->set_bone_parent(tgt_root, root);
	skeleton->set_bone_rest(tgt_root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(tgt_root, make_random_transform_3d(++seed));

	int tgt_bone = skeleton->add_bone("tgt_bone");
	skeleton->set_bone_parent(tgt_bone, tgt_root);
	skeleton->set_bone_rest(tgt_bone, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(tgt_bone, make_random_transform_3d(++seed));

	skeleton->add_child(mod);
	skeleton->add_child(modified);
	modified->set_rotation_edit_mode(Node3D::ROTATION_EDIT_MODE_QUATERNION);
	modified->set_bone_idx(apl_bone);

	mod->set_setting_count(1);
	mod->set_reference_bone(0, tgt_bone);
	mod->set_apply_bone(0, apl_bone);

	mod->set_reference_axis(0, Vector3::AXIS_X);
	mod->set_apply_axis(0, Vector3::AXIS_Y);

	// ===== [ConvertTransformModifier3D] Position x to y =====
	mod->set_reference_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_POSITION);
	mod->set_reference_range_min(0, -100.0);
	mod->set_reference_range_max(0, 100.0);
	mod->set_apply_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_POSITION);
	mod->set_apply_range_min(0, -100.0);
	mod->set_apply_range_max(0, 100.0);

	SUBCASE("[ConvertTransformModifier3D] Position x to y, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				skeleton->get_bone_pose_position(tgt_bone).x,
				(skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin.y));
	}

	SUBCASE("[ConvertTransformModifier3D] Position x to y, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				skeleton->get_bone_pose_position(tgt_bone).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).y));
	}

	SUBCASE("[ConvertTransformModifier3D] Position x to y, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin).y));
	}

	SUBCASE("[ConvertTransformModifier3D] Position x to y, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).y));
	}

	// ===== [ConvertTransformModifier3D] Rotation (roll) x to y =====
	mod->set_reference_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_ROTATION);
	mod->set_reference_range_min(0, -180.0);
	mod->set_reference_range_max(0, 180.0);
	mod->set_apply_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_ROTATION);
	mod->set_apply_range_min(0, -180.0);
	mod->set_apply_range_max(0, 180.0);

	SUBCASE("[ConvertTransformModifier3D] Rotation (roll) x to y, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
				BoneConstraint3D::get_roll_angle((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Y))));
	}

	SUBCASE("[ConvertTransformModifier3D] Rotation (roll) x to y, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Y))));
	}

	SUBCASE("[ConvertTransformModifier3D] Rotation (roll) x to y, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Y))));
	}

	SUBCASE("[ConvertTransformModifier3D] Rotation (roll) x to y, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
				BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Y))));
	}

	// ===== [ConvertTransformModifier3D] Scale x to y =====
	mod->set_reference_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_SCALE);
	mod->set_reference_range_min(0, 0);
	mod->set_reference_range_max(0, 10.0);
	mod->set_apply_transform_mode(0, BoneConstraint3D::TRANSFORM_MODE_SCALE);
	mod->set_apply_range_min(0, 0);
	mod->set_apply_range_max(0, 10.0);

	SUBCASE("[ConvertTransformModifier3D] Scale x to y, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				skeleton->get_bone_pose_scale(tgt_bone).x,
				(skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale().y));
	}

	SUBCASE("[ConvertTransformModifier3D] Scale x to y, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				skeleton->get_bone_pose_scale(tgt_bone).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).y));
	}

	SUBCASE("[ConvertTransformModifier3D] Scale x to y, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()).y));
	}

	SUBCASE("[ConvertTransformModifier3D] Scale x to y, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK(Math::is_equal_approx(
				(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
				((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).y));
	}

	memdelete(modified);
	memdelete(mod);
	memdelete(skeleton);
}

} // namespace TestConvertTransformModifier3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_copy_transform_modifier_3d.cpp ===
/*  test_copy_transform_modifier_3d.cpp                                   */
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

#include "core/types.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestCopyTransformModifier3D {

Transform3D make_random_transform_3d(int p_seed) {
	RandomNumberGenerator rng;
	rng.set_seed(p_seed);

	Vector3 pos;
	pos.x = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);
	pos.y = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);
	pos.z = rng.randf_range(-10.0, 10.0);
	rng.set_seed(++p_seed);

	Quaternion rot;
	rot.x = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.y = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.z = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot.w = rng.randf_range(-1.0, 1.0);
	rng.set_seed(++p_seed);
	rot = rot.normalized();

	Vector3 scl;
	scl.x = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);
	scl.y = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);
	scl.z = rng.randf_range(0.5, 2.0);
	rng.set_seed(++p_seed);

	return Transform3D(Basis(rot).scaled(scl), pos);
}

Vector3 flip_x(Vector3 p_pos) {
	return Vector3(-p_pos.x, p_pos.y, p_pos.z);
}

Vector3 flip_xy(Vector3 p_pos) {
	return Vector3(-p_pos.x, -p_pos.y, p_pos.z);
}

Vector3 flip_all(Vector3 p_pos) {
	return -p_pos;
}

// Quaternion's phase can be confused by inversion, it is aligned via casting to Basis.

Quaternion flip_x(Quaternion p_rot) {
	return Basis(Quaternion(-p_rot.x, p_rot.y, p_rot.z, p_rot.w).normalized()).get_rotation_quaternion();
}

Quaternion flip_xy(Quaternion p_rot) {
	return Basis(Quaternion(-p_rot.x, -p_rot.y, p_rot.z, p_rot.w).normalized()).get_rotation_quaternion();
}

Quaternion flip_all(Quaternion p_rot) {
	return Basis(p_rot.inverse()).get_rotation_quaternion();
}

Vector3 inv_x(Vector3 p_scl) {
	return Vector3(1.0 / p_scl.x, p_scl.y, p_scl.z);
}

Vector3 inv_xy(Vector3 p_scl) {
	return Vector3(1.0 / p_scl.x, 1.0 / p_scl.y, p_scl.z);
}

Vector3 inv_all(Vector3 p_scl) {
	return Vector3(1.0, 1.0, 1.0) / p_scl;
}

TEST_CASE("[SceneTree][CopyTransformModifier3D]") {
	int seed = 12345;
	Skeleton3D *skeleton = memnew(Skeleton3D);
	BoneConstraint3D *mod = memnew(BoneConstraint3D(BoneConstraint3D::CONSTRAINT_COPY_TRANSFORM));

	// Instead of awaiting the process to wait to finish deferred process and watch "skeleton_updated" signal,
	// force notify NOTIFICATION_UPDATE_SKELETON and get the modified pose from the BoneAttachment's transform.
	BoneAttachment3D *modified = memnew(BoneAttachment3D);

	SceneTree::get_root()->add_child(skeleton);

	int root = skeleton->add_bone("root");
	skeleton->set_bone_rest(root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(root, make_random_transform_3d(++seed));

	int apl_root = skeleton->add_bone("apl_root");
	skeleton->set_bone_parent(apl_root, root);
	skeleton->set_bone_rest(apl_root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(apl_root, make_random_transform_3d(++seed));

	int apl_bone = skeleton->add_bone("apl_bone");
	skeleton->set_bone_parent(apl_bone, apl_root);
	skeleton->set_bone_rest(apl_bone, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(apl_bone, make_random_transform_3d(++seed));

	int tgt_root = skeleton->add_bone("tgt_root");
	skeleton->set_bone_parent(tgt_root, root);
	skeleton->set_bone_rest(tgt_root, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(tgt_root, make_random_transform_3d(++seed));

	int tgt_bone = skeleton->add_bone("tgt_bone");
	skeleton->set_bone_parent(tgt_bone, tgt_root);
	skeleton->set_bone_rest(tgt_bone, make_random_transform_3d(++seed));
	skeleton->set_bone_pose(tgt_bone, make_random_transform_3d(++seed));

	skeleton->add_child(mod);
	skeleton->add_child(modified);
	modified->set_rotation_edit_mode(Node3D::ROTATION_EDIT_MODE_QUATERNION);
	modified->set_bone_idx(apl_bone);

	mod->set_setting_count(1);
	mod->set_reference_bone(0, tgt_bone);
	mod->set_apply_bone(0, apl_bone);

	mod->set_copy_position(0, true);
	mod->set_copy_rotation(0, true);
	mod->set_copy_scale(0, true);

	// ===== [CopyTransformModifier3D] Enable 1 axis =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, false);
	mod->set_axis_z_enabled(0, false);
	mod->set_axis_x_inverted(0, false);
	mod->set_axis_y_inverted(0, false);
	mod->set_axis_z_inverted(0, false);

	SUBCASE("[CopyTransformModifier3D] Enable 1 axis, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).x,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin.x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
							  BoneConstraint3D::get_roll_angle((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X))),
				"Rotation x (roll x) is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).x,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale().x),
				"Scale x is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 1 axis, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X))),
				"Rotation x (roll x) is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).x),
				"Scale x is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 1 axis, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X))),
				"Rotation x (roll x) is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()).x),
				"Scale x is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 1 axis, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X)),
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_X))),
				"Rotation x (roll x) is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).x),
				"Scale x is copied correctly.");
	}

	// ===== [CopyTransformModifier3D] Enable 2 axes =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, true);
	mod->set_axis_z_enabled(0, false);
	mod->set_axis_x_inverted(0, false);
	mod->set_axis_y_inverted(0, false);
	mod->set_axis_z_inverted(0, false);

	SUBCASE("[CopyTransformModifier3D] Enable 2 axes, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).x,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin.x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).y,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin.y),
				"Position y is copied correctly.");
		CHECK_MESSAGE(Math::is_zero_approx(
							  BoneConstraint3D::get_roll_angle((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Z))),
				"Rotation z (roll z) is zero correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).x,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale().x),
				"Scale x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).y,
							  (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale().y),
				"Scale y is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 2 axes, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_position(tgt_bone).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).y),
				"Position y is copied correctly.");
		CHECK_MESSAGE(Math::is_zero_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Z))),
				"Rotation z (roll z) is zero correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).x),
				"Scale x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  skeleton->get_bone_pose_scale(tgt_bone).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).y),
				"Scale y is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 2 axes, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin).y),
				"Position y is copied correctly.");
		CHECK_MESSAGE(Math::is_zero_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Z))),
				"Rotation z (roll z) is zero correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()).x),
				"Scale x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()).y),
				"Scale y is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable 2 axes, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).x),
				"Position x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)).y),
				"Position y is copied correctly.");
		CHECK_MESSAGE(Math::is_zero_approx(
							  BoneConstraint3D::get_roll_angle(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion(), BoneConstraint3D::get_vector_from_axis(Vector3::AXIS_Z))),
				"Rotation z (roll z) is zero correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).x,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).x),
				"Scale x is copied correctly.");
		CHECK_MESSAGE(Math::is_equal_approx(
							  (skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).y,
							  ((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)).y),
				"Scale y is copied correctly.");
	}

	// ===== [CopyTransformModifier3D] Enable all axes =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, true);
	mod->set_axis_z_enabled(0, true);
	mod->set_axis_x_inverted(0, false);
	mod->set_axis_y_inverted(0, false);
	mod->set_axis_z_inverted(0, false);

	SUBCASE("[CopyTransformModifier3D] Enable all axes, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(skeleton->get_bone_pose_position(tgt_bone).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin), "Position is copied correctly.");
		CHECK_MESSAGE(Basis(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion())), "Rotation is copied correctly.");
		CHECK_MESSAGE(skeleton->get_bone_pose_scale(tgt_bone).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale()), "Scale is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(skeleton->get_bone_pose_position(tgt_bone).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied correctly.");
		CHECK_MESSAGE(Basis(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()), "Rotation is copied correctly.");
		CHECK_MESSAGE(skeleton->get_bone_pose_scale(tgt_bone).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE((skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin), "Position is copied correctly.");
		CHECK_MESSAGE(Basis(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion())), "Rotation is copied correctly.");
		CHECK_MESSAGE((skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()), "Scale is copied correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE((skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied correctly.");
		CHECK_MESSAGE(Basis(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()), "Rotation is copied correctly.");
		CHECK_MESSAGE((skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied correctly.");
	}

	// ===== [CopyTransformModifier3D] Enable all axes, invert 1 axis =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, true);
	mod->set_axis_z_enabled(0, true);
	mod->set_axis_x_inverted(0, true);
	mod->set_axis_y_inverted(0, false);
	mod->set_axis_z_inverted(0, false);

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 1 axis, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_x(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 1 axis, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_x(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 1 axis, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_x(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_x(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 1 axis, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_x(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_x(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_x(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	// ===== [CopyTransformModifier3D] Enable all axes, invert 2 axes =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, true);
	mod->set_axis_z_enabled(0, true);
	mod->set_axis_x_inverted(0, true);
	mod->set_axis_y_inverted(0, true);
	mod->set_axis_z_inverted(0, false);

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 2 axes, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_xy(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 2 axes, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_xy(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 2 axes, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_xy(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert 2 axes, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_xy(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_xy(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	// ===== [CopyTransformModifier3D] Enable all axes, invert all axes =====
	mod->set_axis_x_enabled(0, true);
	mod->set_axis_y_enabled(0, true);
	mod->set_axis_z_enabled(0, true);
	mod->set_axis_x_inverted(0, true);
	mod->set_axis_y_inverted(0, true);
	mod->set_axis_z_inverted(0, true);

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert all axes, additive=false, relative=false") {
		mod->set_additive(0, false);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_all(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert all axes, additive=true, relative=false") {
		mod->set_additive(0, true);
		mod->set_relative(0, false);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_position(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_all(skeleton->get_bone_pose_scale(tgt_bone)).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert all axes, additive=false, relative=true") {
		mod->set_additive(0, false);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_rest(apl_bone).origin), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_all(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_rest(apl_bone).basis.get_rotation_quaternion().inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_all(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_rest(apl_bone).basis.get_scale()), "Scale is copied/inverted correctly.");
	}

	SUBCASE("[CopyTransformModifier3D] Enable all axes, invert all axes, additive=true, relative=true") {
		mod->set_additive(0, true);
		mod->set_relative(0, true);
		skeleton->notification(Skeleton3D::NOTIFICATION_UPDATE_SKELETON);
		CHECK_MESSAGE(flip_all(skeleton->get_bone_pose_position(tgt_bone) - skeleton->get_bone_rest(tgt_bone).origin).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).origin - skeleton->get_bone_pose_position(apl_bone)), "Position is copied/inverted correctly.");
		CHECK_MESSAGE(flip_all(skeleton->get_bone_rest(tgt_bone).basis.get_rotation_quaternion().inverse() * skeleton->get_bone_pose_rotation(tgt_bone)).is_equal_approx(Basis(skeleton->get_bone_pose_rotation(apl_bone).inverse() * (skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_rotation_quaternion()).get_rotation_quaternion()), "Rotation is copied/inverted correctly.");
		CHECK_MESSAGE(inv_all(skeleton->get_bone_pose_scale(tgt_bone) / skeleton->get_bone_rest(tgt_bone).basis.get_scale()).is_equal_approx((skeleton->get_bone_global_pose(apl_root).affine_inverse() * modified->get_transform()).basis.get_scale() / skeleton->get_bone_pose_scale(apl_bone)), "Scale is copied/inverted correctly.");
	}

	memdelete(modified);
	memdelete(mod);
	memdelete(skeleton);
}

} // namespace TestCopyTransformModifier3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_decal.cpp ===
/*  test_decal.cpp                                                        */
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

#include "scene/main/scene_tree.h"

namespace TestDecal {

TEST_CASE("[SceneTree][Decal] Getters/Setters") {
	Decal *decal = memnew(Decal);
	AABB default_aabb{
		Vector3(-1.0f, -1.0f, -1.0f),
		Vector3(2.0f, 2.0f, 2.0f)
	};
	SUBCASE("Default values") {
		CHECK_MESSAGE(decal->get_aabb() == default_aabb, "get_aabb() returns the expected values for the default.");
	}

	SUBCASE("Set and Get Size") {
		Vector3 size = Vector3(2.0f, 3.0f, 4.0f);

		decal->set_size(size);
		AABB expected_aabb{
			Vector3(-1.0f, -1.5f, -2.0f),
			Vector3(2.0f, 3.0f, 4.0f)
		};
		CHECK_MESSAGE(decal->get_aabb() == expected_aabb, "get_aabb() returns the expected values after setting.");
		//default back the size in order to use the default_aabb for future assertions
		decal->set_size(Vector3(2.0f, 2.0f, 2.0f));
	}

	SUBCASE("Set and Get Texture") {
		Ref<Texture2D> albedo_texture;
		albedo_texture.instantiate();
		decal->set_texture(Decal::TEXTURE_ALBEDO, albedo_texture);
		CHECK_MESSAGE(decal->get_texture(Decal::TEXTURE_ALBEDO) == albedo_texture, "get_texture() returns the expected texture after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting texture.");
	}

	SUBCASE("Set and Get Emission Energy") {
		decal->set_emission_energy(2.0f);
		CHECK_MESSAGE(decal->get_emission_energy() == 2.0, "get_emission_energy() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting emission energy.");
	}

	SUBCASE("Set and Get Albedo Mix") {
		decal->set_albedo_mix(0.5);
		CHECK_MESSAGE(decal->get_albedo_mix() == 0.5f, "get_albedo_mix() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting albedo mix.");
	}

	SUBCASE("Set and Get Modulate") {
		Color new_color(0.5f, 0.5f, 0.5f, 1.0f);
		decal->set_modulate(new_color);
		CHECK_MESSAGE(decal->get_modulate() == new_color, "get_modulate() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting modulate.");
	}

	SUBCASE("Set and Get Upper Fade") {
		decal->set_upper_fade(0.4f);
		CHECK_MESSAGE(decal->get_upper_fade() == 0.4f, "get_upper_fade() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting upper fade.");
	}

	SUBCASE("Set and Get Lower Fade") {
		decal->set_lower_fade(0.2f);
		CHECK_MESSAGE(decal->get_lower_fade() == 0.2f, "get_lower_fade() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting lower fade.");
	}

	SUBCASE("Set and Get Normal Fade") {
		decal->set_normal_fade(0.1f);
		CHECK_MESSAGE(decal->get_normal_fade() == 0.1f, "get_normal_fade() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting normal fade.");
	}

	SUBCASE("Enable and Check Distance Fade") {
		decal->set_enable_distance_fade(true);
		CHECK_MESSAGE(decal->is_distance_fade_enabled() == true, "is_distance_fade_enabled() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after enabling distance fade.");
	}

	SUBCASE("Set and Get Distance Fade Begin") {
		decal->set_distance_fade_begin(50.0f);
		CHECK_MESSAGE(decal->get_distance_fade_begin() == 50.0f, "get_distance_fade_begin() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting distance fade begin.");
	}

	SUBCASE("Set and Get Distance Fade Length") {
		decal->set_distance_fade_length(15.0f);
		CHECK_MESSAGE(decal->get_distance_fade_length() == 15.0f, "get_distance_fade_length() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting distance fade length.");
	}

	SUBCASE("Set and Get Cull Mask") {
		decal->set_cull_mask(0xFF);
		CHECK_MESSAGE(decal->get_cull_mask() == 0xFF, "get_cull_mask() returns the expected value after setting.");
		CHECK_MESSAGE(default_aabb == decal->get_aabb(), "get_aabb() remains unchanged after setting cull mask.");
	}

	memdelete(decal);
}

} // namespace TestDecal

#endif // _3D_DISABLED

// === Merged from tests/scene/test_generic_6dof_joint_3d.cpp ===
/*  test_generic_6dof_joint_3d.cpp                                        */
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



#ifndef PHYSICS_3D_DISABLED

#include "scene/3d/physics/joints/generic_6dof_joint_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

#include <cfloat>

namespace TestGeneric6DOFJoint3D {

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Quaternion angular target is normalized and stored") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	const Quaternion target_rotation(Vector3(0, 1, 0), 0.5);
	joint->set_angular_target_rotation(target_rotation * 2.0);

	CHECK(joint->get_angular_target_rotation().is_normalized());
	CHECK(joint->get_angular_target_rotation().is_equal_approx(target_rotation));
	CHECK(joint->has_target_rotation());

	memdelete(joint);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Euler equilibrium setter clears stored quaternion target") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	joint->set_angular_target_rotation(Quaternion(Vector3(0, 0, 1), 0.25));
	CHECK(joint->has_target_rotation());

	joint->set_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0.1);
	CHECK_FALSE(joint->has_target_rotation());

	memdelete(joint);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] clear_angular_target_rotation resets state") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	joint->set_angular_target_rotation(Quaternion(Vector3(1, 0, 0), 0.5));
	CHECK(joint->has_target_rotation());

	joint->clear_angular_target_rotation();
	CHECK_FALSE(joint->has_target_rotation());
	ERR_PRINT_OFF;
	CHECK(joint->get_angular_target_rotation().is_equal_approx(Quaternion()));
	ERR_PRINT_ON;

	memdelete(joint);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Getter returns identity before configuration when no explicit target is set") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	joint->set_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0.3);
	joint->set_param_y(Generic6DOFJoint3D::PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, -0.2);
	joint->set_param_z(Generic6DOFJoint3D::PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0.4);

	CHECK_FALSE(joint->has_target_rotation());
	ERR_PRINT_OFF;
	CHECK(joint->get_angular_target_rotation().is_equal_approx(Quaternion()));
	ERR_PRINT_ON;

	memdelete(joint);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Explicit quaternion target shadows Euler equilibrium derivation") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	joint->set_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_SPRING_EQUILIBRIUM_POINT, 0.3);
	const Quaternion explicit_target(Vector3(0, 1, 0), 0.7);
	joint->set_angular_target_rotation(explicit_target);

	CHECK(joint->has_target_rotation());
	CHECK(joint->get_angular_target_rotation().is_equal_approx(explicit_target));

	memdelete(joint);
}

static RID make_configured_generic_6dof_joint(RID &r_body_a, RID &r_body_b) {
	r_body_a = PhysicsServer3D::body_create();
	r_body_b = PhysicsServer3D::body_create();

	const RID joint = PhysicsServer3D::joint_create();
	PhysicsServer3D::joint_make_generic_6dof(joint, r_body_a, Transform3D(), r_body_b, Transform3D());
	return joint;
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Drive force and torque limits params round-trip") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	joint->set_param_x(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT, 123.0);
	joint->set_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_DRIVE_TORQUE_LIMIT, 45.0);

	CHECK(joint->get_param_x(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT) == 123.0);
	CHECK(joint->get_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) == 45.0);

	memdelete(joint);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Drive force and torque limits defaults are unlimited") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	CHECK(joint->get_param_x(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT) == FLT_MAX);
	CHECK(joint->get_param_y(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT) == FLT_MAX);
	CHECK(joint->get_param_z(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT) == FLT_MAX);

	CHECK(joint->get_param_x(Generic6DOFJoint3D::PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) == FLT_MAX);
	CHECK(joint->get_param_y(Generic6DOFJoint3D::PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) == FLT_MAX);
	CHECK(joint->get_param_z(Generic6DOFJoint3D::PARAM_ANGULAR_DRIVE_TORQUE_LIMIT) == FLT_MAX);

	memdelete(joint);
}

TEST_CASE("[SceneTree][PhysicsServer3D][Generic6DOFJoint3D] Drive force and torque limits params round-trip per axis") {
	REQUIRE(PhysicsServer3D::is_initialized());

	RID body_a;
	RID body_b;
	RID joint = make_configured_generic_6dof_joint(body_a, body_b);

	constexpr Vector3::Axis axes[] = { Vector3::AXIS_X, Vector3::AXIS_Y, Vector3::AXIS_Z };
	constexpr real_t linear_values[] = { 12.0, 34.0, 56.0 };
	constexpr real_t angular_values[] = { 7.0, 8.0, 9.0 };

	for (int i = 0; i < 3; i++) {
		PhysicsServer3D::generic_6dof_joint_set_param(joint, axes[i], PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT, linear_values[i]);
		PhysicsServer3D::generic_6dof_joint_set_param(joint, axes[i], PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT, angular_values[i]);
	}

	for (int i = 0; i < 3; i++) {
		CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, axes[i], PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT), doctest::Approx(linear_values[i]));
		CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, axes[i], PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT), doctest::Approx(angular_values[i]));
	}

	PhysicsServer3D::free_rid(joint);
	PhysicsServer3D::free_rid(body_a);
	PhysicsServer3D::free_rid(body_b);
}

TEST_CASE("[SceneTree][PhysicsServer3D][Generic6DOFJoint3D] Drive force and torque limits defaults are unlimited") {
	REQUIRE(PhysicsServer3D::is_initialized());

	RID body_a;
	RID body_b;
	RID joint = make_configured_generic_6dof_joint(body_a, body_b);

	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_X, PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT), FLT_MAX);
	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_Y, PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT), FLT_MAX);
	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_Z, PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT), FLT_MAX);

	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_X, PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT), FLT_MAX);
	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_Y, PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT), FLT_MAX);
	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_Z, PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT), FLT_MAX);

	PhysicsServer3D::free_rid(joint);
	PhysicsServer3D::free_rid(body_a);
	PhysicsServer3D::free_rid(body_b);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Legacy motor and DRIVE limits are stored independently") {
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);

	// Set legacy motor limit, then set DRIVE limit to a finite value.
	ERR_PRINT_OFF;
	joint->set_param_x(Generic6DOFJoint3D::PARAM_LINEAR_MOTOR_FORCE_LIMIT, 50.0);
	ERR_PRINT_ON;
	joint->set_param_x(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT, 80.0);

	CHECK_EQ(joint->get_param_x(Generic6DOFJoint3D::PARAM_LINEAR_MOTOR_FORCE_LIMIT), doctest::Approx(50.0));
	CHECK_EQ(joint->get_param_x(Generic6DOFJoint3D::PARAM_LINEAR_DRIVE_FORCE_LIMIT), doctest::Approx(80.0));

	memdelete(joint);
}

TEST_CASE("[SceneTree][PhysicsServer3D][Generic6DOFJoint3D] DRIVE FLT_MAX after legacy motor round-trips through storage") {
	REQUIRE(PhysicsServer3D::is_initialized());

	RID body_a;
	RID body_b;
	RID joint = make_configured_generic_6dof_joint(body_a, body_b);

	// Legacy motor limit set first, then explicit DRIVE FLT_MAX
	PhysicsServer3D::generic_6dof_joint_set_param(joint, Vector3::AXIS_X, PS3DE::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT, 50.0);
	PhysicsServer3D::generic_6dof_joint_set_param(joint, Vector3::AXIS_X, PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT, FLT_MAX);

	CHECK_EQ(PhysicsServer3D::generic_6dof_joint_get_param(joint, Vector3::AXIS_X, PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT), FLT_MAX);

	PhysicsServer3D::free_rid(joint);
	PhysicsServer3D::free_rid(body_a);
	PhysicsServer3D::free_rid(body_b);
}

TEST_CASE("[SceneTree][Generic6DOFJoint3D] Default joint configuration does not emit MOTOR_FORCE_LIMIT deprecation warning") {
	// Instancing a node should be silent
	Generic6DOFJoint3D *joint = memnew(Generic6DOFJoint3D);
	CHECK(joint != nullptr);
	memdelete(joint);
}

} // namespace TestGeneric6DOFJoint3D

#endif // PHYSICS_3D_DISABLED

// === Merged from tests/scene/test_gltf_document.cpp ===
/*  test_gltf_document.cpp                                                */
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

#include "modules/modules_enabled.gen.h" // For gltf.

#ifdef MODULE_GLTF_ENABLED

#include "tests/test_macros.h"

#include "modules/gltf/gltf_document.h"

namespace TestGLTFDocument {

struct GLTFArraySize {
	String key;
	int val;
};

struct GLTFKeyValue {
	String key;
	Variant val;
};

struct GLTFTestCase {
	String filename;
	String copyright;
	String generator;
	String version;
	Vector<GLTFArraySize> array_sizes;
	Vector<GLTFArraySize> json_array_sizes;
	Vector<GLTFKeyValue> keyvalues;
};

const GLTFTestCase glTF_test_cases[] = {
	{ "models/cube.gltf",
			"",
			"Khronos glTF Blender I/O v4.3.47",
			"2.0",
			// Here are the array sizes.
			{
					{ "nodes", 1 },
					{ "buffers", 1 },
					{ "buffer_views", 13 },
					{ "accessors", 13 },
					{ "meshes", 1 },
					{ "materials", 2 },
					{ "root_nodes", 1 },
					{ "textures", 0 },
					{ "texture_samplers", 0 },
					{ "images", 0 },
					{ "skins", 0 },
					{ "cameras", 0 },
					{ "lights", 0 },
					{ "skeletons", 0 },
					{ "animations", 1 },
			},
			// Here are the json array sizes.
			{
					{ "scenes", 1 },
					{ "nodes", 1 },
					{ "animations", 1 },
					{ "meshes", 1 },
					{ "accessors", 13 },
					{ "bufferViews", 13 },
					{ "buffers", 1 },
			},
			// Here are the key-value pairs.
			{
					{ "major_version", 2 },
					{ "minor_version", 0 },
					{ "scene_name", "cube" },
					{ "filename", "cube" } } },
	{ "models/suzanne.glb",
			"this is example text",
			"Khronos glTF Blender I/O v4.3.47",
			"2.0",
			// Here are the array sizes.
			{
					{ "glb_data", 68908 },
					{ "nodes", 2 },
					{ "buffers", 1 },
					{ "buffer_views", 5 },
					{ "accessors", 4 },
					{ "meshes", 1 },
					{ "materials", 1 },
					{ "root_nodes", 2 },
					{ "textures", 1 },
					{ "texture_samplers", 1 },
					{ "images", 1 },
					{ "skins", 0 },
					{ "cameras", 1 },
					{ "lights", 0 },
					{ "unique_names", 4 },
					{ "skeletons", 0 },
					{ "animations", 0 },
			},
			// Here are the json array sizes.
			{
					{ "scenes", 1 },
					{ "nodes", 2 },
					{ "cameras", 1 },
					{ "materials", 1 },
					{ "meshes", 1 },
					{ "textures", 1 },
					{ "images", 1 },
					{ "accessors", 4 },
					{ "bufferViews", 5 },
					{ "buffers", 1 },
			},
			// Here are the key-value pairs.
			{
					{ "major_version", 2 },
					{ "minor_version", 0 },
					{ "scene_name", "suzanne" },
					{ "filename", "suzanne" } } },
};

void register_gltf_extension() {
	GLTFDocument::unregister_all_gltf_document_extensions();

	// Ensures meshes become a MeshInstance3D and not an ImporterMeshInstance3D.
	Ref<GLTFDocumentExtensionConvertImporterMesh> extension_GLTFDocumentExtensionConvertImporterMesh;
	extension_GLTFDocumentExtensionConvertImporterMesh.instantiate();
	GLTFDocument::register_gltf_document_extension(extension_GLTFDocumentExtensionConvertImporterMesh);
}

void test_gltf_document_values(Ref<GLTFDocument> &p_gltf_document, Ref<GLTFState> &p_gltf_state, const GLTFTestCase &p_test_case) {
	const Error err = p_gltf_document->append_from_file(TestUtils::get_data_path(p_test_case.filename), p_gltf_state);
	REQUIRE(err == OK);

	for (GLTFArraySize array_size : p_test_case.array_sizes) {
		CHECK_MESSAGE(((Array)(p_gltf_state->getvar(array_size.key))).size() == array_size.val, "Expected \"", array_size.key, "\" to have ", array_size.val, " elements.");
	}

	for (GLTFArraySize array_size : p_test_case.json_array_sizes) {
		CHECK(p_gltf_state->get_json().has(array_size.key));
		CHECK_MESSAGE(((Array)(p_gltf_state->get_json()[array_size.key])).size() == array_size.val, "Expected \"", array_size.key, "\" to have ", array_size.val, " elements.");
	}

	for (GLTFKeyValue key_value : p_test_case.keyvalues) {
		CHECK_MESSAGE(p_gltf_state->getvar(key_value.key) == key_value.val, "Expected \"", key_value.key, "\" to be \"", key_value.val, "\".");
	}

	CHECK(p_gltf_state->get_copyright() == p_test_case.copyright);
	CHECK(((Dictionary)p_gltf_state->get_json()["asset"])["generator"] == p_test_case.generator);
	CHECK(((Dictionary)p_gltf_state->get_json()["asset"])["version"] == p_test_case.version);
}

void test_gltf_save(Node *p_node) {
	Ref<GLTFDocument> gltf_document_save;
	gltf_document_save.instantiate();
	Ref<GLTFState> gltf_state_save;
	gltf_state_save.instantiate();

	gltf_document_save->append_from_scene(p_node, gltf_state_save);

	// Check saving the scene to gltf and glb.
	const Error err_save_gltf = gltf_document_save->write_to_filesystem(gltf_state_save, TestUtils::get_temp_path("cube.gltf"));
	const Error err_save_glb = gltf_document_save->write_to_filesystem(gltf_state_save, TestUtils::get_temp_path("cube.glb"));
	CHECK(err_save_gltf == OK);
	CHECK(err_save_glb == OK);
}

TEST_CASE("[SceneTree][GLTFDocument] Load cube.gltf") {
	register_gltf_extension();

	Ref<GLTFDocument> gltf_document;
	gltf_document.instantiate();
	Ref<GLTFState> gltf_state;
	gltf_state.instantiate();

	test_gltf_document_values(gltf_document, gltf_state, glTF_test_cases[0]);

	Node *node = gltf_document->generate_scene(gltf_state);

	// Check the loaded scene.
	CHECK(node->is_class("Node3D"));
	CHECK(node->get_name() == "cube");

	CHECK(node->get_child(0)->is_class("MeshInstance3D"));
	CHECK(node->get_child(0)->get_name() == "Cube");

	CHECK(node->get_child(1)->is_class("AnimationPlayer"));
	CHECK(node->get_child(1)->get_name() == "AnimationPlayer");

	test_gltf_save(node);

	// Clean up the node.
	memdelete(node);
}

TEST_CASE("[SceneTree][GLTFDocument] Load suzanne.glb") {
	register_gltf_extension();

	Ref<GLTFDocument> gltf_document;
	gltf_document.instantiate();
	Ref<GLTFState> gltf_state;
	gltf_state.instantiate();

	test_gltf_document_values(gltf_document, gltf_state, glTF_test_cases[1]);

	Node *node = gltf_document->generate_scene(gltf_state);

	// Check the loaded scene.
	CHECK(node->is_class("Node3D"));
	CHECK(node->get_name() == "suzanne");

	CHECK(node->get_child(0)->is_class("MeshInstance3D"));
	CHECK(node->get_child(0)->get_name() == "Suzanne");

	CHECK(node->get_child(1)->is_class("Camera3D"));
	CHECK(node->get_child(1)->get_name() == "Camera");

	test_gltf_save(node);

	// Clean up the node.
	memdelete(node);
}

} // namespace TestGLTFDocument

#endif // MODULE_GLTF_ENABLED

#endif // _3D_DISABLED

// === Merged from tests/scene/test_height_map_shape_3d.cpp ===
/*  test_height_map_shape_3d.cpp                                          */
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



#ifndef PHYSICS_3D_DISABLED

#include "core/io/image.h"
#include "scene/resources/mesh.h"

namespace TestHeightMapShape3D {

TEST_CASE("[SceneTree][HeightMapShape3D] Constructor") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	CHECK(height_map_shape->get_map_width() == 2);
	CHECK(height_map_shape->get_map_depth() == 2);
	CHECK(height_map_shape->get_map_data().size() == 4);
	CHECK(height_map_shape->get_min_height() == 0.0);
	CHECK(height_map_shape->get_max_height() == 0.0);
}

TEST_CASE("[SceneTree][HeightMapShape3D] set_map_width and get_map_width") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	height_map_shape->set_map_width(10);
	CHECK(height_map_shape->get_map_width() == 10);
}

TEST_CASE("[SceneTree][HeightMapShape3D] set_map_depth and get_map_depth") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	height_map_shape->set_map_depth(15);
	CHECK(height_map_shape->get_map_depth() == 15);
}

TEST_CASE("[SceneTree][HeightMapShape3D] set_map_data and get_map_data") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	Vector<real_t> map_data;
	map_data.push_back(1.0);
	map_data.push_back(2.0);
	height_map_shape->set_map_data(map_data);
	CHECK(height_map_shape->get_map_data().size() == 4.0);
	CHECK(height_map_shape->get_map_data()[0] == 0.0);
	CHECK(height_map_shape->get_map_data()[1] == 0.0);
}

TEST_CASE("[SceneTree][HeightMapShape3D] get_min_height") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	height_map_shape->set_map_width(3);
	height_map_shape->set_map_depth(1);
	height_map_shape->set_map_data(Vector<real_t>{ 1.0, 2.0, 0.5 });
	CHECK(height_map_shape->get_min_height() == 0.5);
}

TEST_CASE("[SceneTree][HeightMapShape3D] get_max_height") {
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));
	height_map_shape->set_map_width(3);
	height_map_shape->set_map_depth(1);
	height_map_shape->set_map_data(Vector<real_t>{ 1.0, 2.0, 0.5 });
	CHECK(height_map_shape->get_max_height() == 2.0);
}

TEST_CASE("[SceneTree][HeightMapShape3D] update_map_data_from_image") {
	// Create a HeightMapShape3D instance.
	Ref<Shape3D> height_map_shape = memnew(Shape3D(Shape3D::SHAPE_HEIGHTMAP));

	// Create a mock image with FORMAT_R8 and set its data.
	Vector<uint8_t> image_data;
	image_data.push_back(0);
	image_data.push_back(128);
	image_data.push_back(255);
	image_data.push_back(64);

	Ref<Image> image = memnew(Image);
	image->set_data(2, 2, false, Image::FORMAT_R8, image_data);

	height_map_shape->update_map_data_from_image(image, 0.0, 10.0);

	// Check the map data.
	Vector<real_t> expected_map_data = { 0.0, 5.0, 10.0, 2.5 };
	Vector<real_t> actual_map_data = height_map_shape->get_map_data();
	real_t tolerance = 0.1;

	for (int i = 0; i < expected_map_data.size(); ++i) {
		CHECK(Math::abs(actual_map_data[i] - expected_map_data[i]) < tolerance);
	}

	// Check the min and max heights.
	CHECK(height_map_shape->get_min_height() == 0.0);
	CHECK(height_map_shape->get_max_height() == 10.0);
}

} // namespace TestHeightMapShape3D

#endif // PHYSICS_3D_DISABLED

// === Merged from tests/scene/test_instance_placeholder.cpp ===
/*  test_instance_placeholder.cpp                                         */
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



#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/main/node.h"
#include "tests/test_macros.h"

namespace TestInstancePlaceholder {

class _TestInstancePlaceholderNode : public Node {

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_int_property", "int_property"), &_TestInstancePlaceholderNode::set_int_property);
		ClassDB::bind_method(D_METHOD("get_int_property"), &_TestInstancePlaceholderNode::get_int_property);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "int_property"), "set_int_property", "get_int_property");

		ClassDB::bind_method(D_METHOD("set_reference_property", "reference_property"), &_TestInstancePlaceholderNode::set_reference_property);
		ClassDB::bind_method(D_METHOD("get_reference_property"), &_TestInstancePlaceholderNode::get_reference_property);

		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "reference_property", PROPERTY_HINT_NODE_TYPE), "set_reference_property", "get_reference_property");

		ClassDB::bind_method(D_METHOD("set_reference_array_property", "reference_array_property"), &_TestInstancePlaceholderNode::set_reference_array_property);
		ClassDB::bind_method(D_METHOD("get_reference_array_property"), &_TestInstancePlaceholderNode::get_reference_array_property);

		// The hint string value "24/34:Node" is determined from existing PackedScenes with typed Array properties.
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "reference_array_property", PROPERTY_HINT_TYPE_STRING, "24/34:Node"), "set_reference_array_property", "get_reference_array_property");
	}

public:
	int int_property = 0;

	void set_int_property(int p_int) {
		int_property = p_int;
	}

	int get_int_property() const {
		return int_property;
	}

	Variant reference_property;

	void set_reference_property(const Variant &p_node) {
		reference_property = p_node;
	}

	Variant get_reference_property() const {
		return reference_property;
	}

	Array reference_array_property;

	void set_reference_array_property(const Array &p_array) {
		reference_array_property = p_array;
	}

	Array get_reference_array_property() const {
		return reference_array_property;
	}

	_TestInstancePlaceholderNode() {
		reference_array_property.set_typed(Variant::OBJECT, "Node", Variant());
	}
};

TEST_CASE("[SceneTree][InstancePlaceholder] Instantiate from placeholder with no overrides") {
	GDREGISTER_CLASS(_TestInstancePlaceholderNode);

	SUBCASE("with non-node values") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		ip->set_name("TestScene");
		Node *root = memnew(Node);
		SceneTree::get_root()->add_child(root);

		root->add_child(ip);
		// Create a scene to instance.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		scene->set_int_property(12);

		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		const Error err = packed_scene->pack(scene);
		REQUIRE(err == OK);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_name() == "TestScene");
		CHECK(created->get_int_property() == 12);

		root->queue_free();
		memdelete(scene);
	}

	SUBCASE("with node value") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		ip->set_name("TestScene");
		Node *root = memnew(Node);
		SceneTree::get_root()->add_child(root);

		root->add_child(ip);
		// Create a scene to instance.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		Node *referenced = memnew(Node);
		scene->add_child(referenced);
		referenced->set_owner(scene);
		scene->set_reference_property(referenced);
		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		const Error err = packed_scene->pack(scene);
		REQUIRE(err == OK);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_name() == "TestScene");
		CHECK(created->get_child_count() == 1);
		CHECK(created->get_reference_property().identity_compare(created->get_child(0, false)));
		CHECK_FALSE(created->get_reference_property().identity_compare(referenced));

		root->queue_free();
		memdelete(scene);
	}

	SUBCASE("with node-array value") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		ip->set_name("TestScene");
		Node *root = memnew(Node);
		SceneTree::get_root()->add_child(root);

		root->add_child(ip);
		// Create a scene to instance.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		Node *referenced1 = memnew(Node);
		Node *referenced2 = memnew(Node);
		scene->add_child(referenced1);
		scene->add_child(referenced2);
		referenced1->set_owner(scene);
		referenced2->set_owner(scene);
		Array node_array;
		node_array.set_typed(Variant::OBJECT, "Node", Variant());
		node_array.push_back(referenced1);
		node_array.push_back(referenced2);
		scene->set_reference_array_property(node_array);
		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		const Error err = packed_scene->pack(scene);
		REQUIRE(err == OK);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_name() == "TestScene");
		CHECK(created->get_child_count() == 2);
		Array created_array = created->get_reference_array_property();
		REQUIRE(created_array.size() == node_array.size());
		REQUIRE(created_array.size() == created->get_child_count());

		// Iterate over all nodes, since the ordering is not guaranteed.
		for (int i = 0; i < node_array.size(); i++) {
			bool node_found = false;
			for (int j = 0; j < created->get_child_count(); j++) {
				if (created_array[i].identity_compare(created->get_child(j, true))) {
					node_found = true;
				}
			}
			CHECK(node_found);
		}
		root->queue_free();
		memdelete(scene);
	}
}

TEST_CASE("[SceneTree][InstancePlaceholder] Instantiate from placeholder with overrides") {
	GDREGISTER_CLASS(_TestInstancePlaceholderNode);

	SUBCASE("with non-node values") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		Node *root = memnew(Node);
		SceneTree::get_root()->add_child(root);

		root->add_child(ip);
		ip->set_name("TestScene");
		ip->set("int_property", 45);
		// Create a scene to pack.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		scene->set_int_property(12);

		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		packed_scene->pack(scene);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_int_property() == 45);

		root->queue_free();
		memdelete(scene);
	}

	SUBCASE("with node values") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		ip->set_name("TestScene");
		Node *root = memnew(Node);
		Node *overriding = memnew(Node);
		SceneTree::get_root()->add_child(root);

		root->add_child(ip);
		root->add_child(overriding);
		ip->set("reference_property", overriding);
		// Create a scene to instance.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		Node *referenced = memnew(Node);
		scene->add_child(referenced);
		referenced->set_owner(scene);
		scene->set_reference_property(referenced);
		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		const Error err = packed_scene->pack(scene);
		REQUIRE(err == OK);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_name() == "TestScene");
		CHECK(created->get_child_count() == 1);
		CHECK(created->get_reference_property().identity_compare(overriding));
		CHECK_FALSE(created->get_reference_property().identity_compare(referenced));

		root->queue_free();
		memdelete(scene);
	}

	SUBCASE("with node-array value") {
		InstancePlaceholder *ip = memnew(InstancePlaceholder);
		ip->set_name("TestScene");
		Node *root = memnew(Node);
		SceneTree::get_root()->add_child(root);

		Node *override1 = memnew(Node);
		Node *override2 = memnew(Node);
		Node *override3 = memnew(Node);
		root->add_child(ip);
		root->add_child(override1);
		root->add_child(override2);
		root->add_child(override3);

		Array override_node_array;
		override_node_array.set_typed(Variant::OBJECT, "Node", Variant());
		override_node_array.push_back(override1);
		override_node_array.push_back(override2);
		override_node_array.push_back(override3);

		ip->set("reference_array_property", override_node_array);

		// Create a scene to instance.
		_TestInstancePlaceholderNode *scene = memnew(_TestInstancePlaceholderNode);
		Node *referenced1 = memnew(Node);
		Node *referenced2 = memnew(Node);

		scene->add_child(referenced1);
		scene->add_child(referenced2);

		referenced1->set_owner(scene);
		referenced2->set_owner(scene);
		Array referenced_array;
		referenced_array.set_typed(Variant::OBJECT, "Node", Variant());
		referenced_array.push_back(referenced1);
		referenced_array.push_back(referenced2);

		scene->set_reference_array_property(referenced_array);
		// Pack the scene.
		Ref<PackedScene> packed_scene = memnew(PackedScene);
		const Error err = packed_scene->pack(scene);
		REQUIRE(err == OK);

		// Instantiate the scene.
		_TestInstancePlaceholderNode *created = Object::cast_to<_TestInstancePlaceholderNode>(ip->create_instance(true, packed_scene));
		REQUIRE(created != nullptr);
		CHECK(created->get_name() == "TestScene");
		CHECK(created->get_child_count() == 2);
		Array created_array = created->get_reference_array_property();
		REQUIRE_FALSE(created_array.size() == referenced_array.size());
		REQUIRE(created_array.size() == override_node_array.size());
		REQUIRE_FALSE(created_array.size() == created->get_child_count());

		// Iterate over all nodes, since the ordering is not guaranteed.
		for (int i = 0; i < override_node_array.size(); i++) {
			bool node_found = false;
			for (int j = 0; j < created_array.size(); j++) {
				if (override_node_array[i].identity_compare(created_array[j])) {
					node_found = true;
				}
			}
			CHECK(node_found);
		}
		root->queue_free();
		memdelete(scene);
	}
}

#ifdef TOOLS_ENABLED
TEST_CASE("[SceneTree][InstancePlaceholder] Instance a PackedScene containing an InstancePlaceholder with no overrides") {
	GDREGISTER_CLASS(_TestInstancePlaceholderNode);

	// Create the internal scene.
	_TestInstancePlaceholderNode *internal = memnew(_TestInstancePlaceholderNode);
	internal->set_name("InternalNode");
	Node *referenced = memnew(Node);
	referenced->set_name("OriginalReference");
	internal->add_child(referenced);
	referenced->set_owner(internal);
	internal->set_reference_property(referenced);

	// Pack the internal scene.
	Ref<PackedScene> internal_scene = memnew(PackedScene);
	Error err = internal_scene->pack(internal);
	REQUIRE(err == OK);

	const String internal_path = TestUtils::get_temp_path("instance_placeholder_test_internal.tscn");
	err = ResourceSaver::save(internal_scene, internal_path);
	REQUIRE(err == OK);

	Ref<PackedScene> internal_scene_loaded = ResourceLoader::load(internal_path, "PackedScene", ResourceFormatLoader::CacheMode::CACHE_MODE_IGNORE, &err);
	REQUIRE(err == OK);

	// Create the main scene.
	Node *root = memnew(Node);
	root->set_name("MainNode");
	Node *overriding = memnew(Node);
	overriding->set_name("OverridingReference");

	_TestInstancePlaceholderNode *internal_created = Object::cast_to<_TestInstancePlaceholderNode>(internal_scene_loaded->instantiate(PackedScene::GEN_EDIT_STATE_MAIN_INHERITED));
	internal_created->set_scene_instance_load_placeholder(true);
	root->add_child(internal_created);
	internal_created->set_owner(root);

	root->add_child(overriding);
	overriding->set_owner(root);
	// Here we introduce an error, we override the property with an internal node to the instance placeholder.
	// The InstancePlaceholder is now forced to properly resolve the Node.
	internal_created->set("reference_property", NodePath("OriginalReference"));

	// Pack the main scene.
	Ref<PackedScene> main_scene = memnew(PackedScene);
	err = main_scene->pack(root);
	REQUIRE(err == OK);

	const String main_path = TestUtils::get_temp_path("instance_placeholder_test_main.tscn");
	err = ResourceSaver::save(main_scene, main_path);
	REQUIRE(err == OK);

	// // Instantiate the scene.
	Ref<PackedScene> main_scene_loaded = ResourceLoader::load(main_path, "PackedScene", ResourceFormatLoader::CacheMode::CACHE_MODE_IGNORE, &err);
	REQUIRE(err == OK);

	Node *instanced_main_node = main_scene_loaded->instantiate();
	REQUIRE(instanced_main_node != nullptr);
	SceneTree::get_root()->add_child(instanced_main_node);
	CHECK(instanced_main_node->get_name() == "MainNode");
	REQUIRE(instanced_main_node->get_child_count() == 2);
	InstancePlaceholder *instanced_placeholder = Object::cast_to<InstancePlaceholder>(instanced_main_node->get_child(0, true));
	REQUIRE(instanced_placeholder != nullptr);

	_TestInstancePlaceholderNode *final_node = Object::cast_to<_TestInstancePlaceholderNode>(instanced_placeholder->create_instance(true));
	REQUIRE(final_node != nullptr);
	REQUIRE(final_node->get_child_count() == 1);
	REQUIRE(final_node->get_reference_property().identity_compare(final_node->get_child(0, true)));

	instanced_main_node->queue_free();
	memdelete(overriding);
	memdelete(root);
	memdelete(internal);
	DirAccess::remove_file_or_error(internal_path);
	DirAccess::remove_file_or_error(main_path);
}

TEST_CASE("[SceneTree][InstancePlaceholder] Instance a PackedScene containing an InstancePlaceholder with overrides") {
	GDREGISTER_CLASS(_TestInstancePlaceholderNode);

	// Create the internal scene.
	_TestInstancePlaceholderNode *internal = memnew(_TestInstancePlaceholderNode);
	internal->set_name("InternalNode");
	Node *referenced = memnew(Node);
	referenced->set_name("OriginalReference");
	internal->add_child(referenced);
	referenced->set_owner(internal);
	internal->set_reference_property(referenced);

	Node *array_ref1 = memnew(Node);
	array_ref1->set_name("ArrayRef1");
	internal->add_child(array_ref1);
	array_ref1->set_owner(internal);
	Node *array_ref2 = memnew(Node);
	array_ref2->set_name("ArrayRef2");
	internal->add_child(array_ref2);
	array_ref2->set_owner(internal);
	Array referenced_array;
	referenced_array.set_typed(Variant::OBJECT, "Node", Variant());
	referenced_array.push_back(array_ref1);
	referenced_array.push_back(array_ref2);
	internal->set_reference_array_property(referenced_array);

	// Pack the internal scene.
	Ref<PackedScene> internal_scene = memnew(PackedScene);
	Error err = internal_scene->pack(internal);
	REQUIRE(err == OK);

	const String internal_path = TestUtils::get_temp_path("instance_placeholder_test_internal_override.tscn");
	err = ResourceSaver::save(internal_scene, internal_path);
	REQUIRE(err == OK);

	Ref<PackedScene> internal_scene_loaded = ResourceLoader::load(internal_path, "PackedScene", ResourceFormatLoader::CacheMode::CACHE_MODE_IGNORE, &err);
	REQUIRE(err == OK);

	// Create the main scene.
	Node *root = memnew(Node);
	root->set_name("MainNode");
	Node *overriding = memnew(Node);
	overriding->set_name("OverridingReference");
	Node *array_ext = memnew(Node);
	array_ext->set_name("ExternalArrayMember");

	_TestInstancePlaceholderNode *internal_created = Object::cast_to<_TestInstancePlaceholderNode>(internal_scene_loaded->instantiate(PackedScene::GEN_EDIT_STATE_MAIN_INHERITED));
	internal_created->set_scene_instance_load_placeholder(true);
	root->add_child(internal_created);
	internal_created->set_owner(root);

	root->add_child(overriding);
	overriding->set_owner(root);
	root->add_child(array_ext);
	array_ext->set_owner(root);
	// Here we introduce an error, we override the property with an internal node to the instance placeholder.
	// The InstancePlaceholder is now forced to properly resolve the Node.
	internal_created->set_reference_property(overriding);
	Array internal_array = internal_created->get_reference_array_property();
	Array override_array;
	override_array.set_typed(Variant::OBJECT, "Node", Variant());
	for (int i = 0; i < internal_array.size(); i++) {
		override_array.push_back(internal_array[i]);
	}
	override_array.push_back(array_ext);
	internal_created->set_reference_array_property(override_array);

	// Pack the main scene.
	Ref<PackedScene> main_scene = memnew(PackedScene);
	err = main_scene->pack(root);
	REQUIRE(err == OK);

	const String main_path = TestUtils::get_temp_path("instance_placeholder_test_main_override.tscn");
	err = ResourceSaver::save(main_scene, main_path);
	REQUIRE(err == OK);

	// // Instantiate the scene.
	Ref<PackedScene> main_scene_loaded = ResourceLoader::load(main_path, "PackedScene", ResourceFormatLoader::CacheMode::CACHE_MODE_IGNORE, &err);
	REQUIRE(err == OK);

	Node *instanced_main_node = main_scene_loaded->instantiate();
	REQUIRE(instanced_main_node != nullptr);
	SceneTree::get_root()->add_child(instanced_main_node);
	CHECK(instanced_main_node->get_name() == "MainNode");
	REQUIRE(instanced_main_node->get_child_count() == 3);
	InstancePlaceholder *instanced_placeholder = Object::cast_to<InstancePlaceholder>(instanced_main_node->get_child(0, true));
	REQUIRE(instanced_placeholder != nullptr);

	_TestInstancePlaceholderNode *final_node = Object::cast_to<_TestInstancePlaceholderNode>(instanced_placeholder->create_instance(true));
	REQUIRE(final_node != nullptr);
	REQUIRE(final_node->get_child_count() == 3);
	REQUIRE(final_node->get_reference_property().identity_compare(instanced_main_node->get_child(1, true)));
	Array final_array = final_node->get_reference_array_property();
	REQUIRE(final_array.size() == 3);
	Array wanted_node_array = {
		instanced_main_node->get_child(2, true), // ExternalArrayMember
		final_node->get_child(1, true), // ArrayRef1
		final_node->get_child(2, true) // ArrayRef2
	};

	// Iterate over all nodes, since the ordering is not guaranteed.
	for (int i = 0; i < wanted_node_array.size(); i++) {
		bool node_found = false;
		for (int j = 0; j < final_array.size(); j++) {
			if (wanted_node_array[i].identity_compare(final_array[j])) {
				node_found = true;
			}
		}
		CHECK(node_found);
	}

	instanced_main_node->queue_free();
	memdelete(array_ext);
	memdelete(overriding);
	memdelete(root);
	memdelete(internal);
	DirAccess::remove_file_or_error(internal_path);
	DirAccess::remove_file_or_error(main_path);
}

#endif // TOOLS_ENABLED

} // namespace TestInstancePlaceholder

// === Merged from tests/scene/test_navigation_agent_2d.cpp ===
/*  test_navigation_agent_2d.cpp                                          */
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



#include "modules/modules_enabled.gen.h" // For navigation 2D.

#ifdef MODULE_NAVIGATION_2D_ENABLED

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestNavigationAgent2D {

TEST_SUITE("[Navigation2D]") {
	TEST_CASE("[SceneTree][NavigationAgent2D] New agent should have valid RID") {
		NavigationAgent2D *agent_node = memnew(NavigationAgent2D);
		CHECK(agent_node->get_rid().is_valid());
		memdelete(agent_node);
	}

	TEST_CASE("[SceneTree][NavigationAgent2D] New agent should attach to default map") {
		Node2D *node_2d = memnew(Node2D);
		SceneTree::get_root()->add_child(node_2d);

		NavigationAgent2D *agent_node = memnew(NavigationAgent2D);

		// agent should not be attached to any map when outside of tree
		CHECK_FALSE(agent_node->get_navigation_map().is_valid());

		SUBCASE("Agent should attach to default map when it enters the tree") {
			node_2d->add_child(agent_node);
			CHECK(agent_node->get_navigation_map().is_valid());
			CHECK(agent_node->get_navigation_map() == node_2d->get_world_2d()->get_navigation_map());
		}

		memdelete(agent_node);
		memdelete(node_2d);
	}
}

} // namespace TestNavigationAgent2D

#endif // MODULE_NAVIGATION_2D_ENABLED

// === Merged from tests/scene/test_navigation_agent_3d.cpp ===
/*  test_navigation_agent_3d.cpp                                          */
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



#include "modules/modules_enabled.gen.h" // For navigation 3D.

#ifdef MODULE_NAVIGATION_3D_ENABLED

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestNavigationAgent3D {

TEST_SUITE("[Navigation3D]") {
	TEST_CASE("[SceneTree][NavigationAgent3D] New agent should have valid RID") {
		NavigationAgent3D *agent_node = memnew(NavigationAgent3D);
		CHECK(agent_node->get_rid().is_valid());
		memdelete(agent_node);
	}

	TEST_CASE("[SceneTree][NavigationAgent3D] New agent should attach to default map") {
		Node3D *node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);

		NavigationAgent3D *agent_node = memnew(NavigationAgent3D);

		// agent should not be attached to any map when outside of tree
		CHECK_FALSE(agent_node->get_navigation_map().is_valid());

		SUBCASE("Agent should attach to default map when it enters the tree") {
			node_3d->add_child(agent_node);
			CHECK(agent_node->get_navigation_map().is_valid());
			CHECK(agent_node->get_navigation_map() == node_3d->get_world_3d()->get_navigation_map());
		}

		memdelete(agent_node);
		memdelete(node_3d);
	}
}

} // namespace TestNavigationAgent3D

#endif // MODULE_NAVIGATION_3D_ENABLED

// === Merged from tests/scene/test_navigation_obstacle_2d.cpp ===
/*  test_navigation_obstacle_2d.cpp                                       */
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



#include "modules/modules_enabled.gen.h" // For navigation 2D.

#ifdef MODULE_NAVIGATION_2D_ENABLED

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestNavigationObstacle2D {

TEST_SUITE("[Navigation2D]") {
	TEST_CASE("[SceneTree][NavigationObstacle2D] New obstacle should have valid RID") {
		NavigationObstacle2D *obstacle_node = memnew(NavigationObstacle2D);
		CHECK(obstacle_node->get_rid().is_valid());
		memdelete(obstacle_node);
	}

	TEST_CASE("[SceneTree][NavigationObstacle2D] New obstacle should attach to default map") {
		Node2D *node_2d = memnew(Node2D);
		SceneTree::get_root()->add_child(node_2d);

		NavigationObstacle2D *obstacle_node = memnew(NavigationObstacle2D);
		// obstacle should not be attached to any map when outside of tree
		CHECK_FALSE(obstacle_node->get_navigation_map().is_valid());

		SUBCASE("Obstacle should attach to default map when it enters the tree") {
			node_2d->add_child(obstacle_node);
			CHECK(obstacle_node->get_navigation_map().is_valid());
			CHECK(obstacle_node->get_navigation_map() == node_2d->get_world_2d()->get_navigation_map());
		}

		memdelete(obstacle_node);
		memdelete(node_2d);
	}
}

} // namespace TestNavigationObstacle2D

#endif // MODULE_NAVIGATION_2D_ENABLED

// === Merged from tests/scene/test_navigation_obstacle_3d.cpp ===
/*  test_navigation_obstacle_3d.cpp                                       */
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



#include "modules/modules_enabled.gen.h" // For navigation 3D.

#ifdef MODULE_NAVIGATION_3D_ENABLED

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestNavigationObstacle3D {

TEST_SUITE("[Navigation3D]") {
	TEST_CASE("[SceneTree][NavigationObstacle3D] New obstacle should have valid RID") {
		NavigationObstacle3D *obstacle_node = memnew(NavigationObstacle3D);
		CHECK(obstacle_node->get_rid().is_valid());
		memdelete(obstacle_node);
	}

	TEST_CASE("[SceneTree][NavigationObstacle3D] New obstacle should attach to default map") {
		Node3D *node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);

		NavigationObstacle3D *obstacle_node = memnew(NavigationObstacle3D);
		// obstacle should not be attached to any map when outside of tree
		CHECK_FALSE(obstacle_node->get_navigation_map().is_valid());

		SUBCASE("Obstacle should attach to default map when it enters the tree") {
			node_3d->add_child(obstacle_node);
			CHECK(obstacle_node->get_navigation_map().is_valid());
			CHECK(obstacle_node->get_navigation_map() == node_3d->get_world_3d()->get_navigation_map());
		}

		memdelete(obstacle_node);
		memdelete(node_3d);
	}
}

} // namespace TestNavigationObstacle3D

#endif // MODULE_NAVIGATION_3D_ENABLED

// === Merged from tests/scene/test_navigation_region_2d.cpp ===
/*  test_navigation_region_2d.cpp                                         */
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



#include "modules/modules_enabled.gen.h" // For navigation 2D.

#ifdef MODULE_NAVIGATION_2D_ENABLED

#include "scene/main/scene_tree.h"

namespace TestNavigationRegion2D {

TEST_SUITE("[Navigation2D]") {
	TEST_CASE("[SceneTree][NavigationRegion2D] New region should have valid RID") {
		NavigationRegion2D *region_node = memnew(NavigationRegion2D);
		CHECK(region_node->get_rid().is_valid());
		memdelete(region_node);
	}
}

} // namespace TestNavigationRegion2D

#endif // MODULE_NAVIGATION_2D_ENABLED

// === Merged from tests/scene/test_navigation_region_3d.cpp ===
/*  test_navigation_region_3d.cpp                                         */
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



#include "modules/modules_enabled.gen.h" // For navigation 3D.

#ifdef MODULE_NAVIGATION_3D_ENABLED

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"

namespace TestNavigationRegion3D {

TEST_SUITE("[Navigation3D]") {
	TEST_CASE("[SceneTree][NavigationRegion3D] New region should have valid RID") {
		NavigationRegion3D *region_node = memnew(NavigationRegion3D);
		CHECK(region_node->get_rid().is_valid());
		memdelete(region_node);
	}

	TEST_CASE("[SceneTree][NavigationRegion3D] Region should bake successfully from valid geometry") {
		Node3D *node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);
		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		NavigationRegion3D *navigation_region = memnew(NavigationRegion3D);
		navigation_region->set_navigation_mesh(navigation_mesh);
		node_3d->add_child(navigation_region);
		Ref<PrimitiveMesh> plane_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
		plane_mesh->set_plane_size(Size2(10.0, 10.0));
		MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(plane_mesh);
		navigation_region->add_child(mesh_instance);

		CHECK_FALSE(navigation_region->is_baking());
		CHECK_EQ(navigation_mesh->get_polygon_count(), 0);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 0);

		SUBCASE("Synchronous bake should have immediate effects") {
			ERR_PRINT_OFF; // Suppress warning about baking from visual meshes as source geometry.
			navigation_region->bake_navigation_mesh(false);
			ERR_PRINT_ON;
			CHECK_FALSE(navigation_region->is_baking());
			CHECK_NE(navigation_mesh->get_polygon_count(), 0);
			CHECK_NE(navigation_mesh->get_vertices().size(), 0);
		}

		memdelete(mesh_instance);
		memdelete(navigation_region);
		memdelete(node_3d);
	}
}

} // namespace TestNavigationRegion3D

#endif // MODULE_NAVIGATION_3D_ENABLED

// === Merged from tests/scene/test_node.cpp ===
/*  test_node.cpp                                                         */
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



#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/main/node.h"
#include "tests/test_macros.h"

namespace TestNode {

class TestNode : public Node {

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_INTERNAL_PROCESS: {
				internal_process_counter++;
				push_self();
			} break;
			case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
				internal_physics_process_counter++;
				push_self();
			} break;
			case NOTIFICATION_PROCESS: {
				process_counter++;
				push_self();
			} break;
			case NOTIFICATION_PHYSICS_PROCESS: {
				physics_process_counter++;
				push_self();
			} break;
		}
	}

	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_exported_node", "node"), &TestNode::set_exported_node);
		ClassDB::bind_method(D_METHOD("get_exported_node"), &TestNode::get_exported_node);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "exported_node", PROPERTY_HINT_NODE_TYPE, "Node"), "set_exported_node", "get_exported_node");

		ClassDB::bind_method(D_METHOD("set_exported_nodes", "node"), &TestNode::set_exported_nodes);
		ClassDB::bind_method(D_METHOD("get_exported_nodes"), &TestNode::get_exported_nodes);
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "exported_nodes", PROPERTY_HINT_TYPE_STRING, "24/34:Node"), "set_exported_nodes", "get_exported_nodes");
	}

private:
	void push_self() {
		if (callback_list) {
			callback_list->push_back(this);
		}
	}

public:
	int internal_process_counter = 0;
	int internal_physics_process_counter = 0;
	int process_counter = 0;
	int physics_process_counter = 0;

	Node *exported_node = nullptr;
	Array exported_nodes;

	List<Node *> *callback_list = nullptr;

	void set_exported_node(Node *p_node) { exported_node = p_node; }
	Node *get_exported_node() const { return exported_node; }

	void set_exported_nodes(const Array &p_nodes) { exported_nodes = p_nodes; }
	Array get_exported_nodes() const { return exported_nodes; }

	TestNode() {
		Node *internal = memnew(Node);
		add_child(internal, false, INTERNAL_MODE_FRONT);
		internal = memnew(Node);
		add_child(internal, false, INTERNAL_MODE_BACK);
	}
};

TEST_CASE("[SceneTree][Node] Testing node operations with a very simple scene tree") {
	Node *node = memnew(Node);

	// Check initial scene tree setup.
	CHECK_EQ(SceneTree::get_root()->get_child_count(), 0);
	CHECK_EQ(SceneTree::get_node_count(), 1);

	// Check initial node setup.
	CHECK(node->get_name() == String());
	CHECK_FALSE(node->is_inside_tree());
	CHECK_EQ(node->get_parent(), nullptr);
	ERR_PRINT_OFF;
	CHECK(node->get_path().is_empty());
	ERR_PRINT_ON;
	CHECK_EQ(node->get_child_count(), 0);

	SceneTree::get_root()->add_child(node);

	CHECK_EQ(SceneTree::get_root()->get_child_count(), 1);
	CHECK_EQ(SceneTree::get_node_count(), 2);

	CHECK(node->get_name() != String());
	CHECK(node->is_inside_tree());
	CHECK_EQ(SceneTree::get_root(), node->get_parent());
	CHECK_FALSE(node->get_path().is_empty());
	CHECK_EQ(node->get_child_count(), 0);

	SUBCASE("Node should be accessible as first child") {
		Node *child = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child, node);
	}

	SUBCASE("Node should be accessible via the node path") {
		Node *child_by_path = SceneTree::get_root()->get_node_or_null(node->get_path());
		CHECK_EQ(child_by_path, node);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("Node"));
		CHECK_EQ(child_by_path, nullptr);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("/root/Node"));
		CHECK_EQ(child_by_path, nullptr);

		node->set_name("Node");

		child_by_path = SceneTree::get_root()->get_node_or_null(node->get_path());
		CHECK_EQ(child_by_path, node);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("Node"));
		CHECK_EQ(child_by_path, node);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("/root/Node"));
		CHECK_EQ(child_by_path, node);
	}

	SUBCASE("Node should be accessible via group") {
		Vector<Node *> nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK(nodes.is_empty());

		node->add_to_group("nodes");

		nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK_EQ(nodes.size(), 1);
		Node *E = nodes.get(0);
		CHECK_EQ(E, node);
	}

	SUBCASE("Node should be possible to find") {
		Node *child = SceneTree::get_root()->find_child("Node", true, false);
		CHECK_EQ(child, nullptr);

		node->set_name("Node");

		child = SceneTree::get_root()->find_child("Node", true, false);
		CHECK_EQ(child, node);
	}

	SUBCASE("Node should be possible to remove") {
		SceneTree::get_root()->remove_child(node);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 0);
		CHECK_EQ(SceneTree::get_node_count(), 1);

		CHECK_FALSE(node->is_inside_tree());
		CHECK_EQ(node->get_parent(), nullptr);
		ERR_PRINT_OFF;
		CHECK(node->get_path().is_empty());
		ERR_PRINT_ON;
	}

	SUBCASE("Node should be possible to move") {
		SceneTree::get_root()->move_child(node, 0);

		Node *child = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child, node);
		CHECK(node->is_inside_tree());
	}

	SUBCASE("Node should be possible to reparent") {
		node->reparent(SceneTree::get_root());

		Node *child = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child, node);
		CHECK(node->is_inside_tree());
	}

	SUBCASE("Node should be possible to duplicate") {
		node->set_name("MyName");

		Node *duplicate = node->duplicate();

		CHECK_FALSE(node == duplicate);
		CHECK_FALSE(duplicate->is_inside_tree());
		CHECK_EQ(duplicate->get_name(), node->get_name());

		memdelete(duplicate);
	}

	memdelete(node);
}

TEST_CASE("[SceneTree][Node] Testing node operations with a more complex simple scene tree") {
	Node *node1 = memnew(Node);
	Node *node2 = memnew(Node);
	Node *node1_1 = memnew(Node);

	SceneTree::get_root()->add_child(node1);
	SceneTree::get_root()->add_child(node2);

	node1->add_child(node1_1);

	CHECK(node1_1->is_inside_tree());
	CHECK_EQ(node1_1->get_parent(), node1);
	CHECK_EQ(node1->get_child_count(), 1);

	CHECK_EQ(SceneTree::get_root()->get_child_count(), 2);
	CHECK_EQ(SceneTree::get_node_count(), 4);

	SUBCASE("Nodes should be accessible via get_child(..)") {
		Node *child1 = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child1, node1);

		Node *child2 = SceneTree::get_root()->get_child(1);
		CHECK_EQ(child2, node2);

		Node *child1_1 = node1->get_child(0);
		CHECK_EQ(child1_1, node1_1);
	}

	SUBCASE("Removed nodes should also remove their children from the scene tree") {
		// Should also remove node1_1 from the scene tree.
		SceneTree::get_root()->remove_child(node1);

		CHECK_EQ(node1->get_child_count(), 1);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 1);
		CHECK_EQ(SceneTree::get_node_count(), 2);

		// First child should now be the second node.
		Node *child1 = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child1, node2);
	}

	SUBCASE("Removed children nodes should not affect their parent in the scene tree") {
		node1->remove_child(node1_1);

		CHECK_EQ(node1_1->get_parent(), nullptr);
		CHECK_EQ(node1->get_child_count(), 0);

		CHECK_EQ(SceneTree::get_node_count(), 3);
	}

	SUBCASE("Nodes should be in the expected order when a node is moved to the back") {
		SceneTree::get_root()->move_child(node1, 1);

		Node *child1 = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child1, node2);

		Node *child2 = SceneTree::get_root()->get_child(1);
		CHECK_EQ(child2, node1);
	}

	SUBCASE("Nodes should be in the expected order when a node is moved to the front") {
		SceneTree::get_root()->move_child(node2, 0);

		Node *child1 = SceneTree::get_root()->get_child(0);
		CHECK_EQ(child1, node2);

		Node *child2 = SceneTree::get_root()->get_child(1);
		CHECK_EQ(child2, node1);
	}

	SUBCASE("Nodes should be in the expected order when reparented (remove/add)") {
		CHECK_EQ(node2->get_child_count(), 0);

		node1->remove_child(node1_1);
		CHECK_EQ(node1->get_child_count(), 0);
		CHECK_EQ(node1_1->get_parent(), nullptr);

		node2->add_child(node1_1);
		CHECK_EQ(node2->get_child_count(), 1);
		CHECK_EQ(node1_1->get_parent(), node2);

		Node *child = node2->get_child(0);
		CHECK_EQ(child, node1_1);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 2);
		CHECK_EQ(SceneTree::get_node_count(), 4);
	}

	SUBCASE("Nodes should be in the expected order when reparented") {
		CHECK_EQ(node2->get_child_count(), 0);

		node1_1->reparent(node2);

		CHECK_EQ(node1->get_child_count(), 0);
		CHECK_EQ(node2->get_child_count(), 1);
		CHECK_EQ(node1_1->get_parent(), node2);

		Node *child = node2->get_child(0);
		CHECK_EQ(child, node1_1);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 2);
		CHECK_EQ(SceneTree::get_node_count(), 4);
	}

	SUBCASE("Nodes should be possible to find") {
		Node *child = SceneTree::get_root()->find_child("NestedNode", true, false);
		CHECK_EQ(child, nullptr);

		TypedArray<Node> children = SceneTree::get_root()->find_children("NestedNode", "", true, false);
		CHECK_EQ(children.size(), 0);

		node1->set_name("Node1");
		node2->set_name("Node2");
		node1_1->set_name("NestedNode");

		child = SceneTree::get_root()->find_child("NestedNode", true, false);
		CHECK_EQ(child, node1_1);

		children = SceneTree::get_root()->find_children("NestedNode", "", true, false);
		CHECK_EQ(children.size(), 1);
		CHECK_EQ(Object::cast_to<Node>(children[0]), node1_1);

		// First node that matches with the name is node1.
		child = SceneTree::get_root()->find_child("Node?", true, false);
		CHECK_EQ(child, node1);

		children = SceneTree::get_root()->find_children("Node?", "", true, false);
		CHECK_EQ(children.size(), 2);
		CHECK_EQ(Object::cast_to<Node>(children[0]), node1);
		CHECK_EQ(Object::cast_to<Node>(children[1]), node2);

		SceneTree::get_root()->move_child(node2, 0);

		// It should be node2, as it is now the first one in the tree.
		child = SceneTree::get_root()->find_child("Node?", true, false);
		CHECK_EQ(child, node2);

		children = SceneTree::get_root()->find_children("Node?", "", true, false);
		CHECK_EQ(children.size(), 2);
		CHECK_EQ(Object::cast_to<Node>(children[0]), node2);
		CHECK_EQ(Object::cast_to<Node>(children[1]), node1);
	}

	SUBCASE("Nodes should be accessible via their node path") {
		Node *child_by_path = SceneTree::get_root()->get_node_or_null(node1->get_path());
		CHECK_EQ(child_by_path, node1);

		child_by_path = SceneTree::get_root()->get_node_or_null(node2->get_path());
		CHECK_EQ(child_by_path, node2);

		child_by_path = SceneTree::get_root()->get_node_or_null(node1_1->get_path());
		CHECK_EQ(child_by_path, node1_1);

		node1->set_name("Node1");
		node1_1->set_name("NestedNode");

		child_by_path = node1->get_node_or_null(NodePath("NestedNode"));
		CHECK_EQ(child_by_path, node1_1);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("/root/Node1/NestedNode"));
		CHECK_EQ(child_by_path, node1_1);

		child_by_path = SceneTree::get_root()->get_node_or_null(NodePath("Node1/NestedNode"));
		CHECK_EQ(child_by_path, node1_1);
	}

	SUBCASE("Nodes should be accessible via their groups") {
		Vector<Node *> nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK(nodes.is_empty());

		nodes = SceneTree::get_nodes_in_group("other_nodes");
		CHECK(nodes.is_empty());

		node1->add_to_group("nodes");
		node2->add_to_group("other_nodes");
		node1_1->add_to_group("nodes");
		node1_1->add_to_group("other_nodes");

		nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK_EQ(nodes.size(), 2);

		Node *E = nodes.get(0);
		CHECK_EQ(E, node1);
		E = nodes.get(1);
		CHECK_EQ(E, node1_1);

		// Clear and try again with the other group.
		nodes.clear();

		nodes = SceneTree::get_nodes_in_group("other_nodes");
		CHECK_EQ(nodes.size(), 2);

		E = nodes.get(0);
		CHECK_EQ(E, node1_1);
		E = nodes.get(1);
		CHECK_EQ(E, node2);

		// Clear and try again with the other group and one node removed.
		nodes.clear();

		node1->remove_from_group("nodes");
		nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK_EQ(nodes.size(), 1);

		E = nodes.get(0);
		CHECK_EQ(E, node1_1);
	}

	SUBCASE("Nodes added as siblings of another node should be right next to it") {
		node1->remove_child(node1_1);

		node1->add_sibling(node1_1);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 3);
		CHECK_EQ(SceneTree::get_node_count(), 4);

		CHECK_EQ(SceneTree::get_root()->get_child(0), node1);
		CHECK_EQ(SceneTree::get_root()->get_child(1), node1_1);
		CHECK_EQ(SceneTree::get_root()->get_child(2), node2);
	}

	SUBCASE("Replaced nodes should be be removed and the replacing node added") {
		SceneTree::get_root()->remove_child(node2);

		node1->replace_by(node2);

		CHECK_EQ(SceneTree::get_root()->get_child_count(), 1);
		CHECK_EQ(SceneTree::get_node_count(), 3);

		CHECK_FALSE(node1->is_inside_tree());
		CHECK(node2->is_inside_tree());

		CHECK_EQ(node1->get_parent(), nullptr);
		CHECK_EQ(node2->get_parent(), SceneTree::get_root());
		CHECK_EQ(node2->get_child_count(), 1);
		CHECK_EQ(node2->get_child(0), node1_1);
	}

	SUBCASE("Replacing nodes should keep the groups of the replaced nodes") {
		SceneTree::get_root()->remove_child(node2);

		node1->add_to_group("nodes");
		node1->replace_by(node2, true);

		Vector<Node *> nodes = SceneTree::get_nodes_in_group("nodes");
		CHECK_EQ(nodes.size(), 1);

		Node *E = nodes.get(0);
		CHECK_EQ(E, node2);
	}

	SUBCASE("Duplicating a node should also duplicate the children") {
		node1->set_name("MyName1");
		node1_1->set_name("MyName1_1");
		Node *duplicate1 = node1->duplicate();

		CHECK_EQ(duplicate1->get_child_count(), node1->get_child_count());
		Node *duplicate1_1 = duplicate1->get_child(0);

		CHECK_EQ(duplicate1_1->get_child_count(), node1_1->get_child_count());

		CHECK_EQ(duplicate1->get_name(), node1->get_name());
		CHECK_EQ(duplicate1_1->get_name(), node1_1->get_name());

		CHECK_FALSE(duplicate1->is_inside_tree());
		CHECK_FALSE(duplicate1_1->is_inside_tree());

		memdelete(duplicate1_1);
		memdelete(duplicate1);
	}

	memdelete(node1_1);
	memdelete(node1);
	memdelete(node2);
}

TEST_CASE("[SceneTree][Node] Duplicating node with internal children") {
	GDREGISTER_CLASS(TestNode);

	TestNode *node = memnew(TestNode);
	Node *child = memnew(Node);
	child->set_name("Child");
	node->add_child(child);

	int child_count = node->get_child_count();

	Node *dup = node->duplicate();
	CHECK(dup->get_child_count() == child_count);
	CHECK(dup->has_node(String("Child")));

	memdelete(node);
	memdelete(dup);
}

TEST_CASE("[SceneTree][Node]Exported node checks") {
	TestNode *node = memnew(TestNode);
	SceneTree::get_root()->add_child(node);

	Node *child = memnew(Node);
	child->set_name("Child");
	node->add_child(child);
	child->set_owner(node);

	Node *child2 = memnew(Node);
	child2->set_name("Child2");
	node->add_child(child2);
	child2->set_owner(node);

	Array children;
	children.append(child);

	node->set("exported_node", child);
	node->set("exported_nodes", children);

	SUBCASE("Property of duplicated node should point to duplicated child") {
		GDREGISTER_CLASS(TestNode);

		TestNode *dup = Object::cast_to<TestNode>(node->duplicate());
		Node *new_exported = Object::cast_to<Node>(dup->get("exported_node"));
		CHECK(new_exported == dup->get_child(0, false));

		memdelete(dup);
	}

#ifdef TOOLS_ENABLED
	SUBCASE("Saving instance with exported nodes should not store the unchanged property") {
		Ref<PackedScene> ps;
		ps.instantiate();
		ps->pack(node);

		String scene_path = TestUtils::get_temp_path("test_scene.tscn");
		ps->set_path(scene_path);

		Node *root = memnew(Node);

		Node *sub_child = ps->instantiate(PackedScene::GEN_EDIT_STATE_MAIN);
		root->add_child(sub_child);
		sub_child->set_owner(root);

		Ref<PackedScene> ps2;
		ps2.instantiate();
		ps2->pack(root);

		scene_path = TestUtils::get_temp_path("new_test_scene.tscn");
		ResourceSaver::save(ps2, scene_path);
		memdelete(root);

		bool is_wrong = false;
		Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::READ);
		while (!fa->eof_reached()) {
			const String line = fa->get_line();
			if (line.begins_with("exported_node")) {
				// The property was saved, while it shouldn't.
				is_wrong = true;
				break;
			}
		}
		CHECK_FALSE(is_wrong);
	}

	SUBCASE("Saving instance with exported nodes should store property if changed") {
		Ref<PackedScene> ps;
		ps.instantiate();
		ps->pack(node);

		String scene_path = TestUtils::get_temp_path("test_scene.tscn");
		ps->set_path(scene_path);

		Node *root = memnew(Node);

		Node *sub_child = ps->instantiate(PackedScene::GEN_EDIT_STATE_MAIN);
		root->add_child(sub_child);
		sub_child->set_owner(root);

		sub_child->set("exported_node", sub_child->get_child(1, false));

		children = Array();
		children.append(sub_child->get_child(1, false));
		sub_child->set("exported_nodes", children);

		Ref<PackedScene> ps2;
		ps2.instantiate();
		ps2->pack(root);

		scene_path = TestUtils::get_temp_path("new_test_scene2.tscn");
		ResourceSaver::save(ps2, scene_path);
		memdelete(root);

		int stored_properties = 0;
		Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::READ);
		while (!fa->eof_reached()) {
			const String line = fa->get_line();
			if (line.begins_with("exported_node")) {
				stored_properties++;
			}
		}
		CHECK_EQ(stored_properties, 2);
	}
#endif // TOOLS_ENABLED

	memdelete(node);
}

TEST_CASE("[Node] Processing checks") {
	Node *node = memnew(Node);

	SUBCASE("Processing") {
		CHECK_FALSE(node->is_processing());

		node->set_process(true);

		CHECK(node->is_processing());

		node->set_process(false);

		CHECK_FALSE(node->is_processing());
	}

	SUBCASE("Physics processing") {
		CHECK_FALSE(node->is_physics_processing());

		node->set_physics_process(true);

		CHECK(node->is_physics_processing());

		node->set_physics_process(false);

		CHECK_FALSE(node->is_physics_processing());
	}

	SUBCASE("Unhandled input processing") {
		CHECK_FALSE(node->is_processing_unhandled_input());

		node->set_process_unhandled_input(true);

		CHECK(node->is_processing_unhandled_input());

		node->set_process_unhandled_input(false);

		CHECK_FALSE(node->is_processing_unhandled_input());
	}

	SUBCASE("Input processing") {
		CHECK_FALSE(node->is_processing_input());

		node->set_process_input(true);

		CHECK(node->is_processing_input());

		node->set_process_input(false);

		CHECK_FALSE(node->is_processing_input());
	}

	SUBCASE("Unhandled key input processing") {
		CHECK_FALSE(node->is_processing_unhandled_key_input());

		node->set_process_unhandled_key_input(true);

		CHECK(node->is_processing_unhandled_key_input());

		node->set_process_unhandled_key_input(false);

		CHECK_FALSE(node->is_processing_unhandled_key_input());
	}

	SUBCASE("Shortcut input processing") {
		CHECK_FALSE(node->is_processing_shortcut_input());

		node->set_process_shortcut_input(true);

		CHECK(node->is_processing_shortcut_input());

		node->set_process_shortcut_input(false);

		CHECK_FALSE(node->is_processing_shortcut_input());
	}

	SUBCASE("Internal processing") {
		CHECK_FALSE(node->is_processing_internal());

		node->set_process_internal(true);

		CHECK(node->is_processing_internal());

		node->set_process_internal(false);

		CHECK_FALSE(node->is_processing_internal());
	}

	SUBCASE("Process priority") {
		CHECK_EQ(0, node->get_process_priority());

		node->set_process_priority(1);

		CHECK_EQ(1, node->get_process_priority());
	}

	SUBCASE("Physics process priority") {
		CHECK_EQ(0, node->get_physics_process_priority());

		node->set_physics_process_priority(1);

		CHECK_EQ(1, node->get_physics_process_priority());
	}

	memdelete(node);
}

TEST_CASE("[SceneTree][Node] Test the processing") {
	TestNode *node = memnew(TestNode);
	SceneTree::get_root()->add_child(node);

	SUBCASE("No process") {
		CHECK_EQ(0, node->process_counter);
		CHECK_EQ(0, node->physics_process_counter);
	}

	SUBCASE("Process") {
		node->set_process(true);
		SceneTree::process(0);

		CHECK_EQ(1, node->process_counter);
		CHECK_EQ(0, node->physics_process_counter);
		CHECK_EQ(0, node->internal_process_counter);
		CHECK_EQ(0, node->internal_physics_process_counter);
	}

	SUBCASE("Physics process") {
		node->set_physics_process(true);
		SceneTree::physics_process(0);

		CHECK_EQ(0, node->process_counter);
		CHECK_EQ(1, node->physics_process_counter);
		CHECK_EQ(0, node->internal_process_counter);
		CHECK_EQ(0, node->internal_physics_process_counter);
	}

	SUBCASE("Normal and physics process") {
		node->set_process(true);
		node->set_physics_process(true);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		CHECK_EQ(1, node->process_counter);
		CHECK_EQ(1, node->physics_process_counter);
		CHECK_EQ(0, node->internal_process_counter);
		CHECK_EQ(0, node->internal_physics_process_counter);
	}

	SUBCASE("Internal, normal and physics process") {
		node->set_process_internal(true);
		node->set_physics_process_internal(true);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		CHECK_EQ(0, node->process_counter);
		CHECK_EQ(0, node->physics_process_counter);
		CHECK_EQ(1, node->internal_process_counter);
		CHECK_EQ(1, node->internal_physics_process_counter);
	}

	SUBCASE("All processing") {
		node->set_process(true);
		node->set_physics_process(true);
		node->set_process_internal(true);
		node->set_physics_process_internal(true);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		CHECK_EQ(1, node->process_counter);
		CHECK_EQ(1, node->physics_process_counter);
		CHECK_EQ(1, node->internal_process_counter);
		CHECK_EQ(1, node->internal_physics_process_counter);
	}

	SUBCASE("All processing twice") {
		node->set_process(true);
		node->set_physics_process(true);
		node->set_process_internal(true);
		node->set_physics_process_internal(true);
		SceneTree::process(0);
		SceneTree::physics_process(0);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		CHECK_EQ(2, node->process_counter);
		CHECK_EQ(2, node->physics_process_counter);
		CHECK_EQ(2, node->internal_process_counter);
		CHECK_EQ(2, node->internal_physics_process_counter);
	}

	SUBCASE("Enable and disable processing") {
		node->set_process(true);
		node->set_physics_process(true);
		node->set_process_internal(true);
		node->set_physics_process_internal(true);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		node->set_process(false);
		node->set_physics_process(false);
		node->set_process_internal(false);
		node->set_physics_process_internal(false);
		SceneTree::process(0);
		SceneTree::physics_process(0);

		CHECK_EQ(1, node->process_counter);
		CHECK_EQ(1, node->physics_process_counter);
		CHECK_EQ(1, node->internal_process_counter);
		CHECK_EQ(1, node->internal_physics_process_counter);
	}

	memdelete(node);
}

TEST_CASE("[SceneTree][Node] Test the process priority") {
	List<Node *> process_order;

	TestNode *node = memnew(TestNode);
	node->callback_list = &process_order;
	SceneTree::get_root()->add_child(node);

	TestNode *node2 = memnew(TestNode);
	node2->callback_list = &process_order;
	SceneTree::get_root()->add_child(node2);

	TestNode *node3 = memnew(TestNode);
	node3->callback_list = &process_order;
	SceneTree::get_root()->add_child(node3);

	TestNode *node4 = memnew(TestNode);
	node4->callback_list = &process_order;
	SceneTree::get_root()->add_child(node4);

	SUBCASE("Process priority") {
		node->set_process(true);
		node->set_process_priority(20);
		node2->set_process(true);
		node2->set_process_priority(10);
		node3->set_process(true);
		node3->set_process_priority(40);
		node4->set_process(true);
		node4->set_process_priority(30);

		SceneTree::process(0);

		CHECK_EQ(4, process_order.size());
		List<Node *>::Element *E = process_order.front();
		CHECK_EQ(E->get(), node2);
		E = E->next();
		CHECK_EQ(E->get(), node);
		E = E->next();
		CHECK_EQ(E->get(), node4);
		E = E->next();
		CHECK_EQ(E->get(), node3);
	}

	SUBCASE("Physics process priority") {
		node->set_physics_process(true);
		node->set_physics_process_priority(20);
		node2->set_physics_process(true);
		node2->set_physics_process_priority(10);
		node3->set_physics_process(true);
		node3->set_physics_process_priority(40);
		node4->set_physics_process(true);
		node4->set_physics_process_priority(30);

		SceneTree::physics_process(0);

		CHECK_EQ(4, process_order.size());
		List<Node *>::Element *E = process_order.front();
		CHECK_EQ(E->get(), node2);
		E = E->next();
		CHECK_EQ(E->get(), node);
		E = E->next();
		CHECK_EQ(E->get(), node4);
		E = E->next();
		CHECK_EQ(E->get(), node3);
	}

	memdelete(node);
	memdelete(node2);
	memdelete(node3);
	memdelete(node4);
}

} // namespace TestNode

// === Merged from tests/scene/test_node_2d.cpp ===
/*  test_node_2d.cpp                                                      */
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



#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestNode2D {

TEST_CASE("[SceneTree][Node2D]") {
	SUBCASE("[Node2D][Global Transform] Global Transform should be accessible while not in SceneTree.") { // GH-79453
		Node2D *test_node = memnew(Node2D);
		test_node->set_name("node");
		Node2D *test_child = memnew(Node2D);
		test_child->set_name("child");
		test_node->add_child(test_child);

		test_node->set_global_position(Point2(1, 1));
		CHECK_EQ(test_node->get_global_position(), Point2(1, 1));
		CHECK_EQ(test_child->get_global_position(), Point2(1, 1));
		test_node->set_global_position(Point2(2, 2));
		CHECK_EQ(test_node->get_global_position(), Point2(2, 2));
		test_node->set_global_transform(Transform2D(0, Point2(3, 3)));
		CHECK_EQ(test_node->get_global_position(), Point2(3, 3));

		memdelete(test_child);
		memdelete(test_node);
	}

	SUBCASE("[Node2D][Global Transform] Global Transform should be correct after inserting node from detached tree into SceneTree.") { // GH-86841
		Node2D *main = memnew(Node2D);
		Node2D *outer = memnew(Node2D);
		Node2D *inner = memnew(Node2D);
		SceneTree::get_root()->add_child(main);

		main->set_position(Point2(100, 100));
		outer->set_position(Point2(10, 0));
		inner->set_position(Point2(0, 10));

		outer->add_child(inner);
		// `inner` is still detached.
		CHECK_EQ(inner->get_global_position(), Point2(10, 10));

		main->add_child(outer);
		// `inner` is in scene tree.
		CHECK_EQ(inner->get_global_position(), Point2(110, 110));

		main->remove_child(outer);
		// `inner` is detached again.
		CHECK_EQ(inner->get_global_position(), Point2(10, 10));

		memdelete(inner);
		memdelete(outer);
		memdelete(main);
	}
}

TEST_CASE("[SceneTree][Node2D] Utility methods") {
	Node2D *test_node1 = memnew(Node2D);
	Node2D *test_node2 = memnew(Node2D);
	Node2D *test_node3 = memnew(Node2D);
	Node2D *test_sibling = memnew(Node2D);
	SceneTree::get_root()->add_child(test_node1);

	test_node1->set_position(Point2(100, 100));
	test_node1->set_rotation(Math::deg_to_rad(30.0));
	test_node1->set_scale(Size2(1, 1));
	test_node1->add_child(test_node2);

	test_node2->set_position(Point2(10, 0));
	test_node2->set_rotation(Math::deg_to_rad(60.0));
	test_node2->set_scale(Size2(1, 1));
	test_node2->add_child(test_node3);
	test_node2->add_child(test_sibling);

	test_node3->set_position(Point2(0, 10));
	test_node3->set_rotation(Math::deg_to_rad(0.0));
	test_node3->set_scale(Size2(2, 2));

	test_sibling->set_position(Point2(5, 10));
	test_sibling->set_rotation(Math::deg_to_rad(90.0));
	test_sibling->set_scale(Size2(2, 1));

	SUBCASE("[Node2D] look_at") {
		test_node3->look_at(Vector2(1, 1));

		CHECK(test_node3->get_global_position().is_equal_approx(Point2(98.66026, 105)));
		CHECK(Math::is_equal_approx(test_node3->get_global_rotation(), real_t(-2.32477)));
		CHECK(test_node3->get_global_scale().is_equal_approx(Vector2(2, 2)));

		CHECK(test_node3->get_position().is_equal_approx(Vector2(0, 10)));
		CHECK(Math::is_equal_approx(test_node3->get_rotation(), real_t(2.38762)));
		CHECK(test_node3->get_scale().is_equal_approx(Vector2(2, 2)));

		test_node3->look_at(Vector2(0, 10));

		CHECK(test_node3->get_global_position().is_equal_approx(Vector2(98.66026, 105)));
		CHECK(Math::is_equal_approx(test_node3->get_global_rotation(), real_t(-2.37509)));
		CHECK(test_node3->get_global_scale().is_equal_approx(Vector2(2, 2)));

		CHECK(test_node3->get_position().is_equal_approx(Vector2(0, 10)));
		CHECK(Math::is_equal_approx(test_node3->get_rotation(), real_t(2.3373)));
		CHECK(test_node3->get_scale().is_equal_approx(Vector2(2, 2)));

		// Don't do anything if look_at own position.
		test_node3->look_at(test_node3->get_global_position());

		CHECK(test_node3->get_global_position().is_equal_approx(Vector2(98.66026, 105)));
		CHECK(Math::is_equal_approx(test_node3->get_global_rotation(), real_t(-2.37509)));
		CHECK(test_node3->get_global_scale().is_equal_approx(Vector2(2, 2)));

		CHECK(test_node3->get_position().is_equal_approx(Vector2(0, 10)));
		CHECK(Math::is_equal_approx(test_node3->get_rotation(), real_t(2.3373)));
		CHECK(test_node3->get_scale().is_equal_approx(Vector2(2, 2)));

		// Revert any rotation caused by look_at, must run after look_at tests
		test_node3->set_rotation(Math::deg_to_rad(0.0));
	}

	SUBCASE("[Node2D] get_angle_to") {
		CHECK(Math::is_equal_approx(test_node3->get_angle_to(Vector2(1, 1)), real_t(2.38762)));
		CHECK(Math::is_equal_approx(test_node3->get_angle_to(Vector2(0, 10)), real_t(2.3373)));
		CHECK(Math::is_equal_approx(test_node3->get_angle_to(Vector2(2, -5)), real_t(2.42065)));
		CHECK(Math::is_equal_approx(test_node3->get_angle_to(Vector2(-2, 5)), real_t(2.3529)));

		// Return 0 when get_angle_to own position.
		CHECK(Math::is_equal_approx(test_node3->get_angle_to(test_node3->get_global_position()), real_t(0)));
	}

	SUBCASE("[Node2D] to_local") {
		Point2 node3_local = test_node3->to_local(Point2(1, 2));
		CHECK(node3_local.is_equal_approx(Point2(-51.5, 48.83013)));

		node3_local = test_node3->to_local(Point2(-2, 1));
		CHECK(node3_local.is_equal_approx(Point2(-52, 50.33013)));

		node3_local = test_node3->to_local(Point2(0, 0));
		CHECK(node3_local.is_equal_approx(Point2(-52.5, 49.33013)));

		node3_local = test_node3->to_local(test_node3->get_global_position());
		CHECK(node3_local.is_equal_approx(Point2(0, 0)));
	}

	SUBCASE("[Node2D] to_global") {
		Point2 node3_global = test_node3->to_global(Point2(1, 2));
		CHECK(node3_global.is_equal_approx(Point2(94.66026, 107)));

		node3_global = test_node3->to_global(Point2(-2, 1));
		CHECK(node3_global.is_equal_approx(Point2(96.66026, 101)));

		node3_global = test_node3->to_global(Point2(0, 0));
		CHECK(node3_global.is_equal_approx(test_node3->get_global_position()));
	}

	SUBCASE("[Node2D] get_relative_transform_to_parent") {
		Transform2D relative_xform = test_node3->get_relative_transform_to_parent(test_node3);
		CHECK(relative_xform.is_equal_approx(Transform2D()));

		relative_xform = test_node3->get_relative_transform_to_parent(test_node2);
		CHECK(relative_xform.get_origin().is_equal_approx(Vector2(0, 10)));
		CHECK(Math::is_equal_approx(relative_xform.get_rotation(), real_t(0)));
		CHECK(relative_xform.get_scale().is_equal_approx(Vector2(2, 2)));

		relative_xform = test_node3->get_relative_transform_to_parent(test_node1);
		CHECK(relative_xform.get_origin().is_equal_approx(Vector2(1.339746, 5)));
		CHECK(Math::is_equal_approx(relative_xform.get_rotation(), real_t(1.0472)));
		CHECK(relative_xform.get_scale().is_equal_approx(Vector2(2, 2)));

		ERR_PRINT_OFF;
		// In case of a sibling all transforms until the root are accumulated.
		Transform2D xform = test_node3->get_relative_transform_to_parent(test_sibling);
		Transform2D return_xform = test_node1->get_global_transform().inverse() * test_node3->get_global_transform();
		CHECK(xform.is_equal_approx(return_xform));
		ERR_PRINT_ON;
	}

	memdelete(test_sibling);
	memdelete(test_node3);
	memdelete(test_node2);
	memdelete(test_node1);
}

} // namespace TestNode2D

// === Merged from tests/scene/test_parallax_2d.cpp ===
/*  test_parallax_2d.cpp                                                  */
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



#include "scene/main/scene_tree.h"

// Test cases for the Parallax2D class to ensure its properties are set and retrieved correctly.

namespace TestParallax2D {

TEST_CASE("[SceneTree][Parallax2D] Scroll Scale") {
	// Test setting and getting the scroll scale.
	Parallax2D *parallax = memnew(Parallax2D);
	Size2 scale(2, 2);
	parallax->set_scroll_scale(scale);
	CHECK(parallax->get_scroll_scale() == scale);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Repeat Size") {
	// Test setting and getting the repeat size.
	Parallax2D *parallax = memnew(Parallax2D);
	Size2 size(100, 100);
	parallax->set_repeat_size(size);
	CHECK(parallax->get_repeat_size() == size);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Repeat Times") {
	// Test setting and getting the repeat times.
	Parallax2D *parallax = memnew(Parallax2D);
	int times = 5;
	parallax->set_repeat_times(times);
	CHECK(parallax->get_repeat_times() == times);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Autoscroll") {
	// Test setting and getting the autoscroll values.
	Parallax2D *parallax = memnew(Parallax2D);
	Point2 autoscroll(1, 1);
	parallax->set_autoscroll(autoscroll);
	CHECK(parallax->get_autoscroll() == autoscroll);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Scroll Offset") {
	// Test setting and getting the scroll offset.
	Parallax2D *parallax = memnew(Parallax2D);
	Point2 offset(10, 10);
	parallax->set_scroll_offset(offset);
	CHECK(parallax->get_scroll_offset() == offset);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Screen Offset") {
	// Test setting and getting the screen offset.
	Parallax2D *parallax = memnew(Parallax2D);
	Point2 offset(20, 20);
	parallax->set_screen_offset(offset);
	CHECK(parallax->get_screen_offset() == offset);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Limit Begin") {
	// Test setting and getting the limit begin values.
	Parallax2D *parallax = memnew(Parallax2D);
	Point2 limit_begin(-100, -100);
	parallax->set_limit_begin(limit_begin);
	CHECK(parallax->get_limit_begin() == limit_begin);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Limit End") {
	// Test setting and getting the limit end values.
	Parallax2D *parallax = memnew(Parallax2D);
	Point2 limit_end(100, 100);
	parallax->set_limit_end(limit_end);
	CHECK(parallax->get_limit_end() == limit_end);
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Follow Viewport") {
	// Test setting and getting the follow viewport flag.
	Parallax2D *parallax = memnew(Parallax2D);
	parallax->set_follow_viewport(false);
	CHECK_FALSE(parallax->get_follow_viewport());
	memdelete(parallax);
}

TEST_CASE("[SceneTree][Parallax2D] Ignore Camera Scroll") {
	// Test setting and getting the ignore camera scroll flag.
	Parallax2D *parallax = memnew(Parallax2D);
	parallax->set_ignore_camera_scroll(true);
	CHECK(parallax->is_ignore_camera_scroll());
	memdelete(parallax);
}

} // namespace TestParallax2D

// === Merged from tests/scene/test_path_2d.cpp ===
/*  test_path_2d.cpp                                                      */
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



#include "scene/main/scene_tree.h"

namespace TestPath2D {

TEST_CASE("[SceneTree][Path2D] Initialization") {
	SUBCASE("Path should be empty right after initialization") {
		Path2D *test_path = memnew(Path2D);
		CHECK(test_path->get_curve().is_null());
		memdelete(test_path);
	}
}

TEST_CASE("[SceneTree][Path2D] Curve setter and getter") {
	Path2D *test_path = memnew(Path2D);
	const Ref<Curve2D> &curve = memnew(Curve2D);

	SUBCASE("Curve passed to the class should remain the same") {
		test_path->set_curve(curve);
		CHECK(test_path->get_curve() == curve);
	}
	SUBCASE("Curve passed many times to the class should remain the same") {
		test_path->set_curve(curve);
		test_path->set_curve(curve);
		test_path->set_curve(curve);
		CHECK(test_path->get_curve() == curve);
	}
	SUBCASE("Curve rewrite testing") {
		const Ref<Curve2D> &curve1 = memnew(Curve2D);
		const Ref<Curve2D> &curve2 = memnew(Curve2D);

		test_path->set_curve(curve1);
		test_path->set_curve(curve2);
		CHECK_MESSAGE(test_path->get_curve() != curve1,
				"After rewrite, second curve should be in class");
		CHECK_MESSAGE(test_path->get_curve() == curve2,
				"After rewrite, second curve should be in class");
	}

	SUBCASE("Assign same curve to two paths") {
		Path2D *path2 = memnew(Path2D);

		test_path->set_curve(curve);
		path2->set_curve(curve);
		CHECK_MESSAGE(test_path->get_curve() == path2->get_curve(),
				"Both paths have the same curve.");
		memdelete(path2);
	}

	SUBCASE("Swapping curves between two paths") {
		Path2D *path2 = memnew(Path2D);
		const Ref<Curve2D> &curve1 = memnew(Curve2D);
		const Ref<Curve2D> &curve2 = memnew(Curve2D);

		test_path->set_curve(curve1);
		path2->set_curve(curve2);
		CHECK(test_path->get_curve() == curve1);
		CHECK(path2->get_curve() == curve2);

		// Do the swap
		Ref<Curve2D> temp = test_path->get_curve();
		test_path->set_curve(path2->get_curve());
		path2->set_curve(temp);

		CHECK(test_path->get_curve() == curve2);
		CHECK(path2->get_curve() == curve1);
		memdelete(path2);
	}

	memdelete(test_path);
}

} // namespace TestPath2D

// === Merged from tests/scene/test_path_3d.cpp ===
/*  test_path_3d.cpp                                                      */
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

#include "scene/main/scene_tree.h"

namespace TestPath3D {

TEST_CASE("[Path3D] Initialization") {
	SUBCASE("Path should be empty right after initialization") {
		Path3D *test_path = memnew(Path3D);
		CHECK(test_path->get_curve().is_null());
		memdelete(test_path);
	}
}

TEST_CASE("[Path3D] Curve setter and getter") {
	SUBCASE("Curve passed to the class should remain the same") {
		Path3D *test_path = memnew(Path3D);
		const Ref<Curve3D> &curve = memnew(Curve3D);

		test_path->set_curve(curve);
		CHECK(test_path->get_curve() == curve);
		memdelete(test_path);
	}
	SUBCASE("Curve passed many times to the class should remain the same") {
		Path3D *test_path = memnew(Path3D);
		const Ref<Curve3D> &curve = memnew(Curve3D);

		test_path->set_curve(curve);
		test_path->set_curve(curve);
		test_path->set_curve(curve);
		CHECK(test_path->get_curve() == curve);
		memdelete(test_path);
	}
	SUBCASE("Curve rewrite testing") {
		Path3D *test_path = memnew(Path3D);
		const Ref<Curve3D> &curve1 = memnew(Curve3D);
		const Ref<Curve3D> &curve2 = memnew(Curve3D);

		test_path->set_curve(curve1);
		test_path->set_curve(curve2);
		CHECK_MESSAGE(test_path->get_curve() != curve1,
				"After rewrite, second curve should be in class");
		CHECK_MESSAGE(test_path->get_curve() == curve2,
				"After rewrite, second curve should be in class");
		memdelete(test_path);
	}
}

} // namespace TestPath3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_path_follow_2d.cpp ===
/*  test_path_follow_2d.cpp                                               */
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



#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestPathFollow2D {

bool is_equal_approx(const Vector2 &p_a, const Vector2 &p_b) {
	const real_t tolerance = 0.001;
	return Math::is_equal_approx(p_a.x, p_b.x, tolerance) &&
			Math::is_equal_approx(p_a.y, p_b.y, tolerance);
}

TEST_CASE("[SceneTree][PathFollow2D] Sampling with progress ratio") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->set_bake_interval(1);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	curve->add_point(Vector2(100, 100));
	curve->add_point(Vector2(0, 100));
	curve->add_point(Vector2(0, 0));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path_follow_2d->set_loop(false);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_progress_ratio(0);
	CHECK(is_equal_approx(Vector2(0, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.125);
	CHECK(is_equal_approx(Vector2(50, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.25);
	CHECK(is_equal_approx(Vector2(100, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.375);
	CHECK(is_equal_approx(Vector2(100, 50), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.5);
	CHECK(is_equal_approx(Vector2(100, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.625);
	CHECK(is_equal_approx(Vector2(50, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.75);
	CHECK(is_equal_approx(Vector2(0, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(0.875);
	CHECK(is_equal_approx(Vector2(0, 50), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress_ratio(1);
	CHECK(is_equal_approx(Vector2(0, 0), path_follow_2d->get_transform().get_origin()));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow2D] Sampling with progress") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->set_bake_interval(1);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	curve->add_point(Vector2(100, 100));
	curve->add_point(Vector2(0, 100));
	curve->add_point(Vector2(0, 0));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path_follow_2d->set_loop(false);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_progress(0);
	CHECK(is_equal_approx(Vector2(0, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(50);
	CHECK(is_equal_approx(Vector2(50, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(100);
	CHECK(is_equal_approx(Vector2(100, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(150);
	CHECK(is_equal_approx(Vector2(100, 50), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(200);
	CHECK(is_equal_approx(Vector2(100, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(250);
	CHECK(is_equal_approx(Vector2(50, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(300);
	CHECK(is_equal_approx(Vector2(0, 100), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(350);
	CHECK(is_equal_approx(Vector2(0, 50), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_progress(400);
	CHECK(is_equal_approx(Vector2(0, 0), path_follow_2d->get_transform().get_origin()));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow2D] Removal of a point in curve") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	curve->add_point(Vector2(100, 100));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_progress_ratio(0.5);
	CHECK(is_equal_approx(Vector2(100, 0), path_follow_2d->get_transform().get_origin()));

	curve->remove_point(1);

	path_follow_2d->set_progress_ratio(0.5);
	CHECK_MESSAGE(
			is_equal_approx(Vector2(50, 50), path_follow_2d->get_transform().get_origin()),
			"Path follow's position should be updated after removing a point from the curve");

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow2D] Setting h_offset and v_offset") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_progress_ratio(0.5);
	CHECK(is_equal_approx(Vector2(50, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_h_offset(25);
	CHECK(is_equal_approx(Vector2(75, 0), path_follow_2d->get_transform().get_origin()));

	path_follow_2d->set_v_offset(25);
	CHECK(is_equal_approx(Vector2(75, 25), path_follow_2d->get_transform().get_origin()));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow2D] Progress ratio out of range") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_loop(true);

	path_follow_2d->set_progress_ratio(-0.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress_ratio(), (real_t)0.7),
			"Progress Ratio should loop back from the end in the opposite direction");

	path_follow_2d->set_progress_ratio(1.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress_ratio(), (real_t)0.3),
			"Progress Ratio should loop back from the end in the opposite direction");

	path_follow_2d->set_loop(false);

	path_follow_2d->set_progress_ratio(-0.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress_ratio(), 0),
			"Progress Ratio should be clamped at 0");

	path_follow_2d->set_progress_ratio(1.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress_ratio(), 1),
			"Progress Ratio should be clamped at 1");

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow2D] Progress out of range") {
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(100, 0));
	Path2D *path = memnew(Path2D);
	path->set_curve(curve);
	PathFollow2D *path_follow_2d = memnew(PathFollow2D);
	path->add_child(path_follow_2d);
	SceneTree::get_root()->add_child(path);

	path_follow_2d->set_loop(true);

	path_follow_2d->set_progress(-50);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress(), 50),
			"Progress should loop back from the end in the opposite direction");

	path_follow_2d->set_progress(150);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress(), 50),
			"Progress should loop back from the end in the opposite direction");

	path_follow_2d->set_loop(false);

	path_follow_2d->set_progress(-50);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress(), 0),
			"Progress should be clamped at 0");

	path_follow_2d->set_progress(150);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_2d->get_progress(), 100),
			"Progress should be clamped at 1");

	memdelete(path);
}

} // namespace TestPathFollow2D

// === Merged from tests/scene/test_path_follow_3d.cpp ===
/*  test_path_follow_3d.cpp                                               */
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

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestPathFollow3D {

bool is_equal_approx(const Vector3 &p_a, const Vector3 &p_b) {
	const real_t tolerance = 0.001;
	return Math::is_equal_approx(p_a.x, p_b.x, tolerance) &&
			Math::is_equal_approx(p_a.y, p_b.y, tolerance) &&
			Math::is_equal_approx(p_a.z, p_b.z, tolerance);
}

TEST_CASE("[SceneTree][PathFollow3D] Sampling with progress ratio") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	curve->add_point(Vector3(100, 100, 0));
	curve->add_point(Vector3(100, 100, 100));
	curve->add_point(Vector3(100, 0, 100));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path_follow_3d->set_loop(false);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_progress_ratio(0);
	CHECK(is_equal_approx(Vector3(0, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.125);
	CHECK(is_equal_approx(Vector3(50, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.25);
	CHECK(is_equal_approx(Vector3(100, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.375);
	CHECK(is_equal_approx(Vector3(100, 50, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.5);
	CHECK(is_equal_approx(Vector3(100, 100, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.625);
	CHECK(is_equal_approx(Vector3(100, 100, 50), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.75);
	CHECK(is_equal_approx(Vector3(100, 100, 100), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(0.875);
	CHECK(is_equal_approx(Vector3(100, 50, 100), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress_ratio(1);
	CHECK(is_equal_approx(Vector3(100, 0, 100), path_follow_3d->get_transform().get_origin()));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Sampling with progress") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	curve->add_point(Vector3(100, 100, 0));
	curve->add_point(Vector3(100, 100, 100));
	curve->add_point(Vector3(100, 0, 100));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path_follow_3d->set_loop(false);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_progress(0);
	CHECK(is_equal_approx(Vector3(0, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(50);
	CHECK(is_equal_approx(Vector3(50, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(100);
	CHECK(is_equal_approx(Vector3(100, 0, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(150);
	CHECK(is_equal_approx(Vector3(100, 50, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(200);
	CHECK(is_equal_approx(Vector3(100, 100, 0), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(250);
	CHECK(is_equal_approx(Vector3(100, 100, 50), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(300);
	CHECK(is_equal_approx(Vector3(100, 100, 100), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(350);
	CHECK(is_equal_approx(Vector3(100, 50, 100), path_follow_3d->get_transform().get_origin()));

	path_follow_3d->set_progress(400);
	CHECK(is_equal_approx(Vector3(100, 0, 100), path_follow_3d->get_transform().get_origin()));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Removal of a point in curve") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	curve->add_point(Vector3(100, 100, 0));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_progress_ratio(0.5);
	CHECK(is_equal_approx(Vector3(100, 0, 0), path_follow_3d->get_transform().get_origin()));

	curve->remove_point(1);

	path_follow_3d->set_progress_ratio(0.5);
	CHECK_MESSAGE(
			is_equal_approx(Vector3(50, 50, 0), path_follow_3d->get_transform().get_origin()),
			"Path follow's position should be updated after removing a point from the curve");

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Progress ratio out of range") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_loop(true);

	path_follow_3d->set_progress_ratio(-0.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress_ratio(), (real_t)0.7),
			"Progress Ratio should loop back from the end in the opposite direction");

	path_follow_3d->set_progress_ratio(1.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress_ratio(), (real_t)0.3),
			"Progress Ratio should loop back from the end in the opposite direction");

	path_follow_3d->set_loop(false);

	path_follow_3d->set_progress_ratio(-0.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress_ratio(), 0),
			"Progress Ratio should be clamped at 0");

	path_follow_3d->set_progress_ratio(1.3);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress_ratio(), 1),
			"Progress Ratio should be clamped at 1");

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Progress out of range") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_loop(true);

	path_follow_3d->set_progress(-50);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress(), 50),
			"Progress should loop back from the end in the opposite direction");

	path_follow_3d->set_progress(150);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress(), 50),
			"Progress should loop back from the end in the opposite direction");

	path_follow_3d->set_loop(false);

	path_follow_3d->set_progress(-50);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress(), 0),
			"Progress should be clamped at 0");

	path_follow_3d->set_progress(150);
	CHECK_MESSAGE(
			Math::is_equal_approx(path_follow_3d->get_progress(), 100),
			"Progress should be clamped at max value of curve");

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Calculate forward vector") {
	const real_t dist_cube_100 = 100 * Math::sqrt(3.0);
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(100, 0, 0));
	curve->add_point(Vector3(200, 100, -100));
	curve->add_point(Vector3(200, 100, 200));
	curve->add_point(Vector3(100, 0, 100));
	curve->add_point(Vector3(0, 0, 100));
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_loop(false);
	path_follow_3d->set_rotation_mode(PathFollow3D::RotationMode::ROTATION_ORIENTED);

	path_follow_3d->set_progress(-50);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(0);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(50);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(100);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(100 + dist_cube_100 / 2);
	CHECK(is_equal_approx(Vector3(-0.577348, -0.577348, 0.577348), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(100 + dist_cube_100 - 0.01);
	CHECK(is_equal_approx(Vector3(-0.577348, -0.577348, 0.577348), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(250 + dist_cube_100);
	CHECK(is_equal_approx(Vector3(0, 0, -1), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(400 + dist_cube_100 - 0.01);
	CHECK(is_equal_approx(Vector3(0, 0, -1), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(400 + 1.5 * dist_cube_100);
	CHECK(is_equal_approx(Vector3(0.577348, 0.577348, 0.577348), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(400 + 2 * dist_cube_100 - 0.01);
	CHECK(is_equal_approx(Vector3(0.577348, 0.577348, 0.577348), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress(500 + 2 * dist_cube_100);
	CHECK(is_equal_approx(Vector3(1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	memdelete(path);
}

TEST_CASE("[SceneTree][PathFollow3D] Calculate forward vector with degenerate curves") {
	Ref<Curve3D> curve;
	curve.instantiate();
	curve->add_point(Vector3(0, 0, 1), Vector3(), Vector3(1, 0, 0));
	curve->add_point(Vector3(1, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0));
	curve->add_point(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(-1, 0, 0));
	curve->add_point(Vector3(-1, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0));
	curve->add_point(Vector3(0, 0, 1), Vector3(-1, 0, 0), Vector3());
	Path3D *path = memnew(Path3D);
	path->set_curve(curve);
	PathFollow3D *path_follow_3d = memnew(PathFollow3D);
	path->add_child(path_follow_3d);
	SceneTree::get_root()->add_child(path);

	path_follow_3d->set_loop(false);
	path_follow_3d->set_rotation_mode(PathFollow3D::RotationMode::ROTATION_ORIENTED);

	path_follow_3d->set_progress_ratio(0.00);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.25);
	CHECK(is_equal_approx(Vector3(0, 0, 1), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.50);
	CHECK(is_equal_approx(Vector3(1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.75);
	CHECK(is_equal_approx(Vector3(0, 0, -1), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(1.00);
	CHECK(is_equal_approx(Vector3(-1, 0, 0), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.125);
	CHECK(is_equal_approx(Vector3(-0.688375, 0, 0.725355), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.375);
	CHECK(is_equal_approx(Vector3(0.688375, 0, 0.725355), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.625);
	CHECK(is_equal_approx(Vector3(0.688375, 0, -0.725355), path_follow_3d->get_transform().get_basis().get_column(2)));

	path_follow_3d->set_progress_ratio(0.875);
	CHECK(is_equal_approx(Vector3(-0.688375, 0, -0.725355), path_follow_3d->get_transform().get_basis().get_column(2)));

	memdelete(path);
}

} // namespace TestPathFollow3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_physics_material.cpp ===
/*  test_physics_material.cpp                                             */
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



#ifndef PHYSICS_3D_DISABLED

#include "scene/resources/material.h"

namespace TestPhysicsMaterial {

TEST_CASE("[Physics_material] Defaults") {
	Ref<PhysicsMaterial> physics_material;
	physics_material.instantiate();

	CHECK(physics_material->get_friction() == 1.);
	CHECK(physics_material->is_rough() == false);
	CHECK(physics_material->get_bounce() == 0.);
	CHECK(physics_material->is_absorbent() == false);
}

TEST_CASE("[Physics_material] Friction") {
	Ref<PhysicsMaterial> physics_material;
	physics_material.instantiate();

	real_t friction = 0.314;
	physics_material->set_friction(friction);
	CHECK(physics_material->get_friction() == friction);
}

TEST_CASE("[Physics_material] Rough") {
	Ref<PhysicsMaterial> physics_material;
	physics_material.instantiate();

	bool rough = true;
	physics_material->set_rough(rough);
	CHECK(physics_material->is_rough() == rough);

	real_t friction = 0.314;
	physics_material->set_friction(friction);
	CHECK(physics_material->computed_friction() == -friction);

	rough = false;
	physics_material->set_rough(rough);
	CHECK(physics_material->is_rough() == rough);

	CHECK(physics_material->computed_friction() == friction);
}

TEST_CASE("[Physics_material] Bounce") {
	Ref<PhysicsMaterial> physics_material;
	physics_material.instantiate();

	real_t bounce = 0.271;
	physics_material->set_bounce(bounce);
	CHECK(physics_material->get_bounce() == bounce);
}

TEST_CASE("[Physics_material] Absorbent") {
	Ref<PhysicsMaterial> physics_material;
	physics_material.instantiate();

	bool absorbent = true;
	physics_material->set_absorbent(absorbent);
	CHECK(physics_material->is_absorbent() == absorbent);

	real_t bounce = 0.271;
	physics_material->set_bounce(bounce);
	CHECK(physics_material->computed_bounce() == -bounce);

	absorbent = false;
	physics_material->set_absorbent(absorbent);
	CHECK(physics_material->is_absorbent() == absorbent);

	CHECK(physics_material->computed_bounce() == bounce);
}

} // namespace TestPhysicsMaterial

#endif // PHYSICS_3D_DISABLED

// === Merged from tests/scene/test_primitives.cpp ===
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

// === Merged from tests/scene/test_reference_rect.cpp ===
/*  test_reference_rect.cpp                                               */
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



#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestReferenceRect {

TEST_CASE("[SceneTree][ReferenceRect] Default properties") {
	ReferenceRect *reference_rect = memnew(ReferenceRect);

	CHECK(reference_rect->get_border_color() == Color(1, 0, 0, 1));
	CHECK(reference_rect->get_border_width() == 1.0f);
	CHECK(reference_rect->get_editor_only());

	CHECK(reference_rect->get_mouse_filter() == Control::MOUSE_FILTER_STOP);

	CHECK(reference_rect->get_combined_minimum_size() == Size2(0, 0));

	memdelete(reference_rect);
}

TEST_CASE("[SceneTree][ReferenceRect] Border color") {
	ReferenceRect *reference_rect = memnew(ReferenceRect);

	const Color c = Color(0.25f, 0.5f, 0.75f, 0.9f);
	reference_rect->set_border_color(c);
	CHECK(reference_rect->get_border_color() == c);

	memdelete(reference_rect);
}

TEST_CASE("[SceneTree][ReferenceRect] Border width") {
	ReferenceRect *reference_rect = memnew(ReferenceRect);

	SUBCASE("Positive width is stored") {
		reference_rect->set_border_width(3.5f);
		CHECK(reference_rect->get_border_width() == 3.5f);
	}

	SUBCASE("Negative width clamps to zero") {
		reference_rect->set_border_width(-2.0f);
		CHECK(reference_rect->get_border_width() == 0.0f);
	}

	SUBCASE("Width above the inspector hint range is still stored") {
		reference_rect->set_border_width(12.0f);
		CHECK(reference_rect->get_border_width() == 12.0f);
	}

	memdelete(reference_rect);
}

TEST_CASE("[SceneTree][ReferenceRect] Editor only flag") {
	ReferenceRect *reference_rect = memnew(ReferenceRect);

	SUBCASE("editor_only can be disabled") {
		reference_rect->set_editor_only(false);
		CHECK_FALSE(reference_rect->get_editor_only());
	}

	SUBCASE("editor_only can be re-enabled") {
		reference_rect->set_editor_only(false);
		reference_rect->set_editor_only(true);
		CHECK(reference_rect->get_editor_only());
	}

	SUBCASE("Toggling editor_only does not change minimum size") {
		reference_rect->set_editor_only(false);
		CHECK(reference_rect->get_combined_minimum_size() == Size2(0, 0));

		reference_rect->set_editor_only(true);
		CHECK(reference_rect->get_combined_minimum_size() == Size2(0, 0));
	}

	memdelete(reference_rect);
}

TEST_CASE("[SceneTree][ReferenceRect] Minimum size on tree") {
	ReferenceRect *reference_rect = memnew(ReferenceRect);
	Window *root = SceneTree::get_root();
	root->add_child(reference_rect);

	reference_rect->set_border_width(4.0f);
	reference_rect->set_editor_only(false);
	reference_rect->set_border_color(Color(0, 1, 0, 1));
	SceneTree::process(0);

	CHECK(reference_rect->get_combined_minimum_size() == Size2(0, 0));

	memdelete(reference_rect);
}

} // namespace TestReferenceRect

// === Merged from tests/scene/test_skeleton_3d.cpp ===
/*  test_skeleton_3d.cpp                                                  */
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

#include "scene/main/scene_tree.h"

namespace TestSkeleton3D {

TEST_CASE("[Skeleton3D] Test per-bone meta") {
	Skeleton3D *skeleton = memnew(Skeleton3D);
	skeleton->add_bone("root");
	skeleton->set_bone_rest(0, Transform3D());

	// Adding meta to bone.
	skeleton->set_bone_meta(0, "key1", "value1");
	skeleton->set_bone_meta(0, "key2", 12345);
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key1") == "value1", "Bone meta missing.");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key2") == Variant(12345), "Bone meta missing.");

	// Rename bone and check if meta persists.
	skeleton->set_bone_name(0, "renamed_root");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key1") == "value1", "Bone meta missing.");
	CHECK_MESSAGE(skeleton->get_bone_meta(0, "key2") == Variant(12345), "Bone meta missing.");

	// Retrieve list of keys.
	List<String> keys;
	skeleton->get_bone_meta_list(0, &keys);
	CHECK_MESSAGE(keys.size() == 2, "Wrong number of bone meta keys.");
	CHECK_MESSAGE(keys.find("key1"), "key1 not found in bone meta list");
	CHECK_MESSAGE(keys.find("key2"), "key2 not found in bone meta list");

	// Removing meta.
	skeleton->set_bone_meta(0, "key1", Variant());
	skeleton->set_bone_meta(0, "key2", Variant());
	CHECK_MESSAGE(!skeleton->has_bone_meta(0, "key1"), "Bone meta key1 should be deleted.");
	CHECK_MESSAGE(!skeleton->has_bone_meta(0, "key2"), "Bone meta key2 should be deleted.");
	List<String> should_be_empty_keys;
	skeleton->get_bone_meta_list(0, &should_be_empty_keys);
	CHECK_MESSAGE(should_be_empty_keys.size() == 0, "Wrong number of bone meta keys.");

	// Deleting non-existing key should succeed.
	skeleton->set_bone_meta(0, "non-existing-key", Variant());
	memdelete(skeleton);
}

} // namespace TestSkeleton3D

#endif // _3D_DISABLED

// === Merged from tests/scene/test_sprite_2d.cpp ===
/*  test_sprite_2d.cpp                                                    */
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



#include "scene/main/scene_tree.h"

namespace TestSprite2D {

TEST_CASE("[SceneTree][Sprite2D] Constructor") {
	Sprite2D *sprite_2d = memnew(Sprite2D);

	CHECK(sprite_2d->get_texture().is_null());
	CHECK_EQ(sprite_2d->get_offset(), Point2(0, 0));
	CHECK(sprite_2d->is_centered());
	CHECK_FALSE(sprite_2d->is_flipped_h());
	CHECK_FALSE(sprite_2d->is_flipped_v());
	CHECK_EQ(sprite_2d->get_hframes(), 1);
	CHECK_EQ(sprite_2d->get_vframes(), 1);
	CHECK_EQ(sprite_2d->get_frame(), 0);
	CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 0));
	CHECK_FALSE(sprite_2d->is_region_enabled());

	memdelete(sprite_2d);
}

TEST_CASE("[SceneTree][Sprite2D] Frames") {
	Sprite2D *sprite_2d = memnew(Sprite2D);

	SUBCASE("Invalid range") {
		ERR_PRINT_OFF;
		sprite_2d->set_frame(30);
		sprite_2d->set_frame(-1);
		ERR_PRINT_ON;
		CHECK(sprite_2d->get_frame() == 0);
	}

	SUBCASE("Base value") {
		sprite_2d->set_hframes(1);
		sprite_2d->set_vframes(1);
		CHECK(sprite_2d->get_frame() == 0);
	}

	SUBCASE("2x2 frames") {
		sprite_2d->set_hframes(2);
		sprite_2d->set_vframes(2);

		CHECK(sprite_2d->get_hframes() == 2);
		CHECK(sprite_2d->get_vframes() == 2);

		sprite_2d->set_frame(0);
		CHECK(sprite_2d->get_frame() == 0);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 0));

		sprite_2d->set_frame(1);
		CHECK(sprite_2d->get_frame() == 1);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(1, 0));

		sprite_2d->set_frame(2);
		CHECK(sprite_2d->get_frame() == 2);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 1));

		sprite_2d->set_frame(3);
		CHECK(sprite_2d->get_frame() == 3);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(1, 1));
	}

	SUBCASE("4x4 frames") {
		sprite_2d->set_hframes(4);
		sprite_2d->set_vframes(4);

		CHECK(sprite_2d->get_hframes() == 4);
		CHECK(sprite_2d->get_vframes() == 4);

		sprite_2d->set_frame(0);
		CHECK(sprite_2d->get_frame() == 0);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 0));

		sprite_2d->set_frame(2);
		CHECK(sprite_2d->get_frame() == 2);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(2, 0));

		sprite_2d->set_frame(4);
		CHECK(sprite_2d->get_frame() == 4);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 1));

		sprite_2d->set_frame(6);
		CHECK(sprite_2d->get_frame() == 6);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(2, 1));

		sprite_2d->set_frame(8);
		CHECK(sprite_2d->get_frame() == 8);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 2));

		sprite_2d->set_frame(10);
		CHECK(sprite_2d->get_frame() == 10);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(2, 2));

		sprite_2d->set_frame(12);
		CHECK(sprite_2d->get_frame() == 12);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 3));

		sprite_2d->set_frame(14);
		CHECK(sprite_2d->get_frame() == 14);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(2, 3));
	}

	SUBCASE("8x4 frames") {
		sprite_2d->set_hframes(8);
		sprite_2d->set_vframes(4);

		CHECK(sprite_2d->get_hframes() == 8);
		CHECK(sprite_2d->get_vframes() == 4);

		sprite_2d->set_frame(0);
		CHECK(sprite_2d->get_frame() == 0);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 0));

		sprite_2d->set_frame(4);
		CHECK(sprite_2d->get_frame() == 4);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(4, 0));

		sprite_2d->set_frame(8);
		CHECK(sprite_2d->get_frame() == 8);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 1));

		sprite_2d->set_frame(16);
		CHECK(sprite_2d->get_frame() == 16);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 2));

		sprite_2d->set_frame(31);
		CHECK(sprite_2d->get_frame() == 31);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(7, 3));
	}

	SUBCASE("100x100 frames") {
		sprite_2d->set_hframes(100);
		sprite_2d->set_vframes(100);

		CHECK(sprite_2d->get_hframes() == 100);
		CHECK(sprite_2d->get_vframes() == 100);

		sprite_2d->set_frame(0);
		CHECK(sprite_2d->get_frame() == 0);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(0, 0));

		sprite_2d->set_frame(60);
		CHECK(sprite_2d->get_frame() == 60);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(60, 0));

		sprite_2d->set_frame(120);
		CHECK(sprite_2d->get_frame() == 120);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(20, 1));

		sprite_2d->set_frame(240);
		CHECK(sprite_2d->get_frame() == 240);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(40, 2));

		sprite_2d->set_frame(360);
		CHECK(sprite_2d->get_frame() == 360);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(60, 3));

		sprite_2d->set_frame(2048);
		CHECK(sprite_2d->get_frame() == 2048);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(48, 20));

		sprite_2d->set_frame(8192);
		CHECK(sprite_2d->get_frame() == 8192);
		CHECK_EQ(sprite_2d->get_frame_coords(), Vector2i(92, 81));
	}

	memdelete(sprite_2d);
}

TEST_CASE("[SceneTree][Sprite2D] Flipping") {
	Sprite2D *sprite_2d = memnew(Sprite2D);

	SUBCASE("Both False") {
		CHECK_FALSE(sprite_2d->is_flipped_h());
		CHECK_FALSE(sprite_2d->is_flipped_v());
	}

	SUBCASE("Both True") {
		sprite_2d->set_flip_h(true);
		sprite_2d->set_flip_v(true);

		CHECK(sprite_2d->is_flipped_h());
		CHECK(sprite_2d->is_flipped_v());
	}

	SUBCASE("True False") {
		sprite_2d->set_flip_h(true);
		sprite_2d->set_flip_v(false);

		CHECK(sprite_2d->is_flipped_h());
		CHECK_FALSE(sprite_2d->is_flipped_v());
	}

	SUBCASE("False True") {
		sprite_2d->set_flip_h(false);
		sprite_2d->set_flip_v(true);

		CHECK_FALSE(sprite_2d->is_flipped_h());
		CHECK(sprite_2d->is_flipped_v());
	}

	memdelete(sprite_2d);
}

TEST_CASE("[SceneTree][Sprite2D] Offset") {
	Sprite2D *sprite_2d = memnew(Sprite2D);

	sprite_2d->set_offset(Point2(0.0, 0.0));
	CHECK(sprite_2d->get_offset() == Point2(0.0, 0.0));

	sprite_2d->set_offset(Point2(8.0, 8.0));
	CHECK(sprite_2d->get_offset() == Point2(8.0, 8.0));

	sprite_2d->set_offset(Point2(25.0, 50.0));
	CHECK(sprite_2d->get_offset() == Point2(25.0, 50.0));

	sprite_2d->set_offset(Point2(500.0, 250.0));
	CHECK(sprite_2d->get_offset() == Point2(500.0, 250.0));

	sprite_2d->set_offset(Point2(-8.0, -8.0));
	CHECK(sprite_2d->get_offset() == Point2(-8.0, -8.0));

	sprite_2d->set_offset(Point2(-25.0, -50.0));
	CHECK(sprite_2d->get_offset() == Point2(-25.0, -50.0));

	sprite_2d->set_offset(Point2(-500.0, -250.0));
	CHECK(sprite_2d->get_offset() == Point2(-500.0, -250.0));

	memdelete(sprite_2d);
}

} // namespace TestSprite2D

// === Merged from tests/scene/test_sprite_frames.cpp ===
/*  test_sprite_frames.cpp                                                */
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



#include "scene/main/scene_tree.h"

namespace TestSpriteFrames {

const String test_animation_name = "GodotTest";

TEST_CASE("[SpriteFrames] Constructor methods") {
	const SpriteFrames frames;
	CHECK_MESSAGE(
			frames.get_animation_names().size() == 1,
			"Should be initialized with 1 entry.");
	CHECK_MESSAGE(
			frames.get_animation_names().get(0) == "default",
			"Should be initialized with default entry.");
}

TEST_CASE("[SpriteFrames] Animation addition, list getter, renaming, removal, and retrieval") {
	SpriteFrames frames;
	Vector<String> test_names = { "default", "2", "1", "3" };

	// "default" is there already
	frames.add_animation("2");
	frames.add_animation("1");
	frames.add_animation("3");

	for (int i = 0; i < test_names.size(); i++) {
		CHECK_MESSAGE(
				frames.has_animation(test_names[i]),
				"Add animation properly worked for each value");
	}

	CHECK_MESSAGE(
			!frames.has_animation("999"),
			"Return false when checking for animation that does not exist");

	List<String> sname_list;
	frames.get_animation_list(&sname_list);

	CHECK_MESSAGE(
			sname_list.size() == test_names.size(),
			"String List getter returned list of expected size");

	int idx = 0;
	for (List<String>::ConstIterator itr = sname_list.begin(); itr != sname_list.end(); ++itr, ++idx) {
		CHECK_MESSAGE(
				*itr == String(test_names[idx]),
				"String List getter returned expected values");
	}

	// get_animation_names() sorts the results.
	Vector<String> string_vector = frames.get_animation_names();
	test_names.sort();

	for (int i = 0; i < test_names.size(); i++) {
		CHECK_MESSAGE(
				string_vector[i] == test_names[i],
				"String Vector getter returned expected values");
	}

	// These error handling cases should not crash.
	ERR_PRINT_OFF;
	frames.rename_animation("This does not exist", "0");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			!frames.has_animation("0"),
			"Correctly handles rename error when entry does not exist");

	// These error handling cases should not crash.
	ERR_PRINT_OFF;
	frames.rename_animation("3", "1");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			frames.has_animation("3"),
			"Correctly handles rename error when entry exists, but new name already exists");

	ERR_PRINT_OFF;
	frames.add_animation("1");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			frames.get_animation_names().size() == 4,
			"Correctly does not add when entry already exists");

	frames.rename_animation("3", "9");

	CHECK_MESSAGE(
			frames.has_animation("9"),
			"Animation renamed correctly");

	frames.remove_animation("9");

	CHECK_MESSAGE(
			!frames.has_animation("9"),
			"Animation removed correctly");

	frames.clear_all();

	CHECK_MESSAGE(
			frames.get_animation_names().size() == 1,
			"Clear all removed all animations and re-added the default animation entry");
}

TEST_CASE("[SpriteFrames] Animation Speed getter and setter") {
	SpriteFrames frames;

	frames.add_animation(test_animation_name);

	CHECK_MESSAGE(
			frames.get_animation_speed(test_animation_name) == 5.0,
			"Sets new animation to default speed");

	frames.set_animation_speed(test_animation_name, 123.0004);

	CHECK_MESSAGE(
			frames.get_animation_speed(test_animation_name) == 123.0004,
			"Sets animation to positive double");

	// These error handling cases should not crash.
	ERR_PRINT_OFF;
	frames.get_animation_speed("This does not exist");
	frames.set_animation_speed("This does not exist", 100);
	frames.set_animation_speed(test_animation_name, -999.999);
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			frames.get_animation_speed(test_animation_name) == 123.0004,
			"Prevents speed of animation being set to a negative value");

	frames.set_animation_speed(test_animation_name, 0.0);

	CHECK_MESSAGE(
			frames.get_animation_speed(test_animation_name) == 0.0,
			"Sets animation to zero");
}

TEST_CASE("[SpriteFrames] Animation Loop Mode getter and setter") {
	SpriteFrames frames;

	frames.add_animation(test_animation_name);

	CHECK_MESSAGE(
			frames.get_animation_loop_mode(test_animation_name) == SpriteFrames::LOOP_LINEAR,
			"Sets new animation to default loop mode value (linear).");

	frames.set_animation_loop_mode(test_animation_name, SpriteFrames::LOOP_LINEAR);

	CHECK_MESSAGE(
			frames.get_animation_loop_mode(test_animation_name) == SpriteFrames::LOOP_LINEAR,
			"Sets animation loop mode to linear");

	frames.set_animation_loop_mode(test_animation_name, SpriteFrames::LOOP_PINGPONG);

	CHECK_MESSAGE(
			frames.get_animation_loop_mode(test_animation_name) == SpriteFrames::LOOP_PINGPONG,
			"Sets animation loop mode to ping pong");

	frames.set_animation_loop_mode(test_animation_name, SpriteFrames::LOOP_NONE);

	CHECK_MESSAGE(
			frames.get_animation_loop_mode(test_animation_name) == SpriteFrames::LOOP_NONE,
			"Sets animation loop mode to none");

	// These error handling cases should not crash.
	ERR_PRINT_OFF;
	frames.get_animation_loop_mode("This does not exist");
	frames.set_animation_loop_mode("This does not exist", SpriteFrames::LOOP_NONE);
	ERR_PRINT_ON;
}

// TODO
TEST_CASE("[SpriteFrames] Frame addition, removal, and retrieval") {
	Ref<Texture2D> dummy_frame1;
	dummy_frame1.instantiate();

	SpriteFrames frames;
	frames.add_animation(test_animation_name);
	frames.add_animation("1");
	frames.add_animation("2");

	CHECK_MESSAGE(
			frames.get_frame_count(test_animation_name) == 0,
			"Animation has a default frame count of 0");

	frames.add_frame(test_animation_name, dummy_frame1, 1.0, 0);
	frames.add_frame(test_animation_name, dummy_frame1, 1.0, 1);
	frames.add_frame(test_animation_name, dummy_frame1, 1.0, 2);

	CHECK_MESSAGE(
			frames.get_frame_count(test_animation_name) == 3,
			"Adds multiple frames");

	frames.remove_frame(test_animation_name, 1);
	frames.remove_frame(test_animation_name, 0);

	CHECK_MESSAGE(
			frames.get_frame_count(test_animation_name) == 1,
			"Removes multiple frames");

	// These error handling cases should not crash.
	ERR_PRINT_OFF;
	frames.add_frame("does not exist", dummy_frame1, 1.0, 0);
	frames.remove_frame(test_animation_name, -99);
	frames.remove_frame("does not exist", 0);
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			frames.get_frame_count(test_animation_name) == 1,
			"Handles bad values when adding or removing frames.");

	frames.clear(test_animation_name);

	CHECK_MESSAGE(
			frames.get_frame_count(test_animation_name) == 0,
			"Clears frames.");
}

} // namespace TestSpriteFrames

// === Merged from tests/scene/test_timer.cpp ===
/*  test_timer.cpp                                                        */
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



#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

namespace TestTimer {

TEST_CASE("[SceneTree][Timer] Check Timer Setters and Getters") {
	Timer *test_timer = memnew(Timer);

	SUBCASE("[Timer] Timer set and get wait time") {
		// check default
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 1.0));

		test_timer->set_wait_time(50.0);
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 50.0));

		test_timer->set_wait_time(42.0);
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 42.0));

		// wait time remains unchanged if we attempt to set it negative or zero
		ERR_PRINT_OFF;
		test_timer->set_wait_time(-22.0);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 42.0));

		ERR_PRINT_OFF;
		test_timer->set_wait_time(0.0);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 42.0));
	}

	SUBCASE("[Timer] Timer set and get one shot") {
		// check default
		CHECK(test_timer->is_one_shot() == false);

		test_timer->set_one_shot(true);
		CHECK(test_timer->is_one_shot() == true);

		test_timer->set_one_shot(false);
		CHECK(test_timer->is_one_shot() == false);
	}

	SUBCASE("[Timer] Timer set and get autostart") {
		// check default
		CHECK(test_timer->has_autostart() == false);

		test_timer->set_autostart(true);
		CHECK(test_timer->has_autostart() == true);

		test_timer->set_autostart(false);
		CHECK(test_timer->has_autostart() == false);
	}

	SUBCASE("[Timer] Timer start and stop") {
		test_timer->set_autostart(false);
	}

	SUBCASE("[Timer] Timer set and get paused") {
		// check default
		CHECK(test_timer->is_paused() == false);

		test_timer->set_paused(true);
		CHECK(test_timer->is_paused() == true);

		test_timer->set_paused(false);
		CHECK(test_timer->is_paused() == false);
	}

	memdelete(test_timer);
}

TEST_CASE("[SceneTree][Timer] Check Timer Start and Stop") {
	Timer *test_timer = memnew(Timer);

	SUBCASE("[Timer] Timer start and stop") {
		SceneTree::get_root()->add_child(test_timer);

		test_timer->start(5.0);

		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 5.0));
		CHECK(Math::is_equal_approx(test_timer->get_time_left(), 5.0));

		test_timer->start(-2.0);

		// the wait time and time left remains unchanged when started with a negative start time
		CHECK(Math::is_equal_approx(test_timer->get_wait_time(), 5.0));
		CHECK(Math::is_equal_approx(test_timer->get_time_left(), 5.0));

		test_timer->stop();
		CHECK(test_timer->is_processing() == false);
		CHECK(test_timer->has_autostart() == false);
	}

	memdelete(test_timer);
}

TEST_CASE("[SceneTree][Timer] Check Timer process callback") {
	Timer *test_timer = memnew(Timer);

	SUBCASE("[Timer] Timer process callback") {
		// check default
		CHECK(test_timer->get_timer_process_callback() == Timer::TimerProcessCallback::TIMER_PROCESS_IDLE);

		test_timer->set_timer_process_callback(Timer::TimerProcessCallback::TIMER_PROCESS_PHYSICS);
		CHECK(test_timer->get_timer_process_callback() == Timer::TimerProcessCallback::TIMER_PROCESS_PHYSICS);

		test_timer->set_timer_process_callback(Timer::TimerProcessCallback::TIMER_PROCESS_IDLE);
		CHECK(test_timer->get_timer_process_callback() == Timer::TimerProcessCallback::TIMER_PROCESS_IDLE);
	}

	memdelete(test_timer);
}

TEST_CASE("[SceneTree][Timer] Check Timer timeout signal") {
	Timer *test_timer = memnew(Timer);
	SceneTree::get_root()->add_child(test_timer);

	test_timer->set_process(true);
	test_timer->set_physics_process(true);

	SUBCASE("[Timer] Timer process timeout signal must be emitted") {
		SIGNAL_WATCH(test_timer, SNAME("timeout"));
		test_timer->start(0.1);

		SceneTree::process(0.2);

		Array signal_args = { {} };
		SIGNAL_CHECK(SNAME("timeout"), signal_args);

		SIGNAL_UNWATCH(test_timer, SNAME("timeout"));
	}

	SUBCASE("[Timer] Timer process timeout signal must not be emitted") {
		SIGNAL_WATCH(test_timer, SNAME("timeout"));
		test_timer->start(0.1);

		SceneTree::process(0.05);

		SIGNAL_CHECK_FALSE(SNAME("timeout"));

		SIGNAL_UNWATCH(test_timer, SNAME("timeout"));
	}

	test_timer->set_timer_process_callback(Timer::TimerProcessCallback::TIMER_PROCESS_PHYSICS);

	SUBCASE("[Timer] Timer physics process timeout signal must be emitted") {
		SIGNAL_WATCH(test_timer, SNAME("timeout"));
		test_timer->start(0.1);

		SceneTree::physics_process(0.2);

		Array signal_args = { {} };
		SIGNAL_CHECK(SNAME("timeout"), signal_args);

		SIGNAL_UNWATCH(test_timer, SNAME("timeout"));
	}

	SUBCASE("[Timer] Timer physics process timeout signal must not be emitted") {
		SIGNAL_WATCH(test_timer, SNAME("timeout"));
		test_timer->start(0.1);

		SceneTree::physics_process(0.05);

		SIGNAL_CHECK_FALSE(SNAME("timeout"));

		SIGNAL_UNWATCH(test_timer, SNAME("timeout"));
	}

	memdelete(test_timer);
}

} // namespace TestTimer

// === Merged from tests/scene/test_viewport.cpp ===
/*  test_viewport.cpp                                                     */
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



#include "scene/main/scene_tree.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

namespace TestViewport {

class NotificationControlViewport : public Control {

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_MOUSE_ENTER: {
				if (mouse_over) {
					invalid_order = true;
				}
				mouse_over = true;
			} break;

			case NOTIFICATION_MOUSE_EXIT: {
				if (!mouse_over) {
					invalid_order = true;
				}
				mouse_over = false;
			} break;

			case NOTIFICATION_MOUSE_ENTER_SELF: {
				if (mouse_over_self) {
					invalid_order = true;
				}
				mouse_over_self = true;
			} break;

			case NOTIFICATION_MOUSE_EXIT_SELF: {
				if (!mouse_over_self) {
					invalid_order = true;
				}
				mouse_over_self = false;
			} break;
		}
	}

public:
	bool mouse_over = false;
	bool mouse_over_self = false;
	bool invalid_order = false;
};

// `NotificationControlViewport`-derived class that additionally
// - allows start Dragging
// - stores mouse information of last event
class DragStart : public NotificationControlViewport {

public:
	MouseButton last_mouse_button;
	Point2i last_mouse_move_position;
	String drag_data_name = SNAME("Drag Data");

	virtual Variant get_drag_data(const Point2 &p_point) override {
		return drag_data_name;
	}

	virtual void gui_input(const Ref<InputEvent> &p_event) override {
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid()) {
			last_mouse_button = mb->get_button_index();
			return;
		}

		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid()) {
			last_mouse_move_position = mm->get_position();
			return;
		}
	}
};

// `NotificationControlViewport`-derived class that acts as a Drag and Drop target.
class DragTarget : public NotificationControlViewport {

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_DRAG_BEGIN: {
				during_drag = true;
			} break;

			case NOTIFICATION_DRAG_END: {
				during_drag = false;
			} break;
		}
	}

public:
	Variant drag_data;
	bool valid_drop = false;
	bool during_drag = false;
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override {
		String string_data = p_data;
		// Verify drag data is compatible.
		if (string_data != SNAME("Drag Data")) {
			return false;
		}
		// Only the left half is droppable area.
		if (p_point.x * 2 > get_size().x) {
			return false;
		}
		return true;
	}

	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override {
		drag_data = p_data;
		valid_drop = true;
	}
};

TEST_CASE("[SceneTree][Viewport] Controls and InputEvent handling") {
	DragStart *node_a = memnew(DragStart);
	NotificationControlViewport *node_b = memnew(NotificationControlViewport);
	Node2D *node_c = memnew(Node2D);
	DragTarget *node_d = memnew(DragTarget);
	NotificationControlViewport *node_e = memnew(NotificationControlViewport);
	Node *node_f = memnew(Node);
	NotificationControlViewport *node_g = memnew(NotificationControlViewport);
	NotificationControlViewport *node_h = memnew(NotificationControlViewport);
	NotificationControlViewport *node_i = memnew(NotificationControlViewport);
	NotificationControlViewport *node_j = memnew(NotificationControlViewport);

	node_a->set_name(SNAME("NodeA"));
	node_b->set_name(SNAME("NodeB"));
	node_c->set_name(SNAME("NodeC"));
	node_d->set_name(SNAME("NodeD"));
	node_e->set_name(SNAME("NodeE"));
	node_f->set_name(SNAME("NodeF"));
	node_g->set_name(SNAME("NodeG"));
	node_h->set_name(SNAME("NodeH"));
	node_i->set_name(SNAME("NodeI"));
	node_j->set_name(SNAME("NodeJ"));

	node_a->set_position(Point2i(0, 0));
	node_b->set_position(Point2i(10, 10));
	node_c->set_position(Point2i(0, 0));
	node_d->set_position(Point2i(10, 10));
	node_e->set_position(Point2i(10, 100));
	node_g->set_position(Point2i(10, 100));
	node_h->set_position(Point2i(10, 120));
	node_i->set_position(Point2i(2, 0));
	node_j->set_position(Point2i(2, 0));
	node_a->set_size(Point2i(30, 30));
	node_b->set_size(Point2i(30, 30));
	node_d->set_size(Point2i(30, 30));
	node_e->set_size(Point2i(10, 10));
	node_g->set_size(Point2i(10, 10));
	node_h->set_size(Point2i(10, 10));
	node_i->set_size(Point2i(10, 10));
	node_j->set_size(Point2i(10, 10));
	node_a->set_focus_mode(Control::FOCUS_CLICK);
	node_b->set_focus_mode(Control::FOCUS_CLICK);
	node_d->set_focus_mode(Control::FOCUS_CLICK);
	node_e->set_focus_mode(Control::FOCUS_CLICK);
	node_g->set_focus_mode(Control::FOCUS_CLICK);
	node_h->set_focus_mode(Control::FOCUS_CLICK);
	node_i->set_focus_mode(Control::FOCUS_CLICK);
	node_j->set_focus_mode(Control::FOCUS_CLICK);
	Window *root = SceneTree::get_root();
	DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();

	// Scene tree:
	// - root
	//   - a (Control)
	//   - b (Control)
	//     - c (Node2D)
	//       - d (Control)
	//   - e (Control)
	//     - f (Node)
	//       - g (Control)
	//   - h (Control)
	//     - i (Control)
	//       - j (Control)
	root->add_child(node_a);
	root->add_child(node_b);
	node_b->add_child(node_c);
	node_c->add_child(node_d);
	root->add_child(node_e);
	node_e->add_child(node_f);
	node_f->add_child(node_g);
	root->add_child(node_h);
	node_h->add_child(node_i);
	node_i->add_child(node_j);

	Point2i on_a = Point2i(5, 5);
	Point2i on_b = Point2i(15, 15);
	Point2i on_d = Point2i(25, 25);
	Point2i on_e = Point2i(15, 105);
	Point2i on_g = Point2i(15, 105);
	Point2i on_i = Point2i(13, 125);
	Point2i on_j = Point2i(15, 125);
	Point2i on_background = Point2i(500, 500);
	Point2i on_outside = Point2i(-1, -1);

	// Unit tests for Viewport::gui_find_control and Viewport::_gui_find_control_at_pos
	SUBCASE("[VIEWPORT][GuiFindControl] Finding Controls at a Viewport-position") {
		// FIXME: It is extremely difficult to create a situation where the Control has a zero determinant.
		// Leaving that if-branch untested.

		SUBCASE("[VIEWPORT][GuiFindControl] Basic position tests") {
			CHECK(root->gui_find_control(on_a) == node_a);
			CHECK(root->gui_find_control(on_b) == node_b);
			CHECK(root->gui_find_control(on_d) == node_d);
			CHECK(root->gui_find_control(on_e) == node_g); // Node F makes G a Root Control at the same position as E
			CHECK(root->gui_find_control(on_g) == node_g);
			CHECK_FALSE(root->gui_find_control(on_background));
		}

		SUBCASE("[VIEWPORT][GuiFindControl] Invisible nodes are not considered as results.") {
			// Non-Root Control
			node_d->hide();
			CHECK(root->gui_find_control(on_d) == node_b);
			// Root Control
			node_b->hide();
			CHECK(root->gui_find_control(on_b) == node_a);
		}

		SUBCASE("[VIEWPORT][GuiFindControl] Root Control with CanvasItem as parent is affected by parent's transform.") {
			node_b->remove_child(node_c);
			node_c->set_position(Point2i(50, 50));
			root->add_child(node_c);
			CHECK(root->gui_find_control(Point2i(65, 65)) == node_d);
		}

		SUBCASE("[VIEWPORT][GuiFindControl] Control Contents Clipping clips accessible position of children.") {
			CHECK_FALSE(node_b->is_clipping_contents());
			CHECK(root->gui_find_control(on_d + Point2i(20, 20)) == node_d);
			node_b->set_clip_contents(true);
			CHECK(root->gui_find_control(on_d) == node_d);
			CHECK_FALSE(root->gui_find_control(on_d + Point2i(20, 20)));
		}

		SUBCASE("[VIEWPORT][GuiFindControl] Top Level Control as descendant of CanvasItem isn't affected by parent's transform.") {
			CHECK(root->gui_find_control(on_d + Point2i(20, 20)) == node_d);
			node_d->set_as_top_level(true);
			CHECK_FALSE(root->gui_find_control(on_d + Point2i(20, 20)));
			CHECK(root->gui_find_control(on_b) == node_d);
		}
	}

	SUBCASE("[Viewport][GuiInputEvent] nullptr as argument doesn't lead to a crash.") {
		ERR_PRINT_OFF;
		root->push_input(Ref<InputEvent>());
		ERR_PRINT_ON;
	}

	// Unit tests for Viewport::_gui_input_event (Mouse Buttons)
	SUBCASE("[Viewport][GuiInputEvent] Mouse Button Down/Up.") {
		SUBCASE("[Viewport][GuiInputEvent] Mouse Button Control Focus Change.") {
			SUBCASE("[Viewport][GuiInputEvent] Grab Focus while no Control has focus.") {
				CHECK_FALSE(root->gui_get_focus_owner());

				// Click on A
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK(node_a->has_focus());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			}

			SUBCASE("[Viewport][GuiInputEvent] Grab Focus from other Control.") {
				node_a->grab_focus();
				CHECK(node_a->has_focus());

				// Click on D
				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK(node_d->has_focus());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			}

			SUBCASE("[Viewport][GuiInputEvent] Non-CanvasItem breaks Transform hierarchy.") {
				CHECK_FALSE(root->gui_get_focus_owner());

				// Click on G absolute coordinates
				SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(15, 105), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK(node_g->has_focus());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(15, 105), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			}

			SUBCASE("[Viewport][GuiInputEvent] No Focus change when clicking in background.") {
				CHECK_FALSE(root->gui_get_focus_owner());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_get_focus_owner());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

				node_a->grab_focus();
				CHECK(node_a->has_focus());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_a->has_focus());
			}

			SUBCASE("[Viewport][GuiInputEvent] Mouse Button No Focus Steal while other Mouse Button is pressed.") {
				CHECK_FALSE(root->gui_get_focus_owner());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK(node_a->has_focus());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_b, MouseButton::RIGHT, MouseButtonMask::LEFT | MouseButtonMask::RIGHT, Key::NONE);
				CHECK(node_a->has_focus());

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_b, MouseButton::RIGHT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_a->has_focus());
			}

			SUBCASE("[Viewport][GuiInputEvent] Allow Focus Steal with LMB while other Mouse Button is held down and was initially pressed without being over a Control.") {
				// TODO: Not sure, if this is intended behavior, but this is an edge case.
				CHECK_FALSE(root->gui_get_focus_owner());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_background, MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
				CHECK_FALSE(root->gui_get_focus_owner());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT | MouseButtonMask::RIGHT, Key::NONE);
				CHECK(node_a->has_focus());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::RIGHT, Key::NONE);
				CHECK(node_a->has_focus());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::LEFT | MouseButtonMask::RIGHT, Key::NONE);
				CHECK(node_b->has_focus());

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::RIGHT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_b->has_focus());
			}

			SUBCASE("[Viewport][GuiInputEvent] Ignore Focus from Mouse Buttons when mouse-filter is set to ignore.") {
				node_d->grab_focus();
				node_d->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
				CHECK(node_d->has_focus());

				// Click on overlapping area B&D.
				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK(node_b->has_focus());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			}

			SUBCASE("[Viewport][GuiInputEvent] RMB doesn't grab focus.") {
				node_a->grab_focus();
				CHECK(node_a->has_focus());

				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_a->has_focus());
			}

			SUBCASE("[Viewport][GuiInputEvent] LMB on unfocusable Control doesn't grab focus.") {
				CHECK_FALSE(node_g->has_focus());
				node_g->set_focus_mode(Control::FOCUS_NONE);

				SEND_GUI_MOUSE_BUTTON_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(node_g->has_focus());

				// Now verify the opposite with FOCUS_CLICK
				node_g->set_focus_mode(Control::FOCUS_CLICK);
				SEND_GUI_MOUSE_BUTTON_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_g->has_focus());
				node_g->set_focus_mode(Control::FOCUS_CLICK);
			}

			SUBCASE("[Viewport][GuiInputEvent] Signal 'gui_focus_changed' is only emitted if a previously unfocused Control grabs focus.") {
				SIGNAL_WATCH(root, SNAME("gui_focus_changed"));
				Array signal_args = { { node_a } };

				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				SIGNAL_CHECK(SNAME("gui_focus_changed"), signal_args);

				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_a->has_focus());
				SIGNAL_CHECK_FALSE(SNAME("gui_focus_changed"));

				SIGNAL_UNWATCH(root, SNAME("gui_focus_changed"));
			}

			SUBCASE("[Viewport][GuiInputEvent] Focus Propagation to parent items.") {
				SUBCASE("[Viewport][GuiInputEvent] Unfocusable Control with MOUSE_FILTER_PASS propagates focus to parent CanvasItem.") {
					node_d->set_focus_mode(Control::FOCUS_NONE);
					node_d->set_mouse_filter(Control::MOUSE_FILTER_PASS);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_d + Point2i(20, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					CHECK(node_b->has_focus());
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d + Point2i(20, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

					// Verify break condition for Root Control.
					node_a->set_focus_mode(Control::FOCUS_NONE);
					node_a->set_mouse_filter(Control::MOUSE_FILTER_PASS);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK(node_b->has_focus());
				}

				SUBCASE("[Viewport][GuiInputEvent] Top Level CanvasItem stops focus propagation.") {
					node_d->set_focus_mode(Control::FOCUS_NONE);
					node_d->set_mouse_filter(Control::MOUSE_FILTER_PASS);
					node_c->set_as_top_level(true);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK_FALSE(root->gui_get_focus_owner());

					node_d->set_focus_mode(Control::FOCUS_CLICK);
					SEND_GUI_MOUSE_BUTTON_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_b, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK(node_d->has_focus());
				}
			}
		}

		SUBCASE("[Viewport][GuiInputEvent] Process-Mode affects, if GUI Mouse Button Events are processed.") {
			node_a->last_mouse_button = MouseButton::NONE;
			node_a->set_process_mode(Node::PROCESS_MODE_DISABLED);
			SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->last_mouse_button == MouseButton::NONE);

			// Now verify that with allowed processing the event is processed.
			node_a->set_process_mode(Node::PROCESS_MODE_ALWAYS);
			SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->last_mouse_button == MouseButton::LEFT);
		}
	}

	// Unit tests for Viewport::_gui_input_event (Mouse Motion)
	SUBCASE("[Viewport][GuiInputEvent] Mouse Motion") {
		// FIXME: Tooltips are not yet tested. They likely require an internal clock.

		SUBCASE("[Viewport][GuiInputEvent] Mouse Motion changes the Control that it is over.") {
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_a->mouse_over);
			CHECK_FALSE(node_a->mouse_over_self);

			// Move over Control.
			SEND_GUI_MOUSE_MOTION_EVENT(on_a, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->mouse_over);
			CHECK(node_a->mouse_over_self);

			// No change.
			SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(1, 1), MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->mouse_over);
			CHECK(node_a->mouse_over_self);

			// Move over other Control.
			SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_a->mouse_over);
			CHECK_FALSE(node_a->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_d->mouse_over);
			CHECK_FALSE(node_d->mouse_over_self);

			CHECK_FALSE(node_a->invalid_order);
			CHECK_FALSE(node_d->invalid_order);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse behavior recursive disables mouse motion events.") {
			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Enabled when parent is set to inherit.
			node_h->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_INHERITED);
			node_i->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_INHERITED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);

			// Enabled when parent is set to enabled.
			node_h->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_ENABLED);
			node_i->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_INHERITED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);

			// Disabled when parent is set to disabled.
			node_h->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_DISABLED);
			node_i->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_INHERITED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);

			// Enabled when set to enabled and parent is set to disabled.
			node_h->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_DISABLED);
			node_i->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_ENABLED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);

			// Disabled when it is set to disabled.
			node_h->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_ENABLED);
			node_i->set_mouse_behavior_recursive(Control::MOUSE_BEHAVIOR_DISABLED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification propagation.") {
			node_d->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_g->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK_FALSE(node_d->mouse_over);
			CHECK_FALSE(node_d->mouse_over_self);

			// Move to Control node_d. node_b receives mouse over since it is only separated by a CanvasItem.
			SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK_FALSE(node_d->mouse_over);
			CHECK_FALSE(node_d->mouse_over_self);

			CHECK_FALSE(node_e->mouse_over);
			CHECK_FALSE(node_e->mouse_over_self);
			CHECK_FALSE(node_g->mouse_over);
			CHECK_FALSE(node_g->mouse_over_self);

			// Move to Control node_g. node_g receives mouse over but node_e does not since it is separated by a non-CanvasItem.
			SEND_GUI_MOUSE_MOTION_EVENT(on_g, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_e->mouse_over);
			CHECK_FALSE(node_e->mouse_over_self);
			CHECK(node_g->mouse_over);
			CHECK(node_g->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_e->mouse_over);
			CHECK_FALSE(node_e->mouse_over_self);
			CHECK_FALSE(node_g->mouse_over);
			CHECK_FALSE(node_g->mouse_over_self);

			CHECK_FALSE(node_b->invalid_order);
			CHECK_FALSE(node_d->invalid_order);
			CHECK_FALSE(node_e->invalid_order);
			CHECK_FALSE(node_g->invalid_order);

			node_d->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_g->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification propagation when moving into child.") {
			SIGNAL_WATCH(node_i, SceneString(mouse_entered));
			SIGNAL_WATCH(node_i, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			// Move to Control node_i.
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Move to child Control node_j. node_i should not receive any new Mouse Enter signals.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Move to parent Control node_i. node_i should not receive any new Mouse Enter signals.
			SEND_GUI_MOUSE_MOTION_EVENT(on_i, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK(SceneString(mouse_exited), signal_args);

			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_i, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_i, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification propagation with top level.") {
			node_c->set_as_top_level(true);
			node_i->set_as_top_level(true);
			node_c->set_position(node_b->get_global_position());
			node_i->set_position(node_h->get_global_position());
			node_d->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK_FALSE(node_d->mouse_over);
			CHECK_FALSE(node_d->mouse_over_self);

			// Move to Control node_d. node_b does not receive mouse over since node_c is top level.
			SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK_FALSE(node_d->mouse_over);
			CHECK_FALSE(node_d->mouse_over_self);

			CHECK_FALSE(node_g->mouse_over);
			CHECK_FALSE(node_g->mouse_over_self);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);

			// Move to Control node_j. node_h does not receive mouse over since node_i is top level.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			CHECK_FALSE(node_b->invalid_order);
			CHECK_FALSE(node_d->invalid_order);
			CHECK_FALSE(node_e->invalid_order);
			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_c->set_as_top_level(false);
			node_i->set_as_top_level(false);
			node_c->set_position(Point2i(0, 0));
			node_i->set_position(Point2i(0, 0));
			node_d->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification propagation with mouse filter stop.") {
			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			// Move to Control node_j. node_h does not receive mouse over since node_i is MOUSE_FILTER_STOP.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification propagation with mouse filter ignore.") {
			node_i->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			// Move to Control node_j. node_i does not receive mouse over since node_i is MOUSE_FILTER_IGNORE.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);

			// Move to background.
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification when changing top level.") {
			SIGNAL_WATCH(node_i, SceneString(mouse_entered));
			SIGNAL_WATCH(node_i, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_d->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to Control node_d.
			SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Change node_c to be top level. node_b should receive Mouse Exit.
			node_c->set_as_top_level(true);
			CHECK_FALSE(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Change node_c to be not top level. node_b should receive Mouse Enter.
			node_c->set_as_top_level(false);
			CHECK(node_b->mouse_over);
			CHECK_FALSE(node_b->mouse_over_self);
			CHECK(node_d->mouse_over);
			CHECK(node_d->mouse_over_self);

			// Move to Control node_j.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_i to top level. node_h should receive Mouse Exit. node_i should not receive any new signals.
			node_i->set_as_top_level(true);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_i to not top level. node_h should receive Mouse Enter. node_i should not receive any new signals.
			node_i->set_as_top_level(false);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			CHECK_FALSE(node_b->invalid_order);
			CHECK_FALSE(node_d->invalid_order);
			CHECK_FALSE(node_e->invalid_order);
			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_d->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_i, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_i, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification when changing the mouse filter to stop.") {
			SIGNAL_WATCH(node_i, SceneString(mouse_entered));
			SIGNAL_WATCH(node_i, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to Control node_j.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_i to MOUSE_FILTER_STOP. node_h should receive Mouse Exit. node_i should not receive any new signals.
			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			CHECK_FALSE(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_i to MOUSE_FILTER_PASS. node_h should receive Mouse Enter. node_i should not receive any new signals.
			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_i, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_i, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification when changing the mouse filter to ignore.") {
			SIGNAL_WATCH(node_i, SceneString(mouse_entered));
			SIGNAL_WATCH(node_i, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to Control node_j.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_i to MOUSE_FILTER_IGNORE. node_i should receive Mouse Exit.
			node_i->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK(SceneString(mouse_exited), signal_args);

			// Change node_i to MOUSE_FILTER_PASS. node_i should receive Mouse Enter.
			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_j to MOUSE_FILTER_IGNORE. After updating the mouse motion, node_i should now have mouse_over_self.
			node_j->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Change node_j to MOUSE_FILTER_PASS. After updating the mouse motion, node_j should now have mouse_over_self.
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_i, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_i, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification when removing the hovered Control.") {
			SIGNAL_WATCH(node_h, SceneString(mouse_entered));
			SIGNAL_WATCH(node_h, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to Control node_j.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Remove node_i from the tree. node_i and node_j should receive Mouse Exit. node_h should not receive any new signals.
			node_h->remove_child(node_i);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Add node_i to the tree and update the mouse. node_i and node_j should receive Mouse Enter. node_h should not receive any new signals.
			node_h->add_child(node_i);
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_h, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_h, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Mouse Enter/Exit notification when hiding the hovered Control.") {
			SIGNAL_WATCH(node_h, SceneString(mouse_entered));
			SIGNAL_WATCH(node_h, SceneString(mouse_exited));
			Array signal_args = { {} };

			node_i->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_PASS);

			// Move to Control node_j.
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Hide node_i. node_i and node_j should receive Mouse Exit. node_h should not receive any new signals.
			node_i->hide();
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK_FALSE(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK_FALSE(node_j->mouse_over);
			CHECK_FALSE(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			// Show node_i and update the mouse. node_i and node_j should receive Mouse Enter. node_h should not receive any new signals.
			node_i->show();
			SEND_GUI_MOUSE_MOTION_EVENT(on_j, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_h->mouse_over);
			CHECK_FALSE(node_h->mouse_over_self);
			CHECK(node_i->mouse_over);
			CHECK_FALSE(node_i->mouse_over_self);
			CHECK(node_j->mouse_over);
			CHECK(node_j->mouse_over_self);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			CHECK_FALSE(node_h->invalid_order);
			CHECK_FALSE(node_i->invalid_order);
			CHECK_FALSE(node_j->invalid_order);

			node_i->set_mouse_filter(Control::MOUSE_FILTER_STOP);
			node_j->set_mouse_filter(Control::MOUSE_FILTER_STOP);

			SIGNAL_UNWATCH(node_h, SceneString(mouse_entered));
			SIGNAL_UNWATCH(node_h, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Window Mouse Enter/Exit signals.") {
			SIGNAL_WATCH(root, SceneString(mouse_entered));
			SIGNAL_WATCH(root, SceneString(mouse_exited));
			Array signal_args = { {} };

			SEND_GUI_MOUSE_MOTION_EVENT(on_outside, MouseButtonMask::NONE, Key::NONE);
			SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
			SIGNAL_CHECK(SceneString(mouse_exited), signal_args);

			SEND_GUI_MOUSE_MOTION_EVENT(on_a, MouseButtonMask::NONE, Key::NONE);
			SIGNAL_CHECK(SceneString(mouse_entered), signal_args);
			SIGNAL_CHECK_FALSE(SceneString(mouse_exited));

			SIGNAL_UNWATCH(root, SceneString(mouse_entered));
			SIGNAL_UNWATCH(root, SceneString(mouse_exited));
		}

		SUBCASE("[Viewport][GuiInputEvent] Process-Mode affects, if GUI Mouse Motion Events are processed.") {
			node_a->last_mouse_move_position = on_outside;
			node_a->set_process_mode(Node::PROCESS_MODE_DISABLED);
			SEND_GUI_MOUSE_MOTION_EVENT(on_a, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->last_mouse_move_position == on_outside);

			// Now verify that with allowed processing the event is processed.
			node_a->set_process_mode(Node::PROCESS_MODE_ALWAYS);
			SEND_GUI_MOUSE_MOTION_EVENT(on_a, MouseButtonMask::NONE, Key::NONE);
			CHECK(node_a->last_mouse_move_position == on_a);
		}
	}

	// Unit tests for Viewport::_gui_input_event (Drag and Drop)
	SUBCASE("[Viewport][GuiInputEvent] Drag and Drop") {
		// FIXME: Drag-Preview will likely change. Tests for this part would have to be rewritten anyway.
		// See https://github.com/godotengine/godot/pull/67531#issuecomment-1385353430 for details.
		// Note: Testing Drag and Drop with non-embedded windows would require DisplayServerMock additions.
		int min_grab_movement = 11;
		SUBCASE("[Viewport][GuiInputEvent][DnD] Drag from one Control to another in the same viewport.") {
			SUBCASE("[Viewport][GuiInputEvent][DnD] Perform successful Drag and Drop on a different Control.") {
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());

				// Move above a Control, that is a Drop target and allows dropping at this point.
				SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				CHECK(root->gui_is_dragging());
				CHECK_FALSE(root->gui_is_drag_successful());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK(root->gui_is_drag_successful());
				CHECK((String)node_d->drag_data == SNAME("Drag Data"));
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Perform unsuccessful drop on Control.") {
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				// Move, but don't trigger DnD yet.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(0, min_grab_movement - 1), MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				// Move and trigger DnD.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());

				// Move above a Control, that is not a Drop target.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_FORBIDDEN);

				// Move above a Control, that is a Drop target, but has disallowed this point.
				SEND_GUI_MOUSE_MOTION_EVENT(on_d + Point2i(20, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_FORBIDDEN);
				CHECK(root->gui_is_dragging());

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d + Point2i(20, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK_FALSE(root->gui_is_drag_successful());
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Perform unsuccessful drop on No-Control.") {
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				// Move, but don't trigger DnD yet.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement - 1, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				// Move and trigger DnD.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());

				// Move away from Controls.
				SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

				CHECK(root->gui_is_dragging());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK_FALSE(root->gui_is_drag_successful());
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Perform unsuccessful drop outside of window.") {
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				// Move and trigger DnD.
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());

				SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				// Move outside of window.
				SEND_GUI_MOUSE_MOTION_EVENT(on_outside, MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_outside, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK_FALSE(root->gui_is_drag_successful());
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag and Drop doesn't work with other Mouse Buttons than LMB.") {
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::MIDDLE, MouseButtonMask::MIDDLE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());

				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::MIDDLE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::MIDDLE, MouseButtonMask::NONE, Key::NONE);
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag and Drop parent propagation.") {
				Node2D *node_aa = memnew(Node2D);
				Control *node_aaa = memnew(Control);
				Node2D *node_dd = memnew(Node2D);
				Control *node_ddd = memnew(Control);
				node_aaa->set_size(Size2i(10, 10));
				node_aaa->set_position(Point2i(0, 5));
				node_ddd->set_size(Size2i(10, 10));
				node_ddd->set_position(Point2i(0, 5));
				node_a->add_child(node_aa);
				node_aa->add_child(node_aaa);
				node_d->add_child(node_dd);
				node_dd->add_child(node_ddd);
				Point2i on_aaa = on_a + Point2i(-2, 2);
				Point2i on_ddd = on_d + Point2i(-2, 2);

				SUBCASE("[Viewport][GuiInputEvent] Drag and Drop propagation to parent Controls.") {
					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_PASS);
					node_ddd->set_mouse_filter(Control::MOUSE_FILTER_PASS);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_aaa, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(on_aaa + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
					CHECK(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(on_ddd, MouseButtonMask::LEFT, Key::NONE);

					CHECK(root->gui_is_dragging());
					CHECK_FALSE(root->gui_is_drag_successful());
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_ddd, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());
					CHECK(root->gui_is_drag_successful());

					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_STOP);
					node_ddd->set_mouse_filter(Control::MOUSE_FILTER_STOP);
				}

				SUBCASE("[Viewport][GuiInputEvent] Drag and Drop grab-propagation stopped by Top Level.") {
					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_PASS);
					node_aaa->set_as_top_level(true);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_aaa, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(on_aaa + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					node_aaa->set_as_top_level(false);
					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_STOP);
				}

				SUBCASE("[Viewport][GuiInputEvent] Drag and Drop target-propagation stopped by Top Level.") {
					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_PASS);
					node_ddd->set_mouse_filter(Control::MOUSE_FILTER_PASS);
					node_ddd->set_as_top_level(true);
					node_ddd->set_position(Point2i(30, 100));

					SEND_GUI_MOUSE_BUTTON_EVENT(on_aaa, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(on_aaa + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
					CHECK(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(Point2i(35, 105), MouseButtonMask::LEFT, Key::NONE);

					CHECK(root->gui_is_dragging());
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(35, 105), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());
					CHECK_FALSE(root->gui_is_drag_successful());

					node_ddd->set_position(Point2i(0, 5));
					node_ddd->set_as_top_level(false);
					node_aaa->set_mouse_filter(Control::MOUSE_FILTER_STOP);
					node_ddd->set_mouse_filter(Control::MOUSE_FILTER_STOP);
				}

				SUBCASE("[Viewport][GuiInputEvent] Drag and Drop grab-propagation stopped by non-CanvasItem.") {
					node_g->set_mouse_filter(Control::MOUSE_FILTER_PASS);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
					SEND_GUI_MOUSE_MOTION_EVENT(on_g + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_background, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					node_g->set_mouse_filter(Control::MOUSE_FILTER_STOP);
				}

				SUBCASE("[Viewport][GuiInputEvent] Drag and Drop target-propagation stopped by non-CanvasItem.") {
					node_g->set_mouse_filter(Control::MOUSE_FILTER_PASS);

					SEND_GUI_MOUSE_BUTTON_EVENT(on_a - Point2i(1, 1), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE); // Offset for node_aaa.
					SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(0, min_grab_movement), MouseButtonMask::LEFT, Key::NONE);
					CHECK(root->gui_is_dragging());

					SEND_GUI_MOUSE_MOTION_EVENT(on_g, MouseButtonMask::LEFT, Key::NONE);
					SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_g, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
					CHECK_FALSE(root->gui_is_dragging());

					node_g->set_mouse_filter(Control::MOUSE_FILTER_STOP);
				}

				memdelete(node_ddd);
				memdelete(node_dd);
				memdelete(node_aaa);
				memdelete(node_aa);
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Force Drag and Drop.") {
				SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				node_a->force_drag(SNAME("Drag Data"), nullptr);
				CHECK(root->gui_is_dragging());

				SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::NONE, Key::NONE);

				// Force Drop doesn't get triggered by mouse Buttons other than LMB.
				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::MIDDLE, MouseButtonMask::MIDDLE, Key::NONE);
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::MIDDLE, MouseButtonMask::NONE, Key::NONE);
				CHECK(root->gui_is_dragging());

				// Force Drop with LMB-Down.
				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK(root->gui_is_drag_successful());

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

				node_a->force_drag(SNAME("Drag Data"), nullptr);
				CHECK(root->gui_is_dragging());

				// Cancel with RMB.
				SEND_GUI_MOUSE_BUTTON_EVENT(on_d, MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
				CHECK_FALSE(root->gui_is_dragging());
				CHECK_FALSE(root->gui_is_drag_successful());
				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_a, MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
			}
		}

		SUBCASE("[Viewport][GuiInputEvent][DnD] Drag to a different Viewport.") {
			SubViewportContainer *svc = memnew(SubViewportContainer);
			svc->set_size(Size2(100, 100));
			svc->set_position(Point2(200, 50));
			root->add_child(svc);

			SubViewport *sv = memnew(SubViewport);
			sv->set_embedding_subwindows(true);
			sv->set_size(Size2i(100, 100));
			svc->add_child(sv);

			DragStart *sv_a = memnew(DragStart);
			sv_a->set_position(Point2(10, 10));
			sv_a->set_size(Size2(10, 10));
			sv->add_child(sv_a);
			Point2i on_sva = Point2i(215, 65);

			DragTarget *sv_b = memnew(DragTarget);
			sv_b->set_position(Point2(30, 30));
			sv_b->set_size(Size2(20, 20));
			sv->add_child(sv_b);
			Point2i on_svb = Point2i(235, 85);

			Window *ew = memnew(Window);
			ew->set_position(Point2(50, 200));
			ew->set_size(Size2(100, 100));
			root->add_child(ew);

			DragStart *ew_a = memnew(DragStart);
			ew_a->set_position(Point2(10, 10));
			ew_a->set_size(Size2(10, 10));
			ew->add_child(ew_a);
			Point2i on_ewa = Point2i(65, 215);

			DragTarget *ew_b = memnew(DragTarget);
			ew_b->set_position(Point2(30, 30));
			ew_b->set_size(Size2(20, 20));
			ew->add_child(ew_b);
			Point2i on_ewb = Point2i(85, 235);

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag to SubViewport") {
				sv_b->valid_drop = false;
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());
				CHECK(sv_b->during_drag);
				SEND_GUI_MOUSE_MOTION_EVENT(on_svb, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_svb, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(sv_b->valid_drop);
				CHECK(!sv_b->during_drag);
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag from SubViewport") {
				node_d->valid_drop = false;
				SEND_GUI_MOUSE_BUTTON_EVENT(on_sva, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_MOTION_EVENT(on_sva + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(sv->gui_is_dragging());
				CHECK(node_d->during_drag);
				SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_d->valid_drop);
				CHECK(!node_d->during_drag);
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag to embedded Window") {
				ew_b->valid_drop = false;
				SEND_GUI_MOUSE_BUTTON_EVENT(on_a, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_MOTION_EVENT(on_a + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(root->gui_is_dragging());
				CHECK(ew_b->during_drag);
				SEND_GUI_MOUSE_MOTION_EVENT(on_ewb, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_ewb, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(ew_b->valid_drop);
				CHECK(!ew_b->during_drag);
			}

			SUBCASE("[Viewport][GuiInputEvent][DnD] Drag from embedded Window") {
				node_d->valid_drop = false;
				SEND_GUI_MOUSE_BUTTON_EVENT(on_ewa, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
				SEND_GUI_MOUSE_MOTION_EVENT(on_ewa + Point2i(min_grab_movement, 0), MouseButtonMask::LEFT, Key::NONE);
				CHECK(ew->gui_is_dragging());
				CHECK(node_d->during_drag);
				SEND_GUI_MOUSE_MOTION_EVENT(on_d, MouseButtonMask::LEFT, Key::NONE);
				CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_CAN_DROP);

				SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_d, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
				CHECK(node_d->valid_drop);
				CHECK(!node_d->during_drag);
			}

			memdelete(ew_a);
			memdelete(ew_b);
			memdelete(ew);
			memdelete(sv_a);
			memdelete(sv_b);
			memdelete(sv);
			memdelete(svc);
		}
	}

	memdelete(node_j);
	memdelete(node_i);
	memdelete(node_h);
	memdelete(node_g);
	memdelete(node_f);
	memdelete(node_e);
	memdelete(node_d);
	memdelete(node_c);
	memdelete(node_b);
	memdelete(node_a);
}

TEST_CASE("[SceneTree][Viewport] Control mouse cursor shape") {
	SUBCASE("[Viewport][CursorShape] Mouse cursor is not overridden by SubViewportContainer") {
		SubViewportContainer *node_a = memnew(SubViewportContainer);
		SubViewport *node_b = memnew(SubViewport);
		Control *node_c = memnew(Control);

		node_a->set_name("SubViewportContainer");
		node_b->set_name("SubViewport");
		node_c->set_name("Control");
		node_a->set_position(Point2i(0, 0));
		node_c->set_position(Point2i(0, 0));
		node_a->set_size(Point2i(100, 100));
		node_b->set_size(Point2i(100, 100));
		node_c->set_size(Point2i(100, 100));
		node_a->set_default_cursor_shape(Control::CURSOR_ARROW);
		node_c->set_default_cursor_shape(Control::CURSOR_FORBIDDEN);
		Window *root = SceneTree::get_root();
		DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();

		// Scene tree:
		// - root
		//   - node_a (SubViewportContainer)
		//     - node_b (SubViewport)
		//       - node_c (Control)

		root->add_child(node_a);
		node_a->add_child(node_b);
		node_b->add_child(node_c);

		Point2i on_c = Point2i(5, 5);

		SEND_GUI_MOUSE_MOTION_EVENT(on_c, MouseButtonMask::NONE, Key::NONE);
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_FORBIDDEN); // GH-74805

		memdelete(node_c);
		memdelete(node_b);
		memdelete(node_a);
	}
}

#ifndef PHYSICS_2D_DISABLED
class TestArea2D : public Area2D {

	void _on_mouse_entered() {
		enter_id = ++TestArea2D::counter; // > 0, if activated.
	}

	void _on_mouse_exited() {
		exit_id = ++TestArea2D::counter; // > 0, if activated.
	}

	void _on_input_event(Node *p_vp, Ref<InputEvent> p_ev, int p_shape) {
		last_input_event = p_ev;
	}

public:
	static int counter;
	int enter_id = 0;
	int exit_id = 0;
	Ref<InputEvent> last_input_event;

	void init_signals() {
		connect(SceneString(mouse_entered), callable_mp(this, &TestArea2D::_on_mouse_entered));
		connect(SceneString(mouse_exited), callable_mp(this, &TestArea2D::_on_mouse_exited));
		connect(SceneString(input_event), callable_mp(this, &TestArea2D::_on_input_event));
	}

	void test_reset() {
		enter_id = 0;
		exit_id = 0;
		last_input_event.unref();
	}
};

int TestArea2D::counter = 0;

TEST_CASE("[SceneTree][Viewport] Physics Picking 2D") {
	// FIXME: MOUSE_MODE_CAPTURED if-conditions are not testable, because DisplayServerMock doesn't support it.

	// NOTE: This test requires a real physics server.
	if (!PhysicsServer2D::is_initialized()) {
		return;
	}

	struct PickingCollider {
		TestArea2D *a;
		CollisionShape2D *c;
		Ref<Shape2D> r;
	};

	Window *root = SceneTree::get_root();
	root->set_physics_object_picking(true);

	Point2i on_background = Point2i(800, 800);
	Point2i on_outside = Point2i(-1, -1);
	SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
	SceneTree::physics_process(1);

	Vector<PickingCollider> v;
	for (int i = 0; i < 4; i++) {
		PickingCollider pc;
		pc.a = memnew(TestArea2D);
		pc.c = memnew(CollisionShape2D);
		pc.r = memnew(Shape2D(Shape2D::SHAPE_RECTANGLE));
		pc.r->set_size(Size2(150, 150));
		pc.c->set_shape(pc.r);
		pc.a->add_child(pc.c);
		pc.a->set_name("A" + itos(i));
		pc.c->set_name("C" + itos(i));
		v.push_back(pc);
		SIGNAL_WATCH(pc.a, SceneString(mouse_entered));
		SIGNAL_WATCH(pc.a, SceneString(mouse_exited));
	}

	Node2D *node_a = memnew(Node2D);
	node_a->set_position(Point2i(0, 0));
	v[0].a->set_position(Point2i(0, 0));
	v[1].a->set_position(Point2i(0, 100));
	node_a->add_child(v[0].a);
	node_a->add_child(v[1].a);
	Node2D *node_b = memnew(Node2D);
	node_b->set_position(Point2i(100, 0));
	v[2].a->set_position(Point2i(0, 0));
	v[3].a->set_position(Point2i(0, 100));
	node_b->add_child(v[2].a);
	node_b->add_child(v[3].a);
	root->add_child(node_a);
	root->add_child(node_b);
	Point2i on_all = Point2i(50, 50);
	Point2i on_0 = Point2i(10, 10);
	Point2i on_01 = Point2i(10, 50);
	Point2i on_02 = Point2i(50, 10);

	Array empty_signal_args_2 = { Array(), Array() };
	Array empty_signal_args_4 = { Array(), Array(), Array(), Array() };

	for (PickingCollider E : v) {
		E.a->init_signals();
	}

	SUBCASE("[Viewport][Picking2D] Mouse Motion") {
		SEND_GUI_MOUSE_MOTION_EVENT(on_all, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		SIGNAL_CHECK(SceneString(mouse_entered), empty_signal_args_4);
		SIGNAL_CHECK_FALSE(SceneString(mouse_exited));
		for (PickingCollider E : v) {
			CHECK(E.a->enter_id);
			CHECK_FALSE(E.a->exit_id);
			E.a->test_reset();
		}

		SEND_GUI_MOUSE_MOTION_EVENT(on_01, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
		SIGNAL_CHECK(SceneString(mouse_exited), empty_signal_args_2);

		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			if (i < 2) {
				CHECK_FALSE(v[i].a->exit_id);
			} else {
				CHECK(v[i].a->exit_id);
			}
			v[i].a->test_reset();
		}

		SEND_GUI_MOUSE_MOTION_EVENT(on_outside, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		SIGNAL_CHECK_FALSE(SceneString(mouse_entered));
		SIGNAL_CHECK(SceneString(mouse_exited), empty_signal_args_2);
		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			if (i < 2) {
				CHECK(v[i].a->exit_id);
			} else {
				CHECK_FALSE(v[i].a->exit_id);
			}
			v[i].a->test_reset();
		}
	}

	SUBCASE("[Viewport][Picking2D] Object moved / passive hovering") {
		SEND_GUI_MOUSE_MOTION_EVENT(on_all, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		for (int i = 0; i < v.size(); i++) {
			CHECK(v[i].a->enter_id);
			CHECK_FALSE(v[i].a->exit_id);
			v[i].a->test_reset();
		}

		node_b->set_position(Point2i(200, 0));
		SceneTree::physics_process(1);
		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			if (i < 2) {
				CHECK_FALSE(v[i].a->exit_id);
			} else {
				CHECK(v[i].a->exit_id);
			}
			v[i].a->test_reset();
		}

		node_b->set_position(Point2i(100, 0));
		SceneTree::physics_process(1);
		for (int i = 0; i < v.size(); i++) {
			if (i < 2) {
				CHECK_FALSE(v[i].a->enter_id);
			} else {
				CHECK(v[i].a->enter_id);
			}
			CHECK_FALSE(v[i].a->exit_id);
			v[i].a->test_reset();
		}
	}

	SUBCASE("[Viewport][Picking2D] No Processing") {
		SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		for (PickingCollider E : v) {
			E.a->test_reset();
		}

		v[0].a->set_process_mode(Node::PROCESS_MODE_DISABLED);
		v[0].c->set_process_mode(Node::PROCESS_MODE_DISABLED);
		SEND_GUI_MOUSE_MOTION_EVENT(on_02, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		CHECK_FALSE(v[0].a->enter_id);
		CHECK_FALSE(v[0].a->exit_id);
		CHECK(v[2].a->enter_id);
		CHECK_FALSE(v[2].a->exit_id);
		for (PickingCollider E : v) {
			E.a->test_reset();
		}

		SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);
		CHECK_FALSE(v[0].a->enter_id);
		CHECK_FALSE(v[0].a->exit_id);
		CHECK_FALSE(v[2].a->enter_id);
		CHECK(v[2].a->exit_id);

		for (PickingCollider E : v) {
			E.a->test_reset();
		}
		v[0].a->set_process_mode(Node::PROCESS_MODE_ALWAYS);
		v[0].c->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	}

	SUBCASE("[Viewport][Picking2D] Multiple events in series") {
		SEND_GUI_MOUSE_MOTION_EVENT(on_0, MouseButtonMask::NONE, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(on_0 + Point2i(10, 0), MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			if (i < 1) {
				CHECK(v[i].a->enter_id);
			} else {
				CHECK_FALSE(v[i].a->enter_id);
			}
			CHECK_FALSE(v[i].a->exit_id);
			v[i].a->test_reset();
		}

		SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(on_background + Point2i(10, 10), MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			if (i < 1) {
				CHECK(v[i].a->exit_id);
			} else {
				CHECK_FALSE(v[i].a->exit_id);
			}
			v[i].a->test_reset();
		}
	}

	SUBCASE("[Viewport][Picking2D] Disable Picking") {
		SEND_GUI_MOUSE_MOTION_EVENT(on_02, MouseButtonMask::NONE, Key::NONE);

		root->set_physics_object_picking(false);
		CHECK_FALSE(root->get_physics_object_picking());

		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			v[i].a->test_reset();
		}

		root->set_physics_object_picking(true);
		CHECK(root->get_physics_object_picking());
	}

	SUBCASE("[Viewport][Picking2D] CollisionObject in CanvasLayer") {
		CanvasLayer *node_c = memnew(CanvasLayer);
		node_c->set_rotation(Math::PI);
		node_c->set_offset(Point2i(100, 100));
		root->add_child(node_c);

		v[2].a->reparent(node_c, false);
		v[3].a->reparent(node_c, false);

		SEND_GUI_MOUSE_MOTION_EVENT(on_02, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			if (i == 0 || i == 3) {
				CHECK(v[i].a->enter_id);
			} else {
				CHECK_FALSE(v[i].a->enter_id);
			}
			v[i].a->test_reset();
		}

		v[2].a->reparent(node_b, false);
		v[3].a->reparent(node_b, false);
		root->remove_child(node_c);
		memdelete(node_c);
	}

	SUBCASE("[Viewport][Picking2D] Picking Sort") {
		root->set_physics_object_picking_sort(true);
		CHECK(root->get_physics_object_picking_sort());

		SUBCASE("[Viewport][Picking2D] Picking Sort Z-Index") {
			node_a->set_z_index(10);
			v[0].a->set_z_index(0);
			v[1].a->set_z_index(2);
			node_b->set_z_index(5);
			v[2].a->set_z_index(8);
			v[3].a->set_z_index(11);
			v[3].a->set_z_as_relative(false);

			TestArea2D::counter = 0;
			SEND_GUI_MOUSE_MOTION_EVENT(on_all, MouseButtonMask::NONE, Key::NONE);
			SceneTree::physics_process(1);

			CHECK(v[0].a->enter_id == 4);
			CHECK(v[1].a->enter_id == 2);
			CHECK(v[2].a->enter_id == 1);
			CHECK(v[3].a->enter_id == 3);
			for (int i = 0; i < v.size(); i++) {
				CHECK_FALSE(v[i].a->exit_id);
				v[i].a->test_reset();
			}

			TestArea2D::counter = 0;
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			SceneTree::physics_process(1);

			CHECK(v[0].a->exit_id == 4);
			CHECK(v[1].a->exit_id == 2);
			CHECK(v[2].a->exit_id == 1);
			CHECK(v[3].a->exit_id == 3);
			for (int i = 0; i < v.size(); i++) {
				CHECK_FALSE(v[i].a->enter_id);
				v[i].a->set_z_as_relative(true);
				v[i].a->set_z_index(0);
				v[i].a->test_reset();
			}

			node_a->set_z_index(0);
			node_b->set_z_index(0);
		}

		SUBCASE("[Viewport][Picking2D] Picking Sort Scene Tree Location") {
			TestArea2D::counter = 0;
			SEND_GUI_MOUSE_MOTION_EVENT(on_all, MouseButtonMask::NONE, Key::NONE);
			SceneTree::physics_process(1);

			for (int i = 0; i < v.size(); i++) {
				CHECK(v[i].a->enter_id == 4 - i);
				CHECK_FALSE(v[i].a->exit_id);
				v[i].a->test_reset();
			}

			TestArea2D::counter = 0;
			SEND_GUI_MOUSE_MOTION_EVENT(on_background, MouseButtonMask::NONE, Key::NONE);
			SceneTree::physics_process(1);

			for (int i = 0; i < v.size(); i++) {
				CHECK_FALSE(v[i].a->enter_id);
				CHECK(v[i].a->exit_id == 4 - i);
				v[i].a->test_reset();
			}
		}

		root->set_physics_object_picking_sort(false);
		CHECK_FALSE(root->get_physics_object_picking_sort());
	}

	SUBCASE("[Viewport][Picking2D] Mouse Button") {
		SEND_GUI_MOUSE_BUTTON_EVENT(on_0, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			if (i == 0) {
				CHECK(v[i].a->enter_id);
			} else {
				CHECK_FALSE(v[i].a->enter_id);
			}
			CHECK_FALSE(v[i].a->exit_id);
			v[i].a->test_reset();
		}

		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(on_0, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		SceneTree::physics_process(1);

		for (int i = 0; i < v.size(); i++) {
			CHECK_FALSE(v[i].a->enter_id);
			CHECK_FALSE(v[i].a->exit_id);
			v[i].a->test_reset();
		}
	}

	SUBCASE("[Viewport][Picking2D] Screen Touch") {
		SEND_GUI_TOUCH_EVENT(on_01, true, false);
		SceneTree::physics_process(1);
		for (int i = 0; i < v.size(); i++) {
			if (i < 2) {
				Ref<InputEventScreenTouch> st = v[i].a->last_input_event;
				CHECK(st.is_valid());
			} else {
				CHECK(v[i].a->last_input_event.is_null());
			}
			v[i].a->test_reset();
		}
	}

	for (PickingCollider E : v) {
		SIGNAL_UNWATCH(E.a, SceneString(mouse_entered));
		SIGNAL_UNWATCH(E.a, SceneString(mouse_exited));
		memdelete(E.c);
		memdelete(E.a);
	}
}
#endif // PHYSICS_2D_DISABLED

TEST_CASE("[SceneTree][Viewport] Embedded Windows") {
	Window *root = SceneTree::get_root();
	Window *w = memnew(Window);

	SUBCASE("[Viewport] Safe-rect of embedded Window") {
		root->add_child(w);
		root->subwindow_set_popup_safe_rect(w, Rect2i(10, 10, 10, 10));
		CHECK_EQ(root->subwindow_get_popup_safe_rect(w), Rect2i(10, 10, 10, 10));
		root->remove_child(w);
		CHECK_EQ(root->subwindow_get_popup_safe_rect(w), Rect2i());
	}

	SUBCASE("[Viewport] Clamp to embedder on resize") {
		root->set_size(Size2i(200, 200));
		root->set_embedding_subwindows(true);
		root->add_child(w);
		w->set_size(Size2i(100, 100));
		w->set_position(Point2i(150, 0));
		const int title_height = root->get_theme_constant("title_height");

		// Do not clamp to embedder on resize.
		w->set_clamp_to_embedder(false);
		CHECK_FALSE(w->is_clamped_to_embedder());
		root->set_size(Size2i(200, 201));
		CHECK(w->get_size() == Size2i(100, 100));
		CHECK(w->get_position() == Point2i(150, 0));

		// Clamp to embedder on resize.
		w->set_clamp_to_embedder(true);
		CHECK(w->is_clamped_to_embedder());
		root->set_size(Size2i(200, 202));
		CHECK(w->get_size() == Size2i(100, 100));
		CHECK(w->get_position() == Point2i(100, title_height));
	}

	memdelete(w);
}

} // namespace TestViewport

// === Merged from tests/scene/test_window.cpp ===
/*  test_window.cpp                                                       */
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



#include "core/os/os.h" // IWYU pragma: keep // Used by `SEND_GUI_MOUSE_MOTION_EVENT` macro.
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

namespace TestWindow {

class NotificationControlWindow : public Control {

protected:
	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_MOUSE_ENTER: {
				mouse_over = true;
			} break;

			case NOTIFICATION_MOUSE_EXIT: {
				mouse_over = false;
			} break;
		}
	}

public:
	bool mouse_over = false;
};

TEST_CASE("[SceneTree][Window]") {
	Window *root = SceneTree::get_root();

	SUBCASE("Control-mouse-over within Window-black bars should not happen") {
		Window *w = memnew(Window);
		root->add_child(w);
		w->set_size(Size2i(400, 200));
		w->set_position(Size2i(0, 0));
		w->set_content_scale_size(Size2i(200, 200));
		w->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
		w->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
		NotificationControlWindow *c = memnew(NotificationControlWindow);
		w->add_child(c);
		c->set_size(Size2i(100, 100));
		c->set_position(Size2i(-50, -50));

		CHECK_FALSE(c->mouse_over);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2i(110, 10), MouseButtonMask::NONE, Key::NONE);
		CHECK(c->mouse_over);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2i(90, 10), MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(c->mouse_over); // GH-80011

		/* TODO:
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(90, 10), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2i(90, 10), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK(Control was not pressed);
		*/

		memdelete(c);
		memdelete(w);
	}
}

} // namespace TestWindow

TEST_FORCE_LINK(test_scene_nodes)

