/**************************************************************************/
/*  editor_interface.cpp                                                  */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_preview_plugins.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/property_selector.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/2d/scene_paint_2d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_interface.h"
#include "main/main.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/theme.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

EditorInterface* EditorInterface::singleton = nullptr;

bool EditorInterface::is_exiting() const { return EditorNode::get_singleton()->is_exiting(); }

void EditorInterface::restart_editor(bool p_save)
{
	if (p_save) {
		EditorNode::get_singleton()->save_all_scenes();
	}
	EditorNode::get_singleton()->restart_editor();
}

// Editor tools.

EditorCommandPalette* EditorInterface::get_command_palette() const
{
	return EditorCommandPalette::get_singleton();
}

EditorFileSystem* EditorInterface::get_resource_filesystem() const
{
	return EditorFileSystem::get_singleton();
}

EditorPaths* EditorInterface::get_editor_paths() const { return EditorPaths::get_singleton(); }

EditorResourcePreview* EditorInterface::get_resource_previewer() const
{
	return EditorResourcePreview::get_singleton();
}

EditorSelection* EditorInterface::get_selection() const
{
	return EditorNode::get_singleton()->get_editor_selection();
}

Ref<EditorSettings> EditorInterface::get_editor_settings() const
{
	return EditorSettings::get_singleton();
}

EditorToaster* EditorInterface::get_editor_toaster() const
{
	return EditorToaster::get_singleton();
}

EditorUndoRedoManager* EditorInterface::get_editor_undo_redo() const
{
	return EditorUndoRedoManager::get_singleton();
}

ScenePaint2DEditor* EditorInterface::get_scene_paint_2d() const
{
	return ScenePaint2DEditor::get_singleton();
}

Vector<Ref<Texture2D>> EditorInterface::make_mesh_previews(
	const Vector<Ref<Mesh>>& p_meshes, Vector<Transform3D>* p_transforms, int p_preview_size)
{
	int size = p_preview_size;

	RID scenario = RS::get_singleton()->scenario_create();

	RID viewport = RS::get_singleton()->viewport_create();
	RS::get_singleton()->viewport_set_update_mode(viewport, RSE::VIEWPORT_UPDATE_ALWAYS);
	RS::get_singleton()->viewport_set_scenario(viewport, scenario);
	RS::get_singleton()->viewport_set_size(viewport, size, size);
	RS::get_singleton()->viewport_set_transparent_background(viewport, true);
	RS::get_singleton()->viewport_set_active(viewport, true);
	RID viewport_texture = RS::get_singleton()->viewport_get_texture(viewport);

	RID camera = RS::get_singleton()->camera_create();
	RS::get_singleton()->viewport_attach_camera(viewport, camera);

	RID light = RS::get_singleton()->directional_light_create();
	RID light_instance = RS::get_singleton()->instance_create2(light, scenario);

	RID light2 = RS::get_singleton()->directional_light_create();
	RS::get_singleton()->light_set_color(light2, Color(0.7, 0.7, 0.7));
	RID light_instance2 = RS::get_singleton()->instance_create2(light2, scenario);

	EditorProgress ep("mlib", TTR("Creating Mesh Previews"), p_meshes.size());

	Vector<Ref<Texture2D>> textures;

	for (int i = 0; i < p_meshes.size(); i++) {
		const Ref<Mesh>& mesh = p_meshes[i];
		if (mesh.is_null()) {
			textures.push_back(Ref<Texture2D>());
			continue;
		}

		Transform3D mesh_xform;
		if (p_transforms != nullptr) {
			mesh_xform = (*p_transforms)[i];
		}

		RID inst = RS::get_singleton()->instance_create2(mesh->get_rid(), scenario);
		RS::get_singleton()->instance_set_transform(inst, mesh_xform);

		AABB aabb = mesh->get_aabb();
		Vector3 ofs = aabb.get_center();
		aabb.position -= ofs;
		Transform3D xform;
		xform.basis = Basis().rotated(Vector3(0, 1, 0), -Math::PI / 6);
		xform.basis = Basis().rotated(Vector3(1, 0, 0), Math::PI / 6) * xform.basis;
		AABB rot_aabb = xform.xform(aabb);
		float m = MAX(rot_aabb.size.x, rot_aabb.size.y) * 0.5;
		if (m == 0) {
			textures.push_back(Ref<Texture2D>());
			continue;
		}
		xform.origin = -xform.basis.xform(ofs); //-ofs*m;
		xform.origin.z -= rot_aabb.size.z * 2;
		xform.invert();
		xform = mesh_xform * xform;

		RS::get_singleton()->camera_set_transform(
			camera, xform * Transform3D(Basis(), Vector3(0, 0, 3)));
		RS::get_singleton()->camera_set_orthogonal(camera, m * 2, 0.01, 1000.0);

		RS::get_singleton()->instance_set_transform(light_instance,
			xform * Transform3D().looking_at(Vector3(-2, -1, -1), Vector3(0, 1, 0)));
		RS::get_singleton()->instance_set_transform(light_instance2,
			xform * Transform3D().looking_at(Vector3(+1, -1, -2), Vector3(0, 1, 0)));

		ep.step(TTR("Thumbnail..."), i);
		DisplayServer::get_singleton()->process_events();
		Main::iteration();
		Main::iteration();
		Ref<Image> img = RS::get_singleton()->texture_2d_get(viewport_texture);
		ERR_CONTINUE(img.is_null() || img->is_empty());
		Ref<ImageTexture> it = ImageTexture::create_from_image(img);

		RS::get_singleton()->free_rid(inst);

		textures.push_back(it);
	}

	RS::get_singleton()->free_rid(viewport);
	RS::get_singleton()->free_rid(light);
	RS::get_singleton()->free_rid(light_instance);
	RS::get_singleton()->free_rid(light2);
	RS::get_singleton()->free_rid(light_instance2);
	RS::get_singleton()->free_rid(camera);
	RS::get_singleton()->free_rid(scenario);

	return textures;
}

