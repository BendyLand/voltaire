/**************************************************************************/
/*  group_settings_editor.cpp                                             */
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
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/themes/editor_scale.h"
#include "group_settings_editor.h"
#include "scene/gui/line_edit.h"
#include "scene/resources/packed_scene.h"

void GroupSettingsEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		update_groups();
	} break;
	case NOTIFICATION_THEME_CHANGED: {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
	} break;
	}
}

String GroupSettingsEditor::_check_new_group_name(const String& p_name)
{
	if (p_name.is_empty()) {
		return TTR("Invalid group name. It cannot be empty.");
	}

	if (ProjectSettings::get_singleton()->has_global_group(p_name)) {
		return vformat(TTR("A group with the name '%s' already exists."), p_name);
	}

	return "";
}

void GroupSettingsEditor::_bind_methods() {}

void GroupSettingsEditor::_add_group()
{
	_add_group(group_name->get_text(), group_description->get_text());
}

void GroupSettingsEditor::_text_submitted(const String& p_text)
{
	if (!add_button->is_disabled()) {
		_add_group();
	}
}

void GroupSettingsEditor::_group_name_text_changed(const String& p_name)
{
	String error = _check_new_group_name(p_name.strip_edges());
	add_button->set_tooltip_text(error);
	add_button->set_disabled(!error.is_empty());
}

void GroupSettingsEditor::_modify_references(
	const StringName& p_name, const StringName& p_new_name, bool p_is_rename)
{
	HashSet<String> scenes;

	HashMap<StringName, HashSet<StringName>> scene_groups_cache(
		ProjectSettings::get_singleton()->get_scene_groups_cache());
	for (const KeyValue<StringName, HashSet<StringName>>& E : scene_groups_cache) {
		if (E.value.has(p_name)) {
			scenes.insert(E.key);
		}
	}

	int steps = scenes.size();
	Vector<EditorData::EditedScene> edited_scenes =
		EditorNode::get_editor_data().get_edited_scenes();
	for (const EditorData::EditedScene& es : edited_scenes) {
		if (!es.root) {
			continue;
		}
		if (es.path.is_empty()) {
			++steps;
		}
		else if (!scenes.has(es.path)) {
			++steps;
		}
	}

	String progress_task = p_is_rename ? "rename_reference" : "remove_references";
	String progress_label =
		p_is_rename ? TTR("Renaming Group References") : TTR("Removing Group References");
	EditorProgress progress(progress_task, progress_label, steps);

	int step = 0;
	// Update opened scenes.
	HashSet<String> edited_scenes_path;
	for (const EditorData::EditedScene& es : edited_scenes) {
		if (!es.root) {
			continue;
		}
		progress.step(es.path, step++);
		bool edited = p_is_rename ? rename_node_references(es.root, p_name, p_new_name)
								  : remove_node_references(es.root, p_name);
		if (!es.path.is_empty()) {
			scenes.erase(es.path);
			if (edited) {
				edited_scenes_path.insert(es.path);
			}
		}
	}
	if (!edited_scenes_path.is_empty()) {
		EditorNode::get_singleton()->save_scene_list(edited_scenes_path);
		SceneTreeDock::get_singleton()->get_tree_editor()->update_tree();
	}

	for (const String& E : scenes) {
		Ref<PackedScene> packed_scene = ResourceLoader::load(E);
		progress.step(E, step++);
		ERR_CONTINUE(packed_scene.is_null());
		if (p_is_rename) {
			if (packed_scene->get_state()->rename_group_references(p_name, p_new_name)) {
				ResourceSaver::save(packed_scene.ptr(), E);
			}
		}
		else {
			if (packed_scene->get_state()->remove_group_references(p_name)) {
				ResourceSaver::save(packed_scene.ptr(), E);
			}
		}
	}
}

void GroupSettingsEditor::remove_references(const StringName& p_name)
{
	_modify_references(p_name, StringName(), false);
}

void GroupSettingsEditor::rename_references(
	const StringName& p_old_name, const StringName& p_new_name)
{
	_modify_references(p_old_name, p_new_name, true);
}

bool GroupSettingsEditor::remove_node_references(Node* p_node, const StringName& p_name)
{
	bool edited = false;
	if (p_node->is_in_group(p_name)) {
		p_node->remove_from_group(p_name);
		edited = true;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		edited |= remove_node_references(p_node->get_child(i), p_name);
	}
	return edited;
}

bool GroupSettingsEditor::rename_node_references(
	Node* p_node, const StringName& p_old_name, const StringName& p_new_name)
{
	bool edited = false;
	if (p_node->is_in_group(p_old_name)) {
		p_node->remove_from_group(p_old_name);
		p_node->add_to_group(p_new_name, true);
		edited = true;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		edited |= rename_node_references(p_node->get_child(i), p_old_name, p_new_name);
	}
	return edited;
}

void GroupSettingsEditor::show_message(const String& p_message)
{
	message->set_text(p_message);
	message->popup_centered();
}

LineEdit* GroupSettingsEditor::get_name_box() const { return group_name; }


