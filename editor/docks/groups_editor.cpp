/**************************************************************************/
/*  groups_editor.cpp                                                     */
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

#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "groups_editor.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

static bool can_edit(Node* p_node, const String& p_group)
{
	Node* n = p_node;
	bool can_edit = true;
	while (n) {
		Ref<SceneState> ss = (n == EditorNode::get_singleton()->get_edited_scene())
								 ? n->get_scene_inherited_state()
								 : n->get_scene_instance_state();
		if (ss.is_valid()) {
			int path = ss->find_node_by_path(n->get_path_to(p_node));
			if (path != -1) {
				if (ss->is_node_in_group(path, p_group)) {
					can_edit = false;
					break;
				}
			}
		}
		n = n->get_owner();
	}
	return can_edit;
}

struct _GroupInfoComparator
{
	bool operator()(const Node::GroupInfo& p_a, const Node::GroupInfo& p_b) const
	{
		return p_a.name.string() < p_b.name.string();
	}
};

void GroupsEditor::_add_scene_group(const String& p_name) { scene_groups[p_name] = true; }

void GroupsEditor::_remove_scene_group(const String& p_name)
{
	scene_groups.erase(p_name);
	ProjectSettingsEditor::get_singleton()->get_group_settings()->remove_node_references(
		scene_root_node, p_name);
}

void GroupsEditor::_rename_scene_group(const String& p_old_name, const String& p_new_name)
{
	scene_groups[p_new_name] = scene_groups[p_old_name];
	scene_groups.erase(p_old_name);
	ProjectSettingsEditor::get_singleton()->get_group_settings()->rename_node_references(
		scene_root_node, p_old_name, p_new_name);
}

void GroupsEditor::_set_group_checked(const String& p_name, bool p_checked)
{
	TreeItem* ti = tree->get_item_with_text(p_name);
	if (!ti) {
		return;
	}

	ti->set_checked(0, p_checked);
}

bool GroupsEditor::_can_edit(const StringName& p_group)
{
	for (Node* p_node : selection) {
		if (!can_edit(p_node, p_group)) {
			return false;
		}
	}
	return true;
}

bool GroupsEditor::_has_group(const String& p_name)
{
	return global_groups.has(p_name) || scene_groups.has(p_name);
}

