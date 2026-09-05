/**************************************************************************/
/*  node_3d_editor_gizmos.cpp                                             */
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

#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/settings/editor_settings.h"
#include "node_3d_editor_gizmos.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/rendering/rendering_server.h"

#define HANDLE_HALF_SIZE 9.5

bool EditorNode3DGizmo::is_editable() const
{
	ERR_FAIL_NULL_V(spatial_node, false);
	Node* edited_root = spatial_node->get_tree()->get_edited_scene_root();
	if (spatial_node == edited_root) {
		return true;
	}
	if (spatial_node->get_owner() == edited_root) {
		return true;
	}

	if (edited_root->is_editable_instance(spatial_node->get_owner())) {
		return true;
	}

	return false;
}

void EditorNode3DGizmo::clear()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	for (int i = 0; i < instances.size(); i++) {
		if (instances[i].instance.is_valid()) {
			RS::get_singleton()->free_rid(instances[i].instance);
		}
	}

	billboard_handle = false;
	collision_segments.clear();
	collision_meshes.clear();
	collision_meshes_are_snap_source = false;
	instances.clear();
	handles.clear();
	handle_ids.clear();
	secondary_handles.clear();
	secondary_handle_ids.clear();
}

void EditorNode3DGizmo::redraw()
{
	_update_bvh();
	if (Node3DEditor::get_singleton()->is_current_selected_gizmo(this)) {
		Node3DEditor::get_singleton()->update_transform_gizmo();
	}
}

String EditorNode3DGizmo::get_handle_name(int p_id, bool p_secondary) const
{
	ERR_FAIL_NULL_V(gizmo_plugin, "");
	return gizmo_plugin->get_handle_name(this, p_id, p_secondary);
}

bool EditorNode3DGizmo::is_handle_highlighted(int p_id, bool p_secondary) const
{
	ERR_FAIL_NULL_V(gizmo_plugin, false);
	return gizmo_plugin->is_handle_highlighted(this, p_id, p_secondary);
}

void EditorNode3DGizmo::begin_handle_action(int p_id, bool p_secondary)
{
	ERR_FAIL_NULL(gizmo_plugin);
	gizmo_plugin->begin_handle_action(this, p_id, p_secondary);
}

void EditorNode3DGizmo::set_handle(
	int p_id, bool p_secondary, Camera3D* p_camera, const Point2& p_point)
{
	ERR_FAIL_NULL(gizmo_plugin);
	gizmo_plugin->set_handle(this, p_id, p_secondary, p_camera, p_point);
}

int EditorNode3DGizmo::subgizmos_intersect_ray(Camera3D* p_camera, const Vector2& p_point) const
{
	ERR_FAIL_NULL_V(gizmo_plugin, -1);
	return gizmo_plugin->subgizmos_intersect_ray(this, p_camera, p_point);
}

Transform3D EditorNode3DGizmo::get_subgizmo_transform(int p_id) const
{
	ERR_FAIL_NULL_V(gizmo_plugin, Transform3D());
	return gizmo_plugin->get_subgizmo_transform(this, p_id);
}

void EditorNode3DGizmo::set_subgizmo_transform(int p_id, Transform3D p_transform)
{
	ERR_FAIL_NULL(gizmo_plugin);
	gizmo_plugin->set_subgizmo_transform(this, p_id, p_transform);
}

void EditorNode3DGizmo::set_node_3d(Node3D* p_node)
{
	ERR_FAIL_NULL(p_node);
	spatial_node = p_node;
}

void EditorNode3DGizmo::add_mesh(const Ref<Mesh>& p_mesh, const Ref<Material>& p_material,
	const Transform3D& p_xform, const Ref<SkinReference>& p_skin_reference)
{
	ERR_FAIL_NULL(spatial_node);
	ERR_FAIL_COND_MSG(
		p_mesh.is_null(), "EditorNode3DGizmo.add_mesh() requires a valid Mesh resource.");

	Instance ins;

	ins.mesh = p_mesh;
	ins.skin_reference = p_skin_reference;
	ins.material = p_material;
	ins.xform = p_xform;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		RS::get_singleton()->instance_set_transform(
			ins.instance, spatial_node->get_global_transform() * ins.xform);
		if (ins.material.is_valid()) {
			RS::get_singleton()->instance_geometry_set_material_override(
				ins.instance, p_material->get_rid());
		}
	}

	instances.push_back(ins);
}

