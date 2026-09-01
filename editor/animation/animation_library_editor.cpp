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

void AnimationLibraryEditor::_add_library()
{
	add_library_name->set_text("");
	add_library_dialog->popup_centered();
	add_library_name->grab_focus();
	adding_animation = false;
	adding_animation_to_library = StringName();
	_add_library_validate("");
}

void AnimationLibraryEditor::_add_library_validate(const String& p_name)
{
	String error;

	if (adding_animation) {
		Ref<AnimationLibrary> al = mixer->get_animation_library(adding_animation_to_library);
		ERR_FAIL_COND(al.is_null());
		if (p_name == "") {
			error = TTR("Animation name can't be empty.");
		}
		else if (!AnimationLibrary::is_valid_animation_name(p_name)) {
			error = TTR("Animation name contains invalid characters: '/', ':', ',' or '['.");
		}
		else if (al->has_animation(p_name)) {
			error = TTR("Animation with the same name already exists.");
		}
	}
	else {
		if (p_name == "" && mixer->has_animation_library("")) {
			error = TTR("Enter a library name.");
		}
		else if (!AnimationLibrary::is_valid_library_name(p_name)) {
			error = TTR("Library name contains invalid characters: '/', ':', ',' or '['.");
		}
		else if (mixer->has_animation_library(p_name)) {
			error = TTR("Library with the same name already exists.");
		}
	}

	if (error != "") {
		add_library_validate->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		add_library_validate->set_text(error);
		add_library_dialog->get_ok_button()->set_disabled(true);
	}
	else {
		if (adding_animation) {
			add_library_validate->set_text(TTR("Animation name is valid."));
		}
		else {
			if (p_name == "") {
				add_library_validate->set_text(TTR("Global library will be created."));
			}
			else {
				add_library_validate->set_text(TTR("Library name is valid."));
			}
		}
		add_library_validate->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		add_library_dialog->get_ok_button()->set_disabled(false);
	}
}

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

void AnimationLibraryEditor::update_tree()
{
	if (updating) {
		return;
	}

	tree->clear();
	ERR_FAIL_NULL(mixer);

	Color ss_color = get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor));

	TreeItem* root = tree->create_item();
	LocalVector<StringName> libs;
	const Vector<String> collapsed_libs = _load_mixer_libs_folding();

	mixer->get_animation_library_list(&libs);

	for (const StringName& K : libs) {
		TreeItem* libitem = tree->create_item(root);
		libitem->set_text(0, K);
		if (K == StringName()) {
			libitem->set_suffix(0, TTR("[Global]"));
		}
		else {
			libitem->set_suffix(0, "");
		}

		Ref<AnimationLibrary> al = mixer->get_animation_library(K);
		bool animation_library_is_foreign = false;
		String al_path = al->get_path();
		if (!al_path.is_resource_file()) {
			libitem->set_text(1, TTR("[built-in]"));
			libitem->set_tooltip_text(1, al_path);
			int srpos = al_path.find("::");
			if (srpos != -1) {
				String base = al_path.substr(0, srpos);
				if (ResourceLoader::get_resource_type(base) == "PackedScene") {
					if (!get_tree()->get_edited_scene_root() ||
						get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
						animation_library_is_foreign = true;
						libitem->set_text(1, TTR("[foreign]"));
					}
				}
				else {
					if (FileAccess::exists(base + ".import")) {
						animation_library_is_foreign = true;
						libitem->set_text(1, TTR("[imported]"));
					}
				}
			}
		}
		else {
			if (FileAccess::exists(al_path + ".import")) {
				animation_library_is_foreign = true;
				libitem->set_text(1, TTR("[imported]"));
			}
			else {
				libitem->set_text(1, al_path.get_file());
			}
		}

		libitem->set_editable(0, true);
		libitem->set_icon(0, get_editor_theme_icon("AnimationLibrary"));

		libitem->add_button(0, get_editor_theme_icon("Add"), LIB_BUTTON_ADD,
			animation_library_is_foreign, TTR("Add animation to library."));
		libitem->add_button(0, get_editor_theme_icon("Load"), LIB_BUTTON_LOAD,
			animation_library_is_foreign, TTR("Load animation from file and add to library."));
		libitem->add_button(0, get_editor_theme_icon("ActionPaste"), LIB_BUTTON_PASTE,
			animation_library_is_foreign, TTR("Paste animation to library from clipboard."));

		libitem->add_button(1, get_editor_theme_icon("Save"), LIB_BUTTON_FILE, false,
			TTR("Save animation library to resource on disk."));
		libitem->add_button(1, get_editor_theme_icon("Remove"), LIB_BUTTON_DELETE, false,
			TTR("Remove animation library."));

		libitem->set_custom_bg_color(0, ss_color);

		LocalVector<StringName> animations;
		al->get_animation_list(&animations);
		for (const StringName& L : animations) {
			TreeItem* anitem = tree->create_item(libitem);
			anitem->set_text(0, L);
			anitem->set_editable(0, !animation_library_is_foreign);
			anitem->set_icon(0, get_editor_theme_icon("Animation"));
			anitem->add_button(0, get_editor_theme_icon("ActionCopy"), ANIM_BUTTON_COPY,
				animation_library_is_foreign, TTR("Copy animation to clipboard."));

			Ref<Animation> anim = al->get_animation(L);
			String anim_path = anim->get_path();
			if (!anim_path.is_resource_file()) {
				anitem->set_text(1, TTR("[built-in]"));
				anitem->set_tooltip_text(1, anim_path);
				int srpos = anim_path.find("::");
				if (srpos != -1) {
					String base = anim_path.substr(0, srpos);
					if (ResourceLoader::get_resource_type(base) == "PackedScene") {
						if (!get_tree()->get_edited_scene_root() ||
							get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
							anitem->set_text(1, TTR("[foreign]"));
						}
					}
					else {
						if (FileAccess::exists(base + ".import")) {
							anitem->set_text(1, TTR("[imported]"));
						}
					}
				}
			}
			else {
				if (FileAccess::exists(anim_path + ".import")) {
					anitem->set_text(1, TTR("[imported]"));
				}
				else {
					anitem->set_text(1, anim_path.get_file());
				}
			}

			anitem->add_button(1, get_editor_theme_icon("Save"), ANIM_BUTTON_FILE,
				animation_library_is_foreign, TTR("Save animation to resource on disk."));
			anitem->add_button(1, get_editor_theme_icon("Remove"), ANIM_BUTTON_DELETE,
				animation_library_is_foreign, TTR("Remove animation from Library."));
		}

		if (collapsed_libs.has(String(K))) {
			libitem->set_collapsed_recursive(true);
		}
	}
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

void AnimationLibraryEditor::show_dialog()
{
	update_tree();
	popup_centered_ratio(0.5);
}

void AnimationLibraryEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		new_library_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		load_library_button->set_button_icon(get_editor_theme_icon(SNAME("Load")));
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		tree->set_column_title(0, TTR("Resource"));
		tree->set_column_title(1, TTR("Storage"));
	} break;
	}
}

void AnimationLibraryEditor::_bind_methods() {}