void GroupsEditor::_load_scene_groups(Node* p_node)
{
	List<Node::GroupInfo> groups;
	p_node->get_groups(&groups);

	for (const GroupInfo& gi : groups) {
		if (!gi.persistent) {
			continue;
		}

		if (global_groups.has(gi.name)) {
			continue;
		}

		bool is_editable = can_edit(p_node, gi.name);
		if (scene_groups.has(gi.name)) {
			scene_groups[gi.name] = scene_groups[gi.name] && is_editable;
		}
		else {
			scene_groups[gi.name] = is_editable;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_load_scene_groups(p_node->get_child(i));
	}
}

void GroupsEditor::_update_groups()
{
	if (!is_visible_in_tree()) {
		groups_dirty = true;
		return;
	}

	if (updating_groups) {
		return;
	}

	updating_groups = true;

	global_groups = ProjectSettings::get_singleton()->get_global_groups_list();

	_load_scene_groups(scene_root_node);

	for (HashMap<StringName, bool>::Iterator E = scene_groups.begin(); E;) {
		HashMap<StringName, bool>::Iterator next = E;
		++next;

		if (global_groups.has(E->key)) {
			scene_groups.erase(E->key);
		}
		E = next;
	}

	updating_groups = false;
}

void GroupsEditor::_update_tree()
{
	if (!is_visible_in_tree()) {
		groups_dirty = true;
		return;
	}

	if (selection.is_empty()) {
		return;
	}

	if (updating_tree) {
		return;
	}

	updating_tree = true;

	tree->clear();

	List<Node::GroupInfo> groups;
	for (Node* p_node : selection) {
		p_node->get_groups(&groups);
	}
	groups.sort_custom<_GroupInfoComparator>();

	List<StringName> current_groups;
	for (const Node::GroupInfo& gi : groups) {
		current_groups.push_back(gi.name);
	}

	TreeItem* root = tree->create_item();

	TreeItem* local_root = tree->create_item(root);
	local_root->set_text(0, TTR("Scene Groups"));
	local_root->set_icon(0, get_editor_theme_icon(SNAME("PackedScene")));
	local_root->set_custom_bg_color(
		0, get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor)));
	local_root->set_custom_stylebox(
		0, get_theme_stylebox(SNAME("prop_subsection_stylebox"), EditorStringName(Editor)));
	local_root->set_selectable(0, false);

	List<StringName> scene_keys;
	for (const KeyValue<StringName, bool>& E : scene_groups) {
		scene_keys.push_back(E.key);
	}
	scene_keys.sort_custom<NoCaseComparator>();

	for (const StringName& E : scene_keys) {
		if (!filter->get_text().is_subsequence_ofn(E)) {
			continue;
		}

		TreeItem* item = tree->create_item(local_root);
		item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		item->set_editable(0, _can_edit(E));
		item->set_checked(0, current_groups.find(E) != nullptr);
		item->set_text(0, E);
		if (!scene_groups[E]) {
			item->add_button(0, get_editor_theme_icon(SNAME("Lock")), -1, true,
				TTR("This group belongs to another scene and can't be edited."));
		}
		item->add_button(0, get_editor_theme_icon(SNAME("ActionCopy")), COPY_GROUP, false,
			TTR("Copy group name to clipboard."));
	}

	List<StringName> keys;
	for (const KeyValue<StringName, String>& E : global_groups) {
		keys.push_back(E.key);
	}
	keys.sort_custom<NoCaseComparator>();

	TreeItem* global_root = tree->create_item(root);
	global_root->set_text(0, TTR("Global Groups"));
	global_root->set_icon(0, get_editor_theme_icon(SNAME("Environment")));
	global_root->set_custom_bg_color(
		0, get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor)));
	global_root->set_custom_stylebox(
		0, get_theme_stylebox(SNAME("prop_subsection_stylebox"), EditorStringName(Editor)));
	global_root->set_selectable(0, false);

	for (const StringName& E : keys) {
		if (!filter->get_text().is_subsequence_ofn(E)) {
			continue;
		}

		TreeItem* item = tree->create_item(global_root);
		item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
		item->set_editable(0, _can_edit(E));
		item->set_checked(0, current_groups.find(E) != nullptr);
		item->set_text(0, E);
		if (!global_groups[E].is_empty()) {
			item->set_tooltip_text(0, vformat("%s\n\n%s", E, global_groups[E]));
		}
		item->add_button(0, get_editor_theme_icon(SNAME("ActionCopy")), COPY_GROUP, false,
			TTR("Copy group name to clipboard."));
	}

	updating_tree = false;
}

void GroupsEditor::_update_groups_and_tree()
{
	update_groups_and_tree_queued = false;
	// The scene_root_node could be unset before we actually run this code because this is queued
	// with call_deferred(). In that case NOTIFICATION_VISIBILITY_CHANGED will call this function
	// again soon.
	if (!scene_root_node) {
		return;
	}
	_update_groups();
	_update_tree();
}

void GroupsEditor::set_selection(const Vector<Node*>& p_nodes)
{
	if (p_nodes.is_empty()) {
		holder->hide();
		select_a_node->show();
		selection.clear();
		return;
	}

	selection = p_nodes;

	holder->show();
	select_a_node->hide();

	if (scene_tree->get_edited_scene_root() != scene_root_node) {
		scene_root_node = scene_tree->get_edited_scene_root();
		_update_groups();
	}

	_update_tree();
}

void GroupsEditor::_check_add()
{
	String group_name = add_group_name->get_text().strip_edges();
	_validate_name(group_name, add_validation_panel);
}

void GroupsEditor::_validate_name(const String& p_name, EditorValidationPanel* p_validation_panel)
{
	if (p_name.is_empty()) {
		p_validation_panel->set_message(EditorValidationPanel::MSG_ID_DEFAULT,
			TTRC("Group can't be empty."), EditorValidationPanel::MSG_ERROR);
	}
	else if (_has_group(p_name)) {
		p_validation_panel->set_message(EditorValidationPanel::MSG_ID_DEFAULT,
			TTRC("Group already exists."), EditorValidationPanel::MSG_ERROR);
	}
}

void GroupsEditor::_groups_gui_input(Ref<InputEvent> p_event)
{
	Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
		if (ED_IS_SHORTCUT("groups_editor/delete", p_event)) {
			_menu_id_pressed(DELETE_GROUP);
		}
		else if (ED_IS_SHORTCUT("groups_editor/rename", p_event)) {
			_menu_id_pressed(RENAME_GROUP);
		}
		else if (ED_IS_SHORTCUT("editor/open_search", p_event)) {
			filter->grab_focus();
			filter->select_all();
		}
		else {
			return;
		}

		accept_event();
	}
}