void EditorInterface::add_root_node(Node* p_node)
{
	if (EditorNode::get_singleton()->get_edited_scene()) {
		ERR_PRINT("EditorInterface::add_root_node: The current scene already has a root node.");
		return;
	}

	const String& scene_path = p_node->get_scene_file_path();
	if (!scene_path.is_empty()) {
		Ref<PackedScene> scene = ResourceLoader::load(scene_path);
		if (scene.is_valid()) {
			memfree(scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE)); // Ensure node cache.

			p_node->set_scene_inherited_state(scene->get_state());
			p_node->set_scene_file_path(String());
		}
	}

	EditorNode::get_singleton()->set_edited_scene(p_node);
	EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(
		EditorNode::get_editor_data().get_current_edited_scene_history_id());
	EditorSceneTabs::get_singleton()->update_scene_tabs();
}

void EditorInterface::set_plugin_enabled(const String& p_plugin, bool p_enabled)
{
	EditorNode::get_singleton()->set_addon_plugin_enabled(p_plugin, p_enabled, true);
}

bool EditorInterface::is_plugin_enabled(const String& p_plugin) const
{
	return EditorNode::get_singleton()->is_addon_plugin_enabled(p_plugin);
}

// Editor GUI.

Ref<Theme> EditorInterface::get_editor_theme() const
{
	return EditorNode::get_singleton()->get_editor_theme();
}

Control* EditorInterface::get_base_control() const
{
	return EditorNode::get_singleton()->get_gui_base();
}

VBoxContainer* EditorInterface::get_editor_main_screen() const
{
	return EditorNode::get_singleton()->get_editor_main_screen()->get_control();
}

ScriptEditor* EditorInterface::get_script_editor() const { return ScriptEditor::get_singleton(); }

SubViewport* EditorInterface::get_editor_viewport_2d() const
{
	return EditorNode::get_singleton()->get_scene_root();
}

SubViewport* EditorInterface::get_editor_viewport_3d(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, static_cast<int>(Node3DEditor::VIEWPORTS_COUNT), nullptr);
	return Node3DEditor::get_singleton()->get_editor_viewport(p_idx)->get_viewport_node();
}

