/**************************************************************************/
/*  scene_tree_dock.cpp                                                   */
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
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/rename_dialog.h"
#include "editor/scene/reparent_dialog.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/shader_create_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/animation_tree.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/panel_container.h"
#include "scene/main/missing_node.h"
#include "scene/main/scene_tree.h"
#include "scene/property_utils.h"
#include "scene/resources/packed_scene.h"
#include "scene_tree_dock.h"
#include "servers/display/display_server.h"

void SceneTreeDock::_reset_hovering_timer()
{
	if (!inspect_hovered_node_delay->is_stopped()) {
		inspect_hovered_node_delay->stop();
	}
	node_hovered_previously = nullptr;
}

void SceneTreeDock::_scene_tree_gui_input(Ref<InputEvent> p_event)
{
	Ref<InputEventKey> key = p_event;

	if (key.is_null() || !key->is_pressed() || key->is_echo()) {
		return;
	}

	if (ED_IS_SHORTCUT("editor/open_search", p_event)) {
		filter->grab_focus();
		filter->select_all();
		accept_event();
	}
}

bool SceneTreeDock::_cyclical_dependency_exists(
	const String& p_target_scene_path, Node* p_desired_node)
{
	int childCount = p_desired_node->get_child_count();

	if (_track_inherit(p_target_scene_path, p_desired_node)) {
		return true;
	}

	for (int i = 0; i < childCount; i++) {
		Node* child = p_desired_node->get_child(i);

		if (_cyclical_dependency_exists(p_target_scene_path, child)) {
			return true;
		}
	}

	return false;
}