void EditorNode3DGizmo::_update_bvh()
{
	ERR_FAIL_NULL(spatial_node);

	Transform3D transform = spatial_node->get_global_transform();

	float effective_icon_size = selectable_icon_size > 0.0f ? selectable_icon_size : 0.0f;
	Vector3 icon_size_vector3 =
		Vector3(effective_icon_size, effective_icon_size, effective_icon_size);
	AABB aabb(transform.origin - icon_size_vector3 * 100.0f, icon_size_vector3 * 200.0f);

	for (const Vector3& segment_end : collision_segments) {
		aabb.expand_to(transform.xform(segment_end));
	}

	if (!collision_meshes.is_empty()) {
		for (Ref<TriangleMesh> collision_mesh : collision_meshes) {
			if (collision_mesh.is_valid()) {
				for (const Face3& face : collision_mesh->get_faces()) {
					aabb.expand_to(transform.xform(face.vertex[0]));
					aabb.expand_to(transform.xform(face.vertex[1]));
					aabb.expand_to(transform.xform(face.vertex[2]));
				}
			}
		}
	}

	Node3DEditor::get_singleton()->update_gizmo_bvh_node(bvh_node_id, aabb);
}

void EditorNode3DGizmo::add_lines(const Vector<Vector3>& p_lines, const Ref<Material>& p_material,
	bool p_billboard, const Color& p_modulate)
{
	add_vertices(p_lines, p_material, Mesh::PRIMITIVE_LINES, p_billboard, p_modulate);
}

void EditorNode3DGizmo::add_collision_triangles(const Ref<TriangleMesh>& p_tmesh)
{
	collision_meshes.push_back(p_tmesh);
}

void EditorNode3DGizmo::add_collision_segments(const Vector<Vector3>& p_lines)
{
	int from = collision_segments.size();
	collision_segments.resize(from + p_lines.size());
	for (int i = 0; i < p_lines.size(); i++) {
		collision_segments.write[from + i] = p_lines[i];
	}
}

bool EditorNode3DGizmo::intersect_frustum(const Camera3D* p_camera, const Vector<Plane>& p_frustum)
{
	ERR_FAIL_NULL_V(spatial_node, false);
	ERR_FAIL_COND_V(!valid, false);

	if (hidden && !gizmo_plugin->is_selectable_when_hidden()) {
		return false;
	}

	if (selectable_icon_size > 0.0f) {
		Vector3 origin = spatial_node->get_global_transform().get_origin();

		const Plane* p = p_frustum.ptr();
		int fc = p_frustum.size();

		bool any_out = false;

		for (int j = 0; j < fc; j++) {
			if (p[j].is_point_over(origin)) {
				any_out = true;
				break;
			}
		}

		return !any_out;
	}

	if (collision_segments.size()) {
		const Plane* p = p_frustum.ptr();
		int fc = p_frustum.size();

		int vc = collision_segments.size();
		const Vector3* vptr = collision_segments.ptr();
		Transform3D t = spatial_node->get_global_transform();

		bool any_out = false;
		for (int j = 0; j < fc; j++) {
			for (int i = 0; i < vc; i++) {
				Vector3 v = t.xform(vptr[i]);
				if (p[j].is_point_over(v)) {
					any_out = true;
					break;
				}
			}
			if (any_out) {
				break;
			}
		}

		if (!any_out) {
			return true;
		}
	}

	if (!collision_meshes.is_empty()) {
		Transform3D t = spatial_node->get_global_transform();

		Vector3 mesh_scale = t.get_basis().get_scale();
		t.orthonormalize();

		Transform3D it = t.affine_inverse();

		Vector<Plane> transformed_frustum;
		int plane_count = p_frustum.size();
		transformed_frustum.resize(plane_count);

		for (int i = 0; i < plane_count; i++) {
			transformed_frustum.write[i] = it.xform(p_frustum[i]);
		}

		Vector<Vector3> convex_points =
			Geometry3D::compute_convex_mesh_points(transformed_frustum.ptr(), plane_count);

		for (Ref<TriangleMesh> collision_mesh : collision_meshes) {
			if (collision_mesh.is_valid()) {
				if (collision_mesh->inside_convex_shape(transformed_frustum.ptr(), plane_count,
						convex_points.ptr(), convex_points.size(), mesh_scale)) {
					return true;
				}
			}
		}
	}

	return false;
}

