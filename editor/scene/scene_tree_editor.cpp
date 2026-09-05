/**************************************************************************/
/*  scene_tree_editor.cpp                                                 */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "scene_tree_editor.h"

Node* SceneTreeEditor::get_scene_node() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);

	return get_tree()->get_edited_scene_root();
}

void SceneTreeEditor::_node_script_changed(Node* p_node)
{
	HashMap<Node*, CachedNode>::Iterator I = node_cache.get(p_node, false);
	if (!I) {
		// We leave these signals connected when switching tabs.
		// If the node is not in cache it was for a different tab.
		return;
	}

	node_cache.mark_dirty(p_node);

	_update_if_clean();
}

void SceneTreeEditor::_move_node_children(HashMap<Node*, CachedNode>::Iterator& p_I)
{
	TreeItem* item = p_I->value.item;
	TreeItem* previous_item = nullptr;
	Node* node = p_I->key;
	int cc = node->get_child_count(false);

	for (int i = 0; i < cc; i++) {
		HashMap<Node*, CachedNode>::Iterator CI = node_cache.get(node->get_child(i, false));
		if (CI) {
			_move_node_item(item, CI, previous_item);
			previous_item = CI->value.item;
		}
		else {
			previous_item = nullptr;
		}
	}

	p_I->value.has_moved_children = false;
}

void SceneTreeEditor::_move_node_item(
	TreeItem* p_parent, HashMap<Node*, CachedNode>::Iterator& p_I, TreeItem* p_correct_prev)
{
	if (!p_parent) {
		return;
	}

	Node* node = p_I->key;

	int current_node_index = node->get_index(false);
	int current_item_index = -1;
	TreeItem* item = p_I->value.item;

	if (item->get_parent() != p_parent) {
		TreeItem* p = item->get_parent();
		if (p) {
			item->get_parent()->remove_child(item);
		}
		p_parent->add_child(item);
		p_I->value.removed = false;
		current_item_index = p_parent->get_child_count() - 1;
		p_I->value.index = current_item_index;
	}

	if (p_I->value.index != current_node_index) {
		bool already_in_correct_location;
		if (current_item_index >= 0) {
			// If we just re-parented we know our index.
			already_in_correct_location = current_item_index == current_node_index;
		}
		else if (p_correct_prev) {
			// It's cheaper to check if we're set up correctly by checking via correct_prev if we
			// can
			already_in_correct_location = item->get_prev() == p_correct_prev;
		}
		else {
			already_in_correct_location = item->get_index() == current_node_index;
		}

		// Are we already in the right place?
		if (already_in_correct_location) {
			p_I->value.index = current_node_index;
			return;
		}

		// Are we the first node?
		if (current_node_index == 0) {
			// There has to be at least 1 other node, otherwise we would not have gotten here.
			TreeItem* neighbor_item = p_parent->get_first_child();
			item->move_before(neighbor_item);
		}
		else {
			TreeItem* prev_item = p_correct_prev;
			if (!prev_item) {
				prev_item = p_parent->get_child(
					CLAMP(current_node_index - 1, 0, p_parent->get_child_count() - 1));
			}
			item->move_after(prev_item);
		}

		p_I->value.index = current_node_index;
	}
}

void SceneTreeEditor::_node_child_order_changed(Node* p_node)
{
	// Do not try to change children on nodes currently marked for removal.
	HashMap<Node*, CachedNode>::Iterator I = node_cache.get(p_node, false);
	if (I) {
		node_cache.mark_dirty(I->key);
		I->value.has_moved_children = true;
	}

	_update_if_clean();
}

void SceneTreeEditor::_node_editor_state_changed(Node* p_node)
{
	node_cache.mark_dirty(p_node);
	HashMap<Node*, CachedNode>::Iterator I = node_cache.get(p_node, false);
	if (I) {
		if (p_node->is_inside_tree() && p_node->can_process() != I->value.can_process) {
			// All our children also change process mode.
			node_cache.mark_children_dirty(p_node, true);
		}
	}

	_update_if_clean();
}

void SceneTreeEditor::_node_added(Node* p_node)
{
	if (!get_scene_node()) {
		return;
	}

	if (p_node != get_scene_node() && !get_scene_node()->is_ancestor_of(p_node)) {
		return;
	}

	node_cache.mark_dirty(p_node);
	_update_if_clean();
}

void SceneTreeEditor::_node_removed(Node* p_node)
{
	if (EditorNode::get_singleton()->is_exiting()) {
		return; // Speed up exit.
	}

	if (EditorNode::get_singleton()->is_changing_scene()) {
		return; // Switching tabs we will be destroying node cache anyway.
	}

	if (!get_scene_node()) {
		return;
	}

	if (p_node != get_scene_node() && !get_scene_node()->is_ancestor_of(p_node)) {
		return;
	}
	node_cache.remove(p_node);
	_update_if_clean();
}

void SceneTreeEditor::_compute_hash(Node* p_node, uint64_t& hash)
{
	// Nodes are added and removed by Node* pointers.
	hash = hash_djb2_one_64((ptrdiff_t)p_node, hash);
	// This hash is non-commutative: if the node order changes so will the hash.
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_compute_hash(p_node->get_child(i), hash);
	}
}

