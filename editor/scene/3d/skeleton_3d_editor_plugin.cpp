/**************************************************************************/
/*  skeleton_3d_editor_plugin.cpp                                         */
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
#include "core/io/resource_saver.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_properties_vector.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/physical_bone_3d.h"
#include "scene/3d/physics/physical_bone_simulator_3d.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/skeleton_profile.h"
#include "scene/resources/surface_tool.h"
#include "skeleton_3d_editor_plugin.h"

void BonePropertiesEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		const Color section_color =
			get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor));
		section->set_bg_color(section_color);
		rest_section->set_bg_color(section_color);
	} break;
	}
}

BonePropertiesEditor::BonePropertiesEditor(Skeleton3D* p_skeleton)
{
	create_editors();
	set_skeleton(p_skeleton);
}

void BonePropertiesEditor::set_keyable(const bool p_keyable)
{
	position_property->set_keying(p_keyable);
	rotation_property->set_keying(p_keyable);
	scale_property->set_keying(p_keyable);
}

Skeleton3DEditor* Skeleton3DEditor::singleton = nullptr;

void Skeleton3DEditor::set_keyable(const bool p_keyable)
{
	keyable = p_keyable;
	if (p_keyable) {
		animation_hb->show();
	}
	else {
		animation_hb->hide();
	}
}

void Skeleton3DEditor::set_bone_options_enabled(const bool p_bone_options_enabled)
{
	skeleton_options->get_popup()->set_item_disabled(
		SKELETON_OPTION_RESET_SELECTED_POSES, !p_bone_options_enabled);
	skeleton_options->get_popup()->set_item_disabled(
		SKELETON_OPTION_SELECTED_POSES_TO_RESTS, !p_bone_options_enabled);
}

void Skeleton3DEditor::_bind_methods() {}

void Skeleton3DEditor::_on_click_skeleton_option(int p_skeleton_option)
{
	ERR_FAIL_COND(!skeleton);

	switch (p_skeleton_option) {
	case SKELETON_OPTION_RESET_ALL_POSES: {
		reset_pose(true);
		break;
	}
	case SKELETON_OPTION_RESET_SELECTED_POSES: {
		reset_pose(false);
		break;
	}
	case SKELETON_OPTION_ALL_POSES_TO_RESTS: {
		pose_to_rest(true);
		break;
	}
	case SKELETON_OPTION_SELECTED_POSES_TO_RESTS: {
		pose_to_rest(false);
		break;
	}
	case SKELETON_OPTION_CREATE_PHYSICAL_SKELETON: {
		create_physical_skeleton();
		break;
	}
	case SKELETON_OPTION_EXPORT_SKELETON_PROFILE: {
		export_skeleton_profile();
		break;
	}
	}
}

PhysicalBone3D* Skeleton3DEditor::create_physical_bone(
	int bone_id, int bone_child_id, const Vector<BoneInfo>& bones_infos)
{
	const Transform3D child_rest = skeleton->get_bone_rest(bone_child_id);

	const real_t half_height(child_rest.origin.length() * 0.5);
	const real_t radius(half_height * 0.2);

	Ref<CapsuleShape3D> bone_shape_capsule;
	bone_shape_capsule.instantiate();
	bone_shape_capsule->set_height(half_height * 2);
	bone_shape_capsule->set_radius(radius);

	CollisionShape3D* bone_shape = memnew(CollisionShape3D);
	bone_shape->set_shape(bone_shape_capsule);
	bone_shape->set_name("CollisionShape3D");

	Transform3D capsule_transform;
	capsule_transform.basis.rows[0] = Vector3(1, 0, 0);
	capsule_transform.basis.rows[1] = Vector3(0, 0, 1);
	capsule_transform.basis.rows[2] = Vector3(0, -1, 0);
	bone_shape->set_transform(capsule_transform);

	/// Get an up vector not collinear with child rest origin
	Vector3 up = Vector3(0, 1, 0);
	if (up.cross(child_rest.origin).is_zero_approx()) {
		up = Vector3(0, 0, 1);
	}

	Transform3D body_transform;
	body_transform.basis = Basis::looking_at(child_rest.origin, up);
	body_transform.origin = body_transform.basis.xform(Vector3(0, 0, -half_height));

	Transform3D joint_transform;
	joint_transform.origin = Vector3(0, 0, half_height);

	PhysicalBone3D* physical_bone = memnew(PhysicalBone3D);
	physical_bone->add_child(bone_shape);
	physical_bone->set_name("Physical Bone " + skeleton->get_bone_name(bone_id));
	physical_bone->set_body_offset(body_transform);
	physical_bone->set_joint_offset(joint_transform);
	return physical_bone;
}

