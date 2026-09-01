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

/// ReplicationEditor

void _set_replication_mode_options(TreeItem* p_item)
{
	p_item->set_text(2, TTR("Never", "Replication Mode") + "," + TTR("Always", "Replication Mode") +
							"," + TTR("On Change", "Replication Mode"));
}

void ReplicationEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		TreeItem* root = tree->get_root();
		if (root) {
			for (TreeItem* ti = root->get_first_child(); ti; ti = ti->get_next()) {
				_set_replication_mode_options(ti);
			}
		}
	} break;

	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorThemeManager::is_generated_theme_outdated()) {
			break;
		}
		[[fallthrough]];
	}
	case NOTIFICATION_ENTER_TREE: {
		add_theme_style_override(
			SceneStringName(panel), EditorNode::get_singleton()
										->get_editor_theme()
										->get_stylebox(SceneStringName(panel), SNAME("Panel"))
										.ptr());
		add_pick_button->set_button_icon(
			get_theme_icon(SNAME("Add"), EditorStringName(EditorIcons)));
		pin->set_button_icon(get_theme_icon(SNAME("Pin"), EditorStringName(EditorIcons)));
	} break;
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

void ReplicationEditor::_add_property(
	const NodePath& p_property, bool p_spawn, SceneReplicationConfig::ReplicationMode p_mode)
{
	String prop = String(p_property);
	TreeItem* item = tree->create_item();
	item->set_selectable(0, false);
	item->set_selectable(1, false);
	item->set_selectable(2, false);
	item->set_selectable(3, false);
	item->set_text(0, prop);
	item->set_auto_translate_mode(0, AUTO_TRANSLATE_MODE_DISABLED);
	Node* root_node = current && !current->get_root_path().is_empty()
						  ? current->get_node(current->get_root_path())
						  : nullptr;
	Ref<Texture2D> icon = _get_class_icon(root_node);
	if (root_node) {
		String path = prop.substr(0, prop.find_char(':'));
		String subpath = prop.substr(path.size());
		Node* node = root_node->get_node_or_null(path);
		if (!node) {
			node = root_node;
		}
		item->set_text(0, String(node->get_name()) + ":" + subpath);
		icon = _get_class_icon(node);
		item->set_icon(0, icon);
	}
	else {
		item->set_icon(0, icon);
	}
	item->add_button(3, get_theme_icon(SNAME("Remove"), EditorStringName(EditorIcons)));
	item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_CENTER);
	item->set_cell_mode(1, TreeItem::CELL_MODE_CHECK);
	item->set_checked(1, p_spawn);
	item->set_editable(1, true);
	item->set_text_alignment(2, HORIZONTAL_ALIGNMENT_CENTER);
	item->set_cell_mode(2, TreeItem::CELL_MODE_RANGE);
	item->set_range_config(2, 0, 2, 1);
	item->set_auto_translate_mode(2, AUTO_TRANSLATE_MODE_DISABLED);
	_set_replication_mode_options(item);
	item->set_range(2, (int)p_mode);
	item->set_editable(2, true);
}


