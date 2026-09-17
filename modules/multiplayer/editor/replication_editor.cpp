/**************************************************************************/
/*  replication_editor.cpp                                                */
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

#include "../multiplayer_synchronizer.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/property_selector.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "replication_editor.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/separator.h"
#include "scene/gui/tree.h"

void ReplicationEditor::_pick_node_filter_text_changed(const String& p_newtext)
{
	TreeItem* root_item = pick_node->get_scene_tree()->get_scene_tree()->get_root();

	Vector<Node*> select_candidates;
	Node* to_select = nullptr;

	String filter = pick_node->get_filter_line_edit()->get_text();

	_pick_node_select_recursive(root_item, filter, select_candidates);

	if (!select_candidates.is_empty()) {
		for (int i = 0; i < select_candidates.size(); ++i) {
			Node* candidate = select_candidates[i];

			if (((String)candidate->get_name()).to_lower().begins_with(filter.to_lower())) {
				to_select = candidate;
				break;
			}
		}

		if (!to_select) {
			to_select = select_candidates[0];
		}
	}

	pick_node->get_scene_tree()->set_selected(to_select);
}

void ReplicationEditor::_pick_new_property()
{
	if (current == nullptr) {
		EditorNode::get_singleton()->show_warning(
			TTRC("Select a replicator node in order to pick a property to add to it."));
		return;
	}
	Node* root = current->get_node(current->get_root_path());
	if (!root) {
		EditorNode::get_singleton()->show_warning(
			TTRC("Not possible to add a new property to synchronize without a root."));
		return;
	}
	pick_node->popup_scenetree_dialog(nullptr, current);
	pick_node->get_filter_line_edit()->clear();
	pick_node->get_filter_line_edit()->grab_focus();
}

void ReplicationEditor::_pick_node_property_selected(String p_name)
{
	String adding_prop_path = String(adding_node_path) + ":" + p_name;

	_add_sync_property(adding_prop_path);
}

void ReplicationEditor::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorThemeManager::is_generated_theme_outdated()) {
			break;
		}
		[[fallthrough]];
	}
	}
}

void ReplicationEditor::_add_pressed()
{
	if (!current) {
		EditorNode::get_singleton()->show_warning(
			TTRC("Please select a MultiplayerSynchronizer first."));
		return;
	}
	if (current->get_root_path().is_empty()) {
		EditorNode::get_singleton()->show_warning(
			TTRC("The MultiplayerSynchronizer needs a root path."));
		return;
	}
	String np_text = np_line_edit->get_text();

	if (np_text.is_empty()) {
		EditorNode::get_singleton()->show_warning(TTRC("Property/path must not be empty."));
		return;
	}

	int idx = np_text.find_char(':');
	if (idx == -1) {
		np_text = ".:" + np_text;
	}
	else if (idx == 0) {
		np_text = "." + np_text;
	}
	NodePath path = NodePath(np_text);
	if (path.is_empty()) {
		EditorNode::get_singleton()->show_warning(
			vformat(TTR("Invalid property path: '%s'"), np_text));
		return;
	}

	_add_sync_property(String(path));
}

void ReplicationEditor::_np_text_submitted(const String& p_newtext) { _add_pressed(); }

void ReplicationEditor::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (p_slot != EditorDock::DOCK_SLOT_BOTTOM) {
		tree_mc->set_theme_type_variation("NoBorderHorizontalBottom");
		tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_DISABLED);
	}
	else {
		tree_mc->set_theme_type_variation("NoBorderHorizontal");
		tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTTOM);
	}
}

void ReplicationEditor::edit(MultiplayerSynchronizer* p_sync)
{
	if (current == p_sync) {
		return;
	}
	current = p_sync;
	if (current) {
		config = current->get_replication_config();
	}
	else {
		config.unref();
	}
	_update_config();
}



void ReplicationEditor::_add_sync_property(String) {}

void ReplicationEditor::_update_config() {}

void ReplicationEditor::_pick_node_select_recursive(TreeItem*, String const&, Vector<Node*>&) {}