void Skeleton3DEditor::export_skeleton_profile()
{
	if (!skeleton->get_bone_count()) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Cannot export a SkeletonProfile for a Skeleton3D node with no bones.")));
		return;
	}

	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_title(TTR("Export Skeleton Profile As..."));

	List<String> exts;
	ResourceLoader::get_recognized_extensions_for_type("SkeletonProfile", &exts);
	file_dialog->clear_filters();
	for (const String& K : exts) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->popup_file_dialog();
}

void Skeleton3DEditor::_file_selected(const String& p_file)
{
	// Export SkeletonProfile.
	Ref<SkeletonProfile> sp(memnew(SkeletonProfile));

	// Build SkeletonProfile.
	sp->set_group_size(1);

	Vector<Vector2> handle_positions;
	Vector2 position_max;
	Vector2 position_min;

	const int bone_count = skeleton->get_bone_count();
	sp->set_bone_size(bone_count);
	for (int i = 0; i < bone_count; i++) {
		sp->set_bone_name(i, skeleton->get_bone_name(i));
		int parent = skeleton->get_bone_parent(i);
		if (parent >= 0) {
			sp->set_bone_parent(i, skeleton->get_bone_name(parent));
		}
		sp->set_reference_pose(i, skeleton->get_bone_rest(i));

		Transform3D grest = skeleton->get_bone_global_rest(i);
		handle_positions.append(Vector2(grest.origin.x, grest.origin.y));
		if (i == 0) {
			position_max = Vector2(grest.origin.x, grest.origin.y);
			position_min = Vector2(grest.origin.x, grest.origin.y);
		}
		else {
			position_max = position_max.max(Vector2(grest.origin.x, grest.origin.y));
			position_min = position_min.min(Vector2(grest.origin.x, grest.origin.y));
		}
	}

	// Layout handles provisionaly.
	Vector2 bound = Vector2(position_max.x - position_min.x, position_max.y - position_min.y);
	Vector2 center =
		Vector2((position_max.x + position_min.x) * 0.5, (position_max.y + position_min.y) * 0.5);
	float nrm = MAX(bound.x, bound.y);
	if (nrm > 0) {
		for (int i = 0; i < bone_count; i++) {
			handle_positions.write[i] = (handle_positions[i] - center) / nrm * 0.9;
			sp->set_handle_offset(
				i, Vector2(0.5 + handle_positions[i].x, 0.5 - handle_positions[i].y));
		}
	}

	Error err = ResourceSaver::save(sp.ptr(), p_file);

	if (err != OK) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Error saving file: %s"), p_file));
		return;
	}
}

// May be not used with single select mode.
void Skeleton3DEditor::_joint_tree_rmb_select(const Vector2& p_pos, MouseButton p_button) {}

void Skeleton3DEditor::_update_properties()
{
	if (pose_editor) {
		pose_editor->_update_properties();
	}
	if (!skeleton || !skeleton->is_inside_tree()) {
		return;
	}
	Node3DEditor::get_singleton()->update_transform_gizmo();
}

void Skeleton3DEditor::update_all()
{
	_update_properties();
	update_joint_tree();
}

void Skeleton3DEditor::_loc_toggled(bool p_toggled_on)
{
	if (!editor_plugin) {
		return;
	}
	editor_plugin->loc_pressed = p_toggled_on;
}

void Skeleton3DEditor::_rot_toggled(bool p_toggled_on)
{
	if (!editor_plugin) {
		return;
	}
	editor_plugin->rot_pressed = p_toggled_on;
}

void Skeleton3DEditor::_scl_toggled(bool p_toggled_on)
{
	if (!editor_plugin) {
		return;
	}
	editor_plugin->scl_pressed = p_toggled_on;
}

void Skeleton3DEditor::_node_removed(Node* p_node)
{
	if (!skeleton || p_node != skeleton) {
		return;
	}

	_disconnect_from_tree();
	_disconnect_from_skeleton();
	if (pose_editor) {
		pose_editor->set_skeleton(nullptr);
		pose_editor->set_visible(false);
	}
	edit_mode = false;
	skeleton = nullptr;
	skeleton_options->hide();

	_update_properties();
}

