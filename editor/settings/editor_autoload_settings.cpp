/**************************************************************************/
/*  editor_autoload_settings.cpp                                          */
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
#include "core/core_constants.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/scene/scene_create_dialog.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor_autoload_settings.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

#define PREVIEW_LIST_MAX_SIZE 10

void EditorAutoloadSettings::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		List<String> afn;
		ResourceLoader::get_recognized_extensions_for_type("PackedScene", &afn);

		for (const String& E : afn) {
			scene_file_dialog->add_filter("*." + E);
		}

		ResourceLoader::get_recognized_extensions_for_type("Script", &afn);

		for (const String& E : afn) {
			autoload_file_dialog->add_filter("*." + E);
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		browse_button->set_button_icon(get_editor_theme_icon(SNAME("FileBrowse")));
		create_script_autoload->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		create_scene_autoload->set_button_icon(get_editor_theme_icon(SNAME("Add")));
	} break;
	}
}

void EditorAutoloadSettings::_validate_autoload_name()
{
	String error;
	bool is_valid = _autoload_name_is_valid(name_edit->get_text(), &error);
	if (!is_valid) {
		name_validator->set_message(0, error, EditorValidationPanel::MSG_ERROR);
	}
}

void EditorAutoloadSettings::_autoload_selected()
{
	TreeItem* ti = tree->get_selected();

	if (!ti) {
		return;
	}

	selected_autoload = "autoload/" + ti->get_text(0);
}

void EditorAutoloadSettings::_autoload_activated()
{
	TreeItem* ti = tree->get_selected();
	if (!ti) {
		return;
	}
	_autoload_open(ti->get_text(1));
}

void EditorAutoloadSettings::_autoload_open(const String& fpath)
{
	EditorNode::get_singleton()->load_scene_or_resource(fpath);
	ProjectSettingsEditor::get_singleton()->hide();
}

void EditorAutoloadSettings::_create_scene_autoload()
{
	scene_file_dialog->set_current_file("new_autoload_scene.tscn");
	scene_file_dialog->popup_file_dialog();
}

void EditorAutoloadSettings::_autoload_file_selected(const String& p_path)
{
	// Convert the file name to PascalCase, which is the convention for classes in GDScript.
	_try_add_autoload(p_path.get_file().get_basename().to_pascal_case(), p_path);
}

void EditorAutoloadSettings::_scene_created()
{
	Node* root = scene_create_dialog->create_scene_root();

	Ref<PackedScene> ps;
	ps.instantiate();
	ps->pack(root);

	Error err = ResourceSaver::save(ps.ptr(), scene_create_dialog->get_scene_path());
	if (err != OK) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Failed to create scene. Error: %d."), err));
		return;
	}

	_try_add_autoload(scene_create_dialog->get_root_name().to_pascal_case(),
		scene_create_dialog->get_scene_path());
}

void EditorAutoloadSettings::_add_autoload(const String& p_name, const String& p_path)
{
	autoload_add(p_name, p_path, true);
}

void EditorAutoloadSettings::_try_add_autoload(const String& p_name, const String& p_path)
{
	if (_autoload_name_is_valid(p_name)) {
		_add_autoload(p_name, p_path);
		return;
	}
	pending_autoload_path = p_path;

	name_edit->set_text(p_name);
	name_validator->update();
	name_dialog->popup_centered(Vector2i(600 * EDSCALE, 0));
	name_edit->grab_focus();
	name_edit->select_all();
}

void EditorAutoloadSettings::_confirm_autoload_name()
{
	_add_autoload(name_edit->get_text(), pending_autoload_path);
}

EditorAutoloadSettings::~EditorAutoloadSettings()
{
	for (const AutoloadInfo& info : autoload_cache) {
		if (info.node && !info.in_editor) {
			memdelete(info.node);
		}
	}
}