void EditorInterface::set_main_screen_editor(const String& p_name)
{
	EditorNode::get_singleton()->get_editor_main_screen()->select_by_name(p_name);
}

void EditorInterface::set_distraction_free_mode(bool p_enter)
{
	EditorNode::get_singleton()->set_distraction_free_mode(p_enter);
}

bool EditorInterface::is_distraction_free_mode_enabled() const
{
	return EditorNode::get_singleton()->is_distraction_free_mode_enabled();
}

bool EditorInterface::is_multi_window_enabled() const
{
	return EditorNode::get_singleton()->is_multi_window_enabled();
}

float EditorInterface::get_editor_scale() const { return EDSCALE; }

String EditorInterface::get_editor_language() const
{
	return EditorSettings::get_singleton()->get_language();
}

bool EditorInterface::is_node_3d_snap_enabled() const
{
	return Node3DEditor::get_singleton()->is_snap_enabled();
}

real_t EditorInterface::get_node_3d_translate_snap() const
{
	return Node3DEditor::get_singleton()->get_translate_snap();
}

real_t EditorInterface::get_node_3d_rotate_snap() const
{
	return Node3DEditor::get_singleton()->get_rotate_snap();
}

real_t EditorInterface::get_node_3d_scale_snap() const
{
	return Node3DEditor::get_singleton()->get_scale_snap();
}

void EditorInterface::popup_dialog(Window* p_dialog, const Rect2i& p_screen_rect)
{
	p_dialog->popup_exclusive(EditorNode::get_singleton(), p_screen_rect);
}

void EditorInterface::popup_dialog_centered(Window* p_dialog, const Size2i& p_minsize)
{
	p_dialog->popup_exclusive_centered(EditorNode::get_singleton(), p_minsize);
}

void EditorInterface::popup_dialog_centered_ratio(Window* p_dialog, float p_ratio)
{
	p_dialog->popup_exclusive_centered_ratio(EditorNode::get_singleton(), p_ratio);
}

void EditorInterface::popup_dialog_centered_clamped(
	Window* p_dialog, const Size2i& p_size, float p_fallback_ratio)
{
	p_dialog->popup_exclusive_centered_clamped(
		EditorNode::get_singleton(), p_size, p_fallback_ratio);
}

String EditorInterface::get_current_feature_profile() const
{
	return EditorFeatureProfileManager::get_singleton()->get_current_profile_name();
}

void EditorInterface::set_current_feature_profile(const String& p_profile_name)
{
	EditorFeatureProfileManager::get_singleton()->set_current_profile(p_profile_name, true);
}

// Editor dialogs.

// Editor docks.

FileSystemDock* EditorInterface::get_file_system_dock() const
{
	return FileSystemDock::get_singleton();
}

void EditorInterface::select_file(const String& p_file)
{
	FileSystemDock::get_singleton()->select_file(p_file);
}

Vector<String> EditorInterface::get_selected_paths() const
{
	return FileSystemDock::get_singleton()->get_selected_paths();
}

String EditorInterface::get_current_path() const
{
	return FileSystemDock::get_singleton()->get_current_path();
}

String EditorInterface::get_current_directory() const
{
	return FileSystemDock::get_singleton()->get_current_directory();
}

EditorInspector* EditorInterface::get_inspector() const
{
	return InspectorDock::get_inspector_singleton();
}

// Object/Resource/Node editing.

void EditorInterface::edit_resource(const Ref<Resource>& p_resource)
{
	EditorNode::get_singleton()->edit_resource(p_resource);
}

void EditorInterface::open_scene_from_path(const String& scene_path, bool p_set_inherited)
{
	if (EditorNode::get_singleton()->is_changing_scene()) {
		return;
	}
	EditorNode::get_singleton()->open_scene(scene_path, false, p_set_inherited);
}

void EditorInterface::reload_scene_from_path(const String& scene_path)
{
	if (EditorNode::get_singleton()->is_changing_scene()) {
		return;
	}

	EditorNode::get_singleton()->reload_scene(scene_path);
}