void EditorNode3DGizmo::handles_intersect_ray(
	Camera3D* p_camera, const Vector2& p_point, bool p_shift_pressed, int& r_id, bool& r_secondary)
{
	r_id = -1;
	r_secondary = false;

	ERR_FAIL_NULL(spatial_node);
	ERR_FAIL_COND(!valid);

	if (hidden) {
		return;
	}

	Transform3D camera_xform = p_camera->get_global_transform();
	Transform3D t = spatial_node->get_global_transform();
	if (billboard_handle) {
		t.set_look_at(t.origin, t.origin - camera_xform.basis.get_column(2),
			camera_xform.basis.get_column(1));
	}

	float min_d = 1e20;

	for (int i = 0; i < secondary_handles.size(); i++) {
		Vector3 hpos = t.xform(secondary_handles[i]);
		Vector2 p = p_camera->unproject_position(hpos);

		if (p.distance_to(p_point) < HANDLE_HALF_SIZE) {
			real_t dp = p_camera->get_transform().origin.distance_to(hpos);
			if (dp < min_d) {
				min_d = dp;
				if (secondary_handle_ids.is_empty()) {
					r_id = i;
				}
				else {
					r_id = secondary_handle_ids[i];
				}
				r_secondary = true;
			}
		}
	}

	if (r_id != -1 && p_shift_pressed) {
		return;
	}

	min_d = 1e20;

	for (int i = 0; i < handles.size(); i++) {
		Vector3 hpos = t.xform(handles[i]);
		Vector2 p = p_camera->unproject_position(hpos);

		if (p.distance_to(p_point) < HANDLE_HALF_SIZE) {
			real_t dp = p_camera->get_transform().origin.distance_to(hpos);
			if (dp < min_d) {
				min_d = dp;
				if (handle_ids.is_empty()) {
					r_id = i;
				}
				else {
					r_id = handle_ids[i];
				}
				r_secondary = false;
			}
		}
	}
}