void Skeleton3DEditor::edit_mode_toggled(const bool pressed)
{
	edit_mode = pressed;
	_update_gizmo_visible();
}

void Skeleton3DEditor::update_bone_original()
{
	if (!skeleton) {
		return;
	}
	if (skeleton->get_bone_count() == 0 || selected_bone == -1) {
		return;
	}
	bone_original_position = skeleton->get_bone_pose_position(selected_bone);
	bone_original_rotation = skeleton->get_bone_pose_rotation(selected_bone);
	bone_original_scale = skeleton->get_bone_pose_scale(selected_bone);
}

void Skeleton3DEditor::_hide_handles() { handles_mesh_instance->hide(); }

void Skeleton3DEditor::_draw_gizmo()
{
	if (!skeleton) {
		return;
	}

	// If you call get_bone_global_pose() while drawing the surface, such as toggle rest mode,
	// the skeleton update will be done first and
	// the drawing surface will be interrupted once and an error will occur.
	skeleton->force_update_all_dirty_bones();

	// Handles.
	if (edit_mode) {
		_draw_handles();
	}
	else {
		_hide_handles();
	}
}

void Skeleton3DEditor::_draw_handles()
{
	const int bone_count = skeleton->get_bone_count();

	handles_mesh->clear_surfaces();

	if (bone_count) {
		handles_mesh_instance->show();

		handles_mesh->surface_begin(Mesh::PRIMITIVE_POINTS);

		for (int i = 0; i < bone_count; i++) {
			Color c;
			if (i == selected_bone) {
				c = Color(1, 1, 0);
			}
			else {
				c = Color(0.1, 0.25, 0.8);
			}
			Vector3 point = skeleton->get_bone_global_pose(i).origin;
			handles_mesh->surface_set_color(c);
			handles_mesh->surface_add_vertex(point);
		}
		handles_mesh->surface_end();
		handles_mesh->surface_set_material(0, handle_material);
	}
	else {
		handles_mesh_instance->hide();
	}
}

void Skeleton3DEditor::_subgizmo_selection_change()
{
	if (!skeleton) {
		return;
	}

	// Once validated by subgizmos_intersect_ray, but required if through inspector's bones tree.
	if (!edit_mode) {
		skeleton->clear_subgizmo_selection();
		return;
	}

	int selected = -1;
	Skeleton3DEditor* se = Skeleton3DEditor::get_singleton();
	if (se) {
		selected = se->get_selected_bone();
	}

	if (selected >= 0) {
		Vector<Ref<Node3DGizmo>> gizmos = skeleton->get_gizmos();
		for (int i = 0; i < gizmos.size(); i++) {
			Ref<EditorNode3DGizmo> gizmo = gizmos[i];
			if (gizmo.is_null()) {
				continue;
			}
			Ref<Skeleton3DGizmoPlugin> plugin = gizmo->get_plugin();
			if (plugin.is_null()) {
				continue;
			}
			skeleton->set_subgizmo_selection(
				gizmo, selected, skeleton->get_bone_global_pose(selected));
			break;
		}
	}
	else {
		skeleton->clear_subgizmo_selection();
	}
}

void Skeleton3DEditor::select_bone(int p_idx)
{
	if (p_idx >= 0) {
		TreeItem* ti = _find(joint_tree->get_root(), "bones/" + itos(p_idx));
		if (ti) {
			// Make visible when it's collapsed.
			TreeItem* node = ti->get_parent();
			while (node && node != joint_tree->get_root()) {
				node->set_collapsed(false);
				node = node->get_parent();
			}
			ti->select(0);
			joint_tree->scroll_to_item(ti);
		}
	}
	else {
		selected_bone = -1;
		joint_tree->deselect_all();
		_joint_tree_selection_changed();
	}
}

Skeleton3DEditor::~Skeleton3DEditor()
{
	singleton = nullptr;

	handles_mesh_instance->queue_free();

	Node3DEditor* ne = Node3DEditor::get_singleton();

	ne->remove_control_from_menu_panel(topmenu_bar);
	memdelete(topmenu_bar);
}

Skeleton3DEditorPlugin::Skeleton3DEditorPlugin()
{
	skeleton_plugin = memnew(EditorInspectorPluginSkeleton);

	EditorInspector::add_inspector_plugin(skeleton_plugin);

	Ref<Skeleton3DGizmoPlugin> gizmo_plugin =
		Ref<Skeleton3DGizmoPlugin>(memnew(Skeleton3DGizmoPlugin));
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);
}