Node* EditorInterface::get_edited_scene_root() const
{
	return EditorNode::get_singleton()->get_edited_scene();
}

PackedStringArray EditorInterface::get_open_scenes() const
{
	PackedStringArray ret;
	Vector<EditorData::EditedScene> scenes = EditorNode::get_editor_data().get_edited_scenes();

	for (EditorData::EditedScene& edited_scene : scenes) {
		ret.push_back(edited_scene.path);
	}
	return ret;
}

PackedStringArray EditorInterface::get_unsaved_scenes() const
{
	PackedStringArray ret;
	Vector<EditorData::EditedScene> scenes = EditorNode::get_editor_data().get_edited_scenes();

	for (int i = 0; i < scenes.size(); i++) {
		if (EditorNode::get_singleton()->is_scene_unsaved(i)) {
			ret.push_back(scenes[i].path);
		}
	}
	return ret;
}

Vector<Node*> EditorInterface::get_open_scene_roots() const
{
	Vector<Node*> ret;
	Vector<EditorData::EditedScene> scenes = EditorNode::get_editor_data().get_edited_scenes();

	for (EditorData::EditedScene& edited_scene : scenes) {
		if (edited_scene.root == nullptr) {
			continue;
		}
		ret.push_back(edited_scene.root);
	}
	return ret;
}

Error EditorInterface::save_scene()
{
	if (!get_edited_scene_root()) {
		return ERR_CANT_CREATE;
	}
	if (get_edited_scene_root()->get_scene_file_path().is_empty()) {
		return ERR_CANT_CREATE;
	}

	save_scene_as(get_edited_scene_root()->get_scene_file_path());
	return OK;
}

void EditorInterface::mark_scene_as_unsaved()
{
	EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(
		EditorNode::get_editor_data().get_current_edited_scene_history_id());
	EditorSceneTabs::get_singleton()->update_scene_tabs();
}

void EditorInterface::save_all_scenes() { EditorNode::get_singleton()->save_all_scenes(); }

Error EditorInterface::close_scene()
{
	return EditorNode::get_singleton()->close_scene() ? OK : ERR_DOES_NOT_EXIST;
}

// Scene playback.

void EditorInterface::play_main_scene() { EditorRunBar::get_singleton()->play_main_scene(); }

void EditorInterface::play_current_scene() { EditorRunBar::get_singleton()->play_current_scene(); }

void EditorInterface::play_custom_scene(const String& scene_path)
{
	EditorRunBar::get_singleton()->play_custom_scene(scene_path);
}

void EditorInterface::stop_playing_scene() { EditorRunBar::get_singleton()->stop_playing(); }

bool EditorInterface::is_playing_scene() const
{
	return EditorRunBar::get_singleton()->is_playing();
}

String EditorInterface::get_playing_scene() const
{
	return EditorRunBar::get_singleton()->get_playing_scene();
}

void EditorInterface::set_movie_maker_enabled(bool p_enabled)
{
	EditorRunBar::get_singleton()->set_movie_maker_enabled(p_enabled);
}

bool EditorInterface::is_movie_maker_enabled() const
{
	return EditorRunBar::get_singleton()->is_movie_maker_enabled();
}

void EditorInterface::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "set_main_screen_editor") {
			for (String E : {"\"2D\"", "\"3D\"", "\"Script\"", "\"Game\"", "\"AssetLib\""}) {
				r_options->push_back(E);
			}
		}
		else if (pf == "get_editor_viewport_3d") {
			for (uint32_t i = 0; i < Node3DEditor::VIEWPORTS_COUNT; i++) {
				r_options->push_back(String::num_int64(i));
			}
		}
	}
}

// Base.

void EditorInterface::_bind_methods() {}

void EditorInterface::create() { memnew(EditorInterface); }

void EditorInterface::free()
{
	ERR_FAIL_NULL(singleton);
	memdelete(singleton);
}

EditorInterface::EditorInterface()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}