bool EditorNode3DGizmo::intersect_ray(
	Camera3D* p_camera, const Point2& p_point, Vector3& r_pos, Vector3& r_normal)
{
	ERR_FAIL_NULL_V(spatial_node, false);
	ERR_FAIL_COND_V(!valid, false);

	if (hidden && !gizmo_plugin->is_selectable_when_hidden()) {
		return false;
	}

	if (selectable_icon_size > 0.0f) {
		Transform3D t = spatial_node->get_global_transform();
		Vector3 camera_position = p_camera->get_camera_transform().origin;
		if (!camera_position.is_equal_approx(t.origin)) {
			t.set_look_at(t.origin, camera_position);
		}

		float scale = t.origin.distance_to(p_camera->get_camera_transform().origin);

		if (p_camera->get_projection() == Camera3D::PROJECTION_ORTHOGONAL) {
			float aspect = p_camera->get_viewport()->get_visible_rect().size.aspect();
			float size = p_camera->get_size();
			scale = size / aspect;
		}

		Point2 center = p_camera->unproject_position(t.origin);

		Transform3D orig_camera_transform = p_camera->get_camera_transform();

		if (!orig_camera_transform.origin.is_equal_approx(t.origin) &&
			Math::abs(
				orig_camera_transform.basis.get_column(Vector3::AXIS_Z).dot(Vector3(0, 1, 0))) <
				0.99) {
			p_camera->look_at(t.origin);
		}

		Vector3 c0 = t.xform(Vector3(selectable_icon_size, selectable_icon_size, 0) * scale);
		Vector3 c1 = t.xform(Vector3(-selectable_icon_size, -selectable_icon_size, 0) * scale);

		Point2 p0 = p_camera->unproject_position(c0);
		Point2 p1 = p_camera->unproject_position(c1);

		p_camera->set_global_transform(orig_camera_transform);

		Rect2 rect(p0, (p1 - p0).abs());

		rect.set_position(center - rect.get_size() / 2.0);

		if (rect.has_point(p_point)) {
			r_pos = t.origin;
			r_normal = -p_camera->project_ray_normal(p_point);
			return true;
		}
	}

	if (collision_segments.size()) {
		Plane camp(-p_camera->get_transform().basis.get_column(2).normalized(),
			p_camera->get_transform().origin);

		int vc = collision_segments.size();
		const Vector3* vptr = collision_segments.ptr();
		Transform3D t = spatial_node->get_global_transform();
		if (billboard_handle) {
			t.set_look_at(t.origin, t.origin - p_camera->get_transform().basis.get_column(2),
				p_camera->get_transform().basis.get_column(1));
		}

		Vector3 cp;
		float cpd = 1e20;

		for (int i = 0; i < vc / 2; i++) {
			const Vector3 a = t.xform(vptr[i * 2 + 0]);
			const Vector3 b = t.xform(vptr[i * 2 + 1]);
			const Vector2 segment_a = p_camera->unproject_position(a);
			const Vector2 segment_b = p_camera->unproject_position(b);

			Vector2 p = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);

			float pd = p.distance_to(p_point);

			if (pd < cpd) {
				float d = segment_a.distance_to(segment_b);
				Vector3 tcp;
				if (d > 0) {
					float d2 = segment_a.distance_to(p) / d;
					tcp = a + (b - a) * d2;

				}
				else {
					tcp = a;
				}

				if (camp.distance_to(tcp) < p_camera->get_near()) {
					continue;
				}
				cp = tcp;
				cpd = pd;
			}
		}

		if (cpd < 8) {
			r_pos = cp;
			r_normal = -p_camera->project_ray_normal(p_point);
			return true;
		}
	}

	if (!collision_meshes.is_empty()) {
		Transform3D gt = spatial_node->get_global_transform();

		if (billboard_handle) {
			gt.set_look_at(gt.origin, gt.origin - p_camera->get_transform().basis.get_column(2),
				p_camera->get_transform().basis.get_column(1));
		}

		Transform3D ai = gt.affine_inverse();
		Vector3 ray_from = ai.xform(p_camera->project_ray_origin(p_point));
		Vector3 ray_dir = ai.basis.xform(p_camera->project_ray_normal(p_point)).normalized();
		Vector3 rpos, rnorm;

		for (Ref<TriangleMesh> collision_mesh : collision_meshes) {
			if (collision_mesh.is_valid()) {
				if (collision_mesh->intersect_ray(ray_from, ray_dir, rpos, rnorm)) {
					r_pos = gt.xform(rpos);
					r_normal = gt.basis.xform(rnorm).normalized();
					return true;
				}
			}
		}
	}

	return false;
}

bool EditorNode3DGizmo::is_subgizmo_selected(int p_id) const
{
	Node3DEditor* ed = Node3DEditor::get_singleton();
	ERR_FAIL_NULL_V(ed, false);
	return ed->is_current_selected_gizmo(this) && ed->is_subgizmo_selected(p_id);
}

Vector<int> EditorNode3DGizmo::get_subgizmo_selection() const
{
	Vector<int> ret;

	Node3DEditor* ed = Node3DEditor::get_singleton();
	ERR_FAIL_NULL_V(ed, ret);

	if (ed->is_current_selected_gizmo(this)) {
		ret = ed->get_subgizmo_selection();
	}

	return ret;
}