void SceneTreeEditor::_reset()
{
	// Stop any waiting change to tooltip.
	update_node_tooltip_delay->stop();
	tree->clear();
	node_cache.clear();
}

void SceneTreeEditor::_test_update_tree()
{
	pending_test_update = false;

	if (!is_inside_tree()) {
		return;
	}

	if (tree_dirty) {
		return; // Don't even bother.
	}

	uint64_t hash = hash_djb2_one_64(0);
	if (get_scene_node()) {
		_compute_hash(get_scene_node(), hash);
	}

	// Test hash.
	if (hash == last_hash) {
		return; // Did not change.
	}

	_update_if_clean();
}

void SceneTreeEditor::set_selected(Node* p_node, bool p_emit_selected)
{
	ERR_FAIL_COND(blocked > 0);

	if (pending_test_update) {
		_test_update_tree();
	}

	if (tree_dirty) {
		_update_tree();
	}

	if (selected == p_node) {
		return;
	}
	selected = p_node;

	TreeItem* item = p_node ? _find(tree->get_root(), p_node->get_path()) : nullptr;
	if (item) {
		if (auto_expand_selected) {
			// Make visible when it's collapsed.
			TreeItem* node = item->get_parent();
			while (node) {
				node->set_collapsed(false);
				node = node->get_parent();
			}
			item->select(0);
			tree->ensure_cursor_is_visible();
		}
		else {
			// Ensure the node is selected and visible for the user if the node
			// is not collapsed.
			bool collapsed = false;
			TreeItem* node = item;
			while (node && node != tree->get_root()) {
				if (node->is_collapsed()) {
					collapsed = true;
					break;
				}
				node = node->get_parent();
			}
			if (!collapsed) {
				item->select(0);
				tree->ensure_cursor_is_visible();
			}
		}
	}
}

Node* SceneTreeEditor::get_selected() { return selected; }

void SceneTreeEditor::_update_marking_list(const HashSet<Node*>& p_marked)
{
	for (Node* N : p_marked) {
		HashMap<Node*, CachedNode>::Iterator I = node_cache.get(N);
		if (I) {
			node_cache.mark_dirty(N);
			node_cache.mark_children_dirty(N, true);
		}
	}
}

void SceneTreeEditor::set_marked(
	const HashSet<Node*>& p_marked, bool p_selectable, bool p_children_selectable)
{
	_update_if_clean();

	_update_marking_list(marked);
	_update_marking_list(p_marked);

	marked = p_marked;

	marked_selectable = p_selectable;
	marked_children_selectable = p_children_selectable;
	_update_tree();
}

void SceneTreeEditor::set_marked(Node* p_marked, bool p_selectable, bool p_children_selectable)
{
	HashSet<Node*> s;
	if (p_marked) {
		s.insert(p_marked);
	}
	set_marked(s, p_selectable, p_children_selectable);
}

void SceneTreeEditor::set_filter(const String& p_filter)
{
	filter = p_filter;
	_update_filter(nullptr, true);
}

String SceneTreeEditor::get_filter() const { return filter; }

String SceneTreeEditor::get_filter_term_warning() { return filter_term_warning; }

void SceneTreeEditor::set_show_all_nodes(bool p_show_all_nodes)
{
	show_all_nodes = p_show_all_nodes;
	_update_filter(nullptr, true);
}

void SceneTreeEditor::set_as_scene_tree_dock() { is_scene_tree_dock = true; }

void SceneTreeEditor::set_display_foreign_nodes(bool p_display)
{
	display_foreign = p_display;
	_update_tree();
}

void SceneTreeEditor::set_valid_types(const Vector<StringName>& p_valid)
{
	valid_types = p_valid;
	clear_cache();
}

void SceneTreeEditor::_selection_changed()
{
	if (!editor_selection) {
		return;
	}

	TreeItem* root = tree->get_root();

	if (!root) {
		return;
	}
	_update_selection(root);
}

bool SceneTreeEditor::_is_script_type(const StringName& p_type) const
{
	return (script_types->has(p_type));
}

bool SceneTreeEditor::_has_drop_selection(TreeItem* p_item, const Point2& p_point) const
{
	int section = (p_point == Vector2(Math::INF, Math::INF))
					  ? tree->get_drop_section_at_position(tree->get_item_rect(p_item).position)
					  : tree->get_drop_section_at_position(p_point);
	return !(section < -1 || (section == -1 && !p_item->get_parent()));
}

void SceneTreeEditor::_empty_clicked(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	_rmb_select(p_pos);
}

void SceneTreeEditor::update_warning() { _warning_changed(nullptr); }

void SceneTreeEditor::_warning_changed(Node* p_for_node)
{
	node_cache.mark_dirty(p_for_node);

	// Should use a timer.
	update_timer->start();
}

void SceneTreeEditor::set_connect_to_script_mode(bool p_enable)
{
	connect_to_script_mode = p_enable;
	_update_tree();
}

