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

LineEdit* GroupSettingsEditor::get_name_box() const { return group_name; }



void GroupSettingsEditor::_add_group(String const&, String const&) {}

void GroupSettingsEditor::update_groups() {}

void GroupSettingsEditor::connect_filesystem_dock_signals(FileSystemDock*) {}