void EditorNode3DGizmo::create()
{
	ERR_FAIL_NULL(spatial_node);
	ERR_FAIL_COND(valid);
	valid = true;

	for (int i = 0; i < instances.size(); i++) {
		instances.write[i].create_instance(spatial_node, hidden);
	}

	bvh_node_id = Node3DEditor::get_singleton()->insert_gizmo_bvh_node(
		spatial_node, AABB(spatial_node->get_position(), Vector3(0, 0, 0)));

	transform();
}

void EditorNode3DGizmo::transform()
{
	ERR_FAIL_NULL(spatial_node);
	ERR_FAIL_COND(!valid);
	for (int i = 0; i < instances.size(); i++) {
		RS::get_singleton()->instance_set_transform(
			instances[i].instance, spatial_node->get_global_transform() * instances[i].xform);
	}

	_update_bvh();
}

void EditorNode3DGizmo::free()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	ERR_FAIL_NULL(spatial_node);
	ERR_FAIL_COND(!valid);

	for (int i = 0; i < instances.size(); i++) {
		if (instances[i].instance.is_valid()) {
			RS::get_singleton()->free_rid(instances[i].instance);
		}
		instances.write[i].instance = RID();
	}

	clear();

	Node3DEditor::get_singleton()->remove_gizmo_bvh_node(bvh_node_id);
	bvh_node_id = DynamicBVH::ID();

	valid = false;
}

void EditorNode3DGizmo::set_hidden(bool p_hidden)
{
	hidden = p_hidden;
	int layer = hidden ? 0 : 1 << Node3DEditorViewport::GIZMO_EDIT_LAYER;
	for (int i = 0; i < instances.size(); ++i) {
		RS::get_singleton()->instance_set_layer_mask(instances[i].instance, layer);
	}
}

void EditorNode3DGizmo::set_plugin(EditorNode3DGizmoPlugin* p_plugin) { gizmo_plugin = p_plugin; }

void EditorNode3DGizmo::_bind_methods() {}

EditorNode3DGizmo::EditorNode3DGizmo()
{
	valid = false;
	billboard_handle = false;
	hidden = false;
	selected = false;
	spatial_node = nullptr;
	gizmo_plugin = nullptr;
	selectable_icon_size = -1.0f;
}

EditorNode3DGizmo::~EditorNode3DGizmo()
{
	if (gizmo_plugin != nullptr) {
		gizmo_plugin->unregister_gizmo(this);
	}
	clear();
}

/////

void EditorNode3DGizmoPlugin::create_handle_material(
	const String& p_name, bool p_billboard, const Ref<Texture2D>& p_icon)
{
	Ref<StandardMaterial3D> handle_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));

	handle_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	handle_material->set_flag(StandardMaterial3D::FLAG_USE_POINT_SIZE, true);
	Ref<Texture2D> handle_t = p_icon.is_valid()
								  ? p_icon
								  : EditorNode::get_singleton()->get_editor_theme()->get_icon(
										SNAME("Editor3DHandle"), EditorStringName(EditorIcons));
	handle_material->set_point_size(handle_t->get_width());
	handle_material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, handle_t);
	handle_material->set_albedo(Color(1, 1, 1));
	handle_material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	handle_material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	handle_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	handle_material->set_on_top_of_alpha();
	if (p_billboard) {
		handle_material->set_billboard_mode(StandardMaterial3D::BILLBOARD_ENABLED);
		handle_material->set_on_top_of_alpha();
	}
	handle_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	materials[p_name] = Vector<Ref<StandardMaterial3D>>();
	materials[p_name].push_back(handle_material);
}

void EditorNode3DGizmoPlugin::add_material(const String& p_name, Ref<StandardMaterial3D> p_material)
{
	materials[p_name] = Vector<Ref<StandardMaterial3D>>();
	materials[p_name].push_back(p_material);
}

Ref<StandardMaterial3D> EditorNode3DGizmoPlugin::get_material(
	const String& p_name, const Ref<EditorNode3DGizmo>& p_gizmo)
{
	ERR_FAIL_COND_V(!materials.has(p_name), Ref<StandardMaterial3D>());
	ERR_FAIL_COND_V(materials[p_name].is_empty(), Ref<StandardMaterial3D>());

	if (p_gizmo.is_null() || materials[p_name].size() == 1) {
		return materials[p_name][0];
	}

	int index = (p_gizmo->is_selected() ? 1 : 0) + (p_gizmo->is_editable() ? 2 : 0);

	Ref<StandardMaterial3D> mat = materials[p_name][index];

	bool on_top_mat = mat->get_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST);

	if (!on_top_mat && current_state == ON_TOP && p_gizmo->is_selected()) {
		mat = mat->duplicate();
		mat->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}

	return mat;
}

