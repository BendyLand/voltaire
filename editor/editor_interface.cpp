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

bool EditorInterface::is_plugin_enabled(const String& p_plugin) const
{
	return EditorNode::get_singleton()->is_addon_plugin_enabled(p_plugin);
}

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

bool EditorInterface::is_distraction_free_mode_enabled() const
{
	return EditorNode::get_singleton()->is_distraction_free_mode_enabled();
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

FileSystemDock* EditorInterface::get_file_system_dock() const
{
	return FileSystemDock::get_singleton();
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

void EditorInterface::mark_scene_as_unsaved()
{
	EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(
		EditorNode::get_editor_data().get_current_edited_scene_history_id());
	EditorSceneTabs::get_singleton()->update_scene_tabs();
}

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