EditorPlugin::AfterGUIInput Skeleton3DEditorPlugin::forward_3d_gui_input(
	Camera3D* p_camera, const Ref<InputEvent>& p_event)
{
	Skeleton3DEditor* se = Skeleton3DEditor::get_singleton();
	if (se && se->is_edit_mode()) {
		const Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT && mb->is_pressed()) {
			se->update_bone_original();
		}
		return EditorPlugin::AFTER_GUI_INPUT_CUSTOM;
	}
	return EditorPlugin::AFTER_GUI_INPUT_PASS;
}

void Skeleton3DEditor::_bone_enabled_changed(const int p_bone_id) { _update_gizmo_visible(); }

void Skeleton3DEditor::_update_gizmo_visible()
{
	if (!skeleton) {
		return;
	}

	_subgizmo_selection_change();
	if (edit_mode) {
		if (selected_bone == -1) {
			skeleton->set_transform_gizmo_visible(false);
		}
		else {
			if (skeleton->is_bone_enabled(selected_bone) && !skeleton->is_show_rest_only()) {
				skeleton->set_transform_gizmo_visible(true);
			}
			else {
				skeleton->set_transform_gizmo_visible(false);
			}
		}
	}
	else {
		skeleton->set_transform_gizmo_visible(true);
	}
	_draw_gizmo();
}

int Skeleton3DEditor::get_selected_bone() const { return selected_bone; }

Skeleton3DGizmoPlugin::SelectionMaterials Skeleton3DGizmoPlugin::selection_materials;

Skeleton3DGizmoPlugin::Skeleton3DGizmoPlugin()
{
	selection_materials.unselected_mat.instantiate();
	selection_materials.unselected_mat->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	selection_materials.unselected_mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	selection_materials.unselected_mat->set_flag(
		StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	selection_materials.unselected_mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	selection_materials.unselected_mat->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	selection_materials.selected_mat.instantiate();
	Ref<Shader> selected_sh = Ref<Shader>(memnew(Shader));
	selected_sh->set_code(R"(
// Skeleton 3D gizmo bones shader.

shader_type spatial;
render_mode unshaded, shadows_disabled;

void vertex() {
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)), COLOR.rgb * (1.0 / 12.92), lessThan(COLOR.rgb,vec3(0.04045)));
	}
	VERTEX = VERTEX;
	POSITION = PROJECTION_MATRIX * VIEW_MATRIX * MODEL_MATRIX * vec4(VERTEX.xyz, 1.0);
	POSITION.z = mix(POSITION.z, POSITION.w, 0.998);
}

void fragment() {
	ALBEDO = COLOR.rgb;
	ALPHA = COLO
R.a;
}
)");
	selection_materials.selected_mat->set_shader(selected_sh);
}

Skeleton3DGizmoPlugin::~Skeleton3DGizmoPlugin()
{
	selection_materials.unselected_mat.unref();
	selection_materials.selected_mat.unref();
}

String Skeleton3DGizmoPlugin::get_gizmo_name() const { return "Skeleton3D"; }

int Skeleton3DGizmoPlugin::get_priority() const { return -1; }

int Skeleton3DGizmoPlugin::skeleton_intersect_ray(
	const Skeleton3D* p_skeleton, Camera3D* p_camera, const Vector2& p_point)
{
	real_t grab_threshold = 8 * EDSCALE;
	Vector3 ray_from = p_camera->get_global_transform().origin;
	Transform3D gt = p_skeleton->get_global_transform();
	int closest_idx = -1;
	real_t closest_dist = 1e10;
	const int bone_count = p_skeleton->get_bone_count();
	for (int i = 0; i < bone_count; i++) {
		Vector3 joint_pos_3d = gt.xform(p_skeleton->get_bone_global_pose(i).origin);
		Vector2 joint_pos_2d = p_camera->unproject_position(joint_pos_3d);
		real_t dist_3d = ray_from.distance_to(joint_pos_3d);
		real_t dist_2d = p_point.distance_to(joint_pos_2d);
		if (dist_2d < grab_threshold && dist_3d < closest_dist) {
			closest_dist = dist_3d;
			closest_idx = i;
		}
	}
	return closest_idx;
}