void SceneTreeEditor::set_connecting_signal(bool p_enable)
{
	connecting_signal = p_enable;
	_update_tree();
}

void SceneTreeEditor::set_update_when_invisible(bool p_enable)
{
	update_when_invisible = p_enable;
	_update_tree();
}

void SceneTreeEditor::_bind_methods() {}

SceneTreeEditor::~SceneTreeEditor() { memdelete(script_types); }

/******** DIALOG *********/

void SceneTreeDialog::popup_scenetree_dialog(Node* p_selected_node, Node* p_marked_node,
	bool p_marked_node_selectable, bool p_marked_node_children_selectable)
{
	get_scene_tree()->set_marked(
		p_marked_node, p_marked_node_selectable, p_marked_node_children_selectable);
	get_scene_tree()->set_selected(p_selected_node);
	popup_centered_clamped(Size2(350, 700) * EDSCALE);
}

void SceneTreeDialog::_cancel() { hide(); }

void SceneTreeDialog::_selected_changed() { get_ok_button()->set_disabled(!tree->get_selected()); }

void SceneTreeDialog::_filter_changed(const String& p_filter) { tree->set_filter(p_filter); }

void SceneTreeDialog::_bind_methods() {}

LineEdit* SceneTreeDialog::get_filter_line_edit() { return filter; }

/******** CACHE *********/

HashMap<Node*, SceneTreeEditor::CachedNode>::Iterator SceneTreeEditor::NodeCache::add(
	Node* p_node, TreeItem* p_item)
{
	if (!p_node) {
		return HashMap<Node*, CachedNode>::Iterator();
	}

	return cache.insert(p_node, CachedNode(p_node, p_item));
}

HashMap<Node*, SceneTreeEditor::CachedNode>::Iterator SceneTreeEditor::NodeCache::get(
	Node* p_node, bool p_deleted_ok)
{
	if (!p_node) {
		return HashMap<Node*, CachedNode>::Iterator();
	}

	HashMap<Node*, CachedNode>::Iterator I = cache.find(p_node);
	if (I) {
		if (I->value.delete_serial != UINT16_MAX) {
			// Don't give us a node marked for deletion.
			if (!p_deleted_ok) {
				return HashMap<Node*, CachedNode>::Iterator();
			}

			to_delete.erase(&I->value);
			I->value.delete_serial = UINT16_MAX;

			// If we were resurrected from near-death we might have been renamed.
			// Make sure that we are updated properly.
			mark_dirty(p_node);
			mark_children_dirty(p_node, true);
		}
	}

	return I;
}

bool SceneTreeEditor::NodeCache::has(Node* p_node) { return get(p_node, false).operator bool(); }

void SceneTreeEditor::NodeCache::mark_dirty(Node* p_node, bool p_parents)
{
	Node* node = p_node;
	while (node) {
		HashMap<Node*, CachedNode>::Iterator I = cache.find(node);
		if (I) {
			I->value.dirty = true;
		}

		if (!p_parents) {
			break;
		}

		node = node->get_parent();
	}
}

void SceneTreeEditor::NodeCache::mark_children_dirty(Node* p_node, bool p_recursive)
{
	if (!p_node) {
		return;
	}

	int cc = p_node->get_child_count(false);
	for (int i = 0; i < cc; i++) {
		Node* c = p_node->get_child(i, false);
		HashMap<Node*, CachedNode>::Iterator IC = cache.find(c);

		if (IC) {
			IC->value.dirty = true;

			if (p_recursive) {
				mark_children_dirty(c, p_recursive);
			}
		}
	}
}

void SceneTreeEditor::NodeCache::delete_pending()
{
	HashSet<CachedNode*>::Iterator I = to_delete.begin();
	while (I) {
		// We want to keep TreeItems around just long enough for a Node removal,
		// and immediate reinsertion. This is what happens with moves and
		// type changes.
		if (Math::abs((*I)->delete_serial - delete_serial) >= 2) {
			memdelete((*I)->item);
			cache.remove((*I)->cache_iterator);
			to_delete.remove(I);
		}
		else if (!(*I)->removed) {
			// We don't remove from the tree until now because if the node got
			// deleted from a @tool script the SceneTreeEditor might have had it
			// marked or selected before the node was removed. If we immediately
			// remove from the Tree control then we end up trying to scroll to an
			// Item without a parent.
			//
			// We might already be removed (and thus not have a parent) by rapid
			// undo/redo.
			if (!(*I)->removed) {
				TreeItem* parent = (*I)->item->get_parent();
				parent->remove_child((*I)->item);
			}
			(*I)->removed = true;
		}
		++I;
	}

	++delete_serial;
}

void SceneTreeEditor::NodeCache::clear()
{
	for (CachedNode* E : to_delete) {
		// Only removed entries won't be automatically cleaned up by Tree::clear().
		if (E->removed) {
			memdelete(E->item);
		}
	}
	cache.clear();
	to_delete.clear();
	current_pinned_node = nullptr;
	current_has_pin = false;
}