String EditorNode3DGizmoPlugin::get_gizmo_name() const
{
	WARN_PRINT_ONCE(
		"A 3D editor gizmo has no name defined (it will appear as \"Unnamed Gizmo\" in the \"View "
		"> Gizmos\" menu). To resolve this, override the `_get_gizmo_name()` function to return a "
		"String in the script that extends EditorNode3DGizmoPlugin.");
	return TTR("Unnamed Gizmo");
}

Ref<EditorNode3DGizmo> EditorNode3DGizmoPlugin::create_gizmo(Node3D* p_spatial)
{
	Ref<EditorNode3DGizmo> ret;
	Ref<EditorNode3DGizmo> ref;
	if (has_gizmo(p_spatial)) {
		ref.instantiate();
	}
	return ref;
}

void EditorNode3DGizmoPlugin::set_state(int p_state)
{
	current_state = p_state;
	for (EditorNode3DGizmo* current : current_gizmos) {
		current->set_hidden(current_state == HIDDEN);
	}
}

int EditorNode3DGizmoPlugin::get_state() const { return current_state; }

void EditorNode3DGizmoPlugin::unregister_gizmo(EditorNode3DGizmo* p_gizmo)
{
	current_gizmos.erase(p_gizmo);
}

EditorNode3DGizmoPlugin::EditorNode3DGizmoPlugin() { current_state = VISIBLE; }

EditorNode3DGizmoPlugin::~EditorNode3DGizmoPlugin()
{
	for (EditorNode3DGizmo* current : current_gizmos) {
		current->set_plugin(nullptr);
		current->get_node_3d()->remove_gizmo(current);
	}
	if (Node3DEditor::get_singleton()) {
		Node3DEditor::get_singleton()->update_all_gizmos();
	}
}

bool EditorNode3DGizmoPlugin::can_be_hidden() const { return true; }

bool EditorNode3DGizmoPlugin::is_selectable_when_hidden() const { return false; }

bool EditorNode3DGizmoPlugin::can_commit_handle_on_click() const { return true; }

bool EditorNode3DGizmoPlugin::is_handle_highlighted(
	const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary) const
{
	return false;
}

String EditorNode3DGizmoPlugin::get_handle_name(
	const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary) const
{
	return String();
}

void EditorNode3DGizmoPlugin::begin_handle_action(
	const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary)
{
}

void EditorNode3DGizmoPlugin::set_handle(const EditorNode3DGizmo* p_gizmo, int p_id,
	bool p_secondary, Camera3D* p_camera, const Vector2& p_point)
{
}

int EditorNode3DGizmoPlugin::subgizmos_intersect_ray(
	const EditorNode3DGizmo* p_gizmo, Camera3D* p_camera, const Vector2& p_point) const
{
	return -1;
}

Transform3D EditorNode3DGizmoPlugin::get_subgizmo_transform(
	const EditorNode3DGizmo* p_gizmo, int p_id) const
{
	return Transform3D();
}

void EditorNode3DGizmoPlugin::set_subgizmo_transform(
	const EditorNode3DGizmo* p_gizmo, int p_id, Transform3D p_transform)
{
}

bool EditorNode3DGizmoPlugin ::has_gizmo(Node3D* p_spatial) { return false; }

Ref<EditorNode3DGizmo> EditorNode3DGizmoPlugin::create_gizmo(Node3D* p_spatial) const
{
	return Ref<EditorNode3DGizmo>();
}

int EditorNode3DGizmoPlugin::get_priority() const { return 0; }

void EditorNode3DGizmoPlugin::redraw(EditorNode3DGizmo* p_gizmo) {}

//////


