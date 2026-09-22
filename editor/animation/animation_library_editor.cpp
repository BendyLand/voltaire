/**************************************************************************/
/*  animation_library_editor.cpp                                          */
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

#include "animation_library_editor.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_mixer.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

void AnimationLibraryEditor::_load_library()
{
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("AnimationLibrary", &extensions);

	file_dialog->set_title(TTR("Load Animation"));
	file_dialog->clear_filters();
	for (const String& K : extensions) {
		file_dialog->add_filter("*." + K);
	}

	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILES);
	file_dialog->set_current_file("");
	file_dialog->popup_centered_ratio();

	file_dialog_action = FILE_DIALOG_ACTION_OPEN_LIBRARY;
}

void AnimationLibraryEditor::_save_mixer_lib_folding(TreeItem* p_item)
{
	// Check if ti is a library or animation
	if (p_item->get_parent()->get_parent() != nullptr) {
		return;
	}

	Ref<ConfigFile> config;
	config.instantiate();

	String path =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join("lib_folding.cfg");
	Error err = config->load(path);
	if (err != OK && err != ERR_FILE_NOT_FOUND) {
		ERR_PRINT("Error loading lib_folding.cfg: " + itos(err));
	}

	// Get unique identifier for this scene+mixer combination.
	const String md = (mixer->get_tree()->get_edited_scene_root()->get_scene_file_path() +
					   String(mixer->get_path()))
						  .md5_text();

	Vector<String> collapsed_libs;
	for (int i = collapsed_libs.size() - 1; i >= 0; i--) {
		if (!mixer->has_animation_library(collapsed_libs[i])) {
			collapsed_libs.remove_at(i);
		}
	}

	const String lib_name = p_item->get_text(0);
	if (p_item->is_collapsed()) {
		if (!collapsed_libs.has(lib_name)) {
			collapsed_libs.append(lib_name);
		}
	}
	else {
		collapsed_libs.erase(lib_name);
	}

	// Remove deprecated keys.
	if (config->has_section_key(md, "id")) {
		config->erase_section_key(md, "id");
	}
	if (config->has_section_key(md, "root")) {
		config->erase_section_key(md, "root");
	}
	if (config->has_section_key(md, "mixer")) {
		config->erase_section_key(md, "mixer");
	}

	err = config->save(path);
	if (err != OK) {
		ERR_PRINT("Error saving lib_folding.cfg: " + itos(err));
	}
}

String AnimationLibraryEditor::_get_mixer_signature() const
{
	String signature = String();

	// Get all libraries sorted for consistency
	LocalVector<StringName> libs;
	mixer->get_animation_library_list(&libs);
	libs.sort_custom<StringName::AlphCompare>();

	// Add libraries and their animations to signature
	for (const StringName& lib_name : libs) {
		signature += "::" + String(lib_name);
		Ref<AnimationLibrary> lib = mixer->get_animation_library(lib_name);
		if (lib.is_valid()) {
			LocalVector<StringName> anims;
			lib->get_animation_list(&anims);
			anims.sort_custom<StringName::AlphCompare>();
			for (const StringName& anim_name : anims) {
				signature += "," + String(anim_name);
			}
		}
	}

	return signature.md5_text();
}


