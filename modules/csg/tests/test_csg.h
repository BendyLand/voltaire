/**************************************************************************/
/*  test_csg.h                                                            */
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

#include "modules/gltf/gltf_document.h"

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

namespace TestCSG {

TEST_CASE("[SceneTree][CSG] CSGPolygon3D") {
	SUBCASE("[SceneTree][CSG] CSGPolygon3D: using accurate path tangent for polygon rotation") {
		const float polygon_radius = 10.0f;

		const Vector3 expected_min_bounds = Vector3(-polygon_radius, -polygon_radius, 0);
		const Vector3 expected_max_bounds = Vector3(100 + polygon_radius, polygon_radius, 100);
		const AABB expected_aabb = AABB(expected_min_bounds, expected_max_bounds - expected_min_bounds);

		Ref<Curve3D> curve;
		curve.instantiate();
		curve->add_point(
				// p_position
				Vector3(0, 0, 0),
				// p_in
				Vector3(),
				// p_out
				Vector3(0, 0, 60));
		curve->add_point(
				// p_position
				Vector3(100, 0, 100),
				// p_in
				Vector3(0, 0, -60),
				// p_out
				Vector3());

		Path3D *path = memnew(Path3D);
		path->set_curve(curve);

		CSGShape3D *csg_polygon_3d = memnew(CSGShape3D(CSGShape3D::SHAPE_POLYGON));
		SceneTree::get_root()->add_child(csg_polygon_3d);

		csg_polygon_3d->add_child(path);
		csg_polygon_3d->set_path_node(csg_polygon_3d->get_path_to(path));
		csg_polygon_3d->set_polygon_mode(CSGShape3D::POLYGON_MODE_PATH);

		PackedVector2Array polygon;
		polygon.append(Vector2(-polygon_radius, 0));
		polygon.append(Vector2(0, polygon_radius));
		polygon.append(Vector2(polygon_radius, 0));
		polygon.append(Vector2(0, -polygon_radius));
		csg_polygon_3d->set_polygon(polygon);

		csg_polygon_3d->set_path_rotation(CSGShape3D::PATH_ROTATION_PATH);
		csg_polygon_3d->set_path_rotation_accurate(true);

		// Minimize the number of extrusions.
		// This decreases the number of samples taken from the curve.
		// Having fewer samples increases the inaccuracy of the line between samples as an approximation of the tangent of the curve.
		// With correct polygon orientation, the bounding box for the given curve should be independent of the number of extrusions.
		csg_polygon_3d->set_path_interval_type(CSGShape3D::PATH_INTERVAL_DISTANCE);
		csg_polygon_3d->set_path_interval(1000.0f);

		// Call get_brush_faces to force the bounding box to update.
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		// Perform the bounding box check again with a greater number of extrusions.
		csg_polygon_3d->set_path_interval(1.0f);
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		csg_polygon_3d->remove_child(path);
		SceneTree::get_root()->remove_child(csg_polygon_3d);

		memdelete(csg_polygon_3d);
		memdelete(path);
	}
}

} // namespace TestCSG

// --- Folded from modules/gridmap/tests/test_grid_map.h ---
#include "modules/gltf/gltf_document.h"

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "tests/test_macros.h"

namespace TestGridMap {

TEST_CASE("[SceneTree][GridMap][MeshLibrary] Octant querying") {
	Ref<MeshLibrary> mesh_library;
	mesh_library.instantiate();

	int item1 = 0;
	int item2 = 1;

	{
		Vector<int> item_list = mesh_library->get_item_list();
		CHECK(item_list.size() == 0);
	}

	mesh_library->create_item(item1);
	mesh_library->create_item(item2);

	int last_unused_item_id = mesh_library->get_last_unused_item_id();
	CHECK(last_unused_item_id == 2);

	mesh_library->set_item_name(item1, "Item1");
	mesh_library->set_item_name(item2, "Item2");

	Ref<PrimitiveMesh> box_mesh;
	box_mesh.instantiate(PrimitiveMesh::MESH_BOX);
	box_mesh->set_size(Vector3(1.0, 1.0, 1.0));

	mesh_library->set_item_mesh(item1, box_mesh);
	mesh_library->set_item_mesh(item2, box_mesh);

	{
		Vector<int> item_list = mesh_library->get_item_list();
		CHECK(item_list.size() == 2);
		CHECK(item_list[0] == item1);
		CHECK(item_list[1] == item2);
	}

	GridMap *grid_map = memnew(GridMap);
	grid_map->set_mesh_library(mesh_library);

	SceneTree::get_root()->add_child(grid_map);

	grid_map->set_octant_size(8);
	grid_map->set_cell_size(Vector3(1.0, 1.0, 1.0));

	grid_map->set_cell_item(Vector3i(0, 0, 0), -1);

	CHECK(grid_map->get_used_cells().size() == 0);
	CHECK(grid_map->get_used_octants().size() == 0);

	grid_map->set_cell_item(Vector3i(0, 0, 0), item1);
	grid_map->set_cell_item(Vector3i(7, 7, 7), item2);

	CHECK(grid_map->get_used_cells().size() == 2);
	CHECK(grid_map->get_used_octants().size() == 1);
	CHECK(grid_map->get_used_octants()[0] == Vector3i(0, 0, 0));

	grid_map->set_cell_item(Vector3i(8, 8, 8), item2);

	CHECK(grid_map->get_used_cells().size() == 3);
	CHECK(grid_map->get_used_octants().size() == 2);
	CHECK(grid_map->get_used_octants()[0] == Vector3i(0, 0, 0));
	CHECK(grid_map->get_used_octants()[1] == Vector3i(1, 1, 1));

	AABB bounds;
	bounds.position = Vector3(0.0, 0.0, 0.0);
	bounds.size = Vector3(8.0, 8.0, 8.0);

	CHECK(grid_map->get_octants_in_bounds(bounds).size() == 1);
	CHECK(grid_map->get_used_octants_in_bounds(bounds).size() == 1);
	CHECK(grid_map->get_octants_in_bounds(bounds)[0] == Vector3i(0, 0, 0));

	// Edge margin is -CMP_EPSILON.
	bounds.size = Vector3(8.001, 8.001, 8.001);
	CHECK(grid_map->get_octants_in_bounds(bounds).size() == 8);
	CHECK(grid_map->get_used_octants_in_bounds(bounds).size() == 2);

	SceneTree::get_root()->remove_child(grid_map);

	memdelete(grid_map);
}

} // namespace TestGridMap
// --- End modules/gridmap/tests/test_grid_map.h ---