bool SceneTreeDock::_track_inherit(const String& p_target_scene_path, Node* p_desired_node)
{
	Node* p = p_desired_node;
	bool result = false;
	Vector<Node*> instances;
	while (true) {
		if (p->get_scene_file_path() == p_target_scene_path) {
			result = true;
			break;
		}
		Ref<SceneState> ss = p->get_scene_inherited_state();
		if (ss.is_valid()) {
			String path = ss->get_path();
			Ref<PackedScene> pack_data = ResourceLoader::load(path);
			if (pack_data.is_valid()) {
				p = pack_data->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
				if (!p) {
					continue;
				}
				instances.push_back(p);
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}
	for (int i = 0; i < instances.size(); i++) {
		memdelete(instances[i]);
	}
	return result;
}

void SceneTreeDock::_fill_path_renames(Vector<StringName> base_path,
	Vector<StringName> new_base_path, Node* p_node, HashMap<Node*, NodePath>* p_renames)
{
	base_path.push_back(p_node->get_name());

	NodePath new_path;
	if (!new_base_path.is_empty()) {
		new_base_path.push_back(p_node->get_name());
		new_path = NodePath(new_base_path, true);
	}

	p_renames->insert(p_node, new_path);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_fill_path_renames(base_path, new_base_path, p_node->get_child(i), p_renames);
	}
}

void SceneTreeDock::fill_path_renames(
	Node* p_node, Node* p_new_parent, HashMap<Node*, NodePath>* p_renames)
{
	Vector<StringName> base_path;
	Node* n = p_node->get_parent();
	while (n) {
		base_path.push_back(n->get_name());
		n = n->get_parent();
	}

	Vector<StringName> new_base_path;
	if (p_new_parent) {
		n = p_new_parent;
		while (n) {
			new_base_path.push_back(n->get_name());
			n = n->get_parent();
		}

		// For the case Reparent to New Node, the new parent has not yet been added to the tree.
		if (!p_new_parent->is_inside_tree()) {
			new_base_path.append_array(base_path);
		}

		new_base_path.reverse();
	}
	base_path.reverse();

	_fill_path_renames(base_path, new_base_path, p_node, p_renames);
}

bool SceneTreeDock::_update_node_path(
	Node* p_root_node, NodePath& r_node_path, HashMap<Node*, NodePath>* p_renames) const
{
	Node* target_node = p_root_node->get_node_or_null(r_node_path);
	ERR_FAIL_NULL_V_MSG(target_node, false,
		"Found invalid node path '" + String(r_node_path) + "' on node '" +
			String(scene_root->get_path_to(p_root_node)) + "'");

	// Try to find the target node in modified node paths.
	HashMap<Node*, NodePath>::Iterator found_node_path = p_renames->find(target_node);
	if (found_node_path) {
		if (found_node_path->value.is_empty()) {
			r_node_path = found_node_path->value;
			return true;
		}

		String old_subnames;
		if (r_node_path.get_subname_count() > 0) {
			old_subnames = ":" + r_node_path.get_concatenated_subnames();
		}

		HashMap<Node*, NodePath>::Iterator found_root_path = p_renames->find(p_root_node);
		NodePath root_path_new = found_root_path ? found_root_path->value : p_root_node->get_path();
		r_node_path =
			NodePath(String(root_path_new.rel_path_to(found_node_path->value)) + old_subnames);

		return true;
	}

	// Update the path if the base node has changed and has not been deleted.
	HashMap<Node*, NodePath>::Iterator found_root_path = p_renames->find(p_root_node);
	if (found_root_path) {
		NodePath root_path_new = found_root_path->value;
		if (!root_path_new.is_empty()) {
			String old_subnames;
			if (r_node_path.get_subname_count() > 0) {
				old_subnames = ":" + r_node_path.get_concatenated_subnames();
			}

			NodePath old_abs_path =
				NodePath(String(p_root_node->get_path()).path_join(String(r_node_path)));
			old_abs_path.simplify();
			r_node_path = NodePath(String(root_path_new.rel_path_to(old_abs_path)) + old_subnames);
		}

		return true;
	}

	return false;
}

void SceneTreeDock::set_selected(Node* p_node, bool p_emit_selected)
{
	scene_tree->set_selected(p_node, p_emit_selected);
}

void SceneTreeDock::_set_node_owner_recursive(
	Node* p_node, Node* p_owner, const HashMap<const Node*, Node*>& p_inverse_duplimap)
{
	HashMap<const Node*, Node*>::ConstIterator E = p_inverse_duplimap.find(p_node);

	if (E) {
		const Node* original = E->value;
		if (original->get_owner()) {
			p_node->set_owner(p_owner);
		}
	}

	for (int i = 0; i < p_node->get_child_count(false); i++) {
		_set_node_owner_recursive(p_node->get_child(i, false), p_owner, p_inverse_duplimap);
	}
}

static bool _is_node_visible(Node* p_node)
{
	if (!p_node->get_owner()) {
		return false;
	}
	if (p_node->get_owner() != EditorNode::get_singleton()->get_edited_scene() &&
		!EditorNode::get_singleton()->get_edited_scene()->is_editable_instance(
			p_node->get_owner())) {
		return false;
	}

	return true;
}

void SceneTreeDock::_normalize_drop(Node*& to_node, int& to_pos, int p_type)
{
	// Drop as last child, by default.
	to_pos = -1;

	if (p_type == -1) {
		// Drop as sibling, above.
		if (to_node == EditorNode::get_singleton()->get_edited_scene()) {
			to_node = nullptr;
			ERR_FAIL_MSG("Cannot perform drop above the root node!");
		}

		to_pos = to_node->get_index(false);
		to_node = to_node->get_parent();
	}
	else if (p_type == 1) {
		// Drop as child of root node if out of bounds.
		if (to_node == EditorNode::get_singleton()->get_edited_scene()) {
			to_pos = -1;
			return;
		}

		// Drop as sibling, below, by using indent space or when children are collapsed.
		Node* lower_sibling = nullptr;

		for (int i = to_node->get_index(false) + 1;
			 i < to_node->get_parent()->get_child_count(false); i++) {
			Node* c = to_node->get_parent()->get_child(i, false);
			if (_is_node_visible(c)) {
				lower_sibling = c;
				break;
			}
		}

		if (lower_sibling) {
			to_pos = lower_sibling->get_index(false);
		}

		to_node = to_node->get_parent();
	}
	else if (p_type == 2) {
		// Drop as first child, among others.
		to_pos = 0;
	}
}

void SceneTreeDock::_filter_option_selected(int p_option)
{
	String filter_parameter;
	switch (p_option) {
	case FILTER_BY_TYPE: {
		filter_parameter = "type";
	} break;
	case FILTER_BY_GROUP: {
		filter_parameter = "group";
	} break;
	}

	if (!filter_parameter.is_empty()) {
		set_filter((get_filter() + " " + filter_parameter + ":").strip_edges());
		filter->set_caret_column(filter->get_text().length());
		filter->grab_focus();
	}
}

String SceneTreeDock::get_filter() { return filter->get_text(); }

void SceneTreeDock::set_filter(const String& p_filter)
{
	filter->set_text(p_filter);
	scene_tree->set_filter(p_filter);
}

List<Node*> SceneTreeDock::get_node_clipboard() const { return List<Node*>(node_clipboard); }

void SceneTreeDock::_clear_clipboard()
{
	for (Node* E : node_clipboard) {
		memdelete(E);
	}
	node_clipboard.clear();
	node_clipboard_edited_scene_owned.clear();
	clipboard_resource_remap.clear();
}

SceneTreeDock* SceneTreeDock::singleton = nullptr;

SceneTreeDock::~SceneTreeDock()
{
	singleton = nullptr;

	if (!node_clipboard.is_empty()) {
		_clear_clipboard();
	}
}


