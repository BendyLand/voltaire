/**************************************************************************/
/*  editor_scene_exporter_gltf_plugin.cpp                                 */
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

#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/themes/editor_scale.h"
#include "editor_scene_exporter_gltf_plugin.h"
#include "editor_scene_exporter_gltf_settings.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"

String SceneExporterGLTFPlugin::get_plugin_name() const { return "ConvertGLTF2"; }

SceneExporterGLTFPlugin::SceneExporterGLTFPlugin()
{
	_gltf_document.instantiate();
	// Set up the file dialog.
	_file_dialog = memnew(EditorFileDialog);
	_file_dialog->set_title(TTR("Export Library"));
	_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	_file_dialog->clear_filters();
	_file_dialog->add_filter("*.glb");
	_file_dialog->add_filter("*.gltf");
	_file_dialog->set_title(TTR("Export Scene to glTF 2.0 File"));
	EditorNode::get_singleton()->get_gui_base()->add_child(_file_dialog);
	// Set up the export settings menu.
	_config_dialog = memnew(ConfirmationDialog);
	_config_dialog->set_title(TTRC("Export Settings"));
	EditorNode::get_singleton()->get_gui_base()->add_child(_config_dialog);

	_export_settings.instantiate();
	_export_settings->generate_property_list(_gltf_document);
	_settings_inspector = memnew(EditorInspector);
	_settings_inspector->set_custom_minimum_size(Size2(350, 300) * EDSCALE);
	_config_dialog->add_child(_settings_inspector);
	// Add a button to the Scene -> Export menu to pop up the settings dialog.
	PopupMenu* menu = get_export_as_menu();
	int idx = menu->get_item_count();
}

void SceneExporterGLTFPlugin::_popup_gltf_export_dialog()
{
	Node* root = EditorNode::get_singleton()->get_tree()->get_edited_scene_root();
	if (!root) {
		EditorNode::get_singleton()->show_warning(
			TTR("This operation can't be done without a scene."));
		return;
	}
	// Set the file dialog's file name to the scene name.
	String filename = String(root->get_scene_file_path().get_file().get_basename());
	if (filename.is_empty()) {
		filename = root->get_name();
	}
	_file_dialog->set_current_file(filename + String(".gltf"));
	// Show the file dialog.
	_file_dialog->popup_file_dialog();
}


