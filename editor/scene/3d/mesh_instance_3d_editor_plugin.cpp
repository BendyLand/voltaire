/**************************************************************************/
/*  mesh_instance_3d_editor_plugin.cpp                                    */
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

#include "core/io/resource_loader.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "mesh_instance_3d_editor_plugin.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/box_container.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/spin_box.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sphere_shape_3d.h"

void MeshInstance3DEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
		options->hide();
	}
}

void MeshInstance3DEditor::edit(MeshInstance3D* p_mesh) { node = p_mesh; }

Vector<Ref<Shape3D>> MeshInstance3DEditor::create_shape_from_mesh(
	Ref<Mesh> p_mesh, int p_option, bool p_verbose)
{
	Vector<Ref<Shape3D>> shapes;
	switch (p_option) {
	case SHAPE_TYPE_TRIMESH: {
		shapes.push_back(p_mesh->create_trimesh_shape());

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a Trimesh collision shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_SINGLE_CONVEX: {
		shapes.push_back(p_mesh->create_convex_shape(true, false));

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a single collision shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_SIMPLIFIED_CONVEX: {
		shapes.push_back(p_mesh->create_convex_shape(true, true));

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a simplified collision shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_MULTIPLE_CONVEX: {
		Ref<MeshConvexDecompositionSettings> settings;
		settings.instantiate();
		settings->set_max_convex_hulls(32);
		settings->set_max_concavity(0.001);

		shapes = p_mesh->convex_decompose(settings);

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create any collision shapes."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_BOUNDING_BOX: {
		const Ref<BoxMesh> box_mesh = p_mesh;
		if (box_mesh.is_valid()) {
			Ref<BoxShape3D> box_shape;
			box_shape.instantiate();
			box_shape->set_size(box_mesh->get_size().maxf(0.001));
			shapes.push_back(box_shape);
		}
		else {
			Ref<BoxShape3D> box_shape;
			box_shape.instantiate();
			AABB mesh_aabb = p_mesh->get_aabb();
			box_shape->set_size(mesh_aabb.get_size().maxf(0.001));
			shapes.push_back(box_shape);
			shape_offset_transform.origin = mesh_aabb.get_center();
		}

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a bounding box shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_CAPSULE: {
		const Ref<CapsuleMesh> capsule_mesh = p_mesh;
		if (capsule_mesh.is_valid()) {
			Ref<CapsuleShape3D> capsule_shape;
			capsule_shape.instantiate();
			capsule_shape->set_height(capsule_mesh->get_height());
			capsule_shape->set_radius(MAX(capsule_mesh->get_radius(), 0.001));
			shapes.push_back(capsule_shape);
		}
		else {
			// Use AABB to estimate shape.
			Ref<CapsuleShape3D> capsule_shape;
			capsule_shape.instantiate();
			AABB mesh_aabb = p_mesh->get_aabb();
			int axis = shape_axis->get_selected_id();
			if (axis == (int)SHAPE_AXIS_LONGEST) {
				axis = mesh_aabb.get_longest_axis_index();
			}
			int perpendicular_axis = axis == 0 ? 1 : 0;

			capsule_shape->set_height(mesh_aabb.get_size()[axis]);
			capsule_shape->set_radius(MAX(mesh_aabb.get_size()[perpendicular_axis] / 2.0, 0.001));
			shapes.push_back(capsule_shape);

			shape_offset_transform.origin = mesh_aabb.get_center();
			if (axis == Vector3::AXIS_X) {
				shape_offset_transform.rotate_basis(Vector3(0, 0, 1), Math::PI / 2.0);
			}
			else if (axis == Vector3::AXIS_Z) {
				shape_offset_transform.rotate_basis(Vector3(1, 0, 0), -Math::PI / 2.0);
			}
		}

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a capsule shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_CYLINDER: {
		const Ref<CylinderMesh> cylinder_mesh = p_mesh;
		if (cylinder_mesh.is_valid()) {
			Ref<CylinderShape3D> cylinder_shape;
			cylinder_shape.instantiate();
			cylinder_shape->set_height(MAX(cylinder_mesh->get_height(), 0.001));
			cylinder_shape->set_radius(
				(cylinder_mesh->get_top_radius() + cylinder_mesh->get_bottom_radius()) / 2.0);
			shapes.push_back(cylinder_shape);
		}
		else {
			// Use AABB to estimate shape.
			Ref<CylinderShape3D> cylinder_shape;
			cylinder_shape.instantiate();
			AABB mesh_aabb = p_mesh->get_aabb();
			int axis = shape_axis->get_selected_id();
			if (axis == (int)SHAPE_AXIS_LONGEST) {
				axis = mesh_aabb.get_longest_axis_index();
			}
			int perpendicular_axis = axis == 0 ? 1 : 0;

			cylinder_shape->set_height(MAX(mesh_aabb.get_size()[axis], 0.001));
			cylinder_shape->set_radius(mesh_aabb.get_size()[perpendicular_axis] / 2.0);
			shapes.push_back(cylinder_shape);

			shape_offset_transform.origin = mesh_aabb.get_center();
			if (axis == Vector3::AXIS_X) {
				shape_offset_transform.rotate_basis(Vector3(0, 0, 1), Math::PI / 2.0);
			}
			else if (axis == Vector3::AXIS_Z) {
				shape_offset_transform.rotate_basis(Vector3(1, 0, 0), -Math::PI / 2.0);
			}
		}

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a cylinder shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_SPHERE: {
		const Ref<SphereMesh> sphere_mesh = p_mesh;
		if (sphere_mesh.is_valid()) {
			Ref<SphereShape3D> sphere_shape;
			sphere_shape.instantiate();
			sphere_shape->set_radius(MAX(sphere_mesh->get_radius(), 0.001));
			shapes.push_back(sphere_shape);
		}
		else {
			// Use AABB to estimate shape.
			Ref<SphereShape3D> sphere_shape;
			sphere_shape.instantiate();
			AABB mesh_aabb = p_mesh->get_aabb();
			sphere_shape->set_radius(
				MAX(mesh_aabb.get_size()[mesh_aabb.get_size().max_axis_index()] / 2.0, 0.001));
			shapes.push_back(sphere_shape);
			shape_offset_transform.origin = mesh_aabb.get_center();
		}

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a sphere shape."));
			err_dialog->popup_centered();
		}
	} break;

	case SHAPE_TYPE_PRIMITIVE: {
		const Ref<BoxMesh> box_mesh = p_mesh;
		if (box_mesh.is_valid()) {
			Ref<BoxShape3D> box_shape;
			box_shape.instantiate();
			box_shape->set_size(box_mesh->get_size().maxf(0.001));
			shapes.push_back(box_shape);
		}

		const Ref<CapsuleMesh> capsule_mesh = p_mesh;
		if (capsule_mesh.is_valid()) {
			Ref<CapsuleShape3D> capsule_shape;
			capsule_shape.instantiate();
			capsule_shape->set_height(capsule_mesh->get_height());
			capsule_shape->set_radius(MAX(capsule_mesh->get_radius(), 0.001));
			shapes.push_back(capsule_shape);
		}

		const Ref<CylinderMesh> cylinder_mesh = p_mesh;
		if (cylinder_mesh.is_valid()) {
			Ref<CylinderShape3D> cylinder_shape;
			cylinder_shape.instantiate();
			cylinder_shape->set_height(MAX(cylinder_mesh->get_height(), 0.001));
			cylinder_shape->set_radius(
				(cylinder_mesh->get_top_radius() + cylinder_mesh->get_bottom_radius()) / 2.0);
			shapes.push_back(cylinder_shape);
		}

		const Ref<SphereMesh> sphere_mesh = p_mesh;
		if (sphere_mesh.is_valid()) {
			Ref<SphereShape3D> sphere_shape;
			sphere_shape.instantiate();
			sphere_shape->set_radius(MAX(sphere_mesh->get_radius(), 0.001));
			shapes.push_back(sphere_shape);
		}

		if (p_verbose && shapes.is_empty()) {
			err_dialog->set_text(TTR("Couldn't create a primitive collision shape."));
			err_dialog->popup_centered();
		}
	} break;

	default:
		break;
	}
	return shapes;
}

void MeshInstance3DEditor::_shape_type_selected(int p_option)
{
	bool shape_axis_visible =
		(ShapeType)p_option == SHAPE_TYPE_CAPSULE || (ShapeType)p_option == SHAPE_TYPE_CYLINDER;
	shape_axis->set_visible(shape_axis_visible);
	shape_axis_label->set_visible(shape_axis_visible);
}

struct MeshInstance3DEditorEdgeSort
{
	Vector2 a;
	Vector2 b;

	static uint32_t hash(const MeshInstance3DEditorEdgeSort& p_edge)
	{
		uint32_t h = hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.a));
		return hash_fmix32(hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.b), h));
	}

	bool operator==(const MeshInstance3DEditorEdgeSort& p_b) const
	{
		return a == p_b.a && b == p_b.b;
	}

	MeshInstance3DEditorEdgeSort() {}

	MeshInstance3DEditorEdgeSort(const Vector2& p_a, const Vector2& p_b)
	{
		if (p_a < p_b) {
			a = p_a;
			b = p_b;
		}
		else {
			b = p_a;
			a = p_b;
		}
	}
};

void MeshInstance3DEditor::_debug_uv_draw()
{
	if (uv_lines.is_empty()) {
		return;
	}

	debug_uv->set_clip_contents(true);
	debug_uv->draw_rect(Rect2(Vector2(), debug_uv->get_size()),
		get_theme_color(SNAME("dark_color_3"), EditorStringName(Editor)));

	// Draw an outline to represent the UV2's beginning and end area (useful on Black OLED theme).
	// Top-left coordinate needs to be `(1, 1)` to prevent `clip_contents` from clipping the top and
	// left lines.
	debug_uv->draw_rect(Rect2(Vector2(1, 1), debug_uv->get_size() - Vector2(1, 1)),
		get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.125),
		false, Math::round(EDSCALE));

	for (int x = 1; x <= 7; x++) {
		debug_uv->draw_line(Vector2(debug_uv->get_size().x * 0.125 * x, 0),
			Vector2(debug_uv->get_size().x * 0.125 * x, debug_uv->get_size().y),
			get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.125),
			Math::round(EDSCALE));
	}

	for (int y = 1; y <= 7; y++) {
		debug_uv->draw_line(Vector2(0, debug_uv->get_size().y * 0.125 * y),
			Vector2(debug_uv->get_size().x, debug_uv->get_size().y * 0.125 * y),
			get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.125),
			Math::round(EDSCALE));
	}

	debug_uv->draw_set_transform(Vector2(), 0, debug_uv->get_size());

	// Use a translucent color to allow overlapping triangles to be visible.
	// Divide line width by the drawing scale set above, so that line width is consistent regardless
	// of dialog size. Aspect ratio is preserved by the parent AspectRatioContainer, so we only need
	// to check the X size which is always equal to Y.
	debug_uv->draw_multiline(uv_lines,
		get_theme_color(SNAME("mono_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.5),
		Math::round(EDSCALE) / debug_uv->get_size().x);
}

void MeshInstance3DEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		options->set_button_icon(get_editor_theme_icon(SNAME("MeshInstance3D")));
	} break;
	}
}

MeshInstance3DEditorPlugin::MeshInstance3DEditorPlugin()
{
	mesh_editor = memnew(MeshInstance3DEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(mesh_editor);

	mesh_editor->options->hide();
}


