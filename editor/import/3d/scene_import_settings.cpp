/**************************************************************************/
/*  scene_import_settings.cpp                                             */
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

#include "core/config/project_settings.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/scene/3d/skeleton_3d_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/timer.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/sky.h"
#include "scene/resources/surface_tool.h"
#include "scene_import_settings.h"
#include "servers/display/display_server.h"

class SceneImportSettingsData
{
private:
	friend class SceneImportSettingsDialog;
	List<ResourceImporter::ImportOption> options;
	Vector<String> animation_list;

	bool hide_options = false;
	String path;

	ResourceImporterScene::InternalImportCategory category =
		ResourceImporterScene::INTERNAL_IMPORT_CATEGORY_MAX;
};

void SceneImportSettingsDialog::_update_scene()
{
	scene_tree->clear();
	material_tree->clear();
	mesh_tree->clear();

	// Hidden roots.
	material_tree->create_item();
	mesh_tree->create_item();

	_fill_scene(scene, nullptr);
}

void SceneImportSettingsDialog::request_generate_collider() { generate_collider = true; }

SceneImportSettingsDialog* SceneImportSettingsDialog::singleton = nullptr;

SceneImportSettingsDialog* SceneImportSettingsDialog::get_singleton() { return singleton; }

Node* SceneImportSettingsDialog::get_selected_node()
{
	if (selected_id == "") {
		return nullptr;
	}
	return node_map[selected_id].node;
}

void SceneImportSettingsDialog::_reset_bone_transforms()
{
	for (Skeleton3D* skeleton : skeletons) {
		skeleton->reset_bone_poses();
	}
}

void SceneImportSettingsDialog::_skeleton_tree_entered(Skeleton3D* p_skeleton)
{
	bones_mesh_preview->set_skeleton_path(p_skeleton->get_path());
	Ref<Skin> skin = p_skeleton->create_skin_from_rest_transforms();
	p_skeleton->register_skin(skin);
	bones_mesh_preview->set_skin(skin);
}

void SceneImportSettingsDialog::_animation_update_skeleton_visibility()
{
	if (animation_toggle_skeleton_visibility->is_pressed()) {
		bones_mesh_preview->show();
	}
	else {
		bones_mesh_preview->hide();
	}
}

void SceneImportSettingsDialog::_on_light_1_switch_pressed()
{
	light1->set_visible(light_1_switch->is_pressed());
}

void SceneImportSettingsDialog::_on_light_2_switch_pressed()
{
	light2->set_visible(light_2_switch->is_pressed());
}

void SceneImportSettingsDialog::_on_light_rotate_switch_pressed()
{
	bool light_top_level = !light_rotate_switch->is_pressed();
	light1->set_as_top_level_keep_local(light_top_level);
	light2->set_as_top_level_keep_local(light_top_level);
}

void SceneImportSettingsDialog::_update_theme_item_cache()
{
	ConfirmationDialog::_update_theme_item_cache();
	theme_cache.light_1_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight1"));
	theme_cache.light_2_icon = get_editor_theme_icon(SNAME("MaterialPreviewLight2"));
	theme_cache.rotate_icon = get_editor_theme_icon(SNAME("PreviewRotate"));
}

void SceneImportSettingsDialog::_menu_callback(int p_id)
{
	switch (p_id) {
	case ACTION_EXTRACT_MATERIALS: {
		save_path->set_title(TTR("Select folder to extract material resources"));
		external_extension_type->select(0);
	} break;
	case ACTION_CHOOSE_MESH_SAVE_PATHS: {
		save_path->set_title(TTR("Select folder where mesh resources will save on import"));
		external_extension_type->select(1);
	} break;
	case ACTION_CHOOSE_ANIMATION_SAVE_PATHS: {
		save_path->set_title(TTR("Select folder where animations will save on import"));
		external_extension_type->select(1);
	} break;
	}

	save_path->set_current_dir(base_path.get_base_dir());
	current_action = p_id;
	save_path->popup_centered_ratio();
}

SceneImportSettingsDialog::~SceneImportSettingsDialog() { memdelete(scene_import_settings_data); }


